// ImGui Win32 + DirectX9 binding
// In this binding, ImTextureID is used to store a 'LPDIRECT3DTEXTURE9' texture identifier. Read the FAQ about ImTextureID in imgui.cpp.

// You can copy and use unmodified imgui_impl_* files in your project. See main.cpp for an example of using this.
// If you use this binding you'll need to call 4 functions: ImGui_ImplXXXX_Init(), ImGui_ImplXXXX_NewFrame(), ImGui::Render() and ImGui_ImplXXXX_Shutdown().
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// https://github.com/ocornut/imgui

#define NOMINMAX

#include <imgui/imgui.h>
#include <imgui/backends/imgui_d3d9.h>

// DirectX
#include <d3d9.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

//#include "config.h"
//#include "render.h"

#include <SpecialK/window.h>

#include <atlbase.h>

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
  float    pos [3];
  D3DCOLOR col;
  float    uv  [2];
};
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1)

// This is the main rendering function that you have to implement and provide to ImGui (via setting up 'RenderDrawListsFn' in the ImGuiIO structure)
// If text or lines are blurry when integrating ImGui in your engine:
// - in your Render function, try translating your projection matrix by (0.5f,0.5f) or (0.375f,0.375f)
void
ImGui_ImplDX9_RenderDrawLists (ImDrawData* draw_data)
{
  // Avoid rendering when minimized
  ImGuiIO& io =
    ImGui::GetIO ();

  if ( io.DisplaySize.x <= 0.0f || io.DisplaySize.y <= 0.0f )
    return;

  // Create and grow buffers if needed
  if ((! g_pVB) || g_VertexBufferSize < draw_data->TotalVtxCount )
  {
    if (g_pVB) {
      g_pVB->Release ();
      g_pVB = nullptr;
    }

    g_VertexBufferSize =
      draw_data->TotalVtxCount + 5000;

    if ( g_pd3dDevice->CreateVertexBuffer ( g_VertexBufferSize * sizeof CUSTOMVERTEX,
                                              D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
                                                D3DFVF_CUSTOMVERTEX,
                                                  D3DPOOL_DEFAULT,
                                                    &g_pVB,
                                                      nullptr ) < 0 ) {
      return;
    }
  }

  if ((! g_pIB) || g_IndexBufferSize < draw_data->TotalIdxCount)
  {
    if (g_pIB) {
      g_pIB->Release ();
      g_pIB = NULL;
    }

    g_IndexBufferSize = draw_data->TotalIdxCount + 10000;

    if ( g_pd3dDevice->CreateIndexBuffer ( g_IndexBufferSize * sizeof ImDrawIdx,
                                             D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
                                               sizeof (ImDrawIdx) == 2 ?
                                                  D3DFMT_INDEX16 :
                                                  D3DFMT_INDEX32,
                                                D3DPOOL_DEFAULT,
                                                  &g_pIB,
                                                    nullptr ) < 0 ) {
      return;
    }
  }

  // Backup the DX9 state
  IDirect3DStateBlock9* d3d9_state_block = nullptr;

  if (g_pd3dDevice->CreateStateBlock ( D3DSBT_ALL, &d3d9_state_block ) < 0 )
    return;

  // Copy and convert all vertices into a single contiguous buffer
  CUSTOMVERTEX* vtx_dst;
  ImDrawIdx*    idx_dst;

  if ( g_pVB->Lock ( 0,
                       (UINT)(draw_data->TotalVtxCount * sizeof CUSTOMVERTEX),
                         (void **)&vtx_dst,
                           D3DLOCK_DISCARD ) < 0 )
    return;

  if ( g_pIB->Lock ( 0,
                       (UINT)(draw_data->TotalIdxCount * sizeof ImDrawIdx),
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
      vtx_dst->col     = (vtx_src->col & 0xFF00FF00)      |
                        ((vtx_src->col & 0xFF0000) >> 16) |
                        ((vtx_src->col & 0xFF)     << 16);     // RGBA --> ARGB for DirectX9
      vtx_dst->uv  [0] = vtx_src->uv.x;
      vtx_dst->uv  [1] = vtx_src->uv.y;
      vtx_dst++;
      vtx_src++;
    }

    memcpy ( idx_dst,
               cmd_list->IdxBuffer.Data,
                 cmd_list->IdxBuffer.Size * sizeof ImDrawIdx);

    idx_dst += cmd_list->IdxBuffer.Size;
  }

  g_pVB->Unlock ();
  g_pIB->Unlock ();

  g_pd3dDevice->SetStreamSource (0, g_pVB, 0, sizeof CUSTOMVERTEX);
  g_pd3dDevice->SetIndices      (g_pIB);
  g_pd3dDevice->SetFVF          (D3DFVF_CUSTOMVERTEX);

  // Setup viewport
  D3DVIEWPORT9 vp;

  vp.X = vp.Y = 0;
  vp.Width  = (DWORD)io.DisplaySize.x;
  vp.Height = (DWORD)io.DisplaySize.y;
  vp.MinZ   = 0.0f;
  vp.MaxZ   = 1.0f;
  g_pd3dDevice->SetViewport (&vp);

  // Setup render state: fixed-pipeline, alpha-blending, no face culling, no depth testing
  g_pd3dDevice->SetPixelShader       (NULL);
  g_pd3dDevice->SetVertexShader      (NULL);

  D3DCAPS9                      caps;
  g_pd3dDevice->GetDeviceCaps (&caps);

  g_pd3dDevice->SetRenderState       (D3DRS_CULLMODE,          D3DCULL_NONE);
  g_pd3dDevice->SetRenderState       (D3DRS_LIGHTING,          FALSE);
  g_pd3dDevice->SetRenderState       (D3DRS_ZENABLE,           TRUE);
  g_pd3dDevice->SetRenderState       (D3DRS_ALPHABLENDENABLE,  TRUE);
  g_pd3dDevice->SetRenderState       (D3DRS_ALPHATESTENABLE,   FALSE);
  g_pd3dDevice->SetRenderState       (D3DRS_BLENDOP,           D3DBLENDOP_ADD);
  g_pd3dDevice->SetRenderState       (D3DRS_SRCBLEND,          D3DBLEND_SRCALPHA);
  g_pd3dDevice->SetRenderState       (D3DRS_DESTBLEND,         D3DBLEND_INVSRCALPHA);
  g_pd3dDevice->SetRenderState       (D3DRS_SCISSORTESTENABLE, TRUE);
  g_pd3dDevice->SetRenderState       (D3DRS_ZENABLE,           FALSE);

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

  for (UINT i = 1; i < caps.MaxTextureBlendStages; i++) {
    g_pd3dDevice->SetTextureStageState (i, D3DTSS_COLOROP,     D3DTOP_DISABLE);
    g_pd3dDevice->SetTextureStageState (i, D3DTSS_ALPHAOP,     D3DTOP_DISABLE);
  }

  // Setup orthographic projection matrix
  // Being agnostic of whether <d3dx9.h> or <DirectXMath.h> can be used, we aren't relying on D3DXMatrixIdentity()/D3DXMatrixOrthoOffCenterLH() or DirectX::XMMatrixIdentity()/DirectX::XMMatrixOrthographicOffCenterLH()
  {
    const float L = 0.5f,
                R = io.DisplaySize.x + 0.5f,
                T = 0.5f,
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
          (LONG)pcmd->ClipRect.x, (LONG)pcmd->ClipRect.y,
          (LONG)pcmd->ClipRect.z, (LONG)pcmd->ClipRect.w
        };

        g_pd3dDevice->SetTexture           ( 0, (LPDIRECT3DTEXTURE9)pcmd->TextureId );
        g_pd3dDevice->SetScissorRect       ( &r );
        g_pd3dDevice->DrawIndexedPrimitive ( D3DPT_TRIANGLELIST,
                                               vtx_offset,
                                                 0,
                                                   (UINT)cmd_list->VtxBuffer.Size,
                                                     idx_offset,
                                                       pcmd->ElemCount / 3 );
      }

      idx_offset += pcmd->ElemCount;
    }

    vtx_offset += cmd_list->VtxBuffer.Size;
  }

  // Restore the DX9 state
  d3d9_state_block->Apply   ();
  d3d9_state_block->Release ();
}

