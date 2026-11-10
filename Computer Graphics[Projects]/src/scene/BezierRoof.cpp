#include "scene/BezierRoof.h"

#include "math/BezierPatch.h"
#include "math/MathUtil.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

// ============================================================================
// WP-3 · BezierRoof.cpp
// 朝鲜族“鹤飞屋”歇山顶：四角飞檐微翘、屋脊两端上扬，由 6 片双三次 Bézier 曲面构成：
//   · 前坡 / 后坡各拆分为左片 L(x∈[-W/2,0]) 与右片 R(x∈[0,W/2])，共 4 片主坡曲面。
//     在 x=0 中线拼接处严格施加 C¹ 连续条件：
//         P^R_{i,0} = P^L_{i,3},   P^R_{i,1} = 2·P^L_{i,3} - P^L_{i,2}   (i = 0..3)
//     控制点索引约定与 BezierPatch::eval 一致：i↔u（檐口→屋脊），j↔v（左檐→中缝/右檐）。
//   · 左右两侧歇山梯形翼坡各 1 片，与主坡沿戗脊 C⁰ 共边——用 de Casteljau 取主坡
//     边缘曲线的 [0, 0.55] 子曲线作翼坡侧边控制点，保证共边逐点重合。
// 檐口剖面“中段平缓、两端起翘”：起翘/外挑剖面在 |x| ≤ W/6 内恒为 0，既形成优雅的
// 鹤翼飞檐，又使 C¹ 公式导出的右片与几何镜像严格一致（拼缝处不出现反向凹陷）。
// 檐口四角控制点恰向上抬升 cornerLift 并沿 z 向外微挑 flare。
// 上部山花板、正脊横梁及两端微翘吻兽装饰用 addBox / addTriangle 补齐。
// ============================================================================

namespace {

// ---- 统一调色板（sRGB -> 线性空间）----
const glm::vec3 kTileColor = MathUtil::srgbToLinear(0x4A5560); // 青灰筒瓦
const glm::vec3 kWoodColor = MathUtil::srgbToLinear(0x5B3A29); // 深褐木构（博风/山花）

// 檐口起翘剖面：|x| ≤ W/6 时为 0（拼缝处切线水平），向角部以 smoothstep 平滑升至 1
float eaveProfile(float t) {
    if (t <= 1.0f / 3.0f) return 0.0f;
    const float u = (t - 1.0f / 3.0f) / (2.0f / 3.0f);
    return u * u * (3.0f - 2.0f * u);
}

// 前坡左片控制点：i=0 檐口 -> i=3 屋脊（u 沿坡向），j=0 左檐 -> j=3 中缝 x=0（v 沿展向）
void buildFrontLeftPatch(float width, float depth, float height, float cornerLift,
                         float flare, float ridgeHalf, glm::vec3 L[4][4]) {
    static const float kRowHeight[4] = {0.00f, 0.24f, 0.62f, 1.00f}; // 檐口->屋脊 高度比例
    static const float kRowDepth[4]  = {1.00f, 0.78f, 0.40f, 0.00f}; // 檐口->屋脊 深度比例
    static const float kRowCurl[4]   = {1.00f, 0.62f, 0.28f, 0.00f}; // 起翘/外挑随高度衰减
    static const float kRowSpan[4]   = {0.00f, 0.30f, 0.72f, 1.00f}; // 各排半跨向脊端收拢（歇山短脊）

    const float halfW = 0.5f * width;
    const float halfD = 0.5f * depth;
    for (int i = 0; i < 4; ++i) {
        const float xEdge = halfW - (halfW - ridgeHalf) * kRowSpan[i];  // 本排左端 |x|
        const float yBase = height * kRowHeight[i];
        const float lift  = cornerLift * kRowCurl[i];
        const float sway  = flare * kRowCurl[i];
        for (int j = 0; j < 4; ++j) {
            const float x = -xEdge + (xEdge / 3.0f) * float(j);         // j=3 处 x=0（中缝）
            const float s = eaveProfile(std::fabs(x) / halfW);
            L[i][j] = glm::vec3(x, yBase + lift * s, -(halfD * kRowDepth[i]) - sway * s);
        }
    }
}

// 由左片导出右片：0/1 列施加 C¹ 连续约束，2/3 列按左右镜像设置
void deriveRightPatch(const glm::vec3 L[4][4], glm::vec3 R[4][4]) {
    for (int i = 0; i < 4; ++i) {
        R[i][0] = L[i][3];                      // P^R_{i,0} = P^L_{i,3}
        R[i][1] = 2.0f * L[i][3] - L[i][2];     // P^R_{i,1} = 2P^L_{i,3} - P^L_{i,2}
        R[i][2] = glm::vec3(-L[i][1].x, L[i][1].y, L[i][1].z);
        R[i][3] = glm::vec3(-L[i][0].x, L[i][0].y, L[i][0].z);
    }
}

// 三次 Bézier 曲线 [0, tEnd] 子曲线控制点（de Casteljau 左段），用于翼坡与主坡 C⁰ 共边
void subdivideEdge(const glm::vec3 p[4], float tEnd, glm::vec3 q[4]) {
    glm::vec3 tmp[4] = {p[0], p[1], p[2], p[3]};
    q[0] = p[0];
    for (int k = 1; k <= 3; ++k) {
        for (int i = 0; i <= 3 - k; ++i) tmp[i] = glm::mix(tmp[i], tmp[i + 1], tEnd);
        q[k] = tmp[0];
    }
}

void tessellatePatch(const glm::vec3 cp[4][4], int steps, const glm::vec3& color,
                     MeshBuilder& builder, const glm::mat4& transform) {
    BezierPatch patch;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j) patch.cp[i][j] = cp[i][j];
    patch.tessellate(steps, color, builder, transform);
}

} // namespace

