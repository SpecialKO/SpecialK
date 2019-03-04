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

#define __SK_SUBSYSTEM__ L"RenderBase"

#include <SpecialK/render/backend.h>
#include <SpecialK/render/dxgi/dxgi_backend.h>
#include <SpecialK/render/d3d9/d3d9_backend.h>
#include <SpecialK/render/d3d9/d3d9_texmgr.h>
#include <SpecialK/render/d3d8/d3d8_backend.h>
#include <SpecialK/render/gl/opengl_backend.h>
#include <SpecialK/render/ddraw/ddraw_backend.h>

#include <SpecialK/control_panel.h>

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



volatile LONG SK_RenderBackend::frames_drawn = 0;


class SK_AutoDC
{
public:
   SK_AutoDC (HWND hWnd, HDC hDC) : hWnd_ (hWnd),
                                    hDC_  (hDC) { };
  ~SK_AutoDC (void)    { ReleaseDC (hWnd_, hDC_); }

  HDC  hDC  (void) { return hDC_;  }
  HWND hWnd (void) { return hWnd_; }

private:
  HWND hWnd_;
  HDC  hDC_;
};


float
SK_Display_GetDefaultRefreshRate (void)
{
  static float fRefresh      = 0.0f;
  static DWORD dwLastChecked = 0;

  if (dwLastChecked < timeGetTime () - 500UL)
  {
    SK_RenderBackend& rb =
      SK_GetCurrentRenderBackend ();

    fRefresh =
      static_cast <float> (rb.windows.device.getDevCaps ().res.refresh);

    dwLastChecked = timeGetTime ();
  }

  return fRefresh;
}


