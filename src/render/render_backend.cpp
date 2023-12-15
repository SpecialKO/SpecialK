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
#include <SpecialK/render/dxgi/dxgi_hdr.h>

#include <SpecialK/nvapi.h>

volatile ULONG64 SK_RenderBackend::frames_drawn = 0ULL;
extern void
SK_Display_EnableHDR (SK_RenderBackend_V2::output_s *pDisplay);

bool SK_Display_IsDPIAwarenessUsingAppCompat (void);
void SK_Display_ForceDPIAwarenessUsingAppCompat (bool set);
void SK_Display_SetMonitorDPIAwareness (bool bOnlyIfWin10);

double
SK_Display_GetDefaultRefreshRate (HMONITOR hMonitor)
{
  static double   dRefresh      = 0.0;
  static DWORD    dwLastChecked = 0;
  static HMONITOR hLastMonitor  = 0;

  if (hLastMonitor != hMonitor || dwLastChecked < SK_timeGetTime () - 250UL)
  {
    static auto& rb =
      SK_GetCurrentRenderBackend ();

    // Refresh this bastard!
    rb.windows.device.getDevCaps ().last_checked = 0;

    dRefresh =
      static_cast <double> (rb.windows.device.getDevCaps ().res.refresh);

    dwLastChecked = SK_timeGetTime ();
    hLastMonitor  = hMonitor;
  }

  return dRefresh;
}

SK_RenderBackend&
__stdcall
SK_GetCurrentRenderBackend (void) noexcept
{
  static SK_RenderBackend __SK_RBkEnd;
  return                  __SK_RBkEnd;
}

void
__stdcall
SK_InitRenderBackends (void)
{
  SK_ICommandProcessor
    *pCommandProcessor = nullptr;

  SK_RunOnce (
    pCommandProcessor =
      SK_Render_InitializeSharedCVars ()
  );

  if (pCommandProcessor != nullptr)
  {
    pCommandProcessor->AddVariable (         "RenderHooks.D3D9Ex",
        SK_CreateVar (SK_IVariable::Boolean, &config.apis.d3d9ex.hook));
    pCommandProcessor->AddVariable (         "RenderHooks.D3D9",
        SK_CreateVar (SK_IVariable::Boolean, &config.apis.d3d9.hook));
    //
    // -?- Originally there were two variables for d3d9.hook ... not sure why
    //
    pCommandProcessor->AddVariable (              "RenderHooks.D3D11",
        SK_CreateVar (SK_IVariable::Boolean, &config.apis.dxgi.d3d11.hook));
    pCommandProcessor->AddVariable (              "RenderHooks.D3D12",
        SK_CreateVar (SK_IVariable::Boolean, &config.apis.dxgi.d3d12.hook));
#ifdef _M_AMD64
    pCommandProcessor->AddVariable (         "RenderHooks.Vulkan",
        SK_CreateVar (SK_IVariable::Boolean, &config.apis.Vulkan.hook));
#else /* _M_IX86 */
    pCommandProcessor->AddVariable (         "RenderHooks.D3D8",
        SK_CreateVar (SK_IVariable::Boolean, &config.apis.d3d8.hook));
    pCommandProcessor->AddVariable (         "RenderHooks.DDraw",
        SK_CreateVar (SK_IVariable::Boolean, &config.apis.ddraw.hook));
#endif
    pCommandProcessor->AddVariable (         "RenderHooks.OpenGL",
        SK_CreateVar (SK_IVariable::Boolean, &config.apis.OpenGL.hook));
  }
}


//////////////////////////////////////////////////////////////////////////
//
//  Direct3D 9
//
//////////////////////////////////////////////////////////////////////////
#define D3D9_TEXTURE_MOD

bool
__stdcall
SK_DXVK_CheckForInterop (void)
{
  HMODULE hModD3D9 =
    SK_GetModuleHandle (L"d3d9.dll");

  if (hModD3D9 == 0)
  {
    wchar_t     wszDllPath [MAX_PATH] = { };
    PathAppend (wszDllPath, SK_GetHostPath ());
    PathAppend (wszDllPath, L"d3d9.dll");

    if (PathFileExistsW (wszDllPath))
    {
      hModD3D9 =
        SK_LoadLibraryW (wszDllPath);
    }
  }

  if (hModD3D9 != 0)
  {
    std::wstring str_d3d9ver =
      SK_GetDLLProductName (
        SK_GetModuleFullName (hModD3D9).c_str ()
                          );

    if (str_d3d9ver.find (L"DXVK") != std::wstring::npos)
    {
      if ( config.apis.d3d9.native_dxvk == SK_NoPreference   &&
             sk::NVAPI::InitializeLibrary (SK_GetHostApp ()) &&
             sk::NVAPI::nv_hardware
         )
      {
        if ( IDYES == 
               SK_MessageBox ( L"Enable native DXVK support?",
                               L"DXVK (D3D9) Detected", MB_YESNO ) )
        {
          config.apis.d3d9.hook                        = false;
          config.apis.d3d9ex.hook                      = false;
          config.display.force_windowed                = true;
          config.apis.d3d9.native_dxvk                 = 1;
          config.apis.NvAPI.vulkan_bridge              = 1;

          SK_SaveConfig ();

          SK_NvAPI_EnableVulkanBridge (TRUE);
          SK_RestartGame              (    );
        }

        else
        {
          config.apis.d3d9.native_dxvk                 = 0;

          SK_SaveConfig ();
        }
      }

      return true;
    }
  }

  HMODULE hModD3D11 =
    SK_GetModuleHandle (L"d3d11.dll");

  if (hModD3D11 == 0)
  {
    wchar_t     wszDllPath [MAX_PATH] = { };
    PathAppend (wszDllPath, SK_GetHostPath ());
    PathAppend (wszDllPath, L"d3d11.dll");

    if (PathFileExistsW (wszDllPath))
    {
      hModD3D9 =
        SK_LoadLibraryW (wszDllPath);
    }
  }

  if (hModD3D11 != 0)
  {
    std::wstring str_d3d11ver =
      SK_GetDLLProductName (
        SK_GetModuleFullName (hModD3D11).c_str ()
                          );

    if (str_d3d11ver.find (L"DXVK") != std::wstring::npos)
    {
      return true;
    }
  }

  return false;
}

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
       pTLS->render->d3d9->ctx_init_thread )
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
    // Offer to enable native Vulkan support
    SK_DXVK_CheckForInterop ();

    if (pTLS != nullptr)
        pTLS->render->d3d9->ctx_init_thread = true;

    SK_D3D9_InitShaderModTools  ();

    SK::D3D9::TextureManager& tex_mgr =
      SK_D3D9_GetTextureManager ();

    if (config.textures.d3d9_mod)
    {
      tex_mgr.Init ();
    }

    dll_log->Log (
      L"[API Detect]  <!> [ Bootstrapping Direct3D 9 (d3d9.dll) ] <!>\t(Initialization tid=%x)",
             SK_GetCurrentThreadId ()
    );

    if (SK_GetDLLRole () == DLL_ROLE::D3D9)
    {
      // Load user-defined DLLs (Early)
      SK_RunLHIfBitness ( 64, SK_LoadEarlyImports64 (),
                              SK_LoadEarlyImports32 () );
    }

    if ( config.textures.d3d9_mod ||
         config.render.d3d9.force_d3d9ex )
    {
      tex_mgr.Hook ();
    }

    SK_HookD3D9    ();

    SK_ApplyQueuedHooks ();

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
    dll_log->Log (
      L"[API Detect]  <!> [ Bootstrapping Direct3D 8 (d3d8.dll) ] <!>\t(Initialization tid=%x)",
             SK_GetCurrentThreadId ()
    );

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
    dll_log->Log (
      L"[API Detect]  <!> [ Bootstrapping DirectDraw (ddraw.dll) ] <!>\t(Initialization tid=%x)",
             SK_GetCurrentThreadId ()
    );

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

  SK_COMPAT_CheckStreamlineSupport ();

  SK_TLS *pTLS =
    SK_TLS_Bottom ();

  if ( pTLS != nullptr &&
       pTLS->render->d3d11->ctx_init_thread )
  {
    return;
  }

  //
  // TEMP HACK: D3D11 must be enabled to hook D3D12...
  //
  if (config.apis.dxgi.d3d12.hook && (! config.apis.dxgi.d3d11.hook))
    config.apis.dxgi.d3d11.hook = true;

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
        pTLS->render->d3d11->ctx_init_thread = true;

    dll_log->Log (
      L"[API Detect]  <!> [    Bootstrapping DXGI (dxgi.dll)    ] <!>\t(Initialization tid=%x)",
             SK_GetCurrentThreadId ()
    );

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
  if (SK_GetCurrentGameID () == SK_GAME_ID::yuzu)
  {
    static bool          once = false;
    if (! std::exchange (once, true))
      return;
  }

  if (SK_GetDLLRole () == DLL_ROLE::OpenGL && (! config.compatibility.init_on_separate_thread))
  {
    config.compatibility.init_on_separate_thread = true;
    return;
  }

  SK_TLS *pTLS =
    SK_TLS_Bottom ();

  if ( pTLS != nullptr &&
       pTLS->render->gl->ctx_init_thread )
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
        pTLS->render->gl->ctx_init_thread = true;

    dll_log->Log (
      L"[API Detect]  <!> [ Bootstrapping OpenGL (OpenGL32.dll) ] <!>\t(Initialization tid=%x)",
             SK_GetCurrentThreadId ()
    );




    if (SK_GetDLLRole () == DLL_ROLE::OpenGL)
    {
      // Load user-defined DLLs (Early)
      SK_RunLHIfBitness ( 64, SK_LoadEarlyImports64 (),
                              SK_LoadEarlyImports32 () );
    }

    SK_Thread_CreateEx (SK_HookGL, L"[SK] OpenGL Hook Crawler");

    InterlockedIncrementRelease (&__booted);
  }
#endif

  SK_Thread_SpinUntilAtomicMin (&__booted, 2);
}


#include "vulkan/vulkan.h"
#include "vulkan/vulkan_win32.h"

PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR_Original = nullptr;

VkResult
SK_VK_CreateSwapchainKHR (
            VkDevice                   device,
      const VkSwapchainCreateInfoKHR*  pCreateInfo,
      const VkAllocationCallbacks*     pAllocator,
    VkSwapchainKHR*                    pSwapchain )
{
  VkSurfaceFullScreenExclusiveInfoEXT
    fse_info                     = { };
    fse_info.sType               = VK_STRUCTURE_TYPE_SURFACE_FULL_SCREEN_EXCLUSIVE_INFO_EXT;
    fse_info.fullScreenExclusive = VK_FULL_SCREEN_EXCLUSIVE_DISALLOWED_EXT;

  const void *pNext =     pCreateInfo->pNext;
  auto _CreateInfoCopy = *pCreateInfo;

         fse_info.pNext = (void *)pNext;
  _CreateInfoCopy.pNext =     &fse_info;

  SK_LOG0 (
    ( L"Disabling Fullscreen Exclusive" ), L" VulkanIK "
  );

  return
    vkCreateSwapchainKHR_Original (device, &_CreateInfoCopy, pAllocator, pSwapchain);
}

