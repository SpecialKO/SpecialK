
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

#include <SpecialK/com_util.h>
#include <SpecialK/performance/io_monitor.h>
#include <SpecialK/performance/memory_monitor.h>
#include <SpecialK/log.h>

#include <SpecialK/core.h>
#include <SpecialK/framerate.h>
#include <SpecialK/config.h>
#include <SpecialK/utility.h>
#include <SpecialK/thread.h>

#include <SpecialK/widgets/widget.h>

#include <unordered_map>
#include <algorithm>

#include <process.h>
#include <comdef.h>

thread_events   perfmon        = {};


process_stats_t& __SK_WMI_ProcessStats (void)
 { static process_stats_t _process_stats;  return _process_stats;  };

cpu_perf_t&      __SK_WMI_CPUStats      (void)
 { static cpu_perf_t      _cpu_stats;      return _cpu_stats;      };
disk_perf_t&     __SK_WMI_DiskStats     (void)
 { static disk_perf_t     _disk_stats;     return _disk_stats;     };
pagefile_perf_t& __SK_WMI_PagefileStats (void)
 { static pagefile_perf_t _pagefile_stats; return _pagefile_stats; };


void
SK_CountIO (io_perf_t& ioc, const double update)
{
  // Virtual handle; don't close this.
  static HANDLE hProc =
    SK_GetCurrentProcess ();

  if (ioc.init == false)
  {
    RtlZeroMemory (&ioc, sizeof io_perf_t);
                    ioc.init = true;
  }

  extern LARGE_INTEGER SK_QueryPerf (void);
   LARGE_INTEGER now = SK_QueryPerf ();

  ioc.dt =
    now.QuadPart - ioc.last_update.QuadPart;

  if (ioc.dt >= update)
  {
    IO_COUNTERS current_io = { };

    static SK::Framerate::Stats write_rate;
    static SK::Framerate::Stats read_rate;
    static SK::Framerate::Stats other_rate;

    static SK::Framerate::Stats write_ops;
    static SK::Framerate::Stats read_ops;
    static SK::Framerate::Stats other_ops;

    GetProcessIoCounters (hProc, &current_io);

    const auto dRB = (current_io.ReadTransferCount   - ioc.last_counter.ReadTransferCount);
    const auto dWB = (current_io.WriteTransferCount  - ioc.last_counter.WriteTransferCount);
    const auto dOB = (current_io.OtherTransferCount  - ioc.last_counter.OtherTransferCount);

    const auto dRC = (current_io.ReadOperationCount  - ioc.last_counter.ReadOperationCount);
    const auto dWC = (current_io.WriteOperationCount - ioc.last_counter.WriteOperationCount);
    const auto dOC = (current_io.OtherOperationCount - ioc.last_counter.OtherOperationCount);

    double& read_mb_sec   = ioc.read_mb_sec;
    double& write_mb_sec  = ioc.write_mb_sec;
    double& other_mb_sec  = ioc.other_mb_sec;

    double& read_iop_sec  = ioc.read_iop_sec;
    double& write_iop_sec = ioc.write_iop_sec;
    double& other_iop_sec = ioc.other_iop_sec;

    const double inst_read_mb_sec  =
      ((static_cast <double> (dRB) / 1048576.0) / (1.0e-7 * static_cast <double> (ioc.dt)));
    const double inst_write_mb_sec  =
      ((static_cast <double> (dWB) / 1048576.0) / (1.0e-7 * static_cast <double> (ioc.dt)));
    const double inst_other_mb_sec  =
      ((static_cast <double> (dOB) / 1048576.0) / (1.0e-7 * static_cast <double> (ioc.dt)));

    const double inst_read_ops_sec  =
      (static_cast <double> (dRC) / (1.0e-7 * static_cast <double> (ioc.dt)));
    const double inst_write_ops_sec  =
      (static_cast <double> (dWC) / (1.0e-7 * static_cast <double> (ioc.dt)));
    const double inst_other_ops_sec  =
      (static_cast <double> (dOC) / (1.0e-7 * static_cast <double> (ioc.dt)));

    read_rate.addSample  (inst_read_mb_sec,   now);
    write_rate.addSample (inst_write_mb_sec,  now);
    other_rate.addSample (inst_other_mb_sec,  now);

    read_ops.addSample   (inst_read_ops_sec,  now);
    write_ops.addSample  (inst_write_ops_sec, now);
    other_ops.addSample  (inst_other_ops_sec, now);

    read_mb_sec  = read_rate.calcMean  (2.0f);
    write_mb_sec = write_rate.calcMean (2.0f);
    other_mb_sec = other_rate.calcMean (2.0f);

    read_iop_sec  = read_ops.calcMean  (2.0f);
    write_iop_sec = write_ops.calcMean (2.0f);
    other_iop_sec = other_ops.calcMean (2.0f);

    ioc.last_update.QuadPart = now.QuadPart;

    memcpy (&ioc.last_counter, &current_io, sizeof (IO_COUNTERS));
  }
}


