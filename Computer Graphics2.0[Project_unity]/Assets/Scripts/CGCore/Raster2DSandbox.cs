// ============================================================================
// WP-U2 · Assets/Scripts/CGCore/Raster2DSandbox.cs
// 计算机图形学 2D 光栅化与裁剪沙盘（纯 CPU 像素缓冲算法）：
//   1. 中点 Bresenham 直线光栅化（覆盖全部 8 个象限斜率）
//   2. 中点画圆算法（八分圆对称绘制）
//   3. 有序边表（ET）与活性边表（AET）多边形扫描线填充算法
//   4. Cohen-Sutherland 四位区域编码（TOP=8, BOTTOM=4, RIGHT=2, LEFT=1）直线裁剪
// 实时生成 256×256 Texture2D，同步贴于村口 3D 导览木台与 HUD 放大镜面板。
// ============================================================================
using System.Collections.Generic;
using UnityEngine;

namespace YanbianCG.CGCore
{
    public class Raster2DSandbox
    {
        public const int Size = 256;
        private readonly Color32[] _pixels = new Color32[Size * Size];
        public Texture2D Texture { get; private set; }
        public bool ClipWindowEnabled { get; set; } = true;

        // 裁剪窗口边界 [52, 52] -> [204, 204]
        public int ClipXMin { get; set; } = 52;
        public int ClipYMin { get; set; } = 52;
        public int ClipXMax { get; set; } = 204;
        public int ClipYMax { get; set; } = 204;

        private const int INSIDE = 0; // 0000
        private const int LEFT   = 1; // 0001
        private const int RIGHT  = 2; // 0010
        private const int BOTTOM = 4; // 0100
        private const int TOP    = 8; // 1000

        public Raster2DSandbox()
        {
            Texture = new Texture2D(Size, Size, TextureFormat.RGBA32, false)
            {
                filterMode = FilterMode.Point,
                wrapMode = TextureWrapMode.Clamp
            };
            Redraw(0f);
        }

        private void SetPixel(int x, int y, Color32 c)
        {
            if ((uint)x < Size && (uint)y < Size)
                _pixels[y * Size + x] = c;
        }

        public void Clear(Color32 bg)
        {
            for (int i = 0; i < _pixels.Length; ++i)
                _pixels[i] = bg;
        }

        // --------------------------------------------------------------------
        // 1. 中点 Bresenham 直线算法（全八象限整数递推）
        // --------------------------------------------------------------------
        public void DrawLineBresenham(int x0, int y0, int x1, int y1, Color32 color)
        {
            int dx = Mathf.Abs(x1 - x0);
            int dy = Mathf.Abs(y1 - y0);
            int sx = x0 < x1 ? 1 : -1;
            int sy = y0 < y1 ? 1 : -1;
            int err = dx - dy;

            while (true)
            {
                SetPixel(x0, y0, color);
                if (x0 == x1 && y0 == y1) break;
                int e2 = 2 * err;
                if (e2 > -dy) { err -= dy; x0 += sx; }
                if (e2 <  dx) { err += dx; y0 += sy; }
            }
        }

        public void DrawThickLine(int x0, int y0, int x1, int y1, Color32 color)
        {
            DrawLineBresenham(x0, y0, x1, y1, color);
            DrawLineBresenham(x0 + 1, y0, x1 + 1, y1, color);
            DrawLineBresenham(x0, y0 + 1, x1, y1 + 1, color);
        }

        // --------------------------------------------------------------------
        // 2. 中点画圆算法（Midpoint Circle，八对称点）
        // --------------------------------------------------------------------
        public void DrawCircleMidpoint(int cx, int cy, int r, Color32 color)
        {
            int x = 0, y = r;
            int d = 1 - r;
            PlotCircleOctants(cx, cy, x, y, color);
            while (x < y)
            {
                if (d < 0)
                    d += 2 * x + 3;
                else
                {
                    d += 2 * (x - y) + 5;
                    y--;
                }
                x++;
                PlotCircleOctants(cx, cy, x, y, color);
            }
        }

        private void PlotCircleOctants(int cx, int cy, int x, int y, Color32 c)
        {
            SetPixel(cx + x, cy + y, c); SetPixel(cx - x, cy + y, c);
            SetPixel(cx + x, cy - y, c); SetPixel(cx - x, cy - y, c);
            SetPixel(cx + y, cy + x, c); SetPixel(cx - y, cy + x, c);
            SetPixel(cx + y, cy - x, c); SetPixel(cx - y, cy - x, c);
        }

        // --------------------------------------------------------------------
        // 3. 多边形有序边表（ET + AET）扫描线填充算法
        // --------------------------------------------------------------------
        private class EdgeEntry
        {
            public int yMax;
            public float x;
            public float invSlope;
        }

