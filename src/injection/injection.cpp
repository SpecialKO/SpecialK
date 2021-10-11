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

#include <SpecialK/injection/injection.h>

#include <SpecialK/diagnostics/debug_utils.h>
#include <winternl.h>

#include <regex>
#include <sddl.h>

#define SK_INVALID_HANDLE nullptr

NtUserSetWindowsHookEx_pfn    NtUserSetWindowsHookEx    = nullptr;
NtUserCallNextHookEx_pfn      NtUserCallNextHookEx      = nullptr;
NtUserUnhookWindowsHookEx_pfn NtUserUnhookWindowsHookEx = nullptr;

HMODULE hModHookInstance = nullptr;

// It's not possible to store a structure in the shared data segment.
//
//   This struct will be filled-in when SK boots up using the loose mess of
//     variables below, in order to make working with that data less insane.
//
SK_InjectionRecord_s __SK_InjectionHistory [MAX_INJECTED_PROC_HISTORY] = { 0 };

static auto constexpr x = sizeof (SK_InjectionRecord_s);

#pragma data_seg (".SK_Hooks")
extern "C"
{
  DWORD        dwHookPID  = 0x0;     // Process that owns the CBT hook
  HHOOK        hHookCBT   = nullptr; // CBT hook
  BOOL         bAdmin     = FALSE;   // Is SKIM64 able to inject into admin apps?

  LONG         g_sHookedPIDs [MAX_INJECTED_PROCS]                 =  {   0   };

  wchar_t      __SK_InjectionHistory_name       [MAX_INJECTED_PROC_HISTORY * MAX_PATH] =  {   0   };
  wchar_t      __SK_InjectionHistory_win_title  [MAX_INJECTED_PROC_HISTORY * 128     ] =  {   0   };
  DWORD        __SK_InjectionHistory_ids        [MAX_INJECTED_PROC_HISTORY]            =  {   0   };
  __time64_t   __SK_InjectionHistory_inject     [MAX_INJECTED_PROC_HISTORY]            =  {   0   };
  __time64_t   __SK_InjectionHistory_eject      [MAX_INJECTED_PROC_HISTORY]            =  {   0   };
  bool         __SK_InjectionHistory_crash      [MAX_INJECTED_PROC_HISTORY]            =  { false };

  ULONG64      __SK_InjectionHistory_frames     [MAX_INJECTED_PROC_HISTORY]            =  {   0   };
  SK_RenderAPI __SK_InjectionHistory_api        [MAX_INJECTED_PROC_HISTORY]            =  { SK_RenderAPI::Reserved };
  AppId_t      __SK_InjectionHistory_AppId      [MAX_INJECTED_PROC_HISTORY]            =  {   0   };
  wchar_t      __SK_InjectionHistory_UwpPackage [MAX_INJECTED_PROC_HISTORY *
                                                         PACKAGE_FULL_NAME_MAX_LENGTH] =  {   0   };

  __declspec (dllexport) volatile LONG SK_InjectionRecord_s::count                 =  0L;
  __declspec (dllexport) volatile LONG SK_InjectionRecord_s::rollovers             =  0L;

  __declspec (dllexport)          wchar_t whitelist_patterns [16 * MAX_PATH] = { 0 };
  __declspec (dllexport)          int     whitelist_count                    =   0;
  __declspec (dllexport)          wchar_t blacklist_patterns [16 * MAX_PATH] = { 0 };
  __declspec (dllexport)          int     blacklist_count                    =   0;
  __declspec (dllexport) volatile LONG    injected_procs                     =   0;

  constexpr LONG MAX_HOOKED_PROCS = 262144;

  // Recordkeeping on processes with CBT hooks; required to release the DLL
  //   in any process that has become suspended since hook install.
  volatile LONG  num_hooked_pids                =   0;
  volatile DWORD hooked_pids [MAX_HOOKED_PROCS] = { };
};
#pragma data_seg ()
#pragma comment  (linker, "/SECTION:.SK_Hooks,RWS")

// Change in design, we will use mapped memory instead of DLL shared memory in
// versions 21.6.x+ in order to communicate between 32-bit and 64-bit apps.
volatile HANDLE hMapShared = INVALID_HANDLE_VALUE;


extern void SK_Process_Snapshot    (void);
extern bool SK_Process_IsSuspended (DWORD dwPid);
extern bool SK_Process_Suspend     (DWORD dwPid);
extern bool SK_Process_Resume      (DWORD dwPid);

extern volatile LONG  __SK_HookContextOwner;

SK_InjectionRecord_s*
__stdcall
SK_Inject_GetRecordByIdx (int idx)
{
  return
    SK_Inject_GetRecord (
      __SK_InjectionHistory_ids [idx]
    );
}

SK_InjectionRecord_s*
__stdcall
SK_Inject_GetRecord (DWORD dwPid)
{
  for ( int idx = 0                         ;
            idx < MAX_INJECTED_PROC_HISTORY ;
          ++idx )
  {
    if ( __SK_InjectionHistory_ids [idx] == dwPid )
    {
      wcsncpy_s (__SK_InjectionHistory [idx].process.name,     MAX_PATH-1, &__SK_InjectionHistory_name       [idx * MAX_PATH], _TRUNCATE);
      wcsncpy_s (__SK_InjectionHistory [idx].process.win_title,       128, &__SK_InjectionHistory_win_title  [idx * 128     ], _TRUNCATE);
      wcsncpy_s (__SK_InjectionHistory [idx].platform.uwp_full_name,
                                             PACKAGE_FULL_NAME_MAX_LENGTH, &__SK_InjectionHistory_UwpPackage [idx *
                                             PACKAGE_FULL_NAME_MAX_LENGTH                                                   ], _TRUNCATE);

                 __SK_InjectionHistory [idx].process.id             = __SK_InjectionHistory_ids     [idx];
                 __SK_InjectionHistory [idx].process.inject         = __SK_InjectionHistory_inject  [idx];
                 __SK_InjectionHistory [idx].process.eject          = __SK_InjectionHistory_eject   [idx];
                 __SK_InjectionHistory [idx].process.crashed        = __SK_InjectionHistory_crash   [idx];

                 __SK_InjectionHistory [idx].render.api             = __SK_InjectionHistory_api     [idx];
                 __SK_InjectionHistory [idx].render.frames          = __SK_InjectionHistory_frames  [idx];

                 __SK_InjectionHistory [idx].platform.steam_appid   = __SK_InjectionHistory_AppId   [idx];

      return    &__SK_InjectionHistory [idx];
    }
  }

  return nullptr;
}

HRESULT
__stdcall
SK_Inject_AuditRecord ( DWORD                 dwPid,
                        SK_InjectionRecord_s *pData,
                        size_t                cbSize )
{
  if (cbSize == sizeof (SK_InjectionRecord_s))
  {
    for ( int idx = 0                         ;
              idx < MAX_INJECTED_PROC_HISTORY ;
            ++idx )
    {
      if ( __SK_InjectionHistory [idx].process.id == dwPid )
      { wcsncpy_s (
           __SK_InjectionHistory [idx].process.name, MAX_PATH-1, pData->process.name,           _TRUNCATE);
        wcsncpy_s (
           __SK_InjectionHistory [idx].process.win_title,   128, pData->process.win_title,      _TRUNCATE);
        wcsncpy_s (
           __SK_InjectionHistory [idx].platform.uwp_full_name,
                                   PACKAGE_FULL_NAME_MAX_LENGTH, pData->platform.uwp_full_name, _TRUNCATE);

        wcsncpy_s (&__SK_InjectionHistory_name         [idx * MAX_PATH], MAX_PATH-1,
                    __SK_InjectionHistory [idx].process.name,           _TRUNCATE);
        wcsncpy_s (&__SK_InjectionHistory_win_title    [idx * 128     ], 128,
                    __SK_InjectionHistory [idx].process.win_title,      _TRUNCATE);
        wcsncpy_s (&__SK_InjectionHistory_UwpPackage [idx * PACKAGE_FULL_NAME_MAX_LENGTH],
                                                            PACKAGE_FULL_NAME_MAX_LENGTH,
                    __SK_InjectionHistory [idx].platform.uwp_full_name, _TRUNCATE);


         __SK_InjectionHistory_inject [idx] = pData->process.inject;
         __SK_InjectionHistory_eject  [idx] = pData->process.eject;
         __SK_InjectionHistory_crash  [idx] = pData->process.crashed;

         __SK_InjectionHistory_api    [idx] = pData->render.api;
         __SK_InjectionHistory_frames [idx] = pData->render.frames;

         __SK_InjectionHistory_AppId  [idx] = pData->platform.steam_appid;


         __SK_InjectionHistory [idx].process.id           = __SK_InjectionHistory_ids    [idx];
         __SK_InjectionHistory [idx].process.inject       = __SK_InjectionHistory_inject [idx];
         __SK_InjectionHistory [idx].process.eject        = __SK_InjectionHistory_eject  [idx];
         __SK_InjectionHistory [idx].process.crashed      = __SK_InjectionHistory_crash  [idx];

         __SK_InjectionHistory [idx].render.api           = __SK_InjectionHistory_api    [idx];
         __SK_InjectionHistory [idx].render.frames        = __SK_InjectionHistory_frames [idx];
         __SK_InjectionHistory [idx].platform.steam_appid = __SK_InjectionHistory_AppId  [idx];

        return S_OK;
      }
    }
  }

  return E_NOINTERFACE;
}

LONG local_record = 0;

void
SK_Inject_InitShutdownEvent (void)
{
  if (dwHookPID == 0)
  {
    bAdmin =
      SK_IsAdmin ();

    SECURITY_ATTRIBUTES
      sattr          = { };
      sattr.nLength              = sizeof (SECURITY_ATTRIBUTES);
      sattr.bInheritHandle       = FALSE;
      sattr.lpSecurityDescriptor = nullptr;

    HANDLE hTeardown =
      SK_CreateEvent ( nullptr,
        TRUE, FALSE,
          SK_RunLHIfBitness (32, LR"(Local\SK_GlobalHookTeardown32)",
                                 LR"(Local\SK_GlobalHookTeardown64)") );

    if ( reinterpret_cast <intptr_t> (hTeardown) > 0 &&
                                 GetLastError () == ERROR_ALREADY_EXISTS )
    {
      ResetEvent (hTeardown);
    }

    dwHookPID =
      GetCurrentProcessId ();
  }
}


bool
__stdcall
SK_IsInjected (bool set) noexcept
{
// Indicates that the DLL is injected purely as a hooker, rather than
//   as a wrapper DLL.
  static std::atomic_bool __injected = false;

  if (__injected)
    return true;

  if (set)
  {
    __injected               = true;
  //SK_Inject_AddressManager = new SK_Inject_AddressCacheRegistry ();
  }

  return set;
}


