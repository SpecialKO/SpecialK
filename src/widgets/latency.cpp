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

#include <immintrin.h>

extern iSK_INI* osd_ini;

#include <SpecialK/nvapi.h>
#include <SpecialK/render/d3d11/d3d11_core.h>

struct reflex_frame_s {
  float simulation    = 0.0f;
  float render_submit = 0.0f;
  float frame_total   = 0.0f;
  float gpu_active    = 0.0f;
  float gpu_start     = 0.0f;
  float present       = 0.0f;
  
  struct {
    float cpu0 = 0.0f, gpu0 = 0.0f;
    float cpu1 = 0.0f, gpu1 = 0.0f;
  } fill;
} static reflex;

static constexpr auto _MAX_FRAME_HISTORY = 384;

#pragma warning (disable:4324)
struct alignas (64) {
  float  simulation [_MAX_FRAME_HISTORY+1] = { },
      render_submit [_MAX_FRAME_HISTORY+1] = { }, frame_total [_MAX_FRAME_HISTORY+1] = { },
         gpu_active [_MAX_FRAME_HISTORY+1] = { }, gpu_start   [_MAX_FRAME_HISTORY+1] = { },
          fill_cpu0 [_MAX_FRAME_HISTORY+1] = { }, fill_gpu0   [_MAX_FRAME_HISTORY+1] = { },
          fill_cpu1 [_MAX_FRAME_HISTORY+1] = { }, fill_gpu1   [_MAX_FRAME_HISTORY+1] = { },
          max_stage [_MAX_FRAME_HISTORY+1] = { };
  INT64 sample_time [_MAX_FRAME_HISTORY+1] = { };
  float  sample_age [_MAX_FRAME_HISTORY+1] = { };
  int   frames = 0;
} static frame_history,
         frame_graph;

struct stage_timing_s
{
  const char* label = nullptr;
  const char*
         desc = nullptr;
  NvU64   min = std::numeric_limits <NvU64>::max (),
          max = std::numeric_limits <NvU64>::min (),
          sum = 0;
  float   avg = 0.0f;
  int samples = 0;
  NvU64 start = 0;
  NvU64   end = 0;
  ImColor color;

  float durations [64] = { };

  void reset (void)
  {
    min     = std::numeric_limits <NvU64>::max (),
    max     = std::numeric_limits <NvU64>::min (),
    sum     = 0;
    avg     = 0.0f;
    samples = 0;
    start   = 0;
    end     = 0;

    memset (durations, 0, sizeof (float) * 64);
  }
};

struct frame_timing_s {
  NvU32 gpuActiveRenderTimeUs = 0;
  NvU32 gpuFrameTimeUs        = 0;
} static gpu_frame_times [64];

static stage_timing_s sim      { "Simulation"       }; static stage_timing_s render   { "Render Submit"   };
static stage_timing_s specialk { "Special K"        }; static stage_timing_s present  { "Present"         };
static stage_timing_s driver   { "Driver"           }; static stage_timing_s os       { "OS Render Queue" };
static stage_timing_s gpu      { "GPU Render"       }; static stage_timing_s gpu_busy { "GPU Busy"        };
static stage_timing_s total    { "Total Frame Time" };
static stage_timing_s input    { "Input Age"        };

NvU64
SK_Reflex_GetFrameEndTime (const _NV_LATENCY_RESULT_PARAMS::FrameReport& report)
{
  return
    std::max ( { report.simEndTime,           report.renderSubmitEndTime,
                 report.presentEndTime,       report.driverEndTime,
                 report.osRenderQueueEndTime, report.gpuRenderEndTime } );
}

