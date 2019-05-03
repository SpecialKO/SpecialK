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

#include <SpecialK/render/dxgi/dxgi_backend.h>
#include <SpecialK/render/d3d11/d3d11_core.h>
#include <SpecialK/render/d3d9/d3d9_backend.h>
#include <SpecialK/render/gl/opengl_backend.h>

#ifndef _M_AMD64
#include <SpecialK/render/d3d8/d3d8_backend.h>
#include <SpecialK/render/ddraw/ddraw_backend.h>
#endif


char _RTL_CONSTANT_STRING_type_check (const char    *s);
char _RTL_CONSTANT_STRING_type_check (const wchar_t *s);

template <size_t N> class _RTL_CONSTANT_STRING_remove_const_template_class;
template <        > class _RTL_CONSTANT_STRING_remove_const_template_class <sizeof (char   )>
{ public:                                                                  typedef  char    T; };
template <        > class _RTL_CONSTANT_STRING_remove_const_template_class <sizeof (wchar_t)>
{ public:                                                                  typedef  wchar_t T; };

#define _RTL_CONSTANT_STRING_remove_const_macro(s) \
    (const_cast <_RTL_CONSTANT_STRING_remove_const_template_class <sizeof ((s)[0])>::T*> (s))

#define RTL_CONSTANT_STRING(s)                              \
{                                                           \
  sizeof( s ) - sizeof( (s)[0] ),                           \
  sizeof( s ) / sizeof(_RTL_CONSTANT_STRING_type_check(s)), \
  _RTL_CONSTANT_STRING_remove_const_macro(s)                \
}


// Fix that stupid macro that redirects to Unicode/ANSI
#undef LoadLibrary

// Don't EVER make these function calls from this code unit.
#define LoadLibrary int x = __stdcall;
#define FreeLibrary int x = __stdcall;

static bool _HasLocalDll = false;

SK_LazyGlobal <skModuleRegistry> SK_Modules;

SK_Thread_HybridSpinlock* init_mutex          = nullptr;
SK_Thread_HybridSpinlock* budget_mutex        = nullptr;
SK_Thread_HybridSpinlock* wmi_cs              = nullptr;
SK_Thread_HybridSpinlock* cs_dbghelp          = nullptr;
SK_Thread_HybridSpinlock* steam_mutex         = nullptr;
SK_Thread_HybridSpinlock* steam_callback_cs   = nullptr;
SK_Thread_HybridSpinlock* steam_popup_cs      = nullptr;
SK_Thread_HybridSpinlock* steam_init_cs       = nullptr;

                HANDLE __SK_DLL_TeardownEvent = 0;
volatile          long __SK_DLL_Ending        = FALSE;
volatile          long __SK_DLL_Attached      = FALSE;
            __time64_t __SK_DLL_AttachTime    = 0ULL;
volatile          LONG __SK_Threads_Attached  = 0UL;
volatile          LONG __SK_DLL_Refs          = 0UL;
volatile          long __SK_HookContextOwner  = FALSE;

extern volatile  DWORD __SK_TLS_INDEX;

        volatile LONG  lLastThreadCreate      = 0;


class SK_DLL_Bootstrapper
{
  using BootstrapEntryPoint_pfn = bool (*)(void);
  using BootstrapTerminate_pfn  = bool (*)(void);

public:
  std::set <std::wstring> wrapper_dlls;

  BootstrapEntryPoint_pfn start;
  BootstrapTerminate_pfn  shutdown;
};

