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
                   int    func,
                   int    strip_eotf = 1 )
{
#ifdef INCLUDE_NAN_MITIGATION
  color = SanitizeFP (color);
#endif

  // This looks weird because power-law EOTF does not work as intended on negative colors, and
  //   we may very well have negative colors on WCG SDR input.
  //
  //   sRGB was amended to support negative colors, and the correct behavior is to use the
  //     absolute value of the input color and then apply the original sign to the result.
  float eotf = sdrContentEOTF;

  float4 out_color =
    float4 (
      ( strip_eotf && func != sRGB_to_Linear ) ?
                                 eotf != -2.2f ? sign (color.rgb) * pow (            abs (color.rgb),
                                 eotf) :         sign (color.rgb) * RemoveSRGBCurve (abs (color.rgb)) :
                                                       color.rgb,
                                                       color.a );

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

float4
FinalOutput (float4 vColor)
{
  // HDR10 -Output-, transform scRGB to HDR10
  if (visualFunc.y == 1)
  {
    vColor.rgb =
      saturate ( // PQ is not defined for negative inputs
        LinearToPQ (max (0.0, REC709toREC2020 (vColor.rgb)), 125.0f)
      );

    vColor.a = 1.0;
  }

  return vColor;
}

float4
main (PS_INPUT input) : SV_TARGET
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
      ret = SanitizeFP (ret);
#endif
        return ret;
      }
      break;

