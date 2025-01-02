// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
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
#include <SpecialK/storefront/epic.h> // Lazy EOS init

#include <concurrent_unordered_set.h>

__declspec (noinline)
concurrency::concurrent_unordered_set <HMODULE>&
SK_DbgHlp_Callers (void)
{
  static concurrency::concurrent_unordered_set <HMODULE> _callers (64);
  return                                                 _callers;
}

#define SK_CHAR(x) (_T)   (_T      (std::type_index (typeid (_T)) ==      \
                                    std::type_index (typeid (wchar_t))) ? \
                                   (    _T  )(_L(x)) : (      _T  )(x))
#define SK_TEXT(x) \
              (const _T*) (LPCVOID (std::type_index (typeid (_T)) ==      \
                                    std::type_index (typeid (wchar_t))) ? \
                           (const _T *)(_L(x)) : (const _T *)(x))

using StrStrI_pfn            =
PSTR    (__stdcall *)( LPCVOID lpFirst,
                       LPCVOID lpSearch      );
using PathRemoveFileSpec_pfn =
BOOL    (__stdcall *)( LPVOID  lpPath        );
using LoadLibrary_pfn        =
HMODULE (__stdcall *)( LPCVOID lpLibFileName );
using strncpy_pfn            =
LPVOID  (__cdecl   *)( LPVOID  lpDest,
                       LPCVOID lpSource,
                       size_t  nCount        );
using lstrcat_pfn            =
LPVOID  (__stdcall *)( LPVOID  lpString1,
                       LPCVOID lpString2     );
using GetModuleHandleEx_pfn  =
BOOL    (__stdcall *)( DWORD    dwFlags,
                       LPCVOID lpModuleName,
                       HMODULE *phModule     );


bool SK_LoadLibrary_SILENCE = false;


#ifdef _M_AMD64
#define SK_STEAM_BIT_WSTRING L"64"
#define SK_STEAM_BIT_STRING   "64"
#else /* _M_IX86 */
#define SK_STEAM_BIT_WSTRING L""
#define SK_STEAM_BIT_STRING   ""
#endif

static const wchar_t* wszSteamClientDLL = L"SteamClient";
static const char*     szSteamClientDLL =  "SteamClient";

static const wchar_t* wszSteamNativeDLL = L"SteamNative.dll";
static const char*     szSteamNativeDLL =  "SteamNative.dll";

static const wchar_t* wszSteamAPIDLL    =
         L"steam_api" SK_STEAM_BIT_WSTRING L".dll";
static const char*     szSteamAPIDLL    =
          "steam_api" SK_STEAM_BIT_STRING   ".dll";
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
};

SK_LazyGlobal <sk_loader_hooks_t> _loader_hooks;


FreeLibrary_pfn    FreeLibrary_Original    = nullptr;

LoadLibraryA_pfn   LoadLibraryA_Original   = nullptr;
LoadLibraryW_pfn   LoadLibraryW_Original   = nullptr;

LoadLibraryExA_pfn LoadLibraryExA_Original = nullptr;
LoadLibraryExW_pfn LoadLibraryExW_Original = nullptr;

LoadPackagedLibrary_pfn
              LoadPackagedLibrary_Original = nullptr;


