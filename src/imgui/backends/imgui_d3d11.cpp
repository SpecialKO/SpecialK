// ImGui Win32 + DirectX11 binding
// In this binding, ImTextureID is used to store a 'ID3D11ShaderResourceView*' texture identifier. Read the FAQ about ImTextureID in imgui.cpp.

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you use this binding you'll need to call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(), ImGui::Render() and ImGui_ImplXXXX_Shutdown().
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

#include <imgui/imgui.h>
#include <imgui/backends/imgui_d3d11.h>
#include <SpecialK/render/backend.h>
#include <SpecialK/render/dxgi/dxgi_backend.h>
#include <SpecialK/framerate.h>
#include <SpecialK/config.h>
#include <SpecialK/log.h>

// DirectX
#include <d3d11.h>
#include <d3dcompiler.h>

#include <atlbase.h>
#include <Windows.h>

extern void
SK_ImGui_User_NewFrame (void);

// Data
static INT64                    g_Time                  = 0;
static INT64                    g_TicksPerSecond        = 0;

static HWND                     g_hWnd                  = nullptr;
static ID3D11Buffer*            g_pVB                   = nullptr;
static ID3D11Buffer*            g_pIB                   = nullptr;
static ID3D10Blob *             g_pVertexShaderBlob     = nullptr;
static ID3D10Blob *             g_pVertexShaderBlob2    = nullptr;
static ID3D11VertexShader*      g_pVertexShader         = nullptr;
static ID3D11VertexShader*      g_pVertexShaderSteamHDR = nullptr;
static ID3D11PixelShader*       g_pPixelShaderSteamHDR  = nullptr;
static ID3D11InputLayout*       g_pInputLayout          = nullptr;
static ID3D11Buffer*            g_pVertexConstantBuffer = nullptr;
static ID3D11Buffer*            g_pPixelConstantBuffer  = nullptr;
static ID3D10Blob *             g_pPixelShaderBlob      = nullptr;
static ID3D10Blob *             g_pPixelShaderBlob2     = nullptr;
static ID3D11PixelShader*       g_pPixelShader          = nullptr;
static ID3D11SamplerState*      g_pFontSampler_clamp    = nullptr;
static ID3D11SamplerState*      g_pFontSampler_wrap     = nullptr;
static ID3D11ShaderResourceView*g_pFontTextureView      = nullptr;
static ID3D11ShaderResourceView*g_pHDRCompositeView     = nullptr;
static ID3D11RasterizerState*   g_pRasterizerState      = nullptr;
static ID3D11BlendState*        g_pBlendState           = nullptr;
static ID3D11BlendState*        g_pHDRUIBlendState      = nullptr;
static ID3D11DepthStencilState* g_pDepthStencilState    = nullptr;

static UINT                     g_frameBufferWidth      = 0UL;
static UINT                     g_frameBufferHeight     = 0UL;

static int                      g_VertexBufferSize      = 5000,
                                g_IndexBufferSize       = 10000;


