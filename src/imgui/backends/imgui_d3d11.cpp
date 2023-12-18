// ImGui Win32 + DirectX11 binding
// In this binding, ImTextureID is used to store a 'ID3D11ShaderResourceView*' texture identifier. Read the FAQ about ImTextureID in imgui.cpp.

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you use this binding you'll need to call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(), ImGui::Render() and ImGui_ImplXXXX_Shutdown().
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

#include <SpecialK/stdafx.h>
#include <SpecialK/com_util.h>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_d3d11.h>
#include <SpecialK/render/dxgi/dxgi_backend.h>
#include <SpecialK/render/dxgi/dxgi_swapchain.h>
#include <SpecialK/render/dxgi/dxgi_util.h>
#include <SpecialK/render/dxgi/dxgi_hdr.h>
#include <SpecialK/render/d3d11/d3d11_core.h>

#include <shaders/imgui_d3d11_vs.h>
#include <shaders/imgui_d3d11_ps.h>

#include <shaders/steam_d3d11_vs.h>
#include <shaders/steam_d3d11_ps.h>

#include <shaders/epic_d3d11_vs.h>
#include <shaders/rtss_d3d11_vs.h>
#include <shaders/discord_d3d11_vs.h>
#include <shaders/discord_d3d11_ps.h>

#include <shaders/uplay_d3d11_vs.h>
#include <shaders/uplay_d3d11_ps.h>

// DirectX
#include <d3d11.h>

bool running_on_12 = false;

extern void
SK_ImGui_User_NewFrame (void);

// Data
static ImGuiMouseCursor         g_LastMouseCursor       = ImGuiMouseCursor_COUNT;
static bool                     g_HasGamepad            = false;
static bool                     g_WantUpdateHasGamepad  = true;

static HWND                     g_hWnd                  = nullptr;


static constexpr           UINT _MAX_BACKBUFFERS        = 1;

struct SK_ImGui_D3D11_BackbufferResourceIsolation {
  ID3D11VertexShader*       pVertexShader           = nullptr;
  ID3D11VertexShader*       pVertexShaderSteamHDR   = nullptr;
  ID3D11VertexShader*       pVertexShaderDiscordHDR = nullptr;
  ID3D11VertexShader*       pVertexShaderRTSSHDR    = nullptr;
  ID3D11VertexShader*       pVertexShaderuPlayHDR   = nullptr;
  ID3D11VertexShader*       pVertexShaderEpicHDR    = nullptr;
  ID3D11PixelShader*        pPixelShaderuPlayHDR    = nullptr;
  ID3D11PixelShader*        pPixelShaderSteamHDR    = nullptr;
  ID3D11PixelShader*        pPixelShaderDiscordHDR  = nullptr;
  ID3D11InputLayout*        pInputLayout            = nullptr;
  ID3D11Buffer*             pVertexConstantBuffer   = nullptr;
  ID3D11Buffer*             pPixelConstantBuffer    = nullptr;
  ID3D11PixelShader*        pPixelShader            = nullptr;
  ID3D11SamplerState*       pFontSampler_clamp      = nullptr;
  ID3D11SamplerState*       pFontSampler_wrap       = nullptr;
  ID3D11ShaderResourceView* pFontTextureView        = nullptr;
  ID3D11RasterizerState*    pRasterizerState        = nullptr;
  ID3D11BlendState*         pBlendState             = nullptr;
  ID3D11DepthStencilState*  pDepthStencilState      = nullptr;

  SK_ComPtr
    < ID3D11Texture2D >     pBackBuffer             = nullptr;
  SK_ComPtr <
      ID3D11RenderTargetView
  >                         pRenderTargetView       = nullptr;
  SK_ComPtr <
      ID3D11RenderTargetView
  >                         pRenderTargetView_sRGB  = nullptr;
  SK_ComPtr <
      ID3D11UnorderedAccessView
  >                         pUnorderedAccessView    = nullptr;

  ID3D11Buffer*             pVB                     = nullptr;
  ID3D11Buffer*             pIB                     = nullptr;

  int                       VertexBufferSize        = 5000,
                            IndexBufferSize         = 10000;
} _Frame [_MAX_BACKBUFFERS];

SK_LazyGlobal <SK_ImGui_D3D11Ctx>  _imgui_d3d11;

static UINT                     g_frameIndex            = 0;//UINT_MAX;
static UINT                     g_numFramesInSwapChain  = 1;
static UINT                     g_frameBufferWidth      = 0UL;
static UINT                     g_frameBufferHeight     = 0UL;

ID3D11RenderTargetView*
SK_D3D11_GetHDRHUDView (void)
{
  //if (g_pHDRHUDView == nullptr)
  //{
  //  bool
  //  ImGui_ImplDX11_CreateDeviceObjects (void);
  //  ImGui_ImplDX11_CreateDeviceObjects ();
  //}
  //
  //if (g_pHDRHUDView != nullptr)
  //{
  //  SK_RenderBackend& rb =
  //    SK_GetCurrentRenderBackend ();
  //
  //  return g_pHDRHUDView;
  //}
  //
  //SK_ReleaseAssert (false)

  // Uh oh, user wanted an update but it was not possible
  //   now they get nothing!
  return nullptr;
}

extern ID3D11ShaderResourceView*
SK_D3D11_GetHDRHUDTexture (void)
{
  //if (g_pHDRHUDTexView == nullptr)
  //{
  //  bool
  //  ImGui_ImplDX11_CreateDeviceObjects (void);
  //  ImGui_ImplDX11_CreateDeviceObjects ();
  //}
  //
  //if (g_pHDRHUDTexView != nullptr)
  //{
  //  return g_pHDRHUDTexView;
  //}
  //
  //SK_ReleaseAssert (false)

  return nullptr;
}


struct VERTEX_CONSTANT_BUFFER
{
  float mvp [4][4];

  // scRGB allows values > 1.0, sRGB (SDR) simply clamps them
  float luminance_scale [4]; // For HDR displays,    1.0 = 80 Nits
                             // For SDR displays, >= 1.0 = 80 Nits
  float steam_luminance   [4];
  float discord_luminance [2];
  float rtss_luminance    [2];
  float unused_padding    [4];
};


struct HISTOGRAM_DISPATCH_CBUFFER
{
  uint32_t imageWidth;
  uint32_t imageHeight;

  float    minLuminance;
  float    maxLuminance;

  uint32_t numLocalZones;

  float    RGB_to_xyY [4][4];
};

#include <SpecialK/tls.h>

