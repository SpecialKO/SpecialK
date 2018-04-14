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

#include "../../include/MinHook/MinHook.h"
#include "buffer.h"
#include "trampoline.h"

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

// Suspended threads for Freeze()/Unfreeze().
typedef struct _FROZEN_THREADS
{
  LPDWORD pItems;         // Data heap
  UINT    capacity;       // Size of allocated data heap, items
  UINT    size;           // Actual number of data items
  DWORD   priority;       // Original thread priority
} FROZEN_THREADS, *PFROZEN_THREADS;

//-------------------------------------------------------------------------
// Global Variables:
//-------------------------------------------------------------------------

// Spin lock flag for EnterSpinLock()/LeaveSpinLock().
volatile LONG   g_isLocked = FALSE;

// Private heap handle. If not NULL, this library is initialized.
         HANDLE g_hHeap    = NULL;

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
EnterSpinLock (VOID)
{
  SIZE_T spinCount = 0;

  // Wait until the flag is FALSE.
  while (InterlockedCompareExchange (&g_isLocked, TRUE, FALSE) != FALSE)
  {
    // No need to generate a memory barrier here, since InterlockedCompareExchange()
    // generates a full memory barrier itself.

    // Prevent the loop from being too busy.
    if (spinCount < 32)
        ;
    else
    {
      spinCount = 0;
      SleepEx (1, TRUE);
    }

    spinCount++;
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
         (g_hooks [idx].capacity * 2) * sizeof (HOOK_ENTRY)
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
    BOOL        enable;
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
#pragma pack (push,8)
#define NT_SUCCESS(Status)                      ((NTSTATUS)(Status) >= 0)
#define STATUS_SUCCESS                          0
#define STATUS_INFO_LENGTH_MISMATCH             ((NTSTATUS)0xC0000004L)
#define SystemProcessAndThreadInformation       5

typedef LONG       NTSTATUS;
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
    FILETIME     ftKernelTime;   // 100 nsec units
    FILETIME     ftUserTime;     // 100 nsec units
    FILETIME     ftCreateTime;   // relative to 01-01-1601
    ULONG        dWaitTime;
#ifdef _WIN64
    DWORD32      dwPaddingFor64Bit;
#endif
    PVOID        pStartAddress;
    CLIENT_ID    Cid;           // process/thread ids
    KPRIORITY    dPriority;
    LONG         dBasePriority;
    ULONG        dContextSwitches;
    ULONG        dThreadState;  // 2=running, 5=waiting
    KWAIT_REASON WaitReason;

    // Not even needed if correct packing is used, but let's just make this
    //   obvious since it's easy to overlook!
    DWORD32      dwPaddingEveryoneGets;
} SYSTEM_THREAD,             *PSYSTEM_THREAD;
#define SYSTEM_THREAD_ sizeof (SYSTEM_THREAD)
// -----------------------------------------------------------------
typedef struct _SYSTEM_PROCESS     // common members
{
    ULONG          dNext;
    ULONG          dThreadCount;
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

typedef enum _SYSTEM_INFORMATION_CLASS {
    SystemBasicInformation                                = 0,
    SystemProcessorInformation                            = 1,
    SystemPerformanceInformation                          = 2,
    SystemTimeOfDayInformation                            = 3,
    SystemPathInformation                                 = 4,
    SystemProcessInformation                              = 5,
    SystemCallCountInformation                            = 6,
    SystemDeviceInformation                               = 7,
    SystemProcessorPerformanceInformation                 = 8,
    SystemFlagsInformation                                = 9,
    SystemCallTimeInformation                             = 10,
    SystemModuleInformation                               = 11,
    SystemLocksInformation                                = 12,
    SystemStackTraceInformation                           = 13,
    SystemPagedPoolInformation                            = 14,
    SystemNonPagedPoolInformation                         = 15,
    SystemHandleInformation                               = 16,
    SystemObjectInformation                               = 17,
    SystemPageFileInformation                             = 18,
    SystemVdmInstemulInformation                          = 19,
    SystemVdmBopInformation                               = 20,
    SystemFileCacheInformation                            = 21,
    SystemPoolTagInformation                              = 22,
    SystemInterruptInformation                            = 23,
    SystemDpcBehaviorInformation                          = 24,
    SystemFullMemoryInformation                           = 25,
    SystemLoadGdiDriverInformation                        = 26,
    SystemUnloadGdiDriverInformation                      = 27,
    SystemTimeAdjustmentInformation                       = 28,
    SystemSummaryMemoryInformation                        = 29,
    SystemMirrorMemoryInformation                         = 30,
    SystemPerformanceTraceInformation                     = 31,
    SystemObsolete0                                       = 32,
    SystemExceptionInformation                            = 33,
    SystemCrashDumpStateInformation                       = 34,
    SystemKernelDebuggerInformation                       = 35,
    SystemContextSwitchInformation                        = 36,
    SystemRegistryQuotaInformation                        = 37,
    SystemExtendedServiceTableInformation                 = 38,
    SystemPrioritySeparation                              = 39,
    SystemVerifierAddDriverInformation                    = 40,
    SystemVerifierRemoveDriverInformation                 = 41,
    SystemProcessorIdleInformation                        = 42,
    SystemLegacyDriverInformation                         = 43,
    SystemCurrentTimeZoneInformation                      = 44,
    SystemLookasideInformation                            = 45,
    SystemTimeSlipNotification                            = 46,
    SystemSessionCreate                                   = 47,
    SystemSessionDetach                                   = 48,
    SystemSessionInformation                              = 49,
    SystemRangeStartInformation                           = 50,
    SystemVerifierInformation                             = 51,
    SystemVerifierThunkExtend                             = 52,
    SystemSessionProcessInformation                       = 53,
    SystemLoadGdiDriverInSystemSpace                      = 54,
    SystemNumaProcessorMap                                = 55,
    SystemPrefetcherInformation                           = 56,
    SystemExtendedProcessInformation                      = 57,
    SystemRecommendedSharedDataAlignment                  = 58,
    SystemComPlusPackage                                  = 59,
    SystemNumaAvailableMemory                             = 60,
    SystemProcessorPowerInformation                       = 61,
    SystemEmulationBasicInformation                       = 62,
    SystemEmulationProcessorInformation                   = 63,
    SystemExtendedHandleInformation                       = 64,
    SystemLostDelayedWriteInformation                     = 65,
    SystemBigPoolInformation                              = 66,
    SystemSessionPoolTagInformation                       = 67,
    SystemSessionMappedViewInformation                    = 68,
    SystemHotpatchInformation                             = 69,
    SystemObjectSecurityMode                              = 70,
    SystemWatchdogTimerHandler                            = 71,
    SystemWatchdogTimerInformation                        = 72,
    SystemLogicalProcessorInformation                     = 73,
    SystemWow64SharedInformationObsolete                  = 74,
    SystemRegisterFirmwareTableInformationHandler         = 75,
    SystemFirmwareTableInformation                        = 76,
    SystemModuleInformationEx                             = 77,
    SystemVerifierTriageInformation                       = 78,
    SystemSuperfetchInformation                           = 79,
    SystemMemoryListInformation                           = 80,
    SystemFileCacheInformationEx                          = 81,
    SystemThreadPriorityClientIdInformation               = 82,
    SystemProcessorIdleCycleTimeInformation               = 83,
    SystemVerifierCancellationInformation                 = 84,
    SystemProcessorPowerInformationEx                     = 85,
    SystemRefTraceInformation                             = 86,
    SystemSpecialPoolInformation                          = 87,
    SystemProcessIdInformation                            = 88,
    SystemErrorPortInformation                            = 89,
    SystemBootEnvironmentInformation                      = 90,
    SystemHypervisorInformation                           = 91,
    SystemVerifierInformationEx                           = 92,
    SystemTimeZoneInformation                             = 93,
    SystemImageFileExecutionOptionsInformation            = 94,
    SystemCoverageInformation                             = 95,
    SystemPrefetchPatchInformation                        = 96,
    SystemVerifierFaultsInformation                       = 97,
    SystemSystemPartitionInformation                      = 98,
    SystemSystemDiskInformation                           = 99,
    SystemProcessorPerformanceDistribution                = 100,
    SystemNumaProximityNodeInformation                    = 101,
    SystemDynamicTimeZoneInformation                      = 102,
    SystemCodeIntegrityInformation                        = 103,
    SystemProcessorMicrocodeUpdateInformation             = 104,
    SystemProcessorBrandString                            = 105,
    SystemVirtualAddressInformation                       = 106,
    SystemLogicalProcessorAndGroupInformation             = 107,
    SystemProcessorCycleTimeInformation                   = 108,
    SystemStoreInformation                                = 109,
    SystemRegistryAppendString                            = 110,
    SystemAitSamplingValue                                = 111,
    SystemVhdBootInformation                              = 112,
    SystemCpuQuotaInformation                             = 113,
    SystemNativeBasicInformation                          = 114,
    SystemErrorPortTimeouts                               = 115,
    SystemLowPriorityIoInformation                        = 116,
    SystemBootEntropyInformation                          = 117,
    SystemVerifierCountersInformation                     = 118,
    SystemPagedPoolInformationEx                          = 119,
    SystemSystemPtesInformationEx                         = 120,
    SystemNodeDistanceInformation                         = 121,
    SystemAcpiAuditInformation                            = 122,
    SystemBasicPerformanceInformation                     = 123,
    SystemQueryPerformanceCounterInformation              = 124,
    SystemSessionBigPoolInformation                       = 125,
    SystemBootGraphicsInformation                         = 126,
    SystemScrubPhysicalMemoryInformation                  = 127,
    SystemBadPageInformation                              = 128,
    SystemProcessorProfileControlArea                     = 129,
    SystemCombinePhysicalMemoryInformation                = 130,
    SystemEntropyInterruptTimingInformation               = 131,
    SystemConsoleInformation                              = 132,
    SystemPlatformBinaryInformation                       = 133,
    SystemThrottleNotificationInformation                 = 134,
    SystemPolicyInformation                               = 134,
    SystemHypervisorProcessorCountInformation             = 135,
    SystemDeviceDataInformation                           = 136,
    SystemDeviceDataEnumerationInformation                = 137,
    SystemMemoryTopologyInformation                       = 138,
    SystemMemoryChannelInformation                        = 139,
    SystemBootLogoInformation                             = 140,
    SystemProcessorPerformanceInformationEx               = 141,
    SystemSpare0                                          = 142,
    SystemSecureBootPolicyInformation                     = 143,
    SystemPageFileInformationEx                           = 144,
    SystemSecureBootInformation                           = 145,
    SystemEntropyInterruptTimingRawInformation            = 146,
    SystemPortableWorkspaceEfiLauncherInformation         = 147,
    SystemFullProcessInformation                          = 148,
    SystemKernelDebuggerInformationEx                     = 149,
    SystemBootMetadataInformation                         = 150,
    SystemSoftRebootInformation                           = 151,
    SystemElamCertificateInformation                      = 152,
    SystemOfflineDumpConfigInformation                    = 153,
    SystemProcessorFeaturesInformation                    = 154,
    SystemRegistryReconciliationInformation               = 155,
    SystemEdidInformation                                 = 156,
    SystemManufacturingInformation                        = 157,
    SystemEnergyEstimationConfigInformation               = 158,
    SystemHypervisorDetailInformation                     = 159,
    SystemProcessorCycleStatsInformation                  = 160,
    SystemVmGenerationCountInformation                    = 161,
    SystemTrustedPlatformModuleInformation                = 162,
    SystemKernelDebuggerFlags                             = 163,
    SystemCodeIntegrityPolicyInformation                  = 164,
    SystemIsolatedUserModeInformation                     = 165,
    SystemHardwareSecurityTestInterfaceResultsInformation = 166,
    SystemSingleModuleInformation                         = 167,
    SystemAllowedCpuSetsInformation                       = 168,
    SystemDmaProtectionInformation                        = 169,
    SystemInterruptCpuSetsInformation                     = 170,
    SystemSecureBootPolicyFullInformation                 = 171,
    SystemCodeIntegrityPolicyFullInformation              = 172,
    SystemAffinitizedInterruptProcessorInformation        = 173,
    SystemRootSiloInformation                             = 174,
    SystemCpuSetInformation                               = 175,
    SystemCpuSetTagInformation                            = 176,
    MaxSystemInfoClass                                    = 177,
} SYSTEM_INFORMATION_CLASS;
#pragma pack (pop)

typedef NTSTATUS (WINAPI *NtQuerySystemInformation_pfn)(
  _In_      SYSTEM_INFORMATION_CLASS SystemInformationClass,
  _Inout_   PVOID                    SystemInformation,
  _In_      ULONG                    SystemInformationLength,
  _Out_opt_ PULONG                   ReturnLength
);

static NtQuerySystemInformation_pfn NtQuerySystemInformation = NULL;


PSYSTEM_PROCESS_INFORMATION
ProcessInformation ( PDWORD    pdData,
                     PNTSTATUS pns )
{
  DWORD                      dSize;
  DWORD                      dData = 0;
  NTSTATUS                      ns = STATUS_INVALID_PARAMETER;
  PSYSTEM_PROCESS_INFORMATION pspi = NULL;

  if (NtQuerySystemInformation == NULL)
  {
    HMODULE hModNtDLL =
      GetModuleHandleW (L"Ntdll.dll");

    if (! hModNtDLL) hModNtDLL = LoadLibraryW (L"Ntdll.dll");

    NtQuerySystemInformation =
      (NtQuerySystemInformation_pfn)
      GetProcAddress (hModNtDLL, "NtQuerySystemInformation");
  }

  for (dSize = 4096; (pspi == NULL) && dSize; dSize <<= 1)
  {
    if ((pspi = LocalAlloc (LMEM_FIXED, dSize)) == NULL)
    {
      ns = STATUS_NO_MEMORY;
      break;
    }

    ns = NtQuerySystemInformation ( SystemProcessInformation,
                                      pspi, dSize, &dData );

    if (ns != STATUS_SUCCESS)
    {
      LocalFree (pspi);

      pspi  = NULL;
      dData = 0;

      if (ns != STATUS_INFO_LENGTH_MISMATCH) break;
    }
  }

  if (pdData != NULL) *pdData = dData;
  if (pns    != NULL) *pns    = ns;

  return pspi;
}

volatile LONG* plThreadsCreated = NULL;

void
WINAPI
SH_RegisterThreadCountVar (volatile LONG* pltc)
{
  plThreadsCreated = pltc;
}


LONG                        last_update = -1L;

static
VOID
EnumerateThreads (PFROZEN_THREADS pThreads)
{
#if 1
  DWORD dwPID = GetCurrentProcessId ();
  DWORD dwTID = GetCurrentThreadId  ();

  PSYSTEM_PROCESS_INFORMATION pInfo =
    ProcessInformation (NULL, NULL);

  int i = 0;

  SYSTEM_PROCESS_INFORMATION* pProc = pInfo;
  
  do
  {
    if ((DWORD)((uintptr_t)pProc->UniqueProcessId & 0xFFFFFFFFU) == dwPID)
      break;

    pProc = (SYSTEM_PROCESS_INFORMATION *)((BYTE *)pProc + pProc->dNext);
  } while (pProc->dNext != 0);

  if ((DWORD)((uintptr_t)pProc->UniqueProcessId & 0xFFFFFFFFU) == dwPID)
  {
    int threads = 
      pProc->dThreadCount;

    for (i = 0; i < threads; i++)
    {
      if ( (DWORD)((uintptr_t)pProc->aThreads [i].Cid.UniqueThread  & 0xFFFFFFFFU) == dwTID ||
           (DWORD)((uintptr_t)pProc->aThreads [i].Cid.UniqueProcess & 0xFFFFFFFFU) != dwPID )
        continue;

      if (pThreads->pItems == NULL)
      {
        pThreads->capacity = INITIAL_THREAD_CAPACITY;
        pThreads->pItems   =
              (LPDWORD)HeapAlloc (
                g_hHeap, 0,
                  pThreads->capacity * sizeof (DWORD)
              );

        if (pThreads->pItems == NULL)
          break;
      }

      else if (pThreads->size >= pThreads->capacity)
      {
        LPDWORD p =
            (LPDWORD)HeapReAlloc (
              g_hHeap, 0,
                pThreads->pItems,
               (pThreads->capacity * 2) * sizeof (DWORD)
            );

        if (p == NULL)
          break;

        pThreads->capacity *= 2;
        pThreads->pItems    = p;
      }

      pThreads->pItems [pThreads->size++] =
        (DWORD)((uintptr_t)pProc->aThreads [i].Cid.UniqueThread & 0xFFFFFFFFU);
    }
  }

  LocalFree (pInfo);
#else

  HANDLE hSnapshot =
    CreateToolhelp32Snapshot (TH32CS_SNAPTHREAD, GetProcessId (GetCurrentProcess ()));

  DWORD dwPID = GetCurrentProcessId ();
  DWORD dwTID = GetCurrentThreadId  ();

  if (hSnapshot != INVALID_HANDLE_VALUE)
  {
                        THREADENTRY32 te;
    te.dwSize = sizeof (THREADENTRY32);

    if (Thread32First (hSnapshot, &te))
    {
      do
      {
        if ( te.dwSize >= 
              (FIELD_OFFSET ( THREADENTRY32,
                                th32OwnerProcessID ) + sizeof (DWORD) ) &&
             te.th32OwnerProcessID == dwPID                             &&
             te.th32ThreadID       != dwTID
           )
        {
          if (pThreads->pItems == NULL)
          {
            pThreads->capacity = INITIAL_THREAD_CAPACITY;
            pThreads->pItems   =
                  (LPDWORD)HeapAlloc (
                    g_hHeap, 0,
                      pThreads->capacity * sizeof (DWORD)
                  );

            if (pThreads->pItems == NULL)
              break;
          }

          else if (pThreads->size >= pThreads->capacity)
          {
            LPDWORD p =
                (LPDWORD)HeapReAlloc (
                  g_hHeap, 0,
                    pThreads->pItems,
                   (pThreads->capacity * 2) * sizeof (DWORD)
                );

            if (p == NULL)
              break;

            pThreads->capacity *= 2;
            pThreads->pItems    = p;
          }

          pThreads->pItems [pThreads->size++] = te.th32ThreadID;
        }

        te.dwSize = sizeof (THREADENTRY32);

      } while (Thread32Next (hSnapshot, &te));
    }

    CloseHandle (hSnapshot);
  }
#endif
}

//-------------------------------------------------------------------------
static
VOID
FreezeEx (PFROZEN_THREADS pThreads, UINT pos, UINT action, UINT idx)
{
  HANDLE hThreadSelf = GetCurrentThread ();

  pThreads->pItems   = NULL;
  pThreads->capacity = 0;
  pThreads->size     = 0;
  pThreads->priority = GetThreadPriority (hThreadSelf);

  SetThreadPriority (hThreadSelf, THREAD_PRIORITY_HIGHEST);

  EnumerateThreads (pThreads);

  if (pThreads->pItems != NULL)
  {
    UINT i;

    for (i = 0; i < pThreads->size; ++i)
    {
      HANDLE hThread =
        OpenThread ( THREAD_ACCESS,
                       FALSE,
                         pThreads->pItems [i] );

      if (hThread != NULL)
      {
        SuspendThread      (hThread);
        ProcessThreadIPsEx (hThread, pos, action, idx);
        CloseHandle        (hThread);
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
    UINT i;

    for (i = 0; i < pThreads->size; ++i)
    {
      HANDLE hThread =
        OpenThread ( THREAD_SUSPEND_RESUME,
                       FALSE,
                         pThreads->pItems [i] );

      if (hThread != NULL)
      {
        ResumeThread (hThread);
        CloseHandle  (hThread);
      }
    }

    HeapFree (g_hHeap, 0, pThreads->pItems);
  }

  SetThreadPriority (GetCurrentThread (), pThreads->priority);
}

//-------------------------------------------------------------------------
static
MH_STATUS
EnableHookLLEx (UINT pos, BOOL enable, UINT idx)
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
      pShortJmp->operand       = (UINT8)(0 - (sizeof (JMP_REL_SHORT) + 
                                              sizeof (JMP_REL)));
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
  return EnableHookLLEx (pos, enable, 0);
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
                enable ? ACTION_ENABLE :
                         ACTION_DISABLE, idx );

    for (i = first; i < g_hooks [idx].size; ++i)
    {
      if (g_hooks [idx].pItems [i].isEnabled != enable)
      {
        status = EnableHookLLEx (i, enable, idx);

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
  return EnableAllHooksLLEx (enable, 0);
}

//-------------------------------------------------------------------------
MH_STATUS
WINAPI
MH_Initialize (VOID)
{
  MH_STATUS status = MH_OK;

  EnterSpinLock ();

  if (g_hHeap == NULL)
  {
    g_hHeap = HeapCreate ( 0, sizeof (HOOK_ENTRY)     * (INITIAL_HOOK_CAPACITY*4) + 1 +
                              sizeof (FROZEN_THREADS) * INITIAL_THREAD_CAPACITY   + 1 +
                              16384 * 16, 0 );

    if (g_hHeap != NULL)
    {
      // Initialize the internal function buffer.
      InitializeBuffer ();
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

  if (g_hHeap != NULL)
  {
    for (int i = 0; i < 4; i++)
    {
      status = EnableAllHooksLLEx (FALSE, i);

      if (status == MH_OK)
      {
        // HeapFree is actually not required, but some tools detect a false
        // memory leak without HeapFree.

        HeapFree (g_hHeap, 0, g_hooks [i].pItems);

        g_hooks [i].pItems   = NULL;
        g_hooks [i].capacity = 0;
        g_hooks [i].size     = 0;
      }
    }

    // Free the internal function buffer.
    UninitializeBuffer ();
    HeapDestroy        (g_hHeap);
                        g_hHeap = NULL;
  }
  else
  {
    status = MH_ERROR_NOT_INITIALIZED;
  }

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
              pHook->patchAbove  = ct.patchAbove;
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
      status = EnableAllHooksLLEx (enable, idx);
    }
    else
    {
      FROZEN_THREADS threads;

      UINT pos = FindHookEntryEx (pTarget, idx);

      if (pos != INVALID_HOOK_POS)
      {
        if (g_hooks [idx].pItems [pos].isEnabled != enable)
        {
          FreezeEx (&threads, pos, ACTION_ENABLE, idx);

          status = EnableHookLLEx (pos, enable, idx);

          Unfreeze (&threads);
        }
        else
        {
          status = enable ? MH_ERROR_ENABLED : MH_ERROR_DISABLED;
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
        g_hooks [idx].pItems [i].queueEnable = queueEnable;
    }
    else
    {
      UINT pos = FindHookEntryEx (pTarget, idx);
      if (pos != INVALID_HOOK_POS)
      {
        g_hooks [idx].pItems [pos].queueEnable = queueEnable;
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