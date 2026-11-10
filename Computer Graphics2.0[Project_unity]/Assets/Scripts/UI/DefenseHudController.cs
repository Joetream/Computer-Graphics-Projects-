// ============================================================================
// WP-U5 · Assets/Scripts/UI/DefenseHudController.cs
// 延边朝鲜族民俗村 2.0 交互式课程答辩控制台（HUD + 2D光栅放大镜 + AI概念图对比画廊）：
//   · 顶部状态栏：实时显示 FPS、天光预设、明暗模式、Bézier 细分阶数、视锥剔除率、手写矩阵误差
//   · 右侧折叠答辩控制台：一键按钮 + 实时滑块调整 Bézier 飞檐起翘曲率与 Cohen-Sutherland 裁剪框
//   · 左下/右上画中画：256×256 2D 光栅化沙盘放大镜 [M] 与 Seedream 5.5 概念设计图对比画廊 [G]
// ============================================================================
using System.IO;
using UnityEngine;
using YanbianCG.Scene;

namespace YanbianCG.UI
{
    public class DefenseHudController : MonoBehaviour
    {
        private YanbianSceneBootstrapper _boot;
        private bool _showControlPanel = true;
        private bool _showSandboxMagnifier = true;
        private bool _showConceptGallery = false;
        private bool _showHelpModal = false;

        private Texture2D _conceptDawnTex;
        private Texture2D _conceptNightTex;
        private float _fpsSmoothed = 60f;

        private GUIStyle _headerStyle;
        private GUIStyle _boxStyle;
        private GUIStyle _labelStyle;
        private GUIStyle _btnStyle;

        public void Initialize(YanbianSceneBootstrapper boot)
        {
            _boot = boot;
            _conceptDawnTex  = LoadConceptImage("concept_dawn_village.jpg");
            _conceptNightTex = LoadConceptImage("concept_night_village.jpg");
        }

        private Texture2D LoadConceptImage(string fileName)
        {
            string p = Path.Combine(Application.dataPath, "Resources", "Textures", "ConceptArt", fileName);
            if (File.Exists(p))
            {
                byte[] bytes = File.ReadAllBytes(p);
                Texture2D t = new Texture2D(2, 2, TextureFormat.RGB24, false);
                t.LoadImage(bytes);
                return t;
            }
            return null;
        }

        private void Update()
        {
            _fpsSmoothed = Mathf.Lerp(_fpsSmoothed, 1f / Mathf.Max(Time.unscaledDeltaTime, 1e-4f), 0.1f);
            if (_boot == null || _boot.TimeController == null || _boot.CamPost == null) return;

            // 完整的答辩键盘快捷键（与 1.0 OpenGL 版 100% 兼容并扩展）
            if (Input.GetKeyDown(KeyCode.Alpha1)) { _boot.TimeController.SetPreset(0); _boot.CamPost.SetHeroKeyframe(0); }
            if (Input.GetKeyDown(KeyCode.Alpha2)) { _boot.TimeController.SetPreset(1); _boot.CamPost.SetHeroKeyframe(1); }
            if (Input.GetKeyDown(KeyCode.Alpha3)) { _boot.TimeController.SetPreset(2); _boot.CamPost.SetHeroKeyframe(2); }
            if (Input.GetKeyDown(KeyCode.L)) _boot.TimeController.CyclePreset();
            if (Input.GetKeyDown(KeyCode.K)) _boot.TimeController.ShadeMode = (_boot.TimeController.ShadeMode + 1) % 3;
            if (Input.GetKeyDown(KeyCode.N)) _boot.TimeController.ToggleSnow();
            if (Input.GetKeyDown(KeyCode.B)) _boot.RebuildBezierRoofs(_boot.BezierCornerLift, (_boot.BezierTessLevel + 1) % 3);
            if (Input.GetKeyDown(KeyCode.M)) _showSandboxMagnifier = !_showSandboxMagnifier;
            if (Input.GetKeyDown(KeyCode.C) && _boot.Sandbox2D != null)
                _boot.Sandbox2D.ClipWindowEnabled = !_boot.Sandbox2D.ClipWindowEnabled;
            if (Input.GetKeyDown(KeyCode.P)) _boot.CamPost.ToggleOrthoBirdseye();
            if (Input.GetKeyDown(KeyCode.O)) _boot.CamPost.AutoCinemaSpline = !_boot.CamPost.AutoCinemaSpline;
            if (Input.GetKeyDown(KeyCode.Z)) _boot.CamPost.WireframeMode = !_boot.CamPost.WireframeMode;
            if (Input.GetKeyDown(KeyCode.F) && _boot.RiceField != null)
                _boot.RiceField.EnableFrustumCulling = !_boot.RiceField.EnableFrustumCulling;
            if (Input.GetKeyDown(KeyCode.G)) _showConceptGallery = !_showConceptGallery;
            if (Input.GetKeyDown(KeyCode.H)) _showHelpModal = !_showHelpModal;
            if (Input.GetKeyDown(KeyCode.Tab)) _showControlPanel = !_showControlPanel;

            if (Input.GetKeyDown(KeyCode.F12))
            {
                string path = Path.Combine(Application.dataPath, "..", $"Screenshot_{System.DateTime.Now:HHmmss}.png");
                ScreenCapture.CaptureScreenshot(path);
                Debug.Log($"[DefenseHUD] Saved screenshot to {path}");
            }
        }

