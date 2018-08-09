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

#define PCI_CONFIG_ADDR(bus, dev, fn) \
	( 0x80000000 | ((bus) << 16) |      \
                 ((dev) << 11) |      \
                 ( (fn) << 8)   )

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

#define AMD_ZEN_MSR_MPERF       0x000000E7
#define AMD_ZEN_MSR_APERF       0x000000E8

#define AMD_ZEN_MSR_PSTATE_STATUS 0xC0010293
#define AMD_ZEN_MSR_PSTATESTAT    0xC0010063
#define AMD_ZEN_MSR_PSTATEDEF_0   0xC0010064
#define AMD_ZEN_PSTATE_COUNT      8

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

#include <SpecialK/resource.h>

#include "../depends//include/WinRing0/OlsDef.h"
#include "../depends//include/WinRing0/OlsApi.h"

InitializeOls_pfn       InitializeOls       = nullptr;
ReadPciConfigDword_pfn  ReadPciConfigDword  = nullptr;
WritePciConfigDword_pfn WritePciConfigDword = nullptr;
RdmsrTx_pfn             RdmsrTx             = nullptr;
Rdmsr_pfn               Rdmsr               = nullptr;

void
SK_WinRing0_Unpack (void)
{
  HMODULE hModSelf = 
    SK_GetDLL ();

  HRSRC res =
    FindResource ( hModSelf, MAKEINTRESOURCE (IDR_WINRING0_PACKAGE), L"7ZIP" );

  if (res)
  {
    SK_LOG0 ( ( L"Unpacking WinRing0 Driver because user does not have it in the proper location." ),
                L" WinRing0 " );

    DWORD   res_size     =
      SizeofResource ( hModSelf, res );

    HGLOBAL packed_winring =
      LoadResource   ( hModSelf, res );

    if (! packed_winring) return;


    const void* const locked =
      (void *)LockResource (packed_winring);


    if (locked != nullptr)
    {
      wchar_t      wszArchive     [MAX_PATH * 2 + 1] = { };
      wchar_t      wszDestination [MAX_PATH * 2 + 1] = { };

      static std::wstring path_to_driver =
        SK_FormatStringW ( LR"(%ws\Drivers\WinRing0\)",
            std::wstring ( SK_GetDocumentsDir () + LR"(\My Mods\SpecialK)" ).c_str ()
                         );

      wcscpy (wszDestination, path_to_driver.c_str ());

      if (GetFileAttributesW (wszDestination) == INVALID_FILE_ATTRIBUTES)
        SK_CreateDirectories (wszDestination);

      wcscpy      (wszArchive, wszDestination);
      PathAppendW (wszArchive, L"WinRing0.7z");

      SK_LOG0 ( ( L" >> Archive: %s [Destination: %s]", wszArchive, wszDestination ),
                  L" WinRing0 " );

      FILE* fPackedDriver =
        _wfopen   (wszArchive, L"wb");

      if (fPackedDriver != nullptr)
      {
        fwrite      (locked, 1, res_size, fPackedDriver);
        fclose      (fPackedDriver);
      }

      using SK_7Z_DECOMP_PROGRESS_PFN = int (__stdcall *)(int current, int total);

      extern
      HRESULT
      SK_Decompress7zEx ( const wchar_t*            wszArchive,
                          const wchar_t*            wszDestination,
                          SK_7Z_DECOMP_PROGRESS_PFN callback );

      SK_Decompress7zEx (wszArchive, wszDestination, nullptr);
      DeleteFileW       (wszArchive);
    }

    UnlockResource (packed_winring);
  }
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
  uint32_t cpu_core;
   int64_t sample_taken;

  double   temperature_C;
  double   clock_MHz;
  double   power_W;

  struct accumulator_s {
    double joules;
  } accum;

  double tjMax;
};

