// ============================================================================
// WP-U3 · Assets/Resources/Shaders/CG_SkyboxBlend.shader
// 双 Cubemap 实时插值天空盒着色器（晨曦 / 正午 / 星夜 Seedream 5.5 全景天际平滑过渡）
// ============================================================================
Shader "YanbianCG/CG_SkyboxBlend"
{
    Properties
    {
        _SkyCubeA ("Skybox Cubemap A", Cube) = "" {}
        _SkyCubeB ("Skybox Cubemap B", Cube) = "" {}
        _SkyBlend ("Skybox Blend Factor", Range(0, 1)) = 0.0
    }
    SubShader
    {
        Tags { "Queue"="Background" "RenderType"="Background" "PreviewType"="Skybox" }
        Cull Off ZWrite Off

        Pass
        {
            CGPROGRAM
            #pragma vertex vert
            #pragma fragment frag
            #include "UnityCG.cginc"

            samplerCUBE _CG_SkyCubeA;
            samplerCUBE _CG_SkyCubeB;
            float _CG_SkyBlend;

            struct appdata
            {
                float4 vertex : POSITION;
            };

            struct v2f
            {
                float4 pos : SV_POSITION;
                float3 dir : TEXCOORD0;
            };

            v2f vert(appdata v)
            {
                v2f o;
                o.pos = UnityObjectToClipPos(v.vertex);
                // 翻转 X 轴使 Unity 左手系与我们在 OpenGL 右手系切片校准的方位角完全一致
                o.dir = float3(-v.vertex.x, v.vertex.y, v.vertex.z);
                return o;
            }

            fixed4 frag(v2f i) : SV_Target
            {
                float3 d = normalize(i.dir);
                float3 colA = texCUBE(_CG_SkyCubeA, d).rgb;
                float3 colB = texCUBE(_CG_SkyCubeB, d).rgb;
                float3 sky  = lerp(colA, colB, saturate(_CG_SkyBlend));
                return float4(sky, 1.0);
            }
            ENDCG
        }
    }
}
