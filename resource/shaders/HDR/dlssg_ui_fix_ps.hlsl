// Outputs transparent pixels where the UI and HUDLess buffers have
//   identical pixels in order to workaround an Unreal Engine bug.
//
#pragma warning ( disable : 3571 )

struct PS_INPUT
{
  float4 pos      : SV_POSITION;
  float4 col      : COLOR0;
  float2 coverage : COLOR1;
  float2 uv       : TEXCOORD0;
};

sampler   Sampler       : register (s0);
Texture2D UIBuffer      : register (t0);
Texture2D HUDLessBuffer : register (t1);

float4 main (PS_INPUT input) : SV_TARGET
{
  float4 vUI =
    UIBuffer.Sample (Sampler, input.uv);

  float4 vHUDLess =
    HUDLessBuffer.Sample (Sampler, input.uv);

  if ( abs (vUI.r - vHUDLess.r) < 0.0001 &&
       abs (vUI.g - vHUDLess.g) < 0.0001 &&
       abs (vUI.b - vHUDLess.b) < 0.0001 )
  {
    return
      float4 (0.0f, 0.0f, 0.0f, 0.0f);
  }

  return vUI;
}