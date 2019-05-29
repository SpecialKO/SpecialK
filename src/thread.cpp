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

#define __SK_SUBSYSTEM__ L"ThreadUtil"

extern volatile LONG __SK_Init;

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


using SetThreadDescription_pfn = HRESULT (WINAPI *)(HANDLE, PCWSTR);
      SetThreadDescription_pfn
      SetThreadDescription_Original = nullptr;

const DWORD MAGIC_THREAD_EXCEPTION = 0x406D1388;

SK_LazyGlobal <concurrency::concurrent_unordered_map <DWORD, std::wstring>> _SK_ThreadNames;
SK_LazyGlobal <concurrency::concurrent_unordered_set <DWORD>>               _SK_SelfTitledThreads;

// Game has given this thread a custom name, it's special :)
bool
SK_Thread_HasCustomName (DWORD dwTid)
{
  static auto&
    SelfTitled =
      *_SK_SelfTitledThreads;

  return ( SelfTitled.find (dwTid) !=
           SelfTitled.cend (     ) );
}

std::wstring&
SK_Thread_GetName (DWORD dwTid)
{
  static std::wstring noname;

  static auto& names =
    *_SK_ThreadNames;

  const auto it  =
    names.find (dwTid);

  if (it != names.cend ())
    return (*it).second;

  return
    noname;
}

std::wstring&
SK_Thread_GetName (HANDLE hThread)
{
  return
    SK_Thread_GetName (GetThreadId (hThread));
}

