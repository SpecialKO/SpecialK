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
#include <filesystem>

extern iSK_INI* osd_ini;

ULONG
CALLBACK
SK_CPU_DeviceNotifyCallback (
  PVOID Context,
  ULONG Type,
  PVOID Setting
);

#include <SpecialK/resource.h>

#include "../depends//include/WinRing0/OlsApi.h"

InitializeOls_pfn       InitializeOls       = nullptr;
DeinitializeOls_pfn     DeinitializeOls     = nullptr;
ReadPciConfigDword_pfn  ReadPciConfigDword  = nullptr;
WritePciConfigDword_pfn WritePciConfigDword = nullptr;
RdmsrTx_pfn             RdmsrTx             = nullptr;
Rdmsr_pfn               Rdmsr               = nullptr;
Rdpmc_pfn               Rdpmc               = nullptr;

BOOL  WINAPI InitializeOls_NOP       ( void               ) {return FALSE;}
VOID  WINAPI DeinitializeOls_NOP     ( void               ) {}
DWORD WINAPI ReadPciConfigDword_NOP  ( DWORD /*unused*/, BYTE         /*unused*/) {return 0;    }
VOID  WINAPI WritePciConfigDword_NOP ( DWORD /*unused*/, BYTE /*unused*/, DWORD  /*unused*/) {}
BOOL  WINAPI RdmsrTx_NOP             ( DWORD /*unused*/, PDWORD /*unused*/,
                                       PDWORD /*unused*/,DWORD_PTR    /*unused*/) {return FALSE;}
BOOL  WINAPI Rdmsr_NOP               ( DWORD /*unused*/, PDWORD /*unused*/,
                                       PDWORD              /*unused*/) {return FALSE;}
BOOL  WINAPI Rdpmc_NOP               ( DWORD /*unused*/, PDWORD /*unused*/, PDWORD /*unused*/ ) { return FALSE; }

volatile LONG __SK_WR0_Init  = 0L;
static HMODULE  hModWinRing0 = nullptr;
         bool   SK_CPU_IsZen          (bool retest = false);
         void   SK_WinRing0_Install   (void);
         void   SK_WinRing0_Uninstall (void);
volatile LONG __SK_WR0_NoThreads = 0;

void
SK_WinRing0_Unpack (void)
{
  return;
};

enum SK_CPU_IntelMicroarch
{
  NetBurst,
  Core,
  Atom,
  Nehalem,
  SandyBridge,
  IvyBridge,
  Haswell,
  Broadwell,
  Silvermont,
  Skylake,
  Airmont,
  KabyLake,
  ApolloLake,
  IceLake,

  KnownIntelArchs,

  UnknownIntel,
  NotIntel
};

enum SK_CPU_IntelMSR
{
  PLATFORM_INFO             = 0x00CE,

  IA32_PERF_STATUS          = 0x0198,
  IA32_THERM_STATUS         = 0x019C,
  IA32_PACKAGE_THERM_STATUS = 0x01B1,
  IA32_TEMPERATURE_TARGET   = 0x01A2,

  RAPL_POWER_UNIT           = 0x0606,
  PKG_ENERGY_STATUS         = 0x0611,
  DRAM_ENERGY_STATUS        = 0x0619,
  PP0_ENERGY_STATUS         = 0x0639,
  PP1_ENERGY_STATUS         = 0x0641
};

struct SK_CPU_CoreSensors
{
  double   power_W       = 0.0;
  double   clock_MHz     = 0.0;

  double   temperature_C = 0.0;
  double   tjMax         = 0.0;

  struct accum_result_s {
    double elapsed_ms    = 0.0;
    double value         = 0.0;
  };

  struct accumulator_s {
    volatile  LONG64 update_time  =      0LL;
    volatile ULONG64 tick         =      0ULL;

    accum_result_s  result        = { 0.0, 0.0 };

