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

#ifndef __SK__THREAD_H__
#define __SK__THREAD_H__

#define _PROCESSTHREADSAPI_H_

#include <Windows.h>
#include <avrt.h>
#include <intsafe.h>
#include <string>

namespace sk
{
  template <typename T, typename Q>
    constexpr
    T
      narrow_cast (Q&& q) noexcept
      {
        return static_cast <T> (
                 std::forward <Q> (q)
                               );
      };
};

template < typename T, typename Q    > constexpr
 T  (*narrow_cast)(             Q&& q) noexcept
=&sk::narrow_cast < T,          Q    >;

template <typename T, typename T2, typename Q>
  constexpr
  T
    static_const_cast ( const typename Q q )
    {
      return static_cast <T>  (
               const_cast <T2>  ( q )
                              );
    };

template <typename T, typename Q>
  constexpr
  T**
    static_cast_p2p (     Q **      p2p ) noexcept
    {
      return static_cast <T **> (
               static_cast <T*>   ( p2p )
                                );
    };

  template <typename T, typename Q>
  constexpr
  T
    static_cast_pfn (Q* p2p) noexcept
    {
      return static_cast <T> (p2p);
    };

constexpr static DWORD SK_WINNT_THREAD_NAME_EXCEPTION = 0x406D1388;

#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
  DWORD  dwType;     // Always 4096
  LPCSTR szName;     // Pointer to name (in user addr space).
  DWORD  dwThreadID; // Thread ID (-1=caller thread).
  DWORD  dwFlags;    // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

#define RTL_CRITICAL_SECTION_FLAG_NO_DEBUG_INFO         0x01000000
#define RTL_CRITICAL_SECTION_FLAG_DYNAMIC_SPIN          0x02000000
#define RTL_CRITICAL_SECTION_FLAG_STATIC_INIT           0x04000000
#define RTL_CRITICAL_SECTION_FLAG_RESOURCE_TYPE         0x08000000
#define RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO      0x10000000
#define RTL_CRITICAL_SECTION_ALL_FLAG_BITS              0xFF000000

#define SK_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO       0x0
//0x10000000

#define RTL_CRITICAL_SECTION_FLAG_RESERVED              (RTL_CRITICAL_SECTION_ALL_FLAG_BITS & (~(RTL_CRITICAL_SECTION_FLAG_NO_DEBUG_INFO | RTL_CRITICAL_SECTION_FLAG_DYNAMIC_SPIN | RTL_CRITICAL_SECTION_FLAG_STATIC_INIT | RTL_CRITICAL_SECTION_FLAG_RESOURCE_TYPE | RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO)))


static inline
constexpr HANDLE
  SK_GetCurrentThread (void) noexcept
  {
    return (HANDLE)-2;
  };

static inline
constexpr HANDLE
  SK_GetCurrentProcess (void) noexcept
  {
    return (HANDLE)-1;
  };



class SK_Thread_ScopedPriority
{
public:
  SK_Thread_ScopedPriority (int prio) noexcept : hThread (GetCurrentThread ())
  {
    orig_prio = GetThreadPriority (hThread);
                SetThreadPriority (hThread, prio);
  }

 ~SK_Thread_ScopedPriority (void) noexcept
  {
    SetThreadPriority (hThread, orig_prio);
  }

protected:
  int    orig_prio;
  HANDLE hThread;
};


class SK_Thread_CriticalSection //: public std::recursive_mutex
{
public:
  SK_Thread_CriticalSection ( CRITICAL_SECTION* pCS ) noexcept
  {
    cs_ = pCS;
  };

  ~SK_Thread_CriticalSection (void) noexcept = default;

  _Acquires_exclusive_lock_ (cs_)
  void lock (void) noexcept {
    EnterCriticalSection (cs_);
  }

  _Releases_exclusive_lock_ (cs_)
  void unlock (void) noexcept
  {
    LeaveCriticalSection (cs_);
  }

  _When_ (return != false, _Acquires_exclusive_lock_ (cs_))
  bool try_lock (void) noexcept
  {
    return
      TryEnterCriticalSection (cs_) == TRUE;
  }

protected:
  CRITICAL_SECTION* cs_;
};

class SK_Thread_HybridSpinlock : public SK_Thread_CriticalSection
{
public:
  SK_Thread_HybridSpinlock (int spin_count = 3000) noexcept : SK_Thread_CriticalSection (&impl_cs_)
  {
    InitializeCriticalSectionEx (cs_, spin_count, RTL_CRITICAL_SECTION_FLAG_DYNAMIC_SPIN |
                                                   SK_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO );
  }

