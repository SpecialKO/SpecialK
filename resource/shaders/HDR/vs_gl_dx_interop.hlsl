struct PS_INPUT
{
  float4 pos : SV_POSITION;
  float2 uv  : TEXCOORD0;
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
    output.pos =
      float4 ( ( output.uv.x - 0.5f ) * 2,
              -( output.uv.y - 0.5f ) * 2,
                               0.0f,
                               1.0f );

    output.uv.y = 1.0f - output.uv.y;

  return
    output;
}