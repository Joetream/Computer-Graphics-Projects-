# 工作包 WP-1：纯手写数学库、双三次 Bézier 曲面、B 样条与视锥体

> **给 Agent 的执行指令**：
> 你正在参与开发 `D:/Antigravity_work/YanbianVillage`（C++17 + OpenGL 3.3 Core Profile）。
> 本工作包（WP-1）的所有 `.h` 头文件已经存在于仓库的 `src/math/` 和 `src/core/` 下，**请勿修改任何 `.h` 头文件**。
> 请严格按照下方给定的头文件声明，编写并直接写入（或完整输出）以下 **4 个 `.cpp` 实现文件**的完整源码，严禁使用 `// TODO` 或省略任何代码：
> 1. `src/math/MathUtil.cpp`
> 2. `src/math/BezierPatch.cpp`
> 3. `src/math/BSplinePath.cpp`
> 4. `src/math/Frustum.cpp`

---

## 一、已锁定的头文件契约（只读参考，请勿改动）

### 1. `src/core/Mesh.h` & `src/core/MeshBuilder.h`
```cpp
struct Vertex {
    glm::vec3 pos;     // location = 0
    glm::vec3 normal;  // location = 1
    glm::vec2 uv;      // location = 2
    glm::vec3 color;   // location = 3: 线性空间顶点色
};

class MeshBuilder {
public:
    std::vector<Vertex>   verts;
    std::vector<uint32_t> indices;
    void clear();
    void addBox(const glm::mat4& M, const glm::vec3& linearColor);
    void addCylinder(const glm::mat4& M, float r0, float r1, float h, int seg, const glm::vec3& linearColor);
    void addQuad(const glm::mat4& M, const glm::vec3& linearColor);
    void addGrid(float w, float d, int nx, int nz, const glm::vec3& linearColor);
    void addTriangle(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2,
                     const glm::vec3& linearColor, const glm::mat4& M = glm::mat4(1.0f));
    void append(const MeshBuilder& other, const glm::mat4& M = glm::mat4(1.0f));
    void build(Mesh& outMesh) const;
};
```

### 2. `src/math/MathUtil.h`
```cpp
#pragma once
#include <glm/glm.hpp>
#include <cstdint>

namespace MathUtil {
    glm::mat4 lookAt(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up);
    glm::mat4 perspective(float fovyRad, float aspect, float zNear, float zFar);
    glm::mat4 ortho(float left, float right, float bottom, float top, float zNear, float zFar);
    glm::mat4 translate(const glm::vec3& t);
    glm::mat4 scale(const glm::vec3& s);
    glm::mat4 rotateX(float rad);
    glm::mat4 rotateY(float rad);
    glm::mat4 rotateZ(float rad);
    glm::mat3 normalMatrix(const glm::mat4& model);
    glm::vec3 srgbToLinear(uint32_t hexRGB);
    bool verifyAgainstGLM();
}
```

### 3. `src/math/BezierPatch.h`
```cpp
#pragma once
#include <glm/glm.hpp>
#include "core/MeshBuilder.h"

class BezierPatch {
public:
    glm::vec3 cp[4][4];

    glm::vec3 eval(float u, float v, glm::vec3& outNormal) const;
    void tessellate(int steps, const glm::vec3& linearColor, MeshBuilder& builder,
                    const glm::mat4& transform = glm::mat4(1.0f)) const;
};
```

### 4. `src/math/BSplinePath.h`
```cpp
#pragma once
#include <vector>
#include <glm/glm.hpp>

class BSplinePath {
public:
    void setControlPoints(const std::vector<glm::vec3>& rawPoints, bool closedLoop = true);
    glm::vec3 eval(float t) const;
    glm::vec3 tangent(float t) const;
    void evalByDistance(float distance, glm::vec3& outPos, glm::vec3& outTangent) const;
    float totalLength() const { return m_totalLength; }
    int segmentCount() const { return static_cast<int>(m_ctrl.size()) - 3; }

private:
    std::vector<glm::vec3> m_ctrl;
    struct ArcSample { float t; float dist; };
    std::vector<ArcSample> m_lut;
    float m_totalLength = 0.0f;
};
```

### 5. `src/math/Frustum.h`
```cpp
#pragma once
#include <glm/glm.hpp>

class Frustum {
public:
    glm::vec4 planes[6];

    void extract(const glm::mat4& vp);
    bool intersectAABB(const glm::vec3& minPt, const glm::vec3& maxPt) const;
};
```

---

## 二、需要你编写的 4 个 `.cpp` 文件详细规范

