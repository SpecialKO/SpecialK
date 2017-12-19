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

#include <SpecialK/diagnostics/compatibility.h>

#include <Windows.h>

#include <Shlwapi.h>
#include <process.h>

#include <ctime>

#include <SpecialK/core.h>
#include <SpecialK/config.h>
#include <SpecialK/diagnostics/debug_utils.h>
#include <SpecialK/dxgi_backend.h>
#include <SpecialK/d3d9_backend.h>
#include <SpecialK/input/dinput8_backend.h>

#ifndef _WIN64
#include <SpecialK/d3d8_backend.h>
#include <SpecialK/ddraw_backend.h>
#endif

#include <SpecialK/opengl_backend.h>
#include <SpecialK/log.h>
#include <SpecialK/utility.h>

#include <SpecialK/hooks.h>
#include <SpecialK/injection/injection.h>

#include <SpecialK/tls.h>
#include <SpecialK/framerate.h>


// Fix that stupid macro that redirects to Unicode/ANSI
#undef LoadLibrary

// Don't EVER make these function calls from this code unit.
#define LoadLibrary int x = __stdcall;
#define FreeLibrary int x = __stdcall;


// We need this to load embedded resources correctly...
//std::atomic <HMODULE> hModSelf            = nullptr;
volatile HMODULE hModSelf                       = nullptr;

SK_Thread_HybridSpinlock* init_mutex   = nullptr;
SK_Thread_HybridSpinlock* budget_mutex = nullptr;
SK_Thread_HybridSpinlock* loader_lock  = nullptr;
SK_Thread_HybridSpinlock* wmi_cs       = nullptr;
SK_Thread_HybridSpinlock* cs_dbghelp   = nullptr;

volatile LONG  __SK_DLL_Ending       = FALSE;
volatile LONG  __SK_DLL_Attached     = FALSE;
    __time64_t __SK_DLL_AttachTime   = 0ULL;
volatile ULONG __SK_Threads_Attached = 0UL;
volatile ULONG __SK_DLL_Refs         = 0UL;
volatile DWORD __SK_TLS_INDEX        = MAXDWORD;
volatile LONG  __SK_HookContextOwner = false;


SK_TLS*
__stdcall
SK_TLS_Bottom (void)
{
  if (__SK_TLS_INDEX == MAXDWORD)
    return nullptr;

  LPVOID lpvData =
    TlsGetValue (__SK_TLS_INDEX);

  if (lpvData == nullptr)
  {
    lpvData =
      static_cast <LPVOID> (
        LocalAlloc ( LPTR,
                       sizeof (SK_TLS) * SK_TLS::stack::max )
      );

    if (lpvData != nullptr)
    {
      if (! TlsSetValue (__SK_TLS_INDEX, lpvData))
      {
        LocalFree (lpvData);
        return nullptr;
      }

      static_cast <SK_TLS *> (lpvData)->stack.current = 0;
    }
  }

  return static_cast <SK_TLS *> (lpvData);
}

SK_TLS*
__stdcall
SK_TLS_Top (void)
{
  if (__SK_TLS_INDEX == MAXDWORD)
    return nullptr;

  return &(SK_TLS_Bottom ()[SK_TLS_Bottom ()->stack.current]);
}

bool
__stdcall
SK_TLS_Push (void)
{
  if (__SK_TLS_INDEX == MAXDWORD)
    return false;

  if (SK_TLS_Bottom ()->stack.current < SK_TLS::stack::max)
  {
    static_cast <SK_TLS *>   (SK_TLS_Bottom ())[SK_TLS_Bottom ()->stack.current + 1] =
      static_cast <SK_TLS *> (SK_TLS_Bottom ())[SK_TLS_Bottom ()->stack.current++];

    return true;
  }

  // Overflow
  return false;
}

bool
__stdcall
SK_TLS_Pop  (void)
{
  if (__SK_TLS_INDEX == MAXDWORD)
    return false;

  if (SK_TLS_Bottom ()->stack.current > 0)
  {
    static_cast <SK_TLS *> (SK_TLS_Bottom ())->stack.current--;

    return true;
  }

  // Underflow
  return false;
}


