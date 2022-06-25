#define NUM_HISTOGRAM_BINS          4096
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

float3
SK_Triangle_Barycentric ( float3 pt,
                          float3 vTriangle [3] )
{
  float3 v0 = vTriangle [2] - vTriangle [0],
         v1 = vTriangle [1] - vTriangle [0],
         v2 =            pt - vTriangle [0];

  float d00 = dot (v0, v0);
  float d01 = dot (v0, v1);
  float d11 = dot (v1, v1);
  float d20 = dot (v2, v0);
  float d21 = dot (v2, v1);

  float denom = d00 * d11 -
                d01 * d01;

  float u = (d11 * d20 - d01 * d21) / denom;
  float v = (d00 * d21 - d01 * d20) / denom;

  float w = 1.0f - u - v;

  return
    float3 ( u, v, w );
}

float
transformRGBtoY (float3 rgb)
{
  return
    dot (rgb, float3 (0.2126729, 0.7151522, 0.0721750));
    //mul (rgb, colorspaceMat_RGBtoxyY).z;
}

float
transformRGBtoLogY (float3 rgb)
{
  return
    log (transformRGBtoY (rgb));
}

float luminance_min = 0.0;
float luminance_max = 4.0;

uint
computeBucketIdxFromRGB (float3 rgb)
{
  float logY =
    transformRGBtoLogY (rgb);

  return uint (
    float (NUM_HISTOGRAM_BINS) *
      saturate ( (logY          - luminance_min) /
                 (luminance_max - luminance_min)  )
  );
}

RWTexture2D<float4> texBackbufferHDR;
RWTexture2D<float>  texLuminance;
RWTexture2D<float4> texGamutCoverage;

#if 0
groupshared uint localHistogram [NUM_HISTOGRAM_BINS];
  Buffer         localHistogramIn;
RWBuffer <uint>  localHistogramOut;
#endif

groupshared uint  accum  = 0;
groupshared uint  pixels = 0;

[numthreads (HISTOGRAM_WORKGROUP_SIZE_X, HISTOGRAM_WORKGROUP_SIZE_Y, 1)]
void
LocalHistogramWorker ( uint3 globalIdx : SV_DispatchThreadID,
                       uint3 localIdx  : SV_GroupThreadID,
                       uint3 groupIdx  : SV_GroupID )
{
  const uint subdiv =
    min ( HISTOGRAM_WORKGROUP_SIZE_Y,
          HISTOGRAM_WORKGROUP_SIZE_X );

  uint image_width, image_height;

  texBackbufferHDR.GetDimensions ( image_width,
                                   image_height );

  if (! any (localIdx))
  {
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

      for ( uint X = 0 ; X < x_advance ; ++X )
      for ( uint Y = 0 ; Y < y_advance ; ++Y )
      {
        const uint2 pos =
          uint2 (x_origin + X, y_origin + Y);

        if ( pos.x < image_width &&
             pos.y < image_height )
        {
          float3 pixelValue =
            texBackbufferHDR [pos.xy].rgb;

#if 0
          uint bin =
            computeBucketIdxFromRGB (pixelValue);

          InterlockedAdd (localHistogram [bin], 1u);
#endif
          if ((uint)(1000.0 * dot (pixelValue, float3 (0.2126729, 0.7151522, 0.0721750))) > 0)
          {
            InterlockedAdd (pixels,               1u);
            InterlockedAdd (accum, (uint)(1000.0 * dot (pixelValue, float3 (0.2126729, 0.7151522, 0.0721750))));
          }

#if 0
          float3 xyY =
            SK_Color_xyY_from_RGB ( _sRGB, pixelValue / dot (pixelValue, float3 (0.2126729, 0.7151522, 0.0721750)) );

          //xyY.xy =
          //  saturate (xyY.xy);

          texGamutCoverage [uint2 ( 1024 * xyY.z,
                                    1024 -  1024 * xyY.y )].rgba =
            float4 (pixelValue.rgb, 1.0);

          texBackbufferHDR [uint2 (image_width * xyY.z, image_height - image_height * xyY.y)/*pos.xy*/].rgba =
            float4 (pixelValue.rgb, 1.0);
#endif
        }
      }
    }

    //float3 pixelValue =
    //  texBackbufferHDR [globalIdx.xy].rgb;
    //
    //uint bin =
    //  computeBucketIdxFromRGB (pixelValue);
    //
    //InterlockedAdd (localHistogram [bin], 1u);
  }

  GroupMemoryBarrier ();

  if (! any (localIdx))
  {
  ////uint outputHistogramIdx = NUM_HISTOGRAM_BINS * tileIdx;
  ////for ( uint idx = 0; idx < NUM_HISTOGRAM_BINS; ++idx )
  ////{
  ////  uint outIdx =
  ////    idx + outputHistogramIdx;
  ////
  ////  localHistogramOut [outIdx] =
  ////     localHistogram [   idx];
  ////}

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
  }
}