    accum_result_s
    update ( uint64_t new_val,
             double   coeff )
    {
      const ULONG64 _tick =
        ReadULong64Acquire (&tick);

      //if (new_val != _tick)
      {
        const double elapsed_ms =
          SK_DeltaPerfMS (ReadAcquire64 (&update_time), 1);

        const uint64_t delta =
            new_val - _tick;

        result.value      = static_cast <double> (delta) * coeff;
        result.elapsed_ms =                           elapsed_ms;

        WriteULong64Release (&tick,                         new_val);
        WriteRelease64      (&update_time, SK_QueryPerf ().QuadPart);

      //dll_log->Log (L"elapsed_ms=%f, value=%f, core=%p", result.elapsed_ms, result.value, this);
      }

      return
        result;
    }
  } accum, accum2;

   int64_t sample_taken = 0;
  uint32_t cpu_core     = 0;
};

struct SK_CPU_Package
{
  SK_CPU_Package (void)
  {
    SYSTEM_INFO        sinfo = { };
    SK_GetSystemInfo (&sinfo);

    for ( core_count = 0 ; core_count < sinfo.dwNumberOfProcessors ; core_count++ )
    {
      cores [core_count].cpu_core = core_count;
    }

    pkg_sensor.cpu_core =
      std::numeric_limits <uint32_t>::max ();
  }

  SK_CPU_IntelMicroarch intel_arch  = SK_CPU_IntelMicroarch::KnownIntelArchs;
  int                   amd_zen_idx = 0;

  //enum SensorFlags {
  //  PerPackageThermal = 0x1,
  //  PerPackageEnergy  = 0x2,
  //  PerCoreThermal    = 0x4,
  //  PerCoreEnergy     = 0x8
  //} sensors;

  struct sensor_fudge_s
  {
    // Most CPUs measure tiny fractions of a Joule, we need to know
    //   what that fraction is so we can work-out Wattage.
    double energy = 1.0;
    double power  = 1.0;
    double time   = 1.0; // Granularity of RAPL sensor (adjusted to ms)

    struct
    {
      double tsc = 1.0;
    } intel;
  } coefficients;

  struct {
    double temperature = 0.0;
  } offsets;

  SK_CPU_CoreSensors pkg_sensor = { };
  SK_CPU_CoreSensors cores [64] = { };
  DWORD              core_count =  0 ;
} ;

SK_CPU_Package& __SK_CPU__ (void)
{
  static SK_CPU_Package cpu;
  return                cpu;
}

#define __SK_CPU __SK_CPU__()


double
SK_CPU_GetIntelTjMax (DWORD_PTR core)
{
  return 100.0;
}

void
SK_WR0_Deinit (void)
{
  return;
}

bool
SK_WR0_Init (void)
{
  return false;
}

SK_CPU_IntelMicroarch
SK_CPU_GetIntelMicroarch (void)
{
  auto& cpu =
    __SK_CPU;

  return
    cpu.intel_arch;
}


struct SK_CPU_ZenCoefficients
{
  double power  = 0.0,
         energy = 0.0,
         time   = 0.0;
};

void
SK_CPU_MakePowerUnit_Zen (SK_CPU_ZenCoefficients* pCoeffs)
{
  std::ignore = pCoeffs;

  return;
}

bool
SK_CPU_IsZen (bool retest/* = false*/)
{
  return false;
}

//typedef struct
//{
//  union {
//    struct {
//      unsigned int edx;
//      unsigned int eax;
//    };
//    unsigned long long
//      PU_PowerUnits    :   4 - 0,
//      Reserved2        :   8 - 4,
//      ESU_EnergyUnits  :  13 - 8,
//      Reserved1        :  16 - 13,
//      TU_TimeUnits     :  20 - 16,
//      Reserved0        :  64 - 20;
//  };
//} RAPL_POWER_UNIT;


DWORD_PTR
WINAPI
SK_SetThreadAffinityMask (HANDLE hThread, DWORD_PTR mask)
{
  return
    ( SetThreadAffinityMask_Original != nullptr      ?
      SetThreadAffinityMask_Original (hThread, mask) :
      SetThreadAffinityMask          (hThread, mask) );
}

void
SK_CPU_AssertPowerUnit_Zen (int64_t core)
{
  return;
}

