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

#include <SpecialK/SpecialK.h>

#include <Windows.h>
#include <Shlwapi.h>
#include <process.h>

#include <SpecialK/diagnostics/memory.h>
#include <SpecialK/diagnostics/modules.h>
#include <SpecialK/diagnostics/debug_utils.h>
#include <SpecialK/diagnostics/load_library.h>

#include <SpecialK/core.h>
#include <SpecialK/config.h>

#include <SpecialK/input/dinput8_backend.h>
#include <SpecialK/render/dxgi/dxgi_backend.h>
#include <SpecialK/render/d3d9/d3d9_backend.h>
#include <SpecialK/render/gl/opengl_backend.h>

#ifndef _WIN64
#include <SpecialK/render/d3d8/d3d8_backend.h>
#include <SpecialK/render/ddraw/ddraw_backend.h>
#endif

#include <SpecialK/log.h>
#include <SpecialK/utility.h>
#include <SpecialK/thread.h>
#include <SpecialK/tls.h>

#include <SpecialK/hooks.h>
#include <SpecialK/injection/injection.h>
#include <SpecialK/diagnostics/modules.h>


// Fix that stupid macro that redirects to Unicode/ANSI
#undef LoadLibrary

// Don't EVER make these function calls from this code unit.
#define LoadLibrary int x = __stdcall;
#define FreeLibrary int x = __stdcall;

static bool _HasLocalDll = false;

skModuleRegistry SK_Modules;

SK_Thread_HybridSpinlock* init_mutex        = nullptr;
SK_Thread_HybridSpinlock* budget_mutex      = nullptr;
SK_Thread_HybridSpinlock* wmi_cs            = nullptr;
SK_Thread_HybridSpinlock* cs_dbghelp        = nullptr;
SK_Thread_HybridSpinlock* steam_mutex       = nullptr;
SK_Thread_HybridSpinlock* steam_callback_cs = nullptr;
SK_Thread_HybridSpinlock* steam_popup_cs    = nullptr;
SK_Thread_HybridSpinlock* steam_init_cs     = nullptr;

volatile          long __SK_DLL_Ending       = FALSE;
volatile          long __SK_DLL_Attached     = FALSE;
            __time64_t __SK_DLL_AttachTime   = 0ULL;
volatile          LONG __SK_Threads_Attached = 0UL;
volatile          LONG __SK_DLL_Refs         = 0UL;
volatile          long __SK_HookContextOwner = FALSE;

extern volatile  DWORD __SK_TLS_INDEX;

        volatile LONG  lLastThreadCreate     = 0;


class SK_DLL_Bootstrapper
{
  using BootstrapEntryPoint_pfn = bool (*)(void);
  using BootstrapTerminate_pfn  = bool (*)(void);

public:
  std::set <std::wstring> wrapper_dlls;

  BootstrapEntryPoint_pfn start;
  BootstrapTerminate_pfn  shutdown;
};

static const
std::unordered_map <DLL_ROLE, SK_DLL_Bootstrapper>
  __SK_DLL_Bootstraps = {
    { DLL_ROLE::DXGI,       { { L"dxgi.dll", L"d3d11.dll" }, SK::DXGI::Startup,   SK::DXGI::Shutdown   } },
    { DLL_ROLE::D3D11_CASE, { { L"dxgi.dll", L"d3d11.dll" }, SK::DXGI::Startup,   SK::DXGI::Shutdown   } },
    { DLL_ROLE::D3D9,       { { L"d3d9.dll"               }, SK::D3D9::Startup,   SK::D3D9::Shutdown   } },
    { DLL_ROLE::OpenGL,     { { L"OpenGL32.dll"           }, SK::OpenGL::Startup, SK::OpenGL::Shutdown } },
    { DLL_ROLE::DInput8,    { { L"dinput8.dll"            }, SK::DI8::Startup,    SK::DI8::Shutdown    } },
#ifndef _WIN64
    { DLL_ROLE::D3D8,       { { L"d3d8.dll"               }, SK::D3D8::Startup,   SK::D3D8::Shutdown   } },
    { DLL_ROLE::DDraw,      { { L"ddraw.dll"              }, SK::DDraw::Startup,  SK::DDraw::Shutdown  } },
#endif
  };



skWin32Module&
skModuleRegistry::HostApp (HMODULE hModToSet)
{
  static skWin32Module hModApp =
    skWin32Module::Uninitialized;

  if ( hModApp   == skWin32Module::Uninitialized &&
       hModToSet != skWin32Module::Uninitialized    )
  {
    hModApp =
      hModToSet;
  }

  return
    hModApp;
}

