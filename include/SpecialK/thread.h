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


 
#define RTL_CRITICAL_SECTION_FLAG_NO_DEBUG_INFO         0x01000000
#define RTL_CRITICAL_SECTION_FLAG_DYNAMIC_SPIN          0x02000000
#define RTL_CRITICAL_SECTION_FLAG_STATIC_INIT           0x04000000
#define RTL_CRITICAL_SECTION_FLAG_RESOURCE_TYPE         0x08000000
#define RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO      0x10000000
#define RTL_CRITICAL_SECTION_ALL_FLAG_BITS              0xFF000000

#define SK_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO       0x0
//0x10000000

#define RTL_CRITICAL_SECTION_FLAG_RESERVED              (RTL_CRITICAL_SECTION_ALL_FLAG_BITS & (~(RTL_CRITICAL_SECTION_FLAG_NO_DEBUG_INFO | RTL_CRITICAL_SECTION_FLAG_DYNAMIC_SPIN | RTL_CRITICAL_SECTION_FLAG_STATIC_INIT | RTL_CRITICAL_SECTION_FLAG_RESOURCE_TYPE | RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO))) 


static inline constexpr
  const HANDLE
    SK_GetCurrentThread  (void) { return (PVOID)-2; };

static inline constexpr
  const HANDLE
    SK_GetCurrentProcess (void) { return (PVOID)-1; };



class SK_Thread_ScopedPriority
{
public:
  SK_Thread_ScopedPriority (int prio) : hThread (GetCurrentThread ())
  {
    orig_prio = GetThreadPriority (hThread);
                SetThreadPriority (hThread, prio);
  }

 ~SK_Thread_ScopedPriority (void) ///
  {
    SetThreadPriority (hThread, orig_prio);
  }

protected:
  int    orig_prio;
  HANDLE hThread;
};


class SK_Thread_CriticalSection
{
public:
  SK_Thread_CriticalSection ( CRITICAL_SECTION* pCS )
  {
    cs_ = pCS;
  };

  ~SK_Thread_CriticalSection (void) = default;

  void lock (void) {
    EnterCriticalSection (cs_);
  }

  void unlock (void)
  {
    LeaveCriticalSection (cs_);
  }

  bool try_lock (void)
  {
    return TryEnterCriticalSection (cs_);
  }

protected:
  CRITICAL_SECTION* cs_;
};

class SK_Thread_HybridSpinlock : public SK_Thread_CriticalSection
{
public:
  SK_Thread_HybridSpinlock (int spin_count = 3000) :
                                                     SK_Thread_CriticalSection (new CRITICAL_SECTION)
  {
    InitializeCriticalSectionEx (cs_, spin_count, SK_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO | RTL_CRITICAL_SECTION_FLAG_DYNAMIC_SPIN);
  }

  ~SK_Thread_HybridSpinlock (void) ///
  {
    DeleteCriticalSection (cs_);
    delete cs_;
  }
};

class SK_AutoCriticalSection {
public:
  SK_AutoCriticalSection ( CRITICAL_SECTION* pCS,
                           bool              try_only = false )
  {
    acquired_ = false;
    cs_       = pCS;

    if (try_only)
      TryEnter ();
    else {
      Enter ();
    }
  }

  ~SK_AutoCriticalSection (void) ///
  {
    Leave ();
  }

  bool try_result (void)
  {
    return acquired_;
  }

  void enter (void)
  {
    EnterCriticalSection (this->cs_);

    acquired_ = true;
  }

protected:
  bool TryEnter (_Acquires_lock_(* this->cs_) void)
  {
    return (acquired_ = (TryEnterCriticalSection (cs_) != FALSE));
  }

  void Enter (_Acquires_lock_(* this->cs_) void)
  {
    EnterCriticalSection (cs_);

    acquired_ = true;
  }

