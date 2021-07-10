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

#include <SpecialK/stdafx.h>

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"RenderBase"

#include <SpecialK/render/dxgi/dxgi_backend.h>
#include <SpecialK/render/d3d9/d3d9_backend.h>
#include <SpecialK/render/d3d9/d3d9_texmgr.h>
#include <SpecialK/render/d3d8/d3d8_backend.h>
#include <SpecialK/render/gl/opengl_backend.h>
#include <SpecialK/render/ddraw/ddraw_backend.h>
#include <SpecialK/render/d3d12/d3d12_interfaces.h>
#include <SpecialK/render/d3d11/d3d11_core.h>
#include <SpecialK/render/dxgi/dxgi_hdr.h>


#include <SpecialK/nvapi.h>
#include <SpecialK/resource.h> // Unpack shader compiler

volatile ULONG64 SK_RenderBackend::frames_drawn = 0ULL;

extern void
SK_DXGI_UpdateColorSpace (IDXGISwapChain3* This, DXGI_OUTPUT_DESC1 *outDesc = nullptr);


class SK_AutoDC
{
public:
   SK_AutoDC (HWND hWnd, HDC hDC) : hWnd_ (hWnd),
                                    hDC_  (hDC) { };
  ~SK_AutoDC (void)    { ReleaseDC (hWnd_, hDC_); }

  HDC  hDC  (void) noexcept { return hDC_;  }
  HWND hWnd (void) noexcept { return hWnd_; }

private:
  HWND hWnd_;
  HDC  hDC_;
};


float
SK_Display_GetDefaultRefreshRate (HMONITOR hMonitor)
{
  static float    fRefresh      = 0.0f;
  static DWORD    dwLastChecked = 0;
  static HMONITOR hLastMonitor  = 0;

  if (hLastMonitor != hMonitor || dwLastChecked < SK_timeGetTime () - 250UL)
  {
    auto& rb =
      SK_GetCurrentRenderBackend ();

    // Refresh this bastard!
    rb.windows.device.getDevCaps ().last_checked = 0;

    fRefresh =
      static_cast <float> (rb.windows.device.getDevCaps ().res.refresh);

    dwLastChecked = SK_timeGetTime ();
    hLastMonitor  = hMonitor;
  }

  return fRefresh;
}


SK_LazyGlobal <
  SK_RenderBackend
> __SK_RBkEnd;

SK_RenderBackend&
__stdcall
SK_GetCurrentRenderBackend (void)
{
  return __SK_RBkEnd.get ();
}

void
__stdcall
SK_InitRenderBackends (void)
{ static
  auto *pCommandProcessor =
   SK_GetCommandProcessor ();

  pCommandProcessor->AddVariable ( "RenderHooks.D3D9Ex",
                                     new SK_IVarStub <bool> (&config.apis.d3d9ex.hook ) );
  pCommandProcessor->AddVariable ( "RenderHooks.D3D9",
                                     new SK_IVarStub <bool> (&config.apis.d3d9.hook ) );

  pCommandProcessor->AddVariable ( "RenderHooks.D3D11",
                                     new SK_IVarStub <bool> (&config.apis.dxgi.d3d11.hook ) );
#ifdef _M_AMD64
  pCommandProcessor->AddVariable ( "RenderHooks.D3D12",
                                     new SK_IVarStub <bool> (&config.apis.dxgi.d3d12.hook ) );
  pCommandProcessor->AddVariable ( "RenderHooks.Vulkan",
                                     new SK_IVarStub <bool> (&config.apis.Vulkan.hook ) );
#else /* _M_IX86 */
  pCommandProcessor->AddVariable ( "RenderHooks.D3D8",
                                     new SK_IVarStub <bool> (&config.apis.d3d8.hook ) );
  pCommandProcessor->AddVariable ( "RenderHooks.DDraw",
                                     new SK_IVarStub <bool> (&config.apis.ddraw.hook ) );
#endif

  pCommandProcessor->AddVariable ( "RenderHooks.OpenGL",
                                     new SK_IVarStub <bool> (&config.apis.OpenGL.hook ) );
}


//////////////////////////////////////////////////////////////////////////
//
//  Direct3D 9
//
//////////////////////////////////////////////////////////////////////////
#define D3D9_TEXTURE_MOD

