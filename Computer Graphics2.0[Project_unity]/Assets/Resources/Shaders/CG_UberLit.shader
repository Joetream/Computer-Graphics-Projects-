// ============================================================================
// WP-U3 · Assets/Resources/Shaders/CG_UberLit.shader
// 计算机图形学课程核心着色器（Unity HLSL + ShadowCaster/ShadowCollector）：
//   · _CG_ShadeMode = 0 : Flat Shading（利用 ddx/ddy 屏幕空间偏导求几何棱面法线）
//   · _CG_ShadeMode = 1 : Gouraud Shading（顶点阶段计算 Blinn-Phong 并插值）
//   · _CG_ShadeMode = 2 : Blinn-Phong Per-Pixel Shading（逐片元高光 + 实时软阴影）
//   · 叠加初冬瑞雪积雪混合 (_CG_SnowAmount)、星夜韩纸窗暖光自发光与指数高度雾。
// ============================================================================
Shader "YanbianCG/CG_UberLit"
{
    Properties
    {
        _MainTex ("Albedo Texture", 2D) = "white" {}
        _UseTex ("Use Albedo Texture", Float) = 0.0
        _BaseColor ("Base Color Tint", Color) = (1, 1, 1, 1)
        _Shininess ("Specular Shininess", Range(4, 128)) = 36.0
        _SpecStrength ("Specular Strength", Range(0, 1)) = 0.22
        _EmissiveColor ("Night Emissive Color", Color) = (0, 0, 0, 0)
        _WindAnim ("Vertex Wind Wave (Rice)", Float) = 0.0
    }
    SubShader
    {
        Tags { "RenderType"="Opaque" "Queue"="Geometry" }
        LOD 200
        Cull Off

        Pass
        {
            Name "FORWARD_CG_UBER"
            Tags { "LightMode"="ForwardBase" }

            CGPROGRAM
            #pragma vertex vert
            #pragma fragment frag
            #pragma multi_compile_fwdbase
            #pragma multi_compile_instancing
            #include "UnityCG.cginc"
            #include "AutoLight.cginc"

            sampler2D _MainTex;
            float4 _MainTex_ST;
            float _UseTex;
            float4 _BaseColor;
            float _Shininess;
            float _SpecStrength;
            float4 _EmissiveColor;
            float _WindAnim;

            // 全局课程 Uniform（由 TimeOfDayController 与 DefenseHudController 驱动）
            int   _CG_ShadeMode;          // 0=Flat, 1=Gouraud, 2=Blinn-Phong
            float _CG_SnowAmount;         // 0..1 初冬瑞雪覆盖度
            float3 _CG_SunDir;            // 指向太阳/月亮的单位向量
            float3 _CG_SunColor;
            float3 _CG_Ambient;
            float3 _CG_FogColor;
            float  _CG_FogDensity;
            float  _CG_PointLightFactor;  // 星夜庭院灯强度系数
            int    _CG_NumPointLights;
            float4 _CG_PointLightPos[6];
            float4 _CG_PointLightColor[6];

            struct appdata
            {
                float4 vertex : POSITION;
                float3 normal : NORMAL;
                float2 uv     : TEXCOORD0;
                float4 color  : COLOR;
                UNITY_VERTEX_INPUT_INSTANCE_ID
            };

            struct v2f
            {
                float4 pos            : SV_POSITION;
                float3 worldPos       : TEXCOORD0;
                float3 worldNormal    : TEXCOORD1;
                float2 uv             : TEXCOORD2;
                float4 color          : COLOR;
                float3 gouraudDiffuse : TEXCOORD3;
                float3 gouraudSpec    : TEXCOORD4;
                SHADOW_COORDS(5)
                UNITY_VERTEX_OUTPUT_STEREO
            };

            void EvaluateCGBlinnPhong(
                float3 worldPos, float3 N, float3 V, float shadowAtten,
                out float3 outDiffuse, out float3 outSpec)
            {
                float3 L = normalize(_CG_SunDir);
                float3 H = normalize(L + V);
                float ndotl = max(dot(N, L), 0.0);
                float ndoth = max(dot(N, H), 0.0);

                outDiffuse = _CG_Ambient + _CG_SunColor * (ndotl * shadowAtten);
                outSpec    = _CG_SunColor * (pow(ndoth, max(_Shininess, 1.0)) * _SpecStrength * (ndotl > 0.0 ? shadowAtten : 0.0));

                // 叠加 6 盏延边星夜庭院暖光点光源
                if (_CG_PointLightFactor > 0.001)
                {
                    for (int i = 0; i < 6; ++i)
                    {
                        if (i >= _CG_NumPointLights) break;
                        float3 toLight = _CG_PointLightPos[i].xyz - worldPos;
                        float dist = length(toLight);
                        float3 Lp = toLight / max(dist, 0.001);
                        float atten = 1.0 / (1.0 + 0.09 * dist + 0.032 * dist * dist);
                        float pNdotL = max(dot(N, Lp), 0.0);
                        float3 Hp = normalize(Lp + V);
                        float pNdotH = max(dot(N, Hp), 0.0);

                        float3 pCol = _CG_PointLightColor[i].rgb * (_CG_PointLightFactor * atten);
                        outDiffuse += pCol * pNdotL;
                        outSpec    += pCol * (pow(pNdotH, max(_Shininess, 1.0)) * _SpecStrength);
                    }
                }
            }

            v2f vert(appdata v)
            {
                v2f o;
                UNITY_SETUP_INSTANCE_ID(v);
                UNITY_INITIALIZE_VERTEX_OUTPUT_STEREO(o);

                float4 localPos = v.vertex;
                float3 worldOrigin = mul(unity_ObjectToWorld, float4(0, 0, 0, 1)).xyz;

                // 金秋稻穗顶点风浪摆动动画
                if (_WindAnim > 0.5 && localPos.y > 0.05)
                {
                    float sway = sin(_Time.y * 2.6 + worldOrigin.x * 0.45 + worldOrigin.z * 0.35) * 0.14 * localPos.y;
                    localPos.x += sway;
                    localPos.z += sway * 0.5;
                }

                float4 wPos = mul(unity_ObjectToWorld, localPos);
                o.worldPos = wPos.xyz;
                o.worldNormal = UnityObjectToWorldNormal(v.normal);
                o.uv = TRANSFORM_TEX(v.uv, _MainTex);
                o.color = v.color * _BaseColor;
                o.pos = UnityWorldToClipPos(wPos.xyz);

                // Gouraud 模式：在顶点着色器内完成光照求值
                float3 N = normalize(o.worldNormal);
                float3 V = normalize(_WorldSpaceCameraPos.xyz - o.worldPos);
                EvaluateCGBlinnPhong(o.worldPos, N, V, 1.0, o.gouraudDiffuse, o.gouraudSpec);

                TRANSFER_SHADOW(o);
                return o;
            }

            fixed4 frag(v2f i) : SV_Target
            {
                float4 texSample = (_UseTex > 0.5) ? tex2D(_MainTex, i.uv) : float4(1, 1, 1, 1);
                float3 albedo = texSample.rgb * i.color.rgb;
                float alpha = texSample.a * i.color.a;
                clip(alpha - 0.05);

                float3 interpN = normalize(i.worldNormal);
                float3 N = interpN;

                // ShadeMode 0: Flat Shading（屏幕空间偏导叉乘求三角面几何法线）
                if (_CG_ShadeMode == 0)
                {
                    float3 dpdx = ddx(i.worldPos);
                    float3 dpdy = ddy(i.worldPos);
                    float3 faceN = cross(dpdx, dpdy);
                    if (dot(faceN, faceN) > 1e-12)
                    {
                        faceN = normalize(faceN);
                        if (dot(faceN, interpN) < 0.0) faceN = -faceN;
                        N = faceN;
                    }
                }

                // 初冬瑞雪：朝上表面平滑混合瑞雪白
                if (_CG_SnowAmount > 0.01)
                {
                    float snowMask = smoothstep(0.42, 0.82, N.y) * saturate(_CG_SnowAmount);
                    albedo = lerp(albedo, float3(0.93, 0.96, 1.00), snowMask);
                }

                float3 V = normalize(_WorldSpaceCameraPos.xyz - i.worldPos);
                float shadowAtten = SHADOW_ATTENUATION(i);

                float3 diffuse = 0;
                float3 spec = 0;
                if (_CG_ShadeMode == 1)
                {
                    diffuse = i.gouraudDiffuse * lerp(0.55, 1.0, shadowAtten);
                    spec    = i.gouraudSpec * shadowAtten;
                }
                else
                {
                    EvaluateCGBlinnPhong(i.worldPos, N, V, shadowAtten, diffuse, spec);
                }

                float3 color = albedo * diffuse + spec + _EmissiveColor.rgb * _CG_PointLightFactor;

                // 指数高度雾
                float dist = length(_WorldSpaceCameraPos.xyz - i.worldPos);
                float heightFalloff = exp(-max(i.worldPos.y, 0.0) * 0.045);
                float fogDist = _CG_FogDensity * dist * heightFalloff;
                float fogFactor = 1.0 - exp(-fogDist * fogDist);
                color = lerp(color, _CG_FogColor, saturate(fogFactor));

                return float4(color, alpha);
            }
            ENDCG
        }

        // ShadowCaster Pass 供 Unity 级联阴影贴图 (CSM) 投射真实软阴影
        Pass
        {
            Name "ShadowCaster"
            Tags { "LightMode"="ShadowCaster" }

            ZWrite On ZTest LEqual Cull Off

            CGPROGRAM
            #pragma vertex vertShadow
            #pragma fragment fragShadow
            #pragma multi_compile_shadowcaster
            #pragma multi_compile_instancing
            #include "UnityCG.cginc"

            struct v2fShadow
            {
                V2F_SHADOW_CASTER;
            };

            v2fShadow vertShadow(appdata_base v)
            {
                v2fShadow o;
                UNITY_SETUP_INSTANCE_ID(v);
                TRANSFER_SHADOW_CASTER_NORMALOFFSET(o)
                return o;
            }

            float4 fragShadow(v2fShadow i) : SV_Target
            {
                SHADOW_CASTER_FRAGMENT(i)
            }
            ENDCG
        }
    }
}
