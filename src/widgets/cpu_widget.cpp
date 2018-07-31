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

#include <SpecialK/diagnostics/load_library.h>
#include <SpecialK/diagnostics/modules.h>

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

#define VENDOR_INTEL "GenuineIntel"
#define VENDOR_AMD	 "AuthenticAMD"

#define CRC_INTEL	   0x75a2ba39
#define CRC_AMD 	   0x3485bbd3

struct SK_AMDZenRegistry_s {
  const char  *brandId;
  unsigned int Boost, XFR;
  unsigned int tempOffset;	/* Source: Linux/k10temp.c */
} static  _SK_KnownZen [] = {
  { VENDOR_AMD,            0,  0,   0 },
  { "AMD Ryzen 3 1200",   +3, +1,   0 },
  { "AMD Ryzen 3 1300X",  +2, +2,   0 },
  { "AMD Ryzen 3 2200G",  +2,  0,   0 },
  { "AMD Ryzen 5 1400",   +2, +1,   0 },
  { "AMD Ryzen 5 2400G",  +3,  0,   0 },
  { "AMD Ryzen 5 1500X",  +2, +2,   0 },
  { "AMD Ryzen 5 2500U", +16,  0,   0 },
  { "AMD Ryzen 5 1600X",  +4, +1, +20 },
  { "AMD Ryzen 5 1600",   +4, +1,   0 },
  { "AMD Ryzen 5 2600X",  +5, +2, +10 },
  { "AMD Ryzen 5 2600",   +3, +2,   0 },
  { "AMD Ryzen 7 1700X",  +4, +1, +20 },
  { "AMD Ryzen 7 1700",   +7, +1,   0 },
  { "AMD Ryzen 7 1800X",  +4, +1, +20 },
  { "AMD Ryzen 7 2700X",  +5, +2, +10 },
  { "AMD Ryzen 7 2700U", +16,  0,   0 },
  { "AMD Ryzen 7 2700",   +8, +2,   0 },

  { "AMD Ryzen Threadripper 1950X", +6, +2, +27 },
  { "AMD Ryzen Threadripper 1920X", +5, +2, +27 },
  { "AMD Ryzen Threadripper 1900X", +2, +2, +27 },
  { "AMD Ryzen Threadripper 1950" ,  0,  0, +10 },
  { "AMD Ryzen Threadripper 1920" , +6,  0, +10 },
  { "AMD Ryzen Threadripper 1900" , +6,  0, +10 }
};

static int __SK_ZenIdx = 0;

typedef struct
{
  union {
    unsigned int val;
    unsigned int
      Reserved1	       :  1-0,
      SensorTrip	     :  2-1,  // 1 if temp. sensor trip occurs & was enabled
      SensorCoreSelect :  3-2,  // 0b: CPU1 Therm Sensor. 1b: CPU0 Therm Sensor
      Sensor0Trip	     :  4-3,  // 1 if trip @ CPU0 (single), or @ CPU1 (dual)
      Sensor1Trip	     :  5-4,  // 1 if sensor trip occurs @ CPU0 (dual core)
      SensorTripEnable :  6-5,  // a THERMTRIP High event causes a PLL shutdown
      SelectSensorCPU	 :  7-6,  // 0b: CPU[0,1] Sensor 0. 1b: CPU[0,1] Sensor 1
      Reserved2	       :  8-7,
      DiodeOffset	     : 14-8,  // offset should be added to the external temp.
      Reserved3	       : 16-14,
      CurrentTemp	     : 24-16, // 00h = -49C , 01h = -48C ... ffh = 206C 
      TjOffset	       : 29-24, // Tcontrol = CurTmp - TjOffset * 2 - 49
      Reserved4	       : 31-29,
      SwThermTrip	     : 32-31; // diagnostic bit, for testing purposes only.
  };
} THERMTRIP_STATUS;

#include "../depends//include/WinRing0/OlsDef.h"
#include "../depends//include/WinRing0/OlsApi.h"

InitializeOls_pfn       InitializeOls       = nullptr;
ReadPciConfigDword_pfn  ReadPciConfigDword  = nullptr;
WritePciConfigDword_pfn WritePciConfigDword = nullptr;
RdmsrTx_pfn             RdmsrTx             = nullptr;

