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

#define __SK_SUBSYSTEM__ L"DebugUtils"

#include <SpecialK/diagnostics/debug_utils.h>

#include <SpecialK/config.h>
#include <SpecialK/core.h>
#include <SpecialK/hooks.h>
#include <SpecialK/log.h>
#include <SpecialK/resource.h>
#include <SpecialK/utility.h>
#include <SpecialK/thread.h>

#include <Windows.h>
#include <Winternl.h>
#include <strsafe.h>
#include <unordered_set>

#include <codecvt>

// Fix warnings in dbghelp.h
#pragma warning (disable : 4091)

#define _IMAGEHLP_SOURCE_
//#pragma comment (lib, "dbghelp.lib")
#include <dbghelp.h>

#include <SpecialK/diagnostics/file.h>
#include <SpecialK/diagnostics/memory.h>
#include <SpecialK/diagnostics/network.h>
#include <SpecialK/diagnostics/modules.h>
#include <SpecialK/diagnostics/load_library.h>

iSK_Logger game_debug;

const wchar_t*
SK_SEH_CompatibleCallerName (LPCVOID lpAddr);

extern SK_Thread_HybridSpinlock* cs_dbghelp;

typedef LPWSTR (WINAPI *GetCommandLineW_pfn)(void);
GetCommandLineW_pfn     GetCommandLineW_Original   = nullptr;

typedef LPSTR (WINAPI *GetCommandLineA_pfn)(void);
GetCommandLineA_pfn    GetCommandLineA_Original   = nullptr;

TerminateProcess_pfn
                       TerminateProcess_Original   = nullptr;
ExitProcess_pfn        ExitProcess_Original        = nullptr;
ExitProcess_pfn        ExitProcess_Hook            = nullptr;
OutputDebugStringA_pfn OutputDebugStringA_Original = nullptr;
OutputDebugStringW_pfn OutputDebugStringW_Original = nullptr;

void
SK_SymSetOpts (void);

typedef void (WINAPI *SetLastError_pfn)(_In_ DWORD dwErrCode);
                      SetLastError_pfn
                      SetLastError_Original = nullptr;

typedef FARPROC (WINAPI *GetProcAddress_pfn)(HMODULE,LPCSTR);
                         GetProcAddress_pfn
                         GetProcAddress_Original = nullptr;

#define STATUS_SUCCESS     0


#if 0
typedef struct _LDR_DATA_TABLE_ENTRY
{
  LIST_ENTRY     InLoadOrderLinks;
  LIST_ENTRY     InMemoryOrderLinks;
  LIST_ENTRY     InInitializationOrderLinks;
  PVOID          DllBase;
  PVOID          EntryPoint;
  ULONG          SizeOfImage;
  UNICODE_STRING FullDllName;
  UNICODE_STRING BaseDllName;
  ULONG          Flags;
  WORD           LoadCount;
  WORD           TlsIndex;

  union
  {
    LIST_ENTRY   HashLinks;
    struct
    {
      PVOID      SectionPointer;
      ULONG      CheckSum;
    };
  };

  union
  {
    ULONG        TimeDateStamp;
    PVOID        LoadedImports;
  };

  _ACTIVATION_CONTEXT
                *EntryPointActivationContext;
  PVOID          PatchInformation;
  LIST_ENTRY     ForwarderLinks;
  LIST_ENTRY     ServiceTagLinks;
  LIST_ENTRY     StaticLinks;
} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;
#endif

typedef NTSTATUS (NTAPI *LdrFindEntryForAddress_pfn)(
  HMODULE                 hMod,
  LDR_DATA_TABLE_ENTRY **ppLdrData
);

BOOL
WINAPI
SK_Module_IsProcAddrLocal ( HMODULE  hModExpected,
                             LPCSTR  lpProcName,
                            FARPROC  lpProcAddr,
              PLDR_DATA_TABLE_ENTRY *ppldrEntry = nullptr )
{
  if (! GetProcAddress_Original) return TRUE;

  static LdrFindEntryForAddress_pfn LdrFindEntryForAddress =
        (LdrFindEntryForAddress_pfn)
      GetProcAddress_Original ( GetModuleHandleW (L"NtDll.dll"),
                                                   "LdrFindEntryForAddress" );

  // Indeterminate, so ... I guess no?
  if (! LdrFindEntryForAddress)
    return FALSE;

  PLDR_DATA_TABLE_ENTRY pLdrEntry;

  if (NT_SUCCESS (LdrFindEntryForAddress ((HMODULE)lpProcAddr, &pLdrEntry)))
  {
    if (ppldrEntry != nullptr) *ppldrEntry = pLdrEntry;

    if ( StrStrIW ( SK_GetModuleName (hModExpected).c_str (),
                (wchar_t *)((UNICODE_STRING *)&(pLdrEntry->Reserved4))->Buffer ) )
    {
      return TRUE;
    }

    else
    {
      SK_LOG0 ( ( LR"(Procedure: '%hs' located by NtLdr in '%ws')",
                      lpProcName, (wchar_t *)pLdrEntry->FullDllName.Buffer ),
                  L"DebugUtils" );
      SK_LOG0 ( ( L"  >>  Expected Location:  '%ws'!",
                      SK_GetModuleFullName (hModExpected).c_str () ),
                  L"DebugUtils" );

      return FALSE;
    }
  }

  if (ppldrEntry != nullptr) *ppldrEntry = nullptr;

  return FALSE;
}


#include <SpecialK/ansel.h>
#include <concurrent_unordered_map.h>

void
WINAPI
SetLastError_Detour (
  _In_ DWORD dwErrCode
)
{
  if (dwErrCode != NO_ERROR)
  {
    if (_ReturnAddress () != SetLastError_Detour)
    {
      SK_TLS* pTLS = SK_TLS_Bottom ();

      if (pTLS != nullptr)
      {
        pTLS->win32.error_state.call_site = _ReturnAddress ();
        pTLS->win32.error_state.code      = dwErrCode;
        GetSystemTimeAsFileTime (&pTLS->win32.error_state.last_time);
      }
    }
  }

  SetLastError_Original (dwErrCode);
}

