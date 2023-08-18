#pragma warning ( disable : 3571 )

#include "HDR/common_defs.hlsl"

struct PS_INPUT
{
  float4 pos : SV_POSITION;
  float4 col : COLOR0;
  float2 uv  : TEXCOORD0;
  float2 uv2 : TEXCOORD1;
  float3 uv3 : TEXCOORD2;
};

cbuffer viewportDims : register (b0)
{
  float4 viewport;
};

sampler   sampler0    : register (s0);
Texture2D texture0    : register (t0);
//Texture2D hdrUnderlay : register (t1);
//Texture2D hdrHUD      : register (t2);

float4 main (PS_INPUT input) : SV_Target
{
  float4 out_col =
    texture0.Sample (sampler0, input.uv),
        orig_col =
         out_col;

  bool hdr10 = ( input.uv3.x < 0.0 );

  if (viewport.z > 0.f)
  {
    if ( input.uv2.x > 0.0f &&
         input.uv2.y > 0.0f )
    {
      out_col.rgb =
        PositivePow (
          RemoveSRGBCurve (out_col.rgb),
                input.uv2.yyy
            ) * input.uv2.xxx;
      out_col.a   = 1.0f;
    }

    else
    {
      out_col =
        float4 ( RemoveSRGBCurve (          input.col.rgb) *
                 RemoveSRGBCurve (            out_col.rgb),
                     PositivePow (saturate (  out_col.a) *
                                  saturate (input.col.a), 0.8)
               );
    }

    float hdr_scale  = hdr10 ? ( -input.uv3.x / 10000.0 )
                             :    input.uv3.x;

    float hdr_offset = 0.0f;//hdr10 ? 0.0f : input.uv3.z;

    hdr_scale -= hdr_offset;

    float4 hdr_out =
      float4 (   ( hdr10 ?
        LinearToST2084 (
          REC709toREC2020 (              saturate (out_col.rgb) ) * hdr_scale
                       ) :
     Clamp_scRGB_StripNaN ( expandGamut (saturate (out_col.rgb)   * hdr_scale, 0.12) )
                 )                                   + hdr_offset,
                                         saturate (out_col.a  ) );
        
    hdr_out.r *= (orig_col.r >= FLT_EPSILON);
    hdr_out.g *= (orig_col.g >= FLT_EPSILON);
    hdr_out.b *= (orig_col.b >= FLT_EPSILON);
    hdr_out.a *= (orig_col.a >= FLT_EPSILON);
        
    return hdr_out;
  }

  return
    ( input.col * out_col );
};