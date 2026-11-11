# 多模型协同开发任务书（`planForDs.md`）

**项目名称**：「延边朝鲜族民俗村·乡村振兴」三维数字场景设计与实现  
**工程根目录**：`d:/Antigravity_work/YanbianVillage`  
**技术栈**：C++17 + OpenGL 3.3 Core Profile + GLFW 3.4 + glad + GLM + stb_image  
**协同目标**：将大批量、算法密集型模块与 3000 字报告拆分为 **6 个零耦合工作包（WP-1 ~ WP-6）**，可同时交给 DeepSeek 等外部模型并行生成代码，拿回后直接放入对应目录即可与主工程无缝编译联调。

---

## 一、主从模型分工总览

| 角色 | 负责范围 | 说明 |
| :--- | :--- | :--- |
| **主控 Agent（当前窗口）** | 工程脚手架、第三方库配置（GLFW/glad/GLM/stb）、编译脚本、`core/` 基础封装（`Shader` / `Mesh` / `Texture` / `Camera` / `Input`）、`App` 主循环组装、编译排错与截图验证 | 负责兜底联调与最终编译运行 |
| **外部模型（DeepSeek 等）** | 按下方 **WP-1 ~ WP-6** 独立生成纯算法模块、程序化建模模块、着色器代码与 3000 字课程设计报告 | 每个 WP 均已锁定头文件名与函数签名，**不依赖主控未完成的代码** |

---

## 二、全局冻结接口契约（所有工作包必须严格遵守）

> [!IMPORTANT]
> 发送任务给外部模型时，**必须将本节契约一并复制过去**，确保外部模型产出的 `.h / .cpp` 放进工程就能一次编译通过，不会出现结构体字段名或 Uniform 名称不一致的问题。

### 1. 统一顶点与基础网格构建接口（`src/core/Mesh.h` & `src/core/MeshBuilder.h`）

```cpp
#pragma once
#include <vector>
#include <cstdint>
#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 pos;     // location = 0: 顶点位置
    glm::vec3 normal;  // location = 1: 法向量
    glm::vec2 uv;      // location = 2: 纹理坐标
    glm::vec3 color;   // location = 3: 线性空间顶点色 (sRGB 已做 pow(c, 2.2))
};

class Mesh {
public:
    Mesh() = default;
    void upload(const std::vector<Vertex>& verts, const std::vector<uint32_t>& indices);
    void setInstances(const std::vector<glm::mat4>& models); // location 4..7, divisor=1
    void draw() const;
    void drawInstanced(int count) const;
};

class MeshBuilder {
public:
    std::vector<Vertex>   verts;
    std::vector<uint32_t> indices;

    void clear();
    // 所有图元生成函数均将变换矩阵 M 烘焙进顶点位置，并用 transpose(inverse(mat3(M))) 变换法线
    // 基础图元默认几何中心在原点：
    // - addBox:      [-0.5, 0.5]^3 单位立方体
    // - addCylinder: 底部 y=0 (半径 r0) 到顶部 y=h (半径 r1)，分段数 seg
    // - addQuad:     XZ 平面 [-0.5, 0.5]^2，法线朝 +Y
    // - addGrid:     XZ 平面 [-0.5*w, 0.5*w] x [-0.5*d, 0.5*d]，nx * nz 网格
    void addBox(const glm::mat4& M, const glm::vec3& linearColor);
    void addCylinder(const glm::mat4& M, float r0, float r1, float h, int seg, const glm::vec3& linearColor);
    void addQuad(const glm::mat4& M, const glm::vec3& linearColor);
    void addGrid(float w, float d, int nx, int nz, const glm::vec3& linearColor);
    void append(const MeshBuilder& other, const glm::mat4& M = glm::mat4(1.0f));
    void build(Mesh& outMesh) const;
};
```

### 2. 统一着色器接口（`src/core/Shader.h`）

```cpp
#pragma once
#include <string>
#include <glm/glm.hpp>

class Shader {
public:
    Shader() = default;
    bool load(const std::string& vertPath, const std::string& fragPath);
    void use() const;
    void setInt(const std::string& name, int v) const;
    void setBool(const std::string& name, bool v) const;
    void setFloat(const std::string& name, float v) const;
    void setVec3(const std::string& name, const glm::vec3& v) const;
    void setVec4(const std::string& name, const glm::vec4& v) const;
    void setMat3(const std::string& name, const glm::mat3& m) const;
    void setMat4(const std::string& name, const glm::mat4& m) const;
};
```

