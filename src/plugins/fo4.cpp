#include <SpecialK/dxgi_backend.h>
#include <SpecialK/config.h>
#include <SpecialK/ini.h>
#include <SpecialK/parameter.h>
#include <SpecialK/utility.h>
#include <SpecialK/log.h>

sk::ParameterFactory fo4_factory;

// These are the actual GAME settings
iSK_INI*             fo4_prefs        = nullptr;
sk::ParameterBool*   fo4_fullscreen   = nullptr;
sk::ParameterBool*   fo4_borderless   = nullptr;

// The MOD's settings
sk::ParameterBool*   fo4w_flipmode    = nullptr;
sk::ParameterBool*   fo4w_fullscreen  = nullptr;
sk::ParameterBool*   fo4w_center      = nullptr;

#define __NvAPI_GPU_GetMemoryInfo 0x07F9B368

HMODULE nvapi64_dll;

#include <SpecialK/hooks.h>
#include <SpecialK/core.h>
#include <SpecialK/nvapi.h>

#include <string>
#include <process.h>

#if 0
typedef NvAPI_Status (__cdecl *NvAPI_GPU_GetMemoryInfo_t)(NvPhysicalGpuHandle hPhysicalGpu, NV_DISPLAY_DRIVER_MEMORY_INFO *pMemoryInfo);
NvAPI_GPU_GetMemoryInfo_t NvAPI_GPU_GetMemoryInfo_Original = nullptr;

NvAPI_Status
__cdecl
NvAPI_GPU_GetMemoryInfo_Detour ( NvPhysicalGpuHandle            hPhysicalGpu,
                                 NV_DISPLAY_DRIVER_MEMORY_INFO *pMemoryInfo )
{
  if (fo4_spoof_memory == nullptr) {
    fo4_spoof_memory =
      static_cast <sk::ParameterInt *>
        (fo4_factory.create_parameter <int> (L"Memory Multiplier"));
    fo4_spoof_memory->register_to_ini ( fo4_prefs,
                                        L"Display",
                                          L"iMemoryMultiplier" );

    fo4_spoof_memory->load ();
  }

  const int shift = fo4_spoof_memory->get_value ();

  NvAPI_Status status =
    NvAPI_GPU_GetMemoryInfo_Original (hPhysicalGpu, pMemoryInfo);

  if (status == NVAPI_OK) {
    pMemoryInfo->dedicatedVideoMemory             <<= shift;
    pMemoryInfo->availableDedicatedVideoMemory    <<= shift;
    pMemoryInfo->curAvailableDedicatedVideoMemory <<= shift;
    pMemoryInfo->sharedSystemMemory               <<= shift;
    pMemoryInfo->systemVideoMemory                <<= shift;
  }

  return status;
}
#endif

LPVOID NVAPI_GPU_GETMEMORYINFO_PROC;

void
SK_FO4_InitPlugin (void)
{
  if (fo4_prefs == nullptr) {
    std::wstring fo4_prefs_file =
      SK_GetDocumentsDir () +
      std::wstring (L"\\My Games\\Fallout4\\Fallout4Prefs.ini");

    fo4_prefs =
      SK_CreateINI ((wchar_t *)fo4_prefs_file.c_str ());
  }

  //nvapi64_dll = LoadLibrary (L"nvapi64.dll");

  //typedef void* (*NvAPI_QueryInterface_t)(unsigned int ordinal);

  //static NvAPI_QueryInterface_t NvAPI_QueryInterface =
    //(NvAPI_QueryInterface_t)GetProcAddress (nvapi64_dll, "nvapi_QueryInterface");

    //(NvAPI_GPU_GetMemoryInfo_t)NvAPI_QueryInterface (__NvAPI_GPU_GetMemoryInfo);

#if 0
  SK_CreateFuncHook ( L"NvAPI_GPU_GetMemoryInfo", NvAPI_GPU_GetMemoryInfo,
                        NvAPI_GPU_GetMemoryInfo_Detour, (LPVOID *)&NvAPI_GPU_GetMemoryInfo_Original );
  SK_EnableHook     (NvAPI_GPU_GetMemoryInfo);
#endif
}

bool
SK_FO4_MaximizeBorderless (void)
{
  iSK_INI* dll_ini =
    SK_GetDLLConfig ();

  SK_FO4_InitPlugin ();

  if (fo4w_fullscreen == nullptr) {
    fo4w_fullscreen =
      static_cast <sk::ParameterBool *>
        (fo4_factory.create_parameter <bool> (L"Maximize Borderless Window"));
    fo4w_fullscreen->register_to_ini ( dll_ini,
                                        L"FO4W.PlugIn",
                                          L"FullscreenWindow" );
    fo4w_fullscreen->load ();
  }

  return (fo4w_fullscreen->get_value ());
}

