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


extern void SK_Screenshot_PlaySound            (void);
extern void SK_Steam_CatastropicScreenshotFail (void);

extern volatile LONG  SK_D3D11_DrawTrackingReqs;

SK_D3D11_Screenshot& SK_D3D11_Screenshot::operator= (SK_D3D11_Screenshot&& moveFrom)
{
  if (this != &moveFrom)
  {
    dispose ();

    pDev.p                          = moveFrom.pDev.p;
    pImmediateCtx.p                 = moveFrom.pImmediateCtx.p;

    pSwapChain.p                    = moveFrom.pSwapChain.p;
    pBackbufferSurface.p            = moveFrom.pBackbufferSurface.p;
    pStagingBackbufferCopy.p        = moveFrom.pStagingBackbufferCopy.p;

    pPixelBufferFence.p             = moveFrom.pPixelBufferFence.p;
    ulCommandIssuedOnFrame          = moveFrom.ulCommandIssuedOnFrame;

    framebuffer.Width               = moveFrom.framebuffer.Width;
    framebuffer.Height              = moveFrom.framebuffer.Height;
    framebuffer.NativeFormat        = moveFrom.framebuffer.NativeFormat;
    framebuffer.PackedDstPitch      = moveFrom.framebuffer.PackedDstPitch;
    framebuffer.PackedDstSlicePitch = moveFrom.framebuffer.PackedDstSlicePitch;
    framebuffer.AlphaMode           = moveFrom.framebuffer.AlphaMode;
    framebuffer.PBufferSize         = moveFrom.framebuffer.PBufferSize;

    framebuffer.PixelBuffer.reset (
      moveFrom.framebuffer.PixelBuffer.get () // XXX: Shouldn't this be release?
    );

    //framebuffer.PixelBuffer.reset   ( moveFrom.framebuffer.PixelBuffer.release () );

    moveFrom.pDev.p                          = nullptr;
    moveFrom.pImmediateCtx.p                 = nullptr;
    moveFrom.pSwapChain.p                    = nullptr;
    moveFrom.pStagingBackbufferCopy.p        = nullptr;

    moveFrom.pPixelBufferFence.p             = nullptr;
    moveFrom.ulCommandIssuedOnFrame          = 0;

    moveFrom.framebuffer.Width               = 0;
    moveFrom.framebuffer.Height              = 0;
    moveFrom.framebuffer.NativeFormat        = DXGI_FORMAT_UNKNOWN;
    moveFrom.framebuffer.PackedDstPitch      = 0;
    moveFrom.framebuffer.PackedDstSlicePitch = 0;
    moveFrom.framebuffer.AlphaMode           = DXGI_ALPHA_MODE_UNSPECIFIED;
    moveFrom.framebuffer.PBufferSize         = 0;
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

    SK_ComQIPtr <ID3D11Device> pDev (SK_GetCurrentRenderBackend ().device);

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
      SK_ReleaseAssert ("WTF?!" == nullptr)
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
        compileShaderString ( data.get (), wstr_file.c_str (),
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

SK_D3D11_Screenshot::SK_D3D11_Screenshot (const SK_ComPtr <ID3D11Device>& pDevice) : pDev (pDevice)
{
  SK_ScopedBool decl_tex_scope (
    SK_D3D11_DeclareTexInjectScope ()
  );

  auto& io =
    ImGui::GetIO ();

  if (pDev != nullptr)
  {
    static SK_RenderBackend& rb =
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

          framebuffer.AlphaMode =
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

          framebuffer.Width        = backbuffer_desc.Width;
          framebuffer.Height       = backbuffer_desc.Height;
          framebuffer.NativeFormat = backbuffer_desc.Format;

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
                static SK_D3D11_Stateblock_Lite* pSB = nullptr;

                SK_D3D11_CaptureStateBlock (pImmediateCtx, &pSB);

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

                  if (pDev->GetFeatureLevel () >= D3D_FEATURE_LEVEL_11_0)
                  {
                    pImmediateCtx->HSSetShader        (nullptr,                         nullptr, 0);
                    pImmediateCtx->DSSetShader        (nullptr,                         nullptr, 0);
                  }

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
                    raster_desc.ScissorEnable   = FALSE;
                    raster_desc.DepthClipEnable = TRUE;

                    depth_desc.DepthEnable      = FALSE;
                    depth_desc.DepthWriteMask   = D3D11_DEPTH_WRITE_MASK_ALL;
                    depth_desc.DepthFunc        = D3D11_COMPARISON_ALWAYS;
                    depth_desc.StencilEnable    = FALSE;
                    depth_desc.FrontFace.StencilFailOp = depth_desc.FrontFace.StencilDepthFailOp =
                                                         depth_desc.FrontFace.StencilPassOp      =
                                                       D3D11_STENCIL_OP_KEEP;
                    depth_desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
                    depth_desc.BackFace              = depth_desc.FrontFace;

                    blend_desc.AlphaToCoverageEnable                  = FALSE;
                    blend_desc.RenderTarget [0].BlendEnable           = TRUE;
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

                  SK_D3D11_ApplyStateBlock (pSB, pImmediateCtx);
                }
              }
            }
          }

          D3D11_TEXTURE2D_DESC staging_desc =
          {
            framebuffer.Width, framebuffer.Height,
            1,                 1,

            framebuffer.NativeFormat,

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
                { framebuffer.Width,          framebuffer.Height,
                  1,                          1,
                  framebuffer.NativeFormat, { 1, 0 },
                  D3D11_USAGE_DEFAULT,        0x00,
                  0x0,                        0x00 };

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
                                                      framebuffer.NativeFormat );

                pImmediateCtx->CopyResource ( pStagingBackbufferCopy,
                                              pResolvedTex );
              }
            }

            if (pImmediateCtx3 != nullptr)
            {
              pImmediateCtx3->Flush1 (D3D11_CONTEXT_TYPE_COPY, nullptr);
            }

            pImmediateCtx->End (pPixelBufferFence);

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

