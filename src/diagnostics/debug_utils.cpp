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

#define __SK_SUBSYSTEM__ L"DebugUtils"

#include <SpecialK/diagnostics/debug_utils.h>

#include <SpecialK/config.h>
#include <SpecialK/core.h>
#include <SpecialK/hooks.h>
#include <SpecialK/log.h>
#include <SpecialK/resource.h>
#include <SpecialK/utility.h>
#include <SpecialK/thread.h>

#include <Windows.h>
#include <SpecialK/diagnostics/modules.h>
#include <SpecialK/diagnostics/load_library.h>

// Fix warnings in dbghelp.h
#pragma warning (disable : 4091)

#define _NO_CVCONST_H
#define _IMAGEHLP_SOURCE_
//#pragma comment (lib, "dbghelp.lib")
#include <dbghelp.h>

iSK_Logger game_debug;

extern SK_Thread_HybridSpinlock* cs_dbghelp;

typedef LPWSTR (WINAPI *GetCommandLineW_pfn)(void);
GetCommandLineW_pfn     GetCommandLineW_Original   = nullptr;

typedef LPSTR (WINAPI *GetCommandLineA_pfn)(void);
GetCommandLineA_pfn    GetCommandLineA_Original   = nullptr;

TerminateProcess_pfn
                       TerminateProcess_Original   = nullptr;
ExitProcess_pfn        ExitProcess_Original        = nullptr;
ExitProcess_pfn        ExitProcess_Hook            = nullptr;
OutputDebugStringA_pfn OutputDebugStringA_Original = nullptr;
OutputDebugStringW_pfn OutputDebugStringW_Original = nullptr;

BOOL
__stdcall
SK_TerminateParentProcess (UINT uExitCode)
{
  if (TerminateProcess_Original != nullptr)
  {
    return
      TerminateProcess_Original ( GetCurrentProcess (),
                                    uExitCode );
  }

  else
  {
    return
      TerminateProcess ( GetCurrentProcess (),
                           uExitCode );
  }
}

using ResetEvent_pfn = BOOL (WINAPI *)(
  _In_ HANDLE hEvent
);
ResetEvent_pfn ResetEvent_Original = nullptr;

BOOL
WINAPI
ResetEvent_Detour (
  _In_ HANDLE hEvent
)
{
  // Compliance failure in some games makes application verifier useless
  if (hEvent == nullptr)
  {
    SK_RunOnce (dll_log.Log ( L"[DebugUtils] Invalid handle passed to ResetEvent (...) - %s",
                               SK_SummarizeCaller ().c_str () ));
    return TRUE;
  }

  return ResetEvent_Original (hEvent);
}

using CreateThread_pfn = HANDLE (WINAPI *)(
    _In_opt_                  LPSECURITY_ATTRIBUTES  lpThreadAttributes,
    _In_                      SIZE_T                 dwStackSize,
    _In_                      LPTHREAD_START_ROUTINE lpStartAddress,
    _In_opt_ __drv_aliasesMem LPVOID                 lpParameter,
    _In_                      DWORD                  dwCreationFlags,
    _Out_opt_                 LPDWORD                lpThreadId
);
CreateThread_pfn CreateThread_Original = nullptr;

HANDLE
WINAPI
CreateThread_Detour (
    _In_opt_                  LPSECURITY_ATTRIBUTES  lpThreadAttributes,
    _In_                      SIZE_T                 dwStackSize,
    _In_                      LPTHREAD_START_ROUTINE lpStartAddress,
    _In_opt_ __drv_aliasesMem LPVOID                 lpParameter,
    _In_                      DWORD                  dwCreationFlags,
    _Out_opt_                 LPDWORD                lpThreadId )
{
  SK_LOG_CALL ("ThreadBase");

  return CreateThread_Original ( lpThreadAttributes,  dwStackSize,
                                   lpStartAddress,    lpParameter,
                                     dwCreationFlags, lpThreadId   );
}


BOOL
WINAPI
TerminateProcess_Detour (HANDLE hProcess, UINT uExitCode)
{
  UNREFERENCED_PARAMETER (uExitCode);

  if (hProcess == GetCurrentProcess ())
  {
    OutputDebugString ( L" *** BLOCKED TerminateProcess (...) ***\n\t" );
    OutputDebugString ( SK_GetCallerName ().c_str () );

    return FALSE;
  }

  return TerminateProcess_Original (hProcess, uExitCode);
}

