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

constexpr
float
operator"" _GHz ( long double Hz )
{
  return
    static_cast <float> ( Hz * 1000000000.0 );
}

constexpr
float
 operator"" _MiB ( long double bytes )
{
  return
    static_cast <float> ( bytes * 1048576.0 );
}

struct sk_sensor_prefs_s
{
  bool                             enable;
  std::string                      name;
  std::wstring                     ini_name;
  SK_Stat_DataHistory <float, 96>* pHistory;

  // Min/Max user-expected values (i.e. safe / sane range)
  float                            min_val;
  float                            max_val;

  struct
  {
    sk::ParameterBool*  enable;
    sk::ParameterFloat* min;
    sk::ParameterFloat* max;
  } params;
};

class SKWG_GPU_Monitor : public SK_Widget
{
public:
  SKWG_GPU_Monitor (void) noexcept : SK_Widget ("GPUMonitor")
  {
    SK_ImGui_Widgets->gpu_monitor = this;

    setAutoFit (true).setDockingPoint (DockAnchor::West).setBorder (true).
                      setClickThrough (false);
  };

  void load (iSK_INI* cfg) noexcept override
  {
    SK_Widget::load (cfg);
  }

  void save (iSK_INI* cfg) noexcept override
  {
    if (cfg == nullptr)
      return;

    for ( auto& it : prefs)
    {
      if (it->params.enable != nullptr)
      {
        it->params.enable->store (it->enable);
        it->params.min->store    (it->min_val);
        it->params.max->store    (it->max_val);
      }
    }

    SK_Widget::save (cfg);

    cfg->write ();
  }

  void run (void) noexcept override
  {
    static bool first_run = true;

    if (first_run)
    {
      first_run = false;

      for ( auto& it : prefs)
      {
        it->params.enable =
          dynamic_cast <sk::ParameterBool *> (
            SK_Widget_ParameterFactory->create_parameter <bool> (L"Enable Widget")
          );

        it->params.min =
          dynamic_cast <sk::ParameterFloat *> (
            SK_Widget_ParameterFactory->create_parameter <float> (L"Minimum Value")
          );

        it->params.max =
          dynamic_cast <sk::ParameterFloat *> (
            SK_Widget_ParameterFactory->create_parameter <float> (L"Maximum Value")
          );

        it->params.enable->register_to_ini ( osd_ini,
          SK_FormatStringW ( L"GPU0.%s", it->ini_name.c_str () ),
                             L"EnableSensor" );

        it->params.max->register_to_ini ( osd_ini,
          SK_FormatStringW ( L"GPU0.%s", it->ini_name.c_str () ),
                             L"MaxValue" );

        it->params.min->register_to_ini ( osd_ini,
          SK_FormatStringW ( L"GPU0.%s", it->ini_name.c_str () ),
                             L"MinValue" );
      }

      std::vector <std::tuple <sk::iParameter *, void *, std::type_index>> params;

      for (auto& it : prefs)
      {
        params.push_back ( { it->params.enable,
                            &it->enable,
          std::type_index (
                     typeid (it->params.enable)
                          )
                           }
        );

        params.push_back ( { it->params.max,
                            &it->max_val,
          std::type_index (
                     typeid (it->params.max)
                          )
                           }
        );

        params.push_back ( { it->params.min,
                            &it->min_val,
          std::type_index (
                     typeid (it->params.min)
                          )
                           }
        );
      }

      for (auto& it : params)
      {
        const std::type_index tidx ( std::get <2> (it) );

        if (tidx == std::type_index (typeid (sk::ParameterBool *)))
        {
          sk::ParameterBool *param = (sk::ParameterBool *)std::get <0> (it);
          bool              *pData =              (bool *)std::get <1> (it);

          if (! param->load (*pData))
          {
            param->store (*pData);
          }

          *pData = param->get_value ();
        }

        else if (tidx == std::type_index (typeid (sk::ParameterFloat *)))
        {
          sk::ParameterFloat *param = (sk::ParameterFloat *)std::get <0> (it);
          float              *pData =              (float *)std::get <1> (it);

          if (! param->load (*pData))
          {
            param->store (*pData);
          }

          *pData = param->get_value ();
        }
      }
    }

    if (last_update < SK::ControlPanel::current_time - update_freq)
    {
      SK_PollGPU ();

      core_clock_ghz.addValue ( SK_GPU_GetClockRateInkHz    (0) / 0.001_GHz);
      vram_clock_ghz.addValue ( SK_GPU_GetMemClockRateInkHz (0) / 0.001_GHz);
      gpu_load.addValue       ( SK_GPU_GetGPULoad           (0)    );
      gpu_temp_c.addValue     ( SK_GPU_GetTempInC           (0)    );
      vram_used_mib.addValue  ( SK_GPU_GetVRAMUsed          (0) / 1.0_MiB);
    //vram_shared.addValue    (        SK_GPU_GetVRAMShared (0));
      fan_rpm.addValue        ( (float)SK_GPU_GetFanSpeedRPM(0));

      last_update = SK::ControlPanel::current_time;
    }
  }

