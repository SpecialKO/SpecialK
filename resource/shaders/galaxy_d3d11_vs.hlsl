#pragma warning ( disable : 3571 )
cbuffer cbPerViewChange : register (b0)
{
   float  g_viewportHeight; // Offset:    0 Size:     4
   float  g_viewportWidth;  // Offset:    4 Size:     4
   float  pad1;             // Offset:    8 Size:     4 [unused]
   float  pad2;             // Offset:   12 Size:     4 [unused]
   float4 Luminance;
};

struct VS_INPUT
{
  float4 pos : POSITION;
  float2 uv  : TEXCOORD;
};

struct PS_INPUT
{
  float4 pos : SV_POSITION;
  float2 uv  : TEXCOORD0;
  float4 col : COLOR;
  float2 uv2 : TEXCOORD1;
};

PS_INPUT main (VS_INPUT input)
{
  PS_INPUT output;

  output.pos = input.pos;
  output.col = float4 (1.0f, 1.0f, 1.0f, 1.0f);
  output.uv  = input.uv;
  output.uv2 = Luminance.xy;

  return output;
};