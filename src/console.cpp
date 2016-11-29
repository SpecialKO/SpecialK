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

#include <SpecialK/console.h>
#include <SpecialK/hooks.h>
#include <SpecialK/core.h>

#include <cstdint>

#include <string>
#include <algorithm>
#include <SpecialK/steam_api.h>

#include <SpecialK/log.h>
#include <SpecialK/config.h>
#include <SpecialK/command.h>
#include <SpecialK/utility.h>
#include <SpecialK/osd/text.h>

#include <mmsystem.h>
#pragma comment (lib, "winmm.lib")

#include <comdef.h>
#include <process.h>

#include <windowsx.h>

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

typedef BOOL
(WINAPI *ClipCursor_pfn)(
    _In_opt_ const RECT *lpRect );

typedef BOOL
(WINAPI *GetCursorPos_pfn)(
  _Out_ LPPOINT lpPoint );

typedef BOOL
(WINAPI *GetCursorInfo_pfn)(
  _Inout_ PCURSORINFO pci );

typedef int
(WINAPI *GetSystemMetrics_pfn)(
  _In_ int nIndex
);

ClipCursor_pfn         ClipCursor_Original         = nullptr;
SetWindowPos_pfn       SetWindowPos_Original       = nullptr;
MoveWindow_pfn         MoveWindow_Original         = nullptr;
SetWindowLongW_pfn     SetWindowLongW_Original     = nullptr;
SetWindowLongA_pfn     SetWindowLongA_Original     = nullptr;
AdjustWindowRect_pfn   AdjustWindowRect_Original   = nullptr;
AdjustWindowRectEx_pfn AdjustWindowRectEx_Original = nullptr;

static GetSystemMetrics_pfn GetSystemMetrics_Original = nullptr;
static GetCursorPos_pfn     GetCursorPos_Original     = nullptr;
static GetCursorInfo_pfn    GetCursorInfo_Original    = nullptr;

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

SK_Console::SK_Console (void) {
  visible        = false;
  command_issued = false;
  result_str     = "";

  ZeroMemory (text, 4096);
  ZeroMemory (keys_, 256);
}

SK_Console*
SK_Console::getInstance (void)
{
  if (pConsole == nullptr)
    pConsole = new SK_Console ();

  return pConsole;
}

#include <SpecialK/framerate.h>
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
         std::string output = "";

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
  } else {
    output = "";
  }

  //if (last_output != output) {
    last_output = output;
    extern BOOL
    __stdcall
    SK_DrawExternalOSD (std::string app_name, std::string text);
    SK_DrawExternalOSD ("SpecialK Console", output);
  //}
}

struct sk_window_s {
  HWND      hWnd             = 0x00;
  WNDPROC   WndProc_Original = nullptr;
  WNDPROC   RawProc_Original = nullptr;

  bool      active           = true;

  LONG      style            = 0x00;
  LONG      style_ex         = 0x00;

  RECT      rect        { 0, 0,
                          0, 0 };
  RECT      game_rect   { 0, 0,
                          0, 0 };

  struct {
    // Will be false if remapping is necessary
    bool    identical        = true;

    struct {
      float x                = 1.0f;
      float y                = 1.0f;
    } scale;

    struct {
      float x                = 0.0f;
      float y                = 0.0f;
    } offset;
  } coord_remap;

  LONG      render_x         = 0;
  LONG      render_y         = 0;

  LONG      game_x           = 0; // Resolution game thinks it's running at
  LONG      game_y           = 0; // Resolution game thinks it's running at

  RECT      cursor_clip { LONG_MIN, LONG_MIN,
                          LONG_MAX, LONG_MAX };

  // Cursor position when window activation changed
  POINT     cursor_pos  { 0, 0 };

  // State to restore the cursor to
  //  (TODO: Should probably be a reference count to return to)
  bool      cursor_visible   = true;

  void    getRenderDims (long& x, long& y) {
    x = (rect.right  - rect.left);
    y = (rect.bottom - rect.top);
  }

  bool    needsCoordTransform (void)
  {
    if (! config.window.res.override.fix_mouse)
      return false;

    bool dynamic_window =
      true;//(config.window.borderless /*&& config.window.fullscreen*/);

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

      rect = mi.rcMonitor;
    }

    long game_width   = (game_rect.right - game_rect.left);
    long window_width = (rect.right      - rect.left);

    long game_height   = (game_rect.bottom - game_rect.top);
    long window_height = (rect.bottom      - rect.top);

    bool resized =
      (game_width != window_width || game_height != window_height);

    bool moved =
      ( game_rect.left != rect.left ||
        game_rect.top  != rect.top );

    if (resized || moved) {
      coord_remap.identical = false;

      coord_remap.scale.x =
        (float)window_width  / (float)game_width;
      coord_remap.scale.y =
        (float)window_height / (float)game_height;

      coord_remap.offset.x =
        (float)rect.left - (float)game_rect.left;
      coord_remap.offset.y =
        (float)rect.top  - (float)game_rect.top;
    }

    else {
      coord_remap.identical = true;

      coord_remap.offset.x  = 0.0f;
      coord_remap.offset.y  = 0.0f;

      coord_remap.scale.x   = 0.0f;
      coord_remap.scale.y   = 0.0f;
    }

    // If we are confining the mouse cursor, update the clip
    //   rectangle immediately...
    if ( config.window.confine_cursor && rect.right  - rect.left > 0 &&
                                         rect.bottom - rect.top  > 0 ) {
      ClipCursor_Original (&rect);
    }
  }
} game_window;

void SK_AdjustWindow (void);

