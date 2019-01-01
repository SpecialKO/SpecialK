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

#include <SpecialK/render/backend.h>
#include <SpecialK/render/dxgi/dxgi_backend.h>

#include <SpecialK/performance/gpu_monitor.h>
#include <SpecialK/config.h>

#include <SpecialK/log.h>

#include <algorithm>
#include <vector>
#include <cassert>

//
// All GPU performance data are double-buffered and it is safe to read
//   gpu_stats without any thread synchronization.
//
//  >> It will always reference the last complete set of performance data.
//
volatile LONG           current_gpu_stat      =  0;
         gpu_sensors_t  gpu_stats_buffers [2] = { };
         gpu_sensors_t& gpu_stats             = gpu_stats_buffers [0];

#include <SpecialK/nvapi.h>
extern BOOL nvapi_init;

//#define PCIE_WORKS

#include <SpecialK/adl.h>
extern BOOL ADL_init;

#define NVAPI_GPU_UTILIZATION_DOMAIN_GPU 0
#define NVAPI_GPU_UTILIZATION_DOMAIN_FB  1
#define NVAPI_GPU_UTILIZATION_DOMAIN_VID 2
#define NVAPI_GPU_UTILIZATION_DOMAIN_BUS 3

static HANDLE hPollEvent     = nullptr;
static HANDLE hShutdownEvent = nullptr;
static HANDLE hPollThread    = nullptr;

static std::vector <NvU32> gpu_ids;

