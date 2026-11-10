#include "scene/Water.h"
#include "math/MathUtil.h"

#include <cmath>
#include <cstdint>
#include <vector>

namespace {
constexpr float PI = 3.14159265358979323846f;
} // namespace

void Water::init() {
    // 布尔哈通河湾扇环：90 段角向 × 18 段径向的高密度网格
    constexpr int   kAngularSeg = 90;
    constexpr int   kRadialSeg  = 18;
    constexpr float kInnerR     = 23.8f;
    constexpr float kOuterR     = 31.2f;
    constexpr float kWaterY     = -0.15f;

    // 村口前方弧段：起止角取 0.22 rad 与 PI - 0.22 rad，
    // 保证整片水域 z = r * sin(theta) >= 23.8 * sin(0.22) ≈ 5.2 > 4.5
    constexpr float kThetaStart = 0.22f;
    constexpr float kThetaEnd   = PI - 0.22f;

    const glm::vec3 waterColor = MathUtil::srgbToLinear(0x3A7CA8); // 清澈河水蓝绿
    const int stride = kRadialSeg + 1;

    std::vector<Vertex> verts;
    verts.reserve(static_cast<size_t>(kAngularSeg + 1) * stride);
    std::vector<uint32_t> indices;
    indices.reserve(static_cast<size_t>(kAngularSeg) * kRadialSeg * 6);

    for (int ia = 0; ia <= kAngularSeg; ++ia) {
        const float u = static_cast<float>(ia) / kAngularSeg;
        const float theta = kThetaStart + u * (kThetaEnd - kThetaStart);
        const float ct = std::cos(theta);
        const float st = std::sin(theta);

        for (int ir = 0; ir <= kRadialSeg; ++ir) {
            const float v = static_cast<float>(ir) / kRadialSeg;
            const float radius = kInnerR + v * (kOuterR - kInnerR);

            Vertex vert;
            vert.pos = glm::vec3(radius * ct, kWaterY, radius * st);
            // u_AnimType == 1 时顶点着色器会用解析偏导重算法线，此处提供静态基法线
            vert.normal = glm::vec3(0.0f, 1.0f, 0.0f);
            vert.uv = glm::vec2(u, v);
            vert.color = waterColor;
            verts.push_back(vert);
        }
    }

    // 参数顺序 (theta, radius)：cross(dP/dtheta, dP/dradius) = (0, r, 0) 朝 +Y
    for (int ia = 0; ia < kAngularSeg; ++ia) {
        for (int ir = 0; ir < kRadialSeg; ++ir) {
            const uint32_t A = static_cast<uint32_t>(ia * stride + ir);       // (theta, r)
            const uint32_t B = static_cast<uint32_t>((ia + 1) * stride + ir); // (theta+, r)
            const uint32_t C = A + 1;                                         // (theta, r+)
            const uint32_t D = B + 1;                                         // (theta+, r+)
            indices.insert(indices.end(), {A, B, C, B, D, C});
        }
    }

    m_mesh.upload(verts, indices);
}

void Water::draw(const Shader& shader) const {
    // App 在调用前已设置 u_AnimType = 1（双正弦水波）与 u_UseEnvMap = true（天空盒反射），
    // 此处只负责绑定着色器并提交几何，不覆盖这些渲染状态。
    shader.use();
    shader.setMat4("u_Model", glm::mat4(1.0f));
    shader.setMat3("u_NormalMat", glm::mat3(1.0f));
    m_mesh.draw();
}
