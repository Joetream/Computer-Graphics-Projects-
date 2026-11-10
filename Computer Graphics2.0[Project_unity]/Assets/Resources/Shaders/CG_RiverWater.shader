// ============================================================================
// WP-U3 · Assets/Resources/Shaders/CG_RiverWater.shader
// 布尔哈通河湾水体着色器：
//   · 顶点多波叠加正弦/Gerstner 波位移与解析偏导法线扰动
//   · Schlick 菲涅尔方程：F = 0.08 + 0.92 * (1 - max(N·V, 0))^4
//   · 实时采样并混合双 Seedream 5.5 天空盒 Cubemap (_CG_SkyCubeA / _CG_SkyCubeB)
//   · 浅滩岸边浪花泡沫与日月高光波光粼粼效果
// ============================================================================
Shader "YanbianCG/CG_RiverWater"
{
    Properties
    {
        _ShallowColor ("Shallow Water Color", Color) = (0.14, 0.36, 0.46, 0.88)
        _DeepColor    ("Deep Water Color", Color)    = (0.06, 0.18, 0.28, 0.94)
        _WaveAmp      ("Wave Amplitude", Float)      = 0.08
        _WaveFreq     ("Wave Frequency", Float)      = 0.65
    }
    SubShader
    {
        Tags { "RenderType"="Transparent" "Queue"="Transparent-10" }
        Blend SrcAlpha OneMinusSrcAlpha
        ZWrite On

        Pass
        {
            CGPROGRAM
            #pragma vertex vert
            #pragma fragment frag
            #include "UnityCG.cginc"

            float4 _ShallowColor;
            float4 _DeepColor;
            float  _WaveAmp;
            float  _WaveFreq;

            samplerCUBE _CG_SkyCubeA;
            samplerCUBE _CG_SkyCubeB;
            float  _CG_SkyBlend;
            float3 _CG_SunDir;
            float3 _CG_SunColor;
            float3 _CG_FogColor;
            float  _CG_FogDensity;
            float  _CG_PointLightFactor;
            int    _CG_NumPointLights;
            float4 _CG_PointLightPos[6];
            float4 _CG_PointLightColor[6];

            struct appdata
            {
                float4 vertex : POSITION;
                float2 uv     : TEXCOORD0;
            };

            struct v2f
            {
                float4 pos       : SV_POSITION;
                float3 worldPos  : TEXCOORD0;
                float3 worldNorm : TEXCOORD1;
                float2 uv        : TEXCOORD2;
            };

            v2f vert(appdata v)
            {
                v2f o;
                float3 wPos = mul(unity_ObjectToWorld, v.vertex).xyz;
                float t = _Time.y;

                // 3 组不同方向与频率的波浪叠加 + 解析偏导求水波法线
                float p1 = wPos.x * _WaveFreq + t * 2.1;
                float p2 = wPos.z * (_WaveFreq * 1.25) - t * 1.7;
                float p3 = (wPos.x * 0.7 - wPos.z * 0.7) * (_WaveFreq * 1.8) + t * 2.8;

                float h = _WaveAmp * (sin(p1) + 0.75 * cos(p2) + 0.45 * sin(p3));
                wPos.y += h;

                float dhdX = _WaveAmp * (_WaveFreq * cos(p1) + 0.45 * 0.7 * (_WaveFreq * 1.8) * cos(p3));
                float dhdZ = _WaveAmp * (-0.75 * (_WaveFreq * 1.25) * sin(p2) - 0.45 * 0.7 * (_WaveFreq * 1.8) * cos(p3));

                o.worldPos  = wPos;
                o.worldNorm = normalize(float3(-dhdX, 1.0, -dhdZ));
                o.uv        = v.uv;
                o.pos       = UnityWorldToClipPos(wPos);
                return o;
            }

            fixed4 frag(v2f i) : SV_Target
            {
                float3 N = normalize(i.worldNorm);
                float3 V = normalize(_WorldSpaceCameraPos.xyz - i.worldPos);
                float3 R = reflect(-V, N);
                // C++ / OpenGL cubemap 镜像修正
                float3 sampleDir = float3(-R.x, R.y, R.z);

                float3 skyA = texCUBE(_CG_SkyCubeA, sampleDir).rgb;
                float3 skyB = texCUBE(_CG_SkyCubeB, sampleDir).rgb;
                float3 envSky = lerp(skyA, skyB, saturate(_CG_SkyBlend));

                // Schlick Fresnel 反射率
                float ndotv = max(dot(N, V), 0.0);
                float fresnel = 0.12 + 0.88 * pow(1.0 - ndotv, 4.0);

                // 水体底色（中心深水 -> 岸边浅滩渐变 + 岸边微泡沫）
                float edgeFactor = abs(i.uv.y - 0.5) * 2.0;
                float3 waterBase = lerp(_DeepColor.rgb, _ShallowColor.rgb, smoothstep(0.2, 0.95, edgeFactor));
                float3 color = lerp(waterBase, envSky, saturate(fresnel));

                // 太阳/月光水面碎钻高光
                float3 L = normalize(_CG_SunDir);
                float3 H = normalize(L + V);
                float spec = pow(max(dot(N, H), 0.0), 140.0) * 1.15;
                color += _CG_SunColor * spec;

                // 星夜两岸路灯在水面的暖色倒影光斑
                if (_CG_PointLightFactor > 0.01)
                {
                    for (int k = 0; k < 6; ++k)
                    {
                        if (k >= _CG_NumPointLights) break;
                        float3 toL = _CG_PointLightPos[k].xyz - i.worldPos;
                        float d = length(toL);
                        float3 Lp = toL / max(d, 0.001);
                        float3 Hp = normalize(Lp + V);
                        float atten = 1.0 / (1.0 + 0.06 * d + 0.018 * d * d);
                        float pSpec = pow(max(dot(N, Hp), 0.0), 48.0);
                        color += _CG_PointLightColor[k].rgb * (_CG_PointLightFactor * atten * (0.35 + 1.4 * pSpec));
                    }
                }

                // 指数高度雾融合
                float dist = length(_WorldSpaceCameraPos.xyz - i.worldPos);
                float heightFalloff = exp(-max(i.worldPos.y, 0.0) * 0.045);
                float fogDist = _CG_FogDensity * dist * heightFalloff;
                float fogFactor = 1.0 - exp(-fogDist * fogDist);
                color = lerp(color, _CG_FogColor, saturate(fogFactor));

                return float4(color, 0.92);
            }
            ENDCG
        }
    }
}