  void config_base (void) noexcept override
  {
    SK_Widget::config_base ();

    ImGui::Separator ();

    auto _ConfigSensor = [&](sk_sensor_prefs_s* sensor) -> void
    {
      ImGui::PushID      (sensor);
      ImGui::TreePush    ("");
      ImGui::BeginGroup  (  );
      ImGui::Checkbox    ("Enable###EnableWidgetSensor", &sensor->enable);
    //ImGui::Text        ("Graph Mode");
    //ImGui::Text        ("Unit of Measure");
      ImGui::Text        ("Maximum Normal Value");
      ImGui::Text        ("Minimum Normal Value");
      ImGui::EndGroup    (   );
      ImGui::SameLine    (   );
      ImGui::BeginGroup  (   );
      ImGui::Text        (" ");
    //ImGui::Text        (" ");
    //ImGui::Text        ("-");

      bool bAuto = (sensor->min_val < 0.0f);

      if ( ImGui::Checkbox ("Auto###Auto_MinVal", &bAuto) )
      {
        if  (sensor->min_val == 0.0f) sensor->min_val = -1.0f;
        else       sensor->min_val = -sensor->min_val;
      }

      if (sensor->min_val >= 0.0f)
      {
        ImGui::SameLine    ();
        ImGui::SliderFloat ("Min", &sensor->min_val, 0.0f, 100.0f);
      }

      bAuto = (sensor->max_val < 0.0f);

      if ( ImGui::Checkbox ("Auto###Auto_MaxVal", &bAuto) )
      {
        if  (sensor->max_val == 0.0f) sensor->max_val = -1.0f;
        else       sensor->max_val = -sensor->max_val;
      }

      if (sensor->max_val >= 0.0f)
      {
        ImGui::SameLine    ();
        ImGui::SliderFloat ("Max", &sensor->max_val, 0.0f, 100.0f);
      }

      ImGui::EndGroup    (   );
      ImGui::TreePop     (   );
      ImGui::PopID       (   );
    };

    for (auto& it : prefs)
    {
      if (ImGui::CollapsingHeader (it->name.c_str ()))
      {
        _ConfigSensor (it);
      }
    }
  }

