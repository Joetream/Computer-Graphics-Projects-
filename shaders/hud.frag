#version 330 core
// ============================================================================
// WP-5 · hud.frag
// HUD 纹理采样 + 可选像素网格线：凸显 2D 光栅化沙盘的像素颗粒
// ============================================================================

in vec2 v_UV;

out vec4 FragColor;

uniform vec4 u_RectNDC;        // 契约 Uniform（顶点阶段使用）
uniform sampler2D u_HudTex;    // texture unit 0
uniform bool  u_ShowPixelGrid;
uniform float u_GridRes;

void main() {
    vec4 texel = texture(u_HudTex, v_UV);

    if (u_ShowPixelGrid) {
        // 每个像素格边界处叠加细微深色网格线，fwidth 保证不同缩放下线宽稳定
        vec2 gridUV     = v_UV * u_GridRes;
        vec2 cell       = fract(gridUV);
        vec2 fw         = max(fwidth(gridUV), vec2(1e-4));
        vec2 distToEdge = min(cell, 1.0 - cell) / fw;
        float line      = 1.0 - smoothstep(0.0, 1.5, min(distToEdge.x, distToEdge.y));

        texel.rgb = mix(texel.rgb, texel.rgb * 0.45, line * 0.55);
    }

    FragColor = texel;
}
