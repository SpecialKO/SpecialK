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
#define _CRT_SECURE_NO_WARNINGS

#include <SpecialK/widgets/widget.h>
#include <SpecialK/gpu_monitor.h>

#include <SpecialK/io_monitor.h>

extern iSK_INI* osd_ini;

class SKWG_CPU_Monitor : public SK_Widget
{
public:
  SKWG_CPU_Monitor (void) : SK_Widget ("CPUMonitor")
  {
    SK_ImGui_Widgets.cpu_monitor = this;

    setAutoFit (true).setDockingPoint (DockAnchor::East).setClickThrough (true);
  };

  void run (void)
  {
    static bool started = false;

    if ((! config.cpu.show) || (! started))
    {
      started = true;

      void
      __stdcall
      SK_StartPerfMonThreads (void);
      SK_StartPerfMonThreads (    );
    }


    DWORD dwNow = timeGetTime ();

    if (last_update < dwNow - update_freq)
    {
      if (cpu_stats.num_cpus > 0 && cpu_records.empty ())
        cpu_records.resize (cpu_stats.num_cpus);

      for (unsigned int i = 0; i < cpu_records.size (); i++)
      {
        cpu_records [i].addValue ((float)cpu_stats.cpus [i].percent_load);
      }

      last_update = dwNow;
    }
  }

  void draw (void)
  {
    ImGuiIO& io (ImGui::GetIO ());

    const  float font_size           =             ImGui::GetFont  ()->FontSize                        * io.FontGlobalScale;
    const  float font_size_multiline = font_size + ImGui::GetStyle ().ItemSpacing.y + ImGui::GetStyle ().ItemInnerSpacing.y;

    static char szAvg [512] = { };

    for (unsigned int i = 0; i < cpu_records.size (); i++)
    {
      if (i > 0)
      {
        sprintf_s
              ( szAvg,
                  512,
                    u8"CPU%lu:\n\n"
                    u8"          min: %3.0f%%, max: %3.0f%%, avg: %3.0f%%\n",
                      i-1,
                        cpu_records [i].getMin (), cpu_records [i].getMax (),
                        cpu_records [i].getAvg () );
      }

      else
      {
        sprintf_s
              ( szAvg,
                  512,
                    u8"Combined CPU Load:\n\n"
                    u8"          min: %3.0f%%, max: %3.0f%%, avg: %3.0f%%\n",
                      cpu_records [i].getMin (), cpu_records [i].getMax (),
                      cpu_records [i].getAvg () );
      }

      char szName [128] = { };

      sprintf (szName, "###CPU_%u", i-1);

      float samples = 
        std::min ( (float)cpu_records [i].getUpdates  (),
                   (float)cpu_records [i].getCapacity () );

      ImGui::PushStyleColor ( ImGuiCol_PlotLines, 
                                ImColor::HSV ( 0.31f - 0.31f *
                         std::min ( 1.0f, cpu_records [i].getAvg () / 100.0f ),
                                                 0.86f,
                                                   0.95f ) );

      ImGui::PlotLinesC ( szName,
                           cpu_records [i].getValues     ().data (),
          static_cast <int> (samples),
                               cpu_records [i].getOffset (),
                                 szAvg,
                                   -1.0f,
                                     101.0f,
                                       ImVec2 (
                                         std::max (500.0f, ImGui::GetContentRegionAvailWidth ()), font_size * 4.0f) );

      ImGui::PopStyleColor ();
    }
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

  std::vector <SK_ImGui_DataHistory <float, 96>> cpu_records;
} __cpu_monitor__;