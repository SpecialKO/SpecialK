#pragma warning ( disable : 3571 )

#include "HDR/common_defs.hlsl"

Texture2D    t0 : register (t0);
SamplerState s0 : register (s0);

void
main ( float4 vpos : SV_POSITION,
       float4 vcol : COLOR0,
       float2 uv   : TEXCOORD0,
       float2 luma : TEXCOORD1,
   out float4 col  : SV_TARGET )
{
  float4 tcol       = t0.Sample (s0, uv);

  float4 gamma_exp  = float4 (luma.yyy, 1.f);
  float4 linear_mul = float4 (luma.xxx, 1.f);

  // Blend vertex color and texture; alpha is linear
  col =
    float4 (RemoveSRGBCurve (tcol.rgb * vcol.rgb),
                             tcol.a   * vcol.a);

  // Negative = HDR10
  if (linear_mul.x < 0.0)
  {
    col.rgb =
      ApplyREC2084Curve ( REC709toREC2020 (col.rgb),
                            -linear_mul.x );

  }

  // Positive = scRGB
  else
    col *= linear_mul;
}