IMGUI_API
bool
ImGui_ImplDX9_Init (void* hwnd, IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* pparams)
{
  g_hWnd       = (HWND)hwnd;
  g_pd3dDevice = device;

  if (! QueryPerformanceFrequency ((LARGE_INTEGER *)&g_TicksPerSecond))
    return false;

  if (! QueryPerformanceCounter   ((LARGE_INTEGER *)&g_Time))
    return false;

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
  io.RenderDrawListsFn = ImGui_ImplDX9_RenderDrawLists;
  io.ImeWindowHandle   = g_hWnd;


  float width = 0.0f, height = 0.0f;

  if ( pparams != nullptr )
  {
    width  = (float)pparams->BackBufferWidth;
    height = (float)pparams->BackBufferHeight;
  }

  ImGui::GetIO ().DisplayFramebufferScale = ImVec2 ( width, height );
  //ImGui::GetIO ().DisplaySize             = ImVec2 ( width, height );


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
  // Build texture atlas
  ImGuiIO& io =
    ImGui::GetIO ();

  extern void
  SK_ImGui_LoadFonts (void);

  SK_ImGui_LoadFonts ();

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
    return false;

  D3DLOCKED_RECT tex_locked_rect;

  if ( g_FontTexture->LockRect ( 0,       &tex_locked_rect,
                                 nullptr, 0 ) != D3D_OK )
    return false;

  for (int y = 0; y < height; y++) {
      memcpy ( (unsigned char *)tex_locked_rect.pBits + tex_locked_rect.Pitch * y,
                 pixels + (width * bytes_per_pixel) * y,
                   (width * bytes_per_pixel) );
  }

  g_FontTexture->UnlockRect (0);

  // Store our identifier
  io.Fonts->TexID =
    (void *)g_FontTexture;

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
  if (! g_pd3dDevice)
    return;

  if (g_pVB)
  {
    g_pVB->Release ();
    g_pVB = NULL;
  }

  if (g_pIB)
  {
    g_pIB->Release ();
    g_pIB = NULL;
  }

  if ( LPDIRECT3DTEXTURE9 tex = (LPDIRECT3DTEXTURE9)ImGui::GetIO ().Fonts->TexID )
  {
    tex->Release ();
    ImGui::GetIO ().Fonts->TexID = 0;
  }

  g_FontTexture = NULL;


  float width = 0.0f, height = 0.0f;

  if ( pparams != nullptr )
  {
    width  = (float)pparams->BackBufferWidth;
    height = (float)pparams->BackBufferHeight;
  }

  ImGui::GetIO ().DisplayFramebufferScale = ImVec2 ( width, height );
  ImGui::GetIO ().DisplaySize             = ImVec2 ( width, height );
}

