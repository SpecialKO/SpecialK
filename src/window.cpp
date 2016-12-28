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

#include <Windows.h>
#include <SpecialK/window.h>

#include <SpecialK/hooks.h>
#include <SpecialK/core.h>
#include <SpecialK/console.h>
#include <SpecialK/sound.h>

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

#define SK_BORDERLESS (WS_VISIBLE | WS_POPUP | WS_MINIMIZEBOX)

#undef GetWindowLong
#undef GetWindowLongPtr
#undef SetWindowLong
#undef SetWindowLongPtr

LRESULT
CALLBACK
SK_DetourWindowProc ( _In_  HWND   hWnd,
                      _In_  UINT   uMsg,
                      _In_  WPARAM wParam,
                      _In_  LPARAM lParam );

BOOL
WINAPI
SetWindowPlacement_Detour (
  _In_       HWND             hWnd,
  _In_ const WINDOWPLACEMENT *lpwndpl
);

BOOL
WINAPI
SetWindowPos_Detour(
    _In_     HWND hWnd,
    _In_opt_ HWND hWndInsertAfter,
    _In_     int  X,
    _In_     int  Y,
    _In_     int  cx,
    _In_     int  cy,
    _In_     UINT uFlags);

ClipCursor_pfn         ClipCursor_Original         = nullptr;
SetWindowPos_pfn       SetWindowPos_Original       = nullptr;
SetWindowPlacement_pfn SetWindowPlacement_Original = nullptr;
MoveWindow_pfn         MoveWindow_Original         = nullptr;
SetWindowLong_pfn      SetWindowLongW_Original     = nullptr;
SetWindowLong_pfn      SetWindowLongA_Original     = nullptr;
GetWindowLong_pfn      GetWindowLongW_Original     = nullptr;
GetWindowLong_pfn      GetWindowLongA_Original     = nullptr;
SetWindowLongPtr_pfn   SetWindowLongPtrW_Original  = nullptr;
SetWindowLongPtr_pfn   SetWindowLongPtrA_Original  = nullptr;
GetWindowLongPtr_pfn   GetWindowLongPtrW_Original  = nullptr;
GetWindowLongPtr_pfn   GetWindowLongPtrA_Original  = nullptr;
AdjustWindowRect_pfn   AdjustWindowRect_Original   = nullptr;
AdjustWindowRectEx_pfn AdjustWindowRectEx_Original = nullptr;

GetSystemMetrics_pfn   GetSystemMetrics_Original   = nullptr;
GetCursorPos_pfn       GetCursorPos_Original       = nullptr;
GetCursorInfo_pfn      GetCursorInfo_Original      = nullptr;

GetWindowRect_pfn GetWindowRect_Original = nullptr;
GetClientRect_pfn GetClientRect_Original = nullptr;

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

struct sk_window_s {
  bool      unicode          = false;

  HWND      hWnd             = 0x00;
  WNDPROC   WndProc_Original = nullptr;
  WNDPROC   RawProc_Original = nullptr;

  bool      active           = true;

  struct {
    struct {
      LONG width;
      LONG height;
    } framebuffer;

    struct {
      RECT client;
      RECT window;
    };

    LONG style;
    LONG style_ex;
  } game, actual;

#if 0
  LONG      style            = 0x00;
  LONG      style_ex         = 0x00;
#endif

//  RECT      rect        { 0, 0,
//                          0, 0 };
//  RECT      game_rect   { 0, 0,
//                          0, 0 };

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

#if 0
  LONG      render_x         = 0;
  LONG      render_y         = 0;

  LONG      game_x           = 0; // Resolution game thinks it's running at
  LONG      game_y           = 0; // Resolution game thinks it's running at
#endif

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
} game_window;

bool override_window_rects = false;

void SK_AdjustBorder (void);
void SK_AdjustWindow (void);

// TODO: Refactor that structure above into this class ;)
//
//  Also, combine a few of the input manager settings with window manager
class SK_WindowManager : public SK_IVariableListener
{
public:
  static bool StyleHasBorder (LONG style) {
    return ( style == 0x00         ||
             style & WS_BORDER     ||
             style & WS_THICKFRAME ||
             style & WS_DLGFRAME );
  }

