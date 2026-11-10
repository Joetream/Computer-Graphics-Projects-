// ============================================================================
// WP-U4 · Assets/Scripts/FX/TimeOfDayController.cs
// 延边朝鲜族民俗村三态天光、Seedream 5.5 全景天空盒插值、6盏星夜庭院暖光灯与粒子控制器：
//   · Preset 0: 晨曦初露（Dawn —— 金秋紫霞长白山云海 + 柔和暖阳 + 袅袅炊烟）
//   · Preset 1: 正午晴空（Noon —— 碧空长白雪峰 + 清晰软阴影/平面投影阴影对比）
//   · Preset 2: 星夜灯火（Night —— 银河新月雪山 + 6盏庭院石灯笼暖光 + 韩纸木窗透光辉光）
// ============================================================================
using System.Collections.Generic;
using System.IO;
using UnityEngine;
using YanbianCG.CGCore;
using YanbianCG.Scene;

namespace YanbianCG.FX
{
    public class TimeOfDayController : MonoBehaviour
    {
        private struct SkyPresetData
        {
            public string name;
            public Vector3 sunDir;
            public Color sunColor;
            public Color ambientColor;
            public Color fogColor;
            public float fogDensity;
            public float pointLightFactor;
        }

        private readonly SkyPresetData[] _presets =
        {
            new SkyPresetData
            {
                name = "0 · 晨曦初露 (Dawn)",
                sunDir = new Vector3(0.72f, 0.30f, -0.45f).normalized,
                sunColor = new Color(1.00f, 0.78f, 0.56f),
                ambientColor = new Color(0.46f, 0.38f, 0.48f),
                fogColor = new Color(0.84f, 0.64f, 0.58f),
                fogDensity = 0.0042f,
                pointLightFactor = 0.18f
            },
            new SkyPresetData
            {
                name = "1 · 正午晴空 (Noon)",
                sunDir = new Vector3(0.28f, 0.88f, -0.32f).normalized,
                sunColor = new Color(1.00f, 0.97f, 0.90f),
                ambientColor = new Color(0.48f, 0.54f, 0.62f),
                fogColor = new Color(0.74f, 0.84f, 0.94f),
                fogDensity = 0.0026f,
                pointLightFactor = 0.0f
            },
            new SkyPresetData
            {
                name = "2 · 星夜灯火 (Night)",
                sunDir = new Vector3(-0.35f, 0.62f, -0.52f).normalized,
                sunColor = new Color(0.24f, 0.34f, 0.56f),
                ambientColor = new Color(0.14f, 0.18f, 0.28f),
                fogColor = new Color(0.08f, 0.12f, 0.22f),
                fogDensity = 0.0048f,
                pointLightFactor = 1.35f
            }
        };

        private readonly Vector3[] _lanternPositions =
        {
            new Vector3(-4.2f, 2.35f, 30.2f),
            new Vector3( 4.2f, 2.35f, 30.2f),
            new Vector3(-7.8f, 2.35f, 12.0f),
            new Vector3( 7.8f, 2.35f, 12.0f),
            new Vector3(-7.8f, 2.35f, -4.0f),
            new Vector3( 7.8f, 2.35f, -4.0f)
        };

        private Cubemap[] _skyCubemaps = new Cubemap[3];
        private Material _skyboxMat;
        private Light _mainDirectionalLight;
        private readonly List<Light> _unityPointLights = new List<Light>();
        private readonly List<MeshRenderer> _lanternBulbs = new List<MeshRenderer>();
        private ParticleSystem _snowParticles;

        private int _currentPreset = 0;
        private int _prevPreset    = 0;
        private float _blendT      = 1.0f;

        public int CurrentPreset => _currentPreset;
        public string CurrentPresetName => _presets[_currentPreset].name;
        public Vector3 SunDirection { get; private set; }
        public float SnowAmount { get; set; } = 0f;
        public bool SnowingEnabled { get; private set; } = false;
        public int ShadeMode { get; set; } = 2; // 0=Flat, 1=Gouraud, 2=Blinn-Phong
        public bool UsePlanarShadow { get; set; } = false;

