#define NUM_HISTOGRAM_BINS          1024
#define HISTOGRAM_WORKGROUP_SIZE_X ( 32 )
#define HISTOGRAM_WORKGROUP_SIZE_Y ( 16 )

cbuffer histogramDispatch
{
  struct {
    uint width;
    uint height;
  } image;

  struct {
    float min;
    float max;
  } luminance;

  // Gamut coverage counters
  struct {
    float4 minx, maxx;
    float4 miny, maxy;
    float4 minY, maxY;
  } gamut_xyY;

  float3x3 colorspaceMat_RGBtoxyY;
  uint     numLocalWorkgroups;
};

struct SK_ColorSpace
{
  float xr, yr,
        xg, yg,
        xb, yb,
        Xw, Yw, Zw;

  float minY, maxLocalY, maxY;
};

#define D65 0.3127f, 0.329f

static const SK_ColorSpace _sRGB =
{
  0.64f, 0.33f, 0.3f, 0.6f,          0.150f, 0.060f,
    D65, 1.00f,
  0.00f, 0.00f, 0.0f
};

float
transformRGBtoY (float3 rgb)
{
  return
    mul (rgb, colorspaceMat_RGBtoxyY).z;
}

float
transformRGBtoLogY (float3 rgb)
{
  return
    log (transformRGBtoY (rgb));
}

uint
computeBucketIdxFromRGB (float3 rgb)
{
  float logY =
    transformRGBtoLogY (rgb);

  return uint (
    float (NUM_HISTOGRAM_BINS) *
      saturate ( (logY          - luminance.min) /
                 (luminance.max - luminance.min)  )
  );
}

Texture2D texBackbufferHDR;

groupshared uint localHistogram [NUM_HISTOGRAM_BINS];
   Buffer        localHistogramIn;
 RWBuffer <uint> localHistogramOut;

   Buffer        globalHistogramIn;
 RWBuffer <uint> globalHistogramOut;

uint
GetNumTilesX (void)
{
  return (           image.width * image.height ) /
    ( HISTOGRAM_WORKGROUP_SIZE_X * HISTOGRAM_WORKGROUP_SIZE_Y );
}

[numthreads (HISTOGRAM_WORKGROUP_SIZE_X, HISTOGRAM_WORKGROUP_SIZE_Y, 1)]
void
LocalHistogramWorker ( uint3 globalIdx : SV_DispatchThreadID,
                       uint3 localIdx  : SV_GroupThreadID,
                       uint3 groupIdx  : SV_GroupID )
{
  uint pixelIdx = localIdx.x + localIdx.y * HISTOGRAM_WORKGROUP_SIZE_X;
  uint tileIdx  = groupIdx.x + groupIdx.y * GetNumTilesX ();

  if (localIdx.x == 0)
  {
    for (uint idx = 0; idx < NUM_HISTOGRAM_BINS; ++idx)
    {
      localHistogram [idx] = 0;
    }
  }

  GroupMemoryBarrierWithGroupSync ();

  // For each thread, update the histogram

  if ( globalIdx.x < image.width &&
       globalIdx.y < image.height )
  {
    float3 pixelValue =
      texBackbufferHDR.Load (int3 (globalIdx.xy, 0)).rgb;

    uint bin =
      computeBucketIdxFromRGB (pixelValue);

    InterlockedAdd (localHistogram [bin], 1u);
  }

  GroupMemoryBarrierWithGroupSync ();

  if (localIdx.x == 0)
  {
    // This could write uint4s to the UAV as an optimisation
    uint outputHistogramIdx =
                             NUM_HISTOGRAM_BINS * tileIdx;
    for (uint idx = 0; idx < NUM_HISTOGRAM_BINS; ++idx)
    {
      uint outIdx =
        idx + outputHistogramIdx;

      localHistogramOut [outIdx] =
         localHistogram [   idx];
    }
  }
}