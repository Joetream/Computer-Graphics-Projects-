// ============================================================================
// WP-U4 · Assets/Scripts/Scene/ProceduralMeshLib.cs
// 延边朝鲜族民俗村 3D 程序化网格构建器：
// 精确复现 1.0 引擎的地形、布尔哈通河道、6栋朝鲜族合院民居（含可调曲率双三次 Bézier 飞檐）、
// 村口四柱迎宾牌楼、百年水井、2D 导览木台与交叉十字金秋稻穗簇。
// ============================================================================
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Rendering;
using YanbianCG.CGCore;

namespace YanbianCG.Scene
{
    public class MeshBuildBuffer
    {
        public readonly List<Vector3> verts = new List<Vector3>(2048);
        public readonly List<Vector3> norms = new List<Vector3>(2048);
        public readonly List<Vector2> uvs   = new List<Vector2>(2048);
        public readonly List<Color>   cols  = new List<Color>(2048);
        public readonly List<int>     tris  = new List<int>(4096);

        public void Clear()
        {
            verts.Clear(); norms.Clear(); uvs.Clear(); cols.Clear(); tris.Clear();
        }

        public void AddBox(Vector3 center, Vector3 size, Color color, Quaternion rot = default)
        {
            if (rot.x == 0f && rot.y == 0f && rot.z == 0f && rot.w == 0f)
                rot = Quaternion.identity;

            Vector3 h = size * 0.5f;
            Vector3[] faceNormals =
            {
                Vector3.forward, Vector3.back, Vector3.right,
                Vector3.left, Vector3.up, Vector3.down
            };
            Vector3[,] faceCorners =
            {
                { new Vector3(-h.x,-h.y, h.z), new Vector3( h.x,-h.y, h.z), new Vector3( h.x, h.y, h.z), new Vector3(-h.x, h.y, h.z) },
                { new Vector3( h.x,-h.y,-h.z), new Vector3(-h.x,-h.y,-h.z), new Vector3(-h.x, h.y,-h.z), new Vector3( h.x, h.y,-h.z) },
                { new Vector3( h.x,-h.y, h.z), new Vector3( h.x,-h.y,-h.z), new Vector3( h.x, h.y,-h.z), new Vector3( h.x, h.y, h.z) },
                { new Vector3(-h.x,-h.y,-h.z), new Vector3(-h.x,-h.y, h.z), new Vector3(-h.x, h.y, h.z), new Vector3(-h.x, h.y,-h.z) },
                { new Vector3(-h.x, h.y, h.z), new Vector3( h.x, h.y, h.z), new Vector3( h.x, h.y,-h.z), new Vector3(-h.x, h.y,-h.z) },
                { new Vector3(-h.x,-h.y,-h.z), new Vector3( h.x,-h.y,-h.z), new Vector3( h.x,-h.y, h.z), new Vector3(-h.x,-h.y, h.z) }
            };

            for (int f = 0; f < 6; ++f)
            {
                int b = verts.Count;
                Vector3 n = rot * faceNormals[f];
                for (int c = 0; c < 4; ++c)
                {
                    verts.Add(center + rot * faceCorners[f, c]);
                    norms.Add(n);
                    cols.Add(color);
                }
                uvs.Add(new Vector2(0, 0)); uvs.Add(new Vector2(1, 0));
                uvs.Add(new Vector2(1, 1)); uvs.Add(new Vector2(0, 1));
                tris.Add(b + 0); tris.Add(b + 2); tris.Add(b + 1);
                tris.Add(b + 0); tris.Add(b + 3); tris.Add(b + 2);
            }
        }

