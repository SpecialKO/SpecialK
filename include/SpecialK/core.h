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

#undef COM_NO_WINDOWS_H
#include <Windows.h>

#include <SpecialK/memory_monitor.h>
#include <SpecialK/nvapi.h>

extern CRITICAL_SECTION         init_mutex;
extern CRITICAL_SECTION         budget_mutex;
extern CRITICAL_SECTION         loader_lock;
extern CRITICAL_SECTION         wmi_cs;
extern CRITICAL_SECTION         cs_dbghelp;

extern HMODULE                  backend_dll;
extern CRITICAL_SECTION         init_mutex;

extern volatile LONGLONG SK_SteamAPI_CallbackRunCount;

extern void SK_DS3_InitPlugin    (void);
extern void SK_REASON_InitPlugin (void);
extern void SK_FAR_InitPlugin    (void);
extern void SK_FAR_FirstFrame    (void);

// Disable SLI memory in Batman Arkham Knight
extern bool                     USE_SLI;

extern NV_GET_CURRENT_SLI_STATE sli_state;
extern BOOL                     nvapi_init;

// We have some really sneaky overlays that manage to call some of our
//   exported functions before the DLL's even attached -- make them wait,
//     so we don't crash and burn!
void WaitForInit (void);

void __stdcall SK_InitCore     (const wchar_t* backend, void* callback);
bool __stdcall SK_StartupCore  (const wchar_t* backend, void* callback);
bool __stdcall SK_ShutdownCore (const wchar_t* backend);

void    STDMETHODCALLTYPE SK_BeginBufferSwap (void);
HRESULT STDMETHODCALLTYPE SK_EndBufferSwap   (HRESULT hr, IUnknown* device = nullptr);

const wchar_t* __stdcall SK_DescribeHRESULT (HRESULT result);

void
__stdcall
SK_SetConfigPath (const wchar_t* path);

const wchar_t*
__stdcall
SK_GetConfigPath (void);

HMODULE
__stdcall
SK_GetDLL (void);

DLL_ROLE
__stdcall
SK_GetDLLRole (void);

void
__cdecl
SK_SetDLLRole (DLL_ROLE role);

bool
__cdecl
SK_IsHostAppSKIM (void);

bool
__stdcall
SK_IsInjected (bool set = false);

bool
__stdcall
SK_HasGlobalInjector (void);

ULONG
__stdcall
SK_GetFramesDrawn (void);

enum DLL_ROLE {
  INVALID    = 0x000,


  // Graphics APIs
  DXGI       = 0x001, // D3D 10-12
  D3D9       = 0x002,
  OpenGL     = 0x004, // All versions
  Vulkan     = 0x008,

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
  ThirdParty = 0x80000000
};

#endif /* __SK__CORE_H__ */