  void draw (void) noexcept override
  {
    if (ImGui::GetFont () == nullptr) return;

    const  float font_size = ImGui::GetFont ()->FontSize;//* scale;


    auto _MinVal = [&]( float              auto_val,
                        sk_sensor_prefs_s* sensor ) ->
    float
    {
      if (sensor->min_val > 0.0f) return sensor->min_val;

      return auto_val;
    };

    auto _MaxVal = [&]( float              auto_val,
                        sk_sensor_prefs_s* sensor ) ->
    float
    {
      if (sensor->max_val > 0.0f) return sensor->max_val;

      return auto_val;
    };


    char szAvg  [512] = { };

    if (gpu_load_prefs.enable)
    {
      snprintf
        ( szAvg,
            511,
              "GPU%lu Load %%:\n\n\n"
              "          min: %3.0f%%, max: %3.0f%%, avg: %4.1f%%\n",
                0,
                  gpu_load.getMin (), gpu_load.getMax (),
                    gpu_load.getAvg () );

      float samples =
        std::min ( (float)gpu_load.getUpdates  (),
                   (float)gpu_load.getCapacity () );

      ImGui::PlotLinesC ( "###GPU_LoadPercent",
                           gpu_load.getValues     ().data (),
          static_cast <int> (samples),
                               gpu_load.getOffset (),
                                 szAvg,
                                   gpu_load.getMin   () * 0.95f,
                                     gpu_load.getMax () * 1.05f,
                                       ImVec2 (
                                         ImGui::GetContentRegionAvailWidth (), font_size * 4.5f),
                                           4, _MinVal (0.0f,   &gpu_load_prefs),
                                              _MaxVal (100.0f, &gpu_load_prefs) );
    }

    if (gpu_temp_prefs.enable)
    {
      snprintf
        ( szAvg,
            511,
              (const char *)u8"GPU%lu Temp (°C):\n\n\n"
                            u8"          min: %3.0f°, max: %3.0f°, avg: %4.1f°\n",
                0,
                  gpu_temp_c.getMin   (), gpu_temp_c.getMax (),
                    gpu_temp_c.getAvg () );

      float samples =
        std::min ( (float)gpu_temp_c.getUpdates  (),
                   (float)gpu_temp_c.getCapacity () );

      ImGui::PlotLinesC ( "###GPU_TempC",
                           gpu_temp_c.getValues     ().data (),
          static_cast <int> (samples),
                               gpu_temp_c.getOffset (),
                                 szAvg,
                                   gpu_temp_c.getMin   () * 0.95f,
                                     gpu_temp_c.getMax () * 1.05f,
                                       ImVec2 (
                                         ImGui::GetContentRegionAvailWidth (), font_size * 4.5f),
                                           4, _MinVal (50.0f, &gpu_temp_prefs),
                                              _MaxVal (94.0f, &gpu_temp_prefs) );
    }


    static float min_rpm =  std::numeric_limits <float>::infinity ();
    static float max_rpm = -std::numeric_limits <float>::infinity ();

    if (fan_rpm.getMin () > 0.0f && gpu_fan_prefs.enable)
    {
      snprintf
        ( szAvg,
            511,
              "GPU%lu Fan Speed (RPM):\n\n\n"
              "          min: %3.0f, max: %3.0f, avg: %4.1f\n",
                0,
                  fan_rpm.getMin   (), fan_rpm.getMax (),
                    fan_rpm.getAvg () );

      float samples =
        std::min ( (float)fan_rpm.getUpdates  (),
                   (float)fan_rpm.getCapacity () );

      min_rpm =
        std::min ( min_rpm, fan_rpm.getMin () > 0 ? fan_rpm.getMin () : min_rpm );
      max_rpm =
        std::max ( max_rpm, fan_rpm.getMax () > 0 ? fan_rpm.getMax () : max_rpm );

      ImGui::PlotLinesC ( "###GPU_FanSpeed_Hz",
                           fan_rpm.getValues     ().data (),
          static_cast <int> (samples),
                               fan_rpm.getOffset (),
                                 szAvg,
                                   fan_rpm.getMin () * 0.95f,
                                     max_rpm         * 1.05f,
                                       ImVec2 (
                                         ImGui::GetContentRegionAvailWidth (), font_size * 4.5f),
                                         4, _MinVal (min_rpm, &gpu_fan_prefs),
                                            _MaxVal (max_rpm, &gpu_fan_prefs), 0.0f, true );
    }

    if (core_clock_prefs.enable)
    {
      snprintf
        ( szAvg,
            511,
              "GPU%lu Core Clock (GHz):\n\n\n"
              "          min: %4.2f, max: %4.2f, avg: %5.3f\n",
                0,
                  core_clock_ghz.getMin   (), core_clock_ghz.getMax (),
                    core_clock_ghz.getAvg () );

      float samples =
        std::min ( (float)core_clock_ghz.getUpdates  (),
                   (float)core_clock_ghz.getCapacity () );

      static float max_clock = -std::numeric_limits <float>::infinity ();
      static float min_clock =  std::numeric_limits <float>::infinity ();

      max_clock = std::max (max_clock, core_clock_ghz.getMax ());
      min_clock = std::min (min_clock, core_clock_ghz.getMin () > 0 ?
                                       core_clock_ghz.getMin ()     : min_clock);

      ImGui::PlotLinesC ( "###GPU_CoreClock",
                           core_clock_ghz.getValues ().data (),
          static_cast <int> (samples),
                               core_clock_ghz.getOffset (),
                                 szAvg,
                                   core_clock_ghz.getMin   () / 1.05f,
                                     core_clock_ghz.getMax () * 1.05f,
                                       ImVec2 (
                                         ImGui::GetContentRegionAvailWidth (), font_size * 4.5f),
                                           4, _MinVal (min_clock, &core_clock_prefs),
                                              _MaxVal (max_clock, &core_clock_prefs) );
    }

    if (vram_clock_prefs.enable)
    {
      snprintf
        ( szAvg,
            511,
              "GPU%lu VRAM Clock (GHz):\n\n\n"
              "          min: %4.2f, max: %4.2f, avg: %5.3f\n",
                0,
                  vram_clock_ghz.getMin   (), vram_clock_ghz.getMax (),
                    vram_clock_ghz.getAvg () );

      float samples =
        std::min ( (float)vram_clock_ghz.getUpdates  (),
                   (float)vram_clock_ghz.getCapacity () );

      static float max_vram_clock = -std::numeric_limits <float>::infinity ();
      static float min_vram_clock =  std::numeric_limits <float>::infinity ();

      max_vram_clock = std::max (max_vram_clock, vram_clock_ghz.getMax ());
      min_vram_clock = std::min (min_vram_clock, vram_clock_ghz.getMin () > 0 ?
                                                 vram_clock_ghz.getMin ()     : min_vram_clock);

      ImGui::PlotLinesC ( "###GPU_VRAMClock",
                           vram_clock_ghz.getValues ().data (),
          static_cast <int> (samples),
                               vram_clock_ghz.getOffset (),
                                 szAvg,
                                   vram_clock_ghz.getMin   () * 0.95f,
                                     vram_clock_ghz.getMax () * 1.05f,
                                       ImVec2 (
                                         ImGui::GetContentRegionAvailWidth (), font_size * 4.5f),
                                           4, _MinVal (min_vram_clock, &vram_clock_prefs),
                                              _MaxVal (max_vram_clock, &vram_clock_prefs) );
    }

    // TODO: Add a parameter to data history to control this
    static float max_use = 0.0f;

    max_use = std::max (vram_used_mib.getMax (), max_use);

    if (vram_used_prefs.enable)
    {
      snprintf
        ( szAvg,
            511,
              "GPU%lu VRAM Usage (MiB):\n\n\n"
              "          min: %6.1f, max: %6.1f, avg: %6.1f\n",
                0,
                  vram_used_mib.getMin   (), max_use,
                    vram_used_mib.getAvg () );

      float samples =
        std::min ( (float)vram_used_mib.getUpdates  (),
                   (float)vram_used_mib.getCapacity () );

      auto capacity_in_mib =
        static_cast <float> (SK_GPU_GetVRAMBudget (0) >> 20ULL);

      if (capacity_in_mib <= 0.0f)
        capacity_in_mib = 4096.0f; // Just take a wild guess, lol

      ImGui::PlotLinesC ( "###GPU_VRAMUsage",
                           vram_used_mib.getValues ().data (),
          static_cast <int> (samples),
                               vram_used_mib.getOffset (),
                                 szAvg,
                                   0.0f,//vram_used_mib.getMin () / 1.1f,
                                     capacity_in_mib       * 1.05f,
                                       ImVec2 (
                                         ImGui::GetContentRegionAvailWidth (), font_size * 4.5f),
                                           4, _MinVal (0.0f,            &vram_used_prefs),
                                              _MaxVal (capacity_in_mib, &vram_used_prefs) );
    }
  }

