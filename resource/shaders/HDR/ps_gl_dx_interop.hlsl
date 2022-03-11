// Flips an OpenGL Framebuffer
//   (primarily used for Flip Model overrides)
//
#pragma warning ( disable : 3571 )

struct PS_INPUT
{
  //float4 pos : SV_POSITION;
  //float2 uv  : TEXCOORD0;
  float4 pos      : SV_POSITION;
  float4 color    : COLOR0;
  float2 uv       : TEXCOORD0;
  float2 coverage : TEXCOORD1;
};

sampler   Sampler     : register (s0);
Texture2D FrameBuffer : register (t0);

float4 main (PS_INPUT input) : SV_TARGET
{
  return
    FrameBuffer.Sample ( Sampler,
                           input.uv );
}