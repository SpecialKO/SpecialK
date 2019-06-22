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
#include <SpecialK/widgets/widget.h>

extern iSK_INI* osd_ini;

extern void SK_ImGui_DrawGraph_FramePacing (void);

namespace
SK_ImGui
{
  bool BatteryMeter (void);
};

static bool has_battery = false;

struct
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

    SK_RunOnce (cp->AddVariable ("FramePacing.DisplayPercentiles",    SK_CreateVar (SK_IVariable::Boolean, &display)));
    SK_RunOnce (cp->AddVariable ("FramePacing.PercentilesAboveGraph", SK_CreateVar (SK_IVariable::Boolean, &display_above)));
    SK_RunOnce (cp->AddVariable ("FramePacing.ShortTermPercentiles",  SK_CreateVar (SK_IVariable::Boolean, &display_most_recent)));
    SK_RunOnce (cp->AddVariable ("FramePacing.Percentile[0].Cutoff",  SK_CreateVar (SK_IVariable::Float,   &percentile0.cutoff)));
    SK_RunOnce (cp->AddVariable ("FramePacing.Percentile[1].Cutoff",  SK_CreateVar (SK_IVariable::Boolean, &percentile1.cutoff)));
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
    float cutoff             =  0.0f;
    bool  has_data           = false;
    float computed_fps       =  0.0f;
    ULONG last_calculated    =   0UL;

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
} SK_FramePercentiles;


