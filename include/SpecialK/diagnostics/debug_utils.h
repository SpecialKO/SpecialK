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

#ifndef __SK__DEBUG_UTILS_H__
#define __SK__DEBUG_UTILS_H__

#include <Unknwnbase.h>
#include <shtypes.h>

#include <Windows.h>
#include <SpecialK/tls.h>
#include <concurrent_unordered_map.h>

namespace SK
{
  namespace Diagnostics
  {
    namespace Debugger
    {
      bool Allow        (bool bAllow = true);
      void SpawnConsole (void);
      BOOL CloseConsole (void);

      extern FILE *fStdIn,
                  *fStdOut,
                  *fStdErr;
    };
  }
}

#pragma pack (push,8)
#ifndef SK_UNICODE_STR
#define SK_UNICODE_STR
typedef struct _UNICODE_STRING_SK {
  USHORT           Length;
  USHORT           MaximumLength;
  PWSTR            Buffer;
} UNICODE_STRING_SK;

typedef _UNICODE_STRING_SK* PUNICODE_STRING_SK;
#endif
#pragma pack (pop)

void
WINAPI
SK_InitUnicodeString ( PUNICODE_STRING_SK DestinationString,
                       PCWSTR             SourceString );

void
WINAPI
SK_FreeUnicodeString ( PUNICODE_STRING_SK UnicodeString );

typedef NTSTATUS (NTAPI* LdrGetDllHandle_pfn)(
  IN  PWSTR              DllPath OPTIONAL,
  IN  PULONG             DllCharacteristics OPTIONAL,
  IN  PUNICODE_STRING_SK DllName,
  OUT HMODULE           *DllHandle);

typedef struct _PEB_LDR_DATA_SK {
  ULONG      Length;
  BOOLEAN    Initialized;
  PVOID      SsHandle;
  LIST_ENTRY InLoadOrderModuleList;
  LIST_ENTRY InMemoryOrderModuleList;
  LIST_ENTRY InInitializationOrderModuleList;
} PEB_LDR_DATA_SK,
*PPEB_LDR_DATA_SK;

typedef enum _LDR_DDAG_STATE
{
  LdrModulesMerged                 = -5,
  LdrModulesInitError              = -4,
  LdrModulesSnapError              = -3,
  LdrModulesUnloaded               = -2,
  LdrModulesUnloading              = -1,
  LdrModulesPlaceHolder            =  0,
  LdrModulesMapping                =  1,
  LdrModulesMapped                 =  2,
  LdrModulesWaitingForDependencies =  3,
  LdrModulesSnapping               =  4,
  LdrModulesSnapped                =  5,
  LdrModulesCondensed              =  6,
  LdrModulesReadyToInit            =  7,
  LdrModulesInitializing           =  8,
  LdrModulesReadyToRun             =  9
} LDR_DDAG_STATE;

typedef enum _LDR_DLL_LOAD_REASON
{
  LoadReasonStaticDependency,
  LoadReasonStaticForwarderDependency,
  LoadReasonDynamicForwarderDependency,
  LoadReasonDelayloadDependency,
  LoadReasonDynamicLoad,
  LoadReasonAsImageLoad,
  LoadReasonAsDataLoad,
  LoadReasonEnclavePrimary, // REDSTONE3
  LoadReasonEnclaveDependency,
  LoadReasonUnknown = -1
} LDR_DLL_LOAD_REASON,
*PLDR_DLL_LOAD_REASON;

typedef struct _RTL_BALANCED_NODE
{
  union
  {
    struct _RTL_BALANCED_NODE   *Children [2];
    struct
    {
      struct _RTL_BALANCED_NODE *Left;
      struct _RTL_BALANCED_NODE *Right;
    };
  };

  union
  {
    UCHAR     Red     : 1;
    UCHAR     Balance : 2;
    ULONG_PTR ParentValue;
  };
} RTL_BALANCED_NODE,
*PRTL_BALANCED_NODE;

typedef struct _LDR_SERVICE_TAG_RECORD
{
  _LDR_SERVICE_TAG_RECORD *Next;
  ULONG                    ServiceTag;
} LDR_SERVICE_TAG_RECORD,
*PLDR_SERVICE_TAG_RECORD;

typedef struct _LDRP_CSLIST
{
  PSINGLE_LIST_ENTRY Tail;
} LDRP_CSLIST,
*PLDRP_CSLIST;

