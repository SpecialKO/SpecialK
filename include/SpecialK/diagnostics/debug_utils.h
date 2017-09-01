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

#ifndef __SK__DEBUG_UTILS_H__
#define __SK__DEBUG_UTILS_H__

#include <Windows.h>

namespace SK
{
  namespace Diagnostics
  {
    namespace Debugger
    {
      bool Allow        (bool bAllow = true);
      void SpawnConsole (void);
      BOOL CloseConsole (void);
    }
  }
}


void WINAPI SK_SymRefreshModuleList (HANDLE hProc = GetCurrentProcess ());
BOOL WINAPI SK_IsDebuggerPresent    (void);

using TerminateProcess_pfn   = BOOL (WINAPI *)(HANDLE hProcess, UINT uExitCode);
using ExitProcess_pfn        = void (WINAPI *)(UINT   uExitCode);

using OutputDebugStringA_pfn = void (WINAPI *)(LPCSTR  lpOutputString);
using OutputDebugStringW_pfn = void (WINAPI *)(LPCWSTR lpOutputString);

#endif /* __SK__DEBUG_UTILS_H__ */