float SK_Framerate_GetPercentileByIdx (int idx)
{
  if (idx == 0)
    return SK_FramePercentiles.percentile0.cutoff;

  if (idx == 1)
    return SK_FramePercentiles.percentile1.cutoff;

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

void
SK_ImGui_DrawGraph_FramePacing (void)
{
  static const auto& io =
    ImGui::GetIO ();

  const  float font_size           =
    ( ImGui::GetFont  ()->FontSize * io.FontGlobalScale );

  const  float font_size_multiline =
    ( ImGui::GetStyle ().ItemSpacing.y      +
      ImGui::GetStyle ().ItemInnerSpacing.y + font_size );


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
    ( game_window.active || __target_fps_bg == 0.0f ) ?
            __target_fps  : __target_fps_bg;

  float target_frametime = ( target == 0.0f ) ?
                           ( 1000.0f   / (ffx ? 30.0f : 60.0f) ) :
                             ( 1000.0f / fabs (target) );

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

  extern void SK_ImGui_DrawFramePercentiles (void);

  if (SK_FramePercentiles.display_above)
      SK_ImGui_DrawFramePercentiles ();

  snprintf
        ( szAvg,
            511,
              u8"Avg milliseconds per-frame: %6.3f  (Target: %6.3f)\n"
              u8"    Extreme frame times:     %6.3f min, %6.3f max\n\n\n\n"
              u8"Variation:  %8.5f ms        %.1f FPS  ±  %3.1f frames",
                sum / frames,
                  target_frametime,
                    min, max, (double)max - (double)min,
                      1000.0f / (sum / frames),
                        ((double)max-(double)min)/(1000.0f/(sum/frames)) );

  ImGui::PushStyleColor ( ImGuiCol_PlotLines,
                            (ImVec4&&)ImColor::HSV ( 0.31f - 0.31f *
                     std::min ( 1.0f, (max - min) / (2.0f * target_frametime) ),
                                             0.86f,
                                               0.95f ) );

  const ImVec2 border_dims (
    std::max (500.0f, ImGui::GetContentRegionAvailWidth ()),
      font_size * 7.0f
  );

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
  if ((! SK_FramePercentiles.display) && ImGui::IsItemClicked ())
  {
    SK_FramePercentiles.toggleDisplay ();
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


  if (! SK_FramePercentiles.display_above)
        SK_ImGui_DrawFramePercentiles ();
}

void
SK_ImGui_DrawFramePercentiles (void)
{
  if (! SK_FramePercentiles.display)
    return;

  static auto& snapshots =
    SK::Framerate::frame_history_snapshots;

  auto& percentile0 = SK_FramePercentiles.percentile0;
  auto& percentile1 = SK_FramePercentiles.percentile1;
  auto&        mean = SK_FramePercentiles.mean;

  bool& show_immediate =
    SK_FramePercentiles.display_most_recent;

  static constexpr LARGE_INTEGER         all_samples = { 0ULL };
  extern           SK::Framerate::Stats *frame_history;

  long double data_timespan = ( show_immediate ?
            frame_history->calcDataTimespan () :
          snapshots->mean->calcDataTimespan () );

  ImGui::PushStyleColor (ImGuiCol_Text,           (unsigned int)ImColor (255, 255, 255));
  ImGui::PushStyleColor (ImGuiCol_FrameBg,        (unsigned int)ImColor ( 0.3f,  0.3f,  0.3f, 0.7f));
  ImGui::PushStyleColor (ImGuiCol_FrameBgHovered, (unsigned int)ImColor ( 0.6f,  0.6f,  0.6f, 0.8f));
  ImGui::PushStyleColor (ImGuiCol_FrameBgActive,  (unsigned int)ImColor ( 0.9f,  0.9f,  0.9f, 0.9f));

  if ( SK_FramePercentiles.percentile0.last_calculated !=
       SK_GetFramesDrawn ()
     )
  {
   //const long double   SAMPLE_SECONDS = 5.0L;

    percentile0.computeFPS (                             show_immediate ?
      frame_history->calcPercentile   (percentile0.cutoff, all_samples) :
      snapshots->percentile0->calcMean                   (all_samples) );

    percentile1.computeFPS (                             show_immediate ?
      frame_history->calcPercentile   (percentile1.cutoff, all_samples) :
      snapshots->percentile1->calcMean                   (all_samples) );

    mean.computeFPS ( show_immediate          ?
        frame_history->calcMean (all_samples) :
      snapshots->mean->calcMean (all_samples) );
  }

  float p0_ratio =
        percentile0.computed_fps / mean.computed_fps;
  float p1_ratio =
        percentile1.computed_fps / mean.computed_fps;

  if ( percentile0.computed_fps > std::numeric_limits <float>::epsilon () ||
        (! show_immediate)
     )
  {
    if (! (percentile0.computed_fps > std::numeric_limits <float>::epsilon ()))
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

    percentile0.has_data  = true;

    const float luminance = 0.5f;

    ImGui::BeginGroup ();

    if (data_timespan > 0)
    {
    //ImGui::Text ("Sample Size");

      ImGui::PushStyleColor  (ImGuiCol_Text, ImVec4 (1.0f, 1.0f, 1.0f, 1.0f));
           if (data_timespan > 60.0 * 60.0) ImGui::Text ("%7.3f", data_timespan / (60.0 * 60.0));
      else if (data_timespan > 60.0)        ImGui::Text ("%6.2f", data_timespan / 60.0);
      else                                  ImGui::Text ("%5.1f", data_timespan);

      ImGui::SameLine ();

      ImGui::PushStyleColor  (ImGuiCol_Text, ImVec4 (0.595f, 0.595f, 0.595f, 1.0f));

           if (data_timespan > 60.0 * 60.0) ImGui::Text ("Hours");
      else if (data_timespan > 60.0)        ImGui::Text ("Minutes");
      else                                  ImGui::Text ("Seconds");

      ImGui::PopStyleColor (2);
    }

    if (percentile1.has_data && data_timespan > 0)
    {
      ImGui::Spacing          ();
      ImGui::Spacing          ();
      ImGui::PushStyleColor  (ImGuiCol_Text, ImVec4 (0.595f, 0.595f, 0.595f, 1.0f));
      ImGui::TextUnformatted ("AvgFPS: "); ImGui::SameLine ();
      ImGui::PushStyleColor  (ImGuiCol_Text, ImVec4 (1.0f, 1.0f, 1.0f, 1.0f));
      ImGui::Text            ("%4.1f", mean.computed_fps);
      ImGui::PopStyleColor   (2);
    }

    ImGui::EndGroup   ();

    if (ImGui::IsItemHovered ( ))
    {
      ImGui::BeginTooltip    ( );
      ImGui::BeginGroup      ( );
      ImGui::PushStyleColor  (ImGuiCol_Text, ImVec4 (0.685f, 0.685f, 0.685f, 1.0f));
      ImGui::BulletText      ("Left-Click");
      ImGui::BulletText      ("Ctrl+Click");
      ImGui::PopStyleColor   ( );
      ImGui::EndGroup        ( );
      ImGui::SameLine        ( ); ImGui::Spacing ();
      ImGui::SameLine        ( ); ImGui::Spacing ();
      ImGui::SameLine        ( );
      ImGui::BeginGroup      ( );
      ImGui::PushStyleColor  (ImGuiCol_Text, ImVec4 (1.0f, 1.0f, 1.0f, 1.0f));
      ImGui::TextUnformatted ("Change Datasets (Long / Short)");
      ImGui::TextUnformatted ("Reset Long-term Statistics");
      ImGui::PopStyleColor   ( );
      ImGui::EndGroup        ( );
      ImGui::EndTooltip      ( );
    }

    static auto& io =
      ImGui::GetIO ();

    if (ImGui::IsItemClicked ())
    {
      if (! io.KeyCtrl)
      {
        show_immediate = (! show_immediate);
        SK_FramePercentiles.store_percentile_cfg ();
      }

      else snapshots->reset ();
    }

    ImGui::SameLine   ();
    ImGui::BeginGroup ();
    ImGui::PushStyleColor
                      ( ImGuiCol_PlotHistogram,  (unsigned int)ImColor::HSV (p0_ratio * 0.278f, 0.88f, luminance));
    ImGui::ProgressBar( p0_ratio, ImVec2 (-1.0f, 0.0f),
      SK_FormatString ( "%3.1f%% Low FPS: %5.2f",
                          percentile0.cutoff,
                          percentile0.computed_fps ).c_str ()
    );

    if ( percentile1.computed_fps > std::numeric_limits <float>::epsilon () &&
                                            data_timespan > 0 )
    {
      percentile1.has_data = true;

      ImGui::PushStyleColor ( ImGuiCol_PlotHistogram,  (unsigned int)ImColor::HSV (p1_ratio * 0.278f, 0.88f, luminance));
      ImGui::ProgressBar    ( p1_ratio,
                                 ImVec2 (-1.0f, 0.0f), SK_FormatString
                             ( "%3.1f%% Low FPS: %5.2f",
                                 percentile1.cutoff,
                                 percentile1.computed_fps
                             ).c_str ()
      );
      ImGui::PopStyleColor (2);
    }

    else
    {
      percentile1.has_data = false;
      ImGui::PopStyleColor (1);
    }

    ImGui::EndGroup        ( );

    if ((SK_FramePercentiles.display) && ImGui::IsItemClicked ())
    {
      SK_FramePercentiles.toggleDisplay ();
    }
  }

  else
  { percentile0.has_data = false;
    percentile1.has_data = false;
  }

  ImGui::PopStyleColor     (4);
}

class SKWG_FramePacing : public SK_Widget
{
public:
  SKWG_FramePacing (void) noexcept : SK_Widget ("FramePacing")
  {
    SK_ImGui_Widgets->frame_pacing = this;

    setResizable    (                false).setAutoFit      (true).setMovable (false).
    setDockingPoint (DockAnchor::SouthEast).setClickThrough (true).setVisible (false);

    SK_FramePercentiles.load_percentile_cfg ();
  };

  void load (iSK_INI* cfg) noexcept override
  {
    SK_Widget::load (cfg);

    SK_FramePercentiles.load_percentile_cfg ();
  }

  void save (iSK_INI* cfg) noexcept override
  {
    if (cfg == nullptr)
      return;

    SK_Widget::save (cfg);

    SK_FramePercentiles.store_percentile_cfg ();

    cfg->write (
      cfg->get_filename ()
    );
  }

  void run (void) noexcept override
  {
    static auto* cp =
      SK_GetCommandProcessor ();

    static volatile LONG init = 0;

    if (! InterlockedCompareExchange (&init, 1, 0))
    {
      //auto *display_framerate_percentiles_above =
      //  SK_CreateVar (
      //    SK_IVariable::Boolean,
      //      &SK_FramePercentiles.display_above,
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
            &SK_FramePercentiles.display,
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

    float extra_line_space = 0.0f;

    auto& percentile0 = SK_FramePercentiles.percentile0;
    auto& percentile1 = SK_FramePercentiles.percentile1;

    if (has_battery)            extra_line_space += 1.16f;
    if (SK_FramePercentiles.display)
    {
      if (percentile0.has_data) extra_line_space += 1.16f;
      if (percentile1.has_data) extra_line_space += 1.16f;
    }

    // If configuring ...
    if (state__ != 0) extra_line_space += (1.16f * 5.5f);

    ImVec2   new_size (font_size * 35, font_size_multiline * (5.44f + extra_line_space));
    setSize (new_size);

    if (isVisible ())// && state__ == 0)
    {
      ImGui::SetNextWindowSize (new_size, ImGuiCond_Always);
    }
  }

  void draw (void) noexcept override
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

    SK_ImGui_DrawGraph_FramePacing ();

    has_battery =
      SK_ImGui::BatteryMeter ();
  }


  void config_base (void) noexcept override
  {
    SK_Widget::config_base ();

    static auto& snapshots =
      SK::Framerate::frame_history_snapshots;

    ImGui::Separator ();

    bool changed = false;
    bool display = SK_FramePercentiles.display;

    changed |= ImGui::Checkbox  ("Show Percentile Analysis", &display);

    if (changed)
    {
      SK_FramePercentiles.toggleDisplay ();
    }

    if (SK_FramePercentiles.display)
    {
      changed |= ImGui::Checkbox ("Draw Percentiles Above Graph",         &SK_FramePercentiles.display_above);
      changed |= ImGui::Checkbox ("Use Short-Term (~15-30 seconds) Data", &SK_FramePercentiles.display_most_recent);

      ImGui::Separator ();
      ImGui::TreePush  ();

      if ( ImGui::SliderFloat (
             "Percentile Class 0 Cutoff",
               &SK_FramePercentiles.percentile0.cutoff,
                 0.1f, 99.99f, "%3.1f%%" )
         )
      {
        snapshots->reset (); changed = true;
      }

      if ( ImGui::SliderFloat (
             "Percentile Class 1 Cutoff",
               &SK_FramePercentiles.percentile1.cutoff,
                 0.1f, 99.99f, "%3.1f%%" )
         )
      {
        snapshots->reset (); changed = true;
      }

      ImGui::TreePop ();
    }

    if (changed)
      SK_FramePercentiles.store_percentile_cfg ();
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