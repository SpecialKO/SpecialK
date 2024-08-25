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
#include <winternl.h>

#include <steamapi/isteamcontroller.h>
#include <SpecialK/input/input.h>

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"DebugUtils"

#pragma warning (push)
#pragma warning (disable : 4714)

const wchar_t*
SK_SEH_CompatibleCallerName (LPCVOID lpAddr);

using GetCommandLineW_pfn = LPWSTR (WINAPI *)(void);
      GetCommandLineW_pfn
      GetCommandLineW_Original    = nullptr;

using GetCommandLineA_pfn = LPSTR  (WINAPI *)(void);
      GetCommandLineA_pfn
      GetCommandLineA_Original    = nullptr;

using TerminateThread_pfn = BOOL (WINAPI *)( _In_ HANDLE hThread,
                                             _In_ DWORD  dwExitCode );
      TerminateThread_pfn
      TerminateThread_Original    = nullptr;

using ExitThread_pfn = VOID (WINAPI *)(_In_ DWORD  dwExitCode);
      ExitThread_pfn
      ExitThread_Original         = nullptr;

using _endthreadex_pfn = void (__cdecl *)( _In_ unsigned _ReturnCode );
      _endthreadex_pfn
      _endthreadex_Original       = nullptr;

using NtTerminateProcess_pfn = NTSTATUS (*)(HANDLE, NTSTATUS);
      NtTerminateProcess_pfn
      NtTerminateProcess_Original = nullptr;

using RtlExitUserThread_pfn = VOID (NTAPI *)(_In_ NTSTATUS 	Status);
      RtlExitUserThread_pfn
      RtlExitUserThread_Original  = nullptr;

using CloseHandle_pfn = BOOL (WINAPI *)(HANDLE);

TerminateProcess_pfn   TerminateProcess_Original   = nullptr;
ExitProcess_pfn        ExitProcess_Original        = nullptr;
ExitProcess_pfn        ExitProcess_Hook            = nullptr;
OutputDebugStringA_pfn OutputDebugStringA_Original = nullptr;
OutputDebugStringW_pfn OutputDebugStringW_Original = nullptr;
CloseHandle_pfn        CloseHandle_Original        = nullptr;

bool SK_SEH_CompatibleCallerName (LPCVOID lpAddr, wchar_t* wszDllFullName);\

HMODULE
SK_Debug_LoadHelper (void)
{
  static          HMODULE hModDbgHelp  = nullptr;
  static volatile LONG    __init       = 0;

  // Isolate and load the system DLL as a different module since
  //   dbghelp.dll is not threadsafe and other software may be using
  //     the system DLL.
  if (! InterlockedCompareExchangeAcquire (&__init, 1, 0))
  {
    static std::filesystem::path path_to_driver =
      std::filesystem::path (SK_GetInstallPath ()) /
                             LR"(Drivers\Dbghelp\)";

    wchar_t wszSystemDbgHelp   [MAX_PATH + 2] = { };
    wchar_t wszIsolatedDbgHelp [MAX_PATH + 2] = { };

    GetSystemDirectory (wszSystemDbgHelp, MAX_PATH      );
    PathAppendW        (wszSystemDbgHelp, L"dbghelp.dll");

    lstrcatW           (wszIsolatedDbgHelp, path_to_driver.c_str ());
    PathAppendW        (wszIsolatedDbgHelp,
                            SK_RunLHIfBitness (64, L"dbghelp_sk64.dll",
                                                   L"dbghelp_sk32.dll"));

    if ( FALSE == PathFileExistsW          (wszIsolatedDbgHelp) ||
         FALSE == SK_Assert_SameDLLVersion (wszIsolatedDbgHelp,
                                               wszSystemDbgHelp)
       )
    {
      if (PathFileExistsW (wszIsolatedDbgHelp))
      {
        SK_LOGi0 (
          L"Performing in-place upgrade for %ws",
            SK_StripUserNameFromPathW (
              (std::wstring (wszIsolatedDbgHelp) + L'\0').data ()
                                      )
                 );

        wchar_t
          wszTemp [MAX_PATH + 2] = { };

        GetTempFileNameW   (path_to_driver.c_str   (),
                            L"SKI", SK_timeGetTime (),
                                                wszTemp);
        SK_File_MoveNoFail (wszIsolatedDbgHelp, wszTemp);
      }

      SK_DeleteTemporaryFiles (path_to_driver.c_str ());

      SK_CreateDirectories (wszIsolatedDbgHelp);
      CopyFileW            (
          wszSystemDbgHelp, wszIsolatedDbgHelp, TRUE);
    }

    _Notnull_ auto* mods =
       SK_Modules.getPtr ();

    hModDbgHelp =
      mods->LoadLibrary (wszIsolatedDbgHelp);

    InterlockedIncrementRelease (&__init);
  }

  else
    SK_Thread_SpinUntilAtomicMin (&__init, 2);

  SK_ReleaseAssert (hModDbgHelp != nullptr);

  return
    hModDbgHelp;
}


static SymGetSearchPathW_pfn    SymGetSearchPathW_Imp    = nullptr;
static SymSetSearchPathW_pfn    SymSetSearchPathW_Imp    = nullptr;
static SymRefreshModuleList_pfn SymRefreshModuleList_Imp = nullptr;
static StackWalk64_pfn          StackWalk64_Imp          = nullptr;
static StackWalk_pfn            StackWalk_Imp            = nullptr;
static SymSetOptions_pfn        SymSetOptions_Imp        = nullptr;
static SymSetExtendedOption_pfn SymSetExtendedOption_Imp = nullptr;
static SymGetModuleBase64_pfn   SymGetModuleBase64_Imp   = nullptr;
static SymGetModuleBase_pfn     SymGetModuleBase_Imp     = nullptr;
static SymGetLineFromAddr64_pfn SymGetLineFromAddr64_Imp = nullptr;
static SymGetLineFromAddr_pfn   SymGetLineFromAddr_Imp   = nullptr;
static SymFromAddr_pfn          SymFromAddr_Imp          = nullptr;
static SymInitialize_pfn        SymInitialize_Imp        = nullptr;
static SymCleanup_pfn           SymCleanup_Imp           = nullptr;
static SymLoadModule_pfn        SymLoadModule_Imp        = nullptr;
static SymLoadModule64_pfn      SymLoadModule64_Imp      = nullptr;
static SymUnloadModule_pfn      SymUnloadModule_Imp      = nullptr;
static SymUnloadModule64_pfn    SymUnloadModule64_Imp    = nullptr;
static SymGetTypeInfo_pfn       SymGetTypeInfo_Imp       = nullptr;



void
SK_SymSetOpts (void);

using SetLastError_pfn = void (WINAPI *)(_In_ DWORD dwErrCode);
      SetLastError_pfn
      SetLastError_Original = nullptr;

using GetProcAddress_pfn = FARPROC (WINAPI *)(HMODULE,LPCSTR);
      GetProcAddress_pfn
      GetProcAddress_Original = nullptr;

using LdrLockLoaderLock_pfn   = NTSTATUS (WINAPI *)(ULONG Flags, ULONG *State, ULONG_PTR *Cookie);
using LdrUnlockLoaderLock_pfn = NTSTATUS (WINAPI *)(ULONG Flags,               ULONG_PTR  Cookie);

#define _EXPLICIT_LOCK

NTSTATUS
WINAPI
SK_Module_LockLoader ( ULONG_PTR *pCookie,
                       ULONG        Flags,
                       ULONG     *pState )
{
  // The lock must not be acquired until DllMain (...) returns!
  if (ReadAcquire (&__SK_DLL_Refs) < 1)
    return STATUS_SUCCESS; // No-Op

#ifndef _EXPLICIT_LOCK
  UNREFERENCED_PARAMETER (pCookie);
  UNREFERENCED_PARAMETER (Flags);
  UNREFERENCED_PARAMETER (pState);
  return 0;
#else
  static LdrLockLoaderLock_pfn
         LdrLockLoaderLock =
        (LdrLockLoaderLock_pfn)SK_GetProcAddress (L"NtDll.dll",
        "LdrLockLoaderLock");

  if (LdrLockLoaderLock == nullptr)
    return (NTSTATUS)-1;

  ULONG_PTR Cookie = 0;

  SK_ReleaseAssert (! ((Flags & LDR_LOCK_LOADER_LOCK_FLAG_TRY_ONLY) && pState == nullptr));

  NTSTATUS ret    =
    LdrLockLoaderLock (Flags, pState, &Cookie);

  if (pCookie != nullptr && ret == STATUS_SUCCESS)
     *pCookie  = Cookie;

  return ret;
#endif
}

NTSTATUS
WINAPI
SK_Module_UnlockLoader ( ULONG     Flags,
                         ULONG_PTR Cookie )
{
#ifndef _EXPLICIT_LOCK
  UNREFERENCED_PARAMETER (Cookie);
  UNREFERENCED_PARAMETER (Flags);
  return 0;
#else
  static LdrUnlockLoaderLock_pfn
         LdrUnlockLoaderLock =
        (LdrUnlockLoaderLock_pfn)SK_GetProcAddress (L"NtDll.dll",
        "LdrUnlockLoaderLock");

  if (LdrUnlockLoaderLock == nullptr)
    return (NTSTATUS)-1;

  return
    LdrUnlockLoaderLock (Flags, Cookie);
#endif
}


BOOL
WINAPI
SK_Module_IsProcAddrLocal ( HMODULE                    hModExpected,
                            LPCSTR                     lpProcName,
                            FARPROC                    lpProcAddr,
                            PLDR_DATA_TABLE_ENTRY__SK *ppldrEntry )
{
  static LdrFindEntryForAddress_pfn
         LdrFindEntryForAddress =
        (LdrFindEntryForAddress_pfn)SK_GetProcAddress (L"NtDll.dll",
        "LdrFindEntryForAddress");

  // Indeterminate, so ... I guess no?
  if (! LdrFindEntryForAddress)
    return FALSE;

  ULONG                      ldrLockState = 0x0;
  PLDR_DATA_TABLE_ENTRY__SK pLdrEntry  = { };
  ULONG_PTR                  ldrCookie = 0;
  SK_Module_LockLoader     (&ldrCookie, LDR_LOCK_LOADER_LOCK_FLAG_TRY_ONLY, &ldrLockState);

  if ( NT_SUCCESS (
         LdrFindEntryForAddress ( (HMODULE)lpProcAddr,
                                    &pLdrEntry )
                  )
     )
  {
    if (ppldrEntry != nullptr)
       *ppldrEntry = pLdrEntry;

    const UNICODE_STRING_SK* ShortDllName =
      &pLdrEntry->BaseDllName;

    std::wstring ucs_short (
       ShortDllName->Buffer,
      (ShortDllName->Length / sizeof (wchar_t))
    );

    if ( StrStrIW ( SK_GetModuleName (hModExpected).c_str (),
                                          ucs_short.c_str () ) )
    {
      SK_Module_UnlockLoader (0x0, ldrCookie);

      return TRUE;
    }

    else
    {
      std::wstring ucs_full (
         pLdrEntry->FullDllName.Buffer,
        (pLdrEntry->FullDllName.Length / sizeof (wchar_t))
      );

      SK_LOG0 ( ( LR"(Procedure: '%hs' located by NtLdr in '%ws')",
                    lpProcName, ucs_full.c_str () ),
                  L"DebugUtils" );
      SK_LOG0 ( ( L"  >>  Expected Location:  '%ws'!",
                    SK_GetModuleFullName (hModExpected).c_str () ),
                  L"DebugUtils" );

      SK_Module_UnlockLoader (0x0, ldrCookie);

      return FALSE;
    }
  }

  if (ppldrEntry != nullptr)
     *ppldrEntry  = nullptr;

  SK_Module_UnlockLoader (0x0, ldrCookie);

  return FALSE;
}

void
WINAPI
SK_SetLastError (DWORD dwErrCode) noexcept
{
  __try {
    if (SetLastError_Original != nullptr)
        SetLastError_Original (dwErrCode);

    else
      SetLastError (dwErrCode);
  }

  __except (EXCEPTION_EXECUTE_HANDLER) {
  }
}

void
WINAPI
SetLastError_Detour (
  _In_ DWORD dwErrCode
)
{
  SetLastError_Original (dwErrCode);

  if (    ReadAcquire (&__SK_DLL_Ending  ) ||
       (! ReadAcquire (&__SK_DLL_Attached)  )
      )
  {
    return;
  }

  if (dwErrCode != NO_ERROR)
  {
    if (_ReturnAddress () != &SetLastError_Detour)
    {
      SK_TLS* pTLS =
        SK_TLS_Bottom ();

      if (pTLS != nullptr)
      {
        auto& err_state =
          pTLS->win32->error_state;

        err_state.call_site    = _ReturnAddress ();
        err_state.code         = dwErrCode;
        GetSystemTimeAsFileTime (&err_state.last_time);
      }
    }
  }
}


BOOL
__stdcall
SK_Util_PinModule (HMODULE hModToPin)
{ static
  concurrency::concurrent_unordered_set
                   < HMODULE > __pinned;

  if (__pinned.find (hModToPin) != __pinned.cend ())
    return TRUE;

  HMODULE hModDontCare;

  GetModuleHandleEx ( GET_MODULE_HANDLE_EX_FLAG_PIN |
                      GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                        (LPCWSTR)hModToPin,
                          &hModDontCare );

  return TRUE;
}

using farptr = FARPROC;

SK_LazyGlobal <
  concurrency::concurrent_unordered_map
    < HMODULE,       std::unordered_map
    < std::string,           farptr > >
> __SK_CachedDLLProcAddrs;

HRESULT
WINAPI
D3D11On12CreateDevice_NOP (        _In_ IUnknown*             ,
                                        UINT                  ,
  _In_reads_opt_( FeatureLevels ) CONST D3D_FEATURE_LEVEL*    ,
                                        UINT                  ,
            _In_reads_opt_( NumQueues ) IUnknown* CONST*      ,
                                        UINT                  ,
                                        UINT                  ,
                       _COM_Outptr_opt_ ID3D11Device**        ,
                       _COM_Outptr_opt_ ID3D11DeviceContext** ,
                       _Out_opt_        D3D_FEATURE_LEVEL*    )
{
  return E_NOTIMPL;
}

using SteamAPI_RunCallbacks_pfn            = void (S_CALLTYPE *)(void);
SteamAPI_RunCallbacks_pfn realRunCallbacks = nullptr;

void S_CALLTYPE
SteamAPI_RunCallbacks_throttled (void)
{
  static UINT64 ullLastCallback = 0;

  if ((SK_QueryPerf ().QuadPart - ullLastCallback) > sk::narrow_cast <UINT64>((double)SK_PerfFreq * 0.025))
  {
    ullLastCallback =
      SK_QueryPerf ().QuadPart;

    realRunCallbacks ();
  }

  return;
}

bool S_CALLTYPE
SteamAPI_IsSteamRunning_override (void)
{
  return false;
}


extern bool SK_ImGui_WantGamepadCapture (void);

#define SK_STEAM_READ(type)  SK_Steam_Backend->markRead   (type);
#define SK_STEAM_WRITE(type) SK_Steam_Backend->markWrite  (type);
#define SK_STEAM_VIEW(type)  SK_Steam_Backend->markViewed (type);

using SteamAPI_ISteamInput_GetDigitalActionData_pfn = ControllerDigitalActionData_t (S_CALLTYPE *)(ISteamController *, ControllerHandle_t, ControllerDigitalActionHandle_t);
using SteamAPI_ISteamInput_GetAnalogActionData_pfn  = ControllerAnalogActionData_t  (S_CALLTYPE *)(ISteamController *, ControllerHandle_t, ControllerAnalogActionHandle_t);

SteamAPI_ISteamInput_GetDigitalActionData_pfn realGetDigitalActionData = nullptr;
SteamAPI_ISteamInput_GetAnalogActionData_pfn  realGetAnalogActionData  = nullptr;

ControllerDigitalActionData_t
SteamAPI_ISteamInput_GetDigitalActionData_override (ISteamController *This, ControllerHandle_t controllerHandle, ControllerDigitalActionHandle_t digitalActionHandle)
{
  static ControllerDigitalActionData_t
    neutralized = { false, false };

  if (SK_ImGui_WantGamepadCapture () || config.input.gamepad.steam.disabled_to_game)
    return neutralized;

  SK_STEAM_READ (sk_input_dev_type::Gamepad);

  return
    realGetDigitalActionData (This, controllerHandle, digitalActionHandle);
}

ControllerAnalogActionData_t
SteamAPI_ISteamInput_GetAnalogActionData_override (ISteamController *This, ControllerHandle_t controllerHandle, ControllerAnalogActionHandle_t analogActionHandle)
{
  static ControllerAnalogActionData_t
    neutralized = { k_EControllerSourceMode_None, 0.0f, 0.0f, false };

  if (SK_ImGui_WantGamepadCapture () || config.input.gamepad.steam.disabled_to_game)
    return neutralized;

  SK_STEAM_READ (sk_input_dev_type::Gamepad);

  return
    realGetAnalogActionData (This, controllerHandle, analogActionHandle);
}

// Steam overlay workaround
//#define _PROC_ADDR_REHOOK_MESSAGE_PUMP 1