FARPROC
WINAPI
GetProcAddress_Detour     (
  _In_ HMODULE hModule,
  _In_ LPCSTR  lpProcName )
{
  if (    ReadAcquire (&__SK_DLL_Ending  ) ||
       (! ReadAcquire (&__SK_DLL_Attached)  )
     )
  {
    // How did we manage to invoke the hook if the
    //   trampoline to call the original code is borked?!
    if (GetProcAddress_Original == nullptr)
      return nullptr;
    else
      return GetProcAddress_Original ( hModule, lpProcName );
  }

  //static Concurrency::concurrent_unordered_map <HMODULE, std::unordered_map <std::string, FARPROC>> procs    (4);
  //static Concurrency::concurrent_unordered_map <HMODULE, std::unordered_map <uint16_t,    FARPROC>> ordinals (4);
  //
  //if (((uint16_t)(uintptr_t)lpProcName & 0xFFFF) > 65536)
  //{
  //  if (procs.count (hModule))
  //  {
  //    if (     procs [hModule].count (lpProcName))
  //      return procs [hModule]       [lpProcName];
  //  }
  //
  //  FARPROC proc =
  //    GetProcAddress_Original (hModule, lpProcName);
  //
  //  if (proc != nullptr)
  //  {
  //    procs [hModule][lpProcName] = proc;
  //  }
  //
  //  return proc;
  //}
  //
  //
  //else if ((uint16_t)((uintptr_t)lpProcName & 0xFFFF) <= 65535)
  //{
  //  if (ordinals.count (hModule))
  //  {
  //    if (     ordinals [hModule].count ((uint16_t)((uintptr_t)lpProcName & 0xFFFF)))
  //      return ordinals [hModule]       [(uint16_t)((uintptr_t)lpProcName & 0xFFFF)];
  //  }
  //
  //  FARPROC proc =
  //    GetProcAddress_Original (hModule, lpProcName);
  //
  //  if (proc != nullptr)
  //  {
  //    ordinals [hModule][(uint16_t)((uintptr_t)lpProcName & 0xFFFF)] = proc;
  //  }
  //
  //  return proc;
  //}


  // Ignore ordinals for this (Disable Nahimic) test
  if ((uintptr_t)lpProcName > 65535UL)
  {
    if (! lstrcmpiA (lpProcName, "NoHotPatch"))
    {
      static     DWORD NoHotPatch = 0x1;
      return (FARPROC)&NoHotPatch;
    }


    //if (! lstrcmpA (lpProcName, "AnselEnableCheck"))
    //{
    //  static FARPROC pLast = nullptr;
    //
    //  if (pLast) return pLast;
    //
    //  FARPROC pProc =
    //    GetProcAddress_Original (hModule, lpProcName);
    //
    //  if (pProc != nullptr)
    //  {
    //    pLast = pProc;
    //    SK_NvCamera_ApplyHook__AnselEnableCheck (hModule, lpProcName);
    //    return pLast;
    //  }
    //
    //  return pProc;
    //}

    //if (! lstrcmpA (lpProcName, "isAnselAvailable"))
    //{
    //  static FARPROC pLast = nullptr;
    //
    //  if (pLast) return pLast;
    //
    //  FARPROC pProc =
    //    GetProcAddress_Original (hModule, lpProcName);
    //
    //  if (pProc != nullptr)
    //  {
    //    pLast = pProc;
    //    SK_AnselSDK_ApplyHook__isAnselAvailable (hModule, lpProcName);
    //    return pLast;
    //  }
    //
    //  return pProc;
    //}


    if (config.system.log_level > 1)
    {
      SK_Module_IsProcAddrLocal ( hModule, lpProcName,
         GetProcAddress_Original (hModule, lpProcName) );
    }
  }

  if (config.system.log_level == 0)
    return GetProcAddress_Original (hModule, lpProcName);

  HMODULE hModCaller =
    SK_GetCallingDLL ();
  
  if (hModCaller == SK_GetDLL ())
    return GetProcAddress_Original (hModule, lpProcName);


  static DWORD dwOptimus        = 0x1;
  static DWORD dwAMDPowerXPress = 0x1;

  // We have to handle ordinals as well, those would generally crash anything
  //   that treats them as a nul-terminated string.
  if ((uintptr_t)lpProcName < 65536)
  {
    ///if ((uintptr_t)lpProcName == 100 && SK_GetCallerName ().find (L"gameoverlayrenderer") != std::wstring::npos)
    ///{
    ///  static FARPROC original_addr =
    ///    GetProcAddress_Original (hModule, lpProcName);
    ///
    ///  return original_addr;
    ///}

    // We need to filter this event because the Steam overlay goes berserk at startup
    //   trying to get a different proc. addr for Ordinal 100 (XInputGetStateEx).
    //
    if (config.system.log_level > 0 && dll_log.lines > 15)
    {
      if (hModCaller != SK_GetDLL ())
      {
        SK_LOG3 ( ( LR"(GetProcAddress ([%ws], {Ordinal: %lu})  -  %ws)", 
                           SK_GetModuleFullName (hModule).c_str (),
                        ((uintptr_t)lpProcName & 0xFFFFU), SK_SummarizeCaller ().c_str () ),
                    L"DLL_Loader" );
      }
    }

    return GetProcAddress_Original (hModule, lpProcName);
  }


  //
  // With how frequently this function is called, we need to be smarter about
  //   string handling -- thus a hash set.
  //
  static const std::unordered_set <std::string> handled_strings = {
    "NvOptimusEnablement", "AmdPowerXpressRequestHighPerformance"
  };

  if (handled_strings.count (lpProcName))
  {
    if (! strcmp (lpProcName, "NvOptimusEnablement"))
    {
      dll_log.Log (L"Optimus Enablement");
      return (FARPROC)&dwOptimus;
    }

    if (! strcmp (lpProcName, "AmdPowerXpressRequestHighPerformance"))
      return (FARPROC)&dwAMDPowerXPress;
  }


  ///if (config.system.trace_load_library && StrStrA (lpProcName, "LoadLibrary"))
  ///{
  ///  // Make other DLLs install their hooks _after_ SK.
  ///  if (! strcmp (lpProcName, "LoadLibraryExW"))
  ///  {
  ///    extern HMODULE
  ///    WINAPI
  ///    LoadLibraryExW_Detour (
  ///      _In_       LPCWSTR lpFileName,
  ///      _Reserved_ HANDLE  hFile,
  ///      _In_       DWORD   dwFlags );
  ///    return (FARPROC)LoadLibraryExW_Detour;
  ///  }
  ///
  ///  if (! strcmp (lpProcName, "LoadLibraryW"))
  ///    return (FARPROC)*LoadLibraryW_Original;
  ///
  ///  if (! strcmp (lpProcName, "LoadLibraryExA"))
  ///    return (FARPROC)*LoadLibraryExA_Original;
  ///
  ///  if (! strcmp (lpProcName, "LoadLibraryA"))
  ///    return (FARPROC)*LoadLibraryA_Original;
  ///}


  if (config.system.log_level > 0 && dll_log.lines > 15)
  {  
    if (hModCaller != SK_GetDLL ())
    {
      SK_LOG3 ( ( LR"(GetProcAddress ([%ws], "%hs")  -  %ws)", 
                         SK_GetModuleFullName (hModule).c_str (),
                      lpProcName, SK_SummarizeCaller ().c_str () ),
                  L"DLL_Loader" );
    }
  }


  return
    GetProcAddress_Original (hModule, lpProcName);
}


extern
DWORD_PTR
WINAPI
SetThreadAffinityMask_Detour (
  _In_ HANDLE    hThread,
  _In_ DWORD_PTR dwThreadAffinityMask);

BOOL
__stdcall
SK_TerminateParentProcess (UINT uExitCode)
{
  if (TerminateProcess_Original != nullptr)
  {
    return
      TerminateProcess_Original ( GetCurrentProcess (),
                                    uExitCode );
  }

  else
  {
    return
      TerminateProcess ( GetCurrentProcess (),
                           uExitCode );
  }
}

using ResetEvent_pfn = BOOL (WINAPI *)(
  _In_ HANDLE hEvent
);
ResetEvent_pfn ResetEvent_Original = nullptr;

BOOL
WINAPI
ResetEvent_Detour (
  _In_ HANDLE hEvent
)
{
  // Compliance failure in some games makes application verifier useless
  if (hEvent == nullptr)
  {
    SK_RunOnce (dll_log.Log ( L"[DebugUtils] Invalid handle passed to ResetEvent (...) - %s",
                               SK_SummarizeCaller ().c_str () ));
    return FALSE;
  }

  return ResetEvent_Original (hEvent);
}


BOOL
WINAPI
TerminateProcess_Detour (HANDLE hProcess, UINT uExitCode)
{
  UNREFERENCED_PARAMETER (uExitCode);

  if (hProcess == GetCurrentProcess ())
  {
    OutputDebugString ( L" *** BLOCKED TerminateProcess (...) ***\n\t" );
    OutputDebugString ( SK_GetCallerName ().c_str () );

    return FALSE;
  }

  return TerminateProcess_Original (hProcess, uExitCode);
}

void
WINAPI
ExitProcess_Detour (UINT uExitCode)
{
  // Since many, many games don't shutdown cleanly, let's unload ourself.
  SK_SelfDestruct      ();

  if (ExitProcess_Original != nullptr)
  {
    const ExitProcess_pfn callthrough = ExitProcess_Original;
          ExitProcess_Original        = nullptr;
          callthrough (uExitCode);
  }

  else
    ExitProcess (uExitCode);
}