#ifdef UTIL_STRIP_NAN
    case TONEMAP_REMOVE_INVALID_COLORS:
    {
      float4 color =
        texMainScene.Sample ( sampler0,
                                input.uv );

      color = SanitizeFP (color);

      return color;
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

  float3 orig_color =
    abs (hdr_color.rgb);

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
        FinalOutput (vDist);
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
    clamp (hdr_color, 0.0, float_MAX);
#endif
#endif


#ifdef INCLUDE_VISUALIZATIONS
  if (visualFunc.x == VISUALIZE_GRAYSCALE)
  {
    hdr_color =
      input.uv.xxxx;
  }

  float3 hdr_rgb2 =
    hdr_color.rgb * 2.0f;

  float4 over_range =
    float4 (   hdr_color.rgb,      1.0f ) *
    float4 ( ( hdr_color.rgb > hdr_rgb2 ) +
             (           2.0 < hdr_rgb2 ), 1.0f );

  over_range.a =
    any (over_range.rgb);
#endif


  float3 hdr_color_abs = abs (hdr_color.rgb);
  float  max_rgb_comp  =
    max (1.0, max (hdr_color_abs.r, max (hdr_color_abs.g, hdr_color_abs.b)));

  // Normalize things so that values > 1.0 are not fed into de-gamma.
  //
  //   Games that have gamma are SDR and should never have had values > 1.0
  //     in the first place!
  //
  hdr_color.rgb /= max_rgb_comp;

  hdr_color.rgb =
#ifdef INCLUDE_HDR10
    bIsHDR10 ?
      REC2020toREC709 (RemoveREC2084Curve ( hdr_color.rgb )) :
#endif
                 SK_ProcessColor4 ( hdr_color.rgba,
                                    xRGB_to_Linear,
                      sdrContentEOTF != 1.0f).rgb;

  hdr_color.rgb *= max_rgb_comp;


#ifdef INCLUDE_HDR10
  if (! bIsHDR10)
  {
#endif
    // Clamp scRGB source image to Rec 709, unless using passthrough mode
    if (input.color.x != 1.0)
    {
      hdr_color.rgb =
        Clamp_scRGB (hdr_color.rgb);
    }
#ifdef INCLUDE_HDR10
  }
#endif


  if (overbrightColorFlags != 0x0 && ( any (hdr_color > 1.0f) ||
                                       any (hdr_color < 0.0f)) )
  {
#if 1
    float3 xyz_color =
      Rec709_to_XYZ (hdr_color.rgb);

    if (xyz_color.y > 1.0f)
    {
#if 0
      float3 vNormalizedColor =
        hdr_color.rgb / xyz_color.y;

      hdr_color.rgb =                    vNormalizedColor +
        NeutralTonemap ((hdr_color.rgb - vNormalizedColor) * xyz_color.y) / 2.5f;
#else
      // Soft Clamp?
      //// Rescale the final output, not the input pixel
      //input.color.x /= xyz_color.y;

      // Hard Clamp (Perceptual Boost will not boost beyond target)
      hdr_color.rgb = (hdr_color.rgb / xyz_color.y);
#endif
    }
#else
    float3 hdr_color_abs = abs (hdr_color.rgb);
    float  max_rgb_comp  =
      max (hdr_color_abs.r, max (hdr_color_abs.g, hdr_color_abs.b));

    hdr_color.rgb /= max_rgb_comp;
#endif
  }


#ifdef INCLUDE_SPLITTER
  if (any (input.coverage < input.uv))
  {
    return // Clamp to [0,1] range as the game would have in SDR
      FinalOutput (float4 (saturate (hdr_color.rgb) * sdrLuminance_White, 1.0f));
  }
#endif


#ifdef INCLUDE_VISUALIZATIONS
  if (visualFunc.x == VISUALIZE_HIGHLIGHTS)
  {
    if (length (over_range.rgb) > 0.0)
      return FinalOutput (float4 (float3 (normalize (over_range.rgb) * input.color.x), 1.0f));

    hdr_color.rgb =
      max (hdr_color.r, max (hdr_color.g, hdr_color.b));
       //Luminance ( hdr_color.rgb );
    input.color.x = hdrLuminance_MaxAvg / 80.0;
  }

  // These visualizations do not support negative values, thus
  //   we clamp to 0.0... though lose wide gamut color in the process.
  if (visualFunc.x >= VISUALIZE_8BIT_QUANTIZE &&
      visualFunc.x <= VISUALIZE_16BIT_QUANTIZE)
  {
    hdr_color.rgb =
      clamp (hdr_color.rgb, 0.0f, FLT_MAX);

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
    //
    // Middle-Gray Contrast (has not worked as intended in years, should be removed!)
    //
    if ( sdrLuminance_NonStd != 1.25f &&
         (input.color.x < 0.0125f - FLT_EPSILON ||
          input.color.x > 0.0125f + FLT_EPSILON) )
    {
      hdr_color.rgb = LinearToLogC (hdr_color.rgb);
      hdr_color.rgb = Contrast     (hdr_color.rgb,
                  0.18f * (0.1f * input.color.x / 0.0125f) / 100.0f,
                           (sdrLuminance_NonStd / 0.0125f) / 100.0f);
      hdr_color.rgb = LogCToLinear (hdr_color.rgb);
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
        min (hdr_color.rgb, float_MAX);

      if (input.color.y != 1.0)
      {
        float fLuma =
          dot (hdr_color.rgb, FastSign (hdr_color.rgb) * float3 (0.2126729, 0.7151522, 0.0721750));

        // Clip to 0.35 nits, because lower than that produces garbage
        if (fLuma < 0.004375)
          hdr_color.rgb = 0;
      }

      input.color.xy = 1.0;
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
    if (input.color.y != 1.0f)
    {
      hdr_color.rgb = sign (hdr_color.rgb) *
                  pow (abs (hdr_color.rgb),
                          input.color.yyy);
    }
  }

  if (pqBoostParams.x > 0.1f)
  {
    const float pb_param_0 = pqBoostParams.x;
    const float pb_param_1 = pqBoostParams.y;
    const float pb_param_2 = pqBoostParams.z;
    const float pb_param_3 = pqBoostParams.w;

    float3 hdr_color_xyz =
      Rec709_to_XYZ (hdr_color.rgb);
    
    float fLuma   =
      max (hdr_color_xyz.y, 0.0f);
    hdr_color.rgb =
      max (hdr_color_xyz  , 0.0f);

    float4 new_color  = 0.0f;
    float3 new_color0 = 0.0f,
           new_color1 = 0.0f;

    if (fLuma > 0.0f)
    {
      if (colorBoost == 0.0 || colorBoost == 1.0)
      {
        // Optimized for no color boost
        if (colorBoost == 0.0)
        {
          new_color0 =
            ( PQToLinear (
                LinearToPQ ( fLuma,
                             pb_param_0 ) *
                             pb_param_2, pb_param_1
                         ) / pb_param_3 ).xxx;
        }

        // Optimized for full color boost
        if (colorBoost == 1.0)
        {
          new_color1 =
            PQToLinear (
              LinearToPQ ( hdr_color.rgb,
                           pb_param_0 ) *
                           pb_param_2, pb_param_1
                       ) / pb_param_3;
        }

        new_color.rgb =
          lerp (hdr_color.rgb * new_color0 / fLuma,
                                new_color1, colorBoost);
      }

      // Optimization for both at the same time.
      else
      {
        new_color =
           PQToLinear(
              LinearToPQ(float4(hdr_color.rgb, fLuma),
                           pb_param_0) *
                           pb_param_2, pb_param_1
                       ) / pb_param_3;

        new_color.rgb =
          lerp(hdr_color.rgb * new_color.www / fLuma,
                                new_color.rgb, colorBoost);
      }

#ifdef INCLUDE_NAN_MITIGATION
      if (! AnyIsNan (  new_color))
#endif
        hdr_color.rgb = new_color.rgb;
      
      hdr_color.rgb = XYZ_to_Rec709 (hdr_color.rgb);
    }
  }

#ifdef INCLUDE_HDR10
  if (uiToneMapper != TONEMAP_HDR10_to_scRGB)
#endif
  {
    if ( hdrSaturation >= 1.0f + FLT_EPSILON ||
         hdrSaturation <= 1.0f - FLT_EPSILON || uiToneMapper == TONEMAP_ACES_FILMIC )
    {
      float saturation =
         hdrSaturation + 0.05 * (uiToneMapper == TONEMAP_ACES_FILMIC);

      hdr_color.rgb =
        Saturation ( hdr_color.rgb,
                     saturation );
    }
  }

  hdr_color.rgb *=
    input.color.xxx;

  float fLuma = 0.0f;

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
    max (Rec709_to_XYZ (hdr_color.rgb).y, 0.0f);

  if ( visualFunc.x >= VISUALIZE_REC709_GAMUT &&
       visualFunc.x <  VISUALIZE_GRAYSCALE )
  {
    int cs = visualFunc.x - VISUALIZE_REC709_GAMUT;

    float3 r = float3(_ColorSpaces[cs].xr, _ColorSpaces[cs].yr, 0),
           g = float3(_ColorSpaces[cs].xg, _ColorSpaces[cs].yg, 0),
           b = float3(_ColorSpaces[cs].xb, _ColorSpaces[cs].yb, 0);

    // Same clamp color_output receives
    hdr_color =
      float4 (
        Clamp_scRGB (hdr_color.rgb),
           saturate (hdr_color.a)
             );

    float3 xy_from_sRGB;
    {
      float3 XYZ = sRGBtoXYZ ( hdr_color.rgb );
      xy_from_sRGB = float3 (
        XYZ.x / (XYZ.x + XYZ.y + XYZ.z),
        XYZ.y / (XYZ.x + XYZ.y + XYZ.z),
        0
      );
    }

    float3 vTriangle [] = {
      r, g, b
    };

    // For time-based glow
    float fScale = 1.0f;

    float3 fDistField =
      float3 (
        distance ( r, xy_from_sRGB ),
        distance ( g, xy_from_sRGB ),
        distance ( b, xy_from_sRGB )
      );

    fDistField.x = IsNan (fDistField.x) ? 0 : fDistField.x;
    fDistField.y = IsNan (fDistField.y) ? 0 : fDistField.y;
    fDistField.z = IsNan (fDistField.z) ? 0 : fDistField.z;

    float3 xy_to_analyze = xy_from_sRGB;

    // is HDR_10BitSwap?
    if (visualFunc.y == 1)
    {
      float4 analyze_color = FinalOutput(hdr_color);

      // roll back transfer functions for HDR10 bit swap so we can analyze
      float3 XYZ = sRGBtoXYZ (REC2020toREC709 (PQToLinear (analyze_color.rgb, 125.0f)));
      xy_to_analyze = float3 (
        XYZ.x / (XYZ.x + XYZ.y + XYZ.z),
        XYZ.y / (XYZ.x + XYZ.y + XYZ.z),
        0
      );
    }

    bool bContained =
      SK_Triangle_ContainsPoint ( xy_to_analyze,
        vTriangle ) &&            xy_to_analyze.x !=
                                  xy_to_analyze.y;

    float3 vDist =
      bContained ? (hdrLuminance_MaxAvg / 320.0) * Luminance (hdr_color.rgb) : fDistField;

    return
      FinalOutput (float4 (vDist, 1.0f));
  }
#endif

#ifdef INCLUDE_TEST_PATTERNS
  if (visualFunc.x == VISUALIZE_GRAYSCALE)
  {
    float3 vColor =
      float3 (0.0f, 0.0f, 0.0f);

    float fLuma =
      max (Rec709_to_XYZ (hdr_color.rgb).y, 0.0f);

    if ( fLuma / (hdrLuminance_MaxLocal / 80.0f) >= (1.0f - input.uv.y) - 0.0125 &&
         fLuma / (hdrLuminance_MaxLocal / 80.0f) <= (1.0f - input.uv.y) + 0.0125 )
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
        FinalOutput (float4 (saturate ((fLuma * 80.0) / _fSDRTarget) * input.color.xxx, 1.0f));
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

#define STOP0_COLOR float4 (0.0f, 0.0f, 0.0f, 1.0f) // Black
#define STOP1_COLOR float4 (0.0f, 0.0f, 1.0f, 1.0f) // Blue
#define STOP2_COLOR float4 (0.0f, 1.0f, 1.0f, 1.0f) // Cyan
#define STOP3_COLOR float4 (0.0f, 1.0f, 0.0f, 1.0f) // Green
#define STOP4_COLOR float4 (1.0f, 1.0f, 0.0f, 1.0f) // Yellow
#define STOP5_COLOR float4 (1.0f, 0.2f, 0.0f, 1.0f) // Orange
// Orange isn't a simple combination of primary colors but allows us to have 8 gradient segments,
// which gives us cleaner definitions for the nits --> color mappings.
#define STOP6_COLOR float4 (1.0f, 0.0f, 0.0f, 1.0f) // Red
#define STOP7_COLOR float4 (1.0f, 0.0f, 1.0f, 1.0f) // Magenta
#define STOP8_COLOR float4 (1.0f, 1.0f, 1.0f, 1.0f) // White

      // 1: Calculate luminance in nits.
      // Input is in scRGB. First convert to Y from CIEXYZ, then scale by whitepoint of 80 nits.
      float nits = max (Rec709_to_XYZ (hdr_color.rgb).y, 0.0) * 80.0f;

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
        lerp (STOP0_COLOR, STOP1_COLOR, lerpSegment0) * useSegment0 +
        lerp (STOP1_COLOR, STOP2_COLOR, lerpSegment1) * useSegment1 +
        lerp (STOP2_COLOR, STOP3_COLOR, lerpSegment2) * useSegment2 +
        lerp (STOP3_COLOR, STOP4_COLOR, lerpSegment3) * useSegment3 +
        lerp (STOP4_COLOR, STOP5_COLOR, lerpSegment4) * useSegment4 +
        lerp (STOP5_COLOR, STOP6_COLOR, lerpSegment5) * useSegment5 +
        lerp (STOP6_COLOR, STOP7_COLOR, lerpSegment6) * useSegment6 +
        lerp (STOP7_COLOR, STOP8_COLOR, lerpSegment7) * useSegment7;
      result.a = 1;

      return
        FinalOutput (result);
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

      return FinalOutput (float4 (vDist.rgb, 1.0f));
    }

    const float fLumaMaxAvg =
      hdrLuminance_MaxAvg   / 80.0;

    return FinalOutput (
      float4 (
        colormap (
          (fLuma - (_fSDRTarget / 80.0)) / fDivisor).rgb * ( hdrLuminance_MaxAvg   / 80.0 ),
        1.0
      )
    );

    return
      FinalOutput (float4 (hdr_color.rgb, 1.0f));
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
    else
      color_out.rgb = hdr_color.rgb;

    color_out =
      float4 (
        Clamp_scRGB (color_out.rgb),
           saturate (hdr_color.a)
             );

    // Keep pure black pixels as-per scRGB's limited ability to
    //   represent a black pixel w/ FP16 precision
    color_out.rgb *=
      (dot (orig_color.rgb, 1.0f) > FP16_MIN);
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

  return
    FinalOutput (color_out);
}
