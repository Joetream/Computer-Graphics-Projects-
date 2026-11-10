#pragma once
#include <glm/glm.hpp>
#include "core/MeshBuilder.h"

class BezierPatch {
public:
    glm::vec3 cp[4][4];

    glm::vec3 eval(float u, float v, glm::vec3& outNormal) const;
    void tessellate(int steps, const glm::vec3& linearColor, MeshBuilder& builder,
                    const glm::mat4& transform = glm::mat4(1.0f)) const;
};
