// ============================================================================
// WP-U4/U5 · Assets/Scripts/Scene/YanbianSceneBootstrapper.cs
// 延边朝鲜族民俗村 2.0 全自动场景自举组装器：
//   · 挂有 [RuntimeInitializeOnLoadMethod]，无论打开任何场景直接按 Play 或运行独立 EXE，
//     均自动完成：手写矩阵校验 -> 地形与布尔哈通河 -> 6栋双三次 Bézier 飞檐民居 ->
//     四柱迎宾牌楼与中朝双语横匾 -> 百年水井与 2D 光栅化沙盘木台 -> 万株实例化稻田 ->
//     风车/秋千/跳板/追日光伏/导览车层级动画 -> Seedream 5.5 三态天光与答辩 HUD！
// ============================================================================
using System.Collections;
using System.Collections.Generic;
using System.IO;
using UnityEngine;
using YanbianCG.CGCore;
using YanbianCG.FX;
using YanbianCG.UI;

namespace YanbianCG.Scene
{
    public class YanbianSceneBootstrapper : MonoBehaviour
    {
        public struct HousePlacement
        {
            public Vector3 pos;
            public float yawDeg;
            public Vector3 scale;
            public bool hasChimney;
        }

        private static YanbianSceneBootstrapper _instance;

        [RuntimeInitializeOnLoadMethod(RuntimeInitializeLoadType.AfterSceneLoad)]
        private static void AutoBootstrapOnPlay()
        {
            if (_instance != null || FindFirstObjectByType<YanbianSceneBootstrapper>() != null)
                return;

            GameObject root = new GameObject("YanbianVillage_2.0_Root");
            _instance = root.AddComponent<YanbianSceneBootstrapper>();
        }

        private readonly HousePlacement[] _housePlacements =
        {
            new HousePlacement { pos = new Vector3(-12.0f, 0f,   8.0f), yawDeg =  18f, scale = Vector3.one, hasChimney = true  },
            new HousePlacement { pos = new Vector3( 12.5f, 0f,   9.0f), yawDeg = -22f, scale = Vector3.one, hasChimney = true  },
            new HousePlacement { pos = new Vector3(-14.5f, 0f,  -6.5f), yawDeg =  10f, scale = Vector3.one, hasChimney = false },
            new HousePlacement { pos = new Vector3(  0.0f, 0f, -11.5f), yawDeg =   0f, scale = new Vector3(1.12f, 1.08f, 1.08f), hasChimney = true },
            new HousePlacement { pos = new Vector3( 15.0f, 0f,  -7.5f), yawDeg = -12f, scale = Vector3.one, hasChimney = false },
            new HousePlacement { pos = new Vector3( -2.5f, 0f, -22.0f), yawDeg =   5f, scale = new Vector3(0.95f, 0.95f, 0.95f), hasChimney = true }
        };

        private Material _uberMat;
        private Material _waterMat;
        private Material _plaqueMat;
        private Material _guideSignMat;
        private Material _sandboxTopMat;

        private readonly List<MeshFilter> _houseFilters = new List<MeshFilter>();
        private MeshFilter _gateFilter;
        private MeshFilter _propsFilter;

        public Raster2DSandbox Sandbox2D { get; private set; }
        public TimeOfDayController TimeController { get; private set; }
        public HierarchyAnimator HierarchyAnim { get; private set; }
        public RiceFieldInstancer RiceField { get; private set; }
        public CameraPostProcessor CamPost { get; private set; }

        public float BezierCornerLift { get; private set; } = 0.55f;
        public int BezierTessLevel { get; private set; } = 1; // 0=4, 1=10, 2=20
        public float MathMaxError { get; private set; }

        private void Awake()
        {
            if (_instance != null && _instance != this)
            {
                Destroy(gameObject);
                return;
            }
            _instance = this;
            BuildCompleteVillageScene();
        }

