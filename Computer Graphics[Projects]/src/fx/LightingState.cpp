#include "fx/LightingState.h"
#include "core/MeshBuilder.h"
#include "math/MathUtil.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <string>
#include <unordered_map>

namespace {

constexpr float kTransitionSeconds = 1.2f;
constexpr float kLampHeight        = 3.30f;

float clamp01(float v) {
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}

float smoothstep01(float t) {
    const float x = clamp01(t);
    return x * x * (3.0f - 2.0f * x);
}

// 过渡中途再次切换预设时，需要以「当前画面实际光照」作为新起点，
// 否则连按 1/2/3 会因起点回退到旧预设而出现跳变。
// Preset 是 LightingState 的私有嵌套类型，这里用同布局的快照结构体保存。
struct LightSnapshot {
    glm::vec3 sunDir;
    glm::vec3 sunColor;
    glm::vec3 ambient;
    glm::vec3 fogColor;
    float     fogDensity;
    float     pointLightFactor;
};

std::unordered_map<const LightingState*, LightSnapshot>& fromSnapshots() {
    static std::unordered_map<const LightingState*, LightSnapshot> snapshots;
    return snapshots;
}

LightSnapshot snapshotOf(const glm::vec3& sunDir, const glm::vec3& sunColor,
                         const glm::vec3& ambient, const glm::vec3& fogColor,
                         float fogDensity, float pointLightFactor) {
    return LightSnapshot{sunDir, sunColor, ambient, fogColor, fogDensity, pointLightFactor};
}

} // namespace

LightingState::LightingState() {
    // ------------------------------------------------------------------
    // 0 晨曦初露：低角度暖金阳光 + 暖粉橙晨雾，路灯熄灭
    // ------------------------------------------------------------------
    m_presets[0].sunDir           = glm::normalize(glm::vec3(0.75f, 0.26f, -0.45f));
    m_presets[0].sunColor         = glm::vec3(1.00f, 0.68f, 0.38f);
    m_presets[0].ambient          = glm::vec3(0.26f, 0.22f, 0.25f);
    m_presets[0].fogColor         = glm::vec3(0.78f, 0.52f, 0.42f);
    m_presets[0].fogDensity       = 0.0055f;
    m_presets[0].pointLightFactor = 0.0f;

    // ------------------------------------------------------------------
    // 1 正午晴空：明亮白光 + 淡蓝薄雾，路灯熄灭（启用平面投影阴影）
    // ------------------------------------------------------------------
    m_presets[1].sunDir           = glm::normalize(glm::vec3(0.25f, 0.92f, -0.28f));
    m_presets[1].sunColor         = glm::vec3(1.00f, 0.97f, 0.90f);
    m_presets[1].ambient          = glm::vec3(0.40f, 0.45f, 0.52f);
    m_presets[1].fogColor         = glm::vec3(0.62f, 0.76f, 0.88f);
    m_presets[1].fogDensity       = 0.0030f;
    m_presets[1].pointLightFactor = 0.0f;

    // ------------------------------------------------------------------
    // 2 星夜灯火：微弱深蓝月光 + 深蓝夜雾，6 盏路灯全亮
    // ------------------------------------------------------------------
    m_presets[2].sunDir           = glm::normalize(glm::vec3(-0.35f, 0.65f, -0.55f));
    m_presets[2].sunColor         = glm::vec3(0.16f, 0.22f, 0.42f);
    m_presets[2].ambient          = glm::vec3(0.07f, 0.09f, 0.16f);
    m_presets[2].fogColor         = glm::vec3(0.045f, 0.075f, 0.16f);
    m_presets[2].fogDensity       = 0.0080f;
    m_presets[2].pointLightFactor = 1.0f;

    // 初始状态与头文件默认成员保持一致（Dawn）
    m_prevPreset      = 0;
    m_targetPreset    = 0;
    m_transition      = 1.0f;
    m_currSunDir      = m_presets[0].sunDir;
    m_currSunColor    = m_presets[0].sunColor;
    m_currAmbient     = m_presets[0].ambient;
    m_currFogColor    = m_presets[0].fogColor;
    m_currFogDensity  = m_presets[0].fogDensity;
    m_currPointFactor = m_presets[0].pointLightFactor;
    m_skyA            = 0;
    m_skyB            = 0;
    m_skyBlend        = 0.0f;
    m_seasonTarget    = 0;
    m_snowAmount      = 0.0f;

    // ------------------------------------------------------------------
    // 6 盏村落路灯：迎宾门楼 (0, 0, 32) 两侧 2 盏 +
    // 核心庭院 2 盏 + 布尔哈通河湾步道 2 盏，灯头高度统一 3.30 m
    // ------------------------------------------------------------------
    const glm::vec2 lampXZ[6] = {
        {-5.4f, 31.0f}, { 5.4f, 31.0f},   // 门楼两侧
        {-6.5f,  1.0f}, { 7.5f,  3.0f},   // 核心庭院
        {-15.0f, 19.0f}, {15.0f, 20.0f}   // 河湾步道
    };
    for (int i = 0; i < 6; ++i) {
        m_lampPos[i] = glm::vec3(lampXZ[i].x, kLampHeight, lampXZ[i].y);
        const float strength = (i < 2) ? 1.10f : 0.92f; // 门楼两盏略亮
        m_lampColor[i] = glm::vec3(1.00f, 0.72f, 0.36f) * strength;
    }
}

