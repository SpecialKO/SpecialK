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

struct IUnknown;
#include <Unknwnbase.h>

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

      extern FILE *fStdIn,
                  *fStdOut,
                  *fStdErr;
    };
  }
}


void WINAPI SK_SymRefreshModuleList (HANDLE hProc = GetCurrentProcess ());
BOOL WINAPI SK_IsDebuggerPresent    (void);

BOOL __stdcall SK_TerminateProcess (UINT uExitCode);

using TerminateProcess_pfn   = BOOL (WINAPI *)(HANDLE hProcess, UINT uExitCode);
using ExitProcess_pfn        = void (WINAPI *)(UINT   uExitCode);

using OutputDebugStringA_pfn = void (WINAPI *)(LPCSTR  lpOutputString);
using OutputDebugStringW_pfn = void (WINAPI *)(LPCWSTR lpOutputString);



#define _IMAGEHLP_SOURCE_
#include <DbgHelp.h>

using SymGetSearchPathW_pfn = BOOL (IMAGEAPI *)( _In_  HANDLE hProcess,
                                                 _Out_ PWSTR  SearchPath,
                                                 _In_  DWORD  SearchPathLength );

using SymSetSearchPathW_pfn = BOOL (IMAGEAPI *)( _In_     HANDLE hProcess,
                                                 _In_opt_ PCWSTR SearchPath );

using SymRefreshModuleList_pfn = BOOL (IMAGEAPI *)(HANDLE hProcess);

using StackWalk64_pfn = BOOL (IMAGEAPI *)(_In_     DWORD                            MachineType,
                                          _In_     HANDLE                           hProcess,
                                          _In_     HANDLE                           hThread,
                                          _Inout_  LPSTACKFRAME64                   StackFrame,
                                          _Inout_  PVOID                            ContextRecord,
                                          _In_opt_ PREAD_PROCESS_MEMORY_ROUTINE64   ReadMemoryRoutine,
                                          _In_opt_ PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,
                                          _In_opt_ PGET_MODULE_BASE_ROUTINE64       GetModuleBaseRoutine,
                                          _In_opt_ PTRANSLATE_ADDRESS_ROUTINE64     TranslateAddress);

using StackWalk_pfn = BOOL (IMAGEAPI *)(_In_     DWORD                          MachineType,
                                        _In_     HANDLE                         hProcess,
                                        _In_     HANDLE                         hThread,
                                        _Inout_  LPSTACKFRAME                   StackFrame,
                                        _Inout_  PVOID                          ContextRecord,
                                        _In_opt_ PREAD_PROCESS_MEMORY_ROUTINE   ReadMemoryRoutine,
                                        _In_opt_ PFUNCTION_TABLE_ACCESS_ROUTINE FunctionTableAccessRoutine,
                                        _In_opt_ PGET_MODULE_BASE_ROUTINE       GetModuleBaseRoutine,
                                        _In_opt_ PTRANSLATE_ADDRESS_ROUTINE     TranslateAddress);

using SymSetOptions_pfn      = DWORD (IMAGEAPI *)  (_In_ DWORD SymOptions);
using SymGetModuleBase64_pfn = DWORD64 (IMAGEAPI *)(_In_ HANDLE  hProcess,
                                                    _In_ DWORD64 qwAddr);

using SymGetModuleBase_pfn     = DWORD (IMAGEAPI *)(_In_ HANDLE  hProcess,
                                                    _In_ DWORD   dwAddr);
using SymGetLineFromAddr64_pfn = BOOL (IMAGEAPI *)( _In_  HANDLE           hProcess,
                                                    _In_  DWORD64          qwAddr,
                                                    _Out_ PDWORD           pdwDisplacement,
                                                    _Out_ PIMAGEHLP_LINE64 Line64 );

using SymGetLineFromAddr_pfn = BOOL (IMAGEAPI *)( _In_  HANDLE         hProcess,
                                                  _In_  DWORD          dwAddr,
                                                  _Out_ PDWORD         pdwDisplacement,
                                                  _Out_ PIMAGEHLP_LINE Line );