void
SK_Inject_ValidateProcesses (void)
{
  for (volatile LONG& hooked_pid : g_sHookedPIDs)
  {
    SK_AutoHandle hProc (
      OpenProcess ( PROCESS_QUERY_INFORMATION, FALSE,
                      ReadAcquire (&hooked_pid) )
                  );

    if (reinterpret_cast <intptr_t> (hProc.m_h) <= 0)//== INVALID_HANDLE_VALUE)
    {
      ReadAcquire (&hooked_pid);
    }

    else
    {
      DWORD                           dwExitCode  = STILL_ACTIVE;
      GetExitCodeProcess (hProc.m_h, &dwExitCode);
      if (                            dwExitCode != STILL_ACTIVE)
      {
        ReadAcquire (&hooked_pid);
      }
    }
  }
}

void
SK_Inject_ReleaseProcess (void)
{
  if (! SK_IsInjected ())
    return;

  for (volatile LONG& hooked_pid : g_sHookedPIDs)
  {
    InterlockedCompareExchange (&hooked_pid, 0, GetCurrentProcessId ());
  }

  _time64 (&__SK_InjectionHistory_eject [local_record]);

  __SK_InjectionHistory_api    [local_record] = SK_GetCurrentRenderBackend ().api;
  __SK_InjectionHistory_frames [local_record] = SK_GetFramesDrawn          ();
  __SK_InjectionHistory_crash  [local_record] = SK_Debug_IsCrashing        ();

#ifdef _DEBUG
  dll_log.Log (L"Injection Release (%u, %lu, %s)", (unsigned int)SK_GetCurrentRenderBackend ().api,
                                                                 SK_GetFramesDrawn          (),
                                                                 SK_Debug_IsCrashing        () ? L"Crash" : L"Normal" );
#endif

  //local_record.input.xinput  = SK_Input_
  // ...
  // ...
  // ...
  // ...

  //FreeLibrary (hModHookInstance);
}


DWORD
SetPermissions (wchar_t* wszFilePath)
{
  PACL                 pOldDACL = nullptr,
                       pNewDACL = nullptr;
  PSECURITY_DESCRIPTOR pSD      = nullptr;
  EXPLICIT_ACCESS      eaAccess = {     };
  SECURITY_INFORMATION siInfo   = DACL_SECURITY_INFORMATION;
  DWORD                dwResult = ERROR_SUCCESS;
  PSID                 pSID     = nullptr;

  // Get a pointer to the existing DACL
  dwResult =
    GetNamedSecurityInfo ( wszFilePath,
                            SE_FILE_OBJECT,
                            DACL_SECURITY_INFORMATION,
                              nullptr, nullptr,
                                &pOldDACL, nullptr,
                                  &pSD );

  if (dwResult != ERROR_SUCCESS)
    return dwResult;

  // Get the SID for ALL APPLICATION PACKAGES using its SID string
  ConvertStringSidToSid (L"S-1-15-2-1", &pSID);

  if (pSID == nullptr)
    return dwResult;

  eaAccess.grfAccessPermissions = GENERIC_READ | GENERIC_EXECUTE;
  eaAccess.grfAccessMode        = SET_ACCESS;
  eaAccess.grfInheritance       = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
  eaAccess.Trustee.TrusteeForm  = TRUSTEE_IS_SID;
  eaAccess.Trustee.TrusteeType  = TRUSTEE_IS_WELL_KNOWN_GROUP;
  eaAccess.Trustee.ptstrName    = static_cast <LPWSTR> (pSID);

  // Create a new ACL that merges the new ACE into the existing DACL
  dwResult =
    SetEntriesInAcl (1, &eaAccess, pOldDACL, &pNewDACL);

  if (ERROR_SUCCESS != dwResult)
    return dwResult;

  // Attach the new ACL as the object's DACL
  dwResult =
    SetNamedSecurityInfo ( wszFilePath,
                             SE_FILE_OBJECT, siInfo,
                               nullptr, nullptr,
                                 pNewDACL, nullptr );

  return
    dwResult;
}

void
SK_Inject_AcquireProcess (void)
{
  if (! SK_IsInjected ())
    return;

  wchar_t                            wszDllFullName [MAX_PATH + 2] = { };
  GetModuleFileName ( __SK_hModSelf, wszDllFullName, MAX_PATH );
  SetPermissions    (                wszDllFullName           );

  for (volatile LONG& hooked_pid : g_sHookedPIDs)
  {
    if (! InterlockedCompareExchange (&hooked_pid, GetCurrentProcessId (), 0))
    {
      ULONG injection_idx =
        InterlockedIncrement (&SK_InjectionRecord_s::count);

      // Rollover and start erasing the oldest history
      if (injection_idx >= MAX_INJECTED_PROC_HISTORY)
      {
        if (InterlockedCompareExchange (&SK_InjectionRecord_s::count, 0, injection_idx))
        {
          injection_idx = 1;
          InterlockedIncrement (&SK_InjectionRecord_s::rollovers);
        }

        else
          injection_idx = InterlockedIncrement (&SK_InjectionRecord_s::count);
      }

      local_record =
        injection_idx - 1;

                __SK_InjectionHistory_ids    [local_record] = GetCurrentProcessId ();
      _time64 (&__SK_InjectionHistory_inject [local_record]);

      wchar_t                            wszHostFullName [MAX_PATH + 2] = { };
      GetModuleFileName ( __SK_hModHost, wszHostFullName, MAX_PATH );
      PathStripPath     (                wszHostFullName           );

      wcsncpy_s (&__SK_InjectionHistory_name [local_record * MAX_PATH], MAX_PATH,
                                         wszHostFullName,             _TRUNCATE);

      wchar_t                                wszPackageName [PACKAGE_FULL_NAME_MAX_LENGTH] = { };
      UINT32                          uiLen                                                =  0 ;
      if (GetCurrentPackageFullName (&uiLen, wszPackageName) != APPMODEL_ERROR_NO_PACKAGE)
      {
        wcsncpy_s (&__SK_InjectionHistory_UwpPackage [
                                              local_record * PACKAGE_FULL_NAME_MAX_LENGTH],
                                      uiLen, wszPackageName,                    _TRUNCATE);
      }

      // Hold a reference so that removing the CBT hook doesn't crash the software
      GetModuleHandleEx (
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
            (LPCWSTR)&SK_Inject_AcquireProcess,
                             &hModHookInstance
                        );

      break;
    }
  }
}

bool
SK_Inject_IsInvadingProcess (DWORD dwThreadId)
{
  for (volatile LONG& hooked_pid : g_sHookedPIDs)
  {
    if (ReadAcquire (&hooked_pid) == gsl::narrow_cast <LONG> (dwThreadId))
      return true;
  }

  return false;
}

HHOOK
SKX_GetCBTHook (void) noexcept
{
  return hHookCBT;
}

// Lazy-init
HANDLE
SK_Inject_GetSharedMemory (void)
{
  HANDLE hLocalMap =
    ReadPointerAcquire (&hMapShared);

  if (hLocalMap != INVALID_HANDLE_VALUE)
    return hLocalMap;

  HANDLE hExistingFile =
    OpenFileMapping (FILE_MAP_ALL_ACCESS, FALSE, LR"(Local\SK_GlobalSharedMemory_v1)");

  if (hExistingFile != 0)
  {
    if ( INVALID_HANDLE_VALUE ==
           InterlockedCompareExchangePointer (&hMapShared, hExistingFile, INVALID_HANDLE_VALUE) )
    {
      WritePointerRelease (&hMapShared, hExistingFile);

      return hExistingFile;
    }

    CloseHandle (hExistingFile);
  }

  HANDLE hNewFile  =
    CreateFileMapping ( INVALID_HANDLE_VALUE, nullptr,
                           PAGE_READWRITE, 0, 4096,
                             LR"(Local\SK_GlobalSharedMemory_v1)" );

  if (hNewFile != 0)
  {
    if ( INVALID_HANDLE_VALUE ==
           InterlockedCompareExchangePointer (&hMapShared, hNewFile, INVALID_HANDLE_VALUE) )
    {
      return hNewFile;
    }

    CloseHandle (hNewFile);
  }

  return SK_INVALID_HANDLE;
}

static volatile LPVOID _pSharedMemoryView = nullptr;

LPVOID
SK_Inject_GetViewOfSharedMemory (void)
{
  LPVOID lpLocalView =
    ReadPointerAcquire (&_pSharedMemoryView);

  if (lpLocalView != nullptr)
    return lpLocalView;

  HANDLE hMap =
    SK_Inject_GetSharedMemory ();

  if (hMap != nullptr)
  {
    lpLocalView =
      MapViewOfFile (hMap, FILE_MAP_ALL_ACCESS, 0, 0, 4096);

    if (lpLocalView != nullptr)
    {
      if ( nullptr ==
             InterlockedCompareExchangePointer (&_pSharedMemoryView, lpLocalView, nullptr) )
      {
        return lpLocalView;
      }

      UnmapViewOfFile (lpLocalView);
    }

    return
      ReadPointerAcquire (&_pSharedMemoryView);
  }

  return nullptr;
}

void
SK_Inject_CleanupSharedMemory (void)
{
  WritePointerRelease (&_pSharedMemoryView, nullptr);

  HANDLE hLocalMap =
    InterlockedCompareExchangePointer (&hMapShared, INVALID_HANDLE_VALUE, ReadPointerAcquire (&hMapShared));

  if (hLocalMap != INVALID_HANDLE_VALUE)
  {
    UnmapViewOfFile (
      SK_Inject_GetViewOfSharedMemory ()
    );

    CloseHandle (hLocalMap);
  }
}



HWND SK_Inject_GetExplorerWindow (void)
{
  SK_SharedMemory_v1 *pShared =
    (SK_SharedMemory_v1 *)SK_Inject_GetViewOfSharedMemory ();

  if (pShared != nullptr)
  {
    return
      (HWND)ULongToHandle (pShared->SystemWide.hWndExplorer);
  }

  return 0;
}

UINT SK_Inject_GetExplorerRaiseMsg (void)
{
  SK_SharedMemory_v1 *pShared =
    (SK_SharedMemory_v1 *)SK_Inject_GetViewOfSharedMemory ();

  if (pShared != nullptr)
  {
    return
      (UINT)pShared->SystemWide.uMsgExpRaise;
  }

  return 0;
}