LPWSTR
WINAPI
GetCommandLineW_Detour (void)
{
  SK_LOG_FIRST_CALL

  static
  wchar_t wszFakeOut [MAX_PATH * 4] = { };

  if (*wszFakeOut != L'\0')
    return wszFakeOut;

  lstrcpyW (wszFakeOut, L"\"");
  lstrcatW (wszFakeOut, SK_GetFullyQualifiedApp ());
  lstrcatW (wszFakeOut, L"\"");

  if (! lstrcmpiW ( SK_GetHostApp (), L"RED-Win64-Shipping.exe" ))
  {
    lstrcatW (wszFakeOut, L" -eac-nop-loaded");
    return    wszFakeOut;
  }

  if (! lstrcmpiW ( SK_GetHostApp (), L"DBFighterZ.exe" ))
  {
    lstrcatW (wszFakeOut, L" -eac-nop-loaded");
    return    wszFakeOut;
  }

#ifdef _DEBUG
  if (_wcsicmp (wszFakeOut, GetCommandLineW_Original ()))
    dll_log.Log (L"GetCommandLineW () ==> %ws", GetCommandLineW_Original ());
#endif

  return GetCommandLineW_Original ();
}



LPSTR
WINAPI
GetCommandLineA_Detour (void)
{
  SK_LOG_FIRST_CALL

  //static
  //char szFakeOut [MAX_PATH * 4] = { };
  //
  //if (*szFakeOut != '\0')
  //  return szFakeOut;
  //
  //lstrcpyA (szFakeOut, "\"");
  //lstrcatA (szFakeOut, SK_WideCharToUTF8 (SK_GetFullyQualifiedApp ()).c_str ());
  //lstrcatA (szFakeOut, "\"");
  //
  //if (_stricmp (szFakeOut, GetCommandLineA_Original ()))
#ifdef _DEBUG
  SK_RunOnce (
    dll_log.Log (L"GetCommandLineA () ==> %hs", GetCommandLineA_Original ())
  );
#endif

  return GetCommandLineA_Original ();
}



void
WINAPI
OutputDebugStringA_Detour (LPCSTR lpOutputString)
{
  // fprintf is stupid, but lpOutputString already contains a newline and
  //   fputs would just add another one...

  wchar_t  wszModule [MAX_PATH] = { };
  wcsncpy (wszModule, SK_GetModuleFullNameFromAddr (_ReturnAddress ()).c_str (),
                      MAX_PATH - 1);

  game_debug.LogEx (true,   L"%-72ws:  %hs", wszModule, lpOutputString);
//fwprintf         (stdout, L"%hs",          lpOutputString);
  
  if (! strstr (lpOutputString, "\n"))
    game_debug.LogEx (false, L"\n");
  
  OutputDebugStringA_Original (lpOutputString);
}

void
WINAPI
OutputDebugStringW_Detour (LPCWSTR lpOutputString)
{
  wchar_t  wszModule [MAX_PATH] = { };
  wcsncpy (wszModule, SK_GetModuleFullNameFromAddr (_ReturnAddress ()).c_str (),
                      MAX_PATH - 1);

  game_debug.LogEx (true,   L"%-72ws:  %ws", wszModule, lpOutputString);
//fwprintf         (stdout, L"%ws",                     lpOutputString);

  if (! wcsstr (lpOutputString, L"\n"))
    game_debug.LogEx (false, L"\n");

  OutputDebugStringW_Original (lpOutputString);
}



using GetThreadContext_pfn = BOOL (WINAPI *)(HANDLE,LPCONTEXT);
using SetThreadContext_pfn = BOOL (WINAPI *)(HANDLE,const CONTEXT *);

GetThreadContext_pfn GetThreadContext_Original = nullptr;
SetThreadContext_pfn SetThreadContext_Original = nullptr;

#define STATUS_SUCCESS                          0

const ULONG ThreadHideFromDebugger = 0x11;

#define THREAD_CREATE_FLAGS_CREATE_SUSPENDED        0x00000001
#define THREAD_CREATE_FLAGS_SKIP_THREAD_ATTACH      0x00000002
#define THREAD_CREATE_FLAGS_HIDE_FROM_DEBUGGER      0x00000004
#define THREAD_CREATE_FLAGS_HAS_SECURITY_DESCRIPTOR 0x00000010
#define THREAD_CREATE_FLAGS_ACCESS_CHECK_IN_TARGET  0x00000020
#define THREAD_CREATE_FLAGS_INITIAL_THREAD          0x00000080

typedef NTSTATUS (NTAPI *NtSetInformationThread_pfn)(
  _In_ HANDLE ThreadHandle,
  _In_ ULONG  ThreadInformationClass,
  _In_ PVOID  ThreadInformation,
  _In_ ULONG  ThreadInformationLength
);

typedef NTSTATUS (NTAPI *NtCreateThreadEx_pfn)(
    _Out_    PHANDLE              ThreadHandle,
    _In_     ACCESS_MASK          DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES   ObjectAttributes,
    _In_     HANDLE               ProcessHandle,
    _In_     PVOID                StartRoutine,
    _In_opt_ PVOID                Argument,
    _In_     ULONG                CreateFlags,
    _In_opt_ ULONG_PTR            ZeroBits,
    _In_opt_ SIZE_T               StackSize,
    _In_opt_ SIZE_T               MaximumStackSize,
    _In_opt_ PVOID                AttributeList
);

NtCreateThreadEx_pfn       NtCreateThreadEx_Original       = nullptr;
NtSetInformationThread_pfn NtSetInformationThread_Original = nullptr;

NTSTATUS
NTAPI
NtSetInformationThread_Detour (
  _In_ HANDLE ThreadHandle,
  _In_ ULONG  ThreadInformationClass,
  _In_ PVOID  ThreadInformation,
  _In_ ULONG  ThreadInformationLength )
{
  SK_LOG_FIRST_CALL

  if ( ThreadInformationClass  == ThreadHideFromDebugger && 
       ThreadInformation       == 0                      &&
       ThreadInformationLength == 0 )
  {
    SK_LOG0 ( ( L"tid=%x tried to hide itself from debuggers; please attach one and investigate!",
                  GetThreadId (ThreadHandle) ),
                L"DieAntiDbg" );

    //if (config.system.log_level > 1)
    //{
    //  //SuspendThread (ThreadHandle);
    //  return STATUS_SUCCESS;
    //}
  }

  return
    NtSetInformationThread_Original ( ThreadHandle, 
                                        ThreadInformationClass,
                                        ThreadInformation,
                                          ThreadInformationLength );
}


volatile LONG lLastThreadCreate = 0;

#include <concurrent_unordered_map.h>
#include <concurrent_unordered_set.h>
#include <SpecialK/diagnostics/crash_handler.h>

extern concurrency::concurrent_unordered_map <DWORD, std::wstring>&
__SK_GetThreadNames (void);
extern concurrency::concurrent_unordered_set <DWORD>&
__SK_GetSelfTitledThreads (void);

#define _SK_SelfTitledThreads __SK_GetSelfTitledThreads ()
#define _SK_ThreadNames       __SK_GetThreadNames       ()