extern "C" {
SetThreadDescription_pfn SK_SetThreadDescription = &SetThreadDescription_NOP;
GetThreadDescription_pfn SK_GetThreadDescription = &GetThreadDescription_NOP;

// Avoid SEH unwind problems
void
__make_self_titled (DWORD dwTid)
{
  static auto&
    SelfTitled =
      *_SK_SelfTitledThreads;

  SelfTitled.insert (dwTid);
}

using      RtlRaiseException_pfn = void (WINAPI *)(_In_ PEXCEPTION_RECORD ExceptionRecord);
extern "C" RtlRaiseException_pfn
           RtlRaiseException_Original = nullptr;

void
SK_Thread_RaiseNameException (THREADNAME_INFO* pTni){__try
{
  constexpr int   SK_EXCEPTION_CONTINUABLE
                       = 0x0;
      const DWORD argc = sizeof (*pTni) /
                           sizeof (ULONG_PTR);

  RaiseException ( MAGIC_THREAD_EXCEPTION,
                     SK_EXCEPTION_CONTINUABLE,
                       argc,
      (const ULONG_PTR *)pTni );
}
__except (EXCEPTION_CONTINUE_EXECUTION){}}


HRESULT
WINAPI
SetCurrentThreadDescription (_In_ PCWSTR lpThreadDescription)
{
  if (lpThreadDescription == nullptr)
    return E_POINTER;

  if (SK_GetHostAppUtil ()->isInjectionTool ())
    return S_OK;

  //if (! ReadAcquire (&__SK_DLL_Attached))
  //{
  //  return E_NOT_VALID_STATE;
  //}

  size_t len;

  bool non_empty =
    SUCCEEDED ( StringCbLengthW (
                  lpThreadDescription, 255, &len
                )
              )                           && len > 0;

  if (non_empty)
  {
    auto&
      ThreadNames =
        *_SK_ThreadNames;

    SK_TLS *pTLS     = ( ReadAcquire (&__SK_DLL_Attached) ||
                      (! ReadAcquire (&__SK_DLL_Ending)))  ?
    SK_TLS_Bottom () : nullptr;

    DWORD                 dwTid  = SK_Thread_GetCurrentId ();
    __make_self_titled (dwTid);
           ThreadNames [dwTid] = lpThreadDescription;

    if (pTLS != nullptr)
    {
      // Push this to the TLS datastore so we can get thread names even
      //   when no debugger is attached.
      wcsncpy_s (
        pTLS->debug.name,
          std::min (256, (int)len+1),
            lpThreadDescription,
              _TRUNCATE
      );
    }


    char      szDesc [256] = { };
    wcstombs (szDesc, lpThreadDescription, 255);

    THREADNAME_INFO info = {       };
    info.dwType          =      4096;
    info.szName          =    szDesc;
    info.dwThreadID      = (DWORD)-1;
    info.dwFlags         =       0x0;

    SK_Thread_RaiseNameException (&info);


    // Windows 7 / 8 can go no further, they will have to be happy with the
    //   TLS-backed name or a debugger must catch the exception above.
    //
    if ( SK_SetThreadDescription == &SetThreadDescription_NOP ||
         SK_SetThreadDescription == nullptr ) // Will be nullptr in SKIM64
    {
      return S_OK;
    }


    // Finally, use the new API added in Windows 10...
    HRESULT       hr         = E_UNEXPECTED;
    SK_AutoHandle hRealHandle (INVALID_HANDLE_VALUE);

    if ( DuplicateHandle ( SK_GetCurrentProcess (),
                           SK_GetCurrentThread  (),
                           SK_GetCurrentProcess (),
                             &hRealHandle.m_h,
                               THREAD_ALL_ACCESS,
                                 FALSE,
                                    0 ) )
    {
      hr =
       ( SK_SetThreadDescription ( hRealHandle,
             lpThreadDescription )             );
    }

    return hr;
  }

  return S_OK;
}

HRESULT
WINAPI
GetCurrentThreadDescription (_Out_  PWSTR  *threadDescription)
{
  SK_TLS *pTLS       = ReadAcquire (&__SK_DLL_Attached) ?
    SK_TLS_Bottom () : nullptr;

  // Always use the TLS value if there is one
  if (         pTLS != nullptr   &&
       wcslen (pTLS->debug.name)    )
  {
    // This is not freed here; the caller is expected to free it!
    *threadDescription =
      (wchar_t *)SK_LocalAlloc (LPTR, sizeof (wchar_t) * 1024);

    wcsncpy_s (
      *threadDescription, 1024,
        pTLS->debug.name, _TRUNCATE
    );

    return S_OK;
  }

  // No TLS, no GetThreadDescription (...) -- we are boned :-\
  //
  if ( SK_GetThreadDescription == &GetThreadDescription_NOP ||
       SK_GetThreadDescription ==  nullptr )
  {
    return E_NOTIMPL;
  }

  HRESULT       hr         = E_UNEXPECTED;
  SK_AutoHandle hRealHandle (   nullptr  );

  if ( DuplicateHandle ( SK_GetCurrentProcess (),
                         SK_GetCurrentThread  (),
                         SK_GetCurrentProcess (),
                           &hRealHandle.m_h,
                             THREAD_ALL_ACCESS,
                               FALSE,
                                 0 ) )
  {
    hr =
      SK_GetThreadDescription ( hRealHandle,
                                  threadDescription );
  }

  return hr;
}

using InitializeCriticalSection_pfn = void (WINAPI *)(LPCRITICAL_SECTION lpCriticalSection);
      InitializeCriticalSection_pfn
      InitializeCriticalSection_Original = nullptr;

void
WINAPI
InitializeCriticalSection_Detour (
  LPCRITICAL_SECTION lpCriticalSection
)
{
  InitializeCriticalSectionEx (lpCriticalSection, 0, RTL_CRITICAL_SECTION_FLAG_DYNAMIC_SPIN);
}


bool
SK_Thread_InitDebugExtras (void)
{
  static volatile LONG run_once = FALSE;

  if (! InterlockedCompareExchangeAcquire (&run_once, 1, 0))
  {
    // Hook QPC and Sleep
    SK::Framerate::Init ();

    // Only available in Windows 10
    //
    SK_SetThreadDescription =
      (SetThreadDescription_pfn)
        GetProcAddress ( SK_Modules->getLibrary (L"kernel32", true, true),
                                                 "SetThreadDescription" );
    SK_GetThreadDescription =
      (GetThreadDescription_pfn)
      GetProcAddress ( SK_Modules->getLibrary (L"kernel32", true, true),
                                               "GetThreadDescription" );

    if (SK_SetThreadDescription == nullptr)
      SK_SetThreadDescription = &SetThreadDescription_NOP;

    if (SK_GetThreadDescription == nullptr)
      SK_GetThreadDescription = &GetThreadDescription_NOP;

    InterlockedIncrementRelease (&run_once);
  }

  else
    SK_Thread_SpinUntilAtomicMin (&run_once, 2);

  return
    (SK_GetThreadDescription != &GetThreadDescription_NOP);
}

// Returns TRUE if the call required a change to priority level
BOOL
__stdcall
SK_Thread_SetCurrentPriority (int prio)
{
  if (SK_Thread_GetCurrentPriority () != prio)
  {
    return
      SetThreadPriority (SK_GetCurrentThread (), prio);
  }

  return FALSE;
}


int
__stdcall
SK_Thread_GetCurrentPriority (void)
{
  return
    GetThreadPriority (SK_GetCurrentThread ());
}

} /* extern "C" */