void
_SK_HookVulkan (void)
{
  if (! config.apis.Vulkan.hook)
    return;

  static volatile LONG hooked = FALSE;

  if (SK_LoadLibraryW (L"vulkan-1.dll") != nullptr)
  {
    if (! InterlockedCompareExchangeAcquire (&hooked, TRUE, FALSE))
    {
      config.render.gl.disable_fullscreen = true;

      //
      // DXGI / VK Interop Setup
      //
          SK_CreateDLLHook (L"vulkan-1.dll",
                             "vkCreateSwapchainKHR",
                          SK_VK_CreateSwapchainKHR,
     static_cast_p2p <void> (&vkCreateSwapchainKHR_Original));
    }
  }
}

void
SK_BootVulkan (void)
{
  static volatile ULONG __booted = FALSE;

  if (InterlockedCompareExchange (&__booted, TRUE, FALSE))
    return;

  // Establish the minimal set of APIs necessary to work as vulkan-1.dll
  if (SK_GetDLLRole () == DLL_ROLE::Vulkan)
    config.apis.Vulkan.hook = true;

  if (! config.apis.Vulkan.hook)
    return;

  _SK_HookVulkan ();
}

SK_RenderBackend_V2::output_s*
SK_RenderBackend_V2::output_s::nvapi_ctx_s::getDisplayFromId (NvU32 display_id) noexcept
{
  static auto& rb =
    SK_GetCurrentRenderBackend ();

  const auto active_display =
    rb.active_display;

  // Try the active display first, most of the time it's the one we're looking for.
  if (      gsl::at (rb.displays, active_display).nvapi.display_id == display_id)
    return &gsl::at (rb.displays, active_display);

  // Exhaustive search otherwise
  for ( auto& display : rb.displays )
    if (      display.nvapi.display_id == display_id)
      return &display;

  // Uh-Oh! Spaghetti O's
  return nullptr;
}

SK_RenderBackend_V2::output_s*
SK_RenderBackend_V2::output_s::nvapi_ctx_s::getDisplayFromHandle (NvDisplayHandle display_handle) noexcept
{
  static auto& rb =
    SK_GetCurrentRenderBackend ();

  const auto active_display =
    rb.active_display;

  // Try the active display first, most of the time it's the one we're looking for.
  if (      gsl::at (rb.displays, active_display).nvapi.display_handle == display_handle)
    return &gsl::at (rb.displays, active_display);

  // Exhaustive search otherwise
  for ( auto& display : rb.displays )
    if (      display.nvapi.display_handle == display_handle)
      return &display;

  // Uh-Oh! Spaghetti O's
  return nullptr;
}

void
SK_RenderBackend_V2::gsync_s::update (bool force)
{
  static auto& rb =
    SK_GetCurrentRenderBackend ();

  auto& display =
    rb.displays [rb.active_display];

  auto _EvaluateAutoLowLatency = [&]()
  {
    // Opt-in to Auto-Low Latency the first time this is seen
    if (capable && active && config.render.framerate.auto_low_latency.waiting &&
                             config.render.framerate.present_interval != 0)
    {
      config.nvidia.reflex.enable                 = true;
      config.nvidia.reflex.low_latency            = true;
      config.render.framerate.sync_interval_clamp = 1; // Prevent games from F'ing VRR up.
      config.render.framerate.auto_low_latency.
                                          waiting = false;
      config.render.framerate.auto_low_latency.
                                        triggered = true;
      // ^^^ Now turn auto-low latency off, so the user can select their own setting if they want

      // Use the Low-Latency Limiter mode, even though it might cause stutter.
      if (config.render.framerate.auto_low_latency.policy.ultra_low_latency)
      {
        config.nvidia.reflex.low_latency_boost     = true;
        config.nvidia.reflex.marker_optimization   = true;
        config.render.framerate.enforcement_policy = 2;
      }
      else
      {
        // No need to turn this off, just turn off latency marker optimization
      //config.nvidia.reflex.low_latency_boost     = false;
        config.nvidia.reflex.marker_optimization   = false;
        config.render.framerate.enforcement_policy = 4;
      }
    
      const double dRefreshRate =
        static_cast <double> (display.signal.timing.vsync_freq.Numerator) /
        static_cast <double> (display.signal.timing.vsync_freq.Denominator);
    
      const double dVRROptimalFPS =
        dRefreshRate - (dRefreshRate * dRefreshRate) / (60.0 * 60.0) - 0.1;
    
      extern float __target_fps;
      if (__target_fps == 0.0f || __target_fps > dVRROptimalFPS)
      {
        //SK_ImGui_WarningWithTitle (
        //  SK_FormatStringW (L"Framerate Limit Set to %.2f FPS For Optimal VRR", dVRROptimalFPS).c_str (),
        //                    L"Auto Low-Latency (VRR) Mode Activated"
        //);
    
        config.render.framerate.target_fps = static_cast <float> (dVRROptimalFPS);
        __target_fps                       = static_cast <float> (dVRROptimalFPS);
      }
    }
  };

  SK_RunOnce (disabled.for_app = (! SK_NvAPI_GetVRREnablement ()));

  //
  // All non-D3D9 or D3D11 APIs
  //

  if (rb.api != SK_RenderAPI::D3D11  &&
      rb.api != SK_RenderAPI::D3D9Ex &&
      rb.api != SK_RenderAPI::D3D9)
  {
    maybe_active = false;

    if (! disabled.for_app)
    {
      static HANDLE hVRRThread =
      SK_Thread_CreateEx ([](LPVOID)->
      DWORD
      {
        SK_Thread_SetCurrentPriority (THREAD_PRIORITY_BELOW_NORMAL);

        do
        {
          auto& display =
            rb.displays [rb.active_display];

          NV_GET_VRR_INFO vrr_info            = {       NV_GET_VRR_INFO_VER };
          display.nvapi.monitor_caps.version  = NV_MONITOR_CAPABILITIES_VER;
          display.nvapi.monitor_caps.infoType = NV_MONITOR_CAPS_TYPE_GENERIC;

          SK_NvAPI_Disp_GetVRRInfo             (display.nvapi.display_id, &vrr_info);
          SK_NvAPI_DISP_GetMonitorCapabilities (display.nvapi.display_id,
                                               &display.nvapi.monitor_caps);

          display.nvapi.vrr_enabled =
            vrr_info.bIsVRREnabled;

          rb.gsync_state.capable = display.nvapi.vrr_enabled;
          rb.gsync_state.active  = false;

          if (rb.gsync_state.capable)
          {
            if (rb.presentation.mode == SK_PresentMode::Hardware_Composed_Independent_Flip   ||
                rb.presentation.mode == SK_PresentMode::Hardware_Independent_Flip            ||
                rb.presentation.mode == SK_PresentMode::Hardware_Legacy_Copy_To_Front_Buffer ||
                rb.presentation.mode == SK_PresentMode::Hardware_Legacy_Flip)
            {
              rb.gsync_state.active =
                (rb.present_interval < 2);
            }
            else
            {
              if (rb.presentation.mode == SK_PresentMode::Composed_Flip         ||
                  rb.presentation.mode == SK_PresentMode::Composed_Copy_GPU_GDI ||
                  rb.presentation.mode == SK_PresentMode::Composed_Copy_CPU_GDI)
              {
                rb.gsync_state.active = false;
              }

              if (rb.presentation.mode == SK_PresentMode::Unknown)
                rb.gsync_state.maybe_active = true;
            }
          }

          else
          {
            auto &monitor_caps =
              display.nvapi.monitor_caps;

            rb.gsync_state.capable =
              monitor_caps.data.caps.supportVRR &&
              monitor_caps.data.caps.currentlyCapableOfVRR;
          }
        } while ( WAIT_OBJECT_0 !=
                  WaitForSingleObject (__SK_DLL_TeardownEvent, 750UL) );

        SK_Thread_CloseSelf ();

        return 0;
      }, L"[SK] VRR Status Monitor");
    }

    return
      _EvaluateAutoLowLatency ();
  }


  //
  // D3D9 or D3D11
  //

  // DO NOT hold onto this. NVAPI does not explain how NVDX handles work, but
  //   we can generally assume their lifetime is only as long as the D3D resource
  //     they identify.
  auto _ClearTemporarySurfaces = [&]()->
  void
  {
    rb.surface.dxgi  = nullptr;
    rb.surface.d3d9  = nullptr;
    rb.surface.nvapi = nullptr;

    _EvaluateAutoLowLatency ();
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

    if ( rb.device        == nullptr  ||
         rb.swapchain     == nullptr  ||
        (rb.surface.d3d9  == nullptr  &&
         rb.surface.dxgi  == nullptr) ||
         rb.surface.nvapi == nullptr)
    {
      return
        _ClearTemporarySurfaces ();
    }

    if ( NVAPI_OK == SK_NvAPI_D3D_IsGSyncCapable ( rb.device,
                                                   rb.surface.nvapi, &capable))
    {
      SK_NvAPI_D3D_IsGSyncActive ( rb.device,      rb.surface.nvapi, &active);

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
    if (swapchain != nullptr)
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
              sk::narrow_cast <UINT> (config.render.framerate.refresh_rate);
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


double
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

    SK_HDR_ReleaseResources       ();
    SK_DXGI_ReleaseSRGBLinearizer ();

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

    if (adapter.d3dkmt != 0)
    {
      D3DKMT_CLOSEADAPTER
             close          = {            };
             close.hAdapter = adapter.d3dkmt;

      SK_D3DKMT_CloseAdapter (&close);

      adapter.d3dkmt = 0;
    }
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
SK_API
IUnknown*
__stdcall
SK_Render_GetDevice (void)
{
  return
    SK_GetCurrentRenderBackend ().device;
}

// Does NOT implicitly AddRef, do NOT hold a reference to this!
SK_API
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
  HWND hWndRoot =
    GetAncestor (hWnd, GA_ROOT);

  if (focus.hwnd == nullptr || ( SK_GetFocus () == hWndRoot && hWndRoot != nullptr && GetActiveWindow () == hWndRoot ) )
  {
    focus.update        (hWndRoot);
    game_window.hWnd   = hWndRoot;
    game_window.top    = GetTopWindow (hWndRoot);
    game_window.active = true;

    SK_LOG1 (( __FUNCTIONW__ L" (%p)", hWnd ), L"  DEBUG!  ");
  }

  else
  {
    SK_LOG1 (( __FUNCTIONW__ L" (%p) --- FAIL [%s %s]", hWnd,
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
    SK_LOG1 ( (L"Treating focus HWND as device HWND because device HWND was invalid."),
               L"Window Mgr");
    setDevice (hWnd);
  }
}

void
SK_RenderBackend_V2::window_registry_s::setDevice (HWND hWnd)
{
  ////if (game_window.hWnd == 0 || (! IsWindow (game_window.hWnd)))
  ////{   game_window.hWnd = hWnd; }

  device.update      (hWnd);
  game_window.child = hWnd;

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

        DISPLAYCONFIG_PATH_INFO *pOutput  = nullptr;
        bool                     bVirtual = false;

        for (auto idx = 0U; idx < uiNumPaths; ++idx)
        {
          auto *path =
            &pathArray [idx];

          if ( (path->                 flags & DISPLAYCONFIG_PATH_ACTIVE)
            && (path->sourceInfo.statusFlags & DISPLAYCONFIG_SOURCE_IN_USE) )
          {
            bVirtual =
              (path->flags & DISPLAYCONFIG_PATH_SUPPORT_VIRTUAL_MODE);

            DISPLAYCONFIG_SOURCE_MODE *pSourceMode =
              &modeArray [bVirtual ? path->sourceInfo.sourceModeInfoIdx
                                   : path->sourceInfo.modeInfoIdx ].sourceMode;

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
          bVirtual =
            (pOutput->flags & DISPLAYCONFIG_PATH_SUPPORT_VIRTUAL_MODE);

          devcaps.res.refresh =
            (double)pOutput->targetInfo.refreshRate.Numerator /
            (double)pOutput->targetInfo.refreshRate.Denominator;

          auto modeIdx =
            bVirtual ? pOutput->sourceInfo.sourceModeInfoIdx
                     : pOutput->sourceInfo.modeInfoIdx;

          devcaps.res.x = modeArray [modeIdx].sourceMode.width;
          devcaps.res.y = modeArray [modeIdx].sourceMode.height;

          devcaps.last_checked = dwNow;

          return devcaps;
        }
      }
    }

    HDC hDC =
      GetWindowDC (hwnd);

    if (hDC != 0)
    {
      devcaps.res.x       =          GetDeviceCaps (hDC,  HORZRES);
      devcaps.res.y       =          GetDeviceCaps (hDC,  VERTRES);
      devcaps.res.refresh = (double) GetDeviceCaps (hDC, VREFRESH);

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

uint32_t
SK_Render_GetVulkanInteropSwapChainType (IUnknown *swapchain)
{
  uint32_t  bVkInterop     = 0;
  UINT     uiVkInteropSize = 4;

  SK_ComQIPtr <IDXGISwapChain>
                   pSwapChain (swapchain);

  if ( pSwapChain.p != nullptr && SUCCEEDED (
       pSwapChain->GetPrivateData ( SKID_DXGI_VK_InteropSwapChain,
                                           &uiVkInteropSize,
                                            &bVkInterop ) )
     )
  {
    return bVkInterop;
  }

  return 0;
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
        // Establish the API used this frame (and handle possible translation layers)
        //
        switch (SK_GetDLLRole ())
        {
          case DLL_ROLE::D3D8:
            api = SK_RenderAPI::D3D8On12;
            wcscpy (name, L"D3D8");
            break;
          case DLL_ROLE::DDraw:
            api = SK_RenderAPI::DDrawOn12;
            wcscpy (name, L"DDraw");
            break;
          default:
            api = SK_RenderAPI::D3D12;
            wcsncpy (name, L"D3D12 ", 8);
            break;
        }

        // Handle AMD interop
        //
        auto uiVkLayerType =
          SK_Render_GetVulkanInteropSwapChainType (swapchain);

        if (uiVkLayerType != SK_DXGI_VK_INTEROP_TYPE_NONE)
        {
          switch (uiVkLayerType)
          {
            case SK_DXGI_VK_INTEROP_TYPE_IK:
              wcsncpy (name, L"Vulkan-IK", 10);
              break;
            case SK_DXGI_VK_INTEROP_TYPE_NV:
            case SK_DXGI_VK_INTEROP_TYPE_AMD:
              wcsncpy (name, L"Vulkan", 8);
              break;
            default:
              wcsncpy (name, L"Interop??", 10);
              break;
          }
        }
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
            if (! SK_GL_OnD3D11)
              api = SK_RenderAPI::D3D11;
            break;
        }

        if (api == SK_RenderAPI::D3D11)
        {
          if (SK_GL_OnD3D11)
          {
            wcsncpy (name, L"OpenGL-IK", 10);
          }

          else
          {
            auto uiVkLayerType =
              SK_Render_GetVulkanInteropSwapChainType (swapchain);

            if (uiVkLayerType != SK_DXGI_VK_INTEROP_TYPE_NONE)
            {
              switch (uiVkLayerType)
              {
                case SK_DXGI_VK_INTEROP_TYPE_IK:
                  wcsncpy (name, L"Vulkan-IK", 10);
                  break;
                case SK_DXGI_VK_INTEROP_TYPE_NV:
                case SK_DXGI_VK_INTEROP_TYPE_AMD:
                  wcsncpy (name, L"Vulkan", 8);
                  break;
                default:
                  wcsncpy (name, L"Interop??", 10);
                  break;
              }
            }

            else
            {
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
            }
          }
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

#ifdef _DISPLAY_GL_VERSION
      static int major = 0,
                 minor = 0;

      SK_RunOnce (glGetIntegerv (GL_MAJOR_VERSION, &major));
      SK_RunOnce (glGetIntegerv (GL_MINOR_VERSION, &minor));

      if (major > 0) wsnprintf (name, 8,  L"GL %d.%d", major, minor);
      else
#endif
      wcsncpy (name, L"OpenGL", 8);
    }
  }
}





sk_hwnd_cache_s::sk_hwnd_cache_s (HWND wnd)
{
  update (wnd);
}

bool sk_hwnd_cache_s::update (HWND wnd)
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
    parent  = GetAncestor      (hwnd, GA_PARENT);

    if (*title != L'\0' || *class_name != L'\0')
      last_changed = SK_GetFramesDrawn ();

    return true;
  }

  return false;
}

ULONG64
__stdcall
SK_GetFramesDrawn_NonInline (void)
{
  return SK_GetFramesDrawn ();
}


const char*
HDRModeToStr (NV_HDR_MODE mode)
{
  switch (mode)
  {
    case NV_HDR_MODE_OFF:              return "Off";
    case NV_HDR_MODE_UHDA:             return "HDR10";
    case NV_HDR_MODE_EDR:              return "Extended Dynamic Range";
    case NV_HDR_MODE_SDR:              return "Standard Dynamic Range";
    case NV_HDR_MODE_DOLBY_VISION:     return "Dolby Vision";
    case NV_HDR_MODE_UHDA_PASSTHROUGH: return "HDR10 Pass through";
    case NV_HDR_MODE_UHDA_NB:          return "Notebook HDR";
    default:                           return "Invalid";
  };
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
      //SK_D3D_UnpackShaderCompiler ();
    }
  }
}

