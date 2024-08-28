#pragma warning ( disable : 3577 )

#define NUM_HISTOGRAM_BINS          128
#define HISTOGRAM_WORKGROUP_SIZE_X ( 32 )
#define HISTOGRAM_WORKGROUP_SIZE_Y ( 16 )

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

cbuffer colorSpaceTransform : register (b0)
{
  uint2  visualFunc;

  float  hdrSaturation;
  float  hdrLuminance_MaxAvg;   // Display property
  float  hdrLuminance_MaxLocal; // Display property
  float  hdrLuminance_Min;      // Display property
  float  sdrLuminance_White;    // Windows SDR whitepoint pref.

//float  hdrContrast;
  float  hdrGamutExpansion;
  //float  hdrPaperWhite;
  //float  hdrExposure;
  float  currentTime;
  float  sdrLuminance_NonStd;
  float  sdrContentEOTF;
  uint   uiToneMapper;

  float4 pqBoostParams;
  float  colorBoost;
  uint   overbrightColorFlags;
  bool   alignmentPadding0;
  bool   alignmentPadding1;
};

#define FLT_EPSILON     1.192092896e-07 // Smallest positive number, such that 1.0 + FLT_EPSILON != 1.0
#define FLT_MIN         1.175494351e-38 // Minimum representable positive floating-point number
#define FLT_MAX         3.402823466e+38 // Maximum representable floating-point number

#define minmax(m0,m1,c) min (max ((c), (m0)), (m1))

#define float_MAX        65504.0 // (2 - 2^-10) * 2^15
#define float_MAX_MINUS1 65472.0 // (2 - 2^-9) * 2^15
#define EPSILON          1.0e-4
#define PI               3.14159265359
#define TWO_PI           6.28318530718
#define FOUR_PI          12.56637061436
#define INV_PI           0.31830988618
#define INV_TWO_PI       0.15915494309
#define INV_FOUR_PI      0.07957747155
#define float_PI         1.57079632679
#define INV_float_PI     0.636619772367

#define FLT_EPSILON     1.192092896e-07 // Smallest positive number, such that 1.0 + FLT_EPSILON != 1.0
#define FLT_MIN         1.175494351e-38 // Minimum representable positive floating-point number
#define FLT_MAX         3.402823466e+38 // Maximum representable floating-point number

#define USE_PRECISE_LOGC 1

bool IsFinite (float x)
{
  return
    (asuint (x) & 0x7F8FFFFF) != 0x7F800000;
}

bool    IsNegative (float  x)    { return      x    < 0.0f;  }
bool AnyIsNegative (float2 xy)   { return any (xy   < 0.0f); }
bool AnyIsNegative (float3 xyz)  { return any (xyz  < 0.0f); }
bool AnyIsNegative (float4 xyzw) { return any (xyzw < 0.0f); }

bool   IsNan (float  x) { return (asuint (x) & 0x7fffffff)  > 0x7f800000; } // Scalar NaN checker
float2 IsNan (float2 v) { return float2 ( IsNan (v.x), IsNan (v.y) );                           }
float3 IsNan (float3 v) { return float3 ( IsNan (v.x), IsNan (v.y), IsNan (v.z) );              }
float4 IsNan (float4 v) { return float4 ( IsNan (v.x), IsNan (v.y), IsNan (v.z), IsNan (v.w) ); }

bool   IsInf (float  x) { return (asuint (x) & 0x7f8fffff) == 0x7f800000; } // Scalar Infinity checker
float2 IsInf (float2 v) { return float2 ( IsInf (v.x), IsInf (v.y) );                           }
float3 IsInf (float3 v) { return float3 ( IsInf (v.x), IsInf (v.y), IsInf (v.z) );              }
float4 IsInf (float4 v) { return float4 ( IsInf (v.x), IsInf (v.y), IsInf (v.z), IsInf (v.w) ); }

// Vectorized versions
bool AnyIsInf (float  x)    { return        IsInf (x);                                 }
bool AnyIsInf (float2 xy)   { return any ((asuint (xy)   & 0x7f8fffff) == 0x7f800000); }
bool AnyIsInf (float3 xyz)  { return any ((asuint (xyz)  & 0x7f8fffff) == 0x7f800000); }
bool AnyIsInf (float4 xyzw) { return any ((asuint (xyzw) & 0x7f8fffff) == 0x7f800000); }

bool AnyIsNan (float  x)    { return        IsNan (x);                                 }
bool AnyIsNan (float2 xy)   { return any ((asuint (xy)   & 0x7fffffff)  > 0x7f800000); }
bool AnyIsNan (float3 xyz)  { return any ((asuint (xyz)  & 0x7fffffff)  > 0x7f800000); }
bool AnyIsNan (float4 xyzw) { return any ((asuint (xyzw) & 0x7fffffff)  > 0x7f800000); }

// Combined NaN and Infinity check
bool isnormal (float  x)    { return (! (     (asuint (x)    & 0x7fffffff) >= 0x7f800000));  }
bool isnormal (float2 xy)   { return (! (any ((asuint (xy)   & 0x7fffffff) >= 0x7f800000))); }
bool isnormal (float3 xyz)  { return (! (any ((asuint (xyz)  & 0x7fffffff) >= 0x7f800000))); }
bool isnormal (float4 xyzw) { return (! (any ((asuint (xyzw) & 0x7fffffff) >= 0x7f800000))); }

// Remove special floating-point bit patterns, clamping is the
//   final step before output and outputting NaN or Infinity would
//     break color blending!
#define SanitizeFP(c) ((! isnormal ((c))) ? (IsInf ((c)) ? sign ((c)) * float_MAX : (! IsNan ((c))) * (c)) : (c))

#define FP16_MIN     asfloat (0x33C00000)
#define FP16_EPSILON 0.000488          // Smallest positive number, such that 1.0 + FP16_EPSILON != 1.0

float3 Rec709toAP0_D65 (float3 linearRec709);
float3 AP0_D65toRec709 (float3 linearAP0);

float3 Clamp_scRGB (float3 c)
{
  c = SanitizeFP (c);

  c =
    clamp (c, -float_MAX,
               float_MAX);

  return c;
}

float Clamp_scRGB (float c, bool strip_nan = false)
{
  // No colorspace clamp here, just keep it away from 0.0
  if (strip_nan)
    c = SanitizeFP (c);

  return clamp (c, -float_MAX,
                    float_MAX);
}

// Clamp HDR value within a safe range
float3 SafeHDR (float3 c)
{
  return
    min (c, float_MAX);
}

float4 SafeHDR (float4 c)
{
  return
    min (c, float_MAX);
}

// https://twitter.com/SebAaltonen/status/878250919879639040
// madd_sat + madd
float FastSign (float x)
{
  return
    saturate (x * FLT_MAX + 0.5) * 2.0 - 1.0;
}

float2 FastSign (float2 x)
{
  return
    saturate (x * FLT_MAX + 0.5) * 2.0 - 1.0;
}

float3 FastSign (float3 x)
{
  return
    saturate (x * FLT_MAX + 0.5) * 2.0 - 1.0;
}

float4 FastSign (float4 x)
{
  return
    saturate (x * FLT_MAX + 0.5) * 2.0 - 1.0;
}

// Using pow often result to a warning like this
// "pow(f, e) will not work for negative f, use abs(f) or conditionally handle negative values if you expect them"
// PositivePow remove this warning when you know the value is positive and avoid inf/NAN.
float PositivePow (float base, float power)
{
  return
    sign (base) * pow ( max (abs (base), float (FLT_MIN)), power );
}

float3 PositivePow (float3 base, float3 power)
{
  return
    sign (base) * pow (max (abs (base), float3 ( FLT_MIN, FLT_MIN,
                                                 FLT_MIN )), power );
}