// TODO: Refactor that structure above into this class ;)
//
//  Also, combine a few of the input manager settings with window manager
class SK_WindowManager : public SK_IVariableListener
{
public:
  virtual bool OnVarChange (SK_IVariable* var, void* val)
  {
    if (var == confine_cursor_)
    {
      if (val != nullptr)
      {
        config.window.confine_cursor = *(bool *)val;

        if (! config.window.confine_cursor)
          ClipCursor (nullptr);
        else {
          game_window.updateDims ();
          ClipCursor             (&game_window.game_rect);
        }
      }

      return true;
    }

    else if ( var == center_window_ || var == x_offset_   || var == y_offset_   ||
              var == borderless_    || var == x_override_ || var == y_override_ ||
              var == fullscreen_    || var == x_off_pct_  || var == y_off_pct_ )
    {
      if (val != nullptr)
      {
        if (var == center_window_)
          config.window.center = *(bool *)val;

        else if (var == x_offset_) {
          if (*(int *)val >= -4096 && *(int *)val <= 4096) {
            config.window.offset.x.absolute = *(signed int *)val;
            config.window.offset.x.percent  = 0.0f;
          }
        }

        else if (var == y_offset_) {
          if (*(int *)val >= -4096 && *(int *)val <= 4096) {
            config.window.offset.y.absolute = *(signed int *)val;
            config.window.offset.y.percent  = 0.0f;
          }
        }

        else if (var == x_off_pct_) {
          if (*(float *)val > -1.0f && *(float *)val < 1.0f) {
            config.window.offset.x.absolute = 0;
            config.window.offset.x.percent = *(float *)val;
          }
        }

        else if (var == y_off_pct_) {
          if (*(float *)val > -1.0f && *(float *)val < 1.0f) {
            config.window.offset.y.absolute = 0;
            config.window.offset.y.percent  = *(float *)val;
          }
        }

        else if (var == borderless_)
          config.window.borderless = *(bool *)val;

        else if (var == fullscreen_)
          config.window.fullscreen = *(bool *)val;

        else if (var == x_override_ || var == y_override_) {
          if ((! config.window.borderless) || config.window.fullscreen)
            return false;

          if (var == x_override_) {
            config.window.res.override.x = *(unsigned int *)val;

            // We cannot allow one variable ot remain 0 while the other becomes
            //   non-zero, so just make the window a square temporarily.
            if (config.window.res.override.y == 0)
              config.window.res.override.y = config.window.res.override.x;
          }

          else if (var == y_override_) {
            config.window.res.override.y = *(unsigned int *)val;

            // We cannot allow one variable ot remain 0 while the other becomes
            //   non-zero, so just make the window a square temporarily.
            if (config.window.res.override.x == 0)
              config.window.res.override.x = config.window.res.override.y;
          }

          // We have to override BOTH variables to 0 at the same time, or the window
          //   will poof! :P
          if (*(unsigned int *)val == 0) {
            config.window.res.override.x = 0;
            config.window.res.override.y = 0;
          }
        }

        snprintf ( override_res, 32, "%lux%lu",
                     config.window.res.override.x,
                       config.window.res.override.y );

        SK_AdjustWindow ();
      }

      return true;
    }

    else if ( var == background_mute_ )
    {
      if (val != nullptr)
      {
        config.window.background_mute = *(bool *)val;

        if (config.window.background_mute && (! game_window.active)) {
          muteGame (true);
        } else if (! config.window.background_mute) {
          muteGame (false);
        }

        return true;
      }
    }

    else if (var == res_override_)
    {
      if ((! config.window.borderless) || config.window.fullscreen)
        return false;

      unsigned int x = 65535;
      unsigned int y = 65535;

      char szTemp [32] = { '\0' };
      strncpy (szTemp, *(char **)val, 32);

      if (val != nullptr)
        sscanf (szTemp, "%lux%lu", &x, &y);

      SK_ICommandProcessor* cmd =
        SK_GetCommandProcessor ();

      if ((x > 320 && x < 16384 && y > 240 && y < 16384) || (x == 0 && y == 0)) {
        config.window.res.override.x = x;
        config.window.res.override.y = y;

        SK_AdjustWindow ();

        char *pszRes = (char *)((SK_IVarStub <char *>*)var)->getValuePtr ();
        snprintf (pszRes, 32, "%lux%lu", x, y);

        return true;
      }

      else {
        char *pszRes = (char *)((SK_IVarStub <char *>*)var)->getValuePtr ();
        snprintf (pszRes, 32, "INVALID");

        return false;
      }
    }

    return false;
  }

  SK_WindowManager (void)
  {
    SK_ICommandProcessor* cmd =
      SK_GetCommandProcessor ();

    confine_cursor_ =
      SK_CreateVar (SK_IVariable::Boolean,&config.window.confine_cursor,this);

    cmd->AddVariable (
      "Window.ConfineCursor",
        confine_cursor_
    );

    borderless_ =
      SK_CreateVar (SK_IVariable::Boolean, &config.window.borderless, this);

    background_mute_ =
      SK_CreateVar (SK_IVariable::Boolean, &config.window.background_mute, this);

    center_window_ =
      SK_CreateVar (SK_IVariable::Boolean, &config.window.center, this);

    fullscreen_ =
      SK_CreateVar (SK_IVariable::Boolean, &config.window.fullscreen, this);

    x_override_ =
      SK_CreateVar (SK_IVariable::Int,     &config.window.res.override.x, this);

    y_override_ =
      SK_CreateVar (SK_IVariable::Int,     &config.window.res.override.y, this);

    res_override_ =
      SK_CreateVar (SK_IVariable::String,  override_res, this);

    // Don't need to listen for this event, actually.
    fix_mouse_ =
      SK_CreateVar (SK_IVariable::Boolean, &config.window.res.override.fix_mouse);

    x_offset_ =
      SK_CreateVar (SK_IVariable::Int,     &config.window.offset.x.absolute, this);

    y_offset_ =
      SK_CreateVar (SK_IVariable::Int,     &config.window.offset.y.absolute, this);

    x_off_pct_ =
      SK_CreateVar (SK_IVariable::Float,   &config.window.offset.x.percent, this);

    y_off_pct_ =
      SK_CreateVar (SK_IVariable::Float,   &config.window.offset.y.percent, this);

    cmd->AddVariable (
      "Window.Borderless",
        borderless_
    );

    cmd->AddVariable (
      "Window.BackgroundMute",
        background_mute_
    );

    cmd->AddVariable (
      "Window.Center",
        center_window_
    );

    cmd->AddVariable (
      "Window.Fullscreen",
        fullscreen_
    );

    cmd->AddVariable (
      "Window.OverrideX",
        x_override_
    );

    cmd->AddVariable (
      "Window.OverrideY",
        y_override_
    );

    cmd->AddVariable (
      "Window.OverrideRes",
        res_override_
    );

    snprintf ( override_res, 32, "%lux%lu",
                 config.window.res.override.x,
                   config.window.res.override.y );

    cmd->AddVariable (
      "Window.OverrideMouse",
        fix_mouse_
    );

    cmd->AddVariable (
      "Window.XOffset",
        x_offset_
    );

    cmd->AddVariable (
      "Window.YOffset",
        y_offset_
    );

    cmd->AddVariable (
      "Window.ScaledXOffset",
        x_off_pct_
    );

    cmd->AddVariable (
      "Window.ScaledYOffset",
        y_off_pct_
    );
  };

  static SK_WindowManager* getInstance (void)
  {
    if (pWindowManager == nullptr)
      pWindowManager = new SK_WindowManager ();

    return pWindowManager;
  }

  void muteGame (bool bMute)
  {
    extern void
    SK_SetGameMute (BOOL bMute);

    SK_SetGameMute (bMute);
  }

protected:
  SK_IVariable* borderless_;
  SK_IVariable* background_mute_;
  SK_IVariable* confine_cursor_;
  SK_IVariable* center_window_;
  SK_IVariable* fullscreen_;
  SK_IVariable* x_override_;
  SK_IVariable* y_override_;
  SK_IVariable* res_override_; // Set X and Y at the same time
  SK_IVariable* fix_mouse_;
  SK_IVariable* x_offset_;    SK_IVariable* x_off_pct_;
  SK_IVariable* y_offset_;    SK_IVariable* y_off_pct_;

  char override_res [32];

private:
  static SK_WindowManager* pWindowManager;
};

SK_WindowManager* SK_WindowManager::pWindowManager = nullptr;


void
SK_SetWindowResX (LONG x)
{
  game_window.render_x = x;
}

void
SK_SetWindowResY (LONG y)
{
  game_window.render_y = y;
}

