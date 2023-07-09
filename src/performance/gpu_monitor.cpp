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

#include <SpecialK/nvapi.h>
#include <SpecialK/adl.h>
#include <Pdh.h>

#pragma comment (lib, "pdh.lib")

// D3DKMT stuff
#include <SpecialK/render/d3d11/d3d11_core.h>

static constexpr int GPU_SENSOR_BUFFERS = 3;

//
// All GPU performance data are double-buffered and it is safe to read
//   gpu_stats without any thread synchronization.
//
//  >> It will always reference the last complete set of performance data.
//
volatile LONG           gpu_sensor_frame  =  0;
         // Double-buffered updates
         gpu_sensors_t* gpu_stats_buffers = nullptr;
volatile gpu_sensors_t* gpu_stats         = nullptr;
         // ^^^ ptr to front

//#define PCIE_WORKS

extern BOOL ADL_init;

#define NVAPI_GPU_UTILIZATION_DOMAIN_GPU 0
#define NVAPI_GPU_UTILIZATION_DOMAIN_FB  1
#define NVAPI_GPU_UTILIZATION_DOMAIN_VID 2
#define NVAPI_GPU_UTILIZATION_DOMAIN_BUS 3

#define D3DKMT_SENSORS 1

static SK_AutoHandle hPollEvent;
static SK_AutoHandle hShutdownEvent;
static SK_AutoHandle hPollThread;

void
SK_GPU_InitSensorData (void)
{
  if (gpu_stats_buffers == nullptr)
      gpu_stats_buffers = new gpu_sensors_t [GPU_SENSOR_BUFFERS] { };

  // Num GPUs will be 0 unless NVAPI, ADL or D3DKMT is supported.
#ifndef D3DKMT_SENSORS
  if ( nvapi_init || ADL_init )
#endif
  {
    for ( int i = 0 ; i < GPU_SENSOR_BUFFERS ; ++i )
    { gpu_stats_buffers [i].num_gpus = 1; }
  }

  InterlockedExchangePointer (
    (void **)&gpu_stats,
             &gpu_stats_buffers [0]
  );
}


SK_LazyGlobal <
  std::vector <
    std::pair < NvU32, NvPhysicalGpuHandle >
              >
              > __nv_gpus;