        public void AddCylinder(Vector3 baseCenter, float r0, float r1, float height, int segs, Color color)
        {
            segs = Mathf.Max(segs, 6);
            int baseIdx = verts.Count;
            float slopeY = (r0 - r1) / Mathf.Max(height, 1e-4f);

            for (int i = 0; i <= segs; ++i)
            {
                float u = (float)i / segs;
                float a = u * Mathf.PI * 2f;
                float ca = Mathf.Cos(a), sa = Mathf.Sin(a);
                Vector3 n = new Vector3(ca, slopeY, sa).normalized;

                verts.Add(baseCenter + new Vector3(ca * r0, 0f, sa * r0));
                norms.Add(n); uvs.Add(new Vector2(u, 0f)); cols.Add(color);

                verts.Add(baseCenter + new Vector3(ca * r1, height, sa * r1));
                norms.Add(n); uvs.Add(new Vector2(u, 1f)); cols.Add(color);
            }

            for (int i = 0; i < segs; ++i)
            {
                int k = baseIdx + i * 2;
                tris.Add(k);     tris.Add(k + 1); tris.Add(k + 3);
                tris.Add(k);     tris.Add(k + 3); tris.Add(k + 2);
            }
        }

        public void AddTriangle(Vector3 p0, Vector3 p1, Vector3 p2, Color color)
        {
            int b = verts.Count;
            Vector3 n = Vector3.Cross(p1 - p0, p2 - p0).normalized;
            verts.Add(p0); verts.Add(p1); verts.Add(p2);
            norms.Add(n);  norms.Add(n);  norms.Add(n);
            uvs.Add(new Vector2(0, 0)); uvs.Add(new Vector2(1, 0)); uvs.Add(new Vector2(0.5f, 1));
            cols.Add(color); cols.Add(color); cols.Add(color);
            tris.Add(b); tris.Add(b + 1); tris.Add(b + 2);
        }

        public Mesh ToMesh(string name)
        {
            Mesh m = new Mesh { name = name };
            if (verts.Count > 65000)
                m.indexFormat = IndexFormat.UInt32;
            m.SetVertices(verts);
            m.SetNormals(norms);
            m.SetUVs(0, uvs);
            m.SetColors(cols);
            m.SetTriangles(tris, 0);
            m.RecalculateBounds();
            return m;
        }
    }

    public static class ProceduralMeshLib
    {
        public static readonly Color WallColor   = new Color(0.95f, 0.94f, 0.91f, 1f);
        public static readonly Color TileColor   = new Color(0.29f, 0.34f, 0.38f, 1f);
        public static readonly Color WoodColor   = new Color(0.36f, 0.23f, 0.16f, 1f);
        public static readonly Color StoneColor  = new Color(0.60f, 0.58f, 0.55f, 1f);
        public static readonly Color BrickColor  = new Color(0.78f, 0.35f, 0.28f, 1f);
        public static readonly Color AzaleaColor = new Color(0.88f, 0.48f, 0.63f, 1f);
        public static readonly Color WindowWarm  = new Color(0.98f, 0.84f, 0.55f, 1f);

