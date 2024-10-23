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

#include <SpecialK/render/present_mon/TraceSession.hpp>

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"GlobalHook"

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
SK_InjectionRecord_s __SK_InjectionHistory [MAX_INJECTED_PROC_HISTORY] = { };

#pragma data_seg (".SK_Hooks")
extern "C"
{
  DWORD        dwHookPID  = 0x0;     // Process that owns the CBT hook
  HHOOK        hHookCBT   = nullptr; // CBT hook
  HHOOK        hHookShell = nullptr; // Shell hook
  HHOOK        hHookDebug = nullptr; // Debug hook
  BOOL         bAdmin     = FALSE;   // Is SKIM64 able to inject into admin apps?

  LONG         g_sHookedPIDs [MAX_INJECTED_PROCS]                                      =  { };

  wchar_t      __SK_InjectionHistory_name       [MAX_INJECTED_PROC_HISTORY * MAX_PATH] =  { };
  wchar_t      __SK_InjectionHistory_win_title  [MAX_INJECTED_PROC_HISTORY * 128     ] =  { };
  DWORD        __SK_InjectionHistory_ids        [MAX_INJECTED_PROC_HISTORY]            =  { };
  __time64_t   __SK_InjectionHistory_inject     [MAX_INJECTED_PROC_HISTORY]            =  { };
  __time64_t   __SK_InjectionHistory_eject      [MAX_INJECTED_PROC_HISTORY]            =  { };
  bool         __SK_InjectionHistory_crash      [MAX_INJECTED_PROC_HISTORY]            =  { };
                                                                                            
  ULONG64      __SK_InjectionHistory_frames     [MAX_INJECTED_PROC_HISTORY]            =  { };
  SK_RenderAPI __SK_InjectionHistory_api        [MAX_INJECTED_PROC_HISTORY]            =  { };
  AppId64_t    __SK_InjectionHistory_AppId      [MAX_INJECTED_PROC_HISTORY]            =  { };
  wchar_t      __SK_InjectionHistory_UwpPackage [MAX_INJECTED_PROC_HISTORY *
                                                         PACKAGE_FULL_NAME_MAX_LENGTH] =  { };

  __declspec (dllexport) volatile LONG SK_InjectionRecord_s::count                     =   0L;
  __declspec (dllexport) volatile LONG SK_InjectionRecord_s::rollovers                 =   0L;

  __declspec (dllexport)          wchar_t whitelist_patterns [128 * MAX_PATH] = { };
  __declspec (dllexport)          int     whitelist_count                     =  0;
  __declspec (dllexport)          wchar_t blacklist_patterns [128 * MAX_PATH] = { };
  __declspec (dllexport)          int     blacklist_count                     =  0;
  __declspec (dllexport) volatile LONG    injected_procs                      =  0;

  constexpr LONG MAX_HOOKED_PROCS = 262144;

  // Recordkeeping on processes with CBT hooks; required to release the DLL
  //   in any process that has become suspended since hook install.
  volatile LONG  num_hooked_pids                =  0;
  volatile DWORD hooked_pids [MAX_HOOKED_PROCS] = { };

  DWORD dwPidShell     = 0x0;
  HWND  hWndTaskSwitch = 0;
};
#pragma data_seg ()
#pragma comment  (linker, "/SECTION:.SK_Hooks,RWS")

// Change in design, we will use mapped memory instead of DLL shared memory in
// versions 21.6.x+ in order to communicate between 32-bit and 64-bit apps.
volatile HANDLE hMapShared                   = INVALID_HANDLE_VALUE;
volatile HANDLE hMapRecords                  = INVALID_HANDLE_VALUE;
         HANDLE hWhitelistChangeNotification = INVALID_HANDLE_VALUE;

SK_InjectionRecord_s* __stdcall  SK_Inject_GetRecord32      (DWORD dwPid);
SK_InjectionRecord_s* __stdcall  SK_Inject_GetRecord32ByIdx (int   idx);

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

#ifndef _M_AMD64
        auto   *pRecords = (SK_InjectionRecord_s *)SK_Inject_GetViewOf32BitRecords ();
        memcpy (pRecords, __SK_InjectionHistory, sizeof (__SK_InjectionHistory));
#endif

        return S_OK;
      }
    }
  }

  return E_NOINTERFACE;
}


static volatile LPVOID _pInjection32View  = nullptr;

HANDLE
SK_Inject_Get32BitRecordMemory (void)
{
  HANDLE hLocalMap =
    ReadPointerAcquire (&hMapRecords);

  if (hLocalMap != INVALID_HANDLE_VALUE)
    return hLocalMap;

  HANDLE hExistingFile =
    OpenFileMapping (FILE_MAP_ALL_ACCESS, FALSE, LR"(Local\SK_32BitInjectionRecords_v1)");

  if (hExistingFile != nullptr)
  {
    if ( INVALID_HANDLE_VALUE ==
           InterlockedCompareExchangePointer (&hMapRecords, hExistingFile, INVALID_HANDLE_VALUE) )
    {
      WritePointerRelease (&hMapRecords, hExistingFile);

      return hExistingFile;
    }

    SK_CloseHandle (hExistingFile);
  }

  HANDLE hNewFile  =
    CreateFileMapping ( INVALID_HANDLE_VALUE, nullptr,
                           PAGE_READWRITE, 0, sizeof (__SK_InjectionHistory),
                             LR"(Local\SK_32BitInjectionRecords_v1)" );

  if (hNewFile != 0)
  {
    if ( INVALID_HANDLE_VALUE ==
           InterlockedCompareExchangePointer (&hMapRecords, hNewFile, INVALID_HANDLE_VALUE) )
    {
      return hNewFile;
    }

    SK_CloseHandle (hNewFile);
  }

  return SK_INVALID_HANDLE;
}

