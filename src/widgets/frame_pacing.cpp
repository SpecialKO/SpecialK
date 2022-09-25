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

extern iSK_INI* osd_ini;

extern void SK_ImGui_DrawGraph_FramePacing (void);

#include <SpecialK/render/d3d11/d3d11_core.h>
#include <imgui/font_awesome.h>


namespace
SK_ImGui
{
  bool BatteryMeter (void);
};


static bool has_battery   = false;
static bool debug_limiter = false;

struct SK_ImGui_FramePercentiles
{
  bool  display             =  true;
  bool  display_above       =  true;
  bool  display_most_recent =  true;
  int   seconds_to_sample   =     0; // Unlimited

  struct
  {
    sk::ParameterBool*  display             = nullptr;
    sk::ParameterBool*  display_above       = nullptr;
    sk::ParameterBool*  display_most_recent = nullptr;
    sk::ParameterInt*   seconds_to_sample   = nullptr;
    sk::ParameterFloat* percentile0_cutoff  = nullptr;
    sk::ParameterFloat* percentile1_cutoff  = nullptr;
  } percentile_cfg;

  void load_percentile_cfg (void)
  {
    assert (osd_ini != nullptr);

    percentile_cfg.display =
      dynamic_cast <sk::ParameterBool *> (
        SK_Widget_ParameterFactory->create_parameter <bool> (
          L"Show Statistics"
        )
      );

    percentile_cfg.display_above =
      dynamic_cast <sk::ParameterBool *> (
        SK_Widget_ParameterFactory->create_parameter <bool> (
          L"Draw Statistics Above Line Graph"
        )
      );

    percentile_cfg.display_most_recent =
      dynamic_cast <sk::ParameterBool *> (
        SK_Widget_ParameterFactory->create_parameter <bool> (
          L"Limit sample dataset to the past ~30 seconds at most."
        )
      );

    percentile_cfg.percentile0_cutoff =
      dynamic_cast <sk::ParameterFloat *> (
        SK_Widget_ParameterFactory->create_parameter <float> (
          L"Boundary that defines the percentile being profiled."
        )
      );

    percentile_cfg.percentile1_cutoff =
      dynamic_cast <sk::ParameterFloat *> (
        SK_Widget_ParameterFactory->create_parameter <float> (
          L"Boundary that defines the percentile being profiled."
        )
      );

    percentile_cfg.display->register_to_ini ( osd_ini,
      L"Widget.FramePacing", L"DisplayPercentiles"
    );

    percentile_cfg.display_above->register_to_ini ( osd_ini,
      L"Widget.FramePacing", L"PercentilesAboveGraph"
    );
    percentile_cfg.display_most_recent->register_to_ini ( osd_ini,
      L"Widget.FramePacing", L"ShortTermPercentiles"
    );
    percentile_cfg.percentile0_cutoff->register_to_ini ( osd_ini,
      L"Widget.FramePacing", L"Percentile[0].Cutoff"
    );
    percentile_cfg.percentile1_cutoff->register_to_ini ( osd_ini,
      L"Widget.FramePacing", L"Percentile[1].Cutoff"
    );

    percentile_cfg.display->load             (display);
    percentile_cfg.display_above->load       (display_above);
    percentile_cfg.display_most_recent->load (display_most_recent);

    if (! percentile_cfg.percentile0_cutoff->load  (percentile0.cutoff))
          percentile0.cutoff = 1.0f; // 1.0%

    if (! percentile_cfg.percentile1_cutoff->load  (percentile1.cutoff))
          percentile1.cutoff = 0.1f; // 0.1%

    static auto* cp =
      SK_GetCommandProcessor ();

    SK_RunOnce (
    {
      cp->AddVariable ("FramePacing.DisplayPercentiles",    SK_CreateVar (SK_IVariable::Boolean, &display));
      cp->AddVariable ("FramePacing.PercentilesAboveGraph", SK_CreateVar (SK_IVariable::Boolean, &display_above));
      cp->AddVariable ("FramePacing.ShortTermPercentiles",  SK_CreateVar (SK_IVariable::Boolean, &display_most_recent));
      cp->AddVariable ("FramePacing.Percentile[0].Cutoff",  SK_CreateVar (SK_IVariable::Float,   &percentile0.cutoff));
      cp->AddVariable ("FramePacing.Percentile[1].Cutoff",  SK_CreateVar (SK_IVariable::Float,   &percentile1.cutoff));
    });
  }

  void store_percentile_cfg (void)
  {
    percentile_cfg.display->store             (display);
    percentile_cfg.display_above->store       (display_above);
    percentile_cfg.display_most_recent->store (display_most_recent);
    percentile_cfg.percentile0_cutoff->store  (percentile0.cutoff);
    percentile_cfg.percentile1_cutoff->store  (percentile1.cutoff);
  }

  struct sample_subset_s
  {
    float   cutoff          =  0.0f;
    bool    has_data        = false;
    float   computed_fps    =  0.0f;
    ULONG64 last_calculated =   0UL;

    void computeFPS (long double dt)
    {
      computed_fps =
        (float)(1000.0L / dt);

      last_calculated =
        SK_GetFramesDrawn ();
    }
  } percentile0,
    percentile1, mean;

  void toggleDisplay (void)
  {
    display = (! display);

    store_percentile_cfg ();
  }
};

SK_LazyGlobal <SK_ImGui_FramePercentiles> SK_FramePercentiles;

float SK_Framerate_GetPercentileByIdx (int idx)
{
  if (idx == 0)
    return SK_FramePercentiles->percentile0.cutoff;

  if (idx == 1)
    return SK_FramePercentiles->percentile1.cutoff;

  return 0.0f;
}

extern float __target_fps;
extern float __target_fps_bg;

class SK_ImGui_FrameHistory : public SK_Stat_DataHistory <float, 120>
{
public:
  void timeFrame       (double seconds)
  {
    addValue ((float)(1000.0 * seconds));
  }
};

