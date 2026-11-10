// ============================================================================
// WP-U4 · Assets/Scripts/Scene/HierarchyAnimator.cs
// 树状多层级建模与关节动画控制器（严格使用手写 MatrixStack.Push / Pop 驱动）：
//   1. 3 座现代风力发电机（塔筒 -> 机舱 -> 轮毂 -> 3 枚变桨旋转叶片）
//   2. 朝鲜族民俗秋千（A 字木架 -> 左右双秋千绳 -> 踏板 -> 民俗少女躯干与小腿反向摆动）
//   3. 朝鲜族民俗跳板（支点木墩 -> 跷跷板长木 -> 左右两名民俗舞者交替凌空跃起）
//   4. 8 组双轴追日光伏阵列（立柱 -> 方位角旋转架 -> 仰角追踪太阳的深蓝硅晶板）
//   5. 无人导览电瓶车（沿三次均匀闭环 B 样条曲线巡游，4 个车轮按弧长自转）
// ============================================================================
using System.Collections.Generic;
using UnityEngine;
using YanbianCG.CGCore;

namespace YanbianCG.Scene
{
    public class HierarchyAnimator : MonoBehaviour
    {
        private class PartNode
        {
            public Transform transform;
            public Vector3 meshSize;
        }

        private Mesh _unitBoxMesh;
        private Material _sharedMat;
        private readonly MatrixStack _stack = new MatrixStack();
        private readonly BSplineTourPath _tourPath = new BSplineTourPath();

        // 1. 风车节点 (3 座 × (塔筒 + 机舱 + 3叶片))
        private readonly Vector3[] _windmillPositions =
        {
            new Vector3(-32f, 0f, -28f),
            new Vector3(  2f, 0f, -38f),
            new Vector3( 34f, 0f, -26f)
        };
        private readonly List<Transform> _wmTowers = new List<Transform>();
        private readonly List<Transform> _wmHubs   = new List<Transform>();
        private readonly List<Transform[]> _wmBlades = new List<Transform[]>();

        // 2. 民俗秋千节点 (8, 0, -3)
        private readonly Vector3 _swingBase = new Vector3(8.5f, 0f, -3.0f);
        private readonly List<Transform> _swingPillars = new List<Transform>();
        private Transform _swingTopBar, _swingRopeL, _swingRopeR, _swingBoard, _swingTorso, _swingLegs;

        // 3. 民俗跳板节点 (13, 0, 5)
        private readonly Vector3 _seesawBase = new Vector3(13.5f, 0f, 5.0f);
        private Transform _seesawFulcrum, _seesawPlank, _dancerLeft, _dancerRight;

        // 4. 追日光伏板节点 (8 组)
        private readonly List<Vector3> _solarPositions = new List<Vector3>();
        private readonly List<Transform> _solarPoles  = new List<Transform>();
        private readonly List<Transform> _solarPanels = new List<Transform>();

        // 5. 无人导览电瓶车节点
        private Transform _cartChassis, _cartRoof;
        private readonly Transform[] _cartWheels = new Transform[4];
        private readonly Transform[] _cartPosts  = new Transform[4];
        private float _cartSplineT = 0f;

        public Vector3 CurrentSunDir { get; set; } = new Vector3(0.6f, 0.4f, -0.5f).normalized;

        public void Initialize(Material uberMat)
        {
            _sharedMat = uberMat;
            MeshBuildBuffer mb = new MeshBuildBuffer();
            mb.AddBox(Vector3.zero, Vector3.one, Color.white);
            _unitBoxMesh = mb.ToMesh("UnitHierarchyBox");

            BuildWindmills();
            BuildSwing();
            BuildSeesaw();
            BuildSolarArray();
            BuildTourCart();
            UpdateHierarchy(0f);
        }

        private Transform CreatePart(string name, Color color)
        {
            GameObject go = new GameObject(name);
            go.transform.SetParent(transform, false);
            MeshFilter mf = go.AddComponent<MeshFilter>();
            MeshRenderer mr = go.AddComponent<MeshRenderer>();

            MeshBuildBuffer mb = new MeshBuildBuffer();
            mb.AddBox(Vector3.zero, Vector3.one, color);
            mf.sharedMesh = mb.ToMesh(name + "_Mesh");
            mr.sharedMaterial = _sharedMat;
            mr.shadowCastingMode = UnityEngine.Rendering.ShadowCastingMode.On;
            mr.receiveShadows = true;
            return go.transform;
        }

