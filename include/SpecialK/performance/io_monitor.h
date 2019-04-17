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

#ifndef __SK__IO_MONITOR_H__
#define __SK__IO_MONITOR_H__

#include <Windows.h>
#include <utility>
#include <cstdint>
#include <SpecialK/performance/memory_monitor.h>

struct thread_events
{
  struct telemetry
  {
    HANDLE start;
    HANDLE stop;
    HANDLE poll;
    HANDLE shutdown;
  } gpu, cpu,     IO,
    disk, memory, pagefile;
};

extern SK_LazyGlobal <thread_events> perfmon;

struct io_perf_t {
  bool           init         = false;

  ULARGE_INTEGER last_update  = { 0, 0 };
  IO_COUNTERS    accum        = {      };
  ULONGLONG      dt           =     0ULL;
  IO_COUNTERS    last_counter = {      };

  long double    read_mb_sec   = 0.0L;
  long double    write_mb_sec  = 0.0L;
  long double    other_mb_sec  = 0.0L;

  long double    read_iop_sec  = 0.0L;
  long double    write_iop_sec = 0.0L;
  long double    other_iop_sec = 0.0L;
};

void SK_CountIO (io_perf_t& ioc, const double update = 0.25 / 1.0e-7);

#define _WIN32_DCOM
#include <Wbemidl.h>

struct WMI_refresh_thread_t
{
  volatile HANDLE          hThread                      = INVALID_HANDLE_VALUE;
  volatile HANDLE          hShutdownSignal              = INVALID_HANDLE_VALUE;

  IWbemRefresher          *pRefresher                   = nullptr;
  IWbemConfigureRefresher *pConfig                      = nullptr;
  IWbemHiPerfEnum         *pEnum                        = nullptr;
  IWbemObjectAccess      **apEnumAccess                 = nullptr;
  long                     lID                          = 0;

  DWORD                    dwNumObjects                 = 0;
  DWORD                    dwNumReturned                = 0;

  // Set to false after the first refresh iteration
  bool                     booting                      = true;
};

typedef struct _SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION__SK {
  LARGE_INTEGER IdleTime;
  LARGE_INTEGER KernelTime;
  LARGE_INTEGER UserTime;
  LARGE_INTEGER DpcTime;
  LARGE_INTEGER InterruptTime;
  ULONG         InterruptCount;
}  SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION__SK,
 *PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION__SK;


struct cpu_perf_t : WMI_refresh_thread_t
{
  struct cpu_stat_s {
    volatile LONG          percent_load                 = 0;
    volatile LONG          percent_idle                 = 0;
    volatile LONG          percent_kernel               = 0;
    volatile LONG          percent_user                 = 0;
    volatile LONG          percent_interrupt            = 0;
    volatile LONG          joules_consumed              = 0;

    SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION__SK
      last_perf_count    = { },
      current_perf_count = { };

    SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION__SK
    getAllDeltas (void) noexcept
    {
      SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION__SK ret;

      ret.DpcTime.QuadPart       =
        ( current_perf_count.DpcTime.QuadPart       - last_perf_count.DpcTime.QuadPart       );
      ret.IdleTime.QuadPart      =
        ( current_perf_count.IdleTime.QuadPart      - last_perf_count.IdleTime.QuadPart      );
      ret.InterruptTime.QuadPart =
        ( current_perf_count.InterruptTime.QuadPart - last_perf_count.InterruptTime.QuadPart );
      ret.UserTime.QuadPart      =
        ( current_perf_count.UserTime.QuadPart      - last_perf_count.UserTime.QuadPart      );
      ret.KernelTime.QuadPart    =
        ( current_perf_count.KernelTime.QuadPart    - last_perf_count.KernelTime.QuadPart    );

      return ret;
    }

    LARGE_INTEGER
    updatePercentages (void)
    {
      const SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION__SK& dt_cpu =
        getAllDeltas ();

      LARGE_INTEGER total_delta_run = { }, total_delta_idle = { },
                    total_delta_all = { };

                    total_delta_run.QuadPart  =
        ( dt_cpu.DpcTime.QuadPart  + dt_cpu.InterruptTime.QuadPart +
          dt_cpu.UserTime.QuadPart + dt_cpu.KernelTime.QuadPart    );

                    total_delta_idle.QuadPart =
                                     dt_cpu.IdleTime.QuadPart;

                    total_delta_all.QuadPart =
                  ( total_delta_run.QuadPart + total_delta_idle.QuadPart );

      const long double
        rational_cpu_load =
          std::min   (100.0l,
            std::max (  0.0l,
              ( 100.0l - (static_cast <long double> (dt_cpu.IdleTime.QuadPart)/
                          static_cast <long double> (total_delta_run.QuadPart)) * 100.0l )
            )
          );

      InterlockedExchange (
        &percent_load,      static_cast < LONG > (rational_cpu_load)
      );
      InterlockedExchange (
        &percent_idle,      100 - ReadAcquire (&percent_load)
      );

      InterlockedExchange (
        &percent_kernel,    static_cast < LONG >
                          ((static_cast <long double> (dt_cpu.KernelTime.QuadPart)/
                            static_cast <long double> ( total_delta_run.QuadPart )) * rational_cpu_load)
      );
      InterlockedExchange (
        &percent_user  ,    static_cast < LONG >
                          ((static_cast <double> ( dt_cpu.UserTime.QuadPart )/
                            static_cast <double> ( total_delta_run.QuadPart )) * rational_cpu_load)
      );
      InterlockedExchange (
        &percent_interrupt, static_cast < LONG >
                          ((static_cast <long double> ( dt_cpu.InterruptTime.QuadPart )/
                            static_cast <long double> (    total_delta_run.QuadPart   )) * rational_cpu_load)
      );

      return total_delta_all;
    };