struct SK_CPU_Package
{
  SK_CPU_Package (void)
  {
    SYSTEM_INFO        sinfo = { };
    SK_GetSystemInfo (&sinfo);

    for ( DWORD i = 0 ; i < sinfo.dwNumberOfProcessors ; i++ )
    {
      cores [i].cpu_core = i;
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

    struct
    {
      double tsc = 1.0;
    } intel;
  } coefficients;

  struct {
    double temperature = 0.0;
  } offsets;

  SK_CPU_CoreSensors pkg_sensor;

  std::unordered_map <int, SK_CPU_CoreSensors> cores;
} static __SK_CPU;


double
SK_CPU_GetIntelTjMax (DWORD_PTR core)
{
  DWORD eax, edx;

  if ( RdmsrTx ( SK_CPU_IntelMSR::IA32_TEMPERATURE_TARGET,
                   &eax, &edx,
                     ((DWORD_PTR)1 << core)
               )
     )
  {
    return
      static_cast <double> ((eax >> 16UL) & 0xFFUL);
  }

  return 100.0;
}

bool
SK_WR0_Init (void)
{
  static int init = 0;

  if (init != 0)
    return (init == 1);

  static std::wstring path_to_driver =
    SK_FormatStringW ( LR"(%ws\Drivers\WinRing0\%s)",
                      std::wstring ( SK_GetDocumentsDir () + LR"(\My Mods\SpecialK)" ).c_str (),
                      SK_RunLHIfBitness (64, L"WinRing0x64.dll", L"WinRing0.dll")
    );

  static bool has_WinRing0 =
    GetFileAttributesW (path_to_driver.c_str ()) != INVALID_FILE_ATTRIBUTES;

  if (! has_WinRing0)
  {
    SK_WinRing0_Unpack ();

    has_WinRing0 =
      GetFileAttributesW (path_to_driver.c_str ()) != INVALID_FILE_ATTRIBUTES;
  }

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

    Rdmsr =
      (Rdmsr_pfn)SK_GetProcAddress ( path_to_driver.c_str (),
                                      "Rdmsr" );

    if (InitializeOls ()) init =  1;
    else                  init = -1;
  }

  SK_LOG0 ( (L"Installed CPU: %hs",
                  InstructionSet::Brand  ().c_str () ),
             L"CPU Driver" );
  SK_LOG0 ( (L" >> Family: %02xh, Model: %02xh, Stepping: %02xh",
                  InstructionSet::Family   (), InstructionSet::Model (),
                  InstructionSet::Stepping () ),
             L"CPU Driver" );

  return
    (init == 1);
}

bool SK_CPU_IsZen (void);

