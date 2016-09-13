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
#define _CRT_SECURE_NO_WARNINGS

#define ISOLATION_AWARE_ENABLED 1
#define PSAPI_VERSION           1

#include <Windows.h>

#include <psapi.h>
#pragma comment (lib, "psapi.lib")

#include <Commctrl.h>
#pragma comment (lib,    "advapi32.lib")
#pragma comment (lib,    "user32.lib")
#pragma comment (lib,    "comctl32.lib")
#pragma comment (linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' "  \
                         "version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df'" \
                         " language='*'\"")

#include "../config.h"

#include "../core.h"
#include "../log.h"
#include "../utility.h"

BOOL __stdcall SK_ValidateGlobalRTSSProfile (void);
void __stdcall SK_ReHookLoadLibrary         (void);

bool SK_LoadLibrary_SILENCE = true;

typedef HMODULE (WINAPI *LoadLibraryA_pfn)(LPCSTR  lpFileName);
typedef HMODULE (WINAPI *LoadLibraryW_pfn)(LPCWSTR lpFileName);

LoadLibraryA_pfn LoadLibraryA_Original = nullptr;
LoadLibraryW_pfn LoadLibraryW_Original = nullptr;

typedef HMODULE (WINAPI *LoadLibraryExA_pfn)
( _In_       LPCSTR  lpFileName,
  _Reserved_ HANDLE  hFile,
  _In_       DWORD   dwFlags
);

typedef HMODULE (WINAPI *LoadLibraryExW_pfn)
( _In_       LPCWSTR lpFileName,
  _Reserved_ HANDLE  hFile,
  _In_       DWORD   dwFlags
);

LoadLibraryExA_pfn LoadLibraryExA_Original = nullptr;
LoadLibraryExW_pfn LoadLibraryExW_Original = nullptr;

extern HMODULE hModSelf;

#include <Shlwapi.h>
#pragma comment (lib, "Shlwapi.lib")

#if 0
#include <unordered_set>
#include <string>
#include "../command.h" // str_hash_compare
typedef std::unordered_set < std::wstring, str_hash_compare <std::wstring> > SK_StringSetW;
SK_StringSetW rehook_loadlib;
#endif

BOOL
__stdcall
BlacklistLibraryW (LPCWSTR lpFileName)
{
  //if (blacklist.count (wcsrchr (lpFileName, L'\\') + 1))
  //{
#if 0
    if ( StrStrIW (lpFileName, L"ltc_game") ) {
      if (! (SK_LoadLibrary_SILENCE))
        dll_log.Log (L"[Black List] Preventing Raptr's overlay (ltc_game), it likes to crash games!");
      return TRUE;
    }

    else
#endif
    if ( StrStrIW (lpFileName, L"k_fps") ) {
      if (! (SK_LoadLibrary_SILENCE))
        dll_log.Log (L"[Black List] Disabling Razer Cortex to preserve sanity.");
      return TRUE;
    }

    //if (! (SK_LoadLibrary_SILENCE)) {
      //dll_log.Log ( L"[Black List] Miscellaneous Blacklisted DLL '%s' disabled.",
                      //lpFileName );
    //}

    //return TRUE;
  //}


  static bool isTalesOfZestiria = 
    StrStrIW (SK_GetHostApp ().c_str (), L"Tales of Zestiria.exe") != nullptr;

  if (isTalesOfZestiria) {
    if (StrStrIW (lpFileName, L"GeDoSaTo")) {
      dll_log.Log ( L"[Black List] GeDoSaTo Disabled for Tales of Zestiria, it is not thread safe.");
      return TRUE;
    }
  }

#if 0
  if (StrStrIW (lpFileName, L"igdusc32")) {
    dll_log->Log (L"[Black List] Intel D3D11 Driver Bypassed");
    return TRUE;
  }
#endif

#if 0
  if (StrStrIW (lpFileName, L"ig75icd32")) {
    dll_log->Log (L"[Black List] Preventing Intel Integrated OpenGL driver from activating...");
    return TRUE;
  }
#endif

  return FALSE;
}

BOOL
__stdcall
BlacklistLibraryA (LPCSTR lpFileName)
{
  wchar_t wszWideLibName [MAX_PATH];

  MultiByteToWideChar (CP_OEMCP, 0x00, lpFileName, -1, wszWideLibName, MAX_PATH);

  return BlacklistLibraryW (wszWideLibName);
}