// If text or lines are blurry when integrating ImGui in your engine:
// - in your Render function, try translating your projection matrix by (0.5f,0.5f) or (0.375f,0.375f)
IMGUI_API
void
ImGui_ImplDX11_RenderDrawData (ImDrawData* draw_data)
{
  ImGuiIO& io =
    ImGui::GetIO ();

  static auto& rb =
    SK_GetCurrentRenderBackend ();

  if (! rb.swapchain)
    return;

  if (! rb.device)
    return;

  if (! rb.d3d11.immediate_ctx)
    return;

  auto pDevice =
    rb.getDevice <ID3D11Device> ();

  SK_ComQIPtr <ID3D11DeviceContext> pDevCtx    (rb.d3d11.immediate_ctx);
  SK_ComQIPtr <IDXGISwapChain>      pSwapChain (   rb.swapchain       );
  SK_ComQIPtr <IDXGISwapChain3>     pSwap3     (   rb.swapchain       );

  UINT currentBuffer = 0;
   //(g_frameIndex % g_numFramesInSwapChain);

  auto* _P =
    &_Frame [currentBuffer];

  if (! _P->pVertexConstantBuffer)
      return ImGui_ImplDX11_InvalidateDeviceObjects ();

  if (! _P->pPixelConstantBuffer)
    return;

  if (! _P->pRenderTargetView.p)
    return;

  SK_TLS* pTLS =
    SK_TLS_Bottom ();

  // Are our resources on the same device as the SwapChain we're trying to draw into...?
  if (! (SK_D3D11_EnsureMatchingDevices (pSwapChain, pDevice) &&
         SK_D3D11_EnsureMatchingDevices (pDevCtx,    pDevice)))
  {
    if (pSwapChain != nullptr)
    {
      // No, so let's tear it all down and start over :)
      _d3d11_rbk->release (pSwapChain);
    }

    SK_ComPtr <ID3D11Device> pDev;
    if (pSwapChain.p != nullptr)
        pSwapChain->GetDevice (IID_ID3D11Device, (void **)&pDev.p);

    pDevCtx = nullptr;

    if (pDev != nullptr)
        pDev->GetImmediateContext (&pDevCtx.p);

    if ( pSwapChain != nullptr &&
               pDev != nullptr &&
            pDevCtx != nullptr )
    {
      if (_d3d11_rbk->init (pSwapChain, pDev, pDevCtx))
      {
        rb.device              = pDev;
        rb.d3d11.immediate_ctx = pDevCtx;
      }

      else
        pDevCtx = nullptr;
    }

    if (pDevCtx == nullptr)
    {
      SK_RunOnce (
        SK_LOGi0 (L"ImGui_ImplDX11_RenderDrawData (...) aborted; no working render context available.")
      );
      
      return;
    }

#define _SKIP_FULL_FRAME
#ifdef _SKIP_FULL_FRAME
    SK_LOGi0 (L"ImGui_ImplDX11_RenderDrawData (...) skipped because devices do not match");

    return;
#endif
  }

  // This thread and all commands coming from it are currently related to the
  //   UI, we don't want to track the majority of our own state changes
  //     (and more importantly, resources created on any D3D11 device).
  SK_ScopedBool auto_bool (&pTLS->imgui->drawing);
                            pTLS->imgui->drawing = true;

  // Flag the active device command context (not the calling thread) as currently
  //   participating in UI rendering.
  auto flag_result =
    SK_ImGui_FlagDrawing_OnD3D11Ctx (
      SK_D3D11_GetDeviceContextHandle (pDevCtx)
    );

  SK_ScopedBool auto_bool0 (flag_result.first);
                           *flag_result.first = flag_result.second;

  D3D11_TEXTURE2D_DESC       backbuffer_desc = { };
  _P->pBackBuffer->GetDesc (&backbuffer_desc);

  io.DisplaySize.x             = static_cast <float> (backbuffer_desc.Width);
  io.DisplaySize.y             = static_cast <float> (backbuffer_desc.Height);

  io.DisplayFramebufferScale.x = static_cast <float> (backbuffer_desc.Width);
  io.DisplayFramebufferScale.y = static_cast <float> (backbuffer_desc.Height);

  // Create and grow vertex/index buffers if needed
  if ((! _P->pVB             ) ||
         _P->VertexBufferSize < draw_data->TotalVtxCount)
  {
    SK_COM_ValidateRelease ((IUnknown **)&_P->pVB);

    _P->VertexBufferSize =
      draw_data->TotalVtxCount + 5000;

    D3D11_BUFFER_DESC desc
                        = { };

    desc.Usage          = D3D11_USAGE_DYNAMIC;
    desc.ByteWidth      = _P->VertexBufferSize * sizeof (ImDrawVert);
    desc.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags      = 0;

    if (pDevice->CreateBuffer (&desc, nullptr, &_P->pVB) < 0 ||
                                             (! _P->pVB))
      return;
    SK_D3D11_SetDebugName (                     _P->pVB,
                           L"ImGui D3D11 VertexBuffer");
  }

  if ((! _P->pIB            ) ||
         _P->IndexBufferSize < draw_data->TotalIdxCount)
  {
    SK_COM_ValidateRelease ((IUnknown **)&_P->pIB);

    _P->IndexBufferSize =
      draw_data->TotalIdxCount + 10000;

    D3D11_BUFFER_DESC desc
                        = { };

    desc.Usage          = D3D11_USAGE_DYNAMIC;
    desc.ByteWidth      = _P->IndexBufferSize * sizeof (ImDrawIdx);
    desc.BindFlags      = D3D11_BIND_INDEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    if (pDevice->CreateBuffer (&desc, nullptr, &_P->pIB) < 0 ||
                                             (! _P->pIB))
      return;
    SK_D3D11_SetDebugName (                     _P->pIB,
                           L"ImGui D3D11 IndexBuffer");
  }

  // Copy and convert all vertices into a single contiguous buffer
  D3D11_MAPPED_SUBRESOURCE vtx_resource = { },
                           idx_resource = { };

  if (pDevCtx->Map (_P->pVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &vtx_resource) != S_OK)
    return;

  if (pDevCtx->Map (_P->pIB, 0, D3D11_MAP_WRITE_DISCARD, 0, &idx_resource) != S_OK)
  {
    // If for some reason the first one succeeded, but this one failed.... unmap the first one
    //   then abandon all hope.
    pDevCtx->Unmap (_P->pVB, 0);
    return;
  }

  auto* vtx_dst = static_cast <ImDrawVert *> (vtx_resource.pData);
  auto* idx_dst = static_cast <ImDrawIdx  *> (idx_resource.pData);

  for (int n = 0; n < draw_data->CmdListsCount; n++)
  {
    const ImDrawList* cmd_list =
      draw_data->CmdLists [n];

    if (config.imgui.render.strip_alpha)
    {
      for (INT i = 0; i < cmd_list->VtxBuffer.Size; i++)
      {
        ImU32 color =
          ImColor (cmd_list->VtxBuffer.Data [i].col);

        uint8_t alpha = (((color & 0xFF000000U) >> 24U) & 0xFFU);

        // Boost alpha for visibility
        if (alpha < 93 && alpha != 0)
          alpha += (93  - alpha) / 2;

        float a = ((float)                       alpha / 255.0f);
        float r = ((float)((color & 0xFF0000U) >> 16U) / 255.0f);
        float g = ((float)((color & 0x00FF00U) >>  8U) / 255.0f);
        float b = ((float)((color & 0x0000FFU)       ) / 255.0f);

        color =                    0xFF000000U  |
                ((UINT)((r * a) * 255U) << 16U) |
                ((UINT)((g * a) * 255U) <<  8U) |
                ((UINT)((b * a) * 255U)       );

        cmd_list->VtxBuffer.Data[i].col =
          /// XXX: FIXME
#if 0
          (ImVec4)ImColor (color);
#else
          ImColor (color);
#endif
      }
    }

    memcpy (idx_dst,   cmd_list->IdxBuffer.Data,
                       cmd_list->IdxBuffer.Size * sizeof (ImDrawIdx));
            idx_dst += cmd_list->IdxBuffer.Size;
  }

  for (int n = 0; n < draw_data->CmdListsCount; n++)
  {
    const ImDrawList* cmd_list =
      draw_data->CmdLists [n];

    memcpy (vtx_dst,   cmd_list->VtxBuffer.Data,
                       cmd_list->VtxBuffer.Size * sizeof (ImDrawVert));
            vtx_dst += cmd_list->VtxBuffer.Size;
  }

  pDevCtx->Unmap (_P->pIB, 0);
  pDevCtx->Unmap (_P->pVB, 0);

  // Setup orthographic projection matrix into our constant buffer
  {
    float L = draw_data->DisplayPos.x;
    float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
    float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
    float T = draw_data->DisplayPos.y;

    alignas (__m128d) float mvp [4][4] =
    {
      { 2.0f/(R-L),   0.0f,           0.0f,       0.0f },
      { 0.0f,         2.0f/(T-B),     0.0f,       0.0f },
      { 0.0f,         0.0f,           0.5f,       0.0f },
      { (R+L)/(L-R),  (T+B)/(B-T),    0.5f,       1.0f },
    };

    D3D11_MAPPED_SUBRESOURCE mapped_resource;

    if (pDevCtx->Map (_P->pVertexConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource) != S_OK)
      return;

    auto* constant_buffer =
      static_cast <VERTEX_CONSTANT_BUFFER *> (mapped_resource.pData);

    memcpy         (&constant_buffer->mvp, mvp, sizeof (mvp));

    bool hdr_display =
      (rb.isHDRCapable () &&
       rb.isHDRActive  ());

    if (! hdr_display)
    {
      constant_buffer->luminance_scale   [0] = 1.0f; constant_buffer->luminance_scale   [1] = 1.0f;
      constant_buffer->luminance_scale   [2] = 0.0f; constant_buffer->luminance_scale   [3] = 0.0f;
      constant_buffer->steam_luminance   [0] = 1.0f; constant_buffer->steam_luminance   [1] = 1.0f;
      constant_buffer->steam_luminance   [2] = 1.0f; constant_buffer->steam_luminance   [3] = 1.0f;
      constant_buffer->discord_luminance [0] = 1.0f; constant_buffer->discord_luminance [1] = 1.0f;
      constant_buffer->rtss_luminance    [0] = 1.0f; constant_buffer->rtss_luminance    [1] = 1.0f;
    }

    else
    {
      float luma = 0.0f,
            exp  = 0.0f;

      luma = __SK_HDR_Luma;
      exp  = __SK_HDR_Exp;

      SK_RenderBackend::scan_out_s::SK_HDR_TRANSFER_FUNC eotf =
        rb.scanout.getEOTF ();

      bool bEOTF_is_PQ =
        (eotf == SK_RenderBackend::scan_out_s::SMPTE_2084);

      constant_buffer->luminance_scale   [0] = ( bEOTF_is_PQ ? -80.0f * rb.ui_luminance :
                                                                        rb.ui_luminance );
      constant_buffer->luminance_scale   [1] = 2.2f;
      constant_buffer->luminance_scale   [2] = rb.display_gamut.minY * 1.0_Nits;
      constant_buffer->steam_luminance   [0] = ( bEOTF_is_PQ ? -80.0f * config.platform.overlay_hdr_luminance :
                                                                        config.platform.overlay_hdr_luminance );
      constant_buffer->steam_luminance   [1] = 2.2f;//( bEOTF_is_PQ ? 1.0f : (rb.ui_srgb ? 2.2f :
                                                    //                                     1.0f));
      constant_buffer->steam_luminance   [2] = ( bEOTF_is_PQ ? -80.0f * config.uplay.overlay_luminance :
                                                                        config.uplay.overlay_luminance );
      constant_buffer->steam_luminance   [3] = 2.2f;//( bEOTF_is_PQ ? 1.0f : (rb.ui_srgb ? 2.2f :
                                                  //                1.0f));
      constant_buffer->rtss_luminance    [0] = ( bEOTF_is_PQ ? -80.0f * config.rtss.overlay_luminance :
                                                                        config.rtss.overlay_luminance );
      constant_buffer->rtss_luminance    [1] = 2.2f;

      constant_buffer->discord_luminance [0] = ( bEOTF_is_PQ ? -80.0f * config.discord.overlay_luminance :
                                                                        config.discord.overlay_luminance );
      constant_buffer->discord_luminance [1] = 2.2f;
    }

    pDevCtx->Unmap (_P->pVertexConstantBuffer, 0);

    if (pDevCtx->Map (_P->pPixelConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource) != S_OK)
      return;

    ((float *)mapped_resource.pData)[2] = hdr_display ? (float)backbuffer_desc.Width  : 0.0f;
    ((float *)mapped_resource.pData)[3] = hdr_display ? (float)backbuffer_desc.Height : 0.0f;

    D3D11_TEXTURE2D_DESC             tex2d_desc = { };
    _P->pBackBuffer->GetDesc       (&tex2d_desc);

    // For temporarily disabled Linear sRGB mode
    ((float *)mapped_resource.pData)[4] = 0.0F;//DirectX::MakeTypeless (tex2d_desc.Format) == DXGI_FORMAT_R8G8B8A8_TYPELESS ? (float)1.0f :
                                               //DirectX::MakeTypeless (tex2d_desc.Format) == DXGI_FORMAT_B8G8R8A8_TYPELESS ? (float)1.0f : 0.0f;

    pDevCtx->Unmap (_P->pPixelConstantBuffer, 0);
  }

  // pcmd->TextureId may not point to a valid object anymore, so we do this...
  auto orig_se =
    SK_SEH_ApplyTranslator(
      SK_FilteringStructuredExceptionTranslator(
        EXCEPTION_ACCESS_VIOLATION
      )
    );
  try
  {
    SK_ComPtr <ID3D11Device> pDev;
    pDevCtx->GetDevice     (&pDev.p);

    pDevCtx->OMSetRenderTargets ( 1,
                                   &_P->pRenderTargetView.p,
                                      nullptr );

    // Setup viewport
    D3D11_VIEWPORT vp = { };

    vp.Height   = draw_data->DisplaySize.y;
    vp.Width    = draw_data->DisplaySize.x;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = vp.TopLeftY = 0.0f;

    pDevCtx->RSSetViewports (1, &vp);

    // Bind shader and vertex buffers
    unsigned int stride = sizeof (ImDrawVert);
    unsigned int offset = 0;

    pDevCtx->IASetInputLayout       (_P->pInputLayout);
    pDevCtx->IASetVertexBuffers     (0, 1, &_P->pVB, &stride, &offset);
    pDevCtx->IASetIndexBuffer       (_P->pIB, sizeof (ImDrawIdx) == 2 ?
                                                DXGI_FORMAT_R16_UINT  :
                                                DXGI_FORMAT_R32_UINT,
                                                  0 );
    pDevCtx->IASetPrimitiveTopology (D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    if (pDev->GetFeatureLevel () >= D3D_FEATURE_LEVEL_10_0)
    {
      pDevCtx->GSSetShader          (nullptr, nullptr, 0);
    }

    if (pDev->GetFeatureLevel () >= D3D_FEATURE_LEVEL_11_0)
    {
      pDevCtx->HSSetShader          (nullptr, nullptr, 0);
      pDevCtx->DSSetShader          (nullptr, nullptr, 0);
    }

    pDevCtx->VSSetShader            (_P->pVertexShader, nullptr, 0);
    pDevCtx->VSSetConstantBuffers   (0, 1, &_P->pVertexConstantBuffer);

    pDevCtx->PSSetShader            (_P->pPixelShader, nullptr, 0);
    pDevCtx->PSSetSamplers          (0, 1, &_P->pFontSampler_clamp);
    pDevCtx->PSSetConstantBuffers   (0, 1, &_P->pPixelConstantBuffer);

    // Setup render state
    pDevCtx->OMSetBlendState        (_P->pBlendState, nullptr, 0xffffffff);
    pDevCtx->OMSetDepthStencilState (_P->pDepthStencilState,        0);
    pDevCtx->RSSetState             (_P->pRasterizerState);

    // Render command lists
    int vtx_offset = 0;
    int idx_offset = 0;

    ImVec2 clip_off = draw_data->DisplayPos;

    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
      const ImDrawList* cmd_list =
        draw_data->CmdLists [n];
    
      for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
      {
        const ImDrawCmd* pcmd =
          &cmd_list->CmdBuffer [cmd_i];
    
        if (pcmd->UserCallback)
        {
          // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
          if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
            SK_ReleaseAssert (!L"ImDrawCallback_ResetRenderState = Not Implemented");// ImGui_ImplDX11_SetupRenderState (draw_data, pDevCtx);
          else
            pcmd->UserCallback (cmd_list, pcmd);
        }
    
        else
        {
          // Project scissor/clipping rectangles into framebuffer space
          ImVec2 clip_min (pcmd->ClipRect.x - clip_off.x, pcmd->ClipRect.y - clip_off.y);
          ImVec2 clip_max (pcmd->ClipRect.z - clip_off.x, pcmd->ClipRect.w - clip_off.y);

          if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
              continue;

          // Apply scissor/clipping rectangle
          const D3D11_RECT r = { 
            static_cast <LONG> (clip_min.x), static_cast <LONG> (clip_min.y),
            static_cast <LONG> (clip_max.x), static_cast <LONG> (clip_max.y)
          };
    
          extern ID3D11ShaderResourceView*
            SK_HDR_GetUnderlayResourceView (void);
    
          ID3D11ShaderResourceView* views [2] =
          {
            (ID3D11ShaderResourceView *)pcmd->GetTexID (),
            SK_HDR_GetUnderlayResourceView ()
          };
    
          pDevCtx->PSSetSamplers        (0, 1, &_P->pFontSampler_wrap);
          pDevCtx->PSSetShaderResources (0, 2, views);
          pDevCtx->RSSetScissorRects    (1, &r);
    
          pDevCtx->DrawIndexed          (pcmd->ElemCount, pcmd->IdxOffset + idx_offset,
                                                          pcmd->VtxOffset + vtx_offset);
        }
      }

      idx_offset += cmd_list->IdxBuffer.Size;
      vtx_offset += cmd_list->VtxBuffer.Size;
    }
  }
  catch (const SK_SEH_IgnoredException&)
  {
    // ...
  }
  SK_SEH_RemoveTranslator (orig_se);
}

