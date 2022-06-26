#define WORKGROUP_SIZE_X ( 32 )
#define WORKGROUP_SIZE_Y ( 16 )

RWTexture2D<float4> texBackbufferHDR;
RWTexture2D<float>  texLuminance;

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

#define FLT_EPSILON     1.192092896e-07 // Smallest positive number, such that 1.0 + FLT_EPSILON != 1.0

float PositivePow (float base, float power)
{
  return
    pow ( max (abs (base), float (FLT_EPSILON)), power );
}

float3 PositivePow (float3 base, float3 power)
{
  return
    pow (max (abs (base), float3 ( FLT_EPSILON, FLT_EPSILON,
                                   FLT_EPSILON )), power );
}

float3 LinearToST2084 (float3 normalizedLinearValue)
{
  return
    PositivePow (
      (0.8359375f + 18.8515625f * PositivePow (abs (normalizedLinearValue), 0.1593017578f)) /
            (1.0f + 18.6875f    * PositivePow (abs (normalizedLinearValue), 0.1593017578f)), 78.84375f
        );
}

float3 ST2084ToLinear (float3 ST2084)
{
  return
    PositivePow ( max (
      PositivePow ( ST2084, 1.0f / 78.84375f) - 0.8359375f, 0.0f) / (18.8515625f - 18.6875f *
      PositivePow ( ST2084, 1.0f / 78.84375f)),
                            1.0f / 0.1593017578f
        );
}

[numthreads (WORKGROUP_SIZE_X, WORKGROUP_SIZE_Y, 1)]
void
LuminanceResponse ( uint3 globalIdx : SV_DispatchThreadID,
                    uint3 localIdx  : SV_GroupThreadID,
                    uint3 groupIdx  : SV_GroupID )
{
  uint                             image_width  = 0;
  uint                             image_height = 0;
  texBackbufferHDR.GetDimensions ( image_width,
                                   image_height );

  float avg =
    texLuminance [ uint2 ( WORKGROUP_SIZE_X,
                           WORKGROUP_SIZE_Y ) ];

  const float fMax =
    hdrLuminance_MaxLocal / 80.0f;// hdrPaperWhite;

  uint subdiv =
    min ( WORKGROUP_SIZE_Y,
          WORKGROUP_SIZE_X );

  if ( avg > fMax / 2.333f )
  {
    if ( localIdx.x < subdiv &&
         localIdx.y < subdiv )
    {
      uint x_advance = (image_width  / subdiv) / WORKGROUP_SIZE_X +
                       (image_width  / subdiv) % WORKGROUP_SIZE_X;
      uint y_advance = (image_height / subdiv) / WORKGROUP_SIZE_Y +
                       (image_height / subdiv) % WORKGROUP_SIZE_Y;

      uint x_origin = localIdx.x * x_advance + groupIdx.x * (x_advance * subdiv);
      uint y_origin = localIdx.y * y_advance + groupIdx.y * (y_advance * subdiv);

      float fMax_over_avg =
        fMax / avg;
                                               //[unroll (8)]
      for ( uint X = 0 ; X < x_advance ; ++X ) //[unroll (4)]
      for ( uint Y = 0 ; Y < y_advance ; ++Y ) 
      {
        uint2 pos =
          uint2 ( x_origin + X,
                  y_origin + Y );

        if ( pos.x < image_width &&
             pos.y < image_height )
        {
          float3 color =
            ST2084ToLinear (
              LinearToST2084 (texBackbufferHDR [pos].rgb / avg)
                           ) * (fMax / 2.333f);

          texBackbufferHDR [pos].rgba =
            float4 ( color.r,
                     color.g,
                     color.b, 1.0 );
        }
      }
    }
  }
}