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

#include <d3d9.h>

#include <SpecialK/render_backend.h>
#include <SpecialK/dxgi_backend.h>
#include <SpecialK/nvapi.h>
#include <SpecialK/utility.h>
#include <SpecialK/config.h>
#include <SpecialK/core.h>
#include <SpecialK/command.h>
#include <SpecialK/framerate.h>
#include <SpecialK/log.h>
#include <SpecialK/import.h>

#include <atlbase.h>

//#undef NDEBUG
#include <cassert>

SK_RenderBackend __SK_RBkEnd;

SK_RenderBackend&
__stdcall
SK_GetCurrentRenderBackend (void)
{
  return __SK_RBkEnd;
}

extern void WINAPI SK_HookGL     (void);
extern void WINAPI SK_HookVulkan (void);
extern void WINAPI SK_HookD3D9   (void);
extern void WINAPI SK_HookD3D8   (void);
extern void WINAPI SK_HookDDraw  (void);
extern void WINAPI SK_HookDXGI   (void);

extern void
SK_D3D9_TriggerReset (bool);

void
__stdcall
SK_InitRenderBackends (void)
{
  SK_GetCommandProcessor ()->AddVariable ( "RenderHooks.D3D9Ex",
                                           new SK_IVarStub <bool> (&config.apis.d3d9ex.hook ) );
  SK_GetCommandProcessor ()->AddVariable ( "RenderHooks.D3D9",
                                           new SK_IVarStub <bool> (&config.apis.d3d9.hook ) );

  SK_GetCommandProcessor ()->AddVariable ( "RenderHooks.D3D11",
                                           new SK_IVarStub <bool> (&config.apis.dxgi.d3d11.hook ) );
#ifdef _WIN64
  SK_GetCommandProcessor ()->AddVariable ( "RenderHooks.D3D12",
                                           new SK_IVarStub <bool> (&config.apis.dxgi.d3d12.hook ) );
  SK_GetCommandProcessor ()->AddVariable ( "RenderHooks.Vulkan",
                                           new SK_IVarStub <bool> (&config.apis.Vulkan.hook ) );
#else
  SK_GetCommandProcessor ()->AddVariable ( "RenderHooks.D3D8",
                                           new SK_IVarStub <bool> (&config.apis.d3d8.hook ) );
  SK_GetCommandProcessor ()->AddVariable ( "RenderHooks.DDraw",
                                           new SK_IVarStub <bool> (&config.apis.ddraw.hook ) );
#endif

  SK_GetCommandProcessor ()->AddVariable ( "RenderHooks.OpenGL",
                                           new SK_IVarStub <bool> (&config.apis.OpenGL.hook ) );
}

#include <SpecialK/D3D9/texmgr.h>
#include <SpecialK/hooks.h>

#define D3D9_TEXTURE_MOD

void
SK_BootD3D9 (void)
{
  while (backend_dll == nullptr)
  {
    dll_log.Log (L"[API Detect]  *** Delaying VERY EARLY DLL Usage (d3d9.dll) -- tid=%x ***", GetCurrentThreadId ());
    SleepEx (500UL, TRUE);
  }

  static volatile ULONG __booted = FALSE;

  if (InterlockedCompareExchange (&__booted, TRUE, FALSE))
    return;

  void SK_D3D9_InitShaderModTools (void);
       SK_D3D9_InitShaderModTools ( );

  if (config.textures.d3d9_mod)
  {
    SK::D3D9::tex_mgr.Init ();
  }

  // Establish the minimal set of APIs necessary to work as d3d9.dll
  if (SK_GetDLLRole () == DLL_ROLE::D3D9)
    config.apis.d3d9.hook = true;

  if (! (config.apis.d3d9.hook || config.apis.d3d9ex.hook))
    return;

  //
  // SANITY CHECK: D3D9 must be enabled to hook D3D9Ex...
  //
  if (config.apis.d3d9ex.hook && (! config.apis.d3d9.hook))
    config.apis.d3d9.hook = true;

  dll_log.Log (L"[API Detect]  <!> [ Bootstrapping Direct3D 9 (d3d9.dll) ] <!>");

  if (SK_GetDLLRole () == DLL_ROLE::D3D9)
  {
    // Load user-defined DLLs (Early)
#ifdef _WIN64
    SK_LoadEarlyImports64 ();
#else
    SK_LoadEarlyImports32 ();
#endif
  }

  SK_HookD3D9 ();

  if (config.textures.d3d9_mod)
  {
    SK::D3D9::tex_mgr.Hook ();
  }
}