void
SK_ImGui_DrawGraph_Latency (bool predraw)
{
  if (! sk::NVAPI::nv_hardware) {
    return;
  }

  static auto& rb =
    SK_GetCurrentRenderBackend ();

  if (! rb.isReflexSupported ())
    return;

  static auto lastWidgetFrame = 0ULL;
  static auto lastDataFrame   =
    SK_GetFramesDrawn () - 1;

  static float fScale = 0.0f;

  static double dCPU = 0.0;
  static double dGPU = 0.0;

  static float fMaxAge   = 0.0f;
  static float fMaxStage = 0.0f;

  static auto& graph   = frame_graph;
  static auto& history = frame_history;

  static stage_timing_s *min_stage = &sim;
  static stage_timing_s *max_stage = &sim;

  static NV_LATENCY_RESULT_PARAMS
    latencyResults         = {                          };
    latencyResults.version = NV_LATENCY_RESULT_PARAMS_VER;

  static float fLegendY = 0.0f; // Anchor pt to align latency timing legend w/ text

  static stage_timing_s* stages [] = {
    &sim,     &render, &specialk,
    &present, &driver, &os,
    &gpu
  };

  if (! predraw)
  {
    lastWidgetFrame =
      SK_GetFramesDrawn ();
  }

  struct span_s {
    int tail = 0;
    int head = 0;
  } static span0,
           span1;

  // Precache data if the widget has recently been drawn, otherwise skip...
  if ( predraw &&               (lastWidgetFrame > SK_GetFramesDrawn ()  - 240)
               && std::exchange (lastDataFrame,    SK_GetFramesDrawn ()) < SK_GetFramesDrawn () )
  {
    latencyResults         = {                          };
    latencyResults.version = NV_LATENCY_RESULT_PARAMS_VER;

    total.reset ();
    input.reset ();

    specialk.desc = "Includes drawing SK's overlay and framerate limiting";

    int id = 0;

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
        const auto& frame =
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
            if (duration >= 0 && duration < 6666666)
            {
              stage->samples++;

              stage->durations [idx] = static_cast <float> (duration) / 1000.0f;

              stage->sum += duration;
              stage->min = (duration < stage->min) ?
                            duration : stage->min;
              stage->max = (duration > stage->max) ?
                            duration : stage->max;

              if (idx == 63)
              {
                stage->start = start;
                stage->end   = end;
              }
            }

            else
            {
              stage->durations [idx] = 0.0;
            }
          }
        };

        auto end =
          SK_Reflex_GetFrameEndTime (frame);

        _UpdateStat (frame.simStartTime,           frame.simEndTime,           &sim);
        _UpdateStat (frame.renderSubmitStartTime,  frame.renderSubmitEndTime,  &render);
        _UpdateStat (frame.presentStartTime,       frame.presentEndTime,       &present);
        _UpdateStat (frame.driverStartTime,        frame.driverEndTime,        &driver);
        _UpdateStat (frame.osRenderQueueStartTime, frame.osRenderQueueEndTime, &os);
        _UpdateStat (frame.gpuRenderStartTime,     frame.gpuRenderEndTime,     &gpu);
        _UpdateStat (frame.simStartTime,           end,                        &total);
        _UpdateStat (frame.inputSampleTime,        end,                        &input);
        _UpdateStat (frame.renderSubmitEndTime,    frame.presentStartTime,     &specialk);
      }

      auto _UpdateAverages = [&](stage_timing_s* stage)
      {
        if (stage->samples != 0)
        {
          stage->avg = static_cast <float> (stage->sum) /
                       static_cast <float> (stage->samples);
        }
      };

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
      gpu_busy.avg   = static_cast <float> (gpu_busy.end - gpu_busy.start);

      _UpdateAverages (&input);
      _UpdateAverages (&total);

      if (input.avg > total.avg) {
          input.avg = 0.0f;
      }

      reflex.simulation    = sim.durations     [63];
      reflex.render_submit = render.durations  [63];
      reflex.present       = present.durations [63];
      reflex.gpu_active    = static_cast <float> (gpu_frame_times [63].gpuActiveRenderTimeUs) / 1000.0f;
      reflex.frame_total   = static_cast <float> (os.end - sim.start) / 1000.0f;
      //// Workaround for Unreal Engine, it actually submits GPU work in its "Present" marker
      //reflex.gpu_start     = ( ( reflex.simulation > reflex.present ) ?
      //                           reflex.simulation : reflex.present ) +
      //                           reflex.render_submit; // Start of GPU-only workload

      reflex.gpu_start = reflex.simulation + reflex.render_submit + reflex.present;

      //SK_LOGs0 ( L"Test",
      //  L"Simulation: %.1f ms, Render: %.1f ms, Present: %.1f ms : CPU %.1f fps", reflex.simulation, reflex.render_submit, reflex.present,
      //    1000.0f / reflex.gpu_start
      //);

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

      const auto qpcNow =
        SK_QueryPerf ().QuadPart;

      frame_graph.frames = 0;

      dCPU       = 0.0;
      dGPU       = 0.0;

      fMaxStage  = 0.0f;
      fMaxAge    = 0.0f;

      auto frame_idx =
        (history.frames++ % _MAX_FRAME_HISTORY);

      history.simulation    [frame_idx] = reflex.simulation;
      history.render_submit [frame_idx] = reflex.render_submit;
      history.frame_total   [frame_idx] = reflex.frame_total;
      history.gpu_active    [frame_idx] = reflex.gpu_active;
      history.gpu_start     [frame_idx] = reflex.gpu_start;
      history.fill_cpu0     [frame_idx] = reflex.fill.cpu0;
      history.fill_gpu0     [frame_idx] = reflex.fill.gpu0;
      history.fill_cpu1     [frame_idx] = reflex.fill.cpu1;
      history.fill_gpu1     [frame_idx] = reflex.fill.gpu1;
      history.max_stage     [frame_idx] =
                             std::max ( { reflex.simulation,
                                          reflex.render_submit,/*
                                          reflex.frame_total,   */
                                          reflex.gpu_active,
                                          reflex.gpu_start } );

      history.sample_time   [frame_idx] = qpcNow;

      auto _ProcessGraphFrameSpan =
      [&](span_s& span)
      {
        const auto end   = span.tail;
        const auto begin = span.head;

        const int count =
          end - begin + 1;

        dCPU = std::accumulate ( &history.gpu_start  [begin],
                                 &history.gpu_start  [end], dCPU );
        dGPU = std::accumulate ( &history.gpu_active [begin],
                                 &history.gpu_active [end], dGPU );

        auto* sample_age  = &history.sample_age  [begin];
        auto* sample_time = &history.sample_time [begin];
        auto *_max_stage  = &history.max_stage   [begin];

        for (INT i = 0; i < count; i++)
        {
          *sample_age =
            static_cast <float> (qpcNow - *(sample_time++)) /
            static_cast <float> (SK_QpcFreq);

          fMaxStage = std::max (*(_max_stage++), fMaxStage);
          fMaxAge   = std::max (*(sample_age++),   fMaxAge);
        }

        graph.frames += count;
      };

      span0.tail = frame_idx,              span0.head = span0.tail,
      span1.tail = _MAX_FRAME_HISTORY - 1, span1.head = span1.tail;

      // 1 Second Worth of Samples
      const auto
         oldest_sample_time = qpcNow - SK_QpcFreq;
      auto&     sample_time =
        history.sample_time;

      for ( auto frame  = span0.tail;
                 frame >= 0  &&  sample_time [frame]
                       >= oldest_sample_time; 
                          span0.head     =    frame-- );
             span1.head = span0.head;
      for ( auto frame  = span1.tail;
                 frame  > span0.tail
                              && sample_time [frame]
                       >= oldest_sample_time;
                          span1.head     =    frame-- );

      span0.head = std::max (0, span0.head);
      span1.head = std::max (0, span1.head);

      if ( span1.head >
           span0.head ) _ProcessGraphFrameSpan (span1);
                        _ProcessGraphFrameSpan (span0);

      for (int i = span0.head; i <= span0.tail; ++i)
      {    if (i < 0) continue;
                   history.sample_age [i] =
        (fMaxAge - history.sample_age [i]);
      }

      if (span1.head > span0.head)
      {
        for (int i = span1.head; i <= span1.tail; ++i)
        {
                     history.sample_age [i] =
          (fMaxAge - history.sample_age [i]);
        }
      }

      //
      // Add a repeat of the 1st element at the end, so we can wrap-around and
      //   continue graph connectivity
      //
      history.fill_cpu0     [_MAX_FRAME_HISTORY] = history.fill_cpu0     [0];
      history.fill_gpu0     [_MAX_FRAME_HISTORY] = history.fill_gpu0     [0];
      history.fill_cpu1     [_MAX_FRAME_HISTORY] = history.fill_cpu1     [0];
      history.fill_gpu1     [_MAX_FRAME_HISTORY] = history.fill_gpu1     [0];
      history.gpu_active    [_MAX_FRAME_HISTORY] = history.gpu_active    [0];
      history.gpu_start     [_MAX_FRAME_HISTORY] = history.gpu_start     [0];
      history.frame_total   [_MAX_FRAME_HISTORY] = history.frame_total   [0];
      history.max_stage     [_MAX_FRAME_HISTORY] = history.max_stage     [0];
      history.render_submit [_MAX_FRAME_HISTORY] = history.render_submit [0];
      history.sample_age    [_MAX_FRAME_HISTORY] = history.sample_age    [0];
      history.sample_time   [_MAX_FRAME_HISTORY] = history.sample_time   [0];
      history.simulation    [_MAX_FRAME_HISTORY] = history.simulation    [0];

      dCPU /= static_cast <double> (graph.frames);
      dGPU /= static_cast <double> (graph.frames);
    }

    fMaxStage =
      std::max (10000.0f * total.avg / static_cast <float> (SK_QpcFreq), fMaxStage);

    static constexpr auto              _HistorySize  = 32;
    static float        fScaleHistory [_HistorySize] = { 0.0f };
    static unsigned int iScaleIdx                    = 0;

    float fAvgScale =
      std::accumulate ( &fScaleHistory [0],
                        &fScaleHistory [_HistorySize - 1], 0.0f ) /
                   static_cast <float> (_HistorySize);

    fScaleHistory [iScaleIdx++ % _HistorySize] =
      (fMaxStage * 1.25f + fAvgScale * .75f) * 0.5f;

    fScale =
      std::max (fMaxStage, fAvgScale);

    static float         fLastScale = fScale;
       fScale = (1.75f * fLastScale + fScale * .25f) * 0.5f;
                         fLastScale = fScale;
  }

  if (predraw)
    return;

  bool detailed =
    SK_ImGui_Active () || config.nvidia.reflex.show_detailed_widget;

  //ImGui::BeginGroup ();

  const float fPlotXPos =
    ImGui::GetCursorScreenPos ().x;

  if ( ImPlot::BeginPlot ( "##Stage Time", ImVec2 (-1, 0),
                  ImPlotFlags_NoTitle     | ImPlotFlags_NoInputs |
                  ImPlotFlags_NoMouseText | ImPlotFlags_NoMenus  |
                  ImPlotFlags_NoBoxSelect )
     )
  {
    static constexpr
      auto dragline_flags =
        ImPlotDragToolFlags_NoFit    |
        ImPlotDragToolFlags_NoInputs |
        ImPlotDragToolFlags_NoCursors;

    auto flags =
      ImPlotAxisFlags_NoMenus | ImPlotAxisFlags_AutoFit;

    auto FramerateFormatter = [](double milliseconds, char* buff, int size, void*) -> int
    {
      const auto fps =
        static_cast <unsigned int> (std::max (0.0, 1000.0/milliseconds));

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
    ImPlot::SetupAxisLimits (ImAxis_X1, 0, fMaxAge,           ImPlotCond_Always);
    ImPlot::SetupAxisLimits (ImAxis_Y1, 0, fScale,            ImPlotCond_Always);
    ImPlot::SetupAxisFormat (ImAxis_Y1, MillisecondFormatter);
    ImPlot::SetupLegend     (ImPlotLocation_SouthWest,        ImPlotLegendFlags_Horizontal);

    ImPlot::SetupAxis       (ImAxis_Y2, nullptr,              ImPlotAxisFlags_AuxDefault |
                                                              ImPlotAxisFlags_NoLabel);
    ImPlot::SetupAxisLimits (ImAxis_Y2, 0, fScale,            ImPlotCond_Always);
    ImPlot::SetupAxisFormat (ImAxis_Y2, FramerateFormatter);

    auto _PlotLineData =
    [&](auto head, auto tail, bool wraparound = false)
     {
       const int elements =
        (tail - head + 1) +
         (wraparound ? 1  :  0);

       ImPlot::PlotLine ("Simulation",    &history.sample_age [head], &history.simulation    [head], elements);
       ImPlot::PlotLine ("Render Submit", &history.sample_age [head], &history.render_submit [head], elements);
       ImPlot::PlotLine ("Composite",     &history.sample_age [head], &history.frame_total   [head], elements);
       ImPlot::PlotLine ("GPU Busy",      &history.sample_age [head], &history.gpu_active    [head], elements);
       ImPlot::PlotLine ("CPU Busy",      &history.sample_age [head], &history.gpu_start     [head], elements);
     };

    auto _PlotShadedData =
    [&](auto head, auto tail, bool wraparound = false)
     {
       const int elements =
        (tail - head + 1) +
         (wraparound ? 1  :  0);
       
       ImPlot::PlotShaded   ("GPU Busy", &history.sample_age [head], &history.fill_gpu0 [head],
                                                                     &history.fill_gpu1 [head], elements, flags);
       ImPlot::PlotShaded   ("CPU Busy", &history.sample_age [head], &history.fill_cpu0 [head],
                                                                     &history.fill_cpu1 [head], elements, flags);
     };

    // Handle the circular buffer's wrap-around data first
    if ( span1.head >
         span0.head ) _PlotLineData (span1.head, span1.tail, true);
                      _PlotLineData (span0.head, span0.tail); // Contiguous

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

    ImPlot::Annotation     (0.0,     fScale, ImVec4 (.75,0,0,1), ImVec2 (0.0,0.0), true, "%hs", utf8_gpu_name.c_str ());
    ImPlot::Annotation     (fMaxAge, fScale, ImVec4 (0,0,.75,1), ImVec2 (-ImGui::CalcTextSize (SK_FormatString ("%.1f ", dFpsCpu).c_str ()).x, 0.0),
                                                                                   true, "%hs", utf8_cpu_name.c_str ());
    ImPlot::PushStyleColor (
                        ImPlotCol_InlayText, ImVec4 (1,1,1,1));
    ImPlot::Annotation     (0.0,     fScale, ImVec4 (1,1,1,1),   ImVec2 ( ImGui::CalcTextSize ( utf8_gpu_name.c_str () ).x + 
                                                                          ImGui::CalcTextSize (" ")                     .x, 0.0), true, "%.1f", dFpsGpu);
    ImPlot::Annotation     (fMaxAge, fScale, ImVec4 (1,1,1,1),   ImVec2 (0.0,0.0),                                                true, "%.1f", dFpsCpu);
    ImPlot::PopStyleColor  ();

    if      (dGPU > dCPU * 1.15)
      ImPlot::TagY (dGPU, ImVec4 (1,0,0,1), "GPU Bound");
    else if (dCPU > dGPU * 1.15)
      ImPlot::TagY (dCPU, ImVec4 (0,0,1,1), "CPU Bound");
    else
      ImPlot::TagY ((dCPU + dGPU) * 0.5, ImVec4 (0,1,0,1),"\tBalanced");

    ImPlot::PushStyleVar (ImPlotStyleVar_FillAlpha, 0.15f);

    // Handle the circular buffer's wrap-around data first
    if ( span1.head >
         span0.head ) _PlotShadedData (span1.head, span1.tail, true);
                      _PlotShadedData (span0.head, span0.tail); // Contiguous

    ImPlot::PopStyleVar ();
    ImPlot::EndPlot     ();

    if (detailed)
    {
      ImGui::SameLine ();
      ImGui::SetCursorScreenPos (
        ImVec2 (fPlotXPos, ImGui::GetCursorScreenPos ().y)
      );
    }
  }

  if (detailed)
  {
    if (ImGui::Checkbox ("###ReflexShowDetailedWidget",
           &config.nvidia.reflex.show_detailed_widget))
    {
      config.utility.save_async ();
    }
  
    if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("Show render pipeline timing diagram in widget");
  }

  ImGui::BeginGroup ();

  if (detailed)
  {
    for ( auto* pStage : stages )
    {
      if (pStage->avg < min_stage->avg) min_stage = pStage;
      if (pStage->avg > max_stage->avg) max_stage = pStage;
    }

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
  }
  ImGui::TextColored (
    ImColor (1.f, 1.f, 1.f), "Total Frame Time"
  );
  if (input.avg > 0.0f)
  {
    ImGui::TextColored (
      ImColor (1.f, 1.f, 1.f), "Input Age"
    );
  }
  ImGui::EndGroup    ();
  ImGui::SameLine    ();
  ImGui::SeparatorEx (ImGuiSeparatorFlags_Vertical);
  ImGui::SameLine    (0.0f, 10.0f);
  ImGui::BeginGroup  ();
  if (detailed)
  {
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
            pStage->avg / 1000.0f
      );
    }

    ImGui::Separator ();
  }

  const float frametime =
    ( total.avg / 1000.0f );

  fLegendY =
    ImGui::GetCursorScreenPos ().y;

  ImGui::TextColored (
    ImColor (1.f, 1.f, 1.f),
      "%4.2f ms",
        frametime    );

  static bool
      input_sampled = false;
  if (input_sampled || input.avg > 0.0f)
  {   input_sampled = true;
    ImGui::TextColored (
      ImColor (1.f, 1.f, 1.f),
        input.avg != 0.0f ?
               "%4.2f ms" : "--",
             input.avg / 1000.0f
                       );

    fLegendY +=
      ImGui::GetTextLineHeightWithSpacing () * 0.5f;
  }
  ImGui::EndGroup    ();

  ImGui::SameLine    ();
  ImGui::SeparatorEx (ImGuiSeparatorFlags_Vertical);

  ImGui::SameLine   (0, 10.0f);
  ImGui::BeginGroup ();

  if (detailed)
  {
    extern void
    SK_NV_AdaptiveSyncControl ();
    SK_NV_AdaptiveSyncControl ();

    float fMaxWidth  = ImGui::GetContentRegionAvail ().x;
    float fMaxHeight = ImGui::GetTextLineHeightWithSpacing () * 2.9f;
    float fInset     = fMaxWidth  *  0.025f;
    float X0         = ImGui::GetCursorScreenPos ().x;
    float Y0         = ImGui::GetCursorScreenPos ().y - ImGui::GetTextLineHeightWithSpacing ();
          fMaxWidth  *= 0.95f;
          fMaxHeight *= 0.92f;

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
        static_cast <float> ( SK_Reflex_GetFrameEndTime (frame_report) -
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
  }

  ImGui::SetCursorScreenPos (
    ImVec2 (ImGui::GetCursorScreenPos ().x, fLegendY)
  );

  ImGui::BeginGroup ();
  for ( auto *pStage : stages )
  {
    ImGui::TextColored (pStage->color, "%s", pStage->label);

    if (ImGui::IsItemHovered ())
    {
      if (pStage->desc != nullptr) {
        ImGui::SetTooltip ("%s", pStage->desc);
      }
    }

    ImGui::SameLine (0.0f, 12.0f);

    ImGui::SetCursorScreenPos (
      ImVec2 (ImGui::GetCursorScreenPos ().x, fLegendY)
    );
  }
  ImGui::EndGroup   ();

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

  if (config.nvidia.reflex.native && (! config.nvidia.reflex.disable_native))
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

  if ( config.nvidia.reflex.enable      && bFullReflexSupport &&
       config.nvidia.reflex.low_latency && ((! config.nvidia.reflex.native) || config.nvidia.reflex.disable_native) )
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
       config.nvidia.reflex.low_latency_boost &&
           ( (! config.nvidia.reflex.native)  ||
                config.nvidia.reflex.override ||
                config.nvidia.reflex.disable_native )
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

    SK_ImGui_DrawGraph_Latency (false);
  }


  void config_base () noexcept override
  {
    SK_Widget::config_base ();

    ImGui::Separator (  );
    ImGui::TreePush  ("###LatencyWidgetConfig");

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