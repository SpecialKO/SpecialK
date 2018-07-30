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

#include <SpecialK/framerate.h>

#include <SpecialK/control_panel.h>
#include <SpecialK/utility.h>

#include <powrprof.h>
#include <powerbase.h>
#include <powersetting.h>
#include <WinBase.h>
#include <winnt.h>
#pragma comment (lib, "PowrProf.lib")

#include <SpecialK/tls.h>
#include <SpecialK/log.h>

extern iSK_INI* osd_ini;

ULONG
SK_CPU_DeviceNotifyCallback (
  PVOID Context,
  ULONG Type,
  PVOID Setting );

class SKWG_CPU_Monitor : public SK_Widget
{
public:
  struct power_scheme_s {
    GUID uid;
    char utf8_name [128];
    char utf8_desc [384];
  };

  void
  resolveNameAndDescForPowerScheme (power_scheme_s& scheme)
  {
    DWORD   dwRet         = ERROR_NOT_READY;
    DWORD   dwLen         = 127;
    wchar_t wszName [128] = { };
    wchar_t wszDesc [384] = { };

    dwRet =
      PowerReadFriendlyName ( nullptr,
                               &scheme.uid,
                                  nullptr, nullptr,
                                    (PUCHAR)wszName,
                                           &dwLen );

    if (dwRet == ERROR_MORE_DATA)
    {
      strncpy (scheme.utf8_name, "Too Long <Fix Me>", 127);
    }

    else if (dwRet == ERROR_SUCCESS)
    {
      dwLen = 383;
      dwRet =
        PowerReadDescription ( nullptr,
                                 &scheme.uid,
                                   nullptr, nullptr,
                             (PUCHAR)wszDesc,
                                    &dwLen );

      if (dwRet == ERROR_MORE_DATA)
      {
        strncpy (scheme.utf8_desc, "Description Long <Fix Me>", 383);
      }

      else if (dwRet == ERROR_SUCCESS)
      {
        strncpy (
          scheme.utf8_desc,
            SK_WideCharToUTF8 (wszDesc).c_str (),
              383
        );
      }

      strncpy (
        scheme.utf8_name,
          SK_WideCharToUTF8 (wszName).c_str (),
            127
      );
    }
  }

public:
  SKWG_CPU_Monitor (void) : SK_Widget ("CPUMonitor")
  {
    SK_ImGui_Widgets.cpu_monitor = this;

    setAutoFit (true).setDockingPoint (DockAnchor::East).setClickThrough (true);

    active_scheme.notify   = INVALID_HANDLE_VALUE;

    memset (active_scheme.utf8_name, 0, 128);
    memset (active_scheme.utf8_desc, 0, 256);

    InterlockedExchange (&active_scheme.dirty, 0);

    if (active_scheme.notify == INVALID_HANDLE_VALUE)
    {
      _DEVICE_NOTIFY_SUBSCRIBE_PARAMETERS dnsp = {
        SK_CPU_DeviceNotifyCallback, nullptr
      };
      
      PowerSettingRegisterNotification ( &GUID_POWERSCHEME_PERSONALITY,
                                           DEVICE_NOTIFY_CALLBACK,
                                             (HANDLE)&dnsp,
                                               &active_scheme.notify );
    }
  };

  virtual ~SKWG_CPU_Monitor (void)
  {
    if (active_scheme.notify != INVALID_HANDLE_VALUE)
    {
      DWORD dwRet =
        PowerSettingUnregisterNotification (active_scheme.notify);

      assert (dwRet == ERROR_SUCCESS);

      if (dwRet == ERROR_SUCCESS)
      {
        active_scheme.notify = INVALID_HANDLE_VALUE;
      }
    }

    PowerSetActiveScheme (nullptr, &config.cpu.power_scheme_guid_orig);
  }

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

    typedef struct _PROCESSOR_POWER_INFORMATION {
      ULONG Number;
      ULONG MaxMhz;
      ULONG CurrentMhz;
      ULONG MhzLimit;
      ULONG MaxIdleState;
      ULONG CurrentIdleState;
    } PROCESSOR_POWER_INFORMATION,
     *PPROCESSOR_POWER_INFORMATION;

    SYSTEM_INFO        sinfo = { };
    SK_GetSystemInfo (&sinfo);

