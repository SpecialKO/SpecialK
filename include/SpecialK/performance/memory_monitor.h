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

#ifndef __SK__MEMORY_MONITOR_H__
#define __SK__MEMORY_MONITOR_H__

#include <Windows.h>
#include <cstdint>

#define _WIN32_DCOM
#include <Wbemidl.h>

struct WMI_refresh_instance_thread_t
{
           HANDLE          hThread         = INVALID_HANDLE_VALUE;
  volatile HANDLE          hShutdownSignal = INVALID_HANDLE_VALUE;

  IWbemRefresher          *pRefresher      = nullptr;
  IWbemConfigureRefresher *pConfig         = nullptr;
  IWbemObjectAccess       *pAccess         = nullptr;
  long                     lID             = 0;

  // Set to false after the first
  //  refresh iteration
  bool                     booting         = true;
};

struct process_stats_t : WMI_refresh_instance_thread_t
{
  long       hVirtualBytes        = 0L;
  long       hVirtualBytesPeak    = 0L;
  long       hWorkingSet          = 0L;
  long       hWorkingSetPeak      = 0L;
  long       hPrivateBytes        = 0L;
  long       hThreadCount         = 0L;
  long       hPageFileBytes       = 0L;
  long       hPageFileBytesPeak   = 0L;

  struct {
    uint64_t virtual_bytes        = 0ULL;
    uint64_t virtual_bytes_peak   = 0ULL;
    uint64_t working_set          = 0ULL;
    uint64_t working_set_peak     = 0ULL;
    uint64_t private_bytes        = 0ULL;
    uint64_t page_file_bytes      = 0ULL;
    uint64_t page_file_bytes_peak = 0ULL;
  } memory;

  struct {
    uint32_t thread_count         = 0ui32;
  } tasks;
};

DWORD
WINAPI
SK_MonitorProcess (LPVOID user);


process_stats_t&
__SK_WMI_ProcessStats (void);


#endif /* __SK__MEMORY_MONITOR_H__ */