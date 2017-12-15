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

#include <SpecialK/diagnostics/debug_utils.h>
#include <SpecialK/diagnostics/compatibility.h>


#include <SpecialK/config.h>
#include <SpecialK/core.h>
#include <SpecialK/hooks.h>
#include <SpecialK/log.h>
#include <SpecialK/resource.h>
#include <SpecialK/utility.h>

#include <Windows.h>

// Fix warnings in dbghelp.h
#pragma warning (disable : 4091)

#define _NO_CVCONST_H
#define _IMAGEHLP_SOURCE_
//#pragma comment (lib, "dbghelp.lib")
#include <dbghelp.h>

iSK_Logger game_debug;

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
    ExitProcess_pfn callthrough = ExitProcess_Original;
    ExitProcess_Original        = nullptr;
    callthrough (uExitCode);
  }

  else
    ExitProcess (uExitCode);
}



void
WINAPI
OutputDebugStringA_Detour (LPCSTR lpOutputString)
{
  // fprintf is stupid, but lpOutputString already contains a newline and
  //   fputs would just add another one...
  game_debug.LogEx (true,   L"%-24ws:  %hs", SK_GetCallerName ().c_str (),
                                             lpOutputString);
  fprintf          (stdout,  "%s",           lpOutputString);

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
  fprintf          (stdout,  "%ws",          lpOutputString);

  if (! wcsstr (lpOutputString, L"\n"))
    game_debug.LogEx (false, L"\n");

  // NVIDIA's drivers do something weird, we cannot call the trampoline and
  //   must bail-out, or the NVIDIA streaming service will crash the game!~
  //
  //OutputDebugStringW_Original (lpOutputString);
}

bool spoof_debugger = false;

using IsDebuggerPresent_pfn = BOOL (WINAPI *)(void);
IsDebuggerPresent_pfn IsDebuggerPresent_Original = nullptr;

BOOL
WINAPI
IsDebuggerPresent_Detour (void)
{
  if (spoof_debugger)
    return FALSE;

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
}

bool
SK::Diagnostics::Debugger::Allow (bool bAllow)
{
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

  return bAllow;
}