ID3D11ShaderResourceView*
SK_D3D11_GetRawHDRView ( bool capture )
{
  if (capture && g_pHDRCompositeView != nullptr)
  {
    SK_RenderBackend& rb =
      SK_GetCurrentRenderBackend ();

    CComPtr   <ID3D11Resource>      pSrc, pDst;
    CComQIPtr <IDXGISwapChain>      pSwapChain (rb.swapchain);
    CComQIPtr <ID3D11DeviceContext> pDevCtx    (rb.d3d11.immediate_ctx);

    g_pHDRCompositeView->GetResource (&pDst.p);

    if ( SUCCEEDED (
           pSwapChain->GetBuffer    (
             0,
               IID_ID3D11Texture2D,
                 (void **) &pSrc.p  )
                   )
       )
    {
      pDevCtx->CopyResource (pDst, pSrc);

      return
        g_pHDRCompositeView;
    }

    // Uh oh, user wanted an update but it was not possible
    //   now they get nothing!
    return nullptr;
  }

  return
    g_pHDRCompositeView;
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


// This is the main rendering function that you have to implement and provide to ImGui (via setting up 'RenderDrawListsFn' in the ImGuiIO structure)
// If text or lines are blurry when integrating ImGui in your engine:
// - in your Render function, try translating your projection matrix by (0.5f,0.5f) or (0.375f,0.375f)
IMGUI_API
void
ImGui_ImplDX11_RenderDrawLists (ImDrawData* draw_data)
{
  SK_TLS* pTLS =
    SK_TLS_Bottom ();

  SK_ScopedBool auto_bool (&pTLS->imgui.drawing);
                            pTLS->imgui.drawing = true;

  ImGuiIO& io =
    ImGui::GetIO ();

  SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  if (! rb.swapchain)
    return;

  if (! rb.device)
    return;

  if (! rb.d3d11.immediate_ctx)
    return;

  if (! g_pVertexConstantBuffer)
    return;

  if (! g_pPixelConstantBuffer)
    return;


  CComQIPtr <IDXGISwapChain>               pSwapChain (rb.swapchain);
  CComQIPtr <ID3D11Device>                 pDevice    (rb.device);
  CComQIPtr <ID3D11DeviceContext>          pDevCtx    (rb.d3d11.immediate_ctx);

  CComPtr   <ID3D11Texture2D>              pBackBuffer = nullptr;
  pSwapChain->GetBuffer (0, IID_PPV_ARGS (&pBackBuffer));

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

    if (pDevice->CreateBuffer (&desc, nullptr, &g_pVB) < 0)
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

    if (pDevice->CreateBuffer (&desc, nullptr, &g_pIB) < 0)
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
        ImU32& color =
          cmd_list->VtxBuffer.Data [i].col;

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

    if (pDevCtx->Map (g_pVertexConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource) != S_OK)
      return;

    auto* constant_buffer =
      static_cast <VERTEX_CONSTANT_BUFFER *> (mapped_resource.pData);

    memcpy         (&constant_buffer->mvp, mvp, sizeof (mvp));

    bool hdr_display =
      (rb.framebuffer_flags & SK_FRAMEBUFFER_FLAG_HDR);

    if (! hdr_display)
    {
      constant_buffer->luminance_scale [0] = 1.0f; constant_buffer->luminance_scale [1] = 1.0f;
      constant_buffer->luminance_scale [2] = 0.0f; constant_buffer->luminance_scale [3] = 0.0f;
      constant_buffer->steam_luminance [0] = 1.0f; constant_buffer->steam_luminance [1] = 1.0f;
    }

    else
    {
      extern float __SK_MHW_HDR_Luma; extern float __SK_DQXI_HDR_Luma;
      extern float __SK_MHW_HDR_Exp;  extern float __SK_DQXI_HDR_Exp;

      float luma = 0.0f,
            exp  = 0.0f;

      switch (SK_GetCurrentGameID ())
      {
        case SK_GAME_ID::MonsterHunterWorld:
          luma = __SK_MHW_HDR_Luma;
          exp  = __SK_MHW_HDR_Exp-1.0f;
          break;

        case SK_GAME_ID::DragonQuestXI:
          luma = __SK_DQXI_HDR_Luma;
          exp  = __SK_DQXI_HDR_Exp;
          break;
      }

      constant_buffer->luminance_scale [0] = rb.ui_luminance;
      constant_buffer->luminance_scale [1] = rb.ui_srgb ? 2.2f : 1.0f;
      constant_buffer->luminance_scale [2] = luma;
      constant_buffer->luminance_scale [3] = exp;
      constant_buffer->steam_luminance [0] = config.steam.overlay_hdr_luminance;
      constant_buffer->steam_luminance [1] = rb.ui_srgb ? 2.2f : 1.0f;
    }

    pDevCtx->Unmap (g_pVertexConstantBuffer, 0);


    if (pDevCtx->Map (g_pPixelConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource) != S_OK)
      return;

    ((float *)mapped_resource.pData)[2] = hdr_display ? (float)backbuffer_desc.Width  : 0.0f;
    ((float *)mapped_resource.pData)[3] = hdr_display ? (float)backbuffer_desc.Height : 0.0f;

    pDevCtx->Unmap (g_pPixelConstantBuffer, 0);
  }

  CComPtr <ID3D11RenderTargetView> pRenderTargetView = nullptr;

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

    default:
     pDevice->CreateRenderTargetView (pBackBuffer, nullptr, &pRenderTargetView);
  }

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

  if ( (rb.framebuffer_flags & SK_FRAMEBUFFER_FLAG_HDR) && g_pHDRCompositeView != nullptr )
  {
    CComPtr <ID3D11Resource> pSrc, pDst;

    g_pHDRCompositeView->GetResource (                                 &pDst.p);
    pSwapChain->GetBuffer            (0, IID_ID3D11Texture2D, (void **)&pSrc.p);

    pDevCtx->CopyResource           (pDst, pSrc);
    pDevCtx->OMSetBlendState        (g_pHDRUIBlendState, blend_factor, 0xffffffff);
    pDevCtx->PSSetShaderResources   (1, 1, &g_pHDRCompositeView);
  }
  else
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

        pDevCtx->PSSetShaderResources (0, 1, (ID3D11ShaderResourceView **)&pcmd->TextureId);
        pDevCtx->RSSetScissorRects    (1, &r);

        pDevCtx->DrawIndexed (pcmd->ElemCount, idx_offset, vtx_offset);
      }

      idx_offset += pcmd->ElemCount;
    }

    vtx_offset += cmd_list->VtxBuffer.Size;
  }
}

