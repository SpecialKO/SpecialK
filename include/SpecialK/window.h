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

using MoveWindow_pfn         = BOOL (WINAPI *)(
  _In_ HWND hWnd,
  _In_ int  X,
  _In_ int  Y,
  _In_ int  nWidth,
  _In_ int  nHeight,
  _In_ BOOL bRedraw
);

using SetWindowPlacement_pfn = BOOL (WINAPI *)(
  _In_       HWND              hWnd,
  _In_ const WINDOWPLACEMENT *lpwndpl
);

using SetWindowPos_pfn       = BOOL (WINAPI *)(
  _In_     HWND hWnd,
  _In_opt_ HWND hWndInsertAfter,
  _In_     int  X,
  _In_     int  Y,
  _In_     int  cx,
  _In_     int  cy,
  _In_     UINT uFlags
);

using SetWindowLong_pfn      = LONG (WINAPI *)(
  _In_ HWND hWnd,
  _In_ int  nIndex,
  _In_ LONG dwNewLong
);

using SetWindowLongPtr_pfn   = LONG_PTR (WINAPI *)(
  _In_ HWND     hWnd,
  _In_ int      nIndex,
  _In_ LONG_PTR dwNewLong
);

using GetWindowLong_pfn      = LONG (WINAPI *)(
  _In_ HWND hWnd,
  _In_ int  nIndex
);

using GetWindowLongPtr_pfn   = LONG_PTR (WINAPI *)(
  _In_ HWND hWnd,
  _In_ int  nIndex
);


using AdjustWindowRect_pfn   = BOOL (WINAPI *)(
  _Inout_ LPRECT lpRect,
  _In_    DWORD  dwStyle,
  _In_    BOOL   bMenu
);

using AdjustWindowRectEx_pfn = BOOL (WINAPI *)(
  _Inout_ LPRECT lpRect,
  _In_    DWORD  dwStyle,
  _In_    BOOL   bMenu,
  _In_    DWORD  dwExStyle
);

using GetSystemMetrics_pfn = int (WINAPI *)(
  _In_ int nIndex
);

using GetWindowRect_pfn    = BOOL (WINAPI *)(
  HWND, 
  LPRECT
);
using GetClientRect_pfn    = BOOL (WINAPI *)(
  HWND,
  LPRECT
);

using DefWindowProc_pfn    = LRESULT (WINAPI *)(
  _In_ HWND   hWnd,
  _In_ UINT   Msg,
  _In_ WPARAM wParam,
  _In_ LPARAM lParam
);

using CallWindowProc_pfn   = LRESULT (WINAPI *)(
  _In_ WNDPROC lpPrevWndFunc,
  _In_ HWND    hWnd,
  _In_ UINT    Msg,
  _In_ WPARAM  wParam,
  _In_ LPARAM  lParam
);

using GetWindowLongPtr_pfn = LONG_PTR (WINAPI *)(
  _In_ HWND hWnd,
  _In_ int  nIndex
);


using ClipCursor_pfn       = BOOL (WINAPI *)(
  _In_opt_ const RECT *lpRect
);

using GetCursorPos_pfn     = BOOL (WINAPI *)(
  _Out_ LPPOINT lpPoint
);

using GetCursorInfo_pfn    = BOOL (WINAPI *)(
  _Inout_ PCURSORINFO pci
);


using GetAsyncKeyState_pfn = SHORT (WINAPI *)(
  _In_ int vKey
);

using GetKeyState_pfn      = SHORT (WINAPI *)(
  _In_ int nVirtKey
);

using RegisterRawInputDevices_pfn = BOOL (WINAPI *)(
  _In_ PCRAWINPUTDEVICE pRawInputDevices,
  _In_ UINT             uiNumDevices,
  _In_ UINT             cbSize
);

using GetRawInputData_pfn   = UINT (WINAPI *)(
  _In_      HRAWINPUT hRawInput,
  _In_      UINT      uiCommand,
  _Out_opt_ LPVOID    pData,
  _Inout_   PUINT     pcbSize,
  _In_      UINT      cbSizeHeader
);

using GetRawInputBuffer_pfn = UINT (WINAPI *)(
  _Out_opt_ PRAWINPUT pData,
  _Inout_   PUINT     pcbSize,
  _In_      UINT      cbSizeHeader
);
using GetKeyboardState_pfn  = BOOL (WINAPI *)(PBYTE lpKeyState);


using SetCursorPos_pfn      = BOOL (WINAPI *)(
  _In_ int X,
  _In_ int Y
);

using SendInput_pfn         = UINT (WINAPI *)(
  _In_ UINT    nInputs,
  _In_ LPINPUT pInputs,
  _In_ int     cbSize
);

using mouse_event_pfn       = void (WINAPI *)(
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

  HWND       hWnd             = nullptr;
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

  bool    needsCoordTransform (void);
  void    updateDims          (void);

  SetWindowLongPtr_pfn SetWindowLongPtr = nullptr;
  GetWindowLongPtr_pfn GetWindowLongPtr = nullptr;
  DefWindowProc_pfn    DefWindowProc    = nullptr;
  CallWindowProc_pfn   CallWindowProc   = nullptr;

  LRESULT DefProc (
    _In_ UINT   Msg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
  );

  LRESULT CallProc (
    _In_ HWND    hWnd_,
    _In_ UINT    Msg,
    _In_ WPARAM  wParam,
    _In_ LPARAM  lParam
  );

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