NTSTATUS
NTAPI
NtCreateThreadEx_Detour (
  _Out_    PHANDLE              ThreadHandle,
  _In_     ACCESS_MASK          DesiredAccess,
  _In_opt_ POBJECT_ATTRIBUTES   ObjectAttributes,
  _In_     HANDLE               ProcessHandle,
  _In_     PVOID                StartRoutine,
  _In_opt_ PVOID                Argument,
  _In_     ULONG                CreateFlags,
  _In_opt_ ULONG_PTR            ZeroBits,
  _In_opt_ SIZE_T               StackSize,
  _In_opt_ SIZE_T               MaximumStackSize,
  _In_opt_ PVOID                AttributeList )
{
  SK_LOG_FIRST_CALL

  extern bool
  SK_Thread_InitDebugExtras (void);

  SK_RunOnce (SK::Diagnostics::CrashHandler::InitSyms ());
  SK_RunOnce (SK_Thread_InitDebugExtras ());

  HMODULE hModStart = SK_GetModuleFromAddr (StartRoutine);

  if (! dbghelp_callers.count (hModStart))
  {
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

#ifdef _WIN64
    DWORD64 BaseAddr;
#else
    DWORD   BaseAddr;
#endif

    CHeapPtr <char> szDupName (_strdup (SK_WideCharToUTF8 (SK_GetModuleFullNameFromAddr (StartRoutine)).c_str ()));

    MODULEINFO mod_info = { };

    GetModuleInformation (
      GetCurrentProcess (), hModStart, &mod_info, sizeof (mod_info)
    );

#ifdef _WIN64
    BaseAddr = (DWORD64)mod_info.lpBaseOfDll;
#else
    BaseAddr =   (DWORD)mod_info.lpBaseOfDll;
#endif

    char* pszShortName = szDupName.m_pData;

    PathStripPathA (pszShortName);


    SK_SymLoadModule ( GetCurrentProcess (),
                         nullptr,
                          pszShortName,
                            nullptr,
                              BaseAddr,
                                mod_info.SizeOfImage );


    dbghelp_callers.insert (hModStart);
  }


  char    thread_name [512] = { };
  char    szSymbol    [256] = { };
  ULONG   ulLen             = 191;

  ulLen = SK_GetSymbolNameFromModuleAddr (
            hModStart,
   reinterpret_cast <uintptr_t> ((LPVOID)StartRoutine),
                szSymbol,
                  ulLen );

  if (ulLen > 0)
  {
    sprintf ( thread_name, "%s+%s",
             SK_WideCharToUTF8 (SK_GetCallerName (StartRoutine)).c_str ( ),
                                                     szSymbol );
  }

  else {
    sprintf ( thread_name, "%s",
                SK_WideCharToUTF8 (SK_GetCallerName (StartRoutine)).c_str () );
  }





  BOOL Suspicious = FALSE;

  if ( CreateFlags & THREAD_CREATE_FLAGS_HIDE_FROM_DEBUGGER )
  {
    CreateFlags &= ~THREAD_CREATE_FLAGS_HIDE_FROM_DEBUGGER;

    if (config.system.log_level > 1)
      CreateFlags |= THREAD_CREATE_FLAGS_CREATE_SUSPENDED;

    //SK_LOG0 ( ( L"Tried to begin a debugger-hidden thread; punish it by starting visible and suspended!",
    //              GetThreadId (*ThreadHandle) ),
    //            L"DieAntiDbg" );

    Suspicious = TRUE;
  }

  ////CreateFlags &= ~THREAD_CREATE_FLAGS_SKIP_THREAD_ATTACH;

  //BOOL suspended = CreateFlags  & THREAD_CREATE_FLAGS_CREATE_SUSPENDED;
                 //CreateFlags |= THREAD_CREATE_FLAGS_CREATE_SUSPENDED;

  NTSTATUS ret =
    NtCreateThreadEx_Original ( ThreadHandle, DesiredAccess, ObjectAttributes,
                                ProcessHandle, StartRoutine, Argument, CreateFlags,
                                ZeroBits, StackSize, MaximumStackSize, AttributeList );

  if (NT_SUCCESS (ret))
  {
    InterlockedIncrement (&lLastThreadCreate);

    const DWORD tid =
      GetThreadId (*ThreadHandle);

    if (! _SK_ThreadNames.count (tid))
    {
      _SK_ThreadNames [tid] =
        SK_UTF8ToWideChar (thread_name);
    
      SK_TLS* pTLS =
        SK_TLS_BottomEx (tid);

      if (pTLS != nullptr)
      {
        wcsncpy_s (
          SK_TLS_BottomEx (tid)->debug.name,
            256,
              SK_UTF8ToWideChar (thread_name).c_str (), _TRUNCATE
        );
      }
    }

    if (Suspicious)
    {
      SK_LOG0 ( ( L">>tid=%x", GetThreadId (*ThreadHandle) ),
                  L"DieAntiDbg" );
    }

    //if (! suspended)
    //  ResumeThread (*ThreadHandle);
  }

  return ret;
}


bool spoof_debugger = true;

using IsDebuggerPresent_pfn = BOOL (WINAPI *)(void);
IsDebuggerPresent_pfn IsDebuggerPresent_Original = nullptr;

BOOL
WINAPI
IsDebuggerPresent_Detour (void)
{
  // Community Service Time
  if (SK_GetCurrentGameID () == SK_GAME_ID::FinalFantasyXV)
  {
    static bool killed_ffxv = false;
  
    if ((! killed_ffxv) && GetThreadPriority (SK_GetCurrentThread ()) == THREAD_PRIORITY_LOWEST)
    {
      SK_LOG0 ( ( L"Anti-Debug Detected (tid=%x)",
                    GetCurrentThreadId () ),
                  L"AntiAntiDbg");

      _endthreadex ( 0x0 );
      killed_ffxv = true;
    }
  }

  // Steam DRM work arounds
  std::wstring caller_name =
    SK_GetModuleFullName (SK_GetCallingDLL ());

  if ( StrStrIW (caller_name.c_str (), L"appticket") ||
       StrStrIW (caller_name.c_str (), L"steam")     ||
       StrStrIW (caller_name.c_str (), L"_s") )
  {
    return FALSE;
  }

#ifdef _DEBUG
  return TRUE;
#endif

  if (spoof_debugger)
    return FALSE;

  //return TRUE;

  return IsDebuggerPresent_Original ();
}

using DbgBreakPoint_pfn = void (WINAPI *)(void);
DbgBreakPoint_pfn DbgBreakPoint_Original = nullptr;

using DebugBreak_pfn = void (WINAPI *)(void);
DebugBreak_pfn DebugBreak_Original = nullptr;

__declspec (noinline)
void
WINAPI
DebugBreak_Detour (void)
{
  if (IsDebuggerPresent_Original ())
             DebugBreak_Original ();

  return;
}

__declspec (noinline)
void
WINAPI
DbgBreakPoint_Detour (void)
{
  if (IsDebuggerPresent_Original ())
    DbgBreakPoint_Original ();

  return;
}


#include <SpecialK/parameter.h>

using RaiseException_pfn = void (WINAPI *)(      DWORD      dwExceptionCode,
      DWORD      dwExceptionFlags,
      DWORD      nNumberOfArguments,
const ULONG_PTR *lpArguments);
extern "C" RaiseException_pfn RaiseException_Original = nullptr;

#include <unordered_set>

struct SK_FFXV_Thread
{
  ~SK_FFXV_Thread (void) { if (hThread) CloseHandle (hThread); }

  HANDLE               hThread = 0;
  volatile LONG        dwPrio  = THREAD_PRIORITY_NORMAL;

  sk::ParameterInt* prio_cfg;

  void setup (HANDLE hThread);
} extern sk_ffxv_swapchain,
         sk_ffxv_vsync,
         sk_ffxv_async_run;


constexpr static DWORD MAGIC_THREAD_EXCEPTION = 0x406D1388;


bool
WINAPI
SK_Exception_HandleCxx (
      DWORD      dwExceptionCode,
      DWORD      dwExceptionFlags,
      DWORD      nNumberOfArguments,
const ULONG_PTR *lpArguments,
      bool       pointOfOriginWas_CxxThrowException = false )
{
  if ( SK_IsDebuggerPresent () || pointOfOriginWas_CxxThrowException )
  {
    try {
      try {
        RaiseException_Original (
          dwExceptionCode,
          dwExceptionFlags,
          nNumberOfArguments,
          lpArguments         );
      }

      catch (_com_error& com_err)
      {
         _bstr_t bstrSource      (com_err.Source      ());
         _bstr_t bstrDescription (com_err.Description ());

        SK_LOG0 ( ( L" >> Code: %08lx  <%s>  -  [Source: %s,  Desc: \"%s\"]",
                      com_err.Error        (),
                      com_err.ErrorMessage (),
                      (LPCWSTR)bstrSource, (LPCWSTR)bstrDescription ),
                    L"  COMErr  " );

        throw;
      }
    }

    catch (...)
    {
      if  (! pointOfOriginWas_CxxThrowException)
        throw;
    }

    return true;
  }

  return false;
}