DWORD
__stdcall
SK_GPUPollingThread (LPVOID user)
{
  auto nv_gpus =
     __nv_gpus.getPtr ();

  SK_GPU_InitSensorData ();

  std::ignore = user;

  const HANDLE hEvents [2] = {
    hPollEvent,
    hShutdownEvent
  };

  SetCurrentThreadDescription  (L"[SK] GPU Performance Monitor");
  SK_Thread_SetCurrentPriority (THREAD_PRIORITY_BELOW_NORMAL);
  SetThreadPriorityBoost       (GetCurrentThread (),   FALSE);


  auto SwitchToThreadMinPageFaults = [](void) ->
  void
  {
    static int iters = 0;

    if ((iters++ % 13) == 0)
      SK_Sleep (1);

    else if ((iters % 9) != 0)
      SwitchToThread ();
  };


  NvPhysicalGpuHandle gpus [NVAPI_MAX_PHYSICAL_GPUS] = { };
  NvU32               gpu_count                      =  0;


  if (nvapi_init)
  {
#ifdef _DEBUG
    SK_RunOnce (assert (NvAPI_GetGPUIDFromPhysicalGPU != nullptr));
    SK_RunOnce (assert (NvAPI_GetPhysicalGPUFromGPUID != nullptr));
#endif

    NvAPI_EnumPhysicalGPUs (gpus, &gpu_count);
  }

  while (true)
  {
    DWORD dwWait =
      WaitForMultipleObjectsEx ( 2, hEvents, FALSE,
                                   INFINITE, FALSE );

    if (     dwWait == WAIT_OBJECT_0 + 1) break;
    else if (dwWait != WAIT_OBJECT_0    ) break;

    static LONG idx =
      ( InterlockedIncrement (&gpu_sensor_frame) % GPU_SENSOR_BUFFERS );

    // Previous data set, for stats that are not updated every cycle
    gpu_sensors_t& stats0 =
               gpu_stats_buffers [idx];

    idx =
      ( InterlockedIncrement (&gpu_sensor_frame) % GPU_SENSOR_BUFFERS );

    // The data set we will be working on.
    gpu_sensors_t& stats =
               gpu_stats_buffers [idx];

    // Don't do this every frame
    if ((ReadAcquire (&gpu_sensor_frame) % (GPU_SENSOR_BUFFERS * 2)) == 0)
    {
      extern void SK_DXGI_SignalBudgetThread (void);
                  SK_DXGI_SignalBudgetThread (    );
    }

    static bool bHadFanRPM     = false;
    static bool bHadVoltage    = false;
           bool bHadPercentage = false;

    if (nvapi_init && gpu_count != 0)
    {
      stats.num_gpus =
        std::min (gpu_count, static_cast <NvU32> (NVAPI_MAX_PHYSICAL_GPUS));

      NV_GPU_DYNAMIC_PSTATES_INFO_EX
        psinfoex         = {                                };
        psinfoex.version = NV_GPU_DYNAMIC_PSTATES_INFO_EX_VER;

      if (nv_gpus->empty ())
      {
        for (int i = 0; i < stats.num_gpus; i++)
        {
          NvU32 gpu_id = 0;

          if (NvAPI_GetGPUIDFromPhysicalGPU != nullptr)
              NvAPI_GetGPUIDFromPhysicalGPU ( gpus [i], &gpu_id );

          NvPhysicalGpuHandle gpu = { };

          if (NvAPI_GetPhysicalGPUFromGPUID != nullptr)
              NvAPI_GetPhysicalGPUFromGPUID ( gpu_id, &gpu );

          nv_gpus->emplace_back (
            std::make_pair (gpu_id, gpu)
          );
        }

        std::sort ( nv_gpus->begin (),
                    nv_gpus->end   (),
          [&]( const std::pair <NvU32, NvPhysicalGpuHandle>& a,
               const std::pair <NvU32, NvPhysicalGpuHandle>& b )
          {
            return ( a.first < b.first );
          }
        );
      }

      for (int i = 0; i < stats.num_gpus; i++)
      {
        // In order for DXGI Adapter info to match up... don't just assign
        //   these GPUs wily-nilly, use the high 24-bits of the GPUID as
        //     a bitmask.
        //NvAPI_GetPhysicalGPUFromGPUID (1 << (i + 8), &gpu);
        //stats.gpus [i].nv_gpuid = (1 << (i + 8));

        NvPhysicalGpuHandle gpu = __nv_gpus [i].second;
        stats.gpus [i].nv_gpuid = __nv_gpus [i].first;

        NvAPI_Status
              status = NVAPI_OK;

        if (stats.gpus [i].amortization.phase0 % 2 == 0)
        {
          NvAPI_GPU_GetDynamicPstatesInfoEx (gpu, &psinfoex);

          if (status == NVAPI_OK)
          {
            stats.gpus [i].loads_percent.gpu =
              (psinfoex.utilization [NVAPI_GPU_UTILIZATION_DOMAIN_GPU].percentage * 1000 + stats0.gpus [i].loads_percent.gpu) / 2;
            stats.gpus [i].loads_percent.fb =
              (psinfoex.utilization [NVAPI_GPU_UTILIZATION_DOMAIN_FB].percentage * 1000 + stats0.gpus [i].loads_percent.fb)   / 2;
            stats.gpus [i].loads_percent.vid =
              (psinfoex.utilization [NVAPI_GPU_UTILIZATION_DOMAIN_VID].percentage * 1000 + stats0.gpus [i].loads_percent.vid) / 2;
            stats.gpus [i].loads_percent.bus =
              (psinfoex.utilization [NVAPI_GPU_UTILIZATION_DOMAIN_BUS].percentage * 1000 + stats0.gpus [i].loads_percent.bus) / 2;

            bHadPercentage = true;
          }

          else
          {
            stats.gpus [i].loads_percent.gpu = stats0.gpus [i].loads_percent.gpu;
            stats.gpus [i].loads_percent.fb  = stats0.gpus [i].loads_percent.fb;
            stats.gpus [i].loads_percent.vid = stats0.gpus [i].loads_percent.vid;
            stats.gpus [i].loads_percent.bus = stats0.gpus [i].loads_percent.bus;
          }

          NV_GPU_THERMAL_SETTINGS
            thermal         = {                         };
            thermal.version = NV_GPU_THERMAL_SETTINGS_VER;


          // This is needed to fix some problems with Mirilis Action injecting itself
          //   and immediately breaking NvAPI.
          if (stats.gpus [i].temps_c.supported)
          {
            status =
              NvAPI_GPU_GetThermalSettings ( gpu,
                                               NVAPI_THERMAL_TARGET_ALL,
                                                 &thermal );

            if (status != NVAPI_OK)
                  stats.gpus [i].temps_c.supported = false;
          } if (! stats.gpus [i].temps_c.supported)
                status  = NVAPI_ERROR;


          if (status == NVAPI_OK)
          {
            for (NvU32 j = 0; j < std::min (3UL, thermal.count); j++)
            {
#ifdef SMOOTH_GPU_UPDATES
              if (thermal.sensor [j].target == NVAPI_THERMAL_TARGET_GPU)
                stats.gpus [i].temps_c.gpu = (stats.gpus[i].temps_c.gpu + thermal.sensor [j].currentTemp) / 2;
#else
              if (thermal.sensor [j].target == NVAPI_THERMAL_TARGET_GPU)
                stats.gpus [i].temps_c.gpu =
                        static_cast <float> (thermal.sensor [j].currentTemp);
              if (thermal.sensor [j].target == NVAPI_THERMAL_TARGET_MEMORY)
                stats.gpus [i].temps_c.ram = thermal.sensor [j].currentTemp;
              if (thermal.sensor [j].target == NVAPI_THERMAL_TARGET_POWER_SUPPLY)
                stats.gpus [i].temps_c.psu = thermal.sensor [j].currentTemp;
              if (thermal.sensor [j].target == NVAPI_THERMAL_TARGET_BOARD)
                stats.gpus [i].temps_c.pcb = thermal.sensor [j].currentTemp;
#endif
            }
          }

          else
            stats.gpus [i].temps_c.supported = false;
        }

        // Reuse last sampled values
        //
        else
        {
          stats.gpus [i].loads_percent.gpu = stats0.gpus [i].loads_percent.gpu;
          stats.gpus [i].loads_percent.fb  = stats0.gpus [i].loads_percent.fb;
          stats.gpus [i].loads_percent.vid = stats0.gpus [i].loads_percent.vid;
          stats.gpus [i].loads_percent.bus = stats0.gpus [i].loads_percent.bus;

          stats.gpus [i].temps_c.gpu = stats0.gpus [i].temps_c.gpu;
          stats.gpus [i].temps_c.ram = stats0.gpus [i].temps_c.ram;
          stats.gpus [i].temps_c.psu = stats0.gpus [i].temps_c.psu;
          stats.gpus [i].temps_c.pcb = stats0.gpus [i].temps_c.pcb;

          bHadPercentage = true;
        }


        if (stats.gpus [i].amortization.phase0 % 3 == 0)
        {
          NvU32                                                          pcie_lanes = 0;
          if (NVAPI_OK == NvAPI_GPU_GetCurrentPCIEDownstreamWidth (gpu, &pcie_lanes))
                                      stats.gpus [i].hwinfo.pcie_lanes = pcie_lanes;

          NV_PCIE_INFO
             pcieinfo         = {              };
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

          else {
            stats.gpus [i].hwinfo.pcie_gen           = stats0.gpus [i].hwinfo.pcie_gen;
            stats.gpus [i].hwinfo.pcie_lanes         = stats0.gpus [i].hwinfo.pcie_lanes;
            stats.gpus [i].hwinfo.pcie_transfer_rate = stats0.gpus [i].hwinfo.pcie_transfer_rate;
          }

          NvU32 mem_width = 0,
                mem_loc   = 0,
                mem_type  = 0;

          if (            NvAPI_GPU_GetFBWidthAndLocation != nullptr &&
              NVAPI_OK == NvAPI_GPU_GetFBWidthAndLocation (gpu, &mem_width, &mem_loc))
                           stats.gpus [i].hwinfo.mem_bus_width = mem_width;

          else stats.gpus [i].hwinfo.mem_bus_width =
              stats0.gpus [i].hwinfo.mem_bus_width;


          if (            NvAPI_GPU_GetRamType != nullptr &&
              NVAPI_OK == NvAPI_GPU_GetRamType (gpu, &mem_type))
                     stats.gpus [i].hwinfo.mem_type = mem_type;

          else stats.gpus [i].hwinfo.mem_type =
              stats0.gpus [i].hwinfo.mem_type;

          NV_GPU_MEMORY_INFO_EX
            meminfo         = {                         };
            meminfo.version = NV_GPU_MEMORY_INFO_EX_VER_1;

          if (NVAPI_OK == NvAPI_GPU_GetMemoryInfoEx (gpu, &meminfo))
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
             ( stats.gpus [i].memory_B.total - stats.gpus [i].memory_B.local );
          }

          else
          {
            stats.gpus [i].memory_B.local    = stats0.gpus [i].memory_B.local;
            stats.gpus [i].memory_B.capacity = stats0.gpus [i].memory_B.capacity;

            stats.gpus [i].memory_B.total    = stats0.gpus [i].memory_B.total;
            stats.gpus [i].memory_B.nonlocal = stats0.gpus [i].memory_B.nonlocal;
          }
        }

        // Reuse last sampled values
        //
        else
        {
          stats.gpus [i].hwinfo.pcie_gen           = stats0.gpus [i].hwinfo.pcie_gen;
          stats.gpus [i].hwinfo.pcie_lanes         = stats0.gpus [i].hwinfo.pcie_lanes;
          stats.gpus [i].hwinfo.pcie_transfer_rate = stats0.gpus [i].hwinfo.pcie_transfer_rate;

          stats.gpus [i].memory_B.local            = stats0.gpus [i].memory_B.local;
          stats.gpus [i].memory_B.capacity         = stats0.gpus [i].memory_B.capacity;

          stats.gpus [i].memory_B.total            = stats0.gpus [i].memory_B.total;
          stats.gpus [i].memory_B.nonlocal         = stats0.gpus [i].memory_B.nonlocal;
        }

      //SwitchToThreadMinPageFaults ();

        if (stats.gpus [i].amortization.phase0++ % 3 == 0)
        {
          NV_GPU_CLOCK_FREQUENCIES
            freq           = {                          };
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

          else
          {
            stats.gpus [i].clocks_kHz.gpu = stats0.gpus [i].clocks_kHz.gpu;
            stats.gpus [i].clocks_kHz.ram = stats0.gpus [i].clocks_kHz.ram;
          }
        }

        // Reuse last sampled values
        //
        else
        {
          stats.gpus [i].clocks_kHz.gpu = stats0.gpus [i].clocks_kHz.gpu;
          stats.gpus [i].clocks_kHz.ram = stats0.gpus [i].clocks_kHz.ram;
        }

        if (stats.gpus [i].amortization.phase1++ % 2 == 0)
        {
          NvU32 tach = 0;

          stats.gpus [i].fans_rpm.supported = false;

          if (NVAPI_OK == NvAPI_GPU_GetTachReading (gpu, &tach))
          {
            stats.gpus [i].fans_rpm.gpu       = tach;
            stats.gpus [i].fans_rpm.supported = true;

            bHadFanRPM = true;
          }

          else
          {
            stats.gpus [i].fans_rpm.gpu       = 0;
            stats.gpus [i].fans_rpm.supported = false;

            bHadFanRPM = false;
          }
        }

        // Reuse last sampled values
        //
        else
        {
          stats.gpus [i].fans_rpm.gpu       = stats0.gpus [i].fans_rpm.gpu;
          stats.gpus [i].fans_rpm.supported = stats0.gpus [i].fans_rpm.supported;
        }

        // This is an expensive operation for very little gain,
        //   it rarely changes, but it eats CPU time.
        if ((stats.gpus [i].nv_perf_state_iter++ % 8) == 0 && config.gpu.print_slowdown)
        {
          NvU32                                                perf_decrease_info = 0;
          if (NVAPI_OK == NvAPI_GPU_GetPerfDecreaseInfo (gpu, &perf_decrease_info))
                          stats.gpus [i].nv_perf_state =       perf_decrease_info;
        }

        else stats.gpus [i].nv_perf_state =
            stats0.gpus [i].nv_perf_state;

        NV_GPU_PERF_PSTATES20_INFO
          ps20info         = {                            };
          ps20info.version = NV_GPU_PERF_PSTATES20_INFO_VER;

        stats.gpus [i].volts_mV.supported = false;

        if ( stats.gpus   [i].queried_nv_pstates ||
             ( stats.gpus [i].    has_nv_pstates &&
               NVAPI_OK == NvAPI_GPU_GetPstates20 (gpu, &ps20info) )
           )
        {
          // Rather than querying the list over and over, just get the current
          //   index into the list.
          stats.gpus [i].queried_nv_pstates = true;
          stats.gpus [i].has_nv_pstates     = (ps20info.numPstates != 0);

          NV_GPU_PERF_PSTATE_ID
              current_pstate = NVAPI_GPU_PERF_PSTATE_P0;

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

                      bHadVoltage = true;

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

                    bHadVoltage = true;

                    break;
                  }
                }
                break;
              }
            }
          }
        }

        else stats.gpus [i].has_nv_pstates = false;

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
          dll_log->Log (L"[DisplayLib] INVALID ADL ADAPTER: %i", pAdapter->iAdapterIndex);
          break;
        }

        ADLPMActivity activity       = {                  };

        activity.iSize = sizeof (ADLPMActivity);

        ADL_Overdrive5_CurrentActivity_Get (pAdapter->iAdapterIndex, &activity);

        stats.gpus [i].loads_percent.gpu = ((activity.iActivityPercent * 1000) + stats0.gpus [i].loads_percent.gpu) / 2;
        stats.gpus [i].hwinfo.pcie_gen   = activity.iCurrentBusSpeed;
        stats.gpus [i].hwinfo.pcie_lanes = activity.iCurrentBusLanes;

        stats.gpus [i].clocks_kHz.gpu    = activity.iEngineClock * 10UL;
        stats.gpus [i].clocks_kHz.ram    = activity.iMemoryClock * 10UL;

        // This rarely reads right on AMD's drivers and I don't have AMD hardware anymore, so ...
        //   disable it for now :)
        stats.gpus [i].volts_mV.supported = false;//true;


        stats.gpus [i].volts_mV.over      = false;
        stats.gpus [i].volts_mV.core      = static_cast <float> (activity.iVddc); // mV?

        bHadVoltage = stats.gpus [i].volts_mV.core != 0;

        ADLTemperature temp       = {                     };
                       temp.iSize = sizeof (ADLTemperature);

        ADL_Overdrive5_Temperature_Get (pAdapter->iAdapterIndex, 0, &temp);

        stats.gpus [i].temps_c.gpu = static_cast <float> (temp.iTemperature) / 1000.0f;

        ADLFanSpeedValue fanspeed            = {                           };
                         fanspeed.iSize      = sizeof (ADLFanSpeedValue);
                         fanspeed.iSpeedType = ADL_DL_FANCTRL_SPEED_TYPE_RPM;

        ADL_Overdrive5_FanSpeed_Get (pAdapter->iAdapterIndex, 0, &fanspeed);

        stats.gpus [i].fans_rpm.gpu       = fanspeed.iFanSpeed;
        stats.gpus [i].fans_rpm.supported = fanspeed.iFanSpeed != 0;

        if (fanspeed.iFanSpeed != 0)
          bHadFanRPM = true;
        else
          bHadFanRPM = false;
      }

      if (stats.gpus [0].loads_percent.gpu != 0)
        bHadPercentage = true;
    }

    static auto &rb =
      SK_GetCurrentRenderBackend ();

    static D3DKMT_ADAPTER_PERFDATA
                   adapterPerfData = { };

    if (rb.adapter.d3dkmt != 0)
    {
      if (rb.adapter.perf.sampled_frame < SK_GetFramesDrawn ())
      {   rb.adapter.perf.sampled_frame = 0;
        D3DKMT_ADAPTER_PERFDATA perf_data = { };
        D3DKMT_NODE_PERFDATA    node_data = { };

        D3DKMT_QUERYADAPTERINFO
               queryAdapterInfo                       = { };
               queryAdapterInfo.AdapterHandle         = rb.adapter.d3dkmt;
               queryAdapterInfo.Type                  = KMTQAITYPE_ADAPTERPERFDATA;
               queryAdapterInfo.PrivateDriverData     = &perf_data;
               queryAdapterInfo.PrivateDriverDataSize = sizeof (D3DKMT_ADAPTER_PERFDATA);

        // General adapter perf data
        //
        if (SUCCEEDED (SK_D3DKMT_QueryAdapterInfo (&queryAdapterInfo)))
        {
          memcpy ( &rb.adapter.perf.data, queryAdapterInfo.PrivateDriverData,
                        std::min ((size_t)queryAdapterInfo.PrivateDriverDataSize,
                                             sizeof (D3DKMT_ADAPTER_PERFDATA)) );

          rb.adapter.perf.sampled_frame =
                            SK_GetFramesDrawn ();
        }

        static UINT32 Engine3DNodeOrdinal  = 0;
        static UINT32 Engine3DAdapterIndex = 0;

        SK_RunOnce (
        {
          const UINT NodeCount = 32;

          // NOTE: The proper way to get the node count is using
          //         D3DKMT_QUERYSTATISTICS_PROCESS_ADAPTER_INFORMATION
          for ( UINT i = 0 ; i < NodeCount ; ++i )
          {
            D3DKMT_NODEMETADATA metaData = { MAKEWORD (i, 0) };

            queryAdapterInfo.Type                  = KMTQAITYPE_NODEMETADATA;
            queryAdapterInfo.PrivateDriverData     = &metaData;
            queryAdapterInfo.PrivateDriverDataSize = sizeof (D3DKMT_NODEMETADATA);

            if (SUCCEEDED (SK_D3DKMT_QueryAdapterInfo (&queryAdapterInfo)))
            {
              if (metaData.NodeData.EngineType == DXGK_ENGINE_TYPE_3D)
              {
                Engine3DNodeOrdinal  = i;
                Engine3DAdapterIndex = 0;
                break;
              }
            }
          }
        });

        node_data.NodeOrdinal = Engine3DNodeOrdinal;

        queryAdapterInfo.AdapterHandle         = rb.adapter.d3dkmt;
        queryAdapterInfo.Type                  = KMTQAITYPE_NODEPERFDATA;
        queryAdapterInfo.PrivateDriverData     = &node_data;
        queryAdapterInfo.PrivateDriverDataSize = sizeof (D3DKMT_NODE_PERFDATA);

        // 3D Engine-specific (i.e. GPU clock / voltage)
        //
        if (SUCCEEDED (SK_D3DKMT_QueryAdapterInfo (&queryAdapterInfo)))
        {
          memcpy ( &rb.adapter.perf.engine_3d, queryAdapterInfo.PrivateDriverData,
                             std::min ((size_t)queryAdapterInfo.PrivateDriverDataSize,
                                             sizeof (D3DKMT_NODE_PERFDATA)) );
        }
      }
    }

    // Fallback to Performance Data Helper because ADL or NVAPI were no help
    if (! bHadPercentage)
    {
      PDH_STATUS status;

      static HQUERY   query   = nullptr;
      static HCOUNTER counter = nullptr;

      if (counter == nullptr && rb.adapter.luid.LowPart != 0 && SK_GetFramesDrawn () > 30)
      {
        SK_RunOnce ({
          extern HRESULT
            ModifyPrivilege ( IN LPCTSTR szPrivilege,
                              IN BOOL     fEnable );

          ModifyPrivilege (SE_SYSTEM_PROFILE_NAME, TRUE);
          ModifyPrivilege (SE_CREATE_TOKEN_NAME,   TRUE);
        });

        if (query == nullptr)
        {
          status =
            PdhOpenQuery (nullptr, 0, &query);
        }

        if (query != nullptr)
        {
          const auto counter_path =
            SK_FormatStringW (
                LR"(\GPU Engine(*luid_0x%08X_0x%08X*engtype_3D)\Utilization Percentage)",
                      rb.adapter.luid.HighPart,
                      rb.adapter.luid.LowPart );

          status =
            PdhAddCounter (query, counter_path.c_str (), 0, &counter);

          if ( ERROR_SUCCESS !=
                 PdhCollectQueryData (query)
             )
          {
            PdhRemoveCounter (counter);
            PdhCloseQuery    (query);

            counter = nullptr;
            query   = nullptr;
          }
        }
      }

      if (counter != nullptr)
      {
        status =
          PdhCollectQueryData (query);

        if (status == ERROR_SUCCESS)
        {
          PDH_FMT_COUNTERVALUE counterValue = { };

          status =
            PdhGetFormattedCounterValue (
              counter, PDH_FMT_DOUBLE, nullptr, &counterValue
            );

          // Clamp to 0.1% minimum load and apply smoothing between updates, because Pdh's load%
          //   counter has very high variance and we want to avoid 0.0%
          stats.gpus [0].loads_percent.gpu =
            ( stats0.gpus [0].loads_percent.gpu +
              static_cast <uint32_t> (std::max (100.0, counterValue.doubleValue * 1000.0)) ) / 2;
        }
      }

      stats.num_gpus = 1;

      for (int i = 0; i < stats.num_gpus; i++)
      {
        stats.gpus [i/*adapterPerfData.PhysicalAdapterIndex*/].temps_c.gpu =
            static_cast <float> (rb.adapter.perf.data.Temperature) / 10.0f;

        stats.gpus [i].fans_rpm.supported = (rb.adapter.perf.data.FanRPM != 0);
        stats.gpus [i].fans_rpm.gpu       =
          std::max (0UL, (                   rb.adapter.perf.data.FanRPM + stats0.gpus [i].fans_rpm.gpu) / 2UL);

        stats.gpus [i].clocks_kHz.ram     =
          static_cast <uint32_t> (
            rb.adapter.perf.data.MemoryFrequency / 1000
          );

        // Smooth clock frequency changes
        stats.gpus [i].clocks_kHz.ram =
         ( stats.gpus [i].clocks_kHz.ram +
          stats0.gpus [i].clocks_kHz.ram ) / 2;

        stats.gpus [i].clocks_kHz.gpu     =
          static_cast <uint32_t> (
            rb.adapter.perf.engine_3d.Frequency / 1000
          );

        // Smooth clock frequency changes
        stats.gpus [i].clocks_kHz.gpu =
         ( stats.gpus [i].clocks_kHz.gpu +
          stats0.gpus [i].clocks_kHz.gpu ) / 2;

        stats.gpus [i].volts_mV.core      =
                       static_cast <float> (rb.adapter.perf.engine_3d.Voltage);
        stats.gpus [i].volts_mV.supported = rb.adapter.perf.engine_3d.Voltage != 0;
      }
    }

    //
    // Add data that might be missing from NVAPI
    //

    if (! bHadFanRPM)
    {   stats.gpus [0].fans_rpm.supported = (rb.adapter.perf.data.FanRPM != 0);
        stats.gpus [0].fans_rpm.gpu       = (rb.adapter.perf.data.FanRPM + stats0.gpus [0].fans_rpm.gpu) / 2;
    }
    if (! bHadVoltage)
    {   stats.gpus [0].volts_mV.core      =
                      (static_cast <float> (rb.adapter.perf.engine_3d.Voltage) + stats0.gpus [0].volts_mV.core) / 2.0f;
        stats.gpus [0].volts_mV.supported = rb.adapter.perf.engine_3d.Voltage != 0;
    }

    // Favor D3DKMT Temperature Measurement if it is providing non-zero data
    if ((rb.adapter.perf.data.Temperature) / 10.0f > 0.1f)
    {
      stats.gpus [0].temps_c.gpu =
        ((static_cast <float> (rb.adapter.perf.data.Temperature) / 10.0f) + stats0.gpus [0].temps_c.gpu) / 2.0f;
    }

    InterlockedExchangePointer (
      (void **)&gpu_stats,
               &gpu_stats_buffers [idx]
    );

    ResetEvent (hPollEvent);
  }

  InterlockedExchangePointer (
    (void **)& gpu_stats, nullptr
  );

  SK_Thread_CloseSelf ();

  return 0;
}

