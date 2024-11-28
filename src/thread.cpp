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
#include <SpecialK/diagnostics/debug_utils.h>
#include <SpecialK/utility.h>

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"ThreadUtil"

///////////////////////////////////////////////////////////////////////////
//
// Thread Name Assignment for Meaningful Debug Identification
//
//  ** Necessary given the number of lambdas serving as thread functions
//      in this codebase and the truly useless name mangling that causes.
//
///////////////////////////////////////////////////////////////////////////
GetThreadDescription_pfn GetThreadDescription_Impl = nullptr;
SetThreadDescription_pfn SetThreadDescription_Impl = nullptr;

// Hooked function
SetThreadDescription_pfn SetThreadDescription_Original = nullptr;

static constexpr DWORD MAGIC_THREAD_EXCEPTION = 0x406D1388;

SK_LazyGlobal <concurrency::concurrent_unordered_map <DWORD, std::wstring>> _SK_ThreadNames;
SK_LazyGlobal <concurrency::concurrent_unordered_set <DWORD>>               _SK_SelfTitledThreads;
SK_LazyGlobal <concurrency::concurrent_unordered_set <DWORD>>               _SK_UntitledThreads;

void __make_self_titled (DWORD dwTid);

// Game has given this thread a custom name, it's special :)
bool
SK_Thread_HasCustomName (DWORD dwTid)
{
  auto& SelfTitled =
    _SK_SelfTitledThreads.get ();

  return ( SelfTitled.find (dwTid) !=
           SelfTitled.cend (     ) );
}

HRESULT
WINAPI
SK_SetThreadDescription ( _In_ HANDLE hThread,
                          _In_ PCWSTR lpThreadDescription )
{
  SK_RunOnce (   SetThreadDescription_Impl =
      (decltype (SetThreadDescription_Impl)) SK_GetProcAddress (L"kernel32",
                "SetThreadDescription")
  );

  if (SetThreadDescription_Impl != nullptr)
    return SetThreadDescription_Impl (hThread, lpThreadDescription);

  return
    E_NOTIMPL;
}

HRESULT
WINAPI
SK_GetThreadDescription ( _In_              HANDLE hThread,
                          _Outptr_result_z_ PWSTR* ppszThreadDescription )
{
  SK_RunOnce (   GetThreadDescription_Impl =
      (decltype (GetThreadDescription_Impl)) SK_GetProcAddress (L"kernel32",
                "GetThreadDescription")
  );

  if (GetThreadDescription_Impl != nullptr)
    return GetThreadDescription_Impl (hThread, ppszThreadDescription);

  return
    E_NOTIMPL;
}

static std::wstring _noname = L"";

std::wstring&
SK_Thread_QueryNameFromOS (DWORD dwTid)
{
  if (_SK_UntitledThreads->find (dwTid) == _SK_UntitledThreads->cend ())
  {
    SK_AutoHandle hThread (
               OpenThread ( THREAD_QUERY_LIMITED_INFORMATION,
                              FALSE,
                                dwTid ) );

    if (hThread.isValid ())
    {
      wchar_t                                          *wszThreadName = nullptr;
      if (SUCCEEDED (SK_GetThreadDescription (hThread, &wszThreadName)))
      {
        if ( wszThreadName != nullptr &&
            *wszThreadName != L'\0' ) // Empty strings are not useful :)
        {
          auto& names =
            _SK_ThreadNames.get ();

          __make_self_titled (dwTid);
                       names [dwTid] = wszThreadName;
      
          LocalFree (wszThreadName);

          return
            names [dwTid];
        }

        LocalFree (wszThreadName);
      }
    }

    _SK_UntitledThreads->insert (dwTid);
  }

  return
    _noname;
}

DWORD
SK_Thread_FindByName (std::wstring name)
{
  auto& threads =
    _SK_ThreadNames.get ();

  for (auto &thread : threads)
  {
    if (thread.second._Equal (name.c_str ()))
      return thread.first;
  }

  return 0;
}

std::wstring&
SK_Thread_GetName (DWORD dwTid)
{
  auto& names =
    _SK_ThreadNames.get ();

  const auto it  =
    names.find (dwTid);

  if (it != names.cend ())
    return (*it).second;

  return
    SK_Thread_QueryNameFromOS (dwTid);
}

std::wstring&
SK_Thread_GetName (HANDLE hThread)
{
  return
    SK_Thread_GetName (GetThreadId (hThread));
}

