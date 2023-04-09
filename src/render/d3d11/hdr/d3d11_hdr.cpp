/**
 * This file is part of Special K.
 *
 * Special K is free software : you can redistribute it
 * and/or modify it under the terms of the GNU General Public License
 * as published by The Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * Special K is distributed in the hope that it will be useful,
 *
 * But WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Special K.
 *
 *   If not, see <http://www.gnu.org/licenses/>.
 *
**/

#include <SpecialK/stdafx.h>

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"D3D 11 HDR"

#include <SpecialK/render/dxgi/dxgi_hdr.h>
#include <SpecialK/render/dxgi/dxgi_util.h>
#include <SpecialK/render/d3d11/d3d11_core.h>
#include <SpecialK/render/d3d11/d3d11_state_tracker.h>

#include <shaders/uber_hdr_shader_ps.h>
#include <shaders/basic_hdr_shader_ps.h>
#include <shaders/vs_colorutil.h>
#include <shaders/cs_histogram.h>
#include <shaders/hdr_merge_cs.h>
#include <shaders/hdr_response_cs.h>

bool __SK_HDR_v2_0 = false;

struct SK_HDR_FIXUP
{
  static std::string
      _SpecialNowhere;

  ID3D11Buffer*             mainSceneCBuffer = nullptr;
  ID3D11Buffer*                   hudCBuffer = nullptr;
  ID3D11Buffer*            colorSpaceCBuffer = nullptr;

  ID3D11InputLayout*            pInputLayout = nullptr;

  ID3D11SamplerState*              pSampler0 = nullptr;

  ID3D11ShaderResourceView*         pMainSrv = nullptr;
  ID3D11ShaderResourceView*         pCopySrv = nullptr;
  ID3D11ShaderResourceView*          pHUDSrv = nullptr;

  ID3D11RenderTargetView*           pMainRtv = nullptr;
  ID3D11RenderTargetView*            pHUDRtv = nullptr;

  ID3D11RasterizerState*    pRasterizerState = nullptr;
  ID3D11DepthStencilState*          pDSState = nullptr;

  ID3D11BlendState*              pBlendState = nullptr;

  ID3D11Texture2D*             pLuminanceTex = nullptr;
  ID3D11UnorderedAccessView*   pLuminanceUAV = nullptr;
  ID3D11ShaderResourceView*    pLuminanceSRV = nullptr;

  ID3D11Texture2D*             pGamutTex     = nullptr;
  ID3D11UnorderedAccessView*   pGamutUAV     = nullptr;
  ID3D11ShaderResourceView*    pGamutSRV     = nullptr;

  enum SK_HDR_Type {
    None        = 0x000ul,
    HDR10       = 0x010ul,
    HDR10Plus   = 0x011ul,
    DolbyVision = 0x020ul,

    scRGB       = 0x100ul, // Not a real signal / data standard,
                           //   but a real useful colorspace even so.
  } __SK_HDR_Type = None;

  DXGI_FORMAT
  SK_HDR_GetPreferredDXGIFormat (DXGI_FORMAT fmt_in)
  {
    if (__SK_HDR_10BitSwap)
      __SK_HDR_Type = HDR10;
    else if (__SK_HDR_16BitSwap)
      __SK_HDR_Type = scRGB;

    else
    {
      if (fmt_in == DXGI_FORMAT_R10G10B10A2_UNORM)
        __SK_HDR_Type = HDR10;
      else if (fmt_in == DXGI_FORMAT_R16G16B16A16_FLOAT)
        __SK_HDR_Type = scRGB;
    }

    if ((__SK_HDR_Type & scRGB) != 0)
      return DXGI_FORMAT_R16G16B16A16_FLOAT;

    if ((__SK_HDR_Type & HDR10) != 0)
      return DXGI_FORMAT_R10G10B10A2_UNORM;

    SK_LOG0 ( ( L"Unknown HDR Format, using R10G10B10A2 (HDR10-ish)" ),
                L"HDR Inject" );

    return
      DXGI_FORMAT_R10G10B10A2_UNORM;
  }

  SK::DXGI::ShaderBase <ID3D11PixelShader>   PixelShaderHDR_Uber;
  SK::DXGI::ShaderBase <ID3D11PixelShader>   PixelShaderHDR_Basic;
  SK::DXGI::ShaderBase <ID3D11VertexShader>  VertexShaderHDR_Util;
  SK::DXGI::ShaderBase <ID3D11ComputeShader> ComputeShaderHDR_Histogram;
  SK::DXGI::ShaderBase <ID3D11ComputeShader> ComputeShaderHDR_Merge;
  SK::DXGI::ShaderBase <ID3D11ComputeShader> ComputeShaderHDR_Response;

  void releaseResources (void)
  {
    if (mainSceneCBuffer  != nullptr)  { mainSceneCBuffer->Release  ();   mainSceneCBuffer = nullptr; }
    if (hudCBuffer        != nullptr)  { hudCBuffer->Release        ();         hudCBuffer = nullptr; }
    if (colorSpaceCBuffer != nullptr)  { colorSpaceCBuffer->Release ();  colorSpaceCBuffer = nullptr; }

    if (pSampler0         != nullptr)  { pSampler0->Release         ();          pSampler0 = nullptr; }

    if (pMainSrv          != nullptr)  { pMainSrv->Release          ();           pMainSrv = nullptr; }
    if (pCopySrv          != nullptr)  { pCopySrv->Release          ();           pCopySrv = nullptr; }
    if (pHUDSrv           != nullptr)  { pHUDSrv->Release           ();            pHUDSrv = nullptr; }

    if (pMainRtv          != nullptr)  { pMainRtv->Release          ();           pMainRtv = nullptr; }
    if (pHUDRtv           != nullptr)  { pHUDRtv->Release           ();            pHUDRtv = nullptr; }

    if (pRasterizerState  != nullptr)  { pRasterizerState->Release  ();   pRasterizerState = nullptr; }
    if (pDSState          != nullptr)  { pDSState->Release          ();           pDSState = nullptr; }

    if (pBlendState       != nullptr)  { pBlendState->Release       ();       pBlendState  = nullptr; }

    if (pInputLayout      != nullptr)  { pInputLayout->Release      ();       pInputLayout = nullptr; }

    if (pLuminanceTex     != nullptr)  { pLuminanceTex->Release     ();      pLuminanceTex = nullptr; }
    if (pLuminanceUAV     != nullptr)  { pLuminanceUAV->Release     ();      pLuminanceUAV = nullptr; }
    if (pLuminanceSRV     != nullptr)  { pLuminanceSRV->Release     ();      pLuminanceSRV = nullptr; }

    if (pGamutTex         != nullptr)  { pGamutTex->Release         ();          pGamutTex = nullptr; }
    if (pGamutUAV         != nullptr)  { pGamutUAV->Release         ();          pGamutUAV = nullptr; }
    if (pGamutSRV         != nullptr)  { pGamutSRV->Release         ();          pGamutSRV = nullptr; }

    PixelShaderHDR_Uber.releaseResources        ();
    PixelShaderHDR_Basic.releaseResources       ();
    VertexShaderHDR_Util.releaseResources       ();
    ComputeShaderHDR_Histogram.releaseResources ();
    ComputeShaderHDR_Merge.releaseResources     ();
    ComputeShaderHDR_Response.releaseResources  ();
  }