SK_RenderBackend&
__stdcall
SK_GetCurrentRenderBackend (void)
{
  static SK_RenderBackend __SK_RBkEnd;
  return                  __SK_RBkEnd;
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


//////////////////////////////////////////////////////////////////////////
//
//  Direct3D 9
//
//////////////////////////////////////////////////////////////////////////
#define D3D9_TEXTURE_MOD

#include <delayimp.h>

void
SK_BootD3D9 (void)
{
  // "Normal" games don't change render APIs mid-game; Talos does, but it's
  //   not normal :)
  if (SK_GetFramesDrawn ())
    return;

  SK_TLS *pTLS =
    SK_TLS_Bottom ();

  if (pTLS->d3d9.ctx_init_thread)
    return;

  ///while (backend_dll == nullptr)
  ///{
  ///  dll_log.Log (L"[API Detect]  *** Delaying VERY EARLY DLL Usage (d3d9.dll) -- tid=%x ***", GetCurrentThreadId ());
  ///  SK_Sleep    (100UL);
  ///}

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

  if (! InterlockedCompareExchangeAcquire (&__booted, TRUE, FALSE))
  {
    pTLS->d3d9.ctx_init_thread = true;

    SK_D3D9_InitShaderModTools  ();

    SK::D3D9::TextureManager& tex_mgr =
      SK_D3D9_GetTextureManager ();

    if (config.textures.d3d9_mod)
    {
      tex_mgr.Init ();
    }

    dll_log.Log (L"[API Detect]  <!> [ Bootstrapping Direct3D 9 (d3d9.dll) ] <!>");

    if (SK_GetDLLRole () == DLL_ROLE::D3D9)
    {
      // Load user-defined DLLs (Early)
      SK_RunLHIfBitness ( 64, SK_LoadEarlyImports64 (),
                              SK_LoadEarlyImports32 () );
    }

    if (config.textures.d3d9_mod)
    {
      tex_mgr.Hook ();
    }

    SK_HookD3D9    ();

    InterlockedIncrementRelease (&__booted);
  }

  SK_Thread_SpinUntilAtomicMin (&__booted, 2);
}


#ifndef _WIN64
//////////////////////////////////////////////////////////////////////////
//
//  Direct3D 8
//
//////////////////////////////////////////////////////////////////////////
void
SK_BootD3D8 (void)
{
  // "Normal" games don't change render APIs mid-game; Talos does, but it's
  //   not normal :)
  if (SK_GetFramesDrawn ())
    return;


  while (backend_dll == nullptr)
  {
    dll_log.Log (L"[API Detect]  *** Delaying VERY EARLY DLL Usage (d3d8.dll) -- tid=%x ***", GetCurrentThreadId ());
    SK_Sleep (100UL);
  }

  // Establish the minimal set of APIs necessary to work as d3d8.dll
  if (SK_GetDLLRole () == DLL_ROLE::D3D8)
    config.apis.d3d8.hook = true;

  if (! config.apis.d3d8.hook)
    return;

  static volatile LONG __booted = FALSE;

  if (! InterlockedCompareExchangeAcquire (&__booted, TRUE, FALSE))
  {
    dll_log.Log (L"[API Detect]  <!> [ Bootstrapping Direct3D 8 (d3d8.dll) ] <!>");

    if (SK_GetDLLRole () == DLL_ROLE::D3D8)
    {
      // Load user-defined DLLs (Early)
      SK_LoadEarlyImports32 ();
    }

    SK_HookD3D8 ();

    InterlockedIncrementRelease (&__booted);
  }

  SK_Thread_SpinUntilAtomicMin (&__booted, 2);
}


//////////////////////////////////////////////////////////////////////////
//
//  DirectDraw
//
//////////////////////////////////////////////////////////////////////////
void
SK_BootDDraw (void)
{
  // "Normal" games don't change render APIs mid-game; Talos does, but it's
  //   not normal :)
  if (SK_GetFramesDrawn ())
    return;


  while (backend_dll == nullptr)
  {
    dll_log.Log (L"[API Detect]  *** Delaying VERY EARLY DLL Usage (ddraw.dll) -- tid=%x ***", GetCurrentThreadId ());
    SK_Sleep    (100UL);
  }

  // Establish the minimal set of APIs necessary to work as ddraw.dll
  if (SK_GetDLLRole () == DLL_ROLE::DDraw)
    config.apis.ddraw.hook = true;

  if (! config.apis.ddraw.hook)
    return;


  static volatile LONG __booted = FALSE;

  if (! InterlockedCompareExchangeAcquire (&__booted, TRUE, FALSE))
  {
    dll_log.Log (L"[API Detect]  <!> [ Bootstrapping DirectDraw (ddraw.dll) ] <!>");

    if (SK_GetDLLRole () == DLL_ROLE::DDraw)
    {
      // Load user-defined DLLs (Early)
      SK_LoadEarlyImports32 ();
    }

    SK_HookDDraw ();

    InterlockedIncrementRelease (&__booted);
  }

  SK_Thread_SpinUntilAtomicMin (&__booted, 2);
}
#endif


//////////////////////////////////////////////////////////////////////////
//
//  DXGI (D3D10+)
//
//////////////////////////////////////////////////////////////////////////
void
SK_BootDXGI (void)
{
  //SK_TLS *pTLS =
  //  SK_TLS_Bottom ();
  //
  //if (pTLS->d3d11.ctx_init_thread)
  //  return;

  // "Normal" games don't change render APIs mid-game; Talos does, but it's
  //   not normal :)
  if (SK_GetFramesDrawn ())
    return;


  SK_DXGI_QuickHook ();

  ////while (backend_dll == nullptr)
  ////{
  ////  dll_log.Log (L"[API Detect]  *** Delaying VERY EARLY DLL Usage (dxgi.dll) -- tid=%x ***", GetCurrentThreadId ());
  ////  SK_Sleep    (100UL);
  ////}

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

  if (! InterlockedCompareExchangeAcquire (&__booted, TRUE, FALSE))
  {
    dll_log.Log (L"[API Detect]  <!> [    Bootstrapping DXGI (dxgi.dll)    ] <!>");

    if (SK_GetDLLRole () & DLL_ROLE::DXGI)
    {
      // Load user-defined DLLs (Early)
      SK_RunLHIfBitness ( 64, SK_LoadEarlyImports64 (),
                              SK_LoadEarlyImports32 () );
    }

    SK_HookDXGI ();

    InterlockedIncrementRelease (&__booted);
  }

  SK_Thread_SpinUntilAtomicMin (&__booted, 2);
}


//////////////////////////////////////////////////////////////////////////
//
//  OpenGL
//
//////////////////////////////////////////////////////////////////////////
void
SK_BootOpenGL (void)
{
  SK_TLS *pTLS =
    SK_TLS_Bottom ();

  if (pTLS->gl.ctx_init_thread)
    return;

  // "Normal" games don't change render APIs mid-game; Talos does, but it's
  //   not normal :)
  //if (SK_GetFramesDrawn ())
  //  return;


  //while (backend_dll == nullptr)
  //{
  //  dll_log.Log (L"[API Detect]  *** Delaying VERY EARLY DLL Usage (OpenGL32.dll) -- tid=%x ***", GetCurrentThreadId ());
  //  SK_Sleep    (100UL);
  //}

  // Establish the minimal set of APIs necessary to work as OpenGL32.dll
  if (SK_GetDLLRole () == DLL_ROLE::OpenGL)
    config.apis.OpenGL.hook = true;

  if (! config.apis.OpenGL.hook)
    return;

#ifndef SK_BUILD__INSTALLER
  static volatile LONG __booted = FALSE;

  if (! InterlockedCompareExchangeAcquire (&__booted, TRUE, FALSE))
  {
    pTLS->gl.ctx_init_thread = true;

    dll_log.Log (L"[API Detect]  <!> [ Bootstrapping OpenGL (OpenGL32.dll) ] <!>");

    if (SK_GetDLLRole () == DLL_ROLE::OpenGL)
    {
      // Load user-defined DLLs (Early)
      SK_RunLHIfBitness ( 64, SK_LoadEarlyImports64 (),
                              SK_LoadEarlyImports32 () );
    }

    SK_HookGL ();

    InterlockedIncrementRelease (&__booted);
  }
#endif

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


  if ( rb.device        == nullptr ||
       rb.swapchain     == nullptr ||
       rb.surface.nvapi == nullptr )
  {
    rb.surface.d3d9 = nullptr;
    rb.surface.dxgi = nullptr;

    last_checked = SK::ControlPanel::current_time;
    active       = false;

    return;
  }



  if ( last_checked < SK::ControlPanel::current_time - 500UL )
  {
    bool success = false;

    if (NVAPI_OK    == NvAPI_D3D_IsGSyncCapable (rb.device, rb.surface.nvapi, &capable))
    {
      if ( NVAPI_OK == NvAPI_D3D_IsGSyncActive  (rb.device, rb.surface.nvapi, &active))
      {
        last_checked = SK::ControlPanel::current_time;
        success      = true;
      }

      else
      {
        // On failure, postpone the next check
        last_checked = SK::ControlPanel::current_time + 3000UL;
        active       = FALSE;
      }
    }

    else
      capable = FALSE;

    if (! success)
    {
      // On failure, postpone the next check
      last_checked = SK::ControlPanel::current_time + 3000UL;
      active       = FALSE;
    }

    else
      last_checked = SK::ControlPanel::current_time;

    // DO NOT hold onto this. NVAPI does not explain how NVDX handles work, but
    //   we can generally assume their lifetime is only as long as the D3D resource
    //     they identify.
    rb.surface.nvapi = nullptr;
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

  return SK_Display_GetDefaultRefreshRate ();
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

__declspec (noinline)
void
SK_RenderBackend_V2::setHDRCapable (bool set)
{
  hdr_capable = set;
}

__declspec (noinline)
bool
SK_RenderBackend_V2::isHDRCapable (void)
{
  if (framebuffer_flags & SK_FRAMEBUFFER_FLAG_HDR)
    hdr_capable = true;

  if (driver_based_hdr)
    hdr_capable = true;

  return
    hdr_capable;
}

void
SK_RenderBackend_V2::releaseOwnedResources (void)
{
  SK_LOG1 ( ( L"Releasing Owned Resources" ),
            __SK_SUBSYSTEM__ );

  SK_AutoCriticalSection auto_cs (&cs_res);

  //if (device != nullptr && swapchain != nullptr && d3d11.immediate_ctx != nullptr)
  {
    ////d3d11.immediate_ctx = SK_COM_ValidateRelease (&d3d11.immediate_ctx);
    ////swapchain           = SK_COM_ValidateRelease (&swapchain);
    ////device              = SK_COM_ValidateRelease (&device);

    SK_LOG1 ( ( L"API: %x", api ),
              __SK_SUBSYSTEM__ );

    //// D3D11On12 (PHASING OUT)
    //d3d11.interop.backbuffer_rtv   = nullptr;
    //d3d11.interop.backbuffer_tex2D = nullptr;

    if (d3d11.immediate_ctx != nullptr)
    {
      ID3D11RenderTargetView* pRTVs [] = { nullptr };

      d3d11.immediate_ctx->OMSetRenderTargets   (1, pRTVs, nullptr);
      d3d11.immediate_ctx->PSSetShaderResources (0,     0, nullptr);
      d3d11.immediate_ctx->Flush                (                 );
    }

    d3d11.deferred_ctx  = nullptr;
    d3d11.immediate_ctx = nullptr;

    ////extern ID3D12DescriptorHeap* g_pd3dSrvDescHeap;
    ////
    ////if (g_pd3dSrvDescHeap != nullptr)
    ////{
    ////  extern void ImGui_ImplDX12_Shutdown (void);
    ////              ImGui_ImplDX12_Shutdown (    );
    ////
    ////  g_pd3dSrvDescHeap->Release ();
    ////  g_pd3dSrvDescHeap = nullptr;
    ////}

    if (api != SK_RenderAPI::D3D11On12)
    {
      swapchain         = nullptr;
      device            = nullptr;
    }
  }

  if (surface.d3d9 != nullptr)
  {
    surface.d3d9  = nullptr;
    surface.nvapi = nullptr;
  }

  if (surface.dxgi != nullptr)
  {
    surface.dxgi  = nullptr;
    surface.nvapi = nullptr;
  }
}

SK_RenderBackend_V2::SK_RenderBackend_V2 (void)
{
  InitializeCriticalSectionEx (&cs_res, 64*64, RTL_CRITICAL_SECTION_FLAG_DYNAMIC_SPIN | SK_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
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
  return SK_GetCurrentRenderBackend ().device.p;
}

// Does NOT implicitly AddRef, do NOT hold a reference to this!
__declspec (dllexport)
IUnknown*
__stdcall
SK_Render_GetSwapChain (void)
{
  return SK_GetCurrentRenderBackend ().swapchain.p;
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
  if (focus.hwnd == nullptr || ( GetFocus () == hWnd && hWnd != 0 && GetActiveWindow () == hWnd ) )
  {
    focus              = hWnd;
    game_window.hWnd   = hWnd;
    game_window.active = true;

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
  if (! IsWindow (device))
  {
    SK_LOG0 ( (L"Treating focus HWND as device HWND because device HWND was invalid."),
               L"Window Mgr");
    device = hWnd;
  }
}

void
SK_RenderBackend_V2::window_registry_s::setDevice (HWND hWnd)
{
  device = hWnd;

  SK_LOG1 ( (__FUNCTIONW__ L" (%X)", hWnd), L"  DEBUG!  " );
}


sk_hwnd_cache_s::devcaps_s&
sk_hwnd_cache_s::getDevCaps (void)
{
  DWORD dwNow = timeGetTime ();

  if (devcaps.last_checked < dwNow - 333UL)
  {
    HDC hDC = GetWindowDC (hwnd);

    devcaps.res.x       = GetDeviceCaps (hDC, HORZRES);
    devcaps.res.y       = GetDeviceCaps (hDC, VERTRES);
    devcaps.res.refresh = GetDeviceCaps (hDC, VREFRESH);

    ReleaseDC (hwnd, hDC);

    devcaps.last_checked = dwNow;
  }

  return devcaps;
}


const wchar_t*
SK_Render_GetAPIName (SK_RenderAPI api)
{
  std::unordered_map <SK_RenderAPI, const wchar_t *> api_map {
    { SK_RenderAPI::D3D11,  L"D3D11" },
    { SK_RenderAPI::D3D9,   L"D3D9"  }, { SK_RenderAPI::D3D9Ex,   L"D3D9Ex" },
    { SK_RenderAPI::OpenGL, L"OpenGL"}, { SK_RenderAPI::D3D8,     L"D3D8"   },
    { SK_RenderAPI::DDraw,  L"DDraw" }, { SK_RenderAPI::Reserved, L"N/A"    }
  };

  if (api_map.count (api) != 0)
    return api_map [api];

  SK_LOG0 ( ( L"Missing render API name mapping for %x", (unsigned int)api ),
              L"  FIXME!  " );

  return L"Unknown API";
}






sk_hwnd_cache_s::sk_hwnd_cache_s (HWND wnd)
{
  if (hwnd != wnd || last_changed == 0UL)
  {
    hwnd      = wnd;
    owner.tid =
      GetWindowThreadProcessId (hwnd, &owner.pid);

    RealGetWindowClassW        (hwnd, class_name, 127);
    InternalGetWindowText      (hwnd, title,      127);

    unicode = IsWindowUnicode  (hwnd);
    parent  = GetParent        (hwnd);

    last_changed = SK_GetFramesDrawn ();
  }
}

ULONG
__stdcall
SK_GetFramesDrawn_NonInline (void)
{
  return SK_GetFramesDrawn ();
}