skWin32Module&
skModuleRegistry::Self (HMODULE hModToSet)
{
  static skWin32Module hModSelf =
    skWin32Module::Uninitialized;

  if ( hModSelf  == skWin32Module::Uninitialized &&
       hModToSet != skWin32Module::Uninitialized    )
  {
    hModSelf =
      hModToSet;
  }

  return
    hModSelf;
}

  HMODULE
  __stdcall
  SK_GetDLL (void)
{
  return __SK_hModSelf;
}

typedef HHOOK (NTAPI *NtUserSetWindowsHookEx_pfn)(
          HINSTANCE hMod,
     const wchar_t* UnsafeModuleName,
              DWORD ThreadId,
                int HookId,
           HOOKPROC HookProc,
               BOOL Ansi );

typedef LRESULT (NTAPI *NtUserCallNextHookEx_pfn)(
 _In_opt_ HHOOK  hhk,
 _In_     int    nCode,
 _In_     WPARAM wParam,
 _In_     LPARAM lParam
);

extern NtUserCallNextHookEx_pfn NtUserCallNextHookEx;

#include <cwctype>

extern bool SK_COM_TestInit (void);

// If the process doesn't have integrity to manipulate the UI, we have no
//   real interest in it and can eliminate tons of compat. issues by bailing
//     out now!
BOOL
SK_KeepAway (void)
{
//#define SK_NAIVE
#ifdef SK_NAIVE
  return FALSE;
#endif

  if (! SK_COM_TestInit ())
    return TRUE;

#define SK_PARANOID
#ifdef SK_PARANOID
  BOOL  bNotAUserInteractiveApplication = FALSE;
  DWORD dwIntegrityLevel                = std::numeric_limits <DWORD>::max ();

  PSID pSid =
    SK_Win32_GetTokenSid (TokenIntegrityLevel);

  if (pSid != nullptr)
  {
    dwIntegrityLevel = *GetSidSubAuthority      (pSid,
        (DWORD)(UCHAR)(*GetSidSubAuthorityCount (pSid) - 1));

    bNotAUserInteractiveApplication =
      ( dwIntegrityLevel < ( SECURITY_MANDATORY_MEDIUM_RID /*+ 0x10*/ ) );

    SK_Win32_ReleaseTokenSid (pSid);
  }

  if (bNotAUserInteractiveApplication)
    return TRUE;
#endif

  // If user-interactive, check against an internal blacklist
  #include <SpecialK/injection/blacklist.h>

  wchar_t     wszHostApp [MAX_PATH * 2 + 1] = { };
  wcsncpy_s ( wszHostApp, MAX_PATH, SK_GetHostApp (), _TRUNCATE );

  wchar_t *pwsz = wszHostApp;
  while ( *pwsz != L'\0' )
  {
    if (pwsz > ( wszHostApp + MAX_PATH ))
    {
      break;
    }

    *pwsz =
      std::towlower (*pwsz);

    pwsz =
       CharNextW (pwsz);
  }

  bool blacklisted =
    __blacklist.count (wszHostApp) > 0;

  return blacklisted;
}