  ~SK_Thread_HybridSpinlock (void) noexcept
  {
    DeleteCriticalSection (cs_);
                           cs_ = nullptr;
  }

private:
  CRITICAL_SECTION impl_cs_ = { };
};


DWORD
WINAPI
SK_SleepEx (DWORD dwMilliseconds, BOOL bAlertable) throw ();

DWORD
WINAPI
SK_WaitForSingleObject ( _In_ HANDLE hHandle,
                         _In_ DWORD  dwMilliseconds );

// The underlying Nt kernel wait function has usec granularity,
//   this is a cheat that satisfies WAIT_OBJECT_n semantics rather
//     than the complicated NTSTATUS stuff going on under the hood.
DWORD
WINAPI
SK_WaitForSingleObject_Micro ( _In_ HANDLE         hHandle,
                               _In_ PLARGE_INTEGER liMicroseconds );


static void
SK_Thread_SpinUntilFlagged ( _In_ _Interlocked_operand_ LONG volatile const *pFlag,
                                                        LONG                 _SpinMax = 75L ) noexcept
{
  while (ReadAcquire (pFlag) == 0)
  {
    for (int i = 0; i < _SpinMax && (ReadAcquire (pFlag) == 0); i++)
      YieldProcessor ();

    if (ReadAcquire (pFlag) == 1)
      break;

    SK_SleepEx (0UL, FALSE);
  }
}

void
SK_Thread_SpinUntilFlaggedEx ( _In_ _Interlocked_operand_ LONG volatile const *pFlag,
                                                          LONG                 _TimeoutMs = 250UL,
                                                          LONG                 _SpinMax   = 75L ) noexcept;

static void
SK_Thread_SpinUntilAtomicMin ( _In_ _Interlocked_operand_ LONG volatile const *pVar,
                                                          LONG                 count,
                                                          LONG                 _SpinMax = 75L ) noexcept
{
  while (ReadAcquire (pVar) < count)
  {
    for (int i = 0; i < _SpinMax && (ReadAcquire (pVar) < count); i++)
      YieldProcessor ();

    if (ReadAcquire (pVar) >= count)
      break;

    SK_SleepEx (0UL, FALSE);
  }
}


extern "C" HANDLE WINAPI
SK_Thread_CreateEx ( LPTHREAD_START_ROUTINE lpStartFunc,
                     LPCWSTR                lpThreadName = nullptr,
                     LPVOID                 lpUserParams = nullptr );

extern "C" void WINAPI
SK_Thread_Create   ( LPTHREAD_START_ROUTINE lpStartFunc,
                     LPVOID                 lpUserParams = nullptr );

extern "C" bool WINAPI
SK_Thread_CloseSelf (void);



// Returns TRUE if the call required a change to priority level
extern "C" BOOL WINAPI SK_Thread_SetCurrentPriority (int prio);
extern "C" int  WINAPI SK_Thread_GetCurrentPriority (void);

typedef HRESULT (WINAPI *SetThreadDescription_pfn)(
  _In_ HANDLE hThread,
  _In_ PCWSTR lpThreadDescription
);

typedef HRESULT (WINAPI *GetThreadDescription_pfn)(
  _In_              HANDLE hThread,
  _Outptr_result_z_ PWSTR* ppszThreadDescription
);

typedef DWORD_PTR (WINAPI *SetThreadAffinityMask_pfn)(
  _In_ HANDLE    hThread,
  _In_ DWORD_PTR dwThreadAffinityMask
);

using SetThreadIdealProcessor_pfn = DWORD (WINAPI *)(HANDLE,DWORD);
using SetThreadPriorityBoost_pfn  = BOOL  (WINAPI *)(HANDLE, BOOL);
using SetThreadPriority_pfn       = BOOL  (WINAPI *)(HANDLE, int);

extern "C" HRESULT WINAPI SK_SetThreadDescription (HANDLE, PCWSTR);
extern "C" HRESULT WINAPI SK_GetThreadDescription (HANDLE, PWSTR*);

extern "C" HRESULT WINAPI SetCurrentThreadDescription (_In_  PCWSTR lpThreadDescription);
extern "C" HRESULT WINAPI GetCurrentThreadDescription (_Out_  PWSTR  *threadDescription);

DWORD_PTR WINAPI
SK_SetThreadAffinityMask (HANDLE hThread, DWORD_PTR mask);