#include <SpecialK/window.h>

#include <windowsx.h>
#include <SpecialK/hooks.h>

#define XINPUT_GAMEPAD_DPAD_UP        0x0001
#define XINPUT_GAMEPAD_DPAD_DOWN      0x0002
#define XINPUT_GAMEPAD_DPAD_LEFT      0x0004
#define XINPUT_GAMEPAD_DPAD_RIGHT     0x0008

#define XINPUT_GAMEPAD_START          0x0010
#define XINPUT_GAMEPAD_BACK           0x0020
#define XINPUT_GAMEPAD_LEFT_THUMB     0x0040
#define XINPUT_GAMEPAD_RIGHT_THUMB    0x0080

#define XINPUT_GAMEPAD_LEFT_SHOULDER  0x0100
#define XINPUT_GAMEPAD_RIGHT_SHOULDER 0x0200
#define XINPUT_GAMEPAD_LEFT_TRIGGER   0x10000
#define XINPUT_GAMEPAD_RIGHT_TRIGGER  0x20000

#define XINPUT_GAMEPAD_A              0x1000
#define XINPUT_GAMEPAD_B              0x2000
#define XINPUT_GAMEPAD_X              0x4000
#define XINPUT_GAMEPAD_Y              0x8000

typedef struct _XINPUT_GAMEPAD {
  WORD  wButtons;
  BYTE  bLeftTrigger;
  BYTE  bRightTrigger;
  SHORT sThumbLX;
  SHORT sThumbLY;
  SHORT sThumbRX;
  SHORT sThumbRY;
} XINPUT_GAMEPAD, *PXINPUT_GAMEPAD;

typedef struct _XINPUT_STATE {
  DWORD          dwPacketNumber;
  XINPUT_GAMEPAD Gamepad;
} XINPUT_STATE, *PXINPUT_STATE;

typedef DWORD (WINAPI *XInputGetState_pfn)(
  _In_  DWORD        dwUserIndex,
  _Out_ XINPUT_STATE *pState
);

static XInputGetState_pfn XInputGetState1_3_Original   = nullptr;
static XInputGetState_pfn XInputGetState1_4_Original   = nullptr;
static XInputGetState_pfn XInputGetState9_1_0_Original = nullptr;
static XInputGetState_pfn XInputGetState_Original      = nullptr;

bool  nav_usable       = false;

extern bool SK_ImGui_Visible;

DWORD
WINAPI
XInputGetState1_3_Detour (
  _In_  DWORD         dwUserIndex,
  _Out_ XINPUT_STATE *pState )
{
  DWORD dwRet = XInputGetState1_3_Original (dwUserIndex, pState);

  if (nav_usable)
    ZeroMemory (pState, sizeof XINPUT_STATE);

  return dwRet;
}

DWORD
WINAPI
XInputGetState1_4_Detour (
  _In_  DWORD         dwUserIndex,
  _Out_ XINPUT_STATE *pState )
{
  DWORD dwRet = XInputGetState1_4_Original (dwUserIndex, pState);

  if (nav_usable)
    ZeroMemory (pState, sizeof XINPUT_STATE);

  return dwRet;
}

DWORD
WINAPI
XInputGetState9_1_0_Detour (
  _In_  DWORD         dwUserIndex,
  _Out_ XINPUT_STATE *pState )
{
  DWORD dwRet = XInputGetState9_1_0_Original (dwUserIndex, pState);

  if (nav_usable)
    ZeroMemory (pState, sizeof XINPUT_STATE);

  return dwRet;
}

bool
SK_IsControllerPluggedIn (INT iJoyID)
{
  if (iJoyID == -1)
    return true;

  XINPUT_STATE xstate;

  static DWORD last_poll = timeGetTime ();
  static DWORD dwRet     = XInputGetState_Original (iJoyID, &xstate);

  // This function is actually a performance hazzard when no controllers
  //   are plugged in, so ... throttle the sucker.
  if (last_poll < timeGetTime () - 500UL)
    dwRet = XInputGetState_Original (iJoyID, &xstate);

  if (dwRet == ERROR_DEVICE_NOT_CONNECTED)
    return false;

  return true;
}

