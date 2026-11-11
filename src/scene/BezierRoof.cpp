#include "scene/BezierRoof.h"

#include "math/BezierPatch.h"
#include "math/MathUtil.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <utility>
#include <vector>

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

constexpr float kGableSplit = 0.55f; // 翼坡占戗脊曲线的参数比例，其上为山花板

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

// 按给定 u / v 参数列表细分曲面（索引绕序与 BezierPatch::tessellate 一致）
void tessellatePatchParams(const glm::vec3 cp[4][4], const std::vector<float>& us, const std::vector<float>& vs,
                           const glm::vec3& color, MeshBuilder& builder, const glm::mat4& transform) {
    BezierPatch patch;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j) patch.cp[i][j] = cp[i][j];
    const uint32_t base = static_cast<uint32_t>(builder.verts.size());
    const glm::mat3 normalTransform = MathUtil::normalMatrix(transform);
    for (float v : vs) {
        for (float u : us) {
            glm::vec3 normal;
            const glm::vec3 position = patch.eval(u, v, normal);
            glm::vec3 n = normalTransform * normal;
            const float len = glm::length(n);
            n = len < 1e-6f ? glm::vec3(0.0f, 1.0f, 0.0f) : n / len;
            builder.verts.push_back({glm::vec3(transform * glm::vec4(position, 1.0f)), n, glm::vec2(u, v), color});
        }
    }
    const uint32_t stride = static_cast<uint32_t>(us.size());
    for (std::size_t y = 0; y + 1 < vs.size(); ++y) {
        for (std::size_t x = 0; x + 1 < us.size(); ++x) {
            const uint32_t i0 = base + static_cast<uint32_t>(y) * stride + static_cast<uint32_t>(x);
            const uint32_t i1 = i0 + 1, i2 = i0 + stride, i3 = i2 + 1;
            builder.indices.insert(builder.indices.end(), {i0, i1, i2, i1, i3, i2});
        }
    }
}

void sortUnique(std::vector<float>& ts) {
    std::sort(ts.begin(), ts.end());
    ts.erase(std::unique(ts.begin(), ts.end(), [](float a, float b) { return b - a < 1e-5f; }), ts.end());
}

// 三次 Bézier 曲线求值
glm::vec3 evalCurve(const glm::vec3 p[4], float t) {
    const float o = 1.0f - t;
    return o * o * o * p[0] + 3.0f * t * o * o * p[1] + 3.0f * t * t * o * p[2] + t * t * t * p[3];
}

// 6 片曲面控制点（屋顶局部坐标，檐口基面 y=0）
struct RoofPatches {
    glm::vec3 frontL[4][4], frontR[4][4], backL[4][4], backR[4][4], hipL[4][4], hipR[4][4];
    glm::vec3 hipFront[4];  // 前坡左缘戗脊曲线：檐角 -> 脊端
    glm::vec3 edgeF[4], edgeB[4];
    float ridgeHalf = 0.0f;
};

RoofPatches buildRoofPatches(float width, float depth, float height, float cornerLift) {
    RoofPatches r;
    const float halfW = 0.5f * width;
    const float flare = 0.6f * cornerLift;                                  // 檐角水平外挑量
    r.ridgeHalf = std::max(halfW - 0.32f * depth, 0.18f * width);           // 正脊半长（歇山短脊）

    // ---- 前坡（z < 0）：左片定义 + 右片按 C¹ 公式导出 ----
    buildFrontLeftPatch(width, depth, height, cornerLift, flare, r.ridgeHalf, r.frontL);
    deriveRightPatch(r.frontL, r.frontR);

    // ---- 后坡（z > 0）：z 取反并反转坡向行序，保证 Bézier 曲面法线朝外 ----
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            r.backL[i][j] = glm::vec3(r.frontL[3 - i][j].x, r.frontL[3 - i][j].y, -r.frontL[3 - i][j].z);
    deriveRightPatch(r.backL, r.backR);

    // ---- 左翼坡（梯形）：主坡左缘戗脊曲线取 [0,0.55] 子曲线，上部留作山花板 ----
    glm::vec3 hipBack[4];
    for (int i = 0; i < 4; ++i) r.hipFront[i] = r.frontL[i][0];  // 前坡左缘：檐角 -> 脊端
    for (int i = 0; i < 4; ++i) hipBack[i] = r.backL[3 - i][0];  // 后坡左缘：檐角 -> 脊端
    subdivideEdge(r.hipFront, kGableSplit, r.edgeF);
    subdivideEdge(hipBack, kGableSplit, r.edgeB);

    for (int j = 0; j < 4; ++j) {
        for (int i = 0; i < 4; ++i) {
            const float s = float(i) / 3.0f;
            glm::vec3 p = glm::mix(r.edgeF[j], r.edgeB[j], s);  // 前后戗脊子曲线逐列插值
            if (j == 0 && (i == 1 || i == 2)) {                 // 翼坡檐口：中部外挑下垂，延续角部飞檐
                p.x = -(halfW + 0.40f * flare);
                p.y = 0.42f * cornerLift;
                p.z = r.edgeF[0].z * (1.0f - 2.0f * s);
            } else if ((j == 1 || j == 2) && (i == 1 || i == 2)) {
                p.x -= 0.12f * flare;                           // 翼坡瓦作中部微微鼓肚
            }
            r.hipL[i][j] = p;
        }
    }

    // ---- 右翼坡：镜像 x 并反转行序，保证曲面法线朝外 ----
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            r.hipR[i][j] = glm::vec3(-r.hipL[3 - i][j].x, r.hipL[3 - i][j].y, r.hipL[3 - i][j].z);
    return r;
}