        public void Initialize(Material uberMat, List<Vector3> chimneyTops)
        {
            // 1. 加载 Seedream 5.5 切片的 3 组 6 面体 Cubemap（Dawn / Noon / Night）
            string[] folders = { "dawn", "noon", "night" };
            for (int i = 0; i < 3; ++i)
                _skyCubemaps[i] = LoadSeedreamCubemap(folders[i]);

            Shader skyShader = Resources.Load<Shader>("Shaders/CG_SkyboxBlend");
            if (skyShader == null) skyShader = Shader.Find("YanbianCG/CG_SkyboxBlend");
            if (skyShader != null)
            {
                _skyboxMat = new Material(skyShader) { name = "Mat_SeedreamSkyboxBlend" };
                RenderSettings.skybox = _skyboxMat;
            }

            // 2. 主平行光（投射 Unity 级联软阴影 CSM）
            GameObject dirGo = new GameObject("DirectionalLight_SunMoon");
            dirGo.transform.SetParent(transform, false);
            _mainDirectionalLight = dirGo.AddComponent<Light>();
            _mainDirectionalLight.type = LightType.Directional;
            _mainDirectionalLight.shadows = LightShadows.Soft;
            _mainDirectionalLight.shadowStrength = 0.78f;
            _mainDirectionalLight.shadowBias = 0.03f;

            // 3. 创建 6 座延边传统庭院石木路灯与暖光点光源
            BuildCourtyardLanterns(uberMat);

            // 4. 创建独立烟囱袅袅炊烟粒子与初冬瑞雪粒子系统
            foreach (var top in chimneyTops)
                CreateChimneySmokeEmitter(top);
            CreateWinterSnowEmitter();

            SetPreset(0, immediate: true);
        }

        private Cubemap LoadSeedreamCubemap(string folder)
        {
            Cubemap cube = new Cubemap(512, TextureFormat.RGB24, false)
            {
                name = "Cubemap_Seedream_" + folder,
                wrapMode = TextureWrapMode.Clamp,
                filterMode = FilterMode.Bilinear
            };

            string[] faceNames = { "px", "nx", "py", "ny", "pz", "nz" };
            CubemapFace[] faces =
            {
                CubemapFace.PositiveX, CubemapFace.NegativeX,
                CubemapFace.PositiveY, CubemapFace.NegativeY,
                CubemapFace.PositiveZ, CubemapFace.NegativeZ
            };

            for (int f = 0; f < 6; ++f)
            {
                string filePath = Path.Combine(Application.dataPath, "Resources", "Textures", "Skybox", folder, faceNames[f] + ".png");
                Texture2D tempTex = null;
                if (File.Exists(filePath))
                {
                    byte[] pngBytes = File.ReadAllBytes(filePath);
                    tempTex = new Texture2D(2, 2, TextureFormat.RGB24, false);
                    tempTex.LoadImage(pngBytes);
                }
                else
                {
                    Texture2D resTex = Resources.Load<Texture2D>($"Textures/Skybox/{folder}/{faceNames[f]}");
                    if (resTex != null && resTex.isReadable) tempTex = resTex;
                }

                if (tempTex != null && tempTex.width == 512 && tempTex.height == 512)
                {
                    // OpenGL stbi_load(flip=false) 的首行在顶部，Unity Cubemap.SetPixels 首行亦对应顶部，垂直翻转对齐
                    Color[] src = tempTex.GetPixels();
                    Color[] dst = new Color[512 * 512];
                    for (int y = 0; y < 512; ++y)
                    {
                        int srcRow = (511 - y) * 512;
                        int dstRow = y * 512;
                        System.Array.Copy(src, srcRow, dst, dstRow, 512);
                    }
                    cube.SetPixels(dst, faces[f]);
                    if (File.Exists(filePath)) DestroyImmediate(tempTex);
                }
            }
            cube.Apply(false);
            return cube;
        }