DWORD
WINAPI
SK_MonitorCPU (LPVOID user_param)
{
  SK_Thread_SetCurrentPriority (THREAD_PRIORITY_LOWEST);
  SetCurrentThreadDescription  (L"[SK] WMI CPU Monitoring Thread");

  SetThreadPriorityBoost       (GetCurrentThread (), TRUE);

  SK_WMI_WaitForInit ();

  SK_AutoCOMInit auto_com;

  UNREFERENCED_PARAMETER (user_param);

  COM::base.wmi.Lock ();

  cpu_perf_t&  cpu    = cpu_stats;
  const double update = config.cpu.interval;

  HRESULT hr;

  if (FAILED (hr = CoCreateInstance_Original (
                     CLSID_WbemRefresher,
                     nullptr,
                     CLSCTX_INPROC_SERVER,
                     IID_IWbemRefresher, 
                     (void**) &cpu.pRefresher )
             )
     )
  {
    dll_log.Log (L"[ WMI Wbem ] Failed to create Refresher Instance (%s:%d) [%s]",
      __FILEW__, __LINE__, _com_error (hr).ErrorMessage ());
    goto CPU_CLEANUP;
  }

  if (FAILED (hr = cpu.pRefresher->QueryInterface (
                        IID_IWbemConfigureRefresher,
                        (void **)&cpu.pConfig )
             )
     )
  {
    dll_log.Log (L"[ WMI Wbem ] Failed to Query Refresher Interface (%s:%d) [%s]",
      __FILEW__, __LINE__, _com_error (hr).ErrorMessage ());
    goto CPU_CLEANUP;
  }

  // Add an enumerator to the refresher.
  if (FAILED (hr = cpu.pConfig->AddEnum (
                     COM::base.wmi.pNameSpace,
                     L"Win32_PerfFormattedData_PerfOS_Processor",
                     0,
                     nullptr,
                     &cpu.pEnum,
                     &cpu.lID )
             )
     )
  {
    dll_log.Log (L"[ WMI Wbem ] Failed to Add Enumerator (%s:%d) - %s",
      __FILEW__, __LINE__, _com_error (hr).ErrorMessage ());
    goto CPU_CLEANUP;
  }

  cpu.pConfig->Release ();
  cpu.pConfig = nullptr;

  int iter = 0;

  cpu.dwNumReturned = 0;
  cpu.dwNumObjects  = 0;

  cpu.hShutdownSignal = CreateEvent (nullptr, FALSE, FALSE, L"CPUMon Shutdown Signal");

  COM::base.wmi.Unlock ();

  while (cpu.lID != 0)
  {
    if (MsgWaitForMultipleObjects (1, const_cast <const HANDLE *> (&cpu.hShutdownSignal), FALSE, ( DWORD (update * 1000.0) ), QS_ALLEVENTS) == WAIT_OBJECT_0)
      break;

    // Only poll WMI while the data view is visible
    if (! (config.cpu.show || SK_ImGui_Widgets.cpu_monitor->isActive ()))
      continue;

    cpu.dwNumReturned = 0;

    COM::base.wmi.Lock ();

    if (FAILED (hr = cpu.pRefresher->Refresh (0L)))
    {
      dll_log.Log (L"[ WMI Wbem ] Failed to Refresh CPU (%s:%d) [%s]",
        __FILEW__, __LINE__, _com_error (hr).ErrorMessage ());
      goto CPU_CLEANUP;
    }

    hr = cpu.pEnum->GetObjects ( 0L,
                                 cpu.dwNumObjects,
                                 cpu.apEnumAccess,
                                 &cpu.dwNumReturned );


    // If the buffer was not big enough,
    // allocate a bigger buffer and retry.
    if (hr == WBEM_E_BUFFER_TOO_SMALL 
        && cpu.dwNumReturned > cpu.dwNumObjects)
    {
      cpu.apEnumAccess = new IWbemObjectAccess* [cpu.dwNumReturned];

      if (cpu.apEnumAccess == nullptr)
      {
        dll_log.Log (L"[ WMI Wbem ] Out of Memory (%s:%d)",
          __FILEW__, __LINE__);
        hr = E_OUTOFMEMORY;
        goto CPU_CLEANUP;
      }

      RtlSecureZeroMemory ( cpu.apEnumAccess,
                            cpu.dwNumReturned * sizeof(IWbemObjectAccess *) );

      cpu.dwNumObjects = cpu.dwNumReturned;

      if (FAILED (hr = cpu.pEnum->GetObjects ( 0L,
                                               cpu.dwNumObjects,
                                               cpu.apEnumAccess,
                                               &cpu.dwNumReturned )
                 )
         )
      {
        dll_log.Log (L"[ WMI Wbem ] Failed to get CPU Objects (%s:%d) [%s]",
          __FILEW__, __LINE__, _com_error (hr).ErrorMessage ());

        goto CPU_CLEANUP;
      }

      cpu.dwNumReturned = std::min (cpu.dwNumObjects, cpu.dwNumReturned);
    }
    else
    {
      if (hr != WBEM_S_NO_ERROR)
      {
        dll_log.Log (L"[ WMI Wbem ] UNKNOWN ERROR (%s:%d) [%s]",
          __FILEW__, __LINE__, _com_error (hr).ErrorMessage ());
        hr = WBEM_E_NOT_FOUND;
        goto CPU_CLEANUP;
      }
    }

    if (cpu.dwNumReturned < 1 || cpu.apEnumAccess [0] == nullptr)
    {
      COM::base.wmi.Unlock ();

      SleepEx (100UL, FALSE);

      continue;
    }

    // First time through, get the handles.
    if (iter == 0)
    {
      CIMTYPE PercentInterruptTimeType;
      CIMTYPE PercentPrivilegedTimeType;
      CIMTYPE PercentUserTimeType;
      CIMTYPE PercentProcessorTimeType;
      CIMTYPE PercentIdleTimeType;

      if (FAILED (hr = cpu.apEnumAccess [0]->GetPropertyHandle (
                            L"PercentInterruptTime",
                            &PercentInterruptTimeType,
                            &cpu.lPercentInterruptTimeHandle )
                 )
         )
      {
        dll_log.Log (L"[ WMI Wbem ] Failed to acquire property handle (%s:%d) [%s]",
          __FILEW__, __LINE__, _com_error (hr).ErrorMessage ());
        goto CPU_CLEANUP;
      }

      if (FAILED (hr = cpu.apEnumAccess [0]->GetPropertyHandle (
                            L"PercentPrivilegedTime",
                            &PercentPrivilegedTimeType,
                            &cpu.lPercentPrivilegedTimeHandle )
                 )
         )
      {
        dll_log.Log (L"[ WMI Wbem ] Failed to acquire property handle (%s:%d) [%s]",
          __FILEW__, __LINE__, _com_error (hr).ErrorMessage ());
        goto CPU_CLEANUP;
      }

      if (FAILED (hr = cpu.apEnumAccess [0]->GetPropertyHandle (
                            L"PercentUserTime",
                            &PercentUserTimeType,
                            &cpu.lPercentUserTimeHandle )
                 )
         )
      {
        dll_log.Log (L"[ WMI Wbem ] Failed to acquire property handle (%s:%d) [%s]",
          __FILEW__, __LINE__, _com_error (hr).ErrorMessage ());
        goto CPU_CLEANUP;
      }

      if (FAILED (hr = cpu.apEnumAccess [0]->GetPropertyHandle (
                            L"PercentProcessorTime",
                            &PercentProcessorTimeType,
                            &cpu.lPercentProcessorTimeHandle )
                 )
         )
      {
        dll_log.Log (L"[ WMI Wbem ] Failed to acquire property handle (%s:%d) [%s]",
          __FILEW__, __LINE__, _com_error (hr).ErrorMessage ());
        goto CPU_CLEANUP;
      }

      if (FAILED (hr = cpu.apEnumAccess [0]->GetPropertyHandle (
                            L"PercentIdleTime",
                            &PercentIdleTimeType,
                            &cpu.lPercentIdleTimeHandle )
                 )
         )
      {
        dll_log.Log (L"[ WMI Wbem ] Failed to acquire property handle (%s:%d) [%s]",
          __FILEW__, __LINE__, _com_error (hr).ErrorMessage ());
        goto CPU_CLEANUP;
      }
    }

    for (unsigned int i = 0; i < cpu.dwNumReturned; i++)
    {
      uint64_t interrupt;
      uint64_t kernel;
      uint64_t user;
      uint64_t load;
      uint64_t idle;

      if (cpu.apEnumAccess [i] == nullptr)
      {
        dll_log.Log (L"[ WMI Wbem ] CPU apEnumAccess [%lu] = nullptr",  i);
        goto CPU_CLEANUP;
      }

      if (FAILED (hr = cpu.apEnumAccess [i]->ReadQWORD (
                             cpu.lPercentInterruptTimeHandle,
                             &interrupt )
                 )
         )
      {
        dll_log.Log (L"[ WMI Wbem ] Failed to read Quad-Word Property (%s:%d) [%s]",
          __FILEW__, __LINE__, _com_error (hr).ErrorMessage ());
        goto CPU_CLEANUP;
      }

      if (FAILED (hr = cpu.apEnumAccess [i]->ReadQWORD (
                             cpu.lPercentPrivilegedTimeHandle,
                             &kernel )
                 )
         )
      {
        dll_log.Log (L"[ WMI Wbem ] Failed to read Quad-Word Property (%s:%d) [%s]",
          __FILEW__, __LINE__, _com_error (hr).ErrorMessage ());
        goto CPU_CLEANUP;
      }

      if (FAILED (hr = cpu.apEnumAccess [i]->ReadQWORD (
                             cpu.lPercentUserTimeHandle,
                             &user )
                 )
         )
      {
        dll_log.Log (L"[ WMI Wbem ] Failed to read Quad-Word Property (%s:%d)",
          __FILEW__, __LINE__);
        goto CPU_CLEANUP;
      }

      if (FAILED (hr = cpu.apEnumAccess [i]->ReadQWORD (
                             cpu.lPercentProcessorTimeHandle,
                             &load )
                 )
         )
      {
        dll_log.Log (L"[ WMI Wbem ] Failed to read Quad-Word Property (%s:%d)",
          __FILEW__, __LINE__);
        goto CPU_CLEANUP;
      }

      if (FAILED (hr = cpu.apEnumAccess [i]->ReadQWORD (
                             cpu.lPercentIdleTimeHandle,
                             &idle )
                 )
         )
      {
        dll_log.Log (L"[ WMI Wbem ] Failed to read Quad-Word Property (%s:%d)",
          __FILEW__, __LINE__);
        goto CPU_CLEANUP;
      }

      cpu.cpus [i].percent_idle   = (cpu.cpus [i].percent_idle   + idle)   / 2;
      cpu.cpus [i].percent_load   = (cpu.cpus [i].percent_load   + load)   / 2;
      cpu.cpus [i].percent_user   = (cpu.cpus [i].percent_user   + user)   / 2;
      cpu.cpus [i].percent_kernel = (cpu.cpus [i].percent_kernel + kernel) / 2;
      cpu.cpus [i].percent_interrupt
                                  = ( cpu.cpus [i].percent_interrupt + 
                                      interrupt ) / 2;

      cpu.cpus [i].update_time = timeGetTime ();

      // Done with the object
      cpu.apEnumAccess [i]->Release ();
      cpu.apEnumAccess [i] = nullptr;
    }

    cpu.num_cpus = cpu.dwNumReturned;
    cpu.booting  = false;

    ++iter;

    COM::base.wmi.Unlock ();
  }

  COM::base.wmi.Lock ();

CPU_CLEANUP:
  //dll_log.Log (L" >> CPU_CLEANUP");

  if (cpu.apEnumAccess != nullptr)
  {
    for (unsigned int i = 0; i < cpu.dwNumReturned; i++)
    {
      if (cpu.apEnumAccess [i] != nullptr)
      {
        cpu.apEnumAccess [i]->Release ();
        cpu.apEnumAccess [i] = nullptr;
      }
    }
    delete [] cpu.apEnumAccess;
  }

  if (cpu.pEnum)
  {
    cpu.pEnum->Release ();
    cpu.pEnum = nullptr;
  }

  if (cpu.pConfig != nullptr)
  {
    cpu.pConfig->Release ();
    cpu.pConfig = nullptr;
  }

  if (cpu.pRefresher != nullptr)
  {
    cpu.pRefresher->Release ();
    cpu.pRefresher = nullptr;
  }

  if (cpu.hShutdownSignal != INVALID_HANDLE_VALUE)
  {
    CloseHandle (cpu.hShutdownSignal);
    cpu.hShutdownSignal = nullptr;
  }

  COM::base.wmi.Unlock   ();

  return 0;
}