void
__stdcall
SK_TraceLoadLibraryA ( HMODULE hCallingMod,
                       LPCSTR  lpFileName,
                       LPCSTR  lpFunction )
{
  std::wstring   mod_name   = SK_GetModuleName (hCallingMod);
  const wchar_t* wszModName = mod_name.c_str   ();

  dll_log.Log ( "[DLL Loader]   ( %-28ls ) loaded '%#64s' <%s>",
                  wszModName,
                    lpFileName,
                      lpFunction );

  // This is silly, this many string comparions per-load is
  //   not good. Hash the string and compare it in the future.
  if ( StrStrIW (wszModName, L"gameoverlayrenderer") ||
       StrStrIW (wszModName, L"RTSSHooks") ||
       StrStrIW (wszModName, L"GeDoSaTo") )
    SK_ReHookLoadLibrary ();
}

void
SK_TraceLoadLibraryW ( HMODULE hCallingMod,
                       LPCWSTR lpFileName,
                       LPCWSTR lpFunction )
{
  std::wstring   mod_name   = SK_GetModuleName (hCallingMod);
  const wchar_t* wszModName = mod_name.c_str   ();

  dll_log.Log ( L"[DLL Loader]   ( %-28s ) loaded '%#64s' <%s>",
                  wszModName,
                    lpFileName,
                      lpFunction );

  // This is silly, this many string comparions per-load is
  //   not good. Hash the string and compare it in the future.
  if ( StrStrIW (wszModName, L"gameoverlayrenderer") ||
       StrStrIW (wszModName, L"RTSSHooks") ||
       StrStrIW (wszModName, L"GeDoSaTo") )
    SK_ReHookLoadLibrary ();
}



HMODULE
WINAPI
LoadLibraryA_Detour (LPCSTR lpFileName)
{
  if (lpFileName == nullptr)
    return NULL;

  HMODULE hModEarly = GetModuleHandleA (lpFileName);

  if (hModEarly == NULL && BlacklistLibraryA (lpFileName))
    return NULL;

  HMODULE hMod = LoadLibraryA_Original (lpFileName);

  if (hModEarly != hMod && (! SK_LoadLibrary_SILENCE)) {
    SK_TraceLoadLibraryA ( SK_GetCallingDLL (),
                             lpFileName,
                               "LoadLibraryA" );
  }

  return hMod;
}

HMODULE
WINAPI
LoadLibraryW_Detour (LPCWSTR lpFileName)
{
  if (lpFileName == nullptr)
    return NULL;

  HMODULE hModEarly = GetModuleHandleW (lpFileName);

  if (hModEarly == NULL && BlacklistLibraryW (lpFileName))
    return NULL;

  HMODULE hMod = LoadLibraryW_Original (lpFileName);

  if (hModEarly != hMod && (! SK_LoadLibrary_SILENCE)) {
    SK_TraceLoadLibraryW ( SK_GetCallingDLL (),
                             lpFileName,
                               L"LoadLibraryW" );
  }

  return hMod;
}

HMODULE
WINAPI
LoadLibraryExA_Detour (
  _In_       LPCSTR lpFileName,
  _Reserved_ HANDLE hFile,
  _In_       DWORD  dwFlags )
{
  if (lpFileName == nullptr)
    return NULL;

  HMODULE hModEarly = GetModuleHandleA (lpFileName);

  if (hModEarly == NULL && BlacklistLibraryA (lpFileName))
    return NULL;

  HMODULE hMod = LoadLibraryExA_Original (lpFileName, hFile, dwFlags);

  if ( hModEarly != hMod && (! ((dwFlags & LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE) ||
                                (dwFlags & LOAD_LIBRARY_AS_IMAGE_RESOURCE)))
                         && (! SK_LoadLibrary_SILENCE) ) {
    SK_TraceLoadLibraryA ( SK_GetCallingDLL (),
                             lpFileName,
                               "LoadLibraryExA" );
  }

  return hMod;
}