DWORD
__stdcall
SK_GPUPollingThread (LPVOID user)
{
  UNREFERENCED_PARAMETER (user);

  const HANDLE hEvents [2] = {
    hPollEvent,
    hShutdownEvent
  };

  SetCurrentThreadDescription  (     L"[SK] GPU Performance Monitoring Thread");
  SK_Thread_SetCurrentPriority (THREAD_PRIORITY_IDLE);
  SetThreadPriorityBoost       (GetCurrentThread (), TRUE);
  

  auto SwitchToThreadMinPageFaults = [](void) ->
  void
  {
    static int iters = 0;

    if ((iters++ % 13) == 0)
      SleepEx (1, FALSE);

    else if ((iters % 9) != 0)
      SwitchToThread ();
  };


  NvPhysicalGpuHandle gpus [NVAPI_MAX_PHYSICAL_GPUS] = { };
  NvU32               gpu_count                      =  0;


  if (nvapi_init)
  {
    SK_RunOnce (assert (NvAPI_GetGPUIDFromPhysicalGPU != nullptr));
    SK_RunOnce (assert (NvAPI_GetPhysicalGPUFromGPUID != nullptr));

    if (NVAPI_OK != NvAPI_EnumPhysicalGPUs (gpus, &gpu_count))
    {
      return 0;
    }
  }


  while (true)
  {
    DWORD dwWait =
      WaitForMultipleObjectsEx (2, hEvents, FALSE, INFINITE, FALSE);

    if (dwWait == WAIT_OBJECT_0 + 1)
      break;

    else if (dwWait != WAIT_OBJECT_0)
      break;

    if (InterlockedCompareExchange (&current_gpu_stat, TRUE, FALSE))
        InterlockedCompareExchange (&current_gpu_stat, FALSE, TRUE);

    gpu_sensors_t& stats = gpu_stats_buffers [ReadAcquire (&current_gpu_stat)];

    if (nvapi_init)
    {
      stats.num_gpus =
        std::min (gpu_count, static_cast <NvU32> (NVAPI_MAX_PHYSICAL_GPUS));

      NV_GPU_DYNAMIC_PSTATES_INFO_EX psinfoex;
      psinfoex.version = NV_GPU_DYNAMIC_PSTATES_INFO_EX_VER;

      static bool init = false;

      if (! init)
      {
        for (int i = 0; i < stats.num_gpus; i++)
        {
          NvU32 gpu_id = 0;

          if (NvAPI_GetGPUIDFromPhysicalGPU != nullptr)
              NvAPI_GetGPUIDFromPhysicalGPU ( gpus [i], &gpu_id );

          gpu_ids.push_back                 (            gpu_id );
        }

        std::sort (gpu_ids.begin (), gpu_ids.end ());

        init = true;
      }

      for (int i = 0; i < stats.num_gpus; i++)
      {
        NvPhysicalGpuHandle gpu = { };

        // In order for DXGI Adapter info to match up... don't just assign
        //   these GPUs wily-nilly, use the high 24-bits of the GPUID as
        //     a bitmask.
        //NvAPI_GetPhysicalGPUFromGPUID (1 << (i + 8), &gpu);
        //stats.gpus [i].nv_gpuid = (1 << (i + 8));

        if (NvAPI_GetPhysicalGPUFromGPUID != nullptr)
            NvAPI_GetPhysicalGPUFromGPUID (gpu_ids [i], &gpu);

        stats.gpus [i].nv_gpuid = gpu_ids [i];

        NvAPI_Status        status =
          NvAPI_GPU_GetDynamicPstatesInfoEx (gpu, &psinfoex);

        if (status == NVAPI_OK)
        {
          stats.gpus [i].loads_percent.gpu =
            psinfoex.utilization [NVAPI_GPU_UTILIZATION_DOMAIN_GPU].percentage;
          stats.gpus [i].loads_percent.fb =
            psinfoex.utilization [NVAPI_GPU_UTILIZATION_DOMAIN_FB].percentage;
          stats.gpus [i].loads_percent.vid =
            psinfoex.utilization [NVAPI_GPU_UTILIZATION_DOMAIN_VID].percentage;
          stats.gpus [i].loads_percent.bus =
            psinfoex.utilization [NVAPI_GPU_UTILIZATION_DOMAIN_BUS].percentage;
        }


        static bool has_thermal = true;


        NV_GPU_THERMAL_SETTINGS thermal = { };
        thermal.version = NV_GPU_THERMAL_SETTINGS_VER;


        // This is needed to fix some problems with Mirilis Action injecting itself
        //   and immediately breaking NvAPI.
        if (has_thermal)
        {
          status =
            NvAPI_GPU_GetThermalSettings ( gpu,
                                             NVAPI_THERMAL_TARGET_ALL,
                                               &thermal );

          if (status != NVAPI_OK) has_thermal = false;
        }

        if (! has_thermal) status = NVAPI_ERROR;


        if (status == NVAPI_OK)
        {
          for (NvU32 j = 0; j < std::min (3UL, thermal.count); j++)
          {
#ifdef SMOOTH_GPU_UPDATES
            if (thermal.sensor [j].target == NVAPI_THERMAL_TARGET_GPU)
              stats.gpus [i].temps_c.gpu = (stats.gpus[i].temps_c.gpu + thermal.sensor [j].currentTemp) / 2;
#else
            if (thermal.sensor [j].target == NVAPI_THERMAL_TARGET_GPU)
              stats.gpus [i].temps_c.gpu = thermal.sensor [j].currentTemp;
            if (thermal.sensor [j].target == NVAPI_THERMAL_TARGET_MEMORY)
              stats.gpus [i].temps_c.ram = thermal.sensor [j].currentTemp;
            if (thermal.sensor [j].target == NVAPI_THERMAL_TARGET_POWER_SUPPLY)
              stats.gpus [i].temps_c.psu = thermal.sensor [j].currentTemp;
            if (thermal.sensor [j].target == NVAPI_THERMAL_TARGET_BOARD)
              stats.gpus [i].temps_c.pcb = thermal.sensor [j].currentTemp;
#endif
          }
        }

        else has_thermal = false;



        NvU32 pcie_lanes = 0;
        if (NVAPI_OK == NvAPI_GPU_GetCurrentPCIEDownstreamWidth (gpu, &pcie_lanes))
        {
          stats.gpus [i].hwinfo.pcie_lanes = pcie_lanes;
        }

        NV_PCIE_INFO pcieinfo         = {              };
                     pcieinfo.version = NV_PCIE_INFO_VER;

        if (            NvAPI_GPU_GetPCIEInfo != nullptr &&
            NVAPI_OK == NvAPI_GPU_GetPCIEInfo (gpu, &pcieinfo))
        {
          stats.gpus [i].hwinfo.pcie_gen           =
            pcieinfo.info [0].unknown5;               //states [pstate].pciLinkRate;
          stats.gpus [i].hwinfo.pcie_lanes         =
            pcieinfo.info [0].unknown6;              //pstates [pstate].pciLinkWidth;
          stats.gpus [i].hwinfo.pcie_transfer_rate =
            pcieinfo.info [0].unknown0;              //pstates [pstate].pciLinkTransferRate;
        }

        NvU32 mem_width = 0,
              mem_loc   = 0,
              mem_type  = 0;

        if (            NvAPI_GPU_GetFBWidthAndLocation != nullptr &&
            NVAPI_OK == NvAPI_GPU_GetFBWidthAndLocation (gpu, &mem_width, &mem_loc))
        {
          stats.gpus [i].hwinfo.mem_bus_width = mem_width;
        }

        if (            NvAPI_GPU_GetRamType != nullptr &&
            NVAPI_OK == NvAPI_GPU_GetRamType (gpu, &mem_type))
        {
          stats.gpus [i].hwinfo.mem_type = mem_type;
        }

        NV_DISPLAY_DRIVER_MEMORY_INFO meminfo = { };
                                      meminfo.version = NV_DISPLAY_DRIVER_MEMORY_INFO_VER;

        if (NVAPI_OK == NvAPI_GPU_GetMemoryInfo (gpu, &meminfo))
        {
          int64_t local =
            (meminfo.availableDedicatedVideoMemory) -
            (meminfo.curAvailableDedicatedVideoMemory);

          stats.gpus [i].memory_B.local    = local                        * 1024LL;
          stats.gpus [i].memory_B.capacity = meminfo.dedicatedVideoMemory * 1024LL;

          stats.gpus [i].memory_B.total =
            ((meminfo.dedicatedVideoMemory) -
             (meminfo.curAvailableDedicatedVideoMemory)) * 1024LL;

          // Compute Non-Local
          stats.gpus [i].memory_B.nonlocal =
            stats.gpus [i].memory_B.total - stats.gpus [i].memory_B.local;
        }


      //SwitchToThreadMinPageFaults ();

        static int amortization0 = 0;

        if (amortization0++ % 3 == 0)
        {
          NV_GPU_CLOCK_FREQUENCIES freq           = {                          };
                                   freq.version   = NV_GPU_CLOCK_FREQUENCIES_VER;
                                   freq.ClockType = NV_GPU_CLOCK_FREQUENCIES_CURRENT_FREQ;

          if (NVAPI_OK == NvAPI_GPU_GetAllClockFrequencies (gpu, &freq))
          {
            stats.gpus [i].clocks_kHz.gpu    =
              freq.domain [NVAPI_GPU_PUBLIC_CLOCK_GRAPHICS].frequency;
            stats.gpus [i].clocks_kHz.ram    =
              freq.domain [NVAPI_GPU_PUBLIC_CLOCK_MEMORY].frequency;
            ////stats.gpus [i].clocks_kHz.shader =
              ////freq.domain [NVAPI_GPU_PUBLIC_CLOCK_PROCESSOR].frequency;
          }
        }

        static int amortization1 = 0;

        if (amortization1++ % 2 == 0)
        {
          NvU32 tach = 0;

          stats.gpus [i].fans_rpm.supported = false;

          if (NVAPI_OK == NvAPI_GPU_GetTachReading (gpu, &tach))
          {
            stats.gpus [i].fans_rpm.gpu       = tach;
            stats.gpus [i].fans_rpm.supported = true;
          }
        }

        static int iter = 0;
        
        // This is an expensive operation for very little gain,
        //   it rarely changes, but it eats CPU time.
        if ((iter++ % 8) == 0 && config.gpu.print_slowdown)
        {
          NvU32 perf_decrease_info = 0;

          NvAPI_GPU_GetPerfDecreaseInfo (gpu, &perf_decrease_info);

          stats.gpus [i].nv_perf_state = perf_decrease_info;
        }

        static NV_GPU_PERF_PSTATES20_INFO ps20info = {                            };
               ps20info.version                    = NV_GPU_PERF_PSTATES20_INFO_VER;

        stats.gpus [i].volts_mV.supported   = false;

        static bool has_pstates     = true;
        static bool queried_pstates = false;

        if ( queried_pstates ||
             ( has_pstates &&
               NVAPI_OK == NvAPI_GPU_GetPstates20 (gpu, &ps20info) )
           )
        {
          // Rather than querying the list over and over, just get the current
          //   index into the list.
          queried_pstates = true;

          if (ps20info.numPstates == 0) has_pstates = false;

          NV_GPU_PERF_PSTATE_ID current_pstate = { };

          if (NVAPI_OK == NvAPI_GPU_GetCurrentPstate (gpu, &current_pstate))
          {
            for (NvU32 pstate = 0; pstate < ps20info.numPstates; pstate++)
            {
              if (ps20info.pstates [pstate].pstateId == current_pstate)
              {
                // First, check for over-voltage...
                if (stats.gpus [i].volts_mV.supported == false)
                {
                  for (NvU32 volt = 0; volt < ps20info.ov.numVoltages; volt++)
                  {
                    if ( ps20info.ov.voltages [volt].domainId ==
                         NVAPI_GPU_PERF_VOLTAGE_INFO_DOMAIN_CORE )
                    {
                      stats.gpus [i].volts_mV.supported = true;
                      stats.gpus [i].volts_mV.over      = true;

                      NV_GPU_PSTATE20_BASE_VOLTAGE_ENTRY_V1* voltage =
                        &ps20info.ov.voltages [volt];

                      stats.gpus [i].volts_mV.core = voltage->volt_uV/1000.0f;

                      int over  =
                        voltage->voltDelta_uV.value -
                        voltage->voltDelta_uV.valueRange.max;

                      int under =
                        voltage->voltDelta_uV.valueRange.min -
                        voltage->voltDelta_uV.value;

                      if (over > 0)
                        stats.gpus [i].volts_mV.ov =   over  / 1000.0f;
                      else if (under > 0)
                        stats.gpus [i].volts_mV.ov = -(under / 1000.0f);
                      break;
                    }
                  }
                }

                // If that fails, look through the normal voltages.
                for (NvU32 volt = 0; volt < ps20info.numBaseVoltages; volt++)
                {
                  if ( ps20info.pstates [pstate].baseVoltages [volt].domainId ==
                       NVAPI_GPU_PERF_VOLTAGE_INFO_DOMAIN_CORE )
                  {
                    stats.gpus [i].volts_mV.supported = true;
                    stats.gpus [i].volts_mV.over      = false;

                    NV_GPU_PSTATE20_BASE_VOLTAGE_ENTRY_V1* voltage =
                      &ps20info.pstates [pstate].baseVoltages [volt];

                    stats.gpus [i].volts_mV.core = voltage->volt_uV/1000.0f;

                    int over  =
                      voltage->voltDelta_uV.value -
                      voltage->voltDelta_uV.valueRange.max;

                    int under =
                      voltage->voltDelta_uV.valueRange.min -
                      voltage->voltDelta_uV.value;

                    if (over > 0)
                    {
                      stats.gpus [i].volts_mV.ov   =   over  / 1000.0f;
                      stats.gpus [i].volts_mV.over = true;
                    }

                    else if (under > 0)
                    {
                      stats.gpus [i].volts_mV.ov   = -(under / 1000.0f);
                      stats.gpus [i].volts_mV.over = true;
                    }

                    else if (over != 0)
                    {
                      if (over > under)
                         stats.gpus [i].volts_mV.ov =  -(over  / 1000.0f);
                      else if (under > over)
                        stats.gpus [i].volts_mV.ov  =   (under / 1000.0f);

                      stats.gpus [i].volts_mV.over  = true;
                    }
                    break;
                  }
                }
                break;
              }
            }
          }
        }

        else has_pstates = false;

      //SwitchToThreadMinPageFaults ();
      }
    }

    else if (ADL_init == ADL_TRUE)
    {
      stats.num_gpus = SK_ADL_CountActiveGPUs ();

      //dll_log.Log (L"[DisplayLib] AMD GPUs: %i", stats.num_gpus);
      for (int i = 0; i < stats.num_gpus; i++)
      {
        AdapterInfo* pAdapter = SK_ADL_GetActiveAdapter (i);

        if (pAdapter->iAdapterIndex >= ADL_MAX_ADAPTERS || pAdapter->iAdapterIndex < 0)
        {
          dll_log.Log (L"[DisplayLib] INVALID ADL ADAPTER: %i", pAdapter->iAdapterIndex);
          break;
        }

        ADLPMActivity activity       = {                  };
                      activity.iSize = sizeof ADLPMActivity;

        ADL_Overdrive5_CurrentActivity_Get (pAdapter->iAdapterIndex, &activity);

        stats.gpus [i].loads_percent.gpu = activity.iActivityPercent;
        stats.gpus [i].hwinfo.pcie_gen   = activity.iCurrentBusSpeed;
        stats.gpus [i].hwinfo.pcie_lanes = activity.iCurrentBusLanes;

        stats.gpus [i].clocks_kHz.gpu    = activity.iEngineClock * 10UL;
        stats.gpus [i].clocks_kHz.ram    = activity.iMemoryClock * 10UL;

        // This rarely reads right on AMD's drivers and I don't have AMD hardware anymore, so ...
        //   disable it for now :)
        stats.gpus [i].volts_mV.supported = false;//true;


        stats.gpus [i].volts_mV.over      = false;
        stats.gpus [i].volts_mV.core      = static_cast <float> (activity.iVddc); // mV?

        ADLTemperature temp       = {                   };
                       temp.iSize = sizeof ADLTemperature;

        ADL_Overdrive5_Temperature_Get (pAdapter->iAdapterIndex, 0, &temp);

        stats.gpus [i].temps_c.gpu = temp.iTemperature / 1000UL;

        ADLFanSpeedValue fanspeed            = {                           };
                         fanspeed.iSize      = sizeof ADLFanSpeedValue;
                         fanspeed.iSpeedType = ADL_DL_FANCTRL_SPEED_TYPE_RPM;

        ADL_Overdrive5_FanSpeed_Get (pAdapter->iAdapterIndex, 0, &fanspeed);

        stats.gpus [i].fans_rpm.gpu       = fanspeed.iFanSpeed;
        stats.gpus [i].fans_rpm.supported = true;
      }
    }

    else
      break;

    gpu_stats = gpu_stats_buffers [ReadAcquire (&current_gpu_stat)];
  //ResetEvent (hPollEvent);
  }

  hPollThread = nullptr;

  CloseHandle (hPollEvent);
               hPollEvent = nullptr;

  SK_Thread_CloseSelf ();

  return 0;
}

