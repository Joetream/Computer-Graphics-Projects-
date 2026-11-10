#include "scene/HierarchyObjects.h"

#include "math/MatrixStack.h"
#include "math/MathUtil.h"
#include "math/BSplinePath.h"
#include "core/MeshBuilder.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

// ============================================================================
// WP-3 · HierarchyObjects.cpp
// 全部层级动画物体：每帧用 MatrixStack 计算父子变换链，逐部件设置
// u_Model / u_NormalMat 后绘制（顶点色材质，无纹理）。
//   1. 三座现代风力发电机：塔架 -> 机舱(缓慢偏航 ψ=12°sin(0.25t)) -> 轮毂(旋转 φ)
//      -> 3 片细长扇叶（间隔 120°，绕局部 Z 轴即风轮轴旋转）。
//   2. 民俗高秋千架：A 字形双木立柱与横梁 -> 悬绳+踏板+朝鲜族裙装小人
//      绕横梁做简谐摆动 θ(t)=32°sin(2.1t)（绕局部 X 轴）。
//   3. 民俗跳板：中心支点木墩 -> 长木板绕 Z 轴翘动 α(t)=14°sin(3.2t)，
//      两端小人固结于板上随板交替弹起。
//   4. 六组追日太阳能光伏阵列：金属立柱 -> 深蓝光伏板绕 X 轴实时朝向太阳仰角。
//   5. 观光游览车：车身沿 B 样条轨道匀速前进并朝向切线方向，
//      4 个车轮作为子节点绕局部 X 轴高速滚动。
// ============================================================================

namespace {

// ---- 统一调色板（sRGB -> 线性空间）----
const glm::vec3 kWallColor    = MathUtil::srgbToLinear(0xF2EFE8); // 暖白（短袄/袖）
const glm::vec3 kTileColor    = MathUtil::srgbToLinear(0x4A5560); // 青灰（风挡/轮胎）
const glm::vec3 kWoodColor    = MathUtil::srgbToLinear(0x5B3A29); // 深褐木构
const glm::vec3 kStoneColor   = MathUtil::srgbToLinear(0x9A958C); // 石垫/金属灰
const glm::vec3 kAzaleaColor  = MathUtil::srgbToLinear(0xE07AA0); // 金达莱粉紫（裙/蓬顶/把手）
const glm::vec3 kTurbineWhite = MathUtil::srgbToLinear(0xE6E9EC); // 风机纯白机身
const glm::vec3 kSolarBlue    = MathUtil::srgbToLinear(0x2B3A55); // 光伏板深蓝硅片
const glm::vec3 kHairColor    = MathUtil::srgbToLinear(0x5B3A29); // 发髻深褐

// ---- 动画参数 ----
constexpr float kRotorSpeed      = 1.7f;  // 风轮角速度 (rad/s)
constexpr float kYawAmplitude    = 12.0f; // 机舱偏航幅值 (deg)
constexpr float kYawOmega        = 0.25f; // 机舱偏航角频率 (rad/s)
constexpr float kSwingAmplitude  = 32.0f; // 秋千摆幅 (deg)
constexpr float kSwingOmega      = 2.1f;  // 秋千角频率 (rad/s)
constexpr float kSeesawAmplitude = 14.0f; // 跳板翘动幅值 (deg)
constexpr float kSeesawOmega     = 3.2f;  // 跳板角频率 (rad/s)
constexpr float kCartSpeed       = 3.4f;  // 观光车线速度 (m/s)
constexpr float kWheelRadius     = 0.35f; // 车轮半径 (m)

constexpr float kSolarPitchMin = 14.0f;   // 夜间/低阳保持的最低俯仰角 (deg)
constexpr float kSolarPitchMax = 58.0f;   // 正午最大俯仰角 (deg)

// ---- 变换小工具 ----
glm::mat4 translateTo(float x, float y, float z) {
    return glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
}
glm::mat4 boxAt(float x, float y, float z, float sx, float sy, float sz) {
    return translateTo(x, y, z) * glm::scale(glm::mat4(1.0f), glm::vec3(sx, sy, sz));
}
glm::mat4 rotateXDeg(float deg) {
    return glm::rotate(glm::mat4(1.0f), glm::radians(deg), glm::vec3(1.0f, 0.0f, 0.0f));
}
glm::mat4 rotateYDeg(float deg) {
    return glm::rotate(glm::mat4(1.0f), glm::radians(deg), glm::vec3(0.0f, 1.0f, 0.0f));
}
glm::mat4 rotateZDeg(float deg) {
    return glm::rotate(glm::mat4(1.0f), glm::radians(deg), glm::vec3(0.0f, 0.0f, 1.0f));
}

} // namespace