#include <SpecialK/config.h>

static bool
ImGui_ImplDX11_CreateFontsTexture ( IDXGISwapChain* /*pSwapChain*/,
                                    ID3D11Device*   pDev )
{
  if (! pDev) // Try again later
    return false;

  SK_TLS* pTLS =
    SK_TLS_Bottom ();

  if (! pTLS) return false;

  auto _BuildForSlot = [&](UINT slot) -> void
  {
    auto* _P =
      &_Frame [slot];

    if (_P->pFontSampler_clamp != nullptr &&
        _P->pFontSampler_wrap  != nullptr &&
        _P->pFontTextureView   != nullptr) return;


    // Do not dump ImGui font textures
    SK_ScopedBool auto_bool (&pTLS->imgui->drawing);
                              pTLS->imgui->drawing = true;

    SK_ScopedBool decl_tex_scope (
      SK_D3D11_DeclareTexInjectScope (pTLS)
    );

    // Build texture atlas
    ImGuiIO& io (
      ImGui::GetIO ()
    );

    unsigned char* pixels = nullptr;
    int            width  = 0,
                   height = 0;

    io.Fonts->GetTexDataAsAlpha8 ( &pixels,
                                   &width, &height );

    // Upload texture to graphics system
    D3D11_TEXTURE2D_DESC
      staging_desc                  = { };
      staging_desc.Width            = width;
      staging_desc.Height           = height;
      staging_desc.MipLevels        = 1;
      staging_desc.ArraySize        = 1;
      staging_desc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
      staging_desc.SampleDesc.Count = 1;
      staging_desc.Usage            = D3D11_USAGE_STAGING;
      staging_desc.BindFlags        = 0;
      staging_desc.CPUAccessFlags   = D3D11_CPU_ACCESS_WRITE;

    D3D11_TEXTURE2D_DESC
      tex_desc                      = staging_desc;
      tex_desc.Usage                = D3D11_USAGE_DEFAULT;
      tex_desc.BindFlags            = D3D11_BIND_SHADER_RESOURCE;
      tex_desc.CPUAccessFlags       = 0;

    SK_ComPtr <ID3D11Texture2D>       pStagingTexture = nullptr;
    SK_ComPtr <ID3D11Texture2D>       pFontTexture    = nullptr;

    ThrowIfFailed (
      pDev->CreateTexture2D ( &staging_desc, nullptr,
                                     &pStagingTexture.p ));
        SK_D3D11_SetDebugName   (     pStagingTexture.p,
                               L"ImGui Staging Texture");

    ThrowIfFailed (
      pDev->CreateTexture2D ( &tex_desc,     nullptr,
                                     &pFontTexture.p ));
    SK_D3D11_SetDebugName   (         pFontTexture.p,
                               L"ImGui Font Texture");

    SK_ComPtr   <ID3D11DeviceContext> pDevCtx;
    pDev->GetImmediateContext       (&pDevCtx);

    D3D11_MAPPED_SUBRESOURCE
          mapped_tex = { };

    ThrowIfFailed (
      pDevCtx->Map ( pStagingTexture.p, 0, D3D11_MAP_WRITE, 0,
                     &mapped_tex ));

    for (int y = 0; y < height; y++)
    {
      ImU32  *pDst =
        (ImU32 *)((uintptr_t)mapped_tex.pData +
                             mapped_tex.RowPitch * y);
      ImU8   *pSrc =              pixels + width * y;

      for (int x = 0; x < width; x++)
      {
        *pDst++ =
          IM_COL32 (255, 255, 255, (ImU32)(*pSrc++));
      }
    }

    pDevCtx->Unmap        ( pStagingTexture, 0 );
    pDevCtx->CopyResource (    pFontTexture,
                            pStagingTexture    );

    // Create texture view
    D3D11_SHADER_RESOURCE_VIEW_DESC
      srvDesc = { };
      srvDesc.Format                    = DXGI_FORMAT_R8G8B8A8_UNORM;
      srvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
      srvDesc.Texture2D.MipLevels       = tex_desc.MipLevels;
      srvDesc.Texture2D.MostDetailedMip = 0;

    ThrowIfFailed (
      pDev->CreateShaderResourceView ( pFontTexture, &srvDesc,
                                   &_P->pFontTextureView ));
    SK_D3D11_SetDebugName (         _P->pFontTextureView,
                                L"ImGui Font SRV");

    // Store our identifier
    io.Fonts->TexID =
      _P->pFontTextureView;

    // Create texture sampler
    D3D11_SAMPLER_DESC
      sampler_desc                    = { };
      sampler_desc.Filter             =  D3D11_FILTER_MIN_MAG_MIP_LINEAR;
      sampler_desc.AddressU           =  D3D11_TEXTURE_ADDRESS_CLAMP;
      sampler_desc.AddressV           =  D3D11_TEXTURE_ADDRESS_CLAMP;
      sampler_desc.AddressW           =  D3D11_TEXTURE_ADDRESS_CLAMP;
      sampler_desc.MipLODBias         =  0.f;
      sampler_desc.ComparisonFunc     =  D3D11_COMPARISON_NEVER;
      sampler_desc.MinLOD             = -D3D11_FLOAT32_MAX;
      sampler_desc.MaxLOD             =  D3D11_FLOAT32_MAX;

    ThrowIfFailed (
      pDev->CreateSamplerState ( &sampler_desc,
                         &_P->pFontSampler_clamp ));

      sampler_desc.AddressU           =  D3D11_TEXTURE_ADDRESS_WRAP;
      sampler_desc.AddressV           =  D3D11_TEXTURE_ADDRESS_WRAP;
      sampler_desc.AddressW           =  D3D11_TEXTURE_ADDRESS_WRAP;

    ThrowIfFailed (
      pDev->CreateSamplerState ( &sampler_desc,
                         &_P->pFontSampler_wrap ));
  };

  try
  {
    auto i = 0;
    //for ( auto i = 0 ; i < _MAX_BACKBUFFERS ; ++i )
    {
      _BuildForSlot (i);
    }
  }

  catch (const SK_ComException& e)
  {
    SK_LOGi0 (
      L"ImGui Font: %hs",       e.what ()
    );

    return false;
  }

  return true;
}