void
SK_EndGPUPolling (void)
{
  if (hShutdownEvent != nullptr && hPollThread != nullptr)
  {
    if (SignalObjectAndWait (hShutdownEvent, hPollThread, 333UL, TRUE) != WAIT_OBJECT_0)
    {
      TerminateThread (hPollThread, 0x00);
      CloseHandle     (hPollThread); // Thread cleans itself up normally
    }

    CloseHandle (hShutdownEvent);

    if (hPollEvent != nullptr)
    {
      CloseHandle (hPollEvent);
                   hPollEvent = nullptr;
    }

    hShutdownEvent = nullptr;
    hPollThread    = nullptr;
  }
}

void
SK_PollGPU (void)
{
  if (! nvapi_init)
  {
    if (! ADL_init)
    {
      gpu_stats.num_gpus = 0;
      return;
    }
  }

  static volatile ULONG init = FALSE;

  if (! InterlockedCompareExchange (&init, TRUE, FALSE))
  {
    hShutdownEvent = SK_CreateEvent    (nullptr, FALSE, FALSE, nullptr);
    hPollEvent     = SK_CreateEvent    (nullptr, FALSE, TRUE,  nullptr);
    hPollThread    = SK_Thread_CreateEx ( SK_GPUPollingThread );
  }

  SYSTEMTIME     update_time;
  FILETIME       update_ftime;
  ULARGE_INTEGER update_ul;

  GetSystemTime        (&update_time);
  SystemTimeToFileTime (&update_time, &update_ftime);

  update_ul.HighPart = update_ftime.dwHighDateTime;
  update_ul.LowPart  = update_ftime.dwLowDateTime;

  const double dt =
    (update_ul.QuadPart - gpu_stats_buffers [0].last_update.QuadPart) * 1.0e-7;

  if (dt > config.gpu.interval)
  {
    if (SK_WaitForSingleObject (hPollEvent, 0) == WAIT_TIMEOUT)
    {
      gpu_stats_buffers [0].last_update.QuadPart = update_ul.QuadPart;

      if (hPollEvent != nullptr)
        SetEvent (hPollEvent);
    }
  }
}



