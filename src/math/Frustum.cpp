#include "math/Frustum.h"

namespace {
glm::vec4 row(const glm::mat4& matrix, int rowIndex) {
    return glm::vec4(matrix[0][rowIndex], matrix[1][rowIndex],
                     matrix[2][rowIndex], matrix[3][rowIndex]);
}

glm::vec4 normalizedPlane(const glm::vec4& plane) {
    const float length = glm::length(glm::vec3(plane));
    return length > 1e-6f ? plane / length : plane;
}
} // namespace

void Frustum::extract(const glm::mat4& vp) {
    const glm::vec4 r0 = row(vp, 0), r1 = row(vp, 1), r2 = row(vp, 2), r3 = row(vp, 3);
    planes[0] = normalizedPlane(r3 + r0);
    planes[1] = normalizedPlane(r3 - r0);
    planes[2] = normalizedPlane(r3 + r1);
    planes[3] = normalizedPlane(r3 - r1);
    planes[4] = normalizedPlane(r3 + r2);
    planes[5] = normalizedPlane(r3 - r2);
}

bool Frustum::intersectAABB(const glm::vec3& minPt, const glm::vec3& maxPt) const {
    for (const glm::vec4& plane : planes) {
        const glm::vec3 positive(
            plane.x >= 0.0f ? maxPt.x : minPt.x,
            plane.y >= 0.0f ? maxPt.y : minPt.y,
            plane.z >= 0.0f ? maxPt.z : minPt.z);
        if (glm::dot(glm::vec3(plane), positive) + plane.w < 0.0f) return false;
    }
    return true;
}
