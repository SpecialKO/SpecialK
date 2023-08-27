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

#include <SpecialK/nvapi.h>
#include <SpecialK/render/d3d11/d3d11_core.h>

void
SK_ImGui_DrawGraph_Latency ()
{
  if (! sk::NVAPI::nv_hardware) {
    return;
  }

  static auto& rb =
    SK_GetCurrentRenderBackend ();

  if (! rb.isReflexSupported ())
    return;

  ImGui::BeginGroup ();

  NV_LATENCY_RESULT_PARAMS
    latencyResults         = {                          };
    latencyResults.version = NV_LATENCY_RESULT_PARAMS_VER;

  struct stage_timing_s
  {
    const char* label = nullptr;
    const char*
           desc = nullptr;
    NvU64   min = std::numeric_limits <NvU64>::max (),
            max = std::numeric_limits <NvU64>::min (),
            sum = 0;
    double  avg = 0.0;
    int samples = 0;
    NvU64 start = 0;
    NvU64   end = 0;
    ImColor color;
  } sim      { "Simulation"       }, render  { "Render Submit"   },
    specialk { "Special K"        }, present { "Present"         },
    driver   { "Driver"           }, os      { "OS Render Queue" },
    gpu      { "GPU Render"       },
    total    { "Total Frame Time" },
    input    { "Input Age"        };

  stage_timing_s* stages [] = {
    &sim,     &render, &specialk,
    &present, &driver, &os,
    &gpu,
  };

  specialk.desc = "Includes drawing SK's overlay and framerate limiting";

  float fLegendY = 0.0f; // Anchor pt to align latency timing legend w/ text
  int   id       = 0;

  for ( auto* stage : stages )
  {
    stage->color =
      ImColor::HSV ( ( ++id ) /
        static_cast <float> (
             sizeof ( stages) /
             sizeof (*stages) ), 0.5f,
                                 1.0f );
  }

  if (rb.getLatencyReportNV (&latencyResults))
  {
    for ( auto idx  = 63 ;
               idx >= 0  ;
             --idx )
    {
      auto& frame =
        latencyResults.frameReport
              [idx];

      auto _UpdateStat =
      [&]( NvU64           start,
           NvU64           end,
           stage_timing_s* stage )
      {
        if (start != 0 && end != 0)
        {
          auto
              duration =
                 (
             end - start
                 );
          if (duration >= 0 && duration < static_cast <NvU64> (SK_QpcFreq / 3))
          {
            stage->samples++;

            stage->sum += duration;
            stage->min = static_cast <NvU64>
                       ( (duration < stage->min) ?
                          duration : stage->min );
            stage->max = static_cast <NvU64>
                       ( (duration > stage->max) ?
                          duration : stage->max );

            if (idx == 63)
            {
              stage->start = start;
              stage->end   = end;
            }
          }
        }
      };

      _UpdateStat (frame.simStartTime,           frame.simEndTime,           &sim);
      _UpdateStat (frame.renderSubmitStartTime,  frame.renderSubmitEndTime,  &render);
      _UpdateStat (frame.presentStartTime,       frame.presentEndTime,       &present);
      _UpdateStat (frame.driverStartTime,        frame.driverEndTime,        &driver);
      _UpdateStat (frame.osRenderQueueStartTime, frame.osRenderQueueEndTime, &os);
      _UpdateStat (frame.gpuRenderStartTime,     frame.gpuRenderEndTime,     &gpu);
      _UpdateStat (frame.simStartTime,           frame.gpuRenderEndTime,     &total);
      _UpdateStat (frame.inputSampleTime,        frame.gpuRenderEndTime,     &input);
      _UpdateStat (frame.renderSubmitEndTime,    frame.presentStartTime,     &specialk);
    }

    auto _UpdateAverages = [&](stage_timing_s* stage)
    {
      if (stage->samples != 0)
      {
        stage->avg = static_cast <double> (stage->sum) /
                     static_cast <double> (stage->samples);
      }
    };

    stage_timing_s *min_stage = &sim;
    stage_timing_s *max_stage = &sim;

    for (auto* pStage : stages)
    {
      _UpdateAverages (    pStage     );

      if (min_stage->avg > pStage->avg) {
          min_stage      = pStage;
      }

      if (max_stage->avg < pStage->avg) {
          max_stage      = pStage;
      }
    }

    _UpdateAverages (&input);
    _UpdateAverages (&total);

    if (input.avg > total.avg) {
        input.avg = 0.0;
    }

    ImGui::BeginGroup ();
    for ( auto* pStage : stages )
    {
      ImGui::TextColored (
        pStage == min_stage ? ImColor (0.1f, 1.0f, 0.1f) :
        pStage == max_stage ? ImColor (1.0f, 0.1f, 0.1f) :
                              ImColor (.75f, .75f, .75f),
          "%s", pStage->label );

      if (ImGui::IsItemHovered ())
      {
        if (pStage->desc != nullptr) {
          ImGui::SetTooltip ("%s", pStage->desc);
        }
      }
    }
    ImGui::Separator   ();
    ImGui::TextColored (
      ImColor (1.f, 1.f, 1.f), "Total Frame Time"
    );
    if (input.avg != 0.0) {
    ImGui::TextColored (
      ImColor (1.f, 1.f, 1.f), "Input Age"
    );
    }
    ImGui::EndGroup          ();
    ImGui::SameLine          ();
    ImGui::VerticalSeparator ();
    ImGui::SameLine          (0.0f, 10.0f);
    ImGui::BeginGroup        ();
    ////for (auto* pStage : stages)
    ////{
    ////  ImGui::TextColored (
    ////    ImColor (0.825f, 0.825f, 0.825f),
    ////      "Min / Max / Avg" );
    ////}
    ////ImGui::EndGroup   ();
    ////ImGui::SameLine   (0.0f);
    ////ImGui::BeginGroup ();
    ////for (auto* pStage : stages)
    ////{
    ////  ImGui::TextColored (
    ////    ImColor (0.905f, 0.905f, 0.905f),
    ////      "%llu", pStage->min );
    ////}
    ////ImGui::EndGroup   ();
    ////ImGui::SameLine   (0.0f);
    ////ImGui::BeginGroup ();
    ////for (auto* pStage : stages)
    ////{
    ////  ImGui::TextColored (
    ////    ImColor (0.915f, 0.915f, 0.915f),
    ////      "%llu", pStage->max );
    ////}
    ////ImGui::EndGroup   ();
    ////ImGui::SameLine   (0.0f);
    ////ImGui::BeginGroup ();
    for (auto* pStage : stages)
    {
      ImGui::TextColored (
        pStage == min_stage ? ImColor (0.6f, 1.0f, 0.6f) :
        pStage == max_stage ? ImColor (1.0f, 0.6f, 0.6f) :
                              ImColor (0.9f, 0.9f, 0.9f),
          "%4.2f ms",
            10000.0 * ( pStage->avg /
                          static_cast <double> (SK_QpcFreq)
                      )
      );
    }
    ImGui::Separator   ();

    double frametime = 10000.0 *
      total.avg / static_cast <double> (SK_QpcFreq);

    fLegendY =
      ImGui::GetCursorScreenPos ().y;

    ImGui::TextColored (
      ImColor (1.f, 1.f, 1.f),
        "%4.2f ms",
          frametime, 1000.0 / frametime
                       );

    static bool
        input_sampled = false;
    if (input_sampled || input.avg != 0.0)
    {   input_sampled = true;
      ImGui::TextColored (
        ImColor (1.f, 1.f, 1.f),
          input.avg != 0.0 ?
                "%4.2f ms" : "--",
            10000.0 *
              input.avg / static_cast <double> (SK_QpcFreq)
                         );

      fLegendY +=
        ImGui::GetTextLineHeightWithSpacing () / 2.0f;
    }
    ImGui::EndGroup   ();

    ImGui::SameLine          ();
    ImGui::VerticalSeparator ();
  }

  ImGui::SameLine   (0, 10.0f);
  ImGui::BeginGroup ();

  extern void
  SK_NV_AdaptiveSyncControl ();
  SK_NV_AdaptiveSyncControl ();


  float fMaxWidth  = ImGui::GetContentRegionAvail        ().x;
  float fMaxHeight = ImGui::GetTextLineHeightWithSpacing () * 2.8f;
  float fInset     = fMaxWidth  *  0.025f;
  float X0         = ImGui::GetCursorScreenPos ().x;
  float Y0         = ImGui::GetCursorScreenPos ().y - ImGui::GetTextLineHeightWithSpacing ();
        fMaxWidth *= 0.95f;

  static DWORD                                  dwLastUpdate = 0;
  static float                                  scale        = 1.0f;
  static std::vector <stage_timing_s>           sorted_stages;
  static _NV_LATENCY_RESULT_PARAMS::FrameReport frame_report;

  if (dwLastUpdate < SK::ControlPanel::current_time - 266)
  {   dwLastUpdate = SK::ControlPanel::current_time;

    frame_report =
      latencyResults.frameReport [63];

    sorted_stages.clear ();

    for (auto* stage : stages)
    {
      sorted_stages.push_back (*stage);
    }

    scale = fMaxWidth /
      static_cast <float> ( frame_report.gpuRenderEndTime -
                            frame_report.simStartTime );

    std::sort ( std::begin (sorted_stages),
                std::end   (sorted_stages),
                [](stage_timing_s& a, stage_timing_s& b)
                {
                  return ( a.start < b.start );
                }
              );
  }

  ImDrawList* draw_list =
    ImGui::GetWindowDrawList ();

 ///draw_list->PushClipRect ( //ImVec2 (0.0f, 0.0f), ImVec2 (ImGui::GetIO ().DisplaySize.x, ImGui::GetIO ().DisplaySize.y) );
 ///                          ImVec2 (X0,                                     Y0),
 ///                          ImVec2 (X0 + ImGui::GetContentRegionAvail ().x, Y0 + fMaxHeight) );

  float x  = X0 + fInset;
  float y  = Y0;
  float dY = fMaxHeight / 7.0f;

  for ( auto& kStage : sorted_stages )
  {
    x = X0 + fInset +
    ( kStage.start - frame_report.simStartTime )
                   * scale;

    float duration =
      (kStage.end - kStage.start) * scale;

    ImVec2 r0 (x,            y);
    ImVec2 r1 (x + duration, y + dY);

    draw_list->AddRectFilled ( r0, r1, kStage.color );

    if (ImGui::IsMouseHoveringRect (r0, r1))
    {
      ImGui::SetTooltip ("%s", kStage.label);
    }

    y += dY;
  }

  if (frame_report.inputSampleTime != 0)
  {
    x = X0 + fInset + ( frame_report.inputSampleTime -
                        frame_report.simStartTime )  *  scale;

    draw_list->AddCircle (
      ImVec2  (x, Y0 + fMaxHeight / 2.0f), 4,
      ImColor (1.0f, 1.0f, 1.0f, 1.0f),    12, 2
    );
  }

 ///draw_list->PopClipRect ();

  ImGui::SetCursorScreenPos (
    ImVec2 (ImGui::GetCursorScreenPos ().x, fLegendY)
  );

  for ( auto *pStage : stages )
  {
    ImGui::TextColored (pStage->color, "%s", pStage->label);

    if (ImGui::IsItemHovered ())
    {
      if (pStage->desc != nullptr) {
        ImGui::SetTooltip ("%s", pStage->desc);
      }
    }

    ImGui::SameLine    (0.0f, 12.0f);
  }

  ImGui::Spacing    ();

  ImGui::EndGroup   ();

  if ( ReadULong64Acquire (&SK_Reflex_LastFrameMarked) <
       ReadULong64Acquire (&SK_RenderBackend::frames_drawn) - 1 )
  {
    static bool bRTSS64 =
      SK_GetModuleHandle (L"RTSSHooks64.dll") != 0;

    ImGui::TextColored ( ImColor::HSV (0.1f, 1.f, 1.f),
                           bRTSS64 ? "RivaTuner Statistics Server detected, "
                                     "latency measurements may be inaccurate."
                                   : "No draw calls captured, "
                                     "latency measurements may be inaccurate." );
  }

  ImGui::EndGroup   ();
}