UINT SK_Inject_GetExplorerLowerMsg (void)
{
  SK_SharedMemory_v1 *pShared =
    (SK_SharedMemory_v1 *)SK_Inject_GetViewOfSharedMemory ();

  if (pShared != nullptr)
  {
    return
      (UINT)pShared->SystemWide.uMsgExpLower;
  }

  return 0;
}

HWND
SK_Inject_GetFocusWindow (void)
{
  SK_SharedMemory_v1* pSharedMem =
    (SK_SharedMemory_v1 *)SK_Inject_GetViewOfSharedMemory ();

  if (pSharedMem != nullptr)
  {
    return
      (HWND)ULongToHandle (pSharedMem->SystemWide.hWndFocus);
  }

  return 0;
}

void
SK_Inject_SetFocusWindow (HWND hWndFocus)
{
  SK_SharedMemory_v1* pSharedMem =
    (SK_SharedMemory_v1 *)SK_Inject_GetViewOfSharedMemory ();

  if (pSharedMem != nullptr)
  {
    pSharedMem->SystemWide.hWndFocus =
      HandleToULong (hWndFocus);
  }
}


#include <SpecialK/render/present_mon/TraceSession.hpp>

std::string
SK_Etw_RegisterSession (const char* szPrefix, bool bReuse)
{
  std::string session_name; // _NotAValidSession;

  if (StrStrIA (szPrefix, "SK_PresentMon") != nullptr)
  {
    SK_SharedMemory_v1* pSharedMem =
   (SK_SharedMemory_v1 *)SK_Inject_GetViewOfSharedMemory ();

    if (pSharedMem)
    {
      HANDLE hMutex =
        CreateMutexW ( nullptr, FALSE, LR"(Local\SK_EtwMutex0)" );

      if (hMutex != 0)
      {
        if (WaitForSingleObject (hMutex, INFINITE) == WAIT_OBJECT_0)
        {
          auto __MaxPresentMonSessions =
            SK_SharedMemory_v1::EtwSessionList_s::__MaxPresentMonSessions;

          auto first_free_idx   = __MaxPresentMonSessions,
               first_uninit_idx = __MaxPresentMonSessions;

          for ( int i = 0;   i < __MaxPresentMonSessions;
                           ++i )
          {
            if ( ( first_free_idx == __MaxPresentMonSessions ) &&
                     pSharedMem->EtwSessions.PresentMon [i].dwPid != 0x0 )
            {
              HANDLE hProcess =
                OpenProcess ( PROCESS_QUERY_INFORMATION, FALSE,
                                pSharedMem->EtwSessions.PresentMon [i].dwPid );

              DWORD dwExitCode = 0x0;

              if (                      hProcess != 0          &&
                   GetExitCodeProcess ( hProcess, &dwExitCode) &&
                                                   dwExitCode  != STILL_ACTIVE )
              {
                first_free_idx = i;
              }

              else if (hProcess == 0)
              {
                first_free_idx = i;
              }

              if (         hProcess != 0)
              CloseHandle (hProcess);
            }

            else if ( pSharedMem->EtwSessions.PresentMon [i].dwPid  == 0x0 )
            {
              if (   *pSharedMem->EtwSessions.PresentMon [i].szName == '\0')
              {
                if (first_uninit_idx == __MaxPresentMonSessions)
                    first_uninit_idx = i;
              }

              else if (first_free_idx == __MaxPresentMonSessions)
                       first_free_idx = i;
            }
          }

          int idx =
            std::min (        first_uninit_idx,
                       bReuse ? first_free_idx :
                     __MaxPresentMonSessions - 1 );
          if (idx != __MaxPresentMonSessions)
          {
            auto *pEtwSession =
              &(pSharedMem->EtwSessions.PresentMon [idx]);

            bool isNewSession =
                 (pEtwSession->dwPid == 0x0);
                  pEtwSession->dwPid = GetCurrentProcessId ();

            if (isNewSession && *pEtwSession->szName == '\0')
            {
              pEtwSession->dwSequence =
               pSharedMem->EtwSessions.SessionControl.SequenceId++;

              session_name =
                SK_FormatString ( "%hs [%lu]",
                  szPrefix, idx );
            }

            else
            {
                             pEtwSession->dwSequence =
                  pSharedMem->EtwSessions.SessionControl.SequenceId++;
              session_name = pEtwSession->szName;
            }
          }
        }

        CloseHandle (hMutex);
      }
    }
  }

  return session_name;
}

bool
SK_Etw_UnregisterSession (const char* szPrefix)
{
  int unregistered_count = 0;

  if (StrStrIA (szPrefix, "SK_PresentMon") != nullptr)
  {
    SK_SharedMemory_v1* pSharedMem =
   (SK_SharedMemory_v1 *)SK_Inject_GetViewOfSharedMemory ();

    if (pSharedMem)
    {
      HANDLE hMutex =
        CreateMutexW ( nullptr, FALSE, LR"(Local\SK_EtwMutex0)" );

      if (hMutex != 0)
      {
        if (WaitForSingleObject (hMutex, INFINITE) == WAIT_OBJECT_0)
        {
          for ( int i = 0;
                    i < SK_SharedMemory_v1::EtwSessionList_s::__MaxPresentMonSessions;
                  ++i )
          {
            auto* pEtwSession =
                &(pSharedMem->EtwSessions.PresentMon [i]);

            if (pEtwSession->dwPid == GetCurrentProcessId ())
            {
              *pEtwSession->szName = '\0';
               pEtwSession->dwPid  = 0x00;

              unregistered_count++;
            }
          }
        }

        CloseHandle (hMutex);
      }
    }
  }

  return
    unregistered_count != 0;
}

typedef VOID (NTAPI *RtlInitUnicodeString_pfn)
      ( PUNICODE_STRING DestinationString,
        PCWSTR          SourceString );

void
SK_Inject_RenameProcess (void)
{
  static RtlInitUnicodeString_pfn
         SK_InitUnicodeString =
        (RtlInitUnicodeString_pfn)SK_GetProcAddress (
                           L"NtDll",
                            "RtlInitUnicodeString"  );

  auto pPeb =
    (SK_PPEB)NtCurrentTeb ()->ProcessEnvironmentBlock;

  //RtlAcquirePebLock ();
  //EnterCriticalSection (pPeb->FastPebLock);

  if (SK_InitUnicodeString != nullptr)
  {
    //SK_InitUnicodeString (
    //  (PUNICODE_STRING)&pPeb->ProcessParameters->ImagePathName,
    //  L"Special K Backend Service Host"
    //);

    SK_InitUnicodeString (
      (PUNICODE_STRING)&pPeb->ProcessParameters->CommandLine,
      SK_RunLHIfBitness ( 32, L"32-bit Special K Backend Service Host",
                              L"64-bit Special K Backend Service Host" )
    );

    pPeb->ProcessParameters->DllPath.Length       = pPeb->ProcessParameters->CommandLine.Length;
    pPeb->ProcessParameters->ImagePathName.Length = pPeb->ProcessParameters->CommandLine.Length;
    pPeb->ProcessParameters->WindowTitle.Length   = pPeb->ProcessParameters->CommandLine.Length;

    pPeb->ProcessParameters->DllPath.Buffer       = pPeb->ProcessParameters->CommandLine.Buffer;
    pPeb->ProcessParameters->ImagePathName.Buffer = pPeb->ProcessParameters->CommandLine.Buffer;
    pPeb->ProcessParameters->WindowTitle.Buffer   = pPeb->ProcessParameters->CommandLine.Buffer;
  }

  //LeaveCriticalSection (pPeb->FastPebLock);
  //RtlReleasePebLock ();
}

#if 0
#include <winternl.h>                   //PROCESS_BASIC_INFORMATION

bool
ReadMem ( void *addr,
          void *buf,
          int   size )
{
  return
    ReadProcessMemory ( GetCurrentProcess (), addr,
                                               buf, size,
                                                 nullptr ) != FALSE;
}

using NtQueryInformationProcess_pfn =
  NTSTATUS (NTAPI *)(HANDLE,PROCESSINFOCLASS,PVOID,ULONG,PULONG);

int
GetModuleReferenceCount (HMODULE hModToTest)
{
  LdrFindEntryForAddress_pfn
  LdrFindEntryForAddress =
 (LdrFindEntryForAddress_pfn)SK_GetProcAddress ( L"NtDll",
 "LdrFindEntryForAddress"                      );

  if (LdrFindEntryForAddress != nullptr)
  {
    PLDR_DATA_TABLE_ENTRY__SK
      pLdrDataTable = nullptr;

    if ( NT_SUCCESS (
           LdrFindEntryForAddress ( hModToTest, &pLdrDataTable )
       )            )                     return pLdrDataTable->ReferenceCount;
  }

  return 0;
}

int
GetModuleLoadCount (HMODULE hModToTest)
{
  PROCESS_BASIC_INFORMATION pbi = { 0 };

  NtQueryInformationProcess_pfn
  NtQueryInformationProcess =
 (NtQueryInformationProcess_pfn)SK_GetProcAddress ( L"NtDll",
 "NtQueryInformationProcess"                      );

  if (! NtQueryInformationProcess)
    return 0;

  if ( NT_SUCCESS (
      NtQueryInformationProcess ( GetCurrentProcess (),
                                    ProcessBasicInformation, &pbi,
                                                      sizeof (pbi), nullptr )
                   ) )
  {
    uint8_t *pLdrDataOffset =
   (uint8_t *)(pbi.PebBaseAddress) + offsetof (PEB, Ldr);

    uint8_t*     addr;
    PEB_LDR_DATA LdrData;

    if ((! ReadMem (pLdrDataOffset, &addr,    sizeof (void *))) ||
        (! ReadMem (addr,           &LdrData, sizeof (LdrData))) )
      return 0;

    LIST_ENTRY *pHead = LdrData.InMemoryOrderModuleList.Flink;
    LIST_ENTRY *pNext = pHead;

    do
    {
      LDR_DATA_TABLE_ENTRY   LdrEntry = { };
      LDR_DATA_TABLE_ENTRY *pLdrEntry =
        CONTAINING_RECORD ( pHead,
                             LDR_DATA_TABLE_ENTRY,
                               InMemoryOrderLinks );

      if (! ReadMem (pLdrEntry, &LdrEntry, sizeof (LdrEntry)))
        return 0;

      if (LdrEntry.DllBase == (LPVOID)hModToTest)
      {
        //
        //  http://www.geoffchappell.com/studies/windows/win32/ntdll/structs/ldr_data_table_entry.htm
        //
        int offDdagNode =
          (0x14 - SK_RunLHIfBitness (64, 1, 0)) * sizeof (LPVOID); // See offset on LDR_DDAG_NODE *DdagNode;

        ULONG    count        = 0;
        uint8_t* addrDdagNode = ((uint8_t *)pLdrEntry) + offDdagNode;

        //
        //  http://www.geoffchappell.com/studies/windows/win32/ntdll/structs/ldr_ddag_node.htm
        //  See offset on ULONG LoadCount;
        //
        if ( (! ReadMem (addrDdagNode, &addr,      sizeof (LPVOID))) ||
             (! ReadMem (               addr + 3 * sizeof (LPVOID),
                                       &count,     sizeof (count))) )
        {
          return 0;
        }

        return
          count;
      }

      pHead =
        LdrEntry.InMemoryOrderLinks.Flink;
    }
    while (pHead != pNext);
  }

  return 0;
}
#else
#include <winternl.h>                   //PROCESS_BASIC_INFORMATION

