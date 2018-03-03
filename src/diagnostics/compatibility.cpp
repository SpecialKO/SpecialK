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

#include <Windows.h>
#include <SpecialK/diagnostics/compatibility.h>
#include <SpecialK/diagnostics/crash_handler.h>
#include <process.h>

#pragma warning (disable:4091)
#define _IMAGEHLP_SOURCE_
#include <DbgHelp.h>

#include <psapi.h>

#include <comdef.h>
#include <atlbase.h>

#include <Commctrl.h>
#include <Shlwapi.h>
#pragma comment (lib,    "advapi32.lib")
#pragma comment (lib,    "user32.lib")
#pragma comment (lib,    "comctl32.lib")
#pragma comment (linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' "  \
                         "version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df'" \
                         " language='*'\"")

#pragma comment (lib, "Shlwapi.lib")

#include <array>
#include <string>
#include <memory>
#include <typeindex>
#include <unordered_set>

#include <cassert>

#include <SpecialK/config.h>
#include <SpecialK/hooks.h>
#include <SpecialK/core.h>
#include <SpecialK/log.h>
#include <SpecialK/ini.h>
#include <SpecialK/utility.h>
#include <SpecialK/thread.h>
#include <SpecialK/steam_api.h>
#include <SpecialK/window.h>

#include <SpecialK/render/backend.h>
#include <SpecialK/input/dinput7_backend.h>
#include <SpecialK/input/dinput8_backend.h>

#include <SpecialK/injection/injection.h>

#include <SpecialK/framerate.h>
#include <SpecialK/DLL_VERSION.H>

#include <concurrent_unordered_set.h>

concurrency::concurrent_unordered_set <HMODULE> dbghelp_callers;

#define SK_CHAR(x) (_T)        (constexpr _T      (std::type_index (typeid (_T)) == std::type_index (typeid (wchar_t))) ? (      _T  )(_L(x)) : (      _T  )(x))
#define SK_TEXT(x) (const _T*) (constexpr LPCVOID (std::type_index (typeid (_T)) == std::type_index (typeid (wchar_t))) ? (const _T *)(_L(x)) : (const _T *)(x))

using StrStrI_pfn            = PSTR    (__stdcall *)(LPCVOID lpFirst,   LPCVOID lpSearch);
using PathRemoveFileSpec_pfn = BOOL    (__stdcall *)(LPVOID  lpPath);
using LoadLibrary_pfn        = HMODULE (__stdcall *)(LPCVOID lpLibFileName);
using strncpy_pfn            = LPVOID  (__cdecl   *)(LPVOID  lpDest,    LPCVOID lpSource,     size_t   nCount);
using lstrcat_pfn            = LPVOID  (__stdcall *)(LPVOID  lpString1, LPCVOID lpString2);
using GetModuleHandleEx_pfn  = BOOL    (__stdcall *)(DWORD   dwFlags,   LPCVOID lpModuleName, HMODULE* phModule);


BOOL __stdcall SK_TerminateParentProcess    (UINT uExitCode);

bool SK_LoadLibrary_SILENCE = false;


#ifdef _WIN64
#define SK_STEAM_BIT_WSTRING L"64"
#define SK_STEAM_BIT_STRING   "64"
#else
#define SK_STEAM_BIT_WSTRING L""
#define SK_STEAM_BIT_STRING   ""
#endif

static const wchar_t* wszSteamClientDLL = L"SteamClient";
static const char*     szSteamClientDLL =  "SteamClient";

static const wchar_t* wszSteamNativeDLL = L"SteamNative.dll";
static const char*     szSteamNativeDLL =  "SteamNative.dll";

static const wchar_t* wszSteamAPIDLL    = L"steam_api" SK_STEAM_BIT_WSTRING L".dll";
static const char*     szSteamAPIDLL    =  "steam_api" SK_STEAM_BIT_STRING   ".dll";
static const wchar_t* wszSteamAPIAltDLL = L"steam_api.dll";
static const char*     szSteamAPIAltDLL =  "steam_api.dll";


struct sk_loader_hooks_t {
  // Manually unhooked for compatibility, DO NOT rehook!
  bool   unhooked                   = false;

  LPVOID LoadLibraryA_target        = nullptr;
  LPVOID LoadLibraryExA_target      = nullptr;
  LPVOID LoadLibraryW_target        = nullptr;
  LPVOID LoadLibraryExW_target      = nullptr;
  LPVOID LoadPackagedLibrary_target = nullptr;

  LPVOID GetModuleHandleA_target    = nullptr;
  LPVOID GetModuleHandleW_target    = nullptr;
  LPVOID GetModuleHandleExA_target  = nullptr;
  LPVOID GetModuleHandleExW_target  = nullptr;

  LPVOID GetModuleFileNameA_target  = nullptr;
  LPVOID GetModuleFileNameW_target  = nullptr;

  LPVOID FreeLibrary_target         = nullptr;
} _loader_hooks;

void
__stdcall
SK_LockDllLoader (void)
{
  if (config.system.strict_compliance)
  {
    //bool unlocked = TryEnterCriticalSection (&loader_lock);
                       loader_lock->lock ();
    //EnterCriticalSection (&loader_lock);
    //loader_lock->lock ();
     ///if (unlocked)
                       //LeaveCriticalSection (&loader_lock);
    //else
//      dll_log.Log (L"[DLL Loader]  *** DLL Loader Lock Contention ***");
  }
}

void
__stdcall
SK_UnlockDllLoader (void)
{
  if (config.system.strict_compliance)
    loader_lock->unlock ();
}


using GetModuleHandleExW_pfn = BOOL (WINAPI*)(
  _In_     DWORD       dwFlags,
  _In_opt_ LPCWSTR     lpModuleName,
  _Outptr_ HMODULE*    phModule
);

using GetModuleHandleExA_pfn = BOOL (WINAPI*)(
  _In_     DWORD       dwFlags,
  _In_opt_ LPCSTR      lpModuleName,
  _Outptr_ HMODULE*    phModule
);
GetModuleHandleExA_pfn GetModuleHandleExA_Original = nullptr;
GetModuleHandleExW_pfn GetModuleHandleExW_Original = nullptr;

BOOL
WINAPI
GetModuleHandleExA_Detour (
  _In_     DWORD       dwFlags,
  _In_opt_ LPCSTR      lpModuleName,
  _Outptr_ HMODULE*    phModule )
{
  if (dwFlags & GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS)
  {
    if ((LPVOID)lpModuleName == SK_GetDLL ())
      return false;
  }

  if (StrStrIA (lpModuleName, SK_RunLHIfBitness (64, "SpecialK64",
                                                     "SpecialK32")) )
    return false;

  return GetModuleHandleExA_Original (dwFlags, lpModuleName, phModule);
}

BOOL
WINAPI
GetModuleHandleExW_Detour (
  _In_     DWORD       dwFlags,
  _In_opt_ LPCWSTR     lpModuleName,
  _Outptr_ HMODULE*    phModule )
{
  if (dwFlags & GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS)
  {
    if ((LPVOID)lpModuleName == SK_GetDLL ())
      return false;
  }

  if (StrStrIW (lpModuleName, SK_RunLHIfBitness (64, L"SpecialK64",
                                                     L"SpecialK32")) )
    return false;

  return GetModuleHandleExW_Original (dwFlags, lpModuleName, phModule);
}




typedef DWORD (WINAPI *GetModuleFileNameA_pfn)(
_In_opt_ HMODULE hModule,
_Out_writes_to_(nSize, ((return < nSize) ? (return + 1) : nSize)) LPSTR lpFilename,
_In_ DWORD nSize
);
GetModuleFileNameA_pfn GetModuleFileNameA_Original = nullptr;


typedef DWORD (WINAPI *GetModuleFileNameW_pfn)(
_In_opt_ HMODULE hModule,
_Out_writes_to_(nSize, ((return < nSize) ? (return + 1) : nSize)) LPWSTR lpFilename,
_In_ DWORD nSize
);
GetModuleFileNameW_pfn GetModuleFileNameW_Original = nullptr;


DWORD
WINAPI
GetModuleFileNameA_Detour (
  _In_opt_ HMODULE hModule,
  _Out_writes_to_ (nSize, ( ( return < nSize ) ? ( return +1 ) : nSize )) LPSTR lpFilename,
  _In_ DWORD nSize)
{
  if (hModule == SK_GetDLL ())
    return 0;

  return GetModuleFileNameA_Original (hModule, lpFilename, nSize);
}

DWORD
WINAPI
GetModuleFileNameW_Detour (
  _In_opt_ HMODULE hModule,
  _Out_writes_to_ (nSize, ( ( return < nSize ) ? ( return +1 ) : nSize )) LPWSTR lpFilename,
  _In_ DWORD nSize)
{
  if (hModule == SK_GetDLL ())
    return 0;

  return GetModuleFileNameW_Original (hModule, lpFilename, nSize);
}

using GetModuleHandleA_pfn = HMODULE (WINAPI *)(_In_opt_ LPCSTR lpModuleName);
                         GetModuleHandleA_pfn GetModuleHandleA_Original = nullptr;

HMODULE
WINAPI
GetModuleHandleA_Detour (
  _In_opt_ LPCSTR lpModuleName
)
{
  if ( SK_RunLHIfBitness (64, StrStrIA (lpModuleName, "SpecialK64"),
                              StrStrIA (lpModuleName, "SpecialK32")) )
  {
    return nullptr;
  }

  static const std::unordered_set <std::string> blacklist_nv =
  {
    "libovrrt64_1", "libovrrt32_1", "openvr_api"
  };

  if (lpModuleName != nullptr && blacklist_nv.count (lpModuleName))
  {
    if (config.system.log_level > 4)
    {
      dll_log.Log (L"[Driver Bug] NVIDIA tried to load %hs", lpModuleName);
    }

    SetLastError (0x0000007e);
    return nullptr;
  }

  return
    GetModuleHandleA_Original (lpModuleName);
}

using GetModuleHandleW_pfn = HMODULE (WINAPI *)(_In_opt_ LPCWSTR lpModuleName);
                         GetModuleHandleW_pfn GetModuleHandleW_Original = nullptr;

HMODULE
WINAPI
GetModuleHandleW_Detour (
  _In_opt_ LPCWSTR lpModuleName
)
{
  if ( SK_RunLHIfBitness (64, StrStrIW (lpModuleName, L"SpecialK64"),
                              StrStrIW (lpModuleName, L"SpecialK32")) )
  {
    return nullptr;
  }

  return
    GetModuleHandleW_Original (lpModuleName);
}


