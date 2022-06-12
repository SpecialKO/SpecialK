#pragma warning ( disable : 3571 )
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

struct PS_INPUT
{
  float4 pos      : SV_POSITION;
  float4 color    : COLOR0;
  float2 uv       : TEXCOORD0;
  float2 coverage : TEXCOORD1;
};
sampler   sampler0     : register (s0);
Texture2D texMainScene : register (t0);

static const float _PI     = 3.141592654;
static const float _TWO_PI = 2 * _PI;

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

#define VISUALIZE_AVG_LUMA       1
#define VISUALIZE_LOCAL_LUMA     2
//#define VISUALIZE_HDR400_LUMA   4
//#define VISUALIZE_HDR500_LUMA   5
//#define VISUALIZE_HDR600_LUMA   6
//#define VISUALIZE_HDR1000_LUMA  7
//#define VISUALIZE_HDR1400_LUMA  8
#define VISUALIZE_EXPOSURE       3
#define VISUALIZE_HIGHLIGHTS     4
#define VISUALIZE_8BIT_QUANTIZE  5
#define VISUALIZE_10BIT_QUANTIZE 6
#define VISUALIZE_12BIT_QUANTIZE 7
#define VISUALIZE_16BIT_QUANTIZE 8
#define VISUALIZE_REC709_GAMUT   9
#define VISUALIZE_DCIP3_GAMUT    10
#define VISUALIZE_REC2020_GAMUT  11

#define TONEMAP_NONE                  0
#define TONEMAP_ACES_FILMIC           1
#define TONEMAP_HDR10_to_scRGB        2
#define TONEMAP_HDR10_to_scRGB_FILMIC 3
#define TONEMAP_COPYRESOURCE          255

#define minmax(m0,m1,c) min (max ((c), (m0)), (m1))

float3
RemoveSRGBCurve (float3 x)
{
  // Negative values can come back negative after gamma, unlike pow (...).
  x = /* High-Pass filter the input to clip negative values to 0 */
    max ( 0.0, isfinite (x) ? x : 0.0 );

  // Piecewise is more accurate, but the fitted power-law curve will not
  //   create near-black noise when expanding LDR -> HDR
#define ACCURATE_AND_NOISY
#ifdef  ACCURATE_AND_NOISY
  return ( x < 0.04045f ) ?
          (x / 12.92f)    : // High-pass filter x or gamma will return negative!
    pow ( (x + 0.055f) / 1.055f, 2.4f );
#else
  // This suffers the same problem as piecewise; x * x * x allows negative color.
  return x * (x * (x * 0.305306011 + 0.682171111) + 0.012522878);
#endif
}

float
RemoveSRGBAlpha (float a)
{
  return  ( a < 0.04045f ) ?
            a / 12.92f     :
    pow ( ( a + 0.055f   ) / 1.055f,
                              2.4f );
}

float3
ApplySRGBCurve (float3 x)
{
  return ( x < 0.0031308f ? 12.92f * x :
                            1.055f * pow ( x, 1.0 / 2.4f ) - 0.55f );
}

float3 Clamp_scRGB (float3 c)
{
  return
    clamp (c, 0.0f, 125.0f);
}

float Clamp_scRGB (float c)
{
  return
    clamp (c, 0.0f, 125.0f);
}

//
// Convert rgb to luminance with rgb in linear space with sRGB primaries and D65 white point
//
float Luminance (float3 linearRgb)
{
  return
    dot (linearRgb, float3 (0.2126729, 0.7151522, 0.0721750));
}

float Luminance (float4 linearRgba)
{
  return
    Luminance (linearRgba.rgb);
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
  return
    (c - midpoint) * contrast
       + midpoint;
}


#define FLT_EPSILON     1.192092896e-07 // Smallest positive number, such that 1.0 + FLT_EPSILON != 1.0
#define FLT_MIN         1.175494351e-38 // Minimum representable positive floating-point number
#define FLT_MAX         3.402823466e+38 // Maximum representable floating-point number

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
    pow ( max (abs (base), float (FLT_EPSILON)), power );
}

float2 PositivePow (float2 base, float2 power)
{
  return
    pow (max (abs (base), float2(FLT_EPSILON, FLT_EPSILON)), power );
}

float3 PositivePow (float3 base, float3 power)
{
  return
    pow (max (abs (base), float3 ( FLT_EPSILON, FLT_EPSILON,
                                   FLT_EPSILON )), power );
}