HMODULE
WINAPI
LoadLibraryExW_Detour (
  _In_       LPCWSTR lpFileName,
  _Reserved_ HANDLE  hFile,
  _In_       DWORD   dwFlags )
{
  if (lpFileName == nullptr)
    return NULL;

  HMODULE hModEarly = GetModuleHandleW (lpFileName);

  if (hModEarly == NULL && BlacklistLibraryW (lpFileName))
    return NULL;

  HMODULE hMod = LoadLibraryExW_Original (lpFileName, hFile, dwFlags);

  if ( hModEarly != hMod && (! ((dwFlags & LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE) ||
                                (dwFlags & LOAD_LIBRARY_AS_IMAGE_RESOURCE)))
                         && (! SK_LoadLibrary_SILENCE) ) {
    SK_TraceLoadLibraryW ( SK_GetCallingDLL (),
                             lpFileName,
                               L"LoadLibraryExW" );
  }

  return hMod;
}

struct sk_loader_hooks_t {
  // Manually unhooked for compatibility, DO NOT rehook!
  bool   unhooked              = false;

  LPVOID LoadLibraryA_target   = nullptr;
  LPVOID LoadLibraryExA_target = nullptr;
  LPVOID LoadLibraryW_target   = nullptr;
  LPVOID LoadLibraryExW_target = nullptr;
} _loader_hooks;

struct SK_ThirdPartyDLLs {
  struct {
    HMODULE rtss_hooks    = nullptr;
    HMODULE steam_overlay = nullptr;
  } overlays;
  struct {
    HMODULE gedosato      = nullptr;
  } misc;
} third_party_dlls;

bool
__stdcall
SK_CheckForGeDoSaTo (void)
{
  if (third_party_dlls.misc.gedosato)
    return true;

  return false;
}

//
// Gameoverlayrenderer{64}.dll messes with LoadLibrary hooking, which
//   means identifying which DLL loaded another becomes impossible
//     unless we remove and re-install our hooks.
//
//   ** GeDoSaTo and RTSS may also do the same depending on config. **
//
// Hook order with LoadLibrary is not traditionally important for a mod
//   system such as Special K, but the compatibility layer benefits from
//     knowing exactly WHAT was responsible for loading a library.
//
void
__stdcall
SK_ReHookLoadLibrary (void)
{
  if (_loader_hooks.unhooked)
    return;

  if (_loader_hooks.LoadLibraryA_target != nullptr) {
    SK_RemoveHook (_loader_hooks.LoadLibraryA_target);
    _loader_hooks.LoadLibraryA_target = nullptr;
  }

  SK_CreateDLLHook ( L"kernel32.dll", "LoadLibraryA",
                     LoadLibraryA_Detour,
           (LPVOID*)&LoadLibraryA_Original,
                    &_loader_hooks.LoadLibraryA_target );

  SK_EnableHook (_loader_hooks.LoadLibraryA_target);


  if (_loader_hooks.LoadLibraryW_target != nullptr) {
    SK_RemoveHook (_loader_hooks.LoadLibraryW_target);
    _loader_hooks.LoadLibraryW_target = nullptr;
  }

  SK_CreateDLLHook ( L"kernel32.dll", "LoadLibraryW",
                     LoadLibraryW_Detour,
           (LPVOID*)&LoadLibraryW_Original,
                    &_loader_hooks.LoadLibraryW_target );

  SK_EnableHook (_loader_hooks.LoadLibraryW_target);


  if (_loader_hooks.LoadLibraryExA_target != nullptr) {
    SK_RemoveHook (_loader_hooks.LoadLibraryExA_target);
    _loader_hooks.LoadLibraryExA_target = nullptr;
  }

  SK_CreateDLLHook ( L"kernel32.dll", "LoadLibraryExA",
                     LoadLibraryExA_Detour,
           (LPVOID*)&LoadLibraryExA_Original,
                    &_loader_hooks.LoadLibraryExA_target );

  SK_EnableHook (_loader_hooks.LoadLibraryExA_target);


  if (_loader_hooks.LoadLibraryExW_target != nullptr) {
    SK_RemoveHook (_loader_hooks.LoadLibraryExW_target);
    _loader_hooks.LoadLibraryExW_target = nullptr;
  }

  SK_CreateDLLHook ( L"kernel32.dll", "LoadLibraryExW",
                     LoadLibraryExW_Detour,
           (LPVOID*)&LoadLibraryExW_Original,
                    &_loader_hooks.LoadLibraryExW_target );

  SK_EnableHook (_loader_hooks.LoadLibraryExW_target);
}

