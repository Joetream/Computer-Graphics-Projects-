#version 330 core
// ============================================================================
// WP-5 · skybox.vert
// 天空盒顶点着色器：立方体坐标直接作为 cubemap 采样方向，
// 通过 .xyww 强制天空盒始终位于远裁剪面（深度值恒为 1.0）。
// ============================================================================

layout(location = 0) in vec3 a_Pos;

// 契约 Uniform（采样器与混合系数在片元阶段使用，此处按契约完整声明）
uniform mat4 u_View;
uniform mat4 u_Proj;
uniform samplerCube u_SkyTexA;   // texture unit 1
uniform samplerCube u_SkyTexB;   // texture unit 2
uniform float       u_SkyBlend;

out vec3 v_TexCoord;

void main() {
    v_TexCoord = a_Pos;
    gl_Position = (u_Proj * u_View * vec4(a_Pos, 1.0)).xyww;
}
