// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
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
#include <SpecialK/nvapi.h>
#include <imgui/font_awesome.h>
#include <SpecialK/render/dxgi/dxgi_swapchain.h>

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"  Reflex  "

#include <reflex/pclstats.h>

extern float __target_fps;

volatile ULONG64 SK_Reflex_LastFrameSleptVk  = 0;
volatile ULONG64 SK_Reflex_LastFrameMarked   = 0;
volatile LONG    SK_RenderBackend::flip_skip = 0;

using  NvAPI_QueryInterface_pfn       =
  void*                (*)(unsigned int ordinal);
using  NvAPI_D3D_SetLatencyMarker_pfn =
  NvAPI_Status (__cdecl *)(__in IUnknown                 *pDev,
                           __in NV_LATENCY_MARKER_PARAMS *pSetLatencyMarkerParams);
using  NvAPI_D3D_SetSleepMode_pfn     =
  NvAPI_Status (__cdecl *)(__in IUnknown                 *pDev,
                           __in NV_SET_SLEEP_MODE_PARAMS *pSetSleepModeParams);
using  NvAPI_D3D_Sleep_pfn            =
  NvAPI_Status (__cdecl *)(__in IUnknown                 *pDev);

// Keep track of the last input marker, so we can trigger flashes correctly.
NvU64                    SK_Reflex_LastInputFrameId          = 0ULL;
NvU64                    SK_Reflex_LastNativeMarkerFrame     = 0ULL;
NvU64                    SK_Reflex_LastNativeSleepFrame      = 0ULL;
NvU64                    SK_Reflex_LastNativeFramePresented  = 0ULL;
static constexpr auto    SK_Reflex_MinimumFramesBeforeNative = 150;
NV_SET_SLEEP_MODE_PARAMS SK_Reflex_NativeSleepModeParams     = { };

void SK_PCL_Heartbeat (const NV_LATENCY_MARKER_PARAMS& marker);

//
// NOTE: All hooks currently assume a game only has one D3D device, and that it is the
//       same one as SK's Render Backend is using.
//

static NvAPI_D3D_SetLatencyMarker_pfn NvAPI_D3D_SetLatencyMarker_Original = nullptr;
static NvAPI_D3D_SetSleepMode_pfn     NvAPI_D3D_SetSleepMode_Original     = nullptr;
static NvAPI_D3D_Sleep_pfn            NvAPI_D3D_Sleep_Original            = nullptr;

NVAPI_INTERFACE
SK_NvAPI_D3D_SetLatencyMarker ( __in IUnknown                 *pDev,
                                __in NV_LATENCY_MARKER_PARAMS *pSetLatencyMarkerParams )
{
  SK_PROFILE_SCOPED_TASK (NvAPI_D3D_SetLatencyMarker)

  if (NvAPI_D3D_SetLatencyMarker_Original != nullptr)
  {
    SK_ComPtr <ID3D12Device>                     pDev12;
    if (SK_slGetNativeInterface (pDev, (void **)&pDev12.p) == sl::Result::eOk)
      return NvAPI_D3D_SetLatencyMarker_Original(pDev12.p, pSetLatencyMarkerParams);
    else
      return NvAPI_D3D_SetLatencyMarker_Original(pDev,     pSetLatencyMarkerParams);
  }

  return
    NVAPI_NOT_SUPPORTED;
}

NVAPI_INTERFACE
SK_NvAPI_D3D_Sleep (__in IUnknown *pDev)
{
  SK_PROFILE_SCOPED_TASK (NvAPI_D3D_Sleep)

  // Ensure games never call this more than once per-frame, which
  //   Monster Hunter Wilds does and potentially other games too...
  static UINT64
      lastSleepFrame = MAXUINT64;
  if (lastSleepFrame == SK_GetFramesDrawn ())
  {
    return NVAPI_OK;
  }

  if (NvAPI_D3D_Sleep_Original != nullptr)
  {
    lastSleepFrame = SK_GetFramesDrawn ();

    SK_ComPtr <ID3D12Device>                     pDev12;
    if (SK_slGetNativeInterface (pDev, (void **)&pDev12.p) == sl::Result::eOk)
      return NvAPI_D3D_Sleep_Original (          pDev12.p);
    else
      return NvAPI_D3D_Sleep_Original (          pDev);
  }

  return
    NVAPI_NOT_SUPPORTED;
}

NVAPI_INTERFACE
NvAPI_D3D_Sleep_Detour (__in IUnknown *pDev)
{
  SK_PROFILE_SCOPED_TASK (HOOKED_API__NvAPI_D3D_Sleep)

  SK_LOG_FIRST_CALL

  SK_Reflex_LastNativeSleepFrame =
    SK_GetFramesDrawn ();

  if (SK_IsCurrentGame (SK_GAME_ID::MonsterHunterWilds))
  {
    return NVAPI_OK;
  }

  if (config.nvidia.reflex.disable_native)
    return NVAPI_OK;

  SK_ComPtr <ID3D12Device>                     pDev12;
  if (SK_slGetNativeInterface (pDev, (void **)&pDev12.p) == sl::Result::eOk)
    return SK_NvAPI_D3D_Sleep (                pDev12);
  else
    return SK_NvAPI_D3D_Sleep (pDev);
}

NVAPI_INTERFACE
SK_NvAPI_D3D_SetSleepMode ( __in IUnknown                 *pDev,
                            __in NV_SET_SLEEP_MODE_PARAMS *pSetSleepModeParams )
{
  NV_SET_SLEEP_MODE_PARAMS params =
             *pSetSleepModeParams;

  if (params.minimumIntervalUs != 0 &&         params.minimumIntervalUs > 50)
      params.minimumIntervalUs = pSetSleepModeParams->minimumIntervalUs - 2;

  SK_PROFILE_SCOPED_TASK 
     (NvAPI_D3D_SetSleepMode)
  if (NvAPI_D3D_SetSleepMode_Original != nullptr)
  {
    SK_ComPtr <ID3D12Device>                     pDev12;
    if (SK_slGetNativeInterface (pDev, (void **)&pDev12.p) == sl::Result::eOk)
      return NvAPI_D3D_SetSleepMode_Original (   pDev12, &params);
    else
      return NvAPI_D3D_SetSleepMode_Original (   pDev,   &params);
  }

  return
    NVAPI_NOT_SUPPORTED;
}

// If this returns true, we submitted the latency marker(s) ourselves and normal processing
//   logic should be skipped.
bool
SK_Reflex_FixOutOfBandInput (NV_LATENCY_MARKER_PARAMS& markerParams, IUnknown* pDevice, bool native = false)
{
  // If true, we submitted the latency marker(s) ourselves and the normal processing
  //   should be ignored.
  bool bFixed  = false;
  bool bNative = native && (! config.nvidia.reflex.disable_native);

  static std::atomic_bool bQueueInput    = false;
  static std::atomic_long lastMarkerType = OUT_OF_BAND_PRESENT_END;
  
  if (markerParams.markerType == INPUT_SAMPLE)
  {
    // bQueueInput=true denotes an invalid place to put an input latency marker,
    //   we will try to fudge with things and insert it at an appropriate time
    bQueueInput.store (
      (lastMarkerType.load () != SIMULATION_START)
    );

    return
      bQueueInput;
  }

  // Input Sample has to come between SIMULATION_START and SIMULATION_END, or it's invalid.
  //
  //   So take this opportunity to re-order some events if necessary
  if ( ( (markerParams.markerType == SIMULATION_START) ||
         (markerParams.markerType == SIMULATION_END) ) && bQueueInput.exchange (false) )
  {
    NV_LATENCY_MARKER_PARAMS
      input_params            = markerParams;
      input_params.markerType = INPUT_SAMPLE;

    bool bPreSubmit =
      (markerParams.markerType == SIMULATION_START);

    auto _SubmitMarker = [&](NV_LATENCY_MARKER_PARAMS& marker)
    {
         NvAPI_D3D_SetLatencyMarker_Original == nullptr ? NVAPI_OK :
      SK_NvAPI_D3D_SetLatencyMarker (pDevice, &marker);
  
      if (! bNative)
        SK_PCL_Heartbeat (markerParams);
    };

    if (bPreSubmit)
      _SubmitMarker (markerParams);

    // Submit our generated input marker in-between the possible real markers
    _SubmitMarker (input_params);
  
    if (! bPreSubmit)
      _SubmitMarker (markerParams);

    // We're fixing a game's native Reflex... make note of its internal frame id
    if (bNative)
      SK_Reflex_LastInputFrameId = markerParams.frameID;
  
    bFixed = true;
  }

  // We don't care about these, they can happen multiple times... they just
  //   can't happen anywhere outside of Simulation Start / End.
  if (markerParams.markerType != INPUT_SAMPLE)
  {
    lastMarkerType.store (markerParams.markerType);
  }

  if (bNative)
    SK_Reflex_LastNativeMarkerFrame = SK_GetFramesDrawn ();

  return
    bFixed;
}

