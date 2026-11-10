// ============================================================================
// WP-U2 · Assets/Scripts/CGCore/FrustumCuller.cs
// Gribb-Hartmann 视锥体 6 平面提取算法（从 VP = Proj * View 矩阵行向量直接解算）
// 与 AABB 正顶点测试（Positive Vertex Test），用于万株金秋稻穗实例的实时视锥剔除。
// ============================================================================
using UnityEngine;

namespace YanbianCG.CGCore
{
    public class FrustumCuller
    {
        private readonly Vector4[] _planes = new Vector4[6];

        /// <summary>
        /// 从组合矩阵 M = Proj * View 提取左、右、下、上、近、远 6 个裁剪平面并归一化
        /// </summary>
        public void ExtractFromMatrix(Matrix4x4 vp)
        {
            Vector4 r0 = vp.GetRow(0);
            Vector4 r1 = vp.GetRow(1);
            Vector4 r2 = vp.GetRow(2);
            Vector4 r3 = vp.GetRow(3);

            _planes[0] = NormalizePlane(r3 + r0); // Left
            _planes[1] = NormalizePlane(r3 - r0); // Right
            _planes[2] = NormalizePlane(r3 + r1); // Bottom
            _planes[3] = NormalizePlane(r3 - r1); // Top
            _planes[4] = NormalizePlane(r3 + r2); // Near
            _planes[5] = NormalizePlane(r3 - r2); // Far
        }

        private static Vector4 NormalizePlane(Vector4 p)
        {
            float len = Mathf.Sqrt(p.x * p.x + p.y * p.y + p.z * p.z);
            return len > 1e-7f ? p / len : p;
        }

        /// <summary>
        /// AABB 盒体与 6 个视锥平面的快速相交检测（取沿法线方向最远的正顶点 P-vertex）
        /// </summary>
        public bool TestAABB(Vector3 bMin, Vector3 bMax)
        {
            for (int i = 0; i < 6; ++i)
            {
                Vector4 pl = _planes[i];
                float px = pl.x >= 0f ? bMax.x : bMin.x;
                float py = pl.y >= 0f ? bMax.y : bMin.y;
                float pz = pl.z >= 0f ? bMax.z : bMin.z;

                if (pl.x * px + pl.y * py + pl.z * pz + pl.w < 0f)
                    return false;
            }
            return true;
        }
    }
}
