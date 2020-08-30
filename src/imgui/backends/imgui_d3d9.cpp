// ImGui Win32 + DirectX9 binding
// In this binding, ImTextureID is used to store a 'LPDIRECT3DTEXTURE9' texture identifier. Read the FAQ about ImTextureID in imgui.cpp.

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you use this binding you'll need to call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(), ImGui::Render() and ImGui_ImplXXXX_Shutdown().
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

#include <SpecialK/stdafx.h>


#include <imgui/backends/imgui_d3d9.h>

extern void
SK_ImGui_User_NewFrame (void);


// DirectX


// Data
static HWND                    g_hWnd             = 0;
static INT64                   g_Time             = 0;
static INT64                   g_TicksPerSecond   = 0;
       LPDIRECT3DDEVICE9       g_pd3dDevice       = nullptr;
static LPDIRECT3DVERTEXBUFFER9 g_pVB              = nullptr;
static LPDIRECT3DINDEXBUFFER9  g_pIB              = nullptr;
static LPDIRECT3DTEXTURE9      g_FontTexture      = nullptr;
static int                     g_VertexBufferSize = 5000,
                               g_IndexBufferSize  = 10000;

struct CUSTOMVERTEX
{
  float     pos [3];
  D3DCOLOR  col;
  float     uv  [2];
  float padding [2];
};
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX2)

bool
SK_D3D9_TestFramebufferDimensions (float& width, float& height)
{
  RECT                                 rect = { };
  GetClientRect (SK_GetGameWindow (), &rect);

  float client_width  =
    static_cast <float> (rect.right  - rect.left);

  float client_height =
    static_cast <float> (rect.bottom - rect.top);

  if (width <= 0.0f && height <= 0.0f)
  {
    width  = client_width;
    height = client_height;

    return true;
  }

  if ( width  != client_width ||
       height != client_height )
  {
    return false;
  }

  return true;
}

