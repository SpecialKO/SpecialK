#pragma warning ( disable : 3571 )
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

float3 ApplyREC2084Curve (float3 L, float maxLuminance)
{
	float m1 = 2610.0 / 4096.0 / 4;
	float m2 = 2523.0 / 4096.0 * 128;
	float c1 = 3424.0 / 4096.0;
	float c2 = 2413.0 / 4096.0 * 32;
	float c3 = 2392.0 / 4096.0 * 32;

	// L = FD / 10000, so if FD == 10000, then L = 1.
	// so to scale max luminance, we want to multiply by maxLuminance / 10000


  float maxLuminanceScale = maxLuminance / 10000.0f; 
  L  *= maxLuminanceScale;

	float3 Lp = pow(L, m1);

	return pow((c1 + c2 * Lp) / (1 + c3 * Lp), m2);
}



float3 REC709toREC2020(float3 RGB709)
{
	static const float3x3 ConvMat =
	{
		0.627402, 0.329292, 0.043306,
		0.069095, 0.919544, 0.011360,
		0.016394, 0.088028, 0.895578
	};
	return mul (ConvMat, RGB709);
}

float3
RemoveSRGBCurve (float3 x)
{
	// Approximately pow (x, 2.2);
  //
  //  * We can do better
  //
  //   { Time to go Piecewise! }
	return  ( x < 0.04045f ) ? 
            x / 12.92f     :
     pow ( (x + 0.055f) / 1.055f, 2.4f );
}

//float3 hsv2rgb (float3 c)
//{
//  float4 K = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
//  float3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
//  return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
//}

float Luma (float3 color)
{
  return
    dot (color, float3 (0.299f, 0.587f, 0.114f));
}

float3 HUEtoRGB(in float H)
{
  float R = abs(H * 6 - 3) - 1;
  float G = 2 - abs(H * 6 - 2);
  float B = 2 - abs(H * 6 - 4);
  return saturate(float3(R,G,B));
}

float3 HSVtoRGB(in float3 HSV)
{
  float3 RGB = HUEtoRGB(HSV.x);
  return ((RGB - 1) * HSV.y + 1) * HSV.z;
}

float3 HSLtoRGB(in float3 HSL)
{
  float3 RGB = HUEtoRGB(HSL.x);
  float C = (1 - abs(2 * HSL.z - 1)) * HSL.y;
  return (RGB - 0.5) * C + HSL.z; 
}

float3
ApplyREC709Curve (float3 x)
{
	return ( x < 0.0181f ) ?
    4.5f * x : 1.0993f * pow (x, 0.45f) - 0.0993f;
}

float4 main ( PS_INPUT input ) : SV_TARGET
{
  float4 main_color =
    texMainScene.Sample (sampler0, input.uv);

  if (input.coverage.x < input.uv.x ||
      input.coverage.y < input.uv.y)
  {
    return
      float4 (main_color.rgb, 1.0f);
  }

  main_color.rgb =
    ApplyREC2084Curve (main_color.rgb, input.color.x);


#if 0
  float len =
    length (main_color.rgb);

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
          float3 (len/6.4f, len/6.4f, len/6.4f)
        ), 1.0
      );
  }

  else if (len <= 12.7f)
  {
    return
      float4 (
        float3 (len*len, len*len, len*len) * 
          HUEtoRGB (
            float3 (len/12.7f, len/12.7f, len/12.7f)
          ),
        1.0f
      );
  }
#endif

  return main_color;
}