// ---------------------------------------------------------------------------
// init：程序化生成全部部件网格（各部件局部原点约定见注释）
// ---------------------------------------------------------------------------
void HierarchyObjects::init() {
    MeshBuilder b;

    // ---- 风机塔架（局部原点=地面中心，塔高 18 m）----
    b.addBox(boxAt(0.0f, 0.20f, 0.0f, 2.80f, 0.40f, 2.80f), kStoneColor);          // 基座
    b.addCylinder(glm::mat4(1.0f), 1.05f, 0.50f, 18.0f, 14, kTurbineWhite);        // 圆台塔身
    b.addCylinder(translateTo(0.0f, 18.0f, 0.0f), 0.62f, 0.62f, 0.25f, 12, kStoneColor); // 顶部法兰
    b.addBox(boxAt(0.0f, 1.00f, 0.96f, 0.70f, 1.60f, 0.10f), kStoneColor);         // 检修门
    b.build(m_towerMesh);
    b.clear();

    // ---- 机舱（局部原点=塔顶安装面，机头朝 +Z）----
    b.addBox(boxAt(0.0f, 0.0f, -0.55f, 1.15f, 1.05f, 2.60f), kTurbineWhite);       // 舱体
    b.addBox(boxAt(0.0f, 0.38f, -1.60f, 0.80f, 0.40f, 0.50f), kStoneColor);        // 尾部散热格栅
    b.addCylinder(rotateXDeg(90.0f) * translateTo(0.0f, 0.50f, 0.0f), 0.42f, 0.36f, 0.60f, 10,
                  kTurbineWhite);                                                  // 机头环 z 0.5..1.1
    b.build(m_nacelleMesh);
    b.clear();

    // ---- 轮毂（局部原点=轮毂中心，+Z 为风轮轴）----
    b.addCylinder(rotateXDeg(90.0f) * translateTo(0.0f, -0.30f, 0.0f), 0.34f, 0.30f, 0.60f, 10,
                  kTurbineWhite);                                                  // 锥形轮毂
    b.addCylinder(rotateXDeg(90.0f) * translateTo(0.0f, 0.28f, 0.0f), 0.30f, 0.06f, 0.45f, 10,
                  kStoneColor);                                                    // 整流罩
    b.build(m_hubMesh);
    b.clear();

    // ---- 扇叶（局部原点=叶根，沿 +Y 伸展 7.8 m）----
    b.addBox(boxAt(0.0f, 0.55f, 0.0f, 0.60f, 1.10f, 0.18f), kTurbineWhite);        // 叶根
    b.addBox(boxAt(0.0f, 3.60f, 0.0f, 0.46f, 5.00f, 0.13f), kTurbineWhite);        // 叶身
    b.addBox(boxAt(0.0f, 6.95f, 0.0f, 0.22f, 1.70f, 0.10f), kTurbineWhite);        // 叶尖
    b.build(m_bladeMesh);
    b.clear();

    // ---- 秋千架（局部原点=地面中心，架高约 5.2 m）----
    const float legTop = 5.02f;
    const float legSpread = 1.15f;
    const float legLen = std::sqrt(legTop * legTop + legSpread * legSpread);
    const float legTilt = std::atan2(legSpread, legTop); // 立柱自铅垂外倾角 (rad)
    for (int sx = -1; sx <= 1; sx += 2) {
        for (int sz = -1; sz <= 1; sz += 2) {
            const glm::mat4 leg = translateTo(1.55f * float(sx), legTop * 0.5f, legSpread * 0.5f * float(sz))
                                * glm::rotate(glm::mat4(1.0f), -float(sz) * legTilt, glm::vec3(1.0f, 0.0f, 0.0f))
                                * glm::scale(glm::mat4(1.0f), glm::vec3(0.16f, legLen, 0.16f));
            b.addBox(leg, kWoodColor);                                             // A 形斜柱
            b.addBox(boxAt(1.55f * float(sx), 0.08f, legSpread * float(sz), 0.36f, 0.16f, 0.36f),
                     kStoneColor);                                                 // 柱脚垫石
        }
        b.addBox(boxAt(1.55f * float(sx), 2.10f, 0.0f, 0.13f, 0.13f, 1.90f), kWoodColor); // A 形横撑
        b.addBox(boxAt(1.78f * float(sx), 4.78f, 0.0f, 0.05f, 0.55f, 0.02f), kAzaleaColor); // 金达莱彩带
        b.addBox(boxAt(1.78f * float(sx), 5.02f, 0.0f, 0.09f, 0.09f, 0.05f), kAzaleaColor); // 彩带结
    }
    b.addBox(boxAt(0.0f, 5.07f, 0.0f, 3.70f, 0.22f, 0.22f), kWoodColor);           // 顶部横梁（支点 y=5.07）
    b.build(m_swingFrameMesh);
    b.clear();

    // ---- 秋千摆动总成（局部原点=横梁支点，静止时铅垂向下）----
    for (int s = -1; s <= 1; s += 2) {
        b.addCylinder(translateTo(0.42f * float(s), -3.85f, 0.0f), 0.030f, 0.030f, 3.85f, 6,
                      kWoodColor);                                                 // 双悬绳
    }
    b.addBox(boxAt(0.0f, -3.88f, 0.0f, 1.05f, 0.07f, 0.30f), kWoodColor);          // 踏板
    b.addCylinder(translateTo(0.0f, -3.86f, 0.0f), 0.30f, 0.13f, 0.58f, 10, kAzaleaColor); // 彩裙
    b.addBox(boxAt(0.0f, -3.07f, 0.0f, 0.30f, 0.30f, 0.20f), kWallColor);          // 白色短袄
    b.addCylinder(translateTo(0.0f, -2.92f, 0.0f), 0.105f, 0.115f, 0.20f, 8, kWallColor);  // 头部
    b.addCylinder(translateTo(0.0f, -2.74f, 0.0f), 0.115f, 0.09f, 0.09f, 8, kHairColor);   // 发髻
    b.addBox(translateTo(0.0f, -3.12f, -0.13f) * rotateXDeg(12.0f)
                 * glm::scale(glm::mat4(1.0f), glm::vec3(0.04f, 0.40f, 0.02f)), kAzaleaColor); // 背后辫带
    for (int s = -1; s <= 1; s += 2) {
        b.addBox(translateTo(0.24f * float(s), -3.16f, 0.02f) * rotateZDeg(-30.0f * float(s))
                     * glm::scale(glm::mat4(1.0f), glm::vec3(0.07f, 0.85f, 0.07f)), kWallColor); // 扶绳手臂
    }
    b.build(m_swingSeatMesh);
    b.clear();

    // ---- 跳板支点（局部原点=地面中心）----
    b.addBox(boxAt(0.0f, 0.05f, 0.0f, 0.90f, 0.10f, 0.90f), kStoneColor);          // 底垫
    b.addBox(boxAt(0.0f, 0.21f, 0.0f, 0.55f, 0.42f, 0.70f), kWoodColor);           // 支点木墩
    for (int s = -1; s <= 1; s += 2) {
        b.addBox(boxAt(0.29f * float(s), 0.21f, 0.0f, 0.03f, 0.30f, 0.72f), kAzaleaColor); // 粉漆腰线
    }
    b.build(m_seesawBaseMesh);
    b.clear();

    // ---- 跷板（局部原点=支点铰心，板沿 X，两端小人随板交替弹起）----
    b.addBox(boxAt(0.0f, 0.0f, 0.0f, 4.90f, 0.09f, 0.40f), kWoodColor);            // 长木板
    for (int s = -1; s <= 1; s += 2) {
        b.addBox(boxAt(2.15f * float(s), 0.075f, 0.0f, 0.50f, 0.05f, 0.42f), kAzaleaColor); // 座垫
        b.addBox(boxAt(2.15f * float(s), 0.24f, 0.0f, 0.08f, 0.34f, 0.08f), kAzaleaColor);  // 把手立柱
        b.addBox(boxAt(2.15f * float(s), 0.43f, 0.0f, 0.06f, 0.05f, 0.56f), kAzaleaColor);  // 把手横杆
    }
    b.addCylinder(translateTo(-1.62f, 0.045f, 0.0f), 0.16f, 0.07f, 0.42f, 8, kAzaleaColor); // 粉裙女孩
    b.addBox(boxAt(-1.62f, 0.60f, 0.0f, 0.18f, 0.22f, 0.14f), kWallColor);
    b.addCylinder(translateTo(-1.62f, 0.79f, 0.0f), 0.07f, 0.075f, 0.14f, 6, kWallColor);
    b.addCylinder(translateTo(-1.62f, 0.89f, 0.0f), 0.075f, 0.05f, 0.07f, 6, kHairColor);
    b.addBox(boxAt(1.62f, 0.20f, 0.0f, 0.20f, 0.31f, 0.15f), kTileColor);          // 白袄男孩
    b.addBox(boxAt(1.62f, 0.52f, 0.0f, 0.22f, 0.26f, 0.16f), kWallColor);
    b.addCylinder(translateTo(1.62f, 0.72f, 0.0f), 0.07f, 0.075f, 0.14f, 6, kWallColor);
    b.addCylinder(translateTo(1.62f, 0.82f, 0.0f), 0.075f, 0.05f, 0.07f, 6, kHairColor);
    b.build(m_seesawPlankMesh);
    b.clear();

    // ---- 光伏立柱（局部原点=地面中心，柱顶铰心 y=1.42）----
    b.addBox(boxAt(0.0f, 0.06f, 0.0f, 0.50f, 0.12f, 0.50f), kStoneColor);          // 底座
    b.addCylinder(glm::mat4(1.0f), 0.085f, 0.065f, 1.38f, 10, kStoneColor);        // 金属立柱
    b.addBox(boxAt(0.0f, 1.42f, 0.0f, 0.16f, 0.10f, 0.16f), kStoneColor);          // 铰支座
    b.build(m_solarPoleMesh);
    b.clear();

    // ---- 光伏板（局部原点=铰心，绕 X 轴俯仰，0° 时板面朝上）----
    b.addBox(boxAt(0.0f, 0.02f, 0.0f, 2.70f, 0.07f, 1.70f), kSolarBlue);           // 深蓝硅片
    b.addBox(boxAt(-1.37f, 0.02f, 0.0f, 0.06f, 0.10f, 1.76f), kStoneColor);        // 铝边框
    b.addBox(boxAt(1.37f, 0.02f, 0.0f, 0.06f, 0.10f, 1.76f), kStoneColor);
    b.addBox(boxAt(0.0f, 0.02f, -0.87f, 2.78f, 0.10f, 0.06f), kStoneColor);
    b.addBox(boxAt(0.0f, 0.02f, 0.87f, 2.78f, 0.10f, 0.06f), kStoneColor);
    for (int i = -1; i <= 1; i += 2) {                                             // 电池栅线
        b.addBox(boxAt(0.90f * float(i), 0.062f, 0.0f, 0.03f, 0.012f, 1.62f), kStoneColor);
    }
    b.addBox(boxAt(0.0f, 0.062f, 0.0f, 2.62f, 0.012f, 0.03f), kStoneColor);
    b.build(m_solarPanelMesh);
    b.clear();

    // ---- 观光车车身（局部原点=地面中心，车头朝 +Z）----
    b.addBox(boxAt(0.0f, 0.55f, 0.0f, 1.50f, 0.16f, 3.10f), kWoodColor);           // 底盘
    b.addBox(boxAt(0.0f, 0.95f, 1.50f, 1.50f, 0.70f, 0.12f), kWoodColor);          // 前围
    b.addBox(boxAt(0.0f, 0.95f, -1.50f, 1.50f, 0.70f, 0.12f), kWoodColor);         // 后围
    b.addBox(boxAt(-0.69f, 0.95f, 0.0f, 0.12f, 0.70f, 3.10f), kWoodColor);         // 侧围
    b.addBox(boxAt(0.69f, 0.95f, 0.0f, 0.12f, 0.70f, 3.10f), kWoodColor);
    for (int s = -1; s <= 1; s += 2) {
        b.addBox(boxAt(0.705f * float(s), 0.72f, 0.0f, 0.02f, 0.12f, 3.12f), kAzaleaColor); // 金达莱腰线
    }
    b.addBox(boxAt(0.0f, 0.78f, 0.45f, 1.26f, 0.10f, 0.40f), kWoodColor);          // 长椅×2
    b.addBox(boxAt(0.0f, 0.78f, -0.85f, 1.26f, 0.10f, 0.40f), kWoodColor);
    b.addBox(boxAt(0.0f, 1.02f, 0.68f, 1.26f, 0.42f, 0.08f), kWoodColor);          // 椅背×2
    b.addBox(boxAt(0.0f, 1.02f, -0.62f, 1.26f, 0.42f, 0.08f), kWoodColor);
    b.addBox(boxAt(0.0f, 1.12f, 1.30f, 1.50f, 0.14f, 0.30f), kWoodColor);          // 驾驶台
    b.addBox(translateTo(0.0f, 1.55f, 1.42f) * rotateXDeg(-14.0f)
                 * glm::scale(glm::mat4(1.0f), glm::vec3(1.40f, 0.60f, 0.06f)), kTileColor); // 前风挡
    for (int sx2 = -1; sx2 <= 1; sx2 += 2) {
        for (int sz2 = -1; sz2 <= 1; sz2 += 2) {
            b.addBox(boxAt(0.62f * float(sx2), 1.72f, 1.38f * float(sz2), 0.07f, 0.90f, 0.07f),
                     kWoodColor);                                                  // 蓬柱
        }
    }
    b.addBox(boxAt(0.0f, 2.22f, 0.0f, 1.75f, 0.07f, 3.30f), kAzaleaColor);         // 金达莱蓬顶
    b.build(m_cartBodyMesh);
    b.clear();

    // ---- 车轮（局部原点=轮心，轴向 X；轮毂盖与气门嘴使滚动可见）----
    b.addCylinder(rotateZDeg(90.0f) * translateTo(0.0f, -0.11f, 0.0f), 0.35f, 0.35f, 0.22f, 14,
                  kTileColor);                                                     // 轮胎
    b.addCylinder(rotateZDeg(90.0f) * translateTo(0.0f, -0.15f, 0.0f), 0.14f, 0.14f, 0.30f, 8,
                  kStoneColor);                                                    // 轮毂盖
    b.addBox(boxAt(0.16f, 0.18f, 0.0f, 0.02f, 0.24f, 0.06f), kStoneColor);         // 气门嘴标记
    b.addBox(boxAt(-0.16f, -0.18f, 0.0f, 0.02f, 0.24f, 0.06f), kStoneColor);
    b.build(m_cartWheelMesh);
    b.clear();
}