// Avoid SEH unwind problems
void
__make_self_titled (DWORD dwTid)
{
  auto&
    SelfTitled =
_SK_SelfTitledThreads.get ();

  SelfTitled.insert (dwTid);
}

using RtlRaiseException_pfn = void (WINAPI *)(_In_ PEXCEPTION_RECORD ExceptionRecord);
      RtlRaiseException_pfn
      RtlRaiseException_Original = nullptr;

bool
SK_Thread_SetWin10NameFromException (THREADNAME_INFO *pTni)
{
  bool bRet = false;

  if (pTni->szName != nullptr)
  {
    DWORD dwTid =
      ( pTni->dwThreadID == -1 ) ?
        SK_GetCurrentThreadId () :
             pTni->dwThreadID;

    SK_AutoHandle hThread (
               OpenThread ( THREAD_SET_LIMITED_INFORMATION,
                              FALSE,
                                dwTid ) );

    if (hThread.m_h != 0)
    {
      std::wstring wideDesc =
        SK_UTF8ToWideChar (pTni->szName);

      bRet =
        SUCCEEDED (
          SK_SetThreadDescription ( hThread.m_h,
              wideDesc.c_str ()   )
                  );

      if (bRet)
      {
        if (StrStrIW (wideDesc.c_str (), L"D3D Background Thread"))
        {
          // Running these threads at idle priority leads to optimal shaders
          //   never being compiled in CPU-heavy games.
          SetThreadPriority (hThread.m_h, THREAD_PRIORITY_NORMAL);
        }

        else if (!_wcsicmp (wideDesc.c_str (), L"GameThread"))
        {
          if (GetThreadPriority (hThread.m_h) < THREAD_PRIORITY_HIGHEST)
              SetThreadPriority (hThread.m_h,   THREAD_PRIORITY_HIGHEST);
        }

        else if (StrStrIW (wideDesc.c_str (), L"Foreground Worker #"))
        {
          if (GetThreadPriority (hThread.m_h) < THREAD_PRIORITY_ABOVE_NORMAL)
              SetThreadPriority (hThread.m_h,   THREAD_PRIORITY_ABOVE_NORMAL);
        }

        else if (!_wcsicmp (wideDesc.c_str (), L"IoService"))
        {
          if (GetThreadPriority (hThread.m_h) < THREAD_PRIORITY_HIGHEST)
              SetThreadPriority (hThread.m_h,   THREAD_PRIORITY_HIGHEST);
        }
      }
    }
  }

  return bRet;
}

void
SK_Thread_RaiseNameException (THREADNAME_INFO* pTni)
{
  if (SK_IsDebuggerPresent ())
  {
    __try
    {
      constexpr DWORD argc = sizeof (*pTni) /
                             sizeof (ULONG_PTR);

      SK_RaiseException ( MAGIC_THREAD_EXCEPTION,
                            SK_EXCEPTION_CONTINUABLE,
                              argc,
             (const ULONG_PTR *)pTni );
    }

    __except (EXCEPTION_EXECUTE_HANDLER) {
      // This is a continuable exception, but let's be safe
    }
  }

  SK_Thread_SetWin10NameFromException (pTni);
}


HRESULT
WINAPI
SetCurrentThreadDescription (_In_ PCWSTR lpThreadDescription)
{
  if (lpThreadDescription == nullptr)
    return E_POINTER;

  if (SK_GetHostAppUtil ()->isInjectionTool ())
    return S_OK;

#ifdef _DEBUG
  if (! ReadAcquire (&__SK_DLL_Attached))
  {
    return E_NOT_VALID_STATE;
  }
#endif

  size_t len;

  bool non_empty =
    SUCCEEDED ( StringCbLengthW (
                  lpThreadDescription, MAX_THREAD_NAME_LEN-1, &len
                )
              )                                             && len > 0;

  if (non_empty)
  {
    auto&
      ThreadNames =
        *_SK_ThreadNames;

    SK_TLS *pTLS =
      SK_TLS_Bottom ();

    DWORD               dwTid  = SK_Thread_GetCurrentId ();
    __make_self_titled (dwTid);
           ThreadNames [dwTid] = lpThreadDescription;

    if (pTLS != nullptr)
    {
      // Push this to the TLS data-store so we can get thread names even
      //   when no debugger is attached.
      wcsncpy_s (
        pTLS->debug.name,
          std::min (MAX_THREAD_NAME_LEN, (int)len+1),
            lpThreadDescription,
              _TRUNCATE
      );
    }

    char      szDesc                      [MAX_THREAD_NAME_LEN] = { };
    wcstombs (szDesc, lpThreadDescription, MAX_THREAD_NAME_LEN-1);

    THREADNAME_INFO info = {       };
    info.dwType          =      4096;
    info.szName          =    szDesc;
    info.dwThreadID      = (DWORD)-1;
    info.dwFlags         =       0x0;

    SK_Thread_RaiseNameException (&info);

    // Finally, use the new API added in Windows 10...
    return
      SK_SetThreadDescription ( GetCurrentThread (),
          lpThreadDescription );
  }

  return S_OK;
}