        public static Mesh BuildKoreanHouseMesh(float cornerLift, int roofSteps, bool withChimney)
        {
            MeshBuildBuffer b = new MeshBuildBuffer();

            // 1. 石砌台基 + 暖白墙体 + 门前石阶
            b.AddBox(new Vector3(0f, 0.275f, 0f), new Vector3(7.6f, 0.55f, 5.2f), StoneColor);
            b.AddBox(new Vector3(0f, 1.85f,  0f), new Vector3(6.8f, 2.60f, 4.4f), WallColor);
            b.AddBox(new Vector3(0f, 0.10f, 2.85f), new Vector3(2.3f, 0.20f, 1.1f), StoneColor);

            // 2. 四角深褐木立柱与檐下横梁
            for (int sx = -1; sx <= 1; sx += 2)
            {
                for (int sz = -1; sz <= 1; sz += 2)
                    b.AddBox(new Vector3(3.26f * sx, 1.85f, 2.06f * sz), new Vector3(0.28f, 2.6f, 0.28f), WoodColor);
                b.AddBox(new Vector3(3.26f * sx, 3.02f, 0f), new Vector3(0.20f, 0.24f, 4.5f), WoodColor);
            }
            b.AddBox(new Vector3(0f, 3.02f,  2.06f), new Vector3(6.9f, 0.24f, 0.20f), WoodColor);
            b.AddBox(new Vector3(0f, 3.02f, -2.06f), new Vector3(6.9f, 0.24f, 0.20f), WoodColor);

            // 3. 正面韩式木格栅推拉门 + 韩纸内衬
            b.AddBox(new Vector3(0f, 1.76f, 2.21f), new Vector3(2.40f, 2.15f, 0.04f), WindowWarm);
            b.AddBox(new Vector3(-1.25f, 1.76f, 2.24f), new Vector3(0.12f, 2.28f, 0.10f), WoodColor);
            b.AddBox(new Vector3( 1.25f, 1.76f, 2.24f), new Vector3(0.12f, 2.28f, 0.10f), WoodColor);
            b.AddBox(new Vector3(0f, 2.84f, 2.24f), new Vector3(2.62f, 0.12f, 0.10f), WoodColor);
            b.AddBox(new Vector3(0f, 0.68f, 2.24f), new Vector3(2.62f, 0.12f, 0.10f), WoodColor);
            for (int i = 0; i <= 6; ++i)
            {
                float x = -1.05f + 0.35f * i;
                b.AddBox(new Vector3(x, 1.78f, 2.27f), new Vector3(0.05f, 2.02f, 0.05f), WoodColor);
            }
            for (int i = 0; i <= 4; ++i)
            {
                float y = 0.92f + 0.43f * i;
                b.AddBox(new Vector3(0f, y, 2.28f), new Vector3(2.32f, 0.045f, 0.05f), WoodColor);
            }

            // 4. 两侧木格栅窗
            for (int sideX = -1; sideX <= 1; sideX += 2)
            {
                float wx = 3.42f * sideX;
                b.AddBox(new Vector3(wx - 0.01f * sideX, 1.85f, 0f), new Vector3(0.03f, 0.95f, 1.55f), WindowWarm);
                b.AddBox(new Vector3(wx, 1.85f, -0.80f), new Vector3(0.08f, 1.0f, 0.10f), WoodColor);
                b.AddBox(new Vector3(wx, 1.85f,  0.80f), new Vector3(0.08f, 1.0f, 0.10f), WoodColor);
                b.AddBox(new Vector3(wx, 1.35f,  0f),    new Vector3(0.08f, 0.10f, 1.70f), WoodColor);
                b.AddBox(new Vector3(wx, 2.35f,  0f),    new Vector3(0.08f, 0.10f, 1.70f), WoodColor);
                for (int i = 0; i <= 4; ++i)
                    b.AddBox(new Vector3(wx + 0.01f * sideX, 1.85f, -0.64f + 0.32f * i), new Vector3(0.05f, 0.92f, 0.05f), WoodColor);
            }

            // 5. 6 片双三次 Bézier C1 歇山顶 + 正脊吻兽
            BezierPatchGenerator.AppendKoreanBezierRoof(
                8.4f, 6.0f, 2.1f, cornerLift, roofSteps, new Vector3(0f, 3.15f, 0f),
                TileColor, WoodColor, b.verts, b.norms, b.uvs, b.cols, b.tris);

            float ridgeHalf = Mathf.Max(4.2f - 0.32f * 6.0f, 0.18f * 8.4f);
            b.AddBox(new Vector3(0f, 3.15f + 2.17f, 0f), new Vector3(2f * ridgeHalf + 0.35f, 0.16f, 0.22f), TileColor);
            for (int side = -1; side <= 1; side += 2)
            {
                float xr = side * (ridgeHalf + 0.10f);
                b.AddBox(new Vector3(xr, 3.15f + 2.30f, 0f), new Vector3(0.16f, 0.30f, 0.26f), TileColor);
                b.AddBox(new Vector3(xr + side * 0.05f, 3.15f + 2.50f, 0f), new Vector3(0.10f, 0.14f, 0.18f), TileColor);
            }

            // 6. 朝鲜族特色独立火炕红砖烟囱（与地下火炕烟道相连）
            if (withChimney)
            {
                float cx = 4.5f, cz = -1.2f;
                b.AddBox(new Vector3(cx, 0.28f, cz), new Vector3(0.95f, 0.56f, 0.95f), BrickColor);
                b.AddCylinder(new Vector3(cx, 0.50f, cz), 0.24f, 0.20f, 3.8f, 12, BrickColor);
                b.AddCylinder(new Vector3(cx, 4.30f, cz), 0.27f, 0.27f, 0.10f, 12, BrickColor);
                b.AddBox(new Vector3(cx - 0.16f, 4.44f, cz), new Vector3(0.07f, 0.12f, 0.07f), WoodColor);
                b.AddBox(new Vector3(cx + 0.16f, 4.44f, cz), new Vector3(0.07f, 0.12f, 0.07f), WoodColor);
                b.AddBox(new Vector3(cx, 4.55f, cz), new Vector3(0.70f, 0.08f, 0.70f), WoodColor);
            }

            return b.ToMesh("KoreanHouse_Bezier");
        }