std::optional <NvAPI_Status>
SK_Reflex_GameSpecificLatencyMarkerFixups ( __in IUnknown                 *pDev,
                                            __in NV_LATENCY_MARKER_PARAMS *pSetLatencyMarkerParams )
{
  if (SK_IsCurrentGame (SK_GAME_ID::MonsterHunterWilds))
  {
    if (pSetLatencyMarkerParams->markerType > INPUT_SAMPLE)
    {
      if (pSetLatencyMarkerParams->markerType == TRIGGER_FLASH)
      { // This marker randomly appears and causes problems
        return NVAPI_OK;
      }
      SK_LOGi0 (L"Marker Type: %d, on frame: %d", pSetLatencyMarkerParams->markerType, pSetLatencyMarkerParams->frameID);
    }

    static NvU64 lastSimFrame = MAXUINT64;
    if (pSetLatencyMarkerParams->markerType == SIMULATION_START)
    {
      if (lastSimFrame != pSetLatencyMarkerParams->frameID)
      {   lastSimFrame  = pSetLatencyMarkerParams->frameID;
    
        NV_LATENCY_MARKER_PARAMS fake_input = *pSetLatencyMarkerParams;
                                 fake_input.markerType = INPUT_SAMPLE;

        NVAPI_INTERFACE
        NvAPI_D3D_SetLatencyMarker_Detour ( __in IUnknown                 *pDev,
                                            __in NV_LATENCY_MARKER_PARAMS *pSetLatencyMarkerParams );

               NvAPI_D3D_SetLatencyMarker_Detour (pDev, pSetLatencyMarkerParams);
        return NvAPI_D3D_SetLatencyMarker_Detour (pDev, &fake_input);
      }
    }
  }

  return std::nullopt;
}

IUnknown*                SK_Reflex_LastLatencyDevice     = nullptr;
NV_LATENCY_MARKER_PARAMS SK_Reflex_LastLatencyMarkerParams;
bool                     SK_Reflex_AllowPresentEndMarker   = true;
bool                     SK_Reflex_AllowPresentStartMarker = true;

extern UINT            __SK_DLSSGMultiFrameCount;
extern IDXGISwapChain   *SK_Streamline_ProxyChain;

extern void   SK_SpawnPresentMonWorker (void);
extern HANDLE SK_ImGui_SignalBackupInputThread;

NVAPI_INTERFACE
NvAPI_D3D_SetLatencyMarker_Detour ( __in IUnknown                 *pDev,
                                    __in NV_LATENCY_MARKER_PARAMS *pSetLatencyMarkerParams )
{
  SK_LOG_FIRST_CALL

  SK_PROFILE_SCOPED_TASK (HOOKED_API__NvAPI_D3D_SetLatencyMarker)

  bool bSkipCall = false;

  // Ignore RTSS, it's most certainly not native Reflex.
  static auto hModRTSS = SK_GetModuleHandleW (SK_RunLHIfBitness (64, L"RTSSHooks64.dll",
                                                                     L"RTSSHooks.dll"));
  if (        hModRTSS && SK_GetCallingDLL () == hModRTSS)
    return NVAPI_OK;


  if ( SK_Streamline_ProxyChain != nullptr                          &&
         config.render.framerate.streamline.enable_native_limit     &&
         config.render.framerate.streamline.target_fps > 0.0f       &&
                                                 __SK_IsDLSSGActive &&
                                 pSetLatencyMarkerParams != nullptr )
  {
    if (pDev != nullptr)
    {
      if (pSetLatencyMarkerParams->markerType == PRESENT_START)
      {
        SK_Reflex_LastLatencyDevice       = pDev;
        SK_Reflex_LastLatencyMarkerParams = *pSetLatencyMarkerParams;

        if (! SK_Reflex_AllowPresentStartMarker)
          bSkipCall = true;
      }
      else if (pSetLatencyMarkerParams->markerType == PRESENT_END)
      {
        if (! SK_Reflex_AllowPresentEndMarker)
          bSkipCall = true;
      }
    }

#if 1
    if (pSetLatencyMarkerParams->markerType == SIMULATION_START ||
        pSetLatencyMarkerParams->markerType == INPUT_SAMPLE)
    {  
      auto pLimiter =
        SK::Framerate::GetLimiter (SK_Streamline_ProxyChain, false);

      if (pLimiter != nullptr && __SK_IsDLSSGActive)
      {
        if (SK_IsCurrentGame (SK_GAME_ID::MonsterHunterWilds))
        {
          config.render.framerate.streamline.enforcement_policy = 2;
        }

        auto& rb =
          SK_GetCurrentRenderBackend ();

        if ( config.render.framerate.streamline.enforcement_policy == 2 &&
                               pSetLatencyMarkerParams->markerType == INPUT_SAMPLE )
        {
          pLimiter->wait ();

          auto                                  tNow = SK_QueryPerf ();
          SK::Framerate::TickEx (false, -1.0,   tNow, rb.swapchain.p);
          //if (SK_IsCurrentGame (SK_GAME_ID::AssassinsCreed_Shadows) && __SK_DLSSGMultiFrameCount > 1)
          //{
          //  for ( UINT i = 0 ; i < __SK_DLSSGMultiFrameCount ; ++i )
          //  {                                    tNow.QuadPart += (pLimiter->get_ticks_per_frame () / (__SK_DLSSGMultiFrameCount + 1));
          //    SK::Framerate::TickEx (false, 0.0, tNow, rb.swapchain.p); 
          //  }
          //}
        }

        // Fallback to normal mode if the game has no latency markers
        //
        else if ( ( config.render.framerate.streamline.enforcement_policy == 4 || SK_Reflex_LastInputFrameId == 0 ) &&
                                      pSetLatencyMarkerParams->markerType == SIMULATION_START )
        {
          pLimiter->wait ();

          auto                                  tNow = SK_QueryPerf ();
          SK::Framerate::TickEx (false, -1.0,   tNow, rb.swapchain.p);
          //if (SK_IsCurrentGame (SK_GAME_ID::AssassinsCreed_Shadows) && __SK_DLSSGMultiFrameCount > 1)
          //{
          //  for ( UINT i = 0 ; i < __SK_DLSSGMultiFrameCount ; ++i )
          //  {                                    tNow.QuadPart += (pLimiter->get_ticks_per_frame () / (__SK_DLSSGMultiFrameCount + 1));
          //    SK::Framerate::TickEx (false, 0.0, tNow, rb.swapchain.p); 
          //  }
          //}
        }
      }
    }
#endif
  }

#ifdef _DEBUG
  // Naive test, proper test for equality would involve QueryInterface
  SK_ReleaseAssert (pDev == SK_GetCurrentRenderBackend ().device);
#endif

  if (! config.nvidia.reflex.disable_native)
  {
    const auto fixup =
    SK_Reflex_GameSpecificLatencyMarkerFixups (pDev, pSetLatencyMarkerParams);

    if ( fixup.has_value ())
      return fixup.value ();

    // Shutdown our own Reflex implementation
    if (! std::exchange (config.nvidia.reflex.native, true))
    {
      PCLSTATS_SHUTDOWN ();

      SK_LOG0 ( ( L"# Game is using NVIDIA Reflex natively..." ),
                  L"  Reflex  " );
    }
    
    // Prevent non-monotonic frame counts in AC Shadows,
    //   this might be a problem with Nixxes games too...
    if (! SK_IsCurrentGame (SK_GAME_ID::AssassinsCreed_Shadows))
    {
      SK_Reflex_LastNativeMarkerFrame =
        SK_GetFramesDrawn ();
    }
  }

  if (pSetLatencyMarkerParams != nullptr)
  {
    SK_ReleaseAssert (
      pSetLatencyMarkerParams->version <= NV_LATENCY_MARKER_PARAMS_VER
    );

    const bool bWantAccuratePresentTiming = false;
      //( config.render.framerate.target_fps > 0.0f ||
      //                        __target_fps > 0.0f ) && config.nvidia.reflex.native && (! config.nvidia.reflex.disable_native);

    if ( pSetLatencyMarkerParams->markerType == PRESENT_START ||
         pSetLatencyMarkerParams->markerType == PRESENT_END )
    {
      SK_Reflex_LastNativeFramePresented = pSetLatencyMarkerParams->frameID;

      if (bWantAccuratePresentTiming)
        return NVAPI_OK;
    }

    if (SK_Reflex_FixOutOfBandInput (*pSetLatencyMarkerParams, pDev, true))
      return NVAPI_OK;

    if (pSetLatencyMarkerParams->markerType == INPUT_SAMPLE)
      SK_Reflex_LastInputFrameId = pSetLatencyMarkerParams->frameID;

    if (config.nvidia.reflex.disable_native)
      return NVAPI_OK;
  }

  return
    bSkipCall ? NVAPI_OK :
    SK_NvAPI_D3D_SetLatencyMarker (pDev, pSetLatencyMarkerParams);
}