        private void BuildWindmills()
        {
            Color whiteSteel = new Color(0.92f, 0.94f, 0.96f);
            Color hubGray    = new Color(0.70f, 0.74f, 0.78f);
            for (int i = 0; i < _windmillPositions.Length; ++i)
            {
                _wmTowers.Add(CreatePart($"Windmill_{i}_Tower", whiteSteel));
                _wmHubs.Add(CreatePart($"Windmill_{i}_Hub", hubGray));
                Transform[] blades = new Transform[3];
                for (int b = 0; b < 3; ++b)
                    blades[b] = CreatePart($"Windmill_{i}_Blade_{b}", whiteSteel);
                _wmBlades.Add(blades);
            }
        }

        private void BuildSwing()
        {
            Color wood  = ProceduralMeshLib.WoodColor;
            Color rope  = new Color(0.76f, 0.66f, 0.46f);
            Color dress = new Color(0.92f, 0.36f, 0.48f); // 朝鲜族红裙（Chima）
            Color jeogori = new Color(0.96f, 0.94f, 0.88f); // 象牙白短衣（Jeogori）

            for (int i = 0; i < 4; ++i)
                _swingPillars.Add(CreatePart($"Swing_Pillar_{i}", wood));
            _swingTopBar = CreatePart("Swing_TopBar", wood);
            _swingRopeL  = CreatePart("Swing_RopeL", rope);
            _swingRopeR  = CreatePart("Swing_RopeR", rope);
            _swingBoard  = CreatePart("Swing_Board", wood);
            _swingTorso  = CreatePart("Swing_PerformerTorso", jeogori);
            _swingLegs   = CreatePart("Swing_PerformerSkirtLegs", dress);
        }

        private void BuildSeesaw()
        {
            Color wood = ProceduralMeshLib.WoodColor;
            Color pink = new Color(0.92f, 0.48f, 0.64f);
            Color blue = new Color(0.32f, 0.56f, 0.86f);

            _seesawFulcrum = CreatePart("Seesaw_Fulcrum", wood);
            _seesawPlank   = CreatePart("Seesaw_Plank", new Color(0.68f, 0.48f, 0.28f));
            _dancerLeft    = CreatePart("Seesaw_DancerL", pink);
            _dancerRight   = CreatePart("Seesaw_DancerR", blue);
        }

        private void BuildSolarArray()
        {
            Color poleGray  = new Color(0.55f, 0.58f, 0.62f);
            Color solarBlue = new Color(0.12f, 0.24f, 0.48f);
            for (int row = 0; row < 2; ++row)
            {
                for (int col = 0; col < 4; ++col)
                {
                    float x = -32f + col * 5.2f;
                    float z = -12f - row * 4.8f;
                    _solarPositions.Add(new Vector3(x, ProceduralMeshLib.SampleTerrainHeight(x, z), z));
                    _solarPoles.Add(CreatePart($"Solar_{row}_{col}_Pole", poleGray));
                    _solarPanels.Add(CreatePart($"Solar_{row}_{col}_Panel", solarBlue));
                }
            }
        }

        private void BuildTourCart()
        {
            Color bodyCol  = new Color(0.82f, 0.28f, 0.22f);
            Color roofCol  = new Color(0.95f, 0.90f, 0.78f);
            Color wheelCol = new Color(0.18f, 0.18f, 0.20f);
            Color postCol  = new Color(0.35f, 0.25f, 0.18f);

            _cartChassis = CreatePart("TourCart_Chassis", bodyCol);
            _cartRoof    = CreatePart("TourCart_Canopy", roofCol);
            for (int i = 0; i < 4; ++i)
            {
                _cartPosts[i]  = CreatePart($"TourCart_Post_{i}", postCol);
                _cartWheels[i] = CreatePart($"TourCart_Wheel_{i}", wheelCol);
            }
        }

        private void Update()
        {
            UpdateHierarchy(Time.time);
        }