SK_D3D11_Screenshot::framebuffer_s::PinnedBuffer
SK_D3D11_Screenshot::framebuffer_s::root_;

void
SK_D3D11_Screenshot::dispose (void)
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
                              SK_ScreenshotStage::EndOfFrame )
{
  static const SK_RenderBackend_V2& rb =
    SK_GetCurrentRenderBackend ();

  if ( ((int)rb.api & (int)SK_RenderAPI::D3D11)
                   == (int)SK_RenderAPI::D3D11 )
  {
    static const
      std::map <SK_ScreenshotStage, int>
        __stage_map = {
          { SK_ScreenshotStage::BeforeGameHUD, 0 },
          { SK_ScreenshotStage::BeforeOSD,     1 },
          { SK_ScreenshotStage::EndOfFrame,    2 }
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

      return true;
    }
  }

  return false;
}


float fMaxNitsForDisplay = 6.0f;
UINT filterFlags =
  0x120000FF;
  //DirectX::TEX_FILTER_DEFAULT |
  //DirectX::TEX_FILTER_DITHER_DIFFUSION |
  //DirectX::TEX_FILTER_SRGB_OUT;

void
SK_D3D11_ProcessScreenshotQueueEx ( SK_ScreenshotStage stage_ = SK_ScreenshotStage::EndOfFrame,
                                    bool               wait   = false,
                                    bool               purge  = false )
{
  static auto& rb =
    SK_GetCurrentRenderBackend ();

  constexpr int __MaxStage = 2;
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
              if (SK_GetCurrentRenderBackend ().screenshot_mgr.checkDiskSpace (20ULL * 1024ULL * 1024ULL))
              {
                screenshot_queue->push (
                  new SK_D3D11_Screenshot (
                    pDev
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

            if (config.screenshots.png_compress)
              to_write.emplace_back (pop_off);

            SK_D3D11_Screenshot::framebuffer_s* pFrameData =
              pop_off->getFinishedData ();

            // Why's it on the wait-queue if it's not finished?!
            assert (pFrameData != nullptr);

            if (pFrameData != nullptr)
            {
              using namespace DirectX;

              bool  skip_me = false;
              Image raw_img = {   };

              ComputePitch (
                pFrameData->NativeFormat,
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
                              rb.screenshot_mgr.getBasePath (),
                                _TRUNCATE );

              if ( config.steam.screenshots.enable_hook &&
                          steam_ctx.Screenshots ()      != nullptr )
              {
                PathAppendW          (wszAbsolutePathToScreenshot, L"SK_SteamScreenshotImport.jpg");
                SK_CreateDirectories (wszAbsolutePathToScreenshot);
              }

              else if ( hdr )
              {
                time_t screenshot_time = 0;
                       codec           = WIC_CODEC_PNG;

                PathAppendW (         wszAbsolutePathToScreenshot,
                  SK_FormatStringW ( LR"(LDR\%lu.png)",
                              time (&screenshot_time) ).c_str () );
                SK_CreateDirectories (wszAbsolutePathToScreenshot);
              }

              // Not HDR and not importing to Steam,
              //   we've got nothing left to do...
              else
              {
                skip_me = true;
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

                switch (pFrameData->AlphaMode)
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

                auto RemoveGamma_sRGB = [](XMVECTOR value) ->
                void
                {
                  value.m128_f32 [0] = ( value.m128_f32 [0] < 0.04045f ) ?
                                         value.m128_f32 [0] / 12.92f     :
                                   pow ((value.m128_f32 [0] + 0.055f) / 1.055f, 2.4f);
                  value.m128_f32 [1] = ( value.m128_f32 [1] < 0.04045f ) ?
                                         value.m128_f32 [1] / 12.92f     :
                                   pow ((value.m128_f32 [1] + 0.055f) / 1.055f, 2.4f);
                  value.m128_f32 [2] = ( value.m128_f32 [2] < 0.04045f ) ?
                                         value.m128_f32 [2] / 12.92f     :
                                   pow ((value.m128_f32 [2] + 0.055f) / 1.055f, 2.4f);
                };

                auto ApplyGamma_sRGB = [](XMVECTOR value) ->
                void
                {
                  value.m128_f32 [0] = ( value.m128_f32 [0] < 0.0031308f ) ?
                                         value.m128_f32 [0] * 12.92f      :
                           1.055f * pow (value.m128_f32 [0], 1.0f / 2.4f) - 0.055f;
                  value.m128_f32 [1] = ( value.m128_f32 [1] < 0.0031308f ) ?
                                         value.m128_f32 [1] * 12.92f      :
                           1.055f * pow (value.m128_f32 [1], 1.0f / 2.4f) - 0.055f;
                  value.m128_f32 [2] = ( value.m128_f32 [2] < 0.0031308f ) ?
                                         value.m128_f32 [2] * 12.92f      :
                           1.055f * pow (value.m128_f32 [2], 1.0f / 2.4f) - 0.055f;
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
                      XMVECTOR value  = inPixels [j];
                      XMVECTOR nvalue = XMVector3Transform (value, c_from2020to709);
                                value = XMVectorSelect     (value, nvalue, g_XMSelect1110);

                      ApplyGamma_sRGB (value);

                      outPixels [j]   =                     value;
                    }
                  }, un_scrgb);

                  std::swap (un_scrgb, un_srgb);
                }

#if 0
                XMVECTOR maxLum = XMVectorZero (),
                         minLum = g_XMInfinity;

                FLOAT    maxY   = 0.0F;

                static constexpr
                  XMVECTORF32 mat_bt709_Lum =
                    { 0.2125862307855955516f,
                      0.7151703037034108499f,
                      0.07220049864333622685f };

                
                const auto bin_count = 256;

                const auto num_pixels = raw_img.width *
                                        raw_img.height;
                float      alpha      = std::floor (bin_count) / num_pixels;

                std::vector <int>   bins  (bin_count);
                std::vector <int>   bins2 (bin_count);

                std::vector <float> lut  (bin_count);

                auto lumaToBin = [](float fLuma)
                {
                  return
                    std::max ( 0.0,
                      std::min ( std::floor (bin_count), (fLuma / 10.0) * bin_count) );
                };

#if 1
                hr =              un_srgb.GetImageCount () == 1 ?
                  EvaluateImage ( un_srgb.GetImages     (),
                                  un_srgb.GetImageCount (),
                                  un_srgb.GetMetadata   (),
                  [&](const XMVECTOR* pixels, size_t width, size_t y)
                  {
                    std::ranges::for_each (
                      std::valarray <XMVECTOR> (pixels, width),
                                 [&](XMVECTOR&  pixel)
                      {
#if 1
                        XMVECTOR vLuma =
                            XMVector3Dot ( mat_bt709_Lum, pixel );
#else
                        XMVECTOR vLuma =
                          XMVectorSplatY (XMColorSRGBToXYZ (pixel));
#endif

                        ////vLuma.m128_f32 [0] /= 80.0f;
                        ////vLuma.m128_f32 [1] /= 80.0f;
                        ////vLuma.m128_f32 [2] /= 80.0f;
                        ////vLuma.m128_f32 [3] /= 80.0f;

                        maxLum =
                          XMVectorMax ( maxLum,
                                          vLuma );

                        minLum =
                          XMVectorMin ( minLum,
                                          vLuma );

                        bins [ lumaToBin (vLuma.m128_f32 [0]) ] ++;
                      }
                    );

                    UNREFERENCED_PARAMETER(y);
                  })                                       : E_POINTER;
#else
                                hr =              un_srgb.GetImageCount () == 1 ?
                  EvaluateImage ( un_srgb.GetImages     (),
                                  un_srgb.GetImageCount (),
                                  un_srgb.GetMetadata   (),
                  [&](const XMVECTOR* pixels, size_t width, size_t y)
                  {
                    UNREFERENCED_PARAMETER(y);

                    for (size_t j = 0; j < width; ++j)
                    {
                      static const XMVECTORF32 s_luminance =
                      //{ 0.3f, 0.59f, 0.11f, 0.f };
                      { 0.2125862307855955516f,
                        0.7151703037034108499f,
                        0.07220049864333622685f };

                      XMVECTOR v = *pixels++;
                      RemoveGamma_sRGB          (v);
                               v = XMVector3Dot (v, s_luminance);

                      maxLum =
                        XMVectorMax (v, maxLum);
                    }
                  })                                       : E_POINTER;
#endif

                  SK_LOG0 ( (L"Min Luminance: %f, Max Luminance: %f", minLum.m128_f32 [0],
                                                                      maxLum.m128_f32 [0]), L"XXX" );

                  SK_LOG0 ( (L"Mean Luminance: %f", ( maxLum.m128_f32 [0] +
                                                      minLum.m128_f32 [0] ) / 2.0f), L"XXX" );

                  //double dAccum = 0.0;
                  //
                  //for (auto i = 0 ; i < bin_count ; ++i)
                  //{
                  //  dAccum += bins [i] * sfactor * i;
                  //}

                  // Calculate the probability of each intensity
                  double PrRk [bin_count];
                  for (int i = 0; i < bin_count; i++)
                  {
                    PrRk [i] = (double)bins [i] / num_pixels;
                  }

                  auto cumhist = [](std::vector <int>& histogram,
                                    std::vector <int>& cumhistogram)
                  {
                    cumhistogram [0] =
                       histogram [0];

                    for (int i = 1; i < histogram.size (); i++)
                    {
                      cumhistogram [i] =
                         histogram [i] + cumhistogram [i-1];
                    }
                  };

                  // Generate cumulative frequency histogram
                  cumhist (bins, bins2);

                  // Scale the histogram
                  std::vector <int> scaled (bin_count);

                  for (int i = 0; i < bin_count; i++)
                  {
                    scaled [i] =
                      //cvRound((double)cumhistogram[i] * alpha);
                      std::round ((double)bins2 [i] * alpha);
                  }

                  // Generate the equlized histogram
                  std::vector <double> scaled2 (bin_count);
                  for (int i = 0; i < bin_count; i++)
                  {
                    scaled2 [i] = 0.0;
                  }
                  for (int i = 0; i < 256; i++)
                  {
                    scaled2 [scaled [i]] += PrRk [i];
                  }

    //int final[256];
    //for(int i = 0; i < 256; i++)
    //    final[i] = cvRound(PsSk[i]*255);

                  //SK_LOG0 ( (L"Histogram Mean: %f", ( dAccum / (double)un_srgb.GetPixelsSize () ) ), L"XXX" );

                  //if (config.render.hdr.enable_32bpc)
                  //{
                    //maxLum =
                    //  XMVector3Length  (maxLum);
                  //} else {
                  //  maxLum =
                  //    XMVectorMultiply (maxLum, maxLum);
                  //}

                  static const XMVECTORF32 c_MaxNitsForDisplay =
                    { fMaxNitsForDisplay, fMaxNitsForDisplay, fMaxNitsForDisplay, 1.f };

                  XMVECTOR scale =
                    XMVectorDivide ( c_MaxNitsForDisplay, maxLum );

                  hr =               un_srgb.GetImageCount () == 1 ?
                    TransformImage ( un_srgb.GetImages     (),
                                     un_srgb.GetImageCount (),
                                     un_srgb.GetMetadata   (),
                    [&](XMVECTOR* outPixels, const XMVECTOR* inPixels, size_t width, size_t y)
                    {
                      UNREFERENCED_PARAMETER(y);

                      //bins [ lumaToBin (vLuma.m128_f32 [0]) ] ++;

                      for (size_t j = 0; j < width; ++j)
                      {
                        XMVECTOR value  = inPixels [j];
                        XMVECTOR nvalue =
                          XMVectorMultiply (value, scale);
                      //          value =
                      //  XMVectorSelect   (value, nvalue, g_XMSelect1110);
                        outPixels [j]   =   value;
                      }
                    }, un_scrgb)                             : E_POINTER;

                std::swap (un_srgb, un_scrgb);
#else
                XMVECTOR maxLum = XMVectorZero          (),
                         minLum = XMVectorSplatInfinity ();

                hr =              un_srgb.GetImageCount () == 1 ?
                  EvaluateImage ( un_srgb.GetImages     (),
                                  un_srgb.GetImageCount (),
                                  un_srgb.GetMetadata   (),
                  [&](const XMVECTOR* pixels, size_t width, size_t y)
                  {
                      UNREFERENCED_PARAMETER(y);

                      for (size_t j = 0; j < width; ++j)
                      {
                        static const XMVECTORF32 s_luminance =
                        { 0.3f, 0.59f, 0.11f, 0.f };
                      
                        XMVECTOR vLuma =
                          XMVectorSplatY (
                            XMColorRGBToXYZ (*pixels++)
                          );
                        
                        maxLum =
                          XMVectorMax ( maxLum,
                                          vLuma );

                        minLum =
                          XMVectorMin ( minLum,
                                          vLuma );
                      }
                    })                                       : E_POINTER;

                  SK_LOG0 ( (L"Min Luminance: %f, Max Luminance: %f", minLum.m128_f32 [0],
                                                                      maxLum.m128_f32 [0]), L"XXX" );

                  SK_LOG0 ( (L"Mean Luminance: %f", ( maxLum.m128_f32 [0] +
                                                      minLum.m128_f32 [0] ) / 2.0f), L"XXX" );

                //#define _CHROMA_SCALE TRUE
                  #ifndef _CHROMA_SCALE
                                    maxLum =
                                      XMVector3Length  (maxLum);
                  #else
                                    maxLum =
                                      XMVectorMultiply (maxLum, maxLum);
                  #endif

                  SK_LOG0 ( (L"Min Luminance: %f, Max Luminance: %f", minLum.m128_f32 [0],
                                                                      maxLum.m128_f32 [0]), L"XXX" );

                  SK_LOG0 ( (L"Mean Luminance: %f", ( maxLum.m128_f32 [0] +
                                                      minLum.m128_f32 [0] ) / 2.0f), L"XXX" );

                  const XMVECTORF32 c_MaxNitsForDisplay =
                  { fMaxNitsForDisplay, fMaxNitsForDisplay, fMaxNitsForDisplay, 1.f };

                  hr =               un_srgb.GetImageCount () == 1 ?
                    TransformImage ( un_srgb.GetImages     (),
                                     un_srgb.GetImageCount (),
                                     un_srgb.GetMetadata   (),
                    [&](XMVECTOR* outPixels, const XMVECTOR* inPixels, size_t width, size_t y)
                    {
                      UNREFERENCED_PARAMETER(y);

                      for (size_t j = 0; j < width; ++j)
                      {
                        XMVECTOR value = inPixels [j];

                        XMVECTOR scale =
                          XMVectorDivide (
                            XMVectorAdd (
                              g_XMOne, XMVectorDivide ( value,
                                                          maxLum
                                                      )
                                        ),
                            XMVectorAdd (
                              g_XMOne, c_MaxNitsForDisplay
                                        )
                          );

                        //scale =
                        //  XMVectorDivide ( c_MaxNitsForDisplay, maxLum );

                        XMVECTOR nvalue =
                          XMVectorMultiply (value, scale);
                                  value =
                          XMVectorSelect   (value, nvalue, g_XMSelect1110);
                        outPixels [j]   =   value;
                      }
                    }, un_scrgb)                             : E_POINTER;

                std::swap (un_srgb, un_scrgb);

                //if (         un_srgb.GetImageCount () == 1) {
                //  Convert ( *un_srgb.GetImages     (),
                //              DXGI_FORMAT_B8G8R8X8_UNORM,
                //                TEX_FILTER_DITHER |
                //                TEX_FILTER_SRGB,
                //                  TEX_THRESHOLD_DEFAULT,
                //                    un_scrgb );
                //}
#endif

                if (         un_srgb.GetImageCount () == 1) {
                  Convert ( *un_srgb.GetImages     (),
                              DXGI_FORMAT_B8G8R8X8_UNORM,
                                filterFlags,
                                  TEX_THRESHOLD_DEFAULT,
                                    un_scrgb );
                }

                if (un_scrgb.GetImageCount () == 1)
                {
                  rb.screenshot_mgr.copyToClipboard (*un_scrgb.GetImages ());
                }

                if (               un_scrgb.GetImageCount () == 1 &&
                      SUCCEEDED (
                  SaveToWICFile ( *un_scrgb.GetImages (), WIC_FLAGS_NONE,
                                     GetWICCodec         (codec),
                                      wszAbsolutePathToScreenshot )
                               )
                   )
                {
                  if ( config.steam.screenshots.enable_hook &&
                          steam_ctx.Screenshots () != nullptr )
                  {
                    wchar_t       wszAbsolutePathToThumbnail [ MAX_PATH + 2 ] = { };
                    wcsncpy_s   ( wszAbsolutePathToThumbnail,  MAX_PATH,
                                    rb.screenshot_mgr.getBasePath (),
                                      _TRUNCATE );

                    PathAppendW (wszAbsolutePathToThumbnail, L"SK_SteamThumbnailImport.jpg");

                    float aspect = (float)pFrameData->Height /
                                   (float)pFrameData->Width;

                    ScratchImage thumbnailImage;

                    Resize ( *un_scrgb.GetImages (), 200,
                               sk::narrow_cast <size_t> (200.0 * aspect),
                                TEX_FILTER_DITHER_DIFFUSION | TEX_FILTER_FORCE_WIC
                              | TEX_FILTER_TRIANGLE,
                                  thumbnailImage );

                    if (               thumbnailImage.GetImages ()) {
                      SaveToWICFile ( *thumbnailImage.GetImages (), WIC_FLAGS_DITHER,
                                        GetWICCodec                (codec),
                                          wszAbsolutePathToThumbnail );

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
                              pFrameData->Width, pFrameData->Height,
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

              if (! config.screenshots.png_compress)
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

        if (config.screenshots.png_compress || config.screenshots.copy_to_clipboard)
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

            static concurrency::concurrent_queue <SK_D3D11_Screenshot::framebuffer_s*>
              raw_images_;

            SK_D3D11_Screenshot::framebuffer_s* fb_orig =
              it->getFinishedData ();

            SK_ReleaseAssert (fb_orig != nullptr);

            if (fb_orig != nullptr)
            {
              SK_D3D11_Screenshot::framebuffer_s* fb_copy =
                new SK_D3D11_Screenshot::framebuffer_s ();

              fb_copy->Height              = fb_orig->Height; //-V522
              fb_copy->Width               = fb_orig->Width;
              fb_copy->NativeFormat        = fb_orig->NativeFormat;
              fb_copy->AlphaMode           = fb_orig->AlphaMode;
              fb_copy->PBufferSize         = fb_orig->PBufferSize;
              fb_copy->PackedDstPitch      = fb_orig->PackedDstPitch;
              fb_copy->PackedDstSlicePitch = fb_orig->PackedDstSlicePitch;

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
              concurrency::concurrent_queue <SK_D3D11_Screenshot::framebuffer_s *>*
                images_to_write =
                  (concurrency::concurrent_queue <SK_D3D11_Screenshot::framebuffer_s *>*)pUser;

              SetThreadPriority           ( SK_GetCurrentThread (), THREAD_MODE_BACKGROUND_BEGIN );
              SetThreadPriority           ( SK_GetCurrentThread (), THREAD_PRIORITY_BELOW_NORMAL );

              while (0 == ReadAcquire (&__SK_DLL_Ending))
              {
                SK_D3D11_Screenshot::framebuffer_s *pFrameData =
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

                  time_t screenshot_time = 0;

                  wchar_t       wszAbsolutePathToLossless [ MAX_PATH + 2 ] = { };
                  wcsncpy_s   ( wszAbsolutePathToLossless,  MAX_PATH,
                                  rb.screenshot_mgr.getBasePath (),
                                    _TRUNCATE );

                  bool hdr =
                    ( SK_GetCurrentRenderBackend ().isHDRCapable () &&
                      SK_GetCurrentRenderBackend ().isHDRActive  () );

                  if (hdr)
                  {
                    PathAppendW ( wszAbsolutePathToLossless,
                      SK_FormatStringW ( L"HDR\\%lu.jxr",
                                  time (&screenshot_time) ).c_str () );
                  }

                  else
                  {
                    PathAppendW ( wszAbsolutePathToLossless,
                      SK_FormatStringW ( L"Lossless\\%lu.png",
                                  time (&screenshot_time) ).c_str () );
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
                      switch (pFrameData->NativeFormat)
                      {
                        case DXGI_FORMAT_B8G8R8A8_UNORM:
                        case DXGI_FORMAT_R8G8B8A8_UNORM:
                        {
                          for ( UINT j = 3                          ;
                                     j < pFrameData->PackedDstPitch ;
                                     j += 4 )
                          {    pDst [j] = 255UL;        }
                        } break;

                        case DXGI_FORMAT_R10G10B10A2_UNORM:
                        {
                          for ( UINT j = 3                          ;
                                     j < pFrameData->PackedDstPitch ;
                                     j += 4 )
                          {    pDst [j]  |=  0x3;       }
                        } break;

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

                    ComputePitch ( pFrameData->NativeFormat,
                                     pFrameData->Width,
                                     pFrameData->Height,
                                       raw_img.rowPitch,
                                       raw_img.slicePitch
                    );

                    raw_img.format = pFrameData->NativeFormat;
                    raw_img.width  = pFrameData->Width;
                    raw_img.height = pFrameData->Height;
                    raw_img.pixels = pFrameData->PixelBuffer.get ();

                    if (config.screenshots.copy_to_clipboard)
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
                        if (! hdr)
                          rb.screenshot_mgr.copyToClipboard (*clipboard.GetImages ());
                      }
                    }

                    if (config.screenshots.png_compress)
                    {
                      SK_CreateDirectories (wszAbsolutePathToLossless);

                      ScratchImage
                        un_srgb;
                        un_srgb.InitializeFromImage (raw_img);

                      HRESULT hrSaveToWIC =     un_srgb.GetImages () ?
                                SaveToWICFile (*un_srgb.GetImages (), WIC_FLAGS_DITHER,
                                        GetWICCodec (hdr ? WIC_CODEC_WMP :
                                                           WIC_CODEC_PNG),
                                             wszAbsolutePathToLossless,
                                               hdr ?
                                                 &GUID_WICPixelFormat64bppRGBAHalf :
                                                 pFrameData->NativeFormat == DXGI_FORMAT_R10G10B10A2_UNORM ?
                                                                              &GUID_WICPixelFormat48bppRGB :
                                                                              &GUID_WICPixelFormat24bppBGR)
                                                                     : E_POINTER;

                      if (SUCCEEDED (hrSaveToWIC))
                      {
                        // Refresh
                        rb.screenshot_mgr.getRepoStats (true);
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
    }

    // 1-frame Delay for SDR->HDR Upconversion
    else if (InterlockedCompareExchange (&__SK_D3D11_InitiateHudFreeShot, -1, -2) == -2)
    {
      SK::SteamAPI::TakeScreenshot (SK_ScreenshotStage::EndOfFrame);
    //InterlockedDecrement (&SK_D3D11_DrawTrackingReqs); (Handled by ShowGameHUD)
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