### 3. 统一调色板色值（线性空间转换公式：`srgbToLinear(hex)`）
| 语义 | 十六进制 (sRGB) | 对应用途 |
| :--- | :--- | :--- |
| `PAL_WALL_WHITE` | `#F2EFE8` | 朝鲜族民居暖白灰墙 |
| `PAL_ROOF_SLATE` | `#4A5560` | 歇山顶青灰筒瓦 |
| `PAL_WOOD_DARK` | `#5B3A29` | 梁柱、木框门窗格栅、门楼 |
| `PAL_STONE_GREY` | `#9A958C` | 石砌台基、老水井 |
| `PAL_RICE_GOLD` | `#D9A441` | 金秋稻田主色 |
| `PAL_JINDALLAE` | `#E07AA0` | 金达莱点缀色 / 秋千装饰 |
| `PAL_MOUNTAIN` | `#6E7F8F` | 远景长白山体 |
| `PAL_TURBINE_WHITE` | `#E6E9EC` | 现代风力发电机 |
| `PAL_SOLAR_BLUE` | `#2B3A55` | 太阳能光伏板 |

---

## 三、给外部模型（如 DeepSeek）的通用系统提示词

把下面这段话作为**第一条消息（或连同具体 WP 一起）**发给另一个模型：

````markdown
你现在是一名资深 C++17 与 OpenGL 3.3 计算机图形学工程师，正在与我的主控 Agent 协同开发《计算机图形学》课程设计项目——「延边朝鲜族民俗村·乡村振兴」三维数字场景。

【全局严格约束】
1. 语言与标准：C++17，OpenGL 3.3 Core Profile，数学库使用 GLM (`<glm/glm.hpp>`, `<glm/gtc/matrix_transform.hpp>`)，列主序矩阵，矩阵乘法顺序严格为 `P * V * M * v`。
2. 严禁省略代码：所有要求的 `.h` 和 `.cpp` 必须给出【完整可编译源码】，严禁出现 `// TODO`、`// 其余省略`、`...` 等占位符。
3. 严禁修改契约签名：类名、文件名、结构体名、公有方法签名、Uniform 变量名必须与任务书中给定的头文件声明 100% 一致。
4. 注释规范：在核心图形学公式（如 Bresenham 误差项、扫描线活性边表 AET、Cohen-Sutherland 区域码、Bernstein 基函数、B 样条基函数、平面阴影矩阵、视锥平面提取）处写清楚中文数学注释，方便直接引用到课程设计报告中。
5. 输出格式：每个文件用一个独立的带文件路径注释的 C++ 或 GLSL 代码块输出，例如：
   ```cpp
   // filepath: src/math/MathUtil.h
   ...
   ```
````

---

## 四、六大并行工作包（直接复制对应区块发给外部模型即可）

---

### 📦 工作包 WP-1：纯手写数学库、曲面、样条与视锥体（`src/math/`）

**复制以下提示词发给外部模型**：