FARPROC
WINAPI
GetProcAddress_Detour     (
  _In_ HMODULE hModule,
  _In_ LPCSTR  lpProcName )
{
  // Let someone else sort this out
  if (lpProcName == nullptr)
  {
    return
      SK_GetProcAddress (hModule, lpProcName);
  }

  if (    ReadAcquire (&__SK_DLL_Ending  ) ||
       (! ReadAcquire (&__SK_DLL_Attached)  )
      )
  {
    //// How did we manage to invoke the hook if the
    ////   trampoline to call the original code is borked?!
    if (GetProcAddress_Original == nullptr)
      return nullptr;
    else
    {
      return
        GetProcAddress_Original (hModule, lpProcName);
    }
  }

  if (hModule == SK_GetDLL () && (uintptr_t)lpProcName > 65535)
  {
    // Handle various exports we do not have stubs for; nothing
    //   is going to have an import dependency on this stuff...
    if ( (SK_GetDLLRole () & DLL_ROLE::D3D11) ==
                             DLL_ROLE::D3D11 )
    {
      extern FARPROC SK_GetProcAddressD3D11 (const char* lpProcName);

      FARPROC d3d11_func =
        SK_GetProcAddressD3D11 (lpProcName);

      if (d3d11_func != (FARPROC)-1)
        return d3d11_func;
    }
  }

  DWORD dwLastErr =
    GetLastError ();

  const HMODULE hModCaller =
    SK_GetCallingDLL ();

  if (hModCaller == SK_GetDLL ())
  {
    SK_SetLastError (NO_ERROR);
    return
      GetProcAddress_Original (hModule, lpProcName);
  }

  ///auto _EstablishKnownDLL =
  ///[&](HMODULE* phMod, const wchar_t* wszDll) -> void
  ///{
  ///  if ((*phMod) == nullptr && SK_GetFramesDrawn () < 30)
  ///  {
  ///    *phMod =
  ///      SK_GetModuleHandleW (wszDll);
  ///  }
  ///  else
  ///    *phMod = (HMODULE)1;
  ///};


  //
  // Sorry Steam, you have to die.
  //
  //   Ordinal 100 (XInputGetStateEx) is not for you (!!)
  //
///////if (lpProcName == (LPCSTR)100)
///////{
///////  if ( StrStrIW ( SK_GetModuleName (hModule).c_str (),
///////                    L"XInput" ) )
///////  {
///////    return nullptr;
///////  }
///////}


  char proc_name [512] = { };

  if ((uintptr_t)lpProcName > 65536)
  {
    strncpy_s ( proc_name,        511,
                lpProcName, _TRUNCATE );
  }

  else
  {
    strncpy_s (
      proc_name, 511,
        SK_FormatString ("Ordinal_%lu", (uint32_t)((uintptr_t)lpProcName & 0xFFFFUL)).c_str (),
           _TRUNCATE );
  }

  // Ignore ordinals for these bypasses
  if ((uintptr_t)lpProcName > 65535UL)
  {
#ifdef _PROC_ADDR_REHOOK_MESSAGE_PUMP
    extern BOOL
      WINAPI
      PeekMessageA_Detour (
        _Out_    LPMSG lpMsg,
        _In_opt_ HWND  hWnd,
        _In_     UINT  wMsgFilterMin,
        _In_     UINT  wMsgFilterMax,
        _In_     UINT  wRemoveMsg );

    extern BOOL
      WINAPI
      PeekMessageW_Detour (
        _Out_    LPMSG lpMsg,
        _In_opt_ HWND  hWnd,
        _In_     UINT  wMsgFilterMin,
        _In_     UINT  wMsgFilterMax,
        _In_     UINT  wRemoveMsg );

    extern BOOL
      WINAPI
        GetMessageW_Detour ( LPMSG lpMsg,          HWND hWnd,
                             UINT   wMsgFilterMin, UINT wMsgFilterMax );

    extern BOOL
      WINAPI
      GetMessageA_Detour ( LPMSG lpMsg,          HWND hWnd,
                           UINT   wMsgFilterMin, UINT wMsgFilterMax );
#endif

    ////
    //// Compat Hack No Longer Needed, but is a handy way to disable
    ////   D3D11On12 interop if a game needs pure D3D12 for some reson.
    ////
    //if (             *lpProcName == 'D' &&
    //     (! lstrcmpA (lpProcName, "D3D11On12CreateDevice")) )
    //{
    //  if (SK_IsInjected ())
    //  {
    //    SK_RunOnce (
    //      SK_ImGui_WarningWithTitle (
    //        SK_FormatStringW (
    //          L"The overlay '%ws' uses D3D11On12 and has been disabled to prevent crashing.\r\n\r\n\t\t"
    //          L">> Please use Special K in Local Injection mode or turn the third-party overlay off.",
    //            SK_GetCallerName ().c_str ()
    //        ).c_str (),
    //        L"A non-D3D12 native overlay was detected."
    //      )
    //    );
    //
    //    return nullptr;
    //  }
    //}

 ////   if ( SK_GetCurrentGameID () == SK_GAME_ID::ResidentEvil8 &&
 ////        *lpProcName == 'S'      &&
 ////StrStrA (lpProcName, "SteamAPI_") == lpProcName)
 ////   {
 ////     if (! lstrcmpA (lpProcName, "SteamAPI_RunCallbacks"))
 ////     {
 ////       if (realRunCallbacks == nullptr)
 ////       {
 ////         realRunCallbacks = (SteamAPI_RunCallbacks_pfn)
 ////           GetProcAddress_Original (
 ////             hModule, lpProcName
 ////           );
 ////       }
 ////
 ////       return
 ////         (FARPROC)SteamAPI_RunCallbacks_throttled;
 ////     }
 ////
 ////     return
 ////       GetProcAddress_Original (
 ////         hModule, lpProcName
 ////       );
 ////   }


#if 0
    if ( *lpProcName == 'X' && StrStrA (lpProcName, "XInput") )
    {
      return
        SK_XInput_GetProcAddress (hModule, lpProcName, _ReturnAddress ());
    }
    else
#endif

    if ( *lpProcName == 'S' &&
StrStrA ( lpProcName, "SteamAPI_") == lpProcName )
    {
#ifdef __FULL_KILL
      if (      *lpProcName == 'S' &&
    (! lstrcmpA (lpProcName, "SteamAPI_ISteamInput_Init")) )
      {
        return nullptr;
      }
#endif

      if (! lstrcmpA (lpProcName, "SteamAPI_ISteamInput_GetAnalogActionData") )
      {
        if (realGetAnalogActionData == nullptr)
        {
          realGetAnalogActionData = (SteamAPI_ISteamInput_GetAnalogActionData_pfn)
            GetProcAddress_Original (
              hModule, lpProcName
            );
        }

        return
          (FARPROC)SteamAPI_ISteamInput_GetAnalogActionData_override;
      }

      else if (! lstrcmpA (lpProcName, "SteamAPI_ISteamInput_GetDigitalActionData") )
      {
        if (realGetDigitalActionData == nullptr)
        {
          realGetDigitalActionData = (SteamAPI_ISteamInput_GetDigitalActionData_pfn)
            GetProcAddress_Original (
              hModule, lpProcName
            );
        }

        return
          (FARPROC)SteamAPI_ISteamInput_GetDigitalActionData_override;
      }
    }

#ifdef _PROC_ADDR_REHOOK_MESSAGE_PUMP
    if ( *lpProcName == 'P'      &&
 StrStrA (lpProcName,   "PeekM") == lpProcName )
    {
      if (! lstrcmpA (lpProcName, "PeekMessageA"))
      {
        return
          (FARPROC)PeekMessageA_Detour;
      }

      else if (! lstrcmpA (lpProcName, "PeekMessageW"))
      {
        return
          (FARPROC) PeekMessageW_Detour;
      }

      return
        GetProcAddress_Original (
          hModule, lpProcName
        );
    }

    else if ( *lpProcName == 'G'     &&
      StrStrA (lpProcName,   "GetM") == lpProcName )
    {
      if (! lstrcmpA (lpProcName, "GetMessageA"))
      {
        return
          (FARPROC) GetMessageA_Detour;
      }

      else if (! lstrcmpA (lpProcName, "GetMessageW"))
      {
        return
          (FARPROC) GetMessageW_Detour;
      }

      return
        GetProcAddress_Original (
          hModule, lpProcName
        );
    }
#endif

    // MSI Nahimic workaround
    if ( *lpProcName == 'N' &&
  (! lstrcmpA (lpProcName, "NoHotPatch")) )
    {
      static     DWORD NoHotPatch = 0x1;
      return (FARPROC)&NoHotPatch;
    }


    ///if (! lstrcmpiA (lpProcName, "RaiseException"))
    ///{
    ///  extern
    ///    void
    ///    WINAPI
    ///    RaiseException_Trap (
    ///      DWORD      dwExceptionCode,
    ///      DWORD      dwExceptionFlags,
    ///      DWORD      nNumberOfArguments,
    ///      const ULONG_PTR *lpArguments);
    ///
    ///  //if (SK_GetCallingDLL () == SK_Modules->HostApp ())
    ///  {
    ///    return
    ///      reinterpret_cast <FARPROC> (RaiseException_Trap);
    ///  }
    ///}


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
        GetProcAddress_Original ( hModule, lpProcName ) );
    }
  }

  std::string
   str_lpProcName (((uintptr_t)lpProcName > 65536) ?
                               lpProcName : "Ordinal");

  bool is_new = true;

  if ((LPCSTR)(intptr_t)lpProcName < (LPCSTR)65536)
  {
    str_lpProcName =
      SK_FormatString ("Ordinal_%lu", (intptr_t)lpProcName);
  }

  // Cache is known to cause problems in XInput 9.1.0 games,
  //   it is not worth it.
#define _NOCACHE 1
#if _NOCACHE
  FARPROC farproc = nullptr;
#else
  auto&    proc_list =
        __SK_CachedDLLProcAddrs.get ()
                      [       hModule];
  auto& farproc =
           proc_list  [str_lpProcName];

  bool reobtanium =
   ( farproc == nullptr &&
     hModule != nullptr  ) ||
   (! ( SK_GetModuleFromAddr   (farproc) == hModule ||
        SK_IsAddressExecutable (farproc, true)         )
   );

  if (reobtanium)
  {
#endif
    farproc =
      GetProcAddress_Original (
        hModule, lpProcName
      );
#if (! _NOCACHE)
  }

  else
    is_new = false;
#endif

  if (farproc != nullptr)
  {
#if (! _NOCACHE)
    SK_SetLastError (NO_ERROR);
#endif
  }

  else
  {
    if (hModule != nullptr)
      SK_SetLastError (ERROR_PROC_NOT_FOUND);
    else
      SK_SetLastError (ERROR_MOD_NOT_FOUND);
  }


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
    if (config.system.log_level > 0 && dll_log->lines > 15)
    {
      if (hModCaller != SK_GetDLL () && is_new)
      {
        SK_LOG3 ( ( LR"(GetProcAddress ([%ws], {Ordinal: %lu})  -  %ws)",
                        SK_GetModuleFullName (hModule).c_str (),
                       (DWORD)((uintptr_t)lpProcName & 0xFFFFU),
                          SK_SummarizeCaller (       ).c_str () ),
                     L"DLL_Loader" );
      }
    }

    SK_SetLastError (dwLastErr);

    return farproc;
  }

  else
  {
    constexpr const char *nv_opt  =
             "NvOptimusEnablement";
    constexpr const char *amd_pwr =
      "AmdPowerXpressRequestHighPerformance";

    if (! strcmp (lpProcName, nv_opt))
    {
      dll_log->Log (L"Optimus Enablement");
      return (FARPROC)&dwOptimus;
    }

    else if (! strcmp (lpProcName, amd_pwr))
      return (FARPROC)&dwAMDPowerXPress;

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


    if (config.system.log_level > 0 && (uintptr_t)lpProcName > 65536)
    {
      static HMODULE hModSteamOverlay =
        SK_GetModuleHandle (
           SK_RunLHIfBitness ( 64, L"GameOverlayRenderer64.dll",
                                   L"GameOverlayRenderer.dll" )
                           );

      if ( hModCaller != SK_GetDLL ()  &&
           hModCaller != hModSteamOverlay )
      {

        SK_LOG3 ( ( LR"(GetProcAddress ([%ws], "%hs")  -  %ws)",
                        SK_GetModuleFullName (hModule).c_str (),
                                              lpProcName,
                          SK_SummarizeCaller (       ).c_str () ),
                     L"DLL_Loader" );
      }
    }
  }

  SK_SetLastError (dwLastErr);

  if ((uintptr_t)farproc > 65536)
    return farproc;

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
SK_TerminateProcess (UINT uExitCode)
{
  const bool abnormal_dll_state =
    ( ReadAcquire (&__SK_DLL_Attached) == 0 ||
      ReadAcquire (&__SK_DLL_Ending)   != 0  );

  if (! abnormal_dll_state)
  {
    if (NtTerminateProcess_Original != nullptr)
    {
      return (
        NtTerminateProcess_Original (GetCurrentProcess (), uExitCode)
      ) == STATUS_SUCCESS;
    }
  }

  if (TerminateProcess_Original != nullptr)
  {
    return
      TerminateProcess_Original (GetCurrentProcess (), uExitCode);
  }

  return
    TerminateProcess (GetCurrentProcess (), uExitCode);
}

using ResetEvent_pfn = BOOL (WINAPI *)(_In_ HANDLE hEvent);
      ResetEvent_pfn
      ResetEvent_Original = nullptr;

BOOL
WINAPI
ResetEvent_Detour (
  _In_ HANDLE hEvent
)
{
  // Compliance failure in some games makes application verifier useless
  if (hEvent == nullptr)
  {
    SK_RunOnce (dll_log->Log ( L"[DebugUtils] Invalid handle passed to "
                               L"ResetEvent (...) - %s",
                SK_SummarizeCaller ().c_str () ));
    return FALSE;
  }

  return
    ResetEvent_Original (hEvent);
}


NTSTATUS
NtTerminateProcess_Detour ( HANDLE   ProcessHandle,
                            NTSTATUS ExitStatus )
{
  const bool abnormal_dll_state =
    ( ReadAcquire (&__SK_DLL_Attached) == 0 ||
      ReadAcquire (&__SK_DLL_Ending)   != 0  );

  if (! abnormal_dll_state)
  {
    if (GetProcessId (ProcessHandle) == GetCurrentProcessId ())
    {
      dll_log->Log ( L" *** BLOCKED NtTerminateProcess (%x) ***\t -- %s",
                    ExitStatus, SK_SummarizeCaller ().c_str () );

      return
        STATUS_SUCCESS;// STATUS_PROCESS_IS_TERMINATING;
    }
  }

  return
    NtTerminateProcess_Original (ProcessHandle, ExitStatus);
}


BOOL
WINAPI
TerminateProcess_Detour ( HANDLE hProcess,
                          UINT   uExitCode )
{
  // A game may Terminate its own process, we want to cleanup SK's DLL
  if ( GetProcessId (      hProcess      ) ==
       GetProcessId (GetCurrentProcess ()) )
  {
    const bool abnormal_dll_state =
      ( ReadAcquire (&__SK_DLL_Attached) == 0 ||
        ReadAcquire (&__SK_DLL_Ending)   != 0  );

    // Ubisoft Anti-Tamper Sillyness, just ignore it.
    if (uExitCode == 0xdeadc0de)
    {
      return FALSE;
    }

    if (SK_GetCurrentGameID () == SK_GAME_ID::AlanWake2)
    {
      // This is the game's faulty third-party overlay detection code,
      //   since it's broken even without SK, we might as well bypass it.
      if (uExitCode == 0xbaadda7a)
      {
        return FALSE;
      }
    }

    if (! abnormal_dll_state)
    {
      SK_LOG0 ( ( L"Software Is Terminating Itself With Exit Code (%x)",
                    uExitCode ), __SK_SUBSYSTEM__ );

      SK_SelfDestruct ();
    }
  }

  return
    TerminateProcess_Original (hProcess, uExitCode);
}

BOOL
WINAPI
TerminateThread_Detour ( HANDLE hThread,
                         DWORD dwExitCode )
{
  const bool abnormal_dll_state =
    ( ReadAcquire (&__SK_DLL_Attached) == 0 ||
      ReadAcquire (&__SK_DLL_Ending)   != 0  );

  if (SK_IsDebuggerPresent () && GetThreadId (hThread) == SK_Thread_GetMainId ())
  {
    __debugbreak ();
    return TRUE;
  }

  if (! abnormal_dll_state)
  {
    // Fake it, TerminateThread is dangerous and often used by DRM only.
    if ( (intptr_t)hThread > 0 )
    {
      DWORD                               dwHandleFlags = 0x0;
      if (GetHandleInformation (hThread, &dwHandleFlags))
      {
        // We need to cover all bases here; this would throw an exception
        //   that anti-debug would use to tear-down the whole game.
        if ((dwHandleFlags & HANDLE_FLAG_PROTECT_FROM_CLOSE) !=
                             HANDLE_FLAG_PROTECT_FROM_CLOSE)
        {
          if (GetThreadId (hThread) != GetCurrentThreadId ())
          {
            // Handle is real, we can close it.
            SuspendThread  (hThread);
            SK_CloseHandle (hThread);
          }
        }
      }
    }

////return TRUE;

    //UNREFERENCED_PARAMETER (uExitCode);
    if (dwExitCode == 0xdeadc0de)
    {
      SK_Thread_Create ([](LPVOID)->
      DWORD
      {
        auto orig_se =
        SK_SEH_ApplyTranslator (SK_BasicStructuredExceptionTranslator);
        try {
          SK_LOG0 ( ( L" !!! Denuvo Catastrophe Avoided !!!" ),
                      L"AntiTamper" );
        }

        catch (const SK_SEH_IgnoredException&) { };
        SK_SEH_RemoveTranslator (orig_se);

        SK_Thread_CloseSelf ();

        return 0;
      });

      SK_ImGui_Warning ( L"Denuvo just tried to terminate the game! "
                         L"Bad Denuvo, bad!" );

      return TRUE;
    }

    SK_Thread_Create ([](LPVOID ret_addr)->DWORD
    {
      wchar_t wszCaller [MAX_PATH + 2] = { };

      auto orig_se =
      SK_SEH_ApplyTranslator (
        SK_FilteringStructuredExceptionTranslator (
          EXCEPTION_ACCESS_VIOLATION
        )
      );
      try
      {
        if (SK_SEH_CompatibleCallerName ((LPCVOID)ret_addr, wszCaller))
        {
          dll_log->Log (L"TerminateThread (...) ***\t -- %s",
                        wszCaller);
        }
      }
      catch (const SK_SEH_IgnoredException&) { };
      SK_SEH_RemoveTranslator (orig_se);

      SK_Thread_CloseSelf ();

      return 0;
    }, (LPVOID)_ReturnAddress ());
  }

  return
    TerminateThread_Original (hThread, dwExitCode);
}