extern "C"
void
WINAPI
SK_SEHCompatibleRaiseException (
      DWORD      dwExceptionCode,
      DWORD      dwExceptionFlags,
      DWORD      nNumberOfArguments,
const ULONG_PTR *lpArguments         )
{
  //if (dwExceptionCode == 0xe06d7363)
  //{
  //  if (! SK_Exception_HandleCxx ( dwExceptionCode,    dwExceptionFlags,
  //                                 nNumberOfArguments, lpArguments       ) )
  //  {
  //    /// ...
  //
  //  }
  //}
  
  RaiseException_Original ( dwExceptionCode,    dwExceptionFlags,
                            nNumberOfArguments, lpArguments       );
}

bool
SK_Exception_HandleThreadName (
      DWORD      dwExceptionCode,
      DWORD      /*dwExceptionFlags*/,
      DWORD      /*nNumberOfArguments*/,
const ULONG_PTR *lpArguments         )
{
  if (dwExceptionCode == MAGIC_THREAD_EXCEPTION)
  {
    #pragma pack(push,8)
    typedef struct tagTHREADNAME_INFO
    {
      DWORD  dwType;     // Always 4096
      LPCSTR szName;     // Pointer to name (in user addr space).
      DWORD  dwThreadID; // Thread ID (-1=caller thread).
      DWORD  dwFlags;    // Reserved for future use, must be zero.
    } THREADNAME_INFO;
    #pragma pack(pop)

    THREADNAME_INFO* info = 
      (THREADNAME_INFO *)lpArguments;

    size_t len = 0;

    bool non_empty =
                info->szName  != nullptr &&
      SUCCEEDED (
        StringCbLengthA (
                info->szName,
                   255, &len)
                )                        &&
                          len  > 0       &&
                info->dwFlags == 0       &&
                info->dwType  == 4096;

    if (non_empty)
    {
      DWORD dwTid  =  ( info->dwThreadID != -1 ?
                        info->dwThreadID :
                        GetCurrentThreadId () );

      _SK_SelfTitledThreads.insert (dwTid);

      // Push this to the TLS datastore so we can get thread names even
      //   when no debugger is attached.

      SK_TLS* pTLS =
        SK_TLS_BottomEx (dwTid);

      std::wstring wide_name (
        SK_UTF8ToWideChar (info->szName)
      );

      if (pTLS != nullptr)
      {
        wcsncpy_s (
          pTLS->debug.name,
            std::min (len+1, (size_t)256),
              wide_name.c_str (),
                _TRUNCATE );
      }

      _SK_ThreadNames [dwTid] =
        wide_name;

      if (SK_GetCurrentGameID () == SK_GAME_ID::FinalFantasyXV)
      {
        if (info->szName != 0)
        {
          CHandle hThread ( OpenThread ( THREAD_ALL_ACCESS, FALSE, dwTid ) );

          if ((! sk_ffxv_vsync.hThread) && StrStrIA (info->szName, "VSync"))
          {
            sk_ffxv_vsync.setup (hThread);
          }

          else if ((! sk_ffxv_async_run.hThread) && StrStrIA (info->szName, "AsyncFile.Run"))
          {
            sk_ffxv_async_run.setup (hThread);
          }
        }
      }
    }

    return true;
  }

  return false;
}



//;WINMM.dll;AVRT.dll;secur32.dll;USERENV.dll

// SEH compatible, but not 100% thread-safe (uses Fiber-Local Storage)
bool
SK_SEH_CompatibleCallerName (LPCVOID lpAddr, wchar_t *wszDllFullName)
{
  HMODULE hModOut = nullptr;

  if ( GetModuleHandleEx (
         GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT |
         GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
           static_cast <LPCWSTR> (lpAddr),
             &hModOut
       )
     )
  {
    if ( 0 != GetModuleFileName ( hModOut,
                                    wszDllFullName,
                                      MAX_PATH ) )
    {
      return true;
    }
  }

  wcsncpy_s (
    wszDllFullName,                MAX_PATH,
      L"#Extremely#Invalid.dll#", _TRUNCATE );

  return false;
}

// Detoured so we can get thread names
void
WINAPI
RaiseException_Detour (
      DWORD      dwExceptionCode,
      DWORD      dwExceptionFlags,
      DWORD      nNumberOfArguments,
const ULONG_PTR *lpArguments         )
{
  __try {
    SK_TLS* pTlsThis =
      SK_TLS_Bottom ();

    if (SK_Exception_HandleThreadName (dwExceptionCode, dwExceptionFlags, nNumberOfArguments, lpArguments))
      return;
    
    if (pTlsThis) InterlockedIncrement (&pTlsThis->debug.exceptions);

    auto SK_ExceptionFlagsToStr = [](DWORD dwFlags) -> const char*
    {
      if (dwFlags & EXCEPTION_NONCONTINUABLE)  return "Non-Continuable";
      if (dwFlags & EXCEPTION_UNWINDING)       return "Unwind In Progress";
      if (dwFlags & EXCEPTION_EXIT_UNWIND)     return "Exit Unwind In Progress";
      if (dwFlags & EXCEPTION_STACK_INVALID)   return "Misaligned or Overflowed Stack";
      if (dwFlags & EXCEPTION_NESTED_CALL)     return "Nested Exception Handler";
      if (dwFlags & EXCEPTION_TARGET_UNWIND)   return "Target Unwind In Progress";
      if (dwFlags & EXCEPTION_COLLIDED_UNWIND) return "Collided Exception Handler";
                                               return "Unknown";
    };
    
    
    wchar_t wszCallerName [MAX_PATH * 2 + 1] = { };

    SK_SEH_CompatibleCallerName (
      _ReturnAddress (), wszCallerName
    );

    SK_LOG0 ( ( L"Exception Code: %x  - Flags: (%hs) -  Arg Count: %u   [ Calling Module:  %s ]",
                                        dwExceptionCode,
                SK_ExceptionFlagsToStr (dwExceptionFlags),
                                        nNumberOfArguments,
                                        wszCallerName),
                L"SEH-Except"
            );

    char szSymbol [512] = { };

    SK_GetSymbolNameFromModuleAddr ( SK_GetCallingDLL (),
                            (uintptr_t)_ReturnAddress (),
                                     szSymbol, 511 );

    SK_LOG0 ( ( L"  >> Best-Guess For Source of Exception:  %hs", szSymbol ),
                L"SEH-Except"
            ); 
  }
  __except (EXCEPTION_EXECUTE_HANDLER)
  {
  }
  
  //if (SK_IsDebuggerPresent ())
  //  __debugbreak ();

  RaiseException_Original (
    dwExceptionCode, dwExceptionFlags,
      nNumberOfArguments, lpArguments
    );
}


