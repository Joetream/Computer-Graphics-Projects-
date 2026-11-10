// ============================================================================
// WP-U2 · Assets/Scripts/CGCore/BezierPatchGenerator.cs
// 双三次 Bézier 参数曲面（16 控制点）与朝鲜族“鹤飞屋”歇山顶 6 片 C1 连续曲面生成器：
//   S(u,v) = \sum_{i=0}^3 \sum_{j=0}^3 B_{i,3}(u) B_{j,3}(v) P_{i,j}
//   N(u,v) = normalize(\partial S / \partial u \times \partial S / \partial v)
// 支持在 Unity 答辩 HUD 中实时拖动檐角起翘高度 cornerLift 与细分步数 steps！
// ============================================================================
using System.Collections.Generic;
using UnityEngine;

namespace YanbianCG.CGCore
{
    public static class BezierPatchGenerator
    {
        public static void EvalBernstein3(float t, out float b0, out float b1, out float b2, out float b3)
        {
            float s = 1f - t;
            b0 = s * s * s;
            b1 = 3f * s * s * t;
            b2 = 3f * s * t * t;
            b3 = t * t * t;
        }

        public static void EvalBernstein3Deriv(float t, out float d0, out float d1, out float d2, out float d3)
        {
            float s = 1f - t;
            d0 = -3f * s * s;
            d1 =  3f * s * s - 6f * s * t;
            d2 =  6f * s * t - 3f * t * t;
            d3 =  3f * t * t;
        }

        public static void EvaluatePatch(Vector3[,] cp, float u, float v, out Vector3 pos, out Vector3 normal)
        {
            float[] bu = new float[4], bv = new float[4];
            float[] du = new float[4], dv = new float[4];
            EvalBernstein3(u, out bu[0], out bu[1], out bu[2], out bu[3]);
            EvalBernstein3(v, out bv[0], out bv[1], out bv[2], out bv[3]);
            EvalBernstein3Deriv(u, out du[0], out du[1], out du[2], out du[3]);
            EvalBernstein3Deriv(v, out dv[0], out dv[1], out dv[2], out dv[3]);

            pos = Vector3.zero;
            Vector3 dSdu = Vector3.zero;
            Vector3 dSdv = Vector3.zero;

            for (int i = 0; i < 4; ++i)
            {
                for (int j = 0; j < 4; ++j)
                {
                    Vector3 p = cp[i, j];
                    pos  += (bu[i] * bv[j]) * p;
                    dSdu += (du[i] * bv[j]) * p;
                    dSdv += (bu[i] * dv[j]) * p;
                }
            }

            // 注意 Unity 左手系中 cross(dSdv, dSdu) 与 OpenGL 右手系朝外法线一致
            Vector3 n = Vector3.Cross(dSdu, dSdv);
            normal = n.sqrMagnitude > 1e-10f ? n.normalized : Vector3.up;
        }

        public static void AppendTessellatedPatch(
            Vector3[,] cp, int steps, Color color, Vector3 offset,
            List<Vector3> verts, List<Vector3> norms, List<Vector2> uvs, List<Color> cols, List<int> tris)
        {
            steps = Mathf.Clamp(steps, 2, 32);
            int baseVertex = verts.Count;
            int stride = steps + 1;

            for (int iu = 0; iu <= steps; ++iu)
            {
                float u = (float)iu / steps;
                for (int iv = 0; iv <= steps; ++iv)
                {
                    float v = (float)iv / steps;
                    EvaluatePatch(cp, u, v, out Vector3 p, out Vector3 n);
                    if (n.y < 0f) n = -n; // 确保屋顶曲面法线朝上
                    verts.Add(p + offset);
                    norms.Add(n);
                    uvs.Add(new Vector2(v * 4f, u * 4f));
                    cols.Add(color);
                }
            }

            for (int iu = 0; iu < steps; ++iu)
            {
                for (int iv = 0; iv < steps; ++iv)
                {
                    int i00 = baseVertex + iu * stride + iv;
                    int i10 = baseVertex + (iu + 1) * stride + iv;
                    int i11 = baseVertex + (iu + 1) * stride + (iv + 1);
                    int i01 = baseVertex + iu * stride + (iv + 1);

                    // 双面或匹配朝向：确保正面朝上可见
                    Vector3 e1 = verts[i10] - verts[i00];
                    Vector3 e2 = verts[i01] - verts[i00];
                    if (Vector3.Dot(Vector3.Cross(e1, e2), norms[i00]) >= 0f)
                    {
                        tris.Add(i00); tris.Add(i10); tris.Add(i11);
                        tris.Add(i00); tris.Add(i11); tris.Add(i01);
                    }
                    else
                    {
                        tris.Add(i00); tris.Add(i11); tris.Add(i10);
                        tris.Add(i00); tris.Add(i01); tris.Add(i11);
                    }
                }
            }
        }

        private static float EaveProfile(float t)
        {
            if (t <= 1f / 3f) return 0f;
            float u = (t - 1f / 3f) / (2f / 3f);
            return u * u * (3f - 2f * u);
        }

        private static void SubdivideEdge(Vector3[] p, float tEnd, Vector3[] q)
        {
            Vector3[] tmp = new Vector3[4] { p[0], p[1], p[2], p[3] };
            q[0] = p[0];
            for (int k = 1; k <= 3; ++k)
            {
                for (int i = 0; i <= 3 - k; ++i)
                    tmp[i] = Vector3.Lerp(tmp[i], tmp[i + 1], tEnd);
                q[k] = tmp[0];
            }
        }