  void OnConfig (ConfigEvent event) override
  {
    switch (event)
    {
      case SK_Widget::ConfigEvent::LoadComplete:
        break;

      case SK_Widget::ConfigEvent::SaveStart:
        break;
    }
  }

private:
  DWORD last_update = 0UL;

  SK_Stat_DataHistory <float,    96> core_clock_ghz = { };
  SK_Stat_DataHistory <float,    96> vram_clock_ghz = { };
  SK_Stat_DataHistory <float,    96> gpu_load       = { };
  SK_Stat_DataHistory <float,    96> gpu_temp_c     = { };
  SK_Stat_DataHistory <float,    96> vram_used_mib  = { };
  SK_Stat_DataHistory <uint64_t, 96> vram_shared    = { };
  SK_Stat_DataHistory <float,    96> fan_rpm        = { };

  sk_sensor_prefs_s core_clock_prefs { false, "GPU Core Clock (GHz)",
                                               L"CoreClock",
                                       &core_clock_ghz, -1.0f, -1.0f };
  sk_sensor_prefs_s vram_clock_prefs { false, "GPU VRAM Clock (GHz)",
                                                L"VRAMClock",
                                       &vram_clock_ghz, -1.0f, -1.0f };
  sk_sensor_prefs_s gpu_load_prefs   { true, "GPU Load (%)",
                                              L"Load",
                                       &gpu_load,       0.0f, 100.0f };
  sk_sensor_prefs_s gpu_temp_prefs   { true, (const char *)u8"GPU Temperature (°C)",
                                              L"Temperature",
                                       &gpu_temp_c,    40.0f, 94.0f  };
  sk_sensor_prefs_s gpu_fan_prefs    { true, "GPU Fan Speed (RPM)",
                                              L"FanSpeed",
                                       &fan_rpm,        0.0f, 0.0f   };
  sk_sensor_prefs_s vram_used_prefs  { true, "GPU VRAM in Use (MiB)",
                                              L"VRAMInUse",
                                       &vram_used_mib, -1.0f, -1.0f  };

protected:
  const DWORD update_freq = 666UL;

  std::vector <sk_sensor_prefs_s *> prefs =
    { &gpu_load_prefs,   &gpu_temp_prefs,
      &gpu_fan_prefs,
      &core_clock_prefs, &vram_clock_prefs,
      &vram_used_prefs };

};

SK_LazyGlobal <SKWG_GPU_Monitor> __gpu_monitor__;

void SK_Widget_InitGPUMonitor (void)
{
  SK_RunOnce (__gpu_monitor__.getPtr ());
}