bool
SK::Diagnostics::Debugger::Allow (bool bAllow)
{
  if (SK_IsHostAppSKIM ())
    return true;

  SK_CreateDLLHook2 (      L"kernel32",
                            "IsDebuggerPresent",
                             IsDebuggerPresent_Detour,
    static_cast_p2p <void> (&IsDebuggerPresent_Original) );

  spoof_debugger = bAllow;

  SK_CreateDLLHook2 (      L"kernel32",
                            "OutputDebugStringA",
                             OutputDebugStringA_Detour,
    static_cast_p2p <void> (&OutputDebugStringA_Original) );

  SK_CreateDLLHook2 (      L"kernel32",
                            "OutputDebugStringW",
                             OutputDebugStringW_Detour,
    static_cast_p2p <void> (&OutputDebugStringW_Original) );

  SK_CreateDLLHook2 (      L"kernel32",
                            "ExitProcess",
                             ExitProcess_Detour,
    static_cast_p2p <void> (&ExitProcess_Original) );

  SK_CreateDLLHook2 (      L"kernel32",
                            "DebugBreak",
                             DebugBreak_Detour,
    static_cast_p2p <void> (&DebugBreak_Original) );

    ///SK_CreateDLLHook2 (      L"NtDlll",
    ///                        "DbgBreakPoint",
    ///                         DbgBreakPoint_Detour,
    ///static_cast_p2p <void> (&DbgBreakPoint_Original) );

  //if (config.system.trace_create_thread)
  //{
  //  SK_CreateDLLHook2 (      L"kernel32",
  //                            "CreateThread",
  //                             CreateThread_Detour,
  //    static_cast_p2p <void> (&CreateThread_Original) );
  //}

  SK_CreateDLLHook2 (  L"kernel32",
                        "GetCommandLineW",
                         GetCommandLineW_Detour,
static_cast_p2p <void> (&GetCommandLineW_Original) );

  SK_CreateDLLHook2 (  L"kernel32",
                        "GetCommandLineA",
                         GetCommandLineA_Detour,
static_cast_p2p <void> (&GetCommandLineA_Original) );

  SK_CreateDLLHook2 (      L"kernel32",
                           "ResetEvent",
                            ResetEvent_Detour,
   static_cast_p2p <void> (&ResetEvent_Original) );

  SK_CreateDLLHook2 (      L"kernel32",
                           "RaiseException",
                            RaiseException_Detour,
   static_cast_p2p <void> (&RaiseException_Original) );

  SK_CreateDLLHook2 (      L"NtDll.dll",
                           "NtCreateThreadEx",
                            NtCreateThreadEx_Detour,
   static_cast_p2p <void> (&NtCreateThreadEx_Original) );
  
  SK_CreateDLLHook2 (      L"NtDll.dll",
                           "NtSetInformationThread",
                            NtSetInformationThread_Detour,
   static_cast_p2p <void> (&NtSetInformationThread_Original) );

  SK_CreateDLLHook2 (      L"kernel32",
                            "SetThreadAffinityMask",
                             SetThreadAffinityMask_Detour,
    static_cast_p2p <void> (&SetThreadAffinityMask_Original) );

  SK_CreateDLLHook2 (      L"kernel32",
                            "GetProcAddress",
                             GetProcAddress_Detour,
    static_cast_p2p <void> (&GetProcAddress_Original) );

  SK_CreateDLLHook2 (      L"kernel32",
                            "SetLastError",
                             SetLastError_Detour,
    static_cast_p2p <void> (&SetLastError_Original) );

  SK_Memory_InitHooks  ();
  SK_File_InitHooks    ();
  SK_Network_InitHooks ();

#ifdef SK_AGGRESSIVE_HOOKS
  SK_ApplyQueuedHooks ();
#endif

  return bAllow;
}

void
SK::Diagnostics::Debugger::SpawnConsole (void)
{
  AllocConsole ();

  static volatile LONG init = FALSE;

  if (! InterlockedCompareExchange (&init, 1, 0))
  {
    _wfreopen (L"CONIN$",  L"r", stdin);
    _wfreopen (L"CONOUT$", L"w", stdout);
    _wfreopen (L"CONOUT$", L"w", stderr);

    SK_CreateDLLHook2 (      L"kernel32",
                              "TerminateProcess",
                               TerminateProcess_Detour,
      static_cast_p2p <void> (&TerminateProcess_Original) );
  }
}

BOOL
SK::Diagnostics::Debugger::CloseConsole (void)
{
  return FreeConsole ();
}

BOOL
WINAPI
SK_IsDebuggerPresent (void)
{
  if (IsDebuggerPresent_Original == nullptr)
  {
    SK::Diagnostics::Debugger::Allow (); // DONTCARE, just init
  }

  if (     IsDebuggerPresent_Original != nullptr )
    return IsDebuggerPresent_Original ();

  return IsDebuggerPresent ();
}







#include <Shlwapi.h>

HMODULE
SK_Debug_LoadHelper (void)
{
//std::lock_guard <SK_Thread_HybridSpinlock> auto_lock (*cs_dbghelp);

  static HMODULE
       hModDbgHelp  = nullptr;
  if ( hModDbgHelp != nullptr )
    return hModDbgHelp;

  wchar_t wszSystemDbgHelp [MAX_PATH * 2 + 1] = { };

  GetSystemDirectory ( wszSystemDbgHelp, MAX_PATH * 2   );
  PathAppendW        ( wszSystemDbgHelp, L"dbghelp.dll" );

  return (
    hModDbgHelp = SK_Modules.LoadLibraryLL (wszSystemDbgHelp)
  );
}

BOOL
IMAGEAPI
SymRefreshModuleList (
  _In_ HANDLE hProcess
)
{
  using SymRefreshModuleList_pfn = BOOL (IMAGEAPI *)(HANDLE hProcess);

  static auto SymRefreshModuleList_Imp =
    (SymRefreshModuleList_pfn)
      GetProcAddress ( SK_Debug_LoadHelper (), "SymRefreshModuleList" );


  if (SymRefreshModuleList_Imp != nullptr)
  {
    SK_SymSetOpts ();

    BOOL bRet =
      SymRefreshModuleList_Imp (hProcess);

    return bRet;
  }


  return FALSE;
}

BOOL
IMAGEAPI
StackWalk64(
    _In_     DWORD                            MachineType,
    _In_     HANDLE                           hProcess,
    _In_     HANDLE                           hThread,
    _Inout_  LPSTACKFRAME64                   StackFrame,
    _Inout_  PVOID                            ContextRecord,
    _In_opt_ PREAD_PROCESS_MEMORY_ROUTINE64   ReadMemoryRoutine,
    _In_opt_ PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,
    _In_opt_ PGET_MODULE_BASE_ROUTINE64       GetModuleBaseRoutine,
    _In_opt_ PTRANSLATE_ADDRESS_ROUTINE64     TranslateAddress
    )
{
  using StackWalk64_pfn = BOOL (IMAGEAPI *)(_In_     DWORD                            MachineType,
                                            _In_     HANDLE                           hProcess,
                                            _In_     HANDLE                           hThread,
                                            _Inout_  LPSTACKFRAME64                   StackFrame,
                                            _Inout_  PVOID                            ContextRecord,
                                            _In_opt_ PREAD_PROCESS_MEMORY_ROUTINE64   ReadMemoryRoutine,
                                            _In_opt_ PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,
                                            _In_opt_ PGET_MODULE_BASE_ROUTINE64       GetModuleBaseRoutine,
                                            _In_opt_ PTRANSLATE_ADDRESS_ROUTINE64     TranslateAddress);

  static auto StackWalk64_Imp =
    (StackWalk64_pfn)
      GetProcAddress ( SK_Debug_LoadHelper (), "StackWalk64" );

  if (StackWalk64_Imp != nullptr)
  {
  //std::lock_guard <SK_Thread_HybridSpinlock> auto_lock (*cs_dbghelp);

    SK_SymSetOpts ();

    return
      StackWalk64_Imp ( MachineType,
                          hProcess, hThread,
                            StackFrame, ContextRecord,
                              ReadMemoryRoutine,
                              FunctionTableAccessRoutine,
                              GetModuleBaseRoutine,
                                TranslateAddress );
  }

  return FALSE;
}