typedef struct _LDR_DDAG_NODE
{
  LIST_ENTRY              Modules;
  PLDR_SERVICE_TAG_RECORD ServiceTagList;
  ULONG                   LoadCount;
  ULONG                   ReferenceCount;
  ULONG                   DependencyCount;
  union
  {
    LDRP_CSLIST           Dependencies;
    SINGLE_LIST_ENTRY     RemovalLink;
  };
  LDRP_CSLIST             IncomingDependencies;
  LDR_DDAG_STATE          State;
  SINGLE_LIST_ENTRY       CondenseLink;
  ULONG                   PreorderNumber;
  ULONG                   LowestLink;
} LDR_DDAG_NODE,
*PLDR_DDAG_NODE;

typedef struct _LDR_DATA_TABLE_ENTRY__SK
{
  LIST_ENTRY             InLoadOrderLinks;
  LIST_ENTRY             InMemoryOrderLinks;
  union
  {
    LIST_ENTRY           InInitializationOrderLinks;
    LIST_ENTRY           InProgressLinks;
  };
  PVOID                  DllBase;
  PVOID                  EntryPoint;
  ULONG                  SizeOfImage;
  UNICODE_STRING_SK      FullDllName;
  UNICODE_STRING_SK      BaseDllName;
  union
  {
    UCHAR                FlagGroup [4];
    ULONG                Flags;
    struct
    {
      ULONG              PackagedBinary          : 1;
      ULONG              MarkedForRemoval        : 1;
      ULONG              ImageDll                : 1;
      ULONG              LoadNotificationsSent   : 1;
      ULONG              TelemetryEntryProcessed : 1;
      ULONG              ProcessStaticImport     : 1;
      ULONG              InLegacyLists           : 1;
      ULONG              InIndexes               : 1;
      ULONG              ShimDll                 : 1;
      ULONG              InExceptionTable        : 1;
      ULONG              ReservedFlags1          : 2;
      ULONG              LoadInProgress          : 1;
      ULONG              LoadConfigProcessed     : 1;
      ULONG              EntryProcessed          : 1;
      ULONG              ProtectDelayLoad        : 1;
      ULONG              ReservedFlags3          : 2;
      ULONG              DontCallForThreads      : 1;
      ULONG              ProcessAttachCalled     : 1;
      ULONG              ProcessAttachFailed     : 1;
      ULONG              CorDeferredValidate     : 1;
      ULONG              CorImage                : 1;
      ULONG              DontRelocate            : 1;
      ULONG              CorILOnly               : 1;
      ULONG              ReservedFlags5          : 3;
      ULONG              Redirected              : 1;
      ULONG              ReservedFlags6          : 2;
      ULONG              CompatDatabaseProcessed : 1;
    };
  };

  USHORT                 ObsoleteLoadCount;
  USHORT                 TlsIndex;
  LIST_ENTRY             HashLinks;
  ULONG                  TimeDateStamp;
  struct
    _ACTIVATION_CONTEXT *EntryPointActivationContext;
  PVOID                  Lock;
  PLDR_DDAG_NODE         DdagNode;
  LIST_ENTRY             NodeModuleLink;
  struct
    _LDRP_LOAD_CONTEXT  *LoadContext;
  PVOID                  ParentDllBase;
  PVOID                  SwitchBackContext;
  RTL_BALANCED_NODE      BaseAddressIndexNode;
  RTL_BALANCED_NODE      MappingInfoIndexNode;
  ULONG_PTR              OriginalBase;
  LARGE_INTEGER          LoadTime;
  ULONG                  BaseNameHashValue;
  LDR_DLL_LOAD_REASON    LoadReason;
  ULONG                  ImplicitPathOptions;
  ULONG                  ReferenceCount;
  ULONG                  DependentLoadFlags;
  UCHAR                  SigningLevel; // since REDSTONE2
} LDR_DATA_TABLE_ENTRY__SK,
*PLDR_DATA_TABLE_ENTRY__SK;

typedef struct _API_SET_NAMESPACE
{
  ULONG Version;
  ULONG Size;
  ULONG Flags;
  ULONG Count;
  ULONG EntryOffset;
  ULONG HashOffset;
  ULONG HashFactor;
} API_SET_NAMESPACE,
*PAPI_SET_NAMESPACE;

typedef struct _API_SET_HASH_ENTRY
{
  ULONG Hash;
  ULONG Index;
} API_SET_HASH_ENTRY,
*PAPI_SET_HASH_ENTRY;