#include <SpecialK/config.h>

static void
ImGui_ImplDX11_CreateFontsTexture (void)
{
  SK_TLS* pTLS =
    SK_TLS_Bottom ();

  SK_ScopedBool auto_bool0 (&pTLS->texture_management.injection_thread);
  SK_ScopedBool auto_bool1 (&pTLS->imgui.drawing                      );

  // Do not dump ImGui font textures
  pTLS->texture_management.injection_thread = true;
  pTLS->imgui.drawing                       = true;

  // Build texture atlas
  ImGuiIO& io (ImGui::GetIO ());

  static bool           init   = false;
  static unsigned char* pixels = nullptr;
  static int            width  = 0,
                        height = 0;

  // Only needs to be done once, the raw pixels are API agnostic
  if (! init)
  {
    SK_ImGui_LoadFonts ();

    io.Fonts->GetTexDataAsRGBA32 (&pixels, &width, &height);

    init = true;
  }

  SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  CComQIPtr <ID3D11Device> pDev (rb.device);

  // Upload texture to graphics system
  {
    D3D11_TEXTURE2D_DESC desc
                          = { };

    desc.Width            = width;
    desc.Height           = height;
    desc.MipLevels        = 1;
    desc.ArraySize        = 1;
    desc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage            = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags        = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags   = 0;

    CComPtr <ID3D11Texture2D> pFontTexture = nullptr;
    D3D11_SUBRESOURCE_DATA    subResource  = { };

    subResource.pSysMem          = pixels;
    subResource.SysMemPitch      = desc.Width * 4;
    subResource.SysMemSlicePitch = 0;

    pDev->CreateTexture2D (&desc, &subResource, &pFontTexture.p);

    // Create texture view
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = { };

    srvDesc.Format                    = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels       = desc.MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;

    pDev->CreateShaderResourceView (pFontTexture, &srvDesc, &g_pFontTextureView);

    if (rb.framebuffer_flags & SK_FRAMEBUFFER_FLAG_HDR)
    {
      CComQIPtr <IDXGISwapChain> pSwapChain (rb.swapchain);

      DXGI_SWAP_CHAIN_DESC  swapDesc = { };
      pSwapChain->GetDesc (&swapDesc);

      desc.Width            = swapDesc.BufferDesc.Width;
      desc.Height           = swapDesc.BufferDesc.Height;
      desc.MipLevels        = 1;
      desc.ArraySize        = 1;
      desc.Format           = swapDesc.BufferDesc.Format;
      desc.SampleDesc.Count = 1; // Will probably regret this if HDR ever procreates with MSAA
      desc.Usage            = D3D11_USAGE_DEFAULT;
      desc.BindFlags        = D3D11_BIND_SHADER_RESOURCE;
      desc.CPUAccessFlags   = 0;

      CComPtr <ID3D11Texture2D> pHDRTexture = nullptr;

      pDev->CreateTexture2D          (&desc,       nullptr, &pHDRTexture.p);
      pDev->CreateShaderResourceView (pHDRTexture, nullptr, &g_pHDRCompositeView);
    }

    else if (g_pHDRCompositeView != nullptr)
    {
      g_pHDRCompositeView->Release ();
      g_pHDRCompositeView = nullptr;
    }
  }

  // Store our identifier
  io.Fonts->TexID =
    static_cast <void *> (g_pFontTextureView);

  // Create texture sampler
  {
    D3D11_SAMPLER_DESC desc = { };

    desc.Filter         = D3D11_FILTER_MIN_MAG_MIP_POINT;
    desc.AddressU       = D3D11_TEXTURE_ADDRESS_CLAMP;
    desc.AddressV       = D3D11_TEXTURE_ADDRESS_CLAMP;
    desc.AddressW       = D3D11_TEXTURE_ADDRESS_CLAMP;
    desc.MipLODBias     = 0.f;
    desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    desc.MinLOD         = 0.f;
    desc.MaxLOD         = 0.f;

    pDev->CreateSamplerState (&desc, &g_pFontSampler_clamp);
    pTLS->d3d11.uiSampler_clamp =     g_pFontSampler_clamp;

    desc = { };

    desc.Filter         = D3D11_FILTER_ANISOTROPIC;
    desc.AddressU       = D3D11_TEXTURE_ADDRESS_MIRROR;
    desc.AddressV       = D3D11_TEXTURE_ADDRESS_MIRROR;
    desc.AddressW       = D3D11_TEXTURE_ADDRESS_MIRROR;
    desc.MipLODBias     = 0.f;
    desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    desc.MinLOD         = 0.f;
    desc.MaxLOD         = 0.f;

    pDev->CreateSamplerState (&desc, &g_pFontSampler_wrap);
    pTLS->d3d11.uiSampler_wrap =      g_pFontSampler_wrap;
  }
}

