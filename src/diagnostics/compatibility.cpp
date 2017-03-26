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

#include <Windows.h>
#include <SpecialK/diagnostics/compatibility.h>
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

#include <SpecialK/config.h>
#include <SpecialK/hooks.h>
#include <SpecialK/core.h>
#include <SpecialK/log.h>
#include <SpecialK/utility.h>
#include <SpecialK/steam_api.h>
#include <SpecialK/window.h>
#include <SpecialK/render_backend.h>


BOOL __stdcall SK_ValidateGlobalRTSSProfile (void);
void __stdcall SK_ReHookLoadLibrary         (void);
void           SK_UnhookLoadLibrary         (void);

bool SK_LoadLibrary_SILENCE = false;

#include <Shlwapi.h>
#pragma comment (lib, "Shlwapi.lib")

extern CRITICAL_SECTION loader_lock;

void
__stdcall
SK_LockDllLoader (void)
{
  if (config.system.strict_compliance)
    EnterCriticalSection (&loader_lock);
}

void
__stdcall
SK_UnlockDllLoader (void)
{
  if (config.system.strict_compliance)
    LeaveCriticalSection (&loader_lock);
}

BOOL
__stdcall
BlacklistLibraryW (LPCWSTR lpFileName)
{
  if ( StrStrIW (lpFileName, L"ltc_game") ) {
    if (config.compatibility.disable_raptr) {
      if (! (SK_LoadLibrary_SILENCE))
        dll_log.Log (L"[Black List] Preventing Raptr's overlay (ltc_game), it likes to crash games!");
      return TRUE;
    }

    //dll_log.Log (L"[PlaysTVFix] Delaying Raptr overlay until window is in foreground...");
    //CreateThread (nullptr, 0, SK_DelayLoadThreadW_FOREGROUND, _wcsdup (lpFileName), 0x00, nullptr);
  }

  if (config.compatibility.disable_nv_bloat)
  {
    static bool init = false;
    static std::vector <std::wstring> nv_blacklist;
    
    if (! init)
    {
      nv_blacklist.emplace_back (L"rxgamepadinput.dll");
      nv_blacklist.emplace_back (L"rxcore.dll");
      nv_blacklist.emplace_back (L"nvinject.dll");
      nv_blacklist.emplace_back (L"rxinput.dll");
#ifdef _WIN64
      nv_blacklist.emplace_back (L"nvspcap64.dll");
      nv_blacklist.emplace_back (L"nvSCPAPI64.dll");
#else
      nv_blacklist.emplace_back (L"nvspcap.dll");
      nv_blacklist.emplace_back (L"nvSCPAPI.dll");
#endif
      init = true;
    }
    
    for ( auto&& it : nv_blacklist )
    {
      if (StrStrIW (lpFileName, it.c_str ()))
      {
        HMODULE hModNV;

        //dll_log.Log ( L"[Black List] Disabling NVIDIA BloatWare ('%s'), so long we hardly knew ye', "
                                   //L"but you did bad stuff in a lot of games.",
                        //lpFileName );

        if (GetModuleHandleEx (GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, it.c_str(), &hModNV))
          FreeLibrary_Original (hModNV);

        return TRUE;
      }
    }
  }

  if (config.compatibility.disable_msi_deadlock)
  {
    static bool init = false;
    static std::vector <std::wstring> msi_blacklist;

    if (! init)
    {
      msi_blacklist.emplace_back (L"Nahimic2DevProps.dll");
      msi_blacklist.emplace_back (L"Nahimic2OSD.dll");
      init = true;
    }

    for ( auto&& it : msi_blacklist )
    {
      if (StrStrIW (lpFileName, it.c_str ()))
      {
        HMODULE hModMSI;

        dll_log.Log ( L"[Black List] Disabling MSI DeadlockWare ('%s'), so long we hardly knew ye', "
                                   L"but you deadlocked a lot of games.",
                        lpFileName );

        if (GetModuleHandleEx (GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, it.c_str(), &hModMSI))
          FreeLibrary_Original (hModMSI);

        return TRUE;
      }
    }
  }

  return FALSE;
}

BOOL
__stdcall
BlacklistLibraryA (LPCSTR lpFileName)
{
  wchar_t wszWideLibName [MAX_PATH + 2];
  memset (wszWideLibName, 9, sizeof (wchar_t) * (MAX_PATH + 2));

  MultiByteToWideChar (CP_OEMCP, 0x00, lpFileName, -1, wszWideLibName, MAX_PATH);

  return BlacklistLibraryW (wszWideLibName);
}