        public static Mesh BuildWelcomeGateMesh(float cornerLift, int roofSteps)
        {
            MeshBuildBuffer b = new MeshBuildBuffer();
            b.AddBox(new Vector3(-5.15f, 0.20f, 0f), new Vector3(1.90f, 0.40f, 3.20f), StoneColor);
            b.AddBox(new Vector3( 5.15f, 0.20f, 0f), new Vector3(1.90f, 0.40f, 3.20f), StoneColor);
            b.AddBox(new Vector3( 0.00f, 0.05f, 0f), new Vector3(8.60f, 0.10f, 2.60f), StoneColor);

            for (int s = -1; s <= 1; s += 2)
            {
                float x = 3.90f * s;
                b.AddBox(new Vector3(x, 0.25f, 0f), new Vector3(1.05f, 0.50f, 1.05f), StoneColor);
                b.AddBox(new Vector3(x, 1.80f, 0f), new Vector3(0.70f, 3.60f, 0.70f), WoodColor);
                b.AddBox(new Vector3(x, 3.72f, 0f), new Vector3(1.35f, 0.28f, 0.95f), WoodColor);
            }

            b.AddBox(new Vector3(0f, 3.98f, 0f), new Vector3(8.90f, 0.34f, 0.72f), WoodColor);
            b.AddBox(new Vector3(0f, 4.28f, 0f), new Vector3(9.30f, 0.26f, 0.82f), WoodColor);
            for (int i = -2; i <= 2; ++i)
                b.AddBox(new Vector3(1.60f * i, 4.51f, 0f), new Vector3(0.34f, 0.26f, 0.58f), WoodColor);

            b.AddBox(new Vector3(-2.0f, 3.66f, 0.60f), new Vector3(0.06f, 0.32f, 0.04f), WoodColor);
            b.AddBox(new Vector3( 2.0f, 3.66f, 0.60f), new Vector3(0.06f, 0.32f, 0.04f), WoodColor);

            BezierPatchGenerator.AppendKoreanBezierRoof(
                9.8f, 5.2f, 1.25f, cornerLift, roofSteps, new Vector3(0f, 4.55f, 0f),
                TileColor, WoodColor, b.verts, b.norms, b.uvs, b.cols, b.tris);

            float ridgeHalf = 2.5f;
            b.AddBox(new Vector3(0f, 4.55f + 1.32f, 0f), new Vector3(2f * ridgeHalf + 0.4f, 0.16f, 0.22f), TileColor);
            for (int s = -1; s <= 1; s += 2)
                b.AddBox(new Vector3(s * (ridgeHalf + 0.1f), 4.55f + 1.48f, 0f), new Vector3(0.16f, 0.32f, 0.26f), TileColor);

            return b.ToMesh("WelcomeGate_Bezier");
        }