    PPROCESSOR_POWER_INFORMATION pwi =
      reinterpret_cast <PPROCESSOR_POWER_INFORMATION>   (
        SK_TLS_Bottom ()->scratch_memory.cpu_info.alloc (
        sizeof (PROCESSOR_POWER_INFORMATION) * sinfo.dwNumberOfProcessors
      )
    );

    CallNtPowerInformation ( ProcessorInformation,
                            nullptr, 0,
                            pwi,     sizeof (PROCESSOR_POWER_INFORMATION) * sinfo.dwNumberOfProcessors
    );

    if ( last_update           < ( SK::ControlPanel::current_time - update_freq ) &&
          cpu_stats.num_cpus   > 0 )
    {
      if ( cpu_stats.num_cpus  > 0 &&
         ( cpu_records.size () < ( cpu_stats.num_cpus + 1 )
         )
         ) { cpu_records.resize  ( cpu_stats.num_cpus + 1 ); }

      for (unsigned int i = 1; i < cpu_stats.num_cpus + 1 ; i++)
      {
        cpu_records [i].addValue(
          static_cast  <float>   (
                     ReadAcquire ( &cpu_stats.cpus [pwi [i-1].Number].percent_load )
                                 )
                                 );

        cpu_stats.cpus [pwi [i-1].Number].CurrentMhz =
          pwi [i-1].CurrentMhz;
        cpu_stats.cpus [pwi [i-1].Number].MaximumMhz =
          pwi [i-1].MaxMhz;
      }

      cpu_records [0].addValue   (
          static_cast  <float>   (
                     ReadAcquire ( &cpu_stats.cpus [64].percent_load )
                                 )
                                 );

      last_update =
        SK::ControlPanel::current_time;

      float avg_clock = 0.0f;

      for (unsigned int i = 0; i < sinfo.dwNumberOfProcessors; i++)
      {
        avg_clock +=
          static_cast <float> (pwi [i].CurrentMhz);
      }

      cpu_clocks.addValue ( avg_clock /
                              static_cast <float> (sinfo.dwNumberOfProcessors) );

      bool changed =
        ReadAcquire (&active_scheme.dirty) > 0;

      if (changed)
      {
        GUID* pGUID = nullptr;

        if ( ERROR_SUCCESS == PowerGetActiveScheme ( nullptr,
                                                       &pGUID ) )
        {
          InterlockedDecrement (&active_scheme.dirty);

          active_scheme.uid =
            *pGUID;

          SK_LocalFree ((HLOCAL)pGUID);

          resolveNameAndDescForPowerScheme (active_scheme);
        }
      }
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
        if (j == 0)
        { 
          found = true;

          ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.28F, 1.f, 1.f, 1.f));
          ImGui::Text           ("%#4.2f GHz", cpu_clocks.getAvg () / 1000.0f);
          ImGui::PopStyleColor  (1);

          if ( ReadAcquire (&active_scheme.dirty) == 0 )
          {
            static std::vector <power_scheme_s> schemes;

            ImGui::SameLine (); ImGui::Spacing ();
            ImGui::SameLine (); ImGui::Spacing ();
            ImGui::SameLine ();

            ImGui::TextUnformatted ("Power Scheme: ");
            ImGui::SameLine        ();
            ImGui::PushStyleColor  (ImGuiCol_Text, ImColor::HSV (0.14583f, 0.98f, .97f, 1.f));

            static bool select_scheme = false;

            select_scheme =
              ImGui::Selectable (active_scheme.utf8_name);

            if (ImGui::IsItemHovered ())
            {
              ImGui::SetTooltip (active_scheme.utf8_desc);
            }

            if (select_scheme)
            {
              if (schemes.empty ())
              {
                int   scheme_idx  =  0;
                GUID  scheme_guid = { };
                DWORD guid_size   = sizeof (GUID);

                while ( ERROR_SUCCESS ==
                          PowerEnumerate ( nullptr, nullptr, nullptr,
                                             ACCESS_SCHEME, scheme_idx++, (UCHAR *)&scheme_guid, &guid_size )
                      )
                {
                  power_scheme_s scheme;
                                 scheme.uid = scheme_guid;

                  resolveNameAndDescForPowerScheme (scheme);
                  schemes.emplace_back             (scheme);

                  scheme_guid = { };
                  guid_size   = sizeof (GUID);
                }
              }

              ImGui::OpenPopup ("Power Scheme Selector");
            }

            ImGui::PopStyleColor   (1);

            if ( ImGui::BeginPopupModal ( "Power Scheme Selector",
                 nullptr,
                 ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_ShowBorders |
                 ImGuiWindowFlags_NoScrollbar      | ImGuiWindowFlags_NoScrollWithMouse )
               )
            {
              bool end_dialog = false;

              ImGui::FocusWindow (ImGui::GetCurrentWindow ());

              for (auto& it : schemes)
              {
                bool selected =
                  InlineIsEqualGUID ( it.uid, active_scheme.uid );

                ImGui::PushStyleColor  (ImGuiCol_Text, ImColor (1.f, 1.f, 1.f, 1.f));
                if (ImGui::Selectable  (it.utf8_name, &selected))
                {
                  PowerSetActiveScheme (nullptr, &it.uid);

                  config.cpu.power_scheme_guid =  it.uid;

                  end_dialog = true;
                }

                ImGui::PushStyleColor  (ImGuiCol_Text, ImColor (0.73f, 0.73f, 0.73f, 1.f));
                ImGui::TreePush        (""          );
                ImGui::TextUnformatted (it.utf8_desc);
                ImGui::TreePop         (            );
                ImGui::PopStyleColor   (      2     );
              }

              if (end_dialog)
                ImGui::CloseCurrentPopup (         );

              ImGui::EndPopup ();
            }
          }