#ifndef _WIN64
void
SK_BootD3D8 (void)
{
  while (backend_dll == nullptr)
  {
    dll_log.Log (L"[API Detect]  *** Delaying VERY EARLY DLL Usage (d3d8.dll) -- tid=%x ***", GetCurrentThreadId ());
    SleepEx (500UL, TRUE);
  }

  static volatile ULONG __booted = FALSE;

  if (InterlockedCompareExchange (&__booted, TRUE, FALSE))
    return;

  // Establish the minimal set of APIs necessary to work as d3d8.dll
  if (SK_GetDLLRole () == DLL_ROLE::D3D8)
    config.apis.d3d8.hook = true;

  if (! config.apis.d3d8.hook)
    return;

  dll_log.Log (L"[API Detect]  <!> [ Bootstrapping Direct3D 8 (d3d8.dll) ] <!>");

  if (SK_GetDLLRole () == DLL_ROLE::D3D8)
  {
    // Load user-defined DLLs (Early)
    SK_LoadEarlyImports32 ();
  }

  SK_HookD3D8 ();
}

void
SK_BootDDraw (void)
{
  while (backend_dll == nullptr)
  {
    dll_log.Log (L"[API Detect]  *** Delaying VERY EARLY DLL Usage (ddraw.dll) -- tid=%x ***", GetCurrentThreadId ());
    SleepEx (500UL, TRUE);
  }

  static volatile ULONG __booted = FALSE;

  if (InterlockedCompareExchange (&__booted, TRUE, FALSE))
    return;

  // Establish the minimal set of APIs necessary to work as ddraw.dll
  if (SK_GetDLLRole () == DLL_ROLE::DDraw)
    config.apis.ddraw.hook = true;

  if (! config.apis.ddraw.hook)
    return;

  dll_log.Log (L"[API Detect]  <!> [ Bootstrapping DirectDraw (ddraw.dll) ] <!>");

  if (SK_GetDLLRole () == DLL_ROLE::DDraw)
  {
    // Load user-defined DLLs (Early)
    SK_LoadEarlyImports32 ();
  }

  SK_HookDDraw ();
}
#endif

void
SK_BootDXGI (void)
{
  while (backend_dll == nullptr)
  {
    dll_log.Log (L"[API Detect]  *** Delaying VERY EARLY DLL Usage (dxgi.dll) -- tid=%x ***", GetCurrentThreadId ());
    SleepEx (500UL, TRUE);
  }

  static volatile ULONG __booted = FALSE;

  if (InterlockedCompareExchange (&__booted, TRUE, FALSE))
    return;

  // Establish the minimal set of APIs necessary to work as dxgi.dll
  if (SK_GetDLLRole () == DLL_ROLE::DXGI)
    config.apis.dxgi.d3d11.hook = true;

#ifdef _WIN64
  if (! (config.apis.dxgi.d3d11.hook || config.apis.dxgi.d3d12.hook))
    return;

  //
  // TEMP HACK: D3D11 must be enabled to hook D3D12...
  //
  if (config.apis.dxgi.d3d12.hook && (! config.apis.dxgi.d3d11.hook))
    config.apis.dxgi.d3d11.hook = true;
#endif

  dll_log.Log (L"[API Detect]  <!> [    Bootstrapping DXGI (dxgi.dll)    ] <!>");

  if (SK_GetDLLRole () == DLL_ROLE::DXGI)
  {
    // Load user-defined DLLs (Early)
#ifdef _WIN64
    SK_LoadEarlyImports64 ();
#else
    SK_LoadEarlyImports32 ();
#endif
  }

  SK_HookDXGI ();
}