```markdown
请按【全局严格约束】完成【工作包 WP-1：数学与曲线曲面底层库】，完整输出以下 5 组文件（共 9 个文件）的全部源码：

1. `src/math/MathUtil.h` & `src/math/MathUtil.cpp`
   - 手写推导并实现（不得直接调用 glm::lookAt / glm::perspective / glm::ortho，需按教材公式逐元素填充列主序 `glm::mat4`）：
     - `glm::mat4 lookAt(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up);`
     - `glm::mat4 perspective(float fovyRad, float aspect, float zNear, float zFar);`
     - `glm::mat4 ortho(float left, float right, float bottom, float top, float zNear, float zFar);`
     - `glm::mat4 translate(const glm::vec3& t);`
     - `glm::mat4 scale(const glm::vec3& s);`
     - `glm::mat4 rotateX(float rad);` / `rotateY(float rad);` / `rotateZ(float rad);`
     - `glm::mat3 normalMatrix(const glm::mat4& model);` // 返回 transpose(inverse(mat3(model)))
     - `glm::vec3 srgbToLinear(uint32_t hexRGB);` // 如 0xF2EFE8 -> pow(c/255.0, 2.2)
     - `bool verifyAgainstGLM();` // 用随机参数对比手写矩阵与 glm 标准函数结果，误差 < 1e-5 返回 true

2. `src/math/MatrixStack.h`（纯头文件）
   - 显式维护 `std::vector<glm::mat4>` 栈，初始包含单位矩阵：
     - `void clear();`
     - `void push();`
     - `void push(const glm::mat4& m);` // push 并乘上 m
     - `void pop();`
     - `const glm::mat4& top() const;`
     - `void mul(const glm::mat4& m);` // top() = top() * m
     - `void translate(float x, float y, float z);`
     - `void rotate(float deg, const glm::vec3& axis);`
     - `void scale(float x, float y, float z);`

3. `src/math/BezierPatch.h` & `src/math/BezierPatch.cpp`
   - 双三次 Bézier 曲面片（4x4 控制点 `glm::vec3 cp[4][4]`）：
     - 手写三次 Bernstein 基函数 $B_{i,3}(t)$ 及其一阶导数 $B'_{i,3}(t)$。
     - `glm::vec3 eval(float u, float v, glm::vec3& outNormal) const;`
       计算曲面点 $P(u,v)$ 及偏导叉乘单位法线 $\text{normalize}(\partial P/\partial u \times \partial P/\partial v)$，若叉乘长度 $< 10^{-6}$ 则回退为 `(0,1,0)`。
     - `void tessellate(int steps, const glm::vec3& linearColor, MeshBuilder& builder, const glm::mat4& transform = glm::mat4(1.0f)) const;`
       按 `steps x steps` 均匀采样 $(u,v)\in[0,1]^2$，将顶点与索引追加到 `MeshBuilder`（契约见 `src/core/MeshBuilder.h`）。

4. `src/math/BSplinePath.h` & `src/math/BSplinePath.cpp`
   - Clamped 三次均匀 B 样条曲线（首尾控制点自动各重复 3 次，保证曲线严格经过起点和终点）：
     - `void setControlPoints(const std::vector<glm::vec3>& rawPoints, bool closedLoop = true);`
     - `glm::vec3 eval(float t) const;` // t 为段参数
     - `glm::vec3 tangent(float t) const;` // 单位切向量
     - 构建 1000 个采样点的弧长查找表（Arc-length LUT），提供匀速采样接口：
       `void evalByDistance(float distance, glm::vec3& outPos, glm::vec3& outTangent) const;`
       `float totalLength() const;`

5. `src/math/Frustum.h` & `src/math/Frustum.cpp`
   - 从 `proj * view` 矩阵提取 6 个归一化视锥平面 `glm::vec4 planes[6]`：
     - `void extract(const glm::mat4& vp);`
     - `bool intersectAABB(const glm::vec3& minPt, const glm::vec3& maxPt) const;`
```

---

### 📦 工作包 WP-2：2D 光栅化与裁剪沙盘系统（`src/ui/Raster2DBoard.h/.cpp`）

**复制以下提示词发给外部模型**：

```markdown
请按【全局严格约束】完成【工作包 WP-2：二维光栅化与裁剪沙盘模块】，输出 `src/ui/Raster2DBoard.h` 和 `src/ui/Raster2DBoard.cpp` 的完整源码。

【功能与接口要求】
1. 分辨率固定为 `128 x 128`，维护 CPU 内存像素缓冲区 `std::vector<uint8_t> m_pixels`（大小为 `128 * 128 * 4`，RGBA8 格式）。
2. 必须在类内纯手写以下 4 个经典图形学算法（严禁调用任何第三方绘图库）：
   - **全八分象限整数 Bresenham 画线算法**：`void drawLineBresenham(int x0, int y0, int x1, int y1, glm::u8vec4 color);`
   - **中点画圆算法（Midpoint Circle，八分对称）**：`void drawCircleMidpoint(int cx, int cy, int radius, glm::u8vec4 color);`
   - **有序边表扫描线多边形填充算法（ET + AET 活性边表，左闭右开规则避免顶点重复计数）**：
     `void fillPolygonScanline(const std::vector<glm::ivec2>& poly, glm::u8vec4 color);`
   - **Cohen-Sutherland 区域编码线段裁剪算法（4 位编码 TOP=8, BOTTOM=4, RIGHT=2, LEFT=1）**：
     `bool clipLineCohenSutherland(float& x0, float& y0, float& x1, float& y1, float xmin, float ymin, float xmax, float ymax) const;`
3. 沙盘绘制内容（在 `void rebuild()` 中依次执行）：
   - 背景清空为暖米色 `(242, 239, 232, 255)`。
   - 用 `fillPolygonScanline` 填充：2 块金色稻田多边形 `(217, 164, 65, 255)`、1 条弧形布尔哈通河道多边形 `(98, 168, 216, 255)`、3 座民居屋顶矩形 `(74, 85, 96, 255)`。
   - 用 `drawCircleMidpoint` 绘制：村中心广场外环（半径 22）、古水井内环（半径 6）、3 座风车标识圆（半径 4）。
   - 预设 6 条贯穿村落的主干道与田埂线段：
     - 若裁剪模式关闭（`!m_clipEnabled`），用深褐色 `drawLineBresenham` 完整画出所有道路。
     - 若裁剪模式开启（`m_clipEnabled`），先用浅灰色 `(175, 170, 162, 255)` 画出被剔除的原始全长道路，再用 `clipLineCohenSutherland` 对当前取景框 `[m_clipMin, m_clipMax]` 裁剪，将保留在框内的线段用醒目的亮红色 `(220, 42, 42, 255)` 覆盖绘制（加粗为 2 像素宽），并用亮绿色 `(30, 140, 76, 255)` 画出取景框四条边界。
4. 公有接口声明：
   ```cpp
   class Raster2DBoard {
   public:
       static constexpr int WIDTH = 128;
       static constexpr int HEIGHT = 128;

       Raster2DBoard();
       void setClipEnabled(bool enabled);
       bool isClipEnabled() const { return m_clipEnabled; }
       void moveClipWindow(int dx, int dy); // 移动取景框并自动限制在 [8, 120] 内，触发 rebuild()
       void rebuild();
       const uint8_t* data() const { return m_pixels.data(); }
       bool consumeDirty(); // 若自上次调用后发生过重绘则返回 true 并清零 dirty 标志
   };
   ```
```

