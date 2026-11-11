#pragma once
#include <glm/glm.hpp>

class PlanarShadow {
public:
    // groundPlane = (nx, ny, nz, d) where n.X + d = 0; lightDir = (Lx, Ly, Lz, 0) for directional light
    static glm::mat4 buildShadowMatrix(const glm::vec4& groundPlane, const glm::vec4& lightDir);
};
