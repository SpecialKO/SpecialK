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

#include <SpecialK/render/d3d11/d3d11_core.h>
#include <imgui/font_awesome.h>

#include <SpecialK/adl.h>
#include <SpecialK/render/present_mon/PresentMon.hpp>
#include <SpecialK/nvapi.h>

#include <Pdh.h>
#include <PdhMsg.h>

#pragma comment (lib, "pdh.lib")

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"Frame Pace"

namespace
SK_ImGui
{
  bool BatteryMeter (void);
};

static bool has_battery   = false;
static bool has_vram      = false;
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

    SK_RunOnce (
    {
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
        L"Widget.Frame Pacing", L"DisplayPercentiles"
      );

      percentile_cfg.display_above->register_to_ini ( osd_ini,
        L"Widget.Frame Pacing", L"PercentilesAboveGraph"
      );
      percentile_cfg.display_most_recent->register_to_ini ( osd_ini,
        L"Widget.Frame Pacing", L"ShortTermPercentiles"
      );
      percentile_cfg.percentile0_cutoff->register_to_ini ( osd_ini,
        L"Widget.Frame Pacing", L"Percentile[0].Cutoff"
      );
      percentile_cfg.percentile1_cutoff->register_to_ini ( osd_ini,
        L"Widget.Frame Pacing", L"Percentile[1].Cutoff"
      );
    });

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
SK_RenderBackend::latency_monitor_s::reset (void)
{
  stale = true;

  counters.frameStats0 = { };
  counters.frameStats1 = { };
  counters.lastFrame   =   0;
  counters.lastPresent =   0;

  delays.PresentQueue  = 0;
  delays.SyncDelay     = 0;
  delays.TotalMs       = 0.0F;

  stats.AverageMs      =  0.0F;
  stats.MaxMs          =  0.0F;
  stats.ScaleMs        = 99.0F;

  std::fill_n (stats.History, ARRAYSIZE (stats.History), 0.0F);
}

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


int extra_status_line = 0;

HANDLE __SK_ETW_PresentMon_Thread = INVALID_HANDLE_VALUE;

