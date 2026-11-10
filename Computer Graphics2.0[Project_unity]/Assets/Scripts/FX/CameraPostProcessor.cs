// ============================================================================
// WP-U4 · Assets/Scripts/FX/CameraPostProcessor.cs
// 2.0 电影级相机管线控制器：
//   1. 支持 WASD + 鼠标右键自由漫游、B 样条电影自动运镜、正交/透视投影无缝切换
//   2. 支持硬件线框模式 (GL.wireframe) 与手写齐次平面投影阴影 (CG_PlanarShadow) 实时叠加
//   3. 基于 OnRenderImage 执行 HDR Bloom 辉光提取、双高斯模糊与 ACES Filmic 色调映射
// ============================================================================
using System.Collections.Generic;
using UnityEngine;
using YanbianCG.CGCore;
using YanbianCG.Scene;

namespace YanbianCG.FX
{
    [RequireComponent(typeof(Camera))]
    public class CameraPostProcessor : MonoBehaviour
    {
        private Camera _cam;
        private Material _postMat;
        private Material _planarShadowMat;
        private readonly BSplineTourPath _cinemaSpline = new BSplineTourPath();

        private float _yaw = 178f;
        private float _pitch = 4f;
        private float _cinemaT = 0f;

        public bool EnableBloomAndACES { get; set; } = true;
        public bool WireframeMode { get; set; } = false;
        public bool OrthoMode { get; set; } = false;
        public bool AutoCinemaSpline { get; set; } = false;
        public bool RenderPlanarShadows { get; set; } = false;
        public List<MeshFilter> ShadowCasters { get; set; } = new List<MeshFilter>();

        private void Awake()
        {
            _cam = GetComponent<Camera>();
            _cam.allowHDR = true;
            _cam.fieldOfView = 45f;
            _cam.nearClipPlane = 0.2f;
            _cam.farClipPlane = 260f;

            Shader postShader = Resources.Load<Shader>("Shaders/CG_PostProcessBloomACES");
            if (postShader == null) postShader = Shader.Find("YanbianCG/CG_PostProcessBloomACES");
            if (postShader != null)
                _postMat = new Material(postShader) { name = "Mat_PostProcessBloomACES" };

            Shader shadowShader = Resources.Load<Shader>("Shaders/CG_PlanarShadow");
            if (shadowShader == null) shadowShader = Shader.Find("YanbianCG/CG_PlanarShadow");
            if (shadowShader != null)
                _planarShadowMat = new Material(shadowShader) { name = "Mat_PlanarShadowPass" };

            SetHeroKeyframe(0);
        }

        public void SetHeroKeyframe(int preset)
        {
            AutoCinemaSpline = false;
            OrthoMode = false;
            _cam.orthographic = false;

            if (preset == 0)
            {
                // 晨曦主视角：正对村口“延边欢迎您”牌楼、布尔哈通河与长白山晨霞
                transform.position = new Vector3(2.0f, 3.8f, 44.0f);
                _yaw = 182.0f;
                _pitch = 3.0f;
            }
            else if (preset == 1)
            {
                // 正午近景视角：展示朝鲜族双三次 Bézier 飞檐民居、平面阴影与百年石井
                transform.position = new Vector3(6.0f, 5.2f, 16.0f);
                _yaw = 198.0f;
                _pitch = 8.0f;
            }
            else if (preset == 2)
            {
                // 星夜灯火视角：仰望新月银河、牌楼灯笼与河湾月影
                transform.position = new Vector3(0.0f, 3.6f, 42.0f);
                _yaw = 180.0f;
                _pitch = 2.0f;
            }
            ApplyCameraTransform();
        }

        public void ToggleOrthoBirdseye()
        {
            OrthoMode = !OrthoMode;
            _cam.orthographic = OrthoMode;
            if (OrthoMode)
            {
                _cam.orthographicSize = 26f;
                transform.position = new Vector3(0f, 48f, 38f);
                _yaw = 180f;
                _pitch = 48f;
                ApplyCameraTransform();
            }
            else
            {
                SetHeroKeyframe(0);
            }
        }