typedef struct _API_SET_NAMESPACE_ENTRY
{
  ULONG Flags;
  ULONG NameOffset;
  ULONG NameLength;
  ULONG HashedLength;
  ULONG ValueOffset;
  ULONG ValueCount;
} API_SET_NAMESPACE_ENTRY,
*PAPI_SET_NAMESPACE_ENTRY;

typedef struct _API_SET_VALUE_ENTRY
{
  ULONG Flags;
  ULONG NameOffset;
  ULONG NameLength;
  ULONG ValueOffset;
  ULONG ValueLength;
} API_SET_VALUE_ENTRY,
*PAPI_SET_VALUE_ENTRY;

#define GDI_HANDLE_BUFFER_SIZE32 34
#define GDI_HANDLE_BUFFER_SIZE64 60
#define GDI_HANDLE_BUFFER_SIZE   GDI_HANDLE_BUFFER_SIZE32

typedef ULONG GDI_HANDLE_BUFFER   [GDI_HANDLE_BUFFER_SIZE  ];
typedef ULONG GDI_HANDLE_BUFFER32 [GDI_HANDLE_BUFFER_SIZE32];
typedef ULONG GDI_HANDLE_BUFFER64 [GDI_HANDLE_BUFFER_SIZE64];

typedef struct _CURDIR
{
  UNICODE_STRING_SK DosPath;
  PVOID             Handle;
} CURDIR,
*PCURDIR;

typedef struct _RTL_USER_PROCESS_PARAMETERS_SK
{
  ULONG             AllocationSize;
  ULONG             Size;
  ULONG             Flags;
  ULONG             DebugFlags;
  HANDLE            ConsoleHandle;
  ULONG             ConsoleFlags;
  HANDLE            hStdInput;
  HANDLE            hStdOutput;
  HANDLE            hStdError;
  CURDIR            CurrentDirectory;
  UNICODE_STRING_SK DllPath;
  UNICODE_STRING_SK ImagePathName;
  UNICODE_STRING_SK CommandLine;
  PWSTR             Environment;
  ULONG             dwX;
  ULONG             dwY;
  ULONG             dwXSize;
  ULONG             dwYSize;
  ULONG             dwXCountChars;
  ULONG             dwYCountChars;
  ULONG             dwFillAttribute;
  ULONG             dwFlags;
  ULONG             wShowWindow;
  UNICODE_STRING_SK WindowTitle;
  UNICODE_STRING_SK Desktop;
  UNICODE_STRING_SK ShellInfo;
  UNICODE_STRING_SK RuntimeInfo;
//RTL_DRIVE_LETTER_CURDIR DLCurrentDirectory[0x20]; // Don't care
} SK_RTL_USER_PROCESS_PARAMETERS,
*SK_PRTL_USER_PROCESS_PARAMETERS;

extern volatile PVOID __SK_GameBaseAddr;