// This is the main rendering function that you have to implement and provide to ImGui (via setting up 'RenderDrawListsFn' in the ImGuiIO structure)
// If text or lines are blurry when integrating ImGui in your engine:
// - in your Render function, try translating your projection matrix by (0.5f,0.5f) or (0.375f,0.375f)
IMGUI_API
void
ImGui_ImplDX9_RenderDrawData (ImDrawData* draw_data)
{
  // Avoid rendering when minimized
  auto& io =
    ImGui::GetIO ();

  if ( io.DisplaySize.x <= 0.0f || io.DisplaySize.y <= 0.0f )
  {
    SK_D3D9_TestFramebufferDimensions (io.DisplaySize.x, io.DisplaySize.y);

    if ( io.DisplaySize.x <= 0.0f || io.DisplaySize.y <= 0.0f )
      return;
  }

  // Create and grow buffers if needed
  if ((! g_pVB) || g_VertexBufferSize < draw_data->TotalVtxCount )
  {
    if (g_pVB)
    {
      g_pVB->Release ();
      g_pVB = nullptr;
    }

    g_VertexBufferSize =
      draw_data->TotalVtxCount + 5000;

    if ( g_pd3dDevice->CreateVertexBuffer ( g_VertexBufferSize * sizeof (CUSTOMVERTEX),
                                              D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
                                                D3DFVF_CUSTOMVERTEX,
                                                  D3DPOOL_DEFAULT,
                                                    &g_pVB,
                                                      nullptr ) < 0 )
    {
      return;
    }
  }

  if ((! g_pIB) || g_IndexBufferSize < draw_data->TotalIdxCount)
  {
    if (g_pIB)
    {
      g_pIB->Release ();
      g_pIB = nullptr;
    }

    g_IndexBufferSize = draw_data->TotalIdxCount + 10000;

    if ( g_pd3dDevice->CreateIndexBuffer ( g_IndexBufferSize * sizeof (ImDrawIdx),
                                             D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
                                               sizeof (ImDrawIdx) == 2 ?
                                                  D3DFMT_INDEX16 :
                                                  D3DFMT_INDEX32,
                                                D3DPOOL_DEFAULT,
                                                  &g_pIB,
                                                    nullptr ) < 0 )
    {
      return;
    }
  }

  //// Backup the DX9 state
  IDirect3DStateBlock9* d3d9_state_block = nullptr;

  if (g_pd3dDevice->CreateStateBlock ( D3DSBT_ALL, &d3d9_state_block ) < 0 )
    return;

  // Copy and convert all vertices into a single contiguous buffer
  CUSTOMVERTEX* vtx_dst = nullptr;
  ImDrawIdx*    idx_dst = nullptr;

  if ( g_pVB->Lock ( 0,
    static_cast <UINT> (draw_data->TotalVtxCount * sizeof CUSTOMVERTEX),
                         (void **)&vtx_dst,
                           D3DLOCK_DISCARD ) < 0 )
    return;

  if ( g_pIB->Lock ( 0,
    static_cast <UINT> (draw_data->TotalIdxCount * sizeof (ImDrawIdx)),
                         (void **)&idx_dst,
                           D3DLOCK_DISCARD ) < 0 )
    return;

  for ( int n = 0;
            n < draw_data->CmdListsCount;
            n++ )
  {
    const ImDrawList* cmd_list = draw_data->CmdLists [n];
    const ImDrawVert* vtx_src  = cmd_list->VtxBuffer.Data;

    for (int i = 0; i < cmd_list->VtxBuffer.Size; i++)
    {
      vtx_dst->pos [0] = vtx_src->pos.x;
      vtx_dst->pos [1] = vtx_src->pos.y;
      vtx_dst->pos [2] = 0.0f;

      ImU32 u32_color =
        (ImU32)ImColor (vtx_src->col);

      u32_color =
        (u32_color  & 0xFF00FF00)      |
        ((u32_color & 0xFF0000) >> 16) |
        ((u32_color & 0xFF)     << 16);

      memcpy (
        &vtx_dst->col, &u32_color, sizeof (uint32_t)); // RGBA --> ARGB for DirectX9

      if (config.imgui.render.disable_alpha)
      {
        ImU32 u32_dst =
          vtx_dst->col;////ImColor (ImVec4 (vtx_dst->col [0], vtx_dst->col [1], vtx_dst->col [2], vtx_dst->col [3]));

        uint8_t alpha =
          (((u32_dst & 0xFF000000U) >> 24U) & 0xFFU);

        // Boost alpha for visibility
        if (alpha <   93 && alpha != 0)
            alpha += (93 - alpha) / 2;

        float a = ((float)                         alpha / 255.0f);
        float r = ((float)((u32_dst & 0xFF0000U) >> 16U) / 255.0f);
        float g = ((float)((u32_dst & 0x00FF00U) >>  8U) / 255.0f);
        float b = ((float)((u32_dst & 0x0000FFU)       ) / 255.0f);

        vtx_dst->col = (
          0xFF000000U |
          ((UINT)((b * a) * 255U) << 16U) |
          ((UINT)((g * a) * 255U) << 8U) |
          ((UINT)((r * a) * 255U))
        );

        //memcpy (
        //  vtx_dst->col, &((unsigned int)im_color), sizeof (uint32_t));
      }

      vtx_dst->uv  [0] = vtx_src->uv.x;
      vtx_dst->uv  [1] = vtx_src->uv.y;
      vtx_dst++;
      vtx_src++;
    }

    memcpy ( idx_dst,
               cmd_list->IdxBuffer.Data,
                 cmd_list->IdxBuffer.Size * sizeof (ImDrawIdx));

    idx_dst += cmd_list->IdxBuffer.Size;
  }

  g_pVB->Unlock ();
  g_pIB->Unlock ();

  g_pd3dDevice->SetStreamSource (0, g_pVB, 0, sizeof CUSTOMVERTEX);
  g_pd3dDevice->SetIndices      (   g_pIB);
  g_pd3dDevice->SetFVF          (D3DFVF_CUSTOMVERTEX);

  // Setup viewport
  D3DVIEWPORT9 vp;

  vp.X = vp.Y = 0;
  vp.Width  = (DWORD)io.DisplaySize.x;
  vp.Height = (DWORD)io.DisplaySize.y;
  vp.MinZ   = 0.0f;
  vp.MaxZ   = 1.0f;
  g_pd3dDevice->SetViewport (&vp);

  if (config.system.log_level > 3)
    dll_log->Log ( L"io.DisplaySize.x = %f, io.DisplaySize.y = %f",
                     io.DisplaySize.x,      io.DisplaySize.y );

  // Setup render state: fixed-pipeline, alpha-blending, no face culling, no depth testing
  g_pd3dDevice->SetPixelShader  (nullptr);
  g_pd3dDevice->SetVertexShader (nullptr);

  D3DCAPS9                      caps = { };
  g_pd3dDevice->GetDeviceCaps (&caps);

  SK_ComPtr <IDirect3DSurface9> pBackBuffer;
  SK_ComPtr <IDirect3DSurface9> rts [8];
  SK_ComPtr <IDirect3DSurface9> pDS;

  for ( UINT target = 0                                       ;
             target < std::min (8UL, caps.NumSimultaneousRTs) ;
             target++ )
    g_pd3dDevice->GetRenderTarget (
             target,
       &rts [target].p
    );

  g_pd3dDevice->GetDepthStencilSurface (&pDS);

  if (SUCCEEDED (g_pd3dDevice->GetBackBuffer (0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer.p)))
  {
    g_pd3dDevice->SetRenderTarget        (0, pBackBuffer);
    g_pd3dDevice->SetDepthStencilSurface (       nullptr);

    for ( UINT target = 1                                       ;
               target < std::min (8UL, caps.NumSimultaneousRTs) ;
               target++ )
      g_pd3dDevice->SetRenderTarget (
               target, nullptr
      );
  }

  g_pd3dDevice->SetRenderState       (D3DRS_CULLMODE,          D3DCULL_NONE);
  g_pd3dDevice->SetRenderState       (D3DRS_LIGHTING,          FALSE);

  g_pd3dDevice->SetRenderState       (D3DRS_ALPHABLENDENABLE,  TRUE);// : FALSE);
  g_pd3dDevice->SetRenderState       (D3DRS_ALPHATESTENABLE,   FALSE);
  g_pd3dDevice->SetRenderState       (D3DRS_BLENDOP,           D3DBLENDOP_ADD);
//  g_pd3dDevice->SetRenderState       (D3DRS_BLENDOPALPHA,      D3DBLENDOP_ADD);
  g_pd3dDevice->SetRenderState       (D3DRS_SRCBLEND,          D3DBLEND_SRCALPHA);
  g_pd3dDevice->SetRenderState       (D3DRS_DESTBLEND,         D3DBLEND_INVSRCALPHA);
  g_pd3dDevice->SetRenderState       (D3DRS_STENCILENABLE,     FALSE);
  g_pd3dDevice->SetRenderState       (D3DRS_SCISSORTESTENABLE, TRUE);
  g_pd3dDevice->SetRenderState       (D3DRS_ZENABLE,           FALSE);

  g_pd3dDevice->SetRenderState       (D3DRS_SRGBWRITEENABLE,   FALSE);
  g_pd3dDevice->SetRenderState       (D3DRS_COLORWRITEENABLE,  D3DCOLORWRITEENABLE_RED   |
                                                               D3DCOLORWRITEENABLE_GREEN |
                                                               D3DCOLORWRITEENABLE_BLUE  |
                                                               D3DCOLORWRITEENABLE_ALPHA );

  g_pd3dDevice->SetTextureStageState   (0, D3DTSS_COLOROP,     D3DTOP_MODULATE);
  g_pd3dDevice->SetTextureStageState   (0, D3DTSS_COLORARG1,   D3DTA_TEXTURE);
  g_pd3dDevice->SetTextureStageState   (0, D3DTSS_COLORARG2,   D3DTA_DIFFUSE);
  g_pd3dDevice->SetTextureStageState   (0, D3DTSS_ALPHAOP,     D3DTOP_MODULATE);
  g_pd3dDevice->SetTextureStageState   (0, D3DTSS_ALPHAARG1,   D3DTA_TEXTURE);
  g_pd3dDevice->SetTextureStageState   (0, D3DTSS_ALPHAARG2,   D3DTA_DIFFUSE);
  g_pd3dDevice->SetSamplerState        (0, D3DSAMP_MINFILTER,  D3DTEXF_LINEAR);
  g_pd3dDevice->SetSamplerState        (0, D3DSAMP_MAGFILTER,  D3DTEXF_LINEAR);
  g_pd3dDevice->SetSamplerState        (0, D3DSAMP_MIPFILTER,  D3DTEXF_LINEAR);
  g_pd3dDevice->SetSamplerState        (0, D3DSAMP_ADDRESSU,   D3DTADDRESS_WRAP);
  g_pd3dDevice->SetSamplerState        (0, D3DSAMP_ADDRESSV,   D3DTADDRESS_WRAP);
  g_pd3dDevice->SetSamplerState        (0, D3DSAMP_ADDRESSW,   D3DTADDRESS_WRAP);

  for (UINT i = 1; i < caps.MaxTextureBlendStages; i++) {
    g_pd3dDevice->SetTextureStageState (i, D3DTSS_COLOROP,     D3DTOP_DISABLE);
    g_pd3dDevice->SetTextureStageState (i, D3DTSS_ALPHAOP,     D3DTOP_DISABLE);
  }

  // Setup orthographic projection matrix
  // Being agnostic of whether <d3dx9.h> or <DirectXMath.h> can be used, we aren't relying on D3DXMatrixIdentity()/D3DXMatrixOrthoOffCenterLH() or DirectX::XMMatrixIdentity()/DirectX::XMMatrixOrthographicOffCenterLH()
  {
    const float L =                    0.5f,
                R = io.DisplaySize.x + 0.5f,
                T =                    0.5f,
                B = io.DisplaySize.y + 0.5f;

    D3DMATRIX mat_identity =
    {
      1.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 1.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 1.0f, 0.0f,
      0.0f, 0.0f, 0.0f, 1.0f
    };

    D3DMATRIX mat_projection =
    {
        2.0f/(R-L),   0.0f,         0.0f,  0.0f,
        0.0f,         2.0f/(T-B),   0.0f,  0.0f,
        0.0f,         0.0f,         0.5f,  0.0f,
        (L+R)/(L-R),  (T+B)/(B-T),  0.5f,  1.0f,
    };

    g_pd3dDevice->SetTransform (D3DTS_WORLD,      &mat_identity);
    g_pd3dDevice->SetTransform (D3DTS_VIEW,       &mat_identity);
    g_pd3dDevice->SetTransform (D3DTS_PROJECTION, &mat_projection);
  }

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
      {
        pcmd->UserCallback (cmd_list, pcmd);
      }

      else
      {
        const RECT r = {
          static_cast <LONG> (pcmd->ClipRect.x), static_cast <LONG> (pcmd->ClipRect.y),
          static_cast <LONG> (pcmd->ClipRect.z), static_cast <LONG> (pcmd->ClipRect.w)
        };

        g_pd3dDevice->SetTexture           ( 0, (LPDIRECT3DTEXTURE9)pcmd->TextureId );
        g_pd3dDevice->SetScissorRect       ( &r );
        g_pd3dDevice->DrawIndexedPrimitive ( D3DPT_TRIANGLELIST,
                                               vtx_offset,
                                                 0,
                               static_cast <UINT> (cmd_list->VtxBuffer.Size),
                                                     idx_offset,
                                                       pcmd->ElemCount / 3 );
      }

      idx_offset += pcmd->ElemCount;
    }

    vtx_offset += cmd_list->VtxBuffer.Size;
  }

  for ( UINT target = 0                                       ;
             target < std::min (8UL, caps.NumSimultaneousRTs) ;
             target++ )
    g_pd3dDevice->SetRenderTarget (
             target,
        rts [target].p
    );

  g_pd3dDevice->SetDepthStencilSurface (pDS);

  //// Restore the DX9 state
  d3d9_state_block->Apply   ();
  d3d9_state_block->Release ();
}

