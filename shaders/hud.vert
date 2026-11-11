#version 330 core
// ============================================================================
// WP-5 · hud.vert
// 2D HUD 叠加层：把 [0,1]^2 的顶点位置映射到 u_RectNDC 指定的屏幕 NDC 矩形
// （HUD 关闭深度测试，z 固定为 0）
// ============================================================================

layout(location = 0) in vec2 a_Pos;
layout(location = 1) in vec2 a_UV;

uniform vec4 u_RectNDC;        // xy = 左下角 NDC, zw = 右上角 NDC
uniform sampler2D u_HudTex;    // 契约 Uniform（片元阶段采样）
uniform bool  u_ShowPixelGrid; // 契约 Uniform（片元阶段使用）
uniform float u_GridRes;       // 契约 Uniform（片元阶段使用）

out vec2 v_UV;

void main() {
    vec2 ndc = mix(u_RectNDC.xy, u_RectNDC.zw, a_Pos);
    v_UV = a_UV;
    gl_Position = vec4(ndc, 0.0, 1.0);
}