void
SK_EndGPUPolling (void)
{
  if (hShutdownEvent != nullptr && hPollThread != nullptr)
  {
    if (SignalObjectAndWait (hShutdownEvent, hPollThread, 125UL, FALSE) != WAIT_OBJECT_0)
    {
      SK_TerminateThread (hPollThread, 0x00);
      SK_CloseHandle      (hPollThread); // Thread cleans itself up normally
    }

    hPollEvent.Close     ();
    hShutdownEvent.Close ();
    hPollThread.Close    ();
  }

  if (ReadAcquire (&__SK_DLL_Ending))
  {
    gpu_sensors_t* to_delete = nullptr;
    std::swap     (to_delete, gpu_stats_buffers);

    delete to_delete;
  }
}

void
SK_PollGPU (void)
{
  static volatile ULONG              init       (FALSE);
  if (! InterlockedCompareExchange (&init, TRUE, FALSE))
  {
    hShutdownEvent.Attach (SK_CreateEvent     (nullptr, TRUE, FALSE, nullptr));
    hPollEvent.Attach     (SK_CreateEvent     (nullptr, TRUE,  TRUE, nullptr));
    hPollThread.Attach    (SK_Thread_CreateEx ( SK_GPUPollingThread ));
  }

  gpu_sensors_t* pSensors =
    (gpu_sensors_t *)ReadPointerAcquire ((volatile PVOID *)&gpu_stats);

  while (pSensors == nullptr)
  {
    YieldProcessor ();

    static int spins = 0;
    if (     ++spins > 16)
    {          spins = 0; return; }

    pSensors =
      (gpu_sensors_t *)ReadPointerAcquire ((volatile PVOID *)&gpu_stats);
  }

  ULARGE_INTEGER         update_ul   = { };
  SYSTEMTIME             update_time = { };
  GetSystemTime        (&update_time);
  FILETIME                             update_ftime = { };
  SystemTimeToFileTime (&update_time, &update_ftime);
  update_ul.HighPart =                 update_ftime.dwHighDateTime;
  update_ul.LowPart  =                 update_ftime.dwLowDateTime;

  const double dt =
    (update_ul.QuadPart - pSensors->last_update.QuadPart) * 1.0e-7;

  if ( dt > config.gpu.interval && hPollEvent != nullptr )
  {
    if ( WAIT_TIMEOUT ==
           SK_WaitForSingleObject (hPollEvent, 0)
       )
    {
      if ( WAIT_OBJECT_0 !=
             SK_WaitForSingleObject (hShutdownEvent, 0) )
      {
        pSensors->last_update.QuadPart =
                    update_ul.QuadPart;

        if (        hPollEvent != nullptr)
          SetEvent (hPollEvent);
      }
    }
  }
}