LPRECT
SK_GetGameRect (void)
{
  return &game_window.game_rect;
}

//
// Given raw coordinates directly from Windows, transform them into a coordinate space
//   that looks familiar to the game ;)
//
void
SK_CalcCursorPos (LPPOINT lpPoint)
{
  if (! game_window.needsCoordTransform ())
    return;

  struct {
    float width,
          height;
  } in, out;

  lpPoint->x -= ( (LONG)game_window.rect.left );
  lpPoint->y -= ( (LONG)game_window.rect.top  );

  in.width   = game_window.rect.right       - game_window.rect.left;
  in.height  = game_window.rect.bottom      - game_window.rect.top;

  out.width  = game_window.game_rect.right  - game_window.game_rect.left;
  out.height = game_window.game_rect.bottom - game_window.game_rect.top;

  float x = 2.0f * ((float)lpPoint->x / in.width ) - 1.0f;
  float y = 2.0f * ((float)lpPoint->y / in.height) - 1.0f;

  lpPoint->x = (LONG)( ( x * out.width  + out.width ) / 2.0f +
                       game_window.coord_remap.offset.x );

  lpPoint->y = (LONG)( (y * out.height + out.height) / 2.0f +
                       game_window.coord_remap.offset.y );
}

//
// Given coordinates in the game's original coordinate space, produce coordinates
//   in the global coordinate space.
//
void
SK_ReverseCursorPos (LPPOINT lpPoint)
{
  if (! game_window.needsCoordTransform ())
    return;

  struct {
    float width,
          height;
  } in, out;

  lpPoint->x -= ( (LONG)game_window.coord_remap.offset.x +
                        game_window.game_rect.left );
  lpPoint->y -= ( (LONG)game_window.coord_remap.offset.y +
                        game_window.game_rect.top  );

  in.width   = game_window.game_rect.right  - game_window.game_rect.left;
  in.height  = game_window.game_rect.bottom - game_window.game_rect.top;

  out.width  = game_window.rect.right       - game_window.rect.left;
  out.height = game_window.rect.bottom      - game_window.rect.top;

  float x = 2.0f * ((float)lpPoint->x / in.width ) - 1.0f;
  float y = 2.0f * ((float)lpPoint->y / in.height) - 1.0f;

  lpPoint->x = (LONG)( ( x * out.width  + out.width ) / 2.0f + 
                       game_window.coord_remap.offset.x );

  lpPoint->y = (LONG)( (y * out.height + out.height) / 2.0f +
                       game_window.coord_remap.offset.y );
}

BOOL
WINAPI
GetCursorPos_Detour (LPPOINT lpPoint)
{
  BOOL ret = GetCursorPos_Original (lpPoint);

  // Correct the cursor position for Aspect Ratio
  if (game_window.needsCoordTransform ()) {
    SK_CalcCursorPos (lpPoint);
  }

  return ret;
}

BOOL
WINAPI
GetCursorInfo_Detour (PCURSORINFO pci)
{
  BOOL ret = GetCursorInfo_Original (pci);

  // Correct the cursor position for Aspect Ratio
  if (game_window.needsCoordTransform ()) {
    POINT pt;

    pt.x = pci->ptScreenPos.x;
    pt.y = pci->ptScreenPos.y;

    SK_CalcCursorPos (&pt);

    pci->ptScreenPos.x = pt.x;
    pci->ptScreenPos.y = pt.y;
  }

  return ret;
}


BOOL
WINAPI
ClipCursor_Detour (const RECT *lpRect)
{
  if (lpRect != nullptr)
    game_window.cursor_clip = *lpRect;

  //
  // If the game uses mouse clipping and we are running in borderless,
  //   we need to re-adjust the window coordinates.
  //
  if ( lpRect != nullptr &&
       game_window.needsCoordTransform () )
  {
    POINT top_left, bottom_right;

    top_left.y = game_window.rect.top;
    top_left.x = game_window.rect.left;

    bottom_right.y = game_window.rect.bottom;
    bottom_right.x = game_window.rect.right;

    SK_ReverseCursorPos (&top_left);
    SK_ReverseCursorPos (&bottom_right);

    game_window.cursor_clip.bottom = bottom_right.y;
    game_window.cursor_clip.top    = top_left.y;

    game_window.cursor_clip.left   = top_left.x;
    game_window.cursor_clip.right  = bottom_right.x;
  }

  if (game_window.active && lpRect != nullptr) {
    return ClipCursor_Original (&game_window.cursor_clip);
  } else {
    return ClipCursor_Original (nullptr);
  }
}

void
SK_CenterWindowAtMouse (BOOL remember_pos)
{
  POINT mouse;

  if (GetCursorPos_Original != nullptr)
    GetCursorPos_Original (&mouse);

  struct {
    struct {
      float percent;
      int   absolute;
    } x,y;
  } offsets;

  if (! remember_pos) {
    offsets.x.absolute = config.window.offset.x.absolute;
    offsets.y.absolute = config.window.offset.y.absolute;

    offsets.x.percent = config.window.offset.x.percent;
    offsets.y.percent = config.window.offset.y.percent;
  }

  config.window.offset.x.absolute = mouse.x;
  config.window.offset.y.absolute = mouse.y;

  config.window.offset.x.absolute -= (game_window.rect.right  - game_window.rect.left) / 2;
  config.window.offset.y.absolute -= (game_window.rect.bottom - game_window.rect.top)  / 2;

  if (config.window.offset.x.absolute <= 0)
    config.window.offset.x.absolute = 1;  // 1 = Flush with Left

  if (config.window.offset.y.absolute <= 0)
    config.window.offset.y.absolute = 1;  // 1 = Flush with Top

  config.window.offset.x.percent = 0.0f;
  config.window.offset.y.percent = 0.0f;

  SK_AdjustWindow ();

  if (! remember_pos) {
    config.window.offset.x.absolute = offsets.x.absolute;
    config.window.offset.y.absolute = offsets.y.absolute;

    config.window.offset.x.percent  = offsets.x.percent;
    config.window.offset.y.percent  = offsets.y.percent;
  }
}

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
  //if (game_window.hWnd == 0)
    //game_window.hWnd = hWnd;

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
  //if (game_window.hWnd == 0)
    //game_window.hWnd = hWnd;

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

  
  RECT new_rect = game_window.rect;

  if (! config.window.borderless) {
    AdjustWindowRect_Original ( &new_rect,
                                  game_window.style,
                                    FALSE );
  }

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

  // Fix really weird behavior that happens in Tales of Zestiria
  //   and Tales of Symphonia ... NEVER ALLOW TOPMOST!
  BOOL bRet =
    SetWindowPos_Original ( hWnd, HWND_NOTOPMOST,
                              X, Y,
                                cx, cy,
                                  uFlags  );

  SK_AdjustWindow ();

  return bRet;
}

typedef BOOL (WINAPI *GetWindowRect_pfn)(HWND, LPRECT);
static GetWindowRect_pfn GetWindowRect_Original = nullptr;

typedef BOOL (WINAPI *GetClientRect_pfn)(HWND, LPRECT);
static GetClientRect_pfn GetClientRect_Original = nullptr;