IMGUI_API
bool
ImGui_ImplDX9_Init ( void*                  hwnd,
                     IDirect3DDevice9*      device,
                     D3DPRESENT_PARAMETERS* pparams )
{
  if (device != g_pd3dDevice)
    ImGui_ImplDX9_InvalidateDeviceObjects (pparams);

  g_hWnd       = static_cast <HWND> (hwnd);
  g_pd3dDevice = device;

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
  io.KeyMap [ImGuiKey_Space]      = VK_SPACE;
  io.KeyMap [ImGuiKey_A]          = 'A';
  io.KeyMap [ImGuiKey_C]          = 'C';
  io.KeyMap [ImGuiKey_V]          = 'V';
  io.KeyMap [ImGuiKey_X]          = 'X';
  io.KeyMap [ImGuiKey_Y]          = 'Y';
  io.KeyMap [ImGuiKey_Z]          = 'Z';

  // Alternatively you can set this to NULL and call ImGui::GetDrawData() after ImGui::Render() to get the same ImDrawData pointer.
  io.ImeWindowHandle = g_hWnd;

  float width  = 0.0f,
        height = 0.0f;

  if ( pparams != nullptr )
  {
    width  = static_cast <float> (pparams->BackBufferWidth);
    height = static_cast <float> (pparams->BackBufferHeight);
  }

  io.DisplayFramebufferScale = ImVec2 ( width, height );
  //io.DisplaySize             = ImVec2 ( width, height );


  return true;
}