---

### 📦 工作包 WP-3：朝鲜族歇山顶飞檐、传统民居与层级动画物体（`src/scene/`）

**复制以下提示词发给外部模型**：

```markdown
请按【全局严格约束】及全局接口契约（`Mesh.h`, `MeshBuilder.h`, `Shader.h`, `MatrixStack.h`, `BezierPatch.h`），完成【工作包 WP-3：朝鲜族建筑与层级建模模块】，完整输出以下 3 组共 6 个文件的源码：

1. `src/scene/BezierRoof.h` & `src/scene/BezierRoof.cpp`
   - 构建朝鲜族“鹤飞屋”歇山顶（四角微翘、屋脊两端上扬）：
   - 前坡和后坡各由**左右 2 片**双三次 Bézier 曲面拼接而成（共 4 片主坡曲面），左右两侧歇山短坡各 1 片（共 2 片）。
   - **核心要求（$C^1$ 连续）**：前坡左片 $L$ 与右片 $R$ 在中线拼缝处必须严格满足 $C^1$ 切平面连续：
     $$P^R_{i,0} = P^L_{i,3}, \qquad P^R_{i,1} = 2P^L_{i,3} - P^L_{i,2} \quad (i=0..3)$$
     请通过参数化函数 `(float width, float depth, float height, float cornerLift)` 自动生成控制点，中段平缓下凹、四角控制点向上向外起翘 `cornerLift`。
   - 加上歇山顶两侧三角形山花封板、正脊与四条垂脊装饰条（用 `MeshBuilder::addBox` 生成）。
   - 公有接口：
     ```cpp
     class BezierRoof {
     public:
         void build(float width, float depth, float height, float cornerLift, int tessLevel, MeshBuilder& outBuilder, const glm::mat4& localTransform = glm::mat4(1.0f)) const;
         // tessLevel: 0 -> 4x4, 1 -> 10x10, 2 -> 20x20
         static int stepsFromLevel(int level);
     };
     ```

2. `src/scene/KoreanHouse.h` & `src/scene/KoreanHouse.cpp`
   - 程序化组装朝鲜族传统民居与村口欢迎门楼：
   - **民居结构**（尺寸可通过结构体 `HouseSpec` 配置）：
     1. 石砌台基（`PAL_STONE_GREY`，高 0.55m，比墙体略宽）
     2. 暖白墙体（`PAL_WALL_WHITE`，高 2.6m）
     3. 深褐木柱、檐下横梁、正面木格栅推拉门与窗框格栅（`PAL_WOOD_DARK`，用细长 `addBox` 阵列拼出地道韩屋窗格）
     4. 顶部调用 `BezierRoof::build` 盖上青灰歇山顶（`PAL_ROOF_SLATE`）
     5. **文化特色细节——屋旁独立烟囱**：在房屋右侧 1.4m 处构建一座独立的砖石底座 + 竖直圆筒烟囱（与火炕相连），并计算导出烟囱顶部世界坐标 `glm::vec3 chimneyTopWorldPos`，供炊烟粒子系统作为发射源。
   - **门楼与院落小品**：
     - `buildWelcomeGate(MeshBuilder& outBuilder, glm::mat4& outPlaqueTransform)`：构建双柱歇山顶村口门楼，并返回中央横匾矩形的变换矩阵（用于贴 `sign_gate.png`）。
     - `buildStoneWell(MeshBuilder& outBuilder, const glm::vec3& pos)`：构建八角/圆形石砌老水井 + 木架顶棚。
     - `buildSandboxTable(MeshBuilder& outBuilder, const glm::vec3& pos, glm::mat4& outBoardTopTransform)`：构建村口 2D 导览沙盘木台，并返回台面矩形变换矩阵。
   - 公有接口：
     ```cpp
     struct HouseInstance {
         glm::vec3 pos;
         float yawDeg;
         glm::vec3 scale;
         bool hasChimney;
     };
     class KoreanVillageBuilder {
     public:
         // 根据当前 bezierLevel 重新生成整个村落静态建筑合批网格（不含门楼透明牌匾面片）
         static void buildAllHouses(const std::vector<HouseInstance>& houses, int bezierLevel,
                                    Mesh& outVillageMesh, std::vector<glm::vec3>& outChimneyTops);
         static void buildWelcomeGate(int bezierLevel, Mesh& outGateMesh, Mesh& outPlaqueQuad, glm::mat4& outPlaqueModel);
         static void buildProps(Mesh& outPropsMesh, Mesh& outSandboxTopQuad, glm::mat4& outSandboxTopModel,
                                Mesh& outGuideSignQuad, glm::mat4& outGuideSignModel);
     };
     ```

3. `src/scene/HierarchyObjects.h` & `src/scene/HierarchyObjects.cpp`
   - 使用 `MatrixStack` 实现全部层级动画物体：
     1. **现代风力发电机（3 座）**：三级父子矩阵栈 `塔架 (Tower) -> 机舱偏航 (Nacelle Yaw) -> 轮毂旋转 (Hub Roll) -> 3 片扇叶 (Blades, 间隔 120°)`。
     2. **民俗秋千架与荡秋千少女简模**：`A 字木架横梁 -> 双绳 + 秋千踏板 (绕横梁轴简谐摆动 θ(t) = θ_max * sin(ωt))`。
     3. **民俗跳板（Seol-neol-ttwi-gi）**：`中心垫木支点 -> 长木板绕支点往复翘动 + 两端简化小人上下起落`。
     4. **追日太阳能光伏板阵列（6 组）**：`立柱 -> 光伏板面（俯仰角 pitch 实时朝向当前太阳高度角 u_SunDir）`。
     5. **沿 B 样条轨迹行驶的观光游览小车**：`车身沿 BSplinePath 切线方向移动 -> 4 个车轮作为子节点绕局部 X 轴滚动`。
   - 公有接口：
     ```cpp
     class HierarchyObjects {
     public:
         void init();
         void update(float dt, const glm::vec3& sunDir, const class BSplinePath* tourPath);
         // 传入 shader，内部设置 u_Model 和 u_NormalMat 并绘制各个部件（可用于主 Pass 和平面阴影 Pass）
         void drawAll(const Shader& shader, bool shadowPassOnly = false) const;
     };
     ```
```