HRESULT
SK_D3D11_Inject_uPlayHDR ( _In_ ID3D11DeviceContext  *pDevCtx,
                           _In_ UINT                  IndexCount,
                           _In_ UINT                  StartIndexLocation,
                           _In_ INT                   BaseVertexLocation,
                           _In_ D3D11_DrawIndexed_pfn pfnD3D11DrawIndexed )
{
  if (! SK_D3D11_EnsureMatchingDevices (pDevCtx, _d3d11_rbk->_pDevice))
    return E_NOT_VALID_STATE;

  auto* _P =
    &_Frame [g_frameIndex % g_numFramesInSwapChain];

  if ( _P->pVertexShaderuPlayHDR != nullptr &&
       _P->pVertexConstantBuffer != nullptr &&
       _P->pPixelShaderuPlayHDR  != nullptr    )
  {
    auto flag_result =
      SK_ImGui_FlagDrawing_OnD3D11Ctx (
        SK_D3D11_GetDeviceContextHandle (pDevCtx)
      );

    SK_ScopedBool auto_bool0 (flag_result.first);
                             *flag_result.first = flag_result.second;

    pDevCtx->VSSetShader          ( _P->pVertexShaderuPlayHDR,
                                      nullptr, 0 );
    pDevCtx->PSSetShader          ( _P->pPixelShaderuPlayHDR,
                                      nullptr, 0 );
    pDevCtx->VSSetConstantBuffers ( 0, 1,
                                    &_P->pVertexConstantBuffer );
    pfnD3D11DrawIndexed ( pDevCtx, IndexCount, StartIndexLocation, BaseVertexLocation );

    return
      S_OK;
  }

  return
    E_NOT_VALID_STATE;
}

#define STEAM_OVERLAY_VS_CRC32C   0xf48cf597
#define EPIC_OVERLAY_VS_CRC32C    0xa7ee5199
#define DISCORD_OVERLAY_VS_CRC32C 0x085ee17b
#define RTSS_OVERLAY_VS_CRC32C    0x671afc2f

HRESULT
SK_D3D11_InjectSteamHDR ( _In_ ID3D11DeviceContext *pDevCtx,
                          _In_ UINT                 VertexCount,
                          _In_ UINT                 StartVertexLocation,
                          _In_ D3D11_Draw_pfn       pfnD3D11Draw )
{
  if (! SK_D3D11_EnsureMatchingDevices (pDevCtx, _d3d11_rbk->_pDevice))
    return E_NOT_VALID_STATE;

  auto* _P =
    &_Frame [g_frameIndex % g_numFramesInSwapChain];

  auto flag_result =
    SK_ImGui_FlagDrawing_OnD3D11Ctx (
      SK_D3D11_GetDeviceContextHandle (pDevCtx)
    );

  SK_ScopedBool auto_bool0 (flag_result.first);
                           *flag_result.first = flag_result.second;

  if ( _P->pVertexShaderSteamHDR != nullptr &&
       _P->pVertexConstantBuffer != nullptr &&
       _P->pPixelShaderSteamHDR  != nullptr    )
  {
    pDevCtx->VSSetShader          ( _P->pVertexShaderSteamHDR,
                                      nullptr, 0 );
    pDevCtx->PSSetShader          ( _P->pPixelShaderSteamHDR,
                                      nullptr, 0 );
    pDevCtx->VSSetConstantBuffers ( 0, 1,
                                    &_P->pVertexConstantBuffer );
    pfnD3D11Draw ( pDevCtx, VertexCount, StartVertexLocation );

    return
      S_OK;
  }

  return
    E_NOT_VALID_STATE;
}

HRESULT
SK_D3D11_InjectGenericHDROverlay ( _In_ ID3D11DeviceContext *pDevCtx,
                                   _In_ UINT                 VertexCount,
                                   _In_ UINT                 StartVertexLocation,
                                   _In_ uint32_t             crc32,
                                   _In_ D3D11_Draw_pfn       pfnD3D11Draw )
{
  if (! SK_D3D11_EnsureMatchingDevices (pDevCtx, _d3d11_rbk->_pDevice))
    return E_NOT_VALID_STATE;

  UNREFERENCED_PARAMETER (crc32);

  auto* _P =
    &_Frame [g_frameIndex % g_numFramesInSwapChain];

  auto flag_result =
    SK_ImGui_FlagDrawing_OnD3D11Ctx (
      SK_D3D11_GetDeviceContextHandle (pDevCtx)
    );

  SK_ScopedBool auto_bool0 (flag_result.first);
                           *flag_result.first = flag_result.second;

  if ( _P->pVertexShaderDiscordHDR != nullptr &&
       _P->pVertexConstantBuffer   != nullptr &&
       _P->pPixelShaderDiscordHDR  != nullptr    )
  {
    if (crc32 == DISCORD_OVERLAY_VS_CRC32C)
    {
      config.discord.present = true;
      pDevCtx->VSSetShader          ( _P->pVertexShaderDiscordHDR,
                                        nullptr, 0 );
    }

    else if (crc32 == RTSS_OVERLAY_VS_CRC32C)
    {
      config.rtss.present = true;
      pDevCtx->VSSetShader          ( _P->pVertexShaderRTSSHDR,
                                        nullptr, 0 );
    }

    pDevCtx->PSSetShader          ( _P->pPixelShaderDiscordHDR,
                                      nullptr, 0 );
    pDevCtx->VSSetConstantBuffers ( 0, 1,
                                    &_P->pVertexConstantBuffer );
    pfnD3D11Draw ( pDevCtx, VertexCount, StartVertexLocation );

    return
      S_OK;
  }

  return
    E_NOT_VALID_STATE;
}

HRESULT
SK_D3D11_Inject_EpicHDR ( _In_ ID3D11DeviceContext  *pDevCtx,
                          _In_ UINT                  IndexCount,
                          _In_ UINT                  StartIndexLocation,
                          _In_ INT                   BaseVertexLocation,
                          _In_ D3D11_DrawIndexed_pfn pfnD3D11DrawIndexed )
{
  if (! SK_D3D11_EnsureMatchingDevices (pDevCtx, _d3d11_rbk->_pDevice))
    return E_NOT_VALID_STATE;

  auto* _P =
    &_Frame [g_frameIndex % g_numFramesInSwapChain];

  auto flag_result =
    SK_ImGui_FlagDrawing_OnD3D11Ctx (
      SK_D3D11_GetDeviceContextHandle (pDevCtx)
    );

  SK_ScopedBool auto_bool0 (flag_result.first);
                           *flag_result.first = flag_result.second;

  if ( _P->pVertexShaderEpicHDR  != nullptr &&
       _P->pVertexConstantBuffer != nullptr &&
       _P->pPixelShaderSteamHDR  != nullptr    )
  {
    pDevCtx->VSSetShader          ( _P->pVertexShaderEpicHDR,
                                      nullptr, 0 );
    pDevCtx->PSSetShader          ( _P->pPixelShaderSteamHDR,
                                      nullptr, 0 );
    pDevCtx->VSSetConstantBuffers ( 0, 1,
                                    &_P->pVertexConstantBuffer );
    pfnD3D11DrawIndexed ( pDevCtx, IndexCount, StartIndexLocation, BaseVertexLocation );

    config.epic.present = true;

    return
      S_OK;
  }

  return
    E_NOT_VALID_STATE;
}

