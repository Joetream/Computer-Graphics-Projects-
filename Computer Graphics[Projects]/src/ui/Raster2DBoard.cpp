#include "ui/Raster2DBoard.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>

// =============================================================================
// Raster2DBoard.cpp —— 纯 CPU 软件光栅化沙盘（延边民俗村 128x128 规划板）
//
// 四个算法全部手写，不借助任何库：
//   * 全八分象限整数 Bresenham 画线
//   * 中点画圆 + 八分对称
//   * 有序边表(ET) / 活性边表(AET) 扫描线多边形填充
//   * Cohen-Sutherland 四位区域编码直线裁剪
//
// 像素格式：RGBA8888，行优先存放，字节偏移 (y * WIDTH + x) * 4。
// 坐标系：像素坐标，原点在左上角，y 轴向下（因此“上”对应 y 减小）。
// =============================================================================

namespace {

// Cohen-Sutherland 的 4 位区域码。
// 位值按工作包约定固定：INSIDE=0, LEFT=1, RIGHT=2, BOTTOM=4, TOP=8。
// 注意像素坐标 y 轴向下，所以 y < ymin 落在裁剪框“上方”，记为 TOP(8)；
// y > ymax 落在“下方”，记为 BOTTOM(4)。
constexpr int kCodeInside = 0;
constexpr int kCodeLeft   = 1;
constexpr int kCodeRight  = 2;
constexpr int kCodeBottom = 4;
constexpr int kCodeTop    = 8;

// 把点相对裁剪框的位置压缩成 4 位区域码（可同时落在多条边界外侧）。
int encodeRegion(float x, float y, float xmin, float ymin, float xmax, float ymax) {
    int code = kCodeInside;
    if (x < xmin) {
        code |= kCodeLeft;
    } else if (x > xmax) {
        code |= kCodeRight;
    }
    if (y < ymin) {
        code |= kCodeTop;
    } else if (y > ymax) {
        code |= kCodeBottom;
    }
    return code;
}

}  // namespace

// -----------------------------------------------------------------------------
// 构造：一次性分配 WIDTH*HEIGHT*4 字节的 RGBA 缓冲区，然后立即栅格化一帧沙盘。
// -----------------------------------------------------------------------------
Raster2DBoard::Raster2DBoard() {
    m_pixels.assign(static_cast<std::size_t>(WIDTH) * static_cast<std::size_t>(HEIGHT) * 4u, 0u);
    rebuild();
}

// -----------------------------------------------------------------------------
// 单个像素写入：越界直接丢弃，合法则按 R,G,B,A 顺序写入 4 个字节。
// -----------------------------------------------------------------------------
void Raster2DBoard::setPixel(int x, int y, glm::u8vec4 color) {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) {
        return;
    }
    const std::size_t offset = (static_cast<std::size_t>(y) * WIDTH + static_cast<std::size_t>(x)) * 4u;
    m_pixels[offset + 0] = color.r;
    m_pixels[offset + 1] = color.g;
    m_pixels[offset + 2] = color.b;
    m_pixels[offset + 3] = color.a;
}

// -----------------------------------------------------------------------------
// 裁剪开关：切换后必须重绘，因为 rebuild() 根据该标志决定道路是“全长深褐”还是
// “浅灰全长 + 亮红保留段 + 绿色窗口框”。
// -----------------------------------------------------------------------------
void Raster2DBoard::setClipEnabled(bool enabled) {
    m_clipEnabled = enabled;
    rebuild();
}