// warning C4996: 'GetVersionExW': was declared deprecated
#pragma warning (disable : 4996)
bool
IsWindows8OrGreater (void)
{
  OSVERSIONINFO
    ovi                     = { 0 };
    ovi.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);

  GetVersionEx (&ovi);

  if ( (ovi.dwMajorVersion == 6 &&
        ovi.dwMinorVersion >= 2) ||
         ovi.dwMajorVersion > 6
     ) return true;

  return false;
} //IsWindows8OrGreater
#pragma warning (default : 4996)



bool
ReadMem (void *addr, void *buf, int size)
{
  BOOL b = ReadProcessMemory( GetCurrentProcess(), addr, buf, size, nullptr );
    return b != FALSE;
}

#ifdef _WIN64
    #define BITNESS 1
#else
    #define BITNESS 0
#endif

using NtQueryInformationProcess_pfn = NTSTATUS (NTAPI *)(HANDLE,PROCESSINFOCLASS,PVOID,ULONG,PULONG);

int
GetModuleReferenceCount (HMODULE hDll)
{
  HMODULE hNtDll =
    LoadLibraryW (L"NtDll");
  if (!   hNtDll)
      return 0;

  LdrFindEntryForAddress_pfn
  LdrFindEntryForAddress =
 (LdrFindEntryForAddress_pfn)GetProcAddress (hNtDll,
 "LdrFindEntryForAddress");

  PLDR_DATA_TABLE_ENTRY__SK pldr_data_table = nullptr;

  bool b = false;
  if (LdrFindEntryForAddress != nullptr) b =
    NT_SUCCESS (
      LdrFindEntryForAddress ( hDll, &pldr_data_table )
  );
  FreeLibrary (hNtDll);

  return                          b ?
    pldr_data_table->ReferenceCount : 0;
}

//
//  Queries for .dll module load count, returns 0 if fails.
//
int
GetModuleLoadCount (HMODULE hDll)
{
  // Not supported by earlier versions of windows.
  if (! IsWindows8OrGreater ())
    return 0;

  PROCESS_BASIC_INFORMATION pbi = { 0 };

  HMODULE hNtDll =
    LoadLibraryW (L"NtDll");
  if (!   hNtDll)
      return 0;

  NtQueryInformationProcess_pfn
  NtQueryInformationProcess =
 (NtQueryInformationProcess_pfn)GetProcAddress (hNtDll,
 "NtQueryInformationProcess");

  bool b = false;
  if (NtQueryInformationProcess != nullptr) b = NT_SUCCESS (
      NtQueryInformationProcess ( GetCurrentProcess (),
                                    ProcessBasicInformation, &pbi,
                                                      sizeof (pbi), nullptr )
  );
  FreeLibrary (hNtDll);

  if (! b)
    return 0;

  char* LdrDataOffset =
    (char *)(pbi.PebBaseAddress) + offsetof (PEB, Ldr);

  char*        addr;
  PEB_LDR_DATA LdrData;

  if ((! ReadMem (LdrDataOffset, &addr,    sizeof (void *))) ||
      (! ReadMem (addr,          &LdrData, sizeof (LdrData))) )
    return 0;

  LIST_ENTRY* head = LdrData.InMemoryOrderModuleList.Flink;
  LIST_ENTRY* next = head;

  do {
    LDR_DATA_TABLE_ENTRY   LdrEntry;
    LDR_DATA_TABLE_ENTRY* pLdrEntry =
      CONTAINING_RECORD (head, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);

    if (! ReadMem (pLdrEntry , &LdrEntry, sizeof (LdrEntry)))
      return 0;

    if (LdrEntry.DllBase == (void *)hDll)
    {
      //
      //  http://www.geoffchappell.com/studies/windows/win32/ntdll/structs/ldr_data_table_entry.htm
      //
      int offDdagNode =
        (0x14 - BITNESS) * sizeof(void*);   // See offset on LDR_DDAG_NODE *DdagNode;

      ULONG count        = 0;
      char* addrDdagNode = ((char*)pLdrEntry) + offDdagNode;

      //
      //  http://www.geoffchappell.com/studies/windows/win32/ntdll/structs/ldr_ddag_node.htm
      //  See offset on ULONG LoadCount;
      //
      if ( (! ReadMem (addrDdagNode, &addr,     sizeof (void *))) ||
           (! ReadMem (              addr + 3 * sizeof (void *),
                                    &count,     sizeof (count))) )
          return 0;

      return
        (int)count;
    } //if

    head = LdrEntry.InMemoryOrderLinks.Flink;
  }while( head != next );

  return 0;
} //GetModuleLoadCount
#endif

         HANDLE   g_hPacifierThread = SK_INVALID_HANDLE;
         HANDLE   g_hHookTeardown   = SK_INVALID_HANDLE;
volatile HMODULE  g_hModule_CBT     = 0;
volatile LONG     g_sOnce           = 0;

HWND  hWndToSet   = (HWND)~0;
DWORD dwBandToSet = ZBID_DESKTOP;

LRESULT
CALLBACK
SK_WindowBand_AgentProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  static auto *pShared = (SK_SharedMemory_v1 *)
          SK_Inject_GetViewOfSharedMemory ();

  if (! pShared)
  {
    return
      DefWindowProcW (hwnd, message, wParam, lParam);
  }

  if (message      == pShared->SystemWide.uMsgExpRaise)
  {
    hWndToSet   =  (HWND)wParam;
    dwBandToSet = (DWORD)lParam;

    return 0;
  }

  else if (message == pShared->SystemWide.uMsgExpLower)
  {
    hWndToSet   =  (HWND)wParam;
    dwBandToSet = (DWORD)ZBID_DESKTOP;

    return 0;
  }

  return
    DefWindowProcW (hwnd, message, wParam, lParam);
};

static SetWindowBand_pfn
       SetWindowBand_Original = nullptr;

