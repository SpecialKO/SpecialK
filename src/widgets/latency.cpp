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

#include <implot/implot.h>
#include <imgui/font_awesome.h>

extern iSK_INI* osd_ini;

#include <SpecialK/nvapi.h>
#include <SpecialK/render/d3d11/d3d11_core.h>

namespace ImPlot {

template <typename T>
inline T RandomRange(T min, T max) {
    T scale = rand() / (T) RAND_MAX;
    return min + scale * ( max - min );
}

}

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

    double durations [64] = { };

    void reset (void)
    {
      min     = std::numeric_limits <NvU64>::max (),
      max     = std::numeric_limits <NvU64>::min (),
      sum     = 0;
      avg     = 0.0;
      samples = 0;
      start   = 0;
      end     = 0;

      memset (durations, 0, sizeof (double) * 64);
    }
  };

  struct frame_timing_s {
    NvU32 gpuActiveRenderTimeUs;
    NvU32 gpuFrameTimeUs;
  } gpu_frame_times [64];
  
  static stage_timing_s sim      { "Simulation"       }; static stage_timing_s render   { "Render Submit"   };
  static stage_timing_s specialk { "Special K"        }; static stage_timing_s present  { "Present"         };
  static stage_timing_s driver   { "Driver"           }; static stage_timing_s os       { "OS Render Queue" };
  static stage_timing_s gpu      { "GPU Render"       }; static stage_timing_s gpu_busy { "GPU Busy"        };
  static stage_timing_s total    { "Total Frame Time" };
  static stage_timing_s input    { "Input Age"        };

  total.reset ();
  input.reset ();

  stage_timing_s* stages [] = {
    &sim,     &render, &specialk,
    &present, &driver, &os,
    &gpu
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

    stage->reset ();
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

      gpu_frame_times [idx].gpuActiveRenderTimeUs = frame.gpuActiveRenderTimeUs;
      gpu_frame_times [idx].gpuFrameTimeUs        = frame.gpuFrameTimeUs;

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
          if (duration >= 0 && duration < 666666)
          {
            stage->samples++;

            stage->durations [idx] = static_cast <double> (duration) / 1000.0;

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

      _UpdateStat (frame.simStartTime,           frame.simEndTime,            &sim);
      _UpdateStat (frame.renderSubmitStartTime,  frame.renderSubmitEndTime,   &render);
      _UpdateStat (frame.presentStartTime,       frame.presentEndTime,        &present);
      _UpdateStat (frame.driverStartTime,        frame.driverEndTime,         &driver);
      _UpdateStat (frame.osRenderQueueStartTime, frame.osRenderQueueEndTime,  &os);
      _UpdateStat (frame.gpuRenderStartTime,     frame.gpuRenderEndTime,      &gpu);
      _UpdateStat (frame.simStartTime,           frame.gpuRenderEndTime,      &total);
      _UpdateStat (frame.inputSampleTime,        frame.gpuRenderEndTime,      &input);
      _UpdateStat (frame.renderSubmitEndTime,    frame.presentStartTime,      &specialk);
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

    gpu_busy.start = latencyResults.frameReport [63].gpuRenderEndTime - gpu_frame_times [63].gpuActiveRenderTimeUs;
    gpu_busy.end   = latencyResults.frameReport [63].gpuRenderEndTime;
    gpu_busy.avg   = static_cast <double> (gpu_busy.end - gpu_busy.start);

    _UpdateAverages (&input);
    _UpdateAverages (&total);

    if (input.avg > total.avg) {
        input.avg = 0.0;
    }

    struct reflex_frame_s {
      double simulation;
      double render_submit;
      double gpu_total;
      double gpu_active;
      double gpu_start;
      
      struct {
        double cpu0, gpu0;
        double cpu1, gpu1;
      } fill;
    } static reflex;

    static double dCPU = 0.0;
    static double dGPU = 0.0;

    static double dMaxStage = 0.0;

    reflex.simulation    = sim.durations    [63];
    reflex.render_submit = render.durations [63];
    reflex.gpu_total     = gpu.durations    [63];
    reflex.gpu_active    = static_cast <double> (gpu_frame_times [63].gpuActiveRenderTimeUs) / 1000.0;
    reflex.gpu_start     = reflex.simulation +
                           reflex.render_submit; // Start of GPU-only workload

    // 5% margin to prevent rapid graph shading inversion
    if (reflex.gpu_active * 1.05 < reflex.gpu_start)
    {
      reflex.fill.cpu0 = reflex.gpu_start;
      reflex.fill.cpu1 = reflex.gpu_active;
      reflex.fill.gpu0 = reflex.gpu_active;
      reflex.fill.gpu1 = 0.0;
    }

    else
    {
      reflex.fill.cpu0 = 0.0;
      reflex.fill.cpu1 = reflex.gpu_start;
      reflex.fill.gpu0 = reflex.gpu_start;
      reflex.fill.gpu1 = reflex.gpu_active;
    }

    static auto lastFrame =
      SK_GetFramesDrawn () - 1;

    static constexpr auto _MAX_FRAME_HISTORY = 512;

    struct { double 
                  simulation [_MAX_FRAME_HISTORY],
               render_submit [_MAX_FRAME_HISTORY], gpu_total  [_MAX_FRAME_HISTORY],
                  gpu_active [_MAX_FRAME_HISTORY], gpu_start  [_MAX_FRAME_HISTORY],
                   fill_cpu0 [_MAX_FRAME_HISTORY], fill_gpu0  [_MAX_FRAME_HISTORY],
                   fill_cpu1 [_MAX_FRAME_HISTORY], fill_gpu1  [_MAX_FRAME_HISTORY];
           INT64 sample_time [_MAX_FRAME_HISTORY];
      int frames = 0;
    } static frame_history;

    static double  frame_id [_MAX_FRAME_HISTORY],
                 simulation [_MAX_FRAME_HISTORY],
              render_submit [_MAX_FRAME_HISTORY], gpu_total  [_MAX_FRAME_HISTORY],
                 gpu_active [_MAX_FRAME_HISTORY], gpu_start  [_MAX_FRAME_HISTORY],
                  fill_cpu0 [_MAX_FRAME_HISTORY], fill_gpu0  [_MAX_FRAME_HISTORY],
                  fill_cpu1 [_MAX_FRAME_HISTORY], fill_gpu1  [_MAX_FRAME_HISTORY];

    static int num_frames = 0;
    static double dScale  = 0.0;

    if (std::exchange (lastFrame, SK_GetFramesDrawn ()) < SK_GetFramesDrawn ())
    {
      num_frames = 0;

      dCPU       = 0.0;
      dGPU       = 0.0;

      dMaxStage  = 0.0;

      auto frame_idx =
        (frame_history.frames++ % _MAX_FRAME_HISTORY);

      frame_history.simulation    [frame_idx] = reflex.simulation;
      frame_history.render_submit [frame_idx] = reflex.render_submit;
      frame_history.gpu_total     [frame_idx] = reflex.gpu_total;
      frame_history.gpu_active    [frame_idx] = reflex.gpu_active;
      frame_history.gpu_start     [frame_idx] = reflex.gpu_start;
      frame_history.fill_cpu0     [frame_idx] = reflex.fill.cpu0;
      frame_history.fill_gpu0     [frame_idx] = reflex.fill.gpu0;
      frame_history.fill_cpu1     [frame_idx] = reflex.fill.cpu1;
      frame_history.fill_gpu1     [frame_idx] = reflex.fill.gpu1;
      frame_history.sample_time   [frame_idx] = SK_QueryPerf ().QuadPart;

      const auto qpcNow =
        SK_QueryPerf ().QuadPart;

      for ( int i = frame_idx ; i >= 0 ; --i )
      {
        if (frame_history.sample_time [i] > qpcNow - SK_QpcFreq)
        {
          simulation    [num_frames] = frame_history.simulation    [i];
          render_submit [num_frames] = frame_history.render_submit [i];
          gpu_total     [num_frames] = frame_history.gpu_total     [i];
          gpu_active    [num_frames] = frame_history.gpu_active    [i];
          gpu_start     [num_frames] = frame_history.gpu_start     [i];
          fill_cpu0     [num_frames] = frame_history.fill_cpu0     [i];
          fill_gpu0     [num_frames] = frame_history.fill_gpu0     [i];
          fill_cpu1     [num_frames] = frame_history.fill_cpu1     [i];
          fill_gpu1     [num_frames] = frame_history.fill_gpu1     [i];
          frame_id      [num_frames] = num_frames;

          dCPU += gpu_start  [num_frames];
          dGPU += gpu_active [num_frames];

          dMaxStage = std::max (simulation    [num_frames], dMaxStage);
          dMaxStage = std::max (render_submit [num_frames], dMaxStage);
          dMaxStage = std::max (gpu_total     [num_frames], dMaxStage);
          dMaxStage = std::max (gpu_active    [num_frames], dMaxStage);
          dMaxStage = std::max (gpu_start     [num_frames], dMaxStage);

          num_frames++;
        }
      }

      for ( int i = _MAX_FRAME_HISTORY-1 ; i > frame_idx ; --i )
      {
        if (frame_history.sample_time [i] > qpcNow - SK_QpcFreq)
        {
          simulation    [num_frames] = frame_history.simulation    [i];
          render_submit [num_frames] = frame_history.render_submit [i];
          gpu_total     [num_frames] = frame_history.gpu_total     [i];
          gpu_active    [num_frames] = frame_history.gpu_active    [i];
          gpu_start     [num_frames] = frame_history.gpu_start     [i];
          fill_cpu0     [num_frames] = frame_history.fill_cpu0     [i];
          fill_gpu0     [num_frames] = frame_history.fill_gpu0     [i];
          fill_cpu1     [num_frames] = frame_history.fill_cpu1     [i];
          fill_gpu1     [num_frames] = frame_history.fill_gpu1     [i];
          frame_id      [num_frames] = num_frames;

          dCPU += gpu_start  [num_frames];
          dGPU += gpu_active [num_frames];

          dMaxStage = std::max (simulation    [num_frames], dMaxStage);
          dMaxStage = std::max (render_submit [num_frames], dMaxStage);
          dMaxStage = std::max (gpu_total     [num_frames], dMaxStage);
          dMaxStage = std::max (gpu_active    [num_frames], dMaxStage);
          dMaxStage = std::max (gpu_start     [num_frames], dMaxStage);

          num_frames++;
        }
      }

      // Reverse the data direction
      for ( int i = 0 ; i < num_frames ; ++i )
      {
        frame_id [i] = num_frames - 1 - i;
      }

      dCPU /= num_frames;
      dGPU /= num_frames;

      dMaxStage =
        std::max (10000.0 * total.avg / static_cast <double> (SK_QpcFreq), dMaxStage);

      static constexpr auto              _HistorySize  = 32;
      static double       dScaleHistory [_HistorySize] = { 0.0 };
      static unsigned int iScaleIdx                    = 0;

      double dAvgScale =
        std::accumulate ( &dScaleHistory [0],
                          &dScaleHistory [_HistorySize - 1], 0.0 ) /
                    static_cast <double> (_HistorySize);

      dScaleHistory [iScaleIdx++ % _HistorySize] =
        (dMaxStage * 1.25f + dAvgScale * .75f) / 2.0;

      dScale =
        std::max (dMaxStage, dAvgScale);

      static double
                dLastScale = dScale;
      dScale = (dLastScale * 1.75 + dScale * .25) / 2.0;
                dLastScale = dScale;
    }

    static bool show_lines = true;
    static bool show_fills = true;

    if (ImPlot::BeginPlot ("##Stage Time", ImVec2 (-1, 0), ImPlotFlags_NoTitle | ImPlotFlags_NoInputs | ImPlotFlags_NoMouseText
                                                                               | ImPlotFlags_NoMenus  | ImPlotFlags_NoBoxSelect))
    {
      auto flags =
        ImPlotAxisFlags_NoMenus | ImPlotAxisFlags_AutoFit;

      auto FramerateFormatter = [](double milliseconds, char* buff, int size, void*) -> int
      {
        const auto fps =
          static_cast <unsigned int> (1000.0/milliseconds);

        return (milliseconds <= 0.0 || fps == 0) ?
          snprintf (buff, size, " ")             :
          snprintf (buff, size, "%u fps", fps);
      };

      auto MillisecondFormatter = [](double milliseconds, char* buff, int size, void*) -> int
      {
        return (milliseconds <= 0.0) ?
          snprintf (buff, size, " ") :
          snprintf (buff, size, "%4.1f ms", milliseconds);
      };
      
      ImPlot::SetupAxes       ("Frame", "Milliseconds", flags | ImPlotAxisFlags_NoLabel      |
                                                                ImPlotAxisFlags_NoTickLabels |
                                                                ImPlotAxisFlags_NoTickMarks  |
                                                                ImPlotAxisFlags_NoGridLines,
                                                       (flags &~ImPlotAxisFlags_AutoFit)|
                                                                ImPlotAxisFlags_NoLabel);
      ImPlot::SetupAxisLimits (ImAxis_X1, 0, num_frames-1,      ImPlotCond_Always);
      ImPlot::SetupAxisLimits (ImAxis_Y1, 0, dScale,            ImPlotCond_Always);
      ImPlot::SetupAxisFormat (ImAxis_Y1, MillisecondFormatter);
      ImPlot::SetupLegend     (ImPlotLocation_SouthWest,        ImPlotLegendFlags_Horizontal);

      ImPlot::SetupAxis       (ImAxis_Y2, nullptr,              ImPlotAxisFlags_AuxDefault |
                                                                ImPlotAxisFlags_NoLabel);
      ImPlot::SetupAxisLimits (ImAxis_Y2, 0, dScale,            ImPlotCond_Always);
      ImPlot::SetupAxisFormat (ImAxis_Y2, FramerateFormatter);

      if (show_lines)
      {
        ImPlot::PlotLine ("Simulation",      frame_id, simulation,    num_frames);
        ImPlot::PlotLine ("Render Submit",   frame_id, render_submit, num_frames);
        ImPlot::PlotLine ("Display Scanout", frame_id, gpu_total,     num_frames);
        ImPlot::PlotLine ("GPU Busy",        frame_id, gpu_active,    num_frames);
        ImPlot::PlotLine ("CPU Busy",        frame_id, gpu_start,     num_frames);
      }

      if (show_fills)
      {
        static constexpr
          auto dragline_flags =
            ImPlotDragToolFlags_NoFit    |
            ImPlotDragToolFlags_NoInputs |
            ImPlotDragToolFlags_NoCursors;

        ImPlot::DragLineY (0, &dGPU, ImVec4 (1,0,0,1), 3.333, dragline_flags);
        ImPlot::DragLineY (0, &dCPU, ImVec4 (0,0,1,1), 3.333, dragline_flags);

        static std::string utf8_gpu_name;
        static std::string utf8_cpu_name;

        SK_RunOnce (
        {
          wchar_t    wszGPU [128] = { };
          wcsncpy_s (wszGPU, 128, sk::NVAPI::EnumGPUs_DXGI()[0].Description,_TRUNCATE);
          char        szCPU [ 64] = { };
          strncpy_s ( szCPU,  64, InstructionSet::Brand().c_str(),          _TRUNCATE);

          PathRemoveBlanksA ( szCPU);
          PathRemoveBlanksW (wszGPU);

          utf8_gpu_name =
            SK_WideCharToUTF8 (wszGPU);
          utf8_cpu_name =       szCPU;
        });

        double dFpsCpu = isfinite (1000.0 / dCPU) ? 1000.0 / dCPU : 0.0;
        double dFpsGpu = isfinite (1000.0 / dGPU) ? 1000.0 / dGPU : 0.0;

        ImPlot::Annotation (0.0,         dScale, ImVec4 (.75,0,0,1), ImVec2 (0.0,0.0), true, "%hs",                                                                   utf8_gpu_name.c_str ());
        ImPlot::Annotation (num_frames-1,dScale, ImVec4 (0,0,.75,1), ImVec2 (-ImGui::CalcTextSize (SK_FormatString ("%.1f ", dFpsCpu).c_str ()).x, 0.0), true, "%hs", utf8_cpu_name.c_str ());
        ImPlot::PushStyleColor (ImPlotCol_InlayText, ImVec4 (1,1,1,1));
        ImPlot::Annotation (0.0,         dScale, ImVec4 (1,1,1,1), ImVec2 ( ImGui::CalcTextSize ( utf8_gpu_name.c_str () ).x + 
                                                                            ImGui::CalcTextSize (" ")                     .x,0.0), true, "%.1f", dFpsGpu);
        ImPlot::Annotation (num_frames-1,dScale, ImVec4 (1,1,1,1), ImVec2 (0.0,0.0),                                               true, "%.1f", dFpsCpu);
        ImPlot::PopStyleColor ();

        if      (dGPU > dCPU * 1.15)
          ImPlot::TagY (dGPU, ImVec4 (1,0,0,1), "GPU Bound");
        else if (dCPU > dGPU * 1.15)
          ImPlot::TagY (dCPU, ImVec4 (0,0,1,1), "CPU Bound");
        else
          ImPlot::TagY ((dCPU + dGPU) / 2.0, ImVec4 (0,1,0,1),"\tBalanced");

        ImPlot::PushStyleVar (ImPlotStyleVar_FillAlpha, 0.15f);
        ImPlot::PlotShaded   ("GPU Busy", frame_id, fill_gpu0, fill_gpu1, num_frames, flags);
        ImPlot::PlotShaded   ("CPU Busy", frame_id, fill_cpu0, fill_cpu1, num_frames, flags);
        ImPlot::PopStyleVar  ();
      }

      ImPlot::EndPlot ();
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
    ImGui::EndGroup    ();
    ImGui::SameLine    ();
    ImGui::SeparatorEx (ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine    (0.0f, 10.0f);
    ImGui::BeginGroup  ();
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
            pStage->avg / 1000.0
      );
    }
    ImGui::Separator   ();

    double frametime = total.avg / 1000.0;

    fLegendY =
      ImGui::GetCursorScreenPos ().y;

    ImGui::TextColored (
      ImColor (1.f, 1.f, 1.f),
        "%4.2f ms",
          frametime    );

    static bool
        input_sampled = false;
    if (input_sampled || input.avg != 0.0)
    {   input_sampled = true;
      ImGui::TextColored (
        ImColor (1.f, 1.f, 1.f),
          input.avg != 0.0 ?
                "%4.2f ms" : "--",
              input.avg / 1000.0
                         );

      fLegendY +=
        ImGui::GetTextLineHeightWithSpacing () / 2.0f;
    }
    ImGui::EndGroup    ();

    ImGui::SameLine    ();
    ImGui::SeparatorEx (ImGuiSeparatorFlags_Vertical);
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

    if (show_mode_select)
    {
      ImGui::SameLine    ();
      ImGui::SeparatorEx (ImGuiSeparatorFlags_Vertical);
      ImGui::SameLine    ();
    }
  }

  if (show_mode_select)
  {
    // We can actually use "Nothing But Boost" in
    //   situations where Reflex doesn't normally work
    //     such as DXGI/Vulkan Interop SwapChains.
    if (! bFullReflexSupport) reflex_mode =
                              reflex_mode == 0 ? 0
                                               : 1;

    ImGui::BeginGroup      ();
    ImGui::Spacing         ();
    ImGui::TextUnformatted ("Reflex Mode");
    ImGui::EndGroup        ();
    ImGui::SameLine        ();

    ImGui::SetNextItemWidth (ImGui::GetContentRegionAvail ().x / 3.0f);

    bool selected =
      rb.isReflexSupported () ?
        ImGui::Combo ( "###Reflex Mode", &reflex_mode,
                          "Off\0Low Latency\0"
                               "Low Latency + Boost\0"
                                 "Nothing But Boost\0\0" )
                              :
        ImGui::Combo ( "###Reflex Mode", &reflex_mode,
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

    if (ImGui::IsItemHovered () && config.nvidia.reflex.low_latency)
    {
      double dRefreshRate =
        static_cast <double> (rb.displays [rb.active_display].signal.timing.vsync_freq.Numerator) /
        static_cast <double> (rb.displays [rb.active_display].signal.timing.vsync_freq.Denominator);

      ImGui::BeginTooltip ();
      ImGui::TextColored     (ImVec4 (0.333f, 0.666f, 0.999f, 1.f), ICON_FA_INFO_CIRCLE);
      ImGui::SameLine        ();
      ImGui::PushStyleColor  (ImGuiCol_Text, ImVec4 (1.f, 1.f, 1.f, 1.f));
      ImGui::TextUnformatted ("Reflex Low Latency FPS Limit (VRR)");
      ImGui::PushStyleColor  (ImGuiCol_Text, ImVec4 (.6f, .6f, .6f, 1.f));
      ImGui::Spacing         ();
      ImGui::Text            ("\tRefresh (%4.1f Hz) - Refresh (%4.1f Hz) * Refresh (%4.1f Hz) / 3600.0",
                                dRefreshRate,        dRefreshRate,        dRefreshRate);
      ImGui::PushStyleColor  (ImGuiCol_Text, ImVec4 (.8f, .8f, .8f, 1.f));
      ImGui::Spacing         ();
      ImGui::Text            (       "\t\tReflexFPS:\t%4.1f FPS",
                                dRefreshRate        -dRefreshRate        *dRefreshRate        / 3600.0);
      ImGui::PopStyleColor   (3);
      ImGui::Separator       ();
      ImGui::TextUnformatted ("To pace VRR framerate while using Reflex, set third-party limits below ReflexFPS");
      ImGui::EndTooltip      ();
    }
  }

  if (config.nvidia.reflex.enable && config.nvidia.reflex.low_latency && (! config.nvidia.reflex.native) && bFullReflexSupport)
  {
    config.nvidia.reflex.enforcement_site =
      std::clamp (config.nvidia.reflex.enforcement_site, 0, 1);

    ImGui::SameLine ();

    ImGui::SetNextItemWidth (ImGui::GetContentRegionAvail ().x / 3.0f);

    ImGui::Combo ( "Trigger Point", &config.nvidia.reflex.enforcement_site,
                     "End-of-Frame\0Start-of-Frame\0" );

    if (ImGui::IsItemHovered ())
    {
      ImGui::BeginTooltip    ();
      ImGui::PushStyleColor  (ImGuiCol_Text, ImVec4 (1.f, 1.f, 1.f, 1.f));
      ImGui::TextUnformatted ("Start of Frame");
      ImGui::PushStyleColor  (ImGuiCol_Text, ImVec4 (.7f, .7f, .7f, 1.f));
      ImGui::BulletText      ("Minimizes Latency");
      ImGui::PopStyleColor   (2);
      ImGui::Spacing         ();
      ImGui::Spacing         ();
      ImGui::PushStyleColor  (ImGuiCol_Text, ImVec4 (1.f, 1.f, 1.f, 1.f));
      ImGui::TextUnformatted ("End of Frame");
      ImGui::PushStyleColor  (ImGuiCol_Text, ImVec4 (.7f, .7f, .7f, 1.f));
      ImGui::BulletText      ("Stabilizes Framerate");
      ImGui::PopStyleColor   (2);
      ImGui::EndTooltip      ();
    }

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
       config.nvidia.reflex.low_latency       &&
       config.nvidia.reflex.low_latency_boost && ((! config.nvidia.reflex.native) || config.nvidia.reflex.override)
                                              && rb.isReflexSupported () )
  {
    ImGui::SameLine ();
    ImGui::Checkbox ("Boost Using Latency Markers", &config.nvidia.reflex.marker_optimization);

    if (ImGui::IsItemHovered ())
    {
      ImGui::SetTooltip (
        "Uses timing data collected by SK (i.e. game's first draw call) to aggressively reduce latency"
        " at the expense of frame pacing in boost mode."
      );
    }
  }
  ImGui::EndGroup   ();
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

    ImGui::Separator (  );
    ImGui::TreePush  ("");

    SK_ImGui_DrawConfig_Latency ();

    ImGui::TreePop   (  );
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