#define SK_Hook_GetTrampoline(Target) \
  Target##_Original != nullptr ?      \
 (Target##_Original)           :      \
            (Target);

void
WINAPI
SK_ExitProcess (UINT uExitCode) noexcept
{
  volatile int            dummy = { };
  UNREFERENCED_PARAMETER (dummy);

  auto jmpExitProcess =
    SK_Hook_GetTrampoline (ExitProcess);

  // Since many, many games don't shutdown cleanly, let's unload ourself.
  SK_SelfDestruct (         ); if (jmpExitProcess != nullptr)
  jmpExitProcess  (uExitCode);
}

void
WINAPI
SK_ExitThread (DWORD dwExitCode) noexcept
{
  volatile int            dummy = { };
  UNREFERENCED_PARAMETER (dummy);

  auto jmpExitThread =
    SK_Hook_GetTrampoline (ExitThread);

  if (jmpExitThread != nullptr)
      jmpExitThread (dwExitCode);
}

BOOL
WINAPI
SK_TerminateThread ( HANDLE    hThread,
                     DWORD     dwExitCode ) noexcept
{
  volatile int            dummy = { };
  UNREFERENCED_PARAMETER (dummy);

  auto jmpTerminateThread =
    SK_Hook_GetTrampoline (TerminateThread);

  if (jmpTerminateThread != nullptr)
  {
    return
      jmpTerminateThread (hThread, dwExitCode);
  }

  return FALSE;
}

BOOL
WINAPI
SK_TerminateProcess ( HANDLE    hProcess,
                      UINT      uExitCode ) noexcept
{
  volatile int            dummy = { };
  UNREFERENCED_PARAMETER (dummy);

  auto jmpTerminateProcess =
    SK_Hook_GetTrampoline (TerminateProcess);

  if ( GetProcessId (      hProcess      ) ==
       GetProcessId (GetCurrentProcess ()) )
  {
    SK_SelfDestruct ();
  }

  if (jmpTerminateProcess != nullptr)
  {
    return
      jmpTerminateProcess (hProcess, uExitCode);
  }

  return FALSE;
}

void
__cdecl
SK__endthreadex (_In_ unsigned _ReturnCode) noexcept
{
  auto jmp_endthreadex =
    SK_Hook_GetTrampoline (_endthreadex);

  if (jmp_endthreadex != nullptr)
      jmp_endthreadex (_ReturnCode);
}


void
__cdecl
_endthreadex_Detour ( _In_ unsigned _ReturnCode )
{
  const bool abnormal_dll_state =
    ( ReadAcquire (&__SK_DLL_Attached) == 0 ||
      ReadAcquire (&__SK_DLL_Ending)   != 0  );

  if (! abnormal_dll_state)
  {
    //UNREFERENCED_PARAMETER (uExitCode);
    if (_ReturnCode == 0xdeadc0de)
    {
      SK_Thread_Create ([](LPVOID)->
      DWORD
      {
        auto orig_se =
        SK_SEH_ApplyTranslator (SK_BasicStructuredExceptionTranslator);
        try {
          SK_LOG0 ( ( L" !!! Denuvo Catastrophe Avoided !!!" ),
                      L"AntiTamper" );
        }

        catch (const SK_SEH_IgnoredException&) { };
        SK_SEH_RemoveTranslator (orig_se);

        SK_Thread_CloseSelf ();

        return 0;
      });

      SK_ImGui_Warning ( L"Denuvo just tried to terminate the game! "
                         L"Bad Denuvo, bad!" );

      return;
    }
  }

  return
    _endthreadex_Original (_ReturnCode);
}

void
WINAPI
ExitThread_Detour ( DWORD dwExitCode )
{
  const bool abnormal_dll_state =
    ( ReadAcquire (&__SK_DLL_Attached) == 0 ||
      ReadAcquire (&__SK_DLL_Ending)   != 0  );

  if (! abnormal_dll_state)
  {
    //UNREFERENCED_PARAMETER (uExitCode);
    if (dwExitCode == 0xdeadc0de)
    {
      SK_Thread_Create ([](LPVOID)->
      DWORD
      {
        auto orig_se =
        SK_SEH_ApplyTranslator (SK_BasicStructuredExceptionTranslator);
        try
        {
          SK_LOG0 ( ( L" !!! Denuvo Catastrophe Avoided !!!" ),
                      L"AntiTamper" );
        }
        catch (const SK_SEH_IgnoredException&) { };
        SK_SEH_RemoveTranslator (orig_se);

        SK_Thread_CloseSelf ();

        return 0;
      });

      SK_ImGui_Warning ( L"Denuvo just tried to terminate the game! "
                         L"Bad Denuvo, bad!" );

      return;
    }
  }

  return
    ExitThread_Original (dwExitCode);
}

void
WINAPI
ExitProcess_Detour (UINT uExitCode)
{
  SK_LOG0 ( ( L"Software Is Ending With Exit Code (%x)",
                    uExitCode ), __SK_SUBSYSTEM__ );

  // Since many, many games don't shutdown cleanly, let's unload ourself.
  SK_SelfDestruct ();

  auto jmpTerminateProcess =
    SK_Hook_GetTrampoline (TerminateProcess);

  jmpTerminateProcess (GetCurrentProcess (), uExitCode);
}

using RtlExitUserProcess_pfn = int (WINAPI*)(NTSTATUS);
      RtlExitUserProcess_pfn
      RtlExitUserProcess_Original = nullptr;
int
WINAPI
RtlExitUserProcess_Detour (NTSTATUS ExitStatus)
{
  // Since many, many games don't shutdown cleanly, let's unload ourself.
  SK_SelfDestruct ();

  auto jmpTerminateProcess =
    SK_Hook_GetTrampoline (TerminateProcess);

  jmpTerminateProcess (GetCurrentProcess (), ExitStatus);

  return 0;
}

void
NTAPI
RtlExitUserThread_Detour (NTSTATUS ExitStatus)
{
  SK_LOG_FIRST_CALL

  RtlExitUserThread_Original (ExitStatus);
}




LPWSTR
WINAPI
GetCommandLineW_Detour (void)
{
  SK_LOG_FIRST_CALL

  static
  wchar_t wszFakeOut [8192] = { };

  if (*wszFakeOut != L'\0')
    return wszFakeOut;

  lstrcpyW (wszFakeOut, L"\"");
  lstrcatW (wszFakeOut, SK_GetFullyQualifiedApp ());
  lstrcatW (wszFakeOut, L"\"");

  if (! lstrcmpW ( SK_GetHostApp (), L"RED-Win64-Shipping.exe" ))
  {
    lstrcatW (wszFakeOut, L" -eac-nop-loaded");
    return    wszFakeOut;
  }

  if (! lstrcmpW ( SK_GetHostApp (), L"DBFighterZ.exe" ))
  {
    lstrcatW (wszFakeOut, L" -eac-nop-loaded");
    return    wszFakeOut;
  }

#ifdef _DEBUG
  if (_wcsicmp (wszFakeOut,                     GetCommandLineW_Original ()))
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
  // Anti-debug survival kit
  DWORD dwLastErr =
    GetLastError ();

  if (! lpOutputString) // This is meaningless, just pass it along
  {
    OutputDebugStringA_Original (lpOutputString);
    SK_SetLastError (dwLastErr);

    return;
  }

  wchar_t    wszModule [MAX_PATH + 2] = { };
  wcsncpy_s (wszModule, MAX_PATH,
                      SK_GetModuleFullNameFromAddr (
                        _ReturnAddress ()
                      ).c_str (),        _TRUNCATE );

  game_debug->LogEx (true,   L"%-72ws:  %hs", wszModule, lpOutputString);

  if (           lpOutputString != nullptr &&
      (! strchr (lpOutputString,  '\n')) )
    game_debug->LogEx    (false, L"\n");

  OutputDebugStringA_Original (lpOutputString);

  SK_SetLastError (dwLastErr);


  //
  // Game Specific Stuff when a game's log is inadequate...
  //
  switch (SK_GetCurrentGameID ())
  {
    case SK_GAME_ID::FinalFantasy7: [[unlikely]]
      if (StrStrIA (lpOutputString, "DIRECT SOUND ERROR"))
      {
        SK_MessageBox (
          L"The game (Final Fantasy 7) has crashed due to DirectSound !!\t"
          L"\r\n\r\n\tConfirm audio setup and try restarting the game.",
            L"Special K -:- ( DirectSound Crash Detected )",
              MB_OK | MB_ICONERROR
        );
      }
      break;
    default: [[likely]]
      break;
  }
}

void
WINAPI
SK_OutputDebugStringW (LPCWSTR lpOutputString)
{
  if ( OutputDebugStringW_Original != nullptr)
       OutputDebugStringW_Original (lpOutputString);
  else OutputDebugStringW          (lpOutputString);
}

void
WINAPI
SK_OutputDebugStringA (LPCSTR lpOutputString)
{
  if ( OutputDebugStringA_Original != nullptr)
       OutputDebugStringA_Original (lpOutputString);
  else OutputDebugStringA          (lpOutputString);
}

void
WINAPI
OutputDebugStringW_Detour (LPCWSTR lpOutputString)
{
  // Anti-debug survival kit
  DWORD dwLastErr =
    GetLastError ();

  if (! lpOutputString) // This is meaningless, just pass it along
  {
    OutputDebugStringW_Original (lpOutputString);
    SK_SetLastError (dwLastErr);

    return;
  }

  wchar_t    wszModule [MAX_PATH + 2] = { };
  wcsncpy_s (wszModule, MAX_PATH,
                      SK_GetModuleFullNameFromAddr (
                        _ReturnAddress ()
                      ).c_str (),        _TRUNCATE );

  game_debug->LogEx (true,   L"%-72ws:  %ws", wszModule, lpOutputString);

  if (           lpOutputString != nullptr &&
      (! wcschr (lpOutputString, L'\n')) )
    game_debug->LogEx    (false, L"\n");

  OutputDebugStringW_Original (lpOutputString);

  SK_SetLastError (dwLastErr);


  // Unused for now
#if 0
  //
  // Game Specific Stuff when a game's log is inadequate...
  //
  switch (SK_GetCurrentGameID ())
  {
    default: [[likely]]
      break;
  }
#endif
}



using GetThreadContext_pfn = BOOL (WINAPI *)(HANDLE,LPCONTEXT);
using SetThreadContext_pfn = BOOL (WINAPI *)(HANDLE,const CONTEXT *);

GetThreadContext_pfn GetThreadContext_Original = nullptr;
SetThreadContext_pfn SetThreadContext_Original = nullptr;

typedef enum _SK_THREAD_INFORMATION_CLASS {
  ThreadBasicInformation,
  ThreadTimes,
  ThreadPriority,
  ThreadBasePriority,
  ThreadAffinityMask,
  ThreadImpersonationToken,
  ThreadDescriptorTableEntry,
  ThreadEnableAlignmentFaultFixup,
  ThreadEventPair,
  ThreadQuerySetWin32StartAddress,
  ThreadZeroTlsCell,
  ThreadPerformanceCount,
  ThreadAmILastThread,
  ThreadIdealProcessor,
  ThreadPriorityBoost,
  ThreadSetTlsArrayAddress,
  ThreadIsIoPending_,
  ThreadHideFromDebugger
} SK_THREAD_INFORMATION_CLASS,
*PSK_THREAD_INFORMATION_CLASS;

#define THREAD_CREATE_FLAGS_CREATE_SUSPENDED        0x00000001
#define THREAD_CREATE_FLAGS_SKIP_THREAD_ATTACH      0x00000002
#define THREAD_CREATE_FLAGS_HIDE_FROM_DEBUGGER      0x00000004
#define THREAD_CREATE_FLAGS_HAS_SECURITY_DESCRIPTOR 0x00000010
#define THREAD_CREATE_FLAGS_ACCESS_CHECK_IN_TARGET  0x00000020
#define THREAD_CREATE_FLAGS_INITIAL_THREAD          0x00000080

using ZwSetInformationThread_pfn = NTSTATUS (NTAPI *)(
  _In_ HANDLE                      ThreadHandle,
  _In_ SK_THREAD_INFORMATION_CLASS ThreadInformationClass,
  _In_ PVOID                       ThreadInformation,
  _In_ ULONG                       ThreadInformationLength
);

// The Nt function is the most obvious choice, but some sneaky
//   software will hop right over this and call Zw... directly.
//
//  --> Thus, don't even bother hooking NtSetInformationThread.
//
using NtSetInformationThread_pfn = NTSTATUS (NTAPI *)(
  _In_ HANDLE                      ThreadHandle,
  _In_ SK_THREAD_INFORMATION_CLASS ThreadInformationClass,
  _In_ PVOID                       ThreadInformation,
  _In_ ULONG                       ThreadInformationLength
);

using NtCreateThreadEx_pfn = NTSTATUS (NTAPI *)(
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
NtCreateThreadEx_pfn       ZwCreateThreadEx_Original       = nullptr;
NtSetInformationThread_pfn NtSetInformationThread_Original = nullptr;
ZwSetInformationThread_pfn ZwSetInformationThread_Original = nullptr;

typedef void (NTAPI* RtlAcquirePebLock_pfn)(void);
typedef void (NTAPI* RtlReleasePebLock_pfn)(void);

static RtlAcquirePebLock_pfn RtlAcquirePebLock_Original = nullptr;
static RtlReleasePebLock_pfn RtlReleasePebLock_Original = nullptr;

#define SK_ANTIDEBUG_PARANOIA_STAGE2
#define SK_ANTIDEBUG_PARANOIA_STAGE3

const wchar_t*
SK_AntiAntiDebug_GetNormalWindowTitle (void)
{
  static std::wstring __normal_window_title =
    SK_GetModuleFullNameFromAddr (SK_GetModuleHandle (nullptr));

  return
    __normal_window_title.c_str ();
}

static bool bRealDebug = false;

LPVOID SK_Debug_GetImageBaseAddr (void)
{
  __try {
    auto pPeb =
      (SK_PPEB)NtCurrentTeb ()->ProcessEnvironmentBlock;

    InterlockedCompareExchangePointer (
      &__SK_GameBaseAddr, pPeb->ImageBaseAddress, nullptr
    );

    return pPeb->ImageBaseAddress;
  }

  __except (EXCEPTION_EXECUTE_HANDLER)
  {
    return nullptr;
  }
}

void
SK_Debug_FlagAsDebugging (void)
{
  SK_RunOnce (bRealDebug = TRUE);
}

constexpr USHORT
_constUnicodeLen ( const wchar_t* str )
{
  if (str == nullptr)
    return 0;

  return (   *str == L'\0'   ) ?
                             0 :
         _constUnicodeLen (str + 1)
                               + 1;
}

static constexpr wchar_t *wszFakeWinTitle = L"Curious Amnesia";
SK_PEB                      __OriginalPEB = { };

// Stage a fake PEB so we do not have to deal with as many
//   anti-debug headaches when using traditional tools that
//     cannot erase evidence of itself :)
void
SK_AntiAntiDebug_CleanupPEB (SK_PEB *pPeb)
{
  auto& _cfg = config;

  __try {
    InterlockedCompareExchangePointer (
      &__SK_GameBaseAddr, pPeb->ImageBaseAddress, nullptr
    );

    if (_cfg.render.framerate.override_num_cpus != -1)
    {
      pPeb->NumberOfProcessors =
        _cfg.render.framerate.override_num_cpus;
    }

    if (__OriginalPEB.NumberOfProcessors == 0)
        __OriginalPEB = *(SK_PPEB)NtCurrentTeb ()->ProcessEnvironmentBlock;

#if 0
    BOOL bWasBeingDebugged =
       pPeb->BeingDebugged;
#endif

    static wchar_t wszWindowTitle [512] = { };
    if (          *wszWindowTitle == L'\0')
    {
      wcsncpy (                         wszWindowTitle,
                  pPeb->ProcessParameters->WindowTitle.Buffer,
                  pPeb->ProcessParameters->WindowTitle.Length );
    }

    if (pPeb->BeingDebugged)
    {
      SK_Debug_FlagAsDebugging ();
    }

    if (__OriginalPEB.NumberOfProcessors == 0)
        __OriginalPEB = *(SK_PPEB)NtCurrentTeb ()->ProcessEnvironmentBlock;

    if (bRealDebug && SK_GetFramesDrawn () <= 2)
    {
      pPeb->BeingDebugged                  = FALSE;
      pPeb->NtGlobalFlag                  &= ~0x70;
      pPeb->ProcessParameters->DebugFlags  =  0x00;

      SK_InitUnicodeString (
        (PUNICODE_STRING_SK)
        &pPeb->ProcessParameters->WindowTitle,
                              wszFakeWinTitle );
    }

    if (bRealDebug) SK_RunOnce (
      SK_Thread_Create ([](LPVOID pUser)->DWORD
      {
        SK_PEB *pPeb =
          static_cast <SK_PEB *>(pUser);

        BOOLEAN BeingDebugged = pPeb->BeingDebugged;
        ULONG   NtGlobalFlag  = pPeb->NtGlobalFlag;
        ULONG   DebugFlags    = pPeb->ProcessParameters->DebugFlags;
        wcsncpy_s ( wszWindowTitle, 511,
                      pPeb->ProcessParameters->WindowTitle.Buffer,
                      pPeb->ProcessParameters->WindowTitle.Length );

        auto pRealPeb = (SK_PPEB)NtCurrentTeb ()->ProcessEnvironmentBlock;

        while (! ReadAcquire (&__SK_DLL_Ending))
        {
          // This Tom Foolery is only needed at initial app start
          if (SK_GetFramesDrawn () > 2)
          {
            RtlAcquirePebLock_Original ();

            if (bRealDebug)
            {
              pRealPeb->BeingDebugged                  =  TRUE;

              SK_InitUnicodeString (
                (PUNICODE_STRING_SK)
                &pRealPeb->ProcessParameters->WindowTitle,
                                           wszWindowTitle );
            }

            RtlReleasePebLock_Original ();

            break;
          }

          else
          {
            RtlAcquirePebLock_Original ();

            pRealPeb->BeingDebugged                  = FALSE;
            pRealPeb->NtGlobalFlag                  &= ~0x70;
            pRealPeb->ProcessParameters->DebugFlags  =  0x00;

            RtlReleasePebLock_Original ();
          }
        }

        pRealPeb->BeingDebugged                  = BeingDebugged;
        pRealPeb->NtGlobalFlag                  &=  NtGlobalFlag;
        pRealPeb->ProcessParameters->DebugFlags  =    DebugFlags;

        SK_InitUnicodeString (
            (PUNICODE_STRING_SK)
            &pRealPeb->ProcessParameters->WindowTitle,
                                       wszWindowTitle );

        SK_Thread_CloseSelf ();

        return 0;
      }, static_cast <LPVOID> (&__OriginalPEB) )
    );

#if 0
    BOOL bWasBeingDebugged =
       pPeb->BeingDebugged;

    if (pPeb->BeingDebugged)
    {
      SK_Debug_FlagAsDebugging ();
      pPeb->BeingDebugged = FALSE;
    }

    pPeb->NtGlobalFlag &= ~0x70;

    static wchar_t wszWindowTitle [512] = { };

    if ((! bWasBeingDebugged) && *wszWindowTitle == L'\0')
    {
      wcsncpy (                         wszWindowTitle,
                  pPeb->ProcessParameters->WindowTitle.Buffer,
                  pPeb->ProcessParameters->WindowTitle.Length );
    }

#ifdef SK_ANTIDEBUG_PARANOIA_STAGE2
    if (pPeb->ProcessParameters->DebugFlags == 0x0 ||
        pPeb->ProcessParameters->DebugFlags == DEBUG_ONLY_THIS_PROCESS)
        pPeb->ProcessParameters->DebugFlags  = DEBUG_PROCESS;
    //pPeb->ProcessParameters->DebugFlags  = 0x0;
#endif /* SK_ANTIDEBUG_PARANOIA_STAGE2 */

#ifdef SK_ANTIDEBUG_PARANOIA_STAGE3
    if (bWasBeingDebugged)
    {
      if ( StrStrW ( pPeb->ProcessParameters->WindowTitle.Buffer,
                                            L"EntrianAttach") )
      {
        wcscpy ( wszWindowTitle,
                   SK_AntiAntiDebug_GetNormalWindowTitle () );
      }

      SK_InitUnicodeString (
        &pPeb->ProcessParameters->WindowTitle,
                               wszWindowTitle );
    }
#endif /* SK_ANTIDEBUG_PARANOIA_STAGE3 */
#endif
  }
  __except (EXCEPTION_EXECUTE_HANDLER) { };
}

bool
SK_Debug_CheckDebugFlagInPEB (void)
{
  if ( 0 !=
         ((SK_PPEB)NtCurrentTeb ()->ProcessEnvironmentBlock)->BeingDebugged )
  {
    SK_Debug_FlagAsDebugging ();
    return true;
  }

  return false;
}

void NTAPI
RtlAcquirePebLock_Detour (void)
{
  if (RtlAcquirePebLock_Original != nullptr)
  {
    RtlAcquirePebLock_Original ();

    SK_AntiAntiDebug_CleanupPEB (
      (SK_PPEB)NtCurrentTeb ()->ProcessEnvironmentBlock
    );
  }
}

void NTAPI
RtlReleasePebLock_Detour (void)
{
  if (RtlReleasePebLock_Original != nullptr)
  {
    // And now, another magic trick... debugger's gone again!
    SK_AntiAntiDebug_CleanupPEB (
      (SK_PPEB)NtCurrentTeb ()->ProcessEnvironmentBlock
    );

    RtlReleasePebLock_Original ();
  }
}

NTSTATUS
NTAPI
ZwSetInformationThread_Detour (
  _In_ HANDLE                      ThreadHandle,
  _In_ SK_THREAD_INFORMATION_CLASS ThreadInformationClass,
  _In_ PVOID                       ThreadInformation,
  _In_ ULONG                       ThreadInformationLength )
{
  SK_LOG_FIRST_CALL

  if ( ThreadInformationClass == ThreadZeroTlsCell ||
       ThreadInformationClass == ThreadSetTlsArrayAddress )
  {
    SK_AutoHandle hDuplicate (INVALID_HANDLE_VALUE);

    if ((LONG_PTR)ThreadHandle == -2)
    {
      if (
        DuplicateHandle ( SK_GetCurrentProcess (),
                          SK_GetCurrentThread  (),
                          SK_GetCurrentProcess (),
                            &hDuplicate.m_h,
                              THREAD_ALL_ACCESS, FALSE,
                          DUPLICATE_SAME_ACCESS )
         )
      {
        ThreadHandle = hDuplicate.m_h;
      }
    }

    if (ThreadHandle == 0)
    {
      return 0;
    }

    DWORD                                 dwExitCode = 0;
    if (GetExitCodeThread (ThreadHandle, &dwExitCode) && dwExitCode == STILL_ACTIVE)
    {
      return
        ZwSetInformationThread_Original ( ThreadHandle,
                                          ThreadInformationClass,
                                          ThreadInformation,
                                          ThreadInformationLength );

    }
  }

  if ( ThreadInformationClass  == ThreadHideFromDebugger )
  {
    SK_AutoHandle hDuplicate (INVALID_HANDLE_VALUE);

    if ((LONG_PTR)ThreadHandle == -2)
    {
      if (
        DuplicateHandle ( SK_GetCurrentProcess (),
                          SK_GetCurrentThread  (),
                          SK_GetCurrentProcess (),
                            &hDuplicate.m_h,
                              THREAD_ALL_ACCESS, FALSE,
                          DUPLICATE_SAME_ACCESS )
         )
      {
        ThreadHandle = hDuplicate.m_h;
      }
    }

    if (ThreadHandle == 0)
    {
      return 0;
    }

    // Make sure not to do this if this call is intended to free TLS.
    SK_TLS *pTLS =
          SK_TLS_Bottom ();

    if ( GetThreadId (ThreadHandle) == SK_Thread_GetCurrentId () ||
         (       pTLS               != nullptr   &&
                 pTLS->debug.handle == ThreadHandle )
       )
    {
      if (pTLS != nullptr) pTLS->debug.hidden = true;

      SK_LOG0 ( ( L"tid=%x (%s) tried to hide itself from debuggers; please "
                  L"attach one and investigate!",
                  GetThreadId (ThreadHandle),
            SK_Thread_GetName (ThreadHandle).c_str () ),
                  L"DieAntiDbg" );
    }

    //
    // Expect that the first thing this thread is going to do is re-write
    //   DbgUiRemoteBreakin and a few other NtDll functions.
    //
    if (config.system.log_level > 5)
    {
      CONTEXT                          ThreadContext = { };
      SuspendThread    (ThreadHandle);
      GetThreadContext (ThreadHandle, &ThreadContext);
    }

    SK::Diagnostics::Debugger::Allow (true);

    RtlAcquirePebLock_Detour ();
    RtlReleasePebLock_Detour ();

    return STATUS_SUCCESS;
  }

  return
    ZwSetInformationThread_Original ( ThreadHandle,
                                      ThreadInformationClass,
                                      ThreadInformation,
                                      ThreadInformationLength );
}



void
SK_Thread_NotifyCreation (HANDLE  ThreadHandle,
                          DWORD   ThreadId,
                          PVOID   StartRoutine,
                          HMODULE hModStart)
{
  // We may use these in the future...
  std::ignore = ThreadHandle;
  std::ignore = StartRoutine;

  if (config.input.gamepad.hook_windows_gaming)
  {
    // Identify WGI thread(s?)
    if (StrStrIW (SK_GetModuleName (hModStart).c_str (), L"Windows.Gaming.Input"))
    {
      SK_RunOnce (
        extern DWORD SK_WGI_GamePollingThreadId;
                     SK_WGI_GamePollingThreadId = ThreadId;
      );
    }
  }
}


NTSTATUS
NTAPI
ZwCreateThreadEx_Detour (
  _Out_    PHANDLE            ThreadHandle,
  _In_     ACCESS_MASK        DesiredAccess,
  _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
  _In_     HANDLE             ProcessHandle,
  _In_     PVOID              StartRoutine,
  _In_opt_ PVOID              Argument,
  _In_     ULONG              CreateFlags,
  _In_     SIZE_T             ZeroBits,
  _In_     SIZE_T             StackSize,
  _In_     SIZE_T             MaximumStackSize,
  _In_opt_ PVOID              AttributeList )
{
  SK_LOG_FIRST_CALL

  char    thread_name [512] = { };
  char    szSymbol    [256] = { };
  ULONG   ulLen             =  0 ;

  HMODULE hModStart =
    SK_GetModuleFromAddr (StartRoutine);

  if ( dbghelp_callers.find (hModStart) ==
       dbghelp_callers.cend (         )  )
  {
#ifdef _M_AMD64
# define SK_DBGHELP_STUB(__proto) __proto##64
#else
# define SK_DBGHELP_STUB(__proto) __proto
#endif
#define SK_DBGHELP_STUB_(__proto) __proto

#define SK_StackWalk          SK_DBGHELP_STUB  (StackWalk)
#define SK_SymLoadModule      SK_DBGHELP_STUB  (SymLoadModule)
#define SK_SymUnloadModule    SK_DBGHELP_STUB  (SymUnloadModule)
#define SK_SymGetModuleBase   SK_DBGHELP_STUB  (SymGetModuleBase)
#define SK_SymGetLineFromAddr SK_DBGHELP_STUB  (SymGetLineFromAddr)
#define SK_SymGetTypeInfo     SK_DBGHELP_STUB_ (SymGetTypeInfo)

    auto szDupName =
      SK_WideCharToUTF8 (
        SK_GetModuleFullNameFromAddr (StartRoutine)
      ) + '\0';

    MODULEINFO mod_info = { };

    GetModuleInformation (
      GetCurrentProcess (), hModStart, &mod_info, sizeof (mod_info)
    );

#ifdef _M_AMD64
    DWORD64 BaseAddr = (DWORD64)mod_info.lpBaseOfDll;
#else /* _M_IX86 */
    DWORD   BaseAddr =   (DWORD)mod_info.lpBaseOfDll;
#endif

    char* pszShortName = szDupName.data ();

    PathStripPathA (pszShortName);

    std::scoped_lock <SK_Thread_HybridSpinlock> auto_lock (*cs_dbghelp);

    if ( dbghelp_callers.find (hModStart) ==
         dbghelp_callers.cend (         )  )
    {
      SK_SymLoadModule ( GetCurrentProcess (),
                         nullptr, pszShortName,
                         nullptr, BaseAddr,
                         mod_info.SizeOfImage );

      dbghelp_callers.insert (hModStart);
    }
  }

  BOOL Suspicious = FALSE;

  if ( CreateFlags &   THREAD_CREATE_FLAGS_HIDE_FROM_DEBUGGER )
  {    CreateFlags &= ~THREAD_CREATE_FLAGS_HIDE_FROM_DEBUGGER;

    if (config.system.log_level > 5)
      CreateFlags |= THREAD_CREATE_FLAGS_CREATE_SUSPENDED;

    Suspicious = TRUE;
  }

  CreateFlags &= ~THREAD_CREATE_FLAGS_SKIP_THREAD_ATTACH;

  BOOL suspended =
    ( CreateFlags  & THREAD_CREATE_FLAGS_CREATE_SUSPENDED );

  CreateFlags |= THREAD_CREATE_FLAGS_CREATE_SUSPENDED;

  NTSTATUS ret =
    ZwCreateThreadEx_Original (
      ThreadHandle,  DesiredAccess, ObjectAttributes,
      ProcessHandle, StartRoutine,  Argument,
      CreateFlags,   ZeroBits,      StackSize,
                             MaximumStackSize, AttributeList );

  if (NT_SUCCESS (ret))
  {
    const DWORD tid =
      GetThreadId (*ThreadHandle);

    auto& ThreadNames =
     *_SK_ThreadNames;

    if ( ThreadNames.find (tid) ==
         ThreadNames.cend (   ) )
    {
      ulLen =
        SK_GetSymbolNameFromModuleAddr (
          hModStart, reinterpret_cast <uintptr_t> ((LPVOID)StartRoutine),
            szSymbol, ulLen );

      if (ulLen > 0)
      {
        sprintf ( thread_name, "%s+%s",
                    SK_WideCharToUTF8 (
                      SK_GetCallerName (StartRoutine)
                    ).c_str (), szSymbol
        );
      }

      else
      {
        sprintf ( thread_name, "%s",
                    SK_WideCharToUTF8 (
                      SK_GetCallerName (StartRoutine)
                    ).c_str ()
        );
      }

      std::wstring thr_name (
        SK_UTF8ToWideChar   (
        thread_name       )
      );

      ThreadNames.insert (
        std::make_pair ( tid, thr_name )
      );

      SK_TLS* pTLS =
        SK_TLS_BottomEx (tid);

      if (pTLS != nullptr)
      {
        wcsncpy_s (
          pTLS->debug.name,          MAX_THREAD_NAME_LEN,
                  thr_name.c_str (), _TRUNCATE
        );
      }
    }

    if (Suspicious)
    {
      if (SK_IsDebuggerPresent ())
      {
        __debugbreak ();

        SK_LOG0 ( (L">>tid=%x", GetThreadId (*ThreadHandle) ),
                   L"DieAntiDbg" );
      }
    }

    if (! suspended)
    {
      ResumeThread (*ThreadHandle);
    }

    SK_Thread_NotifyCreation (*ThreadHandle, tid, StartRoutine, hModStart);
  }

  return ret;
}

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

  char    thread_name [512] = { };
  char    szSymbol    [256] = { };
  ULONG   ulLen             =  0 ;

  HMODULE hModStart =
    SK_GetModuleFromAddr (StartRoutine);

  if ( dbghelp_callers.find (hModStart) ==
       dbghelp_callers.cend (         )  )
  {
#ifdef _M_AMD64
# define SK_DBGHELP_STUB(__proto) __proto##64
#else
# define SK_DBGHELP_STUB(__proto) __proto
#endif
#define SK_DBGHELP_STUB_(__proto) __proto

#define SK_StackWalk          SK_DBGHELP_STUB  (StackWalk)
#define SK_SymLoadModule      SK_DBGHELP_STUB  (SymLoadModule)
#define SK_SymUnloadModule    SK_DBGHELP_STUB  (SymUnloadModule)
#define SK_SymGetModuleBase   SK_DBGHELP_STUB  (SymGetModuleBase)
#define SK_SymGetLineFromAddr SK_DBGHELP_STUB  (SymGetLineFromAddr)
#define SK_SymGetTypeInfo     SK_DBGHELP_STUB_ (SymGetTypeInfo)

    auto szDupName =
      SK_WideCharToUTF8 (
        SK_GetModuleFullNameFromAddr (StartRoutine)
      ) + '\0';

    MODULEINFO mod_info = { };

    GetModuleInformation (
      GetCurrentProcess (), hModStart, &mod_info, sizeof (mod_info)
    );

#ifdef _M_AMD64
    DWORD64 BaseAddr = (DWORD64)mod_info.lpBaseOfDll;
#else /* _M_IX86 */
    DWORD   BaseAddr =   (DWORD)mod_info.lpBaseOfDll;
#endif

    char* pszShortName = szDupName.data ();

    PathStripPathA (pszShortName);

    std::scoped_lock <SK_Thread_HybridSpinlock> auto_lock (*cs_dbghelp);

    if ( dbghelp_callers.find (hModStart) ==
         dbghelp_callers.cend (         )  )
    {
      SK_SymLoadModule ( GetCurrentProcess (),
                         nullptr, pszShortName,
                         nullptr, BaseAddr,
                         mod_info.SizeOfImage );

      dbghelp_callers.insert (hModStart);
    }
  }

  BOOL Suspicious = FALSE;

  if ( CreateFlags &   THREAD_CREATE_FLAGS_HIDE_FROM_DEBUGGER )
  {    CreateFlags &= ~THREAD_CREATE_FLAGS_HIDE_FROM_DEBUGGER;

    if (config.system.log_level > 5)
      CreateFlags |= THREAD_CREATE_FLAGS_CREATE_SUSPENDED;

    Suspicious = TRUE;
  }

  CreateFlags &= ~THREAD_CREATE_FLAGS_SKIP_THREAD_ATTACH;

  BOOL suspended =
    ( CreateFlags  & THREAD_CREATE_FLAGS_CREATE_SUSPENDED );

  CreateFlags |= THREAD_CREATE_FLAGS_CREATE_SUSPENDED;

  NTSTATUS ret =
    NtCreateThreadEx_Original (
      ThreadHandle,  DesiredAccess, ObjectAttributes,
      ProcessHandle, StartRoutine,  Argument,
      CreateFlags,   ZeroBits,      StackSize,
                             MaximumStackSize, AttributeList );

  if (NT_SUCCESS (ret))
  {
    const DWORD tid =
      GetThreadId (*ThreadHandle);

    auto& ThreadNames =
     *_SK_ThreadNames;

    if ( ThreadNames.find (tid) ==
         ThreadNames.cend (   ) )
    {
      ulLen =
        SK_GetSymbolNameFromModuleAddr (
          hModStart, reinterpret_cast <uintptr_t> ((LPVOID)StartRoutine),
            szSymbol, ulLen
        );

      if (ulLen > 0)
      {
        sprintf ( thread_name, "%s+%s",
                    SK_WideCharToUTF8 (
                      SK_GetCallerName (StartRoutine)
                    ).c_str (), szSymbol
        );
      }

      else
      {
        sprintf ( thread_name, "%s",
                    SK_WideCharToUTF8 (
                      SK_GetCallerName (StartRoutine)
                    ).c_str ()
        );
      }

      std::wstring thr_name (
        SK_UTF8ToWideChar   (
        thread_name       )
      );

      ThreadNames.insert (
        std::make_pair ( tid, thr_name )
      );

      SK_TLS* pTLS =
        SK_TLS_BottomEx (tid);

      if (pTLS != nullptr)
      {
        wcsncpy_s (
          pTLS->debug.name,          MAX_THREAD_NAME_LEN,
                  thr_name.c_str (), _TRUNCATE
        );
      }
    }

    if (Suspicious)
    {
      if (SK_IsDebuggerPresent ())
      {
        __debugbreak ();

        SK_LOG0 ( (L">>tid=%x", GetThreadId (*ThreadHandle) ),
                   L"DieAntiDbg" );
      }
    }

    if (! suspended)
    {
      ResumeThread (*ThreadHandle);
    }
  }

  return ret;
}


bool spoof_debugger = true;

BOOL
WINAPI
SetThreadContext_Detour ( HANDLE hThread, const CONTEXT* pCtx )
{
//#define LETS_PLAY_DENUVO_GAMES
#ifdef  LETS_PLAY_DENUVO_GAMES
  if (SK_GetCurrentGameID () == SK_GAME_ID::ResidentEvil8)
  {
    //
    // It is extremely unusual for the host application to manipulate
    //   Debug Registers.
    //
    //  Debuggers do this, but they do not execute code from inside
    //    the application.
    //
    //  In all known cases of non-zero DRs, it is an anti-tamper
    //    software layer trying to uninstall breakpoints.
    //
    if ( pCtx->Dr0 != 0x0 || pCtx->Dr1 != 0x0 ||
         pCtx->Dr2 != 0x0 || pCtx->Dr3 != 0x0 ||
       //pCtx->Dr4 != 0x0 || pCtx->Dr5 != 0x0 ||
         pCtx->Dr6 != 0x0 || pCtx->Dr7 != 0x0 )
    {
      CONTEXT _CtxSanitized =
        *pCtx;

      _CtxSanitized.Dr0 = 0x0; _CtxSanitized.Dr1 = 0x0;
      _CtxSanitized.Dr2 = 0x0; _CtxSanitized.Dr3 = 0x0;
      _CtxSanitized.Dr6 = 0x0; _CtxSanitized.Dr7 = 0x0;

      static
        concurrency::concurrent_unordered_set <HANDLE> logged_tids;

      BOOL bRet =
        SetThreadContext_Original ( hThread, &_CtxSanitized );

      if (! logged_tids.count  (hThread))
      {     logged_tids.insert (hThread);

        SK_LOG0 ( ( L"Anti-Debug Breakpoint Removal Detected ('%s' on '%s')",
                      SK_Thread_GetName (                  hThread).c_str (),
                      SK_Thread_GetName (SK_Thread_GetCurrentId ()).c_str () ),
                    L"AntiTamper" );

        if (bRet)
        {
          // Add an extra value to the semaphore to prevent the next
          //   attempt to do this from incurring performance overhead.
          ResumeThread (hThread);
        }
      }

      return bRet;
    }
  }
#endif

  return
    SetThreadContext_Original ( hThread, pCtx );
}

using CheckRemoteDebuggerPresent_pfn = BOOL (WINAPI *)( HANDLE hProcess,
                                                        PBOOL  pbDebuggerPresent );
      CheckRemoteDebuggerPresent_pfn
      CheckRemoteDebuggerPresent_Original = nullptr;

BOOL
WINAPI
CheckRemoteDebuggerPresent_Detour ( HANDLE hProcess,
                                    PBOOL  pbDebuggerPresent )
{
  SK_LOG_FIRST_CALL

  if (            hProcess  != INVALID_HANDLE_VALUE      &&
                ( hProcess  == SK_GetCurrentProcess () ) ||
    GetProcessId (hProcess) ==
    GetProcessId (             SK_GetCurrentProcess () ) )
  {
    if (pbDebuggerPresent != nullptr)
      *pbDebuggerPresent = FALSE;

    return TRUE;
  }

  return
    CheckRemoteDebuggerPresent_Original (
      hProcess, pbDebuggerPresent
    );
}

using IsDebuggerPresent_pfn = BOOL (WINAPI *)(void);
      IsDebuggerPresent_pfn
      IsDebuggerPresent_Original = nullptr;

BOOL
WINAPI
IsDebuggerPresent_Detour (void)
{
  // Most anti-tamper does not use the proper API and this log
  //   serves little purpose.
#ifdef _DEBUG
  DWORD dwTid =
    SK_Thread_GetCurrentId ();

  static concurrency::concurrent_unordered_set <DWORD> logged_tids;

  if ( logged_tids.find (dwTid) ==
       logged_tids.cend (     )  )
  {
    logged_tids.insert (dwTid);

    SK_LOG0 ( ( L"Thread: %x (%s) is inquiring for debuggers",
                dwTid, SK_Thread_GetName (dwTid).c_str () ),
                L"AntiTamper" );
  }
#endif


  // Community Service Time
  if (SK_GetCurrentGameID () == SK_GAME_ID::FinalFantasyXV)
  {
    static bool killed_ffxv = false;

    if ( (! killed_ffxv) &&
            SK_Thread_GetCurrentPriority () == THREAD_PRIORITY_LOWEST )
    {
      SK_LOG0 ( ( L"Anti-Debug Detected (tid=%x)",
               SK_Thread_GetCurrentId () ),
               L"AntiAntiDbg");

      killed_ffxv = true;
      _endthreadex ( 0x0 );
    }
  }

  else if (SK_GetCurrentGameID () == SK_GAME_ID::FinalFantasyXVI)
  {
    return FALSE;
  }

//#ifdef _DEBUG
//  return TRUE;
//#endif

  if (SK_GetFramesDrawn () > 0 && RtlAcquirePebLock_Original != nullptr)
  {
    RtlAcquirePebLock_Original ();
  
    // Low-level construct that IsDebuggerPresent actually looks at,
    //   we want this to be accurate but want to misreport lookups to
    //     the calling application in order to bypass most anti-debug.
    ((SK_PPEB)NtCurrentTeb ()->ProcessEnvironmentBlock)->BeingDebugged =
      bRealDebug;
  
    RtlReleasePebLock_Original ();
  }

  if (config.compatibility.impersonate_debugger)
    return TRUE;

  if (spoof_debugger)
    return FALSE;

  return
    IsDebuggerPresent_Original ();
}

using DbgBreakPoint_pfn = void (WINAPI *)(void);
      DbgBreakPoint_pfn
      DbgBreakPoint_Original = nullptr;

using DebugBreak_pfn = void (WINAPI *)(void);
      DebugBreak_pfn
      DebugBreak_Original = nullptr;

__declspec (noinline)
void
WINAPI
DebugBreak_Detour (void)
{
  // Hooked for the calling process, when a debugger attaches we will
  //   manually fix this up so that it can break execution.
}

__declspec (noinline)
void
WINAPI
DbgBreakPoint_Detour (void)
{
  if (SK_IsDebuggerPresent ())
  {
    __try
    {
      if (DbgBreakPoint_Original != nullptr)
          DbgBreakPoint_Original ();

      else
        __debugbreak ();
    }

    __finally
    {
    }
  }
}


using SetProcessValidCallTargets_pfn = BOOL (WINAPI *)(
    _In_ HANDLE hProcess,
    _In_ PVOID  VirtualAddress,
    _In_ SIZE_T RegionSize,
    _In_ ULONG  NumberOfOffsets,
    _Inout_updates_(NumberOfOffsets) PCFG_CALL_TARGET_INFO OffsetInformation);
      SetProcessValidCallTargets_pfn
      SetProcessValidCallTargets_Original = nullptr;

BOOL
WINAPI
SetProcessValidCallTargets_Detour
( _In_ HANDLE hProcess,
  _In_ PVOID VirtualAddress,
  _In_ SIZE_T RegionSize,
  _In_ ULONG NumberOfOffsets,
  _Inout_updates_(NumberOfOffsets) PCFG_CALL_TARGET_INFO OffsetInformation )
{
  SK_LOG_FIRST_CALL

  CFG_CALL_TARGET_INFO *fakeTargets =
    new CFG_CALL_TARGET_INFO [NumberOfOffsets];

  for ( UINT i = 0; i < NumberOfOffsets ; ++i )
  {
    fakeTargets [i] = OffsetInformation [i];

    // If Flags is 0x0, then something is trying to dynamically rakajigor CFG
    //   and it's been my experience that CFG has no legitimate use.
    fakeTargets [i].Flags = CFG_CALL_TARGET_VALID;

    // So ... we're making that target valid and that's that.
  }

  BOOL bRet =
    SetProcessValidCallTargets_Original (
      hProcess, VirtualAddress, RegionSize,
        NumberOfOffsets, fakeTargets );

  delete [] fakeTargets;

  return bRet;
}




using DbgUiRemoteBreakin_pfn = VOID (NTAPI*)(_In_ PVOID Context);
      DbgUiRemoteBreakin_pfn
      DbgUiRemoteBreakin_Original = nullptr;

__declspec (noinline)
VOID
NTAPI
DbgUiRemoteBreakin_Detour (PVOID Context)
{
  // If we have this hooked, it's for some serious anti-debug purposes,
  //   so go ahead and start faking Debugger-Not-Attached state.
  UNREFERENCED_PARAMETER (Context);

  auto pPeb =
    (SK_PPEB)NtCurrentTeb ()->ProcessEnvironmentBlock;

  BOOL bDebugging = std::exchange (pPeb->BeingDebugged, (BOOLEAN)FALSE);
                                   pPeb->NtGlobalFlag      &= ~0x70;
                                   pPeb->CrossProcessFlags &= ~0x4;

#if 1
  if (bDebugging)
  {
    __try
    {
      DbgBreakPoint_Detour ();
    }

    __except (EXCEPTION_CONTINUE_EXECUTION)
    {
    }
  }

#else
  if (! SK_GetFramesDrawn ())
    DbgBreakPoint_Detour  ();

  while (true)//RtlExitUserProcess_Original == nullptr)
    SK_Sleep (125);
#endif

  // Anti-debug measure in Starfield uses a TLS callback that
  //   will raise STATUS_USER_CALLBACK when this thread exits...
  RtlExitUserThread_Original (STATUS_SUCCESS);
}

RaiseException_pfn
RaiseException_Original = nullptr;

constexpr
  static DWORD
    MAGIC_THREAD_EXCEPTION = 0x406D1388;

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
        _bstr_t bstrSource      ( com_err.ErrorInfo   () != nullptr ?
                                  com_err.Source      ()            : _bstr_t (L"Unknown") );
        _bstr_t bstrDescription ( com_err.ErrorInfo   () != nullptr ?
                                  com_err.Description ()            : _bstr_t (L"Unknown") );

        SK_LOG0 ( ( L" >> Code: %08lx  <%s> - [Source: %s,  Desc: \"%s\"]",
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

struct SK_FFXV_Thread
{
  ~SK_FFXV_Thread (void) {
    if (hThread)
      SK_CloseHandle (hThread);
  }

           HANDLE   hThread  = nullptr;
  volatile LONG     dwPrio   = THREAD_PRIORITY_NORMAL;
  sk::ParameterInt* prio_cfg = nullptr;

  void setup (HANDLE __hThread);
};

extern SK_LazyGlobal <SK_FFXV_Thread> sk_ffxv_swapchain,
                                      sk_ffxv_vsync,
                                      sk_ffxv_async_run;

bool
SK_Exception_HandleThreadName (
  DWORD      dwExceptionCode,
  DWORD      /*dwExceptionFlags*/,
  DWORD      /*nNumberOfArguments*/,
  const ULONG_PTR *lpArguments         )
{
  if (dwExceptionCode == MAGIC_THREAD_EXCEPTION && lpArguments != nullptr)
  {
    THREADNAME_INFO* info =
      (THREADNAME_INFO *)lpArguments;

    size_t len = 0;

    const bool non_empty =
      info->szName  != nullptr &&
      SUCCEEDED (
        StringCbLengthA (
          info->szName,
            255, &len   )
      )                        &&
               len   > 0       &&
      info->dwFlags == 0       &&
      info->dwType  == 4096;

    if (non_empty)
    {
      static auto& ThreadNames = _SK_ThreadNames.get       ();
      static auto& SelfTitled  = _SK_SelfTitledThreads.get ();

      DWORD dwTid  =  ( info->dwThreadID != -1 ?
                        info->dwThreadID :
                        SK_Thread_GetCurrentId () );

      SelfTitled.insert (dwTid);

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
          std::min (len+1, (size_t)MAX_THREAD_NAME_LEN-1),
          wide_name.c_str (),
          _TRUNCATE );
      }

      ThreadNames [dwTid] =
        wide_name;

#ifdef _M_AMD64
      if (SK_GetCurrentGameID () == SK_GAME_ID::EldenRing)
      {
        if (! config.system.game_output)
        {
          extern size_t SK_CPU_CountLogicalCores (void);

          static auto cores =
            SK_CPU_CountLogicalCores ();

          SK_AutoHandle hThread (
            OpenThread ( THREAD_ALL_ACCESS, FALSE, dwTid )
          );

          if (StrStrIA (info->szName, "EzWorkPool_LOW_Clus"))
          {
            static auto constexpr _PoolSize = 4;

            int j = 0;

            std::sscanf (info->szName, "EzWorkPool_LOW_Clus%d", &j);

            int mask = 0x0;

            for ( int i = 0 ; i < cores / _PoolSize ; ++i )
              mask |= (1 << i);

            mask <<= (j * (cores / _PoolSize));

            SetThreadAffinityMask (hThread, mask);

            dll_log->Log (L"Clus%d %08x", j, mask);
          }

          if (StrStrIA (info->szName, "EzWorkPool_HIGHEST"))
          {
            static auto constexpr _PoolSize = 8;

            static int highest = 0;

            int mask = 0x0;

            for ( int i = 0 ; i < cores / _PoolSize ; ++i )
              mask |= (1 << i);

            mask <<= (highest++ * (cores / _PoolSize));

            SetThreadAffinityMask (hThread, mask);

            dll_log->Log (L"Highest%d %08x", highest-1, mask);
          }

          if (StrStrIA (info->szName, "LoadProcess"))
          {
            static auto constexpr _PoolSize = 6;

            static int highest = 0;

            int mask = 0x0;

            for ( int i = 0 ; i < cores / _PoolSize ; ++i )
              mask |= (1 << i);

            mask <<= (highest++ * (cores / _PoolSize));

            SetThreadAffinityMask (hThread, mask);

            dll_log->Log (L"LoadProcess%d %08x", highest-1, mask);
          }

          if (StrStrIA (info->szName, "EzWorkPool_LOW_AllC"))
          {
            static auto constexpr _PoolSize = 2;

            static int highest = 0;

            int mask = 0x0;

            for ( int i = 0 ; i < cores / _PoolSize ; ++i )
              mask |= (1 << i);

            mask <<= (highest++ * (cores / _PoolSize));

            SetThreadAffinityMask (hThread, mask);

            dll_log->Log (L"AllC%d %08x", highest-1, mask);
          }
        }
      }

      if (SK_GetCurrentGameID () == SK_GAME_ID::FinalFantasyXV)
      {
        SK_AutoHandle hThread (
          OpenThread ( THREAD_ALL_ACCESS, FALSE, dwTid )
        );

        if ((! sk_ffxv_vsync->hThread) && StrStrIA (info->szName, "VSync"))
        {
          sk_ffxv_vsync->setup (hThread.m_h);
        }

        else if ( (! sk_ffxv_async_run->hThread) &&
                     StrStrIA (info->szName, "AsyncFile.Run") )
        {
          sk_ffxv_async_run->setup (hThread.m_h);
        }
      }

      else if (SK_GetCurrentGameID () == SK_GAME_ID::AssassinsCreed_Odyssey)
      {
#define SK_DisableThreadPriorityBoost(thread) \
          SetThreadPriorityBoost ((thread), TRUE)

        SK_AutoHandle hThread (
          OpenThread ( THREAD_ALL_ACCESS,
                       FALSE,
                       dwTid
                     )        );

        if (! strcmp (info->szName, "AsyncFileCompletionThread"))
        {
          SetThreadPriority (hThread.m_h, THREAD_PRIORITY_HIGHEST);
          SK_DisableThreadPriorityBoost (hThread.m_h);

          SK_MMCS_TaskEntry* task_me =
            ( config.render.framerate.enable_mmcss ?
                SK_MMCS_GetTaskForThreadIDEx ( dwTid,
                  info->szName, "Games",
                                "DisplayPostProcessing" ) : nullptr );

          if (task_me != nullptr)
          {
            task_me->setPriority (AVRT_PRIORITY_HIGH);
          }
        }

        else
          if (! strcmp (info->szName, "Loading Thread"))
          {
            SetThreadPriority (hThread.m_h, THREAD_PRIORITY_ABOVE_NORMAL);

            SK_MMCS_TaskEntry* task_me =
              ( config.render.framerate.enable_mmcss ?
                  SK_MMCS_GetTaskForThreadIDEx ( dwTid,
                    info->szName, "Games",
                                  "DisplayPostProcessing" ) : nullptr );

            if (task_me != nullptr)
            {
              task_me->setPriority (AVRT_PRIORITY_CRITICAL);
            }
          }

          else
            if (! strcmp (info->szName, "EngineWindowThread"))
            {
              SK_MMCS_TaskEntry* task_me =
                ( config.render.framerate.enable_mmcss ?
                    SK_MMCS_GetTaskForThreadIDEx ( dwTid,
                      info->szName, "Playback",
                                    "Window Manager" ) : nullptr );

              if (task_me != nullptr)
              {
                task_me->setPriority (AVRT_PRIORITY_VERYLOW);
              }
            }

            else
              if (! strcmp (info->szName, "PCTextureMipsUpdateThread"))
              {
                SK_DisableThreadPriorityBoost (hThread);

                SK_MMCS_TaskEntry* task_me =
                  ( config.render.framerate.enable_mmcss ?
                      SK_MMCS_GetTaskForThreadIDEx ( dwTid,
                        info->szName, "Playback",
                                      "DisplayPostProcessing" ) :nullptr );

                if (task_me != nullptr)
                {
                  task_me->setPriority (AVRT_PRIORITY_CRITICAL);
                }
              }

              else
                if (      strstr (info->szName, "TaskThread") == info->szName &&
                    (0 != strcmp (info->szName, "TaskThread0")))
                {
                  SetThreadPriority (hThread, THREAD_PRIORITY_ABOVE_NORMAL);

                  SK_MMCS_TaskEntry* task_me =
                   ( config.render.framerate.enable_mmcss ?
                      SK_MMCS_GetTaskForThreadIDEx ( dwTid,
                       info->szName, "Games",
                                     "DisplayPostProcessing" ) : nullptr );

                  if (task_me != nullptr)
                  {
                    task_me->setPriority (AVRT_PRIORITY_HIGH);
                  }
                }
      }

      else if (SK_GetCurrentGameID () == SK_GAME_ID::Tales_of_Vesperia)
      {
        HANDLE hThread =
          OpenThread ( THREAD_ALL_ACCESS,
                         FALSE,
                           dwTid );

        if ((intptr_t)hThread > 0)
        {
          //extern SK_MMCS_TaskEntry*
          //  SK_MMCS_GetTaskForThreadIDEx (       DWORD  dwTid,
          //                                 const char  *name,
          //                                 const char  *task0,
          //                                 const char  *task1 );

          if (StrStrA (info->szName, "RenderWorkerThread") != nullptr)
          {
            static volatile LONG count = 0;

            auto* task =
              SK_MMCS_GetTaskForThreadIDEx ( dwTid,
                SK_FormatString ("Render Thread #%li", InterlockedIncrement (&count)).c_str (),
                  "Games",
                  "DisplayPostProcessing"
              );

            if (task != nullptr)
            {
              task->queuePriority (AVRT_PRIORITY_HIGH);
            }
          }

          else if (StrStrA (info->szName, "WorkThread"))
          {
            auto* task =
              SK_MMCS_GetTaskForThreadIDEx ( dwTid,
                "Work Thread", "Games",
                               "DisplayPostProcessing"
              );

            if (task != nullptr)
            {
              task->queuePriority (AVRT_PRIORITY_CRITICAL);
            }
          }

          else if (StrStrA (info->szName, "BusyThread"))
          {
            auto* task =
              SK_MMCS_GetTaskForThreadIDEx ( dwTid,
                "Busy Thread", "Games",
                               "Playback"
              );

            if (task != nullptr)
            {
              task->queuePriority (AVRT_PRIORITY_NORMAL);
            }
          }
          SK_CloseHandle (hThread);
        }
      }

      else if (SK_GetCurrentGameID () == SK_GAME_ID::MonsterHunterWorld)
      {
        HANDLE hThread =
          OpenThread ( THREAD_ALL_ACCESS,
                         FALSE,
                           dwTid );

        if (hThread != nullptr)
        {
          bool killed = false;

          if (StrStrA (info->szName, "Intercept") != nullptr)
          {
            SK_MMCS_TaskEntry* task_me =
              ( config.render.framerate.enable_mmcss ?
                  SK_MMCS_GetTaskForThreadID (dwTid, info->szName) :
                  nullptr );

            if (! config.render.framerate.enable_mmcss)
            {
              SetThreadPriority      (hThread, THREAD_PRIORITY_HIGHEST);
              SetThreadPriorityBoost (hThread, FALSE);
            }

            else if (task_me != nullptr)
            {
              SK_AvSetMmThreadPriority ( task_me->hTask,
                                           AVRT_PRIORITY_HIGH );
            }
          }

          else if (StrStrA (info->szName, "Loader") != nullptr)
          {
            SK_MMCS_TaskEntry* task_me =
              ( config.render.framerate.enable_mmcss ?
                  SK_MMCS_GetTaskForThreadID (dwTid, info->szName) :
                  nullptr );

            if (! config.render.framerate.enable_mmcss)
            {
              SetThreadPriority      (hThread, THREAD_PRIORITY_ABOVE_NORMAL);
              SetThreadPriorityBoost (hThread, FALSE);
            }

            else if (task_me != nullptr)
            {
              SK_AvSetMmThreadPriority ( task_me->hTask,
                                           AVRT_PRIORITY_NORMAL );
            }
          }

          else if (StrStrA (info->szName, "Rendering Thread") != nullptr)
          {
            SK_MMCS_TaskEntry* task_me =
              ( config.render.framerate.enable_mmcss ?
                  SK_MMCS_GetTaskForThreadID (dwTid, info->szName) :
                  nullptr );

            if (config.render.framerate.enable_mmcss)
            {
              SetThreadPriority      (hThread, THREAD_PRIORITY_ABOVE_NORMAL);
              SetThreadPriorityBoost (hThread, FALSE);
            }

            else if (task_me != nullptr)
            {
              SK_AvSetMmThreadPriority ( task_me->hTask,
                                           AVRT_PRIORITY_HIGH );
            }
          }

          else if (StrStrA (info->szName, "Job Thread") != nullptr)
          {
            int idx = 0;

            SYSTEM_INFO        si = {};
            SK_GetSystemInfo (&si);

            if (StrStrA (info->szName, "Job Thread") >= info->szName)
            {
              sscanf (
                StrStrA (info->szName, "Job Thread"),
                  "Job Thread - %i", &idx
              );

              // Other job threads count from 0
              if (info->szName != StrStrA (info->szName, "Job Thread"))
              {
                ++idx;
              }
            }

            extern bool __SK_MHW_JobParity;
            extern bool __SK_MHW_JobParityPhysical;

            if (__SK_MHW_JobParity)
            {
              extern size_t
                SK_CPU_CountPhysicalCores (void);

              size_t max_procs = si.dwNumberOfProcessors;

              if (__SK_MHW_JobParityPhysical == true)
              {
                max_procs =
                  SK_CPU_CountPhysicalCores ();
              }

              if ((size_t)idx > max_procs)
              {
                SK_LOG0 ( ( L" * Killed excess job thread (tid=%x), job=%lu",
                            GetThreadId (hThread), idx ),
                            L"Capcom CPU" );

                SK_Sleep (15UL);

                killed = true;
                SK_TerminateThread (hThread, 0x0);
              }
            }

            if (! killed)
            {
              SK_MMCS_TaskEntry* task_me =
                ( config.render.framerate.enable_mmcss ?
                    SK_MMCS_GetTaskForThreadID (GetThreadId (hThread), info->szName) :
                    nullptr );

              if (task_me != nullptr)
              {
                SK_AvSetMmThreadPriority ( task_me->hTask,
                                             AVRT_PRIORITY_NORMAL );
              }
            }
          }

          if (! killed)
            SK_CloseHandle (hThread);

          else
          {
            extern void SK_MHW_PlugIn_Shutdown ();
                        SK_MHW_PlugIn_Shutdown ();

            SK_CloseHandle (hThread);
          }
        }
      }
#endif
    }

    return true;
  }

  return false;
}


// SEH compatible, but not 100% thread-safe (uses Fiber-Local Storage)
bool
SK_SEH_CompatibleCallerName (LPCVOID lpAddr, wchar_t *wszDllFullName)
{
  HMODULE hModOut = nullptr;

  if ( GetModuleHandleEx ( GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT |
                           GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                             static_cast <LPCWSTR> (lpAddr),
                               &hModOut
                         )
     )
  {
    if ( 0 != GetModuleFileName ( hModOut,
                                    wszDllFullName, MAX_PATH ) )
    {
      return true;
    }
  }

  wcsncpy_s (
    wszDllFullName,                MAX_PATH,
    L"#Extremely#Invalid.dll#", _TRUNCATE );

  return false;
}

void
WINAPI
RaiseException_Trap (
  DWORD      dwExceptionCode,
  DWORD      dwExceptionFlags,
  DWORD      nNumberOfArguments,
  const ULONG_PTR *lpArguments         )
{
  if ( SK_Exception_HandleThreadName ( dwExceptionCode, dwExceptionFlags,
                                         nNumberOfArguments, lpArguments )
     ) return;

  SK_TLS* pTlsThis =
    SK_TLS_Bottom ();

  if (pTlsThis != nullptr)
    InterlockedIncrement (&pTlsThis->debug.exceptions);
}


using      RtlRaiseException_pfn =
  VOID (WINAPI *)(PEXCEPTION_RECORD ExceptionRecord);
extern "C" RtlRaiseException_pfn
           RtlRaiseException_Original = nullptr;


// Detoured to catch non-std OutputDebugString implementations
[[noreturn]]
VOID
WINAPI
RtlRaiseException_Detour ( PEXCEPTION_RECORD ExceptionRecord )
{
  bool bSkip = false;

  __try
  {
    switch (ExceptionRecord->ExceptionCode)
    {
      case DBG_PRINTEXCEPTION_C:
      case DBG_PRINTEXCEPTION_WIDE_C:
      {
        wchar_t wszModule [MAX_PATH + 2] = { };

        GetModuleFileName ( SK_GetModuleFromAddr (ExceptionRecord->ExceptionAddress),
                              wszModule,
                                MAX_PATH );

        // Non-std. is anything not coming from kernelbase or kernel32 ;)
        if (! StrStrIW (wszModule, L"kernel"))
        {
          //SK_ReleaseAssert (ExceptionRecord->NumberParameters == 2)

          // ANSI (almost always)
          if (ExceptionRecord->ExceptionCode == DBG_PRINTEXCEPTION_C)
          {
            game_debug->LogEx ( true, L"%-72ws:  %.*hs",
              wszModule, sk::narrow_cast <UINT> (ExceptionRecord->ExceptionInformation [0]),
                      reinterpret_cast <char *> (ExceptionRecord->ExceptionInformation [1]));
          }

          // UTF-16 (rarely ever seen)
          else
          {
            game_debug->LogEx ( true, L"%-72ws:  %.*ws",
              wszModule, sk::narrow_cast <UINT> (ExceptionRecord->ExceptionInformation [0]),
                   reinterpret_cast <wchar_t *> (ExceptionRecord->ExceptionInformation [1]));

            //if (((wchar_t *)ExceptionRecord->ExceptionInformation [1]))
            //               (ExceptionRecord->ExceptionInformation [0]) != L'\n')
              //game_debug.LogEx (false, L"\n");
          }
        }
      } break;
    }
  }
  __except (EXCEPTION_EXECUTE_HANDLER)
  {

  }

  // TODO: Add config setting for interactive debug
  if (config.system.log_level > 1 && SK_IsDebuggerPresent ())
  {
    __debugbreak ();
  }

  __try
  {
    __try
    {
      if (! bSkip)
        RtlRaiseException_Original (ExceptionRecord);
    }

    __finally
    {
      ///////SK::Diagnostics::CrashHandler::Reinstall ();
    }
  }

  __finally
  { };
}


struct SK_SEH_CallRecord
{
  // Number of times before we stop logging the same exception at log level 0
  static constexpr auto LogSilenceThreshold = 3;

  volatile ULONG count = 0;
};

// Counter for the number of times a memory address has raised an exception
static SK_LazyGlobal <
  concurrency::concurrent_unordered_map <LPCVOID, SK_SEH_CallRecord>
> SK_SEH_ReportedCallSites;


//// Detoured so we can get thread names
//[[noreturn]]
void
WINAPI
RaiseException_Detour (
        DWORD      dwExceptionCode,
        DWORD      dwExceptionFlags,
        DWORD      nNumberOfArguments,
  const ULONG_PTR *lpArguments         )
{
  bool skip = false;

  __try
  {
    switch (dwExceptionCode)
    {
#if 0
      case 0x00000087A:
      case 0x00000087B:
      case 0x00000087C:
      case 0x00000087D:
        if (SK_IsDebuggerPresent ()) // DirectX Debug Layer
          RaiseException_Original (dwExceptionCode, dwExceptionFlags, nNumberOfArguments, lpArguments);

        else
        {
          //SK_ReleaseAssert (ExceptionRecord->NumberParameters >= 3);

          SK_LOG0 ( ( L"Debug Layer Text: %hs", (char *)lpArguments [2] ),
                  L" D3DDebug " );

          skip = true;
        }
        break;
#endif

      case DBG_PRINTEXCEPTION_C:
      case DBG_PRINTEXCEPTION_WIDE_C:
      {
        wchar_t               wszModule [MAX_PATH + 2] = { };

        GetModuleFileName ( SK_GetModuleFromAddr (_ReturnAddress ()),
                              wszModule,
                                MAX_PATH );

        // Non-std. is anything not coming from kernelbase or kernel32 ;)
        if (! StrStrIW (wszModule, L"kernel"))
        {
          //SK_ReleaseAssert (ExceptionRecord->NumberParameters == 2)

          // ANSI (almost always)
          if (dwExceptionCode == DBG_PRINTEXCEPTION_C && lpArguments != nullptr && nNumberOfArguments >= 2)
          {
            game_debug->LogEx ( true, L"%-72ws:  %.*hs",
              wszModule, sk::narrow_cast <UINT> (lpArguments [0]),
                      reinterpret_cast <char *> (lpArguments [1]));
          }

          // UTF-16 (rarely ever seen)
          else if (lpArguments != nullptr && nNumberOfArguments >= 2)
          {
            game_debug->LogEx ( true, L"%-72ws:  %.*ws",
              wszModule, sk::narrow_cast <UINT> (lpArguments [0]),
                   reinterpret_cast <wchar_t *> (lpArguments [1]));

            //if (((wchar_t *)ExceptionRecord->ExceptionInformation [1]))
            //               (ExceptionRecord->ExceptionInformation [0]) != L'\n')
              //game_debug.LogEx (false, L"\n");
          }
        }

        skip = true;
      } break;
    }
  }
  __except (EXCEPTION_EXECUTE_HANDLER)
  {

  }



  if (dwExceptionCode == 0x1)
  {
    skip = true;
  }

  else
  {
    SK_TLS* pTlsThis =
      SK_TLS_Bottom ();


    if (     lpArguments != nullptr &&
         SK_Exception_HandleThreadName (
           dwExceptionCode, dwExceptionFlags,
           nNumberOfArguments,
             lpArguments )
       )
    {
      THREADNAME_INFO *pTni =
        ((THREADNAME_INFO *)&(lpArguments [0]));

      THREADNAME_INFO tni = {               };
      tni.dwThreadID      = pTni->dwThreadID;
      tni.dwFlags         = pTni->dwFlags;
      tni.dwType          = pTni->dwType;
      tni.szName          = pTni->szName;

      SK_Thread_RaiseNameException (&tni);

      skip = true;
    }

    if (pTlsThis != nullptr)
    {
      InterlockedIncrement (&pTlsThis->debug.exceptions);
    }


    if (! skip)
    {
      if (pTlsThis == nullptr || (! pTlsThis->debug.silent_exceptions))
      {
        const auto SK_ExceptionFlagsToStr = [](DWORD dwFlags) ->
        const char*
        {
          if (dwFlags & EXCEPTION_NONCONTINUABLE)
            return "Non-Continuable";
          if (dwFlags & EXCEPTION_UNWINDING)
            return "Unwind In Progress";
          if (dwFlags & EXCEPTION_EXIT_UNWIND)
            return "Exit Unwind In Progress";
          if (dwFlags & EXCEPTION_STACK_INVALID)
            return "Misaligned or Overflowed Stack";
          if (dwFlags & EXCEPTION_NESTED_CALL)
            return "Nested Exception Handler";
          if (dwFlags & EXCEPTION_TARGET_UNWIND)
            return "Target Unwind In Progress";
          if (dwFlags & EXCEPTION_COLLIDED_UNWIND)
            return "Collided Exception Handler";
          return "Unknown";
        };

        wchar_t wszCallerName [MAX_PATH + 2] = { };

        SK_SEH_CompatibleCallerName (
          _ReturnAddress (), wszCallerName
        );

        bool print = true;

        // NVIDIA, just STFU please :)
        if (StrStrIW (wszCallerName, L"MessageBus.dll"))
          print = false;

        if (print)
        {
          ULONG num_times_reported =
            InterlockedIncrement (
              &SK_SEH_ReportedCallSites.get ()[_ReturnAddress ()].count
            );

          if (config.system.log_level < 1 &&
                num_times_reported >= SK_SEH_CallRecord::LogSilenceThreshold)
          {
            print = false;

            if (num_times_reported == SK_SEH_CallRecord::LogSilenceThreshold)
            {
              SK_LOG0 (
                ( L"Exception raised from %ws [%p] multiple times; exceptions"
                  L" are being silenced because LogLevel=0", wszCallerName,
                                                       _ReturnAddress () ),
                  L"SEH-Except"
              );
            }
          }
        }

        if (print)
        {
          SK_StripUserNameFromPathW (wszCallerName);

          SK_LOG0 ( ( L"Exception Code: %x  - Flags: (%hs) -  Arg Count: %u   "
                      L"[ Calling Module:  %s ]", dwExceptionCode,
                          SK_ExceptionFlagsToStr (dwExceptionFlags),
                            nNumberOfArguments,      wszCallerName),
                      L"SEH-Except"
          );

          char szSymbol [512] = { };

          SK_GetSymbolNameFromModuleAddr (
                               SK_GetCallingDLL (),
                      (uintptr_t)_ReturnAddress (),
                                          szSymbol, 511 );

          SK_LOG0 ( ( L"  >> Best-Guess For Source of Exception:  %hs",
                      szSymbol ),
                      L"SEH-Except"
          );
        }
      }
    }
  }

  if (! skip)
  {
    // TODO: Add config setting for interactive debug
    if (config.system.log_level > 1 && SK_IsDebuggerPresent ())
    {
      // Ignore Unity
      if (! SK_GetCurrentRenderBackend ().windows.unity)
      {
        __debugbreak ();
      }
    }

    SK::Diagnostics::CrashHandler::Reinstall ();

    if (dwExceptionCode != EXCEPTION_BREAKPOINT || SK_IsDebuggerPresent ())
    {
      //__try
      //{
        SK_RaiseException (
          dwExceptionCode,    dwExceptionFlags,
          nNumberOfArguments, lpArguments
        );
      //}
      //
      //__except (EXCEPTION_EXECUTE_HANDLER)
      //{
      //}
    }
  }
}

BOOL bWasDebuggerPresentInitially = -1;

///
/// Anti-debug workaround, avoid CloseHandle (...) exception handler boobytraps
BOOL
WINAPI
CloseHandle_Detour ( HANDLE hObject )
{
  if (    bWasDebuggerPresentInitially == -1 && SK_GetFramesDrawn () > 0 )
  { if (  bWasDebuggerPresentInitially == -1 )
          bWasDebuggerPresentInitially = SK_IsDebuggerPresent ();
    if (! bWasDebuggerPresentInitially)
    {
      return
        CloseHandle_Original (hObject);
    }
  }

  BOOL bRet = FALSE;

  __try
  {
    DWORD                               dwFlags    (0x0);
    if (GetHandleInformation (hObject, &dwFlags) && 0x0 ==
                                       (dwFlags & HANDLE_FLAG_PROTECT_FROM_CLOSE))
    {
      bRet =
        CloseHandle_Original (hObject);
    }

    assert (false);
  }

  __except ( GetExceptionCode () == EXCEPTION_INVALID_HANDLE
                                  ? EXCEPTION_EXECUTE_HANDLER
                                  : EXCEPTION_CONTINUE_SEARCH )
  {
    bRet = FALSE;
  }

  if (! bRet)
    SK_SetLastError (ERROR_INVALID_HANDLE);

  return
    bRet;
}


//#define _EXTENDED_DEBUG

void
SK_HookEngine_HookGetProcAddress (void)
{
#ifdef _EXTENDED_DEBUG
  // Our GetProcAddress hook relies on SetLastError, which we also hook.
  //
  //   Only do this when extended debug is turned on, it has a tendency
  //     to break things...
  SK_CreateDLLHook2 (      L"kernel32",
                            "SetLastError",
                             SetLastError_Detour,
    static_cast_p2p <void> (&SetLastError_Original) );
#endif
  
  SK_CreateDLLHook2 (      L"kernel32",
                            "GetProcAddress",
                             GetProcAddress_Detour,
    static_cast_p2p <void> (&GetProcAddress_Original) );
  
  SK_ApplyQueuedHooks ();
}

bool
SK::Diagnostics::Debugger::Allow  (bool bAllow)
{
  struct LocalInitOnce_s
  {
    LocalInitOnce_s (void) noexcept
    {
      SK_RunOnce (
      {
        extern void SK_DbgHlp_Init (void);
                    SK_DbgHlp_Init ();

        void SK_SymSetOpts (void);
             SK_SymSetOpts (    );
      });
    }

    ~LocalInitOnce_s (void)
    {
      SK_RunOnce (
      {
        // This is hooked so that we can catch and cleanly unload
        //   if a game crashes during exit.
        SK_CreateDLLHook2 (    L"kernel32",
                                "ExitProcess",
                                 ExitProcess_Detour,
        static_cast_p2p <void> (&ExitProcess_Original) );

        SK_CreateDLLHook2 (    L"kernel32",
                                "TerminateProcess",
                                 TerminateProcess_Detour,
        static_cast_p2p <void> (&TerminateProcess_Original) );

        SK_Thread_InitDebugExtras ();

        SK_InitCompatBlacklist ();

        void SK_HookEngine_HookGetProcAddress (void);
             SK_HookEngine_HookGetProcAddress ();
      });
    }
  } finish_init_on_return;

  if (config.compatibility.disable_debug_features)
  {
    return false;
  }

  static bool          basic_init = false;
  if (! std::exchange (basic_init, true))
  {
    SK_CreateDLLHook2 (      L"kernel32",
                              "OutputDebugStringA",
                               OutputDebugStringA_Detour,
      static_cast_p2p <void> (&OutputDebugStringA_Original) );
  
    SK_CreateDLLHook2 (      L"kernel32",
                              "OutputDebugStringW",
                               OutputDebugStringW_Detour,
      static_cast_p2p <void> (&OutputDebugStringW_Original) );
  
    SK_CreateDLLHook2 (      L"kernel32",
                              "IsDebuggerPresent",
                               IsDebuggerPresent_Detour,
      static_cast_p2p <void> (&IsDebuggerPresent_Original) );
  }

  if (SK_IsHostAppSKIM ())
  {
    return true;
  }

  static volatile LONG                      __init = 0;
  if (! InterlockedCompareExchangeAcquire (&__init, 1, 0))
  {
    // Workaround Steamworks Anti-Debug
    //
#ifdef _EXTENDED_DEBUG
    SK_CreateDLLHook2 (    L"NtDll",
                            "RtlAcquirePebLock",
                             RtlAcquirePebLock_Detour,
    static_cast_p2p <void> (&RtlAcquirePebLock_Original) );

    SK_CreateDLLHook2 (    L"NtDll",
                            "RtlReleasePebLock",
                             RtlReleasePebLock_Detour,
    static_cast_p2p <void> (&RtlReleasePebLock_Original) );

    SK_CreateDLLHook2 (    L"kernel32",
                            "TerminateThread",
                             TerminateThread_Detour,
    static_cast_p2p <void> (&TerminateThread_Original) );
#endif

#ifdef _EXTENDED_DEBUG
    if (true)//config.compatibility.advanced_debug)
    {
      SK_CreateDLLHook2 (      L"kernel32",
                                "SetThreadContext",
                                 SetThreadContext_Detour,
        static_cast_p2p <void> (&SetThreadContext_Original) );

      // Windows 10 Stability De-enhancer that Denuvo is likely to
      //   try eventually...
      if (SK_GetProcAddress ( SK_GetModuleHandle (L"kernelbase.dll"),
                           "SetProcessValidCallTargets") != nullptr)
      {
        // Kill it proactively before it kills us
        SK_CreateDLLHook2 (      L"kernelbase",
                                  "SetProcessValidCallTargets",
                                   SetProcessValidCallTargets_Detour,
          static_cast_p2p <void> (&SetProcessValidCallTargets_Original) );
      }

      spoof_debugger = bAllow;

      if (config.compatibility.rehook_loadlibrary)
      {
        SK_CreateDLLHook2 (      L"kernel32",
                                  "TerminateThread",
                                   TerminateThread_Detour,
          static_cast_p2p <void> (&TerminateThread_Original) );

        //SK_CreateDLLHook2 (       L"NtDll",
        //                           "RtlExitUserThread",
        //                            ExitThread_Detour,
        //   static_cast_p2p <void> (&ExitThread_Original) );

        SK_CreateDLLHook2 (      L"NtDll",
                                  "NtTerminateProcess",
                                   NtTerminateProcess_Detour,
          static_cast_p2p <void> (&NtTerminateProcess_Original) );
      }
    }
#endif

    // Forza Horizon 5 crashes if these are hooked for some reason,
    //   the actual code run via the hook is inconsequential
    //
#if 0
    SK_CreateDLLHook2 (      L"NtDll",
                              "NtCreateThreadEx",
                               NtCreateThreadEx_Detour,
      static_cast_p2p <void> (&NtCreateThreadEx_Original) );
#else
    SK_CreateDLLHook2 (      L"NtDll",
                              "ZwCreateThreadEx",
                               ZwCreateThreadEx_Detour,
      static_cast_p2p <void> (&ZwCreateThreadEx_Original) );
#endif

    SK_CreateDLLHook2 (      L"NtDll",
                              "ZwSetInformationThread",
                               ZwSetInformationThread_Detour,
      static_cast_p2p <void> (&ZwSetInformationThread_Original) );


    SK_CreateDLLHook2 (      L"Kernel32",
                              "RaiseException",
                               RaiseException_Detour,
      static_cast_p2p <void> (&RaiseException_Original) );

#ifdef _EXTENDED_DEBUG
    if (true)//config.advanced_debug)
    {
      // Unstable in Halo Infinite
      SK_CreateDLLHook2 (      L"NtDll",
                              "RtlRaiseException",
                               RtlRaiseException_Detour,
      static_cast_p2p <void> (&RtlRaiseException_Original) );

      SK_CreateDLLHook2 (      L"NtDll",
                                "RtlExitUserProcess",
                                 RtlExitUserProcess_Detour,
        static_cast_p2p <void> (&RtlExitUserProcess_Original) );

          SK_CreateDLLHook2 (      L"NtDll",
                                "RtlExitUserThread",
                                 RtlExitUserThread_Detour,
        static_cast_p2p <void> (&RtlExitUserThread_Original) );

      SK_CreateDLLHook2 (      L"kernel32",
                                "DebugBreak",
                                 DebugBreak_Detour,
        static_cast_p2p <void> (&DebugBreak_Original) );

      SK_CreateDLLHook2 (      L"NtDll",
                              "DbgBreakPoint",
                               DbgBreakPoint_Detour,
      static_cast_p2p <void> (&DbgBreakPoint_Original) );
    }
#endif

    //if (config.system.trace_create_thread)
    //{
    //  SK_CreateDLLHook2 (      L"kernel32",
    //                            "CreateThread",
    //                             CreateThread_Detour,
    //    static_cast_p2p <void> (&CreateThread_Original) );
    //}

    // Watch_Dogs Legion does not like having this detour'd
    //
    ////////////SK_CreateDLLHook2 (      L"kernel32",
    ////////////                          "GetCommandLineW",
    ////////////                           GetCommandLineW_Detour,
    ////////////  static_cast_p2p <void> (&GetCommandLineW_Original) );
    ////////////
    ////////////SK_CreateDLLHook2 (      L"kernel32",
    ////////////                          "GetCommandLineA",
    ////////////                           GetCommandLineA_Detour,
    ////////////  static_cast_p2p <void> (&GetCommandLineA_Original) );

    SK_CreateDLLHook2 (      L"kernel32",
                              "ResetEvent",
                               ResetEvent_Detour,
      static_cast_p2p <void> (&ResetEvent_Original) );

#ifdef _EXTENDED_DEBUG
    if (true)//config.advanced_debug)
    {
      SK_CreateDLLHook2 (      L"NtDll",
                                "DbgUiRemoteBreakin",
                                 DbgUiRemoteBreakin_Detour,
        static_cast_p2p <void> (&DbgUiRemoteBreakin_Original) );
      SK_CreateDLLHook2 (      L"kernel32",
                                "CheckRemoteDebuggerPresent",
                                 CheckRemoteDebuggerPresent_Detour,
        static_cast_p2p <void> (&CheckRemoteDebuggerPresent_Original) );
    }
#endif

    // Only hook if we actually have a debugger present, because
    //   hooking this will be detected by many DRM / anti-debug as
    //    the smoking gun that there is a debugger.
    //if (IsDebuggerPresent ())
    {
      SK_CreateDLLHook2 (    L"kernel32",
                              "CloseHandle",
                               CloseHandle_Detour,
      static_cast_p2p <void> (&CloseHandle_Original) );
    }

#ifdef _EXTENDED_DEBUG
    if (true)//config.advanced_debug)
    {
      RtlAcquirePebLock_Detour ();
      RtlReleasePebLock_Detour ();
    }
#endif

    InterlockedIncrementRelease (&__init);
  }

  else
    SK_Thread_SpinUntilAtomicMin (&__init, 2);

  return bAllow;
}

FILE* SK::Diagnostics::Debugger::fStdErr = nullptr;
FILE* SK::Diagnostics::Debugger::fStdIn  = nullptr;
FILE* SK::Diagnostics::Debugger::fStdOut = nullptr;

void
SK::Diagnostics::Debugger::SpawnConsole (void)
{
  AllocConsole ();

  static volatile LONG               init = FALSE;
  if (! InterlockedCompareExchange (&init, 1, 0))
  {
    fStdIn  = _wfreopen (L"CONIN$",  L"r", stdin);
    fStdOut = _wfreopen (L"CONOUT$", L"w", stdout);
    fStdErr = _wfreopen (L"CONOUT$", L"w", stderr);
  }
}

BOOL
SK::Diagnostics::Debugger::CloseConsole (void)
{
  if (                     SK::Diagnostics::Debugger::fStdErr != nullptr)
    fclose (std::exchange (SK::Diagnostics::Debugger::fStdErr,   nullptr));

  if (                     SK::Diagnostics::Debugger::fStdIn != nullptr)
    fclose (std::exchange (SK::Diagnostics::Debugger::fStdIn,   nullptr));

  if (                     SK::Diagnostics::Debugger::fStdOut != nullptr)
    fclose (std::exchange (SK::Diagnostics::Debugger::fStdOut,   nullptr));

  return
    FreeConsole ();
}

BOOL
WINAPI
SK_IsDebuggerPresent (void)
{
  if (IsDebuggerPresent_Original == nullptr)
  {
    if (ReadAcquire (&__SK_DLL_Attached))
      SK_RunOnce (SK::Diagnostics::Debugger::Allow ()); // DONTCARE, just init
  }

  if (bRealDebug)
    return TRUE;

  auto _IsDebuggerPresent = [&](void) -> bool
  {
    return
      IsDebuggerPresent_Original != nullptr ?
      IsDebuggerPresent_Original ()         :
      IsDebuggerPresent          ();
  };

  // Fast path during initialization
  if (SK_GetFramesDrawn () < 1)
  {
    return _IsDebuggerPresent ();
  }

  bool bDebugger = false;

  // Now we get serious, and avoid anti-debug stuff...
  __try {
    bDebugger = _IsDebuggerPresent ();
  }

  __except (EXCEPTION_EXECUTE_HANDLER)
  {
    return FALSE;
  }

  return bDebugger;
}







BOOL
IMAGEAPI
SymRefreshModuleList (
  _In_ HANDLE hProcess
)
{
  if (! config.system.handle_crashes)
    return FALSE;

  if (SymRefreshModuleList_Imp != nullptr && cs_dbghelp != nullptr)
  {
    SK_SymSetOpts ();

    std::scoped_lock <SK_Thread_HybridSpinlock> auto_lock (*cs_dbghelp);

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
  if (! config.system.handle_crashes)
    return FALSE;

  if (StackWalk64_Imp != nullptr && cs_dbghelp != nullptr)
  {
    SK_SymSetOpts ();

    std::scoped_lock <SK_Thread_HybridSpinlock> auto_lock (*cs_dbghelp);

    return
      StackWalk64_Imp ( MachineType,
                          hProcess,
                          hThread,
                            StackFrame,
                            ContextRecord,
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
  if (! config.system.handle_crashes)
    return FALSE;

  if (StackWalk_Imp != nullptr && cs_dbghelp != nullptr)
  {
    SK_SymSetOpts ();

    std::scoped_lock <SK_Thread_HybridSpinlock> auto_lock (*cs_dbghelp);

    return
      StackWalk_Imp ( MachineType,
                        hProcess,
                        hThread,
                          StackFrame,
                          ContextRecord,
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
  if (SymSetOptions_Imp != nullptr && cs_dbghelp != nullptr)
  {
    std::scoped_lock <SK_Thread_HybridSpinlock> auto_lock (*cs_dbghelp);

    return
      SymSetOptions_Imp (SymOptions);
  }

  return 0x0;
}

BOOL
IMAGEAPI
SymSetExtendedOption (
  _In_ IMAGEHLP_EXTENDED_OPTIONS option,
  _In_ BOOL                      value )
{
  if (SymSetExtendedOption_Imp != nullptr && cs_dbghelp != nullptr)
  {
    std::scoped_lock <SK_Thread_HybridSpinlock> auto_lock (*cs_dbghelp);

    return
      SymSetExtendedOption_Imp (option, value);
  }

  return FALSE;
}


BOOL
IMAGEAPI
SymGetTypeInfo (
  _In_  HANDLE                    hProcess,
  _In_  DWORD64                   ModBase,
  _In_  ULONG                     TypeId,
  _In_  IMAGEHLP_SYMBOL_TYPE_INFO GetType,
  _Out_ PVOID                     pInfo )
{
  if (SymGetTypeInfo_Imp != nullptr)
  {
    std::scoped_lock <SK_Thread_HybridSpinlock> auto_lock (*cs_dbghelp);

    return
      SymGetTypeInfo_Imp (hProcess, ModBase, TypeId, GetType, pInfo);
  }

  return FALSE;
}


DWORD64
IMAGEAPI
SymGetModuleBase64 (
  _In_ HANDLE  hProcess,
  _In_ DWORD64 qwAddr
)
{
  if (SymGetModuleBase64_Imp != nullptr)
  {
    SK_SymSetOpts ();

    std::scoped_lock <SK_Thread_HybridSpinlock> auto_lock (*cs_dbghelp);

    return
      SymGetModuleBase64_Imp ( hProcess, qwAddr );
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
  if (SymGetModuleBase_Imp != nullptr)
  {
    SK_SymSetOpts ();

    std::scoped_lock <SK_Thread_HybridSpinlock> auto_lock (*cs_dbghelp);

    return
      SymGetModuleBase_Imp ( hProcess, dwAddr );
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
  if (SymGetLineFromAddr64_Imp != nullptr && cs_dbghelp != nullptr)
  {
    SK_SymSetOpts ();

    std::scoped_lock <SK_Thread_HybridSpinlock> auto_lock (*cs_dbghelp);

    return
      SymGetLineFromAddr64_Imp ( hProcess, qwAddr,
                                          pdwDisplacement, Line64 );
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
  if (SymGetLineFromAddr_Imp != nullptr && cs_dbghelp != nullptr)
  {
    SK_SymSetOpts ();

    std::scoped_lock <SK_Thread_HybridSpinlock> auto_lock (*cs_dbghelp);

    return
      SymGetLineFromAddr_Imp ( hProcess, dwAddr,
                                        pdwDisplacement, Line );
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
  if (SymInitialize_Imp != nullptr && cs_dbghelp != nullptr)
  {
    SK_SymSetOpts ();

    std::scoped_lock <SK_Thread_HybridSpinlock> auto_lock (*cs_dbghelp);

    return
      SymInitialize_Imp ( hProcess, UserSearchPath, fInvadeProcess );
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
  if (SymUnloadModule_Imp != nullptr && cs_dbghelp != nullptr)
  {
    SK_SymSetOpts ();

    std::scoped_lock <SK_Thread_HybridSpinlock> auto_lock (*cs_dbghelp);

    return
      SymUnloadModule_Imp ( hProcess, BaseOfDll );
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
  if (SymUnloadModule64_Imp != nullptr && cs_dbghelp != nullptr)
  {
    SK_SymSetOpts ();

    std::scoped_lock <SK_Thread_HybridSpinlock> auto_lock (*cs_dbghelp);

    return
      SymUnloadModule64_Imp ( hProcess, BaseOfDll );
  }

  return FALSE;
}


using SymFromAddr_pfn = BOOL (IMAGEAPI *)(
  _In_      HANDLE       hProcess,
  _In_      DWORD64      Address,
  _Out_opt_ PDWORD64     Displacement,
  _Inout_   PSYMBOL_INFO Symbol
);

BOOL
__stdcall
SAFE_SymFromAddr (
  _In_      HANDLE          hProcess,
  _In_      DWORD64         Address,
  _Out_opt_ PDWORD64        Displacement,
  _Inout_   PSYMBOL_INFO    Symbol,
            SymFromAddr_pfn Trampoline )
{
  if (! config.system.handle_crashes)
    return FALSE;

  BOOL bRet = FALSE;

  if (Trampoline == nullptr)
    return bRet;

  __try {
    bRet =
      Trampoline (hProcess, Address, Displacement, Symbol);
  }

  __except ( GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ?
                                    EXCEPTION_EXECUTE_HANDLER  : EXCEPTION_CONTINUE_SEARCH )
  {
  }

  return bRet;
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
  if (! config.system.handle_crashes)
    return FALSE;

  if (SymFromAddr_Imp != nullptr)
  {
    if (cs_dbghelp != nullptr && (  ReadAcquire (&__SK_DLL_Attached)
                              && (! ReadAcquire (&__SK_DLL_Ending))))
    {
      SK_SymSetOpts ();

      std::scoped_lock <SK_Thread_HybridSpinlock> auto_lock (*cs_dbghelp);

      return
        SAFE_SymFromAddr ( hProcess, Address,
                                     Displacement, Symbol,
                                                   SymFromAddr_Imp );
    }
  }

  return FALSE;
}


BOOL
IMAGEAPI
SymCleanup (
  _In_ HANDLE hProcess )
{
  if (SymCleanup_Imp != nullptr)
  {
    std::scoped_lock <SK_Thread_HybridSpinlock> auto_lock (*cs_dbghelp);

    return
      SymCleanup_Imp ( hProcess );
  }

  return FALSE;
}


SK_LazyGlobal <
  concurrency::concurrent_unordered_set <DWORD64>
> _SK_DbgHelp_FailedModules;

extern "C"
NTSTATUS
WINAPI
SK_NtLdr_LockLoaderLock (ULONG Flags, ULONG* State, ULONG_PTR* Cookie)
{
  // The lock must not be acquired until DllMain (...) returns!
  if (ReadAcquire (&__SK_DLL_Refs) < 1)
    return STATUS_SUCCESS; // No-Op

  static LdrLockLoaderLock_pfn LdrLockLoaderLock =
        (LdrLockLoaderLock_pfn)SK_GetProcAddress   (L"NtDll.dll",
        "LdrLockLoaderLock");

  if (! LdrLockLoaderLock)
    return ERROR_NOT_FOUND;

  return
    LdrLockLoaderLock (Flags, State, Cookie);
}

extern "C"
NTSTATUS
WINAPI
SK_NtLdr_UnlockLoaderLock (ULONG Flags, ULONG_PTR Cookie)
{
  static LdrUnlockLoaderLock_pfn LdrUnlockLoaderLock =
        (LdrUnlockLoaderLock_pfn)SK_GetProcAddress (L"NtDll.dll",
        "LdrUnlockLoaderLock");

  if (! LdrUnlockLoaderLock)
    return ERROR_NOT_FOUND;

  NTSTATUS UnlockLoaderStatus =
    LdrUnlockLoaderLock (Flags, Cookie);

  // Check for Loader Unlock Failure...
  if (ReadAcquire (&__SK_DLL_Refs) >= 1 && Cookie != 0)
  {
    SK_ReleaseAssert (UnlockLoaderStatus == STATUS_SUCCESS);
  }

  return UnlockLoaderStatus;
}

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
  if (! config.system.handle_crashes)
    return 0;

  BOOL bRet = FALSE;

  if (SymLoadModule_Imp != nullptr)
  {
    HMODULE hModDll = nullptr;

    GetModuleHandleEx ( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                          (wchar_t *)(uintptr_t)BaseOfDll,
                                    &hModDll );

    size_t loaded =
      ( dbghelp_callers.find (hModDll) !=
        dbghelp_callers.cend (       ) );

    if (hModDll != nullptr && (! loaded))
    {
      if (cs_dbghelp != nullptr && (  ReadAcquire (&__SK_DLL_Attached)
                                && (! ReadAcquire (&__SK_DLL_Ending))))
      {
        SK_SymSetOpts ();

        std::scoped_lock <SK_Thread_HybridSpinlock> auto_lock (*cs_dbghelp);

        // Double lock-checked
        if (dbghelp_callers.find (hModDll) ==
            dbghelp_callers.cend (       ))
        {
          dbghelp_callers.insert (hModDll);

          if (! SymLoadModule_Imp (
                  hProcess, hFile, ImageName,
                    ModuleName,    BaseOfDll,
                                   SizeOfDll  ) )
          {
            _SK_DbgHelp_FailedModules->insert (BaseOfDll);
          }
        }
        else loaded = 1;
      }
    }

    bRet =
      ( loaded != 0 );

    if (hModDll != 0)
      SK_FreeLibrary (hModDll);
  }

  return
    bRet;
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
  if (! config.system.handle_crashes)
    return 0;

  BOOL bRet = FALSE;

  if (SymLoadModule64_Imp != nullptr)
  {
    HMODULE hModDll = nullptr;

    GetModuleHandleEx ( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                          (LPCWSTR)BaseOfDll,
                                    &hModDll );

    size_t loaded =
      ( dbghelp_callers.find (hModDll) !=
        dbghelp_callers.cend (       ) );

    if (hModDll != nullptr && (! loaded))
    {
      if (cs_dbghelp != nullptr && (  ReadAcquire (&__SK_DLL_Attached)
                                && (! ReadAcquire (&__SK_DLL_Ending))))
      {
        SK_SymSetOpts ();

        std::scoped_lock <SK_Thread_HybridSpinlock> auto_lock (*cs_dbghelp);

        // Double lock-checked
        if (dbghelp_callers.find (hModDll) ==
            dbghelp_callers.cend (       ))
        {
          dbghelp_callers.insert (hModDll);

          if (! SymLoadModule64_Imp (
                   hProcess, hFile, ImageName,
                     ModuleName,    BaseOfDll,
                                    SizeOfDll  ))
          {
            _SK_DbgHelp_FailedModules->insert (BaseOfDll);
          }

          else loaded = 1;
        }
      }
    }

    bRet =
      ( loaded != 0 );

    if (hModDll)
      SK_FreeLibrary (hModDll);
  }

  return
    bRet;
}

using SymSetSearchPathW_pfn = BOOL (IMAGEAPI *)(HANDLE,PCWSTR);

BOOL
IMAGEAPI
SymSetSearchPathW (
  _In_     HANDLE hProcess,
  _In_opt_ PCWSTR SearchPath )
{
  if (SymSetSearchPathW_Imp != nullptr && cs_dbghelp != nullptr)
  {
    std::scoped_lock <SK_Thread_HybridSpinlock> auto_lock (*cs_dbghelp);

    return
      SymSetSearchPathW_Imp (hProcess, SearchPath);
  }

  return FALSE;
}

using SymGetSearchPathW_pfn = BOOL (IMAGEAPI *)(HANDLE,PWSTR,DWORD);

BOOL
IMAGEAPI
SymGetSearchPathW (
  _In_      HANDLE hProcess,
  _Out_opt_ PWSTR  SearchPath,
  _In_      DWORD  SearchPathLength)
{
  if (SearchPath != nullptr)
  {  *SearchPath = L'\0';  }

  if (SymGetSearchPathW_Imp != nullptr)
  {
    BOOL bRet =
      SymGetSearchPathW_Imp (hProcess, SearchPath, SearchPathLength);

    if (SearchPath != nullptr)
    {   SearchPath [SearchPathLength-1] = L'\0';};

    return bRet;
  }

  return FALSE;
}

void
SK_DbgHlp_Init (void)
{
  static volatile LONG __init = 0;

  if (! InterlockedCompareExchangeAcquire (&__init, 1, 0))
  {
    SymGetSearchPathW_Imp =
      (SymGetSearchPathW_pfn)
      SK_GetProcAddress ( SK_Debug_LoadHelper (), "SymGetSearchPathW" );

    SymSetSearchPathW_Imp =
      (SymSetSearchPathW_pfn)
      SK_GetProcAddress ( SK_Debug_LoadHelper (), "SymSetSearchPathW" );

    SymRefreshModuleList_Imp =
      (SymRefreshModuleList_pfn)
      SK_GetProcAddress ( SK_Debug_LoadHelper (), "SymRefreshModuleList" );

    StackWalk64_Imp =
      (StackWalk64_pfn)
      SK_GetProcAddress ( SK_Debug_LoadHelper (), "StackWalk64" );

    StackWalk_Imp =
      (StackWalk_pfn)
      SK_GetProcAddress ( SK_Debug_LoadHelper (), "StackWalk" );

    SymSetOptions_Imp =
      (SymSetOptions_pfn)
      SK_GetProcAddress ( SK_Debug_LoadHelper (), "SymSetOptions" );

    SymSetExtendedOption_Imp =
      (SymSetExtendedOption_pfn)
      SK_GetProcAddress ( SK_Debug_LoadHelper (), "SymSetExtendedOption" );

    SymGetModuleBase64_Imp =
      (SymGetModuleBase64_pfn)
      SK_GetProcAddress ( SK_Debug_LoadHelper (), "SymGetModuleBase64" );

    SymGetModuleBase_Imp =
      (SymGetModuleBase_pfn)
      SK_GetProcAddress ( SK_Debug_LoadHelper (), "SymGetModuleBase" );

    SymGetLineFromAddr64_Imp =
      (SymGetLineFromAddr64_pfn)
      SK_GetProcAddress ( SK_Debug_LoadHelper (), "SymGetLineFromAddr64" );

    SymGetLineFromAddr_Imp =
      (SymGetLineFromAddr_pfn)
      SK_GetProcAddress ( SK_Debug_LoadHelper (), "SymGetLineFromAddr" );

    SymInitialize_Imp =
      (SymInitialize_pfn)
      SK_GetProcAddress ( SK_Debug_LoadHelper (), "SymInitialize" );

    SymUnloadModule_Imp =
      (SymUnloadModule_pfn)
      SK_GetProcAddress ( SK_Debug_LoadHelper (), "SymUnloadModule" );

    SymUnloadModule64_Imp =
      (SymUnloadModule64_pfn)
      SK_GetProcAddress ( SK_Debug_LoadHelper (), "SymUnloadModule64" );

    SymFromAddr_Imp =
      (SymFromAddr_pfn)
      SK_GetProcAddress ( SK_Debug_LoadHelper (), "SymFromAddr" );

    SymCleanup_Imp =
      (SymCleanup_pfn)
      SK_GetProcAddress ( SK_Debug_LoadHelper (), "SymCleanup" );

    SymLoadModule_Imp =
      (SymLoadModule_pfn)
      SK_GetProcAddress ( SK_Debug_LoadHelper (), "SymLoadModule" );

    SymLoadModule64_Imp =
      (SymLoadModule64_pfn)
      SK_GetProcAddress ( SK_Debug_LoadHelper (), "SymLoadModule64" );

    SymGetTypeInfo_Imp =
      (SymGetTypeInfo_pfn)
      SK_GetProcAddress ( SK_Debug_LoadHelper (), "SymGetTypeInfo" );

    InterlockedIncrementRelease (&__init);
  }

  else
    SK_Thread_SpinUntilAtomicMin (&__init, 2);
}



bool
SK_Win32_FormatMessageForException (
  wchar_t*&    lpMsgBuf,
  unsigned int nExceptionCode, ... )
{
  wchar_t* lpMsgFormat = nullptr;

  FormatMessage ( FORMAT_MESSAGE_ALLOCATE_BUFFER |
                  FORMAT_MESSAGE_FROM_SYSTEM     |
                  FORMAT_MESSAGE_FROM_HMODULE    |
                  FORMAT_MESSAGE_IGNORE_INSERTS,
                 SK_GetModuleHandle (L"NtDll.dll"),
                      nExceptionCode,
                        MAKELANGID ( LANG_NEUTRAL,
                                  SUBLANG_DEFAULT ),
                    (LPWSTR)&lpMsgFormat, 4096, nullptr );

  if (  lpMsgFormat != nullptr &&
       *lpMsgFormat != L'\0'      )
  {
    lpMsgBuf =
      (wchar_t *)SK_LocalAlloc (LPTR, 8193);

    if (lpMsgBuf != nullptr)
    {
      va_list      args = nullptr;
      va_start    (args,              nExceptionCode);
      vswprintf_s (lpMsgBuf, 4096, lpMsgFormat, args);
      va_end      (args);

      SK_LocalFree (               lpMsgFormat);

      return true;
    }
  }

  return false;
}

using RtlNtStatusToDosError_pfn = ULONG (WINAPI*)(NTSTATUS);

void
SK_SEH_LogException ( unsigned int        nExceptionCode,
                      EXCEPTION_POINTERS* pException,
                      LPVOID              lpRetAddr )
{
  if (config.system.log_level < 2)
    return;

  if (SK_TLS_Bottom ()->debug.silent_exceptions)
    return;

  static RtlNtStatusToDosError_pfn
         RtlNtStatusToDosError =
        (RtlNtStatusToDosError_pfn)SK_GetProcAddress ( L"NtDll.dll",
        "RtlNtStatusToDosError" );

  SK_ReleaseAssert (RtlNtStatusToDosError != nullptr);

  if (RtlNtStatusToDosError == nullptr)
    return;

  wchar_t* lpMsgBuf = nullptr;

  const ULONG ulDosError =
    RtlNtStatusToDosError (nExceptionCode);

  if (ulDosError != ERROR_MR_MID_NOT_FOUND)
  {
    lpMsgBuf = (wchar_t*)SK_LocalAlloc (LPTR, 8192);

    wcsncpy_s ( lpMsgBuf, 8191,
     _com_error (
       HRESULT_FROM_WIN32 (ulDosError)
                ).ErrorMessage (),
                          _TRUNCATE );
  }


  // Get the symbol name and ignore Scaleform's ridiculousness
  //
  char szSymbol [256] = { };

  SK_GetSymbolNameFromModuleAddr (
    SK_GetModuleFromAddr (pException->ExceptionRecord->ExceptionAddress),
               (uintptr_t)pException->ExceptionRecord->ExceptionAddress,
                            szSymbol, 255
  );


  dll_log->LogEx ( false,
    L"===================================== "
    L"--------------------------------------"
    L"----------------------\n" );

  SK_LOG0 ( ( L"(~) Exception Code: 0x%010x (%s) < %s >",
                        nExceptionCode,
                        lpMsgBuf,
    SK_SummarizeCaller (pException->ExceptionRecord->
                                    ExceptionAddress).c_str () ),
             L"SEH-Except"
  );

  if (lpMsgBuf != nullptr)
    SK_LocalFree (lpMsgBuf);

  SK_LOG0 ( ( L"[@]         Return: %s",
    SK_SummarizeCaller (  lpRetAddr  ).c_str () ),
             L"SEH-Except"
  );



  // Hopelessly impossible to debug software; just ignore the exceptions
  //   it creates
  if ( *szSymbol == 'S' &&
        szSymbol == StrStrIA (szSymbol, "Scaleform") )
  {
    return;
  }


  dll_log->LogEx ( false,
    (SK_SEH_SummarizeException (pException) + L"\n").c_str ()
  );
}

SK_SEH_PreState
SK_SEH_ApplyTranslator (_In_opt_ _se_translator_function _NewSETranslator)
{
  SK_SEH_PreState pre_state = { };

  ////if (GetLastError () != NO_ERROR)
  ////{
  ////  SK_TLS* pTLS = SK_TLS_Bottom ();
  ////
  ////  if (pTLS != nullptr)
  ////  {
  ////    pre_state.dwErrorCode = pTLS->win32->error_state.code;
  ////    pre_state.lpCallSite  = pTLS->win32->error_state.call_site;
  ////    pre_state.fOrigTime   = pTLS->win32->error_state.last_time;
  ////  }
  ////}

  auto ret =
    _set_se_translator (_NewSETranslator);

  pre_state.pfnTranslator = ret;

  return pre_state;
}

_se_translator_function
SK_SEH_RemoveTranslator (SK_SEH_PreState pre_state)
{
  auto ret =
    _set_se_translator (pre_state.pfnTranslator);

  ////if (pre_state.dwErrorCode != 0 &&
  ///     GetLastError () != pre_state.dwErrorCode)
  ////{
  ////  SK_TLS* pTLS = SK_TLS_Bottom ();
  ////
  ////  if (pTLS != nullptr)
  ////  {
  ////    pTLS->win32->error_state.code      = pre_state.dwErrorCode;
  ////    pTLS->win32->error_state.call_site = pre_state.lpCallSite;
  ////    pTLS->win32->error_state.last_time = pre_state.fOrigTime;
  ////  }
  ////}

  return ret;
}

_se_translator_function
SK_SEH_SetTranslatorEX ( _In_opt_ _se_translator_function _NewSETranslator,
                                             SEH_LogLevel verbosity )
{
  UNREFERENCED_PARAMETER (verbosity);

  //// The SE translator is about to mess with this stuff.

//if (       verbosity != Unchanged)
//{
//  SK_TLS* pTLS =
//    SK_TLS_Bottom ();
//
//  if (     verbosity == Silent)
//    pTLS->debug.silent_exceptions = true;
//  else if (verbosity == Verbose0)
//    pTLS->debug.silent_exceptions = false;
//}

  auto ret =
    _set_se_translator (_NewSETranslator);

  return ret;
}

#pragma warning (pop)


volatile PVOID __SK_GameBaseAddr = nullptr;




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


using LdrGetDllHandleEx_pfn =
  NTSTATUS (NTAPI *)( IN  ULONG               Flags,
                      IN  PWSTR               DllPath            OPTIONAL,
                      IN  PULONG              DllCharacteristics OPTIONAL,
                      IN  PUNICODE_STRING_SK  DllName,
                      OUT PVOID              *DllHandle          OPTIONAL );

bool
SK_IsModuleLoaded (const wchar_t* wszModule)
{
  HMODULE hModule = nullptr;

  return
    GetModuleHandleExW (
      GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
       wszModule,
        &hModule
    ) != FALSE && hModule != nullptr;
}