void
SK_BootOpenGL (void)
{
  while (backend_dll == nullptr)
  {
    dll_log.Log (L"[API Detect]  *** Delaying VERY EARLY DLL Usage (OpenGL32.dll) -- tid=%x ***", GetCurrentThreadId ());
    SleepEx (500UL, TRUE);
  }

#ifndef SK_BUILD__INSTALLER
  static volatile ULONG __booted = FALSE;

  if (InterlockedCompareExchange (&__booted, TRUE, FALSE))
    return;

  // Establish the minimal set of APIs necessary to work as OpenGL32.dll
  if (SK_GetDLLRole () == DLL_ROLE::OpenGL)
    config.apis.OpenGL.hook = true;

  if (! config.apis.OpenGL.hook)
    return;

  dll_log.Log (L"[API Detect]  <!> [ Bootstrapping OpenGL (OpenGL32.dll) ] <!>");

  if (SK_GetDLLRole ( ) == DLL_ROLE::OpenGL)
  {
    // Load user-defined DLLs (Early)
#ifdef _WIN64
    SK_LoadEarlyImports64 ();
#else
    SK_LoadEarlyImports32 ();
#endif
  }
#endif

  SK_HookGL ();
}


void
SK_BootVulkan (void)
{
#ifdef _WIN64
  static volatile ULONG __booted = FALSE;

  if (InterlockedCompareExchange (&__booted, TRUE, FALSE))
    return;

  // Establish the minimal set of APIs necessary to work as vulkan-1.dll
  if (SK_GetDLLRole () == DLL_ROLE::Vulkan)
    config.apis.Vulkan.hook = true;

  if (! config.apis.Vulkan.hook)
    return;

  dll_log.Log (L"[API Detect]  <!> [ Bootstrapping Vulkan 1.x (vulkan-1.dll) ] <!>");

  SK_HookVulkan ();
#endif
}


void
SK_RenderBackend_V2::gsync_s::update (void)
{
  if (! (config.apis.NvAPI.gsync_status && sk::NVAPI::nv_hardware))
    return;


  SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();


  if ( rb.device    == nullptr ||
       rb.swapchain == nullptr ||
       rb.surface   == nullptr )
  {
    last_checked = timeGetTime ();
    active       = false;
    return;
  }


  if ( last_checked < timeGetTime () - 500UL )
  {
    bool success = false;

    if (NVAPI_OK    == NvAPI_D3D_IsGSyncCapable (rb.device, rb.surface, &capable))
    {
      if ( NVAPI_OK == NvAPI_D3D_IsGSyncActive (rb.device, rb.surface, &active))
      {
        last_checked = timeGetTime ();
        success      = true;
      }

      else
      {
        // On failure, postpone the next check
        last_checked = timeGetTime () + 3000UL;
        active       = FALSE;
      }
    }

    else
      capable = FALSE;

    if (! success)
    {
      // On failure, postpone the next check
      last_checked = timeGetTime () + 3000UL;
      active       = FALSE;
    }

    else
      last_checked = timeGetTime ();

    // DO NOT hold onto this. NVAPI does not explain how NVDX handles work, but
    //   we can generally assume their lifetime is only as long as the D3D resource
    //     they identify.
    rb.surface = nullptr;
  }
}


bool
SK_RenderBackendUtil_IsFullscreen (void)
{
  SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  if (static_cast <int> (rb.api) & static_cast <int> (SK_RenderAPI::D3D11))
  {
    CComPtr <IDXGISwapChain> pSwapChain = nullptr;
    BOOL                     fullscreen = rb.fullscreen_exclusive;

    rb.swapchain->QueryInterface <IDXGISwapChain> (&pSwapChain);

    if (SUCCEEDED (pSwapChain->GetFullscreenState (&fullscreen, nullptr)))
      return fullscreen;

    return true;//rb.fullscreen_exclusive;
  }

  if (static_cast <int> (rb.api) & static_cast <int> (SK_RenderAPI::D3D9))
  {
    CComPtr <IDirect3DSwapChain9> pSwapChain = nullptr;

    rb.swapchain->QueryInterface <IDirect3DSwapChain9> (&pSwapChain);

    D3DPRESENT_PARAMETERS pparams;
    if (SUCCEEDED (pSwapChain->GetPresentParameters (&pparams)))
      return (! pparams.Windowed);

    return rb.fullscreen_exclusive;
  }

  return false;
}

