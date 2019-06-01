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
#include <SpecialK/render/d3d11/d3d11_core.h>

// DirectX
#include <d3d11.h>
#include <d3dcompiler.h>

bool __SK_ImGui_D3D11_DrawDeferred = false;
bool running_on_12                 = false;

extern void
SK_ImGui_User_NewFrame (void);

// Data
static INT64                    g_Time                  = 0;
static INT64                    g_TicksPerSecond        = 0;

static ImGuiMouseCursor         g_LastMouseCursor       = ImGuiMouseCursor_COUNT;
static bool                     g_HasGamepad            = false;
static bool                     g_WantUpdateHasGamepad  = true;

static HWND                     g_hWnd                  = nullptr;
static ID3D11Buffer*            g_pVB                   = nullptr;
static ID3D11Buffer*            g_pIB                   = nullptr;
static ID3D10Blob *             g_pVertexShaderBlob     = nullptr;
static ID3D10Blob *             g_pVertexShaderBlob2    = nullptr;
static ID3D10Blob *             g_pVertexShaderBlob3    = nullptr;
static ID3D11VertexShader*      g_pVertexShader         = nullptr;
static ID3D11VertexShader*      g_pVertexShaderSteamHDR = nullptr;
static ID3D11VertexShader*      g_pVertexShaderuPlayHDR = nullptr;
static ID3D11PixelShader*       g_pPixelShaderuPlayHDR  = nullptr;
static ID3D11PixelShader*       g_pPixelShaderSteamHDR  = nullptr;
static ID3D11InputLayout*       g_pInputLayout          = nullptr;
static ID3D11Buffer*            g_pVertexConstantBuffer = nullptr;
static ID3D11Buffer*            g_pPixelConstantBuffer  = nullptr;
static ID3D10Blob *             g_pPixelShaderBlob      = nullptr;
static ID3D10Blob *             g_pPixelShaderBlob2     = nullptr;
static ID3D10Blob *             g_pPixelShaderBlob3     = nullptr;
static ID3D11PixelShader*       g_pPixelShader          = nullptr;
static ID3D11SamplerState*      g_pFontSampler_clamp    = nullptr;
static ID3D11SamplerState*      g_pFontSampler_wrap     = nullptr;
static ID3D11ShaderResourceView*g_pFontTextureView      = nullptr;
static ID3D11RasterizerState*   g_pRasterizerState      = nullptr;
static ID3D11BlendState*        g_pBlendState           = nullptr;
static ID3D11DepthStencilState* g_pDepthStencilState    = nullptr;

static UINT                     g_frameBufferWidth      = 0UL;
static UINT                     g_frameBufferHeight     = 0UL;

static int                      g_VertexBufferSize      = 5000,
                                g_IndexBufferSize       = 10000;

std::pair <BOOL*, BOOL>
SK_ImGui_FlagDrawing_OnD3D11Ctx (UINT dev_idx);

static void
ImGui_ImplDX11_CreateFontsTexture (void);

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
  float steam_luminance [4];
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


extern void
SK_ImGui_LoadFonts (void);

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

  //if (! rb.d3d11.deferred_ctx)
  //  return;

  if (! g_pVertexConstantBuffer)
    return;

  if (! g_pPixelConstantBuffer)
    return;

  SK_ComQIPtr <IDXGISwapChain>      pSwapChain (rb.swapchain);
  SK_ComQIPtr <IDXGISwapChain3>     pSwap3     (pSwapChain);
  SK_ComQIPtr <ID3D11Device>        pDevice    (rb.device);
  SK_ComQIPtr <ID3D11DeviceContext> pDevCtx    (rb.d3d11.immediate_ctx);

  SK_ComPtr   <ID3D11Texture2D>     pBackBuffer = nullptr;

                    UINT currentBuffer = 0;
