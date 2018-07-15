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
#include <SpecialK/widgets/widget.h>
#include <SpecialK/utility.h>
#include <SpecialK/thread.h>
#include <SpecialK/diagnostics/memory.h>
#include <SpecialK/diagnostics/modules.h>
#include <SpecialK/diagnostics/debug_utils.h>
#include <SpecialK/diagnostics/crash_handler.h>
#include <SpecialK/performance/gpu_monitor.h>
#include <SpecialK/control_panel.h>

#include  <concurrent_unordered_map.h>

#include <processthreadsapi.h>

#pragma pack(push,8)
typedef LONG NTSTATUS;
typedef      NTSTATUS (WINAPI *NtQueryInformationThread_pfn)(HANDLE,/*THREADINFOCLASS*/LONG,DWORD_PTR*,ULONG,PULONG);

#define ThreadQuerySetWin32StartAddress 9

enum THREAD_INFORMATION_CLASS_EX {
//ThreadMemoryPriority      = 0,
//ThreadAbsoluteCpuPriority = 1,
  ThreadDynamicCodePolicy   = 2,
  ThreadPowerThrottling     = 3,
};

typedef struct _THREAD_POWER_THROTTLING_STATE {
  ULONG Version;
  ULONG ControlMask;
  ULONG StateMask;
} THREAD_POWER_THROTTLING_STATE, *PTHREAD_POWER_THROTTLING_STATE;

#define THREAD_POWER_THROTTLING_CURRENT_VERSION 1
#define THREAD_POWER_THROTTLING_EXECUTION_SPEED 1
#define THREAD_POWER_THROTTLING_VALID_FLAGS 1

typedef BOOL (WINAPI *GetThreadInformation_pfn)(HANDLE, THREAD_INFORMATION_CLASS_EX, LPVOID, DWORD);
typedef BOOL (WINAPI *SetThreadInformation_pfn)(HANDLE, THREAD_INFORMATION_CLASS_EX, LPVOID, DWORD);

// Windows 8+
GetThreadInformation_pfn k32GetThreadInformation = nullptr;
SetThreadInformation_pfn k32SetThreadInformation = nullptr;


typedef enum _PROCESS_INFORMATION_CLASS_FULL {
  ProcessBasicInformation,
  ProcessQuotaLimits,
  ProcessIoCounters,
  ProcessVmCounters,
  ProcessTimes,
  ProcessBasePriority,
  ProcessRaisePriority,
  ProcessDebugPort,
  ProcessExceptionPort,
  ProcessAccessToken,
  ProcessLdtInformation,
  ProcessLdtSize,
  ProcessDefaultHardErrorMode,
  ProcessIoPortHandlers,          // Note: this is kernel mode only
  ProcessPooledUsageAndLimits,
  ProcessWorkingSetWatch,
  ProcessUserModeIOPL,
  ProcessEnableAlignmentFaultFixup,
  ProcessPriorityClass,
  ProcessWx86Information,
  ProcessHandleCount,
  ProcessAffinityMask,
  ProcessPriorityBoost,
  ProcessDeviceMap,
  ProcessSessionInformation,
  ProcessForegroundInformation,
  ProcessWow64Information,
  ProcessImageFileName,
  ProcessLUIDDeviceMapsEnabled,
  ProcessBreakOnTermination,
  ProcessDebugObjectHandle,
  ProcessDebugFlags,
  ProcessHandleTracing,
  ProcessIoPriority,
  ProcessExecuteFlags,
  ProcessTlsInformation,
  ProcessCookie,
  ProcessImageInformation,
  ProcessCycleTime,
  ProcessPagePriority,
  ProcessInstrumentationCallback,
  ProcessThreadStackAllocation,
  ProcessWorkingSetWatchEx,
  ProcessImageFileNameWin32,
  ProcessImageFileMapping,
  ProcessAffinityUpdateMode,
  ProcessMemoryAllocationMode,
  ProcessGroupInformation,
  ProcessTokenVirtualizationEnabled,
  ProcessConsoleHostProcess,
  ProcessWindowInformation,
  MaxProcessInfoClass             // MaxProcessInfoClass should always be the last enum
} PROCESS_INFORMATION_CLASS_FULL, *PPROCESS_INFORMATION_CLASS_FULL;

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

typedef NTSTATUS (WINAPI *NtQuerySystemInformation_pfn)(
  _In_      SYSTEM_INFORMATION_CLASS SystemInformationClass,
  _Inout_   PVOID                    SystemInformation,
  _In_      ULONG                    SystemInformationLength,
  _Out_opt_ PULONG                   ReturnLength
);

static NtQuerySystemInformation_pfn NtQuerySystemInformation = nullptr;

typedef struct _PEB {
  BYTE  Reserved1 [2];
  BYTE  BeingDebugged;
  BYTE  Reserved2 [229];
  PVOID Reserved3 [59];
  ULONG SessionId;
} PEB, *PPEB;

typedef struct _PROCESS_BASIC_INFORMATION
{
  NTSTATUS  ExitStatus;
  PPEB      PebBaseAddress;
  ULONG_PTR AffinityMask;
  KPRIORITY BasePriority;
  HANDLE    UniqueProcessId;
  HANDLE    InheritedFromUniqueProcessId;
} PROCESS_BASIC_INFORMATION, *PPROCESS_BASIC_INFORMATION;

typedef NTSTATUS (NTAPI *NtQueryInformationProcess_pfn)(
__in                                   HANDLE                         ProcessHandle,
__in                                   PROCESS_INFORMATION_CLASS_FULL ProcessInformationClass,
__out_bcount(ProcessInformationLength) PVOID                          ProcessInformation,
__in                                   ULONG                          ProcessInformationLength,
__out_opt                              PULONG                         ReturnLength );

NtQueryInformationProcess_pfn NtQueryInformationProcess = nullptr;

extern iSK_INI* osd_ini;

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
#pragma pack(pop)

struct SKWG_Thread_Entry
{
  HANDLE hThread;
  DWORD  dwTid;

  struct runtimes_s
  {
    FILETIME created;
    FILETIME exited;
    FILETIME user;
    FILETIME kernel;

    long double percent_user;
    long double percent_kernel;

    struct
    {
      FILETIME user;
      FILETIME kernel;
    } snapshot;
  } runtimes;

  // Last time percentage was non-zero; used to hide inactive threads
  DWORD last_nonzero;
  bool  exited;

  bool  power_throttle;
  DWORD orig_prio;

  WaitReason  wait_reason  = WaitReason::NotWaiting;
  ThreadState thread_state = ThreadState::Running;

  bool         self_titled;
  std::wstring name;
};

bool
SK_Thread_HasCustomName (DWORD dwTid);

std::map <DWORD,    SKWG_Thread_Entry*> SKWG_Threads;
std::map <LONGLONG, SKWG_Thread_Entry*> SKWG_Ordered_Threads;

PSYSTEM_PROCESS_INFORMATION
ProcessInformation ( PDWORD    pdData,
                     PNTSTATUS pns )
{
  if (NtQuerySystemInformation == nullptr)
  {
    HMODULE hModNtDLL =
      GetModuleHandleW (L"Ntdll.dll");

    if (! hModNtDLL) hModNtDLL = SK_Modules.LoadLibrary (L"Ntdll.dll");

    NtQuerySystemInformation =
      (NtQuerySystemInformation_pfn)
      GetProcAddress (hModNtDLL, "NtQuerySystemInformation");
  }

  if (! NtQuerySystemInformation)
    return nullptr;

  SK_TLS* pTLS =
    SK_TLS_Bottom ();

  if (! pTLS) return nullptr;

  size_t                       dSize;
  DWORD                        dData = 0;
  NTSTATUS                        ns = STATUS_INVALID_PARAMETER;

  if (pTLS->local_scratch.NtQuerySystemInformation.len  < 4096)
      pTLS->local_scratch.NtQuerySystemInformation.alloc (4096, true);

  void* pspi = nullptr;

  for ( dSize = pTLS->local_scratch.NtQuerySystemInformation.len;
             (pspi == nullptr) && dSize;
                                  dSize <<= 1 )
  {
    pTLS->local_scratch.NtQuerySystemInformation.alloc (dSize, true);

    if (pTLS->local_scratch.NtQuerySystemInformation.len < dSize)
    {
      ns = STATUS_NO_MEMORY;
      break;
    }

    dSize =
      pTLS->local_scratch.NtQuerySystemInformation.len;

    pspi  = pTLS->local_scratch.NtQuerySystemInformation.data;

    ns = NtQuerySystemInformation ( SystemProcessInformation,
                                      (PSYSTEM_PROCESS_INFORMATION)pspi,
                                        (DWORD)dSize,
                                          &dData );

    if (ns != STATUS_SUCCESS)
    {
      dData = 0;
      pspi  = nullptr;

      if (ns != STATUS_INFO_LENGTH_MISMATCH) break;
    }
  }

  if (pdData != nullptr) *pdData = dData;
  if (pns    != nullptr) *pns    = ns;

  return (PSYSTEM_PROCESS_INFORMATION)pspi;
}