        /// <summary>
        /// 构建朝鲜族“鹤飞屋”歇山顶 6 片双三次 Bézier 曲面（含中缝 C1 连续约束与翼坡 de Casteljau 共边）
        /// </summary>
        public static void AppendKoreanBezierRoof(
            float width, float depth, float height, float cornerLift, int steps, Vector3 roofBaseOffset,
            Color tileColor, Color woodColor,
            List<Vector3> verts, List<Vector3> norms, List<Vector2> uvs, List<Color> cols, List<int> tris)
        {
            float halfW = 0.5f * width;
            float halfD = 0.5f * depth;
            float flare = 0.6f * cornerLift;
            float ridgeHalf = Mathf.Max(halfW - 0.32f * depth, 0.18f * width);

            float[] rowH    = { 0.00f, 0.24f, 0.62f, 1.00f };
            float[] rowD    = { 1.00f, 0.78f, 0.40f, 0.00f };
            float[] rowCurl = { 1.00f, 0.62f, 0.28f, 0.00f };
            float[] rowSpan = { 0.00f, 0.30f, 0.72f, 1.00f };

            Vector3[,] frontL = new Vector3[4, 4];
            Vector3[,] frontR = new Vector3[4, 4];

            for (int i = 0; i < 4; ++i)
            {
                float xEdge = halfW - (halfW - ridgeHalf) * rowSpan[i];
                float yBase = height * rowH[i];
                float lift  = cornerLift * rowCurl[i];
                float sway  = flare * rowCurl[i];
                for (int j = 0; j < 4; ++j)
                {
                    float x = -xEdge + (xEdge / 3f) * j;
                    float s = EaveProfile(Mathf.Abs(x) / halfW);
                    frontL[i, j] = new Vector3(x, yBase + lift * s, -(halfD * rowD[i]) - sway * s);
                }
            }

            // C1 连续条件导出右半坡：P^R_{i,0} = P^L_{i,3}, P^R_{i,1} = 2*P^L_{i,3} - P^L_{i,2}
            for (int i = 0; i < 4; ++i)
            {
                frontR[i, 0] = frontL[i, 3];
                frontR[i, 1] = 2f * frontL[i, 3] - frontL[i, 2];
                frontR[i, 2] = new Vector3(-frontL[i, 1].x, frontL[i, 1].y, frontL[i, 1].z);
                frontR[i, 3] = new Vector3(-frontL[i, 0].x, frontL[i, 0].y, frontL[i, 0].z);
            }

            Vector3[,] backL = new Vector3[4, 4];
            Vector3[,] backR = new Vector3[4, 4];
            for (int i = 0; i < 4; ++i)
                for (int j = 0; j < 4; ++j)
                    backL[i, j] = new Vector3(frontL[3 - i, j].x, frontL[3 - i, j].y, -frontL[3 - i, j].z);
            for (int i = 0; i < 4; ++i)
            {
                backR[i, 0] = backL[i, 3];
                backR[i, 1] = 2f * backL[i, 3] - backL[i, 2];
                backR[i, 2] = new Vector3(-backL[i, 1].x, backL[i, 1].y, backL[i, 1].z);
                backR[i, 3] = new Vector3(-backL[i, 0].x, backL[i, 0].y, backL[i, 0].z);
            }

            AppendTessellatedPatch(frontL, steps, tileColor, roofBaseOffset, verts, norms, uvs, cols, tris);
            AppendTessellatedPatch(frontR, steps, tileColor, roofBaseOffset, verts, norms, uvs, cols, tris);
            AppendTessellatedPatch(backL,  steps, tileColor, roofBaseOffset, verts, norms, uvs, cols, tris);
            AppendTessellatedPatch(backR,  steps, tileColor, roofBaseOffset, verts, norms, uvs, cols, tris);

            // 左右歇山翼坡
            Vector3[] hipFront = new Vector3[4], hipBack = new Vector3[4];
            for (int i = 0; i < 4; ++i)
            {
                hipFront[i] = frontL[i, 0];
                hipBack[i]  = backL[3 - i, 0];
            }
            Vector3[] edgeF = new Vector3[4], edgeB = new Vector3[4];
            SubdivideEdge(hipFront, 0.55f, edgeF);
            SubdivideEdge(hipBack,  0.55f, edgeB);

            Vector3[,] hipL = new Vector3[4, 4];
            Vector3[,] hipR = new Vector3[4, 4];
            for (int j = 0; j < 4; ++j)
            {
                for (int i = 0; i < 4; ++i)
                {
                    float s = i / 3f;
                    Vector3 p = Vector3.Lerp(edgeF[j], edgeB[j], s);
                    if (j == 0 && (i == 1 || i == 2))
                    {
                        p.x = -(halfW + 0.40f * flare);
                        p.y = 0.42f * cornerLift;
                        p.z = edgeF[0].z * (1f - 2f * s);
                    }
                    else if ((j == 1 || j == 2) && (i == 1 || i == 2))
                    {
                        p.x -= 0.12f * flare;
                    }
                    hipL[i, j] = p;
                }
            }
            for (int i = 0; i < 4; ++i)
                for (int j = 0; j < 4; ++j)
                    hipR[i, j] = new Vector3(-hipL[3 - i, j].x, hipL[3 - i, j].y, hipL[3 - i, j].z);

            AppendTessellatedPatch(hipL, steps, tileColor, roofBaseOffset, verts, norms, uvs, cols, tris);
            AppendTessellatedPatch(hipR, steps, tileColor, roofBaseOffset, verts, norms, uvs, cols, tris);
        }
    }
}