void
__stdcall
SK_TraceLoadLibraryA ( HMODULE hCallingMod,
                       LPCSTR  lpFileName,
                       LPCSTR  lpFunction,
                       LPVOID  lpCallerFunc )
{
  if (config.steam.preload_client)
  {
    if (StrStrIA (lpFileName, "gameoverlayrenderer64"))
    {
       char    szSteamPath             [MAX_PATH + 1] = { '\0' };
      strncpy (szSteamPath, lpFileName, MAX_PATH - 1);

      PathRemoveFileSpecA (szSteamPath);

#ifdef _WIN64
      lstrcatA (szSteamPath, "\\steamclient64.dll");
#else
      lstrcatA (szSteamPath, "\\steamclient.dll");
#endif

      LoadLibraryA (szSteamPath);

      HMODULE hModClient;
      GetModuleHandleExA ( GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT | GET_MODULE_HANDLE_EX_FLAG_PIN,
                           szSteamPath, &hModClient );
    }
  }


  wchar_t wszModName [MAX_PATH] = { L'\0' };
  wcsncpy (wszModName, SK_GetModuleName (hCallingMod).c_str (), MAX_PATH);

  if (! SK_LoadLibrary_SILENCE)
  {    
    char  szSymbol [1024] = { 0 };
    ULONG ulLen  =  1024;
    
    ulLen = SK_GetSymbolNameFromModuleAddr (SK_GetCallingDLL (lpCallerFunc), (uintptr_t)lpCallerFunc, szSymbol, ulLen);

    dll_log.Log ( "[DLL Loader]   ( %-28ls ) loaded '%#64s' <%s> { '%hs' }",
                    wszModName,
                      lpFileName,
                        lpFunction,
                          szSymbol );
  }

  // This is silly, this many string comparions per-load is
  //   not good. Hash the string and compare it in the future.
  if ( StrStrIW (wszModName, L"gameoverlayrenderer") ||
       StrStrIW (wszModName, L"RTSSHooks")           ||
       StrStrIW (wszModName, L"GeDoSaTo")            ||
       StrStrIW (wszModName, L"Nahimic2DevProps.dll") )
  {
    if (config.compatibility.rehook_loadlibrary)
      SK_ReHookLoadLibrary ();
  }

  if (hCallingMod != SK_GetDLL ()) {
#if 0
         if ( (! (SK_GetDLLRole () & DLL_ROLE::D3D9)) &&
              ( StrStrIA (lpFileName,  "d3d9.dll")    ||
                StrStrIW (wszModName, L"d3d9.dll")    ||
                                                      
                StrStrIA (lpFileName,  "d3dx9_")      ||
                StrStrIW (wszModName, L"d3dx9_")      ||

                // NVIDIA's User-Mode D3D Frontend
                StrStrIA (lpFileName,  "nvd3dum.dll") ||
                StrStrIW (wszModName, L"nvd3dum.dll")  ) )
      SK_BootD3D9   ();
#endif
#if 0
    else if ( (! (SK_GetDLLRole () & DLL_ROLE::DXGI)) &&
              ( StrStrIA (lpFileName,  "dxgi.dll") ||
                StrStrIW (wszModName, L"dxgi.dll") ))
      SK_BootDXGI   ();
#endif
        if (  (! (SK_GetDLLRole () & DLL_ROLE::OpenGL)) &&
              ( StrStrIA (lpFileName,  "OpenGL32.dll") ||
                StrStrIW (wszModName, L"OpenGL32.dll") ))
      SK_BootOpenGL ();
    else if (   StrStrIA (lpFileName,  "vulkan-1.dll") ||
                StrStrIW (wszModName, L"vulkan-1.dll")  )
      SK_BootVulkan ();

    if (! config.steam.silent) {
      if (StrStrIA (lpFileName, "CSteamworks.dll")) {
        SK_HookCSteamworks ();
      } else if (StrStrIA (lpFileName, "steam_api.dll")   ||
                 StrStrIA (lpFileName, "steam_api64.dll") ||
                 StrStrIA (lpFileName, "SteamNative.dll") ) {
        SK_HookSteamAPI ();
      }
    }

#if 0
    // Some software repeatedly loads and unloads this, which can
    //   cause TLS-related problems if left unchecked... just leave
    //     the damn thing loaded permanently!
    if (StrStrIA (lpFileName, "d3dcompiler_")) {
      HMODULE hModDontCare;
      GetModuleHandleExA ( GET_MODULE_HANDLE_EX_FLAG_PIN,
                             lpFileName,
                               &hModDontCare );
    }
#endif
  }
}

