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

#ifndef __SK__WINDOW_H__
#define __SK__WINDOW_H__

#include <Windows.h>
#include <SpecialK/config.h>

#undef GetWindowLong
#undef GetWindowLongPtr
#undef SetWindowLong
#undef SetWindowLongPtr
#undef CallWindowProc
#undef DefWindowProc

void SK_HookWinAPI        (void);
void SK_InstallWindowHook (HWND hWnd);
void SK_InitWindow        (HWND hWnd, bool fullscreen_exclusive = false);
void SK_AdjustWindow      (void);
void SK_AdjustBorder      (void);

void SK_CenterWindowAtMouse (BOOL remember_pos);

void SK_SetWindowResX (LONG x);
void SK_SetWindowResY (LONG y);

int __stdcall SK_GetSystemMetrics (_In_ int nIndex);
LPRECT        SK_GetGameRect      (void);
bool          SK_DiscontEpsilon   (int x1, int x2, int tolerance);

DWORD __stdcall SK_RealizeForegroundWindow (HWND hWndForeground);
HWND  __stdcall SK_GetGameWindow           (void);

typedef BOOL
(WINAPI *MoveWindow_pfn)(
    _In_ HWND hWnd,
    _In_ int  X,
    _In_ int  Y,
    _In_ int  nWidth,
    _In_ int  nHeight,
    _In_ BOOL bRedraw );

typedef BOOL (WINAPI *SetWindowPlacement_pfn)(
  _In_       HWND            hWnd,
  _In_ const WINDOWPLACEMENT *lpwndpl
);

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
(WINAPI *SetWindowLong_pfn)(
    _In_ HWND hWnd,
    _In_ int  nIndex,
    _In_ LONG dwNewLong);

typedef LONG_PTR
(WINAPI *SetWindowLongPtr_pfn)(
    _In_ HWND     hWnd,
    _In_ int      nIndex,
    _In_ LONG_PTR dwNewLong);

typedef LONG
(WINAPI *GetWindowLong_pfn)(
  _In_ HWND hWnd,
  _In_ int  nIndex
);

typedef LONG_PTR
(WINAPI *GetWindowLongPtr_pfn)(
  _In_ HWND hWnd,
  _In_ int  nIndex
);


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

typedef int
(WINAPI *GetSystemMetrics_pfn)(
  _In_ int nIndex
);

typedef BOOL
(WINAPI *GetWindowRect_pfn)(
  HWND, 
  LPRECT );
typedef BOOL
(WINAPI *GetClientRect_pfn)(
  HWND,
  LPRECT );

typedef LRESULT (WINAPI *DefWindowProc_pfn)(
    _In_ HWND   hWnd,
    _In_ UINT   Msg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
);

typedef LRESULT (WINAPI *CallWindowProc_pfn)(
    _In_ WNDPROC lpPrevWndFunc,
    _In_ HWND    hWnd,
    _In_ UINT    Msg,
    _In_ WPARAM  wParam,
    _In_ LPARAM  lParam
);

typedef LONG_PTR (WINAPI *GetWindowLongPtr_pfn)(
    _In_ HWND hWnd,
    _In_ int  nIndex
);


typedef BOOL
(WINAPI *ClipCursor_pfn)(
    _In_opt_ const RECT *lpRect );

typedef BOOL
(WINAPI *GetCursorPos_pfn)(
  _Out_ LPPOINT lpPoint );

typedef BOOL
(WINAPI *GetCursorInfo_pfn)(
  _Inout_ PCURSORINFO pci );


typedef SHORT (WINAPI *GetAsyncKeyState_pfn)(
  _In_ int vKey
);

typedef SHORT (WINAPI *GetKeyState_pfn)(
  _In_ int nVirtKey
);

typedef BOOL (WINAPI *RegisterRawInputDevices_pfn)(
  _In_ PCRAWINPUTDEVICE pRawInputDevices,
  _In_ UINT             uiNumDevices,
  _In_ UINT             cbSize
);