bool
ImGui_ImplDX11_CreateDeviceObjectsForBackbuffer ( IDXGISwapChain*      pSwapChain,
                                                  ID3D11Device*        pDev,
                                                  ID3D11DeviceContext* pDevCtx,
                                                  UINT                 idx )
{
  auto* _P =
    &_Frame [idx];

  SK_TLS* pTLS =
    SK_TLS_Bottom ();

  if (! pTLS) return false;

  SK_ScopedBool auto_bool (&pTLS->imgui->drawing);

  // Do not dump ImGui font textures
  pTLS->imgui->drawing = true;

  //auto pDev =
  //  rb.getDevice <ID3D11Device> ();

  if (! (pDev && pDevCtx && pSwapChain))
    return false;

  auto flag_result =
    SK_ImGui_FlagDrawing_OnD3D11Ctx (
      SK_D3D11_GetDeviceContextHandle (pDevCtx)
    );

  SK_ScopedBool auto_bool0 (flag_result.first);
                           *flag_result.first = flag_result.second;

  try
  {
    if (_P->pVertexShader == nullptr)
    {
      ThrowIfFailed (
        pDev->CreateVertexShader ( (void *)(imgui_d3d11_vs_bytecode),
                                    sizeof (imgui_d3d11_vs_bytecode) /
                                    sizeof (imgui_d3d11_vs_bytecode [0]),
                                      nullptr, &_P->pVertexShader));
      SK_D3D11_SetDebugName (                   _P->pVertexShader,
                                            L"ImGui Vertex Shader");
    }

    if (_P->pVertexShaderSteamHDR == nullptr)
    {
      ThrowIfFailed (
        pDev->CreateVertexShader ( (void *)(steam_d3d11_vs_bytecode),
                                    sizeof (steam_d3d11_vs_bytecode) /
                                    sizeof (steam_d3d11_vs_bytecode [0]),
                                      nullptr, &_P->pVertexShaderSteamHDR));
      SK_D3D11_SetDebugName (                   _P->pVertexShaderSteamHDR,
                                L"Steam Overlay HDR Vertex Shader");
    }

    if (_P->pVertexShaderDiscordHDR == nullptr)
    {
      ThrowIfFailed (
        pDev->CreateVertexShader ( (void *)(discord_d3d11_vs_bytecode),
                                    sizeof (discord_d3d11_vs_bytecode) /
                                    sizeof (discord_d3d11_vs_bytecode [0]),
                                      nullptr, &_P->pVertexShaderDiscordHDR));
      SK_D3D11_SetDebugName (                   _P->pVertexShaderDiscordHDR,
                                L"Discord Overlay HDR Vertex Shader");
    }

    if (_P->pVertexShaderRTSSHDR == nullptr)
    {
      ThrowIfFailed (
        pDev->CreateVertexShader ( (void *)(rtss_d3d11_vs_bytecode),
                                    sizeof (rtss_d3d11_vs_bytecode) /
                                    sizeof (rtss_d3d11_vs_bytecode [0]),
                                      nullptr, &_P->pVertexShaderRTSSHDR));
      SK_D3D11_SetDebugName (                   _P->pVertexShaderRTSSHDR,
                                L"RTSS Overlay HDR Vertex Shader");
    }

    if (_P->pVertexShaderuPlayHDR == nullptr)
    {
      ThrowIfFailed (
        pDev->CreateVertexShader ( (void *)(uplay_d3d11_vs_bytecode),
                                    sizeof (uplay_d3d11_vs_bytecode) /
                                    sizeof (uplay_d3d11_vs_bytecode [0]),
                                      nullptr, &_P->pVertexShaderuPlayHDR));
      SK_D3D11_SetDebugName (                   _P->pVertexShaderuPlayHDR,
                                L"uPlay Overlay HDR Vertex Shader");
    }

    if (_P->pVertexShaderEpicHDR == nullptr)
    {
      ThrowIfFailed (
        pDev->CreateVertexShader ( (void *)(epic_d3d11_vs_bytecode),
                                    sizeof (epic_d3d11_vs_bytecode) /
                                    sizeof (epic_d3d11_vs_bytecode [0]),
                                      nullptr, &_P->pVertexShaderEpicHDR));
      SK_D3D11_SetDebugName (                   _P->pVertexShaderEpicHDR,
                                L"Epic Overlay HDR Vertex Shader");
    }

    // Create the input layout
    D3D11_INPUT_ELEMENT_DESC    local_layout [] = {
      { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,       0, (size_t)(&((ImDrawVert *)nullptr)->pos), D3D11_INPUT_PER_VERTEX_DATA, 0 },
      { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, (size_t)(&((ImDrawVert *)nullptr)->uv),  D3D11_INPUT_PER_VERTEX_DATA, 0 },
#if 0 // NO HDR ImGui Yet
      { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, (size_t)(&((ImDrawVert *)nullptr)->col), D3D11_INPUT_PER_VERTEX_DATA, 0 },
#else
      { "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM,     0, (size_t)(&((ImDrawVert *)nullptr)->col), D3D11_INPUT_PER_VERTEX_DATA, 0 },
#endif
    };

    if (_P->pInputLayout == nullptr)
    {
      ThrowIfFailed (
        pDev->CreateInputLayout ( local_layout, 3,
                                     (void *)(imgui_d3d11_vs_bytecode),
                                      sizeof (imgui_d3d11_vs_bytecode) /
                                      sizeof (imgui_d3d11_vs_bytecode [0]),
                              &_P->pInputLayout ));
      SK_D3D11_SetDebugName (  _P->pInputLayout, L"ImGui Vertex Input Layout");
    }

    // Create the constant buffers
    {
      D3D11_BUFFER_DESC desc = { };

      desc.ByteWidth         = sizeof (VERTEX_CONSTANT_BUFFER);
      desc.Usage             = D3D11_USAGE_DYNAMIC;
      desc.BindFlags         = D3D11_BIND_CONSTANT_BUFFER;
      desc.CPUAccessFlags    = D3D11_CPU_ACCESS_WRITE;
      desc.MiscFlags         = 0;

      if (_P->pVertexConstantBuffer == nullptr)
      {
        ThrowIfFailed (
          pDev->CreateBuffer (&desc, nullptr, &_P->pVertexConstantBuffer));
        SK_D3D11_SetDebugName (                _P->pVertexConstantBuffer,
                                          L"ImGui Vertex Constant Buffer");
      }

      desc.ByteWidth         = sizeof (float) * 8;

      if (_P->pPixelConstantBuffer == nullptr)
      {
        ThrowIfFailed (
          pDev->CreateBuffer (&desc, nullptr, &_P->pPixelConstantBuffer));
        SK_D3D11_SetDebugName (                _P->pPixelConstantBuffer,
                                          L"ImGui Pixel Constant Buffer");
      }
    }

    if (_P->pPixelShader == nullptr)
    {
      ThrowIfFailed (
        pDev->CreatePixelShader ( (void *)(imgui_d3d11_ps_bytecode),
                                   sizeof (imgui_d3d11_ps_bytecode) /
                                   sizeof (imgui_d3d11_ps_bytecode [0]),
                                     nullptr, &_P->pPixelShader ));
      SK_D3D11_SetDebugName (                  _P->pPixelShader,
                                           L"ImGui Pixel Shader");
    }

    if (_P->pPixelShaderSteamHDR == nullptr)
    {
      ThrowIfFailed (
        pDev->CreatePixelShader ( (void *) (steam_d3d11_ps_bytecode),
                                    sizeof (steam_d3d11_ps_bytecode) /
                                    sizeof (steam_d3d11_ps_bytecode [0]),
                                      nullptr, &_P->pPixelShaderSteamHDR ));
      SK_D3D11_SetDebugName (                   _P->pPixelShaderSteamHDR,
                                 L"Steam Overlay HDR Pixel Shader");
    }

    if (_P->pPixelShaderDiscordHDR == nullptr)
    {
      ThrowIfFailed (
        pDev->CreatePixelShader ( (void *) (discord_d3d11_ps_bytecode),
                                    sizeof (discord_d3d11_ps_bytecode) /
                                    sizeof (discord_d3d11_ps_bytecode [0]),
                                      nullptr, &_P->pPixelShaderDiscordHDR ));
      SK_D3D11_SetDebugName (                   _P->pPixelShaderDiscordHDR,
                                 L"Discord Overlay HDR Pixel Shader");
    }

    if (_P->pPixelShaderuPlayHDR == nullptr)
    {
      ThrowIfFailed (
        pDev->CreatePixelShader ( (void *) (uplay_d3d11_ps_bytecode),
                                    sizeof (uplay_d3d11_ps_bytecode) /
                                    sizeof (uplay_d3d11_ps_bytecode [0]),
                                      nullptr, &_P->pPixelShaderuPlayHDR ));
      SK_D3D11_SetDebugName (                   _P->pPixelShaderuPlayHDR,
                                 L"uPlay Overlay HDR Pixel Shader");
    }

    // Create the blending setup
    {  
      D3D11_BLEND_DESC
        desc                                        = {   };

      for ( UINT i = 0 ; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT ; ++i )
      {
        desc.RenderTarget [i].BlendEnable           = FALSE;
        desc.RenderTarget [i].SrcBlend              = D3D11_BLEND_ONE;
        desc.RenderTarget [i].DestBlend             = D3D11_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget [i].BlendOp               = D3D11_BLEND_OP_ADD;
        desc.RenderTarget [i].SrcBlendAlpha         = D3D11_BLEND_ONE;
        desc.RenderTarget [i].DestBlendAlpha        = D3D11_BLEND_ZERO;
        desc.RenderTarget [i].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
        desc.RenderTarget [i].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_RED   |
                                                      D3D11_COLOR_WRITE_ENABLE_GREEN |
                                                      D3D11_COLOR_WRITE_ENABLE_BLUE;
      } desc.RenderTarget [0].BlendEnable           = TRUE;

      if (_P->pBlendState == nullptr)
      {
        ThrowIfFailed (
          pDev->CreateBlendState (&desc, &_P->pBlendState));
        SK_D3D11_SetDebugName (           _P->pBlendState,
                                      L"ImGui Blend State");
      }
    }

    // Create the rasterizer state
    {
      D3D11_RASTERIZER_DESC desc = { };

      desc.FillMode        = D3D11_FILL_SOLID;
      desc.CullMode        = D3D11_CULL_NONE;
      desc.ScissorEnable   = TRUE;
      desc.DepthClipEnable = TRUE;

      if (_P->pRasterizerState == nullptr)
      {
        ThrowIfFailed (
          pDev->CreateRasterizerState (&desc, &_P->pRasterizerState ));
        SK_D3D11_SetDebugName (                _P->pRasterizerState,
                                           L"ImGui Rasterizer State");
      }
    }

    // Create depth-stencil State
    {
      D3D11_DEPTH_STENCIL_DESC desc = { };

      desc.DepthEnable              = FALSE;
      desc.StencilEnable            = FALSE;
      desc.DepthWriteMask           = D3D11_DEPTH_WRITE_MASK_ZERO;
      desc.DepthFunc                = D3D11_COMPARISON_ALWAYS;
      desc.FrontFace.StencilFailOp  = desc.FrontFace.StencilDepthFailOp =
                                      desc.FrontFace.StencilPassOp      =
                                      D3D11_STENCIL_OP_KEEP;
      desc.FrontFace.StencilFunc    = D3D11_COMPARISON_ALWAYS;
      desc.BackFace                 = desc.FrontFace;
      

      if (_P->pDepthStencilState == nullptr)
      {
        ThrowIfFailed (
          pDev->CreateDepthStencilState (&desc, &_P->pDepthStencilState));
        SK_D3D11_SetDebugName (                  _P->pDepthStencilState,
                                             L"ImGui Depth/Stencil State");
      }
    }

    if (_P->pBackBuffer == nullptr)
    {
      UINT size = sizeof (LPVOID);

      SK_ComPtr <IDXGISwapChain>           pUnwrappedSwapChain;
      pSwapChain->GetPrivateData ( IID_IUnwrappedDXGISwapChain, &size,
                                 (void **)&pUnwrappedSwapChain.p );

      if (pUnwrappedSwapChain.p == nullptr)
          pUnwrappedSwapChain = pSwapChain;

      ThrowIfFailed (
        pUnwrappedSwapChain->GetBuffer  (0, IID_PPV_ARGS (&_P->pBackBuffer.p))
      ); SK_D3D11_SetDebugName (                           _P->pBackBuffer,
                                                        L"ImGui BackBuffer");
    }

    D3D11_RENDER_TARGET_VIEW_DESC       rt_desc = { };
    D3D11_RENDER_TARGET_VIEW_DESC        *pDesc = &rt_desc;
    D3D11_TEXTURE2D_DESC             tex2d_desc = { };
    _P->pBackBuffer->GetDesc       (&tex2d_desc);

    UINT sizeSwapChainFormat =
         sizeof (DXGI_FORMAT);

    _P->pBackBuffer.p->GetPrivateData (
      SKID_DXGI_SwapChainBackbufferFormat,
                     &sizeSwapChainFormat,
                       &tex2d_desc.Format );

    // SRGB Correction for UIs
    switch (DirectX::MakeTypeless (tex2d_desc.Format))
    {
      case DXGI_FORMAT_R8G8B8A8_TYPELESS:
      {
        rt_desc.Format        = DXGI_FORMAT_R8G8B8A8_UNORM;//_SRGB;
        rt_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
      } break;

      case DXGI_FORMAT_B8G8R8A8_TYPELESS:
      {
        rt_desc.Format        = DXGI_FORMAT_B8G8R8A8_UNORM;//_SRGB;
        rt_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
      } break;

      case DXGI_FORMAT_R16G16B16A16_TYPELESS:
      {
        rt_desc.Format        = DXGI_FORMAT_R16G16B16A16_FLOAT;
        rt_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
      } break;

      case DXGI_FORMAT_R10G10B10A2_TYPELESS:
      {
        rt_desc.Format        = DXGI_FORMAT_R10G10B10A2_UNORM;
        rt_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
      } break;

      default:
      {
        SK_LOGi0 (
          L"Unexpected SwapChain Format Encountered (%hs)",
            SK_DXGI_FormatToStr (tex2d_desc.Format).data ()
        );
        rt_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        rt_desc.Format        =
          DirectX::MakeTypelessUNORM (
          DirectX::MakeTypelessFLOAT (tex2d_desc.Format));
      } break;
    }

    if (tex2d_desc.SampleDesc.Count > 1)
      rt_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;

    if (_P->pRenderTargetView == nullptr)
    {
      ThrowIfFailed (
        pDev->CreateRenderTargetView (  _P->pBackBuffer,         pDesc,
                                       &_P->pRenderTargetView.p ));
      SK_D3D11_SetDebugName (           _P->pRenderTargetView,
                                     L"ImGui RenderTargetView" );
    }

    if (_P->pRenderTargetView_sRGB == nullptr)
    {
      auto desc = *pDesc;

      desc.Format =
        DirectX::MakeSRGB (desc.Format);
      if (DirectX::IsSRGB (desc.Format))
      {
        ThrowIfFailed (
          pDev->CreateRenderTargetView (  _P->pBackBuffer,              &desc,
                                         &_P->pRenderTargetView_sRGB.p ));
        SK_D3D11_SetDebugName (           _P->pRenderTargetView_sRGB,
                                  L"ImGui sRGB RenderTargetView" );
      }
    }

    if (_P->pUnorderedAccessView == nullptr)
    {
      DXGI_SWAP_CHAIN_DESC  swapDesc = { };
      pSwapChain->GetDesc (&swapDesc);

      if ( (swapDesc.BufferUsage & DXGI_USAGE_UNORDERED_ACCESS)
                                == DXGI_USAGE_UNORDERED_ACCESS )
      {
        D3D11_UNORDERED_ACCESS_VIEW_DESC
        uav_desc                    = { };
        uav_desc.ViewDimension      = D3D11_UAV_DIMENSION_TEXTURE2D;
        uav_desc.Texture2D.MipSlice = 0;
        uav_desc.Format             = rt_desc.Format;

        ThrowIfFailed (
          pDev->CreateUnorderedAccessView (_P->pBackBuffer, &uav_desc,
                                          &_P->pUnorderedAccessView.p));
        SK_D3D11_SetDebugName (            _P->pUnorderedAccessView,
                                      L"ImGui [HDR] UnorderedAccessView" );
      }
    }

    return true;
  }

  catch (const SK_ComException& e)
  {
    SK_LOGi0 (
      L"ImGui Backbuffer Resources: %hs",
                                e.what () );

    ImGui_ImplDX11_InvalidateDeviceObjects ();
    return false;
  }
}

bool
ImGui_ImplDX11_CreateDeviceObjects (IDXGISwapChain*      pSwapChain,
                                    ID3D11Device*        pDevice,
                                    ID3D11DeviceContext* pDevCtx)
{
  ImGui_ImplDX11_InvalidateDeviceObjects ();

  if (! pSwapChain)
    return false;

  DXGI_SWAP_CHAIN_DESC  swapDesc = { };
  pSwapChain->GetDesc (&swapDesc);

  g_numFramesInSwapChain =
    _MAX_BACKBUFFERS;

  bool bSuccess = true;

  if (ImGui_ImplDX11_CreateFontsTexture (pSwapChain, pDevice))
  {
    for (UINT i = 0 ; i < g_numFramesInSwapChain ; ++i)
    {
      if (!ImGui_ImplDX11_CreateDeviceObjectsForBackbuffer (pSwapChain, pDevice, pDevCtx, i))
      {
        bSuccess = false;
        break;
      }
    }
  }

  else bSuccess = false;

  if (! bSuccess)
    ImGui_ImplDX11_InvalidateDeviceObjects ();

  return bSuccess;
}


using SK_ImGui_ResetCallback_pfn = void (__stdcall *)(void);

SK_LazyGlobal <std::vector <SK_ComPtr <IUnknown>>>       external_resources;
SK_LazyGlobal <std::set    <SK_ImGui_ResetCallback_pfn>> reset_callbacks;

SK_API
void
__stdcall
SKX_ImGui_RegisterResource (IUnknown* pRes)
{
  external_resources->push_back (pRes);
}


SK_API
void
__stdcall
SKX_ImGui_RegisterResetCallback (SK_ImGui_ResetCallback_pfn pCallback)
{
  reset_callbacks->emplace (pCallback);
}

SK_API
void
__stdcall
SKX_ImGui_UnregisterResetCallback (SK_ImGui_ResetCallback_pfn pCallback)
{
  if (reset_callbacks->count (pCallback))
      reset_callbacks->erase (pCallback);
}

void
SK_ImGui_ResetExternal (void)
{
  external_resources->clear ();

  for ( auto reset_fn : reset_callbacks.get () )
  {
    reset_fn ();
  }
}


void
ImGui_ImplDX11_InvalidateDeviceObjects (void)
{
  #define RESET_FONT
  #define RESET_VTX_IDX_BUFFERS
  #define RESET_SHADERS

  SK_ImGui_ResetExternal ();

  auto _CleanupBackbuffer = [&](UINT index) -> void
  {
    auto* _P = &_Frame [index];

#ifdef RESET_VTX_IDX_BUFFERS
    _P->VertexBufferSize = 5000;
    _P->IndexBufferSize  = 10000;
#endif

    auto _ReleaseAndCountRefs = [](auto** ppUnknown) -> ULONG
    {
      if (ppUnknown != nullptr)
      {
        if (*ppUnknown != nullptr)
        {
          ULONG refs =
          (*(IUnknown **)ppUnknown)->Release ();
           *(IUnknown **)ppUnknown = nullptr;

          return refs;
        }
      }

      return 0;
    };

#ifdef RESET_FONT
    if (_P->pFontSampler_clamp)      { _ReleaseAndCountRefs (&_P->pFontSampler_clamp);      assert (refs == 0); }
    if (_P->pFontSampler_wrap)       { _ReleaseAndCountRefs (&_P->pFontSampler_wrap);       assert (refs == 0); }
    if (_P->pFontTextureView)        { _ReleaseAndCountRefs (&_P->pFontTextureView);        assert (refs == 0);
    
      // Rarely, during application shutdown, ImGui may not be fully initialized.
      auto pFonts  = ImGui::GetIO ().Fonts;
      if ( pFonts != nullptr )
           pFonts->TexID = nullptr;
    }
#endif

#ifdef RESET_VTX_IDX_BUFFERS
    if (_P->pIB)                     { _ReleaseAndCountRefs (&_P->pIB);                     assert (refs == 0); }
    if (_P->pVB)                     { _ReleaseAndCountRefs (&_P->pVB);                     assert (refs == 0); }
#endif
    
#ifdef RESET_SHADERS
    if (_P->pBlendState)             { _ReleaseAndCountRefs (&_P->pBlendState);             assert (refs == 0); }
    if (_P->pDepthStencilState)      { _ReleaseAndCountRefs (&_P->pDepthStencilState);      assert (refs == 0); }
    if (_P->pRasterizerState)        { _ReleaseAndCountRefs (&_P->pRasterizerState);        assert (refs == 0); }
    if (_P->pPixelShader)            { _ReleaseAndCountRefs (&_P->pPixelShader);            assert (refs == 0); }
    if (_P->pPixelShaderuPlayHDR)    { _ReleaseAndCountRefs (&_P->pPixelShaderuPlayHDR);    assert (refs == 0); }
    if (_P->pPixelShaderSteamHDR)    { _ReleaseAndCountRefs (&_P->pPixelShaderSteamHDR);    assert (refs == 0); }
    if (_P->pPixelShaderDiscordHDR)  { _ReleaseAndCountRefs (&_P->pPixelShaderDiscordHDR);  assert (refs == 0); }
    if (_P->pVertexConstantBuffer)   { _ReleaseAndCountRefs (&_P->pVertexConstantBuffer);   assert (refs == 0); }
    if (_P->pPixelConstantBuffer)    { _ReleaseAndCountRefs (&_P->pPixelConstantBuffer);    assert (refs == 0); }
    if (_P->pInputLayout)            { _ReleaseAndCountRefs (&_P->pInputLayout);            assert (refs == 0); }
    if (_P->pVertexShader)           { _ReleaseAndCountRefs (&_P->pVertexShader);           assert (refs == 0); }
    if (_P->pVertexShaderSteamHDR)   { _ReleaseAndCountRefs (&_P->pVertexShaderSteamHDR);   assert (refs == 0); }
    if (_P->pVertexShaderDiscordHDR) { _ReleaseAndCountRefs (&_P->pVertexShaderDiscordHDR); assert (refs == 0); }
    if (_P->pVertexShaderRTSSHDR)    { _ReleaseAndCountRefs (&_P->pVertexShaderRTSSHDR);    assert (refs == 0); }
    if (_P->pVertexShaderuPlayHDR)   { _ReleaseAndCountRefs (&_P->pVertexShaderuPlayHDR);   assert (refs == 0); }
    if (_P->pVertexShaderEpicHDR)    { _ReleaseAndCountRefs (&_P->pVertexShaderEpicHDR);    assert (refs == 0); }
#endif

    if (_P->pRenderTargetView.p)
        _P->pRenderTargetView.Release ();

    if (_P->pRenderTargetView_sRGB.p)
        _P->pRenderTargetView_sRGB.Release ();

    if (_P->pUnorderedAccessView.p)
        _P->pUnorderedAccessView.Release ();

    if (_P->pBackBuffer.p)
        _P->pBackBuffer.Release ();
  };

  //for ( UINT i = 0 ; i < _MAX_BACKBUFFERS ; ++i )
  _CleanupBackbuffer (0);
}

bool
ImGui_ImplDX11_Init ( IDXGISwapChain*      pSwapChain,
                      ID3D11Device*        pDevice,
                      ID3D11DeviceContext* pDevCtx )
{
  static SK_Thread_HybridSpinlock                   _init_lock;
  std::scoped_lock <SK_Thread_HybridSpinlock> lock (_init_lock);

  ImGuiIO& io =
    ImGui::GetIO ();

  DXGI_SWAP_CHAIN_DESC      swap_desc = { };
  if (pSwapChain != nullptr)
  {   pSwapChain->GetDesc (&swap_desc); }

  g_numFramesInSwapChain =  _MAX_BACKBUFFERS;
//g_numFramesInSwapChain = swap_desc.BufferCount;
  g_frameBufferWidth     = swap_desc.BufferDesc.Width;
  g_frameBufferHeight    = swap_desc.BufferDesc.Height;
  g_hWnd                 = swap_desc.OutputWindow;

  //// FIXME
  ////ImGui_ImplDX11_Data* bd = IM_NEW(ImGui_ImplDX11_Data)();
  ////io.BackendRendererUserData = (void*)bd;
  io.BackendRendererName = "imgui_impl_dx11";
  io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.

  static auto& rb =
    SK_GetCurrentRenderBackend ();

  ImGui_ImplDX11_CreateDeviceObjects (pSwapChain, pDevice, pDevCtx);

  return
    rb.device != nullptr;
}

void
ImGui_ImplDX11_Shutdown (void)
{
  ImGuiIO& io =
    ImGui::GetIO ();

  ImGui_ImplDX11_InvalidateDeviceObjects ();
  ImGui::Shutdown                        ();

  io.BackendRendererName     = nullptr;
  io.BackendRendererUserData = nullptr;

  io.BackendFlags &= ~ImGuiBackendFlags_RendererHasVtxOffset;
}

#include <SpecialK/window.h>

void
ImGui_ImplDX11_NewFrame (void)
{
  auto& io =
    ImGui::GetIO ();

  static auto& rb =
    SK_GetCurrentRenderBackend ();

  auto pDev =
    rb.getDevice <ID3D11Device> ();

  if (! pDev.p)
    return;

  auto flag_result =
    SK_ImGui_FlagDrawing_OnD3D11Ctx (
      SK_D3D11_GetDeviceContextHandle (rb.d3d11.immediate_ctx)
    );

  SK_ScopedBool auto_bool0 (flag_result.first);
                           *flag_result.first = flag_result.second;

  auto *_Pool =
    &_Frame [g_frameIndex % g_numFramesInSwapChain];

  if (! _Pool->pFontSampler_clamp)
    return;

  //ImGui_ImplDX11_CreateDeviceObjects (pSwapChain, pDev, pDevCtx);

  if (io.Fonts->Fonts.empty ())
  {
    SK_ComQIPtr  <IDXGISwapChain>
      pSwapChain ( rb.swapchain );

    // This has nothing to do with swapchain, other than we want to abstract
    //   storage per-chain
    ImGui_ImplDX11_CreateFontsTexture (pSwapChain, pDev);

    if (io.Fonts->Fonts.empty ())
      return;
  }

  //// Start the frame
  SK_ImGui_User_NewFrame ();
}

void
ImGui_ImplDX11_Resize ( IDXGISwapChain *This,
                        UINT            BufferCount,
                        UINT            Width,
                        UINT            Height,
                        DXGI_FORMAT     NewFormat,
                        UINT            SwapChainFlags )
{
  UNREFERENCED_PARAMETER (BufferCount);
  UNREFERENCED_PARAMETER (NewFormat);
  UNREFERENCED_PARAMETER (SwapChainFlags);
  UNREFERENCED_PARAMETER (Width);
  UNREFERENCED_PARAMETER (Height);
  UNREFERENCED_PARAMETER (This);

  static auto& rb =
    SK_GetCurrentRenderBackend ();

  if (! rb.getDevice <ID3D11Device> ())
    return;

  SK_TLS *pTLS =
    SK_TLS_Bottom ();

  SK_ScopedBool auto_bool (&pTLS->imgui->drawing);

  // Do not dump ImGui font textures
  pTLS->imgui->drawing = true;

  assert (This == rb.swapchain);

  auto                                 pDev    =
    rb.getDevice <ID3D11Device>        (     );
  SK_ComPtr      <ID3D11DeviceContext> pDevCtx = nullptr;

  if (rb.d3d11.immediate_ctx != nullptr)
  {
    HRESULT hr1 = rb.d3d11.immediate_ctx->QueryInterface
          <ID3D11DeviceContext> (&pDevCtx.p);

    auto flag_result =
      SK_ImGui_FlagDrawing_OnD3D11Ctx (
        SK_D3D11_GetDeviceContextHandle (pDevCtx)
      );

    SK_ScopedBool auto_bool0 (flag_result.first);
                             *flag_result.first = flag_result.second;

    if (pDev.p != nullptr && SUCCEEDED (hr1))
    {
      ImGui_ImplDX11_InvalidateDeviceObjects ();
    }
  }
}


bool
SK_D3D11_RenderCtx::init (IDXGISwapChain*      pSwapChain,
                          ID3D11Device*        pDevice,
                          ID3D11DeviceContext* pDeviceCtx)
{
  try
  {
    if (! frames_.empty ())
    {
      // We're running on the wrong swapchain, shut this down and defer creation until the next frame
      if (! _pSwapChain.IsEqualObject (pSwapChain))
      {
        release (_pSwapChain.p);
      }

      else
      {
        SK_ReleaseAssert (_pSwapChain.IsEqualObject (pSwapChain));
        SK_ReleaseAssert (_pDevice.IsEqualObject    (pDevice));
        SK_ReleaseAssert (_pDeviceCtx.IsEqualObject (pDeviceCtx));
      }

      return true;
    }

    DXGI_SWAP_CHAIN_DESC  swapDesc = { };
    pSwapChain->GetDesc (&swapDesc);

    SK_LOG0 ( ( L"(+) Acquiring D3D11 Render Context: Device=%08xh, SwapChain: {%lu x %hs, HWND=%08xh}",
                pDevice,             swapDesc.BufferCount,
                SK_DXGI_FormatToStr (swapDesc.BufferDesc.Format).data (),
                                     swapDesc.OutputWindow ),
              L"D3D11BkEnd" );

    if ((! ImGui_ImplDX11_Init (pSwapChain, pDevice, pDeviceCtx)) ||
                _Frame [0].pBackBuffer.p          == nullptr  ||
                _Frame [0].pRenderTargetView.p    == nullptr/*||
                _Frame [0].pUnorderedAccessView.p == nullptr */)
    {
      throw (SK_ComException (E_FAIL));
    }

    else
    {
      SK_DXGI_UpdateSwapChain (pSwapChain);

      if (pGenericBlend.p == nullptr)
      {
        D3D11_BLEND_DESC
          blend                                        = {  };

        for ( UINT i = 0 ; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT ; ++i )
        {
          blend.RenderTarget [i].BlendEnable           = FALSE;
          blend.RenderTarget [i].SrcBlend              = D3D11_BLEND_ONE;
          blend.RenderTarget [i].DestBlend             = D3D11_BLEND_ZERO;
          blend.RenderTarget [i].BlendOp               = D3D11_BLEND_OP_ADD;
          blend.RenderTarget [i].SrcBlendAlpha         = D3D11_BLEND_ONE;
          blend.RenderTarget [i].DestBlendAlpha        = D3D11_BLEND_ZERO;
          blend.RenderTarget [i].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
          blend.RenderTarget [i].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_RED   |
                                                         D3D11_COLOR_WRITE_ENABLE_GREEN |
                                                         D3D11_COLOR_WRITE_ENABLE_BLUE;
        }

        ThrowIfFailed (pDevice->CreateBlendState (&blend, &pGenericBlend.p));
        SK_D3D11_SetDebugName (                            pGenericBlend.p,
                                              L"SK Generic Overlay Blend State");
      }

      frames_.resize (1);
      frames_ [0].hdr.pRTV      = _Frame [0].pRenderTargetView;
    //frames_ [0].hdr.pRTV_sRGB = _Frame [0].pRenderTargetView_sRGB;
      frames_ [0].hdr.pUAV      = _Frame [0].pUnorderedAccessView;
      frames_ [0].pRenderOutput = _Frame [0].pBackBuffer;

      _pSwapChain = pSwapChain;
      _pDevice    = pDevice;
      _pDeviceCtx = pDeviceCtx;

      _pReShadeRuntime =
        SK_ReShadeAddOn_CreateEffectRuntime_D3D11 (_pDevice, _pDeviceCtx, _pSwapChain);

      return true;
    }
  }

  catch (const SK_ComException& e)
  {
    SK_LOGi0 (
      L"D3D11 Render Context init: %hs", e.what ()
    );
  };

  frames_.clear                          ();
  ImGui_ImplDX11_InvalidateDeviceObjects ();

  return false;
}


void
__stdcall
SK_D3D11_ResetTexCache (void);

void
SK_D3D11_RenderCtx::release (IDXGISwapChain* pSwapChain)
{
  SK_ReShadeAddOn_CleanupRTVs (SK_ReShadeAddOn_GetRuntimeForSwapChain (pSwapChain), true);

  //SK_ComPtr <IDXGISwapChain> pSwapChain_ (pSwapChain);
  //
  //UINT _size =
  //      sizeof (LPVOID);
  //
  //SK_ComPtr <IUnknown> pUnwrapped = nullptr;
  //
  //if ( pSwapChain != nullptr &&
  //     SUCCEEDED (
  //       pSwapChain->GetPrivateData (
  //         IID_IUnwrappedDXGISwapChain, &_size,
  //            &pUnwrapped.p
  //               )                  )
  //   )
  //{
  //}

  if ( (_pSwapChain.p != nullptr && pSwapChain == nullptr) ||
        _pSwapChain.IsEqualObject  (pSwapChain)            )//||
        //_pSwapChain.IsEqualObject  (pUnwrapped) )
  {
    DXGI_SWAP_CHAIN_DESC swapDesc = { };

    if (_pSwapChain.p != nullptr)
        _pSwapChain->GetDesc (&swapDesc);

    SK_LOG0 ( ( L"(-) Releasing D3D11 Render Context: Device=%08xh, SwapChain: {%lu x %hs, HWND=%08xh}",
                                        _pDevice.p,
                                        swapDesc.BufferCount,
                   SK_DXGI_FormatToStr (swapDesc.BufferDesc.Format).data (),
                                        swapDesc.OutputWindow),
            L"D3D11BkEnd" );

    if (_pReShadeRuntime != nullptr)
    {
      SK_ReShadeAddOn_DestroyEffectRuntime (_pReShadeRuntime);
                                            _pReShadeRuntime = nullptr;
    }

#ifdef REFERENCE_COUNT_FIXUP
    // Release residual references that should have been released following SwapChain Present
    //
    extern bool
           bOriginallyFlip;
    if ((! bOriginallyFlip) && ( swapDesc.SwapEffect == DXGI_SWAP_EFFECT_FLIP_DISCARD ||
                                 swapDesc.SwapEffect == DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL ))
    {
      if (_pDeviceCtx.p != nullptr)
          _pDeviceCtx->OMSetRenderTargets (0, nullptr, nullptr);
    }
#endif

    SK_ReleaseAssert ( pSwapChain   == nullptr ||
                      _pSwapChain.p == nullptr ||
                      _pSwapChain.IsEqualObject (pSwapChain) );

    SK_D3D11_EndFrame     ();

    _pSwapChain.Release   ();
    _pDeviceCtx.Release   ();
    bool bDeviceRemoved =
      _pDevice.p != nullptr  &&
      _pDevice.p->AddRef  () <= 2;
  if (_pDevice.p != nullptr)
      _pDevice.p->Release ();
      _pDevice.Release    ();

    pGenericBlend.Release ();

    frames_.clear         ();

    void
    SK_HDR_ReleaseResources (void);
    SK_HDR_ReleaseResources ();

    void
    SK_DXGI_ReleaseSRGBLinearizer (void);
    SK_DXGI_ReleaseSRGBLinearizer ();

    ImGui_ImplDX11_InvalidateDeviceObjects ();

    if (bDeviceRemoved)
      SK_D3D11_ResetTexCache ();
  }
}

SK_D3D11_RenderCtx::FrameCtx::~FrameCtx (void)
{
  // LOTS OF STUFF TO DO, YIKES...

  pRenderOutput.Release      ();
  hdr.pRTV.Release           ();
  hdr.pSwapChainCopy.Release ();
}





#include <SpecialK/nvapi.h>

extern volatile LONG __SK_NVAPI_UpdateGSync;

//extern D3D11_PSSetSamplers_pfn D3D11_PSSetSamplers_Original;

void  SK_HDR_SnapshotSwapchain (void);
bool  ImGui_DX11Startup        (IDXGISwapChain* pSwapChain);
DWORD SK_ImGui_DrawFrame       (DWORD dwFlags, void* user);

void
SK_D3D11_RenderCtx::present (IDXGISwapChain* pSwapChain)
{
  SK_ReleaseAssert (  _d3d11_rbk->_pSwapChain.IsEqualObject (pSwapChain));
  SK_ReleaseAssert (! _d3d11_rbk->frames_.empty ());

  if (_d3d11_rbk->frames_.empty ())
    return;

  if (! _pDeviceCtx.p)
    return;

  static auto& rb =
    SK_GetCurrentRenderBackend ();

  if (! SK_D3D11_EnsureMatchingDevices (pSwapChain, _d3d11_rbk->_pDevice))
    return;

  SK_TLS *pTLS =
    SK_TLS_Bottom ();

  if (! pTLS)
    return;

  // This is about 22 KiB worth of device context state, it is not a good
  //   idea to allocate this on the stack... use SK's TLS storage.
  auto* state_block_storage =
    pTLS->render->d3d11->state_block.getPtr ();

  if (state_block_storage->empty ())
      state_block_storage->resize (sizeof (D3DX11_STATE_BLOCK));

  auto *sb =
    (D3DX11_STATE_BLOCK *)state_block_storage->data ();

  CreateStateblock (_pDeviceCtx, sb);

  auto _DrawAllReShadeEffects = [&](bool draw_first)
  {
    if ( config.reshade.is_addon   &&
         config.reshade.draw_first == draw_first )
    {
      SK_ComQIPtr <IDXGISwapChain1>         pSwapChain1 (_pSwapChain.p);
      SK_ReShadeAddOn_RenderEffectsD3D11Ex (pSwapChain1.p, _Frame [0].pRenderTargetView.p,
                                                           _Frame [0].pRenderTargetView_sRGB.p);
    }
  };

  // Draw ReShade before HDR image processing
  _DrawAllReShadeEffects (true);

  _pDeviceCtx->OMSetRenderTargetsAndUnorderedAccessViews (0, nullptr, nullptr, 0, 0, nullptr, nullptr);

  // Not necessarily having anything to do with HDR, this is a placeholder for now.
  _pDeviceCtx->OMSetRenderTargets (1, &_d3d11_rbk->frames_ [0].hdr.pRTV.p,    nullptr);
  _pDeviceCtx->OMSetBlendState    (    _d3d11_rbk->pGenericBlend, nullptr, 0xffffffff);

  D3D11_TEXTURE2D_DESC                             tex2d_desc = { };
  _d3d11_rbk->frames_ [0].pRenderOutput->GetDesc (&tex2d_desc);

  D3D11_VIEWPORT
    vp          = { };
    vp.Width    = static_cast <float> (tex2d_desc.Width);
    vp.Height   = static_cast <float> (tex2d_desc.Height);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;

  _pDeviceCtx->RSSetViewports (1, &vp);
  {
    bool hdr_mode =
     ( rb.isHDRCapable () &&
       rb.isHDRActive  () );

    if (rb.srgb_stripped && (! hdr_mode))
    {
//#define _USE_SRGB_WARNING
#ifdef _USE_SRGB_WARNING
      if (config.render.dxgi.srgb_behavior < -1)
      {
        SK_RunOnce (
          SK_ImGui_WarningWithTitle (
            L"The game's original SwapChain (sRGB) was incompatible with DXGI Flip Model"
            L"\r\n\r\n\t>> Please Confirm the Correct sRGB Bypass Mode (e.g. brightness) is active using Special K's Display Menu",
            L"sRGB Gamma Correction May Be Necessary"
          )
        );
      }
#endif

      if (config.render.dxgi.srgb_behavior > 0)
        SK_DXGI_LinearizeSRGB (_pSwapChain);
    }

    if (hdr_mode)
    {
      // Last-ditch effort to get the HDR post-process done before the UI.
      SK_HDR_SnapshotSwapchain ();
    }

    if (_pSwapChain == pSwapChain || ImGui_DX11Startup (_pSwapChain))
    {
      // Draw ReShade after HDR image processing, but before SK's UI
      _DrawAllReShadeEffects (false);

      // Queue-up Pre-SK OSD Screenshots
      SK_Screenshot_ProcessQueue (SK_ScreenshotStage::BeforeOSD, rb);
      
      SK_ImGui_DrawFrame (0x00, nullptr);
    }
  }

  // Queue-up Post-SK OSD Screenshots  (If done here, will not include ReShade)
  SK_Screenshot_ProcessQueue (SK_ScreenshotStage::PrePresent, rb);

  if (_pReShadeRuntime != nullptr)
    SK_ReShadeAddOn_UpdateAndPresentEffectRuntime (_pReShadeRuntime);
  else if (config.reshade.is_addon)
  {
    // Lazy initialize the runtime so that we can hot-inject ReShade
    _pReShadeRuntime =
      SK_ReShadeAddOn_CreateEffectRuntime_D3D11 (_pDevice, _pDeviceCtx, _pSwapChain);
  }

  ApplyStateblock (_pDeviceCtx, sb);

  ZeroMemory (sb, sizeof (D3DX11_STATE_BLOCK));

  //
  // Update G-Sync; doing this here prevents trying to do this on frames where
  //   the swapchain was resized, which would deadlock the software.
  //
  if (sk::NVAPI::nv_hardware && config.apis.NvAPI.gsync_status &&
           ((config.fps.show && config.osd.show)
                             || SK_ImGui_Visible))
  {
    static DWORD dwLastUpdate = 0;
           DWORD dwNow        = SK_timeGetTime ();

    if (dwLastUpdate < dwNow - 2000UL)
    {   dwLastUpdate = dwNow;
      InterlockedExchange (&__SK_NVAPI_UpdateGSync, TRUE);
    }
  }
}

SK_LazyGlobal <SK_D3D11_RenderCtx> _d3d11_rbk;