void
SK_TraceLoadLibraryW ( HMODULE hCallingMod,
                       LPCWSTR lpFileName,
                       LPCWSTR lpFunction,
                       LPVOID  lpCallerFunc )
{
  if (config.steam.preload_client) {
    if (StrStrIW (lpFileName, L"gameoverlayrenderer64"))
    {
       wchar_t wszSteamPath             [MAX_PATH + 1] = { L'\0' };
      wcsncpy (wszSteamPath, lpFileName, MAX_PATH - 1);

      PathRemoveFileSpec (wszSteamPath);

#ifdef _WIN64
      lstrcatW(wszSteamPath, L"\\steamclient64.dll");
#else
      lstrcatW(wszSteamPath, L"\\steamclient.dll");
#endif

      LoadLibraryW (wszSteamPath);

      HMODULE hModClient;
      GetModuleHandleEx ( GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT | GET_MODULE_HANDLE_EX_FLAG_PIN,
                          wszSteamPath, &hModClient );

      SK_ReHookLoadLibrary ();
    }
  }

  wchar_t wszModName [MAX_PATH] = { L'\0' };
  wcsncpy (wszModName, SK_GetModuleName (hCallingMod).c_str (), MAX_PATH);

  if (! SK_LoadLibrary_SILENCE)
  {
    char  szSymbol [1024] = { };
    ULONG ulLen  =  1024;
    
    ulLen = SK_GetSymbolNameFromModuleAddr (SK_GetCallingDLL (lpCallerFunc), (uintptr_t)lpCallerFunc, szSymbol, ulLen);

    dll_log.Log ( L"[DLL Loader]   ( %-28s ) loaded '%#64s' <%s> { '%hs' }",
                    wszModName,
                      lpFileName,
                        lpFunction,
                          szSymbol );
  }

  // This is silly, this many string comparions per-load is
  if ( StrStrIW (wszModName, L"gameoverlayrenderer") ||
  //   not good. Hash the string and compare it in the future.
       StrStrIW (wszModName, L"RTSSHooks")           ||
       StrStrIW (wszModName, L"GeDoSaTo")            ||
       StrStrIW (wszModName, L"Nahimic2DevProps.dll") )
  {
    if (config.compatibility.rehook_loadlibrary)
      SK_ReHookLoadLibrary ();
  }

  if (hCallingMod != SK_GetDLL ()) {
#if 0
       if ( (! (SK_GetDLLRole () & DLL_ROLE::D3D9)) &&
            ( StrStrIW (lpFileName, L"d3d9.dll")    ||
              StrStrIW (wszModName, L"d3d9.dll")    ||

              StrStrIW (lpFileName, L"d3dx9_")      ||
              StrStrIW (wszModName, L"d3dx9_")      ||

              // NVIDIA's User-Mode D3D Frontend
              StrStrIW (lpFileName, L"nvd3dum.dll") ||
              StrStrIW (wszModName, L"nvd3dum.dll")  ) )
      SK_BootD3D9   ();
#endif
#if 0
    else if ( (! (SK_GetDLLRole () & DLL_ROLE::DXGI)) &&
              ( StrStrIW (lpFileName, L"dxgi.dll") ||
                StrStrIW (wszModName, L"dxgi.dll") ))
      SK_BootDXGI   ();
#endif
        if (  (! (SK_GetDLLRole () & DLL_ROLE::OpenGL)) &&
              ( StrStrIW (lpFileName, L"OpenGL32.dll") ||
                StrStrIW (wszModName, L"OpenGL32.dll") ))
      SK_BootOpenGL ();
    else if ( StrStrIW (lpFileName, L"vulkan-1.dll") ||
              StrStrIW (wszModName, L"vulkan-1.dll")  )
      SK_BootVulkan ();
    
    if (! config.steam.silent) {
      if (StrStrIW (lpFileName, L"CSteamworks.dll")) {
        SK_HookCSteamworks ();
      } else if (StrStrIW (lpFileName, L"steam_api.dll")   ||
                 StrStrIW (lpFileName, L"steam_api64.dll") ||
                 StrStrIW (lpFileName, L"SteamNative.dll") ) {
        SK_HookSteamAPI ();
      }
    }
    
#if 0
    // Some software repeatedly loads and unloads this, which can
    //   cause TLS-related problems if left unchecked... just leave
    //     the damn thing loaded permanently!
    if (StrStrIW (lpFileName, L"d3dcompiler_")) {
      HMODULE hModDontCare;
      GetModuleHandleExW ( GET_MODULE_HANDLE_EX_FLAG_PIN,
                             lpFileName,
                               &hModDontCare );
    }
#endif
  }
}


extern volatile ULONG __SK_DLL_Ending;

BOOL
WINAPI
FreeLibrary_Detour (HMODULE hLibModule)
{
  if (InterlockedCompareExchange (&__SK_DLL_Ending, 0, 0) != 0)
  {
    return FreeLibrary_Original (hLibModule);
  }

  SK_LockDllLoader ();

  std::wstring name = SK_GetModuleName     (hLibModule);
  BOOL         bRet = FreeLibrary_Original (hLibModule);

  if ( (! (SK_LoadLibrary_SILENCE)) ||
           SK_GetModuleName (hLibModule).find (L"steam") != std::wstring::npos )
  {
    if ( SK_GetModuleName (hLibModule).find (L"steam") != std::wstring::npos || 
        (bRet && GetModuleHandle (name.c_str ()) == nullptr ) )
    {
      if (config.system.log_level > 2)
      {
        char  szSymbol [1024] = { };
        ULONG ulLen  =  1024;
    
        ulLen = SK_GetSymbolNameFromModuleAddr (SK_GetCallingDLL (), (uintptr_t)_ReturnAddress (), szSymbol, ulLen);

        dll_log.Log ( L"[DLL Loader]   ( %-28ls ) freed  '%#64ls' from { '%hs' }",
                        SK_GetModuleName (SK_GetCallingDLL ()).c_str (),
                          name.c_str (),
                            szSymbol
                    );
      }
    }
  }

  SK_UnlockDllLoader ();

  return bRet;
}

HMODULE
WINAPI
LoadLibraryA_Detour (LPCSTR lpFileName)
{
  if (lpFileName == nullptr)
    return NULL;

  SK_LockDllLoader ();

  HMODULE hModEarly = nullptr;

  __try {
    GetModuleHandleExA ( GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)lpFileName, &hModEarly );
  } 

  __except ( (GetExceptionCode () == EXCEPTION_INVALID_HANDLE) ?
                           EXCEPTION_EXECUTE_HANDLER :
                           EXCEPTION_CONTINUE_SEARCH  )
  {
    SetLastError (0);
  }

  if (hModEarly == nullptr && BlacklistLibraryA (lpFileName)) {
    SK_UnlockDllLoader ();
    return NULL;
  }

  HMODULE hMod = LoadLibraryA_Original (lpFileName);

  if (hModEarly != hMod) {
    SK_TraceLoadLibraryA ( SK_GetCallingDLL (),
                             lpFileName,
                               "LoadLibraryA", _ReturnAddress () );
  }

  SK_UnlockDllLoader ();
  return hMod;
}