//=========================================================================
BOOL
APIENTRY
DllMain ( HMODULE hModule,
          DWORD   ul_reason_for_call,
          LPVOID  lpReserved )
{
  UNREFERENCED_PARAMETER (lpReserved);

  switch (ul_reason_for_call)
  {
    case DLL_PROCESS_ATTACH:
    {
      // Try, if assigned already (how?!) do not deadlock the Kernel loader
      if ( __SK_hModSelf       != hModule )
        skModuleRegistry::Self   (hModule);

      else
        return TRUE;

      WriteULongRelease (
        &__SK_TLS_INDEX,
          FlsAlloc (nullptr)
      );

      SK_TLS *pTLS =
        SK_TLS_Bottom ();

      if (pTLS)
          pTLS->debug.in_DllMain = true;


      auto EarlyOut =
      [&](BOOL bRet = TRUE)
      {
        if (pTLS)
            pTLS->debug.in_DllMain = false;

        if (! bRet)
        {
          FlsFree (
            ReadULongAcquire (&__SK_TLS_INDEX)
          );
        }

        bRet = TRUE;

        return bRet;
      };


      // We use SKIM for injection and rundll32 for various tricks involving restarting
      //   the currently running game; neither needs or even wants this DLL fully
      //     initialized!
      if (SK_GetHostAppUtil ().isInjectionTool ())
      {
        SK_EstablishRootPath ();

        return
          EarlyOut (TRUE);
      }


      // Keep this DLL out of anything that doesn't handle User Interfaces,
      //   everyone will be much happier that way =P
      if (SK_KeepAway ())
      {
        return
          EarlyOut (FALSE);
      }

      // We reserve the right to deny attaching the DLL, this will generally
      //   happen if a game does not opt-in to system wide injection.
      if (! SK_EstablishDllRole (hModule))              return EarlyOut (FALSE);

      // We don't want to initialize the DLL, but we also don't want it to
      //   re-inject itself constantly; just return TRUE here.
      else if (SK_GetDLLRole () == DLL_ROLE::INVALID)   return EarlyOut (FALSE);

      if (! SK_Attach (SK_GetDLLRole ()))               return EarlyOut (FALSE);


      InterlockedIncrementRelease (&__SK_DLL_Refs);


      // If we got this far, it's because this is an injection target
      //
      //   Must hold a reference to this DLL so that removing the global hook does
      //     not crash the game.
      SK_Inject_AcquireProcess ();

      if (pTLS)
          pTLS->debug.in_DllMain = false;

      return TRUE;
    } break;


    case DLL_PROCESS_DETACH:
    {
      SK_Thread_ScopedPriority prio_boost (THREAD_PRIORITY_HIGHEST);

      if (! InterlockedCompareExchangeRelease (&__SK_DLL_Ending, TRUE, FALSE))
      {
        // If the DLL being unloaded is the source of a global hook, then
        //   shut that down before detaching the DLL.
        if (ReadAcquire (&__SK_HookContextOwner))
        {
          SKX_RemoveCBTHook ();

          // If SKX_RemoveCBTHook (...) is successful: (__SK_HookContextOwner = 0)
          if (! ReadAcquire (&__SK_HookContextOwner))
          {
            SK_RunLHIfBitness ( 64, DeleteFileW (L"SpecialK64.pid"),
                                    DeleteFileW (L"SpecialK32.pid") );
          }
        }
      }

      if (ReadAcquire (&__SK_DLL_Attached))
      {
        SK_Memory_RemoveHooks ();

        if (! SK_GetHostAppUtil ().isInjectionTool ())
          SK_Detach (SK_GetDLLRole ());

        SK_TLS *pTLS     = nullptr;
        auto    tls_slot =
          SK_GetTLS (&pTLS);

        if ( tls_slot != nullptr &&
             tls_slot->dwTlsIdx  == ReadULongAcquire (&__SK_TLS_INDEX) )
        {
          FlsFree (
              tls_slot->dwTlsIdx
          );

          if (        pTLS != nullptr )
               delete pTLS;
        }
      }

      else
      {
        FlsFree (
          InterlockedCompareExchangeRelease ( &__SK_TLS_INDEX, TLS_OUT_OF_INDEXES,
                     ReadULongAcquire      (  &__SK_TLS_INDEX                      )
                                            ) 
                );
      }

#ifdef DEBUG
      else {
      //Sanity FAILURE: Attempt to detach something that was not properly attached?!
        dll_log.Log (L"[ SpecialK ]  ** SANITY CHECK FAILED: DLL was never attached !! **");
      }
#endif
    } break;



    case DLL_THREAD_ATTACH:
    {
      InterlockedIncrementAcquire (&lLastThreadCreate);

      if (ReadAcquire (&__SK_DLL_Attached))
      {
        InterlockedIncrementAcquire (&__SK_Threads_Attached);

        SK_TLS *pTLS =
          SK_TLS_Bottom ();

        if (pTLS != nullptr)
        {
            pTLS->debug.mapped = true;
        }
      }
    }
    break;


    case DLL_THREAD_DETACH:
    {
      InterlockedIncrementRelease (&lLastThreadCreate);

      if (ReadAcquire (&__SK_DLL_Attached))
      {
        //SK_TLS *pOldTLS =
          SK_CleanupTLS ();

        //delete pOldTLS;
      }
    }
    break;
  }

  return TRUE;
}
//=========================================================================




HMODULE
SK_GetLocalModuleHandle (const wchar_t* wszModule)
{
  wchar_t    wszLocalModulePath [MAX_PATH * 2 + 1] = { };
  wcsncpy_s (wszLocalModulePath, MAX_PATH, SK_GetHostPath (), _TRUNCATE );
  lstrcatW (wszLocalModulePath, LR"(\)");
  lstrcatW (wszLocalModulePath, wszModule);

  return
    SK_GetModuleHandleW (wszLocalModulePath);
};