#include <process.h>
#include <tlhelp32.h>

#include <SpecialK/log.h>

#define _IMAGEHLP_SOURCE_
#include <dbghelp.h>

#ifdef _WIN64
#define SK_StackWalk          StackWalk64
#define SK_SymLoadModule      SymLoadModule64
#define SK_SymUnloadModule    SymUnloadModule64
#define SK_SymGetModuleBase   SymGetModuleBase64
#define SK_SymGetLineFromAddr SymGetLineFromAddr64
#else
#define SK_StackWalk          StackWalk
#define SK_SymLoadModule      SymLoadModule
#define SK_SymUnloadModule    SymUnloadModule
#define SK_SymGetModuleBase   SymGetModuleBase
#define SK_SymGetLineFromAddr SymGetLineFromAddr
#endif

const char*
SKX_DEBUG_FastSymName (LPCVOID ret_addr)
{
  HMODULE hModSource               = nullptr;
  char    szModName [MAX_PATH + 2] = { };
  HANDLE  hProc                    = SK_GetCurrentProcess ();

  static std::string symbol_name = "";

#ifdef _WIN64
  static DWORD64 last_ip = 0;

  DWORD64  ip       (reinterpret_cast <DWORD64> (ret_addr));
  DWORD64  BaseAddr = 0;
#else
  static DWORD last_ip = 0;

  DWORD    ip       (reinterpret_cast <DWORD> (ret_addr));
  DWORD    BaseAddr = 0;
#endif

  SK_TLS* pTLS =
    SK_TLS_Bottom ();

  if (last_ip != ip)
  {
    last_ip = ip;

    if ( GetModuleHandleEx ( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                             GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
              reinterpret_cast <LPCWSTR> (ip),
                                 &hModSource ) )
    {
      GetModuleFileNameA (hModSource, szModName, MAX_PATH);
    }

    MODULEINFO mod_info = { };

    GetModuleInformation (
      hProc, hModSource, &mod_info, sizeof (mod_info)
    );

#ifdef _WIN64
    BaseAddr = (DWORD64)mod_info.lpBaseOfDll;
#else
    BaseAddr = (DWORD)  mod_info.lpBaseOfDll;
#endif

    char*     szDupName = pTLS->scratch_memory.sym_resolve.alloc (strlen (szModName) + 1);
      strcpy (szDupName, szModName);
    char* pszShortName = szDupName;

    PathStripPathA (pszShortName);


    SK_SymLoadModule ( hProc,
                         nullptr,
                          pszShortName,
                            nullptr,
                              BaseAddr,
                                mod_info.SizeOfImage );

    SYMBOL_INFO_PACKAGE sip = { };

    sip.si.SizeOfStruct = sizeof SYMBOL_INFO;
    sip.si.MaxNameLen   = sizeof sip.name;

    DWORD64 Displacement = 0;

    if ( SymFromAddr ( hProc,
             static_cast <DWORD64> (ip),
                           &Displacement,
                             &sip.si ) )
    {
      DWORD Disp = 0x00UL;

#ifdef _WIN64
      IMAGEHLP_LINE64 ihl              = {                    };
                      ihl.SizeOfStruct = sizeof IMAGEHLP_LINE64;
#else
      IMAGEHLP_LINE   ihl              = {                  };
                      ihl.SizeOfStruct = sizeof IMAGEHLP_LINE;
#endif
      BOOL bFileAndLine =
        SK_SymGetLineFromAddr ( hProc, ip, &Disp, &ihl );

      if (bFileAndLine)
      {
        symbol_name =
          SK_FormatString ( "[%hs] %hs <%hs:%lu>",
                              pszShortName, sip.si.Name, ihl.FileName,ihl.LineNumber );
      }

      else
      {
        symbol_name =
          SK_FormatString ( "[%hs] %hs",
                              pszShortName, sip.si.Name );
      }
    }

    else
    {
      symbol_name = SK_WideCharToUTF8 (SK_MakePrettyAddress (ret_addr));
    }
  }

  return symbol_name.data ();
}