        private void ApplyCameraTransform()
        {
            transform.rotation = Quaternion.Euler(_pitch, _yaw, 0f);
        }

        private void Update()
        {
            // B 样条自动运镜模式
            if (AutoCinemaSpline)
            {
                _cinemaT = (_cinemaT + Time.deltaTime * 0.16f) % _cinemaSpline.SegmentCount;
                Vector3 p = _cinemaSpline.Evaluate(_cinemaT);
                Vector3 lookTarget = new Vector3(0f, 2.8f, 4f);
                transform.position = new Vector3(p.x * 1.25f, 5.2f + 1.5f * Mathf.Sin(Time.time * 0.5f), p.z * 1.25f);
                transform.LookAt(lookTarget);
                Vector3 euler = transform.eulerAngles;
                _pitch = euler.x > 180f ? euler.x - 360f : euler.x;
                _yaw = euler.y;
                return;
            }

            // 鼠标右键旋转视角
            if (Input.GetMouseButton(1))
            {
                _yaw   += Input.GetAxis("Mouse X") * 3.2f;
                _pitch -= Input.GetAxis("Mouse Y") * 2.6f;
                _pitch  = Mathf.Clamp(_pitch, -80f, 80f);
                ApplyCameraTransform();
            }

            // WASD + QE 自由漫游
            float speed = (Input.GetKey(KeyCode.LeftShift) ? 24f : 11f) * Time.deltaTime;
            Vector3 move = Vector3.zero;
            if (Input.GetKey(KeyCode.W)) move += transform.forward;
            if (Input.GetKey(KeyCode.S)) move -= transform.forward;
            if (Input.GetKey(KeyCode.D)) move += transform.right;
            if (Input.GetKey(KeyCode.A)) move -= transform.right;
            if (Input.GetKey(KeyCode.E)) move += Vector3.up;
            if (Input.GetKey(KeyCode.Q)) move -= Vector3.up;

            if (move.sqrMagnitude > 0f)
                transform.position += move.normalized * speed;
        }

        private void OnPreRender()
        {
            if (WireframeMode)
                GL.wireframe = true;
        }

        private void OnPostRender()
        {
            if (WireframeMode)
                GL.wireframe = false;

            // 当开启课程手写齐次平面投影阴影对比模式时，直接调用 CG_PlanarShadow Pass 绘制地面投影
            if (RenderPlanarShadows && !WireframeMode && _planarShadowMat != null)
            {
                _planarShadowMat.SetPass(0);
                foreach (var mf in ShadowCasters)
                {
                    if (mf != null && mf.sharedMesh != null && mf.gameObject.activeInHierarchy)
                        Graphics.DrawMeshNow(mf.sharedMesh, mf.transform.localToWorldMatrix);
                }
            }
        }

        private void OnRenderImage(RenderTexture src, RenderTexture dest)
        {
            if (_postMat == null || !EnableBloomAndACES || WireframeMode)
            {
                Graphics.Blit(src, dest);
                return;
            }

            int w = Mathf.Max(1, src.width / 2);
            int h = Mathf.Max(1, src.height / 2);
            RenderTexture rtBright = RenderTexture.GetTemporary(w, h, 0, src.format);
            RenderTexture rtBlurH  = RenderTexture.GetTemporary(w, h, 0, src.format);

            _postMat.SetFloat("_Threshold", 0.76f);
            _postMat.SetFloat("_Intensity", 0.58f);
            _postMat.SetFloat("_Exposure",  1.06f);
            _postMat.SetFloat("_UseTonemap", 1.0f);

            // Pass 0: 提取高亮区域 -> Pass 1 & 2: 双趟可分离高斯模糊 -> Pass 3: ACES 合成
            Graphics.Blit(src, rtBright, _postMat, 0);
            Graphics.Blit(rtBright, rtBlurH, _postMat, 1);
            Graphics.Blit(rtBlurH, rtBright, _postMat, 2);

            _postMat.SetTexture("_BloomTex", rtBright);
            Graphics.Blit(src, dest, _postMat, 3);

            RenderTexture.ReleaseTemporary(rtBright);
            RenderTexture.ReleaseTemporary(rtBlurH);
        }
    }
}