          return true;
        }

        else
        {
          found = true;

          ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.28F, 1.f, 1.f, 1.f));
          ImGui::Text           ("%#4lu MHz", cpu_stats.cpus [j-1].CurrentMhz);
          ImGui::SameLine       ();
    
          ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.12F, .8f, .95f, 1.f));
          ImGui::Text           ("/ [%#4lu MHz]", cpu_stats.cpus [j-1].MaximumMhz);

          uint64_t parked_since =
            cpu_stats.cpus [j-1].parked_since.QuadPart;

          if (parked_since > 0)
          {
            ImGui::SameLine (); ImGui::Spacing ();
            ImGui::SameLine (); ImGui::Spacing (); ImGui::SameLine ();

            ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.57194F, 0.534f, .94f, 1.f));
            ImGui::Text           ( "Parked For %4.1f Seconds",
                                      SK_DeltaPerfMS ( parked_since, 1 ) / 1000.0
                                  );
          }

          //ImGui::PopStyleColor (3);
          ImGui::PopStyleColor (2);

          return true;
        }

        return false;
      };
      
      DrawHeader (i);

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

  void signalDeviceChange (void)
  {
    InterlockedIncrement (&active_scheme.dirty);
  }

protected:
  const DWORD update_freq        = 666UL;

private:
  struct active_scheme_s : power_scheme_s {
    HPOWERNOTIFY  notify;
    volatile LONG dirty;
  } active_scheme;

  DWORD       last_update        = 0UL;

  std::vector <SK_Stat_DataHistory <float, 96>> cpu_records;
               SK_Stat_DataHistory <float,  3>  cpu_clocks;
} __cpu_monitor__;

ULONG
SK_CPU_DeviceNotifyCallback (
  PVOID Context,
  ULONG Type,
  PVOID Setting )
{
  UNREFERENCED_PARAMETER (Context);
  UNREFERENCED_PARAMETER (Type);
  UNREFERENCED_PARAMETER (Setting);

  __cpu_monitor__.signalDeviceChange ();

  return 1;
}

/**
░▓┏──────────┓▓░LVT Thermal Sensor
░▓┃ APICx330 ┃▓░╰╾╾╾ThermalLvtEntry
░▓┗──────────┛▓░ 

Reset: 0001_0000h.

  Interrupts for this local vector table are caused by changes
    in Core::X86::Msr::PStateCurLim [CurPstateLimit] due to
      SB-RMI or HTC.

  Core::X86::Apic::ThermalLvtEntry_lthree [ 1 : 0 ]
                                  _core   [ 3 : 0 ]
                                  _thread [ 1 : 0 ];
                                  
░▓┏──────────┓▓░ { Core::X86::Msr::APIC_BAR [
░▓┃ APICx330 ┃▓░                    ApicBar [ 47 : 12 ]
░▓┗──────────┛▓░                            ],          000h }
**/