NVAPI_INTERFACE
NvAPI_D3D_SetSleepMode_Detour ( __in IUnknown                 *pDev,
                                __in NV_SET_SLEEP_MODE_PARAMS *pSetSleepModeParams )
{
  SK_LOG_FIRST_CALL

  SK_PROFILE_SCOPED_TASK (HOOKED_API__NvAPI_D3D_SetSleepMode)

  if (pSetSleepModeParams != nullptr)
  {
    SK_ReleaseAssert (
      pSetSleepModeParams->version <= NV_SET_SLEEP_MODE_PARAMS_VER
    );

    SK_Reflex_NativeSleepModeParams = *pSetSleepModeParams;

    if (pSetSleepModeParams->bUseMarkersToOptimize)
    {
      SK_RunOnce (
        SK_LOGi0 (L"NvAPI_D3D_SetSleepMode bUseMarkersToOptimize=true");

        config.nvidia.reflex.marker_optimization = true;
      );
    }
  }

  else {
    return NVAPI_INVALID_ARGUMENT;
  }

  bool applyOverride =
    (__SK_ForceDLSSGPacing && __target_fps > 10.0f) || config.nvidia.reflex.override;

  if (applyOverride)
  {
    pSetSleepModeParams->bLowLatencyBoost      = config.nvidia.reflex.low_latency_boost;
    pSetSleepModeParams->bLowLatencyMode       = config.nvidia.reflex.low_latency;
    pSetSleepModeParams->bUseMarkersToOptimize = config.nvidia.reflex.marker_optimization;

    if ((__SK_ForceDLSSGPacing && __target_fps > 10.0f) || config.nvidia.reflex.use_limiter)
    {
      config.nvidia.reflex.frame_interval_us =
            (UINT)(1000000.0 / __target_fps) + ( __SK_ForceDLSSGPacing ? 6
                                                                       : 0 );
    }
    else
      config.nvidia.reflex.frame_interval_us = 0;

    pSetSleepModeParams->minimumIntervalUs     = config.nvidia.reflex.frame_interval_us;
  }

  if (! pSetSleepModeParams->bLowLatencyMode)
  {
    pSetSleepModeParams->minimumIntervalUs = 0;
  }

  return
    SK_NvAPI_D3D_SetSleepMode (pDev, pSetSleepModeParams);
}

void
SK_NvAPI_HookReflex (void)
{
  static bool          init = false;
  if (! std::exchange (init, true))
  {
    const auto wszLibName =
      SK_RunLHIfBitness (64, L"nvapi64.dll",
                             L"nvapi.dll");
    HMODULE hLib =
      SK_Modules->LoadLibraryLL (wszLibName);

    if (hLib)
    {
      GetModuleHandleEx (
        GET_MODULE_HANDLE_EX_FLAG_PIN, wszLibName, &hLib
      );

      static auto NvAPI_QueryInterface =
        reinterpret_cast <NvAPI_QueryInterface_pfn> (
          SK_GetProcAddress (hLib, "nvapi_QueryInterface")
        );

      static auto constexpr D3D_SET_LATENCY_MARKER = 3650636805;
      static auto constexpr D3D_SET_SLEEP_MODE     = 2887559648;
      static auto constexpr D3D_SLEEP              = 2234307026;

      // Hook SetLatencyMarker so we know if a game is natively using Reflex.
      //
      SK_CreateFuncHook (      L"NvAPI_D3D_SetLatencyMarker",
                                 NvAPI_QueryInterface (D3D_SET_LATENCY_MARKER),
                                 NvAPI_D3D_SetLatencyMarker_Detour,
        static_cast_p2p <void> (&NvAPI_D3D_SetLatencyMarker_Original) );
      MH_QueueEnableHook (       NvAPI_QueryInterface (D3D_SET_LATENCY_MARKER));

      SK_CreateFuncHook (      L"NvAPI_D3D_SetSleepMode",
                                 NvAPI_QueryInterface (D3D_SET_SLEEP_MODE),
                                 NvAPI_D3D_SetSleepMode_Detour,
        static_cast_p2p <void> (&NvAPI_D3D_SetSleepMode_Original) );
      MH_QueueEnableHook (       NvAPI_QueryInterface (D3D_SET_SLEEP_MODE));

      SK_CreateFuncHook (      L"NvAPI_D3D_Sleep",
                                 NvAPI_QueryInterface (D3D_SLEEP),
                                 NvAPI_D3D_Sleep_Detour,
        static_cast_p2p <void> (&NvAPI_D3D_Sleep_Original) );
      MH_QueueEnableHook (       NvAPI_QueryInterface (D3D_SLEEP));

      SK_ApplyQueuedHooks ();
    }
  }
}

void
SK_PCL_Heartbeat (const NV_LATENCY_MARKER_PARAMS& marker)
{
  if (config.nvidia.reflex.native)
    return;

  SK_PROFILE_SCOPED_TASK (SK_PCL_Heartbeat)

  static bool init = false;

  if (marker.markerType == SIMULATION_START)
  {
    if (! std::exchange (init, true))
    {
      SK_NvAPI_HookReflex ();

      PCLSTATS_SET_ID_THREAD ((DWORD)-1);
      PCLSTATS_INIT          (        0);

      g_PCLStatsEnable = true;
    }

    if (g_PCLStatsIdThread == -1)
      PCLSTATS_SET_ID_THREAD (GetCurrentThreadId ());

    // Place latency ping marker
    if (PCLSTATS_IS_SIGNALED ())
    {
      const SK_RenderBackend &rb =
        SK_GetCurrentRenderBackend ();

      if (! config.nvidia.reflex.vulkan)
        rb.setLatencyMarkerNV (PC_LATENCY_PING);
      else
      {
        auto marker_copy = marker;
             marker_copy.markerType = PC_LATENCY_PING;

        SK_PCL_Heartbeat (marker_copy);
      }
    }
  }

  if (init)
  {
    SK_PROFILE_SCOPED_TASK (PCLSTATS_MARKER)

    PCLSTATS_MARKER ( marker.markerType,
                      marker.frameID );
  }
}