template <typename _T>
BOOL
__stdcall
BlacklistLibrary (const _T* lpFileName)
{
#pragma push_macro ("StrStrI")
#pragma push_macro ("GetModuleHandleEx")
#pragma push_macro ("LoadLibrary")

#pragma push_macro ("StrStrI")
#undef StrStrI
#undef GetModuleHandleEx
#undef LoadLibrary

  static StrStrI_pfn            StrStrI =
    (StrStrI_pfn)
      constexpr LPCVOID ( std::type_index (typeid (_T)) == std::type_index (typeid (wchar_t)) ? (StrStrI_pfn)           &StrStrIW           :
                                                                                                (StrStrI_pfn)           &StrStrIA           );

  static GetModuleHandleEx_pfn  GetModuleHandleEx =
    (GetModuleHandleEx_pfn)
      constexpr LPCVOID ( std::type_index (typeid (_T)) == std::type_index (typeid (wchar_t)) ? (GetModuleHandleEx_pfn) &GetModuleHandleExW :
                                                                                                (GetModuleHandleEx_pfn) &GetModuleHandleExA );

  static LoadLibrary_pfn  LoadLibrary =
    (LoadLibrary_pfn)
      constexpr LPCVOID ( std::type_index (typeid (_T)) == std::type_index (typeid (wchar_t)) ? (LoadLibrary_pfn) &LoadLibraryW_Detour :
                                                                                                (LoadLibrary_pfn) &LoadLibraryA_Detour );

  if (config.compatibility.disable_nv_bloat)
  {
    static bool init = false;

    static std::vector < const _T* >
                nv_blacklist;

    if (! init)
    {
      nv_blacklist.emplace_back (SK_TEXT("rxgamepadinput.dll"));
      nv_blacklist.emplace_back (SK_TEXT("rxcore.dll"));
      nv_blacklist.emplace_back (SK_TEXT("nvinject.dll"));
      nv_blacklist.emplace_back (SK_TEXT("rxinput.dll"));
      nv_blacklist.emplace_back (SK_TEXT("nvspcap"));
      nv_blacklist.emplace_back (SK_TEXT("nvSCPAPI"));
      init = true;
    }

    for ( auto&& it : nv_blacklist )
    {
      if (StrStrI (lpFileName, it))
      {
        HMODULE hModNV;

        if (GetModuleHandleEx (GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, it, &hModNV))
          FreeLibrary_Original (hModNV);

        return TRUE;
      }
    }
  }

  return FALSE;

#pragma pop_macro ("StrStrI")
#pragma pop_macro ("GetModuleHandleEx")
#pragma pop_macro ("LoadLibrary")
}


template <typename _T>
BOOL
__stdcall
SK_LoadLibrary_PinModule (const _T* pStr)
{
#pragma push_macro ("GetModuleHandleEx")

#undef GetModuleHandleEx

  static GetModuleHandleEx_pfn  GetModuleHandleEx =
    (GetModuleHandleEx_pfn)
      constexpr LPCVOID ( typeid (_T) == typeid (wchar_t) ? (GetModuleHandleEx_pfn) &GetModuleHandleExW :
                                                            (GetModuleHandleEx_pfn) &GetModuleHandleExA );

  HMODULE hModDontCare;

  return
    GetModuleHandleEx ( GET_MODULE_HANDLE_EX_FLAG_PIN,
                          pStr,
                            &hModDontCare );

#pragma pop_macro ("GetModuleHandleEx")
}

template <typename _T>
bool
__stdcall
SK_LoadLibrary_IsPinnable (const _T* pStr)
{
#pragma push_macro ("StrStrI")

#undef StrStrI

  static StrStrI_pfn            StrStrI =
    (StrStrI_pfn)
      constexpr LPCVOID ( typeid (_T) == typeid (wchar_t) ? (StrStrI_pfn) &StrStrIW :
                                                            (StrStrI_pfn) &StrStrIA );
  static std::vector <const _T*> pinnable_libs =
  {
    SK_TEXT ("OpenCL"),    SK_TEXT ("CEGUI"),
    SK_TEXT ("perfos"),    SK_TEXT ("avrt"),

    SK_TEXT ("AUDIOSES"),  SK_TEXT ("HID"),

    SK_TEXT ("d3dx"),      SK_TEXT ("dsound"),

    // Fix for premature DLL unload issue discussed here:
    //
    //   https://blogs.msdn.microsoft.com/chuckw/2015/10/09/known-issues-xaudio-2-7/
    ///
    SK_TEXT ("XAudio2_7"),

    SK_TEXT ("nvapi"),
    SK_TEXT ("steam_api"),
    SK_TEXT ("steamclient"),

    //SK_TEXT ("vstdlib_s"),
    //SK_TEXT ("tier0_s"),


    //// Some software repeatedly loads and unloads this, which can
    ////   cause TLS-related problems if left unchecked... just leave
    ////     the damn thing loaded permanently!
    SK_TEXT ("d3dcompiler_"),
  };

  for (auto it : pinnable_libs)
  {
    if (StrStrI (pStr, it))
      return true;
  }

  return false;

#pragma pop_macro ("StrStrI")
}

template <typename _T>
void
__stdcall
SK_TraceLoadLibrary (       HMODULE hCallingMod,
                      const _T*     lpFileName,
                      const _T*     lpFunction,
                            LPVOID  lpCallerFunc )
{
#pragma push_macro ("StrStrI")
#pragma push_macro ("PathRemoveFileSpec")
#pragma push_macro ("LoadLibrary")
#pragma push_macro ("lstrcat")
#pragma push_macro ("GetModuleHandleEx")

#undef StrStrI
#undef PathRemoveFileSpec
#undef LoadLibrary
#undef lstrcat
#undef GetModuleHandleEx

  static wchar_t wszDbgHelp [MAX_PATH * 2] = { };

  if (! *wszDbgHelp)
  {
    wcsncpy     (wszDbgHelp, SK_GetSystemDirectory (), MAX_PATH);
    PathAppendW (wszDbgHelp, L"dbghelp.dll");
  }

  if (GetModuleHandle (wszDbgHelp) && (! dbghelp_callers.count (hCallingMod)))
  {
    dbghelp_callers.insert (hCallingMod);
    SymRefreshModuleList   (GetCurrentProcess ());
  }

  static StrStrI_pfn            StrStrI =
    (StrStrI_pfn)
      constexpr LPCVOID ( typeid (_T) == typeid (wchar_t) ? (StrStrI_pfn)           &StrStrIW            :
                                                            (StrStrI_pfn)           &StrStrIA            );

  static PathRemoveFileSpec_pfn PathRemoveFileSpec =
    (PathRemoveFileSpec_pfn)
      constexpr LPCVOID ( typeid (_T) == typeid (wchar_t) ? (PathRemoveFileSpec_pfn)&PathRemoveFileSpecW :
                                                            (PathRemoveFileSpec_pfn)&PathRemoveFileSpecA );

  static LoadLibrary_pfn        LoadLibrary =
    (LoadLibrary_pfn)
      constexpr LPCVOID ( typeid (_T) == typeid (wchar_t) ? (LoadLibrary_pfn)       &LoadLibraryW        :
                                                            (LoadLibrary_pfn)       &LoadLibraryA        );

  static strncpy_pfn            strncpy_ =
    (strncpy_pfn)
      constexpr LPCVOID ( typeid (_T) == typeid (wchar_t) ? (strncpy_pfn)           &wcsncpy             :
                                                            (strncpy_pfn)           &strncpy             );

  static lstrcat_pfn            lstrcat =
    (lstrcat_pfn)
      constexpr LPCVOID ( typeid (_T) == typeid (wchar_t) ? (lstrcat_pfn)           &lstrcatW            :
                                                            (lstrcat_pfn)           &lstrcatA            );

  static GetModuleHandleEx_pfn  GetModuleHandleEx =
    (GetModuleHandleEx_pfn)
      constexpr LPCVOID ( typeid (_T) == typeid (wchar_t) ? (GetModuleHandleEx_pfn) &GetModuleHandleExW  :
                                                            (GetModuleHandleEx_pfn) &GetModuleHandleExA  );

  // It's impossible to log this if we're loading the DLL necessary to log this...
  if (StrStrI (lpFileName, SK_TEXT("dbghelp")))
  {
    return;
  }

  wchar_t  wszModName [MAX_PATH] = { };
  wcsncpy (wszModName, SK_GetModuleName (hCallingMod).c_str (), MAX_PATH);

  if ((! SK_LoadLibrary_SILENCE) && GetModuleHandle (wszDbgHelp))
  {
    char  szSymbol [1024] = { };
    ULONG ulLen  =  1023;

    ulLen =
      SK_GetSymbolNameFromModuleAddr (
        SK_GetCallingDLL      (lpCallerFunc),
 reinterpret_cast <uintptr_t> (lpCallerFunc),
                            szSymbol,
                              ulLen );

    if (constexpr (typeid (_T) == typeid (char)))
    {
      CHeapPtr <char> file_name (
        _strdup (reinterpret_cast <const char *> (lpFileName))
      );

      SK_StripUserNameFromPathA (file_name);

      dll_log.Log ( "[DLL Loader]   ( %-28ws ) loaded '%#116hs' <%14hs> { '%21hs' }",
                      wszModName,
                        file_name.m_pData,
                          lpFunction,
                            szSymbol );
    }

    else
    {
      CHeapPtr <wchar_t> file_name (
        _wcsdup (reinterpret_cast <const wchar_t *> (lpFileName))
      );

      SK_StripUserNameFromPathW (file_name);

      dll_log.Log ( L"[DLL Loader]   ( %-28ws ) loaded '%#116ws' <%14ws> { '%21hs' }",
                      wszModName,
                        file_name.m_pData,
                          lpFunction,
                            szSymbol );
    }
  }

  if (hCallingMod != SK_GetDLL ())
  {
    if (config.compatibility.rehook_loadlibrary)
    {
      // This is silly, this many string comparisons per-load is
      //   not good. Hash the string and compare it in the future.
      if ( StrStrIW (wszModName, L"Activation")          ||
           StrStrIW (wszModName, L"rxcore")              ||
           StrStrIW (wszModName, L"gameoverlayrenderer") ||
           StrStrIW (wszModName, L"RTSSHooks")           ||
           StrStrIW (wszModName, L"Nahimic2DevProps")    ||
           StrStrIW (wszModName, L"ReShade") )
      {
        SK_ReHookLoadLibrary ();
      }
    }
  }

  if (hCallingMod != SK_GetDLL ()/* && SK_IsInjected ()*/)
  {
    if ( (! (SK_GetDLLRole () & DLL_ROLE::D3D9)) && config.apis.d3d9.hook &&
         ( StrStrI  (lpFileName, SK_TEXT("d3d9.dll"))  ||
           StrStrIW (wszModName,        L"d3d9.dll")   ||

           StrStrI  (lpFileName, SK_TEXT("d3dx9_"))    ||
           StrStrIW (wszModName,        L"d3dx9_")     ||

           StrStrI  (lpFileName, SK_TEXT("Direct3D9")) ||
           StrStrIW (wszModName,        L"Direct3D9")  ||

           // NVIDIA's User-Mode D3D Frontend
           StrStrI  (lpFileName, SK_TEXT("nvd3dum.dll")) ||
           StrStrIW (wszModName,        L"nvd3dum.dll")  ) )
      SK_BootD3D9   ();
#ifndef _WIN64
    else if ( (! (SK_GetDLLRole () & DLL_ROLE::D3D8)) && config.apis.d3d8.hook &&
              ( StrStrI  (lpFileName, SK_TEXT("d3d8.dll")) ||
                StrStrIW (wszModName,        L"d3d8.dll")    ) )
      SK_BootD3D8   ();
    else if ( (! (SK_GetDLLRole () & DLL_ROLE::DDraw)) && config.apis.ddraw.hook &&
              ( StrStrI  (lpFileName, SK_TEXT("ddraw.dll")) ||
                StrStrIW (wszModName,        L"ddraw.dll")   ) )
      SK_BootDDraw  ();
#endif
    else if ( (! (SK_GetDLLRole () & DLL_ROLE::DXGI)) && config.apis.dxgi.d3d11.hook &&
              ( StrStrI  (lpFileName, SK_TEXT("d3d11.dll")) ||
                StrStrIW (wszModName,        L"d3d11.dll") ))
      SK_BootDXGI   ();
#ifdef _WIN64
    else if ( (! (SK_GetDLLRole () & DLL_ROLE::DXGI)) && config.apis.dxgi.d3d12.hook &&
              ( StrStrI  (lpFileName, SK_TEXT("d3d12.dll")) ||
                StrStrIW (wszModName,        L"d3d12.dll") ))
      SK_BootDXGI   ();
    else if (   StrStrI  (lpFileName, SK_TEXT("vulkan-1.dll")) ||
                StrStrIW (wszModName,        L"vulkan-1.dll")  )
      SK_BootVulkan ();
#endif
    else if (  (! (SK_GetDLLRole () & DLL_ROLE::OpenGL)) && config.apis.OpenGL.hook &&
              ( StrStrI  (lpFileName, SK_TEXT("OpenGL32.dll")) ||
                StrStrIW (wszModName,        L"OpenGL32.dll") ))
      SK_BootOpenGL ();
    else if (   StrStrI (lpFileName, SK_TEXT("xinput1_3.dll")) )
      SK_Input_HookXInput1_3 ();
    else if (   StrStrI (lpFileName, SK_TEXT("xinput1_4.dll")) )
      SK_Input_HookXInput1_4 ();
    else if (   StrStrI (lpFileName, SK_TEXT("xinput9_1_0.dll")) )
      SK_Input_HookXInput9_1_0 ();
    else if (   StrStrI (lpFileName, SK_TEXT("dinput8.dll")) )
      SK_Input_HookDI8 ();
    else if (   StrStrI (lpFileName, SK_TEXT("dinput.dll")) )
      SK_Input_HookDI7 ();
    else if (   StrStrI (lpFileName, SK_TEXT("hid.dll")) )
      SK_Input_HookHID ();

#if 0
    if (! config.steam.silent) {
      if ( StrStrIA (lpFileName, szSteamAPIDLL)    ||
           StrStrIA (lpFileName, szSteamNativeDLL) ||
           StrStrIA (lpFileName, szSteamClientDLL) ) {
        SK_HookSteamAPI ();
      }
    }
#endif
  }

  if (SK_LoadLibrary_IsPinnable (lpFileName))
       SK_LoadLibrary_PinModule (lpFileName);

#pragma pop_macro ("StrStrI")
#pragma pop_macro ("PathRemoveFileSpec")
#pragma pop_macro ("LoadLibrary")
#pragma pop_macro ("lstrcat")
#pragma pop_macro ("GetModuleHandleEx")
}

