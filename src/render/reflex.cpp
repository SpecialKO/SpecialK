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
#define __SK_SUBSYSTEM__ L"  Reflex  "

#include <reflex/pclstats.h>
PCLSTATS_DEFINE ();

volatile ULONG64 SK_Reflex_LastFrameMarked   = 0;
volatile LONG    SK_RenderBackend::flip_skip = 0;

using  NvAPI_QueryInterface_pfn       = void*                (*)(unsigned int ordinal);
using  NvAPI_D3D_SetLatencyMarker_pfn = NvAPI_Status (__cdecl *)(__in IUnknown                 *pDev,
                                                                 __in NV_LATENCY_MARKER_PARAMS *pSetLatencyMarkerParams);
static NvAPI_D3D_SetLatencyMarker_pfn
       NvAPI_D3D_SetLatencyMarker_Original = nullptr;

NVAPI_INTERFACE
NvAPI_D3D_SetLatencyMarker_Detour ( __in IUnknown                 *pDev,
                                    __in NV_LATENCY_MARKER_PARAMS *pSetLatencyMarkerParams )
{
  // Shutdown our own Reflex implementation
  if (! std::exchange (config.nvidia.sleep.native, true))
  {
    PCLSTATS_SHUTDOWN ();

    SK_LOG0 ( ( L"# Game is using NVIDIA Reflex natively..." ),
                L"  Reflex  " );
  }

  return
    NvAPI_D3D_SetLatencyMarker_Original (pDev, pSetLatencyMarkerParams);
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

      // Hook SetLatencyMarker so we know if a game is natively using Reflex.
      //
      SK_CreateFuncHook (      L"NvAPI_D3D_SetLatencyMarker",
                                 NvAPI_QueryInterface (D3D_SET_LATENCY_MARKER),
                                 NvAPI_D3D_SetLatencyMarker_Detour,
        static_cast_p2p <void> (&NvAPI_D3D_SetLatencyMarker_Original) );
      MH_EnableHook (            NvAPI_QueryInterface (D3D_SET_LATENCY_MARKER));
    }
  }
}

void
SK_PCL_Heartbeat (NV_LATENCY_MARKER_PARAMS marker)
{
  if (config.nvidia.sleep.native)
    return;

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
    const HWND kCurrentThreadId = (HWND)(-1);
          bool ping             = false;
          MSG  msg              = { };

    while (PeekMessage (&msg, kCurrentThreadId, g_PCLStatsWindowMessage, g_PCLStatsWindowMessage, PM_REMOVE))
      ping = true;

    if (ping)
    {
      static auto &rb =
        SK_GetCurrentRenderBackend ();

      rb.setLatencyMarkerNV (PC_LATENCY_PING);
    }
  }

  if (init)
  {
    PCLSTATS_MARKER ( marker.markerType,
                      marker.frameID );
  }
}