#include <algorithm>

float analog_sensitivity = 333.33f;

#include <SpecialK/utility.h>
#include <SpecialK/log.h>
#include <dinput.h>
#include <comdef.h>

#define DINPUT8_CALL(_Ret, _Call) {                                     \
  dll_log.LogEx (true, L"[   Input  ]  Calling original function: ");   \
  (_Ret) = (_Call);                                                     \
  _com_error err ((_Ret));                                              \
  if ((_Ret) != S_OK)                                                   \
    dll_log.LogEx (false, L"(ret=0x%04x - %s)\n", err.WCode (),         \
                                                  err.ErrorMessage ()); \
  else                                                                  \
    dll_log.LogEx (false, L"(ret=S_OK)\n");                             \
}

///////////////////////////////////////////////////////////////////////////////
//
// DirectInput 8
//
///////////////////////////////////////////////////////////////////////////////
typedef HRESULT (WINAPI *IDirectInput8_CreateDevice_pfn)(
  IDirectInput8       *This,
  REFGUID              rguid,
  LPDIRECTINPUTDEVICE *lplpDirectInputDevice,
  LPUNKNOWN            pUnkOuter
);

typedef HRESULT (WINAPI *IDirectInputDevice8_GetDeviceState_pfn)(
  LPDIRECTINPUTDEVICE  This,
  DWORD                cbData,
  LPVOID               lpvData
);

typedef HRESULT (WINAPI *IDirectInputDevice8_SetCooperativeLevel_pfn)(
  LPDIRECTINPUTDEVICE  This,
  HWND                 hwnd,
  DWORD                dwFlags
);

IDirectInput8_CreateDevice_pfn
        IDirectInput8_CreateDevice_Original              = nullptr;
IDirectInputDevice8_GetDeviceState_pfn
        IDirectInputDevice8_GetDeviceState_Original      = nullptr;
IDirectInputDevice8_SetCooperativeLevel_pfn
        IDirectInputDevice8_SetCooperativeLevel_Original = nullptr;

struct di8_keyboard_s {
  LPDIRECTINPUTDEVICE pDev = nullptr;
  uint8_t             state [512];
} _dik;

struct di8_mouse_s {
  LPDIRECTINPUTDEVICE pDev = nullptr;
  DIMOUSESTATE2       state;
} _dim;

HRESULT
WINAPI
IDirectInputDevice8_GetDeviceState_Detour ( LPDIRECTINPUTDEVICE        This,
                                            DWORD                      cbData,
                                            LPVOID                     lpvData )
{
  //dll_log.Log (L"GetDeviceState");

  HRESULT hr;
  hr = IDirectInputDevice8_GetDeviceState_Original ( This,
                                                       cbData,
                                                         lpvData );

  if (SUCCEEDED (hr) && lpvData != nullptr)
  {
    if (cbData == sizeof DIJOYSTATE2) 
    {
      //dll_log.Log (L"Joy2");

      DIJOYSTATE2* out = (DIJOYSTATE2 *)lpvData;

      if (nav_usable)
      {
        memset (out, 0, sizeof DIJOYSTATE2);

        out->rgdwPOV [0] = -1;
        out->rgdwPOV [1] = -1;
        out->rgdwPOV [2] = -1;
        out->rgdwPOV [3] = -1;
      }

      else {
#if 0
        DIJOYSTATE2  in  = *out;

        for (int i = 0; i < 12; i++) {
          // Negative values are for axes, we cannot remap those yet
          if (gamepad.remap.map [ i ] >= 0) {
            out->rgbButtons [ i ] = 
              in.rgbButtons [ gamepad.remap.map [ i ] ];
          }
        }
#endif
      }
    }

    else if (cbData == sizeof DIJOYSTATE) 
    {
      //dll_log.Log (L"Joy");

      DIJOYSTATE* out = (DIJOYSTATE *)lpvData;

      if (nav_usable)
      {
        memset (out, 0, sizeof DIJOYSTATE);

        out->rgdwPOV [0] = -1;
        out->rgdwPOV [1] = -1;
        out->rgdwPOV [2] = -1;
        out->rgdwPOV [3] = -1;
      }
    }

    else if ( cbData == sizeof (DIMOUSESTATE2) ||
              cbData == sizeof (DIMOUSESTATE)  )
    {
      //dll_log.Log (L"Mouse");

      extern bool ImGui_WantMouseCapture (void);

      if (ImGui_WantMouseCapture ())
      {
        switch (cbData)
        {
          case sizeof (DIMOUSESTATE2):
            ((DIMOUSESTATE2 *)lpvData)->lX = 0;
            ((DIMOUSESTATE2 *)lpvData)->lY = 0;
            ((DIMOUSESTATE2 *)lpvData)->lZ = 0;
            memset (((DIMOUSESTATE2 *)lpvData)->rgbButtons, 0, 8);
            break;

          case sizeof (DIMOUSESTATE):
            ((DIMOUSESTATE *)lpvData)->lX = 0;
            ((DIMOUSESTATE *)lpvData)->lY = 0;
            ((DIMOUSESTATE *)lpvData)->lZ = 0;
            memset (((DIMOUSESTATE *)lpvData)->rgbButtons, 0, 4);
            break;
        }
      }
    }
  }

  return hr;
}

