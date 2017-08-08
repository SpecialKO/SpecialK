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
#include <SpecialK/widgets/widget.h>
#include <SpecialK/gpu_monitor.h>

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

class SKWG_GPU_Monitor : public SK_Widget
{
public:
  SKWG_GPU_Monitor (void) : SK_Widget ("GPUMonitor")
  {
    SK_ImGui_Widgets.gpu_monitor = this;

    setAutoFit (true).setDockingPoint (DockAnchor::West).setClickThrough (true);
  };

  void run (void)
  {
    DWORD dwNow = timeGetTime ();

    if (last_update < dwNow - update_freq)
    {
      SK_PollGPU ();

      core_clock_ghz.addValue ( SK_GPU_GetClockRateInkHz    (0) / 0.001_GHz);
      vram_clock_ghz.addValue ( SK_GPU_GetMemClockRateInkHz (0) / 0.001_GHz);
      gpu_load.addValue       ( SK_GPU_GetGPULoad           (0)    );
      gpu_temp_c.addValue     ( SK_GPU_GetTempInC           (0)    );
      vram_used_mib.addValue  ( SK_GPU_GetVRAMUsed          (0) / 1.0_MiB);
      //vram_shared.addValue    (       SK_GPU_GetVRAMShared        (0));
      //fan_rpm.addValue        (       SK_GPU_GetFanSpeedRPM       (0));

      last_update = dwNow;
    }
  }