### 1. `src/math/MathUtil.cpp`
- **禁止直接调用** `glm::lookAt`、`glm::perspective`、`glm::ortho`，必须按照计算机图形学教材公式手工计算各分量并填入列主序 `glm::mat4 M`（注意 GLM 下标 `M[col][row]`）：
  - `lookAt(eye, center, up)`：计算前向 `f = normalize(center - eye)`，右向 `s = normalize(cross(f, up))`，上向 `u = cross(s, f)`，填入旋转与平移分量。
  - `perspective(fovyRad, aspect, zNear, zFar)`：$t = 1/\tan(\text{fovyRad}/2)$，填入透视投影矩阵（OpenGL NDC $z\in[-1,1]$）。
  - `ortho(l, r, b, t, n, f)`：填入正交投影矩阵。
  - `translate` / `scale` / `rotateX` / `rotateY` / `rotateZ`：手工构造 $4\times 4$ 齐次变换矩阵。
  - `normalMatrix(model)`：返回 `glm::transpose(glm::inverse(glm::mat3(model)))`。
  - `srgbToLinear(hexRGB)`：提取 `(r,g,b)` 字节除以 `255.0f`，对每个通道做 `std::pow(c, 2.2f)`。
  - `verifyAgainstGLM()`：构造一组测试向量与角度，对比手写 `lookAt / perspective / ortho` 与 `glm::lookAt / glm::perspective / glm::ortho` 的最大绝对误差，若 $< 10^{-5}$ 返回 `true`。

### 2. `src/math/BezierPatch.cpp`
- 实现三次 Bernstein 基函数及其导数：
  $$B_{0,3}(t)=(1-t)^3,\quad B_{1,3}(t)=3t(1-t)^2,\quad B_{2,3}(t)=3t^2(1-t),\quad B_{3,3}(t)=t^3$$
  $$B'_{0,3}(t)=-3(1-t)^2,\quad B'_{1,3}(t)=3(1-t)^2-6t(1-t),\quad B'_{2,3}(t)=6t(1-t)-3t^2,\quad B'_{3,3}(t)=3t^2$$
- `eval(u, v, outNormal)`：
  计算 $P(u,v) = \sum_{i=0}^3\sum_{j=0}^3 B_{i,3}(u)B_{j,3}(v)cp_{i,j}$ 以及偏导 $\partial P/\partial u, \partial P/\partial v$，法线取 $\text{normalize}(\partial P/\partial u \times \partial P/\partial v)$（若叉乘模长 $< 10^{-6}$，回退为 `(0,1,0)`；若法线 $y < 0$ 则取反保证朝上）。
- `tessellate(steps, linearColor, builder, transform)`：
  将 $(u,v)\in[0,1]^2$ 均匀划分为 `steps * steps` 个小四边形网格（`(steps+1)*(steps+1)` 个顶点），用 `transform` 变换顶点位置、用 `MathUtil::normalMatrix(transform)` 变换法线，追加到 `builder.verts` 和 `builder.indices`（每个小格 2 个三角形，双面渲染或保证法线朝外）。

### 3. `src/math/BSplinePath.cpp`
- `setControlPoints(rawPoints, closedLoop)`：
  - 若 `closedLoop == true`：将输入点序列首尾平滑闭合（追加前 3 个控制点），或构造 Clamped 端点；
  - 若 `closedLoop == false`：首尾控制点各重复 3 次（Clamped 三次均匀 B 样条）。
  - 预采样 1000 个参数点建立弧长查找表 `m_lut` 并计算总长度 `m_totalLength`。
- `eval(t)` 与 `tangent(t)`：
  使用三次均匀 B 样条基函数：
  $$b_0(s)=\frac{(1-s)^3}{6},\quad b_1(s)=\frac{3s^3-6s^2+4}{6},\quad b_2(s)=\frac{-3s^3+3s^2+3s+1}{6},\quad b_3(s)=\frac{s^3}{6}$$
  及其一阶导数计算曲线坐标与单位切向量。
- `evalByDistance(distance, outPos, outTangent)`：在 `m_lut` 中二分查找 `distance` 对应的参数 $t$，调用 `eval(t)` 和 `tangent(t)`。

### 4. `src/math/Frustum.cpp`
- `extract(vp)`：从列主序矩阵 `vp = proj * view` 的行向量 $r_0, r_1, r_2, r_3$ 提取左、右、下、上、近、远 6 个平面方程并归一化法向量。
- `intersectAABB(minPt, maxPt)`：对 6 个平面分别取 AABB 的正顶点（p-vertex），若在任一平面外侧则返回 `false`，否则返回 `true`。