float4 PositivePow (float4 base, float4 power)
{
  return
    sign (base) * pow (max (abs (base), float4 ( FLT_MIN, FLT_MIN,
                                                 FLT_MIN, FLT_MIN )), power );
}

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
    float3 ( XYZ.x / (           XYZ.x + XYZ.y + XYZ.z ),
             XYZ.y / (           XYZ.x + XYZ.y + XYZ.z ),
               1.0 - ( XYZ.x / ( XYZ.x + XYZ.y + XYZ.z ) )
                   - ( XYZ.y / ( XYZ.x + XYZ.y + XYZ.z ) ) );
}

float3 Rec709_to_XYZ (float3 linearRec709)
{
  static const float3x3 ConvMat =
  {
    0.4123907983303070068359375000f, 0.357584327459335327148437500f, 0.180480793118476867675781250f,
    0.2126390039920806884765625000f, 0.715168654918670654296875000f, 0.072192318737506866455078125f,
    0.0193308182060718536376953125f, 0.119194783270359039306640625f, 0.950532138347625732421875000f
  };

  return
    mul (ConvMat, linearRec709);
}

float3 XYZ_to_Rec709 (float3 XYZ)
{
  static const float3x3 ConvMat =
  {
     3.240969896316528320312500000f, -1.5373831987380981445312500f, -0.4986107647418975830078125000f,
    -0.969243645668029785156250000f,  1.8759675025939941406250000f,  0.0415550582110881805419921875f,
     0.055630080401897430419921875f, -0.2039769589900970458984375f,  1.0569715499877929687500000000f
  };

  return
    mul (ConvMat, XYZ);
}

float3 XYZ_to_LMS (float3 XYZ)
{
  static const float3x3 ConvMat =
  {
     0.3592, 0.6976, -0.0358,
    -0.1922, 1.1004,  0.0755,
     0.0070, 0.0749,  0.8434
  };

  return
    mul (ConvMat, XYZ);
}

float3 LMS_to_XYZ (float3 LMS)
{
  static const float3x3 ConvMat =
  {
     2.070180056695613509600, -1.326456876103021025500,  0.206616006847855170810,
     0.364988250032657479740,  0.680467362852235141020, -0.045421753075853231409,
    -0.049595542238932107896, -0.049421161186757487412,  1.187995941732803439400
  };

  return
    mul (ConvMat, LMS);
}

float
transformRGBtoY (float3 rgb)
{
  return
    //RGB_to_ICtCp (max (REC709toREC2020 (rgb), FP16_MIN)).x * 12.5f;
    dot (rgb, sign (rgb) * float3 (0.2126729, 0.7151522, 0.0721750));
}

float
transformRGBtoLogY (float3 rgb)
{
  return
    log (transformRGBtoY (rgb));
}


float3
RemoveSRGBCurve (float3 x)
{
  return             ( abs (x) < 0.04045f ) ?
    sign (x) *       ( abs (x) / 12.92f   ) :
    sign (x) * pow ( ( abs (x) + 0.055f   ) / 1.055f, 2.4f );
}

float
RemoveSRGBAlpha (float a)
{
  return             ( abs (a) < 0.04045f ) ?
    sign (a) *       ( abs (a) / 12.92f   ) :
    sign (a) * pow ( ( abs (a) + 0.055f   ) / 1.055f, 2.4f );
}

float3
RemoveGammaExp (float3 x, float exp)
{
  return     sign (x) *
         pow (abs (x), exp);
}

// Alpha blending works best in linear-space, so -removing- gamma,
//   assuming the source even had any gamma is useful.
float
RemoveAlphaGammaExp (float a, float exp)
{
  return  sign (a) *
      pow (abs (a), exp);
}

float3
ApplySRGBCurve (float3 x)
{
  return ( abs (x) < 0.0031308f ?
          sign (x) * ( 12.92f *       abs (x) ) :
          sign (x) *   1.055f * pow ( abs (x), 1.0 / 2.4f ) - 0.055f );
}

float
ApplySRGBAlpha (float a)
{
  return ( abs (a) < 0.0031308f ?
          sign (a) * ( 12.92f *       abs (a) ) :
          sign (a) *   1.055f * pow ( abs (a), 1.0 / 2.4f ) - 0.055f );
}

float3
ApplyGammaExp (float3 x, float exp)
{
  return     sign (x) *
         pow (abs (x), 1.0f / exp);
}

// Alpha blending works best in linear-space, so -removing- gamma,
//   assuming the source even had any gamma is useful.
float
ApplyAlphaGammaExp (float a, float exp)
{
  return  sign (a) *
      pow (abs (a), 1.0f / exp);
}

//
// Convert rgb to luminance with rgb in linear space with sRGB primaries and D65 white point
//
float Luminance (float3 linearRgb)
{
  return
    max (0.0f, dot (linearRgb, float3 (0.2126729, 0.7151522, 0.0721750)));
}

float Luminance (float4 linearRgba)
{
  return
    Luminance (linearRgba.rgb);
}

float LuminanceRec2020 (float3 linearRgb)
{
  return
    max (0.0f, dot (linearRgb, float3 (0.2627, 0.678, 0.0593)));
}

//
// RGB Saturation (closer to a vibrance effect than actual saturation)
// Recommended workspace: ACEScg (linear)
// Optimal range: [0.0, 2.0]
//
float3 Saturation (float3 c, float sat)
{
  float luma =
    Luminance (c);
  
  return
    luma.xxx + sat.xxx * (c - luma.xxx);
}

//
// Contrast (reacts better when applied in log)
// Optimal range: [0.0, 2.0]
//
float3 Contrast (float3 c, float midpoint, float contrast)
{
  float fLuma =
    Luminance (c);

  if (fLuma <= 0.0666)
  {
    return
      c * smoothstep (0.0, 0.0666, fLuma);
  }

  return
    (c - midpoint) * contrast
       + midpoint;
}


//
// Lift, Gamma (pre-inverted), Gain tuned for HDR use - best used with the ACES tonemapper as
// negative values will creep in the result
// Expected workspace: ACEScg (linear)
//
float3 LiftGammaGainHDR (float3 c, float3 lift, float3 invgamma, float3 gain)
{
  c =
    (c * gain + lift);
  
  // ACEScg will output negative values, as clamping to 0 will lose precious information we'll
  // mirror the gamma function instead
  return
    FastSign (c) * pow (abs (c), invgamma);
}

//
// Lift, Gamma (pre-inverted), Gain tuned for LDR use
// Input is linear RGB
//
float3 LiftGammaGainLDR (float3 c, float3 lift, float3 invgamma, float3 gain)
{
  c =
    saturate (PositivePow (saturate (c), invgamma));

  return
    (gain * c + lift * (1.0 - c));
}

float3 HUEtoRGB (in float H)
{
  float R =     abs (H * 6 - 3) - 1;
  float G = 2 - abs (H * 6 - 2);
  float B = 2 - abs (H * 6 - 4);

  return
    saturate (float3 (R,G,B));
}

float RGBtoHUE (in float3 rgb)
{
	float H;

	if ( rgb.x == rgb.y &&
       rgb.y == rgb.z )
		  H =    0.0;
	else
		  H = ( 180.0 / PI ) * atan2 (
          sqrt (3.0) *
        ( rgb.y - rgb.z ),
    2.0 * rgb.x - rgb.y - rgb.z   );
	if (H <    0.0)
      H =
      H +  360.0;

	return H;
}

float3 HSVtoRGB (in float3 HSV)
{
  float3 RGB =
    HUEtoRGB (HSV.x);

  return ((RGB - 1) * HSV.y + 1) * HSV.z;
}

float3
REC709toREC2020 (float3 linearRec709)
{
  static const float3x3 ConvMat =
  {
    0.627403914928436279296875f,      0.3292830288410186767578125f,  0.0433130674064159393310546875f,
    0.069097287952899932861328125f,   0.9195404052734375f,           0.011362315155565738677978515625f,
    0.01639143936336040496826171875f, 0.08801330626010894775390625f, 0.895595252513885498046875f
  };

  return mul (ConvMat, linearRec709);
}

