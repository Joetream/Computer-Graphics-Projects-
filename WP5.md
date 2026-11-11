# 工作包 WP-5：GLSL 3.3 Core 可编程渲染管线着色器套件

> **给 Agent 的执行指令**：
> 你正在参与开发 `D:/Antigravity_work/YanbianVillage`（C++17 + OpenGL 3.3 Core Profile）。
> 请严格按照下方给定的 Uniform 与顶点属性契约，编写并直接写入（或完整输出）`shaders/` 目录下的全部 **8 个 GLSL 着色器文件**源码（均以 `#version 330 core` 开头），严禁省略任何代码：
> 1. `shaders/uber_scene.vert`
> 2. `shaders/uber_scene.frag`
> 3. `shaders/skybox.vert`
> 4. `shaders/skybox.frag`
> 5. `shaders/particle.vert`
> 6. `shaders/particle.frag`
> 7. `shaders/hud.vert`
> 8. `shaders/hud.frag`

---

## 一、`shaders/uber_scene.vert` & `shaders/uber_scene.frag` 精确契约

### 1. 顶点输入（Vertex Attributes）
```glsl
layout(location = 0) in vec3 a_Pos;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_UV;
layout(location = 3) in vec3 a_Color;
layout(location = 4) in mat4 a_InstanceModel; // occupies locations 4, 5, 6, 7
```

### 2. 主场景 Uniform 列表（名称必须与 `App.cpp` 和 `LightingState.cpp` 完全一致）
```glsl
// 变换矩阵
uniform mat4 u_Model;
uniform mat3 u_NormalMat;
uniform mat4 u_View;
uniform mat4 u_Proj;
uniform mat4 u_ShadowProjMat;

// 渲染状态开关
uniform bool  u_Instanced;       // true: 使用 a_InstanceModel; false: 使用 u_Model
uniform int   u_AnimType;        // 0: 静态, 1: 布尔哈通河正弦水波, 2: 金色稻浪随风摆动
uniform int   u_ShadeMode;       // 0: Flat, 1: Gouraud (逐顶点), 2: Blinn-Phong (逐片元)
uniform bool  u_IsShadowPass;    // true: 平面投影阴影 Pass
uniform bool  u_UseTexture;      // true: 采样 u_AlbedoTex
uniform bool  u_UseEnvMap;       // true: 水面采样天空盒反射
uniform float u_Time;

// 光照与材质参数
uniform vec3  u_SunDir;
uniform vec3  u_SunColor;
uniform vec3  u_Ambient;
uniform vec3  u_CamPos;
uniform float u_Shininess;
uniform int   u_NumPointLights;
uniform vec3  u_PointLightPos[6];
uniform vec3  u_PointLightColor[6];
uniform float u_PointLightFactor; // 夜间为 1.0, 白天为 0.0
uniform float u_SnowAmount;       // 0.0 (金秋) -> 1.0 (初冬瑞雪)

// 大气雾与平面阴影
uniform vec3  u_FogColor;
uniform float u_FogDensity;
uniform vec4  u_ShadowColor;

// 纹理采样器
uniform sampler2D   u_AlbedoTex; // unit 0
uniform samplerCube u_SkyTexA;   // unit 1
uniform samplerCube u_SkyTexB;   // unit 2
uniform float       u_SkyBlend;
```

### 3. 核心着色器算法要求
1. **顶点动画（`uber_scene.vert`）**：
   - 若 `u_AnimType == 1`（水面）：对世界坐标 $y$ 叠加双正弦波 $dh = 0.08\sin(1.1x + 2.2t) + 0.06\sin(1.4z - 1.8t)$，并用解析偏导重算法线 $N = \text{normalize}(-\partial h/\partial x, 1.0, -\partial h/\partial z)$。
   - 若 `u_AnimType == 2`（稻田）：根据局部顶点高度 `a_Pos.y` 对世界坐标 $x, z$ 施加微风正弦偏移，实现随风起伏的稻浪。
   - 若 `u_IsShadowPass == true`：`gl_Position = u_Proj * u_View * u_ShadowProjMat * worldPos;`
   - 否则：`gl_Position = u_Proj * u_View * worldPos;`（严格遵守列向量 `P * V * M * v`）。