//if (pSwap3 != nullptr) currentBuffer = pSwap3->GetCurrentBackBufferIndex ();


  pSwapChain->GetBuffer (currentBuffer, IID_PPV_ARGS (&pBackBuffer));

  if (! pBackBuffer)
    return;

  SK_TLS* pTLS =
    SK_TLS_Bottom ();

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


  D3D11_TEXTURE2D_DESC   backbuffer_desc = { };
  pBackBuffer->GetDesc (&backbuffer_desc);

  io.DisplaySize.x             = static_cast <float> (backbuffer_desc.Width);
  io.DisplaySize.y             = static_cast <float> (backbuffer_desc.Height);

  io.DisplayFramebufferScale.x = static_cast <float> (backbuffer_desc.Width);
  io.DisplayFramebufferScale.y = static_cast <float> (backbuffer_desc.Height);

  // Create and grow vertex/index buffers if needed
  if ((! g_pVB) || g_VertexBufferSize < draw_data->TotalVtxCount)
  {
    SK_COM_ValidateRelease ((IUnknown **)&g_pVB);

    g_VertexBufferSize  =
      draw_data->TotalVtxCount + 5000;

    D3D11_BUFFER_DESC desc
                        = { };

    desc.Usage          = D3D11_USAGE_DYNAMIC;
    desc.ByteWidth      = g_VertexBufferSize * sizeof (ImDrawVert);
    desc.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags      = 0;

    if (pDevice->CreateBuffer (&desc, nullptr, &g_pVB) < 0 || (! g_pVB))
      return;
  }

  if ((! g_pIB) || g_IndexBufferSize < draw_data->TotalIdxCount)
  {
    SK_COM_ValidateRelease ((IUnknown **)&g_pIB);

    g_IndexBufferSize   =
      draw_data->TotalIdxCount + 10000;

    D3D11_BUFFER_DESC desc
                        = { };

    desc.Usage          = D3D11_USAGE_DYNAMIC;
    desc.ByteWidth      = g_IndexBufferSize * sizeof (ImDrawIdx);
    desc.BindFlags      = D3D11_BIND_INDEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    if (pDevice->CreateBuffer (&desc, nullptr, &g_pIB) < 0 || (! g_pIB))
      return;
  }

  // Copy and convert all vertices into a single contiguous buffer
  D3D11_MAPPED_SUBRESOURCE vtx_resource = { },
                           idx_resource = { };

  if (pDevCtx->Map (g_pVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &vtx_resource) != S_OK)
    return;

  if (pDevCtx->Map (g_pIB, 0, D3D11_MAP_WRITE_DISCARD, 0, &idx_resource) != S_OK)
  {
    // If for some reason the first one succeeded, but this one failed.... unmap the first one
    //   then abandon all hope.
    pDevCtx->Unmap (g_pVB, 0);
    return;
  }

  auto* vtx_dst = static_cast <ImDrawVert *> (vtx_resource.pData);
  auto* idx_dst = static_cast <ImDrawIdx  *> (idx_resource.pData);

  for (int n = 0; n < draw_data->CmdListsCount; n++)
  {
    const ImDrawList* cmd_list =
      draw_data->CmdLists [n];

    if (config.imgui.render.disable_alpha)
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
          (ImVec4)ImColor (color);
      }
    }

    memcpy (idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof (ImDrawIdx));
    memcpy (vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof (ImDrawVert));

    vtx_dst += cmd_list->VtxBuffer.Size;
    idx_dst += cmd_list->IdxBuffer.Size;
  }

  pDevCtx->Unmap (g_pIB, 0);
  pDevCtx->Unmap (g_pVB, 0);

  // Setup orthographic projection matrix into our constant buffer
  {
    float L = 0.0f;
    float R = io.DisplaySize.x;
    float B = io.DisplaySize.y;
    float T = 0.0f;

    alignas (__m128d) float mvp [4][4] =
    {
      { 2.0f/(R-L),   0.0f,           0.0f,       0.0f },
      { 0.0f,         2.0f/(T-B),     0.0f,       0.0f },
      { 0.0f,         0.0f,           0.5f,       0.0f },
      { (R+L)/(L-R),  (T+B)/(B-T),    0.5f,       1.0f },
    };

    D3D11_MAPPED_SUBRESOURCE mapped_resource;

    if (! g_pVertexConstantBuffer)
      return ImGui_ImplDX11_InvalidateDeviceObjects ();

    if (pDevCtx->Map (g_pVertexConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource) != S_OK)
      return;

    auto* constant_buffer =
      static_cast <VERTEX_CONSTANT_BUFFER *> (mapped_resource.pData);

    memcpy         (&constant_buffer->mvp, mvp, sizeof (mvp));

    bool hdr_display =
      (rb.isHDRCapable () && (rb.framebuffer_flags & SK_FRAMEBUFFER_FLAG_HDR));

    if (! hdr_display)
    {
      constant_buffer->luminance_scale [0] = 1.0f; constant_buffer->luminance_scale [1] = 1.0f;
      constant_buffer->luminance_scale [2] = 0.0f; constant_buffer->luminance_scale [3] = 0.0f;
      constant_buffer->steam_luminance [0] = 1.0f; constant_buffer->steam_luminance [1] = 1.0f;
      constant_buffer->steam_luminance [2] = 1.0f; constant_buffer->steam_luminance [3] = 1.0f;
    }

    else
    {
      extern float __SK_HDR_Luma;
      extern float __SK_HDR_Exp;

      float luma = 0.0f,
            exp  = 0.0f;

      luma = __SK_HDR_Luma;
      exp  = __SK_HDR_Exp;

      SK_RenderBackend::scan_out_s::SK_HDR_TRANSFER_FUNC eotf =
        rb.scanout.getEOTF ();

      bool bEOTF_is_PQ =
        (eotf == SK_RenderBackend::scan_out_s::SMPTE_2084);

      constant_buffer->luminance_scale [0] = ( bEOTF_is_PQ ? -80.0f * rb.ui_luminance :
                                                                      rb.ui_luminance );
      constant_buffer->luminance_scale [1] = 2.2f;
      constant_buffer->luminance_scale [2] = ( bEOTF_is_PQ ? 1.0f : luma );
      constant_buffer->luminance_scale [3] = ( bEOTF_is_PQ ? 1.0f : exp  );
      constant_buffer->steam_luminance [0] = ( bEOTF_is_PQ ? -80.0f * config.steam.overlay_hdr_luminance :
                                                                      config.steam.overlay_hdr_luminance );
      constant_buffer->steam_luminance [1] = 2.2f;//( bEOTF_is_PQ ? 1.0f : (rb.ui_srgb ? 2.2f :
                                                  //                                     1.0f));
      constant_buffer->steam_luminance [2] = ( bEOTF_is_PQ ? -80.0f * config.uplay.overlay_luminance :
                                                                      config.uplay.overlay_luminance );
      constant_buffer->steam_luminance [3] = 2.2f;//( bEOTF_is_PQ ? 1.0f : (rb.ui_srgb ? 2.2f :
                                                  //                1.0f));
    }

    pDevCtx->Unmap (g_pVertexConstantBuffer, 0);

    if (pDevCtx->Map (g_pPixelConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource) != S_OK)
      return;

    ((float *)mapped_resource.pData)[2] = hdr_display ? (float)backbuffer_desc.Width  : 0.0f;
    ((float *)mapped_resource.pData)[3] = hdr_display ? (float)backbuffer_desc.Height : 0.0f;

    pDevCtx->Unmap (g_pPixelConstantBuffer, 0);
  }


  SK_ComPtr <ID3D11RenderTargetView> pRenderTargetView = nullptr;

  D3D11_TEXTURE2D_DESC   tex2d_desc = { };
  pBackBuffer->GetDesc (&tex2d_desc);

  // SRGB Correction for UIs
  switch (tex2d_desc.Format)
  {
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
    {
      D3D11_RENDER_TARGET_VIEW_DESC rtdesc
                           = { };

      rtdesc.Format        = DXGI_FORMAT_R8G8B8A8_UNORM;
      rtdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

      pDevice->CreateRenderTargetView (pBackBuffer, &rtdesc, &pRenderTargetView);
    } break;

    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
    {
      D3D11_RENDER_TARGET_VIEW_DESC rtdesc
                           = { };

      rtdesc.Format        = DXGI_FORMAT_B8G8R8A8_UNORM;
      rtdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

      pDevice->CreateRenderTargetView (pBackBuffer, &rtdesc, &pRenderTargetView);
    } break;

    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    {
      D3D11_RENDER_TARGET_VIEW_DESC rtdesc
                           = { };

      rtdesc.Format        = DXGI_FORMAT_R16G16B16A16_FLOAT;
      rtdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

      pDevice->CreateRenderTargetView (pBackBuffer, &rtdesc, &pRenderTargetView);
    } break;

    case DXGI_FORMAT_R10G10B10A2_UNORM:
    {
      D3D11_RENDER_TARGET_VIEW_DESC rtdesc
        = { };

      rtdesc.Format        = DXGI_FORMAT_R10G10B10A2_UNORM;
      rtdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

      pDevice->CreateRenderTargetView (pBackBuffer, &rtdesc, &pRenderTargetView);
    } break;

    default:
     pDevice->CreateRenderTargetView (pBackBuffer, nullptr, &pRenderTargetView);
  }

  if (! pRenderTargetView)
    return;


  pDevCtx->OMSetRenderTargets ( 1,
                                  &pRenderTargetView.p,
                                    nullptr );

  // Setup viewport
  D3D11_VIEWPORT vp = { };

  vp.Height   = ImGui::GetIO ().DisplaySize.y;
  vp.Width    = ImGui::GetIO ().DisplaySize.x;
  vp.MinDepth = 0.0f;
  vp.MaxDepth = 1.0f;
  vp.TopLeftX = vp.TopLeftY = 0.0f;

  pDevCtx->RSSetViewports (1, &vp);

  // Bind shader and vertex buffers
  unsigned int stride = sizeof (ImDrawVert);
  unsigned int offset = 0;

  pDevCtx->IASetInputLayout       (g_pInputLayout);
  pDevCtx->IASetVertexBuffers     (0, 1, &g_pVB, &stride, &offset);
  pDevCtx->IASetIndexBuffer       ( g_pIB, sizeof (ImDrawIdx) == 2 ?
                                             DXGI_FORMAT_R16_UINT  :
                                             DXGI_FORMAT_R32_UINT,
                                               0 );
  pDevCtx->IASetPrimitiveTopology (D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  pDevCtx->GSSetShader            (nullptr, nullptr, 0);
  pDevCtx->HSSetShader            (nullptr, nullptr, 0);
  pDevCtx->DSSetShader            (nullptr, nullptr, 0);

  pDevCtx->VSSetShader            (g_pVertexShader, nullptr, 0);
  pDevCtx->VSSetConstantBuffers   (0, 1, &g_pVertexConstantBuffer);


  pDevCtx->PSSetShader            (g_pPixelShader, nullptr, 0);
  pDevCtx->PSSetSamplers          (0, 1, &g_pFontSampler_clamp);
  pDevCtx->PSSetConstantBuffers   (0, 1, &g_pPixelConstantBuffer);

  // Setup render state
  const float blend_factor [4] = { 0.f, 0.f,
                                   0.f, 1.f };

  pDevCtx->OMSetBlendState        (g_pBlendState, blend_factor, 0xffffffff);
  pDevCtx->OMSetDepthStencilState (g_pDepthStencilState,        0);
  pDevCtx->RSSetState             (g_pRasterizerState);

  // Render command lists
  int vtx_offset = 0;
  int idx_offset = 0;

  for (int n = 0; n < draw_data->CmdListsCount; n++)
  {
    const ImDrawList* cmd_list =
      draw_data->CmdLists [n];

    for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
    {
      const ImDrawCmd* pcmd =
        &cmd_list->CmdBuffer [cmd_i];

      if (pcmd->UserCallback)
        pcmd->UserCallback (cmd_list, pcmd);

      else
      {
        const D3D11_RECT r = {
          static_cast <LONG> (pcmd->ClipRect.x), static_cast <LONG> (pcmd->ClipRect.y),
          static_cast <LONG> (pcmd->ClipRect.z), static_cast <LONG> (pcmd->ClipRect.w)
        };

        extern ID3D11ShaderResourceView*
          SK_HDR_GetUnderlayResourceView (void);

        ID3D11ShaderResourceView* views [2] =
        {
           *(ID3D11ShaderResourceView **)&pcmd->TextureId,
           SK_HDR_GetUnderlayResourceView ()
        };

        pDevCtx->PSSetSamplers        (0, 1, &pTLS->d3d11->uiSampler_wrap);
        pDevCtx->PSSetShaderResources (0, 2, views);
        pDevCtx->RSSetScissorRects    (1, &r);

        pDevCtx->DrawIndexed (pcmd->ElemCount, idx_offset, vtx_offset);
      }

      idx_offset += pcmd->ElemCount;
    }

    // Last-ditch effort to get the HDR post-process done before the UI.
    void SK_HDR_SnapshotSwapchain (void);
         SK_HDR_SnapshotSwapchain (    );

    vtx_offset += cmd_list->VtxBuffer.Size;
  }

#if 0
  __SK_ImGui_D3D11_DrawDeferred = false;
  if (__SK_ImGui_D3D11_DrawDeferred)
  {
    SK_ComPtr <ID3D11CommandList> pCmdList = nullptr;

    if ( SUCCEEDED (
           pDevCtx->FinishCommandList ( TRUE,
                                          &pCmdList.p )
                   )
       )
    {
      rb.d3d11.immediate_ctx->ExecuteCommandList (
        pCmdList.p, TRUE
      );
    }

    else
      pDevCtx->Flush ();
  }
#endif
}

#include <SpecialK/config.h>