// ---------------------------------------------------------------------------
// update：推进全部动画状态（CPU 侧，无 GL 调用）
// ---------------------------------------------------------------------------
void HierarchyObjects::update(float dt, const glm::vec3& sunDir, const BSplinePath* tourPath) {
    m_swingTime += dt;                                  // 场景时钟：秋千/跳板/机舱偏航共用
    m_rotorAngle += kRotorSpeed * dt;
    m_nacelleYaw = kYawAmplitude * std::sin(kYawOmega * m_swingTime);

    // 光伏板俯仰角实时跟随太阳仰角（u_SunDir 为由表面指向太阳的方向）
    const float sunLen = glm::length(sunDir);
    if (sunLen > 1e-5f) {
        const float elevation = glm::degrees(std::asin(std::clamp(sunDir.y / sunLen, 0.0f, 1.0f)));
        m_solarPitch = std::clamp(elevation, kSolarPitchMin, kSolarPitchMax);
    }

    // 观光车沿 B 样条轨道匀速巡游（取路径 x/z 投影贴地行驶，朝向切线方向）
    if (tourPath != nullptr && tourPath->totalLength() > 1e-3f) {
        m_cartDist = std::fmod(m_cartDist + kCartSpeed * dt, tourPath->totalLength());
        glm::vec3 pathPos(0.0f);
        glm::vec3 pathTang(0.0f, 0.0f, 1.0f);
        tourPath->evalByDistance(m_cartDist, pathPos, pathTang);
        m_cartPos = glm::vec3(pathPos.x, 0.0f, pathPos.z);
        const glm::vec2 flatTang(pathTang.x, pathTang.z);
        if (glm::length(flatTang) > 1e-4f) {
            m_cartDir = glm::vec3(glm::normalize(flatTang), 0.0f);
        }
    }
    m_wheelRoll += (kCartSpeed / kWheelRadius) * dt;    // 车轮滚动角 = 里程 / 半径
}

