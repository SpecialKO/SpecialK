#include "common_defs.hlsl"

RWTexture2D             <float>    texLuminance     : register (u1);
ConsumeStructuredBuffer <tileData> statisticsBuffer : register (u3);

[numthreads (1, 1, 1)]
void
LuminanceMerge ( uint3 globalIdx : SV_DispatchThreadID,
                 uint3 localIdx  : SV_GroupThreadID,
                 uint3 groupIdx  : SV_GroupID )
{  
  float samples = 0.0f;
  float accum   = 0.0f;

  for ( uint x = 0 ; x < HISTOGRAM_WORKGROUP_SIZE_X ; ++x )
  for ( uint y = 0 ; y < HISTOGRAM_WORKGROUP_SIZE_Y ; ++y )
  {
    const uint2 currentIdx = uint2 ( x,
                                     y );
    const float currentLum = texLuminance [currentIdx];

    // Per-tile weight?
#ifdef _WEIGHTED
    const uint2 historyIdx =               currentIdx + uint2 (HISTOGRAM_WORKGROUP_SIZE_X, 0);
    const float historyLum = texLuminance [historyIdx];
    
    if ( historyLum > 0.0f )
    {
      ++samples;
      accum += historyLum;
    }
#endif

    if ( currentLum > 0.0f )
    {
      ++samples;
      accum += currentLum;
    }

#ifdef _WEIGHTED
    texLuminance [historyIdx] = currentLum;
#endif
  }

  if ( samples > 0 &&
         accum > 0 )
  {
    float old0 = texLuminance [uint2 (HISTOGRAM_WORKGROUP_SIZE_X + 1, HISTOGRAM_WORKGROUP_SIZE_Y)];
    float old1 = texLuminance [uint2 (HISTOGRAM_WORKGROUP_SIZE_X + 2, HISTOGRAM_WORKGROUP_SIZE_Y)];
    float old2 = texLuminance [uint2 (HISTOGRAM_WORKGROUP_SIZE_X + 3, HISTOGRAM_WORKGROUP_SIZE_Y)];

    texLuminance [uint2 (HISTOGRAM_WORKGROUP_SIZE_X, HISTOGRAM_WORKGROUP_SIZE_Y)] =
      ( accum / samples );

    texLuminance [uint2 (HISTOGRAM_WORKGROUP_SIZE_X + 2, HISTOGRAM_WORKGROUP_SIZE_Y)] =
    texLuminance [uint2 (HISTOGRAM_WORKGROUP_SIZE_X + 1, HISTOGRAM_WORKGROUP_SIZE_Y)];

    texLuminance [uint2 (HISTOGRAM_WORKGROUP_SIZE_X + 1, HISTOGRAM_WORKGROUP_SIZE_Y)] =
      ( ( accum / samples ) * 8.0f + old0 * 4.0f + old1 * 2.0f + old2 * 1.0f) / 15.0;

    texLuminance [uint2 (HISTOGRAM_WORKGROUP_SIZE_X + 3, HISTOGRAM_WORKGROUP_SIZE_Y)] =
      old1;
  }
}