static void
ImGui_ImplDX11_CreateFontsTexture (void)
{
  SK_TLS* pTLS =
    SK_TLS_Bottom ();

  if (! pTLS) return;

  // Do not dump ImGui font textures
  SK_ScopedBool auto_bool (&pTLS->imgui->drawing);
                            pTLS->imgui->drawing = true;

  auto decl_tex_scope (
    SK_D3D11_DeclareTexInjectScope (pTLS)
  );

  // Build texture atlas
  ImGuiIO& io (
    ImGui::GetIO ()
  );

  static bool           init   = false;
  static unsigned char* pixels = nullptr;
  static int            width  = 0,
                        height = 0;

  // Only needs to be done once, the raw pixels are API agnostic
  if (! init)
  {
    SK_ImGui_LoadFonts ();

    io.Fonts->GetTexDataAsRGBA32 ( &pixels,
                                   &width, &height );
  }

  static auto& rb =
    SK_GetCurrentRenderBackend ();

  SK_ComQIPtr <ID3D11Device> pDev (rb.device);

  // Upload texture to graphics system
  {
    D3D11_TEXTURE2D_DESC
      desc                                = { };
      desc.Width                          = width;
      desc.Height                         = height;
      desc.MipLevels                      = 1;
      desc.ArraySize                      = 1;
      desc.Format                         = DXGI_FORMAT_R8G8B8A8_UNORM;
      desc.SampleDesc.Count               = 1;
      desc.Usage                          = D3D11_USAGE_IMMUTABLE;
      desc.BindFlags                      = D3D11_BIND_SHADER_RESOURCE;
      desc.CPUAccessFlags                 = 0;

    D3D11_SUBRESOURCE_DATA
      subResource                         = { };
      subResource.pSysMem                 = pixels;
      subResource.SysMemPitch             = desc.Width * 4;
      subResource.SysMemSlicePitch        = 0;

    SK_ComPtr <ID3D11Texture2D>
                      pFontTexture = nullptr;

    if ( SUCCEEDED (
           pDev->CreateTexture2D ( &desc,
                                     &subResource,
                                       &pFontTexture ) ) )
    {
      // Create texture view
      D3D11_SHADER_RESOURCE_VIEW_DESC
        srvDesc = { };
        srvDesc.Format                    = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels       = desc.MipLevels;
        srvDesc.Texture2D.MostDetailedMip = 0;

      if ( SUCCEEDED (
             pDev->CreateShaderResourceView ( pFontTexture,
                                                &srvDesc,
                                                  &g_pFontTextureView ) ) )
      {
        // Store our identifier
        io.Fonts->TexID =
          g_pFontTextureView;

      // Create texture sampler
        D3D11_SAMPLER_DESC
          sampler_desc                    = { };
          sampler_desc.Filter             = D3D11_FILTER_MIN_MAG_MIP_POINT;
          sampler_desc.AddressU           = D3D11_TEXTURE_ADDRESS_CLAMP;
          sampler_desc.AddressV           = D3D11_TEXTURE_ADDRESS_CLAMP;
          sampler_desc.AddressW           = D3D11_TEXTURE_ADDRESS_CLAMP;
          sampler_desc.MipLODBias         = 0.f;
          sampler_desc.ComparisonFunc     = D3D11_COMPARISON_ALWAYS;
          sampler_desc.MinLOD             = 0.f;
          sampler_desc.MaxLOD             = 0.f;

        if ( SUCCEEDED (
               pDev->CreateSamplerState ( &sampler_desc,
                                            &g_pFontSampler_clamp ) ) )
        { pTLS->d3d11->uiSampler_clamp =     g_pFontSampler_clamp;

          sampler_desc = { };

          sampler_desc.Filter             = D3D11_FILTER_ANISOTROPIC;
          sampler_desc.AddressU           = D3D11_TEXTURE_ADDRESS_MIRROR;
          sampler_desc.AddressV           = D3D11_TEXTURE_ADDRESS_MIRROR;
          sampler_desc.AddressW           = D3D11_TEXTURE_ADDRESS_MIRROR;
          sampler_desc.MipLODBias         = 0.f;
          sampler_desc.ComparisonFunc     = D3D11_COMPARISON_ALWAYS;
          sampler_desc.MinLOD             = 0.f;
          sampler_desc.MaxLOD             = 0.f;

          init = true;

          if ( SUCCEEDED (
                 pDev->CreateSamplerState ( &sampler_desc,
                                              &g_pFontSampler_wrap ) ) )
          { pTLS->d3d11->uiSampler_wrap =      g_pFontSampler_wrap; }
        }
      }
    }
  }
}

HRESULT
SK_D3D11_Inject_uPlayHDR ( _In_ ID3D11DeviceContext  *pDevCtx,
                           _In_ UINT                  IndexCount,
                           _In_ UINT                  StartIndexLocation,
                           _In_ INT                   BaseVertexLocation,
                           _In_ D3D11_DrawIndexed_pfn pfnD3D11DrawIndexed )
{
  if ( g_pVertexShaderSteamHDR != nullptr &&
       g_pVertexConstantBuffer != nullptr &&
       g_pPixelShaderSteamHDR  != nullptr    )
  {
    auto flag_result =
      SK_ImGui_FlagDrawing_OnD3D11Ctx (
        SK_D3D11_GetDeviceContextHandle (pDevCtx)
      );

    SK_ScopedBool auto_bool0 (flag_result.first);
                             *flag_result.first = flag_result.second;

    // Seriously, D3D, WTF? Let me first query the number of these and then decide
    //   whether I want to dedicate TLS or heap memory to this task.
    UINT NumStackDestroyingInstances_ProbablyZero0 = 0,
         NumStackDestroyingInstances_ProbablyZero1 = 0;

    ID3D11ClassInstance *GiantListThatDestroysTheStack0 [253] = { };
    ID3D11ClassInstance *GiantListThatDestroysTheStack1 [253] = { };

    SK_ComPtr <ID3D11PixelShader>  pOrigPixShader;
    SK_ComPtr <ID3D11VertexShader> pOrigVtxShader;
    SK_ComPtr <ID3D11Buffer>       pOrigVtxCB;

    pDevCtx->VSGetShader          ( &pOrigVtxShader.p,
                                     GiantListThatDestroysTheStack0,
                                   &NumStackDestroyingInstances_ProbablyZero0 );
    pDevCtx->PSGetShader          ( &pOrigPixShader.p,
                                   GiantListThatDestroysTheStack1,
                                   &NumStackDestroyingInstances_ProbablyZero1 );

    pDevCtx->VSGetConstantBuffers ( 0, 1, &pOrigVtxCB.p );
    pDevCtx->VSSetShader          ( g_pVertexShaderuPlayHDR,
                                      nullptr, 0 );
    pDevCtx->PSSetShader          ( g_pPixelShaderuPlayHDR,
                                      nullptr, 0 );
    pDevCtx->VSSetConstantBuffers ( 0, 1,
                                    &g_pVertexConstantBuffer );
    pfnD3D11DrawIndexed ( pDevCtx, IndexCount, StartIndexLocation, BaseVertexLocation );
    pDevCtx->VSSetConstantBuffers (0, 1, &pOrigVtxCB.p);

    pDevCtx->PSSetShader ( pOrigPixShader,
                           GiantListThatDestroysTheStack1,
                           NumStackDestroyingInstances_ProbablyZero1 );

    pDevCtx->VSSetShader ( pOrigVtxShader,
                           GiantListThatDestroysTheStack0,
                           NumStackDestroyingInstances_ProbablyZero0 );

    for ( UINT i = 0 ; i < NumStackDestroyingInstances_ProbablyZero0 ; ++i )
    {
      if (GiantListThatDestroysTheStack0 [i] != nullptr)
          GiantListThatDestroysTheStack0 [i]->Release ();
    }

    for ( UINT j = 0 ; j < NumStackDestroyingInstances_ProbablyZero1 ; ++j )
    {
      if (GiantListThatDestroysTheStack1 [j] != nullptr)
          GiantListThatDestroysTheStack1 [j]->Release ();
    }

    return
      S_OK;
  }

  return
    E_NOT_VALID_STATE;
}

