#pragma once
#include "core/MeshBuilder.h"
#include <glm/glm.hpp>

class BezierRoof {
public:
    void build(float width, float depth, float height, float cornerLift, int tessLevel,
               MeshBuilder& outBuilder, const glm::mat4& localTransform = glm::mat4(1.0f)) const;
    static int stepsFromLevel(int level);
};