BOOL
WINAPI
SetWindowBand_Detour ( HWND  hWnd,
                       HWND  hWndInsertAfter,
                       DWORD dwBand )
{
  static volatile LONG lLock = 0;

  while (InterlockedCompareExchange (&lLock, 1, 0) != 0)
    SleepEx (1, FALSE);

  BOOL bRet =
    SetWindowBand_Original (
      hWnd, hWndInsertAfter, dwBand
    );

  HWND hWndReal = hWndToSet;

  if (hWndReal != (HWND)~0)
  {
    if (SetWindowBand_Original (hWndReal, HWND_TOP, dwBandToSet))
    {
      extern void WINAPI
        SK_keybd_event (
          _In_ BYTE       bVk,
          _In_ BYTE       bScan,
          _In_ DWORD     dwFlags,
          _In_ ULONG_PTR dwExtraInfo );

      SK_keybd_event (VK_LWIN,   (BYTE)MapVirtualKey (VK_LWIN,   0), KEYEVENTF_EXTENDEDKEY,                   0);
      SK_keybd_event (VK_LWIN,   (BYTE)MapVirtualKey (VK_LWIN,   0), KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
      SK_keybd_event (VK_ESCAPE, (BYTE)MapVirtualKey (VK_ESCAPE, 0), 0,                                       0);
      SK_keybd_event (VK_ESCAPE, (BYTE)MapVirtualKey (VK_ESCAPE, 0),                         KEYEVENTF_KEYUP, 0);

      SleepEx (150UL, FALSE);

      PostMessage              (hWndReal, 0xf00f,   dwBandToSet, 0);
                                hWndToSet = (HWND)~0;
    }
  }

  InterlockedExchange (&lLock, 0);

  return
    bRet;
}

void
SK_Inject_SpawnUnloadListener (void)
{
#define _AtomicClose(handle) if ((handle) != SK_INVALID_HANDLE) \
      CloseHandle (std::exchange((handle), SK_INVALID_HANDLE));

  if (! InterlockedCompareExchangePointer ((void **)&g_hModule_CBT, (void *)1, 0))
  {
    g_hPacifierThread = (HANDLE)
      CreateThread (nullptr, 0, [](LPVOID) ->
      DWORD
      {
        HANDLE hHookTeardown =
          OpenEvent ( EVENT_ALL_ACCESS, FALSE,
              SK_RunLHIfBitness (32, LR"(Local\SK_GlobalHookTeardown32)",
                                     LR"(Local\SK_GlobalHookTeardown64)") );

        if (! InterlockedCompareExchange (&g_sOnce, TRUE, FALSE))
        {
          LONG idx =
            InterlockedIncrement (&num_hooked_pids);

          if (idx < MAX_HOOKED_PROCS)
          {
            WriteULongRelease (
              &hooked_pids [idx],
                GetCurrentProcessId ()
            );
          }
        }

        // Try the Windows 10 API for Thread Names first, it's ideal unless ... not Win10 :)
        SetThreadDescription_pfn
            _SetThreadDescriptionWin10 =
            (SetThreadDescription_pfn)GetProcAddress (GetModuleHandle (L"Kernel32"),
            "SetThreadDescription");

        if (_SetThreadDescriptionWin10 != nullptr) {
            _SetThreadDescriptionWin10 (
              g_hPacifierThread,
                L"[SK] Global Hook Pacifier"
            );
        }

        SetThreadPriority (g_hPacifierThread, THREAD_PRIORITY_TIME_CRITICAL);

        HANDLE signals [] = {
              hHookTeardown,
          __SK_DLL_TeardownEvent
        };

        if (hHookTeardown != SK_INVALID_HANDLE)
        {
          WNDCLASSEXW
          wnd_class               = {                       };
          wnd_class.hInstance     = GetModuleHandle (nullptr);
          wnd_class.lpszClassName = L"SK_WindowBand_Agent";
          wnd_class.lpfnWndProc   = SK_WindowBand_AgentProc;
          wnd_class.cbSize        = sizeof (WNDCLASSEXW);

          InterlockedIncrement  (&injected_procs);

#if 0
          if (GetModuleHandle (L"explorer.exe"))
          {
            ATOM eve =
              RegisterClassExW (&wnd_class);

            if (eve == 0)
              UnregisterClassW (L"SK_WindowBand_Agent", GetModuleHandle (nullptr));

            if (eve != 0 || RegisterClassExW (&wnd_class))
            {
              SK_SharedMemory_v1* pShared =
                (SK_SharedMemory_v1*)SK_Inject_GetViewOfSharedMemory ();

              if (pShared != nullptr)
              {
                static HANDLE
                hExplorerDispatchThread =
                CreateThread (nullptr, 0, [](LPVOID pUser) ->
                DWORD
                {
                  auto hThreadSelf =
                    *(HANDLE *)pUser;

                  SK_SharedMemory_v1 *pShared =
                    (SK_SharedMemory_v1 *)SK_Inject_GetViewOfSharedMemory ();

                  if (! pShared)
                  {
                    CloseHandle (hThreadSelf);
                    return 0;
                  }

                  // Extra init needed for hooks
                  InterlockedCompareExchange (&__SK_DLL_Attached, 1, 0);

                  SK_MinHook_Init ();

                  SK_CreateDLLHook2 (       L"user32",
                                      "SetWindowBand",
                                       SetWindowBand_Detour,
              static_cast_p2p <void> (&SetWindowBand_Original) );

                  SK_ApplyQueuedHooks ();

                  pShared->SystemWide.hWndExplorer =
                    HandleToULong (
                      CreateWindowExW ( 0, L"SK_WindowBand_Agent", NULL, 0,
                                        0, 0, 0, 0, HWND_MESSAGE,  NULL, NULL, NULL )
                                  );

                  if (pShared->SystemWide.hWndExplorer != 0)
                  {
                    pShared->SystemWide.uMsgExpRaise = 0xf00d;//RegisterWindowMessageW (L"uMsgExpRaise");
                    pShared->SystemWide.uMsgExpLower = 0xf00f;//RegisterWindowMessageW (L"uMsgExpLower");
                  }

                  HWND hWndExplorer =
                    (HWND)ULongToHandle (pShared->SystemWide.hWndExplorer);

                  DWORD  dwWaitState  = WAIT_TIMEOUT;
                  while (dwWaitState != WAIT_OBJECT_0)
                  {
                    dwWaitState =
                      MsgWaitForMultipleObjects ( 1,   &__SK_DLL_TeardownEvent,
                                                  FALSE, INFINITE, QS_ALLINPUT );

                    if (dwWaitState == WAIT_OBJECT_0 + 1)
                    {
                      MSG                   msg = { };
                      while (PeekMessageW (&msg, hWndExplorer, 0, 0, PM_REMOVE | PM_NOYIELD) > 0)
                      {
                        TranslateMessage (&msg);
                        DispatchMessageW (&msg);
                      }
                    }
                  }

                  CloseHandle (hThreadSelf);

                  return 0;
                }, &hExplorerDispatchThread, CREATE_SUSPENDED, nullptr);

                if (hExplorerDispatchThread != 0)
                  ResumeThread (hExplorerDispatchThread);
              }
            }
            else wnd_class.cbSize = 0;
          }
#endif
          DWORD dwWaitState =
            MsgWaitForMultipleObjectsEx (
              2, signals, INFINITE, QS_ALLINPUT, 0x0
            );

          // Is Process Actively Using Special K (?)
          if ( dwWaitState      == WAIT_OBJECT_0 &&
               hModHookInstance != nullptr )
          {
            // Global Injection Shutting Down, SK is -ACTIVE- in this App.
            //
            //  ==> Must continue waiting for application exit ...
            //
            MsgWaitForMultipleObjectsEx (
              1, &__SK_DLL_TeardownEvent, INFINITE,
                             QS_ALLINPUT, 0x0 );
          }

#if 0
          if (GetModuleHandle (L"explorer.exe"))
          {
            SK_SharedMemory_v1 *pShared =
              (SK_SharedMemory_v1 *)SK_Inject_GetViewOfSharedMemory ();

            if (pShared != nullptr)
            {
              HWND hWndToKill =
                (HWND)ULongToHandle (pShared->SystemWide.hWndExplorer);

              if (IsWindow        (hWndToKill))
                if (DestroyWindow (hWndToKill))
                  pShared->SystemWide.hWndExplorer = (DWORD)~0;

              if ( std::exchange (wnd_class.cbSize, 0) )
                UnregisterClassW (wnd_class.lpszClassName, wnd_class.hInstance);

              SK_MinHook_UnInit ();
            }
          }
#endif

          // All clear, one less process to worry about
          InterlockedDecrement  (&injected_procs);
        }

        if (! SK_GetHostAppUtil ()->isInjectionTool ())
        {
          if (GetModuleLoadCount (SK_GetDLL ()) > 1)
                          FreeLibrary (SK_GetDLL ());
        }

        CloseHandle (g_hPacifierThread);
                     g_hPacifierThread = 0;

        _AtomicClose (hHookTeardown);

        InterlockedExchangePointer ((void **)&g_hModule_CBT, 0);

        if (GetModuleReferenceCount (SK_GetDLL ()) >= 1)
          FreeLibraryAndExitThread  (SK_GetDLL (), 0x0);

        return 0;
      }, nullptr, CREATE_SUSPENDED, nullptr
    );

    if (g_hPacifierThread != 0)
    {
      HMODULE hMod;

      GetModuleHandleEx ( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                            (LPCWSTR)SK_GetDLL (), &hMod );

      if (hMod)
      {
        InterlockedExchangePointer ((void **)&g_hModule_CBT, hMod);
        ResumeThread               (g_hPacifierThread);
      }

      else
      {
        TerminateThread            (g_hPacifierThread, 0x0);
        CloseHandle                (g_hPacifierThread);
      }
    }
  }
}

#define PreventAlwaysOnTop 0
#define        AlwaysOnTop 1
#define   SmartAlwaysOnTop 2

bool SK_Window_OnFocusChange (HWND hWndNewTarget, HWND hWndOld);
auto _DoWindowsOverlap =
[&](HWND hWndGame, HWND hWndApp, BOOL bSameMonitor = FALSE, INT iDeadzone = 50) -> bool
{
  if (! IsWindow (hWndGame)) return false;
  if (! IsWindow (hWndApp))  return false;

  if (bSameMonitor != FALSE)
  {
    if (MonitorFromWindow (hWndGame, MONITOR_DEFAULTTONEAREST) !=
        MonitorFromWindow (hWndApp,  MONITOR_DEFAULTTONEAREST))
    {
      return false;
    }
  }

  RECT rectGame = { },
       rectApp  = { };

  if (! GetWindowRect (hWndGame, &rectGame)) return false;
  if (! GetWindowRect (hWndApp,  &rectApp))  return false;

  InflateRect (&rectGame, -iDeadzone, -iDeadzone);
  InflateRect (&rectApp,  -iDeadzone, -iDeadzone);

  RECT                rectIntersect = { };
  IntersectRect     (&rectIntersect,
                             &rectGame, &rectApp);

  // Size of intersection is non-zero, we're done
  if (! IsRectEmpty (&rectIntersect)) return true;

  // Test for window entirely inside the other
  UnionRect (&rectIntersect, &rectGame, &rectApp);

  return
    EqualRect (&rectGame, &rectIntersect);
};

LRESULT
CALLBACK
CBTProc ( _In_ int    nCode,
          _In_ WPARAM wParam,
          _In_ LPARAM lParam )
{
  if (nCode < 0)
  {
    return
      CallNextHookEx (
        SKX_GetCBTHook (),
          nCode, wParam, lParam
      );
  }

  SK_Inject_SpawnUnloadListener ();

  extern bool SK_Window_OnFocusChange (HWND hWndNewTarget, HWND hWndOld);

  if (game_window.hWnd == 0)
  {
    if (hWndToSet != (HWND)~0) return
      CallNextHookEx (
        SKX_GetCBTHook (),
          nCode, wParam, lParam
      );

    if (nCode == HCBT_MOVESIZE)
    {
      HWND hWndGame =
        SK_Inject_GetFocusWindow ();

      if (hWndGame != 0x0 && IsWindow (hWndGame))
      {
        if (_DoWindowsOverlap (hWndGame, (HWND)wParam))
          PostMessage (hWndGame, /*WM_USER+WM_SETFOCUS*/0xfa57, (WPARAM)wParam, (LPARAM)0);

        else
        {
          if (MonitorFromWindow ((HWND)wParam,   MONITOR_DEFAULTTONEAREST) ==
              MonitorFromWindow ((HWND)hWndGame, MONITOR_DEFAULTTONEAREST))
          {
            PostMessage (hWndGame, /*WM_USER+WM_SETFOCUS*/0xfa57, (WPARAM)hWndGame, (LPARAM)wParam);
          }
        }
      }
    }
  }

  return
    CallNextHookEx (
      SKX_GetCBTHook (),
        nCode, wParam, lParam
    );
}

BOOL
SK_TerminatePID ( DWORD dwProcessId, UINT uExitCode )
{
  SK_AutoHandle hProcess (
    OpenProcess ( PROCESS_TERMINATE, FALSE, dwProcessId )
  );

  if (reinterpret_cast <intptr_t> (hProcess.m_h) <= 0)// == INVALID_HANDLE_VALUE)
    return FALSE;

  return
    SK_TerminateProcess ( hProcess.m_h, uExitCode );
}


void
__stdcall
SKX_InstallCBTHook (void)
{
  // Nothing to do here, move along.
  if (SKX_GetCBTHook () != nullptr)
    return;

  SK_Inject_InitShutdownEvent ();

  if (SK_GetHostAppUtil ()->isInjectionTool ())
  {
    void SK_Inject_InitWhiteAndBlacklists (void);
         SK_Inject_InitWhiteAndBlacklists ();
  }

  HMODULE                                   hModSelf;
  GetModuleHandleEx ( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                        (LPCWSTR)&CBTProc, &hModSelf );
  hHookCBT =
    SetWindowsHookEx (WH_CBT,     CBTProc,  hModSelf, 0);

  if (hHookCBT != 0)
    InterlockedExchange (&__SK_HookContextOwner, TRUE);
}


void
__stdcall
SKX_RemoveCBTHook (void)
{
  HMODULE                                   hModSelf;
  GetModuleHandleEx ( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                        (LPCWSTR)&CBTProc, &hModSelf );

  HHOOK hHookOrig =
    SKX_GetCBTHook ();

  HANDLE hHookTeardown =
    OpenEvent ( (DWORD)EVENT_ALL_ACCESS, FALSE,
      SK_RunLHIfBitness ( 32, LR"(Local\SK_GlobalHookTeardown32)",
                              LR"(Local\SK_GlobalHookTeardown64)") );

  if ((intptr_t)hHookTeardown > 0)
  {   SetEvent (hHookTeardown);  }

  if ( hHookOrig != nullptr &&
         UnhookWindowsHookEx (hHookOrig) )
  {
    if (SK_GetHostAppUtil ()->isInjectionTool ())
    {
      InterlockedDecrement (&injected_procs);
    }

                         whitelist_count = 0;
    RtlSecureZeroMemory (whitelist_patterns, sizeof (whitelist_patterns));
                         blacklist_count = 0;
    RtlSecureZeroMemory (blacklist_patterns, sizeof (blacklist_patterns));

              DWORD       self_pid = GetCurrentProcessId ();
    std::set <DWORD>   running_pids;
    std::set <DWORD> suspended_pids;
    LONG             hooked_pid_count =
      std::max   (0L,
        std::min (MAX_HOOKED_PROCS, ReadAcquire (&num_hooked_pids)));

    SK_Process_Snapshot ();

    int tries = 0;
    do
    {
      for ( int i = 0                ;
                i < hooked_pid_count ;
              ++i )
      {
        DWORD dwPid =
          ReadULongAcquire (&hooked_pids [i]);

        if (                   0 == dwPid  ||
              suspended_pids.count (dwPid) ||
                running_pids.count (dwPid) ||
                        self_pid == dwPid ) continue;

        if (SK_Process_IsSuspended (dwPid) &&
           (! suspended_pids.count (dwPid)))
        {
          if (SK_Process_Resume    (dwPid))
            suspended_pids.emplace (dwPid);
        }
        else running_pids.emplace  (dwPid);
      }

      DWORD_PTR dwpResult = 0x0;
      SendMessageTimeout ( HWND_BROADCAST,
                            WM_NULL, 0, 0,
                         SMTO_ABORTIFHUNG,
                                      8UL,
               &dwpResult );

      if (suspended_pids.empty () || ++tries > 4)
        break;
    }
    while (ReadAcquire (&injected_procs) > 0);

    hHookCBT = nullptr;
  }

  if (reinterpret_cast <intptr_t> (hHookTeardown) > 0)
    CloseHandle (                  hHookTeardown);

  dwHookPID = 0x0;
}

bool
__stdcall
SKX_IsHookingCBT (void) noexcept
{
  return
    SKX_GetCBTHook () != nullptr;
}


// Useful for managing injection of the 32-bit DLL from a 64-bit application
//   or visa versa.
void
CALLBACK
RunDLL_InjectionManager ( HWND   hwnd,        HINSTANCE hInst,
                          LPCSTR lpszCmdLine, int       nCmdShow )
{
  UNREFERENCED_PARAMETER (hInst);
  UNREFERENCED_PARAMETER (hwnd);

  SK_Inject_RenameProcess ();

  if (StrStrA (lpszCmdLine, "Install") && (! SKX_IsHookingCBT ()))
  {
    SKX_InstallCBTHook ();

    if (SKX_IsHookingCBT ())
    {
      const char* szPIDFile =
        SK_RunLHIfBitness ( 32, "SpecialK32.pid",
                                "SpecialK64.pid" );

      FILE* fPID =
        fopen (szPIDFile, "w");

      if (fPID != nullptr)
      {
        fprintf (fPID, "%lu\n", GetCurrentProcessId ());
        fclose  (fPID);

        HANDLE hThread =
          SK_Thread_CreateEx (
           [](LPVOID user) ->
             DWORD
               {
                 HANDLE hHookTeardown =
                   OpenEvent ( EVENT_ALL_ACCESS, FALSE,
                       SK_RunLHIfBitness (32, LR"(Local\SK_GlobalHookTeardown32)",
                                              LR"(Local\SK_GlobalHookTeardown64)") );

                 if (reinterpret_cast <intptr_t> (hHookTeardown) > 0)
                 {
                   ResetEvent (hHookTeardown);
                 }

                 if ( ReadAcquire       (&__SK_DLL_Attached) ||
                      SK_GetHostAppUtil ()->isInjectionTool () )
                 {
                   HANDLE events [] =
                      { __SK_DLL_TeardownEvent != 0 ?
                        __SK_DLL_TeardownEvent : hHookTeardown,
                                                 hHookTeardown };

                   WaitForMultipleObjects (2, events, FALSE, INFINITE);
                 }

                 if (reinterpret_cast <intptr_t> (hHookTeardown) > 0)
                 {
                   CloseHandle (hHookTeardown);
                                hHookTeardown = 0;
                 }

                 SK_Thread_CloseSelf ();

                 if (PtrToInt (user) != -128)
                 {
                   SK_ExitProcess (0x00);
                 }

                 return 0;
               }, nullptr,
             UIntToPtr (nCmdShow)
          );

        // Closes itself
        DBG_UNREFERENCED_LOCAL_VARIABLE (hThread);

        SK_Sleep (INFINITE);
      }
    }
  }

  else if (StrStrA (lpszCmdLine, "Remove"))
  {
    const char* szPIDFile =
      SK_RunLHIfBitness ( 32, "SpecialK32.pid",
                              "SpecialK64.pid" );

    FILE* fPID =
      fopen (szPIDFile, "r");

    if (fPID != nullptr)
    {
      DWORD dwPID =
           strtoul ("%lu", nullptr, 0);
      fclose (fPID);

      if ( GetCurrentProcessId () == dwPID ||
                    SK_TerminatePID (dwPID, 0x00) )
      {
        DeleteFileA (szPIDFile);
      }
    }

    SKX_RemoveCBTHook ();

    if (nCmdShow != -128)
      SK_ExitProcess (0x00);
  }
}


void
SK_Inject_EnableCentralizedConfig (void)
{
  wchar_t           wszOut [MAX_PATH + 2] = { };
  SK_PathCombineW ( wszOut,
                      SK_GetHostPath (),
                        L"SpecialK.central" );

  FILE* fOut =
    _wfopen (wszOut, L"w");

  if (fOut != nullptr)
  {
    fputws (L" ", fOut);
    fclose (      fOut);

    config.system.central_repository = true;
  }

  SK_EstablishRootPath ();

  if (! ReadAcquire (&__SK_DLL_Attached))
    return;

  switch (SK_GetCurrentRenderBackend ().api)
  {
    case SK_RenderAPI::D3D9:
    case SK_RenderAPI::D3D9Ex:
      SK_SaveConfig (L"d3d9");
      break;

#ifndef _M_AMD64
  case SK_RenderAPI::D3D8On11:
    SK_SaveConfig (L"d3d8");
    break;

  case SK_RenderAPI::DDrawOn11:
    SK_SaveConfig (L"ddraw");
    break;
#endif

    case SK_RenderAPI::D3D10:
    case SK_RenderAPI::D3D11:
#ifdef _M_AMD64
    case SK_RenderAPI::D3D12:
#endif
    {
      SK_SaveConfig (L"dxgi");
    } break;

    case SK_RenderAPI::OpenGL:
      SK_SaveConfig (L"OpenGL32");
      break;

    //case SK_RenderAPI::Vulkan:
      //lstrcatW (wszOut, L"\\vk-1.dll");
      //break;
  }
}


bool
SK_Inject_SwitchToRenderWrapperEx (DLL_ROLE role)
{
  wchar_t   wszIn  [MAX_PATH + 2] = { };
  lstrcatW (wszIn, SK_GetModuleFullName (SK_GetDLL ()).c_str ());

  wchar_t   wszOut [MAX_PATH + 2] = { };
  lstrcatW (wszOut, SK_GetHostPath ());

  switch (role)
  {
    case DLL_ROLE::D3D9:
      lstrcatW (wszOut, L"\\d3d9.dll");
      break;

    case DLL_ROLE::DXGI:
      lstrcatW (wszOut, L"\\dxgi.dll");
      break;

    case DLL_ROLE::DInput8:
      lstrcatW (wszOut, L"\\dinput8.dll");
      break;

    case DLL_ROLE::D3D11:
    case DLL_ROLE::D3D11_CASE:
      lstrcatW (wszOut, L"\\d3d11.dll");
      break;

    case DLL_ROLE::OpenGL:
      lstrcatW (wszOut, L"\\OpenGL32.dll");
      break;

#ifndef _M_AMD64
    case DLL_ROLE::DDraw:
      lstrcatW (wszOut, L"\\ddraw.dll");
      break;

    case DLL_ROLE::D3D8:
      lstrcatW (wszOut, L"\\d3d8.dll");
      break;
#endif

    //case SK_RenderAPI::Vulkan:
      //lstrcatW (wszOut, L"\\vk-1.dll");
      //break;
  }


  //std::queue <DWORD> suspended =
  //  SK_SuspendAllOtherThreads ();
  //
  //extern volatile LONG   SK_bypass_dialog_active;
  //InterlockedIncrement (&SK_bypass_dialog_active);
  //
  //int mb_ret =
  //       SK_MessageBox ( L"Link the Installed Wrapper to the Global DLL?\r\n"
  //                       L"\r\n"
  //                       L"Linked installs allow you to update wrapped games the same way "
  //                       L"as global injection, but require administrator privileges to setup.",
  //                         L"Perform a Linked Wrapper Install?",
  //                           MB_YESNO | MB_ICONQUESTION
  //                     );
  //
  //InterlockedIncrement (&SK_bypass_dialog_active);
  //
  //SK_ResumeThreads (suspended);

  //if ( mb_ret == IDYES )
  //{
  //  wchar_t   wszCmd [MAX_PATH * 3] = { };
  //  swprintf (wszCmd, L"/c mklink \"%s\" \"%s\"", wszOut, wszIn);
  //
  //  ShellExecuteW ( GetActiveWindow (),
  //                    L"runas",
  //                      L"cmd.exe",
  //                        wszCmd,
  //                          nullptr,
  //                            SW_HIDE );
  //
  //  SK_Inject_EnableCentralizedConfig ();
  //
  //  return true;
  //}


  if (CopyFile (wszIn, wszOut, TRUE) != FALSE)
  {
    SK_Inject_EnableCentralizedConfig ();

    *wszOut = L'\0';
    *wszIn  = L'\0';

    lstrcatW (wszOut, SK_GetHostPath ());

    SK_RunLHIfBitness (64, PathAppendW (wszIn,  L"SpecialK64.pdb"),
                           PathAppendW (wszIn,  L"SpecialK32.pdb") );
    SK_RunLHIfBitness (64, PathAppendW (wszOut, L"SpecialK64.pdb"),
                           PathAppendW (wszOut, L"SpecialK32.pdb") );

    if (CopyFileW (wszIn, wszOut, TRUE) == FALSE)
      ReplaceFileW (wszOut, wszIn, nullptr, 0x00, nullptr, nullptr);

    *wszIn = L'\0';

    std::wstring          ver_dir =
      SK_FormatStringW (LR"(%s\Version)", SK_GetConfigPath ());

    const DWORD dwAttribs =
      GetFileAttributesW (ver_dir.c_str ());

    lstrcatW             (wszIn, SK_GetModuleFullName (SK_GetDLL ()).c_str ());
    PathRemoveFileSpecW  (wszIn);
    PathAppendW          (wszIn, LR"(Version\installed.ini)");

    if ( GetFileAttributesW (wszIn) != INVALID_FILE_ATTRIBUTES &&
         ( (dwAttribs != INVALID_FILE_ATTRIBUTES) ||
           CreateDirectoryW (ver_dir.c_str (), nullptr) != FALSE ) )
    {
      *wszOut = L'\0';

      lstrcatW    (wszOut, ver_dir.c_str ());
      PathAppendW (wszOut, L"installed.ini");

      DeleteFileW (       wszOut);
      CopyFile    (wszIn, wszOut, FALSE);

      *wszIn = L'\0';

      lstrcatW             (wszIn, SK_GetModuleFullName (SK_GetDLL ()).c_str ());
      PathRemoveFileSpecW  (wszIn);
      PathAppendW          (wszIn, LR"(Version\repository.ini)");

      *wszOut = L'\0';

      lstrcatW    (wszOut, ver_dir.c_str ());
      PathAppendW (wszOut, L"repository.ini");

      DeleteFileW (       wszOut);
      CopyFile    (wszIn, wszOut, FALSE);
    }

    return true;
  }

  return false;
}

bool
SK_Inject_SwitchToRenderWrapper (void)
{
  wchar_t   wszIn  [MAX_PATH + 2] = { };
  wchar_t   wszOut [MAX_PATH + 2] = { };

  lstrcatW (wszIn,  SK_GetModuleFullName (SK_GetDLL ()).c_str ());
  lstrcatW (wszOut, SK_GetHostPath       (                     ));

  switch (SK_GetCurrentRenderBackend ().api)
  {
    case SK_RenderAPI::D3D9:
    case SK_RenderAPI::D3D9Ex:
      lstrcatW (wszOut, L"\\d3d9.dll");
      break;

#ifndef _M_AMD64
    case SK_RenderAPI::D3D8On11:
      lstrcatW (wszOut, L"\\d3d8.dll");
      break;

    case SK_RenderAPI::DDrawOn11:
      lstrcatW (wszOut, L"\\ddraw.dll");
      break;
#endif

    case SK_RenderAPI::D3D10:
    case SK_RenderAPI::D3D11:
#ifdef _M_AMD64
    case SK_RenderAPI::D3D12:
#endif
    {
      lstrcatW (wszOut, L"\\dxgi.dll");
    } break;

    case SK_RenderAPI::OpenGL:
      lstrcatW (wszOut, L"\\OpenGL32.dll");
      break;

    //case SK_RenderAPI::Vulkan:
      //lstrcatW (wszOut, L"\\vk-1.dll");
      //break;
  }


  //std::queue <DWORD> suspended =
  //  SK_SuspendAllOtherThreads ();
  //
  //extern volatile LONG   SK_bypass_dialog_active;
  //InterlockedIncrement (&SK_bypass_dialog_active);
  //
  //int mb_ret =
  //       SK_MessageBox ( L"Link the Installed Wrapper to the Global DLL?\r\n"
  //                       L"\r\n"
  //                       L"Linked installs allow you to update wrapped games the same way "
  //                       L"as global injection, but require administrator privileges to setup.",
  //                         L"Perform a Linked Wrapper Install?",
  //                           MB_YESNO | MB_ICONQUESTION
  //                     );
  //
  //InterlockedIncrement (&SK_bypass_dialog_active);
  //
  //SK_ResumeThreads (suspended);
  //
  //if ( mb_ret == IDYES )
  //{
  //  wchar_t   wszCmd [MAX_PATH * 3] = { };
  //  swprintf (wszCmd, L"/c mklink \"%s\" \"%s\"", wszOut, wszIn);
  //
  //  ShellExecuteW ( GetActiveWindow (),
  //                    L"runas",
  //                      L"cmd.exe",
  //                        wszCmd,
  //                          nullptr,
  //                            SW_HIDE );
  //
  //  SK_Inject_EnableCentralizedConfig ();
  //
  //  return true;
  //}


  if (CopyFile (wszIn, wszOut, TRUE))
  {
    SK_Inject_EnableCentralizedConfig ();

    *wszOut = L'\0';
    *wszIn  = L'\0';

    lstrcatW (wszOut, SK_GetHostPath ());

    const wchar_t* wszPDBFile =
      SK_RunLHIfBitness (64, L"SpecialK64.pdb",
                             L"SpecialK32.pdb");
    lstrcatW (wszIn,  wszPDBFile);
    lstrcatW (wszOut, L"\\");
    lstrcatW (wszOut, wszPDBFile);

    if (! CopyFileW (wszIn, wszOut, TRUE))
       ReplaceFileW (wszOut, wszIn, nullptr, 0x00, nullptr, nullptr);

    *wszIn = L'\0';

    std::wstring        ver_dir =
      SK_FormatStringW (LR"(%s\Version)", SK_GetConfigPath ());

    const DWORD dwAttribs =
      GetFileAttributesW (ver_dir.c_str ());

    lstrcatW             (wszIn, SK_GetModuleFullName (SK_GetDLL ()).c_str ());
    PathRemoveFileSpecW  (wszIn);
    PathAppendW          (wszIn, LR"(Version\installed.ini)");

    if ( GetFileAttributesW (wszIn) != INVALID_FILE_ATTRIBUTES &&
         ( (dwAttribs != INVALID_FILE_ATTRIBUTES) ||
           CreateDirectoryW (ver_dir.c_str (), nullptr) != FALSE ) )
    {
      *wszOut = L'\0';

      lstrcatW    (wszOut, ver_dir.c_str ());
      PathAppendW (wszOut, L"installed.ini");

      DeleteFileW (       wszOut);
      CopyFile    (wszIn, wszOut, FALSE);

      *wszIn = L'\0';

      lstrcatW             (wszIn, SK_GetModuleFullName (SK_GetDLL ()).c_str ());
      PathRemoveFileSpecW  (wszIn);
      PathAppendW          (wszIn, LR"(Version\repository.ini)");

      *wszOut = L'\0';

      lstrcatW    (wszOut, ver_dir.c_str ());
      PathAppendW (wszOut, L"repository.ini");

      DeleteFileW (       wszOut);
      CopyFile    (wszIn, wszOut, FALSE);
    }

    return true;
  }

  return false;
}

bool
SK_Inject_SwitchToGlobalInjector (void)
{
  config.system.central_repository = true;
  SK_EstablishRootPath ();

  wchar_t wszOut  [MAX_PATH + 2] = { };
  wchar_t wszTemp [MAX_PATH + 2] = { };

  lstrcatW         (wszOut, SK_GetModuleFullName (SK_GetDLL ()).c_str ());
  GetTempFileNameW (SK_GetHostPath (), L"SKI", SK_timeGetTime (), wszTemp);
  MoveFileW        (wszOut,                                       wszTemp);

  SK_SaveConfig (L"SpecialK");

  return true;
}


#if 0
bool SK_Inject_JournalRecord (HMODULE hModule)
{
  return false;
}
#endif



bool
SK_ExitRemoteProcess (const wchar_t* wszProcName, UINT uExitCode = 0x0)
{
  UNREFERENCED_PARAMETER (uExitCode);

  PROCESSENTRY32W pe32      = { };
  SK_AutoHandle   hProcSnap   (
    CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0) );

  if ((intptr_t)hProcSnap.m_h <= 0)
    return false;

  pe32.dwSize =
    sizeof (PROCESSENTRY32W);

  if (Process32FirstW (hProcSnap.m_h, &pe32) == FALSE)
  {
    return false;
  }

  do
  {
    if (StrStrIW (wszProcName, pe32.szExeFile) != nullptr)
    {
      window_t win =
        SK_FindRootWindow (pe32.th32ProcessID);

      if (win.root != 0)
        SendMessage (win.root, WM_USER + 0x123, 0x00, 0x00);

      return true;
    }
  } while (Process32NextW (hProcSnap.m_h, &pe32));

  return false;
}


