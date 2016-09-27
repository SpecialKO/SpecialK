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

#include "core.h"
#include "dxgi_backend.h"
#include "d3d9_backend.h"
#include "opengl_backend.h"
#include "log.h"
#include "utility.h"

#include "tls.h"

volatile DWORD __SK_TLS_INDEX = -1;

SK_TLS*
__stdcall
SK_GetTLS (void)
{
  LPVOID lpvData = TlsGetValue (__SK_TLS_INDEX);

  if (lpvData == nullptr) {
    lpvData = (LPVOID) LocalAlloc (LPTR, sizeof SK_TLS);

    if (! TlsSetValue (__SK_TLS_INDEX, lpvData)) {
      LocalFree (lpvData);
      return nullptr;
    }
  }

  return (SK_TLS *)lpvData;
}

#include <Shlwapi.h>
#include <process.h>

extern "C" BOOL WINAPI _CRT_INIT (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved);

// Indicates that the DLL is injected purely as a hooker, rather than
//   as a wrapper DLL.
bool injected = false;

bool
__stdcall
SK_IsInjected (void)
{
  return injected;
}

// We need this to load embedded resources correctly...
volatile HMODULE hModSelf = 0;

HMODULE
__stdcall
SK_GetDLL (void)
{
  return hModSelf;
}


HANDLE hThreadAttach = 0;

bool
__stdcall
SK_EstablishDllRole (HMODULE hModule)
{
  wchar_t wszDllFullName [MAX_PATH];
          wszDllFullName [MAX_PATH - 1] = L'\0';

  GetModuleFileName (hModule, wszDllFullName, MAX_PATH - 1);

  const wchar_t* wszShort = wcsrchr (wszDllFullName, L'\\') + 1;

  if (wszShort == (const wchar_t *)1)
    wszShort = wszDllFullName;

  if (! lstrcmpiW (wszShort, L"dxgi.dll"))
    SK_SetDLLRole (DLL_ROLE::DXGI);

  else if (! lstrcmpiW (wszShort, L"d3d9.dll"))
    SK_SetDLLRole (DLL_ROLE::D3D9);

  else if (! lstrcmpiW (wszShort, L"OpenGL32.dll"))
    SK_SetDLLRole (DLL_ROLE::OpenGL);


  //
  // This is an injected DLL, not a wrapper DLL...
  //
  else if ( wcsstr (wszShort, L"SpecialK") )
  {
    bool explicit_inject = false;

    if      ( GetFileAttributesW (L"SpecialK.d3d9") != INVALID_FILE_ATTRIBUTES ) {
      SK_SetDLLRole (DLL_ROLE::D3D9);
      explicit_inject = true;
    }

    else if ( GetFileAttributesW (L"SpecialK.dxgi") != INVALID_FILE_ATTRIBUTES ) {
      SK_SetDLLRole (DLL_ROLE::DXGI);
      explicit_inject = true;
    }

    else if ( GetFileAttributesW (L"SpecialK.OpenGL32") != INVALID_FILE_ATTRIBUTES ) {
      SK_SetDLLRole (DLL_ROLE::OpenGL);
      explicit_inject = true;
    }

    // Opted out of explicit injection, now try automatic
    if (! explicit_inject) {

#ifdef _WIN64
      sk_import_test_s steam_tests [] = { { "steam_api64.dll", false },
                                          { "steamnative.dll", false },
                                          { "CSteamworks.dll", false } };
#else
      sk_import_test_s steam_tests [] = { { "steam_api.dll",   false },
                                          { "steamnative.dll", false },
                                          { "CSteamworks.dll", false } };
#endif

      SK_TestImports ( GetModuleHandle (nullptr), steam_tests, 3 );

      DWORD   dwProcessSize = MAX_PATH;
      wchar_t wszProcessName [MAX_PATH] = { L'\0' };

      HANDLE hProc = GetCurrentProcess ();

      QueryFullProcessImageName (hProc, 0, wszProcessName, &dwProcessSize);

      bool is_steamworks_game =
        ( steam_tests [0].used | steam_tests [1].used | steam_tests [2].used ) ||
          StrStrIW (wszProcessName, L"steamapps");

      if (is_steamworks_game) {
        bool gl = false, vulkan = false, d3d9 = false, dxgi = false;

        SK_TestRenderImports ( GetModuleHandle (nullptr), &gl, &vulkan, &d3d9, &dxgi );

        if (dxgi)
          SK_SetDLLRole (DLL_ROLE::DXGI);
        else if (d3d9)
          SK_SetDLLRole (DLL_ROLE::D3D9);
        else if (gl)
          SK_SetDLLRole (DLL_ROLE::OpenGL);
        //else if (vulkan)
          //SK_SetDLLRole (DLL_ROLE::Vulkan);
        else
          SK_SetDLLRole (DLL_ROLE::DXGI); // Auto-Guess DXGI if all else fails...
      } else {
        return false;
      }
    }

    injected = true;
  }


  //
  // Fallback to d3d9
  //
  else
    SK_SetDLLRole (DLL_ROLE::D3D9);


  return true;
}