void
__stdcall
SK_LockDllLoader (void)
{
  if (config.system.strict_compliance)
  {
    //bool unlocked = TryEnterCriticalSection (&loader_lock);
                       SK_DLL_LoaderLockGuard ()->lock ();
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
    SK_DLL_LoaderLockGuard ()->unlock ();
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

template <typename _T>
BOOL
__stdcall
BlacklistLibrary (const _T* lpFileName);

template <typename _T>
BOOL
__stdcall
SK_LoadLibrary_PinModule (const _T* pStr)
{
#pragma push_macro ("GetModuleHandleEx")

#undef   GetModuleHandleEx
  static GetModuleHandleEx_pfn  GetModuleHandleEx =
    (GetModuleHandleEx_pfn)
                LPCVOID ( typeid (     _T) ==
                          typeid (wchar_t) ?
       (GetModuleHandleEx_pfn) &GetModuleHandleExW :
       (GetModuleHandleEx_pfn) &GetModuleHandleExA );

  HMODULE hModDontCare = 0x0;

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

#undef   StrStrI
  static StrStrI_pfn            StrStrI =
    (StrStrI_pfn)
                LPCVOID ( typeid (     _T) ==
                          typeid (wchar_t) ?
                 (StrStrI_pfn) &StrStrIW   :
                 (StrStrI_pfn) &StrStrIA );

  static const std::vector <const _T*> pinnable_libs =
  {
    // Fix for premature DLL unload issue discussed here:
    //
    //   https://blogs.msdn.microsoft.com/chuckw/2015/10/09/known-issues-xaudio-2-7/
    ///
    SK_TEXT ("XAudio2_7"),
    SK_TEXT ("XAudio2_8"),
    SK_TEXT ("XAudio2_9"),

    SK_TEXT ("nvapi"), SK_TEXT ("NvCameraAllowlisting"),
    SK_TEXT ("nvofapi64"),

    SK_TEXT ("kbd"), // Keyboard Layouts take > ~20 ms to load, leave 'em loaded

    //// Some software repeatedly loads and unloads this, which can
    ////   cause TLS-related problems if left unchecked... just leave
    ////     the damn thing loaded permanently!
    SK_TEXT ("d3dcompiler_"),
  };

  for ( const auto it : pinnable_libs )
  {
    if (StrStrI (pStr, it))
      return true;
  }

  bool pinnable = false;

  // If Platform Integration is Opt'd-Out, turn Library Pinning -Off-
  //
  //   ( Generally for Debugging, Users Shouldn't Rely on This... )
  //
  if ((! pinnable) && config.platform.silent != true)
  {      pinnable =
      StrStrI (
        pStr, SK_RunLHIfBitness (64,
            SK_TEXT ("steamclient64.dll"),
            SK_TEXT ("steamclient"".dll"))
      );
  }

  return pinnable;

#pragma pop_macro ("StrStrI")
}


extern std::wstring SK_XInput_LinkedVersion;

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

  HMODULE
  SK_Debug_LoadHelper (void);

  static HMODULE hModDbgHelp =
    SK_Debug_LoadHelper ();

  if ( hModDbgHelp &&
      ( dbghelp_callers.cend (           ) ==
        dbghelp_callers.find (hCallingMod) ) )
  {
#ifdef _M_AMD64
#define SK_StackWalk          StackWalk64
#define SK_SymLoadModule      SymLoadModule64
#define SK_SymUnloadModule    SymUnloadModule64
#define SK_SymGetModuleBase   SymGetModuleBase64
#define SK_SymGetLineFromAddr SymGetLineFromAddr64
#else /* _M_IX86 */
#define SK_StackWalk          StackWalk
#define SK_SymLoadModule      SymLoadModule
#define SK_SymUnloadModule    SymUnloadModule
#define SK_SymGetModuleBase   SymGetModuleBase
#define SK_SymGetLineFromAddr SymGetLineFromAddr
#endif

    MODULEINFO mod_info = { };

    GetModuleInformation (
      GetCurrentProcess (), hCallingMod, &mod_info, sizeof (mod_info)
    );

    std::string szDupName (
        SK_WideCharToUTF8 (
          SK_GetModuleFullName (hCallingMod)
        ) + '\0'          );

    char* pszShortName =
             szDupName.data ();

    PathStripPathA (pszShortName);

    if (cs_dbghelp != nullptr)
    {
      std::scoped_lock <SK_Thread_HybridSpinlock> auto_lock (*cs_dbghelp);

      if ( dbghelp_callers.cend (           ) ==
           dbghelp_callers.find (hCallingMod) ) 
      {
        SK_SymLoadModule ( GetCurrentProcess (),
                             nullptr,
                              pszShortName,
                                nullptr,
#ifdef _M_AMD64
                                  (DWORD64)mod_info.lpBaseOfDll,
#else /* _M_IX86 */
                                    (DWORD)mod_info.lpBaseOfDll,
#endif
                                      mod_info.SizeOfImage );

        dbghelp_callers.insert (hCallingMod);
      }
    }
  }

  static StrStrI_pfn            StrStrI =
        (StrStrI_pfn)
                LPCVOID ( typeid (     _T) ==
                          typeid (wchar_t) ?
        (StrStrI_pfn)          &StrStrIW            :
        (StrStrI_pfn)          &StrStrIA            );

  static PathRemoveFileSpec_pfn PathRemoveFileSpec =
        (PathRemoveFileSpec_pfn)
                LPCVOID ( typeid (     _T) ==
                          typeid (wchar_t) ?
       (PathRemoveFileSpec_pfn)&PathRemoveFileSpecW :
       (PathRemoveFileSpec_pfn)&PathRemoveFileSpecA );

  static LoadLibrary_pfn        LoadLibrary =
        (LoadLibrary_pfn)
                LPCVOID ( typeid (     _T) ==
                          typeid (wchar_t) ?
        (LoadLibrary_pfn)      &SK_LoadLibraryW     :
        (LoadLibrary_pfn)      &SK_LoadLibraryA     );

  static strncpy_pfn            strncpy_ =
        (strncpy_pfn)
                LPCVOID ( typeid (     _T) ==
                          typeid (wchar_t) ?
        (strncpy_pfn)          &wcsncpy             :
        (strncpy_pfn)          &strncpy             );

  static lstrcat_pfn            lstrcat =
        (lstrcat_pfn)
                LPCVOID ( typeid (     _T) ==
                          typeid (wchar_t) ?
        (lstrcat_pfn)          &lstrcatW            :
        (lstrcat_pfn)          &lstrcatA            );

  static GetModuleHandleEx_pfn   GetModuleHandleEx =
        (GetModuleHandleEx_pfn)
                LPCVOID ( typeid (     _T) ==
                          typeid (wchar_t) ?
        (GetModuleHandleEx_pfn) &GetModuleHandleExW  :
        (GetModuleHandleEx_pfn) &GetModuleHandleExA  );

  // It's impossible to log this if we're loading the DLL necessary to log this...
  if (StrStrI (lpFileName, SK_TEXT("dbghelp")))
  {
    return;
  }

  wchar_t     wszModName [MAX_PATH + 2] = { };
  wcsncpy_s ( wszModName, MAX_PATH,
             SK_GetModuleName (hCallingMod).c_str (),
                          _TRUNCATE) ;

  if ((! SK_LoadLibrary_SILENCE) && SK_Debug_LoadHelper ())
  {
    char  szSymbol [1024] = { };
    ULONG ulLen  =  1023;

    ulLen =
      SK_GetSymbolNameFromModuleAddr (
        SK_GetCallingDLL      (lpCallerFunc),
 reinterpret_cast <uintptr_t> (lpCallerFunc),
                            szSymbol,
                              ulLen );

    if (typeid (_T) == typeid (char))
    {
      char szFileName [MAX_PATH + 2] = { };

      strncpy_s ( szFileName, MAX_PATH,
                    reinterpret_cast <const char *> (lpFileName),
                              _TRUNCATE );

      SK_ConcealUserDirA (szFileName);

      dll_log->Log ( L"[DLL Loader]   ( %-28ws ) loaded '%#116hs' <%14hs> "
                                    L"{ '%21hs' }",
                       wszModName,
                         szFileName,
        reinterpret_cast <const char*> (lpFunction),
                             szSymbol );
    }

    else
    {
      wchar_t     wszFileName [MAX_PATH + 2] = { };
      wcsncpy_s ( wszFileName, MAX_PATH,
                  reinterpret_cast <const wchar_t *> (lpFileName),
                              _TRUNCATE );

      SK_ConcealUserDir (wszFileName);

      dll_log->Log ( L"[DLL Loader]   ( %-28ws ) loaded '%#116ws' <%14ws> "
                                    L"{ '%21hs' }",
                       wszModName,
                         wszFileName,
                           lpFunction,
                             szSymbol );
    }
  }

  if (config.compatibility.rehook_loadlibrary)
  {
    if (hCallingMod != SK_GetDLL ())
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

  if (hCallingMod != SK_GetDLL ())
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
      SK_RunOnce (SK_BootD3D9   ());
#ifdef _M_IX86
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
      SK_RunOnce (SK_BootDXGI   ());
    else if ( (! (SK_GetDLLRole () & DLL_ROLE::DXGI)) && config.apis.dxgi.d3d11.hook &&
              ( StrStrI  (lpFileName, SK_TEXT("dxcore.dll")) || // Unity?! WTF are you doing?
                StrStrIW (wszModName,        L"dxcore.dll") ))
      SK_RunOnce (SK_BootDXGI   ());
    else if ( (! (SK_GetDLLRole () & DLL_ROLE::DXGI)) && config.apis.dxgi.d3d12.hook &&
              ( StrStrI  (lpFileName, SK_TEXT("d3d12.dll")) ||
                StrStrIW (wszModName,        L"d3d12.dll") ))
      SK_RunOnce (SK_BootDXGI   ());
#ifdef _M_AMD64
    else if (   StrStrI  (lpFileName, SK_TEXT("vulkan-1.dll")) ||
                StrStrIW (wszModName,        L"vulkan-1.dll")  )
      SK_RunOnce (SK_BootVulkan ());
#endif
    else if (  (! (SK_GetDLLRole () & DLL_ROLE::OpenGL)) && config.apis.OpenGL.hook &&
              ( StrStrI  (lpFileName, SK_TEXT("OpenGL32.dll")) ||
                StrStrIW (wszModName,        L"OpenGL32.dll") ))
    {   if (!SK_IsModuleLoaded (L"EOSOVH-Win64-Shipping.dll"))
          SK_RunOnce (SK_BootOpenGL ());
    }
    else if (   StrStrI  (lpFileName, SK_TEXT("GameInput.dll")) ||
                StrStrIW (wszModName,        L"GameInput.dll")  )
      SK_RunOnce (SK_Input_HookGameInput ());
    else if (   //SK_XInput_LinkedVersion.empty () &&
                StrStrI (lpFileName, SK_TEXT("xinput1_3.dll")) )
                     SK_RunOnce (SK_Input_HookXInput1_3 ());
    else if (   //SK_XInput_LinkedVersion.empty () &&
                StrStrI (lpFileName, SK_TEXT("xinput1_4.dll")) )
                     SK_RunOnce (SK_Input_HookXInput1_4 ());
    else if (   //SK_XInput_LinkedVersion.empty () &&
                StrStrI (lpFileName, SK_TEXT("xinput9_1_0.dll")) )
                     SK_RunOnce (SK_Input_HookXInput9_1_0 ());
    else if (   StrStrI (lpFileName, SK_TEXT("dinput8.dll")) )
      SK_RunOnce (SK_Input_HookDI8 ());
    else if (   StrStrI (lpFileName, SK_TEXT("dinput.dll")) )
      SK_RunOnce (SK_Input_HookDI7 ());
    else if (   StrStrI (lpFileName, SK_TEXT("hid.dll")) )
      SK_RunOnce (SK_Input_HookHID ());
    else if (   StrStrI ( lpFileName, SK_TEXT("EOSSDK-Win")) ||
                StrStrIW (wszModName,        L"EOSSDK-Win") )
      SK_RunOnce (SK::EOS::Init (false));
    else if (   StrStrI ( lpFileName, SK_TEXT("libScePad")) ||
                StrStrIW (wszModName,        L"libScePad") )
      SK_RunOnce (SK_Input_HookScePad ());
    else if (   StrStrI ( lpFileName, SK_TEXT("dstorage.dll")) ||
                StrStrIW (wszModName,        L"dstorage.dll") )
    {
      extern void SK_DStorage_Init (void);
      SK_RunOnce (SK_DStorage_Init ());
    }
    else if (   StrStrI ( lpFileName, SK_TEXT("sl.interposer.dll")) ||
                StrStrIW (wszModName,        L"sl.interposer.dll") )
    {
      SK_COMPAT_CheckStreamlineSupport ();
    }
    else if (   StrStrI ( lpFileName, SK_TEXT("sl.dlss_g.dll")) ||
                StrStrIW (wszModName,        L"sl.dlss_g.dll") )
    {
      SK_COMPAT_CheckStreamlineSupport ();
    }
    else if (   StrStrI ( lpFileName, SK_TEXT("msmpeg2vdec.dll")) ||
                StrStrIW (wszModName,        L"msmpeg2vdec.dll"))
    {
      extern HMODULE SK_KnownModule_MSMPEG2VDEC;
                     SK_KnownModule_MSMPEG2VDEC = SK_GetModuleHandle (L"msmpeg2vdec.dll");
    }
    else if (   StrStrI ( lpFileName, SK_TEXT("mfplat.dll")) ||
                StrStrIW (wszModName,        L"mfplat.dll"))
    {
      extern HMODULE SK_KnownModule_MFPLAT;
                     SK_KnownModule_MFPLAT = SK_GetModuleHandleW (L"mfplat.dll");
    }
    else if (   StrStrI ( lpFileName, SK_TEXT("_nvngx.dll")) ||
                StrStrIW (wszModName,        L"_nvngx.dll") )
    {
      extern void SK_NGX_Init (void);
                  SK_NGX_Init ();
    }
    else if (   StrStrI ( lpFileName, SK_TEXT("nvngx_dlssg.dll")) ||
                StrStrIW (wszModName,        L"nvngx_dlssg.dll") )
    {
      extern void SK_NGX_EstablishDLSSGVersion (void) noexcept;
                  SK_NGX_EstablishDLSSGVersion ();
    }

#if 0
    if (! config.platform.silent) {
      if ( StrStrIA (lpFileName, szSteamAPIDLL)    ||
           StrStrIA (lpFileName, szSteamNativeDLL) ||
           StrStrIA (lpFileName, szSteamClientDLL) ) {
        SK_RunOnce (SK_HookSteamAPI ())
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
SK_FreeLibrary (HMODULE hLibModule)
{
  BOOL bRet = FALSE;

  if (FreeLibrary_Original == nullptr)
    bRet = FreeLibrary (hLibModule);

  else
    bRet = FreeLibrary_Original (hLibModule);

  return bRet;
}

HMODULE
WINAPI
SK_LoadLibraryA (LPCSTR lpModName)
{
  if (LoadLibraryA_Original != nullptr)
    return LoadLibraryA_Original (lpModName);

  return LoadLibraryA (lpModName);
}

HMODULE
WINAPI
SK_LoadLibraryW (LPCWSTR lpModName)
{
  if (LoadLibraryW_Original != nullptr)
    return LoadLibraryW_Original (lpModName);

  return LoadLibraryW (lpModName);
}

HMODULE
WINAPI
SK_LoadLibraryExA (
  _In_       LPCSTR  lpFileName,
  _Reserved_ HANDLE  hFile,
  _In_       DWORD   dwFlags
)
{
  if (LoadLibraryExA_Original != nullptr)
    return LoadLibraryExA_Original (lpFileName, hFile, dwFlags);

  return LoadLibraryExA (lpFileName, hFile, dwFlags);
}

HMODULE
WINAPI
SK_LoadLibraryExW (
  _In_       LPCWSTR lpFileName,
  _Reserved_ HANDLE  hFile,
  _In_       DWORD   dwFlags
)
{
  if (LoadLibraryExW_Original != nullptr)
    return LoadLibraryExW_Original (lpFileName, hFile, dwFlags);

  return LoadLibraryExW (lpFileName, hFile, dwFlags);
}


BOOL
WINAPI
FreeLibrary_Detour (HMODULE hLibModule)
{
  if (ReadAcquire (&__SK_DLL_Ending))
    return TRUE;

#ifdef _M_AMD64
  if ( SK_GetCallingDLL () == SK_GetModuleHandle (L"gameoverlayrenderer64.dll") )
#else /* _M_IX86 */
  if ( SK_GetCallingDLL () == SK_GetModuleHandle (L"gameoverlayrenderer.dll") )
#endif
  {
    // Steam unsafely loads DLLs that are holding onto
    //   Critical Sections, so the safest thing to do
    //     is never let it do that!
    return TRUE;
  }


  LPVOID pAddr = _ReturnAddress ();

  if (ReadAcquire (&__SK_DLL_Ending) != FALSE)
  {
    return
      SK_FreeLibrary (hLibModule);
  }

  if (hLibModule == SK_GetDLL ())
    return TRUE;

  SK_LockDllLoader ();

        std::wstring name =
    SK_GetModuleName (hLibModule);

  const BOOL         bRet =
    FreeLibrary_Original (hLibModule);

  if ( (! (SK_LoadLibrary_SILENCE)) ||
           SK_GetModuleName (hLibModule).find (L"steam") != std::wstring::npos )
  {
    if (   SK_GetModuleName (hLibModule).find (L"steam") != std::wstring::npos ||
        (bRet && SK_GetModuleHandle (name.c_str ()) == nullptr ) )
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

        dll_log->Log ( L"[DLL Loader]   ( %-28ls ) freed  '%#64ls' from { '%hs' }",
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

void
SKX_SummarizeCaller ( LPVOID  lpRet,
                     wchar_t *wszSummary )
{
  wcsncpy_s ( wszSummary,                          128,
              SK_SummarizeCaller (lpRet).c_str (), _TRUNCATE );
}


bool
SK_GetModuleHandleExW ( DWORD    dwFlags,
                        LPCWSTR  lpModuleName,
                        HMODULE* phModule )
{
  __try
  {
    GetModuleHandleExW (
      dwFlags, lpModuleName,
               phModule );
  }

  __except ( GetExceptionCode () == EXCEPTION_INVALID_HANDLE  ?
                                    EXCEPTION_EXECUTE_HANDLER :
                                    EXCEPTION_CONTINUE_SEARCH )
  {
    return false;
  }

  return true;
}

HMODULE
WINAPI
LoadLibrary_Marshal ( LPVOID   lpRet,
                      LPCWSTR  lpFileName,
                const wchar_t *wszSourceFunc )
{
  if (lpFileName == nullptr)
    return nullptr;

  if (config.input.gamepad.xinput.emulate && StrStrIW (lpFileName, L"GameInput.dll"))
  {
    return SK_GetDLL ();
  }

  SK_LockDllLoader ();

  HMODULE hModEarly = nullptr;

  std::unique_ptr <wchar_t []> cleanup;

  wchar_t*          compliant_path =
          const_cast <wchar_t *> (lpFileName);

  if (     StrStrW (compliant_path, L"/"))
  {
    SK_TLS* pTLS =
      SK_TLS_Bottom ();

                    compliant_path =
      reinterpret_cast <wchar_t *> (
                      pTLS->scratch_memory->cmd.alloc (
                             (wcslen (lpFileName) + 1) *
                              sizeof (wchar_t), true )
                                   );

    if (            compliant_path != nullptr)   {
          lstrcatW (compliant_path, lpFileName);
    SK_FixSlashesW (compliant_path);             } else
                    compliant_path =
                         (wchar_t *)lpFileName;
  }

  if (*compliant_path != L'\0')
  {
    if (! SK_GetModuleHandleExW (
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
              compliant_path,
                &hModEarly      )
       )
    {
      SK_SetLastError (0);
    }

    if (hModEarly == nullptr && BlacklistLibrary (compliant_path))
    {
      SK_UnlockDllLoader ();
      SK_SetLastError (ERROR_MOD_NOT_FOUND);
      return nullptr;
    }

    HMODULE hMod = hModEarly;
    
#ifdef _DEBUG
    orig_se =
    SK_SEH_ApplyTranslator (
      SK_FilteringStructuredExceptionTranslator (
        EXCEPTION_ACCESS_VIOLATION
      )
    );
    try
    {
#endif
      //////////////////////////////////////////////////////////////
      //
      // CRAPCOM loads SteamAPI twice as part of its craptastic DRM.
      //
      //  The first DLL is fake and done using LoadLibraryW
      //  The second load is real, and done using LoadLibraryExA
      //
      //////////////////////////////////////////////////////////
      if (config.steam.crapcom_mode)
      {
        // Here we hook and load the -fake- SteamAPI DLL.
        // 
        //   CRAPCOM checks this DLL for memory consistency, they think that
        //   it is the same DLL they loaded w/ LoadLibraryExA, but NOPE!
        //
        if (StrStrIW (compliant_path, L"steam_api64.dll") == compliant_path)
        {
          static wchar_t   steam_dll [] = L"./steam_api64.dll";
          compliant_path = steam_dll;
        }
      }

      // Avoid loader deadlock in Steam overlay by pre-loading this
      //   DLL behind the overlay's back.
      if (StrStrIW (compliant_path, L"rxcore"))
        SK_LoadLibraryW (L"xinput1_4.dll");

      bool bVulkanLayerDisabled = false;

      if (config.apis.NvAPI.vulkan_bridge == 1 && SK_GetCallingDLL (lpRet) == SK_GetModuleHandle (L"vulkan-1.dll"))
      {
        if (StrStrIW (compliant_path, L"graphics-hook"))
        {
          SK_RunOnce (
            dll_log->Log (L"[DLL Loader]  ** Disabling OBS's Vulkan Layer because VulkanBridge is active.")
          );

          bVulkanLayerDisabled = true;
        }

        //else if (StrStrIW (compliant_path, L"SteamOverlayVulkanLayer"))
        //{
        //  SK_RunOnce (
        //    dll_log->Log (L"[DLL Loader]  ** Disabling Steam's Vulkan Layer because VulkanBridge is active.")
        //  );
        //
        //  bVulkanLayerDisabled = true;
        //}

        //else if (StrStrIW (compliant_path, L"RTSSVkLayer"))
        //{
        //  SK_RunOnce (
        //    dll_log->Log (L"[DLL Loader]  ** Disabling RTSS's Vulkan Layer because VulkanBridge is active.")
        //  );
        //
        //  bVulkanLayerDisabled = true;
        //}
      }


      // Disable EOS Overlay in local injection
      if (StrStrIW (compliant_path, L"EOSOVH-Win32-Shipping"))
      {
        SK_LOGs0 (L"DLL Loader", L"Epic Overlay Disabled in 32-bit game to prevent deadlock");
        SK_SetLastError (ERROR_MOD_NOT_FOUND);
        hMod = nullptr;
      }

      // Avoid issues on AMD drivers caused by GOG's overlay
      else if (StrStrIW (compliant_path, L"overlay_mediator_"))
      {
        extern int
            SK_ADL_CountActiveGPUs (void);
        if (SK_ADL_CountActiveGPUs () > 0)
        {
          if (SK_GetModuleHandleW (L"OpenGL32.dll"))
          {
            SK_LOGs0 (L"DLL Loader", L"Disabling GOG Overlay on AMD systems.");
            SK_SetLastError (ERROR_MOD_NOT_FOUND);
            hMod = nullptr;
          }
        }
      }

      // Windows Defender likes to deadlock in the Steam Overlay
      else if (StrStrIW (compliant_path, L"Windows Defender"))
      {
        SK_SetLastError (ERROR_MOD_NOT_FOUND);
        hMod = nullptr;
      }

      else if (bVulkanLayerDisabled)
      {
        SK_SetLastError (ERROR_MOD_NOT_FOUND);
      
        hMod = nullptr;
      }

      else if (config.nvidia.bugs.bypass_ansel && StrStrIW (compliant_path, L"NvCamera"))
      {
        SK_RunOnce (
          dll_log->Log (L"[DLL Loader]  ** Disabling NvCamera because it's unstable.")
        );

        SK_SetLastError (ERROR_MOD_NOT_FOUND);

        hMod = nullptr;
      }

      else if (config.nvidia.bugs.bypass_ansel && StrStrIW (compliant_path, L"NvTelemetry"))
      {
        SK_RunOnce (
          dll_log->Log (L"[DLL Loader]  ** Disabling NvTelemetry because it's unstable.")
        );

        SK_SetLastError (ERROR_MOD_NOT_FOUND);

        hMod = nullptr;
      }

      else if ((! config.compatibility.allow_dxdiagn) && StrStrIW (compliant_path, L"dxdiagn.dll"))
      {
        SK_RunOnce (
          dll_log->Log (L"[DLL Loader]  ** Disabling DxDiagn because it is slow as hell (!!)")
        );

        SK_SetLastError (ERROR_MOD_NOT_FOUND);
        hMod = nullptr;
      }

      else if (config.nvidia.dlss.auto_redirect_dlss && StrStrIW (compliant_path, L"nvngx_dlss.dll"))
      {
        static auto path_to_plugin_dlss =
          std::filesystem::path (
            SK_GetPlugInDirectory (SK_PlugIn_Type::ThirdParty)
          ) / L"NVIDIA" / L"nvngx_dlss.dll";

        std::error_code                                   ec;
        if (std::filesystem::exists (path_to_plugin_dlss, ec))
        {
          if (*SK_GetDLLVersionShort (compliant_path).c_str () > L'1')
          {
            dll_log->Log (
              L"[DLL Loader]  ** Redirecting NVIDIA DLSS DLL (%ws) to (%ws)",
                compliant_path,
                          path_to_plugin_dlss.c_str ()
            );

            hMod =
              SK_LoadLibraryW (path_to_plugin_dlss.c_str ());
          }

          else
          {
            dll_log->Log (
              L"[DLL Loader]  ** Game's DLSS version (%ws) is too old to replace.",
                SK_GetDLLVersionShort (compliant_path).c_str ()
            );

            hMod = nullptr;
          }

          if (! hMod)
          {
            dll_log->Log (L"[DLL Loader]  # Plug-In Load Failed, using original DLL!");

            hMod =
              SK_LoadLibraryW (compliant_path);
          }

          extern void SK_NGX_EstablishDLSSVersion (void);
                      SK_NGX_EstablishDLSSVersion ();
        }
      }

      else
        hMod =
          SK_LoadLibraryW (compliant_path);
#ifdef _DEBUG
    }
    catch (const SK_SEH_IgnoredException&)
    {
      wchar_t                     caller [128] = { };
      SKX_SummarizeCaller (lpRet, caller);

      dll_log->Log ( L"[DLL Loader]  ** Crash Prevented **  "
                      L"DLL raised an exception during"
                       L" %s ('%ws')!  -  %s",
                        wszSourceFunc, compliant_path,
                        caller );
    }
    SK_SEH_RemoveTranslator (orig_se);
#endif


    if (hModEarly != hMod)
    {
      SK_TraceLoadLibrary ( SK_GetCallingDLL (lpRet),
                              compliant_path,
                                wszSourceFunc, lpRet );
    }

    SK_UnlockDllLoader ();

    return hMod;
  }

  SK_SetLastError (ERROR_MOD_NOT_FOUND);

  return nullptr;
}

HMODULE
WINAPI
LoadLibraryA_Detour (LPCSTR lpFileName)
{
  if (ReadAcquire (&__SK_DLL_Ending))
    return nullptr;

  std::wstring wFileName;

  // Stupidity for idTechLauncher
  if (lpFileName != nullptr)
    wFileName = SK_UTF8ToWideChar (lpFileName) + std::wstring (L"\0");

  return
    LoadLibrary_Marshal (
       _ReturnAddress (),
         lpFileName != nullptr ?
          wFileName.data ()    : nullptr,
           L"LoadLibraryA"
    );
}

HMODULE
WINAPI
LoadLibraryW_Detour (LPCWSTR lpFileName)
{
  if (ReadAcquire (&__SK_DLL_Ending))
    return nullptr;

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
  if (ReadAcquire (&__SK_DLL_Ending))
    return nullptr;

#if 1
  return
    LoadPackagedLibrary_Original (lpLibFileName, Reserved);
#else
  LPVOID lpRet = _ReturnAddress ();

  if (lpLibFileName == nullptr)
    return nullptr;

  SK_LockDllLoader ();

  HMODULE hModEarly = nullptr;

  wchar_t*          compliant_path =
          const_cast <wchar_t *> (lpLibFileName);

  if (     StrStrW (compliant_path, L"/"))
  {
    SK_TLS* pTLS =
      SK_TLS_Bottom ();

                    compliant_path =
      reinterpret_cast <wchar_t *> (
                      pTLS->scratch_memory->cmd.alloc (
                             (wcslen (lpLibFileName) + 1) *
                              sizeof (wchar_t), true )
                                   );

    if (            compliant_path != nullptr)   {
          lstrcatW (compliant_path, lpLibFileName);
    SK_FixSlashesW (compliant_path);             } else
                    compliant_path =
                         (wchar_t *)lpLibFileName;
  }

  lpLibFileName = compliant_path;

  auto orig_se =
  SK_SEH_ApplyTranslator (
    SK_FilteringStructuredExceptionTranslator (
      EXCEPTION_INVALID_HANDLE
    )
  );
  try
  {
    GetModuleHandleExW (
      GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        lpLibFileName,
          &hModEarly
    );
  }
  catch (const SK_SEH_IgnoredException&)
  {
    SK_SetLastError (0);
  }
  SK_SEH_RemoveTranslator (orig_se);


  if (hModEarly == nullptr && BlacklistLibrary (lpLibFileName))
  {
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

  SK_UnlockDllLoader ();
  return hMod;
#endif
}

HMODULE
WINAPI
LoadLibraryEx_Marshal ( LPVOID   lpRet, LPCWSTR lpFileName,
                        HANDLE   hFile, DWORD   dwFlags,
                  const wchar_t *wszSourceFunc)
{
  //dwFlags &= ~LOAD_WITH_ALTERED_SEARCH_PATH;

  if (lpFileName == nullptr)
    return nullptr;

  if (config.input.gamepad.xinput.emulate && StrStrIW (lpFileName, L"GameInput.dll"))
  {
    return SK_GetDLL ();
  }

  wchar_t*          compliant_path =
          const_cast <wchar_t *> (lpFileName);

  if (     StrStrW (compliant_path, L"/"))
  {
    SK_TLS* pTLS =
      SK_TLS_Bottom ();

                    compliant_path =
      reinterpret_cast <wchar_t *> (
                      pTLS->scratch_memory->cmd.alloc (
                             (wcslen (lpFileName) + 1) *
                              sizeof (wchar_t), true )
                                   );

    if (            compliant_path != nullptr)   {
          lstrcatW (compliant_path, lpFileName);
    SK_FixSlashesW (compliant_path);             } else
                    compliant_path =
                         (wchar_t *)lpFileName;
  }

  //////////////////////////////////////////////////////////////
  //
  // CRAPCOM loads SteamAPI twice as part of its craptastic DRM.
  //
  //  The first DLL is fake and done using LoadLibraryW
  //  The second load is real, and done using LoadLibraryExA
  //
  //////////////////////////////////////////////////////////
  if (config.steam.crapcom_mode)
  {
    // Here we hook and load the -real- SteamAPI DLL, pirates can put their
    //   DLC stealing DLL here (kaldaien_api/steam_api64.dll) if they want.
    if (StrStrIW (compliant_path, L"steam_api64.dll") == compliant_path)
    {
      compliant_path = const_cast <wchar_t *> (config.steam.dll_path.c_str ());
    }
  }

  // Give Microsoft Store games a copy of XInput1_4 instead of XInputUap,
  //   since it's impossible to install hooks on XInputUap...
  if (StrStrIW (compliant_path, L"XInputUap.dll"))
  {
    static wchar_t   xinput1_4_dll [] = L"XInput1_4.dll";
    compliant_path = xinput1_4_dll;
  }
  
  bool bVulkanLayerDisabled = false;

  if (config.apis.NvAPI.vulkan_bridge == 1 && SK_GetCallingDLL (lpRet) == SK_GetModuleHandle (L"vulkan-1.dll"))
  {
    if (StrStrIW (compliant_path, L"graphics-hook"))
    {
      SK_RunOnce (
        dll_log->Log (L"[DLL Loader]  ** Disabling OBS's Vulkan Layer because VulkanBridge is active.")
      );

      bVulkanLayerDisabled = true;
    }

    //else if (StrStrIW (compliant_path, L"SteamOverlayVulkanLayer"))
    //{
    //  SK_RunOnce (
    //    dll_log->Log (L"[DLL Loader]  ** Disabling Steam's Vulkan Layer because VulkanBridge is active.")
    //  );
    //
    //  bVulkanLayerDisabled = true;
    //}
    //
    //else if (StrStrIW (compliant_path, L"RTSSVkLayer"))
    //{
    //  SK_RunOnce (
    //    dll_log->Log (L"[DLL Loader]  ** Disabling RTSS's Vulkan Layer because VulkanBridge is active.")
    //  );
    //
    //  bVulkanLayerDisabled = true;
    //}
  }

  if (bVulkanLayerDisabled)
  {
    SK_SetLastError (ERROR_MOD_NOT_FOUND);

    return nullptr;
  }

  else if ((! config.compatibility.allow_dxdiagn) && StrStrIW (compliant_path, L"dxdiagn.dll"))
  {
    SK_RunOnce (
      dll_log->Log (L"[DLL Loader]  ** Disabling DxDiagn because it is slow as hell (!!)")
    );

    SK_SetLastError (ERROR_MOD_NOT_FOUND);

    return nullptr;
  }

  else if (config.nvidia.bugs.bypass_ansel && StrStrIW (compliant_path, L"NvCamera"))
  {
    SK_RunOnce (
      dll_log->Log (L"[DLL Loader]  ** Disabling NvCamera because it's unstable.")
    );

    SK_SetLastError (ERROR_MOD_NOT_FOUND);

    return nullptr;
  }

  else if (config.nvidia.bugs.bypass_ansel && StrStrIW (compliant_path, L"NvTelemetry"))
  {
    SK_RunOnce (
      dll_log->Log (L"[DLL Loader]  ** Disabling NvTelemetry because it's unstable.")
    );

    SK_SetLastError (ERROR_MOD_NOT_FOUND);

    return nullptr;
  }

  SK_LockDllLoader ();

  if ((dwFlags & LOAD_LIBRARY_AS_DATAFILE) && (! BlacklistLibrary (compliant_path)))
  {
    HMODULE hModRet =
      SK_LoadLibraryExW (compliant_path, hFile, dwFlags);

    SK_UnlockDllLoader ();
    return hModRet;
  }

  HMODULE hModEarly = nullptr;

  if (! SK_GetModuleHandleExW (
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
          compliant_path,
            &hModEarly      )
   ) SK_SetLastError (0);

  if (hModEarly == nullptr && BlacklistLibrary (compliant_path))
  {
    SK_UnlockDllLoader ();
    return nullptr;
  }


  HMODULE hMod = hModEarly;

#ifdef _DEBUG
  orig_se =
  SK_SEH_ApplyTranslator (
    SK_FilteringStructuredExceptionTranslator (
      EXCEPTION_ACCESS_VIOLATION
    )
  );
  try
  {
#endif
    if (config.nvidia.dlss.auto_redirect_dlss && StrStrIW (compliant_path, L"nvngx_dlss.dll"))
    {
      static auto path_to_plugin_dlss =
        std::filesystem::path (
          SK_GetPlugInDirectory (SK_PlugIn_Type::ThirdParty)
        ) / L"NVIDIA" / L"nvngx_dlss.dll";

      std::error_code                                   ec;
      if (std::filesystem::exists (path_to_plugin_dlss, ec))
      {
        if (*SK_GetDLLVersionShort (compliant_path).c_str () > L'1')
        {
          dll_log->Log (
            L"[DLL Loader]  ** Redirecting NVIDIA DLSS DLL (%ws) to (%ws)",
              compliant_path,
                        path_to_plugin_dlss.c_str ()
          );

          hMod =
            SK_LoadLibraryW (path_to_plugin_dlss.c_str ());
        }

        else
        {
          dll_log->Log (
            L"[DLL Loader]  ** Game's DLSS version (%ws) is too old to replace.",
              SK_GetDLLVersionShort (compliant_path).c_str ()
          );

          hMod = nullptr;
        }

        if (! hMod)
        {
          dll_log->Log (L"[DLL Loader]  # Plug-In Load Failed, using original DLL!");

          hMod =
            SK_LoadLibraryExW (compliant_path, hFile, dwFlags);
        }
      }
      
      extern void SK_NGX_EstablishDLSSVersion (void);
                  SK_NGX_EstablishDLSSVersion ();
    }

    else
    {
      hMod =
        SK_LoadLibraryExW (compliant_path, hFile, dwFlags);
    }
#ifdef _DEBUG
  }
  catch (const SK_SEH_IgnoredException&)
  {
    dll_log->Log ( L"[DLL Loader]  ** Crash Prevented **  DLL raised an exception during"
                   L" %s ('%ws')!",
                     wszSourceFunc, compliant_path );
  }
  SK_SEH_RemoveTranslator (orig_se);
#endif

  if ( hModEarly != hMod && (! ((dwFlags & LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE) ||
                                (dwFlags & LOAD_LIBRARY_AS_IMAGE_RESOURCE))) )
  {
    SK_TraceLoadLibrary ( SK_GetCallingDLL (lpRet),
                            compliant_path,
                              wszSourceFunc, lpRet );
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
  if (ReadAcquire (&__SK_DLL_Ending))
    return nullptr;

  std::wstring wszFileName;

  if (lpFileName != nullptr)
    wszFileName = SK_UTF8ToWideChar (lpFileName) + std::wstring (L"\0");

  return
    LoadLibraryEx_Marshal (
      _ReturnAddress (),
        wszFileName.data (),
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
  if (ReadAcquire (&__SK_DLL_Ending))
    return nullptr;

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

  constexpr SK_ThirdPartyDLLs (void) = default;
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

  if (_loader_hooks->unhooked)
    return false;

  SK_LockDllLoader ();

  if (_loader_hooks->LoadLibraryA_target != nullptr)
  {
    SK_RemoveHook (_loader_hooks->LoadLibraryA_target);
    _loader_hooks->LoadLibraryA_target = nullptr;
  }

  SK_CreateDLLHook2 (      L"kernel32",
                            "LoadLibraryA",
                             LoadLibraryA_Detour,
    static_cast_p2p <void> (&LoadLibraryA_Original),
                           &_loader_hooks->LoadLibraryA_target );

  MH_QueueEnableHook (_loader_hooks->LoadLibraryA_target);


  if (_loader_hooks->LoadLibraryW_target != nullptr)
  {
    SK_RemoveHook (_loader_hooks->LoadLibraryW_target);
    _loader_hooks->LoadLibraryW_target = nullptr;
  }

  SK_CreateDLLHook2 (      L"kernel32",
                            "LoadLibraryW",
                             LoadLibraryW_Detour,
    static_cast_p2p <void> (&LoadLibraryW_Original),
                           &_loader_hooks->LoadLibraryW_target );

  MH_QueueEnableHook (_loader_hooks->LoadLibraryW_target);


  if (SK_GetProcAddress (SK_GetModuleHandle (L"kernel32"),
                                              "LoadPackagedLibrary") != nullptr)
  {
    if (_loader_hooks->LoadPackagedLibrary_target != nullptr)
    {
      SK_RemoveHook (_loader_hooks->LoadPackagedLibrary_target);
      _loader_hooks->LoadPackagedLibrary_target = nullptr;
    }

    SK_CreateDLLHook2 (      L"kernel32",
                              "LoadPackagedLibrary",
                               LoadPackagedLibrary_Detour,
      static_cast_p2p <void> (&LoadPackagedLibrary_Original),
                             &_loader_hooks->LoadPackagedLibrary_target );

    MH_QueueEnableHook (_loader_hooks->LoadPackagedLibrary_target);
  }


  if (_loader_hooks->LoadLibraryExA_target != nullptr)
  {
    SK_RemoveHook (_loader_hooks->LoadLibraryExA_target);
    _loader_hooks->LoadLibraryExA_target = nullptr;
  }

  SK_CreateDLLHook2 (      L"kernel32",
                            "LoadLibraryExA",
                             LoadLibraryExA_Detour,
    static_cast_p2p <void> (&LoadLibraryExA_Original),
                           &_loader_hooks->LoadLibraryExA_target );

  MH_QueueEnableHook (_loader_hooks->LoadLibraryExA_target);


  if (_loader_hooks->LoadLibraryExW_target != nullptr)
  {
    SK_RemoveHook (_loader_hooks->LoadLibraryExW_target);
    _loader_hooks->LoadLibraryExW_target = nullptr;
  }

  SK_CreateDLLHook2 (      L"kernel32",
                            "LoadLibraryExW",
                             LoadLibraryExW_Detour,
    static_cast_p2p <void> (&LoadLibraryExW_Original),
                           &_loader_hooks->LoadLibraryExW_target );

  MH_QueueEnableHook (_loader_hooks->LoadLibraryExW_target);


  if (_loader_hooks->FreeLibrary_target != nullptr)
  {
    SK_RemoveHook (_loader_hooks->FreeLibrary_target);
    _loader_hooks->FreeLibrary_target = nullptr;
  }

  static int calls = 0;


  // Steamclient64.dll leaks heap memory when unloaded,
  //   to prevent this from showing up during debug sessions,
  //     don't hook this function :)
  //// SK_CreateDLLHook2 (      L"kernel32",
  ////                           "FreeLibrary",
  ////                            FreeLibrary_Detour,
  ////   static_cast_p2p <void> (&FreeLibrary_Original),
  ////                          &_loader_hooks.FreeLibrary_target );
  ////
  ////MH_QueueEnableHook (_loader_hooks.FreeLibrary_target);

  extern volatile LONG __SK_Init;

//if (ReadAcquire (&__SK_Init) > 0)
  {
    if (ReadAcquire (&__SK_Init) > 0) SK_ApplyQueuedHooks ();
  } SK_UnlockDllLoader  ();

  return (calls > 1);
}

void
__stdcall
SK_UnhookLoadLibrary (void)
{
  SK_LockDllLoader ();

  _loader_hooks->unhooked = true;

  if (_loader_hooks->LoadLibraryA_target != nullptr)
    MH_QueueDisableHook (_loader_hooks->LoadLibraryA_target);
  if (_loader_hooks->LoadLibraryW_target != nullptr)
    MH_QueueDisableHook(_loader_hooks->LoadLibraryW_target);
  if (_loader_hooks->LoadPackagedLibrary_target != nullptr)
    MH_QueueDisableHook(_loader_hooks->LoadPackagedLibrary_target);
  if (_loader_hooks->LoadLibraryExA_target != nullptr)
    MH_QueueDisableHook (_loader_hooks->LoadLibraryExA_target);
  if (_loader_hooks->LoadLibraryExW_target != nullptr)
    MH_QueueDisableHook (_loader_hooks->LoadLibraryExW_target);
  if (_loader_hooks->FreeLibrary_target != nullptr)
    MH_QueueDisableHook (_loader_hooks->FreeLibrary_target);

  SK_ApplyQueuedHooks ();

  if (_loader_hooks->LoadLibraryA_target != nullptr)
    MH_RemoveHook (_loader_hooks->LoadLibraryA_target);
  if (_loader_hooks->LoadLibraryW_target != nullptr)
    MH_RemoveHook (_loader_hooks->LoadLibraryW_target);
  if (_loader_hooks->LoadPackagedLibrary_target != nullptr)
    MH_RemoveHook (_loader_hooks->LoadPackagedLibrary_target);
  if (_loader_hooks->LoadLibraryExA_target != nullptr)
    MH_RemoveHook (_loader_hooks->LoadLibraryExA_target);
  if (_loader_hooks->LoadLibraryExW_target != nullptr)
    MH_RemoveHook (_loader_hooks->LoadLibraryExW_target);
  if (_loader_hooks->FreeLibrary_target != nullptr)
    MH_RemoveHook (_loader_hooks->FreeLibrary_target);

  _loader_hooks->LoadLibraryW_target        = nullptr;
  _loader_hooks->LoadLibraryA_target        = nullptr;
  _loader_hooks->LoadPackagedLibrary_target = nullptr;
  _loader_hooks->LoadLibraryExW_target      = nullptr;
  _loader_hooks->LoadLibraryExA_target      = nullptr;
  _loader_hooks->FreeLibrary_target         = nullptr;

  SK_UnlockDllLoader ();
}



void
__stdcall
SK_InitCompatBlacklist (void)
{
//memset (&_loader_hooks, 0, sizeof (sk_loader_hooks_t));

  //SK_CreateDLLHook2 (      L"kernel32",
  //                          "GetModuleHandleA",
  //                           GetModuleHandleA_Detour,
  //  static_cast_p2p <void> (&GetModuleHandleA_Original),
  //            &_loader_hooks.GetModuleHandleA_target );
  //
  //MH_QueueEnableHook (_loader_hooks.GetModuleHandleA_target);
  //
  //SK_CreateDLLHook2 (      L"kernel32",
  //                          "GetModuleHandleW",
  //                           GetModuleHandleW_Detour,
  //  static_cast_p2p <void> (&GetModuleHandleW_Original),
  //            &_loader_hooks.GetModuleHandleW_target );
  //
  //MH_QueueEnableHook (_loader_hooks.GetModuleHandleW_target);
  //
  //SK_CreateDLLHook2 (      L"kernel32",
  //                          "GetModuleHandleExA",
  //                           GetModuleHandleExA_Detour,
  //  static_cast_p2p <void> (&GetModuleHandleExA_Original),
  //            &_loader_hooks.GetModuleHandleExA_target );
  //
  //MH_QueueEnableHook (_loader_hooks.GetModuleHandleExA_target);
  //
  //SK_CreateDLLHook2 (      L"kernel32",
  //                          "GetModuleHandleExW",
  //                           GetModuleHandleExW_Detour,
  //  static_cast_p2p <void> (&GetModuleHandleExW_Original),
  //            &_loader_hooks.GetModuleHandleExW_target );
  //
  //MH_QueueEnableHook (_loader_hooks.GetModuleHandleExW_target);
  //
  //SK_CreateDLLHook2 (      L"kernel32",
  //                          "GetModuleFileNameA",
  //                           GetModuleFileNameA_Detour,
  //  static_cast_p2p <void> (&GetModuleFileNameA_Original),
  //            &_loader_hooks.GetModuleFileNameA_target );
  //
  //MH_QueueEnableHook (_loader_hooks.GetModuleFileNameA_target);
  //
  //SK_CreateDLLHook2 (      L"kernel32",
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
#ifdef _M_IX86
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


void
_SK_SummarizeModule ( LPVOID   base_addr,  size_t      mod_size,
                      HMODULE  hMod,       uintptr_t   addr,
                      wchar_t* wszModName, iSK_Logger* pLogger )
{
  if (! (pLogger && wszModName))
    return;

#ifdef _DEBUG
    ULONG ulLen =  0;
    char  szSymbol [512] = { };
    //~~~ This is too slow and useless
    ulLen = SK_GetSymbolNameFromModuleAddr (hMod, addr, szSymbol, ulLen);
#else
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

  else if (GetModuleHandle (wszModName))
#endif
  {
    wchar_t     wszModNameCopy [MAX_PATH + 2] = { };
    wcsncpy_s ( wszModNameCopy, MAX_PATH,
                wszModName,     _TRUNCATE );

    pLogger->Log ( L"[ Module ]  ( %ph + %08i )               "
                   L"                                         "
                   L" %s",
                                             base_addr,
                  sk::narrow_cast <int32_t> (mod_size),
                          SK_ConcealUserDir (wszModNameCopy) );
  }

  if (ver_str.length () > 3)
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
  auto user =
    pWorkingSet;

  static volatile LONG    init           = FALSE;
  static CRITICAL_SECTION cs_thread_walk = { };

  if (! InterlockedCompareExchangeAcquire (&init, 1, 0))
  {
    InitializeCriticalSectionEx (
      &cs_thread_walk,
        32,
          RTL_CRITICAL_SECTION_FLAG_DYNAMIC_SPIN    |
           SK_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO );

    InterlockedIncrementRelease           (&init);
  }

  else
    SK_Thread_SpinUntilAtomicMin (&init, 2);


  auto* pWorkingSet_ =
    static_cast <enum_working_set_s *> (
      _aligned_malloc (sizeof (enum_working_set_s), 16)
    );

  if (pWorkingSet_ == nullptr)
    return;

  EnterCriticalSection (&cs_thread_walk);

  memcpy (pWorkingSet_, user, sizeof (enum_working_set_s));

  iSK_Logger* pLogger =
    pWorkingSet_->logger;

  static std::unordered_set <HMODULE> logged_modules;

  wchar_t wszModName [MAX_PATH + 2] = { };

  auto orig_se =
  SK_SEH_ApplyTranslator (
    SK_MultiFilteringStructuredExceptionTranslator (
      ( EXCEPTION_ACCESS_VIOLATION,
        EXCEPTION_INVALID_HANDLE )
    )
  );
  for (int i = 0; i < pWorkingSet_->count; i++ )
  {
    *wszModName = L'\0';

    try
    {
      // Get the full path to the module's file.
      if ( logged_modules.cend (                         ) ==
           logged_modules.find (pWorkingSet_->modules [i]) &&
           GetModuleFileNameW ( pWorkingSet_->modules [i],
                                  wszModName,
                                    MAX_PATH ) )
      {
        MODULEINFO mi = { };

        uintptr_t entry_pt  = 0;
        uintptr_t base_addr = 0;
        uint32_t  mod_size  = 0UL;

        if ( GetModuleInformation ( pWorkingSet_->proc,
                                    pWorkingSet_->modules [i],
                                                          &mi,
                                           sizeof (MODULEINFO) )
           )
        {
          entry_pt  = reinterpret_cast <std::uintptr_t> (mi.EntryPoint);
          base_addr = reinterpret_cast <std::uintptr_t> (mi.lpBaseOfDll);
          mod_size  =                                    mi.SizeOfImage;
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


        if (SK_LoadLibrary_IsPinnable (wszModName))
             SK_LoadLibrary_PinModule (wszModName);

        //if ( pWorkingSet             != nullptr &&
        //     pWorkingSet->modules [i] > (HMODULE)0 )
        //{
          logged_modules.emplace (
            pWorkingSet_->modules [i]//pWorkingSet->modules [i]
          );
        //}
      }
    }

    catch (const SK_SEH_IgnoredException&)
    {
      // Sometimes a DLL will be unloaded in the middle of doing this...
      //   just ignore that.
    }
  };
  SK_SEH_RemoveTranslator (orig_se);

  SK_SafeCloseHandle (pWorkingSet_->proc);

  LeaveCriticalSection (&cs_thread_walk);

  _aligned_free (pWorkingSet_);
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

#ifdef _M_AMD64
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

    else if ( config.apis.dxgi.d3d12.hook && StrStrIW (wszModName, L"d3d12.dll") &&
                        (SK_IsInjected () || (! (SK_GetDLLRole () & DLL_ROLE::DXGI))) )
    {
      SK_BootDXGI ();

      loaded_dxgi = true;
    }

    else if ( config.apis.d3d9.hook && StrStrIW (wszModName, L"d3d9.dll") &&
                  (SK_IsInjected () || (! (SK_GetDLLRole () & DLL_ROLE::D3D9))) )
    {
      SK_BootD3D9 ();

      loaded_d3d9 = true;
    }

#ifdef _M_IX86
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
         dxgi  = false, d3d11  = false, d3d12 = false,
         d3d8  = false, ddraw  = false,
         glide = false;

    SK_TestRenderImports (
      SK_GetModuleHandleW (wszModName),
        &gl,    &vulkan, &d3d9,
        &dxgi,  &d3d11,  &d3d12,
        &d3d8,  &ddraw,  &glide
    );

#ifdef _M_IX86
    if (ddraw)
      SK_BootDDraw ();
    if (d3d8)
      SK_BootD3D8  ();
#endif
    if (dxgi || d3d11 || d3d12)
      SK_BootDXGI ();
    if (d3d9)
      SK_BootD3D9 ();
    if (gl)
      SK_BootOpenGL ();
  }
}

void
SK_WalkModules_StepImpl (       HMODULE       hMod,
                                wchar_t      *wszModName,
                                bool&         rehook,
                                bool&         new_hooks,
                                SK_ModuleEnum when )
{
  // Get the full path to the module's file.
  if ( GetModuleFileNameW ( hMod,
                              wszModName,
                                MAX_PATH ) )
  {
    if (SK_LoadLibrary_IsPinnable (wszModName))
         SK_LoadLibrary_PinModule (wszModName);

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

    if (! config.platform.silent)
    {
      if ( StrStrIW (wszModName, wszSteamAPIDLL)    ||
           StrStrIW (wszModName, wszSteamAPIAltDLL) ||
           StrStrIW (wszModName, wszSteamNativeDLL) ||
           StrStrIW (wszModName, wszSteamClientDLL) ||
           StrStrIW (wszModName, L"steamwrapper") )
      {
        if (SK_GetCurrentGameID () != SK_GAME_ID::JustCause3)
        {
          BOOL
          SK_Steam_PreHookCore (const wchar_t* wszTry = nullptr);

          if (SK_Steam_PreHookCore ())
            new_hooks = true;

          if (SK_HookSteamAPI () > 0)
            new_hooks = true;
        }

        else
        {
          static volatile               LONG __init = 0;
          if (! InterlockedCompareExchange (&__init, TRUE, FALSE))
          {
            SK_Thread_Create ([](LPVOID) -> DWORD
            {
              SK_HookSteamAPI ();

              SK_Thread_CloseSelf ();

              return 0;
            });
          }
        }
      }
    }
  }
}

void
SK_WalkModules (int cbNeeded, HANDLE /*hProc*/, HMODULE* hMods, SK_ModuleEnum when)
{
  bool rehook    = false;
  bool new_hooks = false;

  wchar_t wszModName [MAX_PATH + 2] = { };

  for ( int i = 0; i < sk::narrow_cast <int> (cbNeeded / sizeof (HMODULE)); i++ )
  {
    *wszModName = L'\0';

    SK_WalkModules_StepImpl ( hMods [i],
                            wszModName, rehook,
                                     new_hooks, when );
  }

  if (rehook)
  {
    // We took care of ApplyQueuedHooks already
    if (SK_ReHookLoadLibrary ())
      return;
  }

  if (rehook || new_hooks || when == SK_ModuleEnum::PostLoad)
  {
#ifdef SK_AGGRESSIVE_HOOKS
    SK_ApplyQueuedHooks ();
#endif
  }
}

void
SK_PrintUnloadedDLLs (iSK_Logger* pLogger)
{
  if (pLogger == nullptr)
    return;

  #pragma pack (push,8)
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
#pragma pack (pop)

    static HMODULE hModNtDLL =
      SK_IsModuleLoaded   (L"NtDll") != 0 ?
      SK_GetModuleHandleW (L"NtDll")      :
          SK_LoadLibraryW (L"NtDll");

  static auto RtlGetUnloadEventTraceEx =
    reinterpret_cast <RtlGetUnloadEventTraceEx_pfn> (
      SK_GetProcAddress (hModNtDLL, "RtlGetUnloadEventTraceEx")
    );

  if (RtlGetUnloadEventTraceEx == nullptr)
    return;

  PULONG element_size  = nullptr,
         element_count = nullptr;
  PVOID trace_log = nullptr;

  RtlGetUnloadEventTraceEx (&element_size, &element_count, &trace_log);

  if ( reinterpret_cast <uintptr_t> (element_size)  == 0 ||
       reinterpret_cast <uintptr_t> (element_count) == 0 ||
       reinterpret_cast <uintptr_t> (trace_log)     == 0    )
  {
    return;
  }

  RTL_UNLOAD_EVENT_TRACE* pTraceEntry =
    *static_cast <RTL_UNLOAD_EVENT_TRACE **>(trace_log);

        ULONG ElementSize  = *element_size;
  const ULONG ElementCount = *element_count;

  if (ElementCount > 0)
  {
    pLogger->LogEx ( false,
      L"================================================================== "
      L"(List Unloads) "
      L"==================================================================\n" );

    for (ULONG i = 0; i < ElementCount; i++)
    {
      if (! SK_ValidatePointer (pTraceEntry, true))
        continue;

      if (pTraceEntry->BaseAddress != nullptr)
      {
        pLogger->Log ( L"[%02lu] Unloaded '%32ws' [ (0x%p) : (0x%p) ]",
                      pTraceEntry->Sequence,    pTraceEntry->ImageName,
                      pTraceEntry->BaseAddress, (LPVOID)((uintptr_t)pTraceEntry->BaseAddress +
                                                                    pTraceEntry->SizeOfImage) );

        std::wstring ver_str =
          SK_GetDLLVersionStr (pTraceEntry->ImageName);

        if (ver_str != L"N/A")
        {
          pLogger->Log (
            L"[%02lu]  * Ver.:  %s",
              i, ver_str.c_str () );
        }
      }

      pTraceEntry =
        (RTL_UNLOAD_EVENT_TRACE *)((uintptr_t)pTraceEntry + ElementSize);
    }
  }

  SK_FreeLibrary (hModNtDLL);
}

void
__stdcall
SK_EnumLoadedModules (SK_ModuleEnum when)
{
  // Begin logging new loads after this
  SK_LoadLibrary_SILENCE = false;

  static iSK_Logger* pLogger  = SK_CreateLog (L"logs/modules.log");
  const  DWORD       dwProcID = GetCurrentProcessId ();
         DWORD       cbNeeded = 0;
         HANDLE      hProc    =
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
    working_set->count  = cbNeeded / sizeof (HMODULE);
    working_set->when   = when;

    if (when == SK_ModuleEnum::PreLoad)
    {
      SK_WalkModules (cbNeeded, hProc, working_set->modules, when);
    }

    SK_Thread_Create ([](LPVOID user) -> DWORD
    {
      static SK_AutoHandle hWalkDone (
        SK_CreateEvent (nullptr, FALSE, TRUE, nullptr)
      );

      SetCurrentThreadDescription (L"[SK] DLL Enumerator");
      SetThreadPriority           (GetCurrentThread (), THREAD_PRIORITY_ABOVE_NORMAL);

      if ( WAIT_TIMEOUT ==
             SK_WaitForSingleObject (hWalkDone.m_h, 7500UL) )
      {
        pLogger->Log (L"Timeout during SK_WalkModules, continuing to prevent deadlock...");
      }

      WaitForInit ();

      auto CleanupLog =
      [](iSK_Logger*& pLogger) ->
      void
      {
        if (pLogger != nullptr )
          std::exchange (pLogger, nullptr)->close ();
      };

      // Doing a full enumeration is slow as hell, spawn a worker thread for this
      //   and learn to deal with the fact that some symbol names will be invalid;
      //     the crash handler will load them, but certain diagnostic readings under
      //       normal operation will not.
      auto* pWorkingSet =
        static_cast <enum_working_set_s *> (user);

      const SK_ModuleEnum when =
             pWorkingSet->when;

      if (pWorkingSet->proc == nullptr && (when != SK_ModuleEnum::PreLoad))
      {
        delete
          std::exchange (pWorkingSet, nullptr);

        SetEvent (hWalkDone.m_h);

        CleanupLog   (pLogger);
        SK_Thread_CloseSelf ();

        return 0;
      }

      if ( when != SK_ModuleEnum::PreLoad )
        SK_WalkModules (    pWorkingSet->count * sizeof (HMODULE),
                            pWorkingSet->proc,
                            pWorkingSet->modules,
                            pWorkingSet->when );
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

      delete
        std::exchange (pWorkingSet, nullptr);

      SetEvent (hWalkDone.m_h);

      SK_Thread_CloseSelf ();

      return 0;
    }, (LPVOID)working_set);
  }
}


void
__stdcall
SK_PreInitLoadLibrary (void)
{
  SK::Diagnostics::Debugger::Allow (true);

  LoadPackagedLibrary_Original = nullptr; // Windows 8 feature
}



template <typename _T>
BOOL
__stdcall
BlacklistLibrary (const _T* lpFileName)
{
// This is being phased out
#if 1
  std::ignore = lpFileName;
#else
  if (lpFileName == nullptr)
    return FALSE;

#pragma push_macro ("StrStrI")
#pragma push_macro ("GetModuleHandleEx")
#pragma push_macro ("LoadLibrary")

#pragma push_macro ("StrStrI")
#undef StrStrI
#undef GetModuleHandleEx
#undef LoadLibrary

  static StrStrI_pfn            StrStrI =
    (StrStrI_pfn)
                LPCVOID ( std::type_index (typeid (_T)) ==
                          std::type_index (typeid (wchar_t)) ?
                 (StrStrI_pfn)           &StrStrIW           :
                 (StrStrI_pfn)           &StrStrIA           );

  static GetModuleHandleEx_pfn  GetModuleHandleEx =
    (GetModuleHandleEx_pfn)
                LPCVOID ( std::type_index (typeid (     _T)) ==
                          std::type_index (typeid (wchar_t)) ?
                 (GetModuleHandleEx_pfn) &GetModuleHandleExW :
                 (GetModuleHandleEx_pfn) &GetModuleHandleExA );

  static LoadLibrary_pfn        LoadLibrary =
    (LoadLibrary_pfn)
                LPCVOID ( std::type_index (typeid (     _T)) ==
                          std::type_index (typeid (wchar_t)) ?
                      (LoadLibrary_pfn) &SK_LoadLibraryW :
                      (LoadLibrary_pfn) &SK_LoadLibraryA );

  if (true/*config.compatibility.disable_streamline_incompatible_software*/)
  {
    static bool has_streamline =
       PathFileExistsW (L"sl.interposer.dll") ||
      GetModuleHandleW (L"sl.interposer.dll") != nullptr;
      
    static std::initializer_list <std::basic_string <_T>> incompatible_dlls =
    {
      SK_TEXT("fps-mon64.dll")
    };

    if (has_streamline)
    {
      for (auto& it : incompatible_dlls)
      {
        if (StrStrI (lpFileName, it.c_str ()))
        {
          SK_LOGs0 ( L"DLL Loader",
                     L"Known Streamline Incompatible DLL Blocked to Prevent Crashing" );

          return TRUE;
        }
      }
    }
  }

  if (config.compatibility.disable_nv_bloat)
  {
    static std::initializer_list <std::basic_string <_T>> nv_blacklist = {
      SK_TEXT("rxgamepadinput.dll"),
      SK_TEXT("rxcore.dll"),
      SK_TEXT("nvinject.dll"),
      SK_TEXT("rxinput.dll"),
      SK_TEXT("nvspcap"),
      SK_TEXT("nvSCPAPI")
    };

    for ( auto& it : nv_blacklist )
    {
      if (StrStrI (lpFileName, it.c_str ()))
      {
        HMODULE hModNV;

        if ( GetModuleHandleEx (
               GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                 it.c_str (), &hModNV)
           )
        {
          SK_FreeLibrary (hModNV);
        }

        return TRUE;
      }
    }
  }

  if (config.nvidia.bugs.bypass_ansel)
  {
    static std::initializer_list <std::basic_string <_T>> nv_blacklist =
    {
      SK_RunLHIfBitness (64, SK_TEXT("nvcamera64.dll"),
                             SK_TEXT("nvcamera.dll"))
    };

    for ( auto& it : nv_blacklist )
    {
      if (StrStrI (lpFileName, it.c_str ()))
      {
        HMODULE hModNV;

        if ( GetModuleHandleEx (
               GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                 it.c_str (), &hModNV)
           )
        {
          SK_FreeLibrary (hModNV);
        }

        return TRUE;
      }
    }
  }

  if (StrStrI (lpFileName, SK_TEXT("openxr_loader.dll")))
  {
    SK_LOGs0 ( L"DLL Loader",
                 L"OpenXR DLL Blocked to Prevent Deadlock at Shutdown" );

    return TRUE;
  }

#pragma pop_macro ("StrStrI")
#pragma pop_macro ("GetModuleHandleEx")
#pragma pop_macro ("LoadLibrary")
#endif

  return FALSE;
}