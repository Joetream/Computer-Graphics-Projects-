// ============================================================================
// common/lighting.glsl
// 模块化公共光照库（由 Shader::readFileWithIncludes 在编译前递归展开）
// 包含：点光源二次距离衰减公式、平行光 + 多点光源 Blinn-Phong 光照模型
// ============================================================================

const int MAX_POINT_LIGHTS = 6;

// 点光源二次距离衰减：f_att(d) = 1.0 / (1.0 + 0.14 * d + 0.07 * d^2)
float pointAttenuation(float d) {
    return 1.0 / (1.0 + 0.14 * d + 0.07 * d * d);
}

// 统一计算太阳平行光 + 6 盏村落路灯点光源的 Blinn-Phong 漫反射与镜面高光
void evaluateBlinnPhong(
    vec3 worldPos,
    vec3 N,
    vec3 V,
    vec3 sunDir,
    vec3 sunColor,
    vec3 ambient,
    float shininess,
    int numPointLights,
    vec3 pointLightPos[6],
    vec3 pointLightColor[6],
    float pointLightFactor,
    out vec3 outDiffuse,
    out vec3 outSpec
) {
    vec3 L = normalize(sunDir);
    vec3 H = normalize(L + V);
    float NdotL = max(dot(N, L), 0.0);
    float NdotH = max(dot(N, H), 0.0);

    outDiffuse = ambient + sunColor * NdotL;
    outSpec    = sunColor * pow(NdotH, shininess) * step(1e-4, NdotL);

    for (int i = 0; i < MAX_POINT_LIGHTS; ++i) {
        if (i >= numPointLights) break;
        vec3  toLight = pointLightPos[i] - worldPos;
        float d       = length(toLight);
        vec3  Lp      = toLight / max(d, 1e-4);
        vec3  Hp      = normalize(Lp + V);
        float att     = pointAttenuation(d) * pointLightFactor;

        float pNdotL  = max(dot(N, Lp), 0.0);
        outDiffuse   += pointLightColor[i] * pNdotL * att;
        outSpec      += pointLightColor[i] * pow(max(dot(N, Hp), 0.0), shininess) * att * step(1e-4, pNdotL);
    }
}
