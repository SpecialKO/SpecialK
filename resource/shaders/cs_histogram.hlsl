#define NUM_HISTOGRAM_BINS          384
#define HISTOGRAM_WORKGROUP_SIZE_X ( 32 )
#define HISTOGRAM_WORKGROUP_SIZE_Y ( 16 )

#define FLT_EPSILON     1.192092896e-07 // Smallest positive number, such that 1.0 + FLT_EPSILON != 1.0
#define FLT_MIN         1.175494351e-38 // Minimum representable positive floating-point number
#define FLT_MAX         3.402823466e+38 // Maximum representable floating-point number

cbuffer colorSpaceTransform : register (b0)
{
  uint3  visualFunc;

  float  hdrSaturation;
  float  hdrLuminance_MaxAvg;   // Display property
  float  hdrLuminance_MaxLocal; // Display property
  float  hdrLuminance_Min;      // Display property

//float  hdrContrast;
  float  hdrPaperWhite;
  //float  hdrExposure;
  float  currentTime;
  float  sdrLuminance_NonStd;
  uint   sdrIsImplicitlysRGB;
  uint   uiToneMapper;

  float4 pqBoostParams;
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
  0.00f, 0.00f, 0.0f,
};

static const SK_ColorSpace _Rec2020 =
{
  0.708f, 0.292f,  0.1700f,  0.797f, 0.131f, 0.046f,
     D65, 1.000f - 0.3127f - 0.329f,
  0.0f, 0.000f,  0.0000f,
};

static const SK_ColorSpace _DCI_P3 =
{
  0.68f, 0.32f,  0.2650f,  0.690f,   0.150f, 0.060f,
    D65, 1.00f - 0.3127f - 0.329f,
  0.00f, 0.00f,  0.0000f,
};

static const SK_ColorSpace _ColorSpaces [] = {
  _sRGB, _DCI_P3, _Rec2020
};


static inline
const  float
SK_Matrix_Det2x2 ( float a1, float a2,
                   float b1, float b2 )
{
  return
    ( a1 * b2 ) -
    ( b1 * a2 );
}

static inline
const  float3x3
SK_Matrix_Inverse3x3 ( float3x3 m )
{
  float3x3 adjoint;

  adjoint [0][0] =  SK_Matrix_Det2x2 (m [1][1], m [1][2], m [2][1], m [2][2]);
  adjoint [0][1] = -SK_Matrix_Det2x2 (m [0][1], m [0][2], m [2][1], m [2][2]);
  adjoint [0][2] =  SK_Matrix_Det2x2 (m [0][1], m [0][2], m [1][1], m [1][2]);

  adjoint [1][0] = -SK_Matrix_Det2x2 (m [1][0], m [1][2], m [2][0], m [2][2]);
  adjoint [1][1] =  SK_Matrix_Det2x2 (m [0][0], m [0][2], m [2][0], m [2][2]);
  adjoint [1][2] = -SK_Matrix_Det2x2 (m [0][0], m [0][2], m [1][0], m [1][2]);

  adjoint [2][0] =  SK_Matrix_Det2x2 (m [1][0], m [1][1], m [2][0], m [2][1]);
  adjoint [2][1] = -SK_Matrix_Det2x2 (m [0][0], m [0][1], m [2][0], m [2][1]);
  adjoint [2][2] =  SK_Matrix_Det2x2 (m [0][0], m [0][1], m [1][0], m [1][1]);

  // Calculate the determinant as a combination of the cofactors of the first row.
  float determinant =
    ( adjoint [0][0] * m [0][0] ) +
    ( adjoint [0][1] * m [1][0] ) +
    ( adjoint [0][2] * m [2][0] );

  // Divide the classical adjoint matrix by the determinant.
  // If determinant is zero, matrix is not invertable, so leave it unchanged.
  return
    ( determinant != 0.0f )
      ? ( adjoint * ( 1.0f / determinant ) )
      : m;
}

float3
SK_Color_XYZ_from_RGB ( const SK_ColorSpace cs, float3 RGB )
{
  const float Xr =       cs.xr / cs.yr;
  const float Zr = (1. - cs.xr - cs.yr) / cs.yr;

  const float Xg =       cs.xg / cs.yg;
  const float Zg = (1. - cs.xg - cs.yg) / cs.yg;

  const float Xb =       cs.xb / cs.yb;
  const float Zb = (1. - cs.xb - cs.yb) / cs.yb;

  const float Yr = 1.;
  const float Yg = 1.;
  const float Yb = 1.;

  float3x3 xyz_primary =
    float3x3 (Xr, Xg, Xb,
              Yr, Yg, Yb,
              Zr, Zg, Zb);

  float3 S =
    mul (SK_Matrix_Inverse3x3 (xyz_primary), float3 (cs.Xw, cs.Yw, cs.Zw));

  float3x3 scaleMat =
    float3x3 (S.r * Xr, S.g * Xg, S.b * Xb,
              S.r * Yr, S.g * Yg, S.b * Yb,
              S.r * Zr, S.g * Zg, S.b * Zb);

  return
    mul (RGB, scaleMat);
}

float3
SK_Color_xyY_from_RGB ( const SK_ColorSpace cs, float3 RGB )
{
  float3 XYZ =
    SK_Color_XYZ_from_RGB ( cs, RGB );

  return
    float3 ( XYZ.x / (XYZ.x + XYZ.y + XYZ.z),
             XYZ.y / (XYZ.x + XYZ.y + XYZ.z),
                              XYZ.y );
}

float
transformRGBtoY (float3 rgb)
{
  return
    dot (rgb, float3 (0.2126729, 0.7151522, 0.0721750));
}

float
transformRGBtoLogY (float3 rgb)
{
  return
    log (transformRGBtoY (rgb));
}

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

struct tileData
{
  uint2 tileIdx;
//uint2 pixelOffset;

  uint  minLuminance;
  uint  maxLuminance;
  uint  avgLuminance;
  uint  blackPixelCount;
  
  //// Gamut coverage counters
  //float minx, maxx, avgx;
  //float miny, maxy, avgy;

  uint  Histogram [NUM_HISTOGRAM_BINS];
};

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
        
          InterlockedMin (tile_data.minLuminance, (uint)(Y * 1000.0));
          InterlockedMax (tile_data.maxLuminance, (uint)(Y * 1000.0));
          InterlockedAdd (tile_data.Histogram [bin], 1u);
        
          if ((uint)(1000.0 * Y) > 0)
          {
            InterlockedAdd (pixels,                1u);
            InterlockedAdd (accum, (uint)(1000.0 * Y));
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
        (0.001f * (float)accum)
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