DWORD
WINAPI
SK_MonitorDisk (LPVOID user)
{
  SK_Thread_SetCurrentPriority (THREAD_PRIORITY_LOWEST);
  SetCurrentThreadDescription  (L"[SK] WMI Disk Monitoring Thread");

  SetThreadPriorityBoost       (GetCurrentThread (), TRUE);

  SK_WMI_WaitForInit ();

  SK_AutoCOMInit auto_com;

  UNREFERENCED_PARAMETER (user);

  COM::base.wmi.Lock ();

  //Win32_PerfFormattedData_PerfDisk_LogicalDisk

  disk_perf_t&  disk  = disk_stats;
  const double update = config.disk.interval;

  HRESULT hr;

  if (FAILED (hr = CoCreateInstance_Original (
                     CLSID_WbemRefresher,
                     nullptr,
                     CLSCTX_INPROC_SERVER,
                     IID_IWbemRefresher, 
                     (void**) &disk.pRefresher )
             )
     )
  {
    goto DISK_CLEANUP;
  }

  if (FAILED (hr = disk.pRefresher->QueryInterface (
                        IID_IWbemConfigureRefresher,
                        (void **)&disk.pConfig )
             )
     )
  {
    goto DISK_CLEANUP;
  }

  // Add an enumerator to the refresher.
  if (FAILED (hr = disk.pConfig->AddEnum (
                     COM::base.wmi.pNameSpace,
                     config.disk.type == 1 ? 
                     L"Win32_PerfFormattedData_PerfDisk_LogicalDisk" :
                     L"Win32_PerfFormattedData_PerfDisk_PhysicalDisk",
                     0,
                     nullptr,
                     &disk.pEnum,
                     &disk.lID )
             )
     )
  {
    goto DISK_CLEANUP;
  }

  disk.pConfig->Release ();
  disk.pConfig = nullptr;

  int iter = 0;

  disk.dwNumReturned = 0;
  disk.dwNumObjects  = 0;

  disk.hShutdownSignal = CreateEvent (nullptr, FALSE, FALSE, L"DiskMon Shutdown Signal");

  COM::base.wmi.Unlock ();

  while (disk_stats.lID != 0)
  {
    if (MsgWaitForMultipleObjects (1, const_cast <const HANDLE *> (&disk_stats.hShutdownSignal), FALSE, ( DWORD (update * 1000.0) ), QS_ALLEVENTS) == WAIT_OBJECT_0)
      break;

    // Only poll WMI while the data view is visible
    if (! config.disk.show)
      continue;

    extern LARGE_INTEGER SK_QueryPerf (void);
     LARGE_INTEGER now = SK_QueryPerf ();

    disk.dwNumReturned = 0;

    COM::base.wmi.Lock ();

    if (FAILED (hr = disk.pRefresher->Refresh (0L)))
    {
      goto DISK_CLEANUP;
    }

    hr = disk.pEnum->GetObjects ( 0L,
                                  disk.dwNumObjects,
                                  disk.apEnumAccess,
                                 &disk.dwNumReturned );

    // If the buffer was not big enough,
    // allocate a bigger buffer and retry.
    if (hr == WBEM_E_BUFFER_TOO_SMALL 
        && disk.dwNumReturned > disk.dwNumObjects)
    {
      disk.apEnumAccess = new IWbemObjectAccess* [disk.dwNumReturned];
      if (disk.apEnumAccess == nullptr)
      {
        hr = E_OUTOFMEMORY;
        goto DISK_CLEANUP;
      }

      SecureZeroMemory (disk.apEnumAccess,
                        disk.dwNumReturned * sizeof (IWbemObjectAccess *));

      disk.dwNumObjects = disk.dwNumReturned;

      if (FAILED (hr = disk.pEnum->GetObjects ( 0L,
                                               disk.dwNumObjects,
                                               disk.apEnumAccess,
                                               &disk.dwNumReturned )
                 )
         )
      {
        goto DISK_CLEANUP;
      }

      disk.dwNumReturned = std::min (disk.dwNumObjects, disk.dwNumReturned);
    }
    else
    {
      if (hr != WBEM_S_NO_ERROR)
      {
        hr = WBEM_E_NOT_FOUND;
        goto DISK_CLEANUP;
      }
    }

    // First time through, get the handles.
    if (iter == 0)
    {
      CIMTYPE NameType;

      CIMTYPE DiskBytesPerSecType;
      CIMTYPE DiskReadBytesPerSecType;
      CIMTYPE DiskWriteBytesPerSecType;

      CIMTYPE PercentDiskTimeType;
      CIMTYPE PercentDiskReadTimeType;
      CIMTYPE PercentDiskWriteTimeType;
      CIMTYPE PercentIdleTimeType;

      if (disk.apEnumAccess [0] == nullptr)
      {
        dll_log.Log (L"[ WMI Wbem ] Disk apEnumAccess [0] = nullptr");
        goto DISK_CLEANUP;
      }

      if (FAILED (hr = disk.apEnumAccess [0]->GetPropertyHandle (
                            L"Name",
                            &NameType,
                            &disk.lNameHandle )
                 )
         )
      {
        goto DISK_CLEANUP;
      }

      if (FAILED (hr = disk.apEnumAccess [0]->GetPropertyHandle (
                            L"DiskBytesPerSec",
                            &DiskBytesPerSecType,
                            &disk.lDiskBytesPerSecHandle )
                 )
         )
      {
        goto DISK_CLEANUP;
      }

      if (FAILED (hr = disk.apEnumAccess [0]->GetPropertyHandle (
                            L"DiskReadBytesPerSec",
                            &DiskReadBytesPerSecType,
                            &disk.lDiskReadBytesPerSecHandle )
                 )
         )
      {
        goto DISK_CLEANUP;
      }

      if (FAILED (hr = disk.apEnumAccess [0]->GetPropertyHandle (
                            L"DiskWriteBytesPerSec",
                            &DiskWriteBytesPerSecType,
                            &disk.lDiskWriteBytesPerSecHandle )
                 )
         )
      {
        goto DISK_CLEANUP;
      }

      if (FAILED (hr = disk.apEnumAccess [0]->GetPropertyHandle (
                            L"PercentDiskTime",
                            &PercentDiskTimeType,
                            &disk.lPercentDiskTimeHandle )
                 )
         )
      {
        goto DISK_CLEANUP;
      }

      if (FAILED (hr = disk.apEnumAccess [0]->GetPropertyHandle (
                            L"PercentDiskReadTime",
                            &PercentDiskReadTimeType,
                            &disk.lPercentDiskReadTimeHandle )
                 )
         )
      {
        goto DISK_CLEANUP;
      }

      if (FAILED (hr = disk.apEnumAccess [0]->GetPropertyHandle (
                            L"PercentDiskWriteTime",
                            &PercentDiskWriteTimeType,
                            &disk.lPercentDiskWriteTimeHandle )
                 )
         )
      {
        goto DISK_CLEANUP;
      }

      if (FAILED (hr = disk.apEnumAccess [0]->GetPropertyHandle (
                            L"PercentIdleTime",
                            &PercentIdleTimeType,
                            &disk.lPercentIdleTimeHandle )
                 )
         )
      {
        goto DISK_CLEANUP;
      }
    }

    for (unsigned int i = 0; i < disk.dwNumReturned; i++)
    {
      uint64_t percent_read;
      uint64_t percent_write;
      uint64_t percent_load;
      uint64_t percent_idle;

      uint64_t bytes_sec;
      uint64_t bytes_read_sec;
      uint64_t bytes_write_sec;

      if (FAILED (hr = disk.apEnumAccess [i]->ReadQWORD (
                             disk.lPercentDiskReadTimeHandle,
                            &percent_read )
                 )
         )
      {
        goto DISK_CLEANUP;
      }

      if (FAILED (hr = disk.apEnumAccess [i]->ReadQWORD (
                             disk.lPercentDiskWriteTimeHandle,
                            &percent_write )
                 )
         )
      {
        goto DISK_CLEANUP;
      }

      if (FAILED (hr = disk.apEnumAccess [i]->ReadQWORD (
                             disk.lPercentDiskTimeHandle,
                            &percent_load )
                 )
         )
      {
        goto DISK_CLEANUP;
      }

      if (FAILED (hr = disk.apEnumAccess [i]->ReadQWORD (
                             disk.lPercentIdleTimeHandle,
                            &percent_idle )
                 )
         )
      {
        goto DISK_CLEANUP;
      }

      if (FAILED (hr = disk.apEnumAccess [i]->ReadQWORD (
                             disk.lDiskBytesPerSecHandle,
                            &bytes_sec )
                 )
         )
      {
        goto DISK_CLEANUP;
      }

      if (FAILED (hr = disk.apEnumAccess [i]->ReadQWORD (
                             disk.lDiskWriteBytesPerSecHandle,
                            &bytes_write_sec )
                 )
         )
      {
        goto DISK_CLEANUP;
      }

      if (FAILED (hr = disk.apEnumAccess [i]->ReadQWORD (
                             disk.lDiskReadBytesPerSecHandle,
                            &bytes_read_sec )
                 )
         )
      {
        goto DISK_CLEANUP;
      }

      long    bytes     =  0;
      wchar_t name [64] = { };

      if (FAILED (hr = disk.apEnumAccess [i]->ReadPropertyValue (
                             disk.lNameHandle,
                             sizeof (wchar_t) * 64,
                             &bytes,
                             reinterpret_cast <LPBYTE> (name) )
                 )
         )
      {
        goto DISK_CLEANUP;
      }

      WideCharToMultiByte (CP_OEMCP, 0, name, 31, disk.disks [i].name, 31, " ", nullptr);
      disk.disks [i].name [31] = '\0';

      if (i == 0)
        strcpy (disk.disks [i].name, "Total");

      disk.disks [i].name [15] = '\0';

      static SK::Framerate::Stats write_rate    [16];
      static SK::Framerate::Stats read_rate     [16];
      static SK::Framerate::Stats combined_rate [16];

      disk.disks [i].percent_idle   = (disk.disks [i].percent_idle   + percent_idle)   / 2;
      disk.disks [i].percent_load   = (disk.disks [i].percent_load   + percent_load)   / 2;
      disk.disks [i].percent_read   = (disk.disks [i].percent_read   + percent_read)   / 2;
      disk.disks [i].percent_write  = (disk.disks [i].percent_write  + percent_write)  / 2;

      write_rate    [i].addSample ( static_cast <double> (bytes_write_sec), now );
      read_rate     [i].addSample (static_cast <double> (bytes_read_sec),   now );
      combined_rate [i].addSample (static_cast <double> (bytes_sec),        now );

      double combined_mean = combined_rate [i].calcMean (2.0);
      double write_mean    = write_rate    [i].calcMean (2.0);
      double read_mean     = read_rate     [i].calcMean (2.0);

      disk.disks [i].bytes_sec       = isnan (combined_mean) ? 0ULL : static_cast <uint64_t> (combined_mean);
      disk.disks [i].write_bytes_sec = isnan (write_mean)    ? 0ULL : static_cast <uint64_t> (write_mean);
      disk.disks [i].read_bytes_sec  = isnan (read_mean)     ? 0ULL : static_cast <uint64_t> (read_mean);

      //disk.disks [i].bytes_sec       = (disk.disks [i].bytes_sec       + bytes_sec)       >> 1;
      //disk.disks [i].write_bytes_sec = (disk.disks [i].write_bytes_sec + bytes_write_sec) >> 1;
      //disk.disks [i].read_bytes_sec  = (disk.disks [i].read_bytes_sec  + bytes_read_sec)  >> 1;

      // Done with the object
      disk.apEnumAccess [i]->Release ();
      disk.apEnumAccess [i] = nullptr;
    }

    disk.num_disks = disk.dwNumReturned;
    disk.booting   = false;

    ++iter;

    COM::base.wmi.Unlock ();
  }

  COM::base.wmi.Lock ();

DISK_CLEANUP:
  //dll_log.Log (L" >> DISK_CLEANUP");

  if (disk.apEnumAccess != nullptr)
  {
    for (unsigned int i = 0; i < disk.dwNumReturned; i++)
    {
      if (disk.apEnumAccess [i] != nullptr)
      {
        disk.apEnumAccess [i]->Release ();
        disk.apEnumAccess [i] = nullptr;
      }
    }
    delete [] disk.apEnumAccess;
  }

  if (disk.pEnum)
  {
    disk.pEnum->Release ();
    disk.pEnum = nullptr;
  }

  if (disk.pConfig != nullptr)
  {
    disk.pConfig->Release ();
    disk.pConfig = nullptr;
  }

  if (disk.pRefresher != nullptr)
  {
    disk.pRefresher->Release ();
    disk.pRefresher = nullptr;
  }

  if (disk.hShutdownSignal != INVALID_HANDLE_VALUE)
  {
    CloseHandle (disk.hShutdownSignal);
    disk.hShutdownSignal = INVALID_HANDLE_VALUE;
  }

  COM::base.wmi.Unlock   ();

  return 0;
}



