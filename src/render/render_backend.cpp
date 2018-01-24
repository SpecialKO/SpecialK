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

#include <SpecialK/render/backend.h>
#include <SpecialK/render/dxgi/dxgi_backend.h>
#include <SpecialK/render/d3d9/d3d9_backend.h>
#include <SpecialK/render/d3d9/d3d9_texmgr.h>
#include <SpecialK/render/d3d8/d3d8_backend.h>
#include <SpecialK/render/gl/opengl_backend.h>
#include <SpecialK/render/ddraw/ddraw_backend.h>

#include <SpecialK/nvapi.h>
#include <SpecialK/utility.h>
#include <SpecialK/thread.h>
#include <SpecialK/config.h>
#include <SpecialK/core.h>
#include <SpecialK/command.h>
#include <SpecialK/framerate.h>
#include <SpecialK/log.h>
#include <SpecialK/hooks.h>
#include <SpecialK/import.h>

#include <atlbase.h>
#include <cassert>

SK_RenderBackend __SK_RBkEnd;

SK_RenderBackend&
__stdcall
SK_GetCurrentRenderBackend (void)
{
  return __SK_RBkEnd;
}

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

#define D3D9_TEXTURE_MOD

void
SK_BootD3D9 (void)
{
  SK_TLS_Bottom ()->d3d9.ctx_init_thread = true;

  // "Normal" games don't change render APIs mid-game; Talos does, but it's
  //   not normal :)
  if (SK_GetFramesDrawn ())
    return;

  while (backend_dll == nullptr)
  {
    dll_log.Log (L"[API Detect]  *** Delaying VERY EARLY DLL Usage (d3d9.dll) -- tid=%x ***", GetCurrentThreadId ());
    SleepEx (100UL, TRUE);
  }

  SK_D3D9_QuickHook ();

  // Establish the minimal set of APIs necessary to work as d3d9.dll
  if (SK_GetDLLRole () == DLL_ROLE::D3D9)
    config.apis.d3d9.hook = true;

  //
  // SANITY CHECK: D3D9 must be enabled to hook D3D9Ex...
  //
  if (config.apis.d3d9ex.hook && (! config.apis.d3d9.hook))
    config.apis.d3d9.hook = true;

  if (! (config.apis.d3d9.hook || config.apis.d3d9ex.hook))
    return;

  static volatile LONG __booted = FALSE;

  if (! InterlockedCompareExchange (&__booted, TRUE, FALSE))
  {
    SK_D3D9_InitShaderModTools ();

    if (config.textures.d3d9_mod)
    {
      SK::D3D9::tex_mgr.Init ();
    }

    dll_log.Log (L"[API Detect]  <!> [ Bootstrapping Direct3D 9 (d3d9.dll) ] <!>");

    if (SK_GetDLLRole () == DLL_ROLE::D3D9)
    {
      // Load user-defined DLLs (Early)
      SK_RunLHIfBitness ( 64, SK_LoadEarlyImports64 (),
                              SK_LoadEarlyImports32 () );
    }

  //SK_D3D9_PreHook ();
    SK_HookD3D9     ();

    if (config.textures.d3d9_mod)
    {
      SK::D3D9::tex_mgr.Hook ();
    }

    InterlockedIncrement (&__booted);
  }

  SK_Thread_SpinUntilAtomicMin (&__booted, 2);
}

#ifndef _WIN64
void
SK_BootD3D8 (void)
{
  while (backend_dll == nullptr)
  {
    dll_log.Log (L"[API Detect]  *** Delaying VERY EARLY DLL Usage (d3d8.dll) -- tid=%x ***", GetCurrentThreadId ());
    SleepEx (100UL, TRUE);
  }

  // Establish the minimal set of APIs necessary to work as d3d8.dll
  if (SK_GetDLLRole () == DLL_ROLE::D3D8)
    config.apis.d3d8.hook = true;

  if (! config.apis.d3d8.hook)
    return;

  static volatile LONG __booted = FALSE;

  if (! InterlockedCompareExchange (&__booted, TRUE, FALSE))
  {
    dll_log.Log (L"[API Detect]  <!> [ Bootstrapping Direct3D 8 (d3d8.dll) ] <!>");

    if (SK_GetDLLRole () == DLL_ROLE::D3D8)
    {
      // Load user-defined DLLs (Early)
      SK_LoadEarlyImports32 ();
    }

    SK_HookD3D8 ();

    InterlockedIncrement (&__booted);
  }

  SK_Thread_SpinUntilAtomicMin (&__booted, 2);
}