HRESULT
WINAPI
GetCurrentThreadDescription (_Out_  PWSTR  *threadDescription)
{
  SK_TLS* pTLS =      //= ReadAcquire (&__SK_DLL_Attached) ?
    SK_TLS_Bottom (); //: nullptr;

  // Always use the TLS value if there is one
  if ( pTLS != nullptr  &&
      *pTLS->debug.name != L'\0' )
  {
    // This is not freed here; the caller is expected to free it!
    *threadDescription =
      (wchar_t *)SK_LocalAlloc (LPTR, sizeof (wchar_t) * MAX_THREAD_NAME_LEN);

    wcsncpy_s (
      *threadDescription, MAX_THREAD_NAME_LEN-1,
        pTLS->debug.name, _TRUNCATE
    );

    return S_OK;
  }

  return
    SK_GetThreadDescription ( GetCurrentThread (),
                                threadDescription );
}

 using RtlEnterCriticalSection_pfn = NTSTATUS (WINAPI *)(PRTL_CRITICAL_SECTION);
       RtlEnterCriticalSection_pfn
       RtlEnterCriticalSection_Original = nullptr;

NTSTATUS
WINAPI
RtlEnterCriticalSection_Detour (
  PRTL_CRITICAL_SECTION crit
)
{
  // If we were to block here during DLL unload,
  //   Windows might terminate the softwre.
  if (ReadAcquire (&__SK_DLL_Ending))
    return STATUS_SUCCESS;

  return
    RtlEnterCriticalSection_Original (
      crit
    );
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
  if (lpCriticalSection == nullptr)
    return;

  InitializeCriticalSectionEx (
    lpCriticalSection, 0,
      RTL_CRITICAL_SECTION_FLAG_DYNAMIC_SPIN
  );
}

void
SK_HookCriticalSections (void)
{
  if (RtlEnterCriticalSection_Original == nullptr)
    SK_CreateDLLHook2 ( L"NtDll", "RtlEnterCriticalSection",
                                   RtlEnterCriticalSection_Detour,
          static_cast_p2p <void> (&RtlEnterCriticalSection_Original)
    );

  if (InitializeCriticalSection_Original == nullptr)
    SK_CreateDLLHook2 ( L"kernel32", "InitializeCriticalSection",
                                      InitializeCriticalSection_Detour,
             static_cast_p2p <void> (&InitializeCriticalSection_Original)
    );

  SK_RunOnce (SK_ApplyQueuedHooks ());
}

HRESULT
WINAPI
SetThreadDescription_Detour (HANDLE hThread, PCWSTR lpThreadDescription)
{
  SK_LOG_FIRST_CALL

  // Only do this if we have a hook on RaiseException
  extern RaiseException_pfn
         RaiseException_Original;
  if (   RaiseException_Original != nullptr)
  {
    SK_TLS *pTLS =
          SK_TLS_Bottom ();

    if (    pTLS != nullptr     &&
         (! pTLS->debug.naming) &&
         SK_ValidatePointer ((void *)lpThreadDescription, true)
       )
    {
      char      szDesc                      [MAX_THREAD_NAME_LEN] = { };
      wcstombs (szDesc, lpThreadDescription, MAX_THREAD_NAME_LEN-1);

      THREADNAME_INFO info = {       };
      info.dwType          =      4096;
      info.szName          =    szDesc;
      info.dwThreadID      = (DWORD)GetThreadId (hThread);
      info.dwFlags         =       0x0;

      constexpr DWORD argc = sizeof (info) /
                             sizeof (ULONG_PTR);

      pTLS->debug.naming = true;

      RaiseException ( MAGIC_THREAD_EXCEPTION,
                         SK_EXCEPTION_CONTINUABLE,
                           argc,
          (const ULONG_PTR *)&info );

      pTLS->debug.naming = false;
    }
  }

  return
    SetThreadDescription_Original (hThread, lpThreadDescription);
}

