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
#include <SpecialK/render/d3d11/d3d11_core.h>
#include <SpecialK/render/d3d11/d3d11_state_tracker.h>

#include <shaders/uber_hdr_shader_ps.h>
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

  ID3D11Texture2D*             pGamutTex     = nullptr;
  ID3D11UnorderedAccessView*   pGamutUAV     = nullptr;

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

  SK::DXGI::ShaderBase <ID3D11PixelShader>   PixelShader_scRGB;
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

    if (pGamutTex         != nullptr)  { pGamutTex->Release         ();          pGamutTex = nullptr; }
    if (pGamutUAV         != nullptr)  { pGamutUAV->Release         ();          pGamutUAV = nullptr; }

    PixelShader_scRGB.releaseResources          ();
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
               nullptr, &PixelShader_scRGB.shader )
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

    if (pGamutTex        != nullptr)  { pGamutTex->Release        ();         pGamutTex = nullptr; }
    if (pGamutUAV        != nullptr)  { pGamutUAV->Release        ();         pGamutUAV = nullptr; }

    static auto& rb =
      SK_GetCurrentRenderBackend ();

    auto pDev =
      rb.getDevice <ID3D11Device> ();

    SK_ComQIPtr <IDXGISwapChain>      pSwapChain (rb.swapchain);
    SK_ComQIPtr <ID3D11DeviceContext> pDevCtx    (rb.d3d11.immediate_ctx);

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
      desc.BindFlags          = D3D11_BIND_SHADER_RESOURCE;
      desc.CPUAccessFlags     = 0x0;
      desc.MiscFlags          = 0x0;

      SK_ComPtr <ID3D11Texture2D> pHDRTexture;
      SK_ComPtr <ID3D11Texture2D> pCopyTexture;

      pDev->CreateTexture2D          (&desc,       nullptr, &pHDRTexture);
      pDev->CreateShaderResourceView (pHDRTexture, nullptr, &pMainSrv);

    //pDev->CreateTexture2D          (&desc,        nullptr, &pCopyTexture);
    //pDev->CreateShaderResourceView (pCopyTexture, nullptr, &pCopySrv);

      D3D11_INPUT_ELEMENT_DESC local_layout [] = {
        { "", 0, DXGI_FORMAT_R32_UINT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
      };

      pDev->CreateInputLayout ( local_layout, 0,
                                  (void *)(colorutil_vs_bytecode),
                                    sizeof (colorutil_vs_bytecode) /
                                    sizeof (colorutil_vs_bytecode [0]),
                                      &pInputLayout );

      D3D11_SAMPLER_DESC
        sampler_desc                    = { };

        sampler_desc.Filter             = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sampler_desc.AddressU           = D3D11_TEXTURE_ADDRESS_WRAP;
        sampler_desc.AddressV           = D3D11_TEXTURE_ADDRESS_WRAP;
        sampler_desc.AddressW           = D3D11_TEXTURE_ADDRESS_WRAP;
        sampler_desc.MipLODBias         = 0.f;
        sampler_desc.MaxAnisotropy      =   1;
        sampler_desc.ComparisonFunc     =  D3D11_COMPARISON_NEVER;
        sampler_desc.MinLOD             = -D3D11_FLOAT32_MAX;
        sampler_desc.MaxLOD             =  D3D11_FLOAT32_MAX;

      pDev->CreateSamplerState ( &sampler_desc,
                                            &pSampler0 );

      D3D11_BLEND_DESC
        blend                                        = {  };
        blend.AlphaToCoverageEnable                  = FALSE;
        blend.IndependentBlendEnable                 = FALSE;
      for ( int i = 0 ; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT ; ++i )
      {
        blend.RenderTarget [i].BlendEnable           = FALSE;
        blend.RenderTarget [i].SrcBlend              = D3D11_BLEND_ONE;
        blend.RenderTarget [i].DestBlend             = D3D11_BLEND_ZERO;
        blend.RenderTarget [i].BlendOp               = D3D11_BLEND_OP_ADD;
        blend.RenderTarget [i].SrcBlendAlpha         = D3D11_BLEND_ONE;
        blend.RenderTarget [i].DestBlendAlpha        = D3D11_BLEND_ZERO;
        blend.RenderTarget [i].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
        blend.RenderTarget [i].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
      }

      pDev->CreateBlendState ( &blend,
                                   &pBlendState );

      D3D11_DEPTH_STENCIL_DESC
        depth                          = {  };

        depth.DepthEnable              = FALSE;
        depth.DepthWriteMask           = D3D11_DEPTH_WRITE_MASK_ZERO;
        depth.DepthFunc                = D3D11_COMPARISON_NEVER;
        depth.StencilEnable            = FALSE;
        depth.StencilReadMask          = D3D11_DEFAULT_STENCIL_READ_MASK;
        depth.StencilWriteMask         = D3D11_DEFAULT_STENCIL_READ_MASK;
        depth.FrontFace.StencilFailOp  = depth.FrontFace.StencilDepthFailOp =
                                         depth.FrontFace.StencilPassOp      =
                                         D3D11_STENCIL_OP_KEEP;
        depth.FrontFace.StencilFunc    = D3D11_COMPARISON_NEVER;
        depth.BackFace                 = depth.FrontFace;

      pDev->CreateDepthStencilState ( &depth,
                                          &pDSState );

      D3D11_RASTERIZER_DESC
        raster                       = {  };

        raster.FillMode              = D3D11_FILL_SOLID;
        raster.CullMode              = D3D11_CULL_NONE;
        raster.ScissorEnable         = FALSE;
        raster.DepthClipEnable       = FALSE;
        raster.MultisampleEnable     = FALSE;
        raster.AntialiasedLineEnable = FALSE;

      pDev->CreateRasterizerState ( &raster,
                                         &pRasterizerState );

      D3D11_TEXTURE2D_DESC
        texDesc                  = {  };
        texDesc.Width            = 64;
        texDesc.Height           = 64;
        texDesc.Format           = DXGI_FORMAT_R32_FLOAT;
        texDesc.BindFlags        = D3D11_BIND_UNORDERED_ACCESS;
        texDesc.SampleDesc.Count = 1;
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

      texDesc                    = {  };
      texDesc.Width              = 384;
      texDesc.Height             = 384;
      texDesc.Format             = DXGI_FORMAT_B8G8R8X8_UNORM;
      texDesc.BindFlags          = D3D11_BIND_UNORDERED_ACCESS |
                                   D3D11_BIND_SHADER_RESOURCE;
      texDesc.SampleDesc.Count   = 1;
      texDesc.MipLevels          = 1;
      texDesc.ArraySize          = 1;

      uavDesc                    = {  };
      uavDesc.ViewDimension      = D3D11_UAV_DIMENSION_TEXTURE2D;
      uavDesc.Texture2D.MipSlice = 0;
      uavDesc.Format             = DXGI_FORMAT_B8G8R8X8_UNORM;

      pDev->CreateTexture2D           (&texDesc, nullptr, &pGamutTex);
      pDev->CreateUnorderedAccessView (                    pGamutTex,
                                       &uavDesc,          &pGamutUAV);

      SK_D3D11_SetDebugName (pSampler0,        L"SK HDR SamplerState");
      SK_D3D11_SetDebugName (pInputLayout,     L"SK HDR InputLayout");
      SK_D3D11_SetDebugName (pBlendState,      L"SK HDR BlendState");
      SK_D3D11_SetDebugName (pDSState,         L"SK HDR Depth/Stencil State");
      SK_D3D11_SetDebugName (pRasterizerState, L"SK HDR Rasterizer State");

    // This is about to exit scope anyway, don't give it a name
    //SK_D3D11_SetDebugName (pHDRTexture,  L"SK HDR OutputTex");
      SK_D3D11_SetDebugName (pMainSrv,     L"SK HDR OutputSRV");
    //SK_D3D11_SetDebugName (pCopySrv,     L"SK HDR LastFrameSRV");

      SK_D3D11_SetDebugName (colorSpaceCBuffer, L"SK HDR ColorSpace CBuffer");
      SK_D3D11_SetDebugName (mainSceneCBuffer,  L"SK HDR MainScene CBuffer");

      SK_D3D11_SetDebugName (pLuminanceTex,     L"SK HDR Luminance Tilemap");
      SK_D3D11_SetDebugName (pLuminanceUAV,     L"SK HDR Luminance UAV");

      SK_D3D11_SetDebugName (pGamutTex,         L"SK HDR Gamut Coverage Texture");
      SK_D3D11_SetDebugName (pGamutUAV,         L"SK HDR Gamut Coverage UAV");
    }
  }
};