SK_LazyGlobal <SK_ImGui_FrameHistory> SK_ImGui_Frames;


#include <dwmapi.h>

HRESULT
WINAPI
SK_DWM_GetCompositionTimingInfo (DWM_TIMING_INFO *pTimingInfo)
{
  static HMODULE hModDwmApi =
    SK_LoadLibraryW (L"dwmapi.dll");

  typedef HRESULT (WINAPI *DwmGetCompositionTimingInfo_pfn)(
                   HWND             hwnd,
                   DWM_TIMING_INFO *pTimingInfo);

  static                   DwmGetCompositionTimingInfo_pfn
                           DwmGetCompositionTimingInfo =
         reinterpret_cast <DwmGetCompositionTimingInfo_pfn> (
    SK_GetProcAddress ( hModDwmApi,
                          "DwmGetCompositionTimingInfo" )   );

  return
    DwmGetCompositionTimingInfo ( 0, pTimingInfo );
}

SK_RenderBackend::latency_monitor_s SK_RenderBackend::latency;

void
SK_RenderBackend::latency_monitor_s::submitQueuedFrame (IDXGISwapChain1* pSwapChain)
{
  if (pSwapChain == nullptr)
  {
    stale = true;
    return;
  }

  if (              latency.counters.lastFrame !=
     std::exchange (latency.counters.lastFrame, SK_GetFramesDrawn ()) )
  {
    DXGI_SWAP_CHAIN_DESC  swapDesc = { };
    pSwapChain->GetDesc (&swapDesc);

    if ( SK_DXGI_IsFlipModelSwapChain (swapDesc)
     && SUCCEEDED (pSwapChain->GetFrameStatistics  (&latency.counters.frameStats1)) )
    {              pSwapChain->GetLastPresentCount (&latency.counters.lastPresent);

      SK_RunOnce (latency.counters.frameStats0 = latency.counters.frameStats1);

      LARGE_INTEGER time;
                    time.QuadPart =
        ( latency.counters.frameStats1.SyncQPCTime.QuadPart -
          latency.counters.frameStats0.SyncQPCTime.QuadPart );

      latency.delays.PresentQueue =
        ( latency.counters.lastPresent -
          latency.counters.frameStats1.PresentCount );

      latency.delays.SyncDelay    =
        ( latency.counters.frameStats1.SyncRefreshCount -
          latency.counters.frameStats0.SyncRefreshCount );

      latency.delays .TotalMs     =
                  static_cast <float>  (
         1000.0 * static_cast <double> (time.QuadPart) /
                  static_cast <double> (   SK_QpcFreq) ) -
           SK_ImGui_Frames->getLastValue ();

      latency.counters.frameStats0 = latency.counters.frameStats1;

      stale = false;
    }

    else stale = true;

    latency.stats.History [latency.counters.lastFrame % std::size (latency.stats.History)] =
      fabs (latency.delays.TotalMs);
  }
}


extern int
SK_ImGui_DrawGamepadStatusBar (void);

       int    extra_status_line = 0;

extern int  SK_PresentMon_Main (int argc, char **argv);
extern bool StopTraceSession   (void);

HANDLE __SK_ETW_PresentMon_Thread = INVALID_HANDLE_VALUE;

void
SK_SpawnPresentMonWorker (void)
{
  // Wine doesn't support this...
  if (config.compatibility.using_wine)
    return;

  // Workaround for Windows 11 22H2 performance issues
  if (! config.render.framerate.enable_etw_tracing)
    return;

  SK_RunOnce (
    __SK_ETW_PresentMon_Thread =
      SK_Thread_CreateEx ( [](LPVOID) -> DWORD
      {
        SK_PresentMon_Main   (0, nullptr);
	      if (StopTraceSession (          ))
          __SK_ETW_PresentMon_Thread = 0;

        return 0;
      }, L"[SK] PresentMon Lite",
    (LPVOID)ULongToPtr (GetCurrentProcessId ())
    )
  );
}

bool
SK_ETW_EndTracing (void)
{
  LONG PresentMon_ETW =
    HandleToLong (__SK_ETW_PresentMon_Thread);

  if (PresentMon_ETW > 0L)
  {
    DWORD dwWaitState =
         SK_WaitForSingleObject (
           LongToHandle (PresentMon_ETW), 500UL );

    if ( dwWaitState == WAIT_OBJECT_0 ||
         dwWaitState == WAIT_TIMEOUT )
    {
      return
        StopTraceSession () || (__SK_ETW_PresentMon_Thread == 0);
    }

    SK_CloseHandle (
       LongToHandle (PresentMon_ETW)
         ); __SK_ETW_PresentMon_Thread = 0;
  }

  return
    (PresentMon_ETW <= 0L);
}

char          SK_PresentDebugStr [2][256] = { "", "" };
volatile LONG SK_PresentIdx               =          0;

extern void SK_ImGui_DrawFramePercentiles (void);