        private void BuildCourtyardLanterns(Material uberMat)
        {
            for (int i = 0; i < _lanternPositions.Length; ++i)
            {
                Vector3 p = _lanternPositions[i];
                GameObject root = new GameObject($"CourtyardLantern_{i}");
                root.transform.SetParent(transform, false);
                root.transform.position = new Vector3(p.x, 0f, p.z);

                // 石底座 + 木灯杆 + 小歇山灯帽
                MeshBuildBuffer b = new MeshBuildBuffer();
                b.AddBox(new Vector3(0f, 0.16f, 0f), new Vector3(0.46f, 0.32f, 0.46f), ProceduralMeshLib.StoneColor);
                b.AddBox(new Vector3(0f, 1.25f, 0f), new Vector3(0.12f, 2.15f, 0.12f), ProceduralMeshLib.WoodColor);
                b.AddBox(new Vector3(0f, 2.12f, 0f), new Vector3(0.54f, 0.08f, 0.54f), ProceduralMeshLib.WoodColor);
                b.AddCylinder(new Vector3(0f, 2.52f, 0f), 0.38f, 0.04f, 0.28f, 4, ProceduralMeshLib.TileColor);

                MeshFilter mf = root.AddComponent<MeshFilter>();
                MeshRenderer mr = root.AddComponent<MeshRenderer>();
                mf.sharedMesh = b.ToMesh($"LanternPost_{i}");
                mr.sharedMaterial = uberMat;

                // 发光韩纸灯芯盒（星夜触发 HDR Bloom 柔光）
                GameObject bulbGo = GameObject.CreatePrimitive(PrimitiveType.Cube);
                bulbGo.name = "LanternGlowBulb";
                bulbGo.transform.SetParent(root.transform, false);
                bulbGo.transform.localPosition = new Vector3(0f, 2.34f, 0f);
                bulbGo.transform.localScale = new Vector3(0.28f, 0.34f, 0.28f);
                DestroyImmediate(bulbGo.GetComponent<Collider>());
                MeshRenderer bulbMr = bulbGo.GetComponent<MeshRenderer>();
                Material bulbMat = new Material(uberMat) { name = $"Mat_LanternBulb_{i}" };
                bulbMat.SetColor("_BaseColor", new Color(1.0f, 0.85f, 0.52f));
                bulbMat.SetColor("_EmissiveColor", new Color(2.2f, 1.45f, 0.65f));
                bulbMr.sharedMaterial = bulbMat;
                _lanternBulbs.Add(bulbMr);

                // Unity Point Light
                GameObject lightGo = new GameObject($"PointLight_{i}");
                lightGo.transform.SetParent(root.transform, false);
                lightGo.transform.localPosition = new Vector3(0f, 2.35f, 0f);
                Light pt = lightGo.AddComponent<Light>();
                pt.type = LightType.Point;
                pt.color = new Color(1.0f, 0.74f, 0.38f);
                pt.range = 16f;
                pt.intensity = 0.2f;
                _unityPointLights.Add(pt);
            }
        }

        private void CreateChimneySmokeEmitter(Vector3 worldPos)
        {
            GameObject go = new GameObject("ChimneySmoke_Particles");
            go.transform.SetParent(transform, false);
            go.transform.position = worldPos;

            ParticleSystem ps = go.AddComponent<ParticleSystem>();
            var main = ps.main;
            main.startLifetime = new ParticleSystem.MinMaxCurve(3.2f, 4.8f);
            main.startSpeed = new ParticleSystem.MinMaxCurve(0.55f, 1.05f);
            main.startSize = new ParticleSystem.MinMaxCurve(0.28f, 0.65f);
            main.startColor = new Color(0.90f, 0.90f, 0.92f, 0.42f);
            main.maxParticles = 120;
            main.simulationSpace = ParticleSystemSimulationSpace.World;

            var emission = ps.emission;
            emission.rateOverTime = 16f;

            var shape = ps.shape;
            shape.shapeType = ParticleSystemShapeType.Cone;
            shape.angle = 12f;
            shape.radius = 0.14f;
            shape.rotation = new Vector3(-90f, 0f, 0f);

            var vel = ps.velocityOverLifetime;
            vel.enabled = true;
            vel.x = new ParticleSystem.MinMaxCurve(0.18f, 0.38f);
            vel.y = new ParticleSystem.MinMaxCurve(0.35f, 0.65f);
            vel.z = new ParticleSystem.MinMaxCurve(-0.10f, 0.15f);

            var sizeOverLifetime = ps.sizeOverLifetime;
            sizeOverLifetime.enabled = true;
            sizeOverLifetime.size = new ParticleSystem.MinMaxCurve(1f, AnimationCurve.Linear(0f, 0.5f, 1f, 2.2f));

            ParticleSystemRenderer psr = go.GetComponent<ParticleSystemRenderer>();
            psr.sharedMaterial = CreateSoftParticleMaterial();
        }