IMGUI_API
void
ImGui_ImplDX9_Shutdown (void)
{
  ImGui_ImplDX9_InvalidateDeviceObjects ( nullptr );
  ImGui::Shutdown                       (         );

  g_pd3dDevice = nullptr;
  g_hWnd       = 0;
}

IMGUI_API
bool
ImGui_ImplDX9_CreateFontsTexture (void)
{
  SK_TLS* pTLS =
    SK_TLS_Bottom ();

  pTLS->texture_management.injection_thread = TRUE;

  // Build texture atlas
  auto& io =
    ImGui::GetIO ();

  extern void
  SK_ImGui_LoadFonts (void);
  SK_ImGui_LoadFonts (    );

  unsigned char* pixels;
  int            width,
                 height,
                 bytes_per_pixel;

  io.Fonts->GetTexDataAsRGBA32 ( &pixels,
                                   &width, &height,
                                     &bytes_per_pixel );

  // Upload texture to graphics system
  g_FontTexture = nullptr;

  if ( g_pd3dDevice->CreateTexture ( width, height,
                                       1, D3DUSAGE_DYNAMIC,
                                          D3DFMT_A8R8G8B8,
                                          D3DPOOL_DEFAULT,
                                            &g_FontTexture,
                                              nullptr ) < 0 )
  {
    pTLS->texture_management.injection_thread = FALSE;
    return false;
  }

  D3DLOCKED_RECT tex_locked_rect;

  if ( g_FontTexture->LockRect ( 0,       &tex_locked_rect,
                                 nullptr, 0 ) != D3D_OK )
  {
    pTLS->texture_management.injection_thread = FALSE;
    return false;
  }

  for (int y = 0; y < height; y++)
  {
      memcpy ( (unsigned char *)tex_locked_rect.pBits +
                                tex_locked_rect.Pitch * y,
                 pixels + (width * bytes_per_pixel)   * y,
                          (width * bytes_per_pixel) );
  }

  g_FontTexture->UnlockRect (0);

  // Store our identifier
  io.Fonts->TexID =
    static_cast <void *> (g_FontTexture);

  pTLS->texture_management.injection_thread = FALSE;

  return true;
}

