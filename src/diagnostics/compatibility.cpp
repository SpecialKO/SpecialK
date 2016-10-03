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
#include <process.h>

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

typedef BOOL (WINAPI *FreeLibrary_pfn)(HMODULE hLibModule);

FreeLibrary_pfn FreeLibrary_Original = nullptr;

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

extern HMODULE __stdcall SK_GetDLL (void);

extern void WINAPI SK_HookGL     (void);
extern void WINAPI SK_HookVulkan (void);
extern void WINAPI SK_HookD3D9   (void);
extern void WINAPI SK_HookDXGI   (void);

#include <Shlwapi.h>
#pragma comment (lib, "Shlwapi.lib")

extern bool __stdcall SK_IsInjected (void);

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
    if ( StrStrIW (lpFileName, L"ltc_game") ) {
      if (config.compatibility.disable_raptr) {
        if (! (SK_LoadLibrary_SILENCE))
          dll_log.Log (L"[Black List] Preventing Raptr's overlay (ltc_game), it likes to crash games!");
        return TRUE;
      }

      //dll_log.Log (L"[PlaysTVFix] Delaying Raptr overlay until window is in foreground...");
      //CreateThread (nullptr, 0, SK_DelayLoadThreadW_FOREGROUND, _wcsdup (lpFileName), 0x00, nullptr);
    }

    else
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
SK_BootD3D9 (void)
{
  static volatile ULONG __booted = FALSE;

  if (InterlockedCompareExchange (&__booted, TRUE, FALSE))
    return;

  dll_log.Log (L"[API Detect]  <!> [ Bootstrapping Direct3D 9 (d3d9.dll) ] <!>");

  SK_HookD3D9 ();
}

void
SK_BootDXGI (void)
{
  static volatile ULONG __booted = FALSE;

  if (InterlockedCompareExchange (&__booted, TRUE, FALSE))
    return;

  dll_log.Log (L"[API Detect]  <!> [    Bootstrapping DXGI (dxgi.dll)    ] <!>");

  SK_HookDXGI ();
}

void
SK_BootOpenGL (void)
{
  static volatile ULONG __booted = FALSE;

  if (InterlockedCompareExchange (&__booted, TRUE, FALSE))
    return;

  dll_log.Log (L"[API Detect]  <!> [ Bootstrapping OpenGL (OpenGL32.dll) ] <!>");

  SK_HookGL ();
}

void
SK_BootVulkan (void)
{
  static volatile ULONG __booted = FALSE;

  if (InterlockedCompareExchange (&__booted, TRUE, FALSE))
    return;

  dll_log.Log (L"[API Detect]  <!> [ Bootstrapping Vulkan 1.x (vulkan-1.dll) ] <!>");

  SK_HookVulkan ();
}


