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

#ifndef __SK__INJECTION_H__
#define __SK__INJECTION_H__

#include <SpecialK/window.h>
#include <SpecialK/core.h>

LRESULT
CALLBACK
CBTProc (int nCode, WPARAM wParam, LPARAM lParam);

void __stdcall SKX_InstallCBTHook (void);
void __stdcall SKX_RemoveCBTHook  (void);
bool __stdcall SKX_IsHookingCBT   (void);

size_t __stdcall SKX_GetInjectedPIDs (DWORD* pdwList, size_t capacity);

bool
SK_Inject_SwitchToGlobalInjector (void);

bool
SK_Inject_SwitchToGlobalInjectorEx (DLL_ROLE role);

bool
SK_Inject_SwitchToRenderWrapper (void);

bool
SK_Inject_SwitchToRenderWrapperEx (DLL_ROLE role);


// Internal use only
//
void
SK_Inject_ReleaseProcess (void);

void
SK_Inject_AcquireProcess (void);


#define MAX_INJECTED_PROCS        16
#define MAX_INJECTED_PROC_HISTORY 64

struct SK_InjectionRecord_s
{
  struct {
    wchar_t    name [MAX_PATH] = {  };
    DWORD      id              =    0;
    __time64_t inject          = 0ULL;
    __time64_t eject           = 0ULL;
  } process;

  struct {
    SK_RenderAPI api    = SK_RenderAPI::Reserved;
    ULONG64      frames = 0ULL;
  } render;

  // Use a bitmask instead of this stupidness
  struct {
    bool xinput       = false;
    bool raw_input    = false;
    bool direct_input = false;
    bool hid          = false;
    bool steam        = false;
  } input;

  static volatile LONG count;
  static volatile LONG rollovers;
};

SK_InjectionRecord_s*
SK_Inject_GetRecord (int idx);


#endif /* __SK__INJECTION_H__ */