float4 PositivePow (float4 base, float4 power)
{
  return
    pow (max (abs (base), float4 ( FLT_EPSILON, FLT_EPSILON,
                                   FLT_EPSILON, FLT_EPSILON )), power );
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
		  H = ( 180.0 / _PI ) * atan2 (
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
ApplyREC709Curve (float3 x)
{
	return ( x < 0.0181f ) ?
    4.5f * x : 1.0993f * pow (x, 0.45f) - 0.0993f;
}

float3
REC709toREC2020 (float3 RGB709)
{
  static const float3x3 ConvMat =
  {
    0.627402, 0.329292, 0.043306,
    0.069095, 0.919544, 0.011360,
    0.016394, 0.088028, 0.895578
  };

  return mul (ConvMat, RGB709);
}


// -----------------------------------------------------------------------------
// API macros

#if defined(SHADER_API_PSSL) || defined(SHADER_API_XBOXONE) || defined(SHADER_API_SWITCH) || defined(SHADER_API_PSP2)
    #define SHADER_API_CONSOLE
#endif

// -----------------------------------------------------------------------------
// Constants

#define float_MAX        65504.0 // (2 - 2^-10) * 2^15
#define float_MAX_MINUS1 65472.0 // (2 - 2^-9) * 2^15
#define EPSILON         1.0e-4
#define PI              3.14159265359
#define TWO_PI          6.28318530718
#define FOUR_PI         12.56637061436
#define INV_PI          0.31830988618
#define INV_TWO_PI      0.15915494309
#define INV_FOUR_PI     0.07957747155
#define float_PI         1.57079632679
#define INV_float_PI     0.636619772367

// -----------------------------------------------------------------------------
// Compatibility functions

#if (SHADER_TARGET < 50 && !defined(SHADER_API_PSSL))
float rcp(float value)
{
    return 1.0 / value;
}
#endif

#if defined(SHADER_API_GLES)
#define mad(a, b, c) (a * b + c)
#endif

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

// NaN checker
// /Gic isn't enabled on fxc so we can't rely on isnan() anymore
bool IsNan (float x)
{
  // For some reason the following tests outputs "internal compiler error" randomly on desktop
  // so we'll use a safer but slightly slower version instead :/
  //return (x <= 0.0 || 0.0 <= x) ? false : true;
  return
    (x < 0.0 || x > 0.0 || x == 0.0) ?
                               false : true;
}

bool AnyIsNan (float2 x)
{
  return IsNan (x.x) ||
         IsNan (x.y);
}

bool AnyIsNan (float3 x)
{
  return IsNan (x.x) ||
         IsNan (x.y) ||
         IsNan (x.z);
}

bool AnyIsNan (float4 x)
{
  return IsNan (x.x) ||
         IsNan (x.y) ||
         IsNan (x.z) ||
         IsNan (x.w);
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

// Decode normals stored in _CameraDepthNormalsTexture
float3 DecodeViewNormalStereo (float4 enc4)
{
  float  kScale = 1.7777;
  float3 nn     = enc4.xyz * float3 (2.0 * kScale, 2.0 * kScale, 0) + 
                             float3 (     -kScale,      -kScale, 1);
  float   g     = 2.0 / dot (nn.xyz, nn.xyz);  
  float3  n;

  n.xy = g * nn.xy;
  n.z  = g - 1.0;

  return n;
}

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


#define LUT_SPACE_ENCODE(x) LinearToLogC(x)
#define LUT_SPACE_DECODE(x) LogCToLinear(x)

#ifndef USE_PRECISE_LOGC
  // Set to 1 to use more precise but more expensive log/linear conversions. I haven't found a proper
  // use case for the high precision version yet so I'm leaving this to 0.
  #define USE_PRECISE_LOGC 0
#endif

#ifndef TONEMAPPING_USE_FULL_ACES
  // Set to 1 to use the full reference ACES tonemapper. This should only be used for research
  // purposes as it's quite heavy and generally overkill.
  #define TONEMAPPING_USE_FULL_ACES 1
#endif

#ifndef DEFAULT_MAX_PQ
  // PQ ST.2048 max value
  // 1.0 = 100nits, 100.0 = 10knits
  #define DEFAULT_MAX_PQ 100.0
#endif

#ifndef USE_VERY_FAST_SRGB
  #if defined(SHADER_API_MOBILE)
      #define USE_VERY_FAST_SRGB 1
  #else
      #define USE_VERY_FAST_SRGB 0
  #endif
#endif

#ifndef USE_FAST_SRGB
  #if defined(SHADER_API_CONSOLE)
      #define USE_FAST_SRGB 1
  #else
      #define USE_FAST_SRGB 0
  #endif
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
  float N, M;
  float C1, C2, C3;
};

static const ParamsPQ PQ =
{
  2610.0 / 4096.0 / 4.0,   // N
  2523.0 / 4096.0 * 128.0, // M
  3424.0 / 4096.0,         // C1
  2413.0 / 4096.0 * 32.0,  // C2
  2392.0 / 4096.0 * 32.0,  // C3
};

float3 LinearToPQ (float3 x, float maxPQValue)
{
  x =
    PositivePow ( x / maxPQValue,
                         PQ.N );
  
  float3 nd =
    (PQ.C1 + PQ.C2 * x) /
      (1.0 + PQ.C3 * x);

  return
    Clamp_scRGB (PositivePow (nd, PQ.M));
}

float3 LinearToPQ (float3 x)
{
  return
    LinearToPQ (x, DEFAULT_MAX_PQ);
}

float3 PQToLinear (float3 x, float maxPQValue)
{
  x =
    PositivePow (x, rcp (PQ.M));

  float3 nd =
    max (x - PQ.C1, 0.0) /
            (PQ.C2 - (PQ.C3 * x));

  return
    Clamp_scRGB (PositivePow (nd, rcp (PQ.N)) * maxPQValue);
}

float3 PQToLinear (float3 x)
{
  return
    PQToLinear (x, DEFAULT_MAX_PQ);
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

//
// Unity to ACES
//
// converts Unity raw (sRGB primaries) to
//          ACES2065-1 (AP0 w/ linear encoding)
//
float3 unity_to_ACES (float3 x)
{
  x =
    mul (sRGB_2_AP0, x);

  return x;
}

//
// ACES to Unity
//
// converts ACES2065-1 (AP0 w/ linear encoding)
//          Unity raw (sRGB primaries) to
//
float3 ACES_to_unity (float3 x)
{
  x =
    mul (AP0_2_sRGB, x);

  return x;
}

//
// Unity to ACEScg
//
// converts Unity raw (sRGB primaries) to
//          ACEScg (AP1 w/ linear encoding)
//
float3 unity_to_ACEScg (float3 x)
{
  x =
    mul (sRGB_2_AP1, x);

  return x;
}

//
// ACEScg to Unity
//
// converts ACEScg (AP1 w/ linear encoding) to
//          Unity raw (sRGB primaries)
//
float3 ACEScg_to_unity (float3 x)
{
  x =
    mul (AP1_2_sRGB, x);

  return x;
}

//
// ACES Color Space Conversion - ACES to ACEScc
//
// converts ACES2065-1 (AP0 w/ linear encoding) to
//          ACEScc (AP1 w/ logarithmic encoding)
//
// This transform follows the formulas from section 4.4 in S-2014-003
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
  x =
    clamp (x, 0.0, float_MAX);
  
  // x is clamped to [0, float_MAX], skip the <= 0 check
  return
      (x < 0.00003051757)                           ?
    (log2 (0.00001525878 + x * 0.5) + 9.72) / 17.52 :
                          (log2 (x) + 9.72) / 17.52;
  
  /*
  return float3(
      ACES_to_ACEScc(x.r),
      ACES_to_ACEScc(x.g),
      ACES_to_ACEScc(x.b)
  );
  */
}

//
// ACES Color Space Conversion - ACEScc to ACES
//
// converts ACEScc (AP1 w/ ACESlog encoding) to
//          ACES2065-1 (AP0 w/ linear encoding)
//
// This transform follows the formulas from section 4.4 in S-2014-003
//
float ACEScc_to_ACES (float x)
{
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
  return float3 (
    ACEScc_to_ACES (x.r),
    ACEScc_to_ACES (x.g),
    ACEScc_to_ACES (x.b)
  );
}

//
// ACES Color Space Conversion - ACES to ACEScg
//
// converts ACES2065-1 (AP0 w/ linear encoding) to
//          ACEScg (AP1 w/ linear encoding)
//
float3 ACES_to_ACEScg (float3 x)
{
  return
    mul (AP0_2_AP1_MAT, x);
}

//
// ACES Color Space Conversion - ACEScg to ACES
//
// converts ACEScg (AP1 w/ linear encoding) to
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
//   Input is ACES
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


#define UNITY_UV_STARTS_AT_TOP 1
#define UNITY_REVERSED_Z 1
#define UNITY_GATHER_SUPPORTED (SHADER_TARGET >= 50)
#define UNITY_CAN_READ_POSITION_IN_FRAGMENT_PROGRAM 1

#define TEXTURE2D_SAMPLER2D(textureName, samplerName) Texture2D textureName; SamplerState samplerName
#define TEXTURE3D_SAMPLER3D(textureName, samplerName) Texture3D textureName; SamplerState samplerName

#define TEXTURE2D(textureName) Texture2D textureName
#define SAMPLER2D(samplerName) SamplerState samplerName

#define TEXTURE3D(textureName) Texture3D textureName
#define SAMPLER3D(samplerName) SamplerState samplerName

#define TEXTURE2D_ARGS(textureName, samplerName) Texture2D textureName, SamplerState samplerName
#define TEXTURE2D_PARAM(textureName, samplerName) textureName, samplerName

#define TEXTURE3D_ARGS(textureName, samplerName) Texture3D textureName, SamplerState samplerName
#define TEXTURE3D_PARAM(textureName, samplerName) textureName, samplerName

#define SAMPLE_TEXTURE2D(textureName, samplerName, coord2) textureName.Sample(samplerName, coord2)
#define SAMPLE_TEXTURE2D_LOD(textureName, samplerName, coord2, lod) textureName.SampleLevel(samplerName, coord2, lod)

#define SAMPLE_TEXTURE3D(textureName, samplerName, coord3) textureName.Sample(samplerName, coord3)

#define LOAD_TEXTURE2D(textureName, texelSize, icoord2) textureName.Load(int3(icoord2, 0))
#define LOAD_TEXTURE2D_LOD(textureName, texelSize, icoord2) textureName.Load(int3(icoord2, lod))

#define GATHER_TEXTURE2D(textureName, samplerName, coord2) textureName.Gather(samplerName, coord2)
#define GATHER_RED_TEXTURE2D(textureName, samplerName, coord2) textureName.GatherRed(samplerName, coord2)
#define GATHER_GREEN_TEXTURE2D(textureName, samplerName, coord2) textureName.GatherGreen(samplerName, coord2)
#define GATHER_BLUE_TEXTURE2D(textureName, samplerName, coord2) textureName.GatherBlue(samplerName, coord2)

#define SAMPLE_DEPTH_TEXTURE(textureName, samplerName, coord2) SAMPLE_TEXTURE2D(textureName, samplerName, coord2).r
#define SAMPLE_DEPTH_TEXTURE_LOD(textureName, samplerName, coord2, lod) SAMPLE_TEXTURE2D_LOD(textureName, samplerName, coord2, lod).r

#define UNITY_BRANCH    [branch]
#define UNITY_FLATTEN   [flatten]
#define UNITY_UNROLL    [unroll]
#define UNITY_LOOP      [loop]
#define UNITY_FASTOPT   [fastopt]

#define CBUFFER_START(name) cbuffer name {
#define CBUFFER_END };

#if UNITY_GATHER_SUPPORTED
  #define FXAA_HLSL_5 1
  #define SMAA_HLSL_4_1 1
#else
  #define FXAA_HLSL_4 1
  #define SMAA_HLSL_4 1
#endif


//
// 3D LUT grading
// scaleOffset = (1 / lut_size, lut_size - 1)
//
float3 ApplyLut3D (TEXTURE3D_ARGS (tex, samplerTex), float3 uvw, float2 scaleOffset)
{
  uvw.xyz =
    uvw.xyz * scaleOffset.yyy *
              scaleOffset.xxx + scaleOffset.xxx * 0.5;
  
  return
    SAMPLE_TEXTURE3D (tex, samplerTex, uvw).rgb;
}

//
// 2D LUT grading
// scaleOffset = (1 / lut_width, 1 / lut_height, lut_height - 1)
//
float3 ApplyLut2D (TEXTURE2D_ARGS (tex, samplerTex), float3 uvw, float3 scaleOffset)
{
  // Strip format where `height = sqrt(width)`
  uvw.z *= scaleOffset.z;

  float shift =
    floor (uvw.z);

  uvw.xy   = uvw.xy * scaleOffset.z * scaleOffset.xy + scaleOffset.xy * 0.5;
  uvw.x   += shift  * scaleOffset.y;
  uvw.xyz  = lerp (
    SAMPLE_TEXTURE2D (tex, samplerTex, uvw.xy).rgb,
    SAMPLE_TEXTURE2D (tex, samplerTex, uvw.xy +
                        float2 (scaleOffset.y, 0.0)).rgb,
                                       uvw.z - shift );

  return uvw;
}

//
// Returns the default value for a given position on a 2D strip-format color lookup table
// params = (lut_height, 0.5 / lut_width, 0.5 / lut_height, lut_height / lut_height - 1)
//
float3 GetLutStripValue (float2 uv, float4 params)
{
  uv -= params.yz;

  float3 color;

  color.r = frac (uv.x           * params.x);
  color.b =       uv.x - color.r / params.x;
  color.g =       uv.y;

  return
    color * params.w;
}

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
// Remaps Y/R/G/B values
// curveTex has to be 128 pixels wide
//
float3 YrgbCurve (float3 c, TEXTURE2D_ARGS(curveTex, sampler_curveTex))
{
  const float kfloatPixel = (1.0 / 128.0) / 2.0;

  // Y (master)
  c += kfloatPixel.xxx;
  float mr = SAMPLE_TEXTURE2D (curveTex, sampler_curveTex, float2 (c.r, 0.75)).a;
  float mg = SAMPLE_TEXTURE2D (curveTex, sampler_curveTex, float2 (c.g, 0.75)).a;
  float mb = SAMPLE_TEXTURE2D (curveTex, sampler_curveTex, float2 (c.b, 0.75)).a;

  c = saturate (float3(mr, mg, mb));
  
  // RGB
  c += kfloatPixel.xxx;
  float r = SAMPLE_TEXTURE2D (curveTex, sampler_curveTex, float2 (c.r, 0.75)).r;
  float g = SAMPLE_TEXTURE2D (curveTex, sampler_curveTex, float2 (c.g, 0.75)).g;
  float b = SAMPLE_TEXTURE2D (curveTex, sampler_curveTex, float2 (c.b, 0.75)).b;
  
  return
    saturate (float3 (r, g, b));
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


#define sRGB_to_Linear           0
#define xRGB_to_Linear           1
#define sRGB_to_scRGB_as_DCIP3   2
#define sRGB_to_scRGB_as_Rec2020 3
#define sRGB_to_scRGB_as_XYZ     4

float4
SK_ProcessColor4 ( float4 color,
                      int func,
                      int strip_srgb = 1 )
{
  float4 out_color =
    float4 (
      (strip_srgb && func != sRGB_to_Linear) ?
                 RemoveSRGBCurve (color.rgb) : color.rgb,
                                               color.a
    );

  // Straight Pass-Through
  if (func <= xRGB_to_Linear)
      return out_color;

  return
    float4 (0.0f, 0.0f, 0.0f, 0.0f);
}


// Apply the (approximate) sRGB curve to linear values
float3 LinearToSRGBEst (float3 color)
{
  return
    pow (abs (color), 1/2.2f);
}


// (Approximate) sRGB to linear
float3 SRGBToLinearEst (float3 srgb)
{
  return
    pow (abs (srgb), 2.2f);
}


// HDR10 Media Profile
// https://en.wikipedia.org/wiki/High-dynamic-range_video#HDR10


// Color rotation matrix to rotate Rec.709 color primaries into Rec.2020
static const float3x3 from709to2020 =
{
  { 0.6274040f, 0.3292820f, 0.0433136f },
  { 0.0690970f, 0.9195400f, 0.0113612f },
  { 0.0163916f, 0.0880132f, 0.8955950f }
};


// Apply the ST.2084 curve to normalized linear values and outputs normalized non-linear values
float3 LinearToST2084 (float3 normalizedLinearValue)
{
  return
    pow (
      (0.8359375f + 18.8515625f * pow (abs (normalizedLinearValue), 0.1593017578f)) /
            (1.0f + 18.6875f    * pow (abs (normalizedLinearValue), 0.1593017578f)), 78.84375f
        );
}


// ST.2084 to linear, resulting in a linear normalized value
float3 ST2084ToLinear (float3 ST2084)
{
  return
    pow ( max (
      pow ( abs (ST2084), 1.0f / 78.84375f) - 0.8359375f, 0.0f) / (18.8515625f - 18.6875f *
      pow ( abs (ST2084), 1.0f / 78.84375f)),
                          1.0f / 0.1593017578f
        );
}


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
  float a = 2.51f;
  float b = 0.03f;
  float c = 2.43f;
  float d = 0.59f;
  float e = 0.14f;

  return
    saturate ((x * (a * x + b)) / (x * (c * x + d) + e));
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
  float3 a = v * (v + 0.0245786f)    - 0.000090537f;
  float3 b = v * (     0.983729f * v + 0.4329510f) + 0.238081f;

  return ( a / b );
}



float3 ACESFitted (float3 color, float input_color, float gamma)
{
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
}



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


struct SK_ColorSpace
{
  float    xr, yr,
           xg, yg,
           xb, yb,
           Xw, Yw, Zw;

  float    minY, maxLocalY, maxY;
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

#if 0
bool
SK_Triangle_ContainsPoint ( const float3 pt,
                            const float3 vTriangle [3] )
{
  float theta = 0.0f;

  float3 v1 = normalize (pt - vTriangle [0]);
  float3 v2 = normalize (pt - vTriangle [1]);
  float3 v3 = normalize (pt - vTriangle [2]);

  theta += acos (dot (v1, v2));
  theta += acos (dot (v2, v3));
  theta += acos (dot (v3, v1));

  return
    abs (theta - _TWO_PI) < 0.000001;
}
#else
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
#endif


float3
RGBFromWavelength (float w)
{
  if (w >= 380.0f && w < 440.0f)
  {
    return float3 (
      -(w - 440.0f) /
           (440.0f - 380.0f),
              0.0f,
              1.0f );
  }
  else if (w >= 440.0f && w < 490.0f)
  {
    return float3 ( 0.0f,
             (w - 440.0f) /
        (490.0f - 440.0f),
                    1.0f );
  }
  else if (w >= 490.0f && w < 510.0f)
  {
    return float3 ( 0.0f, 1.0f,
            -(w - 510.0f) /
                 (510.0f - 490.0f) );
  }
  else if (w >= 510.0f && w < 580.0f)
  {
    return float3 ( (w - 510.0f) /
               (580.0f - 510.0f), 1.0f, 0.0f );
  }
  else if (w >= 580.0f && w < 645.0f)
  {
    return float3 ( 1.0f,
            -(w - 645.0f) /
                 (645.0f - 580.0f),
                    0.0f );
  }
  else if (w >= 645 && w <= 780.0f)
  {
    return float3 (1.0f, 0.0f, 0.0f);
  }
  else
    return float3 (0.0f, 0.0f, 0.0f);
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


float3 REC2020toREC709 (float3 RGB2020)
{
  static const float3x3 ConvMat =
  {
     1.660496, -0.587656, -0.072840,
    -0.124546,  1.132895,  0.008348,
    -0.018154, -0.100597,  1.118751
  };
  return mul (ConvMat, RGB2020);
}

float3 RemoveREC2084Curve (float3 N)
{
  float  m1 = 2610.0 / 4096.0 / 4;
  float  m2 = 2523.0 / 4096.0 * 128;
  float  c1 = 3424.0 / 4096.0;
  float  c2 = 2413.0 / 4096.0 * 32;
  float  c3 = 2392.0 / 4096.0 * 32;
  float3 Np = pow (N, 1 / m2);

  return
    pow (max (Np - c1, 0) / (c2 - c3 * Np), 1 / m1);
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

float4 main (PS_INPUT input) : SV_TARGET
{
  if (uiToneMapper == TONEMAP_COPYRESOURCE)
  {
    return
      float4 (
        texMainScene.Sample ( sampler0,
                                input.uv ).rgb, 1.0f
      );
  }

  bool bIsHDR10 = false;


  if (uiToneMapper == TONEMAP_HDR10_to_scRGB)
       bIsHDR10 = true;


  float4 hdr_color =
    float4 (
      max ( 0.0f,
        texMainScene.Sample ( sampler0,
                                input.uv ).rgb
                            ), 1.0f
           );


    float4 over_range =
      float4 (0.0f, 0.0f, 0.0f, 1.0f);

    if (hdr_color.r <  0.0 || hdr_color.r > 1.0)
        over_range.r = hdr_color.r;
    if (hdr_color.g <  0.0 || hdr_color.g > 1.0)
       over_range.g = hdr_color.g;
    if (hdr_color.b <  0.0 || hdr_color.b > 1.0)
       over_range.b = hdr_color.b;



  hdr_color.rgb = max ( 0.0f,
    bIsHDR10 ?
      REC2020toREC709 (RemoveREC2084Curve ( hdr_color.rgb ))
             :           SK_ProcessColor4 ( hdr_color.rgba,
                                            xRGB_to_Linear,
                             sdrIsImplicitlysRGB ).rgb
                      );

 

  if ( input.coverage.x < input.uv.x ||
       input.coverage.y < input.uv.y )
  {
    return hdr_color * 3.75;
  }


  if (visualFunc.x == VISUALIZE_HIGHLIGHTS)
  {
    if (length (over_range.rgb) > 0.0)
      return (normalize (over_range) * input.color.x);

    hdr_color.rgb =
      max (hdr_color.r, max (hdr_color.g, hdr_color.b));
       //Luminance ( hdr_color.rgb );
    input.color.x = hdrLuminance_MaxAvg / 80.0;
  }

  if (visualFunc.x >= VISUALIZE_8BIT_QUANTIZE &&
      visualFunc.x <= VISUALIZE_16BIT_QUANTIZE)
  {
    uint scale_i = visualFunc.x == VISUALIZE_8BIT_QUANTIZE  ?  255  :
                   visualFunc.x == VISUALIZE_10BIT_QUANTIZE ? 1023  :
                   visualFunc.x == VISUALIZE_12BIT_QUANTIZE ? 4095  :
                                                             65535;
    float scale_f = visualFunc.x == VISUALIZE_8BIT_QUANTIZE  ?  255.0f  :
                    visualFunc.x == VISUALIZE_10BIT_QUANTIZE ? 1023.0f  :
                    visualFunc.x == VISUALIZE_12BIT_QUANTIZE ? 4095.0f  :
                                                              65535.0f;
    uint3 rgb;

    rgb.r = hdr_color.r * scale_i;
    rgb.g = hdr_color.g * scale_i;
    rgb.b = hdr_color.b * scale_i;

    hdr_color.r = (float)rgb.r / scale_f;
    hdr_color.g = (float)rgb.g / scale_f;
    hdr_color.b = (float)rgb.b / scale_f;
  }


  if (pqBoostParams.x > 0.1f)
  {
    float
      pb_params [4] = {
        pqBoostParams.x,
        pqBoostParams.y,
        pqBoostParams.z,
        pqBoostParams.w
      };

    hdr_color.rgb =
      PQToLinear (
        LinearToPQ ( hdr_color.rgb, pb_params [0] ) *
                     pb_params [2], pb_params [1]
                 ) / pb_params [3];
  }


  if (uiToneMapper != TONEMAP_NONE)
  {
    if (uiToneMapper != TONEMAP_HDR10_to_scRGB)
    {
      hdr_color.rgb =
        ACESFitted (   hdr_color.rgb,
                     input.color.x,
                     input.color.y );
    }

    // Make the pass-through actualy pass-through.
    if (uiToneMapper == TONEMAP_HDR10_to_scRGB)
    {
      hdr_color.rgb  *= float3 (125.0, 125.0, 125.0);
      hdr_color.rgb   =
        Clamp_scRGB (hdr_color.rgb);

      if (input.color.y != 1.0)
      {
        float fLuma =
          length (hdr_color.rgb);

        // Clip to 0.35 nits, because lower than that produces garbage
        if (fLuma < 0.004375)
          hdr_color.rgb = 0;
      }

      input.color.y   = 1.0;
      input.color.x   = 1.0;
    }
  }

  else
  {
    hdr_color.rgb =
      SK_ProcessColor4 (hdr_color, uiToneMapper).rgb;
  }


  hdr_color.rgb =
    max ( 0.0, pow ( hdr_color.rgb,
                     input.color.yyy )
        );

  float fLuma =
    /*length (hdr_color.rgb);// */
    Luminance (hdr_color.rgb);

  float  length_color     =
    length (hdr_color.rgb);

  float3 normalized_color = length_color > 0.0 ?
    normalize (hdr_color.rgb)                  : float3 (0.0f, 0.0f, 0.0f);

  hdr_color.rgb += normalized_color * hdrLuminance_Min;

  hdr_color.rgb *= uiToneMapper != TONEMAP_HDR10_to_scRGB ?
    (                            hdrPaperWhite +
      fLuma * (input.color.xxx - hdrPaperWhite) )         :
                                 hdrPaperWhite;

  if (uiToneMapper == TONEMAP_NONE)
  {
    hdr_color.rgb = LinearToLogC (hdr_color.rgb);
    hdr_color.rgb = Contrast (    hdr_color.rgb, 0.18f * (0.1f * input.color.x / 0.0125f) / 100.0f, (sdrLuminance_NonStd / 0.0125f) / 100.0f);
    hdr_color.rgb = LogCToLinear (hdr_color.rgb);
  }

  hdr_color.rgb =
    Saturation (hdr_color.rgb, hdrSaturation);

  hdr_color.rgb -= normalized_color * hdrLuminance_Min;

  fLuma =
    Luminance (hdr_color.rgb);

  if (visualFunc.x >= VISUALIZE_REC709_GAMUT)
  {
    int cs = visualFunc.x - VISUALIZE_REC709_GAMUT;

    float3 r = SK_Color_xyY_from_RGB ( _ColorSpaces [cs], float3 (1.f, 0.f, 0.f) ),
           g = SK_Color_xyY_from_RGB ( _ColorSpaces [cs], float3 (0.f, 1.f, 0.f) ),
           b = SK_Color_xyY_from_RGB ( _ColorSpaces [cs], float3 (0.f, 0.f, 1.f) ),
           w = SK_Color_xyY_from_RGB ( _ColorSpaces [cs], float3 (D65,             hdrLuminance_MaxLocal / 80.0) );

    float3 vColor_xyY =
        SK_Color_xyY_from_RGB ( _ColorSpaces [cs], hdr_color.rgb / fLuma );

    float3 vTriangle [] = {
      r, g, b
    };

    // For time-based glow
    float fScale    = 1.0f;

    static const float3 vIdent3   = float3 ( 1.0f,    1.0f,    1.0f    );
    static const float3 vEpsilon3 = float3 ( FLT_MIN, FLT_MIN, FLT_MIN );

    float3 fDistField =
#if 0
      float3 (
        distance ( r, vColor_xyY ),
        distance ( g, vColor_xyY ),
        distance ( b, vColor_xyY )
      );

      fDistField.x = isnan (fDistField.x) ? 0 : fDistField.x;
      fDistField.y = isnan (fDistField.y) ? 0 : fDistField.y;
      fDistField.z = isnan (fDistField.z) ? 0 : fDistField.z;
#else
    sqrt ( max ( vEpsilon3, float3 (
      dot ( pow ( r - vColor_xyY, 2.0 ), vIdent3 ),
      dot ( pow ( g - vColor_xyY, 2.0 ), vIdent3 ),
      dot ( pow ( b - vColor_xyY, 2.0 ), vIdent3 )
         )     )                   );
#endif

    bool bContained =
      SK_Triangle_ContainsPoint (vColor_xyY, vTriangle) && vColor_xyY.x != vColor_xyY.y;

    if (bContained)
    {
#if 0
      // Visualize wider-than-Rec709
      if (cs > 0)
      {
        r = SK_Color_xyY_from_RGB ( _ColorSpaces [0], float3 (1.f, 0.f, 0.f) ),
        g = SK_Color_xyY_from_RGB ( _ColorSpaces [0], float3 (0.f, 1.f, 0.f) ),
        b = SK_Color_xyY_from_RGB ( _ColorSpaces [0], float3 (0.f, 0.f, 1.f) );

        fColorXYZ =
            SK_Color_xyY_from_RGB ( _ColorSpaces [0], hdr_color.rgb / input.color.xxx );

        float3 vTriangle2 [] = {
          r, g, b
        };

        bContained =
          SK_Triangle_ContainsPoint (fColorXYZ, vTriangle2);

        if (! bContained)
        {
          fScale = 0.0f;
        }
      }
#endif
    }

    float3 vDist =
      bContained ? (hdrLuminance_MaxAvg / 320.0) * Luminance (hdr_color.rgb) : fDistField;
                                                            //hdr_color.rgb;

#if 0
    // Gamut Overshoot
    if (fScale == 1.0f && (! bContained) && cs != 0)
    {
      float fTime =
        TWO_PI * (float)(currentTime % 3333) / 3333.3;

      vDist.r = sin (fTime);
      vDist.g = sin (fTime * 2.0f);
      vDist.b = cos (fTime);

      vDist *= ( hdrLuminance_MaxLocal / 80.0 );
    }
#endif

    return
      float4 (
        vDist, 1.0f
             );
  }

  if (visualFunc.x >= VISUALIZE_AVG_LUMA && visualFunc.x <= VISUALIZE_EXPOSURE)
  {
    //  400 peak, 101 ref ; 0.9709 gamma
    //  600 peak, 138 ref ; 0.9009 gamma
    //  800 peak, 172 ref ; 0.8621 gamma
    // 1000 peak, 203 ref ; 0.8333 gamma
    // 1500 peak, 276 ref ; 0.7874 gamma
    // 2000 peak, 343 ref ; 0.7519 gamma

    // HDR Reference White
    float _fSDRTarget = 276.0f;

    // User's mapped peak luminance
    float fUserTarget =
      ( input.color.x / 0.0125 );

    _fSDRTarget = fUserTarget / 18.0;

    if (fLuma * 80.0 <= _fSDRTarget)
    {
      return
        float4 (saturate ((fLuma * 80.0) / _fSDRTarget) * input.color.xxx, 1.0f);
    }

    float fDivisor = 0.0f;

    switch (visualFunc.x)
    {
      case VISUALIZE_AVG_LUMA:
        fDivisor = (  hdrLuminance_MaxAvg - _fSDRTarget) / 80.0; break;
      case VISUALIZE_LOCAL_LUMA:
        fDivisor = (hdrLuminance_MaxLocal - _fSDRTarget) / 80.0; break;
    };

    if (visualFunc.x == VISUALIZE_EXPOSURE)
    {
      if (input.color.x > 1.0)
        hdr_color.rgb =
          Clamp_scRGB (hdr_color.rgb);

      // find brightest component
      float lum   = max  (hdr_color.r, max (hdr_color.g, hdr_color.b));
	    float Scale = log2 (lum / ((_fSDRTarget / 80.0) * 0.18)) / 2.0f + 2.0f;

	    Scale = min (Scale, 7.0);
	    Scale = max (Scale, 0.0);

	    const float3 Colors[] =
	    {
	    	{ 0.0f, 0.0f, 0.0f },
	    	{ 0.0f, 0.0f, 1.0f },
	    	{ 0.0f, 1.0f, 1.0f },
	    	{ 0.0f, 1.0f, 0.0f },
	    	{ 1.0f, 1.0f, 0.0f },
	    	//{ 1.0f, 0.5f, 0.0f },
	    	{ 1.0f, 0.0f, 0.0f },
	    	{ 1.0f, 0.0f, 1.0f },
	    	{ 1.0f, 1.0f, 1.0f },
	    	{ 1.0f, 1.0f, 1.0f }
	    };
	    int index = int (Scale);


	    float4 result;

      result.rgb =
        lerp ( Colors [index    ],
               Colors [index + 1], Scale - index) *
         min (          Luminance (hdr_color.rgb) * 2.0F,
                                            0.15F * hdrLuminance_MaxAvg / 80.0 );
	    result.a = 1;

	    return result;
    }

    if (((fLuma - (_fSDRTarget / 80.0)) / fDivisor) >= 1.0f)
    {
      float fTime =
        TWO_PI * (float)(currentTime % 3333) / 3333.3;

      float4 vDist;

      vDist.r = sin (fTime);
      vDist.g = sin (fTime * 2.0f);
      vDist.b = cos (fTime);

      vDist *= ( hdrLuminance_MaxLocal / 80.0 );

      return float4 (vDist.rgb, 1.0f);
    }

    const float fLumaMaxAvg =
      hdrLuminance_MaxAvg   / 80.0;

    return
      float4 (
        colormap (
          (fLuma - (_fSDRTarget / 80.0)) / fDivisor).rgb * ( hdrLuminance_MaxAvg   / 80.0 ),
        1.0
      );

    return
      float4 (hdr_color.rgb, 1.0f);
  }


  return hdr_color;
}