bool
SK_FO4_CenterWindow (void)
{
  iSK_INI* dll_ini =
    SK_GetDLLConfig ();

  SK_FO4_InitPlugin ();

  if (fo4w_center == nullptr) {
    fo4w_center =
      static_cast <sk::ParameterBool *>
        (fo4_factory.create_parameter <bool> (L"Center Borderless Window"));
    fo4w_center->register_to_ini ( dll_ini,
                                     L"FO4W.PlugIn",
                                       L"CenterWindow" );
    fo4w_center->load ();
  }

  return (fo4w_center->get_value ());
}

bool
SK_FO4_UseFlipMode (void)
{
  iSK_INI* dll_ini =
    SK_GetDLLConfig ();

  SK_FO4_InitPlugin ();

  if (fo4w_flipmode == nullptr) {
    fo4w_flipmode =
      static_cast <sk::ParameterBool *>
        (fo4_factory.create_parameter <bool> (L"Use Flip Mode"));
    fo4w_flipmode->register_to_ini ( dll_ini,
                                       L"FO4W.PlugIn",
                                         L"FlipMode" );
    fo4w_flipmode->load ();
  }

  return (fo4w_flipmode->get_value ());
}

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

#include <SpecialK/config.h>

RECT window;

unsigned int
__stdcall
SK_FO4_RealizeFullscreenBorderless (LPVOID user)
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

  if (SK_FO4_MaximizeBorderless ()) {
    SetWindowPos ( desc.OutputWindow,
                     HWND_TOP,
                       minfo.rcMonitor.left, minfo.rcMonitor.top,
                         minfo.rcMonitor.right - minfo.rcMonitor.left,
                         minfo.rcMonitor.bottom - minfo.rcMonitor.top,
                           SWP_NOSENDCHANGING | SWP_NOACTIVATE    | SWP_NOREPOSITION |
                           SWP_NOMOVE         | SWP_NOOWNERZORDER | SWP_NOREDRAW );
  }

  else if (SK_FO4_CenterWindow ()) {
    extern HWND __stdcall SK_GetGameWindow (void);

    AdjustWindowRect (&minfo.rcWork, GetWindowLongW (SK_GetGameWindow (), GWL_STYLE), FALSE);

    LONG mon_width  = minfo.rcWork.right  - minfo.rcWork.left;
    LONG mon_height = minfo.rcWork.bottom - minfo.rcWork.top;

    LONG win_width  = min (mon_width,  (LONG)desc.BufferDesc.Width);
    LONG win_height = min (mon_height, (LONG)desc.BufferDesc.Height);

    window.left = max (0, (mon_width  - win_width)  / 2);
    window.top  = max (0, (mon_height - win_height) / 2);

    window.right  = window.left + win_width;
    window.bottom = window.top  + win_height;

    SetWindowPos ( desc.OutputWindow,
                     HWND_TOP,
                       window.left, window.top,
                         window.right  - window.left,
                         window.bottom - window.top,
                           SWP_NOSENDCHANGING | SWP_NOACTIVATE  |
                           SWP_NOOWNERZORDER  | SWP_NOREDRAW );
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
  UNREFERENCED_PARAMETER (hWnd);

  *lpRect = window;
  return TRUE;
}

static BOOL
WINAPI
GetWindowRect_Detour (
    _In_   HWND  hWnd,
    _Out_ LPRECT lpRect )
{
  UNREFERENCED_PARAMETER (hWnd);

  *lpRect = window;
  return TRUE;
}

typedef LRESULT (CALLBACK *SK_DetourWindowProc_pfn)( _In_  HWND   hWnd,
                      _In_  UINT   uMsg,
                      _In_  WPARAM wParam,
                      _In_  LPARAM lParam );
SK_DetourWindowProc_pfn SK_DetourWindowProc_Original = nullptr;

__declspec (noinline)
LRESULT
CALLBACK
SK_FO4_DetourWindowProc ( _In_  HWND   hWnd,
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


HRESULT
STDMETHODCALLTYPE
SK_FO4_PresentFirstFrame ( IDXGISwapChain *This,
                           UINT            SyncInterval,
                           UINT            Flags )
{
  UNREFERENCED_PARAMETER (SyncInterval);
  UNREFERENCED_PARAMETER (Flags);

  DXGI_SWAP_CHAIN_DESC desc;
  This->GetDesc (&desc);

  // Fix the broken borderless window system that doesn't scale the swapchain
  //   properly.
  if (SK_FO4_IsBorderlessWindow ()) {
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
                          SK_FO4_DetourWindowProc,
               (LPVOID *)&SK_DetourWindowProc_Original );
    SK_EnableHook (SK_DetourWindowProc);

    _beginthreadex ( nullptr,
                       0,
                         SK_FO4_RealizeFullscreenBorderless,
                           (LPVOID)This,
                             0x00,
                               nullptr );

    return S_OK;
  }

  return E_FAIL;
}