void
SK_BootDDraw (void)
{
  while (backend_dll == nullptr)
  {
    dll_log.Log (L"[API Detect]  *** Delaying VERY EARLY DLL Usage (ddraw.dll) -- tid=%x ***", GetCurrentThreadId ());
    SleepEx (100UL, TRUE);
  }

  // Establish the minimal set of APIs necessary to work as ddraw.dll
  if (SK_GetDLLRole () == DLL_ROLE::DDraw)
    config.apis.ddraw.hook = true;

  if (! config.apis.ddraw.hook)
    return;

  static volatile LONG __booted = FALSE;

  if (! InterlockedCompareExchange (&__booted, TRUE, FALSE))
  {
    dll_log.Log (L"[API Detect]  <!> [ Bootstrapping DirectDraw (ddraw.dll) ] <!>");

    if (SK_GetDLLRole () == DLL_ROLE::DDraw)
    {
      // Load user-defined DLLs (Early)
      SK_LoadEarlyImports32 ();
    }

    SK_HookDDraw ();

    InterlockedIncrement (&__booted);
  }

  SK_Thread_SpinUntilAtomicMin (&__booted, 2);
}
#endif

void
SK_BootDXGI (void)
{
  // "Normal" games don't change render APIs mid-game; Talos does, but it's
  //   not normal :)
  if (SK_GetFramesDrawn ())
    return;

  SK_DXGI_QuickHook ();

  while (backend_dll == nullptr)
  {
    dll_log.Log (L"[API Detect]  *** Delaying VERY EARLY DLL Usage (dxgi.dll) -- tid=%x ***", GetCurrentThreadId ());
    SleepEx (100UL, TRUE);
  }

  // Establish the minimal set of APIs necessary to work as dxgi.dll
  if (SK_GetDLLRole () == DLL_ROLE::DXGI)
    config.apis.dxgi.d3d11.hook = true;

#ifdef _WIN64
  //
  // TEMP HACK: D3D11 must be enabled to hook D3D12...
  //
  if (config.apis.dxgi.d3d12.hook && (! config.apis.dxgi.d3d11.hook))
    config.apis.dxgi.d3d11.hook = true;
#endif

  if (! config.apis.dxgi.d3d11.hook)
    return;

  static volatile LONG __booted = FALSE;

  if (! InterlockedCompareExchange (&__booted, TRUE, FALSE))
  {
    dll_log.Log (L"[API Detect]  <!> [    Bootstrapping DXGI (dxgi.dll)    ] <!>");

    if (SK_GetDLLRole () & DLL_ROLE::DXGI)
    {
      // Load user-defined DLLs (Early)
      SK_RunLHIfBitness ( 64, SK_LoadEarlyImports64 (),
                              SK_LoadEarlyImports32 () );
    }

    SK_HookDXGI ();

    InterlockedIncrement (&__booted);
  }

  SK_Thread_SpinUntilAtomicMin (&__booted, 2);
}

