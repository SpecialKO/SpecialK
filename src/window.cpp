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

#include <Windows.h>
#include <SpecialK/window.h>

#include <SpecialK/hooks.h>
#include <SpecialK/core.h>
#include <SpecialK/console.h>
#include <SpecialK/sound.h>

#include <cstdint>

#include <string>
#include <algorithm>
#include <numeric>
#include <unordered_map>
#include <unordered_set>
#include <SpecialK/steam_api.h>
#include <SpecialK/framerate.h>

#include <SpecialK/log.h>
#include <SpecialK/config.h>
#include <SpecialK/command.h>
#include <SpecialK/utility.h>
#include <SpecialK/osd/text.h>

#undef  SK_LOG_FIRST_CALL
#define SK_LOG_FIRST_CALL { static bool called = false; if (! called) { SK_LOG0 ( (L"[!] > First Call: %34hs", __FUNCTION__), L"Window Mgr" ); called = true; } }

#include <mmsystem.h>
#pragma comment (lib, "winmm.lib")

#include <comdef.h>
#include <process.h>

#include <windowsx.h>

#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS
#include <imgui/imgui.h>

#define SK_BORDERLESS    ( WS_VISIBLE | WS_POPUP | WS_MINIMIZEBOX | WS_SYSMENU )
#define SK_BORDERLESS_EX ( WS_EX_APPWINDOW )                     // ^^^ Keep the window's icon unchanged

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


// Avoid static storage in the callback function
struct {
  POINTS pos      = {     }; // POINT (Short) - Not POINT plural ;)
  DWORD  sampled  =     0UL;
  bool   cursor   =    true;

  int    init     =   false;
  UINT   timer_id = 0x68993;
} last_mouse;


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
GetCursorPos_pfn       GetCursorPos_Original       = nullptr;
GetCursorInfo_pfn      GetCursorInfo_Original      = nullptr;
SetCursorPos_pfn       SetCursorPos_Original       = nullptr;
SendInput_pfn          SendInput_Original          = nullptr;
mouse_event_pfn        mouse_event_Original        = nullptr;

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

//SetClassLong_pfn       SetClassLongW_Original      = nullptr;
//SetClassLong_pfn       SetClassLongA_Original      = nullptr;
//GetClassLong_pfn       GetClassLongW_Original      = nullptr;
//GetClassLong_pfn       GetClassLongA_Original      = nullptr;
//SetClassLongPtr_pfn    SetClassLongPtrW_Original   = nullptr;
//SetClassLongPtr_pfn    SetClassLongPtrA_Original   = nullptr;
//GetClassLongPtr_pfn    GetClassLongPtrW_Original   = nullptr;
//GetClassLongPtr_pfn    GetClassLongPtrA_Original   = nullptr;

GetSystemMetrics_pfn   GetSystemMetrics_Original   = nullptr;

GetWindowRect_pfn      GetWindowRect_Original      = nullptr;
GetClientRect_pfn      GetClientRect_Original      = nullptr;


struct window_message_dispatch_s {
public:
  bool ProcessMessage (HWND hWnd, UINT uMsg, WPARAM& wParam, LPARAM& lParam, LRESULT *pRet = nullptr)
  {
    switch (uMsg)
    {
      case WM_WINDOWPOSCHANGING:
        return On_WINDOWPOSCHANGING (hWnd, wParam, lParam, pRet);
      case WM_WINDOWPOSCHANGED:
        return On_WINDOWPOSCHANGED  (hWnd, wParam, lParam, pRet);

      case WM_MOUSEACTIVATE:
        return On_MOUSEACTIVATE     (hWnd, wParam, lParam, pRet);

      case WM_ACTIVATEAPP:
        return On_ACTIVATEAPP       (hWnd, wParam, lParam, pRet);
      case WM_ACTIVATE:
        return On_ACTIVATE          (hWnd, wParam, lParam, pRet);
      case WM_NCACTIVATE:
        return On_NCACTIVATE        (hWnd, wParam, lParam, pRet);

      case WM_MOUSEMOVE:
        return On_MOUSEMOVE         (hWnd, wParam, lParam, pRet);

      case WM_ENTERSIZEMOVE:
        return On_ENTERSIZEMOVE     (hWnd, wParam, lParam, pRet);
      case WM_EXITSIZEMOVE:
        return On_EXITSIZEMOVE      (hWnd, wParam, lParam, pRet);
    }

    return false;
  }

  std::unordered_map <HWND, bool> active_windows;
  std::unordered_set <HWND>       moving_windows;

protected:
  bool On_WINDOWPOSCHANGING (HWND hWnd, WPARAM& wParam, LPARAM& lParam, LRESULT *pRet);
  bool On_WINDOWPOSCHANGED  (HWND hWnd, WPARAM& wParam, LPARAM& lParam, LRESULT *pRet);

  bool On_MOUSEACTIVATE     (HWND hWnd, WPARAM& wParam, LPARAM& lParam, LRESULT *pRet);
  bool On_ACTIVATEAPP       (HWND hWnd, WPARAM& wParam, LPARAM& lParam, LRESULT *pRet);
  bool On_ACTIVATE          (HWND hWnd, WPARAM& wParam, LPARAM& lParam, LRESULT *pRet);
  bool On_NCACTIVATE        (HWND hWnd, WPARAM& wParam, LPARAM& lParam, LRESULT *pRet);

  bool On_MOUSEMOVE         (HWND hWnd, WPARAM& wParam, LPARAM& lParam, LRESULT *pRet);

  bool On_ENTERSIZEMOVE     (HWND hWnd, WPARAM& wParam, LPARAM& lParam, LRESULT *pRet);
  bool On_EXITSIZEMOVE      (HWND hWnd, WPARAM& wParam, LPARAM& lParam, LRESULT *pRet);
} wm_dispatch;

bool override_window_rects = false;

// TODO: Refactor that structure above into this class ;)
//
//  Also, combine a few of the input manager settings with window manager
class SK_WindowManager : public SK_IVariableListener
{
public:
  static constexpr bool StyleHasBorder (DWORD_PTR style)
  {
    return (  ( style == 0x0            ) ||
              ( style  &  WS_BORDER     ) ||
              ( style  &  WS_THICKFRAME ) ||
              ( style  &  WS_DLGFRAME   )    );
  }

