#include "fx/ParticleSystem.h"

#include <glad/glad.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numeric>
#include <random>
#include <vector>

namespace {

constexpr int kParticleCount = 360;
constexpr float kPi          = 3.14159265358979323846f;

// 广告牌 Quad：[-0.5, 0.5]^2，两个 CCW 三角形（location 0）
const float kQuadCorners[12] = {
    -0.5f, -0.5f,   0.5f, -0.5f,   0.5f,  0.5f,
    -0.5f, -0.5f,   0.5f,  0.5f,  -0.5f,  0.5f
};

std::mt19937& rng() {
    static std::mt19937 generator(20261110u);
    return generator;
}

float randRange(float lo, float hi) {
    return std::uniform_real_distribution<float>(lo, hi)(rng());
}

float smoothstep(float edge0, float edge1, float x) {
    const float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

// 全局风场相位（无头文件中额外成员可用，粒子系统为单实例渲染资源）
float g_windTime = 0.0f;

} // namespace

ParticleSystem::~ParticleSystem() {
    if (m_instVbo) glDeleteBuffers(1, &m_instVbo);
    if (m_quadVbo) glDeleteBuffers(1, &m_quadVbo);
    if (m_vao)     glDeleteVertexArrays(1, &m_vao);
    m_instVbo = 0;
    m_quadVbo = 0;
    m_vao     = 0;
}

void ParticleSystem::init(const std::vector<glm::vec3>& chimneyTops) {
    m_emitters = chimneyTops;
    if (m_emitters.empty()) {
        // 兜底发射器：即便场景未提供烟囱坐标，炊烟演示也不会变为空画面
        m_emitters.push_back(glm::vec3(0.0f, 3.8f, 0.0f));
    }

    m_particles.resize(kParticleCount);
    m_gpuBuffer.reserve(kParticleCount);
    const glm::vec3 defaultCamPos(0.0f, 4.2f, 42.0f); // App 默认机位
    for (int i = 0; i < kParticleCount; ++i) {
        respawnParticle(m_particles[i], i, defaultCamPos);
    }

    // ---------------------------------------------------------------------
    // VAO：location 0 = QuadCorner(divisor 0)；
    //      location 1..3 = CenterPos / Size / Color（全部 divisor = 1）
    //      实例数据按 GPUInstance 交错存放，偏移量用 offsetof 保证与内存布局一致。
    // ---------------------------------------------------------------------
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_quadVbo);
    glGenBuffers(1, &m_instVbo);

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_quadVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kQuadCorners), kQuadCorners, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);

    glBindBuffer(GL_ARRAY_BUFFER, m_instVbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(kParticleCount * sizeof(GPUInstance)),
                 nullptr, GL_DYNAMIC_DRAW);

    const GLsizei stride = static_cast<GLsizei>(sizeof(GPUInstance));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void*>(offsetof(GPUInstance, pos)));
    glVertexAttribDivisor(1, 1);

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void*>(offsetof(GPUInstance, size)));
    glVertexAttribDivisor(2, 1);

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void*>(offsetof(GPUInstance, color)));
    glVertexAttribDivisor(3, 1);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ParticleSystem::setMode(int mode) {
    const int next = (mode == 1) ? 1 : 0;
    if (next == m_mode) return;
    m_mode = next;
    // 让所有粒子在下一帧 update() 中携带真实相机位置重新出生，
    // 避免季节切换瞬间出现旧模式的粒子残影。
    for (Particle& p : m_particles) {
        p.life = 0.0f;
        p.alpha = 0.0f;
    }
}

void ParticleSystem::respawnParticle(Particle& p, int idx, const glm::vec3& camPos) {
    if (m_mode == 1) {
        // 初冬瑞雪：相机周围 [-45, 45] x [0, 28] x [-45, 45] 空间内飘落
        p.pos = glm::vec3(camPos.x + randRange(-45.0f, 45.0f),
                          randRange(0.0f, 28.0f),
                          camPos.z + randRange(-45.0f, 45.0f));
        p.vel = glm::vec3(randRange(-0.35f, 0.35f),
                          -randRange(1.30f, 2.60f),
                          randRange(-0.30f, 0.30f));
        p.maxLife = randRange(9.0f, 15.0f);
        p.life    = p.maxLife * randRange(0.15f, 1.0f);
        p.size    = randRange(0.07f, 0.16f);
        p.alpha   = 0.0f;
        return;
    }

    // 金秋炊烟：各民居独立火炕烟囱顶端循环发射
    const glm::vec3 emitter = m_emitters.empty()
        ? glm::vec3(0.0f, 3.8f, 0.0f)
        : m_emitters[static_cast<size_t>(idx) % m_emitters.size()];
    p.pos = emitter + glm::vec3(randRange(-0.16f, 0.16f),
                                randRange(0.0f, 0.35f),
                                randRange(-0.16f, 0.16f));
    p.vel = glm::vec3(randRange(0.02f, 0.12f),
                      randRange(0.32f, 0.58f),
                      randRange(-0.04f, 0.04f));
    p.maxLife = randRange(4.5f, 7.5f);
    p.life    = p.maxLife * randRange(0.10f, 1.0f);
    // 错开初始高度，首帧即形成连续的烟柱而非一团结块
    p.pos.y += (p.maxLife - p.life) * p.vel.y * 0.55f;
    p.size    = randRange(0.20f, 0.34f);
    p.alpha   = 0.0f;
}