bool
ImGui_ImplDX9_CreateDeviceObjects (void)
{
  if (! g_pd3dDevice)
      return false;

  if (! ImGui_ImplDX9_CreateFontsTexture ())
      return false;

  return true;
}

void
ImGui_ImplDX9_InvalidateDeviceObjects (D3DPRESENT_PARAMETERS* pparams)
{
  extern void
  SK_ImGui_ResetExternal (void);
  SK_ImGui_ResetExternal (    );

  if (! g_pd3dDevice)
    return;

  auto& io =
    ImGui::GetIO ();

  if (g_pVB)
  {
    g_pVB->Release ();
    g_pVB = nullptr;
  }

  if (g_pIB)
  {
    g_pIB->Release ();
    g_pIB = nullptr;
  }

  if ( LPDIRECT3DTEXTURE9 tex = (LPDIRECT3DTEXTURE9)io.Fonts->TexID )
  {
    tex->Release ();
    io.Fonts->TexID = nullptr;
  }

  g_FontTexture = nullptr;


  if ( pparams != nullptr )
  {
    float width = static_cast <float> (pparams->BackBufferWidth),
         height = static_cast <float> (pparams->BackBufferHeight);

    io.DisplayFramebufferScale = ImVec2 ( width, height );
    io.DisplaySize             = ImVec2 ( width, height );
  }
}