        public void FillPolygonScanline(Vector2Int[] pts, Color32 fillColor)
        {
            if (pts == null || pts.Length < 3) return;
            int yMin = Size - 1, yMax = 0;
            foreach (var p in pts)
            {
                if (p.y < yMin) yMin = p.y;
                if (p.y > yMax) yMax = p.y;
            }
            yMin = Mathf.Clamp(yMin, 0, Size - 1);
            yMax = Mathf.Clamp(yMax, 0, Size - 1);

            List<EdgeEntry>[] et = new List<EdgeEntry>[Size];
            int n = pts.Length;
            for (int i = 0; i < n; ++i)
            {
                Vector2Int p0 = pts[i];
                Vector2Int p1 = pts[(i + 1) % n];
                if (p0.y == p1.y) continue; // 忽略水平边
                if (p0.y > p1.y) { var tmp = p0; p0 = p1; p1 = tmp; }

                int startY = Mathf.Clamp(p0.y, 0, Size - 1);
                float invM = (float)(p1.x - p0.x) / (p1.y - p0.y);
                float startX = p0.x + (startY - p0.y) * invM;
                if (et[startY] == null) et[startY] = new List<EdgeEntry>();
                et[startY].Add(new EdgeEntry { yMax = p1.y, x = startX, invSlope = invM });
            }

            List<EdgeEntry> aet = new List<EdgeEntry>();
            for (int y = yMin; y <= yMax; ++y)
            {
                if (et[y] != null) aet.AddRange(et[y]);
                aet.RemoveAll(e => e.yMax <= y);
                aet.Sort((a, b) => a.x.CompareTo(b.x));

                for (int i = 0; i + 1 < aet.Count; i += 2)
                {
                    int xStart = Mathf.Clamp(Mathf.CeilToInt(aet[i].x), 0, Size - 1);
                    int xEnd   = Mathf.Clamp(Mathf.FloorToInt(aet[i + 1].x), 0, Size - 1);
                    for (int x = xStart; x <= xEnd; ++x)
                        SetPixel(x, y, fillColor);
                }

                for (int i = 0; i < aet.Count; ++i)
                    aet[i].x += aet[i].invSlope;
            }
        }

        // --------------------------------------------------------------------
        // 4. Cohen-Sutherland 4位编码直线段裁剪算法
        // --------------------------------------------------------------------
        private int ComputeOutCode(float x, float y)
        {
            int code = INSIDE;
            if (x < ClipXMin)      code |= LEFT;
            else if (x > ClipXMax) code |= RIGHT;
            if (y < ClipYMin)      code |= BOTTOM;
            else if (y > ClipYMax) code |= TOP;
            return code;
        }

        public bool ClipLineCohenSutherland(ref int x0, ref int y0, ref int x1, ref int y1)
        {
            float fx0 = x0, fy0 = y0, fx1 = x1, fy1 = y1;
            int out0 = ComputeOutCode(fx0, fy0);
            int out1 = ComputeOutCode(fx1, fy1);

            while (true)
            {
                if ((out0 | out1) == 0)
                {
                    x0 = Mathf.RoundToInt(fx0); y0 = Mathf.RoundToInt(fy0);
                    x1 = Mathf.RoundToInt(fx1); y1 = Mathf.RoundToInt(fy1);
                    return true;
                }
                if ((out0 & out1) != 0)
                    return false;

                int outOutside = out0 != 0 ? out0 : out1;
                float x = 0f, y = 0f;
                float dx = fx1 - fx0, dy = fy1 - fy0;

                if ((outOutside & TOP) != 0)
                {
                    x = fx0 + dx * (ClipYMax - fy0) / dy;
                    y = ClipYMax;
                }
                else if ((outOutside & BOTTOM) != 0)
                {
                    x = fx0 + dx * (ClipYMin - fy0) / dy;
                    y = ClipYMin;
                }
                else if ((outOutside & RIGHT) != 0)
                {
                    y = fy0 + dy * (ClipXMax - fx0) / dx;
                    x = ClipXMax;
                }
                else if ((outOutside & LEFT) != 0)
                {
                    y = fy0 + dy * (ClipXMin - fx0) / dx;
                    x = ClipXMin;
                }

                if (outOutside == out0)
                {
                    fx0 = x; fy0 = y;
                    out0 = ComputeOutCode(fx0, fy0);
                }
                else
                {
                    fx1 = x; fy1 = y;
                    out1 = ComputeOutCode(fx1, fy1);
                }
            }
        }

        public void DrawClippedLineDemo(int x0, int y0, int x1, int y1, Color32 rawColor, Color32 clippedColor)
        {
            // 先用虚色画全段，再用 Cohen-Sutherland 裁剪并高亮保留段
            DrawLineBresenham(x0, y0, x1, y1, rawColor);
            if (!ClipWindowEnabled) return;
            int cx0 = x0, cy0 = y0, cx1 = x1, cy1 = y1;
            if (ClipLineCohenSutherland(ref cx0, ref cy0, ref cx1, ref cy1))
            {
                DrawThickLine(cx0, cy0, cx1, cy1, clippedColor);
            }
        }

