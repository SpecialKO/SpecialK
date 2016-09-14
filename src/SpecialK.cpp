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

// Indicates that the DLL is injected purely as a hooker, rather than
//   as a wrapper DLL.
bool injected = false;

DLL_ROLE dll_role;

bool
__stdcall
SK_EstablishDllRole (HMODULE hModule)
{
  std::wstring   dll_name   = SK_GetModuleName (hModule);
  const wchar_t* wszDllName = dll_name.c_str   ();


  if (! _wcsicmp (wszDllName, L"dxgi.dll"))
    dll_role = DLL_ROLE::DXGI;

  else if (! _wcsicmp (wszDllName, L"d3d9.dll"))
    dll_role = DLL_ROLE::D3D9;

  else if (! _wcsicmp (wszDllName, L"OpenGL32.dll"))
    dll_role = DLL_ROLE::OpenGL;


  //
  // This is an injected DLL, not a wrapper DLL...
  //
  else if ( wcsstr (wszDllName, L"SpecialK") )
  {
    if      ( GetFileAttributesW (L"SpecialK.d3d9") != INVALID_FILE_ATTRIBUTES )
      dll_role = DLL_ROLE::D3D9;

    else if ( GetFileAttributesW (L"SpecialK.dxgi") != INVALID_FILE_ATTRIBUTES )
      dll_role = DLL_ROLE::DXGI;

    else if ( GetFileAttributesW (L"SpecialK.OpenGL32") != INVALID_FILE_ATTRIBUTES )
      dll_role = DLL_ROLE::OpenGL;

    // Opted out of automatic injection
    else
      return false;

    injected = true;
  }


  //
  // Fallback to d3d9
  //
  else
    dll_role = DLL_ROLE::D3D9;


  return true;
}

volatile ULONG attached = FALSE;
volatile ULONG refs     = 0;

bool
__stdcall
SK_Attach (DLL_ROLE role)
{
  if (! InterlockedCompareExchange (&attached, TRUE, FALSE))
  {
    switch (role)
    {
    case DLL_ROLE::DXGI:
      return SK::DXGI::Startup   ();
      break;

    case DLL_ROLE::D3D9:
      return SK::D3D9::Startup   ();
      break;

    case DLL_ROLE::OpenGL:
      return SK::OpenGL::Startup ();
      break;

    case DLL_ROLE::Vulkan:
      break;
    }
  }

  return false;
}

bool
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
      bool ret = SK::DXGI::Shutdown    ();
                        SK_ShutdownCOM ();
      return ret;
    } break;
    case DLL_ROLE::D3D9:
    {
      bool ret = SK::D3D9::Shutdown    ();
                        SK_ShutdownCOM ();
      return ret;
    } break;
    case DLL_ROLE::OpenGL:
    {
      bool ret = SK::OpenGL::Shutdown    ();
                          SK_ShutdownCOM ();
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

//#define IGNORE_THREAD_ATTACH

// We need this to load embedded resources correctly...
volatile HMODULE hModSelf = 0;

HMODULE
__stdcall
SK_GetDLL (void)
{
  return hModSelf;
}

DWORD
WINAPI
StartDLL (LPVOID user)
{
  SK_Attach (dll_role);

  CloseHandle (GetCurrentThread ());

  return 0;
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
      ULONG local_refs = InterlockedIncrement (&refs);

      InterlockedCompareExchangePointer ((LPVOID *)&hModSelf, hModule, 0);

      // We reserve the right to deny attaching the DLL, this will generally
      //   happen if a game does not opt-in to system wide injection.
      if (! SK_EstablishDllRole (hModule))
        return FALSE;

#if 0
      CreateThread ( nullptr,
                       0,
                         StartDLL,
                           nullptr,
                             0x00,
                               nullptr );
#else
      // Similar to before, we can deny injection. If we fail at this point,
      //   it is usually because the host application has been identified and
      //     is blacklisted.
      if (! SK_Attach (dll_role))
        return FALSE;
#endif

      // We need TLS managed by the real OpenGL DLL for context
      //   management to work, so we cannot do this...
//      if (dll_role != DLL_ROLE::OpenGL)
//        DisableThreadLibraryCalls (hModule);
    } break;

#ifndef IGNORE_THREAD_ATTACH
    case DLL_THREAD_ATTACH:
    {
      //dll_log.Log (L"Custom dxgi.dll Attached (tid=%x)",
      //                GetCurrentThreadId ());
      if (! injected) {
        if (dll_role == DLL_ROLE::OpenGL) {
          extern HMODULE SK_LoadRealGL (void);
          SK_LoadRealGL ();
        }

        else if (dll_role == DLL_ROLE::D3D9) {
          extern HMODULE WINAPI SK_LoadRealD3D9 (void);
          SK_LoadRealD3D9 ();
        }

        else if (dll_role == DLL_ROLE::DXGI) {
          extern HMODULE SK_LoadRealDXGI (void);
          SK_LoadRealDXGI ();
        }
      }
    } break;

    case DLL_THREAD_DETACH:
    {
      //InterlockedDecrement (&refs);
      //dll_log.Log (L"Custom dxgi.dll Detached (tid=%x)",
      //                GetCurrentThreadId ());

      if (! injected) {
        if (dll_role == DLL_ROLE::OpenGL) {
          extern void SK_FreeRealGL (void);
          SK_FreeRealGL ();
        }

        else if (dll_role == DLL_ROLE::D3D9) {
          extern void WINAPI SK_FreeRealD3D9 (void);
          SK_FreeRealD3D9 ();
        }

        else if (dll_role == DLL_ROLE::DXGI) {
          extern void SK_FreeRealDXGI (void);
          SK_FreeRealDXGI ();
        }
      }
    } break;
#endif

    case DLL_PROCESS_DETACH:
    {
      if (InterlockedCompareExchange (&attached, FALSE, FALSE))
      {
        return SK_Detach (dll_role);
      } else {
        // Sanity FAILURE: Attempt to detach something that was not properly attached?!
        dll_log.Log (L"[ SpecialK ]  ** SANITY CHECK FAILED: DLL was never attached !! **");
        return false;
      }
    } break;
  }

  return TRUE;
}