bool
SK_RenderBackend_V2::isReflexSupported (void) const
{
  // Interop and HW vendor never change...
  //   api -might-, but we'll just ignore that for perf.
  static BOOL _supported  = -1;
  if (        _supported != -1) {
      return (_supported != 0 || config.nvidia.reflex.vulkan);
  }

  bool supported =
    sk::NVAPI::nv_hardware && SK_API_IsDXGIBased (api) && 
    SK_Render_GetVulkanInteropSwapChainType      (swapchain) == SK_DXGI_VK_INTEROP_TYPE_NONE;

  // By 150 frames, we should have a solid idea whether Reflex is supported
  if (SK_GetFramesDrawn () > SK_Reflex_MinimumFramesBeforeNative)
    _supported = supported ? 1 : 0;

  return supported;
}

#include <vulkan/vulkan_core.h>

bool
SK_RenderBackend_V2::setLatencyMarkerNV (NV_LATENCY_MARKER_TYPE marker) const
{
  SK_PROFILE_SCOPED_TASK (SK_RenderBackend_V2__setLatencyMarkerNV)

  if (marker == RENDERSUBMIT_START)
  {
    if (        SK_ImGui_SignalBackupInputThread != 0)
      SetEvent (SK_ImGui_SignalBackupInputThread     );
  }

  NvAPI_Status ret =
    NVAPI_INVALID_CONFIGURATION;

  if (device.p    != nullptr &&
      swapchain.p != nullptr)
  {
    // Avert your eyes... time to check if Device and SwapChain are consistent
    //
    SK_ComPtr <ID3D11Device> pDev11;
    SK_ComPtr <ID3D12Device> pDev12;

    if      (api == SK_RenderAPI::D3D11)
                             pDev11 = (ID3D11Device *)device.p;
    else if (api == SK_RenderAPI::D3D12) 
                             pDev12 = (ID3D12Device *)device.p;
    else
    {
                    pDev11 = getDevice <ID3D11Device> ();
      if (! pDev11) pDev12 = getDevice <ID3D12Device> ();
    }

    if (! (pDev11 || pDev12))
      return false; // Uh... what?

#ifdef DEBUG
    SK_ReleaseAssert (SK_ComQIPtr <IDXGISwapChain> (swapchain.p) != nullptr);
#endif

    SK_ComPtr <IDXGISwapChain1>
        pSwapChain ((IDXGISwapChain1*)swapchain.p);
    if (pSwapChain.p != nullptr)
    {
      // This fails in some ReShade setups, we need to write private data to
      //   the SwapChain device and see if reading it back works... but for now,
      //     just ignore the problem outright.
#if 0
      SK_ComPtr                  <ID3D11Device>            pSwapDev11;
      SK_ComPtr                  <ID3D12Device>            pSwapDev12;

      if (                        pDev11.p != nullptr)
      pSwapChain->GetDevice ( IID_ID3D11Device,  (void **)&pSwapDev11.p );
      if (                        pDev12.p != nullptr)
      pSwapChain->GetDevice ( IID_ID3D12Device,  (void **)&pSwapDev12.p );
      
      if (! ((pDev11.p != nullptr && pDev11.IsEqualObject (pSwapDev11.p)) ||
             (pDev12.p != nullptr && pDev12.IsEqualObject (pSwapDev12.p))))
      {
        return false; // Nope, let's get the hell out of here!
      }
#endif
    }

    if (marker == RENDERSUBMIT_END)
    {
      latency.submitQueuedFrame (pSwapChain.p);
    }

    // Vulkan Early-Out
    if (config.nvidia.reflex.vulkan)
    {
      if (marker == INPUT_SAMPLE &&  config.nvidia.reflex.vulkan
                                 && !config.nvidia.reflex.native)
      {
        VkSetLatencyMarkerInfoNV
          vk_marker           = {                                          };
          vk_marker.sType     = VK_STRUCTURE_TYPE_SET_LATENCY_MARKER_INFO_NV;
          vk_marker.presentID = SK_GetFramesDrawn ();

        void
        SK_VK_SetLatencyMarker (VkSetLatencyMarkerInfoNV& marker, VkLatencyMarkerNV type);
        SK_VK_SetLatencyMarker (vk_marker,             VK_LATENCY_MARKER_INPUT_SAMPLE_NV);
      }

      if (swapchain.p != nullptr &&
          config.render.framerate.pre_render_limit != -1)
      {
        SK_ComQIPtr <IDXGISwapChain>
            pChain (      swapchain);
        if (pChain.p != nullptr)
        {
          SK_ComPtr <IDXGIDevice1>                       pDev1;
          pChain->GetDevice (IID_IDXGIDevice1, (void **)&pDev1.p);

          if (pDev1.p != nullptr) {
              pDev1->SetMaximumFrameLatency (
                config.render.framerate.pre_render_limit );
          }
        }
      }

      return true;
    }

    if (! isReflexSupported ())
      return false;

    if (SK_GetFramesDrawn () < SK_Reflex_MinimumFramesBeforeNative)
      return true;

    const bool bWantAccuratePresentTiming = false;
      //( config.render.framerate.target_fps > 0.0f ||
      //                        __target_fps > 0.0f ) && config.nvidia.reflex.native && (! config.nvidia.reflex.disable_native);

    // Only do this if game is not Reflex native, or if the marker is a flash
    if ((! config.nvidia.reflex.native) || marker == TRIGGER_FLASH || (bWantAccuratePresentTiming && (marker == PRESENT_START || marker == PRESENT_END)))
    {
      NV_LATENCY_MARKER_PARAMS
        markerParams            = {                          };
        markerParams.version    = NV_LATENCY_MARKER_PARAMS_VER;
        markerParams.markerType = marker;
        markerParams.frameID    = static_cast <NvU64> (
             ReadULong64Acquire (&frames_drawn)       );

      // Triggered input flash, in a Reflex-native game
      if (config.nvidia.reflex.native)
      {
        if (marker == TRIGGER_FLASH)
        {
          markerParams.frameID =
            SK_Reflex_LastInputFrameId;
        }

        else if (marker == PRESENT_START && bWantAccuratePresentTiming)
        {
          if (config.nvidia.reflex.native)
          {
            markerParams.frameID =
              SK_Reflex_LastNativeFramePresented;
          }
        }

        else if (marker == PRESENT_END && bWantAccuratePresentTiming)
        {
          if (config.nvidia.reflex.native)
          {
            markerParams.frameID =
              SK_Reflex_LastNativeFramePresented;
          }
        }

        else
          return true;
      }

      // Make sure input events get pushed into the appropriate Simulation stage
      if (SK_Reflex_FixOutOfBandInput (markerParams, device.p))
        return true;

      // No recursion please
      SK_PCL_Heartbeat (markerParams);

      // SetLatencyMarker is hooked on the simulation marker of the first frame,
      //   we may have gotten here out-of-order.
      ret = NvAPI_D3D_SetLatencyMarker_Original == nullptr ? NVAPI_OK :
         SK_NvAPI_D3D_SetLatencyMarker (device.p, &markerParams);
    }
  }

  return
    ( ret == NVAPI_OK );
}

bool
SK_RenderBackend_V2::getLatencyReportNV (NV_LATENCY_RESULT_PARAMS* pGetLatencyParams) const
{
  if (device.p == nullptr)
    return false;

  if (! isReflexSupported ())
    return false;

  SK_PROFILE_SCOPED_TASK (NvAPI_D3D_GetLatency)

  if (vulkan_reflex.api != SK_RenderBackend_V2::vk_reflex_s::None &&
      vulkan_reflex.getLatencyReport (pGetLatencyParams))
  {
    return true;
  }

  NvAPI_Status ret =
    NvAPI_D3D_GetLatency (device.p, pGetLatencyParams);

  return
    ( ret == NVAPI_OK );
}


