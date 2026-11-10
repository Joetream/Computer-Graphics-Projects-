// ============================================================================
// WP-U3 · Assets/Resources/Shaders/CG_PostProcessBloomACES.shader
// 2.0 电影级后处理管线着色器（4 Pass）：
//   Pass 0: 高亮阈值提取（Bright Pass Threshold + Soft Knee）
//   Pass 1: 水平高斯/Kawase 模糊
//   Pass 2: 垂直高斯/Kawase 模糊
//   Pass 3: Bloom 柔光合成 + ACES Filmic 色调映射 + 电影暗角 (Vignette)
// ============================================================================
Shader "YanbianCG/CG_PostProcessBloomACES"
{
    Properties
    {
        _MainTex ("Source RenderTexture", 2D) = "white" {}
        _BloomTex ("Blurred Bloom Texture", 2D) = "black" {}
        _Threshold ("Bloom Threshold", Float) = 0.78
        _Intensity ("Bloom Intensity", Float) = 0.55
        _Exposure ("ACES Exposure", Float) = 1.08
        _UseTonemap ("Enable ACES & Bloom", Float) = 1.0
    }
    SubShader
    {
        Cull Off ZWrite Off ZTest Always

        CGINCLUDE
        #include "UnityCG.cginc"

        sampler2D _MainTex;
        float4 _MainTex_TexelSize;
        sampler2D _BloomTex;
        float _Threshold;
        float _Intensity;
        float _Exposure;
        float _UseTonemap;

        struct v2f
        {
            float4 pos : SV_POSITION;
            float2 uv  : TEXCOORD0;
        };

        v2f vert(appdata_img v)
        {
            v2f o;
            o.pos = UnityObjectToClipPos(v.vertex);
            o.uv = v.texcoord;
            return o;
        }

        // Narkowicz ACES Filmic Tone Mapping Curve
        float3 ACESFilm(float3 x)
        {
            float a = 2.51f;
            float b = 0.03f;
            float c = 2.43f;
            float d = 0.59f;
            float e = 0.14f;
            return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
        }
        ENDCG

        // Pass 0: Bright Pass Threshold
        Pass
        {
            CGPROGRAM
            #pragma vertex vert
            #pragma fragment fragBright

            fixed4 fragBright(v2f i) : SV_Target
            {
                float3 c = tex2D(_MainTex, i.uv).rgb;
                float br = max(c.r, max(c.g, c.b));
                float contribution = max(0.0, br - _Threshold) / max(br, 1e-4);
                return float4(c * contribution, 1.0);
            }
            ENDCG
        }

        // Pass 1: Horizontal 9-tap Gaussian Blur
        Pass
        {
            CGPROGRAM
            #pragma vertex vert
            #pragma fragment fragBlurH

            fixed4 fragBlurH(v2f i) : SV_Target
            {
                float2 d = float2(_MainTex_TexelSize.x * 1.75, 0.0);
                float3 sum = tex2D(_MainTex, i.uv).rgb * 0.227027;
                sum += tex2D(_MainTex, i.uv + d * 1.0).rgb * 0.1945946;
                sum += tex2D(_MainTex, i.uv - d * 1.0).rgb * 0.1945946;
                sum += tex2D(_MainTex, i.uv + d * 2.0).rgb * 0.1216216;
                sum += tex2D(_MainTex, i.uv - d * 2.0).rgb * 0.1216216;
                sum += tex2D(_MainTex, i.uv + d * 3.0).rgb * 0.054054;
                sum += tex2D(_MainTex, i.uv - d * 3.0).rgb * 0.054054;
                sum += tex2D(_MainTex, i.uv + d * 4.0).rgb * 0.016216;
                sum += tex2D(_MainTex, i.uv - d * 4.0).rgb * 0.016216;
                return float4(sum, 1.0);
            }
            ENDCG
        }

        // Pass 2: Vertical 9-tap Gaussian Blur
        Pass
        {
            CGPROGRAM
            #pragma vertex vert
            #pragma fragment fragBlurV

            fixed4 fragBlurV(v2f i) : SV_Target
            {
                float2 d = float2(0.0, _MainTex_TexelSize.y * 1.75);
                float3 sum = tex2D(_MainTex, i.uv).rgb * 0.227027;
                sum += tex2D(_MainTex, i.uv + d * 1.0).rgb * 0.1945946;
                sum += tex2D(_MainTex, i.uv - d * 1.0).rgb * 0.1945946;
                sum += tex2D(_MainTex, i.uv + d * 2.0).rgb * 0.1216216;
                sum += tex2D(_MainTex, i.uv - d * 2.0).rgb * 0.1216216;
                sum += tex2D(_MainTex, i.uv + d * 3.0).rgb * 0.054054;
                sum += tex2D(_MainTex, i.uv - d * 3.0).rgb * 0.054054;
                sum += tex2D(_MainTex, i.uv + d * 4.0).rgb * 0.016216;
                sum += tex2D(_MainTex, i.uv - d * 4.0).rgb * 0.016216;
                return float4(sum, 1.0);
            }
            ENDCG
        }

        // Pass 3: Composite + ACES Filmic Tonemapping + Vignette
        Pass
        {
            CGPROGRAM
            #pragma vertex vert
            #pragma fragment fragComposite

            fixed4 fragComposite(v2f i) : SV_Target
            {
                float3 src   = tex2D(_MainTex, i.uv).rgb;
                if (_UseTonemap < 0.5)
                    return float4(src, 1.0);

                float3 bloom = tex2D(_BloomTex, i.uv).rgb;
                float3 hdr   = (src + bloom * _Intensity) * _Exposure;
                float3 mapped = ACESFilm(hdr);
                // 轻微对比度微调与电影暗角
                float2 centered = i.uv - 0.5;
                float vig = 1.0 - dot(centered, centered) * 0.22;
                mapped *= saturate(vig);
                return float4(mapped, 1.0);
            }
            ENDCG
        }
    }
}
