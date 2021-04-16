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

#include <Unknwnbase.h>

#include <SpecialK/window.h>
#include <SpecialK/core.h>

LRESULT
CALLBACK
CBTProc (int nCode, WPARAM wParam, LPARAM lParam);

void __stdcall SKX_InstallCBTHook (void);
void __stdcall SKX_RemoveCBTHook  (void);
bool __stdcall SKX_IsHookingCBT   (void) noexcept;

size_t __stdcall SKX_GetInjectedPIDs (DWORD* pdwList, size_t capacity);

bool
SK_Inject_SwitchToGlobalInjector (void);

bool
SK_Inject_SwitchToGlobalInjectorEx (DLL_ROLE role);

bool
SK_Inject_SwitchToRenderWrapper (void);

bool
SK_Inject_SwitchToRenderWrapperEx (DLL_ROLE role);

// Are we capable of injecting into admin-elevated applications?
bool
SK_Inject_IsAdminSupported (void) noexcept;


bool
SK_Inject_TestWhitelists (const wchar_t* wszExecutable);

bool
SK_Inject_TestBlacklists (const wchar_t* wszExecutable);


// Internal use only
//
void SK_Inject_ReleaseProcess (void);
void SK_Inject_AcquireProcess (void);


#define MAX_INJECTED_PROCS        32
#define MAX_INJECTED_PROC_HISTORY 128

extern "C"
{
struct SK_InjectionRecord_s
{
  struct {
    wchar_t    name [MAX_PATH + 2] = { 0 };
    DWORD      id                  =   0  ;
    __time64_t inject              =  0ULL;
    __time64_t eject               =  0ULL;
    bool       crashed             = false;
  } process;

  struct {
    SK_RenderAPI api        = SK_RenderAPI::Reserved;
    ULONG64      frames     =  0ULL;
    bool         fullscreen = false;
    bool         dpi_aware  = false;
  } render;

  // Use a bitmask instead of this stupidness
  struct {
    bool xinput       = false;
    bool raw_input    = false;
    bool direct_input = false;
    bool hid          = false;
    bool steam        = false;
  } input;

  struct {
    uint32_t steam_appid = 0;
    // Others?
  } platform;

  static __declspec (dllexport) volatile LONG count;
  static __declspec (dllexport) volatile LONG rollovers;
};
};

SK_InjectionRecord_s*
__stdcall
SK_Inject_GetRecord (int idx);

// Returns false if there is nothing to wait on, or if something else is
//   already waiting for unhook.
bool
SK_Inject_WaitOnUnhook (void);

typedef HHOOK (NTAPI *NtUserSetWindowsHookEx_pfn)(
          HINSTANCE hMod,
     const wchar_t* UnsafeModuleName,
              DWORD ThreadId,
                int HookId,
           HOOKPROC HookProc,
               BOOL Ansi );

typedef LRESULT (NTAPI *NtUserCallNextHookEx_pfn)(
 _In_opt_ HHOOK  hhk,
 _In_     int    nCode,
 _In_     WPARAM wParam,
 _In_     LPARAM lParam
);

typedef BOOL (WINAPI *NtUserUnhookWindowsHookEx_pfn)(
 _In_ HHOOK hhk
);

extern NtUserCallNextHookEx_pfn NtUserCallNextHookEx;


// Part of the DLL Shared Data Segment
//
//struct SK_InjectionBase_s
//{
//};



#endif /* __SK__INJECTION_H__ */