typedef UINT (WINAPI *GetRawInputData_pfn)(
  _In_      HRAWINPUT hRawInput,
  _In_      UINT      uiCommand,
  _Out_opt_ LPVOID    pData,
  _Inout_   PUINT     pcbSize,
  _In_      UINT      cbSizeHeader
);

typedef UINT (WINAPI *GetRawInputBuffer_pfn)(
                               _Out_opt_ PRAWINPUT pData,
                               _Inout_   PUINT     pcbSize,
                               _In_      UINT      cbSizeHeader);
typedef BOOL (WINAPI *GetKeyboardState_pfn)(PBYTE lpKeyState);


typedef BOOL (WINAPI *SetCursorPos_pfn)
(
  _In_ int X,
  _In_ int Y
);

typedef UINT (WINAPI *SendInput_pfn)(
  _In_ UINT    nInputs,
  _In_ LPINPUT pInputs,
  _In_ int     cbSize
);

typedef VOID(WINAPI *mouse_event_pfn)(
  _In_ DWORD     dwFlags,
  _In_ DWORD     dx,
  _In_ DWORD     dy,
  _In_ DWORD     dwData,
  _In_ ULONG_PTR dwExtraInfo
);

extern ClipCursor_pfn              ClipCursor_Original;
extern SetWindowPos_pfn            SetWindowPos_Original;
extern MoveWindow_pfn              MoveWindow_Original;
extern SetWindowLong_pfn           SetWindowLongW_Original;
extern SetWindowLong_pfn           SetWindowLongA_Original;
extern GetWindowLong_pfn           GetWindowLongW_Original;
extern GetWindowLong_pfn           GetWindowLongA_Original;
extern SetWindowLongPtr_pfn        SetWindowLongPtrW_Original;
extern SetWindowLongPtr_pfn        SetWindowLongPtrA_Original;
extern GetWindowLongPtr_pfn        GetWindowLongPtrW_Original;
extern GetWindowLongPtr_pfn        GetWindowLongPtrA_Original;
extern AdjustWindowRect_pfn        AdjustWindowRect_Original;
extern AdjustWindowRectEx_pfn      AdjustWindowRectEx_Original;

extern GetSystemMetrics_pfn        GetSystemMetrics_Original;
extern GetCursorPos_pfn            GetCursorPos_Original;
extern SetCursorPos_pfn            SetCursorPos_Original;
extern GetCursorInfo_pfn           GetCursorInfo_Original;

extern SendInput_pfn               SendInput_Original;
extern mouse_event_pfn             mouse_event_Original;

extern GetKeyState_pfn             GetKeyState_Original;
extern GetAsyncKeyState_pfn        GetAsyncKeyState_Original;
extern GetKeyboardState_pfn        GetKeyboardState_Original;
extern GetRawInputData_pfn         GetRawInputData_Original;
extern GetRawInputBuffer_pfn       GetRawInputBuffer_Original;
extern RegisterRawInputDevices_pfn RegisterRawInputDevices_Original;

#include <SpecialK/input/input.h>

struct sk_window_s {
  bool       unicode          = false;

  HWND       hWnd             = 0x00;
  WNDPROC    WndProc_Original = nullptr;
  WNDPROC    RawProc_Original = nullptr;

  bool       exclusive_full   = false; //D3D only

  bool       active           = true;

  struct {
    int width  = 0;
    int height = 0;
  } border;

  struct {
    struct {
      LONG   width            = 640;
      LONG   height           = 480;
    } framebuffer;

    //struct {
    RECT   client { 0, 0, 640, 480 };
    RECT   window { 0, 0, 640, 480 };
    //};

    ULONG_PTR style           = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
    ULONG_PTR style_ex        = WS_EX_APPWINDOW     | WS_EX_WINDOWEDGE;
  } game, actual;

  ULONG_PTR   border_style    = WS_CLIPSIBLINGS     | WS_CLIPCHILDREN |
                                WS_OVERLAPPEDWINDOW | WS_VISIBLE;
  ULONG_PTR   border_style_ex = WS_EX_APPWINDOW     | WS_EX_WINDOWEDGE;

//  RECT      rect        { 0, 0,
//                          0, 0 };
//  RECT      game_rect   { 0, 0,
//                          0, 0 };

