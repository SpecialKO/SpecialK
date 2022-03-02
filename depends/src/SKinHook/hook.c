/*
 *  MinHook - The Minimalistic API Hooking Library for x64/x86
 *  Copyright (C) 2009-2016 Tsuda Kageyu.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 *  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 *  PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
 *  OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <windows.h>
#include <tlhelp32.h>
#include <limits.h>
#include <assert.h>
#include <intsafe.h>
#include <WinUser.h>

#include "../../include/MinHook/MinHook.h"
#include "buffer.h"
#include "trampoline.h"



#pragma pack (push,8)
#ifndef  NT_SUCCESS
# define NT_SUCCESS(Status)           (((NTSTATUS)(Status)) >= 0)
# define STATUS_SUCCESS                ((NTSTATUS)0x00000000L)
# define STATUS_UNSUCCESSFUL           ((NTSTATUS)0xC0000001L)
# define STATUS_INFO_LENGTH_MISMATCH   ((NTSTATUS)0xC0000004L)
# define STATUS_NO_SUCH_FILE           ((NTSTATUS)0xC000000FL)
# define STATUS_ACCESS_DENIED          ((NTSTATUS)0xc0000022L)
# define STATUS_BUFFER_TOO_SMALL       ((NTSTATUS)0xC0000023L)
# define STATUS_ALERTED                ((NTSTATUS)0x00000101L)
# define STATUS_PROCESS_IS_TERMINATING ((NTSTATUS)0xC000010AL)
#endif
#define SystemProcessAndThreadInformation       5

typedef _Return_type_success_(return >= 0)
        LONG       NTSTATUS;
typedef NTSTATUS *PNTSTATUS;
typedef LONG       KPRIORITY;
typedef LONG       KWAIT_REASON;

typedef struct _CLIENT_ID {
    HANDLE         UniqueProcess;
    HANDLE         UniqueThread;
} CLIENT_ID;

typedef struct _UNICODE_STRING {
    USHORT         Length;
    USHORT         MaximumLength;
    PWSTR          Buffer;
} UNICODE_STRING;

typedef struct _SYSTEM_THREAD {
    FILETIME     ftKernelTime;
    FILETIME     ftUserTime;
    FILETIME     ftCreateTime;
    ULONG        dWaitTime;
#ifdef _WIN64
    DWORD32      dwPaddingFor64Bit;
#endif
    PVOID        pStartAddress;
    CLIENT_ID    Cid;           // PID / TID Pairing
    KPRIORITY    dPriority;
    LONG         dBasePriority;
    ULONG        dContextSwitches;
    ULONG        dThreadState;  // 2 = running,  5 = waiting
    KWAIT_REASON WaitReason;

    // Not needed if correct packing is used, but these data structures
    //   have tricky alignment and the whole world needs to know!
    DWORD32      dwPaddingEveryoneGets;
} SYSTEM_THREAD,             *PSYSTEM_THREAD;
#define SYSTEM_THREAD_ sizeof (SYSTEM_THREAD)
// -----------------------------------------------------------------
typedef struct _SYSTEM_PROCESS     // common members
{
    ULONG          NextThreadOffset;
    ULONG          TotalThreadCount;
    LARGE_INTEGER  WorkingSetPrivateSize;
    ULONG          HardFaultCount;
    ULONG          NumberOfThreadsHighWatermark;
    ULONGLONG      CycleTime;
    LARGE_INTEGER  CreateTime;
    LARGE_INTEGER  UserTime;
    LARGE_INTEGER  KernelTime;
    UNICODE_STRING ImageName;
    KPRIORITY      BasePriority;
    HANDLE         UniqueProcessId;
    HANDLE         InheritedFromUniqueProcessId;
    ULONG          HandleCount;
    ULONG          SessionId;
    ULONG_PTR      UniqueProcessKey;
    ULONG_PTR      PeakVirtualSize;
    ULONG_PTR      VirtualSize;
    ULONG          PageFaultCount;
    ULONG_PTR      PeakWorkingSetSize;
    ULONG_PTR      WorkingSetSize;
    ULONG_PTR      QuotaPeakPagedPoolUsage;
    ULONG_PTR      QuotaPagedPoolUsage;
    ULONG_PTR      QuotaPeakNonPagedPoolUsage;
    ULONG_PTR      QuotaNonPagedPoolUsage;
    ULONG_PTR      PagefileUsage;
    ULONG_PTR      PeakPagefileUsage;
    ULONG_PTR      PrivatePageCount;
    LARGE_INTEGER  ReadOperationCount;
    LARGE_INTEGER  WriteOperationCount;
    LARGE_INTEGER  OtherOperationCount;
    LARGE_INTEGER  ReadTransferCount;
    LARGE_INTEGER  WriteTransferCount;
    LARGE_INTEGER  OtherTransferCount;
    SYSTEM_THREAD  aThreads [1];
} SYSTEM_PROCESS_INFORMATION, *PSYSTEM_PROCESS_INFORMATION;
#define SYSTEM_PROCESS_ sizeof (SYSTEM_PROCESS_INFORMATION)

// There are about 100 more enumerates; all 100 of them are useless and omitted.
typedef enum _SYSTEM_INFORMATION_CLASS {
    SystemProcessInformation                              = 5,
} SYSTEM_INFORMATION_CLASS;

typedef NTSTATUS (WINAPI *NtQuerySystemInformation_pfn)(
  _In_      SYSTEM_INFORMATION_CLASS SystemInformationClass,
  _Inout_   PVOID                    SystemInformation,
  _In_      ULONG                    SystemInformationLength,
  _Out_opt_ PULONG                   ReturnLength
);

typedef NTSTATUS (WINAPI *NtDelayExecution_pfn)(
  _In_ BOOLEAN        Alertable,
  _In_ PLARGE_INTEGER DelayInterval
);
#pragma pack (pop)




#ifndef ARRAYSIZE
    #define ARRAYSIZE(A) (sizeof(A)/sizeof((A)[0]))
#endif

// Initial capacity of the HOOK_ENTRY buffer.
#define INITIAL_HOOK_CAPACITY   128

// Initial capacity of the thread IDs buffer.
#define INITIAL_THREAD_CAPACITY 128

// Special hook position values.
#define INVALID_HOOK_POS UINT_MAX
#define ALL_HOOKS_POS    UINT_MAX

// Freeze() action argument defines.
#define ACTION_DISABLE      0
#define ACTION_ENABLE       1
#define ACTION_APPLY_QUEUED 2

// Thread access rights for suspending/resuming threads.
#define THREAD_ACCESS \
    (THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION | THREAD_SET_CONTEXT)

// Hook information.
typedef struct _HOOK_ENTRY
{
  LPVOID pTarget;         // Address of the target function.
  LPVOID pDetour;         // Address of the detour or relay function.
  LPVOID pTrampoline;     // Address of the trampoline function.
  UINT8  backup [8];      // Original prologue of the target function.

  UINT8  patchAbove  : 1; // Uses the hot patch area.
  UINT8  isEnabled   : 1; // Enabled.
  UINT8  queueEnable : 1; // Queued for enabling/disabling when != isEnabled.

  UINT   nIP         : 4; // Count of the instruction boundaries.
  UINT8  oldIPs [8];      // Instruction boundaries of the target function.
  UINT8  newIPs [8];      // Instruction boundaries of the trampoline function.
} HOOK_ENTRY, *PHOOK_ENTRY;

typedef struct _THREAD_ENTRY
{
  DWORD tid;
  BYTE  suspensions;     // Calling SuspendThread once guarantees *nothing!
  BYTE  runstate;        //
  BYTE  padding2[2];     //   (*) Multiple calls may be required and an equal
                         //       number of ResumeThread calls is necessary
} THREAD_ENTRY, *PTHREAD_ENTRY;// to put things back the way we found them.

// Suspended threads for Freeze()/Unfreeze().
typedef struct _FROZEN_THREADS
{
  PTHREAD_ENTRY pItems;   // Data heap
  UINT          capacity; // Size of allocated data heap, items
  UINT          size;     // Actual number of data items
  DWORD         priority; // Original thread priority
  BOOL          boost;    // Original thread boost (wildly misnamed API!)
  DWORD_PTR     affinity; // Original thread affinity
} FROZEN_THREADS, *PFROZEN_THREADS;

//-------------------------------------------------------------------------
// Global Variables:
//-------------------------------------------------------------------------

// Spin lock flag for EnterSpinLock()/LeaveSpinLock().
volatile LONG   g_isLocked = FALSE;

// Private heap handle. If not NULL, this library is initialized.
         HANDLE g_hHeap    = NULL;

// Secondary heap, because NtDll snapshots are massive.
struct
{
  NtQuerySystemInformation_pfn QuerySystemInformation;
  NtDelayExecution_pfn         DelayExecution;

  HMODULE                      Module;
  HANDLE                       hHeap;
  DWORD                        dwHeapSize;
  DWORD                        dwPadding0;
  PSYSTEM_PROCESS_INFORMATION  pSnapshot;
} g_NtDll = { NULL, NULL, NULL, NULL, 0, 0, NULL };

// Hook entries.
struct
{
  PHOOK_ENTRY pItems;     // Data heap
  UINT        capacity;   // Size of allocated data heap, items
  UINT        size;       // Actual number of data items
} g_hooks [4];

//-------------------------------------------------------------------------
static
VOID
MicroSleep (VOID)
{
  static LARGE_INTEGER            liDelay = { .QuadPart = -25LL };
  g_NtDll.DelayExecution (FALSE, &liDelay);
}

static
VOID
EnterSpinLock (VOID)
{
  SIZE_T spinCount = 0;

  // Wait until the flag is FALSE.
  while (InterlockedCompareExchange (&g_isLocked, TRUE, FALSE) != FALSE)
  {
    // No need to generate a memory barrier here, since InterlockedCompareExchange()
    // generates a full memory barrier itself.

    // Prevent the loop from being too busy.
    if (spinCount++ < 17)
        ;
    else
    {
      MicroSleep ();
      spinCount = 0;
    }
  }
}

//-------------------------------------------------------------------------
static
VOID
LeaveSpinLock (VOID)
{
  // No need to generate a memory barrier here, since InterlockedExchange()
  // generates a full memory barrier itself.

  InterlockedExchange (&g_isLocked, FALSE);
}

//-------------------------------------------------------------------------
// Returns INVALID_HOOK_POS if not found.
static
UINT
FindHookEntryEx (LPVOID pTarget, UINT idx)
{
  UINT i;

  for (i = 0; i < g_hooks [idx].size; ++i)
  {
    if ((ULONG_PTR)pTarget == (ULONG_PTR)g_hooks [idx].pItems [i].pTarget)
      return i;
  }

  return INVALID_HOOK_POS;
}

//-------------------------------------------------------------------------
// Returns INVALID_HOOK_POS if not found.
static
UINT
FindHookEntry (LPVOID pTarget)
{
  return FindHookEntryEx (pTarget, 0);
}

//-------------------------------------------------------------------------
static
PHOOK_ENTRY
AddHookEntryEx (UINT idx)
{
  if (g_hooks [idx].pItems == NULL)
  {
    g_hooks [idx].capacity = INITIAL_HOOK_CAPACITY;
    g_hooks [idx].pItems   = (PHOOK_ENTRY)HeapAlloc (
        g_hHeap, 0,
          g_hooks [idx].capacity * sizeof (HOOK_ENTRY)
    );

    if (g_hooks [idx].pItems == NULL)
      return NULL;
  }
  else if (g_hooks [idx].size >= g_hooks [idx].capacity)
  {
    PHOOK_ENTRY p = (PHOOK_ENTRY)HeapReAlloc (
        g_hHeap, 0,
          g_hooks [idx].pItems,
         ((SIZE_T)g_hooks [idx].capacity * 2) * sizeof (HOOK_ENTRY)
    );

    if (p == NULL)
      return NULL;

    g_hooks [idx].capacity *= 2;
    g_hooks [idx].pItems    = p;
  }

  return &g_hooks [idx].pItems [g_hooks [idx].size++];
}

static
PHOOK_ENTRY
AddHookEntry (void)
{
  return AddHookEntryEx (0);
}

//-------------------------------------------------------------------------
static
void
DeleteHookEntryEx (UINT pos, UINT idx)
{
  if (pos < g_hooks [idx].size - 1)
    g_hooks [idx].pItems [pos] =
      g_hooks [idx].pItems [g_hooks [idx].size - 1];

  g_hooks [idx].size--;

  if ( g_hooks [idx].capacity / 2 >= INITIAL_HOOK_CAPACITY &&
       g_hooks [idx].capacity / 2 >= g_hooks [idx].size )
  {
    PHOOK_ENTRY p = (PHOOK_ENTRY)HeapReAlloc (
        g_hHeap, 0, g_hooks [idx].pItems,
       (g_hooks [idx].capacity / 2) * sizeof (HOOK_ENTRY)
    );

    if (p == NULL)
        return;

    g_hooks [idx].capacity /= 2;
    g_hooks [idx].pItems    = p;
  }
}

//-------------------------------------------------------------------------
static
void
DeleteHookEntry (UINT pos)
{
  DeleteHookEntryEx (pos, 0);
}

//-------------------------------------------------------------------------
static
DWORD_PTR
FindOldIP (PHOOK_ENTRY pHook, DWORD_PTR ip)
{
  UINT i;

  if ( pHook->patchAbove &&
        ip == ((DWORD_PTR)pHook->pTarget - sizeof (JMP_REL)) ) {
    return (DWORD_PTR)pHook->pTarget;
  }

  for (i = 0; i < pHook->nIP; ++i)
  {
    if (ip == ((DWORD_PTR)pHook->pTrampoline + pHook->newIPs [i]))
      return   (DWORD_PTR)pHook->pTarget     + pHook->oldIPs [i];
  }

#ifdef _M_X64
  // Check relay function.
  if ( ip == (DWORD_PTR)pHook->pDetour )
      return (DWORD_PTR)pHook->pTarget;
#endif

  return 0;
}

//-------------------------------------------------------------------------
static
DWORD_PTR
FindNewIP (PHOOK_ENTRY pHook, DWORD_PTR ip)
{
  UINT i;

  for (i = 0; i < pHook->nIP; ++i)
  {
    if (ip == ((DWORD_PTR)pHook->pTarget     + pHook->oldIPs [i]))
      return   (DWORD_PTR)pHook->pTrampoline + pHook->newIPs [i];
  }

  return 0;
}

//-------------------------------------------------------------------------
static
void
ProcessThreadIPsEx ( HANDLE hThread, UINT pos, UINT action,
                     UINT   idx)
{
  // If the thread suspended in the overwritten area,
  // move IP to the proper address.

  CONTEXT c;
#ifdef _M_X64
  DWORD64 *pIP = &c.Rip;
#else
  DWORD   *pIP = &c.Eip;
#endif
  UINT count;

  c.ContextFlags = CONTEXT_CONTROL;

  if (! GetThreadContext (hThread, &c))
    return;

  if (pos == ALL_HOOKS_POS)
  {
    pos   = 0;
    count = g_hooks [idx].size;
  }
  else
  {
    count = pos + 1;
  }

  for (; pos < count; ++pos)
  {
    PHOOK_ENTRY pHook = &g_hooks [idx].pItems [pos];
    BOOL        enable = FALSE;
    DWORD_PTR   ip;

    switch (action)
    {
    case ACTION_DISABLE:
      enable = FALSE;
      break;

    case ACTION_ENABLE:
      enable = TRUE;
      break;

    case ACTION_APPLY_QUEUED:
      enable = pHook->queueEnable;
      break;
    }

    if (pHook->isEnabled == enable)
      continue;

    if (enable)
      ip = FindNewIP (pHook, *pIP);
    else
      ip = FindOldIP (pHook, *pIP);

    if (ip != 0)
    {
      *pIP = ip;
      SetThreadContext (hThread, &c);
    }
  }
}

//-------------------------------------------------------------------------
static
void
ProcessThreadIPs (HANDLE hThread, UINT pos, UINT action)
{
  ProcessThreadIPsEx (hThread, pos, action, 0);
}

//-------------------------------------------------------------------------

PSYSTEM_PROCESS_INFORMATION
SnapshotProcs_NtDll (void)
{
  DWORD                      dSize = 0;
  DWORD                      dData = 0;
  NTSTATUS                      ns = STATUS_INVALID_PARAMETER;
  PSYSTEM_PROCESS_INFORMATION pspi = NULL;


  if (g_NtDll.QuerySystemInformation == NULL)
  {
    g_NtDll.Module =
      GetModuleHandleW (L"NtDll.dll");

    if (! g_NtDll.Module)
      return pspi;

    g_NtDll.QuerySystemInformation =
      (NtQuerySystemInformation_pfn)
        GetProcAddress (g_NtDll.Module, "NtQuerySystemInformation");
  }


  RtlSecureZeroMemory ( g_NtDll.pSnapshot, g_NtDll.dwHeapSize );

  ns = g_NtDll.QuerySystemInformation ( SystemProcessInformation,
                                          g_NtDll.pSnapshot,
                                          g_NtDll.dwHeapSize,  &dData );

  // Memory was not filled.
  if (ns != STATUS_SUCCESS)
  {
    for (  dSize = g_NtDll.dwHeapSize   ;
          (pspi == NULL) && dSize  != 0 ;
                            dSize <<= 1  )
    {
      if ( ( pspi = HeapReAlloc ( g_NtDll.hHeap,     0x0,
                                  g_NtDll.pSnapshot, dSize ) ) == NULL )
      {
        ns = STATUS_NO_MEMORY;
        break;
      }

      g_NtDll.dwHeapSize = dSize;
      g_NtDll.pSnapshot  = pspi;

      ns = g_NtDll.QuerySystemInformation ( SystemProcessInformation,
                                              pspi, dSize, &dData );

      if (ns != STATUS_SUCCESS)
      {
        pspi  = NULL;
        dData = 0;

        if (ns != STATUS_INFO_LENGTH_MISMATCH) break;
      }
    }
  }

  if (dData != 0)
    return g_NtDll.pSnapshot;

//OutputDebugStringA ("Total failure!  (NtStat=%x)", ns);

  return NULL;
}

static
VOID
EnumerateThreads (PFROZEN_THREADS pThreads)
{
  DWORD dwPID = GetCurrentProcessId (),
        dwTID = GetCurrentThreadId  ();

  PSYSTEM_PROCESS_INFORMATION pProc =
    SnapshotProcs_NtDll ();

  if (pProc == NULL)
    return;

  int i = 0;

  do
  {
    if ((DWORD)((uintptr_t)pProc->UniqueProcessId & 0xFFFFFFFFU) == dwPID)
      break;

    pProc = (SYSTEM_PROCESS_INFORMATION *)((BYTE *)pProc + pProc->NextThreadOffset);
  } while (pProc->NextThreadOffset != 0);


  if ((DWORD)((uintptr_t)pProc->UniqueProcessId & 0xFFFFFFFFU) == dwPID)
  {
    int threads =
      pProc->TotalThreadCount;

    for ( i = 0; i < threads; ++i )
    {
      // We want threads for the current PID, except for the thread we're running from!
      if ( (DWORD)((uintptr_t)pProc->aThreads [i].Cid.UniqueThread  & 0xFFFFFFFFU) == dwTID ||
           (DWORD)((uintptr_t)pProc->aThreads [i].Cid.UniqueProcess & 0xFFFFFFFFU) != dwPID )
        continue;

      if (pThreads != NULL)
      {
        if (pThreads->pItems == NULL)
        {
          pThreads->capacity = INITIAL_THREAD_CAPACITY;
          pThreads->pItems   =
                (PTHREAD_ENTRY)HeapAlloc (
                  g_hHeap, 0,
                    pThreads->capacity * sizeof (THREAD_ENTRY)
                );

          if (pThreads->pItems == NULL)
            break;
        }

        else if (pThreads->size >= pThreads->capacity)
        {
          PTHREAD_ENTRY p =
              (PTHREAD_ENTRY)HeapReAlloc (
                g_hHeap, 0,
                  pThreads->pItems,
        (SIZE_T)(((UINT)pThreads->capacity << 1) & (0xFFFFFFFFUL)) * sizeof (THREAD_ENTRY)
              );

          if (p == NULL)
            break;

          pThreads->capacity <<= 1;
          pThreads->pItems     = p;
        }

        pThreads->pItems [pThreads->size++].tid =
          (DWORD)((uintptr_t)pProc->aThreads [i].Cid.UniqueThread & 0xFFFFFFFFU);
      }
    }
  }
}

#include <corecrt_wstdio.h>
#include <stdio.h>

//-------------------------------------------------------------------------
static
VOID
FreezeEx (PFROZEN_THREADS pThreads, UINT pos, UINT action, UINT idx)
{
  HANDLE hThreadSelf =
    GetCurrentThread ();

  pThreads->pItems   = NULL;
  pThreads->capacity = 0;
  pThreads->size     = 0;
  pThreads->priority = GetThreadPriority     (hThreadSelf);
  pThreads->affinity = SetThreadAffinityMask (hThreadSelf, 0x0);

  SetThreadAffinityMask  (hThreadSelf, DWORD_PTR_MAX);
  SetThreadPriority      (hThreadSelf, THREAD_PRIORITY_HIGHEST);

  if (! GetThreadPriorityBoost (hThreadSelf, &pThreads->boost))
                                              pThreads->boost = -1;
  else  SetThreadPriorityBoost (hThreadSelf, FALSE);


  EnumerateThreads (pThreads);

  // Prevent deadlocks with other hook software that might
  //   be in the process of suspending stuff.
  //
  if (pThreads->pItems != NULL)
  {
    SIZE_T spinCount = 0;

    UINT i = 0, frozen = 0, running = 0;

          float frozen_ratio     = 0.0f;
    const float frozen_threshold = 0.01666f;

    DWORD dwSelfThreadId =
      GetCurrentThreadId ();

    do {
      if (spinCount != 0)
      {
        wchar_t wszOutput [256];
               *wszOutput = L'\0';
       *(wszOutput + 255) = L'\0';

#ifdef _WIN64
        swprintf_s (wszOutput, 255, L"MinHook: Deep Freeze: Ratio=%f [%f/%f] { Spins: %I64u }",
                    frozen_ratio, (float)frozen, (float)running, spinCount);
#else
        swprintf_s (wszOutput, 255, L"MinHook: Deep Freeze: Ratio=%f [%f/%f] { Spins: %Iu }",
                    frozen_ratio, (float)frozen, (float)running, spinCount);
#endif

        OutputDebugStringW (wszOutput);
      }

      frozen  = 0;
      running = 0;

      if ((spinCount % 7) == 0)
        MicroSleep ();

      if (spinCount >= 56)
        break;

      for (i = 0; i < pThreads->size; ++i)
      {
        PTHREAD_ENTRY pThread =
          &pThreads->pItems [i];

        if (pThread->tid == dwSelfThreadId)
          continue;

        HANDLE hThread =
          OpenThread ( THREAD_ACCESS,
                         FALSE,
                           pThread->tid );

        if (hThread != 0)
        {
          DWORD dwExitCode = 0;
          DWORD  dwSuspend =
            SuspendThread (hThread);

          CONTEXT                     threadContext;
          GetThreadContext (hThread, &threadContext);

          if (GetExitCodeThread (hThread, &dwExitCode))
          {
            if (dwExitCode == STILL_ACTIVE)
            {
              ++running;

              if (dwSuspend < MAXIMUM_SUSPEND_COUNT)
              {
                if (dwSuspend > 0)
                {
                  ++frozen;
                }
              }
            }
          }

          else
          {
            ++running;

            if (dwSuspend != (DWORD) -1)
            {
              if (dwSuspend > 0)
              {
                ++frozen;
              }
            }
          }

          if (dwSuspend != (DWORD)-1)
            ResumeThread (hThread);

          CloseHandle (hThread);
        }
      }

      frozen_ratio = (float)frozen /
                     (float)running;

      spinCount++;
    } while ( frozen_ratio > frozen_threshold &&
              frozen       > 1 );
  }


  if (pThreads->pItems != NULL)
  {
    UINT i;

    for (i = 0; i < pThreads->size; ++i)
    {
      PTHREAD_ENTRY pThread =
        &pThreads->pItems [i];

      HANDLE hThread =
        OpenThread ( THREAD_ACCESS,
                       FALSE,
                         pThread->tid );

      pThread->suspensions = 0;
      pThread->runstate    = 2; // Unknown

      if (hThread != NULL)
      {
        if ( pThread->tid          == GetCurrentThreadId () ||
             GetThreadId (hThread) != pThread->tid             )
        {
          CloseHandle (hThread);
          continue;
        }

        DWORD                            dwExitCode = 0;
        if (GetExitCodeThread (hThread, &dwExitCode))
        {
          pThread->runstate =
            ( dwExitCode == STILL_ACTIVE ? 1 : 0 );
        }

        if (pThread->runstate == 1)
        {
          DWORD dwRet,
                dwErr = GetLastError ();

          int tries = 0;
#define   MAX_TRIES 16

          do
          {
            ++tries;

            if ( (dwRet = SuspendThread (hThread)) != (DWORD)-1 )
            {
              CONTEXT                     threadContext;
              GetThreadContext (hThread, &threadContext);

              ++pThread->suspensions;

              // Thread is suspended, we can stop now
              if (dwRet < MAXIMUM_SUSPEND_COUNT)
                break;

              --pThread->suspensions;
            }

            else
            {
              dwErr =
                GetLastError ();

              if ( dwErr == ERROR_THREAD_NOT_IN_PROCESS ||
                   dwErr == ERROR_THREAD_WAS_SUSPENDED )
              {
                pThread->runstate = 0;
                break;
              }
            }
          //if (dwErr != NO_ERROR) OutputDebugStringA ("dwErr != NO_ERROR");//assert (dwErr == NO_ERROR);
          } while ( pThread->suspensions == 0 &&
                    tries < MAX_TRIES );

          if ( pThread->runstate    != 0 &&
               pThread->suspensions != 0    )
          {
            ProcessThreadIPsEx (hThread, pos, action, idx);
          }
        }
        CloseHandle            (hThread);
      }
    }
  }
}

static
VOID
Freeze (PFROZEN_THREADS pThreads, UINT pos, UINT action)
{
  FreezeEx (pThreads, pos, action, 0);
}

//-------------------------------------------------------------------------
static
VOID
Unfreeze (PFROZEN_THREADS pThreads)
{
  if (pThreads->pItems != NULL)
  {
    UINT i, j;

    for (i = 0; i < pThreads->size; ++i)
    {
      PTHREAD_ENTRY pThread =
        &pThreads->pItems [i];

      if ( pThread->runstate   == 1 ||
           pThread->suspensions > 0 )
      {
        HANDLE hThread =
          OpenThread ( THREAD_SUSPEND_RESUME |
                       THREAD_QUERY_LIMITED_INFORMATION,
                         FALSE,
                           pThread->tid );

        if ( hThread != NULL )
        {
          DWORD                                dwExit = 0;
          if ( (! GetExitCodeThread (hThread, &dwExit)) ||
                                               dwExit == STILL_ACTIVE )
          {
            for (j = 0; j < pThread->suspensions; ++j)
              ResumeThread (hThread);
          }

          CloseHandle (hThread);
        }
      }
    }

    HeapFree (g_hHeap, 0, pThreads->pItems);
  }

  if (pThreads->boost != -1)
    SetThreadPriorityBoost (GetCurrentThread (), pThreads->boost);

  SetThreadAffinityMask    (GetCurrentThread (), pThreads->affinity);
  SetThreadPriority        (GetCurrentThread (), pThreads->priority);
}

//-------------------------------------------------------------------------
static
MH_STATUS
EnableHookLLEx (UINT pos, UINT8 enable, UINT idx)
{
  PHOOK_ENTRY pHook        = &g_hooks [idx].pItems [pos];
  DWORD       oldProtect;
  SIZE_T      patchSize    = sizeof (JMP_REL);
  LPBYTE      pPatchTarget = (LPBYTE)pHook->pTarget;

  if (pHook->patchAbove)
  {
    pPatchTarget -= sizeof (JMP_REL);
    patchSize    += sizeof (JMP_REL_SHORT);
  }

  if (! VirtualProtect ( pPatchTarget,            patchSize,
                         PAGE_EXECUTE_READWRITE, &oldProtect ) )
    return MH_ERROR_MEMORY_PROTECT;

  if (enable)
  {
    PJMP_REL pJmp = (PJMP_REL)pPatchTarget;
    pJmp->opcode  = 0xE9;
    pJmp->operand = (UINT32)( (LPBYTE)pHook->pDetour -
                              (pPatchTarget + sizeof (JMP_REL)) );

    if (pHook->patchAbove)
    {
      PJMP_REL_SHORT pShortJmp = (PJMP_REL_SHORT)pHook->pTarget;
      pShortJmp->opcode        = 0xEB;
      // Decided to have some fun getting rid of compiler warnings
      //   the stupidest way imaginable
      pShortJmp->operand       = (UINT8)                         (
                                  (INT8)              0x0        -
                                  (INT8)( sizeof (JMP_REL_SHORT) +
                                          sizeof (JMP_REL)     ) );
    }
  }

  else
  {
    if (pHook->patchAbove)
      memcpy ( pPatchTarget, pHook->backup,
                 sizeof (JMP_REL) + sizeof (JMP_REL_SHORT)   );
    else
      memcpy ( pPatchTarget, pHook->backup, sizeof (JMP_REL) );
  }

  VirtualProtect ( pPatchTarget,  patchSize,
                   oldProtect,   &oldProtect );

  // Just-in-case measure.  (Silly on x86/x64: They have snooping caches)
  FlushInstructionCache ( GetCurrentProcess (),
                            pPatchTarget,
                              patchSize );

  pHook->isEnabled   = enable;
  pHook->queueEnable = enable;

  return MH_OK;
}

static
MH_STATUS
EnableHookLL (UINT pos, BOOL enable)
{
  return
    EnableHookLLEx (pos, enable != FALSE ?
                              1 : 0, 0   );
}

//-------------------------------------------------------------------------
static
MH_STATUS
EnableAllHooksLLEx (BOOL enable, UINT idx)
{
  MH_STATUS status = MH_OK;
  UINT      i,
            first  = INVALID_HOOK_POS;

  for (i = 0; i < g_hooks [idx].size; ++i)
  {
    if (g_hooks [idx].pItems [i].isEnabled != enable)
    {
      first = i;
      break;
    }
  }

  if (first != INVALID_HOOK_POS)
  {
    FROZEN_THREADS threads;

    FreezeEx ( &threads, ALL_HOOKS_POS,
           enable != 0 ? (UINT)ACTION_ENABLE :
                         (UINT)ACTION_DISABLE, idx );

    for (i = first; i < g_hooks [idx].size; ++i)
    {
      if (g_hooks [idx].pItems [i].isEnabled != enable)
      {
        status = EnableHookLLEx (i, enable != FALSE ?
                                         1 : 0, idx );

        if (status != MH_OK)
          break;
      }
    }

    Unfreeze (&threads);
  }

  return status;
}

static
MH_STATUS
EnableAllHooksLL (BOOL enable)
{
  return
    EnableAllHooksLLEx (enable, 0);
}

//-------------------------------------------------------------------------
MH_STATUS
WINAPI
MH_Initialize (VOID)
{
  MH_STATUS status = MH_OK;

  if (g_NtDll.DelayExecution == NULL)
  {
    g_NtDll.Module =
      GetModuleHandleW (L"NtDll.dll");

    if (g_NtDll.Module)
    {   g_NtDll.DelayExecution =
          (NtDelayExecution_pfn)
            GetProcAddress (g_NtDll.Module, "NtDelayExecution");
    }
  }

  EnterSpinLock ();

  if (g_hHeap == NULL)
  {
      g_hHeap     = HeapCreate ( 0x0, sizeof (HOOK_ENTRY)     * (INITIAL_HOOK_CAPACITY*4) + 1 +
                                      sizeof (FROZEN_THREADS) * INITIAL_THREAD_CAPACITY   + 1 +
                                      16384 * 16384, 0 );

    g_NtDll.hHeap = HeapCreate ( 0x0, 4194304, 0 );

    if (g_hHeap != NULL && g_NtDll.hHeap != NULL)
    {
      // Initialize the internal function buffer.
      InitializeBuffer ();

      g_NtDll.pSnapshot  = HeapAlloc ( g_NtDll.hHeap, 0x0, 4194304);
      g_NtDll.dwHeapSize = g_NtDll.pSnapshot != NULL ? 4194304 : 0;
    }

    else
    {
      status = MH_ERROR_MEMORY_ALLOC;
    }
  }

  else
  {
    status = MH_ERROR_ALREADY_INITIALIZED;
  }

  LeaveSpinLock ();

  return status;
}

//-------------------------------------------------------------------------
MH_STATUS
WINAPI
MH_Uninitialize (VOID)
{
  MH_STATUS status = MH_OK;

  EnterSpinLock ();

  if (g_hHeap != NULL)
  {
    for (int i = 0; i < 4; i++)
    {
      status =
        EnableAllHooksLLEx (FALSE, i);

      if (status == MH_OK)
      {
        // HeapFree is actually not required, but some tools detect a false
        // memory leak without HeapFree.

        HeapFree    (g_hHeap,       0, g_hooks [i].pItems);
        HeapFree    (g_NtDll.hHeap, 0, g_NtDll.pSnapshot );

        g_hooks [i].pItems   = NULL;
        g_hooks [i].capacity = 0;
        g_hooks [i].size     = 0;

        g_NtDll.pSnapshot    = NULL;
        g_NtDll.dwHeapSize   = 0UL;
      }
    }

    // Free the internal function buffer.
    UninitializeBuffer ();

    HeapDestroy        (g_hHeap);
    HeapDestroy        (g_NtDll.hHeap);
                        g_NtDll.hHeap = NULL;
                        g_hHeap       = NULL;
  }

  else
  {
    status = MH_ERROR_NOT_INITIALIZED;
  }

  LeaveSpinLock ();

  return status;
}

//-------------------------------------------------------------------------
MH_STATUS
WINAPI
MH_CreateHookEx ( LPVOID   pTarget,   LPVOID pDetour,
                  LPVOID *ppOriginal, UINT   idx )
{
  MH_STATUS status = MH_OK;

  EnterSpinLock ();

  if (g_hHeap != NULL)
  {
    if (IsExecutableAddress (pTarget) && IsExecutableAddress (pDetour))
    {
      UINT pos = FindHookEntryEx (pTarget, idx);

      if (pos == INVALID_HOOK_POS)
      {
        LPVOID pBuffer = AllocateBuffer (pTarget);

        if (pBuffer != NULL)
        {
          TRAMPOLINE ct;

          ct.pTarget     = pTarget;
          ct.pDetour     = pDetour;
          ct.pTrampoline = pBuffer;

          if (CreateTrampolineFunction (&ct))
          {
            PHOOK_ENTRY pHook = AddHookEntryEx (idx);
            if (pHook != NULL)
            {
              pHook->pTarget     = ct.pTarget;
#ifdef _M_X64
              pHook->pDetour     = ct.pRelay;
#else
              pHook->pDetour     = ct.pDetour;
#endif
              pHook->pTrampoline = ct.pTrampoline;
              pHook->patchAbove  = ct.patchAbove != FALSE ? 1 : 0;
              pHook->isEnabled   = FALSE;
              pHook->queueEnable = FALSE;
              pHook->nIP         = ct.nIP;
              memcpy (pHook->oldIPs, ct.oldIPs, ARRAYSIZE (ct.oldIPs));
              memcpy (pHook->newIPs, ct.newIPs, ARRAYSIZE (ct.newIPs));

              // Back up the target function.

              if (ct.patchAbove)
              {
                memcpy ( pHook->backup,
                    (LPBYTE)pTarget    - sizeof (JMP_REL),
                      sizeof (JMP_REL) + sizeof (JMP_REL_SHORT)
                );
              }
              else
              {
                memcpy (pHook->backup, pTarget, sizeof (JMP_REL));
              }

              if (ppOriginal != NULL)
                *ppOriginal = pHook->pTrampoline;
            }
            else
            {
              status = MH_ERROR_MEMORY_ALLOC;
            }
          }
          else
          {
            status = MH_ERROR_UNSUPPORTED_FUNCTION;
          }

          if (status != MH_OK)
          {
            FreeBuffer (pBuffer);
          }
        }
        else
        {
          status = MH_ERROR_MEMORY_ALLOC;
        }
      }
      else
      {
        status = MH_ERROR_ALREADY_CREATED;
      }
    }
    else
    {
      status = MH_ERROR_NOT_EXECUTABLE;
    }
  }
  else
  {
    status = MH_ERROR_NOT_INITIALIZED;
  }

  LeaveSpinLock ();

  return status;
}

MH_STATUS
WINAPI
MH_CreateHook ( LPVOID   pTarget,
                LPVOID   pDetour,
                LPVOID *ppOriginal )
{
  return MH_CreateHookEx (pTarget, pDetour, ppOriginal, 0);
}

//-------------------------------------------------------------------------
MH_STATUS
WINAPI
MH_RemoveHookEx (LPVOID pTarget, UINT idx)
{
  MH_STATUS status = MH_OK;

  EnterSpinLock ();

  if (g_hHeap != NULL)
  {
    UINT pos = FindHookEntryEx (pTarget, idx);

    if (pos != INVALID_HOOK_POS)
    {
      if (g_hooks [idx].pItems [pos].isEnabled)
      {
        FROZEN_THREADS threads;
        FreezeEx (&threads, pos, ACTION_DISABLE, idx);

        status = EnableHookLLEx (pos, FALSE, idx);

        Unfreeze (&threads);
      }

      if (status == MH_OK)
      {
        FreeBuffer        (g_hooks [idx].pItems [pos].pTrampoline);
        DeleteHookEntryEx (pos, idx);
      }
    }
    else
    {
      status = MH_ERROR_NOT_CREATED;
    }
  }
  else
  {
    status = MH_ERROR_NOT_INITIALIZED;
  }

  LeaveSpinLock ();

  return status;
}

MH_STATUS
WINAPI
MH_RemoveHook (LPVOID pTarget)
{
  return MH_RemoveHookEx (pTarget, 0);
}

//-------------------------------------------------------------------------
static
MH_STATUS
EnableHookEx (LPVOID pTarget, BOOL enable, UINT idx)
{
  MH_STATUS status = MH_OK;

  EnterSpinLock ();

  if (g_hHeap != NULL)
  {
    if (pTarget == MH_ALL_HOOKS)
    {
      status =
        EnableAllHooksLLEx (enable, idx);
    }

    else
    {
      FROZEN_THREADS threads = { 0 };

      UINT pos =
        FindHookEntryEx (pTarget, idx);

      if (pos != INVALID_HOOK_POS)
      {
        if (g_hooks [idx].pItems [pos].isEnabled != enable)
        {
          FreezeEx (&threads, pos, ACTION_ENABLE, idx);

          status =
            EnableHookLLEx (pos, enable != 0 ? 1 : 0, idx);

          Unfreeze (&threads);
        }
        else
        {
          status = enable ? MH_ERROR_ENABLED :
                            MH_ERROR_DISABLED;
        }
      }

      else
      {
        status = MH_ERROR_NOT_CREATED;
      }
    }
  }

  else
  {
    status = MH_ERROR_NOT_INITIALIZED;
  }

  LeaveSpinLock ();

  return status;
}

static
MH_STATUS
EnableHook (LPVOID pTarget, BOOL enable)
{
  return EnableHookEx (pTarget, enable, 0);
}

//-------------------------------------------------------------------------
MH_STATUS
WINAPI
MH_EnableHookEx (LPVOID pTarget, UINT idx)
{
  return EnableHookEx (pTarget, TRUE, idx);
}

MH_STATUS
WINAPI
MH_EnableHook (LPVOID pTarget)
{
  return EnableHookEx (pTarget, TRUE, 0);
}

//-------------------------------------------------------------------------
MH_STATUS
WINAPI
MH_DisableHookEx (LPVOID pTarget, UINT idx)
{
  return EnableHookEx (pTarget, FALSE, idx);
}

MH_STATUS
WINAPI
MH_DisableHook (LPVOID pTarget)
{
  return MH_DisableHookEx (pTarget, 0);
}

//-------------------------------------------------------------------------
static
MH_STATUS
QueueHookEx (LPVOID pTarget, BOOL queueEnable, UINT idx)
{
  MH_STATUS status = MH_OK;

  EnterSpinLock ();

  if (g_hHeap != NULL)
  {
    if (pTarget == MH_ALL_HOOKS)
    {
      UINT i;
      for (i = 0; i < g_hooks [idx].size; ++i)
        g_hooks [idx].pItems [i].queueEnable =
                        (UINT8)(queueEnable != 0 ? 1 : 0);
    }
    else
    {
      UINT pos = FindHookEntryEx (pTarget, idx);
      if (pos != INVALID_HOOK_POS)
      {
        g_hooks [idx].pItems [pos].queueEnable =
                           (UINT8)(queueEnable != 0 ? 1 : 0);
      }
      else
      {
        status = MH_ERROR_NOT_CREATED;
      }
    }
  }
  else
  {
    status = MH_ERROR_NOT_INITIALIZED;
  }

  LeaveSpinLock ();

  return status;
}

static
MH_STATUS
QueueHook (LPVOID pTarget, BOOL queueEnable)
{
  return QueueHookEx (pTarget, queueEnable, 0);
}

//-------------------------------------------------------------------------
MH_STATUS
WINAPI
MH_QueueEnableHookEx (LPVOID pTarget, UINT idx)
{
  return QueueHookEx (pTarget, TRUE, idx);
}

MH_STATUS
WINAPI
MH_QueueEnableHook (LPVOID pTarget)
{
  return MH_QueueEnableHookEx (pTarget, 0);
}

//-------------------------------------------------------------------------
MH_STATUS
WINAPI
MH_QueueDisableHookEx (LPVOID pTarget, UINT idx)
{
  return QueueHookEx (pTarget, FALSE, idx);
}

MH_STATUS
WINAPI
MH_QueueDisableHook (LPVOID pTarget)
{
  return MH_QueueDisableHookEx (pTarget, 0);
}

//-------------------------------------------------------------------------
#include <debugapi.h>
#include <stdio.h>
#include <Windows.h>

MH_STATUS
WINAPI
MH_ApplyQueuedEx (UINT idx)
{
  MH_STATUS status = MH_OK;
  UINT      i,
            first  = INVALID_HOOK_POS;

  EnterSpinLock ();

  if (g_hHeap != NULL)
  {
    for (i = 0; i < g_hooks [idx].size; ++i)
    {
      if (g_hooks [idx].pItems [i].isEnabled != g_hooks [idx].pItems [i].queueEnable)
      {
        first = i;
        break;
      }
    }

    if (first != INVALID_HOOK_POS)
    {
      FROZEN_THREADS threads;

      FreezeEx (&threads, ALL_HOOKS_POS, ACTION_APPLY_QUEUED, idx);

      for (i = first; i < g_hooks [idx].size; ++i)
      {
        PHOOK_ENTRY pHook = &g_hooks [idx].pItems [i];

        if (pHook->isEnabled != pHook->queueEnable)
        {
          status = EnableHookLLEx (i, pHook->queueEnable, idx);

          if (status != MH_OK)
            break;
        }
      }

      Unfreeze (&threads);
    }
  }

  else
  {
    status = MH_ERROR_NOT_INITIALIZED;
  }

  //char szDesc [4096];
  //sprintf ( szDesc, "g_NtDll = { %ph, %ph, (%x), %lu, %lu, %ph }",
  //                g_NtDll.QuerySystemInformation, g_NtDll.Module, (unsigned int)((uintptr_t)g_NtDll.hHeap & 0xFFFFFFFFUL),
  //                g_NtDll.dwHeapSize, g_NtDll.dwPadding0, g_NtDll.pSnapshot );
  //
  //OutputDebugStringA (szDesc);

  LeaveSpinLock ();

  return status;
}

MH_STATUS
WINAPI MH_ApplyQueued (VOID)
{
  return MH_ApplyQueuedEx (0);
}

//-------------------------------------------------------------------------
MH_STATUS
WINAPI
MH_CreateHookApiEx2 (
    LPCWSTR   pszModule, LPCSTR   pszProcName,
    LPVOID    pDetour,
    LPVOID  *ppOriginal, LPVOID *ppTarget,
    UINT      idx )
{
  UNREFERENCED_PARAMETER (idx);

  HMODULE hModule;
  LPVOID  pTarget;

  hModule = GetModuleHandleW (pszModule);

  if (hModule == NULL)
      return MH_ERROR_MODULE_NOT_FOUND;

  pTarget = (LPVOID)GetProcAddress (hModule, pszProcName);

  if (pTarget == NULL)
      return MH_ERROR_FUNCTION_NOT_FOUND;

  if (ppTarget != NULL)
      *ppTarget = pTarget;

  return MH_CreateHookEx (pTarget, pDetour, ppOriginal, 0);
}

MH_STATUS
WINAPI
MH_CreateHookApiEx (
    LPCWSTR   pszModule, LPCSTR   pszProcName,
    LPVOID    pDetour,
    LPVOID  *ppOriginal, LPVOID *ppTarget )
{
  return MH_CreateHookApiEx2 ( pszModule, pszProcName,
                                 pDetour,
                                   ppOriginal, ppTarget,
                                     0 );
}

//-------------------------------------------------------------------------
MH_STATUS
WINAPI
MH_CreateHookApi2 (
    LPCWSTR pszModule, LPCSTR  pszProcName,
    LPVOID  pDetour,   LPVOID *ppOriginal,
    UINT    idx )
{
  return
      MH_CreateHookApiEx2 ( pszModule, pszProcName,
                              pDetour, ppOriginal, NULL,
                                idx );
}

MH_STATUS
WINAPI
MH_CreateHookApi (
    LPCWSTR pszModule, LPCSTR  pszProcName,
    LPVOID  pDetour,   LPVOID *ppOriginal )
{
  return MH_CreateHookApi2 ( pszModule, pszProcName,
                               pDetour, ppOriginal,
                                 0 );
}

//-------------------------------------------------------------------------
const char*
WINAPI
MH_StatusToString (MH_STATUS status)
{
#define MH_ST2STR(x)  \
  case x:             \
      return #x;

  switch (status) {
      MH_ST2STR (MH_UNKNOWN)
      MH_ST2STR (MH_OK)
      MH_ST2STR (MH_ERROR_ALREADY_INITIALIZED)
      MH_ST2STR (MH_ERROR_NOT_INITIALIZED)
      MH_ST2STR (MH_ERROR_ALREADY_CREATED)
      MH_ST2STR (MH_ERROR_NOT_CREATED)
      MH_ST2STR (MH_ERROR_ENABLED)
      MH_ST2STR (MH_ERROR_DISABLED)
      MH_ST2STR (MH_ERROR_NOT_EXECUTABLE)
      MH_ST2STR (MH_ERROR_UNSUPPORTED_FUNCTION)
      MH_ST2STR (MH_ERROR_MEMORY_ALLOC)
      MH_ST2STR (MH_ERROR_MEMORY_PROTECT)
      MH_ST2STR (MH_ERROR_MODULE_NOT_FOUND)
      MH_ST2STR (MH_ERROR_FUNCTION_NOT_FOUND)
  }
#undef MH_ST2STR

  return "(unknown)";
}

//-----------------------------------------------------------------------------
MH_STATUS
WINAPI
SH_IntrospectEx ( LPVOID            pTarget,
                  SH_INTROSPECT_ID  type,
                  LPVOID           *ppResult,
                  UINT              idx )
{
  if (ppResult == NULL)
    return MH_ERROR_MEMORY_ALLOC;

  UINT hook_entry =
    FindHookEntryEx (pTarget, idx);

  if (hook_entry == INVALID_HOOK_POS)
    return MH_ERROR_NOT_CREATED;

  switch (type)
  {
    // The trampoline function
    case SH_TRAMPOLINE:
    {
      *ppResult = g_hooks [idx].pItems [hook_entry].pTrampoline;
      return MH_OK;
    }

    // The detour function
    case SH_DETOUR:
    {
      *ppResult = g_hooks [idx].pItems [hook_entry].pDetour;
      return MH_OK;
    }

    // Is the target hotpatched?
    case SH_HOTPATCH:
    {
      *(ULONG_PTR *)ppResult =
        ((ULONG_PTR)g_hooks [idx].pItems [hook_entry].patchAbove & 0x1);
      return MH_OK;
    }

    // Is the hook enabled?
    case SH_ENABLED:
    {
      *(ULONG_PTR *)ppResult =
        ((ULONG_PTR)g_hooks [idx].pItems [hook_entry].isEnabled & 0x1);
      return MH_OK;
    }

    // Is the hook enabled?
    case SH_QUEUED_ENABLE:
    {
      *(ULONG_PTR *)ppResult =
        ((ULONG_PTR)g_hooks [idx].pItems [hook_entry].queueEnable & 0x1);
      return MH_OK;
    }

    case SH_NUM_IPS:
    case SH_OLD_IPS:
    case SH_NEW_IPS:
      return MH_ERROR_UNSUPPORTED_FUNCTION;

    default:
      return MH_UNKNOWN;
  }
}

MH_STATUS
WINAPI
SH_Introspect ( LPVOID            pTarget,
                SH_INTROSPECT_ID  type,
                LPVOID           *ppResult )
{
  return SH_IntrospectEx (pTarget, type, ppResult, 0);
}

MH_STATUS
WINAPI
SH_HookCountEx ( PUINT pHookCount, UINT idx )
{
  if (pHookCount == NULL)
    return MH_UNKNOWN;

  *pHookCount = g_hooks [idx].size;

  return MH_OK;
}

MH_STATUS
WINAPI
SH_HookCount ( PUINT pHookCount )
{
  return SH_HookCountEx (pHookCount, 0);
}

MH_STATUS
WINAPI
SH_HookTargetEx ( UINT    hook_slot,
                  LPVOID *ppTarget,
                  UINT    idx )
{
  if (hook_slot > g_hooks [idx].size)
    return MH_ERROR_NOT_CREATED;

  *ppTarget = g_hooks [idx].pItems [hook_slot].pTarget;

  return MH_OK;
}

MH_STATUS
WINAPI
SH_HookTarget ( UINT    hook_slot,
                LPVOID *ppTarget )
{
  return SH_HookTargetEx (hook_slot, ppTarget, 0);
}