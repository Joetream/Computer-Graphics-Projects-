#include "scene/Terrain.h"
#include "math/MathUtil.h"

#include <cmath>
#include <cstdint>
#include <vector>

namespace {

constexpr float PI = 3.14159265358979323846f;

float clamp01(float v) {
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}

float smoothstep(float edge0, float edge1, float x) {
    if (edge1 <= edge0) return x < edge0 ? 0.0f : 1.0f;
    const float t = clamp01((x - edge0) / (edge1 - edge0));
    return t * t * (3.0f - 2.0f * t);
}

// 稳定的二维哈希（正弦扰动取小数部分），返回 [0, 1)
float hash2(float x, float z) {
    const float h = std::sin(x * 127.1f + z * 311.7f) * 43758.5453123f;
    return h - std::floor(h);
}

// 双线性插值的 value noise（哈希噪声）
float valueNoise(float x, float z) {
    const float ix = std::floor(x);
    const float iz = std::floor(z);
    const float fx = x - ix;
    const float fz = z - iz;
    const float u = fx * fx * (3.0f - 2.0f * fx);
    const float v = fz * fz * (3.0f - 2.0f * fz);

    const float n00 = hash2(ix, iz);
    const float n10 = hash2(ix + 1.0f, iz);
    const float n01 = hash2(ix, iz + 1.0f);
    const float n11 = hash2(ix + 1.0f, iz + 1.0f);
    const float nx0 = n00 + (n10 - n00) * u;
    const float nx1 = n01 + (n11 - n01) * u;
    return nx0 + (nx1 - nx0) * v;
}

// 多层正弦 / 哈希分形噪声（FBM），5 个倍频，结果归一化到 [0, 1]
float fbm(float x, float z) {
    float sum = 0.0f;
    float amp = 0.5f;
    float norm = 0.0f;
    for (int octave = 0; octave < 5; ++octave) {
        const float ridges = 0.5f + 0.5f * std::sin(x * 1.37f + z * 0.91f + octave * 0.7f);
        const float layer = 0.55f * valueNoise(x, z) + 0.45f * ridges;
        sum += amp * layer;
        norm += amp;
        x = x * 2.03f + 11.7f;
        z = z * 2.03f - 7.3f;
        amp *= 0.5f;
    }
    return sum / norm;
}

// 布尔哈通河湾弧段的平滑掩膜：径向窗口 [23.5, 31.5] × 村口前方角向窗口
float riverMask(float r, float z, float theta) {
    const float radial = smoothstep(23.5f, 25.2f, r) *
                         (1.0f - smoothstep(29.8f, 31.5f, r));
    const float alongBank = smoothstep(4.0f, 8.5f, z);
    // z > 0 时 theta 位于 (0, PI)；两端用柔和角向淡入淡出收边
    float angular = 0.0f;
    if (theta > 0.0f && theta < PI) {
        angular = smoothstep(0.04f, 0.22f, theta) *
                  (1.0f - smoothstep(PI - 0.22f, PI - 0.04f, theta));
    }
    return radial * alongBank * angular;
}

} // namespace

float Terrain::getHeight(float x, float z) const {
    const float r = std::sqrt(x * x + z * z);
    const float theta = std::atan2(z, x);

    // ---------------------------------------------------------------------
    // 1) 布尔哈通河湾：在 r ∈ [23.5, 31.5]、z > 4.0 的弧段平滑下凹至 -0.85 m
    //    水面位于 y = -0.15 m，因此水深约 0.7 m。
    // ---------------------------------------------------------------------
    float height = -0.85f * riverMask(r, z, theta);

    // ---------------------------------------------------------------------
    // 2) 村落核心区 r < 36：除河床外严格保持 y = 0.0 m。
    //    房屋、门楼与 y = 0.015 的平面投影阴影因此完美贴地、不会穿模。
    // ---------------------------------------------------------------------
    if (r < 36.0f) {
        return height;
    }

    // ---------------------------------------------------------------------
    // 3) 中景振兴产业带：低振幅（<= 1.2 m）田野微起伏，从 r = 36 起平滑渐入，
    //    保证与核心区零高度在接缝处 C0 连续。
    // ---------------------------------------------------------------------
    const float midWeight = smoothstep(36.0f, 48.0f, r);
    const float rolling = 0.32f * std::sin(x * 0.085f + z * 0.041f)
                        + 0.22f * std::sin(z * 0.110f - x * 0.030f)
                        + 0.35f * (valueNoise(x * 0.035f, z * 0.035f) - 0.5f);
    height += midWeight * rolling;

    // ---------------------------------------------------------------------
    // 4) 远景长白山生态圈 r > 82：FBM 分形群山，环状隆起最高约 28 m。
    // ---------------------------------------------------------------------
    height += smoothstep(82.0f, 155.0f, r) * 28.0f * fbm(x * 0.013f, z * 0.013f);

    return height;
}

