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

#include "console.h"
#include "core.h"

#include <cstdint>

#include <string>
#include <algorithm>
#include "steam_api.h"

#include "log.h"
#include "config.h"
#include "command.h"
#include "utility.h"

#include <mmsystem.h>
#pragma comment (lib, "winmm.lib")

#include <comdef.h>
#include <process.h>

#include <windowsx.h>

struct window_t {
  DWORD proc_id;
  HWND  root;
};

BOOL
CALLBACK
SK_EnumWindows (HWND hWnd, LPARAM lParam)
{
  window_t& win = *(window_t*)lParam;

  DWORD proc_id = 0;

  GetWindowThreadProcessId (hWnd, &proc_id);

  if (win.proc_id != proc_id) {
    if (GetWindow (hWnd, GW_OWNER) != (HWND)nullptr ||
        ////GetWindowTextLength (hWnd) < 30           ||
          // I forget what the purpose of this is :P
        (! IsWindowVisible  (hWnd)))
      return TRUE;
  }

  win.root = hWnd;
  return FALSE;
}

HWND
SK_FindRootWindow (DWORD proc_id)
{
  window_t win;

  win.proc_id  = proc_id;
  win.root     = 0;

  EnumWindows (SK_EnumWindows, (LPARAM)&win);

  return win.root;
}

SK_Console::SK_Console (void) { }

SK_Console*
SK_Console::getInstance (void)
{
  if (pConsole == nullptr)
    pConsole = new SK_Console ();

  return pConsole;
}

#include "framerate.h"
bool bNoConsole = false;

void
SK_Console::Draw (void)
{
  // Some plugins have their own...
  if (bNoConsole)
    return;

  static bool          carret    = false;
  static LARGE_INTEGER last_time = { 0 };

  static std::string last_output = "";
         std::string output;

  if (visible) {
    output += text;

    LARGE_INTEGER now;
    LARGE_INTEGER freq;

    QueryPerformanceFrequency        (&freq);

    extern LARGE_INTEGER SK_QueryPerf (void);
    now = SK_QueryPerf ();

    // Blink the Carret
    if ((now.QuadPart - last_time.QuadPart) > (freq.QuadPart / 3)) {
      carret = ! carret;

      last_time.QuadPart = now.QuadPart;
    }

    if (carret)
      output += "-";

    // Show Command Results
      if (command_issued) {
      output += "\n";
      output += result_str;
    }
  }

  if (last_output != output) {
    last_output = output;
    extern BOOL
    __stdcall
    SK_DrawExternalOSD (std::string app_name, std::string text);
    SK_DrawExternalOSD ("SpecialK Console", output);
  }
}

typedef BOOL
(WINAPI *MoveWindow_pfn)(
    _In_ HWND hWnd,
    _In_ int  X,
    _In_ int  Y,
    _In_ int  nWidth,
    _In_ int  nHeight,
    _In_ BOOL bRedraw );

typedef BOOL
(WINAPI *SetWindowPos_pfn)(
    _In_     HWND hWnd,
    _In_opt_ HWND hWndInsertAfter,
    _In_     int  X,
    _In_     int  Y,
    _In_     int  cx,
    _In_     int  cy,
    _In_     UINT uFlags );

typedef LONG
(WINAPI *SetWindowLongA_pfn)(
    _In_ HWND hWnd,
    _In_ int nIndex,
    _In_ LONG dwNewLong);

typedef LONG
(WINAPI *SetWindowLongW_pfn)(
    _In_ HWND hWnd,
    _In_ int nIndex,
    _In_ LONG dwNewLong);

typedef BOOL
(WINAPI *AdjustWindowRect_pfn)(
    _Inout_ LPRECT lpRect,
    _In_    DWORD  dwStyle,
    _In_    BOOL   bMenu );

typedef BOOL
(WINAPI *AdjustWindowRectEx_pfn)(
    _Inout_ LPRECT lpRect,
    _In_    DWORD  dwStyle,
    _In_    BOOL   bMenu,
    _In_    DWORD  dwExStyle );


struct sk_window_s {
  HWND    hWnd             = 0x00;
  WNDPROC WndProc_Original = nullptr;
  LONG    style            = 0x00;
  LONG    style_ex         = 0x00;
  RECT    rect;

  void    getRenderDims (long& x, long& y) {
    x = (rect.right  - rect.left);
    y = (rect.bottom - rect.top);
  }
} game_window;

SetWindowPos_pfn       SetWindowPos_Original       = nullptr;
MoveWindow_pfn         MoveWindow_Original         = nullptr;
SetWindowLongW_pfn     SetWindowLongW_Original     = nullptr;
SetWindowLongA_pfn     SetWindowLongA_Original     = nullptr;
AdjustWindowRect_pfn   AdjustWindowRect_Original   = nullptr;
AdjustWindowRectEx_pfn AdjustWindowRectEx_Original = nullptr;