HMODULE
WINAPI
LoadLibraryW_Detour (LPCWSTR lpFileName)
{
  if (lpFileName == nullptr)
    return NULL;

 SK_LockDllLoader ();

  HMODULE hModEarly = nullptr;

  __try {
    GetModuleHandleExW ( GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, lpFileName, &hModEarly );
  }
  __except ( (GetExceptionCode () == EXCEPTION_INVALID_HANDLE) ?
                           EXCEPTION_EXECUTE_HANDLER :
                           EXCEPTION_CONTINUE_SEARCH )
  {
    SetLastError (0);
  }

  if (hModEarly == nullptr&& BlacklistLibraryW (lpFileName)) {
    SK_UnlockDllLoader ();
    return NULL;
  }


  HMODULE hMod = LoadLibraryW_Original (lpFileName);

  if (hModEarly != hMod) {
    SK_TraceLoadLibraryW ( SK_GetCallingDLL (),
                             lpFileName,
                               L"LoadLibraryW", _ReturnAddress () );
  }

  SK_UnlockDllLoader ();
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

  SK_LockDllLoader ();

  if ((dwFlags & LOAD_LIBRARY_AS_DATAFILE) && (! BlacklistLibraryA (lpFileName))) {
    SK_UnlockDllLoader ();
    return LoadLibraryExA_Original (lpFileName, hFile, dwFlags);
  }

  HMODULE hModEarly = nullptr;

  __try {
    GetModuleHandleExA ( GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, lpFileName, &hModEarly );
  }
  __except ( (GetExceptionCode () == EXCEPTION_INVALID_HANDLE) ?
                           EXCEPTION_EXECUTE_HANDLER :
                           EXCEPTION_CONTINUE_SEARCH )
  {
    SetLastError (0);
  }

  if (hModEarly == NULL && BlacklistLibraryA (lpFileName))
  {
    SK_UnlockDllLoader ();
    return NULL;
  }

  HMODULE hMod = LoadLibraryExA_Original (lpFileName, hFile, dwFlags);

  if ( hModEarly != hMod && (! ((dwFlags & LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE) ||
                                (dwFlags & LOAD_LIBRARY_AS_IMAGE_RESOURCE))) ) {
    SK_TraceLoadLibraryA ( SK_GetCallingDLL (),
                             lpFileName,
                               "LoadLibraryExA", _ReturnAddress () );
  }

  SK_UnlockDllLoader ();
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

  SK_LockDllLoader ();

  if ((dwFlags & LOAD_LIBRARY_AS_DATAFILE) && (! BlacklistLibraryW (lpFileName))) {
    SK_UnlockDllLoader ();
    return LoadLibraryExW_Original (lpFileName, hFile, dwFlags);
  }

  HMODULE hModEarly = nullptr;

  __try {
    GetModuleHandleExW ( GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, lpFileName, &hModEarly );
  }
  __except ( (GetExceptionCode () == EXCEPTION_INVALID_HANDLE) ?
                           EXCEPTION_EXECUTE_HANDLER :
                           EXCEPTION_CONTINUE_SEARCH )
  {
    SetLastError (0);
  }

  if (hModEarly == NULL && BlacklistLibraryW (lpFileName)) {
    SK_UnlockDllLoader ();
    return NULL;
  }

  HMODULE hMod = LoadLibraryExW_Original (lpFileName, hFile, dwFlags);

  if ( hModEarly != hMod && (! ((dwFlags & LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE) ||
                                (dwFlags & LOAD_LIBRARY_AS_IMAGE_RESOURCE))) ) {
    SK_TraceLoadLibraryW ( SK_GetCallingDLL (),
                             lpFileName,
                               L"LoadLibraryExW", _ReturnAddress () );
  }

  SK_UnlockDllLoader ();
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

typedef HHOOK (WINAPI *SetWindowsHookEx_pfn)(
  _In_ int       idHook,
  _In_ HOOKPROC  lpfn,
  _In_ HINSTANCE hMod,
  _In_ DWORD     dwThreadId
);

SetWindowsHookEx_pfn SetWindowsHookExA_Original = nullptr;
SetWindowsHookEx_pfn SetWindowsHookExW_Original = nullptr;

HHOOK
WINAPI
SetWindowsHookExA_Detour (
  _In_ int       idHook,
  _In_ HOOKPROC  lpfn,
  _In_ HINSTANCE hMod,
  _In_ DWORD     dwThreadId
)
{
  //MessageBox (NULL, SK_GetModuleName (hMod).c_str (), L"Module", MB_OK);
  //dll_log.Log (L"[WindowsHook] Module: '%s'", SK_GetModuleName (hMod).c_str ());

  return 0;
  //return SetWindowsHookExA_Original (idHook, lpfn, hMod, dwThreadId);
}

HHOOK
WINAPI
SetWindowsHookExW_Detour (
  _In_ int       idHook,
  _In_ HOOKPROC  lpfn,
  _In_ HINSTANCE hMod,
  _In_ DWORD     dwThreadId
)
{
  //MessageBox (NULL, SK_GetModuleName (hMod).c_str (), L"Module", MB_OK);
  //dll_log.Log (L"[WindowsHook] Module: '%s'", SK_GetModuleName (hMod).c_str ());

  return 0;//return SetWindowsHookExW_Original (idHook, lpfn, hMod, dwThreadId);
}


void
__stdcall
SK_InitWindowsHookHook (void)
{
  static bool init_winhook = false;

  if (! init_winhook)
  {
    SK_CreateDLLHook ( L"user32.dll", "SetWindowsHookExA",
                       SetWindowsHookExA_Detour,
             (LPVOID*)&SetWindowsHookExA_Original );

    SK_CreateDLLHook ( L"user32.dll", "SetWindowsHookExW",
                       SetWindowsHookExW_Detour,
             (LPVOID*)&SetWindowsHookExW_Original );

   init_winhook = true;
  }
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
  //if (! config.system.trace_load_library)
    //return;

  if (_loader_hooks.unhooked)
    return;

  SK_LockDllLoader ();

  if (_loader_hooks.LoadLibraryA_target != nullptr) {
    SK_RemoveHook (_loader_hooks.LoadLibraryA_target);
    _loader_hooks.LoadLibraryA_target = nullptr;
  }

  SK_CreateDLLHook2 ( L"kernel32.dll", "LoadLibraryA",
                     LoadLibraryA_Detour,
           (LPVOID*)&LoadLibraryA_Original,
                    &_loader_hooks.LoadLibraryA_target );

  MH_QueueEnableHook (_loader_hooks.LoadLibraryA_target);


  if (_loader_hooks.LoadLibraryW_target != nullptr) {
    SK_RemoveHook (_loader_hooks.LoadLibraryW_target);
    _loader_hooks.LoadLibraryW_target = nullptr;
  }

  SK_CreateDLLHook2 ( L"kernel32.dll", "LoadLibraryW",
                     LoadLibraryW_Detour,
           (LPVOID*)&LoadLibraryW_Original,
                    &_loader_hooks.LoadLibraryW_target );

  MH_QueueEnableHook (_loader_hooks.LoadLibraryW_target);


  if (_loader_hooks.LoadLibraryExA_target != nullptr) {
    SK_RemoveHook (_loader_hooks.LoadLibraryExA_target);
    _loader_hooks.LoadLibraryExA_target = nullptr;
  }

  SK_CreateDLLHook2 ( L"kernel32.dll", "LoadLibraryExA",
                     LoadLibraryExA_Detour,
           (LPVOID*)&LoadLibraryExA_Original,
                    &_loader_hooks.LoadLibraryExA_target );

  MH_QueueEnableHook (_loader_hooks.LoadLibraryExA_target);


  if (_loader_hooks.LoadLibraryExW_target != nullptr) {
    SK_RemoveHook (_loader_hooks.LoadLibraryExW_target);
    _loader_hooks.LoadLibraryExW_target = nullptr;
  }

  SK_CreateDLLHook2 ( L"kernel32.dll", "LoadLibraryExW",
                     LoadLibraryExW_Detour,
           (LPVOID*)&LoadLibraryExW_Original,
                    &_loader_hooks.LoadLibraryExW_target );

  MH_QueueEnableHook (_loader_hooks.LoadLibraryExW_target);


  if (_loader_hooks.FreeLibrary_target != nullptr) {
    SK_RemoveHook (_loader_hooks.FreeLibrary_target);
    _loader_hooks.FreeLibrary_target = nullptr;
  }

  SK_CreateDLLHook2 ( L"kernel32.dll", "FreeLibrary",
                     FreeLibrary_Detour,
           (LPVOID*)&FreeLibrary_Original,
                    &_loader_hooks.FreeLibrary_target );

  MH_QueueEnableHook (_loader_hooks.FreeLibrary_target);

  MH_ApplyQueued ();

  SK_UnlockDllLoader ();
}

void
SK_UnhookLoadLibrary (void)
{
  SK_LockDllLoader ();

  _loader_hooks.unhooked = true;

  if (_loader_hooks.LoadLibraryA_target != nullptr)
    MH_QueueDisableHook (_loader_hooks.LoadLibraryA_target);
  if (_loader_hooks.LoadLibraryW_target != nullptr)
    MH_QueueDisableHook(_loader_hooks.LoadLibraryA_target);
  if (_loader_hooks.LoadLibraryExA_target != nullptr)
    MH_QueueDisableHook (_loader_hooks.LoadLibraryExA_target);
  if (_loader_hooks.LoadLibraryExW_target != nullptr)
    MH_QueueDisableHook (_loader_hooks.LoadLibraryExW_target);
  if (_loader_hooks.FreeLibrary_target != nullptr)
    MH_QueueDisableHook (_loader_hooks.FreeLibrary_target);

  MH_ApplyQueued ();

  if (_loader_hooks.LoadLibraryA_target != nullptr)
    MH_RemoveHook (_loader_hooks.LoadLibraryA_target);
  if (_loader_hooks.LoadLibraryW_target != nullptr)
    MH_RemoveHook (_loader_hooks.LoadLibraryW_target);
  if (_loader_hooks.LoadLibraryExA_target != nullptr)
    MH_RemoveHook (_loader_hooks.LoadLibraryExA_target);
  if (_loader_hooks.LoadLibraryExW_target != nullptr)
    MH_RemoveHook (_loader_hooks.LoadLibraryExW_target);
  if (_loader_hooks.FreeLibrary_target != nullptr)
    MH_RemoveHook (_loader_hooks.FreeLibrary_target);

  _loader_hooks.LoadLibraryW_target   = nullptr;
  _loader_hooks.LoadLibraryExA_target = nullptr;
  _loader_hooks.LoadLibraryA_target   = nullptr;
  _loader_hooks.LoadLibraryExW_target = nullptr;
  _loader_hooks.FreeLibrary_target    = nullptr;

  // Re-establish the non-hooked functions
  SK_PreInitLoadLibrary ();

  SK_UnlockDllLoader ();
}



void
__stdcall
SK_InitCompatBlacklist (void)
{
  memset (&_loader_hooks, 0, sizeof sk_loader_hooks_t);
  SK_ReHookLoadLibrary ();
}

extern std::wstring
__stdcall
SK_GetDLLVersionStr (const wchar_t* wszName);


static bool loaded_gl     = false;
static bool loaded_vulkan = false;
static bool loaded_d3d9   = false;
static bool loaded_dxgi   = false;

struct enum_working_set_s {
  HMODULE     modules [1024];
  int         count;
  iSK_Logger* logger;
  HANDLE      proc;
};

#include <unordered_set>
std::unordered_set <HMODULE> logged_modules;

void
_SK_SummarizeModule ( LPVOID   base_addr,  ptrdiff_t   mod_size,
                      HMODULE  hMod,       uintptr_t   addr,
                      wchar_t* wszModName, iSK_Logger* pLogger )
{
  extern
  ULONG
  SK_GetSymbolNameFromModuleAddr (HMODULE hMod, uintptr_t addr, char* pszOut, ULONG ulLen);

  char  szSymbol [1024] = { };
  ULONG ulLen  =  1024;

  ulLen = SK_GetSymbolNameFromModuleAddr (hMod, addr, szSymbol, ulLen);
  
  if (ulLen != 0) {
    pLogger->Log ( L"[ Module ]  ( %ph + %08lu )   -:< %-64hs >:-   %s",
                      (void *)base_addr, (uint32_t)mod_size,
                        szSymbol, wszModName );
  } else {
    pLogger->Log ( L"[ Module ]  ( %ph + %08lu )       %-64hs       %s",
                      base_addr, mod_size, "", wszModName );
  }

  std::wstring ver_str = SK_GetDLLVersionStr (wszModName);

  if (ver_str != L"  ") {
    pLogger->LogEx ( false,
      L" ----------------------  [File Ver]    %s\n",
        ver_str.c_str () );
  }
}

void
SK_ThreadWalkModules (enum_working_set_s* pWorkingSet)
{
  SK_LockDllLoader ();

  iSK_Logger* pLogger = pWorkingSet->logger;

  for (int i = 0; i < pWorkingSet->count; i++ )
  {
        wchar_t wszModName [MAX_PATH + 2] = { };

    __try
    {
      // Get the full path to the module's file.
      if ( (! logged_modules.count (pWorkingSet->modules [i])) &&
              GetModuleFileNameExW ( pWorkingSet->proc,
                                       pWorkingSet->modules [i],
                                         wszModName,
                                           MAX_PATH ) )
      {
        MODULEINFO mi = { 0 };

        uintptr_t entry_pt  = 0;
        uintptr_t base_addr = 0;
        uint32_t  mod_size  = 0UL;

        if (GetModuleInformation (pWorkingSet->proc, pWorkingSet->modules [i], &mi, sizeof (MODULEINFO))) {
          entry_pt  = (uintptr_t)mi.EntryPoint;
          base_addr = (uintptr_t)mi.lpBaseOfDll;
          mod_size  =            mi.SizeOfImage;
        }
        else {
          break;
        }

        _SK_SummarizeModule ((void *)base_addr, mod_size, pWorkingSet->modules [i], entry_pt, wszModName, pLogger);

        logged_modules.insert (pWorkingSet->modules [i]);
      }
    }

    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
      // Sometimes a DLL will be unloaded in the middle of doing this... just ignore that.
    }
  }

  SK_UnlockDllLoader ();
}

