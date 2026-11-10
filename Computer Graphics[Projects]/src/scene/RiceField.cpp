#include "scene/RiceField.h"
#include "scene/Terrain.h"
#include "core/MeshBuilder.h"
#include "math/MathUtil.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>
#include <utility>
#include <vector>

namespace {

constexpr float PI = 3.14159265358979323846f;
constexpr float kStalkHeight = 0.95f;

// 三座风力发电机塔基（HierarchyObjects.cpp 中的世界坐标）
const glm::vec2 kTurbineBases[3] = {
    {-58.0f, -42.0f},
    {  0.0f, -68.0f},
    { 60.0f, -40.0f}
};

// 需要避让的区域：村口迎宾大道、光伏板阵列、风机塔基
bool isBlocked(float x, float z) {
    if (z > 25.0f && std::fabs(x) < 10.0f) return true;               // 迎宾大道
    if (x > 35.0f && x < 71.0f && z > 15.0f && z < 29.0f) return true; // 光伏板阵列
    for (const glm::vec2& base : kTurbineBases) {
        const float dx = x - base.x;
        const float dz = z - base.y;
        if (dx * dx + dz * dz < 6.5f * 6.5f) return true;              // 风机底座
    }
    return false;
}

} // namespace

void RiceField::init(const Terrain& terrain) {
    // =====================================================================
    // 1) 单丛低多边形金色稻穗：3 片交叉倾斜的细长叶片，高约 0.95 m
    //    每片叶子由「下段四边形 + 上段穗尖三角形」组成，正反面各生成一次，
    //    保证任意视角下法线都朝向观察者，Blinn-Phong 高光均匀。
    // =====================================================================
    MeshBuilder builder;
    const glm::vec3 gold    = MathUtil::srgbToLinear(0xD9A441); // 金秋稻穗主色
    const glm::vec3 tipGold = MathUtil::srgbToLinear(0xE8C36B); // 稻穗尖端浅金

    for (int blade = 0; blade < 3; ++blade) {
        const float angle = static_cast<float>(blade) * (PI / 3.0f); // 0°, 60°, 120°
        const glm::mat4 M = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f));

        const glm::vec3 p0(-0.035f, 0.0f, 0.0f);  // 根部左
        const glm::vec3 p1( 0.035f, 0.0f, 0.0f);  // 根部右
        const glm::vec3 p2( 0.072f, 0.55f, 0.10f); // 中段右（向外倾斜）
        const glm::vec3 p3(-0.072f, 0.55f, 0.10f); // 中段左
        const glm::vec3 p4( 0.0f, kStalkHeight, 0.21f); // 穗尖

        // 下段两个三角形（正反两面）
        builder.addTriangle(p0, p1, p2, gold, M);
        builder.addTriangle(p1, p0, p2, gold, M);
        builder.addTriangle(p0, p2, p3, gold, M);
        builder.addTriangle(p2, p0, p3, gold, M);
        // 上段穗尖（正反两面）
        builder.addTriangle(p3, p2, p4, tipGold, M);
        builder.addTriangle(p2, p3, p4, tipGold, M);
    }
    builder.build(m_stalkMesh);

    // =====================================================================
    // 2) 中景环带 r ∈ [38, 80] 划分 4 条径向带 × 12 个扇区（共 48 个候选），
    //    剔除迎宾大道 / 风机底座 / 光伏阵列后保留约 40 个 Chunk，
    //    每块 8 × 8 = 64 株带随机微扰旋转 / 缩放的实例矩阵，并统计 AABB。
    // =====================================================================
    m_chunks.clear();
    m_visibleBuffer.clear();
    m_visibleInstanceCount = 0;
    m_visibleChunks = 0;

    std::mt19937 rng(20261110u); // 固定种子：每次运行稻田布局一致，便于课程演示对比
    std::uniform_real_distribution<float> jitter(-0.5f, 0.5f);
    std::uniform_real_distribution<float> yawDist(0.0f, 2.0f * PI);
    std::uniform_real_distribution<float> scaleDist(0.82f, 1.18f);

    constexpr int   kRadialBands    = 4;
    constexpr int   kAngularSectors = 12;
    constexpr float kInnerR         = 38.0f;
    constexpr float kOuterR         = 80.0f;
    const float bandWidth   = (kOuterR - kInnerR) / kRadialBands;
    const float sectorWidth = 2.0f * PI / kAngularSectors;

    for (int ib = 0; ib < kRadialBands; ++ib) {
        const float r0 = kInnerR + ib * bandWidth;
        const float r1 = r0 + bandWidth;

        for (int ia = 0; ia < kAngularSectors; ++ia) {
            const float a0 = ia * sectorWidth;
            const float a1 = a0 + sectorWidth;

            // 整块中心落在禁区（迎宾大道 / 风机底座 / 光伏阵列）则直接跳过
            const float centerAngle = 0.5f * (a0 + a1);
            const float centerR     = 0.5f * (r0 + r1);
            if (isBlocked(centerR * std::cos(centerAngle),
                          centerR * std::sin(centerAngle))) {
                continue;
            }

            Chunk chunk;
            chunk.minPt = glm::vec3( 1.0e9f,  1.0e9f,  1.0e9f);
            chunk.maxPt = glm::vec3(-1.0e9f, -1.0e9f, -1.0e9f);
            chunk.instances.reserve(64);

            for (int iy = 0; iy < 8; ++iy) {
                for (int ix = 0; ix < 8; ++ix) {
                    float fu = (ix + 0.5f + jitter(rng) * 0.7f) / 8.0f;
                    float fv = (iy + 0.5f + jitter(rng) * 0.7f) / 8.0f;
                    fu = std::clamp(fu, 0.02f, 0.98f);
                    fv = std::clamp(fv, 0.02f, 0.98f);

                    float angle  = a0 + fu * sectorWidth;
                    const float radius = r0 + fv * bandWidth;
                    float x = radius * std::cos(angle);
                    float z = radius * std::sin(angle);

                    // 个别实例若落入禁区，则沿角向挪到同一地块内的安全位置
                    for (int attempt = 0; attempt < 4 && isBlocked(x, z); ++attempt) {
                        const float shifted = std::fmod(fu + 0.13f * (attempt + 1), 1.0f);
                        angle = a0 + shifted * sectorWidth;
                        x = radius * std::cos(angle);
                        z = radius * std::sin(angle);
                    }
                    if (isBlocked(x, z)) continue;

                    const float y = terrain.getHeight(x, z); // 贴合地形起伏
                    const float s = scaleDist(rng);
                    const glm::mat4 model =
                        glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z)) *
                        glm::rotate(glm::mat4(1.0f), yawDist(rng), glm::vec3(0.0f, 1.0f, 0.0f)) *
                        glm::scale(glm::mat4(1.0f), glm::vec3(s, s * (0.92f + 0.16f * fv), s));
                    chunk.instances.push_back(model);

                    // AABB 预留水平余量（稻浪摆动）与高度余量（随机缩放）
                    constexpr float kPad = 0.35f;
                    chunk.minPt = glm::min(chunk.minPt,
                                           glm::vec3(x - kPad, y - 0.10f, z - kPad));
                    chunk.maxPt = glm::max(chunk.maxPt,
                                           glm::vec3(x + kPad,
                                                     y + kStalkHeight * 1.25f + 0.30f,
                                                     z + kPad));
                }
            }

            if (!chunk.instances.empty()) {
                m_chunks.push_back(std::move(chunk));
            }
        }
    }

    // 初始化时上传全部实例，保证 instanced VAO（location 4..7, divisor = 1）已建立；
    // 之后每帧由 updateVisibleInstances() 按视锥剔除结果覆盖。
    for (const Chunk& c : m_chunks) {
        m_visibleBuffer.insert(m_visibleBuffer.end(), c.instances.begin(), c.instances.end());
    }
    m_visibleInstanceCount = static_cast<int>(m_visibleBuffer.size());
    m_visibleChunks = static_cast<int>(m_chunks.size());
    m_stalkMesh.setInstances(m_visibleBuffer);
}

void RiceField::updateVisibleInstances(const Frustum& frustum) {
    m_visibleBuffer.clear();
    m_visibleChunks = 0;

    for (const Chunk& c : m_chunks) {
        // 逐地块做 AABB / 视锥平面相交测试，一次剔除整块 64 株稻穗
        if (!frustum.intersectAABB(c.minPt, c.maxPt)) continue;
        m_visibleBuffer.insert(m_visibleBuffer.end(), c.instances.begin(), c.instances.end());
        ++m_visibleChunks;
    }

    m_visibleInstanceCount = static_cast<int>(m_visibleBuffer.size());
    m_stalkMesh.setInstances(m_visibleBuffer);
}

void RiceField::draw(const Shader& shader) const {
    if (m_visibleInstanceCount <= 0) return;
    // App 在调用前已设置 u_Instanced = true 与 u_AnimType = 2（稻浪顶点动画）
    shader.use();
    m_stalkMesh.drawInstanced(m_visibleInstanceCount);
}