void
SK_UnhookLoadLibrary (void)
{
  _loader_hooks.unhooked = true;

  if (_loader_hooks.LoadLibraryA_target != nullptr) {
    SK_RemoveHook (_loader_hooks.LoadLibraryA_target);
    _loader_hooks.LoadLibraryA_target = nullptr;
  }

  if (_loader_hooks.LoadLibraryW_target != nullptr) {
    SK_RemoveHook (_loader_hooks.LoadLibraryW_target);
    _loader_hooks.LoadLibraryW_target = nullptr;
  }

  if (_loader_hooks.LoadLibraryExA_target != nullptr) {
    SK_RemoveHook (_loader_hooks.LoadLibraryExA_target);
    _loader_hooks.LoadLibraryExA_target = nullptr;
  }

  if (_loader_hooks.LoadLibraryExW_target != nullptr) {
    SK_RemoveHook (_loader_hooks.LoadLibraryExW_target);
    _loader_hooks.LoadLibraryExW_target = nullptr;
  }
}

void
__stdcall
SK_InitCompatBlacklist (void)
{
#if 0
  rehook_loadlib.insert (L"gameoverlayrenderer.dll");
  rehook_loadlib.insert (L"gameoverlayrenderer64.dll");

  rehook_loadlib.insert (L"GeDoSaTo.dll");

  rehook_loadlib.insert (L"RTSSHooks.dll");
  rehook_loadlib.insert (L"RTSSHooks64.dll");
#endif

  SK_ReHookLoadLibrary ();
}

void
__stdcall
EnumLoadedModules (void)
{
  // Begin logging new loads after this
  SK_LoadLibrary_SILENCE = false;

  static iSK_Logger* pLogger = SK_CreateLog (L"logs/preloads.log");

  DWORD        dwProcID = GetCurrentProcessId ();

  HMODULE      hMods [1024];
  HANDLE       hProc = nullptr;
  DWORD        cbNeeded;
  unsigned int i;

  // Get a handle to the process.
  hProc = OpenProcess ( PROCESS_QUERY_INFORMATION |
                        PROCESS_VM_READ,
                          FALSE,
                            dwProcID );

  if (hProc == nullptr) {
    pLogger->close ();
    delete pLogger;
    return;
  }

  if ( EnumProcessModules ( hProc,
                              hMods,
                                sizeof (hMods),
                                  &cbNeeded) )
  {
    for ( i = 0; i < (cbNeeded / sizeof (HMODULE)); i++ )
    {
      wchar_t wszModName [MAX_PATH + 2];

      // Get the full path to the module's file.
      if ( GetModuleFileNameExW ( hProc,
                                    hMods [i],
                                      wszModName,
                                        sizeof (wszModName) /
                                        sizeof (wchar_t) ) )
      {
        if ( (! third_party_dlls.overlays.rtss_hooks) &&
              StrStrIW (wszModName, L"RTSSHooks") ) {
          // Hold a reference to this DLL so it is not unloaded prematurely
          GetModuleHandleEx ( 0x00,
                                wszModName,
                                  &third_party_dlls.overlays.rtss_hooks );
          SK_ReHookLoadLibrary ();
        }

        else if ( (! third_party_dlls.overlays.steam_overlay) &&
                   StrStrIW (wszModName, L"gameoverlayrenderer") ) {
          // Hold a reference to this DLL so it is not unloaded prematurely
          GetModuleHandleEx ( 0x00,
                                wszModName,
                                  &third_party_dlls.overlays.steam_overlay );
          SK_ReHookLoadLibrary ();
        }

        else if ( (! third_party_dlls.misc.gedosato) &&
                   StrStrIW (wszModName, L"GeDoSaTo") ) {
          // Hold a reference to this DLL so it is not unloaded prematurely
          GetModuleHandleEx ( 0x00,
                                wszModName,
                                  &third_party_dlls.misc.gedosato );
          SK_ReHookLoadLibrary ();
        }

        else if ( StrStrIW (wszModName, L"ltc_help")) {
          MessageBox ( NULL,
                       L"AMD Gaming Evolved or Raptr is running, please exit this software to prevent compatibility problems.",
                       L"Special K Compatibility Layer", MB_OK | MB_ICONEXCLAMATION );
        }

        else if ( StrStrIW (wszModName, L"\\opengl32.dll")) {
          extern void WINAPI SK_HookGL (void);

          SK_HookGL ();
        }

        else if ( StrStrIW (wszModName, L"\\dxgi.dll")) {
          extern void WINAPI SK_HookDXGI (void);

          SK_HookDXGI ();
        }

        else if ( StrStrIW (wszModName, L"\\d3d9.dll")) {
          extern void WINAPI SK_HookD3D9 (void);

          SK_HookD3D9 ();
        }

        pLogger->Log ( L"[ Module ]  ( %ph )   -:-   * File: %s ",
                        (uintptr_t)hMods [i],
                          wszModName );
      }
    }
  }

  // Release the handle to the process.
  CloseHandle (hProc);

  pLogger->close ();
  delete pLogger;

  if (third_party_dlls.overlays.rtss_hooks != nullptr) {
    SK_ValidateGlobalRTSSProfile ();
  }

  // In 64-bit builds, RTSS is really sneaky :-/
  else if (SK_GetRTSSInstallDir ().length ()) {
    SK_ValidateGlobalRTSSProfile ();
  }

  else if ( GetModuleHandle (L"RTSSHooks.dll") ||
            GetModuleHandle (L"RTSSHooks64.dll") ) {
    SK_ValidateGlobalRTSSProfile ();

    SK_ReHookLoadLibrary ();
  }
}


