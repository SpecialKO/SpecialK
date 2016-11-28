#include <string>

#include <SpecialK/config.h>

#include <SpecialK/ini.h>
#include <SpecialK/parameter.h>
#include <SpecialK/utility.h>

#include <SpecialK/log.h>

#include <SpecialK/hooks.h>
#include <SpecialK/core.h>

#include <process.h>

sk::ParameterFactory tw3_factory;

// The MOD's settings
sk::ParameterBool*   tw3_flipmode    = nullptr;
sk::ParameterBool*   tw3_fullscreen  = nullptr;

void
SK_TW3_InitPlugin (void)
{
}

bool
SK_TW3_MaximizeBorderless (void)
{
  iSK_INI* dll_ini =
    SK_GetDLLConfig ();

  if (tw3_fullscreen == nullptr) {
    tw3_fullscreen =
      static_cast <sk::ParameterBool *>
        (tw3_factory.create_parameter <bool> (L"Maximize Borderless Window"));
    tw3_fullscreen->register_to_ini ( dll_ini,
                                        L"TW3.PlugIn",
                                          L"FullscreenWindow" );
    tw3_fullscreen->load ();
  }

  return (tw3_fullscreen->get_value ());
}

bool
SK_TW3_UseFlipMode (void)
{
  iSK_INI* dll_ini =
    SK_GetDLLConfig ();

  SK_TW3_InitPlugin ();

  if (tw3_flipmode == nullptr) {
    tw3_flipmode =
      static_cast <sk::ParameterBool *>
        (tw3_factory.create_parameter <bool> (L"Use Flip Mode"));
    tw3_flipmode->register_to_ini ( dll_ini,
                                       L"TW3.PlugIn",
                                         L"FlipMode" );
    tw3_flipmode->load ();
  }

  return (tw3_flipmode->get_value ());
}

#if 0
bool
SK_FO4_IsFullscreen (void)
{
  SK_FO4_InitPlugin ();

  if (fo4_fullscreen == nullptr) {
    fo4_fullscreen =
      static_cast <sk::ParameterBool *>
        (fo4_factory.create_parameter <bool> (L"Fullscreen Mode"));
    fo4_fullscreen->register_to_ini ( fo4_prefs,
                                        L"Display",
                                          L"bFull Screen" );

    fo4_fullscreen->load ();
  }

  return (fo4_fullscreen->get_value ());
}

bool
SK_FO4_IsBorderlessWindow (void)
{
  SK_FO4_InitPlugin ();

  if (fo4_borderless == nullptr) {
    fo4_borderless =
      static_cast <sk::ParameterBool *>
        (fo4_factory.create_parameter <bool> (L"Borderless Window"));
    fo4_borderless->register_to_ini ( fo4_prefs,
                                        L"Display",
                                          L"bBorderless" );

    fo4_borderless->load ();
  }

  return (fo4_borderless->get_value ());
}
#endif

#include "config.h"

static RECT window;

unsigned int
__stdcall
SK_TW3_RealizeFullscreenBorderless (LPVOID user)
{
  DEVMODE devmode = { 0 };
  devmode.dmSize  = sizeof DEVMODE;

  EnumDisplaySettings (nullptr, ENUM_CURRENT_SETTINGS, &devmode);

  DXGI_SWAP_CHAIN_DESC desc;
  ((IDXGISwapChain *)user)->GetDesc (&desc);

  //
  // Resize the desktop for DSR
  //
  if (devmode.dmPelsHeight < desc.BufferDesc.Height ||
      devmode.dmPelsWidth  < desc.BufferDesc.Width) {
    devmode.dmPelsWidth  = desc.BufferDesc.Width;
    devmode.dmPelsHeight = desc.BufferDesc.Height;
    ChangeDisplaySettings (&devmode, CDS_FULLSCREEN);
  }

  MONITORINFO minfo;
  minfo.cbSize = sizeof MONITORINFO;

  GetMonitorInfo (MonitorFromWindow (desc.OutputWindow, MONITOR_DEFAULTTONEAREST), &minfo);

  if (SK_TW3_MaximizeBorderless ()) {
    SetWindowPos ( desc.OutputWindow,
                     HWND_TOP,
                       minfo.rcMonitor.left, minfo.rcMonitor.top,
                         minfo.rcMonitor.right - minfo.rcMonitor.left,
                         minfo.rcMonitor.bottom - minfo.rcMonitor.top,
                           SWP_NOSENDCHANGING | SWP_NOACTIVATE    | /*SWP_NOREPOSITION |
                           SWP_NOMOVE         |*/ SWP_NOOWNERZORDER | SWP_NOREDRAW );
  }

  return 0;
}

typedef BOOL (WINAPI *GetWindowRect_pfn)(HWND, LPRECT);
static GetWindowRect_pfn GetWindowRect_Original = nullptr;

typedef BOOL (WINAPI *GetClientRect_pfn)(HWND, LPRECT);
static GetClientRect_pfn GetClientRect_Original = nullptr;