static
BOOL
WINAPI
GetClientRect_Detour (
    _In_  HWND   hWnd,
    _Out_ LPRECT lpRect )
{
  //if (game_window.hWnd == 0)
    //game_window.hWnd = hWnd;

  if ( config.window.borderless &&
       hWnd == game_window.hWnd )
  {
    *lpRect = game_window.game_rect;

    return TRUE;
  }

  else {
    return GetClientRect_Original (hWnd, lpRect);
  }
}

static
BOOL
WINAPI
GetWindowRect_Detour (
    _In_   HWND  hWnd,
    _Out_ LPRECT lpRect )
{
  //if (game_window.hWnd == 0)
    //game_window.hWnd = hWnd;

  if ( config.window.borderless &&
       hWnd == game_window.hWnd )
  {
    *lpRect = game_window.game_rect;

    return TRUE;
  }

  else {
    return GetWindowRect_Original (hWnd, lpRect);
  }
}

BOOL
WINAPI
SK_GetWindowRect (HWND hWnd, LPRECT rect)
{
  if (GetWindowRect_Original != nullptr)
    return GetWindowRect_Original (hWnd, rect);

  return GetWindowRect (hWnd, rect);
}

BOOL
WINAPI
SK_IsRectZero (_In_ LPRECT lpRect)
{
  return ( lpRect->left   == lpRect->right &&
           lpRect->bottom == lpRect->top );
}

BOOL
WINAPI
AdjustWindowRect_Detour (
    _Inout_ LPRECT lpRect,
    _In_    DWORD  dwStyle,
    _In_    BOOL   bMenu )
{
  if (! config.window.borderless) {
    BOOL bRet = AdjustWindowRect_Original (lpRect, dwStyle, bMenu);

    if (bRet && (! SK_IsRectZero (lpRect))) {
      game_window.game_rect = *lpRect;
      game_window.updateDims ();
    }

    return bRet;
  }

  dll_log.Log ( L"[Window Mgr] AdjustWindowRect ( {%lu,%lu / %lu,%lu}, 0x%4X, %lu",
                  lpRect->left, lpRect->top,
                    lpRect->right, lpRect->bottom,
                      dwStyle, bMenu );

  if (! SK_IsRectZero (lpRect)) {
    game_window.game_rect = *lpRect;
    game_window.updateDims ();
  }

  return TRUE;//AdjustWindowRect_Original (lpRect, WS_VISIBLE | WS_POPUP | WS_MINIMIZEBOX, FALSE);
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
    BOOL bRet = AdjustWindowRectEx_Original (lpRect, dwStyle, bMenu, dwExStyle);

    if (bRet && (! SK_IsRectZero (lpRect))) {
      game_window.game_rect = *lpRect;
      game_window.updateDims ();
    }

    return bRet;
  }

  dll_log.Log ( L"[Window Mgr] AdjustWindowRectEx ( {%lu,%lu / %lu,%lu}, 0x%4X, %lu, 0x%4X",
                  lpRect->left, lpRect->top,
                    lpRect->right, lpRect->bottom,
                      dwStyle, bMenu,
                        dwExStyle );

  if (! SK_IsRectZero (lpRect)) {
    game_window.game_rect = *lpRect;
    game_window.updateDims ();
  }

  return TRUE;//AdjustWindowRect_Original (lpRect, WS_VISIBLE | WS_POPUP | WS_MINIMIZEBOX, FALSE);
}

