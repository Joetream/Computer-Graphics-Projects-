# 延边朝鲜族民俗村·乡村振兴 2.0 (`Computer Graphics2.0[Project_unity]`)
### Unity 6 (`6000.6.8f1`) 电影级渲染管线 + 手写计算机图形学核心算法双擎联动版

![金秋晨曦概念主视觉](Assets/Resources/Textures/ConceptArt/concept_dawn_village.jpg)

---

## 一、项目简介与双擎定位

本项目是《计算机图形学》课程设计 **“延边朝鲜族民俗村·乡村振兴”** 的 **2.0 升级工程 (`Computer Graphics2.0[Project_unity]`)**，与仓库中的 **1.0 原生 C++17 + OpenGL 3.3 Core 引擎 (`Computer Graphics[Projects]`)** 形成**“底层光栅算法手写引擎 + 工业级渲染管线展示引擎”**双版本对照：

1. **零手工建模负担，100% 程序化与 C++ 资产桥接**：
   - 通过 [`tools/export_to_unity.cpp`](../tools/export_to_unity.cpp) 将 1.0 引擎生成的双三次 Bézier $C^1$ 朝鲜族“鹤飞屋”歇山顶民居、四柱迎宾牌楼与百年石井导出为标准 `.obj` 网格资产存放于 [`Assets/Meshes_Exported/`](Assets/Meshes_Exported/)；
   - 同时在 Unity C# 端提供实时参数化曲面生成器 [`BezierPatchGenerator.cs`](Assets/Scripts/CGCore/BezierPatchGenerator.cs) 与 [`ProceduralMeshLib.cs`](Assets/Scripts/Scene/ProceduralMeshLib.cs)，支持在答辩控制台中**实时拖动滑块改变飞檐翘角曲率 (`cornerLift`) 与细分阶数 (`4×4 / 10×10 / 20×20`)**！
2. **100% 保留课程五大核心手写算法（位于 [`Assets/Scripts/CGCore/`](Assets/Scripts/CGCore/) 与 [`Assets/Resources/Shaders/`](Assets/Resources/Shaders/)）**：
   - **二维光栅化与裁剪沙盘 ([`Raster2DSandbox.cs`](Assets/Scripts/CGCore/Raster2DSandbox.cs))**：纯 CPU 像素缓冲实现中点 Bresenham 直线、中点画圆、有序边表 (`ET + AET`) 扫描线多边形填充、Cohen-Sutherland 四位编码直线段裁剪（支持滑块实时缩放裁剪窗口）；
   - **手写齐次变换矩阵与矩阵栈 ([`MathUtil4x4.cs`](Assets/Scripts/CGCore/MathUtil4x4.cs)、[`MatrixStack.cs`](Assets/Scripts/CGCore/MatrixStack.cs))**：手写 `Translate / Rotate / Scale / LookAt / Perspective / Ortho / BuildPlanarShadowMatrix`，启动时自动与 Unity `Matrix4x4` 进行数值误差校验（误差 `< 1e-6`），并通过手写 `MatrixStack` 驱动风车、秋千、跳板、双轴追日光伏板与无人导览电瓶车的树状层级动画；
   - **双三次 Bézier 曲面与三次均匀闭环 B 样条曲线 ([`BezierPatchGenerator.cs`](Assets/Scripts/CGCore/BezierPatchGenerator.cs)、[`BSplineTourPath.cs`](Assets/Scripts/CGCore/BSplineTourPath.cs))**：6 片双三次 Bézier 曲面 $C^1$ 连续拼接与解析偏导法线求值；8 控制点闭环 B 样条巡游轨迹；
   - **三种明暗模型与平面投影阴影自定义 HLSL 着色器 ([`CG_UberLit.shader`](Assets/Resources/Shaders/CG_UberLit.shader)、[`CG_PlanarShadow.shader`](Assets/Resources/Shaders/CG_PlanarShadow.shader))**：支持一键切换 `Flat (ddx/ddy)`、`Gouraud (逐顶点)`、`Blinn-Phong (逐片元)`，并支持在 **Unity 级联软阴影 (CSM)** 与 **手写齐次平面投影阴影矩阵 ($M_{\text{shadow}}$ + Stencil 防重影)** 之间一键切换对比；
   - **Gribb-Hartmann 视锥体 6 平面提取与 GPU Instancing ([`FrustumCuller.cs`](Assets/Scripts/CGCore/FrustumCuller.cs)、[`RiceFieldInstancer.cs`](Assets/Scripts/Scene/RiceFieldInstancer.cs))**：32 个稻田分块 AABB 实时视锥剔除 + 3,200 株金秋稻穗 `Graphics.DrawMeshInstanced` 硬件实例化渲染。