void
SK_RenderBackend_V2::driverSleepNV (int site) const
{
  if (! device.p)
    return;

  if (! isReflexSupported ())
    return;

  // Native Reflex games may only call NvAPI_D3D_SetSleepMode once, we can wait
  //   a few frames to determine if the game is Reflex-native.
  if (SK_GetFramesDrawn () < SK_Reflex_MinimumFramesBeforeNative)
    return;

  static bool
    lastOverride = false;

  bool nativeSleepRecently =
    SK_Reflex_LastNativeSleepFrame > SK_GetFramesDrawn () - 10;

  bool applyOverride =
    (__SK_ForceDLSSGPacing && __target_fps > 10.0f) || config.nvidia.reflex.override;

  // Game has native Reflex, we should bail out (unles overriding it).
  if (config.nvidia.reflex.native && (! applyOverride))
  {
    // Restore game's original Sleep mode when turning override off...
    if (std::exchange (lastOverride, applyOverride) !=
                                     applyOverride)
    {
      // Uninitialized? => Game may have done this before we had a hook
      if (SK_Reflex_NativeSleepModeParams.version == 0)
      {
        NV_GET_SLEEP_STATUS_PARAMS
          sleepStatusParams         = {                            };
          sleepStatusParams.version = NV_GET_SLEEP_STATUS_PARAMS_VER;

        {
          SK_PROFILE_SCOPED_TASK (NvAPI_D3D_GetSleepStatus)

          if ( NVAPI_OK ==
                 NvAPI_D3D_GetSleepStatus (device.p, &sleepStatusParams)
             )
          {
            SK_Reflex_NativeSleepModeParams.bLowLatencyMode  =
              sleepStatusParams.bLowLatencyMode;
            SK_Reflex_NativeSleepModeParams.version          =
              NV_SET_SLEEP_MODE_PARAMS_VER;
          }
        }
      }

      SK_NvAPI_D3D_SetSleepMode (
               device.p, &SK_Reflex_NativeSleepModeParams );
    }

    if (nativeSleepRecently)
      return;
  }

  if (site == 2 && (! config.nvidia.reflex.native))
    setLatencyMarkerNV (INPUT_SAMPLE);

  if (site == config.nvidia.reflex.enforcement_site)
  {
    config.nvidia.reflex.frame_interval_us = 0;

    static bool
      valid = true;

    if (! valid)
    {
      // NvAPI Reflex failures shouldn't affect Vulkan
      if (! config.nvidia.reflex.vulkan)
      {
        config.nvidia.reflex.use_limiter = false;

        return;
      }
    }

    if (config.nvidia.reflex.use_limiter || __SK_ForceDLSSGPacing)
    {
      if (__target_fps > 10.0f)
      {
        config.nvidia.reflex.frame_interval_us =
          (UINT)(1000000.0 / __target_fps) + ( __SK_ForceDLSSGPacing ? 24
                                                                     : 0 );
      }
    }

    if (! valid)
    {
      return;
    }

    NV_SET_SLEEP_MODE_PARAMS
      sleepParams = {                          };
      sleepParams.version               = NV_SET_SLEEP_MODE_PARAMS_VER;
      sleepParams.bLowLatencyBoost      = config.nvidia.reflex.low_latency_boost;
      sleepParams.bLowLatencyMode       = config.nvidia.reflex.low_latency;
      sleepParams.minimumIntervalUs     = config.nvidia.reflex.frame_interval_us;
      sleepParams.bUseMarkersToOptimize = config.nvidia.reflex.marker_optimization;

    static NV_SET_SLEEP_MODE_PARAMS
      lastParams = { 1, true, true, 69, 0, { 0 } };

    static bool
      lastEnable = config.nvidia.reflex.enable;

    static IUnknown
      *lastSwapChain = nullptr,
      *lastDevice    = nullptr;

    if ((! config.nvidia.reflex.enable) && (! applyOverride))
    {
      sleepParams.bLowLatencyBoost      = false;
      sleepParams.bLowLatencyMode       = false;
      sleepParams.bUseMarkersToOptimize = false;
      sleepParams.minimumIntervalUs     = 0;
    }

    // Despite what NvAPI says about overhead from changing
    //   sleep parameters every frame, many Streamline games do...
    static volatile ULONG64 _frames_drawn =
      std::numeric_limits <ULONG64>::max ();
    if ( ReadULong64Acquire (&_frames_drawn) ==
         ReadULong64Acquire  (&frames_drawn) )
      return;

    //if ( config.nvidia.reflex.native              &&
    //     applyOverride                            &&
    //     config.nvidia.reflex.marker_optimization &&
    //     SK_GetCurrentRenderBackend ().windows.unreal )
    //{
    //  SK_RunOnce (
    //    SK_LOGi0 ( L"Disabling bUseMarkersToOptimize in Unreal Engine"
    //               L"because of incompatible RHI threading..." )
    //  );
    //
    //  config.nvidia.reflex.marker_optimization = false;
    //}

    if ( lastParams.version               != sleepParams.version               ||
         lastSwapChain                    != swapchain.p                       ||
         lastDevice                       != device.p                          ||
         lastEnable                       != config.nvidia.reflex.enable       ||
         lastParams.bLowLatencyBoost      != sleepParams.bLowLatencyBoost      ||
         lastParams.bLowLatencyMode       != sleepParams.bLowLatencyMode       ||
         lastParams.minimumIntervalUs     != sleepParams.minimumIntervalUs     ||
         lastParams.bUseMarkersToOptimize != sleepParams.bUseMarkersToOptimize ||
         lastOverride                     != applyOverride )
    {
      NvAPI_Status status =
        SK_NvAPI_D3D_SetSleepMode (
           device.p, &sleepParams );

      if (status != NVAPI_OK)
      {
        SK_LOG0 ( ( L"NVIDIA Reflex SetSleepMode Invalid State "
            L"( NvAPI_Status = %i )", status ),
            __SK_SUBSYSTEM__ );

        valid = false;
      }

      else
      {
        SK_LOGi1 (L"Reflex Sleep Mode Set...");

        lastOverride  = applyOverride;
        lastParams    = sleepParams;
        lastSwapChain = swapchain.p;
        lastDevice    = device.p;
        lastEnable    = config.nvidia.reflex.enable;

        WriteULong64Release (&_frames_drawn,
          ReadULong64Acquire (&frames_drawn));
      }
    }

    // Our own implementation
    //
    if ((! config.nvidia.reflex.native) || config.nvidia.reflex.disable_native || (! nativeSleepRecently))
    {
      NvAPI_Status status =
        SK_NvAPI_D3D_Sleep (device.p);

      if ( status != NVAPI_OK )
        valid = false;

      if ((! valid) && ( api == SK_RenderAPI::D3D11 ||
                         api == SK_RenderAPI::D3D12 ))
      {
        SK_LOG0 ( ( L"NVIDIA Reflex Sleep Invalid State "
                    L"( NvAPI_Status = %i )", status ),
                    __SK_SUBSYSTEM__ );
      }
    }
  }
};