void
SK_ImGui_DrawGraph_FramePacing (void)
{
  static const auto& io =
    ImGui::GetIO ();

  const  float font_size           =
    ( ImGui::GetFont  ()->FontSize * io.FontGlobalScale );

#ifdef _ProperSpacing
  const  float font_size_multiline =
    ( ImGui::GetStyle ().ItemSpacing.y      +
      ImGui::GetStyle ().ItemInnerSpacing.y + font_size );
#endif


  float sum = 0.0f;

  float min = FLT_MAX;
  float max = 0.0f;

  for ( const auto& val : SK_ImGui_Frames->getValues () )
  {
    sum += val;

    if (val > max)
      max = val;

    if (val < min)
      min = val;
  }

  static       char szAvg [512] = { };
  static const bool ffx = SK_GetModuleHandle (L"UnX.dll") != nullptr;

  float& target =
    ( SK_IsGameWindowActive () || __target_fps_bg == 0.0f ) ?
                  __target_fps  : __target_fps_bg;

  float target_frametime = ( target == 0.0f ) ?
                           ( 1000.0f   / (ffx ? 30.0f : 60.0f) ) :
                             ( 1000.0f / fabs (target) );

  static auto& rb =
    SK_GetCurrentRenderBackend ();

  if (target == 0.0f && (! ffx))
  {
    target_frametime = (float)(1000.0 /
      rb.windows.device.getDevCaps ().res.refresh);
  }

  float frames =
    std::min ( (float)SK_ImGui_Frames->getUpdates  (),
               (float)SK_ImGui_Frames->getCapacity () );


  if (ffx)
  {
    // Normal Gameplay: 30 FPS
    if (sum / frames > 30.0)
      target_frametime = 33.333333f;

    // Menus: 60 FPS
    else
      target_frametime = 16.666667f;
  }

  extra_status_line = 0;

  SK_SpawnPresentMonWorker ();

  auto presentStrIdx =
    ReadAcquire (&SK_PresentIdx);

  if (*SK_PresentDebugStr [presentStrIdx] != '\0')
  {
    extra_status_line = 1;

    ImGui::TextUnformatted (
      SK_PresentDebugStr [presentStrIdx]
    );

    ImGui::SameLine ();
  }

  if (SK_ImGui_DrawGamepadStatusBar () > 0)
    extra_status_line = 1;

  //extern HRESULT SK_D3DKMT_QueryAdapterInfo (D3DKMT_QUERYADAPTERINFO *pQueryAdapterInfo);
  //
  //D3DKMT_QUERYADAPTERINFO
  //       queryAdapterInfo                       = { };
  //       queryAdapterInfo.AdapterHandle         = rb.adapter.d3dkmt;
  //       queryAdapterInfo.Type                  = KMTQAITYPE_ADAPTERPERFDATA;
  //       queryAdapterInfo.PrivateDriverData     = &rb.adapter.perf.data;
  //       queryAdapterInfo.PrivateDriverDataSize = sizeof (D3DKMT_ADAPTER_PERFDATA);
  //
  //if (SUCCEEDED (SK_D3DKMT_QueryAdapterInfo (&queryAdapterInfo)))
  //{
  //  memcpy ( &rb.adapter.perf.data, queryAdapterInfo.PrivateDriverData,
  //                std::min ((size_t)queryAdapterInfo.PrivateDriverDataSize,
  //                                     sizeof (D3DKMT_ADAPTER_PERFDATA)) );
  //
  //  rb.adapter.perf.sampled_frame =
  //                    SK_GetFramesDrawn ();
  //}

#if 0
  if (rb.adapter.d3dkmt != 0)
  {
    if (rb.adapter.perf.sampled_frame < SK_GetFramesDrawn () - 20)
    {   rb.adapter.perf.sampled_frame = 0;

      extern HRESULT SK_D3DKMT_QueryAdapterInfo (D3DKMT_QUERYADAPTERINFO *pQueryAdapterInfo);

      D3DKMT_QUERYADAPTERINFO
             queryAdapterInfo                       = { };
             queryAdapterInfo.AdapterHandle         = rb.adapter.d3dkmt;
             queryAdapterInfo.Type                  = KMTQAITYPE_ADAPTERPERFDATA;
             queryAdapterInfo.PrivateDriverData     = &rb.adapter.perf.data;
             queryAdapterInfo.PrivateDriverDataSize = sizeof (D3DKMT_ADAPTER_PERFDATA);

      if (SUCCEEDED (SK_D3DKMT_QueryAdapterInfo (&queryAdapterInfo)))
      {
        memcpy ( &rb.adapter.perf.data, queryAdapterInfo.PrivateDriverData,
                      std::min ((size_t)queryAdapterInfo.PrivateDriverDataSize,
                                           sizeof (D3DKMT_ADAPTER_PERFDATA)) );

        rb.adapter.perf.sampled_frame =
                          SK_GetFramesDrawn ();
      }
    }
  }

  if (rb.adapter.perf.sampled_frame != 0)
  {
    ImGui::SameLine ();
    ImGui::Text ("\tPower Draw: %4.1f%%", static_cast <double> (rb.adapter.perf.data.Power) / 10.0);
  }
#endif

  if (SK_FramePercentiles->display_above)
      SK_ImGui_DrawFramePercentiles ();

  if (! SK_RenderBackend_V2::latency.stale)
  {
    SK_RenderBackend_V2::latency.stats.MaxMs =
      *std::max_element ( std::begin (SK_RenderBackend_V2::latency.stats.History),
                          std::end   (SK_RenderBackend_V2::latency.stats.History) );
                       SK_RenderBackend_V2::latency.stats.ScaleMs =
     std::max ( 99.0f, SK_RenderBackend_V2::latency.stats.MaxMs );

    // ...

    SK_RenderBackend_V2::latency.stats.AverageMs =
      std::accumulate (
        std::begin ( SK_RenderBackend_V2::latency.stats.History ),
        std::end   ( SK_RenderBackend_V2::latency.stats.History ), 0.0f )
      / std::size  ( SK_RenderBackend_V2::latency.stats.History );

    snprintf
      ( szAvg,
          511, (const char *)
          u8"Avg milliseconds per-frame: %6.3f  (Target: %6.3f)\n"
          u8"         Render latency:           %lu Frame%s | %3.1f / %3.1f ms |  %lu Hz\n\n\n\n"
          u8"Variation:  %8.5f ms        %.1f FPS  ±  %3.1f frames",
              sum / frames,
                target_frametime,
                    SK_RenderBackend_V2::latency.delays.PresentQueue,
                    SK_RenderBackend_V2::latency.delays.PresentQueue != 1 ?
                                                                     "s " : "  ",
                      SK_RenderBackend_V2::latency.stats.AverageMs,
                      SK_RenderBackend_V2::latency.stats.MaxMs,
                      SK_RenderBackend_V2::latency.delays.SyncDelay,
            (double)max - (double)min,
                    1000.0f / (sum / frames),
                      ((double)max-(double)min)/(1000.0f/(sum/frames)) );
  }

  else
  {
    snprintf
      ( szAvg,
          511, (const char *)
          u8"Avg milliseconds per-frame: %6.3f  (Target: %6.3f)\n"
          u8"    Extreme frame times:     %6.3f min, %6.3f max\n\n\n\n"
          u8"Variation:  %8.5f ms        %.1f FPS  ±  %3.1f frames",
              sum / frames,
                target_frametime,
                  min, max,
            (double)max - (double)min,
                    1000.0f / (sum / frames),
                      ((double)max-(double)min)/(1000.0f/(sum/frames)) );
  }

  // @TODO
  ///float fGPULoad =
  ///  SK_GPU_GetGPULoad (0);
  ///
  ///ImGui::PushItemFlag (ImGuiItemFlags_Disabled, true);
  ///ImGui::VSliderFloat ( "GPU", ImVec2 (font_size * 1.5f, -1.0f),
  ///                     &fGPULoad,      0.0f, 100.0f,
  ///                      "%5.3f%%" );
  ///ImGui::PopItemFlag  ();
  ///ImGui::SameLine     ();
  ImGui::PushStyleColor ( ImGuiCol_PlotLines,
                             ImColor::HSV ( 0.31f - 0.31f *
                     std::min ( 1.0f, (max - min) / (2.0f * target_frametime) ),
                                             1.0f,   1.0f ) );

  const ImVec2 border_dims (
    std::max (500.0f, ImGui::GetContentRegionAvailWidth ()),
      font_size * 7.0f
  );

  float fX = ImGui::GetCursorPosX ();

  ///float fMax = std::max ( 99.0f,
  ///  *std::max_element ( std::begin (fLatencyHistory),
  ///                      std::end   (fLatencyHistory) )
  ///                      );

  ImGui::PushStyleColor (ImGuiCol_PlotHistogram, ImVec4 (.66f, .66f, .66f, .75f));
  ImGui::PlotHistogram ( SK_ImGui_Visible ? "###ControlPanel_LatencyHistogram" :
                                            "###Floating_LatencyHistogram",
                           SK_RenderBackend_V2::latency.stats.History,
             IM_ARRAYSIZE (SK_RenderBackend_V2::latency.stats.History),
                               SK_GetFramesDrawn () % 120,
                                 "",
                                   0,
                           SK_RenderBackend_V2::latency.stats.ScaleMs,
                                      border_dims );
  ImGui::PopStyleColor  ();

  ImGui::SameLine ();
  ImGui::SetCursorPosX (fX);

  ImGui::PlotLines ( SK_ImGui_Visible ? "###ControlPanel_FramePacing" :
                                        "###Floating_FramePacing",
                       SK_ImGui_Frames->getValues     ().data (),
      static_cast <int> (frames),
                           SK_ImGui_Frames->getOffset (),
                             szAvg,
                               -.1f,
                                 2.0f * target_frametime + 0.1f,
                                   border_dims );

  ImGui::PopStyleColor ();


  // Only toggle when clicking the graph and percentiles are off,
  //   to turn them back off, click the progress bars.
  if ((! SK_FramePercentiles->display) && ImGui::IsItemClicked ())
  {
    SK_FramePercentiles->toggleDisplay ();
  }


  //SK_RenderBackend& rb =
  //  SK_GetCurrentRenderBackend ();
  //
  //if (sk::NVAPI::nv_hardware && config.apis.NvAPI.gsync_status)
  //{
  //  if (rb.gsync_state.capable)
  //  {
  //    ImGui::SameLine ();
  //    ImGui::TextColored (ImColor::HSV (0.226537f, 1.0f, 0.36f), "G-Sync: ");
  //    ImGui::SameLine ();
  //
  //    if (rb.gsync_state.active)
  //    {
  //      ImGui::TextColored (ImColor::HSV (0.226537f, 1.0f, 0.45f), "Active");
  //    }
  //
  //
  //        else
  //    {
  //      ImGui::TextColored (ImColor::HSV (0.226537f, 0.75f, 0.27f), "Inactive");
  //    }
  //  }
  //}


  if (! SK_FramePercentiles->display_above)
        SK_ImGui_DrawFramePercentiles ();


#if 0
  DWM_TIMING_INFO dwmTiming        = {                      };
                  dwmTiming.cbSize = sizeof (DWM_TIMING_INFO);

  HRESULT hr =
    SK_DWM_GetCompositionTimingInfo (&dwmTiming);

  if ( SUCCEEDED (hr) )
  {
    ImGui::Text ( "Refresh Rate: %6.2f Hz",
                    static_cast <double> (dwmTiming.rateRefresh.uiNumerator) /
                    static_cast <double> (dwmTiming.rateRefresh.uiDenominator)
                );

    LONGLONG llPerfFreq =
      SK_GetPerfFreq ().QuadPart;

    LONGLONG qpcAtNextVBlank =
         dwmTiming.qpcVBlank;
    double   dMsToNextVBlank =
      1000.0 *
        ( static_cast <double> ( qpcAtNextVBlank - SK_QueryPerf ().QuadPart ) /
          static_cast <double> ( llPerfFreq ) );

    extern LONG64 __SK_VBlankLatency_QPCycles;

    static double uS =
      static_cast <double> ( llPerfFreq ) / ( 1000.0 * 1000.0 );

    ImGui::Text ( "ToNextVBlank:  %f ms", dMsToNextVBlank );
    ImGui::Text ( "VBlankLatency: %f us", __SK_VBlankLatency_QPCycles * uS );
  }
#endif
  //
  //  ImGui::Text ( "Composition Rate: %f",
  //                  static_cast <double> (dwmTiming.rateCompose.uiNumerator) /
  //                  static_cast <double> (dwmTiming.rateCompose.uiDenominator)
  //              );
  //
  //  ImGui::Text ( "DWM Refreshes (%llu), D3D Refreshes (%lu)", dwmTiming.cRefresh, dwmTiming.cDXRefresh );
  //  ImGui::Text ( "D3D Presents (%lu)",                                            dwmTiming.cDXPresent );
  //
  //  ImGui::Text ( "DWM Glitches (%llu)",                                           dwmTiming.cFramesLate );
  //  ImGui::Text ( "DWM Queue Length (%lu)",                                        dwmTiming.cFramesOutstanding );
  //  ImGui::Text ( "D3D Queue Length (%lu)", dwmTiming.cDXPresentSubmitted -
  //                                          dwmTiming.cDXPresentConfirmed );
  //}
}