typedef struct _SK_PEB
{
  BOOLEAN                      InheritedAddressSpace;
  BOOLEAN                      ReadImageFileExecOptions;
  BOOLEAN                      BeingDebugged;
  union
  {
    BOOLEAN                    BitField;
    struct
    {
      BOOLEAN ImageUsesLargePages          : 1;
      BOOLEAN IsProtectedProcess           : 1;
      BOOLEAN IsImageDynamicallyRelocated  : 1;
      BOOLEAN SkipPatchingUser32Forwarders : 1;
      BOOLEAN IsPackagedProcess            : 1;
      BOOLEAN IsAppContainer               : 1;
      BOOLEAN IsProtectedProcessLight      : 1;
      BOOLEAN IsLongPathAwareProcess       : 1;
    };
  };

  HANDLE Mutant;

  PVOID                        ImageBaseAddress;
  PPEB_LDR_DATA_SK             Ldr;
SK_PRTL_USER_PROCESS_PARAMETERS
                               ProcessParameters;
  PVOID                        SubSystemData;
  PVOID                        ProcessHeap;

  PRTL_CRITICAL_SECTION        FastPebLock;

  PVOID                        IFEOKey;
  PSLIST_HEADER                AtlThunkSListPtr;
  union
  {
    ULONG                      CrossProcessFlags;
    struct
    {
      ULONG ProcessInJob               :  1;
      ULONG ProcessInitializing        :  1;
      ULONG ProcessUsingVEH            :  1;
      ULONG ProcessUsingVCH            :  1;
      ULONG ProcessUsingFTH            :  1;
      ULONG ProcessPreviouslyThrottled :  1;
      ULONG ProcessCurrentlyThrottled  :  1;
      ULONG ProcessImagesHotPatched    :  1;
      ULONG ReservedBits0              : 24;
    };
  };
  union
  {
    PVOID               KernelCallbackTable;
    PVOID               UserSharedInfoPtr;
  };
  ULONG                 SystemReserved;
  ULONG                 AtlThunkSListPtr32;

  PAPI_SET_NAMESPACE    ApiSetMap;

  ULONG                 TlsExpansionCounter;
  PVOID                 TlsBitmap;
  ULONG                 TlsBitmapBits [2];

  PVOID                 ReadOnlySharedMemoryBase;
  PVOID                 SharedData;
  PVOID                *ReadOnlyStaticServerData;

  PVOID                 AnsiCodePageData;
  PVOID                 OemCodePageData;
  PVOID                 UnicodeCaseTableData;

  ULONG                 NumberOfProcessors;
  ULONG                 NtGlobalFlag;

  ULARGE_INTEGER        CriticalSectionTimeout;
  SIZE_T                HeapSegmentReserve;
  SIZE_T                HeapSegmentCommit;
  SIZE_T                HeapDeCommitTotalFreeThreshold;
  SIZE_T                HeapDeCommitFreeBlockThreshold;

  ULONG                 NumberOfHeaps;
  ULONG                 MaximumNumberOfHeaps;
  PVOID                *ProcessHeaps; // PHEAP

  PVOID                 GdiSharedHandleTable;
  PVOID                 ProcessStarterHelper;
  ULONG                 GdiDCAttributeList;

  PRTL_CRITICAL_SECTION LoaderLock;

  ULONG                 OSMajorVersion;
  ULONG                 OSMinorVersion;
  USHORT                OSBuildNumber;
  USHORT                OSCSDVersion;
  ULONG                 OSPlatformId;
  ULONG                 ImageSubsystem;
  ULONG                 ImageSubsystemMajorVersion;
  ULONG                 ImageSubsystemMinorVersion;
  ULONG_PTR             ActiveProcessAffinityMask;
  GDI_HANDLE_BUFFER     GdiHandleBuffer;
  PVOID                 PostProcessInitRoutine;

  PVOID                 TlsExpansionBitmap;
  ULONG                 TlsExpansionBitmapBits [32];

  ULONG                 SessionId;

  ULARGE_INTEGER        AppCompatFlags;
  ULARGE_INTEGER        AppCompatFlagsUser;
  PVOID                 pShimData;
  PVOID                 AppCompatInfo; // APPCOMPAT_EXE_DATA

  UNICODE_STRING_SK     CSDVersion;

  PVOID                 ActivationContextData;              // ACTIVATION_CONTEXT_DATA
  PVOID                 ProcessAssemblyStorageMap;          // ASSEMBLY_STORAGE_MAP
  PVOID                 SystemDefaultActivationContextData; // ACTIVATION_CONTEXT_DATA
  PVOID                 SystemAssemblyStorageMap;           // ASSEMBLY_STORAGE_MAP

  SIZE_T                MinimumStackCommit;

  PVOID                 SparePointers [4]; // 19H1 (previously FlsCallback to FlsHighIndex)
  ULONG                 SpareUlongs   [5]; // 19H1
  //PVOID* FlsCallback;
  //LIST_ENTRY FlsListHead;
  //PVOID FlsBitmap;
  //ULONG FlsBitmapBits[FLS_MAXIMUM_AVAILABLE / (sizeof(ULONG) * 8)];
  //ULONG FlsHighIndex;

  PVOID                 WerRegistrationData;
  PVOID                 WerShipAssertPtr;
  PVOID                 pUnused; // pContextData
  PVOID                 pImageHeaderHash;

  union
  {
    ULONG               TracingFlags;
    struct
    {
      ULONG             HeapTracingEnabled      :  1;
      ULONG             CritSecTracingEnabled   :  1;
      ULONG             LibLoaderTracingEnabled :  1;
      ULONG             SpareTracingBits        : 29;
    };
  };
  ULONGLONG             CsrServerReadOnlySharedMemoryBase;
  PRTL_CRITICAL_SECTION TppWorkerpListLock;
  LIST_ENTRY            TppWorkerpList;
  PVOID                 WaitOnAddressHashTable [128];
  PVOID                 TelemetryCoverageHeader;            // REDSTONE3
  ULONG                 CloudFileFlags;
  ULONG                 CloudFileDiagFlags;                 // REDSTONE4
  CHAR                  PlaceholderCompatibilityMode;
  CHAR                  PlaceholderCompatibilityModeReserved [7];

  struct _LEAP_SECOND_DATA *LeapSecondData; // REDSTONE5
  union
  {
    ULONG               LeapSecondFlags;
    struct
    {
      ULONG SixtySecondEnabled :  1;
      ULONG Reserved           : 31;
    };
  };
  ULONG                 NtGlobalFlag2;
} SK_PEB,
*SK_PPEB;

