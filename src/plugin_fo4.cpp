#include <string>

#include "ini.h"
#include "parameter.h"
#include "utility.h"

#include "log.h"

sk::ParameterFactory fo4_factory;

sk::INI::File*       fo4_prefs        = nullptr;
sk::ParameterBool*   fo4_fullscreen   = nullptr;
sk::ParameterBool*   fo4_borderless   = nullptr;
sk::ParameterInt*    fo4_spoof_memory = nullptr;

#define __NvAPI_GPU_GetMemoryInfo 0x07F9B368

HMODULE nvapi64_dll;
#include "core.h"
#include "nvapi.h"

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

LPVOID NVAPI_GPU_GETMEMORYINFO_PROC;

void
SK_FO4_InitPlugin (void)
{
  if (fo4_prefs == nullptr) {
    std::wstring fo4_prefs_file =
      SK_GetDocumentsDir () +
      std::wstring (L"\\My Games\\Fallout4\\Fallout4Prefs.ini");

    fo4_prefs = new sk::INI::File ((wchar_t *)fo4_prefs_file.c_str ());
    fo4_prefs->parse ();

  }

  //nvapi64_dll = LoadLibrary (L"nvapi64.dll");

  //typedef void* (*NvAPI_QueryInterface_t)(unsigned int ordinal);

  //static NvAPI_QueryInterface_t NvAPI_QueryInterface =
    //(NvAPI_QueryInterface_t)GetProcAddress (nvapi64_dll, "nvapi_QueryInterface");

    //(NvAPI_GPU_GetMemoryInfo_t)NvAPI_QueryInterface (__NvAPI_GPU_GetMemoryInfo);

  SK_CreateFuncHook ( L"NvAPI_GPU_GetMemoryInfo", NvAPI_GPU_GetMemoryInfo,
                        NvAPI_GPU_GetMemoryInfo_Detour, (LPVOID *)&NvAPI_GPU_GetMemoryInfo_Original );
  SK_EnableHook     (NvAPI_GPU_GetMemoryInfo);
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

#include "config.h"

DWORD
WINAPI
SK_FO4_RealizeFullscreenBorderless (LPVOID user)
{
  DXGI_SWAP_CHAIN_DESC desc;
  ((IDXGISwapChain *)user)->GetDesc (&desc);

  MONITORINFO minfo;
  minfo.cbSize = sizeof MONITORINFO;

  GetMonitorInfo (MonitorFromWindow (desc.OutputWindow, MONITOR_DEFAULTTONEAREST), &minfo);

  SetWindowPos ( desc.OutputWindow,
                  HWND_TOP,
                    minfo.rcMonitor.left, minfo.rcMonitor.top,
                      minfo.rcMonitor.right - minfo.rcMonitor.left,
                      minfo.rcMonitor.bottom - minfo.rcMonitor.top,
                        SWP_NOSENDCHANGING | SWP_NOACTIVATE | SWP_NOREPOSITION | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOREDRAW );

  return 0;
}

RECT window;

typedef BOOL (WINAPI *GetWindowRect_pfn)(HWND, LPRECT);
GetWindowRect_pfn GetWindowRect_Original = nullptr;

typedef BOOL (WINAPI *GetClientRect_pfn)(HWND, LPRECT);
GetClientRect_pfn GetClientRect_Original = nullptr;

BOOL
WINAPI
GetClientRect_Detour (
    _In_  HWND   hWnd,
    _Out_ LPRECT lpRect )
{
  *lpRect = window;
  return TRUE;
}

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

    CreateThread (nullptr, 0, SK_FO4_RealizeFullscreenBorderless, (LPVOID)This, 0, nullptr);

    return S_OK;
  }

  return E_FAIL;
}