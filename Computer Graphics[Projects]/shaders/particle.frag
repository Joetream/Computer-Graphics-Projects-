#version 330 core
// ============================================================================
// WP-5 · particle.frag
// 圆形软边粒子：半径外裁剪，边缘 smoothstep 软衰减（配合 alpha blending）
// ============================================================================

in vec2 v_UV;
in vec4 v_Color;

out vec4 FragColor;

// 契约 Uniform（变换 / 广告牌向量在顶点阶段使用，此处按契约完整声明）
uniform mat4 u_View;
uniform mat4 u_Proj;
uniform vec3 u_CamRight;
uniform vec3 u_CamUp;

void main() {
    float r = length(v_UV - 0.5) * 2.0;
    if (r > 1.0) discard;

    float soft = smoothstep(1.0, 0.2, r);
    FragColor = vec4(v_Color.rgb, v_Color.a * soft);
}