void
WINAPI
ExitProcess_Detour (UINT uExitCode)
{
  // Since many, many games don't shutdown cleanly, let's unload ourself.
  SK_SelfDestruct      ();

  if (ExitProcess_Original != nullptr)
  {
    const ExitProcess_pfn callthrough = ExitProcess_Original;
          ExitProcess_Original        = nullptr;
          callthrough (uExitCode);
  }

  else
    ExitProcess (uExitCode);
}




LPWSTR
WINAPI
GetCommandLineW_Detour (void)
{
  SK_LOG_FIRST_CALL

  static
  wchar_t wszFakeOut [MAX_PATH * 4] = { };

  if (*wszFakeOut != L'\0')
    return wszFakeOut;

  lstrcpyW (wszFakeOut, L"\"");
  lstrcatW (wszFakeOut, SK_GetFullyQualifiedApp ());
  lstrcatW (wszFakeOut, L"\"");

  if (! lstrcmpiW ( SK_GetHostApp (), L"RED-Win64-Shipping.exe" ))
  {
    lstrcatW (wszFakeOut, L" -eac-nop-loaded");
    return    wszFakeOut;
  }

  if (! lstrcmpiW ( SK_GetHostApp (), L"DBFighterZ.exe" ))
  {
    lstrcatW (wszFakeOut, L" -eac-nop-loaded");
    return    wszFakeOut;
  }

#ifdef _DEBUG
  if (_wcsicmp (wszFakeOut, GetCommandLineW_Original ()))
    dll_log.Log (L"GetCommandLineW () ==> %ws", GetCommandLineW_Original ());
#endif

  return GetCommandLineW_Original ();
}



LPSTR
WINAPI
GetCommandLineA_Detour (void)
{
  SK_LOG_FIRST_CALL

  return GetCommandLineA_Original ();
}



void
WINAPI
OutputDebugStringA_Detour (LPCSTR lpOutputString)
{
  // fprintf is stupid, but lpOutputString already contains a newline and
  //   fputs would just add another one...
  game_debug.LogEx (true,   L"%-24ws:  %hs", SK_GetCallerName ().c_str (),
                                             lpOutputString);
  fwprintf         (stdout, L"%hs",          lpOutputString);

  if (! strstr (lpOutputString, "\n"))
    game_debug.LogEx (false, L"\n");

  // NVIDIA's drivers do something weird, we cannot call the trampoline and
  //   must bail-out, or the NVIDIA streaming service will crash the game!~
  //
//OutputDebugStringA_Original (lpOutputString);
}

void
WINAPI
OutputDebugStringW_Detour (LPCWSTR lpOutputString)
{
  game_debug.LogEx (true,   L"%-24ws:  %ws", SK_GetCallerName ().c_str (),
                                             lpOutputString);
  fwprintf         (stdout, L"%ws",          lpOutputString);

  if (! wcsstr (lpOutputString, L"\n"))
    game_debug.LogEx (false, L"\n");

  // NVIDIA's drivers do something weird, we cannot call the trampoline and
  //   must bail-out, or the NVIDIA streaming service will crash the game!~
  //
//OutputDebugStringW_Original (lpOutputString);
}

bool spoof_debugger = true;

using IsDebuggerPresent_pfn = BOOL (WINAPI *)(void);
IsDebuggerPresent_pfn IsDebuggerPresent_Original = nullptr;

BOOL
WINAPI
IsDebuggerPresent_Detour (void)
{
  // Community Service Time
  if (SK_GetCurrentGameID () == SK_GAME_ID::FinalFantasyXV)
  {
    static bool killed_ffxv = false;

    if ((! killed_ffxv) && GetThreadPriority (SK_GetCurrentThread ()) == THREAD_PRIORITY_LOWEST)
    {
      killed_ffxv    = SK_Thread_CloseSelf ();
      TerminateThread (SK_GetCurrentThread (), 0x00);
    }
  }

  //if (spoof_debugger)
  //  return FALSE;

  return IsDebuggerPresent_Original ();
}

using DebugBreak_pfn = void (WINAPI *)(void);
DebugBreak_pfn DebugBreak_Original = nullptr;

