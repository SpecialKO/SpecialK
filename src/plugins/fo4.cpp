#include <SpecialK/stdafx.h>

#if (defined (NOGDI) || (! defined (_WINGDI_)))
#undef        NOGDI
#undef      _WINGDI_
#include    <wingdi.h>
#endif

// These are the actual GAME settings
iSK_INI*             fo4_prefs        = nullptr;
sk::ParameterBool*   fo4_fullscreen   = nullptr;
sk::ParameterBool*   fo4_borderless   = nullptr;

// The MOD's settings
sk::ParameterBool*   fo4w_flipmode    = nullptr;
sk::ParameterBool*   fo4w_fullscreen  = nullptr;
sk::ParameterBool*   fo4w_center      = nullptr;

#define __NvAPI_GPU_GetMemoryInfo 0x07F9B368

static HMODULE nvapi64_dll;

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
      dynamic_cast <sk::ParameterInt *>
        (g_ParameterFactory->create_parameter <int> (L"Memory Multiplier"));
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

HRESULT STDMETHODCALLTYPE SK_FO4_PresentFirstFrame ( IUnknown *, UINT, UINT );

void
SK_FO4_InitPlugin (void)
{
  if (fo4_prefs == nullptr)
  {
    std::wstring fo4_prefs_file =
      SK_GetDocumentsDir () +
      std::wstring (L"\\My Games\\Fallout4\\Fallout4Prefs.ini");

    fo4_prefs =
      SK_CreateINI ((wchar_t *)fo4_prefs_file.c_str ());
  }

  //nvapi64_dll = SK_LoadLibrary (L"nvapi64.dll");

  //typedef void* (*NvAPI_QueryInterface_t)(unsigned int ordinal);

  //static NvAPI_QueryInterface_t NvAPI_QueryInterface =
    //(NvAPI_QueryInterface_t)SK_GetProcAddress (nvapi64_dll, "nvapi_QueryInterface");

    //(NvAPI_GPU_GetMemoryInfo_t)NvAPI_QueryInterface (__NvAPI_GPU_GetMemoryInfo);

  SK_RunOnce (plugin_mgr->first_frame_fns.emplace (SK_FO4_PresentFirstFrame));

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

  bool bRet = false;

  if (fo4w_fullscreen == nullptr)
  {
    fo4w_fullscreen =
      dynamic_cast <sk::ParameterBool *>
        (g_ParameterFactory->create_parameter <bool> (L"Maximize Borderless Window"));
    fo4w_fullscreen->register_to_ini ( dll_ini,
                                        L"FO4W.PlugIn",
                                          L"FullscreenWindow" );
    fo4w_fullscreen->load (bRet);
  }

  return bRet;
}

bool
SK_FO4_CenterWindow (void)
{
  iSK_INI* dll_ini =
    SK_GetDLLConfig ();

  SK_FO4_InitPlugin ();

  bool bRet = false;

  if (fo4w_center == nullptr)
  {
    fo4w_center =
      dynamic_cast <sk::ParameterBool *>
        (g_ParameterFactory->create_parameter <bool> (L"Center Borderless Window"));
    fo4w_center->register_to_ini ( dll_ini,
                                     L"FO4W.PlugIn",
                                       L"CenterWindow" );
    fo4w_center->load (bRet);
  }

  return bRet;
}

bool
SK_FO4_UseFlipMode (void)
{
  iSK_INI* dll_ini =
    SK_GetDLLConfig ();

  SK_FO4_InitPlugin ();

  bool bRet = false;

  if (fo4w_flipmode == nullptr)
  {
    fo4w_flipmode =
      dynamic_cast <sk::ParameterBool *>
        (g_ParameterFactory->create_parameter <bool> (L"Use Flip Mode"));
    fo4w_flipmode->register_to_ini ( dll_ini,
                                       L"FO4W.PlugIn",
                                         L"FlipMode" );
    fo4w_flipmode->load (bRet);
  }

  return bRet;
}

bool
SK_FO4_IsFullscreen (void)
{
  SK_FO4_InitPlugin ();

  bool bRet = false;

  if (fo4_fullscreen == nullptr)
  {
    fo4_fullscreen =
      dynamic_cast <sk::ParameterBool *>
        (g_ParameterFactory->create_parameter <bool> (L"Fullscreen Mode"));
    fo4_fullscreen->register_to_ini ( fo4_prefs,
                                        L"Display",
                                          L"bFull Screen" );

    fo4_fullscreen->load (bRet);
  }

  return bRet;
}

