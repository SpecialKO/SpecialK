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

[numthreads (WORKGROUP_SIZE_X, WORKGROUP_SIZE_Y, 1)]
void
LuminanceResponse ( uint3 globalIdx : SV_DispatchThreadID,
                    uint3 localIdx  : SV_GroupThreadID,
                    uint3 groupIdx  : SV_GroupID )
{
  uint image_width  = 0;
  uint image_height = 0;

  texBackbufferHDR.GetDimensions ( image_width,
                                   image_height );

  float avg =
    texLuminance [uint2 (WORKGROUP_SIZE_X, WORKGROUP_SIZE_Y)];

  const float fMax =
    hdrLuminance_MaxAvg / 80.0f;// hdrPaperWhite;

  uint subdiv =
    min ( WORKGROUP_SIZE_Y,
          WORKGROUP_SIZE_X );

  if ( avg > fMax )
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

      for ( uint X = 0 ; X < x_advance ; ++X )
      for ( uint Y = 0 ; Y < y_advance ; ++Y )
      {
        uint2 pos =
          uint2 (x_origin + X, y_origin + Y);

        if ( pos.x < image_width &&
             pos.y < image_height )
        {
          texBackbufferHDR [pos].rgba *=
            float4 ( fMax / avg,
                     fMax / avg,
                     fMax / avg, 1.0 );
        }
      }
    }
  }
}