// -----------------------------------------------------------------------------
// 平移裁剪窗口：保持宽高不变，把左上角挪动 (dx, dy)。
// 窗口四条边都必须落在 [6, 121] 内（给外边框留出余量），因此
//   xMin' = clamp(xMin + dx, 6, 121 - w)
//   xMax' = xMin' + w
// w/h 用 std::max 兜底，保证极端尺寸下 clamp 的上下界不会倒挂。
// -----------------------------------------------------------------------------
void Raster2DBoard::moveClipWindow(int dx, int dy) {
    const int w = m_clipMax.x - m_clipMin.x;
    const int h = m_clipMax.y - m_clipMin.y;

    const int loX = 6;
    const int hiX = std::max(loX, 121 - w);
    const int loY = 6;
    const int hiY = std::max(loY, 121 - h);

    const int newMinX = std::clamp(m_clipMin.x + dx, loX, hiX);
    const int newMinY = std::clamp(m_clipMin.y + dy, loY, hiY);

    m_clipMin = glm::ivec2(newMinX, newMinY);
    m_clipMax = glm::ivec2(newMinX + w, newMinY + h);

    rebuild();
}

// -----------------------------------------------------------------------------
// 脏标记消费：供渲染层判断是否需要把 CPU 缓冲区重新上传为纹理。
// -----------------------------------------------------------------------------
bool Raster2DBoard::consumeDirty() {
    if (!m_dirty) {
        return false;
    }
    m_dirty = false;
    return true;
}