volatile ULONG attached = FALSE;
volatile ULONG refs     = 0;

#include <Winver.h>
//#pragma comment (lib, "Mincore_Downlevel.lib")
#pragma comment (lib, "version.lib")

bool
SK_IsDLLSpecialK (const wchar_t* wszName)
{
  UINT     cbTranslatedBytes = 0,
           cbProductBytes    = 0;

  uint8_t  cbData      [16384];
  wchar_t  wszPropName [50];

  wchar_t* wszProduct; // Will point somewhere in cbData

  struct LANGANDCODEPAGE {
    WORD wLanguage;
    WORD wCodePage;
  } *lpTranslate;

  if (GetFileAttributes (wszName) == INVALID_FILE_ATTRIBUTES)
    return false;

  GetFileVersionInfoEx ( FILE_VER_GET_NEUTRAL | FILE_VER_GET_PREFETCHED,
                           wszName,
                             0x00,
                               16384,
                                 cbData );

  VerQueryValue ( cbData,
                    TEXT ("\\VarFileInfo\\Translation"),
                      (LPVOID *)&lpTranslate,
                                &cbTranslatedBytes );

  swprintf ( wszPropName,
               L"\\StringFileInfo\\%04x%04x\\ProductName",
                 lpTranslate [0].wLanguage,
                   lpTranslate [0].wCodePage );

  VerQueryValue ( cbData,
                    wszPropName,
                      (LPVOID *)&wszProduct,
                                &cbProductBytes );

  return (! lstrcmpiW (wszProduct, L"Special K"));
}

BOOL
__stdcall
SK_Attach (DLL_ROLE role)
{
  if (! InterlockedCompareExchange (&attached, FALSE, FALSE))
  {
    switch (role)
    {
    case DLL_ROLE::DXGI:
      // If this is the global injector and there is a wrapper version of Special K
      //  in the DLL search path, then bail-out!
      if (SK_IsInjected () && SK_IsDLLSpecialK (L"dxgi.dll"))
        return FALSE;

      InterlockedCompareExchange    (&attached, SK::DXGI::Startup (), FALSE);
      return InterlockedExchangeAdd (&attached, 0);
      break;

    case DLL_ROLE::D3D9:
      // If this is the global injector and there is a wrapper version of Special K
      //  in the DLL search path, then bail-out!
      if (SK_IsInjected () && SK_IsDLLSpecialK (L"d3d9.dll"))
        return FALSE;

      InterlockedCompareExchange    (&attached, SK::D3D9::Startup (), FALSE);
      return InterlockedExchangeAdd (&attached, 0);
      break;

    case DLL_ROLE::OpenGL:
      // If this is the global injector and there is a wrapper version of Special K
      //  in the DLL search path, then bail-out!
      if (SK_IsInjected () && SK_IsDLLSpecialK (L"OpenGL32.dll"))
        return FALSE;

      InterlockedCompareExchange    (&attached, SK::OpenGL::Startup (), FALSE);
      return InterlockedExchangeAdd (&attached, 0);
      break;

    case DLL_ROLE::Vulkan:
      break;
    }
  }

  return FALSE;
}