gpu_sensors_t*
SK_GPU_CurrentSensorData (void)
{
  if (ReadPointerAcquire ((volatile PVOID *)&gpu_stats) == nullptr)
  {
    static gpu_sensors_t
            nul_sensors = { };
    return &nul_sensors;
  }

  return
    (gpu_sensors_t *)(ReadPointerAcquire ((volatile PVOID *)&gpu_stats));
}

uint32_t
__stdcall
SK_GPU_GetClockRateInkHz (int gpu)
{
  const gpu_sensors_t* pDataView =
    SK_GPU_CurrentSensorData ();

  return (gpu > -1 && gpu < pDataView->num_gpus)                  ?
                            pDataView->gpus [gpu].clocks_kHz.gpu  :
                                    0;
}

uint32_t
__stdcall
SK_GPU_GetMemClockRateInkHz (int gpu)
{
  const gpu_sensors_t* pDataView =
    SK_GPU_CurrentSensorData ();

  return (gpu > -1 && gpu < pDataView->num_gpus)                  ?
                            pDataView->gpus [gpu].clocks_kHz.ram  :
                                    0;
};

uint64_t
__stdcall
SK_GPU_GetMemoryBandwidth (int gpu)
{
  const gpu_sensors_t* pDataView =
    SK_GPU_CurrentSensorData ();

  return (gpu > -1 && gpu <    pDataView->num_gpus)                             ?
      (static_cast <uint64_t> (pDataView->gpus [gpu].clocks_kHz.ram) * 2ULL * 1000ULL *
       static_cast <uint64_t> (pDataView->gpus [gpu].hwinfo.mem_bus_width)) / 8 :
                                    0;
}