        public void BuildCompleteVillageScene()
        {
            // 1. 启动手写矩阵与 Unity Matrix4x4 数值误差自检
            MathMaxError = MathUtil4x4.VerifyAgainstUnity();

            // 2. 初始化 2D 光栅化与裁剪沙盘 (Bresenham / Scanline / Midpoint Circle / Cohen-Sutherland)
            Sandbox2D = new Raster2DSandbox();

            // 3. 加载自定义 HLSL 课程着色器与贴图
            Shader uberShader = Resources.Load<Shader>("Shaders/CG_UberLit");
            if (uberShader == null) uberShader = Shader.Find("YanbianCG/CG_UberLit");
            _uberMat = new Material(uberShader) { name = "Mat_CG_UberScene" };
            _uberMat.SetFloat("_UseTex", 0f);
            _uberMat.SetFloat("_Shininess", 36f);

            Shader waterShader = Resources.Load<Shader>("Shaders/CG_RiverWater");
            if (waterShader == null) waterShader = Shader.Find("YanbianCG/CG_RiverWater");
            _waterMat = new Material(waterShader) { name = "Mat_CG_BurhatongWater" };

            // 4. 配置主相机与后处理管线
            Camera mainCam = Camera.main;
            if (mainCam == null)
            {
                GameObject camGo = new GameObject("Main Camera");
                camGo.tag = "MainCamera";
                mainCam = camGo.AddComponent<Camera>();
            }
            // 清理场景中可能存在的默认多余平行光
            foreach (var l in FindObjectsByType<Light>(FindObjectsSortMode.None))
            {
                if (l.transform.parent == null) DestroyImmediate(l.gameObject);
            }

            CamPost = mainCam.GetComponent<CameraPostProcessor>();
            if (CamPost == null) CamPost = mainCam.gameObject.AddComponent<CameraPostProcessor>();

            // 5. 构建延边盆地地形与布尔哈通河湾水面
            CreateStaticMeshObject("Terrain_YanbianValley", ProceduralMeshLib.BuildTerrainMesh(), _uberMat, Vector3.zero, 0f, Vector3.one, castShadow: false);
            CreateStaticMeshObject("Water_BurhatongRiver", ProceduralMeshLib.BuildRiverWaterMesh(), _waterMat, Vector3.zero, 0f, Vector3.one, castShadow: false);

            // 6. 构建 6 栋朝鲜族传统民居（含双三次 Bézier C1 飞檐与独立火炕烟囱）
            List<Vector3> chimneyTops = new List<Vector3>();
            int steps = StepsFromLevel(BezierTessLevel);
            Mesh houseWithChimney = ProceduralMeshLib.BuildKoreanHouseMesh(BezierCornerLift, steps, true);
            Mesh houseNoChimney   = ProceduralMeshLib.BuildKoreanHouseMesh(BezierCornerLift, steps, false);

            _houseFilters.Clear();
            for (int i = 0; i < _housePlacements.Length; ++i)
            {
                HousePlacement hp = _housePlacements[i];
                // 注意将 OpenGL 右手系 X 镜像为左手系对称布局
                Vector3 unityPos = new Vector3(-hp.pos.x, hp.pos.y, hp.pos.z);
                float unityYaw = 180f - hp.yawDeg;

                Mesh m = hp.hasChimney ? houseWithChimney : houseNoChimney;
                MeshFilter mf = CreateStaticMeshObject($"KoreanHouse_{i}", m, _uberMat, unityPos, unityYaw, hp.scale, castShadow: true);
                _houseFilters.Add(mf);
                CamPost.ShadowCasters.Add(mf);

                if (hp.hasChimney)
                {
                    Quaternion rot = Quaternion.Euler(0f, unityYaw, 0f);
                    Vector3 chimneyLocal = new Vector3(4.5f, 4.46f, -1.2f);
                    chimneyTops.Add(unityPos + rot * Vector3.Scale(chimneyLocal, hp.scale));
                }
            }

            // 7. 构建村口四柱迎宾牌楼 + 中朝双语《延边欢迎您》横匾
            Mesh gateMesh = ProceduralMeshLib.BuildWelcomeGateMesh(BezierCornerLift * 0.88f, steps);
            _gateFilter = CreateStaticMeshObject("WelcomeGate_Main", gateMesh, _uberMat, new Vector3(0f, 0f, 32f), 0f, Vector3.one, castShadow: true);
            CamPost.ShadowCasters.Add(_gateFilter);

            BuildTexturedQuads();

            // 8. 构建广场百年老水井、酱缸台、2D 沙盘木台与导览牌框
            _propsFilter = CreateStaticMeshObject("VillageProps", ProceduralMeshLib.BuildVillagePropsMesh(10), _uberMat, Vector3.zero, 0f, Vector3.one, castShadow: true);
            CamPost.ShadowCasters.Add(_propsFilter);

            // 9. 初始化层级建模动画（风车、秋千、跳板、追日光伏板、无人导览车）
            GameObject hierGo = new GameObject("HierarchyObjects_Animated");
            hierGo.transform.SetParent(transform, false);
            HierarchyAnim = hierGo.AddComponent<HierarchyAnimator>();
            HierarchyAnim.Initialize(_uberMat);

            // 10. 初始化万株金秋稻浪 GPU Instancing + 视锥剔除器
            GameObject riceGo = new GameObject("RiceField_GPUInstanced");
            riceGo.transform.SetParent(transform, false);
            RiceField = riceGo.AddComponent<RiceFieldInstancer>();
            RiceField.Initialize(_uberMat);

            // 11. 初始化三态天光、Seedream 5.5 天空盒与炊烟/瑞雪粒子系统
            GameObject fxGo = new GameObject("TimeOfDay_LightingAndSky");
            fxGo.transform.SetParent(transform, false);
            TimeController = fxGo.AddComponent<TimeOfDayController>();
            TimeController.Initialize(_uberMat, chimneyTops);

            // 12. 挂载交互式答辩 HUD 控制台
            GameObject hudGo = new GameObject("DefenseHUD_Controller");
            hudGo.transform.SetParent(transform, false);
            DefenseHudController hud = hudGo.AddComponent<DefenseHudController>();
            hud.Initialize(this);

            // 若命令行传入 --capture-unity，则自动抓取三态对比实机渲染图
            string[] args = System.Environment.GetCommandLineArgs();
            foreach (string a in args)
            {
                if (a == "--capture-unity")
                {
                    StartCoroutine(AutoCaptureSuiteCoroutine());
                    break;
                }
            }
        }