float3 REC2020toREC709 (float3 linearRec2020)
{
  static const float3x3 ConvMat =
  {
     1.66049098968505859375f,          -0.58764111995697021484375f,     -0.072849862277507781982421875f,
    -0.12455047667026519775390625f,     1.13289988040924072265625f,     -0.0083494223654270172119140625f,
    -0.01815076358616352081298828125f, -0.100578896701335906982421875f,  1.11872971057891845703125f
  };
  
  return mul (ConvMat, linearRec2020);
}

#ifndef INTRINSIC_MINMAX3
float Min3 (float a, float b, float c)
{
  return min (min (a, b), c);
}

float2 Min3 (float2 a, float2 b, float2 c)
{
  return min (min (a, b), c);
}

float3 Min3 (float3 a, float3 b, float3 c)
{
  return min (min (a, b), c);
}

float4 Min3 (float4 a, float4 b, float4 c)
{
  return min (min (a, b), c);
}

float Max3 (float a, float b, float c)
{
  return max (max (a, b), c);
}

float2 Max3 (float2 a, float2 b, float2 c)
{
  return max (max (a, b), c);
}

float3 Max3 (float3 a, float3 b, float3 c)
{
  return max (max (a, b), c);
}

float4 Max3 (float4 a, float4 b, float4 c)
{
  return max (max (a, b), c);
}
#endif // INTRINSIC_MINMAX3

///// Interleaved gradient function from Jimenez 2014
///// http://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare
///float GradientNoise(float2 uv)
///{
///    uv = floor(uv * _ScreenParams.xy);
///    float f = dot(float2(0.06711056, 0.00583715), uv);
///    return frac(52.9829189 * frac(f));
///}

// Vertex manipulation
float2 TransformTriangleVertexToUV (float2 vertex)
{
  float2 uv =
    (vertex + 1.0) * 0.5;

  return uv;
}

#ifndef DEFAULT_MAX_PQ
  // PQ ST.2048 max value
  // 1.0 = 100nits, 100.0 = 10knits
  #define DEFAULT_MAX_PQ 100.0
#endif

//
// Alexa LogC converters (El 1000)
// See http://www.vocas.nl/webfm_send/964
// Max range is ~58.85666
//
struct ParamsLogC
{
  float cut;
  float a, b, c, d, e, f;
};

static const ParamsLogC LogC =
{
  0.011361, // cut
  5.555556, // a
  0.047996, // b
  0.244161, // c
  0.386036, // d
  5.301883, // e
  0.092819  // f
};

float LinearToLogC_Precise (float x)
{
  float o;
  if (x > LogC.cut)
      o = LogC.c * log10(LogC.a * x + LogC.b) + LogC.d;
  else
      o = LogC.e * x + LogC.f;

  return
    Clamp_scRGB (o);
}

float3 LinearToLogC (float3 x)
{
#if USE_PRECISE_LOGC
  return float3(
      LinearToLogC_Precise(x.x),
      LinearToLogC_Precise(x.y),
      LinearToLogC_Precise(x.z)
  );
#else
  return
    Clamp_scRGB ((LogC.c * log10(LogC.a * x + LogC.b) + LogC.d));
#endif
}

float LogCToLinear_Precise (float x)
{
  float o;

  if (x > LogC.e * LogC.cut + LogC.f)
      o = (pow(10.0, (x - LogC.d) / LogC.c) - LogC.b) / LogC.a;
  else
      o = (x - LogC.f) / LogC.e;

  return
    Clamp_scRGB (o);
}

float3 LogCToLinear (float3 x)
{
#if USE_PRECISE_LOGC
  return float3(
      LogCToLinear_Precise(x.x),
      LogCToLinear_Precise(x.y),
      LogCToLinear_Precise(x.z)
  );
#else
  return Clamp_scRGB (
     (pow (10.0, (x - LogC.d) / LogC.c) -
                      LogC.b) / LogC.a);
#endif
}

//
// SMPTE ST.2084 (PQ) transfer functions
// Used for HDR Lut storage, max range depends on the maxPQValue parameter
//
struct ParamsPQ
{
  float N, rcpN;
  float M, rcpM;
  float C1, C2, C3;
};

static const ParamsPQ PQ =
{
       2610.0 / 4096.0 / 4.0,    // N
  rcp (2610.0 / 4096.0 / 4.0),   // rcp (N)
       2523.0 / 4096.0 * 128.0,  // M
  rcp (2523.0 / 4096.0 * 128.0), // rcp (M)
       3424.0 / 4096.0,          // C1
       2413.0 / 4096.0 * 32.0,   // C2
       2392.0 / 4096.0 * 32.0,   // C3
};

float4 LinearToPQ (float4 x, float maxPQValue)
{
  x =
    pow ( max (x, 0) / maxPQValue,
                          PQ.N );
 
  float4 nd =
    (PQ.C1 + PQ.C2 * x) /
      (1.0 + PQ.C3 * x);

  return
    pow (nd, PQ.M);
}

float3 LinearToPQ (float3 x, float maxPQValue)
{
  x =
    pow ( max (x, 0) / maxPQValue,
                          PQ.N );
 
  float3 nd =
    (PQ.C1 + PQ.C2 * x) /
      (1.0 + PQ.C3 * x);

  return
    pow (nd, PQ.M);
}

float LinearToPQ (float x, float maxPQValue)
{
  x =
    pow ( max (x, 0) / maxPQValue,
                          PQ.N );
 
  float nd =
    (PQ.C1 + PQ.C2 * x) /
      (1.0 + PQ.C3 * x);

  return
    pow (nd, PQ.M);
}

float3 LinearToPQ (float3 x)
{
  return
    LinearToPQ (x, DEFAULT_MAX_PQ);
}

float PQToLinear (float x, float maxPQValue)
{
  x =
    pow (max (x, 0), PQ.rcpM);

  float nd =
    max (x - PQ.C1, 0.0) /
            (PQ.C2 - (PQ.C3 * x));

  return
    pow (nd, PQ.rcpN) * maxPQValue;
}

float3 PQToLinear (float3 x, float maxPQValue)
{
  x =
    pow (max (x, 0), PQ.rcpM);

  float3 nd =
    max (x - PQ.C1, 0.0) /
            (PQ.C2 - (PQ.C3 * x));

  return
    pow (nd, PQ.rcpN) * maxPQValue;
}

float4 PQToLinear (float4 x, float maxPQValue)
{
  x =
    pow (max (x, 0), PQ.rcpM);

  float4 nd =
    max (x - PQ.C1, 0.0) /
            (PQ.C2 - (PQ.C3 * x));

  return
    pow (nd, PQ.rcpN) * maxPQValue;
}

float3 PQToLinear (float3 x)
{
  return
    PQToLinear (x, DEFAULT_MAX_PQ);
}


float LinearToPQY (float x, float maxPQValue)
{
  x =
    pow ( max (x, 0) / maxPQValue,
                          PQ.N );
 
  float nd =
    (PQ.C1 + PQ.C2 * x) /
      (1.0 + PQ.C3 * x);

  return
    pow (nd, PQ.M);
}

float LinearToPQY (float x)
{
  return
    LinearToPQY (x, DEFAULT_MAX_PQ);
}

float PQToLinearY (float x, float maxPQValue)
{
  x =
    pow (max (x, 0), PQ.rcpM);

  float nd =
    max (x - PQ.C1, 0.0) /
            (PQ.C2 - (PQ.C3 * x));

  return
    pow (nd, PQ.rcpN) * maxPQValue;
}

float PQToLinearY (float x)
{
  return
    PQToLinearY (x, DEFAULT_MAX_PQ);
}


