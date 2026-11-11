#pragma once
#include "core/MeshBuilder.h"
#include <glm/glm.hpp>

class BezierRoof {
public:
    void build(float width, float depth, float height, float cornerLift, int tessLevel,
               MeshBuilder& outBuilder, const glm::mat4& localTransform = glm::mat4(1.0f)) const;
    static int stepsFromLevel(int level);

    // 墙顶与屋面之间的封檐垫板（墙体为以原点为中心、半宽 wallHalfX、半深 wallHalfZ 的矩形，
    // 墙顶位于屋顶局部坐标 y=0）。参数须与同一屋顶的 build() 一致。
    static void buildEaveSeal(float width, float depth, float height, float cornerLift, int tessLevel,
                              float wallHalfX, float wallHalfZ, const glm::vec3& color,
                              MeshBuilder& outBuilder, const glm::mat4& localTransform = glm::mat4(1.0f));
};