void
__stdcall
SK_TraceLoadLibraryA ( HMODULE hCallingMod,
                       LPCSTR  lpFileName,
                       LPCSTR  lpFunction )
{
  wchar_t wszModName [MAX_PATH] = { L'\0' };
  wcsncpy (wszModName, SK_GetModuleName (hCallingMod).c_str (), MAX_PATH);

  if (! SK_LoadLibrary_SILENCE) {
    dll_log.Log ( "[DLL Loader]   ( %-28ls ) loaded '%#64s' <%s>",
                    wszModName,
                      lpFileName,
                        lpFunction );
  }

  // This is silly, this many string comparions per-load is
  //   not good. Hash the string and compare it in the future.
  if ( StrStrIW (wszModName, L"gameoverlayrenderer") ||
       StrStrIW (wszModName, L"RTSSHooks") ||
       StrStrIW (wszModName, L"GeDoSaTo") ) {
    if (config.compatibility.rehook_loadlibrary)
      SK_ReHookLoadLibrary ();
  }

  else if (hCallingMod != SK_GetDLL ()) {
    bool         self_load    = (hCallingMod == SK_GetDLL ());
    std::wstring calling_name = SK_GetModuleName (hCallingMod);

         if ( (! (SK_GetDLLRole () & DLL_ROLE::D3D9)) &&
              ( StrStrIA (lpFileName,             "d3d9.dll") ||
                StrStrIW (calling_name.c_str (), L"d3d9.dll") )  )
      SK_BootD3D9   ();
    else if ( (! (SK_GetDLLRole () & DLL_ROLE::DXGI)) &&
              ( StrStrIA (lpFileName,             "dxgi.dll") ||
                StrStrIW (calling_name.c_str (), L"dxgi.dll") ))
      SK_BootDXGI   ();
    else if (  (! (SK_GetDLLRole () & DLL_ROLE::OpenGL)) &&
              ( StrStrIA (lpFileName,             "OpenGL32.dll") ||
                StrStrIW (calling_name.c_str (), L"OpenGL32.dll") ))
      SK_BootOpenGL ();
    else if (   StrStrIA (lpFileName,             "vulkan-1.dll") ||
                StrStrIW (calling_name.c_str (), L"vulkan-1.dll")  )
      SK_BootVulkan ();


    if (StrStrIA (lpFileName, "CSteamworks.dll")) {
      extern void SK_HookCSteamworks (void);
      SK_HookCSteamworks ();
    } else if (StrStrIA (lpFileName, "steam_api.dll")   ||
               StrStrIA (lpFileName, "steam_api64.dll") ||
               StrStrIA (lpFileName, "SteamNative.dll") ) {
      extern void SK_HookSteamAPI (void);
      SK_HookSteamAPI ();
    }
  }

  // Some software repeatedly loads and unloads this, which can
  //   cause TLS-related problems if left unchecked... just leave
  //     the damn thing loaded permanently!
  if (StrStrIA (lpFileName, "d3dcompiler_")) {
    HMODULE hModDontCare;
    GetModuleHandleExA ( GET_MODULE_HANDLE_EX_FLAG_PIN,
                           lpFileName,
                             &hModDontCare );
  }
}

void
SK_TraceLoadLibraryW ( HMODULE hCallingMod,
                       LPCWSTR lpFileName,
                       LPCWSTR lpFunction )
{
  wchar_t wszModName [MAX_PATH] = { L'\0' };
  wcsncpy (wszModName, SK_GetModuleName (hCallingMod).c_str (), MAX_PATH);

  if (! SK_LoadLibrary_SILENCE) {
    dll_log.Log ( L"[DLL Loader]   ( %-28s ) loaded '%#64s' <%s>",
                    wszModName,
                      lpFileName,
                        lpFunction );
  }

  // This is silly, this many string comparions per-load is
  //   not good. Hash the string and compare it in the future.
  if ( StrStrIW (wszModName, L"gameoverlayrenderer") ||
       StrStrIW (wszModName, L"RTSSHooks") ||
       StrStrIW (wszModName, L"GeDoSaTo") ) {
    if (config.compatibility.rehook_loadlibrary)
      SK_ReHookLoadLibrary ();
  }

  else {
    bool         self_load    = (hCallingMod == SK_GetDLL ());
    std::wstring calling_name = SK_GetModuleName (hCallingMod);

         if ( (! (SK_GetDLLRole () & DLL_ROLE::D3D9)) &&
              ( StrStrIW (lpFileName,            L"d3d9.dll") ||
                StrStrIW (calling_name.c_str (), L"d3d9.dll") )  )
      SK_BootD3D9   ();
    else if ( (! (SK_GetDLLRole () & DLL_ROLE::DXGI)) &&
              ( StrStrIW (lpFileName,            L"dxgi.dll") ||
                StrStrIW (calling_name.c_str (), L"dxgi.dll") ))
      SK_BootDXGI   ();
    else if (  (! (SK_GetDLLRole () & DLL_ROLE::OpenGL)) &&
              ( StrStrIW (lpFileName,            L"OpenGL32.dll") ||
                StrStrIW (calling_name.c_str (), L"OpenGL32.dll") ))
      SK_BootOpenGL ();
    else if ( StrStrIW (lpFileName,            L"vulkan-1.dll") ||
              StrStrIW (calling_name.c_str (), L"vulkan-1.dll")  )
      SK_BootVulkan ();

    if (StrStrIW (lpFileName, L"CSteamworks.dll")) {
      extern void SK_HookCSteamworks (void);
      SK_HookCSteamworks ();
    } else if (StrStrIW (lpFileName, L"steam_api.dll")   ||
               StrStrIW (lpFileName, L"steam_api64.dll") ||
               StrStrIW (lpFileName, L"SteamNative.dll") ) {
      extern void SK_HookSteamAPI (void);
      SK_HookSteamAPI ();
    }
  }

  // Some software repeatedly loads and unloads this, which can
  //   cause TLS-related problems if left unchecked... just leave
  //     the damn thing loaded permanently!
  if (StrStrIW (lpFileName, L"d3dcompiler_")) {
    HMODULE hModDontCare;
    GetModuleHandleExW ( GET_MODULE_HANDLE_EX_FLAG_PIN,
                           lpFileName,
                             &hModDontCare );
  }
}



