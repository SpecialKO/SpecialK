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


#include <SpecialK/nvapi.h>
#include <SpecialK/resource.h> // Unpack shader compiler

volatile LONG64 SK_RenderBackend::frames_drawn = 0;


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


SK_LazyGlobal <SK_RenderBackend> __SK_RBkEnd;

SK_RenderBackend&
__stdcall
SK_GetCurrentRenderBackend (void)
{
  static auto& rb =
    __SK_RBkEnd.get ();

  return rb;
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

  if (! InterlockedCompareExchangeAcquire (&__booted, TRUE, FALSE))
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
  // "Normal" games don't change render APIs mid-game; Talos does, but it's
  //   not normal :)
  if (SK_GetFramesDrawn () > 0)
    return;


  SK_DXGI_QuickHook ();

  // Establish the minimal set of APIs necessary to work as dxgi.dll
  if (SK_GetDLLRole () == DLL_ROLE::DXGI)
  {
    if (! config.apis.dxgi.d3d12.hook)
          config.apis.dxgi.d3d11.hook = true;
  }

#ifdef _M_AMD64
  //
  // TEMP HACK: D3D11 must be enabled to hook D3D12...
  //
  ///if (config.apis.dxgi.d3d12.hook && (! config.apis.dxgi.d3d11.hook))
  ///  config.apis.dxgi.d3d11.hook = true;
#endif

  if (! ( config.apis.dxgi.d3d11.hook ||
          config.apis.dxgi.d3d12.hook ))
    return;

  static volatile LONG __booted = FALSE;

  if (! InterlockedCompareExchangeAcquire (&__booted, TRUE, FALSE))
  {
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

  if (! InterlockedCompareExchangeAcquire (&__booted, TRUE, FALSE))
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
SK_RenderBackend_V2::gsync_s::update (void)
{
  static auto& rb =
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

  if (! (config.apis.NvAPI.gsync_status &&
                 sk::NVAPI::nv_hardware) )
  {
    capable = false;

    return
      _ClearTemporarySurfaces ();
  }

  bool success = false;

  DWORD dwTimeNow =
    timeGetTime ();

  if ( last_checked < (dwTimeNow - 666UL) )
  {    last_checked =  dwTimeNow;

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
  static auto& rb =
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

        if (config.render.framerate.refresh_rate != -1)
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
               pSwapChain->SetFullscreenState (TRUE, nullptr)
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
SK_RenderBackend_V2::getActiveRefreshRate (void)
{
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
      UINT64 MAX_CACHE_TTL = 500; // 500 Frames

    static
      UINT64 uiCachedFrame =
        SK_GetFramesDrawn ();

    static float   fLastCache      = 0.0f;
    static UINT64 uiLastCacheFrame = uiCachedFrame - MAX_CACHE_TTL;

    if ( refresh_refresh ||
         uiLastCacheFrame < (SK_GetFramesDrawn () - MAX_CACHE_TTL) )
    {
      if (pSwapChain.p != nullptr)
      {
        DXGI_MODE_DESC         idealModeDesc  = { };
        DXGI_MODE_DESC         activeModeDesc = { };
        DXGI_SWAP_CHAIN_DESC   swapDesc       = { };
        pSwapChain->GetDesc  (&swapDesc);

        SK_ComPtr <IDXGIDevice>           pSwapDevice = nullptr;
        SK_ComPtr <IDXGIOutput>           pSwapOutput = nullptr;
        pSwapChain->GetContainingOutput (&pSwapOutput);

        if ( SUCCEEDED (
             pSwapChain->GetDevice (__uuidof (IDXGIDevice), (void **)&pSwapDevice.p)
                       )
           )
        {
          idealModeDesc.Format                  = swapDesc.BufferDesc.Format;
          idealModeDesc.Height                  = swapDesc.BufferDesc.Height;
          idealModeDesc.Width                   = swapDesc.BufferDesc.Width;
          idealModeDesc.RefreshRate.Numerator   = swapDesc.BufferDesc.RefreshRate.Numerator;
          idealModeDesc.RefreshRate.Denominator = swapDesc.BufferDesc.RefreshRate.Denominator;
          idealModeDesc.Scaling                 = swapDesc.BufferDesc.Scaling;
          idealModeDesc.ScanlineOrdering        = swapDesc.BufferDesc.ScanlineOrdering;

          if ( SUCCEEDED (
                 SK_DXGI_FindClosestMode (
                   pSwapChain, &idealModeDesc, &activeModeDesc,
                   pSwapDevice    )
                         )
             )
          {
            uiLastCacheFrame = SK_GetFramesDrawn ();
             fLastCache      =
               ( static_cast <float> (activeModeDesc.RefreshRate.Numerator  ) /
                 static_cast <float> (activeModeDesc.RefreshRate.Denominator) );

             refresh_refresh = false;

             return
               fLastCache;
          }
        }

        DXGI_SWAP_CHAIN_DESC  swap_desc = { };
        pSwapChain->GetDesc (&swap_desc);

        if (swap_desc.BufferDesc.RefreshRate.Denominator != 0)
        {
          uiLastCacheFrame = SK_GetFramesDrawn ();
           fLastCache      =
             ( static_cast <float> (swap_desc.BufferDesc.RefreshRate.Numerator  ) /
               static_cast <float> (swap_desc.BufferDesc.RefreshRate.Denominator) );

           refresh_refresh = false;

           return
             fLastCache;
        }
      }
    }
  }

  refresh_refresh = false;

  return
    SK_Display_GetDefaultRefreshRate ();
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
  if ( swapchain_waithandle.m_h == 0    &&
       swapchain.p              != nullptr )
  {
    SK_ComQIPtr <IDXGISwapChain2>
        pSwap2       (swapchain.p);
    if (pSwap2.p != nullptr)
    {
      swapchain_waithandle.Attach (
        pSwap2.p->GetFrameLatencyWaitableObject ()
      );
    }
  }

  if ( swapchain_waithandle.m_h != 0 &&
       swapchain.p              == nullptr )
  {
    swapchain_waithandle.Close ();
  }

  return
    swapchain_waithandle.m_h;
}

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

    swapchain_waithandle.Close ();

    if (api != SK_RenderAPI::D3D11On12)
    {
      // Flushing at shutdown may cause deadlocks
#ifdef _USE_FLUSH
      if (d3d11.immediate_ctx != nullptr)
          d3d11.immediate_ctx->Flush ();
#endif
      swapchain = nullptr;// .Reset();
      if (interop.d3d12.dev == nullptr)
      device    = nullptr;//.Reset    ();
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

    d3d11.immediate_ctx = nullptr;

    void
    SK_HDR_ReleaseResources (void);
    SK_HDR_ReleaseResources ();
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


extern HWND WINAPI SK_GetFocus (void);

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

SK_RenderBackend_V2::scan_out_s::SK_HDR_TRANSFER_FUNC
SK_RenderBackend_V2::scan_out_s::getEOTF (void)
{
  static auto& rb =
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
  const DWORD dwNow = timeGetTime ();

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
  static const
    std::unordered_map <SK_RenderAPI, const wchar_t *>
      api_map {
        { SK_RenderAPI::D3D11,  L"D3D11" },
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

    RealGetWindowClassW        (hwnd, class_name, 127);
    InternalGetWindowText      (hwnd, title,      127);

    unicode = IsWindowUnicode  (hwnd);
    parent  = GetParent        (hwnd);

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

  if (SUCCEEDED (__HrLoadAllImportsForDll ("D3DCOMPILER_47.dll")))
  {
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

static thread_local std::stack <DPI_AWARENESS_CONTEXT> dpi_ctx_stack;

using GetThreadDpiAwarenessContext_pfn  =
                         DPI_AWARENESS_CONTEXT (WINAPI *)(void);

using GetAwarenessFromDpiAwarenessContext_pfn =
                         DPI_AWARENESS (WINAPI *)(DPI_AWARENESS_CONTEXT);

DPI_AWARENESS_CONTEXT
WINAPI
SK_Display_GetThreadDpiAwarenessContext (void)
{
  assert (SK_IsWindows10OrGreater ());

  static HINSTANCE     user32_dll =
    SK_LoadLibraryW (L"user32.dll");

  static GetThreadDpiAwarenessContext_pfn
         GetThreadDpiAwarenessContextFn = nullptr;

  if (GetThreadDpiAwarenessContextFn == nullptr)
  {   GetThreadDpiAwarenessContextFn =
     (GetThreadDpiAwarenessContext_pfn)
      SK_GetProcAddress ( user32_dll,
      "GetThreadDpiAwarenessContext" );
  }

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

    static HINSTANCE     user32_dll =
      SK_LoadLibraryW (L"user32.dll");

    static GetAwarenessFromDpiAwarenessContext_pfn
           GetAwarenessFromDpiAwarenessContextFn = nullptr;

    if (GetAwarenessFromDpiAwarenessContextFn == nullptr)
    {   GetAwarenessFromDpiAwarenessContextFn =
       (GetAwarenessFromDpiAwarenessContext_pfn)
        SK_GetProcAddress ( user32_dll,
        "GetAwarenessFromDpiAwarenessContext" );
    }

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

  static HINSTANCE     user32_dll =
    SK_LoadLibraryW (L"user32.dll");

  static SetThreadDpiAwarenessContext_pfn
         SetThreadDpiAwarenessContextFn = nullptr;

  if (SetThreadDpiAwarenessContextFn == nullptr)
  {   SetThreadDpiAwarenessContextFn =
     (SetThreadDpiAwarenessContext_pfn)
      SK_GetProcAddress ( user32_dll,
      "SetThreadDpiAwarenessContext" );
  }

  if (SetThreadDpiAwarenessContextFn != nullptr)
    return SetThreadDpiAwarenessContextFn (dpi_ctx);

  return nullptr;
}

static SetProcessDpiAwareness_pfn       SetProcessDpiAwarenessFn       = nullptr;

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
    static HINSTANCE     shcore_dll =
      SK_LoadLibraryW (L"shcore.dll");

    static GetProcessDpiAwareness_pfn
           GetProcessDpiAwarenessFn = nullptr;

    if (GetProcessDpiAwarenessFn == nullptr)
    {
      GetProcessDpiAwarenessFn =
        (GetProcessDpiAwareness_pfn)
      SK_GetProcAddress (shcore_dll,
          "GetProcessDpiAwareness");
    }

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

  dpi_ctx_stack.push (dpi_ctx);
}

void
SK_Display_PopDPIScaling (void)
{
  if (! dpi_ctx_stack.empty ())
  {
    SK_Display_SetThreadDpiAwarenessContext (
      dpi_ctx_stack.top ()
    );

    dpi_ctx_stack.pop ();
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
    static HINSTANCE     shcore_dll =
      SK_LoadLibraryW (L"shcore.dll");

    if (SetProcessDpiAwarenessFn == nullptr)
    {
      SetProcessDpiAwarenessFn =
        (SetProcessDpiAwareness_pfn)
      SK_GetProcAddress (shcore_dll,
          "SetProcessDpiAwareness");
    }

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
SK_Display_DisableDPIScaling (void)
{
  //DWORD   dwProcessSize = MAX_PATH;
  //wchar_t wszProcessName [MAX_PATH + 2] = { };
  //
  //HANDLE hProc =
  // SK_GetCurrentProcess ();
  //
  //QueryFullProcessImageName (
  //  hProc, 0,
  //    wszProcessName, &dwProcessSize
  //);
  //
  //const wchar_t* wszKey        =
  //  LR"(Software\Microsoft\Windows NT\CurrentVersion\AppCompatFlags\Layers)";
  //DWORD          dwDisposition = 0x00;
  //HKEY           hKey          = nullptr;
  //
  //const LSTATUS status =
  //  RegCreateKeyExW ( HKEY_CURRENT_USER,
  //                      wszKey,      0,
  //                        nullptr, 0x0,
  //                        KEY_READ | KEY_WRITE,
  //                           nullptr,
  //                             &hKey,
  //                               &dwDisposition );
  //
  //if ( status == ERROR_SUCCESS &&
  //     hKey   != nullptr          )
  //{
  //  RegSetValueExW (
  //    hKey, wszProcessName,
  //      0, REG_SZ,
  //        (BYTE *)L"~ DPIUNAWARE",
  //           16 * sizeof (wchar_t) );
  //
  //  RegFlushKey (hKey);
  //  RegCloseKey (hKey);
  //}

  if (SK_IsWindows10OrGreater ())
  {
    SK_Display_SetThreadDpiAwarenessContext (
      DPI_AWARENESS_CONTEXT_UNAWARE
    );

    return;
  }

  if (SK_IsWindows8Point1OrGreater ())
  {
    static HINSTANCE     shcore_dll =
      SK_LoadLibraryW (L"shcore.dll");

    if (SetProcessDpiAwarenessFn == nullptr)
    {
      SetProcessDpiAwarenessFn =
        (SetProcessDpiAwareness_pfn)
      SK_GetProcAddress (shcore_dll,
          "SetProcessDpiAwareness");
    }

    if (SetProcessDpiAwarenessFn != nullptr)
    {
      SetProcessDpiAwarenessFn (
        PROCESS_DPI_UNAWARE
      );
    }
  }
}