// ---------------------------------------------------------------------------
// drawAll：矩阵栈层级绘制（主 Pass 与平面阴影 Pass 使用同一套部件矩阵）
// ---------------------------------------------------------------------------
void HierarchyObjects::drawAll(const Shader& shader, bool shadowPassOnly) const {
    (void)shadowPassOnly; // 阴影 Pass 复用相同几何与矩阵，着色器内部处理投影
    MatrixStack stack;

    const float swingAngle  = kSwingAmplitude * std::sin(kSwingOmega * m_swingTime);
    const float seesawAngle = kSeesawAmplitude * std::sin(kSeesawOmega * m_swingTime);

    auto drawPart = [&shader, &stack](const Mesh& mesh) {
        shader.setMat4("u_Model", stack.top());
        shader.setMat3("u_NormalMat", MathUtil::normalMatrix(stack.top()));
        mesh.draw();
    };

    // ---- 1. 三座风力发电机：塔架 -> 机舱(偏航) -> 轮毂(旋转) -> 3 片扇叶 ----
    static const glm::vec3 kTowerPos[3] = {
        glm::vec3(-58.0f, 0.0f, -42.0f),
        glm::vec3(0.0f, 0.0f, -68.0f),
        glm::vec3(60.0f, 0.0f, -40.0f)
    };
    static const float kTowerYaw[3] = {24.0f, -38.0f, 52.0f}; // 各机初始朝向互异
    for (int i = 0; i < 3; ++i) {
        stack.push(translateTo(kTowerPos[i].x, kTowerPos[i].y, kTowerPos[i].z));
        drawPart(m_towerMesh);                                             // 塔架
        stack.push();
        stack.translate(0.0f, 18.25f, 0.0f);                               // 塔顶安装面
        stack.rotate(m_nacelleYaw + kTowerYaw[i], glm::vec3(0.0f, 1.0f, 0.0f));
        drawPart(m_nacelleMesh);                                           // 机舱
        stack.push();
        stack.translate(0.0f, 0.0f, 1.30f);                                // 机头伸出端
        stack.mul(glm::rotate(glm::mat4(1.0f), m_rotorAngle, glm::vec3(0.0f, 0.0f, 1.0f)));
        drawPart(m_hubMesh);                                               // 轮毂
        for (int k = 0; k < 3; ++k) {
            stack.push();
            stack.rotate(120.0f * float(k), glm::vec3(0.0f, 0.0f, 1.0f));  // 间隔 120°
            drawPart(m_bladeMesh);                                         // 扇叶
            stack.pop();
        }
        stack.pop(); // 轮毂
        stack.pop(); // 机舱
        stack.pop(); // 塔架
    }

    // ---- 2. 民俗高秋千：横梁支点 -> 悬绳+踏板+裙装小人（绕 X 轴简谐摆动）----
    stack.push(translateTo(16.0f, 0.0f, 5.0f));
    drawPart(m_swingFrameMesh);
    stack.push();
    stack.translate(0.0f, 5.07f, 0.0f);                                    // 支点位于横梁轴线
    stack.rotate(swingAngle, glm::vec3(1.0f, 0.0f, 0.0f));
    drawPart(m_swingSeatMesh);
    stack.pop();
    stack.pop();

    // ---- 3. 民俗跳板：中心支点 -> 长木板绕 Z 轴翘动，两端小人随板交替弹起 ----
    stack.push(translateTo(21.0f, 0.0f, 9.0f));
    drawPart(m_seesawBaseMesh);
    stack.push();
    stack.translate(0.0f, 0.47f, 0.0f);                                    // 铰心位于木墩顶
    stack.rotate(seesawAngle, glm::vec3(0.0f, 0.0f, 1.0f));
    drawPart(m_seesawPlankMesh);
    stack.pop();
    stack.pop();

    // ---- 4. 六组追日太阳能光伏阵列：立柱 -> 面板绕 X 轴朝向太阳仰角 ----
    static const glm::vec2 kSolarPos[6] = {
        glm::vec2(40.0f, 19.0f), glm::vec2(53.0f, 19.0f), glm::vec2(66.0f, 19.0f),
        glm::vec2(40.0f, 25.0f), glm::vec2(53.0f, 25.0f), glm::vec2(66.0f, 25.0f)
    };
    for (const glm::vec2& p : kSolarPos) {
        stack.push(translateTo(p.x, 0.0f, p.y));
        drawPart(m_solarPoleMesh);
        stack.push();
        stack.translate(0.0f, 1.42f, 0.0f);                                // 铰支座顶
        stack.rotate(-m_solarPitch, glm::vec3(1.0f, 0.0f, 0.0f));          // 朝场区南侧(-Z)追日
        drawPart(m_solarPanelMesh);
        stack.pop();
        stack.pop();
    }

    // ---- 5. 观光游览车：车身沿轨道朝向切线 -> 4 车轮绕局部 X 轴滚动 ----
    stack.push(glm::translate(glm::mat4(1.0f), m_cartPos));
    stack.mul(rotateYDeg(std::atan2(m_cartDir.x, m_cartDir.z)));           // 车头 +Z 对准切向
    drawPart(m_cartBodyMesh);
    static const glm::vec3 kWheelOffset[4] = {
        glm::vec3(-0.82f, 0.35f, 1.05f), glm::vec3(0.82f, 0.35f, 1.05f),
        glm::vec3(-0.82f, 0.35f, -1.05f), glm::vec3(0.82f, 0.35f, -1.05f)
    };
    for (const glm::vec3& w : kWheelOffset) {
        stack.push();
        stack.translate(w.x, w.y, w.z);
        stack.mul(glm::rotate(glm::mat4(1.0f), m_wheelRoll, glm::vec3(1.0f, 0.0f, 0.0f)));
        drawPart(m_cartWheelMesh);
        stack.pop();
    }
    stack.pop();
}
