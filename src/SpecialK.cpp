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

#include <SpecialK/diagnostics/compatibility.h>

#include <Windows.h>

#include <Shlwapi.h>
#include <process.h>

#include <SpecialK/core.h>
#include <SpecialK/dxgi_backend.h>
#include <SpecialK/d3d9_backend.h>
#include <SpecialK/opengl_backend.h>
#include <SpecialK/log.h>
#include <SpecialK/utility.h>

#include <SpecialK/tls.h>

#undef LoadLibrary

// Don't EVER make these function calls from this code unit.
#define LoadLibrary int x = __stdcall;
#define FreeLibrary int x = __stdcall;

// We need this to load embedded resources correctly...
volatile HMODULE hModSelf          = 0;

volatile ULONG   __SK_DLL_Attached     = FALSE;
volatile ULONG   __SK_Threads_Attached = 0;
volatile ULONG   __SK_DLL_Refs         = 0;
volatile DWORD   __SK_TLS_INDEX        = MAXDWORD;
volatile ULONG   __SK_DLL_Ending       = FALSE;

SK_TLS*
__stdcall
SK_GetTLS (void)
{
  if (__SK_TLS_INDEX == -1)
    return nullptr;

  LPVOID lpvData =
    TlsGetValue (__SK_TLS_INDEX);

  if (lpvData == nullptr) {
    lpvData =
      (LPVOID)LocalAlloc (LPTR, sizeof SK_TLS);

    if (! TlsSetValue (__SK_TLS_INDEX, lpvData)) {
      LocalFree (lpvData);
      return nullptr;
    }
  }

  return (SK_TLS *)lpvData;
}

bool
__stdcall
SK_IsInjected (bool set)
{
// Indicates that the DLL is injected purely as a hooker, rather than
//   as a wrapper DLL.
  static volatile LONG __injected = 0L;

  if (InterlockedExchangeAdd (&__injected, 0L) > 0L)
    return true;

  if (set)
    InterlockedExchange (&__injected, 1L);

  return set;
}

HMODULE
__stdcall
SK_GetDLL (void)
{
  return (HMODULE)
    InterlockedCompareExchangePointer ((LPVOID *)&hModSelf, 0, 0);
}

