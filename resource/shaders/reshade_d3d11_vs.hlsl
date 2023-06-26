struct VS_INPUT
{
  float2 pos : POSITION;
  float4 col : COLOR0;
  float2 tex : TEXCOORD0;
};

struct PS_INPUT
{
  float4 pos  : SV_POSITION;
  float4 col  : COLOR0;
  float2 tex  : TEXCOORD0;
  float2 luma : TEXCOORD1;
};

cbuffer cb0 : register (b0)
{
  float4x4 ProjectionMatrix;
};

cbuffer cb1 : register (b1)
{
  float4x4 Unused;
  float4   Luminance;               // N/A
  float4   SteamLuminance;          // N/A
  float4   DiscordAndRTSSLuminance; // N/A
  float4   ReShadeLuminance;        // x: Negative = PQ Whitepoint
                                    // x: Positive = scRGB Whitepoint
};

void main (VS_INPUT input, out PS_INPUT output)
{
  output.pos = mul (ProjectionMatrix, float4 (input.pos.xy, 0.0, 1.0));
  output.col = input.col;
  output.tex = input.tex;

  output.luma = ReShadeLuminance.xy;
}