double
SK_CPU_GetJoulesConsumedTotal (DWORD_PTR package)
{
  return 0.0;
}

double
SK_CPU_GetTemperature_AMDZen (int core)
{
  return 0.0;
}

struct SK_ClockTicker {
  constexpr SK_ClockTicker (void) = default;

  double cumulative_MHz = 0.0;
  int    num_cores      =   0;

  uint64_t getAvg (void)
  {
    return (num_cores > 0)  ?
      static_cast <uint64_t> ( cumulative_MHz /
      static_cast < double > ( num_cores ) )  : 0;
  }
};

SK_LazyGlobal <SK_ClockTicker> __AverageEffectiveClock;

#define UPDATE_INTERVAL_MS (1000.0 * config.cpu.interval)

extern void SK_PushMultiItemsWidths (int components, float w_full = 0.0f);

void
SK_CPU_UpdatePackageSensors (int package)
{
  std::ignore = package;

  return;
}

void
SK_CPU_UpdateCoreSensors (int core_idx)
{
  std::ignore = core_idx;

  return;
}


void
SK_CPU_UpdateAllSensors (void)
{
  return;
}

#pragma pack (push,8)
// Used only if more accurate MSR-based data cannot be
//   sensed.
typedef struct _PROCESSOR_POWER_INFORMATION {
  ULONG Number;
  ULONG MaxMhz;
  ULONG CurrentMhz;
  ULONG MhzLimit;
  ULONG MaxIdleState;
  ULONG CurrentIdleState;
} PROCESSOR_POWER_INFORMATION,
 *PPROCESSOR_POWER_INFORMATION;
#pragma pack(pop)

class SKWG_CPU_Monitor : public SK_Widget
{
public:
  struct power_scheme_s {
    GUID uid;
    char utf8_name [512];
    char utf8_desc [512];
  };

  void
  resolveNameAndDescForPowerScheme (power_scheme_s& scheme)
  {
    DWORD   dwLen         = 511;
    wchar_t wszName [512] = { };

    DWORD dwRet =
      PowerReadFriendlyName ( nullptr,
                               &scheme.uid,
                                  nullptr, nullptr,
                                    (PUCHAR)wszName,
                                           &dwLen );

    if (dwRet == ERROR_MORE_DATA)
    {
      strncpy (scheme.utf8_name, "Too Long <Fix Me>", 511);
    }

    else if (dwRet == ERROR_SUCCESS)
    {
      wchar_t
      wszDesc [512] = { };
      dwLen =  511;
      dwRet =
        PowerReadDescription ( nullptr,
                                 &scheme.uid,
                                   nullptr, nullptr,
                             (PUCHAR)wszDesc,
                                    &dwLen );

      if (dwRet == ERROR_MORE_DATA)
      {
        strncpy (scheme.utf8_desc, "Description Long <Fix Me>", 511);
      }

      else if (dwRet == ERROR_SUCCESS)
      {
        strncpy (
          scheme.utf8_desc,
            SK_WideCharToUTF8 (wszDesc).c_str (),
              511
        );
      }

      strncpy (
        scheme.utf8_name,
          SK_WideCharToUTF8 (wszName).c_str (),
            511
      );
    }
  }

public:
  SKWG_CPU_Monitor (void) : SK_Widget ("CPUMonitor")
  {
    SK_ImGui_Widgets->cpu_monitor = this;

    setAutoFit (true).setDockingPoint (DockAnchor::East).
    setBorder  (true);

    active_scheme.notify   = INVALID_HANDLE_VALUE;

    memset (active_scheme.utf8_name, 0, 512);
    memset (active_scheme.utf8_desc, 0, 512);

    InterlockedExchange (&active_scheme.dirty, 0);

    if (active_scheme.notify == INVALID_HANDLE_VALUE)
    {
      _DEVICE_NOTIFY_SUBSCRIBE_PARAMETERS dnsp = {
        SK_CPU_DeviceNotifyCallback, nullptr
      };

      if (! config.compatibility.using_wine)
      {
        PowerSettingRegisterNotification ( &GUID_POWERSCHEME_PERSONALITY,
                                             DEVICE_NOTIFY_CALLBACK,
                                               (HANDLE)&dnsp,
                                                 &active_scheme.notify );
      }
    }
  };

