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
  float  rtv_type;
  float  padding;
  float2 font_dims;
};

sampler   sampler0    : register (s0);
Texture2D texture0    : register (t0);
//Texture2D hdrUnderlay : register (t1);
//Texture2D hdrHUD      : register (t2);

float4 main (PS_INPUT input) : SV_Target
{
  float4 out_col =
    texture0.Sample (sampler0, input.uv);
 
  // Font Width/Height is only set on Font passes...
  if (dot (font_dims, 1.0f) > 0.0f)
  {
    // Font is a single-channel alpha texture,
    //   supply 1.0 for rgb channels
    out_col.rgb = 1.0;
  }

  float4 orig_col =
     abs (out_col);

  float  ui_alpha = saturate (input.col.a   * out_col.a );
  float3 ui_color =           input.col.rgb * out_col.rgb;

  //
  // HDR (HDR10 or scRGB)
  //
  if (viewport.z > 0.f)
  {
    bool hdr10 = (input.uv3.x < 0.0);

    float hdr_scale = hdr10 ? (-input.uv3.x) / 80.0f
                              : input.uv3.x;

    if ( input.uv2.x > 0.0f &&
         input.uv2.y > 0.0f )
    {
      out_col.rgb =  sign (out_col.rgb) *
        pow (
          RemoveSRGBCurve (out_col.rgb),
                input.uv2.yyy
            ) * input.uv2.xxx;
      out_col.a   = 1.0f;
    }

    else
    {
      out_col =
        float4 (        RemoveGammaExp (                  ui_color,  2.2f ),
            1.0f - RemoveAlphaGammaExp ( max (0.0, 1.0f - ui_alpha), 2.2f ) );

      out_col.rgb =
        expandGamut (out_col.rgb * hdr_scale, 0.333f) / hdr_scale;
    }

    float hdr_offset = 0.0f;//hdr10 ? 0.0f : input.uv3.z;
                            //hdr_scale -= hdr_offset;

    float4 hdr_out =
      hdr10 ? float4 (  LinearToPQ (
                   REC709toREC2020 ( out_col.rgb * (ui_alpha / 2.0f) * (hdr_scale * 2.0f) ), 100.0f ),
                        LinearToPQ ( out_col.a               * 8.66f,                        125.0f ) )
            : float4 ( Clamp_scRGB ( out_col.rgb             * hdr_scale ),
                                     out_col.a );

    // Keep pure black pixels as-per scRGB's limited ability to
    //   represent a black pixel w/ FP16 precision
    hdr_out.rgb *=
      (dot (orig_col.rgb, 1.0f) > FP16_MIN);

    hdr_out.a *= (orig_col.a > FP16_MIN);

    float alpha_mul =
      (hdr10 ? 1.0
             : ui_alpha ); // Use linear alpha in scRGB

    if (hdr10)
    {
      hdr_out.rgba =
        saturate (hdr_out.rgba);

      hdr_out.rgb =
        min (LinearToPQ ((hdr_scale).xxx, 100.0f) / hdr_out.a, hdr_out.rgb) * hdr_out.a;
    }

    return
      float4 ( hdr_out.rgb * alpha_mul,
               hdr_out.a );
  }

  //
  // SDR (sRGB/Rec 709) -- We use a linear view for consistency with HDR blending
  //
  if (rtv_type.x != 0.0f) // sRGB View
  {
    out_col = float4 ( RemoveSRGBCurve (       ui_color) * (1.0f - RemoveSRGBAlpha (1.0f - ui_alpha)),
                1.0f - RemoveSRGBAlpha (1.0f - ui_alpha) );
  }
  else
  {
    out_col = float4 (ui_color * ui_alpha,
                                 ui_alpha);
  }

  return out_col;
};