2. **三种明暗处理模式（按 `F` 键切换对比）**：
   - `u_ShadeMode == 0` (**Flat 平面着色**)：在 `uber_scene.frag` 中利用 `normalize(cross(dFdx(v_WorldPos), dFdy(v_WorldPos)))` 求当前三角面片几何法线并计算 Blinn-Phong 光照，使飞檐与山体呈现出鲜明的低多边形棱面折光感。
   - `u_ShadeMode == 1` (**Gouraud 顶点着色**)：在 `uber_scene.vert` 中直接完成平行光 + 6 盏点光源的 Blinn-Phong 漫反射与镜面高光计算，通过插值变量 `v_GouraudDiffuse` 和 `v_GouraudSpec` 传给片元着色器。
   - `u_ShadeMode == 2` (**Blinn-Phong 片元着色**)：在 `uber_scene.frag` 中对插值法线归一化后逐片元计算半程向量 $H = \text{normalize}(L + V)$ 高光与点光源二次距离衰减 $f_{\text{att}} = \frac{1}{1.0 + 0.14d + 0.07d^2}$。
3. **平面阴影裁剪、初冬瑞雪、水面反射、高度雾与 Gamma 校正**：
   - 当 `u_IsShadowPass == true` 时：若 `length(v_WorldPos.xz) > 35.0` 则 `discard`，否则直接输出 `FragColor = u_ShadowColor; return;`。
   - 当 `u_SnowAmount > 0.01` 时：对朝上的表面（`N.y > 0.45`）平滑混合积雪白 `vec3(0.92, 0.94, 0.98)`。
   - 当 `u_UseEnvMap == true` 时：计算反射向量 `R = reflect(-V, N)`，采样 `mix(texture(u_SkyTexA, R).rgb, texture(u_SkyTexB, R).rgb, u_SkyBlend)`，并按 Schlick Fresnel $F = 0.08 + 0.92(1 - \max(N\cdot V, 0))^4$ 混合。
   - 根据相机距离与世界高度混入指数高度雾 `u_FogColor`。
   - 最终输出前执行 Gamma 校正：`color = pow(max(color, vec3(0.0)), vec3(1.0 / 2.2));`。

---

## 二、其余 3 对着色器精确契约

### 1. `shaders/skybox.vert` & `shaders/skybox.frag`
- 顶点输入：`layout(location = 0) in vec3 a_Pos;`
- Uniform：`uniform mat4 u_View; uniform mat4 u_Proj; uniform samplerCube u_SkyTexA; uniform samplerCube u_SkyTexB; uniform float u_SkyBlend;`
- `gl_Position = (u_Proj * u_View * vec4(a_Pos, 1.0)).xyww;`
- 片元采样 `u_SkyTexA` 和 `u_SkyTexB`，按 `u_SkyBlend` 线性插值后输出。

### 2. `shaders/particle.vert` & `shaders/particle.frag`
- 顶点输入：
  - `layout(location = 0) in vec2 a_QuadCorner;`（`[-0.5, 0.5]^2`）
  - `layout(location = 1) in vec3 a_CenterPos;`（divisor = 1）
  - `layout(location = 2) in float a_Size;`（divisor = 1）
  - `layout(location = 3) in vec4 a_Color;`（divisor = 1）
- Uniform：`uniform mat4 u_View; uniform mat4 u_Proj; uniform vec3 u_CamRight; uniform vec3 u_CamUp;`
- 顶点着色器将广告牌朝向相机展开：`vec3 worldPos = a_CenterPos + (u_CamRight * a_QuadCorner.x + u_CamUp * a_QuadCorner.y) * a_Size;`
- 片元着色器计算距中心半径 $r = \text{length}(v\_UV - 0.5) * 2.0$，若 $r > 1.0$ 则 `discard`，否则乘上软边衰减 `smoothstep(1.0, 0.2, r)`。

### 3. `shaders/hud.vert` & `shaders/hud.frag`
- 顶点输入：`layout(location = 0) in vec2 a_Pos; layout(location = 1) in vec2 a_UV;`（`a_Pos` $\in [0, 1]^2$）
- Uniform：`uniform vec4 u_RectNDC; uniform sampler2D u_HudTex; uniform bool u_ShowPixelGrid; uniform float u_GridRes;`
- 顶点着色器将 `[0,1]^2` 映射到屏幕 NDC 矩形 `[u_RectNDC.xy, u_RectNDC.zw]`。
- 片元着色器采样 `u_HudTex`；当 `u_ShowPixelGrid == true` 时，利用 `fract(v_UV * u_GridRes)` 在每个像素格边界叠加細微深色网格线，直观凸显 2D 光栅化像素颗粒。
