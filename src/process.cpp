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

#include <windows.h>
#include <tlhelp32.h>
#include <climits>
#include <cassert>
#include <intsafe.h>
#include <WinUser.h>
#include <SpecialK/utility.h>
#include <SpecialK/utility/lazy_global.h>

#include <SpecialK/log.h>
#include <SpecialK/config.h>

#pragma pack (push,8)
#define NT_SUCCESS(Status)                      ((NTSTATUS)(Status) >= 0)
#define STATUS_SUCCESS                          0
#define STATUS_INFO_LENGTH_MISMATCH             ((NTSTATUS)0xC0000004L)
#define SystemProcessAndThreadInformation       5

typedef _Return_type_success_(return >= 0)
        LONG       NTSTATUS;
typedef NTSTATUS *PNTSTATUS;
typedef LONG       KPRIORITY;

#define INITIAL_THREAD_CAPACITY 128

typedef enum _KTHREAD_STATE
{
  Initialized,
  Ready,
  Running,
  Standby,
  Terminated,
  Waiting,
  Transition,
  DeferredReady,
  GateWaitObsolete,
  WaitingForProcessInSwap,
  MaximumThreadState
} KTHREAD_STATE,
*PKTHREAD_STATE;

typedef enum _KWAIT_REASON
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
  WrKeyedEvent,
  WrTerminated,
  WrProcessInSwap,
  WrCpuRateControl,
  WrCalloutStack,
  WrKernel,
  WrResource,
  WrPushLock,
  WrMutex,
  WrQuantumEnd,
  WrDispatchInt,
  WrPreempted,
  WrYieldExecution,
  WrFastMutex,
  WrGuardedMutex,
  WrRundown,
  WrAlertByThreadId,
  WrDeferredPreempt,
  MaximumWaitReason
} KWAIT_REASON,
*PKWAIT_REASON;

typedef struct _THREAD_ENTRY
{
  DWORD tid;
  BYTE  suspensions;     // Calling SuspendThread once guarantees *nothing!
  BYTE  runstate;        //
  BYTE  waiting;
  BYTE  padding1[1];     //   (*) Multiple calls may be required and an equal
                         //       number of ResumeThread calls is necessary
} THREAD_ENTRY,          // to put things back the way we found them.
*PTHREAD_ENTRY;

typedef struct _THREAD_LIST
{
  PTHREAD_ENTRY pItems;   // Data heap
  UINT          capacity; // Size of allocated data heap, items
  UINT          size;     // Actual number of data items
} THREAD_LIST,
*PTHREAD_LIST;

typedef LONG (NTAPI *NtSuspendProcess_pfn)(_In_ HANDLE ProcessHandle);
                     NtSuspendProcess_pfn
                     NtSuspendProcess = nullptr;

typedef LONG (NTAPI *NtResumeProcess_pfn)(_In_ HANDLE ProcessHandle);
                     NtResumeProcess_pfn
                     NtResumeProcess = nullptr;

typedef struct _CLIENT_ID {
    HANDLE         UniqueProcess;
    HANDLE         UniqueThread;
} CLIENT_ID;

typedef struct _UNICODE_STRING {
    USHORT         Length;
    USHORT         MaximumLength;
    PWSTR          Buffer;
} UNICODE_STRING; //-V677