HMODULE
SK_LoadLocalModule (const wchar_t* wszModule)
{
  wchar_t    wszLocalModulePath [MAX_PATH * 2] = { };
  wcsncpy_s (wszLocalModulePath, MAX_PATH, SK_GetHostPath (), _TRUNCATE );
  lstrcatW  (wszLocalModulePath, LR"(\)");
  lstrcatW  (wszLocalModulePath, wszModule);

  HMODULE hMod =
    SK_GetModuleHandleW (wszLocalModulePath);

  if (hMod == nullptr)
  {
    return
      LoadLibraryW (wszLocalModulePath);
  }

  return hMod;
};

// If this is the global injector and there is a wrapper version
//   of Special K in the DLL search path, then bail-out!
BOOL
SK_TryLocalWrapperFirst (const std::set <std::wstring>& dlls)
{
  for ( const auto& dll : dlls )
  {
    if ( SK_IsDLLSpecialK   (dll.c_str ()) &&
         SK_LoadLocalModule (dll.c_str ()) )
    {
      return TRUE;
    }
  }

  return FALSE;
};


BOOL
SK_DontInject (void)
{
  _HasLocalDll = true;

  LONG idx_to_free =
    InterlockedCompareExchangeRelease ( &__SK_TLS_INDEX, TLS_OUT_OF_INDEXES,
                      ReadULongAcquire (&__SK_TLS_INDEX) );

  if (idx_to_free != TLS_OUT_OF_INDEXES)
  {
    FlsFree (idx_to_free);
  }

  SK_SetDLLRole              (DLL_ROLE::INVALID);
  InterlockedExchangeAcquire (&__SK_DLL_Attached, FALSE);

  return FALSE;
}


bool
_SKM_AutoBootLastKnownAPI (SK_RenderAPI last_known)
{
  using role_from_api_tbl =
    std::map < SK_RenderAPI, std::tuple < DLL_ROLE, BOOL > >;

  role_from_api_tbl
    role_reversal =
    {
      { SK_RenderAPI::D3D9,
          { DLL_ROLE::D3D9,           config.apis.d3d9.hook } },
      { SK_RenderAPI::D3D9Ex,
          { DLL_ROLE::D3D9,         config.apis.d3d9ex.hook } },

      { SK_RenderAPI::D3D10,
          { DLL_ROLE::DXGI, FALSE /* Stupid API--begone! */ } },
      { SK_RenderAPI::D3D11,
          { DLL_ROLE::DXGI,         config.apis.d3d9ex.hook } },
      { SK_RenderAPI::D3D12,
          { DLL_ROLE::DXGI,                            TRUE } },

     { SK_RenderAPI::OpenGL,
         { DLL_ROLE::OpenGL,        config.apis.OpenGL.hook } },

#ifndef _WIN64

      // Bitness:  32-Bit  (Add:  DDraw, D3D8 and Glide)

      { SK_RenderAPI::D3D8,
          { DLL_ROLE::DXGI,           config.apis.d3d8.hook } },
      { SK_RenderAPI::D3D8On11,
          { DLL_ROLE::DXGI,     config.apis.d3d8.hook   &&
                                config.apis.dxgi.d3d11.hook } },

      { SK_RenderAPI::Glide,
          { DLL_ROLE::Glide,         config.apis.glide.hook } },
      { SK_RenderAPI::GlideOn11,
          { DLL_ROLE::Glide,    config.apis.glide.hook  &&
                                config.apis.dxgi.d3d11.hook } },

      { SK_RenderAPI::DDraw,
          { DLL_ROLE::DDraw,         config.apis.ddraw.hook } },
      { SK_RenderAPI::DDrawOn11,
          { DLL_ROLE::DDraw,    config.apis.ddraw.hook  &&
                                config.apis.dxgi.d3d11.hook } },

      { SK_RenderAPI::Vulkan,    { DLL_ROLE::INVALID, FALSE } },
#else

      // Bitness:  64-Bit  (Remove Legacy APIs  +  Add Vulkan)

      { SK_RenderAPI::D3D8,      { DLL_ROLE::INVALID, FALSE } },
      { SK_RenderAPI::D3D8On11,  { DLL_ROLE::INVALID, FALSE } },

      { SK_RenderAPI::Glide,     { DLL_ROLE::INVALID, FALSE } },
      { SK_RenderAPI::GlideOn11, { DLL_ROLE::INVALID, FALSE } },

      { SK_RenderAPI::DDraw,     { DLL_ROLE::INVALID, FALSE } },
      { SK_RenderAPI::DDrawOn11, { DLL_ROLE::INVALID, FALSE } },

      { SK_RenderAPI::Vulkan,
          { DLL_ROLE::Vulkan,       config.apis.Vulkan.hook } },
#endif
  };


  bool auto_boot_viable = false;

  if (role_reversal.count (last_known) != 0)
  {
    auto_boot_viable =
      std::get <1> (role_reversal [last_known]);

    if (auto_boot_viable)
    {
      SK_SetDLLRole (std::get <0> (role_reversal [last_known]));

      // This actually _saves_ the config, after parsing and
      //   trimming it.
      SK_LoadConfig (L"SpecialK");

      config.apis.last_known = SK_RenderAPI::Reserved;
    }
  }

  return
    auto_boot_viable;
}


