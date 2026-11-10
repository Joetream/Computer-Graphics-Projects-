// ============================================================================
// WP-U3 · Assets/Resources/Shaders/CG_PlanarShadow.shader
// 计算机图形学课程核心：齐次平面投影阴影矩阵 M_shadow = (N·L)I - L*N^T
// 配合 Stencil Buffer 防止同一像素二次混合（无双重暗块伪影），供答辩与 CSM 软阴影实时对比。
// ============================================================================
Shader "YanbianCG/CG_PlanarShadow"
{
    Properties
    {
        _ShadowColor ("Planar Shadow Color", Color) = (0.12, 0.15, 0.24, 0.42)
    }
    SubShader
    {
        Tags { "Queue"="Transparent-5" "RenderType"="Transparent" }
        Blend SrcAlpha OneMinusSrcAlpha
        ZWrite Off
        Offset -1, -1

        Stencil
        {
            Ref 0
            Comp Equal
            Pass IncrSat
        }

        Pass
        {
            CGPROGRAM
            #pragma vertex vert
            #pragma fragment frag
            #include "UnityCG.cginc"

            float4x4 _CG_PlanarShadowMat;
            float4   _ShadowColor;

            struct appdata
            {
                float4 vertex : POSITION;
            };

            struct v2f
            {
                float4 pos      : SV_POSITION;
                float3 worldPos : TEXCOORD0;
            };

            v2f vert(appdata v)
            {
                v2f o;
                float4 wPos = mul(unity_ObjectToWorld, v.vertex);
                float4 shadowWorld = mul(_CG_PlanarShadowMat, wPos);
                shadowWorld.xyz /= max(shadowWorld.w, 1e-5);
                shadowWorld.w = 1.0;

                o.worldPos = shadowWorld.xyz;
                o.pos = UnityWorldToClipPos(shadowWorld.xyz);
                return o;
            }

            fixed4 frag(v2f i) : SV_Target
            {
                // 限制在近景村落核心区 R <= 36m 圆盘内
                if (length(i.worldPos.xz) > 36.0) discard;
                return _ShadowColor;
            }
            ENDCG
        }
    }
}