#define LDR_LOCK_LOADER_LOCK_DISPOSITION_INVALID           0
#define LDR_LOCK_LOADER_LOCK_DISPOSITION_LOCK_ACQUIRED     1
#define LDR_LOCK_LOADER_LOCK_DISPOSITION_LOCK_NOT_ACQUIRED 2

#define LDR_LOCK_LOADER_LOCK_FLAG_RAISE_ON_ERRORS   0x00000001
#define LDR_LOCK_LOADER_LOCK_FLAG_TRY_ONLY          0x00000002
 
#define LDR_UNLOCK_LOADER_LOCK_FLAG_RAISE_ON_ERRORS 0x00000001

NTSTATUS
WINAPI
SK_Module_LockLoader ( ULONG_PTR *pCookie,
                       ULONG       Flags = 0x0,
                       ULONG     *pState = nullptr );

NTSTATUS
WINAPI
SK_Module_UnlockLoader ( ULONG     Flags,
                         ULONG_PTR Cookie );

extern "C"
NTSTATUS
WINAPI
SK_NtLdr_LockLoaderLock (ULONG Flags, ULONG* State, ULONG_PTR* Cookie);

extern "C"
NTSTATUS
WINAPI
SK_NtLdr_UnlockLoaderLock (ULONG Flags, ULONG_PTR Cookie);

using LdrFindEntryForAddress_pfn    = NTSTATUS (NTAPI *)(HMODULE,PLDR_DATA_TABLE_ENTRY__SK*);

// Returns true if the host executable is Large Address Aware
// ----------------------------------------------------------
//
//  If PE is 32-bit (I386) and LAA is not flagged, then user-mode
//    code will generate an exception if it attempts to access
//      memory addresses with the high-order bit set.
//
//  This is intended primarily to prevent code that treats pointers as
//    signed integers from adding two pointers and going the wrong
//      direction.
//
//  User-mode devs. are potentially stupid and must be allowed to
//    remain stupid until they learn about special linker switches ;)
//
//
//    * In other words, will addresses > 2 GiB crash?
//
//
bool SK_PE32_IsLargeAddressAware       (void);
bool SK_PE32_MakeLargeAddressAwareCopy (void);

void WINAPI SK_SymRefreshModuleList (HANDLE hProc = GetCurrentProcess ());
BOOL WINAPI SK_IsDebuggerPresent    (void);

BOOL __stdcall SK_TerminateProcess (UINT uExitCode);

void NTAPI RtlAcquirePebLock_Detour (void);
void NTAPI RtlReleasePebLock_Detour (void);
bool   SK_Debug_CheckDebugFlagInPEB (void);

using TerminateProcess_pfn   = BOOL (WINAPI *)(HANDLE hProcess, UINT uExitCode);
using ExitProcess_pfn        = void (WINAPI *)(UINT   uExitCode);

using OutputDebugStringA_pfn = void (WINAPI *)(LPCSTR  lpOutputString);
using OutputDebugStringW_pfn = void (WINAPI *)(LPCWSTR lpOutputString);



#define _IMAGEHLP_SOURCE_
#include <DbgHelp.h>

using SymGetSearchPathW_pfn = BOOL (IMAGEAPI *)( _In_  HANDLE hProcess,
                                                 _Out_ PWSTR  SearchPath,
                                                 _In_  DWORD  SearchPathLength );

using SymSetSearchPathW_pfn = BOOL (IMAGEAPI *)( _In_     HANDLE hProcess,
                                                 _In_opt_ PCWSTR SearchPath );

using SymRefreshModuleList_pfn = BOOL (IMAGEAPI *)(HANDLE hProcess);

using StackWalk64_pfn = BOOL (IMAGEAPI *)(_In_     DWORD                            MachineType,
                                          _In_     HANDLE                           hProcess,
                                          _In_     HANDLE                           hThread,
                                          _Inout_  LPSTACKFRAME64                   StackFrame,
                                          _Inout_  PVOID                            ContextRecord,
                                          _In_opt_ PREAD_PROCESS_MEMORY_ROUTINE64   ReadMemoryRoutine,
                                          _In_opt_ PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,
                                          _In_opt_ PGET_MODULE_BASE_ROUTINE64       GetModuleBaseRoutine,
                                          _In_opt_ PTRANSLATE_ADDRESS_ROUTINE64     TranslateAddress);