void SK_AdjustWindow (void);


BOOL
WINAPI
MoveWindow_Detour(
    _In_ HWND hWnd,
    _In_ int  X,
    _In_ int  Y,
    _In_ int  nWidth,
    _In_ int  nHeight,
    _In_ BOOL bRedraw )
{
  if (hWnd != game_window.hWnd) {
    return MoveWindow_Original ( hWnd,
                                   X, Y,
                                     nWidth, nHeight,
                                       bRedraw );
  }

  dll_log.Log (L"[Window Mgr][!] MoveWindow (...)");

  game_window.rect.left = X;
  game_window.rect.top  = Y;

  game_window.rect.right  = X + nWidth;
  game_window.rect.bottom = Y + nHeight;

  BOOL bRet = FALSE;

  //if (! config.render.allow_background)
    bRet = MoveWindow_Original (hWnd, X, Y, nWidth, nHeight, bRedraw);
  //else
    //bRet = TRUE;

  SK_AdjustWindow ();

  return bRet;
}

BOOL
WINAPI
SetWindowPos_Detour(
    _In_     HWND hWnd,
    _In_opt_ HWND hWndInsertAfter,
    _In_     int  X,
    _In_     int  Y,
    _In_     int  cx,
    _In_     int  cy,
    _In_     UINT uFlags)
{
  if (hWnd != game_window.hWnd) {
    return SetWindowPos_Original ( hWnd, hWndInsertAfter,
                                     X, Y,
                                       cx, cy,
                                         uFlags );
  }

  long render_width,
       render_height;

  game_window.getRenderDims (render_width, render_height);

  render_width  = std::max (render_width,  0L);
  render_height = std::max (render_height, 0L);

#if 0
  dll_log->Log ( L"[Window Mgr][!] SetWindowPos (...)");
#endif

  // Ignore this, because it's invalid.
  if ((cy == 0 || cx == 0) && (! (uFlags & SWP_NOSIZE))) {
    game_window.rect.right  = game_window.rect.left + render_width;
    game_window.rect.bottom = game_window.rect.top  + render_height;

    dll_log.Log ( L"[Window Mgr] *** Encountered invalid SetWindowPos (...) - "
                                     L"{cy:%lu, cx:%lu, uFlags:0x%x} - "
                                     L"<left:%lu, top:%lu, WxH=(%lux%lu)>",
                     cy, cx, uFlags,
                       game_window.rect.left, game_window.rect.top,
                         render_width, render_height );

    return TRUE;
  }

#if 0
  dll_log->Log ( L"  >> Before :: Top-Left: [%d/%d], Bottom-Right: [%d/%d]",
                   tsf::window.window_rect.left, tsf::window.window_rect.top,
                     tsf::window.window_rect.right, tsf::window.window_rect.bottom );
#endif

  int original_width  = game_window.rect.right -
                        game_window.rect.left;
  int original_height = game_window.rect.bottom -
                        game_window.rect.top;

  if (original_width <= 0 || original_height <= 0) {
    original_width  = render_width;
    original_height = render_height;
  }

  if (! (uFlags & SWP_NOMOVE)) {
    game_window.rect.left = X;
    game_window.rect.top  = Y;
  }

  if (! (uFlags & SWP_NOSIZE)) {
    game_window.rect.right  = game_window.rect.left + cx;
    game_window.rect.bottom = game_window.rect.top  + cy;
  } else {
    game_window.rect.right  = game_window.rect.left +
                                original_width;
    game_window.rect.bottom = game_window.rect.top  +
                                original_height;
  }

#if 0
  dll_log->Log ( L"  >> After :: Top-Left: [%d/%d], Bottom-Right: [%d/%d]",
                   tsf::window.window_rect.left, tsf::window.window_rect.top,
                     tsf::window.window_rect.right, tsf::window.window_rect.bottom );
#endif

  //
  // Fix an invalid scenario that happens for some reason...
  //
  if (game_window.rect.left == game_window.rect.right)
    game_window.rect.left = 0;
  if (game_window.rect.left == game_window.rect.right)
    game_window.rect.right = game_window.rect.left + render_width;

  if (game_window.rect.top == game_window.rect.bottom)
    game_window.rect.top = 0;
  if (game_window.rect.top == game_window.rect.bottom)
    game_window.rect.bottom = game_window.rect.top + render_height;



#if 0
  // Let the game manage its window position...
  if (! config.render.borderless)
    return SetWindowPos_Original (hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
  else
    return TRUE;
#else
#if 0
  if (config.window.borderless) {
    BOOL bRet = TRUE;

    //if (! tsf::RenderFix::fullscreen)
      bRet =
        SetWindowPos_Original ( hWnd,
                                  hWndInsertAfter,
                                    0, 0,
                                      render_width, render_height,
                                        uFlags | SWP_SHOWWINDOW );

    return bRet;
  }
  else
#endif
  {
    RECT new_rect = game_window.rect;

    AdjustWindowRect_Original ( &new_rect,
                                  game_window.style,
                                    FALSE );

    if (! (uFlags & SWP_NOMOVE)) {
      X = new_rect.left;
      Y = new_rect.top;

      game_window.rect.left = X;
      game_window.rect.top  = Y;
    } else {
      X = 0;
      Y = 0;
    }

    if (! (uFlags & SWP_NOSIZE)) {
      cx = new_rect.right  - new_rect.left;
      cy = new_rect.bottom - new_rect.top;

      game_window.rect.right  = game_window.rect.left + cx;
      game_window.rect.bottom = game_window.rect.top  + cy;
    } else {
      cx = 0;
      cy = 0;
    }

    BOOL bRet =
      SetWindowPos_Original ( hWnd, hWndInsertAfter,
                                X, Y,
                                  cx, cy,
                                    uFlags );

    SK_AdjustWindow ();

    return bRet;
  }
#endif
}

BOOL
WINAPI
AdjustWindowRect_Detour (
    _Inout_ LPRECT lpRect,
    _In_    DWORD  dwStyle,
    _In_    BOOL   bMenu )
{
  if (! config.window.borderless) {
    return AdjustWindowRect_Original (lpRect, dwStyle, bMenu);
  }

  return TRUE;
}

BOOL
WINAPI
AdjustWindowRectEx_Detour (
    _Inout_ LPRECT lpRect,
    _In_    DWORD  dwStyle,
    _In_    BOOL   bMenu,
    _In_    DWORD  dwExStyle )
{
  if (! config.window.borderless) {
    return AdjustWindowRectEx_Original (lpRect, dwStyle, bMenu, dwExStyle);
  }

  return TRUE;
}

LONG
WINAPI
SetWindowLongA_Detour (
  _In_ HWND hWnd,
  _In_ int nIndex,
  _In_ LONG dwNewLong
)
{
  if (hWnd != game_window.hWnd) {
    return SetWindowLongA_Original ( hWnd,
                                       nIndex,
                                         dwNewLong );
  }

  if (nIndex == GWL_EXSTYLE || nIndex == GWL_STYLE) {
    //dll_log->Log ( L"[Window Mgr] SetWindowLongA (0x%06X, %s, 0x%06X)",
                     //hWnd,
               //nIndex == GWL_EXSTYLE ? L"GWL_EXSTYLE" :
                                       //L" GWL_STYLE ",
                       //dwNewLong );
  }

  // Override window styles
  if (nIndex == GWL_STYLE || nIndex == GWL_EXSTYLE) {
    // For proper return behavior
    DWORD dwOldStyle = GetWindowLong (hWnd, nIndex);

    // Allow the game to change its frame
    if (! config.window.borderless)
      return SetWindowLongA_Original (hWnd, nIndex, dwNewLong);

    return dwOldStyle;
  }

  return SetWindowLongA_Original (hWnd, nIndex, dwNewLong);
}

LONG
WINAPI
SetWindowLongW_Detour (
  _In_ HWND hWnd,
  _In_ int nIndex,
  _In_ LONG dwNewLong
)
{
  if (hWnd != game_window.hWnd) {
    return SetWindowLongW_Original ( hWnd,
                                       nIndex,
                                         dwNewLong );
  }

  if (nIndex == GWL_EXSTYLE || nIndex == GWL_STYLE) {
    //dll_log->Log ( L"[Window Mgr] SetWindowLongA (0x%06X, %s, 0x%06X)",
                     //hWnd,
               //nIndex == GWL_EXSTYLE ? L"GWL_EXSTYLE" :
                                       //L" GWL_STYLE ",
                       //dwNewLong );
  }

  // Override window styles
  if (nIndex == GWL_STYLE || nIndex == GWL_EXSTYLE) {
    // For proper return behavior
    DWORD dwOldStyle = GetWindowLong (hWnd, nIndex);

    // Allow the game to change its frame
    if (! config.window.borderless)
      return SetWindowLongW_Original (hWnd, nIndex, dwNewLong);

    return dwOldStyle;
  }

  return SetWindowLongW_Original (hWnd, nIndex, dwNewLong);
}

void
SK_AdjustWindow (void)
{
  HMONITOR hMonitor =
    MonitorFromWindow ( game_window.hWnd,
                          MONITOR_DEFAULTTONEAREST );

  MONITORINFO mi = { 0 };
  mi.cbSize      = sizeof (mi);

  GetMonitorInfo (hMonitor, &mi);

#if 0
  if (fullscreen) {
    //dll_log->Log (L"BorderManager::AdjustWindow - Fullscreen");

    SetWindowPos_Original ( tsf::window.hwnd,
                              HWND_TOP,
                                mi.rcMonitor.left,
                                mi.rcMonitor.top,
                                  mi.rcMonitor.right  - mi.rcMonitor.left,
                                  mi.rcMonitor.bottom - mi.rcMonitor.top,
                                    SWP_FRAMECHANGED | SWP_SHOWWINDOW | SWP_NOSENDCHANGING );
    dll_log->Log ( L"[Border Mgr] FULLSCREEN => {Left: %li, Top: %li} - (WxH: %lix%li)",
                     mi.rcMonitor.left, mi.rcMonitor.top,
                       mi.rcMonitor.right - mi.rcMonitor.left,
                         mi.rcMonitor.bottom - mi.rcMonitor.top );
  } else
#endif
  {
    //dll_log->Log (L"BorderManager::AdjustWindow - Windowed");

    AdjustWindowRect_Original (
      &mi.rcWork,
        game_window.style,
          FALSE
    );

    LONG mon_width  = mi.rcWork.right  - mi.rcWork.left;
    LONG mon_height = mi.rcWork.bottom - mi.rcWork.top;

    LONG render_width,
         render_height;

    game_window.getRenderDims (render_width, render_height);

    LONG win_width  = std::min (mon_width,  render_width);
    LONG win_height = std::min (mon_height, render_height);

    if (config.window.x_offset >= 0)
      game_window.rect.left  = mi.rcWork.left  + config.window.x_offset;
    else
      game_window.rect.right = mi.rcWork.right + config.window.x_offset + 1;


    if (config.window.y_offset >= 0)
      game_window.rect.top    = mi.rcWork.top    + config.window.y_offset;
    else
      game_window.rect.bottom = mi.rcWork.bottom + config.window.y_offset + 1;


    if (config.window.center && config.window.x_offset == 0 && config.window.y_offset == 0) {
      //dll_log->Log ( L"[Window Mgr] Center --> (%li,%li)",
      //                 mi.rcWork.right - mi.rcWork.left,
      //                   mi.rcWork.bottom - mi.rcWork.top );

      game_window.rect.left = std::max (0L, (mon_width  - win_width)  / 2);
      game_window.rect.top  = std::max (0L, (mon_height - win_height) / 2);
    }


    if (config.window.x_offset >= 0)
      game_window.rect.right = game_window.rect.left  + win_width;
    else
      game_window.rect.left  = game_window.rect.right - win_width;


    if (config.window.y_offset >= 0)
      game_window.rect.bottom = game_window.rect.top    + win_height;
    else
      game_window.rect.top    = game_window.rect.bottom - win_height;


    //AdjustWindowRect_Original ( &game_window.rect,
                                  //game_window.style,
                                    //FALSE );

    int push_right = 0;

    if (game_window.rect.left < 0)
      push_right = 0 - game_window.rect.left;

    game_window.rect.left  += push_right;
    game_window.rect.right += push_right;


    int push_down = 0;

    if (game_window.rect.top < 0)
      push_down = 0 - game_window.rect.top;

    game_window.rect.top    += push_down;
    game_window.rect.bottom += push_down;


    SetWindowPos_Original ( game_window.hWnd,
                              HWND_TOP,
                                game_window.rect.left, game_window.rect.top,
                                  game_window.rect.right  - game_window.rect.left,
                                  game_window.rect.bottom - game_window.rect.top,
                                    (config.window.borderless ? SWP_FRAMECHANGED : 0x0) |
                                      SWP_NOZORDER      | SWP_NOREDRAW   |
                                      SWP_NOOWNERZORDER | SWP_NOACTIVATE |
                                      SWP_NOCOPYBITS );

    dll_log.Log ( L"[Border Mgr] WINDOW => {Left: %li, Top: %li} - (WxH: %lix%li)",
                    game_window.rect.left, game_window.rect.top,
                       game_window.rect.right - game_window.rect.left,
                         game_window.rect.bottom - game_window.rect.top );
  }
}

void
SK_Console::Start (void)
{
  // STUPID HACK UNTIL WE PROPERLY UNIFY SK AND TSFIX'S CONSOLE.
  if (GetModuleHandle (L"AgDrag.dll") || GetModuleHandle (L"PrettyPrinny.dll")) {
    bNoConsole = true;
    return;
  }

  if ( config.window.borderless ||
       config.window.center     ||
       config.window.x_offset   ||
       config.window.y_offset )
  {
    SK_CreateDLLHook2 ( L"user32.dll", "SetWindowPos",
                          SetWindowPos_Detour,
               (LPVOID *)&SetWindowPos_Original );

    SK_CreateDLLHook2 ( L"user32.dll", "MoveWindow",
                          MoveWindow_Detour,
               (LPVOID *)&MoveWindow_Original );

    SK_CreateDLLHook2 ( L"user32.dll", "SetWindowLongA",
                          SetWindowLongA_Detour,
               (LPVOID *)&SetWindowLongA_Original );

    SK_CreateDLLHook2 ( L"user32.dll", "SetWindowLongW",
                          SetWindowLongW_Detour,
               (LPVOID *)&SetWindowLongW_Original );

    SK_CreateDLLHook2 ( L"user32.dll", "AdjustWindowRect",
                          AdjustWindowRect_Detour,
               (LPVOID *)&AdjustWindowRect_Original );

    SK_CreateDLLHook2 ( L"user32.dll", "AdjustWindowRectEx",
                          AdjustWindowRectEx_Detour,
               (LPVOID *)&AdjustWindowRectEx_Original );

    SK_ApplyQueuedHooks ();
  }

  hMsgPump =
    (HANDLE)
      _beginthreadex ( nullptr,
                         0,
                           SK_Console::MessagePump,
                             &hooks,
                               0,
                                 nullptr );
}

void
SK_Console::End (void)
{
  // STUPID HACK UNTIL WE PROPERLY UNIFY SK AND TZFIX'S CONSOLE.
  if (GetModuleHandle (L"AgDrag.dll") || GetModuleHandle (L"PrettyPrinny.dll")) {
    bNoConsole = true;
    return;
  }

  TerminateThread     (hMsgPump, 0);
  UnhookWindowsHookEx (hooks.keyboard);
}

HANDLE
SK_Console::GetThread (void)
{
  return hMsgPump;
}

#include <dxgi.h>
extern IDXGISwapChain* g_pSwapChainDXGI;
extern HWND            hWndRender;

typedef SHORT (WINAPI *GetAsyncKeyState_pfn)(
  _In_ int vKey
);

typedef SHORT (WINAPI *GetKeyState_pfn)(
  _In_ int nVirtKey
);

typedef UINT (WINAPI *GetRawInputData_pfn)(
  _In_      HRAWINPUT hRawInput,
  _In_      UINT      uiCommand,
  _Out_opt_ LPVOID    pData,
  _Inout_   PUINT     pcbSize,
  _In_      UINT      cbSizeHeader
);

GetKeyState_pfn      GetKeyState_Original      = nullptr;
GetAsyncKeyState_pfn GetAsyncKeyState_Original = nullptr;
GetRawInputData_pfn  GetRawInputData_Original  = nullptr;

UINT
WINAPI
GetRawInputData_Detour (_In_      HRAWINPUT hRawInput,
                        _In_      UINT      uiCommand,
                        _Out_opt_ LPVOID    pData,
                        _Inout_   PUINT     pcbSize,
                        _In_      UINT      cbSizeHeader)
{
  int size = GetRawInputData_Original (hRawInput, uiCommand, pData, pcbSize, cbSizeHeader);

  // Block keyboard input to the game while the console is active
  if (SK_Console::getInstance ()->isVisible () && uiCommand == RID_INPUT) {
    *pcbSize = 0;
    return 0;
  }

  return size;
}

SHORT
WINAPI
GetAsyncKeyState_Detour (_In_ int vKey)
{
#define SK_ConsumeVKey(vKey) { GetAsyncKeyState_Original(vKey); return 0; }

  // Block keyboard input to the game while the console is active
  if (SK_Console::getInstance ()->isVisible ()) {
    SK_ConsumeVKey (vKey);
  }

  return GetAsyncKeyState_Original (vKey);
}

SHORT
WINAPI
GetKeyState_Detour (_In_ int nVirtKey)
{
  // Block keyboard input to the game while the console is active
  if (SK_Console::getInstance ()->isVisible ()) {
    return 0;
  }

  return GetKeyState_Original (nVirtKey);
}

extern bool
WINAPI
SK_IsSteamOverlayActive (void);

// Avoid static storage in the callback function
struct {
  POINTS pos      = { 0 }; // POINT (Short) - Not POINT plural ;)
  DWORD  sampled  = 0UL;
  bool   cursor   = true;

  int    init     = false;
  int    timer_id = 0x68993;
} last_mouse;

DWORD
WINAPI
SK_RealizeForegroundWindow (HWND hWndForeground)
{
  static volatile ULONG nest_lvl = 0UL;

  while (InterlockedExchangeAdd (&nest_lvl, 0))
    Sleep (125);

  InterlockedIncrementAcquire (&nest_lvl);

  CreateThread (
    nullptr,
      0,
        [](LPVOID user)->

        DWORD
        {
          BringWindowToTop    ((HWND)user);
          SetForegroundWindow ((HWND)user);

          CloseHandle (GetCurrentThread ());

          return 0;
        },

        (LPVOID)hWndForeground,
      0x00,
    nullptr
  );

  InterlockedDecrementRelease (&nest_lvl);

  return 0UL;
}

__declspec (noinline)
LRESULT
CALLBACK
SK_DetourWindowProc ( _In_  HWND   hWnd,
                      _In_  UINT   uMsg,
                      _In_  WPARAM wParam,
                      _In_  LPARAM lParam )
{
  bool console_visible =
    SK_Console::getInstance ()->isVisible ();

  // HACK: Fallout 4 terminates abnormally at shutdown, meaning DllMain will
  //         never be called.
  //
  //       To properly shutdown the DLL, trap this window message instead of
  //         relying on DllMain (...) to be called.
  if ( hWnd             == game_window.hWnd &&
       uMsg             == WM_DESTROY       &&
       (! lstrcmpW (SK_GetHostApp (), L"Fallout4.exe")) ) {
    dll_log.Log ( L"[ SpecialK ] --- Invoking DllMain shutdown in response to "
                  L"WM_DESTROY ---" );
    SK_SelfDestruct ();
  }

  if (console_visible) {
    if (uMsg >= WM_KEYFIRST && uMsg <= WM_KEYLAST)
      return DefWindowProc (hWnd, uMsg, wParam, lParam);
    // Block RAW Input
    if (uMsg == WM_INPUT)
      return DefWindowProc (hWnd, uMsg, wParam, lParam);
  }

  if (config.input.cursor.manage)
  {
    //extern bool IsControllerPluggedIn (INT iJoyID);

   auto ActivateCursor = [](bool changed = false)->
    bool
     {
       bool was_active = last_mouse.cursor;

       if (! last_mouse.cursor) {
         if (! SK_IsSteamOverlayActive ()) {
           while (ShowCursor (TRUE) < 0) ;
           last_mouse.cursor = true;
         }
       }

       if (changed && (! SK_IsSteamOverlayActive ()))
         last_mouse.sampled = timeGetTime ();

       return (last_mouse.cursor != was_active);
     };

   auto DeactivateCursor = []()->
    bool
     {
       if (! last_mouse.cursor)
         return false;

       bool was_active = last_mouse.cursor;

       if (last_mouse.sampled <= timeGetTime () - config.input.cursor.timeout) {
         if (! SK_IsSteamOverlayActive ()) {
           while (ShowCursor (FALSE) >= 0) ;
           last_mouse.cursor = false;
          }
       }

       return (last_mouse.cursor != was_active);
     };

    if (! last_mouse.init) {
      if (config.input.cursor.timeout != 0)
        SetTimer (hWnd, last_mouse.timer_id, config.input.cursor.timeout / 2, nullptr);
      else
        SetTimer (hWnd, last_mouse.timer_id, 250/*USER_TIMER_MINIMUM*/, nullptr);

      last_mouse.init = true;
    }

    bool activation_event =
      (uMsg == WM_MOUSEMOVE) && (! SK_IsSteamOverlayActive ());

    // Don't blindly accept that WM_MOUSEMOVE actually means the mouse moved...
    if (activation_event) {
      const short threshold = 2;

      // Filter out small movements
      if ( abs (last_mouse.pos.x - GET_X_LPARAM (lParam)) < threshold &&
           abs (last_mouse.pos.y - GET_Y_LPARAM (lParam)) < threshold )
        activation_event = false;

      last_mouse.pos = MAKEPOINTS (lParam);
    }

    if (config.input.cursor.keys_activate)
      activation_event |= ( uMsg == WM_CHAR       ||
                            uMsg == WM_SYSKEYDOWN ||
                            uMsg == WM_SYSKEYUP );

    // If timeout is 0, just hide the thing indefinitely
    if (activation_event && config.input.cursor.timeout != 0)
      ActivateCursor (true);

    else if (uMsg == WM_TIMER && wParam == last_mouse.timer_id && (! SK_IsSteamOverlayActive ())) {
      if (true)//IsControllerPluggedIn (config.input.gamepad_slot))
        DeactivateCursor ();

      else
        ActivateCursor ();
    }
  }

  return CallWindowProc (game_window.WndProc_Original, hWnd, uMsg, wParam, lParam);
}

#include "core.h"

void
SK_InstallWindowHook (HWND hWnd)
{
  SK_CreateDLLHook2 ( L"user32.dll", "GetRawInputData",
                     GetRawInputData_Detour,
           (LPVOID*)&GetRawInputData_Original );

  SK_CreateDLLHook2 ( L"user32.dll", "GetKeyState",
                     GetKeyState_Detour,
           (LPVOID*)&GetKeyState_Original );

  SK_CreateDLLHook2 ( L"user32.dll", "GetAsyncKeyState",
                     GetAsyncKeyState_Detour,
           (LPVOID*)&GetAsyncKeyState_Original );

  MH_ApplyQueued ();

  game_window.hWnd = hWnd;

  game_window.WndProc_Original =
    (WNDPROC)GetWindowLongPtrW (game_window.hWnd, GWLP_WNDPROC);

  SetWindowLongPtrW ( game_window.hWnd,
                     GWLP_WNDPROC,
                       (LONG_PTR)SK_DetourWindowProc );

  if (! config.window.borderless)
    game_window.style = GetWindowLongA (game_window.hWnd, GWL_STYLE);//0x90CA0000;
  else
    game_window.style = WS_VISIBLE | WS_POPUP | WS_MINIMIZEBOX;

  if ( config.window.borderless ||
       config.window.center     ||
       config.window.x_offset   ||
       config.window.y_offset )
  {
    GetWindowRect (game_window.hWnd, &game_window.rect);

    if (config.window.borderless)
      SetWindowLongA_Original (game_window.hWnd, GWL_STYLE, game_window.style);

    SK_AdjustWindow ();

#if 0
    AdjustWindowRect_Original ( &game_window.rect,
                                  game_window.style,
                                    FALSE );

    SetWindowPos_Original ( game_window.hWnd, HWND_TOP,
                              game_window.rect.left, game_window.rect.top,
                                game_window.rect.right  - game_window.rect.left,
                                game_window.rect.bottom - game_window.rect.top,
                                  SWP_FRAMECHANGED  | SWP_NOZORDER   |
                                  SWP_NOREDRAW      | SWP_NOACTIVATE |
                                  SWP_NOOWNERZORDER | SWP_NOCOPYBITS );
#endif
  }
}

HWND
__stdcall
SK_GetGameWindow (void)
{
  return game_window.hWnd;
}


extern
ULONG
__stdcall
SK_GetFramesDrawn (void);

unsigned int
__stdcall
SK_Console::MessagePump (LPVOID hook_ptr)
{
  hooks_t* pHooks = (hooks_t *)hook_ptr;

  ZeroMemory (text, 4096);

  text [0] = '>';

  extern HMODULE __stdcall SK_GetDLL (void);

  HWND  hWndForeground;
  DWORD dwThreadId;

  unsigned int hits = 0;

  DWORD dwTime = timeGetTime ();

  while (true) {
    // Spin until the game has drawn a frame
    if (hWndRender == 0 || (! SK_GetFramesDrawn ())) {
      Sleep (83);
      continue;
    }

    if (hWndRender != 0) {
      hWndForeground = hWndRender;
    } else {
      hWndForeground = GetForegroundWindow ();
    }

    DWORD dwProc;

    dwThreadId =
      GetWindowThreadProcessId (hWndForeground, &dwProc);

    // Ugly hack, but a different window might be in the foreground...
    if (dwProc != GetCurrentProcessId ()) {
      //dll_log.Log (L" *** Tried to hook the wrong process!!!");
      Sleep (83);

      continue;
    }

    if (SK_GetFramesDrawn () > 1)
      break;
  }
  dll_log.Log ( L"[CmdConsole]  # Found window in %03.01f seconds, "
                L"installing keyboard hook...",
                  (float)(timeGetTime () - dwTime) / 1000.0f );

  SK_InstallWindowHook (hWndForeground);

  dwTime = timeGetTime ();
  hits   = 1;

  while (! (pHooks->keyboard = SetWindowsHookEx ( WH_KEYBOARD,
              KeyboardProc,
                SK_GetDLL (),
                  dwThreadId ))) {
    _com_error err (HRESULT_FROM_WIN32 (GetLastError ()));

    dll_log.Log ( L"[CmdConsole]  @ SetWindowsHookEx failed: 0x%04X (%s)",
      err.WCode (), err.ErrorMessage () );

    ++hits;

    if (hits >= 5) {
      dll_log.Log ( L"[CmdConsole]  * Failed to install keyboard hook after %lu tries... "
        L"bailing out!",
        hits );
      return 0;
    }

    Sleep (1);
  }

  dll_log.Log ( L"[CmdConsole]  * Installed keyboard hook for command console... "
                L"%lu %s (%lu ms!)",
                  hits,
                    hits > 1 ? L"tries" : L"try",
                      timeGetTime () - dwTime );

  // This may look strange, but this thread is a dummy that must be kept alive
  //   in order for the input hook to work.
  Sleep (INFINITE);

  return 0;
}

// Plugins can hook this if they do not have their own input handler
__declspec (noinline)
void
CALLBACK
SK_PluginKeyPress (BOOL Control, BOOL Shift, BOOL Alt, BYTE vkCode)
{
  static bool keys [256];
  keys [vkCode & 0xFF] = ! keys [vkCode & 0xFF];
}

LRESULT
CALLBACK
SK_Console::KeyboardProc (int nCode, WPARAM wParam, LPARAM lParam)
{
  if (nCode >= 0) {
    BYTE    vkCode   = LOWORD (wParam) & 0xFF;
    BYTE    scanCode = HIWORD (lParam) & 0x7F;
    SHORT   repeated = LOWORD (lParam);
    bool    keyDown  = ! (lParam & 0x80000000);

    if (visible && vkCode == VK_BACK) {
      if (keyDown) {
        size_t len = strlen (text);
               len--;

        if (len < 1)
          len = 1;

        text [len] = '\0';
      }
    }

    else if (vkCode == VK_SHIFT || vkCode == VK_LSHIFT || vkCode == VK_RSHIFT) {
      if (keyDown) keys_ [VK_SHIFT] = 0x81; else keys_ [VK_SHIFT] = 0x00;
    }

    else if (vkCode == VK_MENU || vkCode == VK_LMENU || vkCode == VK_RMENU) {
      if (keyDown) keys_ [VK_MENU] = 0x81; else keys_ [VK_MENU] = 0x00;
    }

    else if ((! repeated) && vkCode == VK_CAPITAL) {
      if (keyDown) if (keys_ [VK_CAPITAL] == 0x00) keys_ [VK_CAPITAL] = 0x82; else keys_ [VK_CAPITAL] = 0x00;
    }

    else if (vkCode == VK_CONTROL || vkCode == VK_LCONTROL || vkCode == VK_RCONTROL) {
      if (keyDown) keys_ [VK_CONTROL] = 0x81; else keys_ [VK_CONTROL] = 0x00;
    }

    else if ((vkCode == VK_UP) || (vkCode == VK_DOWN)) {
      if (keyDown && visible) {
        if (vkCode == VK_UP)
          commands.idx--;
        else
          commands.idx++;

        // Clamp the index
        if (commands.idx < 0)
          commands.idx = 0;
        else if (commands.idx >= commands.history.size ())
          commands.idx = commands.history.size () - 1;

        if (commands.history.size ()) {
          strcpy (&text [1], commands.history [commands.idx].c_str ());
          command_issued = false;
        }
      }
    }

    else if (visible && vkCode == VK_RETURN) {
      if (keyDown && LOWORD (lParam) < 2) {
        size_t len = strlen (text+1);
        // Don't process empty or pure whitespace command lines
        if (len > 0 && strspn (text+1, " ") != len) {
          SK_ICommandResult result =
            SK_GetCommandProcessor ()->ProcessCommandLine (text+1);

          if (result.getStatus ()) {
            // Don't repeat the same command over and over
            if (commands.history.size () == 0 ||
                commands.history.back () != &text [1]) {
              commands.history.push_back (&text [1]);
            }

            commands.idx = commands.history.size ();

            text [1] = '\0';

            command_issued = true;
          }
          else {
            command_issued = false;
          }

          result_str = result.getWord () + std::string (" ")   +
                       result.getArgs () + std::string (":  ") +
                       result.getResult ();
        }
      }
    }

    else if (keyDown) {
      bool new_press = keys_ [vkCode] != 0x81;

      keys_ [vkCode] = 0x81;

      if (new_press && (! SK_IsSteamOverlayActive ()))
        SK_PluginKeyPress (keys_ [VK_CONTROL], keys_ [VK_SHIFT], keys_ [VK_MENU], vkCode);

      if (keys_ [VK_CONTROL] && keys_ [VK_SHIFT] && keys_ [VK_TAB] && vkCode == VK_TAB && new_press) {
        visible = ! visible;

        // This will pause/unpause the game
        SK::SteamAPI::SetOverlayState (visible);

        return -1;
      }

      // Don't print the tab character, it's pretty useless.
      if (visible && vkCode != VK_TAB) {
        char key_str [2];
        key_str [1] = '\0';

        if (1 == ToAsciiEx ( vkCode,
                              scanCode,
                              keys_,
                            (LPWORD)key_str,
                              0,
                              GetKeyboardLayout (0) ) &&
             isprint ( *key_str ) ) {
          strncat (text, key_str, 1);
          command_issued = false;
        }
      }
    }

    else if ((! keyDown))
      keys_ [vkCode] = 0x00;
  }

  if (nCode >= 0 && visible)
    return -1;

  return CallNextHookEx (SK_Console::getInstance ()->hooks.keyboard, nCode, wParam, lParam);
};

void
SK_DrawConsole (void)
{
  if (bNoConsole)
    return;

  // Drop the first few frames so that the console shows up below
  //   the main OSD.
  if (SK_GetFramesDrawn () > 15) {
    SK_Console* pConsole = SK_Console::getInstance ();
    pConsole->Draw ();
  }
}

BOOL
__stdcall
SK_IsConsoleVisible (void)
{
  return SK_Console::getInstance ()->isVisible ();
}

SK_Console* SK_Console::pConsole      = nullptr;
char        SK_Console::text [4096];

BYTE        SK_Console::keys_ [256]    = { 0 };
bool        SK_Console::visible        = false;

bool        SK_Console::command_issued = false;
std::string SK_Console::result_str;

SK_Console::command_history_t
             SK_Console::commands;