bool
SK_CPU_IsZen (void)
{
  static int is_zen = -1;

  if (is_zen == -1)
  {
    is_zen = 0;

    static std::wstring path_to_driver =
      SK_FormatStringW ( LR"(%ws\Drivers\WinRing0\%s)",
        std::wstring ( SK_GetDocumentsDir () + LR"(\My Mods\SpecialK)" ).c_str (),
          SK_RunLHIfBitness (64, L"WinRing0x64.dll", L"WinRing0.dll")
    );

    static bool has_WinRing0 =
      GetFileAttributesW (path_to_driver.c_str ()) != INVALID_FILE_ATTRIBUTES;

    if (has_WinRing0)
    {
      SK_Modules.LoadLibraryLL (path_to_driver.c_str ());

      InitializeOls =
        (InitializeOls_pfn)SK_GetProcAddress       ( path_to_driver.c_str (),
        "InitializeOls" );

      ReadPciConfigDword =
        (ReadPciConfigDword_pfn)SK_GetProcAddress  ( path_to_driver.c_str (),
        "ReadPciConfigDword" );

      WritePciConfigDword =
        (WritePciConfigDword_pfn)SK_GetProcAddress ( path_to_driver.c_str (),
        "WritePciConfigDword" );

      RdmsrTx =
        (RdmsrTx_pfn)SK_GetProcAddress ( path_to_driver.c_str (),
        "RdmsrTx" );

      if (InitializeOls ())
      {
        int i = 0;

        for ( auto zen : _SK_KnownZen )
        {
          if ( strstr ( InstructionSet::Brand ().c_str (),
                          zen.brandId )
             )
          {
            __SK_ZenIdx = i;
            is_zen      = 1;
            break;
          }

          i++;
        }
      }
    }
  }

  return ( is_zen == 1 );
}

typedef struct
{
  union {
    struct {
      unsigned int edx;
      unsigned int eax;
    };
    unsigned long long
      PU_PowerUnits    :   4 - 0,
      Reserved2        :   8 - 4,
      ESU_EnergyUnits  :  13 - 8,
      Reserved1        :  16 - 13,
      TU_TimeUnits	   :  20 - 16,
      Reserved0	       :  64 - 20;
  };
} RAPL_POWER_UNIT;

float
SK_CPU_GetJoulesConsumedTotal (DWORD_PTR package)
{
  if (SK_CPU_IsZen ())
  {
    DWORD_PTR package_mask = package;//(1ULL << package);
    DWORD     eax,  edx;
    DWORD     eax2, edx2;

    // AMD Model 17h measures this in 15.3 micro-joule increments
    if (RdmsrTx (0xC001029B, &eax, &edx, package_mask))
    {
      RdmsrTx (0xC0010299, &eax2, &edx2, package_mask);

      if (((eax2 >> 8UL) & 0x1FUL) != 16UL)
      {
        SK_RunOnce (
          dll_log.Log (
            L"Unexpected Energy Units for Model 17H: %lu",
              (eax2 >> 8UL) & 0x1FUL
          )
        );
      }

      return
        static_cast <float> (
          static_cast <long double> (eax) / 65359.4771 );
    }
  }

  return 0.0f;
}

float
SK_CPU_GetJoulesConsumed (int64_t core)
{
  if (SK_CPU_IsZen ())
  {
    DWORD_PTR thread_mask = (1ULL << core);
    DWORD     eax,  edx,
              eax2, edx2;

    // AMD Model 17h measures this in 15.3 micro-joule increments
    if (RdmsrTx (0xC001029A, &eax, &edx, thread_mask))
    {
      RdmsrTx (0xC0010299, &eax2, &edx2, thread_mask);

      if (((eax2 >> 8UL) & 0x1FUL) != 16UL)
      {
        SK_RunOnce (
          dll_log.Log (
            L"Unexpected Energy Units for Model 17H: %lu",
              (eax2 >> 8UL) & 0x1FUL
          )
        );
      }

      return
        static_cast <float> (
          static_cast <long double> (eax) / 65359.4771 );
    }
  }

  return 0.0f;
}