  struct {
    // Will be false if remapping is necessary
    bool     identical        = true;

    struct {
      float  x                = 1.0f;
      float  y                = 1.0f;
    } scale;

    struct {
      float  x                = 0.0f;
      float  y                = 0.0f;
    } offset;
  } coord_remap;

  LONG      render_x         = 640;
  LONG      render_y         = 480;

  LONG      game_x           = 640; // Resolution game thinks it's running at
  LONG      game_y           = 480; // Resolution game thinks it's running at

  RECT      cursor_clip { LONG_MIN, LONG_MIN,
                          LONG_MAX, LONG_MAX };

  // Cursor position when window activation changed
  POINT     cursor_pos  { 0, 0 };

  // State to restore the cursor to
  //  (TODO: Should probably be a reference count to return to)
  bool      cursor_visible   = true;

  void    getRenderDims (long& x, long& y) {
    x = (actual.client.right  - actual.client.left);
    y = (actual.client.bottom - actual.client.top);
  }

  bool    needsCoordTransform (void)
  {
    if (! config.window.res.override.fix_mouse)
      return false;

    bool dynamic_window =
      (config.window.borderless /*&& config.window.fullscreen*/);

    if (! dynamic_window)
      return false;

    return (! coord_remap.identical);
  }

  void updateDims (void)
  {
    if (config.window.borderless && config.window.fullscreen)
    {
      HMONITOR hMonitor =
      MonitorFromWindow ( hWnd,
                            MONITOR_DEFAULTTONEAREST );

      MONITORINFO mi = { 0 };
      mi.cbSize      = sizeof (mi);

      GetMonitorInfo (hMonitor, &mi);

      actual.window = mi.rcMonitor;

      actual.client.left   = 0;
      actual.client.right  = actual.window.right - actual.window.left;
      actual.client.top    = 0;
      actual.client.bottom = actual.window.bottom - actual.window.top;
    }

    long game_width   = (game.client.right   - game.client.left);
    long window_width = (actual.client.right - actual.client.left);

    long game_height   = (game.client.bottom   - game.client.top);
    long window_height = (actual.client.bottom - actual.client.top);

    bool resized =
      (game_width != window_width || game_height != window_height);

    bool moved =
      ( game.window.left != actual.window.left ||
        game.window.top  != actual.window.top );

    if (resized || moved) {
      coord_remap.identical = false;

      coord_remap.scale.x =
        (float)window_width  / (float)game_width;
      coord_remap.scale.y =
        (float)window_height / (float)game_height;

      coord_remap.offset.x =
        (float)(actual.window.left + actual.client.left) -
        (float)(game.window.left   + game.client.left);
      coord_remap.offset.y =
        (float)(actual.window.top + actual.client.top) -
        (float)(game.window.top   + game.client.top);
    }

    else {
      coord_remap.identical = true;

      coord_remap.offset.x  = 0.0f;
      coord_remap.offset.y  = 0.0f;

      coord_remap.scale.x   = 0.0f;
      coord_remap.scale.y   = 0.0f;
    }
  }

  SetWindowLongPtr_pfn SetWindowLongPtr = nullptr;
  GetWindowLongPtr_pfn GetWindowLongPtr = nullptr;
  DefWindowProc_pfn    DefWindowProc    = nullptr;
  CallWindowProc_pfn   CallWindowProc   = nullptr;

  LRESULT DefProc (
    _In_ UINT   Msg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
  ) { return DefWindowProc (hWnd, Msg, wParam, lParam); }

  LRESULT CallProc      (
    _In_ HWND    hWnd_,
    _In_ UINT    Msg,
    _In_ WPARAM  wParam,
    _In_ LPARAM  lParam )
  {
    if (! hooked)
      return CallWindowProc (WndProc_Original, hWnd_, Msg, wParam, lParam);
    else
      return WndProc_Original (hWnd_, Msg, wParam, lParam);
  }

  bool hooked = false;
};

extern sk_window_s game_window;


struct window_t {
  DWORD proc_id;
  HWND  root;
};

window_t
SK_FindRootWindow (DWORD proc_id);

#endif /* __SK__WINDOW_H__ */