HRESULT
WINAPI
IDirectInputDevice8_SetCooperativeLevel_Detour ( LPDIRECTINPUTDEVICE  This,
                                                 HWND                 hwnd,
                                                 DWORD                dwFlags )
{
  //if (config.input.block_windows)
    //dwFlags |= DISCL_NOWINKEY;

#if 0
  if (config.render.allow_background) {
    dwFlags &= ~DISCL_EXCLUSIVE;
    dwFlags &= ~DISCL_BACKGROUND;

    dwFlags |= DISCL_NONEXCLUSIVE;
    dwFlags |= DISCL_FOREGROUND;

    return IDirectInputDevice8_SetCooperativeLevel_Original (This, hwnd, dwFlags);
  }
#endif

  return IDirectInputDevice8_SetCooperativeLevel_Original (This, hwnd, dwFlags);
}

HRESULT
WINAPI
IDirectInput8_CreateDevice_Detour ( IDirectInput8       *This,
                                    REFGUID              rguid,
                                    LPDIRECTINPUTDEVICE *lplpDirectInputDevice,
                                    LPUNKNOWN            pUnkOuter )
{
  const wchar_t* wszDevice = (rguid == GUID_SysKeyboard)   ? L"Default System Keyboard" :
                                (rguid == GUID_SysMouse)   ? L"Default System Mouse"    :  
                                  (rguid == GUID_Joystick) ? L"Gamepad / Joystick"      :
                                                           L"Other Device";

  dll_log.Log ( L"[   Input  ][!] IDirectInput8::CreateDevice (%08Xh, %s, %08Xh, %08Xh)",
                   This,
                     wszDevice,
                       lplpDirectInputDevice,
                         pUnkOuter );

  HRESULT hr;
  DINPUT8_CALL ( hr,
                  IDirectInput8_CreateDevice_Original ( This,
                                                         rguid,
                                                          lplpDirectInputDevice,
                                                           pUnkOuter ) );

  static bool hooked = false;

  if (SUCCEEDED (hr) && (! hooked))
  {
    if (! hooked)
    {
      hooked = true;
      void** vftable = *(void***)*lplpDirectInputDevice;

      SK_CreateFuncHook ( L"IDirectInputDevice8::GetDeviceState",
                           vftable [9],
                           IDirectInputDevice8_GetDeviceState_Detour,
                (LPVOID *)&IDirectInputDevice8_GetDeviceState_Original );

      MH_QueueEnableHook (vftable [9]);

      SK_CreateFuncHook ( L"IDirectInputDevice8::SetCooperativeLevel",
                           vftable [13],
                           IDirectInputDevice8_SetCooperativeLevel_Detour,
                 (LPVOID*)&IDirectInputDevice8_SetCooperativeLevel_Original );

      MH_QueueEnableHook (vftable [13]);

      MH_ApplyQueued ();
    }

    if (rguid == GUID_SysMouse)
      _dim.pDev = *lplpDirectInputDevice;
    else if (rguid == GUID_SysKeyboard)
      _dik.pDev = *lplpDirectInputDevice;
  }

#if 0
  if (SUCCEEDED (hr) && lplpDirectInputDevice != nullptr) {
    DWORD dwFlag = DISCL_FOREGROUND | DISCL_NONEXCLUSIVE;

    if (config.input.block_windows)
      dwFlag |= DISCL_NOWINKEY;

    (*lplpDirectInputDevice)->SetCooperativeLevel (SK_GetGameWindow (), dwFlag);
  }
#endif

  return hr;
}

typedef HRESULT (WINAPI *DirectInput8Create_pfn)(
 HINSTANCE hinst,
 DWORD     dwVersion,
 REFIID    riidltf,
 LPVOID*   ppvOut,
 LPUNKNOWN punkOuter
);

DirectInput8Create_pfn DirectInput8Create_Original = nullptr;

HRESULT
WINAPI
DirectInput8Create_Detour (
  HINSTANCE hinst,
  DWORD     dwVersion,
  REFIID    riidltf,
  LPVOID*   ppvOut,
  LPUNKNOWN punkOuter
)
{
  HRESULT hr = E_NOINTERFACE;

  if ( SUCCEEDED (
         (hr = DirectInput8Create_Original (hinst, dwVersion, riidltf, ppvOut, punkOuter))
       )
     )
  {
    if (! IDirectInput8_CreateDevice_Original)
    {
      void** vftable = *(void***)*ppvOut;
      
      SK_CreateFuncHook ( L"IDirectInput8::CreateDevice",
                           vftable [3],
                           IDirectInput8_CreateDevice_Detour,
                 (LPVOID*)&IDirectInput8_CreateDevice_Original );
      
      SK_EnableHook (vftable [3]);
    }
  }

  return hr;
}

