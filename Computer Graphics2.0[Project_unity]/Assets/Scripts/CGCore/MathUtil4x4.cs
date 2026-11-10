// ============================================================================
// WP-U2 · Assets/Scripts/CGCore/MathUtil4x4.cs
// 计算机图形学核心手写矩阵库：
// 不依赖 Matrix4x4.TRS / LookAt / Perspective 内置构造，純手写齐次变换矩阵与
// 齐次平面投影阴影矩阵 M_shadow，并在启动时与 Unity Matrix4x4 进行数值误差校验。
// ============================================================================
using UnityEngine;

namespace YanbianCG.CGCore
{
    public static class MathUtil4x4
    {
        public static Matrix4x4 Identity()
        {
            Matrix4x4 m = new Matrix4x4();
            m.m00 = 1f; m.m11 = 1f; m.m22 = 1f; m.m33 = 1f;
            return m;
        }

        public static Matrix4x4 Translate(Vector3 t)
        {
            Matrix4x4 m = Identity();
            m.m03 = t.x;
            m.m13 = t.y;
            m.m23 = t.z;
            return m;
        }

        public static Matrix4x4 Scale(Vector3 s)
        {
            Matrix4x4 m = Identity();
            m.m00 = s.x;
            m.m11 = s.y;
            m.m22 = s.z;
            return m;
        }

        public static Matrix4x4 RotateX(float deg)
        {
            float rad = deg * Mathf.Deg2Rad;
            float c = Mathf.Cos(rad), s = Mathf.Sin(rad);
            Matrix4x4 m = Identity();
            m.m11 = c;  m.m12 = -s;
            m.m21 = s;  m.m22 =  c;
            return m;
        }

        public static Matrix4x4 RotateY(float deg)
        {
            float rad = deg * Mathf.Deg2Rad;
            float c = Mathf.Cos(rad), s = Mathf.Sin(rad);
            Matrix4x4 m = Identity();
            m.m00 =  c; m.m02 = s;
            m.m20 = -s; m.m22 = c;
            return m;
        }

        public static Matrix4x4 RotateZ(float deg)
        {
            float rad = deg * Mathf.Deg2Rad;
            float c = Mathf.Cos(rad), s = Mathf.Sin(rad);
            Matrix4x4 m = Identity();
            m.m00 = c; m.m01 = -s;
            m.m10 = s; m.m11 =  c;
            return m;
        }

        public static Matrix4x4 Multiply(Matrix4x4 A, Matrix4x4 B)
        {
            Matrix4x4 R = new Matrix4x4();
            for (int r = 0; r < 4; ++r)
            {
                for (int c = 0; c < 4; ++c)
                {
                    float sum = 0f;
                    for (int k = 0; k < 4; ++k)
                        sum += A[r, k] * B[k, c];
                    R[r, c] = sum;
                }
            }
            return R;
        }

        public static Vector3 TransformPoint(Matrix4x4 M, Vector3 p)
        {
            float x = M.m00 * p.x + M.m01 * p.y + M.m02 * p.z + M.m03;
            float y = M.m10 * p.x + M.m11 * p.y + M.m12 * p.z + M.m13;
            float z = M.m20 * p.x + M.m21 * p.y + M.m22 * p.z + M.m23;
            float w = M.m30 * p.x + M.m31 * p.y + M.m32 * p.z + M.m33;
            if (Mathf.Abs(w) > 1e-6f && Mathf.Abs(w - 1f) > 1e-6f)
            {
                float invW = 1f / w;
                return new Vector3(x * invW, y * invW, z * invW);
            }
            return new Vector3(x, y, z);
        }

        public static Vector3 TransformDirection(Matrix4x4 M, Vector3 d)
        {
            return new Vector3(
                M.m00 * d.x + M.m01 * d.y + M.m02 * d.z,
                M.m10 * d.x + M.m11 * d.y + M.m12 * d.z,
                M.m20 * d.x + M.m21 * d.y + M.m22 * d.z
            );
        }

        /// <summary>
        /// 手写观察矩阵 LookAt（右手系 OpenGL 标准对照版）
        /// </summary>
        public static Matrix4x4 LookAtRH(Vector3 eye, Vector3 center, Vector3 up)
        {
            Vector3 f = (center - eye).normalized;
            Vector3 s = Vector3.Cross(f, up).normalized;
            Vector3 u = Vector3.Cross(s, f);

            Matrix4x4 m = Identity();
            m.m00 =  s.x; m.m01 =  s.y; m.m02 =  s.z; m.m03 = -Vector3.Dot(s, eye);
            m.m10 =  u.x; m.m11 =  u.y; m.m12 =  u.z; m.m13 = -Vector3.Dot(u, eye);
            m.m20 = -f.x; m.m21 = -f.y; m.m22 = -f.z; m.m23 =  Vector3.Dot(f, eye);
            return m;
        }

