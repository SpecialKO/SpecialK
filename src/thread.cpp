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

#define __SK_SUBSYSTEM__ L"ThreadUtil"

#include <SpecialK/log.h>
#include <SpecialK/tls.h>
#include <SpecialK/thread.h>
#include <SpecialK/utility.h>

#include <SpecialK/ini.h>
#include <SpecialK/hooks.h>
#include <SpecialK/config.h>

#include <SpecialK/diagnostics/debug_utils.h>

#include <string>


///////////////////////////////////////////////////////////////////////////
//
// Thread Name Assignment for Meaningful Debug Identification
//
//  ** Necessary given the number of lambdas serving as thread functions
//      in this codebase and the truly useless name mangling that causes.
//
///////////////////////////////////////////////////////////////////////////
HRESULT WINAPI SetThreadDescription_NOP (HANDLE, PCWSTR) { return E_NOTIMPL; }
HRESULT WINAPI GetThreadDescription_NOP (HANDLE, PWSTR*) { return E_NOTIMPL; }


typedef HRESULT (WINAPI *SetThreadDescription_pfn)(HANDLE, PCWSTR);
                         SetThreadDescription_pfn
                         SetThreadDescription_Original = nullptr;

const DWORD MAGIC_THREAD_EXCEPTION = 0x406D1388;

#include <concurrent_unordered_map.h>
#include <concurrent_unordered_set.h>

concurrency::concurrent_unordered_map <DWORD, std::wstring> _SK_ThreadNames;
concurrency::concurrent_unordered_set <DWORD>               _SK_SelfTitledThreads;

// Game has given this thread a custom name, it's special :)
bool
SK_Thread_HasCustomName (DWORD dwTid)
{
  if (_SK_SelfTitledThreads.count (dwTid) != 0)
    return true;

  return false;
}

std::wstring
SK_Thread_GetName (DWORD dwTid)
{
  auto it  =
    _SK_ThreadNames.find (dwTid);

  if (it != _SK_ThreadNames.end ())
    return (*it).second;

  return L"";
}

std::wstring
SK_Thread_GetName (HANDLE hThread)
{
  return SK_Thread_GetName (GetThreadId (hThread));
}

