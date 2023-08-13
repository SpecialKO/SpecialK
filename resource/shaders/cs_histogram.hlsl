#include "HDR/common_defs.hlsl"

#pragma warning (disable: 3205)

#define ANALYZE_GAMUT
//#define DISPLAY_GAMUT_ON_SCREEN
//#define GENERATE_HISTOGRAM

RWTexture2D<float4>           texBackbufferHDR  : register (u0);
RWTexture2D<float>            texLuminance      : register (u1);
RWTexture2D<float4>           texGamutCoverage  : register (u2);
#ifdef GENERATE_HISTOGRAM
RWStructuredBuffer <tileData> bufTiledHistogram : register (u3);
#endif

groupshared struct {
  uint accum;
  uint pixels;
} counter;

groupshared struct {
  uint width;
  uint height;
} image;

#define tile_data bufTiledHistogram [tileIdxFlattened]

uint
computeBinIdxFromLogY (float logY)
{
  return uint (
    float (NUM_HISTOGRAM_BINS) *
      saturate ( (logY                      - hdrLuminance_Min) /
                 (hdrLuminance_MaxLocal * 2 - hdrLuminance_Min)  )
  );
}

uint
computeBinIdxFromRGB (float3 rgb)
{
  float logY =
    transformRGBtoLogY (rgb);

  return
    computeBinIdxFromLogY (logY);
}

[numthreads (HISTOGRAM_WORKGROUP_SIZE_X, HISTOGRAM_WORKGROUP_SIZE_Y, 1)]
void
LocalHistogramWorker ( uint3 globalIdx : SV_DispatchThreadID,
                       uint3 localIdx  : SV_GroupThreadID,
                       uint3 groupIdx  : SV_GroupID )
{
  const uint subdiv =
    min ( HISTOGRAM_WORKGROUP_SIZE_Y,
          HISTOGRAM_WORKGROUP_SIZE_X );

  uint num_tiles = 0;

  if (! any (localIdx))
  {
    texBackbufferHDR.GetDimensions ( image.width,
                                     image.height );

    num_tiles =      ( image.width * image.height ) /
      ( HISTOGRAM_WORKGROUP_SIZE_X * HISTOGRAM_WORKGROUP_SIZE_Y );

    counter.accum  = 0;
    counter.pixels = 0;

#ifdef SLOW_ANALYZE_GAMUT // Clear this using D3D11/12 API calls, dumbass.
    for ( uint x = 0 ; x < 1024 ; ++x )
    for ( uint y = 0 ; y < 1024 ; ++y )
      texGamutCoverage [uint2 (x, y)] = 0.0f;
#endif
  }

  const uint
    tileIdxFlattened =
      groupIdx.x +
      groupIdx.y * num_tiles;

#ifdef GENERATE_HISTOGRAM
  if (! any (localIdx))
  {
    for (uint idx = 0; idx < NUM_HISTOGRAM_BINS; ++idx)
    {
      tile_data.Histogram [idx] = 0;
    }
  }
#endif

  GroupMemoryBarrierWithGroupSync ();

  if ( globalIdx.x < image.width &&
       globalIdx.y < image.height )
  {
    if ( localIdx.x < subdiv &&
         localIdx.y < subdiv )
    {
      const bool tick_tock =
        ( ( ( currentTime % 125 )
                          / 125.0f ) > 0.5f );

      const uint2 advance =
        uint2 ( (image.width  / subdiv) / HISTOGRAM_WORKGROUP_SIZE_X +
                (image.width  / subdiv) % HISTOGRAM_WORKGROUP_SIZE_X, (image.height / subdiv) / HISTOGRAM_WORKGROUP_SIZE_Y +
                                                                      (image.height / subdiv) % HISTOGRAM_WORKGROUP_SIZE_Y );

      const uint2 origin =
        uint2 ( localIdx.x * advance.x + groupIdx.x * (advance.x * subdiv) + ( tick_tock ? 1 : 0 ),
                localIdx.y * advance.y + groupIdx.y * (advance.y * subdiv) + ( tick_tock ? 0 : 1 ) );

      for ( uint X = 0 ; X < advance.x ; X += tick_tock ? 2 : 3 )
      for ( uint Y = 0 ; Y < advance.y ; Y += tick_tock ? 3 : 2 )
      {
        const uint2 pos =
          uint2 ( origin.x + X,
                  origin.y + Y );
        
        if ( pos.x < image.width &&
             pos.y < image.height )
        {
          const float3 color =
            texBackbufferHDR [pos.xy].rgb;
        
          const float Y =
            transformRGBtoY (color);
        
#ifdef GENERATE_HISTOGRAM
          const uint bin =
            computeBinIdxFromLogY (log (Y));
        
          if (Y < FLT_EPSILON)
            InterlockedAdd (tile_data.blackPixelCount, 1u);
          
          InterlockedMin (tile_data.minLuminance, (uint)(Y * 8192.0));
          InterlockedMax (tile_data.maxLuminance, (uint)(Y * 8192.0));

          InterlockedAdd (tile_data.Histogram [bin], 1u);
#endif
        
          if ((uint)(8192.0 * Y) > 0)
          {
            InterlockedAdd (counter.pixels,                1u);
            InterlockedAdd (counter.accum, (uint)(8192.0 * Y));
          }
        
#ifdef  ANALYZE_GAMUT
          float3 XYZ = sRGBtoXYZ (color.rgb);
          float  xyz = XYZ.x + XYZ.y + XYZ.z;
                        
          float4 normalized_color =
            float4 (color.rgb / xyz, 1.0);
        
          texGamutCoverage [ uint2 (        1024 * XYZ.x / xyz,
                                     1024 - 1024 * XYZ.y / xyz ) ].rgba =
            normalized_color;

#ifdef DISPLAY_GAMUT_ON_SCREEN
          texBackbufferHDR [ uint2 ( image.width  * XYZ.x / xyz,
                      image.height - image.height * XYZ.y / xyz ) ].rgba =
            normalized_color;
#endif
#endif  
        }
      }
    }
  }

  GroupMemoryBarrier ();

  if (! any (localIdx))
  {
    float avg = 0.0f;

    if ( counter.pixels > 0 &&
         counter.accum  > 0 )
    {
      avg =
        (0.0001220703125f * counter.accum)
                          / counter.pixels;
    }

    texLuminance [groupIdx.xy] = avg;

#ifdef GENERATE_HISTOGRAM
    tile_data.avgLuminance     = avg;
    
    tile_data.tileIdx      = groupIdx.xy;
#endif
  //tile_data.pixelOffset  = uint2 (0,0);
  
    //// Gamut coverage counters
    ////
    ////   TODO - Needs to be integer for atomics, but float for math
    //tile_data.minx = 0.0f, tile_data.maxx = 0.0f, tile_data.avgx = 0.0f;
    //tile_data.miny = 0.0f, tile_data.maxy = 0.0f, tile_data.avgy = 0.0f;

    //bufTiledHistogram [tileIdxFlattened] = tile_data;
  }
}