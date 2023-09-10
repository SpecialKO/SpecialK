#include "common_defs.hlsl"

//#define INCLUDE_SPLITTER
//#define INCLUDE_TEST_PATTERNS
//#define INCLUDE_VISUALIZATIONS
//#define INCLUDE_ACES
//#define INCLUDE_HDR10
//#define INCLUDE_NAN_MITIGATION
//#define DEBUG_NAN
//#define UTIL_STRIP_NAN

#pragma warning ( disable : 3570 )
#pragma warning ( disable : 3571 )
#pragma warning ( disable : 4000 )

struct PS_INPUT
{
  float4 pos      : SV_POSITION;
  float4 color    : COLOR0;
  float2 coverage : COLOR1;
  float2 uv       : TEXCOORD0;
};

sampler   sampler0      : register (s0);
Texture2D texMainScene  : register (t0);
Texture2D texLastFrame0 : register (t1);

#define VISUALIZE_NONE           0
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
#define VISUALIZE_GRAYSCALE      11
#define VISUALIZE_MAX_LOCAL_CLIP 12
#define VISUALIZE_MAX_AVG_CLIP   13
#define VISUALIZE_MIN_AVG_CLIP   14


#define TONEMAP_NONE                  0
#define TONEMAP_ACES_FILMIC           1
#define TONEMAP_HDR10_to_scRGB        2
#define TONEMAP_HDR10_to_scRGB_FILMIC 3
#define TONEMAP_COPYRESOURCE          255
#define TONEMAP_REMOVE_INVALID_COLORS 256


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
#ifdef INCLUDE_NAN_MITIGATION
  color = float4
    ( (! IsNan (color.r)) * (! IsInf (color.r)) * color.r,
      (! IsNan (color.g)) * (! IsInf (color.g)) * color.g,
      (! IsNan (color.b)) * (! IsInf (color.b)) * color.b,
      (! IsNan (color.a)) * (! IsInf (color.a)) * color.a );
#endif

  float4 out_color =
    float4 (
      (strip_srgb && func != sRGB_to_Linear) ?
                 RemoveSRGBCurve (color.rgb) : color.rgb,
                                               color.a
    );

  // Straight Pass-Through
  if (func <= xRGB_to_Linear)
#ifndef INCLUDE_NAN_MITIGATION
    return out_color;
#else
    return     AnyIsNan (out_color) ?
    float4 (0.0f, 0.0f, 0.0f, 0.0f) : out_color;
#endif
                                
  return
    float4 (0.0f, 0.0f, 0.0f, 0.0f);
}

#ifdef INCLUDE_NAN_MITIGATION
float4 getNonNanSample (float4 color, float2 uv)
{
  uint2                        tex_dims;
  texMainScene.GetDimensions ( tex_dims.x,
                               tex_dims.y );

  const float2
    uv_offset [] = {
      float2 ( 0.0f,               1.0f / tex_dims.y),
      float2 ( 1.0f / tex_dims.x,  1.0f / tex_dims.y),
      float2 ( 0.0f,              -1.0f / tex_dims.y),
      float2 (-1.0f / tex_dims.x,  1.0f / tex_dims.y),
      float2 (-1.0f / tex_dims.x, -1.0f / tex_dims.y),
      float2 (-1.0f / tex_dims.x,               0.0f),
      float2 ( 1.0f / tex_dims.x, -1.0f / tex_dims.y),
      float2 ( 1.0f / tex_dims.x,               0.0f)
    };

  color =
    texLastFrame0.Sample ( sampler0, uv );

  if ( (! AnyIsNan   (color)) )
    return color;

  for ( uint i = 0 ; i < 8 ; ++i )
  {
    if ( AnyIsNan    (color) )
      color =
        texMainScene.Sample  ( sampler0, uv + uv_offset [i] );

    if ( AnyIsNan    (color) )
      color =
        texLastFrame0.Sample ( sampler0, uv + uv_offset [i] );

    if ( (! AnyIsNan (color)) )
      return color;
  }

  return
    float4 (0.0f, 0.0f, 0.0f, 0.0f);
}
#endif