  ~SKWG_CPU_Monitor (void)
  {
    if (active_scheme.notify != INVALID_HANDLE_VALUE)
    {
      const DWORD dwRet =
        PowerSettingUnregisterNotification (active_scheme.notify);

      assert (dwRet == ERROR_SUCCESS);

      if (dwRet == ERROR_SUCCESS)
      {
        active_scheme.notify = INVALID_HANDLE_VALUE;
      }
    }

    PowerSetActiveScheme (nullptr, &config.cpu.power_scheme_guid_orig);
  }

  void run (void) override
  {
    // Wine / Proton / Whatever does not implement most of this; abandon ship!
    if (config.compatibility.using_wine)
      return;

    /// -------- Line of No WINE --------)

    static bool                   started = false;
    if ((! config.cpu.show) || (! started))
    {                             started = true;
      SK_StartPerfMonThreads ();
    }

    static auto& cpu_stats =
      *SK_WMI_CPUStats;

    if ( last_update         < ( SK::ControlPanel::current_time - update_freq ) &&
          cpu_stats.num_cpus > 0 )
    {
      SK_TLS *pTLS =
            SK_TLS_Bottom ();

      // Processor count is not going to change at run-time or I will eat my hat
      static SYSTEM_INFO             sinfo = { };
      SK_RunOnce (SK_GetSystemInfo (&sinfo));

      PPROCESSOR_POWER_INFORMATION pwi =
        reinterpret_cast <PPROCESSOR_POWER_INFORMATION> (
          pTLS != nullptr ?
          pTLS->scratch_memory->cpu_info.alloc (
            sizeof (PROCESSOR_POWER_INFORMATION) * sinfo.dwNumberOfProcessors )
                          : nullptr                     );

      if (pwi == nullptr) return; // Ohnoes, I No Can Haz RAM (!!)

      static bool bUseNtPower = true;
      if (        bUseNtPower)
      {
        const NTSTATUS ntStatus =
          CallNtPowerInformation ( ProcessorInformation,
                                   nullptr, 0,
                                   pwi,     sizeof (PROCESSOR_POWER_INFORMATION) * sinfo.dwNumberOfProcessors );

        bUseNtPower =
          NT_SUCCESS (ntStatus);

        if (! bUseNtPower)
        {
          SK_LOG0 ( ( L"Disabling CallNtPowerInformation (...) due to failed result: %x", ntStatus ),
                      L"CPUMonitor" );
        }
      }

      if (   cpu_records.size () < ( static_cast <size_t> (cpu_stats.num_cpus) + 1 )
         ) { cpu_records.resize    ( static_cast <size_t> (cpu_stats.num_cpus) + 1 ); }

      for (unsigned int i = 1; i < cpu_stats.num_cpus + 1 ; i++)
      {
        auto& stat_cpu =
          cpu_stats.cpus [pwi [i-1].Number];

        cpu_records [i].addValue(
          static_cast  <float>   (
                     ReadAcquire ( &stat_cpu.percent_load )
                                 )
                                 );

        stat_cpu.CurrentMhz =
          pwi [i-1].CurrentMhz;
        stat_cpu.MaximumMhz =
          pwi [i-1].MaxMhz;
      }

      cpu_records [0].addValue   (
          static_cast  <float>   (
                     ReadAcquire ( &cpu_stats.cpus [64].percent_load )
                                 )
                                 );

      last_update =
        SK::ControlPanel::current_time;


      const auto avg =
        __AverageEffectiveClock->getAvg ();

      if (avg != 0)
      {
        cpu_clocks.addValue (
          static_cast <float> (
            static_cast <double> (avg) / 1000000.0
          )
        );

        __AverageEffectiveClock->num_cores      = 0;
        __AverageEffectiveClock->cumulative_MHz = 0;
      }

      const bool changed =
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

  void draw (void) override
  {
    if (ImGui::GetFont () == nullptr) return;

    SK_TLS *pTLS =
          SK_TLS_Bottom ();

    const  float font_size    = ImGui::GetFont ()->FontSize;
    static char  szAvg [1024] = { };
    static bool  detailed     = true;
    static bool  show_parked  = true;
    static bool  show_graphs  = false;

    static int   last_parked_count = 0;

    static auto& cpu_stats =
      *SK_WMI_CPUStats;

    bool temporary_detailed = false;

    int EastWest =
        static_cast <int> (getDockingPoint()) &
      ( static_cast <int> (DockAnchor::East)  |
        static_cast <int> (DockAnchor::West)  );

    if ( EastWest == static_cast <int> (DockAnchor::None) )
    {
      const float center_x =
      ( getPos  ().x +
        getSize ().x * 0.5f );

      EastWest =
        ( center_x > ( ImGui::GetIO ().DisplaySize.x * 0.5f ) ) ?
                           static_cast <int> (DockAnchor::East) :
                           static_cast <int> (DockAnchor::West) ;
    }

    static float panel_width       = 0.0f;
    static float inset_width       = 0.0f;
    static float last_longest_line = 0.0f;
           float longest_line      = 0.0f;

    bool show_mode_buttons =
      (SK_ImGui_Visible);// || ImGui::IsWindowHovered ());

    bool show_install_button = false;// (!SK_WR0_Init ()) &&
                                  //SK_ImGui_Visible &&
            //ReadAcquire (&__SK_WR0_NoThreads) == 0;

    const auto _DrawButtonPanel =
      [&]( int NorthSouth ) ->
    void
    {
      if (show_mode_buttons || show_install_button)
      {
        ImGui::BeginGroup   ();
        {
          SK_ImGui::VerticalToggleButton     ( "All CPUs", &detailed  );

          if (detailed)
          {
            SK_ImGui::VerticalToggleButton   ( "Graphs", &show_graphs );

            if (last_parked_count > 0)
              SK_ImGui::VerticalToggleButton ( "Parked", &show_parked );
          }

          last_parked_count = 0;
        }
        ImGui::EndGroup     ();

        panel_width =
          ImGui::GetItemRectSize ().x;
        temporary_detailed =
          ImGui::IsItemHovered ();
      }

      UNREFERENCED_PARAMETER (NorthSouth);
    };

    ImGui::BeginGroup ();
    if (show_mode_buttons || show_install_button)
    {
      if (                                EastWest &
           static_cast <int> (DockAnchor::East) )
      {
        ImGui::BeginGroup  ();
        _DrawButtonPanel (  static_cast <int> (getDockingPoint()) &
                          ( static_cast <int> (DockAnchor::North) |
                            static_cast <int> (DockAnchor::South) ) );
        ImGui::EndGroup    ();
        ImGui::SameLine    ();
      }
    }
    ImGui::PushItemWidth (last_longest_line);
    ImGui::BeginGroup    ();
    {
      struct SK_CPUCore_PowerLog
      {
        LARGE_INTEGER last_tick;
        float         last_joules;
        float         fresh_value;
      };

      double dTemp =
        __SK_CPU.cores [0].temperature_C;// SK_CPU_UpdateCoreSensors (0)->temperature_C

      extern std::string_view
      SK_FormatTemperature (double in_temp, SK_UNITS in_unit, SK_UNITS out_unit, SK_TLS* pTLS);

      static std::string temp ("", 16);

      temp.assign (
        SK_FormatTemperature (
          dTemp,
            Celsius,
              config.system.prefer_fahrenheit ? Fahrenheit :
                                                Celsius, pTLS ).data ()
        );

      //static SK_CPUCore_PowerLog package_power;

  //SK_CPU_UpdatePackageSensors (0);

      for (unsigned int i = 0; i < cpu_records.size (); i++)
      {
        uint64_t parked_since = 0;

        if (i > 0)
        {
          parked_since =
            cpu_stats.cpus [i-1].parked_since.QuadPart;

          if (parked_since > 0) ++last_parked_count;

          if (! (detailed || temporary_detailed))
            continue;

          snprintf
                ( szAvg,
                    511,
                      "CPU%lu:\n\n"
                      "          min: %3.0f%%, max: %3.0f%%, avg: %3.0f%%\n",
                        i-1,
                          cpu_records [i].getMin (), cpu_records [i].getMax (),
                          cpu_records [i].getAvg () );
        }

        else
        {
          snprintf
                ( szAvg,
                    511,
                      "%s\t\t\n\n"
                      "          min: %3.0f%%, max: %3.0f%%, avg: %3.0f%%\n",
                        InstructionSet::Brand ().c_str (),
                        cpu_records [i].getMin (), cpu_records [i].getMax (),
                        cpu_records [i].getAvg () );
        }

        char szName [128] = { };

        snprintf (szName, 127, "##CPU_%u", i-1);

        const float samples =
          std::min ( (float)cpu_records [i].getUpdates  (),
                     (float)cpu_records [i].getCapacity () );

        if (i == 1)
          ImGui::Separator ();

        ImGui::PushStyleColor ( ImGuiCol_PlotLines,
                            (ImVec4&&)ImColor::HSV ( 0.31f - 0.31f *
                     std::min ( 1.0f, cpu_records [i].getAvg () / 100.0f ),
                                             0.86f,
                                               0.95f ) );

        if (parked_since == 0 || show_parked)
        {
          if (i == 0 || (show_graphs && parked_since == 0))
          {
            ImGui::PlotLinesC ( szName,
                                 cpu_records [i].getValues     ().data (),
                static_cast <int> (samples),
                                     cpu_records [i].getOffset (),
                                       szAvg,
                                         -1.0f,
                                           101.0f,
                                             ImVec2 (last_longest_line,
                                                 font_size * 4.0f
                                                    )
                              );
          }
        }
        ImGui::PopStyleColor ();

        bool found = false;

        const auto DrawHeader = [&](int j) -> bool
        {
          if (j == 0)
          {
            found = true;

            ImGui::PushStyleColor (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (0.28F, 1.f, 1.f, 1.f));
            ImGui::Text           ("%4.2f GHz", cpu_clocks.getAvg () / 1000.0f);
            ImGui::PopStyleColor  (1);

            if (dTemp != 0.0)
            {
              ImGui::SameLine        ();
              ImGui::PushStyleColor  (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (0.67194F, 0.15f, 0.95f, 1.f));
              ImGui::TextUnformatted ((const char *)u8"ー");
              ImGui::PushStyleColor  (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (0.3f - (0.3f * std::min (1.0f, ((static_cast <float> (dTemp) / 2.0f) / 100.0f))), 1.f, 1.f, 1.f));
              ImGui::SameLine        ();
              ImGui::TextUnformatted (temp.c_str ());
              ImGui::PopStyleColor   (2);
            }

            //double J =
            //  SK_CPU_GetJoulesConsumedTotal (0);

            if (__SK_CPU.pkg_sensor.power_W > 1.0)
            {
              ImGui::SameLine        ();
              ImGui::PushStyleColor  (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (0.67194F, 0.15f, 0.95f, 1.f));
              ImGui::TextUnformatted ((const char *)u8"ー");
              ImGui::SameLine        ();
              ImGui::PushStyleColor  (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (0.13294F, 0.734f, .94f, 1.f));
              ImGui::Text            ("%05.2f W", __SK_CPU.pkg_sensor.power_W );
              ImGui::PopStyleColor   (2);
            }

            if ( ReadAcquire (&active_scheme.dirty) == 0 )
            {
              static std::vector <power_scheme_s> schemes;

              ImGui::SameLine        ( ); ImGui::Spacing (); ImGui::SameLine ();
              ImGui::PushStyleColor  (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (0.3f - (0.3f * (cpu_records [0].getAvg () / 100.0f)), 0.80f, 0.95f, 1.f));
              ImGui::TextUnformatted ((const char *)u8"〇");
              ImGui::SameLine        ( ); ImGui::Spacing (); ImGui::SameLine ();
              ImGui::PushStyleColor  (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (0.14583f, 0.98f, .97f, 1.f));

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

              ImGui::PopStyleColor   (2);

              if ( ImGui::BeginPopupModal ( "Power Scheme Selector",
                   nullptr,
                   ImGuiWindowFlags_AlwaysAutoResize |
                   ImGuiWindowFlags_NoScrollbar      | ImGuiWindowFlags_NoScrollWithMouse )
                 )
              {
                bool end_dialog = false;

                ImGui::FocusWindow (ImGui::GetCurrentWindow ());

                for (auto& it : schemes)
                {
                  bool selected =
                    InlineIsEqualGUID ( it.uid, active_scheme.uid );

                  ImGui::PushStyleColor  (ImGuiCol_Text, ImVec4 (1.f, 1.f, 1.f, 1.f));
                  if (ImGui::Selectable  (it.utf8_name, &selected))
                  {
                    PowerSetActiveScheme (nullptr, &it.uid);

                    config.cpu.power_scheme_guid =  it.uid;

                    signalDeviceChange ();

                    end_dialog = true;
                  }

                  ImGui::PushStyleColor  (ImGuiCol_Text, ImVec4 (0.73f, 0.73f, 0.73f, 1.f));
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

            const auto& core_sensors =
              __SK_CPU.cores [j - 1];
              //SK_CPU_UpdateCoreSensors (j - 1);

            ImGui::PushStyleColor (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (0.28F, 1.f, 1.f, 1.f));

            if (parked_since != 0)
              ImGui::PushStyleColor (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (0.28F, 0.5f, 0.5f, 1.f));
            else
              ImGui::PushStyleColor (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (0.28F, 1.f, 1.f, 1.f));

            if (core_sensors.clock_MHz == 0.0)
            {
              if (parked_since == 0 || show_parked)
                ImGui::Text       ("%4.2f GHz", static_cast <float> (cpu_stats.cpus [j-1].CurrentMhz) / 1000.0f);
            }

            else
            {
              __AverageEffectiveClock->cumulative_MHz += core_sensors.clock_MHz;
              __AverageEffectiveClock->num_cores++;

              if (parked_since == 0 || show_parked)
                ImGui::Text       ("%4.2f GHz", core_sensors.clock_MHz / 1e+9f);
            }

            if (core_sensors.temperature_C != 0.0)
            {
              static std::string core_temp ("", 16);

              //if (! SK_CPU_IsZen ())
              //{
                core_temp.assign (
                  SK_FormatTemperature (
                    core_sensors.temperature_C,
                      Celsius,
                        config.system.prefer_fahrenheit ? Fahrenheit :
                                                          Celsius, pTLS ).data ()
                );
              //}

              //else
              //{
              //  core_temp.assign ((const char *)u8"………");
              //}

              if (parked_since == 0 || show_parked)
              {
                ImGui::SameLine        ();
                ImGui::PushStyleColor  (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (0.67194F, 0.15f, 0.95f, 1.f));
                ImGui::TextUnformatted ((const char *)u8"ー");
                if (SK_CPU_IsZen ())
                  ImGui::PushStyleColor  (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (
                    static_cast <float> (0.3 - (0.3 * std::min (1.0, ((core_sensors.temperature_C / 2.0) / 100.0)))),
                                         0.725f, 0.725f, 1.f));
                else
                  ImGui::PushStyleColor  (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (
                    static_cast <float> (0.3 - (0.3 * std::min (1.0, ((core_sensors.temperature_C / 2.0) / 100.0)))),
                                         1.f, 1.f, 1.f));
                ImGui::SameLine        ();
                ImGui::TextUnformatted (core_temp.c_str ());
                ImGui::PopStyleColor   (2);
              }
            }

            ImGui::PopStyleColor (2);

            if ( core_sensors.power_W > 0.005 && (parked_since == 0 || show_parked) )
            {
              ImGui::SameLine        (      );
              ImGui::PushStyleColor  (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (0.67194F, 0.15f, 0.95f, 1.f));
              ImGui::TextUnformatted ((const char *)u8"ー");
              ImGui::SameLine        (      );
              ImGui::PushStyleColor  (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (0.13294F, 0.734f, .94f, 1.f));
              ImGui::Text            ("%05.2f W", core_sensors.power_W );
              ImGui::PopStyleColor   (2);
            }

            if (parked_since == 0 || show_parked)
            {
              ImGui::SameLine        ( ); ImGui::Spacing ( ); ImGui::SameLine ();

              if (parked_since == 0)
                ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4&&)ImColor::HSV (0.3f - (0.3f * (cpu_records [j].getAvg () / 100.0f)), 0.80f, 0.95f, 1.f));
              else
                ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4&&)ImColor::HSV (0.57194F,                                             0.5f,  0.5f, 1.f));
              ImGui::TextUnformatted ((const char *)u8"〇");
              ImGui::SameLine        ( ); ImGui::Spacing ( ); ImGui::SameLine ();
              ImGui::Text            ("%02.0f%%", cpu_records [j].getAvg ());
              ImGui::PopStyleColor   ( );

              if (parked_since > 0)
              {
                ImGui::SameLine        (      );
                ImGui::PushStyleColor  (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (0.67194F, 0.15f, 0.95f, 1.f));
                ImGui::TextUnformatted ((const char *)u8"ー");
                ImGui::SameLine        (      );
                ImGui::PushStyleColor  (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (0.57194F, 0.534f, .94f, 1.f));
                ImGui::Text            ( "Parked %5.1f Secs",
                                          SK_DeltaPerfMS ( parked_since, 1 ) / 1000.0
                                       );
                ImGui::PopStyleColor   (2);
              }
            }

            return true;
          }

          return false;
        };

        ImGui::BeginGroup ( );
               DrawHeader (i);
        ImGui::EndGroup   ( );

        longest_line =
          std::max (longest_line, ImGui::GetItemRectSize ().x);
      }
    }
    ImGui::EndGroup      ();
    ImGui::PopItemWidth  ();
                          last_longest_line =
                               std::max (longest_line, 450.0f);

    if ( (show_mode_buttons || show_install_button) &&
         (EastWest & static_cast <int> (DockAnchor::West)) )
    {
      ImGui::SameLine    ();
      ImGui::BeginGroup  ();
      _DrawButtonPanel (  static_cast <int> (getDockingPoint()) &
                        ( static_cast <int> (DockAnchor::North) |
                          static_cast <int> (DockAnchor::South) ) );
      ImGui::EndGroup    ();
    }
    ImGui::EndGroup      ();

    show_mode_buttons =
      true;// (SK_ImGui_Visible || ImGui::IsWindowHovered ());
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

  void signalDeviceChange (void)
  {
    InterlockedIncrement (&active_scheme.dirty);
  }

protected:
  const DWORD update_freq = sk::narrow_cast <DWORD> (UPDATE_INTERVAL_MS);

private:
  struct active_scheme_s : power_scheme_s {
    HPOWERNOTIFY  notify = INVALID_HANDLE_VALUE;
    volatile LONG dirty  = FALSE;
  } active_scheme;

  DWORD       last_update = 0UL;

  std::vector <SK_Stat_DataHistory <float, 64>> cpu_records = { };
               SK_Stat_DataHistory <float,  3>  cpu_clocks  = { };
};

SKWG_CPU_Monitor* SK_Widget_GetCPU (void)
{
  static SKWG_CPU_Monitor  cpu_mon;
                   return &cpu_mon;
}

ULONG
CALLBACK
SK_CPU_DeviceNotifyCallback (
  PVOID Context,
  ULONG Type,
  PVOID Setting )
{
  UNREFERENCED_PARAMETER (Context);
  UNREFERENCED_PARAMETER (Type);
  UNREFERENCED_PARAMETER (Setting);

  SK_Widget_GetCPU ()->signalDeviceChange ();

  return 1;
}