void
SK::Diagnostics::Debugger::SpawnConsole (void)
{
  AllocConsole ();

  static volatile LONG init = FALSE;

  if (! InterlockedCompareExchange (&init, 1, 0))
  {
    freopen ("CONIN$",  "r", stdin);
    freopen ("CONOUT$", "w", stdout);
    freopen ("CONOUT$", "w", stderr);

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

  if (IsDebuggerPresent_Original)
    return IsDebuggerPresent_Original ();

  return FALSE;
}







#include <Shlwapi.h>

HMODULE
SK_Debug_LoadHelper (void)
{
  static HMODULE hModDbgHelp = nullptr;

  if (hModDbgHelp != nullptr)
    return hModDbgHelp;

  wchar_t wszSystemDbgHelp [MAX_PATH * 2] = { };

  GetSystemDirectory (wszSystemDbgHelp, MAX_PATH * 2 - 1);
  PathAppendW (wszSystemDbgHelp, L"dbghelp.dll");

  return (hModDbgHelp = LoadLibraryW (wszSystemDbgHelp));
}

BOOL
IMAGEAPI
SymRefreshModuleList (
  _In_ HANDLE hProcess
)
{
  typedef BOOL (IMAGEAPI *SymRefreshModuleList_pfn)(HANDLE hProcess);

  static SymRefreshModuleList_pfn SymRefreshModuleList_Imp =
    (SymRefreshModuleList_pfn)
      GetProcAddress ( SK_Debug_LoadHelper (), "SymRefreshModuleList" );

  if (SymRefreshModuleList_Imp != nullptr)
    return SymRefreshModuleList_Imp (hProcess);

  return FALSE;
}

BOOL
IMAGEAPI
StackWalk64(
    _In_ DWORD MachineType,
    _In_ HANDLE hProcess,
    _In_ HANDLE hThread,
    _Inout_ LPSTACKFRAME64 StackFrame,
    _Inout_ PVOID ContextRecord,
    _In_opt_ PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine,
    _In_opt_ PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,
    _In_opt_ PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine,
    _In_opt_ PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress
    )
{
  typedef BOOL (IMAGEAPI *StackWalk64_pfn)(_In_ DWORD MachineType,
                                           _In_ HANDLE hProcess,
                                           _In_ HANDLE hThread,
                                           _Inout_ LPSTACKFRAME64 StackFrame,
                                           _Inout_ PVOID ContextRecord,
                                           _In_opt_ PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine,
                                           _In_opt_ PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,
                                           _In_opt_ PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine,
                                           _In_opt_ PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress);

  static StackWalk64_pfn StackWalk64_Imp =
    (StackWalk64_pfn)
      GetProcAddress ( SK_Debug_LoadHelper (), "StackWalk64" );

  if (StackWalk64_Imp != nullptr)
  {
    return StackWalk64_Imp ( MachineType, hProcess, hThread, StackFrame, ContextRecord,
                             ReadMemoryRoutine, FunctionTableAccessRoutine, GetModuleBaseRoutine,
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
  typedef BOOL (IMAGEAPI *StackWalk_pfn)(_In_     DWORD                          MachineType,
                                         _In_     HANDLE                         hProcess,
                                         _In_     HANDLE                         hThread,
                                         _Inout_  LPSTACKFRAME                   StackFrame,
                                         _Inout_  PVOID                          ContextRecord,
                                         _In_opt_ PREAD_PROCESS_MEMORY_ROUTINE   ReadMemoryRoutine,
                                         _In_opt_ PFUNCTION_TABLE_ACCESS_ROUTINE FunctionTableAccessRoutine,
                                         _In_opt_ PGET_MODULE_BASE_ROUTINE       GetModuleBaseRoutine,
                                         _In_opt_ PTRANSLATE_ADDRESS_ROUTINE     TranslateAddress);

  static StackWalk_pfn StackWalk_Imp =
    (StackWalk_pfn)
      GetProcAddress ( SK_Debug_LoadHelper (), "StackWalk" );

  if (StackWalk_Imp != nullptr)
  {
    return StackWalk_Imp ( MachineType, hProcess, hThread, StackFrame, ContextRecord,
                             ReadMemoryRoutine, FunctionTableAccessRoutine, GetModuleBaseRoutine,
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
  typedef DWORD (IMAGEAPI *SymSetOptions_pfn)(_In_ DWORD SymOptions);

  static SymSetOptions_pfn SymSetOptions_Imp =
    (SymSetOptions_pfn)
      GetProcAddress ( SK_Debug_LoadHelper (), "SymSetOptions" );

  if (SymSetOptions_Imp != nullptr)
  {
    return SymSetOptions_Imp ( SymOptions );
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
  typedef DWORD64 (IMAGEAPI *SymGetModuleBase64_pfn)(_In_ HANDLE  hProcess,
                                                     _In_ DWORD64 qwAddr);

  static SymGetModuleBase64_pfn SymGetModuleBase64_Imp =
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
  typedef DWORD (IMAGEAPI *SymGetModuleBase_pfn)(_In_ HANDLE  hProcess,
                                                 _In_ DWORD   dwAddr);

  static SymGetModuleBase_pfn SymGetModuleBase_Imp =
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
  typedef BOOL (IMAGEAPI *SymGetLineFromAddr64_pfn)(_In_  HANDLE           hProcess,
                                                    _In_  DWORD64          qwAddr,
                                                    _Out_ PDWORD           pdwDisplacement,
                                                    _Out_ PIMAGEHLP_LINE64 Line64);


  static SymGetLineFromAddr64_pfn SymGetLineFromAddr64_Imp =
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
  typedef BOOL (IMAGEAPI *SymGetLineFromAddr_pfn)(_In_  HANDLE         hProcess,
                                                  _In_  DWORD          dwAddr,
                                                  _Out_ PDWORD         pdwDisplacement,
                                                  _Out_ PIMAGEHLP_LINE Line);

  static SymGetLineFromAddr_pfn SymGetLineFromAddr_Imp =
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
  typedef BOOL (IMAGEAPI *SymInitialize_pfn)( _In_     HANDLE hProcess,
                                              _In_opt_ PCSTR  UserSearchPath,
                                              _In_     BOOL   fInvadeProcess );


  static SymInitialize_pfn SymInitialize_Imp =
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
  typedef BOOL (IMAGEAPI *SymUnloadModule_pfn)( _In_ HANDLE hProcess,
                                                _In_ DWORD  BaseOfDll );


  static SymUnloadModule_pfn SymUnloadModule_Imp =
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
  typedef BOOL (IMAGEAPI *SymUnloadModule64_pfn)( _In_ HANDLE  hProcess,
                                                  _In_ DWORD64 BaseOfDll );


  static SymUnloadModule64_pfn SymUnloadModule64_Imp =
    (SymUnloadModule64_pfn)
      GetProcAddress ( SK_Debug_LoadHelper (), "SymUnloadModule64" );

  if (SymUnloadModule64_Imp != nullptr)
  {
    return SymUnloadModule64_Imp ( hProcess, BaseOfDll );
  }

  return FALSE;
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
  typedef BOOL (IMAGEAPI *SymFromAddr_pfn)( _In_      HANDLE       hProcess,
                                            _In_      DWORD64      Address,
                                            _Out_opt_ PDWORD64     Displacement,
                                            _Inout_   PSYMBOL_INFO Symbol );

  static SymFromAddr_pfn SymFromAddr_Imp =
    (SymFromAddr_pfn)
      GetProcAddress ( SK_Debug_LoadHelper (), "SymFromAddr" );

  if (SymFromAddr_Imp != nullptr)
  {
    return SymFromAddr_Imp ( hProcess, Address, Displacement, Symbol );
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
  typedef DWORD (IMAGEAPI *SymLoadModule_pfn)( _In_     HANDLE hProcess,
                                               _In_opt_ HANDLE hFile,
                                               _In_opt_ PCSTR  ImageName,
                                               _In_opt_ PCSTR  ModuleName,
                                               _In_     DWORD  BaseOfDll,
                                               _In_     DWORD  SizeOfDll );

  static SymLoadModule_pfn SymLoadModule_Imp =
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
  typedef DWORD64 (IMAGEAPI *SymLoadModule64_pfn)( _In_     HANDLE  hProcess,
                                                   _In_opt_ HANDLE  hFile,
                                                   _In_opt_ PCSTR   ImageName,
                                                   _In_opt_ PCSTR   ModuleName,
                                                   _In_     DWORD64 BaseOfDll,
                                                   _In_     DWORD   SizeOfDll );

  static SymLoadModule64_pfn SymLoadModule64_Imp =
    (SymLoadModule64_pfn)
      GetProcAddress ( SK_Debug_LoadHelper (), "SymLoadModule64" );

  if (SymLoadModule64_Imp != nullptr)
  {
    return SymLoadModule64_Imp ( hProcess, hFile, ImageName, ModuleName, BaseOfDll, SizeOfDll );
  }

  return FALSE;
}