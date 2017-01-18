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

#include <SpecialK/gpu_monitor.h>
#include <SpecialK/config.h>

#include <SpecialK/log.h>

#include <algorithm>
#include <vector>

gpu_sensors_t gpu_stats;

#include <SpecialK/nvapi.h>
extern BOOL nvapi_init;

//#define PCIE_WORKS

#include <SpecialK/adl.h>
extern BOOL ADL_init;

#define NVAPI_GPU_UTILIZATION_DOMAIN_GPU 0
#define NVAPI_GPU_UTILIZATION_DOMAIN_FB  1
#define NVAPI_GPU_UTILIZATION_DOMAIN_VID 2
#define NVAPI_GPU_UTILIZATION_DOMAIN_BUS 3

void
SK_PollGPU (void)
{
  if (! nvapi_init) {
    if (! ADL_init) {
      gpu_stats.num_gpus = 0;
      return;
    }
  }

  SYSTEMTIME     update_time;
  FILETIME       update_ftime;
  ULARGE_INTEGER update_ul;

  GetSystemTime        (&update_time);
  SystemTimeToFileTime (&update_time, &update_ftime);

  update_ul.HighPart = update_ftime.dwHighDateTime;
  update_ul.LowPart  = update_ftime.dwLowDateTime;

  double dt = (update_ul.QuadPart - gpu_stats.last_update.QuadPart) * 1.0e-7;

  if (dt > config.gpu.interval) {
    gpu_stats.last_update.QuadPart = update_ul.QuadPart;

if (nvapi_init)
{
    NvPhysicalGpuHandle gpus [NVAPI_MAX_PHYSICAL_GPUS];
    NvU32               gpu_count;

    if (NVAPI_OK != NvAPI_EnumPhysicalGPUs (gpus, &gpu_count))
      return;

    NV_GPU_DYNAMIC_PSTATES_INFO_EX psinfoex;
    psinfoex.version = NV_GPU_DYNAMIC_PSTATES_INFO_EX_VER;

    gpu_stats.num_gpus = gpu_count;

    static std::vector <NvU32> gpu_ids;
    static bool init = false;

    if (! init) {
      for (int i = 0; i < gpu_stats.num_gpus; i++) {
        NvU32 gpu_id;
        NvAPI_GetGPUIDFromPhysicalGPU (gpus [i], &gpu_id );
        gpu_ids.push_back (gpu_id);
      }

      std::sort (gpu_ids.begin (), gpu_ids.end ());

      init = true;
    }

    for (int i = 0; i < gpu_stats.num_gpus; i++) {
      NvPhysicalGpuHandle gpu;

      // In order for DXGI Adapter info to match up... don't just assign
      //   these GPUs willy-nilly, use the high 24-bits of the GPUID as
      //     a bitmask.
      //NvAPI_GetPhysicalGPUFromGPUID (1 << (i + 8), &gpu);
      //gpu_stats.gpus [i].nv_gpuid = (1 << (i + 8));

      NvAPI_GetPhysicalGPUFromGPUID (gpu_ids [i], &gpu);
      gpu_stats.gpus [i].nv_gpuid = gpu_ids [i];

      NvAPI_Status        status =
        NvAPI_GPU_GetDynamicPstatesInfoEx (gpu, &psinfoex);

      if (status == NVAPI_OK)
      {
#ifdef SMOOTH_GPU_UPDATES
      gpu_stats.gpus [i].loads_percent.gpu = (gpu_stats.gpus [i].loads_percent.gpu +
       psinfoex.utilization [NVAPI_GPU_UTILIZATION_DOMAIN_GPU].percentage) / 2;
      gpu_stats.gpus [i].loads_percent.fb  = (gpu_stats.gpus [i].loads_percent.fb +
        psinfoex.utilization [NVAPI_GPU_UTILIZATION_DOMAIN_FB].percentage) / 2;
      gpu_stats.gpus [i].loads_percent.vid = (gpu_stats.gpus [i].loads_percent.vid +
        psinfoex.utilization [NVAPI_GPU_UTILIZATION_DOMAIN_VID].percentage) / 2;
      gpu_stats.gpus [i].loads_percent.bus = (gpu_stats.gpus [i].loads_percent.bus +
        psinfoex.utilization [NVAPI_GPU_UTILIZATION_DOMAIN_BUS].percentage) / 2;
#else
      gpu_stats.gpus [i].loads_percent.gpu =
        psinfoex.utilization [NVAPI_GPU_UTILIZATION_DOMAIN_GPU].percentage;
      gpu_stats.gpus [i].loads_percent.fb =
        psinfoex.utilization [NVAPI_GPU_UTILIZATION_DOMAIN_FB].percentage;
      gpu_stats.gpus [i].loads_percent.vid =
        psinfoex.utilization [NVAPI_GPU_UTILIZATION_DOMAIN_VID].percentage;
      gpu_stats.gpus [i].loads_percent.bus =
        psinfoex.utilization [NVAPI_GPU_UTILIZATION_DOMAIN_BUS].percentage;
#endif
      }

      NV_GPU_THERMAL_SETTINGS thermal;
      thermal.version = NV_GPU_THERMAL_SETTINGS_VER;

      status = NvAPI_GPU_GetThermalSettings (gpu,
                                             NVAPI_THERMAL_TARGET_ALL,
                                             &thermal);

      if (status == NVAPI_OK) {
        for (NvU32 j = 0; j < thermal.count; j++) {
#ifdef SMOOTH_GPU_UPDATES
          if (thermal.sensor [j].target == NVAPI_THERMAL_TARGET_GPU)
            gpu_stats.gpus [i].temps_c.gpu = (gpu_stats.gpus[i].temps_c.gpu + thermal.sensor [j].currentTemp) / 2;
#else
          if (thermal.sensor [j].target == NVAPI_THERMAL_TARGET_GPU)
            gpu_stats.gpus [i].temps_c.gpu = thermal.sensor [j].currentTemp;
          if (thermal.sensor [j].target == NVAPI_THERMAL_TARGET_MEMORY)
            gpu_stats.gpus [i].temps_c.ram = thermal.sensor [j].currentTemp;
          if (thermal.sensor [j].target == NVAPI_THERMAL_TARGET_POWER_SUPPLY)
            gpu_stats.gpus [i].temps_c.psu = thermal.sensor [j].currentTemp;
          if (thermal.sensor [j].target == NVAPI_THERMAL_TARGET_BOARD)
            gpu_stats.gpus [i].temps_c.pcb = thermal.sensor [j].currentTemp;
#endif
        }
      }

      NvU32 pcie_lanes;
      if (NVAPI_OK == NvAPI_GPU_GetCurrentPCIEDownstreamWidth(gpu, &pcie_lanes))
      {
        gpu_stats.gpus [i].hwinfo.pcie_lanes         = pcie_lanes;
      }

      NV_PCIE_INFO pcieinfo;
      pcieinfo.version = NV_PCIE_INFO_VER;

      if (NVAPI_OK == NvAPI_GPU_GetPCIEInfo (gpu, &pcieinfo)) {
        gpu_stats.gpus [i].hwinfo.pcie_gen   =
          pcieinfo.info [0].unknown5;//states [pstate].pciLinkRate;
        gpu_stats.gpus [i].hwinfo.pcie_lanes =
          pcieinfo.info [0].unknown6;//pstates [pstate].pciLinkWidth;
        gpu_stats.gpus [i].hwinfo.pcie_transfer_rate =
          pcieinfo.info [0].unknown0;//pstates [pstate].pciLinkTransferRate;
      }

      NvU32 mem_width, mem_loc, mem_type;
      if (NVAPI_OK == NvAPI_GPU_GetFBWidthAndLocation (gpu, &mem_width, &mem_loc))
      {
        gpu_stats.gpus [i].hwinfo.mem_bus_width = mem_width;
      }

      if (NVAPI_OK == NvAPI_GPU_GetRamType (gpu, &mem_type))
      {
        gpu_stats.gpus [i].hwinfo.mem_type = mem_type;
      }

      NV_DISPLAY_DRIVER_MEMORY_INFO meminfo;
      meminfo.version = NV_DISPLAY_DRIVER_MEMORY_INFO_VER;

      if (NVAPI_OK == NvAPI_GPU_GetMemoryInfo (gpu, &meminfo))
      {
        int64_t local =
          (meminfo.availableDedicatedVideoMemory) -
          (meminfo.curAvailableDedicatedVideoMemory);

        gpu_stats.gpus [i].memory_B.local = local * 1024LL;

        gpu_stats.gpus [i].memory_B.total =
          ((meminfo.dedicatedVideoMemory) -
           (meminfo.curAvailableDedicatedVideoMemory)) * 1024LL;

        // Compute Non-Local
        gpu_stats.gpus [i].memory_B.nonlocal =
          gpu_stats.gpus [i].memory_B.total - gpu_stats.gpus [i].memory_B.local;
      }

      NV_GPU_CLOCK_FREQUENCIES freq;
      freq.version = NV_GPU_CLOCK_FREQUENCIES_VER;

      freq.ClockType = NV_GPU_CLOCK_FREQUENCIES_CURRENT_FREQ;
      if (NVAPI_OK == NvAPI_GPU_GetAllClockFrequencies (gpu, &freq))
      {
        gpu_stats.gpus [i].clocks_kHz.gpu    =
          freq.domain [NVAPI_GPU_PUBLIC_CLOCK_GRAPHICS].frequency;
        gpu_stats.gpus [i].clocks_kHz.ram    =
          freq.domain [NVAPI_GPU_PUBLIC_CLOCK_MEMORY].frequency;
        ////gpu_stats.gpus [i].clocks_kHz.shader =
          ////freq.domain [NVAPI_GPU_PUBLIC_CLOCK_PROCESSOR].frequency;
      }

      NvU32 tach;

      gpu_stats.gpus [i].fans_rpm.supported = false;

      if (NVAPI_OK == NvAPI_GPU_GetTachReading (gpu, &tach)) {
        gpu_stats.gpus [i].fans_rpm.gpu       = tach;
        gpu_stats.gpus [i].fans_rpm.supported = true;
      }

      NvU32 perf_decrease_info = 0;
      NvAPI_GPU_GetPerfDecreaseInfo (gpu, &perf_decrease_info);

      gpu_stats.gpus [i].nv_perf_state = perf_decrease_info;

      NV_GPU_PERF_PSTATES20_INFO ps20info;
      ps20info.version = NV_GPU_PERF_PSTATES20_INFO_VER;

      gpu_stats.gpus [i].volts_mV.supported = false;

      if (NVAPI_OK == NvAPI_GPU_GetPstates20 (gpu, &ps20info))
      {
        NV_GPU_PERF_PSTATE_ID current_pstate;

        if (NVAPI_OK == NvAPI_GPU_GetCurrentPstate (gpu, &current_pstate))
        {
          for (NvU32 pstate = 0; pstate < ps20info.numPstates; pstate++) {
            if (ps20info.pstates [pstate].pstateId == current_pstate) {
#if 1
              // First, check for over-voltage...
              if (gpu_stats.gpus [i].volts_mV.supported == false)
              {
                for (NvU32 volt = 0; volt < ps20info.ov.numVoltages; volt++) {
                  if (ps20info.ov.voltages [volt].domainId ==
                    NVAPI_GPU_PERF_VOLTAGE_INFO_DOMAIN_CORE) {
                    gpu_stats.gpus [i].volts_mV.supported = true;
                    gpu_stats.gpus [i].volts_mV.over      = true;

                    NV_GPU_PSTATE20_BASE_VOLTAGE_ENTRY_V1* voltage =
                      &ps20info.ov.voltages [volt];

                    gpu_stats.gpus [i].volts_mV.core = voltage->volt_uV/1000.0f;

                    int over  =
                      voltage->voltDelta_uV.value -
                      voltage->voltDelta_uV.valueRange.max;

                    int under =
                      voltage->voltDelta_uV.valueRange.min -
                      voltage->voltDelta_uV.value;

                    if (over > 0)
                      gpu_stats.gpus [i].volts_mV.ov =   over  / 1000.0f;
                    else if (under > 0)
                      gpu_stats.gpus [i].volts_mV.ov = -(under / 1000.0f);
                    break;
                  }
                }
              }

              // If that fails, look through the normal voltages.
#endif
              for (NvU32 volt = 0; volt < ps20info.numBaseVoltages; volt++) {
                if (ps20info.pstates [pstate].baseVoltages [volt].domainId ==
                    NVAPI_GPU_PERF_VOLTAGE_INFO_DOMAIN_CORE) {
                  gpu_stats.gpus [i].volts_mV.supported = true;
                  gpu_stats.gpus [i].volts_mV.over      = false;

                  NV_GPU_PSTATE20_BASE_VOLTAGE_ENTRY_V1* voltage =
                    &ps20info.pstates [pstate].baseVoltages [volt];

                  gpu_stats.gpus [i].volts_mV.core = voltage->volt_uV/1000.0f;

                  int over  =
                    voltage->voltDelta_uV.value -
                    voltage->voltDelta_uV.valueRange.max;

                  int under =
                    voltage->voltDelta_uV.valueRange.min -
                    voltage->voltDelta_uV.value;

                  if (over > 0) {
                    gpu_stats.gpus [i].volts_mV.ov   =   over  / 1000.0f;
                    gpu_stats.gpus [i].volts_mV.over = true;
                  } else if (under > 0) {
                    gpu_stats.gpus [i].volts_mV.ov   = -(under / 1000.0f);
                    gpu_stats.gpus [i].volts_mV.over = true;
                  } else if (over != 0) {
                    if (over > under)
                       gpu_stats.gpus [i].volts_mV.ov =  -(over  / 1000.0f);
                    else if (under > over)
                      gpu_stats.gpus [i].volts_mV.ov  =   (under / 1000.0f);
                    gpu_stats.gpus [i].volts_mV.over  = true;
                  }
                  break;
                }
              }
              break;
            }
          }
        }
      }
    }
  }

  else if (ADL_init == ADL_TRUE)
  {
    gpu_stats.num_gpus = SK_ADL_CountActiveGPUs ();
    //dll_log.Log (L"[DisplayLib] AMD GPUs: %i", gpu_stats.num_gpus);
    for (int i = 0; i < gpu_stats.num_gpus; i++) {
      AdapterInfo* pAdapter = SK_ADL_GetActiveAdapter (i);

      if (pAdapter->iAdapterIndex >= ADL_MAX_ADAPTERS || pAdapter->iAdapterIndex < 0) {
        dll_log.Log (L"[DisplayLib] INVALID ADL ADAPTER: %i", pAdapter->iAdapterIndex);
        break;
      }

      ADLPMActivity activity;

      ZeroMemory (&activity, sizeof (ADLPMActivity));

      activity.iSize = sizeof (ADLPMActivity);

      ADL_Overdrive5_CurrentActivity_Get (pAdapter->iAdapterIndex, &activity);

      gpu_stats.gpus [i].loads_percent.gpu = activity.iActivityPercent;
      gpu_stats.gpus [i].hwinfo.pcie_gen   = activity.iCurrentBusSpeed;
      gpu_stats.gpus [i].hwinfo.pcie_lanes = activity.iCurrentBusLanes;

      gpu_stats.gpus [i].clocks_kHz.gpu    = activity.iEngineClock * 10UL;
      gpu_stats.gpus [i].clocks_kHz.ram    = activity.iMemoryClock * 10UL;

      gpu_stats.gpus [i].volts_mV.supported = true;
      gpu_stats.gpus [i].volts_mV.over      = false;
      gpu_stats.gpus [i].volts_mV.core      = (float)activity.iVddc; // mV?

      ADLTemperature temp;

      ZeroMemory (&temp, sizeof (ADLTemperature));

      temp.iSize = sizeof (ADLTemperature);

      ADL_Overdrive5_Temperature_Get (pAdapter->iAdapterIndex, 0, &temp);

      gpu_stats.gpus [i].temps_c.gpu = temp.iTemperature / 1000UL;

      ADLFanSpeedValue fanspeed;

      ZeroMemory (&fanspeed, sizeof (ADLFanSpeedValue));

      fanspeed.iSize      = sizeof (ADLFanSpeedValue);
      fanspeed.iSpeedType = ADL_DL_FANCTRL_SPEED_TYPE_RPM;

      ADL_Overdrive5_FanSpeed_Get (pAdapter->iAdapterIndex, 0, &fanspeed);

      gpu_stats.gpus [i].fans_rpm.gpu       = fanspeed.iFanSpeed;
      gpu_stats.gpus [i].fans_rpm.supported = true;
    }
  }
}
}