---

### 📦 工作包 WP-4：地形、水波、实例化稻田、程序化天空盒、光影状态与粒子（`src/scene/` & `src/fx/`）

**复制以下提示词发给外部模型**：

```markdown
请按【全局严格约束】完成【工作包 WP-4：自然环境与特效系统】，完整输出以下 6 组文件源码：

1. `src/scene/Terrain.h` & `src/scene/Terrain.cpp`
   - 生成覆盖 `[-200, 200] x [-200, 200]`、分辨率 `200 x 200` 的高度场网格：
   - 高度函数：
     - 村落核心区 $r < 35$：平坦地面 $y = 0$，但在布尔哈通河湾弧段（$r \in [25, 33], \theta \in [200^\circ, 330^\circ]$）平滑下凹至 $y = -0.8\text{ m}$ 形成河床。
     - 振兴产业带 $r \in [35, 90]$：微起伏田野丘陵（振幅 $\le 1.2\text{ m}$）。
     - 远景长白山生态圈 $r \in [90, 200]$：`smoothstep(85, 160, r) * 28.0 * fbmNoise(x, z)` 形成环绕群山。
   - 用中心差分计算每个顶点的平滑法向量；顶点色根据高度、河岸与半径平滑过渡。
   - 接口：`void init(); void draw(const Shader& shader) const; float getHeight(float x, float z) const;`

2. `src/scene/Water.h` & `src/scene/Water.cpp`
   - 在布尔哈通河湾位置生成极坐标扇环细分网格（$r \in [24.5, 33.5], \theta \in [198^\circ, 332^\circ]$，网格密度 `80 x 16`，水面基准高度 $y = -0.18\text{ m}$）。
   - 接口：`void init(); void draw(const Shader& shader) const;`

3. `src/scene/RiceField.h` & `src/scene/RiceField.cpp`
   - 在中景环带 $r \in [38, 82]$（避开道路与风机）划分 48 个稻田地块（Chunk），每个地块包含 `8 x 8 = 64` 丛低多边形稻穗网格实例（总计约 3072 丛），并记录每个地块的 AABB 包围盒。
   - 每帧调用 `updateVisibleInstances(const Frustum& frustum)`，用视锥体剔除不可见地块，将可见地块的 `glm::mat4` 实例矩阵汇总上传到 `Mesh::setInstances`，并统计 `int visibleChunks() const` 与 `int totalChunks() const`。
   - 接口：`void init(const Terrain& terrain); void updateVisibleInstances(const Frustum& frustum); void draw(const Shader& shader) const; int visibleChunks() const; int totalChunks() const;`

4. `src/fx/LightingState.h` & `src/fx/LightingState.cpp`
   - 管理三态光影预设（`0: 晨曦初露`, `1: 正午晴空`, `2: 星夜灯火`）与平滑过渡（1.2 秒 `smoothstep` 插值）：
     - 太阳/月光方向 `sunDir`、平行光色 `sunColor`、环境光 `ambient`、雾色 `fogColor`、雾密度 `fogDensity`、天空盒混合权重、阴影开关 `shadowEnabled`、点光源强度系数 `pointLightIntensity`。
   - 管理 6 盏点光源位置（村口门楼 2 盏 + 环村路灯 4 盏），并提供路灯柱网格绘制接口。
   - 管理季节状态（`0: 金秋`, `1: 初冬瑞雪`），平滑插值 `snowAmount` $\in [0, 1]$。
   - 接口：`void setPreset(int index); void toggleSeason(); void update(float dt); void applyToShader(const Shader& shader) const;`

5. `src/fx/SkyGenerator.h` & `src/fx/SkyGenerator.cpp`
   - 在 CPU 端程序化生成 3 套 Cubemap（晨曦 `dawn`、正午 `noon`、星夜 `night`，每面 `256 x 256` RGB8）：
     - 包含天顶到地平线大气散射渐变、太阳/月亮圆盘与柔光晕、星夜随机闪烁星点（基于方向向量哈希）、远景长白山主峰与群山剪影（峰顶带积雪色）。
   - 接口：`static unsigned int createProceduralCubemap(int presetType);`

6. `src/fx/PlanarShadow.h` & `src/fx/PlanarShadow.cpp` & `src/fx/ParticleSystem.h` & `src/fx/ParticleSystem.cpp`
   - `PlanarShadow::buildShadowMatrix(const glm::vec4& groundPlane, const glm::vec4& lightDir)`：严格按公式 $M_{ij} = (\mathbf{n}\cdot\mathbf{L})\delta_{ij} - L_i n_j$ 构造 $4\times 4$ 平面投影矩阵。
   - `ParticleSystem`：支持两种模式——`0: 袅袅炊烟`（从传入的民居独立烟囱坐标发射，缓慢上升、水平正弦漂移、尺寸变大、Alpha 衰减）与 `1: 延边瑞雪`（在相机周围空间循环飘落）；每帧按到相机距离由远及近排序，使用十字交叉广告牌网格绘制。
```

