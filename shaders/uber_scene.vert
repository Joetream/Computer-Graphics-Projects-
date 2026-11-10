#version 330 core
// ============================================================================
// WP-5 · uber_scene.vert
// 主场景 Uber 顶点着色器：实例化 / 非实例化模型变换、水面与稻浪顶点动画、
// 平面投影阴影 Pass、Gouraud 逐顶点 Blinn-Phong 光照。
// 矩阵一律遵守列向量约定：P * V * M * v
// ============================================================================

// ---------- 顶点属性（与 Mesh.cpp 的 VAO 布局严格对应）----------
layout(location = 0) in vec3 a_Pos;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_UV;
layout(location = 3) in vec3 a_Color;
layout(location = 4) in mat4 a_InstanceModel;  // 占用 location 4, 5, 6, 7

// ---------- 变换矩阵 ----------
uniform mat4 u_Model;
uniform mat3 u_NormalMat;
uniform mat4 u_View;
uniform mat4 u_Proj;
uniform mat4 u_ShadowProjMat;

// ---------- 渲染状态开关 ----------
uniform bool  u_Instanced;      // true: a_InstanceModel; false: u_Model
uniform int   u_AnimType;       // 0 静态 / 1 水波 / 2 稻浪
uniform int   u_ShadeMode;      // 0 Flat / 1 Gouraud / 2 Blinn-Phong
uniform bool  u_IsShadowPass;   // true: 平面投影阴影 Pass
uniform bool  u_UseTexture;     // true: 采样 u_AlbedoTex（片元阶段）
uniform bool  u_UseEnvMap;      // true: 水面天空盒反射（片元阶段）
uniform float u_Time;

// ---------- 光照与材质参数 ----------
uniform vec3  u_SunDir;         // 由表面指向太阳的方向
uniform vec3  u_SunColor;
uniform vec3  u_Ambient;
uniform vec3  u_CamPos;
uniform float u_Shininess;
uniform int   u_NumPointLights;
uniform vec3  u_PointLightPos[6];
uniform vec3  u_PointLightColor[6];
uniform float u_PointLightFactor;   // 夜间 1.0 / 白天 0.0
uniform float u_SnowAmount;         // 0.0 金秋 -> 1.0 初冬瑞雪

// ---------- 大气雾与平面阴影 ----------
uniform vec3  u_FogColor;
uniform float u_FogDensity;
uniform vec4  u_ShadowColor;

// ---------- 纹理采样器（按契约完整列出；采样发生在片元阶段）----------
uniform sampler2D   u_AlbedoTex;    // texture unit 0
uniform samplerCube u_SkyTexA;      // texture unit 1
uniform samplerCube u_SkyTexB;      // texture unit 2
uniform float       u_SkyBlend;

// ---------- 传给片元着色器的插值量（与 uber_scene.frag 的 in 一一对应）----------
out vec3 v_WorldPos;
out vec3 v_Normal;
out vec2 v_UV;
out vec3 v_Color;
out vec3 v_GouraudDiffuse;
out vec3 v_GouraudSpec;

#include "common/lighting.glsl"

void main() {
    float t = u_Time;

    // ------------------------------------------------------------------
    // 1. 模型 / 法线矩阵：实例化与非实例化两条路径
    // ------------------------------------------------------------------
    mat4 modelMat  = u_Instanced ? a_InstanceModel : u_Model;
    mat3 normalMat = u_Instanced
                   ? transpose(inverse(mat3(a_InstanceModel)))
                   : u_NormalMat;

    vec3 worldPos    = (modelMat * vec4(a_Pos, 1.0)).xyz;
    vec3 worldNormal = normalize(normalMat * a_Normal);

    // ------------------------------------------------------------------
    // 2. 顶点动画（阴影 Pass 不参与动画，保证投影轮廓稳定）
    // ------------------------------------------------------------------
    if (!u_IsShadowPass && u_AnimType == 1) {
        // 布尔哈通河：h(x,z,t) = 0.08 sin(1.1x + 2.2t) + 0.06 sin(1.4z - 1.8t)
        float phaseX = 1.1 * worldPos.x + 2.2 * t;
        float phaseZ = 1.4 * worldPos.z - 1.8 * t;
        float dh     = 0.08 * sin(phaseX) + 0.06 * sin(phaseZ);

        // 解析偏导：dh/dx = 0.088 cos(phaseX), dh/dz = 0.084 cos(phaseZ)
        float dhdx = 0.08 * 1.1 * cos(phaseX);
        float dhdz = 0.06 * 1.4 * cos(phaseZ);

        worldPos.y += dh;
        // N = normalize(-dh/dx, 1, -dh/dz)，再变换到世界空间
        worldNormal = normalize(normalMat * normalize(vec3(-dhdx, 1.0, -dhdz)));

    } else if (!u_IsShadowPass && u_AnimType == 2) {
        // 金色稻浪：以局部顶点高度 a_Pos.y 加权，越高摆幅越大
        float heightWeight = clamp(a_Pos.y, 0.0, 2.0);
        float gustX = sin(1.7 * t + 0.35 * worldPos.x + 0.22 * worldPos.z);
        float gustZ = sin(1.25 * t + 0.28 * worldPos.z - 0.18 * worldPos.x);
        worldPos.x += 0.14 * heightWeight * gustX;
        worldPos.z += 0.09 * heightWeight * gustZ;
        // 稻叶整体摆动，法线保持插值法线即可（微幅形变）
    }

    // ------------------------------------------------------------------
    // 3. Gouraud：逐顶点完成平行光 + 6 盏点光源的 Blinn-Phong 计算
    //    v_GouraudDiffuse 含环境项，v_GouraudSpec 为镜面项（均为线性空间）
    // ------------------------------------------------------------------
    v_GouraudDiffuse = u_Ambient;
    v_GouraudSpec    = vec3(0.0);

    if (u_ShadeMode == 1 && !u_IsShadowPass) {
        vec3 V = normalize(u_CamPos - worldPos);
        evaluateBlinnPhong(
            worldPos, worldNormal, V,
            u_SunDir, u_SunColor, u_Ambient, u_Shininess,
            u_NumPointLights, u_PointLightPos, u_PointLightColor, u_PointLightFactor,
            v_GouraudDiffuse, v_GouraudSpec
        );
    }

    // ------------------------------------------------------------------
    // 4. 通用插值输出
    // ------------------------------------------------------------------
    v_Normal = worldNormal;
    v_UV     = a_UV;
    v_Color  = a_Color;

    // ------------------------------------------------------------------
    // 5. 裁剪坐标：阴影 Pass 追加平面投影矩阵，其余 P * V * worldPos
    //    阴影 Pass 的 v_WorldPos 取投影后的落点，供片元阶段做 35 m 圆盘裁剪
    // ------------------------------------------------------------------
    if (u_IsShadowPass) {
        vec4 shadowWorld = u_ShadowProjMat * vec4(worldPos, 1.0);
        v_WorldPos = shadowWorld.xyz / max(shadowWorld.w, 1e-5);
        gl_Position = u_Proj * u_View * u_ShadowProjMat * vec4(worldPos, 1.0);
    } else {
        v_WorldPos = worldPos;
        gl_Position = u_Proj * u_View * vec4(worldPos, 1.0);
    }
}