    LARGE_INTEGER
    recordNewData (const SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION__SK& new_perf_count )
    {
      std::swap (current_perf_count, last_perf_count);
                 current_perf_count = new_perf_count;

      return std::move (
        updatePercentages ()
      );
    };

    uint64_t               temp_c                       = 0;

    DWORD                  update_time                  = 0UL;

    // Applies to the data in Virtual CPU Node #65;
    //   aggregate counter for combined CPU performance
    static int             unparked_node_count,
                             parked_node_count;

    LARGE_INTEGER          parked_since                 = { 0 };

    uint32_t               CurrentMhz                   = 0;
    uint32_t               MaximumMhz                   = 0;
  } cpus [65];
    // Idx 64 holds combined load totals as discussed above


  LARGE_INTEGER
  addToAggregate ( const SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION__SK& single_node_perf ) noexcept
  {
    cpus [64].current_perf_count.DpcTime.QuadPart       +=
      single_node_perf.DpcTime.QuadPart;
    cpus [64].current_perf_count.IdleTime.QuadPart      +=
      single_node_perf.IdleTime.QuadPart;
    cpus [64].current_perf_count.UserTime.QuadPart      +=
      single_node_perf.UserTime.QuadPart;
    cpus [64].current_perf_count.KernelTime.QuadPart    +=
      single_node_perf.KernelTime.QuadPart;
    cpus [64].current_perf_count.InterruptTime.QuadPart +=
      single_node_perf.InterruptTime.QuadPart;

    if (single_node_perf.IdleTime.QuadPart > 0) ++cpu_stat_s::unparked_node_count;
    else                                          cpu_stat_s::parked_node_count++;

    return { 0 };
  }

  void beginNewAggregate (void) noexcept
  {
    std::swap ( cpus [64].current_perf_count, cpus [64].last_perf_count );
                cpus [64].current_perf_count             =      { };

    cpu_stat_s::parked_node_count   = 0;
    cpu_stat_s::unparked_node_count = 0;
  }

  LARGE_INTEGER
  endAggregateTally (void)
  {
    return
      cpus [64].updatePercentages ();
  }



  DWORD                    num_cpus                     = 0;
};


struct disk_perf_t : WMI_refresh_thread_t
{
  long                     lNameHandle                  = 0;

  long                     lDiskBytesPerSecHandle       = 0;
  long                     lDiskReadBytesPerSecHandle   = 0;
  long                     lDiskWriteBytesPerSecHandle  = 0;

  long                     lPercentDiskReadTimeHandle   = 0;
  long                     lPercentDiskWriteTimeHandle  = 0;
  long                     lPercentDiskTimeHandle       = 0;
  long                     lPercentIdleTimeHandle       = 0;

  struct disk_stat_s {
    char                   name [32];

    uint64_t               percent_load                 = 0;
    uint64_t               percent_write                = 0;
    uint64_t               percent_read                 = 0;
    uint64_t               percent_idle                 = 0;

    uint64_t               read_bytes_sec               = 0;
    uint64_t               write_bytes_sec              = 0;
    uint64_t               bytes_sec                    = 0;
  } disks [16];

  DWORD                    num_disks                    = 0;
};


struct pagefile_perf_t : WMI_refresh_thread_t
{
  long                     lNameHandle                  = 0;

  long                     lPercentUsageHandle          = 0;
  long                     lPercentUsagePeakHandle      = 0;
  long                     lPercentUsage_BaseHandle     = 0;

  struct {
    char                   name [256];

    DWORD                  size                         = 0;
    DWORD                  usage                        = 0;
    DWORD                  usage_peak                   = 0;
  } pagefiles [16];

  DWORD                    num_pagefiles                = 0;
};


DWORD WINAPI SK_MonitorCPU      (LPVOID user);
DWORD WINAPI SK_MonitorDisk     (LPVOID user);
DWORD WINAPI SK_MonitorPagefile (LPVOID user);

extern SK_LazyGlobal <process_stats_t> SK_WMI_ProcessStats;
extern SK_LazyGlobal <cpu_perf_t>      SK_WMI_CPUStats;
extern SK_LazyGlobal <disk_perf_t>     SK_WMI_DiskStats;
extern SK_LazyGlobal <pagefile_perf_t> SK_WMI_PagefileStats;

#endif /* __ SK__IO_MONITOR_H__ */