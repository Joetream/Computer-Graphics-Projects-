#pragma once
#include <glm/glm.hpp>

class Frustum {
public:
    glm::vec4 planes[6];

    void extract(const glm::mat4& vp);
    bool intersectAABB(const glm::vec3& minPt, const glm::vec3& maxPt) const;
};