uint32_t
__stdcall
SK_GPU_GetClockRateInkHz (int gpu)
{
  return (gpu > -1 && gpu < gpu_stats_buffers [ReadAcquire (&current_gpu_stat)].num_gpus)                  ?
                            gpu_stats_buffers [ReadAcquire (&current_gpu_stat)].gpus [gpu].clocks_kHz.gpu  :
                                              0;
}

uint32_t
__stdcall
SK_GPU_GetMemClockRateInkHz (int gpu)
{
  return (gpu > -1 && gpu < gpu_stats_buffers [ReadAcquire (&current_gpu_stat)].num_gpus)                  ?
                            gpu_stats_buffers [ReadAcquire (&current_gpu_stat)].gpus [gpu].clocks_kHz.ram  :
                                              0;
};

uint64_t
__stdcall
SK_GPU_GetMemoryBandwidth (int gpu)
{
  return (gpu > -1 && gpu < gpu_stats_buffers    [ReadAcquire (&current_gpu_stat)].num_gpus) ?
      (static_cast <uint64_t> (gpu_stats_buffers [ReadAcquire (&current_gpu_stat)].gpus [gpu].clocks_kHz.ram) * 2ULL * 1000ULL *
       static_cast <uint64_t> (gpu_stats_buffers [ReadAcquire (&current_gpu_stat)].gpus [gpu].hwinfo.mem_bus_width)) / 8 :
                                                 0;
}

