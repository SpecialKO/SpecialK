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

#ifndef __SK__CORE_H__
#define __SK__CORE_H__

#include <SpecialK/SpecialK.h>

#undef COM_NO_WINDOWS_H
#include <Windows.h>

enum DLL_ROLE
{
  INVALID    = 0x000,


  // Graphics APIs
  DXGI       = 0x001, // D3D 10-12
  D3D9       = 0x002,
  OpenGL     = 0x004, // All versions
  Vulkan     = 0x008,
  D3D11      = 0x010, // Explicitly d3d11.dll
  D3D11_CASE = 0x011, // For use in switch statements

  DInput8    = 0x100,

  // Third-party Wrappers (i.e. dgVoodoo2)
  // -------------------------------------
  //
  //  Special K merely exports the correct symbols
  //    for binary compatibility; it has no native 
  //      support for rendering in these APIs.
  //
  D3D8       = 0xC0000010,
  DDraw      = 0xC0000020,
  Glide      = 0xC0000040, // All versions


  // Behavior Flags
  PlugIn     = 0x00010000, // Stuff like Tales of Zestiria "Fix"
  Wrapper    = 0x40000000,
  ThirdParty = 0x80000000,

  DWORDALIGN = MAXDWORD
};


#ifdef SK_BUILD_DLL
 class SK_Thread_HybridSpinlock;

extern "C++" SK_Thread_HybridSpinlock* init_mutex;
extern "C++" SK_Thread_HybridSpinlock* budget_mutex;
extern "C++" SK_Thread_HybridSpinlock* loader_lock;
extern "C++" SK_Thread_HybridSpinlock* wmi_cs;
extern "C++" SK_Thread_HybridSpinlock* cs_dbghelp;
extern "C++" SK_Thread_HybridSpinlock* init_mutex;
#endif


extern HMODULE                  backend_dll;

extern volatile LONG     __SK_DLL_Ending;
extern volatile LONGLONG SK_SteamAPI_CallbackRunCount;

extern void SK_DS3_InitPlugin    (void);
extern void SK_REASON_InitPlugin (void);
extern void SK_FAR_InitPlugin    (void);
extern void SK_FAR_FirstFrame    (void);

extern BOOL                     nvapi_init;

// We have some really sneaky overlays that manage to call some of our
//   exported functions before the DLL's even attached -- make them wait,
//     so we don't crash and burn!
void WaitForInit (void);

void __stdcall SK_InitCore     (const wchar_t* backend, void* callback);
bool __stdcall SK_StartupCore  (const wchar_t* backend, void* callback);
bool __stdcall SK_ShutdownCore (const wchar_t* backend);

struct IUnknown;

#ifdef __cplusplus
extern "C" {
#endif

      void     SK_PUBLIC_API SK_BeginBufferSwap (void);
      HRESULT  SK_PUBLIC_API SK_EndBufferSwap   (HRESULT hr, IUnknown* device = nullptr);

      HMODULE  SK_PUBLIC_API SK_GetDLL          (void);
      DLL_ROLE SK_PUBLIC_API SK_GetDLLRole      (void);

#ifdef __cplusplus
};
#endif

void
__stdcall
SK_SetConfigPath (const wchar_t* path);

const wchar_t*
__stdcall
SK_GetConfigPath (void);

void
__cdecl
SK_SetDLLRole (DLL_ROLE role);

const wchar_t*
__stdcall
SK_GetBackend (void);

bool
__cdecl
SK_IsHostAppSKIM (void);

bool
__stdcall
SK_IsInjected (bool set = false);

bool
__stdcall
SK_HasGlobalInjector (void);

HWND
SK_Win32_CreateDummyWindow (void);

// Pass nullptr to cleanup ALL windows; for internal use only.
void
SK_Win32_CleanupDummyWindow (HWND hWnd = nullptr);

void
__stdcall
SK_EstablishRootPath (void);


#endif /* __SK__CORE_H__ */