static
BOOL
WINAPI
GetClientRect_Detour (
    _In_  HWND   hWnd,
    _Out_ LPRECT lpRect )
{
  *lpRect = window;
  return TRUE;
}

static
BOOL
WINAPI
GetWindowRect_Detour (
    _In_   HWND  hWnd,
    _Out_ LPRECT lpRect )
{
  *lpRect = window;
  return TRUE;
}

typedef LRESULT (CALLBACK *SK_DetourWindowProc_pfn)( _In_  HWND   hWnd,
                      _In_  UINT   uMsg,
                      _In_  WPARAM wParam,
                      _In_  LPARAM lParam );
static SK_DetourWindowProc_pfn SK_DetourWindowProc_Original = nullptr;

__declspec (noinline)
LRESULT
CALLBACK
SK_TW3_DetourWindowProc ( _In_  HWND   hWnd,
                          _In_  UINT   uMsg,
                          _In_  WPARAM wParam,
                          _In_  LPARAM lParam )
{
 if (uMsg == WM_WINDOWPOSCHANGED || uMsg == WM_WINDOWPOSCHANGING)
    return 0;

  if (uMsg == WM_MOVE || uMsg == WM_MOVING)
    return 0;

  if (uMsg == WM_SIZE || uMsg == WM_SIZING)
    return 0;

  return SK_DetourWindowProc_Original (hWnd, uMsg, wParam, lParam);
}


typedef HRESULT (STDMETHODCALLTYPE *DXGISwap_ResizeBuffers_pfn)(
    _In_ IDXGISwapChain *,
    _In_ UINT,
    _In_ UINT,
    _In_ UINT,
    _In_ DXGI_FORMAT,
    _In_ UINT
);

static DXGISwap_ResizeBuffers_pfn DXGISwap_ResizeBuffers_Original = nullptr;

extern "C" HRESULT STDMETHODCALLTYPE
  DXGISwap_ResizeBuffers_Override ( IDXGISwapChain *,
                               _In_ UINT,
                               _In_ UINT,
                               _In_ UINT,
                               _In_ DXGI_FORMAT,
                               _In_ UINT );

COM_DECLSPEC_NOTHROW
__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
SK_TW3_ResizeBuffers (IDXGISwapChain *This,
                 _In_ UINT            BufferCount,
                 _In_ UINT            Width,
                 _In_ UINT            Height,
                 _In_ DXGI_FORMAT     NewFormat,
                 _In_ UINT            SwapChainFlags)
{
  HRESULT hr =
    DXGISwap_ResizeBuffers_Original ( This,
                                        BufferCount,
                                          Width, Height,
                                            NewFormat,
                                              SwapChainFlags );

  if (SUCCEEDED (hr)) {
    BOOL fullscreen = FALSE;
    This->GetFullscreenState (&fullscreen, nullptr);

    if (SK_TW3_MaximizeBorderless () && (! fullscreen)) {
      _beginthreadex ( nullptr,
                         0,
                           SK_TW3_RealizeFullscreenBorderless,
                             (LPVOID)This,
                               0x00,
                                 nullptr );
    }
  }

  return hr;
}


HRESULT
STDMETHODCALLTYPE
SK_TW3_PresentFirstFrame ( IDXGISwapChain *This,
                           UINT            SyncInterval,
                           UINT            Flags )
{
  DXGI_SWAP_CHAIN_DESC desc;
  This->GetDesc (&desc);

  // Fix the broken borderless window system that doesn't scale the swapchain
  //   properly.
  //if (SK_FO4_IsBorderlessWindow ()) {
    window.top    = 0;
    window.left   = 0;
    window.bottom = desc.BufferDesc.Height;
    window.right  = desc.BufferDesc.Width;

    SK_CreateDLLHook ( L"user32.dll", "GetWindowRect",
                       GetWindowRect_Detour,
             (LPVOID*)&GetWindowRect_Original );

    SK_CreateDLLHook ( L"user32.dll", "GetClientRect",
                       GetClientRect_Detour,
             (LPVOID*)&GetClientRect_Original );

    extern __declspec (noinline)
    LRESULT
    CALLBACK
    SK_DetourWindowProc ( _In_  HWND   hWnd,
                          _In_  UINT   uMsg,
                          _In_  WPARAM wParam,
                          _In_  LPARAM lParam );

    SK_CreateFuncHook ( L"SK_DetourWindowProc",
                          SK_DetourWindowProc,
                          SK_TW3_DetourWindowProc,
               (LPVOID *)&SK_DetourWindowProc_Original );
    SK_EnableHook (SK_DetourWindowProc);

    SK_CreateFuncHook ( L"IDXGISwapChain::ResizeBuffers",
                          DXGISwap_ResizeBuffers_Override,
                            SK_TW3_ResizeBuffers,
                              (LPVOID *)&DXGISwap_ResizeBuffers_Original );
    SK_EnableHook (DXGISwap_ResizeBuffers_Override);


    _beginthreadex ( nullptr,
                       0,
                         SK_TW3_RealizeFullscreenBorderless,
                           (LPVOID)This,
                             0x00,
                               nullptr );

    return S_OK;
  //}

  return E_FAIL;
}