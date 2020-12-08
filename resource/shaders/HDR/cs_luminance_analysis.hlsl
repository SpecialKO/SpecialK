Texture2D <float4>  hdrPixels     : register (t0);
RWByteAddressBuffer lumaHistogram : register (u0);

#define THREADSX 32
#define THREADSY 32

float Luminance (float3 linearRgb)
{
  return
    dot ( linearRgb,
          float3 (0.2126729, 0.7151522, 0.0721750) );
}

float Luminance (float4 linearRgba)
{
  return
    Luminance (linearRgba.rgb);
}

[numthreads (THREADSX, THREADSY, 1)]
void histogramCS ( uint  groupIdx          : SV_GroupIndex,
                   uint3 groupId           : SV_GroupID,
                   uint3 groupThreadId     : SV_GroupThreadID,
                   uint3 dispatchThreadId  : SV_DispatchThreadID )
{
  float4 hdrSample =
    hdrPixels [dispatchThreadId.xy];


  float luma =
    Luminance (hdrSample.rgb);

  lumaHistogram.InterlockedAdd (
    (uint)luma * 4, 1
  );
}