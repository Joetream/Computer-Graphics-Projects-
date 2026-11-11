# 工作包 WP-3：朝鲜族歇山顶飞檐、传统民居与矩阵栈层级动画

> **给 Agent 的执行指令**：
> 你正在参与开发 `D:/Antigravity_work/YanbianVillage`（C++17 + OpenGL 3.3 Core Profile）。
> 本工作包（WP-3）的头文件 `src/scene/BezierRoof.h`、`src/scene/KoreanHouse.h`、`src/scene/HierarchyObjects.h` 已存在于仓库中，**请勿修改任何 `.h` 头文件**。
> 请严格按照下方给定的头文件声明，编写并直接写入（或完整输出）以下 **3 个 `.cpp` 实现文件**的完整源码，严禁使用 `// TODO` 或省略任何代码：
> 1. `src/scene/BezierRoof.cpp`
> 2. `src/scene/KoreanHouse.cpp`
> 3. `src/scene/HierarchyObjects.cpp`

---

## 一、已锁定的头文件契约与调色板（请勿改动 `.h`）

### 1. 统一调色板（使用 `MathUtil::srgbToLinear(0xRRGGBB)` 转为线性颜色）
- `0xF2EFE8`：朝鲜族民居暖白灰墙
- `0x4A5560`：歇山顶青灰筒瓦
- `0x5B3A29`：深褐木梁柱、门窗格栅、门楼木架
- `0x9A958C`：石砌台基、老水井
- `0xC85A48`：独立火炕烟囱红褐砖石
- `0xE07AA0`：金达莱粉紫点缀色（秋千/跳板装饰）
- `0xE6E9EC`：现代风力发电机纯白机身
- `0x2B3A55`：太阳能光伏板深蓝硅片

### 2. `src/scene/BezierRoof.h`
```cpp
#pragma once
#include "core/MeshBuilder.h"
#include <glm/glm.hpp>

class BezierRoof {
public:
    void build(float width, float depth, float height, float cornerLift, int tessLevel,
               MeshBuilder& outBuilder, const glm::mat4& localTransform = glm::mat4(1.0f)) const;
    static int stepsFromLevel(int level);
};
```

### 3. `src/scene/KoreanHouse.h`
```cpp
#pragma once
#include "core/Mesh.h"
#include <glm/glm.hpp>
#include <vector>

struct HouseInstance {
    glm::vec3 pos;
    float yawDeg;
    glm::vec3 scale;
    bool hasChimney;
};

class KoreanVillageBuilder {
public:
    static void buildAllHouses(const std::vector<HouseInstance>& houses, int bezierLevel,
                               Mesh& outVillageMesh, std::vector<glm::vec3>& outChimneyTops);
    static void buildWelcomeGate(int bezierLevel, Mesh& outGateMesh, Mesh& outPlaqueQuad, glm::mat4& outPlaqueModel);
    static void buildProps(Mesh& outPropsMesh, Mesh& outSandboxTopQuad, glm::mat4& outSandboxTopModel,
                           Mesh& outGuideSignQuad, glm::mat4& outGuideSignModel);
};
```

### 4. `src/scene/HierarchyObjects.h`
```cpp
#pragma once
#include "core/Mesh.h"
#include "core/Shader.h"
#include <glm/glm.hpp>

class BSplinePath;

class HierarchyObjects {
public:
    void init();
    void update(float dt, const glm::vec3& sunDir, const BSplinePath* tourPath);
    void drawAll(const Shader& shader, bool shadowPassOnly = false) const;

private:
    Mesh m_towerMesh;
    Mesh m_nacelleMesh;
    Mesh m_hubMesh;
    Mesh m_bladeMesh;

    Mesh m_swingFrameMesh;
    Mesh m_swingSeatMesh;

    Mesh m_seesawBaseMesh;
    Mesh m_seesawPlankMesh;

    Mesh m_solarPoleMesh;
    Mesh m_solarPanelMesh;

    Mesh m_cartBodyMesh;
    Mesh m_cartWheelMesh;

    float m_rotorAngle = 0.0f;
    float m_nacelleYaw = 0.0f;
    float m_swingTime = 0.0f;
    float m_solarPitch = 35.0f;
    float m_cartDist = 15.0f;
    float m_wheelRoll = 0.0f;
    glm::vec3 m_cartPos{0.0f, 0.0f, 32.0f};
    glm::vec3 m_cartDir{1.0f, 0.0f, 0.0f};
};
```

---

## 二、需要你编写的 3 个 `.cpp` 文件详细规范

### 1. `src/scene/BezierRoof.cpp`
- `stepsFromLevel(int level)`：`level == 0` 返回 `4`，`level == 1` 返回 `10`，其余返回 `20`。
- `build(width, depth, height, cornerLift, tessLevel, outBuilder, localTransform)`：
  - 构建朝鲜族“鹤飞屋”歇山顶（四角飞檐微翘、屋脊两端上扬）：
  - 前坡与后坡各拆分为**左片 $L$ ($x \in [-W/2, 0]$) 与右片 $R$ ($x \in [0, W/2]$)** 两个 `BezierPatch`（共 4 片主坡曲面），外加左右两侧歇山三角/梯形翼坡 2 片 `BezierPatch`（总计 6 片双三次 Bézier 曲面）。
  - **严格实施 $C^1$ 连续条件**：在 $x=0$ 的中线拼接处，左片控制点 $P^L_{i,3}$ 与右片控制点 $P^R_{i,0}$ 重合且水平切线连续：
    $$P^R_{i,0} = P^L_{i,3}, \qquad P^R_{i,1} = 2P^L_{i,3} - P^L_{i,2} \quad (i = 0..3)$$
    檐口四角控制点向上抬升 `cornerLift` 并向外微挑，形成优雅的“鹤翼飞檐”曲线。
  - 调用 `BezierPatch::tessellate` 将曲面追加到 `outBuilder`；同时用 `addBox` / `addTriangle` 补齐歇山顶山花板、正脊横梁及两端微翘吻兽装饰。

