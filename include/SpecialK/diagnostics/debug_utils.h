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
#endif

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

using LdrFindEntryForAddress_pfn    = NTSTATUS (NTAPI *)(HMODULE,PLDR_DATA_TABLE_ENTRY__SK*);
#pragma pack (pop)

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
bool SK_PE32_IsLargeAddressAware (void);

void WINAPI SK_SymRefreshModuleList (HANDLE hProc = GetCurrentProcess ());
BOOL WINAPI SK_IsDebuggerPresent    (void);

BOOL __stdcall SK_TerminateProcess (UINT uExitCode);

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

using SymSetOptions_pfn      = DWORD (IMAGEAPI *)  (_In_ DWORD SymOptions);
using SymGetModuleBase64_pfn = DWORD64 (IMAGEAPI *)(_In_ HANDLE  hProcess,
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


std::wstring& SK_Thread_GetName (DWORD  dwTid);
std::wstring& SK_Thread_GetName (HANDLE hThread);


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

extern "C" RaiseException_pfn
           RaiseException_Original;

static
__forceinline
void
SK_RaiseException
(       DWORD      dwExceptionCode,
        DWORD      dwExceptionFlags,
        DWORD      nNumberOfArguments,
  const ULONG_PTR *lpArguments )
{
  if (RaiseException_Original != nullptr)
  {
    RaiseException_Original ( dwExceptionCode,    dwExceptionFlags,
                              nNumberOfArguments, lpArguments );
  }

  else
  {
    RaiseException ( dwExceptionCode,    dwExceptionFlags,
                     nNumberOfArguments, lpArguments );
  }
}


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
SK_SetLastError (DWORD dwErrCode);

LPVOID
SK_Debug_GetImageBaseAddr (void);


extern SK_LazyGlobal <
  concurrency::concurrent_unordered_map <DWORD, std::wstring>
> _SK_ThreadNames;
extern SK_LazyGlobal <
  concurrency::concurrent_unordered_set <DWORD>
> _SK_SelfTitledThreads;

#endif /* __SK__DEBUG_UTILS_H__ */