typedef struct _SYSTEM_THREAD {
    FILETIME     ftKernelTime;
    FILETIME     ftUserTime;
    FILETIME     ftCreateTime;
    ULONG        dWaitTime;
#ifdef _M_AMD64
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

struct SK_NtDllContext
{
  NtQuerySystemInformation_pfn QuerySystemInformation = nullptr;
  NtDelayExecution_pfn         DelayExecution = nullptr;
  NtSuspendProcess_pfn         SuspendProcess = nullptr;
  NtResumeProcess_pfn          ResumeProcess = nullptr;

  HMODULE                      Module     = nullptr;
  HANDLE                       hHeap      = nullptr;
  DWORD                        dwHeapSize = 0UL;
  DWORD                        dwPadding0 = 0UL;
  PSYSTEM_PROCESS_INFORMATION  pSnapshot  = nullptr;
  volatile LONG                _lock      =  0L;

  void lock   (void)
  {
     LONG dwTid      = GetCurrentThreadId ();
    ULONG _SpinCount = 0UL;

    LONG lockCheck =
      InterlockedCompareExchange (&_lock, dwTid, 0);

    // Re-entrant spinlock
    while ( lockCheck != 0 && lockCheck != dwTid )
    {
      if (_SpinCount++ < 17)
        ;
      else
      {
        SK_SleepEx (1, FALSE);
        _SpinCount = 0;
      }

      lockCheck =
        InterlockedCompareExchange (&_lock, dwTid, 0);
    }
  }

  void unlock (void)
  {
    InterlockedExchange (&_lock, 0);
  }

  SK_NtDllContext (void)
  {
    lock ();

    Module =
      LoadLibraryW (L"NtDll.dll");

    if (Module != 0)
    {
      QuerySystemInformation  = (NtQuerySystemInformation_pfn)
        GetProcAddress (Module, "NtQuerySystemInformation");
      SuspendProcess          = (NtSuspendProcess_pfn)
        GetProcAddress (Module, "NtSuspendProcess");
      ResumeProcess           = (NtResumeProcess_pfn)
        GetProcAddress (Module, "NtResumeProcess");
    }

    hHeap =
      HeapCreate (0x0, 1048576, 0);

    if (hHeap != nullptr)
    {
      pSnapshot = (PSYSTEM_PROCESS_INFORMATION)
        HeapAlloc (hHeap, 0x0, 262144);

      dwHeapSize =
        ( pSnapshot != NULL ?
                     262144 : 0 );
    }

    unlock ();
  }

  ~SK_NtDllContext (void)
  {
    lock ();

    if (pSnapshot != nullptr)
    {
      HeapFree (hHeap, 0, pSnapshot);
                          pSnapshot = nullptr;
    }

    dwHeapSize = 0UL;

    if (hHeap != nullptr)
    {
      HeapDestroy (hHeap);
                   hHeap = nullptr;
    }

    if (Module != nullptr)
    {
      QuerySystemInformation = nullptr;
      SuspendProcess         = nullptr;
      ResumeProcess          = nullptr;

      FreeLibrary (Module);
    }

    unlock ();
  }
};

SK_LazyGlobal <SK_NtDllContext> SK_NtDll;

PSYSTEM_PROCESS_INFORMATION
SK_Process_SnapshotNt (void)
{
  if (SK_NtDll->QuerySystemInformation == NULL)
  {
    return nullptr;
  }

  SK_NtDll->lock ();

  RtlSecureZeroMemory ( SK_NtDll->pSnapshot, SK_NtDll->dwHeapSize );

  DWORD                      dSize = 0;
  DWORD                      dData = 0;
  PSYSTEM_PROCESS_INFORMATION pspi = NULL;

  NTSTATUS                      ns =
    SK_NtDll->QuerySystemInformation ( SystemProcessInformation,
                                       SK_NtDll->pSnapshot,
                                       SK_NtDll->dwHeapSize, &dData );

  // Memory was not filled.
  if (ns != STATUS_SUCCESS)
  {
    for (  dSize = SK_NtDll->dwHeapSize ;
          (pspi == NULL) && dSize  != 0 ;
                            dSize <<= 1  )
    {
      if ( ( pspi = (PSYSTEM_PROCESS_INFORMATION)
                    HeapReAlloc ( SK_NtDll->hHeap,     0x0,
                                  SK_NtDll->pSnapshot, dSize ) ) == NULL )
      {
        ns = STATUS_NO_MEMORY;
        break;
      }

      SK_NtDll->dwHeapSize = dSize;
      SK_NtDll->pSnapshot  = pspi;

      ns = SK_NtDll->QuerySystemInformation ( SystemProcessInformation,
                                                pspi, dSize, &dData );

      if (ns != STATUS_SUCCESS)
      {
        pspi  = NULL;
        dData = 0;

        if (ns != STATUS_INFO_LENGTH_MISMATCH) break;
      }
    }
  }

  SK_NtDll->unlock ();

  if (dData != 0)
    return SK_NtDll->pSnapshot;

  return NULL;
}

VOID
SK_Process_EnumerateThreads (PTHREAD_LIST pThreads, DWORD dwPID)
{
  SK_NtDll->lock ();

  PSYSTEM_PROCESS_INFORMATION pProc = nullptr;

  if (SK_NtDll->pSnapshot == nullptr)
  {
    pProc =
      SK_Process_SnapshotNt ();
  }

  else
  {
    pProc = SK_NtDll->pSnapshot;
  }

  SK_ReleaseAssert (pProc != nullptr);

  if (pProc == nullptr)
  {
    SK_NtDll->unlock ();
    return;
  }

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
      if ( (DWORD)((uintptr_t)pProc->aThreads [i].Cid.UniqueProcess & 0xFFFFFFFFU) != dwPID )
        continue;

      if (pThreads->pItems == NULL)
      {
        pThreads->capacity = INITIAL_THREAD_CAPACITY;
        pThreads->pItems   =
              (PTHREAD_ENTRY)HeapAlloc (
                SK_NtDll->hHeap, 0,
                  pThreads->capacity * sizeof (THREAD_ENTRY)
              );

        if (pThreads->pItems == NULL)
          break;
      }

      else if (pThreads->size >= pThreads->capacity)
      {
        PTHREAD_ENTRY p =
            (PTHREAD_ENTRY)HeapReAlloc (
              SK_NtDll->hHeap, 0,
                pThreads->pItems,
               (pThreads->capacity << 1) * sizeof (THREAD_ENTRY)
            );

        if (p == NULL)
          break;

        pThreads->capacity <<= 1;
        pThreads->pItems     = p;
      }

      pThreads->pItems [pThreads->size].tid =
        (DWORD)((uintptr_t)pProc->aThreads [i].Cid.UniqueThread & 0xFFFFFFFFU);
      pThreads->pItems [pThreads->size].runstate =
        (BYTE)(pProc->aThreads [i].dThreadState & 0xFFUL);
      pThreads->pItems [pThreads->size++].waiting =
        (BYTE)(pProc->aThreads [i].WaitReason & 0xFFUL);
    }
  }

  SK_NtDll->unlock ();
}