int BezierRoof::stepsFromLevel(int level) {
    if (level == 0) return 4;
    if (level == 1) return 10;
    return 20;
}

void BezierRoof::build(float width, float depth, float height, float cornerLift, int tessLevel,
                       MeshBuilder& outBuilder, const glm::mat4& localTransform) const {
    const int steps = stepsFromLevel(tessLevel);
    const float halfW = 0.5f * width;
    const float flare = 0.6f * cornerLift;                                  // 檐角水平外挑量
    const float ridgeHalf = std::max(halfW - 0.32f * depth, 0.18f * width); // 正脊半长（歇山短脊）

    // ---- 前坡（z < 0）：左片定义 + 右片按 C¹ 公式导出 ----
    glm::vec3 frontL[4][4], frontR[4][4];
    buildFrontLeftPatch(width, depth, height, cornerLift, flare, ridgeHalf, frontL);
    deriveRightPatch(frontL, frontR);
    tessellatePatch(frontL, steps, kTileColor, outBuilder, localTransform);
    tessellatePatch(frontR, steps, kTileColor, outBuilder, localTransform);

    // ---- 后坡（z > 0）：z 取反并反转坡向行序，保证 Bézier 曲面法线朝外 ----
    glm::vec3 backL[4][4], backR[4][4];
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            backL[i][j] = glm::vec3(frontL[3 - i][j].x, frontL[3 - i][j].y, -frontL[3 - i][j].z);
    deriveRightPatch(backL, backR);
    tessellatePatch(backL, steps, kTileColor, outBuilder, localTransform);
    tessellatePatch(backR, steps, kTileColor, outBuilder, localTransform);

    // ---- 左翼坡（梯形）：主坡左缘戗脊曲线取 [0,0.55] 子曲线，上部留作山花板 ----
    glm::vec3 hipFront[4], hipBack[4];
    for (int i = 0; i < 4; ++i) hipFront[i] = frontL[i][0];   // 前坡左缘：檐角 -> 脊端
    for (int i = 0; i < 4; ++i) hipBack[i]  = backL[3 - i][0]; // 后坡左缘：檐角 -> 脊端
    glm::vec3 edgeF[4], edgeB[4];
    subdivideEdge(hipFront, 0.55f, edgeF);
    subdivideEdge(hipBack, 0.55f, edgeB);

    glm::vec3 hipL[4][4];
    for (int j = 0; j < 4; ++j) {
        for (int i = 0; i < 4; ++i) {
            const float s = float(i) / 3.0f;
            glm::vec3 p = glm::mix(edgeF[j], edgeB[j], s);      // 前后戗脊子曲线逐列插值
            if (j == 0 && (i == 1 || i == 2)) {                 // 翼坡檐口：中部外挑下垂，延续角部飞檐
                p.x = -(halfW + 0.40f * flare);
                p.y = 0.42f * cornerLift;
                p.z = edgeF[0].z * (1.0f - 2.0f * s);
            } else if ((j == 1 || j == 2) && (i == 1 || i == 2)) {
                p.x -= 0.12f * flare;                           // 翼坡瓦作中部微微鼓肚
            }
            hipL[i][j] = p;
        }
    }

    // ---- 右翼坡：镜像 x 并反转行序，保证曲面法线朝外 ----
    glm::vec3 hipR[4][4];
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            hipR[i][j] = glm::vec3(-hipL[3 - i][j].x, hipL[3 - i][j].y, hipL[3 - i][j].z);
    tessellatePatch(hipL, steps, kTileColor, outBuilder, localTransform);
    tessellatePatch(hipR, steps, kTileColor, outBuilder, localTransform);

    // ---- 山花板：戗脊 55% 高度处至脊端的木质三角板（正反两片薄板）----
    const glm::vec3 apexL(-ridgeHalf, height, 0.0f);
    const glm::vec3 topF = edgeF[3];
    const glm::vec3 topB = edgeB[3];
    const float boardT = 0.045f;
    const glm::vec3 nGableL = glm::normalize(glm::cross(topB - topF, apexL - topF));
    outBuilder.addTriangle(topF + nGableL * boardT, topB + nGableL * boardT, apexL + nGableL * boardT,
                           kWoodColor, localTransform);
    outBuilder.addTriangle(topF - nGableL * boardT, apexL - nGableL * boardT, topB - nGableL * boardT,
                           kWoodColor, localTransform);
    const auto mirrorX = [](const glm::vec3& p) { return glm::vec3(-p.x, p.y, p.z); };
    const glm::vec3 apexR = mirrorX(apexL);
    const glm::vec3 tF = mirrorX(topF);
    const glm::vec3 tB = mirrorX(topB);
    const glm::vec3 nGableR = glm::normalize(glm::cross(tB - tF, apexR - tF));
    outBuilder.addTriangle(tF + nGableR * boardT, tB + nGableR * boardT, apexR + nGableR * boardT,
                           kWoodColor, localTransform);
    outBuilder.addTriangle(tF - nGableR * boardT, apexR - nGableR * boardT, tB - nGableR * boardT,
                           kWoodColor, localTransform);

    // ---- 正脊横梁（盒体）与两端微翘吻兽装饰 ----
    const glm::mat4 LT = localTransform;
    outBuilder.addBox(LT * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, height + 0.07f, 0.0f))
                          * glm::scale(glm::mat4(1.0f), glm::vec3(2.0f * ridgeHalf + 0.35f, 0.16f, 0.22f)),
                      kTileColor);
    for (int side = -1; side <= 1; side += 2) {
        const float xr = side * (ridgeHalf + 0.10f);
        outBuilder.addBox(LT * glm::translate(glm::mat4(1.0f), glm::vec3(xr, height + 0.20f, 0.0f))
                              * glm::scale(glm::mat4(1.0f), glm::vec3(0.16f, 0.30f, 0.26f)),
                          kTileColor); // 吻兽身
        outBuilder.addBox(LT * glm::translate(glm::mat4(1.0f), glm::vec3(xr + side * 0.05f, height + 0.40f, 0.0f))
                              * glm::scale(glm::mat4(1.0f), glm::vec3(0.10f, 0.14f, 0.18f)),
                          kTileColor); // 吻首上翘
    }
}