void
SK_ImGui_PollGamepad (void);

void
ImGui_ImplDX9_NewFrame (void)
{
  auto& io =
    ImGui::GetIO ();

  auto& rb =
    SK_GetCurrentRenderBackend ();

  if (! g_FontTexture)
    ImGui_ImplDX9_CreateDeviceObjects ();

  static HMODULE hModTBFix =
    SK_GetModuleHandle (L"tbfix.dll");

  // Setup display size (every frame to accommodate for window resizing)
  RECT                               rect = { };
  GetClientRect (rb.windows.device, &rect);

  if ( (rect.right  - rect.left) > 0 &&
       (rect.bottom - rect.top)  > 0    )
  {
    io.DisplayFramebufferScale =
      ImVec2 ( static_cast <float> (rect.right  - rect.left),
               static_cast <float> (rect.bottom - rect.top ) );
  }

//dll_log.Log (L"Window Width: %lu, Height: %lu", rect.right  - rect.left, rect.bottom - rect.top);


  if (! g_pd3dDevice)
  {
    dll_log->Log (L"No device!");
    return;
  }


  SK_ComPtr <IDirect3DSwapChain9>                 pSwapChain;
  if (SUCCEEDED (g_pd3dDevice->GetSwapChain ( 0, &pSwapChain.p )))
  {
    D3DPRESENT_PARAMETERS                             pp = { };
    if (SUCCEEDED (pSwapChain->GetPresentParameters (&pp)))
    {
      if ( pp.BackBufferWidth  != 0 &&
           pp.BackBufferHeight != 0    )
      {
        io.DisplaySize.x = static_cast <float> (pp.BackBufferWidth);
        io.DisplaySize.y = static_cast <float> (pp.BackBufferHeight);

        io.DisplayFramebufferScale =
          ImVec2 ( static_cast <float> (pp.BackBufferWidth),
                   static_cast <float> (pp.BackBufferHeight) );
      }

      else
      {
        GetClientRect (pp.hDeviceWindow, &rect);

        if ( (rect.right  - rect.left) > 0 &&
             (rect.bottom - rect.top)  > 0    )
        {
          io.DisplayFramebufferScale =
            ImVec2 ( static_cast <float> (rect.right  - rect.left),
                     static_cast <float> (rect.bottom - rect.top ) );

          io.DisplaySize.x = io.DisplayFramebufferScale.x;
          io.DisplaySize.y = io.DisplayFramebufferScale.y;
        }
      }
    }
  }


  // Setup time step
  INT64 current_time;

  SK_QueryPerformanceCounter (
    reinterpret_cast <LARGE_INTEGER *> (&current_time)
  );

  io.DeltaTime =
    std::min ( 1.0f,
    std::max ( 0.0f, static_cast <float>  (
                    (static_cast <double> (                       current_time) -
                     static_cast <double> (std::exchange (g_Time, current_time))) /
                     static_cast <double> (               g_TicksPerSecond      ) ) )
    );

  // Read keyboard modifiers inputs
  io.KeyCtrl   = (io.KeysDown [VK_CONTROL]) != 0;
  io.KeyShift  = (io.KeysDown [VK_SHIFT])   != 0;
  io.KeyAlt    = (io.KeysDown [VK_MENU])    != 0;

  io.KeySuper  = false;

  SK_ImGui_PollGamepad ();


  // For games that hijack the mouse cursor using DirectInput 8.
  //
  //  -- Acquire actually means release their exclusive ownership : )
  //
  //if (SK_ImGui_WantMouseCapture ())
  //  SK_Input_DI8Mouse_Acquire ();
  //else
  //  SK_Input_DI8Mouse_Release ();


  // Start the frame
  SK_ImGui_User_NewFrame ();
}