extern "C" {

#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
  DWORD  dwType;     // Always 4096
  LPCSTR szName;     // Pointer to name (in user addr space).
  DWORD  dwThreadID; // Thread ID (-1=caller thread).
  DWORD  dwFlags;    // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

SetThreadDescription_pfn SetThreadDescription = &SetThreadDescription_NOP;
GetThreadDescription_pfn GetThreadDescription = &GetThreadDescription_NOP;

// Avoid SEH unwind problems
void
__make_self_titled (DWORD dwTid)
{
  _SK_SelfTitledThreads.insert (dwTid);
}

using RtlRaiseException_pfn = void (WINAPI *)(_In_ PEXCEPTION_RECORD ExceptionRecord);
extern "C" RtlRaiseException_pfn RtlRaiseException_Original;

HRESULT
WINAPI
SetCurrentThreadDescription (_In_ PCWSTR lpThreadDescription)
{
  SK_TLS* pTLS = nullptr;

  if ( SK_GetHostAppUtil ().isInjectionTool () || ((pTLS = SK_TLS_Bottom ()) == nullptr) )
    return S_OK;


  bool non_empty =
    lstrlenW (lpThreadDescription) != 0;


  if (non_empty && pTLS != nullptr)
  {
    DWORD dwTid = GetCurrentThreadId ();

    __make_self_titled (dwTid);

    // Push this to the TLS datastore so we can get thread names even
    //   when no debugger is attached.
    wcsncpy (pTLS->debug.name, lpThreadDescription, 255);

    _SK_ThreadNames [dwTid] = lpThreadDescription;


    char      szDesc [256] = { };
    wcstombs (szDesc, lpThreadDescription, 255);


    if (SK_IsDebuggerPresent ())
    {
      const THREADNAME_INFO info =
      { 4096, szDesc, (DWORD)-1, 0x0 };

      const DWORD argc = sizeof (info) /
                         sizeof (ULONG_PTR);

      __try
      {
        constexpr int SK_EXCEPTION_CONTINUABLE = 0x0;

        RaiseException ( MAGIC_THREAD_EXCEPTION,
                           SK_EXCEPTION_CONTINUABLE,
                             argc,
                               reinterpret_cast <const ULONG_PTR *>(&info) );
      }
      __except (EXCEPTION_EXECUTE_HANDLER) { }
    }


    // Windows 7 / 8 can go no further, they will have to be happy with the
    //   TLS-backed name or a debugger must catch the exception above.
    //
    if ( SetThreadDescription == &SetThreadDescription_NOP ||
         SetThreadDescription == nullptr ) // Will be nullptr in SKIM64
      return S_OK;


    // Finally, use the new API added in Windows 10...
    HRESULT hr = E_UNEXPECTED;
    HANDLE  hRealHandle;

    if ( DuplicateHandle ( SK_GetCurrentProcess (),
                           SK_GetCurrentThread  (),
                           SK_GetCurrentProcess (),
                             &hRealHandle,
                               THREAD_ALL_ACCESS,
                                 FALSE,
                                    0 ) )
    {
      hr =
        SetThreadDescription (hRealHandle, lpThreadDescription);

      CloseHandle (hRealHandle);
    }

    return hr;
  }

  return S_OK;
}

HRESULT
WINAPI
GetCurrentThreadDescription (_Out_  PWSTR  *threadDescription)
{
  SK_TLS* pTLS =
    SK_TLS_Bottom ();

  // Always use the TLS value if there is one
  if (wcslen (pTLS->debug.name))
  {
    // This is not freed here; the caller is expected to free it!
    *threadDescription =
      (wchar_t *)LocalAlloc (LPTR, 1024);

    wcsncpy (*threadDescription, pTLS->debug.name, 255);

    return S_OK;
  }


  // No TLS, no GetThreadDescription (...) -- we are boned :-\
  //
  if ( GetThreadDescription == &GetThreadDescription_NOP ||
       GetThreadDescription ==  nullptr )
  {
    return E_NOTIMPL;
  }


  HRESULT hr          = E_UNEXPECTED;
  HANDLE  hRealHandle = nullptr;

  if ( DuplicateHandle ( SK_GetCurrentProcess (),
                         SK_GetCurrentThread  (),
                         SK_GetCurrentProcess (),
                           &hRealHandle,
                             THREAD_ALL_ACCESS,
                               FALSE,
                                 0 ) )
  {
    hr =
      GetThreadDescription (hRealHandle, threadDescription);

    CloseHandle (hRealHandle);
  }

  return hr;
}


bool
SK_Thread_InitDebugExtras (void)
{
  static volatile LONG run_once = FALSE;

  if (! InterlockedCompareExchange (&run_once, 1, 0))
  {
    // Only available in Windows 10
    //
    SetThreadDescription =
      (SetThreadDescription_pfn)
        GetProcAddress ( GetModuleHandle (L"kernel32"),
                                           "SetThreadDescription" );
    GetThreadDescription =
      (GetThreadDescription_pfn)
        GetProcAddress ( GetModuleHandle (L"kernel32"),
                                           "GetThreadDescription" );

    if (SetThreadDescription == nullptr)
      SetThreadDescription = &SetThreadDescription_NOP;

    if (GetThreadDescription == nullptr)
      GetThreadDescription = &GetThreadDescription_NOP;

    InterlockedIncrement (&run_once);
  }

  SK_Thread_SpinUntilAtomicMin (&run_once, 2);

  if (GetThreadDescription != &GetThreadDescription_NOP)
    return true;

  return false;
}

// Returns TRUE if the call required a change to priority level
BOOL
__stdcall
SK_Thread_SetCurrentPriority (int prio)
{
  if (SK_Thread_GetCurrentPriority () != prio)
  {
    return SetThreadPriority (SK_GetCurrentThread (), prio);
  }

  return FALSE;
}


int
__stdcall
SK_Thread_GetCurrentPriority (void)
{
  return GetThreadPriority (SK_GetCurrentThread ());
}

} /* extern "C" */  