        private void BuildTexturedQuads()
        {
            // 1) 迎宾牌楼正中央双语横匾 (sign_gate.png)
            Texture2D gateTex = LoadArchitectureTexture("sign_gate");
            _plaqueMat = new Material(_uberMat) { name = "Mat_WelcomePlaque" };
            _plaqueMat.SetFloat("_UseTex", 1f);
            _plaqueMat.mainTexture = gateTex;

            GameObject plaqueGo = GameObject.CreatePrimitive(PrimitiveType.Quad);
            plaqueGo.name = "WelcomeGate_BilingualPlaque";
            plaqueGo.transform.SetParent(transform, false);
            plaqueGo.transform.position = new Vector3(0f, 2.90f, 32.62f);
            plaqueGo.transform.rotation = Quaternion.Euler(0f, 180f, 0f);
            plaqueGo.transform.localScale = new Vector3(4.8f, 1.2f, 1f);
            DestroyImmediate(plaqueGo.GetComponent<Collider>());
            plaqueGo.GetComponent<MeshRenderer>().sharedMaterial = _plaqueMat;

            // 2) 2D 光栅化与裁剪沙盘动态贴图台面 (-7.5, 0.91, 18)
            _sandboxTopMat = new Material(_uberMat) { name = "Mat_Sandbox2DTop" };
            _sandboxTopMat.SetFloat("_UseTex", 1f);
            _sandboxTopMat.mainTexture = Sandbox2D.Texture;

            GameObject sbQuad = GameObject.CreatePrimitive(PrimitiveType.Quad);
            sbQuad.name = "Sandbox2D_DynamicSurface";
            sbQuad.transform.SetParent(transform, false);
            sbQuad.transform.position = new Vector3(-7.5f, 0.86f, 18.0f);
            sbQuad.transform.rotation = Quaternion.Euler(90f, 0f, 0f);
            sbQuad.transform.localScale = new Vector3(1.86f, 1.36f, 1f);
            DestroyImmediate(sbQuad.GetComponent<Collider>());
            sbQuad.GetComponent<MeshRenderer>().sharedMaterial = _sandboxTopMat;

            // 3) 村口旅游导览图立牌正面 (sign_guide.png)
            Texture2D guideTex = LoadArchitectureTexture("sign_guide");
            _guideSignMat = new Material(_uberMat) { name = "Mat_GuideSignBoard" };
            _guideSignMat.SetFloat("_UseTex", 1f);
            _guideSignMat.mainTexture = guideTex;

            GameObject guideQuad = GameObject.CreatePrimitive(PrimitiveType.Quad);
            guideQuad.name = "GuideSign_FrontQuad";
            guideQuad.transform.SetParent(transform, false);
            guideQuad.transform.position = new Vector3(7.5f, 1.55f, 22.07f);
            guideQuad.transform.rotation = Quaternion.Euler(0f, 180f, 0f);
            guideQuad.transform.localScale = new Vector3(1.45f, 1.05f, 1f);
            DestroyImmediate(guideQuad.GetComponent<Collider>());
            guideQuad.GetComponent<MeshRenderer>().sharedMaterial = _guideSignMat;
        }