bool
SK_FO4_IsBorderlessWindow (void)
{
  SK_FO4_InitPlugin ();

  bool bRet = false;

  if (fo4_borderless == nullptr)
  {
    fo4_borderless =
      dynamic_cast <sk::ParameterBool *>
        (g_ParameterFactory->create_parameter <bool> (L"Borderless Window"));
    fo4_borderless->register_to_ini ( fo4_prefs,
                                        L"Display",
                                          L"bBorderless" );

    fo4_borderless->load (bRet);
  }

  return bRet;
}

static RECT window;

typedef BOOL (WINAPI *EnumDisplaySettingsW_pfn) (
                        _In_opt_ LPWSTR    lpszDeviceName,
                        _In_     DWORD      iModeNum,
                        _Inout_  DEVMODEW *lpDevMode
);
using ChangeDisplaySettingsW_pfn = LONG (WINAPI *)(
  _In_opt_ DEVMODEW *lpDevMode,
  _In_     DWORD     dwFlags
);

extern EnumDisplaySettingsW_pfn   EnumDisplaySettingsW_Original;
extern ChangeDisplaySettingsW_pfn ChangeDisplaySettingsW_Original;

DWORD
__stdcall
SK_FO4_RealizeFullscreenBorderless (LPVOID user)
{
  DEVMODE devmode = { 0 };
  devmode.dmSize  = sizeof DEVMODE;

  EnumDisplaySettingsW_Original (nullptr, ENUM_CURRENT_SETTINGS, &devmode);

  DXGI_SWAP_CHAIN_DESC desc;
  ((IDXGISwapChain *)user)->GetDesc (&desc);

  //
  // Resize the desktop for DSR
  //
  if (devmode.dmPelsHeight < desc.BufferDesc.Height ||
      devmode.dmPelsWidth  < desc.BufferDesc.Width) {
    devmode.dmPelsWidth  = desc.BufferDesc.Width;
    devmode.dmPelsHeight = desc.BufferDesc.Height;
    ChangeDisplaySettingsW_Original (&devmode, CDS_FULLSCREEN);
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

    LONG win_width  = std::min (mon_width,  (LONG)desc.BufferDesc.Width);
    LONG win_height = std::min (mon_height, (LONG)desc.BufferDesc.Height);

    window.left = std::max (0L, (mon_width  - win_width)  / 2L);
    window.top  = std::max (0L, (mon_height - win_height) / 2L);

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
SK_FO4_PresentFirstFrame ( IUnknown *pSwapChain,
                           UINT      SyncInterval,
                           UINT      Flags )
{
  IDXGISwapChain *This = (IDXGISwapChain *)pSwapChain;
  UNREFERENCED_PARAMETER (SyncInterval);
  UNREFERENCED_PARAMETER (Flags);

  DXGI_SWAP_CHAIN_DESC desc = { };
  This->GetDesc (&desc);

  // Fix the broken borderless window system that doesn't scale the swapchain
  //   properly.
  if (SK_FO4_IsBorderlessWindow ())
  {
    window.top    = 0;
    window.left   = 0;
    window.bottom = desc.BufferDesc.Height;
    window.right  = desc.BufferDesc.Width;

    SK_CreateDLLHook (       L"user32.dll",
                              "GetWindowRect",
                               GetWindowRect_Detour,
      static_cast_p2p <void> (&GetWindowRect_Original) );

    SK_CreateDLLHook (       L"user32.dll",
                              "GetClientRect",
                               GetClientRect_Detour,
      static_cast_p2p <void> (&GetClientRect_Original) );

    extern __declspec (noinline)
    LRESULT
    CALLBACK
    SK_DetourWindowProc ( _In_  HWND   hWnd,
                          _In_  UINT   uMsg,
                          _In_  WPARAM wParam,
                          _In_  LPARAM lParam );

    SK_CreateFuncHook (      L"SK_DetourWindowProc",
                               SK_DetourWindowProc,
                               SK_FO4_DetourWindowProc,
      static_cast_p2p <void> (&SK_DetourWindowProc_Original) );
    SK_EnableHook (SK_DetourWindowProc);

    CreateThread ( nullptr,
                       0,
                         SK_FO4_RealizeFullscreenBorderless,
                           (LPVOID)This,
                             0x00,
                               nullptr );

    return S_OK;
  }

  return E_FAIL;
}