void
SK_ImGui_DrawFramePercentiles (void)
{
  if (! SK_FramePercentiles->display)
    return;

  auto pLimiter =
    SK::Framerate::GetLimiter (
      SK_GetCurrentRenderBackend ().swapchain.p,
      false
    );

  if (! pLimiter)
    return;

  auto& snapshots =
    pLimiter->frame_history_snapshots;

  // FIXME: We may have more than one limiter, but percentiles are currently single
  //
  static auto& percentile0 = SK_FramePercentiles->percentile0;
  static auto& percentile1 = SK_FramePercentiles->percentile1;
  static auto&        mean = SK_FramePercentiles->mean;

  bool& show_immediate =
    SK_FramePercentiles->display_most_recent;

  static constexpr LARGE_INTEGER all_samples = { 0ULL };

  auto frame_history =
    pLimiter->frame_history.getPtr ();

  long double data_timespan = ( show_immediate ?
            frame_history->calcDataTimespan () :
           snapshots.mean->calcDataTimespan () );

  ImGui::PushStyleColor (ImGuiCol_Text,           (unsigned int)ImColor (255, 255, 255));
  ImGui::PushStyleColor (ImGuiCol_FrameBg,        (unsigned int)ImColor ( 0.3f,  0.3f,  0.3f, 0.7f));
  ImGui::PushStyleColor (ImGuiCol_FrameBgHovered, (unsigned int)ImColor ( 0.6f,  0.6f,  0.6f, 0.8f));
  ImGui::PushStyleColor (ImGuiCol_FrameBgActive,  (unsigned int)ImColor ( 0.9f,  0.9f,  0.9f, 0.9f));

  ////if ( SK_FramePercentiles->percentile0.last_calculated !=
  ////     SK_GetFramesDrawn ()
  ////   )
  {
   //const long double   SAMPLE_SECONDS = 5.0L;

    percentile0.computeFPS (                             show_immediate ?
      frame_history->calcPercentile   (percentile0.cutoff, all_samples) :
        snapshots.percentile0->calcMean                   (all_samples) );

    percentile1.computeFPS (                             show_immediate ?
      frame_history->calcPercentile   (percentile1.cutoff, all_samples) :
        snapshots.percentile1->calcMean                   (all_samples) );

    mean.computeFPS ( show_immediate          ?
        frame_history->calcMean (all_samples) :
       snapshots.mean->calcMean (all_samples) );
  }

  if ( std::isnormal (percentile0.computed_fps) ||
        (! show_immediate)
     )
  {
    float p0_ratio =
        percentile0.computed_fps / mean.computed_fps;

    if (! std::isnormal (percentile0.computed_fps))
    {
      mean.computeFPS (
        frame_history->calcMean (all_samples)
      );
      percentile0.computeFPS (
        frame_history->calcPercentile (percentile0.cutoff, all_samples)
      );

      p0_ratio      =
        percentile0.computed_fps / mean.computed_fps;

      data_timespan =
        frame_history->calcDataTimespan ();
    }

    percentile0.has_data = true;

    static constexpr float  luminance  =   0.5f;
    static constexpr ImVec4 white_color (  1.0f,   1.0f,   1.0f, 1.0f);
    static constexpr ImVec4 label_color (0.595f, 0.595f, 0.595f, 1.0f);
    static constexpr ImVec4 click_color (0.685f, 0.685f, 0.685f, 1.0f);

    ImGui::BeginGroup ();

    if (data_timespan > 0.0)
    {
    //ImGui::Text ("Sample Size");

      ImGui::PushStyleColor  (ImGuiCol_Text, white_color);
           if (data_timespan > 60.0 * 60.0)  ImGui::Text ("%7.3f", data_timespan / (60.0 * 60.0));
      else if (data_timespan > 60.0)         ImGui::Text ("%6.2f", data_timespan / (60.0       ));
      else                                   ImGui::Text ("%5.1f", data_timespan);

      ImGui::SameLine ();

      ImGui::PushStyleColor  (ImGuiCol_Text, label_color);

           if (data_timespan > 60.0 * 60.0)  ImGui::Text ("Hours");
      else if (data_timespan > 60.0)         ImGui::Text ("Minutes");
      else                                   ImGui::Text ("Seconds");

      ImGui::PopStyleColor (2);
    }

    if (percentile1.has_data && data_timespan > 0.0)
    {
      ImGui::Spacing          ();
      ImGui::Spacing          ();
      ImGui::PushStyleColor  (ImGuiCol_Text, label_color);
      ImGui::TextUnformatted ("AvgFPS: "); ImGui::SameLine ();
      ImGui::PushStyleColor  (ImGuiCol_Text, white_color);
      ImGui::Text            ("%4.1f", mean.computed_fps);
      ImGui::PopStyleColor   (2);
    }

    ImGui::EndGroup   ();

    if (ImGui::IsItemHovered ( ))
    {
      ImGui::BeginTooltip    ( );
      ImGui::BeginGroup      ( );
      ImGui::PushStyleColor  (ImGuiCol_Text, click_color);
      ImGui::BulletText      ("Left-Click");
      ImGui::BulletText      ("Ctrl+Click");
      ImGui::PopStyleColor   ( );
      ImGui::EndGroup        ( );
      ImGui::SameLine        ( ); ImGui::Spacing ();
      ImGui::SameLine        ( ); ImGui::Spacing ();
      ImGui::SameLine        ( );
      ImGui::BeginGroup      ( );
      ImGui::PushStyleColor  (ImGuiCol_Text, white_color);
      ImGui::TextUnformatted ("Change Datasets (Long / Short)");
      ImGui::TextUnformatted ("Reset Long-term Statistics");
      ImGui::PopStyleColor   ( );
      ImGui::EndGroup        ( );
      ImGui::EndTooltip      ( );
    }

    auto& io =
      ImGui::GetIO ();

    if (ImGui::IsItemClicked ())
    {
      if (! io.KeyCtrl)
      {
        show_immediate = (! show_immediate);
        SK_FramePercentiles->store_percentile_cfg ();
      }

      else snapshots.reset ();
    }

    unsigned int     p0_color  (
      ImColor::HSV ( p0_ratio * 0.278f,
              0.88f, luminance )
                   );

    static char      p0_txt          [64] = { };
    std::string_view p0_view (p0_txt, 64);
         size_t      p0_len      =
      SK_FormatStringView ( p0_view,
                              "%3.1f%% Low FPS: %5.2f",
                                   percentile0.cutoff,
                                   percentile0.computed_fps );

      p0_txt [ std::max ((size_t)0,
               std::min ((size_t)64, p0_len)) ] = '\0';

    ImGui::SameLine       ( );
    ImGui::BeginGroup     ( );
    ImGui::PushStyleColor ( ImGuiCol_PlotHistogram,
                            p0_color                 );
    ImGui::ProgressBar    ( p0_ratio, ImVec2 ( -1.0f,
                                                0.0f ),
                            p0_txt );

    if ( data_timespan > 0.0                   &&
         std::isnormal ( percentile1.computed_fps )
       )
    { percentile1.has_data = true;

      float p1_ratio =
          percentile1.computed_fps / mean.computed_fps;

      unsigned       p1_color  (
      ImColor::HSV ( p1_ratio * 0.278f,
              0.88f, luminance )
                   );

        static char      p1_txt          [64] = { };
        std::string_view p1_view (p1_txt, 64);
             size_t      p1_len      =
        SK_FormatStringView ( p1_view,
                                "%3.1f%% Low FPS: %5.2f",
                                  percentile1.cutoff,
                                  percentile1.computed_fps );

      p1_txt [ std::max ((size_t)0,
               std::min ((size_t)64, p1_len)) ] = '\0';

      ImGui::PushStyleColor ( ImGuiCol_PlotHistogram,
                              p1_color                 );
      ImGui::ProgressBar    ( p1_ratio, ImVec2 ( -1.0f,
                                                  0.0f ),
                              p1_txt );
      ImGui::PopStyleColor  (2);
    }

    else
    {
      percentile1.has_data = false;
      ImGui::PopStyleColor  (1);
    }

    ImGui::EndGroup         ( );

    if (SK_FramePercentiles->display)
    {
      if (ImGui::IsItemClicked ())
        SK_FramePercentiles->toggleDisplay ();
    }
  }

  else
  {
    percentile0.has_data = false;
    percentile1.has_data = false;
  }

  ImGui::PopStyleColor      (4);
}