BOOL
WINAPI
FreeLibrary_Detour (HMODULE hLibModule)
{
//  bool specialk_free = (SK_GetCallingDLL () == SK_GetDLL ());

//  if (specialk_free)
//    return FreeLibrary_Original (hLibModule);

  std::wstring free_name = SK_GetModuleName (hLibModule);

  if (! SK_Path_wcsicmp (free_name.c_str (), L"dbghelp.dll"))
    return false;

#if 0
  // BLACKLIST FreeLibrary for VERY important DLLs
  if (
#ifdef _WIN32
       (! lstrcmpiW (free_name.c_str (), L"steam_api.dll"))   ||
       (! lstrcmpiW (free_name.c_str (), L"nvapi.dll"))       ||
       (! lstrcmpiW (free_name.c_str (), L"SpecialK32.dll"))  ||
#else
       (! lstrcmpiW (free_name.c_str (), L"steam_api64.dll")) ||
       (! lstrcmpiW (free_name.c_str (), L"SpecialK64.dll"))  ||
       (! lstrcmpiW (free_name.c_str (), L"nvapi64.dll"))     ||
#endif
       (! lstrcmpiW (free_name.c_str (), L"d3d11.dll"))       ||
       (! lstrcmpiW (free_name.c_str (), L"d3d9.dll"))        ||
       (! lstrcmpiW (free_name.c_str (), L"dxgi.dll"))        ||
       (! lstrcmpiW (free_name.c_str (), L"OpenGL32.dll"))
    )
    return FALSE;
#endif

  BOOL bRet = FreeLibrary_Original (hLibModule);

  if (bRet && GetModuleHandle (free_name.c_str ()) == nullptr) {
    dll_log.Log ( L"[DLL Loader]   ( %-28ls ) freed  '%#64ls'",
                    SK_GetModuleName (SK_GetCallingDLL ()).c_str (),
                      free_name.c_str () );
  }

  return bRet;
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

  if (hModEarly != hMod) {
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

  if (hModEarly != hMod) {
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
                                (dwFlags & LOAD_LIBRARY_AS_IMAGE_RESOURCE))) ) {
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
                                (dwFlags & LOAD_LIBRARY_AS_IMAGE_RESOURCE))) ) {
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

  LPVOID FreeLibrary_target    = nullptr;
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

  MH_QueueEnableHook (_loader_hooks.LoadLibraryA_target);


  if (_loader_hooks.LoadLibraryW_target != nullptr) {
    SK_RemoveHook (_loader_hooks.LoadLibraryW_target);
    _loader_hooks.LoadLibraryW_target = nullptr;
  }

  SK_CreateDLLHook ( L"kernel32.dll", "LoadLibraryW",
                     LoadLibraryW_Detour,
           (LPVOID*)&LoadLibraryW_Original,
                    &_loader_hooks.LoadLibraryW_target );

  MH_QueueEnableHook (_loader_hooks.LoadLibraryW_target);


  if (_loader_hooks.LoadLibraryExA_target != nullptr) {
    SK_RemoveHook (_loader_hooks.LoadLibraryExA_target);
    _loader_hooks.LoadLibraryExA_target = nullptr;
  }

  SK_CreateDLLHook ( L"kernel32.dll", "LoadLibraryExA",
                     LoadLibraryExA_Detour,
           (LPVOID*)&LoadLibraryExA_Original,
                    &_loader_hooks.LoadLibraryExA_target );

  MH_QueueEnableHook (_loader_hooks.LoadLibraryExA_target);


  if (_loader_hooks.LoadLibraryExW_target != nullptr) {
    SK_RemoveHook (_loader_hooks.LoadLibraryExW_target);
    _loader_hooks.LoadLibraryExW_target = nullptr;
  }

  SK_CreateDLLHook ( L"kernel32.dll", "LoadLibraryExW",
                     LoadLibraryExW_Detour,
           (LPVOID*)&LoadLibraryExW_Original,
                    &_loader_hooks.LoadLibraryExW_target );

  MH_QueueEnableHook (_loader_hooks.LoadLibraryExW_target);


#if 0
  if (_loader_hooks.FreeLibrary_target != nullptr) {
    SK_RemoveHook (_loader_hooks.FreeLibrary_target);
    _loader_hooks.FreeLibrary_target = nullptr;
  }

  SK_CreateDLLHook ( L"kernel32.dll", "FreeLibrary",
                     FreeLibrary_Detour,
           (LPVOID*)&FreeLibrary_Original,
                    &_loader_hooks.FreeLibrary_target );

  MH_QueueEnableHook (_loader_hooks.FreeLibrary_target);
#endif

  MH_ApplyQueued ();
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

  memset (&_loader_hooks, 0, sizeof sk_loader_hooks_t);
  SK_ReHookLoadLibrary ();
}