float
__stdcall
SK_GPU_GetMemoryLoad (int gpu)
{
  return (gpu > -1 && gpu < gpu_stats_buffers [ReadAcquire (&current_gpu_stat)].num_gpus) ?
       static_cast <float> (gpu_stats_buffers [ReadAcquire (&current_gpu_stat)].gpus [gpu].loads_percent.fb) :
                                             0.0f;
}

float
__stdcall SK_GPU_GetGPULoad (int gpu)
{
  return (gpu > -1 && gpu < gpu_stats_buffers [ReadAcquire (&current_gpu_stat)].num_gpus) ?
       static_cast <float> (gpu_stats_buffers [ReadAcquire (&current_gpu_stat)].gpus [gpu].loads_percent.gpu) :
                                             0.0f;
}

float
__stdcall
SK_GPU_GetTempInC           (int gpu)
{
  return (gpu > -1 && gpu < gpu_stats_buffers [ReadAcquire (&current_gpu_stat)].num_gpus)               ?
       static_cast <float> (gpu_stats_buffers [ReadAcquire (&current_gpu_stat)].gpus [gpu].temps_c.gpu) :
                                             0.0f;
}

uint32_t
__stdcall
SK_GPU_GetFanSpeedRPM       (int gpu)
{
  return (gpu > -1 && gpu < gpu_stats_buffers [ReadAcquire (&current_gpu_stat)].num_gpus) ?
    gpu_stats_buffers   [ReadAcquire (&current_gpu_stat)].gpus [gpu].fans_rpm.supported   ?
      gpu_stats_buffers [ReadAcquire (&current_gpu_stat)].gpus [gpu].fans_rpm.gpu : 0 : 0;
}