void
SK_ImGui_DrawFCAT (void)
{
  ImGui::SetNextWindowSize (ImGui::GetIO ().DisplaySize, ImGuiCond_Always);
  ImGui::SetNextWindowPos  (ImVec2 (0.0f, 0.0f),         ImGuiCond_Always);

  static constexpr auto flags =
    ImGuiWindowFlags_NoTitleBar        | ImGuiWindowFlags_NoResize           | ImGuiWindowFlags_NoMove                | ImGuiWindowFlags_NoScrollbar     |
    ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse         | ImGuiWindowFlags_NoBackground          | ImGuiWindowFlags_NoSavedSettings |
    ImGuiWindowFlags_NoMouseInputs     | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavInputs     |
    ImGuiWindowFlags_NoNavFocus        | ImGuiWindowFlags_NoNav              | ImGuiWindowFlags_NoDecoration          | ImGuiWindowFlags_NoInputs;

  ImGui::Begin ( "###FCAT", nullptr,
                    flags );

  DWORD dwNow = SK_GetCurrentMS ();

  const ImVec4 col =
    ImColor::HSV (  ( (SK_GetFramesDrawn () & 0x1) == 0 ?        (static_cast <float>(dwNow % 250) / 250.0f)
                                                        : 1.0f - (static_cast <float>(dwNow % 250) / 250.0f) ),
                                                          1.0f,
                                                          0.8f );

  const ImU32 col32 =
     ImColor (col);

  ImDrawList* draw_list =
    ImGui::GetWindowDrawList ();

  ImVec2 xy0, xy1;
  ImVec2 xy2, xy3;
  ImVec2 xy4, xy5;
  ImVec2 xy6, xy7;

  static auto& io =
    ImGui::GetIO ();

  xy0 = ImVec2 ( io.DisplaySize.x - io.DisplaySize.x * 0.01f, 0.0f             );
  xy1 = ImVec2 ( io.DisplaySize.x - io.DisplaySize.x * 0.01f, io.DisplaySize.y );

  xy2 = ImVec2 ( io.DisplaySize.x * 0.01f,                    io.DisplaySize.y );
  xy3 = ImVec2 ( io.DisplaySize.x * 0.01f,                    0.0f             );

  xy4 = ImVec2 ( io.DisplaySize.x / 2.0f - (io.DisplaySize.x * 0.125f), io.DisplaySize.y );
  xy5 = ImVec2 ( io.DisplaySize.x / 2.0f - (io.DisplaySize.x * 0.125f), 0.0f             );

  xy6 = ImVec2 ( io.DisplaySize.x / 2.0f + (io.DisplaySize.x * 0.125f), 0.0f             );
  xy7 = ImVec2 ( io.DisplaySize.x / 2.0f + (io.DisplaySize.x * 0.125f), io.DisplaySize.y );

  draw_list->PushClipRectFullScreen (                                                       );
  draw_list->AddRect                ( xy0, xy1, col32, 0.0f, 0x00, io.DisplaySize.x * 0.01f );
  draw_list->AddRect                ( xy2, xy3, col32, 0.0f, 0x00, io.DisplaySize.x * 0.01f );
  draw_list->AddRect                ( xy4, xy5, col32, 0.0f, 0x00, io.DisplaySize.x * 0.01f );
  draw_list->AddRect                ( xy6, xy7, col32, 0.0f, 0x00, io.DisplaySize.x * 0.01f );
  draw_list->PopClipRect            (                                                       );

  ImGui::End ();
}