void
SK_NV_AdaptiveSyncControl (void)
{
  if (sk::NVAPI::nv_hardware != false)
  {
    SK_RenderBackend& rb =
      SK_GetCurrentRenderBackend ();

    // We need to force a GSync status check at least once
    //   in order to draw this correctly
    SK_RunOnce (
    {
      config.apis.NvAPI.implicit_gsync = true;

      if (rb.api == SK_RenderAPI::D3D12)
      {
        // It is necessary to start PresentMon in D3D12, or the VRR indicator will not work
        SK_SpawnPresentMonWorker ();
      }
    });

    for ( auto& display : rb.displays )
    {
      if (display.monitor == rb.monitor)
      {
        static NV_GET_ADAPTIVE_SYNC_DATA
                     getAdaptiveSync   = { };
        static DWORD lastChecked       = 0;

        if (SK::ControlPanel::current_time > lastChecked + 333)
        {                                    lastChecked = SK::ControlPanel::current_time;

          ZeroMemory (&getAdaptiveSync,  sizeof (NV_GET_ADAPTIVE_SYNC_DATA));
                       getAdaptiveSync.version = NV_GET_ADAPTIVE_SYNC_DATA_VER;

          if ( NVAPI_OK ==
                 SK_NvAPI_DISP_GetAdaptiveSyncData (
                   display.nvapi.display_id,
                           &getAdaptiveSync )
             )
          {
            rb.gsync_state.update (true);
          }

          else lastChecked = SK::ControlPanel::current_time + 333;
        }

        float fEffectiveRefresh =
          display.statistics.vblank_counter.getVBlankHz (SK_QueryPerf ().QuadPart);

        ImGui::Text       ("%hs Status for %hs", display.vrr.type, SK_WideCharToUTF8 (rb.display_name).c_str ());
        ImGui::Separator  ();
        ImGui::BeginGroup ();
        ImGui::Text       ("Current State:");
        if (! getAdaptiveSync.bDisableAdaptiveSync)
        {
          ImGui::Text     ("Current Refresh:");

          if (getAdaptiveSync.maxFrameInterval != 0)
          {
            ImGui::Text   ("Minimum Refresh:");
          }
        }
        ImGui::EndGroup   ();
        ImGui::SameLine   ();
        ImGui::BeginGroup ();

        const ImVec4 vStatusColor =
          getAdaptiveSync.bDisableAdaptiveSync   ? ImVec4 (1.f, 0.f, 0.f, 1.f) :
                           rb.gsync_state.active ? ImVec4 (0.f, 1.f, 0.f, 1.f) :
                                                   ImVec4 (.8f, .8f, .8f, 1.f);

        ImGui::TextColored ( vStatusColor,
              getAdaptiveSync.bDisableAdaptiveSync   ? "Disabled" :
                               rb.gsync_state.active ? "Active"   :
                                                       "Inactive" );

        if (! getAdaptiveSync.bDisableAdaptiveSync)
        {
          auto *pLimiter =
            SK::Framerate::GetLimiter (
              rb.swapchain.p, false   );

          if (pLimiter != nullptr)
              pLimiter->frame_history_snapshots->frame_history.calcMean ();

          ImGui::Text     ( "%#5.01f Hz", fEffectiveRefresh );

          if (pLimiter != nullptr)
          {
            const auto snapshots =
              pLimiter->frame_history_snapshots.getPtr ();

            const float fFPS =
              static_cast <float> (1000.0 / snapshots->cached_mean.val);
            //const float fLFC = 1000000.0f /
            //  static_cast <float> (getAdaptiveSync.maxFrameInterval);

            if (fEffectiveRefresh > 1.08 * fFPS)
            {
              ImGui::SameLine ();

              ImGui::TextColored (ImVec4 (.8f, .8f, 0.f, 1.f), " (LFC x%d)",
                static_cast <int> (std::floorf (0.5f + (fEffectiveRefresh / fFPS))) );
            }
          }

          if (! config.apis.NvAPI.gsync_status)
          {
            ImGui::SameLine ();
            ImGui::Text   ( " " ICON_FA_EXCLAMATION_TRIANGLE " G-Sync Status is disabled in the Display menu!");
          }

          if (getAdaptiveSync.maxFrameInterval != 0)
          {
            ImGui::Text   ( "%#5.01f Hz ",
                             1000000.0 / static_cast <double> (getAdaptiveSync.maxFrameInterval) );
          }
        }
        ImGui::Text       ( "\t\t\t\t" );
        
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

          if (! getAdaptiveSync.bDisableAdaptiveSync)
          {
            ImGui::SameLine ();

            toggle_split =
              ImGui::Button (
                getAdaptiveSync.bDisableFrameSplitting == 0x0 ?
                              "Disable Frame Splitting"       :
                               "Enable Frame Splitting" );
          }
        }

        ImGui::EndGroup   ();
        //ImGui::SameLine   ();
        //ImGui::BeginGroup ();
        //
        //if (rb.displays [rb.active_display].nvapi.monitor_caps.data.caps.isRLACapable)
        //{
        //  if (ImGui::Button ("Trigger Reflex Flash"))
        //  {
        //    rb.setLatencyMarkerNV (TRIGGER_FLASH);
        //  }
        //}
        //ImGui::EndGroup   ();

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

          SK_NvAPI_DISP_SetAdaptiveSyncData (
            display.nvapi.display_id,
                    &setAdaptiveSync
          );
        }
        break;
      }
    }
  }
}

#include <vulkan/vulkan.h>

typedef DWORD NvLL_VK_Status;

static constexpr NvLL_VK_Status NVLL_VK_OK = 0;

struct NVLL_VK_SET_SLEEP_MODE_PARAMS {
  bool     bLowLatencyMode;
  bool     bLowLatencyBoost;
  uint32_t minimumIntervalUs;
};

struct NVLL_VK_LATENCY_RESULT_PARAMS {
  struct vkFrameReport {
    uint64_t frameID;
    uint64_t inputSampleTime;
    uint64_t simStartTime;
    uint64_t simEndTime;
    uint64_t renderSubmitStartTime;
    uint64_t renderSubmitEndTime;
    uint64_t presentStartTime;
    uint64_t presentEndTime;
    uint64_t driverStartTime;
    uint64_t driverEndTime;
    uint64_t osRenderQueueStartTime;
    uint64_t osRenderQueueEndTime;
    uint64_t gpuRenderStartTime;
    uint64_t gpuRenderEndTime;
  } frameReport [64];
};

enum NVLL_VK_LATENCY_MARKER_TYPE
{
  VK_SIMULATION_START               = 0,
  VK_SIMULATION_END                 = 1,
  VK_RENDERSUBMIT_START             = 2,
  VK_RENDERSUBMIT_END               = 3,
  VK_PRESENT_START                  = 4,
  VK_PRESENT_END                    = 5,
  VK_INPUT_SAMPLE                   = 6,
  VK_TRIGGER_FLASH                  = 7,
  VK_PC_LATENCY_PING                = 8,
  VK_OUT_OF_BAND_RENDERSUBMIT_START = 9,
  VK_OUT_OF_BAND_RENDERSUBMIT_END   = 10,
  VK_OUT_OF_BAND_PRESENT_START      = 11,
  VK_OUT_OF_BAND_PRESENT_END        = 12,
};

struct NVLL_VK_LATENCY_MARKER_PARAMS
{
  uint64_t                    frameID;
  NVLL_VK_LATENCY_MARKER_TYPE markerType;
};

using  NvLL_VK_SetSleepMode_pfn = NvLL_VK_Status (*)(VkDevice, NVLL_VK_SET_SLEEP_MODE_PARAMS*);
static NvLL_VK_SetSleepMode_pfn
       NvLL_VK_SetSleepMode_Original = nullptr;
       
using  NvLL_VK_InitLowLatencyDevice_pfn = NvLL_VK_Status (*)(VkDevice, VkSemaphore*);
static NvLL_VK_InitLowLatencyDevice_pfn
       NvLL_VK_InitLowLatencyDevice_Original = nullptr;

using  NvLL_VK_Sleep_pfn = NvLL_VK_Status (*)(VkDevice, uint64_t);
static NvLL_VK_Sleep_pfn
       NvLL_VK_Sleep_Original = nullptr;

using  NvLL_VK_SetLatencyMarker_pfn = NvLL_VK_Status (*)(VkDevice, NVLL_VK_LATENCY_MARKER_PARAMS*);
static NvLL_VK_SetLatencyMarker_pfn
       NvLL_VK_SetLatencyMarker_Original = nullptr;

using  NvLL_VK_GetLatency_pfn = NvLL_VK_Status (*)(VkDevice, NVLL_VK_LATENCY_RESULT_PARAMS*);
static NvLL_VK_GetLatency_pfn
       NvLL_VK_GetLatency = nullptr;

extern void SK_VK_HookFirstDevice (VkDevice device);

struct {
  VkDevice       device         = 0;
  VkSwapchainKHR swapchain      = 0;
  uint64_t       last_frame     = 0;
  VkSemaphore    NvLL_semaphore = 0;
} SK_VK_Reflex;

#undef VK_NV_low_latency2

void
SK_Reflex_SetVulkanSwapchain (VkDevice device, VkSwapchainKHR swapchain)
{
  SK_GetCurrentRenderBackend ().vulkan_reflex.api =
    SK_RenderBackend_V2::vk_reflex_s::VK_NV_low_latency2;

  SK_VK_Reflex.device    = device;
  SK_VK_Reflex.swapchain = swapchain;
}