SK_LazyGlobal <SK_HDR_FIXUP> hdr_base;

SK_ComPtr <ID3D11Texture2D> SK_HDR_GetGamutTex     (void) { return hdr_base->pGamutTex;     }
SK_ComPtr <ID3D11Texture2D> SK_HDR_GetLuminanceTex (void) { return hdr_base->pLuminanceTex; }

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

void
SK_HDR_SanitizeFP16SwapChain (void)
{
  if (! __SK_HDR_16BitSwap)
    return;

  static auto& rb =
    SK_GetCurrentRenderBackend ();

  auto& vs_hdr_util  = hdr_base->VertexShaderHDR_Util;
  auto& ps_hdr_scrgb = hdr_base->PixelShader_scRGB;

  if ( vs_hdr_util.shader  != nullptr &&
       ps_hdr_scrgb.shader != nullptr &&
       hdr_base->pMainSrv  != nullptr )
  {
    auto pDev =
      rb.getDevice <ID3D11Device> ();

    SK_ComQIPtr <IDXGISwapChain>      pSwapChain (rb.swapchain);
    SK_ComQIPtr <ID3D11DeviceContext> pDevCtx    (rb.d3d11.immediate_ctx);

    if (pDev != nullptr     &&      pDevCtx == nullptr)
    {   pDev->GetImmediateContext (&pDevCtx.p); }

    if (! pDevCtx) return;

    SK_ComPtr <ID3D11Resource> pSrcRes = nullptr;
    SK_ComPtr <ID3D11Resource> pDst    = nullptr;

    SK_ComPtr <ID3D11RenderTargetView> pRTV;
    pDevCtx->OMGetRenderTargets   (1, &pRTV, nullptr);

    if (! pRTV.p) return;

    pRTV->GetResource                    (&pSrcRes);
    SK_ComQIPtr <ID3D11Texture2D> pSrcTex (pSrcRes);

    hdr_base->pMainSrv->GetResource ( &pDst.p );

    if (! pSrcTex.p) return;

    D3D11_TEXTURE2D_DESC texDesc = { };
    pSrcTex->GetDesc   (&texDesc);

    if (! pSwapChain.p)
      return;

    DXGI_SWAP_CHAIN_DESC  swapDesc = { };
    pSwapChain->GetDesc (&swapDesc);

    if (texDesc.Format != DXGI_FORMAT_R16G16B16A16_FLOAT ||
        texDesc.Format != DXGI_FORMAT_R32G32B32A32_FLOAT ||
        texDesc.Format != DXGI_FORMAT_R11G11B10_FLOAT)
      return;
    if (texDesc.Width  != swapDesc.BufferDesc.Width ||
        texDesc.Height != swapDesc.BufferDesc.Height)
      return;

    SK_ReleaseAssert (texDesc.Format == DXGI_FORMAT_R16G16B16A16_FLOAT);
    
    if (texDesc.SampleDesc.Count > 1)
      pDevCtx->ResolveSubresource ( pDst, 0, pSrcRes, 0, texDesc.Format );
    else
      pDevCtx->CopyResource       ( pDst,    pSrcRes );

    D3D11_MAPPED_SUBRESOURCE mapped_resource  = { };
    HDR_LUMINANCE            cbuffer_luma     = { };
    HDR_COLORSPACE_PARAMS    cbuffer_cspace   = { };

    cbuffer_luma.luminance_scale [0] =  __SK_HDR_Luma;
    cbuffer_luma.luminance_scale [1] =  __SK_HDR_Exp;
    cbuffer_luma.luminance_scale [2] = 1.0f;
    cbuffer_luma.luminance_scale [3] = 1.0f;

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

    cbuffer_cspace.uiToneMapper = 256; // Sanitize

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

//#define _ImGui_Stateblock

    SK_ComPtr <ID3D11RenderTargetView>  pRenderTargetView =
      pRTV;

    if (        pRenderTargetView.p != nullptr                    )//&&
         //swapDesc.BufferDesc.Format == DXGI_FORMAT_R16G16B16A16_FLOAT )
    {
#ifdef _ImGui_Stateblock
      SK_D3D11_CaptureStateBlock (pDevCtx, &snap_cache.sb);
#endif

      struct {
        SK_ComPtr <ID3D11VertexShader>      pVS;
        SK_ComPtr <ID3D11PixelShader>       pPS;
        SK_ComPtr <ID3D11GeometryShader>    pGS;
        SK_ComPtr <ID3D11DomainShader>      pDS;
        SK_ComPtr <ID3D11HullShader>        pHS;
      } shaders;

      struct {
        SK_ComPtr <ID3D11BlendState>        pBlendState;
        UINT                                uiBlendMask;
        FLOAT                               fBlendFactors [4] = { };
        SK_ComPtr <ID3D11RenderTargetView>  pRTVs    [D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];
        SK_ComPtr <ID3D11Buffer>            pStreams [D3D11_SO_BUFFER_SLOT_COUNT];
      } output;

      struct {
        D3D11_PRIMITIVE_TOPOLOGY            topology;
        SK_ComPtr <ID3D11InputLayout>       pLayout;
      } primitive_assembly;


      struct {
        SK_ComPtr <ID3D11Buffer>            pBuffer;
        DXGI_FORMAT                         format;
        UINT                                offset;
      } index_buffer;

      struct {
        SK_ComPtr <ID3D11DepthStencilView>  pView;
        SK_ComPtr <ID3D11DepthStencilState> pState;
        UINT                                uiRef;
      } depth_stencil;

      struct {
        UINT                            num_scissors = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
        D3D11_RECT                          scissors  [D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
        D3D11_VIEWPORT                      viewports [D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
        SK_ComPtr <ID3D11RasterizerState>   pState;
      } rasterizer;

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
          SK_ComPtr <ID3D11Buffer>          vs [D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT] = { };
          SK_ComPtr <ID3D11Buffer>          ps [D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT] = { };
        } cbuffers;
      } resources;

      pDevCtx->OMGetRenderTargets     (   D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT,                     &output.pRTVs [0].p,
                                                                                                   &depth_stencil.pView.p);
      pDevCtx->VSGetConstantBuffers   (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, &resources.cbuffers.vs [0].p);
      pDevCtx->PSGetConstantBuffers   (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, &resources.cbuffers.ps [0].p);
                                      
      pDevCtx->OMGetBlendState        (&output.pBlendState.p, output.fBlendFactors, &output.uiBlendMask);
                                      
      pDevCtx->VSGetShader            (&shaders.pVS.p, nullptr, nullptr);
      pDevCtx->PSGetShader            (&shaders.pPS.p, nullptr, nullptr);

      //D3D11_RENDER_TARGET_VIEW_DESC rtdesc               = {                           };
      //                              rtdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

      pDevCtx->PSGetShaderResources   (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, &resources.ps.original [0].p);
      pDevCtx->PSGetSamplers          (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT,        &resources.ps.samplers [0].p);

      pDevCtx->IAGetPrimitiveTopology (&primitive_assembly.topology);
      pDevCtx->IAGetInputLayout       (&primitive_assembly.pLayout.p);
      pDevCtx->IAGetIndexBuffer       (&index_buffer.pBuffer.p,
                                       &index_buffer.format,
                                       &index_buffer.offset);

      pDevCtx->IASetPrimitiveTopology (D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
      pDevCtx->IASetVertexBuffers     (0, 1, std::array <ID3D11Buffer *, 1> { nullptr }.data (),
                                             std::array <UINT,           1> { 0       }.data (),
                                             std::array <UINT,           1> { 0       }.data ());
      pDevCtx->IASetInputLayout       (hdr_base->pInputLayout);
      pDevCtx->IASetIndexBuffer       (nullptr, DXGI_FORMAT_UNKNOWN, 0);

      pDevCtx->OMSetBlendState        (hdr_base->pBlendState, nullptr, 0xFFFFFFFFUL);

      pDevCtx->VSSetShader            (       hdr_base->VertexShaderHDR_Util.shader, nullptr, 0);
      pDevCtx->VSSetConstantBuffers   (0, 1, &hdr_base->mainSceneCBuffer);

      //rtdesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
      pDevCtx->PSSetShader            (       hdr_base->PixelShader_scRGB.shader,    nullptr, 0);
      pDevCtx->PSSetConstantBuffers   (0, 1, &hdr_base->colorSpaceCBuffer);

      pDevCtx->RSGetState             (&rasterizer.pState.p);
      pDevCtx->RSSetState             (hdr_base->pRasterizerState);
      pDevCtx->RSGetScissorRects      (&rasterizer.num_scissors,
                                       &rasterizer.scissors [0]);
      pDevCtx->RSSetScissorRects      (0, nullptr);

       SK_ReleaseAssert (
         rasterizer.num_scissors <=
           D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE
       );

      pDevCtx->OMGetDepthStencilState ( &depth_stencil.pState.p,
                                        &depth_stencil.uiRef );
      pDevCtx->OMSetDepthStencilState (    hdr_base->pDSState, 0 );

      pDevCtx->OMSetRenderTargets     ( 1,
                                         &pRenderTargetView.p,
                                            nullptr );

      pDevCtx->PSSetShaderResources   (0, 2, &resources.ps.hdr [0].p);
      pDevCtx->PSSetSamplers          (0, 1, &hdr_base->pSampler0);

      if (pDev->GetFeatureLevel () >= D3D_FEATURE_LEVEL_10_0)
      {
        pDevCtx->GSGetShader          (&shaders.pGS.p, nullptr, nullptr);
        pDevCtx->GSSetShader          (nullptr,        nullptr,       0);
        pDevCtx->SOGetTargets         (D3D11_SO_BUFFER_SLOT_COUNT,
                                                 &output.pStreams [0].p);
        pDevCtx->SOSetTargets         (0,              nullptr, nullptr);

        if (pDev->GetFeatureLevel () >= D3D_FEATURE_LEVEL_11_0)
        {
          pDevCtx->HSGetShader        (&shaders.pHS.p, nullptr, nullptr);
          pDevCtx->DSGetShader        (&shaders.pDS.p, nullptr, nullptr);
          pDevCtx->HSSetShader        (nullptr,        nullptr,       0);
          pDevCtx->DSSetShader        (nullptr,        nullptr,       0);
        }
      }

      // -*- //

      pDevCtx->Draw (4, 0);

      // -*- //

      pDevCtx->PSSetShaderResources   (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT,
                                      &resources.ps.original [0].p);
      pDevCtx->PSSetSamplers          (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT,
                                      &resources.ps.samplers [0].p);
      pDevCtx->IASetPrimitiveTopology (primitive_assembly.topology);
      pDevCtx->IASetInputLayout       (primitive_assembly.pLayout);
      pDevCtx->IASetIndexBuffer       (index_buffer.pBuffer,
                                       index_buffer.format,
                                       index_buffer.offset);

      pDevCtx->PSSetShader            (shaders.pPS, nullptr, 0);
      pDevCtx->VSSetShader            (shaders.pVS, nullptr, 0);

      if (pDev->GetFeatureLevel () >= D3D_FEATURE_LEVEL_10_0)
      {
        pDevCtx->GSSetShader          (shaders.pGS, nullptr, 0);
        pDevCtx->SOSetTargets         (D3D11_SO_BUFFER_SLOT_COUNT,
                                       &output.pStreams [0].p, nullptr);

        if (pDev->GetFeatureLevel () >= D3D_FEATURE_LEVEL_11_0)
        {
          pDevCtx->HSSetShader        (shaders.pHS, nullptr, 0);
          pDevCtx->DSSetShader        (shaders.pDS, nullptr, 0);
        }
      }
      pDevCtx->VSSetConstantBuffers   (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT,
                                                       &resources.cbuffers.vs [0].p);
      pDevCtx->PSSetConstantBuffers   (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT,
                                                       &resources.cbuffers.ps [0].p);
      pDevCtx->RSSetScissorRects      (rasterizer.num_scissors, rasterizer.scissors);
      pDevCtx->RSSetState             (rasterizer.pState.p);
      pDevCtx->OMSetDepthStencilState (depth_stencil.pState.p,
                                       depth_stencil.uiRef);
      pDevCtx->OMSetRenderTargets     (D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, &output.pRTVs [0].p,
                                       depth_stencil.pView.p);
      pDevCtx->OMSetBlendState        (output.pBlendState,
                                       output.fBlendFactors,
                                       output.uiBlendMask);
#ifdef _ImGui_Stateblock
      SK_D3D11_ApplyStateBlock (snap_cache.sb, pDevCtx);
#endif
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


  struct snapshot_cache_s {
    ULONG64 lLastFrame = SK_GetFramesDrawn () - 1;
    ULONG64 lHDRFrames =                        0;

    SK_D3D11_Stateblock_Lite
                 *sb                      = nullptr;

    ID3D11Buffer *last_gpu_cbuffer_cspace = hdr_base->colorSpaceCBuffer;
    ID3D11Buffer *last_gpu_cbuffer_luma   = hdr_base->mainSceneCBuffer;
    uint32_t      last_hash_luma          = 0x0;
    uint32_t      last_hash_cspace        = 0x0;
  };

  static
    concurrency::concurrent_unordered_map <
    IUnknown *,
    snapshot_cache_s
  > snapshot_cache;


  auto& snap_cache =
    snapshot_cache [rb.swapchain];

  ULONG64 lThisFrame = SK_GetFramesDrawn ();
  if     (lThisFrame == snap_cache.lLastFrame) { return; }
  else               {  snap_cache.lLastFrame = lThisFrame; }

  auto& vs_hdr_util  = hdr_base->VertexShaderHDR_Util;
  auto& ps_hdr_scrgb = hdr_base->PixelShader_scRGB;

  if ( vs_hdr_util.shader  == nullptr ||
       ps_hdr_scrgb.shader == nullptr    )
  {
    if (snap_cache.lHDRFrames++ > 2) hdr_base->reloadResources ();
  }

  DXGI_SWAP_CHAIN_DESC swapDesc = { };

  if ( vs_hdr_util.shader  != nullptr &&
       ps_hdr_scrgb.shader != nullptr &&
       hdr_base->pMainSrv  != nullptr )
  {
    auto pDev =
      rb.getDevice <ID3D11Device> ();

    SK_ComQIPtr <IDXGISwapChain>      pSwapChain (rb.swapchain);
    SK_ComQIPtr <ID3D11DeviceContext> pDevCtx    (rb.d3d11.immediate_ctx);

    if (pDev != nullptr     &&      pDevCtx == nullptr)
    {   pDev->GetImmediateContext (&pDevCtx.p); }

    if (! pDevCtx) return;

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
        hdr_base->pMainSrv->GetResource ( &pDst.p );

        D3D11_TEXTURE2D_DESC texDesc = { };
        pSrc->GetDesc      (&texDesc);

        if (texDesc.SampleDesc.Count > 1)
          pDevCtx->ResolveSubresource ( pDst, 0, pSrc, 0, texDesc.Format );
        else
          pDevCtx->CopyResource       ( pDst,    pSrc );
      }
    }

    D3D11_MAPPED_SUBRESOURCE mapped_resource  = { };
    HDR_LUMINANCE            cbuffer_luma     = { };
    HDR_COLORSPACE_PARAMS    cbuffer_cspace   = { };

    if (snap_cache.last_gpu_cbuffer_cspace != hdr_base->colorSpaceCBuffer) {
        snap_cache.last_gpu_cbuffer_cspace =  hdr_base->colorSpaceCBuffer;
        snap_cache.last_hash_cspace        = 0x0;
    }

    if (snap_cache.last_gpu_cbuffer_luma != hdr_base->mainSceneCBuffer) {
        snap_cache.last_gpu_cbuffer_luma =  hdr_base->mainSceneCBuffer;
        snap_cache.last_hash_luma        = 0x0;
    }

    cbuffer_luma.luminance_scale [0] =  __SK_HDR_Luma;
    cbuffer_luma.luminance_scale [1] =  __SK_HDR_Exp;
    cbuffer_luma.luminance_scale [2] = (__SK_HDR_HorizCoverage / 100.0f) * 2.0f - 1.0f;
    cbuffer_luma.luminance_scale [3] = (__SK_HDR_VertCoverage  / 100.0f) * 2.0f - 1.0f;

    //uint32_t cb_hash_luma =
    //  crc32c (0x0, (const void *) &cbuffer_luma, sizeof SK_HDR_FIXUP::HDR_LUMINANCE);
    //
    ////if (cb_hash_luma != snap_cache.last_hash_luma)
    {
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
    }

    cbuffer_cspace.uiToneMapper           =   __SK_HDR_tonemap;
    cbuffer_cspace.hdrSaturation          =   __SK_HDR_Saturation;
    cbuffer_cspace.hdrPaperWhite          =   __SK_HDR_PaperWhite;
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

    //uint32_t cb_hash_cspace =
    //  crc32c (0x0, (const void *) &cbuffer_cspace, sizeof SK_HDR_FIXUP::HDR_COLORSPACE_PARAMS);
    //
    ////if (cb_hash_cspace != snap_cache.last_hash_cspace)
    {
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
    }

//#define _ImGui_Stateblock

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

#ifdef _ImGui_Stateblock
      SK_D3D11_CaptureStateBlock (pDevCtx, &snap_cache.sb);
#endif

      struct {
        SK_ComPtr <ID3D11VertexShader>      pVS;
        SK_ComPtr <ID3D11PixelShader>       pPS;
        SK_ComPtr <ID3D11GeometryShader>    pGS;
        SK_ComPtr <ID3D11DomainShader>      pDS;
        SK_ComPtr <ID3D11HullShader>        pHS;
      } shaders;

      struct {
        SK_ComPtr <ID3D11BlendState>        pBlendState;
        UINT                                uiBlendMask;
        FLOAT                               fBlendFactors [4] = { };
        SK_ComPtr <ID3D11RenderTargetView>  pRTVs    [D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];
        SK_ComPtr <ID3D11Buffer>            pStreams [D3D11_SO_BUFFER_SLOT_COUNT];
      } output;

      struct {
        D3D11_PRIMITIVE_TOPOLOGY            topology;
        SK_ComPtr <ID3D11InputLayout>       pLayout;
      } primitive_assembly;


      struct {
        SK_ComPtr <ID3D11Buffer>            pBuffer;
        DXGI_FORMAT                         format;
        UINT                                offset;
      } index_buffer;

      struct {
        SK_ComPtr <ID3D11DepthStencilView>  pView;
        SK_ComPtr <ID3D11DepthStencilState> pState;
        UINT                                uiRef;
      } depth_stencil;

      struct {
        UINT                            num_scissors = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
        D3D11_RECT                          scissors  [D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
        D3D11_VIEWPORT                      viewports [D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
        SK_ComPtr <ID3D11RasterizerState>   pState;
      } rasterizer;

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
          SK_ComPtr <ID3D11Buffer>          vs [D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT] = { };
          SK_ComPtr <ID3D11Buffer>          ps [D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT] = { };
        } cbuffers;
      } resources;

      pDevCtx->OMGetRenderTargets     (   D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT,                     &output.pRTVs [0].p,
                                                                                                   &depth_stencil.pView.p);
      pDevCtx->VSGetConstantBuffers   (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, &resources.cbuffers.vs [0].p);
      pDevCtx->PSGetConstantBuffers   (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, &resources.cbuffers.ps [0].p);
                                      
      pDevCtx->OMGetBlendState        (&output.pBlendState.p, output.fBlendFactors, &output.uiBlendMask);
                                      
      pDevCtx->VSGetShader            (&shaders.pVS.p, nullptr, nullptr);
      pDevCtx->PSGetShader            (&shaders.pPS.p, nullptr, nullptr);

      //D3D11_RENDER_TARGET_VIEW_DESC rtdesc               = {                           };
      //                              rtdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

      pDevCtx->PSGetShaderResources   (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, &resources.ps.original [0].p);
      pDevCtx->PSGetSamplers          (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT,        &resources.ps.samplers [0].p);

      pDevCtx->IAGetPrimitiveTopology (&primitive_assembly.topology);
      pDevCtx->IAGetInputLayout       (&primitive_assembly.pLayout.p);
      pDevCtx->IAGetIndexBuffer       (&index_buffer.pBuffer.p,
                                       &index_buffer.format,
                                       &index_buffer.offset);

      pDevCtx->IASetPrimitiveTopology (D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
      pDevCtx->IASetVertexBuffers     (0, 1, std::array <ID3D11Buffer *, 1> { nullptr }.data (),
                                             std::array <UINT,           1> { 0       }.data (),
                                             std::array <UINT,           1> { 0       }.data ());
      pDevCtx->IASetInputLayout       (hdr_base->pInputLayout);
      pDevCtx->IASetIndexBuffer       (nullptr, DXGI_FORMAT_UNKNOWN, 0);

      pDevCtx->OMSetBlendState        (hdr_base->pBlendState, output.fBlendFactors,
                                         output.uiBlendMask);

      pDevCtx->VSSetShader            (       hdr_base->VertexShaderHDR_Util.shader, nullptr, 0);
      pDevCtx->VSSetConstantBuffers   (0, 1, &hdr_base->mainSceneCBuffer);

      //rtdesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
      pDevCtx->PSSetShader            (       hdr_base->PixelShader_scRGB.shader,    nullptr, 0);
      pDevCtx->PSSetConstantBuffers   (0, 1, &hdr_base->colorSpaceCBuffer);

      pDevCtx->RSGetState             (&rasterizer.pState.p);
      pDevCtx->RSSetState             (hdr_base->pRasterizerState);
      pDevCtx->RSGetScissorRects      (&rasterizer.num_scissors,
                                       &rasterizer.scissors [0]);
      pDevCtx->RSSetScissorRects      (0, nullptr);

       SK_ReleaseAssert (
         rasterizer.num_scissors <=
           D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE
       );

      pDevCtx->OMGetDepthStencilState ( &depth_stencil.pState.p,
                                        &depth_stencil.uiRef );
      pDevCtx->OMSetDepthStencilState (    hdr_base->pDSState,
                                         depth_stencil.uiRef );

      pDevCtx->OMSetRenderTargets     ( 1,
                                         &pRenderTargetView.p,
                                            nullptr );

      pDevCtx->PSSetShaderResources   (0, 2, &resources.ps.hdr [0].p);
      pDevCtx->PSSetSamplers          (0, 1, &hdr_base->pSampler0);

      if (pDev->GetFeatureLevel () >= D3D_FEATURE_LEVEL_10_0)
      {
        pDevCtx->GSGetShader          (&shaders.pGS.p, nullptr, nullptr);
        pDevCtx->GSSetShader          (nullptr,        nullptr,       0);
        pDevCtx->SOGetTargets         (D3D11_SO_BUFFER_SLOT_COUNT,
                                                 &output.pStreams [0].p);
        pDevCtx->SOSetTargets         (0,              nullptr, nullptr);

        if (pDev->GetFeatureLevel () >= D3D_FEATURE_LEVEL_11_0)
        {
          pDevCtx->HSGetShader        (&shaders.pHS.p, nullptr, nullptr);
          pDevCtx->DSGetShader        (&shaders.pDS.p, nullptr, nullptr);
          pDevCtx->HSSetShader        (nullptr,        nullptr,       0);
          pDevCtx->DSSetShader        (nullptr,        nullptr,       0);
        }
      }

      // -*- //

      pDevCtx->Draw (4, 0);

      if (__SK_HDR_AdaptiveToneMap)
      {
        D3D11_UNORDERED_ACCESS_VIEW_DESC
          uav_desc                    = { };
          uav_desc.ViewDimension      = D3D11_UAV_DIMENSION_TEXTURE2D;
          uav_desc.Texture2D.MipSlice = 0;
          uav_desc.Format             = DXGI_FORMAT_R16G16B16A16_FLOAT;

        SK_ComPtr <ID3D11UnorderedAccessView>                pFramebufferUAV;
        pDev->CreateUnorderedAccessView (pSrc.p, &uav_desc, &pFramebufferUAV.p);

        if (pFramebufferUAV.p != nullptr)
        {
          static ID3D11ShaderResourceView* const
            nul_srvs [D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = { };
          static ID3D11RenderTargetView* const
            nul_rtvs [D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT]       = { };
          static ID3D11UnorderedAccessView* const
            nul_uavs [D3D11_1_UAV_SLOT_COUNT]                       = { };

          ID3D11UnorderedAccessView* uavs [] =
          {         pFramebufferUAV.p,
            hdr_base->pLuminanceUAV,
                hdr_base->pGamutUAV
          };

          //pDevCtx->ClearUnorderedAccessViewFloat (hdr_base->pLuminanceUAV, std::array <FLOAT, 4> { 0.0f, 0.0f, 0.0f, 0.0f }.data ());

          pDevCtx->OMSetRenderTargets                          (   D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT,       nul_rtvs, nullptr);
          //pDevCtx->OMSetRenderTargetsAndUnorderedAccessViews (0, nullptr,
          //                                                       nullptr, 0, D3D11_1_UAV_SLOT_COUNT,           nul_uavs, nullptr);

          pDevCtx->PSSetShaderResources      (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, nul_srvs);
          pDevCtx->PSSetConstantBuffers      (0, 1,  std::array <ID3D11Buffer *,1> { nullptr }.data () );
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

#define	D3D11_UAV_SLOT_COUNT	( 8 )
          if (pDev->GetFeatureLevel () >= D3D_FEATURE_LEVEL_11_1)
          pDevCtx->CSSetUnorderedAccessViews (0, D3D11_1_UAV_SLOT_COUNT, nul_uavs,           nullptr   );
          else
          pDevCtx->CSSetUnorderedAccessViews (0, D3D11_UAV_SLOT_COUNT,   nul_uavs,           nullptr   );
          pDevCtx->CSSetConstantBuffers      (0, 1,  std::array <ID3D11Buffer *,1> { nullptr }.data () );
        }
      }

      if (pDevCtx3.p != nullptr)
      {
        pDevCtx3->ClearView (hdr_base->pMainSrv, std::array <FLOAT, 4> {0.0f, 0.0f, 0.0f, 1.0f}.data (), nullptr, 0);

      //if (depth_stencil.pView.p != nullptr)
      //  pDevCtx->ClearDepthStencilView (depth_stencil.pView.p, D3D11_CLEAR_DEPTH |
      //                                                         D3D11_CLEAR_STENCIL, 0.0f, 0x0);
      }

      // -*- //

      pDevCtx->PSSetShaderResources   (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT,
                                      &resources.ps.original [0].p);
      pDevCtx->PSSetSamplers          (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT,
                                      &resources.ps.samplers [0].p);
      pDevCtx->IASetPrimitiveTopology (primitive_assembly.topology);
      pDevCtx->IASetInputLayout       (primitive_assembly.pLayout);
      pDevCtx->IASetIndexBuffer       (index_buffer.pBuffer,
                                       index_buffer.format,
                                       index_buffer.offset);

      pDevCtx->PSSetShader            (shaders.pPS, nullptr, 0);
      pDevCtx->VSSetShader            (shaders.pVS, nullptr, 0);

      if (pDev->GetFeatureLevel () >= D3D_FEATURE_LEVEL_10_0)
      {
        pDevCtx->GSSetShader          (shaders.pGS, nullptr, 0);
        pDevCtx->SOSetTargets         (D3D11_SO_BUFFER_SLOT_COUNT,
                                       &output.pStreams [0].p, nullptr);

        if (pDev->GetFeatureLevel () >= D3D_FEATURE_LEVEL_11_0)
        {
          pDevCtx->HSSetShader        (shaders.pHS, nullptr, 0);
          pDevCtx->DSSetShader        (shaders.pDS, nullptr, 0);
        }
      }
      pDevCtx->VSSetConstantBuffers   (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT,
                                                       &resources.cbuffers.vs [0].p);
      pDevCtx->PSSetConstantBuffers   (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT,
                                                       &resources.cbuffers.ps [0].p);
      pDevCtx->RSSetScissorRects      (rasterizer.num_scissors, rasterizer.scissors);
      pDevCtx->RSSetState             (rasterizer.pState.p);
      pDevCtx->OMSetDepthStencilState (depth_stencil.pState.p,
                                       depth_stencil.uiRef);
      pDevCtx->OMSetRenderTargets     (D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, &output.pRTVs [0].p,
                                       depth_stencil.pView.p);
      pDevCtx->OMSetBlendState        (output.pBlendState,
                                       output.fBlendFactors,
                                       output.uiBlendMask);

                              ++snap_cache.lHDRFrames;
#ifdef _ImGui_Stateblock
      SK_D3D11_ApplyStateBlock (snap_cache.sb, pDevCtx);
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