float fExtraData = 0.0f;

class SKWG_FramePacing : public SK_Widget
{
public:
  SKWG_FramePacing (void) : SK_Widget ("FramePacing")
  {
    SK_ImGui_Widgets->frame_pacing = this;

    setResizable    (                false).setAutoFit (true ).setMovable (false).
    setDockingPoint (DockAnchor::SouthEast).setVisible (false);

    SK_FramePercentiles->load_percentile_cfg ();
  };

  void load (iSK_INI* cfg) override
  {
    SK_Widget::load (cfg);

    SK_FramePercentiles->load_percentile_cfg ();
  }

  void save (iSK_INI* cfg) override
  {
    if (cfg == nullptr)
      return;

    SK_Widget::save (cfg);

    SK_FramePercentiles->store_percentile_cfg ();

    cfg->write ();
  }

  void run (void) override
  {
    static auto* cp =
      SK_GetCommandProcessor ();

    static volatile LONG init = 0;

    if (0 == InterlockedCompareExchange (&init, 1, 0))
    {
      //auto *display_framerate_percentiles_above =
      //  SK_CreateVar (
      //    SK_IVariable::Boolean,
      //      &SK_FramePercentiles->display_above,
      //        nullptr );
      //
      //SK_RunOnce (
      //  cp->AddVariable (
      //    "Framepacing.DisplayLessDynamicPercentiles",
      //      display_framerate_percentiles
      //  )
      //);


      auto *display_framerate_percentiles =
        SK_CreateVar (
          SK_IVariable::Boolean,
            &SK_FramePercentiles->display,
              nullptr );

      SK_RunOnce (
        cp->AddVariable (
          "Framepacing.DisplayPercentiles",
            display_framerate_percentiles
        ) );
    }

    if (ImGui::GetFont () == nullptr)
      return;

    const float font_size           =             ImGui::GetFont  ()->FontSize                        ;//* scale;
    const float font_size_multiline = font_size + ImGui::GetStyle ().ItemSpacing.y + ImGui::GetStyle ().ItemInnerSpacing.y;

    float extra_line_space = 0.0F;

    static auto& percentile0 = SK_FramePercentiles->percentile0;
    static auto& percentile1 = SK_FramePercentiles->percentile1;

    if (has_battery)            extra_line_space += 1.16F;
    if (SK_FramePercentiles->display)
    {
      if (percentile0.has_data) extra_line_space += 1.16F;
      if (percentile1.has_data) extra_line_space += 1.16F;
    }

    // If configuring ...
    if (state__ != 0) extra_line_space += (1.16F * 5.5F);

    ImVec2   new_size (font_size * 35.0F, font_size_multiline * (5.44F + extra_line_space));
             new_size.y += fExtraData;

    if (extra_status_line > 0)
             new_size.y += ImGui::GetFont ()->FontSize + ImGui::GetStyle ().ItemSpacing.y;

    setSize (new_size);

    if (isVisible ())// && state__ == 0)
    {
      ImGui::SetNextWindowSize (new_size, ImGuiCond_Always);
    }
  }