void
SK_ImGui_ThreadCallstack (HANDLE hThread, LARGE_INTEGER userTime, LARGE_INTEGER kernelTime)
{
  static LARGE_INTEGER lastUser   { 0, 0 },
                       lastKernel { 0, 0 };
  static DWORD         lastThreadId  = 0;
  static CONTEXT       last_ctx      = { };
  static bool           new_ctx      = false;

  static std::set <int> special_lines;

  HMODULE hModSource               = nullptr;
  char    szModName [MAX_PATH + 2] = { };
  HANDLE  hProc                    = SK_GetCurrentProcess ();

  CONTEXT thread_ctx              = {             };
          thread_ctx.ContextFlags = CONTEXT_CONTROL;

  GetThreadContext (hThread, &thread_ctx);

#ifdef _WIN64
  DWORD64  ip       = thread_ctx.Rip;
  DWORD64  BaseAddr = 0;

  STACKFRAME64 stackframe = { };
               stackframe.AddrStack.Offset = thread_ctx.Rsp;
               stackframe.AddrFrame.Offset = thread_ctx.Rbp;
#else
  DWORD    ip       = thread_ctx.Eip;
  DWORD    BaseAddr = 0;

  STACKFRAME   stackframe = { };
               stackframe.AddrStack.Offset = thread_ctx.Esp;
               stackframe.AddrFrame.Offset = thread_ctx.Ebp;
#endif

  stackframe.AddrPC.Mode   = AddrModeFlat;
  stackframe.AddrPC.Offset = ip;

  stackframe.AddrStack.Mode = AddrModeFlat;
  stackframe.AddrFrame.Mode = AddrModeFlat;

  std::string top_func = "";

  static std::vector <std::string> file_names;
  static std::vector <std::string> symbol_names;
  static std::vector <std::string> code_lines;

  BOOL       ret   = TRUE;
  static int lines = 0;

  SK_TLS* pTLS =
    SK_TLS_Bottom ();

  if (
#ifndef _WIN64
       last_ctx.Eip        != thread_ctx.Eip || 
#else
       last_ctx.Rip        != thread_ctx.Rip || 
#endif
     /*lastUser.QuadPart   != userTime.QuadPart   || 
       lastKernel.QuadPart != kernelTime.QuadPart || */
       lastThreadId        != GetThreadId (hThread) )
  {
    last_ctx            = thread_ctx;
    lastUser.QuadPart   = userTime.QuadPart;
    lastKernel.QuadPart = kernelTime.QuadPart;
    lastThreadId        = GetThreadId (hThread);

    new_ctx = true;

    file_names.clear   ();
    symbol_names.clear ();
    code_lines.clear   ();

    do
    {
      ip = stackframe.AddrPC.Offset;

      if ( GetModuleHandleEx ( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                               GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                reinterpret_cast <LPCWSTR> (ip),
                                   &hModSource ) )
      {
        GetModuleFileNameA (hModSource, szModName, MAX_PATH);
      }

      MODULEINFO mod_info = { };

      GetModuleInformation (
        hProc, hModSource, &mod_info, sizeof (mod_info)
      );

#ifdef _WIN64
      BaseAddr = (DWORD64)mod_info.lpBaseOfDll;
#else
      BaseAddr = (DWORD)  mod_info.lpBaseOfDll;
#endif

      char*     szDupName = pTLS->scratch_memory.cmd.alloc (strlen (szModName) + 1);
        strcpy (szDupName, szModName);
      char* pszShortName = szDupName;

      PathStripPathA (pszShortName);


      SK_SymLoadModule ( hProc,
                           nullptr,
                            pszShortName,
                              nullptr,
                                BaseAddr,
                                  mod_info.SizeOfImage );

      SYMBOL_INFO_PACKAGE sip = { };

      sip.si.SizeOfStruct = sizeof SYMBOL_INFO;
      sip.si.MaxNameLen   = sizeof sip.name;

      DWORD64 Displacement = 0;

      if ( SymFromAddr ( hProc,
               static_cast <DWORD64> (ip),
                             &Displacement,
                               &sip.si ) )
      {
        DWORD Disp = 0x00UL;

#ifdef _WIN64
        IMAGEHLP_LINE64 ihl              = {                    };
                        ihl.SizeOfStruct = sizeof IMAGEHLP_LINE64;
#else
        IMAGEHLP_LINE   ihl              = {                  };
                        ihl.SizeOfStruct = sizeof IMAGEHLP_LINE;
#endif
        BOOL bFileAndLine =
          SK_SymGetLineFromAddr ( hProc, ip, &Disp, &ihl );

        file_names.emplace_back   (pszShortName);
        symbol_names.emplace_back (sip.si.Name);

        if (bFileAndLine)
        {
          code_lines.emplace_back (
            SK_FormatString ( "<%hs:%lu>",
                                ihl.FileName,
                                  ihl.LineNumber )
          );
        }

        else
        {
          code_lines.emplace_back ("");
        }

        if (top_func == "")
          top_func = sip.si.Name;

        ++lines;
      }

      ret =
        SK_StackWalk ( SK_RunLHIfBitness ( 32, IMAGE_FILE_MACHINE_I386,
                                               IMAGE_FILE_MACHINE_AMD64 ),
                         hProc,
                           hThread,
                             &stackframe,
                               &thread_ctx,
                                 nullptr, nullptr,
                                   nullptr, nullptr );
    } while (ret == TRUE);
  }


  if (file_names.size ( ) > 2)
  {
    extern HMODULE __stdcall SK_GetDLL (void);

    static std::string self_ansi =
      SK_WideCharToUTF8 (SK_GetModuleName (SK_Modules.Self));
    static std::string host_ansi =
      SK_WideCharToUTF8 (SK_GetModuleName (SK_Modules.HostApp));


    ImGui::PushStyleColor (ImGuiCol_Text,   ImColor::HSV (0.15f, 0.95f, 0.99f));
    ImGui::PushStyleColor (ImGuiCol_Border, ImColor::HSV (0.15f, 0.95f, 0.99f));
    ImGui::Separator      (  );
    ImGui::PopStyleColor  (02);

    ImGui::TreePush       ("");
    ImGui::BeginGroup     (  );

    if (new_ctx) special_lines.clear ();

    unsigned int idx = 0;

    for ( const auto& it : file_names )
    {
      if (idx++ == 0 || idx == file_names.size ()) continue;

      ImColor color (0.75f, 0.75f, 0.75f, 1.0f);

      if (! _stricmp (it.c_str (), self_ansi.c_str ()))
      {
        if (new_ctx) special_lines.emplace (idx);
        color = ImColor::HSV (0.244444, 0.85f, 1.0f);
      }

      else if (! _stricmp (it.c_str (), host_ansi.c_str ()))
      {
        if (new_ctx) special_lines.emplace (idx);
        color = ImColor (1.0f, 0.729412f, 0.3411764f);
      }

      ImGui::TextColored (color, "%hs", it.c_str ());
    }
    ImGui::EndGroup   (); ImGui::SameLine ();


    ImGui::BeginGroup ();
    idx = 0;
    for ( auto& it : symbol_names )
    {
      if (idx++ == 0 || idx == symbol_names.size ()) continue;

      ImColor color (0.95f, 0.95f, 0.95f, 1.0f);

      if (special_lines.count (idx))
        color = ImColor::HSV (0.527778f, 0.85f, 1.0f);

      ImGui::TextColored (color, "%hs", it.c_str ());
    }
    ImGui::EndGroup   (); ImGui::SameLine ();


    ImGui::BeginGroup ();
    idx = 0;
    for ( auto& it : code_lines )
    {
      if (idx++ == 0 || idx == code_lines.size ()) continue;

      ImColor color (1.0f, 1.0f, 1.0f, 1.0f);

      if (special_lines.count (idx))
        color = ImColor::HSV (0.1f, 0.85f, 1.0f);

      ImGui::TextColored (color, "%hs", it.c_str ());
    }
    ImGui::EndGroup   ();
    ImGui::TreePop    ();
  }

  new_ctx = false;
}

class SKWG_Thread_Profiler : public SK_Widget
{
public:
  SKWG_Thread_Profiler (void) : SK_Widget ("Thread Profiler")
  {
    SK_ImGui_Widgets.thread_profiler = this;

    setAutoFit (true).setDockingPoint (DockAnchor::West).setClickThrough (true);
  };

