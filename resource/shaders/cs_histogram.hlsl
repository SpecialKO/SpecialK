#define NUM_HISTOGRAM_BINS 1024

cbuffer histogramDispatch
{
  uint  imageWidth;
  uint  imageHeight;

  float minLuminance;
  float maxLuminance;

  uint  numLocalZones;

  float3x3 colorspaceMat_RGBtoxyY;
};

float transformRGBtoY (float3 rgb)
{
  return mul (rgb, colorspaceMat_RGBtoxyY).z;
}

float transformRGBtoLogY (float3 rgb)
{
  return log (transformRGBtoY (rgb));
}

uint computeBucketIdxFromRGB (float3 rgb)
{
  float logY =
    transformRGBtoLogY (rgb);

  float luminance = 1.0f;

  return uint (
    float (NUM_HISTOGRAM_BINS) *
      saturate ( (luminance    - minLuminance) /
                 (maxLuminance - minLuminance)  )
  );
}

//groupshared uint localHistogram [NUM_HISTOGRAM_BINS];
//
//  Buffer localHistogramIn;
//RWBuffer localHistogramOut;
//
//  Buffer globalHistogramIn;
//RWBuffer globalHistogramOut;