__declspec (noinline)
void
WINAPI
DebugBreak_Detour (void)
{
  //if (config.debug.allow_break)
  //  return DebugBreak_Original ();

  return DebugBreak_Original ();
}


using RaiseException_pfn = void (WINAPI *)(DWORD,DWORD,DWORD,const ULONG_PTR *);
RaiseException_pfn RaiseException_Original = nullptr;

struct SK_FFXV_Thread { HANDLE hThread; DWORD dwPrio; void setup (); } extern sk_ffxv_vsync,
                                                                              sk_ffxv_async_run;

// Detoured so we can get thread names
__declspec (noinline)
void
WINAPI
RaiseException_Detour (
  _In_       DWORD      dwExceptionCode,
  _In_       DWORD      dwExceptionFlags,
  _In_       DWORD      nNumberOfArguments,
  _In_ const ULONG_PTR *lpArguments )
{
  constexpr static DWORD MAGIC_THREAD_EXCEPTION = 0x406D1388;

  if (dwExceptionCode == MAGIC_THREAD_EXCEPTION)
  {
    #pragma pack(push,8)
    typedef struct tagTHREADNAME_INFO
    {
      DWORD  dwType;     // Always 4096
      LPCSTR szName;     // Pointer to name (in user addr space).
      DWORD  dwThreadID; // Thread ID (-1=caller thread).
      DWORD  dwFlags;    // Reserved for future use, must be zero.
    } THREADNAME_INFO;
    #pragma pack(pop)

    THREADNAME_INFO* info = 
      (THREADNAME_INFO *)lpArguments;

    // Push this to the TLS datastore so we can get thread names even
    //   when no debugger is attached.
    wcsncpy ( SK_TLS_Bottom ()->debug.name,
                SK_UTF8ToWideChar (info->szName).c_str (),
                  255 );

    if (SK_GetCurrentGameID () == SK_GAME_ID::FinalFantasyXV)
    {
      if (StrStrIA (info->szName, "VSync"))
      {
        sk_ffxv_vsync.setup ();
      }

      else if (StrStrA (info->szName, "AsyncFile.Run"))
      {
        sk_ffxv_async_run.setup ();
      }
    }
  }

  return
    RaiseException_Original ( dwExceptionCode, dwExceptionFlags,
                              nNumberOfArguments, lpArguments );
}


bool
SK::Diagnostics::Debugger::Allow (bool bAllow)
{
  if (SK_IsHostAppSKIM ())
    return true;

  SK_CreateDLLHook2 (      L"kernel32.dll",
                            "IsDebuggerPresent",
                             IsDebuggerPresent_Detour,
    static_cast_p2p <void> (&IsDebuggerPresent_Original) );

  spoof_debugger = bAllow;

  SK_CreateDLLHook2 (      L"kernel32.dll",
                            "OutputDebugStringA",
                             OutputDebugStringA_Detour,
    static_cast_p2p <void> (&OutputDebugStringA_Original) );

  SK_CreateDLLHook2 (      L"kernel32.dll",
                            "OutputDebugStringW",
                             OutputDebugStringW_Detour,
    static_cast_p2p <void> (&OutputDebugStringW_Original) );

  SK_CreateDLLHook2 (      L"kernel32.dll",
                            "ExitProcess",
                             ExitProcess_Detour,
    static_cast_p2p <void> (&ExitProcess_Original) );

  SK_CreateDLLHook2 (      L"kernel32.dll",
                            "DebugBreak",
                             DebugBreak_Detour,
    static_cast_p2p <void> (&DebugBreak_Original) );

  if (config.system.trace_create_thread)
  {
    SK_CreateDLLHook2 (      L"kernel32.dll",
                              "CreateThread",
                               CreateThread_Detour,
      static_cast_p2p <void> (&CreateThread_Original) );
  }

  SK_CreateDLLHook2 (  L"kernel32.dll",
                        "GetCommandLineW",
                         GetCommandLineW_Detour,
static_cast_p2p <void> (&GetCommandLineW_Original) );

  SK_CreateDLLHook2 (  L"kernel32.dll",
                        "GetCommandLineA",
                         GetCommandLineA_Detour,
static_cast_p2p <void> (&GetCommandLineA_Original) );

    SK_CreateDLLHook2 (      L"kernel32.dll",
                             "ResetEvent",
                              ResetEvent_Detour,
     static_cast_p2p <void> (&ResetEvent_Original) );

    SK_CreateDLLHook2 (      L"kernel32.dll",
                             "RaiseException",
                              RaiseException_Detour,
     static_cast_p2p <void> (&RaiseException_Original) );

    SK_ApplyQueuedHooks ();

  return bAllow;
}

