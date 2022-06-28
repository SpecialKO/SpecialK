cbuffer vertexBuffer : register (b0)
{
  float4 Luminance;
};

struct PS_INPUT
{
  float4 pos      : SV_POSITION;
  float4 col      : COLOR0;
  float2 coverage : COLOR1;
  float2 uv       : TEXCOORD0;
};

struct VS_INPUT
{
  uint vI : SV_VERTEXID;
};

PS_INPUT main ( VS_INPUT input )
{
  PS_INPUT
    output;

    output.uv  = float2 (input.vI  & 1,
                         input.vI >> 1);
    output.col = float4 (Luminance.rgb,1);
    output.pos =
      float4 ( ( output.uv.x - 0.5f ) * 2,
              -( output.uv.y - 0.5f ) * 2,
                               0.0f,
                               1.0f );

    output.coverage =
      float2 ( (Luminance.z * .5f + .5f),
               (Luminance.w * .5f + .5f) );

  return
    output;
}