LPVOID
SK_Inject_GetViewOf32BitRecords (void)
{
  LPVOID lpLocalView =
    ReadPointerAcquire (&_pInjection32View);

  if (lpLocalView != nullptr)
    return lpLocalView;

  // Do not close this
  HANDLE hMap =
    SK_Inject_Get32BitRecordMemory ();

  if (hMap != nullptr)
  {
    lpLocalView =
      MapViewOfFile (hMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof (__SK_InjectionHistory));

    if (lpLocalView != nullptr)
    {
      if ( nullptr ==
             InterlockedCompareExchangePointer (&_pInjection32View, lpLocalView, nullptr) )
      {
        return lpLocalView;
      }

      UnmapViewOfFile (lpLocalView);
    }

    return
      ReadPointerAcquire (&_pInjection32View);
  }

  return nullptr;
}

void
SK_Inject_Cleanup32BitRecords (void)
{
  WritePointerRelease (&_pInjection32View, nullptr);

  SK_AutoHandle hLocalMap (
    InterlockedCompareExchangePointer (&hMapRecords, INVALID_HANDLE_VALUE,
                   ReadPointerAcquire (&hMapRecords))
  );

  if (hLocalMap != INVALID_HANDLE_VALUE)
  {
    UnmapViewOfFile (
      SK_Inject_GetViewOf32BitRecords ()
    );
  }
}

SK_InjectionRecord_s*
__stdcall
SK_Inject_GetRecord32 (DWORD dwPid)
{
  if (auto pHistory32 = (SK_InjectionRecord_s *)SK_Inject_GetViewOf32BitRecords ();
           pHistory32 != nullptr)
  {
    for ( int idx = 0                         ;
              idx < MAX_INJECTED_PROC_HISTORY ;
            ++idx )
    {
      if ( pHistory32 [idx].process.id == dwPid )
      {
        return
          &pHistory32 [idx];
      }
    }
  }

  return nullptr;
}