void Terrain::init() {
    constexpr int   kSegments = 180;
    constexpr float kExtent   = 200.0f;
    constexpr float kStep     = (2.0f * kExtent) / static_cast<float>(kSegments);
    constexpr int   kStride   = kSegments + 1;

    std::vector<Vertex> verts;
    verts.reserve(static_cast<size_t>(kStride) * kStride);
    std::vector<uint32_t> indices;
    indices.reserve(static_cast<size_t>(kSegments) * kSegments * 6);

    // 线性空间调色板（sRGB -> linear，pow(c, 2.2)）
    const glm::vec3 courtyard = MathUtil::srgbToLinear(0xC9B18A); // 庭院暖沙土
    const glm::vec3 pebble    = MathUtil::srgbToLinear(0x9A958C); // 河岸鹅卵石灰
    const glm::vec3 meadow    = MathUtil::srgbToLinear(0x8B9A55); // 中景秋草绿
    const glm::vec3 mountain  = MathUtil::srgbToLinear(0x6E7F8F); // 远山冷黛
    const glm::vec3 snow      = MathUtil::srgbToLinear(0xF2F6FA); // 山巅积雪白

    // 中心差分步长取半个网格间距，得到平滑法线
    const float eps = 0.5f * kStep;

    for (int iz = 0; iz <= kSegments; ++iz) {
        const float z = -kExtent + static_cast<float>(iz) * kStep;
        for (int ix = 0; ix <= kSegments; ++ix) {
            const float x = -kExtent + static_cast<float>(ix) * kStep;
            const float h = getHeight(x, z);

            // 中心差分：N = normalize(-dh/dx, 1, -dh/dz)
            const float hL = getHeight(x - eps, z);
            const float hR = getHeight(x + eps, z);
            const float hD = getHeight(x, z - eps);
            const float hU = getHeight(x, z + eps);
            const glm::vec3 normal = glm::normalize(glm::vec3(hL - hR, 2.0f * eps, hD - hU));

            const float r = std::sqrt(x * x + z * z);
            const float theta = std::atan2(z, x);

            // ---- 区域顶点色：庭院 -> 河岸 -> 秋草 -> 冷黛远山 -> 雪顶 ----
            glm::vec3 color = courtyard;

            const float bankMix = smoothstep(22.5f, 25.0f, r) *
                                 (1.0f - smoothstep(31.0f, 33.5f, r)) *
                                 smoothstep(3.5f, 8.0f, z);
            color = glm::mix(color, pebble, bankMix * 0.92f);

            const float meadowMix = smoothstep(34.0f, 46.0f, r) *
                                    (1.0f - smoothstep(76.0f, 90.0f, r));
            color = glm::mix(color, meadow, meadowMix);

            const float mountainMix = smoothstep(80.0f, 102.0f, r);
            color = glm::mix(color, mountain, mountainMix);

            const float snowMix = smoothstep(14.0f, 25.0f, h) * mountainMix;
            color = glm::mix(color, snow, snowMix);

            // 低频斑驳，避免大片纯色；河床额外压暗一点以显出水的通透
            const float speck = 0.94f + 0.12f * valueNoise(x * 0.22f, z * 0.22f);
            color *= speck * (1.0f - 0.18f * riverMask(r, z, theta));

            Vertex v;
            v.pos = glm::vec3(x, h, z);
            v.normal = normal;
            v.uv = glm::vec2((x + kExtent) / (2.0f * kExtent),
                             (z + kExtent) / (2.0f * kExtent));
            v.color = color;
            verts.push_back(v);
        }
    }

    // 与 MeshBuilder::addGrid 一致的绕序，使三角形正面法线朝 +Y
    for (int iz = 0; iz < kSegments; ++iz) {
        for (int ix = 0; ix < kSegments; ++ix) {
            const uint32_t i0 = static_cast<uint32_t>(iz * kStride + ix);
            const uint32_t i1 = i0 + 1;
            const uint32_t i2 = i0 + kStride;
            const uint32_t i3 = i2 + 1;
            indices.insert(indices.end(), {i0, i2, i1, i1, i2, i3});
        }
    }

    m_mesh.upload(verts, indices);
}

void Terrain::draw(const Shader& shader) const {
    // 顶点已烘焙在世界坐标中，模型矩阵保持单位阵；水面/稻田/雾等开关由 App 管理。
    shader.use();
    shader.setMat4("u_Model", glm::mat4(1.0f));
    shader.setMat3("u_NormalMat", glm::mat3(1.0f));
    m_mesh.draw();
}