void
SK_BootOpenGL (void)
{
  // "Normal" games don't change render APIs mid-game; Talos does, but it's
  //   not normal :)
  if (SK_GetFramesDrawn ())
    return;

  while (backend_dll == nullptr)
  {
    dll_log.Log (L"[API Detect]  *** Delaying VERY EARLY DLL Usage (OpenGL32.dll) -- tid=%x ***", GetCurrentThreadId ());
    SleepEx (100UL, TRUE);
  }

  // Establish the minimal set of APIs necessary to work as OpenGL32.dll
  if (SK_GetDLLRole () == DLL_ROLE::OpenGL)
    config.apis.OpenGL.hook = true;

  if (! config.apis.OpenGL.hook)
    return;

#ifndef SK_BUILD__INSTALLER
  static volatile LONG __booted = FALSE;

  if (! InterlockedCompareExchange (&__booted, TRUE, FALSE))
  {
    dll_log.Log (L"[API Detect]  <!> [ Bootstrapping OpenGL (OpenGL32.dll) ] <!>");

    if (SK_GetDLLRole () == DLL_ROLE::OpenGL)
    {
      // Load user-defined DLLs (Early)
      SK_RunLHIfBitness ( 64, SK_LoadEarlyImports64 (),
                              SK_LoadEarlyImports32 () );
    }
#endif

    SK_HookGL ();

    InterlockedIncrement (&__booted);
  }

  SK_Thread_SpinUntilAtomicMin (&__booted, 2);
}


void
SK_BootVulkan (void)
{
//#ifdef _WIN64
//  static volatile ULONG __booted = FALSE;
//
//  if (InterlockedCompareExchange (&__booted, TRUE, FALSE))
//    return;
//
//  // Establish the minimal set of APIs necessary to work as vulkan-1.dll
//  if (SK_GetDLLRole () == DLL_ROLE::Vulkan)
//    config.apis.Vulkan.hook = true;
//
//  if (! config.apis.Vulkan.hook)
//    return;
//
//  dll_log.Log (L"[API Detect]  <!> [ Bootstrapping Vulkan 1.x (vulkan-1.dll) ] <!>");
//
//  SK_HookVulkan ();
//#endif
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
    CComQIPtr <IDXGISwapChain> pSwapChain (rb.swapchain);
    BOOL                       fullscreen = rb.fullscreen_exclusive;

    if (pSwapChain != nullptr)
    {
      if (SUCCEEDED (pSwapChain->GetFullscreenState (&fullscreen, nullptr)))
        return fullscreen;

      return true;//rb.fullscreen_exclusive;
    }
  }

  if (static_cast <int> (rb.api) & static_cast <int> (SK_RenderAPI::D3D9))
  {
    CComQIPtr <IDirect3DSwapChain9> pSwapChain (rb.swapchain);

    if (pSwapChain != nullptr)
    {
      D3DPRESENT_PARAMETERS pparams = { };

      if (SUCCEEDED (pSwapChain->GetPresentParameters (&pparams)))
        return (! pparams.Windowed);

      return rb.fullscreen_exclusive;
    }
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

    else if ((static_cast <int> (api) & static_cast <int> (SK_RenderAPI::D3D11)) && swapchain != nullptr)
    {
      CComQIPtr <IDXGISwapChain> pSwapChain (swapchain);

      if (pSwapChain != nullptr)
      {
        pSwapChain->SetFullscreenState (TRUE, nullptr);
      }
    }
  }

  if ((static_cast <int> (api) & static_cast <int> (SK_RenderAPI::D3D11)))
  {
    DXGI_SWAP_CHAIN_DESC swap_desc = { };

    CComQIPtr <IDXGISwapChain> pSwapChain (swapchain);

    if (pSwapChain != nullptr)
    {
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

    else if ((static_cast <int> (api) & static_cast <int> (SK_RenderAPI::D3D11)) && swapchain != nullptr)
    {
      CComQIPtr <IDXGISwapChain> pSwapChain (swapchain);

      if (pSwapChain != nullptr)
      {
        pSwapChain->SetFullscreenState (FALSE, nullptr);
      }
    }
  }
}


