#pragma warning ( disable : 3571 )
cbuffer vertexBuffer : register (b0)
{
  float4x4 ProjectionMatrix;
  float4   Luminance;
  float4   EpicLuminance;
};

struct VS_INPUT
{
  float2 uv : TEXCOORD;
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

  output.pos.x = input.uv.x *  2.0 - 1.0;
  output.pos.y = input.uv.y * -2.0 + 1.0;
  output.pos.z =                     0.0;
  output.pos.w =                     1.0;
  output.col   = float4 (1.0, 1.0, 1.0, 1.0);
  output.uv    = input.uv;
  output.uv2   = EpicLuminance.xy;

  return output;
};