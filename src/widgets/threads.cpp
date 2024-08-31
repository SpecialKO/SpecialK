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

#pragma pack(push,8)
typedef LONG NTSTATUS;
typedef      NTSTATUS (WINAPI *NtQueryInformationThread_pfn)
       (HANDLE,/*THREADINFOCLASS*/LONG,DWORD_PTR*,ULONG,PULONG);

#define ThreadQuerySetWin32StartAddress 9

#if (VER_PRODUCTBUILD < 10011)
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
} THREAD_POWER_THROTTLING_STATE,
 *PTHREAD_POWER_THROTTLING_STATE;

#define THREAD_POWER_THROTTLING_CURRENT_VERSION 1
#define THREAD_POWER_THROTTLING_EXECUTION_SPEED 1
#define THREAD_POWER_THROTTLING_VALID_FLAGS     1

typedef BOOL (WINAPI *GetThreadInformation_pfn)(HANDLE, THREAD_INFORMATION_CLASS_EX, LPVOID, DWORD);
typedef BOOL (WINAPI *SetThreadInformation_pfn)(HANDLE, THREAD_INFORMATION_CLASS_EX, LPVOID, DWORD);
#else
typedef BOOL (WINAPI* GetThreadInformation_pfn)(HANDLE, THREAD_INFORMATION_CLASS, LPVOID, DWORD);
typedef BOOL (WINAPI* SetThreadInformation_pfn)(HANDLE, THREAD_INFORMATION_CLASS, LPVOID, DWORD);
#endif



extern iSK_INI* osd_ini;

bool SK_Thread_HasCustomName (DWORD dwTid);

float __SK_Thread_RebalanceEveryNSeconds = 21.0f;

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
  MaxProcessInfoClass // MaxProcessInfoClass should always be the last enum
} PROCESS_INFORMATION_CLASS_FULL,
 *PPROCESS_INFORMATION_CLASS_FULL;

#define SystemProcessAndThreadInformation 5

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
} UNICODE_STRING; //-V677

typedef struct _SYSTEM_THREAD {
    FILETIME     ftKernelTime;   // 100 nsec units
    FILETIME     ftUserTime;     // 100 nsec units
    FILETIME     ftCreateTime;   // relative to 01-01-1601
    ULONG        dWaitTime;
#ifdef _M_AMD64
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
} SYSTEM_PROCESS_INFORMATION,
*PSYSTEM_PROCESS_INFORMATION;

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

static NtQuerySystemInformation_pfn
       NtQuerySystemInformation = nullptr;

typedef struct _PEB {
  BYTE  Reserved1 [2];
  BYTE  BeingDebugged;
  BYTE  Reserved2 [229];
  PVOID Reserved3 [59];
  ULONG SessionId;
} PEB,
*PPEB;

typedef struct _PROCESS_BASIC_INFORMATION
{ //-V802 (Not optimal packing, but it's a Windows datastructure!)
  NTSTATUS  ExitStatus;
  PPEB      PebBaseAddress;
  ULONG_PTR AffinityMask;
  KPRIORITY BasePriority;
  HANDLE    UniqueProcessId;
  HANDLE    InheritedFromUniqueProcessId;
} PROCESS_BASIC_INFORMATION,
*PPROCESS_BASIC_INFORMATION;

typedef NTSTATUS (NTAPI *NtQueryInformationProcess_pfn)(
__in                                   HANDLE                         ProcessHandle,
__in                                   PROCESS_INFORMATION_CLASS_FULL ProcessInformationClass,
__out_bcount(ProcessInformationLength) PVOID                          ProcessInformation,
__in                                   ULONG                          ProcessInformationLength,
__out_opt                              PULONG                         ReturnLength );

NtQueryInformationProcess_pfn
NtQueryInformationProcess = nullptr;
#pragma pack(pop)

static constexpr DWORD SNAP_FREQUENCY = 30000UL;

SK_LazyGlobal <std::map <DWORD,    SKWG_Thread_Entry*>> SKWG_Threads;
SK_LazyGlobal <std::map <LONGLONG, SKWG_Thread_Entry*>> SKWG_Ordered_Threads;

struct SK_ProcessTimeCounters
{
  FILETIME ftCreate { 0UL, 0UL }, ftExit { 0UL, 0UL },
           ftKernel { 0UL, 0UL }, ftUser { 0UL, 0UL };
};

SK_LazyGlobal <SK_ProcessTimeCounters> process_time;

PSYSTEM_PROCESS_INFORMATION
ProcessInformation ( PDWORD                       pdData,
                     PNTSTATUS                    pns,
                     SK_NtQuerySystemInformation* pQuery )
{
  PSYSTEM_PROCESS_INFORMATION ret = nullptr;

  if (NtQuerySystemInformation == nullptr)
  {
    if (! SK_GetModuleHandleW  (L"NtDll"))
       SK_Modules->LoadLibrary (L"NtDll");

    NtQuerySystemInformation =
      (NtQuerySystemInformation_pfn)
        SK_GetProcAddress (L"NtDll",
                            "NtQuerySystemInformation");
  }

  if (NtQuerySystemInformation != nullptr)
  {
    if (pQuery != nullptr)
    {
      size_t                       dSize = 0;
      DWORD                        dData = 0;
      NTSTATUS                        ns = STATUS_INVALID_PARAMETER;

      if (pQuery->NtInfo.len  < 2048000)
          pQuery->NtInfo.alloc (2048000, true);

      void* pspi = nullptr;

      for ( dSize = pQuery->NtInfo.len;
                 (pspi == nullptr) && dSize  != 0;
                                      dSize <<= 1 )
      {
        pQuery->NtInfo.alloc (dSize, true);

        if (pQuery->NtInfo.len < dSize)
        {
          ns = STATUS_NO_MEMORY;
        }

        else
        {

          dSize =
            pQuery->NtInfo.len;

          pspi =
            pQuery->NtInfo.data;

          if (pspi != nullptr)
          {
            ns =
              NtQuerySystemInformation ( SystemProcessInformation,
                                           (PSYSTEM_PROCESS_INFORMATION)pspi,
                                             (DWORD)dSize,
                                               &dData );
          }

          else
          {
            ns = STATUS_NO_MEMORY;
          }
        }

        if (ns != STATUS_SUCCESS)
        {
          dData = 0;
          pspi  = nullptr;

          if (ns != STATUS_INFO_LENGTH_MISMATCH) break;
        }
      }

      if (pdData != nullptr) *pdData = dData;
      if (pns    != nullptr) *pns    = ns;

      ret =
        static_cast <PSYSTEM_PROCESS_INFORMATION> (pspi);

      pQuery->NtStatus = ns;
    }
  }

  return
    ret;
}

#ifdef _M_AMD64
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

static          size_t rebalance_idx       = 0;
static          bool   rebalance           = false;
static volatile LONG   rebalance_queue     = 0;

void
SK_Thread_RebalanceThreads (void)
{
  InterlockedCompareExchange (&rebalance_queue, 1, 0);
}

void
SK_ImGui_RebalanceThreadButton (void)
{
  if (! rebalance)
  {
    if (ImGui::Button ("Rebalance Threads"))
    {
      SK_Thread_RebalanceThreads ();
    }
  }
}

