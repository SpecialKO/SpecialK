#pragma warning ( disable : 3571 )
struct PS_INPUT
{
  float4 pos : SV_POSITION;
  float2 uv  : TEXCOORD0;
  float2 uv2 : TEXCOORD1;
};

sampler   PS_QUAD_Sampler   : register (s0);
Texture2D PS_QUAD_Texture2D : register (t0);

float3 RemoveSRGBCurve(float3 x)
{
  /* Approximately pow(x, 2.2)*/
  return x < 0.04045 ? x / 12.92 :
                  pow((x + 0.055) / 1.055, 2.4);
}

float4 main (PS_INPUT input) : SV_Target
{
  float4 gamma_exp  = float4 (input.uv2.yyy, 1.f);
  float4 linear_mul = float4 (input.uv2.xxx, 1.f);

  float4 out_col =
    PS_QUAD_Texture2D.Sample (PS_QUAD_Sampler, input.uv);

  out_col =
    float4 (RemoveSRGBCurve (out_col.rgb),out_col.a)  *
                                                 linear_mul;

  return
    float4 (out_col.rgb, saturate (out_col.a));
}