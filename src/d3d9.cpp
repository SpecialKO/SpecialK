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

#define _CRT_SECURE_NO_WARNINGS

#include "d3d9_backend.h"
#include "D3D9/texmgr.h"

#include "log.h"

#include "stdafx.h"
#include "nvapi.h"
#include "config.h"

#include <cstdio>
#include <cstdlib>
#include <string>

#include <atlbase.h>

#include "log.h"

volatile LONG                        __d3d9_ready = FALSE;
SK::D3D9::PipelineStatsD3D9 SK::D3D9::pipeline_stats_d3d9;

unsigned int
__stdcall
HookD3D9Ex (LPVOID user);

void WaitForInit_D3D9 (void)
{
  while (! InterlockedCompareExchange (&__d3d9_ready, FALSE, FALSE))
    Sleep (config.system.init_delay);
}

extern "C++" void
__stdcall
SK_D3D9_UpdateRenderStats (IDirect3DSwapChain9* pSwapChain, IDirect3DDevice9* pDevice = nullptr);


typedef IDirect3D9*
  (STDMETHODCALLTYPE *Direct3DCreate9PROC)(  UINT           SDKVersion);
typedef HRESULT
  (STDMETHODCALLTYPE *Direct3DCreate9ExPROC)(UINT           SDKVersion,
                                             IDirect3D9Ex** d3d9ex);
COM_DECLSPEC_NOTHROW
__declspec (noinline)
D3DPRESENT_PARAMETERS*
WINAPI
SK_SetPresentParamsD3D9 (IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pparams);


typedef HRESULT (STDMETHODCALLTYPE *D3D9PresentDevice_pfn)(
           IDirect3DDevice9    *This,
_In_ const RECT                *pSourceRect,
_In_ const RECT                *pDestRect,
_In_       HWND                 hDestWindowOverride,
_In_ const RGNDATA             *pDirtyRegion);

typedef HRESULT (STDMETHODCALLTYPE *D3D9PresentDeviceEx_pfn)(
           IDirect3DDevice9Ex  *This,
_In_ const RECT                *pSourceRect,
_In_ const RECT                *pDestRect,
_In_       HWND                 hDestWindowOverride,
_In_ const RGNDATA             *pDirtyRegion,
_In_       DWORD                dwFlags);

typedef HRESULT (STDMETHODCALLTYPE *D3D9PresentSwapChain_pfn)(
             IDirect3DSwapChain9 *This,
  _In_ const RECT                *pSourceRect,
  _In_ const RECT                *pDestRect,
  _In_       HWND                 hDestWindowOverride,
  _In_ const RGNDATA             *pDirtyRegion,
  _In_       DWORD                dwFlags);

typedef HRESULT (STDMETHODCALLTYPE *D3D9PresentSwapChainEx_pfn)(
  IDirect3DDevice9Ex  *This,
  _In_ const RECT                *pSourceRect,
  _In_ const RECT                *pDestRect,
  _In_       HWND                 hDestWindowOverride,
  _In_ const RGNDATA             *pDirtyRegion,
  _In_       DWORD                dwFlags);

typedef HRESULT (STDMETHODCALLTYPE *D3D9CreateDevice_pfn)(
           IDirect3D9             *This,
           UINT                    Adapter,
           D3DDEVTYPE              DeviceType,
           HWND                    hFocusWindow,
           DWORD                   BehaviorFlags,
           D3DPRESENT_PARAMETERS  *pPresentationParameters,
           IDirect3DDevice9      **ppReturnedDeviceInterface);

typedef HRESULT (STDMETHODCALLTYPE *D3D9CreateDeviceEx_pfn)(
           IDirect3D9Ex           *This,
           UINT                    Adapter,
           D3DDEVTYPE              DeviceType,
           HWND                    hFocusWindow,
           DWORD                   BehaviorFlags,
           D3DPRESENT_PARAMETERS  *pPresentationParameters,
           D3DDISPLAYMODEEX       *pFullscreenDisplayMode,
           IDirect3DDevice9Ex    **ppReturnedDeviceInterface);

typedef HRESULT (STDMETHODCALLTYPE *D3D9Reset_pfn)(
           IDirect3DDevice9      *This,
           D3DPRESENT_PARAMETERS *pPresentationParameters);

typedef HRESULT (STDMETHODCALLTYPE *D3D9ResetEx_pfn)(
           IDirect3DDevice9Ex    *This,
           D3DPRESENT_PARAMETERS *pPresentationParameters,
           D3DDISPLAYMODEEX      *pFullscreenDisplayMode );

D3D9PresentDevice_pfn    D3D9Present_Original        = nullptr;
D3D9PresentDeviceEx_pfn  D3D9PresentEx_Original      = nullptr;
D3D9PresentSwapChain_pfn D3D9PresentSwap_Original    = nullptr;
D3D9CreateDevice_pfn     D3D9CreateDevice_Original   = nullptr;
D3D9CreateDeviceEx_pfn   D3D9CreateDeviceEx_Original = nullptr;
D3D9Reset_pfn            D3D9Reset_Original          = nullptr;
D3D9ResetEx_pfn          D3D9ResetEx_Original        = nullptr;

Direct3DCreate9PROC   Direct3DCreate9_Import   = nullptr;
Direct3DCreate9ExPROC Direct3DCreate9Ex_Import = nullptr;

IDirect3DDevice9*     g_pD3D9Dev = nullptr;
D3DPRESENT_PARAMETERS g_D3D9PresentParams;


HMODULE local_d3d9 = 0;

HMODULE
SK_LoadRealD3D9 (void)
{
  wchar_t wszBackendDLL [MAX_PATH] = { L'\0' };

#ifdef _WIN64
  GetSystemDirectory (wszBackendDLL, MAX_PATH);
#else
  BOOL bWOW64;
  ::IsWow64Process (GetCurrentProcess (), &bWOW64);

  if (bWOW64)
    GetSystemWow64Directory (wszBackendDLL, MAX_PATH);
  else
    GetSystemDirectory (wszBackendDLL, MAX_PATH);
#endif

  lstrcatW (wszBackendDLL, L"\\d3d9.dll");

  if (local_d3d9 == 0)
    local_d3d9 = LoadLibraryW (wszBackendDLL);
  else {
    HMODULE hMod;
    GetModuleHandleEx (0x00, wszBackendDLL, &hMod);
  }

  return local_d3d9;
}

void
SK_FreeRealD3D9 (void)
{
  // Decrease the reference count
  FreeLibrary (local_d3d9);
}


typedef void (WINAPI *finish_pfn)(void);

void
WINAPI
d3d9_init_callback (finish_pfn finish)
{
  dll_log.Log (L"[   D3D9   ] Importing Direct3DCreate9{Ex}...");
  dll_log.Log (L"[   D3D9   ] ================================");

  dll_log.Log (L"[   D3D9   ]   Direct3DCreate9:   %08Xh",
    (Direct3DCreate9_Import) =  \
      (Direct3DCreate9PROC)GetProcAddress (backend_dll, "Direct3DCreate9"));
  dll_log.Log (L"[   D3D9   ]   Direct3DCreate9Ex: %08Xh",
    (Direct3DCreate9Ex_Import) =  \
      (Direct3DCreate9ExPROC)GetProcAddress (backend_dll, "Direct3DCreate9Ex"));

#if 0
  HookD3D9Ex (nullptr);
#else
  HANDLE hThread =
    (HANDLE)
      _beginthreadex ( nullptr,
                         0,
                           HookD3D9Ex,
                             nullptr,
                               0x00,
                                 nullptr );

  WaitForSingleObject (hThread, INFINITE);
  CloseHandle         (hThread);
#endif

  finish ();
}

bool
SK::D3D9::Startup (void)
{
  return SK_StartupCore (L"d3d9", d3d9_init_callback);
}

bool
SK::D3D9::Shutdown (void)
{
#if 0
  // The texture manager built-in to SK is derived from these ...
  //   until those projects are modified to use THIS texture manager,
  //     they need special treatment.
  if ( GetModuleHandle (L"tzfix.dll") == NULL &&
       GetModuleHandle (L"tsfix.dll") == NULL ) {
    sk::d3d9::tex_mgr.Shutdown ();
  }
#endif

  return SK_ShutdownCore (L"d3d9");
}


extern "C" MH_STATUS
WINAPI
SK_CreateVFTableHook ( LPCWSTR pwszFuncName,
                       LPVOID *ppVFTable,
                       DWORD   dwOffset,
                       LPVOID  pDetour,
                       LPVOID *ppOriginal );

#define D3D9_VIRTUAL_HOOK(_Base,_Index,_Name,_Override,_Original,_Type) {     \
  void** vftable = *(void***)*(_Base);                                        \
                                                                              \
  if ((_Original) == nullptr) {                                               \
    SK_CreateVFTableHook ( L##_Name,                                          \
                             vftable,                                         \
                               (_Index),                                      \
                                 (_Override),                                 \
                                   (LPVOID *)&(_Original));                   \
  }                                                                           \
}

extern "C" const wchar_t* SK_DescribeVirtualProtectFlags (DWORD dwProtect);

#define __PTR_SIZE   sizeof LPCVOID
#define __PAGE_PRIVS PAGE_EXECUTE_READWRITE

#define D3D9_VIRTUAL_OVERRIDE(_Base,_Index,_Name,_Override,_Original,_Type) { \
  void** vftable = *(void***)*_Base;                                          \
                                                                              \
  if (vftable [_Index] != _Override) {                                        \
    DWORD dwProtect;                                                          \
                                                                              \
    VirtualProtect (&vftable [_Index], __PTR_SIZE, __PAGE_PRIVS, &dwProtect); \
                                                                              \
    /*dll_log.Log (L" Old VFTable entry for %s: %08Xh  (Memory Policy: %s)",*/\
                 /*L##_Name, vftable [_Index],                              */\
                 /*SK_DescribeVirtualProtectFlags (dwProtect));             */\
                                                                              \
    if (_Original == NULL)                                                    \
      _Original = (##_Type)vftable [_Index];                                  \
                                                                              \
    /*dll_log.Log (L"  + %s: %08Xh", L#_Original, _Original);*/               \
                                                                              \
    vftable [_Index] = _Override;                                             \
                                                                              \
    VirtualProtect (&vftable [_Index], __PTR_SIZE, dwProtect, &dwProtect);    \
                                                                              \
    /*dll_log.Log(L" New VFTable entry for %s: %08Xh  (Memory Policy: %s)\n"*/\
                  /*,L##_Name, vftable [_Index],                            */\
                  /*SK_DescribeVirtualProtectFlags (dwProtect));            */\
  }                                                                           \
}

