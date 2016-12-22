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

void SK_HookWinAPI        (void);
void SK_InstallWindowHook (HWND hWnd);
void SK_AdjustWindow      (void);

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

extern ClipCursor_pfn         ClipCursor_Original;
extern SetWindowPos_pfn       SetWindowPos_Original;
extern MoveWindow_pfn         MoveWindow_Original;
extern SetWindowLong_pfn      SetWindowLongW_Original;
extern SetWindowLong_pfn      SetWindowLongA_Original;
extern GetWindowLong_pfn      GetWindowLongW_Original;
extern GetWindowLong_pfn      GetWindowLongA_Original;
extern SetWindowLongPtr_pfn   SetWindowLongPtrW_Original;
extern SetWindowLongPtr_pfn   SetWindowLongPtrA_Original;
extern GetWindowLongPtr_pfn   GetWindowLongPtrW_Original;
extern GetWindowLongPtr_pfn   GetWindowLongPtrA_Original;
extern AdjustWindowRect_pfn   AdjustWindowRect_Original;
extern AdjustWindowRectEx_pfn AdjustWindowRectEx_Original;

extern GetSystemMetrics_pfn   GetSystemMetrics_Original;
extern GetCursorPos_pfn       GetCursorPos_Original;
extern GetCursorInfo_pfn      GetCursorInfo_Original;

extern GetKeyState_pfn             GetKeyState_Original;
extern GetAsyncKeyState_pfn        GetAsyncKeyState_Original;
extern GetRawInputData_pfn         GetRawInputData_Original;
extern RegisterRawInputDevices_pfn RegisterRawInputDevices_Original;

#endif /* __SK__WINDOW_H__ */