void
__stdcall
EnumLoadedModules (void)
{
  static bool loaded_gl     = false;
  static bool loaded_vulkan = false;
  static bool loaded_d3d9   = false;
  static bool loaded_dxgi   = false;

  // Begin logging new loads after this
  SK_LoadLibrary_SILENCE = false;

  iSK_Logger* pLogger = SK_CreateLog (L"logs/preloads.log");

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

          if (config.compatibility.rehook_loadlibrary) {
            SK_ReHookLoadLibrary ();
            Sleep (16);
          }
        }

        else if ( (! third_party_dlls.overlays.steam_overlay) &&
                   StrStrIW (wszModName, L"gameoverlayrenderer") ) {
          // Hold a reference to this DLL so it is not unloaded prematurely
          GetModuleHandleEx ( 0x00,
                                wszModName,
                                  &third_party_dlls.overlays.steam_overlay );

          if (config.compatibility.rehook_loadlibrary) {
            SK_ReHookLoadLibrary ();
            Sleep (16);
          }
        }

        else if ( (! third_party_dlls.misc.gedosato) &&
                   StrStrIW (wszModName, L"GeDoSaTo") ) {
          // Hold a reference to this DLL so it is not unloaded prematurely
          GetModuleHandleEx ( 0x00,
                                wszModName,
                                  &third_party_dlls.misc.gedosato );

          if (config.compatibility.rehook_loadlibrary) {
            SK_ReHookLoadLibrary ();
            Sleep (16);
          }
        }

        else if ( StrStrIW (wszModName, L"ltc_help") && (! (config.compatibility.ignore_raptr || config.compatibility.disable_raptr)) ) {
          static bool warned = false;

          // When Raptr's in full effect, it has its own overlay plus PlaysTV ...
          //   only warn about ONE of these things!
          if (! warned) {
            warned = true;

            extern DWORD __stdcall SK_RaptrWarn (LPVOID user);
            CreateThread ( nullptr, 0, SK_RaptrWarn, nullptr, 0x00, nullptr );
          }
        }

        else if ( StrStrIW (wszModName, L"\\opengl32.dll") && (SK_IsInjected () || (! (SK_GetDLLRole () & DLL_ROLE::OpenGL)))) {
          SK_BootOpenGL ();

          loaded_gl = true;
        }

        else if ( StrStrIW (wszModName, L"\\vulkan-1.dll") && (SK_IsInjected () || (! (SK_GetDLLRole () & DLL_ROLE::Vulkan)))) {
          SK_BootVulkan ();

          loaded_vulkan = true;
        }

        else if ( StrStrIW (wszModName, L"\\dxgi.dll") && (SK_IsInjected () || (! (SK_GetDLLRole () & DLL_ROLE::DXGI)))) {
          SK_BootDXGI ();

          loaded_dxgi = true;
        }

        else if ( StrStrIW (wszModName, L"\\d3d9.dll") && (SK_IsInjected () || (! (SK_GetDLLRole () & DLL_ROLE::D3D9)))) {
          SK_BootD3D9 ();

          loaded_d3d9 = true;
        }

        else if (StrStrIW (wszModName, L"CSteamworks.dll")) {
          extern void SK_HookCSteamworks (void);
          SK_HookCSteamworks ();
        }

        else if (StrStrIW (wszModName, L"steam_api.dll")   ||
                   StrStrIW (wszModName, L"steam_api64.dll") ||
                   StrStrIW (wszModName, L"SteamNative.dll") ) {
          extern void SK_HookSteamAPI (void);
          SK_HookSteamAPI ();
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
    // RTSS is in High App Detection or Stealth Mode
    //
    //   The software is probably going to crash.
    dll_log.Log ( L"[RTSSCompat] RTSS appears to be in High App Detection or Stealth mode, "
                  L"your game is probably going to crash." );
  }

  bool imports_gl     = false,
       imports_vulkan = false,
       imports_d3d9   = false,
       imports_dxgi   = false;

  SK_TestRenderImports ( GetModuleHandle (nullptr),
                           &imports_gl,
                             &imports_vulkan,
                               &imports_d3d9,
                                 &imports_dxgi );

  if ( (! loaded_gl) && imports_gl && (SK_IsInjected () || (! (SK_GetDLLRole () & DLL_ROLE::OpenGL)))) {
    SK_BootOpenGL ();
  }

  if ( (! loaded_vulkan) && imports_vulkan && (SK_IsInjected () || (! (SK_GetDLLRole () & DLL_ROLE::Vulkan)))) {
    SK_BootVulkan ();
  }

  if ( (! loaded_dxgi) && imports_dxgi && (SK_IsInjected () || (! (SK_GetDLLRole () & DLL_ROLE::DXGI)))) {
    SK_BootDXGI ();
  }

  if ( (! loaded_d3d9) && imports_d3d9 && (SK_IsInjected () || (! (SK_GetDLLRole () & DLL_ROLE::D3D9)))) {
    SK_BootD3D9 ();
  }
}


