#pragma warning ( disable : 3571 )
cbuffer colorSpaceTransform : register (b0)
{
  uint4  visualFunc;
  uint   uiColorSpaceIn;
  uint   uiColorSpaceOut;
  bool   bUseSMPTE2084;
  float  sdrLuminance_NonStd;
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
Texture2D texHUD       : register (t1);

float3
RemoveSRGBCurve (float3 x)
{
	return  ( x < 0.04045f ) ?
            x / 12.92f     :
     pow ( (x + 0.055f) / 1.055f, 2.4f );
}

float
RemoveSRGBAlpha (float a)
{
  return  ( a < 0.04045f ) ?
            a / 12.92f     :
    pow ( ( a + 0.055f   ) / 1.055f,
                              2.4f );
}

float3 HUEtoRGB(in float H)
{
  float R = abs(H * 6 - 3) - 1;
  float G = 2 - abs(H * 6 - 2);
  float B = 2 - abs(H * 6 - 4);
  return saturate(float3(R,G,B));
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

static const
float3x3 sRGB_2_AP1 =
{
  0.61319, 0.33951, 0.04737,
  0.07021, 0.91634, 0.01345,
  0.02062, 0.10957, 0.86961
};

static const
float3x3 AP0_2_sRGB =
{
   2.52169, -1.13413, -0.38756,
  -0.27648,  1.37272, -0.09624,
  -0.01538, -0.15298,  1.16835,
};

static const
float3x3 AP1_2_sRGB =
{
   1.70505, -0.62179, -0.08326,
  -0.13026,  1.14080, -0.01055,
  -0.02400, -0.12897,  1.15297,
};

static const
float3x3 AP0_2_AP1_MAT =
{
   1.4514393161, -0.2365107469, -0.2149285693,
  -0.0765537734,  1.1762296998, -0.0996759264,
   0.0083161484, -0.0060324498,  0.9977163014
};

static const
float3x3 AP1_2_XYZ_MAT =
{
   0.6624541811, 0.1340042065, 0.1561876870,
   0.2722287168, 0.6740817658, 0.0536895174,
  -0.0055746495, 0.0040607335, 1.0103391003
};

static const
float3x3 XYZ_2_REC709_MAT =
{
   3.2409699419, -1.5373831776, -0.4986107603,
  -0.9692436363,  1.8759675015,  0.0415550574,
   0.0556300797, -0.2039769589,  1.0569715142
};

static const
float3x3 XYZ_2_REC2020_MAT =
{
   1.7166511880, -0.3556707838, -0.2533662814,
  -0.6666843518,  1.6164812366,  0.0157685458,
   0.0176398574, -0.0427706133,  0.9421031212
};

static const
float3x3 XYZ_2_DCIP3_MAT =
{
   2.7253940305, -1.0180030062, -0.4401631952,
  -0.7951680258,  1.6897320548,  0.0226471906,
   0.0412418914, -0.0876390192,  1.1009293786
};

static const
float3 AP1_RGB2Y = float3 (0.272229, 0.674082, 0.0536895);

float3
sRGBtoXYZ (float3 color)
{
  return
    mul ( AP1_2_XYZ_MAT,
            mul (sRGB_2_AP1, color) );
}

float3
sRGBtoDCIP3 (float3 color)
{
  return
    mul (XYZ_2_DCIP3_MAT, sRGBtoXYZ (color));
}

float3
sRGBtoRec2020 (float3 color)
{
  return
    mul (XYZ_2_REC2020_MAT, sRGBtoXYZ (color));
}

#define sRGB_to_Linear           0
#define sRGB_to_scRGB_as_DCIP3   1
#define sRGB_to_scRGB_as_Rec2020 2
#define sRGB_to_scRGB_as_XYZ     3

float4
SK_ProcessColor4 ( float4 color,
                      int func,
                      int strip_srgb = 1 )
{
  float4 out_color =
    float4 (
      (strip_srgb && func != sRGB_to_Linear) ? RemoveSRGBCurve (color.rgb) : color.rgb,
      color.a
    );

  switch (func)
  {
    // Straight Pass-Through
    case sRGB_to_Linear:
    default:
    {
      return out_color;
    }


    // DCI-P3
    case sRGB_to_scRGB_as_DCIP3:
    {
      return
        float4 ( sRGBtoDCIP3 (out_color.rgb),
                              out_color.a  );
    }


    // Rec2020
    case sRGB_to_scRGB_as_Rec2020:
    {
      return
        float4 ( sRGBtoRec2020 (out_color.rgb),
                                out_color.a  );
    }

    // XYZ
    case sRGB_to_scRGB_as_XYZ:
    {
      return
        float4 ( sRGBtoXYZ (out_color.rgb),
                            out_color.a  );
    }
  }
}

float4 main ( PS_INPUT input ) : SV_TARGET
{
  if ( input.coverage.x < input.uv.x ||
       input.coverage.y < input.uv.y )
  {
    return
      SK_ProcessColor4 (
        texMainScene.Sample ( sampler0,
                                input.uv ),
        uiColorSpaceIn
      ) * sdrLuminance_NonStd;
  }


  float4 hdr_color =
    texMainScene.Sample (sampler0, input.uv);

  hdr_color =
    SK_ProcessColor4 (hdr_color, uiColorSpaceIn);

  hdr_color =
    pow (
     abs (hdr_color), input.color.yyyw
        )           * input.color.xxxw;

  hdr_color =
    SK_ProcessColor4 (hdr_color, uiColorSpaceOut, 0);


  if (visualFunc.x > 0)
  {
    float len =
      length (hdr_color.rgb);

    if (len <= 1.0f)
    {
      return
        float4 (len*12.6875f, len*12.6875f, len*12.6875f, 1.0f);
    }

    else if (len <= 6.4f)
    {
      return
        float4 (
          HUEtoRGB (
            len / 6.4f
          ), 1.0
        );
    }

    else if (len <= 12.7f)
    {
      return
        float4 (
          float3 (len*len, len*len, len*len) *
            HUEtoRGB (
              len / 12.7f
            ),
          1.0f
        );
    }
  }

  return hdr_color;
}