void
SK::Diagnostics::Debugger::SpawnConsole (void)
{
  AllocConsole ();

  static volatile LONG init = FALSE;

  if (! InterlockedCompareExchange (&init, 1, 0))
  {
    _wfreopen (L"CONIN$",  L"r", stdin);
    _wfreopen (L"CONOUT$", L"w", stdout);
    _wfreopen (L"CONOUT$", L"w", stderr);

    SK_CreateDLLHook2 (      L"kernel32.dll",
                              "TerminateProcess",
                               TerminateProcess_Detour,
      static_cast_p2p <void> (&TerminateProcess_Original) );
  }
}

BOOL
SK::Diagnostics::Debugger::CloseConsole (void)
{
  return FreeConsole ();
}

BOOL
WINAPI
SK_IsDebuggerPresent (void)
{
  if (IsDebuggerPresent_Original == nullptr)
  {
    SK::Diagnostics::Debugger::Allow (); // DONTCARE, just init
  }

  if (     IsDebuggerPresent_Original != nullptr )
    return IsDebuggerPresent_Original ();

  return IsDebuggerPresent ();
}







#include <Shlwapi.h>

HMODULE
SK_Debug_LoadHelper (void)
{
  static HMODULE
       hModDbgHelp  = nullptr;
  if ( hModDbgHelp != nullptr )
    return hModDbgHelp;

  wchar_t wszSystemDbgHelp [MAX_PATH * 2 + 1] = { };

  GetSystemDirectory ( wszSystemDbgHelp, MAX_PATH * 2   );
  PathAppendW        ( wszSystemDbgHelp, L"dbghelp.dll" );

  return (
    hModDbgHelp = SK_Modules.LoadLibraryLL (wszSystemDbgHelp)
  );
}

BOOL
IMAGEAPI
SymRefreshModuleList (
  _In_ HANDLE hProcess
)
{
  using SymRefreshModuleList_pfn = BOOL (IMAGEAPI *)(HANDLE hProcess);

  static auto SymRefreshModuleList_Imp =
    (SymRefreshModuleList_pfn)
      GetProcAddress ( SK_Debug_LoadHelper (), "SymRefreshModuleList" );


  if (SymRefreshModuleList_Imp != nullptr)
  {
    BOOL bRet =
      SymRefreshModuleList_Imp (hProcess);

    return bRet;
  }


  return FALSE;
}

BOOL
IMAGEAPI
StackWalk64(
    _In_     DWORD                            MachineType,
    _In_     HANDLE                           hProcess,
    _In_     HANDLE                           hThread,
    _Inout_  LPSTACKFRAME64                   StackFrame,
    _Inout_  PVOID                            ContextRecord,
    _In_opt_ PREAD_PROCESS_MEMORY_ROUTINE64   ReadMemoryRoutine,
    _In_opt_ PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,
    _In_opt_ PGET_MODULE_BASE_ROUTINE64       GetModuleBaseRoutine,
    _In_opt_ PTRANSLATE_ADDRESS_ROUTINE64     TranslateAddress
    )
{
  using StackWalk64_pfn = BOOL (IMAGEAPI *)(_In_     DWORD                            MachineType,
                                            _In_     HANDLE                           hProcess,
                                            _In_     HANDLE                           hThread,
                                            _Inout_  LPSTACKFRAME64                   StackFrame,
                                            _Inout_  PVOID                            ContextRecord,
                                            _In_opt_ PREAD_PROCESS_MEMORY_ROUTINE64   ReadMemoryRoutine,
                                            _In_opt_ PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,
                                            _In_opt_ PGET_MODULE_BASE_ROUTINE64       GetModuleBaseRoutine,
                                            _In_opt_ PTRANSLATE_ADDRESS_ROUTINE64     TranslateAddress);

  static auto StackWalk64_Imp =
    (StackWalk64_pfn)
      GetProcAddress ( SK_Debug_LoadHelper (), "StackWalk64" );

  if (StackWalk64_Imp != nullptr)
  {
    return
      StackWalk64_Imp ( MachineType,
                          hProcess, hThread,
                            StackFrame, ContextRecord,
                              ReadMemoryRoutine,
                              FunctionTableAccessRoutine,
                              GetModuleBaseRoutine,
                                TranslateAddress );
  }

  return FALSE;
}