const char*
SKX_DEBUG_FastSymName (LPCVOID ret_addr)
{
  HMODULE hModSource = nullptr;
  HANDLE  hProc      = SK_GetCurrentProcess ();

  static std::string symbol_name = "";

#ifdef _M_AMD64
  static DWORD64 last_ip = 0;

  DWORD64  ip       (reinterpret_cast <DWORD64> (ret_addr));
  DWORD64  BaseAddr = 0;
#else /* _M_IX86 */
  static DWORD last_ip = 0;

  DWORD    ip       (reinterpret_cast <DWORD> (ret_addr));
  DWORD    BaseAddr = 0;
#endif

  SK_TLS* pTLS =
    SK_TLS_Bottom ();

  if (last_ip != ip)
  {
    last_ip = ip;

    char szModName [MAX_PATH + 2] = { };

    if ( GetModuleHandleEx ( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
              reinterpret_cast <LPCWSTR> (ip),
                                 &hModSource ) )
    {
      GetModuleFileNameA (hModSource, szModName, MAX_PATH);
    }

    MODULEINFO mod_info = { };

    GetModuleInformation (
      hProc, hModSource,
        &mod_info, sizeof (MODULEINFO)
    );

#ifdef _M_AMD64
    BaseAddr = (DWORD64)mod_info.lpBaseOfDll;
#else /* _M_IX86 */
    BaseAddr = (DWORD)  mod_info.lpBaseOfDll;
#endif

    char* szDupName =
      pTLS->scratch_memory->sym_resolve.alloc (
        strnlen (szModName, MAX_PATH) + 1
      );

    if (szDupName != nullptr)
    {
      strncpy_s ( szDupName, pTLS->scratch_memory->sym_resolve.len,
                  szModName, _TRUNCATE );

      char *pszShortName =
              szDupName;

      PathStripPathA (pszShortName);

      if ( dbghelp_callers.find (hModSource) ==
           dbghelp_callers.cend (          ) && cs_dbghelp != nullptr )
      {
        std::scoped_lock <SK_Thread_HybridSpinlock> auto_lock (*cs_dbghelp);

        if ( dbghelp_callers.find (hModSource) ==
             dbghelp_callers.cend (          )  )
        {
          SK_SymLoadModule ( hProc,
                               nullptr,
                                pszShortName,
                                  nullptr,
                                    BaseAddr,
                                      mod_info.SizeOfImage );

          dbghelp_callers.insert (hModSource);
        }
      }

      SYMBOL_INFO_PACKAGE
        sip                 = {                  };
        sip.si.SizeOfStruct = sizeof (SYMBOL_INFO);
        sip.si.MaxNameLen   = sizeof (  sip.name );

      DWORD64 Displacement = 0;

      if ( SymFromAddr ( hProc,
               static_cast <DWORD64> (ip),
                             &Displacement,
                               &sip.si ) )
      {
        DWORD Disp =
          0x00UL;

#ifdef _M_AMD64
        IMAGEHLP_LINE64 ihl              = {                      };
                        ihl.SizeOfStruct = sizeof (IMAGEHLP_LINE64);
#else /* _M_IX86 */
        IMAGEHLP_LINE   ihl              = {                    };
                        ihl.SizeOfStruct = sizeof (IMAGEHLP_LINE);
#endif
        const bool bFileAndLine =
          ( SK_SymGetLineFromAddr ( hProc, ip, &Disp, &ihl ) != FALSE );

        if (bFileAndLine)
        {
          symbol_name =
            SK_FormatString ( "[%hs] %hs <%hs:%lu>",
                                pszShortName,
                                  sip.si.Name,
                                     ihl.FileName,
                                     ihl.LineNumber );
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
        symbol_name =
          SK_WideCharToUTF8 (
            SK_MakePrettyAddress (ret_addr)
          );
      }
    }

    if (hModSource)
      SK_FreeLibrary (hModSource);
  }

  return
    symbol_name.data ();
}

void
SK_ImGui_ThreadCallstack ( HANDLE hThread, LARGE_INTEGER userTime,
                                           LARGE_INTEGER kernelTime )
{
  SK_TLS* pTLS =
    SK_TLS_Bottom ();

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

#ifdef _M_AMD64
  DWORD64  ip       = thread_ctx.Rip;
  DWORD64  BaseAddr = 0;

  STACKFRAME64
    stackframe                  = {            };
    stackframe.AddrStack.Offset = thread_ctx.Rsp;
    stackframe.AddrFrame.Offset = thread_ctx.Rbp;
#else /* _M_IX86 */
  DWORD    ip       = thread_ctx.Eip;
  DWORD    BaseAddr = 0;

  STACKFRAME
    stackframe                  = {            };
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

         BOOL ret   = TRUE;
  static int  lines = 0;

  if (
#ifndef _M_AMD64
       last_ctx.Eip        != thread_ctx.Eip ||
#else /* _M_IX86 */
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

      if ( GetModuleHandleEx ( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                reinterpret_cast <LPCWSTR> (ip),
                                   &hModSource ) )
      {
        GetModuleFileNameA (hModSource, szModName, MAX_PATH);
      }

      MODULEINFO mod_info = { };

      GetModuleInformation (
        hProc, hModSource,
          &mod_info, sizeof (MODULEINFO)
      );

#ifdef _M_AMD64
      BaseAddr = (DWORD64)mod_info.lpBaseOfDll;
#else /* _M_IX86 */
      BaseAddr = (DWORD)  mod_info.lpBaseOfDll;
#endif

      char* szDupName =
        pTLS->scratch_memory->cmd.alloc (
          strnlen (szModName, MAX_PATH) + 1, true );

      if (szDupName != nullptr)
      {
        strncpy_s ( szDupName, pTLS->scratch_memory->cmd.len,
                    szModName, _TRUNCATE );

        char* pszShortName =
                szDupName;

        PathStripPathA (pszShortName);


        if ( dbghelp_callers.find (hModSource) ==
             dbghelp_callers.cend (          ) && cs_dbghelp != nullptr )
        {
          std::scoped_lock <SK_Thread_HybridSpinlock> auto_lock (*cs_dbghelp);

          if ( dbghelp_callers.find (hModSource) ==
               dbghelp_callers.cend (          )  )
          {
            SK_SymLoadModule ( hProc,
                                 nullptr,
                                  pszShortName,
                                    nullptr,
                                      BaseAddr,
                                        mod_info.SizeOfImage );

            dbghelp_callers.insert (hModSource);
          }
        }

        SYMBOL_INFO_PACKAGE sip = { };

        sip.si.SizeOfStruct = sizeof (SYMBOL_INFO);
        sip.si.MaxNameLen   = sizeof (sip.name);

        DWORD64 Displacement = 0;

        if ( SymFromAddr ( hProc,
               static_cast <DWORD64> (ip),
                          &Displacement,
                            &sip.si  )  )
        {
          DWORD Disp = 0x00UL;

#ifdef _M_AMD64
          IMAGEHLP_LINE64 ihl = {                      };
          ihl.SizeOfStruct    = sizeof (IMAGEHLP_LINE64);
#else /* _M_IX86 */
          IMAGEHLP_LINE ihl = {                    };
          ihl.SizeOfStruct  = sizeof (IMAGEHLP_LINE);
#endif
          const BOOL bFileAndLine =
            SK_SymGetLineFromAddr (hProc, ip, &Disp, &ihl);

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
            code_lines.emplace_back ();
          }

          if (top_func.empty ())
            top_func = sip.si.Name;

          ++lines;
        }
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

      if (hModSource)
        SK_FreeLibrary (hModSource);
    } while (ret != FALSE);
  }


  if (file_names.size ( ) > 2)
  {
    static std::string self_ansi =
      SK_WideCharToUTF8 (SK_GetModuleName (skModuleRegistry::Self ()));
    static std::string host_ansi =
      SK_WideCharToUTF8 (SK_GetModuleName (skModuleRegistry::HostApp ()));


    ImGui::PushStyleColor (ImGuiCol_Text,   (ImVec4&&)ImColor::HSV (0.15f, 0.95f, 0.99f));
    ImGui::PushStyleColor (ImGuiCol_Border, (ImVec4&&)ImColor::HSV (0.15f, 0.95f, 0.99f));
    ImGui::Separator      (  );
    ImGui::PopStyleColor  (2);

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
        color = ImColor::HSV (0.244444f, 0.85f, 1.0f);
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

      if ( special_lines.find (idx) !=
           special_lines.cend (   ) )
      {
        color = ImColor::HSV (0.527778f, 0.85f, 1.0f);
      }

      ImGui::TextColored (color, "%hs", it.c_str ());
    }
    ImGui::EndGroup   (); ImGui::SameLine ();


    ImGui::BeginGroup ();
    idx = 0;
    for ( auto& it : code_lines )
    {
      if (idx++ == 0 || idx == code_lines.size ()) continue;

      ImColor color (1.0f, 1.0f, 1.0f, 1.0f);

      if ( special_lines.find (idx) !=
           special_lines.cend (   )  )
      {
        color = ImColor::HSV (0.1f, 0.85f, 1.0f);
      }

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
  struct SK_Thread_DataCollector
  {
    // Double-buffered
             SK_NtQuerySystemInformation* pQuery         = nullptr;
             HANDLE                       hProduceThread = INVALID_HANDLE_VALUE;
             HANDLE                       hSignalProduce =
               SK_CreateEvent (nullptr, FALSE, TRUE, nullptr);
             HANDLE                       hSignalConsume =
               SK_CreateEvent (nullptr, FALSE, FALSE, nullptr);
             HANDLE                       hSignalShutdown =
               SK_CreateEvent (nullptr, FALSE, FALSE, nullptr);
             DWORD                        dwProducerTid;

    SK_Thread_DataCollector (void)
    {
      hProduceThread =
        SK_Thread_CreateEx ([](LPVOID user) ->
          DWORD
          {
            auto&
              _process_time =
               process_time.get ();

            SK_Thread_SetCurrentPriority (THREAD_PRIORITY_LOWEST);

            SK_TLS* pTLS =
                  SK_TLS_Bottom ();

            auto pParams =
              reinterpret_cast <SK_Thread_DataCollector *>(user);

            HANDLE hSignals [] = {
              pParams->hSignalProduce,
              pParams->hSignalShutdown
            };

            HANDLE hCurrentProcess = GetCurrentProcess ();
            DWORD  dwWaitState     = 0;
            int    write_idx       = 0;
            int    read_idx        = 1;

            constexpr int WAIT_PRODUCE_DATA    = WAIT_OBJECT_0;
            constexpr int WAIT_SHUTDOWN_THREAD = WAIT_OBJECT_0 + 1;

            const SK_RenderBackend& rb =
              SK_GetCurrentRenderBackend ();

            do
            {
              dwWaitState =
                 WaitForMultipleObjects ( 2,
                            hSignals, FALSE, INFINITE );

              if (dwWaitState == WAIT_PRODUCE_DATA)
              {
                // Throttle data collection to once every 2 frames maximum
                ULONG64 ullFrameStart = SK_GetFramesDrawn ();
                while ( ullFrameStart > SK_GetFramesDrawn () - 2 )
                {
                  DWORD dwMsToWait =
                    std::min ( 100UL,
                      static_cast <DWORD> (
                              ( 2ULL * rb.frame_delta.getDeltaTime () ) /
                                                  ( SK_PerfTicksPerMs )
                             )            );

                  if ( WAIT_TIMEOUT !=
                         SK_WaitForSingleObject ( pParams->hSignalShutdown,
                                                    dwMsToWait )
                     )
                  {
                    dwWaitState =
                      WAIT_SHUTDOWN_THREAD;

                    break;
                  }
                }
              }

              SK_NtQuerySystemInformation*  pWrite
              = &pTLS->local_scratch->query [write_idx];

              if (dwWaitState == WAIT_PRODUCE_DATA)
              {
                ProcessInformation ( nullptr,
                           &pWrite->NtStatus,
                            pWrite );

                if (STATUS_SUCCESS == pWrite->NtStatus)
                {
                  // Cumulative times; sample them as close to the thread snapshot as possible or
                  //   the load %'s will not be accurate.
                  GetProcessTimes (   hCurrentProcess,
                   &_process_time.ftCreate, &_process_time.ftExit,
                   &_process_time.ftKernel, &_process_time.ftUser
                                  );

                  std::swap ( write_idx,
                               read_idx );

                  InterlockedExchangePointer (
                      (void **)&pParams->pQuery,
                    &pTLS->local_scratch->query [read_idx]
                  );

                  SetEvent (
                    pParams->hSignalConsume
                  );
                }
              }
            } while (dwWaitState != WAIT_SHUTDOWN_THREAD);

            return 0;
          },
        L"[SK] Thread Analytics Producer",
      (LPVOID)this );

      dwProducerTid =
        GetThreadId (hProduceThread);
    }

    ~SK_Thread_DataCollector (void)
    {
      SignalObjectAndWait (hSignalShutdown,  hProduceThread, 100, FALSE);
      SK_CloseHandle      (hSignalProduce);  SK_CloseHandle (hSignalConsume);
      SK_CloseHandle      (hSignalShutdown); SK_CloseHandle (hProduceThread);
    }
  };

public:
  SKWG_Thread_Profiler (void) noexcept : SK_Widget ("Thread Profiler")
  {
    SK_ImGui_Widgets->thread_profiler = this;

    setAutoFit      ( true).setDockingPoint (DockAnchor::West).
    setClickThrough (false).setBorder       (true            );
  };

  void run (void) override
  {
    // Thread profiling currently does not work in WINE
    //
    if (config.compatibility.using_wine)
      return;

    std::scoped_lock <std::mutex>
                lock (run_lock);

    static
      std::once_flag init_once;
    std::call_once ( init_once,
    [this]()
    {
      k32SetThreadInformation =
        (SetThreadInformation_pfn)SK_GetProcAddress (
          L"kernel32",
           "SetThreadInformation"
        );

      k32GetThreadInformation =
        (GetThreadInformation_pfn)SK_GetProcAddress (
          L"kernel32",
           "GetThreadInformation"
        );

      NtQueryInformationProcess =
        (NtQueryInformationProcess_pfn)SK_GetProcAddress (
          L"NtDll",
           "NtQueryInformationProcess"
        );

      data_thread =
        std::make_unique <SK_Thread_DataCollector> ();
    } );

    static float last_rebalance = 0.0f;

    if ( __SK_Thread_RebalanceEveryNSeconds    > 0.0f &&
        ( (float)SK_timeGetTime () / 1000.0f ) >
       (last_rebalance + __SK_Thread_RebalanceEveryNSeconds) )
    {
      SK_Thread_RebalanceThreads ();
    }


    if ((! rebalance) && 1 == InterlockedCompareExchange (&rebalance_queue, 0, 1))
    {
      rebalance     = true;
      rebalance_idx = 0;
    }


    static std::vector        <SKWG_Thread_Entry *> rebalance_list;
    static std::unordered_set <DWORD>               blacklist;

    if (rebalance && rebalance_list.empty ())
    {
      last_rebalance =
        static_cast <float> (SK_timeGetTime ()) / 1000.0f;

      for ( auto& it : *SKWG_Ordered_Threads )
      {
        if (                 it.second         == nullptr ||
                             it.second->exited == true    ||
             blacklist.find (it.second->dwTid) !=
             blacklist.cend (                )  )
        {
          continue;
        }

        SK_TLS* pTLS =
          SK_TLS_BottomEx (it.second->dwTid);

        if (pTLS != nullptr)
        {
          blacklist.emplace (it.second->dwTid);
          continue;
        }

        if (blacklist.find  (it.second->dwTid) !=
            blacklist.cend  (                ))
        {   blacklist.erase (it.second->dwTid); }

        rebalance_list.push_back (it.second);
      }

      std::sort ( rebalance_list.begin (), rebalance_list.end (),
           []( SKWG_Thread_Entry *lh,
               SKWG_Thread_Entry *rh ) ->
           bool
           {
             LARGE_INTEGER lil =
             {sk::narrow_cast <DWORD> (lh->runtimes.user.dwLowDateTime),
              sk::narrow_cast <LONG>  (lh->runtimes.user.dwHighDateTime)},
                           lir =
             {sk::narrow_cast <DWORD> (rh->runtimes.user.dwLowDateTime),
              sk::narrow_cast <LONG>  (rh->runtimes.user.dwHighDateTime)};

             return
               ( lil.QuadPart < lir.QuadPart );
           }
      );
    }

    if (rebalance)
    {
      size_t idx       = 0;
      DWORD  dwTidSelf = SK_Thread_GetCurrentId ();

      for ( auto& it : rebalance_list )
      {
        if ( it->dwTid == dwTidSelf )
        {
          ++rebalance_idx; ++idx; continue;
        }

        if (rebalance_idx == idx)
        {
          HANDLE hThreadOrig =
             it->hThread;

          if (it->hThread == INVALID_HANDLE_VALUE)
          {
            it->hThread =
              OpenThread (THREAD_ALL_ACCESS, FALSE, it->dwTid);
          }

          if ( (intptr_t)it->hThread > 0 )
          {
            DWORD dwExitCode = 0;
            DWORD pnum       =
            SK_SetThreadIdealProcessor (it->hThread, MAXIMUM_PROCESSORS);
            GetExitCodeThread          (it->hThread, &dwExitCode);

            if ( pnum != sk::narrow_cast <DWORD> (-1) && dwExitCode == STILL_ACTIVE )
            {
              static SYSTEM_INFO
                  sysinfo = { };
              if (sysinfo.dwNumberOfProcessors == 0)
              {
                SK_GetSystemInfo (&sysinfo);
              }

              SK_TLS* pTLS =
                SK_TLS_BottomEx (it->dwTid);

              static DWORD     ideal  = 0;
                     DWORD_PTR dwMask = pTLS   ?
                pTLS->scheduler->affinity_mask : DWORD_PTR_MAX;

              if (pnum != ideal && (( (dwMask >> pnum)  & 0x1) == 0x1)
                                && (( (dwMask >> ideal) & 0x1) == 0x1))
              {
                if ( SK_SetThreadIdealProcessor ( it->hThread, ideal ) != DWORD_MAX )
                  pnum = ideal;
              }

              if (pnum == ideal || ((dwMask >> ideal) & 0x1) != 0x1)
              {
                if (pnum == ideal)
                  rebalance_idx++;

                ideal++;

                if (ideal >= sysinfo.dwNumberOfProcessors)
                  ideal = 0;
              }
            }

            else
              ++rebalance_idx;

            if (it->hThread != hThreadOrig)
            {
              SK_CloseHandle (it->hThread);
                              it->hThread = INVALID_HANDLE_VALUE;
            }
          }

          else
            ++rebalance_idx;
        }

        // No more rebalancing to do!
        if ( rebalance_idx >= rebalance_list.size () )
        {
          rebalance = false;
          break;
        }

        ++idx;
      }

      if (! rebalance)
      {
        rebalance_list.clear ();
        rebalance_idx = 0;
      }
    }


    // Snapshotting is _slow_, so only do it when a thread has been created...
    extern volatile LONG  lLastThreadCreate;
    static          LONG  lLastThreadRefresh   =  -69;
              const DWORD _UPDATE_INTERVAL1_MS =  200; // Refresh no more than once every 200 ms
              const DWORD _UPDATE_INTERVAL2_MS = 5000; // Refresh at least once every 5 seconds
    static          DWORD dwLastTime           =    0;

    const DWORD dwNow =
      SK_timeGetTime ();
    const LONG  last  =
      ReadAcquire (&lLastThreadCreate);
    // If true, no new threads have been created since we
    //   last enumerated our list.
    if (last == lLastThreadRefresh)
    {
      if (dwLastTime > dwNow - _UPDATE_INTERVAL2_MS)
      {
        if (! visible)
          return;

        if (dwLastTime + _UPDATE_INTERVAL1_MS > dwNow)
          return;
      }
    }

    dwLastTime = dwNow;

    static PSYSTEM_PROCESS_INFORMATION pInfo = nullptr;
    static NTSTATUS                    nts   = STATUS_INVALID_PARAMETER;

    bool update = false;

    if (SK_WaitForSingleObject (data_thread->hSignalConsume, 0) == WAIT_OBJECT_0)
    {
      // The producer is double-buffered, this always points to the last finished
      //   data collection cycle.
      SK_NtQuerySystemInformation* pLatestQuery =
        reinterpret_cast <SK_NtQuerySystemInformation *> (
          ReadPointerAcquire ((volatile LPVOID*)& data_thread->pQuery)
        );

      pInfo =
        reinterpret_cast <PSYSTEM_PROCESS_INFORMATION> (
              pLatestQuery->NtInfo.data );
      nts   = pLatestQuery->NtStatus;

      update = true;
    }

    if (update && pInfo != nullptr && nts == STATUS_SUCCESS)
    {
      int i = 0;

      const DWORD dwPID = GetCurrentProcessId    ();
                //dwTID = SK_Thread_GetCurrentId ();

      SYSTEM_PROCESS_INFORMATION* pProc = pInfo;

      do
      {
        if ((DWORD)((uintptr_t)pProc->UniqueProcessId & 0xFFFFFFFFU) == dwPID)
          break;

        pProc =
          reinterpret_cast <SYSTEM_PROCESS_INFORMATION *>
            ((BYTE *)pProc + pProc->dNext);
      } while (pProc->dNext != 0);


      if ((DWORD)((uintptr_t)pProc->UniqueProcessId & 0xFFFFFFFFU) == dwPID)
      {
        const int threads =
          pProc->dThreadCount;

        for (i = 0; i < threads; i++)
        {
          if ((DWORD)((uintptr_t)pProc->aThreads [i].Cid.UniqueProcess & 0xFFFFFFFFU) != dwPID)
            continue;

          bool  new_thread = false;
          DWORD dwLocalTID =
            (DWORD)((uintptr_t)pProc->aThreads [i].Cid.UniqueThread & 0xFFFFFFFFU);

          if (SKWG_Threads->find (dwLocalTID) == SKWG_Threads->cend ())
          {
            new_thread = true;

            SKWG_Thread_Entry::runtimes_s runtimes = { };

            SKWG_Thread_Entry *ptEntry =
              new (std::nothrow)
             SKWG_Thread_Entry {
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

            if (ptEntry != nullptr)
            {
              SKWG_Threads->insert (
                std::make_pair ( dwLocalTID, ptEntry )
              );
            }

            if ( config.render.framerate.enable_mmcss &&
                 SK_GetCurrentGameID () == SK_GAME_ID::AssassinsCreed_Odyssey )
            {
              if (! ptEntry->name.empty ())
              {
                std::string utf8_name =
                  SK_WideCharToUTF8 (ptEntry->name);

                if (ptEntry->name._Equal (L"AsyncFileCompletionThread"))
                {
                  SK_MMCS_TaskEntry* task_me =
                    SK_MMCS_GetTaskForThreadIDEx (ptEntry->dwTid, utf8_name.c_str (), "Audio", "Capture");

                  if (task_me != nullptr)
                  {
                    task_me->setMaxCharacteristics ("Audio", "Capture");
                    task_me->setPriority           (AVRT_PRIORITY_HIGH);
                  }
                }

                if (ptEntry->name._Equal (L"Loading Thread"))
                {
                  SK_MMCS_TaskEntry* task_me =
                    SK_MMCS_GetTaskForThreadIDEx (ptEntry->dwTid, utf8_name.c_str (), "Playback", "Distribution");

                  if (task_me != nullptr)
                  {
                    task_me->setMaxCharacteristics ("Playback", "Distribution");
                    task_me->setPriority           (AVRT_PRIORITY_HIGH);
                  }
                }

                if (ptEntry->name._Equal (L"EngineWindowThread"))
                {
                  SK_MMCS_TaskEntry* task_me =
                    SK_MMCS_GetTaskForThreadIDEx (ptEntry->dwTid, utf8_name.c_str (), "Playback", "Window Manager");

                  if (task_me != nullptr)
                  {
                    task_me->setMaxCharacteristics ("Playback", "Window Manager");
                    task_me->setPriority           (AVRT_PRIORITY_NORMAL);
                  }
                }
              }
            }
          }

          SKWG_Thread_Entry* ptEnt =
            SKWG_Threads.get ()[dwLocalTID];

          const auto& thread =
            pProc->aThreads [i];

          ptEnt->wait_reason =
            static_cast <WaitReason>
                 (thread.WaitReason);
          ptEnt->thread_state =
            static_cast <ThreadState>
                (thread.dThreadState);

          ptEnt->runtimes.created = thread.ftCreateTime;
          ptEnt->runtimes.kernel  = thread.ftKernelTime;
          ptEnt->runtimes.user    = thread.ftUserTime;

          if (new_thread)
          {
            LARGE_INTEGER liCreate;

            liCreate.HighPart = ptEnt->runtimes.created.dwHighDateTime;
            liCreate.LowPart  = ptEnt->runtimes.created.dwLowDateTime;

            (*SKWG_Ordered_Threads)[liCreate.QuadPart] =
              ptEnt;
          }
        }

        lLastThreadRefresh =
         last;
      }

      SetEvent (data_thread->hSignalProduce);
    }
  }


  void draw (void) override
  {
    std::scoped_lock <std::mutex>
                lock (run_lock);

    if (ImGui::GetFont () == nullptr) return;

    DWORD dwNow =
      SK_timeGetTime ();

           bool drew_tooltip   = false;
    static bool show_callstack = false;

    const auto ThreadMemTooltip = [&](DWORD dwSelectedTid) ->
    void
    {
      if (drew_tooltip)
        return;

      drew_tooltip = true;

      SK_TLS* pTLS =
        SK_TLS_BottomEx (dwSelectedTid);

      if (pTLS != nullptr)
      {
        auto& error_state =
          pTLS->win32->error_state;

        ImGui::BeginTooltip ();

        if (ReadAcquire64 (&pTLS->memory->global_bytes)  != 0 ||
            ReadAcquire64 (&pTLS->memory->local_bytes)   != 0 ||
            ReadAcquire64 (&pTLS->memory->virtual_bytes) != 0 ||
            ReadAcquire64 (&pTLS->memory->heap_bytes)    != 0    )
        {
          ImGui::BeginGroup   ();
          if (ReadAcquire64 (&pTLS->memory->local_bytes))   ImGui::TextUnformatted ("Local Memory:\t");
          if (ReadAcquire64 (&pTLS->memory->global_bytes))  ImGui::TextUnformatted ("Global Memory:\t");
          if (ReadAcquire64 (&pTLS->memory->heap_bytes))    ImGui::TextUnformatted ("Heap Memory:\t");
          if (ReadAcquire64 (&pTLS->memory->virtual_bytes)) ImGui::TextUnformatted ("Virtual Memory:\t");
          ImGui::EndGroup     ();

          ImGui::SameLine     ();

          ImGui::BeginGroup   ();
          if (ReadAcquire64 (&pTLS->memory->local_bytes)   != 0) ImGui::TextUnformatted (SK_File_SizeToStringAF (ReadAcquire64 (&pTLS->memory->local_bytes),   2, 3, Auto, pTLS).data ());
          if (ReadAcquire64 (&pTLS->memory->global_bytes)  != 0) ImGui::TextUnformatted (SK_File_SizeToStringAF (ReadAcquire64 (&pTLS->memory->global_bytes),  2, 3, Auto, pTLS).data ());
          if (ReadAcquire64 (&pTLS->memory->heap_bytes)    != 0) ImGui::TextUnformatted (SK_File_SizeToStringAF (ReadAcquire64 (&pTLS->memory->heap_bytes),    2, 3, Auto, pTLS).data ());
          if (ReadAcquire64 (&pTLS->memory->virtual_bytes) != 0) ImGui::TextUnformatted (SK_File_SizeToStringAF (ReadAcquire64 (&pTLS->memory->virtual_bytes), 2, 3, Auto, pTLS).data ());
          ImGui::EndGroup     ();

          ImGui::SameLine     ();

          ImGui::BeginGroup   ();
          if (ReadAcquire64 (&pTLS->memory->local_bytes)   != 0) ImGui::TextUnformatted ("\t(Lifetime)");
          if (ReadAcquire64 (&pTLS->memory->global_bytes)  != 0) ImGui::TextUnformatted ("\t(Lifetime)");
          if (ReadAcquire64 (&pTLS->memory->heap_bytes)    != 0) ImGui::TextUnformatted ("\t(Lifetime)");
          if (ReadAcquire64 (&pTLS->memory->virtual_bytes) != 0) ImGui::TextUnformatted ("\t(In-Use Now)");
          ImGui::EndGroup     ();
        }

        if ( ReadAcquire64 (&pTLS->disk->bytes_read)    > 0 ||
             ReadAcquire64 (&pTLS->disk->bytes_written) > 0    )
        {
          ImGui::Separator ();

          ImGui::BeginGroup ();
          if (ReadAcquire64 (&pTLS->disk->bytes_read))    ImGui::TextUnformatted ("File Reads:\t\t\t");
          if (ReadAcquire64 (&pTLS->disk->bytes_written)) ImGui::TextUnformatted ("File Writes:\t\t\t");
          ImGui::EndGroup   ();

          ImGui::SameLine   ();

          ImGui::BeginGroup ();
          if (ReadAcquire64 (&pTLS->disk->bytes_read))    ImGui::TextUnformatted (SK_File_SizeToStringAF (ReadAcquire64 (&pTLS->disk->bytes_read),    2, 3, Auto, pTLS).data ());
          if (ReadAcquire64 (&pTLS->disk->bytes_written)) ImGui::TextUnformatted (SK_File_SizeToStringAF (ReadAcquire64 (&pTLS->disk->bytes_written), 2, 3, Auto, pTLS).data ());
          ImGui::EndGroup   ();
        }

        if ( ReadAcquire64 (&pTLS->net->bytes_received) > 0 ||
             ReadAcquire64 (&pTLS->net->bytes_sent)     > 0 )
        {
          ImGui::Separator ();

          ImGui::BeginGroup ();
          if (ReadAcquire64 (&pTLS->net->bytes_sent))     ImGui::TextUnformatted ("Network Sent:\t");
          if (ReadAcquire64 (&pTLS->net->bytes_received)) ImGui::TextUnformatted ("Network Received:\t");
          ImGui::EndGroup   ();

          ImGui::SameLine   ();

          ImGui::BeginGroup ();
          if (ReadAcquire64 (&pTLS->net->bytes_sent))     ImGui::TextUnformatted (SK_File_SizeToStringAF (ReadAcquire64 (&pTLS->net->bytes_sent),     2, 3, Auto, pTLS).data ());
          if (ReadAcquire64 (&pTLS->net->bytes_received)) ImGui::TextUnformatted (SK_File_SizeToStringAF (ReadAcquire64 (&pTLS->net->bytes_received), 2, 3, Auto, pTLS).data ());
          ImGui::EndGroup   ();
        }

        LONG64 steam_calls = ReadAcquire64 (&pTLS->steam->callback_count);

        ULONG64 present_calls =
          ReadULong64Acquire (&pTLS->render->frames_presented);

        if (present_calls > 0)
        {
          ImGui::Separator  ();

          ImGui::BeginGroup ();
          ImGui::TextUnformatted
                            ("Frames Submitted:\t");
          ImGui::EndGroup   ();

          ImGui::SameLine   ();

          ImGui::BeginGroup ();
          ImGui::Text       ("%lli", present_calls);
          ImGui::EndGroup   ();
        }

        if ( steam_calls > 0 )
        {
          ImGui::Separator  ();

          ImGui::BeginGroup ();
          ImGui::TextUnformatted
                            ("Steam Callbacks: \t");
          ImGui::EndGroup   ();

          ImGui::SameLine   ();

          ImGui::BeginGroup ();
          ImGui::Text       ("%lli", steam_calls);
          ImGui::EndGroup   ();
        }

        LONG exceptions = ReadAcquire (&pTLS->debug.exceptions);

        if (exceptions > 0)
        {
          ImGui::Separator       ();

          ImGui::BeginGroup      ();
          ImGui::TextUnformatted ("Exceptions Raised:");
          ImGui::EndGroup        ();

          ImGui::SameLine        ();

          ImGui::BeginGroup      ();
          ImGui::Text            ("\t%li", exceptions);
          ImGui::EndGroup        ();
        }

        if (error_state.code != NO_ERROR)
        {
          ImGui::Separator  ();

          ImGui::BeginGroup ();
          ImGui::TextUnformatted
                            ("Win32 Error Code:");
          ImGui::EndGroup   ();

          ImGui::SameLine   ();

          _com_error err (error_state.code & 0xFFFF);

          ImGui::BeginGroup ();
          ImGui::Text       ("\t0x%04lx (%hs)", (error_state.code & 0xFFFF), SK_WideCharToUTF8 (err.ErrorMessage ()).c_str ());
          ImGui::EndGroup   ();

          ImGui::BeginGroup ();
          ImGui::Text       ("\t\t%hs", SKX_DEBUG_FastSymName (error_state.call_site));
          ImGui::EndGroup   ();
        }

        if (show_callstack)
        {
          SK_AutoHandle hThreadStack (
            OpenThread ( THREAD_ALL_ACCESS, false, dwSelectedTid )
          );

          if ((intptr_t)hThreadStack.m_h > 0)//!= INVALID_HANDLE_VALUE)
          {
            FILETIME ftCreateStack, ftUserStack,
                     ftKernelStack, ftExitStack;

            GetThreadTimes           ( hThreadStack.m_h, &ftCreateStack, &ftExitStack, &ftKernelStack, &ftUserStack );
            SK_ImGui_ThreadCallstack ( hThreadStack.m_h, LARGE_INTEGER {       ftUserStack.dwLowDateTime,
                                                                         (LONG)ftUserStack.dwHighDateTime   },
                                                         LARGE_INTEGER {       ftKernelStack.dwLowDateTime,
                                                                         (LONG)ftKernelStack.dwHighDateTime } );
          }
        }

#if 0
        ImGui::Separator ();
        ImGui::Text      ("Alertable Waits (APC): %lu", ReadAcquire (&pTLS->scheduler->alert_waits));
        ImGui::TreePush  ("");

        std::map <HANDLE, SK_Sched_ThreadContext::wait_record_s>
          objects_waited = { pTLS->scheduler->objects_waited->begin (),
                             pTLS->scheduler->objects_waited->end   () };

        ImGui::BeginGroup ();
        for ( auto& it : objects_waited )
        {
          ImGui::Text ("Wait Object: %x", it.first);
        }
        ImGui::EndGroup   ();
        ImGui::SameLine   ();
        ImGui::BeginGroup ();
        for ( auto& it : objects_waited )
        {
          ImGui::Text ("Waits Issued: %lu", it.second.calls);
        }
        ImGui::EndGroup   ();
        ImGui::SameLine   ();
        ImGui::BeginGroup ();
        for ( auto& it : objects_waited )
        {
          ImGui::Text ("Time Spent Waiting: %lu ms", ReadAcquire64 (&it.second.time_blocked));
        }
        ImGui::EndGroup   ();
        ImGui::TreePop    ();
#endif

        ImGui::EndTooltip   ();
      }
    };

           DWORD dwExitCode    = 0;
    static DWORD dwSelectedTid = 0;
    static UINT  uMaxCPU       = 0;

    static bool   show_exited_threads = false;
    static bool   hide_inactive       = true;
    static bool   reset_stats         = true;

    extern std::vector <SK_MMCS_TaskEntry*>
    SK_MMCS_GetTasks (void);

    const auto& tasks =
      SK_MMCS_GetTasks ();

    if ( (! tasks.empty ()) &&
         ImGui::CollapsingHeader ("Multi-Media Class Tasks", ImGuiTreeNodeFlags_DefaultOpen) )
    {
      float fSpacingMax = 0.0f;

      for ( auto& task : tasks )
      {
        if (task == nullptr || task->dwTid == 0)
          continue;

        if (task->dwTid != 0)
        {
          CHandle hThread (
            OpenThread (THREAD_QUERY_INFORMATION, FALSE, task->dwTid)
          );

          DWORD dwTaskCode = 0;
          if (                      hThread.m_h == 0           ||
              (! GetExitCodeThread (hThread.m_h, &dwTaskCode)) ||
                                                  dwTaskCode != STILL_ACTIVE)
          {
            extern bool
                SK_MMCS_RemoveTask (DWORD dwTid);
            if (SK_MMCS_RemoveTask (task->dwTid))
              continue;
          }
        }

        fSpacingMax =
          std::fmaxf ( ImGui::CalcTextSize (task->name).x,
                         fSpacingMax );
      }

      ImGui::Columns (2);//(4);
      for ( auto& task : tasks )
      {
        if (task == nullptr || task->dwTid == 0)
          continue;

        ImGui::PushID  (task->dwTid);

        static const float const_spacing =
          ImGui::CalcTextSize ("[SK] ").x;

      ImGui::SetColumnOffset ( 0, 0.0f   );
      ImGui::SetColumnOffset ( 1, fSpacingMax + const_spacing );

      ImGui::Spacing         ( ); ImGui::SameLine ();
      ImGui::TextUnformatted (task->name);
      ImGui::NextColumn      ( );

      int prio =
        int (task->priority + 2);

      if (ImGui::Combo ("Priority", &prio, "Very Low\0Low\0Normal\0High\0Critical\0\0"))
      {
        strncpy (task->change.task0, task->task0, 64);
        strncpy (task->change.task1, task->task1, 64);

        task->queuePriority (
          AVRT_PRIORITY (prio - 2)
        );
      }
      else if (! ReadAcquire (&task->change.pending))
        task->priority = task->change.priority;

        ImGui::PopID      ( );
        ImGui::NextColumn ( );
      } ImGui::Columns    (1);
    }

    if ( tasks.empty () ||
         ImGui::CollapsingHeader ("Normal Win32 Threads", ImGuiTreeNodeFlags_DefaultOpen) )
    {
      bool clear_counters = false;

      ImGui::BeginGroup ();

      ImGui::BeginGroup ();
      ImGui::BeginGroup ();
      ImGui::Checkbox   ("Show Finished", &show_exited_threads);
      extern volatile LONG _SK_TLS_AllocationFailures;
      LONG lAllocFails =
        ReadAcquire (&_SK_TLS_AllocationFailures);
      if (lAllocFails > 0)
      {
        ImGui::Text ("TLS Allocation Failures: %li", lAllocFails);
        if (ImGui::IsItemHovered ()) ImGui::SetTooltip ("Thread Local Storage is critical for Special K's thread performance.");
      }
      ImGui::EndGroup   ();
      ImGui::SameLine   ();
      ImGui::Checkbox   ("Hide Inactive", &hide_inactive);
      ImGui::SameLine   ();
      ImGui::Checkbox   ("Reset Stats Every 30 Seconds", &reset_stats);
      if (ImGui::Button ("Compact Working Set Memory"))
      {
        //SIZE_T MaximumWorkingSet = -1,
        //       MinimumWorkingSet = -1;
        //
        //HANDLE hProc   = GetCurrentProcess ();
        //DWORD  dwFlags;
        //
        //PROCESS_MEMORY_COUNTERS       pmc = { };    pmc.cb
        //                                  = sizeof (PROCESS_MEMORY_COUNTERS);
        //GetProcessMemoryInfo (hProc, &pmc,  sizeof (pmc));
        //
        //GetProcessWorkingSetSizeEx (
        //  hProc, &MinimumWorkingSet, &MaximumWorkingSet, &dwFlags
        //);
        //
        //SetProcessWorkingSetSize (
        //  hProc, MinimumWorkingSet, 9 * (pmc.PeakWorkingSetSize / 10)
        //);
        //
        //SetProcessWorkingSetSize (
        //  hProc, MinimumWorkingSet, MaximumWorkingSet
        //);

        typedef BOOL (WINAPI* K32EmptyWorkingSet_pfn)(HANDLE hProcess);
        static                K32EmptyWorkingSet_pfn
                              K32EmptyWorkingSet =
                             (K32EmptyWorkingSet_pfn) SK_GetProcAddress (
                L"kernel32", "K32EmptyWorkingSet" );

        if (K32EmptyWorkingSet != nullptr)
        {   K32EmptyWorkingSet (GetCurrentProcess ()); }
      }
      ImGui::EndGroup   ();
      ImGui::SameLine   ();
      ImGui::BeginGroup ();
      if (ImGui::Button ("Reset Performance Counters"))
        clear_counters = true;
      SK_ImGui_RebalanceThreadButton ();
      ImGui::EndGroup   ();
      ImGui::SameLine   ();
      ImGui::BeginGroup ();
      ImGui::Checkbox   ("Show Callstack Analysis", &show_callstack);

      if (ImGui::IsItemHovered ()) ImGui::SetTooltip ("This feature is experimental and may cause hitching while debug symbols are loaded.");

      static bool analytics =
        config.threads.enable_file_io_trace;

      if (ImGui::Checkbox ("Enable File I/O Analytics", &config.threads.enable_file_io_trace))
      {   SK_SaveConfig (); }
      if (analytics != config.threads.enable_file_io_trace)
      {   ImGui::BulletText ("Game Restart Required");    }
      ImGui::EndGroup   ();
      ImGui::Separator  ();
      ImGui::EndGroup   ();
      ImGui::BeginGroup ();
    if (ImGui::BeginPopup ("ThreadInspectPopup"))
    {
      SK_AutoHandle hSelectedThread (INVALID_HANDLE_VALUE);

      if (dwSelectedTid != 0)
      {
        hSelectedThread.m_h =
          OpenThread ( THREAD_QUERY_INFORMATION |
                       THREAD_SET_INFORMATION   |
                       THREAD_SUSPEND_RESUME, FALSE, dwSelectedTid );

        if (GetProcessIdOfThread (hSelectedThread.m_h) != GetCurrentProcessId ())
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
         ( SKWG_Threads->find (dwSelectedTid) != SKWG_Threads->cend () &&
           SKWG_Threads  [     dwSelectedTid]->wait_reason == WaitReason::Suspended );

        if (SK_Thread_GetCurrentId () != GetThreadId (hSelectedThread) )
        {
          if ( ImGui::Button ( suspended ?  "Resume this Thread" :
                                            "Suspend this Thread" ) )
          {
            suspended = (! suspended);

            if (suspended)
            {
              struct suspend_params_s
              {
                ULONG64 frame_requested;
                HANDLE  hThread;
                DWORD   dwTid;
                ULONG   time_requested;
              };

              static concurrency::concurrent_queue <suspend_params_s>
                to_suspend;

              to_suspend.push (
                { SK_GetFramesDrawn (), nullptr, dwSelectedTid, SK_GetCurrentMS () }
              );

              static HANDLE hThreadRecovery = INVALID_HANDLE_VALUE;
              static HANDLE hRecoveryEvent  = INVALID_HANDLE_VALUE;

              if (hThreadRecovery == INVALID_HANDLE_VALUE)
              {
                static constexpr
                  DWORD  dwMilliseconds = 500UL;
                hRecoveryEvent =
                  CreateWaitableTimer (nullptr, FALSE, nullptr);

                if (hRecoveryEvent != 0)
                {
                  LARGE_INTEGER liDelay = { };
                                liDelay.QuadPart =
                                  (LONGLONG)(-10000.0l * (long double)500UL);

                  if ( SetWaitableTimer ( hRecoveryEvent, &liDelay,
                                            dwMilliseconds, nullptr, nullptr, 0 )
                     )
                  {
                    hThreadRecovery =
                    SK_Thread_CreateEx ([](LPVOID pUser) ->
                    DWORD
                    {
                      SK_Thread_DataCollector *pDataCollector =
                        (SK_Thread_DataCollector *)pUser;

                      concurrency::concurrent_queue <suspend_params_s>* pWaitQueue =
                        (concurrency::concurrent_queue <suspend_params_s>*)&to_suspend;

                      static std::vector <suspend_params_s> suspended_threads;

                      SK_Thread_SetCurrentPriority (THREAD_PRIORITY_LOWEST);

                      do
                      {
                        SK_WaitForSingleObject (hRecoveryEvent, INFINITE);

                        while (! pWaitQueue->empty ())
                        {
                          suspend_params_s            suspend_me = { };
                          if (   pWaitQueue->try_pop (suspend_me))
                          {
                            // Two threads are always invalid for suspension:
                            //
                            //  1. The watchdog thread (this one)
                            //  2. The kernel data collection thread
                            //
                            if ( suspend_me.dwTid == SK_Thread_GetCurrentId () ||
                                 suspend_me.dwTid == pDataCollector->dwProducerTid )
                            {
                              continue;
                            }

                            SK_AutoHandle hThread__ (
                              OpenThread ( THREAD_SUSPEND_RESUME,
                                             FALSE,
                                               suspend_me.dwTid )
                            );

                            if (SuspendThread (hThread__) != -1)
                            {
                              CONTEXT                           threadCtx = { };
                              GetThreadContext (hThread__.m_h, &threadCtx);

                              suspend_me.time_requested  =
                                SK_GetCurrentMS   ();
                              suspend_me.frame_requested =
                                SK_GetFramesDrawn ();
                              suspend_me.hThread         =
                                hThread__.Detach  ();

                              suspended_threads.emplace_back (
                                suspend_me
                              );
                            }
                          }
                        }

#define CHECKUP_TIME 666UL
#define MIN_FRAMES   4

                        std::vector <suspend_params_s> leftovers;
                                                       leftovers.reserve (
                                             suspended_threads.size ()   );
                        for ( auto& thread : suspended_threads )
                        {
                          if (thread.time_requested < (SK_GetCurrentMS () - CHECKUP_TIME))
                          {
                            SK_AutoHandle hThread
                                  (thread.hThread);

                            if (thread.frame_requested > (SK_GetFramesDrawn () - MIN_FRAMES))
                            {
                              SK_ImGui_Warning (L"Unsafe thread suspension detected; thread resumed to prevent deadlock!");
                              ResumeThread (hThread.m_h);
                            }

                            continue;
                          }

                          leftovers.emplace_back (thread);
                        }

                        std::swap (leftovers, suspended_threads);
                      } while (0 == ReadAcquire (&__SK_DLL_Ending));

                      CancelWaitableTimer (hRecoveryEvent);
                      SK_CloseHandle      (hRecoveryEvent);

                      SK_Thread_CloseSelf ();

                      return 0;
                    }, L"[SK] Thread Profiler Watchdog Timer",
                         (LPVOID)data_thread.get ());
                  }
                }
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
              (pTLS->scheduler->affinity_mask & (Processor0 << j)) != 0;

            UINT i =
              SK_SetThreadIdealProcessor (hSelectedThread, MAXIMUM_PROCESSORS);

            bool ideal = (i == j);

            float c_scale =
              pTLS->scheduler->lock_affinity ? 0.5f : 1.0f;

            ImGui::TextColored ( ideal    ? ImColor (0.5f   * c_scale, 1.0f   * c_scale,   0.5f * c_scale):
                                 affinity ? ImColor (1.0f   * c_scale, 1.0f   * c_scale,   1.0f * c_scale) :
                                            ImColor (0.375f * c_scale, 0.375f * c_scale, 0.375f * c_scale),
                                   "CPU%lu",
                                     j );

            if (ImGui::IsItemClicked () && (! pTLS->scheduler->lock_affinity))
            {
              affinity = (! affinity);

              DWORD_PTR dwAffinityMask =
                pTLS->scheduler->affinity_mask;

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

          ImGui::Checkbox ("Prevent changes to affinity", &pTLS->scheduler->lock_affinity);
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
          bool boost = (bDisableBoost == FALSE);
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

        const bool contains_thread =
         ( SKWG_Threads->find (dwSelectedTid) !=
           SKWG_Threads->cend (             ) );

        bool throttle =
          ( contains_thread && SKWG_Threads [dwSelectedTid]->power_throttle );

        if (! throttle)
          if (contains_thread) SKWG_Threads [dwSelectedTid]->orig_prio = dwPrio;

        if (ImGui::Checkbox ("Enable Power Throttling", &throttle))
        {
          SetThreadPriority ( hSelectedThread, throttle ? THREAD_MODE_BACKGROUND_BEGIN | THREAD_PRIORITY_IDLE :
                                                          THREAD_MODE_BACKGROUND_END );

          if ((! throttle) && contains_thread)
          {
            SetThreadPriority ( hSelectedThread, SKWG_Threads [dwSelectedTid]->orig_prio );
          }
        }

        ImGui::EndGroup   ();
      }

      ImGui::EndPopup ();
    }

    const auto IsThreadNonIdle =
    [&](SKWG_Thread_Entry& tent) ->
    bool
    {
      if (tent.exited)
      {
        if (! show_exited_threads)
          return false;
      }

      else
      {
        if ( tent.hThread == nullptr ||
             tent.hThread == INVALID_HANDLE_VALUE )
        {    tent.exited   = true;                }
      }

      if (tent.exited && (! show_exited_threads))
        return false;

      static constexpr DWORD IDLE_PERIOD = 1500UL;

      if (! hide_inactive)
        return true;

      if (tent.last_nonzero > dwNow - IDLE_PERIOD)
        return true;

      return false;
    };

    ImGui::BeginGroup ();
    for ( auto& it : *SKWG_Ordered_Threads )
    {
      if (it.second == nullptr)
        continue;

      if (! it.second->exited)
      {
        HANDLE hThread =
          OpenThread ( THREAD_ALL_ACCESS, FALSE, it.second->dwTid );

        if ( hThread == nullptr ||
             hThread == INVALID_HANDLE_VALUE )
        {
          it.second->exited = true;
        }

        else
          it.second->hThread = hThread;
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
             it.second->hThread =   nullptr;
             dwExitCode         = (DWORD)-1;
          }
        }
      } else { dwExitCode =  0; }


      if (SKWG_Threads->find (it.second->dwTid) != SKWG_Threads->cend () &&
          SKWG_Threads       [it.second->dwTid]->wait_reason == WaitReason::Suspended)
        ImGui::PushStyleColor (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (0.85f, 0.95f, 0.99f));
      else if (dwExitCode == STILL_ACTIVE)
        ImGui::PushStyleColor (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (0.3f, 0.95f, 0.99f));
      else
      {
        it.second->exited = true;
        ImGui::PushStyleColor (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (0.15f, 0.95f, 0.99f));
      }

      bool selected = false;
      if (ImGui::Selectable (SK_FormatString ("Thread %x###thr_%x", it.second->dwTid,it.second->dwTid).c_str (), &selected))
      {
        ImGui::PopStyleColor     ();
        ImGui::PushStyleColor    (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (0.5f, 0.99f, 0.999f));

        // We sure as hell cannot suspend the thread that's drawing the UI! :)
        if ( dwExitCode            == STILL_ACTIVE )
        {
          ImGui::OpenPopup ("ThreadInspectPopup");
        }

        dwSelectedTid =
          it.second->dwTid;

        ImGui::SetNextWindowSize (ImVec2 (-1.0f, -1.0f), ImGuiCond_Always);
      }

      ImGui::PopStyleColor ();

      if (ImGui::IsItemHovered ())
      { if (                           NO_ERROR == dwErrorCode ) ThreadMemTooltip (it.second->dwTid);
        else ImGui::SetTooltip ("Error Code: %lu", dwErrorCode);
      }

    }
    ImGui::EndGroup   (); ImGui::SameLine ();


    ImGui::BeginGroup ( );

    for ( auto& it : *SKWG_Ordered_Threads )
    {
      if (it.second == nullptr)
        continue;

      if (! IsThreadNonIdle (*it.second)) continue;

      if (! it.second->self_titled)
      {
        auto& thread_name = SK_Thread_GetName       (it.second->dwTid);
        auto  self_titled = SK_Thread_HasCustomName (it.second->dwTid);

        if (! thread_name.empty ())
        {
          it.second->name        = thread_name;
          it.second->self_titled = self_titled;
        }
      }

      if (it.second->self_titled)
      {
        it.second->name        = SK_Thread_GetName       (it.second->dwTid); // Name may have changed
        ImGui::PushStyleColor (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (0.572222f, 0.63f, 0.95f));
      }
      else
        ImGui::PushStyleColor (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (0.472222f, 0.23f, 0.91f));

      if (it.second->name.length () > 1)
        ImGui::Text ("%hs", SK_WideCharToUTF8 (it.second->name).c_str ());
      else
      {
        static NtQueryInformationThread_pfn
               NtQueryInformationThread =
              (NtQueryInformationThread_pfn)SK_GetProcAddress ( L"NtDll",
              "NtQueryInformationThread" );

        DWORD_PTR pdwStartAddress
                          = NULL;
        HANDLE    hThread = it.second->hThread;
        NTSTATUS ntStatus =
          NtQueryInformationThread (
            hThread, ThreadQuerySetWin32StartAddress,
              &pdwStartAddress, sizeof (DWORD_PTR), nullptr
          );

        if (STATUS_SUCCESS == ntStatus)
        {
          char  thread_name [MAX_THREAD_NAME_LEN] = { };
          char  szSymbol    [256]                 = { };
          ULONG ulLen                             = 191;

          SK::Diagnostics::CrashHandler::InitSyms ();

          ulLen = SK_GetSymbolNameFromModuleAddr (
                  SK_GetModuleFromAddr ((LPCVOID)pdwStartAddress),
          reinterpret_cast <uintptr_t> ((LPCVOID)pdwStartAddress),
                        szSymbol,
                          ulLen );

          if (ulLen > 0)
          {
            snprintf ( thread_name, MAX_THREAD_NAME_LEN-1, "%s+%s",
                         SK_WideCharToUTF8 (SK_GetCallerName ((LPCVOID)pdwStartAddress)).c_str ( ),
                                                                 szSymbol );
          }

          else {
            snprintf ( thread_name, MAX_THREAD_NAME_LEN-1, "%s",
                         SK_WideCharToUTF8 (SK_GetCallerName ((LPCVOID)pdwStartAddress)).c_str ( ) );
          }

          SK_TLS* pTLS =
            SK_TLS_BottomEx (it.second->dwTid);

          if (pTLS != nullptr)
            wcsncpy_s ( pTLS->debug.name,               MAX_THREAD_NAME_LEN-1,
                        SK_UTF8ToWideChar (thread_name).c_str (), _TRUNCATE );

          if (_SK_ThreadNames->find (it.second->dwTid) == _SK_ThreadNames->cend ())
          {
            _SK_ThreadNames [it.second->dwTid] =
              SK_UTF8ToWideChar (thread_name);

            it.second->name = _SK_ThreadNames [it.second->dwTid];
          }
        }

        ImGui::TextUnformatted ("");
      }

      ImGui::PopStyleColor ();

      if (ImGui::IsItemHovered ()) ThreadMemTooltip (it.second->dwTid);
    }
    ImGui::EndGroup   (); ImGui::SameLine ();


    ImGui::BeginGroup ();
    for (auto& it : *SKWG_Ordered_Threads)
    {
      if (! IsThreadNonIdle (*it.second)) continue;

      if (it.second->hThread == nullptr)
        continue;

      UINT i =
        SK_SetThreadIdealProcessor (it.second->hThread, MAXIMUM_PROCESSORS);

      if (i == (UINT)-1)
        i = 0;

      if (! it.second->exited)
      {
        ImGui::TextColored ( ImColor::HSV ( (float)( i       + 1 ) /
                                            (float)( uMaxCPU + 1 ), 0.5f, 1.0f ),
                               "CPU %u", i );
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
    for ( auto& it : *SKWG_Ordered_Threads )
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
          prio_txt = "Idle";
          break;
        case THREAD_PRIORITY_LOWEST:
          //prio_txt = std::move ("THREAD_PRIORITY_LOWEST");
          prio_txt = "Lowest";
          break;
        case THREAD_PRIORITY_BELOW_NORMAL:
          //prio_txt = std::move ("THREAD_PRIORITY_BELOW_NORMAL");
          prio_txt = "Below Normal";
          break;
        case THREAD_PRIORITY_NORMAL:
          //prio_txt = std::move ("THREAD_PRIORITY_NORMAL");
          prio_txt = "Normal";
          break;
        case THREAD_PRIORITY_ABOVE_NORMAL:
          //prio_txt = std::move ("THREAD_PRIORITY_ABOVE_NORMAL");
          prio_txt = "Above Normal";
          break;
        case THREAD_PRIORITY_HIGHEST:
          //prio_txt = std::move ("THREAD_PRIORITY_HIGHEST");
          prio_txt = "Highest";
          break;
        case THREAD_PRIORITY_TIME_CRITICAL:
          //prio_txt = std::move ("THREAD_PRIORITY_TIME_CRITICAL");
          prio_txt = "Time Critical";
          break;
        default:
          if (! it.second->exited)
            prio_txt = SK_FormatString ("%d", dwPrio);
          else
            prio_txt = (const char *)u8"ⁿ/ₐ";
          break;
      }

      ImGui::Text ("  %s  ", prio_txt.c_str ());

      if (ImGui::IsItemHovered ()) ThreadMemTooltip (it.second->dwTid);
    }
    ImGui::EndGroup (); ImGui::SameLine ();


    static DWORD    dwLastSnap = 0;
    static FILETIME ftSnapKernel, ftSnapUser;

    static auto& _process_time =
                  process_time.get ();

    if ( (reset_stats && (dwLastSnap < dwNow - SNAP_FREQUENCY)) ||
          clear_counters )
    {
      ftSnapKernel = _process_time.ftKernel;
      ftSnapUser   = _process_time.ftUser;
    }

    LARGE_INTEGER liKernel; liKernel.LowPart  = _process_time.ftKernel.dwLowDateTime;
                            liKernel.HighPart = _process_time.ftKernel.dwHighDateTime;

                            LARGE_INTEGER liSnapDeltaK;
                                          liSnapDeltaK.LowPart  = ftSnapKernel.dwLowDateTime;
                                          liSnapDeltaK.HighPart = ftSnapKernel.dwHighDateTime;

                                          liKernel.QuadPart -= liSnapDeltaK.QuadPart;

    LARGE_INTEGER liUser;   liUser.LowPart    = _process_time.ftUser.dwLowDateTime;
                            liUser.HighPart   = _process_time.ftUser.dwHighDateTime;

                            LARGE_INTEGER liSnapDeltaU;
                                          liSnapDeltaU.LowPart  = ftSnapUser.dwLowDateTime;
                                          liSnapDeltaU.HighPart = ftSnapUser.dwHighDateTime;

                                          liUser.QuadPart -= liSnapDeltaU.QuadPart;

    long double p_kmax = 0.0;
    long double p_umax = 0.0;

    for ( auto& it : *SKWG_Ordered_Threads )
    {
      if ( (reset_stats && (dwLastSnap < dwNow - SNAP_FREQUENCY)) ||
            clear_counters )
      {
        it.second->runtimes.snapshot.kernel = it.second->runtimes.kernel;
        it.second->runtimes.snapshot.user   = it.second->runtimes.user;
      }

      LARGE_INTEGER liThreadKernel,
                    liThreadUser;

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
        std::min ( 100.0L,
          100.0L * static_cast <long double> ( liThreadKernel.QuadPart ) /
                   static_cast <long double> (       liKernel.QuadPart ) );

      it.second->runtimes.percent_user =
        std::min ( 100.0L,
          100.0L * static_cast <long double> ( liThreadUser.QuadPart ) /
                   static_cast <long double> (       liUser.QuadPart ) );

      p_kmax = std::fmax (p_kmax, it.second->runtimes.percent_kernel);
      p_umax = std::fmax (p_umax, it.second->runtimes.percent_user);

      if (it.second->runtimes.percent_kernel >= 0.001) it.second->last_nonzero = dwNow;
      if (it.second->runtimes.percent_user   >= 0.001) it.second->last_nonzero = dwNow;
    }


    ImGui::BeginGroup ();
    for ( auto& it : *SKWG_Ordered_Threads )
    {
      if (! IsThreadNonIdle (*it.second)) continue;

      if (it.second->runtimes.percent_kernel == p_kmax)
        ImGui::PushStyleColor (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (0.0944444f, 0.79f, 1.0f));

      if (it.second->runtimes.percent_kernel >= 0.001)
        ImGui::Text ("%6.2Lf%% Kernel", it.second->runtimes.percent_kernel);
      else
        ImGui::TextUnformatted ("-");

      if (it.second->runtimes.percent_kernel == p_kmax)
        ImGui::PopStyleColor ();

      if (ImGui::IsItemHovered ()) ThreadMemTooltip (it.second->dwTid);
    }
    ImGui::EndGroup (); ImGui::SameLine ();


    ImGui::BeginGroup ();
    for ( auto& it : *SKWG_Ordered_Threads )
    {
      if (! IsThreadNonIdle (*it.second)) continue;

      if (it.second->runtimes.percent_user == p_umax)
        ImGui::PushStyleColor (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (0.0944444f, 0.79f, 1.0f));

      if (it.second->runtimes.percent_user >= 0.001)
        ImGui::Text ("%6.2Lf%% User", it.second->runtimes.percent_user);
      else
        ImGui::TextUnformatted ("-");

      if (it.second->runtimes.percent_user == p_umax)
        ImGui::PopStyleColor ();

      if (ImGui::IsItemHovered ()) ThreadMemTooltip (it.second->dwTid);
    }
    ImGui::EndGroup ();    ImGui::SameLine ();


    ImGui::BeginGroup ();
    for (auto& it : *SKWG_Ordered_Threads)
    {
      if (! IsThreadNonIdle (*it.second)) continue;

      HANDLE hThread       = it.second->hThread;
      BOOL   bDisableBoost = FALSE;

      if (GetThreadPriorityBoost (hThread, &bDisableBoost) && (! bDisableBoost))
      {
        ImGui::TextColored (ImColor::HSV (1.0f, 0.0f, 0.38f), " (Dynamic Boost)");
      }

      else
        ImGui::TextUnformatted ("");

      if (ImGui::IsItemHovered ()) ThreadMemTooltip (it.second->dwTid);
    }
    ImGui::EndGroup ();    ImGui::SameLine ();


    // Used to determine whether to srhink the dialog box;
    //
    //   Waiting I/O is a status that frequently comes and goes, but we don't
    //     want the widget rapidly resizing itself, so we need a grace period.
    constexpr DWORD WAIT_GRACE    = 666UL;
    static    DWORD dwLastWaiting = 0;

    ImGui::BeginGroup ();
    for (auto& it : *SKWG_Ordered_Threads)
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
        ImGui::TextUnformatted ("");
      else
        // Alpha: 0  ==>  Hide this text, it's for padding only.
        ImGui::TextColored (ImColor (1.0f, 0.090196f, 0.364706f, 0.0f), "Waiting I/O");

      ImGui::SameLine ();

      if (it.second->power_throttle)
        ImGui::TextColored (ImColor (0.090196f, 1.0f, 0.364706f), "Power-Throttle");
      else
        ImGui::TextUnformatted ("");

      if (ImGui::IsItemHovered ()) ThreadMemTooltip (it.second->dwTid);
    }
    ImGui::EndGroup   ();

  //ImGui::EndChildFrame ();
    ImGui::EndGroup      ();

    for (auto& it : *SKWG_Ordered_Threads)
    {
      if ( it.second->hThread != nullptr &&
           it.second->hThread != INVALID_HANDLE_VALUE )
      {
        SK_CloseHandle (it.second->hThread);
                        it.second->hThread = INVALID_HANDLE_VALUE;
      }
    }

    if ( (reset_stats && (dwLastSnap < dwNow - SNAP_FREQUENCY)) ||
          clear_counters )
    {
      dwLastSnap = dwNow;
    }
    }

    // No maximum size
    setMaxSize (ImGui::GetIO ().DisplaySize);
  }


  void OnConfig (ConfigEvent event) noexcept override
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
  //
  // Producer / Consumer for NtQuerySystemInformation
  //
  std::unique_ptr <SK_Thread_DataCollector> data_thread;
  std::mutex                                run_lock;

private:
};

SK_LazyGlobal <SKWG_Thread_Profiler> __thread_profiler__;

void SK_Widget_InitThreadProfiler (void)
{
  SK_RunOnce (__thread_profiler__.get ());
}

void SK_Widget_InvokeThreadProfiler (void)
{
  if (SK_GetHostAppUtil ()->isInjectionTool ())
    return;

  static
    SK_AutoHandle produce_event (
                 SK_CreateEvent ( nullptr, FALSE,
                                           FALSE, nullptr ) );

  SK_RunOnce (
    SK_Thread_CreateEx (
     [ ](LPVOID)
   -> DWORD
      {
        HANDLE wait_objs [] =   {
              produce_event.m_h,
         __SK_DLL_TeardownEvent };

        DWORD  dwRet  = WAIT_OBJECT_0;
        while (dwRet == WAIT_OBJECT_0)
        {      dwRet  =
          WaitForMultipleObjects (
            2, wait_objs, FALSE, INFINITE
          );

          if (dwRet == WAIT_OBJECT_0)
          {
            extern volatile LONG          lLastThreadCreate;
            InterlockedIncrementAcquire (&lLastThreadCreate);

            if (ReadAcquire (&__SK_DLL_Ending) == FALSE)
            {
              SK_Widget_InitThreadProfiler ();
                  __thread_profiler__->run ();

              continue;
            }
          }
        }

        SK_Thread_CloseSelf ();

        return 0;
      }, L"[SK] Thread Profiler Data Broker"
    )
  );

  SetEvent (produce_event.m_h);
}