// [1.0reborn] 共边对齐的细分参数
// 翼坡侧缘是主坡侧缘曲线的 [0, 0.55] 子段。原先两者各自均匀细分：主坡顶点在 u=k/n，
// 翼坡顶点在 u=0.55·k/n，共边上的顶点互不重合，4x4 细分时裂缝最宽约 0.8 cm。
// 这里让主坡 u 参数同时包含 k/n 与 0.55·k/n（及其关于 0.5 的对称值，供后坡使用），
// 翼坡沿戗脊的参数取对应的 u/0.55，使共边两侧顶点逐个重合。
struct RoofParams {
    std::vector<float> mainU;   // 主坡：檐口 -> 屋脊
    std::vector<float> uniform; // 主坡展向 / 翼坡前后向：均匀 k/n
    std::vector<float> hipV;    // 翼坡：沿戗脊子曲线
    std::vector<float> gableU;  // 山花板：主坡侧缘曲线 [0.55, 1] 段的采样参数
};

RoofParams buildRoofParams(int steps) {
    RoofParams p;
    for (int k = 0; k <= steps; ++k) {
        const float t = float(k) / float(steps);
        p.uniform.push_back(t);
        p.mainU.push_back(t);
        p.mainU.push_back(kGableSplit * t);
        p.mainU.push_back(1.0f - kGableSplit * t);
    }
    sortUnique(p.mainU);
    for (float u : p.mainU) {
        if (u <= kGableSplit + 1e-5f) p.hipV.push_back(std::min(u / kGableSplit, 1.0f));
        if (u >= kGableSplit - 1e-5f) p.gableU.push_back(std::max(u, kGableSplit));
    }
    sortUnique(p.hipV);
    sortUnique(p.gableU);
    return p;
}

void tessellateRoofShell(const RoofPatches& r, const RoofParams& p, const glm::vec3& color,
                         MeshBuilder& builder, const glm::mat4& transform) {
    tessellatePatchParams(r.frontL, p.mainU, p.uniform, color, builder, transform);
    tessellatePatchParams(r.frontR, p.mainU, p.uniform, color, builder, transform);
    tessellatePatchParams(r.backL, p.mainU, p.uniform, color, builder, transform);
    tessellatePatchParams(r.backR, p.mainU, p.uniform, color, builder, transform);
    tessellatePatchParams(r.hipL, p.uniform, p.hipV, color, builder, transform);
    tessellatePatchParams(r.hipR, p.uniform, p.hipV, color, builder, transform);
}

// 竖直射线与三角网求交，返回 (x,z) 处最低的屋面高度；无交点返回 false
bool lowestSurfaceY(const MeshBuilder& shell, float x, float z, float& outY) {
    bool hit = false;
    for (std::size_t k = 0; k + 2 < shell.indices.size(); k += 3) {
        const glm::vec3& a = shell.verts[shell.indices[k]].pos;
        const glm::vec3& b = shell.verts[shell.indices[k + 1]].pos;
        const glm::vec3& c = shell.verts[shell.indices[k + 2]].pos;
        const float det = (b.x - a.x) * (c.z - a.z) - (c.x - a.x) * (b.z - a.z);
        if (std::fabs(det) < 1e-12f) continue;
        const float l1 = ((x - a.x) * (c.z - a.z) - (c.x - a.x) * (z - a.z)) / det;
        const float l2 = ((b.x - a.x) * (z - a.z) - (x - a.x) * (b.z - a.z)) / det;
        const float l0 = 1.0f - l1 - l2;
        constexpr float kTol = -1e-5f;
        if (l0 < kTol || l1 < kTol || l2 < kTol) continue;
        const float y = l0 * a.y + l1 * b.y + l2 * c.y;
        if (!hit || y < outY) outY = y;
        hit = true;
    }
    return hit;
}