#include <Commctrl.h>
#include <comdef.h>

HRESULT
CALLBACK
TaskDialogCallback (
  _In_ HWND     hWnd,
  _In_ UINT     uNotification,
  _In_ WPARAM   wParam,
  _In_ LPARAM   lParam,
  _In_ LONG_PTR dwRefData
)
{
  if (uNotification == TDN_HYPERLINK_CLICKED) {
    ShellExecuteW (nullptr, L"open", (wchar_t *)lParam, nullptr, nullptr, SW_SHOW);
    return S_OK;
  }

  return S_OK;
}

#include "../config.h"
#include "../ini.h"

BOOL
__stdcall
SK_ValidateGlobalRTSSProfile (void)
{
  if (config.system.ignore_rtss_delay)
    return TRUE;

  wchar_t wszRTSSHooks [MAX_PATH + 2] = { L'\0' };

  if (third_party_dlls.overlays.rtss_hooks) {
    GetModuleFileNameW (
      third_party_dlls.overlays.rtss_hooks,
        wszRTSSHooks,
          MAX_PATH );

    wchar_t* pwszShortName = wszRTSSHooks + lstrlenW (wszRTSSHooks);

    while (  pwszShortName      >  wszRTSSHooks &&
           *(pwszShortName - 1) != L'\\')
      --pwszShortName;

    *(pwszShortName - 1) = L'\0';
  } else {
    wcscpy (wszRTSSHooks, SK_GetRTSSInstallDir ().c_str ());
  }

  lstrcatW (wszRTSSHooks, L"\\Profiles\\Global");


  iSK_INI rtss_global (wszRTSSHooks);

  rtss_global.parse ();

  iSK_INISection& rtss_hooking =
    rtss_global.get_section (L"Hooking");


  bool valid = true;


  if ( (! rtss_hooking.contains_key (L"InjectionDelay")) ) {
    rtss_hooking.add_key_value (L"InjectionDelay", L"10000");
    valid = false;
  }
  else if (_wtol (rtss_hooking.get_value (L"InjectionDelay").c_str()) < 10000) {
    rtss_hooking.get_value (L"InjectionDelay") = L"10000";
    valid = false;
  }


  if ( (! rtss_hooking.contains_key (L"InjectionDelayTriggers")) ) {
    rtss_hooking.add_key_value (
      L"InjectionDelayTriggers",
        L"SpecialK32.dll,d3d9.dll,steam_api.dll,steam_api64.dll,dxgi.dll,SpecialK64.dll"
    );
    valid = false;
  }

  else {
    std::wstring& triggers =
      rtss_hooking.get_value (L"InjectionDelayTriggers");

    const wchar_t* delay_dlls [] = { L"SpecialK32.dll",
                                     L"d3d9.dll",
                                     L"steam_api.dll",
                                     L"steam_api64.dll",
                                     L"dxgi.dll",
                                     L"SpecialK64.dll" };

    const int     num_delay_dlls =
      sizeof (delay_dlls) / sizeof (const wchar_t *);

    for (int i = 0; i < num_delay_dlls; i++) {
      if (triggers.find (delay_dlls [i]) == std::wstring::npos) {
        valid = false;
        triggers += L",";
        triggers += delay_dlls [i];
      }
    }
  }

  // No action is necessary, delay triggers are working as intended.
  if (valid)
    return TRUE;

  static BOOL warned = FALSE;

  // Prevent the dialog from repeatedly popping up if the user decides to ignore
  if (warned)
    return TRUE;

  int               nButtonPressed = 0;
  TASKDIALOGCONFIG  config         = {0};

  int idx = 0;

  config.cbSize             = sizeof (config);
  config.hInstance          = GetModuleHandle (nullptr);
  config.hwndParent         = GetForegroundWindow ();
  config.pszWindowTitle     = L"Special K Compatibility Layer";
  config.dwCommonButtons    = TDCBF_OK_BUTTON;
  config.pButtons           = nullptr;
  config.cButtons           = 0;
  config.dwFlags            = TDF_ENABLE_HYPERLINKS;
  config.pfCallback         = TaskDialogCallback;
  config.lpCallbackData     = 0;

  config.pszMainInstruction = L"RivaTuner Statistics Server Incompatibility";

  wchar_t wszFooter [1024];


  // Make the following dialog the ONLY thing the process is doing
  std::queue <DWORD> suspended_tids =
    SK_SuspendAllOtherThreads ();

  // Delay triggers are invalid, but we can do nothing about it due to
  //   privilige issues.
  if (! SK_IsAdmin ()) {
    config.pszMainIcon        = TD_WARNING_ICON;
    config.pszContent         = L"RivaTuner Statistics Server requires a 10 second injection delay to workaround "
                                L"compatibility issues.";

    config.pszFooterIcon      = TD_SHIELD_ICON;
    config.pszFooter          = L"This can be fixed by starting the game as Admin once.";

    config.pszVerificationText = L"Check here if you do not care (risky).";

    BOOL verified;

    TaskDialogIndirect (&config, nullptr, nullptr, &verified);

    if (verified)
      ::config.system.ignore_rtss_delay = true;
    else
      ExitProcess (0);
  } else {
    config.pszMainIcon        = TD_INFORMATION_ICON;

    config.pszContent         = L"RivaTuner Statistics Server requires a 10 second injection delay to workaround "
                                L"compatibility issues.";

    config.dwCommonButtons    = TDCBF_YES_BUTTON | TDCBF_NO_BUTTON;
    config.nDefaultButton     = IDNO;

    wsprintf ( wszFooter,

                L"\r\n\r\n"

                L"Proposed Changes\r\n\r\n"

                L"<A HREF=\"%s\">%s</A>\r\n\r\n"

                L"[Hooking]\r\n"
                L"InjectionDelay=%s\r\n"
                L"InjectionDelayTriggers=%s",

                  wszRTSSHooks, wszRTSSHooks,
                    rtss_global.get_section (L"Hooking").get_value (L"InjectionDelay").c_str (),
                      rtss_global.get_section (L"Hooking").get_value (L"InjectionDelayTriggers").c_str () );

    config.pszExpandedInformation = wszFooter;
    config.pszExpandedControlText = L"Apply Proposed Config Changes?";

    int nButton;

    TaskDialogIndirect (&config, &nButton, nullptr, nullptr);

    if (nButton == IDYES) {
      // Delay triggers are invalid, and we are going to fix them...
      dll_log.Log ( L"[RTSSCompat] NEW Global Profile:  InjectDelay=%s,  DelayTriggers=%s",
                      rtss_global.get_section (L"Hooking").get_value (L"InjectionDelay").c_str (),
                        rtss_global.get_section (L"Hooking").get_value (L"InjectionDelayTriggers").c_str () );

      rtss_global.write (wszRTSSHooks);

      ShellExecute ( GetDesktopWindow (),
                      L"OPEN",
                        SK_GetHostApp ().c_str (),
                          NULL,
                            NULL,
                              SW_SHOWDEFAULT );

      ExitProcess (0);
    }
  }

  warned = TRUE;

  SK_ResumeThreads (suspended_tids);

  return TRUE;
}