        // --------------------------------------------------------------------
        // 生成延边民俗村 2D 导览沙盘图（含河流扫描线填充、稻田、水井圆环与裁剪演示）
        // --------------------------------------------------------------------
        public void Redraw(float angleDeg)
        {
            Clear(new Color32(238, 232, 218, 255)); // 韩纸米白底色

            // 细网格线
            Color32 gridCol = new Color32(215, 208, 192, 255);
            for (int g = 16; g < Size; g += 16)
            {
                DrawLineBresenham(g, 0, g, Size - 1, gridCol);
                DrawLineBresenham(0, g, Size - 1, g, gridCol);
            }

            // 1) 扫描线填充：布尔哈通河湾（顶部蓝色弧形多边形）
            Vector2Int[] riverPoly = new Vector2Int[]
            {
                new Vector2Int(8, 248), new Vector2Int(248, 248),
                new Vector2Int(225, 215), new Vector2Int(175, 196),
                new Vector2Int(128, 190), new Vector2Int(80, 196),
                new Vector2Int(30, 215)
            };
            FillPolygonScanline(riverPoly, new Color32(68, 146, 202, 255));

            // 2) 扫描线填充：两侧金秋稻田梯形区块
            Vector2Int[] riceLeft = new Vector2Int[]
            {
                new Vector2Int(22, 42), new Vector2Int(108, 35),
                new Vector2Int(104, 88), new Vector2Int(32, 95)
            };
            Vector2Int[] riceRight = new Vector2Int[]
            {
                new Vector2Int(148, 35), new Vector2Int(234, 42),
                new Vector2Int(224, 95), new Vector2Int(152, 88)
            };
            FillPolygonScanline(riceLeft,  new Color32(218, 166, 48, 255));
            FillPolygonScanline(riceRight, new Color32(218, 166, 48, 255));

            // 3) 扫描线填充：中心民居飞檐与风车扇叶（随 angleDeg 实时旋转展示动态光栅化）
            int cx = 128, cy = 128;
            for (int b = 0; b < 3; ++b)
            {
                float a0 = (angleDeg + b * 120f) * Mathf.Deg2Rad;
                float a1 = a0 + 0.32f;
                Vector2Int[] blade = new Vector2Int[]
                {
                    new Vector2Int(cx, cy),
                    new Vector2Int(cx + Mathf.RoundToInt(Mathf.Cos(a0) * 44f), cy + Mathf.RoundToInt(Mathf.Sin(a0) * 44f)),
                    new Vector2Int(cx + Mathf.RoundToInt(Mathf.Cos(a1) * 40f), cy + Mathf.RoundToInt(Mathf.Sin(a1) * 40f))
                };
                FillPolygonScanline(blade, new Color32(58, 76, 98, 255));
            }

            // 4) 中点画圆算法：广场图腾环、水井圆圈与中心轮毂
            DrawCircleMidpoint(cx, cy, 48, new Color32(92, 58, 40, 255));
            DrawCircleMidpoint(cx, cy, 47, new Color32(92, 58, 40, 255));
            DrawCircleMidpoint(cx, cy, 10, new Color32(46, 118, 178, 255));
            DrawCircleMidpoint(48, 18, 9,  new Color32(110, 115, 125, 255));
            DrawCircleMidpoint(128, 18, 9, new Color32(110, 115, 125, 255));
            DrawCircleMidpoint(208, 18, 9, new Color32(110, 115, 125, 255));

            // 5) Cohen-Sutherland 直线裁剪窗口与放射测试线段
            Color32 rawRayCol  = new Color32(170, 162, 150, 255);
            Color32 clipRayCol = new Color32(216, 38, 38, 255);
            DrawClippedLineDemo(10,  10,  246, 246, rawRayCol, clipRayCol);
            DrawClippedLineDemo(10,  246, 246, 10,  rawRayCol, clipRayCol);
            DrawClippedLineDemo(128, 6,   128, 250, rawRayCol, clipRayCol);
            DrawClippedLineDemo(6,   128, 250, 128, rawRayCol, clipRayCol);
            DrawClippedLineDemo(18,  195, 238, 68,  rawRayCol, clipRayCol);
            DrawClippedLineDemo(25,  72,  235, 185, rawRayCol, clipRayCol);

            if (ClipWindowEnabled)
            {
                Color32 winCol = new Color32(24, 148, 56, 255);
                DrawThickLine(ClipXMin, ClipYMin, ClipXMax, ClipYMin, winCol);
                DrawThickLine(ClipXMax, ClipYMin, ClipXMax, ClipYMax, winCol);
                DrawThickLine(ClipXMax, ClipYMax, ClipXMin, ClipYMax, winCol);
                DrawThickLine(ClipXMin, ClipYMax, ClipXMin, ClipYMin, winCol);
            }

            // 外框木边
            Color32 borderCol = new Color32(82, 48, 30, 255);
            for (int t = 0; t < 4; ++t)
            {
                DrawLineBresenham(t, t, Size - 1 - t, t, borderCol);
                DrawLineBresenham(Size - 1 - t, t, Size - 1 - t, Size - 1 - t, borderCol);
                DrawLineBresenham(Size - 1 - t, Size - 1 - t, t, Size - 1 - t, borderCol);
                DrawLineBresenham(t, Size - 1 - t, t, t, borderCol);
            }

            Texture.SetPixels32(_pixels);
            Texture.Apply(false);
        }
    }
}