  virtual bool OnVarChange (SK_IVariable* var, void* val)
  {
    if (var == confine_cursor_)
    {
      if (val != nullptr)
      {
        config.window.confine_cursor = *(bool *)val;

        if (! config.window.confine_cursor)
          ClipCursor_Original (nullptr);
        else {
          GetWindowRect_Original (game_window.hWnd, &game_window.actual.window);
          ClipCursor_Original    (&game_window.actual.window);
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

        else if (var == borderless_) {
          config.window.borderless = *(bool *)val;

          if ( state_.borderless.set (config.window.borderless) !=
               config.window.borderless ) {
            SK_AdjustBorder ();
          }

          return true;
        }

        else if (var == fullscreen_) {
          if (config.window.borderless) {
            config.window.fullscreen = *(bool *)val;

            if ( state_.fullscreen.set (config.window.fullscreen) !=
                 config.window.fullscreen ) {

              // Restore the game's normal resolution
              if (! config.window.fullscreen) {
                game_window.actual.client = game_window.game.client;
                game_window.actual.window = game_window.game.window;
              }
            }
          } else {
            return true;
          }
        }

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

    static_rects_ =
      SK_CreateVar (SK_IVariable::Boolean, &override_window_rects);

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

    cmd->AddVariable (
      "Window.StaticRects",
        static_rects_ );
  };

  static SK_WindowManager* getInstance (void)
  {
    if (pWindowManager == nullptr)
      pWindowManager = new SK_WindowManager ();

    return pWindowManager;
  }

  void muteGame (bool bMute)
  {
    SK_SetGameMute (bMute);
  }

  void finishFullscreen (bool fullscreen) {
    state_.fullscreen.actual = fullscreen;
  }

  void finishBorderless (bool borderless) {
    state_.borderless.actual = borderless;
  }

  bool makeFullscreen (bool fullscreen) {
    state_.fullscreen.set (fullscreen);
    return state_.fullscreen.get ();
  }

  bool makeBorderless (bool borderless) {
    state_.borderless.set (borderless);
    return state_.borderless.get ();
  }

  bool isChanging (void) {
    return state_.changing ();
  }

protected:
  struct state_s {
    struct value_s {
      bool set (bool val) { requested = val; return get (); };
      bool get (void)     { return actual;                  };

      bool requested = false;
      bool actual    = false;
    } borderless,
      fullscreen;

    bool changing (void) {
      return (borderless.requested != borderless.actual ||
              fullscreen.requested != fullscreen.actual);
    };
  } state_;

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

  SK_IVariable* static_rects_; // Fake the game into thinking the
                               //   client rectangle has not changed.

                               // This solves display scaling problems in Fallout 4 and
                               //   Skyrim...

  char override_res [32];

private:
  static SK_WindowManager* pWindowManager;
};

SK_WindowManager* SK_WindowManager::pWindowManager = nullptr;


void
SK_SetWindowResX (LONG x)
{
  game_window.game.client.right = game_window.game.client.left + x;
  //game_window.render_x = x;
}

void
SK_SetWindowResY (LONG y)
{
  game_window.game.client.bottom = game_window.game.client.top + y;
  //game_window.render_y = y;
}

LPRECT
SK_GetGameRect (void)
{
  return &game_window.game.client;
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

  lpPoint->x -= ( (LONG)game_window.actual.window.left );
  lpPoint->y -= ( (LONG)game_window.actual.window.top  );

  in.width   = (float)(game_window.actual.client.right  - game_window.actual.client.left);
  in.height  = (float)(game_window.actual.client.bottom - game_window.actual.client.top);

  out.width  = (float)(game_window.game.client.right  - game_window.game.client.left);
  out.height = (float)(game_window.game.client.bottom - game_window.game.client.top);

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
                        game_window.game.window.left     +
                        game_window.game.client.left );
  lpPoint->y -= ( (LONG)game_window.coord_remap.offset.y +
                        game_window.game.window.top      +
                        game_window.game.client.top );

  in.width   = (float)(game_window.game.client.right  - game_window.game.client.left);
  in.height  = (float)(game_window.game.client.bottom - game_window.game.client.top);

  out.width  = (float)(game_window.actual.client.right  - game_window.actual.client.left);
  out.height = (float)(game_window.actual.client.bottom - game_window.actual.client.top);

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

    top_left.y = game_window.actual.window.top;
    top_left.x = game_window.actual.window.left;

    bottom_right.y = game_window.actual.window.bottom;
    bottom_right.x = game_window.actual.window.right;

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
  POINT mouse = { 0, 0 };

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

  config.window.offset.x.absolute -= (game_window.actual.window.right  - game_window.actual.window.left) / 2;
  config.window.offset.y.absolute -= (game_window.actual.window.bottom - game_window.actual.window.top)  / 2;

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
  if (hWnd != game_window.hWnd) {
    return MoveWindow_Original ( hWnd,
                                   X, Y,
                                     nWidth, nHeight,
                                       bRedraw );
  }

  dll_log.Log (L"[Window Mgr][!] MoveWindow (...)");

  game_window.game.window.left = X;
  game_window.game.window.top  = Y;

  game_window.game.window.right  = X + nWidth;
  game_window.game.window.bottom = Y + nHeight;

  BOOL bRet = FALSE;

  bRet = MoveWindow_Original (hWnd, X, Y, nWidth, nHeight, bRedraw);

  SK_AdjustWindow ();

  return bRet;
}

BOOL
WINAPI
SetWindowPlacement_Detour(
  _In_       HWND             hWnd,
  _In_ const WINDOWPLACEMENT *lpwndpl)
{
  dll_log.Log (L"[Window Mgr][!] SetWindowPlacement (...)");

  return TRUE;
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
  bool game_changed = false;

  if (hWnd == game_window.hWnd) {
    dll_log.Log ( L"[Window Mgr][!] SetWindowPos (...)");

    if (! ((uFlags & SWP_NOMOVE) || (uFlags & SWP_NOSIZE))) {
      game_changed = true;
    }
  }

  BOOL bRet = 
    SetWindowPos_Original ( hWnd, hWndInsertAfter,
                              X, Y,
                                cx, cy,
                                  uFlags );

  if (bRet && game_changed) {
    if (uFlags & SWP_ASYNCWINDOWPOS)
      dll_log.Log (L"[Window Mgr] OH NO! (Async Window Pos)");

    GetWindowRect_Original (hWnd, &game_window.game.window);
    GetClientRect_Original (hWnd, &game_window.game.client);

    GetWindowRect_Original (hWnd, &game_window.actual.window);
    GetClientRect_Original (hWnd, &game_window.actual.client);

    game_window.updateDims ();

    if (config.window.confine_cursor) {
      ClipCursor_Original (&game_window.actual.window);
    }
  }

  return bRet;
}

static
BOOL
WINAPI
GetClientRect_Detour (
    _In_  HWND   hWnd,
    _Out_ LPRECT lpRect )
{
  if ( config.window.borderless &&
       hWnd == game_window.hWnd &&
       override_window_rects )
  {
    *lpRect = game_window.game.client;

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
  if ( config.window.borderless &&
       hWnd == game_window.hWnd && 
       override_window_rects )
  {
    *lpRect = game_window.game.window;

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
  dll_log.Log ( L"[Window Mgr] AdjustWindowRect ( {%4lu,%4lu / %4lu,%4lu}, 0x%04X, %lu )",
                  lpRect->left, lpRect->top,
                    lpRect->right, lpRect->bottom,
                      dwStyle, bMenu );

  return AdjustWindowRect_Original (lpRect, dwStyle, bMenu);
}

BOOL
WINAPI
AdjustWindowRectEx_Detour (
    _Inout_ LPRECT lpRect,
    _In_    DWORD  dwStyle,
    _In_    BOOL   bMenu,
    _In_    DWORD  dwExStyle )
{
  dll_log.Log ( L"[Window Mgr] AdjustWindowRectEx ( {%4lu,%4lu / %4lu,%4lu}, 0x%04X, %lu, 0x%04X )",
                  lpRect->left, lpRect->top,
                    lpRect->right, lpRect->bottom,
                      dwStyle, bMenu,
                        dwExStyle );

  return AdjustWindowRectEx_Original (lpRect, dwStyle, bMenu, dwExStyle);
}

// Convenience function since there are so damn many
//   variants of these functions that need to be hooked.
__forceinline
LONG
WINAPI
SetWindowLong_Marshall (
  _In_ SetWindowLong_pfn pOrigFunc,
  _In_ HWND              hWnd,
  _In_ int               nIndex,
  _In_ LONG              dwNewLong )
{
  // Override window styles
  if (hWnd == game_window.hWnd && nIndex == GWL_STYLE)
  {
    // For proper return behavior
    game_window.game.style = (DWORD)dwNewLong | WS_VISIBLE;

    if (config.window.borderless) {
      game_window.actual.style = SK_BORDERLESS;
      pOrigFunc (hWnd, nIndex, SK_BORDERLESS);
      return dwNewLong;
    }

    game_window.actual.style = game_window.game.style;
  }

  return pOrigFunc (hWnd, nIndex, dwNewLong);
}

DECLSPEC_NOINLINE
LONG
WINAPI
SetWindowLongA_Detour (
  _In_ HWND hWnd,
  _In_ int  nIndex,
  _In_ LONG dwNewLong
)
{
  return SetWindowLong_Marshall (
      SetWindowLongA_Original,
        hWnd,
          nIndex,
            dwNewLong
  );
}

DECLSPEC_NOINLINE
LONG
WINAPI
SetWindowLongW_Detour (
  _In_ HWND hWnd,
  _In_ int  nIndex,
  _In_ LONG dwNewLong
)
{
  return SetWindowLong_Marshall (
      SetWindowLongW_Original,
        hWnd,
          nIndex,
            dwNewLong
  );
}

// Convenience function since there are so damn many
//   variants of these functions that need to be hooked.
__forceinline
LONG
WINAPI
GetWindowLong_Marshall (
  _In_ GetWindowLong_pfn pOrigFunc,
  _In_ HWND              hWnd,
  _In_ int               nIndex
)
{
  if (hWnd == game_window.hWnd && nIndex == GWL_STYLE)
  {
    if (config.window.borderless)
      return game_window.game.style;
    //if (config.window.borderless)
      //return game_window.style;
  }

  return pOrigFunc (hWnd, nIndex);
}

DECLSPEC_NOINLINE
LONG
WINAPI
GetWindowLongA_Detour (
  _In_ HWND hWnd,
  _In_ int  nIndex
)
{
  return GetWindowLong_Marshall (
      GetWindowLongA_Original,
        hWnd,
          nIndex
  );
}

DECLSPEC_NOINLINE
LONG
WINAPI
GetWindowLongW_Detour (
  _In_ HWND hWnd,
  _In_ int  nIndex
)
{
  return GetWindowLong_Marshall (
      GetWindowLongW_Original,
        hWnd,
          nIndex
  );
}

// Convenience function since there are so damn many
//   variants of these functions that need to be hooked.
__forceinline
LONG_PTR
WINAPI
SetWindowLongPtr_Marshall (
  _In_ SetWindowLongPtr_pfn pOrigFunc,
  _In_ HWND                 hWnd,
  _In_ int                  nIndex,
  _In_ LONG_PTR             dwNewLong
)
{
  // Override window styles
  if (hWnd == game_window.hWnd && nIndex == GWL_STYLE)
  {
    // For proper return behavior
    game_window.game.style = (DWORD)dwNewLong | WS_VISIBLE;

    if (config.window.borderless) {
      game_window.actual.style = SK_BORDERLESS;
      pOrigFunc (hWnd, nIndex, SK_BORDERLESS);
      return dwNewLong;
    }

    game_window.actual.style = game_window.game.style;
  }

  return pOrigFunc (hWnd, nIndex, dwNewLong);
}

DECLSPEC_NOINLINE
LONG_PTR
WINAPI
SetWindowLongPtrA_Detour (
  _In_ HWND     hWnd,
  _In_ int      nIndex,
  _In_ LONG_PTR dwNewLong
)
{
  return SetWindowLongPtr_Marshall (
    SetWindowLongPtrA_Original,
      hWnd,
        nIndex,
          dwNewLong
  );
}

DECLSPEC_NOINLINE
LONG_PTR
WINAPI
SetWindowLongPtrW_Detour (
  _In_ HWND     hWnd,
  _In_ int      nIndex,
  _In_ LONG_PTR dwNewLong
)
{
  return SetWindowLongPtr_Marshall (
    SetWindowLongPtrW_Original,
      hWnd,
        nIndex,
          dwNewLong
  );
}

// Convenience function since there are so damn many
//   variants of these functions that need to be hooked.
__forceinline
LONG_PTR
WINAPI
GetWindowLongPtr_Marshall (
  _In_ GetWindowLongPtr_pfn pOrigFunc,
  _In_ HWND                 hWnd,
  _In_ int                  nIndex
)
{
  if (hWnd == game_window.hWnd && nIndex == GWL_STYLE)
  {
    if (config.window.borderless)
      return ((LONG_PTR)game_window.game.style) & 0xFFFFFFFF;
  }

  return pOrigFunc (hWnd, nIndex);
}

DECLSPEC_NOINLINE
LONG_PTR
WINAPI
GetWindowLongPtrA_Detour (
  _In_ HWND     hWnd,
  _In_ int      nIndex,
  _In_ LONG_PTR dwNewLong
)
{
  return GetWindowLongPtr_Marshall (
    GetWindowLongPtrA_Original,
      hWnd,
        nIndex
  );
}

DECLSPEC_NOINLINE
LONG_PTR
WINAPI
GetWindowLongPtrW_Detour (
  _In_ HWND     hWnd,
  _In_ int      nIndex,
  _In_ LONG_PTR dwNewLong
)
{
  return GetWindowLongPtr_Marshall (
    GetWindowLongPtrW_Original,
      hWnd,
        nIndex
  );
}

void
SK_AdjustBorder (void)
{
  if (game_window.GetWindowLongPtr == nullptr)
    return;

  UINT async = IsGUIThread (FALSE) ? SWP_ASYNCWINDOWPOS : 0x00;

  game_window.actual.style =
    config.window.borderless ?
      SK_BORDERLESS :
        game_window.game.style;

  game_window.SetWindowLongPtr ( game_window.hWnd,
                                  GWL_STYLE,
                                    game_window.actual.style );

  SetWindowPos_Original ( game_window.hWnd,
                  HWND_TOP,
                    0, 0,
                      0, 0,
                        SWP_NOMOVE   | SWP_NOSIZE       | 
                        SWP_NOZORDER | SWP_FRAMECHANGED |
                        SWP_NOSENDCHANGING | SWP_NOREPOSITION | SWP_NOACTIVATE | async );


  RECT new_window = game_window.game.client;

  new_window.left += game_window.game.window.left;
  new_window.top  += game_window.game.window.top;

  AdjustWindowRect_Original (&new_window, game_window.actual.style, FALSE);

  SetWindowPos_Original ( game_window.hWnd,
                  HWND_TOP,
                    new_window.left, new_window.top,
                      new_window.right  - new_window.left,
                      new_window.bottom - new_window.top,
                        SWP_NOSENDCHANGING | SWP_NOREPOSITION | SWP_NOACTIVATE | async );

  GetWindowRect_Original (game_window.hWnd, &game_window.actual.window);
  GetClientRect_Original (game_window.hWnd, &game_window.actual.client);
}

void
SK_AdjustWindow (void)
{
  if (game_window.GetWindowLongPtr == nullptr)
    return;

  UINT async = IsGUIThread (FALSE) ? SWP_ASYNCWINDOWPOS : 0x00;

  HMONITOR hMonitor =
    MonitorFromWindow ( game_window.hWnd,
                          MONITOR_DEFAULTTONEAREST );

  MONITORINFO mi = { 0 };

  mi.cbSize      = sizeof (mi);

  GetMonitorInfo (hMonitor, &mi);

  if (config.window.borderless && config.window.fullscreen) {
    //dll_log->Log (L"BorderManager::AdjustWindow - Fullscreen");
    SetWindowPos_Original ( game_window.hWnd,
                              HWND_TOP,
                                mi.rcMonitor.left,
                                mi.rcMonitor.top,
                                  mi.rcMonitor.right  - mi.rcMonitor.left,
                                  mi.rcMonitor.bottom - mi.rcMonitor.top,
                                    SWP_NOSENDCHANGING | SWP_NOREPOSITION | SWP_NOACTIVATE | async );

    dll_log.Log ( L"[Border Mgr] FULLSCREEN => {Left: %li, Top: %li} - (WxH: %lix%li)",
                    mi.rcMonitor.left, mi.rcMonitor.top,
                      mi.rcMonitor.right - mi.rcMonitor.left,
                        mi.rcMonitor.bottom - mi.rcMonitor.top );

    game_window.actual.window = mi.rcMonitor;
    game_window.actual.client = mi.rcMonitor;

    game_window.updateDims ();
  }

  else
  {
    //dll_log->Log (L"BorderManager::AdjustWindow - Windowed");

    // Adjust the desktop resolution to make room for window decorations
    //   if the game window were maximized.
    if (! config.window.borderless) {
      AdjustWindowRect_Original (
        &mi.rcWork,
          game_window.actual.style,
            FALSE
      );
    }

    LONG mon_width     = mi.rcWork.right     - mi.rcWork.left;
    LONG mon_height    = mi.rcWork.bottom    - mi.rcWork.top;

    LONG real_width    = mi.rcMonitor.right  - mi.rcMonitor.left;
    LONG real_height   = mi.rcMonitor.bottom - mi.rcMonitor.top;

    LONG render_width  = game_window.game.client.right  - game_window.game.client.left;
    LONG render_height = game_window.game.client.bottom - game_window.game.client.top;

    if ((! config.window.res.override.isZero ())) {
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
      game_window.actual.window.left   = 0;
      game_window.actual.window.top    = 0;
      game_window.actual.window.right  = win_width;
      game_window.actual.window.bottom = win_height;
    } else {
      game_window.actual.window.left   = game_window.game.window.left;
      game_window.actual.window.top    = game_window.game.window.top;
    }

    int x_offset = ( config.window.offset.x.percent != 0.0f ? 
                 (int)( config.window.offset.x.percent *
                         (mi.rcWork.right - mi.rcWork.left) ) :
                       config.window.offset.x.absolute );

    int y_offset = ( config.window.offset.y.percent != 0.0f ? 
                 (int)( config.window.offset.y.percent *
                         (mi.rcWork.bottom - mi.rcWork.top) ) :
                       config.window.offset.y.absolute );

    if (x_offset > 0)
      game_window.actual.window.left = mi.rcWork.left  + x_offset - 1;
    else if (x_offset < 0)
      game_window.actual.window.right = mi.rcWork.right + x_offset + 1;


    if (y_offset > 0)
      game_window.actual.window.top    = mi.rcWork.top    + y_offset - 1;
    else if (y_offset < 0)
      game_window.actual.window.bottom = mi.rcWork.bottom + y_offset + 1;


    if (config.window.center) {
      //dll_log->Log ( L"[Window Mgr] Center --> (%li,%li)",
      //                 mi.rcWork.right - mi.rcWork.left,
      //                   mi.rcWork.bottom - mi.rcWork.top );

      if (x_offset < 0) {
        game_window.actual.window.left  -= (win_width / 2);
        game_window.actual.window.right -= (win_width / 2);
      }

      if (y_offset < 0) {
        game_window.actual.window.top    -= (win_height / 2);
        game_window.actual.window.bottom -= (win_height / 2);
      }

      game_window.actual.window.left += std::max (0L, (mon_width  - win_width)  / 2);
      game_window.actual.window.top  += std::max (0L, (mon_height - win_height) / 2);
    }


    if (x_offset >= 0)
      game_window.actual.window.right = game_window.actual.window.left  + win_width;
    else
      game_window.actual.window.left  = game_window.actual.window.right - win_width;


    if (y_offset >= 0)
      game_window.actual.window.bottom = game_window.actual.window.top    + win_height;
    else
      game_window.actual.window.top    = game_window.actual.window.bottom - win_height;

    AdjustWindowRect_Original (&game_window.actual.window, game_window.actual.style, FALSE);


    //
    // Compensate for scenarios where the window is partially offscreen
    //
    int push_right = 0;

    if (game_window.actual.window.left < mi.rcWork.left)
      push_right = mi.rcWork.left - game_window.actual.window.left;
    else if (game_window.actual.window.right > mi.rcWork.right)
      push_right = (mi.rcWork.right - game_window.actual.window.right);

    game_window.actual.window.left  += push_right;
    game_window.actual.window.right += push_right;

    int push_down = 0;

    if ((game_window.actual.window.top - game_window.actual.client.top) < 0)
      push_down = 0 - (game_window.actual.window.top - game_window.actual.client.top);
    else if (game_window.actual.window.bottom + game_window.actual.client.top > mi.rcWork.bottom)
      push_down = mi.rcWork.bottom - (game_window.actual.window.bottom + game_window.actual.client.top);

    game_window.actual.window.top    += push_down;
    game_window.actual.window.bottom += push_down;

    SetWindowPos_Original ( game_window.hWnd,
                              HWND_TOP,
                                game_window.actual.window.left,
                                game_window.actual.window.top,
                                  game_window.actual.window.right  - game_window.actual.window.left,
                                  game_window.actual.window.bottom - game_window.actual.window.top,
                                    SWP_NOSENDCHANGING | SWP_NOREPOSITION | SWP_NOACTIVATE | async );

    wchar_t wszBorderDesc [128] = { L'\0' };

    bool has_border = SK_WindowManager::StyleHasBorder (game_window.actual.style);

    // Summarize the border
    if (SK_WindowManager::StyleHasBorder (game_window.actual.style)) {
      _swprintf ( wszBorderDesc,
                   L"(Frame = %lupx x %lupx, Title = %lupx)",
                     2 * SK_GetSystemMetrics (SM_CXDLGFRAME),
                     2 * SK_GetSystemMetrics (SM_CYDLGFRAME),
                         SK_GetSystemMetrics (SM_CYCAPTION) );
    }

    dll_log.Log ( L"[Border Mgr] WINDOW => {Left: %li, Top: %li} - (WxH: %lix%li) - { Border: %s }",
                    game_window.actual.window.left, game_window.actual.window.top,
                       game_window.actual.window.right - game_window.actual.window.left,
                         game_window.actual.window.bottom - game_window.actual.window.top,
                    (! has_border) ? L"None" : wszBorderDesc );
  }

  GetWindowRect_Original (game_window.hWnd, &game_window.actual.window);
  GetClientRect_Original (game_window.hWnd, &game_window.actual.client);

  game_window.updateDims ();

  if (config.window.confine_cursor)
    ClipCursor_Original (&game_window.actual.window);
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
    if (nIndex == SM_CXPADDEDBORDER)
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
  if (! GetSystemMetrics_Original)
    SK_HookWinAPI ();

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

  SK_CreateDLLHook2 ( L"user32.dll", "SetWindowPlacement",
                        SetWindowPlacement_Detour,
             (LPVOID *)&SetWindowPlacement_Original );

  SK_CreateDLLHook2 ( L"user32.dll", "ClipCursor",
                        ClipCursor_Detour,
             (LPVOID *)&ClipCursor_Original );

  SK_CreateDLLHook2 ( L"user32.dll", "MoveWindow",
                        MoveWindow_Detour,
             (LPVOID *)&MoveWindow_Original );

// These functions are dispatched through the Ptr version, so
//   hooking them in the 64-bit version would be a bad idea.
#ifndef _WIN64
  SK_CreateDLLHook2 ( L"user32.dll", "SetWindowLongA",
                        SetWindowLongA_Detour,
             (LPVOID *)&SetWindowLongA_Original );

  SK_CreateDLLHook2 ( L"user32.dll", "SetWindowLongW",
                        SetWindowLongW_Detour,
             (LPVOID *)&SetWindowLongW_Original );

  SK_CreateDLLHook2 ( L"user32.dll", "GetWindowLongA",
                        GetWindowLongA_Detour,
             (LPVOID *)&GetWindowLongA_Original );

  SK_CreateDLLHook2 ( L"user32.dll", "GetWindowLongW",
                        GetWindowLongW_Detour,
             (LPVOID *)&GetWindowLongW_Original );
#endif

#ifdef _WIN64
  SK_CreateDLLHook2 ( L"user32.dll", "SetWindowLongPtrA",
                        SetWindowLongPtrA_Detour,
             (LPVOID *)&SetWindowLongPtrA_Original );

  SK_CreateDLLHook2 ( L"user32.dll", "SetWindowLongPtrW",
                        SetWindowLongPtrW_Detour,
             (LPVOID *)&SetWindowLongPtrW_Original );

  SK_CreateDLLHook2 ( L"user32.dll", "GetWindowLongPtrA",
                        GetWindowLongPtrA_Detour,
             (LPVOID *)&GetWindowLongPtrA_Original );

  SK_CreateDLLHook2 ( L"user32.dll", "GetWindowLongPtrW",
                        GetWindowLongPtrW_Detour,
             (LPVOID *)&GetWindowLongPtrW_Original );
#endif

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

#ifndef _WIN64
  SetWindowLongPtrA_Original = SetWindowLongA_Original;
  SetWindowLongPtrW_Original = SetWindowLongW_Original;
#else
#endif
}


GetKeyState_pfn             GetKeyState_Original             = nullptr;
GetAsyncKeyState_pfn        GetAsyncKeyState_Original        = nullptr;
GetRawInputData_pfn         GetRawInputData_Original         = nullptr;
RegisterRawInputDevices_pfn RegisterRawInputDevices_Original = nullptr;

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

bool hooked = false;

__declspec (noinline)
LRESULT
CALLBACK
SK_DetourWindowProc ( _In_  HWND   hWnd,
                      _In_  UINT   uMsg,
                      _In_  WPARAM wParam,
                      _In_  LPARAM lParam )
{
  if (hWnd != game_window.hWnd) {
    dll_log.Log ( L"[Window Mgr] New HWND detected in the window proc. used"
                  L" for rendering... (Old=%p, New=%p)",
                    game_window.hWnd, hWnd );

    game_window.hWnd = hWnd;

    game_window.active        = true;
    game_window.game.style    = game_window.GetWindowLongPtr (game_window.hWnd, GWL_STYLE);
    game_window.actual.style  = game_window.GetWindowLongPtr (game_window.hWnd, GWL_STYLE);
    game_window.unicode       = IsWindowUnicode (game_window.hWnd) != FALSE;

    GetWindowRect_Original (game_window.hWnd, &game_window.game.window);
    GetClientRect_Original (game_window.hWnd, &game_window.game.client);
    GetWindowRect_Original (game_window.hWnd, &game_window.actual.window);
    GetClientRect_Original (game_window.hWnd, &game_window.actual.client);

    SK_AdjustBorder        ();
    SK_AdjustWindow        ();
  }

  //return DefWindowProc (hWnd, uMsg, wParam, lParam);

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
          ////// XXX: Is this really necessary? State should be consistent unless we missed
          //////        an event --- Write unit test?
          GetWindowRect_Original (game_window.hWnd, &game_window.actual.window);
          ClipCursor_Original    (&game_window.actual.window);
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
  if (uMsg == WM_MOUSEACTIVATE/* && config.window.background_render*/) {
    if ((HWND)wParam == game_window.hWnd) {
      dll_log.Log (L"[Window Mgr] WM_MOUSEACTIVATE ==> Activate and Eat");

      ActivateWindow (true);

      if (config.window.background_render)
        return MA_ACTIVATEANDEAT;
    }

    ActivateWindow (false);

    dll_log.Log (L"[Window Mgr] WM_MOUSEACTIVATE ==> Activate");

    if (config.window.background_render)
      return MA_ACTIVATE;
  }

  // Allow the game to run in the background
  if (uMsg == WM_ACTIVATEAPP || uMsg == WM_ACTIVATE || uMsg == WM_NCACTIVATE /*|| uMsg == WM_MOUSEACTIVATE*/)
  {
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
          if (! hooked)
            game_window.CallWindowProc (game_window.WndProc_Original, hWnd, uMsg, TRUE, (LPARAM)hWnd);
          else
            game_window.WndProc_Original (hWnd, uMsg, TRUE, (LPARAM)hWnd);

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

  if (uMsg == WM_NCCALCSIZE) {
    return game_window.DefWindowProc (game_window.hWnd, uMsg, wParam, lParam);
  }

  if (uMsg == WM_WINDOWPOSCHANGING) {
    LPWINDOWPOS wnd_pos = (LPWINDOWPOS)lParam;
  }

  if (uMsg == WM_WINDOWPOSCHANGED) {
    LPWINDOWPOS wnd_pos = (LPWINDOWPOS)lParam;

    if (wnd_pos->flags & SWP_FRAMECHANGED) {
      game_window.actual.style =
        game_window.GetWindowLongPtr (game_window.hWnd, GWL_STYLE);

      bool borderless = (! SK_WindowManager::StyleHasBorder (game_window.actual.style));

      SK_WindowManager* pWndMgr = SK_WindowManager::getInstance ();

      pWndMgr->finishBorderless (borderless);
    }

    if (! (wnd_pos->flags & SWP_NOMOVE || wnd_pos->flags & SWP_NOSIZE)) {
      SK_AdjustWindow ();
    }
  }

  if ((! game_window.active) && uMsg == WM_MOUSEMOVE) {
    GetCursorPos_Original (&game_window.cursor_pos);
  }

  bool background_render =
    config.window.background_render && (! game_window.active);

  if ((uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN) && (! background_render)) {
    if (SK_Console::getInstance ()->KeyDown (wParam & 0xFF, lParam) && (uMsg != WM_SYSKEYDOWN)) {
      if (! hooked)
        return game_window.CallWindowProc (game_window.WndProc_Original, hWnd, uMsg, wParam, lParam);
      else
        return game_window.WndProc_Original (hWnd, uMsg, wParam, lParam);
    }
  }

  if ((uMsg == WM_KEYUP || uMsg == WM_SYSKEYUP) && (! background_render)) {
    if (SK_Console::getInstance ()->KeyUp (wParam & 0xFF, lParam) && (uMsg != WM_SYSKEYUP)) {
      if (! hooked)
        return game_window.CallWindowProc (game_window.WndProc_Original, hWnd, uMsg, wParam, lParam);
      else
        return game_window.WndProc_Original (hWnd, uMsg, wParam, lParam);
    }
  }

  // Block keyboard input to the game while the console is visible
  if (console_visible || (background_render && uMsg != WM_SYSKEYDOWN)) {
    if (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST)
      return game_window.DefWindowProc (hWnd, uMsg, wParam, lParam);

    if (uMsg >= WM_KEYFIRST && uMsg <= WM_KEYLAST)
      return game_window.DefWindowProc (hWnd, uMsg, wParam, lParam);

    // Block RAW Input
    if (console_visible && uMsg == WM_INPUT)
      return game_window.DefWindowProc (hWnd, uMsg, wParam, lParam);
  }

  if (! hooked)
    return game_window.CallWindowProc (game_window.WndProc_Original, hWnd, uMsg, wParam, lParam);
  else
    return game_window.WndProc_Original (hWnd, uMsg, wParam, lParam);
}

#include <SpecialK/core.h>

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
  for (unsigned int i = 0; i < uiNumDevices; i++) {
    pDevices [i] = pRawInputDevices [i];
    pDevices [i].dwFlags &= ~(RIDEV_NOLEGACY | RIDEV_APPKEYS | RIDEV_REMOVE);
    pDevices [i].dwFlags &= ~RIDEV_CAPTUREMOUSE;
  }

  BOOL bRet =
    RegisterRawInputDevices_Original (pDevices, uiNumDevices, cbSize);

  delete [] pDevices;

  return bRet;
}

void
SK_InstallWindowHook (HWND hWnd)
{
  static volatile LONG installed = FALSE;

  if (InterlockedCompareExchange (&installed, TRUE, FALSE))
    return;

  SK_CreateDLLHook2 ( L"user32.dll", "GetRawInputData",
                     GetRawInputData_Detour,
           (LPVOID*)&GetRawInputData_Original );

  SK_CreateDLLHook2 ( L"user32.dll", "GetKeyState",
                     GetKeyState_Detour,
           (LPVOID*)&GetKeyState_Original );

  static HMODULE hModTZFix = GetModuleHandle (L"tzfix.dll");

  //
  // TZFix has its own limiter
  //
  if (! hModTZFix) {
    SK_CreateDLLHook2 ( L"user32.dll", "GetCursorPos",
                       GetCursorPos_Detour,
             (LPVOID*)&GetCursorPos_Original );

    SK_CreateDLLHook2 ( L"user32.dll", "GetCursorInfo",
                       GetCursorInfo_Detour,
            (LPVOID*)&GetCursorInfo_Original );
  } else {
    GetCursorPos_Original =
      (GetCursorPos_pfn)GetProcAddress (
        GetModuleHandle (L"user32.dll"),
          "GetCursorPos"
      );
    GetCursorInfo_Original =
      (GetCursorInfo_pfn)GetProcAddress (
        GetModuleHandle (L"user32.dll"),
          "GetCursorInfo"
      );
  }

  SK_CreateDLLHook2 ( L"user32.dll", "GetAsyncKeyState",
                     GetAsyncKeyState_Detour,
           (LPVOID*)&GetAsyncKeyState_Original );

  SK_CreateDLLHook2 ( L"user32.dll", "RegisterRawInputDevices",
                     RegisterRawInputDevices_Detour,
           (LPVOID*)&RegisterRawInputDevices_Original );

  MH_ApplyQueued ();

  game_window.hWnd = hWnd;
  game_window.unicode = IsWindowUnicode (game_window.hWnd) != FALSE;

  if (game_window.unicode)  {
#ifdef _WIN64
    game_window.GetWindowLongPtr = GetWindowLongPtrW_Original;
#else
    game_window.GetWindowLongPtr = (GetWindowLongPtr_pfn)GetWindowLongW_Original;
#endif
    game_window.SetWindowLongPtr = SetWindowLongPtrW_Original;
    game_window.DefWindowProc    = (DefWindowProc_pfn)
      GetProcAddress ( GetModuleHandle (L"user32.dll"),
                         "DefWindowProcW" );
    game_window.CallWindowProc   = (CallWindowProc_pfn)
      GetProcAddress ( GetModuleHandle (L"user32.dll"),
                         "CallWindowProcW" );
  } else {
#ifdef _WIN64
    game_window.GetWindowLongPtr = GetWindowLongPtrW_Original;
#else
    game_window.GetWindowLongPtr = (GetWindowLongPtr_pfn)GetWindowLongA_Original;
#endif
    game_window.SetWindowLongPtr = SetWindowLongPtrA_Original;
    game_window.DefWindowProc    = (DefWindowProc_pfn)
      GetProcAddress ( GetModuleHandle (L"user32.dll"),
                         "DefWindowProcA" );
    game_window.CallWindowProc   = (CallWindowProc_pfn)
      GetProcAddress ( GetModuleHandle (L"user32.dll"),
                         "CallWindowProcA" );
  }

  WNDPROC class_proc = game_window.unicode ? (WNDPROC)
    GetClassLongPtrW  ( game_window.hWnd, GCLP_WNDPROC ) :
                                 (WNDPROC)
    GetClassLongPtrA  ( game_window.hWnd, GCLP_WNDPROC );

  WNDPROC wnd_proc = (WNDPROC)
    game_window.GetWindowLongPtr ( game_window.hWnd, GWLP_WNDPROC );

  game_window.WndProc_Original = nullptr;

  wchar_t wszClassName [256] = { L'\0' };
  GetClassNameW ( game_window.hWnd, wszClassName, 256 );

  dll_log.Log ( L"[Window Mgr] Hooking the Window Procedure for "
                L"%s Window Class ('%s') :: (ClassProc: %p, WndProc: %p)",
                  game_window.unicode ? L"Unicode" : L"ANSI",
                    wszClassName,
                      class_proc,
                        wnd_proc );

#if 0
  if (MH_OK == MH_CreateHook ( (LPVOID)class_proc,
                        SK_DetourWindowProc,
             (LPVOID *)&game_window.WndProc_Original )) {
    MH_EnableHook ((LPVOID)class_proc);
    dll_log.Log ( L"[Window Mgr]  >> Hooked ClassProc.");
    hooked = true;
  }

  else if (MH_OK == MH_CreateHook ( (LPVOID)wnd_proc,
                        SK_DetourWindowProc,
             (LPVOID *)&game_window.WndProc_Original )) {
    MH_EnableHook ((LPVOID)wnd_proc);
    dll_log.Log ( L"[Window Mgr]  >> Hooked WndProc.");
    hooked = true;
  }

  else
#endif
  {
    dll_log.Log ( L"[Window Mgr]  >> Hooking was impossible; installing new "
                  L"window procedure instead (this may be undone "
                  L"by other software)." );
    game_window.WndProc_Original = (WNDPROC)wnd_proc;

    game_window.SetWindowLongPtr ( game_window.hWnd, GWLP_WNDPROC, (LONG_PTR)SK_DetourWindowProc );

    if (game_window.unicode)
      SetClassLongPtrW ( game_window.hWnd, GCLP_WNDPROC, (LONG_PTR)SK_DetourWindowProc );
    else
      SetClassLongPtrA ( game_window.hWnd, GCLP_WNDPROC, (LONG_PTR)SK_DetourWindowProc );

    hooked = false;
  }

  game_window.game.style   = game_window.GetWindowLongPtr (game_window.hWnd, GWL_STYLE) | WS_VISIBLE;//0x90CA0000;
  game_window.actual.style = game_window.GetWindowLongPtr (game_window.hWnd, GWL_STYLE) | WS_VISIBLE;//0x90CA0000;

  GetWindowRect_Original (game_window.hWnd, &game_window.game.window);
  GetClientRect_Original (game_window.hWnd, &game_window.game.client);

  GetWindowRect_Original (game_window.hWnd, &game_window.actual.window);
  GetClientRect_Original (game_window.hWnd, &game_window.actual.client);

  //if (! (game_window.style & WS_VISIBLE)) {
    //game_window.style |= (WS_VISIBLE | WS_MINIMIZEBOX);
    //game_window.style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
  //}

  GetCursorPos_Original  (&game_window.cursor_pos);

  SK_AdjustBorder ();
  SK_AdjustWindow ();

  //if (game_window.style & WS_VISIBLE)
    //SK_RealizeForegroundWindow (game_window.hWnd);

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