BOOL
IMAGEAPI
StackWalk (
    _In_     DWORD                          MachineType,
    _In_     HANDLE                         hProcess,
    _In_     HANDLE                         hThread,
    _Inout_  LPSTACKFRAME                   StackFrame,
    _Inout_  PVOID                          ContextRecord,
    _In_opt_ PREAD_PROCESS_MEMORY_ROUTINE   ReadMemoryRoutine,
    _In_opt_ PFUNCTION_TABLE_ACCESS_ROUTINE FunctionTableAccessRoutine,
    _In_opt_ PGET_MODULE_BASE_ROUTINE       GetModuleBaseRoutine,
    _In_opt_ PTRANSLATE_ADDRESS_ROUTINE     TranslateAddress
    )
{
  using StackWalk_pfn = BOOL (IMAGEAPI *)(_In_     DWORD                          MachineType,
                                          _In_     HANDLE                         hProcess,
                                          _In_     HANDLE                         hThread,
                                          _Inout_  LPSTACKFRAME                   StackFrame,
                                          _Inout_  PVOID                          ContextRecord,
                                          _In_opt_ PREAD_PROCESS_MEMORY_ROUTINE   ReadMemoryRoutine,
                                          _In_opt_ PFUNCTION_TABLE_ACCESS_ROUTINE FunctionTableAccessRoutine,
                                          _In_opt_ PGET_MODULE_BASE_ROUTINE       GetModuleBaseRoutine,
                                          _In_opt_ PTRANSLATE_ADDRESS_ROUTINE     TranslateAddress);

  static auto StackWalk_Imp =
    (StackWalk_pfn)
      GetProcAddress ( SK_Debug_LoadHelper (), "StackWalk" );

  if (StackWalk_Imp != nullptr)
  {
  //std::lock_guard <SK_Thread_HybridSpinlock> auto_lock (*cs_dbghelp);

    SK_SymSetOpts ();

    return
      StackWalk_Imp ( MachineType,
                        hProcess, hThread,
                          StackFrame, ContextRecord,
                            ReadMemoryRoutine,
                            FunctionTableAccessRoutine,
                            GetModuleBaseRoutine,
                              TranslateAddress );
  }

  return FALSE;
}


DWORD
IMAGEAPI
SymSetOptions (
  _In_ DWORD SymOptions
)
{
  using SymSetOptions_pfn = DWORD (IMAGEAPI *)(_In_ DWORD SymOptions);

  static auto SymSetOptions_Imp =
    (SymSetOptions_pfn)
      GetProcAddress ( SK_Debug_LoadHelper (), "SymSetOptions" );

  if (SymSetOptions_Imp != nullptr)
  {
  //std::lock_guard <SK_Thread_HybridSpinlock> auto_lock (*cs_dbghelp);

    return
      SymSetOptions_Imp (SymOptions);
  }

  return 0x0;
}


DWORD64
IMAGEAPI
SymGetModuleBase64 (
  _In_ HANDLE  hProcess,
  _In_ DWORD64 qwAddr
)
{
  using SymGetModuleBase64_pfn = DWORD64 (IMAGEAPI *)(_In_ HANDLE  hProcess,
                                                      _In_ DWORD64 qwAddr);

  static auto SymGetModuleBase64_Imp =
    (SymGetModuleBase64_pfn)
      GetProcAddress ( SK_Debug_LoadHelper (), "SymGetModuleBase64" );

  if (SymGetModuleBase64_Imp != nullptr)
  {
  //std::lock_guard <SK_Thread_HybridSpinlock> auto_lock (*cs_dbghelp);

    SK_SymSetOpts ();

    return SymGetModuleBase64_Imp ( hProcess, qwAddr );
  }

  return 0x0;
}

DWORD
IMAGEAPI
SymGetModuleBase (
  _In_ HANDLE hProcess,
  _In_ DWORD  dwAddr
)
{
  using SymGetModuleBase_pfn = DWORD (IMAGEAPI *)(_In_ HANDLE  hProcess,
                                                  _In_ DWORD   dwAddr);

  static auto SymGetModuleBase_Imp =
    (SymGetModuleBase_pfn)
      GetProcAddress ( SK_Debug_LoadHelper (), "SymGetModuleBase" );

  if (SymGetModuleBase_Imp != nullptr)
  {
  //std::lock_guard <SK_Thread_HybridSpinlock> auto_lock (*cs_dbghelp);

    SK_SymSetOpts ();

    return SymGetModuleBase_Imp ( hProcess, dwAddr );
  }

  return 0x0;
}


BOOL
IMAGEAPI
SymGetLineFromAddr64 (
  _In_  HANDLE           hProcess,
  _In_  DWORD64          qwAddr,
  _Out_ PDWORD           pdwDisplacement,
  _Out_ PIMAGEHLP_LINE64 Line64
)
{
  using SymGetLineFromAddr64_pfn = BOOL (IMAGEAPI *)(_In_  HANDLE           hProcess,
                                                     _In_  DWORD64          qwAddr,
                                                     _Out_ PDWORD           pdwDisplacement,
                                                     _Out_ PIMAGEHLP_LINE64 Line64);


  static auto SymGetLineFromAddr64_Imp =
    (SymGetLineFromAddr64_pfn)
      GetProcAddress ( SK_Debug_LoadHelper (), "SymGetLineFromAddr64" );

  if (SymGetLineFromAddr64_Imp != nullptr)
  {
  //std::lock_guard <SK_Thread_HybridSpinlock> auto_lock (*cs_dbghelp);

    SK_SymSetOpts ();

    return SymGetLineFromAddr64_Imp ( hProcess, qwAddr, pdwDisplacement, Line64 );
  }

  return FALSE;
}

BOOL
IMAGEAPI
SymGetLineFromAddr (
  _In_  HANDLE           hProcess,
  _In_  DWORD            dwAddr,
  _Out_ PDWORD           pdwDisplacement,
  _Out_ PIMAGEHLP_LINE   Line
)
{
  using SymGetLineFromAddr_pfn = BOOL (IMAGEAPI *)(_In_  HANDLE         hProcess,
                                                   _In_  DWORD          dwAddr,
                                                   _Out_ PDWORD         pdwDisplacement,
                                                   _Out_ PIMAGEHLP_LINE Line);

  static auto SymGetLineFromAddr_Imp =
    (SymGetLineFromAddr_pfn)
      GetProcAddress ( SK_Debug_LoadHelper (), "SymGetLineFromAddr" );

  if (SymGetLineFromAddr_Imp != nullptr)
  {
  //std::lock_guard <SK_Thread_HybridSpinlock> auto_lock (*cs_dbghelp);

    SK_SymSetOpts ();

    return SymGetLineFromAddr_Imp ( hProcess, dwAddr, pdwDisplacement, Line );
  }

  return FALSE;
}


BOOL
IMAGEAPI
SymInitialize (
  _In_     HANDLE hProcess,
  _In_opt_ PCSTR  UserSearchPath,
  _In_     BOOL   fInvadeProcess
)
{
  using SymInitialize_pfn = BOOL (IMAGEAPI *)( _In_     HANDLE hProcess,
                                               _In_opt_ PCSTR  UserSearchPath,
                                               _In_     BOOL   fInvadeProcess );


  static auto SymInitialize_Imp =
    (SymInitialize_pfn)
      GetProcAddress ( SK_Debug_LoadHelper (), "SymInitialize" );

  if (SymInitialize_Imp != nullptr)
  {
  //std::lock_guard <SK_Thread_HybridSpinlock> auto_lock (*cs_dbghelp);

    return SymInitialize_Imp ( hProcess, UserSearchPath, fInvadeProcess );
  }

  return FALSE;
}


BOOL
IMAGEAPI
SymUnloadModule (
  _In_ HANDLE hProcess,
  _In_ DWORD  BaseOfDll
)
{
  using SymUnloadModule_pfn = BOOL (IMAGEAPI *)( _In_ HANDLE hProcess,
                                                 _In_ DWORD  BaseOfDll );


  static auto SymUnloadModule_Imp =
    (SymUnloadModule_pfn)
      GetProcAddress ( SK_Debug_LoadHelper (), "SymUnloadModule" );

  if (SymUnloadModule_Imp != nullptr)
  {
  //std::lock_guard <SK_Thread_HybridSpinlock> auto_lock (*cs_dbghelp);

    SK_SymSetOpts ();

    return SymUnloadModule_Imp ( hProcess, BaseOfDll );
  }

  return FALSE;
}