  void draw (void) override
  {
    if (ImGui::GetFont () == nullptr)
      return;

    static const auto& io =
      ImGui::GetIO ();

    static bool move = true;

    if (move)
    {
      ImGui::SetWindowPos (
        ImVec2 ( io.DisplaySize.x - getSize ().x,
                 io.DisplaySize.y - getSize ().y )
      );

      move = false;
    }

    ImGui::BeginGroup ();

    SK_ImGui_DrawGraph_FramePacing ();

    has_battery =
      SK_ImGui::BatteryMeter ();
    ImGui::EndGroup   ();

    auto* pLimiter = debug_limiter ?
      SK::Framerate::GetLimiter (
       SK_GetCurrentRenderBackend ( ).swapchain.p,
        false                   )  : nullptr;

    if (pLimiter != nullptr)
    {
      ImGui::BeginGroup ();

      SK::Framerate::Limiter::snapshot_s snapshot =
                            pLimiter->getSnapshot ();

      struct {
        DWORD  dwLastSnap =   0;
        double dLastMS    = 0.0;
        double dLastFPS   = 0.0;

        void update (SK::Framerate::Limiter::snapshot_s& snapshot)
        {
          static constexpr DWORD UPDATE_INTERVAL = 225UL;

          DWORD dwNow =
            SK::ControlPanel::current_time;

          if (dwLastSnap < dwNow - UPDATE_INTERVAL)
          {
            dLastMS    =          snapshot.effective_ms;
            dLastFPS   = 1000.0 / snapshot.effective_ms;
            dwLastSnap = dwNow;
          }
        }
      } static effective_snapshot;

      effective_snapshot.update (snapshot);

      ImGui::BeginGroup ();
      ImGui::Text ("MS:");
      ImGui::Text ("FPS:");
      ImGui::Text ("EffectiveMS:");
      ImGui::Text ("Clock Ticks per-Frame:");
      ImGui::Separator ();
      ImGui::Text ("Limiter Time=");
      ImGui::Text ("Limiter Start=");
      ImGui::Text ("Limiter Next=");
      ImGui::Text ("Limiter Last=");
      ImGui::Text ("Limiter Freq=");
      ImGui::Separator ();
      ImGui::Text ("Limited Frames:");
      ImGui::Separator  ();
      ImGui::EndGroup   ();
      ImGui::SameLine   ();
      ImGui::BeginGroup ();
      ImGui::Text ("%f ms",  snapshot.ms);
      ImGui::Text ("%f fps", snapshot.fps);
      ImGui::Text ("%f ms        (%#06.1f fps)",
                   effective_snapshot.dLastMS,
                   effective_snapshot.dLastFPS);
      ImGui::Text ("%llu",   snapshot.ticks_per_frame);
      ImGui::Separator ();
      ImGui::Text ("%llu", ReadAcquire64 (&snapshot.time));
      ImGui::Text ("%llu", ReadAcquire64 (&snapshot.start));
      ImGui::Text ("%llu", ReadAcquire64 (&snapshot.next));
      ImGui::Text ("%llu", ReadAcquire64 (&snapshot.last));
      ImGui::Text ("%llu", SK_QpcFreq);
      ImGui::Separator ();
      ImGui::Text ("%llu", ReadAcquire64 (&snapshot.frames));
      //ImGui::SameLine  ();
      //ImGui::ProgressBar (
      //  static_cast <float> (
      //    static_cast <double> (pLimiter->frames_of_fame.frames_measured.count ()) /
      //    static_cast <double> (ReadAcquire64 (&snapshot.frames))
      //  )
      //);
      ImGui::EndGroup  ();

      if (ImGui::Button ("Reset"))        *snapshot.pRestart     = true;
      ImGui::SameLine ();
      if (ImGui::Button ("Full Restart")) *snapshot.pFullRestart = true;
      ImGui::EndGroup ();

      // Will be NULL for D3D9 and non-Flip SwapChains
      SK_ComQIPtr <IDXGISwapChain1> pSwap1 (
          SK_GetCurrentRenderBackend ().swapchain.p
        );

      static UINT                  uiLastPresent = 0;
      static DXGI_FRAME_STATISTICS frameStats    = { };

      if (pLimiter->get_limit () > 0.0f && pSwap1 != nullptr)
      {
        if ( SUCCEEDED (
               pSwap1->GetLastPresentCount (&uiLastPresent)
                       )
           )
        {
          if ( SUCCEEDED (
                 pSwap1->GetFrameStatistics (&frameStats)
                         )
             )
          {
          }
        }

        UINT uiLatency = ( uiLastPresent - frameStats.PresentCount );

        ImGui::Text ( "Present Latency: %i Frames", uiLatency );

        ImGui::Separator ();

        ImGui::Text ( "LastPresent: %i, PresentCount: %i", uiLastPresent, frameStats.PresentCount     );
        ImGui::Text ( "PresentRefreshCount: %i, SyncRefreshCount: %i",    frameStats.PresentRefreshCount,
                                                                          frameStats.SyncRefreshCount );
      }

      float fUndershoot =
        pLimiter->get_undershoot ();

      if (ImGui::InputFloat ("Undershoot %", &fUndershoot, 0.1f, 0.1f))
      {
        pLimiter->set_undershoot (fUndershoot);
        pLimiter->reset          (true);
      }

      fExtraData = ImGui::GetItemRectSize ().y;
    } else
      fExtraData = 0.0f;
  }