        public static Texture2D LoadArchitectureTexture(string baseName)
        {
            string filePath = Path.Combine(Application.dataPath, "Resources", "Textures", "Architecture", baseName + ".png");
            if (File.Exists(filePath))
            {
                byte[] bytes = File.ReadAllBytes(filePath);
                Texture2D tex = new Texture2D(2, 2, TextureFormat.RGBA32, true);
                tex.LoadImage(bytes);
                tex.wrapMode = TextureWrapMode.Clamp;
                return tex;
            }
            return Resources.Load<Texture2D>("Textures/Architecture/" + baseName);
        }

        private MeshFilter CreateStaticMeshObject(string objName, Mesh mesh, Material mat, Vector3 pos, float yawDeg, Vector3 scale, bool castShadow)
        {
            GameObject go = new GameObject(objName);
            go.transform.SetParent(transform, false);
            go.transform.localPosition = pos;
            go.transform.localRotation = Quaternion.Euler(0f, yawDeg, 0f);
            go.transform.localScale = scale;

            MeshFilter mf = go.AddComponent<MeshFilter>();
            MeshRenderer mr = go.AddComponent<MeshRenderer>();
            mf.sharedMesh = mesh;
            mr.sharedMaterial = mat;
            mr.shadowCastingMode = castShadow
                ? UnityEngine.Rendering.ShadowCastingMode.On
                : UnityEngine.Rendering.ShadowCastingMode.Off;
            mr.receiveShadows = true;
            return mf;
        }

        public static int StepsFromLevel(int level)
        {
            if (level <= 0) return 4;
            if (level == 1) return 10;
            return 20;
        }

        /// <summary>
        /// 实时调节双三次 Bézier 飞檐翘角高度或细分步数时，即时重建 6 栋民居与牌楼曲面网格
        /// </summary>
        public void RebuildBezierRoofs(float newCornerLift, int newTessLevel)
        {
            BezierCornerLift = Mathf.Clamp(newCornerLift, 0.10f, 1.15f);
            BezierTessLevel  = Mathf.Clamp(newTessLevel, 0, 2);
            int steps = StepsFromLevel(BezierTessLevel);

            Mesh houseWithChimney = ProceduralMeshLib.BuildKoreanHouseMesh(BezierCornerLift, steps, true);
            Mesh houseNoChimney   = ProceduralMeshLib.BuildKoreanHouseMesh(BezierCornerLift, steps, false);

            for (int i = 0; i < _houseFilters.Count && i < _housePlacements.Length; ++i)
            {
                if (_houseFilters[i] != null)
                    _houseFilters[i].sharedMesh = _housePlacements[i].hasChimney ? houseWithChimney : houseNoChimney;
            }

            if (_gateFilter != null)
                _gateFilter.sharedMesh = ProceduralMeshLib.BuildWelcomeGateMesh(BezierCornerLift * 0.88f, steps);
        }

        private void Update()
        {
            // 同步太阳方向至追日光伏阵列，并以 20Hz 刷新 2D 动态光栅化沙盘贴图
            if (TimeController != null && HierarchyAnim != null)
                HierarchyAnim.CurrentSunDir = TimeController.SunDirection;

            if (Sandbox2D != null && Time.frameCount % 3 == 0)
                Sandbox2D.Redraw(Time.time * 48f);
        }

        private IEnumerator AutoCaptureSuiteCoroutine()
        {
            yield return new WaitForSeconds(0.5f);
            string outDir = Path.Combine(Application.dataPath, "..", "Screenshots");
            Directory.CreateDirectory(outDir);

            for (int preset = 0; preset < 3; ++preset)
            {
                TimeController.SetPreset(preset, immediate: true);
                CamPost.SetHeroKeyframe(preset);
                yield return new WaitForEndOfFrame();
                ScreenCapture.CaptureScreenshot(Path.Combine(outDir, $"unity2_preset_{preset}.png"));
                yield return new WaitForSeconds(0.2f);
            }
            Application.Quit();
        }
    }
}