BOOL
IMAGEAPI
SymUnloadModule64 (
  _In_ HANDLE  hProcess,
  _In_ DWORD64 BaseOfDll
)
{
  using SymUnloadModule64_pfn = BOOL (IMAGEAPI *)( _In_ HANDLE  hProcess,
                                                   _In_ DWORD64 BaseOfDll );


  static auto SymUnloadModule64_Imp =
    (SymUnloadModule64_pfn)
      GetProcAddress ( SK_Debug_LoadHelper (), "SymUnloadModule64" );

  if (SymUnloadModule64_Imp != nullptr)
  {
  //std::lock_guard <SK_Thread_HybridSpinlock> auto_lock (*cs_dbghelp);

    SK_SymSetOpts ();

    return SymUnloadModule64_Imp ( hProcess, BaseOfDll );
  }

  return FALSE;
}


using SymFromAddr_pfn = BOOL (IMAGEAPI *)( _In_      HANDLE       hProcess,
                                           _In_      DWORD64      Address,
                                           _Out_opt_ PDWORD64     Displacement,
                                           _Inout_   PSYMBOL_INFO Symbol );

BOOL
__stdcall
SAFE_SymFromAddr (
  _In_      HANDLE          hProcess,
  _In_      DWORD64         Address,
  _Out_opt_ PDWORD64        Displacement,
  _Inout_   PSYMBOL_INFO    Symbol,
            SymFromAddr_pfn Trampoline )
{
  __try {
    return Trampoline (hProcess, Address, Displacement, Symbol);
  }
  __except ( (GetExceptionCode () & EXCEPTION_NONCONTINUABLE) ? EXCEPTION_EXECUTE_HANDLER :
                                                                EXCEPTION_CONTINUE_SEARCH ) { return FALSE; }
}

BOOL
IMAGEAPI
SymFromAddr (
  _In_      HANDLE       hProcess,
  _In_      DWORD64      Address,
  _Out_opt_ PDWORD64     Displacement,
  _Inout_   PSYMBOL_INFO Symbol
)
{
  using SymFromAddr_pfn = BOOL (IMAGEAPI *)( _In_      HANDLE       hProcess,
                                             _In_      DWORD64      Address,
                                             _Out_opt_ PDWORD64     Displacement,
                                             _Inout_   PSYMBOL_INFO Symbol );

  static auto SymFromAddr_Imp =
    (SymFromAddr_pfn)
      GetProcAddress ( SK_Debug_LoadHelper (), "SymFromAddr" );

  if (SymFromAddr_Imp != nullptr)
  {
  //std::lock_guard <SK_Thread_HybridSpinlock> auto_lock (*cs_dbghelp);

    SK_SymSetOpts ();

    return SAFE_SymFromAddr (hProcess, Address, Displacement, Symbol, SymFromAddr_Imp);
  }

  return FALSE;
}


BOOL
IMAGEAPI
SymCleanup (
  _In_ HANDLE hProcess )
{
  using SymCleanup_pfn = BOOL (IMAGEAPI *)( _In_ HANDLE hProcess );

  static auto SymCleanup_Imp =
    (SymCleanup_pfn)
      GetProcAddress ( SK_Debug_LoadHelper (), "SymCleanup" );

  if (SymCleanup_Imp != nullptr)
  {
  //std::lock_guard <SK_Thread_HybridSpinlock> auto_lock (*cs_dbghelp);

    return SymCleanup_Imp ( hProcess );
  }

  return FALSE;
}



concurrency::concurrent_unordered_set <HMODULE> _SK_DbgHelp_LoadedModules__32;
concurrency::concurrent_unordered_set <HMODULE> _SK_DbgHelp_LoadedModules__64;

DWORD
IMAGEAPI
SymLoadModule (
  _In_     HANDLE hProcess,
  _In_opt_ HANDLE hFile,
  _In_opt_ PCSTR  ImageName,
  _In_opt_ PCSTR  ModuleName,
  _In_     DWORD  BaseOfDll,
  _In_     DWORD  SizeOfDll
)
{
  HMODULE hMod = { };

  if (! GetModuleHandleEx ( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                              GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                (LPCWSTR)(DWORD_PTR)BaseOfDll, &hMod ) )
  {
    return FALSE;
  }

  if (! _SK_DbgHelp_LoadedModules__32.count (hMod))
  {
    using SymLoadModule_pfn = DWORD (IMAGEAPI *)( _In_     HANDLE hProcess,
                                                  _In_opt_ HANDLE hFile,
                                                  _In_opt_ PCSTR  ImageName,
                                                  _In_opt_ PCSTR  ModuleName,
                                                  _In_     DWORD  BaseOfDll,
                                                  _In_     DWORD  SizeOfDll );

    static auto SymLoadModule_Imp =
      (SymLoadModule_pfn)
        GetProcAddress ( SK_Debug_LoadHelper (), "SymLoadModule" );

    if (SymLoadModule_Imp != nullptr)
    {
      SK_SymSetOpts ();

      BOOL bRet;

      std::lock_guard <SK_Thread_HybridSpinlock> auto_lock (*cs_dbghelp);

      if (! _SK_DbgHelp_LoadedModules__32.count (hMod))
        bRet = ( SymLoadModule_Imp ( hProcess, hFile, ImageName, ModuleName, BaseOfDll, SizeOfDll ) != 0 );
      else
        return TRUE;

      if (bRet)
      {
        _SK_DbgHelp_LoadedModules__32.insert (hMod);

        return TRUE;
      }
    }

    return FALSE;
  }

  return TRUE;
}

DWORD64
IMAGEAPI
SymLoadModule64 (
  _In_     HANDLE  hProcess,
  _In_opt_ HANDLE  hFile,
  _In_opt_ PCSTR   ImageName,
  _In_opt_ PCSTR   ModuleName,
  _In_     DWORD64 BaseOfDll,
  _In_     DWORD   SizeOfDll
)
{
  HMODULE hMod = { };

  if (! GetModuleHandleEx ( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                              GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                (LPCWSTR)BaseOfDll, &hMod ) )
  {
    return FALSE;
  }

  if (! _SK_DbgHelp_LoadedModules__64.count (hMod))
  {
    using SymLoadModule64_pfn = DWORD64 (IMAGEAPI *)( _In_     HANDLE  hProcess,
                                                      _In_opt_ HANDLE  hFile,
                                                      _In_opt_ PCSTR   ImageName,
                                                      _In_opt_ PCSTR   ModuleName,
                                                      _In_     DWORD64 BaseOfDll,
                                                      _In_     DWORD   SizeOfDll );

    static auto SymLoadModule64_Imp =
      (SymLoadModule64_pfn)
        GetProcAddress ( SK_Debug_LoadHelper (), "SymLoadModule64" );

    if (SymLoadModule64_Imp != nullptr)
    {
      SK_SymSetOpts ();
    
      BOOL bRet;

      std::lock_guard <SK_Thread_HybridSpinlock> auto_lock (*cs_dbghelp);

      if (! _SK_DbgHelp_LoadedModules__64.count (hMod))
        bRet = ( SymLoadModule64_Imp ( hProcess, hFile, ImageName, ModuleName, BaseOfDll, SizeOfDll ) != 0 );
      else
        return TRUE;

      if (bRet)
      {
        _SK_DbgHelp_LoadedModules__64.insert (hMod);

        return TRUE;
      }
    }

    return FALSE;
  }

  return TRUE;
}

typedef BOOL (IMAGEAPI *SymSetSearchPathW_pfn)(HANDLE,PCWSTR);

BOOL
IMAGEAPI
SymSetSearchPathW (
    _In_     HANDLE hProcess,
    _In_opt_ PCWSTR SearchPath )
{
  using SymSetSearchPathW_pfn = BOOL (IMAGEAPI *)( _In_     HANDLE hProcess,
                                                   _In_opt_ PCWSTR SearchPath );

  static auto SymSetSearchPathW_Imp =
    (SymSetSearchPathW_pfn)
      GetProcAddress ( SK_Debug_LoadHelper (), "SymSetSearchPathW" );

  if (SymSetSearchPathW_Imp != nullptr)
  {
  //std::lock_guard <SK_Thread_HybridSpinlock> auto_lock (*cs_dbghelp);

    return SymSetSearchPathW_Imp (hProcess, SearchPath);
  }

  return FALSE;
}