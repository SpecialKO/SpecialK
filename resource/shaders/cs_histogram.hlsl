#include "HDR/common_defs.hlsl"

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

RWTexture2D<float4>           texBackbufferHDR  : register (u0);
RWTexture2D<float>            texLuminance      : register (u1);
RWTexture2D<float4>           texGamutCoverage  : register (u2);
RWStructuredBuffer <tileData> bufTiledHistogram : register (u3);

groupshared uint              accum        = 0;
groupshared uint              pixels       = 0;
groupshared uint              image_width  = 0;
groupshared uint              image_height = 0;

#define tile_data bufTiledHistogram [tileIdxFlattened]

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
    texBackbufferHDR.GetDimensions ( image_width,
                                     image_height );

    num_tiles =      ( image_width * image_height ) /
      ( HISTOGRAM_WORKGROUP_SIZE_X * HISTOGRAM_WORKGROUP_SIZE_Y );

#if 0
    for (uint idx = 0; idx < NUM_HISTOGRAM_BINS; ++idx)
    {
      localHistogram [idx] = 0;
    }
#endif

    accum  = 0;
    pixels = 0;

#if 0
    if (tileIdx == 0)
    {
      for ( uint x = 0 ; x < 1024 ; ++x )
      for ( uint y = 0 ; y < 1024 ; ++y )
        texGamutCoverage [uint2 (x, y)] = 0.0f;
    }
#endif
  }

  const uint
    tileIdxFlattened =
      groupIdx.x +
      groupIdx.y * num_tiles;

  GroupMemoryBarrier ();

  if ( globalIdx.x < image_width &&
       globalIdx.y < image_height )
  {
    if ( localIdx.x < subdiv &&
         localIdx.y < subdiv )
    {
      const uint x_advance = (image_width  / subdiv) / HISTOGRAM_WORKGROUP_SIZE_X +
                             (image_width  / subdiv) % HISTOGRAM_WORKGROUP_SIZE_X,
                 y_advance = (image_height / subdiv) / HISTOGRAM_WORKGROUP_SIZE_Y +
                             (image_height / subdiv) % HISTOGRAM_WORKGROUP_SIZE_Y;

      const uint x_origin = localIdx.x * x_advance + groupIdx.x * (x_advance * subdiv),
                 y_origin = localIdx.y * y_advance + groupIdx.y * (y_advance * subdiv);

                                                  //[unroll (8)]
      for ( uint X = 0 ; X < x_advance ; X += 3 ) //[unroll (4)]
      for ( uint Y = 0 ; Y < y_advance ; Y += 3 )
      {
        const uint2 pos =
          uint2 ( x_origin + X,
                  y_origin + Y );
        
        if ( pos.x < image_width &&
             pos.y < image_height )
        {
          const float3 color =
            texBackbufferHDR [pos.xy].rgb;
        
          const float Y =
            transformRGBtoY (color);
        
          const uint bin =
            computeBinIdxFromLogY (log (Y));
        
          if (Y < FLT_EPSILON)
            InterlockedAdd (tile_data.blackPixelCount, 1u);
        
          InterlockedMin (tile_data.minLuminance, (uint)(Y * 8192.0));
          InterlockedMax (tile_data.maxLuminance, (uint)(Y * 8192.0));
          InterlockedAdd (tile_data.Histogram [bin], 1u);
        
          if ((uint)(8192.0 * Y) > 0)
          {
            InterlockedAdd (pixels,                1u);
            InterlockedAdd (accum, (uint)(8192.0 * Y));
          }
        
#if 0   
          float3 xyY =
            SK_Color_xyY_from_RGB ( _sRGB, color / dot (color, float3 (0.2126729, 0.7151522, 0.0721750)) );
        
          //xyY.xy =
          //  saturate (xyY.xy);
        
          texGamutCoverage [uint2 ( 1024 * xyY.z,
                                    1024 -  1024 * xyY.y )].rgba =
            float4 (color.rgb, 1.0);
        
          texBackbufferHDR [uint2 (image_width * xyY.z, image_height - image_height * xyY.y)/*pos.xy*/].rgba =
            float4 (color.rgb, 1.0);
#endif  
        }
      }
    }
  }

  GroupMemoryBarrier ();

#if 1
  if (! any (localIdx))
  {
    float avg = 0.0f;

    if ( pixels > 0 &&
         accum  > 0 )
    {
      avg =
        (0.000122f * (float)accum)
                   / (float)pixels;
    }

    texLuminance [ uint2 ( groupIdx.x,
                           groupIdx.y ) ] = avg;

    tile_data.avgLuminance = avg;

    tile_data.tileIdx      = uint2 (groupIdx.x, groupIdx.y);
  //tile_data.pixelOffset  = uint2 (0,0);
  
    //// Gamut coverage counters
    ////
    ////   TODO - Needs to be integer for atomics, but float for math
    //tile_data.minx = 0.0f, tile_data.maxx = 0.0f, tile_data.avgx = 0.0f;
    //tile_data.miny = 0.0f, tile_data.maxy = 0.0f, tile_data.avgy = 0.0f;

    //bufTiledHistogram [tileIdxFlattened] = tile_data;
  }
#endif
}