uint64_t
__stdcall
SK_GPU_GetVRAMUsed          (int gpu)
{
  buffer_t buffer = mem_info [0].buffer;
  int      nodes  = mem_info [buffer].nodes;

  return ( gpu   > -1   && gpu < gpu_stats_buffers [ReadAcquire (&current_gpu_stat)].num_gpus )      ?
         ( nodes >= gpu ?  mem_info [buffer].local [gpu].CurrentUsage            :
                  gpu_stats_buffers [ReadAcquire (&current_gpu_stat)].gpus [gpu].memory_B.local )    :
                                     0;
}

uint64_t
__stdcall
SK_GPU_GetVRAMShared        (int gpu)
{
  buffer_t buffer = mem_info [0].buffer;
  int      nodes  = mem_info [buffer].nodes;

  return ( gpu   > -1   && gpu < gpu_stats_buffers [ReadAcquire (&current_gpu_stat)].num_gpus )      ?
         ( nodes >= gpu ?  mem_info [buffer].nonlocal [gpu].CurrentUsage         :
                  gpu_stats_buffers [ReadAcquire (&current_gpu_stat)].gpus [gpu].memory_B.nonlocal ) :
                                     0;
}

uint64_t
__stdcall
SK_GPU_GetVRAMCapacity      (int gpu)
{
  return (gpu > -1 && gpu < gpu_stats_buffers [ReadAcquire (&current_gpu_stat)].num_gpus)                    ?
                            gpu_stats_buffers [ReadAcquire (&current_gpu_stat)].gpus [gpu].memory_B.capacity :
                                              0;
}

uint64_t
__stdcall
SK_GPU_GetVRAMBudget        (int gpu)
{
  buffer_t buffer = mem_info [0].buffer;
  int      nodes  = mem_info [buffer].nodes;

  return ( gpu   > -1   && gpu < gpu_stats_buffers [ReadAcquire (&current_gpu_stat)].num_gpus ) ?
         ( nodes >= gpu ?  mem_info [buffer].local [gpu].Budget                                 :
                            SK_GPU_GetVRAMCapacity (gpu) )                                      :
                                     0;
}