#define D3D9_INTERCEPT(_Base,_Index,_Name,_Override,_Original,_Type) { \
  if (config.render.d3d9.hook_type == 0) {                             \
    D3D9_VIRTUAL_HOOK (   _Base,   _Index, _Name, _Override,           \
                        _Original, _Type );                            \
  } else {                                                             \
    D3D9_VIRTUAL_OVERRIDE(_Base,   _Index, _Name, _Override,           \
                        _Original, _Type );                            \
  }                                                                    \
}

#define D3D9_CALL(_Ret, _Call) {                                      \
  dll_log.LogEx (true, L"[   D3D9   ]  Calling original function: "); \
  (_Ret) = (_Call);                                                   \
  dll_log.LogEx (false, L"(ret=%s)\n", SK_DescribeHRESULT (_Ret));    \
}

void
SK_D3D9_FixUpBehaviorFlags (DWORD& BehaviorFlags)
{
  if (BehaviorFlags & D3DCREATE_SOFTWARE_VERTEXPROCESSING) {
    dll_log.Log (L"[CompatHack] D3D9 Fixup: "
                 L"Software Vertex Processing Replaced with Mixed-Mode.");
    BehaviorFlags &= ~D3DCREATE_SOFTWARE_VERTEXPROCESSING;
    BehaviorFlags |=  D3DCREATE_MIXED_VERTEXPROCESSING;
  }
}

void
SK_D3D9_SetFPSTarget ( D3DPRESENT_PARAMETERS* pPresentationParameters,
                       D3DDISPLAYMODEEX*      pFullscreenMode = nullptr )
{
  int TargetFPS = config.render.framerate.target_fps;

  // First consider D3D9Ex FullScreen Mode
  int Refresh   = pFullscreenMode != nullptr ?
                    pFullscreenMode->RefreshRate :
                    0;

  // Then, use the presentation parameters
  if (Refresh == 0) {
    Refresh = pPresentationParameters != nullptr ?
                pPresentationParameters->FullScreen_RefreshRateInHz :
                0;
  }

  if (config.render.d3d9.refresh_rate != -1) {
    if (pFullscreenMode != nullptr) {
      Refresh = config.render.d3d9.refresh_rate;

      pFullscreenMode->RefreshRate =
        config.render.d3d9.refresh_rate;
    }

    if (pPresentationParameters != nullptr) {
      Refresh = config.render.d3d9.refresh_rate;

      pPresentationParameters->FullScreen_RefreshRateInHz =
        config.render.d3d9.refresh_rate;
    }
  }

  if (TargetFPS != 0 && Refresh != 0) {
    if (Refresh >= TargetFPS) {
      if (! (Refresh % TargetFPS)) {
        dll_log.Log ( L"[   D3D9   ]  >> Targeting %li FPS - using 1:%li VSYNC;"
                      L" (refresh = %li Hz)",
                        TargetFPS,
                          Refresh / TargetFPS,
                            Refresh );

        pPresentationParameters->SwapEffect           = D3DSWAPEFFECT_DISCARD;
        pPresentationParameters->PresentationInterval = Refresh / TargetFPS;

      } else {
        dll_log.Log ( L"[   D3D9   ]  >> Cannot target %li FPS using VSYNC - no such factor exists;"
                      L" (refresh = %li Hz)",
                        TargetFPS,
                          Refresh );
      }
    } else {
      dll_log.Log ( L"[   D3D9   ]  >> Cannot target %li FPS using VSYNC - higher than refresh rate;"
                    L" (refresh = %li Hz)",
                      TargetFPS,
                        Refresh );
    }
  }

  if (pPresentationParameters != nullptr) {
    if ( config.render.framerate.buffer_count != -1 &&
         config.render.framerate.buffer_count != 
           pPresentationParameters->BackBufferCount ) {
      dll_log.Log ( L"[   D3D9   ]  >> Backbuffer Override: (Requested=%lu, Override=%lu)",
                      pPresentationParameters->BackBufferCount,
                        config.render.framerate.buffer_count );
      pPresentationParameters->BackBufferCount =
        config.render.framerate.buffer_count;
    }

    if ( config.render.framerate.present_interval != -1 &&
         config.render.framerate.present_interval !=
            pPresentationParameters->PresentationInterval ) {
      dll_log.Log ( L"[   D3D9   ]  >> VSYNC Override: (Requested=1:%lu, Override=1:%lu)",
                      pPresentationParameters->PresentationInterval,
                        config.render.framerate.present_interval );
      pPresentationParameters->PresentationInterval =
        config.render.framerate.present_interval;
    }

    extern HWND hWndRender;

    if (hWndRender == 0 || (! IsWindow (hWndRender)))
      hWndRender = pPresentationParameters->hDeviceWindow;
  }
}

COM_DECLSPEC_NOTHROW
__declspec (noinline)
HRESULT
WINAPI D3D9PresentCallbackEx (IDirect3DDevice9Ex *This,
                   _In_ const RECT               *pSourceRect,
                   _In_ const RECT               *pDestRect,
                   _In_       HWND                hDestWindowOverride,
                   _In_ const RGNDATA            *pDirtyRegion,
                   _In_       DWORD               dwFlags)
{
  g_pD3D9Dev = This;

  SK_D3D9_UpdateRenderStats (nullptr, This);

  SK_BeginBufferSwap ();

  HRESULT hr = D3D9PresentEx_Original (This,
                                       pSourceRect,
                                       pDestRect,
                                       hDestWindowOverride,
                                       pDirtyRegion,
                                       dwFlags);

  if (! config.osd.pump)
    return SK_EndBufferSwap ( hr,
                                (IUnknown *)This );

  return hr;
}

COM_DECLSPEC_NOTHROW
__declspec (noinline)
HRESULT
WINAPI D3D9PresentCallback (IDirect3DDevice9 *This,
                 _In_ const RECT             *pSourceRect,
                 _In_ const RECT             *pDestRect,
                 _In_       HWND              hDestWindowOverride,
                 _In_ const RGNDATA          *pDirtyRegion)
{
  if (g_D3D9PresentParams.SwapEffect == D3DSWAPEFFECT_FLIPEX) {
    HRESULT hr =
      D3D9PresentCallbackEx ( (IDirect3DDevice9Ex *)This,
                                pSourceRect,
                                  pDestRect,
                                    hDestWindowOverride,
                                      pDirtyRegion,
                                        D3DPRESENT_FORCEIMMEDIATE |
                                        D3DPRESENT_DONOTWAIT );

    return hr;
  }

  g_pD3D9Dev = This;

  SK_D3D9_UpdateRenderStats (nullptr, This);

  SK_BeginBufferSwap ();

  HRESULT hr = D3D9Present_Original (This,
                                     pSourceRect,
                                     pDestRect,
                                     hDestWindowOverride,
                                     pDirtyRegion);

  if (! config.osd.pump)
    return SK_EndBufferSwap ( hr,
                              (IUnknown *)This );

  return hr;
}