SK_CPU_IntelMicroarch
SK_CPU_GetIntelMicroarch (void)
{
  auto& cpu =
    __SK_CPU;

  if (cpu.intel_arch != SK_CPU_IntelMicroarch::KnownIntelArchs)
    return cpu.intel_arch;


  if (! SK_WR0_Init ())
    cpu.intel_arch = SK_CPU_IntelMicroarch::NotIntel;

  if (SK_CPU_IsZen ())
    cpu.intel_arch = SK_CPU_IntelMicroarch::NotIntel;


  DWORD eax, edx;

  if (cpu.intel_arch == SK_CPU_IntelMicroarch::KnownIntelArchs)
  {
    cpu.intel_arch = SK_CPU_IntelMicroarch::NotIntel;

    switch (InstructionSet::Family ())
    {
      case 0x06:
      {
        switch (InstructionSet::Model ())
        {
          case 0x0F: // Intel Core 2 (65nm)
            cpu.intel_arch = SK_CPU_IntelMicroarch::Core;
            switch (InstructionSet::Stepping ())
            {
              case 0x06: // B2
                switch (cpu.cores.size ())
                {
                  case 2:
                    for ( auto& core : cpu.cores )
                      core.second.tjMax = 80.0 + 10.0;
                    break;
                  case 4:
                    for ( auto& core : cpu.cores )
                      core.second.tjMax = 90.0 + 10.0;
                    break;
                  default:
                    for ( auto& core : cpu.cores )
                      core.second.tjMax = 85.0 + 10.0;
                    break;
                }
                for ( auto& core : cpu.cores )
                  core.second.tjMax = 85.0 + 10.0;
                break;
              case 0x0B: // G0
                for ( auto& core : cpu.cores )
                  core.second.tjMax = 90.0 + 10.0;
                break;
              case 0x0D: // M0
                for ( auto& core : cpu.cores )
                  core.second.tjMax = 85.0 + 10.0;
                break;
              default:
                for ( auto& core : cpu.cores )
                  core.second.tjMax = 85.0 + 10.0;
                break;
            }
            break;

          case 0x17: // Intel Core 2 (45nm)
            cpu.intel_arch = SK_CPU_IntelMicroarch::Core;
            for ( auto& core : cpu.cores )
              core.second.tjMax = 100.0;
            break;

          case 0x1C: // Intel Atom (45nm)
            cpu.intel_arch = SK_CPU_IntelMicroarch::Atom;

            switch (InstructionSet::Stepping ())
            {
              case 0x02: // C0
                for ( auto& core : cpu.cores )
                  core.second.tjMax = 90.0;
                break;

              case 0x0A: // A0, B0
                for ( auto& core : cpu.cores )
                  core.second.tjMax = 100.0;
                break;

              default:
                for ( auto& core : cpu.cores )
                  core.second.tjMax = 90.0;
                break;
            } break;

          case 0x1A: // Intel Core i7 LGA1366 (45nm)
          case 0x1E: // Intel Core i5, i7 LGA1156 (45nm)
          case 0x1F: // Intel Core i5, i7 
          case 0x25: // Intel Core i3, i5, i7 LGA1156 (32nm)
          case 0x2C: // Intel Core i7 LGA1366 (32nm) 6 Core
          case 0x2E: // Intel Xeon Processor 7500 series (45nm)
          case 0x2F: // Intel Xeon Processor (32nm)
            cpu.intel_arch = SK_CPU_IntelMicroarch::Nehalem;
            for ( auto& core : cpu.cores ) core.second.tjMax =
              SK_CPU_GetIntelTjMax (core.second.cpu_core);
            break;

          case 0x0A: // -----
          case 0x2A: // Intel Core i5, i7 2xxx LGA1155 (32nm)
          case 0x2D: // Next Generation Intel Xeon, i7 3xxx LGA2011 (32nm)
            cpu.intel_arch = SK_CPU_IntelMicroarch::SandyBridge;
            for ( auto& core : cpu.cores ) core.second.tjMax =
              SK_CPU_GetIntelTjMax (core.second.cpu_core);
            break;

          case 0x3A: // Intel Core i5, i7 3xxx LGA1155 (22nm)
          case 0x3E: // Intel Core i7 4xxx LGA2011 (22nm)
            cpu.intel_arch = SK_CPU_IntelMicroarch::IvyBridge;
            for ( auto& core : cpu.cores ) core.second.tjMax =
              SK_CPU_GetIntelTjMax (core.second.cpu_core);
            break;

          case 0x3C: // Intel Core i5, i7 4xxx LGA1150 (22nm)
          case 0x3F: // Intel Xeon E5-2600/1600 v3, Core i7-59xx
                     // LGA2011-v3, Haswell-E (22nm)
          case 0x45: // Intel Core i5, i7 4xxxU (22nm)
          case 0x46: 
            cpu.intel_arch = SK_CPU_IntelMicroarch::Haswell;
            for ( auto& core : cpu.cores ) core.second.tjMax =
              SK_CPU_GetIntelTjMax (core.second.cpu_core);
            break;

          case 0x3D: // Intel Core M-5xxx (14nm)
          case 0x47: // Intel i5, i7 5xxx, Xeon E3-1200 v4 (14nm)
          case 0x4F: // Intel Xeon E5-26xx v4
          case 0x56: // Intel Xeon D-15xx
            cpu.intel_arch = SK_CPU_IntelMicroarch::Broadwell;
            for ( auto& core : cpu.cores ) core.second.tjMax =
              SK_CPU_GetIntelTjMax (core.second.cpu_core);
            break;

          case 0x36: // Intel Atom S1xxx, D2xxx, N2xxx (32nm)
            cpu.intel_arch = SK_CPU_IntelMicroarch::Atom;
            for ( auto& core : cpu.cores ) core.second.tjMax =
              SK_CPU_GetIntelTjMax (core.second.cpu_core);
            break;

          case 0x37: // Intel Atom E3xxx, Z3xxx (22nm)
          case 0x4A:
          case 0x4D: // Intel Atom C2xxx (22nm)
          case 0x5A:
          case 0x5D:
            cpu.intel_arch = SK_CPU_IntelMicroarch::Silvermont;
            for ( auto& core : cpu.cores ) core.second.tjMax =
              SK_CPU_GetIntelTjMax (core.second.cpu_core);
            break;

          case 0x4E:
          case 0x0E: // Intel Core i7     6xxxx BGA1440 (14nm)
          case 0x5E: // Intel Core i5, i7 6xxxx LGA1151 (14nm)
          case 0x55: // Intel Core i7, i9 7xxxx LGA2066 (14nm)
            cpu.intel_arch = SK_CPU_IntelMicroarch::Skylake;
            for ( auto& core : cpu.cores ) core.second.tjMax =
              SK_CPU_GetIntelTjMax (core.second.cpu_core);
            break;

          case 0x4C:
            cpu.intel_arch = SK_CPU_IntelMicroarch::Airmont;
            for ( auto& core : cpu.cores ) core.second.tjMax =
              SK_CPU_GetIntelTjMax (core.second.cpu_core);
            break;

          case 0x8E: 
          case 0x9E: // Intel Core i5, i7 7xxxx (14nm)
            cpu.intel_arch = SK_CPU_IntelMicroarch::KabyLake;
            for ( auto& core : cpu.cores ) core.second.tjMax =
              SK_CPU_GetIntelTjMax (core.second.cpu_core);
            break;

          case 0x5C: // Intel Atom processors
            cpu.intel_arch = SK_CPU_IntelMicroarch::ApolloLake;
            for ( auto& core : cpu.cores ) core.second.tjMax =
              SK_CPU_GetIntelTjMax (core.second.cpu_core);
            break;

          default:
            cpu.intel_arch = SK_CPU_IntelMicroarch::UnknownIntel;
            for ( auto& core : cpu.cores ) core.second.tjMax =
              SK_CPU_GetIntelTjMax (core.second.cpu_core);
            break;
        }
      } break;

      case 0x0F:
      {
        switch (InstructionSet::Model ())
        {
          case 0x00: // Pentium 4 (180nm)
          case 0x01: // Pentium 4 (130nm)
          case 0x02: // Pentium 4 (130nm)
          case 0x03: // Pentium 4, Celeron D (90nm)
          case 0x04: // Pentium 4, Pentium D, Celeron D (90nm)
          case 0x06: // Pentium 4, Pentium D, Celeron D (65nm)
            cpu.intel_arch = SK_CPU_IntelMicroarch::NetBurst;
            for ( auto& core : cpu.cores )
              core.second.tjMax = 100.0;
            break;

          default:
            cpu.intel_arch = SK_CPU_IntelMicroarch::UnknownIntel;
            for ( auto& core : cpu.cores )
              core.second.tjMax = 100.0;
            break;
        }
      } break;

      default:
        cpu.intel_arch = SK_CPU_IntelMicroarch::UnknownIntel;
        break;
    }
  }

  if (cpu.intel_arch < SK_CPU_IntelMicroarch::NotIntel)
  {
    switch (cpu.intel_arch)
    {
      case SK_CPU_IntelMicroarch::NetBurst:
      case SK_CPU_IntelMicroarch::Atom:
      case SK_CPU_IntelMicroarch::Core:
      {
        if (Rdmsr (IA32_PERF_STATUS, &eax, &edx))
        {
          cpu.coefficients.intel.tsc =
            ((edx >> 8) & 0x1f) + 0.5 * ((edx >> 14) & 1);
        }
      } break;

      case SK_CPU_IntelMicroarch::Nehalem:
      case SK_CPU_IntelMicroarch::SandyBridge:
      case SK_CPU_IntelMicroarch::IvyBridge:
      case SK_CPU_IntelMicroarch::Haswell: 
      case SK_CPU_IntelMicroarch::Broadwell:
      case SK_CPU_IntelMicroarch::Silvermont:
      case SK_CPU_IntelMicroarch::Skylake:
      case SK_CPU_IntelMicroarch::Airmont:
      case SK_CPU_IntelMicroarch::KabyLake:
      case SK_CPU_IntelMicroarch::ApolloLake:
      {
        if (Rdmsr (PLATFORM_INFO, &eax, &edx))
        {
          cpu.coefficients.intel.tsc =
            (eax >> 8) & 0xff;
        }
      } break;

      default: 
        cpu.coefficients.intel.tsc = 0.0;
        break;
    }

    if (Rdmsr (RAPL_POWER_UNIT, &eax, &edx))
    {
      switch (cpu.intel_arch)
      {
        case SK_CPU_IntelMicroarch::Silvermont:
        case SK_CPU_IntelMicroarch::Airmont:
          cpu.coefficients.energy =
            1.0e-6 * static_cast <double> (1 << (int)((eax >> 8) & 0x1F));
          break;

        default:
          cpu.coefficients.energy =
            1.0 / static_cast <double> (1 << (int)((eax >> 8) & 0x1F));
          break;
      }
    }
  }

  return
    cpu.intel_arch;
}

