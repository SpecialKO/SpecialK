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
    float new_lum =
      (accum / samples);

    float old0  = texLuminance [uint2 (HISTOGRAM_WORKGROUP_SIZE_X +  0, HISTOGRAM_WORKGROUP_SIZE_Y)];
    float old1  = texLuminance [uint2 (HISTOGRAM_WORKGROUP_SIZE_X +  1, HISTOGRAM_WORKGROUP_SIZE_Y)];
    float old2  = texLuminance [uint2 (HISTOGRAM_WORKGROUP_SIZE_X +  2, HISTOGRAM_WORKGROUP_SIZE_Y)];
    float old3  = texLuminance [uint2 (HISTOGRAM_WORKGROUP_SIZE_X +  3, HISTOGRAM_WORKGROUP_SIZE_Y)];
    float old4  = texLuminance [uint2 (HISTOGRAM_WORKGROUP_SIZE_X +  4, HISTOGRAM_WORKGROUP_SIZE_Y)];
    float old5  = texLuminance [uint2 (HISTOGRAM_WORKGROUP_SIZE_X +  5, HISTOGRAM_WORKGROUP_SIZE_Y)];
    float old6  = texLuminance [uint2 (HISTOGRAM_WORKGROUP_SIZE_X +  6, HISTOGRAM_WORKGROUP_SIZE_Y)];
    float old7  = texLuminance [uint2 (HISTOGRAM_WORKGROUP_SIZE_X +  7, HISTOGRAM_WORKGROUP_SIZE_Y)];
    float old8  = texLuminance [uint2 (HISTOGRAM_WORKGROUP_SIZE_X +  8, HISTOGRAM_WORKGROUP_SIZE_Y)];
    float old9  = texLuminance [uint2 (HISTOGRAM_WORKGROUP_SIZE_X +  9, HISTOGRAM_WORKGROUP_SIZE_Y)];
    float old10 = texLuminance [uint2 (HISTOGRAM_WORKGROUP_SIZE_X + 10, HISTOGRAM_WORKGROUP_SIZE_Y)];
    float old11 = texLuminance [uint2 (HISTOGRAM_WORKGROUP_SIZE_X + 11, HISTOGRAM_WORKGROUP_SIZE_Y)];

    texLuminance [uint2 (HISTOGRAM_WORKGROUP_SIZE_X    , HISTOGRAM_WORKGROUP_SIZE_Y)] =
      ( old0 + old1  + old2 + old3  + old4  + old5  + old6  + old7  +
        old8 + old9 + old10 + old11 + new_lum ) / 13.0;

    texLuminance [uint2 (HISTOGRAM_WORKGROUP_SIZE_X +  1, HISTOGRAM_WORKGROUP_SIZE_Y)] = old0;
    texLuminance [uint2 (HISTOGRAM_WORKGROUP_SIZE_X +  2, HISTOGRAM_WORKGROUP_SIZE_Y)] = old1;
    texLuminance [uint2 (HISTOGRAM_WORKGROUP_SIZE_X +  3, HISTOGRAM_WORKGROUP_SIZE_Y)] = old2;
    texLuminance [uint2 (HISTOGRAM_WORKGROUP_SIZE_X +  4, HISTOGRAM_WORKGROUP_SIZE_Y)] = old3;
    texLuminance [uint2 (HISTOGRAM_WORKGROUP_SIZE_X +  5, HISTOGRAM_WORKGROUP_SIZE_Y)] = old4;
    texLuminance [uint2 (HISTOGRAM_WORKGROUP_SIZE_X +  6, HISTOGRAM_WORKGROUP_SIZE_Y)] = old5;
    texLuminance [uint2 (HISTOGRAM_WORKGROUP_SIZE_X +  7, HISTOGRAM_WORKGROUP_SIZE_Y)] = old6;
    texLuminance [uint2 (HISTOGRAM_WORKGROUP_SIZE_X +  8, HISTOGRAM_WORKGROUP_SIZE_Y)] = old7;
    texLuminance [uint2 (HISTOGRAM_WORKGROUP_SIZE_X +  9, HISTOGRAM_WORKGROUP_SIZE_Y)] = old8;
    texLuminance [uint2 (HISTOGRAM_WORKGROUP_SIZE_X + 10, HISTOGRAM_WORKGROUP_SIZE_Y)] = old9;
    texLuminance [uint2 (HISTOGRAM_WORKGROUP_SIZE_X + 11, HISTOGRAM_WORKGROUP_SIZE_Y)] = old10;
  }
}