float3 RemoveREC2084Curve (float3 N)
{
  if (AnyIsNan (N))
    return 0.0f;

  float  m1 = 2610.0 / 4096.0 / 4;
  float  m2 = 2523.0 / 4096.0 * 128;
  float  c1 = 3424.0 / 4096.0;
  float  c2 = 2413.0 / 4096.0 * 32;
  float  c3 = 2392.0 / 4096.0 * 32;
  float3 Np = pow (max (N, 0.0), 1 / m2);

  return
    PositivePow ( max (Np - c1,   0) /
                      (c2 - c3 * Np),
                        1 / m1);
}


//
// sRGB transfer functions
// Fast path ref: http://chilliant.blogspot.com.au/2012/08/srgb-approximations-for-hlsl.html?m=1
//
float SRGBToLinear (float c)
{
#if USE_VERY_FAST_SRGB
  return c * c;
#elif USE_FAST_SRGB
  return c * (c * (c * 0.305306011 + 0.682171111) + 0.012522878);
#else
  float linearRGBLo =               c  / 12.92;
  float linearRGBHi = PositivePow ((c  + 0.055) / 1.055, 2.4);
  float linearRGB   =              (c <= 0.04045) ?
                                      linearRGBLo : linearRGBHi;
  return                 Clamp_scRGB (linearRGB);
#endif
}

float3 SRGBToLinear (float3 c)
{
#if USE_VERY_FAST_SRGB
  return c * c;
#elif USE_FAST_SRGB
  return c * (c * (c * 0.305306011 + 0.682171111) + 0.012522878);
#else
  float3 linearRGBLo =               c  / 12.92;
  float3 linearRGBHi = PositivePow ((c  + 0.055) / 1.055, float3 (2.4, 2.4, 2.4));
  float3 linearRGB   =              (c <= 0.04045) ?
                                       linearRGBLo : linearRGBHi;
  return                  Clamp_scRGB (linearRGB);
#endif
}

float4 SRGBToLinear (float4 c)
{
  return
    float4 (
      SRGBToLinear (c.rgb),
                    c.a
    );
}

float LinearToSRGB (float c)
{
#if USE_VERY_FAST_SRGB
  return sqrt(c);
#elif USE_FAST_SRGB
  return max(1.055 * PositivePow(c, 0.416666667) - 0.055, 0.0);
#else
  float sRGBLo =               c * 12.92;
  float sRGBHi = (PositivePow (c, 1.0 / 2.4) * 1.055) - 0.055;
  float sRGB   =              (c <= 0.0031308) ?
                                        sRGBLo : sRGBHi;
  return                   Clamp_scRGB (sRGB);
#endif
}

float3 LinearToSRGB (float3 c)
{
#if USE_VERY_FAST_SRGB
    return sqrt(c);
#elif USE_FAST_SRGB
    return max(1.055 * PositivePow(c, 0.416666667) - 0.055, 0.0);
#else
  float3 sRGBLo =  c * 12.92;
  float3 sRGBHi = (PositivePow (c, float3(1.0 / 2.4, 1.0 / 2.4, 1.0 / 2.4)) * 1.055) - 0.055;
  float3 sRGB   = (c <= 0.0031308) ?
                            sRGBLo : sRGBHi;
  return       Clamp_scRGB (sRGB);
#endif
}

float4 LinearToSRGB (float4 c)
{
  return
    float4 (
      LinearToSRGB (c.rgb),
                    c.a );
}

//
// Quadratic color thresholding
// curve = (threshold - knee, knee * 2, 0.25 / knee)
//
float4 QuadraticThreshold (float4 color, float threshold, float3 curve)
{
  // Pixel brightness
  float br =
    Max3 (color.r, color.g, color.b);
  
  // Under-threshold part: quadratic curve
  float rq =
    clamp (br - curve.x, 0.0, curve.y);
  
  rq =
    curve.z * rq * rq;
  
  // Combine and apply the brightness response curve.
  color *= max (rq, br - threshold) /
           max (    br,    EPSILON);
  
  return color;
}

//
// Fast reversible tonemapper
// http://gpuopen.com/optimized-reversible-tonemapper-for-resolve/
//
float3 FastTonemap (float3 c)
{
  return
    c * rcp ( Max3 (c.r, c.g, c.b) + 1.0 );
}

float4 FastTonemap (float4 c)
{
  return
    float4 ( FastTonemap (c.rgb),
                          c.a );
}

float3 FastTonemap (float3 c, float w)
{
  return
    c * (w * rcp (
          Max3 (c.r, c.g, c.b) + 1.0
        )        );
}

float4 FastTonemap (float4 c, float w)
{
  return
    float4 ( FastTonemap (c.rgb, w),
                          c.a );
}

float3 FastTonemapInvert (float3 c)
{
  return
    c * rcp (1.0 - Max3 (c.r, c.g, c.b ) );
}

float4 FastTonemapInvert (float4 c)
{
  return
    float4 ( FastTonemapInvert (c.rgb),
                                c.a );
}

//
// Neutral tonemapping (Hable/Hejl/Frostbite)
// Input is linear RGB
//
float3 NeutralCurve ( float3 x, float a, float b,
                      float  c, float d, float e,
                      float  f )
{
  return
    ((x * (a * x + c * b) + d  *  e) /
     (x * (a * x + b) + d * f)) - e  / f;
}

float3 NeutralTonemap (float3 x)
{
  // Tonemap
  float          a = 0.2;
  float          b = 0.29;
  float          c = 0.24;
  float          d = 0.272;
  float          e = 0.02;
  float          f = 0.3;
  float whiteLevel = 5.3;
  float  whiteClip = 1.0;
  
  float3 whiteScale = (1.0).xxx / NeutralCurve (whiteLevel,     a, b, c, d, e, f);
  x =                             NeutralCurve (x * whiteScale, a, b, c, d, e, f);
  x *= whiteScale;
  
  // Post-curve white point adjustment
  x /= whiteClip.xxx;
  
  return x;
}

//
// Raw, unoptimized version of John Hable's artist-friendly tone curve
// Input is linear RGB
//
float EvalCustomSegment (float x, float4 segmentA, float2 segmentB)
{
  const float kOffsetX = segmentA.x;
  const float kOffsetY = segmentA.y;
  const float kScaleX  = segmentA.z;
  const float kScaleY  = segmentA.w;
  const float kLnA     = segmentB.x;
  const float kB       = segmentB.y;
  
  float x0 = (x - kOffsetX) * kScaleX;
  float y0 =             (x0 > 0.0) ?
    exp (kLnA + kB * log (x0))      : 0.0;

  return
    (y0 * kScaleY + kOffsetY);
}

float EvalCustomCurve ( float x, float3 curve, float4 toeSegmentA, float2 toeSegmentB,
                        float4    midSegmentA, float2 midSegmentB, float4 shoSegmentA,
                        float2    shoSegmentB )
{
  float4 segmentA;
  float2 segmentB;
  
  if (x < curve.y)
  {
    segmentA = toeSegmentA;
    segmentB = toeSegmentB;
  }
  else if (x < curve.z)
  {
    segmentA = midSegmentA;
    segmentB = midSegmentB;
  }
  else
  {
    segmentA = shoSegmentA;
    segmentB = shoSegmentB;
  }
  
  return
    EvalCustomSegment (x, segmentA, segmentB);
}

// curve: x: inverseWhitePoint, y: x0, z: x1
float3 CustomTonemap ( float3 x, float3 curve, float4 toeSegmentA, float2 toeSegmentB,
                                               float4 midSegmentA, float2 midSegmentB,
                                               float4 shoSegmentA, float2 shoSegmentB )
{
  float3 normX = x * curve.x;
  float3 ret;

  ret.x = EvalCustomCurve(normX.x, curve, toeSegmentA, toeSegmentB, midSegmentA, midSegmentB, shoSegmentA, shoSegmentB);
  ret.y = EvalCustomCurve(normX.y, curve, toeSegmentA, toeSegmentB, midSegmentA, midSegmentB, shoSegmentA, shoSegmentB);
  ret.z = EvalCustomCurve(normX.z, curve, toeSegmentA, toeSegmentB, midSegmentA, midSegmentB, shoSegmentA, shoSegmentB);

  return ret;
}


