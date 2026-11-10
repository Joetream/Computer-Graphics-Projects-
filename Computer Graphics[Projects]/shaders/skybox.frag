#version 330 core
// ============================================================================
// WP-5 · skybox.frag
// 天空盒片元着色器：双 cubemap 按 u_SkyBlend 线性插值（晨曦 / 正午 / 星夜过渡）
// ============================================================================

in vec3 v_TexCoord;

out vec4 FragColor;

// 契约 Uniform（视图 / 投影矩阵在顶点阶段使用，此处按契约完整声明）
uniform mat4 u_View;
uniform mat4 u_Proj;
uniform samplerCube u_SkyTexA;   // texture unit 1
uniform samplerCube u_SkyTexB;   // texture unit 2
uniform float       u_SkyBlend;

void main() {
    vec3 skyA = texture(u_SkyTexA, v_TexCoord).rgb;
    vec3 skyB = texture(u_SkyTexB, v_TexCoord).rgb;
    vec3 sky  = mix(skyA, skyB, clamp(u_SkyBlend, 0.0, 1.0));
    FragColor = vec4(sky, 1.0);
}