void
SK_Inject_Stop (void)
{
  wchar_t wszCurrentDir [MAX_PATH + 2] = { };
  wchar_t wszWOW64      [MAX_PATH + 2] = { };
  wchar_t wszSys32      [MAX_PATH + 2] = { };

  GetCurrentDirectoryW  (MAX_PATH, wszCurrentDir);
  SetCurrentDirectoryW  (SK_SYS_GetInstallPath ().c_str ());
  GetSystemDirectoryW   (wszSys32, MAX_PATH);

  SK_RunLHIfBitness ( 32, GetSystemDirectoryW      (wszWOW64, MAX_PATH),
                          GetSystemWow64DirectoryW (wszWOW64, MAX_PATH) );

  //SK_ExitRemoteProcess (L"SKIM64.exe", 0x00);

  //if (GetFileAttributes (L"SKIM64.exe") == INVALID_FILE_ATTRIBUTES)
  if (true)
  {
    PathAppendW      ( wszWOW64, L"rundll32.exe");
    SK_ShellExecuteA ( nullptr,
                         "open", SK_WideCharToUTF8 (wszWOW64).c_str (),
                         "SpecialK32.dll,RunDLL_InjectionManager Remove", nullptr, SW_HIDE );

    PathAppendW      ( wszSys32, L"rundll32.exe");
    SK_ShellExecuteA ( nullptr,
                         "open", SK_WideCharToUTF8 (wszSys32).c_str (),
                         "SpecialK64.dll,RunDLL_InjectionManager Remove", nullptr, SW_HIDE );
  }

  else
  {
    HWND hWndExisting =
      FindWindow (L"SKIM_Frontend", nullptr);

    // Best-case, SKIM restarts itself
    if (hWndExisting)
    {
      SendMessage (hWndExisting, WM_USER + 0x122, 0, 0);
      SK_Sleep    (333UL);
    }

    // Worst-case, we do this manually and confuse Steam
    else
    {
      SK_ShellExecuteA     ( nullptr,
                               "open", "SKIM64.exe",
                               "-Inject", SK_WideCharToUTF8 (SK_SYS_GetInstallPath ()).c_str (),
                                 SW_FORCEMINIMIZE );
      SK_Sleep             ( 333UL );
      SK_ExitRemoteProcess ( L"SKIM64.exe", 0x00);
    }

    PathAppendW      ( wszWOW64, L"rundll32.exe");
    SK_ShellExecuteA ( nullptr,
                         "open", SK_WideCharToUTF8 (wszWOW64).c_str (),
                         "SpecialK32.dll,RunDLL_InjectionManager Remove", nullptr, SW_HIDE );

    PathAppendW      ( wszSys32, L"rundll32.exe");
    SK_ShellExecuteA ( nullptr,
                         "open", SK_WideCharToUTF8 (wszSys32).c_str (),
                         "SpecialK64.dll,RunDLL_InjectionManager Remove", nullptr, SW_HIDE );
  }

  SetCurrentDirectoryW (wszCurrentDir);
}