#define D3D9_STUB_HRESULT(_Return, _Name, _Proto, _Args)                  \
  COM_DECLSPEC_NOTHROW __declspec (noinline) _Return STDMETHODCALLTYPE    \
  _Name _Proto {                                                          \
    WaitForInit ();                                                       \
                                                                          \
    typedef _Return (STDMETHODCALLTYPE *passthrough_pfn) _Proto;          \
    static passthrough_pfn _default_impl = nullptr;                       \
                                                                          \
    if (_default_impl == nullptr) {                                       \
      static const char* szName = #_Name;                                 \
      _default_impl = (passthrough_pfn)                                   \
        GetProcAddress (backend_dll, szName);                             \
                                                                          \
      if (_default_impl == nullptr) {                                     \
        dll_log.Log (                                                     \
          L"[   D3D9   ] Unable to locate symbol  %s in d3d9.dll",        \
          L#_Name);                                                       \
        return E_NOTIMPL;                                                 \
      }                                                                   \
    }                                                                     \
                                                                          \
    /*dll_log.Log (L"[!] %s %s - "                                      */\
             /*L"[Calling Thread: 0x%04x]",                             */\
      /*L#_Name, L#_Proto, GetCurrentThreadId ());                      */\
                                                                          \
    return _default_impl _Args;                                           \
}

#define D3D9_STUB_VOIDP(_Return, _Name, _Proto, _Args)                    \
  COM_DECLSPEC_NOTHROW __declspec (noinline) _Return STDMETHODCALLTYPE    \
  _Name _Proto {                                                          \
    WaitForInit ();                                                       \
                                                                          \
    typedef _Return (STDMETHODCALLTYPE *passthrough_pfn) _Proto;          \
    static passthrough_pfn _default_impl = nullptr;                       \
                                                                          \
    if (_default_impl == nullptr) {                                       \
      static const char* szName = #_Name;                                 \
      _default_impl = (passthrough_pfn)                                   \
        GetProcAddress (backend_dll, szName);                             \
                                                                          \
      if (_default_impl == nullptr) {                                     \
        dll_log.Log (                                                     \
          L"[   D3D9   ] Unable to locate symbol  %s in d3d9.dll",        \
          L#_Name);                                                       \
        return nullptr;                                                   \
      }                                                                   \
    }                                                                     \
                                                                          \
    /*dll_log.Log (L"[!] %s %s - "                                      */\
           /*L"[Calling Thread: 0x%04x]",                               */\
      /*L#_Name, L#_Proto, GetCurrentThreadId ());                      */\
                                                                          \
    return _default_impl _Args;                                           \
}

#define D3D9_STUB_VOID(_Return, _Name, _Proto, _Args)                     \
  COM_DECLSPEC_NOTHROW __declspec (noinline) _Return STDMETHODCALLTYPE    \
  _Name _Proto {                                                          \
    WaitForInit ();                                                       \
                                                                          \
    typedef _Return (STDMETHODCALLTYPE *passthrough_pfn) _Proto;          \
    static passthrough_pfn _default_impl = nullptr;                       \
                                                                          \
    if (_default_impl == nullptr) {                                       \
      static const char* szName = #_Name;                                 \
      _default_impl = (passthrough_pfn)                                   \
        GetProcAddress (backend_dll, szName);                             \
                                                                          \
      if (_default_impl == nullptr) {                                     \
        dll_log.Log (                                                     \
          L"[   D3D9   ] Unable to locate symbol  %s in d3d9.dll",        \
          L#_Name);                                                       \
        return;                                                           \
      }                                                                   \
    }                                                                     \
                                                                          \
    /*dll_log.Log (L"[!] %s %s - "                                      */\
             /*L"[Calling Thread: 0x%04x]",                             */\
      /*L#_Name, L#_Proto, GetCurrentThreadId ());                      */\
                                                                          \
    _default_impl _Args;                                                  \
}

#define D3D9_STUB_INT(_Return, _Name, _Proto, _Args)                      \
  COM_DECLSPEC_NOTHROW __declspec (noinline) _Return STDMETHODCALLTYPE    \
  _Name _Proto {                                                          \
    WaitForInit ();                                                       \
                                                                          \
    typedef _Return (STDMETHODCALLTYPE *passthrough_pfn) _Proto;          \
    static passthrough_pfn _default_impl = nullptr;                       \
                                                                          \
    if (_default_impl == nullptr) {                                       \
      static const char* szName = #_Name;                                 \
      _default_impl = (passthrough_pfn)                                   \
        GetProcAddress (backend_dll, szName);                             \
                                                                          \
      if (_default_impl == nullptr) {                                     \
        dll_log.Log (                                                     \
          L"[   D3D9   ] Unable to locate symbol  %s in d3d9.dll",        \
          L#_Name);                                                       \
        return 0;                                                         \
      }                                                                   \
    }                                                                     \
                                                                          \
    /*dll_log.Log (L"[!] %s %s - "                                      */\
             /*L"[Calling Thread: 0x%04x]",                             */\
      /*L#_Name, L#_Proto, GetCurrentThreadId ());                      */\
                                                                          \
    return _default_impl _Args;                                           \
}

D3D9_STUB_VOIDP   (void*, Direct3DShaderValidatorCreate, (void),
                                                         (    ))

D3D9_STUB_INT     (int,   D3DPERF_BeginEvent, (D3DCOLOR color, LPCWSTR name),
                                                       (color,         name))
//D3D9_STUB_INT     (int,   D3DPERF_EndEvent,   (void),          ( ))

D3D9_STUB_INT     (DWORD, D3DPERF_GetStatus,  (void),          ( ))
D3D9_STUB_VOID    (void,  D3DPERF_SetOptions, (DWORD options), (options))

D3D9_STUB_INT     (BOOL,  D3DPERF_QueryRepeatFrame, (void),    ( ))
D3D9_STUB_VOID    (void,  D3DPERF_SetMarker, (D3DCOLOR color, LPCWSTR name),
                                                      (color,         name))
D3D9_STUB_VOID    (void,  D3DPERF_SetRegion, (D3DCOLOR color, LPCWSTR name),
                                                      (color,         name))

COM_DECLSPEC_NOTHROW
__declspec (noinline)
int
STDMETHODCALLTYPE
D3DPERF_EndEvent (void)
{
  WaitForInit ();

  typedef int (STDMETHODCALLTYPE *passthrough_pfn) (void);

  static passthrough_pfn _default_impl = nullptr;

  if (_default_impl == nullptr) {
    static const char* szName = "D3DPERF_EndEvent";
    _default_impl = (passthrough_pfn)GetProcAddress (backend_dll, szName);

    if (_default_impl == nullptr) {
      dll_log.Log (
          L"[   D3D9   ] Unable to locate symbol  %s in d3d9.dll",
          L"D3DPERF_EndEvent");
      return 0;
    }
  }

  return _default_impl ();
}

typedef HRESULT (STDMETHODCALLTYPE *CreateDXGIFactory_pfn)(REFIID,IDXGIFactory**);
typedef HRESULT (STDMETHODCALLTYPE *GetSwapChain_pfn)
  (IDirect3DDevice9* This, UINT iSwapChain, IDirect3DSwapChain9** pSwapChain);

typedef HRESULT (STDMETHODCALLTYPE *CreateAdditionalSwapChain_pfn)
  (IDirect3DDevice9* This, D3DPRESENT_PARAMETERS* pPresentationParameters,
   IDirect3DSwapChain9** pSwapChain);

CreateAdditionalSwapChain_pfn D3D9CreateAdditionalSwapChain_Original = nullptr;

  COM_DECLSPEC_NOTHROW
  __declspec (noinline)
  HRESULT
  WINAPI D3D9PresentSwapCallback (IDirect3DSwapChain9 *This,
                          _In_ const RECT             *pSourceRect,
                          _In_ const RECT             *pDestRect,
                          _In_       HWND              hDestWindowOverride,
                          _In_ const RGNDATA          *pDirtyRegion,
                          _In_       DWORD             dwFlags)
  {
    SK_D3D9_UpdateRenderStats (This);

    SK_BeginBufferSwap ();

    HRESULT hr = D3D9PresentSwap_Original (This,
                                           pSourceRect,
                                           pDestRect,
                                           hDestWindowOverride,
                                           pDirtyRegion,
                                           dwFlags);

    // We are manually pumping OSD updates, do not do them on buffer swaps.
    if (config.osd.pump) {
      return hr;
    }

    //
    // Get the Device Pointer so we can check the SLI state through NvAPI
    //
    IDirect3DDevice9* dev = nullptr;

    if (SUCCEEDED (This->GetDevice (&dev)) && dev != nullptr) {
      HRESULT ret = SK_EndBufferSwap ( hr,
                                       (IUnknown *)dev );
      ((IUnknown *)dev)->Release ();
      return ret;
    }

    return SK_EndBufferSwap (hr);
  }

COM_DECLSPEC_NOTHROW
__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9CreateAdditionalSwapChain_Override (
    IDirect3DDevice9       *This,
    D3DPRESENT_PARAMETERS  *pPresentationParameters,
    IDirect3DSwapChain9   **pSwapChain
  )
{
  dll_log.Log (L"[   D3D9   ] [!] %s (%08Xh, %08Xh, %08Xh) - "
    L"[Calling Thread: 0x%04x]",
    L"IDirect3DDevice9::CreateAdditionalSwapChain", This,
    pPresentationParameters, pSwapChain, GetCurrentThreadId ()
  );

  HRESULT hr;

  D3D9_CALL (hr,D3D9CreateAdditionalSwapChain_Original(This,
                                                       pPresentationParameters,
                                                       pSwapChain));

  if (SUCCEEDED (hr)) {
    D3D9_INTERCEPT ( pSwapChain, 3,
                     "IDirect3DSwapChain9::Present",
                     D3D9PresentSwapCallback, D3D9PresentSwap_Original,
                     D3D9PresentSwapChain_pfn );
  }

  return hr;
}

typedef HRESULT (STDMETHODCALLTYPE *BeginScene_pfn)
  (IDirect3DDevice9* This);

BeginScene_pfn D3D9BeginScene_Original = nullptr;

COM_DECLSPEC_NOTHROW
__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9BeginScene_Override (IDirect3DDevice9* This)
{
  HRESULT hr;

  hr = D3D9BeginScene_Original (This);

  //D3D9_CALL (hr, D3D9Begincene_Original (This));

  return hr;
}

typedef HRESULT (STDMETHODCALLTYPE *EndScene_pfn)
  (IDirect3DDevice9* This);

EndScene_pfn D3D9EndScene_Original = nullptr;

COM_DECLSPEC_NOTHROW
__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9EndScene_Override (IDirect3DDevice9* This)
{
  //dll_log.Log (L"[   D3D9   ] [!] %s (%08Xh) - "
    //L"[Calling Thread: 0x%04x]",
    //L"IDirect3DDevice9::EndScene", This,
    //GetCurrentThreadId ()
  //);

  HRESULT hr;

  hr = D3D9EndScene_Original (This);

  return hr;
}

void
SK_D3D9_HookPresent (IDirect3DDevice9 *pDev)
{
  static LPVOID vftable_17  = nullptr;
  static LPVOID vftable_121 = nullptr;

  void** vftable = *(void***)*&pDev;

  if (D3D9Present_Original != nullptr) {
    if (config.render.d3d9.hook_type == 0) {
      //dll_log.Log (L"Rehooking IDirect3DDevice9::Present (...)");

      if (MH_OK == SK_RemoveHook (vftable [17]))
        D3D9Present_Original = nullptr;
      else {
        dll_log.Log ( L"[   D3D9   ] Altered vftable detected, re-hooking "
                      L"IDirect3DDevice9::Present (...)!" );
        if (MH_OK == SK_RemoveHook (vftable_17))
          D3D9Present_Original = nullptr;
      }
    }
  }

  D3D9_INTERCEPT ( &pDev, 17,
                   "IDirect3DDevice9::Present",
                    D3D9PresentCallback,
                    D3D9Present_Original,
                    D3D9PresentDevice_pfn );

  vftable_17 = vftable [17];

  CComPtr <IDirect3DDevice9Ex> pDevEx;

  if (SUCCEEDED (pDev->QueryInterface (IID_PPV_ARGS (&pDevEx))))
  {
    vftable = *(void***)*&pDevEx;

    if (D3D9PresentEx_Original != nullptr) {
      if (config.render.d3d9.hook_type == 0) {
        //dll_log.Log (L"Rehooking IDirect3DDevice9Ex::PresentEx (...)");

        if (MH_OK == SK_RemoveHook (vftable [121]))
          D3D9PresentEx_Original = nullptr;
        else {
          dll_log.Log ( L"[   D3D9   ] Altered vftable detectd, re-hooking "
                        L"IDirect3DDevice9Ex::PresentEx (...)!" );
          if (MH_OK == SK_RemoveHook (vftable_121))
            D3D9PresentEx_Original = nullptr;
        }
      }
    }

    //
    // D3D9Ex Specific Stuff
    //
    D3D9_INTERCEPT ( &pDevEx, 121,
                       "IDirect3DDevice9Ex::PresentEx",
                        D3D9PresentCallbackEx,
                        D3D9PresentEx_Original,
                        D3D9PresentDeviceEx_pfn );

    vftable_121 = vftable [121];
  }
}

COM_DECLSPEC_NOTHROW
__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9Reset_Override ( IDirect3DDevice9      *This,
                     D3DPRESENT_PARAMETERS *pPresentationParameters )
{
  dll_log.Log ( L"[   D3D9   ] [!] %s (%08Xh, %08Xh) - "
                L"[Calling Thread: 0x%04x]",
                L"IDirect3DDevice9::Reset", This, pPresentationParameters,
                                GetCurrentThreadId ()
  );

  SK_D3D9_SetFPSTarget    (      pPresentationParameters);
  SK_SetPresentParamsD3D9 (This, pPresentationParameters);

#if 0
  // The texture manager built-in to SK is derived from these ...
  //   until those projects are modified to use THIS texture manager,
  //     they need special treatment.
  if ( GetModuleHandle (L"tzfix.dll") == NULL &&
       GetModuleHandle (L"tsfix.dll") == NULL ) {
    sk::d3d9::tex_mgr.reset ();
  }
#endif

  HRESULT hr;

  D3D9_CALL (hr, D3D9Reset_Original (This,
                                      pPresentationParameters));

  SK_D3D9_HookPresent (This);

  return hr;
}

COM_DECLSPEC_NOTHROW
__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9ResetEx ( IDirect3DDevice9Ex    *This,
              D3DPRESENT_PARAMETERS *pPresentationParameters,
              D3DDISPLAYMODEEX      *pFullscreenDisplayMode )
{
  dll_log.Log ( L"[   D3D9   ] [!] %s (%08Xh, %08Xh, %08Xh) - "
                L"[Calling Thread: 0x%04x]",
                  L"IDirect3DDevice9Ex::ResetEx",
                    This, pPresentationParameters, pFullscreenDisplayMode,
                      GetCurrentThreadId () );

  SK_D3D9_SetFPSTarget    (      pPresentationParameters, pFullscreenDisplayMode);
  SK_SetPresentParamsD3D9 (This, pPresentationParameters);

  HRESULT hr;

  D3D9_CALL (hr, D3D9ResetEx_Original ( This,
                                          pPresentationParameters,
                                            pFullscreenDisplayMode ));

  CComPtr <IDirect3DDevice9> pDev = nullptr;

  if (SUCCEEDED (This->QueryInterface ( IID_PPV_ARGS (&pDev) ))) {
    SK_D3D9_HookPresent (pDev);
  }

  return hr;
}

typedef HRESULT (STDMETHODCALLTYPE *DrawPrimitive_pfn)
  ( IDirect3DDevice9* This,
    D3DPRIMITIVETYPE  PrimitiveType,
    UINT              StartVertex,
    UINT              PrimitiveCount );

DrawPrimitive_pfn D3D9DrawPrimitive_Original = nullptr;

COM_DECLSPEC_NOTHROW
__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9DrawPrimitive_Override ( IDirect3DDevice9* This,
                             D3DPRIMITIVETYPE  PrimitiveType,
                             UINT              StartVertex,
                             UINT              PrimitiveCount )
{
  return
    D3D9DrawPrimitive_Original ( This,
                                   PrimitiveType,
                                     StartVertex,
                                       PrimitiveCount );
}

typedef HRESULT (STDMETHODCALLTYPE *DrawIndexedPrimitive_pfn)
  ( IDirect3DDevice9* This,
    D3DPRIMITIVETYPE  Type,
    INT               BaseVertexIndex,
    UINT              MinVertexIndex,
    UINT              NumVertices,
    UINT              startIndex,
    UINT              primCount );

DrawIndexedPrimitive_pfn D3D9DrawIndexedPrimitive_Original = nullptr;

COM_DECLSPEC_NOTHROW
__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9DrawIndexedPrimitive_Override ( IDirect3DDevice9* This,
                                    D3DPRIMITIVETYPE  Type,
                                    INT               BaseVertexIndex,
                                    UINT              MinVertexIndex,
                                    UINT              NumVertices,
                                    UINT              startIndex,
                                    UINT              primCount )
{
  return
    D3D9DrawIndexedPrimitive_Original ( This,
                                          Type,
                                            BaseVertexIndex,
                                              MinVertexIndex,
                                                NumVertices,
                                                  startIndex,
                                                    primCount );
}

typedef HRESULT (STDMETHODCALLTYPE *DrawPrimitiveUP_pfn)
  ( IDirect3DDevice9* This,
    D3DPRIMITIVETYPE  PrimitiveType,
    UINT              PrimitiveCount,
    const void       *pVertexStreamZeroData,
    UINT              VertexStreamZeroStride );

DrawPrimitiveUP_pfn D3D9DrawPrimitiveUP_Original = nullptr;

COM_DECLSPEC_NOTHROW
__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9DrawPrimitiveUP_Override ( IDirect3DDevice9* This,
                               D3DPRIMITIVETYPE  PrimitiveType,
                               UINT              PrimitiveCount,
                               const void       *pVertexStreamZeroData,
                               UINT              VertexStreamZeroStride )
{
  return
    D3D9DrawPrimitiveUP_Original ( This,
                                     PrimitiveType,
                                       PrimitiveCount,
                                         pVertexStreamZeroData,
                                           VertexStreamZeroStride );
}

typedef HRESULT (STDMETHODCALLTYPE *DrawIndexedPrimitiveUP_pfn)
  ( IDirect3DDevice9* This,
    D3DPRIMITIVETYPE  PrimitiveType,
    UINT              MinVertexIndex,
    UINT              NumVertices,
    UINT              PrimitiveCount,
    const void       *pIndexData,
    D3DFORMAT         IndexDataFormat,
    const void       *pVertexStreamZeroData,
    UINT              VertexStreamZeroStride );

DrawIndexedPrimitiveUP_pfn D3D9DrawIndexedPrimitiveUP_Original = nullptr;

COM_DECLSPEC_NOTHROW
__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9DrawIndexedPrimitiveUP_Override ( IDirect3DDevice9* This,
                                      D3DPRIMITIVETYPE  PrimitiveType,
                                      UINT              MinVertexIndex,
                                      UINT              NumVertices,
                                      UINT              PrimitiveCount,
                                      const void       *pIndexData,
                                      D3DFORMAT         IndexDataFormat,
                                      const void       *pVertexStreamZeroData,
                                      UINT              VertexStreamZeroStride )
{
  return
    D3D9DrawIndexedPrimitiveUP_Original (
      This,
        PrimitiveType,
          MinVertexIndex,
            NumVertices,
              PrimitiveCount,
                pIndexData,
                  IndexDataFormat,
                    pVertexStreamZeroData,
                      VertexStreamZeroStride );
}



typedef HRESULT (STDMETHODCALLTYPE *SetTexture_pfn)
  (     IDirect3DDevice9      *This,
   _In_ DWORD                  Sampler,
   _In_ IDirect3DBaseTexture9 *pTexture);

SetTexture_pfn D3D9SetTexture_Original = nullptr;

COM_DECLSPEC_NOTHROW
__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9SetTexture_Override ( IDirect3DDevice9      *This,
                    _In_  DWORD                  Sampler,
                    _In_  IDirect3DBaseTexture9 *pTexture )
{
  return D3D9SetTexture_Original (This, Sampler, pTexture);
}

typedef HRESULT (STDMETHODCALLTYPE *SetSamplerState_pfn)
  (IDirect3DDevice9*   This,
   DWORD               Sampler,
   D3DSAMPLERSTATETYPE Type,
   DWORD               Value);

SetSamplerState_pfn D3D9SetSamplerState_Original = nullptr;

COM_DECLSPEC_NOTHROW
__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9SetSamplerState_Override (IDirect3DDevice9*   This,
                              DWORD               Sampler,
                              D3DSAMPLERSTATETYPE Type,
                              DWORD               Value)
{
  return D3D9SetSamplerState_Original (This, Sampler, Type, Value);
}


typedef HRESULT (STDMETHODCALLTYPE *SetViewport_pfn)
  (      IDirect3DDevice9* This,
   CONST D3DVIEWPORT9*     pViewport);

SetViewport_pfn D3D9SetViewport_Original = nullptr;

COM_DECLSPEC_NOTHROW
__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9SetViewport_Override (IDirect3DDevice9* This,
                    CONST D3DVIEWPORT9*     pViewport)
{
  return D3D9SetViewport_Original (This, pViewport);
}


typedef HRESULT (STDMETHODCALLTYPE *SetRenderState_pfn)
  (IDirect3DDevice9*  This,
   D3DRENDERSTATETYPE State,
   DWORD              Value);

SetRenderState_pfn D3D9SetRenderState_Original = nullptr;

COM_DECLSPEC_NOTHROW
__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9SetRenderState_Override (IDirect3DDevice9*  This,
                             D3DRENDERSTATETYPE State,
                             DWORD              Value)
{
  return D3D9SetRenderState_Original (This, State, Value);
}


typedef HRESULT (STDMETHODCALLTYPE *SetVertexShaderConstantF_pfn)
  (IDirect3DDevice9* This,
    UINT             StartRegister,
    CONST float*     pConstantData,
    UINT             Vector4fCount);

SetVertexShaderConstantF_pfn D3D9SetVertexShaderConstantF_Original = nullptr;

__declspec (dllexport)
COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9SetVertexShaderConstantF_Override (IDirect3DDevice9* This,
                                       UINT             StartRegister,
                                       CONST float*     pConstantData,
                                       UINT             Vector4fCount)
{
  return D3D9SetVertexShaderConstantF_Original (This, StartRegister, pConstantData, Vector4fCount);
}


typedef HRESULT (STDMETHODCALLTYPE *SetPixelShaderConstantF_pfn)
  (IDirect3DDevice9* This,
    UINT             StartRegister,
    CONST float*     pConstantData,
    UINT             Vector4fCount);

SetPixelShaderConstantF_pfn D3D9SetPixelShaderConstantF_Original = nullptr;

COM_DECLSPEC_NOTHROW
__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9SetPixelShaderConstantF_Override (IDirect3DDevice9* This,
                                      UINT             StartRegister,
                                      CONST float*     pConstantData,
                                      UINT             Vector4fCount)
{
  return D3D9SetPixelShaderConstantF_Original (This, StartRegister, pConstantData, Vector4fCount);
}



struct IDirect3DPixelShader9;

typedef HRESULT (STDMETHODCALLTYPE *SetPixelShader_pfn)
  (IDirect3DDevice9*      This,
   IDirect3DPixelShader9* pShader);

SetPixelShader_pfn D3D9SetPixelShader_Original = nullptr;

COM_DECLSPEC_NOTHROW
__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9SetPixelShader_Override (IDirect3DDevice9* This,
                        IDirect3DPixelShader9* pShader)
{
  return D3D9SetPixelShader_Original (This, pShader);
}


struct IDirect3DVertexShader9;

typedef HRESULT (STDMETHODCALLTYPE *SetVertexShader_pfn)
  (IDirect3DDevice9*       This,
   IDirect3DVertexShader9* pShader);

SetVertexShader_pfn D3D9SetVertexShader_Original = nullptr;

COM_DECLSPEC_NOTHROW
__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9SetVertexShader_Override (IDirect3DDevice9* This,
                        IDirect3DVertexShader9* pShader)
{
  return D3D9SetVertexShader_Original (This, pShader);
}


typedef HRESULT (STDMETHODCALLTYPE *SetScissorRect_pfn)
  (IDirect3DDevice9* This,
   CONST RECT*       pRect);

SetScissorRect_pfn D3D9SetScissorRect_Original = nullptr;

COM_DECLSPEC_NOTHROW
__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9SetScissorRect_Override (IDirect3DDevice9* This,
                                   const RECT* pRect)
{
  return D3D9SetScissorRect_Original (This, pRect);
}

typedef HRESULT (STDMETHODCALLTYPE *CreateTexture_pfn)
  (IDirect3DDevice9   *This,
   UINT                Width,
   UINT                Height,
   UINT                Levels,
   DWORD               Usage,
   D3DFORMAT           Format,
   D3DPOOL             Pool,
   IDirect3DTexture9 **ppTexture,
   HANDLE             *pSharedHandle);

CreateTexture_pfn D3D9CreateTexture_Original = nullptr;

COM_DECLSPEC_NOTHROW
__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9CreateTexture_Override (IDirect3DDevice9   *This,
                            UINT                Width,
                            UINT                Height,
                            UINT                Levels,
                            DWORD               Usage,
                            D3DFORMAT           Format,
                            D3DPOOL             Pool,
                            IDirect3DTexture9 **ppTexture,
                            HANDLE             *pSharedHandle)
{
  return D3D9CreateTexture_Original (This, Width, Height, Levels, Usage,
                                     Format, Pool, ppTexture, pSharedHandle);
}

struct IDirect3DSurface9;

typedef HRESULT (STDMETHODCALLTYPE *CreateRenderTarget_pfn)
  (IDirect3DDevice9     *This,
   UINT                  Width,
   UINT                  Height,
   D3DFORMAT             Format,
   D3DMULTISAMPLE_TYPE   MultiSample,
   DWORD                 MultisampleQuality,
   BOOL                  Lockable,
   IDirect3DSurface9   **ppSurface,
   HANDLE               *pSharedHandle);

CreateRenderTarget_pfn D3D9CreateRenderTarget_Original = nullptr;

COM_DECLSPEC_NOTHROW
__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9CreateRenderTarget_Override (IDirect3DDevice9     *This,
                                 UINT                  Width,
                                 UINT                  Height,
                                 D3DFORMAT             Format,
                                 D3DMULTISAMPLE_TYPE   MultiSample,
                                 DWORD                 MultisampleQuality,
                                 BOOL                  Lockable,
                                 IDirect3DSurface9   **ppSurface,
                                 HANDLE               *pSharedHandle)
{
  return D3D9CreateRenderTarget_Original (This, Width, Height, Format,
                                          MultiSample, MultisampleQuality,
                                          Lockable, ppSurface, pSharedHandle);
}

typedef HRESULT (STDMETHODCALLTYPE *CreateDepthStencilSurface_pfn)
  (IDirect3DDevice9     *This,
   UINT                  Width,
   UINT                  Height,
   D3DFORMAT             Format,
   D3DMULTISAMPLE_TYPE   MultiSample,
   DWORD                 MultisampleQuality,
   BOOL                  Discard,
   IDirect3DSurface9   **ppSurface,
   HANDLE               *pSharedHandle);

CreateDepthStencilSurface_pfn D3D9CreateDepthStencilSurface_Original = nullptr;

COM_DECLSPEC_NOTHROW
__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9CreateDepthStencilSurface_Override (IDirect3DDevice9     *This,
                                        UINT                  Width,
                                        UINT                  Height,
                                        D3DFORMAT             Format,
                                        D3DMULTISAMPLE_TYPE   MultiSample,
                                        DWORD                 MultisampleQuality,
                                        BOOL                  Discard,
                                        IDirect3DSurface9   **ppSurface,
                                        HANDLE               *pSharedHandle)
{
  return D3D9CreateDepthStencilSurface_Original (This, Width, Height, Format,
                                                 MultiSample, MultisampleQuality,
                                                 Discard, ppSurface, pSharedHandle);
}

typedef HRESULT (STDMETHODCALLTYPE *SetRenderTarget_pfn)
  (IDirect3DDevice9  *This,
   DWORD              RenderTargetIndex,
   IDirect3DSurface9 *pRenderTarget);

SetRenderTarget_pfn D3D9SetRenderTarget_Original = nullptr;

__declspec (dllexport)
COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9SetRenderTarget_Override (IDirect3DDevice9  *This,
                              DWORD              RenderTargetIndex,
                              IDirect3DSurface9 *pRenderTarget)
{
  return D3D9SetRenderTarget_Original (This, RenderTargetIndex, pRenderTarget);
}

typedef HRESULT (STDMETHODCALLTYPE *SetDepthStencilSurface_pfn)
  (IDirect3DDevice9  *This,
   IDirect3DSurface9 *pNewZStencil);

SetDepthStencilSurface_pfn D3D9SetDepthStencilSurface_Original = nullptr;

__declspec (dllexport)
COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9SetDepthStencilSurface_Override (IDirect3DDevice9  *This,
                                     IDirect3DSurface9 *pNewZStencil)
{
  return D3D9SetDepthStencilSurface_Original (This, pNewZStencil);
}

struct IDirect3DBaseTexture9;

typedef HRESULT (STDMETHODCALLTYPE *UpdateTexture_pfn)
  (IDirect3DDevice9      *This,
   IDirect3DBaseTexture9 *pSourceTexture,
   IDirect3DBaseTexture9 *pDestinationTexture);

UpdateTexture_pfn D3D9UpdateTexture_Original = nullptr;

COM_DECLSPEC_NOTHROW
__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9UpdateTexture_Override (IDirect3DDevice9      *This,
                            IDirect3DBaseTexture9 *pSourceTexture,
                            IDirect3DBaseTexture9 *pDestinationTexture)
{
  return D3D9UpdateTexture_Original ( This,
                                        pSourceTexture,
                                          pDestinationTexture );
}

typedef HRESULT (STDMETHODCALLTYPE *StretchRect_pfn)
  (      IDirect3DDevice9    *This,
         IDirect3DSurface9   *pSourceSurface,
   const RECT                *pSourceRect,
         IDirect3DSurface9   *pDestSurface,
   const RECT                *pDestRect,
         D3DTEXTUREFILTERTYPE Filter);

StretchRect_pfn D3D9StretchRect_Original = nullptr;

COM_DECLSPEC_NOTHROW
__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9StretchRect_Override (      IDirect3DDevice9    *This,
                                IDirect3DSurface9   *pSourceSurface,
                          const RECT                *pSourceRect,
                                IDirect3DSurface9   *pDestSurface,
                          const RECT                *pDestRect,
                                D3DTEXTUREFILTERTYPE Filter )
{
  return D3D9StretchRect_Original ( This,
                                      pSourceSurface,
                                      pSourceRect,
                                        pDestSurface,
                                        pDestRect,
                                          Filter );
}

typedef HRESULT (STDMETHODCALLTYPE *SetCursorPosition_pfn)
(
       IDirect3DDevice9 *This,
  _In_ INT               X,
  _In_ INT               Y,
  _In_ DWORD             Flags
);

SetCursorPosition_pfn D3D9SetCursorPosition_Original = nullptr;

COM_DECLSPEC_NOTHROW
__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9SetCursorPosition_Override (      IDirect3DDevice9 *This,
                                 _In_ INT               X,
                                 _In_ INT               Y,
                                 _In_ DWORD             Flags )
{
  return D3D9SetCursorPosition_Original ( This,
                                            X,
                                              Y,
                                                Flags );
}

__declspec (noinline)
D3DPRESENT_PARAMETERS*
WINAPI
SK_SetPresentParamsD3D9 (IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pparams)
{
  memcpy (&g_D3D9PresentParams, pparams, sizeof D3DPRESENT_PARAMETERS);
  return pparams;
}

COM_DECLSPEC_NOTHROW
__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9CreateDeviceEx_Override (IDirect3D9Ex           *This,
                             UINT                    Adapter,
                             D3DDEVTYPE              DeviceType,
                             HWND                    hFocusWindow,
                             DWORD                   BehaviorFlags,
                             D3DPRESENT_PARAMETERS  *pPresentationParameters,
                             D3DDISPLAYMODEEX       *pFullscreenDisplayMode,
                             IDirect3DDevice9Ex    **ppReturnedDeviceInterface)
{
  dll_log.Log ( L"[   D3D9   ] [!] %s (%08Xh, %lu, %lu, %08Xh, 0x%04X, %08Xh, %08Xh, %08Xh) - "
                L"[Calling Thread: 0x%04x]",
                L"IDirect3D9Ex::CreateDeviceEx",
                  This, Adapter, (DWORD)DeviceType,
                    hFocusWindow, BehaviorFlags, pPresentationParameters,
                      pFullscreenDisplayMode, ppReturnedDeviceInterface,
                        GetCurrentThreadId () );

  SK_D3D9_SetFPSTarget       (pPresentationParameters, pFullscreenDisplayMode);
  SK_D3D9_FixUpBehaviorFlags (BehaviorFlags);

  HRESULT ret;

  D3D9_CALL (ret, D3D9CreateDeviceEx_Original (This,
                                               Adapter,
                                               DeviceType,
                                               hFocusWindow,
                                               BehaviorFlags,
                                               pPresentationParameters,
                                               pFullscreenDisplayMode,
                                               ppReturnedDeviceInterface));

  if (! SUCCEEDED (ret))
    return ret;

  SK_SetPresentParamsD3D9 ( (IDirect3DDevice9 *)*ppReturnedDeviceInterface,
                              pPresentationParameters );


      D3D9_INTERCEPT ( ppReturnedDeviceInterface, 11,
                          "IDirect3DDevice9::SetCursorPosition",
                          D3D9SetCursorPosition_Override,
                          D3D9SetCursorPosition_Original,
                          SetCursorPosition_pfn );

      D3D9_INTERCEPT ( ppReturnedDeviceInterface, 13,
                          "IDirect3DDevice9::CreateAdditionalSwapChain",
                          D3D9CreateAdditionalSwapChain_Override,
                          D3D9CreateAdditionalSwapChain_Original,
                          CreateAdditionalSwapChain_pfn );

      D3D9_INTERCEPT ( ppReturnedDeviceInterface, 23,
                          "IDirect3DDevice9::CreateTexture",
                          D3D9CreateTexture_Override,
                          D3D9CreateTexture_Original,
                          CreateTexture_pfn );

      D3D9_INTERCEPT ( ppReturnedDeviceInterface, 28,
                          "IDirect3DDevice9::CreateRenderTarget",
                          D3D9CreateRenderTarget_Override,
                          D3D9CreateRenderTarget_Original,
                          CreateRenderTarget_pfn );

      D3D9_INTERCEPT ( ppReturnedDeviceInterface, 29,
                          "IDirect3DDevice9::CreateDepthStencilSurface",
                          D3D9CreateDepthStencilSurface_Override,
                          D3D9CreateDepthStencilSurface_Original,
                          CreateDepthStencilSurface_pfn );

      D3D9_INTERCEPT ( ppReturnedDeviceInterface, 31,
                          "IDirect3DDevice9::UpdateTexture",
                          D3D9UpdateTexture_Override,
                          D3D9UpdateTexture_Original,
                          UpdateTexture_pfn );

      D3D9_INTERCEPT ( ppReturnedDeviceInterface, 34,
                          "IDirect3DDevice9::StretchRect",
                          D3D9StretchRect_Override,
                          D3D9StretchRect_Original,
                          StretchRect_pfn );

      D3D9_INTERCEPT ( ppReturnedDeviceInterface, 37,
                          "IDirect3DDevice9::SetRenderTarget",
                          D3D9SetRenderTarget_Override,
                          D3D9SetRenderTarget_Original,
                          SetRenderTarget_pfn );

      D3D9_INTERCEPT ( ppReturnedDeviceInterface, 39,
                       "IDirect3DDevice9::SetDepthStencilSurface",
                        D3D9SetDepthStencilSurface_Override,
                        D3D9SetDepthStencilSurface_Original,
                        SetDepthStencilSurface_pfn );

      D3D9_INTERCEPT ( ppReturnedDeviceInterface,
                       41,
                       "IDirect3DDevice9::BeginScene",
                        D3D9BeginScene_Override,
                        D3D9BeginScene_Original,
                        BeginScene_pfn );

      D3D9_INTERCEPT ( ppReturnedDeviceInterface,
                       42,
                       "IDirect3DDevice9::EndScene",
                        D3D9EndScene_Override,
                        D3D9EndScene_Original,
                        EndScene_pfn );

      D3D9_INTERCEPT ( ppReturnedDeviceInterface, 47,
                       "IDirect3DDevice9::SetViewport",
                        D3D9SetViewport_Override,
                        D3D9SetViewport_Original,
                        SetViewport_pfn );

      D3D9_INTERCEPT ( ppReturnedDeviceInterface, 57,
                       "IDirect3DDevice9::SetRenderState",
                        D3D9SetRenderState_Override,
                        D3D9SetRenderState_Original,
                        SetRenderState_pfn );

      D3D9_INTERCEPT ( ppReturnedDeviceInterface, 65,
                       "IDirect3DDevice9::SetTexture",
                        D3D9SetTexture_Override,
                        D3D9SetTexture_Original,
                        SetTexture_pfn );

      D3D9_INTERCEPT ( ppReturnedDeviceInterface, 69,
                       "IDirect3DDevice9::SetSamplerState",
                        D3D9SetSamplerState_Override,
                        D3D9SetSamplerState_Original,
                        SetSamplerState_pfn );

      D3D9_INTERCEPT ( ppReturnedDeviceInterface, 75,
                       "IDirect3DDevice9::SetScissorRect",
                        D3D9SetScissorRect_Override,
                        D3D9SetScissorRect_Original,
                        SetScissorRect_pfn );

      D3D9_INTERCEPT ( ppReturnedDeviceInterface, 81,
                       "IDirect3DDevice9::DrawPrimitive",
                        D3D9DrawPrimitive_Override,
                        D3D9DrawPrimitive_Original,
                        DrawPrimitive_pfn );

      D3D9_INTERCEPT ( ppReturnedDeviceInterface, 82,
                       "IDirect3DDevice9::DrawIndexedPrimitive",
                        D3D9DrawIndexedPrimitive_Override,
                        D3D9DrawIndexedPrimitive_Original,
                        DrawIndexedPrimitive_pfn );

      D3D9_INTERCEPT ( ppReturnedDeviceInterface, 83,
                       "IDirect3DDevice9::DrawPrimitiveUP",
                        D3D9DrawPrimitiveUP_Override,
                        D3D9DrawPrimitiveUP_Original,
                        DrawPrimitiveUP_pfn );

      D3D9_INTERCEPT ( ppReturnedDeviceInterface, 84,
                       "IDirect3DDevice9::DrawIndexedPrimitiveUP",
                        D3D9DrawIndexedPrimitiveUP_Override,
                        D3D9DrawIndexedPrimitiveUP_Original,
                        DrawIndexedPrimitiveUP_pfn );

#if 0
      D3D9_INTERCEPT ( ppReturnedDeviceInterface, 87,
                       "IDirect3DDevice9::SetVertexDeclaration",
                        D3D9SetVertexDeclaration_Override,
                        D3D9SetVertexDeclaration_Original,
                        SetVertexDeclaration_pfn );

      D3D9_INTERCEPT ( ppReturnedDeviceInterface, 89,
                       "IDirect3DDevice9::DrawIndexedPrimitive",
                        D3D9SetFVF_Override,
                        D3D9SetFVF_Original,
                        SetFVF_pfn );
#endif

      D3D9_INTERCEPT ( ppReturnedDeviceInterface, 92,
                       "IDirect3DDevice9::SetVertexShader",
                        D3D9SetVertexShader_Override,
                        D3D9SetVertexShader_Original,
                        SetVertexShader_pfn );

      D3D9_INTERCEPT ( ppReturnedDeviceInterface, 94,
                       "IDirect3DDevice9::SetSetVertexShaderConstantF",
                        D3D9SetVertexShaderConstantF_Override,
                        D3D9SetVertexShaderConstantF_Original,
                        SetVertexShaderConstantF_pfn );

      D3D9_INTERCEPT ( ppReturnedDeviceInterface, 107,
                       "IDirect3DDevice9::SetPixelShader",
                        D3D9SetPixelShader_Override,
                        D3D9SetPixelShader_Original,
                        SetPixelShader_pfn );

      D3D9_INTERCEPT ( ppReturnedDeviceInterface, 109,
                       "IDirect3DDevice9::SetPixelShaderConstantF",
                        D3D9SetPixelShaderConstantF_Override,
                        D3D9SetPixelShaderConstantF_Original,
                        SetPixelShaderConstantF_pfn );

  extern HWND hWndRender;
  if (hFocusWindow != 0)
    hWndRender = hFocusWindow;

  //// Allow the hooked function to change stuff immediately
  return S_OK; ////////return (*ppReturnedDeviceInterface)->Reset (pPresentationParameters);
}

HRESULT
__declspec (noinline)
WINAPI
D3D9CreateDevice_Override (IDirect3D9*            This,
                           UINT                   Adapter,
                           D3DDEVTYPE             DeviceType,
                           HWND                   hFocusWindow,
                           DWORD                  BehaviorFlags,
                           D3DPRESENT_PARAMETERS* pPresentationParameters,
                           IDirect3DDevice9**     ppReturnedDeviceInterface)
{
  dll_log.Log ( L"[   D3D9   ] [!] %s (%08Xh, %lu, %lu, %08Xh, 0x%04X, %08Xh, %08Xh) - "
                L"[Calling Thread: 0x%04x]",
                  L"IDirect3D9::CreateDevice", This, Adapter, (DWORD)DeviceType,
                    hFocusWindow, BehaviorFlags, pPresentationParameters,
                      ppReturnedDeviceInterface, GetCurrentThreadId ());

  SK_D3D9_SetFPSTarget       (pPresentationParameters);
  SK_D3D9_FixUpBehaviorFlags (BehaviorFlags);

  HRESULT ret;

  D3D9_CALL (ret, D3D9CreateDevice_Original ( This, Adapter,
                                          DeviceType,
                                            hFocusWindow,
                                              BehaviorFlags,
                                                pPresentationParameters,
                                                  ppReturnedDeviceInterface ));

  // Do not attempt to do vtable override stuff if this failed,
  //   that will cause an immediate crash! Instead log some information that
  //     might help diagnose the problem.
  if (! SUCCEEDED (ret)) {
    if (pPresentationParameters != nullptr) {
      dll_log.LogEx (true,
                L"[   D3D9   ]  SwapChain Settings:   Res=(%lux%lu), Format=0x%04X, "
                                        L"Count=%lu - "
                                        L"SwapEffect: 0x%02X, Flags: 0x%04X,"
                                        L"AutoDepthStencil: %s "
                                        L"PresentationInterval: %lu\n",
                   pPresentationParameters->BackBufferWidth,
                   pPresentationParameters->BackBufferHeight,
                   pPresentationParameters->BackBufferFormat,
                   pPresentationParameters->BackBufferCount,
                   pPresentationParameters->SwapEffect,
                   pPresentationParameters->Flags,
                   pPresentationParameters->EnableAutoDepthStencil ? L"true" :
                                                                     L"false",
                   pPresentationParameters->PresentationInterval);

      if (! pPresentationParameters->Windowed) {
        dll_log.LogEx (true,
                L"[   D3D9   ]  Fullscreen Settings:  Refresh Rate: %lu\n",
                   pPresentationParameters->FullScreen_RefreshRateInHz);
        dll_log.LogEx (true,
                L"[   D3D9   ]  Multisample Settings: Type: %X, Quality: %lu\n",
                   pPresentationParameters->MultiSampleType,
                   pPresentationParameters->MultiSampleQuality);
      }
    }

    return ret;
  }

  SK_SetPresentParamsD3D9 ( *ppReturnedDeviceInterface,
                               pPresentationParameters );


      D3D9_INTERCEPT ( ppReturnedDeviceInterface, 11,
                          "IDirect3DDevice9::SetCursorPosition",
                          D3D9SetCursorPosition_Override,
                          D3D9SetCursorPosition_Original,
                          SetCursorPosition_pfn );

      D3D9_INTERCEPT ( ppReturnedDeviceInterface, 13,
                          "IDirect3DDevice9::CreateAdditionalSwapChain",
                          D3D9CreateAdditionalSwapChain_Override,
                          D3D9CreateAdditionalSwapChain_Original,
                          CreateAdditionalSwapChain_pfn );

      D3D9_INTERCEPT ( ppReturnedDeviceInterface, 23,
                          "IDirect3DDevice9::CreateTexture",
                          D3D9CreateTexture_Override,
                          D3D9CreateTexture_Original,
                          CreateTexture_pfn );

      D3D9_INTERCEPT ( ppReturnedDeviceInterface, 28,
                          "IDirect3DDevice9::CreateRenderTarget",
                          D3D9CreateRenderTarget_Override,
                          D3D9CreateRenderTarget_Original,
                          CreateRenderTarget_pfn );

      D3D9_INTERCEPT ( ppReturnedDeviceInterface, 29,
                          "IDirect3DDevice9::CreateDepthStencilSurface",
                          D3D9CreateDepthStencilSurface_Override,
                          D3D9CreateDepthStencilSurface_Original,
                          CreateDepthStencilSurface_pfn );

      D3D9_INTERCEPT ( ppReturnedDeviceInterface, 31,
                          "IDirect3DDevice9::UpdateTexture",
                          D3D9UpdateTexture_Override,
                          D3D9UpdateTexture_Original,
                          UpdateTexture_pfn );

      D3D9_INTERCEPT ( ppReturnedDeviceInterface, 34,
                          "IDirect3DDevice9::StretchRect",
                          D3D9StretchRect_Override,
                          D3D9StretchRect_Original,
                          StretchRect_pfn );

      D3D9_INTERCEPT ( ppReturnedDeviceInterface, 37,
                          "IDirect3DDevice9::SetRenderTarget",
                          D3D9SetRenderTarget_Override,
                          D3D9SetRenderTarget_Original,
                          SetRenderTarget_pfn );

      D3D9_INTERCEPT ( ppReturnedDeviceInterface, 39,
                       "IDirect3DDevice9::SetDepthStencilSurface",
                        D3D9SetDepthStencilSurface_Override,
                        D3D9SetDepthStencilSurface_Original,
                        SetDepthStencilSurface_pfn );

      D3D9_INTERCEPT ( ppReturnedDeviceInterface,
                       41,
                       "IDirect3DDevice9::BeginScene",
                        D3D9BeginScene_Override,
                        D3D9BeginScene_Original,
                        BeginScene_pfn );

      D3D9_INTERCEPT ( ppReturnedDeviceInterface,
                       42,
                       "IDirect3DDevice9::EndScene",
                        D3D9EndScene_Override,
                        D3D9EndScene_Original,
                        EndScene_pfn );

      D3D9_INTERCEPT ( ppReturnedDeviceInterface, 47,
                       "IDirect3DDevice9::SetViewport",
                        D3D9SetViewport_Override,
                        D3D9SetViewport_Original,
                        SetViewport_pfn );

      D3D9_INTERCEPT ( ppReturnedDeviceInterface, 57,
                       "IDirect3DDevice9::SetRenderState",
                        D3D9SetRenderState_Override,
                        D3D9SetRenderState_Original,
                        SetRenderState_pfn );

      D3D9_INTERCEPT ( ppReturnedDeviceInterface, 65,
                       "IDirect3DDevice9::SetTexture",
                        D3D9SetTexture_Override,
                        D3D9SetTexture_Original,
                        SetTexture_pfn );

      D3D9_INTERCEPT ( ppReturnedDeviceInterface, 69,
                       "IDirect3DDevice9::SetSamplerState",
                        D3D9SetSamplerState_Override,
                        D3D9SetSamplerState_Original,
                        SetSamplerState_pfn );

      D3D9_INTERCEPT ( ppReturnedDeviceInterface, 75,
                       "IDirect3DDevice9::SetScissorRect",
                        D3D9SetScissorRect_Override,
                        D3D9SetScissorRect_Original,
                        SetScissorRect_pfn );

      D3D9_INTERCEPT ( ppReturnedDeviceInterface, 81,
                       "IDirect3DDevice9::DrawPrimitive",
                        D3D9DrawPrimitive_Override,
                        D3D9DrawPrimitive_Original,
                        DrawPrimitive_pfn );

      D3D9_INTERCEPT ( ppReturnedDeviceInterface, 82,
                       "IDirect3DDevice9::DrawIndexedPrimitive",
                        D3D9DrawIndexedPrimitive_Override,
                        D3D9DrawIndexedPrimitive_Original,
                        DrawIndexedPrimitive_pfn );

      D3D9_INTERCEPT ( ppReturnedDeviceInterface, 83,
                       "IDirect3DDevice9::DrawPrimitiveUP",
                        D3D9DrawPrimitiveUP_Override,
                        D3D9DrawPrimitiveUP_Original,
                        DrawPrimitiveUP_pfn );

      D3D9_INTERCEPT ( ppReturnedDeviceInterface, 84,
                       "IDirect3DDevice9::DrawIndexedPrimitiveUP",
                        D3D9DrawIndexedPrimitiveUP_Override,
                        D3D9DrawIndexedPrimitiveUP_Original,
                        DrawIndexedPrimitiveUP_pfn );

#if 0
      D3D9_INTERCEPT ( ppReturnedDeviceInterface, 87,
                       "IDirect3DDevice9::SetVertexDeclaration",
                        D3D9SetVertexDeclaration_Override,
                        D3D9SetVertexDeclaration_Original,
                        SetVertexDeclaration_pfn );

      D3D9_INTERCEPT ( ppReturnedDeviceInterface, 89,
                       "IDirect3DDevice9::DrawIndexedPrimitive",
                        D3D9SetFVF_Override,
                        D3D9SetFVF_Original,
                        SetFVF_pfn );
#endif

      D3D9_INTERCEPT ( ppReturnedDeviceInterface, 92,
                       "IDirect3DDevice9::SetVertexShader",
                        D3D9SetVertexShader_Override,
                        D3D9SetVertexShader_Original,
                        SetVertexShader_pfn );

      D3D9_INTERCEPT ( ppReturnedDeviceInterface, 94,
                       "IDirect3DDevice9::SetSetVertexShaderConstantF",
                        D3D9SetVertexShaderConstantF_Override,
                        D3D9SetVertexShaderConstantF_Original,
                        SetVertexShaderConstantF_pfn );

      D3D9_INTERCEPT ( ppReturnedDeviceInterface, 107,
                       "IDirect3DDevice9::SetPixelShader",
                        D3D9SetPixelShader_Override,
                        D3D9SetPixelShader_Original,
                        SetPixelShader_pfn );

      D3D9_INTERCEPT ( ppReturnedDeviceInterface, 109,
                       "IDirect3DDevice9::SetPixelShaderConstantF",
                        D3D9SetPixelShaderConstantF_Override,
                        D3D9SetPixelShaderConstantF_Original,
                        SetPixelShaderConstantF_pfn );

  extern HWND hWndRender;
  if (hFocusWindow != 0)
    hWndRender = hFocusWindow;

  //// Allow the hooked function to change stuff immediately (no, don't)
  return S_OK; ////////return (*ppReturnedDeviceInterface)->Reset (pPresentationParameters);
}


bool tex_init = false;

__declspec (noinline)
IDirect3D9*
STDMETHODCALLTYPE
Direct3DCreate9 (UINT SDKVersion)
{
  WaitForInit      ();
  WaitForInit_D3D9 ();

  bool force_d3d9ex = config.render.d3d9.force_d3d9ex;

  // Disable D3D9EX whenever GeDoSaTo is present
  if (force_d3d9ex) {
    if (GetModuleHandle (L"GeDoSaTo.dll")) {
      dll_log.Log (L"[CompatHack] <!> Disabling D3D9Ex optimizations because GeDoSaTo.dll is present!");
      force_d3d9ex = false;
    }
  }

  IDirect3D9*   d3d9  = nullptr;
  IDirect3D9Ex* pD3D9 = nullptr;

  if ((! force_d3d9ex) || FAILED (Direct3DCreate9Ex (SDKVersion, &pD3D9))) {
    if (Direct3DCreate9_Import) {
      dll_log.Log ( L"[   D3D9   ] [!] %s (%lu) - "
        L"[Calling Thread: 0x%04x]",
        L"Direct3DCreate9", SDKVersion, GetCurrentThreadId ());

      d3d9 = Direct3DCreate9_Import (SDKVersion);
    }
  } else {
    return pD3D9;
  }

  if (d3d9 != nullptr) {
    // ...
  }

  return d3d9;
}

HRESULT
__declspec (noinline)
STDMETHODCALLTYPE
Direct3DCreate9Ex (__in UINT SDKVersion, __out IDirect3D9Ex **ppD3D)
{
  WaitForInit      ();
  WaitForInit_D3D9 ();

  dll_log.Log ( L"[   D3D9   ] [!] %s (%lu, %08Xh) - "
                L"[Calling Thread: 0x%04x]",
                  L"Direct3DCreate9Ex",
                    SDKVersion,
                      ppD3D,
                        GetCurrentThreadId () );

  HRESULT hr = E_FAIL;

  IDirect3D9Ex* d3d9ex = nullptr;

  if (Direct3DCreate9Ex_Import) {
    D3D9_CALL (hr, Direct3DCreate9Ex_Import (SDKVersion, &d3d9ex));

    if (SUCCEEDED (hr) && d3d9ex!= nullptr) {
      *ppD3D = d3d9ex;
    }
  }

  return hr;
}

void
__stdcall
SK_D3D9_UpdateRenderStats (IDirect3DSwapChain9* pSwapChain, IDirect3DDevice9* pDevice)
{
  if (! ((pDevice || pSwapChain) && config.render.show))
    return;

  SK::D3D9::PipelineStatsD3D9& pipeline_stats =
    SK::D3D9::pipeline_stats_d3d9;

  CComPtr <IDirect3DDevice9> dev = pDevice;

  if (pDevice != nullptr || (pDevice == nullptr && SUCCEEDED (pSwapChain->GetDevice (&dev)))) {
    if (pipeline_stats.query.object != nullptr) {
      if (pipeline_stats.query.active) {
        pipeline_stats.query.object->Issue (D3DISSUE_END);
        pipeline_stats.query.active = false;
      } else {
        HRESULT hr =
          pipeline_stats.query.object->GetData (
                              &pipeline_stats.last_results,
                                sizeof D3DDEVINFO_D3D9PIPELINETIMINGS,
                                  D3DGETDATA_FLUSH );
        if (hr == S_OK) {
          pipeline_stats.query.object->Release ();
          pipeline_stats.query.object = nullptr;
        }
      }
    }

    else {
      if (SUCCEEDED (dev->CreateQuery (D3DQUERYTYPE_PIPELINETIMINGS, &pipeline_stats.query.object))) {
        pipeline_stats.query.object->Issue (D3DISSUE_BEGIN);
        pipeline_stats.query.active = true;
      }
    }
  }
}

std::wstring
SK::D3D9::getPipelineStatsDesc (void)
{
  wchar_t wszDesc [1024];

  D3DDEVINFO_D3D9PIPELINETIMINGS& stats =
    pipeline_stats_d3d9.last_results;

  //
  // VERTEX SHADING
  //
  if (stats.VertexProcessingTimePercent > 0.0f) {
    _swprintf ( wszDesc,
                  L"  VERTEX : %5.2f%%\n",
                    stats.VertexProcessingTimePercent );
  } else {
    _swprintf ( wszDesc,
                  L"  VERTEX : <Unused>\n" );
  }

  //
  // PIXEL SHADING
  //
  if (stats.PixelProcessingTimePercent > 0.0f) {
    _swprintf ( wszDesc,
                  L"%s  PIXEL  : %5.2f%%\n",
                    wszDesc,
                      stats.PixelProcessingTimePercent );
  } else {
    _swprintf ( wszDesc,
                  L"%s  PIXEL  : <Unused>\n",
                    wszDesc );
  }

  //
  // OTHER
  //
  if (stats.OtherGPUProcessingTimePercent > 0.0f) {
    _swprintf ( wszDesc,
                  L"%s  OTHER  : %5.2f%%\n",
                    wszDesc, stats.OtherGPUProcessingTimePercent);
  }

  //
  // IDLE
  //
  if (stats.GPUIdleTimePercent > 0.0f) {
    _swprintf ( wszDesc,
                  L"%s  IDLE   : %5.2f%%\n",
                    wszDesc,
                      stats.GPUIdleTimePercent );
  }

  return wszDesc;
}

unsigned int
__stdcall
HookD3D9Ex (LPVOID user)
{
  CComPtr <IDirect3D9Ex> pD3D9Ex = nullptr;

  HRESULT hr =
    Direct3DCreate9Ex_Import (D3D_SDK_VERSION, &pD3D9Ex);

  HWND hwnd = 0;

  if (SUCCEEDED (hr)) {
    hwnd =
      CreateWindowW ( L"STATIC", L"Dummy D3D9 Window",
                        WS_OVERLAPPEDWINDOW,
                          CW_USEDEFAULT, CW_USEDEFAULT,
                            800, 600, 0,
                              nullptr, nullptr, 0x00 );

    D3DPRESENT_PARAMETERS pparams;
    ZeroMemory (&pparams, sizeof (pparams));

    pparams.SwapEffect       = D3DSWAPEFFECT_FLIPEX;
    pparams.BackBufferFormat = D3DFMT_UNKNOWN;
    pparams.Windowed         = TRUE;

    CComPtr <IDirect3DDevice9Ex> pD3D9DevEx = nullptr;

    if ( SUCCEEDED ( pD3D9Ex->CreateDeviceEx (
                       D3DADAPTER_DEFAULT,
                         D3DDEVTYPE_HAL,
                           hwnd,
                             D3DCREATE_HARDWARE_VERTEXPROCESSING,
                               &pparams,
                                 nullptr,
                                   &pD3D9DevEx )
                   )
       )
    {
      static HMODULE hDXGI = LoadLibrary (L"dxgi.dll");
      static CreateDXGIFactory_pfn CreateDXGIFactory =
        (CreateDXGIFactory_pfn)GetProcAddress (hDXGI, "CreateDXGIFactory");

      IDXGIFactory* factory = nullptr;

      // Only spawn the DXGI 1.4 budget thread if ... DXGI 1.4 is implemented.
      if (SUCCEEDED (CreateDXGIFactory (__uuidof (IDXGIFactory4), &factory))) {
        IDXGIAdapter* adapter = nullptr;

        if (SUCCEEDED (factory->EnumAdapters (0, &adapter))) {
          SK_StartDXGI_1_4_BudgetThread (&adapter);

          adapter->Release ();
        }

        factory->Release ();
      }

      //dll_log.Log (L"Hooking D3D9Ex ...");

      D3D9_INTERCEPT ( &pD3D9Ex, 16,
                       "IDirect3D9::CreateDevice",
                        D3D9CreateDevice_Override,
                        D3D9CreateDevice_Original,
                        D3D9CreateDevice_pfn );

      D3D9_INTERCEPT ( &pD3D9Ex, 20,
                       "IDirect3D9Ex::CreateDeviceEx",
                        D3D9CreateDeviceEx_Override,
                        D3D9CreateDeviceEx_Original,
                        D3D9CreateDeviceEx_pfn );

      //vtbl (This)
      //-----------
      // 3   TestCooperativeLevel
      // 4   GetAvailableTextureMem
      // 5   EvictManagedResources
      // 6   GetDirect3D
      // 7   GetDeviceCaps
      // 8   GetDisplayMode
      // 9   GetCreationParameters
      // 10  SetCursorProperties
      // 11  SetCursorPosition
      // 12  ShowCursor
      // 13  CreateAdditionalSwapChain
      // 14  GetSwapChain
      // 15  GetNumberOfSwapChains
      // 16  Reset
      // 17  Present
      // 18  GetBackBuffer
      // 19  GetRasterStatus
      // 20  SetDialogBoxMode
      // 21  SetGammaRamp
      // 22  GetGammaRamp
      // 23  CreateTexture
      // 24  CreateVolumeTexture
      // 25  CreateCubeTexture
      // 26  CreateVertexBuffer
      // 27  CreateIndexBuffer
      // 28  CreateRenderTarget
      // 29  CreateDepthStencilSurface
      // 30  UpdateSurface
      // 31  UpdateTexture
      // 32  GetRenderTargetData
      // 33  GetFrontBufferData
      // 34  StretchRect
      // 35  ColorFill
      // 36  CreateOffscreenPlainSurface
      // 37  SetRenderTarget
      // 38  GetRenderTarget
      // 39  SetDepthStencilSurface
      // 40  GetDepthStencilSurface
      // 41  BeginScene
      // 42  EndScene
      // 43  Clear
      // 44  SetTransform
      // 45  GetTransform
      // 46  MultiplyTransform
      // 47  SetViewport
      // 48  GetViewport
      // 49  SetMaterial
      // 50  GetMaterial
      // 51  SetLight
      // 52  GetLight
      // 53  LightEnable
      // 54  GetLightEnable
      // 55  SetClipPlane
      // 56  GetClipPlane
      // 57  SetRenderState
      // 58  GetRenderState
      // 59  CreateStateBlock
      // 60  BeginStateBlock
      // 61  EndStateBlock
      // 62  SetClipStatus
      // 63  GetClipStatus
      // 64  GetTexture
      // 65  SetTexture
      // 66  GetTextureStageState
      // 67  SetTextureStageState
      // 68  GetSamplerState
      // 69  SetSamplerState
      // 70  ValidateDevice
      // 71  SetPaletteEntries
      // 72  GetPaletteEntries
      // 73  SetCurrentTexturePalette
      // 74  GetCurrentTexturePalette
      // 75  SetScissorRect
      // 76  GetScissorRect
      // 77  SetSoftwareVertexProcessing
      // 78  GetSoftwareVertexProcessing
      // 79  SetNPatchMode
      // 80  GetNPatchMode
      // 81  DrawPrimitive
      // 82  DrawIndexedPrimitive
      // 83  DrawPrimitiveUP
      // 84  DrawIndexedPrimitiveUP
      // 85  ProcessVertices
      // 86  CreateVertexDeclaration
      // 87  SetVertexDeclaration
      // 88  GetVertexDeclaration
      // 89  SetFVF
      // 90  GetFVF
      // 91  CreateVertexShader
      // 92  SetVertexShader
      // 93  GetVertexShader
      // 94  SetVertexShaderConstantF
      // 95  GetVertexShaderConstantF
      // 96  SetVertexShaderConstantI
      // 97  GetVertexShaderConstantI
      // 98  SetVertexShaderConstantB
      // 99  GetVertexShaderConstantB
      // 100 SetStreamSource
      // 101 GetStreamSource
      // 102 SetStreamSourceFreq
      // 103 GetStreamSourceFreq
      // 104 SetIndices
      // 105 GetIndices
      // 106 CreatePixelShader
      // 107 SetPixelShader
      // 108 GetPixelShader
      // 109 SetPixelShaderConstantF
      // 110 GetPixelShaderConstantF
      // 111 SetPixelShaderConstantI
      // 112 GetPixelShaderConstantI
      // 113 SetPixelShaderConstantB
      // 114 GetPixelShaderConstantB
      // 115 DrawRectPatch
      // 116 DrawTriPatch
      // 117 DeletePatch
      // 118 CreateQuery

      IDirect3DDevice9Ex**ppReturnedDeviceInterface = &pD3D9DevEx;

      D3D9_INTERCEPT ( ppReturnedDeviceInterface, 16,
                       "IDirect3DDevice9::Reset",
                        D3D9Reset_Override,
                        D3D9Reset_Original,
                        D3D9Reset_pfn );
      //
      // D3D9Ex Specific Stuff
      //
      D3D9_INTERCEPT ( ppReturnedDeviceInterface, 132,
                       "IDirect3DDevice9Ex::ResetEx",
                        D3D9ResetEx,
                        D3D9ResetEx_Original,
                        D3D9ResetEx_pfn );

      SK_D3D9_HookPresent (pD3D9DevEx);
    }
  }

  DestroyWindow (hwnd);

  InterlockedExchange (&__d3d9_ready, TRUE);

  return 0;
}