float
SK_RenderBackend_V2::getActiveRefreshRate (void)
{
  if (static_cast <int> (api) & static_cast <int> (SK_RenderAPI::D3D9))
  {
    CComQIPtr <IDirect3DDevice9> pDev9 (device);

    if (pDev9 != nullptr)
    {
      CComQIPtr <IDirect3DSwapChain9> pSwap9 (swapchain);

      if (pSwap9 != nullptr)
      {
        D3DPRESENT_PARAMETERS pparams = { };

        if (SUCCEEDED (pSwap9->GetPresentParameters (&pparams)))
        {
          if (pparams.FullScreen_RefreshRateInHz != 0)
          {
            return
              static_cast <float> (pparams.FullScreen_RefreshRateInHz);
          }
        }
      }
    }
  }

  else if (static_cast <int> (api) & static_cast <int> (SK_RenderAPI::D3D11))
  {
    CComQIPtr <IDXGISwapChain> pSwapDXGI (swapchain);

    if (pSwapDXGI != nullptr)
    {
      DXGI_SWAP_CHAIN_DESC swap_desc = { };
      pSwapDXGI->GetDesc (&swap_desc);

      if (swap_desc.BufferDesc.RefreshRate.Denominator != 0)
      {
        return static_cast <float> (swap_desc.BufferDesc.RefreshRate.Numerator) /
               static_cast <float> (swap_desc.BufferDesc.RefreshRate.Denominator);
      }
    }
  }

  class SK_AutoDC
  {
  public:
    SK_AutoDC (HWND hWnd, HDC hDC) : hWnd_ (hWnd),
                                     hDC_  (hDC) { };
    ~SK_AutoDC (void) { ReleaseDC (hWnd_, hDC_); }

    HWND hWnd_;
    HDC  hDC_;
  };

  SK_AutoDC auto_dc (NULL, GetDC (NULL));

  return
    static_cast <float> (GetDeviceCaps (auto_dc.hDC_, VREFRESH));
}


_Return_type_success_ (nullptr)
IUnknown*
SK_COM_ValidateRelease (IUnknown** ppObj)
{
  if ((! ppObj) || (! ReadPointerAcquire ((volatile LPVOID *)ppObj)))
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


LONG                  trigger_frame       = 0L;
reset_stage_e         trigger_reset       (reset_stage_e::Clear);
mode_change_request_e request_mode_change (mode_change_request_e::None);



// Does NOT implicitly AddRef, do NOT hold a reference to this!
__declspec (dllexport)
IUnknown*
__stdcall
SK_Render_GetDevice (void)
{
  return SK_GetCurrentRenderBackend ().device;
}

// Does NOT implicitly AddRef, do NOT hold a reference to this!
__declspec (dllexport)
IUnknown*
__stdcall
SK_Render_GetSwapChain (void)
{
  return SK_GetCurrentRenderBackend ().swapchain;
}




HWND
SK_RenderBackend_V2::window_registry_s::getDevice (void)
{
  SK_LOG4 ( (__FUNCTIONW__), L"  DEBUG!  " );

  return device;
}

HWND
SK_RenderBackend_V2::window_registry_s::getFocus (void)
{
  SK_LOG4 ( (__FUNCTIONW__), L"  DEBUG!  " );

  return focus;
}


#include <SpecialK/window.h>

void
SK_RenderBackend_V2::window_registry_s::setFocus (HWND hWnd)
{
  if (focus == nullptr || ( GetActiveWindow () == hWnd && GetFocus () == hWnd && hWnd != 0))
  {
    focus            = hWnd;
    game_window.hWnd = hWnd;

    SK_LOG1 (( __FUNCTIONW__ L" (%X)", hWnd ), L"  DEBUG!  ");
  }

  else
  {
    SK_LOG1 (( __FUNCTIONW__ L" (%X) --- FAIL [%s %s]", hWnd,
                GetFocus        () != hWnd ?
                  L"GetFocus () != this,"  : L"",
                GetActiveWindow () != hWnd     ?
                 L"GetActiveWindow () != this" : L""  ),
              L"  DEBUG!  ");
  }
}

void
SK_RenderBackend_V2::window_registry_s::setDevice (HWND hWnd)
{
  device = hWnd;

  SK_LOG1 ( (__FUNCTIONW__ L" (%X)", hWnd), L"  DEBUG!  " );
}