extern "C" bool SK_Thread_InitDebugExtras (void);

HMODULE SK_AVRT_LoadLibrary (void);

_Success_(return != NULL)
HANDLE
WINAPI
SK_AvSetMmMaxThreadCharacteristicsA (
  _In_    LPCSTR  FirstTask,
  _In_    LPCSTR  SecondTask,
  _Inout_ LPDWORD TaskIndex
);

_Success_(return != FALSE)
BOOL
WINAPI
SK_AvSetMmThreadPriority (
  _In_ HANDLE        AvrtHandle,
  _In_ AVRT_PRIORITY Priority
);

_Success_(return != FALSE)
BOOL
WINAPI
SK_AvRevertMmThreadCharacteristics (
  _In_ HANDLE AvrtHandle
);

extern DWORD SK_GetRenderThreadID (void);
extern DWORD SK_GetMainThreadID   (void);

extern volatile LONG __SK_MMCS_PendingChanges;

struct SK_MMCS_TaskEntry {
  DWORD         dwTid      = 0;
  DWORD         dwTaskIdx  = 0;                    // MMCSS Task Idx
  HANDLE        hTask      = INVALID_HANDLE_VALUE; // MMCSS Priority Boost Handle
  DWORD         dwFrames   = 0;
  AVRT_PRIORITY priority   = AVRT_PRIORITY_NORMAL;
  char          name  [64] = { };
  char          task0 [64] = { };
  char          task1 [64] = { };

  struct {
    volatile LONG pending    = FALSE;
    AVRT_PRIORITY priority   = AVRT_PRIORITY_NORMAL;
    char          task0 [64] = { };
    char          task1 [64] = { };

    void flush (void) noexcept {
      if (! InterlockedExchange (&pending, TRUE))
           InterlockedIncrement (&__SK_MMCS_PendingChanges);
    }
  } change;

  void queuePriority (AVRT_PRIORITY prio) noexcept
  {
    change.priority =
      prio;

    // Will cause this to take affect the next time
    //   the thread is rescheduled. AVRT priority can
    //     only be changed for a thread by running the
    //       code to change priority _on_ that thread.
    change.flush ();
  }

  BOOL          setPriority (AVRT_PRIORITY prio)
  {
    if (SK_AvSetMmThreadPriority (hTask, prio))
    {
      priority = prio;

      change.priority =
                 prio;

      return TRUE;
    }

    return FALSE;
  }

  HANDLE        setMaxCharacteristics ( const char* first_task,
                                        const char* second_task )
  {
    HANDLE hTask_ =
      SK_AvSetMmMaxThreadCharacteristicsA (first_task, second_task, &dwTaskIdx);

    if (hTask_ != nullptr)
    {
      strncpy_s (task0, 63, first_task,  _TRUNCATE);
      strncpy_s (task1, 63, second_task, _TRUNCATE);

      hTask = hTask_;
    }

    else
    {
      if ( (intptr_t)hTask > 0)
                     hTask    = INVALID_HANDLE_VALUE;
                    dwTaskIdx = 0;
    }

    return hTask;
  }

  BOOL disassociateWithTask (void)
  {
    BOOL bRet = FALSE;

    if ((intptr_t)hTask > 0)
    {
      bRet =
        SK_AvRevertMmThreadCharacteristics (hTask);

      hTask = INVALID_HANDLE_VALUE;
    }

    return bRet;
  }

  HANDLE reassociateWithTask (void)
  {
    HANDLE hTask_ =
      SK_AvSetMmMaxThreadCharacteristicsA (task0, task1, &dwTaskIdx);

    if (hTask_ != nullptr)
    {
      SK_AvSetMmThreadPriority (hTask_, priority);

      hTask = hTask_;
    }

    return hTask;
  }
};

SK_MMCS_TaskEntry&     __SK_MMCS_GetNulTaskRef (void);
#define nul_task_ref() __SK_MMCS_GetNulTaskRef ()

size_t
SK_MMCS_GetTaskCount (void);

SK_MMCS_TaskEntry*
SK_MMCS_GetTaskForThreadIDEx ( DWORD dwTid,       const char* name,
                               const char* task1, const char* task2 );
SK_MMCS_TaskEntry*
SK_MMCS_GetTaskForThreadID   ( DWORD dwTid,        const char* name );


#include <winnt.h>