DWORD
WINAPI
SK_MonitorPagefile (LPVOID user)
{
  SK_Thread_SetCurrentPriority (THREAD_PRIORITY_LOWEST);
  SetCurrentThreadDescription  (L"[SK] WMI Pagefile Monitoring Thread");

  SetThreadPriorityBoost       (GetCurrentThread (), TRUE);

  SK_WMI_WaitForInit ();

  SK_AutoCOMInit auto_com;

  UNREFERENCED_PARAMETER (user);

  COM::base.wmi.Lock ();

  pagefile_perf_t&  pagefile = pagefile_stats;

                    pagefile.dwNumReturned = 0;
                    pagefile.dwNumObjects  = 0;

  const double update = config.pagefile.interval;

  HRESULT hr;

  if (FAILED (hr = CoCreateInstance_Original (
                     CLSID_WbemRefresher,
                     nullptr,
                     CLSCTX_INPROC_SERVER,
                     IID_IWbemRefresher, 
                     (void**) &pagefile.pRefresher )
             )
     )
  {
    config.pagefile.show = false;
    goto PAGEFILE_CLEANUP;
  }

  if (FAILED (hr = pagefile.pRefresher->QueryInterface (
                        IID_IWbemConfigureRefresher,
                        (void **)&pagefile.pConfig )
             )
     )
  {
    config.pagefile.show = false;
    goto PAGEFILE_CLEANUP;
  }

  // Add an enumerator to the refresher.
  if (FAILED (hr = pagefile.pConfig->AddEnum (
                     COM::base.wmi.pNameSpace,
                     L"Win32_PerfRawData_PerfOS_PagingFile",
                     0,
                     nullptr,
                     &pagefile.pEnum,
                     &pagefile.lID )
             )
     )
  {
    config.pagefile.show = false;
    goto PAGEFILE_CLEANUP;
  }

  pagefile.pConfig->Release ();
  pagefile.pConfig = nullptr;

  int iter = 0;

  pagefile.hShutdownSignal = CreateEvent (nullptr, FALSE, FALSE, L"Pagefile Monitor Shutdown Signal");

  COM::base.wmi.Unlock ();

  while (pagefile.lID != 0)
  {
    if (MsgWaitForMultipleObjects (1, const_cast <const HANDLE *> (&pagefile.hShutdownSignal), FALSE, ( DWORD (update * 1000.0) ), QS_ALLEVENTS) == WAIT_OBJECT_0)
      break;

    // Only poll WMI while the pagefile stats are shown
    if (! config.pagefile.show)
      continue;

    pagefile.dwNumReturned = 0;

    COM::base.wmi.Lock ();

    if (FAILED (hr = pagefile.pRefresher->Refresh (0L)))
    {
      config.pagefile.show = false;
      goto PAGEFILE_CLEANUP;
    }

    hr = pagefile.pEnum->GetObjects ( 0L,
                                      pagefile.dwNumObjects,
                                      pagefile.apEnumAccess,
                                      &pagefile.dwNumReturned );

    // If the buffer was not big enough,
    // allocate a bigger buffer and retry.
    if (hr == WBEM_E_BUFFER_TOO_SMALL 
        && pagefile.dwNumReturned > pagefile.dwNumObjects)
    {
      pagefile.apEnumAccess = new IWbemObjectAccess* [pagefile.dwNumReturned + 1];
      if (pagefile.apEnumAccess == nullptr)
      {
        hr = E_OUTOFMEMORY;
        goto PAGEFILE_CLEANUP;
      }

      SecureZeroMemory (pagefile.apEnumAccess,
                        pagefile.dwNumReturned * sizeof (IWbemObjectAccess *));

      pagefile.dwNumObjects = pagefile.dwNumReturned;

      if (FAILED (hr = pagefile.pEnum->GetObjects ( 0L,
                                                    pagefile.dwNumObjects,
                                                    pagefile.apEnumAccess,
                                                    &pagefile.dwNumReturned )
                 )
         )
      {
        config.pagefile.show = false;
        goto PAGEFILE_CLEANUP;
      }

      pagefile.dwNumReturned = std::min (pagefile.dwNumObjects, pagefile.dwNumReturned);
    }
    else
    {
      if (hr != WBEM_S_NO_ERROR)
      {
        hr = WBEM_E_NOT_FOUND;
        config.pagefile.show = false;
        goto PAGEFILE_CLEANUP;
      }
    }

    // First time through, get the handles.
    if (iter == 0)
    {
      if (pagefile.dwNumReturned <= 0)
      {
        dll_log.Log (L"[ WMI Wbem ] No pagefile exists");
        config.pagefile.show = false;
        goto PAGEFILE_CLEANUP;
      }

      if (pagefile.apEnumAccess [0] == nullptr)
      {
        dll_log.Log (L"[ WMI Wbem ] Pagefile apEnumAccess [0] = nullptr");
        config.pagefile.show = false;
        goto PAGEFILE_CLEANUP;
      }

      CIMTYPE NameType;

      CIMTYPE PercentUsageType;
      CIMTYPE PercentUsagePeakType;
      CIMTYPE PercentUsage_BaseType;

      if (FAILED (hr = pagefile.apEnumAccess [0]->GetPropertyHandle (
                            L"Name",
                            &NameType,
                            &pagefile.lNameHandle )
                 )
         )
      {
        config.pagefile.show = false;
        goto PAGEFILE_CLEANUP;
      }

      if (FAILED (hr = pagefile.apEnumAccess [0]->GetPropertyHandle (
                            L"PercentUsage",
                            &PercentUsageType,
                            &pagefile.lPercentUsageHandle )
                 )
         )
      {
        config.pagefile.show = false;
        goto PAGEFILE_CLEANUP;
      }

      if (FAILED (hr = pagefile.apEnumAccess [0]->GetPropertyHandle (
                            L"PercentUsagePeak",
                            &PercentUsagePeakType,
                            &pagefile.lPercentUsagePeakHandle )
                 )
         )
      {
        config.pagefile.show = false;
        goto PAGEFILE_CLEANUP;
      }

      if (FAILED (hr = pagefile.apEnumAccess [0]->GetPropertyHandle (
                            L"PercentUsage_Base",
                            &PercentUsage_BaseType,
                            &pagefile.lPercentUsage_BaseHandle )
                 )
         )
      {
        config.pagefile.show = false;
        goto PAGEFILE_CLEANUP;
      }
    }

    for (unsigned int i = 0; i < pagefile.dwNumReturned; i++)
    {
      DWORD size;
      DWORD usage;
      DWORD usage_peak;

      if (FAILED (hr = pagefile.apEnumAccess [i]->ReadDWORD (
                                 pagefile.lPercentUsageHandle,
                                 &usage )
                 )
         )
      {
        goto PAGEFILE_CLEANUP;
      }

      if (FAILED (hr = pagefile.apEnumAccess [i]->ReadDWORD (
                                 pagefile.lPercentUsagePeakHandle,
                                 &usage_peak )
                 )
         )
      {
        goto PAGEFILE_CLEANUP;
      }

      if (FAILED (hr = pagefile.apEnumAccess [i]->ReadDWORD (
                                 pagefile.lPercentUsage_BaseHandle,
                                 &size )
                 )
         )
      {
        goto PAGEFILE_CLEANUP;
      }

      long    bytes      =  0;
      wchar_t name [256] = { };

      if (FAILED (hr = pagefile.apEnumAccess [i]->ReadPropertyValue (
                             pagefile.lNameHandle,
                             sizeof (wchar_t) * 255,
                             &bytes,
                             reinterpret_cast <LPBYTE> (name) )
                 )
         )
      {
        config.pagefile.show = false;
        goto PAGEFILE_CLEANUP;
      }

      WideCharToMultiByte (CP_OEMCP, 0, name, 255, pagefile.pagefiles [i].name,
                           255, " ", nullptr);

      pagefile.pagefiles [i].name [31] = '\0';

      if (i == (pagefile.dwNumReturned - 1))
        strcpy (pagefile.pagefiles [i].name, "Total");

      pagefile.pagefiles [i].size  = (pagefile.pagefiles [i].size  + size) / 2;
      pagefile.pagefiles [i].usage = (pagefile.pagefiles [i].usage + usage)/ 2;
      pagefile.pagefiles [i].usage_peak
                                   = (pagefile.pagefiles [i].usage_peak
                                                                   +
                                                               usage_peak) / 2;

      // Done with the object
      pagefile.apEnumAccess [i]->Release ();
      pagefile.apEnumAccess [i] = nullptr;
    }

    pagefile.num_pagefiles = pagefile.dwNumReturned;
    pagefile.booting       = false;

    COM::base.wmi.Unlock ();

    ++iter;
  }

  COM::base.wmi.Lock ();

PAGEFILE_CLEANUP:
  //dll_log.Log (L" >> PAGEFILE_CLEANUP");

  if (pagefile.apEnumAccess != nullptr)
  {
    for (unsigned int i = 0; i < pagefile.dwNumReturned; i++)
    {
      if (pagefile.apEnumAccess [i] != nullptr)
      {
        pagefile.apEnumAccess [i]->Release ();
        pagefile.apEnumAccess [i] = nullptr;
      }
    }
    delete [] pagefile.apEnumAccess;
  }

  if (pagefile.pEnum)
  {
    pagefile.pEnum->Release ();
    pagefile.pEnum = nullptr;
  }

  if (pagefile.pConfig != nullptr)
  {
    pagefile.pConfig->Release ();
    pagefile.pConfig = nullptr;
  }

  if (pagefile.pRefresher != nullptr)
  {
    pagefile.pRefresher->Release ();
    pagefile.pRefresher = nullptr;
  }

  if (pagefile.hShutdownSignal != INVALID_HANDLE_VALUE)
  {
    CloseHandle (pagefile.hShutdownSignal);
    pagefile.hShutdownSignal = INVALID_HANDLE_VALUE;
  }

  COM::base.wmi.Unlock   ();

  return 0;
}