void LightingState::setPreset(int index) {
    if (index < 0 || index > 2 || index == m_targetPreset) return;

    // 固化当前插值结果，作为本次过渡的真实起点
    fromSnapshots()[this] = snapshotOf(m_currSunDir, m_currSunColor, m_currAmbient,
                                       m_currFogColor, m_currFogDensity,
                                       m_currPointFactor);

    m_prevPreset   = (m_skyBlend >= 0.5f) ? m_skyB : m_skyA;
    m_targetPreset = index;
    m_transition   = 0.0f;
    m_skyA         = m_prevPreset; // 天空盒 A = 过渡开始时的当前天空
    m_skyB         = index;
    m_skyBlend     = 0.0f;
}

void LightingState::toggleSeason() {
    m_seasonTarget = (m_seasonTarget == 0) ? 1 : 0;
}

void LightingState::update(float dt) {
    if (dt < 0.0f) dt = 0.0f;

    // ------------------------------------------------------------------
    // 三态光影：1.2 秒 smoothstep 平滑插值（方向向量插值后重新归一化）
    // ------------------------------------------------------------------
    if (m_transition < 1.0f) {
        m_transition = std::min(1.0f, m_transition + dt / kTransitionSeconds);
        const float t = smoothstep01(m_transition);

        LightSnapshot from;
        auto it = fromSnapshots().find(this);
        if (it != fromSnapshots().end()) {
            from = it->second;
        } else {
            const Preset& p = m_presets[m_prevPreset];
            from = snapshotOf(p.sunDir, p.sunColor, p.ambient, p.fogColor,
                              p.fogDensity, p.pointLightFactor);
        }
        const Preset& to = m_presets[m_targetPreset];

        m_currSunDir      = glm::normalize(glm::mix(from.sunDir, to.sunDir, t));
        m_currSunColor    = glm::mix(from.sunColor, to.sunColor, t);
        m_currAmbient     = glm::mix(from.ambient, to.ambient, t);
        m_currFogColor    = glm::mix(from.fogColor, to.fogColor, t);
        m_currFogDensity  = from.fogDensity + (to.fogDensity - from.fogDensity) * t;
        m_currPointFactor = from.pointLightFactor + (to.pointLightFactor - from.pointLightFactor) * t;
        m_skyBlend        = t;

        if (m_transition >= 1.0f) {
            // 过渡结束：两张天空盒都指向目标，水面反射同样稳定
            m_skyA     = m_targetPreset;
            m_skyB     = m_targetPreset;
            m_skyBlend = 0.0f;
            fromSnapshots().erase(this);
        }
    }

    // ------------------------------------------------------------------
    // 季节：金秋 (0) <-> 初冬瑞雪 (1)，snowAmount 指数平滑至 [0, 1]
    // ------------------------------------------------------------------
    const float snowTarget = (m_seasonTarget == 1) ? 1.0f : 0.0f;
    const float k = 1.0f - std::exp(-dt * 1.8f);
    m_snowAmount += (snowTarget - m_snowAmount) * k;
    if (std::fabs(snowTarget - m_snowAmount) < 0.001f) {
        m_snowAmount = snowTarget;
    }
}