void
SK_Inject_Start (void)
{
  wchar_t wszCurrentDir [MAX_PATH + 2] = { };
  wchar_t wszWOW64      [MAX_PATH + 2] = { };
  wchar_t wszSys32      [MAX_PATH + 2] = { };

  GetCurrentDirectoryW  (MAX_PATH, wszCurrentDir);
  SetCurrentDirectoryW  (SK_SYS_GetInstallPath ().c_str ());
  GetSystemDirectoryW   (wszSys32, MAX_PATH);

  SK_RunLHIfBitness ( 32, GetSystemDirectoryW      (wszWOW64, MAX_PATH),
                          GetSystemWow64DirectoryW (wszWOW64, MAX_PATH) );

  if (GetFileAttributes (L"SKIM64.exe") == INVALID_FILE_ATTRIBUTES)
  {
    if (SKX_IsHookingCBT ())
    {
      RunDLL_InjectionManager ( nullptr, nullptr,
                                "Remove", -128 );
    }

    PathAppendW      ( wszSys32, L"rundll32.exe");
    SK_ShellExecuteA ( nullptr,
                         "open", SK_WideCharToUTF8 (wszSys32).c_str (),
                         "SpecialK64.dll,RunDLL_InjectionManager Install", nullptr, SW_HIDE );

    PathAppendW      ( wszWOW64, L"rundll32.exe");
    SK_ShellExecuteA ( nullptr,
                         "open", SK_WideCharToUTF8 (wszWOW64).c_str (),
                         "SpecialK32.dll,RunDLL_InjectionManager Install", nullptr, SW_HIDE );
  }

  else
  {
    if (SKX_IsHookingCBT ())
    {
      RunDLL_InjectionManager ( nullptr, nullptr,
                                "Remove", -128 );
    }

    HWND hWndExisting =
      FindWindow (L"SKIM_Frontend", nullptr);

    // Best-case, SKIM restarts itself
    if (hWndExisting)
    {
      SendMessage (hWndExisting, WM_USER + 0x125, 0, 0);
      SK_Sleep    (250UL);
      SendMessage (hWndExisting, WM_USER + 0x124, 0, 0);
    }

    // Worst-case, we do this manually and confuse Steam
    else
    {
      SK_ShellExecuteA ( nullptr,
                           "open", "SKIM64.exe", "+Inject",
                           SK_WideCharToUTF8 (SK_SYS_GetInstallPath ()).c_str (),
                             SW_FORCEMINIMIZE );
    }
  }

  SetCurrentDirectoryW (wszCurrentDir);
}