float
SK_CPU_GetTemperature_AMDZen (int core)
{
  UNREFERENCED_PARAMETER (core);

  static double temp_offset = -1.0;

  if (! SK_CPU_IsZen ())
  {
    return 0.0f;
  }

  else if (temp_offset == -1.0)
  {
    temp_offset = 0.0;

    for ( auto zen : _SK_KnownZen )
    {
      if ( strstr ( InstructionSet::Brand ().c_str (),
                      zen.brandId )
         )
      {
        temp_offset =
          static_cast <double> (zen.tempOffset);

        break;
      }
    }
  }

#define PCI_CONFIG_ADDR(bus, dev, fn) \
	( 0x80000000 | ((bus) << 16) |      \
                 ((dev) << 11) |      \
                 ( (fn) << 8)   )

  const unsigned int indexRegister = 0x00059800;
        unsigned int sensor        = 0;

  WritePciConfigDword  (PCI_CONFIG_ADDR (0, 0, 0), 0x60, indexRegister);
  sensor =
    ReadPciConfigDword (PCI_CONFIG_ADDR (0, 0, 0), 0x64);

  //
  // Zen (17H) has two different ways of reporting temperature;
  //
  //   ==> It is critical to first check what the CPU defines
  //         the measurement's 11-bit range to be !!!
  //
  //     ( So many people are glossing the hell over this )
  //
  const bool usingNeg49To206C =
    ((sensor >> 19) & 0x1);

  auto invScale11BitDbl =
    [ ] (uint32_t in) -> const double
     {
       return (1.0 / 2047.0) *
                static_cast <double> ((in >> 21UL) & 0x7FFUL);
     };

  return
    static_cast <float> ( temp_offset +
        ( usingNeg49To206C ?
            ( invScale11BitDbl (sensor) * 206.0 ) - 49.0 :
              invScale11BitDbl (sensor) * 255.0            )
    );
}

struct {
  double clocks;
  int    sources;

  uint64_t getAvg (void) {
    return sources > 0 ?
      static_cast <uint64_t> ( clocks / static_cast <double> (sources) ) :
                     0;
  }
} __SK_Ryzen_AverageEffectiveClock;

#define AMD_ZEN_MSR_MPERF       0x000000E7
#define AMD_ZEN_MSR_APERF       0x000000E8

#define AMD_ZEN_MSR_PSTATE_STATUS 0xC0010293
#define AMD_ZEN_MSR_PSTATESTAT    0xC0010063
#define AMD_ZEN_MSR_PSTATEDEF_0   0xC0010064
#define AMD_ZEN_PSTATE_COUNT      8

struct SK_CPU_AMD_Zen_PState
{
  float FID = 0.0f;
  float DID = 0.0f;
} static __SK_Zen_PStates [AMD_ZEN_PSTATE_COUNT] = { };

struct SK_CPU_CoreSensors
{
  uint32_t cpu_core;
   int64_t sample_taken;

  double   temperature_C;
  double   clock_MHz;
  double   power_W;

  struct accumulator_s {
    double joules;
  } accum;
};

void
SK_CPU_Zen_PopulatePStates (void)
{
  if (__SK_Zen_PStates [0].FID != 0.0f)
    return;

  for ( int i = 0 ; i < AMD_ZEN_PSTATE_COUNT ; ++i )
  {
    DWORD_PTR thread_mask = 1;
    DWORD     eax,  edx;

    if (RdmsrTx (AMD_ZEN_MSR_PSTATEDEF_0 + i, &eax, &edx, thread_mask))
    {
      // [0x10 - 0xFF] => (x25) == < 400 - 6375 >
      uint8_t _FID = (eax & 0xFF);

      if (_FID < 0x10)
      {
        // PStates 3-7 are usually undefined, so this is okay...
        if (_FID != 0x0)
          dll_log.Log (L"[  AMDZen  ] Invalid FID (%x) Encountered on CPU - {PState%lu}", _FID, i);
      }
      
      else
      {
        __SK_Zen_PStates [i].FID =
          static_cast <float> (_FID) * (25.0f);
      }

      uint8_t _DID = (eax >> 8UL) & 0x3F;

      switch (_DID)
      {
        case 0:
          __SK_Zen_PStates [i].DID = 1.0f;// 0.0f; ???
          break;
        case 0x8:
          __SK_Zen_PStates [i].DID = 1.0f;
          break;
        case 0x9:
          __SK_Zen_PStates [i].DID = 1.125f;
          break;
        case 0x0A:
        case 0x0B:
        case 0x0C:
        case 0x0D:
        case 0x0E:
        case 0x0F:
        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13:
        case 0x14:
        case 0x15:
        case 0x16:
        case 0x17:
        case 0x18:
        case 0x1A:
        case 0x1C:
        case 0x1E:
        case 0x20:
        case 0x22:
        case 0x24:
        case 0x25:
        case 0x28:
        case 0x2A:
        case 0x2C:
          __SK_Zen_PStates [i].DID =
            static_cast <float> (_DID) / 8.0f;
          break;
        default:
          dll_log.Log (L"[  AMDZen  ] Invalid DID (%x) Encountered on CPU - {PState%lu}", _DID, i);
          break;
      }
    }
  }
}

