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
#define NOMINMAX

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

#define SK_LOG_FIRST_CALL

#include <mmsystem.h>
#pragma comment (lib, "winmm.lib")

#include <comdef.h>
#include <process.h>

#include <windowsx.h>

extern volatile ULONG __SK_DLL_Ending;

#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS
#include <imgui/imgui.h>

#define SK_BORDERLESS    ( WS_VISIBLE | WS_POPUP | WS_MINIMIZEBOX )
#define SK_BORDERLESS_EX ( WS_EX_APPWINDOW )

#define SK_LOG_LEVEL_UNTESTED

#define SK_WINDOW_LOG_CALL0() if (config.system.log_level >= 0) SK_LOG_CALL ("Window Mgr")
#define SK_WINDOW_LOG_CALL1() if (config.system.log_level >= 1) SK_LOG_CALL ("Window Mgr")
#define SK_WINDOW_LOG_CALL2() if (config.system.log_level >= 2) SK_LOG_CALL ("Window Mgr")
#define SK_WINDOW_LOG_CALL3() if (config.system.log_level >= 3) SK_LOG_CALL ("Window Mgr")
#define SK_WINDOW_LOG_CALL4() if (config.system.log_level >= 3) SK_LOG_CALL ("Window Mgr")

#ifdef SK_LOG_LEVEL_UNTESTED
# define SK_WINDOW_LOG_CALL_UNTESTED() SK_LOG_CALL ("Window Mgr");
#else
# define SK_WINDOW_LOG_CALL_UNTESTED() { }
#endif


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

sk_window_s game_window;

typedef BOOL (WINAPI *SetCursorPos_pfn)
(
  _In_ int X,
  _In_ int Y
);

SetCursorPos_pfn  SetCursorPos_Original  = nullptr;

typedef BOOL (WINAPI *SetPhysicalCursorPos_pfn)
(
  _In_ int X,
  _In_ int Y
);

SetCursorPos_pfn  SetPhysicalCursorPos_Original  = nullptr;

typedef UINT (WINAPI *SendInput_pfn)(
  _In_ UINT    nInputs,
  _In_ LPINPUT pInputs,
  _In_ int     cbSize
);

SendInput_pfn SendInput_Original = nullptr;

typedef VOID(WINAPI *mouse_event_pfn)(
  _In_ DWORD     dwFlags,
  _In_ DWORD     dx,
  _In_ DWORD     dy,
  _In_ DWORD     dwData,
  _In_ ULONG_PTR dwExtraInfo
);

mouse_event_pfn mouse_event_Original = nullptr;

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

bool override_window_rects = false;

void SK_AdjustBorder (void);
void SK_AdjustWindow (void);

//
// If we did this from the render thread, we would deadlock most games
//
auto DeferCommand = [=] (const char* szCommand) ->
  void
    {
      CreateThread ( nullptr,
                       0x00,
                    [ ](LPVOID user) ->
        DWORD
          {
            SK_GetCommandProcessor ()->ProcessCommandLine (
               (const char*)user
            );

            CloseHandle (GetCurrentThread ());

            return 0;
          },

        (LPVOID)szCommand,
      0x00,
    nullptr
  );
};

// TODO: Refactor that structure above into this class ;)
//
//  Also, combine a few of the input manager settings with window manager
class SK_WindowManager : public SK_IVariableListener
{
public:
  static bool StyleHasBorder (DWORD_PTR style)
  {
    return (  ( style == 0x0            ) ||
              ( style  &  WS_BORDER     ) ||
              ( style  &  WS_THICKFRAME ) ||
              ( style  &  WS_DLGFRAME   )    );
  }

