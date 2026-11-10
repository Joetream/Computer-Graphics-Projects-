// ============================================================================
// WP-U2 · Assets/Scripts/CGCore/BSplineTourPath.cs
// 三次均匀闭环 B 样条曲线（Cubic Uniform Closed B-Spline）：
//   b0(u) = (1-u)^3 / 6
//   b1(u) = (3u^3 - 6u^2 + 4) / 6
//   b2(u) = (-3u^3 + 3u^2 + 3u + 1) / 6
//   b3(u) = u^3 / 6
// 用于驱动无人导览电瓶车环村巡游与电影级 B 样条自动运镜。
// ============================================================================
using UnityEngine;

namespace YanbianCG.CGCore
{
    public class BSplineTourPath
    {
        private readonly Vector3[] _waypoints;

        public BSplineTourPath(Vector3[] waypoints = null)
        {
            if (waypoints != null && waypoints.Length >= 4)
            {
                _waypoints = waypoints;
            }
            else
            {
                // 默认环绕延边民俗村核心庭院与河湾的 8 个控制顶点
                _waypoints = new Vector3[]
                {
                    new Vector3( 14f, 0f,  22f),
                    new Vector3( 22f, 0f,   8f),
                    new Vector3( 18f, 0f, -10f),
                    new Vector3(  4f, 0f, -16f),
                    new Vector3(-14f, 0f, -12f),
                    new Vector3(-22f, 0f,   4f),
                    new Vector3(-16f, 0f,  20f),
                    new Vector3(  0f, 0f,  25f)
                };
            }
        }

        public int SegmentCount => _waypoints.Length;

        public Vector3 Evaluate(float t)
        {
            int n = _waypoints.Length;
            float s = ((t % n) + n) % n;
            int seg = Mathf.FloorToInt(s) % n;
            float u = s - Mathf.Floor(s);

            float u2 = u * u, u3 = u2 * u;
            float b0 = (1f - 3f * u + 3f * u2 - u3) / 6f;
            float b1 = (4f - 6f * u2 + 3f * u3) / 6f;
            float b2 = (1f + 3f * u + 3f * u2 - 3f * u3) / 6f;
            float b3 = u3 / 6f;

            Vector3 p0 = _waypoints[(seg + 0) % n];
            Vector3 p1 = _waypoints[(seg + 1) % n];
            Vector3 p2 = _waypoints[(seg + 2) % n];
            Vector3 p3 = _waypoints[(seg + 3) % n];

            return b0 * p0 + b1 * p1 + b2 * p2 + b3 * p3;
        }

        public Vector3 EvaluateTangent(float t)
        {
            int n = _waypoints.Length;
            float s = ((t % n) + n) % n;
            int seg = Mathf.FloorToInt(s) % n;
            float u = s - Mathf.Floor(s);

            float u2 = u * u;
            float d0 = (-3f + 6f * u - 3f * u2) / 6f;
            float d1 = (-12f * u + 9f * u2) / 6f;
            float d2 = (3f + 6f * u - 9f * u2) / 6f;
            float d3 = (3f * u2) / 6f;

            Vector3 p0 = _waypoints[(seg + 0) % n];
            Vector3 p1 = _waypoints[(seg + 1) % n];
            Vector3 p2 = _waypoints[(seg + 2) % n];
            Vector3 p3 = _waypoints[(seg + 3) % n];

            Vector3 tan = d0 * p0 + d1 * p1 + d2 * p2 + d3 * p3;
            return tan.sqrMagnitude > 1e-8f ? tan.normalized : Vector3.forward;
        }
    }
}
