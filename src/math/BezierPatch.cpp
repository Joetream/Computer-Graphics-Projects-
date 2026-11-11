#include "math/BezierPatch.h"

#include "math/MathUtil.h"
#include <algorithm>

namespace {
void bernstein(float t, float basis[4], float derivative[4]) {
    const float oneMinusT = 1.0f - t;
    basis[0] = oneMinusT * oneMinusT * oneMinusT;
    basis[1] = 3.0f * t * oneMinusT * oneMinusT;
    basis[2] = 3.0f * t * t * oneMinusT;
    basis[3] = t * t * t;
    derivative[0] = -3.0f * oneMinusT * oneMinusT;
    derivative[1] = 3.0f * oneMinusT * oneMinusT - 6.0f * t * oneMinusT;
    derivative[2] = 6.0f * t * oneMinusT - 3.0f * t * t;
    derivative[3] = 3.0f * t * t;
}
} // namespace

glm::vec3 BezierPatch::eval(float u, float v, glm::vec3& outNormal) const {
    float bu[4], du[4], bv[4], dv[4];
    bernstein(u, bu, du); bernstein(v, bv, dv);
    glm::vec3 position(0.0f), duPosition(0.0f), dvPosition(0.0f);
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            position += bu[i] * bv[j] * cp[i][j];
            duPosition += du[i] * bv[j] * cp[i][j];
            dvPosition += bu[i] * dv[j] * cp[i][j];
        }
    }
    glm::vec3 normal = glm::cross(duPosition, dvPosition);
    const float length = glm::length(normal);
    outNormal = length < 1e-6f ? glm::vec3(0.0f, 1.0f, 0.0f) : normal / length;
    if (outNormal.y < 0.0f) outNormal = -outNormal;
    return position;
}

void BezierPatch::tessellate(int steps, const glm::vec3& linearColor, MeshBuilder& builder,
                             const glm::mat4& transform) const {
    steps = std::max(1, steps);
    const uint32_t base = static_cast<uint32_t>(builder.verts.size());
    const glm::mat3 normalTransform = MathUtil::normalMatrix(transform);
    const int stride = steps + 1;
    for (int y = 0; y <= steps; ++y) {
        const float v = static_cast<float>(y) / static_cast<float>(steps);
        for (int x = 0; x <= steps; ++x) {
            const float u = static_cast<float>(x) / static_cast<float>(steps);
            glm::vec3 normal;
            const glm::vec3 position = eval(u, v, normal);
            glm::vec3 transformedNormal = normalTransform * normal;
            const float normalLength = glm::length(transformedNormal);
            if (normalLength < 1e-6f) transformedNormal = glm::vec3(0.0f, 1.0f, 0.0f);
            else transformedNormal /= normalLength;
            builder.verts.push_back({glm::vec3(transform * glm::vec4(position, 1.0f)),
                                     transformedNormal, glm::vec2(u, v), linearColor});
        }
    }
    for (int y = 0; y < steps; ++y) {
        for (int x = 0; x < steps; ++x) {
            const uint32_t i0 = base + static_cast<uint32_t>(y * stride + x);
            const uint32_t i1 = i0 + 1;
            const uint32_t i2 = i0 + static_cast<uint32_t>(stride);
            const uint32_t i3 = i2 + 1;
            builder.indices.insert(builder.indices.end(), {i0, i1, i2, i1, i3, i2});
        }
    }
}