extern "C" SetThreadAffinityMask_pfn SetThreadAffinityMask_Original = nullptr;

DWORD_PTR
WINAPI
SetThreadAffinityMask_Detour (
  _In_ HANDLE    hThread,
  _In_ DWORD_PTR dwThreadAffinityMask )
{
  DWORD_PTR dwRet = 0;
  DWORD     dwTid = GetThreadId (hThread);
  SK_TLS*   pTLS  =
    (dwTid == GetCurrentThreadId ()) ?
      SK_TLS_Bottom   (     )        :
      SK_TLS_BottomEx (dwTid);


  if (pTLS != nullptr && pTLS->scheduler.lock_affinity)
  {
    dwRet =
      pTLS->scheduler.affinity_mask;
  }

  else
  {
    dwRet =
      SetThreadAffinityMask_Original ( hThread, dwThreadAffinityMask );
  }


  if ( pTLS != nullptr && dwRet != 0 &&
    (! pTLS->scheduler.lock_affinity) )
  {
    pTLS->scheduler.affinity_mask = dwThreadAffinityMask;
  }

  return dwRet;
}





struct SK_ThreadBaseParams {
  LPTHREAD_START_ROUTINE lpStartFunc;
  const wchar_t*         lpThreadName;
  LPVOID                 lpUserParams;
  HANDLE                 hHandleToStuffInternally;
};

DWORD
WINAPI
SKX_ThreadThunk ( LPVOID lpUserPassThrough )
{
  SK_ThreadBaseParams *pStartParams =
 (SK_ThreadBaseParams *)lpUserPassThrough;

  while (pStartParams->hHandleToStuffInternally == INVALID_HANDLE_VALUE)
    SleepEx (0, TRUE);

  SK_TLS* pTLS =
    SK_TLS_Bottom ();

  if (pTLS)
  {
    pTLS->debug.handle = pStartParams->hHandleToStuffInternally;
    pTLS->debug.tid    = GetCurrentThreadId ();
  }

  LocalFree ((HLOCAL)pStartParams);

  DWORD dwRet =
    pStartParams->lpStartFunc (pStartParams->lpUserParams);

  return dwRet;
}

extern "C"
HANDLE
WINAPI
SK_Thread_CreateEx ( LPTHREAD_START_ROUTINE lpStartFunc,
                     const wchar_t*       /*lpThreadName*/,
                     LPVOID                 lpUserParams )
{
  SK_ThreadBaseParams *params =
 (SK_ThreadBaseParams *)LocalAlloc ( 0x0, sizeof (SK_ThreadBaseParams) );

  *params = {
    lpStartFunc,  nullptr,
    lpUserParams, INVALID_HANDLE_VALUE
  };

  DWORD dwTid = 0;

  params->hHandleToStuffInternally =
    CreateThread ( nullptr, 0,
                     SKX_ThreadThunk,
                       (LPVOID)params,
                         0x0, &dwTid );

  return params->hHandleToStuffInternally;
}

extern "C"
void
WINAPI
SK_Thread_Create (LPTHREAD_START_ROUTINE lpStartFunc, LPVOID lpUserParams)
{
  SK_Thread_CreateEx ( lpStartFunc, nullptr, lpUserParams );
}






extern "C"
bool
WINAPI
SK_Thread_CloseSelf (void)
{
  SK_TLS* pTLS =
    SK_TLS_Bottom ();

  HANDLE hCopyAndSwapHandle = INVALID_HANDLE_VALUE;

  std::swap (pTLS->debug.handle, hCopyAndSwapHandle);

  if (! CloseHandle (hCopyAndSwapHandle))
  {
    std::swap (pTLS->debug.handle, hCopyAndSwapHandle);

    return false;
  }

  return true;
}