void
SK_RenderBackend_V2::requestFullscreenMode (bool override)
{
  if (override)
  {
    config.display.force_windowed   = false;
    config.display.force_fullscreen = true;
  }

  fullscreen_exclusive =
    SK_RenderBackendUtil_IsFullscreen ();

  if (! fullscreen_exclusive)
  {
    if (static_cast <int> (api) & static_cast <int> (SK_RenderAPI::D3D9))
    {
      SK_D3D9_TriggerReset (true);
    }

    else if (static_cast <int> (api) & static_cast <int> (SK_RenderAPI::D3D11))
    {
      CComPtr <IDXGISwapChain> pSwapChain = nullptr;
      swapchain->QueryInterface <IDXGISwapChain> (&pSwapChain);

      pSwapChain->SetFullscreenState (TRUE, nullptr);
    }
  }

  if (static_cast <int> (api) & static_cast <int> (SK_RenderAPI::D3D11))
  {
    DXGI_SWAP_CHAIN_DESC swap_desc;

    CComPtr <IDXGISwapChain> pSwapChain = nullptr;
    swapchain->QueryInterface <IDXGISwapChain> (&pSwapChain);

    if (SUCCEEDED (pSwapChain->GetDesc (&swap_desc)))
    {
      if (SUCCEEDED (pSwapChain->ResizeTarget (&swap_desc.BufferDesc)))
      {
        SendMessage ( swap_desc.OutputWindow, WM_SIZE, SIZE_RESTORED,
                        MAKELPARAM ( swap_desc.BufferDesc.Width,
                                     swap_desc.BufferDesc.Height ) );
      }
    }
  }
}

void
SK_RenderBackend_V2::requestWindowedMode (bool override)
{
  if (override)
  {
    config.display.force_windowed   = true;
    config.display.force_fullscreen = false;
  }

  fullscreen_exclusive =
    SK_RenderBackendUtil_IsFullscreen ();

  if (fullscreen_exclusive)
  {
    if (static_cast <int> (api) & static_cast <int> (SK_RenderAPI::D3D9))
    {
      SK_D3D9_TriggerReset (true);
    }

    else if (static_cast <int> (api) & static_cast <int> (SK_RenderAPI::D3D11))
    {
      CComPtr <IDXGISwapChain> pSwapChain = nullptr;
      swapchain->QueryInterface <IDXGISwapChain> (&pSwapChain);

      pSwapChain->SetFullscreenState (FALSE, nullptr);
    }
  }
}


_Return_type_success_ (nullptr)
IUnknown*
SK_COM_ValidateRelease (IUnknown** ppObj)
{
  if ((! ppObj) || (! InterlockedCompareExchangePointer ((volatile LPVOID *)ppObj, nullptr, nullptr)))
    return nullptr;
  
  ULONG refs =
    (*ppObj)->Release ();
  
  assert (refs == 0);
  
  if (refs == 0)
  {
    InterlockedExchangePointer ((void **)ppObj, nullptr);
  }

  return *ppObj;
}

void
SK_RenderBackend_V2::releaseOwnedResources (void)
{
  SK_AutoCriticalSection auto_cs (&cs_res);

  if (device != nullptr && swapchain != nullptr && d3d11.immediate_ctx != nullptr)
  {
    ////device              = SK_COM_ValidateRelease (&device);
    ////swapchain           = SK_COM_ValidateRelease (&swapchain);
    ////d3d11.immediate_ctx = SK_COM_ValidateRelease (&d3d11.immediate_ctx);

    device              = nullptr;
    swapchain           = nullptr;
    d3d11.immediate_ctx = nullptr;
  }
}

SK_RenderBackend_V2::SK_RenderBackend_V2 (void)
{
  InitializeCriticalSectionAndSpinCount (&cs_res, 1024*1024);
}

SK_RenderBackend_V2::~SK_RenderBackend_V2 (void)
{
  releaseOwnedResources ();
  DeleteCriticalSection (&cs_res);
}


reset_stage_e         trigger_reset       (reset_stage_e::Clear);
mode_change_request_e request_mode_change (mode_change_request_e::None);



__declspec (dllexport)
IUnknown*
__stdcall
SK_Render_GetDevice (void)
{
  return SK_GetCurrentRenderBackend ().device;
}

__declspec (dllexport)
IUnknown*
__stdcall
SK_Render_GetSwapChain (void)
{
  return SK_GetCurrentRenderBackend ().swapchain;
}