float
__stdcall
SK_GPU_GetMemoryLoad (int gpu)
{
  const gpu_sensors_t* pDataView =
    SK_GPU_CurrentSensorData ();

  return (gpu > -1 && gpu < pDataView->num_gpus)                              ?
       static_cast <float> (pDataView->gpus [gpu].loads_percent.fb) / 1000.0f :
                                    0.0f;
}

float
__stdcall SK_GPU_GetGPULoad (int gpu)
{
  const gpu_sensors_t* pDataView =
    SK_GPU_CurrentSensorData ();

  return (gpu > -1 && gpu < pDataView->num_gpus)                               ?
       static_cast <float> (pDataView->gpus [gpu].loads_percent.gpu) / 1000.0f :
                                    0.0f;
}

float
__stdcall
SK_GPU_GetTempInC           (int gpu)
{
  const gpu_sensors_t* pDataView =
    SK_GPU_CurrentSensorData ();

  return (gpu > -1 && gpu < pDataView->num_gpus)              ?
                            pDataView->gpus [gpu].temps_c.gpu :
                                    0.0f;
}

uint32_t
__stdcall
SK_GPU_GetFanSpeedRPM       (int gpu)
{
  const gpu_sensors_t* pDataView =
    SK_GPU_CurrentSensorData ();

  return (gpu > -1 && pDataView != nullptr && gpu < pDataView->num_gpus) ?
                      pDataView->gpus [gpu].fans_rpm.supported           ?
                      pDataView->gpus [gpu].fans_rpm.gpu : 0 : 0;
}

