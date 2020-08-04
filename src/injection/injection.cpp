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

#include <regex>
#include <sddl.h>

#define SK_INVALID_HANDLE nullptr

NtUserSetWindowsHookEx_pfn
NtUserSetWindowsHookEx    = nullptr;

NtUserCallNextHookEx_pfn
NtUserCallNextHookEx      = nullptr;

NtUserUnhookWindowsHookEx_pfn
NtUserUnhookWindowsHookEx = nullptr;


HMODULE hModHookInstance = nullptr;

// It's not possible to store a structure in the shared data segment.
//
//   This struct will be filled-in when SK boots up using the loose mess of
//     variables below, in order to make working with that data less insane.
//
SK_InjectionRecord_s __SK_InjectionHistory [MAX_INJECTED_PROC_HISTORY] = { 0 };

#pragma data_seg (".SK_Hooks")
extern "C"
{
//__declspec (dllexport) HANDLE hShutdownSignal= INVALID_HANDLE_VALUE;
  DWORD        dwHookPID = 0x0;     // Process that owns the CBT hook
  HHOOK        hHookCBT  = nullptr; // CBT hook
  BOOL         bAdmin    = FALSE;   // Is SKIM64 able to inject into admin apps?

  LONG         g_sHookedPIDs [MAX_INJECTED_PROCS]                 =  {   0   };

  wchar_t      __SK_InjectionHistory_name   [MAX_INJECTED_PROC_HISTORY * MAX_PATH] =  {   0   };
  DWORD        __SK_InjectionHistory_ids    [MAX_INJECTED_PROC_HISTORY]            =  {   0   };
  __time64_t   __SK_InjectionHistory_inject [MAX_INJECTED_PROC_HISTORY]            =  {   0   };
  __time64_t   __SK_InjectionHistory_eject  [MAX_INJECTED_PROC_HISTORY]            =  {   0   };
  bool         __SK_InjectionHistory_crash  [MAX_INJECTED_PROC_HISTORY]            =  { false };

  ULONG64      __SK_InjectionHistory_frames [MAX_INJECTED_PROC_HISTORY]            =  {   0   };
  SK_RenderAPI __SK_InjectionHistory_api    [MAX_INJECTED_PROC_HISTORY]            =  {
    SK_RenderAPI::Reserved, SK_RenderAPI::Reserved, SK_RenderAPI::Reserved, SK_RenderAPI::Reserved,
    SK_RenderAPI::Reserved, SK_RenderAPI::Reserved, SK_RenderAPI::Reserved, SK_RenderAPI::Reserved,
    SK_RenderAPI::Reserved, SK_RenderAPI::Reserved, SK_RenderAPI::Reserved, SK_RenderAPI::Reserved,
    SK_RenderAPI::Reserved, SK_RenderAPI::Reserved, SK_RenderAPI::Reserved, SK_RenderAPI::Reserved,
    SK_RenderAPI::Reserved, SK_RenderAPI::Reserved, SK_RenderAPI::Reserved, SK_RenderAPI::Reserved,
    SK_RenderAPI::Reserved, SK_RenderAPI::Reserved, SK_RenderAPI::Reserved, SK_RenderAPI::Reserved,
    SK_RenderAPI::Reserved, SK_RenderAPI::Reserved, SK_RenderAPI::Reserved, SK_RenderAPI::Reserved,
    SK_RenderAPI::Reserved, SK_RenderAPI::Reserved, SK_RenderAPI::Reserved, SK_RenderAPI::Reserved,
    SK_RenderAPI::Reserved, SK_RenderAPI::Reserved, SK_RenderAPI::Reserved, SK_RenderAPI::Reserved,
    SK_RenderAPI::Reserved, SK_RenderAPI::Reserved, SK_RenderAPI::Reserved, SK_RenderAPI::Reserved,
    SK_RenderAPI::Reserved, SK_RenderAPI::Reserved, SK_RenderAPI::Reserved, SK_RenderAPI::Reserved,
    SK_RenderAPI::Reserved, SK_RenderAPI::Reserved, SK_RenderAPI::Reserved, SK_RenderAPI::Reserved,
    SK_RenderAPI::Reserved, SK_RenderAPI::Reserved, SK_RenderAPI::Reserved, SK_RenderAPI::Reserved,
    SK_RenderAPI::Reserved, SK_RenderAPI::Reserved, SK_RenderAPI::Reserved, SK_RenderAPI::Reserved,
    SK_RenderAPI::Reserved, SK_RenderAPI::Reserved, SK_RenderAPI::Reserved, SK_RenderAPI::Reserved,
    SK_RenderAPI::Reserved, SK_RenderAPI::Reserved, SK_RenderAPI::Reserved, SK_RenderAPI::Reserved
  };

  __declspec (dllexport) volatile LONG SK_InjectionRecord_s::count                 =  0L;
  __declspec (dllexport) volatile LONG SK_InjectionRecord_s::rollovers             =  0L;

  __declspec (dllexport)          wchar_t whitelist_patterns [16 * MAX_PATH] = { 0 };
  __declspec (dllexport)          int     whitelist_count                    =   0;
  __declspec (dllexport)          wchar_t blacklist_patterns [16 * MAX_PATH] = { 0 };
  __declspec (dllexport)          int     blacklist_count                    =   0;
  __declspec (dllexport) volatile LONG    injected_procs                     =   0;

  static constexpr LONG MAX_HOOKED_PROCS = 4096;

  // Recordkeeping on processes with CBT hooks; required to release the DLL
  //   in any process that has become suspended since hook install.
  volatile LONG  num_hooked_pids                =   0;
  volatile DWORD hooked_pids [MAX_HOOKED_PROCS] = { };
};
#pragma data_seg ()
#pragma comment  (linker, "/SECTION:.SK_Hooks,RWS")