void
SK_SpawnPresentMonWorker (void)
{
  // Wine doesn't support this...
  if (config.compatibility.using_wine)
    return;

  // User Permissions do not permit PresentMon
  if (! config.render.framerate.supports_etw_trace)
  {
    return;
  }

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

float fMinimumHeight = 0.0f;
float fExtraData     = 0.0f;

class SKWG_FramePacing : public SK_Widget
{
public:
  SKWG_FramePacing (void) : SK_Widget ("Frame Pacing")
  {
    SK_ImGui_Widgets->frame_pacing = this;

    setResizable    (                false).setAutoFit (true ).setMovable (false).
    setDockingPoint (DockAnchor::SouthEast).setVisible (false);

    SK_FramePercentiles->load_percentile_cfg ();

    meter_cfg.display_battery =
      dynamic_cast <sk::ParameterBool *> (
        SK_Widget_ParameterFactory->create_parameter <bool> (
          L"Display Battery Info (When running on battery)"
        )
      );

    meter_cfg.display_vram =
      dynamic_cast <sk::ParameterBool *> (
        SK_Widget_ParameterFactory->create_parameter <bool> (
          L"Draw VRAM Gauge below Framepacing Widget"
        )
      );

    meter_cfg.display_load =
      dynamic_cast <sk::ParameterBool *> (
        SK_Widget_ParameterFactory->create_parameter <bool> (
          L"Draw CPU/GPU Load on the side of Framepacing Widget"
        )
      );

    meter_cfg.display_disk =
      dynamic_cast <sk::ParameterBool *> (
        SK_Widget_ParameterFactory->create_parameter <bool> (
          L"Draw Disk Activity on the side of Framepacing Widget"
        )
      );

    meter_cfg.display_vram->register_to_ini ( osd_ini,
      L"Widget.Frame Pacing", L"DisplayVRAMGauge"
    );

    meter_cfg.display_disk->register_to_ini ( osd_ini,
      L"Widget.Frame Pacing", L"DisplayDiskActivity"
    );

    meter_cfg.display_load->register_to_ini ( osd_ini,
      L"Widget.Frame Pacing", L"DisplayProcessorLoad"
    );

    meter_cfg.display_battery->register_to_ini ( osd_ini,
      L"Widget.Frame Pacing", L"DisplayBatteryInfo"
    );

    meter_cfg.display_vram->load    (display_vram);
    meter_cfg.display_load->load    (display_load);
    meter_cfg.display_disk->load    (display_disk);
    meter_cfg.display_battery->load (display_battery);
  };

  void load (iSK_INI* cfg) override
  {
    SK_Widget::load (cfg);

    meter_cfg.display_vram->load    (display_vram);
    meter_cfg.display_load->load    (display_load);
    meter_cfg.display_disk->load    (display_disk);
    meter_cfg.display_battery->load (display_battery);

    SK_FramePercentiles->load_percentile_cfg ();
  }

  void save (iSK_INI* cfg) override
  {
    if (cfg == nullptr)
      return;

    setMinSize (ImVec2 (0.0f, 0.0f));

    SK_Widget::save (cfg);

    meter_cfg.display_vram->store    (display_vram);
    meter_cfg.display_load->store    (display_load);
    meter_cfg.display_disk->store    (display_disk);
    meter_cfg.display_battery->store (display_battery);

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

      setMinSize (ImVec2 (0.0f, 0.0f));
    }

    if (ImGui::GetFont () == nullptr)
      return;

    const float ui_scale            =             ImGui::GetIO    ().FontGlobalScale;
    const float font_size           =             ImGui::GetFont  ()->FontSize * ui_scale;
    auto&       style               =             ImGui::GetStyle ();
  //const float font_size_multiline = font_size + ImGui::GetStyle ().ItemSpacing.y + ImGui::GetStyle ().ItemInnerSpacing.y;

    float extra_line_space = 0.0F;

    // If configuring ...
    if (state__ != 0) extra_line_space += 300.0F * ui_scale;

    // Make room for control panel's title bar
    if (SK_ImGui_Visible)
      extra_line_space += font_size + style.ItemSpacing.y +
                                      style.ItemInnerSpacing.y;

    ImVec2   new_size (font_size * 35.0F, fMinimumHeight + style.ItemSpacing.y      * 2.0F +
                                                           style.ItemInnerSpacing.y * 2.0F +
                                                                           extra_line_space);
             new_size.y += fExtraData;

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

    has_battery = display_battery &&
      SK_ImGui::BatteryMeter ();
    has_vram    = display_vram;

    if (has_vram)
    {
      SK_ImGui_DrawVRAMGauge ();
    }
    ImGui::EndGroup   ();

    fMinimumHeight = ImGui::GetItemRectSize ().y;

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
      ImGui::Text ("%llu", SK_PerfFreq);
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

    changed |= ImGui::Checkbox ("Show VRAM Gauge",    &display_vram);
    changed |= ImGui::Checkbox ("Show CPU/GPU Load",  &display_load);
    changed |= ImGui::Checkbox ("Show Disk Activity", &display_disk);
    changed |= ImGui::Checkbox ("Show Battery State", &display_battery);

    if (changed)
      save (osd_ini);

         changed = false;
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
      ImGui::TreePush  ("");

      if ( ImGui::SliderFloat (
             "Percentile Class 0 Cutoff",
               &SK_FramePercentiles->percentile0.cutoff,
                 0.1f, 99.99f, "%3.1f%%" )
         )
      {
        snapshots->reset (); changed = true;
      }

      if ( ImGui::SliderFloat (
             "Percentile Class 1 Cutoff",
               &SK_FramePercentiles->percentile1.cutoff,
                 0.1f, 99.99f, "%3.1f%%" )
         )
      {
        snapshots->reset (); changed = true;
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

  bool display_vram    = false;
  bool display_load    = true;
  bool display_disk    = false;
  bool display_battery = true;

  struct {
    sk::ParameterBool* display_vram    = nullptr;
    sk::ParameterBool* display_load    = nullptr;
    sk::ParameterBool* display_disk    = nullptr;
    sk::ParameterBool* display_battery = nullptr;
  } meter_cfg;
  //sk::ParameterInt* samples_max;
};

SK_LazyGlobal <SKWG_FramePacing> __frame_pacing__;

void SK_Widget_InitFramePacing (void)
{
  SK_RunOnce (__frame_pacing__.get ());
}

using namespace ImGui;
using namespace std;

enum ValueBarFlags_ {
    ValueBarFlags_None = 0,
    ValueBarFlags_Vertical = 1 << 0,
};
using ValueBarFlags = int;

// Similar to `ImGui::ProgressBar`, but with a horizontal/vertical switch.
// The value text doesn't follow the value like `ImGui::ProgressBar`.
// Here it's simply displayed in the middle of the bar.
// Horizontal labels are placed to the right of the rect.
// Vertical labels are placed below the rect.
void ValueBar(const char *label, const float value, const ImVec2 &size, const float min_value = 0, const float max_value = 1, const ValueBarFlags flags = ValueBarFlags_None) {
    const bool is_h = !(flags & ValueBarFlags_Vertical);
    const auto &style = GetStyle();
    const auto &draw_list = GetWindowDrawList();
    const auto &cursor_pos = GetCursorScreenPos();
    const float fraction = (value - min_value) / max_value;
    const float frame_height = GetFrameHeight();
    const auto &label_size = strlen(label) > 0 ? ImVec2{CalcTextSize(label).x, frame_height} : ImVec2{0, 0};
    const auto &rect_size = is_h ? ImVec2{CalcItemWidth(), frame_height} : ImVec2{GetFontSize() * 2, size.y - label_size.y};
    const auto &rect_start = cursor_pos + ImVec2{is_h ? 0 : max(0.0f, (label_size.x - rect_size.x) / 2), 0};

    draw_list->AddRectFilled(rect_start, rect_start + rect_size, GetColorU32(ImGuiCol_FrameBg), style.FrameRounding);
    draw_list->AddRectFilled(
        rect_start + ImVec2{0, is_h ? 0 : (1 - fraction) * rect_size.y},
        rect_start + rect_size * ImVec2{is_h ? fraction : 1, 1},
        GetColorU32(ImGuiCol_PlotHistogram),
      /// XXX: Is this accurate?
        style.FrameRounding, is_h ? ImDrawFlags_RoundCornersTopLeft : ImDrawFlags_RoundCornersBottomRight
    );
    const string value_text = is_h ? format("{:.2f}", value) : format("{:.1f}", value);
    draw_list->AddText(rect_start + (rect_size - CalcTextSize(value_text.c_str())) / 2, GetColorU32(ImGuiCol_Text), value_text.c_str());
    if (label) {
        draw_list->AddText(
            rect_start + ImVec2{is_h ? rect_size.x + style.ItemInnerSpacing.x : (rect_size.x - label_size.x) / 2, style.FramePadding.y + (is_h ? 0 : rect_size.y)},
            GetColorU32(ImGuiCol_Text), label);
    }
}

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


  auto pLimiter =
    SK::Framerate::GetLimiter (
      SK_GetCurrentRenderBackend ().swapchain.p,
      false
    );

  static ULONG64    ullResetFrame = 0;
  static SK::Framerate::Limiter
                    *pLastLimiter = nullptr;
  if (std::exchange (pLastLimiter, pLimiter) != pLimiter || SK_GetFramesDrawn () == ullResetFrame)
  {
    // Finish the reset after 2 frames
    if (ullResetFrame != SK_GetFramesDrawn ())
        ullResetFrame  = SK_GetFramesDrawn () + 2;
    else
    {
      // The underlying framerate limiter changed, clear the old data...
      SK_ImGui_Frames->reset ();

      SK_RenderBackend_V2::latency.reset ();
    }
  }

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

  const SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  if (target == 0.0f && (! ffx))
  {
    target_frametime = (float)(1000.0 /
      (static_cast <double> (rb.displays [rb.active_display].signal.timing.vsync_freq.Numerator) / 
       static_cast <double> (rb.displays [rb.active_display].signal.timing.vsync_freq.Denominator)) );
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

  if (rb.presentation.mode != SK_PresentMode::Unknown)
  {
    extra_status_line = 1;

    const bool fast_path =
      ( rb.presentation.mode == PresentMode::Hardware_Legacy_Flip                 ) ||
      ( rb.presentation.mode == PresentMode::Hardware_Legacy_Copy_To_Front_Buffer ) ||
      ( rb.presentation.mode == PresentMode::Hardware_Independent_Flip            ) ||
      ( rb.presentation.mode == PresentMode::Hardware_Composed_Independent_Flip   );

    static constexpr char* pszHourglasses [] = {
      ICON_FA_HOURGLASS_START,
      ICON_FA_HOURGLASS_HALF,
      ICON_FA_HOURGLASS_END,
      ICON_FA_HOURGLASS_HALF,
    };

    const char* szHourglass =
      pszHourglasses [
        (SK::ControlPanel::current_time % 4000) / 1000
      ];

    ImGui::Text (
      " " ICON_FA_LINK "  %8ws    ", rb.name
    );

    ImGui::SameLine ();

    ImGui::Text (
      "%s%s    ", fast_path ? ICON_FA_TACHOMETER_ALT       " "
                            : ICON_FA_EXCLAMATION_TRIANGLE " ",
        PresentModeToString (rb.presentation.mode)
    );

    ImGui::SameLine ();

    ImGui::Text (
      "%hs %5.2f ms    ",
        szHourglass, rb.presentation.avg_stats.latency * 1000.0
    );

    if (ImGui::IsItemHovered ())
    {
      ImGui::BeginTooltip    ();
      ImGui::BeginGroup      ();
      ImGui::TextUnformatted ("Present Latency:      ");
      ImGui::TextUnformatted ("GPU Idle Time:        ");
      ImGui::TextUnformatted ("Frame Time (Display): ");
      ImGui::TextUnformatted ("Frame Time (CPU):     ");
      ImGui::EndGroup        ();
      ImGui::SameLine        ();
      ImGui::BeginGroup      ();
      ImGui::Text            ("%5.2f ms", rb.presentation.avg_stats.latency * 1000.0);
      ImGui::Text            ("%5.2f ms", rb.presentation.avg_stats.idle    * 1000.0);
      ImGui::Text            ("%5.2f ms", rb.presentation.avg_stats.display * 1000.0);
      ImGui::Text            ("%5.2f ms", rb.presentation.avg_stats.cpu     * 1000.0);
      ImGui::EndGroup        ();
      ImGui::EndTooltip      ();
    }

    const float
      fBusyWaitMs      = SK_Framerate_GetBusyWaitMs      (),
      fSleepWaitMs     = SK_Framerate_GetSleepWaitMs     (),
      fBusyWaitPercent = SK_Framerate_GetBusyWaitPercent ();

    // For extremely short wait periods (i.e. OpenGL-IK composition),
    //   don't even bother printing this...
    if ( ( fBusyWaitMs  >= 0.1 || 
           fSleepWaitMs >= 0.1 ) && fBusyWaitPercent >= 0.0f
                                 && fBusyWaitPercent <= 100.0 )
    {
      ImGui::SameLine ();

      ImGui::Text (
        ICON_FA_MICROCHIP " %3.1f%%", fBusyWaitPercent
      );

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip ();
        ImGui::Text         (
          "Framerate Limiter CPU busy-wait;"
          " lower values are more energy efficient."
        );
        ImGui::Separator    ();
        ImGui::Text         ("Average Wait Time: %5.2f ms", fBusyWaitMs +
                                                            fSleepWaitMs);
        ImGui::BulletText   ("Sleep-Wait:\t%5.2f ms",       fSleepWaitMs);
        ImGui::BulletText   ("Busy-Wait: \t%5.2f ms",       fBusyWaitMs );
        ImGui::EndTooltip   ();
      }
    }
  }

  else
  {
    // User Permissions do not permit PresentMon
    if (! config.render.framerate.supports_etw_trace)
    {
      if (! config.compatibility.using_wine)
      {
        ImGui::TextUnformatted (
          "Presentation Model Unknown  (Full SK Install is Required)"
        );

        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip    ();
          ImGui::TextUnformatted (
            "User Account Permissions Do Not Permit Running PresentMon"
          );
          ImGui::Separator       ();
          ImGui::BulletText      (
            "Refer to SwapChain Presentation Monitor Settings in SKIF"
          );
          ImGui::EndTooltip      ();
        }
      }

      else
      {
        ImGui::TextUnformatted (
          "SteamOS  (Presentation Model Undefined)"
        );
      }
    }

    else
    {
      ImGui::TextUnformatted (
        "Presentation Model Tracking Unavailable"
      );

      if (ImGui::IsItemHovered ())
      {
        ImGui::SetTooltip (
          "Too many applications may be using PresentMon;"
          " often restarting the game will fix this."
        );
      }
    }
  }

  ImGui::SameLine ();

  if (SK_ImGui_ProcessGamepadStatusBar (true) > 0)
    extra_status_line = 1;

  bool bDrawProcessorLoad =
    ((SKWG_FramePacing *)SK_ImGui_Widgets->frame_pacing)->display_load;

  bool bDrawDisk =
    ((SKWG_FramePacing *)SK_ImGui_Widgets->frame_pacing)->display_disk;

  if (SK_FramePercentiles->display_above)
      SK_ImGui_DrawFramePercentiles ();

  bool valid_latency =
    (! SK_RenderBackend_V2::latency.stale);


  static int ver_major = { },
             ver_minor = { };

  static bool has_stable_hw_flip_queue = true;

  // NVIDIA's drivers are still broken for now
#if 0
  if (sk::NVAPI::nv_hardware)
  {
    if ( ver_major == 0 && 2 == swscanf ( sk::NVAPI::GetDriverVersion (nullptr).c_str (),
                                            L"%d.%d", &ver_major, &ver_minor ) )
    {
      has_stable_hw_flip_queue =
        ( ver_major  > 546 ||
          ver_major == 546 && ver_minor >= 31 );
    }

    else
    {
      ver_major = -1;
    }
  }
#endif


  if (valid_latency)
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

    if ( SK_RenderBackend_V2::latency.stats.MaxMs     > 1000.0 ||
         SK_RenderBackend_V2::latency.stats.AverageMs > 1000.0 )
    {
      // We have invalid data, stop displaying latency stats until resolved
      valid_latency = false;
    }
  }

  if (valid_latency)
  {
    if ((! rb.displays [rb.active_display].wddm_caps._3_0.HwFlipQueueEnabled) || has_stable_hw_flip_queue)
    {
      snprintf
        ( szAvg,
            511, (const char *)
            u8"Avg milliseconds per-frame: %6.3f  (Target: %6.3f)\n"
            u8"         Render latency:           %lu Frame%s | %3.1f / %3.1f ms |  %lu Hz \n\n\n\n"
            u8"Variation:  %9.5f ms    %5.1f FPS  ±  %3.1f frames",
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
            u8"         Render latency:          %lu Frame%s | HW Flip Q |  %lu Hz \n\n\n\n"
            u8"Variation:  %9.5f ms    %5.1f FPS  ±  %3.1f frames",
                sum / frames,
                  target_frametime,
                      SK_RenderBackend_V2::latency.delays.PresentQueue,
                      SK_RenderBackend_V2::latency.delays.PresentQueue != 1 ?
                                                                       "s " : "  ",
                        SK_RenderBackend_V2::latency.delays.SyncDelay,
              (double)max - (double)min,
                      1000.0f / (sum / frames),
                        ((double)max-(double)min)/(1000.0f/(sum/frames)) );
    }
  }

  else
  {
    snprintf
      ( szAvg,
          511, (const char *)
          u8"Avg milliseconds per-frame: %6.3f  (Target: %6.3f)\n"
          u8"    Extreme frame times:     %6.3f min, %6.3f max\n\n\n\n"
          u8"Variation:  %9.5f ms    %5.1f FPS  ±  %3.1f frames",
              sum / frames,
                target_frametime,
                  min, max,
            (double)max - (double)min,
                    1000.0f / (sum / frames),
                      ((double)max-(double)min)/(1000.0f/(sum/frames)) );
  }

  struct disk_s {
    HQUERY   query   = nullptr;
    HCOUNTER counter = nullptr;

    struct counter_enum_s {
      wchar_t *Buffer = nullptr;
      DWORD    Size   = 0;

      ~counter_enum_s (void)
      {
        if (Buffer != nullptr)
        {
          std::free (
            std::exchange (Buffer, nullptr)
          );
        }
      }
    };
  } static disk;

  SK_RunOnce (
  {
    disk_s::counter_enum_s CounterList;
    disk_s::counter_enum_s InstanceList;

    const auto drive_num =
      PathGetDriveNumberW (SK_GetHostPath ());

    PDH_STATUS status =
      PdhEnumObjectItems ( nullptr, nullptr, L"PhysicalDisk",
          CounterList.Buffer,  &CounterList.Size,
         InstanceList.Buffer, &InstanceList.Size,
          PERF_DETAIL_WIZARD, 0
      );

    if (status == PDH_MORE_DATA)
    {
      CounterList.Buffer  = (wchar_t *)malloc (CounterList.Size  * sizeof (wchar_t));
      InstanceList.Buffer = (wchar_t *)malloc (InstanceList.Size * sizeof (wchar_t));

      status =
        PdhEnumObjectItems ( nullptr, nullptr, L"PhysicalDisk",
            CounterList.Buffer, &CounterList.Size,
           InstanceList.Buffer, &InstanceList.Size,
            PERF_DETAIL_WIZARD, 0
        );

      if (status == ERROR_SUCCESS)
      {
        const auto physical_name =
          SK_FormatStringW (L" %wc:", L'A' + drive_num);

        for ( wchar_t *pInstance  = InstanceList.Buffer;
                      *pInstance != 0;
                       pInstance += wcslen (pInstance) + 1 )
        {
          if (StrStrIW (pInstance, physical_name.c_str ()))
          {
            status =
              PdhOpenQuery (nullptr, 0, &disk.query);

            if (status == ERROR_SUCCESS)
            {
              const auto counter_path =
                SK_FormatStringW (
                  LR"(\PhysicalDisk(%ws)\%% Disk Time)",
                    pInstance
                );

              status =
                PdhAddCounter ( disk.query, counter_path.c_str (),
                                   0, &disk.counter );

              SK_LOGi1 (
                L"Using Disk Activity Counter: %ws",
                                      counter_path.c_str ()
              );

              break;
            }
          }
        }
      }
    }
  });

  float fGPULoadPercent = 0.0f;

  float fUIScale    = ImGui::GetIO ().FontGlobalScale;
  float fGaugeSizes = 0.0f;

  static const float fCPUSize  = ImGui::CalcTextSize ("CPU.").x  / fUIScale;
  static const float fGPUSize  = ImGui::CalcTextSize ("GPU.").x  / fUIScale;
  static const float fDISKSize = ImGui::CalcTextSize ("DISK.").x / fUIScale;

  if (bDrawProcessorLoad)
  {
    fGaugeSizes = fCPUSize * fUIScale;

    static constexpr DWORD _UpdateFrequencyInMsec = 75;
    static           DWORD dwLastUpdatedGPU =
      SK::ControlPanel::current_time;

    if (dwLastUpdatedGPU < SK::ControlPanel::current_time - _UpdateFrequencyInMsec)
    {   dwLastUpdatedGPU = SK::ControlPanel::current_time;
      SK_PollGPU ();
    }

    fGPULoadPercent =
      SK_GPU_GetGPULoad (0);

    if (fGPULoadPercent > 0.0f)
      fGaugeSizes += fGPUSize * fUIScale;
  }

  if (bDrawDisk)
    fGaugeSizes += fDISKSize * fUIScale;

  const ImVec2 border_dims (
    std::max (               (500.0f * fUIScale) - fGaugeSizes,
               ImGui::GetContentRegionAvail ().x - fGaugeSizes ),
      font_size * 7.0f
  );

  float fX = ImGui::GetCursorPosX (),
        fY = ImGui::GetCursorPosY ();

  ImGui::PushItemFlag ( ImGuiItemFlags_NoNav             |
                        ImGuiItemFlags_NoNavDefaultFocus |
                        ImGuiItemFlags_AllowOverlap, true );

  ImGui::PushStyleColor ( ImGuiCol_PlotLines,
                             ImColor::HSV ( 0.31f - 0.31f *
                     std::min ( 1.0f, (max - min) / (2.0f * target_frametime) ),
                                             1.0f,   1.0f ).Value );

  ImGui::PushStyleVar   (ImGuiStyleVar_FrameRounding, 0.0f);
  ImGui::PushStyleColor (ImGuiCol_PlotHistogram, ImVec4 (.66f, .66f, .66f, .75f));
  ImGui::PlotHistogram ( SK_ImGui_Visible ? "###ControlPanel_LatencyHistogram" :
                                            "###Floating_LatencyHistogram",
                           SK_RenderBackend_V2::latency.stats.History,
                                          valid_latency && ((! rb.displays [rb.active_display].wddm_caps._3_0.HwFlipQueueEnabled) || has_stable_hw_flip_queue) ?
             IM_ARRAYSIZE (SK_RenderBackend_V2::latency.stats.History)
                                                        : 0,
                               SK_GetFramesDrawn () % 120,
                                 "",
                                   0,
                           SK_RenderBackend_V2::latency.stats.ScaleMs,
                                      border_dims );
  ImGui::PopStyleColor (  );

  ImGui::SameLine      (  );
  ImGui::SetCursorPosX (fX);
  ImGui::BeginGroup    (  );

  // We don't want a second background dimming things even more...
  ImGui::PushStyleColor (ImGuiCol_FrameBg, ImVec4 (0.0f, 0.0f, 0.0f, 0.0f));
  ImGui::PlotLines ( SK_ImGui_Visible ? "###ControlPanel_FramePacing" :
                                        "###Floating_FramePacing",
                       SK_ImGui_Frames->getValues     ().data (),
      static_cast <int> (frames),
                           SK_ImGui_Frames->getOffset (),
                             szAvg,
                               -.1f,
                                 2.0f * target_frametime + 0.1f,
                                   border_dims );

  const bool bDLSSPacing =
    ( ( __SK_ForceDLSSGPacing || ( config.nvidia.reflex.use_limiter && config.nvidia.reflex.override ) )
                              &&   config.nvidia.reflex.frame_interval_us != 0
                              && rb.isReflexSupported () );

  if (bDLSSPacing)
  {
    ImGui::SameLine      (  );
    ImGui::SetCursorPosX (fX);
    ImGui::TextColored   (ImVec4 (0.463f, 0.726f, 0.0f, 1.0f), "NV");
  }
  
  ImGui::PopStyleColor (2);
  ImGui::EndGroup      ( );
  ImGui::PopItemFlag   ( );

  // Only toggle when clicking the graph and percentiles are off,
  //   to turn them back off, click the progress bars.
  if ((! SK_FramePercentiles->display) && ImGui::IsItemClicked ())
  {
    SK_FramePercentiles->toggleDisplay ();
  }

  if (bDrawProcessorLoad || bDrawDisk)
  {
    ImGui::SameLine (0.0f, 0.0f);

    auto window_pos = ImGui::GetWindowPos (),
         cursor_pos = ImGui::GetCursorPos ();
    auto scroll_y   = ImGui::GetScrollY ();

    if (bDrawProcessorLoad && SK_ImGui_Widgets->cpu_monitor != nullptr)
    {
      SK_RunOnce ({
        SK_ImGui_Widgets->cpu_monitor->setActive (true);
        SK_StartPerfMonThreads                   (    );
      });
    }

    float fCPULoadPercent = 0.0f;

    static cpu_perf_t::cpu_stat_s prev_sample = { };
    static cpu_perf_t::cpu_stat_s new_sample  = { };

    if (SK_WMI_CPUStats->cpus [64].update_time >= new_sample.update_time)
    {
      prev_sample = new_sample;
      new_sample  = SK_WMI_CPUStats->cpus [64];
    }

    fCPULoadPercent =
      ( std::max (0.0f, prev_sample.getPercentLoad ()) +
        std::max (0.0f, new_sample. getPercentLoad ()) ) / 2.0f;

    ImRect frame_bb
      ( window_pos.x + cursor_pos.x - 2, window_pos.y + cursor_pos.y + 1 - scroll_y,
        window_pos.x + cursor_pos.x - 2 +
         fGaugeSizes + 1,                window_pos.y + cursor_pos.y +
                                                    font_size * 7.0f - 1 - scroll_y );

    ImGui::BeginGroup     (); // 2 frames is intentional to match the opacity of the rest of the graph
    ImGui::RenderFrame    (frame_bb.Min, frame_bb.Max, ImGui::GetColorU32 (ImGuiCol_FrameBg), false);
    ImGui::PushStyleColor (ImGuiCol_FrameBg,     ImGui::GetStyleColorVec4 (ImGuiCol_ChildBg));
    ImGui::PushStyleColor (ImGuiCol_Text,          ImVec4 (1.f,  1.f,  1.f, 1.f));
    ImGui::PushStyleColor (ImGuiCol_PlotHistogram, ImColor::HSV ((100.0f - fGPULoadPercent) / 100.0f * 0.278f, .88f, .75f).Value);
    if (bDrawProcessorLoad)
    {
      if (               fGPULoadPercent > 0.0f)
      {
        ValueBar ("GPU", fGPULoadPercent, ImVec2 (5.0f * fUIScale, font_size * 7.0f), 0.0f, 100.0f, ValueBarFlags_Vertical);
        ImGui::SetCursorPos (ImVec2 (GetCursorPosX () + fGPUSize * fUIScale, fY));
      }
      ImGui::PushStyleColor (ImGuiCol_PlotHistogram, ImColor::HSV ((100.0f - fCPULoadPercent) / 100.0f * 0.278f, .88f, .75f).Value);
      ValueBar ("CPU", fCPULoadPercent, ImVec2 (5.0f * fUIScale, font_size * 7.0f), 0.0f, 100.0f, ValueBarFlags_Vertical);
      ImGui::PopStyleColor  ();
      ImGui::SetCursorPos   (ImVec2 (GetCursorPosX () + fCPUSize * fUIScale, fY));
    }

    if (bDrawDisk)
    {
      PDH_FMT_COUNTERVALUE counterValue = { };

      static float  fLastDisk    = 0.0f;
      static DWORD dwLastSampled = SK::ControlPanel::current_time;

      counterValue.doubleValue = fLastDisk;

      if (dwLastSampled < SK::ControlPanel::current_time - 150)
      {
        if ( ERROR_SUCCESS == PdhCollectQueryData         (disk.query) &&
             ERROR_SUCCESS == PdhGetFormattedCounterValue (disk.counter, PDH_FMT_DOUBLE, nullptr, &counterValue) )
        {
          dwLastSampled =
            SK::ControlPanel::current_time;
        }

        else
          counterValue.doubleValue = fLastDisk;
      }

      float     fDisk = (static_cast <float> (3 * std::min (counterValue.doubleValue, 100.0)) + fLastDisk) / 4.0f;
            fLastDisk = fDisk;

      ImGui::PushStyleColor (ImGuiCol_PlotHistogram, ImColor::HSV ((100.0f - fDisk) / 100.0f * 0.278f, .88f, .75f).Value);
      ValueBar ("DISK", fDisk, ImVec2 (5.0f * fUIScale, font_size * 7.0f), 0.0f, 100.0f, ValueBarFlags_Vertical);
      ImGui::PopStyleColor  ();
    }

    ImGui::PopStyleColor  (3);
    ImGui::EndGroup       ();

    ImGui::SetCursorPos   (ImVec2 (fX, fY + font_size * 7.0f + ImGui::GetStyle ().ItemSpacing.y * fUIScale));
  }

  ImGui::PopStyleVar ();



  if (! SK_FramePercentiles->display_above)
        SK_ImGui_DrawFramePercentiles ();
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
    &pLimiter->frame_history_snapshots->frame_history;

  static ULONG64    ullResetFrame = 0;
  static SK::Framerate::Limiter
                    *pLastLimiter = nullptr;
  if (std::exchange (pLastLimiter, pLimiter) != pLimiter || SK_GetFramesDrawn () == ullResetFrame)
  {
    // Finish the reset after 2 frames
    if (ullResetFrame != SK_GetFramesDrawn ())
        ullResetFrame  = SK_GetFramesDrawn () + 2;
    else
    {
      // (re)Initialize our counters if the underlying framerate limiter changes
      frame_history->reset ();
          snapshots->reset ();
    }
  }

  long double data_timespan = ( show_immediate ?
            frame_history->calcDataTimespan () :
           snapshots->mean.calcDataTimespan () );

  ImGui::PushStyleColor (ImGuiCol_Text,           ImColor (255, 255, 255).Value);
  ImGui::PushStyleColor (ImGuiCol_FrameBg,        ImColor ( 0.3f,  0.3f,  0.3f, 0.7f).Value);
  ImGui::PushStyleColor (ImGuiCol_FrameBgHovered, ImColor ( 0.6f,  0.6f,  0.6f, 0.8f).Value);
  ImGui::PushStyleColor (ImGuiCol_FrameBgActive,  ImColor ( 0.9f,  0.9f,  0.9f, 0.9f).Value);

  percentile0.computeFPS (                             show_immediate ?
    frame_history->calcPercentile   (percentile0.cutoff, all_samples) :
      snapshots->percentile0.calcMean                   (all_samples) );

  percentile1.computeFPS (                             show_immediate ?
    frame_history->calcPercentile   (percentile1.cutoff, all_samples) :
      snapshots->percentile1.calcMean                   (all_samples) );

  mean.computeFPS ( show_immediate          ?
      frame_history->calcMean (all_samples) :
     snapshots->mean.calcMean (all_samples) );

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

      else snapshots->reset ();
    }

    ImVec4           p0_color  (
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

      ImVec4         p1_color  (
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

#if 0 // Steam Deck Horizontal Tearing Bars
  const ImU32 col32 =
     ImColor (col);

  ImDrawList* draw_list =
    ImGui::GetWindowDrawList ();

  ImVec2  xy0, xy1;
  ImVec2  xy2, xy3;
  ImVec2  xy4, xy5;
  ImVec2  xy6, xy7;
  ImVec2  xy8, xy9;
  ImVec2 xy10, xy11;

  static auto& io =
    ImGui::GetIO ();

  xy0 = ImVec2 ( io.DisplaySize.x/* - io.DisplaySize.x * 0.01f*/, 0.0f             );
  xy1 = ImVec2 ( io.DisplaySize.x/* - io.DisplaySize.x * 0.01f*/, io.DisplaySize.y );

  xy2 = ImVec2 ( /*io.DisplaySize.x * 0.01f*/0.0f,                    io.DisplaySize.y );
  xy3 = ImVec2 ( /*io.DisplaySize.x * 0.01f*/0.0f,                    0.0f             );

  xy4 = ImVec2 ( io.DisplaySize.x / 2.0f - (io.DisplaySize.x * 0.125f), io.DisplaySize.y );
  xy5 = ImVec2 ( io.DisplaySize.x / 2.0f - (io.DisplaySize.x * 0.125f), 0.0f             );

  xy6 = ImVec2 ( io.DisplaySize.x / 2.0f + (io.DisplaySize.x * 0.125f), 0.0f             );
  xy7 = ImVec2 ( io.DisplaySize.x / 2.0f + (io.DisplaySize.x * 0.125f), io.DisplaySize.y );

  xy8 = ImVec2 ( 0.0f,             io.DisplaySize.y/* - io.DisplaySize.y * 0.01f*/);
  xy9 = ImVec2 ( io.DisplaySize.x, io.DisplaySize.y/* - io.DisplaySize.y * 0.01f*/);

  xy10 =ImVec2 ( 0.0f,                               0.0f/*io.DisplaySize.y * 0.01f*/);
  xy11 =ImVec2 ( io.DisplaySize.x,                   0.0f/*io.DisplaySize.y * 0.01f*/);

  draw_list->PushClipRectFullScreen (                                                         );
  draw_list->AddRect                (  xy0, xy1,  col32, 0.0f, 0x00, io.DisplaySize.x * 0.01f );
  draw_list->AddRect                (  xy2, xy3,  col32, 0.0f, 0x00, io.DisplaySize.x * 0.01f );
//draw_list->AddRect                (  xy4, xy5,  col32, 0.0f, 0x00, io.DisplaySize.x * 0.01f );
//draw_list->AddRect                (  xy6, xy7,  col32, 0.0f, 0x00, io.DisplaySize.x * 0.01f );
  draw_list->AddRect                (  xy8, xy9,  col32, 0.0f, 0x00, io.DisplaySize.y * 0.01f );
  draw_list->AddRect                ( xy10, xy11, col32, 0.0f, 0x00, io.DisplaySize.y * 0.01f );
  draw_list->PopClipRect            (                                                         );
#else // Original Tearing Bars
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
#endif

  ImGui::End ();
}