  bool
  recompileShaders (void)
  {
    std::wstring debug_shader_dir = SK_GetConfigPath ();
                 debug_shader_dir +=
            LR"(SK_Res\Debug\shaders\)";

    static auto& rb =
      SK_GetCurrentRenderBackend ();

    auto pDev =
      rb.getDevice <ID3D11Device> ();

    bool ret =
      SUCCEEDED (
        pDev->CreatePixelShader ( uber_hdr_shader_ps_bytecode,
                          sizeof (uber_hdr_shader_ps_bytecode),
               nullptr, &PixelShaderHDR_Uber.shader )
                ) &&
      SUCCEEDED (
        pDev->CreatePixelShader ( basic_hdr_shader_ps_bytecode,
                          sizeof (basic_hdr_shader_ps_bytecode),
               nullptr, &PixelShaderHDR_Basic.shader )
                ) &&
      SUCCEEDED (
        pDev->CreateVertexShader ( colorutil_vs_bytecode,
                           sizeof (colorutil_vs_bytecode),
              nullptr, &VertexShaderHDR_Util.shader )
                ) &&
      SUCCEEDED (
        pDev->CreateComputeShader ( hdr_histogram_cs_bytecode,
                            sizeof (hdr_histogram_cs_bytecode),
               nullptr, &ComputeShaderHDR_Histogram.shader )
      )           &&
      SUCCEEDED (
        pDev->CreateComputeShader ( hdr_merge_cs_bytecode,
                            sizeof (hdr_merge_cs_bytecode),
               nullptr, &ComputeShaderHDR_Merge.shader )
      )           &&
      SUCCEEDED (
        pDev->CreateComputeShader ( hdr_response_cs_bytecode,
                            sizeof (hdr_response_cs_bytecode),
               nullptr, &ComputeShaderHDR_Response.shader )
      );

    return ret;
  }