extern void SK_Process_Snapshot    (void);
extern bool SK_Process_IsSuspended (DWORD dwPid);
extern bool SK_Process_Suspend     (DWORD dwPid);
extern bool SK_Process_Resume      (DWORD dwPid);

extern volatile LONG  __SK_HookContextOwner;

SK_InjectionRecord_s*
SK_Inject_GetRecord (int idx)
{
  wcsncpy_s
          (__SK_InjectionHistory [idx].process.name,   MAX_PATH-1, &__SK_InjectionHistory_name    [idx * MAX_PATH], _TRUNCATE);
           __SK_InjectionHistory [idx].process.id      = __SK_InjectionHistory_ids    [idx];
           __SK_InjectionHistory [idx].process.inject  = __SK_InjectionHistory_inject [idx];
           __SK_InjectionHistory [idx].process.eject   = __SK_InjectionHistory_eject  [idx];
           __SK_InjectionHistory [idx].process.crashed = __SK_InjectionHistory_crash  [idx];

           __SK_InjectionHistory [idx].render.api      = __SK_InjectionHistory_api    [idx];
           __SK_InjectionHistory [idx].render.frames   = __SK_InjectionHistory_frames [idx];

  return &__SK_InjectionHistory [idx];
}

static LONG local_record = 0;

