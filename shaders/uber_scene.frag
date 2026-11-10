#version 330 core
// ============================================================================
// WP-5 · uber_scene.frag
// 主场景 Uber 片元着色器：Flat / Gouraud / Blinn-Phong 三种明暗处理、
// 平面阴影圆盘裁剪、初冬瑞雪、水面 Schlick Fresnel 天空盒反射、
// 指数高度雾与最终 Gamma 校正。
// ============================================================================

// ---------- 顶点着色器插值输入 ----------
in vec3 v_WorldPos;
in vec3 v_Normal;
in vec2 v_UV;
in vec3 v_Color;
in vec3 v_GouraudDiffuse;
in vec3 v_GouraudSpec;

out vec4 FragColor;

// ---------- 变换矩阵（按 WP-5 契约完整列出）----------
uniform mat4 u_Model;
uniform mat3 u_NormalMat;
uniform mat4 u_View;
uniform mat4 u_Proj;
uniform mat4 u_ShadowProjMat;

// ---------- 渲染状态开关 ----------
uniform bool  u_Instanced;
uniform int   u_AnimType;
uniform int   u_ShadeMode;
uniform bool  u_IsShadowPass;
uniform bool  u_UseTexture;
uniform bool  u_UseEnvMap;
uniform float u_Time;

// ---------- 光照与材质参数 ----------
uniform vec3  u_SunDir;
uniform vec3  u_SunColor;
uniform vec3  u_Ambient;
uniform vec3  u_CamPos;
uniform float u_Shininess;
uniform int   u_NumPointLights;
uniform vec3  u_PointLightPos[6];
uniform vec3  u_PointLightColor[6];
uniform float u_PointLightFactor;
uniform float u_SnowAmount;

// ---------- 大气雾与平面阴影 ----------
uniform vec3  u_FogColor;
uniform float u_FogDensity;
uniform vec4  u_ShadowColor;

// ---------- 纹理采样器 ----------
uniform sampler2D   u_AlbedoTex;    // texture unit 0
uniform samplerCube u_SkyTexA;      // texture unit 1
uniform samplerCube u_SkyTexB;      // texture unit 2
uniform float       u_SkyBlend;

const int MAX_POINT_LIGHTS = 6;

// 点光源二次距离衰减：f_att = 1 / (1 + 0.14 d + 0.07 d^2)
float pointAttenuation(float d) {
    return 1.0 / (1.0 + 0.14 * d + 0.07 * d * d);
}

// 平行光 + 6 盏点光源的 Blinn-Phong 逐片元光照
void blinnPhong(vec3 N, vec3 V, out vec3 diffuse, out vec3 spec) {
    vec3 L = normalize(u_SunDir);
    vec3 H = normalize(L + V);
    float NdotL = max(dot(N, L), 0.0);
    float NdotH = max(dot(N, H), 0.0);

    diffuse = u_Ambient + u_SunColor * NdotL;
    spec    = u_SunColor * pow(NdotH, u_Shininess) * step(1e-4, NdotL);

    for (int i = 0; i < MAX_POINT_LIGHTS; ++i) {
        if (i >= u_NumPointLights) break;
        vec3  toLight = u_PointLightPos[i] - v_WorldPos;
        float d       = length(toLight);
        vec3  Lp      = toLight / max(d, 1e-4);
        vec3  Hp      = normalize(Lp + V);
        float att     = pointAttenuation(d) * u_PointLightFactor;

        diffuse += u_PointLightColor[i] * max(dot(N, Lp), 0.0) * att;
        spec    += u_PointLightColor[i]
                 * pow(max(dot(N, Hp), 0.0), u_Shininess)
                 * att * step(1e-4, max(dot(N, Lp), 0.0));
    }
}

