// Encodes or decodes a SwapChain that needs sRGB added or removed
//   (primarily used for Flip Model overrides)
//
#pragma warning ( disable : 3571 )

struct PS_INPUT
{
  float4 pos      : SV_POSITION;
  float4 col      : COLOR0;
  float2 uv       : TEXCOORD0;
  float2 coverage : TEXCOORD1;
};

cbuffer srgbTransform : register (b0)
{
  bool passthrough;
  bool apply;
  bool strip;
};

sampler   srgbSampler     : register (s0);
Texture2D srgbFrameBuffer : register (t0);

// Dumbed down version that does not deal with values < 0,
//   since it does not expect to be applied to buffers
//     unclamped outside of [0,1].
float3
RemoveSRGBCurve (float3 x)
{
  return ( x < 0.04045f ) ?
          (x / 12.92f)    :
    pow ( (x + 0.055f) / 1.055f, 2.4f );
}

float3
ApplySRGBCurve (float3 x)
{
  return ( x < 0.0031308f ?
               12.92f * x :
    1.055f * pow ( x, 1.0 / 2.4f ) - 0.55f );
}


float4 main (PS_INPUT input) : SV_TARGET
{
  float4 vLinear =
    srgbFrameBuffer.Sample ( srgbSampler,
                               input.uv );

  if (passthrough)
    return vLinear;

  if (strip)
    vLinear.rgb =
      RemoveSRGBCurve (vLinear.rgb);
  else if (apply)
    vLinear.rgb =
      ApplySRGBCurve (vLinear.rgb);

  return
    float4 ( vLinear );
}