#include <Commctrl.h>
#include <comdef.h>

extern volatile LONG SK_bypass_dialog_active;
                HWND SK_bypass_dialog_hwnd;

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
  if (uNotification == TDN_TIMER) {
    DWORD dwProcId;
    DWORD dwThreadId =
      GetWindowThreadProcessId (GetForegroundWindow (), &dwProcId);

    if (dwProcId == GetCurrentProcessId ()) {
      extern DWORD WINAPI SK_RealizeForegroundWindow (HWND);
      SK_RealizeForegroundWindow (SK_bypass_dialog_hwnd);
    }
  }

  if (uNotification == TDN_HYPERLINK_CLICKED) {
    ShellExecuteW (nullptr, L"open", (wchar_t *)lParam, nullptr, nullptr, SW_SHOW);
    return S_OK;
  }

  if (uNotification == TDN_DIALOG_CONSTRUCTED) {
    while (InterlockedCompareExchange (&SK_bypass_dialog_active, 0, 0))
      Sleep (10);

    InterlockedIncrementAcquire (&SK_bypass_dialog_active);
  }

  if (uNotification == TDN_CREATED) {
    SK_bypass_dialog_hwnd = hWnd;

    extern DWORD WINAPI SK_RealizeForegroundWindow (HWND);
    SK_RealizeForegroundWindow (SK_bypass_dialog_hwnd);
  }

  if (uNotification == TDN_DESTROYED) {
    InterlockedDecrementRelease (&SK_bypass_dialog_active);
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
  TASKDIALOGCONFIG  task_config    = {0};

  int idx = 0;

  task_config.cbSize             = sizeof (task_config);
  task_config.hInstance          = GetModuleHandle (nullptr);
  task_config.hwndParent         = GetForegroundWindow ();
  task_config.pszWindowTitle     = L"Special K Compatibility Layer";
  task_config.dwCommonButtons    = TDCBF_OK_BUTTON;
  task_config.pButtons           = nullptr;
  task_config.cButtons           = 0;
  task_config.dwFlags            = TDF_ENABLE_HYPERLINKS;
  task_config.pfCallback         = TaskDialogCallback;
  task_config.lpCallbackData     = 0;

  task_config.pszMainInstruction = L"RivaTuner Statistics Server Incompatibility";

  wchar_t wszFooter [1024];

  // Delay triggers are invalid, but we can do nothing about it due to
  //   privilige issues.
  if (! SK_IsAdmin ()) {
    task_config.pszMainIcon        = TD_WARNING_ICON;
    task_config.pszContent         = L"RivaTuner Statistics Server requires a 10 second injection delay to workaround "
                                     L"compatibility issues.";

    task_config.pszFooterIcon      = TD_SHIELD_ICON;
    task_config.pszFooter          = L"This can be fixed by starting the game as Admin once.";

    task_config.pszVerificationText = L"Check here if you do not care (risky).";

    BOOL verified;

    TaskDialogIndirect (&task_config, nullptr, nullptr, &verified);

    if (verified)
      config.system.ignore_rtss_delay = true;
    else
      ExitProcess (0);
  } else {
    task_config.pszMainIcon        = TD_INFORMATION_ICON;

    task_config.pszContent         = L"RivaTuner Statistics Server requires a 10 second injection delay to workaround "
                                     L"compatibility issues.";

    task_config.dwCommonButtons    = TDCBF_YES_BUTTON | TDCBF_NO_BUTTON;
    task_config.nDefaultButton     = IDNO;

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

    task_config.pszExpandedInformation = wszFooter;
    task_config.pszExpandedControlText = L"Apply Proposed Config Changes?";

    int nButton;

    TaskDialogIndirect (&task_config, &nButton, nullptr, nullptr);

    if (nButton == IDYES) {
      // Delay triggers are invalid, and we are going to fix them...
      dll_log.Log ( L"[RTSSCompat] NEW Global Profile:  InjectDelay=%s,  DelayTriggers=%s",
                      rtss_global.get_section (L"Hooking").get_value (L"InjectionDelay").c_str (),
                        rtss_global.get_section (L"Hooking").get_value (L"InjectionDelayTriggers").c_str () );

      rtss_global.write  (wszRTSSHooks);

      STARTUPINFO         sinfo = { 0 };
      PROCESS_INFORMATION pinfo = { 0 };

      sinfo.cb          = sizeof STARTUPINFO;
      sinfo.dwFlags     = STARTF_USESHOWWINDOW;
      sinfo.wShowWindow = SW_SHOWDEFAULT;

      CreateProcess (
        nullptr,
          (LPWSTR)SK_GetHostApp (),
            nullptr, nullptr,
              TRUE,
                CREATE_SUSPENDED,
                  nullptr, nullptr,
                    &sinfo, &pinfo );

      ResumeThread     (pinfo.hThread);
      TerminateProcess (GetCurrentProcess (), 0x00);
    }
  }

  warned = TRUE;

  return TRUE;
}