bool
SK_CPU_IsZen (void)
{
  static int is_zen = -1;

  if (is_zen == -1)
  {
    is_zen = 0;

    if (! SK_WR0_Init ())
      return false;

    auto& cpu =
      __SK_CPU;

    int i = 0;

    for ( auto zen : _SK_KnownZen )
    {
      if ( strstr ( InstructionSet::Brand ().c_str (),
                      zen.brandId )
         )
      {
        cpu.offsets.temperature =
          _SK_KnownZen [i].tempOffset;

        // 1.0 / 65359.4771
        cpu.coefficients.energy =
          1.5300000005661000002094570000775e-5;

        is_zen      = 1;
        break;
      }

      i++;
    }
  }

  return
    (is_zen == 1);
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
//      TU_TimeUnits	   :  20 - 16,
//      Reserved0	       :  64 - 20;
//  };
//} RAPL_POWER_UNIT;

double
SK_CPU_GetJoulesConsumedTotal (DWORD_PTR package)
{
  UNREFERENCED_PARAMETER (package);

  auto& cpu =
    __SK_CPU;

  if (SK_CPU_IsZen ())
  {
    DWORD     eax,  edx;
    DWORD     eax2, edx2;

    // AMD Model 17h measures this in 15.3 micro-joule increments
    if (RdmsrTx (0xC001029B, &eax,  &edx,  std::numeric_limits <DWORD_PTR>::max ()))
    {   RdmsrTx (0xC0010299, &eax2, &edx2, std::numeric_limits <DWORD_PTR>::max ());

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
          static_cast <long double> (eax) *
                                     cpu.coefficients.energy
        );
    }
  }

  else if (SK_CPU_GetIntelMicroarch () < SK_CPU_IntelMicroarch::NotIntel)
  {
    if (cpu.coefficients.energy != 0.0)
    {
      DWORD eax, edx;

      if (Rdmsr (PKG_ENERGY_STATUS, &eax, &edx))
      {
        return 
          cpu.coefficients.energy *
            static_cast <double> (eax);
      }
    }
  }

  return 0.0;
}