static volatile LONG _InitDebugExtrasOnce = FALSE;

DWORD
SK_Thread_GetMainId (void)
{
  static DWORD
           dwMainTid  = std::numeric_limits <DWORD>::max ();
  if (     dwMainTid != std::numeric_limits <DWORD>::max ())
    return dwMainTid;

  SK_AutoHandle hThreadSnapshot (
       CreateToolhelp32Snapshot (TH32CS_SNAPTHREAD, 0)
  );

  if ((intptr_t)hThreadSnapshot.m_h <= 0)
  {
    return 0;
  }

  THREADENTRY32
    tent        = {                    };
    tent.dwSize = sizeof (THREADENTRY32);

  DWORD result     = 0;
  DWORD dwCurrentPid = GetCurrentProcessId ();

  for ( BOOL success = Thread32First (hThreadSnapshot.m_h, &tent) ;
          (! result) &&
             success && GetLastError() != ERROR_NO_MORE_FILES     ;
             success = Thread32Next  (hThreadSnapshot.m_h, &tent) )
  {
    if (tent.th32OwnerProcessID == dwCurrentPid)
      result = tent.th32ThreadID;
  }

  return
    ( dwMainTid = result );
}

static SetThreadIdealProcessor_pfn
       SetThreadIdealProcessor_Original = nullptr;

DWORD
WINAPI
SK_SetThreadIdealProcessor (HANDLE hThread, DWORD dwIdealProcessor)
{
  if (SetThreadIdealProcessor_Original != nullptr)
    return SetThreadIdealProcessor_Original (hThread, dwIdealProcessor);

  return SetThreadIdealProcessor (hThread, dwIdealProcessor);
}

static SetThreadPriorityBoost_pfn
       SetThreadPriorityBoost_Original = nullptr;

BOOL
WINAPI
SK_SetThreadPriorityBoost ( HANDLE hThread,
                            BOOL   bDisableBoost )
{
  if (     SetThreadPriorityBoost_Original != nullptr)
    return SetThreadPriorityBoost_Original (hThread, bDisableBoost);

  return
    SetThreadPriorityBoost (hThread, bDisableBoost);
}

DWORD
WINAPI
SetThreadPriorityBoost_Detour (HANDLE hThread, BOOL bDisableBoost)
{
  SK_LOG_FIRST_CALL

  return
    SetThreadPriorityBoost_Original (hThread, bDisableBoost);
}

DWORD
WINAPI
SetThreadIdealProcessor_Detour (HANDLE hThread, DWORD dwIdealProcessor)
{
  SK_LOG_FIRST_CALL

  if (SK_GetCurrentGameID () == SK_GAME_ID::EldenRing)
  {
    return
      SetThreadIdealProcessor_Original (hThread, MAXIMUM_PROCESSORS);
  }

  return
    SetThreadIdealProcessor_Original (hThread, dwIdealProcessor);
}

static SetThreadAffinityMask_pfn
       SetThreadAffinityMask_Original = nullptr;

static SetThreadPriority_pfn
       SetThreadPriority_Original = nullptr;

BOOL
WINAPI
SK_SetThreadPriority ( HANDLE hThread,
                       int    nPriority )
{
  if (     SetThreadPriority_Original != nullptr)
    return SetThreadPriority_Original (hThread, nPriority);

  return
    SetThreadPriority (hThread, nPriority);
}

BOOL
WINAPI
SetThreadPriority_Detour ( HANDLE hThread,
                           int    nPriority )
{
  SK_LOG_FIRST_CALL

  if (hThread == nullptr)
  {
    SK_LOGi1 (
      L"Game called SetThreadPriority (...) with an invalid handle [%ws]",
        SK_GetCallerName ().c_str ()
    );

    SetLastError (ERROR_INVALID_HANDLE);

    return FALSE;
  }

  return
    SetThreadPriority_Original (
      hThread, nPriority
    );
}