  virtual bool OnVarChange (SK_IVariable* var, void* val)
  {
    if (var == confine_cursor_)
    {
      if (val != nullptr)
      {
        config.window.confine_cursor = *(bool *)val;

        if (! config.window.confine_cursor)
        {
          if (! config.window.unconfine_cursor)
          {
            ClipCursor_Original    (&game_window.cursor_clip);
            SK_AdjustWindow        ();
          }

          else
            ClipCursor_Original    (nullptr);
        }

        else
        {
          GetWindowRect_Original (game_window.hWnd, &game_window.actual.window);
          ClipCursor_Original    (&game_window.actual.window);
        }
      }

      return true;
    }

    if (var == unconfine_cursor_)
    {
      if (val != nullptr)
      {
        config.window.unconfine_cursor = *(bool *)val;

        if (config.window.unconfine_cursor)
          ClipCursor_Original (nullptr);

        else if (config.window.confine_cursor)
        {
          GetWindowRect_Original (game_window.hWnd, &game_window.actual.window);
          ClipCursor_Original    (&game_window.actual.window);
        }

        else
        {
          ClipCursor_Original    (&game_window.cursor_clip);
          SK_AdjustWindow        ();
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

        else if ( var == borderless_ && 
         *(bool *)val != config.window.borderless )
        {
          config.window.borderless = *(bool *)val;

          SK_AdjustBorder ();
        }

        else if (var == fullscreen_) 
        {
          static int x = config.window.res.override.x;
          static int y = config.window.res.override.y;

          if ( config.window.fullscreen != *(bool *)val && ( config.window.borderless ||
                                                            (config.window.fullscreen) ) )
          {
            config.window.fullscreen = *(bool *)val;

            static bool first_set = true;
            static RECT last_known_client;
            static RECT last_known_window;

            RECT client;
            GetClientRect_Original (game_window.hWnd, &client);

            HMONITOR hMonitor =
              MonitorFromWindow ( game_window.hWnd,
                                    MONITOR_DEFAULTTONEAREST );

            MONITORINFO mi  = { 0 };
            mi.cbSize       = sizeof (mi);
            GetMonitorInfo (hMonitor, &mi);

            if ( (client.right  - client.left != mi.rcMonitor.right  - mi.rcMonitor.left ) &&
                 (client.bottom - client.top  != mi.rcMonitor.bottom - mi.rcMonitor.top  ) )
            {
              GetClientRect_Original (game_window.hWnd, &last_known_client);
              GetWindowRect_Original (game_window.hWnd, &last_known_window);
            }

            unsigned int orig_x = config.window.res.override.x,
                         orig_y = config.window.res.override.y;

            static unsigned int persist_x = orig_x, persist_y = orig_y;

            // Restore Window Dimensions
            if (! config.window.fullscreen) 
            {
              config.window.res.override.x = last_known_client.right  - last_known_client.left;
              config.window.res.override.y = last_known_client.bottom - last_known_client.top;

              // Trigger re-calc on adjust
              game_window.game.client   = RECT { last_known_client.left,last_known_client.top,last_known_client.right,last_known_client.bottom };
              game_window.game.window   = RECT { last_known_window.left,last_known_window.top,last_known_window.right,last_known_window.bottom };

              game_window.actual.client = RECT { last_known_client.left,last_known_client.top,last_known_client.right,last_known_client.bottom };
              game_window.actual.window = RECT { last_known_window.left,last_known_window.top,last_known_window.right,last_known_window.bottom };

              if (! config.window.unconfine_cursor)
              {
                RECT clip = game_window.actual.window;

                ClipCursor_Original (&clip);
              }

              else
                ClipCursor_Original (nullptr);
            }

            // Go Fullscreen (Stetch Window to Fill)
            else
            {
              config.window.res.override.x = mi.rcMonitor.right  - mi.rcMonitor.left;
              config.window.res.override.y = mi.rcMonitor.bottom - mi.rcMonitor.top;

              game_window.actual.client = RECT { 0,0, mi.rcMonitor.right-mi.rcMonitor.left, mi.rcMonitor.bottom-mi.rcMonitor.top };
              game_window.actual.window = RECT { mi.rcMonitor.left,mi.rcMonitor.top,mi.rcMonitor.right,mi.rcMonitor.bottom };
            }

            SK_AdjustWindow ();

            if (! config.window.fullscreen)
            {
              // XXX: This is being clobbered by another thread, needs redesign...
              config.window.res.override.x = persist_x;
              config.window.res.override.y = persist_y;
            }

            else
            {
              persist_x = orig_x;
              persist_y = orig_y;

              config.window.res.override.x = 0;
              config.window.res.override.y = 0;
            }

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

        if (var == borderless_ && (! config.window.borderless))
        {
          if (config.window.fullscreen)
          {
            // Bring the game OUT of fullscreen mode, that's only for borderless
            SK_GetCommandProcessor ()->ProcessCommandLine ("Window.Fullscreen 0");
          } else {
            SK_AdjustWindow ();
            return true;
          }
        }

#if 1
        SK_AdjustWindow ();
#else
        if (var != center_window_)
        {
          static char szCmd [64] = { '\0' };
          snprintf (szCmd, 63, "Window.Center %d", config.window.center);

          DeferCommand (szCmd);
        }

        if (var != borderless_) {
          static char szCmd [64] = { '\0' };
          snprintf (szCmd, 63, "Window.Borderless %d", config.window.borderless);

          DeferCommand (szCmd);
        }
#endif

        return true;
      }
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

         char szTemp    [32] = { 0 };

      strncpy (szTemp, *(char **)val, 31);

      if (val != nullptr)
        sscanf (szTemp, "%lux%lu", &x, &y);

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

    unconfine_cursor_ =
      SK_CreateVar (SK_IVariable::Boolean,&config.window.unconfine_cursor,this);

    cmd->AddVariable (
      "Window.UnconfineCursor",
        unconfine_cursor_
    );

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
  SK_IVariable* unconfine_cursor_;
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
  //game_window.game.client.right = game_window.game.client.left + x;
  game_window.render_x = x;
}

void
SK_SetWindowResY (LONG y)
{
  //game_window.game.client.bottom = game_window.game.client.top + y;
  game_window.render_y = y;
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

extern bool SK_ImGui_Visible;

ULONG   game_mouselook = 0;
int     game_x, game_y;
HCURSOR game_cursor;

bool
ImGui_WantMouseCapture (void)
{
  bool imgui_capture = false;

  if (SK_ImGui_Visible)
  {
    ImGuiIO& io =
      ImGui::GetIO ();

    if (config.input.ui.capture || io.WantCaptureMouse || (game_mouselook >= SK_GetFramesDrawn () - 1))
      imgui_capture = true;
  }

  return imgui_capture;
}

typedef HCURSOR (WINAPI *SetCursor_pfn)(HCURSOR hCursor);
SetCursor_pfn SetCursor_Original = nullptr;

void
ImGui_ToggleCursor (void)
{
  static bool imgui_cursor = false;

  if (! imgui_cursor)
  {
    ShowCursor         (FALSE);
    SetCursor_Original (nullptr);
    SetCursorPos       ( (int)ImGui::GetIO ().DisplaySize.x / 2, 
                         (int)ImGui::GetIO ().DisplaySize.y / 2 );
  } else {
    ShowCursor         (TRUE);
    SetCursor_Original (game_cursor);
  }

  imgui_cursor = (! imgui_cursor);
}

HCURSOR
WINAPI
SetCursor_Detour (
  _In_opt_ HCURSOR hCursor )
{
  SK_LOG_FIRST_CALL

  game_cursor = SetCursor_Original (hCursor);

  return game_cursor;
}

BOOL
WINAPI
GetCursorInfo_Detour (PCURSORINFO pci)
{
  SK_LOG_FIRST_CALL

  BOOL ret = GetCursorInfo_Original (pci);

  pci->hCursor = game_cursor;

  if (ImGui_WantMouseCapture ())
  {
    POINT tmp_pt = pci->ptScreenPos;

    pci->ptScreenPos.x = game_x;
    pci->ptScreenPos.y = game_y;

    game_x = tmp_pt.x;
    game_y = tmp_pt.y;

    return TRUE;
  }

  return ret;
}

BOOL
WINAPI
GetCursorPos_Detour (LPPOINT lpPoint)
{
  SK_LOG_FIRST_CALL

  BOOL ret = GetCursorPos_Original (lpPoint);

  if (ImGui_WantMouseCapture ()) {
    POINT tmp_pt = *lpPoint;

    lpPoint->x = game_x;
    lpPoint->y = game_y;

    game_x = tmp_pt.x;
    game_y = tmp_pt.y;

    return TRUE;
  }

  return ret;
}

BOOL
WINAPI
SetCursorPos_Detour (_In_ int x, _In_ int y)
{
  SK_LOG_FIRST_CALL

  // Don't let the game continue moving the cursor while
  //   Alt+Tabbed out
  if (config.window.background_render && (! game_window.active))
    return TRUE;

  // Prevent Mouse Look while Drag Locked
  if (config.window.drag_lock)
    return TRUE;

  if (SK_ImGui_Visible)
  {
    game_mouselook = SK_GetFramesDrawn ();
  }

  else {
    return SetCursorPos_Original (x, y);
  }

  return TRUE;
}

BOOL
WINAPI
SetPhysicalCursorPos_Detour (_In_ int x, _In_ int y)
{
  SK_LOG_FIRST_CALL

  if (SK_ImGui_Visible)
  {
    game_mouselook = SK_GetFramesDrawn ();
  }

  else {
    return SetPhysicalCursorPos_Original (x, y);
  }

  return TRUE;
}

UINT
WINAPI
SendInput_Detour (
  _In_ UINT    nInputs,
  _In_ LPINPUT pInputs,
  _In_ int     cbSize
)
{
  SK_LOG_FIRST_CALL

  // TODO: Process this the right way...

  if (SK_ImGui_Visible)
  {
    return 0;
  }

  return SendInput_Original (nInputs, pInputs, cbSize);
}

VOID
WINAPI
mouse_event_Detour (
  _In_ DWORD     dwFlags,
  _In_ DWORD     dx,
  _In_ DWORD     dy,
  _In_ DWORD     dwData,
  _In_ ULONG_PTR dwExtraInfo
)
{
  SK_LOG_FIRST_CALL

// TODO: Process this the right way...

  if (SK_ImGui_Visible)
  {
    return;
  }

  mouse_event_Original (dwFlags, dx, dy, dwData, dwExtraInfo);
}

#if 0
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
#endif


BOOL
WINAPI
ClipCursor_Detour (const RECT *lpRect)
{
  SK_LOG_FIRST_CALL

  if (lpRect != nullptr)
    game_window.cursor_clip = *lpRect;

  // Don't let the game unclip the cursor, but DO remember the
  //   coordinates that it wants.
  if (config.window.confine_cursor && lpRect != &game_window.cursor_clip)
    return TRUE;

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


  // Prevent the game from clipping the cursor
  if (config.window.unconfine_cursor)
    return ClipCursor_Original (nullptr);


  if (game_window.active && lpRect != nullptr) {
    return ClipCursor_Original (&game_window.cursor_clip);
  } else {
    return ClipCursor_Original (nullptr);
  }
}

void
SK_CenterWindowAtMouse (BOOL remember_pos)
{
  // This is too much trouble to bother with...
  if (config.window.center || (config.window.borderless && config.window.fullscreen))
    return;

  static CRITICAL_SECTION cs_center;
  static bool             init = false;

  if (! init) {
    InitializeCriticalSection (&cs_center);
    init = true;
  }

  CreateThread ( nullptr, 0,
    [](LPVOID user) ->
      DWORD {
  EnterCriticalSection (&cs_center);

  BOOL  remember_pos = (BOOL)(user != nullptr);
  POINT mouse        = { 0, 0 };

  if (GetCursorPos_Original != nullptr)
    GetCursorPos_Original (&mouse);

  struct {
    struct {
      float percent  = 0.0f;
      int   absolute =   0L;
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

  LeaveCriticalSection (&cs_center);

  CloseHandle (GetCurrentThread ());

  return 0;
       // Don't dereference this, it's actually a boolean
    }, (LPVOID)(uintptr_t)remember_pos, 0x0, nullptr );
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
  SK_LOG_FIRST_CALL

  return MoveWindow_Original ( hWnd,
                                 X, Y,
                                   nWidth, nHeight,
                                     bRedraw );
}

BOOL
WINAPI
SetWindowPlacement_Detour(
  _In_       HWND             hWnd,
  _In_ const WINDOWPLACEMENT *lpwndpl)
{
  SK_WINDOW_LOG_CALL_UNTESTED ();

  return SetWindowPlacement_Original ( hWnd, lpwndpl );
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
  SK_LOG_FIRST_CALL

  if (hWnd == game_window.hWnd) {
    SK_WINDOW_LOG_CALL0 ();
  }

  BOOL bRet = 
    SetWindowPos_Original ( hWnd, hWndInsertAfter,
                              X, Y,
                                cx, cy,
                                  uFlags );

  return bRet;
}

static
BOOL
WINAPI
GetClientRect_Detour (
    _In_  HWND   hWnd,
    _Out_ LPRECT lpRect )
{
  SK_LOG_FIRST_CALL

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
  SK_LOG_FIRST_CALL

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
SK_IsRectInfinite (_In_ LPRECT lpRect)
{
  return ( lpRect->left   == LONG_MIN && lpRect->top   == LONG_MIN &&
           lpRect->bottom == LONG_MAX && lpRect->right == LONG_MAX );
}

BOOL
WINAPI
SK_IsRectFullscreen (_In_ LPRECT lpRect)
{
  int virtual_x = GetSystemMetrics_Original (SM_CXVIRTUALSCREEN);
  int virtual_y = GetSystemMetrics_Original (SM_CYVIRTUALSCREEN);

  return ( lpRect->left  == 0         && lpRect->top    == 0 &&
           lpRect->right == virtual_x && lpRect->bottom == virtual_y );
}

BOOL
WINAPI
SK_IsClipRectFinite (_In_ LPRECT lpRect)
{
  return (! ( SK_IsRectZero       (lpRect) ||
              SK_IsRectInfinite   (lpRect) ||
              SK_IsRectFullscreen (lpRect) ) );
}

BOOL
WINAPI
AdjustWindowRect_Detour (
    _Inout_ LPRECT lpRect,
    _In_    DWORD  dwStyle,
    _In_    BOOL   bMenu )
{
  SK_LOG1 ( ( L"AdjustWindowRect ( "
              L"{%4lu,%4lu / %4lu,%4lu}, 0x%04X, %lu ) - %s",
                lpRect->left, lpRect->top,
                  lpRect->right, lpRect->bottom,
                    dwStyle, bMenu,
                      SK_SummarizeCaller ().c_str () ),
                L"Window Mgr" );

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
  SK_LOG1 ( ( L"AdjustWindowRectEx ( "
              L"{%4lu,%4lu / %4lu,%4lu}, 0x%04X, %lu, 0x%04X ) - %s",
                lpRect->left, lpRect->top,
                  lpRect->right, lpRect->bottom,
                    dwStyle, bMenu,
                      dwExStyle,
                        SK_SummarizeCaller ().c_str () ),
               L"Window Mgr" );

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
  if (hWnd == game_window.hWnd)
  {
    switch (nIndex)
    {
      case GWL_STYLE:
      {
        game_window.game.style =
          (LONG_PTR)dwNewLong;

        if (config.window.borderless)
        {
          game_window.actual.style =
            SK_BORDERLESS;
        }

        else
        {
          game_window.actual.style =
            game_window.game.style;
        }

        if ( SK_WindowManager::StyleHasBorder (
               game_window.game.style
             )
           )
        {
          game_window.border_style =
            game_window.game.style;
        }

        pOrigFunc ( hWnd,
                      GWL_STYLE,
                        (LONG)game_window.actual.style );

        return (LONG)game_window.actual.style;
      }

      case GWL_EXSTYLE:
      {
        game_window.game.style_ex =
          (LONG_PTR)dwNewLong;

        if (config.window.borderless)
        {
          game_window.actual.style_ex =
            SK_BORDERLESS_EX;
        }

        else
        {
          game_window.actual.style_ex =
            game_window.game.style;
        }

        if ( SK_WindowManager::StyleHasBorder (
               game_window.actual.style
             )
           )
        {
          game_window.border_style_ex =
            game_window.game.style_ex;
        }

        pOrigFunc ( hWnd,
                      GWL_EXSTYLE,
                        (LONG)game_window.actual.style_ex );

        return (LONG)game_window.actual.style_ex;
      }
    }
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
  SK_LOG_FIRST_CALL

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
  SK_LOG_FIRST_CALL

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
  if (hWnd == game_window.hWnd)
  {
    switch (nIndex)
    {
      case GWL_STYLE:
        return (LONG)game_window.game.style;
      case GWL_EXSTYLE:
        return (LONG)game_window.game.style_ex;
    }
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
  SK_LOG_FIRST_CALL

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
  SK_LOG_FIRST_CALL

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
  if (hWnd == game_window.hWnd)
  {
    switch (nIndex)
    {
      case GWL_STYLE:
      {
        game_window.game.style =
          dwNewLong;

        if (config.window.borderless)
        {
          game_window.actual.style =
            SK_BORDERLESS;
        }

        else
        {
          game_window.actual.style =
            game_window.game.style;
        }

        if ( SK_WindowManager::StyleHasBorder (
               game_window.game.style
             )
           )
        {
          game_window.border_style =
            game_window.game.style;
        }

        pOrigFunc ( hWnd,
                      GWL_STYLE,
                        game_window.actual.style );

        return game_window.actual.style;
      }

      case GWL_EXSTYLE:
      {
        game_window.game.style_ex =
          dwNewLong;

        if (config.window.borderless)
        {
          game_window.actual.style_ex =
            SK_BORDERLESS_EX;
        }

        else
        {
          game_window.actual.style_ex =
            game_window.game.style;
        }

        if ( SK_WindowManager::StyleHasBorder (
               game_window.actual.style
             )
           )
        {
          game_window.border_style_ex =
            game_window.game.style_ex;
        }

        pOrigFunc ( hWnd,
                      GWL_EXSTYLE,
                        game_window.actual.style_ex );

        return game_window.actual.style_ex;
      }
    }
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
  SK_LOG_FIRST_CALL

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
  SK_LOG_FIRST_CALL

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
  if (hWnd == game_window.hWnd)
  {
    switch (nIndex)
    {
      case GWL_STYLE:
        return game_window.game.style;
      case GWL_EXSTYLE:
        return game_window.game.style_ex;
    }
  }

  return pOrigFunc (hWnd, nIndex);
}

DECLSPEC_NOINLINE
LONG_PTR
WINAPI
GetWindowLongPtrA_Detour (
  _In_ HWND     hWnd,
  _In_ int      nIndex
)
{
  SK_LOG_FIRST_CALL

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
  _In_ int      nIndex
)
{
  SK_LOG_FIRST_CALL

  return GetWindowLongPtr_Marshall (
    GetWindowLongPtrW_Original,
      hWnd,
        nIndex
  );
}

void
SK_SetWindowStyle (DWORD_PTR dwStyle)
{
  // This is damn important, never allow this style
  //   to be removed or the window will vanish.
  dwStyle |= WS_VISIBLE;

  game_window.actual.style = dwStyle;

  game_window.SetWindowLongPtr ( game_window.hWnd,
                                   GWL_STYLE,
                                     dwStyle );
}

void
SK_SetWindowStyleEx (DWORD_PTR dwStyleEx)
{
  // This is damn important, never allow this style
  //   to be removed or the window will vanish.
  dwStyleEx |= WS_EX_APPWINDOW;

  game_window.actual.style_ex = dwStyleEx;

  game_window.SetWindowLongPtr ( game_window.hWnd,
                                   GWL_EXSTYLE,
                                     dwStyleEx );
}

RECT
SK_ComputeClientSize (void)
{
  bool use_override = (! config.window.res.override.isZero ());

  if (use_override) {
    return ( RECT { 0L, 0L,
                      (LONG)config.window.res.override.x,
                        (LONG)config.window.res.override.y } );
  }

  RECT ret = game_window.actual.client;

  GetClientRect_Original ( game_window.hWnd, &ret );

  ret = { 0, 0,
            ret.right - ret.left,
              ret.bottom - ret.top };

  return ret;
}

POINT
SK_ComputeClientOrigin (void)
{
  RECT window, client;

  GetWindowRect_Original ( game_window.hWnd, &window );
  GetClientRect_Original ( game_window.hWnd, &client );

  return POINT { window.left + client.left,
                   window.top + client.top };
}

bool
SK_IsRectTooBigForDesktop (RECT wndRect)
{
  HMONITOR hMonitor =
    MonitorFromWindow ( game_window.hWnd,
                          MONITOR_DEFAULTTONEAREST );

  MONITORINFO mi  = { 0 };
  mi.cbSize       = sizeof (mi);
  GetMonitorInfo (hMonitor, &mi);

  int win_width      = wndRect.right       - wndRect.left;
  int win_height     = wndRect.bottom      - wndRect.top;

  if (! config.window.res.override.isZero ()) {
    win_width  = config.window.res.override.x;
    win_height = config.window.res.override.y;
  }

  int desktop_width  = mi.rcWork.right     - mi.rcWork.left;
  int desktop_height = mi.rcWork.bottom    - mi.rcWork.top;

  int mon_width      = mi.rcMonitor.right  - mi.rcMonitor.left;
  int mon_height     = mi.rcMonitor.bottom - mi.rcMonitor.top;

  if (win_width == mon_width && win_height == mon_height)
    return true;

  if (win_width > desktop_width && win_height > desktop_height)
    return true;

  return false;
}

void
SK_AdjustBorder (void)
{
  SK_WINDOW_LOG_CALL3 ();

  if (game_window.GetWindowLongPtr == nullptr)
    return;

  game_window.actual.style    = game_window.GetWindowLongPtr ( game_window.hWnd, GWL_STYLE   );
  game_window.actual.style_ex = game_window.GetWindowLongPtr ( game_window.hWnd, GWL_EXSTYLE );

  bool has_border =
    SK_WindowManager::StyleHasBorder (
      game_window.actual.style
  );

  // If these are opposite, we can skip a whole bunch of
  //   pointless work!
  if (has_border != config.window.borderless)
    return;

  game_window.actual.style =
    config.window.borderless ?
      SK_BORDERLESS :
        game_window.border_style;

  game_window.actual.style_ex =
    config.window.borderless ?
      SK_BORDERLESS_EX :
        game_window.border_style_ex;

  RECT orig_client;
  GetClientRect_Original (game_window.hWnd, &orig_client);

  RECT new_client =
    SK_ComputeClientSize ();

  RECT  new_window = new_client;
  POINT origin     { new_client.left, new_client.top };//SK_ComputeClientOrigin ();

  ClientToScreen (game_window.hWnd, &origin);

  SK_SetWindowStyle   ( game_window.actual.style    );
  SK_SetWindowStyleEx ( game_window.actual.style_ex );

  if ( AdjustWindowRectEx_Original ( &new_window,
                                       PtrToUint ( (LPVOID)game_window.actual.style ),
                                         FALSE,
                                           PtrToUint ( (LPVOID)game_window.actual.style_ex )
                                   )
     )
  {
    bool had_border = has_border;

    int origin_x = had_border ? origin.x + orig_client.left : origin.x;
    int origin_y = had_border ? origin.y + orig_client.top  : origin.y;

    SetWindowPos_Original ( game_window.hWnd,
                    HWND_TOP,
                      origin_x, origin_y,
                        new_window.right - new_window.left,
                        new_window.bottom - new_window.top,
                          SWP_NOZORDER | SWP_NOREPOSITION | SWP_FRAMECHANGED |
                          SWP_NOSENDCHANGING );

    GetWindowRect (game_window.hWnd, &game_window.actual.window);
    GetClientRect (game_window.hWnd, &game_window.actual.client);
  }

  game_window.game.window = game_window.actual.window;
}

void
SK_ResetWindow (void)
{
  static CRITICAL_SECTION cs_reset;
  static bool             init = false;

  if (! init) {
    InitializeCriticalSection (&cs_reset);
    init = true;
  }

  CreateThread (nullptr, 0,
    [](LPVOID user) ->
    DWORD
    {
      Sleep (100);

      EnterCriticalSection (&cs_reset);

      GetWindowRect (game_window.hWnd, &game_window.actual.window);
      GetClientRect (game_window.hWnd, &game_window.actual.client);
      
      if (config.window.borderless)
        SK_AdjustBorder ();
      
      if (config.window.center)
        SK_AdjustWindow ();

      if (config.window.fullscreen && config.window.borderless)
        SK_GetCommandProcessor ()->ProcessCommandLine ("Window.Fullscreen 1");

      LeaveCriticalSection (&cs_reset);

      CloseHandle (GetCurrentThread ());

      return 0;
    }, nullptr, 0x00, nullptr);

}

void
SK_AdjustWindow (void)
{
  SK_WINDOW_LOG_CALL3 ();

  DWORD async       = 0x00;//IsGUIThread (FALSE) ? SWP_ASYNCWINDOWPOS : SWP_ASYNCWINDOWPOS;

  if (game_window.GetWindowLongPtr == nullptr)
    return;

  HMONITOR hMonitor =
    MonitorFromWindow ( game_window.hWnd,
                          MONITOR_DEFAULTTONEAREST );

  MONITORINFO mi  = { 0 };
  mi.cbSize       = sizeof (mi);
  GetMonitorInfo (hMonitor, &mi);

  if (config.window.borderless && config.window.fullscreen)
  {
    SK_LOG4 ( (L" > SK_AdjustWindow (Fullscreen)"),
              L"Window Mgr" );

    SetWindowPos_Original ( game_window.hWnd,
                              HWND_TOP,
                                mi.rcMonitor.left,
                                mi.rcMonitor.top,
                                  mi.rcMonitor.right  - mi.rcMonitor.left,
                                  mi.rcMonitor.bottom - mi.rcMonitor.top,
                                    SWP_NOZORDER       | SWP_NOREPOSITION   |
                                    SWP_NOSENDCHANGING | SWP_ASYNCWINDOWPOS | SWP_FRAMECHANGED );

    SK_LOG1 ( ( L"FULLSCREEN => {Left: %li, Top: %li} - (WxH: %lix%li)",
                  mi.rcMonitor.left, mi.rcMonitor.top,
                    mi.rcMonitor.right - mi.rcMonitor.left,
                      mi.rcMonitor.bottom - mi.rcMonitor.top ),
                L"Border Mgr" );

    // Must set this or the mouse cursor clip rect will be wrong
    game_window.actual.window = mi.rcMonitor;
    game_window.actual.client = mi.rcMonitor;
  }

  else
  {
    SK_LOG4 ( (L" > SK_AdjustWindow (Windowed)"),
               L"Window Mgr" );

    // Adjust the desktop resolution to make room for window decorations
    //   if the game window were maximized.
    if (! config.window.borderless) {
      AdjustWindowRect_Original (
        &mi.rcWork,
          (LONG)game_window.actual.style,
            FALSE
      );
    }

    // Monitor Workspace
    LONG mon_width     = mi.rcWork.right     - mi.rcWork.left;
    LONG mon_height    = mi.rcWork.bottom    - mi.rcWork.top;

    // Monitor's ENTIRE coordinate space (includes taskbar)
    LONG real_width    = mi.rcMonitor.right  - mi.rcMonitor.left;
    LONG real_height   = mi.rcMonitor.bottom - mi.rcMonitor.top;

    // The Game's _Requested_ Client Rectangle
    LONG render_width  = game_window.game.client.right  - game_window.game.client.left;
    LONG render_height = game_window.game.client.bottom - game_window.game.client.top;

    // The Game's _Requested_ Window Rectangle (including borders)
    LONG full_width     = game_window.game.window.right  - game_window.game.window.left;
    LONG full_height    = game_window.game.window.bottom - game_window.game.window.top;

    if ((! config.window.res.override.isZero ()))
    {
      render_width  = config.window.res.override.x;
      render_height = config.window.res.override.y;
    }

    else {
      int  render_width_before  = game_window.game.client.right  - game_window.game.client.left;
      int  render_height_before = game_window.game.client.bottom - game_window.game.client.top;

      RECT client_before = game_window.game.client;

      GetClientRect_Original (game_window.hWnd, &game_window.game.client);
      GetWindowRect_Original (game_window.hWnd, &game_window.game.window);

      render_width  = game_window.game.client.right  - game_window.game.client.left;
      render_height = game_window.game.client.bottom - game_window.game.client.top;

      if ( render_width  != render_width_before  ||
           render_height != render_height_before )
      {
        SK_AdjustWindow ();
        return;
      }
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
      SK_LOG4 ( ( L"Center --> (%li,%li)",
                    mi.rcWork.right - mi.rcWork.left,
                      mi.rcWork.bottom - mi.rcWork.top ),
                    L"Window Mgr" );

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


    // Adjust the window to fit if it's not fullscreen
    if ( (full_height < mon_height ) ||
         (full_width  < mon_width  ) )
    {
      AdjustWindowRect_Original (
        &game_window.actual.window,
          (LONG)game_window.actual.style,
            FALSE
      );

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
    }

    else
    {
#if 0
      game_window.actual.window.left   = 0;
      game_window.actual.window.top    = 0;
      game_window.actual.window.right  = real_width;
      game_window.actual.window.bottom = real_height;

      game_window.actual.client = game_window.actual.window;

      game_window.actual.style    = SK_BORDERLESS;
      game_window.actual.style_ex = SK_BORDERLESS_EX;

      SK_SetWindowStyle   ( game_window.actual.style    );
      SK_SetWindowStyleEx ( game_window.actual.style_ex );
#endif
    }


    if (game_window.actual.window.right  - game_window.actual.window.left > 0 &&
        game_window.actual.window.bottom - game_window.actual.window.top  > 0 )
    {
      SetWindowPos_Original ( game_window.hWnd,
                                HWND_TOP,
                                  game_window.actual.window.left,
                                  game_window.actual.window.top,
                                    game_window.actual.window.right  - game_window.actual.window.left,
                                    game_window.actual.window.bottom - game_window.actual.window.top,
                                      SWP_NOZORDER | SWP_NOREPOSITION );
    }

    GetWindowRect_Original (game_window.hWnd, &game_window.actual.window);
    GetClientRect_Original (game_window.hWnd, &game_window.actual.client);

    GetWindowRect_Original (game_window.hWnd, &game_window.game.window);
    GetClientRect_Original (game_window.hWnd, &game_window.game.client);


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

    SK_LOG1 ( ( L"WINDOW => {Left: %li, Top: %li} - (WxH: %lix%li) - { Border: %s }",
                  game_window.actual.window.left, game_window.actual.window.top,
                    game_window.actual.window.right - game_window.actual.window.left,
                      game_window.actual.window.bottom - game_window.actual.window.top,
                        (! has_border) ? L"None" : wszBorderDesc ),
                L"Border Mgr" );
  }


  // Post-Process Results:
  //
  //  If window is moved as a result of this function, we need to:
  //  ------------------------------------------------------------
  //   Re-compute certain game-defined metrics (which may differ):
  //   ===========================================================
  //    1. Client Rectangle
  //    2. Clip Rectangle (mouse confinement)
  //    3. Which monitor the game is on (XXX: that whole thing needs consideration)
  //
  //   Decide how to translate mouse events so that if the game does not know the
  //     window was moved, it still processes mouse input in a sane way.
  //

  auto DescribeRect = [=](RECT* lpRect) ->
  void {
    SK_LOG2_EX ( false, ( L" => {Left: %li, Top: %li} - {WxH: %lix%li)\n",
                            lpRect->left, lpRect->top,
                              lpRect->right - lpRect->left,
                                lpRect->bottom - lpRect->top ) );
  };

  // 1. Client Rectangle
  ////RECT client;
  ////GetClientRect_Original (game_window.hWnd, &client);


  // 2. Re-Compute Clip Rectangle
  bool unconfine = config.window.unconfine_cursor;
  bool   confine = config.window.confine_cursor;

  // This logic is questionable -- we probably need to transform the clip rect,
  //                                 but not apply it when unconfine is true.
  if (! unconfine)
  {
    RECT clip;
    GetClipCursor (&clip);

    auto DescribeClipRect = [=](const wchar_t* wszName, RECT* lpRect) ->
    void {
      SK_LOG2_EX ( true, ( L"[Cursor Mgr] Clip Rect (%s)", wszName ) );
      DescribeRect (lpRect);
    };

    if (confine || SK_IsClipRectFinite (&game_window.cursor_clip) || SK_IsClipRectFinite (&clip))
    { 
      DescribeClipRect (L"Game", &game_window.cursor_clip);
      DescribeClipRect (L" IN ", &clip);

      if (confine)
      {
        clip.left   = game_window.actual.window.left + game_window.actual.client.left;
        clip.top    = game_window.actual.window.top  + game_window.actual.client.top;
        clip.right  = game_window.actual.window.left + game_window.actual.client.right;
        clip.bottom = game_window.actual.window.top  + game_window.actual.client.bottom;
      }

      // TODO: Transform clip rectangle using existing rectangle
      //
      else
      {
        SK_LOG2 ( ( L"Need to transform original clip rect..." ),
                    L"Cursor Mgr" );

        clip.left   = game_window.actual.window.left + game_window.actual.client.left;
        clip.top    = game_window.actual.window.top  + game_window.actual.client.top;
        clip.right  = game_window.actual.window.left + game_window.actual.client.right;
        clip.bottom = game_window.actual.window.top  + game_window.actual.client.bottom;
      }

      DescribeClipRect (L"OUT ", &clip);

      ClipCursor_Original (&clip);
    } 
  }

  // Unconfine Cursor
  else
    ClipCursor_Original (nullptr);
}

static int
WINAPI
GetSystemMetrics_Detour (_In_ int nIndex)
{
  SK_LOG_FIRST_CALL

  int nRet = GetSystemMetrics_Original (nIndex);

  SK_LOG4 ( ( L"GetSystemMetrics (%4lu) : %-5lu - %s",
                nIndex, nRet,
                  SK_SummarizeCaller ().c_str () ),
              L"Resolution" );

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
  SK_LOG_FIRST_CALL

  return GetSystemMetrics_Original (nIndex);
}

typedef UINT (WINAPI *GetRawInputBuffer_pfn)(
                               _Out_opt_ PRAWINPUT pData,
                               _Inout_   PUINT     pcbSize,
                               _In_      UINT      cbSizeHeader);

GetKeyState_pfn             GetKeyState_Original             = nullptr;
GetAsyncKeyState_pfn        GetAsyncKeyState_Original        = nullptr;
GetRawInputData_pfn         GetRawInputData_Original         = nullptr;
RegisterRawInputDevices_pfn RegisterRawInputDevices_Original = nullptr;
GetRawInputBuffer_pfn       GetRawInputBuffer_Original       = nullptr;

UINT
WINAPI
GetRawInputBuffer_Detour (_Out_opt_ PRAWINPUT pData,
                          _Inout_   PUINT     pcbSize,
                          _In_      UINT      cbSizeHeader)
{
  SK_LOG_FIRST_CALL

  if (SK_ImGui_Visible)
  {
    ImGuiIO& io =
      ImGui::GetIO ();

    bool filter = false;

    // Unconditional
    if (config.input.ui.capture)
      filter = true;

    // Only specific types
    if (io.WantCaptureKeyboard || io.WantCaptureMouse)
      filter = true;

    if (filter)
    {
      if (pData != nullptr)
      {
        const int max_items = (sizeof RAWINPUT / *pcbSize);
              int count     =                            0;
        uint8_t *pTemp      = (uint8_t *)
                                  new RAWINPUT [max_items];
        uint8_t *pInput     =                        pTemp;
        uint8_t *pOutput    =             (uint8_t *)pData;
        UINT     cbSize     =                     *pcbSize;
                  *pcbSize  =                            0;

        int       temp_ret  =
          GetRawInputBuffer_Original ( (RAWINPUT *)pTemp, &cbSize, cbSizeHeader );

        for (int i = 0; i < temp_ret; i++)
        {
          RAWINPUT* pItem = (RAWINPUT *)pInput;

          bool  remove = false;
          int  advance = pItem->header.dwSize;

          switch (pItem->header.dwType)
          {
            case RIM_TYPEKEYBOARD:
              if (io.WantCaptureKeyboard)
                remove = true;
              break;

            case RIM_TYPEMOUSE:
              if (io.WantCaptureMouse || config.input.ui.capture)
                remove = true;
              break;
          }

          if (config.input.ui.capture)
            remove = true;

          if (! remove) {
            memcpy (pOutput, pItem, pItem->header.dwSize);
             pOutput += advance;
                        ++count;
            *pcbSize += advance;
          }
          
          pInput += advance;
        }

        delete [] pTemp;

        return count;
      }
    }
  }

  return GetRawInputBuffer_Original (pData, pcbSize, cbSizeHeader);
}

UINT
WINAPI
GetRawInputData_Detour (_In_      HRAWINPUT hRawInput,
                        _In_      UINT      uiCommand,
                        _Out_opt_ LPVOID    pData,
                        _Inout_   PUINT     pcbSize,
                        _In_      UINT      cbSizeHeader)
{
  SK_LOG_FIRST_CALL

  int size = GetRawInputData_Original (hRawInput, uiCommand, pData, pcbSize, cbSizeHeader);

  if (uiCommand == RID_INPUT)
  {
    // Block keyboard input to the game while the console is active
    if (SK_Console::getInstance ()->isVisible ())
    {
      *pcbSize = 0;
      return     0;
    }

    // Block keyboard input to the game while it's in the background
    if (config.window.background_render && (! game_window.active))
    {
      *pcbSize = 0;
      return     0;
    }

    if (SK_ImGui_Visible && pData != nullptr)
    {
      bool filter = false;

      switch (((RAWINPUT *)pData)->header.dwType)
      {
        case RIM_TYPEMOUSE:
          if (config.input.ui.capture || ImGui_WantMouseCapture ())
            filter = true;
          break;
        case RIM_TYPEKEYBOARD:
          if (ImGui::GetIO ().WantCaptureKeyboard)
            filter = true;
          break;
        default:
          break;
      }

      if (filter)
      {
        *pcbSize = 0;
        return     0;
      }
    }
  }

  return size;
}

SHORT
WINAPI
GetAsyncKeyState_Detour (_In_ int vKey)
{
  SK_LOG_FIRST_CALL

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
  SK_LOG_FIRST_CALL

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

__declspec (noinline)
LRESULT
CALLBACK
SK_DetourWindowProc2 ( _In_  HWND   hWnd,
                       _In_  UINT   uMsg,
                       _In_  WPARAM wParam,
                       _In_  LPARAM lParam )
{
  UNREFERENCED_PARAMETER (hWnd);
  UNREFERENCED_PARAMETER (uMsg);
  UNREFERENCED_PARAMETER (wParam);
  UNREFERENCED_PARAMETER (lParam);

  return 1;
}

LRESULT
WINAPI
ImGui_WndProcHandler (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

extern void    ImGui_ImplDX11_InvalidateDeviceObjects (void);
extern bool    ImGui_ImplDX11_CreateDeviceObjects     (void);

__declspec (noinline)
LRESULT
CALLBACK
SK_DetourWindowProc ( _In_  HWND   hWnd,
                      _In_  UINT   uMsg,
                      _In_  WPARAM wParam,
                      _In_  LPARAM lParam )
{
  if (InterlockedExchangeAdd (&__SK_DLL_Ending, 0))
    return game_window.DefWindowProc (hWnd, uMsg, wParam, lParam);


  static bool first_run = true;

  if (first_run)
  {
    // Start unmuted (in case the game crashed in the background)
    if (config.window.background_mute)
      SK_SetGameMute (FALSE);

    first_run = false;
  }


  if (hWnd != game_window.hWnd) {
    if (game_window.hWnd != 0) {
      dll_log.Log ( L"[Window Mgr] New HWND detected in the window proc. used"
                    L" for rendering... (Old=%p, New=%p)",
                      game_window.hWnd, hWnd );
    }

    game_window.hWnd = hWnd;

    game_window.active        = true;
    game_window.game.style    = game_window.GetWindowLongPtr (game_window.hWnd, GWL_STYLE);
    game_window.actual.style  = game_window.GetWindowLongPtr (game_window.hWnd, GWL_STYLE);
    game_window.unicode       = IsWindowUnicode (game_window.hWnd) != FALSE;

    GetWindowRect_Original (game_window.hWnd, &game_window.game.window);
    GetClientRect_Original (game_window.hWnd, &game_window.game.client);
    GetWindowRect_Original (game_window.hWnd, &game_window.actual.window);
    GetClientRect_Original (game_window.hWnd, &game_window.actual.client);

    //SK_AdjustBorder        ();
    //SK_AdjustWindow        ();

    SK_InitWindow (hWnd, false);
  }


  static bool last_active = game_window.active;

  bool console_visible =
    SK_Console::getInstance ()->isVisible ();


  //
  // ImGui input handling
  //
  ImGui_WndProcHandler (hWnd, uMsg, wParam, lParam);

  if (SK_ImGui_Visible && game_window.active)
  {
    ImGuiIO& io =
      ImGui::GetIO ();

    bool keyboard_capture =
      ( (uMsg >= WM_KEYFIRST   && uMsg <= WM_KEYLAST)   &&
          io.WantCaptureKeyboard );
    bool mouse_capture =
      ( (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST) &&
          ImGui_WantMouseCapture () );

    // Capturing WM_INPUT messages would discard every type of input,
    //   not what we want generally.
    bool rawinput_capture =
      ( uMsg == WM_INPUT && ( io.WantCaptureKeyboard ||
                              io.WantCaptureMouse ) );

    if (config.input.ui.capture) {
      //keyboard_capture = (uMsg >= WM_KEYFIRST   && uMsg <= WM_KEYLAST);
      mouse_capture    = (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST);
      rawinput_capture = (uMsg == WM_INPUT);
    }

    bool filter = true;

    if ( keyboard_capture || mouse_capture || rawinput_capture )
    {
      if (! keyboard_capture)
      {
        if (uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN)
          SK_Console::getInstance ()->KeyDown (wParam & 0xFF, lParam);

        if (uMsg == WM_KEYUP || uMsg == WM_SYSKEYUP)
          SK_Console::getInstance ()->KeyUp (wParam & 0xFF, lParam);
      }

      if (uMsg == WM_INPUT)
      {
                        // Unconditional if true, conditional otherwise.
        filter        = config.input.ui.capture;

        RAWINPUT data;
        UINT     size = sizeof RAWINPUT;

        int      ret  =
          GetRawInputData_Original ((HRAWINPUT)lParam, RID_HEADER, &data, &size, sizeof (RAWINPUTHEADER) );

        if (ret)
        {
          switch (data.header.dwType)
          {
            case RIM_TYPEMOUSE:
              filter = io.WantCaptureMouse || config.input.ui.capture;
              break;

            case RIM_TYPEKEYBOARD:
              filter = io.WantCaptureKeyboard;
              break;
          }
        }

        if (filter)
          return game_window.DefWindowProc (hWnd, uMsg, wParam, lParam);
      }

      else
        return 0;
    }
  }


#if 0
  // HACK: Fallout 4 terminates abnormally at shutdown, meaning DllMain will
  //         never be called.
  //
  //       To properly shutdown the DLL, trap this window message instead of
  //         relying on DllMain (...) to be called.
  if ( hWnd             == game_window.hWnd &&
       uMsg             == WM_DESTROY       ){//&&
       ////(! lstrcmpW (SK_GetHostApp (), L"Fallout4.exe")) ) {
    //dll_log.Log ( L"[ SpecialK ] --- Invoking DllMain shutdown in response to "
                  //L"WM_DESTROY ---" );
    SK_SelfDestruct ();
    ExitProcess (0);
    return 0;
  }
#endif


  if ((uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST) && game_window.needsCoordTransform ())
  {
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

    if (! last_mouse.init)
    {
      if (config.input.cursor.timeout != 0)
        SetTimer (hWnd, last_mouse.timer_id, config.input.cursor.timeout / 2, nullptr);
      else
        SetTimer (hWnd, last_mouse.timer_id, 250/*USER_TIMER_MINIMUM*/, nullptr);

      last_mouse.init = true;
    }

    bool activation_event =
      (uMsg == WM_MOUSEMOVE) && (! SK_IsSteamOverlayActive ());

    // Don't blindly accept that WM_MOUSEMOVE actually means the mouse moved...
    if (activation_event)
    {
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

    else if ( uMsg   == WM_TIMER            &&
              wParam == last_mouse.timer_id &&
             (! SK_IsSteamOverlayActive ()) &&
              game_window.active )
    {
      if (true)//IsControllerPluggedIn (config.input.gamepad_slot))
        DeactivateCursor ();

      else
        ActivateCursor ();
    }
  }


  auto ActivateWindow = [](bool active = false)->
  void
    {
      bool state_changed =
        (game_window.active != active);

      game_window.active = active;

      if (state_changed) {
        SK_Console::getInstance ()->reset ();

        if (config.window.background_mute)
          SK_WindowManager::getInstance ()->muteGame ((! active));
      }


      if (active && state_changed)
      {
        GetCursorPos_Original (&game_window.cursor_pos);

        if (config.window.background_render)
        {
          if (! game_window.cursor_visible) {
            while (ShowCursor (FALSE) >= 0)
              ;
          }

          ClipCursor_Original (&game_window.cursor_clip);
        }
      }

      else if ((! active) && state_changed)
      {
        if (config.window.background_render)
        {
          game_window.cursor_visible =
            ShowCursor (TRUE) >= 1;

          while (ShowCursor (TRUE) < 0)
            ;

          ClipCursor_Original   ( nullptr );
          SetCursorPos_Original ( game_window.cursor_pos.x,
                                    game_window.cursor_pos.y );
        }
      }


      if (config.window.confine_cursor && state_changed)
      {
        if (active)
        {
          SK_LOG4 ( ( L"Confining Mouse Cursor" ),
                      L"Window Mgr" );

          ////// XXX: Is this really necessary? State should be consistent unless we missed
          //////        an event --- Write unit test?
          GetWindowRect_Original (game_window.hWnd, &game_window.actual.window);
          ClipCursor_Original    (&game_window.actual.window);
        }

        else
        {
          SK_LOG4 ( ( L"Unconfining Mouse Cursor" ),
                      L"Window Mgr" );

          ClipCursor_Original (nullptr);
        }
      }

      if (config.window.unconfine_cursor && state_changed)
      {
        SK_LOG4 ( ( L"Unconfining Mouse Cursor" ),
                    L"Window Mgr" );
        
        ClipCursor_Original (nullptr);
      }
   };


  switch (uMsg)
  {
    case WM_SYSCOMMAND:
      if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
        return 0;
      break;

    // Ignore (and physically remove) this event from the message queue if background_render = true
    case WM_MOUSEACTIVATE:
    {
      if ((HWND)wParam == game_window.hWnd)
      {
        ActivateWindow (true);

        if (config.window.background_render)
        {
          SK_LOG2 ( ( L"WM_MOUSEACTIVATE ==> Activate and Eat" ),
                      L"Window Mgr" );
          return MA_ACTIVATEANDEAT;
        }
      }

      else
      {
        ActivateWindow (false);

        // Game window was deactivated, but the game doesn't need to know this!
        //   in fact, it needs to be told the opposite.
        if (config.window.background_render)
        {
          SK_LOG2 ( ( L"WM_MOUSEACTIVATE (Other Window) ==> Activate" ),
                      L"Window Mgr" );
          return MA_ACTIVATE;
        }
      }
    } break;

    // Allow the game to run in the background
    case WM_ACTIVATEAPP:
    case WM_ACTIVATE:
    case WM_NCACTIVATE:
    {
      if (uMsg == WM_NCACTIVATE || WM_ACTIVATEAPP)
      {
        if (wParam == TRUE)
        {
          if (last_active == false)
            SK_LOG3 ( ( L"Application Activated (Non-Client)" ),
                        L"Window Mgr" );

          ActivateWindow (true);
        }

        else if (wParam == FALSE)
        {
          if (last_active == true)
            SK_LOG3 ( ( L"Application Deactivated (Non-Client)" ),
                        L"Window Mgr" );

          ActivateWindow (false);

          // We must fully consume one of these messages or audio will stop playing
          //   when the game loses focus, so do not simply pass this through to the
          //     default window procedure.
          if (config.window.background_render)
          {
            game_window.CallProc (hWnd, uMsg, TRUE, (LPARAM)hWnd);

            return 0;
          }
        }
      }

      else if (uMsg == WM_ACTIVATE)
      {
        switch (LOWORD (wParam))
        {
          case 1:  // WA_ACTIVE / TRUE
          case 2:  // WA_CLICKACTIVE
          default: // Unknown
          {
            if ((HWND)lParam != game_window.hWnd)
            {
              if (last_active == false) {
                SK_LOG2 ( ( L"Application Activated (WM_ACTIVATEAPP)" ),
                            L"Window Mgr" );
              }

              ActivateWindow (true);
            }
          } break;

          case 0: // WA_INACTIVE / FALSE
          {
            if ((HWND)lParam == game_window.hWnd)
            {
              if (last_active == true)
              {
                SK_LOG2 ( ( L"Application Deactivated (WM_ACTIVATEAPP)" ),
                            L"Window Mgr" );
              }

              ActivateWindow (false);

              if (config.window.background_render)
              {
                return 1;
              }
            }
          } break;
        }
      }
    } break;

    case WM_NCCALCSIZE:
      break;

    case WM_WINDOWPOSCHANGING:
    {
      LPWINDOWPOS wnd_pos = (LPWINDOWPOS)lParam;

      if (wnd_pos->flags ^ SWP_NOMOVE)
      {
        int width  = game_window.game.window.right  - game_window.game.window.left;
        int height = game_window.game.window.bottom - game_window.game.window.top;

        game_window.game.window.left   = wnd_pos->x;
        game_window.game.window.top    = wnd_pos->y;

        game_window.game.window.right  = wnd_pos->x + width;
        game_window.game.window.bottom = wnd_pos->y + height;
      }

      if (wnd_pos->flags ^ SWP_NOSIZE) {
        game_window.game.window.right  = game_window.game.window.left + wnd_pos->cx;
        game_window.game.window.bottom = game_window.game.window.top  + wnd_pos->cy;
      }

      if (config.window.borderless && (wnd_pos->flags & SWP_FRAMECHANGED))
        SK_AdjustBorder ();

      //game_window.game.client = game_window.game.window;

      // Filter this message
      if (config.window.borderless && config.window.fullscreen)
        return 0;
    } break;

    
    case WM_WINDOWPOSCHANGED:
    {
      // Unconditionally doing this tends to anger Obduction :)
      //ImGui_ImplDX11_InvalidateDeviceObjects ();

      LPWINDOWPOS wnd_pos = (LPWINDOWPOS)lParam;

      GetWindowRect_Original (game_window.hWnd, &game_window.actual.window);
      GetClientRect_Original (game_window.hWnd, &game_window.actual.client);

      //game_window.game.client = game_window.actual.client;
      //game_window.game.window = game_window.actual.window;

      if (((wnd_pos->flags ^ SWP_NOMOVE) || (wnd_pos->flags ^ SWP_NOSIZE)))
      {
        bool offset = false;

        // Test for user-defined position; if it exists, then we must
        //   respond to all WM_WINDOWPOSCHANGED messages indicating window movement
        if ( config.window.offset.x.absolute                 || config.window.offset.y.absolute ||
             ( config.window.offset.x.percent != 0.0f &&
               config.window.offset.x.percent != 0.000001f ) ||
             ( config.window.offset.y.percent != 0.0f &&
               config.window.offset.y.percent != 0.000001f )
           )
          offset = true;

        bool temp_override = false;

        // Prevent all of this craziness from resizing the window accidentally
        if (config.window.res.override.isZero ())
        {
          temp_override = true;

          RECT client;
          GetClientRect_Original (game_window.hWnd, &client);

          config.window.res.override.x = client.right  - client.left;
          config.window.res.override.y = client.bottom - client.top;
        }

        if (config.window.center)
          SK_AdjustWindow ();

        else if (offset                                                      && (wnd_pos->flags ^ SWP_NOMOVE))
          SK_AdjustWindow ();

        else if ((! (config.window.res.override.isZero () || temp_override)) && (wnd_pos->flags ^ SWP_NOSIZE))
          SK_AdjustWindow ();

        if (temp_override)
        {
          config.window.res.override.x = 0;
          config.window.res.override.y = 0;
        }

        if (config.window.unconfine_cursor)
          ClipCursor_Original (nullptr);

        else if (config.window.confine_cursor)
          ClipCursor_Original (&game_window.actual.window);
      }

      // Filter this message
      if (config.window.borderless && config.window.fullscreen)
        return 0;
    } break;


    case WM_SIZE:
      ImGui_ImplDX11_InvalidateDeviceObjects ();
      // Fallthrough to WM_MOVE

    case WM_MOVE:
      GetWindowRect_Original (game_window.hWnd, &game_window.actual.window);
      GetClientRect_Original (game_window.hWnd, &game_window.actual.client);

      if (config.window.unconfine_cursor)
        ClipCursor_Original (nullptr);
      else if (config.window.confine_cursor)
        ClipCursor_Original (&game_window.actual.window);


      // Filter this message
      if (config.window.borderless && config.window.fullscreen)
        return 0;

      break;

      case WM_SIZING:
      case WM_MOVING:
        if (config.window.unconfine_cursor)
          ClipCursor_Original (nullptr);

        // Filter this message
        if (config.window.borderless && config.window.fullscreen)
          return 0;
        break;


    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
      if (game_window.active)
      {
        if (SK_Console::getInstance ()->KeyDown (wParam & 0xFF, lParam) && (uMsg != WM_SYSKEYDOWN)) {
          return game_window.CallProc (hWnd, uMsg, wParam, lParam);
        }
      }
      break;

    case WM_KEYUP:
    case WM_SYSKEYUP:
      if (game_window.active)
      {
        if (SK_Console::getInstance ()->KeyUp (wParam & 0xFF, lParam) && (uMsg != WM_SYSKEYUP)) {
          return game_window.CallProc (hWnd, uMsg, wParam, lParam);
        }
      }
      break;

    case WM_MOUSEMOVE:
      if (! game_window.active)
        GetCursorPos_Original (&game_window.cursor_pos);
      break;
  }


  //
  // DO NOT HOOK THIS FUNCTION outside of SpecialK plug-ins, the ABI is not guaranteed
  //
  if (SK_DetourWindowProc2 (hWnd, uMsg, wParam, lParam)) {
    // Block keyboard input to the game while the console is visible
    if (console_visible /*|| ((! active) && uMsg != WM_SYSKEYDOWN)*/) {
      if (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST)
        return game_window.DefWindowProc (hWnd, uMsg, wParam, lParam);

      if (uMsg >= WM_KEYFIRST && uMsg <= WM_KEYLAST)
        return game_window.DefWindowProc (hWnd, uMsg, wParam, lParam);

      // Block RAW Input
      if (console_visible && uMsg == WM_INPUT)
        return game_window.DefWindowProc (hWnd, uMsg, wParam, lParam);
    }
  }
  
  else {
    return 0;
  }


  return game_window.CallProc (hWnd, uMsg, wParam, lParam);
}

#include <SpecialK/core.h>

BOOL WINAPI RegisterRawInputDevices_Detour (
  _In_ PCRAWINPUTDEVICE pRawInputDevices,
  _In_ UINT             uiNumDevices,
  _In_ UINT             cbSize
)
{
  //SK_LOG_FIRST_CALL

#if 0
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
#endif

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
SK_InitWindow (HWND hWnd, bool fullscreen_exclusive)
{
  if (game_window.GetWindowLongPtr == nullptr) {
    SK_InstallWindowHook (hWnd);
  }

  GetWindowRect_Original (hWnd, &game_window.game.window);
  GetClientRect_Original (hWnd, &game_window.game.client);

  GetWindowRect_Original (hWnd, &game_window.actual.window);
  GetClientRect_Original (hWnd, &game_window.actual.client);

  GetCursorPos_Original  (&game_window.cursor_pos);

  if (game_window.GetWindowLongPtr == nullptr)
    return;

  game_window.actual.style =
    game_window.GetWindowLongPtr ( hWnd, GWL_STYLE );

  game_window.actual.style_ex =
    game_window.GetWindowLongPtr ( hWnd, GWL_EXSTYLE );



  bool has_border  = SK_WindowManager::StyleHasBorder (
                       game_window.actual.style
                     );

  if (has_border) {
    game_window.border_style    = game_window.actual.style;
    game_window.border_style_ex = game_window.actual.style_ex;
  }

  game_window.game.style    = game_window.actual.style;
  game_window.game.style_ex = game_window.actual.style_ex;

  game_window.hWnd = hWnd;


  if (! fullscreen_exclusive)
  {      
    // Next, adjust the border and/or window location if the user
    //   wants an override
    SK_ResetWindow ();

    if (game_window.actual.style & WS_VISIBLE)
      SK_RealizeForegroundWindow (hWnd);
  }
}

void
SK_InstallWindowHook (HWND hWnd)
{
  static volatile LONG installed = FALSE;

  if (InterlockedCompareExchange (&installed, TRUE, FALSE))
    return;

  game_window.unicode = IsWindowUnicode (hWnd) != FALSE;

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
    GetClassLongPtrW  ( hWnd, GCLP_WNDPROC ) :
                                 (WNDPROC)
    GetClassLongPtrA  ( hWnd, GCLP_WNDPROC );

  WNDPROC wnd_proc = (WNDPROC)
    game_window.GetWindowLongPtr ( hWnd, GWLP_WNDPROC );

  game_window.WndProc_Original = nullptr;

  wchar_t wszClassName [256] = { L'\0' };
  GetClassNameW ( hWnd, wszClassName, 256 );

  dll_log.Log ( L"[Window Mgr] Hooking the Window Procedure for "
                L"%s Window Class ('%s') :: (ClassProc: %p, WndProc: %p)",
                  game_window.unicode ? L"Unicode" : L"ANSI",
                    wszClassName,
                      class_proc,
                        wnd_proc );

  bool hook_classfunc = false;

  // Compat Hack: EverQuest (hook classfunc)
  //
  //   Input will be processed in-game, but not during server select.
  //
  if (wcsstr (L"eqgame.exe", SK_GetHostApp ()))
    hook_classfunc = true;

  if (hook_classfunc)
  {
    if ( MH_OK ==
           MH_CreateHook ( (LPVOID)class_proc,
                                   SK_DetourWindowProc,
                        (LPVOID *)&game_window.WndProc_Original )
      )
    {
      MH_EnableHook ((LPVOID)class_proc);

      dll_log.Log (L"[Window Mgr]  >> Hooked ClassProc.");

      game_window.hooked = true;
    }

    else if ( MH_OK == MH_CreateHook ( (LPVOID)wnd_proc,
                                               SK_DetourWindowProc,
                                    (LPVOID *)&game_window.WndProc_Original )
            )
    {
      MH_EnableHook ((LPVOID)wnd_proc);

      dll_log.Log (L"[Window Mgr]  >> Hooked WndProc.");

      game_window.hooked = true;
    }
  }

  else
  {
    dll_log.Log ( L"[Window Mgr]  >> Hooking was impossible; installing new "
                  L"window procedure instead (this may be undone "
                  L"by other software)." );

    game_window.WndProc_Original =
      (WNDPROC)wnd_proc;

    game_window.SetWindowLongPtr ( hWnd, GWLP_WNDPROC, (LONG_PTR)SK_DetourWindowProc );

    if (game_window.unicode)
      SetClassLongPtrW ( hWnd, GCLP_WNDPROC, (LONG_PTR)SK_DetourWindowProc );
    else
      SetClassLongPtrA ( hWnd, GCLP_WNDPROC, (LONG_PTR)SK_DetourWindowProc );

    if (game_window.unicode) {
      DWORD dwStyle = GetClassLongW (hWnd, GCL_STYLE);
      dwStyle &= (~CS_DBLCLKS);
      SetClassLongW ( hWnd, GCL_STYLE, dwStyle );
    }
    else {
      DWORD dwStyle = GetClassLongA (hWnd, GCL_STYLE);
      dwStyle &= (~CS_DBLCLKS);
      SetClassLongA ( hWnd, GCL_STYLE, dwStyle );
    }

    game_window.hooked = false;
  }

  SK_InitWindow (hWnd);

  SK_ICommandProcessor* cmd =
    SK_GetCommandProcessor ();

  cmd->AddVariable ("Cursor.Manage",           SK_CreateVar (SK_IVariable::Boolean, (bool *)&config.input.cursor.manage));
  cmd->AddVariable ("Cursor.Timeout",          SK_CreateVar (SK_IVariable::Int,     (int  *)&config.input.cursor.timeout));
  cmd->AddVariable ("Cursor.KeysActivate",     SK_CreateVar (SK_IVariable::Boolean, (bool *)&config.input.cursor.keys_activate));

  cmd->AddVariable ("Window.BackgroundRender", SK_CreateVar (SK_IVariable::Boolean, (bool *)&config.window.background_render));

  cmd->AddVariable ("ImGui.Visible",           SK_CreateVar (SK_IVariable::Boolean, (bool *)&SK_ImGui_Visible));
}

HWND
__stdcall
SK_GetGameWindow (void)
{
  ////SK_WINDOW_LOG_CALL3 ();

  return game_window.hWnd;
}


void
SK_HookWinAPI (void)
{
  // Initialize the Window Manager
  SK_WindowManager::getInstance ();

  SK_CreateDLLHook2 ( L"user32.dll", "GetRawInputData",
                     GetRawInputData_Detour,
           (LPVOID*)&GetRawInputData_Original );

  SK_CreateDLLHook2 ( L"user32.dll", "GetRawInputBuffer",
                     GetRawInputBuffer_Detour,
           (LPVOID*)&GetRawInputBuffer_Original );

  SK_CreateDLLHook2 ( L"user32.dll", "GetKeyState",
                     GetKeyState_Detour,
           (LPVOID*)&GetKeyState_Original );

  SK_CreateDLLHook2 ( L"user32.dll", "GetCursorPos",
                     GetCursorPos_Detour,
           (LPVOID*)&GetCursorPos_Original );

  SK_CreateDLLHook2 ( L"user32.dll", "GetCursorInfo",
                     GetCursorInfo_Detour,
           (LPVOID*)&GetCursorInfo_Original );

  SK_CreateDLLHook2 ( L"user32.dll", "SetCursor",
                     SetCursor_Detour,
           (LPVOID*)&SetCursor_Original );

  SK_CreateDLLHook2 ( L"user32.dll", "SetCursorPos",
                     SetCursorPos_Detour,
           (LPVOID*)&SetCursorPos_Original );

  SK_CreateDLLHook2 ( L"user32.dll", "SetPhysicalCursorPos",
                     SetPhysicalCursorPos_Detour,
           (LPVOID*)&SetPhysicalCursorPos_Original );

  SK_CreateDLLHook2 ( L"user32.dll", "SendInput",
                     SendInput_Detour,
           (LPVOID*)&SendInput_Original );

  SK_CreateDLLHook2 ( L"user32.dll", "mouse_event",
                     mouse_event_Detour,
           (LPVOID*)&mouse_event_Original );

  SK_CreateDLLHook2 ( L"user32.dll", "GetAsyncKeyState",
                     GetAsyncKeyState_Detour,
           (LPVOID*)&GetAsyncKeyState_Original );

  SK_CreateDLLHook2 ( L"user32.dll", "RegisterRawInputDevices",
                     RegisterRawInputDevices_Detour,
           (LPVOID*)&RegisterRawInputDevices_Original );

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

#if 0
  SK_CreateDLLHook2 ( L"user32.dll", "GetWindowRect",
                      GetWindowRect_Detour,
            (LPVOID*)&GetWindowRect_Original );

  SK_CreateDLLHook2 ( L"user32.dll", "GetClientRect",
                      GetClientRect_Detour,
            (LPVOID*)&GetClientRect_Original );
#else
    GetWindowRect_Original =
      (GetWindowRect_pfn)
        GetProcAddress ( GetModuleHandleW (L"user32.dll"), 
                         "GetWindowRect" );

    GetClientRect_Original =
      (GetClientRect_pfn)
        GetProcAddress ( GetModuleHandleW (L"user32.dll"), 
                         "GetClientRect" );
#endif
  SK_CreateDLLHook2 ( L"user32.dll", "AdjustWindowRect",
                        AdjustWindowRect_Detour,
             (LPVOID *)&AdjustWindowRect_Original );

  SK_CreateDLLHook2 ( L"user32.dll", "AdjustWindowRectEx",
                        AdjustWindowRectEx_Detour,
             (LPVOID *)&AdjustWindowRectEx_Original );

  SK_CreateDLLHook2 ( L"user32.dll",+
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