size_t
__stdcall
SKX_GetInjectedPIDs ( DWORD* pdwList,
                      size_t capacity )
{
  DWORD*  pdwListIter = pdwList;
  SSIZE_T i           = 0;

  SK_Inject_ValidateProcesses ();

  for (volatile LONG& hooked_pid : g_sHookedPIDs)
  {
    if (ReadAcquire (&hooked_pid) != 0)
    {
      if (i < (SSIZE_T)capacity - 1)
      {
        if (pdwListIter != nullptr)
        {
          *pdwListIter = ReadAcquire (&hooked_pid);
           pdwListIter++;
        }
      }

      ++i;
    }
  }

  if (pdwListIter != nullptr)
     *pdwListIter  = 0;

  return i;
}

BOOL
SK_IsWindowsVersionOrGreater (DWORD dwMajorVersion, DWORD dwMinorVersion, DWORD dwBuildNumber)
{
  NTSTATUS(WINAPI *RtlGetVersion)(LPOSVERSIONINFOEXW);

  OSVERSIONINFOEXW
    osInfo                     = { };
    osInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFOEXW);

  *reinterpret_cast<FARPROC *>(&RtlGetVersion) =
    SK_GetProcAddress (L"ntdll", "RtlGetVersion");

  if (RtlGetVersion != nullptr)
  {
    if (NT_SUCCESS (RtlGetVersion (&osInfo)))
    {
      return
        ( osInfo.dwMajorVersion   >  dwMajorVersion ||
          ( osInfo.dwMajorVersion == dwMajorVersion &&
            osInfo.dwMinorVersion >= dwMinorVersion &&
            osInfo.dwBuildNumber  >= dwBuildNumber  )
        );
    }
  }

  return FALSE;
}

void
SK_Inject_ParseWhiteAndBlacklists (const std::wstring& base_path)
{
  struct list_builder_s {
    struct dll_shared_seg_s {
      int&       count;
      wchar_t*   patterns;
    } dll_global;
    std::wstring file_name;
  }

  lists [] =
  { { whitelist_count, whitelist_patterns,
         base_path + L"whitelist.ini" },
    { blacklist_count, blacklist_patterns,
         base_path + L"blacklist.ini" } };

  for ( auto& list : lists )
  {
    list.dll_global.count = -1;

    std::wifstream list_file (
      list.file_name
    );

    if (list_file.is_open ())
    {
      // Requires Windows 10 1903+ (Build 18362)
      if (SK_IsWindowsVersionOrGreater (10, 0, 18362))
      {
        list_file.imbue (
            std::locale (".UTF-8")
        );
      }

      else
      {
        // Win8.1 fallback relies on deprecated stuff, so surpress warning when compiling
#pragma warning(suppress : 4996)
        list_file.imbue (
            std::locale (std::locale::empty (),
                         new (std::nothrow) std::codecvt_utf8 <wchar_t, 0x10ffff> ())
        );
      }

      // Don't update the shared DLL data segment until we finish parsing
      int          count = 0;
      std::wstring line;

      while (list_file.good ())
      {
        std::getline (list_file, line);

        // Skip blank lines, since they would match everything....
        for (const auto& it : line)
        {
          if (iswalpha (it) != 0)
          {
            wcsncpy_s ( (wchar_t *)&list.dll_global.patterns [MAX_PATH * count++],
                                                              MAX_PATH,
                        line.c_str (),                       _TRUNCATE );
            break;
          }
        }
      }

      // Now update the shared data seg.
      list.dll_global.count = count;
    }
  }
};

void
SK_Inject_InitWhiteAndBlacklists (void)
{
  SK_Thread_CreateEx ([](LPVOID)->DWORD
  {
    SK_AutoCOMInit _require_COM;

    static
      std::wstring global_cfg_dir =
        std::wstring (
          SK_GetDocumentsDir () + LR"(\My Mods\SpecialK\Global\)"
        );

    SK_Inject_ParseWhiteAndBlacklists (global_cfg_dir);

    HANDLE hChangeNotification =
      FindFirstChangeNotificationW (
        global_cfg_dir.c_str (), FALSE,
          FILE_NOTIFY_CHANGE_FILE_NAME  |
          FILE_NOTIFY_CHANGE_SIZE       |
          FILE_NOTIFY_CHANGE_LAST_WRITE
      );

    if (hChangeNotification != INVALID_HANDLE_VALUE)
    {
      while ( FindNextChangeNotification (
                     hChangeNotification ) != FALSE )
      {
        if ( WAIT_OBJECT_0 ==
               WaitForSingleObject (hChangeNotification, INFINITE) )
        {
          SK_Inject_ParseWhiteAndBlacklists (global_cfg_dir);
        }
      }
    }

    SK_Thread_CloseSelf ();

    return 0;
  }, L"[SK_INJECT] White/Blacklist Sentinel");
}

bool
SK_Inject_TestUserWhitelist (const wchar_t* wszExecutable)
{
  if ( whitelist_count <= 0 )
    return false;

  for ( int i = 0; i < whitelist_count; i++ )
  {
    std::wregex regexp (
      &whitelist_patterns [MAX_PATH * i],
        std::regex_constants::icase
    );

    if (std::regex_search (wszExecutable, regexp))
    {
      return true;
    }
  }

  return false;
}

bool
SK_Inject_TestWhitelists (const wchar_t* wszExecutable)
{
  // Sort of a temporary hack for important games that I support that are
  //   sold on alternative stores to Steam.
  if (StrStrNIW (wszExecutable, L"ffxv", MAX_PATH) != nullptr)
    return true;

  return
    SK_Inject_TestUserWhitelist (wszExecutable);
}

bool
SK_Inject_TestUserBlacklist (const wchar_t* wszExecutable)
{
  if ( blacklist_count <= 0 )
    return false;

  for ( int i = 0; i < blacklist_count; i++ )
  {
    std::wregex regexp (
      &blacklist_patterns [MAX_PATH * i],
        std::regex_constants::icase
    );

    if (std::regex_search (wszExecutable, regexp))
    {
      return true;
    }
  }

  return false;
}

bool
SK_Inject_TestBlacklists (const wchar_t* wszExecutable)
{
  wchar_t    wszExecutableCopy [MAX_PATH] = { };
  wcsncpy_s (wszExecutableCopy, MAX_PATH,
             wszExecutable,     _TRUNCATE );

  PathStripPath (wszExecutableCopy);
  if (StrStrNIW (wszExecutableCopy, L"launcher", MAX_PATH) != nullptr)
    return true;

  return
    SK_Inject_TestUserBlacklist (wszExecutable);
}

bool
SK_Inject_IsAdminSupported (void) noexcept
{
  return
    ( bAdmin != FALSE );
}


void SK_Inject_BroadcastAttachNotify (void)
{
  CHandle hInjectAck (
    OpenEvent ( EVENT_ALL_ACCESS, FALSE, LR"(Local\SKIF_InjectAck)" )
  );

  if (hInjectAck.m_h > 0)
  {
    SetEvent (hInjectAck.m_h);
  }
}