#include <SpecialK/injection/address_cache.h>

bool
__stdcall
SK_IsInjected (bool set)
{
// Indicates that the DLL is injected purely as a hooker, rather than
//   as a wrapper DLL.
  static std::atomic_bool __injected = false;

  if (__injected == true)
    return true;

  if (set)
  {
    __injected               = true;
    SK_Inject_AddressManager = new SK_Inject_AddressCacheRegistry ();
  }

  return set;
}

HMODULE
__stdcall
SK_GetDLL (void)
{
  return reinterpret_cast <HMODULE> (ReadPointerAcquire ((const volatile LPVOID*)&hModSelf));
}

#include <unordered_set>


extern void
__stdcall
SK_EstablishRootPath (void);

#pragma data_seg (".SK_Hooks")
#pragma data_seg (".SK_Hooks")
const std::unordered_set <std::wstring> blacklist = {
L"steam.exe",
L"GameOverlayUI.exe",
L"streaming_client.exe",
L"steamerrorreporter.exe",
L"steamerrorreporter64.exe",
L"steamservice.exe",
L"steam_monitor.exe",
L"steamwebhelper.exe",
L"html5app_steam.exe",
L"wow_helper.exe",
L"uninstall.exe",

L"WriteMiniDump.exe",
L"CrashReporter.exe",
L"SupportTool.exe",
L"CrashSender1400.exe",
L"WerFault.exe",

L"DXSETUP.exe",
L"setup.exe",
L"vc_redist.x64.exe",
L"vc_redist.x86.exe",
L"vc2010redist_x64.exe",
L"vc2010redist_x86.exe",
L"vcredist_x64.exe",
L"vcredist_x86.exe",
L"NDP451-KB2872776-x86-x64-AllOS-ENU.exe",
L"dotnetfx35.exe",
L"DotNetFx35Client.exe",
L"dotNetFx40_Full_x86_x64.exe",
L"dotNetFx40_Client_x86_x64.exe",
L"oalinst.exe",
L"EasyAntiCheat_Setup.exe",
L"UplayInstaller.exe",


L"x64launcher.exe",
L"x86launcher.exe",
L"Launcher.exe",
L"FFX&X-2_LAUNCHER.exe",
L"Fallout4Launcher.exe",
L"SkyrimSELauncher.exe",
L"ModLauncher.exe",
L"AkibaUU_Config.exe",
L"Obduction.exe",
L"Grandia2Launcher.exe",
L"FFXiii2Launcher.exe",
L"Bethesda.net_Launcher.exe",
L"UbisoftGameLauncher.exe",
L"UbisoftGameLauncher64.exe",
L"SplashScreen.exe",
L"GameLauncherCefChildProcess.exe",
L"LaunchPad.exe",
L"CNNLauncher.exe",
L"FF9_Launcher.exe",
L"A17Config.exe",
L"A18Config.exe", // Atelier Firis
L"DPLauncher.exe",
L"ZeroEscape-Launcher.exe",


L"ActivationUI.exe",
L"zosSteamStarter.exe",
L"notepad.exe",
L"mspaint.exe",
L"7zFM.exe",
L"WinRar.exe",
L"EAC.exe",
L"vcpkgsrv.exe",
L"dllhost.exe",
L"git.exe",
L"link.exe",
L"cl.exe",
L"rc.exe",
L"conhost.exe",
L"GameBarPresenceWriter.exe",
L"OAWrapper.exe",
L"NvOAWrapperCache.exe",
L"waifu2x-caffe.exe",
L"waifu2x-caffe-cui.exe",

L"GameServer.exe",// Sacred   Game Server
L"s2gs.exe",      // Sacred 2 Game Server

L"sihost.exe",
L"Chrome.exe",
L"explorer.exe",
L"browser_broker.exe",
L"dwm.exe",
L"LaunchTM.exe",


L"SleepOnLan.exe",
L"ds3t.exe",
L"tzt.exe"
};
#pragma data_seg ()
#pragma comment  (linker, "/section:.SK_Hooks,RWS")

