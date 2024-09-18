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

#include <SpecialK/render/d3d11/d3d11_screenshot.h>

#include <ranges>
#include <valarray>

#ifndef __SK_SUBSYSTEM__
#define __SK_SUBSYSTEM__ L"D3D11SShot"
#endif

extern volatile LONG  SK_D3D11_DrawTrackingReqs;

SK_D3D11_Screenshot& SK_D3D11_Screenshot::operator= (SK_D3D11_Screenshot&& moveFrom)
{
  if (this != &moveFrom)
  {
    dispose ();

    if (moveFrom.pPixelBufferFence.p != nullptr)
    {
      pDev.p                         = std::exchange (moveFrom.pDev.p,                   nullptr);
      pImmediateCtx.p                = std::exchange (moveFrom.pImmediateCtx.p,          nullptr);
      pSwapChain.p                   = std::exchange (moveFrom.pSwapChain.p,             nullptr);
      pBackbufferSurface.p           = std::exchange (moveFrom.pBackbufferSurface.p,     nullptr);
      pStagingBackbufferCopy.p       = std::exchange (moveFrom.pStagingBackbufferCopy.p, nullptr);
      pPixelBufferFence.p            = std::exchange (moveFrom.pPixelBufferFence.p,      nullptr);
    }

    auto&& fromBuffer =
      moveFrom.framebuffer;

    framebuffer.Width                = std::exchange (fromBuffer.Width,                    0);
    framebuffer.Height               = std::exchange (fromBuffer.Height,                   0);
    framebuffer.PackedDstPitch       = std::exchange (fromBuffer.PackedDstPitch,           0);
    framebuffer.PackedDstSlicePitch  = std::exchange (fromBuffer.PackedDstSlicePitch,      0);
    framebuffer.PBufferSize          = std::exchange (fromBuffer.PBufferSize,              0);

    framebuffer.AllowCopyToClipboard = moveFrom.bCopyToClipboard;
    framebuffer.AllowSaveToDisk      = moveFrom.bSaveToDisk;
                                       std::exchange (fromBuffer.AllowCopyToClipboard, false);
                                       std::exchange (fromBuffer.AllowSaveToDisk,      false);

    framebuffer.PixelBuffer.reset (
     fromBuffer.PixelBuffer.get ()
    );

    framebuffer.dxgi.AlphaMode       = std::exchange (fromBuffer.dxgi.AlphaMode,    DXGI_ALPHA_MODE_UNSPECIFIED);
    framebuffer.dxgi.NativeFormat    = std::exchange (fromBuffer.dxgi.NativeFormat, DXGI_FORMAT_UNKNOWN);

    framebuffer.hdr.avg_cll_nits     = std::exchange (fromBuffer.hdr.avg_cll_nits, 0.0f);
    framebuffer.hdr.max_cll_nits     = std::exchange (fromBuffer.hdr.max_cll_nits, 0.0f);
    framebuffer.file_name            = std::exchange (fromBuffer.file_name,         L"");
    framebuffer.title                = std::exchange (fromBuffer.title,              "");

    bPlaySound                       = moveFrom.bPlaySound;
    bSaveToDisk                      = moveFrom.bSaveToDisk;
    bCopyToClipboard                 = moveFrom.bCopyToClipboard;

    ulCommandIssuedOnFrame           = std::exchange (moveFrom.ulCommandIssuedOnFrame, 0);
  }

  return *this;
}

static std::string _SpecialNowhere;

template <typename _ShaderType>
struct ShaderBase
{
  _ShaderType *shader           = nullptr;
  ID3D10Blob  *amorphousTheBlob = nullptr;

  /////
  /////bool loadPreBuiltShader ( ID3D11Device* pDev, const BYTE pByteCode [] )
  /////{
  /////  HRESULT hrCompile =
  /////    E_NOTIMPL;
  /////
  /////  if ( std::type_index ( typeid (    _ShaderType   ) ) ==
  /////       std::type_index ( typeid (ID3D11VertexShader) ) )
  /////  {
  /////    hrCompile =
  /////      pDev->CreateVertexShader (
  /////                pByteCode,
  /////        sizeof (pByteCode), nullptr,
  /////        reinterpret_cast <ID3D11VertexShader **>(&shader)
  /////      );
  /////  }
  /////
  /////  else if ( std::type_index ( typeid (   _ShaderType   ) ) ==
  /////            std::type_index ( typeid (ID3D11PixelShader) ) )
  /////  {
  /////    hrCompile =
  /////      pDev->CreatePixelShader (
  /////                pByteCode,
  /////        sizeof (pByteCode), nullptr,
  /////        reinterpret_cast <ID3D11PixelShader **>(&shader)
  /////      );
  /////  }
  /////
  /////  return
  /////    SUCCEEDED (hrCompile);
  /////}

  bool compileShaderString ( ID3D11Device*  pDev,
                             const char*    szShaderString,
                             const wchar_t* wszShaderName,
                             const char*    szEntryPoint,
                             const char*    szShaderModel,
                                   bool     recompile =
                                              false,
                               std::string& error_log =
                                              _SpecialNowhere )
  {
    UNREFERENCED_PARAMETER (recompile);
    UNREFERENCED_PARAMETER (error_log);

    SK_ComPtr <ID3D10Blob>
      amorphousTheMessenger;

    HRESULT hr =
      SK_D3D_Compile ( szShaderString,
                         strlen (szShaderString),
                           nullptr, nullptr, nullptr,
                             szEntryPoint, szShaderModel,
                               0, 0,
                                 &amorphousTheBlob,
                                   &amorphousTheMessenger );

    if (FAILED (hr))
    {
      if ( amorphousTheMessenger != nullptr     &&
           amorphousTheMessenger->GetBufferSize () > 0 )
      {
        std::string err;
                    err.reserve (
                      amorphousTheMessenger->GetBufferSize ()
                    );
        err = (
          (char *)amorphousTheMessenger->GetBufferPointer ());

        if (! err.empty ())
        {
          dll_log->LogEx ( true,
                             L"SK D3D11 Shader (SM=%hs) [%ws]: %hs",
                               szShaderModel, wszShaderName,
                                 err.c_str ()
                         );
        }
      }

      return false;
    }

    HRESULT hrCompile =
      DXGI_ERROR_NOT_CURRENTLY_AVAILABLE;

    if ( pDev != nullptr &&
         std::type_index ( typeid (    _ShaderType   ) ) ==
         std::type_index ( typeid (ID3D11VertexShader) ) )
    {
      hrCompile =
        pDev->CreateVertexShader (
           static_cast <DWORD *> ( amorphousTheBlob->GetBufferPointer () ),
                                   amorphousTheBlob->GetBufferSize    (),
                                     nullptr,
           reinterpret_cast <ID3D11VertexShader **>(&shader)
                                 );
    }

    else if ( pDev != nullptr &&
              std::type_index ( typeid (   _ShaderType   ) ) ==
              std::type_index ( typeid (ID3D11PixelShader) ) )
    {
      hrCompile =
        pDev->CreatePixelShader  (
           static_cast <DWORD *> ( amorphousTheBlob->GetBufferPointer () ),
                                   amorphousTheBlob->GetBufferSize    (),
                                     nullptr,
           reinterpret_cast <ID3D11PixelShader **>(&shader)
                                 );
    }

    else
    {
#pragma warning (push)
#pragma warning (disable: 4130) // No @#$% sherlock
      SK_ReleaseAssert ("WTF?!" == nullptr);
#pragma warning (pop)
    }

    return
      SUCCEEDED (hrCompile);
  }

  bool compileShaderFile ( ID3D11Device*  pDev,
                           const wchar_t* wszShaderFile,
                           const    char* szEntryPoint,
                           const    char* szShaderModel,
                                    bool  recompile =
                                            false,
                             std::string& error_log =
                                            _SpecialNowhere )
  {
    std::wstring wstr_file =
      wszShaderFile;

    SK_ConcealUserDir (
      wstr_file.data ()
    );

    if ( GetFileAttributesW (wszShaderFile) == INVALID_FILE_ATTRIBUTES )
    {
      static Concurrency::concurrent_unordered_set <std::wstring> warned_shaders;

      if (! warned_shaders.count (wszShaderFile))
      {
        warned_shaders.insert (wszShaderFile);

        SK_LOG0 ( ( L"Shader Compile Failed: File '%ws' Is Not Valid!",
                      wstr_file.c_str ()
                  ),L"D3DCompile" );
      }

      return false;
    }

    FILE* fShader =
      _wfsopen ( wszShaderFile, L"rtS",
                   _SH_DENYNO );

    if (fShader != nullptr)
    {
      uint64_t size =
        SK_File_GetSize (wszShaderFile);

      auto data =
        std::make_unique <char> (static_cast <size_t> (size) + 32);

      memset ( data.get (),
                 0,
        sk::narrow_cast <size_t> (size) + 32 );

      fread  ( data.get (),
                 1,
        sk::narrow_cast <size_t> (size) + 2,
                                     fShader );

      fclose (fShader);

      const bool result =
        compileShaderString ( pDev,
                              data.get (), wstr_file.c_str (),
                                szEntryPoint, szShaderModel,
                                  recompile,  error_log );

      return result;
    }

    SK_LOG0 ( (L"Cannot Compile Shader Because of Filesystem Issues"),
               L"D3DCompile" );

    return  false;
  }

  void releaseResources (void)
  {
    if (shader           != nullptr) {           shader->Release (); shader           = nullptr; }
    if (amorphousTheBlob != nullptr) { amorphousTheBlob->Release (); amorphousTheBlob = nullptr; }
  }
};