HRESULT
__stdcall
SK_TaskBoxWithConfirm ( wchar_t* wszMainInstruction,
                        PCWSTR   wszMainIcon,
                        wchar_t* wszContent,
                        wchar_t* wszConfirmation,
                        wchar_t* wszFooter,
                        PCWSTR   wszFooterIcon,
                        wchar_t* wszVerifyText,
                        BOOL*    verify )
{
  bool timer = true;

  int               nButtonPressed = 0;
  TASKDIALOGCONFIG  task_config    = {0};

  int idx = 0;

  HWND hWndForeground = GetForegroundWindow ();

  task_config.cbSize             = sizeof (task_config);
  task_config.hInstance          = GetModuleHandle (nullptr);
  task_config.hwndParent         = hWndForeground;
  task_config.pszWindowTitle     = L"Special K Compatibility Layer";
  task_config.dwCommonButtons    = TDCBF_OK_BUTTON;
  task_config.pButtons           = nullptr;
  task_config.cButtons           = 0;
  task_config.dwFlags            = 0x00;
  task_config.pfCallback         = TaskDialogCallback;
  task_config.lpCallbackData     = 0;

  task_config.pszMainInstruction = wszMainInstruction;

  task_config.hwndParent = GetDesktopWindow ();

  task_config.pszMainIcon        = wszMainIcon;
  task_config.pszContent         = wszContent;

  task_config.pszFooterIcon      = wszFooterIcon;
  task_config.pszFooter          = wszFooter;

  task_config.pszVerificationText = wszVerifyText;

  if (verify != nullptr && *verify)
    task_config.dwFlags |= TDF_VERIFICATION_FLAG_CHECKED;

  if (timer)
    task_config.dwFlags |= TDF_CALLBACK_TIMER;

  HRESULT hr =
    TaskDialogIndirect ( &task_config,
                          &nButtonPressed,
                            nullptr,
                              verify );

  return hr;
}