// -----------------------------------------------------------------------------
// Bresenham 画线（任意斜率 / 任意方向，全八分象限统一处理）
//
// 以 dx、dy 代替 1/4 象限分类：
//   dx =  |x1 - x0|          （水平跨度，恒非负）
//   dy = -|y1 - y0|          （竖直跨度，取负，使两者可直接相加表示“误差”）
//   sx = sign(x1 - x0)       （x 步进方向 ±1）
//   sy = sign(y1 - y0)       （y 步进方向 ±1）
//   err = dx + dy            （初始误差项）
// 主循环每次点亮当前像素后：
//   e2 = 2 * err
//   e2 >= dy  →  err += dy; x += sx      （水平步）
//   e2 <= dx  →  err += dx; y += sy      （竖直步）
// 同时满足时即为 45° 对角步，两个方向各走一格，天然覆盖八种斜率。
// -----------------------------------------------------------------------------
void Raster2DBoard::drawLineBresenham(int x0, int y0, int x1, int y1, glm::u8vec4 color) {
    int dx = std::abs(x1 - x0);
    int dy = -std::abs(y1 - y0);
    const int sx = (x0 < x1) ? 1 : -1;
    const int sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;  // 初始误差项 err = dx + dy

    for (;;) {
        setPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) {
            break;
        }
        const int e2 = 2 * err;  // 误差放大 2 倍，避免使用浮点半步
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

// -----------------------------------------------------------------------------
// 中点画圆（只从 0° 走到 45° 的一段圆弧，其余七段靠对称性补全）
//
// 初值：x = 0, y = radius, 判别式 d = 1 - radius
// 每一步点亮 8 个对称点：
//   (±x, ±y) 与 (±y, ±x) 组合
// 判别式递推（中点 M 落在圆弧内侧还是外侧）：
//   d < 0  →  d += 2x + 3          （选 E 点，y 不变）
//   d >= 0 →  d += 2(x - y) + 5; y--（选 SE 点）
// 循环条件 x <= y，正好走完 1/8 圆。
// -----------------------------------------------------------------------------
void Raster2DBoard::drawCircleMidpoint(int cx, int cy, int radius, glm::u8vec4 color) {
    if (radius < 0) {
        return;
    }
    int x = 0;
    int y = radius;
    int d = 1 - radius;  // 初值 d = 1 - r

    while (x <= y) {  // 只遍历 1/8 圆
        // 八分对称：同时点亮 8 个对称像素
        setPixel(cx + x, cy + y, color);
        setPixel(cx - x, cy + y, color);
        setPixel(cx + x, cy - y, color);
        setPixel(cx - x, cy - y, color);
        setPixel(cx + y, cy + x, color);
        setPixel(cx - y, cy + x, color);
        setPixel(cx + y, cy - x, color);
        setPixel(cx - y, cy - x, color);

        if (d < 0) {
            d += 2 * x + 3;  // 中点在圆内 → 取正东方向像素 E
        } else {
            d += 2 * (x - y) + 5;  // 中点在圆外 → 取东南方向像素 SE
            --y;
        }
        ++x;
    }
}

// -----------------------------------------------------------------------------
// 扫描线多边形填充（有序边表 ET + 活性边表 AET，奇偶规则）
//
// 边表结构：Edge{ ymax, x, invSlope }
//   ymax      —— 该边上端的扫描线号（不包含，即区间为 [ymin, ymax)）
//   x         —— 当前扫描线与该边的交点横坐标
//   invSlope  —— dx/dy，用于扫描线 +1 时增量推进交点：x += invSlope
//
// 主循环（左闭右开，规避奇异顶点重复计数）：
//   for y = polyYMin .. polyYMax - 1:
//     1) 从 AET 删除 ymax == y 的边（该边已不覆盖当前扫描线）
//     2) 把 ET[y] 中以 y 为 ymin 的边插入 AET
//     3) AET 按交点 x 升序排序
//     4) 两两配对，填充 [ceil(xL), ceil(xR)) 区间
//     5) 所有 AET 边的 x += invSlope，推进到下一条扫描线
// 水平边直接跳过（否则斜率无穷，AET 无法维护）。
// -----------------------------------------------------------------------------
void Raster2DBoard::fillPolygonScanline(const std::vector<glm::ivec2>& poly, glm::u8vec4 color) {
    const int vertexCount = static_cast<int>(poly.size());
    if (vertexCount < 3) {
        return;
    }

    struct Edge {
        int   ymax;
        float x;
        float invSlope;
    };

    std::vector<std::vector<Edge>> edgeTable(static_cast<std::size_t>(HEIGHT));  // 有序边表 ET，按 ymin 分桶
    int polyYMin = HEIGHT;
    int polyYMax = -1;

    for (int i = 0; i < vertexCount; ++i) {
        const glm::ivec2& p1 = poly[static_cast<std::size_t>(i)];
        const glm::ivec2& p2 = poly[static_cast<std::size_t>((i + 1) % vertexCount)];
        if (p1.y == p2.y) {
            continue;  // 跳过水平边（斜率为无穷，不参与交点配对）
        }

        const glm::ivec2& pLow  = (p1.y < p2.y) ? p1 : p2;  // y 较小（上端）
        const glm::ivec2& pHigh = (p1.y < p2.y) ? p2 : p1;  // y 较大（下端）

        Edge edge;
        edge.ymax     = pHigh.y;
        edge.x        = static_cast<float>(pLow.x);
        edge.invSlope = static_cast<float>(pHigh.x - pLow.x) / static_cast<float>(pHigh.y - pLow.y);

        if (pLow.y >= 0 && pLow.y < HEIGHT) {
            edgeTable[static_cast<std::size_t>(pLow.y)].push_back(edge);
        }
        polyYMin = std::min(polyYMin, pLow.y);
        polyYMax = std::max(polyYMax, pHigh.y);
    }

    if (polyYMax < 0) {
        return;  // 全部是水平边，没有可填充区域
    }
    polyYMin = std::max(polyYMin, 0);
    polyYMax = std::min(polyYMax, HEIGHT);
    if (polyYMin >= polyYMax) {
        return;
    }

    std::vector<Edge> activeTable;  // 活性边表 AET

    for (int y = polyYMin; y < polyYMax; ++y) {
        // 1) 移出：上端 ymax == y 的边在左闭右开规则下已不覆盖本扫描线
        activeTable.erase(std::remove_if(activeTable.begin(), activeTable.end(),
                                         [y](const Edge& edge) { return edge.ymax <= y; }),
                          activeTable.end());

        // 2) 活化：取出生效于本扫描线的新边
        for (const Edge& edge : edgeTable[static_cast<std::size_t>(y)]) {
            activeTable.push_back(edge);
        }

        // 3) 按交点 x 升序排序，保证相邻两条边构成一个内部区间
        std::sort(activeTable.begin(), activeTable.end(),
                  [](const Edge& a, const Edge& b) { return a.x < b.x; });

        // 4) 两两配对填充（左闭右开：填充到右交点之前一个像素）
        for (std::size_t i = 0; i + 1 < activeTable.size(); i += 2) {
            int xStart = static_cast<int>(std::ceil(activeTable[i].x));
            int xEnd   = static_cast<int>(std::ceil(activeTable[i + 1].x)) - 1;
            xStart = std::max(xStart, 0);
            xEnd   = std::min(xEnd, WIDTH - 1);
            for (int x = xStart; x <= xEnd; ++x) {
                setPixel(x, y, color);
            }
        }

        // 5) 增量推进：x(y + 1) = x(y) + dx/dy
        for (Edge& edge : activeTable) {
            edge.x += edge.invSlope;
        }
    }
}

// -----------------------------------------------------------------------------
// Cohen-Sutherland 直线裁剪（4 位区域编码）
//
// 编码：INSIDE=0, LEFT=1, RIGHT=2, BOTTOM=4, TOP=8（按位或）
// 判定：
//   (c0 | c1) == 0        → 线段完全在框内，直接返回 true（简取）
//   (c0 & c1) != 0        → 两端点共享某一外侧，整条线在框外，返回 false（简弃）
//   否则取一个框外端点，用它与某条裁剪边求交（参数式），用它替换该端点并重算编码，
//   循环直至收敛（每次至少消除一个外侧位，最多 4 轮）。
// 求交公式（以 y 边界为例）：
//   x = x0 + (x1 - x0) * (yEdge - y0) / (y1 - y0),  y = yEdge
// 以 x 边界为例：
//   y = y0 + (y1 - y0) * (xEdge - x0) / (x1 - x0),  x = xEdge
// 裁剪结果原地写回 x0/y0/x1/y1，供调用者用整数 Bresenham 落笔。
// -----------------------------------------------------------------------------
bool Raster2DBoard::clipLineCohenSutherland(float& x0, float& y0, float& x1, float& y1,
                                            float xmin, float ymin, float xmax, float ymax) const {
    int code0 = encodeRegion(x0, y0, xmin, ymin, xmax, ymax);
    int code1 = encodeRegion(x1, y1, xmin, ymin, xmax, ymax);

    // 每次循环至少把某个端点拉回到一条边界上，最多 4 条边界，8 次足够收敛。
    for (int guard = 0; guard < 8; ++guard) {
        if ((code0 | code1) == kCodeInside) {
            return true;  // 完全可见
        }
        if ((code0 & code1) != 0) {
            return false;  // 同侧完全不可见
        }

        const int outCode = (code0 != kCodeInside) ? code0 : code1;
        float x = 0.0f;
        float y = 0.0f;

        if ((outCode & kCodeTop) != 0) {  // 越过了上边界 y = ymin
            y = ymin;
            x = x0 + (x1 - x0) * (ymin - y0) / (y1 - y0);
        } else if ((outCode & kCodeBottom) != 0) {  // 越过了下边界 y = ymax
            y = ymax;
            x = x0 + (x1 - x0) * (ymax - y0) / (y1 - y0);
        } else if ((outCode & kCodeRight) != 0) {  // 越过了右边界 x = xmax
            x = xmax;
            y = y0 + (y1 - y0) * (xmax - x0) / (x1 - x0);
        } else {  // 越过了左边界 x = xmin
            x = xmin;
            y = y0 + (y1 - y0) * (xmin - x0) / (x1 - x0);
        }

        if (outCode == code0) {
            x0 = x;
            y0 = y;
            code0 = encodeRegion(x0, y0, xmin, ymin, xmax, ymax);
        } else {
            x1 = x;
            y1 = y;
            code1 = encodeRegion(x1, y1, xmin, ymin, xmax, ymax);
        }
    }

    return (code0 | code1) == kCodeInside;
}

// -----------------------------------------------------------------------------
// 重绘整块沙盘：暖米白底 → 金色稻田 → 布尔哈通河生态湾 → 三座民居屋顶
//              → 中心广场圆环 / 古井 / 风电机位圆标 → 六条道路（可裁剪）
//              → 裁剪窗口绿框 → 深木色外边框。
// 每帧/每次交互都会整体重画，最后置脏标记供纹理上传。
// -----------------------------------------------------------------------------
void Raster2DBoard::rebuild() {
    // ---- 0. 清屏：暖米白底色 ----
    const glm::u8vec4 background(242, 239, 232, 255);
    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            setPixel(x, y, background);
        }
    }

    // ---- 1. 金色稻田（左上、右上两块多边形，扫描线填充）----
    const glm::u8vec4 riceGold(217, 164, 65, 255);
    const std::vector<glm::ivec2> riceFieldLeft{
        glm::ivec2(10, 24), glm::ivec2(54, 16), glm::ivec2(56, 40), glm::ivec2(18, 52)
    };
    const std::vector<glm::ivec2> riceFieldRight{
        glm::ivec2(72, 16), glm::ivec2(116, 24), glm::ivec2(112, 50), glm::ivec2(71, 38)
    };
    fillPolygonScanline(riceFieldLeft, riceGold);
    fillPolygonScanline(riceFieldRight, riceGold);

    // ---- 2. 布尔哈通河生态湾（下方弧形水带）----
    // 上边界取抛物线弧 y = 96 + 28 * ((x - 64) / 60)^2：正中最低（y=96）、
    // 越靠近两侧越向下弯，形成“湾”的形态；采样后沿 y=125 的底边闭合。
    const glm::u8vec4 riverBlue(98, 168, 216, 255);
    std::vector<glm::ivec2> riverBay;
    for (int x = 6; x <= 122; x += 2) {
        const float t = static_cast<float>(x - 64) / 60.0f;
        const float arcY = 96.0f + 28.0f * t * t;
        int y = static_cast<int>(arcY + 0.5f);
        y = std::clamp(y, 96, 124);
        riverBay.push_back(glm::ivec2(x, y));
    }
    riverBay.push_back(glm::ivec2(122, 125));
    riverBay.push_back(glm::ivec2(6, 125));
    fillPolygonScanline(riverBay, riverBlue);

    // ---- 3. 中央三座朝鲜族民居屋顶（梯形：屋脊窄、檐口宽）----
    const glm::u8vec4 roofSlate(74, 85, 96, 255);
    auto makeRoof = [](int cx, int cy) {
        std::vector<glm::ivec2> roof;
        roof.push_back(glm::ivec2(cx - 7, cy + 4));  // 左檐口（宽）
        roof.push_back(glm::ivec2(cx - 4, cy - 4));  // 左屋脊（窄）
        roof.push_back(glm::ivec2(cx + 4, cy - 4));  // 右屋脊（窄）
        roof.push_back(glm::ivec2(cx + 7, cy + 4));  // 右檐口（宽）
        return roof;
    };
    fillPolygonScanline(makeRoof(50, 58), roofSlate);
    fillPolygonScanline(makeRoof(78, 58), roofSlate);
    fillPolygonScanline(makeRoof(64, 76), roofSlate);

    // ---- 4. 中心广场外圆环 / 百年古井内圆 / 远景风电机位圆标 ----
    const glm::u8vec4 darkWood(91, 58, 41, 255);
    drawCircleMidpoint(64, 64, 25, darkWood);  // 广场外圆环（外圈）
    drawCircleMidpoint(64, 64, 24, darkWood);  // 广场外圆环（内圈）

    const glm::u8vec4 wellBlue(58, 124, 168, 255);
    drawCircleMidpoint(64, 64, 7, wellBlue);  // 古井井口（外圈）
    drawCircleMidpoint(64, 64, 6, wellBlue);  // 古井井口（内圈）

    // 远景风电机位色：工作包未指定色值，取雾灰蓝以与米白底拉开层次。
    const glm::u8vec4 windMarker(116, 134, 146, 255);
    drawCircleMidpoint(20, 10, 5, windMarker);
    drawCircleMidpoint(64, 10, 5, windMarker);
    drawCircleMidpoint(108, 10, 5, windMarker);

    // ---- 5. 六条贯穿全村的主干道与田埂斜线 ----
    // {x0, y0, x1, y1}
    static const int roads[6][4] = {
        {  2,   4, 125, 111},  // 西北—东南主干道
        {125,   6,   3, 111},  // 东北—西南主干道
        { 64,   0,  64, 127},  // 南北中轴路
        {  0,  58, 127,  58},  // 东西横路
        {  6,  88, 122,  22},  // 田园斜路（田埂）
        {  0, 104, 127,  71}   // 临河斜路（田埂）
    };
    constexpr int kRoadCount = 6;

    if (!m_clipEnabled) {
        // 未开裁剪：全部道路用深褐色画满全长
        const glm::u8vec4 roadBrown(115, 74, 52, 255);
        for (int i = 0; i < kRoadCount; ++i) {
            drawLineBresenham(roads[i][0], roads[i][1], roads[i][2], roads[i][3], roadBrown);
        }
    } else {
        // 开了裁剪：先画原始全长（浅灰），代表被裁掉的外部段
        const glm::u8vec4 roadFaded(180, 175, 168, 255);
        for (int i = 0; i < kRoadCount; ++i) {
            drawLineBresenham(roads[i][0], roads[i][1], roads[i][2], roads[i][3], roadFaded);
        }

        // 再画裁剪后的保留段：亮红色，偏移 1 像素重复一次 → 2 像素粗线
        const glm::u8vec4 roadClipped(220, 40, 40, 255);
        const float clipXMin = static_cast<float>(m_clipMin.x);
        const float clipYMin = static_cast<float>(m_clipMin.y);
        const float clipXMax = static_cast<float>(m_clipMax.x);
        const float clipYMax = static_cast<float>(m_clipMax.y);

        for (int i = 0; i < kRoadCount; ++i) {
            float x0 = static_cast<float>(roads[i][0]);
            float y0 = static_cast<float>(roads[i][1]);
            float x1 = static_cast<float>(roads[i][2]);
            float y1 = static_cast<float>(roads[i][3]);

            if (!clipLineCohenSutherland(x0, y0, x1, y1, clipXMin, clipYMin, clipXMax, clipYMax)) {
                continue;  // 整条线落在窗口外，无保留段
            }

            const int ix0 = static_cast<int>(std::lround(x0));
            const int iy0 = static_cast<int>(std::lround(y0));
            const int ix1 = static_cast<int>(std::lround(x1));
            const int iy1 = static_cast<int>(std::lround(y1));

            drawLineBresenham(ix0, iy0, ix1, iy1, roadClipped);
            drawLineBresenham(ix0 + 1, iy0 + 1, ix1 + 1, iy1 + 1, roadClipped);  // 偏移 1 像素加粗
        }

        // 最后画裁剪窗口矩形框：亮绿色，2 像素宽（内外各一条边）
        const glm::u8vec4 clipFrame(28, 145, 75, 255);
        for (int t = 0; t < 2; ++t) {
            drawLineBresenham(m_clipMin.x + t, m_clipMin.y + t, m_clipMax.x - t, m_clipMin.y + t, clipFrame);
            drawLineBresenham(m_clipMin.x + t, m_clipMax.y - t, m_clipMax.x - t, m_clipMax.y - t, clipFrame);
            drawLineBresenham(m_clipMin.x + t, m_clipMin.y + t, m_clipMin.x + t, m_clipMax.y - t, clipFrame);
            drawLineBresenham(m_clipMax.x - t, m_clipMin.y + t, m_clipMax.x - t, m_clipMax.y - t, clipFrame);
        }
    }

    // ---- 6. 外边框（深木色，2 像素）----
    // 放在最后画，避免道路端点一直顶到缓冲区边缘时把边框覆盖掉。
    const glm::u8vec4 borderWood(91, 58, 41, 255);
    for (int t = 0; t < 2; ++t) {
        for (int x = 0; x < WIDTH; ++x) {
            setPixel(x, t, borderWood);
            setPixel(x, HEIGHT - 1 - t, borderWood);
        }
        for (int y = 0; y < HEIGHT; ++y) {
            setPixel(t, y, borderWood);
            setPixel(WIDTH - 1 - t, y, borderWood);
        }
    }

    m_dirty = true;
}