#ifndef __ACES__
#define __ACES__

/**
 * https://github.com/ampas/aces-dev
 *
 * Academy Color Encoding System (ACES) software and tools are provided by the
 * Academy under the following terms and conditions: A worldwide, royalty-free,
 * non-exclusive right to copy, modify, create derivatives, and use, in source and
 * binary forms, is hereby granted, subject to acceptance of this license.
 *
 * Copyright 2015 Academy of Motion Picture Arts and Sciences (A.M.P.A.S.).
 * Portions contributed by others as indicated. All rights reserved.
 *
 * Performance of any of the aforementioned acts indicates acceptance to be bound
 * by the following terms and conditions:
 *
 * * Copies of source code, in whole or in part, must retain the above copyright
 * notice, this list of conditions and the Disclaimer of Warranty.
 *
 * * Use in binary form must retain the above copyright notice, this list of
 * conditions and the Disclaimer of Warranty in the documentation and/or other
 * materials provided with the distribution.
 *
 * * Nothing in this license shall be deemed to grant any rights to trademarks,
 * copyrights, patents, trade secrets or any other intellectual property of
 * A.M.P.A.S. or any contributors, except as expressly stated herein.
 *
 * * Neither the name "A.M.P.A.S." nor the name of any other contributors to this
 * software may be used to endorse or promote products derivative of or based on
 * this software without express prior written permission of A.M.P.A.S. or the
 * contributors, as appropriate.
 *
 * This license shall be construed pursuant to the laws of the State of
 * California, and any disputes related thereto shall be subject to the
 * jurisdiction of the courts therein.
 *
 * Disclaimer of Warranty: THIS SOFTWARE IS PROVIDED BY A.M.P.A.S. AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND
 * NON-INFRINGEMENT ARE DISCLAIMED. IN NO EVENT SHALL A.M.P.A.S., OR ANY
 * CONTRIBUTORS OR DISTRIBUTORS, BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, RESITUTIONARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * WITHOUT LIMITING THE GENERALITY OF THE FOREGOING, THE ACADEMY SPECIFICALLY
 * DISCLAIMS ANY REPRESENTATIONS OR WARRANTIES WHATSOEVER RELATED TO PATENT OR
 * OTHER INTELLECTUAL PROPERTY RIGHTS IN THE ACADEMY COLOR ENCODING SYSTEM, OR
 * APPLICATIONS THEREOF, HELD BY PARTIES OTHER THAN A.M.P.A.S.,WHETHER DISCLOSED OR
 * UNDISCLOSED.
 */

#define ACEScc_MAX      1.4679964
#define ACEScc_MIDGRAY  0.4135884

//
// Precomputed matrices (pre-transposed)
// See https://github.com/ampas/aces-dev/blob/master/transforms/ctl/README-MATRIX.md
//
static const float3x3 sRGB_2_AP0 = {
  0.4397010, 0.3829780, 0.1773350,
  0.0897923, 0.8134230, 0.0967616,
  0.0175440, 0.1115440, 0.8707040
};

static const float3x3 sRGB_2_AP1 = {
  0.61319, 0.33951, 0.04737,
  0.07021, 0.91634, 0.01345,
  0.02062, 0.10957, 0.86961
};

static const float3x3 AP0_2_sRGB = {
   2.52169, -1.13413, -0.38756,
  -0.27648,  1.37272, -0.09624,
  -0.01538, -0.15298,  1.16835,
};

static const float3x3 AP1_2_sRGB = {
   1.70505, -0.62179, -0.08326,
  -0.13026,  1.14080, -0.01055,
  -0.02400, -0.12897,  1.15297,
};

static const float3x3 AP0_2_AP1_MAT = {
   1.4514393161, -0.2365107469, -0.2149285693,
  -0.0765537734,  1.1762296998, -0.0996759264,
   0.0083161484, -0.0060324498,  0.9977163014
};

static const float3x3 AP1_2_AP0_MAT = {
   0.6954522414, 0.1406786965, 0.1638690622,
   0.0447945634, 0.8596711185, 0.0955343182,
  -0.0055258826, 0.0040252103, 1.0015006723
};

static const float3x3 AP1_2_XYZ_MAT = {
   0.6624541811, 0.1340042065, 0.1561876870,
   0.2722287168, 0.6740817658, 0.0536895174,
  -0.0055746495, 0.0040607335, 1.0103391003
};

static const float3x3 XYZ_2_AP1_MAT = {
   1.6410233797, -0.3248032942, -0.2364246952,
  -0.6636628587,  1.6153315917,  0.0167563477,
   0.0117218943, -0.0082844420,  0.9883948585
};

static const float3x3 XYZ_2_REC709_MAT = {
   3.2409699419, -1.5373831776, -0.4986107603,
  -0.9692436363,  1.8759675015,  0.0415550574,
   0.0556300797, -0.2039769589,  1.0569715142
};

static const float3x3 XYZ_2_REC2020_MAT = {
   1.7166511880, -0.3556707838, -0.2533662814,
  -0.6666843518,  1.6164812366,  0.0157685458,
   0.0176398574, -0.0427706133,  0.9421031212
};

static const float3x3 XYZ_2_DCIP3_MAT = {
   2.7253940305, -1.0180030062, -0.4401631952,
  -0.7951680258,  1.6897320548,  0.0226471906,
   0.0412418914, -0.0876390192,  1.1009293786
};

static const float3 AP1_RGB2Y =
  float3 (0.272229, 0.674082, 0.0536895);

static const float3x3 RRT_SAT_MAT = {
  0.9708890, 0.0269633, 0.00214758,
  0.0108892, 0.9869630, 0.00214758,
  0.0108892, 0.0269633, 0.96214800
};

static const float3x3 ODT_SAT_MAT = {
  0.949056, 0.0471857, 0.00375827,
  0.019056, 0.9771860, 0.00375827,
  0.019056, 0.0471857, 0.93375800
};

static const float3x3 D60_2_D65_CAT = {
   0.98722400, -0.00611327, 0.0159533,
  -0.00759836,  1.00186000, 0.0053302,
   0.00307257, -0.00509595, 1.0816800
};

float3 sRGB_to_DCIP3 (float3 x)
{
  x =
    mul ( XYZ_2_DCIP3_MAT,
      mul ( AP1_2_XYZ_MAT,
        mul ( sRGB_2_AP1, x)
          )
        );

  return x;
}

// sRGB to ACES
//
// Converts sRGB primaries to
//          ACES2065-1 (AP0 w/ linear encoding)
float3 sRGB_to_ACES (float3 x)
{
  x =
    mul (sRGB_2_AP0, x);

  return x;
}

// ACES to sRGB
//
// Converts ACES2065-1 (AP0 w/ linear encoding)
//       to sRGB primaries
float3 ACES_to_sRGB (float3 x)
{
  x =
    mul (AP0_2_sRGB, x);

  return x;
}

// sRGB to ACEScg
//
// Converts sRGB primaries to
//          ACEScg (AP1 w/ linear encoding)
float3 sRGB_to_ACEScg (float3 x)
{
  x =
    mul (sRGB_2_AP1, x);

  return x;
}

// ACEScg to sRGB
//
// Converts ACEScg (AP1 w/ linear encoding)
//       to sRGB primaries
float3 ACEScg_to_sRGB (float3 x)
{
  x =
    mul (AP1_2_sRGB, x);

  return x;
}

