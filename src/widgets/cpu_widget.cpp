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

#include <SpecialK/performance/gpu_monitor.h>
#include <SpecialK/performance/io_monitor.h>

#include <SpecialK/control_panel.h>
#include <SpecialK/utility.h>

#include <powerbase.h>
#include <WinBase.h>
#pragma comment (lib, "PowrProf.lib")

#include <SpecialK/tls.h>

extern iSK_INI* osd_ini;

class SKWG_CPU_Monitor : public SK_Widget
{
public:
  SKWG_CPU_Monitor (void) : SK_Widget ("CPUMonitor")
  {
    SK_ImGui_Widgets.cpu_monitor = this;

    setAutoFit (true).setDockingPoint (DockAnchor::East).setClickThrough (true);
  };

  virtual void run (void) override
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


    if (last_update < SK::ControlPanel::current_time - update_freq)
    {
      if (cpu_stats.num_cpus > 0 && cpu_records.empty ())
        cpu_records.resize (cpu_stats.num_cpus);

      for (unsigned int i = 0; i < cpu_records.size (); i++)
      {
        cpu_records [i].addValue (static_cast <float> (cpu_stats.cpus [i].percent_load));
      }

      last_update = SK::ControlPanel::current_time;
    }
  }

  virtual void draw (void) override
  {
    if (! ImGui::GetFont ()) return;

    const  float font_size   = ImGui::GetFont ()->FontSize;
    static char  szAvg [512] = { };
    //static bool  stress      = config.render.framerate.max_delta_time == 500 ?
    //                             true : false;
    //
    //if (SK_ImGui::VerticalToggleButton ("Stress Test (100% CPU Load)", &stress))
    //{
    //  if (stress) config.render.framerate.max_delta_time = 500;
    //  else        config.render.framerate.max_delta_time = 0;
    //}
    //
    //ImGui::SameLine   ();
    ImGui::BeginGroup ();


	typedef struct _PROCESSOR_POWER_INFORMATION {
		ULONG Number;
		ULONG MaxMhz;
		ULONG CurrentMhz;
		ULONG MhzLimit;
		ULONG MaxIdleState;
		ULONG CurrentIdleState;
	} PROCESSOR_POWER_INFORMATION, *PPROCESSOR_POWER_INFORMATION;

	SYSTEM_INFO        sinfo = { };
	SK_GetSystemInfo (&sinfo);

	PPROCESSOR_POWER_INFORMATION pwi =
		reinterpret_cast <PPROCESSOR_POWER_INFORMATION> (
			SK_TLS_Bottom ()->scratch_memory.cpu_info.alloc(
				sizeof (PROCESSOR_POWER_INFORMATION) * sinfo.dwNumberOfProcessors
			)
		);

	CallNtPowerInformation ( ProcessorInformation,
		                       nullptr, 0,
		                       pwi,     sizeof (PROCESSOR_POWER_INFORMATION) * sinfo.dwNumberOfProcessors
		                   );

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

	  bool found = false;

	  auto DrawHeader = [&](int j) -> bool
	  {
		if (pwi [j].Number == i)
		{
		  found = true;

		  ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.28F, 1.f, 1.f, 1.f));
		  ImGui::Text           ("%#4lu MHz", pwi [j].CurrentMhz);
		  ImGui::SameLine       ();

		  ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.12F, .8f, .95f, 1.f));
		  ImGui::Text           ("/ [%#4lu MHz]", pwi [j].MaxMhz);

		  //ImGui::SameLine (); ImGui::Spacing ();
		  //ImGui::SameLine (); ImGui::Spacing ();
		  //
		  //ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.57194F, 1.0f, .9f, 1.f));
		  //ImGui::Text           ("Idle: %3lu%%", pwi [j].CurrentIdleState);
		  //
		  //ImGui::PopStyleColor (3);
		  ImGui::PopStyleColor (2);

		  return true;
		}

		return false;
	  };

	  for (DWORD j = i ; j < sinfo.dwNumberOfProcessors; j++)
	  {
            found = DrawHeader (j);
        if (found) break;
      }

	  if (! found)
	  {
		for (int j = sinfo.dwNumberOfProcessors - 1; j >= 0; j--)
		{
		  if (DrawHeader (j)) break;
		}
	  }


      ImGui::PopStyleColor ();
    }

    ImGui::EndGroup ();
  }

  virtual void OnConfig (ConfigEvent event) override
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

  std::vector <SK_Stat_DataHistory <float, 96>> cpu_records;
} __cpu_monitor__;