  void
  reloadResources (void)
  {
    if (mainSceneCBuffer  != nullptr) { mainSceneCBuffer->Release  ();   mainSceneCBuffer = nullptr; }
    if (colorSpaceCBuffer != nullptr) { colorSpaceCBuffer->Release ();  colorSpaceCBuffer = nullptr; }
    ////if (hudCBuffer       == nullptr)  { hudCBuffer->Release       ();        hudCBuffer = nullptr; }

    if (pSampler0        != nullptr)  { pSampler0->Release        ();         pSampler0 = nullptr; }

    if (pMainSrv         != nullptr)  { pMainSrv->Release         ();          pMainSrv = nullptr; }
    if (pCopySrv         != nullptr)  { pCopySrv->Release         ();          pCopySrv = nullptr; }
    if (pHUDSrv          != nullptr)  { pHUDSrv->Release          ();           pHUDSrv = nullptr; }

    if (pMainRtv         != nullptr)  { pMainRtv->Release         ();          pMainRtv = nullptr; }
    if (pHUDRtv          != nullptr)  { pHUDRtv->Release          ();           pHUDRtv = nullptr; }

    if (pRasterizerState != nullptr)  { pRasterizerState->Release ();  pRasterizerState = nullptr; }
    if (pDSState         != nullptr)  { pDSState->Release         ();          pDSState = nullptr; }

    if (pBlendState      != nullptr)  { pBlendState->Release      ();       pBlendState = nullptr; }

    if (pInputLayout     != nullptr)  { pInputLayout->Release     ();      pInputLayout = nullptr; }

    if (pLuminanceTex    != nullptr)  { pLuminanceTex->Release    ();     pLuminanceTex = nullptr; }
    if (pLuminanceUAV    != nullptr)  { pLuminanceUAV->Release    ();     pLuminanceUAV = nullptr; }
    if (pLuminanceSRV    != nullptr)  { pLuminanceSRV->Release    ();     pLuminanceSRV = nullptr; }

    if (pGamutTex        != nullptr)  { pGamutTex->Release        ();         pGamutTex = nullptr; }
    if (pGamutUAV        != nullptr)  { pGamutUAV->Release        ();         pGamutUAV = nullptr; }
    if (pGamutSRV        != nullptr)  { pGamutSRV->Release        ();         pGamutSRV = nullptr; }

    static auto& rb =
      SK_GetCurrentRenderBackend ();

    auto pDev =
      rb.getDevice <ID3D11Device> ();

    SK_ComQIPtr <IDXGISwapChain>      pSwapChain (rb.swapchain);
    SK_ComQIPtr <ID3D11DeviceContext> pDevCtx    (rb.d3d11.immediate_ctx);

    // These things aren't updated atomically, apparently :)
    //
    //   Skip reload if device and SwapChain are not consistent
    if (! SK_D3D11_EnsureMatchingDevices (pSwapChain.p, pDev.p))
      return;

    if (pDev != nullptr)
    {
      if (! recompileShaders ())
        return;

      D3D11_BUFFER_DESC desc = { };

      desc.ByteWidth         = sizeof (HDR_LUMINANCE);
      desc.Usage             = D3D11_USAGE_DYNAMIC;
      desc.BindFlags         = D3D11_BIND_CONSTANT_BUFFER;
      desc.CPUAccessFlags    = D3D11_CPU_ACCESS_WRITE;
      desc.MiscFlags         = 0;

      pDev->CreateBuffer (&desc, nullptr, &mainSceneCBuffer);

      desc.ByteWidth         = sizeof (HDR_COLORSPACE_PARAMS);
      desc.Usage             = D3D11_USAGE_DYNAMIC;
      desc.BindFlags         = D3D11_BIND_CONSTANT_BUFFER;
      desc.CPUAccessFlags    = D3D11_CPU_ACCESS_WRITE;
      desc.MiscFlags         = 0;

      pDev->CreateBuffer (&desc, nullptr, &colorSpaceCBuffer);
    }

    if ( pMainSrv == nullptr &&
       pSwapChain != nullptr )
    {
      DXGI_SWAP_CHAIN_DESC swapDesc = { };
      D3D11_TEXTURE2D_DESC desc     = { };

      pSwapChain->GetDesc (&swapDesc);

      desc.Width              = swapDesc.BufferDesc.Width;
      desc.Height             = swapDesc.BufferDesc.Height;
      desc.MipLevels          = 1;
      desc.ArraySize          = 1;
      desc.Format             = SK_HDR_GetPreferredDXGIFormat (swapDesc.BufferDesc.Format);
      desc.SampleDesc.Count   = 1; // Will probably regret this if HDR ever procreates with MSAA
      desc.SampleDesc.Quality = 0;
      desc.Usage              = D3D11_USAGE_DEFAULT;
      desc.BindFlags          = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
      desc.CPUAccessFlags     = 0x0;
      desc.MiscFlags          = 0x0;

      SK_ComPtr <ID3D11Texture2D> pHDRTexture;
      SK_ComPtr <ID3D11Texture2D> pCopyTexture;

      pDev->CreateTexture2D          (&desc,       nullptr, &pHDRTexture);
      pDev->CreateShaderResourceView (pHDRTexture, nullptr, &pMainSrv);

#define SK_HDR_NAN_MITIGATION
#ifdef SK_HDR_NAN_MITIGATION
      pDev->CreateTexture2D          (&desc,        nullptr, &pCopyTexture);
      pDev->CreateShaderResourceView (pCopyTexture, nullptr, &pCopySrv);
#endif

      D3D11_INPUT_ELEMENT_DESC local_layout [] = {
        { "", 0, DXGI_FORMAT_R32_UINT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
      };

      pDev->CreateInputLayout ( local_layout, 0,
                                  (void *)(colorutil_vs_bytecode),
                                    sizeof (colorutil_vs_bytecode) /
                                    sizeof (colorutil_vs_bytecode [0]),
                                      &pInputLayout );

      D3D11_SAMPLER_DESC
        sampler_desc                 = { };

        sampler_desc.Filter          = D3D11_FILTER_MIN_MAG_MIP_POINT;
        sampler_desc.AddressU        = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampler_desc.AddressV        = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampler_desc.AddressW        = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampler_desc.MipLODBias      = 0.f;
        sampler_desc.MaxAnisotropy   =   1;
        sampler_desc.ComparisonFunc  =  D3D11_COMPARISON_ALWAYS;
        sampler_desc.MinLOD          = -D3D11_FLOAT32_MAX;
        sampler_desc.MaxLOD          =  D3D11_FLOAT32_MAX;
        sampler_desc.BorderColor [0] = 1.0f;
        sampler_desc.BorderColor [1] = 1.0f;
        sampler_desc.BorderColor [2] = 1.0f;
        sampler_desc.BorderColor [3] = 1.0f;

      pDev->CreateSamplerState ( &sampler_desc,
                                            &pSampler0 );

      D3D11_BLEND_DESC
        blend                                        = {  };

      for ( int i = 0 ; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT ; ++i )
      {
        blend.RenderTarget [i].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_RED   |
                                                       D3D11_COLOR_WRITE_ENABLE_GREEN |
                                                       D3D11_COLOR_WRITE_ENABLE_BLUE;
      }

      pDev->CreateBlendState ( &blend,
                                   &pBlendState );

      D3D11_DEPTH_STENCIL_DESC
        depth                          = {  };

        depth.DepthEnable              = FALSE;
        depth.StencilEnable            = FALSE;
        depth.DepthWriteMask           = D3D11_DEPTH_WRITE_MASK_ZERO;
        depth.DepthFunc                = D3D11_COMPARISON_ALWAYS;
        depth.FrontFace.StencilFailOp  = depth.FrontFace.StencilDepthFailOp =
                                         depth.FrontFace.StencilPassOp      =
                                         D3D11_STENCIL_OP_KEEP;
        depth.FrontFace.StencilFunc    = D3D11_COMPARISON_ALWAYS;
        depth.BackFace                 = depth.FrontFace;

      pDev->CreateDepthStencilState ( &depth,
                                          &pDSState );

      D3D11_RASTERIZER_DESC
        raster                       = {  };

        raster.FillMode              = D3D11_FILL_SOLID;
        raster.CullMode              = D3D11_CULL_NONE;
        raster.DepthClipEnable       = TRUE;
        raster.DepthBiasClamp        = 0.0F;
        raster.SlopeScaledDepthBias  = 0.0F;

      pDev->CreateRasterizerState ( &raster,
                                         &pRasterizerState );

      D3D11_TEXTURE2D_DESC
        texDesc                  = {  };
        texDesc.Width            = 64;
        texDesc.Height           = 64;
        texDesc.Format           = DXGI_FORMAT_R32_FLOAT;
        texDesc.BindFlags        = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
        texDesc.SampleDesc.Count = 1;  // Avoid a copy so we can show the contents of this
        texDesc.MipLevels        = 1;
        texDesc.ArraySize        = 1;

      D3D11_UNORDERED_ACCESS_VIEW_DESC
        uavDesc                    = {  };
        uavDesc.ViewDimension      = D3D11_UAV_DIMENSION_TEXTURE2D;
        uavDesc.Texture2D.MipSlice = 0;
        uavDesc.Format             = DXGI_FORMAT_R32_FLOAT;

      pDev->CreateTexture2D           (&texDesc, nullptr, &pLuminanceTex);
      pDev->CreateUnorderedAccessView (                    pLuminanceTex,
                                       &uavDesc,          &pLuminanceUAV);
      pDev->CreateShaderResourceView  (                    pLuminanceTex,
                                                 nullptr, &pLuminanceSRV);

      texDesc                    = {  };
      texDesc.Width              = 1024;
      texDesc.Height             = 1024;
      texDesc.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;
      texDesc.BindFlags          = D3D11_BIND_UNORDERED_ACCESS |
                                   D3D11_BIND_SHADER_RESOURCE;
      texDesc.SampleDesc.Count   = 1;
      texDesc.MipLevels          = 1;
      texDesc.ArraySize          = 1;

      uavDesc                    = {  };
      uavDesc.ViewDimension      = D3D11_UAV_DIMENSION_TEXTURE2D;
      uavDesc.Texture2D.MipSlice = 0;
      uavDesc.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;

      pDev->CreateTexture2D           (&texDesc, nullptr, &pGamutTex);
      pDev->CreateUnorderedAccessView (                    pGamutTex,
                                       &uavDesc,          &pGamutUAV);
      pDev->CreateShaderResourceView  (                    pGamutTex,
                                                 nullptr, &pGamutSRV);

      SK_D3D11_SetDebugName (pSampler0,        L"SK HDR SamplerState");
      SK_D3D11_SetDebugName (pInputLayout,     L"SK HDR InputLayout");
      SK_D3D11_SetDebugName (pBlendState,      L"SK HDR BlendState");
      SK_D3D11_SetDebugName (pDSState,         L"SK HDR Depth/Stencil State");
      SK_D3D11_SetDebugName (pRasterizerState, L"SK HDR Rasterizer State");

    // This is about to exit scope anyway, don't give it a name
    //SK_D3D11_SetDebugName (pHDRTexture,  L"SK HDR OutputTex");
      SK_D3D11_SetDebugName (pMainSrv,     L"SK HDR OutputSRV");
      SK_D3D11_SetDebugName (pCopySrv,     L"SK HDR LastFrameSRV");

      SK_D3D11_SetDebugName (colorSpaceCBuffer, L"SK HDR ColorSpace CBuffer");
      SK_D3D11_SetDebugName (mainSceneCBuffer,  L"SK HDR MainScene CBuffer");

      SK_D3D11_SetDebugName (pLuminanceTex,     L"SK HDR Luminance Tilemap");
      SK_D3D11_SetDebugName (pLuminanceUAV,     L"SK HDR Luminance UAV");
      SK_D3D11_SetDebugName (pLuminanceSRV,     L"SK HDR Luminance SRV");

      SK_D3D11_SetDebugName (pGamutTex,         L"SK HDR Gamut Coverage Texture");
      SK_D3D11_SetDebugName (pGamutUAV,         L"SK HDR Gamut Coverage UAV");
      SK_D3D11_SetDebugName (pGamutSRV,         L"SK HDR Gamut Coverage SRV");
    }
  }
};

SK_LazyGlobal <SK_HDR_FIXUP> hdr_base;

SK_ComPtr <ID3D11Texture2D>          SK_HDR_GetGamutTex     (void) { return hdr_base->pGamutTex;     }
SK_ComPtr <ID3D11ShaderResourceView> SK_HDR_GetGamutSRV     (void) { return hdr_base->pGamutSRV;     }
SK_ComPtr <ID3D11Texture2D>          SK_HDR_GetLuminanceTex (void) { return hdr_base->pLuminanceTex; }
SK_ComPtr <ID3D11ShaderResourceView> SK_HDR_GetLuminanceSRV (void) { return hdr_base->pLuminanceSRV; }

int   __SK_HDR_tonemap       = 1;
int   __SK_HDR_visualization = 0;
int   __SK_HDR_Bypass_sRGB   = -1;
float __SK_HDR_PaperWhite    = 3.75f;
float __SK_HDR_user_sdr_Y    = 100.0f;

void
SK_HDR_ReleaseResources (void)
{
  hdr_base->releaseResources ();
}

void
SK_HDR_InitResources (void)
{
  hdr_base->reloadResources ();
}

//
// Remove negative numbers, infinity and NAN from floating-point
// RenderTargets because non-HDR shaders may operate assuming that
// these things are all unsigned and normalized out of existence.
// 
//   FP blending is particularly problematic, a NAN value breaks
//   the computation of both Src and Dst and will remain in the
//   buffer as an invalid pixel no matter the blend equation.
// 
// Similar to the HDR snapshot function, but stateblock code is
// very important here since this may be called upon to tidy-up
// a RenderTarget in the middle of a frame rather than a post-
// process step in SK's overlay rendering.
//
void
SK_HDR_SanitizeFP16SwapChain (void)
{
  if (! __SK_HDR_16BitSwap)
    return;

    static auto& rb =
    SK_GetCurrentRenderBackend ();

  bool hdr_display =
    (rb.isHDRCapable () && rb.isHDRActive ());

  if (! hdr_display)
    return;

  auto& vs_hdr_util  = hdr_base->VertexShaderHDR_Util;
  auto& ps_hdr_uber  = hdr_base->PixelShaderHDR_Uber;
  auto& ps_hdr_basic = hdr_base->PixelShaderHDR_Basic;

  if ( vs_hdr_util.shader  == nullptr ||
       ps_hdr_uber.shader  == nullptr ||
       ps_hdr_basic.shader == nullptr )
  {
    return;
  }

  DXGI_SWAP_CHAIN_DESC swapDesc = { };

  if ( vs_hdr_util.shader  != nullptr &&
       ps_hdr_uber.shader  != nullptr &&
       ps_hdr_basic.shader != nullptr &&
       hdr_base->pMainSrv  != nullptr )
  {
    auto pDev =
      rb.getDevice <ID3D11Device> ();

    SK_ComQIPtr <IDXGISwapChain>      pSwapChain (rb.swapchain);
    SK_ComQIPtr <ID3D11DeviceContext> pDevCtx    (rb.d3d11.immediate_ctx);

    if (pDev != nullptr     &&      pDevCtx == nullptr)
    {   pDev->GetImmediateContext (&pDevCtx.p); }

    if (! pDevCtx) return;

    SK_ComPtr <ID3D11RenderTargetView>   pRTV;
    SK_ComPtr <ID3D11ShaderResourceView> pSRV;

    SK_ComPtr <ID3D11Texture2D> pSrc = nullptr;
    SK_ComPtr <ID3D11Texture2D> pDst = nullptr;

    if (pSwapChain != nullptr)
    {
      pSwapChain->GetDesc (&swapDesc);
      pDevCtx->OMGetRenderTargets (1, &pRTV.p, nullptr);

      if (! pRTV.p)
        return;

      SK_ComPtr <ID3D11Resource> pOut;
      pRTV->GetResource        (&pOut.p);
      pOut->QueryInterface <ID3D11Texture2D> (&pDst.p);

      if (! pDst.p)
        return;

      D3D11_TEXTURE2D_DESC    texDesc = { };
      pDst->GetDesc         (&texDesc);
      pDev->CreateTexture2D (&texDesc, nullptr, &pSrc.p);
      
      if (texDesc.SampleDesc.Count > 1)
        pDevCtx->ResolveSubresource        (pSrc, 0, pDst, 0, texDesc.Format);
      else
      {
        SK_ComQIPtr <ID3D11DeviceContext1>
            pDevCtx1 (pDevCtx);
        if (pDevCtx1.p != nullptr)
          pDevCtx1->CopySubresourceRegion1 (pSrc, 0, 0, 0, 0,
                                            pDst, 0, nullptr, D3D11_COPY_DISCARD);
        else
          pDevCtx->CopyResource            (pSrc,    pDst);
      }

      SK_ComQIPtr <ID3D11Resource>    pSrcRes (pSrc);
      pDev->CreateShaderResourceView (pSrcRes.p, nullptr, &pSRV.p);
    }

    D3D11_MAPPED_SUBRESOURCE mapped_resource  = { };
    HDR_LUMINANCE            cbuffer_luma     = { };
    HDR_COLORSPACE_PARAMS    cbuffer_cspace   = { };

    bool need_full_hdr_processing = false;
 
    need_full_hdr_processing |= 
      ( __SK_HDR_HorizCoverage != 100.0f ||
        __SK_HDR_VertCoverage  != 100.0f ||
        __SK_HDR_visualization != 0 ||
        __SK_HDR_tonemap       != 0 );

    auto pPixelShaderHDR =
      need_full_hdr_processing ? hdr_base->PixelShaderHDR_Uber.shader
                               : hdr_base->PixelShaderHDR_Basic.shader;

    if ( SUCCEEDED (
           pDevCtx->Map ( hdr_base->mainSceneCBuffer,
                            0, D3D11_MAP_WRITE_DISCARD, 0,
                               &mapped_resource )
                   )
       )
    {
      _ReadWriteBarrier ();

      memcpy (          static_cast <HDR_LUMINANCE *> (mapped_resource.pData),
               &cbuffer_luma, sizeof HDR_LUMINANCE );

      pDevCtx->Unmap (hdr_base->mainSceneCBuffer, 0);

    //snap_cache.last_hash_luma = cb_hash_luma;
    }

    else return;

    cbuffer_cspace.uiToneMapper           =   255; // Copy Resource

    if ( SUCCEEDED (
           pDevCtx->Map ( hdr_base->colorSpaceCBuffer,
                            0, D3D11_MAP_WRITE_DISCARD, 0,
                              &mapped_resource )
                   )
       )
    {
      _ReadWriteBarrier ();

      memcpy (            static_cast <HDR_COLORSPACE_PARAMS *> (mapped_resource.pData),
               &cbuffer_cspace, sizeof HDR_COLORSPACE_PARAMS );

      pDevCtx->Unmap (hdr_base->colorSpaceCBuffer, 0);

    //snap_cache.last_hash_cspace = cb_hash_cspace;
    }

    else return;

#if 0
    SK_IMGUI_D3D11StateBlock
      sb;
      sb.Capture (pDevCtx);
#else
    D3DX11_STATE_BLOCK stateBlock = { };
    CreateStateblock (pDevCtx, &stateBlock);
#endif

    SK_ComPtr <ID3D11RenderTargetView>  pRenderTargetView;
    if (! _d3d11_rbk->frames_.empty ()) pRenderTargetView =
          _d3d11_rbk->frames_ [0].hdr.pRTV;

    if (              pRenderTargetView.p != nullptr                                 &&
               swapDesc.BufferDesc.Format == DXGI_FORMAT_R16G16B16A16_FLOAT          &&
         ( rb.scanout.dxgi_colorspace     == DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709 ||
           rb.scanout.dwm_colorspace      == DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709 ||
           rb.scanout.colorspace_override == DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709 ) )
    {
      SK_ComQIPtr <ID3D11DeviceContext3> pDevCtx3 (pDevCtx);

      using d3d11_srv_t =
           ID3D11ShaderResourceView;

      struct {
        struct {
          SK_ComPtr <d3d11_srv_t>           original [D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = { };
          SK_ComPtr <d3d11_srv_t>           hdr      [2] = {
            hdr_base->pMainSrv,
            hdr_base->pCopySrv
             //hdr_base->pHUDSrv
          };
          SK_ComPtr <ID3D11SamplerState>    samplers [D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT] = { };
        } ps;

        struct {
          SK_ComPtr <ID3D11UnorderedAccessView>
                                            uavs [D3D11_1_UAV_SLOT_COUNT] = { };
        } cs;

        struct {
          SK_ComPtr <ID3D11Buffer>          vs [D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT] = { };
          SK_ComPtr <ID3D11Buffer>          ps [D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT] = { };
          SK_ComPtr <ID3D11Buffer>          cs [D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT] = { };
        } cbuffers;
      } resources;

      const D3D11_VIEWPORT vp = {      0.0f, 0.0f,
        static_cast <float> (swapDesc.BufferDesc.Width ),
        static_cast <float> (swapDesc.BufferDesc.Height),
                                       0.0f, 1.0f
      };

      resources.ps.hdr [0] = pSRV.p;

      pDevCtx->IASetPrimitiveTopology (D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
      pDevCtx->IASetVertexBuffers     (0, 1, std::array <ID3D11Buffer *, 1> { nullptr }.data (),
                                             std::array <UINT,           1> { 0       }.data (),
                                             std::array <UINT,           1> { 0       }.data ());
      pDevCtx->IASetInputLayout       (hdr_base->pInputLayout);
      pDevCtx->IASetIndexBuffer       (nullptr, DXGI_FORMAT_UNKNOWN, 0);

      pDevCtx->OMSetBlendState        (hdr_base->pBlendState, std::array <const FLOAT, 4> { 0.0f, 0.0f, 0.0f, 0.0f }.data (),
                                         0xFFFFFFFFUL);

      pDevCtx->VSSetShader            (       hdr_base->VertexShaderHDR_Util.shader, nullptr, 0);
      pDevCtx->VSSetConstantBuffers   (0, 1, &hdr_base->mainSceneCBuffer);

      pDevCtx->PSSetShader            (       pPixelShaderHDR,                       nullptr, 0);
      pDevCtx->PSSetConstantBuffers   (0, 1, &hdr_base->colorSpaceCBuffer);
      
      pDevCtx->RSSetState             (hdr_base->pRasterizerState);
      pDevCtx->RSSetScissorRects      (0, nullptr);
      pDevCtx->RSSetViewports         (1, &vp);

      pDevCtx->OMSetDepthStencilState (hdr_base->pDSState, 0);

      pDevCtx->PSSetShaderResources   (0, 2, &resources.ps.hdr [0].p);
      pDevCtx->PSSetSamplers          (0, 1, &hdr_base->pSampler0);

      if (pDev->GetFeatureLevel () >= D3D_FEATURE_LEVEL_10_0)
      {
        pDevCtx->GSSetShader          (nullptr,        nullptr,       0);
        pDevCtx->SOSetTargets         (0,              nullptr, nullptr);

        if (pDev->GetFeatureLevel () >= D3D_FEATURE_LEVEL_11_0)
        {
          pDevCtx->HSSetShader        (nullptr,        nullptr,       0);
          pDevCtx->DSSetShader        (nullptr,        nullptr,       0);
        }
      }

      // -*- //

      pDevCtx->SetPredication         (nullptr, FALSE);
      D3D11_Draw_Original             (pDevCtx, 4,  0);

      // -*- //

      ApplyStateblock (pDevCtx, &stateBlock);
      //sb.Apply (pDevCtx);
    }
  }
}

void
SK_HDR_SnapshotSwapchain (void)
{
  static auto& rb =
    SK_GetCurrentRenderBackend ();

  bool hdr_display =
    (rb.isHDRCapable () && rb.isHDRActive ());

  if (! hdr_display)
    return;

  SK_RenderBackend::scan_out_s::SK_HDR_TRANSFER_FUNC eotf =
    rb.scanout.getEOTF ();

  bool bEOTF_is_PQ =
    (eotf == SK_RenderBackend::scan_out_s::SMPTE_2084);

  if (bEOTF_is_PQ)
    return;

  auto& vs_hdr_util  = hdr_base->VertexShaderHDR_Util;
  auto& ps_hdr_uber  = hdr_base->PixelShaderHDR_Uber;
  auto& ps_hdr_basic = hdr_base->PixelShaderHDR_Basic;

  if ( vs_hdr_util.shader  == nullptr ||
       ps_hdr_uber.shader  == nullptr ||
       ps_hdr_basic.shader == nullptr )
  {
    hdr_base->reloadResources ();
  }

  DXGI_SWAP_CHAIN_DESC swapDesc = { };

  if ( vs_hdr_util.shader  != nullptr &&
       ps_hdr_uber.shader  != nullptr &&
       ps_hdr_basic.shader != nullptr &&
       hdr_base->pMainSrv  != nullptr )
  {
    SK_ComQIPtr <IDXGISwapChain>      pSwapChain (rb.swapchain);
    SK_ComPtr   <ID3D11Device>        pDev;
    SK_ComPtr   <ID3D11DeviceContext> pDevCtx;

    if (pSwapChain.p != nullptr)
        pSwapChain->GetDevice (IID_ID3D11Device, (void **)&pDev.p);

    if (pDev != nullptr     &&      pDevCtx == nullptr)
    {   pDev->GetImmediateContext (&pDevCtx.p); }

    if (! pDevCtx)
      return;

    SK_ComPtr <ID3D11Device>        pShaderDevice;
    vs_hdr_util.shader->GetDevice (&pShaderDevice.p);

    if (! (pShaderDevice.p           == pDev.p ||
           pShaderDevice.IsEqualObject (pDev)) )
    {
      return;
    }

    SK_ComPtr <ID3D11Texture2D> pSrc = nullptr;
    SK_ComPtr <ID3D11Resource>  pDst = nullptr;

    if (pSwapChain != nullptr)
    {
      pSwapChain->GetDesc (&swapDesc);

      if ( SUCCEEDED (
             pSwapChain->GetBuffer   (
               0,
                 IID_ID3D11Texture2D,
                   (void **)&pSrc.p  )
                     )
         )
      {
        hdr_base->pMainSrv->GetResource      (&pDst.p);

        D3D11_TEXTURE2D_DESC texDesc = { };
        pSrc->GetDesc      (&texDesc);

        if (texDesc.SampleDesc.Count > 1)
          pDevCtx->ResolveSubresource        (pDst, 0, pSrc, 0, texDesc.Format);
        else
        {
          SK_ComQIPtr <ID3D11DeviceContext1>
              pDevCtx1 (pDevCtx);
          if (pDevCtx1.p != nullptr)
            pDevCtx1->CopySubresourceRegion1 (pDst, 0, 0, 0, 0,
                                              pSrc, 0, nullptr, D3D11_COPY_DISCARD);
          else
            pDevCtx->CopyResource            (pDst,    pSrc);
        }
      }
    }

    D3D11_MAPPED_SUBRESOURCE mapped_resource  = { };
    HDR_LUMINANCE            cbuffer_luma     = { };
    HDR_COLORSPACE_PARAMS    cbuffer_cspace   = { };

    bool need_full_hdr_processing = false;

    cbuffer_luma.luminance_scale [0] =  __SK_HDR_Luma;
    cbuffer_luma.luminance_scale [1] =  __SK_HDR_Exp;
    cbuffer_luma.luminance_scale [2] = (__SK_HDR_HorizCoverage / 100.0f) * 2.0f - 1.0f;
    cbuffer_luma.luminance_scale [3] = (__SK_HDR_VertCoverage  / 100.0f) * 2.0f - 1.0f;
 
    need_full_hdr_processing |= 
      ( __SK_HDR_HorizCoverage != 100.0f ||
        __SK_HDR_VertCoverage  != 100.0f ||
        __SK_HDR_visualization != 0 ||
        __SK_HDR_tonemap       != 0 );

    auto pPixelShaderHDR =
      need_full_hdr_processing ? hdr_base->PixelShaderHDR_Uber.shader
                               : hdr_base->PixelShaderHDR_Basic.shader;

    if ( SUCCEEDED (
           pDevCtx->Map ( hdr_base->mainSceneCBuffer,
                            0, D3D11_MAP_WRITE_DISCARD, 0,
                               &mapped_resource )
                   )
       )
    {
      _ReadWriteBarrier ();

      memcpy (          static_cast <HDR_LUMINANCE *> (mapped_resource.pData),
               &cbuffer_luma, sizeof HDR_LUMINANCE );

      pDevCtx->Unmap (hdr_base->mainSceneCBuffer, 0);
    }

    else return;

    cbuffer_cspace.uiToneMapper           =   __SK_HDR_tonemap;
    cbuffer_cspace.hdrSaturation          =   __SK_HDR_Saturation;
    cbuffer_cspace.hdrGamutExpansion      =   __SK_HDR_Gamut;
  //cbuffer_cspace.hdrPaperWhite          =   __SK_HDR_PaperWhite;
    cbuffer_cspace.sdrLuminance_NonStd    =   __SK_HDR_user_sdr_Y * 1.0_Nits;
    cbuffer_cspace.sdrIsImplicitlysRGB    =   __SK_HDR_Bypass_sRGB != 1;
    cbuffer_cspace.visualFunc [0]         = (uint32_t)__SK_HDR_visualization;
    cbuffer_cspace.visualFunc [1]         = (uint32_t)__SK_HDR_visualization;
    cbuffer_cspace.visualFunc [2]         = (uint32_t)__SK_HDR_visualization;

    cbuffer_cspace.hdrLuminance_MaxAvg   = __SK_HDR_tonemap == 2 ?
                                    rb.working_gamut.maxAverageY != 0.0f ?
                                    rb.working_gamut.maxAverageY         : rb.display_gamut.maxAverageY
                                                                 :         rb.display_gamut.maxAverageY;
    cbuffer_cspace.hdrLuminance_MaxLocal = __SK_HDR_tonemap == 2 ?
                                    rb.working_gamut.maxLocalY != 0.0f ?
                                    rb.working_gamut.maxLocalY         : rb.display_gamut.maxLocalY
                                                                 :       rb.display_gamut.maxLocalY;
    cbuffer_cspace.hdrLuminance_Min      = rb.display_gamut.minY * 1.0_Nits;
    cbuffer_cspace.currentTime           = (float)SK_timeGetTime ();

    extern float                       __SK_HDR_PQBoost0;
    extern float                       __SK_HDR_PQBoost1;
    extern float                       __SK_HDR_PQBoost2;
    extern float                       __SK_HDR_PQBoost3;
    cbuffer_cspace.pqBoostParams [0] = __SK_HDR_PQBoost0;
    cbuffer_cspace.pqBoostParams [1] = __SK_HDR_PQBoost1;
    cbuffer_cspace.pqBoostParams [2] = __SK_HDR_PQBoost2;
    cbuffer_cspace.pqBoostParams [3] = __SK_HDR_PQBoost3;

    if ( SUCCEEDED (
           pDevCtx->Map ( hdr_base->colorSpaceCBuffer,
                            0, D3D11_MAP_WRITE_DISCARD, 0,
                              &mapped_resource )
                   )
       )
    {
      _ReadWriteBarrier ();

      memcpy (            static_cast <HDR_COLORSPACE_PARAMS *> (mapped_resource.pData),
               &cbuffer_cspace, sizeof HDR_COLORSPACE_PARAMS );

      pDevCtx->Unmap (hdr_base->colorSpaceCBuffer, 0);
    }

    else return;

    SK_ComPtr <ID3D11RenderTargetView>  pRenderTargetView;
    if (! _d3d11_rbk->frames_.empty ()) pRenderTargetView =
          _d3d11_rbk->frames_ [0].hdr.pRTV;

    if (              pRenderTargetView.p != nullptr                                 &&
               swapDesc.BufferDesc.Format == DXGI_FORMAT_R16G16B16A16_FLOAT          &&
         ( rb.scanout.dxgi_colorspace     == DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709 ||
           rb.scanout.dwm_colorspace      == DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709 ||
           rb.scanout.colorspace_override == DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709 ) )
    {
      SK_ComQIPtr <ID3D11DeviceContext3> pDevCtx3 (pDevCtx);

#if 0
    D3DX11_STATE_BLOCK stateBlock = { };
    CreateStateblock (pDevCtx, &stateBlock);
#endif

      using d3d11_srv_t =
           ID3D11ShaderResourceView;

      struct {
        struct {
          SK_ComPtr <d3d11_srv_t>           hdr      [2] = {
            hdr_base->pMainSrv,
            hdr_base->pCopySrv
             //hdr_base->pHUDSrv
          };
        } ps;
      } resources;

      const D3D11_VIEWPORT vp = {      0.0f, 0.0f,
        static_cast <float> (swapDesc.BufferDesc.Width ),
        static_cast <float> (swapDesc.BufferDesc.Height),
                                       0.0f, 1.0f
      };

      if (pDev->GetFeatureLevel () >= D3D_FEATURE_LEVEL_10_0)
      {
        pDevCtx->GSSetShader          (nullptr,        nullptr,       0);
        pDevCtx->SOSetTargets         (0,              nullptr, nullptr);

        if (pDev->GetFeatureLevel () >= D3D_FEATURE_LEVEL_11_0)
        {
          pDevCtx->HSSetShader        (nullptr,        nullptr,       0);
          pDevCtx->DSSetShader        (nullptr,        nullptr,       0);
        }
      }

      pDevCtx->IASetPrimitiveTopology (D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
      pDevCtx->IASetVertexBuffers     (0, 1, std::array <ID3D11Buffer *, 1> { nullptr }.data (),
                                             std::array <UINT,           1> { 0       }.data (),
                                             std::array <UINT,           1> { 0       }.data ());
      pDevCtx->IASetInputLayout       (hdr_base->pInputLayout);
      pDevCtx->IASetIndexBuffer       (nullptr, DXGI_FORMAT_UNKNOWN, 0);

      pDevCtx->OMSetBlendState        (hdr_base->pBlendState, nullptr, 0xFFFFFFFFUL);

      pDevCtx->VSSetShader            (       hdr_base->VertexShaderHDR_Util.shader, nullptr, 0);
      pDevCtx->VSSetConstantBuffers   (0, 1, &hdr_base->mainSceneCBuffer);

      pDevCtx->PSSetShader            (       pPixelShaderHDR,                       nullptr, 0);
      pDevCtx->PSSetConstantBuffers   (0, 1, &hdr_base->colorSpaceCBuffer);

      pDevCtx->RSSetState             (hdr_base->pRasterizerState);
      pDevCtx->RSSetScissorRects      (0, nullptr);
      pDevCtx->RSSetViewports         (1, &vp);

      pDevCtx->OMSetDepthStencilState ( hdr_base->pDSState,
                                        0 );
      pDevCtx->OMSetRenderTargets     ( 1,
                                         &pRenderTargetView.p,
                                            nullptr );

      pDevCtx->PSSetShaderResources   (0, 2, &resources.ps.hdr [0].p);
      pDevCtx->PSSetSamplers          (0, 1, &hdr_base->pSampler0);

      // -*- //

      pDevCtx->SetPredication         (nullptr, FALSE);
      pDevCtx->Draw                   (4,       0);

#ifdef SK_HDR_NAN_MITIGATION
      SK_ComPtr <ID3D11Resource>        pHDRTexture;
      SK_ComPtr <ID3D11Resource>        pCopyTexture;
      hdr_base->pMainSrv->GetResource (&pHDRTexture.p);
      hdr_base->pCopySrv->GetResource (&pCopyTexture.p);
      pDevCtx->CopyResource           ( pCopyTexture,
                                        pHDRTexture );
#endif

      if ( __SK_HDR_AdaptiveToneMap && // SwapChain may not support UAVs...
            _d3d11_rbk->frames_ [0].hdr.pUAV.p != nullptr )
      {
        SK_ComPtr <ID3D11UnorderedAccessView>
            pFramebufferUAV (_d3d11_rbk->frames_ [0].hdr.pUAV);
        if (pFramebufferUAV.p != nullptr)
        {
          static ID3D11UnorderedAccessView* const
            nul_uavs [D3D11_PS_CS_UAV_REGISTER_COUNT]                    = { };
          static ID3D11ShaderResourceView* const
            nul_srvs [D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT]      = { };
          static ID3D11RenderTargetView* const                           
            nul_rtvs [D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT]            = { };
          static ID3D11Buffer* const
            nul_bufs [D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT] = { };

          pDevCtx->OMSetRenderTargetsAndUnorderedAccessViews (0, nullptr, nullptr, 0, 0, nullptr, nullptr);
          pDevCtx->CSSetUnorderedAccessViews (0, D3D11_PS_CS_UAV_REGISTER_COUNT, nul_uavs, nullptr);

          pDevCtx->ClearUnorderedAccessViewFloat (
            hdr_base->pGamutUAV,
            std::array <FLOAT, 4> { 0.0F, 0.0F,
                                    0.0F, 1.0F }.data ()
          );

          ID3D11UnorderedAccessView* uavs [] =
          {         pFramebufferUAV.p,
            hdr_base->pLuminanceUAV,
                hdr_base->pGamutUAV
          };

          pDevCtx->OMSetRenderTargets        (   D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT,       nul_rtvs, nullptr);
          pDevCtx->PSSetShaderResources      (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, nul_srvs);
          pDevCtx->PSSetConstantBuffers      (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT,
                                                                                               nul_bufs);
          pDevCtx->CSSetConstantBuffers      (0, 1,                        &hdr_base->colorSpaceCBuffer);
          pDevCtx->CSSetUnorderedAccessViews (0, 3, uavs,                                    nullptr   );
          pDevCtx->CSSetShader               (hdr_base->ComputeShaderHDR_Histogram.shader,   nullptr, 0);
          pDevCtx->Dispatch                 ( 32,
                                              16, 1 );
          pDevCtx->CSSetShader               (hdr_base->ComputeShaderHDR_Merge.shader,       nullptr, 0);
          pDevCtx->Dispatch                 ( 1, 1, 1 );
          pDevCtx->CSSetShader               (hdr_base->ComputeShaderHDR_Response.shader,    nullptr, 0);
          pDevCtx->Dispatch                 ( 32,
                                              16, 1 );

          pDevCtx->OMSetRenderTargetsAndUnorderedAccessViews
                                             (0, nullptr, nullptr, 0, 0, nullptr, nullptr);
          pDevCtx->CSSetUnorderedAccessViews (0, D3D11_PS_CS_UAV_REGISTER_COUNT, nul_uavs, nullptr);
        }
      }

      // -*- //

#if 0
      ApplyStateblock (pDevCtx, &stateBlock);
#endif
    }
  }
}

void
SK_HDR_CombineSceneWithHUD (void)
{
}

bool
SK_HDR_RecompileShaders (void)
{
  return
    hdr_base->recompileShaders ();
}

ID3D11ShaderResourceView*
SK_HDR_GetUnderlayResourceView (void)
{
  return
    hdr_base->pMainSrv;
}



std::string SK_HDR_FIXUP::_SpecialNowhere;