HMODULE
SK_D3D_GetShaderCompiler (void)
{
  static HMODULE hModCompiler =
    SK_LoadLibraryW (L"D3DCompiler_47.dll");

  if (hModCompiler != nullptr)
    return hModCompiler;

  static bool once = false;
  if (std::exchange (once, true) == false)
  {
    for ( auto wszDLLName : { L"D3DCompiler_46.dll", L"D3DCompiler_45.dll",
                              L"D3DCompiler_44.dll", L"D3DCompiler_43.dll",
                              L"D3DCompiler_42.dll", L"D3DCompiler_41.dll",
                              L"D3DCompiler_40.dll" } )
    {
      hModCompiler =
        SK_LoadLibraryW (wszDLLName);

      if (hModCompiler != nullptr)
        break;
    }
  }

  return hModCompiler;
}

HRESULT
WINAPI
SK_D3D_Disassemble (_In_reads_bytes_(SrcDataSize) LPCVOID    pSrcData,
                    _In_                          SIZE_T     SrcDataSize,
                    _In_                          UINT       Flags,
                    _In_opt_                      LPCSTR     szComments,
                    _Out_                         ID3DBlob** ppDisassembly)
{
  using D3DDisassemble_pfn = HRESULT (WINAPI *)(LPCVOID,SIZE_T,UINT,LPCSTR,ID3DBlob**);

  static D3DDisassemble_pfn
        _D3DDisassemble =
        (D3DDisassemble_pfn)SK_GetProcAddress (SK_D3D_GetShaderCompiler (),
        "D3DDisassemble");

  if (_D3DDisassemble != nullptr)
    return _D3DDisassemble ( pSrcData, SrcDataSize, Flags,
                              szComments, ppDisassembly );

  return E_NOTIMPL;
}

HRESULT
WINAPI
SK_D3D_Reflect (_In_reads_bytes_(SrcDataSize) LPCVOID pSrcData,
                _In_                          SIZE_T  SrcDataSize,
                _In_                          REFIID  pInterface,
                _Out_                         void**  ppReflector)
{
  using D3DReflect_pfn = HRESULT (WINAPI *)(LPCVOID,SIZE_T,REFIID,void**);

  static D3DReflect_pfn
        _D3DReflect =
        (D3DReflect_pfn)SK_GetProcAddress (SK_D3D_GetShaderCompiler (),
        "D3DReflect");

  if (_D3DReflect != nullptr)
    return _D3DReflect (pSrcData, SrcDataSize, pInterface, ppReflector);

  return E_NOTIMPL;
}

HRESULT
WINAPI
SK_D3D_Compile (
  _In_reads_bytes_(SrcDataSize)           LPCVOID           pSrcData,
  _In_                                    SIZE_T            SrcDataSize,
  _In_opt_                                LPCSTR            pSourceName,
  _In_reads_opt_(_Inexpressible_(pDefines->Name != NULL))
                                    CONST D3D_SHADER_MACRO* pDefines,
  _In_opt_                                ID3DInclude*      pInclude,
  _In_opt_                                LPCSTR            pEntrypoint,
  _In_                                    LPCSTR            pTarget,
  _In_                                    UINT              Flags1,
  _In_                                    UINT              Flags2,
  _Out_                                   ID3DBlob**        ppCode,
  _Always_(_Outptr_opt_result_maybenull_) ID3DBlob**        ppErrorMsgs)
{
  using D3DCompile_pfn = HRESULT (WINAPI *)(LPCVOID,SIZE_T,LPCSTR,CONST D3D_SHADER_MACRO*,
                                            ID3DInclude*,LPCSTR,LPCSTR,UINT,UINT,
                                            ID3DBlob**,ID3DBlob**);

  static D3DCompile_pfn
        _D3DCompile =
        (D3DCompile_pfn)SK_GetProcAddress (SK_D3D_GetShaderCompiler (),
        "D3DCompile");

  if (_D3DCompile != nullptr)
    return _D3DCompile ( pSrcData, SrcDataSize, pSourceName,
                           pDefines, pInclude, pEntrypoint,
                             pTarget, Flags1, Flags2, ppCode,
                               ppErrorMsgs );

  return E_NOTIMPL;
}

#undef  __SK_SUBSYSTEM__
#define __SK_SUBSYSTEM__ L"RenderBase"

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
#ifndef DPI_AWARENESS_CONTEXT_UNAWARE
#define DPI_AWARENESS_CONTEXT_UNAWARE              ((DPI_AWARENESS_CONTEXT)-1)
#endif
#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((DPI_AWARENESS_CONTEXT)-4)
#endif
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