void
SK_WalkModules (int cbNeeded, HANDLE hProc, HMODULE* hMods, SK_ModuleEnum when)
{
  SK_LockDllLoader ();

  for ( int i = 0; i < (int)(cbNeeded / sizeof (HMODULE)); i++ )
  {
    wchar_t wszModName [MAX_PATH + 2] = { L'\0' };
            ZeroMemory (wszModName, sizeof (wchar_t) * (MAX_PATH + 2));

    __try {
      // Get the full path to the module's file.
      if ( GetModuleFileNameExW ( hProc,
                                    hMods [i],
                                      wszModName,
                                        MAX_PATH ) )
      {
        if ( (! third_party_dlls.overlays.rtss_hooks) &&
              StrStrIW (wszModName, L"RTSSHooks") ) {
          // Hold a reference to this DLL so it is not unloaded prematurely
          GetModuleHandleEx ( GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
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
          GetModuleHandleEx ( GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
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
          GetModuleHandleEx ( GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
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

        if (when == SK_ModuleEnum::PostLoad) {
          if ( StrStrIW (wszModName, L"\\opengl32.dll") && (SK_IsInjected () || (! (SK_GetDLLRole () & DLL_ROLE::OpenGL)))) {
            SK_BootOpenGL ();

            loaded_gl = true;
          }

          else if ( StrStrIW (wszModName, L"\\vulkan-1.dll") && (SK_IsInjected () || (! (SK_GetDLLRole () & DLL_ROLE::Vulkan)))) {
            SK_BootVulkan ();
          
            loaded_vulkan = true;
          }
          
#if 0
          else if ( StrStrIW (wszModName, L"\\dxgi.dll") && (SK_IsInjected () || (! (SK_GetDLLRole () & DLL_ROLE::DXGI)))) {
            SK_BootDXGI ();
          
            loaded_dxgi = true;
          }
#endif

#if 0 
          else if ( StrStrIW (wszModName, L"\\d3d9.dll") && (SK_IsInjected () || (! (SK_GetDLLRole () & DLL_ROLE::D3D9)))) {
            SK_BootD3D9 ();
          
            loaded_d3d9 = true;
          }
#endif
        }

        if (! config.steam.silent)
        {
          if (StrStrIW (wszModName, L"CSteamworks.dll")) {
            SK_HookCSteamworks ();
          } else if (StrStrIW (wszModName, L"steam_api.dll")   ||
                 StrStrIW     (wszModName, L"steam_api64.dll") ||
                 StrStrIW     (wszModName, L"SteamNative.dll") ) {
              SK_HookSteamAPI ();
          }
        }
      }
    }

    __except ( (GetExceptionCode () == EXCEPTION_INVALID_HANDLE) ?
                           EXCEPTION_EXECUTE_HANDLER :
                           EXCEPTION_CONTINUE_SEARCH  )
    {
      // Sometimes a DLL will be unloaded in the middle of doing this... just ignore that.
    }
  }

  SK_UnlockDllLoader ();
}

void
__stdcall
SK_EnumLoadedModules (SK_ModuleEnum when)
{
  // Begin logging new loads after this
  SK_LoadLibrary_SILENCE = false;

  static iSK_Logger*
               pLogger  = SK_CreateLog (L"logs/modules.log");
  DWORD        dwProcID = GetCurrentProcessId ();

  HMODULE      hMods [1024];
  HANDLE       hProc    = nullptr;
  DWORD        cbNeeded = 0;

  // Get a handle to the process.
  hProc = OpenProcess ( PROCESS_QUERY_INFORMATION |
                        PROCESS_VM_READ,
                          FALSE,
                            dwProcID );

  if (hProc == nullptr && (when != SK_ModuleEnum::PreLoad))
  {
    pLogger->close ();
    delete pLogger;
    return;
  }

  if ( when   == SK_ModuleEnum::PostLoad &&
      pLogger != nullptr )
  {
    pLogger->LogEx (
      false,
        L"================================================================== "
        L"(End Preloads) "
        L"==================================================================\n"
    );
  }


  if (hProc == nullptr)
    return;

  if ( EnumProcessModules ( hProc,
                              hMods,
                                sizeof (hMods),
                                  &cbNeeded) )
  {
    enum_working_set_s working_set;

            working_set.proc   = hProc;
            working_set.logger = pLogger;
            working_set.count  = cbNeeded / sizeof HMODULE;
    memcpy (working_set.modules, hMods, cbNeeded);

    enum_working_set_s* pWorkingSet = (enum_working_set_s *)&working_set;
    SK_ThreadWalkModules (pWorkingSet);

    //pLogger->close ();
    //delete pLogger;

    SK_WalkModules (cbNeeded, hProc, hMods, when);
  }

  if (third_party_dlls.overlays.rtss_hooks != nullptr) {
    SK_ValidateGlobalRTSSProfile ();
  }

  // In 64-bit builds, RTSS is really sneaky :-/
  else if (SK_GetRTSSInstallDir ().length ()) {
    SK_ValidateGlobalRTSSProfile ();
  }

  else if ( GetModuleHandle (L"RTSSHooks.dll") ||
            GetModuleHandle (L"RTSSHooks64.dll") )
  {
    SK_ValidateGlobalRTSSProfile ();
    // RTSS is in High App Detection or Stealth Mode
    //
    //   The software is probably going to crash.
    dll_log.Log ( L"[RTSSCompat] RTSS appears to be in High App Detection or Stealth mode, "
                  L"your game is probably going to crash." );
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
  UNREFERENCED_PARAMETER (dwRefData);
  UNREFERENCED_PARAMETER (wParam);

  if (uNotification == TDN_TIMER) {
    //DWORD dwProcId;
    //DWORD dwThreadId =
      //GetWindowThreadProcessId (GetForegroundWindow (), &dwProcId);

    //if (dwProcId == GetCurrentProcessId ()) {
      SK_RealizeForegroundWindow (SK_bypass_dialog_hwnd);
    //}
  }

  if (uNotification == TDN_HYPERLINK_CLICKED) {
    ShellExecuteW (nullptr, L"open", (wchar_t *)lParam, nullptr, nullptr, SW_SHOW);
    return S_OK;
  }

  if (uNotification == TDN_DIALOG_CONSTRUCTED) {
    while (InterlockedCompareExchange (&SK_bypass_dialog_active, 0, 0) > 1)
      Sleep (10);

    SK_bypass_dialog_hwnd = hWnd;

    InterlockedIncrementAcquire (&SK_bypass_dialog_active);
  }

  if (uNotification == TDN_CREATED) {
    SK_bypass_dialog_hwnd = hWnd;

    //SK_RealizeForegroundWindow (SK_bypass_dialog_hwnd);
  }

  if (uNotification == TDN_DESTROYED) {
    SK_bypass_dialog_hwnd = 0;
    InterlockedDecrementRelease (&SK_bypass_dialog_active);
  }

  return S_OK;
}

#include <SpecialK/config.h>
#include <SpecialK/ini.h>

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

  TASKDIALOGCONFIG task_config    = {0};

  task_config.cbSize              = sizeof (task_config);
  task_config.hInstance           = SK_GetDLL ();
  task_config.hwndParent          = GetActiveWindow ();
  task_config.pszWindowTitle      = L"Special K Compatibility Layer";
  task_config.dwCommonButtons     = TDCBF_OK_BUTTON;
  task_config.pButtons            = nullptr;
  task_config.cButtons            = 0;
  task_config.dwFlags             = TDF_ENABLE_HYPERLINKS;
  task_config.pfCallback          = TaskDialogCallback;
  task_config.lpCallbackData      = 0;

  task_config.pszMainInstruction  = L"RivaTuner Statistics Server Incompatibility";

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
      sinfo.dwFlags     = STARTF_USESHOWWINDOW | STARTF_RUNFULLSCREEN;
      sinfo.wShowWindow = SW_SHOWNORMAL;

      CreateProcess (
        nullptr,
          (LPWSTR)SK_GetHostApp (),
            nullptr, nullptr,
              TRUE,
                CREATE_SUSPENDED,
                  nullptr, nullptr,
                    &sinfo, &pinfo );

      while (ResumeThread (pinfo.hThread))
        ;

      CloseHandle  (pinfo.hThread);
      CloseHandle  (pinfo.hProcess);

      extern BOOL __stdcall SK_TerminateParentProcess (UINT uExitCode);
      SK_TerminateParentProcess (0x00);
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

  int              nButtonPressed = 0;
  TASKDIALOGCONFIG task_config    = {0};

  task_config.cbSize              = sizeof (task_config);
  task_config.hInstance           = SK_GetDLL ();
  task_config.hwndParent          = GetActiveWindow ();
  task_config.pszWindowTitle      = L"Special K Compatibility Layer";
  task_config.dwCommonButtons     = TDCBF_OK_BUTTON;
  task_config.pButtons            = nullptr;
  task_config.cButtons            = 0;
  task_config.dwFlags             = 0x00;
  task_config.pfCallback          = TaskDialogCallback;
  task_config.lpCallbackData      = 0;

  task_config.pszMainInstruction  = wszMainInstruction;

  task_config.pszMainIcon         = wszMainIcon;
  task_config.pszContent          = wszContent;

  task_config.pszFooterIcon       = wszFooterIcon;
  task_config.pszFooter           = wszFooter;

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

  int              nButtonPressed =   0;
  TASKDIALOGCONFIG task_config    = { 0 };

  task_config.cbSize              = sizeof    (task_config);
  task_config.hInstance           = SK_GetDLL ();
  task_config.pszWindowTitle      = L"Special K Compatibility Layer";
  task_config.dwCommonButtons     = TDCBF_OK_BUTTON;

  TASKDIALOG_BUTTON button;
  button.nButtonID               = 0xdead01ae;
  button.pszButtonText           = wszCommand;

  task_config.pButtons           = &button;
  task_config.cButtons           = 1;

  task_config.dwFlags            = 0x00;
  task_config.dwFlags           |= TDF_USE_COMMAND_LINKS | TDF_SIZE_TO_CONTENT |
                                   TDF_POSITION_RELATIVE_TO_WINDOW;

  task_config.pfCallback         = TaskDialogCallback;
  task_config.lpCallbackData     = 0;

  task_config.pszMainInstruction = wszMainInstruction;

  task_config.hwndParent         = GetActiveWindow ();

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

enum {
  SK_BYPASS_UNKNOWN    = 0x0,
  SK_BYPASS_ACTIVATE   = 0x1,
  SK_BYPASS_DEACTIVATE = 0x2
};

volatile LONG SK_BypassResult = SK_BYPASS_UNKNOWN;

DWORD
WINAPI
SK_RaptrWarn (LPVOID user)
{
  UNREFERENCED_PARAMETER (user);

  // Don't check for Raptr while installing something...
  if (SK_IsHostAppSKIM ()) {
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

struct sk_bypass_s {
  BOOL    disable;
  wchar_t wszBlacklist [MAX_PATH];
} __bypass;

DWORD
WINAPI
SK_Bypass_CRT (LPVOID user)
{
  UNREFERENCED_PARAMETER (user);

  static BOOL     disable      = __bypass.disable;
         wchar_t* wszBlacklist = __bypass.wszBlacklist;

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

      InterlockedExchange (&SK_BypassResult, SK_BYPASS_ACTIVATE);
    } else {
      DeleteFileW (wszBlacklist);
      InterlockedExchange (&SK_BypassResult, SK_BYPASS_DEACTIVATE);
    }
  }

  InterlockedDecrement (&SK_bypass_dialog_active);

  if (disable != __bypass.disable)
  {
    STARTUPINFO         sinfo = { 0 };
    PROCESS_INFORMATION pinfo = { 0 };

    sinfo.cb          = sizeof STARTUPINFO;
    sinfo.dwFlags     = STARTF_USESHOWWINDOW | STARTF_RUNFULLSCREEN;
    sinfo.wShowWindow = SW_SHOWNORMAL;

    CreateProcess (
      nullptr,
        (LPWSTR)SK_GetHostApp (),
          nullptr, nullptr,
            TRUE,
              CREATE_SUSPENDED,
                nullptr, nullptr,
                  &sinfo, &pinfo );

    ResumeThread     (pinfo.hThread);
    CloseHandle      (pinfo.hThread);
    CloseHandle      (pinfo.hProcess);

    extern BOOL __stdcall SK_TerminateParentProcess (UINT uExitCode);
    SK_TerminateParentProcess (0x00);
  }

  CloseHandle (GetCurrentThread ());
  ExitProcess (0);

//return 0;
}

#include <process.h>

std::pair <std::queue <DWORD>, BOOL>
__stdcall
SK_BypassInject (void)
{
  std::queue <DWORD> tids = SK_SuspendAllOtherThreads ();

  lstrcpyW (__bypass.wszBlacklist, SK_GetBlacklistFilename ());

  __bypass.disable = 
    (GetFileAttributesW (__bypass.wszBlacklist) != INVALID_FILE_ATTRIBUTES);

  IsGUIThread (TRUE);

  InterlockedIncrement (&SK_bypass_dialog_active);

  CreateThread ( nullptr,
                   0,
                     SK_Bypass_CRT, nullptr,
                       0x00,
                         nullptr );

  return std::make_pair (tids, __bypass.disable);
}












void
__stdcall
SK_PreInitLoadLibrary (void)
{
  FreeLibrary_Original    = &FreeLibrary;
  LoadLibraryA_Original   = &LoadLibraryA;
  LoadLibraryW_Original   = &LoadLibraryW;
  LoadLibraryExA_Original = &LoadLibraryExA;
  LoadLibraryExW_Original = &LoadLibraryExW;
}

FreeLibrary_pfn    FreeLibrary_Original    = nullptr;

LoadLibraryA_pfn   LoadLibraryA_Original   = nullptr;
LoadLibraryW_pfn   LoadLibraryW_Original   = nullptr;

LoadLibraryExA_pfn LoadLibraryExA_Original = nullptr;
LoadLibraryExW_pfn LoadLibraryExW_Original = nullptr;