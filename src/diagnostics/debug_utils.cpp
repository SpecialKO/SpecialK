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
#include "debug_utils.h"

#include "../config.h"
#include "../core.h"
#include "../log.h"
#include "../resource.h"

#include <Windows.h>

#define _NO_CVCONST_H
#include <dbghelp.h>

sk_logger_t game_debug;

typedef BOOL (WINAPI *TerminateProcess_pfn)(HANDLE hProcess, UINT uExitCode);
TerminateProcess_pfn TerminateProcess_Original = nullptr;

BOOL
WINAPI
TerminateProcess_Detour (HANDLE hProcess, UINT uExitCode)
{
  OutputDebugString (L" *** BLOCKED TerminateProcess (...) ***");

  return FALSE;
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

  OutputDebugStringA_Original (lpOutputString);
}

void
WINAPI
OutputDebugStringW_Detour (LPCWSTR lpOutputString)
{
  game_debug.LogEx (true,   L"%s",  lpOutputString);
  fprintf          (stdout,  "%ws", lpOutputString);

  OutputDebugStringW_Original (lpOutputString);
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
  SK_CreateDLLHook ( L"Kernel32.dll",
                      "IsDebuggerPresent",
                     IsDebuggerPresent_Detour,
          (LPVOID *)&IsDebuggerPresent_Original );

  spoof_debugger = bAllow;

  SK_CreateDLLHook ( L"kernel32.dll", "OutputDebugStringA",
                     OutputDebugStringA_Detour,
           (LPVOID*)&OutputDebugStringA_Original );

  SK_CreateDLLHook ( L"kernel32.dll", "OutputDebugStringW",
                     OutputDebugStringW_Detour,
           (LPVOID*)&OutputDebugStringW_Original );

  return bAllow;
}

void
SK::Diagnostics::Debugger::SpawnConsole (void)
{
  AllocConsole ();

  freopen ("CONIN$",  "r", stdin);
  freopen ("CONOUT$", "w", stdout);
  freopen ("CONOUT$", "w", stderr);

  SK_CreateDLLHook ( L"kernel32.dll", "TerminateProcess",
                     TerminateProcess_Detour,
           (LPVOID*)&TerminateProcess_Original );
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