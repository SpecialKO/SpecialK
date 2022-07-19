#include "common_defs.hlsl"

//#define INCLUDE_SPLITTER
//#define INCLUDE_TEST_PATTERNS
//#define INCLUDE_VISUALIZATIONS
//#define INCLUDE_ACES
//#define INCLUDE_HDR10
#define INCLUDE_NAN_MITIGATION
#define DEBUG_NAN
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
  if (AnyIsNan      (color) ||
      AnyIsNegative (color))
                color =
    float4 (0.0f, 0.0f, 0.0f, 0.0f);
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

  if ( (! AnyIsNan      (color)) &&
       (! AnyIsNegative (color)) )
    return color;

  for ( uint i = 0 ; i < 8 ; ++i )
  {
    if ( AnyIsNan      (color) ||
         AnyIsNegative (color) )
      color =
        texMainScene.Sample  ( sampler0, uv + uv_offset [i] );

    if ( AnyIsNan      (color) ||
         AnyIsNegative (color) )
      color =
        texLastFrame0.Sample ( sampler0, uv + uv_offset [i] );

    if ( (! AnyIsNan      (color)) &&
         (! AnyIsNegative (color)) )
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
      if (AnyIsNan (ret.rgba) || AnyIsNegative (ret.rgba))
      {
        return
          float4 (0.0f, 0.0f, 0.0f, 0.0f);
      }
#endif

      return ret;
    } break;

#ifdef UTIL_STRIP_NAN
    case TONEMAP_REMOVE_INVALID_COLORS:
    {
      float4 color =
        texMainScene.Sample ( sampler0,
                                input.uv );

      if (AnyIsNan (color))
        return float4 (0.0f, 0.0f, 0.0f, 0.0f);

      if (color.r < 0.0f) color.r = 0.0f;
      if (color.g < 0.0f) color.g = 0.0f;
      if (color.b < 0.0f) color.b = 0.0f;
      if (color.a < 0.0f) color.a = 0.0f;

      return
        color;
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

#ifdef INCLUDE_NAN_MITIGATION
#ifdef DEBUG_NAN
  if ( AnyIsNan      (hdr_color) ||
       AnyIsNegative (hdr_color) )
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
  if ( AnyIsNan (hdr_color) )
    hdr_color =
      getNonNanSample (hdr_color, input.uv);

  if (hdr_color.r < 0.0f||
      hdr_color.g < 0.0f||
      hdr_color.b < 0.0f||
      hdr_color.a < 0.0f )
  {if(hdr_color.r < 0.0f )
      hdr_color.r = 0.0f;
   if(hdr_color.g < 0.0f )
      hdr_color.g = 0.0f;
   if(hdr_color.b < 0.0f )
      hdr_color.b = 0.0f;
   if(hdr_color.a < 0.0f )
      hdr_color.a = 0.0f;}

 if (! any (hdr_color))
   return float4 (0.0f, 0.0f, 0.0f, 0.0f);
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


  hdr_color.rgb = max ( 0.0f,
#ifdef INCLUDE_HDR10
    bIsHDR10 ?
      REC2020toREC709 (RemoveREC2084Curve ( hdr_color.rgb )) :
#endif
                         SK_ProcessColor4 ( hdr_color.rgba,
                                            xRGB_to_Linear,
                             sdrIsImplicitlysRGB ).rgb
                      );

 
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
  
  if ( input.color.x < 0.0125f - FLT_MIN ||
       input.color.x > 0.0125f + FLT_MIN )
  {
    hdr_color.rgb = LinearToLogC (hdr_color.rgb);
    hdr_color.rgb = Contrast     (hdr_color.rgb,
            0.18f * (0.1f * input.color.x / 0.0125f) / 100.0f,
                     (sdrLuminance_NonStd / 0.0125f) / 100.0f);
    hdr_color.rgb = LogCToLinear (hdr_color.rgb);
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

  hdr_color.rgb =
    PositivePow ( hdr_color.rgb,
                input.color.yyy );

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
        LinearToPQ ( hdr_color.rgb, pb_params [0] ) *
                     pb_params [2], pb_params [1]
                 ) / pb_params [3];

#ifdef INCLUDE_NAN_MITIGATION
    if (! AnyIsNan (  new_color))
#endif
      hdr_color.rgb = new_color;
  }

  if ( hdrSaturation >= 1.0f + FLT_MIN ||
       hdrSaturation <= 1.0f - FLT_MIN || uiToneMapper == TONEMAP_ACES_FILMIC )
  {
    float saturation =
      hdrSaturation + 0.05 * ( uiToneMapper == TONEMAP_ACES_FILMIC );

    // sRGB primaries <--> ACEScg  (* not sRGB gamma)
    hdr_color.rgb =
      ACEScg_to_sRGB (
        Saturation (
          sRGB_to_ACEScg (hdr_color.rgb),
            saturation
        )
      );
  }

  float3 vNormalColor =
    normalize (hdr_color.rgb);

  float fLuma =
    max (Luminance (vNormalColor), 0.0f);

  hdr_color.rgb *=
#ifdef INCLUDE_HDR10
    uiToneMapper != TONEMAP_HDR10_to_scRGB ?
#endif
    (                            hdrPaperWhite +
      fLuma * (input.color.xxx - hdrPaperWhite) )
#ifdef INCLUDE_HDR10
                        :        hdrPaperWhite
#endif
    ;

#ifdef INCLUDE_VISUALIZATIONS
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
      dot ( PositivePow ( (r - vColor_xyY), 2.0 ), vIdent3 ),
      dot ( PositivePow ( (g - vColor_xyY), 2.0 ), vIdent3 ),
      dot ( PositivePow ( (b - vColor_xyY), 2.0 ), vIdent3 )
         )     )                   );
#endif

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
#endif

  return
    float4 (
      Clamp_scRGB (hdr_color.rgb),
         saturate (hdr_color.a)
           );
}