        private void CreateWinterSnowEmitter()
        {
            GameObject go = new GameObject("WinterSnow_Particles");
            go.transform.SetParent(transform, false);
            go.transform.position = new Vector3(0f, 26f, 8f);

            _snowParticles = go.AddComponent<ParticleSystem>();
            var main = _snowParticles.main;
            main.startLifetime = new ParticleSystem.MinMaxCurve(6.0f, 9.0f);
            main.startSpeed = new ParticleSystem.MinMaxCurve(2.2f, 3.6f);
            main.startSize = new ParticleSystem.MinMaxCurve(0.12f, 0.24f);
            main.startColor = new Color(0.96f, 0.98f, 1.0f, 0.85f);
            main.maxParticles = 1200;
            main.simulationSpace = ParticleSystemSimulationSpace.World;

            var emission = _snowParticles.emission;
            emission.rateOverTime = 160f;

            var shape = _snowParticles.shape;
            shape.shapeType = ParticleSystemShapeType.Box;
            shape.scale = new Vector3(95f, 2f, 95f);
            shape.rotation = new Vector3(90f, 0f, 0f);

            ParticleSystemRenderer psr = go.GetComponent<ParticleSystemRenderer>();
            psr.sharedMaterial = CreateSoftParticleMaterial();
            _snowParticles.Stop();
        }

        private Material CreateSoftParticleMaterial()
        {
            Shader sh = Shader.Find("Particles/Standard Unlit");
            if (sh == null) sh = Shader.Find("Legacy Shaders/Particles/Alpha Blended");
            if (sh == null) sh = Shader.Find("Sprites/Default");
            Material m = new Material(sh) { name = "Mat_SoftParticle" };
            Texture2D softCircle = new Texture2D(32, 32, TextureFormat.RGBA32, false);
            for (int y = 0; y < 32; ++y)
            {
                for (int x = 0; x < 32; ++x)
                {
                    float dx = (x - 15.5f) / 15.5f;
                    float dy = (y - 15.5f) / 15.5f;
                    float a = Mathf.Clamp01(1f - Mathf.Sqrt(dx * dx + dy * dy));
                    softCircle.SetPixel(x, y, new Color(1f, 1f, 1f, a * a));
                }
            }
            softCircle.Apply();
            m.mainTexture = softCircle;
            return m;
        }

        public void SetPreset(int index, bool immediate = false)
        {
            index = Mathf.Clamp(index, 0, 2);
            if (immediate)
            {
                _prevPreset = index;
                _currentPreset = index;
                _blendT = 1.0f;
            }
            else if (index != _currentPreset)
            {
                _prevPreset = _currentPreset;
                _currentPreset = index;
                _blendT = 0.0f;
            }
            ApplyShaderUniforms();
        }

        public void CyclePreset()
        {
            SetPreset((_currentPreset + 1) % 3, immediate: false);
        }

        public void ToggleSnow()
        {
            SnowingEnabled = !SnowingEnabled;
            if (_snowParticles != null)
            {
                if (SnowingEnabled) _snowParticles.Play();
                else _snowParticles.Stop();
            }
        }

        private void Update()
        {
            if (_blendT < 1.0f)
                _blendT = Mathf.Min(1.0f, _blendT + Time.deltaTime * 0.85f);

            if (SnowingEnabled)
                SnowAmount = Mathf.Min(1.0f, SnowAmount + Time.deltaTime * 0.18f);
            else
                SnowAmount = Mathf.Max(0.0f, SnowAmount - Time.deltaTime * 0.25f);

            ApplyShaderUniforms();
        }