float4 main (PS_INPUT input) : SV_TARGET
{
  switch (uiToneMapper)
  {
    case TONEMAP_COPYRESOURCE:
    {
      float4 ret =
        texMainScene.Sample ( sampler0,
                                input.uv );

#ifdef INCLUDE_NAN_MITIGATION
      // A UNORM RenderTarget would return this instead of NaN,
      //   and the game is expecting UNORM render targets :)
      return
        float4 ( (! IsNan (ret.r)) * (! IsInf (ret.r)) * ret.r,
                 (! IsNan (ret.g)) * (! IsInf (ret.g)) * ret.g,
                 (! IsNan (ret.b)) * (! IsInf (ret.b)) * ret.b,
                 (! IsNan (ret.a)) * (! IsInf (ret.a)) * ret.a );
#endif
    } break;

#ifdef UTIL_STRIP_NAN
    case TONEMAP_REMOVE_INVALID_COLORS:
    {
      float4 color =
        texMainScene.Sample ( sampler0,
                                input.uv );

      return
        float4 ( (! IsNan (color.r)) * (! IsInf (color.r)) * color.r,
                 (! IsNan (color.g)) * (! IsInf (color.g)) * color.g,
                 (! IsNan (color.b)) * (! IsInf (color.b)) * color.b,
                 (! IsNan (color.a)) * (! IsInf (color.a)) * color.a );
    } break;
#endif
  }

#ifdef INCLUDE_HDR10
  bool bIsHDR10 = false;

  if (uiToneMapper == TONEMAP_HDR10_to_scRGB)
       bIsHDR10 = true;
#endif

  float4 hdr_color =
    texMainScene.Sample ( sampler0,
                            input.uv );

  float4 orig_color = hdr_color;

#ifdef INCLUDE_NAN_MITIGATION
#ifdef DEBUG_NAN
  if ( AnyIsNan      (hdr_color)/*||
       AnyIsNegative (hdr_color) */)
  {
    hdr_color.rgba =
      getNonNanSample ( hdr_color, input.uv );

    if ( AnyIsNan      (hdr_color) ||
         AnyIsNegative (hdr_color) )
    {
      float fTime =
        TWO_PI * (float)(currentTime % 3333) / 3333.3;

      float4 vDist =
        float4 ( sin (fTime),
                 sin (fTime * 2.0f),
                 cos (fTime), 1.0f );

      vDist.rgb *=
        ( hdrLuminance_MaxLocal / 80.0 );

      return
        vDist;
    }
  }
#else
  // If the input were strictly SDR, we could eliminate NaN
  //   using saturate (...), but we want to keep potential
  //     HDR input pixels.
  if ( AnyIsNan (hdr_color) )
    hdr_color =
      getNonNanSample (hdr_color, input.uv);

  hdr_color =
    clamp (hdr_color, 0.0, 125.0);
#endif
#endif


 #ifdef INCLUDE_VISUALIZATIONS
  if (visualFunc.x == VISUALIZE_GRAYSCALE)
  {
    hdr_color =
      input.uv.x * float4 (1.0f, 1.0f, 1.0f, 1.0f);
  }

  float4 over_range =
    float4 ( 0.0f, 0.0f,
             0.0f, 1.0f );

  if (hdr_color.r <  0.0 || hdr_color.r > 1.0)
     over_range.r = hdr_color.r;
  if (hdr_color.g <  0.0 || hdr_color.g > 1.0)
     over_range.g = hdr_color.g;
  if (hdr_color.b <  0.0 || hdr_color.b > 1.0)
     over_range.b = hdr_color.b;
#endif


  hdr_color.rgb = Clamp_scRGB (
#ifdef INCLUDE_HDR10
    bIsHDR10 ?
      REC2020toREC709 (RemoveREC2084Curve ( hdr_color.rgb )) :
#endif
                         SK_ProcessColor4 ( hdr_color.rgba,
                                            xRGB_to_Linear,
                             sdrIsImplicitlysRGB ).rgb
                      );

  ///hdr_color.rgb =
  ///  sRGB_to_ACES (hdr_color.rgb);
 
#ifdef INCLUDE_SPLITTER
  if ( input.coverage.x < input.uv.x ||
       input.coverage.y < input.uv.y )
  {
    return
      float4 (hdr_color.rgb * 3.75, 1.0f);
  }
#endif


#ifdef INCLUDE_VISUALIZATIONS
  if (visualFunc.x == VISUALIZE_HIGHLIGHTS)
  {
    if (length (over_range.rgb) > 0.0)
      return (float4 (float3 (normalize (over_range.rgb) * input.color.x), 1.0f));

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
#endif
  
#ifdef INCLUDE_HDR10
  if (uiToneMapper != TONEMAP_HDR10_to_scRGB)
#endif
  {
    if ( input.color.x < 0.0125f - FLT_EPSILON ||
         input.color.x > 0.0125f + FLT_EPSILON )
    {
      hdr_color.rgb = clamp (LinearToLogC (hdr_color.rgb), 0.0, 125.0);
      hdr_color.rgb =        Contrast     (hdr_color.rgb,
              0.18f * (0.1f * input.color.x / 0.0125f) / 100.0f,
                       (sdrLuminance_NonStd / 0.0125f) / 100.0f);
      hdr_color.rgb = clamp (LogCToLinear (hdr_color.rgb), 0.0, 125.0);
    }
  }

#if defined (INCLUDE_ACES) || defined (INCLUDE_HDR10)
  if (uiToneMapper != TONEMAP_NONE)
#ifndef INCLUDE_ACES
  if (uiToneMapper == TONEMAP_HDR10_to_scRGB)
#endif
  {
#ifdef INCLUDE_ACES
    if (uiToneMapper != TONEMAP_HDR10_to_scRGB)
    {
      hdr_color.rgb =
        //TM_Stanard (hdr_color.rgb);
         RemoveSRGBCurve (
           ACESFitted (hdr_color.rgb,
                     input.color.x,
                     input.color.y ));
    }
#endif

#ifdef INCLUDE_HDR10
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
#endif
  }
  else
#endif
  {
    hdr_color =
      SK_ProcessColor4 (hdr_color, uiToneMapper);
  }
    
#ifdef INCLUDE_HDR10
  if (uiToneMapper != TONEMAP_HDR10_to_scRGB)
#endif
  {
    hdr_color.rgb =
      PositivePow ( hdr_color.rgb,
                  input.color.yyy );
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

    float3 new_color =
      PQToLinear (
        LinearToPQ ( hdr_color.rgb, pb_params [0]) *
                     pb_params [2], pb_params [1]
                 ) / pb_params [3];

#ifdef INCLUDE_NAN_MITIGATION
    if (! AnyIsNan (  new_color))
#endif
      hdr_color.rgb = new_color;
  }
    
#ifdef INCLUDE_HDR10
  if (uiToneMapper != TONEMAP_HDR10_to_scRGB)
#endif
  {
    if ( hdrSaturation >= 1.0f + FLT_EPSILON ||
         hdrSaturation <= 1.0f - FLT_EPSILON || uiToneMapper == TONEMAP_ACES_FILMIC )
    {
      float saturation =
        hdrSaturation + 0.05 * ( uiToneMapper == TONEMAP_ACES_FILMIC );

      hdr_color.rgb =
        Saturation ( hdr_color.rgb,
                     saturation );
    }
  }
    
#if 0
  float3 vNormalColor =
    normalize (hdr_color.rgb);

  float fLuma =
    max (Luminance (vNormalColor), 0.0f);

  hdr_color.rgb *=
#ifdef INCLUDE_HDR10
    uiToneMapper != TONEMAP_HDR10_to_scRGB ?
#endif
    (                          //hdrPaperWhite +
      fLuma * (input.color.xxx/*-hdrPaperWhite*/))
#ifdef INCLUDE_HDR10
                        :        hdrPaperWhite
#endif
    ;
#else
  hdr_color.rgb *=
    input.color.xxx;

  float fLuma = 0.0f;
#endif

#ifdef INCLUDE_VISUALIZATIONS
  if (visualFunc.x != VISUALIZE_NONE) // Expand Gamut before visualization
  {
    if (hdrGamutExpansion > 0.0f)
    {
      hdr_color.rgb =
        expandGamut (hdr_color.rgb, hdrGamutExpansion);
    }
  }

  fLuma =
    max (Luminance (hdr_color.rgb), 0.0);

  if ( visualFunc.x >= VISUALIZE_REC709_GAMUT &&
       visualFunc.x <  VISUALIZE_GRAYSCALE )
  {
    int cs = visualFunc.x - VISUALIZE_REC709_GAMUT;

    float3 r = SK_Color_xyY_from_RGB ( _ColorSpaces [cs], float3 (1.f, 0.f, 0.f) ),
           g = SK_Color_xyY_from_RGB ( _ColorSpaces [cs], float3 (0.f, 1.f, 0.f) ),
           b = SK_Color_xyY_from_RGB ( _ColorSpaces [cs], float3 (0.f, 0.f, 1.f) ),
           w = SK_Color_xyY_from_RGB ( _ColorSpaces [cs], float3 (D65,             hdrLuminance_MaxLocal / 80.0) );

    float3 vColor_xyY =
         SK_Color_xyY_from_RGB ( _ColorSpaces [cs], normalize (hdr_color.rgb) );

    float3 vTriangle [] = {
      r, g, b
    };

    // For time-based glow
    float fScale = 1.0f;

    float3 fDistField =
      float3 (
        distance ( r, vColor_xyY ),
        distance ( g, vColor_xyY ),
        distance ( b, vColor_xyY )
      );

    fDistField.x = IsNan (fDistField.x) ? 0 : fDistField.x;
    fDistField.y = IsNan (fDistField.y) ? 0 : fDistField.y;
    fDistField.z = IsNan (fDistField.z) ? 0 : fDistField.z;

    bool bContained =
      SK_Triangle_ContainsPoint ( vColor_xyY,
        vTriangle ) &&            vColor_xyY.x !=
                                  vColor_xyY.y;

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
          SK_Color_xyY_from_RGB ( _ColorSpaces [0], hdr_color.rgb );

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
      float4 (vDist, 1.0f);
  }
#endif

#ifdef INCLUDE_TEST_PATTERNS
  if (visualFunc.x == VISUALIZE_GRAYSCALE)
  {
    float3 vColor =
      float3 (0.0f, 0.0f, 0.0f);

    if ( Luminance (hdr_color.rgb) / (hdrLuminance_MaxLocal / 80.0f) >= (1.0f - input.uv.y) - 0.0125 &&
         Luminance (hdr_color.rgb) / (hdrLuminance_MaxLocal / 80.0f) <= (1.0f - input.uv.y) + 0.0125 )
    {
      vColor.rgb =
        ( hdrLuminance_MaxAvg / 80.0f );
    }

    if (! all (vColor))
    {
      hdr_color.rgb =
        float3 ( min (hdrLuminance_MaxLocal / 80.0f, hdr_color.r),
                 min (hdrLuminance_MaxLocal / 80.0f, hdr_color.g),
                 min (hdrLuminance_MaxLocal / 80.0f, hdr_color.b) );
    }

    else
      hdr_color.rgb = vColor.rgb;
  }

  if (visualFunc.x == VISUALIZE_MAX_LOCAL_CLIP)
  { 
    float  scale = 8.0f;
    float2 uv3   = frac (1.0f * (scale * input.uv) + 0.5f) - 0.5f;
    float      t = 0.01f;
    
    // thickness thick line
    float d3 = 3.0 * scale * t;
    
    // background
    hdr_color.rgb =
      float3 (125.0f, 125.0f, 125.0f);
    
    if ( (input.uv.x > 0.333 && input.uv.x < 0.666) &&
         (input.uv.y > 0.333 && input.uv.y < 0.666) )
    {
      if (abs (uv3.x) < d3)
        hdr_color.rgb = float3 (input.color.x, input.color.x, input.color.x);
      else
        hdr_color.rgb = float3 (125.0, 125.0, 125.0);
    }
  }

  if (visualFunc.x == VISUALIZE_MAX_AVG_CLIP)
  {
    float2 texDims;

    texMainScene.GetDimensions (
           texDims.x,
           texDims.y
    );

    float2     uv = input.uv;
    float2  scale =   float2 ( texDims.x / 10.0,
                               texDims.y / 10.0 );
                         
    float2 size = texDims.xy / scale;
    float total =
        floor (uv.x * size.x) +
        floor (uv.y * size.y);
               
    bool isEven =
      fmod (total, 2.0f) == 0.0f;
    
    float4 color1 = float4 (input.color.x, input.color.x, input.color.x, 1.0);
    float4 color2 = float4 (125.0,         125.0,                 125.0, 1.0);
    
    hdr_color   =
         isEven ?
         color1 : color2;
  }

  if (visualFunc.x == VISUALIZE_MIN_AVG_CLIP)
  {
    hdr_color =
      float4 (0.0, 0.0, 0.0, 1.0);

    float2 uv = input.uv;
    
    if ( (uv.x > 0.45 && uv.x < 0.55) &&
         (uv.y > 0.45 && uv.y < 0.55) )
    {
        if (uv.x > 0.46 && uv.x < 0.54 &&
            uv.y > 0.46 && uv.y < 0.54)
        {
            if ( (uv.x > 0.495 && uv.x < 0.505) ||
                 (uv.y > 0.495 && uv.y < 0.505) )
              hdr_color = float4 (125.0, 125.0, 125.0, 1.0);
            else
              hdr_color =
                float4 ( input.color.x, input.color.x,
                         input.color.x, 1.0 );
        }
        else
          hdr_color =
            float4 (125.0, 125.0, 125.0, 1.0);
    }
  }
#endif

#ifdef INCLUDE_VISUALIZATIONS
  if ( visualFunc.x >= VISUALIZE_AVG_LUMA &&
       visualFunc.x <= VISUALIZE_EXPOSURE )
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

    if (visualFunc.x != VISUALIZE_EXPOSURE)
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

      float4 result;

#if 0
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
        { 1.0f, 0.2f, 0.0f },
        { 1.0f, 0.0f, 0.0f },
        { 1.0f, 0.0f, 1.0f },
        { 1.0f, 1.0f, 1.0f }
      };
      int index = int (Scale);

      result.rgb =
        lerp ( Colors [index    ],
               Colors [index + 1], Scale - index) *
         min (          Luminance (hdr_color.rgb) * 2.0F,
                                            0.15F * hdrLuminance_MaxAvg / 80.0 );
#else
// Taken from https://github.com/microsoft/Windows-universal-samples/blob/main/Samples/D2DAdvancedColorImages/cpp/D2DAdvancedColorImages/LuminanceHeatmapEffect.hlsl in order to match HDR + WCG Image Viewer's Heatmap
// 
// Define constants based on above behavior: 9 "stops" for a piecewise linear gradient in scRGB space.
#define STOP0_NITS 0.00f
#define STOP1_NITS 3.16f
#define STOP2_NITS 10.0f
#define STOP3_NITS 31.6f
#define STOP4_NITS 100.f
#define STOP5_NITS 316.f
#define STOP6_NITS 1000.f
#define STOP7_NITS 3160.f
#define STOP8_NITS 10000.f

#define STOP0_COLOR float4(0.0f, 0.0f, 0.0f, 1.0f) // Black
#define STOP1_COLOR float4(0.0f, 0.0f, 1.0f, 1.0f) // Blue
#define STOP2_COLOR float4(0.0f, 1.0f, 1.0f, 1.0f) // Cyan
#define STOP3_COLOR float4(0.0f, 1.0f, 0.0f, 1.0f) // Green
#define STOP4_COLOR float4(1.0f, 1.0f, 0.0f, 1.0f) // Yellow
#define STOP5_COLOR float4(1.0f, 0.2f, 0.0f, 1.0f) // Orange
// Orange isn't a simple combination of primary colors but allows us to have 8 gradient segments,
// which gives us cleaner definitions for the nits --> color mappings.
#define STOP6_COLOR float4(1.0f, 0.0f, 0.0f, 1.0f) // Red
#define STOP7_COLOR float4(1.0f, 0.0f, 1.0f, 1.0f) // Magenta
#define STOP8_COLOR float4(1.0f, 1.0f, 1.0f, 1.0f) // White

      // 1: Calculate luminance in nits.
      // Input is in scRGB. First convert to Y from CIEXYZ, then scale by whitepoint of 80 nits.
      float nits = dot(float3(0.2126f, 0.7152f, 0.0722f), hdr_color.rgb) * 80.0f;

      // 2: Determine which gradient segment will be used.
      // Only one of useSegmentN will be 1 (true) for a given nits value.
      float useSegment0 = sign(nits - STOP0_NITS) - sign(nits - STOP1_NITS);
      float useSegment1 = sign(nits - STOP1_NITS) - sign(nits - STOP2_NITS);
      float useSegment2 = sign(nits - STOP2_NITS) - sign(nits - STOP3_NITS);
      float useSegment3 = sign(nits - STOP3_NITS) - sign(nits - STOP4_NITS);
      float useSegment4 = sign(nits - STOP4_NITS) - sign(nits - STOP5_NITS);
      float useSegment5 = sign(nits - STOP5_NITS) - sign(nits - STOP6_NITS);
      float useSegment6 = sign(nits - STOP6_NITS) - sign(nits - STOP7_NITS);
      float useSegment7 = sign(nits - STOP7_NITS) - sign(nits - STOP8_NITS);

      // 3: Calculate the interpolated color.
      float lerpSegment0 = (nits - STOP0_NITS) / (STOP1_NITS - STOP0_NITS);
      float lerpSegment1 = (nits - STOP1_NITS) / (STOP2_NITS - STOP1_NITS);
      float lerpSegment2 = (nits - STOP2_NITS) / (STOP3_NITS - STOP2_NITS);
      float lerpSegment3 = (nits - STOP3_NITS) / (STOP4_NITS - STOP3_NITS);
      float lerpSegment4 = (nits - STOP4_NITS) / (STOP5_NITS - STOP4_NITS);
      float lerpSegment5 = (nits - STOP5_NITS) / (STOP6_NITS - STOP5_NITS);
      float lerpSegment6 = (nits - STOP6_NITS) / (STOP7_NITS - STOP6_NITS);
      float lerpSegment7 = (nits - STOP7_NITS) / (STOP8_NITS - STOP7_NITS);

      //  Only the "active" gradient segment contributes to the output color.
      result =
          lerp(STOP0_COLOR, STOP1_COLOR, lerpSegment0) * useSegment0 +
          lerp(STOP1_COLOR, STOP2_COLOR, lerpSegment1) * useSegment1 +
          lerp(STOP2_COLOR, STOP3_COLOR, lerpSegment2) * useSegment2 +
          lerp(STOP3_COLOR, STOP4_COLOR, lerpSegment3) * useSegment3 +
          lerp(STOP4_COLOR, STOP5_COLOR, lerpSegment4) * useSegment4 +
          lerp(STOP5_COLOR, STOP6_COLOR, lerpSegment5) * useSegment5 +
          lerp(STOP6_COLOR, STOP7_COLOR, lerpSegment6) * useSegment6 +
          lerp(STOP7_COLOR, STOP8_COLOR, lerpSegment7) * useSegment7;
#endif
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
#endif
    
  float4 color_out;

  // Extra clipping and gamut expansion logic for regular display output
#ifdef INCLUDE_TEST_PATTERNS
  if (visualFunc.x == VISUALIZE_NONE)
#endif
  {
    if (hdrGamutExpansion > 0.0f)
    {
      color_out.rgb =
        expandGamut (hdr_color.rgb, hdrGamutExpansion);
    }

    
    color_out =
      float4 (
        Clamp_scRGB_StripNaN (color_out.rgb),
                    saturate (hdr_color.a)
             );

    color_out.r *= (orig_color.r >= FLT_EPSILON);
    color_out.g *= (orig_color.g >= FLT_EPSILON);
    color_out.b *= (orig_color.b >= FLT_EPSILON);
  }
#ifdef INCLUDE_TEST_PATTERNS
  else
  {
    color_out =
      float4 (
        Clamp_scRGB (hdr_color.rgb),
           saturate (hdr_color.a)
             );
  }
#endif
  
  // HDR10 -Output-, transform scRGB to HDR10
  if (visualFunc.y == 1)
  {
    color_out.rgb =
      LinearToPQ (REC709toREC2020 (color_out.rgb), 125.0f);
  }

  return color_out;
}