DWORD_PTR
WINAPI
SetThreadAffinityMask_Detour (
  _In_ HANDLE    hThread,
  _In_ DWORD_PTR dwThreadAffinityMask )
{
  SK_LOG_FIRST_CALL

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

DWORD_PTR
WINAPI
SK_SetThreadAffinityMask (HANDLE hThread, DWORD_PTR mask)
{
  return
    ( SetThreadAffinityMask_Original != nullptr      ?
      SetThreadAffinityMask_Original (hThread, mask) :
      SetThreadAffinityMask          (hThread, mask) );
}

bool
SK_Thread_InitDebugExtras (void)
{
  if (! InterlockedCompareExchangeAcquire (&_InitDebugExtrasOnce, 1, 0))
  {
    // Hook QPC and Sleep
    SK_Scheduler_Init ();

    // Epic Online Services has code that doesn't work correctly if
    //   a spinlock is used
    if (config.threads.enable_dynamic_spinlocks)
    { // Extremely helpful in Chrono Cross, but potentially breaking in
      //   other games...
      SK_HookCriticalSections ();
    }

    // Only available in Windows 10
    //
    if (SetThreadDescription_Original == nullptr)
    {
      if (SK_GetProcAddress (L"kernel32", "SetThreadDescription") != nullptr)
      {
        SK_CreateDLLHook2 (L"kernel32", "SetThreadDescription",
                                         SetThreadDescription_Detour,
                static_cast_p2p <void> (&SetThreadDescription_Original)
        );
      }
    }

    SK_CreateDLLHook2 ( L"kernel32", "SetThreadPriority",
                                      SetThreadPriority_Detour,
             static_cast_p2p <void> (&SetThreadPriority_Original) );

    SK_CreateDLLHook2 ( L"kernel32", "SetThreadAffinityMask",
                                      SetThreadAffinityMask_Detour,
             static_cast_p2p <void> (&SetThreadAffinityMask_Original) );

    SK_CreateDLLHook2 ( L"kernel32", "SetThreadIdealProcessor",
                                      SetThreadIdealProcessor_Detour,
             static_cast_p2p <void> (&SetThreadIdealProcessor_Original) );

    SK_CreateDLLHook2 ( L"kernel32", "SetThreadPriorityBoost",
                                      SetThreadPriorityBoost_Detour,
             static_cast_p2p <void> (&SetThreadPriorityBoost_Original) );

    InterlockedIncrementRelease (&_InitDebugExtrasOnce);
  }

  else
    SK_Thread_SpinUntilAtomicMin (&_InitDebugExtrasOnce, 2);

  return
    (SetThreadDescription_Original != nullptr);
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


#define MAX_THREAD_NAME_LEN MAX_PATH

struct SK_ThreadBaseParams {
  LPTHREAD_START_ROUTINE lpStartFunc                        = nullptr;
  LPVOID                 lpUserParams                       = nullptr;
  HANDLE                 hHandleToStuffInternally           = nullptr;
  wchar_t                lpThreadName [MAX_THREAD_NAME_LEN] = {     };
};

DWORD
WINAPI
SKX_ThreadThunk ( LPVOID lpUserPassThrough )
{
  SK_ThreadBaseParams *pStartParams =
    static_cast <SK_ThreadBaseParams *> (lpUserPassThrough);

  SK_TLS *pTLS =
    SK_TLS_Bottom ();

  if (pTLS != nullptr)
  {
    pTLS->debug.handle.Attach (pStartParams->hHandleToStuffInternally);
    pTLS->debug.tid =
      SK_Thread_GetCurrentId ();

    wcsncpy_s ( pTLS->debug.name, MAX_THREAD_NAME_LEN,
      pStartParams->lpThreadName,           _TRUNCATE );

#ifdef _DEBUG
    SK_ReleaseAssert (
      sk::narrow_cast    <DWORD    > (
        reinterpret_cast <DWORD_PTR> (
          SK_Thread_GetTEB_FAST ()->Cid.UniqueThread
        ) & 0x00000000FFFFFFFFULL
      ) == GetCurrentThreadId ()
    );
#endif
  }

  // We have hooks in place, this is a thread of interest to us.
  if (pTLS != nullptr)
  {
    if (*pStartParams->lpThreadName != L'\0')
      SetCurrentThreadDescription (pStartParams->lpThreadName);

    // Kick-off data collection on thread start
    extern void SK_Widget_InvokeThreadProfiler (void);
                SK_Widget_InvokeThreadProfiler (    );
  }

  DWORD dwRet =
    pStartParams->lpStartFunc (pStartParams->lpUserParams);

  if (pTLS == nullptr) // We cannot rely on the caller to free this handle
    SK_CloseHandle (pStartParams->hHandleToStuffInternally);

  SK_LocalFree ((HLOCAL)pStartParams);

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

  params->lpStartFunc              = lpStartFunc;
  params->lpUserParams             = lpUserParams;
  params->hHandleToStuffInternally = INVALID_HANDLE_VALUE;

  if (lpThreadName != nullptr)
  {
    wcsncpy_s ( params->lpThreadName,  MAX_THREAD_NAME_LEN,
                        lpThreadName, _TRUNCATE );
  }

  unsigned int dwTid = 0;

  HANDLE hRet =
    reinterpret_cast <HANDLE> (
      _beginthreadex ( nullptr, 0,
               (_beginthreadex_proc_type)SKX_ThreadThunk,
                 (LPVOID)params,
                   CREATE_SUSPENDED, &dwTid )
    );

  params->hHandleToStuffInternally = hRet;

  if (hRet != 0)
    ResumeThread (hRet);

  return
    hRet;
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
  SK_TLS* pTLS      =
    ReadAcquire (&__SK_DLL_Attached) ?
                    SK_TLS_Bottom () : nullptr;

  if (pTLS != nullptr)
  {
    HANDLE hCopyAndSwapHandle =
      INVALID_HANDLE_VALUE;

    if (pTLS->debug.handle.isValid ())
    { std::swap  (pTLS->debug.handle.m_h, hCopyAndSwapHandle);
      if (! SK_CloseHandle (              hCopyAndSwapHandle)) {
        std::swap(pTLS->debug.handle.m_h, hCopyAndSwapHandle); }
      else
        return true;
    }
  }

  return false;
}

using AvSetMmMaxThreadCharacteristicsA_pfn =
  HANDLE (WINAPI *)(LPCSTR, LPCSTR, LPDWORD);
using AvSetMmThreadPriority_pfn =
  BOOL   (WINAPI *)(HANDLE, AVRT_PRIORITY);
using AvRevertMmThreadCharacteristics_pfn =
  BOOL   (WINAPI *)(HANDLE);

static AvSetMmMaxThreadCharacteristicsA_pfn
      _AvSetMmMaxThreadCharacteristicsA = nullptr;
static AvSetMmThreadPriority_pfn
      _AvSetMmThreadPriority            = nullptr;
static AvRevertMmThreadCharacteristics_pfn
      _AvRevertMmThreadCharacteristics  = nullptr;

static HMODULE hModAVRT  = nullptr;
static bool    bAVRTInit = false;

HMODULE
SK_AVRT_LoadLibrary (void)
{
  if (!            hModAVRT)
                   hModAVRT =
    SK_LoadLibraryW (L"AVRT.dll");

  return
    hModAVRT;
}

void SK_AVRT_Init (void)
{
  if (bAVRTInit)
    return;

  if (! _AvSetMmMaxThreadCharacteristicsA)
  {     _AvSetMmMaxThreadCharacteristicsA =
        (AvSetMmMaxThreadCharacteristicsA_pfn)SK_GetProcAddress (
                                              SK_AVRT_LoadLibrary (),
        "AvSetMmMaxThreadCharacteristicsA" );
  }

  if (! _AvSetMmThreadPriority)
  {     _AvSetMmThreadPriority =
        (AvSetMmThreadPriority_pfn)SK_GetProcAddress (
                                   SK_AVRT_LoadLibrary (),
        "AvSetMmThreadPriority" );
  }

  if (! _AvRevertMmThreadCharacteristics)
  {     _AvRevertMmThreadCharacteristics =
        (AvRevertMmThreadCharacteristics_pfn)SK_GetProcAddress (
                                             SK_AVRT_LoadLibrary (),
        "AvRevertMmThreadCharacteristics" );
  }

  bAVRTInit = true;
}

_Success_(return != NULL)
HANDLE
WINAPI
SK_AvSetMmMaxThreadCharacteristicsA (
  _In_    LPCSTR  FirstTask,
  _In_    LPCSTR  SecondTask,
  _Inout_ LPDWORD TaskIndex )
{
  SK_AVRT_Init ();

  return
    _AvSetMmMaxThreadCharacteristicsA (
      FirstTask, SecondTask, TaskIndex
    );
}

_Success_(return != FALSE)
BOOL
WINAPI
SK_AvSetMmThreadPriority (
  _In_ HANDLE        AvrtHandle,
  _In_ AVRT_PRIORITY Priority )
{
  SK_AVRT_Init ();

  return
    _AvSetMmThreadPriority (AvrtHandle, Priority);
}

_Success_(return != FALSE)
BOOL
WINAPI
SK_AvRevertMmThreadCharacteristics (
  _In_ HANDLE AvrtHandle )
{
  SK_AVRT_Init ();

  using AvRevertMmThreadCharacteristics_pfn =
    BOOL (WINAPI *)(HANDLE);

  return
    _AvRevertMmThreadCharacteristics (AvrtHandle);
}


using _SK_MMCS_TaskMap =
        concurrency::concurrent_unordered_map <
          DWORD, SK_MMCS_TaskEntry*
        >;

SK_LazyGlobal <
      _SK_MMCS_TaskMap
>             _task_map;

__declspec (noinline)
_SK_MMCS_TaskMap&
SK_MMCS_GetTaskMap (void)
{
  return
    _task_map.get ();
}

size_t
SK_MMCS_GetTaskCount (void)
{
  return
    _task_map->size ();
}

std::vector <SK_MMCS_TaskEntry *>
SK_MMCS_GetTasks (void)
{
  auto& task_map =
    SK_MMCS_GetTaskMap ();

  std::vector <SK_MMCS_TaskEntry *> tasks;

  std::transform ( task_map.cbegin (),
                   task_map.cend   (),
                     std::back_inserter (tasks),
                     []( std::pair < DWORD,
                                     SK_MMCS_TaskEntry *> c) ->
                             auto {                if (c.second->dwTid != 0)
                                                return c.second;
                                           else return (SK_MMCS_TaskEntry *)nullptr;
                                  }
                 );

  return
    tasks;
}

bool
SK_MMCS_RemoveTask (DWORD dwTid)
{
  auto& task_map =
    SK_MMCS_GetTaskMap ();

  if (task_map.count (dwTid) != 0)
  {
    task_map [dwTid]->dwTid = 0;

    return true;
  }

  return false;
}

SK_MMCS_TaskEntry*
SK_MMCS_GetTaskForThreadIDEx ( DWORD dwTid, const char* name,
                                            const char* task1,
                                            const char* task2 )
{
  auto& task_map =
    SK_MMCS_GetTaskMap ();

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
      SK_AvSetMmMaxThreadCharacteristicsA ( task1, task2,
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
  const SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  return
    ReadULongAcquire (&rb.thread);
}

DWORD
SK_GetMainThreadID (void)
{
  static DWORD tid = 0;

  if (tid != 0)
    return tid;

  SK_AutoHandle hThreadSnapshot (
    CreateToolhelp32Snapshot (TH32CS_SNAPTHREAD, 0)
  );

  if (! hThreadSnapshot.isValid ())
  {
    return
      static_cast <DWORD> (-1);
  }

  THREADENTRY32
    tent        = { };
    tent.dwSize = sizeof (THREADENTRY32);

  DWORD pid = GetCurrentProcessId ();

  if (Thread32First (hThreadSnapshot, &tent))
  {
    while (! tid)
    {
      if (! Thread32Next (hThreadSnapshot, &tent))
        break;

      if (GetLastError () == ERROR_NO_MORE_FILES)
        break;

      if (tent.th32OwnerProcessID == pid)
        tid = tent.th32ThreadID;
    }
  }

  if (tid == 0)
    return static_cast <DWORD> (-1);

  return tid;
}


void
SK_Thread_SpinUntilFlaggedEx ( _In_ _Interlocked_operand_ LONG volatile const *pFlag,
                                                          LONG                 _TimeoutMs,
                                                          LONG                 _SpinMax ) noexcept
{
  DWORD dwStartTime =
    SK_timeGetTime ();

  while (ReadAcquire (pFlag) == 0)
  {
    for (int i = 0; i < _SpinMax && (ReadAcquire (pFlag) == 0); i++)
      YieldProcessor ();

    if (ReadAcquire (pFlag) == 1)
      break;

    SK_SleepEx (0UL, FALSE);

    if (SK_timeGetTime () > dwStartTime + _TimeoutMs)
      return;
  }
}


volatile LONG __SK_MMCS_PendingChanges = 0;