        public void ApplyShaderUniforms()
        {
            SkyPresetData a = _presets[_prevPreset];
            SkyPresetData b = _presets[_currentPreset];
            float t = Mathf.SmoothStep(0f, 1f, _blendT);

            SunDirection = Vector3.Slerp(a.sunDir, b.sunDir, t).normalized;
            Color sunCol = Color.Lerp(a.sunColor, b.sunColor, t);
            Color ambCol = Color.Lerp(a.ambientColor, b.ambientColor, t);
            Color fogCol = Color.Lerp(a.fogColor, b.fogColor, t);
            float fogDen = Mathf.Lerp(a.fogDensity, b.fogDensity, t);
            float ptFactor = Mathf.Lerp(a.pointLightFactor, b.pointLightFactor, t);

            // 更新主平行光姿态与软阴影开关（当开启手写平面投影阴影对比时自动关闭 CSM 防重叠）
            if (_mainDirectionalLight != null)
            {
                _mainDirectionalLight.transform.rotation = Quaternion.LookRotation(-SunDirection, Vector3.up);
                _mainDirectionalLight.color = sunCol;
                _mainDirectionalLight.intensity = 1.0f;
                _mainDirectionalLight.shadows = UsePlanarShadow ? LightShadows.None : LightShadows.Soft;
            }

            for (int i = 0; i < _unityPointLights.Count; ++i)
                _unityPointLights[i].intensity = ptFactor * 1.4f;

            // 全局课程 Shader Uniforms
            Shader.SetGlobalInt("_CG_ShadeMode", ShadeMode);
            Shader.SetGlobalFloat("_CG_SnowAmount", SnowAmount);
            Shader.SetGlobalVector("_CG_SunDir", SunDirection);
            Shader.SetGlobalVector("_CG_SunColor", new Vector4(sunCol.r, sunCol.g, sunCol.b, 1f));
            Shader.SetGlobalVector("_CG_Ambient", new Vector4(ambCol.r, ambCol.g, ambCol.b, 1f));
            Shader.SetGlobalVector("_CG_FogColor", new Vector4(fogCol.r, fogCol.g, fogCol.b, 1f));
            Shader.SetGlobalFloat("_CG_FogDensity", fogDen);
            Shader.SetGlobalFloat("_CG_PointLightFactor", ptFactor);
            Shader.SetGlobalInt("_CG_NumPointLights", 6);

            Vector4[] posArr = new Vector4[6];
            Vector4[] colArr = new Vector4[6];
            for (int i = 0; i < 6; ++i)
            {
                posArr[i] = new Vector4(_lanternPositions[i].x, _lanternPositions[i].y, _lanternPositions[i].z, 1f);
                colArr[i] = new Vector4(1.00f, 0.72f, 0.34f, 1f);
            }
            Shader.SetGlobalVectorArray("_CG_PointLightPos", posArr);
            Shader.SetGlobalVectorArray("_CG_PointLightColor", colArr);

            if (_skyCubemaps[_prevPreset] != null)
                Shader.SetGlobalTexture("_CG_SkyCubeA", _skyCubemaps[_prevPreset]);
            if (_skyCubemaps[_currentPreset] != null)
                Shader.SetGlobalTexture("_CG_SkyCubeB", _skyCubemaps[_currentPreset]);
            Shader.SetGlobalFloat("_CG_SkyBlend", t);

            // 手写齐次平面投影阴影矩阵 M_shadow = (N·L)I - L*N^T（投影至地面 y = 0.025）
            Matrix4x4 planarShadowMat = MathUtil4x4.BuildPlanarShadowMatrix(
                new Vector4(0f, 1f, 0f, -0.025f),
                new Vector4(SunDirection.x, Mathf.Max(SunDirection.y, 0.15f), SunDirection.z, 0f)
            );
            Shader.SetGlobalMatrix("_CG_PlanarShadowMat", planarShadowMat);
        }
    }
}