using StackWalk_pfn = BOOL (IMAGEAPI *)(_In_     DWORD                          MachineType,
                                        _In_     HANDLE                         hProcess,
                                        _In_     HANDLE                         hThread,
                                        _Inout_  LPSTACKFRAME                   StackFrame,
                                        _Inout_  PVOID                          ContextRecord,
                                        _In_opt_ PREAD_PROCESS_MEMORY_ROUTINE   ReadMemoryRoutine,
                                        _In_opt_ PFUNCTION_TABLE_ACCESS_ROUTINE FunctionTableAccessRoutine,
                                        _In_opt_ PGET_MODULE_BASE_ROUTINE       GetModuleBaseRoutine,
                                        _In_opt_ PTRANSLATE_ADDRESS_ROUTINE     TranslateAddress);

using SymSetOptions_pfn        = DWORD   (IMAGEAPI *)(_In_ DWORD SymOptions);
using SymSetExtendedOption_pfn = BOOL    (IMAGEAPI *)(_In_ IMAGEHLP_EXTENDED_OPTIONS option, BOOL value);
using SymGetModuleBase64_pfn   = DWORD64 (IMAGEAPI *)(_In_ HANDLE  hProcess,
                                                      _In_ DWORD64 qwAddr);

using SymGetModuleBase_pfn     = DWORD (IMAGEAPI *)(_In_ HANDLE  hProcess,
                                                    _In_ DWORD   dwAddr);
using SymGetLineFromAddr64_pfn = BOOL (IMAGEAPI *)( _In_  HANDLE           hProcess,
                                                    _In_  DWORD64          qwAddr,
                                                    _Out_ PDWORD           pdwDisplacement,
                                                    _Out_ PIMAGEHLP_LINE64 Line64 );

using SymGetLineFromAddr_pfn = BOOL (IMAGEAPI *)( _In_  HANDLE         hProcess,
                                                  _In_  DWORD          dwAddr,
                                                  _Out_ PDWORD         pdwDisplacement,
                                                  _Out_ PIMAGEHLP_LINE Line );
using SymUnloadModule_pfn   = BOOL (IMAGEAPI *)( _In_ HANDLE hProcess,
                                                 _In_ DWORD  BaseOfDll );
using SymUnloadModule64_pfn = BOOL (IMAGEAPI *)( _In_ HANDLE  hProcess,
                                                 _In_ DWORD64 BaseOfDll );

using SymGetTypeInfo_pfn =
BOOL (IMAGEAPI *)( _In_  HANDLE                    hProcess,
                   _In_  DWORD64                   ModBase,
                   _In_  ULONG                     TypeId,
                   _In_  IMAGEHLP_SYMBOL_TYPE_INFO GetType,
                   _Out_ PVOID                     pInfo );


using SymFromAddr_pfn   = BOOL (IMAGEAPI *)( _In_      HANDLE       hProcess,
                                             _In_      DWORD64      Address,
                                             _Out_opt_ PDWORD64     Displacement,
                                             _Inout_   PSYMBOL_INFO Symbol );

using SymInitialize_pfn = BOOL (IMAGEAPI *)( _In_     HANDLE hProcess,
                                             _In_opt_ PCSTR  UserSearchPath,
                                             _In_     BOOL   fInvadeProcess );
using SymCleanup_pfn    = BOOL (IMAGEAPI *)( _In_ HANDLE hProcess );


using SymLoadModule_pfn   = DWORD (IMAGEAPI *)( _In_     HANDLE hProcess,
                                                _In_opt_ HANDLE hFile,
                                                _In_opt_ PCSTR  ImageName,
                                                _In_opt_ PCSTR  ModuleName,
                                                _In_     DWORD  BaseOfDll,
                                                _In_     DWORD  SizeOfDll );
using SymLoadModule64_pfn = DWORD64 (IMAGEAPI *)( _In_     HANDLE  hProcess,
                                                  _In_opt_ HANDLE  hFile,
                                                  _In_opt_ PCSTR   ImageName,
                                                  _In_opt_ PCSTR   ModuleName,
                                                  _In_     DWORD64 BaseOfDll,
                                                  _In_     DWORD   SizeOfDll );


const wchar_t* SK_Thread_GetName    (DWORD  dwTid);
const wchar_t* SK_Thread_GetName    (HANDLE hThread);
DWORD          SK_Thread_FindByName (std::wstring name);