static
__forceinline
DWORD
SK_GetCurrentThreadId (void)
{
  return reinterpret_cast <DWORD *> (NtCurrentTeb ()) [
#ifdef _M_AMD64
    18
#elif _M_IX86
     9
#endif
  ];
}

class SK_TLS;

static
__forceinline
SK_TLS*
SK_FAST_GetTLS (void)
{
  extern volatile DWORD __SK_TLS_INDEX;

  DWORD tls_idx =
    ReadULongAcquire (&__SK_TLS_INDEX);

  if (tls_idx == TLS_OUT_OF_INDEXES)
    return nullptr;

  return *(SK_TLS **)((uintptr_t *)NtCurrentTeb ()) [
#ifdef _M_AMD64
    0x290 + tls_idx
#elif _M_IX86
    0x384 + tls_idx
#else
    static_assert (false, "Unsupported platform");
#endif
  ];
}


#pragma pack (push,8)
typedef struct _HANDLEENTRY_SK {
  PVOID            phead;
  PVOID            pOwner;
  BYTE             bType;
  BYTE             bFlags;
  WORD             wUniq;
} HANDLEENTRY_SK,
*PHANDLEENTRY_SK;

typedef struct _SERVERINFO_SK {
  WORD             wRIPFlags;
  WORD             wSRVIFlags;
  WORD             wRIPPID;
  WORD             wRIPError;
  ULONG            cHandleEntries;
} SERVERINFO_SK,
*PSERVERINFO_SK;

typedef struct _SHAREDINFO_SK {
  PSERVERINFO_SK   psi;
  PHANDLEENTRY_SK  aheList;
  ULONG            HeEntrySize;
} SHAREDINFO_SK,
*PSHAREDINFO_SK;

typedef struct _PEB_SK {
  BYTE             Reserved1     [  2];
  BYTE             BeingDebugged;
  BYTE             Reserved2     [229];
  PVOID            Reserved3     [ 59];
  ULONG            SessionId;
} PEB_SK,
 *PPEB_SK;

typedef struct _CLIENT_ID_SK
{
  HANDLE           UniqueProcess;
  HANDLE           UniqueThread;
} CLIENT_ID_SK,
*PCLIENT_ID_SK;

typedef struct _NT_TIB_SK {
 struct _EXCEPTION_REGISTRATION_RECORD* ExceptionList;
         PVOID                          StackBase;
         PVOID                          StackLimit;
         PVOID                          SubSystemTib;
       union {
         PVOID                          FiberData;
         DWORD                          Version;
       };
         PVOID                          ArbitraryUserPointer;
 struct _NT_TIB* Self;
} NT_TIB_SK, *PNT_TIB_SK;

#ifndef SK_UNICODE_STR
#define SK_UNICODE_STR
typedef struct _UNICODE_STRING_SK {
  USHORT           Length;
  USHORT           MaximumLength;
  PWSTR            Buffer;
} UNICODE_STRING_SK;

typedef _UNICODE_STRING_SK* PUNICODE_STRING_SK;
#endif

typedef _Return_type_success_(return >= 0) LONG NTSTATUS;

typedef struct _TEB_SK {
  NT_TIB_SK         Tib;
  PVOID             EnvironmentPointer;
  CLIENT_ID_SK      Cid;
  PVOID             ActiveRpcInfo;
  PVOID             ThreadLocalStoragePointer;
  PPEB_SK           Peb;
  ULONG             LastErrorValue;
  ULONG             CountOfOwnedCriticalSections;
  PVOID             CsrClientThread;
  PVOID             Win32ThreadInfo;
  ULONG             Win32ClientInfo               [0x1F];
  PVOID             WOW32Reserved;
  ULONG             CurrentLocale;
  ULONG             FpSoftwareStatusRegister;
  PVOID             SystemReserved1               [0x36];
  PVOID             Spare1;
  ULONG             ExceptionCode;
  ULONG             SpareBytes1                   [0x28];
  PVOID             SystemReserved2               [0xA];
  ULONG             GdiRgn;
  ULONG             GdiPen;
  ULONG             GdiBrush;
  CLIENT_ID_SK      RealClientId;
  PVOID             GdiCachedProcessHandle;
  ULONG             GdiClientPID;
  ULONG             GdiClientTID;
  PVOID             GdiThreadLocaleInfo;
  PVOID             UserReserved                  [5];
  PVOID             GlDispatchTable               [0x118];
  ULONG             GlReserved1                   [0x1A];
  PVOID             GlReserved2;
  PVOID             GlSectionInfo;
  PVOID             GlSection;
  PVOID             GlTable;
  PVOID             GlCurrentRC;
  PVOID             GlContext;
  NTSTATUS          LastStatusValue;
  UNICODE_STRING_SK StaticUnicodeString;
  WCHAR             StaticUnicodeBuffer           [0x105];
  PVOID             DeallocationStack;
  PVOID             TlsSlots                      [0x40];
  LIST_ENTRY        TlsLinks;
  PVOID             Vdm;
  PVOID             ReservedForNtRpc;
  PVOID             DbgSsReserved                 [0x2];
  ULONG             HardErrorDisabled;
  PVOID             Instrumentation               [0x10];
  PVOID             WinSockData;
  ULONG             GdiBatchCount;
  ULONG             Spare2;
  ULONG             Spare3;
  ULONG             Spare4;
  PVOID             ReservedForOle;
  ULONG             WaitingOnLoaderLock;
  PVOID             StackCommit;
  PVOID             StackCommitMax;
  PVOID             StackReserved;
} TEB_SK,
*PTEB_SK;
#pragma pack (pop)

