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

#include <SpecialK/diagnostics/compatibility.h>
#include <SpecialK/diagnostics/debug_utils.h>

#include <Windows.h>
#include <Shlwapi.h>
#include <process.h>

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
#include <SpecialK/injection/blacklist.h>


// Fix that stupid macro that redirects to Unicode/ANSI
#undef LoadLibrary

// Don't EVER make these function calls from this code unit.
#define LoadLibrary int x = __stdcall;
#define FreeLibrary int x = __stdcall;


// We need this to load embedded resources correctly...
HMODULE hModSelf                       = nullptr;

SK_Thread_HybridSpinlock* init_mutex   = nullptr;
SK_Thread_HybridSpinlock* budget_mutex = nullptr;
SK_Thread_HybridSpinlock* loader_lock  = nullptr;
SK_Thread_HybridSpinlock* wmi_cs       = nullptr;
SK_Thread_HybridSpinlock* cs_dbghelp   = nullptr;

volatile          long __SK_DLL_Ending       = FALSE;
volatile          long __SK_DLL_Attached     = FALSE;
            __time64_t __SK_DLL_AttachTime   = 0ULL;
volatile unsigned long __SK_Threads_Attached = 0UL;
volatile unsigned long __SK_DLL_Refs         = 0UL;
volatile          long __SK_HookContextOwner = false;