void
SK_ImGui_InitDirectInput (void)
{
  SK_CreateDLLHook ( L"dinput8.dll",
                      "DirectInput8Create",
                      DirectInput8Create_Detour,
           (LPVOID *)&DirectInput8Create_Original );
}

void
SK_ImGui_PollGamepad1 (void)
{
  if (XInputGetState_Original == nullptr)
  {
    static sk_import_test_s tests [] = { { "XInput1_3.dll",   false },
                                         { "XInput1_4.dll",   false },
                                         { "XInput9_1_0.dll", false } };

    SK_TestImports (GetModuleHandle (nullptr), tests, 3);

    if (tests [0].used)
    {
      SK_CreateDLLHook3 ( L"XInput1_3.dll",
                           "XInputGetState",
                           XInputGetState1_3_Detour,
                (LPVOID *)&XInputGetState1_3_Original );
      XInputGetState_Original = XInputGetState1_3_Original;
    }

    else if (tests [1].used)
    {
      SK_CreateDLLHook3 ( L"XInput1_4.dll",
                           "XInputGetState",
                           XInputGetState1_4_Detour,
                (LPVOID *)&XInputGetState1_4_Original );
      XInputGetState_Original = XInputGetState1_4_Original;
    }

    else if (tests [2].used)
    {
      SK_CreateDLLHook3 ( L"XInput9_1_0.dll",
                           "XInputGetState",
                           XInputGetState9_1_0_Detour,
                (LPVOID *)&XInputGetState9_1_0_Original );
      XInputGetState_Original = XInputGetState9_1_0_Original;
    }

    else
    {
      if (GetModuleHandle (L"XInput1_3.dll"))
      {
        SK_CreateDLLHook3 ( L"XInput1_3.dll",
                             "XInputGetState",
                             XInputGetState1_3_Detour,
                  (LPVOID *)&XInputGetState1_3_Original );
        XInputGetState_Original = XInputGetState1_3_Original;
      }

      else if (GetModuleHandle (L"XInput1_4.dll"))
      {
        SK_CreateDLLHook3 ( L"XInput1_4.dll",
                             "XInputGetState",
                             XInputGetState1_4_Detour,
                  (LPVOID *)&XInputGetState1_4_Original );
        XInputGetState_Original = XInputGetState1_4_Original;
      }

      else if (GetModuleHandle (L"XInput9_1_0.dll"))
      {
        SK_CreateDLLHook3 ( L"XInput9_1_0.dll",
                             "XInputGetState",
                             XInputGetState9_1_0_Detour,
                  (LPVOID *)&XInputGetState9_1_0_Original );
        XInputGetState_Original = XInputGetState9_1_0_Original;
      }

      else
      {
        SK_CreateDLLHook3 ( L"XInput1_3.dll",
                             "XInputGetState",
                             XInputGetState1_3_Detour,
                  (LPVOID *)&XInputGetState1_3_Original );
        XInputGetState_Original = XInputGetState1_3_Original;
      }
    }

    SK_CreateDLLHook3 ( L"XInput1_3.dll",
                         "XInputGetState",
                         XInputGetState1_3_Detour,
              (LPVOID *)&XInputGetState1_3_Original );

    SK_CreateDLLHook3 ( L"XInput1_4.dll",
                         "XInputGetState",
                         XInputGetState1_4_Detour,
              (LPVOID *)&XInputGetState1_4_Original );

    SK_CreateDLLHook3 ( L"XInput9_1_0.dll",
                         "XInputGetState",
                         XInputGetState9_1_0_Detour,
              (LPVOID *)&XInputGetState9_1_0_Original );

    XInputGetState_Original = XInputGetState1_3_Original;

    MH_ApplyQueued ();
  }

  XINPUT_STATE state;
    static XINPUT_STATE last_state = { 0 };

  extern void
  SK_ImGui_Toggle (void);

  if (SK_IsControllerPluggedIn (0))
  {
    XInputGetState_Original (0, &state);
    
    if ( state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK &&
         state.Gamepad.wButtons & XINPUT_GAMEPAD_START )
    {
      if (! ( last_state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK &&
              last_state.Gamepad.wButtons & XINPUT_GAMEPAD_START ) )
      {
        SK_ImGui_Toggle ();
    
        nav_usable = SK_ImGui_Visible;
    
        if (nav_usable) ImGui::SetNextWindowFocus ();
      }
    }

    //if (SK_ImGui_Visible)
    //{
       const DWORD LONG_PRESS  = 400UL;
      static DWORD dwLastPress = MAXDWORD;

      if ( (     state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) &&
           (last_state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) )
      {
        if (dwLastPress < timeGetTime () - LONG_PRESS)
        {
          if (! SK_ImGui_Visible)
            SK_ImGui_Toggle ();

          nav_usable  = (! nav_usable);
          dwLastPress = MAXDWORD;
        }
      }

      else if (state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK)
        dwLastPress = timeGetTime ();

      else
        dwLastPress = MAXDWORD;
  }

  else
    ZeroMemory (&state, sizeof XINPUT_STATE);

  last_state = state;
}