DWORD
WINAPI
SK_MonitorProcess (LPVOID user)
{
  SK_Thread_SetCurrentPriority (THREAD_PRIORITY_LOWEST);
  SetCurrentThreadDescription  (L"[SK] WMI Memory Monitoring Thread");

  SetThreadPriorityBoost       (GetCurrentThread (), TRUE);

  SK_WMI_WaitForInit ();

  SK_AutoCOMInit auto_com;

  UNREFERENCED_PARAMETER (user);

  COM::base.wmi.Lock ();

  process_stats_t& proc   = process_stats;
  const double     update = config.mem.interval;

  HRESULT hr;

  if (FAILED (hr = CoCreateInstance_Original (
                     CLSID_WbemRefresher,
                     nullptr,
                     CLSCTX_INPROC_SERVER,
                     IID_IWbemRefresher,
                     (void**) &proc.pRefresher )
             )
     )
  {
    SK_LOG0 ( ( L"Failed to create Refresher Instance (%s:%d) -- (hr=%x; %s)",
                  __FILEW__, __LINE__, hr, _com_error (hr).ErrorMessage () ),
                L" WMI Wbem " );
    goto PROC_CLEANUP;
  }

  if (FAILED (hr = proc.pRefresher->QueryInterface (
                     IID_IWbemConfigureRefresher,
                     (void **)&proc.pConfig )
             )
     )
  {
    SK_LOG0 ( ( L" Failed to Query Refresher Interface (%s:%d) -- (hr=%x; %s)",
                  __FILEW__, __LINE__, hr, _com_error (hr).ErrorMessage () ),
                L" WMI Wbem " );

    goto PROC_CLEANUP;
  }

  IWbemClassObject *pClassObj = nullptr;

  DWORD   dwProcessSize = MAX_PATH * 2;
  wchar_t wszProcessName [MAX_PATH * 2 + 1] = { };

  GetModuleFileNameW (0, wszProcessName, dwProcessSize);

  wchar_t* pwszShortName = wcsrchr (wszProcessName, L'\\') + 1;
  wchar_t* pwszTruncName = wcsrchr (pwszShortName,  L'.');

  if (pwszTruncName != nullptr)
    *pwszTruncName = L'\0';

  wchar_t      wszInstance [256] = { };
  _snwprintf ( wszInstance,
                255,
                  L"Win32_PerfFormattedData_PerfProc_Process.Name='%ws'",
                    pwszShortName );

  if (FAILED (hr = proc.pConfig->AddObjectByPath (
                     COM::base.wmi.pNameSpace,
                     wszInstance,
                     0,
                     nullptr,
                     &pClassObj,
                     nullptr )
             )
     )
  {
    SK_LOG0 ( ( L" Failed to AddObjectByPath (%s:%d) -- (hr=%x; %s)",
                  __FILEW__, __LINE__, hr, _com_error (hr).ErrorMessage () ),
                L" WMI Wbem " );

    goto PROC_CLEANUP;
  }

  if (FAILED (hr = pClassObj->QueryInterface ( IID_IWbemObjectAccess,
                                               (void **)(&proc.pAccess ) )
             )
     )
  {
    SK_LOG0 ( ( L" Failed to Query WbemObjectAccess Interface (%s:%d) -- (hr=%x; %s)",
                  __FILEW__, __LINE__, hr, _com_error (hr).ErrorMessage () ),
                L" WMI Wbem " );

    pClassObj->Release ();
    pClassObj = nullptr;

    goto PROC_CLEANUP;
  }

  pClassObj->Release ();
  pClassObj = nullptr;

  CIMTYPE variant;
  if (FAILED (hr = proc.pAccess->GetPropertyHandle ( L"PageFileBytes",
                                                     &variant,
                                                     &proc.hPageFileBytes )
             )
     )
  {
    goto PROC_CLEANUP;
  }

  if (FAILED (hr = proc.pAccess->GetPropertyHandle ( L"PageFileBytesPeak",
                                                     &variant,
                                                     &proc.hPageFileBytesPeak )
             )
     )
  {
    goto PROC_CLEANUP;
  }

  if (FAILED (hr = proc.pAccess->GetPropertyHandle ( L"ThreadCount",
                                                     &variant,
                                                     &proc.hThreadCount )
             )
     )
  {
    goto PROC_CLEANUP;
  }

  if (FAILED (hr = proc.pAccess->GetPropertyHandle ( L"PrivateBytes",
                                                     &variant,
                                                     &proc.hPrivateBytes )
             )
     )
  {
    goto PROC_CLEANUP;
  }

  if (FAILED (hr = proc.pAccess->GetPropertyHandle ( L"WorkingSetPeak",
                                                     &variant,
                                                     &proc.hWorkingSetPeak )
             )
     )
  {
    goto PROC_CLEANUP;
  }

  if (FAILED (hr = proc.pAccess->GetPropertyHandle ( L"WorkingSet",
                                                     &variant,
                                                     &proc.hWorkingSet )
             )
     )
  {
    goto PROC_CLEANUP;
  }

  if (FAILED (hr = proc.pAccess->GetPropertyHandle ( L"VirtualBytesPeak",
                                                     &variant,
                                                     &proc.hVirtualBytesPeak )
             )
     )
  {
    goto PROC_CLEANUP;
  }

  if (FAILED (hr = proc.pAccess->GetPropertyHandle ( L"VirtualBytes",
                                                     &variant,
                                                     &proc.hVirtualBytes )
             )
     )
  {
    goto PROC_CLEANUP;
  }

  proc.pConfig->Release ();
  proc.pConfig = nullptr;

  int iter = 0;

  proc.lID = 1;

  proc.hShutdownSignal =
    CreateEvent (nullptr, FALSE, FALSE, L"ProcMon Shutdown Signal");

  COM::base.wmi.Unlock ();

  while (proc.lID != 0)
  {
    // Sleep until ready
    if (MsgWaitForMultipleObjects (1, const_cast <const HANDLE *> (&proc.hShutdownSignal), FALSE, (DWORD (update * 1000.0)), QS_ALLEVENTS) == WAIT_OBJECT_0)
      break;

    // Only poll WMI while the data view is visible
    if (! config.mem.show)
      continue;

    COM::base.wmi.Lock ();

    if (FAILED (hr = proc.pRefresher->Refresh (0L)))
    {
      goto PROC_CLEANUP;
    }

    proc.pAccess->ReadQWORD ( proc.hVirtualBytes,
                                &proc.memory.virtual_bytes );
    proc.pAccess->ReadQWORD ( proc.hVirtualBytesPeak,
                                &proc.memory.virtual_bytes_peak );

    proc.pAccess->ReadQWORD ( proc.hWorkingSet,
                                &proc.memory.working_set );
    proc.pAccess->ReadQWORD ( proc.hWorkingSetPeak,
                                &proc.memory.working_set_peak );

    proc.pAccess->ReadQWORD ( proc.hPrivateBytes,
                                &proc.memory.private_bytes );

    proc.pAccess->ReadDWORD ( proc.hThreadCount,
                                (DWORD *)&proc.tasks.thread_count );

    proc.pAccess->ReadQWORD ( proc.hPageFileBytes,
                                &proc.memory.page_file_bytes );
    proc.pAccess->ReadQWORD ( proc.hPageFileBytesPeak,
                                &proc.memory.page_file_bytes_peak );

    proc.booting = false;

    ++iter;

    COM::base.wmi.Unlock ();
  }

  COM::base.wmi.Lock ();

PROC_CLEANUP:
  // dll_log.Log (L" >> PROC_CLEANUP");

  if (proc.pAccess != nullptr)
  {
    proc.pAccess->Release ();
    proc.pAccess = nullptr;
  }

  if (proc.pConfig != nullptr)
  {
    proc.pConfig->Release ();
    proc.pConfig = nullptr;
  }

  if (proc.pRefresher != nullptr)
  {
    proc.pRefresher->Release ();
    proc.pRefresher = nullptr;
  }

  if (proc.hShutdownSignal != INVALID_HANDLE_VALUE)
  {
    CloseHandle (proc.hShutdownSignal);
                 proc.hShutdownSignal = INVALID_HANDLE_VALUE;
  }

  COM::base.wmi.Unlock ();

  return 0;
}