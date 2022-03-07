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

#include <cstdint>

#include <Unknwnbase.h>
#include <appmodel.h>
#include <evntrace.h>

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
void SK_Inject_ReleaseProcess        (void);
void SK_Inject_AcquireProcess        (void);
void SK_Inject_BroadcastAttachNotify (void);


#define MAX_INJECTED_PROCS        32
#define MAX_INJECTED_PROC_HISTORY 128
#define INJECTION_RECORD_ABI_VER  "1.2.1"

typedef uint64_t AppId64_t;

extern "C"
{
struct SK_InjectionRecord_s
{
  struct {
    wchar_t      name  [MAX_PATH + 2] = { 0 };
    DWORD        id                   =   0  ;
    __time64_t   inject               =  0ULL;
    __time64_t   eject                =  0ULL;
    bool         crashed              = false;
    wchar_t      win_title [     128] = { 0 };
  } process;

  struct {
    ULONG64      frames                    =  0ULL;
    SK_RenderAPI api                       = SK_RenderAPI::Reserved;
    bool         fullscreen                = false;
    bool         dpi_aware                 = false;
  } render;

  // Use a bitmask instead of this stupidness
  struct {
    bool         xinput                    = false;
    bool         raw_input                 = false;
    bool         direct_input              = false;
    bool         hid                       = false;
    bool         steam                     = false;
  } input;

  struct {
    AppId64_t    steam_appid     =  0 ;
    wchar_t      uwp_full_name [
             PACKAGE_FULL_NAME_MAX_LENGTH
                               ] = { };
    // Others?
  } platform;

  static __declspec (dllexport) volatile LONG count;
  static __declspec (dllexport) volatile LONG rollovers;
};
};

#pragma pack (push, 1)
struct SK_SharedMemory_v1
{
  // Initialized = 0x1
  // Standby     = 0x2
  // Free        = 0x4
  uint32_t MemoryState = 0x0;
  uint32_t HighDWORD   = sizeof (SK_SharedMemory_v1) -
                         sizeof (uint32_t);


  struct WindowState_s {
    DWORD hWndFocus      = 0x0;
    DWORD hWndActive     = 0x0;
    DWORD hWndForeground = 0x0;
    DWORD dwPadding      = 0x0;
    DWORD hWndExplorer   = 0x0;
    DWORD uMsgExpRaise   = 0x0;
    DWORD uMsgExpLower   = 0x0;
    DWORD _Reserved [ 9] = { };
  } SystemWide,
    CurrentGame;


  struct EtwSessionList_s
  {
    struct SessionCtl_s {
      uint32_t SequenceId = 0;
      DWORD    _Reserved [28];
    } SessionControl;

    struct EtwSessionBase_s {
      DWORD       dwSequence  = 0x0;
      DWORD       dwPid       = 0x0;
      char        szName [24] =  "";
    } PresentMon    [ 8];

    struct EtwSessionEx_s {
      TRACEHANDLE hSession    =   0;
      TRACEHANDLE hTrace      =   0;

      struct TraceProps_s :
      EVENT_TRACE_PROPERTIES
      {
        wchar_t   wszName [MAX_PATH]
                              = L"";
      } props;
    } PresentMonEx  [ 8]; // Max Concurrent = 5

    static auto constexpr
      __MaxPresentMonSessions = 5;
  } EtwSessions;
};
#pragma pack (pop)

SK_InjectionRecord_s*
__stdcall
SK_Inject_GetRecordByIdx (int idx);

SK_InjectionRecord_s*
__stdcall
SK_Inject_GetRecord (DWORD dwPid);

HRESULT
__stdcall
SK_Inject_AuditRecord (DWORD dwPid, SK_InjectionRecord_s *pData, size_t cbSize);

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


bool SK_Inject_IsHookActive (void);
// Part of the DLL Shared Data Segment
//
//struct SK_InjectionBase_s
//{
//};



#endif /* __SK__INJECTION_H__ */