  virtual bool OnVarChange (SK_IVariable* var, void* val) override
  {
    if (var == confine_cursor_)
    {
      if (val != nullptr)
      {
        config.window.confine_cursor =
          *static_cast <bool *> (val);

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
        config.window.unconfine_cursor =
          *static_cast <bool *> (val);

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
          config.window.center =
            *static_cast <bool *> (val);

        else if (var == x_offset_)
        {
          if ( *static_cast <int *> (val) >= -4096 &&
               *static_cast <int *> (val) <= 4096     )
          {
            config.window.offset.x.absolute = *static_cast <signed int *> (val);
            config.window.offset.x.percent  = 0.0f;
          }
        }

        else if (var == y_offset_)
        {
          if ( *static_cast <int *> (val) >= -4096 &&
               *static_cast <int *> (val) <=  4096    )
          {
            config.window.offset.y.absolute = *static_cast <signed int *> (val);
            config.window.offset.y.percent  = 0.0f;
          }
        }

        else if (var == x_off_pct_)
        {
          if ( *static_cast <float *> (val) > -1.0f &&
               *static_cast <float *> (val) <  1.0f    )
          {
            config.window.offset.x.absolute = 0;
            config.window.offset.x.percent = *static_cast <float *> (val);
          }
        }

        else if (var == y_off_pct_)
        {
          if ( *static_cast <float *> (val) > -1.0f &&
               *static_cast <float *> (val) <  1.0f    )
          {
            config.window.offset.y.absolute = 0;
            config.window.offset.y.percent  = *static_cast <float *> (val);
          }
        }

        else if ( var == borderless_ && 
         *static_cast <bool *> (val) != config.window.borderless )
        {
          config.window.borderless = *static_cast <bool *> (val);

          SK_AdjustBorder ();
        }

        else if (var == fullscreen_) 
        {
          static int x = config.window.res.override.x;
          static int y = config.window.res.override.y;

          if ( config.window.fullscreen != *static_cast <bool *> (val) &&
             ( config.window.borderless || (config.window.fullscreen) ) )
          {
            config.window.fullscreen = *static_cast <bool *> (val);

            static bool first_set         = true;
            static RECT last_known_client = { };
            static RECT last_known_window = { };

                   RECT client            = { };

            GetClientRect_Original (game_window.hWnd, &client);

            HMONITOR hMonitor =
              MonitorFromWindow ( game_window.hWnd,
                                    MONITOR_DEFAULTTONEAREST );

            MONITORINFO mi  = { };
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
              game_window.game.client   = RECT { last_known_client.left,  last_known_client.top,
                                                 last_known_client.right, last_known_client.bottom };
              game_window.game.window   = RECT { last_known_window.left,  last_known_window.top,
                                                 last_known_window.right, last_known_window.bottom };

              game_window.actual.client = RECT { last_known_client.left,  last_known_client.top,
                                                 last_known_client.right, last_known_client.bottom };
              game_window.actual.window = RECT { last_known_window.left,  last_known_window.top,
                                                 last_known_window.right, last_known_window.bottom };

              if (! config.window.unconfine_cursor)
              {
                if (! wm_dispatch.moving_windows.count (game_window.hWnd))
                {
                  RECT clip =
                    game_window.actual.window;

                  ClipCursor_Original (&clip);
                }
              }

              else
                ClipCursor_Original (nullptr);
            }

            // Go Fullscreen (Stretch Window to Fill)
            else
            {
              config.window.res.override.x = mi.rcMonitor.right  - mi.rcMonitor.left;
              config.window.res.override.y = mi.rcMonitor.bottom - mi.rcMonitor.top;

              game_window.actual.client = RECT { 0,                                    0,
                                                 mi.rcMonitor.right-mi.rcMonitor.left, mi.rcMonitor.bottom-mi.rcMonitor.top };
              game_window.actual.window = RECT { mi.rcMonitor.left,  mi.rcMonitor.top,
                                                 mi.rcMonitor.right, mi.rcMonitor.bottom };
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

        else if (var == x_override_ || var == y_override_)
        {
          if ((! config.window.borderless) || config.window.fullscreen)
            return false;

          if (var == x_override_)
          {
            config.window.res.override.x = *static_cast <unsigned int *> (val);

            // We cannot allow one variable to remain 0 while the other becomes
            //   non-zero, so just make the window a square temporarily.
            if (config.window.res.override.y == 0)
              config.window.res.override.y = config.window.res.override.x;
          }

          else if (var == y_override_)
          {
            config.window.res.override.y = *static_cast <unsigned int *> (val);

            // We cannot allow one variable to remain 0 while the other becomes
            //   non-zero, so just make the window a square temporarily.
            if (config.window.res.override.x == 0)
              config.window.res.override.x = config.window.res.override.y;
          }

          // We have to override BOTH variables to 0 at the same time, or the window
          //   will poof! :P
          if (*static_cast <unsigned int *> (val) == 0)
          {
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
          }

          else
          {
            SK_AdjustWindow ();
            return true;
          }
        }

#if 1
        SK_AdjustWindow ();
#else
        if (var != center_window_)
        {
          static char szCmd [64] = { };
          snprintf (szCmd, 63, "Window.Center %d", config.window.center);

          DeferCommand (szCmd);
        }

        if (var != borderless_)
        {
          static char szCmd [64] = { };
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
        config.window.background_mute = *static_cast <bool *> (val);

        if (config.window.background_mute && (! game_window.active))
        {
          muteGame (true);
        }

        else if (!config.window.background_mute)
        {
          muteGame (false);
        }

        return true;
      }
    }

    else if (var == res_override_)
    {
      unsigned int x = 65535;
      unsigned int y = 65535;

         char szTemp    [31] = { };

      if (val != nullptr) {
        strncat (szTemp, *static_cast <char **> (val), 31);
        sscanf  (szTemp, "%ux%u", &x, &y);
      }

      if ((x > 320 && x < 16384 && y > 240 && y < 16384) || (x == 0 && y == 0))
      {
        config.window.res.override.x = x;
        config.window.res.override.y = y;

        SK_AdjustWindow ();

        auto *pszRes =
          static_cast <char *> (((SK_IVarStub <char *> *)var)->getValuePointer ());

        snprintf (pszRes, 31, "%ux%u", x, y);

        return true;
      }

      else
      {
        auto *pszRes =
          static_cast <char *> (((SK_IVarStub <char *> *)var)->getValuePointer ());

        snprintf (pszRes, 31, "INVALID");

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
      SK_CreateVar (SK_IVariable::Boolean, &config.window.borderless,      this);

    background_mute_ =
      SK_CreateVar (SK_IVariable::Boolean, &config.window.background_mute, this);

    center_window_ =
      SK_CreateVar (SK_IVariable::Boolean, &config.window.center,          this);

    fullscreen_ =
      SK_CreateVar (SK_IVariable::Boolean, &config.window.fullscreen,      this);

    x_override_ =
      SK_CreateVar (SK_IVariable::Int,     &config.window.res.override.x,  this);

    y_override_ =
      SK_CreateVar (SK_IVariable::Int,     &config.window.res.override.y,  this);

    res_override_ =
      SK_CreateVar (SK_IVariable::String,  override_res,                   this);

    // Don't need to listen for this event, actually.
    fix_mouse_ =
      SK_CreateVar (SK_IVariable::Boolean, &config.window.res.override.fix_mouse);

    x_offset_ =
      SK_CreateVar (SK_IVariable::Int,     &config.window.offset.x.absolute, this);

    y_offset_ =
      SK_CreateVar (SK_IVariable::Int,     &config.window.offset.y.absolute, this);

    x_off_pct_ =
      SK_CreateVar (SK_IVariable::Float,   &config.window.offset.x.percent,  this);

    y_off_pct_ =
      SK_CreateVar (SK_IVariable::Float,   &config.window.offset.y.percent,  this);

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

  void finishFullscreen (bool fullscreen)
  {
    state_.fullscreen.actual = fullscreen;
  }

  void finishBorderless (bool borderless)
  {
    state_.borderless.actual = borderless;
  }

  bool makeFullscreen (bool fullscreen)
  {
    state_.fullscreen.set (fullscreen);
    return state_.fullscreen.get ();
  }

  bool makeBorderless (bool borderless)
  {
    state_.borderless.set (borderless);
    return state_.borderless.get ();
  }

  bool isChanging (void)
  {
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

    bool changing (void)
    {
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

  char override_res [32] = { };

private:
  static SK_WindowManager* pWindowManager;
};

auto ActivateWindow =[&](HWND hWnd, bool active = false)
{
  bool state_changed =
    (game_window.active != active);

  game_window.active = active;

  if (state_changed)
  {
    SK_Console::getInstance ()->reset ();

    if (config.window.background_mute)
      SK_WindowManager::getInstance ()->muteGame ((! active));

    // Keep Unity games from crashing at startup when forced into FULLSCREEN
    //
    //  ... also prevents a game from staying topmost when you Alt+Tab
    //

    if ( active && config.display.force_fullscreen &&
         ( static_cast <int> (SK_GetCurrentRenderBackend ().api)  &
           static_cast <int> (SK_RenderAPI::D3D9               )
         )
       )
    {
      SetWindowLongPtrW    (game_window.hWnd, GWL_EXSTYLE,
       ( GetWindowLongPtrW (game_window.hWnd, GWL_EXSTYLE) & ~(WS_EX_TOPMOST | WS_EX_NOACTIVATE)
       ) | WS_EX_APPWINDOW );
      //SetWindowPos      (game_window.hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSENDCHANGING | SWP_NOMOVE     | SWP_NOSIZE     |
      //                                                                 SWP_FRAMECHANGED   | SWP_DEFERERASE | SWP_NOCOPYBITS |
      //                                                                 SWP_ASYNCWINDOWPOS | SWP_SHOWWINDOW | SWP_NOACTIVATE );
      //SetWindowPos      (game_window.hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOSENDCHANGING | SWP_NOMOVE     | SWP_NOSIZE     |
      //                                                           SWP_FRAMECHANGED   | SWP_DEFERERASE | SWP_NOCOPYBITS |
      //                                                           SWP_ASYNCWINDOWPOS | SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOZORDER );

      extern void
      SK_D3D9_TriggerReset (bool);
    
      SK_D3D9_TriggerReset (false);
    }
  }


  if (active && state_changed)
  {
    if ((! SK_GetCurrentRenderBackend ().fullscreen_exclusive) && config.window.background_render)
    {
      if (! game_window.cursor_visible)
      {
        while (ShowCursor (FALSE) >= 0)
          ;
      }

      if (! wm_dispatch.moving_windows.count (game_window.hWnd))
        ClipCursor_Original (&game_window.cursor_clip);
    }
  }

  else if ((! active) && state_changed)
  {
    if ((! SK_GetCurrentRenderBackend ().fullscreen_exclusive) && config.window.background_render)
    {
      game_window.cursor_visible =
        ShowCursor (TRUE) >= 1;

      while (ShowCursor (TRUE) < 0)
        ;

      ClipCursor_Original (nullptr);
    }
  }


  if (config.window.confine_cursor && state_changed)
  {
    if (active)
    {
      SK_LOG4 ( ( L"Confining Mouse Cursor" ),
                  L"Window Mgr" );

      if (! wm_dispatch.moving_windows.count (game_window.hWnd))
      {
        ////// XXX: Is this really necessary? State should be consistent unless we missed
        //////        an event --- Write unit test?
        GetWindowRect_Original (game_window.hWnd, &game_window.actual.window);
        ClipCursor_Original    (&game_window.actual.window);
      }
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

  if (state_changed)
    SK_ImGui_Cursor.activateWindow (active);

  wm_dispatch.active_windows [hWnd] = active;
};

extern volatile LONG SK_bypass_dialog_active;

bool
window_message_dispatch_s::On_ENTERSIZEMOVE (HWND hWnd, WPARAM&, LPARAM&, LRESULT*)
{
  ClipCursor (nullptr);

  moving_windows.emplace (hWnd);

  return false;
}

bool
window_message_dispatch_s::On_EXITSIZEMOVE (HWND hWnd, WPARAM&, LPARAM&, LRESULT*)
{
  if ( moving_windows.count (hWnd) ||
       moving_windows.count (game_window.hWnd) )
  {
    if (moving_windows.count (hWnd))
      moving_windows.erase (hWnd);
    else
      moving_windows.erase (game_window.hWnd);

    SK_AdjustWindow ();

    if (config.window.unconfine_cursor)
      ClipCursor_Original (nullptr);

    if (config.window.confine_cursor)
      ClipCursor_Original (&game_window.actual.window);
  }

  return false;
}

bool
window_message_dispatch_s::On_MOUSEMOVE (HWND, WPARAM&, LPARAM&, LRESULT*)
{
  if (! game_window.active)
    GetCursorPos_Original (&game_window.cursor_pos);

  return false;
}

bool
window_message_dispatch_s::On_MOUSEACTIVATE (HWND hWnd, WPARAM& wParam, LPARAM&, LRESULT* pRet)
{
  SK_RenderBackend& rb = SK_GetCurrentRenderBackend ();

  if ( reinterpret_cast <HWND> (wParam) == game_window.hWnd )
  {
    ActivateWindow (hWnd, true);

    if ((! rb.fullscreen_exclusive) && config.window.background_render)
    {
      SK_LOG2 ( ( L"WM_MOUSEACTIVATE ==> Activate and Eat" ),
                  L"Window Mgr" );
      if (pRet) *pRet = MA_ACTIVATEANDEAT;

      return true;
    }
  }

  else
  {
    ActivateWindow (hWnd, false);

    // Game window was deactivated, but the game doesn't need to know this!
    //   in fact, it needs to be told the opposite.
    if ((! rb.fullscreen_exclusive) && config.window.background_render)
    {
      SK_LOG2 ( ( L"WM_MOUSEACTIVATE (Other Window) ==> Activate" ),
                  L"Window Mgr" );
      if (pRet) *pRet = MA_ACTIVATE;

      return true;
    }

    rb.fullscreen_exclusive = false;
  }

  return false;
}


bool
window_message_dispatch_s::On_NCACTIVATE (HWND hWnd, WPARAM& wParam, LPARAM& lParam, LRESULT*)
{
  if (wParam != FALSE)
  {
    if (active_windows [hWnd] == false)
      SK_LOG3 ( ( L"Application Activated (Non-Client)" ),
                  L"Window Mgr" );

    ActivateWindow (hWnd, true);
  }

  else
  {
    if (active_windows [hWnd] == true)
      SK_LOG3 ( ( L"Application Deactivated (Non-Client)" ),
                  L"Window Mgr" );

    ActivateWindow (hWnd, false);

    // We must fully consume one of these messages or audio will stop playing
    //   when the game loses focus, so do not simply pass this through to the
    //     default window procedure.
    if ( (! SK_GetCurrentRenderBackend ().fullscreen_exclusive) &&
            config.window.background_render
       )
    {
      wParam = TRUE,
      lParam = reinterpret_cast <LPARAM> (hWnd);
    }

    SK_GetCurrentRenderBackend ().fullscreen_exclusive = false;
  }

  return false;
}

bool
window_message_dispatch_s::On_ACTIVATEAPP (HWND hWnd, WPARAM& wParam, LPARAM& lParam, LRESULT*)
{
  if (wParam != FALSE)
  {
    if (active_windows [hWnd] == false)
      SK_LOG3 ( ( L"Application Activated (Non-Client)" ),
                  L"Window Mgr" );

    ActivateWindow (hWnd, true);
  }

  else
  {
    if (active_windows [hWnd] == true)
      SK_LOG3 ( ( L"Application Deactivated (Non-Client)" ),
                  L"Window Mgr" );

    ActivateWindow (hWnd, false);

    // We must fully consume one of these messages or audio will stop playing
    //   when the game loses focus, so do not simply pass this through to the
    //     default window procedure.
    if ( (! SK_GetCurrentRenderBackend ().fullscreen_exclusive) &&
            config.window.background_render
       )
    {
      wParam = TRUE,
      lParam = reinterpret_cast <LPARAM> (hWnd);
    }

    SK_GetCurrentRenderBackend ().fullscreen_exclusive = false;
  }

  return false;
}

bool
window_message_dispatch_s::On_ACTIVATE (HWND hWnd, WPARAM& wParam, LPARAM& lParam, LRESULT* pRet)
{
  const wchar_t* source   = L"UNKKNOWN";
  bool           activate = false;

  bool last_active = active_windows [hWnd];

  switch (LOWORD (wParam))
  {
    case WA_ACTIVE:
    case WA_CLICKACTIVE:
    default: // Unknown
    {
      activate = reinterpret_cast <HWND> (lParam) != game_window.hWnd;
      source   = LOWORD (wParam) == 1 ? L"(WM_ACTIVATE [ WA_ACTIVE ])" :
                                        L"(WM_ACTIVATE [ WA_CLICKACTIVE ])";
      ActivateWindow (hWnd, activate);
    } break;

    case WA_INACTIVE:
    {
      activate = ( lParam                           == 0                ) ||
                 ( reinterpret_cast <HWND> (lParam) == game_window.hWnd );
      source   = L"(WM_ACTIVATE [ WA_INACTIVE ])";
      ActivateWindow (hWnd, activate);
    } break;
  }

  switch (last_active)
  {
    case true:
      if (! activate)
      {
        SK_LOG2 ( ( L"Application Deactivated %s", source ),
                    L"Window Mgr" );
      }
      break;

    case false:
      if (activate)
      {
        SK_LOG2 ( ( L"Application Activated %s", source ),
                    L"Window Mgr" );
      }
      break;
  }

  if ((! SK_GetCurrentRenderBackend ().fullscreen_exclusive) && config.window.background_render)
  {
    if (pRet) *pRet = 1;
    return true;
  }

  if (! activate)
    SK_GetCurrentRenderBackend ().fullscreen_exclusive = false;

  return false;
}

LRESULT
CALLBACK
SK_DetourWindowProc ( _In_  HWND   hWnd,
                      _In_  UINT   uMsg,
                      _In_  WPARAM wParam,
                      _In_  LPARAM lParam );

// Fix for Trails of Cold Steel
LRESULT
SK_COMPAT_SafeCallProc (sk_window_s* pWin, HWND hWnd_, UINT Msg, WPARAM wParam, LPARAM lParam);

sk_window_s       game_window;
bool              caught_register = false; // If we initialized early enough, skip installing a custom hook

extern bool SK_ImGui_Visible;

BOOL
CALLBACK
SK_EnumWindows (HWND hWnd, LPARAM lParam)
{
  window_t& win =
    *reinterpret_cast <window_t *> (lParam);

  DWORD proc_id = 0;

  GetWindowThreadProcessId (hWnd, &proc_id);

  if (win.proc_id == proc_id)
  {
    //if ( GetWindow (hWnd, GW_OWNER) == (HWND)nullptr )
    {
      DWORD dwStyle, dwStyleEx;

      switch (IsWindowUnicode (hWnd))
      {
        case TRUE:
          dwStyle   = GetWindowLongW_Original (hWnd, GWL_STYLE);
          dwStyleEx = GetWindowLongW_Original (hWnd, GWL_EXSTYLE);
          break;

        default:
        case FALSE:
          dwStyle   = GetWindowLongA_Original (hWnd, GWL_STYLE);
          dwStyleEx = GetWindowLongA_Original (hWnd, GWL_EXSTYLE);
          break;
      }

      bool    SKIM                   = false;
      wchar_t wszName [MAX_PATH + 2] = { };

      if (RealGetWindowClassW (hWnd, wszName, MAX_PATH))
      {
        if (StrStrIW (wszName, L"SKIM"))
          SKIM = true;
      }

      if ( (dwStyle & (WS_VISIBLE | WS_MINIMIZEBOX)) || SKIM )
      {
        win.root = hWnd;
        return FALSE;
      }
    }
  }

  return TRUE;
}

window_t
SK_FindRootWindow (DWORD proc_id)
{
  window_t win;

  win.proc_id  = proc_id;
  win.root     = nullptr;

  EnumWindows (SK_EnumWindows, (LPARAM)&win);

  return win;
}

void SK_AdjustBorder (void);
void SK_AdjustWindow (void);

//
// If we did this from the render thread, we would deadlock most games
//
auto DeferCommand = [&] (const char* szCommand)
{
  CreateThread ( nullptr,
                   0x00,
                [](LPVOID user) ->
    DWORD
      {
        SK_GetCommandProcessor ()->ProcessCommandLine (
           static_cast <const char *> (user)
        );

        CloseHandle (GetCurrentThread ());

        return 0;
      }, static_cast <LPVOID> (const_cast <char *> (szCommand)),
      0x00,
    nullptr
  );
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
    float width  = 64.0f,
          height = 64.0f;
  } in, out;

  lpPoint->x -= ( static_cast <LONG> ( game_window.actual.window.left ) );
  lpPoint->y -= ( static_cast <LONG> ( game_window.actual.window.top  ) );

  in.width    = static_cast <float> ( game_window.actual.client.right  -
                                      game_window.actual.client.left   );
  in.height   = static_cast <float> ( game_window.actual.client.bottom -
                                      game_window.actual.client.top    );

  out.width   = static_cast <float> ( game_window.game.client.right    -
                                      game_window.game.client.left     );
  out.height  = static_cast <float> ( game_window.game.client.bottom   -
                                      game_window.game.client.top      );

  float x     = 2.0f * (static_cast <float> (lpPoint->x) / in.width ) - 1.0f;
  float y     = 2.0f * (static_cast <float> (lpPoint->y) / in.height) - 1.0f;

  lpPoint->x  = static_cast <LONG> ( (x * out.width  + out.width) / 2.0f +
                                      game_window.coord_remap.offset.x );

  lpPoint->y  = static_cast <LONG> ( (y * out.height + out.height) / 2.0f +
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
    float width  = 64.0f,
          height = 64.0f;
  } in, out;

  lpPoint->x -= ( static_cast <LONG> ( game_window.coord_remap.offset.x +
                                       game_window.game.window.left     +
                                       game_window.game.client.left ) );
  lpPoint->y -= ( static_cast <LONG> ( game_window.coord_remap.offset.y +
                                       game_window.game.window.top      +
                                       game_window.game.client.top ) );

  in.width   = static_cast <float> ( game_window.game.client.right  -
                                     game_window.game.client.left   );
  in.height  = static_cast <float> ( game_window.game.client.bottom -
                                     game_window.game.client.top    );

  out.width  = static_cast <float> ( game_window.actual.client.right -
                                     game_window.actual.client.left  );
  out.height = static_cast <float> ( game_window.actual.client.bottom -
                                     game_window.actual.client.top    );

  float x    = 2.0f * (static_cast <float> (lpPoint->x) / in.width ) - 1.0f;
  float y    = 2.0f * (static_cast <float> (lpPoint->y) / in.height) - 1.0f;

  lpPoint->x = static_cast <LONG> ( (x * out.width  + out.width) / 2.0f + 
                                     game_window.coord_remap.offset.x );

  lpPoint->y = static_cast <LONG> ( (y * out.height + out.height) / 2.0f +
                                     game_window.coord_remap.offset.y );
}

ULONG   game_mouselook = 0;
int     game_x, game_y;

#if 0
BOOL
WINAPI
GetCursorPos_Detour (LPPOINT lpPoint)
{
  BOOL ret = GetCursorPos_Original (lpPoint);

  // Correct the cursor position for Aspect Ratio
  if (game_window.needsCoordTransform ())
  {
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
  if (game_window.needsCoordTransform ())
  {
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
  else
    return ClipCursor_Original (nullptr);

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
    POINT top_left     = { std::numeric_limits <LONG>::max (), std::numeric_limits <LONG>::max () };
    POINT bottom_right = { std::numeric_limits <LONG>::min (), std::numeric_limits <LONG>::min () };

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


  if (game_window.active && (! wm_dispatch.moving_windows.count (game_window.hWnd)) && lpRect != nullptr)
  {
    return ClipCursor_Original (&game_window.cursor_clip);
  }

  else
  {
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

  if (! init)
  {
    InitializeCriticalSection (&cs_center);
    init = true;
  }

  CreateThread ( nullptr, 0,
    [](LPVOID user) ->
      DWORD {
  EnterCriticalSection (&cs_center);

  BOOL  remember_pos = (user != nullptr);
  POINT mouse        = { 0, 0 };

  if (GetCursorPos_Original != nullptr)
    GetCursorPos_Original (&mouse);

  struct {
    struct {
      float percent  = 0.0f;
      int   absolute =   0L;
    } x,y;
  } offsets;

  if (! remember_pos)
  {
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

  if (! remember_pos)
  {
    config.window.offset.x.absolute = offsets.x.absolute;
    config.window.offset.y.absolute = offsets.y.absolute;

    config.window.offset.x.percent  = offsets.x.percent;
    config.window.offset.y.percent  = offsets.y.percent;
  }

  LeaveCriticalSection (&cs_center);

  CloseHandle (GetCurrentThread ());

  return 0;
       // Don't dereference this, it's actually a boolean
    }, reinterpret_cast <LPVOID> (static_cast <uintptr_t> (remember_pos)), 0x0, nullptr );
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

  if (hWnd == game_window.hWnd)
    SK_WINDOW_LOG_CALL1 ();

  if (config.display.force_windowed)
    if (hWndInsertAfter == HWND_TOPMOST)
      hWndInsertAfter = HWND_TOP;



  if (config.render.dxgi.safe_fullscreen) uFlags |= SWP_ASYNCWINDOWPOS;



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
       hWnd == game_window.hWnd && config.window.fullscreen )
  {
    //lpRect->left = 0; lpRect->right  = config.window.res.override.x;
    //lpRect->top  = 0; lpRect->bottom = config.window.res.override.y;

    HMONITOR hMonitor =
      MonitorFromWindow ( game_window.hWnd,
                            MONITOR_DEFAULTTOPRIMARY );

    MONITORINFO mi   = {         };
    mi.cbSize        = sizeof (mi);
    GetMonitorInfo (hMonitor, &mi);

    lpRect->left  = 0;
    lpRect->right = mi.rcMonitor.right - mi.rcMonitor.left;

    lpRect->top    = 0;
    lpRect->bottom = mi.rcMonitor.bottom - mi.rcMonitor.top;

//    *lpRect = game_window.game.client;

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
       config.window.fullscreen )
  {
    //lpRect->left = 0; lpRect->right  = config.window.res.override.x;
    //lpRect->top  = 0; lpRect->bottom = config.window.res.override.y;

    HMONITOR hMonitor =
      MonitorFromWindow ( game_window.hWnd,
                            MONITOR_DEFAULTTOPRIMARY );

    MONITORINFO mi   = {         };
    mi.cbSize        = sizeof (mi);
    GetMonitorInfo (hMonitor, &mi);

    lpRect->left  = mi.rcMonitor.left;
    lpRect->right = mi.rcMonitor.right;

    lpRect->top    = mi.rcMonitor.top;
    lpRect->bottom = mi.rcMonitor.bottom;

    //*lpRect = game_window.game.window;

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
              L"{%4li,%4li / %4li,%4li}, 0x%04X, %li ) - %s",
                lpRect->left, lpRect->top,
                  lpRect->right, lpRect->bottom,
                    dwStyle, bMenu,
                      SK_SummarizeCaller ().c_str () ),
                L"Window Mgr" );

  if (SK_GetCurrentGameID () == SK_GAME_ID::ZeroEscape)
    return TRUE;

  // Override if forcing Fullscreen Borderless
  //
  if (config.window.fullscreen && config.window.borderless && (! bMenu))
  {
    HMONITOR hMonitor =
      MonitorFromWindow ( game_window.hWnd,
                            MONITOR_DEFAULTTONEAREST );

    MONITORINFO mi   = {         };
    mi.cbSize        = sizeof (mi);
    GetMonitorInfo (hMonitor, &mi);

    lpRect->left  = mi.rcMonitor.left;
    lpRect->right = mi.rcMonitor.right;

    lpRect->top    = mi.rcMonitor.top;
    lpRect->bottom = mi.rcMonitor.bottom;

    return TRUE;
  }

  return AdjustWindowRect_Original (lpRect, dwStyle, bMenu);
}

void SK_SetWindowStyle   (DWORD_PTR dwStyle_ptr,   SetWindowLongPtr_pfn pDispatchFunc = nullptr);
void SK_SetWindowStyleEx (DWORD_PTR dwStyleEx_ptr, SetWindowLongPtr_pfn pDispatchFunc = nullptr);

BOOL
WINAPI
AdjustWindowRectEx_Detour (
    _Inout_ LPRECT lpRect,
    _In_    DWORD  dwStyle,
    _In_    BOOL   bMenu,
    _In_    DWORD  dwExStyle )
{
  SK_LOG1 ( ( L"AdjustWindowRectEx ( "
              L"{%4li,%4li / %4li,%4li}, 0x%04X, %li, 0x%04X ) - %s",
                lpRect->left,       lpRect->top,
                  lpRect->right, lpRect->bottom,
                    dwStyle, bMenu,
                      dwExStyle,
                        SK_SummarizeCaller ().c_str () ),
               L"Window Mgr" );

  if (SK_GetCurrentGameID () == SK_GAME_ID::ZeroEscape)
    return TRUE;

  // Override if forcing Fullscreen Borderless
  //
  if (config.window.fullscreen && config.window.borderless && (! bMenu))
  {
    HMONITOR hMonitor =
      MonitorFromWindow ( game_window.hWnd,
                            MONITOR_DEFAULTTONEAREST );

    MONITORINFO mi   = {         };
    mi.cbSize        = sizeof (mi);
    GetMonitorInfo (hMonitor, &mi);

    lpRect->left  = mi.rcMonitor.left;
    lpRect->right = mi.rcMonitor.right;

    lpRect->top    = mi.rcMonitor.top;
    lpRect->bottom = mi.rcMonitor.bottom;

    return TRUE;
  }

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
          static_cast <LONG_PTR> (dwNewLong);

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

        SK_SetWindowStyle ( static_cast      <DWORD_PTR>            (game_window.actual.style),
                            reinterpret_cast <SetWindowLongPtr_pfn> (pOrigFunc) );

        return static_cast <LONG> (game_window.actual.style);
      }

      case GWL_EXSTYLE:
      {
        game_window.game.style_ex =
          static_cast <LONG_PTR> (dwNewLong);

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

        SK_SetWindowStyleEx ( static_cast      <DWORD_PTR>            (game_window.actual.style_ex),
                              reinterpret_cast <SetWindowLongPtr_pfn> (pOrigFunc) );

        return static_cast <LONG> (game_window.actual.style_ex);
      }

      case GWLP_WNDPROC:
      {
        if (game_window.hooked)
        {
          //game_window.WndProc_Original = reinterpret_cast <WNDPROC> (dwNewLong);
          //return dwNewLong;
        }
      } break;
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

#if 0
// Convenience function since there are so damn many
//   variants of these functions that need to be hooked.
__forceinline
LONG
WINAPI
SetClassLong_Marshall (
  _In_ SetClassLong_pfn pOrigFunc,
  _In_ HWND             hWnd,
  _In_ int              nIndex,
  _In_ LONG             dwNewLong )
{
  return pOrigFunc (hWnd, nIndex,dwNewLong);
}

DECLSPEC_NOINLINE
LONG
WINAPI
SetClassLongA_Detour (
  _In_ HWND hWnd,
  _In_ int  nIndex,
  _In_ LONG dwNewLong
)
{
  SK_LOG_FIRST_CALL

  return SetClassLong_Marshall (
    SetClassLongA_Original,
      hWnd,
        nIndex,
          dwNewLong
  );
}

DECLSPEC_NOINLINE
LONG
WINAPI
SetClassLongPtrA_Detour (
  _In_ HWND     hWnd,
  _In_ int      nIndex,
  _In_ LONG_PTR dwNewLong
)
{
  SK_LOG_FIRST_CALL

  return SetClassLong_Marshall (
    SetClassLongPtrA_Original,
      hWnd,
        nIndex,
          dwNewLong
  );
}

DECLSPEC_NOINLINE
LONG
WINAPI
SetClassLongPtrW_Detour (
  _In_ HWND     hWnd,
  _In_ int      nIndex,
  _In_ LONG_PTR dwNewLong
)
{
  SK_LOG_FIRST_CALL

  return SetClassLong_Marshall (
    SetClassLongPtrW_Original,
      hWnd,
        nIndex,
          dwNewLong
  );
}
#endif

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
        return static_cast <LONG> (game_window.game.style);
      case GWL_EXSTYLE:
        return static_cast <LONG> (game_window.game.style_ex);
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
  //dll_log.Log (L"SetWindowLongPtr: Idx=%li, Val=%X -- Caller: %s", nIndex, dwNewLong, SK_GetCallerName ().c_str ());

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

        SK_SetWindowStyle (static_cast <LONG> (game_window.actual.style), pOrigFunc);

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

        SK_SetWindowStyleEx (static_cast <LONG> (game_window.actual.style_ex), pOrigFunc);

        return game_window.actual.style_ex;
      }

      case GWLP_WNDPROC:
      {
        if (game_window.hooked)
        {
          //game_window.WndProc_Original = reinterpret_cast <WNDPROC> (dwNewLong);
          //return dwNewLong;
        }
      } break;
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
SK_SetWindowStyle (DWORD_PTR dwStyle_ptr, SetWindowLongPtr_pfn pDispatchFunc)
{
  // Ensure that the border style is sane
  if (dwStyle_ptr == game_window.border_style)
  {
    game_window.border_style |= WS_CAPTION     | WS_SYSMENU | WS_POPUP |
                                WS_MINIMIZEBOX | WS_VISIBLE;
    dwStyle_ptr               = game_window.border_style;
  }

  // Clear the high-bits
  DWORD dwStyle = (dwStyle_ptr & 0xFFFFFFFF);

  // Minimal sane set of extended window styles for sane rendering
  dwStyle |= (WS_VISIBLE | WS_SYSMENU) | WS_POPUP | WS_MINIMIZEBOX;
  dwStyle &= (~WS_DISABLED);

  game_window.actual.style = dwStyle;

  if (pDispatchFunc == nullptr)
    pDispatchFunc = game_window.SetWindowLongPtr;


  pDispatchFunc ( game_window.hWnd,
                    GWL_STYLE,
                      game_window.actual.style );
}

void
SK_SetWindowStyleEx (DWORD_PTR dwStyleEx_ptr, SetWindowLongPtr_pfn pDispatchFunc)
{
  // Clear the high-bits
  DWORD dwStyleEx = (dwStyleEx_ptr & 0xFFFFFFFF);

  // Minimal sane set of extended window styles for sane rendering
  dwStyleEx |=   WS_EX_APPWINDOW;
  dwStyleEx &= ~(WS_EX_NOACTIVATE | WS_EX_TRANSPARENT | WS_EX_LAYOUTRTL |
                 WS_EX_RIGHT      | WS_EX_RTLREADING);

  game_window.actual.style_ex = dwStyleEx;

  if (pDispatchFunc == nullptr)
    pDispatchFunc = game_window.SetWindowLongPtr;


  pDispatchFunc ( game_window.hWnd,
                    GWL_EXSTYLE,
                      game_window.actual.style_ex );
}

RECT
SK_ComputeClientSize (void)
{
  bool use_override = (! config.window.res.override.isZero ());

  if (use_override)
  {
    return RECT { 0L, 0L,
                    static_cast   <LONG> (config.window.res.override.x),
                      static_cast <LONG> (config.window.res.override.y)
                };
  }

  RECT ret =
    game_window.actual.client;

  GetClientRect_Original ( game_window.hWnd, &ret );

  ret = { 0, 0,
            ret.right    - ret.left,
              ret.bottom - ret.top };

  return ret;
}

POINT
SK_ComputeClientOrigin (void)
{
  RECT window = { },
       client = { };

  GetWindowRect_Original ( game_window.hWnd, &window );
  GetClientRect_Original ( game_window.hWnd, &client );

  return POINT { window.left  + client.left,
                   window.top + client.top };
}

bool
SK_IsRectTooBigForDesktop (RECT wndRect)
{
  HMONITOR hMonitor =
    MonitorFromWindow ( game_window.hWnd,
                          MONITOR_DEFAULTTONEAREST );

  MONITORINFO mi   = {         };
  mi.cbSize        = sizeof (mi);
  GetMonitorInfo (hMonitor, &mi);

  int win_width      = wndRect.right       - wndRect.left;
  int win_height     = wndRect.bottom      - wndRect.top;

  if (! config.window.res.override.isZero ())
  {
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

  RECT orig_client =
    { };

  GetClientRect_Original (game_window.hWnd, &orig_client);

  RECT new_client =
    SK_ComputeClientSize ();

  RECT  new_window = new_client;
  POINT origin     { new_client.left, new_client.top };//SK_ComputeClientOrigin ();

  ClientToScreen (game_window.hWnd, &origin);

  SK_SetWindowStyle   ( game_window.actual.style    );
  SK_SetWindowStyleEx ( game_window.actual.style_ex );

  if ( AdjustWindowRectEx_Original ( &new_window,
                                       static_cast <DWORD>     (game_window.actual.style),
                                         FALSE,
                                           static_cast <DWORD> (game_window.actual.style_ex)
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
                          SWP_NOZORDER     | SWP_NOREPOSITION   |
                          SWP_FRAMECHANGED | SWP_NOSENDCHANGING | SWP_SHOWWINDOW  );

    GetWindowRect (game_window.hWnd, &game_window.actual.window);
    GetClientRect (game_window.hWnd, &game_window.actual.client);
  }

  game_window.game.window = game_window.actual.window;
  game_window.game.client = game_window.actual.client;
}

void
SK_ResetWindow (void)
{
  SK_RenderBackend& rb = SK_GetCurrentRenderBackend ();

  if (! (config.window.borderless || config.window.center))
    return;

  if (config.window.center && (! ( (config.window.fullscreen  &&
                                    config.window.borderless) ||
                                    rb.fullscreen_exclusive) ) )
    return;

  static CRITICAL_SECTION cs_reset;
  static volatile LONG    init = FALSE;

  if (! InterlockedCompareExchange (&init, 1, 0))
  {
    InitializeCriticalSection (&cs_reset);
  }

  CreateThread (nullptr, 0,
    [](LPVOID user) ->
    DWORD
    {
      UNREFERENCED_PARAMETER (user);

      SleepEx (33, FALSE);

      GetWindowRect (game_window.hWnd, &game_window.actual.window);
      GetClientRect (game_window.hWnd, &game_window.actual.client);

      ////
      ////// Force a sane set of window styles initially
      SK_SetWindowStyle   ( GetWindowLongPtrW (game_window.hWnd, GWL_STYLE)   );
      SK_SetWindowStyleEx ( GetWindowLongPtrW (game_window.hWnd, GWL_EXSTYLE) );
      
      EnterCriticalSection (&cs_reset);

      if (config.window.borderless)
      {
        SK_AdjustBorder ();

        // XXX: Why can't the call to SK_AdjustWindow (...) suffice?
        if (config.window.fullscreen)
          SK_GetCommandProcessor ()->ProcessCommandLine ("Window.Fullscreen 1");
      }

      else if (config.window.center)
        SK_AdjustWindow ();

      LeaveCriticalSection (&cs_reset);

      CloseHandle (GetCurrentThread ());

      return 0;
    }, nullptr, 0x00, nullptr);

}

// KNOWN ISSUES: 1. Do not move window using title bar while override res is enabled
//
//
void
SK_AdjustWindow (void)
{
  SK_WINDOW_LOG_CALL3 ();

  if (game_window.GetWindowLongPtr == nullptr)
    return;

  if (! game_window.active)
    return;

  HMONITOR hMonitor =
    MonitorFromWindow ( game_window.hWnd,
                          MONITOR_DEFAULTTONEAREST );

  MONITORINFO mi   = {         };
  mi.cbSize        = sizeof (mi);
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
                                    SWP_NOSENDCHANGING | SWP_NOZORDER |
                                    SWP_NOREPOSITION   | SWP_ASYNCWINDOWPOS | SWP_SHOWWINDOW );

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
          static_cast <LONG> (game_window.actual.style),
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

      RECT client_before = 
        game_window.game.client;

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
    if (render_width == real_width)
    {
      mon_width       = real_width;
      mi.rcWork.right = mi.rcMonitor.right;
      mi.rcWork.left  = mi.rcMonitor.left;
    }

    // No adjustment if the window is full-height
    if (render_height == real_height)
    {
      mon_height       = real_height;
      mi.rcWork.top    = mi.rcMonitor.top;
      mi.rcWork.bottom = mi.rcMonitor.bottom;
    }

    LONG win_width  = std::min (mon_width,  render_width);
    LONG win_height = std::min (mon_height, render_height);

    // Eliminate relative epsilon from screen percentage offset coords;
    //   otherwise we may accidentally move the window.
    bool nomove = config.window.offset.isZero       ();
    bool nosize = config.window.res.override.isZero ();

    SK_RenderBackend& rb = SK_GetCurrentRenderBackend ();

    // We will offset coordinates later; move the window to the top-left
    //   origin first.
    if (config.window.center && (! ( (config.window.fullscreen &&
                                      config.window.borderless)  ||
                                      rb.fullscreen_exclusive  ) ) )
    {
      game_window.actual.window.left   = 0;
      game_window.actual.window.top    = 0;
      game_window.actual.window.right  = win_width;
      game_window.actual.window.bottom = win_height;
      nomove                           = false; // Centering requires moving ;)
    }

    else
    {
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

    if (config.window.center && (! ( (config.window.fullscreen &&
                                      config.window.borderless)  ||
                                      rb.fullscreen_exclusive  ) ) )
    {
      SK_LOG4 ( ( L"Center --> (%li,%li)",
                    mi.rcWork.right - mi.rcWork.left,
                      mi.rcWork.bottom - mi.rcWork.top ),
                    L"Window Mgr" );

      if (x_offset < 0)
      {
        game_window.actual.window.left  -= (win_width / 2);
        game_window.actual.window.right -= (win_width / 2);
      }

      if (y_offset < 0)
      {
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
          static_cast <LONG> (game_window.actual.style),
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
                                      SWP_NOSENDCHANGING | SWP_NOZORDER   |
                                      SWP_NOREPOSITION   | SWP_SHOWWINDOW |
                            (nomove ? SWP_NOMOVE : 0x00) |
                            (nosize ? SWP_NOSIZE : 0x00) );
    }

    GetWindowRect_Original (game_window.hWnd, &game_window.actual.window);
    GetClientRect_Original (game_window.hWnd, &game_window.actual.client);

    GetWindowRect_Original (game_window.hWnd, &game_window.game.window);
    GetClientRect_Original (game_window.hWnd, &game_window.game.client);



    wchar_t wszBorderDesc [128] = { };

    bool has_border = SK_WindowManager::StyleHasBorder (game_window.actual.style);

    // Summarize the border
    if (SK_WindowManager::StyleHasBorder (game_window.actual.style))
    {
      _swprintf ( wszBorderDesc,
                   L"(Frame = %lipx x %lipx, Title = %lipx)",
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

  auto DescribeRect = [&](RECT* lpRect)
  {
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

    auto DescribeClipRect = [&](const wchar_t* wszName, RECT* lpRect)
    {
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

      if (! wm_dispatch.moving_windows.count (game_window.hWnd))
        ClipCursor_Original (&clip);
      else
        ClipCursor_Original (nullptr);
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

  SK_LOG4 ( ( L"GetSystemMetrics (%4li) : %-5li - %s",
                nIndex, nRet,
                  SK_SummarizeCaller ().c_str () ),
              L"Resolution" );

#if 0
  if (config.window.borderless)
  {
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
#endif

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



using TranslateMessage_pfn =
  BOOL (WINAPI *)(_In_ const MSG *lpMsg);

using DispatchMessageW_pfn =
  LRESULT (WINAPI *)(
    _In_ const MSG *lpmsg
  );

using DispatchMessageA_pfn =
  LRESULT (WINAPI *)(
    _In_ const MSG *lpmsg
  );

using GetMessageA_pfn =
  BOOL (WINAPI *)( LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax );
using GetMessageW_pfn =
  BOOL (WINAPI *)( LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax );

using PeekMessageW_pfn =
  BOOL (WINAPI *)(
    _Out_    LPMSG lpMsg,
    _In_opt_ HWND  hWnd,
    _In_     UINT  wMsgFilterMin,
    _In_     UINT  wMsgFilterMax,
    _In_     UINT  wRemoveMsg
  );

using PeekMessageA_pfn =
  BOOL (WINAPI *)(
    _Out_    LPMSG lpMsg,
    _In_opt_ HWND  hWnd,
    _In_     UINT  wMsgFilterMin,
    _In_     UINT  wMsgFilterMax,
    _In_     UINT  wRemoveMsg
  );

TranslateMessage_pfn TranslateMessage_Original = nullptr;

PeekMessageW_pfn     PeekMessageW_Original     = nullptr;
PeekMessageA_pfn     PeekMessageA_Original     = nullptr;
GetMessageA_pfn      GetMessageA_Original      = nullptr;
GetMessageW_pfn      GetMessageW_Original      = nullptr;
DispatchMessageW_pfn DispatchMessageW_Original = nullptr;
DispatchMessageA_pfn DispatchMessageA_Original = nullptr;

BOOL
WINAPI
TranslateMessage_Detour (_In_ const MSG *lpMsg)
{
  if (SK_ImGui_WantTextCapture ())
  {
    switch (lpMsg->message)
    {
      case WM_KEYDOWN:
      case WM_SYSKEYDOWN:
      case WM_IME_KEYDOWN:

      // This shouldn't happen considering this function is supposed
      //   to generate this message, but better to be on the safe side.
      case WM_CHAR:
      case WM_MENUCHAR:
      {
        return TRUE;
      } break;
    }
  }

  return TranslateMessage_Original (lpMsg);
}

LPMSG last_message = nullptr;

static HMODULE hModSteamOverlay = nullptr;

bool
SK_EarlyDispatchMessage (LPMSG lpMsg, bool remove, bool peek = false)
{
  LRESULT lRet = 0;
  //if (wm_dispatch.ProcessMessage (lpMsg->hwnd, lpMsg->message, lpMsg->wParam, lpMsg->lParam, &lRet))
  //{
  //  if (remove)
  //    lpMsg->message = WM_NULL;
  //
  //  return true;
  //}

  if ( SK_ImGui_HandlesMessage (lpMsg, remove, peek) )
  {
    if (remove)
      lpMsg->message = WM_NULL;

    return true;
  }

  return false;
}

BOOL
WINAPI
PeekMessageW_Detour (
  _Out_    LPMSG lpMsg,
  _In_opt_ HWND  hWnd,
  _In_     UINT  wMsgFilterMin,
  _In_     UINT  wMsgFilterMax,
  _In_     UINT  wRemoveMsg )
{
  if (hModSteamOverlay == nullptr)
  {
    hModSteamOverlay = GetModuleHandle
#ifdef _WIN64
    (L"GameOverlayRenderer64.dll");
#else
    (L"GameOverlayRenderer.dll");
#endif
  }

  if (SK_GetCallingDLL () == hModSteamOverlay)
  {
    return PeekMessageW_Original ( lpMsg, hWnd,
                                   wMsgFilterMin,                           wMsgFilterMax,                          wRemoveMsg );
  }

  SK_LOG_FIRST_CALL

  if (config.render.dxgi.safe_fullscreen && (IsWindowUnicode (GetActiveWindow ())))
    wRemoveMsg |= PM_REMOVE;

  MSG msg = { };

  if (PeekMessageW_Original (&msg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg))
  {
    *lpMsg = msg;

    // Avoid processing the message twice if it's marshalled because
    //   stupid software called the wrong version of this function ;)
    if (IsWindowUnicode (msg.hwnd))
    {
      if (SK_EarlyDispatchMessage (&msg, true, true))
      {
        if (! (wRemoveMsg & PM_REMOVE))
        {
          PeekMessageW_Original ( &msg, lpMsg->hwnd, lpMsg->message,
                                                     lpMsg->message, PM_REMOVE | PM_NOYIELD );
        }

        if (lpMsg->message == WM_INPUT)
          DefWindowProcW (lpMsg->hwnd, lpMsg->message, lpMsg->wParam, lpMsg->lParam);

        lpMsg->message = WM_NULL;
        lpMsg->pt.x    = 0; lpMsg->pt.y = 0;

        return FALSE;
      }
    }

    return TRUE;
  }

  return FALSE;
}

BOOL
WINAPI
PeekMessageA_Detour (
  _Out_    LPMSG lpMsg,
  _In_opt_ HWND  hWnd,
  _In_     UINT  wMsgFilterMin,
  _In_     UINT  wMsgFilterMax,
  _In_     UINT  wRemoveMsg )
{
  if (hModSteamOverlay == nullptr)
  {
    hModSteamOverlay = GetModuleHandle
#ifdef _WIN64
    (L"GameOverlayRenderer64.dll");
#else
    (L"GameOverlayRenderer.dll");
#endif
  }

  if (SK_GetCallingDLL () == hModSteamOverlay)
  {
    return PeekMessageA_Original ( lpMsg, hWnd,
                                   wMsgFilterMin,                           wMsgFilterMax,                          wRemoveMsg );
  }

  SK_LOG_FIRST_CALL

  if (config.render.dxgi.safe_fullscreen && (! IsWindowUnicode (GetActiveWindow ())))
    wRemoveMsg |= PM_REMOVE;

  MSG msg = { };

  if (PeekMessageA_Original (&msg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg))
  {
    *lpMsg = msg;

    // Avoid processing the message twice if it's marshalled because
    //   stupid software called the wrong version of this function ;)
    if (! IsWindowUnicode (lpMsg->hwnd))
    {
      if (SK_EarlyDispatchMessage (&msg, true, true))
      {
        if (! (wRemoveMsg & PM_REMOVE))
        {
          PeekMessageA_Original ( &msg, lpMsg->hwnd, lpMsg->message,
                                                     lpMsg->message, PM_REMOVE | PM_NOYIELD );
        }

        if (lpMsg->message == WM_INPUT)
          DefWindowProcA (lpMsg->hwnd, lpMsg->message, lpMsg->wParam, lpMsg->lParam);

        lpMsg->message = WM_NULL;
        lpMsg->pt.x    = 0; lpMsg->pt.y = 0;

        return FALSE;
      }
    }

    return TRUE;
  }

  return FALSE;
}

BOOL
WINAPI
GetMessageA_Detour (LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax)
{
  if (hModSteamOverlay == nullptr)
  {
    hModSteamOverlay = GetModuleHandle
#ifdef _WIN64
    (L"GameOverlayRenderer64.dll");
#else
    (L"GameOverlayRenderer.dll");
#endif
  }

  if (SK_GetCallingDLL () == hModSteamOverlay)
  {
    return GetMessageA_Original ( lpMsg, hWnd,                                       wMsgFilterMin,                                   wMsgFilterMax );
  }

  SK_LOG_FIRST_CALL

  if (! GetMessageA_Original (lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax))
    return FALSE;

  if (lpMsg->hwnd == game_window.hWnd)
    SK_EarlyDispatchMessage (lpMsg, true);

  return TRUE;
}

BOOL
WINAPI
GetMessageW_Detour (LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax)
{
  if (hModSteamOverlay == nullptr)
  {
    hModSteamOverlay = GetModuleHandle
#ifdef _WIN64
    (L"GameOverlayRenderer64.dll");
#else
    (L"GameOverlayRenderer.dll");
#endif
  }

  if (SK_GetCallingDLL () == hModSteamOverlay)
  {
    return GetMessageW_Original ( lpMsg, hWnd,                                       wMsgFilterMin,                                   wMsgFilterMax );
  }

  SK_LOG_FIRST_CALL

  if (! GetMessageW_Original (lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax))
    return FALSE;

  if (lpMsg->hwnd == game_window.hWnd)
    SK_EarlyDispatchMessage (lpMsg, true);

  return TRUE;
}

LRESULT
WINAPI
DispatchMessageW_Detour (_In_ const MSG *lpMsg)
{
  if (hModSteamOverlay == nullptr)
  {
    hModSteamOverlay = GetModuleHandle
#ifdef _WIN64
    (L"GameOverlayRenderer64.dll");
#else
    (L"GameOverlayRenderer.dll");
#endif
  }

  if (SK_GetCallingDLL () == hModSteamOverlay)
  {
    return DispatchMessageW_Original ( lpMsg );
  }

  SK_LOG_FIRST_CALL

  if (lpMsg->hwnd == game_window.hWnd)
  {
    if ( SK_EarlyDispatchMessage ( const_cast <MSG *> (lpMsg),
                                     false )
       )
    {
      return DefWindowProcW (lpMsg->hwnd, lpMsg->message, lpMsg->wParam, lpMsg->lParam);
    }
  }

  return DispatchMessageW_Original (lpMsg);
}

LRESULT
WINAPI
DispatchMessageA_Detour (_In_ const MSG *lpMsg)
{
  if (hModSteamOverlay == nullptr)
  {
    hModSteamOverlay = GetModuleHandle
#ifdef _WIN64
    (L"GameOverlayRenderer64.dll");
#else
    (L"GameOverlayRenderer.dll");
#endif
  }

  if (SK_GetCallingDLL () == hModSteamOverlay)
  {
    return DispatchMessageA_Original ( lpMsg );
  }

  SK_LOG_FIRST_CALL

  if (lpMsg->hwnd == game_window.hWnd)
  {
    if ( SK_EarlyDispatchMessage ( const_cast <MSG *> (lpMsg),
                                     false )
       )
    {
      return DefWindowProcA (lpMsg->hwnd, lpMsg->message, lpMsg->wParam, lpMsg->lParam);
    }
  }

  return DispatchMessageA_Original (lpMsg);
}


#include <SpecialK/render_backend.h>
#include <SpecialK/dxgi_backend.h>
#include <SpecialK/dxgi_interfaces.h>


DWORD
WINAPI
SK_RealizeForegroundWindow (HWND hWndForeground)
{
  static volatile LONG nest_lvl = 0L;

  while (ReadAcquire (&nest_lvl))
    MsgWaitForMultipleObjectsEx (0, nullptr, 125, QS_ALLINPUT, MWMO_ALERTABLE);

  InterlockedIncrementAcquire (&nest_lvl);

  CreateThread (
    nullptr,
      0,
        [](LPVOID user)->

        DWORD
        {
          BringWindowToTop    (static_cast <HWND> (user));
          SetForegroundWindow (static_cast <HWND> (user));
          SetActiveWindow     (static_cast <HWND> (user));
          SetFocus            (static_cast <HWND> (user));

          CloseHandle (GetCurrentThread ());

          return 0;
        },

        static_cast <LPVOID> (hWndForeground),
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

HHOOK g_hkCallWndProc;

bool
window_message_dispatch_s::On_WINDOWPOSCHANGING (HWND hWnd, WPARAM&, LPARAM& lParam, LRESULT *pRet)
{
  if (hWnd != game_window.hWnd)
    return false;

  auto wnd_pos =
    reinterpret_cast <LPWINDOWPOS> (lParam);

  if ( (wnd_pos->flags ^ SWP_NOSIZE) ||
       (wnd_pos->flags ^ SWP_NOMOVE) )
  {
    ImGui_ImplDX11_InvalidateDeviceObjects ();

    //GetWindowRect_Original (game_window.hWnd, &game_window.actual.window);
    //GetClientRect_Original (game_window.hWnd, &game_window.actual.client);

    int width  = wnd_pos->cx;
    int height = wnd_pos->cy;

    if (wnd_pos->flags & SWP_NOMOVE)
    {
      //game_window.game.window.left   = wnd_pos->x;
      //game_window.game.window.top    = wnd_pos->y;

      game_window.game.window.right  = game_window.game.window.left   + width;
      game_window.game.window.bottom = game_window.game.window.bottom + height;
    }

    if (wnd_pos->flags & SWP_NOSIZE)
    {
      width  = game_window.game.window.right  - game_window.game.window.left;
      height = game_window.game.window.bottom - game_window.game.window.top;

      game_window.game.window.right  = game_window.game.window.left + width;
      game_window.game.window.bottom = game_window.game.window.top  + height;
    }
  }

  //if (config.window.borderless && (wnd_pos->flags & SWP_FRAMECHANGED))
  //  SK_AdjustBorder ();

  //game_window.game.client = game_window.game.window;

  if (((! (wnd_pos->flags & SWP_NOMOVE)) || (! (wnd_pos->flags & SWP_NOSIZE))))
  {
    // Filter this message
    if (config.window.borderless && config.window.fullscreen)
    {
      HMONITOR hMonitor =
      MonitorFromWindow ( hWnd,
                            MONITOR_DEFAULTTONEAREST );

      MONITORINFO mi = { 0 };
      mi.cbSize      = sizeof (mi);

      GetMonitorInfo (hMonitor, &mi);

      wnd_pos->cx = game_window.game.window.right  - game_window.game.window.left;
      wnd_pos->cy = game_window.game.window.bottom - game_window.game.window.top;

      game_window.actual.window = mi.rcMonitor;

      (game_window.actual.window.right  -= game_window.actual.window.left);
      (game_window.actual.window.bottom -= game_window.actual.window.top);

      game_window.actual.client.left   = 0;
      game_window.actual.client.right  = game_window.actual.window.right;
      game_window.actual.client.top    = 0;
      game_window.actual.client.bottom = game_window.actual.window.bottom;

      wnd_pos->x  = 0;
      wnd_pos->y  = 0;
      wnd_pos->cx = game_window.actual.window.right  - game_window.actual.window.left;
      wnd_pos->cy = game_window.actual.window.bottom - game_window.actual.window.top;

      wnd_pos->flags |= (SWP_NOMOVE | SWP_NOSIZE | SWP_NOSENDCHANGING);

      if (pRet) *pRet = 0;

      if (wnd_pos->flags & SWP_NOACTIVATE)
        return true;
    }
  }

  return false;
}

bool
window_message_dispatch_s::On_WINDOWPOSCHANGED (HWND hWnd, WPARAM&, LPARAM& lParam, LRESULT *pRet)
{
  if (game_window.hWnd != hWnd)
    return false;

  auto wnd_pos =
    reinterpret_cast <LPWINDOWPOS> (lParam);

  bool offset = false;

  // Test for user-defined position; if it exists, then we must
  //   respond to all WM_WINDOWPOSCHANGED messages indicating window movement
  if ( config.window.offset.x.absolute                 || config.window.offset.y.absolute ||
       ( config.window.offset.x.percent >  0.000001f ||
         config.window.offset.x.percent < -0.000001f ) ||
       ( config.window.offset.y.percent >  0.000001f ||
         config.window.offset.y.percent < -0.000001f )
     )
    offset = true;

  bool temp_override = false;

  // Prevent all of this craziness from resizing the window accidentally
  if (config.window.res.override.isZero ())
  {
         temp_override = true;
    RECT client        = {  };

    GetClientRect_Original (game_window.hWnd, &client);

    config.window.res.override.x = client.right  - client.left;
    config.window.res.override.y = client.bottom - client.top;
  }

  SK_RenderBackend& rb = SK_GetCurrentRenderBackend ();

  if (   (    config.window.center && (! ( (config.window.fullscreen &&
                                  config.window.borderless)  ||
                                  rb.fullscreen_exclusive  ) ) )
      || (    offset                                      &&  (!(wnd_pos->flags & SWP_NOMOVE)) )
      || (! ( config.window.res.override.isZero () ||
         (    temp_override                           ))  &&  (!(wnd_pos->flags & SWP_NOSIZE)) )
     )
  {
    wnd_pos->flags |= ( SWP_NOSENDCHANGING );
    SK_AdjustWindow ();
  }

  if (temp_override)
  {
    config.window.res.override.x = 0;
    config.window.res.override.y = 0;
  }

  wnd_pos->x  = game_window.actual.window.left;
  wnd_pos->y  = game_window.actual.window.top;

  wnd_pos->cx = game_window.actual.window.right  - game_window.actual.window.left;
  wnd_pos->cy = game_window.actual.window.bottom - game_window.actual.window.top;

  game_window.game.window = game_window.actual.window;

  if (config.window.confine_cursor && (! moving_windows.count (hWnd)))
    ClipCursor_Original (&game_window.actual.window);
  else if (config.window.unconfine_cursor)
    ClipCursor_Original (nullptr);

  // Filter this message
  if (config.window.borderless && config.window.fullscreen)
  {
    if (pRet) *pRet = 1;

    //return true;
  }

  return false;
}

LRESULT CALLBACK CallWndProc(
  _In_ int    nCode,
  _In_ WPARAM wParam_HOOK,
  _In_ LPARAM lParam_HOOK
)
{
  if (nCode < 0 || __SK_DLL_Ending)
    return CallNextHookEx (g_hkCallWndProc, nCode, wParam_HOOK, lParam_HOOK);

  {
    HWND&   hWnd   = ((CWPSTRUCT *)lParam_HOOK)->hwnd;
    UINT&   uMsg   = ((CWPSTRUCT *)lParam_HOOK)->message;
    WPARAM& wParam = ((CWPSTRUCT *)lParam_HOOK)->wParam;
    LPARAM& lParam = ((CWPSTRUCT *)lParam_HOOK)->lParam;

    static bool first = true;

    if (first && (hWnd == game_window.hWnd))
    {
      first = false;

      //if (game_window.hWnd != nullptr)
      //{
      //  dll_log.Log ( L"[Window Mgr] New HWND detected in the window proc. used"
      //                L" for rendering... (Old=%p, New=%p)",
      //                  game_window.hWnd, hWnd );
      //}

      game_window.hWnd = hWnd;

      game_window.active       = true;
      game_window.game.style   = game_window.GetWindowLongPtr (game_window.hWnd, GWL_STYLE);
      game_window.actual.style = game_window.GetWindowLongPtr (game_window.hWnd, GWL_STYLE);
      game_window.unicode      =              IsWindowUnicode (game_window.hWnd)   != FALSE;

      GetWindowRect_Original (game_window.hWnd, &game_window.game.window  );
      GetClientRect_Original (game_window.hWnd, &game_window.game.client  );
      GetWindowRect_Original (game_window.hWnd, &game_window.actual.window);
      GetClientRect_Original (game_window.hWnd, &game_window.actual.client);

      SK_InitWindow (hWnd, false);
    }

    if (hWnd == game_window.hWnd)
    {
#if 0
  if ((uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST) && game_window.needsCoordTransform ())
  {
    POINT pt;

    pt.x = GET_X_LPARAM (lParam);
    pt.y = GET_Y_LPARAM (lParam);

    SK_CalcCursorPos (&pt);

    lParam = MAKELPARAM ((SHORT)pt.x, (SHORT)pt.y);
  }
#endif




    if (config.input.cursor.manage)
    {
      //extern bool IsControllerPluggedIn (INT iJoyID);

     auto ActivateCursor = [](bool changed = false)->
      bool
       {
         bool was_active = last_mouse.cursor;

         if (! last_mouse.cursor)
         {
           if ((! SK_IsSteamOverlayActive ()) && game_window.active)
           {
             while (ShowCursor (TRUE) < 0) ;
             last_mouse.cursor = true;
           }
         }

         if (changed && (! SK_IsSteamOverlayActive ()))
           last_mouse.sampled = timeGetTime ();

         return (last_mouse.cursor != was_active);
       };

     auto DeactivateCursor = 
     []{
         if (! last_mouse.cursor)
           return false;

         bool was_active = last_mouse.cursor;

         if (last_mouse.sampled <= timeGetTime () - config.input.cursor.timeout)
         {
           if ((! SK_IsSteamOverlayActive ()) && game_window.active)
           {
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
        {
          SetTimer ( hWnd,
                       static_cast <UINT_PTR> (        last_mouse.timer_id),
                       static_cast <UINT>     (config.input.cursor.timeout) / 2,
                         nullptr );
        }
        else
        {
          SetTimer ( hWnd,
                       static_cast <UINT_PTR> (last_mouse.timer_id),
                         250UL/*USER_TIMER_MINIMUM*/,
                           nullptr );
        }

        last_mouse.init = true;
      }

      bool activation_event =
        (! SK_IsSteamOverlayActive ());

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




  switch (uMsg)
  {
    case WM_SYSCOMMAND:
      if ((wParam & 0xfff0) == SC_KEYMENU && lParam == 0) // Disable ALT application menu
        return CallNextHookEx (g_hkCallWndProc, nCode, wParam_HOOK, lParam_HOOK);
      break;

    case WM_MOUSEACTIVATE: 
    case WM_ACTIVATEAPP:
    case WM_ACTIVATE:
    case WM_NCACTIVATE:
    {
      LRESULT lRet = 0;
      if (wm_dispatch.ProcessMessage (hWnd, uMsg, wParam, lParam, &lRet))
      {
        if (lRet != 0) return CallNextHookEx (g_hkCallWndProc, nCode, wParam_HOOK, lParam_HOOK);
        else           return lRet;
      }
    } break;

    case WM_NCCALCSIZE:
      break;

    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
    {
      LRESULT lRet = 1;
      if (wm_dispatch.ProcessMessage (hWnd, uMsg, wParam, lParam, &lRet))
      {
        if (lRet != 0) CallNextHookEx (g_hkCallWndProc, nCode, wParam_HOOK, lParam_HOOK);
        else           return          lRet;
      }
    } break;

    case WM_ENTERSIZEMOVE:
    case WM_EXITSIZEMOVE:
    {
      wm_dispatch.ProcessMessage (hWnd, uMsg, wParam, lParam);
    } break;

    case WM_SIZING:
    case WM_MOVING:
      ClipCursor (nullptr);

      // Filter this message
      if (config.window.borderless && config.window.fullscreen)
        return CallNextHookEx (g_hkCallWndProc, nCode, wParam_HOOK, lParam_HOOK);
      break;


    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
      if (game_window.active)
      {
        if (SK_Console::getInstance ()->KeyDown (wParam & 0xFF, lParam) && (uMsg != WM_SYSKEYDOWN))
        {
          return CallNextHookEx (g_hkCallWndProc, nCode, wParam_HOOK, lParam_HOOK);
          //return SK_COMPAT_SafeCallProc (&game_window, hWnd, uMsg, wParam, lParam);
        }
      }
      else
        return CallNextHookEx (g_hkCallWndProc, nCode, wParam_HOOK, lParam_HOOK);
      break;

    case WM_KEYUP:
    case WM_SYSKEYUP:
      if (game_window.active)
      {
        if (SK_Console::getInstance ()->KeyUp (wParam & 0xFF, lParam) && (uMsg != WM_SYSKEYUP))
        {
          return CallNextHookEx (g_hkCallWndProc, nCode, wParam_HOOK, lParam_HOOK);
        }
      }
      else
        return CallNextHookEx (g_hkCallWndProc, nCode, wParam_HOOK, lParam_HOOK);
      break;

    case WM_MOUSEMOVE:
      wm_dispatch.ProcessMessage (hWnd, uMsg, wParam, lParam);
      break;

    case WM_INPUT:
      if (! game_window.active)
        return CallNextHookEx (g_hkCallWndProc, nCode, wParam_HOOK, lParam_HOOK);
      break;
  }


  bool handled = 
    ImGui_WndProcHandler (hWnd, uMsg, wParam, lParam);



  // Synaptics Touchpad Compat Hack:
  // -------------------------------
  //
  //  PROBLEM:    Driver only generates window messages for mousewheel, it does
  //                not activate RawInput, DirectInput or HID like a real mouse
  //
  //  WORKAROUND: Generate a full-blown input event using SendInput (...); be
  //                aware that this event will generate ANOTHER WM_MOUSEWHEEL.
  //
  //    ** MUST handle recursive behavior caused by this fix-up **
  //
  static bool recursive_wheel = false;

  // Dual purpose: This also catches any WM_MOUSEWHEEL messages that Synaptics
  //                 issued through CallWindowProc (...) rather than
  //                   SendMessage (...) / PostMessage (...) -- UGH.
  //
  //      >> We need to process those for ImGui <<
  //
  if ((! handled) && uMsg == WM_MOUSEWHEEL && (! recursive_wheel))
  {
    if ((! handled) && config.input.mouse.fix_synaptics)
    {
      INPUT input        = { };

      input.type         = INPUT_MOUSE;
      input.mi.dwFlags   = MOUSEEVENTF_WHEEL;
      input.mi.mouseData = GET_WHEEL_DELTA_WPARAM (wParam);

      recursive_wheel    = true;

      SendInput_Original (1, &input, sizeof INPUT);
    }
  }

  // In-lieu of a proper fence, this solves the recursion problem.
  //
  //   There's no guarantee the message we are ignoring is the one we
  //     generated, but one misplaced message won't kill anything.
  //
  else if (recursive_wheel && uMsg == WM_MOUSEWHEEL)
    recursive_wheel = false;



  //
  // Squelch input messages that managed to get into the loop without triggering
  //   filtering logic in the GetMessage (...), PeekMessage (...) and
  //     DispatchMessage (...) hooks.
  //
  //   [ Mostly for EverQuest ]
  //
  if (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST && SK_ImGui_WantMouseCapture    ())
    return CallNextHookEx (g_hkCallWndProc, nCode, wParam_HOOK, lParam_HOOK);
  
  if (uMsg >= WM_KEYFIRST   && uMsg <= WM_KEYLAST   && SK_ImGui_WantKeyboardCapture ())
    return CallNextHookEx (g_hkCallWndProc, nCode, wParam_HOOK, lParam_HOOK);
  
  if (uMsg == WM_INPUT      && SK_ImGui_WantGamepadCapture ())
    return CallNextHookEx (g_hkCallWndProc, nCode, wParam_HOOK, lParam_HOOK);



  //
  // DO NOT HOOK THIS FUNCTION outside of SpecialK plug-ins, the ABI is not guaranteed
  //
  if (SK_DetourWindowProc2 (hWnd, uMsg, wParam, lParam))
  {
    bool console_visible =
      SK_Console::getInstance ()->isVisible ();

    // Block keyboard input to the game while the console is visible
    if (console_visible)
    {
      if (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST)
        return CallNextHookEx (g_hkCallWndProc, nCode, wParam_HOOK, lParam_HOOK);

      if (uMsg >= WM_KEYFIRST   && uMsg <= WM_KEYLAST)
        return CallNextHookEx (g_hkCallWndProc, nCode, wParam_HOOK, lParam_HOOK);

      // Block RAW Input
      if (uMsg == WM_INPUT)
        return CallNextHookEx (g_hkCallWndProc, nCode, wParam_HOOK, lParam_HOOK);
    }
  }
  
  else {
    return 0;
  }


  // Filter this out for fullscreen override safety
  ////if (uMsg == WM_DISPLAYCHANGE)    return 1;//game_window.DefWindowProc (hWnd, uMsg, wParam, lParam);
  ////if (uMsg == WM_WINDOWPOSCHANGED) return game_window.DefWindowProc (hWnd, uMsg, wParam, lParam);


  LRESULT lRet =
    CallNextHookEx (g_hkCallWndProc, nCode, wParam_HOOK, lParam_HOOK);


  // Post-Process the game's result to fix any non-compliant behaviors
  //


  // Fix for Skyrim SE beeping when Alt is pressed.
  if (uMsg == WM_MENUCHAR && (! HIWORD (lRet)))
    return MAKEWPARAM (0, MNC_CLOSE);


  return lRet;
    }
  }

  return CallNextHookEx (g_hkCallWndProc, nCode, wParam_HOOK, lParam_HOOK);
}

#if 0
__declspec (noinline)
LRESULT
CALLBACK
SK_DetourWindowProc ( _In_  HWND   hWnd,
                      _In_  UINT   uMsg,
                      _In_  WPARAM wParam,
                      _In_  LPARAM lParam )
{
  if (ReadAcquire (&SK_bypass_dialog_active))
    return game_window.DefWindowProc (hWnd, uMsg, wParam, lParam);

  // If we are forcing a shutdown, then route any messages through the
  //   default Win32 handler.
  if (__SK_DLL_Ending)
    return game_window.DefWindowProc (hWnd, uMsg, wParam, lParam);

  static bool first_run = true;

  if (first_run)
  {
    // Start unmuted (in case the game crashed in the background)
    if (config.window.background_mute)
      SK_SetGameMute (FALSE);

    first_run = false;
  }

  if (hWnd != game_window.hWnd)
  {
    if (game_window.hWnd != nullptr)
    {
      dll_log.Log ( L"[Window Mgr] New HWND detected in the window proc. used"
                    L" for rendering... (Old=%p, New=%p)",
                      game_window.hWnd, hWnd );
    }

    game_window.hWnd = hWnd;

    game_window.active       = true;
    game_window.game.style   = game_window.GetWindowLongPtr (game_window.hWnd, GWL_STYLE);
    game_window.actual.style = game_window.GetWindowLongPtr (game_window.hWnd, GWL_STYLE);
    game_window.unicode      =              IsWindowUnicode (game_window.hWnd)   != FALSE;

    GetWindowRect_Original (game_window.hWnd, &game_window.game.window  );
    GetClientRect_Original (game_window.hWnd, &game_window.game.client  );
    GetWindowRect_Original (game_window.hWnd, &game_window.actual.window);
    GetClientRect_Original (game_window.hWnd, &game_window.actual.client);

    SK_InitWindow (hWnd, false);
  }



    if (config.input.cursor.manage)
    {
      //extern bool IsControllerPluggedIn (INT iJoyID);

     auto ActivateCursor = [](bool changed = false)->
      bool
       {
         bool was_active = last_mouse.cursor;

         if (! last_mouse.cursor)
         {
           if ((! SK_IsSteamOverlayActive ()) && game_window.active)
           {
             while (ShowCursor (TRUE) < 0) ;
             last_mouse.cursor = true;
           }
         }

         if (changed && (! SK_IsSteamOverlayActive ()))
           last_mouse.sampled = timeGetTime ();

         return (last_mouse.cursor != was_active);
       };

     auto DeactivateCursor = 
     []{
         if (! last_mouse.cursor)
           return false;

         bool was_active = last_mouse.cursor;

         if (last_mouse.sampled <= timeGetTime () - config.input.cursor.timeout)
         {
           if ((! SK_IsSteamOverlayActive ()) && game_window.active)
           {
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
        {
          SetTimer ( hWnd,
                       static_cast <UINT_PTR> (        last_mouse.timer_id),
                       static_cast <UINT>     (config.input.cursor.timeout) / 2,
                         nullptr );
        }
        else
        {
          SetTimer ( hWnd,
                       static_cast <UINT_PTR> (last_mouse.timer_id),
                         250UL/*USER_TIMER_MINIMUM*/,
                           nullptr );
        }

        last_mouse.init = true;
      }

      bool activation_event =
        (! SK_IsSteamOverlayActive ());

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




  switch (uMsg)
  {
    case WM_SYSCOMMAND:
      if ((wParam & 0xfff0) == SC_KEYMENU && lParam == 0) // Disable ALT application menu
        return 0;
      break;

    // Ignore (and physically remove) this event from the message queue if background_render = true
    case WM_MOUSEACTIVATE: 
    case WM_ACTIVATEAPP:
    case WM_ACTIVATE:
    case WM_NCACTIVATE:
    {
      LRESULT lRet = 0;
      if (wm_dispatch.ProcessMessage (hWnd, uMsg, wParam, lParam, &lRet))
        return lRet;
    } break;

    case WM_NCCALCSIZE:
      break;

    case WM_WINDOWPOSCHANGING:
    case WM_WINDOWPOSCHANGED:
    {
      LRESULT lRet = 1;
      if (wm_dispatch.ProcessMessage (hWnd, uMsg, wParam, lParam, &lRet)) return lRet;
    } break;

    case WM_ENTERSIZEMOVE:
    case WM_EXITSIZEMOVE:
    {
      wm_dispatch.ProcessMessage (hWnd, uMsg, wParam, lParam);
    } break;

    //case WM_CAPTURECHANGED:
    //{
    //  if (reinterpret_cast <HWND> (lParam) == game_window.hWnd)
    //  {
    //    wm_dispatch.ProcessMessage (hWnd, WM_ENTERSIZEMOVE, wParam, lParam);
    //  }
    //  else
    //    wm_dispatch.ProcessMessage (hWnd, WM_EXITSIZEMOVE, wParam, lParam);
    //} break;

    case WM_SIZING:
    case WM_MOVING:
      ClipCursor (nullptr);

      // Filter this message
      if (config.window.borderless && config.window.fullscreen)
        return 0;
      break;


    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
      if (game_window.active)
      {
        if (SK_Console::getInstance ()->KeyDown (wParam & 0xFF, lParam) && (uMsg != WM_SYSKEYDOWN))
        {
          return SK_COMPAT_SafeCallProc (&game_window, hWnd, uMsg, wParam, lParam);
        }
      }
      else
        return DefWindowProcW (hWnd, uMsg, wParam, lParam);
      break;

    case WM_KEYUP:
    case WM_SYSKEYUP:
      if (game_window.active)
      {
        if (SK_Console::getInstance ()->KeyUp (wParam & 0xFF, lParam) && (uMsg != WM_SYSKEYUP))
        {
          return SK_COMPAT_SafeCallProc (&game_window, hWnd, uMsg, wParam, lParam);
        }
      }
      else
        return DefWindowProcW (hWnd, uMsg, wParam, lParam);
      break;

    case WM_MOUSEMOVE:
      wm_dispatch.ProcessMessage (hWnd, uMsg, wParam, lParam);
      break;

    case WM_INPUT:
      if (! game_window.active)
        return DefWindowProcW (hWnd, uMsg, wParam, lParam);
      break;
  }


  bool handled =
    ImGui_WndProcHandler (hWnd, uMsg, wParam, lParam);


  // Synaptics Touchpad Compat Hack:
  // -------------------------------
  //
  //  PROBLEM:    Driver only generates window messages for mousewheel, it does
  //                not activate RawInput, DirectInput or HID like a real mouse
  //
  //  WORKAROUND: Generate a full-blown input event using SendInput (...); be
  //                aware that this event will generate ANOTHER WM_MOUSEWHEEL.
  //
  //    ** MUST handle recursive behavior caused by this fix-up **
  //
  static bool recursive_wheel = false;

  // Dual purpose: This also catches any WM_MOUSEWHEEL messages that Synaptics
  //                 issued through CallWindowProc (...) rather than
  //                   SendMessage (...) / PostMessage (...) -- UGH.
  //
  //      >> We need to process those for ImGui <<
  //
  if ((! handled) && uMsg == WM_MOUSEWHEEL && (! recursive_wheel))
  {
    //if (! game_window.hooked)
    //  handled = ImGui_WndProcHandler (hWnd, uMsg, wParam, lParam);

    if ((! handled) && config.input.mouse.fix_synaptics)
    {
      INPUT input        = { };

      input.type         = INPUT_MOUSE;
      input.mi.dwFlags   = MOUSEEVENTF_WHEEL;
      input.mi.mouseData = GET_WHEEL_DELTA_WPARAM (wParam);

      recursive_wheel    = true;

      SendInput_Original (1, &input, sizeof INPUT);
    }
  }

  // In-lieu of a proper fence, this solves the recursion problem.
  //
  //   There's no guarantee the message we are ignoring is the one we
  //     generated, but one misplaced message won't kill anything.
  //
  else if (recursive_wheel && uMsg == WM_MOUSEWHEEL)
    recursive_wheel = false;



  //
  // Squelch input messages that managed to get into the loop without triggering
  //   filtering logic in the GetMessage (...), PeekMessage (...) and
  //     DispatchMessage (...) hooks.
  //
  //   [ Mostly for EverQuest ]
  //
  if (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST && SK_ImGui_WantMouseCapture    ())
    return game_window.DefWindowProc (hWnd, uMsg, wParam, lParam);

  if (uMsg >= WM_KEYFIRST   && uMsg <= WM_KEYLAST   && SK_ImGui_WantKeyboardCapture ())
    return game_window.DefWindowProc (hWnd, uMsg, wParam, lParam);

  if (uMsg == WM_INPUT      && SK_ImGui_WantGamepadCapture ())
    return game_window.DefWindowProc (hWnd, uMsg, wParam, lParam);



  //
  // DO NOT HOOK THIS FUNCTION outside of SpecialK plug-ins, the ABI is not guaranteed
  //
  if (SK_DetourWindowProc2 (hWnd, uMsg, wParam, lParam))
  {
    bool console_visible =
      SK_Console::getInstance ()->isVisible ();

    // Block keyboard input to the game while the console is visible
    if (console_visible)
    {
      if (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST)
        return game_window.DefWindowProc (hWnd, uMsg, wParam, lParam);

      if (uMsg >= WM_KEYFIRST   && uMsg <= WM_KEYLAST)
        return game_window.DefWindowProc (hWnd, uMsg, wParam, lParam);

      // Block RAW Input
      if (uMsg == WM_INPUT)
        return game_window.DefWindowProc (hWnd, uMsg, wParam, lParam);
    }
  }
  
  else {
    return 0;
  }


  // Filter this out for fullscreen override safety
  ////if (uMsg == WM_DISPLAYCHANGE)    return 1;//game_window.DefWindowProc (hWnd, uMsg, wParam, lParam);
  ////if (uMsg == WM_WINDOWPOSCHANGED) return game_window.DefWindowProc (hWnd, uMsg, wParam, lParam);


  LRESULT lRet =
    SK_COMPAT_SafeCallProc (&game_window, hWnd, uMsg, wParam, lParam);


  // Post-Process the game's result to fix any non-compliant behaviors
  //


  // Fix for Skyrim SE beeping when Alt is pressed.
  if (uMsg == WM_MENUCHAR && (! HIWORD (lRet)))
    return MAKEWPARAM (0, MNC_CLOSE);


  return lRet;
}
#else
__declspec (noinline)
LRESULT
CALLBACK
SK_DetourWindowProc ( _In_  HWND   hWnd,
                      _In_  UINT   uMsg,
                      _In_  WPARAM wParam,
                      _In_  LPARAM lParam )
{
  // If we are forcing a shutdown, then route any messages through the
  //   default Win32 handler.
  if (ReadAcquire (&__SK_DLL_Ending))
    return game_window.DefWindowProc (hWnd, uMsg, wParam, lParam);


  static bool first_run = true;

  if (first_run)
  {
    // Start unmuted (in case the game crashed in the background)
    if (config.window.background_mute)
      SK_SetGameMute (FALSE);

    first_run = false;
  }



  if (hWnd != game_window.hWnd)
  {
    if (game_window.hWnd != nullptr)
    {
      dll_log.Log ( L"[Window Mgr] New HWND detected in the window proc. used"
                    L" for rendering... (Old=%p, New=%p)",
                      game_window.hWnd, hWnd );
    }

    game_window.hWnd = hWnd;

    game_window.active       = true;
    game_window.game.style   = game_window.GetWindowLongPtr (game_window.hWnd, GWL_STYLE);
    game_window.actual.style = game_window.GetWindowLongPtr (game_window.hWnd, GWL_STYLE);
    game_window.unicode      =              IsWindowUnicode (game_window.hWnd)   != FALSE;

    GetWindowRect_Original (game_window.hWnd, &game_window.game.window  );
    GetClientRect_Original (game_window.hWnd, &game_window.game.client  );
    GetWindowRect_Original (game_window.hWnd, &game_window.actual.window);
    GetClientRect_Original (game_window.hWnd, &game_window.actual.client);

    SK_InitWindow (hWnd, false);
  }



  static bool last_active = game_window.active;

  bool console_visible =
    SK_Console::getInstance ()->isVisible ();


#if 0
  if ((uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST) && game_window.needsCoordTransform ())
  {
    POINT pt;

    pt.x = GET_X_LPARAM (lParam);
    pt.y = GET_Y_LPARAM (lParam);

    SK_CalcCursorPos (&pt);

    lParam = MAKELPARAM ((SHORT)pt.x, (SHORT)pt.y);
  }
#endif


  if (config.input.cursor.manage)
  {
    //extern bool IsControllerPluggedIn (INT iJoyID);

   auto ActivateCursor = [](bool changed = false)->
    bool
     {
       bool was_active = last_mouse.cursor;

       if (! last_mouse.cursor)
       {
         if ((! SK_IsSteamOverlayActive ()) && game_window.active)
         {
           while (ShowCursor (TRUE) < 0) ;
           last_mouse.cursor = true;
         }
       }

       if (changed && (! SK_IsSteamOverlayActive ()))
         last_mouse.sampled = timeGetTime ();

       return (last_mouse.cursor != was_active);
     };

   auto DeactivateCursor = 
   []{
       if (! last_mouse.cursor)
         return false;

       bool was_active = last_mouse.cursor;

       if (last_mouse.sampled <= timeGetTime () - config.input.cursor.timeout)
       {
         if ((! SK_IsSteamOverlayActive ()) && game_window.active)
         {
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
      {
        SetTimer ( hWnd,
                     static_cast <UINT_PTR> (        last_mouse.timer_id),
                     static_cast <UINT>     (config.input.cursor.timeout) / 2,
                       nullptr );
      }
      else
      {
        SetTimer ( hWnd,
                     static_cast <UINT_PTR> (last_mouse.timer_id),
                       250UL/*USER_TIMER_MINIMUM*/,
                         nullptr );
      }

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


  auto ActivateWindow =[&](bool active = false)
  {
    bool state_changed =
      (game_window.active != active);

    game_window.active = active;

    if (state_changed)
    {
      SK_Console::getInstance ()->reset ();

      if (config.window.background_mute)
        SK_WindowManager::getInstance ()->muteGame ((! active));

      // Keep Unity games from crashing at startup when forced into FULLSCREEN
      //
      //  ... also prevents a game from staying topmost when you Alt+Tab
      //

      if ( active && config.display.force_fullscreen &&
           ( static_cast <int> (SK_GetCurrentRenderBackend ().api)  &
             static_cast <int> (SK_RenderAPI::D3D9               )
           )
         )
      {
        SetWindowLongPtrW    (game_window.hWnd, GWL_EXSTYLE,
         ( GetWindowLongPtrW (game_window.hWnd, GWL_EXSTYLE) & ~(WS_EX_TOPMOST | WS_EX_NOACTIVATE)
         ) | WS_EX_APPWINDOW );
        //SetWindowPos      (game_window.hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSENDCHANGING | SWP_NOMOVE     | SWP_NOSIZE     |
        //                                                                 SWP_FRAMECHANGED   | SWP_DEFERERASE | SWP_NOCOPYBITS |
        //                                                                 SWP_ASYNCWINDOWPOS | SWP_SHOWWINDOW | SWP_NOACTIVATE );
        //SetWindowPos      (game_window.hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOSENDCHANGING | SWP_NOMOVE     | SWP_NOSIZE     |
        //                                                           SWP_FRAMECHANGED   | SWP_DEFERERASE | SWP_NOCOPYBITS |
        //                                                           SWP_ASYNCWINDOWPOS | SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOZORDER );

        extern void
        SK_D3D9_TriggerReset (bool);
      
        SK_D3D9_TriggerReset (false);
      }
    }


    if (active && state_changed)
    {
      if ((! SK_GetCurrentRenderBackend ().fullscreen_exclusive) && config.window.background_render)
      {
        if (! game_window.cursor_visible)
        {
          while (ShowCursor (FALSE) >= 0)
            ;
        }

        ClipCursor_Original (&game_window.cursor_clip);
      }
    }

    else if ((! active) && state_changed)
    {
      if ((! SK_GetCurrentRenderBackend ().fullscreen_exclusive) && config.window.background_render)
      {
        game_window.cursor_visible =
          ShowCursor (TRUE) >= 1;

        while (ShowCursor (TRUE) < 0)
          ;

        ClipCursor_Original (nullptr);
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

    if (state_changed)
      SK_ImGui_Cursor.activateWindow (active);
  };


  switch (uMsg)
  {
    case WM_SYSCOMMAND:
      if ((wParam & 0xfff0) == SC_KEYMENU && lParam == 0) // Disable ALT application menu
        return 0;
      break;

    // Ignore (and physically remove) this event from the message queue if background_render = true
    case WM_MOUSEACTIVATE:
    {
      if ( reinterpret_cast <HWND> (wParam) == game_window.hWnd )
      {
        ActivateWindow (true);

        if ((! SK_GetCurrentRenderBackend ().fullscreen_exclusive) && config.window.background_render)
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
        if ((! SK_GetCurrentRenderBackend ().fullscreen_exclusive) && config.window.background_render)
        {
          SK_LOG2 ( ( L"WM_MOUSEACTIVATE (Other Window) ==> Activate" ),
                      L"Window Mgr" );
          return MA_ACTIVATE;
        }

        SK_GetCurrentRenderBackend ().fullscreen_exclusive = false;
      }
    } break;

    // Allow the game to run in the background
    case WM_ACTIVATEAPP:
    case WM_ACTIVATE:
    case WM_NCACTIVATE:
    {
      if (uMsg == WM_NCACTIVATE || uMsg == WM_ACTIVATEAPP)
      {
        if (wParam != FALSE)
        {
          if (last_active == false)
            SK_LOG3 ( ( L"Application Activated (Non-Client)" ),
                        L"Window Mgr" );

          ActivateWindow (true);
        }

        else
        {
          if (last_active == true)
            SK_LOG3 ( ( L"Application Deactivated (Non-Client)" ),
                        L"Window Mgr" );

          ActivateWindow (false);

          // We must fully consume one of these messages or audio will stop playing
          //   when the game loses focus, so do not simply pass this through to the
          //     default window procedure.
          if ( (! SK_GetCurrentRenderBackend ().fullscreen_exclusive) &&
                  config.window.background_render
             )
          {
            game_window.CallProc (
              hWnd,
                uMsg,
                  TRUE,
                    reinterpret_cast <LPARAM> (hWnd) );

            return 0;
          }

          SK_GetCurrentRenderBackend ().fullscreen_exclusive = false;
        }
      }

      else if (uMsg == WM_ACTIVATE)
      {
        const wchar_t* source   = L"UNKKNOWN";
        bool           activate = false;

        switch (LOWORD (wParam))
        {
          case WA_ACTIVE:
          case WA_CLICKACTIVE:
          default: // Unknown
          {
            activate = reinterpret_cast <HWND> (lParam) != game_window.hWnd;
            source   = LOWORD (wParam) == 1 ? L"(WM_ACTIVATE [ WA_ACTIVE ])" :
                                              L"(WM_ACTIVATE [ WA_CLICKACTIVE ])";
            ActivateWindow (activate);
          } break;

          case WA_INACTIVE:
          {
            activate = ( lParam                           == 0                ) ||
                       ( reinterpret_cast <HWND> (lParam) == game_window.hWnd );
            source   = L"(WM_ACTIVATE [ WA_INACTIVE ])";
            ActivateWindow (activate);
          } break;
        }

        switch (last_active)
        {
          case true:
            if (! activate)
            {
              SK_LOG2 ( ( L"Application Deactivated %s", source ),
                          L"Window Mgr" );
            }
            break;

          case false:
            if (activate)
            {
              SK_LOG2 ( ( L"Application Activated %s", source ),
                          L"Window Mgr" );
            }
            break;
        }

        if ((! SK_GetCurrentRenderBackend ().fullscreen_exclusive) && config.window.background_render)
        {
          return 1;
        }

        if (! activate)
          SK_GetCurrentRenderBackend ().fullscreen_exclusive = false;
      }
    } break;

    case WM_NCCALCSIZE:
      break;

    case WM_WINDOWPOSCHANGING:
    {
      auto wnd_pos =
        reinterpret_cast <LPWINDOWPOS> (lParam);

      if (wnd_pos->flags ^ SWP_NOMOVE)
      {
        int width  = game_window.game.window.right  - game_window.game.window.left;
        int height = game_window.game.window.bottom - game_window.game.window.top;

        game_window.game.window.left   = wnd_pos->x;
        game_window.game.window.top    = wnd_pos->y;

        game_window.game.window.right  = wnd_pos->x + width;
        game_window.game.window.bottom = wnd_pos->y + height;
      }

      if (wnd_pos->flags ^ SWP_NOSIZE)
      {
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

      auto wnd_pos =
        reinterpret_cast <LPWINDOWPOS> (lParam);

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
             ( config.window.offset.x.percent >  0.000001f ||
               config.window.offset.x.percent < -0.000001f ) ||
             ( config.window.offset.y.percent >  0.000001f ||
               config.window.offset.y.percent < -0.000001f )
           )
          offset = true;

        bool temp_override = false;

        // Prevent all of this craziness from resizing the window accidentally
        if (config.window.res.override.isZero ())
        {
               temp_override = true;
          RECT client        = {  };

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
        return 1;
    } break;


    case WM_SIZE:
      ImGui_ImplDX11_InvalidateDeviceObjects ();
      // Fallthrough to WM_MOVE

    case WM_MOVE:
      GetWindowRect_Original (game_window.hWnd, &game_window.actual.window);
      GetClientRect_Original (game_window.hWnd, &game_window.actual.client);

      if (config.window.confine_cursor)
        ClipCursor_Original (&game_window.actual.window);
      else if (config.window.unconfine_cursor)
        ClipCursor_Original (nullptr);


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
        if (SK_Console::getInstance ()->KeyDown (wParam & 0xFF, lParam) && (uMsg != WM_SYSKEYDOWN))
        {
          return SK_COMPAT_SafeCallProc (&game_window, hWnd, uMsg, wParam, lParam);
        }
      }
      else
        return DefWindowProcW (hWnd, uMsg, wParam, lParam);
      break;

    case WM_KEYUP:
    case WM_SYSKEYUP:
      if (game_window.active)
      {
        if (SK_Console::getInstance ()->KeyUp (wParam & 0xFF, lParam) && (uMsg != WM_SYSKEYUP))
        {
          return SK_COMPAT_SafeCallProc (&game_window, hWnd, uMsg, wParam, lParam);
        }
      }
      else
        return DefWindowProcW (hWnd, uMsg, wParam, lParam);
      break;

    case WM_MOUSEMOVE:
      if (! game_window.active)
        GetCursorPos_Original (&game_window.cursor_pos);
      break;

    case WM_INPUT:
      if (! game_window.active)
        return DefWindowProcW (hWnd, uMsg, wParam, lParam);
      break;
  }

         bool handled = false;
  static bool eqgame  =
    wcsstr (SK_GetHostApp (), L"eqgame.exe");

  if (eqgame)
  {
    handled =
      ImGui_WndProcHandler (hWnd, uMsg, wParam, lParam);
  }



  // Synaptics Touchpad Compat Hack:
  // -------------------------------
  //
  //  PROBLEM:    Driver only generates window messages for mousewheel, it does
  //                not activate RawInput, DirectInput or HID like a real mouse
  //
  //  WORKAROUND: Generate a full-blown input event using SendInput (...); be
  //                aware that this event will generate ANOTHER WM_MOUSEWHEEL.
  //
  //    ** MUST handle recursive behavior caused by this fix-up **
  //
  static bool recursive_wheel = false;

  // Dual purpose: This also catches any WM_MOUSEWHEEL messages that Synaptics
  //                 issued through CallWindowProc (...) rather than
  //                   SendMessage (...) / PostMessage (...) -- UGH.
  //
  //      >> We need to process those for ImGui <<
  //
  if ((! handled) && uMsg == WM_MOUSEWHEEL && (! recursive_wheel))
  {
    if (! eqgame)
      handled = ImGui_WndProcHandler (hWnd, uMsg, wParam, lParam);

    if ((! handled) && config.input.mouse.fix_synaptics)
    {
      INPUT input        = { };

      input.type         = INPUT_MOUSE;
      input.mi.dwFlags   = MOUSEEVENTF_WHEEL;
      input.mi.mouseData = GET_WHEEL_DELTA_WPARAM (wParam);

      recursive_wheel    = true;

      SendInput_Original (1, &input, sizeof INPUT);
    }
  }

  // In-lieu of a proper fence, this solves the recursion problem.
  //
  //   There's no guarantee the message we are ignoring is the one we
  //     generated, but one misplaced message won't kill anything.
  //
  else if (recursive_wheel && uMsg == WM_MOUSEWHEEL)
    recursive_wheel = false;



  //
  // Squelch input messages that managed to get into the loop without triggering
  //   filtering logic in the GetMessage (...), PeekMessage (...) and
  //     DispatchMessage (...) hooks.
  //
  //   [ Mostly for EverQuest ]
  //
  if (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST && SK_ImGui_WantMouseCapture    ())
    return game_window.DefWindowProc (hWnd, uMsg, wParam, lParam);

  if (uMsg >= WM_KEYFIRST   && uMsg <= WM_KEYLAST   && SK_ImGui_WantKeyboardCapture ())
    return game_window.DefWindowProc (hWnd, uMsg, wParam, lParam);

  if (uMsg == WM_INPUT      && SK_ImGui_WantGamepadCapture ())
    return game_window.DefWindowProc (hWnd, uMsg, wParam, lParam);



  //
  // DO NOT HOOK THIS FUNCTION outside of SpecialK plug-ins, the ABI is not guaranteed
  //
  if (SK_DetourWindowProc2 (hWnd, uMsg, wParam, lParam))
  {
    // Block keyboard input to the game while the console is visible
    if (console_visible)
    {
      if (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST)
        return game_window.DefWindowProc (hWnd, uMsg, wParam, lParam);

      if (uMsg >= WM_KEYFIRST   && uMsg <= WM_KEYLAST)
        return game_window.DefWindowProc (hWnd, uMsg, wParam, lParam);

      // Block RAW Input
      if (uMsg == WM_INPUT)
        return game_window.DefWindowProc (hWnd, uMsg, wParam, lParam);
    }
  }
  
  else {
    return 0;
  }


  // Filter this out for fullscreen override safety
  ////if (uMsg == WM_DISPLAYCHANGE)    return 1;//game_window.DefWindowProc (hWnd, uMsg, wParam, lParam);
  ////if (uMsg == WM_WINDOWPOSCHANGED) return game_window.DefWindowProc (hWnd, uMsg, wParam, lParam);


  LRESULT lRet =
    SK_COMPAT_SafeCallProc (&game_window, hWnd, uMsg, wParam, lParam);


  // Post-Process the game's result to fix any non-compliant behaviors
  //


  // Fix for Skyrim SE beeping when Alt is pressed.
  if (uMsg == WM_MENUCHAR && (! HIWORD (lRet)))
    return MAKEWPARAM (0, MNC_CLOSE);


  return lRet;
}
#endif

#include <SpecialK/core.h>

void
SK_InitWindow (HWND hWnd, bool fullscreen_exclusive)
{
  if (game_window.GetWindowLongPtr == nullptr)
  {
    SK_InstallWindowHook (hWnd);
  }

  GetWindowRect_Original (hWnd, &game_window.game.window);
  GetClientRect_Original (hWnd, &game_window.game.client);

  GetWindowRect_Original (hWnd, &game_window.actual.window);
  GetClientRect_Original (hWnd, &game_window.actual.client);

  GetCursorPos_Original  (      &game_window.cursor_pos);

  if (game_window.GetWindowLongPtr == nullptr)
    return;

  game_window.actual.style =
    game_window.GetWindowLongPtr ( hWnd, GWL_STYLE );

  game_window.actual.style_ex =
    game_window.GetWindowLongPtr ( hWnd, GWL_EXSTYLE );

  bool has_border  = SK_WindowManager::StyleHasBorder (
                       game_window.actual.style
                     );

  if (has_border)
  {
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

  if (game_window.unicode && (! caught_register)) 
  {
#ifdef _WIN64
    game_window.GetWindowLongPtr = GetWindowLongPtrW_Original;
  //game_window.GetClassLongPtr  = GetClassLongPtrW_Original;
#else
    game_window.GetWindowLongPtr =
      static_cast <GetWindowLongPtr_pfn> (GetWindowLongW_Original);
  //game_window.GetClassLongPtr =
  //  static_cast <GetClassLongPtr_pfn>  (GetClassLongW_Original);
#endif
    game_window.SetWindowLongPtr = SetWindowLongPtrW_Original;
  //game_window.SetClassLongPtr  = SetClassLongPtrW_Original;
    game_window.DefWindowProc    = (DefWindowProc_pfn)
      GetProcAddress ( GetModuleHandle (L"user32.dll"),
                         "DefWindowProcW" );
    game_window.CallWindowProc   = (CallWindowProc_pfn)
      GetProcAddress ( GetModuleHandle (L"user32.dll"),
                         "CallWindowProcW" );
  }

  else if (! caught_register)
  {
#ifdef _WIN64
    game_window.GetWindowLongPtr = GetWindowLongPtrA_Original;
  //game_window.GetClassLongPtr  = GetClassLongPtrA_Original;
#else
    game_window.GetWindowLongPtr =
      static_cast <GetWindowLongPtr_pfn> (GetWindowLongA_Original);
  //game_window.GetClassLongPtr =
  //  static_cast <GetClassLongPtr_pfn>  (GetClassLongA_Original);
#endif
    game_window.SetWindowLongPtr = SetWindowLongPtrA_Original;
  //game_window.SetClassLongPtr  = SetClassLongPtrA_Original;
    game_window.DefWindowProc    = reinterpret_cast <DefWindowProc_pfn>
      ( GetProcAddress ( GetModuleHandle (L"user32.dll"),
                          "DefWindowProcA" )
      );
    game_window.CallWindowProc   = reinterpret_cast <CallWindowProc_pfn>
      ( GetProcAddress ( GetModuleHandle (L"user32.dll"),
                           "CallWindowProcA" )
      );
  }

  WNDPROC class_proc = game_window.unicode ? (WNDPROC)
    GetClassLongPtrW  ( hWnd, GCLP_WNDPROC ) :
                                             (WNDPROC)
    GetClassLongPtrA  ( hWnd, GCLP_WNDPROC );

  WNDPROC wnd_proc = game_window.unicode    ? (WNDPROC)
    GetWindowLongPtrW  ( hWnd, GWLP_WNDPROC ) :
                                              (WNDPROC)
    GetWindowLongPtrA  ( hWnd, GWLP_WNDPROC );


  // When ImGui is active, we will do character translation internally
  //   for any Raw Input or Win32 keydown event -- disable Windows'
  //     translation from WM_KEYDOWN to WM_CHAR while we are doing this.
  //
  SK_CreateDLLHook2 (       L"user32.dll",
                             "TranslateMessage",
                              TranslateMessage_Detour,
     static_cast_p2p <void> (&TranslateMessage_Original) );


  SK_CreateDLLHook2 (       L"user32.dll",
                             "PeekMessageW",
                              PeekMessageW_Detour,
     static_cast_p2p <void> (&PeekMessageW_Original) );

  SK_CreateDLLHook2 (       L"user32.dll",
                             "PeekMessageA",
                              PeekMessageA_Detour,
     static_cast_p2p <void> (&PeekMessageA_Original) );


  // Hook as few of these as possible, disrupting the message pump
  //   when we already have the game's main window hooked is redundant.
  //
  //   ** PeekMessage is hooked because The Witness pulls mouse click events
  //        out of the pump without passing them through its window procedure.
  //
  SK_CreateDLLHook2 (       L"user32.dll",
                             "GetMessageW",
                              GetMessageW_Detour,
     static_cast_p2p <void> (&GetMessageW_Original) );

  SK_CreateDLLHook2 (       L"user32.dll",
                             "DispatchMessageW",
                              DispatchMessageW_Detour,
     static_cast_p2p <void> (&DispatchMessageW_Original) );
  
  SK_CreateDLLHook2 (       L"user32.dll",
                             "GetMessageA",
                              GetMessageA_Detour,
     static_cast_p2p <void> (&GetMessageA_Original) );

  SK_CreateDLLHook2 (       L"user32.dll",
                             "DispatchMessageA",
                              DispatchMessageA_Detour,
     static_cast_p2p <void> (&DispatchMessageA_Original) );

  if (! caught_register)
    game_window.WndProc_Original = nullptr;

  wchar_t wszClassName [256] = { };
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
  //   Initial PeekMessage hook will handle input at server select,
  //     hooked window proc will handle events in-game.
  //
  //   This game hooks its own message loop dispatch functions, probably
  //     as an anti-cheat method. Hooking the class procedure is the only
  //       workaround.
  //
  //if (! _wcsicmp (SK_GetHostApp (), L"eqgame.exe"))
    hook_classfunc = true;
  
  if (hook_classfunc && (! caught_register))
  {
    game_window.hooked = false;

#if 0
    game_window.WndProc_Original = (WNDPROC)GetWindowLongPtrW (game_window.hWnd, GWLP_WNDPROC);
    g_hkCallWndProc = SetWindowsHookEx (WH_CALLWNDPROC, CallWndProc, SK_GetDLL (), GetCurrentThreadId ());
#else
    if ( MH_OK ==
           MH_CreateHook (
                                                   class_proc,
                                          SK_DetourWindowProc,
                 static_cast_p2p <void> (&game_window.WndProc_Original)
           )
      )
    {
      MH_QueueEnableHook (class_proc);
    
      dll_log.Log (L"[Window Mgr]  >> Hooked ClassProc.");
    
      game_window.hooked = false;
    }
    
    else if ( MH_OK ==
                MH_CreateHook (
                                                      wnd_proc,
                                           SK_DetourWindowProc,
                  static_cast_p2p <void> (&game_window.WndProc_Original)
                )
            )
    {
      MH_QueueEnableHook (wnd_proc);
    
      dll_log.Log (L"[Window Mgr]  >> Hooked WndProc.");
    
      game_window.hooked = false;
    }
#endif
  }
  
  else if (! caught_register)
  {
    //dll_log.Log ( L"[Window Mgr]  >> Hooking was impossible; installing new "
    //              L"window procedure instead (this may be undone "
    //              L"by other software)." );
  
    game_window.WndProc_Original =
      static_cast <WNDPROC> (wnd_proc);
  
    game_window.SetWindowLongPtr ( hWnd,
                                     GWLP_WNDPROC,
          reinterpret_cast <LONG_PTR> (SK_DetourWindowProc) );
  
    //game_window.SetClassLongPtr ( hWnd,
    //                                GWLP_WNDPROC,
    //      reinterpret_cast <LONG_PTR> (SK_DetourWindowProc) );
  
    if (game_window.unicode)
      SetClassLongPtrW ( hWnd, GCLP_WNDPROC, (LONG_PTR)SK_DetourWindowProc );
    else
      SetClassLongPtrA ( hWnd, GCLP_WNDPROC, (LONG_PTR)SK_DetourWindowProc );
  
    game_window.hooked = true;
  }



  bool has_raw_mouse = false;
  UINT count         = 0;

                 GetRegisteredRawInputDevices_Original (nullptr, &count, sizeof RAWINPUTDEVICE);
  DWORD dwLast = GetLastError                          ();

  if (count > 0)
  {
    auto* pDevs =
      new RAWINPUTDEVICE [count + 1];

    GetRegisteredRawInputDevices_Original ( pDevs,
                                              &count,
                                                sizeof RAWINPUTDEVICE );

    for (UINT i = 0 ; i < count ; i++)
    {
      if ( ( pDevs [i].usUsage     == HID_USAGE_GENERIC_MOUSE ||
             pDevs [i].usUsage     == HID_USAGE_GENERIC_POINTER ) &&
           ( pDevs [i].usUsagePage == HID_USAGE_PAGE_GENERIC  ||
             pDevs [i].usUsagePage == HID_USAGE_PAGE_GAME       ) )
      {
        has_raw_mouse = true; // Technically this could be just about any pointing device
                              //   including a touchscreen (on purpose).
        break;
      }
    }

    delete [] pDevs;
  }

  SetLastError (dwLast);

  // If no pointing device is setup, go ahead and register our own mouse,
  //   it won't interfere with the game's operation until the ImGui overlay
  //     requires a mouse... at which point third-party software will flip out :)
  if (! has_raw_mouse)
  {
    RAWINPUTDEVICE rid;
    rid.hwndTarget  = hWnd;
    rid.usUsage     = HID_USAGE_GENERIC_MOUSE;
    rid.usUsagePage = HID_USAGE_PAGE_GENERIC;
    rid.dwFlags     = RIDEV_INPUTSINK |
                      RIDEV_DEVNOTIFY;
  
    RegisterRawInputDevices_Original (&rid, 1, sizeof RAWINPUTDEVICE);
  }


  SK_ApplyQueuedHooks ();
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

using RegisterClassA_pfn = ATOM (WINAPI *)(const WNDCLASSA* lpWndClass);
using RegisterClassW_pfn = ATOM (WINAPI *)(const WNDCLASSW* lpWndClass);
using RegisterClassExA_pfn = ATOM (WINAPI *)(const WNDCLASSEXA* lpWndClassEx);
using RegisterClassExW_pfn = ATOM (WINAPI *)(const WNDCLASSEXW* lpWndClassEx);

RegisterClassA_pfn   RegisterClassA_Original   = nullptr;
RegisterClassW_pfn   RegisterClassW_Original   = nullptr;
RegisterClassExA_pfn RegisterClassExA_Original = nullptr;
RegisterClassExW_pfn RegisterClassExW_Original = nullptr;

ATOM
WINAPI
RegisterClassA_Detour (const WNDCLASSA* lpWndClass)
{
  SK_LOG_FIRST_CALL

  auto wnd_class = *lpWndClass;

  if (lpWndClass->hInstance == GetModuleHandle (nullptr))
  {
    game_window.WndProc_Original = wnd_class.lpfnWndProc;
    wnd_class.lpfnWndProc        = (WNDPROC)SK_DetourWindowProc;
    caught_register              = true;
  }

  return RegisterClassA_Original (&wnd_class);
}

ATOM
WINAPI
RegisterClassW_Detour (const WNDCLASSW* lpWndClass)
{
  SK_LOG_FIRST_CALL

  auto wnd_class = *lpWndClass;

  if (lpWndClass->hInstance == GetModuleHandle (nullptr))
  {
    game_window.WndProc_Original = wnd_class.lpfnWndProc;
    wnd_class.lpfnWndProc        = (WNDPROC)SK_DetourWindowProc;
    caught_register              = true;
  }

  return RegisterClassW_Original (&wnd_class);
}

ATOM
WINAPI
RegisterClassExA_Detour (const WNDCLASSEXA* lpWndClassEx)
{
  SK_LOG_FIRST_CALL

  auto wnd_class_ex = *lpWndClassEx;

  if (lpWndClassEx->hInstance == GetModuleHandle (nullptr))
  {
    game_window.WndProc_Original = wnd_class_ex.lpfnWndProc;
    wnd_class_ex.lpfnWndProc     = (WNDPROC)SK_DetourWindowProc;
    caught_register              = true;
  }

  return RegisterClassExA_Original (&wnd_class_ex);
}

ATOM
WINAPI
RegisterClassExW_Detour (const WNDCLASSEXW* lpWndClassEx)
{
  SK_LOG_FIRST_CALL

  auto wnd_class_ex = *lpWndClassEx;

  if (lpWndClassEx->hInstance == GetModuleHandle (nullptr))
  {
    game_window.WndProc_Original = wnd_class_ex.lpfnWndProc;
    wnd_class_ex.lpfnWndProc     = (WNDPROC)SK_DetourWindowProc;
    caught_register              = true;
  }

  return RegisterClassExW_Original (&wnd_class_ex);
}

using CreateWindowExA_pfn = HWND (WINAPI *)(
    _In_     DWORD     dwExStyle,
    _In_opt_ LPCSTR    lpClassName,
    _In_opt_ LPCSTR    lpWindowName,
    _In_     DWORD     dwStyle,
    _In_     int       X,
    _In_     int       Y,
    _In_     int       nWidth,
    _In_     int       nHeight,
    _In_opt_ HWND      hWndParent,
    _In_opt_ HMENU     hMenu,
    _In_opt_ HINSTANCE hInstance,
    _In_opt_ LPVOID    lpParam );

using CreateWindowExW_pfn = HWND (WINAPI *)(
    _In_     DWORD     dwExStyle,
    _In_opt_ LPCWSTR   lpClassName,
    _In_opt_ LPCWSTR   lpWindowName,
    _In_     DWORD     dwStyle,
    _In_     int       X,
    _In_     int       Y,
    _In_     int       nWidth,
    _In_     int       nHeight,
    _In_opt_ HWND      hWndParent,
    _In_opt_ HMENU     hMenu,
    _In_opt_ HINSTANCE hInstance,
    _In_opt_ LPVOID    lpParam );

CreateWindowExA_pfn CreateWindowExA_Original = nullptr;
CreateWindowExW_pfn CreateWindowExW_Original = nullptr;

void
SK_MakeWindowHook (LPVOID class_proc, LPVOID wnd_proc)
{  
  if ( MH_OK ==
         MH_CreateHook (
                                                 class_proc,
                                        SK_DetourWindowProc,
               static_cast_p2p <void> (&game_window.WndProc_Original)
         )
    )
  {
    MH_QueueEnableHook (class_proc);
  
    dll_log.Log (L"[Window Mgr]  >> Hooked ClassProc.");
  
    game_window.hooked = false;
  }

  else
  if ( MH_OK ==
              MH_CreateHook (
                                                    wnd_proc,
                                         SK_DetourWindowProc,
                static_cast_p2p <void> (&game_window.WndProc_Original)
              )
          )
  {
    MH_QueueEnableHook (wnd_proc);
  
    dll_log.Log (L"[Window Mgr]  >> Hooked WndProc.");
  
    game_window.hooked = false;
  }

  SK_ApplyQueuedHooks ();
}

HWND
WINAPI
CreateWindowExA_Detour (
    _In_ DWORD dwExStyle,
    _In_opt_ LPCSTR lpClassName,
    _In_opt_ LPCSTR lpWindowName,
    _In_ DWORD dwStyle,
    _In_ int X,
    _In_ int Y,
    _In_ int nWidth,
    _In_ int nHeight,
    _In_opt_ HWND hWndParent,
    _In_opt_ HMENU hMenu,
    _In_opt_ HINSTANCE hInstance,
    _In_opt_ LPVOID lpParam )
{
  if (SK_GetCallingDLL () == SK_GetDLL ())
  {
    return
      CreateWindowExA_Original (dwExStyle, lpClassName, lpWindowName, dwStyle,
                                X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
  }

  if ((dwStyle & WS_POPUP))
  {
    if (ReadAcquire (&SK_bypass_dialog_active))
    {
      if (IsGUIThread (FALSE))
      {
        while (ReadAcquire ((&SK_bypass_dialog_active)))
          MsgWaitForMultipleObjectsEx (0, nullptr, 1, QS_ALLINPUT, MWMO_ALERTABLE);
      }

      else
      {
        while (ReadAcquire ((&SK_bypass_dialog_active)))
          WaitForMultipleObjectsEx (0, nullptr, 1, QS_ALLINPUT, MWMO_ALERTABLE);
      }
    }
  }

  HWND hWndRet =
    CreateWindowExA_Original (dwExStyle, lpClassName, lpWindowName, dwStyle,
                              X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);


  //// Early render window detection. Every little bit helps when the possibility of installing this hook first exists.
  //if (hWndRet != 0 && (hInstance == nullptr || hInstance == GetModuleHandle (nullptr)) && (! caught_register))
  //{
  //  if (dwStyle & (WS_OVERLAPPEDWINDOW))
  //  {
  //    caught_register = true;
  //      
  //    SK_LOG0 ( (L"[!] > ApplicationWindow: [Class: %16hs, Title: %34hs]", lpClassName, lpWindowName), L"WindowFind" );
  //
  //    game_window.hWnd    = hWndRet;
  //    game_window.unicode = false;
  //    game_window.active  = true;
  //
  //    game_window.GetWindowLongPtr = GetWindowLongPtrA_Original;
  //    game_window.SetWindowLongPtr = SetWindowLongPtrA_Original;
  //  //game_window.SetClassLongPtr  = SetClassLongPtrW_Original;
  //    game_window.DefWindowProc    = (DefWindowProc_pfn)
  //      GetProcAddress ( GetModuleHandle (L"user32.dll"),
  //                         "DefWindowProcA" );
  //    game_window.CallWindowProc   = (CallWindowProc_pfn)
  //      GetProcAddress ( GetModuleHandle (L"user32.dll"),
  //                         "CallWindowProcA" );
  //
  //    SK_MakeWindowHook ((LPVOID)GetClassLongPtrA (hWndRet, GCLP_WNDPROC), (LPVOID)GetWindowLongPtrA (hWndRet, GWLP_WNDPROC));
  //
  //    //game_window.WndProc_Original = (WNDPROC)GetClassLongPtrA (hWndRet, GCLP_WNDPROC);
  //    //                                        SetClassLongPtrA (hWndRet, GCLP_WNDPROC, (LONG_PTR)(WNDPROC)SK_DetourWindowProc);
  //  }
  //}

  return hWndRet;
}

HWND
WINAPI
CreateWindowExW_Detour(
    _In_ DWORD dwExStyle,
    _In_opt_ LPCWSTR lpClassName,
    _In_opt_ LPCWSTR lpWindowName,
    _In_ DWORD dwStyle,
    _In_ int X,
    _In_ int Y,
    _In_ int nWidth,
    _In_ int nHeight,
    _In_opt_ HWND hWndParent,
    _In_opt_ HMENU hMenu,
    _In_opt_ HINSTANCE hInstance,
    _In_opt_ LPVOID lpParam)
{
  if (SK_GetCallingDLL () == SK_GetDLL ())
  {
    return
      CreateWindowExW_Original (dwExStyle, lpClassName, lpWindowName, dwStyle,
                                X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
  }

  if ((dwStyle & WS_POPUP))
  {
    if (ReadAcquire (&SK_bypass_dialog_active))
    {
      if (IsGUIThread (FALSE))
      {
        while (ReadAcquire ((&SK_bypass_dialog_active)))
          MsgWaitForMultipleObjectsEx (0, nullptr, 1, QS_ALLINPUT, MWMO_ALERTABLE);
      }

      else
      {
        while (ReadAcquire ((&SK_bypass_dialog_active)))
          WaitForMultipleObjectsEx (0, nullptr, 1, QS_ALLINPUT, MWMO_ALERTABLE);
      }
    }
  }

  HWND hWndRet =
    CreateWindowExW_Original (dwExStyle, lpClassName, lpWindowName, dwStyle,
                              X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);

  //// Early render window detection. Every little bit helps when the possibility of installing this hook first exists.
  //if (hWndRet != 0 && (hInstance == nullptr || hInstance == GetModuleHandle (nullptr)) && (! caught_register))
  //{
  //  if (dwStyle & (WS_OVERLAPPEDWINDOW))
  //  {
  //    caught_register = true;
  //
  //    SK_LOG0 ( (L"[!] > ApplicationWindow: [Class: %16ws, Title: %34ws]", lpClassName, lpWindowName), L"WindowFind" );
  //
  //    game_window.hWnd    = hWndRet;
  //    game_window.unicode = true;
  //    game_window.active  = true;
  //
  //    game_window.GetWindowLongPtr = GetWindowLongPtrW_Original;
  //    game_window.SetWindowLongPtr = SetWindowLongPtrW_Original;
  //  //game_window.SetClassLongPtr  = SetClassLongPtrW_Original;
  //    game_window.DefWindowProc    = (DefWindowProc_pfn)
  //      GetProcAddress ( GetModuleHandle (L"user32.dll"),
  //                         "DefWindowProcW" );
  //    game_window.CallWindowProc   = (CallWindowProc_pfn)
  //      GetProcAddress ( GetModuleHandle (L"user32.dll"),
  //                         "CallWindowProcW" );
  //
  //    SK_MakeWindowHook ((LPVOID)GetClassLongPtrW (hWndRet, GCLP_WNDPROC), (LPVOID)GetWindowLongPtrW (hWndRet, GWLP_WNDPROC));
  //
  //    //game_window.WndProc_Original = (WNDPROC)GetClassLongPtrW (hWndRet, GCLP_WNDPROC);
  //    //                                        SetClassLongPtrW (hWndRet, GCLP_WNDPROC, (LONG_PTR)(WNDPROC)SK_DetourWindowProc);
  //  }
  //}

  return hWndRet;
}

void
SK_HookWinAPI (void)
{
  // Initialize the Window Manager
  SK_WindowManager::getInstance ();


  //int phys_w         = GetDeviceCaps (GetDC (GetDesktopWindow ()), PHYSICALWIDTH);
  //int phys_h         = GetDeviceCaps (GetDC (GetDesktopWindow ()), PHYSICALHEIGHT);
  //int phys_off_x     = GetDeviceCaps (GetDC (GetDesktopWindow ()), PHYSICALOFFSETX);
  //int phys_off_y     = GetDeviceCaps (GetDC (GetDesktopWindow ()), PHYSICALOFFSETY);
  //int scale_factor_x = GetDeviceCaps (GetDC (GetDesktopWindow ()), SCALINGFACTORX);
  //int scale_factor_y = GetDeviceCaps (GetDC (GetDesktopWindow ()), SCALINGFACTORY);

  //SK_LOG0 ( ( L" Physical Coordinates  ::  { %lux%lu) + <%lu,%lu> * {%lu:%lu}",
                //phys_w, phys_h,
                //phys_off_x, phys_off_y,
                //scale_factor_x, scale_factor_y ),
              //L"Window Sys" );


#if 1
  SK_CreateDLLHook2 (       L"user32.dll",
                             "SetWindowPos",
                              SetWindowPos_Detour,
     static_cast_p2p <void> (&SetWindowPos_Original) );

  SK_CreateDLLHook2 (       L"user32.dll",
                             "SetWindowPlacement",
                              SetWindowPlacement_Detour,
     static_cast_p2p <void> (&SetWindowPlacement_Original) );

  SK_CreateDLLHook2 (       L"user32.dll",
                             "MoveWindow",
                              MoveWindow_Detour,
     static_cast_p2p <void> (&MoveWindow_Original) );
#else
    SetWindowPos_Original =
      (SetWindowPos_pfn)
        GetProcAddress ( GetModuleHandleW (L"user32.dll"), 
                         "SetWindowPos" );
    SetWindowPlacement_Original =
      (SetWindowPlacement_pfn)
        GetProcAddress ( GetModuleHandleW (L"user32.dll"), 
                         "SetWindowPlacement" );

    MoveWindow_Original =
      (MoveWindow_pfn)
        GetProcAddress ( GetModuleHandleW (L"user32.dll"), 
                         "MoveWindow" );
#endif


  SK_CreateDLLHook2 (       L"user32.dll",
                             "ClipCursor",
                              ClipCursor_Detour,
     static_cast_p2p <void> (&ClipCursor_Original) );


  SK_CreateDLLHook2 (       L"user32.dll",
                             "CreateWindowExW",
                              CreateWindowExW_Detour,
     static_cast_p2p <void> (&CreateWindowExW_Original) );

  SK_CreateDLLHook2 (       L"user32.dll",
                             "CreateWindowExA",
                              CreateWindowExA_Detour,
     static_cast_p2p <void> (&CreateWindowExA_Original) );


// These functions are dispatched through the Ptr version, so
//   hooking them in the 64-bit version would be a bad idea.
  SK_CreateDLLHook2 (       L"user32.dll",
                             "SetWindowLongA",
                              SetWindowLongA_Detour,
     static_cast_p2p <void> (&SetWindowLongA_Original) );

  SK_CreateDLLHook2 (       L"user32.dll",
                             "SetWindowLongW",
                              SetWindowLongW_Detour,
     static_cast_p2p <void> (&SetWindowLongW_Original) );

  SK_CreateDLLHook2 (       L"user32.dll",
                             "GetWindowLongA",
                              GetWindowLongA_Detour,
     static_cast_p2p <void> (&GetWindowLongA_Original) );

  SK_CreateDLLHook2 (       L"user32.dll",
                             "GetWindowLongW",
                              GetWindowLongW_Detour,
     static_cast_p2p <void> (&GetWindowLongW_Original) );

  //SK_CreateDLLHook2 (       L"user32.dll",
  //                           "SetClassLongA",
  //                            SetClassLongA_Detour,
  //   static_cast_p2p <void> (&SetClassLongA_Original) );
  //
  //SK_CreateDLLHook2 (       L"user32.dll",
  //                           "SetClassLongW",
  //                            SetClassLongW_Detour,
  //   static_cast_p2p <void> (&SetClassLongW_Original) );
  //
  //SK_CreateDLLHook2 (       L"user32.dll",
  //                           "GetClassLongA",
  //                            GetClassLongA_Detour,
  //   static_cast_p2p <void> (&GetClassLongA_Original) );
  //
  //SK_CreateDLLHook2 (       L"user32.dll",
  //                           "GetClassLongW",
  //                            GetClassLongW_Detour,
  //   static_cast_p2p <void> (&GetClassLongW_Original) );

#ifdef _WIN64
  SK_CreateDLLHook2 (       L"user32.dll",
                             "SetWindowLongPtrA",
                              SetWindowLongPtrA_Detour,
     static_cast_p2p <void> (&SetWindowLongPtrA_Original) );

  SK_CreateDLLHook2 (       L"user32.dll",
                             "SetWindowLongPtrW",
                              SetWindowLongPtrW_Detour,
     static_cast_p2p <void> (&SetWindowLongPtrW_Original) );

  SK_CreateDLLHook2 (       L"user32.dll",
                             "GetWindowLongPtrA",
                              GetWindowLongPtrA_Detour,
     static_cast_p2p <void> (&GetWindowLongPtrA_Original) );

  SK_CreateDLLHook2 (       L"user32.dll",
                             "GetWindowLongPtrW",
                              GetWindowLongPtrW_Detour,
     static_cast_p2p <void> (&GetWindowLongPtrW_Original) );

  //SK_CreateDLLHook2 (       L"user32.dll",
  //                           "SetClassLongPtrA",
  //                            SetClassLongPtrA_Detour,
  //   static_cast_p2p <void> (&SetClassLongPtrA_Original) );
  //
  //SK_CreateDLLHook2 (       L"user32.dll",
  //                           "SetClassLongPtrW",
  //                            SetClassLongPtrW_Detour,
  //   static_cast_p2p <void> (&SetClassLongPtrW_Original) );
  //
  //SK_CreateDLLHook2 (       L"user32.dll",
  //                           "GetClassLongPtrA",
  //                            GetClassLongPtrA_Detour,
  //   static_cast_p2p <void> (&GetClassLongPtrA_Original) );
  //
  //SK_CreateDLLHook2 (       L"user32.dll",
  //                           "GetClassLongPtrW",
  //                            GetClassLongPtrW_Detour,
  //   static_cast_p2p <void> (&GetClassLongPtrW_Original) );
#endif

#if 0
  SK_CreateDLLHook2 (         L"user32.dll",
                             "GetWindowRect",
                              GetWindowRect_Detour,
     static_cast_p2p <void> (&GetWindowRect_Original) );
#else
    GetWindowRect_Original =
      reinterpret_cast <GetWindowRect_pfn> (
        GetProcAddress ( GetModuleHandleW (L"user32.dll"), 
                                            "GetWindowRect" )
      );
#endif

#if 0
  SK_CreateDLLHook2 (       L"user32.dll",
                             "GetClientRect",
                              GetClientRect_Detour,
     static_cast_p2p <void> (&GetClientRect_Original) );
#else
    GetClientRect_Original =
      reinterpret_cast <GetClientRect_pfn> (
        GetProcAddress ( GetModuleHandleW (L"user32.dll"), 
                                            "GetClientRect" )
      );
#endif

  SK_CreateDLLHook2 (       L"user32.dll",
                             "AdjustWindowRect",
                              AdjustWindowRect_Detour,
     static_cast_p2p <void> (&AdjustWindowRect_Original) );

  SK_CreateDLLHook2 (       L"user32.dll",
                             "AdjustWindowRectEx",
                              AdjustWindowRectEx_Detour,
     static_cast_p2p <void> (&AdjustWindowRectEx_Original) );

  SK_CreateDLLHook2 (       L"user32.dll",
                             "GetSystemMetrics",
                              GetSystemMetrics_Detour,
     static_cast_p2p <void> (&GetSystemMetrics_Original) );

  SK_ApplyQueuedHooks ();

#ifndef _WIN64
  SetWindowLongPtrA_Original = SetWindowLongA_Original;
  SetWindowLongPtrW_Original = SetWindowLongW_Original;
#else
#endif
}


bool
sk_window_s::needsCoordTransform (void)
{
  if (! config.window.res.override.fix_mouse)
    return false;

  bool dynamic_window =
    (config.window.borderless /*&& config.window.fullscreen*/);

  if (! dynamic_window)
    return false;

  return (! coord_remap.identical);
}

void
sk_window_s::updateDims (void)
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


LRESULT
sk_window_s::DefProc (
  _In_ UINT   Msg,
  _In_ WPARAM wParam,
  _In_ LPARAM lParam
)
{
  return DefWindowProc (hWnd, Msg, wParam, lParam);
}

LRESULT 
sk_window_s::CallProc (
    _In_ HWND    hWnd_,
    _In_ UINT    Msg,
    _In_ WPARAM  wParam,
    _In_ LPARAM  lParam )
{
  if (hooked)
    return CallWindowProc (WndProc_Original, hWnd_, Msg, wParam, lParam);
  else
    return WndProc_Original (hWnd_, Msg, wParam, lParam);
}


LRESULT
SK_COMPAT_SafeCallProc (sk_window_s* pWin, HWND hWnd_, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  __try {
    return pWin->CallProc (hWnd_, Msg, wParam, lParam);
  }
  __except (EXCEPTION_EXECUTE_HANDLER)
  {
    return 1;
  }
}