const std::unordered_map <DLL_ROLE, SK_DLL_Bootstrapper>*
SK_DLL_GetBootstraps (void)
{
  static
    const std::unordered_map <DLL_ROLE, SK_DLL_Bootstrapper>
    __SK_DLL_Bootstraps = {
      { DLL_ROLE::DXGI,       { { L"dxgi.dll", L"d3d11.dll" }, SK::DXGI::Startup,   SK::DXGI::Shutdown   } },
      { DLL_ROLE::D3D11_CASE, { { L"dxgi.dll", L"d3d11.dll" }, SK::DXGI::Startup,   SK::DXGI::Shutdown   } },
      { DLL_ROLE::D3D9,       { { L"d3d9.dll"               }, SK::D3D9::Startup,   SK::D3D9::Shutdown   } },
      { DLL_ROLE::OpenGL,     { { L"OpenGL32.dll"           }, SK::OpenGL::Startup, SK::OpenGL::Shutdown } },
      { DLL_ROLE::DInput8,    { { L"dinput8.dll"            }, SK::DI8::Startup,    SK::DI8::Shutdown    } },
  #ifndef _M_AMD64
      { DLL_ROLE::D3D8,       { { L"d3d8.dll"               }, SK::D3D8::Startup,   SK::D3D8::Shutdown   } },
      { DLL_ROLE::DDraw,      { { L"ddraw.dll"              }, SK::DDraw::Startup,  SK::DDraw::Shutdown  } },
  #endif
    };

  return
    &__SK_DLL_Bootstraps;
}



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
  return
    __SK_hModSelf;
}

//
// Various detection methods to prevent injection into software that is
//   either untested or known incompatible.
//
//  * Applies primarily to global injection, and would prevent games from
//      running at all if extended to proxy/wrapper DLL code injection.
//
INT
SK_KeepAway (void)
{
//#define SK_NAIVE
#ifdef SK_NAIVE
  return FALSE;
#endif

//#define SK_PARANOID
#ifdef SK_PARANOID
  if (! SK_COM_TestInit ())
    return TRUE;

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

  static HMODULE hModNtDll =
    GetModuleHandle (L"NtDll");

  // If this DLL is not present, there's no way in hell we're interested
  //   in hooking this software.
  if (! hModNtDll) return 1;

  // If user-interactive, check against an internal blacklist
  #include <SpecialK/injection/blacklist.h>

  static LdrGetDllHandle_pfn
         LdrGetDllHandle =    (LdrGetDllHandle_pfn)
    GetProcAddress (hModNtDll,"LdrGetDllHandle");

  // Applications using these are known to suspend themselves while holding
  //   a reference to our poor old DLL; so avoid them.
  static const UNICODE_STRING trigger_dlls [] = {
    SK_MakeUnicode (L"Windows.Internal.Shell.Broker.dll"),
    SK_MakeUnicode (L"Windows.UI.Immersive.dll"),
    SK_MakeUnicode (L"ApplicationFrame.dll"),
    SK_MakeUnicode (L"browserbroker.dll"),
    SK_MakeUnicode (L"chromehtml.dll"),
    SK_MakeUnicode (L"libcef.dll")
  };

  auto _TestUndesirableDll = [](auto& list, INT stage) ->
  INT
  {
    for ( auto& unwanted_dll : list )
    {
      HANDLE hMod = 0;

      if ( NT_SUCCESS (
             LdrGetDllHandle (nullptr, nullptr, &unwanted_dll, &hMod)
           )    &&    hMod != nullptr )
      {
        return stage;
      }
    }

    return 0;
  };

  return
    _TestUndesirableDll     ( trigger_dlls, 1 ) ||
      _TestUndesirableDll   ( __graylist,   2 ) ||
        _TestUndesirableDll ( __blacklist,  3 );
}