double
SK_CPU_GetJoulesConsumed (int64_t core)
{
  auto& cpu =
    __SK_CPU;

  DWORD_PTR thread_mask = (1ULL << core);
  DWORD     eax,  edx,
            eax2, edx2;

  if (SK_CPU_IsZen ())
  {
    // AMD Model 17h measures this in 15.3 micro-joule increments
    if (RdmsrTx (0xC001029A, &eax,  &edx,  thread_mask))
    {   RdmsrTx (0xC0010299, &eax2, &edx2, thread_mask);

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
        static_cast <double> (
          static_cast <long double> (eax) *
                                     cpu.coefficients.energy
        );
    }
  }

  else if (SK_CPU_GetIntelMicroarch () < SK_CPU_IntelMicroarch::NotIntel)
  {
    const double div =
      1.0 / static_cast <double> (cpu.cores.size ());

    if (RdmsrTx (PP1_ENERGY_STATUS, &eax, &edx, thread_mask) && eax != 0)
    {
      return static_cast <double> (eax) *
                                   cpu.coefficients.energy *
                                   div;
    }

    else if (RdmsrTx (PP0_ENERGY_STATUS, &eax, &edx, thread_mask) && eax != 0)
    {
      return
        static_cast <double> (eax) *
                              cpu.coefficients.energy *
                              div;
    }
  }

  return 0.0;
}

double
SK_CPU_GetTemperature_AMDZen (int core)
{
  UNREFERENCED_PARAMETER (core);

  if (! SK_CPU_IsZen ())
  {
    return 0.0;
  }

  auto& cpu =
    __SK_CPU;

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
    static_cast <double> ( cpu.offsets.temperature +
        ( usingNeg49To206C ?
            ( invScale11BitDbl (sensor) * 206.0 ) - 49.0 :
              invScale11BitDbl (sensor) * 255.0            )
    );
}