        private void EnsureStyles()
        {
            if (_headerStyle != null) return;

            Texture2D darkBg = new Texture2D(1, 1);
            darkBg.SetPixel(0, 0, new Color(0.08f, 0.11f, 0.16f, 0.86f));
            darkBg.Apply();

            _boxStyle = new GUIStyle(GUI.skin.box);
            _boxStyle.normal.background = darkBg;

            _headerStyle = new GUIStyle(GUI.skin.label)
            {
                fontSize = 14,
                fontStyle = FontStyle.Bold,
                normal = { textColor = new Color(1.0f, 0.88f, 0.52f) }
            };

            _labelStyle = new GUIStyle(GUI.skin.label)
            {
                fontSize = 12,
                normal = { textColor = new Color(0.92f, 0.95f, 0.98f) }
            };

            _btnStyle = new GUIStyle(GUI.skin.button)
            {
                fontSize = 12,
                fontStyle = FontStyle.Bold
            };
        }

        private void OnGUI()
        {
            if (_boot == null || _boot.TimeController == null) return;
            EnsureStyles();

            string[] shadeNames = { "0: Flat (ddx/ddy 棱面)", "1: Gouraud (逐顶点插值)", "2: Blinn-Phong (逐片元)" };
            int steps = YanbianSceneBootstrapper.StepsFromLevel(_boot.BezierTessLevel);
            int visRice = _boot.RiceField != null ? _boot.RiceField.VisibleInstances : 0;
            int totRice = _boot.RiceField != null ? _boot.RiceField.TotalInstances : 0;

            // ----------------------------------------------------------------
            // 1. 顶部全局实时指标横幅
            // ----------------------------------------------------------------
            GUI.Box(new Rect(12, 10, Screen.width - 24, 34), "", _boxStyle);
            string banner = $"【延边朝鲜族民俗村·乡村振兴 2.0 (Unity + 手写CG双擎版)】  " +
                            $"FPS: {_fpsSmoothed:F0}  |  天光: {_boot.TimeController.CurrentPresetName}  |  " +
                            $"明暗: {shadeNames[_boot.TimeController.ShadeMode]}  |  " +
                            $"Bézier飞檐: {steps}×{steps} (起翘 {_boot.BezierCornerLift:F2}m)  |  " +
                            $"稻田视锥剔除: {visRice}/{totRice} 株  |  " +
                            $"手写矩阵最大误差: {_boot.MathMaxError:E1}";
            GUI.Label(new Rect(22, 16, Screen.width - 44, 24), banner, _headerStyle);

            // ----------------------------------------------------------------
            // 2. 左侧交互式答辩控制台（可按 Tab 折叠）
            // ----------------------------------------------------------------
            if (_showControlPanel)
            {
                Rect panelRect = new Rect(12, 52, 330, 495);
                GUI.Box(panelRect, "", _boxStyle);
                GUILayout.BeginArea(new Rect(22, 60, 310, 480));

                GUILayout.Label("■ 课程五大核心模块实时交互台 [Tab 折叠]", _headerStyle);
                GUILayout.Space(4);

                // (1) 三态天光与相机机位
                GUILayout.Label("1. Seedream 5.5 三态天光与主视角 [1/2/3/L]：", _labelStyle);
                GUILayout.BeginHorizontal();
                if (GUILayout.Button("1·晨曦初露", _btnStyle)) { _boot.TimeController.SetPreset(0); _boot.CamPost.SetHeroKeyframe(0); }
                if (GUILayout.Button("2·正午晴空", _btnStyle)) { _boot.TimeController.SetPreset(1); _boot.CamPost.SetHeroKeyframe(1); }
                if (GUILayout.Button("3·星夜灯火", _btnStyle)) { _boot.TimeController.SetPreset(2); _boot.CamPost.SetHeroKeyframe(2); }
                GUILayout.EndHorizontal();

                GUILayout.Space(6);
                // (2) 三种明暗处理与阴影对比
                GUILayout.Label("2. 明暗模型 [K] & 阴影算法对比：", _labelStyle);
                if (GUILayout.Button($"切换明暗: {shadeNames[_boot.TimeController.ShadeMode]} [K]", _btnStyle))
                    _boot.TimeController.ShadeMode = (_boot.TimeController.ShadeMode + 1) % 3;

                GUILayout.BeginHorizontal();
                bool planar = _boot.TimeController.UsePlanarShadow;
                if (GUILayout.Button(planar ? "阴影: 手写齐次矩阵 M_shadow" : "阴影: Unity CSM 级联软阴影", _btnStyle))
                {
                    _boot.TimeController.UsePlanarShadow = !planar;
                    _boot.CamPost.RenderPlanarShadows = !planar;
                }
                if (GUILayout.Button(_boot.CamPost.EnableBloomAndACES ? "ACES+Bloom: 开启" : "ACES+Bloom: 关闭", _btnStyle))
                    _boot.CamPost.EnableBloomAndACES = !_boot.CamPost.EnableBloomAndACES;
                GUILayout.EndHorizontal();

                GUILayout.Space(6);
                // (3) 双三次 Bézier 曲面实时参数化调参
                GUILayout.Label($"3. 双三次 Bézier 飞檐起翘曲率 ({_boot.BezierCornerLift:F2}m) & 细分 [B]：", _labelStyle);
                float newLift = GUILayout.HorizontalSlider(_boot.BezierCornerLift, 0.15f, 1.05f);
                if (Mathf.Abs(newLift - _boot.BezierCornerLift) > 0.01f)
                    _boot.RebuildBezierRoofs(newLift, _boot.BezierTessLevel);

                GUILayout.BeginHorizontal();
                if (GUILayout.Button("低细分 (4×4)", _btnStyle))  _boot.RebuildBezierRoofs(_boot.BezierCornerLift, 0);
                if (GUILayout.Button("中细分 (10×10)", _btnStyle)) _boot.RebuildBezierRoofs(_boot.BezierCornerLift, 1);
                if (GUILayout.Button("高细分 (20×20)", _btnStyle)) _boot.RebuildBezierRoofs(_boot.BezierCornerLift, 2);
                GUILayout.EndHorizontal();

                GUILayout.Space(6);
                // (4) 2D 光栅化与 Cohen-Sutherland 裁剪窗口实时缩放
                GUILayout.Label("4. 2D 光栅沙盘 [M] & Cohen-Sutherland 裁剪框 [C]：", _labelStyle);
                if (_boot.Sandbox2D != null)
                {
                    GUILayout.BeginHorizontal();
                    if (GUILayout.Button(_showSandboxMagnifier ? "隐藏 2D 放大镜 [M]" : "显示 2D 放大镜 [M]", _btnStyle))
                        _showSandboxMagnifier = !_showSandboxMagnifier;
                    if (GUILayout.Button(_boot.Sandbox2D.ClipWindowEnabled ? "裁剪框: 开启 [C]" : "裁剪框: 关闭 [C]", _btnStyle))
                        _boot.Sandbox2D.ClipWindowEnabled = !_boot.Sandbox2D.ClipWindowEnabled;
                    GUILayout.EndHorizontal();

                    GUILayout.Label($"裁剪窗口边距: [{_boot.Sandbox2D.ClipXMin}, {256 - _boot.Sandbox2D.ClipXMin}]", _labelStyle);
                    int margin = Mathf.RoundToInt(GUILayout.HorizontalSlider(_boot.Sandbox2D.ClipXMin, 24f, 100f));
                    _boot.Sandbox2D.ClipXMin = _boot.Sandbox2D.ClipYMin = margin;
                    _boot.Sandbox2D.ClipXMax = _boot.Sandbox2D.ClipYMax = 256 - margin;
                }

                GUILayout.Space(6);
                // (5) 投影切换、B样条巡游、初冬瑞雪、线框、AI概念图画廊
                GUILayout.Label("5. 相机运镜 / 瑞雪粒子 / 视锥剔除 / 概念图：", _labelStyle);
                GUILayout.BeginHorizontal();
                if (GUILayout.Button(_boot.TimeController.SnowingEnabled ? "初冬瑞雪: 开启 [N]" : "初冬瑞雪: 关闭 [N]", _btnStyle))
                    _boot.TimeController.ToggleSnow();
                if (GUILayout.Button(_boot.CamPost.AutoCinemaSpline ? "B样条运镜: 巡游中 [O]" : "B样条自动运镜 [O]", _btnStyle))
                    _boot.CamPost.AutoCinemaSpline = !_boot.CamPost.AutoCinemaSpline;
                GUILayout.EndHorizontal();

                GUILayout.BeginHorizontal();
                if (GUILayout.Button(_boot.CamPost.OrthoMode ? "投影: 正交鸟瞰 [P]" : "投影: 透视漫游 [P]", _btnStyle))
                    _boot.CamPost.ToggleOrthoBirdseye();
                if (GUILayout.Button(_boot.CamPost.WireframeMode ? "网格线框: 开启 [Z]" : "网格线框: 关闭 [Z]", _btnStyle))
                    _boot.CamPost.WireframeMode = !_boot.CamPost.WireframeMode;
                GUILayout.EndHorizontal();

                GUILayout.BeginHorizontal();
                if (GUILayout.Button(_showConceptGallery ? "关闭 Seedream 概念图对比 [G]" : "打开 Seedream 概念图对比 [G]", _btnStyle))
                    _showConceptGallery = !_showConceptGallery;
                if (GUILayout.Button("快捷键帮助 [H]", _btnStyle))
                    _showHelpModal = !_showHelpModal;
                GUILayout.EndHorizontal();

                GUILayout.EndArea();
            }

            // ----------------------------------------------------------------
            // 3. 右上角画中画：256×256 2D 光栅化与裁剪沙盘实时放大镜 [M]
            // ----------------------------------------------------------------
            if (_showSandboxMagnifier && _boot.Sandbox2D != null && _boot.Sandbox2D.Texture != null)
            {
                float sz = 236f;
                Rect magRect = new Rect(Screen.width - sz - 24f, 52f, sz + 12f, sz + 34f);
                GUI.Box(magRect, "", _boxStyle);
                GUI.Label(new Rect(magRect.x + 8f, magRect.y + 4f, sz, 22f),
                          "2D 光栅化与 Cohen-Sutherland 裁剪沙盘 [M]", _headerStyle);
                GUI.DrawTexture(new Rect(magRect.x + 6f, magRect.y + 26f, sz, sz), _boot.Sandbox2D.Texture);
            }

            // ----------------------------------------------------------------
            // 4. 底部画廊：Seedream 5.5 晨曦/星夜概念设计图与实机场景同屏对比 [G]
            // ----------------------------------------------------------------
            if (_showConceptGallery)
            {
                float gw = 660f, gh = 235f;
                Rect galRect = new Rect((Screen.width - gw) * 0.5f, Screen.height - gh - 16f, gw, gh);
                GUI.Box(galRect, "", _boxStyle);
                GUI.Label(new Rect(galRect.x + 12f, galRect.y + 6f, gw - 24f, 22f),
                          "■ Seedream 5.5 AI 概念设计图 vs 实时 3D 渲染同屏对照画廊 [按 G 关闭]", _headerStyle);

                if (_conceptDawnTex != null)
                {
                    GUI.DrawTexture(new Rect(galRect.x + 14f, galRect.y + 32f, 305f, 172f), _conceptDawnTex, ScaleMode.StretchToFill);
                    GUI.Label(new Rect(galRect.x + 18f, galRect.y + 206f, 300f, 22f), "图2：金秋晨曦·延边民俗村概念主视觉", _labelStyle);
                }
                if (_conceptNightTex != null)
                {
                    GUI.DrawTexture(new Rect(galRect.x + 340f, galRect.y + 32f, 305f, 172f), _conceptNightTex, ScaleMode.StretchToFill);
                    GUI.Label(new Rect(galRect.x + 344f, galRect.y + 206f, 300f, 22f), "图3：星夜灯火·延边民俗村夜景概念图", _labelStyle);
                }
            }

            // ----------------------------------------------------------------
            // 5. 居中弹窗：快捷键与算法清单帮助面板 [H]
            // ----------------------------------------------------------------
            if (_showHelpModal)
            {
                float hw = 520f, hh = 310f;
                Rect hRect = new Rect((Screen.width - hw) * 0.5f, (Screen.height - hh) * 0.5f, hw, hh);
                GUI.Box(hRect, "", _boxStyle);
                GUILayout.BeginArea(new Rect(hRect.x + 18f, hRect.y + 14f, hw - 36f, hh - 28f));
                GUILayout.Label("《计算机图形学》课程设计 2.0 交互操作与算法对照指南", _headerStyle);
                GUILayout.Space(6);
                GUILayout.Label("• [WASD + QE] 自由移动相机    |  [按住鼠标右键拖动] 旋转视角", _labelStyle);
                GUILayout.Label("• [1 / 2 / 3 / L] 切换晨曦 / 正午 / 星夜三态天光与 Seedream 5.5 天空盒", _labelStyle);
                GUILayout.Label("• [K] 切换 Flat(ddx/ddy) / Gouraud(逐顶点) / Blinn-Phong(逐片元) 明暗模式", _labelStyle);
                GUILayout.Label("• [B 或左侧滑块] 实时调节 6 片双三次 Bézier 飞檐起翘曲率与 4/10/20 细分", _labelStyle);
                GUILayout.Label("• [M / C] 开关 2D 光栅化沙盘放大镜 & Cohen-Sutherland 4位编码裁剪框", _labelStyle);
                GUILayout.Label("• [N] 开启/关闭初冬瑞雪粒子与法线积雪渐变混合", _labelStyle);
                GUILayout.Label("• [O / P / Z] B样条自动巡游运镜 / 正交鸟瞰投影 / 硬件线框模式", _labelStyle);
                GUILayout.Label("• [G / F12 / Tab] 打开 AI 概念图对比画廊 / 保存高清截图 / 折叠左侧面板", _labelStyle);
                GUILayout.Space(10);
                if (GUILayout.Button("关闭帮助面板 [H]", _btnStyle))
                    _showHelpModal = false;
                GUILayout.EndArea();
            }
        }
    }
}