BOOL
IMAGEAPI
StackWalk (
    _In_     DWORD                          MachineType,
    _In_     HANDLE                         hProcess,
    _In_     HANDLE                         hThread,
    _Inout_  LPSTACKFRAME                   StackFrame,
    _Inout_  PVOID                          ContextRecord,
    _In_opt_ PREAD_PROCESS_MEMORY_ROUTINE   ReadMemoryRoutine,
    _In_opt_ PFUNCTION_TABLE_ACCESS_ROUTINE FunctionTableAccessRoutine,
    _In_opt_ PGET_MODULE_BASE_ROUTINE       GetModuleBaseRoutine,
    _In_opt_ PTRANSLATE_ADDRESS_ROUTINE     TranslateAddress
    )
{
  using StackWalk_pfn = BOOL (IMAGEAPI *)(_In_     DWORD                          MachineType,
                                          _In_     HANDLE                         hProcess,
                                          _In_     HANDLE                         hThread,
                                          _Inout_  LPSTACKFRAME                   StackFrame,
                                          _Inout_  PVOID                          ContextRecord,
                                          _In_opt_ PREAD_PROCESS_MEMORY_ROUTINE   ReadMemoryRoutine,
                                          _In_opt_ PFUNCTION_TABLE_ACCESS_ROUTINE FunctionTableAccessRoutine,
                                          _In_opt_ PGET_MODULE_BASE_ROUTINE       GetModuleBaseRoutine,
                                          _In_opt_ PTRANSLATE_ADDRESS_ROUTINE     TranslateAddress);

  static auto StackWalk_Imp =
    (StackWalk_pfn)
      GetProcAddress ( SK_Debug_LoadHelper (), "StackWalk" );

  if (StackWalk_Imp != nullptr)
  {
    return
      StackWalk_Imp ( MachineType,
                        hProcess, hThread,
                          StackFrame, ContextRecord,
                            ReadMemoryRoutine,
                            FunctionTableAccessRoutine,
                            GetModuleBaseRoutine,
                              TranslateAddress );
  }

  return FALSE;
}


DWORD
IMAGEAPI
SymSetOptions (
  _In_ DWORD SymOptions
)
{
  using SymSetOptions_pfn = DWORD (IMAGEAPI *)(_In_ DWORD SymOptions);

  static auto SymSetOptions_Imp =
    (SymSetOptions_pfn)
      GetProcAddress ( SK_Debug_LoadHelper (), "SymSetOptions" );

  if (SymSetOptions_Imp != nullptr)
  {
    return
      SymSetOptions_Imp (SymOptions);
  }

  return 0x0;
}


DWORD64
IMAGEAPI
SymGetModuleBase64 (
  _In_ HANDLE  hProcess,
  _In_ DWORD64 qwAddr
)
{
  using SymGetModuleBase64_pfn = DWORD64 (IMAGEAPI *)(_In_ HANDLE  hProcess,
                                                      _In_ DWORD64 qwAddr);

  static auto SymGetModuleBase64_Imp =
    (SymGetModuleBase64_pfn)
      GetProcAddress ( SK_Debug_LoadHelper (), "SymGetModuleBase64" );

  if (SymGetModuleBase64_Imp != nullptr)
  {
    return SymGetModuleBase64_Imp ( hProcess, qwAddr );
  }

  return 0x0;
}

DWORD
IMAGEAPI
SymGetModuleBase (
  _In_ HANDLE hProcess,
  _In_ DWORD  dwAddr
)
{
  using SymGetModuleBase_pfn = DWORD (IMAGEAPI *)(_In_ HANDLE  hProcess,
                                                  _In_ DWORD   dwAddr);

  static auto SymGetModuleBase_Imp =
    (SymGetModuleBase_pfn)
      GetProcAddress ( SK_Debug_LoadHelper (), "SymGetModuleBase" );

  if (SymGetModuleBase_Imp != nullptr)
  {
    return SymGetModuleBase_Imp ( hProcess, dwAddr );
  }

  return 0x0;
}