  virtual void run (void) override
  {
    SK_RunOnce (
      k32SetThreadInformation =
        (SetThreadInformation_pfn)GetProcAddress (
          GetModuleHandle (L"kernel32"),
                            "SetThreadInformation"
        )
    );

    SK_RunOnce (
      k32GetThreadInformation =
        (GetThreadInformation_pfn)GetProcAddress (
          GetModuleHandle (L"kernel32"),
                            "GetThreadInformation"
        )
    );

    SK_RunOnce (
      NtQueryInformationProcess =
          (NtQueryInformationProcess_pfn)GetProcAddress ( GetModuleHandle (L"NtDll.dll"),
                                                            "NtQueryInformationProcess" )
    );

    // Snapshotting is _slow_, so only do it when a thread has been created...
    extern volatile LONG lLastThreadCreate;
    static          LONG lLastThreadRefresh = 0;

    LONG last = ReadAcquire (&lLastThreadCreate);

    if ((last == lLastThreadRefresh) && (! visible))
    {
      return;
    }

    NTSTATUS nts = STATUS_INVALID_PARAMETER;

    ProcessInformation (nullptr, &nts);

    PSYSTEM_PROCESS_INFORMATION pInfo =
      (PSYSTEM_PROCESS_INFORMATION)SK_TLS_Bottom ()->local_scratch.NtQuerySystemInformation.data;
  
    if (pInfo != nullptr && nts == STATUS_SUCCESS)
    {
      int i = 0;

      const DWORD dwPID = GetCurrentProcessId ();
      const DWORD dwTID = GetCurrentThreadId  ();
  
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
          if ((DWORD)((uintptr_t)pProc->aThreads [i].Cid.UniqueProcess & 0xFFFFFFFFU) != dwPID)
            continue;

          bool new_thread = false;

          DWORD dwLocalTID = (DWORD)((uintptr_t)pProc->aThreads [i].Cid.UniqueThread & 0xFFFFFFFFU);

          if (! SKWG_Threads.count (dwLocalTID))
          {
            new_thread = true;

            SKWG_Thread_Entry::runtimes_s runtimes = { };

            SKWG_Thread_Entry *ptEntry =
              new SKWG_Thread_Entry {
                               INVALID_HANDLE_VALUE,
                                  dwLocalTID,
                                  { { 0 , 0 } , { 0 , 0 },
                                    { 0 , 0 } , { 0 , 0 }, 0.0, 0.0,
                                          { 0 , 0 } },    0, false,
                                            false,  0, WaitReason::NotWaiting,
                                                      ThreadState::Running,
                                            false,
                 SK_Thread_GetName (dwLocalTID)
              };

            SKWG_Threads.insert (
              std::make_pair ( dwLocalTID, ptEntry )
            );
          }

          SKWG_Thread_Entry* ptEnt =
            SKWG_Threads [dwLocalTID];

          ptEnt->wait_reason =
            (WaitReason)pProc->aThreads [i].WaitReason;
          ptEnt->thread_state =
            (ThreadState)pProc->aThreads [i].dThreadState;

          ptEnt->runtimes.created = pProc->aThreads [i].ftCreateTime;
          ptEnt->runtimes.kernel  = pProc->aThreads [i].ftKernelTime;
          ptEnt->runtimes.user    = pProc->aThreads [i].ftUserTime;

          if (new_thread)
          {
            LARGE_INTEGER liCreate;
                          liCreate.HighPart = ptEnt->runtimes.created.dwHighDateTime;
                          liCreate.LowPart  = ptEnt->runtimes.created.dwLowDateTime;

            SKWG_Ordered_Threads [liCreate.QuadPart] =
              ptEnt;
          }
        }

        lLastThreadRefresh =
          last;
      }
    }
  }


  virtual void draw (void) override
  {
    if (! ImGui::GetFont ()) return;


           bool drew_tooltip   = false;
    static bool show_callstack = false;

    auto ThreadMemTooltip = [&](DWORD dwSelectedTid) ->
    void
    {
      if (drew_tooltip)
        return;

      drew_tooltip= true;

      SK_TLS* pTLS =
        SK_TLS_BottomEx (dwSelectedTid);

      if (pTLS != nullptr)
      {
        ImGui::BeginTooltip ();

        if (ReadAcquire64 (&pTLS->memory.global_bytes)  ||
            ReadAcquire64 (&pTLS->memory.local_bytes)   ||
            ReadAcquire64 (&pTLS->memory.virtual_bytes) ||
            ReadAcquire64 (&pTLS->memory.heap_bytes)       )
        {
          ImGui::BeginGroup   ();
          if (ReadAcquire64 (&pTLS->memory.local_bytes))   ImGui::Text ("Local Memory:\t");
          if (ReadAcquire64 (&pTLS->memory.global_bytes))  ImGui::Text ("Global Memory:\t");
          if (ReadAcquire64 (&pTLS->memory.heap_bytes))    ImGui::Text ("Heap Memory:\t");
          if (ReadAcquire64 (&pTLS->memory.virtual_bytes)) ImGui::Text ("Virtual Memory:\t");
          ImGui::EndGroup     ();

          ImGui::SameLine     ();

          ImGui::BeginGroup   ();
          if (ReadAcquire64 (&pTLS->memory.local_bytes))   ImGui::Text ("%ws", SK_File_SizeToStringF (ReadAcquire64 (&pTLS->memory.local_bytes),   2, 3).c_str ());
          if (ReadAcquire64 (&pTLS->memory.global_bytes))  ImGui::Text ("%ws", SK_File_SizeToStringF (ReadAcquire64 (&pTLS->memory.global_bytes),  2, 3).c_str ());
          if (ReadAcquire64 (&pTLS->memory.heap_bytes))    ImGui::Text ("%ws", SK_File_SizeToStringF (ReadAcquire64 (&pTLS->memory.heap_bytes),    2, 3).c_str ());
          if (ReadAcquire64 (&pTLS->memory.virtual_bytes)) ImGui::Text ("%ws", SK_File_SizeToStringF (ReadAcquire64 (&pTLS->memory.virtual_bytes), 2, 3).c_str ());
          ImGui::EndGroup     ();

          ImGui::SameLine     ();

          ImGui::BeginGroup   ();
          if (ReadAcquire64 (&pTLS->memory.local_bytes))   ImGui::Text ("\t(Lifetime)");
          if (ReadAcquire64 (&pTLS->memory.global_bytes))  ImGui::Text ("\t(Lifetime)");
          if (ReadAcquire64 (&pTLS->memory.heap_bytes))    ImGui::Text ("\t(Lifetime)");
          if (ReadAcquire64 (&pTLS->memory.virtual_bytes)) ImGui::Text ("\t(In-Use Now)");
          ImGui::EndGroup     ();
        }

        if ( ReadAcquire64 (&pTLS->disk.bytes_read)    > 0 ||
             ReadAcquire64 (&pTLS->disk.bytes_written) > 0    )
        {
          ImGui::Separator ();

          ImGui::BeginGroup ();
          if (ReadAcquire64 (&pTLS->disk.bytes_read))    ImGui::Text ("File Reads:\t\t\t");
          if (ReadAcquire64 (&pTLS->disk.bytes_written)) ImGui::Text ("File Writes:\t\t\t");
          ImGui::EndGroup   ();

          ImGui::SameLine   ();

          ImGui::BeginGroup ();
          if (ReadAcquire64 (&pTLS->disk.bytes_read))    ImGui::Text ("%ws", SK_File_SizeToStringF (ReadAcquire64 (&pTLS->disk.bytes_read),    2, 3).c_str  ());
          if (ReadAcquire64 (&pTLS->disk.bytes_written)) ImGui::Text ("%ws", SK_File_SizeToStringF (ReadAcquire64 (&pTLS->disk.bytes_written), 2, 3).c_str  ());
          ImGui::EndGroup   ();
        }

        if ( ReadAcquire64 (&pTLS->net.bytes_received) > 0 ||
             ReadAcquire64 (&pTLS->net.bytes_sent)     > 0 )
        {
          ImGui::Separator ();

          ImGui::BeginGroup ();
          if (ReadAcquire64 (&pTLS->net.bytes_sent))     ImGui::Text ("Network Sent:\t");
          if (ReadAcquire64 (&pTLS->net.bytes_received)) ImGui::Text ("Network Received:\t");
          ImGui::EndGroup   ();

          ImGui::SameLine   ();

          ImGui::BeginGroup ();
          if (ReadAcquire64 (&pTLS->net.bytes_sent))     ImGui::Text ("%ws", SK_File_SizeToStringF (ReadAcquire64 (&pTLS->net.bytes_sent),     2, 3).c_str  ());
          if (ReadAcquire64 (&pTLS->net.bytes_received)) ImGui::Text ("%ws", SK_File_SizeToStringF (ReadAcquire64 (&pTLS->net.bytes_received), 2, 3).c_str  ());
          ImGui::EndGroup   ();
        }

        LONG64 steam_calls = ReadAcquire64 (&pTLS->steam.callback_count);

        if ( steam_calls > 0 )
        {
          ImGui::Separator  ();

          ImGui::BeginGroup ();
          ImGui::Text       ("Steam Callbacks:\t");
          ImGui::EndGroup   ();

          ImGui::SameLine   ();

          ImGui::BeginGroup ();
          ImGui::Text       ("%lli", steam_calls);
          ImGui::EndGroup   ();
        }

        LONG exceptions = ReadAcquire (&pTLS->debug.exceptions);

        if (exceptions > 0)
        {
          ImGui::Separator  ();

          ImGui::BeginGroup ();
          ImGui::Text       ("Exceptions Raised:");
          ImGui::EndGroup   ();

          ImGui::SameLine   ();

          ImGui::BeginGroup ();
          ImGui::Text       ("\t%i", exceptions);
          ImGui::EndGroup   ();
        }

        if (pTLS->win32.error_state.code != NO_ERROR)
        {
          ImGui::Separator  ();

          ImGui::BeginGroup ();
          ImGui::Text       ("Win32 Error Code:");
          ImGui::EndGroup   ();

          ImGui::SameLine   ();

          _com_error err (pTLS->win32.error_state.code & 0xFFFF);

          ImGui::BeginGroup ();
          ImGui::Text       ("\t0x%04x (%ws)", (pTLS->win32.error_state.code & 0xFFFF), err.ErrorMessage ());
          ImGui::EndGroup   ();

          ImGui::BeginGroup ();
          ImGui::Text       ("\t\t%hs", SKX_DEBUG_FastSymName (pTLS->win32.error_state.call_site));
          ImGui::EndGroup   ();
        }

        if (show_callstack)
        {
          CHandle hThreadStack (
            OpenThread ( THREAD_ALL_ACCESS, false, dwSelectedTid )
          );

          if (hThreadStack.m_h != INVALID_HANDLE_VALUE)
          {
            FILETIME ftCreate, ftUser,
                     ftKernel, ftExit;

            GetThreadTimes           ( hThreadStack, &ftCreate, &ftExit, &ftKernel, &ftUser );
            SK_ImGui_ThreadCallstack ( hThreadStack, LARGE_INTEGER {       ftUser.dwLowDateTime,
                                                                     (LONG)ftUser.dwHighDateTime   },
                                                     LARGE_INTEGER {       ftKernel.dwLowDateTime,
                                                                     (LONG)ftKernel.dwHighDateTime } );
          }
        }

        ImGui::EndTooltip   ();
      }
    };

           DWORD dwExitCode    = 0;
    static DWORD dwSelectedTid = 0;
    static UINT  uMaxCPU       = 0;

    static bool   show_exited_threads = false;
    static bool   hide_inactive       = true;
    static bool   reset_stats         = true;

    static size_t rebalance_idx       = 0;
    static bool   rebalance           = false;

    bool clear_counters = false;

    //ImGui::BeginChild ("Thread_List",   ImVec2 (0,0), false, ImGuiWindowFlags_NavFlattened | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_HorizontalScrollbar);

    ImGui::BeginGroup ();

    ImGui::Checkbox ("Show Finished", &show_exited_threads);
    ImGui::SameLine ();
    ImGui::Checkbox ("Hide Inactive", &hide_inactive);
    ImGui::SameLine ();
    ImGui::Checkbox ("Reset Stats Every 30 Seconds", &reset_stats);
    ImGui::SameLine ();
    if (ImGui::Button ("Reset Performance Counters"))
      clear_counters = true;
    ImGui::SameLine ();

    if (! rebalance)
    {
      if (ImGui::Button ("Rebalance Threads"))
      {
        rebalance_idx = 0;
        rebalance     = true;
      }

      ImGui::SameLine ();
    }

    ImGui::Checkbox ("Show Callstack Analysis", &show_callstack);

    if (ImGui::IsItemHovered ()) ImGui::SetTooltip ("This feature is experimental and may cause hitching while debug symbols are loaded.");

    ImGui::Separator ();

    ImGui::EndGroup ();

    ImGui::BeginGroup ();
  //ImGui::BeginChildFrame (ImGui::GetID ("Thread_List2"), ImVec2 (0,0), ImGuiWindowFlags_NavFlattened | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_HorizontalScrollbar);

    if (ImGui::BeginPopup ("ThreadInspectPopup"))
    {
      CHandle hSelectedThread;
              hSelectedThread.m_h = INVALID_HANDLE_VALUE;

      if (dwSelectedTid != 0)
      {
        hSelectedThread.m_h =
          OpenThread ( THREAD_QUERY_INFORMATION |
                       THREAD_SET_INFORMATION   |
                       THREAD_SUSPEND_RESUME, FALSE, dwSelectedTid );

        if (GetProcessIdOfThread (hSelectedThread) != GetCurrentProcessId ())
        {
          hSelectedThread.Close ();
          hSelectedThread.m_h = INVALID_HANDLE_VALUE;
        }
      }

      bool active_selection =
        (                     hSelectedThread != INVALID_HANDLE_VALUE &&
          GetExitCodeThread ( hSelectedThread, &dwExitCode )          &&
                                                dwExitCode == STILL_ACTIVE
        );

      if (active_selection)
      {
        ImGui::BeginGroup ();

        bool suspended =
          SKWG_Threads [dwSelectedTid]->wait_reason == WaitReason::Suspended;

        if (GetCurrentThreadId () != GetThreadId (hSelectedThread) )
        {
        if ( ImGui::Button ( suspended ?  "Resume this Thread" :
                                          "Suspend this Thread" ) )
        {
          suspended = (! suspended);

          if (suspended)
          {
            struct suspend_params_s
              { DWORD* pdwTid;
                ULONG  frames; } static params { &dwSelectedTid,
                                                  SK_GetFramesDrawn () };

            static HANDLE hThreadRecovery = INVALID_HANDLE_VALUE;
            static HANDLE hRecoveryEvent  = INVALID_HANDLE_VALUE;

            if (hThreadRecovery == INVALID_HANDLE_VALUE)
            {
              hRecoveryEvent  =
              CreateEvent  (nullptr, FALSE, TRUE, nullptr);
              hThreadRecovery =
              SK_Thread_CreateEx ([](LPVOID user) -> DWORD
              {
                SetCurrentThreadDescription  (L"[SK] Thread Profiler Watchdog Timer");
                SK_Thread_SetCurrentPriority (THREAD_PRIORITY_LOWEST);

                while (true)
                {
                  WaitForSingleObjectEx (hRecoveryEvent, INFINITE, TRUE);

                  extern volatile LONG __SK_DLL_Ending;

                  if (ReadAcquire (&__SK_DLL_Ending))
                    break;

                  DWORD dwTid = *((suspend_params_s *)user)->pdwTid;

                  CHandle hThread__ (
                    OpenThread ( THREAD_SUSPEND_RESUME,
                                   FALSE,
                                     dwTid )
                  );

                  if (SuspendThread (hThread__) != (DWORD)-1)
                  {
                    SleepEx (90, TRUE);

                    if (SK_GetFramesDrawn () <= ((suspend_params_s *)user)->frames)
                    {
                      ResumeThread (hThread__);
                    }
                  }
                }

                CloseHandle (hRecoveryEvent);
                             hRecoveryEvent = INVALID_HANDLE_VALUE;

                SK_Thread_CloseSelf ();

                return 0;
              }, nullptr,
        (LPVOID)&params);
            }

            else if (hRecoveryEvent != INVALID_HANDLE_VALUE)
            {
              params.frames = SK_GetFramesDrawn ();
              params.pdwTid = &dwSelectedTid;

              SetEvent (hRecoveryEvent);
            }
          }

          else
          {
            ResumeThread (hSelectedThread);
          }
        }
        }

        SYSTEM_INFO        sysinfo = { };
        SK_GetSystemInfo (&sysinfo);

        SK_TLS* pTLS =
          SK_TLS_BottomEx (dwSelectedTid);

        if (sysinfo.dwNumberOfProcessors > 1 && pTLS != nullptr)
        {
          ImGui::Separator ();

          for (DWORD_PTR j = 0; j < sysinfo.dwNumberOfProcessors; j++)
          {
            constexpr DWORD_PTR Processor0 = 0x1;

            bool affinity =
              (pTLS->scheduler.affinity_mask & (Processor0 << j)) != 0;

            UINT i =
              SetThreadIdealProcessor (hSelectedThread, MAXIMUM_PROCESSORS);

            bool ideal = (i == j);

            float c_scale = 
              pTLS->scheduler.lock_affinity ? 0.5f : 1.0f;

            ImGui::TextColored ( ideal    ? ImColor (0.5f   * c_scale, 1.0f   * c_scale,   0.5f * c_scale):
                                 affinity ? ImColor (1.0f   * c_scale, 1.0f   * c_scale,   1.0f * c_scale) :
                                            ImColor (0.375f * c_scale, 0.375f * c_scale, 0.375f * c_scale),
                                   "CPU%lu",
                                     j );

            if (ImGui::IsItemClicked () && (! pTLS->scheduler.lock_affinity))
            {
              affinity = (! affinity);

              DWORD_PTR dwAffinityMask =
                pTLS->scheduler.affinity_mask;

                if (affinity) dwAffinityMask |=  (Processor0 << j);
                else          dwAffinityMask &= ~(Processor0 << j);

              SetThreadAffinityMask ( hSelectedThread, dwAffinityMask );
            }

            if (  j < (sysinfo.dwNumberOfProcessors / 2 - 1) ||
                 (j > (sysinfo.dwNumberOfProcessors / 2 - 1) && j != (sysinfo.dwNumberOfProcessors - 1) ) )
            {
              ImGui::SameLine ();
            }
          }

          ImGui::Checkbox ("Prevent changes to affinity", &pTLS->scheduler.lock_affinity);
        }
        ImGui::EndGroup ();
        ImGui::SameLine ();

        ImGui::BeginGroup ();
        ImGui::Spacing    ();
        ImGui::EndGroup   ();

        ImGui::SameLine   ();
        ImGui::BeginGroup ();
        //if (ImGui::Button ("Terminate this Thread")) TerminateThread (hThread_, 0x0);

        int sel = 0;

        DWORD dwPrio =
          GetThreadPriority (hSelectedThread);

        switch (dwPrio)
        {
          case THREAD_PRIORITY_IDLE:          sel = 0; break;
          case THREAD_PRIORITY_LOWEST:        sel = 1; break;
          case THREAD_PRIORITY_BELOW_NORMAL:  sel = 2; break;
          case THREAD_PRIORITY_NORMAL:        sel = 3; break;
          case THREAD_PRIORITY_ABOVE_NORMAL:  sel = 4; break;
          case THREAD_PRIORITY_HIGHEST:       sel = 5; break;
          case THREAD_PRIORITY_TIME_CRITICAL: sel = 6; break;
          default:                            sel = 7; break;
        }

        if (sel < 7)
        {
          if (ImGui::Combo ( "Thread Priority", &sel,
                             "Idle\0Lowest\0Below Normal\0Normal\0"
                             "Above Normal\0Highest\0Time Critical\0\0" ))
          {
            switch (sel)
            {
              case 0: dwPrio = (DWORD)THREAD_PRIORITY_IDLE;          break;
              case 1: dwPrio = (DWORD)THREAD_PRIORITY_LOWEST;        break;
              case 2: dwPrio = (DWORD)THREAD_PRIORITY_BELOW_NORMAL;  break;
              case 3: dwPrio = (DWORD)THREAD_PRIORITY_NORMAL;        break;
              case 4: dwPrio = (DWORD)THREAD_PRIORITY_ABOVE_NORMAL;  break;
              case 5: dwPrio = (DWORD)THREAD_PRIORITY_HIGHEST;       break;
              case 6: dwPrio = (DWORD)THREAD_PRIORITY_TIME_CRITICAL; break;
              default:                                               break;
            }

            SetThreadPriority (hSelectedThread, dwPrio);
          }
        }

        BOOL bDisableBoost = FALSE;

        if (GetThreadPriorityBoost (hSelectedThread, &bDisableBoost))
        {
          bool boost = (! bDisableBoost);
          if (ImGui::Checkbox ("Enable Dynamic Boost", &boost))
          {
            SetThreadPriorityBoost (hSelectedThread, ! boost);
          }
        }

        //if (k32SetThreadInformation != nullptr)
        //{
        //  THREAD_POWER_THROTTLING_STATE pthrottle = {
        //    0, 0, 0
        //  };
        //
        //  k32GetThreadInformation ( hSelectedThread,
        //    ThreadPowerThrottling, &pthrottle,
        //                    sizeof (pthrottle) );
        //  {
        //    bool throttle = 
        //      ( pthrottle.StateMask == THREAD_POWER_THROTTLING_EXECUTION_SPEED );
        //
        //    if (ImGui::Checkbox ("Enable Power Throttling", &throttle))
        //    {
        //      pthrottle.StateMask = throttle ?
        //        THREAD_POWER_THROTTLING_EXECUTION_SPEED : 0;
        //
        //      pthrottle.Version     = THREAD_POWER_THROTTLING_CURRENT_VERSION;
        //      pthrottle.ControlMask = THREAD_POWER_THROTTLING_EXECUTION_SPEED;
        //
        //      k32SetThreadInformation ( hSelectedThread,
        //        ThreadPowerThrottling, &pthrottle,
        //                        sizeof (pthrottle) );
        //    }
        //  }
        //}

        bool& throttle =
          SKWG_Threads [dwSelectedTid]->power_throttle;

        if (! throttle)
          SKWG_Threads [dwSelectedTid]->orig_prio = dwPrio;

        if (ImGui::Checkbox ("Enable Power Throttling", &throttle))
        {
          SetThreadPriority ( hSelectedThread, throttle ? THREAD_MODE_BACKGROUND_BEGIN | THREAD_PRIORITY_IDLE :
                                                          THREAD_MODE_BACKGROUND_END );

          if (! throttle)
          {
            SetThreadPriority ( hSelectedThread, SKWG_Threads [dwSelectedTid]->orig_prio );
          }
        }

        ImGui::EndGroup   ();
      }

      ImGui::EndPopup ();
    }

    DWORD dwNow = timeGetTime ();

    auto IsThreadNonIdle = [&](SKWG_Thread_Entry& tent) ->
    bool
    {
      const DWORD IDLE_PERIOD = 1500UL;

      if (tent.exited && (! show_exited_threads))
        return false;

      if (! hide_inactive)
        return true;

      if (tent.last_nonzero > dwNow - IDLE_PERIOD)
        return true;

      return false;
    };

    ImGui::BeginGroup ();
    for ( auto& it : SKWG_Ordered_Threads )
    {
      if (! it.second->exited)
      {
        it.second->hThread =
          OpenThread ( THREAD_ALL_ACCESS, FALSE, it.second->dwTid );
      }

      if (! IsThreadNonIdle (*it.second)) continue;

      DWORD  dwErrorCode = 0;

      if (! it.second->exited)
      {
        if (! GetExitCodeThread (it.second->hThread, &dwExitCode))
        {
          dwErrorCode = GetLastError ();

          if (dwErrorCode != ERROR_INVALID_HANDLE)
              dwExitCode   = STILL_ACTIVE;

          else
          {
             it.second->hThread =         0;
             dwExitCode         = (DWORD)-1;
          }
        }
      } else { dwExitCode =  0; }


      if (SKWG_Threads [it.second->dwTid]->wait_reason == WaitReason::Suspended)
        ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.85f, 0.95f, 0.99f));
      else if (dwExitCode == STILL_ACTIVE)
        ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.3f, 0.95f, 0.99f));
      else
      {
        it.second->exited = true;
        ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.15f, 0.95f, 0.99f));
      }

      bool selected = false;
      if (ImGui::Selectable (SK_FormatString ("Thread %x###thr_%x", it.second->dwTid,it.second->dwTid).c_str (), &selected))
      {
        ImGui::PopStyleColor     ();
        ImGui::PushStyleColor    (ImGuiCol_Text, ImColor::HSV (0.5f, 0.99f, 0.999f));

        // We sure as hell cannot suspend the thread that's drawing the UI! :)
        if ( dwExitCode            == STILL_ACTIVE )
        {
          ImGui::OpenPopup ("ThreadInspectPopup");
        }

        dwSelectedTid =
          it.second->dwTid;

        ImGui::SetNextWindowSize (ImVec2 (-1.0f, -1.0f), ImGuiSetCond_Always);
      }

      ImGui::PopStyleColor ();

      if (ImGui::IsItemHovered ())
      { if ( ! dwErrorCode ) ThreadMemTooltip (it.second->dwTid);
        else                 ImGui::SetTooltip ("Error Code: %lu", dwErrorCode);
      }

    }
    ImGui::EndGroup   (); ImGui::SameLine ();


    ImGui::BeginGroup ( );

    static std::vector <SKWG_Thread_Entry *> rebalance_list;

    if (rebalance && rebalance_list.empty ())
    {
      for ( auto& it : SKWG_Ordered_Threads )
      {
        SK_TLS* pTLS =
          SK_TLS_BottomEx (it.second->dwTid);

        if ((! pTLS)) continue;

        rebalance_list.push_back (it.second);
      }

      std::sort ( rebalance_list.begin (), rebalance_list.end (),
           [](SKWG_Thread_Entry *lh, SKWG_Thread_Entry *rh) ->
           bool
           {
             LARGE_INTEGER lil = { (DWORD)lh->runtimes.user.dwLowDateTime,
                                    (LONG)lh->runtimes.user.dwHighDateTime },
                           lir = { (DWORD)rh->runtimes.user.dwLowDateTime,
                                    (LONG)rh->runtimes.user.dwHighDateTime };

             return lil.QuadPart < lir.QuadPart;
           }
      );
    }

    if (rebalance)
    {
      size_t idx = 0;

      for ( auto& it : rebalance_list )
      {
        if (rebalance_idx == idx)
        {
          SK_TLS* pTLS =
            SK_TLS_BottomEx (it->dwTid);

          DWORD pnum = 
          SetThreadIdealProcessor (it->hThread, MAXIMUM_PROCESSORS);

          if ( pnum != (DWORD)-1 && dwExitCode == STILL_ACTIVE )
          {
            static SYSTEM_INFO
                sysinfo = { };
            if (sysinfo.dwNumberOfProcessors == 0)
            {
              SK_GetSystemInfo (&sysinfo);
            }

            static DWORD ideal = 0;

            DWORD_PTR dwMask =
              pTLS->scheduler.affinity_mask;

            if (pnum != ideal && ( (dwMask >> pnum)  & 0x1)
                              && ( (dwMask >> ideal) & 0x1))
            {
              if ( SetThreadIdealProcessor ( it->hThread, ideal ) != -1 )
                pnum = ideal;
            }

            if (pnum == ideal || (! ((dwMask >> ideal) & 0x1)))
            {
              if (pnum == ideal)
                rebalance_idx++;

              ideal++;

              if (ideal >= sysinfo.dwNumberOfProcessors)
                ideal = 0;
            }
          }

          else ++rebalance_idx;
        }

        // No more rebalancing to do!
        if ( rebalance &&
             rebalance_idx >= rebalance_list.size () )
        {
          rebalance = false;
        }

        ++idx;
      }

      if (! rebalance) rebalance_list.clear ();
    }


    for ( auto& it : SKWG_Ordered_Threads )
    {
      if (! IsThreadNonIdle (*it.second)) continue;

      if (! it.second->self_titled)
      {
        it.second->name        = SK_Thread_GetName       (it.second->dwTid);
        it.second->self_titled = SK_Thread_HasCustomName (it.second->dwTid);
      }

      if (it.second->self_titled)
        ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.572222, 0.63f, 0.95f));
      else
        ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.472222, 0.23f, 0.91f));

      if (it.second->name.length () > 1)
        ImGui::Text ("%ws", it.second->name.c_str ());
      else
      {
        NTSTATUS  ntStatus;
        DWORD_PTR pdwStartAddress; 

        static NtQueryInformationThread_pfn NtQueryInformationThread =
          (NtQueryInformationThread_pfn)GetProcAddress ( GetModuleHandle (L"NtDll.dll"),
                                                           "NtQueryInformationThread" );

        HANDLE hThread = it.second->hThread;

        ntStatus =
          NtQueryInformationThread (  hThread, ThreadQuerySetWin32StartAddress,
                                     &pdwStartAddress, sizeof (DWORD), NULL );

        char  thread_name [512] = { };
        char  szSymbol    [256] = { };
        ULONG ulLen             = 191;

        SK::Diagnostics::CrashHandler::InitSyms ();

        ULONG
        SK_GetSymbolNameFromModuleAddr ( HMODULE hMod,   uintptr_t addr,
                                         char*   pszOut, ULONG     ulLen );

        ulLen = SK_GetSymbolNameFromModuleAddr (
                SK_GetModuleFromAddr ((LPCVOID)pdwStartAddress),
        reinterpret_cast <uintptr_t> ((LPCVOID)pdwStartAddress),
                      szSymbol,
                        ulLen );

        if (ulLen > 0)
        {
          sprintf ( thread_name, "%s+%s",
                   SK_WideCharToUTF8 (SK_GetCallerName ((LPCVOID)pdwStartAddress)).c_str ( ),
                                                           szSymbol );
        }

        else {
          sprintf ( thread_name, "%s",
                      SK_WideCharToUTF8 (SK_GetCallerName ((LPCVOID)pdwStartAddress)).c_str () );
        }

        SK_TLS* pTLS =
          SK_TLS_BottomEx (it.second->dwTid);

        if (pTLS != nullptr)
          wcsncpy (pTLS->debug.name, SK_UTF8ToWideChar (thread_name).c_str (), 255);

        {
          extern concurrency::concurrent_unordered_map <DWORD, std::wstring> _SK_ThreadNames;

          if (! _SK_ThreadNames.count (it.second->dwTid))
          {
            _SK_ThreadNames [it.second->dwTid] =
              std::move (SK_UTF8ToWideChar (thread_name));

            it.second->name = std::move (SK_UTF8ToWideChar (thread_name));
          }
        }

        ImGui::Text ("");
      }

      ImGui::PopStyleColor ();

      if (ImGui::IsItemHovered ()) ThreadMemTooltip (it.second->dwTid);
    }
    ImGui::EndGroup   (); ImGui::SameLine ();


    ImGui::BeginGroup ();
    for (auto& it : SKWG_Ordered_Threads)
    {
      if (! IsThreadNonIdle (*it.second)) continue;

      UINT i =
        SetThreadIdealProcessor (it.second->hThread, MAXIMUM_PROCESSORS);

      if (i == (UINT)-1)
        i = 0;

      if (! it.second->exited)
      {
        ImGui::TextColored ( ImColor::HSV ( (float)( i       + 1 ) /
                                            (float)( uMaxCPU + 1 ), 0.5f, 1.0f ),
                               "CPU %lu", i );
      }

      else
      {
        ImGui::TextColored ( ImColor (0.85f, 0.85f, 0.85f), "-" );
      }

      if (ImGui::IsItemHovered ()) ThreadMemTooltip (it.second->dwTid);

      uMaxCPU  =  std::max (uMaxCPU, i);
    }
    ImGui::EndGroup ();  ImGui::SameLine ();


    ImGui::BeginGroup ();
    for ( auto& it : SKWG_Ordered_Threads )
    {
      if (! IsThreadNonIdle (*it.second)) continue;

      HANDLE hThread = it.second->hThread;

      int dwPrio = GetThreadPriority (hThread);

      if (! it.second->power_throttle)
        it.second->orig_prio = dwPrio;

      std::string prio_txt = "";

      switch (dwPrio)
      {
        case THREAD_PRIORITY_IDLE:
          //prio_txt = std::move ("THREAD_PRIORITY_IDLE");
          prio_txt = std::move ("Idle");
          break;
        case THREAD_PRIORITY_LOWEST:
          //prio_txt = std::move ("THREAD_PRIORITY_LOWEST");
          prio_txt = std::move ("Lowest");
          break;
        case THREAD_PRIORITY_BELOW_NORMAL:
          //prio_txt = std::move ("THREAD_PRIORITY_BELOW_NORMAL");
          prio_txt = std::move ("Below Normal");
          break;
        case THREAD_PRIORITY_NORMAL:
          //prio_txt = std::move ("THREAD_PRIORITY_NORMAL");
          prio_txt = std::move ("Normal");
          break;
        case THREAD_PRIORITY_ABOVE_NORMAL:
          //prio_txt = std::move ("THREAD_PRIORITY_ABOVE_NORMAL");
          prio_txt = std::move ("Above Normal");
          break;
        case THREAD_PRIORITY_HIGHEST:
          //prio_txt = std::move ("THREAD_PRIORITY_HIGHEST");
          prio_txt = std::move ("Highest");
          break;
        case THREAD_PRIORITY_TIME_CRITICAL:
          //prio_txt = std::move ("THREAD_PRIORITY_TIME_CRITICAL");
          prio_txt = std::move ("Time Critical");
          break;
        default:
          if (! it.second->exited)
            prio_txt = SK_FormatString ("%d", dwPrio);
          else
            prio_txt = u8"ⁿ/ₐ";
          break;
      }

      ImGui::Text ("  %s  ", prio_txt.c_str ());

      if (ImGui::IsItemHovered ()) ThreadMemTooltip (it.second->dwTid);
    }
    ImGui::EndGroup (); ImGui::SameLine ();


    static DWORD    dwLastSnap = 0;
    static FILETIME ftSnapKernel, ftSnapUser;

    FILETIME ftCreate, ftExit,
             ftKernel, ftUser;

    const DWORD SNAP_FREQUENCY = 30000UL;

    GetProcessTimes (GetCurrentProcess (), &ftCreate, &ftExit, &ftKernel, &ftUser);

    if ( (reset_stats && (dwLastSnap < dwNow - SNAP_FREQUENCY)) ||
          clear_counters )
    {
      ftSnapKernel = ftKernel;
      ftSnapUser   = ftUser;
    }

    LARGE_INTEGER liKernel; liKernel.LowPart  = ftKernel.dwLowDateTime;
                            liKernel.HighPart = ftKernel.dwHighDateTime;

                            LARGE_INTEGER liSnapDeltaK;
                                          liSnapDeltaK.LowPart  = ftSnapKernel.dwLowDateTime;
                                          liSnapDeltaK.HighPart = ftSnapKernel.dwHighDateTime;

                                          liKernel.QuadPart -= liSnapDeltaK.QuadPart;

    LARGE_INTEGER liUser;   liUser.LowPart    = ftUser.dwLowDateTime;
                            liUser.HighPart   = ftUser.dwHighDateTime;

                            LARGE_INTEGER liSnapDeltaU;
                                          liSnapDeltaU.LowPart  = ftSnapUser.dwLowDateTime;
                                          liSnapDeltaU.HighPart = ftSnapUser.dwHighDateTime;

                                          liUser.QuadPart -= liSnapDeltaU.QuadPart;

    long double p_kmax = 0.0;
    long double p_umax = 0.0;

    for ( auto& it : SKWG_Ordered_Threads )
    {
      if ( (reset_stats && (dwLastSnap < dwNow - SNAP_FREQUENCY)) ||
            clear_counters )
      {
        it.second->runtimes.snapshot.kernel = it.second->runtimes.kernel;
        it.second->runtimes.snapshot.user   = it.second->runtimes.user;
      }

      LARGE_INTEGER liThreadKernel, liThreadUser;

      liThreadKernel.HighPart = it.second->runtimes.kernel.dwHighDateTime;
      liThreadKernel.LowPart  = it.second->runtimes.kernel.dwLowDateTime;

      liThreadUser.HighPart   = it.second->runtimes.user.dwHighDateTime;
      liThreadUser.LowPart    = it.second->runtimes.user.dwLowDateTime;

         LARGE_INTEGER liThreadSnapDeltaK;
                       liThreadSnapDeltaK.LowPart  = it.second->runtimes.snapshot.kernel.dwLowDateTime;
                       liThreadSnapDeltaK.HighPart = it.second->runtimes.snapshot.kernel.dwHighDateTime;

                       liThreadKernel.QuadPart -= liThreadSnapDeltaK.QuadPart;

         LARGE_INTEGER liThreadSnapDeltaU;
                       liThreadSnapDeltaU.LowPart  = it.second->runtimes.snapshot.user.dwLowDateTime;
                       liThreadSnapDeltaU.HighPart = it.second->runtimes.snapshot.user.dwHighDateTime;

                       liThreadUser.QuadPart -= liThreadSnapDeltaU.QuadPart;

      it.second->runtimes.percent_kernel =
        100.0 * static_cast <long double> ( liThreadKernel.QuadPart ) /
                      static_cast <long double> ( liKernel.QuadPart );

      it.second->runtimes.percent_user =
        100.0 * static_cast <long double> ( liThreadUser.QuadPart ) /
                static_cast <long double> (       liUser.QuadPart );

      p_kmax = std::fmax (p_kmax, it.second->runtimes.percent_kernel);
      p_umax = std::fmax (p_umax, it.second->runtimes.percent_user);

      if (it.second->runtimes.percent_kernel >= 0.001) it.second->last_nonzero = dwNow;
      if (it.second->runtimes.percent_user   >= 0.001) it.second->last_nonzero = dwNow;
    }


    ImGui::BeginGroup ();
    for ( auto& it : SKWG_Ordered_Threads )
    {
      if (! IsThreadNonIdle (*it.second)) continue;

      if (it.second->runtimes.percent_kernel == p_kmax)
        ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.0944444, 0.79f, 1.0f));

      if (it.second->runtimes.percent_kernel >= 0.001)
        ImGui::Text ("%6.2f%% Kernel", it.second->runtimes.percent_kernel);
      else
        ImGui::Text ("-");

      if (it.second->runtimes.percent_kernel == p_kmax)
        ImGui::PopStyleColor ();

      if (ImGui::IsItemHovered ()) ThreadMemTooltip (it.second->dwTid);
    }
    ImGui::EndGroup (); ImGui::SameLine ();


    ImGui::BeginGroup ();
    for ( auto& it : SKWG_Ordered_Threads )
    {
      if (! IsThreadNonIdle (*it.second)) continue;

      if (it.second->runtimes.percent_user == p_umax)
        ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.0944444, 0.79f, 1.0f));

      if (it.second->runtimes.percent_user >= 0.001)
        ImGui::Text ("%6.2f%% User", it.second->runtimes.percent_user);
      else
        ImGui::Text ("-");

      if (it.second->runtimes.percent_user == p_umax)
        ImGui::PopStyleColor ();

      if (ImGui::IsItemHovered ()) ThreadMemTooltip (it.second->dwTid);
    }
    ImGui::EndGroup ();    ImGui::SameLine ();


    ImGui::BeginGroup ();
    for (auto& it : SKWG_Ordered_Threads)
    {
      if (! IsThreadNonIdle (*it.second)) continue;

      HANDLE hThread = it.second->hThread;

      BOOL bDisableBoost = FALSE;

      if (GetThreadPriorityBoost (hThread, &bDisableBoost) && (! bDisableBoost))
      {
        ImGui::TextColored (ImColor::HSV (1.0f, 0.0f, 0.38f), " (Dynamic Boost)");
      }

      else
        ImGui::Text ("");

      if (ImGui::IsItemHovered ()) ThreadMemTooltip (it.second->dwTid);
    }
    ImGui::EndGroup ();    ImGui::SameLine ();


    // Used to determine whether to srhink the dialog box;
    //
    //   Waiting I/O is a status that frequently comes and goes, but we don't
    //     want the widget rapidly resizing itself, so we need a grace period.
    static DWORD dwLastWaiting = 0;
    const  DWORD WAIT_GRACE    = 666UL;

    ImGui::BeginGroup ();
    for (auto& it : SKWG_Ordered_Threads)
    {
      if (! IsThreadNonIdle (*it.second)) continue;

      HANDLE hThread  = it.second->hThread;
      BOOL   bPending = FALSE;

      if (GetThreadIOPendingFlag (hThread, &bPending) && bPending)
      {
        dwLastWaiting = dwNow;
        ImGui::TextColored (ImColor (1.0f, 0.090196f, 0.364706f), "Waiting I/O");
      }
      else if (dwNow > dwLastWaiting + WAIT_GRACE)
        ImGui::Text ("");
      else
        // Alpha: 0  ==>  Hide this text, it's for padding only.
        ImGui::TextColored (ImColor (1.0f, 0.090196f, 0.364706f, 0.0f), "Waiting I/O");

      ImGui::SameLine ();

      if (it.second->power_throttle)
        ImGui::TextColored (ImColor (0.090196f, 1.0f, 0.364706f), "Power-Throttle");
      else
        ImGui::Text ("");

      if (ImGui::IsItemHovered ()) ThreadMemTooltip (it.second->dwTid);
    }
    ImGui::EndGroup   ();

  //ImGui::EndChildFrame ();
    ImGui::EndGroup      ();

    for (auto& it : SKWG_Ordered_Threads)
    {
      if ( it.second->hThread != 0 &&
           it.second->hThread != INVALID_HANDLE_VALUE )
      {
        CloseHandle (it.second->hThread);
                     it.second->hThread = INVALID_HANDLE_VALUE;
      }
    }

    if ( (reset_stats && (dwLastSnap < dwNow - SNAP_FREQUENCY)) ||
          clear_counters )
    {
      dwLastSnap = dwNow;
    }
  }


  virtual void OnConfig (ConfigEvent event) override
  {
    switch (event)
    {
      case SK_Widget::ConfigEvent::LoadComplete:
        break;

      case SK_Widget::ConfigEvent::SaveStart:
        break;
    }
  }

protected:
private:
} __thread_profiler__;