bool
SK_RenderBackend_V2::setLatencyMarkerNV (NV_LATENCY_MARKER_TYPE marker)
{
  NvAPI_Status ret =
    NVAPI_INVALID_CONFIGURATION;

  if (device.p    != nullptr &&
      swapchain.p != nullptr)
  {
    // Avert your eyes... time to check if Device and SwapChain are consistent
    //
    auto pDev11 = getDevice <ID3D11Device> ();
    auto pDev12 = getDevice <ID3D12Device> ();

    if (! (pDev11 || pDev12))
      return false; // Uh... what?

    SK_ComQIPtr <IDXGISwapChain> pSwapChain (swapchain.p);

    if (pSwapChain.p != nullptr)
    {
      SK_ComPtr                  <ID3D11Device>           pSwapDev11;
      pSwapChain->GetDevice ( IID_ID3D11Device, (void **)&pSwapDev11.p );
      SK_ComPtr                  <ID3D12Device>           pSwapDev12;
      pSwapChain->GetDevice ( IID_ID3D12Device, (void **)&pSwapDev12.p );
      if (! ((pDev11.p != nullptr && pDev11.IsEqualObject (pSwapDev11.p)) ||
             (pDev12.p != nullptr && pDev12.IsEqualObject (pSwapDev12.p))))
      {
        return false; // Nope, let's get the hell out of here!
      }
    }

    // Only do this if game is not Reflex native, or if the marker is a flash
    if (sk::NVAPI::nv_hardware && ((! config.nvidia.sleep.native) || marker == TRIGGER_FLASH))
    {
      NV_LATENCY_MARKER_PARAMS
        markerParams            = {                          };
        markerParams.version    = NV_LATENCY_MARKER_PARAMS_VER;
        markerParams.markerType = marker;
        markerParams.frameID    = static_cast <NvU64> (
             ReadULong64Acquire (&frames_drawn)       );

      SK_PCL_Heartbeat (markerParams);

      // SetLatencyMarker is hooked on the simulation marker of the first frame,
      //   we may have gotten here out-of-order.
      ret = NvAPI_D3D_SetLatencyMarker_Original == nullptr ? NVAPI_OK :
            NvAPI_D3D_SetLatencyMarker_Original (device.p, &markerParams);
    }

    if ( marker == RENDERSUBMIT_END /*||
         marker == RENDERSUBMIT_START*/ )
    {
      latency.submitQueuedFrame (
        SK_ComQIPtr <IDXGISwapChain1> (pSwapChain)
      );
    }
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

  // Game has native Reflex, we should bail out.
  if (config.nvidia.sleep.native)
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

    if ( lastParams.bLowLatencyBoost      != sleepParams.bLowLatencyBoost  ||
         lastParams.bLowLatencyMode       != sleepParams.bLowLatencyMode   ||
         lastParams.minimumIntervalUs     != sleepParams.minimumIntervalUs ||
         lastParams.bUseMarkersToOptimize != sleepParams.bUseMarkersToOptimize )
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

    if ( NVAPI_OK != NvAPI_D3D_Sleep (device.p) )
      valid = false;

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
        static NV_GET_ADAPTIVE_SYNC_DATA
                     getAdaptiveSync   = { };

        static DWORD lastChecked       = 0;
        static NvU64 lastFlipTimeStamp = 0;
        static NvU64 lastFlipFrame     = 0;
        static double   dFlipPrint     = 0.0;

        if (SK_timeGetTime () > lastChecked + 333)
        {                       lastChecked = SK_timeGetTime ();

          ZeroMemory (&getAdaptiveSync,  sizeof (NV_GET_ADAPTIVE_SYNC_DATA));
                       getAdaptiveSync.version = NV_GET_ADAPTIVE_SYNC_DATA_VER;

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
                static_cast <double> (SK_QpcFreq) );

                 dFlipPrint = dFlipRate;
              lastFlipFrame = SK_GetFramesDrawn ();
            }

            rb.gsync_state.update (true);
          }

          else lastChecked = SK_timeGetTime () + 333;
        }

        ImGui::Text       ("Adaptive Sync Status for %hs", SK_WideCharToUTF8 (rb.display_name).c_str ());
        ImGui::Separator  ();
        ImGui::BeginGroup ();
        ImGui::Text       ("Current State:");
        if (! getAdaptiveSync.bDisableAdaptiveSync)
        {
          ImGui::Text     ("Frame Splitting:");

          if (getAdaptiveSync.maxFrameInterval != 0)
            ImGui::Text   ("Minimum Refresh:");
                           
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
            ImGui::Text   ( "%#6.2f Hz ",
                             1000000.0 / static_cast <double> (getAdaptiveSync.maxFrameInterval) );
        }
        ImGui::Text       ( "" );

      //ImGui::Text       ( "%#6.2f Hz ", dFlipPrint);
      //ImGui::Text       ( "\t\t\t\t\t\t\t\t" );
        
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
        ImGui::SameLine   ();
        ImGui::BeginGroup ();
        if (ImGui::Button ("Trigger Reflex Flash"))
        {
          rb.setLatencyMarkerNV (TRIGGER_FLASH);
        }
        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("Requires the GeForce Experience overlay and a Reflex Latency Analysis capable display");
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