BOOL
__stdcall
SK_Detach (DLL_ROLE role)
{
  ULONG local_refs = InterlockedDecrement (&refs);

  if (local_refs == 0 && InterlockedCompareExchange (&attached, FALSE, TRUE))
  {
    extern void SK_ShutdownCOM (void);

    switch (role)
    {
    case DLL_ROLE::DXGI:
    {
      BOOL ret = SK::DXGI::Shutdown    ();
//                        SK_ShutdownCOM ();
      return ret;
    } break;
    case DLL_ROLE::D3D9:
    {
      BOOL ret = SK::D3D9::Shutdown    ();
//                        SK_ShutdownCOM ();
      return ret;
    } break;
    case DLL_ROLE::OpenGL:
    {
      BOOL ret = SK::OpenGL::Shutdown    ();
//                          SK_ShutdownCOM ();
      return ret;
    } break;
    case DLL_ROLE::Vulkan:
      break;
    }
  } else {
    dll_log.Log (L"[ SpecialK ]  ** UNCLEAN DLL Process Detach !! **");
  }

  return false;
}

BOOL
APIENTRY
DllMain ( HMODULE hModule,
          DWORD   ul_reason_for_call,
          LPVOID  lpReserved )
{
  switch (ul_reason_for_call)
  {
    case DLL_PROCESS_ATTACH:
    {
      hThreadAttach = GetCurrentThread ();

      ULONG local_refs = InterlockedIncrement (&refs);

      InterlockedCompareExchangePointer ((LPVOID *)&hModSelf, hModule, 0);

      // We reserve the right to deny attaching the DLL, this will generally
      //   happen if a game does not opt-in to system wide injection.
      if (! SK_EstablishDllRole (hModule))
        return FALSE;

      DWORD   dwProcessSize = MAX_PATH;
      wchar_t wszProcessName [MAX_PATH];

      HANDLE hProc = GetCurrentProcess ();

      QueryFullProcessImageName (hProc, 0, wszProcessName, &dwProcessSize);

      wchar_t* pwszShortName = wszProcessName + lstrlenW (wszProcessName);

      while (  pwszShortName      >  wszProcessName &&
             *(pwszShortName - 1) != L'\\')
        --pwszShortName;

      BOOL ret = SK_Attach (SK_GetDLLRole ());

      if (ret) {
        __SK_TLS_INDEX = TlsAlloc ();

        if (__SK_TLS_INDEX == TLS_OUT_OF_INDEXES) {
          ret = FALSE;

          MessageBox ( NULL,
                         L"Out of TLS Indexes",
                           L"Cannot Init. Special K",
                             MB_ICONERROR | MB_OK | MB_APPLMODAL );
        }
      }

      return ret;
    } break;

    case DLL_THREAD_ATTACH:
    {
      if (InterlockedExchangeAdd (&attached, 0)) {
        _CRT_INIT ((HINSTANCE)hModule,ul_reason_for_call, lpReserved);

        LPVOID lpvData = (LPVOID) LocalAlloc (LPTR, sizeof SK_TLS);

        if (lpvData != nullptr)
          if (! TlsSetValue (__SK_TLS_INDEX, lpvData))
            LocalFree (lpvData);
      }
    } break;

    case DLL_THREAD_DETACH:
    {
      if (InterlockedExchangeAdd (&attached, 0)) {
        LPVOID lpvData = (LPVOID) TlsGetValue (__SK_TLS_INDEX);

        if (lpvData != nullptr) {
          LocalFree   (lpvData);
          TlsSetValue (__SK_TLS_INDEX, nullptr);
        }

        _CRT_INIT ((HINSTANCE)hModule,ul_reason_for_call, lpReserved);
      }
    } break;

    case DLL_PROCESS_DETACH:
    {
      BOOL ret = FALSE;

      if (InterlockedExchangeAdd (&attached, 0))
      {
        ret =
          SK_Detach (SK_GetDLLRole ());
        TlsFree     (__SK_TLS_INDEX);
        _CRT_INIT   ((HINSTANCE)hModule,ul_reason_for_call, lpReserved);
      } else {
        //Sanity FAILURE: Attempt to detach something that was not properly attached?!
        dll_log.Log (L"[ SpecialK ]  ** SANITY CHECK FAILED: DLL was never attached !! **");
        return FALSE;
      }

      return ret;
    } break;
  }

  return TRUE;
}