---

### 📦 工作包 WP-5：全套 GLSL 3.3 着色器（`shaders/`）

**复制以下提示词发给外部模型**：

```markdown
请按【全局严格约束】完成【工作包 WP-5：GLSL 3.3 着色器套件】，完整输出以下 8 个着色器文件源码（使用 `#version 330 core`）：

1. `shaders/uber_scene.vert` & `shaders/uber_scene.frag`
   - 顶点输入：`layout(location=0) in vec3 a_Pos; layout(location=1) in vec3 a_Normal; layout(location=2) in vec2 a_UV; layout(location=3) in vec3 a_Color; layout(location=4) in mat4 a_InstanceModel;`（占 location 4..7）。
   - Uniform 变量：
     - 矩阵：`u_Model`, `u_NormalMat`, `u_View`, `u_Proj`, `u_ShadowProjMat`
     - 模式开关：`u_Instanced` (bool), `u_AnimType` (0:无, 1:水波正弦偏移并解析重算法线, 2:稻浪随风摆动), `u_ShadeMode` (0:Flat, 1:Gouraud, 2:Blinn-Phong), `u_IsShadowPass` (bool), `u_UseTexture` (bool), `u_UseEnvMap` (bool)
     - 光照与季节：`u_SunDir`, `u_SunColor`, `u_Ambient`, `u_CamPos`, `u_Shininess`, `u_PointLightPos[6]`, `u_PointLightColor[6]`, `u_NumPointLights`, `u_PointLightFactor`, `u_SnowAmount`, `u_Time`
     - 雾与阴影：`u_FogColor`, `u_FogDensity`, `u_ShadowColor`
     - 采样器：`u_AlbedoTex` (unit 0), `u_SkyTexA` (unit 1), `u_SkyTexB` (unit 2), `u_SkyBlend` (float)
   - 核心逻辑：
     - 若 `u_IsShadowPass == true`：顶点乘 `u_Proj * u_View * u_ShadowProjMat * worldPos`；片元若 `length(v_WorldPos.xz) > 36.0` 则 `discard`，否则直接输出半透明冷色阴影 `u_ShadowColor`。
     - 三种明暗模式对比必须肉眼极其明显：
       - `u_ShadeMode == 0` (Flat)：在片元中使用 `normalize(cross(dFdx(v_WorldPos), dFdy(v_WorldPos)))` 计算三角面片几何法线并计算光照，呈现鲜明的棱面晶格感。
       - `u_ShadeMode == 1` (Gouraud)：在顶点着色器中计算 Blinn-Phong 漫反射与高光，片元直接使用插值结果。
       - `u_ShadeMode == 2` (Phong)：在片元着色器中对插值法线归一化后逐片元计算 Blinn-Phong 与 6 盏点光源二次距离衰减 $1 / (1.0 + 0.14d + 0.07d^2)$。
     - 当 `u_Season / u_SnowAmount > 0` 时：对朝上的表面（`N.y > 0.5`）叠加初冬积雪白，并将稻田金色平滑插值为收割后的浅褐茬地色。
     - 当 `u_UseEnvMap == true`（水面）：用 `reflect(-V, N)` 采样混合后的天空盒 Cubemap，并按 Schlick Fresnel 公式与水体底色混合。
     - 末尾执行指数距离高度雾混合与 Gamma 校正 `pow(color, vec3(1.0/2.2))`。

