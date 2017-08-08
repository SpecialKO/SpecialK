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

typedef void (WINAPI *DebugBreak_pfn)(void);
DebugBreak_pfn DebugBreak_Original = nullptr;

__declspec (noinline)
void
WINAPI
DebugBreak_Detour (void)
{
  //if (config.debug.allow_break)
  //  return DebugBreak_Original ();

  return;
}

bool
SK::Diagnostics::Debugger::Allow (bool bAllow)
{
  SK_CreateDLLHook2 (       L"kernel32.dll",
                             "IsDebuggerPresent",
                              IsDebuggerPresent_Detour,
reinterpret_cast <LPVOID *> (&IsDebuggerPresent_Original) );

  spoof_debugger = bAllow;

  SK_CreateDLLHook2 (      L"kernel32.dll",
                            "OutputDebugStringA",
                             OutputDebugStringA_Detour,
reinterpret_cast <LPVOID *> (&OutputDebugStringA_Original) );

  SK_CreateDLLHook2 (       L"kernel32.dll",
                             "OutputDebugStringW",
                              OutputDebugStringW_Detour,
reinterpret_cast <LPVOID *> (&OutputDebugStringW_Original) );

  SK_CreateDLLHook2 (       L"kernel32.dll",
                             "ExitProcess",
                              ExitProcess_Detour,
reinterpret_cast <LPVOID *> (&ExitProcess_Original) );

  SK_CreateDLLHook2 (       L"kernel32.dll",
                             "DebugBreak",
                              DebugBreak_Detour,
reinterpret_cast <LPVOID *> (&DebugBreak_Original) );

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

    SK_CreateDLLHook2 (     L"kernel32.dll",
                             "TerminateProcess",
                              TerminateProcess_Detour,
reinterpret_cast <LPVOID *> (&TerminateProcess_Original) );
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