BOOL
IMAGEAPI
SymGetLineFromAddr64 (
  _In_  HANDLE           hProcess,
  _In_  DWORD64          qwAddr,
  _Out_ PDWORD           pdwDisplacement,
  _Out_ PIMAGEHLP_LINE64 Line64
)
{
  using SymGetLineFromAddr64_pfn = BOOL (IMAGEAPI *)(_In_  HANDLE           hProcess,
                                                     _In_  DWORD64          qwAddr,
                                                     _Out_ PDWORD           pdwDisplacement,
                                                     _Out_ PIMAGEHLP_LINE64 Line64);


  static auto SymGetLineFromAddr64_Imp =
    (SymGetLineFromAddr64_pfn)
      GetProcAddress ( SK_Debug_LoadHelper (), "SymGetLineFromAddr64" );

  if (SymGetLineFromAddr64_Imp != nullptr)
  {
    return SymGetLineFromAddr64_Imp ( hProcess, qwAddr, pdwDisplacement, Line64 );
  }

  return FALSE;
}

BOOL
IMAGEAPI
SymGetLineFromAddr (
  _In_  HANDLE           hProcess,
  _In_  DWORD            dwAddr,
  _Out_ PDWORD           pdwDisplacement,
  _Out_ PIMAGEHLP_LINE   Line
)
{
  using SymGetLineFromAddr_pfn = BOOL (IMAGEAPI *)(_In_  HANDLE         hProcess,
                                                   _In_  DWORD          dwAddr,
                                                   _Out_ PDWORD         pdwDisplacement,
                                                   _Out_ PIMAGEHLP_LINE Line);

  static auto SymGetLineFromAddr_Imp =
    (SymGetLineFromAddr_pfn)
      GetProcAddress ( SK_Debug_LoadHelper (), "SymGetLineFromAddr" );

  if (SymGetLineFromAddr_Imp != nullptr)
  {
    return SymGetLineFromAddr_Imp ( hProcess, dwAddr, pdwDisplacement, Line );
  }

  return FALSE;
}


BOOL
IMAGEAPI
SymInitialize (
  _In_     HANDLE hProcess,
  _In_opt_ PCSTR  UserSearchPath,
  _In_     BOOL   fInvadeProcess
)
{
  using SymInitialize_pfn = BOOL (IMAGEAPI *)( _In_     HANDLE hProcess,
                                               _In_opt_ PCSTR  UserSearchPath,
                                               _In_     BOOL   fInvadeProcess );


  static auto SymInitialize_Imp =
    (SymInitialize_pfn)
      GetProcAddress ( SK_Debug_LoadHelper (), "SymInitialize" );

  if (SymInitialize_Imp != nullptr)
  {
    return SymInitialize_Imp ( hProcess, UserSearchPath, fInvadeProcess );
  }

  return FALSE;
}


BOOL
IMAGEAPI
SymUnloadModule (
  _In_ HANDLE hProcess,
  _In_ DWORD  BaseOfDll
)
{
  using SymUnloadModule_pfn = BOOL (IMAGEAPI *)( _In_ HANDLE hProcess,
                                                 _In_ DWORD  BaseOfDll );


  static auto SymUnloadModule_Imp =
    (SymUnloadModule_pfn)
      GetProcAddress ( SK_Debug_LoadHelper (), "SymUnloadModule" );

  if (SymUnloadModule_Imp != nullptr)
  {
    return SymUnloadModule_Imp ( hProcess, BaseOfDll );
  }

  return FALSE;
}

BOOL
IMAGEAPI
SymUnloadModule64 (
  _In_ HANDLE  hProcess,
  _In_ DWORD64 BaseOfDll
)
{
  using SymUnloadModule64_pfn = BOOL (IMAGEAPI *)( _In_ HANDLE  hProcess,
                                                   _In_ DWORD64 BaseOfDll );


  static auto SymUnloadModule64_Imp =
    (SymUnloadModule64_pfn)
      GetProcAddress ( SK_Debug_LoadHelper (), "SymUnloadModule64" );

  if (SymUnloadModule64_Imp != nullptr)
  {
    return SymUnloadModule64_Imp ( hProcess, BaseOfDll );
  }

  return FALSE;
}