HRESULT
SK_D3D11_InjectSteamHDR ( _In_ ID3D11DeviceContext *pDevCtx,
                          _In_ UINT                 VertexCount,
                          _In_ UINT                 StartVertexLocation,
                          _In_ D3D11_Draw_pfn       pfnD3D11Draw )
{
  auto flag_result =
    SK_ImGui_FlagDrawing_OnD3D11Ctx (
      SK_D3D11_GetDeviceContextHandle (pDevCtx)
    );

  SK_ScopedBool auto_bool0 (flag_result.first);
                           *flag_result.first = flag_result.second;

  if ( g_pVertexShaderSteamHDR != nullptr &&
       g_pVertexConstantBuffer != nullptr &&
       g_pPixelShaderSteamHDR  != nullptr    )
  {
    // Seriously, D3D, WTF? Let me first query the number of these and then decide
    //   whether I want to dedicate TLS or heap memory to this task.
    UINT NumStackDestroyingInstances_ProbablyZero0 = 0,
         NumStackDestroyingInstances_ProbablyZero1 = 0;

    ID3D11ClassInstance *GiantListThatDestroysTheStack0 [253] = { };
    ID3D11ClassInstance *GiantListThatDestroysTheStack1 [253] = { };

    SK_ComPtr <ID3D11PixelShader>  pOrigPixShader;
    SK_ComPtr <ID3D11VertexShader> pOrigVtxShader;
    SK_ComPtr <ID3D11Buffer>       pOrigVtxCB;

    pDevCtx->VSGetShader          ( &pOrigVtxShader.p,
                                     GiantListThatDestroysTheStack0,
                                   &NumStackDestroyingInstances_ProbablyZero0 );
    pDevCtx->PSGetShader          ( &pOrigPixShader.p,
                                   GiantListThatDestroysTheStack1,
                                   &NumStackDestroyingInstances_ProbablyZero1 );

    pDevCtx->VSGetConstantBuffers ( 0, 1, &pOrigVtxCB.p );
    pDevCtx->VSSetShader          ( g_pVertexShaderSteamHDR,
                                      nullptr, 0 );
    pDevCtx->PSSetShader          ( g_pPixelShaderSteamHDR,
                                      nullptr, 0 );
    pDevCtx->VSSetConstantBuffers ( 0, 1,
                                    &g_pVertexConstantBuffer );
    pfnD3D11Draw ( pDevCtx, VertexCount, StartVertexLocation );
    pDevCtx->VSSetConstantBuffers (0, 1, &pOrigVtxCB.p);

    pDevCtx->PSSetShader ( pOrigPixShader,
                           GiantListThatDestroysTheStack1,
                           NumStackDestroyingInstances_ProbablyZero1 );

    pDevCtx->VSSetShader ( pOrigVtxShader,
                           GiantListThatDestroysTheStack0,
                           NumStackDestroyingInstances_ProbablyZero0 );

    for ( UINT i = 0 ; i < NumStackDestroyingInstances_ProbablyZero0 ; ++i )
    {
      if (GiantListThatDestroysTheStack0 [i] != nullptr)
          GiantListThatDestroysTheStack0 [i]->Release ();
    }

    for ( UINT j = 0 ; j < NumStackDestroyingInstances_ProbablyZero1 ; ++j )
    {
      if (GiantListThatDestroysTheStack1 [j] != nullptr)
          GiantListThatDestroysTheStack1 [j]->Release ();
    }

    return
      S_OK;
  }

  return
    E_NOT_VALID_STATE;
}