SK_InjectionRecord_s*
__stdcall
SK_Inject_GetRecord32ByIdx (int idx)
{
  if (auto pHistory32 = (SK_InjectionRecord_s *)SK_Inject_GetViewOf32BitRecords ();
           pHistory32 != nullptr)
  {
    return
      SK_Inject_GetRecord32 (
        pHistory32 [idx].process.id
      );
  }

  return nullptr;
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
  for (const volatile LONG& hooked_pid : g_sHookedPIDs)
  {
    SK_AutoHandle hProc (
      OpenProcess ( PROCESS_QUERY_INFORMATION, FALSE,
                      ReadAcquire (&hooked_pid) )
                  );

    if (! hProc.isValid ())
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
  SK_LOG0 ( ( L"Injection Release (%u, %lu, %s)", (unsigned int)SK_GetCurrentRenderBackend ().api,
                                                                SK_GetFramesDrawn          (),
                                                                SK_Debug_IsCrashing        () ? L"Crash" : L"Normal" ),
              L"Injection" );
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
      
      // GetCurrentPackageFullName (Windows 8+)
      using GetCurrentPackageFullName_pfn =
        LONG (WINAPI *)(IN OUT UINT32*, OUT OPTIONAL PWSTR);

      static GetCurrentPackageFullName_pfn
        SK_GetCurrentPackageFullName =
            (GetCurrentPackageFullName_pfn)GetProcAddress (LoadLibraryEx (L"kernel32.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32),
            "GetCurrentPackageFullName");

      wchar_t                                wszPackageName [PACKAGE_FULL_NAME_MAX_LENGTH] = { };
      UINT32                             uiLen                                             =  0 ;
      if (SK_GetCurrentPackageFullName != nullptr &&
          SK_GetCurrentPackageFullName (&uiLen, wszPackageName) != APPMODEL_ERROR_NO_PACKAGE)
      {
        wcsncpy_s (&__SK_InjectionHistory_UwpPackage [
                                              local_record * PACKAGE_FULL_NAME_MAX_LENGTH],
                                         uiLen, wszPackageName,                 _TRUNCATE);
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
  for (const
       volatile LONG& hooked_pid : g_sHookedPIDs)
  {
    if (ReadAcquire (&hooked_pid) == sk::narrow_cast <LONG> (dwThreadId))
      return true;
  }

  return false;
}

HHOOK
SKX_GetCBTHook (void) noexcept
{
  return hHookCBT;
}

bool
SK_Inject_IsHookActive (void)
{
  return
    ( SK_IsInjected () && dwHookPID != 0x0 );
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

  if (hExistingFile != nullptr)
  {
    if ( INVALID_HANDLE_VALUE ==
           InterlockedCompareExchangePointer (&hMapShared, hExistingFile, INVALID_HANDLE_VALUE) )
    {
      WritePointerRelease (&hMapShared, hExistingFile);

      return hExistingFile;
    }

    SK_CloseHandle (hExistingFile);
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

    SK_CloseHandle (hNewFile);
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

  // Do not close this
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

  SK_AutoHandle hLocalMap (
    InterlockedCompareExchangePointer (&hMapShared, INVALID_HANDLE_VALUE,
                   ReadPointerAcquire (&hMapShared))
  );

  if (hLocalMap != INVALID_HANDLE_VALUE)
  {
    UnmapViewOfFile (
      SK_Inject_GetViewOfSharedMemory ()
    );
  }
}

bool SK_Window_OnFocusChange (HWND hWndNewTarget, HWND hWndOld);

HWINEVENTHOOK g_WinEventHook;

void
CALLBACK
SK_Inject_WinEventHookProc (
  HWINEVENTHOOK hook,
  DWORD         event,
  HWND          hwnd,
  LONG          idObject,
  LONG          idChild,
  DWORD         dwEventThread,
  DWORD         dwmsEventTime )
{
  std::ignore = hook,
  std::ignore = idObject,
  std::ignore = idChild,
  std::ignore = dwEventThread,
  std::ignore = dwmsEventTime;

  if (event == EVENT_SYSTEM_FOREGROUND)
  {
    bool is_smart_always_on_top =
      config.window.always_on_top == SmartAlwaysOnTop;

    if (config.window.always_on_top == NoPreferenceOnTop && SK_GetCurrentRenderBackend ().isFakeFullscreen ())
      is_smart_always_on_top = true;

    // Processing this will cause Baldur's Gate 3 to react funny;
    //   consider it incompatible with "Smart Always on Top"
    if (                 is_smart_always_on_top &&
                    game_window.hWnd != hwnd    &&
                    game_window.hWnd != nullptr &&
          IsWindow (game_window.hWnd) )
    {
      if (SK_Window_TestOverlap (game_window.hWnd, hwnd, FALSE, 25))
      {
        if (dwPidShell == 0 &&      hWndTaskSwitch == 0)
        {                           hWndTaskSwitch =
                FindWindow (nullptr, L"Task Switching");
          GetWindowThreadProcessId (hWndTaskSwitch, &dwPidShell);
        }

        DWORD                            dwPidNewTarget;
        GetWindowThreadProcessId (hwnd, &dwPidNewTarget);

        // Do not remove Smart Always On Top for the task switcher,
        //   it runs in a window band above this application...
        if (dwPidNewTarget == dwPidShell)
        {
          return;
        }

        if (SK_Window_IsTopMost (game_window.hWnd))
        {
          SK_Window_SetTopMost (
            false, false, game_window.hWnd
          );

          BringWindowToTop (hwnd);
        }
        SK_Window_OnFocusChange (hwnd, (HWND)0);
      }
    }
  }
}

HWND
SK_Inject_GetFocusWindow (void)
{
  const SK_SharedMemory_v1* pSharedMem =
    (SK_SharedMemory_v1 *)SK_Inject_GetViewOfSharedMemory ();

  if (pSharedMem != nullptr)
  {
    return
      (HWND)((uintptr_t)ULongToHandle (pSharedMem->SystemWide.hWndFocus) & 0xFFFFFFFFUL);
  }

  return nullptr;
}

void
SK_Inject_SetFocusWindow (HWND hWndFocus)
{
  SK_SharedMemory_v1* pSharedMem =
    (SK_SharedMemory_v1 *)SK_Inject_GetViewOfSharedMemory ();

  if (pSharedMem != nullptr)
  {
    if ( g_WinEventHook == nullptr &&
              hWndFocus != nullptr )
    {
      g_WinEventHook =
        SetWinEventHook ( EVENT_SYSTEM_FOREGROUND,
                          EVENT_SYSTEM_FOREGROUND, nullptr,
                            SK_Inject_WinEventHookProc, 0, 0,
                              WINEVENT_OUTOFCONTEXT/*|
                              WINEVENT_SKIPOWNTHREAD*/);
    }

    else
    {
      if ( g_WinEventHook != nullptr &&
                hWndFocus == nullptr )
      {
        if (UnhookWinEvent (g_WinEventHook))
                            g_WinEventHook = nullptr;
      }
    }

    if (                                 hWndFocus == nullptr
         || ChangeWindowMessageFilterEx (hWndFocus, 0xfa57, MSGFLT_ALLOW, nullptr) )
    {
      pSharedMem->SystemWide.hWndFocus = sk::narrow_cast <DWORD> (
          HandleToULong (
            (void *)((uintptr_t)hWndFocus & 0xFFFFFFFFUL)
          ) & 0xFFFFFFFFUL
        );
    }
  }
}

struct {
  bool bLowPriv   = false,
       bQuotaFull = false;
  HWND hWnd       = nullptr;

  bool isValid (void) const
  {
    const
    bool   bValid =                   nullptr != hWnd &&
      (! (bLowPriv || bQuotaFull)) && IsWindow ( hWnd );

    if (  !bValid                  && nullptr != hWnd)
            SK_Inject_SetFocusWindow (nullptr);

    return bValid;
  }
  void setHwnd (HWND wnd)
  {
    if (std::exchange (hWnd, wnd) != wnd)
       reset   (    );
  }
  void postMsg (WPARAM wParam, LPARAM lParam)
  {
    if (! PostMessage (hWnd, /*WM_USER+WM_SETFOCUS*/0xfa57, wParam, lParam))
    {
      switch (GetLastError ())
      {
        case ERROR_ACCESS_DENIED:    bLowPriv   = true; break;
        case ERROR_NOT_ENOUGH_QUOTA: bQuotaFull = true; break;
        default:                                        break;
      }
    }
  }
  void reset   (void)     { bLowPriv = bQuotaFull = false; }
} static focus_ctx;

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
      SK_AutoHandle hMutex (
        CreateMutexW ( nullptr, TRUE, LR"(Local\SK_EtwMutex0)" )
      );

      if (hMutex != nullptr)
      {
        if (WaitForSingleObject (hMutex, INFINITE) == WAIT_OBJECT_0)
        {
          auto constexpr __MaxPresentMonSessions =
            SK_SharedMemory_v1::EtwSessionList_s::__MaxPresentMonSessions;

          auto first_free_idx   = __MaxPresentMonSessions,
               first_uninit_idx = __MaxPresentMonSessions;

          for ( int i = 0;   i < __MaxPresentMonSessions;
                           ++i )
          {
            if ( ( first_free_idx == __MaxPresentMonSessions ) &&
                     pSharedMem->EtwSessions.PresentMon [i].dwPid != 0x0 )
            {
              SK_AutoHandle hProcess (
                OpenProcess ( PROCESS_QUERY_INFORMATION, FALSE,
                                pSharedMem->EtwSessions.PresentMon [i].dwPid )
              );

              DWORD dwExitCode = 0x0;

              if (                      hProcess != nullptr    &&
                   GetExitCodeProcess ( hProcess, &dwExitCode) &&
                                                   dwExitCode  != STILL_ACTIVE )
              {
                first_free_idx = i;
              }

              else if (hProcess == nullptr)
              {
                first_free_idx = i;
              }
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
      SK_AutoHandle hMutex (
        CreateMutexW ( nullptr, TRUE, LR"(Local\SK_EtwMutex0)" )
      );

      if (hMutex != nullptr)
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
      }
    }
  }

  return
    unregistered_count != 0;
}

void
SK_Inject_RenameProcess (void)
{
  auto pPeb =
    (SK_PPEB)NtCurrentTeb ()->ProcessEnvironmentBlock;

  //RtlAcquirePebLock ();
  //EnterCriticalSection (pPeb->FastPebLock);

  //SK_InitUnicodeString (
  //  (PUNICODE_STRING_SK)&pPeb->ProcessParameters->ImagePathName,
  //  L"Special K Backend Service Host"
  //);

  SK_InitUnicodeString (
    (PUNICODE_STRING_SK)&pPeb->ProcessParameters->CommandLine,
    SK_RunLHIfBitness ( 32, L"32-bit Special K Backend Service Host",
                            L"64-bit Special K Backend Service Host" )
  );

  pPeb->ProcessParameters->DllPath.Length       = pPeb->ProcessParameters->CommandLine.Length;
  pPeb->ProcessParameters->ImagePathName.Length = pPeb->ProcessParameters->CommandLine.Length;
  pPeb->ProcessParameters->WindowTitle.Length   = pPeb->ProcessParameters->CommandLine.Length;

  pPeb->ProcessParameters->DllPath.Buffer       = pPeb->ProcessParameters->CommandLine.Buffer;
  pPeb->ProcessParameters->ImagePathName.Buffer = pPeb->ProcessParameters->CommandLine.Buffer;
  pPeb->ProcessParameters->WindowTitle.Buffer   = pPeb->ProcessParameters->CommandLine.Buffer;

  //LeaveCriticalSection (pPeb->FastPebLock);
  //RtlReleasePebLock ();
}

bool
ReadMem (void *addr, void *buf, int size)
{
  const BOOL b = ReadProcessMemory (GetCurrentProcess (), addr, buf, size, nullptr);
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
  int refs = -1;

  if ( HMODULE hNtDll = SK_LoadLibraryW (L"NtDll") ;
               hNtDll != nullptr )
  {
    auto LdrFindEntryForAddress =
        (LdrFindEntryForAddress_pfn)SK_GetProcAddress (hNtDll,
        "LdrFindEntryForAddress");

    if (LdrFindEntryForAddress != nullptr)
    {
      if ( PLDR_DATA_TABLE_ENTRY__SK                  pLdrDataTable;
           NT_SUCCESS (LdrFindEntryForAddress (hDll, &pLdrDataTable)) )
      {
        refs =
          pLdrDataTable->ReferenceCount;
      }
    }

    SK_FreeLibrary (hNtDll);
  }

  return refs;
}

//
//  Queries for .dll module load count, returns 0 if fails.
//
int
GetModuleLoadCount (HMODULE hDll)
{
  // Not supported by earlier versions of windows.
  if (! SK_IsWindows8Point1OrGreater ())
    return 0;

  PROCESS_BASIC_INFORMATION pbi = { };

  HMODULE hNtDll =
    SK_LoadLibraryW (L"NtDll");
  if (!   hNtDll)
      return 0;

  auto NtQueryInformationProcess =
      (NtQueryInformationProcess_pfn)SK_GetProcAddress (hNtDll,
      "NtQueryInformationProcess");

  bool b = false;
  if (NtQueryInformationProcess != nullptr) b = NT_SUCCESS (
      NtQueryInformationProcess ( GetCurrentProcess (),
                                    ProcessBasicInformation, &pbi,
                                                      sizeof (pbi), nullptr )
  );
  SK_FreeLibrary (hNtDll);

  if (! b)
    return 0;

  char* LdrDataOffset =
    (char *)(pbi.PebBaseAddress) + offsetof (PEB, Ldr);

  char*        addr    = nullptr;
  PEB_LDR_DATA LdrData = {     };

  if ((! ReadMem (LdrDataOffset, &addr,    sizeof (void *))) ||
      (! ReadMem (addr,          &LdrData, sizeof (LdrData))) )
    return 0;

  LIST_ENTRY* head = LdrData.InMemoryOrderModuleList.Flink;
  LIST_ENTRY* next = head;

  do {
    LDR_DATA_TABLE_ENTRY   LdrEntry = { };
    LDR_DATA_TABLE_ENTRY* pLdrEntry =
      CONTAINING_RECORD (head, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);

    if (! ReadMem (pLdrEntry, &LdrEntry, sizeof (LdrEntry)))
      return 0;

    if (LdrEntry.DllBase == (void *)hDll)
    {
      //
      //  http://www.geoffchappell.com/studies/windows/win32/ntdll/structs/ldr_data_table_entry.htm
      //
      unsigned long long offDdagNode =
        (unsigned long long)(0x14 - BITNESS) * sizeof (void *);   // See offset on LDR_DDAG_NODE *DdagNode;

      ULONG count        = 0;
      char* addrDdagNode = ((char *)pLdrEntry) + offDdagNode;

      //
      //  http://www.geoffchappell.com/studies/windows/win32/ntdll/structs/ldr_ddag_node.htm
      //  See offset on ULONG LoadCount;
      //
      if ( (! ReadMem (addrDdagNode, &addr,     sizeof (void *))) ||
           (! ReadMem (              addr + 3 * sizeof (void *),
                                    &count,     sizeof (count))) )
          return 0;

      return
        static_cast <int> (count);
    } //if

    head = LdrEntry.InMemoryOrderLinks.Flink;
  }while( head != next );

  return 0;
} //GetModuleLoadCount

void
SK_FreeLibraryAndExitThread (HMODULE hModToFree, DWORD dwExitCode)
{
  __try
  {
    FreeLibraryAndExitThread (hModToFree, dwExitCode);
  }

  __except (EXCEPTION_EXECUTE_HANDLER)
  {
    if (IsDebuggerPresent ())
      __debugbreak ();
  }

  ExitThread (dwExitCode);
}

         HANDLE   g_hPacifierThread = SK_INVALID_HANDLE;
volatile HMODULE  g_hModule_CBT     = 0;
volatile LONG     g_sOnce           = 0;

BOOL
SK_IsImmersiveProcess (HANDLE hProcess = SK_GetCurrentProcess ())
{
  using  IsImmersiveProcess_pfn = BOOL (WINAPI *)(HANDLE);
  static IsImmersiveProcess_pfn
        _IsImmersiveProcess =
        (IsImmersiveProcess_pfn)SK_GetProcAddress (L"user32.dll",
        "IsImmersiveProcess");

  if (     _IsImmersiveProcess != nullptr)
    return _IsImmersiveProcess (hProcess);

  return FALSE;
}

void
SK_Inject_SpawnUnloadListener (void)
{
  if (! InterlockedCompareExchangePointer ((void **)&g_hModule_CBT, (void *)1, nullptr))
  {
    static SK_AutoHandle hHookTeardown (
      OpenEvent ( EVENT_ALL_ACCESS, FALSE,
          SK_RunLHIfBitness (32, LR"(Local\SK_GlobalHookTeardown32)",
                                 LR"(Local\SK_GlobalHookTeardown64)") )
                                );

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

      std::atexit ([]{
        hHookTeardown.Close ();
      });
    }

    g_hPacifierThread = (HANDLE)
      CreateThread (nullptr, 0, [](LPVOID) ->
      DWORD
      {
        HMODULE hModKernel32 =
          SK_LoadLibraryW (L"Kernel32");

        if (hModKernel32 != nullptr)
        {
          // Try the Windows 10 API for Thread Names first, it's ideal unless ... not Win10 :)
           SetThreadDescription_pfn
          _SetThreadDescriptionWin10 =
          (SetThreadDescription_pfn)SK_GetProcAddress (hModKernel32, "SetThreadDescription");

          if (_SetThreadDescriptionWin10 != nullptr) {
              _SetThreadDescriptionWin10 (
                g_hPacifierThread,
                  L"[SK] Global Hook Pacifier"
              );
          }

          SK_FreeLibrary (hModKernel32);
        }

        SetThreadPriority (g_hPacifierThread, THREAD_PRIORITY_TIME_CRITICAL);
        SK_CloseHandle    (g_hPacifierThread);
                           g_hPacifierThread = nullptr;

        const HANDLE
          signals [] =
          {     hHookTeardown,
            __SK_DLL_TeardownEvent };

        if (hHookTeardown.isValid ())
        {
          InterlockedIncrement (&injected_procs);

          const DWORD dwTimeout   = INFINITE;
          const DWORD dwWaitState =
            WaitForMultipleObjects ( 2, signals,
                                 FALSE, dwTimeout );

          // Is Process Actively Using Special K (?)
          if ( dwWaitState      == WAIT_OBJECT_0 &&
               hModHookInstance != nullptr )
          {
            // Global Injection Shutting Down, SK is -ACTIVE- in this App.
            //
            //  ==> Must continue waiting for application exit ...
            //
            WaitForSingleObject (
              __SK_DLL_TeardownEvent, INFINITE
            );
          }

          hHookTeardown.Close ();

          // All clear, one less process to worry about
          InterlockedDecrement  (&injected_procs);
        }

        HMODULE this_module = (HMODULE)
          ReadPointerAcquire ((void **)&g_hModule_CBT);

        InterlockedExchangePointer ((void **)&g_hModule_CBT, nullptr);

        SK_FreeLibraryAndExitThread (this_module, 0x0);

        return 0;
      }, nullptr, CREATE_SUSPENDED, nullptr
    );

    if (g_hPacifierThread != nullptr)
    {
      HMODULE hMod = nullptr;

      GetModuleHandleEx ( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                            (LPCWSTR)SK_GetDLL (), &hMod );

      if (hMod != nullptr)
      {
        InterlockedExchangePointer ((void **)&g_hModule_CBT, hMod);
        ResumeThread               (g_hPacifierThread);
      }

      else
      {
        SetEvent                   (hHookTeardown);
        SK_CloseHandle             (g_hPacifierThread);
      }
    }
  }
}

LRESULT
CALLBACK
DebugProc ( _In_ int    nCode,
            _In_ WPARAM wParam,
            _In_ LPARAM lParam )
{
  return
    CallNextHookEx (
      hHookDebug, nCode, wParam, lParam
    );
}

LRESULT
CALLBACK
ShellProc ( _In_ int    nCode,
            _In_ WPARAM wParam,
            _In_ LPARAM lParam )
{
  return
    CallNextHookEx (
      hHookShell, nCode, wParam, lParam
    );
}

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
        hHookCBT, nCode, wParam, lParam
      );
  }

  // Enabling this is causing the DLL to become stuck in various processes
  //   consider signaling the teardown thread to do it instead of trying to
  //     run this code from inside the context of the program's window pump.
#if 1
  if (game_window.hWnd == nullptr && (HWND)wParam != nullptr)
  {
    if (nCode == HCBT_MOVESIZE)
    {
      HWND hWndGame   = SK_Inject_GetFocusWindow (),
           hWndwParam = (HWND)wParam;

      focus_ctx.setHwnd (hWndGame);

      if (hWndGame != hWndwParam && focus_ctx.isValid ())
      {
        if (SK_Window_TestOverlap (hWndGame, hWndwParam, FALSE))
        {
          focus_ctx.postMsg (wParam, static_cast <LPARAM> (0x0));
        }
      }
    }
  }
#endif

  return
    CallNextHookEx (
      hHookCBT, nCode, wParam, lParam
    );
}

