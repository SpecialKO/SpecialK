
cbuffer vertexBuffer : register (b0)
{
  float4x4 ProjectionMatrix;
  float4   Luminance;
  float4   SteamLuminance;
};

struct VS_INPUT
{
  float2 pos : POSITION;
  float4 col : COLOR0;
  float2 uv  : TEXCOORD0;
};

struct PS_INPUT
{
  float4 pos : SV_POSITION;
  float4 col : COLOR0;
  float2 uv  : TEXCOORD0;
  float2 uv2 : TEXCOORD1;
  float2 uv3 : TEXCOORD2;
};

PS_INPUT main (VS_INPUT input)
{
  PS_INPUT output;

  output.pos  = mul ( ProjectionMatrix,
                        float4 (input.pos.xy, 0.f, 1.f) );

  output.uv  = saturate (input.uv);

  output.col = input.col;
  output.uv2 = float2 (0.f, 0.f);
  output.uv3 = Luminance.xy;

  return output;
}