void ParticleSystem::update(float dt, const glm::vec3& camPos) {
    if (dt <= 0.0f) return;
    if (dt > 0.10f) dt = 0.10f; // 切回窗口时避免大步长使粒子瞬移
    g_windTime += dt;

    for (int i = 0; i < static_cast<int>(m_particles.size()); ++i) {
        Particle& p = m_particles[i];
        p.life -= dt;
        if (p.life <= 0.0f) {
            respawnParticle(p, i, camPos);
            p.alpha = 0.0f;
            continue;
        }

        p.pos += p.vel * dt;

        if (m_mode == 0) {
            // 正弦微风漂移 + 随生命周期膨胀
            const float phase = g_windTime * 0.85f + static_cast<float>(i) * 0.37f;
            p.pos.x += std::sin(phase + p.pos.y * 0.35f) * 0.22f * dt;
            p.pos.z += std::cos(phase * 0.73f + p.pos.y * 0.28f) * 0.14f * dt;
            p.size  += 0.115f * dt;
        } else {
            p.pos.x += std::sin(g_windTime * 0.55f + p.pos.y * 0.22f) * 0.35f * dt;
            // 落地或飘出相机周围盒子时在相机附近循环重生
            if (p.pos.y < 0.0f ||
                std::fabs(p.pos.x - camPos.x) > 50.0f ||
                std::fabs(p.pos.z - camPos.z) > 50.0f) {
                respawnParticle(p, i, camPos);
                p.alpha = 0.0f;
                continue;
            }
        }

        // 生命周期淡入 / 淡出
        const float age = 1.0f - p.life / p.maxLife; // 0: 出生, 1: 消亡
        p.alpha = smoothstep(0.0f, 0.12f, age) * (1.0f - smoothstep(0.55f, 1.0f, age));
    }
}

void ParticleSystem::draw(const Shader& particleShader, const glm::mat4& view,
                          const glm::mat4& proj, const glm::vec3& camPos) const {
    if (!m_vao || m_particles.empty()) return;

    // ---------------------------------------------------------------------
    // 1) 半透明混合要求由远及近排序：按到相机距离的平方降序
    // ---------------------------------------------------------------------
    std::vector<size_t> order(m_particles.size());
    std::iota(order.begin(), order.end(), size_t(0));
    std::stable_sort(order.begin(), order.end(), [&](size_t a, size_t b) {
        const glm::vec3 da = m_particles[a].pos - camPos;
        const glm::vec3 db = m_particles[b].pos - camPos;
        return glm::dot(da, da) > glm::dot(db, db);
    });

    // ---------------------------------------------------------------------
    // 2) 打包 GPUInstance（位置 / 尺寸 / 颜色含 Alpha）
    // ---------------------------------------------------------------------
    m_gpuBuffer.clear();
    m_gpuBuffer.reserve(m_particles.size());
    for (const size_t idx : order) {
        const Particle& p = m_particles[idx];
        if (p.alpha <= 0.001f) continue;
        GPUInstance inst;
        inst.pos  = p.pos;
        inst.size = p.size;
        inst.color = (m_mode == 0)
            ? glm::vec4(0.84f, 0.81f, 0.76f, p.alpha * 0.52f)  // 暖灰白炊烟
            : glm::vec4(0.96f, 0.98f, 1.00f, p.alpha * 0.88f); // 晶莹雪白
        m_gpuBuffer.push_back(inst);
    }
    if (m_gpuBuffer.empty()) return;

    // ---------------------------------------------------------------------
    // 3) 上传实例数据
    // ---------------------------------------------------------------------
    glBindBuffer(GL_ARRAY_BUFFER, m_instVbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    static_cast<GLsizeiptr>(m_gpuBuffer.size() * sizeof(GPUInstance)),
                    m_gpuBuffer.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // ---------------------------------------------------------------------
    // 4) 广告牌朝向：view 矩阵的前两行即相机的世界空间 right / up 轴
    // ---------------------------------------------------------------------
    const glm::vec3 camRight = glm::normalize(glm::vec3(view[0][0], view[1][0], view[2][0]));
    const glm::vec3 camUp    = glm::normalize(glm::vec3(view[0][1], view[1][1], view[2][1]));

    particleShader.use();
    particleShader.setMat4("u_View", view);
    particleShader.setMat4("u_Proj", proj);
    particleShader.setVec3("u_CamRight", camRight);
    particleShader.setVec3("u_CamUp", camUp);

    glBindVertexArray(m_vao);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, static_cast<GLsizei>(m_gpuBuffer.size()));
    glBindVertexArray(0);
}