2. `shaders/skybox.vert` & `shaders/skybox.frag`
   - 移除视图矩阵位移，输出 `gl_Position = pos.xyww`；片元按 `u_SkyBlend` 混合 `u_SkyTexA` 与 `u_SkyTexB` 并做 Gamma 校正。

3. `shaders/particle.vert` & `shaders/particle.frag`
   - 渲染半透明炊烟与雪花粒子，带软边圆形衰减与 Alpha 混合。

4. `shaders/hud.vert` & `shaders/hud.frag`
   - 在屏幕指定 NDC 矩形内绘制 2D 纹理（用于右上角放大显示 2D 光栅化沙盘及中央显示 `help.png`）；当 `u_ShowPixelGrid == true` 时，在 128×128 像素边界叠加细网格线，清晰展示每个光栅化像素格。
```

---

### 📦 工作包 WP-6：3000 字《计算机图形学课程设计报告书》完整终稿（`docs/Course_Report.md`）

**复制以下提示词发给外部模型**：

```markdown
请根据我们的项目方案，撰写一份结构严谨、公式推导完整、可直接导出 PDF 提交的 **3500 字《计算机图形学课程设计报告书》**（Markdown + LaTeX 公式格式）。

【章节与内容硬性要求】
1. **第 1 章 绪论与设计立意（约 500 字）**
   - 阐述「延边朝鲜族民俗村·乡村振兴」主题融合：人文核心（歇山式“鹤飞屋”、屋旁独立火炕烟囱、民俗秋千跳板、中朝双语门楼）、乡村振兴（金色稻田、现代风电与追日光伏）、旅游导览（布尔哈通河生态湾、B 样条自动巡游、2D 导览沙盘）。
   - 插入空间布局图引用 `![三层同心圆空间规划图](images/diagram_scene_layout.png)` 与概念图引用 `![晨曦主视觉](images/concept_dawn_village.jpg)`。