bool
__stdcall
SK_EstablishDllRole (HMODULE hModule)
{
  SK_SetDLLRole (DLL_ROLE::INVALID);

  // If Blacklisted, Bail-Out
  if (blacklist.count (std::wstring (SK_GetHostApp ())))
  {
    SK_SetDLLRole (DLL_ROLE::INVALID);

    return false;
  }


  static bool has_dgvoodoo =
    GetFileAttributesA (
      SK_FormatString ( R"(%ws\PlugIns\ThirdParty\dgVoodoo\d3dimm.dll)",
                          std::wstring ( SK_GetDocumentsDir () + L"\\My Mods\\SpecialK" ).c_str ()
                      ).c_str ()
    ) != INVALID_FILE_ATTRIBUTES;


  wchar_t wszDllFullName [  MAX_PATH + 2 ] = { };

  GetModuleFileName (hModule, wszDllFullName, MAX_PATH );

  const wchar_t* wszShort =
    SK_Path_wcsrchr (wszDllFullName, L'\\') + 1;

  if (wszShort == static_cast <const wchar_t *>(nullptr) + 1)
    wszShort = wszDllFullName;


  if (! SK_Path_wcsicmp (wszShort, L"dxgi.dll"))
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

  else if (! SK_Path_wcsicmp (wszShort, L"dinput8.dll"))
    SK_SetDLLRole (DLL_ROLE::DInput8);


  //
  // This is an injected DLL, not a wrapper DLL...
  //
  else if ( SK_Path_wcsstr (wszShort, L"SpecialK") )
  {
    /////// Skip lengthy tests if we already have the wrapper version loaded.
    if ( ( SK_IsDLLSpecialK (L"d3d9.dll") )     || 
         ( SK_IsDLLSpecialK (L"dxgi.dll") )     ||
         ( SK_IsDLLSpecialK (L"d3d11.dll") )    ||
         ( SK_IsDLLSpecialK (L"OpenGL32.dll") ) ||
         ( SK_IsDLLSpecialK (L"d3d8.dll") )     ||
         ( SK_IsDLLSpecialK (L"ddraw.dll") )    ||
         ( SK_IsDLLSpecialK (L"dinput8.dll") ) )
    {
      SK_SetDLLRole (DLL_ROLE::INVALID);
      return true;
    }

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

    lstrcatW (wszD3D9, SK_GetHostPath ());
    lstrcatW (wszD3D9, L"\\SpecialK.d3d9");

#ifndef _WIN64
    lstrcatW (wszD3D8, SK_GetHostPath ());
    lstrcatW (wszD3D8, L"\\SpecialK.d3d8");

    lstrcatW (wszDDraw, SK_GetHostPath ());
    lstrcatW (wszDDraw, L"\\SpecialK.ddraw");
#endif

    lstrcatW (wszDXGI, SK_GetHostPath ());
    lstrcatW (wszDXGI, L"\\SpecialK.dxgi");

    lstrcatW (wszD3D11, SK_GetHostPath ());
    lstrcatW (wszD3D11, L"\\SpecialK.d3d11");

    lstrcatW (wszGL,   SK_GetHostPath ());
    lstrcatW (wszGL,   L"\\SpecialK.OpenGL32");

    lstrcatW (wszDI8,  SK_GetHostPath ());
    lstrcatW (wszDI8,  L"\\SpecialK.DInput8");


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
    if (! explicit_inject)
    {
      SK_EstablishRootPath ();
      SK_LoadConfigEx      (L"SpecialK", false);


      sk_import_test_s steam_tests [] = {
#ifdef _WIN64
           { "steam_api64.dll", false },
#else
           { "steam_api.dll",   false },
#endif
           { "steamnative.dll", false }
      };

      SK_TestImports ( GetModuleHandle (nullptr), steam_tests, 2 );


      DWORD   dwProcessSize = MAX_PATH;
      wchar_t wszProcessName [MAX_PATH + 2] = { };

      HANDLE hProc =
        GetCurrentProcess ();

      QueryFullProcessImageName (hProc, 0, wszProcessName, &dwProcessSize);

      const bool is_steamworks_game =
        ( steam_tests [0].used | steam_tests [1].used ) ||
           SK_Path_wcsstr (wszProcessName, L"steamapps");


      bool
      SK_Inject_TestUserWhitelist (const wchar_t* wszExecutable);


      // If this is a Steamworks game, then let's figure out the graphics API dynamically
      if (is_steamworks_game || SK_Inject_TestUserWhitelist (SK_GetFullyQualifiedApp ()))
      {
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

        //
        // Not specific enough; some engines will pull in DXGI even if they
        //   do not use D3D10/11/12/D2D/DWrite
        //
        //dxgi   |= (GetModuleHandle (L"dxgi.dll")     != nullptr); 

        d3d11  |= (GetModuleHandle (L"d3d11.dll")     != nullptr);
        d3d11  |= (GetModuleHandle (L"d3dx11_43.dll") != nullptr);

#ifndef _WIN64
        d3d8   |= (GetModuleHandle (L"d3d8.dll")     != nullptr);
        ddraw  |= (GetModuleHandle (L"ddraw.dll")    != nullptr);

        if (config.apis.d3d8.hook && d3d8 && has_dgvoodoo)
        {
          SK_SetDLLRole (DLL_ROLE::D3D8);

          if (SK_IsDLLSpecialK (L"d3d8.dll"))
            return FALSE;
        }

        else
#endif

        if (config.apis.dxgi.d3d11.hook && (dxgi || d3d11)) 
        {
          if (d3d11)
          {
            SK_SetDLLRole ( static_cast <DLL_ROLE> ( (int)DLL_ROLE::DXGI |
                                                     (int)DLL_ROLE::D3D11 ) );
          }

          else
            SK_SetDLLRole (DLL_ROLE::DXGI);


          if (SK_IsDLLSpecialK (L"dxgi.dll"))
            return FALSE;

          if (SK_IsDLLSpecialK (L"d3d11.dll"))
            return FALSE;
        }

        else if (config.apis.d3d9.hook && d3d9)
        {
          SK_SetDLLRole (DLL_ROLE::D3D9);

          if (SK_IsDLLSpecialK (L"d3d9.dll"))
            return FALSE;
        }

        else if (config.apis.OpenGL.hook && gl)
        {
          SK_SetDLLRole (DLL_ROLE::OpenGL);

          if (SK_IsDLLSpecialK (L"OpenGL32.dll"))
            return FALSE;
        }

#ifdef _WIN64
        else if (config.apis.Vulkan.hook && vulkan)
          SK_SetDLLRole (DLL_ROLE::Vulkan);

#else

        else if (config.apis.ddraw.hook && ddraw && has_dgvoodoo)
        {
          SK_SetDLLRole (DLL_ROLE::DDraw);
        
          if (SK_IsDLLSpecialK (L"ddraw.dll"))
            return FALSE;
        }
#endif


        // No Freaking Clue What API This is, Let's use the config file to
        //   filter out any APIs the user knows are not valid.
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

        // This time, save the config file
        SK_LoadConfig (L"SpecialK");

        return true;
      }
    }
  }

  if (SK_GetDLLRole () != DLL_ROLE::INVALID)
    return true;

  return false;
}