// 2D 线段 p->q 与 a->b 相交时返回 p->q 上的参数 t
bool segmentParam(const glm::vec2& p, const glm::vec2& q, const glm::vec2& a, const glm::vec2& b, float& t) {
    const glm::vec2 r = q - p, s = b - a;
    const float den = r.x * s.y - r.y * s.x;
    if (std::fabs(den) < 1e-12f) return false;
    const glm::vec2 w = a - p;
    t = (w.x * s.y - w.y * s.x) / den;
    const float u = (w.x * r.y - w.y * r.x) / den;
    return t >= 0.0f && t <= 1.0f && u >= 0.0f && u <= 1.0f;
}

// 按期望朝向添加三角形（MeshBuilder::addTriangle 依绕序求面法线）
void addOrientedTriangle(MeshBuilder& b, glm::vec3 p0, glm::vec3 p1, glm::vec3 p2,
                         const glm::vec3& outward, const glm::vec3& color, const glm::mat4& M) {
    if (glm::dot(glm::cross(p1 - p0, p2 - p0), outward) < 0.0f) std::swap(p1, p2);
    b.addTriangle(p0, p1, p2, color, M);
}

} // namespace

int BezierRoof::stepsFromLevel(int level) {
    if (level == 0) return 4;
    if (level == 1) return 10;
    return 20;
}