//
// ACES Color Space Conversion - ACES to ACEScc
//
// Converts ACES2065-1 (AP0 w/ linear encoding) to
//          ACEScc     (AP1 w/ logarithmic encoding)
//
//  * This transform follows the formulas from section 4.4 in S-2014-003
//
float ACES_to_ACEScc (float x)
{
  if (x <= 0.0)
    return -0.35828683; // = (log2(pow(2.0, -15.0) * 0.5) + 9.72) / 17.52
  else if (x < pow (2.0, -15.0))
    return (log2 (pow (2.0, -16.0) + x * 0.5)
                     + 9.72) / 17.52;
  else // (x >= pow(2.0, -15.0))
    return (log2 (x) + 9.72) / 17.52;
}

float3 ACES_to_ACEScc (float3 x)
{
  if (AnyIsNan (x))
    x = 0.0f;

  x =
    clamp (x, 0.0, float_MAX);
  
  // x is clamped to [0, float_MAX], skip the <= 0 check
  return
      (x < 0.00003051757)                           ?
    (log2 (0.00001525878 + x * 0.5) + 9.72) / 17.52 :
                          (log2 (x) + 9.72) / 17.52;
}

//
// ACES Color Space Conversion - ACEScc to ACES
//
// Converts ACEScc     (AP1 w/ ACESlog encoding) to
//          ACES2065-1 (AP0 w/ linear encoding)
//
//  * This transform follows the formulas from section 4.4 in S-2014-003
//
float ACEScc_to_ACES (float x)
{
  if (IsNan (x))
    x = 0.0f;

  // TODO: Optimize me
  if (x < -0.3013698630) // (9.72 - 15) / 17.52
    return (pow(2.0, x * 17.52 - 9.72) - pow(2.0, -16.0)) * 2.0;
  else if (x < (log2(float_MAX) + 9.72) / 17.52)
    return pow(2.0, x * 17.52 - 9.72);
  else // (x >= (log2(float_MAX) + 9.72) / 17.52)
    return float_MAX;
}

float3 ACEScc_to_ACES (float3 x)
{
  if (AnyIsNan (x))
   x = float3 (0.0f, 0.0f, 0.0f);

  return float3 (
    ACEScc_to_ACES (x.r),
    ACEScc_to_ACES (x.g),
    ACEScc_to_ACES (x.b)
  );
}

//
// ACES Color Space Conversion - ACES to ACEScg
//
// Converts ACES2065-1 (AP0 w/ linear encoding) to
//          ACEScg     (AP1 w/ linear encoding)
//
float3 ACES_to_ACEScg (float3 x)
{
  return
    mul (AP0_2_AP1_MAT, x);
}

//
// ACES Color Space Conversion - ACEScg to ACES
//
// Converts ACEScg     (AP1 w/ linear encoding) to
//          ACES2065-1 (AP0 w/ linear encoding)
//
float3 ACEScg_to_ACES (float3 x)
{
  return
    mul (AP1_2_AP0_MAT, x);
}

//
// Reference Rendering Transform (RRT)
//
//    Input is ACES
//   Output is OCES
//
float rgb_2_saturation (float3 rgb)
{
  static const float TINY = 1e-4;

  float mi = Min3 (rgb.r, rgb.g, rgb.b);
  float ma = Max3 (rgb.r, rgb.g, rgb.b);

  return ( max (ma, TINY) -
           max (mi, TINY) ) /
           max (ma, 1e-2);
}

float rgb_2_yc (float3 rgb)
{
  const float ycRadiusWeight = 1.75;
  
  // Converts RGB to a luminance proxy, here called YC
  // YC is ~ Y + K * Chroma
  // Constant YC is a cone-shaped surface in RGB space, with the tip on the
  // neutral axis, towards white.
  // YC is normalized: RGB 1 1 1 maps to YC = 1
  //
  // ycRadiusWeight defaults to 1.75, although can be overridden in function
  // call to rgb_2_yc
  // ycRadiusWeight = 1 -> YC for pure cyan, magenta, yellow == YC for neutral
  // of same value
  // ycRadiusWeight = 2 -> YC for pure red, green, blue  == YC for  neutral of
  // same value.
  
  float r      = rgb.x;
  float g      = rgb.y;
  float b      = rgb.z;
  float chroma = sqrt (b * (b - g) + g * (g - r) + r * (r - b));

  return
    (b + g + r + ycRadiusWeight * chroma) / 3.0;
}

float rgb_2_hue (float3 rgb)
{
  // Returns a geometric hue angle in degrees (0-360) based on RGB values.
  // For neutral colors, hue is undefined and the function will return a quiet NaN value.
  float hue;
  if (rgb.x == rgb.y && rgb.y == rgb.z)
    hue = 0.0; // RGB triplets where RGB are equal have an undefined hue
  else
    hue = (180.0 / PI) * atan2(sqrt(3.0) * (rgb.y - rgb.z), 2.0 * rgb.x - rgb.y - rgb.z);
  
  if (hue < 0.0)
      hue = hue + 360.0;
  
  return hue;
}

float center_hue (float hue, float centerH)
{
  float hueCentered = hue - centerH;

  if (     hueCentered < -180.0) hueCentered = hueCentered + 360.0;
  else if (hueCentered >  180.0) hueCentered = hueCentered - 360.0;

  return hueCentered;
}

float sigmoid_shaper (float x)
{
  // Sigmoid function in the range 0 to 1 spanning -2 to +2.
  
  float t = max (1.0 - abs (x / 2.0), 0.0);
  float y = 1.0 + FastSign (x) * (1.0 - t * t);
  
  return
    y / 2.0;
}

float glow_fwd (float ycIn, float glowGainIn, float glowMid)
{
  float glowGainOut;
  
  if (ycIn <= 2.0 / 3.0 * glowMid)
    glowGainOut = glowGainIn;
  else if (ycIn >= 2.0 * glowMid)
    glowGainOut = 0.0;
  else
    glowGainOut = glowGainIn * (glowMid / ycIn - 1.0 / 2.0);
  
  return
    glowGainOut;
}
#endif

//
// White balance
// Recommended workspace: ACEScg (linear)
//
static const float3x3 LIN_2_LMS_MAT = {
  3.90405e-1, 5.49941e-1, 8.92632e-3,
  7.08416e-2, 9.63172e-1, 1.35775e-3,
  2.31082e-2, 1.28021e-1, 9.36245e-1
};

static const float3x3 LMS_2_LIN_MAT = {
   2.85847e+0, -1.62879e+0, -2.48910e-2,
  -2.10182e-1,  1.15820e+0,  3.24281e-4,
  -4.18120e-2, -1.18169e-1,  1.06867e+0
};

float3 WhiteBalance (float3 c, float3 balance)
{
  float3 lms  = mul (LIN_2_LMS_MAT, c);
         lms *= balance;

  return
    mul (LMS_2_LIN_MAT, lms);
}

//
// RGB / Full-range YCbCr conversions (ITU-R BT.601)
//
float3 RgbToYCbCr (float3 c)
{
  float Y  =  0.299 * c.r + 0.587 * c.g + 0.114 * c.b;
  float Cb = -0.169 * c.r - 0.331 * c.g + 0.500 * c.b;
  float Cr =  0.500 * c.r - 0.419 * c.g - 0.081 * c.b;

  return
    float3 (Y, Cb, Cr);
}

float3 YCbCrToRgb (float3 c)
{
  float R = c.x + 0.000 * c.y + 1.403 * c.z;
  float G = c.x - 0.344 * c.y - 0.714 * c.z;
  float B = c.x - 1.773 * c.y + 0.000 * c.z;

  return
    float3 (R, G, B);
}