SK_CPU_CoreSensors&
SK_CPU_UpdateCoreSensors (uint32_t core)
{
  static
    std::unordered_map <uint32_t, SK_CPU_CoreSensors> cores;

  if (SK_CPU_IsZen ())
  {
    if (__SK_Zen_PStates [0].FID == 0.0f)
    {
      SK_CPU_Zen_PopulatePStates ();
    }

    double time_elapsed_ms =
      SK_DeltaPerfMS (cores [core].sample_taken, 1.0);

    if (time_elapsed_ms > 666.666667)
    {
      DWORD_PTR thread_mask = (1ULL << core);
      DWORD     eax,  edx;

      //RdmsrTx (AMD_ZEN_MSR_PSTATESTAT, &eax, &edx, thread_mask);
      //
      //int PStateIdx = (eax & 0x7);
      //
      //P0Freq =
      //  (uint64_t)((__SK_Zen_PStates [PStateIdx].FID  /
      //              __SK_Zen_PStates [PStateIdx].DID) /** 200.0f*/) * 1000000ULL;

      RdmsrTx (AMD_ZEN_MSR_PSTATE_STATUS, &eax, &edx, thread_mask);

      int Did = (int)((eax >> 8) & 0x3F);
      int Fid = (int)( eax       & 0xFF);

      cores [core].clock_MHz =
        ( static_cast <double> (Fid) /
          static_cast <double> (Did)  ) * 200.0 * 1000000.0;

      float J =
        SK_CPU_GetJoulesConsumed (core);

      double joules_used  =
        (J - cores [core].accum.joules);

      cores [core].accum.joules = J;
      cores [core].power_W      =
        ( joules_used / (time_elapsed_ms / 1000.0) );

      cores [core].temperature_C =
        SK_CPU_GetTemperature_AMDZen (core);

      cores [core].sample_taken =
        SK_QueryPerf ().QuadPart;
    }

    return cores [core];
  }

  static SK_CPU_CoreSensors nil {
     0UL, 0LL, 0.0, 0.0, 0.0
  };

  return nil;
}