NvLL_VK_Status
NvLL_VK_InitLowLatencyDevice_Detour (VkDevice device, VkSemaphore* signalSemaphoreHandle)
{
  SK_LOG_FIRST_CALL

  SK_VK_HookFirstDevice (device);

  auto& rb =
    SK_GetCurrentRenderBackend ();

  config.nvidia.reflex.native = true;
  config.nvidia.reflex.vulkan = true;
  SK_VK_Reflex.device         = device;
  SK_VK_Reflex.swapchain      = 0;
  rb.vulkan_reflex.api        = SK_RenderBackend_V2::vk_reflex_s::NvLowLatencyVk;

  auto ret =
    NvLL_VK_InitLowLatencyDevice_Original (device, &SK_VK_Reflex.NvLL_semaphore);

  if (signalSemaphoreHandle != nullptr)
     *signalSemaphoreHandle = SK_VK_Reflex.NvLL_semaphore;

  return ret;
}

NVLL_VK_SET_SLEEP_MODE_PARAMS SK_NVLL_LastSleepParams;

NvLL_VK_Status
NvLL_VK_SetSleepMode_Detour (VkDevice device, NVLL_VK_SET_SLEEP_MODE_PARAMS* sleepModeParams)
{
  SK_LOG_FIRST_CALL

  SK_VK_HookFirstDevice (device);
  SK_VK_Reflex.device  = device;

  if (sleepModeParams != nullptr)
  {
    if (config.nvidia.reflex.override)
    {
      if (config.nvidia.reflex.enable)
      {
        sleepModeParams->bLowLatencyMode  = config.nvidia.reflex.low_latency;
        sleepModeParams->bLowLatencyBoost = config.nvidia.reflex.low_latency_boost;
      }

      else
      {
        sleepModeParams->bLowLatencyMode  = false;
        sleepModeParams->bLowLatencyBoost = false;
      }
    }

    const auto reflex_interval_us =
      SK_Reflex_CalculateSleepMinIntervalForVulkan (sleepModeParams->bLowLatencyMode);

    sleepModeParams->minimumIntervalUs =
      std::max (sleepModeParams->minimumIntervalUs, reflex_interval_us);

    SK_NVLL_LastSleepParams = *sleepModeParams;
  }

  return
    NvLL_VK_SetSleepMode_Original (device, sleepModeParams);
}

NvLL_VK_Status
NvLL_VK_SetLatencyMarker_Detour (VkDevice vkDevice, NVLL_VK_LATENCY_MARKER_PARAMS* pSetLatencyMarkerParams)
{
  SK_LOG_FIRST_CALL

  SK_VK_HookFirstDevice (vkDevice);
  SK_VK_Reflex.device  = vkDevice;

  if (pSetLatencyMarkerParams != nullptr)
  {
    // The game's frameID, SK has a different running counter...
    SK_VK_Reflex.last_frame = pSetLatencyMarkerParams->frameID;
  }

  return
    NvLL_VK_SetLatencyMarker_Original (vkDevice, pSetLatencyMarkerParams);
}

NvLL_VK_Status
NvLL_VK_Sleep_Detour (VkDevice device, uint64_t signalValue)
{
  SK_LOG_FIRST_CALL

  SK_VK_HookFirstDevice (device);
  SK_VK_Reflex.device  = device;

  if (config.nvidia.reflex.override || __SK_ForceDLSSGPacing)
  {
    NvLL_VK_SetSleepMode_Detour (device, &SK_NVLL_LastSleepParams);
  }

  auto ret =
    NvLL_VK_Sleep_Original (device, signalValue);

  if (0 == ret)
  {
    auto& rb =
      SK_GetCurrentRenderBackend ();

    rb.vulkan_reflex.sleep ();

    extern void SK_Reflex_WaitOnSemaphore (VkDevice device, VkSemaphore       semaphore, uint64_t value);
                SK_Reflex_WaitOnSemaphore (         device, SK_VK_Reflex.NvLL_semaphore,    signalValue);

    if (config.render.framerate.enforcement_policy == 2 && rb.vulkan_reflex.isPacingEligible ())
    {
      SK::Framerate::Tick ( true, 0.0,
                      { 0,0 }, rb.swapchain.p);
    }
  }

  return ret;
}

void
SK_VK_SetLatencyMarker (VkSetLatencyMarkerInfoNV& marker, VkLatencyMarkerNV type)
{
  NV_LATENCY_MARKER_PARAMS
    nvapi_marker            = {              };
    nvapi_marker.markerType = (NV_LATENCY_MARKER_TYPE)type;

  extern PFN_vkSetLatencyMarkerNV
             vkSetLatencyMarkerNV_Original;

  auto& rb =
    SK_GetCurrentRenderBackend ();

  if (rb.vulkan_reflex.api == SK_RenderBackend_V2::vk_reflex_s::VK_NV_low_latency2)
  {
    if (! vkSetLatencyMarkerNV_Original)
      return;

    extern VkDevice       SK_Reflex_VkDevice;
    extern VkSwapchainKHR SK_Reflex_VkSwapchain;

                                                                               marker.marker = type;
    vkSetLatencyMarkerNV_Original (SK_Reflex_VkDevice, SK_Reflex_VkSwapchain, &marker);

    nvapi_marker.frameID = marker.presentID;

    // Up to marker type TRIGGER_FLASH, the Vulkan extension and NVAPI enums match for marker type.
  }

  else if (rb.vulkan_reflex.api == SK_RenderBackend_V2::vk_reflex_s::NvLowLatencyVk)
  {
    NVLL_VK_LATENCY_MARKER_PARAMS
      marker_params            = {                     };
      marker_params.frameID    = SK_VK_Reflex.last_frame;
      marker_params.markerType = (NVLL_VK_LATENCY_MARKER_TYPE)type;//VK_INPUT_SAMPLE;

    NvLL_VK_SetLatencyMarker_Original (SK_VK_Reflex.device, &marker_params);

    nvapi_marker.frameID = marker.presentID;
  }

  extern void
  SK_PCL_Heartbeat (const NV_LATENCY_MARKER_PARAMS& marker);
  SK_PCL_Heartbeat (nvapi_marker);
}

void SK_VK_HookReflex (void)
{
  SK_RunOnce (
    SK_CreateDLLHook2 (      L"NvLowLatencyVk.dll",
                              "NvLL_VK_InitLowLatencyDevice",
                               NvLL_VK_InitLowLatencyDevice_Detour,
      static_cast_p2p <void> (&NvLL_VK_InitLowLatencyDevice_Original) );

    SK_CreateDLLHook2 (      L"NvLowLatencyVk.dll",
                              "NvLL_VK_SetSleepMode",
                               NvLL_VK_SetSleepMode_Detour,
      static_cast_p2p <void> (&NvLL_VK_SetSleepMode_Original) );

    SK_CreateDLLHook2 (      L"NvLowLatencyVk.dll",
                              "NvLL_VK_Sleep",
                               NvLL_VK_Sleep_Detour,
      static_cast_p2p <void> (&NvLL_VK_Sleep_Original) );

    SK_CreateDLLHook2 (      L"NvLowLatencyVk.dll",
                              "NvLL_VK_SetLatencyMarker",
                               NvLL_VK_SetLatencyMarker_Detour,
      static_cast_p2p <void> (&NvLL_VK_SetLatencyMarker_Original) );

    NvLL_VK_GetLatency =
   (NvLL_VK_GetLatency_pfn)SK_GetProcAddress (L"NvLowLatencyVk.dll",
   "NvLL_VK_GetLatency");

    SK_ApplyQueuedHooks ();
  );
}

ULONG64
SK_RenderBackend_V2::vk_reflex_s::sleep (void)
{
  config.nvidia.reflex.vulkan = true;

  return
    InterlockedExchange (&last_slept, SK_GetFramesDrawn ());
}

bool
SK_RenderBackend_V2::vk_reflex_s::isSupported (void) const
{
  return
     SK_VK_Reflex.device    != 0                           &&
    (SK_VK_Reflex.swapchain != 0 || api == NvLowLatencyVk) &&
                                    api != None;
}