//
// Hue, Saturation, Value
// Ranges:
//  Hue [0.0, 1.0]
//  Sat [0.0, 1.0]
//  Lum [0.0, float_MAX]
//
float3 RgbToHsv (float3 c)
{
  float4 K =       float4 (0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
  float4 p = lerp (float4 (c.bg, K.wz), float4 (c.gb, K.xy), step (c.b, c.g));
  float4 q = lerp (float4 (p.xyw, c.r), float4 (c.r, p.yzx), step (p.x, c.r));
  float  d = q.x - min (q.w, q.y);
  float  e = EPSILON;

  return
    float3 (abs (q.z + (q.w - q.y) / (6.0 * d + e)),
                                 d / (q.x + e), q.x);
}

float3 HsvToRgb (float3 c)
{
  float4 K =
    float4 ( 1.0, 2.0 / 3.0,
                  1.0 / 3.0, 3.0 );

  float3 p =
    abs (frac (c.xxx + K.xyz) * 6.0 - K.www);

  return
    c.z * lerp (K.xxx, saturate (p - K.xxx), c.y);
}

float RotateHue (float value, float low, float hi)
{
  return (value < low)
          ? value + hi
          : (value > hi)
              ? value - hi
              : value;
}

//
// Channel mixing (same as Photoshop's and DaVinci's Resolve)
// Recommended workspace: ACEScg (linear)
//      Input mixers should be in range [-2.0; 2.0]
//
float3 ChannelMixer (float3 c, float3 red, float3 green, float3 blue)
{
  return
    float3 (
      dot (c, red),
      dot (c, green),
      dot (c, blue)
    );
}

float3 gain (float3 x, float k)
{
  const float3 c =
    0.5 * pow ( 2.0 * ( ( x < 0.5 ) ?
                          x         :
                    1.0 - x ), k );
  return
    (x < 0.5) ?
            c : 1.0 - c;
}







// Apply the (approximate) sRGB curve to linear values
float3 LinearToSRGBEst (float3 color)
{
  return
    PositivePow (color, 1.0f / 2.2f);
}


// (Approximate) sRGB to linear
float3 SRGBToLinearEst (float3 srgb)
{
  return
    PositivePow (srgb, 2.2f);
}


// HDR10 Media Profile
// https://en.wikipedia.org/wiki/High-dynamic-range_video#HDR10


// Color rotation matrix to rotate Rec.709 color primaries into Rec.2020
static const float3x3 from709to2020 =
{
  { 0.627225305694944,  0.329476882715808,  0.0432978115892484, },
  { 0.0690418812810714, 0.919605681354755,  0.0113524373641739, },
  { 0.0163911702607078, 0.0880887513437058, 0.895520078395586   }
};


// Reinhard tonemap operator
// Reinhard et al. "Photographic tone reproduction for digital images." ACM Transactions on Graphics. 21. 2002.
// http://www.cs.utah.edu/~reinhard/cdrom/tonemap.pdf
float3 ToneMapReinhard (float3 color)
{
  return
    color / (1.0f + color);
}


// ACES Filmic tonemap operator
// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
float3 ToneMapACESFilmic (float3 x)
{
  const float a = 2.51f;
  const float b = 0.03f;
  const float c = 2.43f;
  const float d = 0.59f;
  const float e = 0.14f;

  return
    saturate ( (x * (a * x + b)) /
               (x * (c * x + d) + e) );
}


// sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT
static const float3x3 ACESInputMat =
{
  {0.59719, 0.35458, 0.04823},
  {0.07600, 0.90834, 0.01566},
  {0.02840, 0.13383, 0.83777}
};

// ODT_SAT => XYZ => D60_2_D65 => sRGB
static const float3x3 ACESOutputMat =
{
  { 1.60475, -0.53108, -0.07367},
  {-0.10208,  1.10813, -0.00605},
  {-0.00327, -0.07276,  1.07602}
};

float3 RRTAndODTFit (float3 v)
{
  const float3 a = v * (v + 0.0245786f)    - 0.000090537f;
  const float3 b = v * (     0.983729f * v + 0.4329510f) + 0.238081f;

  return ( a / b );
}

float3 ToneMapACES (float3 hdr)
{
  const float
    A = 2.51,
    B = 0.03,
    C = 2.43,
    D = 0.59,
    E = 0.14;

  return
    saturate ( (hdr * (A * hdr + B)) /
               (hdr * (C * hdr + D) + E) );
}



float3 ACESFitted (float3 color, float input_color, float gamma)
{
  if (! any (color.rgb))
    return 0.0f;

  color  = pow (color, float3 (0.82f, 0.82f, 0.82f));
  color *= 1.12f;

  float  luma             = length    (color);
  float3 normalized_color = normalize (color);

  color =
    luma * float3 ( 0.5773502691896258,
                    0.5773502691896258,
                    0.5773502691896258 );

  return
    ToneMapACES (color) * normalized_color;
#if 0
  color.rgb  =
    max ( 0.0f,           color.rgb < 1.0 ?
         pow ( max ( 0.0, color.rgb ), 0.81429f ) :
                          color.rgb );

  color.rgb  = mul (         ACESInputMat,
                             color.rgb);
  color.rgb  = LinearToLogC (color.rgb);
  color.rgb  = Contrast (    color.rgb, 0.18f * (0.1f * input_color / 0.0125f) / 100.0f, (sdrLuminance_NonStd / 0.0125f) / 100.0f);
  color.rgb = LogCToLinear (color.rgb);

  // Apply RRT and ODT
//color.rgb = Saturation (                 color.rgb, hdrSaturation);
  color.rgb = RRTAndODTFit (               color.rgb);
  color.rgb = mul (         ACESOutputMat, color.rgb);

  return
    max (           0.0f,
         min ( color.rgb, 50.0f ) ) * 1.3f; // Clamp to 4,000 nits
#endif
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

bool
SK_Triangle_ContainsPoint ( float3 pt, float3 vTriangle [3] )
{
  float3 vBary =
    SK_Triangle_Barycentric ( pt, vTriangle );

  return (
    vBary.x >= 0.0 && vBary.y  >= 0.0 &&
   (vBary.x         + vBary.y) <  1.0
  );
}


float3
RGBFromWavelength (float w)
{
  if ( w >= 380.0f &&
       w <  440.0f )
  {
    return float3 (
      -(w - 440.0f)
          /  60.0f,
              0.0f,
              1.0f );
  }

  else if ( w >= 440.0f &&
            w <  490.0f )
  {
    return float3 ( 0.0f,
             (w - 440.0f)
                /  40.0f,
                    1.0f );
  }

  else if ( w >= 490.0f &&
            w <  510.0f )
  {
    return float3 ( 0.0f, 1.0f,
            -(w - 510.0f)
                /  20.0f );
  }

  else if ( w >= 510.0f &&
            w <  580.0f )
  {
    return float3 ( (w - 510.0f)
                       /  70.0f, 1.0f, 0.0f );
  }

  else if ( w >= 580.0f &&
            w <  645.0f )
  {
    return float3 ( 1.0f,
            -(w - 645.0f)
                /  65.0f,
                    0.0f );
  }

  else if ( w >= 645 &&
            w <= 780.0f )
  {
    return
      float3 (1.0f, 0.0f, 0.0f);
  }

  return
    float3 (0.0f, 0.0f, 0.0f);
}

float4 colormap (float x)
{
  return
    float4 (
      RGBFromWavelength (
        380.0f + 400.0f * x
      ),           1.0f
    );
}


float3 Rec709toAP0_D65 (float3 linearRec709)
{
  static const float3x3 ConvMat =
  {
    0.433931618928909301757812500f, 0.376252382993698120117187500f, 0.18981596827507019042968750f,
    0.088618390262126922607421875f, 0.809275329113006591796875000f, 0.10210628807544708251953125f,
    0.017750039696693420410156250f, 0.109447620809078216552734375f, 0.87280231714248657226562500f
  };

  return mul (ConvMat, linearRec709);
}

float3 AP0_D65toRec709 (float3 linearAP0)
{
  static const float3x3 ConvMat =
  {
     2.55248308181762695312500000000f, -1.12950992584228515625000000f, -0.4229732155799865722656250f,
    -0.27734413743019104003906250000f,  1.37826657295227050781250000f, -0.1009224355220794677734375f,
    -0.01713105104863643646240234375f, -0.14986114203929901123046875f,  1.1669921875000000000000000f
  };

  return mul (ConvMat, linearAP0);
}

float3 Rec709toAP1_D65 (float3 linearRec709)
{
  static const float3x3 ConvMat =
  {
    0.61702883243560791015625000000f, 0.333867609500885009765625000f, 0.049103543162345886230468750000f,
    0.06992232054471969604492187500f, 0.917349696159362792968750000f, 0.012727967463433742523193359375f,
    0.02054978720843791961669921875f, 0.107552029192447662353515625f, 0.871898174285888671875000000000f
  };

  return mul (ConvMat, linearRec709);
}

float3 AP1_D65toRec709 (float3 linearAP1)
{
  static const float3x3 AP1_D65toXYZ =
  {
     0.64750719070434570312500000000f, 0.134379133582115173339843750000f, 0.168569594621658325195312500f,
     0.26608639955520629882812500000f, 0.675967812538146972656250000000f, 0.057945795357227325439453125f,
    -0.00544886849820613861083984375f, 0.004072095267474651336669921875f, 1.090434551239013671875000000f
  };

  static const float3x3 XYZtoRec709 =
  {
     3.240969896316528320312500000f, -1.5373831987380981445312500f, -0.4986107647418975830078125000f,
    -0.969243645668029785156250000f,  1.8759675025939941406250000f,  0.0415550582110881805419921875f,
     0.055630080401897430419921875f, -0.2039769589900970458984375f,  1.0569715499877929687500000000f
  };

  return mul (XYZtoRec709, mul (AP1_D65toXYZ, linearAP1));
}

float3
sRGBtoXYZ (float3 color)
{
  return
    mul ( AP1_2_XYZ_MAT,
         mul ( sRGB_2_AP1, color ) );
}

float3
XYZtosRGB (float3 color)
{
  return
    mul ( AP1_2_sRGB,
         mul ( XYZ_2_AP1_MAT, color ) );
}





























// Expand bright saturated colors outside the sRGB (REC.709) gamut to fake wide gamut rendering (BT.2020).
// Inspired by Unreal Engine 4/5 (ACES).
// Input (and output) needs to be in sRGB linear space.
// Calling this with a fExpandGamut value of 0 still results in changes (it's actually an edge case, don't call it).
// Calling this with fExpandGamut values above 1 yields diminishing returns.
float3
expandGamut (float3 vHDRColor, float fExpandGamut = 1.0f)
{
  if (fExpandGamut <= 0.0f)
    return vHDRColor;

  //AP1 with D65 white point instead of the custom white point from ACES which is around 6000K
  const float3x3 sRGB_2_AP1_D65_MAT =
  {
    0.6168509940091290, 0.334062934274955, 0.0490860717159169,
    0.0698663939791712, 0.917416678964894, 0.0127169270559354,
    0.0205490668158849, 0.107642210710817, 0.8718087224732980
  };
  const float3x3 AP1_D65_2_sRGB_MAT =
  {
     1.6926793984921500, -0.606218057156000, -0.08646134133615040,
    -0.1285739800722680,  1.137933633392290, -0.00935965332001697,
    -0.0240224650921189, -0.126211717940702,  1.15023418303282000
  };
  const float3x3 AP1_D65_2_XYZ_MAT =
  {
     0.64729265784680500, 0.13440339917805700, 0.1684710654303190,
     0.26599824508992100, 0.67608982616840700, 0.0579119287416720,
    -0.00544706303938401, 0.00407283027812294, 1.0897972045023700
  };
  // Bizarre matrix but this expands sRGB to between P3 and AP1
  // CIE 1931 chromaticities:   x         y
  //                Red:        0.6965    0.3065
  //                Green:      0.245     0.718
  //                Blue:       0.1302    0.0456
  //                White:      0.3127    0.3291 (=D65)
  const float3x3 Wide_2_AP1_D65_MAT = 
  {
    0.83451690546233900, 0.1602595895494930, 0.00522350498816804,
    0.02554519357785500, 0.9731015318660700, 0.00135327455607548,
    0.00192582885428273, 0.0303727970124423, 0.96770137413327500
  };
  const float3x3
         ExpandMat = mul (Wide_2_AP1_D65_MAT, AP1_D65_2_sRGB_MAT);
  float3 ColorAP1  = mul (sRGB_2_AP1_D65_MAT, vHDRColor);

  float  LumaAP1   = Rec709_to_XYZ (AP1_D65toRec709 (ColorAP1)).y;
  float3 ChromaAP1 =      ColorAP1 / LumaAP1;

  float ChromaDistSqr =
    dot (ChromaAP1 - 1.0f,
         ChromaAP1 - 1.0f);

  ChromaDistSqr =
    max (abs (ChromaDistSqr), 0.000001f);

  float ExpandAmount  = (1.0f - exp2 (-4.0f * ChromaDistSqr)) * (1.0f - exp2 (-4.0f * fExpandGamut * LumaAP1 * LumaAP1));

  float3 ColorExpand =
    mul (ExpandMat, ColorAP1);
  
  ColorAP1 =
    lerp (ColorAP1, ColorExpand, ExpandAmount);

  vHDRColor =
    mul (AP1_2_sRGB, ColorAP1);
  
  return vHDRColor;
}

float3 Rec709toICtCp (float3 c)
{
  c = Rec709_to_XYZ (c);
  c = XYZ_to_LMS    (c);
  
  c =
    LinearToPQ (c, 125.0f);

  static const float3x3 ConvMat =
  {
    0.5000,  0.5000,  0.0000,
    1.6137, -3.3234,  1.7097,
    4.3780, -4.2455, -0.1325
  };

  return
    mul (ConvMat, c);
}

float3 ICtCptoRec709 (float3 c)
{
  static const float3x3 ConvMat =
  {
    1.0,  0.00860514569398152,  0.11103560447547328,
    1.0, -0.00860514569398152, -0.11103560447547328,
    1.0,  0.56004885956263900, -0.32063747023212210
  };
  
  c =
    mul (ConvMat, c);
  
  c = PQToLinear (c, 125.0f);
  c = LMS_to_XYZ (c);
  
  return
    XYZ_to_Rec709 (c);
}

float3 LMS_to_ICtCp (float3 LMS)
{
  LMS =
    LinearToPQ (LMS);

  static const float3x3 ConvMat =
  {
     2048.0,   2048.0,    0.0,
     6610.0, -13613.0, 7003.0,
    17933.0, -17390.0, -543.0
  };

  return
    mul (mul (ConvMat, 1.0 / 4096.0), LMS);
}

float3 ICtCp_to_LMS (float3 ICtCp)
{
  static const float3x3 ConvMat =
  {
    0.99998889656284013833,  0.00860505014728705821,  0.11103437159861647860,
    1.00001110343715986160, -0.00860505014728705821, -0.11103437159861647860,
    1.00003206339100541200,  0.56004913547279000113, -0.32063391005412026469
  };

  ICtCp =
    mul (ConvMat, ICtCp);

  ICtCp =
    PQToLinear (ICtCp);

  return ICtCp;
}

float3 RGB_to_ICtCp (float3 color)
{
  color /= 125.0f;

  color = Rec709_to_XYZ (color);
  color = XYZ_to_LMS    (color);
  color = LMS_to_ICtCp  (color);

  return color;
}

float3 ICtCp_to_RGB (float3 color)
{
  color = ICtCp_to_LMS  (color);
  color = LMS_to_XYZ    (color);
  color = XYZ_to_Rec709 (color);

  color *= 125.0f;

  return color;
}