class SK_DLL_Bootstrapper
{
  typedef bool (* BootstrapEntryPoint_pfn)(void);
  typedef bool (* BootstrapTerminate_pfn )(void);

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


HMODULE
__stdcall
SK_GetDLL (void)
{
  return reinterpret_cast <HMODULE>       (
    ReadPointerAcquire                    (
      reinterpret_cast <volatile PVOID *> (
            const_cast <HMODULE        *> (&hModSelf))));
}


HMODULE
SK_GetLocalModuleHandle (const wchar_t* wszModule)
{
  wchar_t   wszLocalModulePath [MAX_PATH * 2] = { };
  wcsncpy  (wszLocalModulePath, SK_GetHostPath (), MAX_PATH);
  lstrcatW (wszLocalModulePath, LR"(\)");
  lstrcatW (wszLocalModulePath, wszModule);

  return GetModuleHandleW (wszLocalModulePath);
};

HMODULE
SK_LoadLocalModule (const wchar_t* wszModule)
{
  wchar_t   wszLocalModulePath [MAX_PATH * 2] = { };
  wcsncpy  (wszLocalModulePath, SK_GetHostPath (), MAX_PATH);
  lstrcatW (wszLocalModulePath, LR"(\)");
  lstrcatW (wszLocalModulePath, wszModule);

  return LoadLibraryW (wszLocalModulePath);
};

// If this is the global injector and there is a wrapper version
//   of Special K in the DLL search path, then bail-out!
BOOL
SK_TryLocalWrapperFirst (std::set <std::wstring> dlls)
{
  for ( auto& dll : dlls )
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
  LONG idx_to_free =
    InterlockedCompareExchange ( &__SK_TLS_INDEX,
                                    TLS_OUT_OF_INDEXES,
                                      __SK_TLS_INDEX );

  if (idx_to_free != TLS_OUT_OF_INDEXES)
  {
    TlsFree (idx_to_free);
  }

  SK_SetDLLRole       (DLL_ROLE::INVALID);
  InterlockedExchange (&__SK_DLL_Attached, FALSE);

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

  return auto_boot_viable;
}


bool
__stdcall
SK_EstablishDllRole (HMODULE hModule)
{
  SK_SetDLLRole (DLL_ROLE::INVALID);

  // If Blacklisted, Bail-Out
  wchar_t         wszAppNameLower                   [MAX_PATH + 2] = { };
  wcsncpy        (wszAppNameLower, SK_GetHostApp (), MAX_PATH);
  CharLowerBuffW (wszAppNameLower,                   MAX_PATH);

  if (__blacklist.count (wszAppNameLower)) return false;


  static bool has_dgvoodoo =
    GetFileAttributesA (
      SK_FormatString ( R"(%ws\PlugIns\ThirdParty\dgVoodoo\d3dimm.dll)",
                          std::wstring ( SK_GetDocumentsDir () + LR"(\My Mods\SpecialK)" ).c_str ()
                      ).c_str ()
    ) != INVALID_FILE_ATTRIBUTES;


  wchar_t wszDllFullName [  MAX_PATH + 2 ] = { };

  GetModuleFileName (hModule, wszDllFullName, MAX_PATH );

  const wchar_t* wszShort =
    SK_Path_wcsrchr (wszDllFullName, L'\\') + 1;

  if (wszShort == static_cast <const wchar_t *>(nullptr) + 1)
    wszShort = wszDllFullName;


  if (! SK_Path_wcsicmp (wszShort, L"dinput8.dll"))
  {
    SK_SetDLLRole (DLL_ROLE::DInput8);

    if ( SK_IsDLLSpecialK (L"dxgi.dll")     ||
         SK_IsDLLSpecialK (L"d3d9.dll")     ||
         SK_IsDLLSpecialK (L"d3d11.dll")    ||
         SK_IsDLLSpecialK (L"OpenGL32.dll") ||
         SK_IsDLLSpecialK (L"ddraw.dll")    ||
         SK_IsDLLSpecialK (L"d3d8.dll")        )
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
    SK_IsInjected (true); // SET the injected state

    config.system.central_repository = true;

    bool explicit_inject = false;


    wchar_t wszD3D9  [MAX_PATH + 2] = { };
    wchar_t wszD3D8  [MAX_PATH + 2] = { };
    wchar_t wszDDraw [MAX_PATH + 2] = { };
    wchar_t wszDXGI  [MAX_PATH + 2] = { };
    wchar_t wszD3D11 [MAX_PATH + 2] = { };
    wchar_t wszGL    [MAX_PATH + 2] = { };
    wchar_t wszDI8   [MAX_PATH + 2] = { };

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
      if ( SK_TryLocalWrapperFirst ( local_dlls ) )
      {
        return SK_DontInject ();
      }


      // Most frequently imported DLLs for games that use SteamAPI
      //
      //  It is trivial to use SteamAPI without linking to the DLL, so
      //    this is not the final test to determine Steam compatibility.
      //
      sk_import_test_s steam_tests [] =
      {
        { SK_RunLHIfBitness ( 64, "steam_api64.dll",
                                  "steam_api.dll"    ), false },

        {                         "steamnative.dll",    false }
      };

      SK_TestImports ( GetModuleHandle (nullptr), steam_tests, 2 );


      DWORD   dwProcessSize = MAX_PATH;
      wchar_t wszProcessName [MAX_PATH + 2] = { };

      HANDLE hProc =
        GetCurrentProcess ();

      QueryFullProcessImageName (hProc, 0, wszProcessName, &dwProcessSize);

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

        gl     |= (GetModuleHandle (L"OpenGL32.dll") != nullptr);
        d3d9   |= (GetModuleHandle (L"d3d9.dll")     != nullptr);

        // Not specific enough; some engines will pull in DXGI even if they
        //   do not use D3D10/11/12/D2D/DWrite
        //
        dxgi   |= (GetModuleHandle (L"dxgi.dll")     != nullptr); 

        d3d11  |= (GetModuleHandle (L"d3d11.dll")     != nullptr);
        d3d11  |= (GetModuleHandle (L"d3dx11_43.dll") != nullptr);

#ifndef _WIN64
        d3d8   |= (GetModuleHandle (L"d3d8.dll")     != nullptr);
        ddraw  |= (GetModuleHandle (L"ddraw.dll")    != nullptr);

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
          return SK_DontInject ();
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
          if (config.apis.dxgi.d3d11.hook)
            SK_SetDLLRole (DLL_ROLE::DXGI);
#ifdef _WIN64
          if (config.apis.dxgi.d3d11.hook)
            SK_SetDLLRole (DLL_ROLE::DXGI);
#endif
          else if (config.apis.d3d9.hook  || config.apis.d3d9ex.hook)
            SK_SetDLLRole (DLL_ROLE::D3D9);
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

  return (SK_GetDLLRole () != DLL_ROLE::INVALID);
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


BOOL
__stdcall
SK_Attach (DLL_ROLE role)
{
  if (! InterlockedCompareExchange (&__SK_DLL_Attached, TRUE,        FALSE))
  {
    if (__SK_DLL_Bootstraps.count (role))
    {
      const auto& bootstrap =
        __SK_DLL_Bootstraps.at (role);

      if (SK_IsInjected () && SK_TryLocalWrapperFirst (bootstrap.wrapper_dlls))
      {
        return SK_DontInject ();
      }


      budget_mutex = new SK_Thread_HybridSpinlock (  400);
      init_mutex   = new SK_Thread_HybridSpinlock ( 5000);
      loader_lock  = new SK_Thread_HybridSpinlock ( 6536);
      wmi_cs       = new SK_Thread_HybridSpinlock (  128);
      cs_dbghelp   = new SK_Thread_HybridSpinlock (65536);


      _time64 (&__SK_DLL_AttachTime);

      InterlockedCompareExchange (
        &__SK_DLL_Attached,
          bootstrap.start (),
            TRUE );

      if (ReadAcquire (&__SK_DLL_Attached))
      {
        return TRUE;
      }


      SK_CleanupMutex (&budget_mutex); SK_CleanupMutex (&init_mutex);
      SK_CleanupMutex (&loader_lock);  SK_CleanupMutex (&cs_dbghelp);
      SK_CleanupMutex (&wmi_cs);
    }
  }


  return SK_DontInject ();
}



BOOL
__stdcall
SK_Detach (DLL_ROLE role)
{
  ULONG local_refs =
    InterlockedDecrement (&__SK_DLL_Refs);

  if ( local_refs == 0 &&
         InterlockedCompareExchange (
                    &__SK_DLL_Attached,
                      FALSE,
                        TRUE        )
     )
  {
    if ( __SK_DLL_Bootstraps.count (role) &&
         __SK_DLL_Bootstraps.at    (role).shutdown () )
    {
      SK_CleanupMutex (&budget_mutex); SK_CleanupMutex (&init_mutex);
      SK_CleanupMutex (&loader_lock);  SK_CleanupMutex (&cs_dbghelp);
      SK_CleanupMutex (&wmi_cs);

      return TRUE;
    }
  }

  else {
    dll_log.Log (L"[ SpecialK ]  ** UNCLEAN DLL Process Detach !! **");
  }

  return FALSE;
}



void SK_AllocTLS (void)
{
  InterlockedCompareExchange (&__SK_TLS_INDEX, TlsAlloc (), TLS_OUT_OF_INDEXES);

  if ( TLS_OUT_OF_INDEXES !=
         ReadAcquire (&__SK_TLS_INDEX) )
  {
    auto lpvData =
      static_cast <LPVOID> (
        LocalAlloc ( LPTR, sizeof (SK_TLS) * SK_TLS::stack::max )
      );

    if (lpvData != nullptr)
    {
      if (! TlsSetValue (ReadAcquire (&__SK_TLS_INDEX), lpvData))
      {
        LocalFree (lpvData);
      }

      else
      {
        *(SK_TLS *)lpvData = SK_TLS::SK_TLS ();
        (static_cast <SK_TLS *> (lpvData))->stack.current = 0;
      }
    }
  }
}

void SK_CleanupTLS (void)
{
  if (ReadAcquire (&__SK_TLS_INDEX) != TLS_OUT_OF_INDEXES)
  {
    auto lpvData =
      static_cast <LPVOID> (TlsGetValue (__SK_TLS_INDEX));

    if (lpvData != nullptr)
    {
#ifdef _DEBUG
      size_t freed =
#endif
      SK_TLS_Bottom ()->Cleanup (SK_TLS_CleanupReason_e::Unload);

#ifdef _DEBUG
      SK_LOG0 ( ( L"Freed %zu bytes of temporary heap storage for tid=%x",
                    freed, GetCurrentThreadId () ), L"TLS Memory" );
#endif

      LocalFree   (lpvData);
      TlsSetValue (ReadAcquire (&__SK_TLS_INDEX), nullptr);
    }
  }
}

BOOL
APIENTRY
DllMain ( HMODULE hModule,
          DWORD   ul_reason_for_call,
          LPVOID  lpReserved )
{
  UNREFERENCED_PARAMETER (lpReserved);

  auto EarlyOut =
  [&](BOOL bRet = TRUE)
  {
    if (ReadAcquire (&__SK_TLS_INDEX) != TLS_OUT_OF_INDEXES)
      SK_TLS_Bottom ()->debug.in_DllMain = false;

    SK_CleanupTLS ();

    return bRet;
  };


  switch (ul_reason_for_call)
  {
    case DLL_PROCESS_ATTACH:
    {
      SK_AllocTLS ();

      if (ReadAcquire (&__SK_TLS_INDEX) != TLS_OUT_OF_INDEXES)
        SK_TLS_Bottom ()->debug.in_DllMain = true;

      if (InterlockedExchangePointer (
            reinterpret_cast <LPVOID  *> (
                  const_cast <HMODULE *> (&hModSelf)
                                         ),
            hModule
           )
         )
      {
        return EarlyOut (FALSE);
      }

      // We use SKIM for injection and rundll32 for various tricks involving restarting
      //   the currently running game; neither needs or even wants this DLL fully
      //     initialized!
      if (SK_HostApp.isInjectionTool ())
      {
        SK_EstablishRootPath ();

        return EarlyOut (TRUE);
      }


      InterlockedIncrement (&__SK_DLL_Refs);

      // We reserve the right to deny attaching the DLL, this will generally
      //   happen if a game does not opt-in to system wide injection.
      if (! SK_EstablishDllRole (hModule))              return EarlyOut (TRUE);

      // We don't want to initialize the DLL, but we also don't want it to
      //   re-inject itself constantly; just return TRUE here.
      else if (SK_GetDLLRole () == DLL_ROLE::INVALID)   return EarlyOut (TRUE);

      // Setup unhooked function pointers
      SK_PreInitLoadLibrary ();

      if (! SK_Attach (SK_GetDLLRole ()))               return EarlyOut (TRUE);

      // If we got this far, it's because this is an injection target
      //
      //   Must hold a reference to this DLL so that removing the CBT hook does
      //     not crash the game.
      if (SK_IsInjected ())
      {
        SK_Inject_AcquireProcess ();
      }

      // We're done with DllMain, this thread is no longer special...
      if (ReadAcquire (&__SK_TLS_INDEX) != TLS_OUT_OF_INDEXES)
        SK_TLS_Bottom ()->debug.in_DllMain = false;

      return TRUE;
    } break;


    case DLL_PROCESS_DETACH:
    {
      SK_Inject_ReleaseProcess ();

      if (! InterlockedCompareExchange (&__SK_DLL_Ending, TRUE, FALSE))
      {
        // If the DLL being unloaded is the source of a CBT hook, then
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
        SK_Detach (SK_GetDLLRole ());

      LONG idx_to_free =
        InterlockedCompareExchange ( &__SK_TLS_INDEX,
                                        TLS_OUT_OF_INDEXES,
                                          __SK_TLS_INDEX );

      if (idx_to_free != TLS_OUT_OF_INDEXES)
      {
        TlsFree (idx_to_free);
      }

#ifdef _DEBUG
      else {
      //Sanity FAILURE: Attempt to detach something that was not properly attached?!
        dll_log.Log (L"[ SpecialK ]  ** SANITY CHECK FAILED: DLL was never attached !! **");
      }
#endif

      return TRUE;
    } break;



    case DLL_THREAD_ATTACH:
    {
      InterlockedIncrement (&__SK_Threads_Attached);

      SK_AllocTLS ();
    }
    break;


    case DLL_THREAD_DETACH:
    {
      SK_CleanupTLS ();
    }
    break;
  }

  return TRUE;
}