using SymUnloadModule_pfn   = BOOL (IMAGEAPI *)( _In_ HANDLE hProcess,
                                                 _In_ DWORD  BaseOfDll );
using SymUnloadModule64_pfn = BOOL (IMAGEAPI *)( _In_ HANDLE  hProcess,
                                                 _In_ DWORD64 BaseOfDll );


using SymFromAddr_pfn   = BOOL (IMAGEAPI *)( _In_      HANDLE       hProcess,
                                             _In_      DWORD64      Address,
                                             _Out_opt_ PDWORD64     Displacement,
                                             _Inout_   PSYMBOL_INFO Symbol );

using SymInitialize_pfn = BOOL (IMAGEAPI *)( _In_     HANDLE hProcess,
                                             _In_opt_ PCSTR  UserSearchPath,
                                             _In_     BOOL   fInvadeProcess );
using SymCleanup_pfn    = BOOL (IMAGEAPI *)( _In_ HANDLE hProcess );


using SymLoadModule_pfn   = DWORD (IMAGEAPI *)( _In_     HANDLE hProcess,
                                                _In_opt_ HANDLE hFile,
                                                _In_opt_ PCSTR  ImageName,
                                                _In_opt_ PCSTR  ModuleName,
                                                _In_     DWORD  BaseOfDll,
                                                _In_     DWORD  SizeOfDll );
using SymLoadModule64_pfn = DWORD64 (IMAGEAPI *)( _In_     HANDLE  hProcess,
                                                  _In_opt_ HANDLE  hFile,
                                                  _In_opt_ PCSTR   ImageName,
                                                  _In_opt_ PCSTR   ModuleName,
                                                  _In_     DWORD64 BaseOfDll,
                                                  _In_     DWORD   SizeOfDll );


std::wstring& SK_Thread_GetName (DWORD  dwTid);
std::wstring& SK_Thread_GetName (HANDLE hThread);


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

extern bool SK_Debug_IsCrashing (void);


#include <concurrent_unordered_set.h>

extern concurrency::concurrent_unordered_set <HMODULE>&
SK_DbgHlp_Callers (void);

#define dbghelp_callers SK_DbgHlp_Callers ()


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



#ifndef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(P) (P)
#endif

std::wstring
SK_SEH_SummarizeException (_In_ struct _EXCEPTION_POINTERS* ExceptionInfo, bool crash_log = false);


void
SK_SEH_LogException ( unsigned int        nExceptionCode,
                      EXCEPTION_POINTERS* pException,
                      LPVOID              lpRetAddr );

class SK_SEH_IgnoredException
{
public:
  SK_SEH_IgnoredException ( unsigned int        nExceptionCode,
                            EXCEPTION_POINTERS* pException,
                            LPVOID              lpRetAddr )
  {
    SK_SEH_LogException ( nExceptionCode, pException,
                                         lpRetAddr );
  }

  // Really, and truly ignored, since we know nothing about it
  SK_SEH_IgnoredException (void) noexcept
  {
  }
};


#define SK_BasicStructuredExceptionTranslator         \
  []( unsigned int        nExceptionCode,             \
      EXCEPTION_POINTERS* pException )->              \
  void {                                              \
    throw                                             \
      SK_SEH_IgnoredException (                       \
        nExceptionCode, pException,                   \
          _ReturnAddress ()   );                      \
  }

#define SK_FilteringStructuredExceptionTranslator(Filter) \
  []( unsigned int        nExceptionCode,             \
      EXCEPTION_POINTERS* pException )->              \
  void {                                              \
    throw                                             \
      SK_SEH_IgnoredException (                       \
        nExceptionCode, pException,                   \
          _ReturnAddress ()   );                      \
    if (nExceptionCode != Filter)                     \
      RaiseException ( nExceptionCode, pException->ExceptionRecord->ExceptionFlags,         \
                                       pException->ExceptionRecord->NumberParameters,       \
                                       pException->ExceptionRecord->ExceptionInformation ); \
  }


#endif /* __SK__DEBUG_UTILS_H__ */