        public static Mesh BuildVillagePropsMesh(int roofSteps)
        {
            MeshBuildBuffer b = new MeshBuildBuffer();

            // 1. 百年石砌老水井 (3, 0, 0) + 酱缸台
            b.AddCylinder(new Vector3(3f, 0f, 0f), 0.62f, 0.55f, 0.55f, 8, StoneColor);
            b.AddBox(new Vector3(3f, 0.50f, 0f), new Vector3(0.86f, 0.04f, 0.86f), new Color(0.12f, 0.28f, 0.38f));
            for (int s = -1; s <= 1; s += 2)
                b.AddBox(new Vector3(3f + 0.78f * s, 1.05f, 0f), new Vector3(0.14f, 2.10f, 0.14f), WoodColor);

            BezierPatchGenerator.AppendKoreanBezierRoof(
                2.5f, 2.1f, 0.62f, 0.18f, roofSteps, new Vector3(3f, 2.12f, 0f),
                TileColor, WoodColor, b.verts, b.norms, b.uvs, b.cols, b.tris);

            b.AddBox(new Vector3(3f, 1.75f, 0f), new Vector3(1.48f, 0.10f, 0.10f), WoodColor);
            b.AddCylinder(new Vector3(3f, 0.60f, 0f), 0.02f, 0.02f, 1.15f, 6, WoodColor);
            b.AddBox(new Vector3(3f, 0.53f, 0f), new Vector3(0.22f, 0.16f, 0.22f), WoodColor);

            // 朝鲜族辣白菜/大酱缸台 (Jangdokdae)
            b.AddBox(new Vector3(-4.2f, 0.18f, 2.5f), new Vector3(2.8f, 0.36f, 1.4f), StoneColor);
            for (int j = 0; j < 4; ++j)
            {
                float jx = -5.1f + j * 0.60f;
                b.AddCylinder(new Vector3(jx, 0.36f, 2.3f), 0.18f, 0.23f, 0.35f, 10, new Color(0.32f, 0.18f, 0.11f));
                b.AddCylinder(new Vector3(jx, 0.71f, 2.3f), 0.23f, 0.16f, 0.15f, 10, new Color(0.32f, 0.18f, 0.11f));
                b.AddCylinder(new Vector3(jx, 0.86f, 2.3f), 0.19f, 0.19f, 0.05f, 10, WoodColor);
            }

            // 2. 2D 沙盘导览木台 (-7.5, 0, 18)
            float sx = -7.5f, sz = 18.0f;
            for (int ix = -1; ix <= 1; ix += 2)
                for (int iz = -1; iz <= 1; iz += 2)
                    b.AddBox(new Vector3(sx + 0.86f * ix, 0.36f, sz + 0.62f * iz), new Vector3(0.12f, 0.72f, 0.12f), WoodColor);
            b.AddBox(new Vector3(sx, 0.78f, sz), new Vector3(2.0f, 0.12f, 1.5f), WoodColor);

            // 3. 村口旅游导览立牌木框 (7.5, 0, 22) + 金达莱花坛
            float px = 7.5f, pz = 22.0f;
            for (int s = -1; s <= 1; s += 2)
                b.AddBox(new Vector3(px + 0.82f * s, 1.10f, pz), new Vector3(0.12f, 2.20f, 0.12f), WoodColor);
            b.AddBox(new Vector3(px, 2.14f, pz), new Vector3(1.80f, 0.12f, 0.09f), WoodColor);
            b.AddBox(new Vector3(px, 0.96f, pz), new Vector3(1.80f, 0.12f, 0.09f), WoodColor);
            b.AddBox(new Vector3(px, 2.30f, pz + 0.05f), new Vector3(1.90f, 0.08f, 0.55f), TileColor);
            for (int s = -1; s <= 1; s += 2)
            {
                b.AddBox(new Vector3(px + 1.30f * s, 0.20f, pz + 0.35f), new Vector3(0.44f, 0.40f, 0.44f), BrickColor);
                b.AddCylinder(new Vector3(px + 1.30f * s, 0.40f, pz + 0.35f), 0.24f, 0.18f, 0.32f, 8, AzaleaColor);
            }

            return b.ToMesh("VillageProps_Combined");
        }