using SymFromAddr_pfn = BOOL (IMAGEAPI *)( _In_      HANDLE       hProcess,
                                           _In_      DWORD64      Address,
                                           _Out_opt_ PDWORD64     Displacement,
                                           _Inout_   PSYMBOL_INFO Symbol );

BOOL
__stdcall
SAFE_SymFromAddr (
  _In_      HANDLE          hProcess,
  _In_      DWORD64         Address,
  _Out_opt_ PDWORD64        Displacement,
  _Inout_   PSYMBOL_INFO    Symbol,
            SymFromAddr_pfn Trampoline )
{
  __try {
    return Trampoline (hProcess, Address, Displacement, Symbol);
  }
  __except (EXCEPTION_EXECUTE_HANDLER) { return FALSE; }
}

BOOL
IMAGEAPI
SymFromAddr (
  _In_      HANDLE       hProcess,
  _In_      DWORD64      Address,
  _Out_opt_ PDWORD64     Displacement,
  _Inout_   PSYMBOL_INFO Symbol
)
{
  using SymFromAddr_pfn = BOOL (IMAGEAPI *)( _In_      HANDLE       hProcess,
                                             _In_      DWORD64      Address,
                                             _Out_opt_ PDWORD64     Displacement,
                                             _Inout_   PSYMBOL_INFO Symbol );

  static auto SymFromAddr_Imp =
    (SymFromAddr_pfn)
      GetProcAddress ( SK_Debug_LoadHelper (), "SymFromAddr" );

  if (SymFromAddr_Imp != nullptr)
  {
    return SAFE_SymFromAddr (hProcess, Address, Displacement, Symbol, SymFromAddr_Imp);
  }

  return FALSE;
}


DWORD
IMAGEAPI
SymLoadModule (
  _In_     HANDLE hProcess,
  _In_opt_ HANDLE hFile,
  _In_opt_ PCSTR  ImageName,
  _In_opt_ PCSTR  ModuleName,
  _In_     DWORD  BaseOfDll,
  _In_     DWORD  SizeOfDll
)
{
  using SymLoadModule_pfn = DWORD (IMAGEAPI *)( _In_     HANDLE hProcess,
                                                _In_opt_ HANDLE hFile,
                                                _In_opt_ PCSTR  ImageName,
                                                _In_opt_ PCSTR  ModuleName,
                                                _In_     DWORD  BaseOfDll,
                                                _In_     DWORD  SizeOfDll );

  static auto SymLoadModule_Imp =
    (SymLoadModule_pfn)
      GetProcAddress ( SK_Debug_LoadHelper (), "SymLoadModule" );

  if (SymLoadModule_Imp != nullptr)
  {
    return SymLoadModule_Imp ( hProcess, hFile, ImageName, ModuleName, BaseOfDll, SizeOfDll );
  }

  return FALSE;
}

DWORD64
IMAGEAPI
SymLoadModule64 (
  _In_     HANDLE  hProcess,
  _In_opt_ HANDLE  hFile,
  _In_opt_ PCSTR   ImageName,
  _In_opt_ PCSTR   ModuleName,
  _In_     DWORD64 BaseOfDll,
  _In_     DWORD   SizeOfDll
)
{
  using SymLoadModule64_pfn = DWORD64 (IMAGEAPI *)( _In_     HANDLE  hProcess,
                                                    _In_opt_ HANDLE  hFile,
                                                    _In_opt_ PCSTR   ImageName,
                                                    _In_opt_ PCSTR   ModuleName,
                                                    _In_     DWORD64 BaseOfDll,
                                                    _In_     DWORD   SizeOfDll );

  static auto SymLoadModule64_Imp =
    (SymLoadModule64_pfn)
      GetProcAddress ( SK_Debug_LoadHelper (), "SymLoadModule64" );

  if (SymLoadModule64_Imp != nullptr)
  {
    return SymLoadModule64_Imp ( hProcess, hFile, ImageName, ModuleName, BaseOfDll, SizeOfDll );
  }

  return FALSE;
}