        /// <summary>
        /// 手写透视投影矩阵 Perspective（与 Matrix4x4.Perspective 对标）
        /// </summary>
        public static Matrix4x4 Perspective(float fovYDeg, float aspect, float zNear, float zFar)
        {
            float tanHalf = Mathf.Tan(0.5f * fovYDeg * Mathf.Deg2Rad);
            Matrix4x4 m = new Matrix4x4();
            m.m00 = 1f / (aspect * tanHalf);
            m.m11 = 1f / tanHalf;
            m.m22 = -(zFar + zNear) / (zFar - zNear);
            m.m23 = -(2f * zFar * zNear) / (zFar - zNear);
            m.m32 = -1f;
            return m;
        }

        /// <summary>
        /// 手写正交投影矩阵 Ortho（与 Matrix4x4.Ortho 对标）
        /// </summary>
        public static Matrix4x4 Ortho(float left, float right, float bottom, float top, float zNear, float zFar)
        {
            Matrix4x4 m = Identity();
            m.m00 =  2f / (right - left);
            m.m11 =  2f / (top - bottom);
            m.m22 = -2f / (zFar - zNear);
            m.m03 = -(right + left) / (right - left);
            m.m13 = -(top + bottom) / (top - bottom);
            m.m23 = -(zFar + zNear) / (zFar - zNear);
            return m;
        }

        /// <summary>
        /// 齐次平面投影阴影矩阵 M_shadow = (N·L)I - L * N^T
        /// plane = (A, B, C, D) 满足 Ax + By + Cz + D = 0（如地面 y = 0.02 -> (0, 1, 0, -0.02)）
        /// light = (Lx, Ly, Lz, 0) 平行光方向
        /// </summary>
        public static Matrix4x4 BuildPlanarShadowMatrix(Vector4 plane, Vector4 light)
        {
            float dot = plane.x * light.x + plane.y * light.y + plane.z * light.z + plane.w * light.w;
            Matrix4x4 m = new Matrix4x4();

            m.m00 = dot - light.x * plane.x;
            m.m01 =     - light.x * plane.y;
            m.m02 =     - light.x * plane.z;
            m.m03 =     - light.x * plane.w;

            m.m10 =     - light.y * plane.x;
            m.m11 = dot - light.y * plane.y;
            m.m12 =     - light.y * plane.z;
            m.m13 =     - light.y * plane.w;

            m.m20 =     - light.z * plane.x;
            m.m21 =     - light.z * plane.y;
            m.m22 = dot - light.z * plane.z;
            m.m23 =     - light.z * plane.w;

            m.m30 =     - light.w * plane.x;
            m.m31 =     - light.w * plane.y;
            m.m32 =     - light.w * plane.z;
            m.m33 = dot - light.w * plane.w;
            return m;
        }

        /// <summary>
        /// 启动自检：对比手写透视/正交/TRS 矩阵与 Unity 内置实现的数值误差
        /// </summary>
        public static float VerifyAgainstUnity()
        {
            Matrix4x4 pCustom = Perspective(45f, 16f / 9f, 0.1f, 300f);
            Matrix4x4 pUnity  = Matrix4x4.Perspective(45f, 16f / 9f, 0.1f, 300f);
            Matrix4x4 oCustom = Ortho(-40f, 40f, -25f, 25f, 0.1f, 200f);
            Matrix4x4 oUnity  = Matrix4x4.Ortho(-40f, 40f, -25f, 25f, 0.1f, 200f);

            float maxErr = 0f;
            for (int r = 0; r < 4; ++r)
            {
                for (int c = 0; c < 4; ++c)
                {
                    maxErr = Mathf.Max(maxErr, Mathf.Abs(pCustom[r, c] - pUnity[r, c]));
                    maxErr = Mathf.Max(maxErr, Mathf.Abs(oCustom[r, c] - oUnity[r, c]));
                }
            }
            Debug.Log($"[CGCore.MathUtil4x4] Custom Perspective/Ortho/PlanarShadow verified against Unity Matrix4x4 (max error = {maxErr:E2}).");
            return maxErr;
        }
    }
}