        public void UpdateHierarchy(float t)
        {
            // ----------------------------------------------------------------
            // 1. 3 座风力发电机（塔高 14m，叶轮随时间旋转，层级：Root -> Tower -> Hub -> 3 Blades）
            // ----------------------------------------------------------------
            for (int i = 0; i < _windmillPositions.Length; ++i)
            {
                Vector3 basePos = _windmillPositions[i];
                basePos.y = ProceduralMeshLib.SampleTerrainHeight(basePos.x, basePos.z);

                _stack.LoadIdentity();
                _stack.Translate(basePos);

                // Tower
                _stack.Push();
                _stack.Translate(0f, 7.0f, 0f);
                _stack.Scale(0.75f, 14.0f, 0.75f);
                _stack.ApplyToTransform(_wmTowers[i]);
                _stack.Pop();

                // Hub at top of tower
                _stack.Push();
                _stack.Translate(0f, 14.0f, 0f);

                _stack.Push();
                _stack.Translate(0f, 0f, 0.45f);
                _stack.Scale(1.1f, 0.9f, 1.6f);
                _stack.ApplyToTransform(_wmHubs[i]);
                _stack.Pop();

                // 3 Rotating Blades
                float spinDeg = t * 52f + i * 40f;
                for (int b = 0; b < 3; ++b)
                {
                    _stack.Push();
                    _stack.Translate(0f, 0f, 1.15f);
                    _stack.RotateZ(spinDeg + b * 120f);
                    _stack.Translate(0f, 3.1f, 0f);
                    _stack.Scale(0.32f, 6.2f, 0.08f);
                    _stack.ApplyToTransform(_wmBlades[i][b]);
                    _stack.Pop();
                }
                _stack.Pop();
            }

            // ----------------------------------------------------------------
            // 2. 朝鲜族民俗秋千（Root -> 横梁 -> 双绳摆角 θ(t) -> 踏板 -> 舞者躯干与小腿反向摆动）
            // ----------------------------------------------------------------
            {
                _stack.LoadIdentity();
                _stack.Translate(_swingBase);

                // A字四立柱与横梁
                float[] sx = { -1.35f, -1.35f, 1.35f, 1.35f };
                float[] sz = { -0.95f,  0.95f, -0.95f,  0.95f };
                float[] tiltX = {  14f, -14f,  14f, -14f };
                for (int k = 0; k < 4; ++k)
                {
                    _stack.Push();
                    _stack.Translate(sx[k], 1.90f, sz[k] * 0.5f);
                    _stack.RotateX(tiltX[k]);
                    _stack.Scale(0.14f, 3.95f, 0.14f);
                    _stack.ApplyToTransform(_swingPillars[k]);
                    _stack.Pop();
                }

                _stack.Push();
                _stack.Translate(0f, 3.72f, 0f);
                _stack.Scale(3.10f, 0.18f, 0.18f);
                _stack.ApplyToTransform(_swingTopBar);
                _stack.Pop();

                // 摆动挂点 (y = 3.72)
                float swingAngle = 32f * Mathf.Sin(t * 2.2f);
                _stack.Push();
                _stack.Translate(0f, 3.72f, 0f);
                _stack.RotateX(swingAngle);

                // 左右双绳 (长 2.6m)
                _stack.Push();
                _stack.Translate(-0.45f, -1.30f, 0f);
                _stack.Scale(0.04f, 2.60f, 0.04f);
                _stack.ApplyToTransform(_swingRopeL);
                _stack.Pop();

                _stack.Push();
                _stack.Translate(0.45f, -1.30f, 0f);
                _stack.Scale(0.04f, 2.60f, 0.04f);
                _stack.ApplyToTransform(_swingRopeR);
                _stack.Pop();

                // 秋千踏板 (y = -2.60)
                _stack.Translate(0f, -2.60f, 0f);
                _stack.Push();
                _stack.Scale(1.05f, 0.07f, 0.36f);
                _stack.ApplyToTransform(_swingBoard);
                _stack.Pop();

                // 踏板上的朝鲜族少女躯干与反向摆动裙摆小腿
                _stack.Push();
                _stack.Translate(0f, 0.72f, 0f);
                _stack.Scale(0.36f, 0.75f, 0.24f);
                _stack.ApplyToTransform(_swingTorso);
                _stack.Pop();

                _stack.Push();
                _stack.Translate(0f, 0.28f, 0f);
                _stack.RotateX(-0.45f * swingAngle);
                _stack.Scale(0.46f, 0.55f, 0.34f);
                _stack.ApplyToTransform(_swingLegs);
                _stack.Pop();

                _stack.Pop();
            }

            // ----------------------------------------------------------------
            // 3. 朝鲜族民俗跳板（Root -> 支点 -> 翘板倾角 α(t) -> 左右舞者交替凌空跳跃）
            // ----------------------------------------------------------------
            {
                _stack.LoadIdentity();
                _stack.Translate(_seesawBase);

                _stack.Push();
                _stack.Translate(0f, 0.22f, 0f);
                _stack.Scale(0.45f, 0.44f, 0.75f);
                _stack.ApplyToTransform(_seesawFulcrum);
                _stack.Pop();

                float seesawPhase = t * 2.8f;
                float tiltDeg = 13f * Mathf.Sin(seesawPhase);

                _stack.Push();
                _stack.Translate(0f, 0.46f, 0f);
                _stack.RotateZ(tiltDeg);
                _stack.Scale(3.80f, 0.09f, 0.48f);
                _stack.ApplyToTransform(_seesawPlank);
                _stack.Pop();

                // 左右跳板舞者：当翘板一端向上弹起时凌空跃起
                float jumpL = Mathf.Max(0f,  Mathf.Sin(seesawPhase)) * 1.15f;
                float jumpR = Mathf.Max(0f, -Mathf.Sin(seesawPhase)) * 1.15f;
                float boardYL = 0.46f - 1.55f * Mathf.Sin(tiltDeg * Mathf.Deg2Rad);
                float boardYR = 0.46f + 1.55f * Mathf.Sin(tiltDeg * Mathf.Deg2Rad);

                _stack.Push();
                _stack.Translate(-1.55f, boardYL + 0.58f + jumpL, 0f);
                _stack.Scale(0.38f, 1.12f, 0.32f);
                _stack.ApplyToTransform(_dancerLeft);
                _stack.Pop();

                _stack.Push();
                _stack.Translate(1.55f, boardYR + 0.58f + jumpR, 0f);
                _stack.Scale(0.38f, 1.12f, 0.32f);
                _stack.ApplyToTransform(_dancerRight);
                _stack.Pop();
            }

            // ----------------------------------------------------------------
            // 4. 双轴追日光伏阵列（根据太阳方向向量 CurrentSunDir 实时解算方位角与仰角）
            // ----------------------------------------------------------------
            Vector3 sun = CurrentSunDir.normalized;
            float targetYawDeg   = Mathf.Atan2(sun.x, sun.z) * Mathf.Rad2Deg;
            float targetPitchDeg = Mathf.Clamp(90f - Mathf.Asin(Mathf.Clamp(sun.y, 0.05f, 1f)) * Mathf.Rad2Deg, 15f, 58f);

            for (int i = 0; i < _solarPositions.Count; ++i)
            {
                _stack.LoadIdentity();
                _stack.Translate(_solarPositions[i]);

                _stack.Push();
                _stack.Translate(0f, 0.75f, 0f);
                _stack.Scale(0.16f, 1.50f, 0.16f);
                _stack.ApplyToTransform(_solarPoles[i]);
                _stack.Pop();

                _stack.Push();
                _stack.Translate(0f, 1.55f, 0f);
                _stack.RotateY(targetYawDeg);
                _stack.RotateX(targetPitchDeg);
                _stack.Scale(2.40f, 0.08f, 1.55f);
                _stack.ApplyToTransform(_solarPanels[i]);
                _stack.Pop();
            }

            // ----------------------------------------------------------------
            // 5. 无人导览电瓶车（沿三次均匀闭环 B 样条曲线巡游 + 四轮同步自转）
            // ----------------------------------------------------------------
            _cartSplineT = (t * 0.24f) % _tourPath.SegmentCount;
            Vector3 cartPos = _tourPath.Evaluate(_cartSplineT);
            cartPos.y = ProceduralMeshLib.SampleTerrainHeight(cartPos.x, cartPos.z);
            Vector3 cartFwd = _tourPath.EvaluateTangent(_cartSplineT);
            float cartYaw = Mathf.Atan2(cartFwd.x, cartFwd.z) * Mathf.Rad2Deg;
            float wheelSpin = t * 240f;

            _stack.LoadIdentity();
            _stack.Translate(cartPos);
            _stack.RotateY(cartYaw);

            // 底盘
            _stack.Push();
            _stack.Translate(0f, 0.52f, 0f);
            _stack.Scale(1.45f, 0.42f, 2.75f);
            _stack.ApplyToTransform(_cartChassis);
            _stack.Pop();

            // 顶棚
            _stack.Push();
            _stack.Translate(0f, 1.82f, 0f);
            _stack.Scale(1.52f, 0.10f, 2.85f);
            _stack.ApplyToTransform(_cartRoof);
            _stack.Pop();

            // 四根立柱与四个旋转车轮
            float[] wx = { -0.68f,  0.68f, -0.68f,  0.68f };
            float[] wz = { -0.92f, -0.92f,  0.92f,  0.92f };
            for (int w = 0; w < 4; ++w)
            {
                _stack.Push();
                _stack.Translate(wx[w] * 0.92f, 1.18f, wz[w] * 1.15f);
                _stack.Scale(0.07f, 1.18f, 0.07f);
                _stack.ApplyToTransform(_cartPosts[w]);
                _stack.Pop();

                _stack.Push();
                _stack.Translate(wx[w], 0.30f, wz[w]);
                _stack.RotateX(wheelSpin);
                _stack.Scale(0.18f, 0.56f, 0.56f);
                _stack.ApplyToTransform(_cartWheels[w]);
                _stack.Pop();
            }
        }
    }
}