struct {
  double cumulative_MHz;
  int    num_cores;

  uint64_t getAvg (void) {
    return (num_cores > 0) ?
      static_cast <uint64_t> ( cumulative_MHz /
             static_cast <double> (num_cores) ) : 0;
  }
} __AverageEffectiveClock;

void
SK_CPU_UpdatePackageSensors (int package)
{
  auto& cpu =
    __SK_CPU;

  double time_elapsed_ms =
    SK_DeltaPerfMS (cpu.pkg_sensor.sample_taken, 1.0);

  if (time_elapsed_ms < 666.666667)
  {
    return;// cpu.pkg_sensor;
  }

  double J =
    SK_CPU_GetJoulesConsumedTotal (package);

  if (J != 0.0)
  {
    if (time_elapsed_ms > 666.666667)
    {
      double joules_used =
        (J - cpu.pkg_sensor.accum.joules);

      cpu.pkg_sensor.accum.joules =
        J;
      cpu.pkg_sensor.power_W      = 
        joules_used / ( time_elapsed_ms / 1000.0 );
    };
  }

  cpu.pkg_sensor.sample_taken =
    SK_QueryPerf ().QuadPart;
}

SK_CPU_CoreSensors&
SK_CPU_UpdateCoreSensors (int core)
{
  std::unordered_map <int, SK_CPU_CoreSensors>& cores
    = __SK_CPU.cores;

  double time_elapsed_ms =
    SK_DeltaPerfMS (cores [core].sample_taken, 1.0);

  if (time_elapsed_ms < 666.666667)
  {
    return cores [core];
  }


  DWORD_PTR thread_mask = (1ULL << core);
  DWORD     eax,  edx;

  if (SK_CPU_IsZen ())
  {
    RdmsrTx (AMD_ZEN_MSR_PSTATE_STATUS, &eax, &edx, thread_mask);

    int Did = (int)((eax >> 8) & 0x3F);
    int Fid = (int)( eax       & 0xFF);

    cores [core].clock_MHz =
      ( static_cast <double> (Fid) /
       static_cast <double> (Did)  ) * 200.0 * 1000000.0;

    double J =
      SK_CPU_GetJoulesConsumed (core);

    double joules_used  =
      (J - cores [core].accum.joules);

    cores [core].accum.joules = J;
    cores [core].power_W      =
      ( joules_used / (time_elapsed_ms / 1000.0) );

    cores [core].temperature_C =
      SK_CPU_GetTemperature_AMDZen (core);
  }

  else if (SK_CPU_GetIntelMicroarch () < SK_CPU_IntelMicroarch::NotIntel)
  {
    if ( RdmsrTx (SK_CPU_IntelMSR::IA32_THERM_STATUS, &eax, &edx, thread_mask) &&
           (eax & 0x80000000) != 0 )
    {
      // get the dist from tjMax from bits 22:16
      double deltaT = static_cast <double> (((eax & 0x007F0000UL) >> 16UL));
      double tjMax  = cores [core].tjMax;
      double tSlope = 1.0;//coreTemperatures[i].Parameters[1].Value;
        cores [core].temperature_C =
          tjMax - tSlope * deltaT;
    }
    
    else {
      cores [core].temperature_C = 0.0;
    }

    auto& cpu = __SK_CPU;

    if (cpu.coefficients.energy != 0.0)
    {
      double J =
        SK_CPU_GetJoulesConsumed (core);

      double joules_used  =
        (J - cores [core].accum.joules);

      cores [core].accum.joules = J;
      cores [core].power_W      =
        ( joules_used / (time_elapsed_ms / 1000.0) );
    }
  }

  cores [core].sample_taken =
    SK_QueryPerf ().QuadPart;

  return cores [core];
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
      wchar_t
      wszDesc [384] = { };
      dwLen =  383;
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
    memset (active_scheme.utf8_desc, 0, 384);

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

  void run (void) override
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

    if (                   pwi != nullptr                                         &&
         last_update           < ( SK::ControlPanel::current_time - update_freq ) &&
          cpu_stats.num_cpus   > 0 )
    {
      if ( cpu_stats.num_cpus  > 0 &&
         ( cpu_records.size () < ( cpu_stats.num_cpus + 1 )
         )
         ) { cpu_records.resize  ( cpu_stats.num_cpus + 1 ); }

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

      float avg_clock = 0.0f;

      for (unsigned int i = 0; i < sinfo.dwNumberOfProcessors; i++)
      {
        avg_clock +=
          static_cast <float> (pwi [i].CurrentMhz);
      }

      if (__AverageEffectiveClock.getAvg () == 0)
      {
        cpu_clocks.addValue ( avg_clock /
                                static_cast <float> (sinfo.dwNumberOfProcessors) );
      }

      else
      {
        cpu_clocks.addValue (
          __AverageEffectiveClock.getAvg () / 1000000.0f
        );

        __AverageEffectiveClock.num_cores      = 0;
        __AverageEffectiveClock.cumulative_MHz = 0;
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

  void draw (void) override
  {
    if (! ImGui::GetFont ()) return;

    const  float font_size   = ImGui::GetFont ()->FontSize;
    static char  szAvg [512] = { };
    static bool  detailed    = true;
    static bool  show_parked = true;
    static bool  show_graphs = false;

    static int   last_parked_count = 0;
    
    ImGui::BeginGroup ();

    SK_ImGui::VerticalToggleButton     ( "All CPUs", &detailed  );

    if (detailed)
    {
      SK_ImGui::VerticalToggleButton   ( "Graphs", &show_graphs );

      if (last_parked_count > 0)
        SK_ImGui::VerticalToggleButton ( "Parked", &show_parked );
    }

    ImGui::EndGroup ();

    last_parked_count = 0;

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

    SK_CPU_UpdatePackageSensors (0);

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
                    u8"%s\t\t\n\n"
                    u8"          min: %3.0f%%, max: %3.0f%%, avg: %3.0f%%\n",
                      InstructionSet::Brand ().c_str (),
                      cpu_records [i].getMin (), cpu_records [i].getMax (),
                      cpu_records [i].getAvg () );
      }

      char szName [128] = { };

      sprintf (szName, "###CPU_%u", i-1);

      float samples = 
        std::min ( (float)cpu_records [i].getUpdates  (),
                   (float)cpu_records [i].getCapacity () );

      if (i == 1)
        ImGui::Separator ();

      ImGui::PushStyleColor ( ImGuiCol_PlotLines, 
                          ImColor::HSV ( 0.31f - 0.31f *
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
                                           ImVec2 (
                                             std::max ( 500.0f,
                                        ImGui::GetContentRegionAvailWidth () ),
                                               font_size * 4.0f
                                                  )
                            );
        }
      }

      bool found = false;
  
      auto DrawHeader = [&](int j) -> bool
      {
        if (j == 0)
        { 
          found = true;

          ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.28F, 1.f, 1.f, 1.f));
          ImGui::Text           ("%4.2f GHz", cpu_clocks.getAvg () / 1000.0f);
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

          double J =
            SK_CPU_GetJoulesConsumedTotal (0);

          if (J != 0.0)
          {
            ImGui::SameLine        ();
            ImGui::PushStyleColor  (ImGuiCol_Text, ImColor::HSV (0.67194F, 0.15f, 0.95f, 1.f));
            ImGui::TextUnformatted (u8"ー");
            ImGui::SameLine        ();
            ImGui::PushStyleColor  (ImGuiCol_Text, ImColor::HSV (0.13294F, 0.734f, .94f, 1.f));
            ImGui::Text            ("%05.2f W", __SK_CPU.pkg_sensor.power_W );
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

          ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.28F, 1.f, 1.f, 1.f));

          if (parked_since != 0)
            ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.28F, 0.5f, 0.5f, 1.f));
          else
            ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.28F, 1.f, 1.f, 1.f));

          if (core_sensors.clock_MHz == 0.0)
          {
            if (parked_since == 0 || show_parked)
              ImGui::Text       ("%4.2f GHz", static_cast <float> (cpu_stats.cpus [j-1].CurrentMhz) / 1000.0f);
          }

          else
          {
            __AverageEffectiveClock.cumulative_MHz += core_sensors.clock_MHz;
            __AverageEffectiveClock.num_cores++;

            if (parked_since == 0 || show_parked)
              ImGui::Text       ("%4.2f GHz", core_sensors.clock_MHz / 1e+9f);
          }

          if (core_sensors.temperature_C != 0.0)
          {
            static std::string core_temp;
            
            //if (! SK_CPU_IsZen ())
            //{
              core_temp.assign (
                SK_FormatTemperature (
                  core_sensors.temperature_C,
                    Celsius,
                      config.system.prefer_fahrenheit ? Fahrenheit :
                                                        Celsius )
              );
            //}

            //else
            //{
            //  core_temp.assign (u8"………");
            //}

            if (parked_since == 0 || show_parked)
            {
              ImGui::SameLine        ();
              ImGui::PushStyleColor  (ImGuiCol_Text, ImColor::HSV (0.67194F, 0.15f, 0.95f, 1.f));
              ImGui::TextUnformatted (u8"ー");
              if (SK_CPU_IsZen ())
                ImGui::PushStyleColor  (ImGuiCol_Text, ImColor::HSV (
                  static_cast <float> (0.3 - (0.3 * std::min (1.0, ((core_sensors.temperature_C / 2.0) / 100.0)))),
                                       0.725f, 0.725f, 1.f));
              else
                ImGui::PushStyleColor  (ImGuiCol_Text, ImColor::HSV (
                  static_cast <float> (0.3 - (0.3 * std::min (1.0, ((core_sensors.temperature_C / 2.0) / 100.0)))),
                                       1.f, 1.f, 1.f));
              ImGui::SameLine        ();
              ImGui::TextUnformatted (core_temp.c_str ());
              ImGui::PopStyleColor   (2);
            }
          }

          if (core_sensors.power_W != 0.0 && (parked_since == 0 || show_parked))
          {
            ImGui::SameLine        (      );
            ImGui::PushStyleColor  (ImGuiCol_Text, ImColor::HSV (0.67194F, 0.15f, 0.95f, 1.f));
            ImGui::TextUnformatted (u8"ー");
            ImGui::SameLine        (      );
            ImGui::PushStyleColor  (ImGuiCol_Text, ImColor::HSV (0.13294F, 0.734f, .94f, 1.f));
            ImGui::Text            ("%05.2f W", core_sensors.power_W );
            ImGui::PopStyleColor   (2);
          }

          if (parked_since == 0 || show_parked)
          {
            ImGui::SameLine        ( ); ImGui::Spacing ( ); ImGui::SameLine ();

            if (parked_since == 0)
              ImGui::PushStyleColor(ImGuiCol_Text, ImColor::HSV (0.3f - (0.3f * (cpu_records [j].getAvg () / 100.0f)), 0.80f, 0.95f, 1.f));
            else
              ImGui::PushStyleColor(ImGuiCol_Text, ImColor::HSV (0.57194F,                                             0.5f,  0.5f, 1.f));
            ImGui::TextUnformatted (u8"〇");
            ImGui::SameLine        ( ); ImGui::Spacing ( ); ImGui::SameLine ();
            ImGui::Text            ("%02.0f%%", cpu_records [j].getAvg ());
            ImGui::PopStyleColor   ( );

            if (parked_since > 0)
            {
              ImGui::SameLine        (      );
              ImGui::PushStyleColor  (ImGuiCol_Text, ImColor::HSV (0.67194F, 0.15f, 0.95f, 1.f));
              ImGui::TextUnformatted (u8"ー");
              ImGui::SameLine        (      );
              ImGui::PushStyleColor  (ImGuiCol_Text, ImColor::HSV (0.57194F, 0.534f, .94f, 1.f));
              ImGui::Text            ( "Parked %5.1f Secs",
                                        SK_DeltaPerfMS ( parked_since, 1 ) / 1000.0
                                     );
              ImGui::PopStyleColor   (2);
            }
          }

          ImGui::PopStyleColor ();

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
    HPOWERNOTIFY  notify = INVALID_HANDLE_VALUE;
    volatile LONG dirty  = FALSE;
  } active_scheme;

  DWORD       last_update        = 0UL;

  std::vector <SK_Stat_DataHistory <float, 96>> cpu_records = { };
               SK_Stat_DataHistory <float,  3>  cpu_clocks  = { };
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