bool
__stdcall
SK_EstablishDllRole (HMODULE hModule)
{
  wchar_t wszDllFullName [  MAX_PATH  ] = { L'\0' };
          wszDllFullName [MAX_PATH - 1] =   L'\0';

  GetModuleFileName (hModule, wszDllFullName, MAX_PATH - 1);

  const wchar_t* wszShort =
    SK_Path_wcsrchr (wszDllFullName, L'\\') + 1;

  if (wszShort == (const wchar_t *)nullptr + 1)
    wszShort = wszDllFullName;

  if (! SK_Path_wcsicmp (wszShort, L"dxgi.dll"))
    SK_SetDLLRole (DLL_ROLE::DXGI);

  else if (! SK_Path_wcsicmp (wszShort, L"d3d9.dll"))
    SK_SetDLLRole (DLL_ROLE::D3D9);

  else if (! SK_Path_wcsicmp (wszShort, L"OpenGL32.dll"))
    SK_SetDLLRole (DLL_ROLE::OpenGL);


  //
  // This is an injected DLL, not a wrapper DLL...
  //
  else if ( SK_Path_wcsstr (wszShort, L"SpecialK") )
  {
    bool explicit_inject = false;

    wchar_t wszD3D9 [MAX_PATH] = { L'\0' };
    wchar_t wszDXGI [MAX_PATH] = { L'\0' };
    wchar_t wszGL   [MAX_PATH] = { L'\0' };

    lstrcatW (wszD3D9, SK_GetHostPath ());
    lstrcatW (wszD3D9, L"\\SpecialK.d3d9");

    lstrcatW (wszDXGI, SK_GetHostPath ());
    lstrcatW (wszDXGI, L"\\SpecialK.dxgi");

    lstrcatW (wszGL,   SK_GetHostPath ());
    lstrcatW (wszGL,   L"\\SpecialK.OpenGL32");

    if      ( GetFileAttributesW (wszD3D9) != INVALID_FILE_ATTRIBUTES ) {
      SK_SetDLLRole (DLL_ROLE::D3D9);
      explicit_inject = true;
    }

    else if ( GetFileAttributesW (wszDXGI) != INVALID_FILE_ATTRIBUTES ) {
      SK_SetDLLRole (DLL_ROLE::DXGI);
      explicit_inject = true;
    }

    else if ( GetFileAttributesW (wszGL) != INVALID_FILE_ATTRIBUTES ) {
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

      HANDLE hProc =
        GetCurrentProcess ();

      QueryFullProcessImageName (hProc, 0, wszProcessName, &dwProcessSize);

      bool is_steamworks_game =
        ( steam_tests [0].used | steam_tests [1].used |
          steam_tests [2].used )                        ||
           SK_Path_wcsstr (wszProcessName, L"steamapps");

      if (is_steamworks_game) {
        bool gl = false, vulkan = false, d3d9 = false, dxgi = false;

        SK_TestRenderImports (
          GetModuleHandle (nullptr),
            &gl, &vulkan,
              &d3d9, &dxgi
        );

        if (dxgi)
          SK_SetDLLRole (DLL_ROLE::DXGI);
        else if (d3d9)
          SK_SetDLLRole (DLL_ROLE::D3D9);
        //else if (gl)
          //SK_SetDLLRole (DLL_ROLE::OpenGL);
        //else if (vulkan)
          //SK_SetDLLRole (DLL_ROLE::Vulkan);

        else {
          SK_SetDLLRole (DLL_ROLE::D3D9);
          //SK_SetDLLRole (DLL_ROLE::DXGI); // Auto-Guess DXGI if all else fails...
        }
      } else {
        return false;
      }
    }

    SK_IsInjected (true);

    return true;
  }

  else {
    SK_SetDLLRole (DLL_ROLE::INVALID);
    return false;
  }

  return true;
}

