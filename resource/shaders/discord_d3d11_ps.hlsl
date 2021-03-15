#pragma warning ( disable : 3571 )
struct PS_INPUT
{
  float4 pos : SV_POSITION;
  float4 col : COLOR;
  float2 uv  : TEXCOORD0;
  float2 uv2 : TEXCOORD1;
};

sampler   samLinear : register (s0);
Texture2D txScreen  : register (t0);

float3 RemoveSRGBCurve(float3 x)
{
  /* Approximately pow(x, 2.2)*/
  return x < 0.04045 ? x / 12.92 :
                  pow((x + 0.055) / 1.055, 2.4);
}

float3 ApplyREC2084Curve (float3 L, float maxLuminance)
{
  float m1 = 2610.0 / 4096.0 / 4;
  float m2 = 2523.0 / 4096.0 * 128;
  float c1 = 3424.0 / 4096.0;
  float c2 = 2413.0 / 4096.0 * 32;
  float c3 = 2392.0 / 4096.0 * 32;

  float maxLuminanceScale = maxLuminance / 10000.0f;
  L *= maxLuminanceScale;

  float3 Lp = pow (L, m1);

  return pow ((c1 + c2 * Lp) / (1 + c3 * Lp), m2);
}

float3 REC709toREC2020 (float3 RGB709)
{
  static const float3x3 ConvMat =
  {
    0.627402, 0.329292, 0.043306,
    0.069095, 0.919544, 0.011360,
    0.016394, 0.088028, 0.895578
  };
  return mul (ConvMat, RGB709);
}

float4 main (PS_INPUT input) : SV_Target
{
  float4 gamma_exp  = float4 (input.uv2.yyy, 1.f);
  float4 linear_mul = float4 (input.uv2.xxx, 1.f);

  float4 out_col =
    txScreen.Sample (samLinear, input.uv);

  out_col =
    float4 (RemoveSRGBCurve (input.col.rgb * out_col.rgb),
                             input.col.a   * out_col.a);

  // Negative = HDR10
  if (linear_mul.x < 0.0)
  {
    out_col.rgb =
      ApplyREC2084Curve ( REC709toREC2020 (out_col.rgb),
                            -linear_mul.x );

  }

  // Positive = scRGB
  else
    out_col *= linear_mul;

  return
    float4 (out_col.rgb, saturate (out_col.a));
}