void
SK_ImGui_DrawConfig_Latency ()
{
  static auto& rb =
    SK_GetCurrentRenderBackend ();

  bool bFullReflexSupport =
    rb.isReflexSupported ();

  if (! (sk::NVAPI::nv_hardware && SK_API_IsDXGIBased (rb.api)))
    return;

  ImGui::BeginGroup ();

  int reflex_mode = 0;

  if (config.nvidia.reflex.enable)
  {
    if (       config.nvidia.reflex.low_latency_boost)
    { if (     config.nvidia.reflex.low_latency)
      reflex_mode = 2;
      else
      reflex_mode = 3;
    } else if (config.nvidia.reflex.low_latency) {
      reflex_mode = 1;
    } else {
      reflex_mode = 0;
    }
  }

  else {
    reflex_mode = 0;
  }

  bool show_mode_select = true;

  if (config.nvidia.reflex.native)
  {
    ImGui::Bullet   ();
    ImGui::SameLine ();
    ImGui::TextColored ( ImColor::HSV (0.1f, 1.f, 1.f),
                           "Game is using native Reflex, data shown here may not update every frame." );

    ImGui::Checkbox ( "Override Game's Reflex Mode",
                        &config.nvidia.reflex.override );

    if (ImGui::IsItemHovered ())
    {
      ImGui::BeginTooltip    ();
      ImGui::TextUnformatted ("This may allow you to reduce latency even more in native Reflex games");
      ImGui::Separator       ();
      ImGui::BulletText      ("Minimum Latency: Low Latency + Boost w/ Latency Marker Trained Optimization");
      ImGui::EndTooltip      ();
    }

    show_mode_select = config.nvidia.reflex.override;
  }

  if (show_mode_select)
  {
    // We can actually use "Nothing But Boost" in
    //   situations where Reflex doesn't normally work
    //     such as DXGI/Vulkan Interop SwapChains.
    if (! bFullReflexSupport) reflex_mode =
                              reflex_mode == 0 ? 0
                                               : 1;

    bool selected =
      rb.isReflexSupported () ?
        ImGui::Combo ( "NVIDIA Reflex Mode", &reflex_mode,
                          "Off\0Low Latency\0"
                               "Low Latency + Boost\0"
                                 "Nothing But Boost\0\0" )
                              :
        ImGui::Combo ( "NVIDIA Reflex Mode", &reflex_mode,
                          "Off\0Nothing But Boost\0\0" );

    if (selected)
    {
      // We can actually use "Nothing But Boost" in
      //   situations where Reflex doesn't normally work
      //     such as DXGI/Vulkan Interop SwapChains.
      if (! bFullReflexSupport) reflex_mode =
                                reflex_mode == 0 ? 0
                                                 : 2;

      switch (reflex_mode)
      {
        case 0:
          config.nvidia.reflex.enable            = false;
          config.nvidia.reflex.low_latency       = false;
          config.nvidia.reflex.low_latency_boost = false;
          break;

        case 1:
          config.nvidia.reflex.enable            = true;
          config.nvidia.reflex.low_latency       = true;
          config.nvidia.reflex.low_latency_boost = false;
          break;

        case 2:
          config.nvidia.reflex.enable            = true;
          config.nvidia.reflex.low_latency       = true;
          config.nvidia.reflex.low_latency_boost = true;
          break;

        case 3:
          config.nvidia.reflex.enable            =  true;
          config.nvidia.reflex.low_latency       = false;
          config.nvidia.reflex.low_latency_boost =  true;
          break;
      }

      rb.driverSleepNV (config.nvidia.reflex.enforcement_site);
    }

    if (ImGui::IsItemHovered ())
      ImGui::SetTooltip ("NOTE: Reflex has greatest impact on G-Sync users -- it may lower peak framerate to minimize latency.");
  }

  if (config.nvidia.reflex.enable && config.nvidia.reflex.low_latency && (! config.nvidia.reflex.native) && bFullReflexSupport)
  {
    config.nvidia.reflex.enforcement_site =
      std::clamp (config.nvidia.reflex.enforcement_site, 0, 1);

    ImGui::Combo ( "NVIDIA Reflex Trigger Point", &config.nvidia.reflex.enforcement_site,
                     "End-of-Frame\0Start-of-Frame\0" );

  //bool unlimited =
  //  config.nvidia.reflex.frame_interval_us == 0;
  //
  //if (ImGui::Checkbox ("Use Unlimited Reflex FPS", &unlimited))
  //{
  //  extern float __target_fps;
  //
  //  if (unlimited) config.nvidia.reflex.frame_interval_us = 0;
  //  else           config.nvidia.reflex.frame_interval_us =
  //           static_cast <UINT> ((1000.0 / __target_fps) * 1000.0);
  //}
  }
  
  if ( config.nvidia.reflex.enable            &&
       config.nvidia.reflex.low_latency_boost && ((! config.nvidia.reflex.native) || config.nvidia.reflex.override)
                                              && rb.isReflexSupported () )
  {
    ImGui::Checkbox ("Use Latency Marker Trained Optimization", &config.nvidia.reflex.marker_optimization);
  }
  ImGui::EndGroup   ();
  ImGui::Separator  ();
}