3. **2.0 电影级视觉升级**：
   - **Seedream 5.5 360° 全景天空盒三态平滑过渡 ([`TimeOfDayController.cs`](Assets/Scripts/FX/TimeOfDayController.cs)、[`CG_SkyboxBlend.shader`](Assets/Resources/Shaders/CG_SkyboxBlend.shader))**：晨曦紫霞云海、正午晴空雪峰、星夜银河新月无缝插值，并与布尔哈通河水波菲涅尔反射实时联动；
   - **HDR Bloom 辉光与 ACES Filmic 色调映射后处理 ([`CG_PostProcessBloomACES.shader`](Assets/Resources/Shaders/CG_PostProcessBloomACES.shader)、[`CameraPostProcessor.cs`](Assets/Scripts/FX/CameraPostProcessor.cs))**：星夜模式下韩纸木格窗与 6 盏庭院石灯笼散发温暖柔光。

---

## 二、快速运行指南（打开即跑，零手工配置）

### 方式 A：在 Unity Hub / Unity Editor (`6000.6.8f1`) 中打开运行
1. 打开 **Unity Hub** (`D:\Unity\UnityHub\Unity Hub.exe`)，点击右上角 **`Add -> Add project from disk`**，选择本目录：
   `d:\Antigravity_work\YanbianVillage\Computer Graphics2.0[Project_unity]`
2. 进入编辑器后：
   - **直接点击顶部 `Play (▶)` 按钮**：借助 [`YanbianSceneBootstrapper.cs`](Assets/Scripts/Scene/YanbianSceneBootstrapper.cs) 的 `[RuntimeInitializeOnLoadMethod]` 自举机制，哪怕当前处于空白默认场景，也会在 0.2 秒内全自动生成整个延边朝鲜族民俗村、灯光、天空盒与交互式答辩 UI！
   - 或者点击顶部菜单栏 **`Yanbian CG 2.0 -> 1. One-Click Build & Save Village Scene`** 将场景持久化保存至 `Assets/Scenes/YanbianVillage_Main.unity`。
   - 点击顶部菜单栏 **`Yanbian CG 2.0 -> 2. Build Standalone Windows EXE`** 可一键编译独立可执行程序 `Build/YanbianVillage_Unity2.exe`。

---

## 三、交互快捷键与答辩控制台说明

| 按键 / 控件 | 功能说明 | 对应图形学考点 |
| :--- | :--- | :--- |
| **`WASD` + `QE` + 鼠标右键** | 自由飞行漫游相机（按住 `Shift` 加速） | 观察变换矩阵与相机欧拉角解算 |
| **`1` / `2` / `3` / `L`** | 切换 **晨曦初露 / 正午晴空 / 星夜灯火** 三态天光与主视角 | 双 Cubemap 线性插值 + 多光源融合 |
| **`K` 或面板按钮** | 循环切换 `0: Flat` / `1: Gouraud` / `2: Blinn-Phong` 明暗模式 | 三种经典光照与明暗着色模型对比 |
| **`B` 或面板曲率滑块** | 切换双三次 Bézier 飞檐细分阶数 (`4×4 / 10×10 / 20×20`) 并实时调整檐角起翘高度 | 双三次 Bézier 参数曲面与 $C^1$ 连续拼接 |
| **`M` / `C` 或裁剪滑块** | 开关右上角 2D 光栅沙盘放大镜 / 开关并实时缩放 Cohen-Sutherland 裁剪框 | Bresenham、扫描线填充与编码裁剪算法 |
| **阴影对比按钮** | 在 **Unity CSM 级联软阴影** 与 **手写齐次平面投影矩阵 $M_{\text{shadow}}$** 间切换 | 平面投影阴影推导与模板缓冲防重影 |
| **`N`** | 开启/关闭初冬瑞雪粒子飘落与屋顶/地面法线积雪渐变混合 | 粒子系统与表面法线混合着色 |
| **`O` / `P` / `Z`** | B 样条自动环村运镜 / 正交鸟瞰投影切换 / 硬件线框模式 | B 样条曲线求值、正交/透视投影对比 |
| **`G` / `H` / `Tab`** | 打开 Seedream 5.5 概念设计图同屏对比画廊 / 快捷键帮助 / 折叠控制面板 | 概念设计与实时 3D 渲染对照 |