void BezierRoof::build(float width, float depth, float height, float cornerLift, int tessLevel,
                       MeshBuilder& outBuilder, const glm::mat4& localTransform) const {
    const RoofPatches r = buildRoofPatches(width, depth, height, cornerLift);
    const RoofParams params = buildRoofParams(stepsFromLevel(tessLevel));
    const float ridgeHalf = r.ridgeHalf;

    // ---- 6 片 Bézier 曲面：前/后坡 L+R（C¹ 拼接）+ 左/右翼坡（与主坡 C⁰ 共边，顶点对齐）----
    tessellateRoofShell(r, params, kTileColor, outBuilder, localTransform);

    // ---- 山花板：戗脊曲线 [0.55, 1] 段至脊端 ----
    // [1.0reborn] 修复漏缝：原先为平面三角形，斜边是直线，而主坡侧缘是三次曲线，
    // 两者之间留下最宽约 3.7 cm 的楔形缝。现改为沿戗脊曲线逐段采样的条带：
    // 前后对称点 (x,y,±z) 两两成对，外表面顶点直接取在曲线上，与主坡侧缘逐点重合；
    // 底边即翼坡顶边 edgeF[3] -> edgeB[3]。木板厚度向屋内偏移，不影响外表面贴合。
    const float boardT = 0.045f;
    const glm::vec3 apexL(-ridgeHalf, height, 0.0f);
    glm::vec3 nOutL = glm::normalize(glm::cross(r.edgeB[3] - r.edgeF[3], apexL - r.edgeF[3]));
    if (nOutL.x > 0.0f) nOutL = -nOutL;  // 左山花板外法线朝 -X
    // 采样参数与主坡侧缘顶点的 u 参数一一对应，所以板边落在主坡网格边上
    const int kGableSegs = static_cast<int>(params.gableU.size()) - 1;
    std::vector<glm::vec3> rail(params.gableU.size());
    for (int k = 0; k <= kGableSegs; ++k) rail[k] = evalCurve(r.hipFront, params.gableU[k]);
    const auto backOf = [](const glm::vec3& p) { return glm::vec3(p.x, p.y, -p.z); };
    for (int side = -1; side <= 1; side += 2) {
        const glm::vec3 m(float(side) * -1.0f, 1.0f, 1.0f);  // side=-1 左山花，side=+1 镜像到右侧
        const glm::vec3 nOut = nOutL * m;
        for (int pass = 0; pass < 2; ++pass) {               // pass 0 外表面，pass 1 内表面
            const glm::vec3 off = pass == 0 ? glm::vec3(0.0f) : -nOut * boardT;
            const glm::vec3 facing = pass == 0 ? nOut : -nOut;
            for (int k = 0; k < kGableSegs; ++k) {
                const glm::vec3 f0 = rail[k] * m + off, f1 = rail[k + 1] * m + off;
                const glm::vec3 b0 = backOf(rail[k]) * m + off, b1 = backOf(rail[k + 1]) * m + off;
                if (k == 0) {
                    // 底边与翼坡顶边共边：翼坡顶边是 edgeF[3]->edgeB[3] 的直线，按 uniform 参数
                    // 分段。底边取同样的分点并扇形连到 f1，避免 T 形接缝
                    const auto& us = params.uniform;
                    for (std::size_t s = 0; s + 1 < us.size(); ++s) {
                        const glm::vec3 p0 = glm::mix(f0, b0, us[s]);
                        const glm::vec3 p1 = glm::mix(f0, b0, us[s + 1]);
                        addOrientedTriangle(outBuilder, p0, p1, f1, facing, kWoodColor, localTransform);
                    }
                } else {
                    addOrientedTriangle(outBuilder, f0, b0, f1, facing, kWoodColor, localTransform);
                }
                if (k + 1 < kGableSegs) {                    // 最后一段在脊端收为三角形
                    addOrientedTriangle(outBuilder, f1, b0, b1, facing, kWoodColor, localTransform);
                }
            }
        }
    }

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

// ---------------------------------------------------------------------------
// [1.0reborn] 封檐垫板：封住墙顶与屋面之间的空隙
// 墙顶（檐口基面 y=0）到屋面下表面在墙线处仍有 0.59 ~ 1.03 m 的空隙，
// 从侧面可以直接看穿到屋内。这里沿墙体四周竖起一圈板：
//   1. 用与 build() 相同的曲面和细分级别生成屋面三角网；
//   2. 求墙线与三角网各边在 XZ 平面上的全部交点，并加入四个墙角；
//   3. 在每个交点处取正下方的最低屋面高度作为板顶。
// 交点之间屋面在墙线上是线性的，所以板顶折线和屋面网格严格重合，不留缝也不穿出瓦面。
// ---------------------------------------------------------------------------
void BezierRoof::buildEaveSeal(float width, float depth, float height, float cornerLift, int tessLevel,
                               float wallHalfX, float wallHalfZ, const glm::vec3& color,
                               MeshBuilder& outBuilder, const glm::mat4& localTransform) {
    const RoofPatches r = buildRoofPatches(width, depth, height, cornerLift);
    MeshBuilder shell;  // 与 build() 完全相同的屋面三角网（屋顶局部坐标）
    tessellateRoofShell(r, buildRoofParams(stepsFromLevel(tessLevel)), kTileColor, shell, glm::mat4(1.0f));

    constexpr float kBottom = 0.0f;    // 与墙顶共边：与墙面同一平面，不重叠也不留缝
    const glm::vec2 corners[4] = {
        {-wallHalfX, -wallHalfZ}, {wallHalfX, -wallHalfZ}, {wallHalfX, wallHalfZ}, {-wallHalfX, wallHalfZ}
    };
    for (int e = 0; e < 4; ++e) {
        const glm::vec2 p = corners[e];
        const glm::vec2 q = corners[(e + 1) % 4];

        std::vector<float> ts = {0.0f, 1.0f};
        for (std::size_t k = 0; k + 2 < shell.indices.size(); k += 3) {
            for (int s = 0; s < 3; ++s) {
                const glm::vec3& a = shell.verts[shell.indices[k + s]].pos;
                const glm::vec3& b = shell.verts[shell.indices[k + (s + 1) % 3]].pos;
                float t;
                if (segmentParam(p, q, glm::vec2(a.x, a.z), glm::vec2(b.x, b.z), t)) ts.push_back(t);
            }
        }
        sortUnique(ts);

        // 外法线：墙线 p->q 按逆时针（俯视）排列，外侧为其右手方向
        const glm::vec2 dir = q - p;
        const glm::vec3 outward(dir.y, 0.0f, -dir.x);
        glm::vec3 prevTop(0.0f), prevBot(0.0f);
        bool havePrev = false;
        for (float t : ts) {
            const glm::vec2 xz = p + dir * t;
            float y;
            if (!lowestSurfaceY(shell, xz.x, xz.y, y)) { havePrev = false; continue; }
            const glm::vec3 top(xz.x, y, xz.y);
            const glm::vec3 bot(xz.x, kBottom, xz.y);
            if (havePrev) {
                addOrientedTriangle(outBuilder, prevBot, bot, top, outward, color, localTransform);
                addOrientedTriangle(outBuilder, prevBot, top, prevTop, outward, color, localTransform);
            }
            prevTop = top;
            prevBot = bot;
            havePrev = true;
        }
    }
}