// Ideally, we want to keep the TLS data stores around so that built-in
//   debug features can analyze finished threads. _I see dead threads..._
SK_LazyGlobal <concurrency::concurrent_queue <SK_TLS *>> __SK_TLS_FreeList;


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


      auto EarlyOut =
      [&](BOOL bRet = TRUE)
      {
        return bRet;
      };


      // We use SKIM for injection and rundll32 for various tricks involving restarting
      //   the currently running game; neither needs or even wants this DLL fully
      //     initialized!
      if (SK_GetHostAppUtil ()->isInjectionTool ())
      {
        // We need a TLS slot immediately
        WriteULongRelease (
          &__SK_TLS_INDEX,
            FlsAlloc (nullptr)
        );

        SK_EstablishRootPath ();

        InterlockedExchange (&__SK_DLL_Attached, 1);

        return
          EarlyOut (TRUE);
      }



      //// Keep this DLL out of anything that doesn't handle User Interfaces,
      ////   everyone will be much happier that way =P
      if (SK_KeepAway () != 0)
      {
        return
          EarlyOut (FALSE);
      }


      // We reserve the right to deny attaching the DLL, this will generally
      //   happen if a game does not opt-in to system wide injection.
      if (! SK_EstablishDllRole (hModule))              return EarlyOut (TRUE);

      // We don't want to initialize the DLL, but we also don't want it to
      //   re-inject itself constantly; just return TRUE here.
      else if (SK_GetDLLRole () == DLL_ROLE::INVALID)   return EarlyOut (TRUE);

      if (! SK_Attach (SK_GetDLLRole ()))               return EarlyOut (TRUE);

      InterlockedIncrementRelease (&__SK_DLL_Refs);


      // If we got this far, it's because this is an injection target
      //
      //   Must hold a reference to this DLL so that removing the global hook does
      //     not crash the game.
      SK_Inject_AcquireProcess ();

      __SK_DLL_TeardownEvent =
        CreateEvent (nullptr, TRUE, FALSE, nullptr);

      return TRUE;
    } break;



    case DLL_PROCESS_DETACH:
    {
      if (__SK_DLL_TeardownEvent != 0)
        SetEvent (__SK_DLL_TeardownEvent);

      if (! InterlockedCompareExchangeRelease (&__SK_DLL_Ending, TRUE, FALSE))
      {
        // If the DLL being unloaded is the source of a global hook, then
        //   shut that down before detaching the DLL.
        if (ReadAcquire (&__SK_HookContextOwner))
        {
          SKX_RemoveCBTHook ();
        }
      }


      if (ReadAcquire (&__SK_DLL_Attached))
      {
        SK_Memory_RemoveHooks ();

        if (! SK_GetHostAppUtil ()->isInjectionTool ())
          SK_Detach (SK_GetDLLRole ());

        SK_TLS *pTLS     = nullptr;
        auto    tls_slot =
          SK_GetTLS (&pTLS);

        if ( tls_slot != nullptr &&
             tls_slot->dwTlsIdx  == ReadULongAcquire (&__SK_TLS_INDEX) )
        {
          delete
            SK_CleanupTLS ();

          FlsFree (
            InterlockedExchange ( &__SK_TLS_INDEX,
                                    TLS_OUT_OF_INDEXES )
          );
        }

        while (! __SK_TLS_FreeList->empty ())
        {
          SK_TLS*
            pZombieTLS = nullptr;

          if (__SK_TLS_FreeList->try_pop (pZombieTLS))
          {
            delete pZombieTLS;
          }
        }
      }

      else if ( ReadULongAcquire ( &__SK_TLS_INDEX ) <= 1088 )
      {
        FlsFree (
          InterlockedExchange ( &__SK_TLS_INDEX,
                                  TLS_OUT_OF_INDEXES )
                );
      }

      if (__SK_DLL_TeardownEvent != 0)
        CloseHandle (__SK_DLL_TeardownEvent);

#ifdef DEBUG
      else {
      //Sanity FAILURE: Attempt to detach something that was not properly attached?!
        dll_log->Log (L"[ SpecialK ]  ** SANITY CHECK FAILED: DLL was never attached !! **");
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
        // Strip TLS and Mark Freeable
        // ---------------------------
        //
        // Partially clean TLS; a small portion of debug data lingers
        //   per-thread until the DLL's complete process detach.
        //
        __SK_TLS_FreeList->push (
          SK_CleanupTLS ()
        );
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

  static
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

#ifdef _M_IX86

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

#ifndef _M_AMD64
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
  if (wszShort == reinterpret_cast <const wchar_t *> (1))
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

#ifndef _M_AMD64
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

#ifndef _M_AMD64
    wchar_t wszD3D8  [MAX_PATH + 2] = { };
    wchar_t wszDDraw [MAX_PATH + 2] = { };
#endif

    lstrcatW (wszD3D9,   SK_GetHostPath ());
    lstrcatW (wszD3D9,   LR"(\SpecialK.d3d9)");

#ifndef _M_AMD64
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

#ifndef _M_AMD64
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


      WriteULongRelease (
        &__SK_TLS_INDEX,
          FlsAlloc (nullptr)
      );


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
      bool is_steamworks_game =
           SK_Path_wcsstr (wszProcessName, L"SteamApps");

      // Most frequently imported DLLs for games that use SteamAPI
      //
      //  It is trivial to use SteamAPI without linking to the DLL, so
      //    this is not the final test to determine Steam compatibility.
      //
      static sk_import_test_s steam_tests[] =
      {
        { SK_RunLHIfBitness (64, "steam_api64.dll",
                                  "steam_api.dll"),  false },

        {                         "steamnative.dll", false }
      };

      if (! is_steamworks_game)
      {
        SK_TestImports (GetModuleHandle (nullptr), steam_tests, 2);

        is_steamworks_game =
          (steam_tests[0].used | steam_tests[1].used);
      }

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


        ///// That did not work; examine the game's Import Address Table
        /////
        bool gl   = false, vulkan = false, d3d9  = false, d3d11 = false,
             dxgi = false, d3d8   = false, ddraw = false, glide = false;

        SK_TestRenderImports (
          __SK_hModHost,
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

#ifndef _M_AMD64
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

#ifdef _M_AMD64
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
//#ifdef _M_AMD64
//          if (config.apis.dxgi.d3d12.hook)
//            SK_SetDLLRole (DLL_ROLE::DXGI);
//#endif
          else if (config.apis.OpenGL.hook)
            SK_SetDLLRole (DLL_ROLE::OpenGL);
#ifdef _M_AMD64
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

        extern SK_LazyGlobal <sk_config_t> _config;
                          auto& config_ = *_config;
        DBG_UNREFERENCED_LOCAL_VARIABLE (config_);

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
  auto& bootstraps =
    *SK_DLL_GetBootstraps ();

  if (bootstraps.count (role))
  {
    if (! InterlockedCompareExchangeAcquire (&__SK_DLL_Attached, TRUE, FALSE))
    {
      skModuleRegistry::HostApp (GetModuleHandle (nullptr));

      const auto& bootstrap =
                  bootstraps.at (role);

      if ( SK_IsInjected           () &&
           SK_TryLocalWrapperFirst (bootstrap.wrapper_dlls))
      {
        return
          SK_DontInject ();
      }

      if (GetFileAttributesW (L"SpecialK.WaitForDebugger") != INVALID_FILE_ATTRIBUTES)
      {
        while (! SK_IsDebuggerPresent ())
          SK_Sleep (50);
      }

      try {
        if (ReadULongAcquire (&__SK_TLS_INDEX) == TLS_OUT_OF_INDEXES)
        {
          WriteULongRelease (
            &__SK_TLS_INDEX,
              FlsAlloc (nullptr)
          );
        }

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

        void SK_D3D11_InitMutexes (void);
             SK_D3D11_InitMutexes (    );

        void SK_ImGui_Init (void);
             SK_ImGui_Init (    );

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
        dll_log->Log ( L"[ SpecialK  ] Caught an exception during DLL Attach,"
                       L" game may not be stable..." );

        SK_CleanupMutex (&budget_mutex); SK_CleanupMutex (&init_mutex);
        SK_CleanupMutex (&wmi_cs);       SK_CleanupMutex (&steam_mutex);

        SK_CleanupMutex (&steam_callback_cs); SK_CleanupMutex (&steam_popup_cs);
        SK_CleanupMutex (&steam_init_cs);

        void SK_D3D11_CleanupMutexes (void);
             SK_D3D11_CleanupMutexes (    );
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

  void SK_D3D11_CleanupMutexes (void);
       SK_D3D11_CleanupMutexes (    );

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

    auto& bootstraps =
      *SK_DLL_GetBootstraps ();

    if ( bootstraps.count (role) &&
         bootstraps.at    (role).shutdown () )
    {
      return TRUE;
    }
  }

  else {
    dll_log->Log (L"[ SpecialK ]  ** UNCLEAN DLL Process Detach !! **");
  }

  return FALSE;
}



#include <SpecialK/DLL_VERSION.h>

const wchar_t*
__stdcall
SK_GetVersionStrW (void)
{
  return SK_VERSION_STR_W;
}

const char*
__stdcall
SK_GetVersionStrA (void)
{
  return SK_VERSION_STR_A;
}