BOOL
SK_TerminatePID ( DWORD dwProcessId, UINT uExitCode )
{
  SK_AutoHandle hProcess (
    OpenProcess ( PROCESS_TERMINATE, FALSE, dwProcessId )
  );

  if (! hProcess.isValid ())
    return FALSE;

  return
    SK_TerminateProcess ( hProcess.m_h, uExitCode );
}


void
__stdcall
SKX_InstallCBTHook (void)
{
  SK_SleepEx (15UL, FALSE);

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
  hHookCBT   =
    SetWindowsHookEx (WH_CBT,     CBTProc,  hModSelf, 0);

  if (hHookCBT != nullptr)
  {
    hHookShell =
      SetWindowsHookEx (WH_SHELL, ShellProc,  hModSelf, 0);

    hHookDebug =
      SetWindowsHookEx (WH_DEBUG, DebugProc,  hModSelf, 0);

    InterlockedExchange (&__SK_HookContextOwner, TRUE);
  }

  FreeLibrary (hModSelf);
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

  SK_AutoHandle hHookTeardown (
    OpenEvent ( (DWORD)EVENT_ALL_ACCESS, FALSE,
      SK_RunLHIfBitness ( 32, LR"(Local\SK_GlobalHookTeardown32)",
                              LR"(Local\SK_GlobalHookTeardown64)") )
                              );

  if (          hHookTeardown.isValid ())
  {   SetEvent (hHookTeardown);  }

  if ( hHookOrig != nullptr &&
         UnhookWindowsHookEx (hHookOrig) )
  {
    if (SK_GetHostAppUtil ()->isInjectionTool ())
    {
      if (INVALID_HANDLE_VALUE   !=  hWhitelistChangeNotification)
        FindCloseChangeNotification (hWhitelistChangeNotification);

      InterlockedDecrement (&injected_procs);
    }

                   whitelist_count = 0;
    RtlZeroMemory (whitelist_patterns, sizeof (whitelist_patterns));
                   blacklist_count = 0;
    RtlZeroMemory (blacklist_patterns, sizeof (blacklist_patterns));

        const DWORD       self_pid = GetCurrentProcessId ();
    std::set <DWORD>   running_pids;
    std::set <DWORD> suspended_pids;
    const LONG          hooked_pid_count =
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
                               SMTO_BLOCK,
                                      8UL,
               &dwpResult );

      if (suspended_pids.empty () || ++tries > 4)
        break;
    }
    while (ReadAcquire (&injected_procs) > 0);

    hHookCBT = nullptr;

    if (hHookShell != nullptr && UnhookWindowsHookEx (hHookShell))
        hHookShell  = nullptr;

    if (hHookDebug != nullptr && UnhookWindowsHookEx (hHookDebug))
        hHookDebug  = nullptr;

    dwHookPID = 0x0;
  }

  FreeLibrary (hModSelf);
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
                 SK_AutoHandle hHookTeardown (
                   OpenEvent ( EVENT_ALL_ACCESS, FALSE,
                       SK_RunLHIfBitness (32, LR"(Local\SK_GlobalHookTeardown32)",
                                              LR"(Local\SK_GlobalHookTeardown64)") )
                                             );

                 if (hHookTeardown.isValid ())
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
    SKX_RemoveCBTHook ();

    const char* szPIDFile =
      SK_RunLHIfBitness ( 32, "SpecialK32.pid",
                              "SpecialK64.pid" );

    FILE* fPID =
      fopen (szPIDFile, "r");

    if (fPID != nullptr)
    {
      const
       DWORD dwPID =
           strtoul ("%lu", nullptr, 0);
      fclose (fPID);

      if ( GetCurrentProcessId () == dwPID ||
                    SK_TerminatePID (dwPID, 0x00) )
      {
        DeleteFileA (szPIDFile);
      }
    }

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
  case SK_RenderAPI::D3D8On12:
    SK_SaveConfig (L"d3d8");
    break;

  case SK_RenderAPI::DDrawOn11:
  case SK_RenderAPI::DDrawOn12:
    SK_SaveConfig (L"ddraw");
    break;