void
SK_Inject_InitShutdownEvent (void)
{
  if (dwHookPID == 0)
  {
    bAdmin           = SK_IsAdmin ();

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

    if ( (uintptr_t)hTeardown > 0 &&
                  GetLastError () == ERROR_ALREADY_EXISTS )
    {
      ResetEvent (hTeardown);
    }

    dwHookPID        = GetCurrentProcessId ();
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

    if (hProc == INVALID_HANDLE_VALUE)
    {
      ReadAcquire (&hooked_pid);
    }

    else
    {
      DWORD dwExitCode = STILL_ACTIVE;

      GetExitCodeProcess (hProc, &dwExitCode);

      if (dwExitCode != STILL_ACTIVE)
        ReadAcquire (&hooked_pid);
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


class SK_Auto_Local {
public:
  explicit SK_Auto_Local (LPVOID* ppMem) : mem_ (ppMem)
  { };

  ~SK_Auto_Local (void) noexcept
  {
    if (mem_ != nullptr && (*mem_) != nullptr)
    {
      SK_LocalFree ((HLOCAL)*mem_);
                            *mem_ = nullptr;
    }
  }

private:
  LPVOID* mem_;
};

DWORD
SetPermissions (const std::wstring& wstrFilePath)
{
  PACL                 pOldDACL = nullptr,
                       pNewDACL = nullptr;
  PSECURITY_DESCRIPTOR pSD      = nullptr;
  EXPLICIT_ACCESS      eaAccess = {     };
  SECURITY_INFORMATION siInfo   = DACL_SECURITY_INFORMATION;
  DWORD                dwResult = ERROR_SUCCESS;
  PSID                 pSID     = nullptr;

  SK_Auto_Local auto_sid  ((LPVOID *)&pSID);
  SK_Auto_Local auto_dacl ((LPVOID *)&pNewDACL);

  // Get a pointer to the existing DACL
  dwResult =
    GetNamedSecurityInfo ( wstrFilePath.c_str (),
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
    SetNamedSecurityInfo ( const_cast <LPWSTR> (wstrFilePath.c_str ()),
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

  SetPermissions (
    SK_GetModuleFullName (__SK_hModSelf)
  );

  for (volatile LONG& hooked_pid : g_sHookedPIDs)
  {
    if (! InterlockedCompareExchange (&hooked_pid, GetCurrentProcessId (), 0))
    {
      ULONG injection_idx = InterlockedIncrement (&SK_InjectionRecord_s::count);

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

      wchar_t    wszName [MAX_PATH + 2] = { };
      wcsncpy_s (wszName, MAX_PATH,
                 SK_GetModuleFullName (SK_GetModuleHandle (nullptr)).c_str (),
                          _TRUNCATE);

      PathStripPath (wszName);
      wcsncpy_s (&__SK_InjectionHistory_name [local_record * MAX_PATH], MAX_PATH,
                     wszName,                                          _TRUNCATE);

      // Hold a reference so that removing the CBT hook doesn't crash the software
      GetModuleHandleEx ( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                          GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            (LPCWSTR)&SK_Inject_AcquireProcess,
                               &hModHookInstance );

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


#define HSHELL_MONITORCHANGED         16
#define HSHELL_HIGHBIT            0x8000
#define HSHELL_FLASH              (HSHELL_REDRAW|HSHELL_HIGHBIT)
#define HSHELL_RUDEAPPACTIVATED   (HSHELL_WINDOWACTIVATED|HSHELL_HIGHBIT)

// Don't use a local static variable, since that drags in the MSVCRT to do
//   the dirty work, just make this a global.
HMODULE hModule_CBT = nullptr;

LRESULT
CALLBACK
CBTProc ( _In_ int    nCode,
          _In_ WPARAM wParam,
          _In_ LPARAM lParam )
{
  if (nCode < 0)
  {
    LRESULT lRet =
      CallNextHookEx (nullptr, nCode, wParam, lParam);

    // ...

    return lRet;
  }

  if ( hModule_CBT == nullptr &&
         GetModuleHandleEx ( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                               (LPCWSTR)&CBTProc, &hModule_CBT )
     )
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

    HANDLE hThread =
    CreateThread (nullptr, 0, [](LPVOID lpUser) -> //-V513
    DWORD
    {
      HMODULE hMod =
        *((HMODULE *)lpUser);

      //
      // Exception handling is not valid until after the CRT invokes
      //   dllmain_dispatch, hopefully it has done so by the time this
      //     thread begins execution.
      //
      //   * Don't move this code outside of the thread, and don't use
      //       the Visual C++ CRT _beginthread (...) construct within this
      //         hook.
      //
      //     ==> In fact, don't use any part of the C/C++ standard library
      //           unless you are fond of impossible to debug problems !!
      //
      __try
      {
        SetThreadPriority (GetCurrentThread (), THREAD_PRIORITY_LOWEST);

        const char* szDesc =
          "[SK] Global Hook Pacifier";

        THREADNAME_INFO info = {  };
        info.dwType     =      4096;
        info.szName     =    szDesc;
        info.dwThreadID = (DWORD)-1;
        info.dwFlags    =       0x0;

        const DWORD argc = sizeof (info) /
                           sizeof (ULONG_PTR);

        RaiseException ( SK_WINNT_THREAD_NAME_EXCEPTION,
                           0x0, argc,
                             (const ULONG_PTR *)&info );
      }
      __except (EXCEPTION_EXECUTE_HANDLER) { };


      HANDLE hHookTeardown =
        OpenEvent ( EVENT_ALL_ACCESS, FALSE,
            SK_RunLHIfBitness (32, LR"(Local\SK_GlobalHookTeardown32)",
                                   LR"(Local\SK_GlobalHookTeardown64)") );

      // Every 20 seconds anything we're injected into has to wake up and
      //   give us a chance to unload.
      DWORD  dwMilliseconds = 20000uL;
      HANDLE hWaitTimer =
        CreateWaitableTimer (nullptr , FALSE, nullptr);

      if ( hWaitTimer    != SK_INVALID_HANDLE &&
           hHookTeardown != SK_INVALID_HANDLE    )
      {
        LARGE_INTEGER liDelay = { };
                      liDelay.QuadPart =
          (LONGLONG)(-10000.0l * (long double)dwMilliseconds);

        HANDLE events [] =
             { hWaitTimer,
               hHookTeardown };

        if ( SetWaitableTimer ( hWaitTimer, &liDelay,
                                  dwMilliseconds, nullptr,
                                                  nullptr, TRUE )
           )
        {
          DWORD  dwWaitStatus  = WAIT_TIMEOUT;
          while (dwWaitStatus != WAIT_FAILED)
          {
            dwWaitStatus =
              WaitForMultipleObjectsEx ( 2, events,        FALSE,
                                           dwMilliseconds, FALSE );

            if (dwWaitStatus == WAIT_OBJECT_0)
              continue;

            // Any other wait state is unexpected and the safest behavior
            //   is to let this thread run to completion and unload the DLL.
            break;
          }

          CancelWaitableTimer (hWaitTimer);
          CloseHandle         (hWaitTimer);
        }

        CloseHandle (hHookTeardown);
      }

      FreeLibraryAndExitThread (hMod, 0x0);

      return 0;
    }, (LPVOID)&hModule_CBT, 0x0, nullptr);

    if (hThread != SK_INVALID_HANDLE)
      CloseHandle (hThread);
  }

  return
    CallNextHookEx (
      nullptr,
        nCode, wParam, lParam
    );
}

BOOL
SK_TerminatePID ( DWORD dwProcessId, UINT uExitCode )
{
  SK_AutoHandle hProcess (
    OpenProcess ( PROCESS_TERMINATE, FALSE, dwProcessId )
  );

  if (hProcess == INVALID_HANDLE_VALUE)
    return FALSE;

  return
    SK_TerminateProcess ( hProcess, uExitCode );
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
    InterlockedIncrementRelease (&injected_procs);
  }

  HMODULE hModSelf;
  GetModuleHandleEx ( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                        (LPCWSTR)&CBTProc, &hModSelf );

  hHookCBT =
    SetWindowsHookEx (WH_CBT, CBTProc, hModSelf, 0);
}


void
__stdcall
SKX_RemoveCBTHook (void)
{
  if (SK_GetHostAppUtil ()->isInjectionTool ())
  {
    InterlockedDecrement (&injected_procs);
  }

  HHOOK hHookOrig =
    SKX_GetCBTHook ();

  if ( hHookOrig != nullptr &&
         UnhookWindowsHookEx (hHookOrig) )
  {
                         whitelist_count = 0;
    RtlSecureZeroMemory (whitelist_patterns, sizeof (whitelist_patterns));
                         blacklist_count = 0;
    RtlSecureZeroMemory (blacklist_patterns, sizeof (blacklist_patterns));

    HANDLE hHookTeardown =
      OpenEvent ( (DWORD)EVENT_ALL_ACCESS, FALSE,
          SK_RunLHIfBitness (32, LR"(Local\SK_GlobalHookTeardown32)",
                                 LR"(Local\SK_GlobalHookTeardown64)") );

    if ((uintptr_t)hHookTeardown > 0)
    {
      SetEvent (hHookTeardown);
    }

    std::set <DWORD> running_pids;
    std::set <DWORD> suspended_pids;
    LONG             hooked_pid_count =
      std::max   (0L,
        std::min (MAX_HOOKED_PROCS, ReadAcquire (&num_hooked_pids)));

    SK_Process_Snapshot ();

    for ( int i = 0                ;
              i < hooked_pid_count ;
            ++i )
    {
      DWORD dwPid =
        ReadULongAcquire (&hooked_pids [i]);

      if (                      0 == dwPid ||
           suspended_pids.count     (dwPid)||
           running_pids.count       (dwPid)||
           GetCurrentProcessId () == dwPid )
        continue;

      if (SK_Process_IsSuspended (dwPid))
      {
        suspended_pids.emplace (dwPid);
             SK_Process_Resume (dwPid);
      }

      else
        running_pids.emplace (dwPid);
    }

    // If SKX_RemoveCBTHook (...) is successful: (__SK_HookContextOwner = 0)
    if (! ReadAcquire (&__SK_HookContextOwner))
    {
      DWORD_PTR dwpResult = 0;

      SendMessageTimeout ( HWND_BROADCAST,
                             WM_NULL, 0, 0,
                               SMTO_ABORTIFHUNG |
                               SMTO_NOTIMEOUTIFNOTHUNG,
                                 666UL, &dwpResult );

      SK_RunLHIfBitness ( 64, DeleteFileW (L"SpecialK64.pid"),
                              DeleteFileW (L"SpecialK32.pid") );
    }

    hHookCBT  = nullptr;
    hHookOrig = nullptr;

    for ( auto& pid : suspended_pids )
    {
      SK_Process_Suspend (pid);
    }

    if ((uintptr_t)hHookTeardown > 0)
      CloseHandle (hHookTeardown);

    WriteRelease (&__SK_HookContextOwner, FALSE);
  }

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

                 if ((intptr_t)hHookTeardown > 0)
                 {
                   ResetEvent (hHookTeardown);
                 }

                 while ( ReadAcquire       (&__SK_DLL_Attached) ||
                         SK_GetHostAppUtil ()->isInjectionTool () )
                 {
                   HANDLE events [] = { __SK_DLL_TeardownEvent != 0 ?
                                        __SK_DLL_TeardownEvent :
                                            hHookTeardown,
                                            hHookTeardown };

                   DWORD dwWait =
                     WaitForMultipleObjects ( 2, events,
                                                FALSE, INFINITE );

                   if ( dwWait ==   WAIT_OBJECT_0          ||
                        dwWait == ( WAIT_OBJECT_0    + 1 ) ||
                        dwWait ==   WAIT_ABANDONED_0       ||
                        dwWait == ( WAIT_ABANDONED_0 + 1 ) ||
                        dwWait ==   WAIT_TIMEOUT           ||
                        dwWait ==   WAIT_FAILED )
                   {
                      break;
                   }
                 }

                 if ((intptr_t)hHookTeardown > 0)
                 {

                   CloseHandle (hHookTeardown);
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
  }

  if (nCmdShow != -128)
    SK_ExitProcess (0x00);
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
    fclose (fOut);

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

    std::wstring      ver_dir;
    SK_FormatStringW (ver_dir, LR"(%s\Version)", SK_GetConfigPath ());

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

    std::wstring      ver_dir;
    SK_FormatStringW (ver_dir, LR"(%s\Version)", SK_GetConfigPath ());

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
  GetTempFileNameW (SK_GetHostPath (), L"SKI", timeGetTime (), wszTemp);
  MoveFileW        (wszOut,                                    wszTemp);

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

  if (hProcSnap == INVALID_HANDLE_VALUE)
    return false;

  pe32.dwSize = sizeof (PROCESSENTRY32W);

  if (Process32FirstW (hProcSnap, &pe32) != TRUE)
  {
    return false;
  }

  do
  {
    if (StrStrIW (wszProcName, pe32.szExeFile) != nullptr)
    {
      window_t win =
        SK_FindRootWindow (pe32.th32ProcessID);

      SendMessage (win.root, WM_USER + 0x123, 0x00, 0x00);

      return true;
    }
  } while (Process32NextW (hProcSnap, &pe32));

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

void
SK_Inject_ParseWhiteAndBlacklists (std::wstring base_path)
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

    while ( FindNextChangeNotification (
                   hChangeNotification ) != FALSE )
    {
      if ( WAIT_OBJECT_0 ==
             WaitForSingleObject (hChangeNotification, INFINITE) )
      {
        SK_Inject_ParseWhiteAndBlacklists (global_cfg_dir);
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
  if (StrStrIW (wszExecutable, L"ffxv") != nullptr)
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
  if (StrStrNIW (wszExecutable, L"launcher", MAX_PATH) != nullptr)
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