bool
ImGui_ImplDX11_CreateDeviceObjects (void)
{
  SK_TLS* pTLS =
    SK_TLS_Bottom ();

  if (! pTLS) return false;

  SK_ScopedBool auto_bool (&pTLS->imgui->drawing);

  // Do not dump ImGui font textures
  pTLS->imgui->drawing = true;

  ImGui_ImplDX11_InvalidateDeviceObjects ();

  static auto& rb =
    SK_GetCurrentRenderBackend ();

  SK_ComQIPtr <ID3D11Device> pDev (rb.device);

  ///if (rb.api != SK_RenderAPI::D3D11On12)
  ///{
    if (! rb.device)
      return false;

    if (! rb.d3d11.immediate_ctx)
      return false;

    if (! rb.swapchain)
      return false;
  ///}

  ///if (rb.api == SK_RenderAPI::D3D11On12)
  ///{
  ///  rb.d3d11.wrapper_dev->QueryInterface <ID3D11Device> (&pDev.p);
  ///
  ///                              rb.d3d11.immediate_ctx = nullptr;
  ///  pDev->GetImmediateContext (&rb.d3d11.immediate_ctx.p);
  ///}

  // Create the vertex shader
  {
    static const char vertexShader [] =
     "#pragma warning ( disable : 3571 )\n                        \
      cbuffer vertexBuffer : register (b0)                        \
      {                                                           \
        float4x4 ProjectionMatrix;                                \
        float4   Luminance;                                       \
        float4   SteamLuminance;                                  \
      };                                                          \
                                                                  \
      struct VS_INPUT                                             \
      {                                                           \
        float2 pos : POSITION;                                    \
        float4 col : COLOR0;                                      \
        float2 uv  : TEXCOORD0;                                   \
      };                                                          \
                                                                  \
      struct PS_INPUT                                             \
      {                                                           \
        float4 pos : SV_POSITION;                                 \
        float4 col : COLOR0;                                      \
        float2 uv  : TEXCOORD0;                                   \
        float2 uv2 : TEXCOORD1;                                   \
        float2 uv3 : TEXCOORD2;                                   \
      };                                                          \
                                                                  \
      PS_INPUT main (VS_INPUT input)                              \
      {                                                           \
        PS_INPUT output;                                          \
                                                                  \
        output.pos  = mul ( ProjectionMatrix,                     \
                              float4 (input.pos.xy, 0.f, 1.f) );  \
                                                                  \
        output.uv  = saturate (input.uv);                         \
                                                                  \
        output.col = input.col;                                   \
        output.uv2 = float2 (0.f, 0.f);                           \
        output.uv3 = Luminance.xy;                                \
                                                                  \
        return output;                                            \
      }";

    SK_ComPtr <ID3D10Blob> blob_msg_vtx;

    D3DCompile ( vertexShader,
                   sizeof (vertexShader),
                     nullptr, nullptr, nullptr,
                       "main", "vs_4_0",
                         0, 0,
                           &g_pVertexShaderBlob,
                             &blob_msg_vtx );

    if (blob_msg_vtx != nullptr && blob_msg_vtx->GetBufferSize ())
    {
      std::string err;

      err.reserve (blob_msg_vtx->GetBufferSize ());
      err = (
           (char *)blob_msg_vtx->GetBufferPointer ());

      if (! err.empty ())
      {
        dll_log->LogEx (true, L"ImGui D3D11 Vertex Shader: %hs", err.c_str ());
      }
    }

    // NB: Pass ID3D10Blob* pErrorBlob to D3DCompile() to get error showing in
    //       (const char*)pErrorBlob->GetBufferPointer(). Make sure to Release() the blob!
    if (g_pVertexShaderBlob == nullptr)
      return false;

    if ( pDev->CreateVertexShader ( static_cast <DWORD *> (g_pVertexShaderBlob->GetBufferPointer ()),
                                      g_pVertexShaderBlob->GetBufferSize (),
                                        nullptr,
                                          &g_pVertexShader ) != S_OK )
      return false;


    static const char vertexShaderSteamHDR [] =
   "#pragma warning ( disable : 3571 )\n                    \
    cbuffer vertexBuffer : register (b0)                    \
    {                                                       \
      float4x4 ProjectionMatrix;                            \
      float4   Luminance;                                   \
      float4   SteamLuminance;                              \
    };                                                      \
                                                            \
    struct VS_INPUT                                         \
    {                                                       \
      float4 pos : POSITION;                                \
      float4 col : COLOR;                                   \
      float2 uv  : TEXCOORD;                                \
    };                                                      \
                                                            \
    struct PS_INPUT                                         \
    {                                                       \
      float4 pos : SV_POSITION;                             \
      float4 col : COLOR;                                   \
      float2 uv  : TEXCOORD0;                               \
      float2 uv2 : TEXCOORD1;                               \
    };                                                      \
                                                            \
    PS_INPUT main (VS_INPUT input)                          \
    {                                                       \
      PS_INPUT output;                                      \
                                                            \
      output.pos = input.pos;                               \
      output.col = input.col;                               \
      output.uv  = input.uv;                                \
      output.uv2 = SteamLuminance.xy;                       \
                                                            \
      return output;                                        \
    }";

   static const char vertexShaderuPlayHDR [] =
   "#pragma warning ( disable : 3571 )\n                    \
    cbuffer vertexBuffer : register (b0)                    \
    {                                                       \
      float4x4 ProjectionMatrix;                            \
      float4   Luminance;                                   \
      float4   uPlayLuminance;                              \
    };                                                      \
                                                            \
    struct VS_INPUT                                         \
    {                                                       \
      float4 pos : POSITION;                                \
      float2 uv  : TEXCOORD;                                \
    };                                                      \
                                                            \
    struct PS_INPUT                                         \
    {                                                       \
      float4 pos : SV_POSITION;                             \
      float2 uv  : TEXCOORD0;                               \
      float2 uv2 : TEXCOORD1;                               \
    };                                                      \
                                                            \
    PS_INPUT main (VS_INPUT input)                          \
    {                                                       \
      PS_INPUT output;                                      \
                                                            \
      output.pos = input.pos;                               \
      output.uv  = input.uv;                                \
      output.uv2 = uPlayLuminance.zw;                       \
                                                            \
      return output;                                        \
    }";

    D3DCompile ( vertexShaderSteamHDR,
                   sizeof (vertexShaderSteamHDR),
                     nullptr, nullptr, nullptr,
                       "main", "vs_4_0",
                         0, 0,
                           &g_pVertexShaderBlob2,
                             &blob_msg_vtx );

    if (blob_msg_vtx != nullptr && blob_msg_vtx->GetBufferSize ())
    {
      std::string err;

      err.reserve (blob_msg_vtx->GetBufferSize ());
      err = (
           (char *)blob_msg_vtx->GetBufferPointer ());

      if (! err.empty ())
      {
        dll_log->LogEx (true, L"Steam HDR D3D11 Vertex Shader: %hs", err.c_str ());
      }
    }

    // NB: Pass ID3D10Blob* pErrorBlob to D3DCompile() to get error showing in
    //       (const char*)pErrorBlob->GetBufferPointer(). Make sure to Release() the blob!
    if (g_pVertexShaderBlob2 == nullptr)
      return false;

    if ( pDev->CreateVertexShader ( static_cast <DWORD *> (g_pVertexShaderBlob2->GetBufferPointer ()),
                                      g_pVertexShaderBlob2->GetBufferSize (),
                                        nullptr,
                                          &g_pVertexShaderSteamHDR ) != S_OK )
      return false;

    D3DCompile ( vertexShaderuPlayHDR,
                   sizeof (vertexShaderuPlayHDR),
                     nullptr, nullptr, nullptr,
                       "main", "vs_4_0",
                         0, 0,
                           &g_pVertexShaderBlob3,
                             &blob_msg_vtx );

    if (blob_msg_vtx != nullptr && blob_msg_vtx->GetBufferSize ())
    {
      std::string err;

      err.reserve (blob_msg_vtx->GetBufferSize ());
      err = (
           (char *)blob_msg_vtx->GetBufferPointer ());

      if (! err.empty ())
      {
        dll_log->LogEx (true, L"uPlay HDR D3D11 Vertex Shader: %hs", err.c_str ());
      }
    }

    if (g_pVertexShaderBlob3 == nullptr)
      return false;

    if ( pDev->CreateVertexShader ( static_cast <DWORD *> (g_pVertexShaderBlob3->GetBufferPointer ()),
                                      g_pVertexShaderBlob3->GetBufferSize (),
                                        nullptr,
                                          &g_pVertexShaderuPlayHDR ) != S_OK )
      return false;


    // Create the input layout
    D3D11_INPUT_ELEMENT_DESC local_layout [] = {
      { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,       0, (size_t)(&((ImDrawVert *)nullptr)->pos), D3D11_INPUT_PER_VERTEX_DATA, 0 },
      { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, (size_t)(&((ImDrawVert *)nullptr)->uv),  D3D11_INPUT_PER_VERTEX_DATA, 0 },
      { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, (size_t)(&((ImDrawVert *)nullptr)->col), D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    if ( (! g_pVertexShaderBlob) ||
          pDev->CreateInputLayout ( local_layout, 3,
                                    g_pVertexShaderBlob->GetBufferPointer (),
                                      g_pVertexShaderBlob->GetBufferSize  (),
                                        &g_pInputLayout ) != S_OK )
    {
      return false;
    }

    // Create the constant buffers
    {
      D3D11_BUFFER_DESC desc = { };

      desc.ByteWidth         = sizeof (VERTEX_CONSTANT_BUFFER);
      desc.Usage             = D3D11_USAGE_DYNAMIC;
      desc.BindFlags         = D3D11_BIND_CONSTANT_BUFFER;
      desc.CPUAccessFlags    = D3D11_CPU_ACCESS_WRITE;
      desc.MiscFlags         = 0;

      pDev->CreateBuffer (&desc, nullptr, &g_pVertexConstantBuffer);

      desc.ByteWidth         = sizeof (float) * 4;
      desc.Usage             = D3D11_USAGE_DYNAMIC;
      desc.BindFlags         = D3D11_BIND_CONSTANT_BUFFER;
      desc.CPUAccessFlags    = D3D11_CPU_ACCESS_WRITE;
      desc.MiscFlags         = 0;

      pDev->CreateBuffer (&desc, nullptr, &g_pPixelConstantBuffer);
    }
  }

  // Create the pixel shader
  {
    static const char pixelShader[] =
      "#pragma warning ( disable : 3571 )\n                                      \
      struct PS_INPUT                                                         \n\
      {                                                                       \n\
        float4 pos : SV_POSITION;                                             \n\
        float4 col : COLOR0;                                                  \n\
        float2 uv  : TEXCOORD0;                                               \n\
        float2 uv2 : TEXCOORD1;                                               \n\
        float2 uv3 : TEXCOORD2;                                               \n\
      };                                                                      \n\
                                                                              \n\
      cbuffer viewportDims : register (b0)                                    \n\
      {                                                                       \n\
        float4 viewport;                                                      \n\
      };                                                                      \n\
                                                                              \n\
      sampler   sampler0    : register (s0);                                  \n\
                                                                              \n\
      Texture2D texture0    : register (t0);                                  \n\
      Texture2D hdrUnderlay : register (t1);                                  \n\
      Texture2D hdrHUD      : register (t2);                                  \n\
                                                                              \n\
      float3 RemoveSRGBCurve (float3 x)                                       \n\
      {                                                                       \n\
        /* Approximately pow(x, 2.2)*/                                        \n\
        return x < 0.04045 ? x / 12.92 : pow((x + 0.055) / 1.055, 2.4);       \n\
      }                                                                       \n\
                                                                              \n\
      float3 ApplyREC709Curve (float3 x)                                      \n\
      {                                                                       \n\
        return x < 0.0181 ? 4.5 * x : 1.0993 * pow(x, 0.45) - 0.0993;         \n\
      }                                                                       \n\
      float Luma (float3 color)                                               \n\
      {                                                                       \n\
        return                                                                \n\
          dot (color, float3 (0.299f, 0.587f, 0.114f));                       \n\
      }                                                                       \n\
                                                                              \n\
      float3 ApplyREC2084Curve (float3 L, float maxLuminance)                 \n\
      {                                                                       \n\
        float m1 = 2610.0 / 4096.0 / 4;                                       \n\
        float m2 = 2523.0 / 4096.0 * 128;                                     \n\
        float c1 = 3424.0 / 4096.0;                                           \n\
        float c2 = 2413.0 / 4096.0 * 32;                                      \n\
        float c3 = 2392.0 / 4096.0 * 32;                                      \n\
                                                                              \n\
        float maxLuminanceScale = maxLuminance / 10000.0f;                    \n\
        L *= maxLuminanceScale;                                               \n\
                                                                              \n\
        float3 Lp = pow (L, m1);                                              \n\
                                                                              \n\
        return pow ((c1 + c2 * Lp) / (1 + c3 * Lp), m2);                      \n\
      }                                                                       \n\
      float3 RemoveREC2084Curve (float3 N)                                    \n\
      {                                                                       \n\
        float  m1 = 2610.0 / 4096.0 / 4;                                      \n\
        float  m2 = 2523.0 / 4096.0 * 128;                                    \n\
        float  c1 = 3424.0 / 4096.0;                                          \n\
        float  c2 = 2413.0 / 4096.0 * 32;                                     \n\
        float  c3 = 2392.0 / 4096.0 * 32;                                     \n\
        float3 Np = pow (N, 1 / m2);                                          \n\
                                                                              \n\
        return                                                                \n\
          pow (max (Np - c1, 0) / (c2 - c3 * Np), 1 / m1);                    \n\
      }                                                                       \n\
      float3 REC709toREC2020 (float3 RGB709)                                  \n\
      {                                                                       \n\
        static const float3x3 ConvMat =                                       \n\
        {                                                                     \n\
          0.627402, 0.329292, 0.043306,                                       \n\
          0.069095, 0.919544, 0.011360,                                       \n\
          0.016394, 0.088028, 0.895578                                        \n\
        };                                                                    \n\
        return mul (ConvMat, RGB709);                                         \n\
      }                                                                       \n\
                                                                              \n\
      float3 REC2020toREC709 (float3 RGB2020)                                 \n\
      {                                                                       \n\
        static const float3x3 ConvMat =                                       \n\
        {                                                                     \n\
           1.660496, -0.587656, -0.072840,                                    \n\
          -0.124546,  1.132895,  0.008348,                                    \n\
          -0.018154, -0.100597,  1.118751                                     \n\
        };                                                                    \n\
        return mul (ConvMat, RGB2020);                                        \n\
      }                                                                       \n\
                                                                              \n\
      float4 main (PS_INPUT input) : SV_Target                                \n\
      {                                                                       \n\
        float4 out_col =                                                      \n\
          texture0.Sample (sampler0, input.uv);                               \n\
                                                                              \n\
        bool hdr10 = ( input.uv3.x < 0.0 );                                   \n\
                                                                              \n\
        if (viewport.z > 0.f)                                                 \n\
        {                                                                     \n\
          float4 under_color;                                                 \n\
                                                                              \n\
          float blend_alpha =                                                 \n\
            saturate (input.col.a * out_col.a);                               \n\
                                                                              \n\
          if (abs (blend_alpha) < 0.001f) blend_alpha = 0.0f;                 \n\
          if (abs (blend_alpha) > 0.999f) blend_alpha = 1.0f;                 \n\
                                                                              \n\
          float4 hud = float4 (0.0f, 0.0f, 0.0f, 0.0f);                       \n\
                                                                              \n\
          if (input.uv2.x > 0.0f && input.uv2.y > 0.0f)                       \n\
          {                                                                   \n\
            hud = hdrHUD.Sample (sampler0, input.uv);                         \n\
            hud.rbg     = RemoveSRGBCurve (hud.rgb);                          \n\
            hud.rgb    *= ( input.uv3.xxx );                                  \n\
            out_col.rgb = RemoveSRGBCurve (out_col.rgb);                      \n\
            out_col =                                                         \n\
              pow (abs (out_col), float4 (input.uv2.yyy, 1.0f)) *             \n\
                input.uv2.xxxx;                                               \n\
            out_col.a   = 1.0f;                                               \n\
            blend_alpha = 1.0f;                                               \n\
            under_color = float4 (0.0f, 0.0f, 0.0f, 0.0f);                    \n\
          }                                                                   \n\
                                                                              \n\
          else                                                                \n\
          {                                                                   \n\
            under_color =                                                     \n\
              float4 ( hdrUnderlay.Sample ( sampler0, input.pos.xy /          \n\
                                                      viewport.zw ).rgb, 1.0);\n\
            out_col =                                                         \n\
              pow (abs (input.col * out_col), float4 (input.uv3.yyy, 1.0f));  \n\
                                                                              \n\
            if (hdr10)                                                        \n\
            {                                                                 \n\
              under_color.rgb =                                               \n\
                REC2020toREC709 (                                             \n\
                  ( RemoveREC2084Curve (12.5f * under_color.rgb) )            \n\
                );                                                            \n\
                                                                              \n\
              blend_alpha =                                                   \n\
                Luma (                                                        \n\
                  ApplyREC2084Curve (                                         \n\
                    float3 ( blend_alpha, blend_alpha, blend_alpha ),         \n\
                                                      -input.uv3.x )          \n\
                );                                                            \n\
            }                                                                 \n\
                                                                              \n\
            if (! hdr10)                                                      \n\
            {                                                                 \n\
              blend_alpha =                                                   \n\
                Luma (                                                        \n\
                  ApplyREC709Curve (                                          \n\
                    float3 ( blend_alpha, blend_alpha, blend_alpha )          \n\
                                   )                                          \n\
                );                                                            \n\
              under_color.rgb =                                               \n\
                  RemoveSRGBCurve (under_color.rgb);                          \n\
              under_color.rgb /= (1.0f + under_color.rgb);                    \n\
              out_col.rgb *= input.uv3.x;                                     \n\
            }                                                                 \n\
                                                                              \n\
            out_col.rgb *= blend_alpha;                                       \n\
          }                                                                   \n\
                                                                              \n\
          float4 final =                                                      \n\
            float4 (                         out_col.rgb +                    \n\
                  (1.0f - blend_alpha) * under_color.rgb,                     \n\
                saturate (blend_alpha));                                      \n\
                                                                              \n\
          if (hdr10)                                                          \n\
          {                                                                   \n\
            final.rgb =                                                       \n\
              ApplyREC2084Curve ( REC709toREC2020 (final.rgb),                \n\
                                    -input.uv3.x );                           \n\
          }                                                                   \n\
                                                                              \n\
          return final;                                                       \n\
        }                                                                     \n\
                                                                              \n\
        return                                                                \n\
          ( input.col * out_col );                                            \n\
      }";

    SK_ComPtr <ID3D10Blob> blob_msg_pix;

    D3DCompile ( pixelShader,
                   sizeof (pixelShader),
                     nullptr, nullptr, nullptr,
                       "main", "ps_4_0",
                         0, 0,
                           &g_pPixelShaderBlob,
                             &blob_msg_pix );

    if (blob_msg_pix != nullptr && blob_msg_pix->GetBufferSize ())
    {
      std::string err;

      err.reserve (blob_msg_pix->GetBufferSize    ());
      err = (
           (char *)blob_msg_pix->GetBufferPointer ()
      );

      if (! err.empty ())
      {
        dll_log->LogEx (true, L"ImGui D3D11 Pixel Shader:  %hs", err.c_str ());
      }
    }

    // NB: Pass ID3D10Blob* pErrorBlob to D3DCompile() to get error showing in
    //       (const char*)pErrorBlob->GetBufferPointer(). Make sure to Release() the blob!
    if (g_pPixelShaderBlob == nullptr)
      return false;

    if ( pDev->CreatePixelShader ( static_cast <DWORD *> (g_pPixelShaderBlob->GetBufferPointer ()),
                                     g_pPixelShaderBlob->GetBufferSize (),
                                       nullptr,
                                         &g_pPixelShader ) != S_OK )
    {
      return false;
    }



    ///////
    static const char pixelShaderSteamHDR[] =
   "#pragma warning ( disable : 3571 )\n                         \
    struct PS_INPUT                                              \n\
    {                                                            \n\
      float4 pos : SV_POSITION;                                  \n\
      float4 col : COLOR;                                        \n\
      float2 uv  : TEXCOORD0;                                    \n\
      float2 uv2 : TEXCOORD1;                                    \n\
    };                                                           \n\
                                                                 \n\
    sampler   PS_QUAD_Sampler   : register (s0);                 \n\
    Texture2D PS_QUAD_Texture2D : register (t0);                 \n\
                                                                 \n\
    float3 RemoveSRGBCurve(float3 x)                             \n\
    {                                                            \n\
      /* Approximately pow(x, 2.2)*/                             \n\
      return x < 0.04045 ? x / 12.92 :                           \n\
                      pow((x + 0.055) / 1.055, 2.4);             \n\
    }                                                            \n\
                                                                 \n\
    float3 ApplyREC2084Curve (float3 L, float maxLuminance)      \n\
    {                                                            \n\
      float m1 = 2610.0 / 4096.0 / 4;                            \n\
      float m2 = 2523.0 / 4096.0 * 128;                          \n\
      float c1 = 3424.0 / 4096.0;                                \n\
      float c2 = 2413.0 / 4096.0 * 32;                           \n\
      float c3 = 2392.0 / 4096.0 * 32;                           \n\
                                                                 \n\
      float maxLuminanceScale = maxLuminance / 10000.0f;         \n\
      L *= maxLuminanceScale;                                    \n\
                                                                 \n\
      float3 Lp = pow (L, m1);                                   \n\
                                                                 \n\
      return pow ((c1 + c2 * Lp) / (1 + c3 * Lp), m2);           \n\
    }                                                            \n\
                                                                 \n\
    float3 REC709toREC2020 (float3 RGB709)                       \n\
    {                                                            \n\
      static const float3x3 ConvMat =                            \n\
      {                                                          \n\
        0.627402, 0.329292, 0.043306,                            \n\
        0.069095, 0.919544, 0.011360,                            \n\
        0.016394, 0.088028, 0.895578                             \n\
      };                                                         \n\
      return mul (ConvMat, RGB709);                              \n\
    }                                                            \n\
                                                                 \n\
    float4 main (PS_INPUT input) : SV_Target                     \n\
    {                                                            \n\
      float4 gamma_exp  = float4 (input.uv2.yyy, 1.f);           \n\
      float4 linear_mul = float4 (input.uv2.xxx, 1.f);           \n\
                                                                 \n\
      float4 out_col =                                           \n\
        PS_QUAD_Texture2D.Sample (PS_QUAD_Sampler, input.uv);    \n\
                                                                 \n\
      out_col =                                                  \n\
        float4 (RemoveSRGBCurve (input.col.rgb * out_col.rgb),   \n\
                                 input.col.a   * out_col.a);     \n\
                                                                 \n\
      // Negative = HDR10                                        \n\
      if (linear_mul.x < 0.0)                                    \n\
      {                                                          \n\
        out_col.rgb =                                            \n\
          ApplyREC2084Curve ( REC709toREC2020 (out_col.rgb),     \n\
                                -linear_mul.x );                 \n\
                                                                 \n\
      }                                                          \n\
                                                                 \n\
      // Positive = scRGB                                        \n\
      else                                                       \n\
        out_col *= linear_mul;                                   \n\
                                                                 \n\
      return                                                     \n\
        float4 (out_col.rgb, saturate (out_col.a));              \n\
    }";

#if 0
    "
    float4 main (PS_INPUT input) : SV_Target                     \
    {                                                            \
      float4 gamma_exp  = float4 (input.uv2.yyy, 1.f);           \
      float4 linear_mul = float4 (input.uv2.xxx, 1.f);           \
                                                                 \
      float4 out_col =                                           \
        PS_QUAD_Texture2D.Sample (PS_QUAD_Sampler, input.uv);    \
                                                                 \
      out_col =                                                  \
        float4 (RemoveSRGBCurve (input.col.rgb * out_col.rgb),   \
                                 input.col.a   * out_col.a)  *   \
                                                     linear_mul; \
                                                                 \
      return                                                     \
        float4 (out_col.rgb, saturate (out_col.a));              \
      "
#endif


   static const char pixelShaderuPlayHDR [] =
   "#pragma warning ( disable : 3571 )\n                         \
    struct PS_INPUT                                              \
    {                                                            \
      float4 pos : SV_POSITION;                                  \
      float2 uv  : TEXCOORD0;                                    \
      float2 uv2 : TEXCOORD1;                                    \
    };                                                           \
                                                                 \
    sampler   PS_QUAD_Sampler   : register (s0);                 \
    Texture2D PS_QUAD_Texture2D : register (t0);                 \
                                                                 \
    float3 RemoveSRGBCurve(float3 x)                             \
    {                                                            \
      /* Approximately pow(x, 2.2)*/                             \
      return x < 0.04045 ? x / 12.92 :                           \
                      pow((x + 0.055) / 1.055, 2.4);             \
    }                                                            \
                                                                 \
    float4 main (PS_INPUT input) : SV_Target                     \
    {                                                            \
      float4 gamma_exp  = float4 (input.uv2.yyy, 1.f);           \
      float4 linear_mul = float4 (input.uv2.xxx, 1.f);           \
                                                                 \
      float4 out_col =                                           \
        PS_QUAD_Texture2D.Sample (PS_QUAD_Sampler, input.uv);    \
                                                                 \
      out_col =                                                  \
        float4 (RemoveSRGBCurve (out_col.rgb),out_col.a)  *      \
                                                     linear_mul; \
                                                                 \
      return                                                     \
        float4 (out_col.rgb, saturate (out_col.a));              \
    }";

    D3DCompile ( pixelShaderSteamHDR,
                   sizeof (pixelShaderSteamHDR),
                     nullptr, nullptr, nullptr,
                       "main", "ps_4_0",
                         0, 0,
                           &g_pPixelShaderBlob2,
                             &blob_msg_pix );

    if (blob_msg_pix != nullptr && blob_msg_pix->GetBufferSize ())
    {
      std::string err;

      err.reserve (blob_msg_pix->GetBufferSize ());
      err = (
           (char *)blob_msg_pix->GetBufferPointer ());

      if (! err.empty ())
      {
        dll_log->LogEx (true, L"Steam HDR D3D11 Pixel Shader: %hs", err.c_str ());
      }
    }

    if (g_pPixelShaderBlob2 == nullptr)
      return false;

    if ( pDev->CreatePixelShader ( static_cast <DWORD *> (g_pPixelShaderBlob2->GetBufferPointer ()),
                                     g_pPixelShaderBlob2->GetBufferSize (),
                                       nullptr,
                                         &g_pPixelShaderSteamHDR ) != S_OK )
      return false;

    D3DCompile ( pixelShaderuPlayHDR,
                   sizeof (pixelShaderuPlayHDR),
                     nullptr, nullptr, nullptr,
                       "main", "ps_4_0",
                         0, 0,
                           &g_pPixelShaderBlob3,
                             &blob_msg_pix );

    if (blob_msg_pix != nullptr && blob_msg_pix->GetBufferSize ())
    {
      std::string err;

      err.reserve (blob_msg_pix->GetBufferSize ());
      err = (
           (char *)blob_msg_pix->GetBufferPointer ());

      if (! err.empty ())
      {
        dll_log->LogEx (true, L"uPlay HDR D3D11 Pixel Shader: %hs", err.c_str ());
      }
    }

    if (g_pPixelShaderBlob3 == nullptr)
      return false;

    if ( pDev->CreatePixelShader ( static_cast <DWORD *> (g_pPixelShaderBlob3->GetBufferPointer ()),
                                     g_pPixelShaderBlob3->GetBufferSize (),
                                       nullptr,
                                         &g_pPixelShaderuPlayHDR ) != S_OK )
      return false;
  }

  // Create the blending setup
  {
    D3D11_BLEND_DESC desc                       = {   };

    desc.AlphaToCoverageEnable                  = false;
    desc.RenderTarget [0].BlendEnable           =  true;
    desc.RenderTarget [0].SrcBlend              = D3D11_BLEND_SRC_ALPHA;
    desc.RenderTarget [0].DestBlend             = D3D11_BLEND_INV_SRC_ALPHA;
    desc.RenderTarget [0].BlendOp               = D3D11_BLEND_OP_ADD;
    desc.RenderTarget [0].SrcBlendAlpha         = D3D11_BLEND_ONE;
    desc.RenderTarget [0].DestBlendAlpha        = D3D11_BLEND_ZERO;
    desc.RenderTarget [0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
    desc.RenderTarget [0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    pDev->CreateBlendState (&desc, &g_pBlendState);

    //desc.RenderTarget [0].BlendEnable           = true;
    //desc.RenderTarget [0].SrcBlend              = D3D11_BLEND_ONE;
    //desc.RenderTarget [0].DestBlend             = D3D11_BLEND_ONE;
    //desc.RenderTarget [0].BlendOp               = D3D11_BLEND_OP_ADD;
    //desc.RenderTarget [0].SrcBlendAlpha         = D3D11_BLEND_ONE;
    //desc.RenderTarget [0].DestBlendAlpha        = D3D11_BLEND_ONE;
    //desc.RenderTarget [0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
    //desc.IndependentBlendEnable                 = true;
  }

  // Create the rasterizer state
  {
    D3D11_RASTERIZER_DESC desc = { };

    desc.FillMode        = D3D11_FILL_SOLID;
    desc.CullMode        = D3D11_CULL_NONE;
    desc.ScissorEnable   = true;
    desc.DepthClipEnable = true;

    pDev->CreateRasterizerState (&desc, &g_pRasterizerState);
  }

  // Create depth-stencil State
  {
    D3D11_DEPTH_STENCIL_DESC desc = { };

    desc.DepthEnable              = false;
    desc.DepthWriteMask           = D3D11_DEPTH_WRITE_MASK_ALL;
    desc.DepthFunc                = D3D11_COMPARISON_ALWAYS;
    desc.StencilEnable            = false;
    desc.FrontFace.StencilFailOp  = desc.FrontFace.StencilDepthFailOp =
                                    desc.FrontFace.StencilPassOp      =
                                    D3D11_STENCIL_OP_KEEP;
    desc.FrontFace.StencilFunc    = D3D11_COMPARISON_ALWAYS;
    desc.BackFace                 = desc.FrontFace;

    pDev->CreateDepthStencilState (&desc, &g_pDepthStencilState);
  }

  ImGui_ImplDX11_CreateFontsTexture ();

  return true;
}


using SK_ImGui_ResetCallback_pfn = void (__stdcall *)(void);

SK_LazyGlobal <std::vector <SK_ComPtr <IUnknown>>>       external_resources;
SK_LazyGlobal <std::set    <SK_ImGui_ResetCallback_pfn>> reset_callbacks;

__declspec (dllexport)
void
__stdcall
SKX_ImGui_RegisterResource (IUnknown* pRes)
{
  external_resources->push_back (pRes);
}


__declspec (dllexport)
void
__stdcall
SKX_ImGui_RegisterResetCallback (SK_ImGui_ResetCallback_pfn pCallback)
{
  reset_callbacks->emplace (pCallback);
}

__declspec (dllexport)
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
  SK_TLS* pTLS =
    SK_TLS_Bottom ();


  SK_ImGui_ResetExternal ();

  if (g_pFontSampler_clamp)    { g_pFontSampler_clamp->Release (); g_pFontSampler_clamp = nullptr; }
  if (g_pFontSampler_wrap)     { g_pFontSampler_wrap->Release  (); g_pFontSampler_wrap  = nullptr; }
  if (g_pFontTextureView)      { g_pFontTextureView->Release   (); g_pFontTextureView   = nullptr;  ImGui::GetIO ().Fonts->TexID = nullptr; }
  if (g_pIB)                   { g_pIB->Release                ();              g_pIB   = nullptr; }
  if (g_pVB)                   { g_pVB->Release                ();              g_pVB   = nullptr; }

  if (g_pBlendState)           { g_pBlendState->Release           ();           g_pBlendState = nullptr; }
  if (g_pDepthStencilState)    { g_pDepthStencilState->Release    ();    g_pDepthStencilState = nullptr; }
  if (g_pRasterizerState)      { g_pRasterizerState->Release      ();      g_pRasterizerState = nullptr; }
  if (g_pPixelShader)          { g_pPixelShader->Release          ();          g_pPixelShader = nullptr; }
  if (g_pPixelShaderuPlayHDR)  { g_pPixelShaderuPlayHDR->Release  (); g_pPixelShaderuPlayHDR  = nullptr; }
  if (g_pPixelShaderSteamHDR)  { g_pPixelShaderSteamHDR->Release  (); g_pPixelShaderSteamHDR  = nullptr; }
  if (g_pPixelShaderBlob)      { g_pPixelShaderBlob->Release      ();      g_pPixelShaderBlob = nullptr; }
  if (g_pPixelShaderBlob2)     { g_pPixelShaderBlob2->Release     ();     g_pPixelShaderBlob2 = nullptr; }
  if (g_pPixelShaderBlob3)     { g_pPixelShaderBlob3->Release     ();     g_pPixelShaderBlob3 = nullptr; }
  if (g_pVertexConstantBuffer) { g_pVertexConstantBuffer->Release (); g_pVertexConstantBuffer = nullptr; }
  if (g_pPixelConstantBuffer)  { g_pPixelConstantBuffer->Release  (); g_pPixelConstantBuffer  = nullptr; }
  if (g_pInputLayout)          { g_pInputLayout->Release          ();          g_pInputLayout = nullptr; }
  if (g_pVertexShader)         { g_pVertexShader->Release         ();         g_pVertexShader = nullptr; }
  if (g_pVertexShaderSteamHDR) { g_pVertexShaderSteamHDR->Release (); g_pVertexShaderSteamHDR = nullptr; }
  if (g_pVertexShaderuPlayHDR) { g_pVertexShaderuPlayHDR->Release (); g_pVertexShaderuPlayHDR = nullptr; }
  if (g_pVertexShaderBlob)     { g_pVertexShaderBlob->Release     ();     g_pVertexShaderBlob = nullptr; }
  if (g_pVertexShaderBlob2)    { g_pVertexShaderBlob2->Release    ();    g_pVertexShaderBlob2 = nullptr; }
  if (g_pVertexShaderBlob3)    { g_pVertexShaderBlob3->Release    ();    g_pVertexShaderBlob3 = nullptr; }

  pTLS->d3d11->uiSampler_clamp = nullptr;
  pTLS->d3d11->uiSampler_wrap  = nullptr;
}

bool
ImGui_ImplDX11_Init ( IDXGISwapChain* pSwapChain,
                      ID3D11Device*,
                      ID3D11DeviceContext* )
{
  static bool first = true;

  if (first)
  {
    g_TicksPerSecond  =
      SK_GetPerfFreq ( ).QuadPart;
    g_Time            =
      SK_QueryPerf   ( ).QuadPart;

    first = false;
  }

  ImGuiIO& io =
    ImGui::GetIO ();

  DXGI_SWAP_CHAIN_DESC  swap_desc  = { };
  pSwapChain->GetDesc (&swap_desc);

  g_frameBufferWidth  = swap_desc.BufferDesc.Width;
  g_frameBufferHeight = swap_desc.BufferDesc.Height;
  g_hWnd              = swap_desc.OutputWindow;
  io.ImeWindowHandle  = g_hWnd;

  io.KeyMap [ImGuiKey_Tab]        = VK_TAB;
  io.KeyMap [ImGuiKey_LeftArrow]  = VK_LEFT;
  io.KeyMap [ImGuiKey_RightArrow] = VK_RIGHT;
  io.KeyMap [ImGuiKey_UpArrow]    = VK_UP;
  io.KeyMap [ImGuiKey_DownArrow]  = VK_DOWN;
  io.KeyMap [ImGuiKey_PageUp]     = VK_PRIOR;
  io.KeyMap [ImGuiKey_PageDown]   = VK_NEXT;
  io.KeyMap [ImGuiKey_Home]       = VK_HOME;
  io.KeyMap [ImGuiKey_End]        = VK_END;
  io.KeyMap [ImGuiKey_Insert]     = VK_INSERT;
  io.KeyMap [ImGuiKey_Delete]     = VK_DELETE;
  io.KeyMap [ImGuiKey_Backspace]  = VK_BACK;
  io.KeyMap [ImGuiKey_Space]      = VK_SPACE;
  io.KeyMap [ImGuiKey_Enter]      = VK_RETURN;
  io.KeyMap [ImGuiKey_Escape]     = VK_ESCAPE;
  io.KeyMap [ImGuiKey_A]          = 'A';
  io.KeyMap [ImGuiKey_C]          = 'C';
  io.KeyMap [ImGuiKey_V]          = 'V';
  io.KeyMap [ImGuiKey_X]          = 'X';
  io.KeyMap [ImGuiKey_Y]          = 'Y';
  io.KeyMap [ImGuiKey_Z]          = 'Z';

  static auto& rb =
    SK_GetCurrentRenderBackend ();

  return
    rb.device != nullptr;
}

void
ImGui_ImplDX11_Shutdown (void)
{
  ImGui_ImplDX11_InvalidateDeviceObjects ();
  ImGui::Shutdown                        ();
}

#include <SpecialK/window.h>

void
SK_ImGui_PollGamepad (void);

void
ImGui_ImplDX11_NewFrame (void)
{
  static auto& rb =
    SK_GetCurrentRenderBackend ();

  if (! rb.device)
    return;

  ImGuiIO& io (ImGui::GetIO ());

  if (! g_pFontSampler_clamp)
    ImGui_ImplDX11_CreateDeviceObjects ();

  if (io.Fonts->Fonts.empty ())
  {
    ImGui_ImplDX11_CreateFontsTexture ();

    if (io.Fonts->Fonts.empty ())
      return;
  }

  // Setup display size (every frame to accommodate for window resizing)
  //io.DisplaySize =
    //ImVec2 ( g_frameBufferWidth,
               //g_frameBufferHeight );

  // Setup time step
  INT64 current_time;

  SK_QueryPerformanceCounter (
    reinterpret_cast <LARGE_INTEGER *> (&current_time)
  );

  io.DeltaTime = static_cast <float> (current_time - g_Time) /
                 static_cast <float> (g_TicksPerSecond);
  g_Time       =                      current_time;

  // Read keyboard modifiers inputs
  io.KeyCtrl   = (io.KeysDown [VK_CONTROL]) != 0;
  io.KeyShift  = (io.KeysDown [VK_SHIFT])   != 0;
  io.KeyAlt    = (io.KeysDown [VK_MENU])    != 0;

  io.KeySuper  = false;

  // For games that hijack the mouse cursor using DirectInput 8.
  //
  //  -- Acquire actually means release their exclusive ownership : )
  //
  //if (SK_ImGui_WantMouseCapture ())
  //  SK_Input_DI8Mouse_Acquire ();
  //else
  //  SK_Input_DI8Mouse_Release ();


  // Update OS mouse cursor with the cursor requested by imgui
  //ImGuiMouseCursor mouse_cursor =
  //           io.MouseDrawCursor ? ImGuiMouseCursor_None  :
  //                                ImGui::GetMouseCursor ( );
  //
  //if (g_LastMouseCursor != mouse_cursor)
  //{
  //    g_LastMouseCursor = mouse_cursor;
  //    ImGui_ImplWin32_UpdateMouseCursor();
  //}

  SK_ImGui_PollGamepad ();

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

  if (! rb.device)
    return;

  SK_TLS *pTLS =
    SK_TLS_Bottom ();

  SK_ScopedBool auto_bool (&pTLS->imgui->drawing);

  // Do not dump ImGui font textures
  pTLS->imgui->drawing = true;

  assert (This == rb.swapchain);

  SK_ComPtr <ID3D11Device>        pDev    = nullptr;
  SK_ComPtr <ID3D11DeviceContext> pDevCtx = nullptr;

  if (rb.d3d11.immediate_ctx != nullptr)
  {
    auto flag_result =
      SK_ImGui_FlagDrawing_OnD3D11Ctx (
        SK_D3D11_GetDeviceContextHandle (pDevCtx)
      );

    SK_ScopedBool auto_bool0 (flag_result.first);
                             *flag_result.first = flag_result.second;

    HRESULT hr0 = rb.device->QueryInterface              <ID3D11Device>        (&pDev.p);
    HRESULT hr1 = rb.d3d11.immediate_ctx->QueryInterface <ID3D11DeviceContext> (&pDevCtx.p);

    if (SUCCEEDED (hr0) && SUCCEEDED (hr1))
    {
      ImGui_ImplDX11_InvalidateDeviceObjects ();
    }
  }
}