#endif

    case SK_RenderAPI::D3D10:
    case SK_RenderAPI::D3D11:
    case SK_RenderAPI::D3D12:
      SK_SaveConfig (L"dxgi");
      break;

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
    case SK_RenderAPI::D3D8On12:
      lstrcatW (wszOut, L"\\d3d8.dll");
      break;

    case SK_RenderAPI::DDrawOn11:
    case SK_RenderAPI::DDrawOn12:
      lstrcatW (wszOut, L"\\ddraw.dll");
      break;
#endif

    case SK_RenderAPI::D3D10:
    case SK_RenderAPI::D3D11:
    case SK_RenderAPI::D3D12:
      lstrcatW (wszOut, L"\\dxgi.dll");
      break;

    case SK_RenderAPI::OpenGL:
      lstrcatW (wszOut, L"\\OpenGL32.dll");
      break;

    //case SK_RenderAPI::Vulkan:
      //lstrcatW (wszOut, L"\\vk-1.dll");
      //break;
  }


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

  if (! hProcSnap.isValid ())
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

  const bool bHas32BitDLL =
    GetFileAttributesW (L"SpecialK32.dll") != INVALID_FILE_ATTRIBUTES;
  const bool bHas64BitDLL =
    GetFileAttributesW (L"SpecialK64.dll") != INVALID_FILE_ATTRIBUTES;

  //if (GetFileAttributes (L"SKIM64.exe") == INVALID_FILE_ATTRIBUTES)
  if (true)
  {
    if (bHas32BitDLL)
    {
      PathAppendW      ( wszWOW64, L"rundll32.exe");
      SK_ShellExecuteA ( nullptr,
                           "open", SK_WideCharToUTF8 (wszWOW64).c_str (),
                           "SpecialK32.dll,RunDLL_InjectionManager Remove", nullptr, SW_HIDE );
    }

    if (bHas64BitDLL)
    {
      PathAppendW      ( wszSys32, L"rundll32.exe");
      SK_ShellExecuteA ( nullptr,
                           "open", SK_WideCharToUTF8 (wszSys32).c_str (),
                           "SpecialK64.dll,RunDLL_InjectionManager Remove", nullptr, SW_HIDE );
    }
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

    if (bHas32BitDLL)
    {
      PathAppendW      ( wszWOW64, L"rundll32.exe");
      SK_ShellExecuteA ( nullptr,
                           "open", SK_WideCharToUTF8 (wszWOW64).c_str (),
                           "SpecialK32.dll,RunDLL_InjectionManager Remove", nullptr, SW_HIDE );
    }

    if (bHas64BitDLL)
    {
      PathAppendW      ( wszSys32, L"rundll32.exe");
      SK_ShellExecuteA ( nullptr,
                           "open", SK_WideCharToUTF8 (wszSys32).c_str (),
                           "SpecialK64.dll,RunDLL_InjectionManager Remove", nullptr, SW_HIDE );
    }
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

  const bool bHas32BitDLL =
    GetFileAttributesW (L"SpecialK32.dll") != INVALID_FILE_ATTRIBUTES;
  const bool bHas64BitDLL =
    GetFileAttributesW (L"SpecialK64.dll") != INVALID_FILE_ATTRIBUTES;

  if (GetFileAttributes (L"SKIM64.exe") == INVALID_FILE_ATTRIBUTES)
  {
    if (SKX_IsHookingCBT ())
    {
      RunDLL_InjectionManager ( nullptr, nullptr,
                                "Remove", -128 );
    }

    if (bHas32BitDLL)
    {
      PathAppendW      ( wszSys32, L"rundll32.exe");
      SK_ShellExecuteA ( nullptr,
                           "open", SK_WideCharToUTF8 (wszSys32).c_str (),
                           "SpecialK64.dll,RunDLL_InjectionManager Install", nullptr, SW_HIDE );
    }

    if (bHas64BitDLL)
    {
      PathAppendW      ( wszWOW64, L"rundll32.exe");
      SK_ShellExecuteA ( nullptr,
                           "open", SK_WideCharToUTF8 (wszWOW64).c_str (),
                           "SpecialK32.dll,RunDLL_InjectionManager Install", nullptr, SW_HIDE );
    }
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

  for (const
       volatile LONG& hooked_pid : g_sHookedPIDs)
  {
    auto pid =
      ReadAcquire (&hooked_pid);

    if (pid != 0)
    {
      if (i < (SSIZE_T)capacity - 1)
      {
        if (pdwListIter != nullptr)
        {
          SK_AutoHandle hProc (
            OpenProcess ( PROCESS_QUERY_INFORMATION, FALSE,
                          pid )
                        );

          if (hProc.isValid ())
          {
            DWORD                           dwExitCode = 0;
            if (GetExitCodeProcess (hProc, &dwExitCode) &&
                                            dwExitCode == STILL_ACTIVE)
            {
              *pdwListIter = pid;
               pdwListIter++;
               ++i;
            }
          }
        }
      }
    }
  }

  if (pdwListIter != nullptr)
     *pdwListIter  = 0;

  return i;
}

BOOL
SK_IsWindowsVersionOrGreater (DWORD dwMajorVersion, DWORD dwMinorVersion, DWORD dwBuildNumber)
{
  NTSTATUS(WINAPI *RtlGetVersion)(LPOSVERSIONINFOEXW) = nullptr;

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
      std::filesystem::path global_cfg_dir =
        std::filesystem::path (SK_GetInstallPath ()) / LR"(Global\)";

    SK_Inject_ParseWhiteAndBlacklists (global_cfg_dir);

    hWhitelistChangeNotification =
      FindFirstChangeNotificationW (
        global_cfg_dir.c_str (), FALSE,
          FILE_NOTIFY_CHANGE_FILE_NAME  |
          FILE_NOTIFY_CHANGE_SIZE       |
          FILE_NOTIFY_CHANGE_LAST_WRITE
      );

    if (hWhitelistChangeNotification != INVALID_HANDLE_VALUE)
    {
      while ( FindNextChangeNotification (
            hWhitelistChangeNotification ) != FALSE )
      {
        if ( WAIT_OBJECT_0 ==
               WaitForSingleObject (hWhitelistChangeNotification, INFINITE) )
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
  if (StrStrNIW (wszExecutable, L"SteamApps", MAX_PATH) != nullptr)
    return true;

  // Sort of a temporary hack for important games that I support that are
  //   sold on alternative stores to Steam.
  if (     StrStrNIW (wszExecutable, L"ffxv",      MAX_PATH) != nullptr)
    return true;

  else if (StrStrNIW (wszExecutable, L"ff7remake", MAX_PATH) != nullptr)
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
  return
    SK_Inject_TestUserBlacklist (wszExecutable);
}

bool
SK_Inject_IsAdminSupported (void) noexcept
{
  return
    ( bAdmin != FALSE );
}

static bool __SKIF_SuppressExitNotify = false;

void SK_Inject_SuppressExitNotify (void)
{
  __SKIF_SuppressExitNotify = true;
}

// Posted when SK signals one of the various acknowledgement events
constexpr UINT WM_SKIF_EVENT_SIGNAL = WM_USER + 0x3000;

void SK_Inject_WakeUpSKIF (void)
{
  HWND hWndExisting = FindWindow (L"SK_Injection_Frontend", nullptr);

  // Try the notification icon if the traditional window is not present
  if (! hWndExisting)
        hWndExisting = FindWindow (L"SKIF_NotificationIcon", nullptr);
          

  if (hWndExisting != nullptr && IsWindow (hWndExisting))
  {
    DWORD                                    dwPid = 0x0;
    GetWindowThreadProcessId (hWndExisting, &dwPid);
  
    if ( dwPid != 0x0 &&
         dwPid != GetCurrentProcessId () )
    {
      AllowSetForegroundWindow (dwPid);
      PostMessage              (hWndExisting, WM_SKIF_EVENT_SIGNAL, (WPARAM)game_window.hWnd, 0x0);
    }
  }
}

void SK_Inject_BroadcastExitNotify (bool force)
{
  if (! force)
  {
    if (__SKIF_SuppressExitNotify || SK_GetFramesDrawn () == 0)
      return;
  }

  // This is effectively a launcher, its exit indicates the real game has started
  if (SK_IsCurrentGame (SK_GAME_ID::SonicXShadowGenerations))
  {
    // When SONIC_GENERATIONS.exe starts, it will send a start notification.
    if (! SK_IsProcessRunning (L"SONIC_GENERATIONS.exe"))
      SK_Inject_BroadcastInjectionNotify (true);
  }

  // A new signal (23.6.28+) that is broadcast even for local injection
  SK_AutoHandle hInjectExitAckEx (
    OpenEvent ( EVENT_ALL_ACCESS, FALSE,
               LR"(Local\SKIF_InjectExitAckEx)" )
  );

  if (hInjectExitAckEx.isValid ())
  {
    SK_Inject_WakeUpSKIF ();

    SetEvent (hInjectExitAckEx.m_h);
  }

  if (! force)
  {
    // The signal below is only for global injection
    if (! SK_IsInjected ())
      return;
  }

  SK_AutoHandle hInjectExitAck (
    OpenEvent ( EVENT_ALL_ACCESS, FALSE,
               LR"(Local\SKIF_InjectExitAck)" )
  );

  if (hInjectExitAck.isValid ())
  {
    SK_Inject_WakeUpSKIF ();

    SetEvent (hInjectExitAck.m_h);
  }
}

void SK_Inject_BroadcastInjectionNotify (bool force)
{
  // This is effectively a launcher, ignore it.
  if (force == false && SK_IsCurrentGame (SK_GAME_ID::SonicXShadowGenerations))
    return;

  // A new signal (23.6.28+) that is broadcast even for local injection
  SK_AutoHandle hInjectAckEx (
    OpenEvent ( EVENT_ALL_ACCESS, FALSE,
               LR"(Local\SKIF_InjectAckEx)" )
  );

  if (hInjectAckEx.isValid ())
  {
    SK_Inject_WakeUpSKIF ();

    SetEvent (hInjectAckEx.m_h);
  }

  if (! force)
  {
    // The original signal below denotes successful global injection
    if (! SK_IsInjected ())
      return;
  }

  SK_AutoHandle hInjectAck (
    OpenEvent ( EVENT_ALL_ACCESS, FALSE,
               LR"(Local\SKIF_InjectAck)" )
  );

  if (hInjectAck.isValid ())
  {
    SK_Inject_WakeUpSKIF ();

    SetEvent (hInjectAck.m_h);
  }
}

float
SK_Inject_GetInjectionDelayInSeconds (void)
{
  // Would lead to deadlock
  if (SK_IsModuleLoaded (L"EOSOVH-Win64-Shipping.dll"))
    return 0.0f;

  return config.system.global_inject_delay;
}

SK_SharedMemory_v1::SK_SharedMemory_v1 (void)
{
  HighDWORD = { sizeof (SK_SharedMemory_v1) -
                sizeof (uint32_t) };
}