  void Leave (_Releases_lock_(* this->cs_) void)
  {
    if (acquired_ != false)
      LeaveCriticalSection (cs_);

    acquired_ = false;
  }

private:
  bool              acquired_;
  CRITICAL_SECTION* cs_;
};





DWORD
WINAPI
SK_WaitForSingleObject ( _In_ HANDLE hHandle,
                         _In_ DWORD  dwMilliseconds );

// The underlying Nt kernel wait function has usec granularity,
//   this is a cheat that satisfies WAIT_OBJECT_n semantics rather
//     than the complicated NTSTATUS stuff going on under the hood.
DWORD
WINAPI
SK_WaitForSingleObject_Micro ( _In_ HANDLE        hHandle,
                               _In_ LARGE_INTEGER liMicroseconds );


__forceinline
static void
SK_Thread_SpinUntilFlagged (volatile LONG* pFlag, LONG _SpinMax = 75L)
{
  while (! ReadAcquire (pFlag))
  {
    for (int i = 0; i < _SpinMax && (! ReadAcquire (pFlag)); i++)
      ;

    if (ReadAcquire (pFlag) == 1)
      break;

    SleepEx (2UL, TRUE);
  }
}


__forceinline
static void
SK_Thread_SpinUntilAtomicMin (volatile LONG* pVar, LONG count, LONG _SpinMax = 75L)
{
  while (ReadAcquire (pVar) < count)
  {
    for (int i = 0; i < _SpinMax && (ReadAcquire (pVar) < count); i++)
      ;

    if (ReadAcquire (pVar) >= count)
      break;

    SleepEx (2UL, TRUE);
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

extern "C" SetThreadDescription_pfn SK_SetThreadDescription;
extern "C" GetThreadDescription_pfn SK_GetThreadDescription;

extern "C" HRESULT WINAPI SetCurrentThreadDescription (_In_  PCWSTR lpThreadDescription);
extern "C" HRESULT WINAPI GetCurrentThreadDescription (_Out_  PWSTR  *threadDescription);

extern "C" bool SK_Thread_InitDebugExtras (void);

extern "C" SetThreadAffinityMask_pfn SetThreadAffinityMask_Original;


extern DWORD SK_GetRenderThreadID (void);

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

    void flush (void) {
      InterlockedExchange (&pending, TRUE);
    }
  } change;

  void queuePriority (AVRT_PRIORITY prio)
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
    if (AvSetMmThreadPriority (hTask, prio))
    {
      priority = prio;
      return TRUE;
    }

    return FALSE;
  }

  HANDLE        setMaxCharacteristics ( const char* first_task,
                                        const char* second_task )
  {
    hTask =
      AvSetMmMaxThreadCharacteristicsA (first_task, second_task, &dwTaskIdx);

    if (hTask != 0)
    {
      strncpy (task0, first_task,  63);
      strncpy (task1, second_task, 63);
    }

    else
      hTask = INVALID_HANDLE_VALUE;

    return hTask;
  }

  BOOL disassociateWithTask (void)
  {
    return
      AvRevertMmThreadCharacteristics (hTask);
  }

  HANDLE reassociateWithTask (void)
  {
    hTask =
      AvSetMmMaxThreadCharacteristicsA (task0, task1, &dwTaskIdx);

    if (hTask != 0)
    {
      AvSetMmThreadPriority (hTask, priority);
    }

    return hTask;
  }
} static nul_task_ref;

size_t
SK_MMCS_GetTaskCount (void);

extern
SK_MMCS_TaskEntry*
SK_MMCS_GetTaskForThreadID (DWORD dwTid, const char* name);


#include <winnt.h>

static
__forceinline
DWORD
SK_GetCurrentThreadId (void)
{
  return ((DWORD *)NtCurrentTeb ()) [
#ifdef _WIN64
    18
#else
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
#ifdef _WIN64
    0x290 + tls_idx
#else
    0x384 + tls_idx
#endif
  ];
}


#endif /* __SK__THREAD_H__ */