        // --------------------------------------------------------------------
        // 延边盆地地形高度与色彩采样（含中央平坦村落庭院、布尔哈通河槽与远景长白山余脉）
        // --------------------------------------------------------------------
        public static float SampleTerrainHeight(float x, float z)
        {
            float r = Mathf.Sqrt(x * x + z * z);
            float h = 0f;

            // 外围长白山余脉起伏（r > 38m）
            if (r > 38f)
            {
                float t = Mathf.Clamp01((r - 38f) / 55f);
                float smooth = t * t * (3f - 2f * t);
                float waves = 6.5f + 3.2f * Mathf.Sin(x * 0.055f) * Mathf.Cos(z * 0.055f)
                            + 2.1f * Mathf.Sin(x * 0.11f + z * 0.08f);
                // 北侧远景山脉更高
                float northBoost = z < -25f ? Mathf.Clamp01((-25f - z) / 60f) * 8.5f : 0f;
                h = smooth * (waves + northBoost);
            }

            // 布尔哈通河道下切槽：河中心线 z_river(x) = 28.0 + 2.2 * sin(x * 0.06)
            float zRiver = 28.0f + 2.2f * Mathf.Sin(x * 0.06f);
            float distRiver = Mathf.Abs(z - zRiver);
            // 村口迎宾通道 x ∈ [-5.2, 5.2] 保留平坦步道跨河桥面
            float bridgeMask = Mathf.Clamp01((Mathf.Abs(x) - 4.8f) / 2.2f);
            if (distRiver < 5.5f)
            {
                float u = distRiver / 5.5f;
                float carve = (1f - u * u * (3f - 2f * u)) * 1.15f * bridgeMask;
                h -= carve;
            }
            return h;
        }

        public static Mesh BuildTerrainMesh()
        {
            const int N = 128;
            const float Size = 220f;
            MeshBuildBuffer b = new MeshBuildBuffer();

            for (int iz = 0; iz <= N; ++iz)
            {
                float vz = (float)iz / N;
                float z = (vz - 0.5f) * Size;
                for (int ix = 0; ix <= N; ++ix)
                {
                    float vx = (float)ix / N;
                    float x = (vx - 0.5f) * Size;
                    float y = SampleTerrainHeight(x, z);

                    // 中心差分计算解析地形法线
                    float hx1 = SampleTerrainHeight(x + 0.5f, z);
                    float hx0 = SampleTerrainHeight(x - 0.5f, z);
                    float hz1 = SampleTerrainHeight(x, z + 0.5f);
                    float hz0 = SampleTerrainHeight(x, z - 0.5f);
                    Vector3 n = new Vector3(hx0 - hx1, 1.0f, hz0 - hz1).normalized;

                    // 按高度与区域赋予自然金秋延边大地色彩
                    Color c;
                    if (y < -0.15f)
                        c = new Color(0.52f, 0.46f, 0.38f); // 河床砂石
                    else if (y < 1.2f)
                    {
                        float pathDist = Mathf.Abs(x);
                        if (pathDist < 4.5f && z > -15f && z < 45f)
                            c = new Color(0.68f, 0.63f, 0.55f); // 村口迎宾石板大道
                        else
                            c = new Color(0.76f, 0.64f, 0.44f); // 金秋庭院黄土与草甸
                    }
                    else if (y < 7.5f)
                        c = Color.Lerp(new Color(0.62f, 0.54f, 0.38f), new Color(0.46f, 0.44f, 0.42f), (y - 1.2f) / 6.3f);
                    else
                        c = new Color(0.55f, 0.56f, 0.60f); // 远山岩脊

                    b.verts.Add(new Vector3(x, y, z));
                    b.norms.Add(n);
                    b.uvs.Add(new Vector2(vx * 16f, vz * 16f));
                    b.cols.Add(c);
                }
            }

            int stride = N + 1;
            for (int iz = 0; iz < N; ++iz)
            {
                for (int ix = 0; ix < N; ++ix)
                {
                    int i00 = iz * stride + ix;
                    int i10 = i00 + 1;
                    int i01 = i00 + stride;
                    int i11 = i01 + 1;

                    b.tris.Add(i00); b.tris.Add(i01); b.tris.Add(i11);
                    b.tris.Add(i00); b.tris.Add(i11); b.tris.Add(i10);
                }
            }

            return b.ToMesh("YanbianTerrain");
        }

