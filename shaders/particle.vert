#version 330 core
// ============================================================================
// WP-5 · particle.vert
// 炊烟 / 瑞雪粒子系统：相机朝向广告牌（Billboard）展开
// location 1/2/3 为逐实例属性（glVertexAttribDivisor = 1）
// ============================================================================

layout(location = 0) in vec2 a_QuadCorner;   // [-0.5, 0.5]^2
layout(location = 1) in vec3 a_CenterPos;    // divisor = 1
layout(location = 2) in float a_Size;        // divisor = 1
layout(location = 3) in vec4  a_Color;       // divisor = 1

uniform mat4 u_View;
uniform mat4 u_Proj;
uniform vec3 u_CamRight;
uniform vec3 u_CamUp;

out vec2 v_UV;
out vec4 v_Color;

void main() {
    // 广告牌四个角沿相机右 / 上向量在世界空间展开
    vec3 worldPos = a_CenterPos
                  + (u_CamRight * a_QuadCorner.x + u_CamUp * a_QuadCorner.y) * a_Size;

    v_UV    = a_QuadCorner + 0.5;   // [-0.5,0.5] -> [0,1]
    v_Color = a_Color;

    gl_Position = u_Proj * u_View * vec4(worldPos, 1.0);
}
