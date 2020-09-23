#pragma warning ( disable : 3571 )
struct PS_INPUT
{
  float4 pos : SV_POSITION;
  float4 col : COLOR0;
  float2 uv  : TEXCOORD0;
  float2 uv2 : TEXCOORD1;
  float2 uv3 : TEXCOORD2;
};

cbuffer viewportDims : register (b0)
{
  float4 viewport;
};

sampler   sampler0    : register (s0);

Texture2D texture0    : register (t0);
Texture2D hdrUnderlay : register (t1);
Texture2D hdrHUD      : register (t2);

float3 RemoveSRGBCurve (float3 x)
{
  /* Approximately pow(x, 2.2)*/
  return x < 0.04045 ? x / 12.92 : pow((x + 0.055) / 1.055, 2.4);
}

float3 ApplyREC709Curve (float3 x)
{
  return x < 0.0181 ? 4.5 * x : 1.0993 * pow(x, 0.45) - 0.0993;
}
float Luma (float3 color)
{
  return
    dot (color, float3 (0.299f, 0.587f, 0.114f));
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
float3 RemoveREC2084Curve (float3 N)
{
  float  m1 = 2610.0 / 4096.0 / 4;
  float  m2 = 2523.0 / 4096.0 * 128;
  float  c1 = 3424.0 / 4096.0;
  float  c2 = 2413.0 / 4096.0 * 32;
  float  c3 = 2392.0 / 4096.0 * 32;
  float3 Np = pow (N, 1 / m2);

  return
    pow (max (Np - c1, 0) / (c2 - c3 * Np), 1 / m1);
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

float3 REC2020toREC709 (float3 RGB2020)
{
  static const float3x3 ConvMat =
  {
     1.660496, -0.587656, -0.072840,
    -0.124546,  1.132895,  0.008348,
    -0.018154, -0.100597,  1.118751
  };
  return mul (ConvMat, RGB2020);
}

float4 main (PS_INPUT input) : SV_Target
{
  float4 out_col =
    texture0.Sample (sampler0, input.uv);

  bool hdr10 = ( input.uv3.x < 0.0 );

  if (viewport.z > 0.f)
  {
    float4 under_color;

    float blend_alpha =
      saturate (input.col.a * out_col.a);

    if (abs (blend_alpha) < 0.001f) blend_alpha = 0.0f;
    if (abs (blend_alpha) > 0.999f) blend_alpha = 1.0f;

    float alpha =
      blend_alpha;

    float4 hud = float4 (0.0f, 0.0f, 0.0f, 0.0f);

    if (input.uv2.x > 0.0f && input.uv2.y > 0.0f)
    {
      hud = hdrHUD.Sample (sampler0, input.uv);
      hud.rbg     = RemoveSRGBCurve (hud.rgb);
      hud.rgb    *= ( input.uv3.xxx );
      out_col.rgb = RemoveSRGBCurve (out_col.rgb);
      out_col =
        pow (abs (out_col), float4 (input.uv2.yyy, 1.0f)) *
          input.uv2.xxxx;
      out_col.a   = 1.0f;
      blend_alpha = 1.0f;
      under_color = float4 (0.0f, 0.0f, 0.0f, 0.0f);
    }

    else
    {
      under_color =
        float4 ( hdrUnderlay.Sample ( sampler0, input.pos.xy /
                                                viewport.zw ).rgb, 1.0);
      out_col =
        pow (abs (input.col * out_col), float4 (input.uv3.yyy, 1.0f));

      if (hdr10)
      {
        under_color.rgb =
          REC2020toREC709 (
            ( RemoveREC2084Curve (12.5f * under_color.rgb) )
          );

        blend_alpha =
          saturate (
            Luma   (
              ApplyREC2084Curve (
                float3 ( blend_alpha, blend_alpha, blend_alpha ),
                                                  -input.uv3.x )
            )
          );
      }

      if (! hdr10)
      {
        blend_alpha =
          saturate (
            Luma   (
              ApplyREC709Curve (
                    float3 ( blend_alpha, blend_alpha, blend_alpha )
                               )
            )
          );

        under_color.rgb = 0.0f;
      }

      out_col.rgb *= blend_alpha;
    }

    float4 final =
      float4 (                         out_col.rgb +
            (1.0f - blend_alpha) * under_color.rgb,
          saturate (blend_alpha));

    if (hdr10)
    {
      final.rgb =
        ApplyREC2084Curve ( REC709toREC2020 (final.rgb),
                              -input.uv3.x );
    }

    else
      final.rgb *= input.uv3.xxx;

    return final;
  }

  return
    ( input.col * out_col );
};