int8_t
SK_CPU_GetPState_AMDZen (uint64_t core)
{
  if (SK_CPU_IsZen ())
  {
    DWORD_PTR thread_mask = (1ULL << core);
    DWORD     eax, edx;

    if (RdmsrTx (0xC0010063, &eax, &edx, thread_mask))
    {
      return
        (eax & 0x7);
    }
  }

  return -1;
}

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

      if (__SK_Ryzen_AverageEffectiveClock.getAvg () == 0)
      {
        cpu_clocks.addValue ( avg_clock /
                                static_cast <float> (sinfo.dwNumberOfProcessors) );
      }

      else
      {
        cpu_clocks.addValue ( __SK_Ryzen_AverageEffectiveClock.getAvg () / 1000000.0f );
                              __SK_Ryzen_AverageEffectiveClock.sources = 0;
                              __SK_Ryzen_AverageEffectiveClock.clocks  = 0;
      }

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
    static bool  detailed    = false;
    static bool  hide_parked = true;
    
    ImGui::BeginGroup ();

    SK_ImGui::VerticalToggleButton ( detailed ? "Show All CPUs" :
                                                "Simplified" , &detailed);

    if (detailed)
    {
      SK_ImGui::VerticalToggleButton ( hide_parked ? "Hide Parked" :
                                                     "Show Parked" , &hide_parked);
    }

    ImGui::EndGroup ();

    bool temporary_detailed =
      ImGui::IsItemHovered ();

    ImGui::SameLine   ();
    ImGui::BeginGroup ();

    struct SK_CPUCore_PowerLog
    {
      LARGE_INTEGER last_tick;
      float         last_joules;
      float         fresh_value;
    };

    float fTemp =
      static_cast <float> (
        SK_CPU_UpdateCoreSensors (0).temperature_C
      );

    extern std::string
    SK_FormatTemperature (double in_temp, SK_UNITS in_unit, SK_UNITS out_unit);

    static std::string temp;

    temp.assign (
      SK_FormatTemperature (
        fTemp,
          Celsius,
            config.system.prefer_fahrenheit ? Fahrenheit :
                                              Celsius )
    );

    static SK_CPUCore_PowerLog package_power;

    float fJoules =
      SK_CPU_GetJoulesConsumedTotal (1);

    if (fJoules != 0.0f)
    {
      static bool first = true;
      if (first)
      {
        package_power = {
          SK_QueryPerf (), fJoules,
          0.0f
        };

        first = false;
      }

      else
      {
        double time_elapsed_ms =
          SK_DeltaPerfMS (package_power.last_tick.QuadPart, 1.0);

        if (time_elapsed_ms > 666.666667)
        {
          float joules_used =
            (fJoules - package_power.last_joules);

          package_power = {
            SK_QueryPerf (), fJoules,
            joules_used / ( (float)time_elapsed_ms / 1000.0f )
          };
        }
      }
    }

    for (unsigned int i = 0; i < cpu_records.size (); i++)
    {
      if (i > 0)
      {
        if (! (detailed || temporary_detailed))
          break;

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

      uint64_t parked_since = 0;

      if (i > 0)
        parked_since =
          cpu_stats.cpus [i-1].parked_since.QuadPart;

      ImGui::PushStyleColor ( ImGuiCol_PlotLines, 
                          ImColor::HSV ( 0.31f - 0.31f *
                   std::min ( 1.0f, cpu_records [i].getAvg () / 100.0f ),
                                           0.86f,
                                             0.95f ) );

    if (parked_since == 0 || (! hide_parked))
    {
      ImGui::PlotLinesC ( szName,
                           cpu_records [i].getValues     ().data (),
          static_cast <int> (samples),
                               cpu_records [i].getOffset (),
                                 szAvg,
                                   -1.0f,
                                     101.0f,
                                       ImVec2 (
                                         std::max (500.0f, ImGui::GetContentRegionAvailWidth ()), font_size * 4.0f) );
    }

      bool found = false;
  
      auto DrawHeader = [&](int j) -> bool
      {
        if (j == 0)
        { 
          found = true;

          ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.28F, 1.f, 1.f, 1.f));
          ImGui::Text           ("%#4.2f GHz", cpu_clocks.getAvg () / 1000.0f);
          ImGui::PopStyleColor  (1);

          if (fTemp != 0.0f)
          {
            ImGui::SameLine        ();
            ImGui::PushStyleColor  (ImGuiCol_Text, ImColor::HSV (0.67194F, 0.15f, 0.95f, 1.f));
            ImGui::TextUnformatted (u8"ー");
            ImGui::PushStyleColor  (ImGuiCol_Text, ImColor::HSV (0.3f - (0.3f * std::min (1.0f, ((fTemp / 2.0f) / 100.0f))), 1.f, 1.f, 1.f));
            ImGui::SameLine        ();
            ImGui::Text            ("%s", temp.c_str ());
            ImGui::PopStyleColor   (2);
          }

          if (fJoules != 0.0f)
          {
            ImGui::SameLine        ();
            ImGui::PushStyleColor  (ImGuiCol_Text, ImColor::HSV (0.67194F, 0.15f, 0.95f, 1.f));
            ImGui::TextUnformatted (u8"ー");
            ImGui::SameLine        ();
            ImGui::PushStyleColor  (ImGuiCol_Text, ImColor::HSV (0.13294F, 0.734f, .94f, 1.f));
            ImGui::Text            ("%4.1f W", package_power.fresh_value );
            ImGui::PopStyleColor   (2);
          }

          if ( ReadAcquire (&active_scheme.dirty) == 0 )
          {
            static std::vector <power_scheme_s> schemes;

            ImGui::SameLine        ( ); ImGui::Spacing (); ImGui::SameLine ();
            ImGui::PushStyleColor  (ImGuiCol_Text, ImColor::HSV (0.3f - (0.3f * (cpu_records [0].getAvg () / 100.0f)), 0.80f, 0.95f, 1.f));
            ImGui::TextUnformatted (u8"〇");
            ImGui::SameLine        ( ); ImGui::Spacing (); ImGui::SameLine ();
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

            ImGui::PopStyleColor   (2);

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

          auto core_sensors =
            SK_CPU_UpdateCoreSensors (j - 1);

      if (parked_since == 0 || (! hide_parked))
      {
          ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.28F, 1.f, 1.f, 1.f));
          ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.28F, 1.f, 1.f, 1.f));

          if (core_sensors.clock_MHz == 0.0)
            ImGui::Text         ("%#4.2f GHz", static_cast <float> (cpu_stats.cpus [j-1].CurrentMhz) / 1000.0f);
          else
          {
            __SK_Ryzen_AverageEffectiveClock.clocks += core_sensors.clock_MHz;
            __SK_Ryzen_AverageEffectiveClock.sources++;
            ImGui::Text         ("%#4.2f GHz", core_sensors.clock_MHz / 1e+9f);
          }

          // Not exactly per-core right now, so don't
          //   go crazy with formatting...
          if (core_sensors.temperature_C != 0.0)
          {
            //extern std::string
            //SK_FormatTemperature (double in_temp, SK_UNITS in_unit, SK_UNITS out_unit);
            //
            //static std::string temp;
            //
            //temp.assign (
            //  SK_FormatTemperature (
            //    core_sensors.temperature_C,
            //      Celsius,
            //        config.system.prefer_fahrenheit ? Fahrenheit :
            //                                          Celsius )
            //);

            ImGui::SameLine        ();
            ImGui::PushStyleColor  (ImGuiCol_Text, ImColor::HSV (0.67194F, 0.15f, 0.95f, 1.f));
            ImGui::TextUnformatted (u8"ー");
            ImGui::PushStyleColor  (ImGuiCol_Text, ImColor::HSV (
              static_cast <float> (0.3 - (0.3 * std::min (1.0, ((core_sensors.temperature_C / 2.0) / 100.0)))),
                                   1.f, 1.f, 1.f));
            ImGui::SameLine        ();
            ImGui::TextUnformatted (temp.c_str ());
            ImGui::PopStyleColor   (2);
          }
        }

        if (core_sensors.power_W != 0.0 && (parked_since == 0 || (! hide_parked)))
        {
          ImGui::SameLine        ();
          ImGui::PushStyleColor  (ImGuiCol_Text, ImColor::HSV (0.67194F, 0.15f, 0.95f, 1.f));
          ImGui::TextUnformatted (u8"ー");
          ImGui::SameLine        ();
          ImGui::PushStyleColor  (ImGuiCol_Text, ImColor::HSV (0.13294F, 0.734f, .94f, 1.f));
          ImGui::Text            ("%#5.2f W", core_sensors.power_W );
          ImGui::PopStyleColor   (2);
        }

        if (parked_since == 0 || (! hide_parked))
        {
          ImGui::SameLine        ( ); ImGui::Spacing ( ); ImGui::SameLine ();
          if (parked_since == 0)
            ImGui::PushStyleColor(ImGuiCol_Text, ImColor::HSV (0.3f - (0.3f * (cpu_records [j].getAvg () / 100.0f)), 0.80f, 0.95f, 1.f));
          else
            ImGui::PushStyleColor(ImGuiCol_Text, ImColor::HSV (0.57194F,                                             0.534f, .94f, 1.f));
          ImGui::TextUnformatted (u8"〇");
          ImGui::SameLine        ( ); ImGui::Spacing ( );
          ImGui::PopStyleColor   ( );

          if (parked_since > 0)
          {
            ImGui::SameLine (); ImGui::Spacing ();
            ImGui::SameLine (); ImGui::Spacing (); ImGui::SameLine ();

            ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.57194F, 0.534f, .94f, 1.f));
            ImGui::Text           ( "Parked For %4.1f Seconds",
                                      SK_DeltaPerfMS ( parked_since, 1 ) / 1000.0
                                  );
          }

          ImGui::PopStyleColor (2);
        }

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