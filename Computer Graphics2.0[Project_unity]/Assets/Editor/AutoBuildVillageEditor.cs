// ============================================================================
// WP-U5 · Assets/Editor/AutoBuildVillageEditor.cs
// Unity 编辑器顶部菜单栏一键构建脚本（支持 GUI 点击与命令行 -executeMethod 批处理构建）：
//   · Yanbian CG 2.0 -> 1. One-Click Build & Save Village Scene
//   · Yanbian CG 2.0 -> 2. Build Standalone Windows EXE
//   · Yanbian CG 2.0 -> 3. Verify Hand-Written CG Math Library
// ============================================================================
#if UNITY_EDITOR
using System.IO;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using YanbianCG.CGCore;
using YanbianCG.Scene;

namespace YanbianCG.EditorTools
{
    public static class AutoBuildVillageEditor
    {
        [MenuItem("Yanbian CG 2.0/1. One-Click Build & Save Village Scene")]
        public static void BuildAndSaveScene()
        {
            var scene = EditorSceneManager.NewScene(NewSceneSetup.EmptyScene, NewSceneMode.Single);

            GameObject root = new GameObject("YanbianVillage_2.0_Root");
            var boot = root.AddComponent<YanbianSceneBootstrapper>();

            GameObject camGo = new GameObject("Main Camera");
            camGo.tag = "MainCamera";
            Camera cam = camGo.AddComponent<Camera>();
            cam.transform.position = new Vector3(2.0f, 3.8f, 44.0f);
            cam.transform.rotation = Quaternion.Euler(3.0f, 182.0f, 0.0f);
            camGo.AddComponent<YanbianCG.FX.CameraPostProcessor>();

            // 在编辑器 Scene 视图中也直接生成静态预览场景，方便不按 Play 也能在 Scene 窗口直接查看和选中物体
            boot.BuildCompleteVillageScene();
            if (SceneView.lastActiveSceneView != null)
            {
                SceneView.lastActiveSceneView.AlignViewToObject(cam.transform);
            }

            string scenesDir = "Assets/Scenes";
            if (!Directory.Exists(scenesDir))
                Directory.CreateDirectory(scenesDir);

            string scenePath = scenesDir + "/YanbianVillage_Main.unity";
            EditorSceneManager.SaveScene(scene, scenePath);

            EditorBuildSettings.scenes = new[]
            {
                new EditorBuildSettingsScene(scenePath, true)
            };

            AssetDatabase.SaveAssets();
            AssetDatabase.Refresh();
            Debug.Log("[AutoBuildVillageEditor] Successfully created and saved " + scenePath);
        }

        [MenuItem("Yanbian CG 2.0/2. Build Standalone Windows EXE")]
        public static void BuildStandaloneWindowsExe()
        {
            BuildAndSaveScene();

            string buildDir = Path.Combine(Application.dataPath, "..", "Build");
            Directory.CreateDirectory(buildDir);
            string exePath = Path.Combine(buildDir, "YanbianVillage_Unity2.exe");

            BuildPlayerOptions opts = new BuildPlayerOptions
            {
                scenes = new[] { "Assets/Scenes/YanbianVillage_Main.unity" },
                locationPathName = exePath,
                target = BuildTarget.StandaloneWindows64,
                options = BuildOptions.None
            };

            var report = BuildPipeline.BuildPlayer(opts);
            Debug.Log($"[AutoBuildVillageEditor] Build finished with result: {report.summary.result} -> {exePath}");
        }

        [MenuItem("Yanbian CG 2.0/3. Verify Hand-Written CG Math Library")]
        public static void VerifyMathLibraryMenu()
        {
            float err = MathUtil4x4.VerifyAgainstUnity();
            EditorUtility.DisplayDialog("CG Math Verification",
                $"Hand-written Perspective, Ortho, TRS & Planar Shadow matrices verified against Unity Matrix4x4!\n\nMax numerical error: {err:E2}",
                "OK");
        }
    }
}
#endif
