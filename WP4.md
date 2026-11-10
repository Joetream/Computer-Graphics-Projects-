# 工作包 WP-4：自然地形、河湾水面、实例化稻田、程序化天空盒、三态光影与粒子系统

> **给 Agent 的执行指令**：
> 你正在参与开发 `D:/Antigravity_work/YanbianVillage`（C++17 + OpenGL 3.3 Core Profile）。
> 本工作包（WP-4）的所有 `.h` 头文件均已存在于 `src/scene/` 和 `src/fx/` 下，**请勿修改任何 `.h` 头文件**。
> 请严格按照下方给定的头文件声明，编写并直接写入（或完整输出）以下 **7 个 `.cpp` 实现文件**的完整源码，严禁使用 `// TODO` 或省略任何代码：
> 1. `src/scene/Terrain.cpp`
> 2. `src/scene/Water.cpp`
> 3. `src/scene/RiceField.cpp`
> 4. `src/fx/LightingState.cpp`
> 5. `src/fx/SkyGenerator.cpp`
> 6. `src/fx/PlanarShadow.cpp`
> 7. `src/fx/ParticleSystem.cpp`

---

## 一、已锁定的头文件契约（请勿改动 `.h`）

### 1. `src/scene/Terrain.h` & `src/scene/Water.h` & `src/scene/RiceField.h`
```cpp
class Terrain {
public:
    void init();
    void draw(const Shader& shader) const;
    float getHeight(float x, float z) const;
private:
    Mesh m_mesh;
};

class Water {
public:
    void init();
    void draw(const Shader& shader) const;
private:
    Mesh m_mesh;
};

class RiceField {
public:
    void init(const Terrain& terrain);
    void updateVisibleInstances(const Frustum& frustum);
    void draw(const Shader& shader) const;
    int visibleChunks() const { return m_visibleChunks; }
    int totalChunks() const { return static_cast<int>(m_chunks.size()); }
private:
    struct Chunk {
        glm::vec3 minPt;
        glm::vec3 maxPt;
        std::vector<glm::mat4> instances;
    };
    Mesh m_stalkMesh;
    std::vector<Chunk> m_chunks;
    std::vector<glm::mat4> m_visibleBuffer;
    int m_visibleChunks = 0;
    int m_visibleInstanceCount = 0;
};
```

### 2. `src/fx/LightingState.h` & `src/fx/SkyGenerator.h` & `src/fx/PlanarShadow.h` & `src/fx/ParticleSystem.h`
```cpp
class LightingState {
public:
    LightingState();
    void setPreset(int index); // 0: Dawn, 1: Noon, 2: Night
    void toggleSeason();       // 0: Autumn, 1: Winter
    void update(float dt);
    void applyToShader(const Shader& shader) const;
    void drawLampPosts(const Shader& shader) const;
    glm::vec3 sunDir() const { return m_currSunDir; }
    bool shadowEnabled() const { return m_targetPreset != 2; }
    int skyIndexA() const { return m_skyA; }
    int skyIndexB() const { return m_skyB; }
    float skyBlend() const { return m_skyBlend; }
    float snowAmount() const { return m_snowAmount; }
    // private members already defined in src/fx/LightingState.h
};

class SkyGenerator {
public:
    static unsigned int createProceduralCubemap(int presetType);
};

class PlanarShadow {
public:
    static glm::mat4 buildShadowMatrix(const glm::vec4& groundPlane, const glm::vec4& lightDir);
};

class ParticleSystem {
public:
    ParticleSystem() = default;
    ~ParticleSystem();
    void init(const std::vector<glm::vec3>& chimneyTops);
    void setMode(int mode); // 0 = Chimney Smoke (Autumn), 1 = Snowfall (Winter)
    void update(float dt, const glm::vec3& camPos);
    void draw(const Shader& particleShader, const glm::mat4& view, const glm::mat4& proj, const glm::vec3& camPos) const;
    // private members already defined in src/fx/ParticleSystem.h
};
```

---

## 二、需要你编写的 7 个 `.cpp` 文件详细规范

### 1. `src/scene/Terrain.cpp`
- `getHeight(x, z)`：
  - 计算半径 $r = \sqrt{x^2 + z^2}$ 与方位角 $\theta = \text{atan2}(z, x)$。
  - 在布尔哈通河湾弧段（$r \in [23.5, 31.5], z > 4.0$）：平滑下凹至 $y = -0.85\text{ m}$ 形成河床。
  - 在村落核心区 $r < 36$：除河床外严格保持 $y = 0.0\text{ m}$，确保房屋、门楼与平面投影阴影（$y=0.015$）完美贴合无穿模。
  - 在远景长白山圈 $r > 82$：用多层正弦/哈希分形噪声（FBM）乘以 `smoothstep(82.0f, 155.0f, r) * 28.0f` 隆起环村群山。