__forceinline
TEB_SK*
SK_Thread_GetTEB_FAST (void)
{
#ifdef _UNSTABLE
#ifdef _M_IX86
  return
    reinterpret_cast <TEB_SK *> (__readfsdword (0x18));
#elif _M_AMD64
  return
    reinterpret_cast <TEB_SK *> (__readgsqword (0x30));
#endif
#else
  return
    reinterpret_cast <TEB_SK *> (NtCurrentTeb ());
#endif
}

__forceinline
DWORD
SK_Thread_GetCurrentId (void)
{
  return
    sk::narrow_cast    <DWORD    > (
      reinterpret_cast <DWORD_PTR> (
        SK_Thread_GetTEB_FAST ()->Cid.UniqueThread
      ) & 0x00000000FFFFFFFFULL
    );
}

DWORD SK_Thread_GetMainId (void);

void SK_Thread_RaiseNameException (THREADNAME_INFO* pTni);


enum class WaitReason
{
  Executive,
  FreePage,
  PageIn,
  PoolAllocation,
  DelayExecution,
  Suspended,
  UserRequest,
  WrExecutive,
  WrFreePage,
  WrPageIn,
  WrPoolAllocation,
  WrDelayExecution,
  WrSuspended,
  WrUserRequest,
  WrEventPair,
  WrQueue,
  WrLpcReceive,
  WrLpcReply,
  WrVirtualMemory,
  WrPageOut,
  WrRendezvous,
  Spare2,
  Spare3,
  Spare4,
  Spare5,
  Spare6,
  WrKernel,
  MaximumWaitReason,
  NotWaiting = MaximumWaitReason
};

enum class ThreadState
{
  //Aborted          = 256,
  //AbortRequested   = 128,
  //Background       = 4,
  //Running          = 0,
  //Stopped          = 16,
  //StopRequested    = 1,
  //Suspended        = 64,
  //SuspendRequested = 2,
  //Unstarted        = 8,
  //WaitSleepJoin    = 32
  Running = 2,
  Waiting = 5
};

struct SKWG_Thread_Entry
{
  HANDLE hThread = nullptr;
  DWORD  dwTid   = 0UL;

  struct runtimes_s
  {
    FILETIME created = { };
    FILETIME exited  = { };
    FILETIME user    = { };
    FILETIME kernel  = { };

    long double percent_user   = 0.0L;
    long double percent_kernel = 0.0L;

    struct
    {
      FILETIME user   = { };
      FILETIME kernel = { };
    } snapshot;
  } runtimes;

  // Last time percentage was non-zero; used to hide inactive threads
  DWORD last_nonzero       = 0;
  bool  exited             = false;

  bool  power_throttle     = false;
  DWORD orig_prio          = DWORD_MAX;

  WaitReason  wait_reason  = WaitReason::NotWaiting;
  ThreadState thread_state = ThreadState::Running;

  bool         self_titled = false;
  std::wstring name        = L"";
};

DWORD WINAPI
SK_DelayExecution (double dMilliseconds, BOOL bAlertable) noexcept;

BOOL
WINAPI
SK_SetProcessAffinityMask (HANDLE hProcess, DWORD_PTR dwProcessAffinityMask);

void SK_Widget_InvokeThreadProfiler (void);
void SK_ImGui_RebalanceThreadButton (void);
extern float __SK_Thread_RebalanceEveryNSeconds;

#endif /* __SK__THREAD_H__ */