extern "C" SetThreadAffinityMask_pfn SetThreadAffinityMask_Original = nullptr;

DWORD_PTR
WINAPI
SetThreadAffinityMask_Detour (
  _In_ HANDLE    hThread,
  _In_ DWORD_PTR dwThreadAffinityMask )
{
  static SYSTEM_INFO
    sysinfo = {   };

  if (sysinfo.dwNumberOfProcessors == 0)
  {
    SK_GetSystemInfo (&sysinfo);
  }

  DWORD_PTR dwRet = 0;
  DWORD     dwTid = ( hThread ==
    SK_GetCurrentThread (              ) ) ?
        SK_Thread_GetCurrentId (       )   :
                   GetThreadId (hThread);

  if (dwTid == 0)
  {
    return
      SetThreadAffinityMask_Original (
        hThread,
          dwThreadAffinityMask );
  }

  SK_TLS*   pTLS  =
    (dwTid == SK_Thread_GetCurrentId ()) ?
      SK_TLS_Bottom   (     )            :
      SK_TLS_BottomEx (dwTid);


  if ( pTLS != nullptr &&
       pTLS->scheduler->lock_affinity )
  {
    dwRet =
      pTLS->scheduler->affinity_mask;
  }

  else
  {
    dwRet =
      SetThreadAffinityMask_Original (
              hThread,
                dwThreadAffinityMask );
  }


  if ( pTLS != nullptr && dwRet != 0 &&
    (! pTLS->scheduler->lock_affinity) )
  {
    pTLS->scheduler->affinity_mask =
      dwThreadAffinityMask;
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
    static_cast <SK_ThreadBaseParams *> (lpUserPassThrough);

  while (pStartParams->hHandleToStuffInternally == INVALID_HANDLE_VALUE)
    SK_Sleep (1);

  SK_TLS *pTLS       = ReadAcquire (&__SK_DLL_Attached) ?
    SK_TLS_Bottom () : nullptr;

  if (pStartParams->lpThreadName != nullptr)
    SetCurrentThreadDescription (pStartParams->lpThreadName);

  if (pTLS != nullptr)
  {
    pTLS->debug.handle = pStartParams->hHandleToStuffInternally;
    pTLS->debug.tid    = SK_Thread_GetCurrentId ();
  }

  DWORD dwRet =
    pStartParams->lpStartFunc (pStartParams->lpUserParams);

  if (LocalFree_Original != nullptr)
      LocalFree_Original ((HLOCAL)pStartParams);

  return dwRet;
}


extern "C"
HANDLE
WINAPI
SK_Thread_CreateEx ( LPTHREAD_START_ROUTINE lpStartFunc,
                     const wchar_t*         lpThreadName,
                     LPVOID                 lpUserParams )
{
  SK_LOG2 ( ( L" [+] SK_Thread_CreateEx (%ws, %ws, %p)",
                     SK_SummarizeCaller (lpStartFunc).c_str (),
                                       lpThreadName ? lpThreadName
                                                    : L"{Unnamed}",
                                       lpUserParams ),
              L"ThreadBase" );

  SK_ThreadBaseParams
    *params =
      static_cast <SK_ThreadBaseParams *> (
        SK_LocalAlloc ( LPTR, sizeof (SK_ThreadBaseParams) )
      );

  assert (params != nullptr);

  *params = {
    lpStartFunc,  lpThreadName,
    lpUserParams, INVALID_HANDLE_VALUE
  };

  unsigned int dwTid = 0;

  HANDLE hRet  =
    reinterpret_cast <HANDLE> (
      _beginthreadex ( nullptr, 0,
               (_beginthreadex_proc_type)SKX_ThreadThunk,
                 (LPVOID)params,
                   0x0, &dwTid )
    );

  return
    ( (params->hHandleToStuffInternally = hRet) );
}

extern "C"
void
WINAPI
SK_Thread_Create ( LPTHREAD_START_ROUTINE lpStartFunc,
                   LPVOID                 lpUserParams )
{
  SK_Thread_CreateEx (
    lpStartFunc, nullptr, lpUserParams
  );
}






extern "C"
bool
WINAPI
SK_Thread_CloseSelf (void)
{
  SK_TLS *pTLS       = ReadAcquire (&__SK_DLL_Attached) ?
    SK_TLS_Bottom () : nullptr;

  if (pTLS != nullptr)
  {
    HANDLE hCopyAndSwapHandle =
      INVALID_HANDLE_VALUE;

    std::swap   (pTLS->debug.handle, hCopyAndSwapHandle);

    if (! CloseHandle (hCopyAndSwapHandle))
    {
      std::swap (pTLS->debug.handle, hCopyAndSwapHandle);

      return false;
    }
  } else return false;

  return true;
}

SK_LazyGlobal <concurrency::concurrent_unordered_map <DWORD, SK_MMCS_TaskEntry*>> SK_MMCS_TaskMap;

size_t
SK_MMCS_GetTaskCount (void)
{
  static auto& task_map =
    *SK_MMCS_TaskMap;

  return
    task_map.size ();
}

std::vector <SK_MMCS_TaskEntry *>
SK_MMCS_GetTasks (void)
{
  static auto& task_map =
    *SK_MMCS_TaskMap;

  std::vector <SK_MMCS_TaskEntry *> tasks;

  std::transform ( task_map.cbegin (),
                   task_map.cend   (),
                     std::back_inserter (tasks),
                     []( std::pair < DWORD,
                                     SK_MMCS_TaskEntry *> c) ->
                             auto {                return c.second;
                                  }
                 );

  return
    tasks;
}

SK_MMCS_TaskEntry*
SK_MMCS_GetTaskForThreadIDEx ( DWORD dwTid, const char* name,
                                            const char* task1,
                                            const char* task2 )
{
  static auto& task_map =
    *SK_MMCS_TaskMap;

  SK_MMCS_TaskEntry* task_me =
    nullptr;

  if (task_map.count (dwTid))
    task_me = task_map.at (dwTid);
  else
  {
    SK_MMCS_TaskEntry* new_entry =
      new SK_MMCS_TaskEntry {
        dwTid, 0, INVALID_HANDLE_VALUE, 0, AVRT_PRIORITY_NORMAL, "", "", ""
      };

    strncpy_s ( new_entry->name,       63,
                name,           _TRUNCATE );

    strncpy_s ( new_entry->task0,      63,
                task1,          _TRUNCATE );
    strncpy_s ( new_entry->task1,      63,
                task2,          _TRUNCATE );

    new_entry->hTask =
      AvSetMmMaxThreadCharacteristicsA ( task1, task2,
                                           &new_entry->dwTaskIdx );

    SK_TLS* pTLS =
      SK_TLS_Bottom ();

    if (pTLS != nullptr)
    {
      pTLS->scheduler->mmcs_task =
        new_entry;
    }

      task_map [dwTid] = new_entry;
    task_me =
      task_map [dwTid];
  }

  return
    task_me;
}

SK_MMCS_TaskEntry*
SK_MMCS_GetTaskForThreadID (DWORD dwTid, const char* name)
{
  return
    SK_MMCS_GetTaskForThreadIDEx ( dwTid, name,
                                     "Games",
                                     "Playback" );
}



DWORD
SK_GetRenderThreadID (void)
{
  static auto& rb =
    SK_GetCurrentRenderBackend ();

  return
    ReadULongAcquire (&rb.thread);
}