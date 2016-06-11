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

DLL_ROLE dll_role;

bool
SK_EstablishDllRole (HMODULE hModule)
{
  wchar_t wszDllFullName [MAX_PATH];
  wszDllFullName [MAX_PATH - 1] = L'\0';

  GetModuleFileName (hModule, wszDllFullName, MAX_PATH - 1);

  wchar_t* wszDllName = wcsrchr (wszDllFullName, L'\\') + 1;


  if (! _wcsicmp (wszDllName, L"dxgi.dll"))
    dll_role = DLL_ROLE::DXGI;

  else if (! _wcsicmp (wszDllName, L"d3d9.dll"))
    dll_role = DLL_ROLE::D3D9;

  else if (! _wcsicmp (wszDllName, L"OpenGL32.dll"))
    dll_role = DLL_ROLE::OpenGL;

  //
  // Fallback to d3d9
  //
  else
    dll_role = DLL_ROLE::D3D9;

  return true;
}

static bool attached = false;

bool
SK_Attach (DLL_ROLE role)
{
  switch (role)
  {
  case DLL_ROLE::DXGI:
    attached = true;
    return SK::DXGI::Startup ();
    break;
  case DLL_ROLE::D3D9:
    attached = true;
    return SK::D3D9::Startup ();
    break;
  case DLL_ROLE::OpenGL:
    attached = true;
    return SK::OpenGL::Startup ();
    break;
  case DLL_ROLE::Vulkan:
    break;
  }

  return false;
}

bool
SK_Detach (DLL_ROLE role)
{
  switch (role)
  {
  case DLL_ROLE::DXGI:
    attached = false;
    return SK::DXGI::Shutdown ();
    break;
  case DLL_ROLE::D3D9:
    attached = false;
    return SK::D3D9::Shutdown ();
    break;
  case DLL_ROLE::OpenGL:
    attached = false;
    return SK::OpenGL::Shutdown ();
    break;
  case DLL_ROLE::Vulkan:
    break;
  }

  return false;
}

//#define IGNORE_THREAD_ATTACH

// We need this to load embedded resources correctly...
HMODULE hModSelf;

BOOL
APIENTRY DllMain ( HMODULE hModule,
                   DWORD   ul_reason_for_call,
                   LPVOID  lpReserved )
{
  switch (ul_reason_for_call)
  {
    case DLL_PROCESS_ATTACH:
    {
#ifdef IGNORE_THREAD_ATTACH
      DisableThreadLibraryCalls (hModule);
#endif

      if (! attached)
      {
        hModSelf = hModule;

        SK_EstablishDllRole (hModule);
        SK_Attach           (dll_role);
      }
    } break;

#if 0
#ifndef IGNORE_THREAD_ATTACH
    case DLL_THREAD_ATTACH:
      //dll_log.Log (L"Custom dxgi.dll Attached (tid=%x)",
      //                GetCurrentThreadId ());
      if (dll_role == DLL_ROLE::OpenGL) {
        extern HMODULE LoadRealGL (void);
        LoadRealGL ();
      }
      break;

    case DLL_THREAD_DETACH:
      //dll_log.Log (L"Custom dxgi.dll Detached (tid=%x)",
      //                GetCurrentThreadId ());
      if (dll_role == DLL_ROLE::OpenGL) {
        //extern HMODULE FreeRealGL (void);
        //FreeRealGL ();
      }
      break;
#endif
#endif

    case DLL_PROCESS_DETACH:
    {
      if (attached)
      {
        SK_Detach (dll_role);

        extern void SK_ShutdownCOM (void);
        SK_ShutdownCOM ();
      }
    } break;
  }

  return TRUE;
}