void main() {
    // ------------------------------------------------------------------
    // 1. 平面投影阴影 Pass：近景核心区 R = 35 m 圆盘裁剪，直接输出阴影色
    // ------------------------------------------------------------------
    if (u_IsShadowPass) {
        if (length(v_WorldPos.xz) > 35.0) discard;
        FragColor = u_ShadowColor;
        return;
    }

    // ------------------------------------------------------------------
    // 2. 基色：顶点色或 u_AlbedoTex（纹理 alpha 保留给 Pass 5 的混合）
    // ------------------------------------------------------------------
    vec4 texel  = u_UseTexture ? texture(u_AlbedoTex, v_UV) : vec4(v_Color, 1.0);
    vec3 albedo = texel.rgb;
    float alpha = texel.a;

    // ------------------------------------------------------------------
    // 3. 法线：Flat 用屏幕空间偏导求三角面几何法线，其余用插值法线
    // ------------------------------------------------------------------
    vec3 interpN = normalize(v_Normal);
    vec3 N = interpN;

    if (u_ShadeMode == 0) {
        // cross(dFdx(P), dFdy(P))：低多边形棱面折光感（飞檐、山体）
        vec3 dpdx  = dFdx(v_WorldPos);
        vec3 dpdy  = dFdy(v_WorldPos);
        vec3 faceN = cross(dpdx, dpdy);
        if (dot(faceN, faceN) > 1e-12) {
            faceN = normalize(faceN);
            // 屏幕坐标 y 方向可能导致叉乘反向，用插值法线做朝向校正
            if (dot(faceN, interpN) < 0.0) faceN = -faceN;
            N = faceN;
        }
    }

    // ------------------------------------------------------------------
    // 4. 初冬瑞雪：朝上表面（N.y > 0.45）平滑混合积雪白
    // ------------------------------------------------------------------
    if (u_SnowAmount > 0.01) {
        float snowMask = smoothstep(0.45, 0.85, N.y) * clamp(u_SnowAmount, 0.0, 1.0);
        albedo = mix(albedo, vec3(0.92, 0.94, 0.98), snowMask);
    }

    // ------------------------------------------------------------------
    // 5. 三种明暗模式汇合
    //    ShadeMode 0/2 -> 逐片元 Blinn-Phong；ShadeMode 1 -> 顶点插值结果
    // ------------------------------------------------------------------
    vec3 V = normalize(u_CamPos - v_WorldPos);
    vec3 diffuse = vec3(0.0);
    vec3 spec    = vec3(0.0);

    if (u_ShadeMode == 1) {
        diffuse = v_GouraudDiffuse;
        spec    = v_GouraudSpec;
    } else {
        blinnPhong(N, V, diffuse, spec);
    }

    vec3 color = albedo * diffuse + spec;

    // ------------------------------------------------------------------
    // 6. 水面天空盒反射 + Schlick Fresnel
    //    R = reflect(-V, N)；F = 0.08 + 0.92 (1 - max(N·V, 0))^4
    // ------------------------------------------------------------------
    if (u_UseEnvMap) {
        vec3 R = reflect(-V, N);
        vec3 sky = mix(texture(u_SkyTexA, R).rgb,
                       texture(u_SkyTexB, R).rgb,
                       u_SkyBlend);
        float F = 0.08 + 0.92 * pow(1.0 - max(dot(N, V), 0.0), 4.0);
        color = mix(color, sky, clamp(F, 0.0, 1.0));
    }

    // ------------------------------------------------------------------
    // 7. 指数高度雾：随相机距离增大、随世界高度升高而变薄
    // ------------------------------------------------------------------
    float dist           = length(u_CamPos - v_WorldPos);
    float heightFalloff  = exp(-max(v_WorldPos.y, 0.0) * 0.045);
    float fogDist        = u_FogDensity * dist * heightFalloff;
    float fogFactor      = 1.0 - exp(-fogDist * fogDist);
    color = mix(color, u_FogColor, clamp(fogFactor, 0.0, 1.0));

    // ------------------------------------------------------------------
    // 8. Gamma 校正（线性空间 -> 显示空间）
    // ------------------------------------------------------------------
    color = pow(max(color, vec3(0.0)), vec3(1.0 / 2.2));
    FragColor = vec4(color, alpha);
}