void
SK_ImGui_PollGamepad (void)
{
  ImGuiIO& io =
    ImGui::GetIO ();

  // io.KeysDown : filled by WM_KEYDOWN/WM_KEYUP events
  // io.MousePos : filled by WM_MOUSEMOVE events
  // io.MouseDown : filled by WM_*BUTTON* events
  // io.MouseWheel : filled by WM_MOUSEWHEEL events

  XINPUT_STATE state;
    static XINPUT_STATE last_state = { 0 };

  for (int i = 0; i < ImGuiNavInput_COUNT; i++)
    io.NavInputs [i] = 0.0f;

  if (SK_IsControllerPluggedIn (0))
  {
    XInputGetState_Original (0, &state);

    if (nav_usable)
    {
      io.NavInputs [ImGuiNavInput_PadActivate]    += state.Gamepad.wButtons & XINPUT_GAMEPAD_A;            // press button, tweak value                    // e.g. Circle button
      io.NavInputs [ImGuiNavInput_PadCancel]      += state.Gamepad.wButtons & XINPUT_GAMEPAD_B;            // close menu/popup/child, lose selection       // e.g. Cross button
      io.NavInputs [ImGuiNavInput_PadInput]       += state.Gamepad.wButtons & XINPUT_GAMEPAD_Y;            // text input                                   // e.g. Triangle button
      io.NavInputs [ImGuiNavInput_PadMenu]        += state.Gamepad.wButtons & XINPUT_GAMEPAD_X;            // access menu, focus, move, resize             // e.g. Square button
      io.NavInputs [ImGuiNavInput_PadUp]          += state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP;      // move up, resize window (with PadMenu held)   // e.g. D-pad up/down/left/right, analog
      io.NavInputs [ImGuiNavInput_PadDown]        += state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN;    // move down
      io.NavInputs [ImGuiNavInput_PadLeft]        += state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT;    // move left
      io.NavInputs [ImGuiNavInput_PadRight]       += state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;   // move right

      io.NavInputs [ImGuiNavInput_PadScrollUp]    += std::max (0.0f, (float)state.Gamepad.sThumbLY /  32767.0f) / analog_sensitivity;
      io.NavInputs [ImGuiNavInput_PadScrollDown]  += std::max (0.0f, (float)state.Gamepad.sThumbLY / -32768.0f) / analog_sensitivity;
      io.NavInputs [ImGuiNavInput_PadScrollLeft]  += std::max (0.0f, (float)state.Gamepad.sThumbLX / -32768.0f) / analog_sensitivity;
      io.NavInputs [ImGuiNavInput_PadScrollRight] += std::max (0.0f, (float)state.Gamepad.sThumbLX /  32767.0f) / analog_sensitivity;

      io.NavInputs [ImGuiNavInput_PadFocusPrev]   += state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;   // next window (with PadMenu held)              // e.g. L-trigger
      io.NavInputs [ImGuiNavInput_PadFocusNext]   += state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;   // prev window (with PadMenu held)              // e.g. R-trigger
    }
  }


  if (io.KeysDown [VK_CAPITAL] && io.KeysDownDuration [VK_CAPITAL] == 0.0f)
  {
    nav_usable = (! nav_usable);

    if (nav_usable) ImGui::SetNextWindowFocus ();

    io.NavActive = false;
  }


  if (! nav_usable)
    io.NavActive = false;

  if (io.NavUsable && nav_usable)
  {
    //io.NavInputs [ImGuiNavInput_PadTweakSlow]     // slower tweaks                                // e.g. L-trigger, analog
    //io.NavInputs [ImGuiNavInput_PadTweakFast]     // faster tweaks                                // e.g. R-trigger, analog

    io.NavInputs [ImGuiNavInput_PadScrollUp]    += (1.0f / analog_sensitivity) * (io.KeysDown [VK_UP]    ? 1.0f : 0.0f);
    io.NavInputs [ImGuiNavInput_PadScrollDown]  += (1.0f / analog_sensitivity) * (io.KeysDown [VK_DOWN]  ? 1.0f : 0.0f);
    io.NavInputs [ImGuiNavInput_PadScrollLeft]  += (1.0f / analog_sensitivity) * (io.KeysDown [VK_LEFT]  ? 1.0f : 0.0f);
    io.NavInputs [ImGuiNavInput_PadScrollRight] += (1.0f / analog_sensitivity) * (io.KeysDown [VK_RIGHT] ? 1.0f : 0.0f);

    io.NavInputs [ImGuiNavInput_PadScrollUp]    += (1.0f / analog_sensitivity) * (io.KeysDown ['W'] ? 1.0f : 0.0f);
    io.NavInputs [ImGuiNavInput_PadScrollDown]  += (1.0f / analog_sensitivity) * (io.KeysDown ['S'] ? 1.0f : 0.0f);
    io.NavInputs [ImGuiNavInput_PadScrollLeft]  += (1.0f / analog_sensitivity) * (io.KeysDown ['A'] ? 1.0f : 0.0f);
    io.NavInputs [ImGuiNavInput_PadScrollRight] += (1.0f / analog_sensitivity) * (io.KeysDown ['D'] ? 1.0f : 0.0f);

    io.NavInputs [ImGuiNavInput_PadMenu]       += io.KeyAlt ? 1.0f : 0.0f;   // access menu, focus, move, resize             // e.g. Square button

    if (! io.WantTextInput)
    {
      io.NavInputs [ImGuiNavInput_PadUp]       += io.KeysDown [VK_UP]    ? 1.0f : 0.0f; // move up, resize window (with PadMenu held)   // e.g. D-pad up/down/left/right, analog
      io.NavInputs [ImGuiNavInput_PadDown]     += io.KeysDown [VK_DOWN]  ? 1.0f : 0.0f; // move down
      io.NavInputs [ImGuiNavInput_PadLeft]     += io.KeysDown [VK_LEFT]  ? 1.0f : 0.0f; // move left
      io.NavInputs [ImGuiNavInput_PadRight]    += io.KeysDown [VK_RIGHT] ? 1.0f : 0.0f; // move right

      io.NavInputs [ImGuiNavInput_PadUp]       += io.KeysDown ['W'] ? 1.0f : 0.0f;  // move up, resize window (with PadMenu held)   // e.g. D-pad up/down/left/right, analog
      io.NavInputs [ImGuiNavInput_PadDown]     += io.KeysDown ['S'] ? 1.0f : 0.0f;  // move down
      io.NavInputs [ImGuiNavInput_PadLeft]     += io.KeysDown ['A'] ? 1.0f : 0.0f;  // move left
      io.NavInputs [ImGuiNavInput_PadRight]    += io.KeysDown ['D'] ? 1.0f : 0.0f;  // move right
    }

    io.NavInputs [ImGuiNavInput_PadActivate]   += io.KeysDown [VK_RETURN] ? 1.0f : 0.0f;
    io.NavInputs [ImGuiNavInput_PadCancel]     += io.KeysDown [VK_ESCAPE] ? 1.0f : 0.0f;
  }

  else
    io.NavActive = false;

  io.NavInputs [ImGuiNavInput_PadFocusPrev]  += io.KeyCtrl && io.KeyShift && io.KeysDown [VK_TAB] && io.KeysDownDuration [VK_TAB] == 0.0f ? 1.0f : 0.0f;
  io.NavInputs [ImGuiNavInput_PadFocusNext]  += io.KeyCtrl &&                io.KeysDown [VK_TAB] && io.KeysDownDuration [VK_TAB] == 0.0f ? 1.0f : 0.0f;

}
IMGUI_API
void
ImGui_ImplDX9_NewFrame (void)
{
  ImGuiIO& io =
    ImGui::GetIO ();

  if (! g_FontTexture) {
    ImGui_ImplDX9_CreateDeviceObjects ();
  }

  static HMODULE hModTBFix = GetModuleHandle (L"tbfix.dll");


  // Setup display size (every frame to accommodate for window resizing)
  RECT rect;
  GetClientRect (g_hWnd, &rect);

  io.DisplayFramebufferScale =
    ImVec2 ( (float)(rect.right - rect.left),
               (float)(rect.bottom - rect.top) );


  if (! g_pd3dDevice)
    return;


  CComPtr <IDirect3DSwapChain9> pSwapChain = nullptr;

  if (SUCCEEDED (g_pd3dDevice->GetSwapChain ( 0, &pSwapChain )))
  {
    D3DPRESENT_PARAMETERS pp;

    if (SUCCEEDED (pSwapChain->GetPresentParameters (&pp)))
    {
      if (pp.BackBufferWidth != 0 && pp.BackBufferHeight != 0)
      {
        io.DisplaySize.x = (float)pp.BackBufferWidth;
        io.DisplaySize.y = (float)pp.BackBufferHeight;

        io.DisplayFramebufferScale = ImVec2 ( (float)pp.BackBufferWidth, (float)pp.BackBufferHeight );
      }
    }
  }

  // Setup time step
  INT64 current_time;

  QueryPerformanceCounter ((LARGE_INTEGER *)&current_time);

  io.DeltaTime = (float)(current_time - g_Time) / g_TicksPerSecond;
  g_Time       =         current_time;

  // Read keyboard modifiers inputs
  io.KeyCtrl   = (GetAsyncKeyState_Original (VK_CONTROL) & 0x8000) != 0;
  io.KeyShift  = (GetAsyncKeyState_Original (VK_SHIFT)   & 0x8000) != 0;
  io.KeyAlt    = (GetAsyncKeyState_Original (VK_MENU)    & 0x8000) != 0;

  io.KeySuper  = false;

  SK_ImGui_PollGamepad ();

  // Start the frame
  ImGui::NewFrame ();
}
