#pragma warning ( disable : 3571 )
cbuffer vertexBuffer : register (b0)
{
  float4x4 ProjectionMatrix;
  float4   Luminance;
  float4   SteamLuminance;
};

struct VS_INPUT
{
  float4 pos : POSITION;
  float4 col : COLOR;
  float2 uv  : TEXCOORD;
};

struct PS_INPUT
{
  float4 pos : SV_POSITION;
  float4 col : COLOR;
  float2 uv  : TEXCOORD0;
  float2 uv2 : TEXCOORD1;
};

PS_INPUT main (VS_INPUT input)
{
  PS_INPUT output;

  output.pos = input.pos;
  output.col = input.col;
  output.uv  = input.uv;
  output.uv2 = SteamLuminance.xy;

  return output;
};