- `init()`：采样 `[-200, 200]^2` 网格（`180 x 180` 分段），用中心差分计算平滑法线；根据高度与区域赋予线性空间顶点色（庭院暖沙土、河岸鹅卵石灰、中景秋草绿、远山冷黛色 `#6E7F8F`、山巅积雪白）。

### 2. `src/scene/Water.cpp`
- `init()`：在布尔哈通河湾区域（$r \in [23.8, 31.2]$，角度覆盖村口前方弧段 $z > 4.5$）生成高密度扇环网格（`90 x 18`），水面基准高度 $y = -0.15\text{ m}$，顶点色设为清澈河水蓝绿 `MathUtil::srgbToLinear(0x3A7CA8)`。

### 3. `src/scene/RiceField.cpp`
- `init(const Terrain& terrain)`：
  - 构建单丛低多边形金色稻穗网格 `m_stalkMesh`（由 3 片交叉倾斜的细长叶片/稻穗组成，高约 $0.95\text{ m}$，颜色 `MathUtil::srgbToLinear(0xD9A441)`）。
  - 在中景环带 $r \in [38, 80]$（避开 $z > 25, |x| < 10$ 的迎宾大道及风机底座）划分约 40 个 `Chunk`，每个 `Chunk` 内生成 `8 x 8 = 64` 株随机微扰旋转与缩放的实例矩阵，并计算该 `Chunk` 的 `minPt / maxPt` AABB。
- `updateVisibleInstances(const Frustum& frustum)`：
  - 遍历 `m_chunks`，用 `frustum.intersectAABB(c.minPt, c.maxPt)` 剔除视野外地块，将可见地块的实例矩阵汇总到 `m_visibleBuffer` 并调用 `m_stalkMesh.setInstances(m_visibleBuffer)`。

### 4. `src/fx/LightingState.cpp`
- 配置三态预设：
  - **0 晨曦初露**：`sunDir = normalize(vec3(0.75, 0.26, -0.45))`，暖金阳光、暖粉橙晨雾、`pointLightFactor = 0.0f`。
  - **1 正午晴空**：`sunDir = normalize(vec3(0.25, 0.92, -0.28))`，明亮白光、淡蓝薄雾、`pointLightFactor = 0.0f`。
  - **2 星夜灯火**：`sunDir = normalize(vec3(-0.35, 0.65, -0.55))`，微弱深蓝月光、深蓝夜雾、`pointLightFactor = 1.0f`。
- 配置 6 盏村落路灯点光源坐标（门楼两侧 2 盏 + 核心庭院与河湾步道 4 盏），并在 `drawLampPosts` 中绘制带暖黄灯头的复古木质路灯柱网格。
- `applyToShader(shader)`：传入 `u_SunDir`, `u_SunColor`, `u_Ambient`, `u_FogColor`, `u_FogDensity`, `u_PointLightFactor`, `u_SnowAmount`, `u_NumPointLights=6`, 以及 `u_PointLightPos[0..5]` 和 `u_PointLightColor[0..5]`。

### 5. `src/fx/SkyGenerator.cpp`
- `createProceduralCubemap(int presetType)`：
  - 在 CPU 端生成 6 个面（`256 x 256` RGB8），调用 `glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, ...)` 上传至 GPU：
  - 根据方向向量 `dir = normalize(...)` 计算：
    1. 天顶到地平线的大气渐变色（晨曦金粉紫渐变 / 正午晴蓝渐变 / 星夜深邃蓝黑渐变）；
    2. 太阳或月亮圆盘高光与柔和大气光晕；
    3. 星夜模式下基于方向哈希生成闪烁繁星；
    4. 地平线附近（`dir.y` 较低处）叠加远景长白山天池主峰与群山剪影，山巅带有积雪高光。

### 6. `src/fx/PlanarShadow.cpp`
- `buildShadowMatrix(groundPlane, lightDir)`：
  严格按公式 $M_{c,r} = (\text{dot}(\text{plane}, \text{lightDir}))\delta_{r,c} - \text{lightDir}[r]\cdot\text{plane}[c]$ 构造列主序 $4\times 4$ 平面投影阴影矩阵。

### 7. `src/fx/ParticleSystem.cpp`
- 初始化约 360 个粒子：
  - 当 `m_mode == 0`（金秋炊烟）：粒子从 `m_emitters`（各民居独立火炕烟囱顶端）循环发射，缓慢上升并随正弦微风漂移，尺寸随生命周期变大，颜色为暖灰白。
  - 当 `m_mode == 1`（初冬瑞雪）：粒子在相机周围 `[-45, 45] x [0, 28] x [-45, 45]` 空间内飘落，颜色为晶莹雪白。
- `update` 中更新粒子并在 `draw` 中按到 `camPos` 的距离**由远及近排序**，上传实例缓冲并用广告牌 Quad 绘制。