bool SK_Display_IsDPIAwarenessUsingAppCompat (void)
{
  bool    bDPIAppCompat = false;

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
                          KEY_READ,
                             nullptr,
                               &hKey,
                                 &dwDisposition );

  if ( status == ERROR_SUCCESS &&
       hKey   != nullptr          )
  {
    wchar_t             wszKeyVal [2048] = { };
    DWORD len = sizeof (wszKeyVal) / 2;

    RegGetValueW (
      hKey, nullptr, wszProcessName, RRF_RT_REG_SZ,
            nullptr,  wszKeyVal, &len
    );

    bDPIAppCompat =
      ( StrStrIW (wszKeyVal, L"HIGHDPIAWARE") != nullptr );

    RegCloseKey (hKey);
  }

  return
    bDPIAppCompat;
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

      if (wszOrigKeyVal [0] != L'\0')
      {
        RegSetValueExW (
          hKey, wszProcessName,
            0, REG_SZ,
              (BYTE *)wszOrigKeyVal,
                (DWORD)(( wcslen (wszOrigKeyVal) + 1 ) * sizeof (wchar_t))
        );
      }
      else
      {
        RegDeleteValueW (hKey, wszProcessName);
        RegCloseKey     (hKey);
        return;
      }
    }

    else if (set && pwszHIGHDPIAWARE == nullptr)
    {
      StrCatW  (wszOrigKeyVal, L" HIGHDPIAWARE");
      StrTrimW (wszOrigKeyVal, L" ");

      RegSetValueExW (
        hKey, wszProcessName,
          0, REG_SZ,
            (BYTE *)wszOrigKeyVal,
              (DWORD)(( wcslen (wszOrigKeyVal) + 1 ) * sizeof (wchar_t))
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
      DPI_AWARENESS_CONTEXT_UNAWARE
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
  if (! IsProcessDPIAware ())
  {
    bool bWasAppCompatAware =
      SK_Display_IsDPIAwarenessUsingAppCompat ();

    // Persistently disable DPI scaling problems so that initialization order doesn't matter
    SK_Display_ForceDPIAwarenessUsingAppCompat (true);
    SK_Display_SetMonitorDPIAwareness          (false);

    if ((! bWasAppCompatAware) && SK_Display_IsDPIAwarenessUsingAppCompat ())
    {
      SK_RestartGame (nullptr, L"A one-time game restart is required to fix DPI Scaling issues in this game.");
    }
  }


  if (SK_IsWindows10OrGreater ())
  {
    SK_Display_SetThreadDpiAwarenessContext (
      DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
    );

    //return;
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
        PROCESS_PER_MONITOR_DPI_AWARE
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
        if (pDevice11.p != nullptr)// && (! SK_GL_OnD3D11))
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

      if (config.apis.d3d9.hook)
      {
        SK_ComQIPtr <IDirect3DDevice9> pDevice9 (pDevice);
        if (pDevice9.p != nullptr)
        {
          device = pDevice;
          api    = SK_RenderAPI::D3D9;
        }

        SK_ComQIPtr <IDirect3DDevice9Ex> pDevice9Ex (pDevice);
        if (pDevice9Ex.p != nullptr)
        {
          device = pDevice;
          api    = SK_RenderAPI::D3D9Ex;
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

const char*
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

static constexpr uint8_t MONITOR_NAME    = 0xfc;

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
    if (config.system.log_level > 0)
    {
      SK_RunOnce (dll_log->Log (L"SK_EDID_Parse (...): Checksum fail"));
      //return "";
    }
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
    if (config.system.log_level > 0)
    {
      SK_RunOnce (dll_log->Log (L"SK_EDID_Parse (...): Checksum fail"));
      //return { };
    }
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

    swprintf (display.name, L"UNKNOWN");

    bool nvSuppliedEDID = false;

    // This is known to return EDIDs with checksums that don't match expected,
    //   there's not much benefit to getting EDID this way, so use the registry instead.
#if 1
    if (sk::NVAPI::nv_hardware != false)
    {
      NvPhysicalGpuHandle nvGpuHandles [NVAPI_MAX_PHYSICAL_GPUS] = {     };
      NvU32               nvGpuCount                             =       0;
      NvDisplayHandle     nvDisplayHandle                        = nullptr;
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

        if (devInfo != nullptr)
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
        if (config.system.log_level > 0)
        {
          if (SK_GetCurrentRenderBackend ().display_crc [display.idx] == 0)
          {
            dll_log->Log ( L"[Output Dev] DeviceName: %ws, devIdx: %lu, DeviceID: %ws",
                             disp_desc.DeviceName, devIdx,
                               disp_desc.DeviceID );
          }
        }

        swprintf ( display.name, edid_name.empty () ?
                                    LR"(%ws (%ws))" :
                                      L"%hs",
                     edid_name.empty ()       ?
                       disp_desc.DeviceString :
      (const WCHAR *)edid_name.c_str (),
                         disp_desc.DeviceName );
      }
    }
  }
}

void
SK_WDDM_CAPS::init (D3DKMT_HANDLE hAdapter)
{
  D3DKMT_QUERYADAPTERINFO
         queryAdapterInfo                       = { };
         queryAdapterInfo.AdapterHandle         = hAdapter;
         queryAdapterInfo.Type                  = KMTQAITYPE_DRIVERVERSION;
         queryAdapterInfo.PrivateDriverData     = &version;
         queryAdapterInfo.PrivateDriverDataSize = sizeof (D3DKMT_DRIVERVERSION);
  
         queryAdapterInfo.Type                  = KMTQAITYPE_WDDM_3_0_CAPS;
         queryAdapterInfo.PrivateDriverData     = &_3_0;
         queryAdapterInfo.PrivateDriverDataSize = sizeof (D3DKMT_WDDM_3_0_CAPS);
  
  SK_D3DKMT_QueryAdapterInfo (&queryAdapterInfo);
  
         queryAdapterInfo.Type                  = KMTQAITYPE_WDDM_2_9_CAPS;
         queryAdapterInfo.PrivateDriverData     = &_2_9;
         queryAdapterInfo.PrivateDriverDataSize = sizeof (D3DKMT_WDDM_2_9_CAPS);
  
  SK_D3DKMT_QueryAdapterInfo (&queryAdapterInfo);
  
         queryAdapterInfo.Type                  = KMTQAITYPE_WDDM_2_7_CAPS;
         queryAdapterInfo.PrivateDriverData     = &_2_7;
         queryAdapterInfo.PrivateDriverDataSize = sizeof (D3DKMT_WDDM_2_7_CAPS);
  
  SK_D3DKMT_QueryAdapterInfo (&queryAdapterInfo);

  // For Windows 10, just fill-in WDDM 2.9 values using what's available.
  if (version < KMT_DRIVERVERSION_WDDM_2_9)
  {
    _2_9.HwSchEnabled      = _2_7.HwSchEnabled;
    _2_9.HwSchSupportState = _2_7.HwSchSupported ? DXGK_FEATURE_SUPPORT_STABLE
                                                 : DXGK_FEATURE_SUPPORT_EXPERIMENTAL;
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
SK_RenderBackend_V2::updateWDDMCaps (SK_RenderBackend_V2::output_s *pDisplay)
{
  if (D3DKMTOpenAdapterFromGdiDisplayName == nullptr)
      D3DKMTOpenAdapterFromGdiDisplayName = SK_GetProcAddress (L"gdi32.dll",
     "D3DKMTOpenAdapterFromGdiDisplayName");

  if (D3DKMTOpenAdapterFromGdiDisplayName != nullptr)
  {
    if (adapter.d3dkmt != 0)
    {
      D3DKMT_CLOSEADAPTER
        closeAdapter;
        closeAdapter.hAdapter = adapter.d3dkmt;

      if (SUCCEEDED (SK_D3DKMT_CloseAdapter (&closeAdapter)))
      {
        SK_LOG1 ( ( L"SK_D3DKMT_CloseAdapter successful for %ws", pDisplay->gdi_name ),
                      __SK_SUBSYSTEM__ );

        adapter.d3dkmt        = 0;
        adapter.VidPnSourceId = 0;
        adapter.luid.HighPart = 0;
        adapter.luid.LowPart  = 0;
      }
    }

    D3DKMT_OPENADAPTERFROMGDIDISPLAYNAME
      openAdapter = { };

    wcsncpy_s ( openAdapter.DeviceName, 32,
                    pDisplay->gdi_name, _TRUNCATE );

    if ( STATUS_SUCCESS ==
           ((D3DKMTOpenAdapterFromGdiDisplayName_pfn)
             D3DKMTOpenAdapterFromGdiDisplayName)(&openAdapter) )
    {
      SK_LOG1 ( (L"D3DKMTOpenAdapterFromGdiDisplayName successful for %ws, VidPnSourceId=%lu",
                 pDisplay->gdi_name, openAdapter.VidPnSourceId ),
                 __SK_SUBSYSTEM__ );

      adapter.d3dkmt        = openAdapter.hAdapter;
      adapter.VidPnSourceId = openAdapter.VidPnSourceId;
      adapter.luid.HighPart = openAdapter.AdapterLuid.HighPart;
      adapter.luid.LowPart  = openAdapter.AdapterLuid.LowPart;

    //SK_ReleaseAssert (adapter.luid == openAdapter.AdapterLuid);
    }

    else
    {
      SK_LOG0 ( (L"D3DKMTOpenAdapterFromGdiDisplayName unsuccessful for %ws", pDisplay->gdi_name ),
                 __SK_SUBSYSTEM__ );

      adapter.d3dkmt        = 0;
      adapter.VidPnSourceId = 0;
      adapter.luid.HighPart = 0;
      adapter.luid.LowPart  = 0;
    }

    if (adapter.d3dkmt != 0)
    {
      if (D3DKMTGetMultiPlaneOverlayCaps == nullptr)
          D3DKMTGetMultiPlaneOverlayCaps = SK_GetProcAddress (L"gdi32.dll",
         "D3DKMTGetMultiPlaneOverlayCaps");

      pDisplay->mpo_planes = 0;

      if (D3DKMTGetMultiPlaneOverlayCaps != nullptr)
      {
        D3DKMT_GET_MULTIPLANE_OVERLAY_CAPS caps = {};

        caps.hAdapter      = openAdapter.hAdapter;
        caps.VidPnSourceId = adapter.VidPnSourceId;

        if ( ((D3DKMTGetMultiPlaneOverlayCaps_pfn)
               D3DKMTGetMultiPlaneOverlayCaps)(&caps) == STATUS_SUCCESS )
        {
          pDisplay->mpo_planes = caps.MaxRGBPlanes; // Don't care about YUV planes, this is a game!

          if (pDisplay == &displays [active_display] && config.display.warn_no_mpo_planes && pDisplay->mpo_planes <= 1)
          {
            SK_RunOnce (
              SK_ImGui_Warning (L"MPOs are not active, consider restarting your driver.")
            );
          }
        }
      }

      pDisplay->wddm_caps.init (adapter.d3dkmt);
    }
  }
}

bool
SK_RenderBackend_V2::routeAudioForDisplay (SK_RenderBackend_V2::output_s *pDisplay, bool force_update)
{
  bool routed = false;

  if (! pDisplay)
    return routed;

  if (_wcsicmp (pDisplay->audio.paired_device, L"System Default"))
  {
    if (_wcsicmp (pDisplay->audio.paired_device, L"No Preference"))
    {
      routed =
        SK_WASAPI_EndPointMgr->setPersistedDefaultAudioEndpoint (
          GetCurrentProcessId (), eRender, pDisplay->audio.paired_device, force_update
        );
    }
  }
  
  else
  {
    routed =
      SK_WASAPI_EndPointMgr->setPersistedDefaultAudioEndpoint (
        GetCurrentProcessId (), eRender, L"", force_update
      );
  }

  return routed;
}

bool
SK_RenderBackend_V2::assignOutputFromHWND (HWND hWndContainer)
{
  RECT                              rectOutputWindow = { };
  SK_GetWindowRect (hWndContainer, &rectOutputWindow);

  auto pContainer =
    getContainingOutput (rectOutputWindow);

  output_s* pOutput = nullptr;

////for ( auto& display : displays )
////{
////  if ( pContainer          != nullptr &&
////       pContainer->monitor == display.monitor )
////  {
////    pOutput = &display;
////    break;
////  }
////}

  // Fallback to whatever has been assigned to rb.monitor
  if (pOutput == nullptr)
  {
    for ( auto& display : displays )
    {
      if (display.attached     &&
          display.monitor != 0 &&
          display.monitor == monitor)
      {
        pOutput = &display;
        break;
      }
    }
  }

  if (pOutput != nullptr)
  {
    auto& display = *pOutput;

    updateWDDMCaps (pOutput);

    wcsncpy_s ( display_name,        128,
                display.name, _TRUNCATE );

    // Late init
    if (monitor != display.monitor)
        monitor  = display.monitor;

    active_display = static_cast <int> (
      ((intptr_t)&display -
       (intptr_t)&displays [0]) /
sizeof (output_s));

    routeAudioForDisplay (pOutput);

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
      DXGI_OUTPUT_DESC1
        uncachedOutDesc;
        uncachedOutDesc.BitsPerColor = pContainer->bpc;
        uncachedOutDesc.ColorSpace   = pContainer->colorspace;

      SK_DXGI_UpdateColorSpace (pSwap3.p, &uncachedOutDesc);

      if (config.render.dxgi.temporary_dwm_hdr)
      {
        SK_RunOnce (SK_Display_EnableHDR (pOutput));
      }

      //
      // Reload the MaxLuminance setting from hdr.ini so that it overrides
      //   whatever values we just got from DXGI
      //
      extern void
      SK_HDR_UpdateMaxLuminanceForActiveDisplay (bool forced = false);
      SK_HDR_UpdateMaxLuminanceForActiveDisplay (true);

      if ((! isHDRCapable ()) && ( __SK_HDR_16BitSwap ||
                                   __SK_HDR_10BitSwap ))
      {
        SK_Display_EnableHDR (pOutput);

        //assignOutputFromHWND (hWndContainer);

        //if (! (display.hdr.supported && display.hdr.enabled))
        //{
        //  SK_RunOnce (
        //    SK_ImGui_WarningWithTitle (
        //      L"ERROR: Special K HDR Applied to a non-HDR Display\r\n\r\n"
        //      L"\t\t>> Please Disable SK HDR or set the Windows Desktop to use HDR",
        //                               L"HDR is Unsupported by the Active Display" )
        //  );
        //}
        //
        //else
        //{
        //  updateOutputTopology ();
        //
        //  return true;
        //}
      }
    }

    return true;
  }

  return false;
}

const char*
SK_RenderBackend_V2::output_s::signal_info_s::timing_s::video_standard_s::toStr (void)
{
  static std::unordered_map
    < D3DKMDT_VIDEO_SIGNAL_STANDARD, const char * >
  standard_names =
  {
    { D3DKMDT_VSS_UNINITIALIZED, "D3DKMDT_VSS_UNINITIALIZED" },
    { D3DKMDT_VSS_VESA_DMT,      "D3DKMDT_VSS_VESA_DMT"      },
    { D3DKMDT_VSS_VESA_GTF,      "D3DKMDT_VSS_VESA_GTF"      },
    { D3DKMDT_VSS_VESA_CVT,      "D3DKMDT_VSS_VESA_CVT"      },
    { D3DKMDT_VSS_IBM,           "D3DKMDT_VSS_IBM"           },
    { D3DKMDT_VSS_APPLE,         "D3DKMDT_VSS_APPLE"         },
    { D3DKMDT_VSS_NTSC_M,        "D3DKMDT_VSS_NTSC_M"        },
    { D3DKMDT_VSS_NTSC_J,        "D3DKMDT_VSS_NTSC_J"        },
    { D3DKMDT_VSS_NTSC_443,      "D3DKMDT_VSS_NTSC_443"      },
    { D3DKMDT_VSS_PAL_B,         "D3DKMDT_VSS_PAL_B"         },
    { D3DKMDT_VSS_PAL_B1,        "D3DKMDT_VSS_PAL_B1"        },
    { D3DKMDT_VSS_PAL_G,         "D3DKMDT_VSS_PAL_G"         },
    { D3DKMDT_VSS_PAL_H,         "D3DKMDT_VSS_PAL_H"         },
    { D3DKMDT_VSS_PAL_I,         "D3DKMDT_VSS_PAL_I"         },
    { D3DKMDT_VSS_PAL_D,         "D3DKMDT_VSS_PAL_D"         },
    { D3DKMDT_VSS_PAL_N,         "D3DKMDT_VSS_PAL_N"         },
    { D3DKMDT_VSS_PAL_NC,        "D3DKMDT_VSS_PAL_NC"        },
    { D3DKMDT_VSS_SECAM_B,       "D3DKMDT_VSS_SECAM_B"       },
    { D3DKMDT_VSS_SECAM_D,       "D3DKMDT_VSS_SECAM_D"       },
    { D3DKMDT_VSS_SECAM_G,       "D3DKMDT_VSS_SECAM_G"       },
    { D3DKMDT_VSS_SECAM_H,       "D3DKMDT_VSS_SECAM_H"       },
    { D3DKMDT_VSS_SECAM_K,       "D3DKMDT_VSS_SECAM_K"       },
    { D3DKMDT_VSS_SECAM_K1,      "D3DKMDT_VSS_SECAM_K1"      },
    { D3DKMDT_VSS_SECAM_L,       "D3DKMDT_VSS_SECAM_L"       },
    { D3DKMDT_VSS_SECAM_L1,      "D3DKMDT_VSS_SECAM_L1"      },
    { D3DKMDT_VSS_EIA_861,       "D3DKMDT_VSS_EIA_861"       },
    { D3DKMDT_VSS_EIA_861A,      "D3DKMDT_VSS_EIA_861A"      },
    { D3DKMDT_VSS_EIA_861B,      "D3DKMDT_VSS_EIA_861B"      },
    { D3DKMDT_VSS_PAL_K,         "D3DKMDT_VSS_PAL_K"         },
    { D3DKMDT_VSS_PAL_K1,        "D3DKMDT_VSS_PAL_K1"        },
    { D3DKMDT_VSS_PAL_L,         "D3DKMDT_VSS_PAL_L"         },
    { D3DKMDT_VSS_PAL_M,         "D3DKMDT_VSS_PAL_M"         },
    { D3DKMDT_VSS_OTHER,         "D3DKMDT_VSS_OTHER"         }
  };

  if (standard_names.count ((D3DKMDT_VIDEO_SIGNAL_STANDARD)videoStandard)) return
      standard_names [      (D3DKMDT_VIDEO_SIGNAL_STANDARD)videoStandard];

  else
    return "N/A";
}

void SK_Display_EnableHDR (SK_RenderBackend_V2::output_s *pOutput = nullptr)
{
  auto& rb =
    SK_GetCurrentRenderBackend ();

  if (pOutput == nullptr)
      pOutput = &rb.displays [rb.active_display];

  if (    pOutput->hdr.supported)
  { if (! pOutput->hdr.enabled)
    {
      DISPLAYCONFIG_SET_ADVANCED_COLOR_STATE
        setHdrState                     = { };
        setHdrState.header.type         = DISPLAYCONFIG_DEVICE_INFO_SET_ADVANCED_COLOR_STATE;
        setHdrState.header.size         =     sizeof (DISPLAYCONFIG_SET_ADVANCED_COLOR_STATE);
        setHdrState.header.adapterId    = pOutput->vidpn.targetInfo.adapterId;
        setHdrState.header.id           = pOutput->vidpn.targetInfo.id;

        setHdrState.enableAdvancedColor = true;

      if ( ERROR_SUCCESS == DisplayConfigSetDeviceInfo ( (DISPLAYCONFIG_DEVICE_INFO_HEADER *)&setHdrState ) )
      {
        pOutput->hdr.enabled = setHdrState.enableAdvancedColor;

        if (pOutput->hdr.enabled)
          rb.hdr_enabled_displays.emplace (pOutput);
        else
          rb.hdr_enabled_displays.erase   (pOutput);
      }
    }
  }
}

void SK_Display_DisableHDR (SK_RenderBackend_V2::output_s *pOutput = nullptr)
{
  auto& rb =
    SK_GetCurrentRenderBackend ();

   if (pOutput == nullptr)
      pOutput = &rb.displays [rb.active_display];

  if (  pOutput->hdr.supported)
  { if (pOutput->hdr.enabled)
    {
      DISPLAYCONFIG_SET_ADVANCED_COLOR_STATE
        setHdrState                     = { };
        setHdrState.header.type         = DISPLAYCONFIG_DEVICE_INFO_SET_ADVANCED_COLOR_STATE;
        setHdrState.header.size         =     sizeof (DISPLAYCONFIG_SET_ADVANCED_COLOR_STATE);
        setHdrState.header.adapterId    = pOutput->vidpn.targetInfo.adapterId;
        setHdrState.header.id           = pOutput->vidpn.targetInfo.id;

        setHdrState.enableAdvancedColor = false;

      if ( ERROR_SUCCESS == DisplayConfigSetDeviceInfo ( (DISPLAYCONFIG_DEVICE_INFO_HEADER *)&setHdrState ) )
      {
        pOutput->hdr.enabled = setHdrState.enableAdvancedColor;

        if (pOutput->hdr.enabled)
          rb.hdr_enabled_displays.emplace (pOutput);
        else
          rb.hdr_enabled_displays.erase   (pOutput);
      }
    }
  }
}

void
SK_RenderBackend_V2::updateOutputTopology (void)
{
  // This needs to be limited to once per-frame
  //
  static volatile   ULONG64 ulLastUpdate = 0LL;
  if (InterlockedExchange (&ulLastUpdate, SK_GetFramesDrawn ())
                                       == SK_GetFramesDrawn ()) return;

  update_outputs = false;

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
        else
          // Will happen if we need to do this during INI load
          ((CreateDXGIFactory1_pfn)SK_GetProcAddress (L"dxgi.dll", "CreateDXGIFactory1"))(IID_IDXGIFactory1, (void **)&factory.p);
      }

      SK_ComPtr <IDXGIAdapter>
                 pTempAdapter;

      if (factory.p != nullptr)
      {
        if (SUCCEEDED (factory->EnumAdapters (0, &pTempAdapter.p)))
        {
          pAdapter = pTempAdapter;
        }
      }

      return
        ( pAdapter.p != nullptr );
    }


    SK_ComPtr <IDXGIFactory1> pFactory1;

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
      adapter.luid =
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

      adapter.luid =
        adapterDesc.AdapterLuid;
    }

    SK_ComQIPtr <IDXGIFactory4>
        pFactory4   (pFactory1);
    if (pFactory4 != nullptr)
    {
      pAdapter.Release ();

      if ( FAILED (
             pFactory4->EnumAdapterByLuid ( adapter.luid,
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

        if (adapterDesc.AdapterLuid.HighPart == adapter.luid.HighPart &&
            adapterDesc.AdapterLuid.LowPart  == adapter.luid.LowPart)
        {
          pAdapter = pNewAdapter;
          break;
        }
      }
    }

    return
      ( pAdapter.p != nullptr );
  };

  if (! _GetAdapter (swapchain.p, pAdapter))
  {
    // Try without the SwapChain (FF7 Remake workaround)
    if (! _GetAdapter (nullptr, pAdapter))
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

//for ( auto& display : displays )
//{
//  RtlSecureZeroMemory (
//    &display, display_size
//  );
//}

  UINT                             idx = 0;
  SK_ComPtr <IDXGIOutput>                pOutput;

  while ( DXGI_ERROR_NOT_FOUND !=
            pAdapter->EnumOutputs (idx, &pOutput.p) )
  {
    if (pOutput.p != nullptr)
    {
      auto& display =
        displays [idx++];

      DXGI_OUTPUT_DESC                  outDesc  = { };
      if (SUCCEEDED (pOutput->GetDesc (&outDesc)))
      {
        MONITORINFOEXW
        mi        = {         };
        mi.cbSize = sizeof (mi);

        if (GetMonitorInfoW (outDesc.Monitor, &mi))
        {
          swscanf (StrStrIW (mi.szDevice, LR"(\DISPLAY)"), LR"(\DISPLAY%u)", &display.idx);
        }

        display.monitor     = outDesc.Monitor;
        display.rect        = outDesc.DesktopCoordinates;
        display.attached    = outDesc.AttachedToDesktop;

        if (sk::NVAPI::nv_hardware != false)
        {
          NvPhysicalGpuHandle nvGpuHandles [NVAPI_MAX_PHYSICAL_GPUS] = { };
          NvU32               nvGpuCount                             =   0;
          NvDisplayHandle     nvDisplayHandle                        =   0;
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

            NV_GET_VRR_INFO         vrr_info     = { NV_GET_VRR_INFO_VER         };
            NV_MONITOR_CAPABILITIES monitor_caps = { NV_MONITOR_CAPABILITIES_VER };
                                    monitor_caps.
                                        infoType =   NV_MONITOR_CAPS_TYPE_GENERIC;

            SK_NvAPI_Disp_GetVRRInfo             (nvDisplayId, &vrr_info);
            SK_NvAPI_DISP_GetMonitorCapabilities (nvDisplayId, &monitor_caps);

            display.nvapi.monitor_caps = monitor_caps;
            display.nvapi.vrr_enabled  = vrr_info.bIsVRREnabled;
          }
        }

        wcsncpy_s ( display.gdi_name,  32,
                    outDesc.DeviceName, _TRUNCATE );

        SK_RBkEnd_UpdateMonitorName (display, outDesc);

        SK_ComQIPtr <IDXGIOutput6>
            pOutput6 (pOutput);
        if (pOutput6.p != nullptr)
        {
          DXGI_OUTPUT_DESC1                   outDesc1  = { };
          if (SUCCEEDED (pOutput6->GetDesc1 (&outDesc1)))
          {
            if (outDesc1.MinLuminance > outDesc1.MaxFullFrameLuminance)
            {
              std::swap (outDesc1.MinLuminance, outDesc1.MaxFullFrameLuminance);

              static bool once = false;

              if (! std::exchange (once, true))
              {
                SK_LOG0 ( (L" --- Working around DXGI bug, swapping invalid min / avg luminance levels"),
                           L"   DXGI   "
                        );
              }
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

      pOutput.Release ();
    }
  }

  UINT        enum_count = idx;
  static UINT last_count = idx;

  for ( auto& disp : displays )
  {
    // Clear out any old entries that might be wrong now
    disp.primary = false;
  }

  auto& display_audio_ini =
    dll_ini->get_section (L"Display.Audio");

  for ( idx = 0 ; idx < enum_count ; ++idx )
  {
    auto& display =
      displays [idx];

    std::wstring key_name =
      SK_FormatStringW (L"RenderDevice.%ws", display.path_name);

    if (display_audio_ini.contains_key (key_name))
    {
      wcsncpy (display.audio.paired_device, display_audio_ini.get_value (key_name).c_str (), 127);
    }

    MONITORINFOEXW
    minfo        = {            };
    minfo.cbSize = sizeof (minfo);

    if (! GetMonitorInfoW (display.monitor, &minfo))
      continue;

    display.primary =
      ( minfo.dwFlags & MONITORINFOF_PRIMARY );

    float bestIntersectArea = -1.0f;

    int ax1 = minfo.rcMonitor.left,
        ax2 = minfo.rcMonitor.right;
    int ay1 = minfo.rcMonitor.top,
        ay2 = minfo.rcMonitor.bottom;

    DISPLAYCONFIG_PATH_INFO *pVidPn = nullptr;

    bool bVirtual = false;

    for (UINT32 pathIdx = 0; pathIdx < uiNumPaths; ++pathIdx)
    {
      auto *path =
        &pathArray [pathIdx];

      bVirtual =
        ( path->flags & DISPLAYCONFIG_PATH_SUPPORT_VIRTUAL_MODE );

      if ( (path->                 flags & DISPLAYCONFIG_PATH_ACTIVE)
        && (path->sourceInfo.statusFlags & DISPLAYCONFIG_SOURCE_IN_USE) )
      {
        DISPLAYCONFIG_SOURCE_MODE *pSourceMode =
          &modeArray [ bVirtual ? path->sourceInfo.sourceModeInfoIdx :
                                  path->sourceInfo.modeInfoIdx ].sourceMode;

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
      bVirtual =
        ( pVidPn->flags & DISPLAYCONFIG_PATH_SUPPORT_VIRTUAL_MODE );

      int modeIdx =
        ( bVirtual ? pVidPn->targetInfo.targetModeInfoIdx
                   : pVidPn->targetInfo.modeInfoIdx );

      auto pModeInfo =
        &modeArray [modeIdx];

      display.vidpn = *pVidPn;

      if (pModeInfo->infoType == DISPLAYCONFIG_MODE_INFO_TYPE_TARGET)
      {
        auto orig_signal =
          display.signal;

        display.signal.timing.pixel_clock            =
          pModeInfo->targetMode.targetVideoSignalInfo.pixelRate;
        display.signal.timing.hsync_freq.Numerator   =
          pModeInfo->targetMode.targetVideoSignalInfo.hSyncFreq.Numerator;
        display.signal.timing.hsync_freq.Denominator =
          pModeInfo->targetMode.targetVideoSignalInfo.hSyncFreq.Denominator;
        display.signal.timing.vsync_freq.Numerator   =
          pModeInfo->targetMode.targetVideoSignalInfo.vSyncFreq.Numerator;
        display.signal.timing.vsync_freq.Denominator =
          pModeInfo->targetMode.targetVideoSignalInfo.vSyncFreq.Denominator;
        display.signal.timing.active_size.cx         =
          pModeInfo->targetMode.targetVideoSignalInfo.activeSize.cx;
        display.signal.timing.active_size.cy         =
          pModeInfo->targetMode.targetVideoSignalInfo.activeSize.cy;
        display.signal.timing.total_size.cx          =
          pModeInfo->targetMode.targetVideoSignalInfo.totalSize.cx;
        display.signal.timing.total_size.cy          =
          pModeInfo->targetMode.targetVideoSignalInfo.totalSize.cy;
        display.signal.timing.videoStandard.
                                       videoStandard = (UINT32)
          pModeInfo->targetMode.targetVideoSignalInfo.videoStandard;

        if ( 0 != std::memcmp ( &orig_signal,
                             &display.signal,
            sizeof (decltype (display.signal))) )
        {
          char szVSyncFreq [16] = { },
               szHSyncFreq [16] = { };

          std::string_view str_view_vsync (szVSyncFreq, 16),
                           str_view_hsync (szHSyncFreq, 16);

          SK_FormatStringView ( str_view_vsync, "%7.3f",
                              static_cast <double> (display.signal.timing.vsync_freq.Numerator) /
                              static_cast <double> (display.signal.timing.vsync_freq.Denominator) ),
          SK_FormatStringView ( str_view_hsync, "%7.3f",
                              static_cast <double> (display.signal.timing.hsync_freq.Numerator) /
                              static_cast <double> (display.signal.timing.hsync_freq.Denominator) / 1000.0 );

          SK_RemoveTrailingDecimalZeros (szVSyncFreq, 16);
          SK_RemoveTrailingDecimalZeros (szHSyncFreq, 16);

          SK_LOG0 (
             (L" ( %20s ) :: PixelClock=%6.1f MHz, vSyncFreq=%7hs Hz, hSyncFreq=%7hs kHz, activeSize=(%lux%lu), totalSize=(%lux%lu), Standard=%hs",
                                    display.name,
              static_cast <double> (display.signal.timing.pixel_clock) / 1000000.0,
                                                             szVSyncFreq, szHSyncFreq,
                                    display.signal.timing.active_size.cx, display.signal.timing.active_size.cy,
                                    display.signal.timing.total_size.cx,  display.signal.timing.total_size.cy,
                                    display.signal.timing.videoStandard.toStr ()), __SK_SUBSYSTEM__ );
        }
      }

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

      // Don't need this info unless HDR is active
      if (display.hdr.enabled)
      {
        DISPLAYCONFIG_SDR_WHITE_LEVEL
          getSdrWhiteLevel                  = { };
          getSdrWhiteLevel.header.type      = DISPLAYCONFIG_DEVICE_INFO_GET_SDR_WHITE_LEVEL;
          getSdrWhiteLevel.header.size      = sizeof         (DISPLAYCONFIG_SDR_WHITE_LEVEL);
          getSdrWhiteLevel.header.adapterId = display.vidpn.targetInfo.adapterId;
          getSdrWhiteLevel.header.id        = display.vidpn.targetInfo.id;

        if ( ERROR_SUCCESS == DisplayConfigGetDeviceInfo ( (DISPLAYCONFIG_DEVICE_INFO_HEADER *)&getSdrWhiteLevel ) )
        {
          display.hdr.white_level =
            (static_cast <float> (getSdrWhiteLevel.SDRWhiteLevel) / 1000.0f) * 80.0f;
        }

        else
          display.hdr.white_level = 80.0f;
      }

      else
        display.hdr.white_level = 80.0f;

      // Name and preferred modes are immutable, so we can skip this
      if (*display.path_name == L'\0' || last_count != enum_count)
      {                               // That only applies if the number of monitors did not change
        // Clear any cached names
        *display.name = L'\0';

        DISPLAYCONFIG_TARGET_PREFERRED_MODE
          getPreferredMode                  = { };
          getPreferredMode.header.type      = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_PREFERRED_MODE;
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
          switch (getTargetName.outputTechnology)
          {
            case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_OTHER:
              StrCpyA (display.signal.type, "Other");                 break;
            case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_HD15:
              StrCpyA (display.signal.type, "HD15 (VGA)");            break;
            case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_SVIDEO:
              StrCpyA (display.signal.type, "S-Video");               break;
            case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_COMPOSITE_VIDEO:
              StrCpyA (display.signal.type, "Composite Video");       break;
            case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_COMPONENT_VIDEO:
              StrCpyA (display.signal.type, "Component Video");       break;
            case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_DVI:
              StrCpyA (display.signal.type, "DVI");                   break;
            case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_HDMI:
              StrCpyA (display.signal.type, "HDMI");                  break;
            case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_LVDS:
              StrCpyA (display.signal.type, "LVDS");                  break;
            case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_D_JPN:
              StrCpyA (display.signal.type, "Japanese D");            break;
            case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_SDI:
              StrCpyA (display.signal.type, "SDI");                   break;
            case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_DISPLAYPORT_EXTERNAL:
              StrCpyA (display.signal.type, "Display Port");          break;
            case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_DISPLAYPORT_EMBEDDED:
              StrCpyA (display.signal.type, "Embedded Display Port"); break;
            case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_UDI_EXTERNAL:
              StrCpyA (display.signal.type, "UDI");                   break;
            case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_UDI_EMBEDDED:
              StrCpyA (display.signal.type, "Embedded UDI");          break;
            case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_SDTVDONGLE:
              StrCpyA (display.signal.type, "SDTV Dongle");           break;
            case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_MIRACAST:
              StrCpyA (display.signal.type, "Miracast Wireless");     break;
            case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_INDIRECT_WIRED:
              StrCpyA (display.signal.type, "Indirect Wired");        break;
            case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_INDIRECT_VIRTUAL:
              StrCpyA (display.signal.type, "Indirect Virtual");      break;
            default:
              StrCpyA (display.signal.type, "UNKNOWN");               break;
          };

          display.signal.connector_idx =
            getTargetName.connectorInstance;

          wcsncpy_s ( display.name,                                   64,
                      getTargetName.monitorFriendlyDeviceName, _TRUNCATE );


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
                swprintf (display.name, L"%hs", edid_name.c_str ());
              }
            }
          }

          sprintf ( display.full_name,
                      "%ws -<+>- %s %u", display.name,
                                         display.signal.type,
                                         display.signal.connector_idx );

          wcsncpy_s ( display.path_name,               128,
                      getTargetName.monitorDevicePath, _TRUNCATE );
        }
      }
    }

    auto old_crc =
     display_crc [idx];


    updateWDDMCaps (&display);


    //
    // Do not include this in the hash, because "supportULMB" weirdly
    //   varies every time the caps are queried...
    //
    auto            monitor_caps_data =
      display.nvapi.monitor_caps.data;
      display.nvapi.monitor_caps.data = {};

     display_crc [idx] =
             crc32c ( 0, &display,
                          display_size );

    display_changed [idx] =
      old_crc != display_crc [idx];

    display.nvapi.monitor_caps.data =
                  monitor_caps_data;

    last_count = enum_count;

    if (display_changed [idx])
    {
      const bool bIsActiveDisplay =
        (display.idx == displays [active_display].idx);

      if (bIsActiveDisplay)
      {
        extern void SK_HDR_UpdateMaxLuminanceForActiveDisplay (bool forced = false);
                    SK_HDR_UpdateMaxLuminanceForActiveDisplay (true);
      }

      dll_log->LogEx ( true,
        L"[Output Dev]\n"
        L"  +------------------+---------------------------------------------------------------------\n"
        L"  | EDID Device Name |  %hs\n"
        L"  | GDI  Device Name |  %ws (HMONITOR: %06p)\n"
        L"  | Desktop Display. |  %ws%ws\n"
        L"  | Bits Per Color.. |  %u\n"
        L"  | Color Space..... |  %hs\n"
        L"  | Red Primary..... |  %f, %f\n"
        L"  | Green Primary... |  %f, %f\n"
        L"  | Blue Primary.... |  %f, %f\n"
        L"  | White Point..... |  %f, %f\n"
        L"  | SDR White Level. |  %10.5f cd/m²\n"
        L"  | Min Luminance... |  %10.5f cd/m²   %ws\n"
        L"  | Max Luminance... |  %10.5f cd/m²   %ws\n"
        L"  | Max FullFrame... |  %10.5f cd/m²   %ws\n"
        L"  +------------------+---------------------------------------------------------------------\n",
          display.full_name,
          display.gdi_name, display.monitor,
          display.attached ? L"Yes"                : L"No",
          display.primary  ? L" (Primary Display)" : L"",
                      display.bpc,
          DXGIColorSpaceToStr (display.colorspace),
          display.gamut.xr,    display.gamut.yr,
          display.gamut.xg,    display.gamut.yg,
          display.gamut.xb,    display.gamut.yb,
          display.gamut.Xw,    display.gamut.Yw,
          display.hdr.white_level,
          display.gamut.minY,
          display.gamut.minY != display_gamut.minY               && display_gamut.minY > 0.0f        && bIsActiveDisplay
           ? SK_FormatStringW (L"(Profile Override: %10.5f cd/m²)", display_gamut.minY).c_str ()
           :                   L"",
          display.gamut.maxY,
          display.gamut.maxY != display_gamut.maxLocalY          && display_gamut.maxLocalY > 0.0f   && bIsActiveDisplay
           ? SK_FormatStringW (L"(Profile Override: %10.5f cd/m²)", display_gamut.maxLocalY).c_str ()
           :                   L"",
          display.gamut.maxAverageY,
          display.gamut.maxAverageY != display_gamut.maxAverageY && display_gamut.maxAverageY > 0.0f && bIsActiveDisplay
           ? SK_FormatStringW (L"(Profile Override: %10.5f cd/m²)", display_gamut.maxAverageY).c_str ()
           :                   L""
      );
    }
  }

  const bool any_changed =
    std::count ( std::begin (display_changed),
                 std::end   (display_changed), true ) > 0;

  if (any_changed)
  {
    for ( UINT i = 0 ; i < enum_count; ++i )
    {
      SK_ComQIPtr <IDXGISwapChain>
          pChain (swapchain.p);
      if (pChain.p != nullptr)
      {
        DXGI_SWAP_CHAIN_DESC  swapDesc = { };
        pChain->GetDesc     (&swapDesc);
        assignOutputFromHWND (swapDesc.OutputWindow);
      }

      else
        assignOutputFromHWND (game_window.hWnd);

      SK_LOG0 ( ( L"%s Monitor %i: [ %ix%i | (%5i,%#5i) ] %16ws :: %s",
                    displays [i].primary ? L"*" : L" ",
                    displays [i].idx,
                    displays [i].rect.right  - displays [i].rect.left,
                    displays [i].rect.bottom - displays [i].rect.top,
                    displays [i].rect.left,    displays [i].rect.top,
                    displays [i].name,
                    displays [i].hdr.enabled ? L"HDR" : L"SDR" ),
                  L"   DXGI   " );
    }

    SK_Display_ResolutionSelectUI (true);
  }

  else
  {
    stale_display_info = false;
  }
}

const SK_RenderBackend_V2::output_s*
SK_RenderBackend_V2::getContainingOutput (const RECT& rkRect)
{
  const output_s* pOutput = nullptr;

  float bestIntersectArea = -1.0f;

  int ax1 = rkRect.left,
      ax2 = rkRect.right;
  int ay1 = rkRect.top,
      ay2 = rkRect.bottom;

  for ( auto& it : displays )
  {
    if (it.attached && (! IsRectEmpty (&it.rect)))
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

  return pOutput;
}

/* size of a form name string */
#define CCHFORMNAME 32

using EnumDisplaySettingsA_pfn = BOOL (WINAPI *) (
  _In_opt_ LPCSTR    lpszDeviceName,
  _In_     DWORD      iModeNum,
  _Inout_  DEVMODEA *lpDevMode
  );
EnumDisplaySettingsA_pfn EnumDisplaySettingsA_Original = nullptr;

using EnumDisplaySettingsW_pfn = BOOL (WINAPI *) (
  _In_opt_ LPWSTR    lpszDeviceName,
  _In_     DWORD      iModeNum,
  _Inout_  DEVMODEW *lpDevMode
  );
EnumDisplaySettingsW_pfn EnumDisplaySettingsW_Original = nullptr;


// SAL notation in Win32 API docs is wrong
using ChangeDisplaySettingsA_pfn = LONG (WINAPI *)(
  _In_opt_ DEVMODEA *lpDevMode,
  _In_     DWORD     dwFlags
  );
ChangeDisplaySettingsA_pfn ChangeDisplaySettingsA_Original = nullptr;

// SAL notation in Win32 API docs is wrong
using ChangeDisplaySettingsW_pfn = LONG (WINAPI *)(
  _In_opt_ DEVMODEW *lpDevMode,
  _In_     DWORD     dwFlags
  );
ChangeDisplaySettingsW_pfn ChangeDisplaySettingsW_Original = nullptr;

using ChangeDisplaySettingsExA_pfn = LONG (WINAPI *)(
  _In_ LPCSTR    lpszDeviceName,
  _In_ DEVMODEA *lpDevMode,
       HWND      hwnd,
  _In_ DWORD     dwflags,
  _In_ LPVOID    lParam
  );
ChangeDisplaySettingsExA_pfn ChangeDisplaySettingsExA_Original = nullptr;

using ChangeDisplaySettingsExW_pfn = LONG (WINAPI *)(
  _In_ LPCWSTR   lpszDeviceName,
  _In_ DEVMODEW *lpDevMode,
       HWND      hwnd,
  _In_ DWORD     dwflags,
  _In_ LPVOID    lParam
  );
ChangeDisplaySettingsExW_pfn ChangeDisplaySettingsExW_Original = nullptr;

void SK_GL_SetVirtualDisplayMode (HWND hWnd, bool Fullscreen, UINT Width, UINT Height);

LONG
WINAPI
ChangeDisplaySettingsExA_Detour (
  _In_ LPCSTR    lpszDeviceName,
  _In_ DEVMODEA *lpDevMode,
       HWND      hWnd,
  _In_ DWORD     dwFlags,
  _In_ LPVOID    lParam )
{
  SK_LOG_FIRST_CALL

  if (config.display.force_windowed)
    return DISP_CHANGE_SUCCESSFUL;


  // NOP this sucker, we have borderless flip model in GL!
  if (config.render.gl.disable_fullscreen && config.apis.dxgi.d3d11.hook)
  {
    if ( lpDevMode == nullptr || ( (lpDevMode->dmFields & DM_PELSWIDTH) &&
                                   (lpDevMode->dmFields & DM_PELSHEIGHT) ) )
    {
      UINT Width  = lpDevMode != nullptr ? lpDevMode->dmPelsWidth  : 0;
      UINT Height = lpDevMode != nullptr ? lpDevMode->dmPelsHeight : 0;

      SK_GL_SetVirtualDisplayMode (hWnd, (dwFlags & CDS_UPDATEREGISTRY) || (dwFlags & CDS_FULLSCREEN), Width, Height);
    }

    if (SK_GL_OnD3D11)
      return DISP_CHANGE_SUCCESSFUL;
  }

  static bool called = false;

  DEVMODEA dev_mode        = { };
           dev_mode.dmSize = sizeof (DEVMODEW);

  EnumDisplaySettingsA_Original (lpszDeviceName, 0, &dev_mode);

  if (! config.window.res.override.isZero ())
  {
    if (lpDevMode != nullptr)
    {
      lpDevMode->dmPelsWidth  = config.window.res.override.x;
      lpDevMode->dmPelsHeight = config.window.res.override.y;
    }
  }

  EnumDisplaySettingsA_Original (lpszDeviceName, 0, &dev_mode);

  if (dwFlags != CDS_TEST)
  {
    if (called)
      ChangeDisplaySettingsExA_Original (lpszDeviceName, lpDevMode, hWnd, CDS_RESET, lParam);

    called = true;

    return
      ChangeDisplaySettingsExA_Original (lpszDeviceName, lpDevMode, hWnd, CDS_FULLSCREEN, lParam);
  }

  else
  {
    return
      ChangeDisplaySettingsExA_Original (lpszDeviceName, lpDevMode, hWnd, dwFlags, lParam);
  }
}


LONG
WINAPI
ChangeDisplaySettingsA_Detour (
  _In_opt_ DEVMODEA *lpDevMode,
  _In_     DWORD     dwFlags )
{
  SK_LOG_FIRST_CALL

  return
    ChangeDisplaySettingsExA_Detour (nullptr, lpDevMode, nullptr, dwFlags, nullptr);
}

LONG
__stdcall
SK_ChangeDisplaySettings (DEVMODEW *lpDevMode, DWORD dwFlags)
{
  return
    ChangeDisplaySettingsW_Original != nullptr           ?
    ChangeDisplaySettingsW_Original (lpDevMode, dwFlags) :
    ChangeDisplaySettingsW          (lpDevMode, dwFlags);
}

LONG
WINAPI
ChangeDisplaySettingsExW_Detour (
  _In_ LPWSTR    lpszDeviceName,
  _In_ DEVMODEW *lpDevMode,
       HWND      hWnd,
  _In_ DWORD     dwFlags,
  _In_ LPVOID    lParam)
{
  SK_LOG_FIRST_CALL


  if (config.display.force_windowed)
    return DISP_CHANGE_SUCCESSFUL;


  // NOP this sucker, we have borderless flip model in GL!
  if (config.render.gl.disable_fullscreen && config.apis.dxgi.d3d11.hook)
  {
    if ( lpDevMode == nullptr || ( (lpDevMode->dmFields & DM_PELSWIDTH) &&
                                   (lpDevMode->dmFields & DM_PELSHEIGHT) ) )
    {
      UINT Width  = lpDevMode != nullptr ? lpDevMode->dmPelsWidth  : 0;
      UINT Height = lpDevMode != nullptr ? lpDevMode->dmPelsHeight : 0;

      SK_GL_SetVirtualDisplayMode (hWnd, (dwFlags & CDS_UPDATEREGISTRY) || (dwFlags & CDS_FULLSCREEN), Width, Height);
    }

    if (SK_GL_OnD3D11)
      return DISP_CHANGE_SUCCESSFUL;
  }

  DEVMODEW dev_mode        = { };
           dev_mode.dmSize = sizeof (DEVMODEW);

  EnumDisplaySettingsW_Original (lpszDeviceName, 0, &dev_mode);

////return
////  ChangeDisplaySettingsExW_Original (lpszDeviceName, lpDevMode, hWnd, dwFlags, lParam);

  static bool called = false;

  if (! config.window.res.override.isZero ())
  {
    if (lpDevMode != nullptr)
    {
      lpDevMode->dmPelsWidth  = config.window.res.override.x;
      lpDevMode->dmPelsHeight = config.window.res.override.y;
    }
  }

  EnumDisplaySettingsW_Original (lpszDeviceName, 0, &dev_mode);

  if (dwFlags != CDS_TEST)
  {
    if (called)
      ChangeDisplaySettingsExW_Original (lpszDeviceName, lpDevMode, hWnd, CDS_RESET, lParam);

    called = true;

    return
      ChangeDisplaySettingsExW_Original (lpszDeviceName, lpDevMode, hWnd, CDS_FULLSCREEN, lParam);
  }

  else
  {
    return
      ChangeDisplaySettingsExW_Original (lpszDeviceName, lpDevMode, hWnd, dwFlags, lParam);
  }
}

LONG
__stdcall
SK_ChangeDisplaySettingsEx ( _In_ LPCWSTR   lpszDeviceName,
                             _In_ DEVMODEW *lpDevMode,
                                  HWND      hWnd,
                             _In_ DWORD     dwFlags,
                             _In_ LPVOID    lParam )
{
  return
    ChangeDisplaySettingsExW_Original != nullptr           ?
    ChangeDisplaySettingsExW_Original (lpszDeviceName, lpDevMode, hWnd, dwFlags, lParam) :
    ChangeDisplaySettingsExW          (lpszDeviceName, lpDevMode, hWnd, dwFlags, lParam);
}

LONG
WINAPI
ChangeDisplaySettingsW_Detour (
  _In_opt_ DEVMODEW *lpDevMode,
  _In_     DWORD     dwFlags )
{
  SK_LOG_FIRST_CALL

  return
    ChangeDisplaySettingsExW_Detour (nullptr, lpDevMode, nullptr, dwFlags, nullptr);
}

void
SK_Display_HookModeChangeAPIs (void)
{
  static volatile LONG               hooked      = FALSE;
  if (! InterlockedCompareExchange (&hooked, TRUE, FALSE))
  {
    auto hModUser32 =
      SK_GetModuleHandle (L"user32");

    SK_CreateDLLHook2 (      L"user32",
                              "ChangeDisplaySettingsA",
                               ChangeDisplaySettingsA_Detour,
      static_cast_p2p <void> (&ChangeDisplaySettingsA_Original) );
    SK_CreateDLLHook2 (      L"user32",
                              "ChangeDisplaySettingsW",
                               ChangeDisplaySettingsW_Detour,
      static_cast_p2p <void> (&ChangeDisplaySettingsW_Original) );
    SK_CreateDLLHook2 (       L"user32",
                              "ChangeDisplaySettingsExA",
                               ChangeDisplaySettingsExA_Detour,
      static_cast_p2p <void> (&ChangeDisplaySettingsExA_Original) );
    SK_CreateDLLHook2 (       L"user32",
                              "ChangeDisplaySettingsExW",
                               ChangeDisplaySettingsExW_Detour,
      static_cast_p2p <void> (&ChangeDisplaySettingsExW_Original) );

    EnumDisplaySettingsA_Original =
      (EnumDisplaySettingsA_pfn) SK_GetProcAddress
      ( hModUser32, "EnumDisplaySettingsA" );
    EnumDisplaySettingsW_Original =
      (EnumDisplaySettingsW_pfn) SK_GetProcAddress
      ( hModUser32, "EnumDisplaySettingsW" );

    InterlockedIncrement (&hooked);
  }

  else
    SK_Thread_SpinUntilAtomicMin (&hooked, 2);
}

SK_ICommandProcessor*
SK_Render_InitializeSharedCVars (void)
{
  SK_ICommandProcessor 
    *pCommandProc = nullptr;

  SK_RunOnce ( pCommandProc =
          SK_GetCommandProcessor () );

  if (pCommandProc != nullptr)
  {
    pCommandProc->AddVariable ( "PresentationInterval",
            new SK_IVarStub <int> (
              &config.render.framerate.present_interval )
                              );
    pCommandProc->AddVariable ( "PreRenderLimit",
            new SK_IVarStub <int> (
              &config.render.framerate.pre_render_limit )
                              );
    pCommandProc->AddVariable ( "BufferCount",
            new SK_IVarStub <int> (
              &config.render.framerate.buffer_count )
                              );
  }

  else
    pCommandProc = SK_GetCommandProcessor ();

  return pCommandProc;
}

bool
SK_Display_ApplyDesktopResolution (MONITORINFOEX& mi)
{
  if ((! config.display.resolution.override.isZero ()) ||
         config.display.refresh_rate > 0.0f)
  {
    DEVMODEW devmode              = {               };
             devmode.dmSize       = sizeof (DEVMODEW);

    if (! config.display.resolution.override.isZero ())
    {
      devmode.dmFields     |= DM_PELSWIDTH | DM_PELSHEIGHT;
      devmode.dmPelsWidth   = config.display.resolution.override.x;
      devmode.dmPelsHeight  = config.display.resolution.override.y;
    }

    if (config.display.refresh_rate > 0.0f)
    {
      devmode.dmFields           |= DM_DISPLAYFREQUENCY;
      devmode.dmDisplayFrequency  =
        static_cast <DWORD> (std::ceilf (config.display.refresh_rate));
    }

    if ( DISP_CHANGE_SUCCESSFUL ==
           SK_ChangeDisplaySettingsEx (
               mi.szDevice, &devmode,
               0, CDS_TEST, nullptr )
       )
    {
      if ( DISP_CHANGE_SUCCESSFUL ==
             SK_ChangeDisplaySettingsEx (
                 mi.szDevice, &devmode,
               0, 0/*CDS_UPDATEREGISTRY*/, nullptr )
         )
      {
        config.display.resolution.applied = true;
        return true;
      }
    }
  }

  return false;
};

void
SK_Vulkan_DisableThirdPartyLayers (void)
{
  SetEnvironmentVariableW (L"DISABLE_VK_LAYER_VALVE_steam_fossilize_1", L"1");
  SetEnvironmentVariableW (L"DISABLE_VULKAN_OBS_CAPTURE",               L"1");

//SetEnvironmentVariableW (L"DISABLE_VK_LAYER_VALVE_steam_overlay_1",   L"0");
//SetEnvironmentVariableW (L"ENABLE_VK_LAYER_VALVE_steam_overlay_1",    L"1");
//SetEnvironmentVariableW (L"SteamNoOverlayUIDrawing",                  L"1");
//SetEnvironmentVariableW (L"DISABLE_RTSS_LAYER",                       L"1");
}

bool
SK_RenderBackend_V2::resetTemporaryDisplayChanges (void)
{
  if (config.render.dxgi.temporary_dwm_hdr)
  {
    for ( auto pHDROutput : hdr_enabled_displays )
    {
      SK_Display_DisableHDR (pHDROutput);
    }
  }

  if (config.display.resolution.applied)
  {
    return 
      SK_ChangeDisplaySettingsEx (
        nullptr, nullptr,
          0, CDS_RESET, nullptr  ) == DISP_CHANGE_SUCCESSFUL;
  }

  return false;
}
