#include "common_defs.hlsl"

RWTexture2D<float4> texBackbufferHDR;
RWTexture2D<float>  texLuminance;

[ numthreads ( HISTOGRAM_WORKGROUP_SIZE_X,
               HISTOGRAM_WORKGROUP_SIZE_Y, 1 ) ]
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
    texLuminance [ uint2 ( HISTOGRAM_WORKGROUP_SIZE_X,
                           HISTOGRAM_WORKGROUP_SIZE_Y ) ];

  const float fMax =
    hdrLuminance_MaxLocal / 80.0f;// hdrPaperWhite;

  uint subdiv =
    min ( HISTOGRAM_WORKGROUP_SIZE_Y,
          HISTOGRAM_WORKGROUP_SIZE_X );

  if ( avg > fMax / 2.333f )
  {
    if ( localIdx.x < subdiv &&
         localIdx.y < subdiv )
    {
      uint x_advance = (image_width  / subdiv) / HISTOGRAM_WORKGROUP_SIZE_X +
                       (image_width  / subdiv) % HISTOGRAM_WORKGROUP_SIZE_X;
      uint y_advance = (image_height / subdiv) / HISTOGRAM_WORKGROUP_SIZE_Y +
                       (image_height / subdiv) % HISTOGRAM_WORKGROUP_SIZE_Y;

      uint x_origin = localIdx.x * x_advance + groupIdx.x * (x_advance * subdiv);
      uint y_origin = localIdx.y * y_advance + groupIdx.y * (y_advance * subdiv);

      float fMax_over_avg =
        fMax / avg;

      for ( uint X = 0 ; X < x_advance ; ++X )
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





//RWBuffer responseCurveUAV;


Buffer<uint>   perTileHistogramSRV;
Buffer<uint>   mergedHistogramSRV;
RWBuffer<uint> perTileHistogramUAV;
RWBuffer<uint> mergedHistogramUAV;

cbuffer histogramDispatchParams : register (b0)
{
  float outputLuminanceMax;
  float outputLuminanceMin;

  float inputLuminanceMax;
  float inputLuminanceMin;
};

RWBuffer<uint> responseCurveUAV;

groupshared float frequencyPerBin        [NUM_HISTOGRAM_BINS];
groupshared float initialFrequencyPerBin [NUM_HISTOGRAM_BINS];

[numthreads(1, 1, 1)]
void
HistogramResponseCurve ( uint3 globalIdx : SV_DispatchThreadID,
                         uint3 localIdx  : SV_GroupThreadID,
                         uint3 groupIdx  : SV_GroupID )
{
  // Compute the initial frequency per-bin, and save it
  float T = 0.0f;

  for (uint bin = 0; bin < NUM_HISTOGRAM_BINS; ++bin)
  {
    float frequency =
      float (mergedHistogramSRV [bin]);

           frequencyPerBin [bin] = frequency;
    initialFrequencyPerBin [bin] = frequency;

    T += frequency;
  }

  // Naive histogram adjustment. There are many, many such histogram modification algorithms you may seek to employ - this is 
  // an example implementation that will no doubt later be changed
  // This is an implementation of page 14 of "A Visibility Matching Tone Reproduction Operator for High Dynamic Range Scenes"
  // There are other, better approaches, like Duan 2010: http://ima.ac.uk/papers/duan2010.pdf
  float rcpDisplayRange =
    1.0f / (log (outputLuminanceMax) - log (outputLuminanceMin));

  // Histogram bin step size - in log(cd/m2)
  float deltaB = // Luminance values are already log()d
    (inputLuminanceMax - inputLuminanceMin) /
         float (NUM_HISTOGRAM_BINS);

  float tolerance = T * 0.025f;
  float trimmings = 0.0f;

  uint loops = 0;

  do
  {
    // Work out the new histogram total
    T = 0.0f;

    for (uint bin2 = 0; bin2 < NUM_HISTOGRAM_BINS; ++bin2)
    {
      T += frequencyPerBin [bin2];
    }
    
    if (T < tolerance)
    {
      // This convergence is wrong - put it back to the original
      T = 0.0f;
      for (uint index = 0; index < NUM_HISTOGRAM_BINS; ++index)
      {
               frequencyPerBin [index] =
        initialFrequencyPerBin [index];
          T += frequencyPerBin [index];
      }
      break;
    }
    
    // Compute the ceiling
    trimmings = 0.0f;

    float ceiling =
      T * deltaB * rcpDisplayRange;

    for (uint bin3 = 0; bin3 < NUM_HISTOGRAM_BINS; ++bin3)
    {
      if (frequencyPerBin [bin3] > ceiling)
      {
        trimmings += frequencyPerBin [bin3] - ceiling;
                     frequencyPerBin [bin3] = ceiling;
      }
    }
    T -= trimmings;
    
    ++loops;
  }
  while ( trimmings > tolerance &&
              loops < 10 );

  // Compute the cumulative distribution function, per bin
  float rcpT = 1.0f / T;
  float sum  = 0.0f;

  for (uint bin5 = 0; bin5 < NUM_HISTOGRAM_BINS; ++bin5)
  {
    float probability =
      frequencyPerBin [bin5] * rcpT;
    
    sum += probability;
    
    responseCurveUAV [bin5] = sum;
  }
}