BOOL
__stdcall
SK_Attach (DLL_ROLE role)
{
  auto DontInject = [] (void) ->
    BOOL
      {
        SK_SetDLLRole (DLL_ROLE::INVALID);
        InterlockedExchange (&__SK_DLL_Attached, 0);
        return FALSE;
      };


  if (! InterlockedCompareExchange (&__SK_DLL_Attached, 1, 0))
  {
    budget_mutex = new SK_Thread_HybridSpinlock ( 400);
    init_mutex   = new SK_Thread_HybridSpinlock (5000);
    loader_lock  = new SK_Thread_HybridSpinlock (6536);
    wmi_cs       = new SK_Thread_HybridSpinlock ( 128);
    cs_dbghelp   = new SK_Thread_HybridSpinlock (104857);

    __SK_TLS_INDEX = TlsAlloc ();

    if (__SK_TLS_INDEX == TLS_OUT_OF_INDEXES)
    {
#if 0
      MessageBox ( NULL,
                     L"Out of TLS Indexes",
                       L"Cannot Init. Special K",
                         MB_ICONERROR | MB_OK |
                         MB_APPLMODAL | MB_SETFOREGROUND );
#endif
    }

    else
    {
      _time64 (&__SK_DLL_AttachTime);

      switch (role)
      {
        case DLL_ROLE::DXGI:
        case DLL_ROLE::D3D11_CASE:
        {
          // If this is the global injector and there is a wrapper version
          //   of Special K in the DLL search path, then bail-out!
          if (SK_IsInjected () && ( SK_IsDLLSpecialK (L"dxgi.dll") ||
                                    SK_IsDLLSpecialK (L"d3d11.dll") ))
          {
            return DontInject ();
          }

          InterlockedCompareExchange (
            &__SK_DLL_Attached,
              SK::DXGI::Startup (),
                1
          );
        } break;

        case DLL_ROLE::D3D9:
        {
          // If this is the global injector and there is a wrapper version
          //   of Special K in the DLL search path, then bail-out!
          if (SK_IsInjected () && SK_IsDLLSpecialK (L"d3d9.dll"))
          {
            return DontInject ();
          }

          InterlockedCompareExchange (
            &__SK_DLL_Attached,
              SK::D3D9::Startup (),
                1
          );
        } break;

#ifndef _WIN64
        case DLL_ROLE::D3D8:
        {
          // If this is the global injector and there is a wrapper version
          //   of Special K in the DLL search path, then bail-out!
          if (SK_IsInjected () && SK_IsDLLSpecialK (L"d3d8.dll"))
          {
            return DontInject ();
          }

          InterlockedCompareExchange (
            &__SK_DLL_Attached,
              SK::D3D8::Startup (),
                1
          );
        } break;

        case DLL_ROLE::DDraw:
        {
          // If this is the global injector and there is a wrapper version
          //   of Special K in the DLL search path, then bail-out!
          if (SK_IsInjected () && SK_IsDLLSpecialK (L"ddraw.dll"))
          {
            return DontInject ();
          }

          InterlockedCompareExchange (
            &__SK_DLL_Attached,
              SK::DDraw::Startup (),
                1
          );
        } break;
#endif

        case DLL_ROLE::OpenGL:
        {
          // If this is the global injector and there is a wrapper version
          //   of Special K in the DLL search path, then bail-out!
          if (SK_IsInjected () && SK_IsDLLSpecialK (L"OpenGL32.dll"))
          {
            return DontInject ();
          }

          InterlockedCompareExchange (
            &__SK_DLL_Attached,
              SK::OpenGL::Startup (),
                1
          );
        } break;

        case DLL_ROLE::Vulkan:
        {
          return DontInject ();
        } break;

        case DLL_ROLE::DInput8:
        {
          // If this is the global injector and there is a wrapper version
          //   of Special K in the DLL search path, then bail-out!
          if (SK_IsInjected () && SK_IsDLLSpecialK (L"dinput8.dll"))
          {
            return DontInject ();
          }

          InterlockedCompareExchange (
            &__SK_DLL_Attached,
              SK::DI8::Startup (),
                1
          );
        } break;
      }

      return true;
      //return
      //  InterlockedExchangeAddRelease (
      //    &__SK_DLL_Attached,
      //      1
      //  );
    }
  }

  return DontInject ();
}