void LightingState::applyToShader(const Shader& shader) const {
    shader.setVec3("u_SunDir", m_currSunDir);
    shader.setVec3("u_SunColor", m_currSunColor);
    shader.setVec3("u_Ambient", m_currAmbient);
    shader.setVec3("u_FogColor", m_currFogColor);
    shader.setFloat("u_FogDensity", m_currFogDensity);
    shader.setFloat("u_PointLightFactor", m_currPointFactor);
    shader.setFloat("u_SnowAmount", m_snowAmount);
    shader.setInt("u_NumPointLights", 6);

    // GLSL uniform 数组元素必须用 "name[i]" 逐个查询 location
    for (int i = 0; i < 6; ++i) {
        const std::string idx = std::to_string(i);
        shader.setVec3("u_PointLightPos[" + idx + "]", m_lampPos[i]);
        shader.setVec3("u_PointLightColor[" + idx + "]", m_lampColor[i]);
    }
}

void LightingState::drawLampPosts(const Shader& shader) const {
    // const 方法中惰性构建一次路灯柱网格（mutable 成员）
    if (!m_lampMeshBuilt) {
        MeshBuilder builder;
        const glm::vec3 wood  = MathUtil::srgbToLinear(0x5B3A29); // 复古深褐木柱
        const glm::vec3 stone = MathUtil::srgbToLinear(0x9A958C); // 石砌基座
        const glm::vec3 cap   = MathUtil::srgbToLinear(0x3E2A20); // 攒尖灯罩
        const glm::vec3 glow  = MathUtil::srgbToLinear(0xFFD27A); // 暖黄灯头

        for (int i = 0; i < 6; ++i) {
            const glm::mat4 at = glm::translate(glm::mat4(1.0f),
                                                glm::vec3(m_lampPos[i].x, 0.0f, m_lampPos[i].z));
            const auto shifted = [&at](float y) {
                return at * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, y, 0.0f));
            };

            // 八角石砌基座 0.00 - 0.30 m
            builder.addCylinder(shifted(0.0f), 0.30f, 0.24f, 0.30f, 8, stone);
            // 木质灯柱 0.30 - 3.05 m
            builder.addCylinder(shifted(0.30f), 0.11f, 0.085f, 2.75f, 8, wood);
            // 柱顶十字木枋托盘
            builder.addBox(shifted(3.05f) *
                           glm::scale(glm::mat4(1.0f), glm::vec3(0.62f, 0.10f, 0.62f)), wood);
            // 暖黄发光灯头（与点光源位置 y = 3.30 对齐）
            builder.addBox(shifted(kLampHeight) *
                           glm::scale(glm::mat4(1.0f), glm::vec3(0.30f, 0.42f, 0.30f)), glow);
            // 四角攒尖灯罩 + 顶尖
            builder.addCylinder(shifted(3.51f), 0.34f, 0.06f, 0.26f, 8, cap);
            builder.addCylinder(shifted(3.77f), 0.06f, 0.0f, 0.14f, 8, cap);
            // 木枋两端的小挂饰，强化复古轮廓
            builder.addBox(shifted(3.05f) *
                           glm::translate(glm::mat4(1.0f), glm::vec3(-0.26f, -0.14f, 0.0f)) *
                           glm::scale(glm::mat4(1.0f), glm::vec3(0.07f, 0.26f, 0.07f)), wood);
            builder.addBox(shifted(3.05f) *
                           glm::translate(glm::mat4(1.0f), glm::vec3(0.26f, -0.14f, 0.0f)) *
                           glm::scale(glm::mat4(1.0f), glm::vec3(0.07f, 0.26f, 0.07f)), wood);
        }

        builder.build(m_lampPostMesh);
        m_lampMeshBuilt = true;
    }

    // 网格顶点已烘焙在世界坐标，使用单位模型矩阵绘制（不影响 App 后续设置）
    shader.use();
    shader.setMat4("u_Model", glm::mat4(1.0f));
    shader.setMat3("u_NormalMat", glm::mat3(1.0f));
    m_lampPostMesh.draw();
}