#undef VK_NV_low_latency2

bool
SK_RenderBackend_V2::vk_reflex_s::getLatencyReport (NV_LATENCY_RESULT_PARAMS* latencyReport) const
{
  if (SK_VK_Reflex.device == nullptr)
    return false;

  switch (api)
  {
    case NvLowLatencyVk:
    {
      NVLL_VK_LATENCY_RESULT_PARAMS                               report = { };
      if (NVLL_VK_OK == NvLL_VK_GetLatency (SK_VK_Reflex.device, &report))
      {
        for ( auto i = 0 ; i < 64 ; ++i )
        {
          latencyReport->frameReport [i].frameID                = report.frameReport [i].frameID;
          latencyReport->frameReport [i].inputSampleTime        = report.frameReport [i].inputSampleTime;
          latencyReport->frameReport [i].simStartTime           = report.frameReport [i].simStartTime;
          latencyReport->frameReport [i].simEndTime             = report.frameReport [i].simEndTime;
          latencyReport->frameReport [i].renderSubmitStartTime  = report.frameReport [i].renderSubmitStartTime;
          latencyReport->frameReport [i].renderSubmitEndTime    = report.frameReport [i].renderSubmitEndTime;
          latencyReport->frameReport [i].presentStartTime       = report.frameReport [i].presentStartTime;
          latencyReport->frameReport [i].presentEndTime         = report.frameReport [i].presentEndTime;
          latencyReport->frameReport [i].driverStartTime        = report.frameReport [i].driverStartTime;
          latencyReport->frameReport [i].driverEndTime          = report.frameReport [i].driverEndTime;
          latencyReport->frameReport [i].osRenderQueueStartTime = report.frameReport [i].osRenderQueueStartTime;
          latencyReport->frameReport [i].osRenderQueueEndTime   = report.frameReport [i].osRenderQueueEndTime;
          latencyReport->frameReport [i].gpuRenderStartTime     = report.frameReport [i].gpuRenderStartTime;
          latencyReport->frameReport [i].gpuRenderEndTime       = report.frameReport [i].gpuRenderEndTime;
          latencyReport->frameReport [i].gpuActiveRenderTimeUs  = static_cast <NvU32> (report.frameReport [i].gpuRenderEndTime - report.frameReport [i].gpuRenderStartTime);
          latencyReport->frameReport [i].gpuFrameTimeUs         = 0;
        }

        return true;
      }
    } break;

    case VK_NV_low_latency2:
    {
      extern VkDevice       SK_Reflex_VkDevice;
      extern VkSwapchainKHR SK_Reflex_VkSwapchain;
      extern VkSemaphore    SK_Reflex_VkSemaphore;
      extern VkInstance     SK_Reflex_VkInstance;

      VkLatencyTimingsFrameReportNV timings [64];

      VkGetLatencyMarkerInfoNV
        markerInfo             = {                                          };
        markerInfo.sType       = VK_STRUCTURE_TYPE_GET_LATENCY_MARKER_INFO_NV;
        markerInfo.timingCount = 64;
        markerInfo.pTimings    = &timings [0];

      extern PFN_vkGetLatencyTimingsNV vkGetLatencyTimingsNV_Original;

      vkGetLatencyTimingsNV_Original (SK_Reflex_VkDevice, SK_Reflex_VkSwapchain, &markerInfo);

      for ( auto i = 0u ; i < 64 ; ++i )
      {
        if (i >= markerInfo.timingCount)
          break;

        latencyReport->frameReport [i].frameID                = markerInfo.pTimings [i].presentID;
        latencyReport->frameReport [i].inputSampleTime        = markerInfo.pTimings [i].inputSampleTimeUs;
        latencyReport->frameReport [i].simStartTime           = markerInfo.pTimings [i].simStartTimeUs;
        latencyReport->frameReport [i].simEndTime             = markerInfo.pTimings [i].simEndTimeUs;
        latencyReport->frameReport [i].renderSubmitStartTime  = markerInfo.pTimings [i].renderSubmitStartTimeUs;
        latencyReport->frameReport [i].renderSubmitEndTime    = markerInfo.pTimings [i].renderSubmitEndTimeUs;
        latencyReport->frameReport [i].presentStartTime       = markerInfo.pTimings [i].presentStartTimeUs;
        latencyReport->frameReport [i].presentEndTime         = markerInfo.pTimings [i].presentEndTimeUs;
        latencyReport->frameReport [i].driverStartTime        = markerInfo.pTimings [i].driverStartTimeUs;
        latencyReport->frameReport [i].driverEndTime          = markerInfo.pTimings [i].driverEndTimeUs;
        latencyReport->frameReport [i].osRenderQueueStartTime = markerInfo.pTimings [i].osRenderQueueStartTimeUs;
        latencyReport->frameReport [i].osRenderQueueEndTime   = markerInfo.pTimings [i].osRenderQueueEndTimeUs;
        latencyReport->frameReport [i].gpuRenderStartTime     = markerInfo.pTimings [i].gpuRenderStartTimeUs;
        latencyReport->frameReport [i].gpuRenderEndTime       = markerInfo.pTimings [i].gpuRenderEndTimeUs;
        latencyReport->frameReport [i].gpuActiveRenderTimeUs  = static_cast <NvU32> (markerInfo.pTimings [i].gpuRenderEndTimeUs - markerInfo.pTimings [i].gpuRenderStartTimeUs);
        latencyReport->frameReport [i].gpuFrameTimeUs         = 0;
      }

      return true;
    } break;

    default:
      break;
  }

  return false;
}

bool
SK_RenderBackend_V2::vk_reflex_s::isPacingEligible (void) const
{
  return
    config.nvidia.reflex.vulkan && ReadULong64Acquire (&last_slept) > SK_GetFramesDrawn () - 3 &&

  // DLSS-G Pacing needs traditional frame history calculation
  !__SK_IsDLSSGActive;
}

bool
SK_RenderBackend_V2::vk_reflex_s::needsFallbackSleep (void) const
{
  return
    false;
    //config.nvidia.reflex.vulkan && config.nvidia.reflex.use_limiter && !isPacingEligible () && !__SK_IsDLSSGActive;
}

UINT
SK_Reflex_CalculateSleepMinIntervalForVulkan (bool bLowLatency)
{
  UINT reflex_interval = 0UL;

  const auto& rb =
    SK_GetCurrentRenderBackend ();

  const auto& display =
    rb.displays [rb.active_display];

  // Enforce upper-bound framerate limit on VRR displays when VSYNC is on,
  //   the same as D3D Reflex would.
  if ( rb.present_interval > 0 && bLowLatency &&
       display.nvapi.monitor_caps.data.caps.currentlyCapableOfVRR &&
       display.signal.timing.vsync_freq.Denominator != 0 )
  {
    const double dRefresh =
      static_cast <double> (display.signal.timing.vsync_freq.Numerator) /
      static_cast <double> (display.signal.timing.vsync_freq.Denominator);

    const double dReflexFPS =
      (dRefresh - (dRefresh * dRefresh) / 3600.0);

    // Set VRR framerate limit accordingly
    reflex_interval =
      static_cast <UINT> (1000000.0 / dReflexFPS);
  }

  const bool applyUserOverride =
    (__target_fps > 10.0f && (__SK_ForceDLSSGPacing || config.nvidia.reflex.use_limiter));

  if (applyUserOverride)
  {
    UINT interval =
      (UINT)(1000000.0 / __target_fps) + ( __SK_ForceDLSSGPacing ? 6 : 0 );

    // Vulkan Reflex is too primitive to perform this on its own, so we need to
    //   pre-adjust the limit.
    if (__SK_IsDLSSGActive)
      interval *= (__SK_DLSSGMultiFrameCount + 1);

    reflex_interval =
      reflex_interval == 0 ?           interval
                           : std::max (interval, reflex_interval);
  }

  // Throw away any intervals > 100 ms, parameters are suspect.
  if (reflex_interval > 100000)
      reflex_interval = 0;

  return
    reflex_interval;
}