### 2. `src/scene/KoreanHouse.cpp`
- `buildAllHouses`：
  - 遍历 `houses`，对每栋朝鲜族民居生成：
    1. **石砌台基**（`0x9A958C`，宽 $7.6\text{ m}$、深 $5.2\text{ m}$、高 $0.55\text{ m}$）
    2. **暖白墙体**（`0xF2EFE8`，宽 $6.8\text{ m}$、深 $4.4\text{ m}$、高 $2.6\text{ m}$）
    3. **深褐木构细节**（`0x5B3A29`）：四角立柱、檐下横梁、正面韩式木格栅推拉门与两侧木格栅窗（用细长 `addBox` 拼出精致横竖窗棂）
    4. **青灰歇山顶**（调用 `BezierRoof::build`，宽 $8.4\text{ m}$、深 $6.0\text{ m}$、高 $2.1\text{ m}$、起翘 $0.55\text{ m}$）
    5. **文化标志——屋旁独立烟囱**：若 `hasChimney == true`，在房屋右侧偏后方 $(+4.5\text{ m}, 0, -1.2\text{ m})$ 处建造与地下火炕相连的独立砖石烟囱（方形底座 + 高 $3.8\text{ m}$ 圆筒烟囱 + 遮雨小帽），并将烟囱顶端世界坐标 push 到 `outChimneyTops`。
  - 最终合并上传至 `outVillageMesh`。
- `buildWelcomeGate`：
  - 在村口 $(0, 0, 32.0\text{ m})$ 处构建宏伟的双柱四阿/歇山顶迎宾门楼（高 $5.8\text{ m}$，跨度 $8.5\text{ m}$）上传至 `outGateMesh`。
  - 生成门楼正中央竖直悬挂的横匾矩形网格 `outPlaqueQuad`（宽 $4.8\text{ m}$、高 $1.2\text{ m}$，法线朝 $+Z$，UV 对应整张 `sign_gate.png`）及其世界变换矩阵 `outPlaqueModel`。
- `buildProps`：
  - 在村落广场与庭院内构建：
    1. **百年石砌老水井**（位于 `(3.0f, 0.0f, 0.0f)`：八角/圆柱石井圈 + 双木柱小歇山井亭 + 辘轳横轴）；
    2. **2D 村落沙盘导览木台**（位于 `(-7.5f, 0.0f, 18.0f)`），并输出台面水平 Quad `outSandboxTopQuad` 及 `outSandboxTopModel`（用于贴 `Raster2DBoard` 动态纹理）；
    3. **村口旅游导览立牌木框**（位于 `(7.5f, 0.0f, 22.0f)`），并输出立牌正面竖直 Quad `outGuideSignQuad` 及 `outGuideSignModel`（用于贴 `sign_guide.png`）。

### 3. `src/scene/HierarchyObjects.cpp`
- 使用 `MatrixStack`（`#include "math/MatrixStack.h"`）管理全部父子层级变换，每次绘制部件时设置 `shader.setMat4("u_Model", stack.top())` 和 `shader.setMat3("u_NormalMat", MathUtil::normalMatrix(stack.top()))`：
  1. **3 座现代风力发电机**（位于 `(-58, 0, -42)`, `(0, 0, -68)`, `(60, 0, -40)`，塔高 $18\text{ m}$）：
     - `Tower`（圆台塔架）$\rightarrow$ `Nacelle`（机舱缓慢偏航 $\psi = 12^\circ\sin(0.25t)$）$\rightarrow$ `Hub`（轮毂旋转 $\phi$）$\rightarrow$ 3 片细长 `Blade`（间隔 $120^\circ$）。
  2. **民俗高秋千架**（位于 `(16, 0, 5)`，高 $5.2\text{ m}$）：
     - A 字形双木立柱与横梁 $\rightarrow$ 双悬绳与踏板 + 简化朝鲜族裙装小人绕横梁做简谐摆动 $\theta(t) = 32^\circ\sin(2.1t)$。
  3. **民俗跳板**（位于 `(21, 0, 9)`）：
     - 中心支点木墩 $\rightarrow$ 长木板绕 Z 轴翘动 $\alpha(t) = 14^\circ\sin(3.2t)$，两端小人随跳板交替弹起。
  4. **6 组追日太阳能光伏板阵列**（位于 `x = 38..68, z = 18..26`）：
     - 金属立柱 $\rightarrow$ 蓝色光伏板面绕 X 轴实时朝向当前太阳仰角 `m_solarPitch`。
  5. **沿 B 样条轨道行驶的观光游览车**：
     - 车身沿 `tourPath` 匀速前进并朝向切线方向 $\rightarrow$ 4 个车轮作为子节点绕局部轴高速旋转 `m_wheelRoll`。
