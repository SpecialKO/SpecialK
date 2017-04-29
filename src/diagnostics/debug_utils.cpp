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
#define _CRT_SECURE_NO_WARNINGS

#include <SpecialK/diagnostics/debug_utils.h>

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
#include <dbghelp.h>

iSK_Logger game_debug;

typedef BOOL (WINAPI *TerminateProcess_pfn)(HANDLE hProcess, UINT uExitCode);
TerminateProcess_pfn TerminateProcess_Original = nullptr;

typedef void (WINAPI *ExitProcess_pfn)(UINT uExitCode);

ExitProcess_pfn ExitProcess_Original = nullptr;
ExitProcess_pfn ExitProcess_Hook     = nullptr;

BOOL
__stdcall
SK_TerminateParentProcess (UINT uExitCode)
{
  if (TerminateProcess_Original != nullptr) {
    return TerminateProcess_Original (GetCurrentProcess (), uExitCode);
  } else {
    return TerminateProcess (GetCurrentProcess (), uExitCode);
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

extern void
SK_UnhookLoadLibrary (void);

void
WINAPI
ExitProcess_Detour (UINT uExitCode)
{
  // Since many, many games don't shutdown cleanly, let's unload ourself.
  SK_SelfDestruct      ();
  ExitProcess_Original (uExitCode);
}

typedef void (WINAPI *OutputDebugStringA_pfn)(LPCSTR lpOutputString);
OutputDebugStringA_pfn OutputDebugStringA_Original = nullptr;

typedef void (WINAPI *OutputDebugStringW_pfn)(LPCWSTR lpOutputString);
OutputDebugStringW_pfn OutputDebugStringW_Original = nullptr;

void
WINAPI
OutputDebugStringA_Detour (LPCSTR lpOutputString)
{
  // fprintf is stupid, but lpOutputString already contains a newline and
  //   fputs would just add another one...
  game_debug.LogEx (true,   L"%hs", lpOutputString);
  fprintf          (stdout,  "%s",  lpOutputString);

  // NVIDIA's drivers do something weird, we cannot call the trampoline and
  //   must bail-out, or the NVIDIA streaming service will crash the game!~
  //
  //OutputDebugStringA_Original (lpOutputString);
}

void
WINAPI
OutputDebugStringW_Detour (LPCWSTR lpOutputString)
{
  game_debug.LogEx (true,   L"%s",  lpOutputString);
  fprintf          (stdout,  "%ws", lpOutputString);

  // NVIDIA's drivers do something weird, we cannot call the trampoline and
  //   must bail-out, or the NVIDIA streaming service will crash the game!~
  //
  //OutputDebugStringW_Original (lpOutputString);
}

bool spoof_debugger = false;

typedef BOOL (WINAPI *IsDebuggerPresent_pfn)(void);
IsDebuggerPresent_pfn IsDebuggerPresent_Original = nullptr;

BOOL
WINAPI
IsDebuggerPresent_Detour (void)
{
  if (spoof_debugger)
    return FALSE;

  return IsDebuggerPresent_Original ();
}

bool
SK::Diagnostics::Debugger::Allow (bool bAllow)
{
  SK_CreateDLLHook2 ( L"Kernel32.dll",
                      "IsDebuggerPresent",
                     IsDebuggerPresent_Detour,
          (LPVOID *)&IsDebuggerPresent_Original );

  spoof_debugger = bAllow;

  SK_CreateDLLHook2 ( L"kernel32.dll", "OutputDebugStringA",
                     OutputDebugStringA_Detour,
           (LPVOID*)&OutputDebugStringA_Original );

  SK_CreateDLLHook2 ( L"kernel32.dll", "OutputDebugStringW",
                     OutputDebugStringW_Detour,
           (LPVOID*)&OutputDebugStringW_Original );

  SK_CreateDLLHook2 ( L"kernel32.dll", "ExitProcess",
                     ExitProcess_Detour,
           (LPVOID*)&ExitProcess_Original );

  MH_ApplyQueued ();

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

    SK_CreateDLLHook2 ( L"kernel32.dll", "TerminateProcess",
                       TerminateProcess_Detour,
             (LPVOID*)&TerminateProcess_Original );

    MH_ApplyQueued ();
  }
}

BOOL
SK::Diagnostics::Debugger::CloseConsole (void)
{
  return FreeConsole ();
}

BOOL
WINAPI
SK_IsDebuggerPresent (void) {
  if (IsDebuggerPresent_Original == nullptr) {
    SK::Diagnostics::Debugger::Allow (); // DONTCARE, just init
  }

  if (IsDebuggerPresent_Original)
    return IsDebuggerPresent_Original ();

  return FALSE;
}