uint64_t
__stdcall
SK_GPU_GetVRAMUsed          (int gpu)
{
  const gpu_sensors_t* pDataView =
    SK_GPU_CurrentSensorData ();

  buffer_t buffer = dxgi_mem_info [0].buffer;
  int      nodes  = dxgi_mem_info [buffer].nodes;

  return ( gpu   > -1   && gpu < pDataView->num_gpus )                         ?
         ( nodes >= gpu ?  dxgi_mem_info [buffer].local [gpu].CurrentUsage     :
                                        pDataView->gpus [gpu].memory_B.local ) :
                                    0;
}

uint64_t
__stdcall
SK_GPU_GetVRAMShared        (int gpu)
{
  const gpu_sensors_t* pDataView =
    SK_GPU_CurrentSensorData ();

  buffer_t buffer = dxgi_mem_info [0].buffer;
  int      nodes  = dxgi_mem_info [buffer].nodes;

  return ( gpu   > -1   && gpu < pDataView->num_gpus )                               ?
         ( nodes >= gpu ?  dxgi_mem_info [buffer].nonlocal [gpu].CurrentUsage        :
                                           pDataView->gpus [gpu].memory_B.nonlocal ) :
                                    0;
}

uint64_t
__stdcall
SK_GPU_GetVRAMCapacity      (int gpu)
{
  const gpu_sensors_t* pDataView =
    SK_GPU_CurrentSensorData ();

  return (gpu > -1 && gpu < pDataView->num_gpus)                    ?
                            pDataView->gpus [gpu].memory_B.capacity :
                                    0;
}

uint64_t
__stdcall
SK_GPU_GetVRAMBudget        (int gpu)
{
  const gpu_sensors_t* pDataView =
    SK_GPU_CurrentSensorData ();

  buffer_t buffer = dxgi_mem_info [0].buffer;
  int      nodes  = dxgi_mem_info [buffer].nodes;

  return ( gpu   > -1   && gpu < pDataView->num_gpus ) ?
         ( nodes >= gpu ?  dxgi_mem_info [buffer].local [gpu].Budget :
                            SK_GPU_GetVRAMCapacity (gpu) )           :
                                    0;
}