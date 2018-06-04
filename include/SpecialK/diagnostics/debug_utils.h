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
#include <SpecialK/tls.h>

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


std::wstring SK_Thread_GetName (DWORD  dwTid);
std::wstring SK_Thread_GetName (HANDLE hThread);


#include <cassert>

#define SK_ASSERT_NOT_THREADSAFE() {                \
  static DWORD dwLastThread = 0;                    \
                                                    \
  assert ( dwLastThread == 0 ||                     \
           dwLastThread == GetCurrentThreadId () ); \
                                                    \
  dwLastThread = GetCurrentThreadId ();             \
}

#define SK_ASSERT_NOT_DLLMAIN_THREAD() assert (! SK_TLS_Bottom ()->debug.in_DllMain);

// https://gist.github.com/TheWisp/26097ee941ce099be33cfe3095df74a6
//
#include <functional>

template <class F>
struct DebuggableLambda : F
{
  template <class F2>

  DebuggableLambda ( F2&& func, const wchar_t* file, int line) :
    F (std::forward <F2> (func) ),
                    file (file)  ,
                    line (line)     {
  }

  using F::operator ();

  const wchar_t *file;
        int      line;
};

template <class F>
auto MakeDebuggableLambda (F&& func, const wchar_t* file, int line) ->
DebuggableLambda <typename std::remove_reference <F>::type>
{
  return { std::forward <F> (func), file, line };
}

#define  ENABLE_DEBUG_LAMBDA

#ifdef   ENABLE_DEBUG_LAMBDA
# define SK_DEBUG_LAMBDA(F) \
  MakeDebuggableLambda (F, __FILEW__, __LINE__)
# else
# define SK_DEBUGGABLE_LAMBDA(F) F
#endif

#endif /* __SK__DEBUG_UTILS_H__ */
