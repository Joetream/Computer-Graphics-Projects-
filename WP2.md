# 工作包 WP-2：2D 光栅化与裁剪沙盘系统

> **给 Agent 的执行指令**：
> 你正在参与开发 `D:/Antigravity_work/YanbianVillage`（C++17 + OpenGL 3.3 Core Profile）。
> 本工作包（WP-2）的头文件 `src/ui/Raster2DBoard.h` 已经存在于仓库中，**请勿修改 `src/ui/Raster2DBoard.h`**。
> 请严格按照下方给定的头文件声明，编写并直接写入（或完整输出）**`src/ui/Raster2DBoard.cpp`** 的完整源码，严禁使用 `// TODO` 或省略任何代码。

---

## 一、已锁定的头文件契约（`src/ui/Raster2DBoard.h`，请勿改动）

```cpp
#pragma once
#include <vector>
#include <cstdint>
#include <glm/glm.hpp>

class Raster2DBoard {
public:
    static constexpr int WIDTH = 128;
    static constexpr int HEIGHT = 128;

    Raster2DBoard();
    void setClipEnabled(bool enabled);
    bool isClipEnabled() const { return m_clipEnabled; }
    void moveClipWindow(int dx, int dy);
    void rebuild();
    const uint8_t* data() const { return m_pixels.data(); }
    bool consumeDirty();

    void drawLineBresenham(int x0, int y0, int x1, int y1, glm::u8vec4 color);
    void drawCircleMidpoint(int cx, int cy, int radius, glm::u8vec4 color);
    void fillPolygonScanline(const std::vector<glm::ivec2>& poly, glm::u8vec4 color);
    bool clipLineCohenSutherland(float& x0, float& y0, float& x1, float& y1,
                                 float xmin, float ymin, float xmax, float ymax) const;

private:
    std::vector<uint8_t> m_pixels;
    bool m_clipEnabled = false;
    bool m_dirty = true;
    glm::ivec2 m_clipMin{28, 28};
    glm::ivec2 m_clipMax{100, 96};

    void setPixel(int x, int y, glm::u8vec4 color);
};
```

---

## 二、`src/ui/Raster2DBoard.cpp` 编写要求

1. **像素缓冲管理**：
   - 构造函数中将 `m_pixels` 初始化为 `WIDTH * HEIGHT * 4` 字节，并调用 `rebuild()`。
   - `setPixel(int x, int y, glm::u8vec4 color)`：检查 `0 <= x < WIDTH` 且 `0 <= y < HEIGHT`，写入 `m_pixels[(y * WIDTH + x) * 4 + 0..3]`。
   - `setClipEnabled(bool enabled)`：切换 `m_clipEnabled` 并调用 `rebuild()`。
   - `moveClipWindow(int dx, int dy)`：保持裁剪框宽高不变，将 `m_clipMin` 和 `m_clipMax` 平移 `(dx, dy)`，限制在 `[6, 121]` 范围内，调用 `rebuild()`。
   - `consumeDirty()`：若 `m_dirty` 为 `true`，将其置为 `false` 并返回 `true`，否则返回 `false`。

2. **纯手写四大经典二维图形学算法（配清晰中文公式注释）**：
   - **`drawLineBresenham`**：全八分象限整数 Bresenham 画线算法（误差项 `err = dx + dy`，其中 `dx = abs(x1-x0)`, `dy = -abs(y1-y0)`，每次判断 `e2 = 2 * err` 递推更新）。
   - **`drawCircleMidpoint`**：中点画圆算法，初值 `x = 0, y = radius, d = 1 - radius`；当 `d < 0` 时 `d += 2*x + 3`，否则 `d += 2*(x - y) + 5; y--`；利用八分对称性同时点亮 8 个对称像素。
   - **`fillPolygonScanline`**：有序边表（ET）+ 活性边表（AET）扫描线多边形填充算法：
     - 跳过水平边；
     - 采用**左闭右开**区间规则（`y` 从 `ymin` 到 `ymax - 1`，且当 `edge.ymax == y` 时从 AET 移除），避免多边形奇异顶点重复计数；
     - 每条扫描线对 AET 按 `x` 升序排列，两两配对填充像素，然后通过 `x += invSlope` 增量更新下一条扫描线交点。
   - **`clipLineCohenSutherland`**：4 位区域编码（`INSIDE=0, LEFT=1, RIGHT=2, BOTTOM=4, TOP=8`）：
     - 若 `(c0 | c1) == 0` 简取返回 `true`；
     - 若 `(c0 & c1) != 0` 简弃返回 `false`；
     - 否则取框外端点与裁剪框边界求交并更新区域码，循环直至收敛。

3. **`rebuild()` 沙盘绘制内容（体现延边民俗村规划布局）**：
   - 将整个 `128x128` 缓冲区清空为暖米白底色 `{242, 239, 232, 255}`，并用深木色画出外边框。
   - 使用 `fillPolygonScanline` 填充：
     - 左上、右上两块金色稻田多边形 `{217, 164, 65, 255}`；
     - 下方弧形布尔哈通河生态湾多边形 `{98, 168, 216, 255}`；
     - 中央 3 座朝鲜族民居屋顶多边形 `{74, 85, 96, 255}`。
   - 使用 `drawCircleMidpoint` 绘制：
     - 民俗村中心广场外圆环（圆心 `(64, 64)`，半径 `24` 和 `25`，色值 `{91, 58, 41, 255}`）；
     - 百年石砌古井内圆（圆心 `(64, 64)`，半径 `6` 和 `7`，色值 `{58, 124, 168, 255}`）；
     - 远景 3 座风力发电机位置圆标（半径 `5`）。
   - 预设 6 条贯穿全村的主干道与田埂斜线：
     - 若 `!m_clipEnabled`：用深褐色 `{115, 74, 52, 255}` 调用 `drawLineBresenham` 绘制全部道路。
     - 若 `m_clipEnabled`：
       1. 先用浅灰色 `{180, 175, 168, 255}` 画出所有道路的原始全长（代表被裁剪掉的外部段）；
       2. 对每条道路调用 `clipLineCohenSutherland`，若返回 `true`，用醒目的亮红色 `{220, 40, 40, 255}` 绘制裁剪后的保留线段（并偏移 1 像素再画一次使其呈 2 像素粗线，视觉对比强烈）；
       3. 用亮绿色 `{28, 145, 75, 255}` 画出 `[m_clipMin, m_clipMax]` 裁剪窗口矩形框（2 像素宽）。
   - 最后将 `m_dirty = true;`。