void
SK_BootD3D9 (void)
{
  // Need to check for recursion thanks to Ansel
  static DWORD dwInitTid = GetCurrentThreadId ();


  // "Normal" games don't change render APIs mid-game; Talos does, but it's
  //   not normal :)
  if (SK_GetFramesDrawn () > 0)
    return;

  SK_TLS *pTLS =
    SK_TLS_Bottom ();

  if ( pTLS != nullptr &&
       pTLS->d3d9->ctx_init_thread )
  {
    return;
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

  if (InterlockedCompareExchangeAcquire (&__booted, TRUE, FALSE) == FALSE)
  {
    if (pTLS != nullptr)
        pTLS->d3d9->ctx_init_thread = true;

    SK_D3D9_InitShaderModTools  ();

    SK::D3D9::TextureManager& tex_mgr =
      SK_D3D9_GetTextureManager ();

    if (config.textures.d3d9_mod)
    {
      tex_mgr.Init ();
    }

    dll_log->Log (L"[API Detect]  <!> [ Bootstrapping Direct3D 9 (d3d9.dll) ] <!>");

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

  if (dwInitTid != GetCurrentThreadId ())
    SK_Thread_SpinUntilAtomicMin (&__booted, 2);
}


#ifndef _M_AMD64
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
  if (SK_GetFramesDrawn () > 0)
    return;

  // Establish the minimal set of APIs necessary to work as d3d8.dll
  if (SK_GetDLLRole () == DLL_ROLE::D3D8)
    config.apis.d3d8.hook = true;

  if (! config.apis.d3d8.hook)
    return;

  static volatile LONG __booted = FALSE;

  if (! InterlockedCompareExchangeAcquire (&__booted, TRUE, FALSE))
  {
    dll_log->Log (L"[API Detect]  <!> [ Bootstrapping Direct3D 8 (d3d8.dll) ] <!>");

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
  if (SK_GetFramesDrawn () > 0)
    return;


  while (backend_dll == nullptr)
  {
    dll_log->Log (L"[API Detect]  *** Delaying VERY EARLY DLL Usage (ddraw.dll) -- tid=%x ***", SK_Thread_GetCurrentId ());
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
    dll_log->Log (L"[API Detect]  <!> [ Bootstrapping DirectDraw (ddraw.dll) ] <!>");

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
  // Need to check for recursion thanks to Ansel
  static DWORD dwInitTid = GetCurrentThreadId ();

  // "Normal" games don't change render APIs mid-game; Talos does, but it's
  //   not normal :)
  if (SK_GetFramesDrawn () > 0)
    return;

  SK_TLS *pTLS =
    SK_TLS_Bottom ();

  if ( pTLS != nullptr &&
       pTLS->d3d11->ctx_init_thread )
  {
    return;
  }

#ifdef _M_AMD64
  //
  // TEMP HACK: D3D11 must be enabled to hook D3D12...
  //
  if (config.apis.dxgi.d3d12.hook && (! config.apis.dxgi.d3d11.hook))
    config.apis.dxgi.d3d11.hook = true;
#endif

  if (! ( config.apis.dxgi.d3d11.hook ||
          config.apis.dxgi.d3d12.hook ))
    return;

  static volatile LONG __booted = FALSE;

  if (InterlockedCompareExchangeAcquire (&__booted, TRUE, FALSE) == FALSE)
  {
    SK_DXGI_QuickHook ();

    // Establish the minimal set of APIs necessary to work as dxgi.dll
    if (SK_GetDLLRole () == DLL_ROLE::DXGI)
    {
      if (! config.apis.dxgi.d3d12.hook)
            config.apis.dxgi.d3d11.hook = true;
    }

    if (pTLS)
        pTLS->d3d11->ctx_init_thread = true;

    dll_log->Log (L"[API Detect]  <!> [    Bootstrapping DXGI (dxgi.dll)    ] <!>");

    if (SK_GetDLLRole () & DLL_ROLE::DXGI)
    {
      // Load user-defined DLLs (Early)
      SK_RunLHIfBitness ( 64, SK_LoadEarlyImports64 (),
                              SK_LoadEarlyImports32 () );
    }

    SK_HookDXGI ();

    InterlockedIncrementRelease (&__booted);
  }

  if (dwInitTid != GetCurrentThreadId ())
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

  if ( pTLS != nullptr &&
       pTLS->gl->ctx_init_thread )
  {
    return;
  }

  // Establish the minimal set of APIs necessary to work as OpenGL32.dll
  if (SK_GetDLLRole () == DLL_ROLE::OpenGL)
    config.apis.OpenGL.hook = true;

  if (! config.apis.OpenGL.hook)
    return;

#ifndef SK_BUILD__INSTALLER
  static volatile LONG __booted = FALSE;

  if (InterlockedCompareExchangeAcquire (&__booted, TRUE, FALSE) == FALSE)
  {
    if (pTLS)
        pTLS->gl->ctx_init_thread = true;

    dll_log->Log (L"[API Detect]  <!> [ Bootstrapping OpenGL (OpenGL32.dll) ] <!>");

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
//#ifdef _M_AMD64
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
SK_RenderBackend_V2::gsync_s::update (bool force)
{
  auto& rb =
    SK_GetCurrentRenderBackend ();

  // DO NOT hold onto this. NVAPI does not explain how NVDX handles work, but
  //   we can generally assume their lifetime is only as long as the D3D resource
  //     they identify.
  auto _ClearTemporarySurfaces = [&]()->
  void
  {
    rb.surface.dxgi  = nullptr;
    rb.surface.d3d9  = nullptr;
    rb.surface.nvapi = nullptr;
  };

  if (! ((force || config.apis.NvAPI.gsync_status) &&
                           sk::NVAPI::nv_hardware) )
  {
    capable = false;

    return
      _ClearTemporarySurfaces ();
  }

  DWORD dwTimeNow =
    SK_timeGetTime ();

  if ( last_checked < (dwTimeNow - 666UL) )
  {    last_checked =  dwTimeNow;

    bool success = false;

    if ( rb.device       == nullptr ||
         rb.swapchain    == nullptr ||
        (rb.surface.d3d9 == nullptr &&
         rb.surface.dxgi == nullptr) )
    {
      return
        _ClearTemporarySurfaces ();
    }

    if ( NVAPI_OK == NvAPI_D3D_IsGSyncCapable ( rb.device,
                                                rb.surface.nvapi, &capable))
    {
      NvAPI_D3D_IsGSyncActive ( rb.device,      rb.surface.nvapi, &active);

      success      = true;
    }

    else capable   = FALSE;

    if (!  success)
    {
      // On failure, postpone the next check
      last_checked = dwTimeNow + 3000UL;
      active       = FALSE;
    }

    else if (active)
      last_checked = dwTimeNow + 150UL;

    else
      last_checked = dwTimeNow + 500UL;
  }

  _ClearTemporarySurfaces ();
}


bool
SK_RenderBackendUtil_IsFullscreen (void)
{
  auto& rb =
    SK_GetCurrentRenderBackend ();

  SK_ComQIPtr <IDXGISwapChain>
                   pSwapChain (rb.swapchain);

  if (pSwapChain.p != nullptr)
  {
    BOOL fullscreen =
      rb.fullscreen_exclusive;

    if (pSwapChain != nullptr)
    {
      if ( SUCCEEDED (
        pSwapChain->GetFullscreenState (
          &fullscreen, nullptr         )
                     )
         )   rb.fullscreen_exclusive =
                fullscreen;

      return rb.fullscreen_exclusive;
    }
  }

  else
  if ( (static_cast <int> (rb.api) &
        static_cast <int> (SK_RenderAPI::D3D9)) != 0 )
  {
    SK_ComQIPtr <IDirect3DSwapChain9>
                         pSwapChain9 (rb.swapchain);

    if (pSwapChain9.p != nullptr)
    {
      D3DPRESENT_PARAMETERS pparams = { };

      if (SUCCEEDED (pSwapChain9->GetPresentParameters (&pparams)))
      {
        rb.fullscreen_exclusive =
          (! pparams.Windowed);
      }

      return rb.fullscreen_exclusive;
    }
  }

  return false;
}

// Refresh the cached refresh value?! :)
bool refresh_refresh = false;

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
  }

  SK_ComQIPtr <IDXGISwapChain>
                   pSwapChain (swapchain);

  //if ((static_cast <int> (api) & static_cast <int> (SK_RenderAPI::D3D11)))
  {
    if    (! swapchain) refresh_refresh = true;
    else if (swapchain != nullptr)
    {
      if (pSwapChain != nullptr)
      {
        DXGI_MODE_DESC  idealModeDesc = { };
        DXGI_MODE_DESC actualModeDesc = { };
        DXGI_SWAP_CHAIN_DESC swapDesc = { };

        pSwapChain->GetDesc (&swapDesc);

        SK_ComPtr          <IDXGIOutput>  pOutput;
        pSwapChain->GetContainingOutput (&pOutput);

        if (config.render.framerate.refresh_rate != -1.0f)
        {
          if (config.render.framerate.rescan_.Denom != 1)
          {
            swapDesc.BufferDesc.RefreshRate.Numerator   =
              config.render.framerate.rescan_.Numerator;
            swapDesc.BufferDesc.RefreshRate.Denominator =
              config.render.framerate.rescan_.Denom;
          }

          else
          {
            swapDesc.BufferDesc.RefreshRate.Numerator   =
              (UINT)config.render.framerate.refresh_rate;
            swapDesc.BufferDesc.RefreshRate.Denominator =
              1;
          }
        }

        idealModeDesc.Format      = swapDesc.BufferDesc.Format;
        idealModeDesc.Height      = swapDesc.BufferDesc.Height;
        idealModeDesc.Width       = swapDesc.BufferDesc.Width;
        idealModeDesc.RefreshRate = swapDesc.BufferDesc.RefreshRate;
        idealModeDesc.Scaling     = DXGI_MODE_SCALING_UNSPECIFIED;

        SK_ComPtr <IDXGIDevice> pSwapDevice = nullptr;

        if ( SUCCEEDED (
               pSwapChain->GetDevice ( __uuidof (IDXGIDevice),
                                       (void **)&pSwapDevice )
                       )
           )
        {
          if ( SUCCEEDED
               (
                 SK_DXGI_FindClosestMode
                 (
                   pSwapChain, &idealModeDesc,
                              &actualModeDesc,
                   pSwapDevice, TRUE
                 )
               )
             )
          {
            if ( FAILED (
                   SK_DXGI_ResizeTarget (pSwapChain, &actualModeDesc, TRUE)
                        )
               )
            {
              SK_LOG0 ((L"Failed to Establish Fullscreen Mode for Swapchain"), L"   DXGI   ");
            }
          }
        }

        if ( SUCCEEDED (
               pSwapChain->SetFullscreenState (TRUE, pOutput.p)
                       )
           )
        {
          SK_DXGI_ResizeTarget (
            pSwapChain, &actualModeDesc, TRUE
          );
        }
      }
    }
  }

  if ( (! fullscreen_exclusive) && pSwapChain.p != nullptr )
  {
    DXGI_SWAP_CHAIN_DESC swap_desc = { };

    if ( SUCCEEDED (pSwapChain->GetDesc  (&swap_desc))   &&
         SUCCEEDED (SK_DXGI_ResizeTarget (pSwapChain,
                                          &swap_desc.BufferDesc, TRUE)) )
    {
      PostMessage ( swap_desc.OutputWindow, WM_SIZE, SIZE_RESTORED,
       MAKELPARAM ( swap_desc.BufferDesc.Width,
                    swap_desc.BufferDesc.Height ) );
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
    if ( static_cast <int> (api) &
         static_cast <int> (SK_RenderAPI::D3D9) )
    {
      SK_D3D9_TriggerReset (true);
    }

    else
    {
      SK_ComQIPtr <IDXGISwapChain>
                       pSwapChain (swapchain);

      if (pSwapChain != nullptr) {
          pSwapChain->SetFullscreenState (
            FALSE, nullptr
          );
      }
    }
  }
}


float
SK_RenderBackend_V2::getActiveRefreshRate (HMONITOR hMonitor)
{
  // This isn't implemented for arbitrary monitors at the moment
  SK_ReleaseAssert (hMonitor == 0);
  //if (hMonitor == 0)
  //{
  //  hMonitor =
  //    MonitorFromWindow ( windows.getDevice (),
  //                          MONITOR_DEFAULTTONEAREST );
  //}

#if 0
  if ( static_cast <int> (api) &
       static_cast <int> (SK_RenderAPI::D3D9) )
  {
    SK_ComQIPtr <IDirect3DDevice9> pDev9 (device);

    if (pDev9 != nullptr)
    {
      SK_ComQIPtr <IDirect3DSwapChain9> pSwap9 (swapchain);

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

  // Handle D3D11/12
  else
  {
    SK_ComQIPtr <IDXGISwapChain>
                     pSwapChain (swapchain);

    constexpr
      UINT64 MAX_CACHE_TTL = 6; // 6 Frames, to prevent the assertion from triggering

    static
      UINT64 uiCachedFrame =
        SK_GetFramesDrawn ();

    if (pSwapChain.p != nullptr)
    {
      static float    fLastCache      = 0.0f;
      static UINT64  uiLastCacheFrame = 0;// uiCachedFrame - MAX_CACHE_TTL;
      static HMONITOR hLastMonitor    = 0;

      if (hLastMonitor != hMonitor)
           refresh_refresh = true;

      if ( refresh_refresh ||
           uiLastCacheFrame < (SK_GetFramesDrawn () - MAX_CACHE_TTL) )
      {
        SK_ComPtr <IDXGIDevice>           pSwapDevice = nullptr;
        SK_ComPtr <IDXGIOutput>           pSwapOutput = nullptr;
        pSwapChain->GetContainingOutput (&pSwapOutput);

        if (pSwapOutput != nullptr)
        {
          DXGI_OUTPUT_DESC           outDesc = { };
          if (pSwapOutput != nullptr)
              pSwapOutput->GetDesc (&outDesc);

          SK_ReleaseAssert (outDesc.Monitor == hMonitor);

          hLastMonitor = outDesc.Monitor;
        }

        DXGI_SWAP_CHAIN_DESC  swap_desc = { };
        pSwapChain->GetDesc (&swap_desc);

        if (swap_desc.BufferDesc.RefreshRate.Denominator != 0)
        {
          uiLastCacheFrame = SK_GetFramesDrawn ();
           fLastCache      =
             ( static_cast <float> (swap_desc.BufferDesc.RefreshRate.Numerator  ) /
               static_cast <float> (swap_desc.BufferDesc.RefreshRate.Denominator) );
        }

        if (fLastCache == 0.0)
            fLastCache = SK_Display_GetDefaultRefreshRate (hMonitor);

        if (fLastCache != 0.0)
        {
          uiLastCacheFrame = SK_GetFramesDrawn ();
          refresh_refresh  = false;
        }
      }

      if (! refresh_refresh)
      {
        return
          fLastCache;
      }
    }
  }
#endif

  refresh_refresh = false;

  return
    SK_Display_GetDefaultRefreshRate (hMonitor);
}


_Return_type_success_ (nullptr)
IUnknown*
SK_COM_ValidateRelease (IUnknown** ppObj)
{
  if ((! ppObj) || (! ReadPointerAcquire ((volatile LPVOID *)ppObj)))
    return nullptr;

  const ULONG refs =
    (*ppObj)->Release ();

  assert (refs == 0);

  if (refs == 0)
  {
    InterlockedExchangePointer ((void **)ppObj, nullptr);
  }

  return *ppObj;
}

HANDLE
SK_RenderBackend_V2::getSwapWaitHandle (void)
{
  SK_ComQIPtr <IDXGISwapChain2>
      pSwap2       (swapchain.p);
  if (pSwap2.p != nullptr)
  {
    if (config.render.framerate.pre_render_limit > 0)
    {
      /////if (
      /////  FAILED ( pSwap2.p->SetMaximumFrameLatency (
      /////             config.render.framerate.pre_render_limit
      /////                                            )
      /////         )
      /////   )
      /////{
      /////  SK_LOG0 ( ( L"Failed to SetMaximumFrameLatency: %i",
      /////                config.render.framerate.pre_render_limit ),
      /////              L"   DXGI   " );
      /////
      /////  config.render.framerate.pre_render_limit = -1;
      /////
      /////  return 0;
      /////}

      DXGI_SWAP_CHAIN_DESC swap_desc = { };
      pSwap2->GetDesc    (&swap_desc);

      HANDLE hWait = 0;

      if (swap_desc.Flags & DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT)
        hWait = pSwap2.p->GetFrameLatencyWaitableObject ();

      if ((intptr_t)hWait > 0)
      {
        return hWait;
      }
    }
  }

  return 0;// INVALID_HANDLE_VALUE;
  // ^^^ So many places check for > 0 but do not cast to signed,
  //       thus do not return INVALID_HANDLE_VALUE; return 0.
}

void SK_HDR_ReleaseResources       (void);
void SK_DXGI_ReleaseSRGBLinearizer (void);

void
SK_RenderBackend_V2::releaseOwnedResources (void)
{
  SK_LOG1 ( ( L"Releasing Owned Resources" ),
            __SK_SUBSYSTEM__ );

  std::scoped_lock <SK_Thread_HybridSpinlock>
         auto_lock (res_lock);

  auto orig_se =
  SK_SEH_ApplyTranslator (
    SK_FilteringStructuredExceptionTranslator (
      EXCEPTION_ACCESS_VIOLATION
    )
  );
  try
  {
    SK_LOG1 ( ( L"API: %x", api ),
               __SK_SUBSYSTEM__ );

///#define _USE_FLUSH

    // Flushing at shutdown may cause deadlocks
#ifdef _USE_FLUSH
    if (d3d11.immediate_ctx != nullptr) {
        d3d11.immediate_ctx->Flush      ();
        d3d11.immediate_ctx->ClearState ();
    }
#endif
    swapchain = nullptr;//.Reset();
    device    = nullptr;//.Reset();
    factory   = nullptr;

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

    d3d11.immediate_ctx = nullptr;
    d3d12.command_queue = nullptr;

    SK_HDR_ReleaseResources       ();
    SK_DXGI_ReleaseSRGBLinearizer ();
  }

  catch (const SK_SEH_IgnoredException &)
  {
    SK_LOG0 ( (
      L"Access Violation Exception during SK_RenderBackend_V2::"
      L"releaseOwnedResources (...)"
              ), __SK_SUBSYSTEM__ );
  }

  SK_SEH_RemoveTranslator (orig_se);
}

SK_RenderBackend_V2::SK_RenderBackend_V2 (void)
{
}

SK_RenderBackend_V2::~SK_RenderBackend_V2 (void)
{
  releaseOwnedResources ();
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
  return
    SK_GetCurrentRenderBackend ().device;
}

// Does NOT implicitly AddRef, do NOT hold a reference to this!
__declspec (dllexport)
IUnknown*
__stdcall
SK_Render_GetSwapChain (void)
{
  return
    SK_GetCurrentRenderBackend ().swapchain;
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

void
SK_RenderBackend_V2::window_registry_s::setFocus (HWND hWnd)
{
  if (focus.hwnd == nullptr || ( SK_GetFocus () == hWnd && hWnd != nullptr && GetActiveWindow () == hWnd ) )
  {
    focus              = hWnd;
    game_window.hWnd   = hWnd;
    game_window.active = true;

    SK_LOG1 (( __FUNCTIONW__ L" (%X)", hWnd ), L"  DEBUG!  ");
  }

  else
  {
    SK_LOG1 (( __FUNCTIONW__ L" (%X) --- FAIL [%s %s]", hWnd,
             SK_GetFocus        () != hWnd ?
                  L"GetFocus () != this,"  : L"",
                GetActiveWindow () != hWnd     ?
                 L"GetActiveWindow () != this" : L""  ),
              L"  DEBUG!  ");
  }

  ////if (game_window.hWnd == 0 || (! IsWindow (game_window.hWnd)))
  ////{   game_window.hWnd = hWnd; }

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
  ////if (game_window.hWnd == 0 || (! IsWindow (game_window.hWnd)))
  ////{   game_window.hWnd = hWnd; }

  device = hWnd;

  SK_LOG1 ( (__FUNCTIONW__ L" (%X)", hWnd), L"  DEBUG!  " );
}

SK_RenderBackend_V2::scan_out_s::SK_HDR_TRANSFER_FUNC
SK_RenderBackend_V2::scan_out_s::getEOTF (void)
{
  auto& rb =
    SK_GetCurrentRenderBackend ();

  if (nvapi_hdr.isHDR10 ())
  {
    if (rb.framebuffer_flags & (uint64_t)SK_FRAMEBUFFER_FLAG_FLOAT)
      return Linear; // Seems a 16-bit swapchain is always scRGB...
    else
      return SMPTE_2084;
  }

  switch (dwm_colorspace)//dxgi_colorspace)
  {
    case DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709:
      return Linear;
    case DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709:
      return sRGB;
    case DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P2020:
    case DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P2020:
      return G22;
    case DXGI_COLOR_SPACE_RGB_STUDIO_G2084_NONE_P2020:
    case DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020:
    {
      if (rb.framebuffer_flags & (uint64_t)SK_FRAMEBUFFER_FLAG_FLOAT)
        return Linear; // Seems a 16-bit swapchain is always scRGB...
      else
        return SMPTE_2084;
    } break;
    // Pretty much only used by film
    case DXGI_COLOR_SPACE_RGB_STUDIO_G24_NONE_P709:
    case DXGI_COLOR_SPACE_RGB_STUDIO_G24_NONE_P2020:
      return G24;
    default:
      return NONE;
  }
}


sk_hwnd_cache_s::devcaps_s&
sk_hwnd_cache_s::getDevCaps (void)
{
  const DWORD dwNow = SK_timeGetTime ();

  if (devcaps.last_checked < dwNow - 250UL)
  {
    UINT32 uiNumPaths =  128;
    UINT32 uiNumModes = 1024;

    RECT                  rcWindow = { };
    GetWindowRect (hwnd, &rcWindow);

    if ( ERROR_SUCCESS ==
           GetDisplayConfigBufferSizes ( QDC_ONLY_ACTIVE_PATHS, &uiNumPaths,         &uiNumModes )
                                                              && uiNumPaths <= 32 &&  uiNumModes <= 32 )
    {
      SK_TLS *pTLS =
        SK_TLS_Bottom ();

      auto *pathArray = (DISPLAYCONFIG_PATH_INFO *)pTLS->scratch_memory->ccd.display_paths.alloc (uiNumPaths);
      auto *modeArray = (DISPLAYCONFIG_MODE_INFO *)pTLS->scratch_memory->ccd.display_modes.alloc (uiNumModes);

      if ( ERROR_SUCCESS == QueryDisplayConfig ( QDC_ONLY_ACTIVE_PATHS, &uiNumPaths, pathArray,
                                                                        &uiNumModes, modeArray, nullptr ) )
      {
        float bestIntersectArea = -1.0f;

        int ax1 = rcWindow.left,
            ax2 = rcWindow.right;
        int ay1 = rcWindow.top,
            ay2 = rcWindow.bottom;

        DISPLAYCONFIG_PATH_INFO *pOutput = nullptr;

        for (auto idx = 0U; idx < uiNumPaths; ++idx)
        {
          auto *path =
            &pathArray [idx];

          if (path->flags & DISPLAYCONFIG_PATH_ACTIVE)
          {
            DISPLAYCONFIG_SOURCE_MODE *pSourceMode =
              &modeArray [path->sourceInfo.modeInfoIdx].sourceMode;

            RECT rect;
            rect.left   = pSourceMode->position.x;
            rect.top    = pSourceMode->position.y;
            rect.right  = pSourceMode->position.x + pSourceMode->width;
            rect.bottom = pSourceMode->position.y + pSourceMode->height;

            if (! IsRectEmpty (&rect))
            {
              int bx1 = rect.left;
              int by1 = rect.top;
              int bx2 = rect.right;
              int by2 = rect.bottom;

              int intersectArea =
                ComputeIntersectionArea (ax1, ay1, ax2, ay2, bx1, by1, bx2, by2);

              if (intersectArea > bestIntersectArea)
              {
                pOutput           = path;
                bestIntersectArea =
                  static_cast <float> (intersectArea);
              }
            }
          }
        }

        if (pOutput != nullptr)
        {
          devcaps.res.refresh = (float)(
            (double)pOutput->targetInfo.refreshRate.Numerator /
            (double)pOutput->targetInfo.refreshRate.Denominator);

          devcaps.res.x = modeArray [pOutput->sourceInfo.modeInfoIdx].sourceMode.width;
          devcaps.res.y = modeArray [pOutput->sourceInfo.modeInfoIdx].sourceMode.height;

          devcaps.last_checked = dwNow;

          return devcaps;
        }
      }
    }

    HDC hDC =
      GetWindowDC (hwnd);

    if (hDC != 0)
    {
      devcaps.res.x       =         GetDeviceCaps (hDC,  HORZRES);
      devcaps.res.y       =         GetDeviceCaps (hDC,  VERTRES);
      devcaps.res.refresh = (float) GetDeviceCaps (hDC, VREFRESH);

      ReleaseDC (hwnd, hDC);
    }

    else
      devcaps.res = { 0 };

    devcaps.last_checked = dwNow;
  }

  return devcaps;
}


const wchar_t*
SK_Render_GetAPIName (SK_RenderAPI api)
{
  static const
    std::unordered_map <SK_RenderAPI, const wchar_t *>
      api_map {
        { SK_RenderAPI::D3D11,  L"D3D11" }, { SK_RenderAPI::D3D12,    L"D3D12"  },
        { SK_RenderAPI::D3D9,   L"D3D9"  }, { SK_RenderAPI::D3D9Ex,   L"D3D9Ex" },
        { SK_RenderAPI::OpenGL, L"OpenGL"}, { SK_RenderAPI::D3D8,     L"D3D8"   },
        { SK_RenderAPI::DDraw,  L"DDraw" }, { SK_RenderAPI::Reserved, L"N/A"    }
      };

  if (api_map.count (api) != 0)
    return api_map.at (api);

  SK_LOG0 ( ( L"Missing render API name mapping for %x", (unsigned int)api ),
              L"  FIXME!  " );

  return
    L"Unknown API";
}


void
SK_RenderBackend_V2::updateActiveAPI (SK_RenderAPI _api)
{
  if (std::exchange (update_outputs, false))
  {
    updateOutputTopology ();
  }

  static SK_RenderAPI LastKnownAPI =
         SK_RenderAPI::Reserved;

  if (_api != SK_RenderAPI::Reserved)
  {
    LastKnownAPI = _api;
                 return;
  }

  assert ( ReadULongAcquire (&thread) == (LONG)SK_Thread_GetCurrentId () ||
           LastKnownAPI               ==       SK_RenderAPI::Reserved );

  if ((device != nullptr || api == SK_RenderAPI::D3D12) && LastKnownAPI != api)
  {
    WriteULongRelease (&thread, SK_Thread_GetCurrentId ());

    SK_LOG0 ( ( L"SwapChain Presentation Thread has Priority=%i",
                GetThreadPriority (SK_GetCurrentThread ()) ),
                L"RenderBack" );

    SK_ComPtr <IDirect3DDevice9>   pDev9   = nullptr;
    SK_ComPtr <IDirect3DDevice9Ex> pDev9Ex = nullptr;
    SK_ComPtr <ID3D11Device>       pDev11  = nullptr;
    SK_ComPtr <ID3D12Device>       pDev12  = nullptr;

    if (device != nullptr)
    {
      if (SUCCEEDED (device->QueryInterface <IDirect3DDevice9Ex> (&pDev9Ex)))
      {
        reinterpret_cast <int &> (api) =
          ( static_cast <int> (SK_RenderAPI::D3D9  ) |
            static_cast <int> (SK_RenderAPI::D3D9Ex)  );

        wcsncpy (name, L"D3D9Ex", 8);
      }

      else if (SUCCEEDED (device->QueryInterface <IDirect3DDevice9> (&pDev9)))
      {
                 api  = SK_RenderAPI::D3D9;
        wcsncpy (name, L"D3D9  ", 8);
      }

      else if (SUCCEEDED (device->QueryInterface <ID3D12Device> (&pDev12)))
      {
        api  = SK_RenderAPI::D3D12;
        wcsncpy (name, L"D3D12 ", 8);
      }

      else if (SUCCEEDED (device->QueryInterface <ID3D11Device> (&pDev11)))
      {
        // Establish the API used this frame (and handle possible translation layers)
        //
        switch (SK_GetDLLRole ())
        {
          case DLL_ROLE::D3D8:
            api = SK_RenderAPI::D3D8On11;
            break;
          case DLL_ROLE::DDraw:
            api = SK_RenderAPI::DDrawOn11;
            break;
          default:
            api = SK_RenderAPI::D3D11;
            break;
        }

        SK_ComPtr <IUnknown> pTest = nullptr;

        if (       SUCCEEDED (device->QueryInterface (IID_ID3D11Device5, (void **)&pTest))) {
          wcsncpy (name, L"D3D11.4", 8); // Creators Update
        } else if (SUCCEEDED (device->QueryInterface (IID_ID3D11Device4, (void **)&pTest))) {
          wcsncpy (name, L"D3D11.4", 8);
        } else if (SUCCEEDED (device->QueryInterface (IID_ID3D11Device3, (void **)&pTest))) {
          wcsncpy (name, L"D3D11.3", 8);
        } else if (SUCCEEDED (device->QueryInterface (IID_ID3D11Device2, (void **)&pTest))) {
          wcsncpy (name, L"D3D11.2", 8);
        } else if (SUCCEEDED (device->QueryInterface (IID_ID3D11Device1, (void **)&pTest))) {
          wcsncpy (name, L"D3D11.1", 8);
        } else {
          wcsncpy (name, L"D3D11 ", 8);
        }

        if (     SK_GetDLLRole () == DLL_ROLE::D3D8)  {
          wcscpy (name, L"D3D8");
        }
        else if (SK_GetDLLRole () == DLL_ROLE::DDraw) {
          wcscpy (name, L"DDraw");
        }
      }

      else
      {
        api = SK_RenderAPI::Reserved;
        wcsncpy (name, L"UNKNOWN", 8);
      }
    }
  }

  else if (LastKnownAPI != api)
  {
    if (config.apis.OpenGL.hook)// && SK_GL_GetCurrentContext () != nullptr)
    {
      api = SK_RenderAPI::OpenGL;
      wcsncpy (name, L"OpenGL", 8);
    }
  }
}





sk_hwnd_cache_s::sk_hwnd_cache_s (HWND wnd)
{
  if (hwnd != wnd || last_changed == 0UL)
  {
    hwnd      = wnd;
    owner.tid =
      GetWindowThreadProcessId (hwnd, &owner.pid);

    if ( 0 ==
           RealGetWindowClassW   (hwnd, class_name, 128) )
                                       *class_name = L'\0';
    if ( 0 ==
           InternalGetWindowText (hwnd, title,      128) )
                                       *title = L'\0';

    unicode = IsWindowUnicode  (hwnd);
    parent  = GetParent        (hwnd);

    if (*title != L'\0' && *class_name != L'\0')
      last_changed = SK_GetFramesDrawn ();
  }
}

ULONG64
__stdcall
SK_GetFramesDrawn_NonInline (void)
{
  return SK_GetFramesDrawn ();
}


const wchar_t*
HDRModeToStr (NV_HDR_MODE mode)
{
  switch (mode)
  {
    case NV_HDR_MODE_OFF:              return L"Off";
    case NV_HDR_MODE_UHDA:             return L"HDR10";
    case NV_HDR_MODE_EDR:              return L"Extended Dynamic Range";
    case NV_HDR_MODE_SDR:              return L"Standard Dynamic Range";
    case NV_HDR_MODE_DOLBY_VISION:     return L"Dolby Vision";
    case NV_HDR_MODE_UHDA_PASSTHROUGH: return L"HDR10 Pass through";
    case NV_HDR_MODE_UHDA_NB:          return L"Notebook HDR";
    default:                           return L"Invalid";
  };
};


using D3DStripShader_pfn =
HRESULT (WINAPI *)
(   LPCVOID   pShaderBytecode,
    SIZE_T    BytecodeLength,
    UINT      uStripFlags,
  ID3DBlob  **ppStrippedBlob );

D3DStripShader_pfn
D3DStripShader_40_Original;
D3DStripShader_pfn
D3DStripShader_41_Original;
D3DStripShader_pfn
D3DStripShader_42_Original;
D3DStripShader_pfn
D3DStripShader_43_Original;
D3DStripShader_pfn
D3DStripShader_47_Original;

#undef  __SK_SUBSYSTEM__
#define __SK_SUBSYSTEM__ L"Shadr Comp"

HRESULT
WINAPI
D3DStripShader_40_Detour      (
  LPCVOID    pShaderBytecode,
  SIZE_T     BytecodeLength,
  UINT       uStripFlags,
  ID3DBlob** ppStrippedBlob   )
{
  UNREFERENCED_PARAMETER (pShaderBytecode);
  UNREFERENCED_PARAMETER (BytecodeLength);
  UNREFERENCED_PARAMETER (uStripFlags);
  UNREFERENCED_PARAMETER (ppStrippedBlob);

  SK_LOG_FIRST_CALL

  return S_OK;
}

HRESULT
WINAPI
D3DStripShader_41_Detour      (
  LPCVOID    pShaderBytecode,
  SIZE_T     BytecodeLength,
  UINT       uStripFlags,
  ID3DBlob** ppStrippedBlob   )
{
  UNREFERENCED_PARAMETER (pShaderBytecode);
  UNREFERENCED_PARAMETER (BytecodeLength);
  UNREFERENCED_PARAMETER (uStripFlags);
  UNREFERENCED_PARAMETER (ppStrippedBlob);

  SK_LOG_FIRST_CALL

  return S_OK;
}

HRESULT
WINAPI
D3DStripShader_42_Detour      (
  LPCVOID    pShaderBytecode,
  SIZE_T     BytecodeLength,
  UINT       uStripFlags,
  ID3DBlob** ppStrippedBlob   )
{
  UNREFERENCED_PARAMETER (pShaderBytecode);
  UNREFERENCED_PARAMETER (BytecodeLength);
  UNREFERENCED_PARAMETER (uStripFlags);
  UNREFERENCED_PARAMETER (ppStrippedBlob);

  SK_LOG_FIRST_CALL

  return S_OK;
}

HRESULT
WINAPI
D3DStripShader_43_Detour      (
  LPCVOID    pShaderBytecode,
  SIZE_T     BytecodeLength,
  UINT       uStripFlags,
  ID3DBlob** ppStrippedBlob   )
{
  UNREFERENCED_PARAMETER (pShaderBytecode);
  UNREFERENCED_PARAMETER (BytecodeLength);
  UNREFERENCED_PARAMETER (uStripFlags);
  UNREFERENCED_PARAMETER (ppStrippedBlob);

  SK_LOG_FIRST_CALL

  return S_OK;
}

HRESULT
WINAPI
D3DStripShader_47_Detour     (
  LPCVOID    pShaderBytecode,
  SIZE_T     BytecodeLength,
  UINT       uStripFlags,
  ID3DBlob** ppStrippedBlob  )
{
  UNREFERENCED_PARAMETER (pShaderBytecode);
  UNREFERENCED_PARAMETER (BytecodeLength);
  UNREFERENCED_PARAMETER (uStripFlags);
  UNREFERENCED_PARAMETER (ppStrippedBlob);

  SK_LOG_FIRST_CALL

  return S_OK;
}


void
SK_D3D_UnpackShaderCompiler (void)
{
  HMODULE hModSelf =
    SK_GetDLL ();

  HRSRC res =
    FindResource (
      hModSelf,
        MAKEINTRESOURCE (IDR_D3DCOMPILER_PACKAGE),
          L"7ZIP"
    );

  if (res)
  {
    SK_LOG0 ( ( L"Unpacking D3DCompiler_47.dll because user does not have "
                L"June 2010 DirectX Redistributables `ed." ),
                L"D3DCompile" );

    DWORD   res_size     =
      SizeofResource ( hModSelf, res );

    HGLOBAL packed_compiler =
      LoadResource   ( hModSelf, res );

    if (! packed_compiler) return;


    const void* const locked =
      (void *)LockResource (packed_compiler);


    if (locked != nullptr)
    {
      wchar_t      wszArchive     [MAX_PATH + 2] = { };
      wchar_t      wszDestination [MAX_PATH + 2] = { };

      wcsncpy_s ( wszDestination,    MAX_PATH,
                  SK_GetHostPath (), _TRUNCATE );

      if (GetFileAttributesW (wszDestination) == INVALID_FILE_ATTRIBUTES)
        SK_CreateDirectories (wszDestination);

      wcscpy      (wszArchive, wszDestination);
      PathAppendW (wszArchive, L"D3DCompiler_47.7z");

      ///SK_LOG0 ( ( L" >> Archive: %s [Destination: %s]", wszArchive,wszDestination ),
      ///            L"D3DCompile" );

      FILE* fPackedCompiler =
        _wfopen   (wszArchive, L"wb");

      if (fPackedCompiler != nullptr)
      {
        fwrite (locked, 1, res_size, fPackedCompiler);
        fclose (fPackedCompiler);
      }

      if (GetFileAttributes (wszArchive) != INVALID_FILE_ATTRIBUTES)
      {
        SK_Decompress7zEx (wszArchive, wszDestination, nullptr);
        DeleteFileW       (wszArchive);
      }
    }

    UnlockResource (packed_compiler);
  }
};

void
SK_D3D_SetupShaderCompiler (void)
{
  bool local_install = false;
  if (! SK_COMPAT_IsSystemDllInstalled (L"D3DCompiler_43.dll", &local_install))
  {
    SK_ImGui_Warning (L"Your system is missing the June 2010 DirectX Runtime.\t\t\n"
                      L"\n"
                      L"\t\t\t\t* Please install it as soon as possible.");
  }

  local_install = false;
  if (! SK_COMPAT_IsSystemDllInstalled (L"D3DCompiler_47.dll", &local_install))
  {
    if (! local_install)
    {
      SK_D3D_UnpackShaderCompiler ();
    }
  }

#if 0
  struct SK_D3D_AntiStrip {
    const wchar_t*    wszDll;
    const    char*    szSymbol;
           LPVOID    pfnHookFunc;
              void** ppfnTrampoline;
  } static strippers [] =
  { { L"D3DCOMPILER_47.dll",
       "D3DStripShader",   D3DStripShader_47_Detour,
  static_cast_p2p <void> (&D3DStripShader_47_Original) },
    { L"D3DCOMPILER_43.dll",
       "D3DStripShader",   D3DStripShader_43_Detour,
  static_cast_p2p <void> (&D3DStripShader_43_Original) },
    { L"D3DCOMPILER_42.dll",
       "D3DStripShader",   D3DStripShader_42_Detour,
  static_cast_p2p <void> (&D3DStripShader_42_Original) },
    { L"D3DCOMPILER_41.dll",
       "D3DStripShader",   D3DStripShader_41_Detour,
  static_cast_p2p <void> (&D3DStripShader_41_Original) },
    { L"D3DCOMPILER_40.dll",
       "D3DStripShader",   D3DStripShader_40_Detour,
  static_cast_p2p <void> (&D3DStripShader_40_Original) } };
#endif

  if (SUCCEEDED (__HrLoadAllImportsForDll ("D3DCOMPILER_47.dll")))
  {
// Causes problems with CroEngine games, we don't need unstripped
//   shaders anyway.
#ifdef _USE_ANTISTRIP
    for ( auto& stripper : strippers )
    {
      if (SK_GetModuleHandleW (stripper.wszDll) != nullptr)
      {
        SK_CreateDLLHook2 ( stripper.wszDll,
                            stripper.szSymbol,
                            stripper.pfnHookFunc,
                            stripper.ppfnTrampoline );
      }
    }
#endif
  }
}

#ifndef DPI_ENUMS_DECLARED
typedef
  enum { PROCESS_DPI_UNAWARE           = 0,
         PROCESS_SYSTEM_DPI_AWARE      = 1,
         PROCESS_PER_MONITOR_DPI_AWARE = 2
       } PROCESS_DPI_AWARENESS;

typedef
  enum { MDT_EFFECTIVE_DPI = 0,
         MDT_ANGULAR_DPI   = 1,
         MDT_RAW_DPI       = 2,
         MDT_DEFAULT       = MDT_EFFECTIVE_DPI
       } MONITOR_DPI_TYPE;
#endif

#ifndef _DPI_AWARENESS_CONTEXTS_
DECLARE_HANDLE (DPI_AWARENESS_CONTEXT);
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE    (DPI_AWARENESS_CONTEXT)-3
#endif
#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 (DPI_AWARENESS_CONTEXT)-4
#endif

// Shcore.lib+dll, Windows 8.1
typedef HRESULT               (WINAPI* SetProcessDpiAwareness_pfn)      (PROCESS_DPI_AWARENESS);
typedef HRESULT               (WINAPI* GetProcessDpiAwareness_pfn)      (HANDLE, PROCESS_DPI_AWARENESS*);
// Shcore.lib+dll, Windows 8.1
typedef HRESULT               (WINAPI* GetDpiForMonitor_pfn)            (HMONITOR, MONITOR_DPI_TYPE, UINT*, UINT*);
// User32.lib + dll, Windows 10 v1607 (Creators Update)
typedef DPI_AWARENESS_CONTEXT (WINAPI* SetThreadDpiAwarenessContext_pfn)(DPI_AWARENESS_CONTEXT);

using GetThreadDpiAwarenessContext_pfn  =
                         DPI_AWARENESS_CONTEXT (WINAPI *)(void);

using GetAwarenessFromDpiAwarenessContext_pfn =
                         DPI_AWARENESS (WINAPI *)(DPI_AWARENESS_CONTEXT);

skWin32Module user32_dll;
skWin32Module shcore_dll;

DPI_AWARENESS_CONTEXT
WINAPI
SK_Display_GetThreadDpiAwarenessContext (void)
{
  assert (SK_IsWindows10OrGreater ());

  if (user32_dll == skWin32Module::Uninitialized)
      user32_dll  =
        SK_Modules->LoadLibrary (L"user32.dll");

  static auto       GetThreadDpiAwarenessContextFn = user32_dll.
    GetProcAddress <GetThreadDpiAwarenessContext_pfn>
                  ("GetThreadDpiAwarenessContext");

  if (GetThreadDpiAwarenessContextFn != nullptr)
    return GetThreadDpiAwarenessContextFn ();

  return nullptr;
}

DPI_AWARENESS
WINAPI
SK_Display_GetThreadDpiAwareness (void)
{
  DPI_AWARENESS_CONTEXT dpi_ctx =
    SK_Display_GetThreadDpiAwarenessContext ();

  if (dpi_ctx != nullptr)
  {
    assert (SK_IsWindows10OrGreater ());

    if (user32_dll == skWin32Module::Uninitialized)
        user32_dll  =
          SK_Modules->LoadLibrary (L"user32.dll");

    static auto       GetAwarenessFromDpiAwarenessContextFn = user32_dll.
      GetProcAddress <GetAwarenessFromDpiAwarenessContext_pfn>
                    ("GetAwarenessFromDpiAwarenessContext");

    if (GetAwarenessFromDpiAwarenessContextFn != nullptr)
      return GetAwarenessFromDpiAwarenessContextFn (dpi_ctx);
  }

  return DPI_AWARENESS_INVALID;
}

DPI_AWARENESS_CONTEXT
WINAPI
SK_Display_SetThreadDpiAwarenessContext (DPI_AWARENESS_CONTEXT dpi_ctx)
{
  assert (SK_IsWindows10OrGreater ());

  if (user32_dll == skWin32Module::Uninitialized)
      user32_dll  =
        SK_Modules->LoadLibrary (L"user32.dll");

  static auto       SetThreadDpiAwarenessContextFn = user32_dll.
    GetProcAddress <SetThreadDpiAwarenessContext_pfn>
                  ("SetThreadDpiAwarenessContext");

  if (SetThreadDpiAwarenessContextFn != nullptr)
    return SetThreadDpiAwarenessContextFn (dpi_ctx);

  return nullptr;
}

extern BOOL SK_IsWindows8Point1OrGreater (void);
extern BOOL SK_IsWindows10OrGreater      (void);


BOOL
SK_Display_IsProcessDPIAware (void)
{
  if (SK_IsWindows10OrGreater ())
  {
    auto awareness =
      SK_Display_GetThreadDpiAwareness ();

    return
      ( awareness != DPI_AWARENESS_INVALID &&
        awareness != DPI_AWARENESS_UNAWARE );
  }

  if (SK_IsWindows8Point1OrGreater ())
  {
    if (shcore_dll == skWin32Module::Uninitialized)
        shcore_dll  =
          SK_Modules->LoadLibrary (L"shcore.dll");

    static auto       GetProcessDpiAwarenessFn = shcore_dll.
      GetProcAddress <GetProcessDpiAwareness_pfn>
                    ("GetProcessDpiAwareness");

    if (GetProcessDpiAwarenessFn != nullptr)
    {
      PROCESS_DPI_AWARENESS awareness =
        PROCESS_DPI_UNAWARE;

      HRESULT hr =
        GetProcessDpiAwarenessFn (
          SK_GetCurrentProcess (),
            &awareness );

      if (SUCCEEDED (hr))
        return awareness;
    }
  }

  return
    IsProcessDPIAware ();
}

BOOL
SK_Display_IsThreadDPIAware (void)
{
  if (SK_IsWindows10OrGreater ())
  {
    auto awareness =
      SK_Display_GetThreadDpiAwareness ();

    return
      ( awareness != DPI_AWARENESS_INVALID &&
        awareness != DPI_AWARENESS_UNAWARE );
  }

  return
    SK_Display_IsProcessDPIAware ();
}

void
SK_Display_PushDPIScaling (void)
{
  DPI_AWARENESS_CONTEXT dpi_ctx =
    SK_Display_GetThreadDpiAwarenessContext ();

  if (dpi_ctx == nullptr)
    return;

  SK_TLS_Bottom ()->scratch_memory->dpi.dpi_ctx_stack.push (dpi_ctx);
}

void
SK_Display_PopDPIScaling (void)
{
  auto &dpi_stack =
    SK_TLS_Bottom ()->scratch_memory->dpi.dpi_ctx_stack;

  if (! dpi_stack.empty ())
  {
    SK_Display_SetThreadDpiAwarenessContext (
      dpi_stack.top ()
    );

    dpi_stack.pop ();
  }
}

void SK_Display_ForceDPIAwarenessUsingAppCompat (bool set)
{
  DWORD   dwProcessSize = MAX_PATH;
  wchar_t wszProcessName [MAX_PATH + 2] = { };

  HANDLE hProc =
   SK_GetCurrentProcess ();

  QueryFullProcessImageName (
    hProc, 0,
      wszProcessName, &dwProcessSize
  );

  const wchar_t* wszKey        =
    LR"(Software\Microsoft\Windows NT\CurrentVersion\AppCompatFlags\Layers)";
  DWORD          dwDisposition = 0x00;
  HKEY           hKey          = nullptr;

  const LSTATUS status =
    RegCreateKeyExW ( HKEY_CURRENT_USER,
                        wszKey,      0,
                          nullptr, 0x0,
                          KEY_READ | KEY_WRITE,
                             nullptr,
                               &hKey,
                                 &dwDisposition );

  if ( status == ERROR_SUCCESS &&
       hKey   != nullptr          )
  {
    wchar_t             wszOrigKeyVal [2048] = { };
    DWORD len = sizeof (wszOrigKeyVal) / 2;

    RegGetValueW (
      hKey, nullptr, wszProcessName, RRF_RT_REG_SZ,
            nullptr,  wszOrigKeyVal, &len
    );

    wchar_t *pwszHIGHDPIAWARE =
      StrStrIW (wszOrigKeyVal,     L"HIGHDPIAWARE");
    wchar_t *pwszNextToken    =  pwszHIGHDPIAWARE + 13;

    if ((! set) && pwszHIGHDPIAWARE != nullptr)
    {
      *pwszHIGHDPIAWARE  = L'\0';

      std::wstring combined  = wszOrigKeyVal;
                   combined += L" ";
                   combined += pwszNextToken;

      wcsncpy_s ( wszOrigKeyVal,           len,
                  combined.c_str (), _TRUNCATE );

      StrTrimW (wszOrigKeyVal, L" ");

      RegSetValueExW (
        hKey, wszProcessName,
          0, REG_SZ,
            (BYTE *)wszOrigKeyVal,
               ( wcslen (wszOrigKeyVal) + 1 ) * sizeof (wchar_t)
      );
    }

    else if (set && pwszHIGHDPIAWARE == nullptr)
    {
      StrCatW  (wszOrigKeyVal, L" HIGHDPIAWARE");
      StrTrimW (wszOrigKeyVal, L" ");

      RegSetValueExW (
        hKey, wszProcessName,
          0, REG_SZ,
            (BYTE *)wszOrigKeyVal,
              ( wcslen (wszOrigKeyVal) + 1 ) * sizeof (wchar_t)
      );
    }

    RegFlushKey (hKey);
    RegCloseKey (hKey);
  }
}

void
SK_Display_SetMonitorDPIAwareness (bool bOnlyIfWin10)
{
  if (SK_IsWindows10OrGreater ())
  {
    SK_Display_SetThreadDpiAwarenessContext (
      DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
    );

    return;
  }

  if (bOnlyIfWin10)
    return;

  if (SK_IsWindows8Point1OrGreater ())
  {
    if (shcore_dll == skWin32Module::Uninitialized)
        shcore_dll  =
          SK_Modules->LoadLibrary (L"shcore.dll");

    static auto       SetProcessDpiAwarenessFn = shcore_dll.
      GetProcAddress <SetProcessDpiAwareness_pfn>
                    ("SetProcessDpiAwareness");

    if (SetProcessDpiAwarenessFn != nullptr)
    {
      SetProcessDpiAwarenessFn (
        PROCESS_PER_MONITOR_DPI_AWARE
      );

      return;
    }
  }

  SetProcessDPIAware ();
}

void
SK_Display_ClearDPIAwareness (bool bOnlyIfWin10)
{
  if (SK_IsWindows10OrGreater ())
  {
    SK_Display_SetThreadDpiAwarenessContext (
      DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
    );

    return;
  }

  if (bOnlyIfWin10)
    return;

  if (SK_IsWindows8Point1OrGreater ())
  {
    if (shcore_dll == skWin32Module::Uninitialized)
        shcore_dll  =
          SK_Modules->LoadLibrary (L"shcore.dll");

    static auto       SetProcessDpiAwarenessFn = shcore_dll.
      GetProcAddress <SetProcessDpiAwareness_pfn>
                    ("SetProcessDpiAwareness");

    if (SetProcessDpiAwarenessFn != nullptr)
    {
      SetProcessDpiAwarenessFn (
        PROCESS_DPI_UNAWARE
      );

      return;
    }
  }
}

void
SK_Display_DisableDPIScaling (void)
{
  // Persistently disable DPI scaling problems so that initialization order doesn't matter
  void SK_Display_ForceDPIAwarenessUsingAppCompat (bool set);
       SK_Display_ForceDPIAwarenessUsingAppCompat (true);

  if (! IsProcessDPIAware ())
  {
    void SK_Display_SetMonitorDPIAwareness (bool bOnlyIfWin10);
         SK_Display_SetMonitorDPIAwareness (false);

    // Only do this for Steam games, the Microsoft Store Yakuza games
    //   are chronically DPI unaware and broken
    if (StrStrIW (SK_GetHostPath (), L"SteamApps"))
      SK_RestartGame ();
  }


  if (SK_IsWindows10OrGreater ())
  {
    SK_Display_SetThreadDpiAwarenessContext (
      DPI_AWARENESS_CONTEXT_UNAWARE
    );

    return;
  }

  if (SK_IsWindows8Point1OrGreater ())
  {
    if (shcore_dll == skWin32Module::Uninitialized)
        shcore_dll  =
          SK_Modules->LoadLibrary (L"shcore.dll");

    static auto       SetProcessDpiAwarenessFn = shcore_dll.
      GetProcAddress <SetProcessDpiAwareness_pfn>
                    ("SetProcessDpiAwareness");

    if (SetProcessDpiAwarenessFn != nullptr)
    {
      SetProcessDpiAwarenessFn (
        PROCESS_DPI_UNAWARE
      );
    }
  }
}

bool
SK_RenderBackend_V2::checkHDRState (void)
{
  if (swapchain.p == nullptr)
    return false;

  SK_ComQIPtr <IDXGISwapChain3> pSwap3 (swapchain.p);

  if (pSwap3.p != nullptr)
  {
    SK_DXGI_UpdateColorSpace (pSwap3.p);

    return
      isHDRCapable () && isHDRActive ();
  }

  return false;
}


HRESULT
SK_RenderBackend_V2::setDevice (IUnknown *pDevice)
{
  if (device.p == pDevice)
    return S_OK;

  else if (pDevice != nullptr)
  {
    bool already_set =
      device.p != nullptr;

    //SK_ComQIPtr <ID3D10Device> pDevice10 (pDevice);
    //if (pDevice10.p != nullptr)
    //{
    ////d3d10.device = pDevice10;
    //        device = pDevice10;
    //}
    //

    if (! already_set)
    {
      if (config.apis.dxgi.d3d11.hook)
      {
        SK_ComQIPtr <ID3D11Device> pDevice11 (pDevice);
        if (pDevice11.p != nullptr)
        {
        //d3d11.device = pDevice11;
                device = pDevice;//pDevice11;
                api    = SK_RenderAPI::D3D11;
        }
      }

      if (config.apis.dxgi.d3d12.hook)
      {
        SK_ComQIPtr <ID3D12Device> pDevice12 (pDevice);
        if (pDevice12.p != nullptr)
        {
        //d3d12.device = pDevice12;
                device = pDevice;//pDevice12;
                api    = SK_RenderAPI::D3D12;
        }
      }
    }

    if (already_set)
      return DXGI_ERROR_DEVICE_RESET;

    return S_OK;
  }

  else
  {
    device = nullptr;
    return DXGI_ERROR_DEVICE_REMOVED;
  }
}

const uint8_t
  edid_v1_header [] =
    { 0x00, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0x00 };

const uint8_t
  edid_v1_descriptor_flag [] =
    { 0x00, 0x00 };

const wchar_t*
DXGIColorSpaceToStr (DXGI_COLOR_SPACE_TYPE space) noexcept;

#define EDID_LENGTH                        0x80
#define EDID_HEADER                        0x00
#define EDID_HEADER_END                    0x07

#define ID_MANUFACTURER_NAME               0x08
#define ID_MANUFACTURER_NAME_END           0x09

#define EDID_STRUCT_VERSION                0x12
#define EDID_STRUCT_REVISION               0x13

#define UNKNOWN_DESCRIPTOR           (uint8_t)1
#define DETAILED_TIMING_BLOCK        (uint8_t)2
#define DESCRIPTOR_DATA                       5

#define DETAILED_TIMING_DESCRIPTIONS_START 0x36
#define DETAILED_TIMING_DESCRIPTION_SIZE     18
#define NUM_DETAILED_TIMING_DESCRIPTIONS      4

const uint8_t MONITOR_NAME                = 0xfc;

static uint8_t
blockType (uint8_t* block) noexcept
{
  if (  block [0] == 0 &&
        block [1] == 0 &&
        block [2] == 0 &&
        block [3] != 0 &&
        block [4] == 0    )
  {
    if (block [3] >= (uint8_t) 0xFA)
    {
      return
        block [3];
    }
  }

  return
    UNKNOWN_DESCRIPTOR;
}

static std::string
SK_EDID_GetMonitorNameFromBlock ( uint8_t const* block )
{
  char     name [13] = { };
  unsigned i;

  uint8_t const* ptr =
    block + DESCRIPTOR_DATA;

  for (i = 0; i < 13; ++i, ++ptr)
  {
    if (*ptr == 0xa)
    {
      name [i] = '\0';

      return
        name;
    }

    name [i] = *ptr;
  }

  return name;
}

std::string
SK_RenderBackend_V2::parseEDIDForName (uint8_t *edid, size_t length)
{
  std::string edid_name;

  unsigned int i        = 0;
  uint8_t*     block    = 0;
  uint8_t      checksum = 0;

  for (i = 0; i < length; ++i)
    checksum += edid [i];

  // Bad checksum, fail EDID
  if (checksum != 0)
  {
    SK_RunOnce (dll_log->Log (L"SK_EDID_Parse (...): Checksum fail"));
    //return "";
  }

  if ( 0 != memcmp ( (const char*)edid          + EDID_HEADER,
                     (const char*)edid_v1_header, EDID_HEADER_END + 1 ) )

  {
    dll_log->Log (L"SK_EDID_Parse (...): Not V1 Header");

    // Not a V1 header
    return "";
  }

  // Monitor name and timings
  block =
    &edid [DETAILED_TIMING_DESCRIPTIONS_START];

  uint8_t *end =
    &edid [length - 1];

#if 0
  int byte_idx = 0;

  while (block < end)
  {
    dll_log->Log (L"Byte %lu : %u", byte_idx++, (uint32_t) (*block));
    ++block;
  }
#endif

  while (block < end)
  {
    uint8_t type =
      blockType (block);

    switch (type)
    {
      case DETAILED_TIMING_BLOCK:
        block += DETAILED_TIMING_DESCRIPTION_SIZE;
        break;

      case UNKNOWN_DESCRIPTOR:
        ++block;
        break;

      case MONITOR_NAME:
      {
        uint8_t vendorString [5] = { };

        vendorString [0] =  (edid [8] >> 2   & 31)
                                             + 64;
        vendorString [1] = ((edid [8]  & 3) << 3) |
                            (edid [9] >> 5)  + 64;
        vendorString [2] =  (edid [9]  & 31) + 64;

        edid_name = (const char *)vendorString;
        edid_name += " ";
        edid_name +=
          SK_EDID_GetMonitorNameFromBlock (block);

        bool one_pt_4 =
          ((((unsigned) edid [10]) & 0xffU) == 4);

#define EDID_LOG2(x) {                              \
          if (one_pt_4) SK_LOG2 (x, L" EDID 1.4 "); \
          else          SK_LOG2 (x, L" EDID 1.3 "); }

        EDID_LOG2 ( ( L"SK_EDID_Parse (...): [ Name: %hs ]",
                      edid_name.c_str () ) );

      block += DETAILED_TIMING_DESCRIPTION_SIZE;
    } break;

    default:
      ++block;
      break;
    }
  }

  return edid_name;
}

POINT
SK_RenderBackend_V2::parseEDIDForNativeRes (uint8_t* edid, size_t length)
{
  unsigned int   i = 0;
  uint8_t checksum = 0;

  for (i = 0; i < length; ++i)
    checksum += edid [i];

  // Bad checksum, fail EDID
  if (checksum != 0)
  {
    SK_RunOnce (dll_log->Log (L"SK_EDID_Parse (...): Checksum fail"));
    return { };
  }

  if (0 != memcmp ((const char*)edid + EDID_HEADER,
                   (const char*)edid_v1_header, EDID_HEADER_END + 1))

  {
    dll_log->Log (L"SK_EDID_Parse (...): Not V1 Header");

    // Not a V1 header
    return { };
  }

  POINT ret = {
    ( (edid [58] >> 4) << 8) | edid [56],
    ( (edid [61] >> 4) << 8) | edid [59]
  };

  return ret;
}


//#include "../depends/include/nvapi/nvapi_lite_common.h"

static
const GUID
      GUID_CLASS_MONITOR =
    { 0x4d36e96e, 0xe325, 0x11ce, 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18 };

static
const GUID
      GUID_DEVINTERFACE_MONITOR =
    { 0xe6f07b5f, 0xee97, 0x4a90, 0xb0, 0x76, 0x33, 0xf5, 0x7b, 0xf4, 0xea, 0xa7 };

std::wstring
SK_GetKeyPathFromHKEY (HKEY& key)
{
#define STATUS_SUCCESS          ((NTSTATUS)0x00000000L)
#define STATUS_BUFFER_TOO_SMALL ((NTSTATUS)0xC0000023L)

  std::wstring keyPath;

  if (key != nullptr)
  {
    typedef DWORD (__stdcall *NtQueryKey_pfn)(
                       HANDLE KeyHandle,
                       int    KeyInformationClass,
                       PVOID  KeyInformation,
                       ULONG  Length,
                       PULONG ResultLength );

    static
      NtQueryKey_pfn    NtQueryKey =
      reinterpret_cast <NtQueryKey_pfn> (
        SK_GetProcAddress ( L"NtDll.dll",
                             "NtQueryKey" ) );

    if (NtQueryKey != nullptr)
    {
      DWORD size   = 0;
      DWORD result = 0;

      result =
        NtQueryKey (key, 3, nullptr, 0, &size);

      if (result == STATUS_BUFFER_TOO_SMALL)
      {
        size += 2;

        wchar_t* buffer =
          new (std::nothrow) wchar_t [size / sizeof (wchar_t)];

        if (buffer != nullptr)
        {
          result =
            NtQueryKey (key, 3, buffer, size, &size);

          if (result == STATUS_SUCCESS)
          {
            buffer [size / sizeof (wchar_t)] = L'\0';

            keyPath =
              std::wstring (buffer + 2);
          }

          delete [] buffer;
        }
      }
    }
  }

  return
    keyPath;
}

static volatile LONG lUpdatingOutputs = 0;

void
SK_RBkEnd_UpdateMonitorName ( SK_RenderBackend_V2::output_s& display,
                              DXGI_OUTPUT_DESC&              outDesc )
{
  static auto& rb =
    SK_GetCurrentRenderBackend ();

  if (*display.name == L'\0')
  {
    std::string edid_name;

    UINT devIdx = display.idx;

    wsprintf (display.name, L"UNKNOWN");

    bool nvSuppliedEDID = false;

    // This is known to return EDIDs with checksums that don't match expected,
    //   there's not much benefit to getting EDID this way, so use the registry instead.
#if 1
    if (sk::NVAPI::nv_hardware != false)
    {
      NvPhysicalGpuHandle nvGpuHandles [NVAPI_MAX_PHYSICAL_GPUS] = { };
      NvU32               nvGpuCount                             =   0;
      NvDisplayHandle     nvDisplayHandle;
      NvU32               nvOutputId  = std::numeric_limits <NvU32>::max ();
      NvU32               nvDisplayId = std::numeric_limits <NvU32>::max ();

      if ( NVAPI_OK ==
             NvAPI_GetAssociatedNvidiaDisplayHandle (
               SK_FormatString (R"(%ws)", outDesc.DeviceName).c_str (),
                 &nvDisplayHandle
             ) &&
           NVAPI_OK ==
             NvAPI_GetAssociatedDisplayOutputId (nvDisplayHandle, &nvOutputId) &&
           NVAPI_OK ==
             NvAPI_GetPhysicalGPUsFromDisplay   (nvDisplayHandle, nvGpuHandles,
                                                                 &nvGpuCount)  &&
           NVAPI_OK ==
             NvAPI_SYS_GetDisplayIdFromGpuAndOutputId (
               nvGpuHandles [0], nvOutputId,
                                &nvDisplayId
             )
         )
      {
        NV_EDID edid = {         };
        edid.version = NV_EDID_VER;

        if ( NVAPI_OK ==
               NvAPI_GPU_GetEDID (
                 nvGpuHandles [0],
                 nvDisplayId, &edid
               )
           )
        {
          edid_name =
            rb.parseEDIDForName ( edid.EDID_Data, edid.sizeofEDID );

          auto nativeRes =
            rb.parseEDIDForNativeRes ( edid.EDID_Data, edid.sizeofEDID );

          if (                      nativeRes.x != 0 &&
                                    nativeRes.y != 0 )
          { display.native.width  = nativeRes.x;
            display.native.height = nativeRes.y;
          }

          if (! edid_name.empty ())
          {
            nvSuppliedEDID = true;
          }
        }
      }
    }
#endif

    *display.name = L'\0';

     //@TODO - ELSE: Test support for various HDR colorspaces.

    DISPLAY_DEVICEW        disp_desc = { };
    disp_desc.cb = sizeof (disp_desc);

    if (EnumDisplayDevices ( outDesc.DeviceName, 0,
                               &disp_desc, EDD_GET_DEVICE_INTERFACE_NAME ))
    {
      if (! nvSuppliedEDID)
      {
        HDEVINFO devInfo =
          SetupDiGetClassDevsEx ( &GUID_CLASS_MONITOR,
                                    nullptr, nullptr,
                                      DIGCF_PRESENT,
                                        nullptr, nullptr, nullptr );

        if ((! nvSuppliedEDID) && devInfo != nullptr)
        {
          wchar_t   wszDevName [ 64] = { };
          wchar_t   wszDevInst [128] = { };
          wchar_t* pwszTok           = nullptr;

          swscanf (disp_desc.DeviceID, LR"(\\?\DISPLAY#%63ws)", wszDevName);

          pwszTok =
            StrStrIW (wszDevName, L"#");

          if (pwszTok != nullptr)
          {
            *pwszTok = L'\0';
            wcsncpy_s ( wszDevInst,  128,
                        pwszTok + 1, _TRUNCATE );

            pwszTok =
              StrStrIW (wszDevInst, L"#");

            if (pwszTok != nullptr)
               *pwszTok  = L'\0';


            uint8_t EDID_Data [256] = { };
            DWORD   edid_size       =  sizeof (EDID_Data);


            DWORD   dwType = REG_NONE;
            LRESULT lStat  =
              RegGetValueW ( HKEY_LOCAL_MACHINE,
                SK_FormatStringW ( LR"(SYSTEM\CurrentControlSet\Enum\DISPLAY\)"
                                   LR"(%ws\%ws\Device Parameters)",
                                     wszDevName, wszDevInst ).c_str (),
                              L"EDID",
                                RRF_RT_REG_BINARY, &dwType,
                                  EDID_Data, &edid_size );

            if (ERROR_SUCCESS == lStat)
            {
              edid_name =
                rb.parseEDIDForName ( EDID_Data, edid_size );

              auto nativeRes =
                rb.parseEDIDForNativeRes ( EDID_Data, edid_size );

              if (                      nativeRes.x != 0 &&
                                        nativeRes.y != 0 )
              { display.native.width  = nativeRes.x;
                display.native.height = nativeRes.y;
              }
            }
          }
        }
      }

      if (EnumDisplayDevices (outDesc.DeviceName, 0, &disp_desc, 0))
      {
        if (SK_GetCurrentRenderBackend ().display_crc [display.idx] == 0)
        {
          dll_log->Log ( L"[Output Dev] DeviceName: %ws, devIdx: %lu, DeviceID: %ws",
                           disp_desc.DeviceName, devIdx,
                             disp_desc.DeviceID );
        }

        wsprintf ( display.name, edid_name.empty () ?
                                    LR"(%ws (%ws))" :
                                      L"%hs",
                     edid_name.empty () ?
                       disp_desc.DeviceString :
            (WCHAR *)edid_name.c_str (),
                         disp_desc.DeviceName );
      }
    }
  }
}

extern void
SK_Display_ResolutionSelectUI (bool bMarkDirty);

void
SK_RenderBackend_V2::queueUpdateOutputs (void)
{
  update_outputs = true;
}

void
SK_RenderBackend_V2::updateOutputTopology (void)
{
  gsync_state.update (true);

  SK_Display_ResolutionSelectUI (true);

  SK_ComPtr <IDXGIAdapter> pAdapter;

  auto _GetAdapter = [&]( IUnknown*      swapchain,
               SK_ComPtr <IDXGIAdapter>& pAdapter ) ->
  bool
  {
    SK_ComQIPtr
     <IDXGISwapChain>
          pSwapChain (swapchain);

    if (! pSwapChain.p)
    {
      if (factory.p == nullptr || (! factory->IsCurrent ()))
      {
        factory.Release ();

        if (CreateDXGIFactory1_Import != nullptr)
            CreateDXGIFactory1_Import (IID_IDXGIFactory1, (void **)&factory.p);
      }

      SK_ComPtr <IDXGIAdapter>
                 pTempAdapter;

      if (SUCCEEDED (factory->EnumAdapters (0, &pTempAdapter.p)))
      {
        pAdapter = pTempAdapter;
      }

      return
        ( pAdapter.p != nullptr );
    }


    LUID                       luidAdapter = { };
    SK_ComPtr <IDXGIFactory1>  pFactory1;

    if (SUCCEEDED (pSwapChain->GetParent (IID_PPV_ARGS (&pFactory1.p))))
    {
      // We might have cached hooks that make this impossible
      if (CreateDXGIFactory1_Import != nullptr)
      {
        if (factory.p == nullptr)
        {
          if (! pFactory1->IsCurrent ()) // Stale factories must be retired
          {
            CreateDXGIFactory1_Import (IID_IDXGIFactory1, (void **)&factory.p);
          }
        }

        if (factory.p != nullptr)
        {
          if (! factory->IsCurrent ()) // Stale factories must be retired
          {
            factory.Release ();
            CreateDXGIFactory1_Import (IID_IDXGIFactory1, (void **)&factory.p);
          }

          if (factory != nullptr)
            pFactory1 = factory;
        }
      }

      else
      {
        SK_LOG0 ( ( L"DXGI Factory is Stale, but hook caching does not permit creation of a new one." ),
                    L"   DXGI   " );
      }
    }

    SK_ComPtr <ID3D12Device>                             pDevice12; // D3D12
    if (SUCCEEDED (pSwapChain->GetDevice (IID_PPV_ARGS (&pDevice12.p))))
    {
      // Yep, now for the fun part
      luidAdapter =
        pDevice12->GetAdapterLuid ();
    }

    // D3D11 or something else
    else
    {
      SK_ComPtr <IDXGIDevice>                                         pDevice;
      if ( FAILED (pSwapChain->GetDevice  (IID_IDXGIDevice, (void **)&pDevice.p)) ||
           FAILED (   pDevice->GetAdapter (                         &pAdapter.p)) )
        return false;

      DXGI_ADAPTER_DESC   adapterDesc = { };
      pAdapter->GetDesc (&adapterDesc);

      luidAdapter =
        adapterDesc.AdapterLuid;
    }

    SK_ComQIPtr <IDXGIFactory4>
        pFactory4   (pFactory1);
    if (pFactory4 != nullptr)
    {
      pAdapter.Release ();

      if ( FAILED (
             pFactory4->EnumAdapterByLuid ( luidAdapter,
                                       IID_IDXGIAdapter,
                                     (void **)&pAdapter.p )
                  )
         ) return false;
    }

    else if (pFactory1 != nullptr)
    {
      SK_ComPtr <IDXGIAdapter> pNewAdapter;

      for ( auto                      idx = 0 ; SUCCEEDED (
            pFactory1->EnumAdapters ( idx, &pNewAdapter.p ) ) ;
                                    ++idx )
      {
        DXGI_ADAPTER_DESC      adapterDesc = { };
        pNewAdapter->GetDesc (&adapterDesc);

        if (adapterDesc.AdapterLuid.HighPart == luidAdapter.HighPart &&
            adapterDesc.AdapterLuid.LowPart  == luidAdapter.LowPart)
        {
          pAdapter = pNewAdapter;
          break;
        }
      }
    }

    return
      ( pAdapter.p != nullptr );
  };


  while (InterlockedCompareExchange (&lUpdatingOutputs, 1, 0) != 0)
    ;


  if (! _GetAdapter (swapchain.p, pAdapter))
  {
    InterlockedCompareExchange (&lUpdatingOutputs, 0, 1);
    return;
  }


  UINT32 uiNumPaths =  128;
  UINT32 uiNumModes = 1024;

  DISPLAYCONFIG_PATH_INFO *pathArray = nullptr;
  DISPLAYCONFIG_MODE_INFO *modeArray = nullptr;

  if ( ERROR_SUCCESS ==
         GetDisplayConfigBufferSizes ( QDC_ONLY_ACTIVE_PATHS, &uiNumPaths,          &uiNumModes )
                                                            && uiNumPaths <= 128 &&  uiNumModes <= 1024 )
  {
    SK_TLS *pTLS =
      SK_TLS_Bottom ();

    pathArray = (DISPLAYCONFIG_PATH_INFO *)pTLS->scratch_memory->ccd.display_paths.alloc (uiNumPaths);
    modeArray = (DISPLAYCONFIG_MODE_INFO *)pTLS->scratch_memory->ccd.display_modes.alloc (uiNumModes);

    if ( ERROR_SUCCESS != QueryDisplayConfig ( QDC_ONLY_ACTIVE_PATHS, &uiNumPaths, pathArray,
                                                                      &uiNumModes, modeArray, nullptr ) )
    {
      SK_ReleaseAssert (! "QueryDisplayConfig (QDC_ONLY_ACTIVE_PATHS");
    }
  }


  static constexpr
    size_t display_size = sizeof (SK_RenderBackend_V2::output_s),
       num_displays     = sizeof ( displays) /
                                   display_size;

  bool display_changed [num_displays] = { };

  for ( auto idx = 0 ; idx < num_displays ; ++idx )
  {
    RtlSecureZeroMemory (
      &displays [idx], display_size
    );
  }

  UINT                             idx = 0;
  SK_ComPtr <IDXGIOutput>                pOutput;

  while ( DXGI_ERROR_NOT_FOUND !=
            pAdapter->EnumOutputs (idx, &pOutput.p) )
  {
    auto& display =
      displays [idx];

    if (pOutput.p != nullptr)
    {
      DXGI_OUTPUT_DESC1 outDesc1 = { };
      DXGI_OUTPUT_DESC  outDesc  = { };

      if (SUCCEEDED (pOutput->GetDesc (&outDesc)))
      {
        display.idx      = idx;
        display.monitor  = outDesc.Monitor;
        display.rect     = outDesc.DesktopCoordinates;
        display.attached = outDesc.AttachedToDesktop;


        if (sk::NVAPI::nv_hardware != false)
        {
          NvPhysicalGpuHandle nvGpuHandles [NVAPI_MAX_PHYSICAL_GPUS] = { };
          NvU32               nvGpuCount                             =   0;
          NvDisplayHandle     nvDisplayHandle;
          NvU32               nvOutputId  = std::numeric_limits <NvU32>::max ();
          NvU32               nvDisplayId = std::numeric_limits <NvU32>::max ();

          if ( NVAPI_OK ==
                 NvAPI_GetAssociatedNvidiaDisplayHandle (
                   SK_FormatString (R"(%ws)", outDesc.DeviceName).c_str (),
                     &nvDisplayHandle
                 ) &&
               NVAPI_OK ==
                 NvAPI_GetAssociatedDisplayOutputId (nvDisplayHandle, &nvOutputId) &&
               NVAPI_OK ==
                 NvAPI_GetPhysicalGPUsFromDisplay   (nvDisplayHandle, nvGpuHandles,
                                                                     &nvGpuCount)  &&
               NVAPI_OK ==
                 NvAPI_SYS_GetDisplayIdFromGpuAndOutputId (
                   nvGpuHandles [0], nvOutputId,
                                    &nvDisplayId
                 )
             )
          {
            display.nvapi.display_handle = nvDisplayHandle;
            display.nvapi.gpu_handle     = nvGpuHandles [0];
            display.nvapi.display_id     = nvDisplayId;
            display.nvapi.output_id      = nvOutputId;
          }
        }

        wcsncpy_s ( display.dxgi_name,  32,
                    outDesc.DeviceName, _TRUNCATE );

        SK_RBkEnd_UpdateMonitorName (display, outDesc);

        MONITORINFO
          minfo        = {                  };
          minfo.cbSize = sizeof (MONITORINFO);

        GetMonitorInfo (display.monitor, &minfo);

        float bestIntersectArea = -1.0f;

        int ax1 = minfo.rcMonitor.left,
            ax2 = minfo.rcMonitor.right;
        int ay1 = minfo.rcMonitor.top,
            ay2 = minfo.rcMonitor.bottom;

        DISPLAYCONFIG_PATH_INFO *pVidPn = nullptr;

        for (UINT32 pathIdx = 0; pathIdx < uiNumPaths; ++pathIdx)
        {
          auto *path =
            &pathArray [pathIdx];

          if (path->flags & DISPLAYCONFIG_PATH_ACTIVE)
          {
            DISPLAYCONFIG_SOURCE_MODE *pSourceMode =
              &modeArray [path->sourceInfo.modeInfoIdx].sourceMode;

            RECT rect;
            rect.left   = pSourceMode->position.x;
            rect.top    = pSourceMode->position.y;
            rect.right  = pSourceMode->position.x + pSourceMode->width;
            rect.bottom = pSourceMode->position.y + pSourceMode->height;

            if (! IsRectEmpty (&rect))
            {
              int bx1 = rect.left;
              int by1 = rect.top;
              int bx2 = rect.right;
              int by2 = rect.bottom;

              int intersectArea =
                ComputeIntersectionArea (ax1, ay1, ax2, ay2, bx1, by1, bx2, by2);

              if (intersectArea > bestIntersectArea)
              {
                pVidPn            = path;
                bestIntersectArea =
                  static_cast <float> (intersectArea);
              }
            }
          }
        }

        if (pVidPn != nullptr)
        {
          display.vidpn = *pVidPn;

          DISPLAYCONFIG_GET_ADVANCED_COLOR_INFO
            getHdrInfo                  = { };
            getHdrInfo.header.type      = DISPLAYCONFIG_DEVICE_INFO_GET_ADVANCED_COLOR_INFO;
            getHdrInfo.header.size      = sizeof     (DISPLAYCONFIG_GET_ADVANCED_COLOR_INFO);
            getHdrInfo.header.adapterId = display.vidpn.targetInfo.adapterId;
            getHdrInfo.header.id        = display.vidpn.targetInfo.id;

          if ( ERROR_SUCCESS == DisplayConfigGetDeviceInfo ( (DISPLAYCONFIG_DEVICE_INFO_HEADER *)&getHdrInfo ) )
          {
            display.hdr.supported = getHdrInfo.advancedColorSupported;
            display.hdr.enabled   = getHdrInfo.advancedColorEnabled;
            display.hdr.encoding  = getHdrInfo.colorEncoding;
            display.bpc           = getHdrInfo.bitsPerColorChannel;
          }

          else
          {
            display.hdr.supported = false;
            display.hdr.enabled   = false;
          }

          DISPLAYCONFIG_SDR_WHITE_LEVEL
            getSdrWhiteLevel                  = { };
            getSdrWhiteLevel.header.type      = DISPLAYCONFIG_DEVICE_INFO_GET_SDR_WHITE_LEVEL;
            getSdrWhiteLevel.header.size      = sizeof         (DISPLAYCONFIG_SDR_WHITE_LEVEL);
            getSdrWhiteLevel.header.adapterId = display.vidpn.targetInfo.adapterId;
            getSdrWhiteLevel.header.id        = display.vidpn.targetInfo.id;

          if ( ERROR_SUCCESS == DisplayConfigGetDeviceInfo ( (DISPLAYCONFIG_DEVICE_INFO_HEADER *)&getSdrWhiteLevel ) )
          {
            display.hdr.white_level =
              (float)(((double)getSdrWhiteLevel.SDRWhiteLevel / 1000.0) * 80.0);
          }

          else
            display.hdr.white_level = 80.0f;

          DISPLAYCONFIG_TARGET_PREFERRED_MODE
            getPreferredMode                  = { };
            getPreferredMode.header.type
              = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_PREFERRED_MODE;
            getPreferredMode.header.size      =         sizeof (DISPLAYCONFIG_TARGET_PREFERRED_MODE);
            getPreferredMode.header.adapterId = display.vidpn.targetInfo.adapterId;
            getPreferredMode.header.id        = display.vidpn.targetInfo.id;

          if ( ERROR_SUCCESS == DisplayConfigGetDeviceInfo ( (DISPLAYCONFIG_DEVICE_INFO_HEADER *)&getPreferredMode ) )
          {
            display.native.width   = getPreferredMode.width;
            display.native.height  = getPreferredMode.height;

            display.native.refresh = {
              getPreferredMode.targetMode.targetVideoSignalInfo.vSyncFreq.Numerator,
              getPreferredMode.targetMode.targetVideoSignalInfo.vSyncFreq.Denominator
            };
          }

          DISPLAYCONFIG_TARGET_DEVICE_NAME
            getTargetName                     = { };
            getTargetName.header.type         = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
            getTargetName.header.size         =  sizeof (DISPLAYCONFIG_TARGET_DEVICE_NAME);
            getTargetName.header.adapterId    = display.vidpn.targetInfo.adapterId;
            getTargetName.header.id           = display.vidpn.targetInfo.id;

          if ( ERROR_SUCCESS == DisplayConfigGetDeviceInfo ( (DISPLAYCONFIG_DEVICE_INFO_HEADER *)&getTargetName ) )
          {
            wcsncpy_s ( display.name,                                   64,
                        getTargetName.monitorFriendlyDeviceName, _TRUNCATE );
          }
        }

        // Didn't get a name using the Windows APIs, let's fallback to EDID
        if (*display.name == L'\0')
        {
          if (sk::NVAPI::nv_hardware)
          {
            NV_EDID edid = {         };
                    edid.version = NV_EDID_VER;

            std::string edid_name;

            if ( NVAPI_OK ==
                   NvAPI_GPU_GetEDID (
                     display.nvapi.gpu_handle,
                     display.nvapi.display_id, &edid
                   )
               )
            {
              edid_name =
                parseEDIDForName ( edid.EDID_Data, edid.sizeofEDID );
            }

            if (! edid_name.empty ())
            {
              wsprintf (display.name, L"%hs", edid_name.c_str ());
            }
          }
        }

        display.primary =
          ( minfo.dwFlags & MONITORINFOF_PRIMARY );

        SK_ComQIPtr <IDXGIOutput6>
            pOutput6 (pOutput);
        if (pOutput6.p != nullptr)
        {
          if (SUCCEEDED (pOutput6->GetDesc1 (&outDesc1)))
          {
            if (outDesc1.MinLuminance > outDesc1.MaxFullFrameLuminance)
              std::swap (outDesc1.MinLuminance, outDesc1.MaxFullFrameLuminance);

            static bool once = false;

            if (! std::exchange (once, true))
            {
              SK_LOG0 ( (L" --- Working around DXGI bug, swapping invalid min / avg luminance levels"),
                         L"   DXGI   "
                      );
            }

            display.bpc               = outDesc1.BitsPerColor;
            display.gamut.minY        = outDesc1.MinLuminance;
            display.gamut.maxY        = outDesc1.MaxLuminance;
            display.gamut.maxLocalY   = outDesc1.MaxLuminance;
            display.gamut.maxAverageY = outDesc1.MaxFullFrameLuminance;
            display.gamut.xr          = outDesc1.RedPrimary   [0];
            display.gamut.yr          = outDesc1.RedPrimary   [1];
            display.gamut.xb          = outDesc1.BluePrimary  [0];
            display.gamut.yb          = outDesc1.BluePrimary  [1];
            display.gamut.xg          = outDesc1.GreenPrimary [0];
            display.gamut.yg          = outDesc1.GreenPrimary [1];
            display.gamut.Xw          = outDesc1.WhitePoint   [0];
            display.gamut.Yw          = outDesc1.WhitePoint   [1];
            display.gamut.Zw          = 1.0f - display.gamut.Xw - display.gamut.Yw;
            display.colorspace        = outDesc1.ColorSpace;

            display.hdr.enabled       = outDesc1.ColorSpace ==
              DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
          }
        }
      }
    }

    stale_display_info = false;

    pOutput.Release ();

    idx++;
  }

  idx = 0;

  while ( DXGI_ERROR_NOT_FOUND !=
            pAdapter->EnumOutputs (idx, &pOutput.p) )
  {
    auto& display =
      displays [idx];

    if (pOutput.p != nullptr)
    {
      auto old_crc =
       display_crc [idx];

       display_crc [idx] =
               crc32c ( 0,
      &display,
       display_size   );

      display_changed [idx] =
        old_crc != display_crc [idx];

      SK_ComQIPtr <IDXGISwapChain> pChain (swapchain.p);

      DXGI_SWAP_CHAIN_DESC swapDesc         = { };
      RECT                 rectOutputWindow = { };

      if (pChain.p != nullptr)
      {
        pChain->GetDesc (&swapDesc);
        GetWindowRect   ( swapDesc.OutputWindow, &rectOutputWindow);
      }

      auto pContainer =
        getContainingOutput (rectOutputWindow);

      if (pContainer != nullptr)
      {
        if (pContainer->monitor == display.monitor)
        {
          wcsncpy_s ( display_name,        128,
                      display.name, _TRUNCATE );

          // Late init
          if (monitor != display.monitor)
              monitor  = display.monitor;

          display_gamut.xr = display.gamut.xr;
          display_gamut.yr = display.gamut.yr;
          display_gamut.xg = display.gamut.xg;
          display_gamut.yg = display.gamut.yg;
          display_gamut.xb = display.gamut.xb;
          display_gamut.yb = display.gamut.yb;
          display_gamut.Xw = display.gamut.Xw;
          display_gamut.Yw = display.gamut.Yw;
          display_gamut.Zw = 1.0f - display.gamut.Xw - display.gamut.Yw;

          display_gamut.minY        = display.gamut.minY;
          display_gamut.maxY        = display.gamut.maxY;
          display_gamut.maxLocalY   = display.gamut.maxY;
          display_gamut.maxAverageY = display.gamut.maxAverageY;

          if (display.attached)
          {
            scanout.dwm_colorspace =
              display.colorspace;
          }

          SK_ComQIPtr <IDXGISwapChain3>
                 pSwap3 (swapchain.p);
          if (   pSwap3)
          {
            // Windows tends to cache this stuff, we're going to build our own with
            //   more up-to-date values instead.
            DXGI_OUTPUT_DESC1 uncachedOutDesc;
            uncachedOutDesc.BitsPerColor = pContainer->bpc;
            uncachedOutDesc.ColorSpace   = pContainer->colorspace;

            SK_DXGI_UpdateColorSpace (pSwap3.p, &uncachedOutDesc);

            if ((! isHDRCapable ()) && __SK_HDR_16BitSwap)
            {
              SK_RunOnce (
                SK_ImGui_WarningWithTitle (
                  L"ERROR: Special K HDR Applied to a non-HDR Display\r\n\r\n"
                  L"\t\t>> Please Disable SK HDR or set the Windows Desktop to use HDR",
                                           L"HDR is Unsupported by the Active Display" )
              );
            }
          }
        }
      }

      if (display_changed [idx])
      {
        dll_log->LogEx ( true,
          L"[Output Dev]\n"
          L"  +------------------+---------------------\n"
          L"  | EDID Device Name |  %ws\n"
          L"  | DXGI Device Name |  %ws (HMONITOR: %x)\n"
          L"  | Desktop Display. |  %ws%ws\n"
          L"  | Bits Per Color.. |  %u\n"
          L"  | Color Space..... |  %s\n"
          L"  | Red Primary..... |  %f, %f\n"
          L"  | Green Primary... |  %f, %f\n"
          L"  | Blue Primary.... |  %f, %f\n"
          L"  | White Point..... |  %f, %f\n"
          L"  | Min Luminance... |  %f\n"
          L"  | Max Luminance... |  %f\n"
          L"  |  \"  FullFrame... |  %f\n"
          L"  +------------------+---------------------\n",
            display.name,
            display.dxgi_name, display.monitor,
            display.attached ? L"Yes"                : L"No",
            display.primary  ? L" (Primary Display)" : L"",
                        display.bpc,
            DXGIColorSpaceToStr (display.colorspace),
            display.gamut.xr,    display.gamut.yr,
            display.gamut.xg,    display.gamut.yg,
            display.gamut.xb,    display.gamut.yb,
            display.gamut.Xw,    display.gamut.Yw,
            display.gamut.minY,  display.gamut.maxY,
            display.gamut.maxAverageY
        );
      }

      pOutput.Release ();
    }

    idx++;
  }

  bool any_changed =
    std::count ( std::begin (display_changed),
                 std::end   (display_changed), true ) > 0;

  if (any_changed)
  {
    for ( UINT i = 0 ; i < idx; ++i )
    {
      SK_LOG0 ( ( L"%s Monitor %i: [ %ix%i | (%5i,%#5i) ] \"%ws\" :: %s",
                    displays [i].primary ? L"*" : L" ",
                    displays [i].idx,
                    displays [i].rect.right  - displays [i].rect.left,
                    displays [i].rect.bottom - displays [i].rect.top,
                    displays [i].rect.left,    displays [i].rect.top,
                    displays [i].name,
                    displays [i].hdr.enabled ? L"HDR" : L"SDR" ),
                  L"   DXGI   " );
    }
  }

  InterlockedDecrement (&lUpdatingOutputs);
}

const SK_RenderBackend_V2::output_s*
SK_RenderBackend_V2::getContainingOutput (const RECT& rkRect)
{
  //while (InterlockedCompareExchange (&lUpdatingOutputs, 1, 0) != 0)
  //  ;

  const output_s* pOutput = nullptr;

  float bestIntersectArea = -1.0f;

  int ax1 = rkRect.left,
      ax2 = rkRect.right;
  int ay1 = rkRect.top,
      ay2 = rkRect.bottom;

  for ( auto& it : displays )
  {
    if (! IsRectEmpty (&it.rect))
    {
      int bx1 = it.rect.left;
      int by1 = it.rect.top;
      int bx2 = it.rect.right;
      int by2 = it.rect.bottom;

      int intersectArea =
        ComputeIntersectionArea (ax1, ay1, ax2, ay2, bx1, by1, bx2, by2);

      if (intersectArea > bestIntersectArea)
      {
        pOutput           = &it;
        bestIntersectArea =
          static_cast <float> (intersectArea);
      }
    }
  }

  //InterlockedDecrement (&lUpdatingOutputs);

  return pOutput;
}

volatile ULONG64 SK_Reflex_LastFrameMarked   = 0;
volatile LONG    SK_RenderBackend::flip_skip = 0;

bool
SK_RenderBackend_V2::setLatencyMarkerNV (NV_LATENCY_MARKER_TYPE marker)
{
  NvAPI_Status ret =
    NVAPI_INVALID_CONFIGURATION;

  if (sk::NVAPI::nv_hardware && device.p != nullptr)
  {
    NV_LATENCY_MARKER_PARAMS
      markerParams            = {                          };
      markerParams.version    = NV_LATENCY_MARKER_PARAMS_VER;
      markerParams.markerType = marker;
      markerParams.frameID    = static_cast <NvU64> (
           ReadULong64Acquire (&frames_drawn)       );

    ret =
      NvAPI_D3D_SetLatencyMarker (device.p, &markerParams);
  }

  if ( marker == RENDERSUBMIT_END /*||
       marker == RENDERSUBMIT_START*/ )
  {
    latency.submitQueuedFrame (
      SK_ComQIPtr <IDXGISwapChain1> (swapchain)
    );
  }

  return
    ( ret == NVAPI_OK );
}

bool
SK_RenderBackend_V2::getLatencyReportNV (NV_LATENCY_RESULT_PARAMS* pGetLatencyParams)
{
  if (! sk::NVAPI::nv_hardware)
    return false;

  if (device.p == nullptr)
    return false;

  NvAPI_Status ret =
    NvAPI_D3D_GetLatency (device.p, pGetLatencyParams);

  return
    ( ret == NVAPI_OK );
}


void
SK_RenderBackend_V2::driverSleepNV (int site)
{
  if (! sk::NVAPI::nv_hardware)
    return;

  if (! device.p)
    return;

  if (site == 2)
    setLatencyMarkerNV (INPUT_SAMPLE);

  if (site == config.nvidia.sleep.enforcement_site)
  {
    static bool
      valid = true;

    if (! valid)
      return;

    if (config.nvidia.sleep.frame_interval_us != 0)
    {
      ////extern float __target_fps;
      ////
      ////if (__target_fps > 0.0)
      ////  config.nvidia.sleep.frame_interval_us = static_cast <UINT> ((1000.0 / __target_fps) * 1000.0);
      ////else
      config.nvidia.sleep.frame_interval_us = 0;
    }

    NV_SET_SLEEP_MODE_PARAMS
      sleepParams = {                          };
      sleepParams.version               = NV_SET_SLEEP_MODE_PARAMS_VER;
      sleepParams.bLowLatencyBoost      = config.nvidia.sleep.low_latency_boost;
      sleepParams.bLowLatencyMode       = config.nvidia.sleep.low_latency;
      sleepParams.minimumIntervalUs     = config.nvidia.sleep.frame_interval_us;
      sleepParams.bUseMarkersToOptimize = config.nvidia.sleep.marker_optimization;

    static NV_SET_SLEEP_MODE_PARAMS
      lastParams = { 1, true, true, 69, 0, { 0 } };

    if (! config.nvidia.sleep.enable)
    {
      sleepParams.bLowLatencyBoost      = false;
      sleepParams.bLowLatencyMode       = false;
      sleepParams.bUseMarkersToOptimize = false;
      sleepParams.minimumIntervalUs     = 0;
    }

    static volatile ULONG64 _frames_drawn =
      std::numeric_limits <ULONG64>::max ();
    if ( ReadULong64Acquire (&_frames_drawn) ==
         ReadULong64Acquire  (&frames_drawn) )
      return;

    SK_Thread_ScopedPriority
            __scoped_prio (THREAD_PRIORITY_TIME_CRITICAL);

    //if ( lastParams.bLowLatencyBoost  != sleepParams.bLowLatencyBoost ||
    //     lastParams.bLowLatencyMode   != sleepParams.bLowLatencyMode  ||
    //     lastParams.minimumIntervalUs != sleepParams.minimumIntervalUs )
    {
      if ( NVAPI_OK !=
             NvAPI_D3D_SetSleepMode (
               device.p, &sleepParams
             )
         ) valid = false;

      else
      {
        //NV_GET_SLEEP_STATUS_PARAMS
        //  getParams         = {                            };
        //  getParams.version = NV_GET_SLEEP_STATUS_PARAMS_VER;

        //NvAPI_D3D_Sleep (device.p);
        //
        //if ( NVAPI_OK ==
        //       NvAPI_D3D_GetSleepStatus (
        //         device.p, &getParams
        //       )
        //   )
        //{
        //  config.nvidia.sleep.low_latency = getParams.bLowLatencyMode;
        //
        //  if (! config.nvidia.sleep.low_latency)
        //        config.nvidia.sleep.low_latency_boost = false;
        //
        //  lastParams.bLowLatencyMode  = getParams.bLowLatencyMode;
        //  lastParams.bLowLatencyBoost = config.nvidia.sleep.low_latency_boost;
        //}

        lastParams = sleepParams;
      }
    }

    if (config.nvidia.sleep.enable)
    {
      if ( NVAPI_OK != NvAPI_D3D_Sleep (device.p) )
        valid = false;
    }

    WriteULong64Release (&_frames_drawn,
      ReadULong64Acquire (&frames_drawn));

    if ((! valid) && ( api == SK_RenderAPI::D3D11 ||
                       api == SK_RenderAPI::D3D12 ))
    {
      SK_LOG0 ( ( L"NVIDIA Reflex Sleep Invalid State" ),
                  __SK_SUBSYSTEM__ );
    }
  }
};

void
SK_NV_AdaptiveSyncControl (void)
{
  if (sk::NVAPI::nv_hardware != false)
  {
    static auto& rb =
      SK_GetCurrentRenderBackend ();

    for ( auto& display : rb.displays )
    {
      if (display.monitor == rb.monitor)
      {
        NV_GET_ADAPTIVE_SYNC_DATA
                     getAdaptiveSync       = {                           };
        ZeroMemory (&getAdaptiveSync,  sizeof (NV_GET_ADAPTIVE_SYNC_DATA));
                     getAdaptiveSync.version = NV_GET_ADAPTIVE_SYNC_DATA_VER;

        static DWORD lastChecked       = 0;
        static NvU64 lastFlipTimeStamp = 0;
        static NvU64 lastFlipFrame     = 0;
        static double   dFlipPrint     = 0.0;

        if (SK_timeGetTime () > lastChecked + 333)
        {                       lastChecked = SK_timeGetTime ();
          if ( NVAPI_OK ==
                 NvAPI_DISP_GetAdaptiveSyncData (
                   display.nvapi.display_id,
                           &getAdaptiveSync )
             )
          {
            NvU64 deltaFlipTime     = getAdaptiveSync.lastFlipTimeStamp - lastFlipTimeStamp;
                  lastFlipTimeStamp = getAdaptiveSync.lastFlipTimeStamp;

            if (deltaFlipTime > 0)
            {
              double dFlipRate  =
                static_cast <double> (SK_GetFramesDrawn () - lastFlipFrame) *
              ( static_cast <double> (deltaFlipTime) /
                static_cast <double> (SK_GetPerfFreq ().QuadPart) );

                 dFlipPrint = dFlipRate;
              lastFlipFrame = SK_GetFramesDrawn ();
            }

            rb.gsync_state.update (true);
          }

          else lastChecked = SK_timeGetTime () + 333;
        }

        ImGui::Text       ("Adaptive Sync Status for %ws", rb.display_name);
        ImGui::Separator  ();
        ImGui::BeginGroup ();
        ImGui::Text       ("Current State:");
        if (! getAdaptiveSync.bDisableAdaptiveSync)
        {
          ImGui::Text     ("Frame Splitting:");

          if (getAdaptiveSync.maxFrameInterval != 0)
            ImGui::Text   ("Max Frame Interval:");
        }
        ImGui::Text       ("");
      //ImGui::Text       ("Effective Refresh:");
        ImGui::EndGroup   ();
        ImGui::SameLine   ();
        ImGui::BeginGroup ();

        ImGui::Text       ( getAdaptiveSync.bDisableAdaptiveSync   ? "Disabled" :
                                             rb.gsync_state.active ? "Active"   :
                                                                     "Inactive" );
        if (! getAdaptiveSync.bDisableAdaptiveSync)
        {
          ImGui::Text     ( getAdaptiveSync.bDisableFrameSplitting ? "Disabled" :
                                                                     "Enabled" );

          if (getAdaptiveSync.maxFrameInterval != 0)
            ImGui::Text   ( "%#06.2f Hz ",
                             1000000.0 / static_cast <double> (getAdaptiveSync.maxFrameInterval) );
        }

      //ImGui::Text       ( "%#06.2f Hz ", dFlipPrint);
        ImGui::Text       ( "" );
        ImGui::Text       ( "\t\t\t\t\t\t\t\t" );
        ImGui::EndGroup   ();
        ImGui::SameLine   ();
        ImGui::BeginGroup ();

        static bool secret_menu = false;

        if (ImGui::IsItemClicked (1))
          secret_menu = true;

        bool toggle_sync  = false;
        bool toggle_split = false;

        if (secret_menu)
        {
          toggle_sync =
            ImGui::Button (
              getAdaptiveSync.bDisableAdaptiveSync == 0x0 ?
                            "Disable Adaptive Sync"       :
                             "Enable Adaptive Sync" );

          toggle_split =
            ImGui::Button (
              getAdaptiveSync.bDisableFrameSplitting == 0x0 ?
                            "Disable Frame Splitting"       :
                             "Enable Frame Splitting" );
        }

        ImGui::EndGroup   ();

        if (toggle_sync || toggle_split)
        {
          NV_SET_ADAPTIVE_SYNC_DATA
                       setAdaptiveSync;
          ZeroMemory (&setAdaptiveSync,  sizeof (NV_SET_ADAPTIVE_SYNC_DATA));
                       setAdaptiveSync.version = NV_SET_ADAPTIVE_SYNC_DATA_VER;

          setAdaptiveSync.bDisableAdaptiveSync   =
            toggle_sync ? !getAdaptiveSync.bDisableAdaptiveSync :
                           getAdaptiveSync.bDisableAdaptiveSync;
          setAdaptiveSync.bDisableFrameSplitting =
            toggle_split ? !getAdaptiveSync.bDisableFrameSplitting :
                            getAdaptiveSync.bDisableFrameSplitting;

          NvAPI_DISP_SetAdaptiveSyncData (
            display.nvapi.display_id,
                    &setAdaptiveSync
          );
        }
        break;
      }
    }
  }
}