SK_D3D11_Screenshot::SK_D3D11_Screenshot (const SK_ComPtr <ID3D11Device>& pDevice, bool allow_sound = true, bool clipboard_only, std::string title) : pDev (pDevice), SK_Screenshot (clipboard_only)
{
  if (! title.empty ())
  {
    framebuffer.file_name = SK_UTF8ToWideChar (title);
    framebuffer.title     = title;

    PathStripPathA (framebuffer.title.data ());

    sanitizeFilename (true);
  }

  bPlaySound = allow_sound;

  SK_ScopedBool decl_tex_scope (
    SK_D3D11_DeclareTexInjectScope ()
  );

  auto& io =
    ImGui::GetIO ();

  if (pDev != nullptr)
  {
    SK_RenderBackend_V2 &rb =
      SK_GetCurrentRenderBackend ();

    auto pTestDev =
        rb.getDevice <ID3D11Device> ();

    if (! ( pTestDev.IsEqualObject (pDev) ) &&
            rb.d3d11.immediate_ctx != nullptr
       )
    {
      pDev->GetImmediateContext (
             &pImmediateCtx.p
      );
    }

    else
      pImmediateCtx = rb.d3d11.immediate_ctx;

    SK_ComQIPtr <ID3D11DeviceContext3> pImmediateCtx3 (pImmediateCtx);

    if ( pImmediateCtx3 != nullptr &&
         pImmediateCtx3 != pImmediateCtx )
    {    pImmediateCtx   = pImmediateCtx3; }

    D3D11_QUERY_DESC fence_query_desc =
    {
      D3D11_QUERY_EVENT,
      0x00
    };

    if ( SUCCEEDED ( pDev->CreateQuery ( &fence_query_desc,
                             &pPixelBufferFence.p
                                       )
                   )
       )
    {
      if (rb.swapchain != nullptr)
          rb.swapchain->QueryInterface <IDXGISwapChain> (&pSwapChain);

      if (pSwapChain != nullptr)
      {
        SK_ComQIPtr <IDXGISwapChain4> pSwap4 (pSwapChain);

        if (pSwap4 != nullptr)
        {
          DXGI_SWAP_CHAIN_DESC1 sd1 = {};
          pSwap4->GetDesc1    (&sd1);

          framebuffer.dxgi.AlphaMode =
                       sd1.AlphaMode;
        }
        ulCommandIssuedOnFrame = SK_GetFramesDrawn ();

        if ( SUCCEEDED ( pSwapChain->GetBuffer ( 0,
                           __uuidof (ID3D11Texture2D),
                             (void **)&pBackbufferSurface.p
                                               )
                       )
           )
        {
          bool hdr10_to_scRGB = false;
          SK_ComPtr <ID3D11Texture2D> pHDRConvertTex;

          D3D11_TEXTURE2D_DESC          backbuffer_desc = { };
          pBackbufferSurface->GetDesc (&backbuffer_desc);

          framebuffer.Width             = backbuffer_desc.Width;
          framebuffer.Height            = backbuffer_desc.Height;
          framebuffer.dxgi.NativeFormat = backbuffer_desc.Format;

          bool hdr =
            (rb.isHDRCapable () &&
             rb.isHDRActive  ());

          if (hdr && backbuffer_desc.Format == DXGI_FORMAT_R10G10B10A2_UNORM)
          {
            D3D11_TEXTURE2D_DESC tex_desc = { };
            tex_desc.Width          = framebuffer.Width;
            tex_desc.Height         = framebuffer.Height;
            tex_desc.Format         = DXGI_FORMAT_R16G16B16A16_FLOAT;
            tex_desc.MipLevels      = 1;
            tex_desc.ArraySize      = 1;
            tex_desc.SampleDesc     = { 1, 0 };
            tex_desc.BindFlags      = D3D11_BIND_RENDER_TARGET |
                                      D3D11_BIND_SHADER_RESOURCE;
            tex_desc.Usage          = D3D11_USAGE_DEFAULT;
            tex_desc.CPUAccessFlags = 0x0;

            if ( SUCCEEDED (
                   pDev->CreateTexture2D (&tex_desc, nullptr, &pHDRConvertTex.p)
                 )
               )
            {
              D3D11_RENDER_TARGET_VIEW_DESC rtdesc
                = { };

              rtdesc.Format             = DXGI_FORMAT_R16G16B16A16_FLOAT;
              rtdesc.ViewDimension      = D3D11_RTV_DIMENSION_TEXTURE2D;
              rtdesc.Texture2D.MipSlice = 0;

              SK_ComPtr <ID3D11RenderTargetView> pRenderTargetView;

              if (SUCCEEDED (pDev->CreateRenderTargetView (pHDRConvertTex, &rtdesc, &pRenderTargetView.p)))
              {
                D3DX11_STATE_BLOCK sbs = { };

                auto *sb =
                  (D3DX11_STATE_BLOCK *)&sbs;

                CreateStateblock (pImmediateCtx, sb);

                DXGI_SWAP_CHAIN_DESC swapDesc = { };
                D3D11_TEXTURE2D_DESC desc     = { };

                pSwapChain->GetDesc (&swapDesc);

                desc.Width            = swapDesc.BufferDesc.Width;
                desc.Height           = swapDesc.BufferDesc.Height;
                desc.MipLevels        = 1;
                desc.ArraySize        = 1;
                desc.Format           = swapDesc.BufferDesc.Format;
                desc.SampleDesc.Count = 1;
                desc.Usage            = D3D11_USAGE_DEFAULT;
                desc.BindFlags        = D3D11_BIND_SHADER_RESOURCE;
                desc.CPUAccessFlags   = 0;

                static concurrency::concurrent_unordered_map <ID3D11Device*, ShaderBase <ID3D11PixelShader>>  PixelShader_HDR10toscRGB;
                static concurrency::concurrent_unordered_map <ID3D11Device*, ShaderBase <ID3D11VertexShader>> VertexShaderHDR_Util;

              //static std::wstring debug_shader_dir = SK_GetConfigPath ();

                bool compiled = true;

                if (! PixelShader_HDR10toscRGB.count (pDev))
                {
                  compiled =
                  PixelShader_HDR10toscRGB [pDev].compileShaderString ( pDev,
                    "#pragma warning ( disable : 3571 )                  \n\
                    struct PS_INPUT                                      \n\
                    {                                                    \n\
                      float4 pos      : SV_POSITION;                     \n\
                      float4 color    : COLOR0;                          \n\
                      float2 uv       : TEXCOORD0;                       \n\
                      float2 coverage : TEXCOORD1;                       \n\
                    };                                                   \n\
                                                                         \n\
                    sampler   sampler0 : register (s0);                  \n\
                    Texture2D texHDR10 : register (t0);                  \n\
                                                                         \n\
                    static const double3x3 from2020to709 = {             \n\
                       1.660496, -0.587656, -0.072840,                   \n\
                      -0.124546,  1.132895,  0.008348,                   \n\
                      -0.018154, -0.100597,  1.118751                    \n\
                    };                                                   \n\
                                                                         \n\
                    double3 RemoveREC2084Curve (double3 N)               \n\
                    {                                                    \n\
                      double  m1 = 2610.0 / 4096.0 / 4;                  \n\
                      double  m2 = 2523.0 / 4096.0 * 128;                \n\
                      double  c1 = 3424.0 / 4096.0;                      \n\
                      double  c2 = 2413.0 / 4096.0 * 32;                 \n\
                      double  c3 = 2392.0 / 4096.0 * 32;                 \n\
                      double3 Np = pow (N, 1 / m2);                      \n\
                                                                         \n\
                      return                                             \n\
                        pow (max (Np - c1, 0) / (c2 - c3 * Np), 1 / m1); \n\
                    }                                                    \n\
                                                                         \n\
                    float4 main ( PS_INPUT input ) : SV_TARGET           \n\
                    {                                                    \n\
                      double4 hdr10_color =                              \n\
                        texHDR10.Sample (sampler0, input.uv);            \n\
                                                                         \n\
                      // HDR10 (normalized) is 125x brighter than scRGB  \n\
                      hdr10_color.rgb =                                  \n\
                        125.0 *                                          \n\
                          mul ( from2020to709,                           \n\
                                  RemoveREC2084Curve ( hdr10_color.rgb ) \n\
                              );                                         \n\
                                                                         \n\
                      return                                             \n\
                        float4 (hdr10_color.rgb, 1.0);                   \n\
                    }", L"HDR10->scRGB Color Transform", "main", "ps_5_0", true);
                };

                if (! VertexShaderHDR_Util.count (pDev))
                {
                  compiled = compiled &&
                  VertexShaderHDR_Util [pDev].compileShaderString (pDev,
                    "cbuffer vertexBuffer : register (b0)       \n\
                    {                                           \n\
                      float4 Luminance;                         \n\
                    };                                          \n\
                                                                \n\
                    struct PS_INPUT                             \n\
                    {                                           \n\
                      float4 pos      : SV_POSITION;            \n\
                      float4 col      : COLOR0;                 \n\
                      float2 uv       : TEXCOORD0;              \n\
                      float2 coverage : TEXCOORD1;              \n\
                    };                                          \n\
                                                                \n\
                    struct VS_INPUT                             \n\
                    {                                           \n\
                      uint vI : SV_VERTEXID;                    \n\
                    };                                          \n\
                                                                \n\
                    PS_INPUT main ( VS_INPUT input )            \n\
                    {                                           \n\
                      PS_INPUT                                  \n\
                        output;                                 \n\
                                                                \n\
                        output.uv  = float2 (input.vI  & 1,     \n\
                                             input.vI >> 1);    \n\
                        output.col = float4 (Luminance.rgb,1);  \n\
                        output.pos =                            \n\
                          float4 ( ( output.uv.x - 0.5f ) * 2,  \n\
                                  -( output.uv.y - 0.5f ) * 2,  \n\
                                                   0.0f,        \n\
                                                   1.0f );      \n\
                                                                \n\
                        output.coverage =                       \n\
                          float2 ( (Luminance.z * .5f + .5f),   \n\
                                   (Luminance.w * .5f + .5f) ); \n\
                                                                \n\
                      return                                    \n\
                        output;                                 \n\
                    }", L"HDR Color Utility Vertex Shader",
                         "main", "vs_5_0", true );
                }

                if (     VertexShaderHDR_Util.count (pDev) &&
                     PixelShader_HDR10toscRGB.count (pDev) && compiled )
                {
                  SK_ComPtr <ID3D11Texture2D>          pHDR10Texture = nullptr;
                  SK_ComPtr <ID3D11ShaderResourceView> pHDR10Srv     = nullptr;

                  D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = { };

                  srvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
                  srvDesc.Format                    = desc.Format;
                  srvDesc.Texture2D.MipLevels       = desc.MipLevels;
                  srvDesc.Texture2D.MostDetailedMip = 0;

                  pDev->CreateTexture2D          ( &desc, nullptr,
                                                   &pHDR10Texture.p );
                  pImmediateCtx->CopyResource    ( pHDR10Texture,
                                                     pBackbufferSurface );

                  pDev->CreateShaderResourceView (pHDR10Texture, &srvDesc, &pHDR10Srv.p);

                  ID3D11ShaderResourceView* pResources [1] = { pHDR10Srv };

                  pImmediateCtx->IASetPrimitiveTopology (D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

                  static const FLOAT                      fBlendFactor [4] =
                                                      { 0.0f, 0.0f, 0.0f, 1.0f };
                  pImmediateCtx->VSSetShader          (VertexShaderHDR_Util     [pDev].shader, nullptr, 0);
                  pImmediateCtx->PSSetShader          (PixelShader_HDR10toscRGB [pDev].shader, nullptr, 0);
                  pImmediateCtx->GSSetShader          (nullptr,                         nullptr, 0);

                  if (pDev->GetFeatureLevel () >= D3D_FEATURE_LEVEL_11_0)
                  {
                    pImmediateCtx->HSSetShader        (nullptr,                         nullptr, 0);
                    pImmediateCtx->DSSetShader        (nullptr,                         nullptr, 0);
                  }

                  pImmediateCtx->PSSetShaderResources (0, 1, pResources);
                  pImmediateCtx->OMSetRenderTargets   (1, &pRenderTargetView.p, nullptr);

                  static D3D11_RASTERIZER_DESC    raster_desc = { .FillMode        = D3D11_FILL_SOLID,
                                                                  .CullMode        = D3D11_CULL_NONE,
                                                                  .DepthClipEnable = TRUE,
                                                                  .ScissorEnable   = FALSE };
                  static D3D11_DEPTH_STENCIL_DESC depth_desc  = { .DepthEnable     = FALSE,
                                                                  .DepthWriteMask  = D3D11_DEPTH_WRITE_MASK_ALL,
                                                                  .DepthFunc       = D3D11_COMPARISON_ALWAYS,
                                                                  .StencilEnable   = FALSE,
                                                                  .FrontFace = { .StencilFailOp      = D3D11_STENCIL_OP_KEEP,
                                                                                 .StencilDepthFailOp = D3D11_STENCIL_OP_KEEP,
                                                                                 .StencilPassOp      = D3D11_STENCIL_OP_KEEP,
                                                                                 .StencilFunc        = D3D11_COMPARISON_ALWAYS
                                                                               },
                                                                  .BackFace  = { .StencilFailOp      = D3D11_STENCIL_OP_KEEP,
                                                                                 .StencilDepthFailOp = D3D11_STENCIL_OP_KEEP,
                                                                                 .StencilPassOp      = D3D11_STENCIL_OP_KEEP,
                                                                                 .StencilFunc        = D3D11_COMPARISON_ALWAYS
                                                                               }
                                                                };
                  static D3D11_BLEND_DESC
                    blend_desc  = { };
                    blend_desc.AlphaToCoverageEnable                  = FALSE;
                    blend_desc.RenderTarget [0].BlendEnable           = TRUE;
                    blend_desc.RenderTarget [0].SrcBlend              = D3D11_BLEND_ONE;
                    blend_desc.RenderTarget [0].DestBlend             = D3D11_BLEND_ZERO;
                    blend_desc.RenderTarget [0].BlendOp               = D3D11_BLEND_OP_ADD;
                    blend_desc.RenderTarget [0].SrcBlendAlpha         = D3D11_BLEND_ONE;
                    blend_desc.RenderTarget [0].DestBlendAlpha        = D3D11_BLEND_ZERO;
                    blend_desc.RenderTarget [0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
                    blend_desc.RenderTarget [0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

                  SK_ComPtr <ID3D11RasterizerState>                     pRasterizerState;
                  pDev->CreateRasterizerState           (&raster_desc, &pRasterizerState);
                  SK_ComPtr <ID3D11DepthStencilState>                   pDepthStencilState;
                  pDev->CreateDepthStencilState         (&depth_desc,  &pDepthStencilState);
                  SK_ComPtr <ID3D11BlendState>                          pBlendState;
                  pDev->CreateBlendState                (&blend_desc,  &pBlendState);

                  pImmediateCtx->OMSetDepthStencilState (pDepthStencilState, 0);
                  pImmediateCtx->RSSetState             (pRasterizerState     );
                  pImmediateCtx->OMSetBlendState        (pBlendState,
                                                         fBlendFactor, 0xFFFFFFFF);

                  D3D11_VIEWPORT vp = { };

                  vp.Height   = io.DisplaySize.y;
                  vp.Width    = io.DisplaySize.x;
                  vp.MinDepth = 0.0f;
                  vp.MaxDepth = 1.0f;
                  vp.TopLeftX = vp.TopLeftY = 0.0f;

                  pImmediateCtx->RSSetViewports (1, &vp);

                  pImmediateCtx->Draw (4, 0);

                  hdr10_to_scRGB                = true;
                  framebuffer.dxgi.NativeFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
                }

                ApplyStateblock (pImmediateCtx, sb);
              }
            }
          }

          D3D11_TEXTURE2D_DESC staging_desc =
          {
            framebuffer.Width, framebuffer.Height,
            1,                 1,

            framebuffer.dxgi.NativeFormat,

            { 1, 0 },//D3D11_STANDARD_MULTISAMPLE_PATTERN },

              D3D11_USAGE_STAGING,     0x00,
            ( D3D11_CPU_ACCESS_READ ), 0x00
          };

          if ( SUCCEEDED (
            pDev->CreateTexture2D (
              &staging_desc, nullptr,
                &pStagingBackbufferCopy )
                         )
             )
          {
            if (backbuffer_desc.SampleDesc.Count == 1)
            {
              if (hdr10_to_scRGB)
              {
                pImmediateCtx->CopyResource ( pStagingBackbufferCopy,
                                              pHDRConvertTex          );
              }

              else
              {
                pImmediateCtx->CopyResource ( pStagingBackbufferCopy,
                                              pBackbufferSurface      );
              }
            }

            else
            {
              D3D11_TEXTURE2D_DESC resolve_desc =
                { framebuffer.Width,               framebuffer.Height,
                  1,                               1,
                  framebuffer.dxgi.NativeFormat, { 1, 0 },
                  D3D11_USAGE_DEFAULT,                  0x00,
                  0x0,                                  0x00 };

              SK_ComPtr <ID3D11Texture2D> pResolvedTex = nullptr;

              if ( SUCCEEDED (
                     pDev->CreateTexture2D (
                       &resolve_desc, nullptr, &pResolvedTex
                                           )
                             )
                 )
              {
                pImmediateCtx->ResolveSubresource ( pResolvedTex,       D3D11CalcSubresource (0, 0, 0),
                                                    pBackbufferSurface, D3D11CalcSubresource (0, 0, 0),
                                                      framebuffer.dxgi.NativeFormat );

                pImmediateCtx->CopyResource ( pStagingBackbufferCopy,
                                              pResolvedTex );
              }
            }

            if (pImmediateCtx3 != nullptr)
            {
              pImmediateCtx3->Flush1 (D3D11_CONTEXT_TYPE_COPY, nullptr);
            }

            pImmediateCtx->End (pPixelBufferFence);

            if (bPlaySound)
              SK_Screenshot_PlaySound ();

            return;
          }
        }
      }
    }
  }

  SK_Steam_CatastropicScreenshotFail ();

  // Something went wrong, crap!
  dispose ();
}

void
SK_D3D11_Screenshot::dispose (void) noexcept
{
  pPixelBufferFence      = nullptr;
  pStagingBackbufferCopy = nullptr;

  pImmediateCtx          = nullptr;
  pSwapChain             = nullptr;
  pDev                   = nullptr;

  pBackbufferSurface     = nullptr;

  if ( framebuffer.PixelBuffer ==
       framebuffer.root_.bytes )
  {
    framebuffer.PixelBuffer.release ();
    framebuffer.PBufferSize =
      framebuffer.root_.size;
  }

  framebuffer.PixelBuffer.reset (nullptr);

  size_t before =
    SK_ScreenshotQueue::pooled.capture_bytes.load ();

  SK_ScreenshotQueue::pooled.capture_bytes -=
    std::exchange (
      framebuffer.PBufferSize, 0
    );

  if (before < SK_ScreenshotQueue::pooled.capture_bytes.load  ( ))
  {            SK_ScreenshotQueue::pooled.capture_bytes.store (0); if (config.system.log_level > 1) SK_ReleaseAssert (false && "capture underflow"); }
};

bool
SK_D3D11_Screenshot::getData ( UINT* const pWidth,
                               UINT* const pHeight,
                               uint8_t   **ppData,
                               bool        Wait )
{
  framebuffer.AllowCopyToClipboard = wantClipboardCopy ();
  framebuffer.AllowSaveToDisk      = wantDiskCopy      ();

  auto& pooled =
    SK_ScreenshotQueue::pooled;

  auto ReadBack =
  [&]
  {
    const UINT Subresource =
      D3D11CalcSubresource ( 0, 0, 1 );

    D3D11_MAPPED_SUBRESOURCE finished_copy = { };

    if ( SUCCEEDED ( pImmediateCtx->Map (
                       pStagingBackbufferCopy, Subresource,
                         D3D11_MAP_READ,        0x0,
                           &finished_copy )
                   )
       )
    {
      size_t PackedDstPitch,
             PackedDstSlicePitch;

      DirectX::ComputePitch (
        framebuffer.dxgi.NativeFormat,
          framebuffer.Width, framebuffer.Height,
            PackedDstPitch, PackedDstSlicePitch
       );

      framebuffer.PackedDstSlicePitch = PackedDstSlicePitch;
      framebuffer.PackedDstPitch      = PackedDstPitch;

      size_t allocSize =
        (static_cast <size_t> (framebuffer.Height) + 1ULL)
                                                   *
                               framebuffer.PackedDstPitch;

      try
      {
        // Stash in Root?
        //  ( Less dynamic allocation for base-case )
        if ( pooled.capture_bytes.fetch_add  ( allocSize ) == 0 )
        {
          if (framebuffer.root_.size.load () < allocSize)
          {
            framebuffer.root_.bytes =
                std::make_unique <uint8_t []> (allocSize);
            framebuffer.root_.size.store      (allocSize);
          }

          allocSize =
            framebuffer.root_.size;

          framebuffer.PixelBuffer.reset (
            framebuffer.root_.bytes.get ()
          );
        }

        else
        {
          framebuffer.PixelBuffer =
            std::make_unique <uint8_t []> (
                                    allocSize );
        }

        framebuffer.PBufferSize =   allocSize;

        *pWidth  = framebuffer.Width;
        *pHeight = framebuffer.Height;

        uint8_t* pSrc = (uint8_t *)finished_copy.pData;
        uint8_t* pDst = framebuffer.PixelBuffer.get ();

        if (pSrc != nullptr)
        {
          for ( UINT i = 0; i < framebuffer.Height; ++i )
          {
            memcpy ( pDst, pSrc, finished_copy.RowPitch );

            pSrc += finished_copy.RowPitch;
            pDst +=         PackedDstPitch;
          }
        }
      }

      catch (const std::exception&)
      {
        pooled.capture_bytes   -= allocSize;
      }

      SK_LOG0 ( ( L"Screenshot Readback Complete after %lli frames",
                    SK_GetFramesDrawn () - ulCommandIssuedOnFrame ),
                  L"D3D11SShot" );

      pImmediateCtx->Unmap ( pStagingBackbufferCopy,
                                       Subresource );

      *ppData =
        framebuffer.PixelBuffer.get ();

      return true;
    }

    dispose ();

    return false;
  };

  bool ready_to_read = false;


  if (! Wait)
  {
    if (isReady ())
    {
      ready_to_read = true;
    }
  }

  else if (isValid ())
  {
    ready_to_read = true;
  }


  return ( ready_to_read ? ReadBack () :
                             false         );
}


volatile LONG __SK_ScreenShot_CapturingHUDless = 0;
volatile LONG __SK_D3D11_QueuedShots           = 0;
volatile LONG __SK_D3D11_InitiateHudFreeShot   = 0;

SK_LazyGlobal <concurrency::concurrent_queue <SK_D3D11_Screenshot *>> screenshot_queue;
SK_LazyGlobal <concurrency::concurrent_queue <SK_D3D11_Screenshot *>> screenshot_write_queue;


static volatile LONG
  __SK_HUD_YesOrNo = 1L;

bool
SK_D3D11_ShouldSkipHUD (void)
{
  return
    ( SK_Screenshot_IsCapturingHUDless () ||
       ReadAcquire    (&__SK_HUD_YesOrNo) <= 0 );
}

LONG
SK_D3D11_ShowGameHUD (void)
{
  InterlockedDecrement (&SK_D3D11_DrawTrackingReqs);

  return
    InterlockedIncrement (&__SK_HUD_YesOrNo);
}

LONG
SK_D3D11_HideGameHUD (void)
{
  InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);

  return
    InterlockedDecrement (&__SK_HUD_YesOrNo);
}

LONG
SK_D3D11_ToggleGameHUD (void)
{
  static volatile LONG last_state =
    (ReadAcquire (&__SK_HUD_YesOrNo) > 0);

  if (ReadAcquire (&last_state))
  {
    SK_D3D11_HideGameHUD ();

    return
      InterlockedDecrement (&last_state);
  }

  SK_D3D11_ShowGameHUD ();

  return
    InterlockedIncrement (&last_state);
}

void
SK_Screenshot_D3D11_RestoreHUD (void)
{
  if ( -1 ==
         InterlockedCompareExchange (
           &__SK_D3D11_InitiateHudFreeShot, 0, -1
         )
     )
  {
    SK_D3D11_ShowGameHUD ();
  }
}

bool
SK_D3D11_RegisterHUDShader (        uint32_t  bytecode_crc32c,
                             std::type_index _T,
                                        bool  remove )
{
  SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>* record =
    (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders->vertex;

  auto& hud    =
    record->hud;


  enum class _ShaderType
  {
    Vertex, Pixel, Geometry, Domain, Hull, Compute
  };

  static
    std::unordered_map < std::type_index, _ShaderType >
      __type_map =
      {
        { std::type_index (typeid ( ID3D11VertexShader   )), _ShaderType::Vertex   },
        { std::type_index (typeid ( ID3D11PixelShader    )), _ShaderType::Pixel    },
        { std::type_index (typeid ( ID3D11GeometryShader )), _ShaderType::Geometry },
        { std::type_index (typeid ( ID3D11DomainShader   )), _ShaderType::Domain   },
        { std::type_index (typeid ( ID3D11HullShader     )), _ShaderType::Hull     },
        { std::type_index (typeid ( ID3D11ComputeShader  )), _ShaderType::Compute  }
      };

  static
    std::unordered_map < _ShaderType, SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>* >
      __record_map =
      {
        { _ShaderType::Vertex  , (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders->vertex   },
        { _ShaderType::Pixel   , (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders->pixel    },
        { _ShaderType::Geometry, (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders->geometry },
        { _ShaderType::Domain  , (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders->domain   },
        { _ShaderType::Hull    , (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders->hull     },
        { _ShaderType::Compute , (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D11_Shaders->compute  }
      };

  static
    std::unordered_map < _ShaderType, std::unordered_map <uint32_t, LONG>& >
      __hud_map =
      {
        { _ShaderType::Vertex  , __record_map.at (_ShaderType::Vertex)->hud   },
        { _ShaderType::Pixel   , __record_map.at (_ShaderType::Pixel)->hud    },
        { _ShaderType::Geometry, __record_map.at (_ShaderType::Geometry)->hud },
        { _ShaderType::Domain  , __record_map.at (_ShaderType::Domain)->hud   },
        { _ShaderType::Hull    , __record_map.at (_ShaderType::Hull)->hud     },
        { _ShaderType::Compute , __record_map.at (_ShaderType::Compute)->hud  }
      };

  if (__type_map.count (_T))
  {
    hud =
      __hud_map.at    (__type_map.at (_T));
    record =
      __record_map.at (__type_map.at (_T));
  }

  else
  {
    SK_ReleaseAssert (! L"Invalid Shader Type");
  }

  if (! remove)
  {
    InterlockedIncrement (&SK_D3D11_TrackingCount->Conditional);

    return
      record->addTrackingRef (hud, bytecode_crc32c);
  }

  InterlockedDecrement (&SK_D3D11_TrackingCount->Conditional);

  return
    record->releaseTrackingRef (hud, bytecode_crc32c);
}

bool
SK_D3D11_UnRegisterHUDShader ( uint32_t         bytecode_crc32c,
                               std::type_index _T               )
{
  return
    SK_D3D11_RegisterHUDShader (
      bytecode_crc32c,   _T,   true
    );
}

bool
SK_D3D11_CaptureScreenshot  ( SK_ScreenshotStage when =
                              SK_ScreenshotStage::EndOfFrame,
                              bool               allow_sound = true,
                              std::string        title       = "" )
{
  const SK_RenderBackend_V2& rb =
    SK_GetCurrentRenderBackend ();

  if ( ((int)rb.api & (int)SK_RenderAPI::D3D11)
                   == (int)SK_RenderAPI::D3D11 )
  {
    static const
      std::map <SK_ScreenshotStage, int>
        __stage_map = {
          { SK_ScreenshotStage::BeforeGameHUD, 0 },
          { SK_ScreenshotStage::BeforeOSD,     1 },
          { SK_ScreenshotStage::PrePresent,    2 },
          { SK_ScreenshotStage::EndOfFrame,    3 },
          { SK_ScreenshotStage::ClipboardOnly, 4 }
        };

    const auto it =
               __stage_map.find (when);
    if ( it != __stage_map.cend (    ) )
    {
      const int stage =
        it->second;

      if (when == SK_ScreenshotStage::BeforeGameHUD)
      {
        static const auto& vertex = SK_D3D11_Shaders->vertex;
        static const auto& pixel  = SK_D3D11_Shaders->pixel;

        if ( vertex.hud.empty () &&
              pixel.hud.empty ()    )
        {
          return false;
        }
      }

      InterlockedIncrement (
        &enqueued_screenshots.stages [stage]
      );

      if (allow_sound)
      {
        InterlockedIncrement (
          &enqueued_sounds.stages [stage]
        );
      }

      if (! title.empty ())
      {
        enqueued_titles.stages [stage] = title;
      }

      return true;
    }
  }

  return false;
}

void
SK_D3D11_ProcessScreenshotQueueEx ( SK_ScreenshotStage stage_,
                                    bool               wait,
                                    bool               purge)
{
  static std::atomic_int run_count = 0;

  if (stage_ != SK_ScreenshotStage::_FlushQueue)
    ++run_count;

  else if (run_count == 0)
    return;


  const SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  constexpr int __MaxStage = ARRAYSIZE (SK_ScreenshotQueue::stages) - 1;
  const     int      stage =
    sk::narrow_cast <int> (stage_);

  assert ( stage >= 0 &&
           stage <= ( __MaxStage + 1 ) );

  if ( stage < 0 ||
       stage > ( __MaxStage + 1 ) )
    return;


  if ( stage == ( __MaxStage + 1 ) && purge )
  {
    // Empty all stage queues first, then we
    //   can wait for outstanding screenshots
    //     to finish.
    for ( int implied_stage =  0          ;
              implied_stage <= __MaxStage ;
            ++implied_stage                 )
    {
      InterlockedExchange (
        &enqueued_screenshots.stages [
              implied_stage          ],   0
      );
    }
  }


  else if (stage <= __MaxStage)
  {
    if (ReadAcquire (&enqueued_screenshots.stages [stage]) > 0)
    {
      // Just kill any queued shots, we need to be quick about this.
      if (purge)
        InterlockedExchange      (&enqueued_screenshots.stages [stage], 0);

      else
      {
        if (InterlockedDecrement (&enqueued_screenshots.stages [stage]) >= 0)
        {    // --
          auto pDev =
            rb.getDevice <ID3D11Device> ();

          if (pDev != nullptr)
          {
            if ( SK_ScreenshotQueue::pooled.capture_bytes.load  () <
                 SK_ScreenshotQueue::maximum.capture_bytes.load () )
            {
              bool clipboard_only =
                ( stage == __MaxStage );

              if (clipboard_only || SK_GetCurrentRenderBackend ().screenshot_mgr->checkDiskSpace (20ULL * 1024ULL * 1024ULL))
              {
                bool allow_sound =
                  ReadAcquire (&enqueued_sounds.stages [stage]) > 0;

                if ( allow_sound )
                  InterlockedDecrement (&enqueued_sounds.stages [stage]);

                std::string                                title = "";
                std::swap (enqueued_titles.stages [stage], title);

                screenshot_queue->push (
                  new SK_D3D11_Screenshot (
                    pDev, allow_sound, clipboard_only, title
                  )
                );
              }
            }

            else {
              SK_RunOnce (
                SK_ImGui_Warning (L"Too much screenshot data queued")
              );
            }
          }
        }

        else // ++
          InterlockedIncrement   (&enqueued_screenshots.stages [stage]);
      }
    }
  }


  static volatile LONG _lockVal = 0;

  struct signals_s {
    HANDLE capture    = INVALID_HANDLE_VALUE;
    HANDLE hq_encode  = INVALID_HANDLE_VALUE;

    struct {
      HANDLE initiate = INVALID_HANDLE_VALUE;
      HANDLE finished = INVALID_HANDLE_VALUE;
    } abort;
  } static signal = { };

  static HANDLE
    hWriteThread      = INVALID_HANDLE_VALUE;


  if ( 0                    == InterlockedCompareExchange (&_lockVal, 1, 0) &&
       INVALID_HANDLE_VALUE == signal.capture )
  {
    signal.capture        = SK_CreateEvent ( nullptr, FALSE, TRUE,  nullptr );
    signal.abort.initiate = SK_CreateEvent ( nullptr, TRUE,  FALSE, nullptr );
    signal.abort.finished = SK_CreateEvent ( nullptr, FALSE, FALSE, nullptr );
    signal.hq_encode      = SK_CreateEvent ( nullptr, FALSE, FALSE, nullptr );

    hWriteThread =
    SK_Thread_CreateEx ([](LPVOID) -> DWORD
    {
      SK_RenderBackend& rb =
        SK_GetCurrentRenderBackend ();

      SetThreadPriority ( SK_GetCurrentThread (), THREAD_PRIORITY_NORMAL );

      do
      {
        HANDLE signals [] = {
          signal.capture,       // Screenshots are waiting for write
          signal.abort.initiate // Screenshot full-abort requested (i.e. for SwapChain Resize)
        };

        DWORD dwWait =
          WaitForMultipleObjects ( 2, signals, FALSE, INFINITE );

        bool
          purge_and_run =
            ( dwWait == ( WAIT_OBJECT_0 + 1 ) );

        static std::vector <SK_D3D11_Screenshot*> to_write;

        while (! screenshot_write_queue->empty ())
        {
          SK_D3D11_Screenshot*                  pop_off   = nullptr;
          if ( screenshot_write_queue->try_pop (pop_off) &&
                                                pop_off  != nullptr )
          {
            if (purge_and_run)
            {
              delete pop_off;
              continue;
            }

            SK_Screenshot::framebuffer_s* pFrameData =
              pop_off->getFinishedData ();

            // Why's it on the wait-queue if it's not finished?!
            assert (pFrameData != nullptr);

            if (pFrameData != nullptr)
            {
              if (pFrameData->AllowSaveToDisk && config.screenshots.png_compress)
                to_write.emplace_back (pop_off);

              using namespace DirectX;

              bool  skip_me = false;
              Image raw_img = {   };

              ComputePitch (
                pFrameData->dxgi.NativeFormat,
                  pFrameData->Width, pFrameData->Height,
                    raw_img.rowPitch, raw_img.slicePitch
              );

              // Steam wants JPG, smart people want PNG
              DirectX::WICCodecs codec = WIC_CODEC_JPEG;

              raw_img.format = pop_off->getInternalFormat ();
              raw_img.width  = pFrameData->Width;
              raw_img.height = pFrameData->Height;
              raw_img.pixels = pFrameData->PixelBuffer.get ();

              bool hdr = ( rb.isHDRCapable () &&
                           rb.isHDRActive  () );

              SK_RunOnce (SK_SteamAPI_InitManagers ());

              wchar_t       wszAbsolutePathToScreenshot [ MAX_PATH + 2 ] = { };
              wcsncpy_s   ( wszAbsolutePathToScreenshot,  MAX_PATH,
                              rb.screenshot_mgr->getBasePath (),
                                _TRUNCATE );

              if ( config.steam.screenshots.enable_hook &&
                  !config.platform.steam_is_b0rked      &&
                          steam_ctx.Screenshots ()      != nullptr )
              {
                PathAppendW          (wszAbsolutePathToScreenshot, L"SK_SteamScreenshotImport.jpg");
                SK_CreateDirectories (wszAbsolutePathToScreenshot);
              }

              else if ( hdr )
              {
                codec = WIC_CODEC_PNG;

                PathAppendW (         wszAbsolutePathToScreenshot,
                    SK_FormatStringW (LR"(LDR\%ws.png)", pFrameData->file_name.c_str ()).c_str ());
                SK_CreateDirectories (wszAbsolutePathToScreenshot);
              }

              // Not HDR and not importing to Steam,
              //   we've got nothing left to do...
              else
              {
                if (! pop_off)
                  skip_me = true;
                else // Unless user wants a Clipboard Copy
                  skip_me = !(pop_off->wantClipboardCopy () && (config.screenshots.copy_to_clipboard || (! pFrameData->AllowSaveToDisk)));
              }

              if ((! skip_me) && pop_off != nullptr)
              {
                ScratchImage
                  un_srgb;
                  un_srgb.InitializeFromImage (raw_img);

                DirectX::TexMetadata
                meta           = {           };
                meta.width     = raw_img.width;
                meta.height    = raw_img.height;
                meta.depth     = 1;
                meta.format    = raw_img.format;
                meta.dimension = TEX_DIMENSION_TEXTURE2D;
                meta.arraySize = 1;
                meta.mipLevels = 1;

                switch (pFrameData->dxgi.AlphaMode)
                {
                  case DXGI_ALPHA_MODE_UNSPECIFIED:
                    meta.SetAlphaMode (TEX_ALPHA_MODE_UNKNOWN);
                    break;
                  case DXGI_ALPHA_MODE_PREMULTIPLIED:
                    meta.SetAlphaMode (TEX_ALPHA_MODE_PREMULTIPLIED);
                    break;
                  case DXGI_ALPHA_MODE_STRAIGHT:
                    meta.SetAlphaMode (TEX_ALPHA_MODE_STRAIGHT);
                    break;
                  case DXGI_ALPHA_MODE_IGNORE:
                    meta.SetAlphaMode (TEX_ALPHA_MODE_OPAQUE);
                    break;
                }

                meta.SetAlphaMode (TEX_ALPHA_MODE_OPAQUE);

                ScratchImage
                  un_scrgb;
                  un_scrgb.Initialize (meta);

                ScratchImage
                  final_sdr;
                  final_sdr.Initialize (meta);

                ScratchImage tonemapped_copy;

                static const XMVECTORF32 c_MaxNitsFor2084 =
                  { 10000.0f, 10000.0f, 10000.0f, 1.f };

                static const XMMATRIX c_from2020to709 = // Transposed
                {
                   1.66096379471340f,   -0.124477196529907f,   -0.0181571579858552f, 0.0f,
                  -0.588112737547978f,   1.13281946828499f,    -0.100666415661988f,  0.0f,
                  -0.0728510571654192f, -0.00834227175508652f,  1.11882357364784f,   0.0f,
                   0.0f,                 0.0f,                  0.0f,                1.0f
                };

                HRESULT hr = S_OK;

                if ( un_srgb.GetImageCount () == 1 &&
                     hdr                      && raw_img.format != DXGI_FORMAT_R16G16B16A16_FLOAT &&
                     rb.scanout.getEOTF    () == SK_RenderBackend::scan_out_s::SMPTE_2084 )
                { // ^^^ EOTF is not always accurate, but we know SMPTE 2084 is not used w/ FP16 color
                  TransformImage ( un_srgb.GetImages     (),
                                   un_srgb.GetImageCount (),
                                   un_srgb.GetMetadata   (),
                    [&](XMVECTOR* outPixels, const XMVECTOR* inPixels, size_t width, size_t y)
                    {
                      UNREFERENCED_PARAMETER(y);

                      for (size_t j = 0; j < width; ++j)
                      {
                        XMVECTOR value = inPixels [j];
                        outPixels [j]  = XMVector3Transform (value, c_from2020to709);
                      }
                    }, un_scrgb
                  );

                  std::swap (un_scrgb, un_srgb);
                }

                if ( un_srgb.GetImageCount () == 1 &&
                     hdr )
                {
                  float    maxLum = 0.0f,
                           minLum = 5240320.0f;
                  XMVECTOR maxCLL = XMVectorZero (),
                           maxRGB = XMVectorZero ();

                  static const XMVECTORF32 s_luminance =
                    { 0.2126729f, 0.7151522f, 0.0721750f, 0.f };

                  double lumTotal    = 0.0;
                  double logLumTotal = 0.0;
                  double N           = 0.0;

                  hr =              un_srgb.GetImageCount () == 1 ?
                    EvaluateImage ( un_srgb.GetImages     (),
                                    un_srgb.GetImageCount (),
                                    un_srgb.GetMetadata   (),
                    [&](const XMVECTOR* pixels, size_t width, size_t y)
                    {
                      UNREFERENCED_PARAMETER(y);

                      for (size_t j = 0; j < width; ++j)
                      {
                        XMVECTOR v = *pixels;

                        maxCLL =
                          XMVectorMax (XMVectorMultiply (v, s_luminance), maxCLL);

                        v =
                          XMVector3Transform (v, c_from709toXYZ);

                        const float fLum =
                          XMVectorGetY (v);

                        maxLum =
                          std::max (fLum, maxLum);

                        minLum =
                          std::min (fLum, minLum);

                        logLumTotal +=
                          log2 ( std::max (0.000001, static_cast <double> (std::max (0.0f, XMVectorGetY (v)))) );
                           lumTotal +=               static_cast <double> (std::max (0.0f, XMVectorGetY (v)));
                        ++N;

                        v =
                          XMVectorMax (g_XMZero, v);

                        maxRGB =
                          XMVectorMax (v, maxRGB);

                        pixels++;
                      }
                    }
                  ) : E_POINTER;

                  maxCLL =
                    XMVectorReplicate (
                      std::max ({ XMVectorGetX (maxCLL), XMVectorGetY (maxCLL), XMVectorGetZ (maxCLL) })
                    );

                  SK_LOGi0 ( L"Min Luminance: %f, Max Luminance: %f", std::max (0.0f, minLum) * 80.0f,
                                                                                      maxLum  * 80.0f );

                  SK_LOGi0 ( L"Mean Luminance (arithmetic, geometric): %f, %f", 80.0 *      ( lumTotal    / N ),
                                                                                80.0 * exp2 ( logLumTotal / N ) );

                  auto        luminance_freq = std::make_unique <uint32_t []> (65536);
                  ZeroMemory (luminance_freq.get (),     sizeof (uint32_t)  *  65536);

                  // 0 nits - 10k nits (appropriate for screencap, but not HDR photography)
                  minLum = std::clamp (minLum, 0.0f,   125.0f);
                  maxLum = std::clamp (maxLum, minLum, 125.0f);

                  const float fLumRange =
                           (maxLum - minLum);

                  EvaluateImage ( un_srgb.GetImages     (),
                                  un_srgb.GetImageCount (),
                                  un_srgb.GetMetadata   (),
                  [&](const XMVECTOR* pixels, size_t width, size_t y)
                  {
                    UNREFERENCED_PARAMETER(y);

                    for (size_t j = 0; j < width; ++j)
                    {
                      XMVECTOR v = *pixels++;

                      v =
                        XMVector3Transform (v, c_from709toXYZ);

                      luminance_freq [
                        std::clamp ( (int)
                          std::roundf (
                            (XMVectorGetY (v) - minLum)     /
                                                 (fLumRange / 65536.0f) ),
                                                           0, 65535 ) ]++;
                    }
                  });

                        double percent  = 100.0;
                  const double img_size = (double)un_srgb.GetMetadata ().width *
                                          (double)un_srgb.GetMetadata ().height;

                  for (auto i = 65535; i >= 0; --i)
                  {
                    percent -=
                      100.0 * ((double)luminance_freq [i] / img_size);

                    if (percent <= 99.94)
                    {
                      maxLum =
                        minLum + (fLumRange * ((float)i / 65536.0f));

                      break;
                    }
                  }

                  pFrameData->hdr.max_cll_nits =                      80.0f * maxLum;
                  pFrameData->hdr.avg_cll_nits = static_cast <float> (80.0  * ( lumTotal / N ));

                  // After tonemapping, re-normalize the image to preserve peak white,
                  //   this is important in cases where the maximum luminance was < 1000 nits
                  XMVECTOR maxTonemappedRGB = g_XMZero;

                  // User's display is the canonical mastering display, anything that would have clipped
                  //   on their display should be clipped in the tonemapped SDR image.
                  float _maxNitsToTonemap = rb.displays [rb.active_display].gamut.maxLocalY / 80.0f;

                  const float SDR_YInPQ =
                    LinearToPQY (1.5f);

                  const float  maxYInPQ =
                    std::max (SDR_YInPQ,
                      LinearToPQY (std::min (_maxNitsToTonemap, maxLum))
                    );

                  static std::vector <parallel_tonemap_job_s> parallel_jobs   (config.screenshots.avif.max_threads);
                  static std::vector <HANDLE>                 parallel_start  (config.screenshots.avif.max_threads);
                  static std::vector <HANDLE>                 parallel_finish (config.screenshots.avif.max_threads);
                         std::vector <XMVECTOR>               parallel_pixels (un_srgb.GetMetadata ().width *
                                                                               un_srgb.GetMetadata ().height);

                  SK_Image_InitializeTonemap   (         parallel_jobs, parallel_start,      parallel_finish);
                  SK_Image_EnqueueTonemapTask  (un_srgb, parallel_jobs, parallel_pixels, maxYInPQ, SDR_YInPQ);
                  SK_Image_DispatchTonemapJobs (         parallel_jobs);
                  SK_Image_GetTonemappedPixels (final_sdr, un_srgb,     parallel_pixels,     parallel_finish);

                  for (auto& job : parallel_jobs)
                  {
                    maxTonemappedRGB =
                      XMVectorMax (job.maxTonemappedRGB, maxTonemappedRGB);
                  }

                  float fMaxR = XMVectorGetX (maxTonemappedRGB);
                  float fMaxG = XMVectorGetY (maxTonemappedRGB);
                  float fMaxB = XMVectorGetZ (maxTonemappedRGB);

                  if (false)
                      //( fMaxR <  1.0f ||
                      //  fMaxG <  1.0f ||
                      //  fMaxB <  1.0f ) &&
                      //( fMaxR >= 1.0f ||
                      //  fMaxG >= 1.0f ||
                      //  fMaxB >= 1.0f ))
                  {
                    float fSmallestComp =
                      std::min ({fMaxR, fMaxG, fMaxB});

                    if (fSmallestComp > .875f)
                    {
                      SK_LOGi0 (
                        L"After tone mapping, maximum RGB was %4.2fR %4.2fG %4.2fB -- "
                        L"SDR image will be normalized to min (R|G|B) and clipped.",
                          fMaxR, fMaxG, fMaxB
                      );

                      float fRescale =
                        (1.0f / fSmallestComp);

                      XMVECTOR vNormalizationScale =
                        XMVectorReplicate (fRescale);

                      TransformImage (*final_sdr.GetImages (),
                        [&]( _Out_writes_ (width)       XMVECTOR* outPixels,
                              _In_reads_  (width) const XMVECTOR* inPixels,
                                                        size_t    width,
                                                        size_t )
                        {
                          for (size_t j = 0; j < width; ++j)
                          {
                            XMVECTOR value =
                             inPixels [j];
                            outPixels [j] =
                              XMVectorSaturate (
                                XMVectorMultiply (value, vNormalizationScale)
                              );
                          }
                        }, tonemapped_copy
                      );

                      std::swap (final_sdr, tonemapped_copy);
                    }
                  }

                  if (         final_sdr.GetImageCount () == 1) {
                    Convert ( *final_sdr.GetImages     (),
                                DXGI_FORMAT_B8G8R8X8_UNORM_SRGB,
                                  (TEX_FILTER_FLAGS)0x200000FF, 1.0f,
                                      un_srgb );

                    std::swap (un_scrgb, un_srgb);
                  }
                }

                else if (un_srgb.GetMetadata ().format != DXGI_FORMAT_B8G8R8X8_UNORM)
                {
                  if (         un_srgb.GetImageCount () == 1) {
                    Convert ( *un_srgb.GetImages     (),
                                DXGI_FORMAT_B8G8R8X8_UNORM,
                                  TEX_FILTER_DITHER_DIFFUSION,
                                    TEX_THRESHOLD_DEFAULT,
                                      un_scrgb );
                  }
                }

                bool bCopiedToClipboard = false;

                // Store HDR PNG copy
                if (pFrameData->AllowSaveToDisk && config.screenshots.use_hdr_png && hdr)
                {
                  DirectX::ScratchImage                  hdr10_img;
                  if (SK_HDR_ConvertImageToPNG (raw_img, hdr10_img))
                  {
                    wchar_t       wszAbsolutePathToLossless [ MAX_PATH + 2 ] = { };
                    wcsncpy_s   ( wszAbsolutePathToLossless,  MAX_PATH,
                                    rb.screenshot_mgr->getBasePath (),
                                      _TRUNCATE );

                    PathAppendW ( wszAbsolutePathToLossless,
                      SK_FormatStringW ( L"HDR\\%ws.png",
                                  pFrameData->file_name.c_str () ).c_str () );

                    SK_CreateDirectories (wszAbsolutePathToLossless);

                    if (SK_HDR_SavePNGToDisk (wszAbsolutePathToLossless, hdr10_img.GetImages (), &raw_img, pFrameData->title.c_str ()))
                    {
                      if (un_scrgb.GetImageCount () == 1 && pop_off->wantClipboardCopy () && (config.screenshots.copy_to_clipboard))
                      {
                        if (SK_PNG_CopyToClipboard (*hdr10_img.GetImages (), wszAbsolutePathToLossless, 0))
                        {
                          bCopiedToClipboard = true;
                        }
                      }
                    }
                  }
                }

                if (un_scrgb.GetImageCount () == 1 && pop_off->wantClipboardCopy () && (config.screenshots.copy_to_clipboard || (! pFrameData->AllowSaveToDisk)) && (! bCopiedToClipboard))
                {
                  rb.screenshot_mgr->copyToClipboard (*un_scrgb.GetImages (), hdr ? &raw_img
                                                                                  : nullptr,
                                                                              hdr ? wszAbsolutePathToScreenshot
                                                                                  : nullptr);
                }

                // Store tonemapped copy
                if (               pFrameData->AllowSaveToDisk    &&
                                   un_scrgb.GetImageCount () == 1 &&
                      SUCCEEDED (
                  SaveToWICFile ( *un_scrgb.GetImages (), WIC_FLAGS_DITHER_DIFFUSION,
                                     GetWICCodec         (codec),
                                      wszAbsolutePathToScreenshot, nullptr,
                                        SK_WIC_SetMaximumQuality,
                                        [&](IWICMetadataQueryWriter *pMQW)
                                        {
                                          SK_WIC_SetMetadataTitle (pMQW, pFrameData->title);
                                        } )
                                )
                   )
                {
#if 0
                  if (codec == WIC_CODEC_JPEG && hdr)
                  {
                    SK_Screenshot_SaveUHDR (raw_img, *un_scrgb.GetImages (), wszAbsolutePathToScreenshot);
                  }
#endif

                  if ( config.steam.screenshots.enable_hook &&
                      !config.platform.steam_is_b0rked      &&
                          steam_ctx.Screenshots () != nullptr )
                  {
                    wchar_t       wszAbsolutePathToThumbnail [ MAX_PATH + 2 ] = { };
                    wcsncpy_s   ( wszAbsolutePathToThumbnail,  MAX_PATH,
                                    rb.screenshot_mgr->getBasePath (),
                                      _TRUNCATE );

                    PathAppendW (wszAbsolutePathToThumbnail, L"SK_SteamThumbnailImport.jpg");

                    auto aspect =
                      static_cast <double> (pFrameData->Height) /
                      static_cast <double> (pFrameData->Width);

                    ScratchImage thumbnailImage;

                    Resize ( *un_scrgb.GetImages (), 200,
                               sk::narrow_cast <size_t> (200.0 * aspect),
                                TEX_FILTER_DITHER_DIFFUSION | TEX_FILTER_FORCE_WIC
                              | TEX_FILTER_TRIANGLE,
                                  thumbnailImage );

                    if (               thumbnailImage.GetImages ()) {
                      SaveToWICFile ( *thumbnailImage.GetImages (), WIC_FLAGS_DITHER_DIFFUSION,
                                        GetWICCodec                (codec),
                                          wszAbsolutePathToThumbnail, nullptr,
                                            SK_WIC_SetMaximumQuality,
                                            [&](IWICMetadataQueryWriter *pMQW)
                                            {
                                              SK_WIC_SetMetadataTitle (pMQW, pFrameData->title);
                                            } );

                      std::string ss_path (
                        SK_WideCharToUTF8 (wszAbsolutePathToScreenshot)
                      );

                      std::string ss_thumb (
                        SK_WideCharToUTF8 (wszAbsolutePathToThumbnail)
                      );

                      ScreenshotHandle screenshot =
                        SK_SteamAPI_AddScreenshotToLibraryEx (
                          ss_path.c_str    (),
                            ss_thumb.c_str (),
                              pFrameData->Width,
                              pFrameData->Height,
                                true );

                      SK_LOG1 ( ( L"Finished Steam Screenshot Import for Handle: '%x' (%lu frame latency)",
                                  screenshot, SK_GetFramesDrawn () - pop_off->getStartFrame () ),
                                    L"SteamSShot" );

                      // Remove the temporary files...
                      DeleteFileW (wszAbsolutePathToThumbnail);
                    }
                    DeleteFileW (wszAbsolutePathToScreenshot);
                  }
                }
              }

              if (! (config.screenshots.png_compress && pFrameData->AllowSaveToDisk))
              {
                delete pop_off;
                       pop_off = nullptr;
              }
            }
          }
        }

        if (purge_and_run)
        {
          SetThreadPriority ( SK_GetCurrentThread (), THREAD_PRIORITY_NORMAL );

          purge_and_run = false;

          ResetEvent (signal.abort.initiate); // Abort no longer needed
          SetEvent   (signal.abort.finished); // Abort is complete
        }

        ////if (config.screenshots.png_compress || config.screenshots.copy_to_clipboard)
        {
          int enqueued_lossless = 0;

          while ( ! to_write.empty () )
          {
            auto it =
              to_write.back ();

            to_write.pop_back ();

            if ( it                     == nullptr ||
                 it->getFinishedData () == nullptr )
            {
              delete it;

              continue;
            }

            static concurrency::concurrent_queue <SK_Screenshot::framebuffer_s*>
              raw_images_;

            SK_Screenshot::framebuffer_s* fb_orig =
              it->getFinishedData ();

            SK_ReleaseAssert (fb_orig != nullptr);

            if (fb_orig != nullptr)
            {
              SK_Screenshot::framebuffer_s* fb_copy =
                new SK_Screenshot::framebuffer_s ();

              fb_copy->Height               = fb_orig->Height;
              fb_copy->Width                = fb_orig->Width;
              fb_copy->dxgi.NativeFormat    = fb_orig->dxgi.NativeFormat;
              fb_copy->dxgi.AlphaMode       = fb_orig->dxgi.AlphaMode;
              fb_copy->hdr.max_cll_nits     = fb_orig->hdr.max_cll_nits;
              fb_copy->hdr.avg_cll_nits     = fb_orig->hdr.avg_cll_nits;
              fb_copy->PBufferSize          = fb_orig->PBufferSize;
              fb_copy->PackedDstPitch       = fb_orig->PackedDstPitch;
              fb_copy->PackedDstSlicePitch  = fb_orig->PackedDstSlicePitch;
              fb_copy->AllowCopyToClipboard = fb_orig->AllowCopyToClipboard;
              fb_copy->AllowSaveToDisk      = fb_orig->AllowSaveToDisk;
              fb_copy->file_name            = fb_orig->file_name;
              fb_copy->title                = fb_orig->title;

              fb_copy->PixelBuffer =
                std::move (fb_orig->PixelBuffer);

              raw_images_.push (fb_copy);

              ++enqueued_lossless;
            }

            delete it;

            static volatile HANDLE
              hThread = INVALID_HANDLE_VALUE;

            if (InterlockedCompareExchangePointer (&hThread, 0, INVALID_HANDLE_VALUE) == INVALID_HANDLE_VALUE)
            {                                     SK_Thread_CreateEx ([](LPVOID pUser)->DWORD {
              SK_RenderBackend& rb =
                SK_GetCurrentRenderBackend ();

              concurrency::concurrent_queue <SK_Screenshot::framebuffer_s *>*
                images_to_write =
                  (concurrency::concurrent_queue <SK_Screenshot::framebuffer_s *>*)pUser;

              SetThreadPriority           ( SK_GetCurrentThread (), THREAD_MODE_BACKGROUND_BEGIN );
              SetThreadPriority           ( SK_GetCurrentThread (), THREAD_PRIORITY_BELOW_NORMAL );

              while (0 == ReadAcquire (&__SK_DLL_Ending))
              {
                SK_Screenshot::framebuffer_s *pFrameData =
                  nullptr;

                SK_WaitForSingleObject  (
                  signal.hq_encode, INFINITE
                );

                while   (! images_to_write->empty   (          ))
                { while (! images_to_write->try_pop (pFrameData))
                  {
                    SwitchToThread ();
                  }

                  if (ReadAcquire (&__SK_DLL_Ending))
                    break;

                  wchar_t       wszAbsolutePathToLossless [ MAX_PATH + 2 ] = { };
                  wcsncpy_s   ( wszAbsolutePathToLossless,  MAX_PATH,
                                  rb.screenshot_mgr->getBasePath (),
                                    _TRUNCATE );

                  bool hdr =
                    ( SK_GetCurrentRenderBackend ().isHDRCapable () &&
                      SK_GetCurrentRenderBackend ().isHDRActive  () );

                  if (hdr)
                  {
                    PathAppendW ( wszAbsolutePathToLossless,
                      SK_FormatStringW ( L"HDR\\%ws.jxr",
                                  pFrameData->file_name.c_str () ).c_str () );
                  }

                  else
                  {
                    PathAppendW ( wszAbsolutePathToLossless,
                      SK_FormatStringW ( L"Lossless\\%ws.png",
                                  pFrameData->file_name.c_str () ).c_str () );
                  }

                  // Why's it on the wait-queue if it's not finished?!
                  assert (pFrameData != nullptr);

                  if (pFrameData != nullptr)
                  {
                    using namespace DirectX;

                    Image raw_img = { };

                    uint8_t* pDst =
                      pFrameData->PixelBuffer.get ();

                    for (UINT i = 0; i < pFrameData->Height; ++i)
                    {
                      // Eliminate pre-multiplied alpha problems (the stupid way)
                      switch (pFrameData->dxgi.NativeFormat)
                      {
                        case DXGI_FORMAT_B8G8R8A8_UNORM:
                        case DXGI_FORMAT_R8G8B8A8_UNORM:
                        {
                          for ( UINT j = 3                          ;
                                     j < pFrameData->PackedDstPitch ;
                                     j += 4 )
                          {    pDst [j] = 255UL;        }
                        } break;

                        case DXGI_FORMAT_R16G16B16A16_FLOAT:
                        {
                          using namespace DirectX::PackedVector;

                          static const HALF g_XMOneFP16 (XMConvertFloatToHalf (1.0f));

                          for ( UINT j  = 0                          ;
                                     j < pFrameData->PackedDstPitch  ;
                                     j += 8 )
                          {
                            ((XMHALF4 *)&(pDst [j]))->w =
                              g_XMOneFP16;
                          }
                        } break;
                      }

                      pDst += pFrameData->PackedDstPitch;
                    }

                    ComputePitch ( pFrameData->dxgi.NativeFormat,
                                   pFrameData->Width,
                                   pFrameData->Height,
                                       raw_img.rowPitch,
                                       raw_img.slicePitch
                    );

                    raw_img.format = pFrameData->dxgi.NativeFormat;
                    raw_img.width  = pFrameData->Width;
                    raw_img.height = pFrameData->Height;
                    raw_img.pixels = pFrameData->PixelBuffer.get ();

                    if (pFrameData->AllowCopyToClipboard && (config.screenshots.copy_to_clipboard || (! pFrameData->AllowSaveToDisk)) && (! hdr))
                    {
                      ScratchImage
                        clipboard;
                        clipboard.InitializeFromImage (raw_img);

                      if (SUCCEEDED ( Convert ( raw_img,
                                                  DXGI_FORMAT_B8G8R8X8_UNORM,
                                                    TEX_FILTER_DITHER,
                                                      TEX_THRESHOLD_DEFAULT,
                                                        clipboard ) ) )
                      {
                        rb.screenshot_mgr->copyToClipboard (*clipboard.GetImages ());
                      }
                    }

                    // Handle Normal PNG or JXR/AVIF; HDR PNG was already handled above...
                    if (pFrameData->AllowSaveToDisk && config.screenshots.png_compress && ((! config.screenshots.use_hdr_png) || (! hdr)))
                    {
                      SK_CreateDirectories (wszAbsolutePathToLossless);

                      ScratchImage
                        un_srgb;
                        un_srgb.InitializeFromImage (raw_img);

                      if (hdr && config.screenshots.use_avif)
                      {
                        SK_Screenshot_SaveAVIF (un_srgb, wszAbsolutePathToLossless, static_cast <uint16_t> (std::max (0.0f, pFrameData->hdr.max_cll_nits)),
                                                                                    static_cast <uint16_t> (std::max (0.0f, pFrameData->hdr.avg_cll_nits)));
                      }

                      if (hdr && config.screenshots.use_jxl)
                      {
                        SK_Screenshot_SaveJXL (un_srgb, wszAbsolutePathToLossless);
                      }

                      HRESULT hrSaveToWIC = S_OK;

                      if ((! hdr) || (! (config.screenshots.use_avif ||
                                         config.screenshots.use_jxl)))
                      {
                        const bool bUseCompatHacks =
                          config.screenshots.compatibility_mode;

                        hrSaveToWIC =     un_srgb.GetImages () ?
                          SaveToWICFile (*un_srgb.GetImages (), WIC_FLAGS_DITHER,
                                  GetWICCodec (hdr ? WIC_CODEC_WMP :
                                                     WIC_CODEC_PNG),
                                       wszAbsolutePathToLossless,
                                         hdr ? (bUseCompatHacks ?                  &GUID_WICPixelFormat64bppRGBAHalf :
                                                                                   &GUID_WICPixelFormat48bppRGBHalf)
                                             :   pFrameData->dxgi.NativeFormat == DXGI_FORMAT_R10G10B10A2_UNORM ?
                                                                                   &GUID_WICPixelFormat48bppRGB :
                                                                                   &GUID_WICPixelFormat24bppBGR,
                          [&](IPropertyBag2* props)
                          {
                            if (hdr && (! bUseCompatHacks))
                            {
                              PROPBAG2 options [2] = { };
                              VARIANT  vars    [2] = { };

                              options [0].pstrName = L"UseCodecOptions";
                              vars    [0].vt       = VT_BOOL;
                              vars    [0].boolVal  = VARIANT_TRUE;

                              options [1].pstrName = L"Quality";
                              vars    [1].vt       = VT_UI1;

                              // Lossless
                              if (config.screenshots.compression_quality == 100)
                                vars  [1].bVal = 1;
                              else
                                vars  [1].bVal =
                                  std::max ( 1ui8, static_cast <uint8_t> (255 - static_cast <uint8_t> (255.0f * (static_cast <float> (config.screenshots.compression_quality) / 100.0f))) );

                              props->Write (2, options, vars);
                            }
                          }, [&](IWICMetadataQueryWriter *pMQW)
                             {
                               SK_WIC_SetMetadataTitle (pMQW, pFrameData->title);
                             }
                        )  : E_POINTER;
                      }

                      if (SUCCEEDED (hrSaveToWIC))
                      {
                        // Refresh
                        rb.screenshot_mgr->getRepoStats (true);
                      }

                      else
                      {
                        SK_LOG0 ( ( L"Unable to write Screenshot, hr=%s",
                                                       SK_DescribeHRESULT (hrSaveToWIC) ),
                                    L"D3D11SShot" );

                        SK_ImGui_Warning ( L"Smart Screenshot Capture Failed.\n\n"
                                           L"\t\t\t\t>> More than likely this is a problem with MSAA or Windows 7\n\n"
                                           L"\t\tTo prevent future problems, disable this under Steam Enhancements / Screenshots" );
                      }
                    }

                    delete pFrameData;
                  }
                }
              }

              SK_Thread_CloseSelf ();

              return 0;
            }, L"[SK] D3D11 Screenshot Encoder",
              (LPVOID)&raw_images_ );
          } }

          if (enqueued_lossless > 0)
          {   enqueued_lossless = 0;
            SetEvent (signal.hq_encode);
          }
        }
      } while (0 == ReadAcquire (&__SK_DLL_Ending));

      SK_Thread_CloseSelf ();

      SK_CloseHandle (signal.capture);
      SK_CloseHandle (signal.abort.initiate);
      SK_CloseHandle (signal.abort.finished);
      SK_CloseHandle (signal.hq_encode);

      return 0;
    }, L"[SK] D3D11 Screenshot Capture" );

    InterlockedIncrement (&_lockVal);
  }

  else
    SK_Thread_SpinUntilAtomicMin (&_lockVal, 2);


  if (stage_ != SK_ScreenshotStage::_FlushQueue && wait == false && purge == false)
    return;


  // Any incomplete captures are pushed onto this queue, and then the pending
  //   queue (once drained) is re-built.
  //
  //  This is faster than iterating a synchronized list in highly multi-threaded engines.
  static concurrency::concurrent_queue <SK_D3D11_Screenshot *> rejected_screenshots;

  bool new_jobs = false;

  do
  {
    do
    {
      SK_D3D11_Screenshot*            pop_off   = nullptr;
      if ( screenshot_queue->try_pop (pop_off) &&
                                      pop_off  != nullptr )
      {
        if (purge)
          delete pop_off;

        else
        {
          UINT     Width, Height;
          uint8_t* pData;

          // There is a condition in which waiting is necessary;
          //   after a swapchain resize or fullscreen mode switch.
          //
          //  * For now, ignore this problem until it actuall poses one.
          //
          if (pop_off->getData (&Width, &Height, &pData, wait))
          {
            screenshot_write_queue->push (pop_off);
            new_jobs = true;
          }

          else
            rejected_screenshots.push (pop_off);
        }
      }
    } while ((! screenshot_queue->empty ()) && (purge || wait));

    do
    {
      SK_D3D11_Screenshot*               push_back   = nullptr;
      if ( rejected_screenshots.try_pop (push_back) &&
                                         push_back  != nullptr )
      {
        if (purge)
          delete push_back;

        else
          screenshot_queue->push (push_back);
      }
    } while ((! rejected_screenshots.empty ()) && (purge || wait));

    if ( wait ||
                 purge )
    {
      if ( screenshot_queue->empty    () &&
           rejected_screenshots.empty ()    )
      {
        if ( purge && (! screenshot_write_queue->empty ()) )
        {
          SetThreadPriority   ( hWriteThread,          THREAD_PRIORITY_TIME_CRITICAL );
          SignalObjectAndWait ( signal.abort.initiate,
                                signal.abort.finished, INFINITE,              FALSE  );
        }

        wait  = false;
        purge = false;

        break;
      }
    }
  } while ( wait ||
                    purge );

  if (new_jobs)
    SetEvent (signal.capture);
}

void
SK_D3D11_ProcessScreenshotQueue (SK_ScreenshotStage stage)
{
  SK_D3D11_ProcessScreenshotQueueEx (stage, false);
}


void
SK_D3D11_WaitOnAllScreenshots (void)
{
}



void SK_Screenshot_D3D11_EndFrame (void)
{
  if (InterlockedCompareExchange (&__SK_D3D11_InitiateHudFreeShot, 0, -1) == -1)
  {
    SK_D3D11_ShowGameHUD ();
  }
}

bool SK_Screenshot_D3D11_BeginFrame (void)
{
  InterlockedExchange (&__SK_ScreenShot_CapturingHUDless, 0);

  if ( ReadAcquire (&__SK_D3D11_QueuedShots)          > 0 ||
       ReadAcquire (&__SK_D3D11_InitiateHudFreeShot) != 0    )
  {
    InterlockedExchange            (&__SK_ScreenShot_CapturingHUDless, 1);
    if (InterlockedCompareExchange (&__SK_D3D11_InitiateHudFreeShot, -2, 1) == 1)
    {
      SK_D3D11_HideGameHUD ();
    //InterlockedDecrement (&SK_D3D11_DrawTrackingReqs); (Handled by ShowGameHUD)
    }

    // 1-frame Delay for SDR->HDR Upconversion
    else if (InterlockedCompareExchange (&__SK_D3D11_InitiateHudFreeShot, -1, -2) == -2)
    {
      SK::SteamAPI::TakeScreenshot (SK_ScreenshotStage::BeforeOSD);
    }

    else if (0 == ReadAcquire (&__SK_D3D11_InitiateHudFreeShot))
    {
      InterlockedDecrement (&__SK_D3D11_QueuedShots);
      InterlockedExchange  (&__SK_D3D11_InitiateHudFreeShot, 1);

      return true;
    }
  }

  return false;
}