BOOL
WINAPI
FreeLibrary_Detour (HMODULE hLibModule)
{
  LPVOID pAddr = _ReturnAddress ();

  if (ReadAcquire (&__SK_DLL_Ending) == TRUE)
  {
    return FreeLibrary_Original (hLibModule);
  }

  if (hLibModule == SK_GetDLL ())
    return TRUE;

  SK_LockDllLoader ();

  std::wstring name = SK_GetModuleName (hLibModule);

  BOOL bRet = FreeLibrary_Original (hLibModule);

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

        ulLen =
          SK_GetSymbolNameFromModuleAddr ( SK_GetCallingDLL (),
                                             reinterpret_cast <uintptr_t> (pAddr),
                                               szSymbol,
                                                 ulLen );

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
LoadLibrary_Marshal (LPVOID lpRet, LPCWSTR lpFileName, const wchar_t* wszSourceFunc)
{
  if ( SK_RunLHIfBitness (64, StrStrIW (lpFileName, L"SpecialK64"),
                              StrStrIW (lpFileName, L"SpecialK32")) )
  {
    return nullptr;
  }

  if (lpFileName == nullptr)
    return nullptr;

  SK_LockDllLoader ();

  HMODULE hModEarly = nullptr;

  wchar_t*        compliant_path = _wcsdup (lpFileName);
  SK_FixSlashesW (compliant_path);
     lpFileName = compliant_path;

  __try {
    GetModuleHandleExW ( GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           static_cast <LPCWSTR> (lpFileName),
                             &hModEarly );
  }

  __except ( (GetExceptionCode () == EXCEPTION_INVALID_HANDLE) ?
                       EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH  )
  {
    SetLastError (0);
  }


  if (hModEarly == nullptr && BlacklistLibrary (lpFileName))
  {
    free ( static_cast <void *> (compliant_path) );

    SK_UnlockDllLoader ();
    return nullptr;
  }


  HMODULE hMod = hModEarly;

  __try                                  { hMod = LoadLibraryW_Original (lpFileName); }
  __except ( ( GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION )  ?
                       EXCEPTION_EXECUTE_HANDLER :
                       EXCEPTION_CONTINUE_SEARCH )
  {
    dll_log.Log ( L"[DLL Loader]  ** Crash Prevented **  DLL raised an exception during"
                  L" %s ('%hs')!",
                    wszSourceFunc, lpFileName );
  }


  if (hModEarly != hMod)
  {
    SK_TraceLoadLibrary ( SK_GetCallingDLL (lpRet),
                            lpFileName,
                              wszSourceFunc, lpRet );
  }

  free ( static_cast <void *> (compliant_path) );

  SK_UnlockDllLoader ();
  return hMod;
}

HMODULE
WINAPI
LoadLibraryA_Detour (LPCSTR lpFileName)
{
  return
    LoadLibrary_Marshal (
       _ReturnAddress (),
         SK_UTF8ToWideChar (lpFileName).c_str (),
           L"LoadLibraryA"
    );
}

HMODULE
WINAPI
LoadLibraryW_Detour (LPCWSTR lpFileName)
{
  return
    LoadLibrary_Marshal (
       _ReturnAddress (),
         lpFileName,
           L"LoadLibraryW"
    );
}

HMODULE
WINAPI
LoadPackagedLibrary_Detour (LPCWSTR lpLibFileName, DWORD Reserved)
{
  LPVOID lpRet = _ReturnAddress ();

  if (lpLibFileName == nullptr)
    return nullptr;

  SK_LockDllLoader ();

  HMODULE hModEarly = nullptr;

  wchar_t*        compliant_path = _wcsdup (lpLibFileName);
  SK_FixSlashesW (compliant_path);
  lpLibFileName = compliant_path;

  __try {
    GetModuleHandleExW ( GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, lpLibFileName, &hModEarly );
  }
  __except ( (GetExceptionCode () == EXCEPTION_INVALID_HANDLE) ?
                           EXCEPTION_EXECUTE_HANDLER :
                           EXCEPTION_CONTINUE_SEARCH )
  {
    SetLastError (0);
  }


  if (hModEarly == nullptr && BlacklistLibrary (lpLibFileName))
  {
    free (static_cast <void *> (compliant_path));

    SK_UnlockDllLoader ();
    return nullptr;
  }


  HMODULE hMod =
    LoadPackagedLibrary_Original (lpLibFileName, Reserved);

  if (hModEarly != hMod)
  {
    SK_TraceLoadLibrary ( SK_GetCallingDLL (lpRet),
                            lpLibFileName,
                              L"LoadPackagedLibrary", lpRet );
  }

  free (static_cast <void *> (compliant_path));

  SK_UnlockDllLoader ();
  return hMod;
}

HMODULE
LoadLibraryEx_Marshal (LPVOID lpRet, LPCWSTR lpFileName, HANDLE hFile, DWORD dwFlags, const wchar_t* wszSourceFunc)
{
  if ( SK_RunLHIfBitness (64, StrStrIW (lpFileName, L"SpecialK64"),
                              StrStrIW (lpFileName, L"SpecialK32")) )
  {
    return nullptr;
  }

  if (lpFileName == nullptr)
    return nullptr;

  wchar_t*        compliant_path = _wcsdup (lpFileName);
  SK_FixSlashesW (compliant_path);
  lpFileName    = compliant_path;

  SK_LockDllLoader ();

  if ((dwFlags & LOAD_LIBRARY_AS_DATAFILE) && (! BlacklistLibrary (lpFileName)))
  {
    HMODULE hModRet =
      LoadLibraryExW_Original (lpFileName, hFile, dwFlags);

    free (static_cast <void *> (compliant_path));

    SK_UnlockDllLoader ();
    return hModRet;
  }

  HMODULE hModEarly = nullptr;

  __try
  {
    GetModuleHandleExW ( GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, lpFileName, &hModEarly );
  }

  __except ( (GetExceptionCode () == EXCEPTION_INVALID_HANDLE) ?
                           EXCEPTION_EXECUTE_HANDLER :
                           EXCEPTION_CONTINUE_SEARCH )
  {
    SetLastError (0);
  }

  if (hModEarly == nullptr && BlacklistLibrary (lpFileName))
  {
    free (static_cast <void *> (compliant_path));

    SK_UnlockDllLoader ();
    return nullptr;
  }


  HMODULE hMod = hModEarly;

  __try                                  { hMod = LoadLibraryExW_Original (lpFileName, hFile, dwFlags); }
  __except ( ( GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION )  ?
                       EXCEPTION_EXECUTE_HANDLER :
                       EXCEPTION_CONTINUE_SEARCH )
  {
    dll_log.Log ( L"[DLL Loader]  ** Crash Prevented **  DLL raised an exception during"
                  L" %s ('%ws')!",
                    wszSourceFunc, lpFileName );
  }


  if ( hModEarly != hMod && (! ((dwFlags & LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE) ||
                                (dwFlags & LOAD_LIBRARY_AS_IMAGE_RESOURCE))) )
  {
    SK_TraceLoadLibrary ( SK_GetCallingDLL (lpRet),
                            lpFileName,
                              wszSourceFunc, lpRet );
  }

  free (static_cast <void *> (compliant_path));

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
  return
    LoadLibraryEx_Marshal (
      _ReturnAddress (),
        SK_UTF8ToWideChar (lpFileName).c_str (),
          hFile,
            dwFlags,
              L"LoadLibraryExA"
    );
}

HMODULE
WINAPI
LoadLibraryExW_Detour (
  _In_       LPCWSTR lpFileName,
  _Reserved_ HANDLE  hFile,
  _In_       DWORD   dwFlags )
{
  return
    LoadLibraryEx_Marshal (
      _ReturnAddress (),
        lpFileName,
          hFile,
            dwFlags,
              L"LoadLibraryExW"
    );
}

struct SK_ThirdPartyDLLs {
  struct {
    HMODULE rtss_hooks    = nullptr;
    HMODULE steam_overlay = nullptr;
  } overlays;
} third_party_dlls;

//
// Gameoverlayrenderer{64}.dll messes with LoadLibrary hooking, which
//   means identifying which DLL loaded another becomes impossible
//     unless we remove and re-install our hooks.
//
// Hook order with LoadLibrary is not traditionally important for a mod
//   system such as Special K, but the compatibility layer benefits from
//     knowing exactly WHAT was responsible for loading a library.
//
bool
__stdcall
SK_ReHookLoadLibrary (void)
{
  if (! config.system.trace_load_library)
    return false;

  if (_loader_hooks.unhooked)
    return false;

  SK_LockDllLoader ();

  if (_loader_hooks.LoadLibraryA_target != nullptr)
  {
    SK_RemoveHook (_loader_hooks.LoadLibraryA_target);
    _loader_hooks.LoadLibraryA_target = nullptr;
  }

  SK_CreateDLLHook2 (      L"kernel32.dll",
                            "LoadLibraryA",
                             LoadLibraryA_Detour,
    static_cast_p2p <void> (&LoadLibraryA_Original),
                           &_loader_hooks.LoadLibraryA_target );

  MH_QueueEnableHook (_loader_hooks.LoadLibraryA_target);


  if (_loader_hooks.LoadLibraryW_target != nullptr)
  {
    SK_RemoveHook (_loader_hooks.LoadLibraryW_target);
    _loader_hooks.LoadLibraryW_target = nullptr;
  }

  SK_CreateDLLHook2 (      L"kernel32.dll",
                            "LoadLibraryW",
                             LoadLibraryW_Detour,
    static_cast_p2p <void> (&LoadLibraryW_Original),
                           &_loader_hooks.LoadLibraryW_target );

  MH_QueueEnableHook (_loader_hooks.LoadLibraryW_target);


  if (GetProcAddress (GetModuleHandle (L"kernel32.dll"), "LoadPackagedLibrary") != nullptr)
  {
    if (_loader_hooks.LoadPackagedLibrary_target != nullptr)
    {
      SK_RemoveHook (_loader_hooks.LoadPackagedLibrary_target);
      _loader_hooks.LoadPackagedLibrary_target = nullptr;
    }

    SK_CreateDLLHook2 (      L"kernel32.dll",
                              "LoadPackagedLibrary",
                               LoadPackagedLibrary_Detour,
      static_cast_p2p <void> (&LoadPackagedLibrary_Original),
                             &_loader_hooks.LoadPackagedLibrary_target );

    MH_QueueEnableHook (_loader_hooks.LoadPackagedLibrary_target);
  }


  if (_loader_hooks.LoadLibraryExA_target != nullptr)
  {
    SK_RemoveHook (_loader_hooks.LoadLibraryExA_target);
    _loader_hooks.LoadLibraryExA_target = nullptr;
  }

  SK_CreateDLLHook2 (      L"kernel32.dll",
                            "LoadLibraryExA",
                             LoadLibraryExA_Detour,
    static_cast_p2p <void> (&LoadLibraryExA_Original),
                           &_loader_hooks.LoadLibraryExA_target );

  MH_QueueEnableHook (_loader_hooks.LoadLibraryExA_target);


  if (_loader_hooks.LoadLibraryExW_target != nullptr)
  {
    SK_RemoveHook (_loader_hooks.LoadLibraryExW_target);
    _loader_hooks.LoadLibraryExW_target = nullptr;
  }

  SK_CreateDLLHook2 (      L"kernel32.dll",
                            "LoadLibraryExW",
                             LoadLibraryExW_Detour,
    static_cast_p2p <void> (&LoadLibraryExW_Original),
                           &_loader_hooks.LoadLibraryExW_target );

  MH_QueueEnableHook (_loader_hooks.LoadLibraryExW_target);


  if (_loader_hooks.FreeLibrary_target != nullptr)
  {
    SK_RemoveHook (_loader_hooks.FreeLibrary_target);
    _loader_hooks.FreeLibrary_target = nullptr;
  }

  static int calls = 0;


  // Steamclient64.dll leaks heap memory when unloaded,
  //   to prevent this from showing up during debug sessions,
  //     don't hook this function :)
#if 0
   SK_CreateDLLHook2 (      L"kernel32.dll",
                             "FreeLibrary",
                              FreeLibrary_Detour,
     static_cast_p2p <void> (&FreeLibrary_Original),
                            &_loader_hooks.FreeLibrary_target );

  MH_QueueEnableHook (_loader_hooks.FreeLibrary_target);
#endif

  if (calls++ > 0)
  {
    SK_ApplyQueuedHooks ();
  }

  SK_UnlockDllLoader ();

  return (calls > 1);
}

void
__stdcall
SK_UnhookLoadLibrary (void)
{
  SK_LockDllLoader ();

  _loader_hooks.unhooked = true;

  if (_loader_hooks.LoadLibraryA_target != nullptr)
    MH_QueueDisableHook (_loader_hooks.LoadLibraryA_target);
  if (_loader_hooks.LoadLibraryW_target != nullptr)
    MH_QueueDisableHook(_loader_hooks.LoadLibraryW_target);
  if (_loader_hooks.LoadPackagedLibrary_target != nullptr)
    MH_QueueDisableHook(_loader_hooks.LoadPackagedLibrary_target);
  if (_loader_hooks.LoadLibraryExA_target != nullptr)
    MH_QueueDisableHook (_loader_hooks.LoadLibraryExA_target);
  if (_loader_hooks.LoadLibraryExW_target != nullptr)
    MH_QueueDisableHook (_loader_hooks.LoadLibraryExW_target);
  if (_loader_hooks.FreeLibrary_target != nullptr)
    MH_QueueDisableHook (_loader_hooks.FreeLibrary_target);

  SK_ApplyQueuedHooks ();

  if (_loader_hooks.LoadLibraryA_target != nullptr)
    MH_RemoveHook (_loader_hooks.LoadLibraryA_target);
  if (_loader_hooks.LoadLibraryW_target != nullptr)
    MH_RemoveHook (_loader_hooks.LoadLibraryW_target);
  if (_loader_hooks.LoadPackagedLibrary_target != nullptr)
    MH_RemoveHook (_loader_hooks.LoadPackagedLibrary_target);
  if (_loader_hooks.LoadLibraryExA_target != nullptr)
    MH_RemoveHook (_loader_hooks.LoadLibraryExA_target);
  if (_loader_hooks.LoadLibraryExW_target != nullptr)
    MH_RemoveHook (_loader_hooks.LoadLibraryExW_target);
  if (_loader_hooks.FreeLibrary_target != nullptr)
    MH_RemoveHook (_loader_hooks.FreeLibrary_target);

  _loader_hooks.LoadLibraryW_target        = nullptr;
  _loader_hooks.LoadLibraryA_target        = nullptr;
  _loader_hooks.LoadPackagedLibrary_target = nullptr;
  _loader_hooks.LoadLibraryExW_target      = nullptr;
  _loader_hooks.LoadLibraryExA_target      = nullptr;
  _loader_hooks.FreeLibrary_target         = nullptr;

  // Re-establish the non-hooked functions
  SK_PreInitLoadLibrary ();

  SK_UnlockDllLoader ();
}



void
__stdcall
SK_InitCompatBlacklist (void)
{
  memset (&_loader_hooks, 0, sizeof sk_loader_hooks_t);
  
  //SK_CreateDLLHook2 (      L"kernel32.dll",
  //                          "GetModuleHandleA",
  //                           GetModuleHandleA_Detour,
  //  static_cast_p2p <void> (&GetModuleHandleA_Original),
  //            &_loader_hooks.GetModuleHandleA_target );
  //
  //MH_QueueEnableHook (_loader_hooks.GetModuleHandleA_target);
  //
  //SK_CreateDLLHook2 (      L"kernel32.dll",
  //                          "GetModuleHandleW",
  //                           GetModuleHandleW_Detour,
  //  static_cast_p2p <void> (&GetModuleHandleW_Original),
  //            &_loader_hooks.GetModuleHandleW_target );
  //
  //MH_QueueEnableHook (_loader_hooks.GetModuleHandleW_target);
  //
  //SK_CreateDLLHook2 (      L"kernel32.dll",
  //                          "GetModuleHandleExA",
  //                           GetModuleHandleExA_Detour,
  //  static_cast_p2p <void> (&GetModuleHandleExA_Original),
  //            &_loader_hooks.GetModuleHandleExA_target );
  //
  //MH_QueueEnableHook (_loader_hooks.GetModuleHandleExA_target);
  //
  //SK_CreateDLLHook2 (      L"kernel32.dll",
  //                          "GetModuleHandleExW",
  //                           GetModuleHandleExW_Detour,
  //  static_cast_p2p <void> (&GetModuleHandleExW_Original),
  //            &_loader_hooks.GetModuleHandleExW_target );
  //
  //MH_QueueEnableHook (_loader_hooks.GetModuleHandleExW_target);
  //
  //SK_CreateDLLHook2 (      L"kernel32.dll",
  //                          "GetModuleFileNameA",
  //                           GetModuleFileNameA_Detour,
  //  static_cast_p2p <void> (&GetModuleFileNameA_Original),
  //            &_loader_hooks.GetModuleFileNameA_target );
  //
  //MH_QueueEnableHook (_loader_hooks.GetModuleFileNameA_target);
  //
  //SK_CreateDLLHook2 (      L"kernel32.dll",
  //                          "GetModuleFileNameW",
  //                           GetModuleFileNameW_Detour,
  //  static_cast_p2p <void> (&GetModuleFileNameW_Original),
  //            &_loader_hooks.GetModuleFileNameW_target );
  //
  //MH_QueueEnableHook (_loader_hooks.GetModuleFileNameW_target);
  
  SK_ReHookLoadLibrary ();
}


static bool loaded_gl     = false;
static bool loaded_vulkan = false;
static bool loaded_d3d9   = false;
#ifndef _WIN64
static bool loaded_d3d8   = false;
static bool loaded_ddraw  = false;
#endif
static bool loaded_dxgi   = false;


struct enum_working_set_s {
  HMODULE       modules [1024] = {     };
  int           count          =   0;
  iSK_Logger*   logger         = nullptr;
  HANDLE        proc           = INVALID_HANDLE_VALUE;
  SK_ModuleEnum when           = SK_ModuleEnum::Checkpoint;
};


#include <unordered_set>
std::unordered_set <HMODULE> logged_modules;

void
_SK_SummarizeModule ( LPVOID   base_addr,  size_t      mod_size,
                      HMODULE  hMod,       uintptr_t   addr,
                      wchar_t* wszModName, iSK_Logger* pLogger )
{
  ULONG ulLen =  0;

#ifdef _DEBUG
    char  szSymbol [512] = { };
    //~~~ This is too slow and useless
    ulLen = SK_GetSymbolNameFromModuleAddr (hMod, addr, szSymbol, ulLen);
#else
  UNREFERENCED_PARAMETER (ulLen);
  UNREFERENCED_PARAMETER (hMod);
  UNREFERENCED_PARAMETER (addr);
#endif

  std::wstring ver_str =
    SK_GetDLLVersionStr (wszModName);

#ifdef _DEBUG
  if (ulLen != 0)
  {
    pLogger->Log ( L"[ Module ]  ( %ph + %08u )   -:< %-64hs >:-   %s",
                   static_cast   <void *>   (base_addr),
                     static_cast <uint32_t> (mod_size),
                      szSymbol, SK_StripUserNameFromPathW (wszModName) );
  }

  else
#endif
  {
    pLogger->Log ( L"[ Module ]  ( %ph + %08i )       %-64hs       %s",
                    static_cast   <void *>  (base_addr),
                      static_cast <int32_t> (mod_size),
                        "", SK_StripUserNameFromPathW (wszModName) );
  }

  if (ver_str != L"  ")
  {
    pLogger->LogEx ( true,
      L"[File Ver]    %s\n",
        ver_str.c_str () );
  }

  pLogger->LogEx (false, L"\n");
}

void
SK_ThreadWalkModules (enum_working_set_s* pWorkingSet)
{
#ifdef REAL_THREAD
CreateThread (nullptr, 0, [](LPVOID user) -> DWORD
{
#else
{
  auto user =
    static_cast <LPVOID> (pWorkingSet);
#endif
  static volatile LONG    init           = FALSE;
  static CRITICAL_SECTION cs_thread_walk = { };

  if (! InterlockedCompareExchange (&init, 1, 0))
  {
    InitializeCriticalSectionAndSpinCount (&cs_thread_walk, 32);
    InterlockedIncrement                  (&init);
  }

  SK_Thread_SpinUntilAtomicMin (&init, 2);


  EnterCriticalSection (&cs_thread_walk);

  auto* pWorkingSet_ =
    static_cast <enum_working_set_s *> (
      malloc (sizeof (enum_working_set_s))
    );

  memcpy (pWorkingSet_, user, sizeof (enum_working_set_s));

  iSK_Logger* pLogger =
    pWorkingSet_->logger;

  // Workaround silly SEH restrictions
  auto __SEH_Compliant_EmplaceModule =
  [&](enum_working_set_s* pWorkingSet, int idx)
   {
     logged_modules.emplace (pWorkingSet->modules [idx]);
   };


  for (int i = 0; i < pWorkingSet_->count; i++ )
  {
    wchar_t wszModName [MAX_PATH + 2] = { };

    __try
    {
      // Get the full path to the module's file.
      if ( (! logged_modules.count (pWorkingSet_->modules [i])) &&
              GetModuleFileNameExW ( pWorkingSet_->proc,
                                       pWorkingSet_->modules [i],
                                        wszModName,
                                          MAX_PATH ) )
      {
        MODULEINFO mi = { };

        uintptr_t entry_pt  = 0;
        uintptr_t base_addr = 0;
        uint32_t  mod_size  = 0UL;

        if (GetModuleInformation (pWorkingSet_->proc, pWorkingSet_->modules [i], &mi, sizeof (MODULEINFO)))
        {
          entry_pt  = reinterpret_cast <uintptr_t> (mi.EntryPoint);
          base_addr = reinterpret_cast <uintptr_t> (mi.lpBaseOfDll);
          mod_size  =                               mi.SizeOfImage;
        }
        else {
          break;
        }

        _SK_SummarizeModule ( reinterpret_cast <void *> (base_addr),
                                mod_size,
                                  pWorkingSet_->modules [i],
                                    entry_pt,
                                      wszModName,
                                        pLogger );

        __SEH_Compliant_EmplaceModule (pWorkingSet_, i);
      }
    }

    __except ( ( GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ||
                 GetExceptionCode () == EXCEPTION_INVALID_HANDLE )  ?
                         EXCEPTION_EXECUTE_HANDLER :
                         EXCEPTION_CONTINUE_SEARCH )
    {
      // Sometimes a DLL will be unloaded in the middle of doing this... just ignore that.
    }
  };

  CloseHandle (pWorkingSet_->proc);

  LeaveCriticalSection (&cs_thread_walk);

  free (pWorkingSet_);

#ifdef REAL_THREAD
  CloseHandle (GetCurrentThread ());

  return 0;
}, static_cast <LPVOID> (WorkingSet), 0x00, nullptr);
#else
}
#endif
}

void
SK_BootModule (const wchar_t* wszModName)
{
  if (SK_IsInjected ())
  {
    if ( config.apis.OpenGL.hook && StrStrIW (wszModName, L"opengl32.dll") &&
               (SK_IsInjected () || (! (SK_GetDLLRole () & DLL_ROLE::OpenGL))) )
    {
      SK_BootOpenGL ();

      loaded_gl = true;
    }

#ifdef _WIN64
  else if ( config.apis.Vulkan.hook && StrStrIW (wszModName, L"vulkan-1.dll") &&
                  (SK_IsInjected () || (! (SK_GetDLLRole () & DLL_ROLE::Vulkan))) )
  {
    SK_BootVulkan ();

    loaded_vulkan = true;
  }
#endif

  else if ( config.apis.dxgi.d3d11.hook && StrStrIW (wszModName, L"d3d11.dll") &&
                      (SK_IsInjected () || (! (SK_GetDLLRole () & DLL_ROLE::DXGI))) )
  {
    SK_BootDXGI ();

    loaded_dxgi = true;
  }

#ifdef _WIN64
  else if ( config.apis.dxgi.d3d12.hook && StrStrIW (wszModName, L"d3d12.dll") &&
                      (SK_IsInjected () || (! (SK_GetDLLRole () & DLL_ROLE::DXGI))) )
  {
    SK_BootDXGI ();

    loaded_dxgi = true;
  }
#endif

  else if ( config.apis.d3d9.hook && StrStrIW (wszModName, L"d3d9.dll") &&
                (SK_IsInjected () || (! (SK_GetDLLRole () & DLL_ROLE::D3D9))) )
  {
    SK_BootD3D9 ();

    loaded_d3d9 = true;
  }

#ifndef _WIN64
  else if ( config.apis.d3d8.hook && StrStrIW (wszModName, L"d3d8.dll") &&
                (SK_IsInjected () || (! (SK_GetDLLRole () & DLL_ROLE::D3D8))) )
  {
    SK_BootD3D8 ();

    loaded_d3d8 = true;
  }

  else if ( config.apis.ddraw.hook && StrStrIW (wszModName, L"\\ddraw.dll") &&
                 (SK_IsInjected () || (! (SK_GetDLLRole () & DLL_ROLE::DDraw))) )
  {
    SK_BootDDraw ();

    loaded_ddraw = true;
  }
#endif
  }

  if (std::wstring (wszModName).find (SK_GetHostPath ()) != std::wstring::npos)
  {
    bool gl    = false, vulkan = false,
         d3d9  = false,
         dxgi  = false, d3d11  = false,
         d3d8  = false, ddraw  = false,
         glide = false;

    SK_TestRenderImports (GetModuleHandleW (wszModName), &gl, &vulkan, &d3d9, &dxgi, &d3d11, &d3d8, &ddraw, &glide);

#ifndef _WIN64
    if (ddraw)
      SK_BootDDraw ();
    if (d3d8)
      SK_BootD3D8  ();
#endif
    if (dxgi)
      SK_BootDXGI ();
    if (d3d9)
      SK_BootD3D9 ();
    if (gl)
      SK_BootOpenGL ();
  }
}

void
SK_WalkModules (int cbNeeded, HANDLE hProc, HMODULE* hMods, SK_ModuleEnum when)
{
  bool rehook    = false;
  bool new_hooks = false;

  for ( int i = 0; i < static_cast <int> (cbNeeded / sizeof (HMODULE)); i++ )
  {
    wchar_t wszModName [MAX_PATH + 2] = { };

    __try
    {
      // Get the full path to the module's file.
      if ( GetModuleFileNameExW ( hProc,
                                    hMods [i],
                                      wszModName,
                                        MAX_PATH ) )
      {
        if (! rehook)
        {
          if ( (! third_party_dlls.overlays.rtss_hooks) &&
                StrStrIW (wszModName, L"RTSSHooks") )
          {
            // Hold a reference to this DLL so it is not unloaded prematurely
            GetModuleHandleEx ( 0x0,
                                  wszModName,
                                    &third_party_dlls.overlays.rtss_hooks );

            rehook = config.compatibility.rehook_loadlibrary;
          }

          else if ( (! third_party_dlls.overlays.steam_overlay) &&
                     StrStrIW (wszModName, L"gameoverlayrenderer") )
          {
            // Hold a reference to this DLL so it is not unloaded prematurely
            GetModuleHandleEx ( 0x0,
                                  wszModName,
                                    &third_party_dlls.overlays.steam_overlay );

            rehook = config.compatibility.rehook_loadlibrary;
          }
        }

        if (when == SK_ModuleEnum::PostLoad)
        {
          SK_BootModule (wszModName);
        }

        if (! config.steam.silent)
        {
          if ( StrStrIW (wszModName, wszSteamAPIDLL)    ||
               StrStrIW (wszModName, wszSteamAPIAltDLL) ||
               StrStrIW (wszModName, wszSteamNativeDLL) ||
               StrStrIW (wszModName, wszSteamClientDLL) ||
               StrStrIW (wszModName, L"steamwrapper") )
          {
            BOOL
            SK_Steam_PreHookCore (void);

            if (SK_Steam_PreHookCore ())
              new_hooks = true;

            if (SK_HookSteamAPI () > 0)
              new_hooks = true;
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

  if (rehook)
  {
    // We took care of ApplyQueuedHooks already
    if (SK_ReHookLoadLibrary ())
      return;
  }

  if (rehook || new_hooks || when == SK_ModuleEnum::PostLoad)
  {
    SK_ApplyQueuedHooks ();
  }
}

void
SK_PrintUnloadedDLLs (iSK_Logger* pLogger)
{
  typedef struct _RTL_UNLOAD_EVENT_TRACE {
      PVOID BaseAddress;   // Base address of dll
      SIZE_T SizeOfImage;  // Size of image
      ULONG Sequence;      // Sequence number for this event
      ULONG TimeDateStamp; // Time and date of image
      ULONG CheckSum;      // Image checksum
      WCHAR ImageName[32]; // Image name
  } RTL_UNLOAD_EVENT_TRACE, *PRTL_UNLOAD_EVENT_TRACE;

  using RtlGetUnloadEventTraceEx_pfn = void (WINAPI *)(
    _Out_ PULONG *ElementSize,
    _Out_ PULONG *ElementCount,
    _Out_ PVOID  *EventTrace
  );

  HMODULE hModNtDLL =
    LoadLibraryW (L"ntdll.dll");

  auto RtlGetUnloadEventTraceEx =
    (RtlGetUnloadEventTraceEx_pfn)GetProcAddress (hModNtDLL, "RtlGetUnloadEventTraceEx");

  PULONG element_size  = nullptr,
         element_count = nullptr;
  PVOID trace_log      = nullptr;

  RtlGetUnloadEventTraceEx (&element_size, &element_count, &trace_log);

  RTL_UNLOAD_EVENT_TRACE* pTraceEntry =
    *(RTL_UNLOAD_EVENT_TRACE **)trace_log;

  ULONG ElementSize  = *element_size;
  ULONG ElementCount = *element_count;

  if (ElementCount > 0)
  {
    pLogger->LogEx ( false, 
      L"================================================================== "
      L"(List Unloads) "
      L"==================================================================\n" );

    for (ULONG i = 0; i < ElementCount; i++)
    {
      if (! SK_ValidatePointer (pTraceEntry))
        break;

      if (pTraceEntry->BaseAddress != nullptr)
      {
        pLogger->Log ( L"[%lu] Unloaded '%32ws' [ (0x%p) : (0x%p) ]",
                      pTraceEntry->Sequence,    pTraceEntry->ImageName,
                      pTraceEntry->BaseAddress, (uintptr_t)pTraceEntry->BaseAddress +
                                                           pTraceEntry->SizeOfImage );
      }

      pTraceEntry =
        (RTL_UNLOAD_EVENT_TRACE *)((uintptr_t)pTraceEntry + ElementSize);
    }
  }

  FreeLibrary (hModNtDLL);
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
  DWORD        cbNeeded =   0;
  HANDLE       hProc    =
    // Get a handle to the process.
    OpenProcess ( PROCESS_QUERY_INFORMATION |
                  PROCESS_VM_READ,
                    FALSE,
                      dwProcID );

  if (pLogger != nullptr) pLogger->flush_freq = 0;
  if (hProc == nullptr)
  {
    return;
  }

  auto *working_set =
    new enum_working_set_s ();

  if ( EnumProcessModules ( hProc,
                              working_set->modules,
                      sizeof (working_set->modules),
                                  &cbNeeded) )
  {
    assert (cbNeeded <= sizeof (working_set->modules));

    working_set->proc   = hProc;
    working_set->logger = pLogger;
    working_set->count  = cbNeeded / sizeof HMODULE;
    working_set->when   = when;

    if (when == SK_ModuleEnum::PreLoad)
    {
      SK_WalkModules (cbNeeded, hProc, working_set->modules, when);
    }

    CreateThread (nullptr, 0, [](LPVOID user) -> DWORD
    {
      SetCurrentThreadDescription (L"[SK] DLL Enumeration Thread");

      SetThreadPriority (GetCurrentThread (), THREAD_PRIORITY_LOWEST);

      SleepEx (1UL, FALSE);

      static volatile LONG                walking = 0;
      while (InterlockedCompareExchange (&walking, 1, 0))
      {
        SleepEx (20UL, FALSE);
      }

      auto CleanupLog = [](iSK_Logger*& pLogger) ->
      void
      {
        if (pLogger != nullptr)
        {
          pLogger->close ();
          delete pLogger;
          pLogger = nullptr;
        }
      };

      // Doing a full enumeration is slow as hell, spawn a worker thread for this
      //   and learn to deal with the fact that some symbol names will be invalid;
      //     the crash handler will load them, but certain diagnostic readings under
      //       normal operation will not.
      auto* pWorkingSet =
        static_cast <enum_working_set_s *> (user);

      SK_ModuleEnum when = pWorkingSet->when;

      if (pWorkingSet->proc == nullptr && (when != SK_ModuleEnum::PreLoad))
      {
        delete pWorkingSet;

        InterlockedExchange (&walking, 0);

        CleanupLog  (pLogger);
        CloseHandle (GetCurrentThread ());

        return 0;
      }

      if ( when != SK_ModuleEnum::PreLoad )
        SK_WalkModules     (pWorkingSet->count * sizeof (HMODULE), pWorkingSet->proc, pWorkingSet->modules, pWorkingSet->when);

      SK_ThreadWalkModules (pWorkingSet);

      if ( when != SK_ModuleEnum::PreLoad && pLogger != nullptr )
      {
        SK_PrintUnloadedDLLs (pLogger);
      }

      if ( when   == SK_ModuleEnum::PreLoad &&
          pLogger != nullptr )
      {
        pLogger->LogEx (
          false,
            L"================================================================== "
            L"(End Preloads) "
            L"==================================================================\n\n"
        );
      }

      if (when != SK_ModuleEnum::PreLoad)
      {
        CleanupLog (pLogger);
      }

      delete pWorkingSet;

      InterlockedExchange (&walking, 0);

      CloseHandle (GetCurrentThread ());

      return 0;
    }, (LPVOID)working_set, 0x00, nullptr);
  }
}

volatile LONG __SK_TaskDialogActive = FALSE;

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

  if (uNotification == TDN_TIMER)
  {
    if (GetFocus            () != hWnd ||
        GetForegroundWindow () != hWnd)
    {
      SK_RealizeForegroundWindow (hWnd);

      SetForegroundWindow (hWnd);
      SetWindowPos        (hWnd, HWND_TOPMOST, 0, 0, 0, 0,
                           SWP_ASYNCWINDOWPOS | SWP_SHOWWINDOW   |
                           SWP_NOSIZE         | SWP_NOMOVE       |
                           SWP_NOSENDCHANGING   );
      SetFocus            (hWnd);
    }
  }

  if (uNotification == TDN_HYPERLINK_CLICKED)
  {
    ShellExecuteW (nullptr, L"open", reinterpret_cast <wchar_t *>(lParam), nullptr, nullptr, SW_SHOW);
    return S_OK;
  }

  // It's important to keep the compatibility menu always on top, far more important than even
  //   the "Always On Top" window style can give us.
  if (uNotification == TDN_DIALOG_CONSTRUCTED)
  {
    InterlockedExchange (&__SK_TaskDialogActive, TRUE);

    SK_RealizeForegroundWindow (hWnd);

    SetForegroundWindow (hWnd);
    SetWindowLongW      (hWnd, GWL_EXSTYLE,
     ( (GetWindowLongW  (hWnd, GWL_EXSTYLE) | (WS_EX_TOPMOST))));
    BringWindowToTop    (hWnd);
  }

  if (uNotification == TDN_CREATED)
  {
  }

  if (uNotification == TDN_DESTROYED)
  {
    SK_RealizeForegroundWindow (game_window.hWnd);
    SetForegroundWindow        (game_window.hWnd);
    SetFocus                   (game_window.hWnd);
    SetActiveWindow            (game_window.hWnd);
    InterlockedExchange   (&__SK_TaskDialogActive, FALSE);
  }

  return S_OK;
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
  // TODO
  UNREFERENCED_PARAMETER (wszConfirmation);

  const bool timer = true;

  int              nButtonPressed = 0;
  TASKDIALOGCONFIG task_config    = {0};

  task_config.cbSize              = sizeof (task_config);
  task_config.hInstance           = __SK_HMODULE_0;
  task_config.hwndParent          =                   nullptr;
  task_config.pszWindowTitle      = L"Special K Compatibility Layer (v " SK_VERSION_STR_W L")";
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
  UNREFERENCED_PARAMETER (wszConfirmation);

  const bool timer = true;

  int              nButtonPressed =   0;
  TASKDIALOGCONFIG task_config    = {   };

  task_config.cbSize              = sizeof    (task_config);
  task_config.hInstance           = __SK_HMODULE_0;
  task_config.hwndParent          =        nullptr;
  task_config.pszWindowTitle      = L"Special K Compatibility Layer (v " SK_VERSION_STR_W L")";
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

enum {
  SK_BYPASS_UNKNOWN    = 0x0,
  SK_BYPASS_ACTIVATE   = 0x1,
  SK_BYPASS_DEACTIVATE = 0x2
};

volatile LONG SK_BypassResult = SK_BYPASS_UNKNOWN;

struct sk_bypass_s {
  BOOL    disable;
  wchar_t wszBlacklist [MAX_PATH];
} __bypass;



LPVOID pfnGetClientRect    = nullptr;
LPVOID pfnGetWindowRect    = nullptr;
LPVOID pfnGetSystemMetrics = nullptr;

std::queue <DWORD> suspended_tids;


DWORD
WINAPI
SK_Bypass_CRT (LPVOID)
{
  SetCurrentThreadDescription (L"[SK] Compatibility Layer Dialog");

  static BOOL     disable      = __bypass.disable;
         wchar_t* wszBlacklist = __bypass.wszBlacklist;

  bool timer = true;

  int              nButtonPressed = 0;
  TASKDIALOGCONFIG task_config    = {0};

  enum {
    BUTTON_INSTALL         = 0,
    BUTTON_UNINSTALL       = 0,
    BUTTON_OK              = 1,
    BUTTON_DISABLE_PLUGINS = 2,
    BUTTON_RESET_CONFIG    = 3,
  };

  // The global injector; local wrapper not installed for the current game
  const TASKDIALOG_BUTTON  buttons_global [] = {  { BUTTON_INSTALL,         L"Install Wrapper DLL" },
                                                  { BUTTON_DISABLE_PLUGINS, L"Disable Plug-Ins"    },
                                                  { BUTTON_RESET_CONFIG,    L"Reset Config"        },
                                               };

  // Wrapper installation in system containing global version of Special K
  const TASKDIALOG_BUTTON  buttons_local  [] = {  { BUTTON_UNINSTALL,       L"Uninstall Wrapper DLL" },
                                                  { BUTTON_DISABLE_PLUGINS, L"Disable Plug-Ins"      },
                                                  { BUTTON_RESET_CONFIG,    L"Reset Config"          },
                                               };

  // Standalone Special K mod; no global injector detected
  const TASKDIALOG_BUTTON  buttons_standalone  [] = {  { BUTTON_UNINSTALL,       L"Uninstall Mod"    },
                                                       { BUTTON_DISABLE_PLUGINS, L"Disable Plug-Ins" },
                                                       { BUTTON_RESET_CONFIG,    L"Reset Config"     },
                                                    };

  bool gl     = false,
       vulkan = false,
       d3d9   = false,
       d3d8   = false,
       dxgi   = false,
       ddraw  = false,
       d3d11  = false,
       glide  = false;

  SK_TestRenderImports (__SK_HMODULE_0, &gl, &vulkan, &d3d9, &dxgi, &d3d11, &d3d8, &ddraw, &glide);

  const bool no_imports = !(gl || vulkan || d3d9 || d3d8 || ddraw || d3d11 || glide);

  auto dgVoodoo_Check = [](void) ->
    bool
    {
      return
        GetFileAttributesA (
          SK_FormatString ( R"(%ws\PlugIns\ThirdParty\dgVoodoo\d3dimm.dll)",
                              std::wstring ( SK_GetDocumentsDir () + LR"(\My Mods\SpecialK)" ).c_str ()
                          ).c_str ()
        ) != INVALID_FILE_ATTRIBUTES;
    };

  auto dgVooodoo_Nag = [&](void) ->
    bool
    {
      while (
        MessageBox (HWND_DESKTOP, L"dgVoodoo is required for Direct3D8 / DirecrtDraw support\r\n\t"
                                  L"Please install its DLLs to 'Documents\\My Mods\\SpecialK\\PlugIns\\ThidParty\\dgVoodoo'",
                                    L"Third-Party Plug-In Required",
                                      MB_ICONSTOP | MB_RETRYCANCEL) == IDRETRY && (! dgVoodoo_Check ())
            ) ;

      return dgVoodoo_Check ();
    };

  const bool has_dgvoodoo = dgVoodoo_Check ();

  const wchar_t* wszAPI = SK_GetBackend ();

  if (config.apis.last_known != SK_RenderAPI::Reserved)
  {
    switch (static_cast <SK_RenderAPI> (config.apis.last_known))
    {
      case SK_RenderAPI::D3D8:
      case SK_RenderAPI::D3D8On11:
        wszAPI = L"d3d8";
        SK_SetDLLRole (DLL_ROLE::D3D8);
        break;
      case SK_RenderAPI::DDraw:
      case SK_RenderAPI::DDrawOn11:
        wszAPI = L"ddraw";
        SK_SetDLLRole (DLL_ROLE::DDraw);
        break;
      case SK_RenderAPI::D3D10:
      case SK_RenderAPI::D3D11:
      case SK_RenderAPI::D3D12:
        wszAPI = L"dxgi";
        SK_SetDLLRole (DLL_ROLE::DXGI);
        break;
      case SK_RenderAPI::D3D9:
      case SK_RenderAPI::D3D9Ex:
        wszAPI = L"d3d9";
        SK_SetDLLRole (DLL_ROLE::D3D9);
        break;
      case SK_RenderAPI::OpenGL:
        wszAPI = L"OpenGL32";
        SK_SetDLLRole (DLL_ROLE::OpenGL);
        break;
      case SK_RenderAPI::Vulkan:
        wszAPI = L"vulkan-1";
        SK_SetDLLRole (DLL_ROLE::Vulkan);
        break;
    }
  }

  static wchar_t wszAPIName [128] = { L"Auto-Detect   " };
       lstrcatW (wszAPIName, L"(");
       lstrcatW (wszAPIName, wszAPI);
       lstrcatW (wszAPIName, L")");

  if (no_imports && config.apis.last_known == SK_RenderAPI::Reserved)
    lstrcatW (wszAPIName, L"  { Import Address Table FAIL -> Detected API May Be Wrong }");

  const TASKDIALOG_BUTTON rbuttons [] = {  { 0, wszAPIName          },
                                           { 1, L"Enable All APIs"  },
#ifndef _WIN64
                                           { 7, L"Direct3D8"        },
                                           { 8, L"DirectDraw"       },
#endif
                                           { 2, L"Direct3D9{Ex}"    },
                                           { 3, L"Direct3D11"       },
#ifdef _WIN64
                                           { 4, L"Direct3D12"       },
#endif
                                           { 5, L"OpenGL"           },
#ifdef _WIN64
                                           { 6, L"Vulkan"           }
#endif
                                       };

  task_config.cbSize              = sizeof (task_config);
  task_config.hInstance           = __SK_HMODULE_0;
  task_config.hwndParent          =        nullptr;
  task_config.pszWindowTitle      = L"Special K Compatibility Layer (v " SK_VERSION_STR_W L")";
  task_config.dwCommonButtons     = TDCBF_OK_BUTTON;
  task_config.pRadioButtons       = rbuttons;
  task_config.cRadioButtons       = ARRAYSIZE (rbuttons);

  if (SK_IsInjected ())
  {
    task_config.pButtons          =            buttons_global;
    task_config.cButtons          = ARRAYSIZE (buttons_global);
  }

  else
  {
    if (SK_HasGlobalInjector ())
    {
      task_config.pButtons        =            buttons_local;
      task_config.cButtons        = ARRAYSIZE (buttons_local);
    }

    else
    {
      task_config.pButtons        =            buttons_standalone;
      task_config.cButtons        = ARRAYSIZE (buttons_standalone);
    }
  }

  task_config.dwFlags             = 0x00;
  task_config.pfCallback          = TaskDialogCallback;
  task_config.lpCallbackData      = 0;
  task_config.nDefaultButton      = TDCBF_OK_BUTTON;

  task_config.pszMainInstruction  = L"Special K Injection Compatibility Options";

  task_config.pszMainIcon         = TD_SHIELD_ICON;
  task_config.pszContent          = L"Compatibility Mode has been Activated by Pressing and "
                                    L"Holding Ctrl + Shift"
                                    L"\n\nUse this menu to troubleshoot problems caused by"
                                    L" the software";

  if (SK_IsInjected ())
  {
    task_config.pszFooterIcon       = TD_INFORMATION_ICON;
    task_config.pszFooter           = L"WARNING: Installing the wrong API wrapper may require "
                                      L"manual recovery; if in doubt, use AUTO.";
    task_config.pszVerificationText = L"Disable Global Injection for this Game";
  }

  else
  {
    task_config.pszFooter           = nullptr;
    task_config.pszVerificationText = nullptr;
  }

  if (__bypass.disable)
  task_config.dwFlags |= TDF_VERIFICATION_FLAG_CHECKED;

  if (timer)
    task_config.dwFlags |= TDF_CALLBACK_TIMER;

  int nRadioPressed = 0;


  HRESULT hr =
    TaskDialogIndirect ( &task_config,
                          &nButtonPressed,
                            &nRadioPressed,
                              SK_IsInjected () ? &disable : nullptr );


  extern iSK_INI* dll_ini;
  const  wchar_t* wszConfigName = L"";

  if (SK_IsInjected ())
    wszConfigName = L"SpecialK";

  else
  {
    wszConfigName = dll_ini->get_filename ();
    //wszConfigName = SK_GetBackend ();
  }

  std::wstring temp_dll = L"";


  SK_LoadConfig (wszConfigName);

  config.apis.d3d9.hook       = false;
  config.apis.d3d9ex.hook     = false;
  config.apis.dxgi.d3d11.hook = false;

  config.apis.OpenGL.hook     = false;

#ifdef _WIN64
  config.apis.dxgi.d3d12.hook = false;
  config.apis.Vulkan.hook     = false;
#else
  config.apis.d3d8.hook       = false;
  config.apis.ddraw.hook      = false;
#endif

  if (SUCCEEDED (hr))
  {
    if (nRadioPressed == 0)
    {
      if (SK_GetDLLRole () & DLL_ROLE::DXGI)
      {
        config.apis.dxgi.d3d11.hook = true;
#ifdef _WIN64
        config.apis.dxgi.d3d12.hook = true;
#endif
      }

      if (SK_GetDLLRole () & DLL_ROLE::D3D9)
      {
        config.apis.d3d9.hook   = true;
        config.apis.d3d9ex.hook = true;
      }

      if (SK_GetDLLRole () & DLL_ROLE::OpenGL)
      {
        config.apis.OpenGL.hook = true;
      }

#ifndef _WIN64
      if (SK_GetDLLRole () & DLL_ROLE::D3D8)
      {
        config.apis.d3d8.hook       = true;
        config.apis.dxgi.d3d11.hook = true;
      }

      if (SK_GetDLLRole () & DLL_ROLE::DDraw)
      {
        config.apis.ddraw.hook      = true;
        config.apis.dxgi.d3d11.hook = true;
      }
#else
#endif
    }

    switch (nRadioPressed)
    {
      case 0:
        if (nButtonPressed == BUTTON_INSTALL)
        {
          if (SK_IsInjected ()) SK_Inject_SwitchToRenderWrapperEx (SK_GetDLLRole ());
          else
          {
            SK_Inject_SwitchToGlobalInjector ();

            temp_dll = SK_UTF8ToWideChar (
                         SK_FormatString ( R"(%ws\My Mods\SpecialK\SpecialK%lu.dll)",
                           SK_GetDocumentsDir ().c_str (),
#ifndef _WIN64
                             32
#else
                             64
#endif
                         )
                      );
            }
        }
        break;

      case 1:
        config.apis.d3d9.hook       = true;
        config.apis.d3d9ex.hook     = true;

        config.apis.dxgi.d3d11.hook = true;

        config.apis.OpenGL.hook     = true;

#ifndef _WIN64
        config.apis.d3d8.hook       = true;
        config.apis.ddraw.hook      = true;
#else
        config.apis.Vulkan.hook     = true;
        config.apis.dxgi.d3d12.hook = true;
#endif

        if (nButtonPressed == BUTTON_INSTALL)
        {
          if (SK_IsInjected ()) SK_Inject_SwitchToRenderWrapperEx  (SK_GetDLLRole ());
          else
          {
            SK_Inject_SwitchToGlobalInjector ();

            temp_dll = SK_UTF8ToWideChar (
                         SK_FormatString ( R"(%ws\My Mods\SpecialK\SpecialK%lu.dll)",
                           SK_GetDocumentsDir ().c_str (),
#ifndef _WIN64
                             32
#else
                             64
#endif
                         )
                      );
            }
        }
        break;

      case 2:
        config.apis.d3d9.hook       = true;
        config.apis.d3d9ex.hook     = true;

        if (nButtonPressed == BUTTON_INSTALL)
        {
          if (SK_IsInjected ())
          {
            //temp_dll = L"d3d9.dll";
            SK_Inject_SwitchToRenderWrapperEx (DLL_ROLE::D3D9);
          }

          else
          {
            temp_dll = SK_UTF8ToWideChar (
                         SK_FormatString ( R"(%ws\My Mods\SpecialK\SpecialK%lu.dll)",
                           SK_GetDocumentsDir ().c_str (),
#ifndef _WIN64
                             32
#else
                             64
#endif
                         )
                      );
            SK_Inject_SwitchToGlobalInjector ();
          }
        }
        break;

      case 3:
        config.apis.dxgi.d3d11.hook = true;

        if (nButtonPressed == BUTTON_INSTALL)
        {
          if (SK_IsInjected ())
          {
            //temp_dll = L"dxgi.dll";
            SK_Inject_SwitchToRenderWrapperEx (DLL_ROLE::DXGI);
          }

          else
          {
            SK_Inject_SwitchToGlobalInjector ();
            temp_dll = SK_UTF8ToWideChar (
                         SK_FormatString ( R"(%ws\My Mods\SpecialK\SpecialK%lu.dll)",
                           SK_GetDocumentsDir ().c_str (),
#ifndef _WIN64
                             32
#else
                             64
#endif
                         )
                      );
          }
        }
        break;

#ifdef _WIN64
      case 4:
        config.apis.dxgi.d3d11.hook = true; // Required by D3D12 code :P
        config.apis.dxgi.d3d12.hook = true;

        if (nButtonPressed == BUTTON_INSTALL)
        {
          if (SK_IsInjected ())
          {
            //temp_dll = L"dxgi.dll";
            SK_Inject_SwitchToRenderWrapperEx (DLL_ROLE::DXGI);
          }
          else
          {
            SK_Inject_SwitchToGlobalInjector ();
            temp_dll = SK_UTF8ToWideChar (
                         SK_FormatString ( R"(%ws\My Mods\SpecialK\SpecialK%lu.dll)",
                           SK_GetDocumentsDir ().c_str (),
#ifndef _WIN64
                             32
#else
                             64
#endif
                         )
                      );
            }
        }
        break;
#endif

      case 5:
        config.apis.OpenGL.hook = true;

        if (nButtonPressed == BUTTON_INSTALL)
        {
          if (SK_IsInjected ())
          {
            //temp_dll = L"OpenGL32.dll";
            SK_Inject_SwitchToRenderWrapperEx (DLL_ROLE::OpenGL);
          }

          else
          {
            SK_Inject_SwitchToGlobalInjector ();
            temp_dll = SK_UTF8ToWideChar (
                         SK_FormatString ( R"(%ws\My Mods\SpecialK\SpecialK%lu.dll)",
                           SK_GetDocumentsDir ().c_str (),
#ifndef _WIN64
                             32
#else
                             64
#endif
                         )
                      );
          }
        }
        break;

#ifdef _WIN64
      case 6:
        config.apis.Vulkan.hook = true;
        break;
#endif

#ifndef _WIN64
      case 7:
        config.apis.dxgi.d3d11.hook = true;  // D3D8 on D3D11 (not native D3D8)
        config.apis.d3d8.hook       = true;

        if (has_dgvoodoo || dgVooodoo_Nag ())
        {
          if (nButtonPressed == BUTTON_INSTALL)
          {
            if (SK_IsInjected ())
            {
              SK_Inject_SwitchToRenderWrapperEx (DLL_ROLE::D3D8);
            }

            else
            {
              SK_Inject_SwitchToGlobalInjector ();
              temp_dll = SK_UTF8ToWideChar (
                           SK_FormatString ( R"(%ws\My Mods\SpecialK\SpecialK%lu.dll)",
                             SK_GetDocumentsDir ().c_str (),
#ifndef _WIN64
                               32
#else
                               64
#endif
                           )
                        );
            }
          }
        }
        break;

      case 8:
        config.apis.dxgi.d3d11.hook = true;  // DDraw on D3D11 (not native DDraw)
        config.apis.ddraw.hook      = true;

        if (nButtonPressed == BUTTON_INSTALL)
        {
          if (has_dgvoodoo || dgVooodoo_Nag ())
          {
            if (SK_IsInjected ())
              SK_Inject_SwitchToRenderWrapperEx (DLL_ROLE::DDraw);
            else
            {
              SK_Inject_SwitchToGlobalInjector ();

              temp_dll = SK_UTF8ToWideChar (
                           SK_FormatString ( R"(%ws\My Mods\SpecialK\SpecialK%lu.dll)",
                             SK_GetDocumentsDir ().c_str (),
#ifndef _WIN64
                               32
#else
                               64
#endif
                           )
                        );
            }
          }
        }
        break;
#endif

      default:
        break;
    }


    if (nButtonPressed == BUTTON_DISABLE_PLUGINS)
    {
      // TEMPORARY: There will be a function to disable plug-ins here, for now
      //              just disable ReShade.
#ifdef _WIN64
      dll_ini->remove_section (L"Import.ReShade64");
      dll_ini->remove_section (L"Import.ReShade64_Custom");
#else
      dll_ini->remove_section (L"Import.ReShade32");
      dll_ini->remove_section (L"Import.ReShade32_Custom");
#endif
      dll_ini->write (dll_ini->get_filename ());
    }

    if (nButtonPressed == BUTTON_RESET_CONFIG)
    {
      std::wstring fname = dll_ini->get_filename ();

      delete dll_ini;

      SK_DeleteConfig (fname.c_str ());
      SK_DeleteConfig (wszConfigName);

      // Hard-code known plug-in config files -- bad (lack of) design.
      //
      SK_DeleteConfig (L"FAR.ini");   SK_DeleteConfig (L"UnX.ini");   SK_DeleteConfig (L"PPrinny.ini");
      SK_DeleteConfig (L"TBFix.ini"); SK_DeleteConfig (L"TSFix.ini"); SK_DeleteConfig (L"TZFix.ini");
    }

    else if (nButtonPressed != BUTTON_OK)
    {
      SK_SaveConfig (wszConfigName);
    }

    if ( nButtonPressed         == BUTTON_INSTALL &&
         no_imports                               &&
         nRadioPressed          == 0 /* Auto */   &&
         config.apis.last_known == SK_RenderAPI::Reserved )
    {
      MessageBoxA   ( HWND_DESKTOP,
                        SK_FormatString ( "API detection may be incorrect, delete '%ws.dll' "
                                          "manually if Special K does not inject "
                                          "itself.", SK_GetBackend () ).c_str (),
                          "Possible API Detection Problems",
                            MB_ICONINFORMATION | MB_OK
                    );
      ShellExecuteW ( HWND_DESKTOP, L"explore", SK_GetHostPath (), nullptr, nullptr, SW_SHOWNORMAL );
    }

    if (disable)
    {
      FILE* fDeny =
        _wfopen (wszBlacklist, L"w");

      if (fDeny != nullptr)
      {
        fputc  ('\0', fDeny);
        fflush (      fDeny);
        fclose (      fDeny);
      }

      InterlockedExchange (&SK_BypassResult, SK_BYPASS_ACTIVATE);
    }

    else
    {
      DeleteFileW         (wszBlacklist);
      InterlockedExchange (&SK_BypassResult, SK_BYPASS_DEACTIVATE);
    }
  }

  //if (nButtonPressed != TDCBF_OK_BUTTON)
  {
    if (! temp_dll.length ())
      SK_RestartGame (nullptr);
    else
      SK_RestartGame (temp_dll.c_str ());

    ExitProcess (0);
  }

  // The old system to try and restart the bootstrapping process
  //   is impractical, restarting the game works better.
  //
  ///else
  ///{
  ///  dll_ini->write (dll_ini->get_filename ());
  ///  SK_SaveConfig  ();
  ///
  ///  SK_EnumLoadedModules (SK_ModuleEnum::PostLoad);
  ///  SK_ResumeThreads     (suspended_tids);
  ///}


  CloseHandle (GetCurrentThread ());

  return 0;
}



std::pair <std::queue <DWORD>, BOOL>
__stdcall
SK_BypassInject (void)
{
  lstrcpyW (__bypass.wszBlacklist, SK_GetBlacklistFilename ());

  __bypass.disable =
    (GetFileAttributesW (__bypass.wszBlacklist) != INVALID_FILE_ATTRIBUTES);

  CreateThread ( nullptr,
                   0,
                     SK_Bypass_CRT, nullptr,
                       0x00,
                         nullptr );

  return
    std::make_pair (suspended_tids, __bypass.disable);
}




void
SK_COMPAT_FixUpFullscreen_DXGI (bool Fullscreen)
{
  if (Fullscreen)
  {
    if (SK_GetCurrentGameID () == SK_GAME_ID::WorldOfFinalFantasy)
    {
      ShowCursor  (TRUE);
      ShowWindow  ( GetForegroundWindow (), SW_HIDE );
      MessageBox  ( GetForegroundWindow (),
                      L"Please re-configure this game to run in windowed mode",
                        L"Special K Conflict",
                          MB_OK        | MB_SETFOREGROUND |
                          MB_APPLMODAL | MB_ICONASTERISK );

      ShellExecuteW (HWND_DESKTOP, L"open", L"WOFF_config.exe", nullptr, nullptr, SW_NORMAL);

      while (SK_IsProcessRunning (L"WOFF_config.exe"))
        SleepEx (250UL, FALSE);

      ShellExecuteW (HWND_DESKTOP, L"open", L"WOFF.exe",        nullptr, nullptr, SW_NORMAL);
      ExitProcess   (0x00);
    }
  }
}



#include <SpecialK/sound.h>

//
// MSI Nahimic sometimes @#$%s the bed of MMDevAPI.dll is not loaded
//   in the right order, so to make it happy, we need a link-time reference
//     to a function in that DLL that won't be optimized away.
//
//   This causes the kernel to load MMDevAPI before Special K and before
//     Nahimic, and now we don't race to be the first to blow up the program.
//
HRESULT
SK_COMPAT_FixNahimicDeadlock (void)
{
  //CComPtr <IActivateAudioInterfaceAsyncOperation> aOp;
  //
  //ActivateAudioInterfaceAsync ( L"I don't exist",
  //                                __uuidof (IAudioClient),
  //                                  nullptr, nullptr, &aOp );
  //
  ////if (GetModuleHandle (L"NahimicDevProps.dll"))
  ////  return S_OK;

  return S_FALSE;
}

void
__stdcall
SK_PreInitLoadLibrary (void)
{
  FreeLibrary_Original         = &FreeLibrary;
  LoadLibraryA_Original        = &LoadLibraryA;
  LoadLibraryW_Original        = &LoadLibraryW;
  LoadLibraryExA_Original      = &LoadLibraryExA;
  LoadLibraryExW_Original      = &LoadLibraryExW;
  GetModuleHandleA_Original    = &GetModuleHandleA;
  GetModuleHandleW_Original    = &GetModuleHandleW;
  GetModuleHandleExA_Original  = &GetModuleHandleExA;
  GetModuleHandleExW_Original  = &GetModuleHandleExW;
  GetModuleFileNameA_Original  = &GetModuleFileNameA;
  GetModuleFileNameW_Original  = &GetModuleFileNameW;
  LoadPackagedLibrary_Original = nullptr; // Windows 8 feature
}

FreeLibrary_pfn    FreeLibrary_Original    = nullptr;

LoadLibraryA_pfn   LoadLibraryA_Original   = nullptr;
LoadLibraryW_pfn   LoadLibraryW_Original   = nullptr;

LoadLibraryExA_pfn LoadLibraryExA_Original = nullptr;
LoadLibraryExW_pfn LoadLibraryExW_Original = nullptr;

LoadPackagedLibrary_pfn LoadPackagedLibrary_Original = nullptr;