HRESULT
__stdcall
SK_TaskBoxWithConfirmEx ( wchar_t* wszMainInstruction,
                          PCWSTR   wszMainIcon,
                          wchar_t* wszContent,
                          wchar_t* wszConfirmation,
                          wchar_t* wszFooter,
                          PCWSTR   wszFooterIcon,
                          wchar_t* wszVerifyText,
                          BOOL*    verify,
                          wchar_t* wszCommand )
{
  bool timer = true;

  int               nButtonPressed = 0;
  TASKDIALOGCONFIG  task_config    = {0};

  int idx = 0;

  HWND hWndForeground = GetForegroundWindow ();

  task_config.cbSize             = sizeof (task_config);
  task_config.hInstance          = GetModuleHandle (nullptr);
  task_config.hwndParent         = hWndForeground;
  task_config.pszWindowTitle     = L"Special K Compatibility Layer";
  task_config.dwCommonButtons    = TDCBF_OK_BUTTON;

  TASKDIALOG_BUTTON button;
  button.nButtonID     = 0xdead01ae;
  button.pszButtonText = wszCommand;

  task_config.pButtons           = &button;
  task_config.cButtons           = 1;

  task_config.dwFlags            = 0x00;
  task_config.dwFlags           |= TDF_USE_COMMAND_LINKS;

  task_config.pfCallback         = TaskDialogCallback;
  task_config.lpCallbackData     = 0;

  task_config.pszMainInstruction = wszMainInstruction;

  task_config.hwndParent = GetDesktopWindow ();

  task_config.pszMainIcon        = wszMainIcon;
  task_config.pszContent         = wszContent;

  task_config.pszFooterIcon      = wszFooterIcon;
  task_config.pszFooter          = wszFooter;

  task_config.pszVerificationText = wszVerifyText;

  if (verify != nullptr && *verify)
    task_config.dwFlags |= TDF_VERIFICATION_FLAG_CHECKED;

  if (timer)
    task_config.dwFlags |= TDF_CALLBACK_TIMER;

  HRESULT hr =
    TaskDialogIndirect ( &task_config,
                          &nButtonPressed,
                            nullptr,
                              verify );

  if (nButtonPressed == 0xdead01ae) {
    config.compatibility.disable_raptr = true;
  }

  return hr;
}

