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


#include <SpecialK/config.h>
#include <SpecialK/io_monitor.h>
#include <SpecialK/memory_monitor.h>

#include <SpecialK/log.h>

#undef min
#undef max

#pragma comment (lib, "wbemuuid.lib")

namespace COM {
  struct Base {
    volatile ULONG     init;

    struct WMI {
      volatile LONG    init;

      IWbemServices*   pNameSpace;
      IWbemLocator*    pWbemLocator;
      BSTR             bstrNameSpace;

      HANDLE           hServerThread;

      void Lock         (void);
      void Unlock       (void);
    } wmi;
  } extern base;
}

extern bool SK_InitWMI     (void);

process_stats_t process_stats;

unsigned int
__stdcall
SK_MonitorProcess (LPVOID user)
{
  SK_AutoCOMInit auto_com;

  UNREFERENCED_PARAMETER (user);

  if (! SK_InitWMI ())
    return std::numeric_limits <unsigned int>::max ();

  COM::base.wmi.Lock ();

  process_stats_t& proc   = process_stats;
  const double     update = config.mem.interval;

  HRESULT hr;

  if (FAILED (hr = CoCreateInstance (
                     CLSID_WbemRefresher,
                     NULL,
                     CLSCTX_INPROC_SERVER,
                     IID_IWbemRefresher,
                     (void**) &proc.pRefresher )
             )
     )
  {
    dll_log.Log(L"[ WMI Wbem ] Failed to create Refresher Instance (%s:%d) -- 0x%X",
      __FILEW__, __LINE__, hr);
    goto PROC_CLEANUP;
  }

  if (FAILED (hr = proc.pRefresher->QueryInterface (
                     IID_IWbemConfigureRefresher,
                     (void **)&proc.pConfig )
             )
     )
  {
    dll_log.Log(L"[ WMI Wbem ] Failed to Query Refresher Interface (%s:%d) -- 0x%X",
      __FILEW__, __LINE__, hr);
    goto PROC_CLEANUP;
  }

  IWbemClassObject *pClassObj = nullptr;

  HANDLE hProc = GetCurrentProcess ();

  DWORD   dwProcessSize = MAX_PATH;
  wchar_t wszProcessName [MAX_PATH];

  QueryFullProcessImageName (hProc, 0, wszProcessName, &dwProcessSize);

  wchar_t* pwszShortName = wcsrchr (wszProcessName, L'\\') + 1;
  wchar_t* pwszTruncName = wcsrchr (pwszShortName, L'.');

  if (pwszTruncName != nullptr)
    *pwszTruncName = L'\0';

  wchar_t wszInstance [512];
  wsprintf ( wszInstance,
               L"Win32_PerfFormattedData_PerfProc_Process.Name='%ws'",
                 pwszShortName );

  if (FAILED (hr = proc.pConfig->AddObjectByPath (
                     COM::base.wmi.pNameSpace,
                     wszInstance,
                     0,
                     0,
                     &pClassObj,
                     0 )
             )
     )
  {
    dll_log.Log(L"[ WMI Wbem ] Failed to AddObjectByPath (%s:%d) -- 0x%X",
      __FILEW__, __LINE__, hr);
    goto PROC_CLEANUP;
  }

  if (FAILED (hr = pClassObj->QueryInterface ( IID_IWbemObjectAccess,
                                               (void **)(&proc.pAccess ) )
             )
     )
  {
    dll_log.Log(L"[ WMI Wbem ] Failed to Query WbemObjectAccess Interface (%s:%d)"
                L" -- 0x%X",
      __FILEW__, __LINE__, hr);
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

  COM::base.wmi.Unlock ();

  while (proc.lID != 0)
  {
    // Sleep until ready
    Sleep (DWORD (update * 1000.0));

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

    ++iter;

    COM::base.wmi.Unlock ();
  }

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

  COM::base.wmi.Unlock   ();

  return 0;
}