#include <cassert>

#define SK_ASSERT_NOT_THREADSAFE() {                    \
  static DWORD dwLastThread = 0;                        \
                                                        \
  assert ( dwLastThread == 0 ||                         \
           dwLastThread == SK_Thread_GetCurrentId () ); \
                                                        \
  dwLastThread = SK_Thread_GetCurrentId ();             \
}

#define SK_ASSERT_NOT_DLLMAIN_THREAD() assert (! SK_TLS_Bottom ()->debug.in_DllMain);

extern bool SK_Debug_IsCrashing (void);


#include <concurrent_unordered_set.h>

extern concurrency::concurrent_unordered_set <HMODULE>&
SK_DbgHlp_Callers (void);

#define dbghelp_callers SK_DbgHlp_Callers ()


// https://gist.github.com/TheWisp/26097ee941ce099be33cfe3095df74a6
//
#include <functional>

template <class F>
struct DebuggableLambda : F
{
  template <class F2>

  DebuggableLambda ( F2&& func, const wchar_t* file, int line) :
    F (std::forward <F2> (func) ),
                    file (file)  ,
                    line (line)     {
  }

  using F::operator ();

  const wchar_t *file;
        int      line;
};

template <class F>
auto MakeDebuggableLambda (F&& func, const wchar_t* file, int line) ->
DebuggableLambda <typename std::remove_reference <F>::type>
{
  return { std::forward <F> (func), file, line };
}

#define  ENABLE_DEBUG_LAMBDA

#ifdef   ENABLE_DEBUG_LAMBDA
# define SK_DEBUG_LAMBDA(F) \
  MakeDebuggableLambda (F, __FILEW__, __LINE__)
# else
# define SK_DEBUGGABLE_LAMBDA(F) F
#endif



#ifndef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(P) (P)
#endif

std::wstring
SK_SEH_SummarizeException (_In_ struct _EXCEPTION_POINTERS* ExceptionInfo, bool crash_handled = false);


void
SK_SEH_LogException ( unsigned int        nExceptionCode,
                      EXCEPTION_POINTERS* pException,
                      LPVOID              lpRetAddr );

class SK_SEH_IgnoredException : public std::exception
{
public:
  SK_SEH_IgnoredException ( unsigned int        nExceptionCode,
                            EXCEPTION_POINTERS* pException,
                            LPVOID              lpRetAddr )
  {
    SK_SEH_LogException ( nExceptionCode, pException,
                                         lpRetAddr );
  }

  // Really, and truly ignored, since we know nothing about it
  SK_SEH_IgnoredException (void) noexcept
  {
  }
};

static constexpr int SK_EXCEPTION_CONTINUABLE = 0x0;

using RaiseException_pfn =
  void (WINAPI *)(       DWORD      dwExceptionCode,
                         DWORD      dwExceptionFlags,
                         DWORD      nNumberOfArguments,
                   const ULONG_PTR *lpArguments );

void
SK_RaiseException
(       DWORD      dwExceptionCode,
        DWORD      dwExceptionFlags,
        DWORD      nNumberOfArguments,
  const ULONG_PTR *lpArguments );


#define SK_BasicStructuredExceptionTranslator         \
  []( unsigned int        nExceptionCode,             \
      EXCEPTION_POINTERS* pException )->              \
  void {                                              \
    throw                                             \
      SK_SEH_IgnoredException (                       \
        nExceptionCode, pException,                   \
          _ReturnAddress ()   );                      \
  }

#define SK_FilteringStructuredExceptionTranslator(Filter) \
  []( unsigned int        nExceptionCode,                 \
      EXCEPTION_POINTERS* pException )->                  \
  void                                                    \
  {                                                       \
    if (nExceptionCode == (Filter))                       \
    {                                                     \
      throw                                               \
        SK_SEH_IgnoredException (                         \
          nExceptionCode, pException,                     \
            _ReturnAddress ()   );                        \
    }                                                     \
  }

// Various simpler macros were tried, all created internal compiler errors!!
#define SK_MultiFilteringStructuredExceptionTranslator(_Filters) \
  []( unsigned int        nExceptionCode,                        \
      EXCEPTION_POINTERS* pException )->                         \
  void                                                           \
  {                                                              \
    using     _List =     std::initializer_list <unsigned int>;  \
              _List              exception_list (                \
              _List {(_Filters)}                );               \
    if ( std::end  (             exception_list) !=              \
         std::find ( std::begin (exception_list),                \
                     std::end   (exception_list),                \
                                nExceptionCode )        )        \
    {                                                            \
      throw                                                      \
        SK_SEH_IgnoredException (                                \
          nExceptionCode, pException,                            \
            _ReturnAddress ()   );                               \
    }                                                            \
  }

