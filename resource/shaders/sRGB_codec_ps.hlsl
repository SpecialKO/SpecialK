// Encodes or decodes a SwapChain that needs sRGB added or removed
//   (primarily used for Flip Model overrides)
//
#pragma warning ( disable : 3571 )

#include "HDR/common_defs.hlsl"

struct PS_INPUT
{
  float4 pos      : SV_POSITION;
  float4 col      : COLOR0;
  float2 coverage : COLOR1;
  float2 uv       : TEXCOORD0;
};

cbuffer srgbTransform : register (b0)
{
  bool passthrough;
  bool apply;
  bool strip;
  bool no_alpha;
};

sampler   srgbSampler     : register (s0);
Texture2D srgbFrameBuffer : register (t0);

float4 main (PS_INPUT input) : SV_TARGET
{
  float4 vLinear =
    srgbFrameBuffer.Sample ( srgbSampler,
                               input.uv );

  vLinear =
    float4 ( (! IsNan (vLinear.r)) * (! IsInf (vLinear.r)) * vLinear.r,
             (! IsNan (vLinear.g)) * (! IsInf (vLinear.g)) * vLinear.g,
             (! IsNan (vLinear.b)) * (! IsInf (vLinear.b)) * vLinear.b,
             (! IsNan (vLinear.a)) * (! IsInf (vLinear.a)) * vLinear.a );

  vLinear.rgba =
    float4 ( clamp (vLinear.rgb, 0.0f, 125.0f),
          saturate (vLinear.a) );

  if (passthrough)
    return vLinear;

  if (strip)
    vLinear.rgb =
      RemoveSRGBCurve (vLinear.rgb);
  else if (apply)
    vLinear.rgb =
      ApplySRGBCurve (vLinear.rgb);

  if (no_alpha)
    vLinear.a = 1.0;

  return
    float4 ( vLinear );
}