BOOL
__stdcall
SK_EstablishDllRole (skWin32Module&& module)
{
  SK_SetDLLRole (DLL_ROLE::INVALID);


#ifndef _WIN64
  static bool has_dgvoodoo =
    GetFileAttributesW (
      SK_FormatStringW ( LR"(%ws\PlugIns\ThirdParty\dgVoodoo\d3dimm.dll)",
                          std::wstring ( SK_GetDocumentsDir () + LR"(\My Mods\SpecialK)" ).c_str ()
                       ).c_str ()
    ) != INVALID_FILE_ATTRIBUTES;
#endif

  const wchar_t* wszSelfTitledDLL =
    module;

  const wchar_t* wszShort =
    CharNextW ( SK_Path_wcsrchr ( wszSelfTitledDLL, *LR"(\)" ) );

  // The DLL path was _already_ in non-fully-qualified form... oops?
  if (wszShort == static_cast <const wchar_t *>(nullptr) + 1)
    wszShort = wszSelfTitledDLL;


  if (! SK_Path_wcsicmp (wszShort, L"dinput8.dll"))
  {
    SK_SetDLLRole (DLL_ROLE::DInput8);

    if ( SK_IsDLLSpecialK (L"dxgi.dll")     ||
         SK_IsDLLSpecialK (L"d3d9.dll")     ||
         SK_IsDLLSpecialK (L"d3d11.dll")    ||
         SK_IsDLLSpecialK (L"OpenGL32.dll") ||
         SK_IsDLLSpecialK (L"ddraw.dll")    ||
         SK_IsDLLSpecialK (L"d3d8.dll"))
    {
      SK_MessageBox ( L"Please limit Special K to one (1) "
                       "of the following: d3d8.dll, d3d9.dll,"
                                         " d3d11.dll, ddraw.dll, dinput8.dll,"
                                         " dxgi.dll or OpenGL32.dll",
                      L"Conflicting Special K Injection DLLs Detected",
                        MB_SYSTEMMODAL | MB_SETFOREGROUND |
                        MB_ICONERROR   | MB_OK );
    }
  }

  else if (! SK_Path_wcsicmp (wszShort, L"dxgi.dll"))
    SK_SetDLLRole (DLL_ROLE::DXGI);

  else if (! SK_Path_wcsicmp (wszShort, L"d3d11.dll"))
  {
    SK_SetDLLRole ( static_cast <DLL_ROLE> ( (int)DLL_ROLE::DXGI |
                                             (int)DLL_ROLE::D3D11 ) );
  }

#ifndef _WIN64
  else if (! SK_Path_wcsicmp (wszShort, L"d3d8.dll")  && has_dgvoodoo)
    SK_SetDLLRole (DLL_ROLE::D3D8);

  else if (! SK_Path_wcsicmp (wszShort, L"ddraw.dll") && has_dgvoodoo)
    SK_SetDLLRole (DLL_ROLE::DDraw);
#endif

  else if (! SK_Path_wcsicmp (wszShort, L"d3d9.dll"))
    SK_SetDLLRole (DLL_ROLE::D3D9);

  else if (! SK_Path_wcsicmp (wszShort, L"OpenGL32.dll"))
    SK_SetDLLRole (DLL_ROLE::OpenGL);


  //
  // This is an injected DLL, not a wrapper DLL...
  //
  else if ( SK_Path_wcsstr (wszShort, L"SpecialK") )
  {
             // SET the injected state
             SK_IsInjected (true);
    bool explicit_inject = false;

    config.system.central_repository = true;

    wchar_t wszD3D9  [MAX_PATH + 2] = { };
    wchar_t wszDXGI  [MAX_PATH + 2] = { };
    wchar_t wszD3D11 [MAX_PATH + 2] = { };
    wchar_t wszGL    [MAX_PATH + 2] = { };
    wchar_t wszDI8   [MAX_PATH + 2] = { };

#ifndef _WIN64
    wchar_t wszD3D8  [MAX_PATH + 2] = { };
    wchar_t wszDDraw [MAX_PATH + 2] = { };
#endif

    lstrcatW (wszD3D9,   SK_GetHostPath ());
    lstrcatW (wszD3D9,   LR"(\SpecialK.d3d9)");

#ifndef _WIN64
    lstrcatW (wszD3D8,   SK_GetHostPath ());
    lstrcatW (wszD3D8,   LR"(\SpecialK.d3d8)");

    lstrcatW (wszDDraw,  SK_GetHostPath ());
    lstrcatW (wszDDraw,  LR"(\SpecialK.ddraw)");
#endif

    lstrcatW (wszDXGI,  SK_GetHostPath ());
    lstrcatW (wszDXGI,  LR"(\SpecialK.dxgi)");

    lstrcatW (wszD3D11, SK_GetHostPath ());
    lstrcatW (wszD3D11, LR"(\SpecialK.d3d11)");

    lstrcatW (wszGL,    SK_GetHostPath ());
    lstrcatW (wszGL,    LR"(\SpecialK.OpenGL32)");

    lstrcatW (wszDI8,   SK_GetHostPath ());
    lstrcatW (wszDI8,   LR"(\SpecialK.DInput8)");


    if      ( GetFileAttributesW (wszD3D9) != INVALID_FILE_ATTRIBUTES )
    {
      SK_SetDLLRole (DLL_ROLE::D3D9);
      explicit_inject = true;
    }

#ifndef _WIN64
    else if ( GetFileAttributesW (wszD3D8) != INVALID_FILE_ATTRIBUTES && has_dgvoodoo )
    {
      SK_SetDLLRole (DLL_ROLE::D3D8);
      explicit_inject = true;
    }

    else if ( GetFileAttributesW (wszDDraw) != INVALID_FILE_ATTRIBUTES && has_dgvoodoo )
    {
      SK_SetDLLRole (DLL_ROLE::DDraw);
      explicit_inject = true;
    }
#endif

    else if ( GetFileAttributesW (wszDXGI) != INVALID_FILE_ATTRIBUTES )
    {
      SK_SetDLLRole (DLL_ROLE::DXGI);
      explicit_inject = true;
    }

    else if ( GetFileAttributesW (wszD3D11) != INVALID_FILE_ATTRIBUTES )
    {
      SK_SetDLLRole ( static_cast <DLL_ROLE> ( (int)DLL_ROLE::DXGI |
                                               (int)DLL_ROLE::D3D11 ) );
      explicit_inject = true;
    }

    else if ( GetFileAttributesW (wszGL) != INVALID_FILE_ATTRIBUTES )
    {
      SK_SetDLLRole (DLL_ROLE::OpenGL);
      explicit_inject = true;
    }

    else if ( GetFileAttributesW (wszDI8) != INVALID_FILE_ATTRIBUTES )
    {
      SK_SetDLLRole (DLL_ROLE::DInput8);
      explicit_inject = true;
    }


    // Opted out of explicit injection, now try automatic
    //
    if (! explicit_inject)
    {
      // This order is not arbitrary, but not worth explaining
      const std::set <std::wstring> local_dlls =
        { L"dxgi.dll",   L"d3d9.dll",
          L"d3d11.dll",  L"OpenGL32.dll",
          L"ddraw.dll",  L"d3d8.dll",
          L"dinput8.dll"                };

      // If there is a local Special K DLL in the game's directory,
      //   load it and then bow-out -- we are done here.
      //
      if ( (! SK_IsHostAppSKIM ()) && SK_TryLocalWrapperFirst ( local_dlls ) )
      {
        return
          SK_DontInject ();
      }


      // Most frequently imported DLLs for games that use SteamAPI
      //
      //  It is trivial to use SteamAPI without linking to the DLL, so
      //    this is not the final test to determine Steam compatibility.
      //
      static sk_import_test_s steam_tests [] =
      {
        { SK_RunLHIfBitness ( 64, "steam_api64.dll",
                                  "steam_api.dll"    ), false },

        {                         "steamnative.dll",    false }
      };

      SK_TestImports ( GetModuleHandle (nullptr), steam_tests, 2 );


      DWORD   dwProcessSize = MAX_PATH;
      wchar_t wszProcessName [MAX_PATH + 2] = { };

      GetModuleFileNameW (nullptr, wszProcessName, dwProcessSize);

      // To catch all remaining Steam games, look for "\SteamApps\" in the
      //   executable path.
      //
      //  These games may not use SteamAPI, but they are designed for the
      //    Steamworks platform and we can connect to the client using the
      //      steam_api{64}.dll files distributed with Special K.
      //
      const bool is_steamworks_game =
        ( steam_tests [0].used | steam_tests [1].used ) ||
           SK_Path_wcsstr (wszProcessName, L"steamapps");


      // If this is a Steamworks game, then lets start doing stuff to it!
      //
      //   => We still need to figure out the primary graphics API.
      //
      if ( is_steamworks_game ||
           SK_Inject_TestWhitelists (SK_GetFullyQualifiedApp ()) )
      {
        SK_EstablishRootPath ();
        SK_LoadConfigEx      (L"SpecialK", false);


        // Try the last-known API first -- if we have one.
        //
        if (config.apis.last_known != SK_RenderAPI::Reserved)
        {
          if (_SKM_AutoBootLastKnownAPI (config.apis.last_known))
          {
            return true;
          }
        }


        // That did not work; examine the game's Import Address Table
        //
        bool gl   = false, vulkan = false, d3d9  = false, d3d11 = false,
             dxgi = false, d3d8   = false, ddraw = false, glide = false;

        SK_TestRenderImports (
          GetModuleHandle (nullptr),
            &gl, &vulkan,
              &d3d9, &dxgi, &d3d11,
                &d3d8, &ddraw, &glide
        );

        gl     |= (SK_GetModuleHandle (L"OpenGL32.dll") != nullptr);
        d3d9   |= (SK_GetModuleHandle (L"d3d9.dll")     != nullptr);

        // Not specific enough; some engines will pull in DXGI even if they
        //   do not use D3D10/11/12/D2D/DWrite
        //
        dxgi   |= (SK_GetModuleHandle (L"dxgi.dll")     != nullptr);

        d3d11  |= (SK_GetModuleHandle (L"d3d11.dll")     != nullptr);
        d3d11  |= (SK_GetModuleHandle (L"d3dx11_43.dll") != nullptr);

#ifndef _WIN64
        d3d8   |= (SK_GetModuleHandle (L"d3d8.dll")     != nullptr);
        ddraw  |= (SK_GetModuleHandle (L"ddraw.dll")    != nullptr);

        if (config.apis.d3d8.hook && d3d8 && has_dgvoodoo)
        {
          if (SK_TryLocalWrapperFirst ({ L"d3d8.dll" }))                return SK_DontInject ();

          config.apis.dxgi.d3d11.hook = true;

          SK_SetDLLRole (DLL_ROLE::D3D8);
        }

        else if (config.apis.ddraw.hook && ddraw && has_dgvoodoo)
        {
          if (SK_TryLocalWrapperFirst ({ L"ddraw.dll" }))               return SK_DontInject ();

          config.apis.dxgi.d3d11.hook = true;

          SK_SetDLLRole (DLL_ROLE::DDraw);
        }

        else
#endif
        if (SK_TryLocalWrapperFirst ({ L"dinput8.dll" }))
        {
          return
            SK_DontInject ();
        }

        else if (config.apis.dxgi.d3d11.hook && (dxgi || d3d11))
        {
          if (SK_TryLocalWrapperFirst ({ L"dxgi.dll", L"d3d11.dll" }))  return SK_DontInject ();

          //if (d3d11)
          //{
          //  SK_SetDLLRole ( static_cast <DLL_ROLE> ( (int)DLL_ROLE::DXGI |
          //                                           (int)DLL_ROLE::D3D11 ) );
          //}
          //
          //else
            SK_SetDLLRole (DLL_ROLE::DXGI);
        }

        else if (config.apis.d3d9.hook && d3d9)
        {
          if (SK_TryLocalWrapperFirst ({ L"d3d9.dll" }))                return SK_DontInject ();

          SK_SetDLLRole (DLL_ROLE::D3D9);
        }

        else if (config.apis.OpenGL.hook && gl)
        {
          if (SK_TryLocalWrapperFirst ({ L"OpenGL32.dll" }))            return SK_DontInject ();

          SK_SetDLLRole (DLL_ROLE::OpenGL);
        }

#ifdef _WIN64
        else if (config.apis.Vulkan.hook && vulkan)
          SK_SetDLLRole (DLL_ROLE::Vulkan);
#endif


        //
        // *** No Freaking Clue What Graphics API This Game Uses ?!
        //
        //  Use the config file to filter out any APIs the user
        //    knows are not valid.
        //
        else
        {
          if (config.apis.d3d9.hook  || config.apis.d3d9ex.hook)
            SK_SetDLLRole (DLL_ROLE::D3D9);

          else if (config.apis.dxgi.d3d11.hook)
            SK_SetDLLRole (DLL_ROLE::DXGI);
//#ifdef _WIN64
//          if (config.apis.dxgi.d3d12.hook)
//            SK_SetDLLRole (DLL_ROLE::DXGI);
//#endif
          else if (config.apis.OpenGL.hook)
            SK_SetDLLRole (DLL_ROLE::OpenGL);
#ifdef _WIN64
          else if (config.apis.Vulkan.hook)
            SK_SetDLLRole (DLL_ROLE::Vulkan);
#else
          else if (config.apis.d3d8.hook && has_dgvoodoo)
            SK_SetDLLRole (DLL_ROLE::D3D8);
          else if (config.apis.ddraw.hook && has_dgvoodoo)
            SK_SetDLLRole (DLL_ROLE::DDraw);
#endif
        }

        if (SK_GetDLLRole () == DLL_ROLE::INVALID)
          SK_SetDLLRole (DLL_ROLE::DXGI); // Auto-Guess DXGI if all else fails...

        // Write any default values to the config file
        SK_LoadConfig (L"SpecialK");


        config.apis.last_known = SK_RenderAPI::Reserved;
      }
    }
  }

  return
    (SK_GetDLLRole () != DLL_ROLE::INVALID);
}