enum SEH_LogLevel {
  Unchanged = 0,
  Silent    = 1,
  Verbose0  = 2
};

struct SK_SEH_PreState {
  _se_translator_function pfnTranslator;
  DWORD                   dwErrorCode;
  LPVOID                  lpCallSite;
  FILETIME                fOrigTime;
};

SK_SEH_PreState
SK_SEH_ApplyTranslator (_In_opt_ _se_translator_function _NewSETranslator);

_se_translator_function
SK_SEH_RemoveTranslator (SK_SEH_PreState pre_state);

_se_translator_function
SK_SEH_SetTranslator (
  _In_opt_ _se_translator_function _NewSETranslator,
                      SEH_LogLevel verbosity = Unchanged );


void WINAPI
SK_SetLastError (DWORD dwErrCode) noexcept;

LPVOID
SK_Debug_GetImageBaseAddr (void);

BOOL
WINAPI
SK_SetThreadPriorityBoost ( HANDLE hThread,
                            BOOL   bDisableBoost );

BOOL
WINAPI
SK_SetThreadPriority ( HANDLE hThread,
                       int    nPriority );


extern SK_LazyGlobal <
  concurrency::concurrent_unordered_map <DWORD, std::array <wchar_t, SK_MAX_THREAD_NAME_LEN+1>>
> _SK_ThreadNames;
extern SK_LazyGlobal <
  concurrency::concurrent_unordered_set <DWORD>
> _SK_SelfTitledThreads;

void SK_Process_Snapshot    (void);
bool SK_Process_IsSuspended (DWORD dwPid);
bool SK_Process_Suspend     (DWORD dwPid);
bool SK_Process_Resume      (DWORD dwPid);

void WINAPI SK_OutputDebugStringW (LPCWSTR lpOutputString);
void WINAPI SK_OutputDebugStringA (LPCSTR  lpOutputString);

using SetLastError_pfn   = void (WINAPI *)(_In_ DWORD dwErrCode);
using GetProcAddress_pfn = FARPROC (WINAPI *)(HMODULE,LPCSTR);
using CloseHandle_pfn    = BOOL (WINAPI *)(HANDLE);

using GetCommandLineW_pfn = LPWSTR (WINAPI *)(void);
using GetCommandLineA_pfn = LPSTR  (WINAPI *)(void);
using TerminateThread_pfn = BOOL (WINAPI *)( _In_ HANDLE hThread,
                                             _In_ DWORD  dwExitCode );
                                             using ExitThread_pfn = VOID (WINAPI *)(_In_ DWORD  dwExitCode);
using _endthreadex_pfn = void (__cdecl *)( _In_ unsigned _ReturnCode );
using NtTerminateProcess_pfn = NTSTATUS (*)(HANDLE, NTSTATUS);
using RtlExitUserThread_pfn = VOID (NTAPI *)(_In_ NTSTATUS 	Status);
using SHGetKnownFolderPath_pfn = HRESULT (WINAPI *)(REFKNOWNFOLDERID,DWORD,HANDLE,PWSTR*);

extern SHGetKnownFolderPath_pfn SHGetKnownFolderPath_Original;

extern GetCommandLineW_pfn      GetCommandLineW_Original;
extern GetCommandLineA_pfn      GetCommandLineA_Original;

extern NtTerminateProcess_pfn   NtTerminateProcess_Original;
extern RtlExitUserThread_pfn    RtlExitUserThread_Original;
extern ExitThread_pfn           ExitThread_Original;
extern _endthreadex_pfn         _endthreadex_Original;
extern TerminateThread_pfn      TerminateThread_Original;

extern RaiseException_pfn     RaiseException_Original;
extern SetLastError_pfn       SetLastError_Original;
extern GetProcAddress_pfn     GetProcAddress_Original;
extern TerminateProcess_pfn   TerminateProcess_Original;
extern ExitProcess_pfn        ExitProcess_Original;
extern OutputDebugStringA_pfn OutputDebugStringA_Original;
extern OutputDebugStringW_pfn OutputDebugStringW_Original;
extern CloseHandle_pfn        CloseHandle_Original;

#endif /* __SK__DEBUG_UTILS_H__ */