LONG
WINAPI
SetWindowLongA_Detour (
  _In_ HWND hWnd,
  _In_ int nIndex,
  _In_ LONG dwNewLong
)
{
  //if (game_window.hWnd == 0)
    //game_window.hWnd = hWnd;

  if (hWnd != game_window.hWnd) {
    return SetWindowLongA_Original ( hWnd,
                                       nIndex,
                                         dwNewLong );
  }

  // Override window styles
  if (nIndex == GWL_STYLE || nIndex == GWL_EXSTYLE) {
    // For proper return behavior
    DWORD dwOldStyle = GetWindowLong (hWnd, nIndex);

    // Allow the game to change its frame
    if (nIndex == GWL_STYLE) {
      if (config.window.borderless) {
        return SetWindowLongA_Original ( hWnd, nIndex,
                                           WS_VISIBLE | WS_POPUP |
                                           WS_MINIMIZEBOX );
      }
    }
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
  //if (game_window.hWnd == 0)
    //game_window.hWnd = hWnd;

  if (hWnd != game_window.hWnd) {
    return SetWindowLongW_Original ( hWnd,
                                       nIndex,
                                         dwNewLong );
  }

  // Override window styles
  if (nIndex == GWL_STYLE || nIndex == GWL_EXSTYLE) {
    // For proper return behavior
    DWORD dwOldStyle = GetWindowLong (hWnd, nIndex);

    // Allow the game to change its frame
    if (nIndex == GWL_STYLE) {
      if (config.window.borderless) {
        return SetWindowLongW_Original ( hWnd, nIndex,
                                           WS_VISIBLE | WS_POPUP |
                                           WS_MINIMIZEBOX );
      }
    }
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

  // Restore the game's original dimensions when this changes
  static bool was_fullscreen = false;
  static bool was_override   = false;
  static bool was_borderless = false;

  bool border_changed = false;

  if (config.window.borderless && (! was_borderless)) {
    game_window.style = GetWindowLongW (game_window.hWnd, GWL_STYLE);
    SetWindowLongW_Original (game_window.hWnd, GWL_STYLE, WS_VISIBLE | WS_POPUP | WS_MINIMIZEBOX);
    was_borderless = true;  border_changed = true;
  }

  else if ((! config.window.borderless) && was_borderless) {
    SetWindowLongW_Original (game_window.hWnd, GWL_STYLE, game_window.style);
    was_borderless = false; border_changed = true;
  }

  if (border_changed) {
    SetWindowPos_Original   ( game_window.hWnd,
                                HWND_TOP,
                                  0, 0,
                                    0, 0,
                                      SWP_NOMOVE   | SWP_NOSIZE |
                                      SWP_NOZORDER | SWP_FRAMECHANGED );
  }

  if (config.window.borderless && config.window.fullscreen) {
    //dll_log->Log (L"BorderManager::AdjustWindow - Fullscreen");

    SetWindowPos_Original ( game_window.hWnd,
                              HWND_TOP,
                                mi.rcMonitor.left,
                                mi.rcMonitor.top,
                                  mi.rcMonitor.right  - mi.rcMonitor.left,
                                  mi.rcMonitor.bottom - mi.rcMonitor.top,
                                    SWP_NOCOPYBITS | SWP_ASYNCWINDOWPOS | SWP_NOSENDCHANGING );

    dll_log.Log ( L"[Border Mgr] FULLSCREEN => {Left: %li, Top: %li} - (WxH: %lix%li)",
                    mi.rcMonitor.left, mi.rcMonitor.top,
                      mi.rcMonitor.right - mi.rcMonitor.left,
                        mi.rcMonitor.bottom - mi.rcMonitor.top );

    if (! was_fullscreen) {
      if (game_window.render_x == 0 || game_window.render_y == 0) {
        game_window.render_x = mi.rcMonitor.right  - mi.rcMonitor.left;
        game_window.render_y = mi.rcMonitor.bottom - mi.rcMonitor.top;
      }

      game_window.game_x = game_window.render_x;
      game_window.game_y = game_window.render_y;

      was_fullscreen     = true;
    }

    game_window.rect = mi.rcMonitor;
    game_window.updateDims ();
  }

  else
  {
    if (was_fullscreen) {
      if (game_window.game_x != 0 && game_window.game_y != 0) {
        game_window.render_x = game_window.game_x;
        game_window.render_y = game_window.game_y;
      }

      was_fullscreen         = false;
    }

    //dll_log->Log (L"BorderManager::AdjustWindow - Windowed");

    if (! config.window.borderless) {
      AdjustWindowRect_Original (
        &mi.rcWork,
          game_window.style,
            FALSE
      );
    }

    LONG mon_width   = mi.rcWork.right     - mi.rcWork.left;
    LONG mon_height  = mi.rcWork.bottom    - mi.rcWork.top;

    LONG real_width  = mi.rcMonitor.right  - mi.rcMonitor.left;
    LONG real_height = mi.rcMonitor.bottom - mi.rcMonitor.top;

    GetWindowRect_Original (game_window.hWnd, &game_window.rect);

    LONG render_width  = game_window.render_x,
         render_height = game_window.render_y;

    game_window.getRenderDims (render_width, render_height);

    static int no_override_x = 0, no_override_y = 0;

    if ( (! was_override) && config.window.res.override.x &&
                             config.window.res.override.y )
    {
      no_override_x = game_window.render_x;
      no_override_y = game_window.render_y;

      was_override  = true;
    }

    else if ( was_override && ( config.window.res.override.x == 0 &&
                                config.window.res.override.y == 0 ) )
    {
      if (no_override_x != 0 && no_override_y != 0) {
        game_window.render_x    = no_override_x;
        game_window.render_y    = no_override_y;

        game_window.game_x      = no_override_x;
        game_window.game_y      = no_override_y;

        render_width            = no_override_x;
        render_height           = no_override_y;

        game_window.rect.right  = game_window.rect.left + render_width;
        game_window.rect.bottom = game_window.rect.top  + render_height;
      }

      was_override              = false;
    }

    if (render_width < game_window.render_x || render_height < game_window.render_y) {
      render_width  = game_window.render_x;
      render_height = game_window.render_y;
    }

    if ( (config.window.borderless && (! config.window.fullscreen) ) && 
       (! config.window.res.override.isZero ()) )
    {
      render_width  = config.window.res.override.x;
      render_height = config.window.res.override.y;
    }

    // No adjustment if the window is full-width
    if (render_width == real_width) {
      mon_width       = real_width;
      mi.rcWork.right = mi.rcMonitor.right;
      mi.rcWork.left  = mi.rcMonitor.left;
    }


    // No adjustment if the window is full-height
    if (render_height == real_height) {
      mon_height       = real_height;
      mi.rcWork.top    = mi.rcMonitor.top;
      mi.rcWork.bottom = mi.rcMonitor.bottom;
    }


    LONG win_width  = std::min (mon_width,  render_width);
    LONG win_height = std::min (mon_height, render_height);


    // We will offset coordinates later; move the window to the top-left
    //   origin first.
    if (config.window.center) {
      game_window.rect.left   = 0;
      game_window.rect.top    = 0;
      game_window.rect.right  = win_width;
      game_window.rect.bottom = win_height;
    }

    int x_offset = ( config.window.offset.x.percent != 0.0f ? 
                       config.window.offset.x.percent * (mi.rcWork.right - mi.rcWork.left) :
                       config.window.offset.x.absolute );

    int y_offset = ( config.window.offset.y.percent != 0.0f ? 
                       config.window.offset.y.percent * (mi.rcWork.bottom - mi.rcWork.top) :
                       config.window.offset.y.absolute );

    if (x_offset > 0)
      game_window.rect.left  = mi.rcWork.left  + x_offset - 1;
    else if (x_offset < 0)
      game_window.rect.right = mi.rcWork.right + x_offset + 1;


    if (y_offset > 0)
      game_window.rect.top    = mi.rcWork.top    + y_offset - 1;
    else if (y_offset < 0)
      game_window.rect.bottom = mi.rcWork.bottom + y_offset + 1;


    if (config.window.center) {
      //dll_log->Log ( L"[Window Mgr] Center --> (%li,%li)",
      //                 mi.rcWork.right - mi.rcWork.left,
      //                   mi.rcWork.bottom - mi.rcWork.top );

      if (x_offset < 0) {
        game_window.rect.left  -= (win_width / 2);
        game_window.rect.right -= (win_width / 2);
      }

      if (y_offset < 0) {
        game_window.rect.top    -= (win_height / 2);
        game_window.rect.bottom -= (win_height / 2);
      }

      game_window.rect.left += std::max (0L, (mon_width  - win_width)  / 2);
      game_window.rect.top  += std::max (0L, (mon_height - win_height) / 2);
    }


    if (x_offset >= 0)
      game_window.rect.right = game_window.rect.left  + win_width;
    else
      game_window.rect.left  = game_window.rect.right - win_width;


    if (y_offset >= 0)
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
                                      SWP_NOCOPYBITS | SWP_ASYNCWINDOWPOS |
                                      SWP_NOSENDCHANGING );

    DWORD
    WINAPI
    SK_RealizeForegroundWindow (HWND hWndForeground);

    SK_RealizeForegroundWindow ( game_window.hWnd );

    dll_log.Log ( L"[Border Mgr] WINDOW => {Left: %li, Top: %li} - (WxH: %lix%li)",
                    game_window.rect.left, game_window.rect.top,
                       game_window.rect.right - game_window.rect.left,
                         game_window.rect.bottom - game_window.rect.top );
  }

  //GetWindowRect_Original (game_window.hWnd, &game_window.rect);
  game_window.updateDims ();
}

static int
WINAPI
GetSystemMetrics_Detour (_In_ int nIndex)
{
  int nRet = GetSystemMetrics_Original (nIndex);

  //dll_log.Log ( L"[Resolution] GetSystemMetrics (%lu) : %lu",
                  //nIndex, nRet );

  if (config.window.borderless) {
    if (nIndex == SM_CYCAPTION)
      return 0;
    if (nIndex == SM_CYMENU)
      return 0;
    if (nIndex == SM_CXBORDER)
      return 0;
    if (nIndex == SM_CYBORDER)
      return 0;
    if (nIndex == SM_CXDLGFRAME)
      return 0;
    if (nIndex == SM_CYDLGFRAME)
      return 0;
    if (nIndex == SM_CXFRAME)
      return 0;
    if (nIndex == SM_CYFRAME)
      return 0;
  }

  //dll_log.Log ( L"[Resolution] GetSystemMetrics (%lu) : %lu",
                  //nIndex, nRet );

  return nRet;
}

int
WINAPI
SK_GetSystemMetrics (_In_ int nIndex)
{
  return GetSystemMetrics_Original (nIndex);
}

void
SK_HookWinAPI (void)
{
  static bool hooked = false;

  if (hooked)
    return;

  hooked = true;

  // Initialize the Window Manager
  SK_WindowManager::getInstance ();

  SK_CreateDLLHook2 ( L"user32.dll", "SetWindowPos",
                        SetWindowPos_Detour,
             (LPVOID *)&SetWindowPos_Original );

  SK_CreateDLLHook2 ( L"user32.dll", "ClipCursor",
                        ClipCursor_Detour,
             (LPVOID *)&ClipCursor_Original );

  SK_CreateDLLHook2 ( L"user32.dll", "MoveWindow",
                        MoveWindow_Detour,
             (LPVOID *)&MoveWindow_Original );

  SK_CreateDLLHook2 ( L"user32.dll", "SetWindowLongA",
                        SetWindowLongA_Detour,
             (LPVOID *)&SetWindowLongA_Original );

  SK_CreateDLLHook2 ( L"user32.dll", "SetWindowLongW",
                        SetWindowLongW_Detour,
             (LPVOID *)&SetWindowLongW_Original );

  SK_CreateDLLHook2 ( L"user32.dll", "GetWindowRect",
                      GetWindowRect_Detour,
            (LPVOID*)&GetWindowRect_Original );

  SK_CreateDLLHook2 ( L"user32.dll", "GetClientRect",
                      GetClientRect_Detour,
            (LPVOID*)&GetClientRect_Original );

  SK_CreateDLLHook2 ( L"user32.dll", "AdjustWindowRect",
                        AdjustWindowRect_Detour,
             (LPVOID *)&AdjustWindowRect_Original );

  SK_CreateDLLHook2 ( L"user32.dll", "AdjustWindowRectEx",
                        AdjustWindowRectEx_Detour,
             (LPVOID *)&AdjustWindowRectEx_Original );

  SK_CreateDLLHook2 ( L"user32.dll",
                      "GetSystemMetrics",
                       GetSystemMetrics_Detour,
            (LPVOID *)&GetSystemMetrics_Original );

  SK_ApplyQueuedHooks ();
}
void
SK_Console::Start (void)
{
  // STUPID HACK UNTIL WE PROPERLY UNIFY SK AND TSFIX'S CONSOLE.
  if (GetModuleHandle (L"AgDrag.dll") || GetModuleHandle (L"PrettyPrinny.dll")) {
    bNoConsole = true;
    return;
  }

  SK_HookWinAPI ();

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

  TerminateThread (hMsgPump, 0);
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

  if (uiCommand == RID_INPUT) {
    // Block keyboard input to the game while the console is active
    if (SK_Console::getInstance ()->isVisible ()) {
      *pcbSize = 0;
      return 0;
    }

    // Block keyboard input to the game while it's in the background
    if (config.window.background_render && (! game_window.active)) {
      *pcbSize = 0;
      return 0;
    }
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

  // Block keyboard input to the game while it's in the background
  if (config.window.background_render && (! game_window.active))
    SK_ConsumeVKey (vKey);

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

  // Block keyboard input to the game while it's in the background
  if (config.window.background_render && (! game_window.active))
    return 0;

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
  if (hWnd != game_window.hWnd)
    return DefWindowProc (hWnd, uMsg, wParam, lParam);

  static bool last_active = game_window.active;

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


  if ((uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST) && game_window.needsCoordTransform ()) {
    POINT pt;

    pt.x = GET_X_LPARAM (lParam);
    pt.y = GET_Y_LPARAM (lParam);

    SK_CalcCursorPos (&pt);

    lParam = MAKELPARAM ((SHORT)pt.x, (SHORT)pt.y);
  }


  if (uMsg == WM_SIZE) {
    //if ( SK_GetCurrentRenderBackend ().api == SK_RenderAPI::OpenGL ) {
      SK_SetWindowResX (LOWORD (lParam));
      SK_SetWindowResY (HIWORD (lParam));
    //}
  }

#if 0
  if (config.window.borderless) {
    if (uMsg == WM_WINDOWPOSCHANGED || uMsg == WM_WINDOWPOSCHANGING)
      return 0;

    if (uMsg == WM_MOVE || uMsg == WM_MOVING)
      return 0;

    if (uMsg == WM_SIZE || uMsg == WM_SIZING)
      return 0;
  }
#endif


  if (config.input.cursor.manage)
  {
    //extern bool IsControllerPluggedIn (INT iJoyID);

   auto ActivateCursor = [](bool changed = false)->
    bool
     {
       bool was_active = last_mouse.cursor;

       if (! last_mouse.cursor) {
         if ((! SK_IsSteamOverlayActive ()) && game_window.active) {
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
         if ((! SK_IsSteamOverlayActive ()) && game_window.active) {
           while (ShowCursor (FALSE) >= -1) ;
           last_mouse.cursor = false;

           last_mouse.sampled = timeGetTime ();
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

    else if (uMsg == WM_TIMER && wParam == last_mouse.timer_id && (! SK_IsSteamOverlayActive ()) && game_window.active) {
      if (true)//IsControllerPluggedIn (config.input.gamepad_slot))
        DeactivateCursor ();

      else
        ActivateCursor ();
    }
  }

  last_active = game_window.active;

  auto ActivateWindow = [](bool active = false)->
  void
    {
      bool was_active = last_active;
      game_window.active = active;

      if (active && (! was_active)) {
        if (config.window.background_mute)
          SK_WindowManager::getInstance ()->muteGame (false);

        GetCursorPos_Original (&game_window.cursor_pos);

        if (config.window.background_render) {
          if (! game_window.cursor_visible) {
            while (ShowCursor (FALSE) >= 0)
              ;
          }

          ClipCursor (&game_window.cursor_clip);
        }

        if (config.window.confine_cursor) {
          //dll_log.Log (L"[Window Mgr] Confining Mouse Cursor");
          GetWindowRect_Original (game_window.hWnd, &game_window.rect);
          ClipCursor_Original    (&game_window.rect);
        }
      }

      else if ((! active) && was_active) {
        if (config.window.background_mute)
          SK_WindowManager::getInstance ()->muteGame (true);

        if (config.window.background_render) {
          game_window.cursor_visible =
            ShowCursor (TRUE) >= 1;

          while (ShowCursor (TRUE) < 0)
            ;

          ClipCursor   ( nullptr );
          SetCursorPos ( game_window.cursor_pos.x,
                         game_window.cursor_pos.y );
        }

        if (config.window.confine_cursor) {
          //dll_log.Log (L"[Window Mgr] Unconfining Mouse Cursor");
          ClipCursor_Original (nullptr);
        }
      }
   };

  // Ignore this event
  if (uMsg == WM_MOUSEACTIVATE && config.window.background_render) {
    if ((HWND)wParam == game_window.hWnd) {
      dll_log.Log (L"[Window Mgr] WM_MOUSEACTIVATE ==> Activate and Eat");

      ActivateWindow (true);

      return MA_ACTIVATEANDEAT;
    }

    ActivateWindow (false);

    dll_log.Log (L"[Window Mgr] WM_MOUSEACTIVATE ==> Activate");

    return MA_ACTIVATE;
  }

  // Allow the game to run in the background
  if (uMsg == WM_ACTIVATEAPP || uMsg == WM_ACTIVATE || uMsg == WM_NCACTIVATE /*|| uMsg == WM_MOUSEACTIVATE*/)
  {
    CallWindowProc (game_window.WndProc_Original, hWnd, uMsg, TRUE, (LPARAM)hWnd);

    if (uMsg == WM_NCACTIVATE) {
      if (wParam == TRUE && last_active == false) {
        //dll_log->Log (L"[Window Mgr] Application Activated (Non-Client)");
        ActivateWindow (true);
      }

      else if (wParam == FALSE && last_active == true) {
        //dll_log->Log (L"[Window Mgr] Application Deactivated (Non-Client)");
        ActivateWindow (false);

        // We must fully consume one of these messages or audio will stop playing
        //   when the game loses focus, so do not simply pass this through to the
        //     default window procedure.
        if (config.window.background_render) {
          return 0;
        }
      }
    }

    else if (uMsg == WM_ACTIVATEAPP || uMsg == WM_ACTIVATE) {
      switch (wParam)
      {
        case 1:  // WA_ACTIVE / TRUE
        case 2:  // WA_CLICKACTIVE
        default: // Unknown
        {
          if (last_active == false) {
            //dll_log->Log (L"[Window Mgr] Application Activated (WM_ACTIVATEAPP)");
            ActivateWindow (true);
          }
        } break;

        case 0: // WA_INACTIVE / FALSE
        {
          if (last_active == true) {
            //dll_log->Log (L"[Window Mgr] Application Deactivated (WM_ACTIVATEAPP)");
            ActivateWindow (false);

            if (config.window.background_render) {
              return 1;
            }
          }
        } break;
      }
    }
  }

#if 0
  // Block the menu key from messing with stuff
  if ( config.input.block_left_alt &&
       (uMsg == WM_SYSKEYDOWN || uMsg == WM_SYSKEYUP) ) {
    // Make an exception for Alt+Enter, for fullscreen mode toggle.
    //   F4 as well for exit
    if ( wParam != VK_RETURN && wParam != VK_F4 &&
         wParam != VK_TAB    && wParam != VK_PRINT )
      return DefWindowProc (hWnd, uMsg, wParam, lParam);
  }
#endif

  if (config.window.background_render) {
    if ((! game_window.active) && uMsg == WM_MOUSEMOVE) {
      GetCursorPos_Original (&game_window.cursor_pos);
    }
  }

  bool background_render =
    config.window.background_render && (! game_window.active);

  if ((uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN) && (! background_render)) {
    if (SK_Console::getInstance ()->KeyDown (wParam & 0xFF, lParam) && (uMsg != WM_SYSKEYDOWN))
      return CallWindowProc (game_window.WndProc_Original, hWnd, uMsg, wParam, lParam);
  }

  if ((uMsg == WM_KEYUP || uMsg == WM_SYSKEYUP) && (! background_render)) {
    if (SK_Console::getInstance ()->KeyUp (wParam & 0xFF, lParam) && (uMsg != WM_SYSKEYUP))
      return CallWindowProc (game_window.WndProc_Original, hWnd, uMsg, wParam, lParam);
  }

  // Block keyboard input to the game while the console is visible
  if (console_visible || (background_render && uMsg != WM_SYSKEYDOWN)) {
    if (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST)
      return DefWindowProc (hWnd, uMsg, wParam, lParam);

    if (uMsg >= WM_KEYFIRST && uMsg <= WM_KEYLAST)
      return DefWindowProc (hWnd, uMsg, wParam, lParam);

    // Block RAW Input
    if (console_visible && uMsg == WM_INPUT)
      return DefWindowProc (hWnd, uMsg, wParam, lParam);
  }

  return CallWindowProc (game_window.WndProc_Original, hWnd, uMsg, wParam, lParam);
}

#include <SpecialK/core.h>

typedef BOOL (WINAPI *RegisterRawInputDevices_pfn)(
  _In_ PCRAWINPUTDEVICE pRawInputDevices,
  _In_ UINT             uiNumDevices,
  _In_ UINT             cbSize
);

RegisterRawInputDevices_pfn RegisterRawInputDevices_Original = nullptr;

BOOL WINAPI RegisterRawInputDevices_Detour (
  _In_ PCRAWINPUTDEVICE pRawInputDevices,
  _In_ UINT             uiNumDevices,
  _In_ UINT             cbSize
)
{
  if (cbSize != sizeof RAWINPUTDEVICE) {
    dll_log.Log ( L"[ RawInput ] RegisterRawInputDevices has wrong "
                  L"structure size (%lu bytes), expected: %lu",
                    cbSize,
                      sizeof RAWINPUTDEVICE );

    return
      RegisterRawInputDevices_Original ( pRawInputDevices,
                                           uiNumDevices,
                                             cbSize );
  }

  RAWINPUTDEVICE* pDevices = new RAWINPUTDEVICE [uiNumDevices];

  // We need to continue receiving window messages for the console to work
  for (int i = 0; i < uiNumDevices; i++) {
    pDevices [i] = pRawInputDevices [i];
    pDevices [i].dwFlags &= ~(RIDEV_NOLEGACY | RIDEV_APPKEYS | RIDEV_REMOVE);
    pDevices [i].dwFlags &= ~RIDEV_CAPTUREMOUSE;
  }

  BOOL bRet =
    RegisterRawInputDevices_Original (pDevices, uiNumDevices, cbSize);

  delete [] pDevices;

  return bRet;
}

#include <SpecialK/command.h>

void
SK_InstallWindowHook (HWND hWnd)
{
  static bool installed = false;

  if (installed)
    return;

  installed = true;

  SK_CreateDLLHook2 ( L"user32.dll", "GetRawInputData",
                     GetRawInputData_Detour,
           (LPVOID*)&GetRawInputData_Original );

  SK_CreateDLLHook2 ( L"user32.dll", "GetKeyState",
                     GetKeyState_Detour,
           (LPVOID*)&GetKeyState_Original );

  SK_CreateDLLHook2 ( L"user32.dll", "GetCursorPos",
                     GetCursorPos_Detour,
           (LPVOID*)&GetCursorPos_Original );

  SK_CreateDLLHook2 ( L"user32.dll", "GetCursorInfo",
                     GetCursorInfo_Detour,
           (LPVOID*)&GetCursorInfo_Original );

  SK_CreateDLLHook2 ( L"user32.dll", "GetAsyncKeyState",
                     GetAsyncKeyState_Detour,
           (LPVOID*)&GetAsyncKeyState_Original );

  SK_CreateDLLHook2 ( L"user32.dll", "RegisterRawInputDevices",
                     RegisterRawInputDevices_Detour,
           (LPVOID*)&RegisterRawInputDevices_Original );

  MH_ApplyQueued ();

  game_window.hWnd = hWnd;

  game_window.WndProc_Original =
    (WNDPROC)GetWindowLongPtrW (game_window.hWnd, GWLP_WNDPROC);

  SetWindowLongPtrW ( game_window.hWnd,
                     GWLP_WNDPROC,
                       (LONG_PTR)SK_DetourWindowProc );

  game_window.style = GetWindowLongA (game_window.hWnd, GWL_STYLE);//0x90CA0000;

  if (! (game_window.style & WS_VISIBLE)) {
    game_window.style |= (WS_VISIBLE | WS_MINIMIZEBOX);
    //game_window.style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
  }

  GetCursorPos_Original  (&game_window.cursor_pos);

  SK_AdjustWindow            ();
  SK_RealizeForegroundWindow (game_window.hWnd);

  SK_ICommandProcessor* cmd =
    SK_GetCommandProcessor ();

  cmd->AddVariable ("Cursor.Manage",       SK_CreateVar (SK_IVariable::Boolean, (bool *)&config.input.cursor.manage));
  cmd->AddVariable ("Cursor.Timeout",      SK_CreateVar (SK_IVariable::Int,     (int  *)&config.input.cursor.timeout));
  cmd->AddVariable ("Cursor.KeysActivate", SK_CreateVar (SK_IVariable::Boolean, (bool *)&config.input.cursor.keys_activate));

  cmd->AddVariable ("Window.BackgroundRender", SK_CreateVar (SK_IVariable::Boolean, (bool *)&config.window.background_render));
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

// Pending removal, because we now hook the window's message procedure
//
unsigned int
__stdcall
SK_Console::MessagePump (LPVOID hook_ptr)
{
  hooks_t* pHooks = (hooks_t *)hook_ptr;

  char* text = SK_Console::getInstance ()->text;

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
      Sleep (333);
      //hWndForeground = GetForegroundWindow ();
      continue;
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

    if (SK_GetFramesDrawn () > 1) {
      hWndRender = hWndForeground;
      break;
    }
  }

  DWORD dwNow = timeGetTime ();

  dll_log.Log ( L"[CmdConsole]  # Found window in %03.01f seconds, "
                L"installing window hook...",
                  (float)(dwNow - dwTime) / 1000.0f );

  SK_InstallWindowHook (hWndForeground);

#if 0
  while (! (pHooks->keyboard_ll = SetWindowsHookEx ( WH_KEYBOARD_LL,
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
#endif

  CloseHandle (GetCurrentThread ());

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

int
SK_HandleConsoleKey (bool keyDown, BYTE vkCode, LPARAM lParam)
{
  bool&                          visible        = SK_Console::getInstance ()->visible;
  char*                          text           = SK_Console::getInstance ()->text;
  BYTE*                          keys_          = SK_Console::getInstance ()->keys_;
  SK_Console::command_history_t& commands       = SK_Console::getInstance ()->commands;
  bool&                          command_issued = SK_Console::getInstance ()->command_issued;
  std::string&                   result_str     = SK_Console::getInstance ()->result_str;

  if ((! SK_IsSteamOverlayActive ())) {
    //BYTE    vkCode   = LOWORD (wParam) & 0xFF;
    BYTE    scanCode = HIWORD (lParam) & 0x7F;
    SHORT   repeated = LOWORD (lParam);
    bool    keyDown  = ! (lParam & 0x80000000UL);

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
      vkCode = VK_SHIFT;

      if (keyDown) keys_ [vkCode] = 0x81; else keys_ [vkCode] = 0x00;
    }

    else if (vkCode == VK_MENU || vkCode == VK_LMENU || vkCode == VK_RMENU) {
      vkCode = VK_MENU;

      if (keyDown) keys_ [vkCode] = 0x81; else keys_ [vkCode] = 0x00;
    }

    //else if ((! repeated) && vkCode == VK_CAPITAL) {
      //if (keyDown) if (keys_ [VK_CAPITAL] == 0x00) keys_ [VK_CAPITAL] = 0x81; else keys_ [VK_CAPITAL] = 0x00;
    //}

    else if (vkCode == VK_CONTROL || vkCode == VK_LCONTROL || vkCode == VK_RCONTROL) {
      vkCode = VK_CONTROL;

      if (keyDown) keys_ [vkCode] = 0x81; else keys_ [vkCode] = 0x00;
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

      if (new_press) {
        SK_PluginKeyPress (keys_ [VK_CONTROL], keys_ [VK_SHIFT], keys_ [VK_MENU], vkCode);

        if (keys_ [VK_CONTROL] && keys_ [VK_SHIFT] && keys_ [VK_TAB] && vkCode == VK_TAB) {
          visible = ! visible;

          // This will pause/unpause the game
          SK::SteamAPI::SetOverlayState (visible);

          return 0;
        }
      }

      // Don't print the tab character, it's pretty useless.
      if (visible && vkCode != VK_TAB) {
        keys_ [VK_CAPITAL] = GetKeyState_Original (VK_CAPITAL) & 0xFFUL;

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

  if (visible)
    return 0;

  return 1;
}

int
SK_Console::KeyUp (BYTE vkCode, LPARAM lParam)
{
  return SK_HandleConsoleKey (false, vkCode, lParam);
}

int
SK_Console::KeyDown (BYTE vkCode, LPARAM lParam)
{
  return SK_HandleConsoleKey (true, vkCode, lParam);
}

LRESULT
CALLBACK
SK_Console::KeyboardProc (int nCode, WPARAM wParam, LPARAM lParam)
{
#if 0
  if (nCode >= 0)
  {
    switch (wParam) {
      case WM_KEYDOWN:
      case WM_SYSKEYDOWN:
      {
        LPKBDLLHOOKSTRUCT lpKbdLL = (LPKBDLLHOOKSTRUCT)lParam;
        if (! (lpKbdLL->flags & 0x40)) {
            BYTE    scanCode = HIWORD (lParam) & 0x7F;
            SHORT   repeated = LOWORD (lParam);
            bool    keyDown  = ! (lParam & 0x80000000UL);
          return SK_Console::getInstance ()->KeyDown (lpKbdLL->vkCode, (1 << 30) | (1 << 29));
        }
      } break;

      case WM_KEYUP:
      case WM_SYSKEYUP:
      {
        LPKBDLLHOOKSTRUCT lpKbdLL = (LPKBDLLHOOKSTRUCT)lParam;
        if ((lpKbdLL->flags & 0x40)) {
            BYTE    scanCode = HIWORD (lParam) & 0x7F;
            SHORT   repeated = LOWORD (lParam);
            bool    keyDown  = ! (lParam & 0x80000000UL);
          return SK_Console::getInstance ()->KeyUp (lpKbdLL->vkCode, 0x4000 | (1 << 29) | (1 << 30));
        }
      } break;
    }
  }
#endif

  return CallNextHookEx (SK_Console::getInstance ()->hooks.keyboard_ll, nCode, wParam, lParam);
}

void
SK_DrawConsole (void)
{
  if (bNoConsole)
    return;

  SK_Console* pConsole = SK_Console::getInstance ();
  pConsole->Draw ();
}

BOOL
__stdcall
SK_IsConsoleVisible (void)
{
  return SK_Console::getInstance ()->isVisible ();
}

SK_Console* SK_Console::pConsole      = nullptr;