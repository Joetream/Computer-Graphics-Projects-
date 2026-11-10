// ============================================================================
// WP-U2 · Assets/Scripts/CGCore/MatrixStack.cs
// 手写矩阵堆栈（Push / Pop / Translate / Rotate / Scale）：
// 驱动风力发电机、秋千、跳板、双轴追日光伏阵列与无人导览电瓶车的树状层级变换。
// ============================================================================
using System.Collections.Generic;
using UnityEngine;

namespace YanbianCG.CGCore
{
    public class MatrixStack
    {
        private readonly List<Matrix4x4> _stack = new List<Matrix4x4>(16);

        public MatrixStack()
        {
            _stack.Add(MathUtil4x4.Identity());
        }

        public void LoadIdentity()
        {
            _stack.Clear();
            _stack.Add(MathUtil4x4.Identity());
        }

        public void Push()
        {
            _stack.Add(_stack[_stack.Count - 1]);
        }

        public void Pop()
        {
            if (_stack.Count > 1)
                _stack.RemoveAt(_stack.Count - 1);
        }

        public Matrix4x4 Top()
        {
            return _stack[_stack.Count - 1];
        }

        public void MultMatrix(Matrix4x4 m)
        {
            int idx = _stack.Count - 1;
            _stack[idx] = MathUtil4x4.Multiply(_stack[idx], m);
        }

        public void Translate(Vector3 t) => MultMatrix(MathUtil4x4.Translate(t));
        public void Translate(float x, float y, float z) => MultMatrix(MathUtil4x4.Translate(new Vector3(x, y, z)));
        public void RotateX(float deg) => MultMatrix(MathUtil4x4.RotateX(deg));
        public void RotateY(float deg) => MultMatrix(MathUtil4x4.RotateY(deg));
        public void RotateZ(float deg) => MultMatrix(MathUtil4x4.RotateZ(deg));
        public void Scale(Vector3 s) => MultMatrix(MathUtil4x4.Scale(s));
        public void Scale(float sx, float sy, float sz) => MultMatrix(MathUtil4x4.Scale(new Vector3(sx, sy, sz)));

        /// <summary>
        /// 将当前栈顶手写齐次矩阵应用至 Unity Transform（位置、旋转、缩放解算）
        /// </summary>
        public void ApplyToTransform(Transform target)
        {
            if (target == null) return;
            Matrix4x4 m = Top();
            Vector3 pos = new Vector3(m.m03, m.m13, m.m23);

            Vector3 col0 = new Vector3(m.m00, m.m10, m.m20);
            Vector3 col1 = new Vector3(m.m01, m.m11, m.m21);
            Vector3 col2 = new Vector3(m.m02, m.m12, m.m22);

            Vector3 scale = new Vector3(col0.magnitude, col1.magnitude, col2.magnitude);
            if (scale.x > 1e-5f && scale.y > 1e-5f && scale.z > 1e-5f)
            {
                Vector3 fwd = col2 / scale.z;
                Vector3 up  = col1 / scale.y;
                target.SetPositionAndRotation(pos, Quaternion.LookRotation(fwd, up));
            }
            else
            {
                target.position = pos;
            }
            target.localScale = scale;
        }
    }
}