void
SK_Process_Snapshot (void)
{
  SK_Process_SnapshotNt ();
}

bool
SK_Process_IsSuspended (DWORD dwPid)
{
  SK_NtDll->lock ();

  bool state = false;

  THREAD_LIST                   threads   = { };
  SK_Process_EnumerateThreads (&threads, dwPid);

  PTHREAD_ENTRY pThread =
    threads.pItems;

  for ( UINT thread = 0;
             thread < threads.capacity;
           ++thread )
  {
    if ( pThread->runstate != Waiting ||
         pThread->waiting  != Suspended )
    {
      state = false;
      break;
    }
  }

  if (threads.pItems != nullptr)
  {
    HeapFree (SK_NtDll->hHeap, 0x0, threads.pItems);
  }

  SK_NtDll->unlock ();

  return state;
}

bool
SK_Process_Suspend (DWORD dwPid)
{
  if (SK_NtDll->SuspendProcess == nullptr)
    return false;

  SK_AutoHandle hProcess (
    OpenProcess ( PROCESS_SUSPEND_RESUME,
                    FALSE, dwPid )
  );

  if ((intptr_t)hProcess.m_h > 0)
  {
    return
      NT_SUCCESS (
        SK_NtDll->SuspendProcess (hProcess.m_h)
      );
  }

  return false;
}

bool
SK_Process_Resume (DWORD dwPid)
{
  if (SK_NtDll->ResumeProcess == nullptr)
    return false;

  SK_AutoHandle hProcess (
    OpenProcess ( PROCESS_SUSPEND_RESUME,
                    FALSE, dwPid )
  );

  if ((intptr_t)hProcess.m_h > 0)
  {
    return
      NT_SUCCESS (
        SK_NtDll->ResumeProcess (hProcess.m_h)
      );
  }

  return false;
}