void
SK_CleanupMutex (SK_Thread_HybridSpinlock** ppMutex)
{
  if (*ppMutex != nullptr)
  {
    delete
      static_cast <SK_Thread_HybridSpinlock *> (
        InterlockedCompareExchangePointer ((void **)ppMutex, nullptr, *ppMutex)
      );
  }
};

extern "C" {
  void __cdecl
  __crc32_init (void);
}

BOOL
__stdcall
SK_Attach (DLL_ROLE role)
{
  if (__SK_DLL_Bootstraps.count (role))
  {
    if (! InterlockedCompareExchangeAcquire (&__SK_DLL_Attached, TRUE, FALSE))
    {
      skModuleRegistry::HostApp (GetModuleHandle (nullptr));

      const auto& bootstrap =
        __SK_DLL_Bootstraps.at (role);

      if ( SK_IsInjected           () &&
           SK_TryLocalWrapperFirst (bootstrap.wrapper_dlls))
      {
        return
          SK_DontInject ();
      }

      if (GetFileAttributesW (L"SpecialK.WaitForDebugger") != INVALID_FILE_ATTRIBUTES)
      {
        while (! SK_IsDebuggerPresent ())
          SleepEx (50, FALSE);
      }

      try {
        _time64 (&__SK_DLL_AttachTime);

        cs_dbghelp =
          new SK_Thread_HybridSpinlock (16384);
        budget_mutex =
          new SK_Thread_HybridSpinlock (  100);
        init_mutex   =
          new SK_Thread_HybridSpinlock ( 2150);
        steam_mutex  =
          new SK_Thread_HybridSpinlock (    3);
        wmi_cs       =
          new SK_Thread_HybridSpinlock (  128);

        steam_callback_cs =
          new SK_Thread_HybridSpinlock (256UL);
        steam_popup_cs    =
          new SK_Thread_HybridSpinlock (512UL);
        steam_init_cs     =
          new SK_Thread_HybridSpinlock (128UL);

        __crc32_init ();

        InterlockedCompareExchangeAcquire (
          &__SK_DLL_Attached,
            bootstrap.start (),
              TRUE
        );

        if (ReadAcquire (&__SK_DLL_Attached))
        {
          return TRUE;
        }
      }

      catch (...) {
        SK_CleanupMutex (&budget_mutex); SK_CleanupMutex (&init_mutex);
        SK_CleanupMutex (&wmi_cs);       SK_CleanupMutex (&steam_mutex);

        SK_CleanupMutex (&steam_callback_cs); SK_CleanupMutex (&steam_popup_cs);
        SK_CleanupMutex (&steam_init_cs);
      }
    }
  }


  return
    SK_DontInject ();
}



BOOL
__stdcall
SK_Detach (DLL_ROLE role)
{
  SK_CleanupMutex (&budget_mutex);      SK_CleanupMutex (&init_mutex);
  SK_CleanupMutex (&cs_dbghelp);        SK_CleanupMutex (&wmi_cs);
  SK_CleanupMutex (&steam_mutex);

  SK_CleanupMutex (&steam_callback_cs); SK_CleanupMutex (&steam_popup_cs);
  SK_CleanupMutex (&steam_init_cs);

  ULONG local_refs =
    InterlockedDecrementRelease (&__SK_DLL_Refs);

  if ( local_refs == 0 &&
         InterlockedCompareExchangeRelease (
                    &__SK_DLL_Attached,
                      FALSE,
                        TRUE        )
     )
  {
    SK_Inject_ReleaseProcess ();

    if ( __SK_DLL_Bootstraps.count (role) &&
         __SK_DLL_Bootstraps.at    (role).shutdown () )
    {
      return TRUE;
    }
  }

  else {
    dll_log.Log (L"[ SpecialK ]  ** UNCLEAN DLL Process Detach !! **");
  }

  return FALSE;
}