HRESULT
SK_D3D11_InjectSteamHDR ( _In_ ID3D11DeviceContext *pDevCtx,
                          _In_ UINT                 VertexCount,
                          _In_ UINT                 StartVertexLocation,
                          _In_ D3D11_Draw_pfn       pfnD3D11Draw )
{
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

    CComPtr <ID3D11PixelShader>  pOrigPixShader;
    CComPtr <ID3D11VertexShader> pOrigVtxShader;
    CComPtr <ID3D11Buffer>       pOrigVtxCB;

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

  SK_ScopedBool auto_bool (&pTLS->imgui.drawing);

  // Do not dump ImGui font textures
  pTLS->imgui.drawing = true;

  if (g_pFontSampler_clamp)
    ImGui_ImplDX11_InvalidateDeviceObjects ();

  SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  if (! rb.device)
    return false;

  if (! rb.d3d11.immediate_ctx)
    return false;

  if (! rb.swapchain)
    return false;

  CComQIPtr <ID3D11Device> pDev (rb.device);

  // Create the vertex shader
  {
    static const char* vertexShader =
     "#pragma warning ( disable : 3571 )\n                                    \
      cbuffer vertexBuffer : register (b0)                                    \
      {                                                                       \
        float4x4 ProjectionMatrix;                                            \
        float4   Luminance;                                                   \
        float4   SteamLuminance;                                              \
      };                                                                      \
                                                                              \
      struct VS_INPUT                                                         \
      {                                                                       \
        float2 pos : POSITION;                                                \
        float4 col : COLOR0;                                                  \
        float2 uv  : TEXCOORD0;                                               \
      };                                                                      \
                                                                              \
      struct PS_INPUT                                                         \
      {                                                                       \
        float4 pos : SV_POSITION;                                             \
        float4 col : COLOR0;                                                  \
        float2 uv  : TEXCOORD0;                                               \
        float2 uv2 : TEXCOORD1;                                               \
        float2 uv3 : TEXCOORD2;                                               \
      };                                                                      \
                                                                              \
      PS_INPUT main (VS_INPUT input)                                          \
      {                                                                       \
        PS_INPUT output;                                                      \
                                                                              \
        output.pos          = mul ( ProjectionMatrix,                         \
                                      float4 (input.pos.xy, 0.f, 1.f) );      \
                                                                              \
        output.uv           = saturate (input.uv);                            \
                                                                              \
        if ( Luminance.w > 0.f && Luminance.z > 0.f &&                        \
                             abs (input.uv.x) > 1.f )                         \
        {                                                                     \
          output.col        = float4 (1.f, 1.f, 1.f, 1.f);                    \
          output.uv2        = Luminance.zw;                                   \
          output.uv3        =          float2 (0.f, 0.f);                     \
        }                                                                     \
                                                                              \
        else                                                                  \
        {                                                                     \
          output.col        = input.col;                                      \
          output.uv2        = float2 (0.f, 0.f);                              \
          output.uv3        = Luminance.xy;                                   \
        }                                                                     \
                                                                              \
        return output;                                                        \
      }";

    CComPtr <ID3D10Blob> blob_msg_vtx;

    D3DCompile ( vertexShader,
                   strlen (vertexShader),
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
        dll_log.LogEx (true, L"ImGui D3D11 Vertex Shader: %hs", err.c_str ());
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


    static const char* vertexShaderSteamHDR =
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

    D3DCompile ( vertexShaderSteamHDR,
                   strlen (vertexShaderSteamHDR),
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
        dll_log.LogEx (true, L"Steam HDR D3D11 Vertex Shader: %hs", err.c_str ());
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


    // Create the input layout
    D3D11_INPUT_ELEMENT_DESC local_layout [] = {
      { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (size_t)(&((ImDrawVert *)nullptr)->pos), D3D11_INPUT_PER_VERTEX_DATA, 0 },
      { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (size_t)(&((ImDrawVert *)nullptr)->uv),  D3D11_INPUT_PER_VERTEX_DATA, 0 },
      { "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, (size_t)(&((ImDrawVert *)nullptr)->col), D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    if ( pDev->CreateInputLayout ( local_layout, 3,
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
    static const char* pixelShader =
     "#pragma warning ( disable : 3571 )\n                             \
      struct PS_INPUT                                                  \
      {                                                                \
        float4 pos : SV_POSITION;                                      \
        float4 col : COLOR0;                                           \
        float2 uv  : TEXCOORD0;                                        \
        float2 uv2 : TEXCOORD1;                                        \
        float2 uv3 : TEXCOORD2;                                        \
      };                                                               \
                                                                       \
      cbuffer viewportDims : register (b0)                             \
      {                                                                \
        float4 viewport;                                               \
      };                                                               \
                                                                       \
      sampler   sampler0    : register (s0);                           \
                                                                       \
      Texture2D texture0    : register (t0);                           \
      Texture2D hdrUnderlay : register (t1);                           \
\
float3 ApplyREC2084Curve(float3 L, float maxLuminance)\
{\
	float m1 = 2610.0 / 4096.0 / 4;\
	float m2 = 2523.0 / 4096.0 * 128;\
	float c1 = 3424.0 / 4096.0;\
	float c2 = 2413.0 / 4096.0 * 32;\
	float c3 = 2392.0 / 4096.0 * 32;\
  \
	/* L = FD / 10000, so if FD == 10000, then L = 1.*/\
	/* so to scale max luminance, we want to multiply by maxLuminance / 10000*/\
  \
	float maxLightScale = maxLuminance / 10000.0f;\
	L *= maxLightScale;\
  \
	float3 Lp = pow(L, m1);\
  \
	return pow((c1 + c2 * Lp) / (1 + c3 * Lp), m2);\
}\
\
float3 REC709toREC2020(float3 RGB709)\
{\
	static const float3x3 ConvMat =\
	{\
		0.627402, 0.329292, 0.043306,\
		0.069095, 0.919544, 0.011360,\
		0.016394, 0.088028, 0.895578\
	};\
\
	return mul(ConvMat, RGB709);\
}\
\
float3 ApplyREC709Curve(float3 x)\
{\
	return x < 0.0181 ? 4.5 * x : 1.0993 * pow(x, 0.45) - 0.0993;\
}\
\
float3 ApplySRGBCurve(float3 x)\
{\
	/* Approximately pow(x, 1.0 / 2.2) */\
	return x < 0.0031308 ? 12.92 * x : 1.055 * pow(x, 1.0 / 2.4) - 0.055;\
}\
\
float ApplySRGBCurve(float x)\
{\
	/* Approximately pow(x, 1.0 / 2.2)*/\
	return x < 0.0031308 ? 12.92 * x : 1.055 * pow(x, 1.0 / 2.4) - 0.055;\
}\
\
float3 RemoveSRGBCurve(float3 x)\
{\
	/* Approximately pow(x, 2.2)*/\
	return x < 0.04045 ? x / 12.92 : pow((x + 0.055) / 1.055, 2.4);\
}\
\
static const float tenThousand = 10000.0f;\
\
float Luminance(float3 linearColor)\
{\
	return 0.2126 * linearColor.r + 0.7152 * linearColor.g + 0.0722 * linearColor.b;\
}\
                                                                       \
      float4 main (PS_INPUT input) : SV_Target                         \
      {                                                                \
        float4 out_col =                                               \
          texture0.Sample (sampler0, input.uv);                        \
                                                                       \
        if (viewport.z > 0.f)                                          \
        {                                                              \
          float4 under_color =                                         \
            hdrUnderlay.Sample (sampler0, input.pos.xy / viewport.zw); \
                                                                       \
          float blend_alpha =                                          \
            saturate (input.col.a * out_col.a);                        \
                                                                       \
          if (input.uv2.x > 0.0f && input.uv2.y > 0.0f)                \
          {                                                            \
            out_col.rgb = RemoveSRGBCurve   (out_col.rgb);             \
            out_col =                                                  \
              pow (abs (out_col), float4 (input.uv2.yyy, 1.0f)) *      \
                input.uv2.xxxx;                                        \
            /*out_col.rgb = ApplyREC2084Curve (out_col.rgb, input.uv2.x);*/\
            out_col.a   = 1.0f;                                        \
            blend_alpha = 1.0f;                                        \
          }                                                            \
                                                                       \
          else                                                         \
          {                                                            \
            out_col =                                                  \
              pow (abs (input.col * out_col), float4 (input.uv3.yyy, 1.0f)) *      \
                                              float4 (input.uv3.xxx, 1.0f);        \
            out_col *= float4 (out_col.aaa, 1.f);                      \
          }                                                            \
                                                                       \
          return                                                       \
            float4 (  ( (input.col.rgb * out_col.rgb)) -               \
               (1.0f - blend_alpha) * float4 (under_color.rgb, 1.0f),  \
                       blend_alpha);                                   \
        }                                                              \
                                                                       \
        return                                                         \
          ( input.col * out_col );                                     \
      }";

    CComPtr <ID3D10Blob> blob_msg_pix;

    D3DCompile ( pixelShader,
                   strlen (pixelShader),
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
           (char *)blob_msg_pix->GetBufferPointer ());

      if (! err.empty ())
      {
        dll_log.LogEx (true, L"ImGui D3D11 Pixel Shader:  %hs", err.c_str ());
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
    static const char* pixelShaderSteamHDR =
   "#pragma warning ( disable : 3571 )\n                         \
    struct PS_INPUT                                              \
    {                                                            \
      float4 pos : SV_POSITION;                                  \
      float4 col : COLOR;                                        \
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
        float4 (RemoveSRGBCurve (input.col.rgb * out_col.rgb), input.col.a * out_col.a) * linear_mul; \
                                                                 \
      return                                                     \
        float4 (out_col.rgb, saturate (out_col.a));              \
    }";

    D3DCompile ( pixelShaderSteamHDR,
                   strlen (pixelShaderSteamHDR),
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
        dll_log.LogEx (true, L"Steam HDR D3D11 Pixel Shader: %hs", err.c_str ());
      }
    }

    if (g_pPixelShaderBlob2 == nullptr)
      return false;

    if ( pDev->CreatePixelShader ( static_cast <DWORD *> (g_pPixelShaderBlob2->GetBufferPointer ()),
                                     g_pPixelShaderBlob2->GetBufferSize (),
                                       nullptr,
                                         &g_pPixelShaderSteamHDR ) != S_OK )
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

    desc.RenderTarget [0].BlendEnable           =  true;

    pDev->CreateBlendState (&desc, &g_pHDRUIBlendState);
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

std::vector <IUnknown *>                 external_resources;
std::set    <SK_ImGui_ResetCallback_pfn> reset_callbacks;

__declspec (dllexport)
void
__stdcall
SKX_ImGui_RegisterResource (IUnknown* pRes)
{
  external_resources.push_back (pRes);
}


__declspec (dllexport)
void
__stdcall
SKX_ImGui_RegisterResetCallback (SK_ImGui_ResetCallback_pfn pCallback)
{
  reset_callbacks.emplace (pCallback);
}

__declspec (dllexport)
void
__stdcall
SKX_ImGui_UnregisterResetCallback (SK_ImGui_ResetCallback_pfn pCallback)
{
  if (reset_callbacks.count (pCallback))
    reset_callbacks.erase (pCallback);
}

void
SK_ImGui_ResetExternal (void)
{
  for ( auto it : external_resources )
  {
    it->Release ();
  }

  external_resources.clear ();

  for ( auto reset_fn : reset_callbacks )
  {
    reset_fn ();
  }
}


void
ImGui_ImplDX11_InvalidateDeviceObjects (void)
{
  SK_TLS* pTLS =
    SK_TLS_Bottom ();

  SK_ScopedBool auto_bool (&pTLS->imgui.drawing);

  // Do not dump ImGui font textures
  pTLS->imgui.drawing = true;

  SK_ImGui_ResetExternal ();

  if (g_pFontSampler_clamp)    { g_pFontSampler_clamp->Release (); g_pFontSampler_clamp = nullptr; }
  if (g_pFontSampler_wrap)     { g_pFontSampler_wrap->Release  (); g_pFontSampler_wrap  = nullptr; }
  if (g_pFontTextureView)      { g_pFontTextureView->Release   (); g_pFontTextureView   = nullptr;  ImGui::GetIO ().Fonts->TexID = nullptr; }
  if (g_pHDRCompositeView)     { g_pHDRCompositeView->Release  (); g_pHDRCompositeView  = nullptr; }
  if (g_pIB)                   { g_pIB->Release                ();              g_pIB   = nullptr; }
  if (g_pVB)                   { g_pVB->Release                ();              g_pVB   = nullptr; }

  if (g_pBlendState)           { g_pBlendState->Release           ();           g_pBlendState = nullptr; }
  if (g_pHDRUIBlendState)      { g_pHDRUIBlendState->Release      ();      g_pHDRUIBlendState = nullptr; }
  if (g_pDepthStencilState)    { g_pDepthStencilState->Release    ();    g_pDepthStencilState = nullptr; }
  if (g_pRasterizerState)      { g_pRasterizerState->Release      ();      g_pRasterizerState = nullptr; }
  if (g_pPixelShader)          { g_pPixelShader->Release          ();          g_pPixelShader = nullptr; }
  if (g_pPixelShaderSteamHDR)  { g_pPixelShaderSteamHDR->Release  (); g_pPixelShaderSteamHDR  = nullptr; }
  if (g_pPixelShaderBlob)      { g_pPixelShaderBlob->Release      ();      g_pPixelShaderBlob = nullptr; }
  if (g_pPixelShaderBlob2)     { g_pPixelShaderBlob2->Release     ();     g_pPixelShaderBlob2 = nullptr; }
  if (g_pVertexConstantBuffer) { g_pVertexConstantBuffer->Release (); g_pVertexConstantBuffer = nullptr; }
  if (g_pPixelConstantBuffer)  { g_pPixelConstantBuffer->Release  (); g_pPixelConstantBuffer  = nullptr; }
  if (g_pInputLayout)          { g_pInputLayout->Release          ();          g_pInputLayout = nullptr; }
  if (g_pVertexShader)         { g_pVertexShader->Release         ();         g_pVertexShader = nullptr; }
  if (g_pVertexShaderSteamHDR) { g_pVertexShaderSteamHDR->Release (); g_pVertexShaderSteamHDR = nullptr; }
  if (g_pVertexShaderBlob)     { g_pVertexShaderBlob->Release     ();     g_pVertexShaderBlob = nullptr; }
  if (g_pVertexShaderBlob2)    { g_pVertexShaderBlob2->Release    ();    g_pVertexShaderBlob2 = nullptr; }

  pTLS->d3d11.uiSampler_clamp = nullptr;
  pTLS->d3d11.uiSampler_wrap  = nullptr;
}

bool
ImGui_ImplDX11_Init ( IDXGISwapChain* pSwapChain,
                      ID3D11Device*,
                      ID3D11DeviceContext* )
{
  static bool first = true;

  if (first)
  {
    if (! QueryPerformanceFrequency  (reinterpret_cast <LARGE_INTEGER *> (&g_TicksPerSecond)))
      return false;

    if (! SK_QueryPerformanceCounter (reinterpret_cast <LARGE_INTEGER *> (&g_Time)))
      return false;

    first = false;
  }

  DXGI_SWAP_CHAIN_DESC  swap_desc  = { };
  pSwapChain->GetDesc (&swap_desc);

  g_hWnd              = swap_desc.OutputWindow;

  g_frameBufferWidth  = swap_desc.BufferDesc.Width;
  g_frameBufferHeight = swap_desc.BufferDesc.Height;

  ImGuiIO& io =
    ImGui::GetIO ();

  // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array that we will update during the application lifetime.
  io.KeyMap [ImGuiKey_Tab]        = VK_TAB;
  io.KeyMap [ImGuiKey_LeftArrow]  = VK_LEFT;
  io.KeyMap [ImGuiKey_RightArrow] = VK_RIGHT;
  io.KeyMap [ImGuiKey_UpArrow]    = VK_UP;
  io.KeyMap [ImGuiKey_DownArrow]  = VK_DOWN;
  io.KeyMap [ImGuiKey_PageUp]     = VK_PRIOR;
  io.KeyMap [ImGuiKey_PageDown]   = VK_NEXT;
  io.KeyMap [ImGuiKey_Home]       = VK_HOME;
  io.KeyMap [ImGuiKey_End]        = VK_END;
  io.KeyMap [ImGuiKey_Delete]     = VK_DELETE;
  io.KeyMap [ImGuiKey_Backspace]  = VK_BACK;
  io.KeyMap [ImGuiKey_Enter]      = VK_RETURN;
  io.KeyMap [ImGuiKey_Escape]     = VK_ESCAPE;
  io.KeyMap [ImGuiKey_A]          = 'A';
  io.KeyMap [ImGuiKey_C]          = 'C';
  io.KeyMap [ImGuiKey_V]          = 'V';
  io.KeyMap [ImGuiKey_X]          = 'X';
  io.KeyMap [ImGuiKey_Y]          = 'Y';
  io.KeyMap [ImGuiKey_Z]          = 'Z';

  // Alternatively you can set this to NULL and call ImGui::GetDrawData() after ImGui::Render() to get the same ImDrawData pointer.
  io.RenderDrawListsFn = ImGui_ImplDX11_RenderDrawLists;
  io.ImeWindowHandle   = g_hWnd;

  return SK_GetCurrentRenderBackend ().device != nullptr;
}

void
ImGui_ImplDX11_Shutdown (void)
{
  SK_TLS *pTLS =
    SK_TLS_Bottom ();

  SK_ScopedBool auto_bool (&pTLS->imgui.drawing);

  // Do not dump ImGui font textures
  pTLS->imgui.drawing = true;

  ImGui_ImplDX11_InvalidateDeviceObjects ();
  ImGui::Shutdown                        ();
}

#include <SpecialK/window.h>

void
SK_ImGui_PollGamepad (void);

void
ImGui_ImplDX11_NewFrame (void)
{
  if (! SK_GetCurrentRenderBackend ().device)
    return;

  if (! g_pFontSampler_clamp)
    ImGui_ImplDX11_CreateDeviceObjects ();
  
  ImGuiIO& io (ImGui::GetIO ());

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


  SK_ImGui_PollGamepad ();


  // Start the frame
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

  SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  if (! rb.device)
    return;

  SK_TLS *pTLS =
    SK_TLS_Bottom ();

  SK_ScopedBool auto_bool (&pTLS->imgui.drawing);

  // Do not dump ImGui font textures
  pTLS->imgui.drawing = true;


  assert (This == rb.swapchain);

  CComPtr <ID3D11Device>        pDev    = nullptr;
  CComPtr <ID3D11DeviceContext> pDevCtx = nullptr;

  if (rb.d3d11.immediate_ctx != nullptr)
  {
    HRESULT hr0 = rb.device->QueryInterface              <ID3D11Device>        (&pDev.p);
    HRESULT hr1 = rb.d3d11.immediate_ctx->QueryInterface <ID3D11DeviceContext> (&pDevCtx.p);

    if (SUCCEEDED (hr0) && SUCCEEDED (hr1))
    {
      ImGui_ImplDX11_InvalidateDeviceObjects ();
    }
  }
}