  void config_base (void) override
  {
    SK_Widget::config_base ();

    auto *pLimiter =
      SK::Framerate::GetLimiter (
        SK_GetCurrentRenderBackend ().swapchain.p,
        false
      );

    if (pLimiter == nullptr)
      return;

    auto& snapshots =
      pLimiter->frame_history_snapshots;

    ImGui::Separator ();

    bool changed = false;
    bool display = SK_FramePercentiles->display;

        changed |= ImGui::Checkbox  ("Show Percentile Analysis", &display);
    if (changed)
    {
      SK_FramePercentiles->toggleDisplay ();
    }

    if (SK_FramePercentiles->display)
    {
      changed |= ImGui::Checkbox ("Draw Percentiles Above Graph",         &SK_FramePercentiles->display_above);
      changed |= ImGui::Checkbox ("Use Short-Term (~15-30 seconds) Data", &SK_FramePercentiles->display_most_recent);

      ImGui::Separator ();
      ImGui::TreePush  ();

      if ( ImGui::SliderFloat (
             "Percentile Class 0 Cutoff",
               &SK_FramePercentiles->percentile0.cutoff,
                 0.1f, 99.99f, "%3.1f%%" )
         )
      {
        snapshots.reset (); changed = true;
      }

      if ( ImGui::SliderFloat (
             "Percentile Class 1 Cutoff",
               &SK_FramePercentiles->percentile1.cutoff,
                 0.1f, 99.99f, "%3.1f%%" )
         )
      {
        snapshots.reset (); changed = true;
      }

      ImGui::TreePop ();
    }

    if (changed)
      SK_FramePercentiles->store_percentile_cfg ();

    ImGui::Checkbox ("Framerate Limiter Debug", &debug_limiter);
  }

  void OnConfig (ConfigEvent event) noexcept override
  {
    switch (event)
    {
      case SK_Widget::ConfigEvent::LoadComplete:
        break;

      case SK_Widget::ConfigEvent::SaveStart:
        break;
    }
  }

  //sk::ParameterInt* samples_max;
};

SK_LazyGlobal <SKWG_FramePacing> __frame_pacing__;

void SK_Widget_InitFramePacing (void)
{
  SK_RunOnce (__frame_pacing__.get ());
}