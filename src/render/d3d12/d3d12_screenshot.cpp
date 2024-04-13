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
#include <SpecialK/render/d3d12/d3d12_screenshot.h>

#ifndef __SK_SUBSYSTEM__
#define __SK_SUBSYSTEM__ L"D3D12SShot"
#endif

// Not Fully Implemented for D3D12
bool SK_D3D12_EnableTracking      = false;
bool SK_D3D12_EnableMMIOTracking  = false;
volatile LONG
     SK_D3D12_DrawTrackingReqs    = 0L;
volatile LONG
     SK_D3D12_CBufferTrackingReqs = 0L;

HRESULT
SK_D3D12_CaptureScreenshot (
  SK_D3D12_Screenshot   *pScreenshot,
  D3D12_RESOURCE_STATES  beforeState = D3D12_RESOURCE_STATE_PRESENT,
  D3D12_RESOURCE_STATES  afterState  = D3D12_RESOURCE_STATE_PRESENT
);

SK_D3D12_Screenshot& SK_D3D12_Screenshot::operator= (SK_D3D12_Screenshot&& moveFrom)
{
  if (this != &moveFrom)
  {
    dispose ();

    if (moveFrom.readback_ctx.pFence.p != nullptr)
    {
      auto &&rFrom = moveFrom.readback_ctx;
      auto &&rTo   =          readback_ctx;

      rTo.pBackbufferSurface.p       = std::exchange (rFrom.pBackbufferSurface.p,     nullptr);
      rTo.pStagingBackbufferCopy.p   = std::exchange (rFrom.pStagingBackbufferCopy.p, nullptr);

      rTo.pCmdQueue.p                = std::exchange (rFrom.pCmdQueue.p,              nullptr);
      rTo.pCmdAlloc.p                = std::exchange (rFrom.pCmdAlloc.p,              nullptr);
      rTo.pCmdList.p                 = std::exchange (rFrom.pCmdList.p,               nullptr);

      rTo.pFence.p                   = std::exchange (rFrom.pFence.p,                 nullptr);
      rTo.uiFenceVal                 = std::exchange (rFrom.uiFenceVal,               0);

      rTo.pBackingStore              = std::exchange (rFrom.pBackingStore,            nullptr);
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
  _ShaderType *shader   = nullptr;
  ID3D10Blob  *bytecode = nullptr;

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

  bool compileShaderString ( const char*    szShaderString,
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
      compilerOutput;

    HRESULT hr =
      SK_D3D_Compile ( szShaderString,
                strlen (szShaderString),
                  nullptr, nullptr, nullptr,
                    szEntryPoint, szShaderModel,
                      0, 0,
                        &bytecode.p,
                          &compilerOutput.p );

    if (FAILED (hr))
    {
      if ( compilerOutput != nullptr     &&
           compilerOutput->GetBufferSize () > 0 )
      {
        std::string
          err;
          err.reserve (
            compilerOutput->GetBufferSize ()
          );
          err =
        ( (char *)compilerOutput->GetBufferPointer () );

        if (! err.empty ())
        {
          dll_log->LogEx ( true,
                             L"SK D3D12 Shader (SM=%hs) [%ws]: %hs",
                               szShaderModel, wszShaderName,
                                 err.c_str ()
                         );
        }
      }

      return false;
    }

    HRESULT hrCompile =
      DXGI_ERROR_NOT_CURRENTLY_AVAILABLE;

    SK_ComQIPtr <ID3D11Device> pDev (SK_GetCurrentRenderBackend ().device);

    if ( pDev != nullptr &&
         std::type_index ( typeid (    _ShaderType   ) ) ==
         std::type_index ( typeid (ID3D11VertexShader) ) )
    {
      hrCompile =
        pDev->CreateVertexShader (
           static_cast <DWORD *> ( bytecode->GetBufferPointer () ),
                                   bytecode->GetBufferSize    (),
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
           static_cast <DWORD *> ( bytecode->GetBufferPointer () ),
                                   bytecode->GetBufferSize    (),
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

  bool compileShaderFile ( const wchar_t* wszShaderFile,
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

        SK_LOGs0 ( L"D3DCompile", L"Shader Compile Failed: File '%ws' Is Not Valid!",
                      wstr_file.c_str () );
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

      memset ( data.get (),             0,
        sk::narrow_cast <size_t> (size) + 32 );

      fread  ( data.get (),             1,
        sk::narrow_cast <size_t> (size) + 2,
                                     fShader );

      fclose (fShader);

      const bool result =
        compileShaderString ( data.get (), wstr_file.c_str (),
                                szEntryPoint, szShaderModel,
                                  recompile,  error_log );

      return result;
    }

    SK_LOGs0 ( L"D3DCompile",
            "Cannot Compile Shader Because of Filesystem Issues" );

    return  false;
  }

  void releaseResources (void)
  {
    if (shader   != nullptr) {   shader->Release (); shader   = nullptr; }
    if (bytecode != nullptr) { bytecode->Release (); bytecode = nullptr; }
  }
};

SK_D3D12_Screenshot::SK_D3D12_Screenshot ( const SK_ComPtr <ID3D12Device>&       pDevice,
                                           const SK_ComPtr <ID3D12CommandQueue>& pCmdQueue,
                                           const SK_ComPtr <IDXGISwapChain3>&    pSwapChain,
                                                 bool                            allow_sound,
                                                 bool                            clipboard_only,
                                                 std::string                     title) : SK_Screenshot (clipboard_only)
{
  if (! title.empty ())
  {
    framebuffer.file_name = SK_UTF8ToWideChar (title);
    framebuffer.title     = title;

    PathStripPathA (framebuffer.title.data ());

    sanitizeFilename (true);
  }

  bPlaySound = allow_sound;

  readback_ctx.pBackingStore = &framebuffer;

  //SK_ScopedBool decl_tex_scope (
  //  SK_D3D12_DeclareTexInjectScope ()
  //);

  if ( pDevice    != nullptr &&
       pSwapChain != nullptr &&
       pCmdQueue  != nullptr )
  {
    readback_ctx.pCmdQueue = pCmdQueue;

#ifdef HDR_CONVERT
    static auto& rb =
      SK_GetCurrentRenderBackend ();
#endif

    SK_ComQIPtr <IDXGISwapChain4>
        pSwap4 (pSwapChain);

    if (pSwap4.p != nullptr)
    {
      DXGI_SWAP_CHAIN_DESC1 sd1 = {};
      pSwap4->GetDesc1    (&sd1);

      framebuffer.dxgi.AlphaMode =
                   sd1.AlphaMode;
    }
    ulCommandIssuedOnFrame = SK_GetFramesDrawn ();

    SK_ComPtr <ID3D12Resource> pBackbuffer;

    readback_ctx.pCmdList  = _d3d12_rbk->frames_ [pSwapChain->GetCurrentBackBufferIndex ()].pCmdList.p;
    readback_ctx.pCmdAlloc = _d3d12_rbk->frames_ [pSwapChain->GetCurrentBackBufferIndex ()].pCmdAllocator.p;
    pBackbuffer            = _d3d12_rbk->frames_ [pSwapChain->GetCurrentBackBufferIndex ()].pBackBuffer.p;

    readback_ctx.pBackbufferSurface            = pBackbuffer;
    D3D12_RESOURCE_DESC        backbuffer_desc = pBackbuffer->GetDesc ();

    framebuffer.Width             = sk::narrow_cast <UINT> (backbuffer_desc.Width);
    framebuffer.Height            = sk::narrow_cast <UINT> (backbuffer_desc.Height);
    framebuffer.dxgi.NativeFormat = backbuffer_desc.Format;

#ifdef HDR_CONVERT
    SK_ComPtr <ID3D12Resource> pHDRConvertTex;

    bool hdr10_to_scRGB = false;

    bool hdr =
      (  rb.isHDRCapable ()  &&
        (rb.framebuffer_flags & SK_FRAMEBUFFER_FLAG_HDR) );

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

          static ShaderBase <ID3D11PixelShader>  PixelShader_HDR10toscRGB;
          static ShaderBase <ID3D11VertexShader> VertexShaderHDR_Util;

        //static std::wstring debug_shader_dir = SK_GetConfigPath ();

          static bool compiled = true;

          SK_RunOnce ( compiled =
            PixelShader_HDR10toscRGB.compileShaderString (
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
              }", L"HDR10->scRGB Color Transform", "main", "ps_5_0", true )
          );

          SK_RunOnce ( compiled &=
            VertexShaderHDR_Util.compileShaderString (
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
                   "main", "vs_5_0", true )
          );

          if (compiled)
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
            pImmediateCtx->VSSetShader          (VertexShaderHDR_Util.shader,     nullptr, 0);
            pImmediateCtx->PSSetShader          (PixelShader_HDR10toscRGB.shader, nullptr, 0);
            pImmediateCtx->GSSetShader          (nullptr,                         nullptr, 0);
            pImmediateCtx->HSSetShader          (nullptr,                         nullptr, 0);
            pImmediateCtx->DSSetShader          (nullptr,                         nullptr, 0);

            pImmediateCtx->PSSetShaderResources (0, 1, pResources);
            pImmediateCtx->OMSetRenderTargets   (1, &pRenderTargetView.p, nullptr);

            static bool run_once = false;

            static D3D11_RASTERIZER_DESC    raster_desc = { };
            static D3D11_DEPTH_STENCIL_DESC depth_desc  = { };
            static D3D11_BLEND_DESC         blend_desc  = { };

            if (! run_once)
            {
              raster_desc.FillMode        = D3D11_FILL_SOLID;
              raster_desc.CullMode        = D3D11_CULL_NONE;
              raster_desc.ScissorEnable   = false;
              raster_desc.DepthClipEnable = true;

              depth_desc.DepthEnable      = false;
              depth_desc.DepthWriteMask   = D3D11_DEPTH_WRITE_MASK_ALL;
              depth_desc.DepthFunc        = D3D11_COMPARISON_ALWAYS;
              depth_desc.StencilEnable    = false;
              depth_desc.FrontFace.StencilFailOp = depth_desc.FrontFace.StencilDepthFailOp =
                                                   depth_desc.FrontFace.StencilPassOp      =
                                                 D3D11_STENCIL_OP_KEEP;
              depth_desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
              depth_desc.BackFace              = depth_desc.FrontFace;

              blend_desc.AlphaToCoverageEnable                  = false;
              blend_desc.RenderTarget [0].BlendEnable           = true;
              blend_desc.RenderTarget [0].SrcBlend              = D3D11_BLEND_ONE;
              blend_desc.RenderTarget [0].DestBlend             = D3D11_BLEND_ZERO;
              blend_desc.RenderTarget [0].BlendOp               = D3D11_BLEND_OP_ADD;
              blend_desc.RenderTarget [0].SrcBlendAlpha         = D3D11_BLEND_ONE;
              blend_desc.RenderTarget [0].DestBlendAlpha        = D3D11_BLEND_ZERO;
              blend_desc.RenderTarget [0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
              blend_desc.RenderTarget [0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
            }

            SK_RunOnce (run_once = true);

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

            hdr10_to_scRGB           = true;
            framebuffer.NativeFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
          }
        }
      }
    }
#endif
#if 0
    const uint32_t data_pitch     = framebuffer.Width * 4;
    const uint32_t download_pitch = (data_pitch + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u)
                                              & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u);

    D3D12_RESOURCE_DESC
      staging_desc                  = { D3D12_RESOURCE_DIMENSION_BUFFER };
      staging_desc.Width            = framebuffer.Height * download_pitch;
      staging_desc.Height           = 1;
      staging_desc.DepthOrArraySize = 1;
      staging_desc.MipLevels        = 1;
      staging_desc.SampleDesc       = { 1, 0 };
      staging_desc.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    D3D12_HEAP_PROPERTIES
      heapProps                     = { D3D12_HEAP_TYPE_READBACK };

    if ( SUCCEEDED (
      pDev->CreateCommittedResource ( &heapProps, D3D12_HEAP_FLAG_NONE,
        &staging_desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
          __uuidof (ID3D12Resource),
            (void **)&pReadback->pStagingBackbufferCopy.p )
                   )
       )
    {
      // DXGI Flip Model Does Not Allow This, so ignore...
      SK_ReleaseAssert (backbuffer_desc.SampleDesc.Count == 1);

      if (backbuffer_desc.SampleDesc.Count == 1)
      {
        if (hdr10_to_scRGB)
        {
          //pImmediateCtx->CopyResource ( pStagingBackbufferCopy,
          //                              pHDRConvertTex          );
        }

        else
        {
          //pImmediateCtx->CopyResource ( pStagingBackbufferCopy,
          //                              pBackbufferSurface      );
        }
      }
#endif
    HRESULT hr =
      SK_D3D12_CaptureScreenshot (this);

    if (SUCCEEDED (hr))
    {
      if (bPlaySound)
        SK_Screenshot_PlaySound ();

      return;
    }
  }


  extern void SK_Steam_CatastropicScreenshotFail (void);
              SK_Steam_CatastropicScreenshotFail ();

  // Something went wrong, crap!
  dispose ();
}

void
SK_D3D12_Screenshot::dispose (void) noexcept
{
  readback_ctx.pFence                 = nullptr;
  readback_ctx.uiFenceVal             =       0;

  readback_ctx.pStagingBackbufferCopy = nullptr;
  readback_ctx.pBackbufferSurface     = nullptr;

  readback_ctx.pCmdQueue              = nullptr;
  readback_ctx.pCmdAlloc              = nullptr;
  readback_ctx.pCmdList               = nullptr;

  // Hopefully we still point back to ourself
  SK_ReleaseAssert (readback_ctx.pBackingStore == &framebuffer);

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


inline void
SK_D3D12_TransitionResource (
    _In_ ID3D12GraphicsCommandList *pCmdList,
    _In_ ID3D12Resource            *pResource,
    _In_ D3D12_RESOURCE_STATES      stateBefore,
    _In_ D3D12_RESOURCE_STATES      stateAfter )
{
  assert (pCmdList  != nullptr);
  assert (pResource != nullptr);

  if (stateBefore == stateAfter)
      return;

  D3D12_RESOURCE_BARRIER
    barrier_desc                        = {                                    };
    barrier_desc.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier_desc.Transition.pResource   = pResource;
    barrier_desc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier_desc.Transition.StateBefore = stateBefore;
    barrier_desc.Transition.StateAfter  = stateAfter;

  pCmdList->ResourceBarrier (
    1, &barrier_desc
  );
}

#include <../depends/include/DirectXTex/d3dx12.h>

//--------------------------------------------------------------------------------------
HRESULT
SK_D3D12_CaptureScreenshot (
  SK_D3D12_Screenshot   *pScreenshot,
  D3D12_RESOURCE_STATES  beforeState,
  D3D12_RESOURCE_STATES  afterState
)
{
  auto pStagingCtx =
    pScreenshot->getReadbackContext ();

  auto pCmdQueue =
    pStagingCtx->pCmdQueue;

  auto pSource =
    pStagingCtx->pBackbufferSurface;

  /// TODO: This should be a passed argument, rather than stored.
  pStagingCtx->pBackbufferSurface.Release ();

  SK_ComPtr <ID3D12Device> pDevice;

  if (FAILED (pCmdQueue->GetDevice (IID_PPV_ARGS (&pDevice.p))))
    return E_INVALIDARG;

  D3D12_HEAP_PROPERTIES sourceHeapProperties = { };
  D3D12_HEAP_FLAGS      sourceHeapFlags      = { };

  HRESULT hr =
    pSource->GetHeapProperties ( &sourceHeapProperties,
                                 &sourceHeapFlags );

  if (FAILED (hr))
       return hr;

  const D3D12_RESOURCE_DESC desc =
     pSource->GetDesc ();

  // Resource must be single-sampled with no mipmaps and be 64 KiB aligned.
  //
  SK_ReleaseAssert (desc.MipLevels          <= 1);
  SK_ReleaseAssert (desc.SampleDesc.Count   == 1 &&
                    desc.SampleDesc.Quality == 0);
  SK_ReleaseAssert (desc.Alignment          == 0 ||
                    desc.Alignment          == 65536);

  D3D12_PLACED_SUBRESOURCE_FOOTPRINT
         layout            = {  };
  UINT64 totalResourceSize = 0ULL;

  pDevice->GetCopyableFootprints (
    &desc, 0, 1, 0,
      &layout, nullptr,
               nullptr,
                 &totalResourceSize
  );

  SK_Screenshot::framebuffer_s *pBackingStore =
                   pStagingCtx->pBackingStore;

  pBackingStore->PixelBuffer.reset (
    new (std::nothrow) uint8_t [static_cast <SIZE_T> (totalResourceSize)]
  );

  if (! pBackingStore->PixelBuffer)
    return E_OUTOFMEMORY;

  pBackingStore->PBufferSize =
    static_cast <SIZE_T> (totalResourceSize);

  CD3DX12_HEAP_PROPERTIES defaultHeapProperties  (D3D12_HEAP_TYPE_DEFAULT);
  CD3DX12_HEAP_PROPERTIES readBackHeapProperties (D3D12_HEAP_TYPE_READBACK);

  // Readback resources must be buffers
  D3D12_RESOURCE_DESC
    bufferDesc                    = {};
    bufferDesc.Alignment          = desc.Alignment;
    bufferDesc.DepthOrArraySize   = 1;
    bufferDesc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Flags              = D3D12_RESOURCE_FLAG_NONE;
    bufferDesc.Format             = DXGI_FORMAT_UNKNOWN;
    bufferDesc.Height             = 1;
    bufferDesc.Width              = pBackingStore->PBufferSize;
    bufferDesc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufferDesc.MipLevels          = 1;
    bufferDesc.SampleDesc.Count   = 1;
    bufferDesc.SampleDesc.Quality = 0;

  DirectX::ComputePitch (
    pBackingStore->dxgi.NativeFormat,
      static_cast <size_t> (pBackingStore->Width),
      static_cast <size_t> (pBackingStore->Height),
        pBackingStore->PackedDstPitch,
        pBackingStore->PackedDstSlicePitch
   );

  // Since we're going to be screenshotting the SwapChain backbuffer,
  //   there's no way it could be MSAA in D3D12 (Flip Model)
  SK_ReleaseAssert (desc.SampleDesc.Count <= 1);

  // Create a staging texture
  hr =
    pDevice->CreateCommittedResource (
      &readBackHeapProperties, D3D12_HEAP_FLAG_NONE,
      &bufferDesc,             D3D12_RESOURCE_STATE_COPY_DEST,
          nullptr,
      IID_PPV_ARGS (&pStagingCtx->pStagingBackbufferCopy.p)
    );

  if (FAILED (hr))
       return hr;

  SK_D3D12_SetDebugName (
    pStagingCtx->pStagingBackbufferCopy.p,
    L"SK D3D12 Screenshot Backbuffer Copy"
  );

  auto& d3d12_rbk =
    _d3d12_rbk.get ();

  bool bRecording =
    d3d12_rbk.frames_ [d3d12_rbk._pSwapChain->GetCurrentBackBufferIndex ()].bCmdListRecording;

  if (! bRecording)
  {
    // This can now happen if post-Present screenshots are taken
    //SK_ReleaseAssert (!"D3D12 Screenshot Initiated While SK Was Not Recording A Command List!");

    d3d12_rbk.frames_ [d3d12_rbk._pSwapChain->GetCurrentBackBufferIndex ()].begin_cmd_list ();

    beforeState = D3D12_RESOURCE_STATE_PRESENT;
    afterState  = D3D12_RESOURCE_STATE_PRESENT;
  }

  else
  {
    beforeState = D3D12_RESOURCE_STATE_RENDER_TARGET;
    afterState  = D3D12_RESOURCE_STATE_RENDER_TARGET;
  }

  // Transition the resource if necessary
  SK_D3D12_TransitionResource ( pStagingCtx->pCmdList, pSource,
                 beforeState, D3D12_RESOURCE_STATE_COPY_SOURCE );

  CD3DX12_TEXTURE_COPY_LOCATION copyDest (pStagingCtx->pStagingBackbufferCopy, layout);
  CD3DX12_TEXTURE_COPY_LOCATION copySrc  (pSource,                                  0);

  copyDest.PlacedFootprint.Footprint.Format = pBackingStore->dxgi.NativeFormat;
   copySrc.PlacedFootprint.Footprint.Format = pBackingStore->dxgi.NativeFormat;

  // NOTE: This will pass through SK's hook, the aligned data size may be different than expected
  pStagingCtx->pCmdList->CopyTextureRegion ( &copyDest, 0, 0,
                                          0, &copySrc, nullptr );

  // Transition the resource to the next state
  SK_D3D12_TransitionResource ( pStagingCtx->pCmdList, pSource,
                  D3D12_RESOURCE_STATE_COPY_SOURCE, afterState );


  d3d12_rbk.frames_ [d3d12_rbk._pSwapChain->GetCurrentBackBufferIndex ()].exec_cmd_list  ();
  d3d12_rbk.frames_ [d3d12_rbk._pSwapChain->GetCurrentBackBufferIndex ()].begin_cmd_list ();

  // Create a fence
  hr =
    pDevice->CreateFence ( 0, D3D12_FENCE_FLAG_NONE,
                 IID_PPV_ARGS (&pStagingCtx->pFence.p) );

  if (FAILED (hr))
       return hr;

  ULONG64 ulFenceFrame =
    SK_GetFramesDrawn ();

  SK_D3D12_SetDebugName (              pStagingCtx->pFence,
       SK_FormatStringW ( L"SK Screenshot Completion Fence (%lu)",
                                                   ulFenceFrame ).c_str () );

  pStagingCtx->uiFenceVal =
               ulFenceFrame;

  // Signal the fence
  hr =
    pCmdQueue->Signal ( pStagingCtx->pFence,
                        pStagingCtx->uiFenceVal + 1 );

  return hr;
}

bool
SK_D3D12_Screenshot::getData ( UINT* const pWidth,
                               UINT* const pHeight,
                               uint8_t   **ppData,
                               bool        Wait )
{
  auto ReadBack =
  [&]
  {
    auto pStagingCtx =
      getReadbackContext ();

    auto& pStagingBuffer =
      pStagingCtx->pStagingBackbufferCopy;

    if (! pStagingBuffer)
      return false;

    SK_ComPtr <ID3D12Device> pDevice;

    pStagingBuffer->GetDevice (
      IID_PPV_ARGS (&pDevice.p)
    );

    BYTE *pData = nullptr;

    UINT64         uiFenceComplete =
      pStagingCtx->uiFenceVal + 1;

    if ( pStagingCtx->pFence->GetCompletedValue () <
                     uiFenceComplete )
    {
      if (! Wait)
        return false;

      while ( pStagingCtx->pFence->GetCompletedValue () <
                          uiFenceComplete )
        SK_SleepEx (1UL, FALSE);
    }

    HRESULT hr =
      pStagingBuffer->Map (0, nullptr, reinterpret_cast <void **> (&pData));

    if (FAILED (hr))
      return false;

    SK_ReleaseAssert (
          framebuffer.PBufferSize ==
      GetRequiredIntermediateSize (pStagingBuffer, 0, 1)
    );

    D3D12_MEMCPY_DEST destData = {
      framebuffer.PixelBuffer.get (),
      framebuffer.PackedDstPitch,
      framebuffer.PackedDstSlicePitch
    };

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout            = {  };
    UINT64                             totalResourceSize = 0ULL;

    const auto staging_desc =
      pStagingBuffer->GetDesc ();

    UINT64 RowSizeInBytes = 0ULL;
    UINT   NumRows        = 0U;

    pDevice->GetCopyableFootprints (
      &staging_desc, 0, 1, 0,
        &layout, &NumRows,
                    &RowSizeInBytes,
          &totalResourceSize
    );

    SK_ReleaseAssert (totalResourceSize == framebuffer.PBufferSize);

    D3D12_SUBRESOURCE_DATA srcData =
    {
                      pData + layout.Offset,
      static_cast <LONG_PTR> (layout.Footprint.RowPitch),
      static_cast <LONG_PTR> (layout.Footprint.RowPitch) *
      static_cast <LONG_PTR> (NumRows)
    };

    SK_ReleaseAssert (RowSizeInBytes <= (SIZE_T)-1);

    if (RowSizeInBytes > (SIZE_T)-1)
    {
      pStagingBuffer->Unmap (0, nullptr);

      return false;
    }

    MemcpySubresource ( &destData, &srcData,
           (SIZE_T)RowSizeInBytes,  NumRows, 1 );

    SK_LOGi0 ( L"Screenshot Readback Complete after %li frames",
                SK_GetFramesDrawn () - ulCommandIssuedOnFrame );

    *pWidth  = static_cast <UINT> (framebuffer.Width);
    *pHeight = static_cast <UINT> (framebuffer.Height);
    *ppData  =
      framebuffer.PixelBuffer.get ();

    uint8_t* pSrc = *ppData;
    uint8_t* pDst = *ppData;
    
    auto bitsPerPixel  =
      DirectX::BitsPerPixel (framebuffer.dxgi.NativeFormat);
    auto bytesPerPixel = bitsPerPixel / 8;

    // We can only deal with 10:10:10:2, 8:8:8:8, or 16:16:16:16
    SK_ReleaseAssert ( bitsPerPixel == 32 ||
                       bitsPerPixel == 64 );

    SK_ReleaseAssert ( bytesPerPixel * framebuffer.Width <= framebuffer.PackedDstPitch );

    size_t src_row_pitch =
      ( static_cast <size_t> (layout.Footprint.RowPitch) / std::max (static_cast <size_t> (1), static_cast <size_t> (framebuffer.Height)) ) +
      ( static_cast <size_t> (layout.Footprint.RowPitch) / std::max (static_cast <size_t> (1), static_cast <size_t> (framebuffer.Height)) ) % D3D12_TEXTURE_DATA_PITCH_ALIGNMENT,
           dst_row_pitch = ( bytesPerPixel * framebuffer.Width );

    SK_ReleaseAssert ( src_row_pitch >=
                       dst_row_pitch );

    // Compact (in-place) the copied memory per-scanline
    for ( UINT scanline = 0 ;
               scanline < framebuffer.Height ;
             ++scanline )
    {
      memcpy ( pDst, pSrc, dst_row_pitch );

      pSrc += src_row_pitch;
      pDst += dst_row_pitch;
    }

    pStagingBuffer->Unmap (0, nullptr);

    return true;

#ifdef FINISHED_D3D12
    const UINT   Subresource =
      D3D12CalcSubresource ( 0, 0, 0, 1, 0);

    D3D11_MAPPED_SUBRESOURCE finished_copy = { };

    auto& pooled =
      SK_ScreenshotQueue::pooled;

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
        framebuffer.NativeFormat,
          framebuffer.Width, framebuffer.Height,
            PackedDstPitch, PackedDstSlicePitch
       );

      framebuffer.PackedDstSlicePitch = PackedDstSlicePitch;
      framebuffer.PackedDstPitch      = PackedDstPitch;

      size_t allocSize =
        (framebuffer.Height + 1)
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

      SK_LOG0 ( ( L"Screenshot Readback Complete after %li frames",
                    SK_GetFramesDrawn () - ulCommandIssuedOnFrame ),
                  L"D3D12SShot" );

      pImmediateCtx->Unmap ( pStagingBackbufferCopy,
                                       Subresource );

      *ppData =
        framebuffer.PixelBuffer.get ();

      return true;
    }
#else
    //UNREFERENCED_PARAMETER (pWidth);
    //UNREFERENCED_PARAMETER (pHeight);
    //UNREFERENCED_PARAMETER (ppData);
#endif

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


extern volatile LONG __SK_ScreenShot_CapturingHUDless;

volatile LONG __SK_D3D12_QueuedShots           = 0;
volatile LONG __SK_D3D12_InitiateHudFreeShot   = 0;

SK_LazyGlobal <concurrency::concurrent_queue <SK_D3D12_Screenshot *>> screenshot_queue;
SK_LazyGlobal <concurrency::concurrent_queue <SK_D3D12_Screenshot *>> screenshot_write_queue;


//static volatile LONG
//  __SK_HUD_YesOrNo = 1L;
//
//bool
//SK_D3D12_ShouldSkipHUD (void)
//{
//  return
//    ( SK_Screenshot_IsCapturingHUDless () ||
//       ReadAcquire    (&__SK_HUD_YesOrNo) <= 0 );
//}

//LONG
//SK_D3D12_ShowGameHUD (void)
//{
//  InterlockedDecrement (&SK_D3D12_DrawTrackingReqs);
//
//  return
//    InterlockedIncrement (&__SK_HUD_YesOrNo);
//}
//
//LONG
//SK_D3D12_HideGameHUD (void)
//{
//  InterlockedIncrement (&SK_D3D12_DrawTrackingReqs);
//
//  return
//    InterlockedDecrement (&__SK_HUD_YesOrNo);
//}
//
//LONG
//SK_D3D12_ToggleGameHUD (void)
//{
//  static volatile LONG last_state =
//    (ReadAcquire (&__SK_HUD_YesOrNo) > 0);
//
//  if (ReadAcquire (&last_state) != 0)
//  {
//    SK_D3D12_HideGameHUD ();
//
//    return
//      InterlockedDecrement (&last_state);
//  }
//
//  else
//  {
//    SK_D3D12_ShowGameHUD ();
//
//    return
//      InterlockedIncrement (&last_state);
//  }
//}

void
SK_Screenshot_D3D12_RestoreHUD (void)
{
  if ( -1 ==
         InterlockedCompareExchange (
           &__SK_D3D12_InitiateHudFreeShot, 0, -1
         )
     )
  {
    SK_D3D12_ShowGameHUD ();
  }
}

bool
SK_D3D12_RegisterHUDShader (        uint32_t  bytecode_crc32c,
                             std::type_index _T,
                                        bool  remove )
{
#ifdef SK_D3D12_HUDLESS
  SK_D3D12_KnownShaders::ShaderRegistry <IUnknown>* record =
    (SK_D3D12_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D12_Shaders->vertex;
  K
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
    std::unordered_map < _ShaderType, SK_D3D12_KnownShaders::ShaderRegistry <IUnknown>* >
      __record_map =
      {
        { _ShaderType::Vertex  , (SK_D3D12_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D12_Shaders->vertex   },
        { _ShaderType::Pixel   , (SK_D3D12_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D12_Shaders->pixel    },
        { _ShaderType::Geometry, (SK_D3D12_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D12_Shaders->geometry },
        { _ShaderType::Domain  , (SK_D3D12_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D12_Shaders->domain   },
        { _ShaderType::Hull    , (SK_D3D12_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D12_Shaders->hull     },
        { _ShaderType::Compute , (SK_D3D12_KnownShaders::ShaderRegistry <IUnknown>*)&SK_D3D12_Shaders->compute  }
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
    InterlockedIncrement (&SK_D3D12_TrackingCount->Conditional);

    return
      record->addTrackingRef (hud, bytecode_crc32c);
  }

  InterlockedDecrement (&SK_D3D12_TrackingCount->Conditional);

  return
    record->releaseTrackingRef (hud, bytecode_crc32c);
#else
  UNREFERENCED_PARAMETER (bytecode_crc32c);
  UNREFERENCED_PARAMETER (_T);
  UNREFERENCED_PARAMETER (remove);
#endif

  return false;
}

bool
SK_D3D12_UnRegisterHUDShader ( uint32_t         bytecode_crc32c,
                               std::type_index _T               )
{
#ifdef SK_D3D12_HUDLESS
  return
    SK_D3D12_RegisterHUDShader (
      bytecode_crc32c,   _T,   true
    );
#else
  UNREFERENCED_PARAMETER (bytecode_crc32c);
  UNREFERENCED_PARAMETER (_T);
#endif

  return false;
}

bool
SK_D3D12_CaptureScreenshot  ( SK_ScreenshotStage when =
                              SK_ScreenshotStage::EndOfFrame,
                              bool               allow_sound = true,
                              std::string        title       = "" )
{
  static auto& rb =
    SK_GetCurrentRenderBackend ();

  if ( ((int)rb.api & (int)SK_RenderAPI::D3D12)
                   == (int)SK_RenderAPI::D3D12)
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
        //static const auto& vertex = SK_D3D12_Shaders->vertex;
        //static const auto& pixel  = SK_D3D12_Shaders->pixel;

        //if ( vertex.hud.empty () &&
        //      pixel.hud.empty ()    )
        //{
        //  return false;
        //}
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
SK_D3D12_ProcessScreenshotQueueEx ( SK_ScreenshotStage stage_ = SK_ScreenshotStage::EndOfFrame,
                                    bool               wait   = false,
                                    bool               purge  = false )
{
  static auto& rb =
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
          SK_ComQIPtr <ID3D12Device>
              pDev (_d3d12_rbk->_pDevice);

          SK_ReleaseAssert (pDev.IsEqualObject (rb.device));

          if (pDev != nullptr && rb.swapchain != nullptr)
          {
            if ( SK_ScreenshotQueue::pooled.capture_bytes.load  () <
                 SK_ScreenshotQueue::maximum.capture_bytes.load () )
            {
              SK_ReleaseAssert (rb.d3d12.command_queue.p != nullptr);

              const bool clipboard_only =
                ( stage == __MaxStage );

              if (clipboard_only || SK_GetCurrentRenderBackend ().screenshot_mgr->checkDiskSpace (20ULL * 1024ULL * 1024ULL))
              {
                const bool allow_sound =
                  ReadAcquire (&enqueued_sounds.stages [stage]) > 0;

                if ( allow_sound )
                  InterlockedDecrement (&enqueued_sounds.stages [stage]);

                std::string                                title = "";
                std::swap (enqueued_titles.stages [stage], title);

                screenshot_queue->push (
                  new SK_D3D12_Screenshot (
                    pDev, rb.d3d12.command_queue, (IDXGISwapChain3 *)
                          rb.swapchain.p,
                            allow_sound, clipboard_only, title
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


  static volatile LONG           _lockVal = 0;

  struct signals_s {
    HANDLE capture    = INVALID_HANDLE_VALUE;
    HANDLE hq_encode  = INVALID_HANDLE_VALUE;

    struct {
      HANDLE initiate = INVALID_HANDLE_VALUE;
      HANDLE finished = INVALID_HANDLE_VALUE;
    } abort;
  } static signal = { };

  static HANDLE
    hWriteThread        = INVALID_HANDLE_VALUE;


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

        static std::vector <SK_D3D12_Screenshot*> to_write;

        while (! screenshot_write_queue->empty ())
        {
          SK_D3D12_Screenshot*                  pop_off   = nullptr;
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
                  static_cast <size_t> (pFrameData->Width),
                  static_cast <size_t> (pFrameData->Height),
                    raw_img.rowPitch, raw_img.slicePitch
              );

              // Steam wants JPG, smart people want PNG
              DirectX::WICCodecs codec = WIC_CODEC_JPEG;

              raw_img.format = pop_off->getInternalFormat ();
              raw_img.width  = static_cast <size_t> (pFrameData->Width);
              raw_img.height = static_cast <size_t> (pFrameData->Height);
              raw_img.pixels = pFrameData->PixelBuffer.get ();

              bool hdr = ( rb.isHDRCapable () &&
                           rb.isHDRActive  () );

              SK_RunOnce (SK_SteamAPI_InitManagers ());

              wchar_t       wszAbsolutePathToScreenshot [ MAX_PATH + 2 ] = { };
              wcsncpy_s   ( wszAbsolutePathToScreenshot,  MAX_PATH,
                              rb.screenshot_mgr->getBasePath (),
                                _TRUNCATE );

              if ( config.steam.screenshots.enable_hook &&
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

                static const XMVECTORF32 c_MaxNitsFor2084 =
                  { 10000.0f, 10000.0f, 10000.0f, 1.f };

                static const XMMATRIX c_from2020to709 =
                {
                  { 1.6604910f,  -0.1245505f, -0.0181508f, 0.f },
                  { -0.5876411f,  1.1328999f, -0.1005789f, 0.f },
                  { -0.0728499f, -0.0083494f,  1.1187297f, 0.f },
                  { 0.f,          0.f,         0.f,        1.f }
                };

                HRESULT hr = S_OK;

                if (hdr && raw_img.format != DXGI_FORMAT_R16G16B16A16_FLOAT)
                {
                  auto hdr10_metadata        = un_srgb.GetMetadata ();
                       hdr10_metadata.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
                       hdr10_metadata.SetAlphaMode (TEX_ALPHA_MODE_OPAQUE);

                  ScratchImage
                    un_hdr10;
                    un_hdr10.Initialize (hdr10_metadata);

                  DirectX::Convert (raw_img, DXGI_FORMAT_R16G16B16A16_FLOAT, 0x0, 0.0f, un_srgb);

                  struct ParamsPQ
                  {
                    XMVECTOR N, M;
                    XMVECTOR C1, C2, C3;
                    XMVECTOR MaxPQ;
                  };
                  
                  static const ParamsPQ PQ =
                  {
                    XMVectorReplicate (2610.0 / 4096.0 / 4.0),   // N
                    XMVectorReplicate (2523.0 / 4096.0 * 128.0), // M
                    XMVectorReplicate (3424.0 / 4096.0),         // C1
                    XMVectorReplicate (2413.0 / 4096.0 * 32.0),  // C2
                    XMVectorReplicate (2392.0 / 4096.0 * 32.0),  // C3
                    XMVectorReplicate (125.0),
                  };

                  auto PQToLinear = [](XMVECTOR N)
                  {
                    XMVECTOR ret;

                    ret =
                      XMVectorPow (N, XMVectorDivide (g_XMOne, PQ.M));

                    XMVECTOR nd;

                    nd =
                      XMVectorDivide (
                        XMVectorMax (XMVectorSubtract (ret, PQ.C1), g_XMZero),
                                     XMVectorSubtract (     PQ.C2,
                              XMVectorMultiply (PQ.C3, ret)));

                    ret =
                      XMVectorMultiply (
                        XMVectorPow (nd, XMVectorDivide (g_XMOne, PQ.N)), PQ.MaxPQ
                      );

                    return ret;
                  };

                  auto metadata =
                    un_srgb.GetMetadata ();

                  metadata.SetAlphaMode (TEX_ALPHA_MODE_OPAQUE);

                  if (
                    SUCCEEDED (
                      TransformImage ( un_srgb.GetImages     (),
                                       un_srgb.GetImageCount (),
                                       metadata,
                      [&](XMVECTOR* outPixels, const XMVECTOR* inPixels, size_t width, size_t y)
                      {
                        UNREFERENCED_PARAMETER(y);

                        for (size_t j = 0; j < width; ++j)
                        {
                          XMVECTOR value = inPixels [j];

                          outPixels [j] = XMVector3Transform (PQToLinear (value), c_from2020to709);
                          outPixels [j].m128_f32 [3] = 1.0f;
                        }
                      }, un_hdr10    )
                     )        )
                  {
                    std::swap (un_srgb, un_hdr10);
                  }

                  else
                    hdr = false; // Couldn't undo HDR10, don't store a .jxr
                }

                ScratchImage
                  final_sdr;
                  final_sdr.Initialize (meta);

                hr = S_OK;

                static const XMVECTORF32 s_luminance_2020 =
                  { 0.2627,   0.678,    0.0593,   0.f };

                static const XMMATRIX c_from709to2020 =
                {
                  { 0.627225305694944,  0.329476882715808,  0.0432978115892484, 0.0 },
                  { 0.0690418812810714, 0.919605681354755,  0.0113524373641739, 0.0 },
                  { 0.0163911702607078, 0.0880887513437058, 0.895520078395586,  0.0 },
                  { 0.0,                0.0,                0.0,                1.0 }
                };

                //if ( un_srgb.GetImageCount () == 1 &&
                //     hdr                      && raw_img.format != DXGI_FORMAT_R16G16B16A16_FLOAT &&
                //     rb.scanout.getEOTF    () == SK_RenderBackend::scan_out_s::SMPTE_2084 )
                //{ // ^^^ EOTF is not always accurate, but we know SMPTE 2084 is not used w/ FP16 color
                  TransformImage ( un_srgb.GetImages     (),
                                   un_srgb.GetImageCount (),
                                   un_srgb.GetMetadata   (),
                    [&](XMVECTOR* outPixels, const XMVECTOR* inPixels, size_t width, size_t y)
                    {
                      UNREFERENCED_PARAMETER(y);
                    
                      for (size_t j = 0; j < width; ++j)
                      {
                                                      //XMVECTOR value = inPixels [j];
                        outPixels [j]  = inPixels [j];//XMVector3Transform (value, c_from2020to709);
                      }
                    }, un_scrgb
                  );
                
                  std::swap (un_scrgb, un_srgb);
                //}

                if ( un_srgb.GetImageCount () == 1 &&
                     hdr )
                {
                  XMVECTOR maxLum = XMVectorZero          (),
                           minLum = XMVectorSplatInfinity (),
                           maxCLL = XMVectorZero          (),
                           maxRGB = XMVectorZero          ();

                  static const XMVECTORF32 s_luminance =
                    { 0.2126729, 0.7151522, 0.0721750, 0.f };

                  float lumTotal   = 0.0f;
                  float N          = 0.0f;

                  hr =              un_srgb.GetImageCount () == 1 ?
                    EvaluateImage ( un_srgb.GetImages     (),
                                    un_srgb.GetImageCount (),
                                    un_srgb.GetMetadata   (),
                    [&](const XMVECTOR* pixels, size_t width, size_t y)
                    {
                      UNREFERENCED_PARAMETER(y);

                      for (size_t j = 0; j < width; ++j)
                      {
                        XMVECTOR v =
                          XMVectorMax (*pixels, g_XMZero);

                        maxRGB =
                          XMVectorMax (v, maxRGB);

                        maxCLL =
                          XMVectorMax (XMVectorMultiply (v, s_luminance), maxCLL);

                        v = XMVector3Dot (v, s_luminance);

                        maxLum =
                          XMVectorMax (v, maxLum);

                        minLum =
                          XMVectorMin (v, minLum);

                        lumTotal +=
                          logf ( std::max (0.000001f, 0.000001f + v.m128_f32 [0]) ),
                        ++N;

                        pixels++;
                      }
                    }
                  ) : E_POINTER;

                  maxCLL =
                    XMVectorReplicate (
                      std::max ({ maxCLL.m128_f32 [0], maxCLL.m128_f32 [1], maxCLL.m128_f32 [2] })
                    );

                  SK_LOGi0 ( L"Min Luminance: %f, Max Luminance: %f", minLum.m128_f32 [0] * 80.0f,
                                                                      maxLum.m128_f32 [0] * 80.0f );

                  SK_LOGi0 ( L"Mean Luminance (arithmetic, geometric): %f, %f", 80.0f * ( minLum.m128_f32 [0] + maxLum.m128_f32 [0] ) * 0.5f,
                                                                                80.0f * expf ( (1.0f / N) * lumTotal ) );

                  pFrameData->hdr.max_cll_nits = maxCLL.m128_f32 [0] * 80.0f;
                  pFrameData->hdr.avg_cll_nits = 80.0f * ( minLum.m128_f32 [0] + maxLum.m128_f32 [0] ) * 0.5f + 
                                                 80.0f * expf ( (1.0f / N) * lumTotal );

                  extern float _cLerpScale;
                  extern float _cSdrPower;

                  hr =               un_srgb.GetImageCount () == 1 ?
                    TransformImage ( un_srgb.GetImages     (),
                                     un_srgb.GetImageCount (),
                                     un_srgb.GetMetadata   (),
                    [&](XMVECTOR* outPixels, const XMVECTOR* inPixels, size_t width, size_t y)
                    {
                      UNREFERENCED_PARAMETER(y);

                      const XMVECTORF32 c_SdrPower =
                        { _cSdrPower, _cSdrPower, _cSdrPower, 1.f };

                      XMVECTOR maxLumExp =
                        XMVectorMultiply ( maxCLL,
                                           maxCLL );

                      for (size_t j = 0; j < width; ++j)
                      {
                        XMVECTOR value = XMVectorMax (inPixels [j], g_XMZero);
                        XMVECTOR luma  = XMVector3Dot ( value, s_luminance );

                        XMVECTOR numerator = 
                          XMVectorAdd (
                            g_XMOne,
                              XMVectorDivide (
                                luma, maxLumExp
                              )
                          );

                        XMVECTOR scale0 =
                          XMVectorDivide (
                            numerator, XMVectorAdd (
                              g_XMOne, luma
                            )
                          );

                        XMVECTOR scale1 =
                          XMVectorDivide (
                            numerator, XMVectorAdd (
                              g_XMOne, value
                            )
                          );

                        value =
                          XMVectorMultiply (value, XMVectorLerp (scale1, scale0, luma.m128_f32 [0] /
                                                                               maxLum.m128_f32 [0] / _cLerpScale));
                        
                        outPixels [j] =
                          XMVectorPow ( value, c_SdrPower );
                      }
                    }, final_sdr
                  ) : E_POINTER;

                  extern UINT filterFlags; // Pending removal, this is to debug WIC

                  if (         final_sdr.GetImageCount () == 1) {
                    Convert ( *final_sdr.GetImages     (),
                                DXGI_FORMAT_B8G8R8X8_UNORM,
                                  filterFlags,
                                    TEX_THRESHOLD_DEFAULT,
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

                if (un_scrgb.GetImageCount () == 1 && pop_off->wantClipboardCopy () && (config.screenshots.copy_to_clipboard || (! pFrameData->AllowSaveToDisk)))
                {
                  rb.screenshot_mgr->copyToClipboard (*un_scrgb.GetImages ());
                }

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
                  if ( config.steam.screenshots.enable_hook &&
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
                              (int)pFrameData->Width, (int)pFrameData->Height,
                                true );

                      SK_LOG1 ( ( L"Finished Steam Screenshot Import for Handle: '%x' (%llu frame latency)",
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

        //if (config.screenshots.png_compress || config.screenshots.copy_to_clipboard)
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
              concurrency::concurrent_queue <SK_Screenshot::framebuffer_s *>*
                images_to_write =
                  (concurrency::concurrent_queue <SK_Screenshot::framebuffer_s *>*)pUser;

              SetThreadPriority           ( SK_GetCurrentThread (), THREAD_MODE_BACKGROUND_BEGIN );
              SetThreadPriority           ( SK_GetCurrentThread (), THREAD_PRIORITY_BELOW_NORMAL );

              while (! ReadAcquire (&__SK_DLL_Ending))
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
                     (rb.isHDRCapable () &&
                      rb.isHDRActive  ());

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

                      //case DXGI_FORMAT_R10G10B10A2_UNORM:
                      //{
                      //  for ( UINT j = 3                          ;
                      //             j < pFrameData->PackedDstPitch ;
                      //             j += 4 )
                      //  {    pDst [j]  |=  0x3;       }
                      //} break;

                        case DXGI_FORMAT_R16G16B16A16_FLOAT:
                        {
                          for ( UINT j  = 0                          ;
                                     j < pFrameData->PackedDstPitch  ;
                                     j += 8 )
                          {
                            glm::vec4 color =
                              glm::unpackHalf4x16 (*((uint64*)&(pDst [j])));

                            color.a = 1.0f;

                            *((uint64*)& (pDst[j])) =
                              glm::packHalf4x16 (color);
                          }
                        } break;
                      }

                      pDst += pFrameData->PackedDstPitch;
                    }

                    ComputePitch ( pFrameData->dxgi.NativeFormat,
             static_cast <size_t> (pFrameData->Width),
             static_cast <size_t> (pFrameData->Height),
                                       raw_img.rowPitch,
                                       raw_img.slicePitch
                    );

                    raw_img.format = pFrameData->dxgi.NativeFormat;
                    raw_img.width  = static_cast <size_t> (pFrameData->Width);
                    raw_img.height = static_cast <size_t> (pFrameData->Height);
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

                    if (pFrameData->AllowSaveToDisk && config.screenshots.png_compress)
                    {
                      SK_CreateDirectories (wszAbsolutePathToLossless);

                      ScratchImage
                        un_srgb;
                        un_srgb.InitializeFromImage (raw_img);

                      if (hdr && config.screenshots.use_avif)
                      {
                        SK_Screenshot_SaveAVIF (un_srgb, wszAbsolutePathToLossless, static_cast <uint16_t> (pFrameData->hdr.max_cll_nits),
                                                                                    static_cast <uint16_t> (pFrameData->hdr.avg_cll_nits));
                      }

                      if (hdr && raw_img.format != DXGI_FORMAT_R16G16B16A16_FLOAT)
                      {
                        auto hdr10_metadata        = un_srgb.GetMetadata ();
                             hdr10_metadata.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
                             hdr10_metadata.SetAlphaMode (TEX_ALPHA_MODE_OPAQUE);

                        ScratchImage
                          un_hdr10;
                          un_hdr10.Initialize (hdr10_metadata);

                        DirectX::Convert (raw_img, DXGI_FORMAT_R16G16B16A16_FLOAT, 0x0, 0.0f, un_srgb);

                        static const XMMATRIX c_from2020to709 =
                        {
                          { 1.6604910f,  -0.1245505f, -0.0181508f, 0.f },
                          { -0.5876411f,  1.1328999f, -0.1005789f, 0.f },
                          { -0.0728499f, -0.0083494f,  1.1187297f, 0.f },
                          { 0.f,          0.f,         0.f,        1.f }
                        };

                        static const XMMATRIX c_from709to2020 =
                        {
                          { 0.627225305694944,  0.329476882715808,  0.0432978115892484, 0.0 },
                          { 0.0690418812810714, 0.919605681354755,  0.0113524373641739, 0.0 },
                          { 0.0163911702607078, 0.0880887513437058, 0.895520078395586,  0.0 },
                          { 0.0,                0.0,                0.0,                1.0 }
                        };

                        struct ParamsPQ
                        {
                          XMVECTOR N, M;
                          XMVECTOR C1, C2, C3;
                          XMVECTOR MaxPQ;
                        };

                        static const ParamsPQ PQ =
                        {
                          XMVectorReplicate (2610.0 / 4096.0 / 4.0),   // N
                          XMVectorReplicate (2523.0 / 4096.0 * 128.0), // M
                          XMVectorReplicate (3424.0 / 4096.0),         // C1
                          XMVectorReplicate (2413.0 / 4096.0 * 32.0),  // C2
                          XMVectorReplicate (2392.0 / 4096.0 * 32.0),  // C3
                          XMVectorReplicate (125.0),
                        };

                        auto PQToLinear = [](XMVECTOR N)
                        {
                          XMVECTOR ret;

                          ret =
                            XMVectorPow (N, XMVectorDivide (g_XMOne, PQ.M));

                          XMVECTOR nd;

                          nd =
                            XMVectorDivide (
                              XMVectorMax (XMVectorSubtract (ret, PQ.C1), g_XMZero),
                                           XMVectorSubtract (     PQ.C2,
                                    XMVectorMultiply (PQ.C3, ret)));

                          ret =
                            XMVectorMultiply (
                              XMVectorPow (nd, XMVectorDivide (g_XMOne, PQ.N)), PQ.MaxPQ
                            );

                          return ret;
                        };

                        auto metadata =
                          un_srgb.GetMetadata ();

                        metadata.SetAlphaMode (TEX_ALPHA_MODE_OPAQUE);

                        if (
                          SUCCEEDED (
                            TransformImage ( un_srgb.GetImages     (),
                                             un_srgb.GetImageCount (),
                                             metadata,
                            [&](XMVECTOR* outPixels, const XMVECTOR* inPixels, size_t width, size_t y)
                            {
                              UNREFERENCED_PARAMETER(y);

                              for (size_t j = 0; j < width; ++j)
                              {
                                outPixels [j] =
                                  XMVector3Transform (
                                    PQToLinear (
                                      XMVectorClamp (
                                        inPixels [j],
                                        g_XMZero,
                                        g_XMOne )
                                    ),  c_from2020to709
                                  );
                                outPixels [j].m128_f32 [3] = 1.0f;
                              }
                            }, un_hdr10    )
                           )        )
                        {
                          std::swap (un_srgb, un_hdr10);
                        }

                        else
                          hdr = false; // Couldn't undo HDR10, don't store a .jxr
                      }

                      HRESULT hrSaveToWIC = S_OK;
                      
                      if ((! hdr) || (! config.screenshots.use_avif))
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
                        SK_LOGi0 ( L"Unable to write Screenshot, hr=%s",
                                                       SK_DescribeHRESULT (hrSaveToWIC) );

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
            }, L"[SK] D3D12 Screenshot Encoder",
              (LPVOID)&raw_images_ );
          } }

          if (enqueued_lossless > 0)
          {   enqueued_lossless = 0;
            SetEvent (signal.hq_encode);
          }
        }
      } while (ReadAcquire (&__SK_DLL_Ending) == FALSE);

      SK_Thread_CloseSelf ();

      SK_CloseHandle (signal.capture);
      SK_CloseHandle (signal.abort.initiate);
      SK_CloseHandle (signal.abort.finished);
      SK_CloseHandle (signal.hq_encode);

      return 0;
    }, L"[SK] D3D12 Screenshot Capture" );

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
  static concurrency::concurrent_queue <SK_D3D12_Screenshot *> rejected_screenshots;

  bool new_jobs = false;

  do
  {
    do
    {
      SK_D3D12_Screenshot*            pop_off   = nullptr;
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
      SK_D3D12_Screenshot*               push_back   = nullptr;
      if ( rejected_screenshots.try_pop (push_back) &&
                                         push_back  != nullptr )
      {
        if (purge)
          delete push_back;

        else
          screenshot_queue->push (push_back);
      }
    } while ((!rejected_screenshots.empty ()) && (purge || wait));

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
SK_D3D12_ProcessScreenshotQueue (SK_ScreenshotStage stage)
{
  SK_D3D12_ProcessScreenshotQueueEx (stage, false);
}


void
SK_D3D12_WaitOnAllScreenshots (void)
{
}



void SK_Screenshot_D3D12_EndFrame (void)
{
  extern bool   SK_D3D12_ShouldSkipHUD (void);
  std::ignore = SK_D3D12_ShouldSkipHUD (    );

  if (InterlockedCompareExchange (&__SK_D3D12_InitiateHudFreeShot, 0, -1) == -1)
  {
    SK_D3D12_ShowGameHUD ();
  }
}

bool SK_Screenshot_D3D12_BeginFrame (void)
{
  InterlockedExchange (&__SK_ScreenShot_CapturingHUDless, 0);

  if ( ReadAcquire (&__SK_D3D12_QueuedShots)          > 0 ||
       ReadAcquire (&__SK_D3D12_InitiateHudFreeShot) != 0    )
  {
    InterlockedExchange            (&__SK_ScreenShot_CapturingHUDless, 1);
    if (InterlockedCompareExchange (&__SK_D3D12_InitiateHudFreeShot, -3, 1) == 1)
    {
      SK_D3D12_HideGameHUD ();
      SK::SteamAPI::TakeScreenshot (SK_ScreenshotStage::EndOfFrame);
    }

    // 2-frame Delay for SDR->HDR Upconversion
    else if (InterlockedCompareExchange (&__SK_D3D12_InitiateHudFreeShot, -2, -3) == -3)
    {
      //SK::SteamAPI::TakeScreenshot (SK_ScreenshotStage::EndOfFrame);
    //InterlockedDecrement (&SK_D3D12_DrawTrackingReqs); (Handled by ShowGameHUD)
    }

    else if (InterlockedCompareExchange (&__SK_D3D12_InitiateHudFreeShot, -1, -2) == -2)
    {
      //SK::SteamAPI::TakeScreenshot (SK_ScreenshotStage::EndOfFrame);
    //InterlockedDecrement (&SK_D3D12_DrawTrackingReqs); (Handled by ShowGameHUD)
    }

    else if (ReadAcquire (&__SK_D3D12_InitiateHudFreeShot) == FALSE)
    {
      InterlockedDecrement (&__SK_D3D12_QueuedShots);
      InterlockedExchange  (&__SK_D3D12_InitiateHudFreeShot, 1);

      return true;
    }
  }

  return false;
}


// For effects that blink; updated once per-frame.
extern DWORD& dwFrameTime;

DWORD D3D12_GetFrameTime (void)
{
  return dwFrameTime;
}

void
SK_D3D12_BeginFrame (void)
{
  if (SK_Screenshot_D3D12_BeginFrame ())
  {
    ///// This looks silly, but it lets HUDless screenshots
    /////   set shader state before the frame begins... to
    /////     remove HUD shaders.
    return
      SK_D3D12_BeginFrame (); // This recursion will end.
  }
}

void
SK_D3D12_EndFrame (SK_TLS* /* pTLS = SK_TLS_Bottom ()*/)
{
  for ( auto end_frame_fn : plugin_mgr->end_frame_fns )
  {
    end_frame_fn ();
  }

  dwFrameTime = SK::ControlPanel::current_time;

  SK_Screenshot_D3D12_RestoreHUD ();
  SK_Screenshot_D3D12_EndFrame   ();
}