2. **第 2 章 系统总体架构与五阶段渲染管线（约 600 字）**
   - 给出工程模块架构表、课程章节知识点映射表、Uber-Shader 五阶段单帧渲染管线流程图（Mermaid 语法），重点说明开启/关闭深度测试时天空盒的渲染顺序保护机制。
3. **第 3 章 核心图形学算法数学推导与实现（约 1800 字，核心得分点）**
   - **3.1 二维图元光栅化与裁剪沙盘**：写出 Bresenham 决策参数 $p_k$ 递推公式、中点画圆判别式推导、扫描线多边形填充（ET 与 AET 活性边表、左闭右开防重算规则）、Cohen-Sutherland 4 位区域编码位运算判别与求交公式，引用 `![2D沙盘光栅化与裁剪](images/diagram_raster_clip_demo.png)`。
   - **3.2 朝鲜族飞檐的多片双三次 Bézier 曲面与 $C^1$ 拼接**：写出三次 Bernstein 基函数、曲面方程 $P(u,v) = \sum_{i=0}^3\sum_{j=0}^3 B_{i,3}(u)B_{j,3}(v)P_{i,j}$、偏导叉乘求法线公式，以及左右坡曲面片 $C^1$ 切平面连续的共线等距控制点约束 $P^R_{i,1} = 2P^L_{i,3} - P^L_{i,2}$，引用 `![Bézier飞檐与C1连续](images/diagram_bezier_c1.png)`。
   - **3.3 层级矩阵栈建模与 Clamped 三次 B 样条自动导览**：推导齐次变换 $M = T\cdot R\cdot S$、法线逆转置矩阵 $N=(M^{-1})^T$、风车三级父子变换链 $M_{\text{blade}} = M_{\text{tower}}\cdot M_{\text{nacelle}}(\psi)\cdot M_{\text{hub}}(\phi)\cdot R_z(120^\circ k)$；写出三次均匀 B 样条基函数矩阵与弧长参数化匀速插值方法。
   - **3.4 多光源 Blinn-Phong、三种明暗处理、环境反射与平面投影阴影**：写出 Blinn-Phong 方程、点光源二次衰减公式、Flat（屏幕空间偏导 `dFdx/dFdy` 求面法线）/ Gouraud / Phong 的本质差异、水面 Fresnel 环境反射方程，以及从平面方程 $\mathbf{n}\cdot\mathbf{X}+d=0$ 和平行光方向 $\mathbf{L}$ 推导 $4\times 4$ 平面阴影矩阵 $M_{\text{shadow}} = (\mathbf{n}\cdot\mathbf{L})I - \mathbf{L}\mathbf{n}^T$ 的完整过程与 Stencil 防重叠加深机制。
4. **第 4 章 系统交互功能与多模式对比分析（约 400 字）**
   - 列出全部交互快捷键表，并针对“线框 vs 实体”、“Bézier 细分 4×4 vs 20×20”、“Flat vs Gouraud vs Phong”、“晨曦 vs 正午 vs 星夜”、“开启 vs 关闭 Z-Buffer”五组对比实验撰写专业图像学分析。
5. **第 5 章 开发难点排查与课程总结（约 300 字）**
   - 详述 4 个典型图形学工程 Bug 的排查过程：① 矩阵乘法顺序 `P*V*M` 与列主序内存布局；② 平面阴影与地面 Z-fighting（高度偏移 $y=0.01$ + PolygonOffset + Stencil 缓冲）；③ 多片 Bézier 屋顶拼接处 Phong 高光断裂（通过 $C^1$ 控制点共线约束解决）；④ 关闭 `GL_DEPTH_TEST` 时天空盒覆盖全屏（动态前置天空盒 Pass 解决）。
```

---

## 五、同步与回传方式

1. 你可以把上面的 **通用系统提示词 + WP-1 ~ WP-6** 分别发给另一个模型（可以开多个对话同时跑，速度最快）。
2. 外部模型生成好后：
   - 你既可以**直接把它的回复粘贴发给我**；
   - 也可以把生成的内容存放在桌面或 `d:/Antigravity_work/YanbianVillage/` 下，然后对我说一声**“我把 WP-1 ~ WP-5 放进去了，你来整合编译”**。
3. 同时，我现在可以在这边同步为你搭好 **`CMakeLists.txt` / `Makefile`、第三方依赖（glad / stb）和 `src/core/` 主框架**，你那边的代码一回来就能直接编译出可执行程序！