BOOL
__stdcall
SK_Attach (DLL_ROLE role)
{
  if (! InterlockedCompareExchangeAcquire (
          &__SK_DLL_Attached,
            FALSE,
              FALSE )
     )
  {
    switch (role)
    {
      case DLL_ROLE::DXGI:
      {
        // If this is the global injector and there is a wrapper version
        //   of Special K in the DLL search path, then bail-out!
        if (SK_IsInjected () && SK_IsDLLSpecialK (L"dxgi.dll"))
        {
          SK_SetDLLRole (DLL_ROLE::INVALID);
          return FALSE;
        }

        InterlockedCompareExchange (
          &__SK_DLL_Attached,
            SK::DXGI::Startup (),
              FALSE
        );

        return
          InterlockedExchangeAddRelease (
            &__SK_DLL_Attached,
              1
          );
      }

      case DLL_ROLE::D3D9:
      {
        // If this is the global injector and there is a wrapper version
        //   of Special K in the DLL search path, then bail-out!
        if (SK_IsInjected () && SK_IsDLLSpecialK (L"d3d9.dll"))
        {
          SK_SetDLLRole (DLL_ROLE::INVALID);
          return FALSE;
        }

        InterlockedCompareExchange (
          &__SK_DLL_Attached,
            SK::D3D9::Startup (),
              FALSE
        );

        return
          InterlockedExchangeAddRelease (
            &__SK_DLL_Attached,
              1
          );
      }


#ifndef SK_BUILD__INSTALLER
      case DLL_ROLE::OpenGL:
        // If this is the global injector and there is a wrapper version
        //   of Special K in the DLL search path, then bail-out!
        if (SK_IsInjected () && SK_IsDLLSpecialK (L"OpenGL32.dll"))
        {
          SK_SetDLLRole (DLL_ROLE::INVALID);
          return FALSE;
        }

        InterlockedCompareExchange (
          &__SK_DLL_Attached,
            SK::OpenGL::Startup (),
              FALSE
        );

        return
          InterlockedExchangeAddRelease (
            &__SK_DLL_Attached,
              1
          );
#endif

      case DLL_ROLE::Vulkan:
        SK_SetDLLRole (DLL_ROLE::INVALID);
        return FALSE;
        break;
    }
  }

  SK_SetDLLRole (DLL_ROLE::INVALID);
  return FALSE;
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
        ret = SK::DXGI::Shutdown ();
        break;

      case DLL_ROLE::D3D9:
        ret = SK::D3D9::Shutdown ();
        break;

#ifndef SK_BUILD__INSTALLER
      case DLL_ROLE::OpenGL:
        ret = SK::OpenGL::Shutdown ();
        break;
#endif

      case DLL_ROLE::Vulkan:
        break;
    }
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
      ULONG                     local_refs =
          InterlockedIncrement (&__SK_DLL_Refs);

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
      if (InterlockedCompareExchangePointer ((LPVOID *)&hModSelf, hModule, 0))
        return FALSE;

      SK_PreInitLoadLibrary ();

      // We reserve the right to deny attaching the DLL, this will generally
      //   happen if a game does not opt-in to system wide injection.
      if (! SK_EstablishDllRole (hModule))
        return FALSE;

      DWORD   dwProcessSize = MAX_PATH;
      wchar_t wszProcessName [MAX_PATH] = { L'\0' };

      HANDLE hProc = GetCurrentProcess ();

      QueryFullProcessImageName (hProc, 0, wszProcessName, &dwProcessSize);

      wchar_t* pwszShortName = wszProcessName + lstrlenW (wszProcessName);

      while (  pwszShortName      >  wszProcessName &&
             *(pwszShortName - 1) != L'\\')
        --pwszShortName;

      BOOL ret = TRUE;

      __SK_TLS_INDEX = TlsAlloc ();

      if (__SK_TLS_INDEX == TLS_OUT_OF_INDEXES) {
        ret = FALSE;

        MessageBox ( NULL,
                       L"Out of TLS Indexes",
                         L"Cannot Init. Special K",
                           MB_ICONERROR | MB_OK | MB_APPLMODAL );
      }

      if (ret) {
        ret = SK_Attach (SK_GetDLLRole ());

        if (! ret)
         TlsFree (__SK_TLS_INDEX);
      }

      return ret;
    } break;


    case DLL_PROCESS_DETACH:
    {
      BOOL ret = FALSE;

      if (InterlockedExchangeAddAcquire (&__SK_DLL_Attached, 0))
      {
        if (! InterlockedCompareExchange (&__SK_DLL_Ending, 0, 0)) {
          InterlockedExchange (&__SK_DLL_Ending, TRUE);

          ret =
            SK_Detach (SK_GetDLLRole ());
          TlsFree     (__SK_TLS_INDEX);
        }
      }

      else {
        //Sanity FAILURE: Attempt to detach something that was not properly attached?!
        dll_log.Log (L"[ SpecialK ]  ** SANITY CHECK FAILED: DLL was never attached !! **");
      }

      return ret;
    } break;



    case DLL_THREAD_ATTACH:
    {
      if (InterlockedExchangeAddAcquire (&__SK_DLL_Attached, 0))
      {
        InterlockedIncrement (&__SK_Threads_Attached);

        LPVOID lpvData =
          (LPVOID)LocalAlloc (LPTR, sizeof SK_TLS);

        if (lpvData != nullptr) {
          if (! TlsSetValue (__SK_TLS_INDEX, lpvData))
            LocalFree (lpvData);
        }
      }
    } break;


    case DLL_THREAD_DETACH:
    {
      if (InterlockedExchangeAddRelease (&__SK_DLL_Attached, 0))
      {
        LPVOID lpvData =
          (LPVOID)TlsGetValue (__SK_TLS_INDEX);

        if (lpvData != nullptr) {
          LocalFree   (lpvData);
          TlsSetValue (__SK_TLS_INDEX, nullptr);
        }

        if (InterlockedDecrement (&__SK_Threads_Attached) == 1)
        {
          InterlockedExchange (&__SK_DLL_Ending, TRUE);
          SK_Detach (SK_GetDLLRole ());
          TlsFree   (__SK_TLS_INDEX);
        }
      }
    } break;
  }

  return TRUE;
}