DWORD
WINAPI
SK_RaptrWarn (LPVOID user)
{
  // Don't check for Raptr while installing something...
  if (! SK_Path_wcsicmp (SK_GetHostApp (), L"SKIM.exe")) {
    CloseHandle (GetCurrentThread ());
    return 0;
  }

  HRESULT
  __stdcall
  SK_TaskBoxWithConfirmEx ( wchar_t* wszMainInstruction,
                            PCWSTR   wszMainIcon,
                            wchar_t* wszContent,
                            wchar_t* wszConfirmation,
                            wchar_t* wszFooter,
                            PCWSTR   wszFooterIcon,
                            wchar_t* wszVerifyText,
                            BOOL*    verify,
                            wchar_t* wszCommand );

  SK_TaskBoxWithConfirmEx ( L"AMD Gaming Evolved or Raptr is running",
                            TD_WARNING_ICON,
                            L"In some software you can expect weird things to happen, including"
                            L" the game mysteriously disappearing.\n\n"
                            L"If the game behaves strangely, you may need to disable it.",
                            nullptr,
                            nullptr,
                            nullptr,
                            L"Check here to ignore this warning in the future.",
                            (BOOL *)&config.compatibility.ignore_raptr,
                            L"Disable Raptr / Plays.TV\n\n"
                            L"Special K will disable it (for this game)." );

  CloseHandle (GetCurrentThread ());

  return 0;
}

unsigned int
__stdcall
SK_Bypass_CRT (LPVOID user)
{
  wchar_t wszBlacklist [MAX_PATH] = { L'\0' };

  lstrcatW (wszBlacklist, L"SpecialK.deny.");
  lstrcatW (wszBlacklist, SK_GetHostApp ());

  wchar_t* pwszDot = wcsrchr (wszBlacklist, L'.');

  if (pwszDot != nullptr)
    *pwszDot = L'\0';

  BOOL disable = 
    (GetFileAttributesW (wszBlacklist) != INVALID_FILE_ATTRIBUTES);

  HRESULT hr =
  SK_TaskBoxWithConfirm ( L"Special K Injection Compatibility Options",
                          TD_SHIELD_ICON,
                          L"By pressing Ctrl + Shift at application start, you"
                          L" have opted into compatibility mode.\n\nUse the"
                          L" menu options provided to troubleshoot problems"
                          L" that may be caused by the mod.",

                          nullptr,//L"Check here to DISABLE Special K for this game.",

                          L"You can re-enable auto-injection at any time by "
                          L"holding Ctrl + Shift down at startup.",
                          TD_INFORMATION_ICON,

                          L"Check here to DISABLE Special K for this game.",
                          &disable );

  if (SUCCEEDED (hr)) {
    if (disable) {
      FILE* fDeny = _wfopen (wszBlacklist, L"w");

      if (fDeny) {
        fputc  ('\0', fDeny);
        fflush (      fDeny);
        fclose (      fDeny);
      }
    } else {
      DeleteFileW (wszBlacklist);
    }
  }


  STARTUPINFO         sinfo = { 0 };
  PROCESS_INFORMATION pinfo = { 0 };

  sinfo.cb          = sizeof STARTUPINFO;
  sinfo.dwFlags     = STARTF_USESHOWWINDOW;
  sinfo.wShowWindow = SW_SHOWDEFAULT;

  CreateProcess (
    nullptr,
      (LPWSTR)SK_GetHostApp (),
        nullptr, nullptr,
          TRUE,
            CREATE_SUSPENDED,
              nullptr, nullptr,
                &sinfo, &pinfo );

  ResumeThread     (pinfo.hThread);
  TerminateProcess (GetCurrentProcess (), 0x00);


  return 0;
}

#include <process.h>

bool
__stdcall
SK_BypassInject (void)
{
  _beginthreadex ( nullptr, 0, SK_Bypass_CRT, nullptr, 0x00, nullptr );

  return TRUE;
}