BOOL
__stdcall
SK_Detach (DLL_ROLE role)
{
  BOOL  ret        = FALSE;
  ULONG local_refs = InterlockedDecrement (&__SK_DLL_Refs);

  if ( local_refs == 0 &&
         InterlockedCompareExchangeRelease (
                    &__SK_DLL_Attached,
                      FALSE,
                        TRUE
         )
     )
  {
    switch (role)
    {
      case DLL_ROLE::DXGI:
      case DLL_ROLE::D3D11_CASE:
        ret = SK::DXGI::Shutdown ();
        break;

      case DLL_ROLE::D3D9:
        ret = SK::D3D9::Shutdown ();
        break;

#ifndef _WIN64
      case DLL_ROLE::D3D8:
        ret = SK::D3D8::Shutdown ();
        break;

      case DLL_ROLE::DDraw:
        ret = SK::DDraw::Shutdown ();
        break;
#else
#endif

      case DLL_ROLE::OpenGL:
        ret = SK::OpenGL::Shutdown ();
        break;

      case DLL_ROLE::DInput8:
        ret = SK::DI8::Shutdown ();
        break;
    }

    delete budget_mutex;
    delete loader_lock;
    delete init_mutex;
    delete cs_dbghelp;
    delete wmi_cs;
  }

  else {
    dll_log.Log (L"[ SpecialK ]  ** UNCLEAN DLL Process Detach !! **");
  }

  return ret;
}




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
      // Sanity Check:
      // -------------
      //
      //  Bail-out if by some fluke this DLL is attached twice
      //    (usually caused by a stack overflow in a third-party injector).
      //
      //    It is impossible to report this error, because that would bring
      //      in dependencies on DLLs not yet loaded. Fortunately if the DLL
      //        is of the wrapper variety, Windows will complain with error:
      //
      //          0xc0000142
      //
      if (InterlockedCompareExchangePointer ((LPVOID *)&hModSelf, hModule, nullptr))
      {
        return FALSE;
      }

      // We use SKIM for injection and rundll32 for various tricks involving restarting
      //   the currently running game; neither needs or even wants this DLL fully
      //     initialized!
      if (SK_HostApp.isInjectionTool ())
      {
        SK_EstablishRootPath ();
        return TRUE;
      }

      if ( ReadAcquire (&__SK_DLL_Attached) ||
           ReadAcquire (&__SK_DLL_Ending)      )
      {
        SK_EstablishRootPath ();
        return TRUE;
      }


      //++__SK_DLL_Refs;
      InterlockedIncrement (&__SK_DLL_Refs);


      // Setup unhooked function pointers
      SK_PreInitLoadLibrary ();



      // We reserve the right to deny attaching the DLL, this will generally
      //   happen if a game does not opt-in to system wide injection.
      if (! SK_EstablishDllRole (hModule))
      {
        return TRUE;
      }

      // We don't want to initialize the DLL, but we also don't want it to
      //   re-inject itself constantly; just return TRUE here.
      else if (SK_GetDLLRole () == DLL_ROLE::INVALID)
      {
        return TRUE;
      }

      QueryPerformanceCounter_Original =
        reinterpret_cast <QueryPerformanceCounter_pfn> (
          GetProcAddress (
            GetModuleHandle ( L"kernel32.dll"),
                                "QueryPerformanceCounter" )
        );

      SK_Init_MinHook        ();
      SK_InitCompatBlacklist ();

      BOOL bRet = SK_Attach (SK_GetDLLRole ());

      if (! bRet)
      {
        //blacklist.emplace (std::wstring (SK_GetHostApp ()));
        return TRUE;
      }

      if (SK_GetDLLRole () == DLL_ROLE::INVALID)
      {
        return TRUE;
      }


      // If we got this far, it's because this is an injection target
      //
      //   Must hold a reference to this DLL so that removing the CBT hook does
      //     not crash the game.
      if (SK_IsInjected ())
      {
        SK_Inject_AcquireProcess ();
      }

      return bRet;
    } break;


    case DLL_PROCESS_DETACH:
    {
      SK_Inject_ReleaseProcess ();

      if (! InterlockedCompareExchange (&__SK_DLL_Ending, 1, 0))
      {
        // If the DLL being unloaded is the source of a CBT hook, then
        //   shut that down before detaching the DLL.
        if (ReadAcquire (&__SK_HookContextOwner))
        {
          SKX_RemoveCBTHook ();

          // If SKX_RemoveCBTHook (...) is successful: (__SK_HookContextOwner = 0)
          if (! ReadAcquire (&__SK_HookContextOwner))
          {
            SK_RunIf64Bit (DeleteFileW (L"SpecialK64.pid"));
            SK_RunIf32Bit (DeleteFileW (L"SpecialK32.pid"));
          }
        }

        if (ReadAcquire (&__SK_DLL_Attached))
        {
          InterlockedExchange (&__SK_DLL_Ending, TRUE);
        }
      }

      if (ReadAcquire (&__SK_DLL_Attached))
        SK_Detach (SK_GetDLLRole ());

      if (__SK_TLS_INDEX != MAXDWORD)
      {
        TlsFree (__SK_TLS_INDEX);
        __SK_TLS_INDEX = MAXDWORD;
      }

      //else {
        //Sanity FAILURE: Attempt to detach something that was not properly attached?!
        //dll_log.Log (L"[ SpecialK ]  ** SANITY CHECK FAILED: DLL was never attached !! **");
      //}

      return TRUE;
    } break;



    case DLL_THREAD_ATTACH:
    { 
      InterlockedIncrement (&__SK_Threads_Attached);

      if (__SK_TLS_INDEX != MAXDWORD)
      {
        auto lpvData =
          static_cast <LPVOID> (
            LocalAlloc ( LPTR, sizeof (SK_TLS) * SK_TLS::stack::max )
          );

        if (lpvData != nullptr)
        {
          if (! TlsSetValue (__SK_TLS_INDEX, lpvData))
          {
            LocalFree (lpvData);
          }

          else
            (static_cast <SK_TLS *> (lpvData))->stack.current = 0;
        }
      }
    } break;


    case DLL_THREAD_DETACH:
    {
      if (__SK_TLS_INDEX != MAXDWORD)
      {
        auto lpvData =
          static_cast <LPVOID> (TlsGetValue (__SK_TLS_INDEX));

        if (lpvData != nullptr)
        {
          if (SK_TLS_Bottom ()->known_modules.pResolved != nullptr)
          {
            delete SK_TLS_Bottom ()->known_modules.pResolved;
                   SK_TLS_Bottom ()->known_modules.pResolved = nullptr;
          }

          LocalFree   (lpvData);
          TlsSetValue (__SK_TLS_INDEX, nullptr);
        }
      }
    } break;
  }

  return TRUE;
}





#include <unordered_map>



SK_ModuleAddrMap::SK_ModuleAddrMap (void) = default;

bool
SK_ModuleAddrMap::contains (LPVOID pAddr, HMODULE* phMod)
{
  if (pResolved == nullptr)
      pResolved = new std::unordered_map <LPVOID, HMODULE> ();

  std::unordered_map <LPVOID, HMODULE> *pResolved_ =
    ((std::unordered_map <LPVOID, HMODULE> *)pResolved);

  const auto&& it =
    pResolved_->find (pAddr);

  if (it != pResolved_->cend ())
  {
    *phMod = (*pResolved_) [pAddr];
    return true;
  }

  return false;
}

void 
SK_ModuleAddrMap::insert (LPVOID pAddr, HMODULE hMod)
{
  if (pResolved == nullptr)
    pResolved = new std::unordered_map <LPVOID, HMODULE> ( );

  std::unordered_map <LPVOID, HMODULE> *pResolved_ = 
    ((std::unordered_map <LPVOID, HMODULE> *)pResolved);

  (*pResolved_) [pAddr] = hMod;
}