class SKWG_Latency : public SK_Widget
{
public:
  SKWG_Latency () noexcept : SK_Widget ("Latency Analysis")
  {
    SK_ImGui_Widgets->latency = this;

    setResizable    (                false).setAutoFit (true).setMovable (false).
    setDockingPoint (DockAnchor::SouthWest).setVisible (false);
  };

  void load (iSK_INI* cfg) noexcept override
  {
    SK_Widget::load (cfg);
  }

  void save (iSK_INI* cfg) noexcept override
  {
    if (cfg == nullptr) {
      return;
    }

    SK_Widget::save (cfg);

    cfg->write ();
  }

  void run () noexcept override
  {
    static auto* cp =
      SK_GetCommandProcessor ();

    static volatile LONG init = 0;

    if (! InterlockedCompareExchange (&init, 1, 0))
    {
      const float ui_scale = ImGui::GetIO ().FontGlobalScale;

      setMinSize (ImVec2 (50.0f * ui_scale, 50.0f * ui_scale));
    }

    if (ImGui::GetFont () == nullptr) {
      return;
    }
  }

  void draw () override
  {
    if (ImGui::GetFont () == nullptr) {
      return;
    }

    // Prevent showing this widget if conditions are not met
    if (! SK_GetCurrentRenderBackend ().isReflexSupported ())
      setVisible (false);

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

    SK_ImGui_DrawGraph_Latency ();
  }


  void config_base () noexcept override
  {
    SK_Widget::config_base ();

    ImGui::Separator ();
    ImGui::TreePush  ();

    SK_ImGui_DrawConfig_Latency ();

    ImGui::TreePop   ();
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
};

SK_LazyGlobal <SKWG_Latency> __latency__;

void SK_Widget_InitLatency ()
{
  SK_RunOnce (__latency__.get ());
}