        public static Mesh BuildRiverWaterMesh()
        {
            const int nx = 96, nz = 24;
            const float widthX = 160f, depthZ = 9.2f;
            MeshBuildBuffer b = new MeshBuildBuffer();

            for (int iz = 0; iz <= nz; ++iz)
            {
                float vz = (float)iz / nz;
                float localZ = (vz - 0.5f) * depthZ;
                for (int ix = 0; ix <= nx; ++ix)
                {
                    float vx = (float)ix / nx;
                    float x = (vx - 0.5f) * widthX;
                    float centerZ = 28.0f + 2.2f * Mathf.Sin(x * 0.06f);
                    b.verts.Add(new Vector3(x, -0.18f, centerZ + localZ));
                    b.norms.Add(Vector3.up);
                    b.uvs.Add(new Vector2(vx * 8f, vz));
                    b.cols.Add(Color.white);
                }
            }

            int stride = nx + 1;
            for (int iz = 0; iz < nz; ++iz)
            {
                for (int ix = 0; ix < nx; ++ix)
                {
                    int i00 = iz * stride + ix;
                    int i10 = i00 + 1;
                    int i01 = i00 + stride;
                    int i11 = i01 + 1;
                    b.tris.Add(i00); b.tris.Add(i01); b.tris.Add(i11);
                    b.tris.Add(i00); b.tris.Add(i11); b.tris.Add(i10);
                }
            }
            return b.ToMesh("BurhatongRiverWater");
        }

        public static Mesh BuildCrossRiceClusterMesh()
        {
            MeshBuildBuffer b = new MeshBuildBuffer();
            Color stalkBottom = new Color(0.68f, 0.55f, 0.18f, 1f);
            Color earTop      = new Color(0.95f, 0.78f, 0.22f, 1f);

            // 3 片 60° 交叉的饱满金黄稻穗簇面片
            for (int k = 0; k < 3; ++k)
            {
                float angle = k * 60f * Mathf.Deg2Rad;
                float dx = Mathf.Cos(angle) * 0.24f;
                float dz = Mathf.Sin(angle) * 0.24f;

                int idx = b.verts.Count;
                b.verts.Add(new Vector3(-dx, 0.00f, -dz));
                b.verts.Add(new Vector3( dx, 0.00f,  dz));
                b.verts.Add(new Vector3( dx * 1.15f, 0.78f,  dz * 1.15f));
                b.verts.Add(new Vector3(-dx * 1.15f, 0.78f, -dz * 1.15f));

                for (int c = 0; c < 4; ++c) b.norms.Add(Vector3.up);
                b.uvs.Add(new Vector2(0, 0)); b.uvs.Add(new Vector2(1, 0));
                b.uvs.Add(new Vector2(1, 1)); b.uvs.Add(new Vector2(0, 1));
                b.cols.Add(stalkBottom); b.cols.Add(stalkBottom);
                b.cols.Add(earTop);      b.cols.Add(earTop);

                b.tris.Add(idx + 0); b.tris.Add(idx + 2); b.tris.Add(idx + 1);
                b.tris.Add(idx + 0); b.tris.Add(idx + 3); b.tris.Add(idx + 2);
            }
            return b.ToMesh("GoldenRiceCluster");
        }
    }
}