  void draw (void)
  {
    ImGuiIO& io (ImGui::GetIO ( ));

    const  float font_size           =             ImGui::GetFont  ()->FontSize                        * io.FontGlobalScale;
    const  float font_size_multiline = font_size + ImGui::GetStyle ().ItemSpacing.y + ImGui::GetStyle ().ItemInnerSpacing.y;

    char szAvg  [512] = { };

    sprintf_s
      ( szAvg,
          512,
            u8"GPU%lu Load %%:\n\n\n"
            u8"          min: %3.0f%%, max: %3.0f%%, avg: %4.1f%%\n",
              0,
                gpu_load.getMin (), gpu_load.getMax (),
                  gpu_load.getAvg () );

    float samples = 
      std::min ( (float)gpu_load.getUpdates  (),
                 (float)gpu_load.getCapacity () );

    ImGui::PushStyleColor ( ImGuiCol_PlotLines, 
                              ImColor::HSV ( 0.31f - 0.31f *
                       std::min ( 1.0f, gpu_load.getAvg () / 100.0f ),
                                               0.86f,
                                                 0.95f ) );

    ImGui::PlotLinesC ( "###GPU_LoadPercent",
                         gpu_load.getValues     ().data (),
        static_cast <int> (samples),
                             gpu_load.getOffset (),
                               szAvg,
                                 0.0f,
                                   100.0f,
                                     ImVec2 (
                                       ImGui::GetContentRegionAvailWidth (), font_size * 4.5f) );

    sprintf_s
      ( szAvg,
          512,
            u8"GPU%lu Temp (°C):\n\n\n"
            u8"          min: %3.0f°, max: %3.0f°, avg: %4.1f°\n",
              0,
                gpu_temp_c.getMin   (), gpu_temp_c.getMax (),
                  gpu_temp_c.getAvg () );

    samples = 
      std::min ( (float)gpu_temp_c.getUpdates  (),
                 (float)gpu_temp_c.getCapacity () );

    ImGui::PushStyleColor ( ImGuiCol_PlotLines, 
                              ImColor::HSV ( 0.31f - 0.31f *
                       std::min ( 1.0f, gpu_temp_c.getAvg () / 100.0f ),
                                               0.86f,
                                                 0.95f ) );

    ImGui::PlotLinesC ( "###GPU_TempC",
                         gpu_temp_c.getValues     ().data (),
        static_cast <int> (samples),
                             gpu_temp_c.getOffset (),
                               szAvg,
                                 0.0f,
                                   100.0f,
                                     ImVec2 (
                                       ImGui::GetContentRegionAvailWidth (), font_size * 4.5f) );


    ImGui::PushStyleColor ( ImGuiCol_PlotLines, ImColor::HSV (0.0f, 0.0f, 0.725f) );

    sprintf_s
      ( szAvg,
          512,
            u8"GPU%lu Core Clock (GHz):\n\n\n"
            u8"          min: %4.2f, max: %4.2f, avg: %5.3f\n",
              0,
                core_clock_ghz.getMin   (), core_clock_ghz.getMax (),
                  core_clock_ghz.getAvg () );

    samples = 
      std::min ( (float)core_clock_ghz.getUpdates  (),
                 (float)core_clock_ghz.getCapacity () );

    ImGui::PlotLinesC ( "###GPU_CoreClock",
                         core_clock_ghz.getValues ().data (),
        static_cast <int> (samples),
                             core_clock_ghz.getOffset (),
                               szAvg,
                                 core_clock_ghz.getMin   () / 2.0f,
                                   core_clock_ghz.getMax () * 1.05f,
                                     ImVec2 (
                                       ImGui::GetContentRegionAvailWidth (), font_size * 4.5f) );

    sprintf_s
      ( szAvg,
          512,
            u8"GPU%lu VRAM Clock (GHz):\n\n\n"
            u8"          min: %4.2f, max: %4.2f, avg: %5.3f\n",
              0,
                vram_clock_ghz.getMin   (), vram_clock_ghz.getMax (),
                  vram_clock_ghz.getAvg () );

    samples = 
      std::min ( (float)vram_clock_ghz.getUpdates  (),
                 (float)vram_clock_ghz.getCapacity () );

    ImGui::PlotLinesC ( "###GPU_VRAMClock",
                         vram_clock_ghz.getValues ().data (),
        static_cast <int> (samples),
                             vram_clock_ghz.getOffset (),
                               szAvg,
                                 vram_clock_ghz.getMin   () / 2.0f,
                                   vram_clock_ghz.getMax () * 1.05f,
                                     ImVec2 (
                                       ImGui::GetContentRegionAvailWidth (), font_size * 4.5f) );

    // TODO: Add a parameter to data history to control this
    static float max_use = 0.0f;

    max_use = std::max (vram_used_mib.getMax (), max_use);

    sprintf_s
      ( szAvg,
          512,
            u8"GPU%lu VRAM Usage (MiB):\n\n\n"
            u8"          min: %6.1f, max: %6.1f, avg: %6.1f\n",
              0,
                vram_used_mib.getMin   (), max_use,
                  vram_used_mib.getAvg () );

    samples = 
      std::min ( (float)vram_used_mib.getUpdates  (),
                 (float)vram_used_mib.getCapacity () );

    float capacity_in_mib =
      static_cast <float> (SK_GPU_GetVRAMBudget (0) >> 20ULL);

    if (capacity_in_mib <= 0.0f)
      capacity_in_mib = 4096.0f; // Just take a wild guess, lol

    ImGui::PushStyleColor ( ImGuiCol_PlotLines, 
                              ImColor::HSV ( 0.31f - 0.31f *
                       std::min ( 1.0f, vram_used_mib.getAvg () / capacity_in_mib ),
                                               0.86f,
                                                 0.95f ) );

    ImGui::PlotLinesC ( "###GPU_VRAMUsage",
                         vram_used_mib.getValues ().data (),
        static_cast <int> (samples),
                             vram_used_mib.getOffset (),
                               szAvg,
                                 0.0f,//vram_used_mib.getMin   (),//0.0f,
                                   capacity_in_mib * 1.05f,
                                     ImVec2 (
                                       ImGui::GetContentRegionAvailWidth (), font_size * 4.5f) );

    ImGui::PopStyleColor (4);
  }

  void OnConfig (ConfigEvent event)
  {
    switch (event)
    {
      case SK_Widget::ConfigEvent::LoadComplete:
        break;

      case SK_Widget::ConfigEvent::SaveStart:
        break;
    }
  }

protected:
  const DWORD update_freq = 666UL;

private:
  DWORD last_update = 0UL;

  SK_ImGui_DataHistory <float,    96> core_clock_ghz;
  SK_ImGui_DataHistory <float,    96> vram_clock_ghz;
  SK_ImGui_DataHistory <float,    96> gpu_load;
  SK_ImGui_DataHistory <float,    96> gpu_temp_c;
  SK_ImGui_DataHistory <float,    96> vram_used_mib;
  SK_ImGui_DataHistory <uint64_t, 96> vram_shared;
  SK_ImGui_DataHistory <uint32_t, 96> fan_rpm;
} __gpu_monitor__;

