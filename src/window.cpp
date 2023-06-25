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

#include <SpecialK/stdafx.h>
#include <imgui/backends/imgui_d3d11.h>

#include <future>

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"Window Mgr"

// For SK_D3D9_TriggerReset .... (should probably be in SK_RenderBackend_v{2|3+})
#include <SpecialK/render/d3d9/d3d9_backend.h>

static constexpr int SK_MAX_WINDOW_DIM = 16384;

// WS_SYSMENU keeps the window's icon unchanged
#define SK_BORDERLESS    ( WS_VISIBLE | WS_POPUP | WS_MINIMIZEBOX | WS_SYSMENU | \
                           WS_CLIPCHILDREN | WS_CLIPSIBLINGS )
#define SK_BORDERLESS_EX ( WS_EX_APPWINDOW )

#define SK_LOG_LEVEL_UNTESTED

#define SK_WINDOW_LOG_CALL0() if (config.system.log_level >= 0) SK_LOG_CALL ("Window Mgr")
#define SK_WINDOW_LOG_CALL1() if (config.system.log_level >= 1) SK_LOG_CALL ("Window Mgr")
#define SK_WINDOW_LOG_CALL2() if (config.system.log_level >= 2) SK_LOG_CALL ("Window Mgr")
#define SK_WINDOW_LOG_CALL3() if (config.system.log_level >= 3) SK_LOG_CALL ("Window Mgr")
#define SK_WINDOW_LOG_CALL4() if (config.system.log_level >= 4) SK_LOG_CALL ("Window Mgr")

#ifdef SK_LOG_LEVEL_UNTESTED
# define SK_WINDOW_LOG_CALL_UNTESTED() SK_LOG_CALL ("Window Mgr");
#else
# define SK_WINDOW_LOG_CALL_UNTESTED() { }
#endif

extern bool SK_WantBackgroundRender ();

#define PreventAlwaysOnTop 0
#define        AlwaysOnTop 1
#define   SmartAlwaysOnTop 2

bool SK_Window_OnFocusChange (HWND hWndNewTarget, HWND hWndOld);

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

ClipCursor_pfn           ClipCursor_Original           = nullptr;
GetCursorPos_pfn         GetCursorPos_Original         = nullptr;
SetCursorPos_pfn         SetCursorPos_Original         = nullptr;
GetCursorPos_pfn         GetPhysicalCursorPos_Original = nullptr;
SetCursorPos_pfn         SetPhysicalCursorPos_Original = nullptr;
GetMessagePos_pfn        GetMessagePos_Original        = nullptr;
SendInput_pfn            SendInput_Original            = nullptr;
mouse_event_pfn          mouse_event_Original          = nullptr;

SetWindowPos_pfn         SetWindowPos_Original         = nullptr;
SetWindowPlacement_pfn   SetWindowPlacement_Original   = nullptr;
MoveWindow_pfn           MoveWindow_Original           = nullptr;
SetWindowLong_pfn        SetWindowLongW_Original       = nullptr;
SetWindowLong_pfn        SetWindowLongA_Original       = nullptr;
GetWindowLong_pfn        GetWindowLongW_Original       = nullptr;
GetWindowLong_pfn        GetWindowLongA_Original       = nullptr;
SetWindowLongPtr_pfn     SetWindowLongPtrW_Original    = nullptr;
SetWindowLongPtr_pfn     SetWindowLongPtrA_Original    = nullptr;
GetWindowLongPtr_pfn     GetWindowLongPtrW_Original    = nullptr;
GetWindowLongPtr_pfn     GetWindowLongPtrA_Original    = nullptr;
AdjustWindowRect_pfn     AdjustWindowRect_Original     = nullptr;
AdjustWindowRectEx_pfn   AdjustWindowRectEx_Original   = nullptr;

GetSystemMetrics_pfn     GetSystemMetrics_Original     = nullptr;

GetWindowRect_pfn        GetWindowRect_Original        = nullptr;
GetClientRect_pfn        GetClientRect_Original        = nullptr;

using GetWindowInfo_pfn = BOOL (WINAPI *)(HWND, PWINDOWINFO);
      GetWindowInfo_pfn
      GetWindowInfo_Original = nullptr;

bool
SK_EarlyDispatchMessage (MSG *lpMsg, bool remove, bool peek = false);

BOOL
SK_Window_IsUnicode (HWND hWnd)
{
  struct cache_entry_s {
    explicit cache_entry_s (HWND hWnd) : hwnd (hWnd)
    {
      unicode = IsWindowUnicode (hWnd);
    };

    BOOL test_and_set (HWND hWnd)
    {
      if (hWnd == hwnd)
        return unicode;

      cache_entry_s new_record (hWnd);
      cache_entry_s old_record = *this;
                                 *this =
                    new_record;

      return unicode;
    }

    BOOL unicode;
    HWND hwnd;
  } static cache (hWnd);

  return
    cache.test_and_set (hWnd);
}

BOOL
SK_IsChild (HWND hWndParent, HWND hWnd)
{
  static Concurrency::concurrent_unordered_map
    < HWND, HWND > parents_;

  if (parents_.count (hWnd))
  {
    if (parents_ [hWnd] == hWndParent)
      return TRUE;

    return FALSE;
  }

  parents_ [hWnd] =
    GetParent (hWnd);

  return FALSE;
}




struct sk_window_message_dispatch_s {
public:
  concurrency::concurrent_unordered_map <HWND, bool> active_windows;
  concurrency::concurrent_unordered_map <HWND, bool> moving_windows;
};

SK_LazyGlobal <sk_window_message_dispatch_s> wm_dispatch;

bool override_window_rects = false;

// TODO: Refactor that structure above into this class ;)
//
//  Also, combine a few of the input manager settings with window manager
class SK_WindowManager : public SK_IVariableListener
{
public:
  // Max length of human-readable resolution strings
  static constexpr int _RES_STR_MAX = 32;

  static constexpr bool StyleHasBorder (DWORD_PTR style)
  {
    return ( ( style == 0x0            ) ||
             ( style  &  WS_BORDER     ) ||
             ( style  &  WS_THICKFRAME ) ||
             ( style  &  WS_DLGFRAME   )    );
  }

  bool OnVarChange (SK_IVariable* var, void* val) override
  {
    if (var == preferred_monitor_)
    {
      if (val != nullptr)
      {
        const int set  =
          *static_cast <int *> (val);

        if (config.display.monitor_idx != set)
        {
          config.display.monitor_idx      = set;
          config.display.monitor_handle   =   0; // Clear until we validate the idx
          config.display.monitor_path_ccd = L"";

          if (set != 0) // 0 = No Preference (i.e. clear preference w/o moving)
          {
            EnumDisplayMonitors (nullptr, nullptr, [](HMONITOR hMonitor, HDC hDC, LPRECT lpRect, LPARAM lParam) -> BOOL
            {
              std::ignore = hDC;
              std::ignore = lpRect;
              std::ignore = lParam;

              MONITORINFOEXW
                mi        = {         };
                mi.cbSize = sizeof (mi);

              if (GetMonitorInfoW (hMonitor, &mi))
              {
                if (config.display.monitor_idx > 0)
                {
                  if (0 == _wcsicmp (mi.szDevice, SK_FormatStringW (LR"(\\.\DISPLAY%i)", config.display.monitor_idx).c_str ()))
                  {
                    config.display.monitor_handle = hMonitor;
                    return FALSE;
                  }
                }

                else if (config.display.monitor_idx == -1)
                {
                  if (mi.dwFlags & MONITORINFOF_PRIMARY)
                  {
                    config.display.monitor_handle = hMonitor;
                    return FALSE;
                  }
                }
              }

              return TRUE;
            }, 0);

            static auto& rb =
              SK_GetCurrentRenderBackend ();

            // TODO: Use this isntead config.display.monitor_handle in RepositionIfNeeded
            //
            //   This will prevent locking a game window unless the user wants that
            //
            rb.next_monitor =
              config.display.monitor_handle;

            if (config.display.monitor_handle != 0)
            {
              for ( const auto& display : rb.displays )
              {
                if (display.monitor == rb.next_monitor)
                {
                  config.display.monitor_path_ccd =
                                 display.path_name;
                }
              }
            }
          }
        }
      }

      *static_cast <int *> (var->getValuePointer ()) =
        config.display.monitor_idx;

      return true;
    }

    if (var == monitor_)
    {
      if (val != nullptr)
      {
        int set  =
          *static_cast <int *> (val);

        config.display.monitor_handle = 0;

        static auto& rb =
          SK_GetCurrentRenderBackend ();

        UINT highest_idx = 1,
             lowest_idx  =   SK_RenderBackend_V2::_MAX_DISPLAYS;
        for ( int i = 0; i < SK_RenderBackend_V2::_MAX_DISPLAYS ; ++i )
        {
          if (rb.displays [i].attached)
          {
            highest_idx = std::max (rb.displays [i].idx, highest_idx);
            lowest_idx  = std::min (rb.displays [i].idx,  lowest_idx);
          }
        }

        if (set > sk::narrow_cast <int> (highest_idx))
            set = sk::narrow_cast <int> ( lowest_idx);
        if (set < sk::narrow_cast <int> ( lowest_idx))
            set = sk::narrow_cast <int> (highest_idx);

        for ( int i = 0; i < SK_RenderBackend_V2::_MAX_DISPLAYS ; ++i )
        {
          if ( rb.displays [i].attached &&
               rb.displays [i].idx      == static_cast <UINT> (set) )
          {
            config.display.monitor_handle =
               rb.displays [i].monitor;

            SK_SaveConfig ();
          }
        }

        rb.next_monitor =
          config.display.monitor_handle;

        *static_cast <int *> (var->getValuePointer ()) =
          set;
      }

      return true;
    }

    if (var == top_most_)
    {
      if (val != nullptr)
      {
        const bool orig = SK_Window_IsTopMost (game_window.hWnd);
        const bool set  =
          *static_cast <bool *> (val);

        if (set != orig)
        {
          SK_Window_SetTopMost (set, set, game_window.hWnd);
        }
      }

      *static_cast <bool *> (var->getValuePointer ()) =
        SK_Window_IsTopMost (game_window.hWnd);

      return true;
    }

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
            SK_ClipCursor   (&game_window.cursor_clip);
            SK_AdjustWindow ();
          }

          else
            SK_ClipCursor (nullptr);
        }

        else
        {
          SK_GetWindowRect (game_window.hWnd, &game_window.actual.window);
          SK_GetClientRect (game_window.hWnd, &game_window.actual.client);

          RECT            rcClip;
          CopyRect      (&rcClip,             &game_window.actual.window);
          SK_ClipCursor (&rcClip);
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
          SK_ClipCursor (nullptr);

        else if (config.window.confine_cursor)
        {
          SK_GetWindowRect (game_window.hWnd, &game_window.actual.window);
          SK_GetClientRect (game_window.hWnd, &game_window.actual.client);

          RECT            rcClip;
          CopyRect      (&rcClip,             &game_window.actual.window);
          SK_ClipCursor (&rcClip);
        }

        else
        {
          SK_ClipCursor   (&game_window.cursor_clip);
          SK_AdjustWindow ();
        }
      }

      return true;
    }

    if ( var == center_window_ || var == x_offset_   || var == y_offset_   ||
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
          if ( *static_cast <int *> (val) >= -SK_MAX_WINDOW_DIM &&
               *static_cast <int *> (val) <=  SK_MAX_WINDOW_DIM    )
          {
            config.window.offset.x.absolute = *static_cast <signed int *> (val);
            config.window.offset.x.percent  = 0.0f;
          }
        }

        else if (var == y_offset_)
        {
          if ( *static_cast <int *> (val) >= -SK_MAX_WINDOW_DIM &&
               *static_cast <int *> (val) <=  SK_MAX_WINDOW_DIM    )
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

          if (! config.window.borderless)
          {
            game_window.attach_border = true;
          }

          SK_AdjustBorder ();
        }

        else if (var == fullscreen_)
        {
          if ( config.window.fullscreen != *static_cast <bool *> (val) &&
             ( config.window.borderless || (config.window.fullscreen) ) )
          {
            config.window.fullscreen = *static_cast <bool *> (val);

            static RECT last_known_client = { };
            static RECT last_known_window = { };

            RECT client            = { };

            SK_GetClientRect (game_window.hWnd, &client);

            HMONITOR hMonitor = config.display.monitor_handle != 0 ?
                                config.display.monitor_handle      :
              MonitorFromWindow ( game_window.hWnd,
                                    config.display.monitor_default );

            MONITORINFO mi  = { };
            mi.cbSize       =  sizeof (mi);
            GetMonitorInfo (hMonitor, &mi);

            if ( (client.right  - client.left != mi.rcMonitor.right  - mi.rcMonitor.left ) &&
                 (client.bottom - client.top  != mi.rcMonitor.bottom - mi.rcMonitor.top  ) )
            {
              SK_GetClientRect (game_window.hWnd, &last_known_client);
              SK_GetWindowRect (game_window.hWnd, &last_known_window);
            }

            const unsigned int orig_x = config.window.res.override.x;
            const unsigned int orig_y = config.window.res.override.y;

            static unsigned int persist_x = orig_x;
            static unsigned int persist_y = orig_y;

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
              //if (! wm_dispatch->moving_windows.count (game_window.hWnd))
                if (config.window.confine_cursor)
                {
                  const RECT clip =
                    game_window.actual.window;

                  SK_ClipCursor (&clip);
                }
              }

              else
                SK_ClipCursor (nullptr);
            }

            // Go Fullscreen (Stretch Window to Fill)
            else
            {
              config.window.res.override.x = mi.rcMonitor.right  - mi.rcMonitor.left;
              config.window.res.override.y = mi.rcMonitor.bottom - mi.rcMonitor.top;

              game_window.actual.client =
                RECT { 0,                                    0,
                       mi.rcMonitor.right-mi.rcMonitor.left, mi.rcMonitor.bottom-mi.rcMonitor.top };

              CopyRect (&game_window.actual.window, &mi.rcMonitor);
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

            if (config.window.confine_cursor)
              SK_ClipCursor (&game_window.actual.window);

            return true;
          }
        }

        else if (var == x_override_ || var == y_override_)
        {
          if ((! config.window.borderless) || config.window.fullscreen)
            return false;

          if (var == x_override_)
          {
            config.window.res.override.x =
              *static_cast <unsigned int *> (val);

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

        _snprintf_s ( override_res, _RES_STR_MAX, _TRUNCATE, "%lux%lu",
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

        if (config.window.background_mute && (! SK_IsGameWindowActive ()))
        {
          muteGame (true);
        }

        else if (! config.window.background_mute)
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

      if (val != nullptr)
      {
        char         szTemp [_RES_STR_MAX] = { };
        strncat_s  ( szTemp, _RES_STR_MAX, *static_cast <char **> (val),
                             _TRUNCATE );
        _snscanf_s ( szTemp, _RES_STR_MAX, "%ux%u", &x, &y);
      }

      static constexpr int _MIN_VALID_X = 320;
      static constexpr int _MIN_VALID_Y = 240;

      if ( ( x > _MIN_VALID_X && x < SK_MAX_WINDOW_DIM &&
             y > _MIN_VALID_Y && y < SK_MAX_WINDOW_DIM ) || (x == 0 && y == 0))
      {
        config.window.res.override.x = x;
        config.window.res.override.y = y;

        SK_AdjustWindow ();

        auto *pszRes =
          static_cast <char *> (((SK_IVarStub <char *> *)var)->getValuePointer ());

        _snprintf_s (pszRes, _RES_STR_MAX, _TRUNCATE, "%ux%u", x, y);

        return true;
      }

      auto *pszRes =
        static_cast <char *> (((SK_IVarStub <char *> *)var)->getValuePointer ());

      _snprintf_s (pszRes, _RES_STR_MAX, _TRUNCATE, "INVALID");

      return false;
    }

    return false;
  }

  virtual ~SK_WindowManager (void) = default;

  SK_WindowManager (void)
  {
    SK_ICommandProcessor* cmd =
      SK_GetCommandProcessor ();

    if (cmd == nullptr)
    {
      SK_ReleaseAssert (! "Could not create or get the Special K Command Processor!");
      return;
    }

    preferred_monitor_ =
      SK_CreateVar (SK_IVariable::Int,&config.display.monitor_idx,this);

    cmd->AddVariable (
      "Window.PreferredMonitor",
              preferred_monitor_ );

    static int current_monitor =
        config.display.monitor_idx;

    monitor_ =
      SK_CreateVar (SK_IVariable::Int,&current_monitor,this);

    cmd->AddVariable (
      "Window.Monitor",
              monitor_ );

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

    static bool dontcare;
    top_most_ =
      SK_CreateVar (SK_IVariable::Boolean, &dontcare, this);

    cmd->AddVariable (
      "Window.TopMost",
      top_most_
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

    _snprintf_s ( override_res, _RES_STR_MAX, _TRUNCATE, "%lux%lu",
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
        pWindowManager = std::make_unique <SK_WindowManager> ();

    return
      pWindowManager.get ();
  }

  static void muteGame (bool bMute)
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

  SK_IVariable* monitor_           = nullptr;
  SK_IVariable* preferred_monitor_ = nullptr;
  SK_IVariable* borderless_        = nullptr;
  SK_IVariable* background_mute_   = nullptr;
  SK_IVariable* confine_cursor_    = nullptr;
  SK_IVariable* top_most_          = nullptr;
  SK_IVariable* unconfine_cursor_  = nullptr;
  SK_IVariable* center_window_     = nullptr;
  SK_IVariable* fullscreen_        = nullptr;
  SK_IVariable* x_override_        = nullptr;
  SK_IVariable* y_override_        = nullptr;
  SK_IVariable* res_override_      = nullptr; // Set X and Y at the same time
  SK_IVariable* fix_mouse_         = nullptr;
  SK_IVariable* x_offset_          = nullptr;
  SK_IVariable* y_offset_          = nullptr;

  SK_IVariable* x_off_pct_         = nullptr;
  SK_IVariable* y_off_pct_         = nullptr;

  SK_IVariable* static_rects_      = nullptr; // Fake the game into thinking the
                                              //   client rectangle has not changed.
                                              //
                                              // This solves display scaling problems in Fallout 4 and
                                              //   Skyrim...

  char override_res [_RES_STR_MAX] = { };

private:
  static
    std::unique_ptr <SK_WindowManager>
                       pWindowManager;
};

void
ActivateWindow ( HWND hWnd,
                 bool active          = false,
                 HWND hWndDeactivated = 0 )
{
  // Sticky Keys (alt-tab) :)
  //
  static BYTE __LastKeyState [512] = { };


  UNREFERENCED_PARAMETER (hWndDeactivated); // Apparently it's not important?


  SK_ASSERT_NOT_THREADSAFE ();

  static auto& rb =
    SK_GetCurrentRenderBackend ();

  const bool is_game_window =
    ( hWnd == SK_GetGameWindow () || IsChild (game_window.hWnd, hWnd) || IsChild (hWnd, game_window.hWnd) );
  const bool state_changed  =
    ( wm_dispatch->active_windows [hWnd] != active &&
                  is_game_window );

  if (is_game_window)
  {
    game_window.active = active;
  }

  if (state_changed)
  {
    HWND hWndFocus =
      GetFocus ();

    if (game_window.active)
    {
      // Release the AltKin
      for ( BYTE VKey = 0x8 ; VKey < 255 ; ++VKey )
      {
        if (std::exchange (__LastKeyState [VKey], (BYTE)FALSE) != (BYTE)FALSE)
        {
          if ((SK_GetAsyncKeyState (VKey) & 0x8000) == 0x0/* &&
               SK_GetKeyState      (VKey)           != 0x0*/)
          {
            SK_keybd_event ((BYTE)VKey, 0, KEYEVENTF_KEYUP, 0);
          }
        }
      }

      InterlockedCompareExchange (&SK_RenderBackend::flip_skip, 3, 0);
    }

    else
    {
      for ( BYTE VKey = 0x8 ; VKey < 255 ; ++VKey )
      {
        if ( SK_GetKeyState      (VKey)           != 0x0 ||
            (SK_GetAsyncKeyState (VKey) & 0x8000) != 0x0)
        {
          std::exchange (__LastKeyState [VKey], (BYTE)TRUE);
        }
      }
    }

    BYTE              newKeyboardState [256] = { };
    SetKeyboardState (newKeyboardState);

    if (hWndFocus != game_window.hWnd && (is_game_window && (! game_window.active)))
    {
      BringWindowToTop    (hWndFocus);
      SetForegroundWindow (hWndFocus);
      SetActiveWindow     (hWndFocus);
    }

    struct {
      INT   GPU       = 0;
      DWORD dwProcess = NORMAL_PRIORITY_CLASS;

      bool proposeChange (INT _GPU, DWORD _dwProcess)
      {
        SK_ComPtr      <ID3D11Device> pDev11 =
          rb.getDevice <ID3D11Device> ();
        SK_ComPtr      <ID3D12Device> pDev12 =
          rb.getDevice <ID3D12Device> ();

        SK_ComQIPtr <IDXGIDevice>
            pDXGIDev (pDev12);
        if (pDXGIDev == nullptr)
            pDXGIDev = pDev11;

        bool success = false;

        if (_GPU != GPU || _dwProcess != dwProcess || config.priority.deny_foreign_change)
        {
          if (SetPriorityClass (GetCurrentProcess (), _dwProcess))
          {                               dwProcess = _dwProcess;
            success = true;
          }

          if (             pDXGIDev.p != nullptr)
          { if (SUCCEEDED (pDXGIDev.p->SetGPUThreadPriority (_GPU)))
                                                       GPU = _GPU;
          }
        }

        return success;
      }
    } static prio;

    if (! game_window.active)
    {
      if ( config.priority.raise_bg     ||
           config.priority.raise_always ||
           config.window.always_on_top  == SmartAlwaysOnTop )
      {
        bool bBackgroundBoost = false;

        if (SK_WantBackgroundRender ())
          bBackgroundBoost = config.priority.raise_bg;

        if ( config.priority.raise_always || bBackgroundBoost ||
             config.window.always_on_top  == SmartAlwaysOnTop )
             prio.proposeChange (3, ABOVE_NORMAL_PRIORITY_CLASS);
        else prio.proposeChange (0,       NORMAL_PRIORITY_CLASS);
      } else prio.proposeChange (0,       NORMAL_PRIORITY_CLASS);
    }

    SK_XInput_Enable (TRUE);

    if (game_window.active)
    {
      if ( config.priority.raise_fg     ||
           config.priority.raise_always ||
           config.window.always_on_top  == SmartAlwaysOnTop )
           prio.proposeChange (3, ABOVE_NORMAL_PRIORITY_CLASS);
      else prio.proposeChange (0,       NORMAL_PRIORITY_CLASS);
    }
    SK_Console::getInstance ()->reset ();

    if (config.window.background_mute)
      SK_WindowManager::getInstance ()->muteGame ((! game_window.active));

    // Keep Unity games from crashing at startup when forced into FULLSCREEN
    //
    //  ... also prevents a game from staying topmost when you Alt+Tab
    //

    if ( game_window.active && config.display.force_fullscreen &&
        ( static_cast <int> (rb.api)                &
          static_cast <int> (SK_RenderAPI::D3D9     )
        )                                           )
    {
      SetWindowLongPtrW    (game_window.hWnd, GWL_EXSTYLE,
       ( GetWindowLongPtrW (game_window.hWnd, GWL_EXSTYLE) &
                         ~(WS_EX_TOPMOST | WS_EX_NOACTIVATE
                                         | WS_EX_TOOLWINDOW)
       )                                 | WS_EX_APPWINDOW );

      SK_D3D9_TriggerReset (false);
    }

    if (game_window.active)
    {
      // Engage fullscreen
      if ( config.display.force_fullscreen )
      {
        rb.requestFullscreenMode (true);
      }

      DWORD                                                 dwProcess;
      GetWindowThreadProcessId (SK_GetForegroundWindow (), &dwProcess);

      if (dwProcess != GetCurrentProcessId ())
      {
        // Re-engage cursor clipping behavior
        SK_Window_RepositionIfNeeded ();

        if (rb.swapchain != nullptr)
        {
          auto* pLimiter =
            SK::Framerate::GetLimiter (rb.swapchain);

          pLimiter->reset (true);
        }
      }
    }

    else
    {
      if ((! rb.fullscreen_exclusive) && SK_WantBackgroundRender ())
      {
        game_window.cursor_visible =
          ShowCursor (TRUE) >= 1;
          ShowCursor (FALSE);
      }
    }

    if (config.window.confine_cursor)
    {
      if (game_window.active)
      {
        SK_LOG4 ( ( L"Confining Mouse Cursor" ),
                    L"Window Mgr" );

      //if (! wm_dispatch->moving_windows.count (game_window.hWnd))
        {
          ////// XXX: Is this really necessary? State should be consistent unless we missed
          //////        an event --- Write unit test?
          SK_GetWindowRect (game_window.hWnd, &game_window.actual.window);
          SK_ClipCursor    (&game_window.actual.window);
        }
      }

      else
      {
        SK_LOG4 ( ( L"Unconfined Mouse Cursor" ),
                    L"Window Mgr" );

        SK_ClipCursor (nullptr);
      }
    }

    if (config.window.unconfine_cursor || (! game_window.active))
    {
      SK_LOG4 ( ( L"Unconfined Mouse Cursor" ),
                  L"Window Mgr" );

      SK_ClipCursor (nullptr);
    }

    //if (SK_IsGameWindowActive ())//hWnd == game_window.hWnd || IsChild (game_window.hWnd, hWnd))
    {
      if (config.window.always_on_top != -1)
      {
        SK_Window_SetTopMost (game_window.active &&
          config.window.always_on_top != PreventAlwaysOnTop,
                              game_window.active, hWnd);
      }

      SK_ImGui_Cursor.activateWindow (game_window.active);
    }
  }

  wm_dispatch->active_windows [hWnd] = active;
};


LRESULT
CALLBACK
SK_DetourWindowProc ( _In_  HWND   hWnd,
                      _In_  UINT   uMsg,
                      _In_  WPARAM wParam,
                      _In_  LPARAM lParam );

// Fix for Trails of Cold Steel
LRESULT
WINAPI
SK_COMPAT_SafeCallProc (sk_window_s* pWin, HWND hWnd_, UINT Msg, WPARAM wParam, LPARAM lParam);

sk_window_s       game_window = { };

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
      DWORD dwStyle   = 0,
            dwStyleEx = 0;

      switch (IsWindowUnicode (hWnd))
      {
        case TRUE:
          dwStyle   = SK_GetWindowLongW (hWnd, GWL_STYLE);
          dwStyleEx = SK_GetWindowLongW (hWnd, GWL_EXSTYLE);
          break;

        default:
        case FALSE:
          dwStyle   = SK_GetWindowLongA (hWnd, GWL_STYLE);
          dwStyleEx = SK_GetWindowLongA (hWnd, GWL_EXSTYLE);
          break;
      }

      bool    SKIM                   = false;
      wchar_t wszName [MAX_PATH + 2] = { };

      if (RealGetWindowClassW (hWnd, wszName, MAX_PATH))
      {
        if (StrStrIW (wszName, L"SKIM") != nullptr)
                                 SKIM    = true;
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
  window_t win = { };

  win.proc_id  = proc_id;
  win.root     = nullptr;

  EnumWindows (
    SK_EnumWindows,
      (LPARAM)&win
  );

  return win;
}

std::unique_ptr <SK_WindowManager>
                 SK_WindowManager::pWindowManager = nullptr;

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

  lpPoint->x -= ( game_window.actual.window.left );
  lpPoint->y -= ( game_window.actual.window.top  );

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
#endif

bool SK_ImGui_ImplicitMouseAntiwarp (void)
{
  extern bool SK_ImGui_IsMouseRelevant       (void);
  extern bool SK_InputUtil_IsHWCursorVisible (void);

  if (SK_ImGui_IsMouseRelevant ())
  {
    // Depending on warp prefs, we may not allow the game to know about mouse movement
    //   (even if ImGui doesn't want mouse capture)
    if ( ( SK_ImGui_Cursor.prefs.no_warp.ui_open                                      ) ||
         ( SK_ImGui_Cursor.prefs.no_warp.visible && SK_InputUtil_IsHWCursorVisible () )    )
    {
      return true;
    }
  }

  return false;
}

bool
SK_IsRectTooSmall (RECT* lpRect0, RECT* lpRect1)
{
  if ( (lpRect0->right  - lpRect0->left) <
       (lpRect1->right  - lpRect1->left) ||
       (lpRect0->bottom - lpRect0->top)  <
       (lpRect1->bottom - lpRect1->top) )
    return true;

  return false;
}

bool
SK_ExpandSmallClipCursor (RECT *lpRect)
{
  if (! (config.input.mouse.ignore_small_clips || (SK_ImGui_ImplicitMouseAntiwarp () && SK_ImGui_Active ())))
    return false;

  // Don't do this until we've initialized the window
  if (game_window.WndProc_Original == nullptr)
    return false;

  RECT client, window;

  SK_GetClientRect (SK_GetGameWindow (), &client);
  SK_GetWindowRect (SK_GetGameWindow (), &window);

  if (        (lpRect->right  - lpRect->left) <
      0.98f * ( window.right  -  window.left) ||
              (lpRect->bottom - lpRect->top)  <
      0.98f * ( window.bottom -  window.top)  )
  {
    // Report this if user _explicitly_ wants the feature, but
    //   don't report anything if implicitly unclipping while
    //     SK's control panel is open.
    if (  config.input.mouse.ignore_small_clips)
    { if (config.window.unconfine_cursor)
             SK_ClipCursor (nullptr);

      static bool          reported = false;
      if (! std::exchange (reported,  true ))
      {
        SK_LOG0 ( (L"Game is using an abnormally small mouse clipping rect, it"
                   L" would adversely affect UI support and has been ignored." ),
                   L"Input Mgr." );
      }
    }

    return true;
  }

  return false;
}

RECT  __SK_BackupClipRectStorage = {     };
RECT* __SK_BackedClipRect        = nullptr;

RECT
SK_Input_SaveClipRect (RECT *pSave)
{
  if (pSave != nullptr)
  {
    __SK_BackedClipRect   = &__SK_BackupClipRectStorage;
                             __SK_BackupClipRectStorage =
     *pSave;
  }

  else
  {
    if (GetClipCursor (     &__SK_BackupClipRectStorage))
      __SK_BackedClipRect = &__SK_BackupClipRectStorage;
    else
      __SK_BackedClipRect = nullptr;
  }

  return __SK_BackupClipRectStorage;
}

RECT
SK_Input_RestoreClipRect (void)
{
  if (               __SK_BackedClipRect != nullptr &&
      SK_ClipCursor (__SK_BackedClipRect))
                     __SK_BackedClipRect = nullptr;
  // No backing store, this means we should unrestrict the cursor
  else
      SK_ClipCursor (nullptr);

  return __SK_BackupClipRectStorage;
}

BOOL
WINAPI
ClipCursor_Detour (const RECT *lpRect)
{
  SK_LOG_FIRST_CALL

  SK_LOGi4 (L"ClipCursor (...) - Frame=%d", SK_GetFramesDrawn ());

  RECT _rect = { };

  if (lpRect != nullptr)
  {
    _rect = *lpRect;

    // Fix-Up Clip Rects that don't work correctly on multi-monitor systems
    //
    ////if (game_window.hWnd == 0)
    ////{
    ////  HMONITOR hMonitorCovered =
    ////    MonitorFromPoint (
    ////      POINT { lpRect->right,
    ////              lpRect->bottom -
    ////             (lpRect->bottom - lpRect->top) / 2
    ////            }, MONITOR_DEFAULTTONEAREST );
    ////
    ////  RECT rcMonitor = { };
    ////  MONITORINFO
    ////    mi           = { };
    ////    mi.cbSize    = sizeof (MONITORINFO);
    ////
    ////  if (GetMonitorInfo (hMonitorCovered, &mi))
    ////  {
    ////    if (_rect.left   < mi.rcWork.left)   _rect.left   = mi.rcWork.left;
    ////    if (_rect.right  > mi.rcWork.right)  _rect.right  = mi.rcWork.right;
    ////    if (_rect.top    < mi.rcWork.top)    _rect.top    = mi.rcWork.top;
    ////    if (_rect.bottom > mi.rcWork.bottom) _rect.bottom = mi.rcWork.bottom;
    ////
    ////    if (! EqualRect (lpRect, &_rect))
    ////    {
    ////      dll_log->Log (L"Clip Rect (ORIG.): {%-5li, %-5li, %-5li, %-5li}", lpRect->left, lpRect->right, lpRect->bottom, lpRect->top);
    ////      dll_log->Log (L"Clip Rect (FIXED): {%-5li, %-5li, %-5li, %-5li}",   _rect.left,   _rect.right,   _rect.bottom,   _rect.top);
    ////    }
    ////  }
    ////}
    ////
    ////else
    ////{
    ////  RECT                                  rcWindow = { };
    ////  if (GetWindowRect (game_window.hWnd, &rcWindow))
    ////  {
    ////    if (_rect.left   < rcWindow.left)   _rect.left   = rcWindow.left;
    ////    if (_rect.right  > rcWindow.right)  _rect.right  = rcWindow.right;
    ////    if (_rect.top    < rcWindow.top)    _rect.top    = rcWindow.top;
    ////    if (_rect.bottom > rcWindow.bottom) _rect.bottom = rcWindow.bottom;
    ////
    ////    if (! EqualRect (lpRect, &_rect))
    ////    {
    ////      dll_log->Log (L"Clip Rect (ORIG.): {%-5li, %-5li, %-5li, %-5li}", lpRect->left, lpRect->right, lpRect->bottom, lpRect->top);
    ////      dll_log->Log (L"Clip Rect (FIXED): {%-5li, %-5li, %-5li, %-5li}",   _rect.left,   _rect.right,   _rect.bottom,   _rect.top);
    ////    }
    ////  }
    ////}


    if (SK_ExpandSmallClipCursor ((RECT *)&_rect))
    {
      // Generally this only matters if background rendering is enabled;
      //   flat-out ignore cursor clip rects if the window's not even active.
      if (SK_IsGameWindowActive ())
      {
        return
          SK_ClipCursor ( config.window.unconfine_cursor ? nullptr
                                                         :
                                      &game_window.actual.window );
      }
    }

    game_window.cursor_clip = _rect;
  }

  else if (! config.window.confine_cursor)
  {
    // Save a nullptr clip rect as an infinite rect
    game_window.cursor_clip = { LONG_MIN, LONG_MIN,
                                LONG_MAX, LONG_MAX };

    return SK_ClipCursor (nullptr);
  }


  // Don't let the game unclip the cursor, but DO remember the
  //   coordinates that it wants.
  if ( config.window.confine_cursor &&
        &game_window.cursor_clip    != lpRect )
  {
    if (SK_IsGameWindowActive ())
    {
      if (lpRect != nullptr)
      {
        if (! SK_ImGui_Active ()) // Never narrow the clip rect while SK's UI is active
        {
          // If confining, and the game provides a rectangle small enough to satisfy confinement,
          //   then allow it to happen.
          if ( PtInRect (&game_window.actual.window, POINT { _rect.left,  _rect.top    }) &&
               PtInRect (&game_window.actual.window, POINT { _rect.right, _rect.bottom }) )
          {
            return
              SK_ClipCursor (&_rect);
          }
        }
      }

      return
        SK_ClipCursor (&game_window.actual.window);
    }
  }


  //
  // If the game uses mouse clipping and we are running in borderless,
  //   we need to re-adjust the window coordinates.
  //
  if (game_window.needsCoordTransform ())
  {
    POINT top_left     = { std::numeric_limits <LONG>::max (),
                           std::numeric_limits <LONG>::max () };
    POINT bottom_right = { std::numeric_limits <LONG>::min (),
                           std::numeric_limits <LONG>::min () };

    top_left.y     = game_window.actual.window.top;
    top_left.x     = game_window.actual.window.left;

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
  {
    return
      SK_ClipCursor (nullptr);
  }

  if ( SK_IsGameWindowActive () )
  {
    if (! (config.window.confine_cursor || config.window.unconfine_cursor))
    { // Expand clip rects while SK's UI is open so the mouse works as expected :)
      if ( SK_ImGui_Active () )
      {
        SK_LOG1 ( ( L"Ignoring Clip Rectangle Set While SK's UI is Active" ),
                    L"Input Mgr." );

        SK_Input_SaveClipRect (&game_window.cursor_clip  );
        SK_ClipCursor         (&game_window.actual.window);

        return TRUE;
      }
    }

    return
      SK_ClipCursor (&game_window.cursor_clip);
  }

  return
    SK_ClipCursor (nullptr);
}

void
SK_CenterWindowAtMouse (BOOL remember_pos)
{
  // This is too much trouble to bother with...
  if ( config.window.center || ( config.window.borderless &&
                                 config.window.fullscreen ) )
  {
    return;
  }

  static volatile LONG               __busy = FALSE;
  if (! InterlockedCompareExchange (&__busy, TRUE, FALSE))
  {
    SK_Thread_Create (
    [](LPVOID user) ->
    DWORD
    {
      SetCurrentThreadDescription (L"[SK] Window Center Thread");

      BOOL  remember_pos = (user != nullptr);
      POINT mouse        = { 0, 0 };

      SK_GetCursorPos (&mouse);

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

      config.window.offset.x.absolute -=
        (        game_window.actual.window.right  -
                 game_window.actual.window.left ) / 2;
      config.window.offset.y.absolute -=
        (        game_window.actual.window.bottom -
                 game_window.actual.window.top  ) / 2;

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

      InterlockedExchange (&__busy, FALSE);

      SK_Thread_CloseSelf ();

      return 0;
      // Don't dereference this, it's actually a boolean
    }, reinterpret_cast <LPVOID>   (
            static_cast <uintptr_t> (remember_pos)
                                   ) );
  }
}

BOOL
WINAPI
SK_MoveWindow (
  _In_ HWND hWnd,
  _In_ int  X,
  _In_ int  Y,
  _In_ int  nWidth,
  _In_ int  nHeight,
  _In_ BOOL bRedraw )
{
  if (MoveWindow_Original != nullptr)
    return MoveWindow_Original (hWnd, X, Y, nWidth, nHeight, bRedraw);

  return
    MoveWindow (hWnd, X, Y, nWidth, nHeight, bRedraw);
}

BOOL
WINAPI
MoveWindow_Detour (
  _In_ HWND hWnd,
  _In_ int  X,
  _In_ int  Y,
  _In_ int  nWidth,
  _In_ int  nHeight,
  _In_ BOOL bRedraw )
{
  SK_LOG_FIRST_CALL

  return
    SK_MoveWindow ( hWnd,
                      X, Y,
                        nWidth, nHeight,
                          bRedraw );
}

BOOL
WINAPI
SK_SetWindowPlacement (
  _In_       HWND             hWnd,
  _In_ const WINDOWPLACEMENT *lpwndpl)
{
  if (SetWindowPlacement_Original != nullptr)
    return SetWindowPlacement_Original (hWnd, lpwndpl);

  return
    SetWindowPlacement (hWnd, lpwndpl);
}

BOOL
WINAPI
SetWindowPlacement_Detour(
  _In_       HWND             hWnd,
  _In_ const WINDOWPLACEMENT *lpwndpl)
{
  SK_WINDOW_LOG_CALL_UNTESTED ();

  return
    SK_SetWindowPlacement ( hWnd, lpwndpl );
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
  {
    if (hWndInsertAfter == HWND_TOPMOST)
        hWndInsertAfter  = HWND_TOP;
  }

  ////uFlags |= SWP_ASYNCWINDOWPOS;

  BOOL bRet =
    SK_SetWindowPos ( hWnd,
                      hWndInsertAfter,
                         X,   Y,
                         cx, cy,
                                 uFlags );

  return bRet;
}

BOOL
WINAPI
SK_GetWindowRect (HWND hWnd, LPRECT rect)
{
  if (GetWindowRect_Original != nullptr)
    return GetWindowRect_Original (hWnd, rect);

  return
    GetWindowRect (hWnd, rect);
}

BOOL
WINAPI
SK_GetClientRect (HWND hWnd, LPRECT rect)
{
  if (GetClientRect_Original != nullptr)
    return GetClientRect_Original (hWnd, rect);

  return
    GetClientRect (hWnd, rect);
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
      (hWnd == game_window.hWnd /*|| IsChild (hWnd, game_window.hWnd)*/)
                                && config.window.fullscreen )
  {
    //lpRect->left = 0; lpRect->right  = config.window.res.override.x;
    //lpRect->top  = 0; lpRect->bottom = config.window.res.override.y;

    HMONITOR hMonitor = config.display.monitor_handle != 0 ?
                        config.display.monitor_handle      :
      MonitorFromWindow ( game_window.hWnd,
                            config.display.monitor_default );

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

  else
  {
    return
      SK_GetClientRect (hWnd, lpRect);
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
      (hWnd == game_window.hWnd /*|| IsChild (hWnd, game_window.hWnd)*/)
                                && config.window.fullscreen )
  {
    //lpRect->left = 0; lpRect->right  = config.window.res.override.x;
    //lpRect->top  = 0; lpRect->bottom = config.window.res.override.y;

    HMONITOR hMonitor = config.display.monitor_handle != 0 ?
                        config.display.monitor_handle      :
      MonitorFromWindow ( game_window.hWnd,
                            config.display.monitor_default );

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

  else
  {
    return
      SK_GetWindowRect (hWnd, lpRect);
  }
}

bool
WINAPI
SK_IsRectZero (_In_ const RECT *lpRect)
{
  return ( lpRect->left   == lpRect->right &&
           lpRect->bottom == lpRect->top );
}

bool
WINAPI
SK_IsRectInfinite (_In_ const tagRECT *lpRect)
{
  return ( lpRect->left   == LONG_MIN && lpRect->top   == LONG_MIN &&
           lpRect->bottom == LONG_MAX && lpRect->right == LONG_MAX );
}

bool
WINAPI
SK_IsRectFullscreen (_In_ const tagRECT *lpRect)
{
  const int virtual_x = SK_GetSystemMetrics (SM_CXVIRTUALSCREEN);
  const int virtual_y = SK_GetSystemMetrics (SM_CYVIRTUALSCREEN);

  return ( lpRect->left  == 0         && lpRect->top    == 0 &&
           lpRect->right == virtual_x && lpRect->bottom == virtual_y );
}

bool
WINAPI
SK_IsClipRectFinite (_In_ const tagRECT *lpRect)
{
  return (! ( SK_IsRectZero       (lpRect) ||
              SK_IsRectInfinite   (lpRect) ||
              SK_IsRectFullscreen (lpRect) ) );
}

BOOL
WINAPI
SK_AdjustWindowRect (
  _Inout_ LPRECT lpRect,
  _In_    DWORD  dwStyle,
  _In_    BOOL   bMenu )
{
  if (AdjustWindowRect_Original != nullptr)
    return AdjustWindowRect_Original (lpRect, dwStyle, bMenu);

  return
    AdjustWindowRect (lpRect, dwStyle, bMenu);
}

BOOL
WINAPI
AdjustWindowRect_Detour (
  _Inout_ LPRECT lpRect,
  _In_    DWORD  dwStyle,
  _In_    BOOL   bMenu  )
{
  SK_LOG1 ( ( L"AdjustWindowRect ( "
              L"{%4li,%4li / %4li,%4li}, 0x%04X, %li ) - %s",
              lpRect->left,  lpRect->top,
              lpRect->right, lpRect->bottom,
                dwStyle,
                bMenu,
                  SK_SummarizeCaller ().c_str () ),
              L"Window Mgr" );

  // Override if forcing Fullscreen Borderless
  //
  if (config.window.borderless)
  {
    // Application window
    if (dwStyle & WS_CAPTION)
    {
      dwStyle &= ~WS_BORDER;
      dwStyle &= ~WS_THICKFRAME;
      dwStyle &= ~WS_DLGFRAME;


      //dwStyle &= ~WS_GROUP;
      //dwStyle &= ~WS_SYSMENU;
    }

    if (config.window.fullscreen && (! bMenu) && (IsRectEmpty (lpRect)))
    {
      HMONITOR hMonitor = config.display.monitor_handle != 0 ?
                          config.display.monitor_handle      :
        MonitorFromWindow ( game_window.hWnd,
                              config.display.monitor_default );//MONITOR_DEFAULTTONEAREST );

      MONITORINFO mi   = {         };
      mi.cbSize        = sizeof (mi);
      GetMonitorInfo (hMonitor, &mi);

      lpRect->left   = mi.rcMonitor.left;
      lpRect->right  = mi.rcMonitor.right;

      lpRect->top    = mi.rcMonitor.top;
      lpRect->bottom = mi.rcMonitor.bottom;

      return
        TRUE;
    }
  }

  return
    SK_AdjustWindowRect (lpRect, dwStyle, bMenu);
}

void SK_SetWindowStyle   (DWORD_PTR dwStyle_ptr,   SetWindowLongPtr_pfn pDispatchFunc = nullptr);
void SK_SetWindowStyleEx (DWORD_PTR dwStyleEx_ptr, SetWindowLongPtr_pfn pDispatchFunc = nullptr);

DWORD dwBorderStyle,
      dwBorderStyleEx;

void
SK_Window_RemoveBorders (void)
{
  if (SK_Window_HasBorder (game_window.hWnd))
  {
    dwBorderStyle   = static_cast <DWORD> (game_window.actual.style);
    dwBorderStyleEx = static_cast <DWORD> (game_window.actual.style_ex);

    SK_SetWindowStyle   ( SK_BORDERLESS    );
    SK_SetWindowStyleEx ( SK_BORDERLESS_EX );

    SK_SetWindowPos ( game_window.hWnd,
                               SK_HWND_TOP,
                        0, 0,
                        0, 0,  SWP_NOZORDER     | SWP_NOREPOSITION | SWP_NOSIZE |
                               SWP_FRAMECHANGED | SWP_NOACTIVATE   | SWP_ASYNCWINDOWPOS );
  }
}

void
SK_Window_RestoreBorders (DWORD dwStyle, DWORD dwStyleEx)
{
  if (! SK_Window_HasBorder (game_window.hWnd))
  {
    if (! config.window.borderless)
    {
      SK_SetWindowStyle   ( dwStyle   == 0 ? dwBorderStyle   : dwStyle   );
      SK_SetWindowStyleEx ( dwStyleEx == 0 ? dwBorderStyleEx : dwStyleEx );

      SK_SetWindowPos ( game_window.hWnd,
                                 SK_HWND_TOP,
                          0, 0,
                          0, 0,  SWP_NOZORDER     | SWP_NOREPOSITION | SWP_NOSIZE |
                                 SWP_FRAMECHANGED | SWP_NOACTIVATE   | SWP_ASYNCWINDOWPOS );
    }
  }
}

BOOL
WINAPI
SK_AdjustWindowRectEx (
  _Inout_ LPRECT lpRect,
  _In_    DWORD  dwStyle,
  _In_    BOOL   bMenu,
  _In_    DWORD  dwExStyle )
{
  if (AdjustWindowRectEx_Original != nullptr)
    return AdjustWindowRectEx_Original (lpRect, dwStyle, bMenu, dwExStyle);

  return
    AdjustWindowRectEx (lpRect, dwStyle, bMenu, dwExStyle);
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
              L"{%4li,%4li / %4li,%4li}, 0x%04X, %li, 0x%04X ) - %s",
           lpRect->left,  lpRect->top,
           lpRect->right, lpRect->bottom,
           dwStyle, bMenu,
           dwExStyle,
           SK_SummarizeCaller ().c_str () ),
           L"Window Mgr" );

  // Override if forcing Fullscreen Borderless
  //
  if (config.window.borderless)
  {
    // Application window
    if (dwStyle & WS_CAPTION)
    {
      dwStyle &= ~WS_BORDER;
      dwStyle &= ~WS_THICKFRAME;
      dwStyle &= ~WS_DLGFRAME;

      //dwStyle &= ~WS_GROUP;
      //dwStyle &= ~WS_SYSMENU;
    }

    if (config.window.fullscreen && (! bMenu) && (IsRectEmpty (lpRect) && (! (dwExStyle & 0x20000000))))
    {
      HMONITOR hMonitor = config.display.monitor_handle != 0 ?
                          config.display.monitor_handle      :
        MonitorFromWindow ( game_window.hWnd,
                              config.display.monitor_default );//MONITOR_DEFAULTTONEAREST );

      MONITORINFO mi   = {         };
      mi.cbSize        = sizeof (mi);
      GetMonitorInfo (hMonitor, &mi);

      lpRect->left   = mi.rcMonitor.left;
      lpRect->right  = mi.rcMonitor.right;

      lpRect->top    = mi.rcMonitor.top;
      lpRect->bottom = mi.rcMonitor.bottom;

      return
        TRUE;
    }
  }

  return
    SK_AdjustWindowRectEx (lpRect, dwStyle, bMenu, dwExStyle);
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
  if (pOrigFunc == nullptr) return 0;

  // Override window styles
  if (hWnd == game_window.hWnd)
  {
    switch (nIndex)
    {
      case GWL_STYLE:
      {
        game_window.game.style =
          static_cast <ULONG_PTR> (dwNewLong);

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
              game_window.game.style          )
           )
        {
          game_window.border_style =
            game_window.game.style;
        }

        SK_SetWindowStyle ( game_window.actual.style,
         reinterpret_cast < SetWindowLongPtr_pfn >
                              ( pOrigFunc )
                          );

        return
          static_cast <LONG> (game_window.actual.style);
      }

      case GWL_EXSTYLE:
      {
        game_window.game.style_ex =
          static_cast <ULONG_PTR> (dwNewLong);

        if (config.window.borderless)
        {
          game_window.actual.style_ex =
            SK_BORDERLESS_EX;
        }

        else
        {
          game_window.actual.style_ex =
            game_window.game.style_ex;
        }

        if ( SK_WindowManager::StyleHasBorder (
               game_window.actual.style
           )
          )
        {
          game_window.border_style_ex =
            game_window.game.style_ex;
        }

        SK_SetWindowStyleEx (                                          game_window.actual.style_ex,
                              reinterpret_cast <SetWindowLongPtr_pfn> (pOrigFunc) );

        return
          sk::narrow_cast <ULONG> (
            game_window.actual.style_ex
          );
      }

      case GWLP_WNDPROC:
      {
        if (game_window.hooked)
        {
          //game_window.WndProc_Original = reinterpret_cast <WNDPROC> (dwNewLong);
          //return dwNewLong;
        }
      } break;

      default:
        break;
    }
  }

  return
    pOrigFunc (hWnd, nIndex, dwNewLong);
}

LONG
WINAPI
SK_SetWindowLongA (
  _In_ HWND hWnd,
  _In_ int  nIndex,
  _In_ LONG dwNewLong )
{
  return
    static_cast <LONG> (
      SK_SetWindowLongPtrA (hWnd, nIndex, dwNewLong)
    );
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

  return
    SetWindowLong_Marshall (
      SK_SetWindowLongA,
        hWnd,
          nIndex,
            dwNewLong
    );
}

LONG
WINAPI
SK_SetWindowLongW (
  _In_ HWND hWnd,
  _In_ int  nIndex,
  _In_ LONG dwNewLong )
{
  return
    static_cast <LONG> (
      SK_SetWindowLongPtrW (hWnd, nIndex, dwNewLong)
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

  return
    SetWindowLong_Marshall (
      SK_SetWindowLongW,
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
  if (hWnd == game_window.hWnd /*|| IsChild (hWnd, game_window.hWnd)*/)
  {
    switch (nIndex)
    {
      case GWL_STYLE:
        return static_cast <LONG> (game_window.game.style);
      case GWL_EXSTYLE:
        return static_cast <LONG> (game_window.game.style_ex);
    }
  }


  if (pOrigFunc != nullptr)
  {
    return
      pOrigFunc (hWnd, nIndex);
  }

  return 0;
}

LONG
WINAPI
SK_GetWindowLongA (
  _In_ HWND hWnd,
  _In_ int  nIndex)
{
  if (GetWindowLongA_Original != nullptr)
    return GetWindowLongA_Original (hWnd, nIndex);

  return
    GetWindowLongA (hWnd, nIndex);
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
        SK_GetWindowLongA,
          hWnd,
            nIndex
    );
}

LONG
WINAPI
SK_GetWindowLongW (
  _In_ HWND hWnd,
  _In_ int  nIndex)
{
  if (GetWindowLongW_Original != nullptr)
    return GetWindowLongW_Original (hWnd, nIndex);

  return
    GetWindowLongW (hWnd, nIndex);
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
      SK_GetWindowLongW,
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
  if (pOrigFunc == nullptr) return 0;

  //dll_log->Log (L"SetWindowLongPtr: Idx=%li, Val=%X -- Caller: %s", nIndex, dwNewLong, SK_GetCallerName ().c_str ());

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

        SK_SetWindowStyle ( game_window.actual.style,
                              pOrigFunc );

        return
          game_window.actual.style;
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
            game_window.game.style_ex;
        }

        if ( SK_WindowManager::StyleHasBorder (
               game_window.actual.style
             )
           )
        {
          game_window.border_style_ex =
            game_window.game.style_ex;
        }

        SK_SetWindowStyleEx (                                          game_window.actual.style_ex,
                              reinterpret_cast <SetWindowLongPtr_pfn> (pOrigFunc) );

        return
          sk::narrow_cast <ULONG> (
            game_window.actual.style_ex
          );
      }

      case GWLP_WNDPROC:
      {
        if (game_window.hooked)
        {
          //game_window.WndProc_Original = reinterpret_cast <WNDPROC> (dwNewLong);
          //return dwNewLong;
        }
      } break;

      default:
        break;
    }
  }

  return
    pOrigFunc (hWnd, nIndex, dwNewLong);
}

struct args_s {
  HWND     hWnd;
  int      nIndex;
  LONG_PTR dwNewLong;
};

struct dispatch_queue_s {
  concurrency::concurrent_queue <args_s>
         queued;
  HANDLE hSignal;
};

SK_LazyGlobal <dispatch_queue_s> _SK_SetWindowLongPtrA_work;
SK_LazyGlobal <dispatch_queue_s> _SK_SetWindowLongPtrW_work;

void
SK_Window_WaitForAsyncSetWindowLong (void)
{
  int sleepy_spins = 0;
  int alert_spins  = 0;

  while (! ( _SK_SetWindowLongPtrW_work->queued.empty () &&
             _SK_SetWindowLongPtrA_work->queued.empty () ))
  {
    if ( ++alert_spins  < 3 )
      continue;

    if ( ++sleepy_spins > 1 )
      return;

    SK_SleepEx (0, FALSE);
  }
}

LONG_PTR
WINAPI
SK_SetWindowLongPtrA (
  _In_ HWND     hWnd,
  _In_ int      nIndex,
  _In_ LONG_PTR dwNewLong )
{
  if (nIndex == GWLP_WNDPROC)
  {
    if (SetWindowLongPtrA_Original != nullptr)
      return SetWindowLongPtrA_Original (hWnd, nIndex, dwNewLong);
    else
      return SetWindowLongPtrA          (hWnd, nIndex, dwNewLong);
  }

        DWORD dwPid;
  const DWORD dwOrigThreadId =
    GetWindowThreadProcessId (hWnd, &dwPid);

  if (dwOrigThreadId == SK_GetCurrentThreadId ()/* || nIndex == GWL_EXSTYLE*/)
  {
    if (SetWindowLongPtrA_Original != nullptr)
      return SetWindowLongPtrA_Original (hWnd, nIndex, dwNewLong);
    else
      return SetWindowLongPtrA          (hWnd, nIndex, dwNewLong);
  }


  SK_RunOnce (_SK_SetWindowLongPtrA_work->hSignal =
                 SK_CreateEvent (nullptr, FALSE, FALSE, nullptr) );

  _SK_SetWindowLongPtrA_work->queued.push (
    args_s { hWnd, nIndex, dwNewLong }
  );

  SK_RunOnce (
    SK_Thread_CreateEx ([](LPVOID lpUser) ->
    DWORD
    {
      SK_Thread_SetCurrentPriority (THREAD_PRIORITY_HIGHEST);

      auto *dispatch =
        (dispatch_queue_s *)lpUser;

      HANDLE hSignals [] = {
        __SK_DLL_TeardownEvent,
             dispatch->hSignal
      };

      DWORD dwWait = 0;

      while ( (dwWait = WaitForMultipleObjects (
                          2, hSignals, FALSE, INFINITE )
              ) != WAIT_OBJECT_0 )
      {
        args_s args;

        while (! dispatch->queued.empty ())
        {
          if (dispatch->queued.try_pop (args))
          {
            if (SetWindowLongPtrA_Original != nullptr)
                SetWindowLongPtrA_Original (args.hWnd, args.nIndex, args.dwNewLong);
            else
                SetWindowLongPtrA          (args.hWnd, args.nIndex, args.dwNewLong);
          }
        }
      }

      SK_CloseHandle (dispatch->hSignal);

      SK_Thread_CloseSelf ();

      return 0;
    }, L"[SK] SetWindowLongPtrA Async Dispatch", (LPVOID)_SK_SetWindowLongPtrA_work.getPtr () )
  );

  SetEvent (_SK_SetWindowLongPtrA_work->hSignal);

  SK_Window_WaitForAsyncSetWindowLong ();

  return
    dwNewLong;
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
    SK_SetWindowLongPtrA,
      hWnd,
        nIndex,
          dwNewLong
  );
}

LONG_PTR
WINAPI
SK_SetWindowLongPtrW (
  _In_ HWND     hWnd,
  _In_ int      nIndex,
  _In_ LONG_PTR dwNewLong )
{
  if (nIndex == GWLP_WNDPROC)
  {
    if (SetWindowLongPtrW_Original != nullptr)
      return SetWindowLongPtrW_Original (hWnd, nIndex, dwNewLong);
    else
      return SetWindowLongPtrW          (hWnd, nIndex, dwNewLong);
  }


        DWORD dwPid;
  const DWORD dwOrigThreadId =
    GetWindowThreadProcessId (hWnd, &dwPid);

  if (dwOrigThreadId == SK_GetCurrentThreadId ()/*|| nIndex == GWL_EXSTYLE*/)
  {
    if (SetWindowLongPtrW_Original != nullptr)
      return SetWindowLongPtrW_Original (hWnd, nIndex, dwNewLong);
    else
      return SetWindowLongPtrW          (hWnd, nIndex, dwNewLong);
  }

  SK_RunOnce ( _SK_SetWindowLongPtrW_work->hSignal =
                 SK_CreateEvent (nullptr, FALSE, FALSE, nullptr) );

  _SK_SetWindowLongPtrW_work->queued.push (
    args_s { hWnd, nIndex, dwNewLong }
  );

  SK_RunOnce (
    SK_Thread_CreateEx ([](LPVOID lpUser) ->
    DWORD
    {
      SK_Thread_SetCurrentPriority (THREAD_PRIORITY_HIGHEST);

      auto* dispatch =
        (dispatch_queue_s *)lpUser;

      HANDLE hSignals [] = {
        __SK_DLL_TeardownEvent,
             dispatch->hSignal
      };

      DWORD dwWait = 0;

      while ( (dwWait = WaitForMultipleObjects (
                          2, hSignals, FALSE, INFINITE )
              ) != WAIT_OBJECT_0 )
      {
        args_s args;

        while (! dispatch->queued.empty ())
        {
          if (dispatch->queued.try_pop (args))
          {
            if (SetWindowLongPtrW_Original != nullptr)
                SetWindowLongPtrW_Original (args.hWnd, args.nIndex, args.dwNewLong);
            else
                SetWindowLongPtrW          (args.hWnd, args.nIndex, args.dwNewLong);
          }
        }
      }

      SK_CloseHandle (dispatch->hSignal);

      SK_Thread_CloseSelf ();

      return 0;
    }, L"[SK] SetWindowLongPtrW Async Dispatch", (LPVOID)_SK_SetWindowLongPtrW_work.getPtr () )
  );

  SetEvent (_SK_SetWindowLongPtrW_work->hSignal);

  SK_Window_WaitForAsyncSetWindowLong ();

  return
    dwNewLong;
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
    SK_SetWindowLongPtrW,
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

  if (pOrigFunc != nullptr)
    return
      pOrigFunc (hWnd, nIndex);
  else
    return -1;
}

LONG_PTR
WINAPI
SK_GetWindowLongPtrA (
  _In_ HWND     hWnd,
  _In_ int      nIndex   )
{
  if (GetWindowLongPtrA_Original != nullptr)
    return GetWindowLongPtrA_Original (hWnd, nIndex);

  return GetWindowLongPtrA (hWnd, nIndex);
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
      SK_GetWindowLongPtrA,
           hWnd,
             nIndex
  );
}

LONG_PTR
WINAPI
SK_GetWindowLongPtrW   (
  _In_ HWND     hWnd,
  _In_ int      nIndex )
{
  if (GetWindowLongPtrW_Original != nullptr)
    return GetWindowLongPtrW_Original (hWnd, nIndex);

  return GetWindowLongPtrW (hWnd, nIndex);
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
      SK_GetWindowLongPtrW,
           hWnd,
             nIndex
  );
}

static constexpr DWORD SK_ALL_STYLES = 0xFFFFFFFFUL;
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
  DWORD_PTR dwStyle =
           (dwStyle_ptr & 0xFFFFFFFF);

  // Minimal sane set of extended window styles for sane rendering
  dwStyle |=  ( WS_VISIBLE  | WS_SYSMENU );
  dwStyle &= ~( WS_DISABLED | WS_ICONIC  | WS_CHILD );

  if (config.window.borderless)
    dwStyle &= ~( WS_GROUP | WS_SYSMENU );


  game_window.actual.style = dwStyle;

  if (pDispatchFunc == nullptr)
    pDispatchFunc = game_window.SetWindowLongPtr;


  pDispatchFunc ( game_window.hWnd,
                    GWL_STYLE,
                      game_window.actual.style );
}

void
SK_SetWindowStyleEx ( DWORD_PTR            dwStyleEx_ptr,
                      SetWindowLongPtr_pfn pDispatchFunc )
{
  // Clear the high-bits
  DWORD_PTR dwStyleEx =
           (dwStyleEx_ptr & 0xFFFFFFFF);

  // Minimal sane set of extended window styles for sane rendering
  dwStyleEx |=   WS_EX_APPWINDOW;
  dwStyleEx &= ~(WS_EX_NOACTIVATE | WS_EX_TRANSPARENT | WS_EX_LAYOUTRTL |
                 WS_EX_RIGHT      | WS_EX_RTLREADING  | WS_EX_TOOLWINDOW);

  game_window.actual.style_ex = (DWORD_PTR)dwStyleEx;

  if (pDispatchFunc == nullptr)
      pDispatchFunc = game_window.SetWindowLongPtr;


  pDispatchFunc ( game_window.hWnd,
                    GWL_EXSTYLE,
                      game_window.actual.style_ex );
}

RECT
SK_ComputeClientSize (void)
{
  const bool use_override =
    (! config.window.res.override.isZero ());

  if (use_override)
  {
    return RECT { 0L, 0L,
                    sk::narrow_cast <LONG> (config.window.res.override.x),
                    sk::narrow_cast <LONG> (config.window.res.override.y)
    };
  }

  RECT ret =
    game_window.actual.client;

  SK_GetClientRect ( game_window.hWnd, &ret );

  ret = { 0, 0,
          ret.right  - ret.left,
          ret.bottom - ret.top };

  return ret;
}

POINT
SK_ComputeClientOrigin (void)
{
  RECT window = { },
       client = { };

  SK_GetWindowRect ( game_window.hWnd, &window );
  SK_GetClientRect ( game_window.hWnd, &client );

  return
    POINT { window.left + client.left,
            window.top  + client.top };
}

bool
SK_IsRectTooBigForDesktop (const RECT& wndRect)
{
  HMONITOR hMonitor = config.display.monitor_handle != 0 ?
                      config.display.monitor_handle      :
    MonitorFromWindow ( game_window.hWnd,
                          config.display.monitor_default );//MONITOR_DEFAULTTONEAREST );

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

  const int desktop_width  = mi.rcWork.right     - mi.rcWork.left;
  const int desktop_height = mi.rcWork.bottom    - mi.rcWork.top;

  const int mon_width      = mi.rcMonitor.right  - mi.rcMonitor.left;
  const int mon_height     = mi.rcMonitor.bottom - mi.rcMonitor.top;

  if (win_width == mon_width && win_height == mon_height)
    return true;

  if (win_width > desktop_width && win_height > desktop_height)
    return true;

  return false;
}

bool
SK_Window_HasBorder (HWND hWnd)
{
  return
    SK_WindowManager::StyleHasBorder (
      (DWORD_PTR)SK_GetWindowLongW ( hWnd, GWL_STYLE )
    );
}

bool
SK_Window_IsFullscreen (HWND hWnd)
{
  HMONITOR hMonitor = config.display.monitor_handle != 0 ?
                      config.display.monitor_handle      :
    MonitorFromWindow ( hWnd,
                          config.display.monitor_default );//MONITOR_DEFAULTTONEAREST );

  RECT                     rc_window = { };
  SK_GetWindowRect (hWnd, &rc_window);

  auto ResolutionMatches = [](RECT& rect, HMONITOR hMonitor) ->
  bool
  {
    MONITORINFO mon_info        = { };
                mon_info.cbSize = sizeof (MONITORINFO);

    GetMonitorInfoW (
      hMonitor,
        &mon_info
    );

    return
       ( rect.left   == mon_info.rcMonitor.left   &&
         rect.right  == mon_info.rcMonitor.right  &&
         rect.top    == mon_info.rcMonitor.top    &&
         rect.bottom == mon_info.rcMonitor.bottom    );
  };

  return
    ResolutionMatches (rc_window, hMonitor);
}

void
SK_AdjustBorder (void)
{
  SK_WINDOW_LOG_CALL3 ();

  if (game_window.GetWindowLongPtr == nullptr)
    return;

  if (SK_GetCurrentRenderBackend ().fullscreen_exclusive)
    return;

  // Make sure any pending changes are finished before querying the
  //   actual value
  SK_Window_WaitForAsyncSetWindowLong ();

  game_window.actual.style    =
    game_window.GetWindowLongPtr ( game_window.hWnd, GWL_STYLE   );
  game_window.actual.style_ex =
    game_window.GetWindowLongPtr ( game_window.hWnd, GWL_EXSTYLE );

  const bool has_border =
    SK_WindowManager::StyleHasBorder (
      game_window.actual.style
    );

  // If these are opposite, we can skip a whole bunch of
  //   pointless work!
  if ((! game_window.attach_border) && has_border != config.window.borderless)
    return;

  if (config.window.borderless)
  {
    game_window.actual.style    =
      (ULONG_PTR)SK_BORDERLESS;
    game_window.actual.style_ex =
      (ULONG_PTR)SK_BORDERLESS_EX;
  }

  if (game_window.attach_border)
  {
    game_window.actual.style    =
      (ULONG_PTR)game_window.border_style;
    game_window.actual.style_ex =
      (ULONG_PTR)game_window.border_style_ex;

    game_window.attach_border = false;
  }

  RECT                                 orig_client = { };
  SK_GetClientRect (game_window.hWnd, &orig_client);

  const RECT new_client =
    SK_ComputeClientSize ();

  RECT  new_window = new_client;
  POINT origin     { new_client.left, new_client.top };//SK_ComputeClientOrigin ();

  ClientToScreen (game_window.hWnd, &origin);

  SK_SetWindowStyle   ( game_window.actual.style    );
  SK_SetWindowStyleEx ( game_window.actual.style_ex );

  if ( SK_AdjustWindowRectEx ( &new_window,
         static_cast <DWORD>  (game_window.actual.style),
         FALSE,
         static_cast <DWORD>  (game_window.actual.style_ex) )
     )
  {
    const bool had_border = has_border;

    const int origin_x = had_border ? origin.x + orig_client.left : origin.x;
    const int origin_y = had_border ? origin.y + orig_client.top  : origin.y;

    SK_SetWindowPos ( game_window.hWnd,
                      SK_HWND_TOP,
                      origin_x, origin_y,
                      new_window.right  - new_window.left,
                      new_window.bottom - new_window.top,
                      SWP_NOZORDER     |   SWP_NOREPOSITION   |
                    /*SWP_FRAMECHANGED |*/ SWP_NOSENDCHANGING | SWP_NOACTIVATE );

    ShowWindow (game_window.hWnd, SW_SHOWNA);
  }

  GetWindowRect (game_window.hWnd, &game_window.actual.window);
  GetClientRect (game_window.hWnd, &game_window.actual.client);

  CopyRect (&game_window.game.window, &game_window.actual.window);
  CopyRect (&game_window.game.client, &game_window.actual.client);
}


void
SK_Window_RepositionIfNeeded (void)
{
  static SK_AutoHandle hRepoSignal (
    SK_CreateEvent (nullptr, TRUE, FALSE, nullptr)
  );

  SetEvent (hRepoSignal);

  SK_LOG4 ( ( L"Reposition" ), L"Window Mgr" );

  SK_RunOnce (
    SK_Thread_CreateEx (
      [](LPVOID user) ->
      DWORD
    {
      HANDLE hSignals [] = {
        __SK_DLL_TeardownEvent, (HANDLE)user
      };

      // We're not fully initialized yet
      while (game_window.GetWindowLongPtr == nullptr)
        SK_Sleep (1);

      if (IsWindow (game_window.hWnd))
      {
        // Make sure any pending changes are finished before querying the
        //   actual value
        SK_Window_WaitForAsyncSetWindowLong ();

        ////// Force a sane set of window styles initially
        SK_RunOnce (
        {
          SK_SetWindowStyle (
            SK_GetWindowLongPtrW (game_window.hWnd, GWL_STYLE)
          );
          SK_SetWindowStyleEx (
            SK_GetWindowLongPtrW (game_window.hWnd, GWL_EXSTYLE)
          );
        })
      }

      static constexpr DWORD _WorkSignal = WAIT_OBJECT_0 + 1;
      static constexpr DWORD _EndSignal  = WAIT_OBJECT_0;

      DWORD   dwWaitState =  _WorkSignal;
      while ( dwWaitState != _EndSignal )
      {
        static ULONG64 ullLastFrame =
          SK_GetFramesDrawn ();

        dwWaitState =
          WaitForMultipleObjects (2, hSignals, FALSE, INFINITE);

        while (ullLastFrame == SK_GetFramesDrawn ())
        {
          if (WaitForSingleObject (hSignals [0], 3UL) == _EndSignal)
          {
            dwWaitState = _EndSignal;
            break;
          }
        }

        if (dwWaitState == WAIT_OBJECT_0 + 1)
        {
          static auto& rb =
            SK_GetCurrentRenderBackend ();

          ResetEvent (hSignals [1]);

          static bool lastBorderless  = config.window.borderless;
          static bool lastCenter      = config.window.center;
          static bool lastFullscreen  = config.window.fullscreen;
          static auto lastOffset      = config.window.offset;
          static auto lastResOverride = config.window.res;

          SK_GetWindowRect (game_window.hWnd, &game_window.actual.window);
          SK_GetClientRect (game_window.hWnd, &game_window.actual.client);

          static RECT  rcClientOrig, rcClientLast = { };
          static RECT  rcWindowOrig, rcWindowLast = { };
          CopyRect   (&rcClientOrig, &game_window.actual.client);
          CopyRect   (&rcWindowOrig, &game_window.actual.window);

          static HMONITOR hMonitorOrig = 0;

          bool _configFullscreen = config.window.fullscreen,
               _configCenter     = config.window.center,
               _overrideRes      = false;

          SK_RunOnce (rb.updateOutputTopology ());

          if (! rb.fullscreen_exclusive)
          {
          if ( config.window.center     ||
               config.window.borderless ||

               (rb.monitor != rb.next_monitor &&
                              rb.next_monitor != 0) ||

               lastFullscreen             != config.window.fullscreen        ||
               lastBorderless             != config.window.borderless        ||
               lastCenter                 != config.window.center            ||
               lastOffset.x.absolute      != config.window.offset.x.absolute ||
               lastOffset.y.absolute      != config.window.offset.y.absolute ||
               lastOffset.x.percent       != config.window.offset.x.percent  ||
               lastOffset.y.percent       != config.window.offset.y.percent  ||
               lastResOverride.override.x != config.window.res.override.x    ||
               lastResOverride.override.y != config.window.res.override.y    ||

            (! EqualRect (&rcClientOrig, &rcClientLast))         ||
            (! EqualRect (&rcWindowOrig, &rcWindowLast)) )
          {
            if (rb.monitor != rb.next_monitor && rb.next_monitor != 0)
            {
              // We have to either center the window or stretch it to move monitors,
              //   if user's preference is neither, then stretch it temporarily.
              switch (config.window.borderless)
              {
                case true:
                  config.window.fullscreen |=
                    ( config.window.center == false && config.window.fullscreen == false );
                  break;

                default:
                  config.window.center = true;
                  break;
              }
            }

            SK_AdjustBorder ();
            SK_AdjustWindow ();

            lastBorderless             = config.window.borderless;
            lastCenter                 = _configCenter;
            lastFullscreen             = _configFullscreen;

            lastOffset.x.absolute      = config.window.offset.x.absolute;
            lastOffset.y.absolute      = config.window.offset.y.absolute;
            lastOffset.x.percent       = config.window.offset.x.percent;
            lastOffset.y.percent       = config.window.offset.y.percent;

            if (std::exchange (lastResOverride.override.x, config.window.res.override.x)
                                                        != config.window.res.override.x)
                                              _overrideRes = true;
            if (std::exchange (lastResOverride.override.y, config.window.res.override.y)
                                                        != config.window.res.override.y)
                                              _overrideRes = true;
          }
          }

          SK_GetWindowRect (game_window.hWnd, &game_window.actual.window);
          SK_GetClientRect (game_window.hWnd, &game_window.actual.client);

          rb.monitor =
            MonitorFromWindow (game_window.hWnd, MONITOR_DEFAULTTONEAREST);

          CopyRect (&rcClientLast, &game_window.actual.client);
          CopyRect (&rcWindowLast, &game_window.actual.window);

          extern void SK_AdjustClipRect (void);
                      SK_AdjustClipRect ();

          config.window.fullscreen = _configFullscreen;
          config.window.center     = _configCenter;

          extern volatile LONG lResetD3D11;

          // Client rect changed, we probably want to reset our render context to
          //   accomodate the new internal resolution
          if (! EqualRect (&rcClientOrig, &game_window.actual.client))
          {
          //if ( rcClientOrig.right  - rcClientOrig.left != rcClientLast.right  - rcClientLast.left ||
          //     rcClientOrig.bottom - rcClientOrig.top  != rcClientLast.bottom - rcClientLast.top )
            {
              //if (     SK_GetCurrentRenderBackend ().api == SK_RenderAPI::D3D12)
              //  InterlockedCompareExchange (&lResetD3D12, 1, 0);
              //if (SK_GetCurrentRenderBackend ().api == SK_RenderAPI::D3D11)
              //  InterlockedCompareExchange (&lResetD3D11, 1, 0);
            }
          }

          // Trigger updates on single-monitor systems even if the window doesn't move
          //
          SK_RunOnce (hMonitorOrig = 0);

          if (hMonitorOrig != rb.monitor || ReadAcquire (&lResetD3D11) == 1 || _overrideRes)
          {   hMonitorOrig  = rb.monitor;
            rb.assignOutputFromHWND (game_window.hWnd);

            extern void
            SK_Display_ResolutionSelectUI (bool bMarkDirty = false);
            SK_Display_ResolutionSelectUI (true);
            rb.gsync_state.update         (true);

            PostMessage ( game_window.hWnd,                 WM_SIZE,        SIZE_RESTORED,
              MAKELPARAM (game_window.actual.client.right -
                          game_window.actual.client.left,   game_window.actual.client.bottom -
                                                            game_window.actual.client.top )
                        );
            PostMessage ( game_window.hWnd,                 WM_DISPLAYCHANGE, 32,
              MAKELPARAM (game_window.actual.client.right -
                          game_window.actual.client.left,   game_window.actual.client.bottom -
                                                            game_window.actual.client.top )
                        );
          }

          // Clear any window move requests
          if (! config.display.save_monitor_prefs)
            rb.next_monitor = 0;

          ullLastFrame =
            SK_GetFramesDrawn ();
        }
      }

      hRepoSignal.Close ();

      SK_Thread_CloseSelf ();

      return 0;
    }, L"[SK] Window Relocation Thread", (LPVOID)hRepoSignal.m_h)
  );
}


#include <SpecialK/render/d3d11/d3d11_core.h>

extern volatile LONG lResetD3D12;
extern volatile LONG lResetD3D11;

void
SK_AdjustClipRect (void)
{
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
                          lpRect->left,    lpRect->top,
                          lpRect->right  - lpRect->left,
                          lpRect->bottom - lpRect->top ) );
  };

  // 1. Client Rectangle
  ////RECT client;
  ////GetClientRect_Original (game_window.hWnd, &client);


  // 2. Re-Compute Clip Rectangle
  const bool unconfine = config.window.unconfine_cursor;
  const bool   confine = config.window.confine_cursor;

  // This logic is questionable -- we probably need to transform the clip rect,
  //                                 but not apply it when unconfine is true.
  if (! unconfine)
  {
    RECT            clip = {};
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
      //SK_ReleaseAssert (SK_Window_HasBorder () == !config.window.borderless);

        clip = game_window.actual.window;
      }

      // TODO: Transform clip rectangle using existing rectangle
      //
      else
      {
        SK_LOG2 ( ( L"Need to transform original clip rect..." ),
                    L"Cursor Mgr" );

      ////SK_ReleaseAssert (SK_Window_HasBorder () == !config.window.borderless);
        //
        //if (! SK_Window_HasBorder ())
        //{
        //  clip.left   = game_window.actual.window.left + game_window.actual.client.left;
        //  clip.top    = game_window.actual.window.top  + game_window.actual.client.top;
        //  clip.right  = game_window.actual.window.left + game_window.actual.client.right;
        //  clip.bottom = game_window.actual.window.top  + game_window.actual.client.bottom;
        //}
        //
        //else
        //{
        //  clip.left   = game_window.actual.window.left;
        //  clip.top    = game_window.actual.window.top;
        //  clip.right  = game_window.actual.window.right;
        //  clip.bottom = game_window.actual.window.bottom;
        //}
      }

      DescribeClipRect (L"OUT ", &clip);

      //if (! wm_dispatch->moving_windows.count (game_window.hWnd))
      if (SK_IsGameWindowActive ())
          SK_ClipCursor (&clip);
      else
        SK_ClipCursor (nullptr);
    }
  }

  // Unconfine Cursor
  else
    SK_ClipCursor (nullptr);
}

// KNOWN ISSUES: 1. Do not move window using title bar while override res is enabled
//
//
void
SK_AdjustWindow (void)
{
  SK_WINDOW_LOG_CALL3 ();

  HMONITOR hMonitor =   config.display.monitor_handle != 0 ?
                        config.display.monitor_handle      :
    MonitorFromWindow ( game_window.hWnd,
                          config.display.monitor_default );//MONITOR_DEFAULTTONEAREST );

  MONITORINFO mi   = {         };
  mi.cbSize        = sizeof (mi);
  GetMonitorInfo (hMonitor, &mi);

  if (config.window.borderless && config.window.fullscreen)
  {
    SK_LOG4 ( (L" > SK_AdjustWindow (Fullscreen)"),
             L"Window Mgr" );

    SK_SetWindowPos ( game_window.hWnd,
                      SK_HWND_TOP,
                      mi.rcMonitor.left,
                      mi.rcMonitor.top,
                      mi.rcMonitor.right  - mi.rcMonitor.left,
                      mi.rcMonitor.bottom - mi.rcMonitor.top,
                      SWP_NOSENDCHANGING | SWP_NOZORDER   | SWP_ASYNCWINDOWPOS |
                      SWP_NOREPOSITION   | SWP_SHOWWINDOW | SWP_NOACTIVATE );

    SK_LOG1 ( ( L"FULLSCREEN => {Left: %li, Top: %li} - (WxH: %lix%li)",
                    mi.rcMonitor.left,    mi.rcMonitor.top,
                    mi.rcMonitor.right  - mi.rcMonitor.left,
                    mi.rcMonitor.bottom - mi.rcMonitor.top
              ), L"Border Mgr" );

    // Must set this or the mouse cursor clip rect will be wrong
    CopyRect (&game_window.actual.window, &mi.rcMonitor);
  }

  else
  {
    SK_LOG4 ( (L" > SK_AdjustWindow (Windowed)"),
             L"Window Mgr" );

    // Adjust the desktop resolution to make room for window decorations
    //   if the game window were maximized.
    if ((! config.window.borderless) && (! SK_GetCurrentRenderBackend ().fullscreen_exclusive)) {
      SK_AdjustWindowRect (
        &mi.rcWork,
        static_cast <LONG> (game_window.actual.style),
        FALSE
      );
    }

    // Monitor Workspace
          LONG mon_width     = mi.rcWork.right     - mi.rcWork.left;
          LONG mon_height    = mi.rcWork.bottom    - mi.rcWork.top;

    // Monitor's ENTIRE coordinate space (includes taskbar)
    const LONG real_width    = mi.rcMonitor.right  - mi.rcMonitor.left;
    const LONG real_height   = mi.rcMonitor.bottom - mi.rcMonitor.top;

    // The Game's _Requested_ Client Rectangle
          LONG render_width  = game_window.game.client.right  - game_window.game.client.left;
          LONG render_height = game_window.game.client.bottom - game_window.game.client.top;

    // The Game's _Requested_ Window Rectangle (including borders)
    const LONG full_width     = game_window.game.window.right  - game_window.game.window.left;
    const LONG full_height    = game_window.game.window.bottom - game_window.game.window.top;

    if ((! config.window.res.override.isZero ()))
    {
      render_width  = config.window.res.override.x;
      render_height = config.window.res.override.y;

      SK_GetClientRect (game_window.hWnd, &game_window.game.client);
      SK_GetWindowRect (game_window.hWnd, &game_window.game.window);
    }

    else {
      const int  render_width_before  = game_window.game.client.right  - game_window.game.client.left;
      const int  render_height_before = game_window.game.client.bottom - game_window.game.client.top;

      const RECT client_before =
        game_window.game.client;

      SK_GetClientRect (game_window.hWnd, &game_window.game.client);
      SK_GetWindowRect (game_window.hWnd, &game_window.game.window);

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

    const LONG win_width  = std::min (mon_width,  render_width);
    const LONG win_height = std::min (mon_height, render_height);

    // Eliminate relative epsilon from screen percentage offset coords;
    //   otherwise we may accidentally move the window.
    bool nomove = config.window.offset.isZero       ();
    bool nosize = config.window.res.override.isZero ();

    static SK_RenderBackend& rb =
      SK_GetCurrentRenderBackend ();

    // We will offset coordinates later; move the window to the top-left
    //   origin first.
    if (config.window.center && (! rb.fullscreen_exclusive))
    {
      // If centering changes monitors, ignore.
      ////if ( MonitorFromRect   (&mi.rcMonitor,    MONITOR_DEFAULTTONEAREST) ==
      ////     MonitorFromWindow (game_window.hWnd, MONITOR_DEFAULTTONEAREST) )
      ////{
        game_window.actual.window.left   = mi.rcMonitor.left;
        game_window.actual.window.top    = mi.rcMonitor.top;
        game_window.actual.window.right  = mi.rcMonitor.left + win_width;
        game_window.actual.window.bottom = mi.rcMonitor.top  + win_height;

        nomove                           = false; // Centering requires moving ;)
      ////}
    }

    else
    {
      HMONITOR hMonGame =
           MonitorFromRect (&game_window.game.window, MONITOR_DEFAULTTONEAREST);
      if ( MonitorFromRect (&mi.rcMonitor,            MONITOR_DEFAULTTONEAREST) ==
                                 hMonGame)
      {
        game_window.actual.window.left   = game_window.game.window.left;
        game_window.actual.window.top    = game_window.game.window.top;
      }

      // Apply the game's offset from monitor edge to the monitor SK is moving
      //   the window to
      else
      {
        MONITORINFO
          mi_game        = {                  };
          mi_game.cbSize = sizeof (MONITORINFO);
        if (GetMonitorInfo (hMonGame, &mi_game))
        {
          game_window.actual.window.left = (game_window.game.window.left - mi_game.rcMonitor.left) + mi.rcMonitor.left;
          game_window.actual.window.top  = (game_window.game.window.top  - mi_game.rcMonitor.top)  + mi.rcMonitor.top;
        }
      }
    }

    const int x_offset = ( config.window.offset.x.percent != 0.0f ?
             (int)( config.window.offset.x.percent *
                           (mi.rcWork.right - mi.rcWork.left) ) :
                           config.window.offset.x.absolute );

    const int y_offset = ( config.window.offset.y.percent != 0.0f ?
             (int)( config.window.offset.y.percent *
                           (mi.rcWork.bottom - mi.rcWork.top) ) :
                           config.window.offset.y.absolute );

    if (x_offset > 0)
      game_window.actual.window.left  = mi.rcWork.left  + x_offset - 1;
    else if (x_offset < 0)
      game_window.actual.window.right = mi.rcWork.right + x_offset + 1;


    if (y_offset > 0)
      game_window.actual.window.top    = mi.rcWork.top    + y_offset - 1;
    else if (y_offset < 0)
      game_window.actual.window.bottom = mi.rcWork.bottom + y_offset + 1;

    if (config.window.center && (! ( (config.window.fullscreen &&
        config.window.borderless)||
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
      SK_AdjustWindowRect (
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
      SK_SetWindowPos ( game_window.hWnd,
                        SK_HWND_TOP,
                        game_window.actual.window.left,
                        game_window.actual.window.top,
                        game_window.actual.window.right  - game_window.actual.window.left,
                        game_window.actual.window.bottom - game_window.actual.window.top,
                        SWP_NOSENDCHANGING | SWP_NOZORDER   |
                        SWP_NOREPOSITION   | SWP_SHOWWINDOW |
                        (nomove ? SWP_NOMOVE : 0x00) |
                        (nosize ? SWP_NOSIZE : 0x00) | SWP_NOACTIVATE );
    }
    SK_GetWindowRect (game_window.hWnd, &game_window.game.window);
    SK_GetClientRect (game_window.hWnd, &game_window.game.client);

    CopyRect (&game_window.actual.window, &game_window.game.window);
    CopyRect (&game_window.actual.client, &game_window.game.client);


    wchar_t wszBorderDesc [128] = { };

    const bool has_border =
      SK_WindowManager::StyleHasBorder (game_window.actual.style);

    // Summarize the border
    if (SK_WindowManager::StyleHasBorder (game_window.actual.style))
    {
      swprintf ( wszBorderDesc,
                L"(Frame = %lipx x %lipx, Title = %lipx)",
                2 * SK_GetSystemMetrics (SM_CXDLGFRAME),
                2 * SK_GetSystemMetrics (SM_CYDLGFRAME),
                    SK_GetSystemMetrics (SM_CYCAPTION) );
    }

    SK_LOG1 ( ( L"WINDOW => {Left: %li, Top: %li} - (WxH: %lix%li) - { Border: %s }",
                game_window.actual.window.left,    game_window.actual.window.top,
                game_window.actual.window.right  - game_window.actual.window.left,
                game_window.actual.window.bottom - game_window.actual.window.top,
             (! has_border) ?
                    L"None" :
                    wszBorderDesc
              ),    L"Border Mgr" );
  }

  SK_AdjustClipRect ();
}

static int
WINAPI
GetSystemMetrics_Detour (_In_ int nIndex)
{
  SK_LOG_FIRST_CALL

  const int nRet =
    SK_GetSystemMetrics (nIndex);

  SK_LOG4 ( ( L"GetSystemMetrics (%4li) : %-5li - %s",
                                 nIndex, nRet,
                      SK_SummarizeCaller ().c_str () ),
              L"Resolution"
          );


#ifndef _REMOVE_DEPRECATED_CRAP
  static bool bDotHack =
    (SK_GetCurrentGameID () == SK_GAME_ID::DotHackGU);

  if (bDotHack)
  {
    if (nIndex == SM_CYCAPTION)      return 0;
    if (nIndex == SM_CYMENU)         return 0;
    if (nIndex == SM_CXBORDER)       return 0;
    if (nIndex == SM_CYBORDER)       return 0;
    if (nIndex == SM_CXDLGFRAME)     return 0;
    if (nIndex == SM_CYDLGFRAME)     return 0;
    if (nIndex == SM_CXFRAME)        return 0;
    if (nIndex == SM_CYFRAME)        return 0;
    if (nIndex == SM_CXPADDEDBORDER) return 0;
  }
#endif

  return nRet;
}

int
WINAPI
SK_GetSystemMetrics (_In_ int nIndex)
{
  if (     GetSystemMetrics_Original != nullptr)
    return GetSystemMetrics_Original (nIndex);

  return
    GetSystemMetrics (nIndex);
}

BOOL
WINAPI
GetWindowInfo_Detour (HWND hwnd, PWINDOWINFO pwi)
{
  SK_LOG_FIRST_CALL

  BOOL bRet =
    GetWindowInfo_Original (hwnd, pwi);

  // Apps may call GetWindowInfo and check WS_ACTIVECAPTION rather
  //   than watching their Win32 message pump for the appropriate
  //     message...
  if ( pwi         != nullptr             &&
       pwi->cbSize == sizeof (WINDOWINFO) &&
       bRet                               &&
                 hwnd == game_window.hWnd &&
          SK_WantBackgroundRender () )
  {
    pwi->dwWindowStatus = WS_ACTIVECAPTION;
  }

  return bRet;
}

BOOL
WINAPI
SK_GetWindowInfo (HWND hwnd, PWINDOWINFO pwi)
{
  if (     GetWindowInfo_Original != nullptr)
    return GetWindowInfo_Original (hwnd, pwi);

  return
    GetWindowInfo (hwnd, pwi);
}


using TranslateMessage_pfn =
BOOL (WINAPI *)(
  _In_ const MSG *lpMsg
);

using NtUserDispatchMessage_pfn =
LRESULT (NTAPI *)(
  _In_ const MSG *lpmsg
);

using NtUserGetMessage_pfn =
BOOL (NTAPI *)( LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax );

using NtUserPeekMessage_pfn =
BOOL (NTAPI *)(
  _Out_    LPMSG lpMsg,
  _In_opt_ HWND  hWnd,
  _In_     UINT  wMsgFilterMin,
  _In_     UINT  wMsgFilterMax,
  _In_     UINT  wRemoveMsg
  );

TranslateMessage_pfn TranslateMessage_Original = nullptr;

NtUserPeekMessage_pfn     NtUserPeekMessage     = nullptr;
NtUserGetMessage_pfn      NtUserGetMessage      = nullptr;
//NtUserDispatchMessage_pfn NtUserDispatchMessage = nullptr;

NtUserPeekMessage_pfn     PeekMessageA_Original     = nullptr;
NtUserGetMessage_pfn      GetMessageA_Original      = nullptr;
NtUserDispatchMessage_pfn DispatchMessageA_Original = nullptr;

NtUserPeekMessage_pfn     PeekMessageW_Original     = nullptr;
NtUserGetMessage_pfn      GetMessageW_Original      = nullptr;
NtUserDispatchMessage_pfn DispatchMessageW_Original = nullptr;

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
        if (lpMsg->message == WM_MENUCHAR)
          return MAKEWPARAM (0, MNC_CLOSE);

        return TRUE;
      } break;
    }
  }

  return TranslateMessage_Original (lpMsg);
}

LRESULT
WINAPI
ImGui_WndProcHandler ( HWND hWnd, UINT   msg,
                       WPARAM wParam,
                       LPARAM lParam );

bool
SK_EarlyDispatchMessage (MSG *lpMsg, bool remove, bool peek)
{
  static constexpr DWORD dwMsgMax = 0xFFFFUL;

  // Remove SK's global window focus notification before it can be
  //   marshalled by the application
  if (lpMsg != nullptr && lpMsg->message == 0xfa57/*WM_USER + WM_SETFOCUS*/)
  {
    SK_Window_OnFocusChange ((HWND)lpMsg->wParam, (HWND)lpMsg->lParam);

    lpMsg->message = WM_NULL;

    return true;
  }

  if (                          lpMsg != nullptr &&
                    dwMsgMax >= lpMsg->message   &&
       SK_ImGui_HandlesMessage (lpMsg, remove, peek)                       )
  {
    if ( lpMsg->message == WM_INPUT )
    {
      DefWindowProcW ( lpMsg->hwnd,   lpMsg->message,
                       lpMsg->wParam, lpMsg->lParam );

      remove = true;
    }

    if (remove)
    {
      lpMsg->message = WM_NULL;
    }

    return true;
  }

  return false;
}


UINT
SK_Input_ClassifyRawInput ( HRAWINPUT lParam,
                            bool&     mouse,
                            bool&     keyboard,
                            bool&     gamepad );


BOOL
WINAPI
PeekMessageA_Detour (
  _Out_    LPMSG lpMsg,
  _In_opt_ HWND  hWnd,
  _In_     UINT  wMsgFilterMin,
  _In_     UINT  wMsgFilterMax,
  _In_     UINT  wRemoveMsg )
{
#if _DEBUG
  SK_LOG_FIRST_CALL
#endif

  auto PeekFunc = NtUserPeekMessage != nullptr ?
                  NtUserPeekMessage :
                        PeekMessageA_Original;

  if ( PeekFunc == nullptr )
  {
    // A nasty kludge to fix the Steam overlay
    static auto         early_PeekMessageA =
      reinterpret_cast <NtUserPeekMessage_pfn> (
             SK_GetProcAddress      (
               SK_GetModuleHandle (   L"user32"  ),
                                       "PeekMessageA"
                                    )          );

    PeekFunc =
      early_PeekMessageA;

    if (PeekFunc != nullptr)
    {
      return
        PeekFunc ( lpMsg,
                        hWnd,
                             wMsgFilterMin,
                             wMsgFilterMax,
                                           wRemoveMsg );
    }

    return 0;
  }


  if (config.render.dxgi.safe_fullscreen)
    wRemoveMsg |= PM_REMOVE;

  if ( PeekFunc ( lpMsg,
                       hWnd,
                            wMsgFilterMin,
                            wMsgFilterMax,
                                          wRemoveMsg )
     )
  { // ---- RAW Input Background Hack ----
    if (      SK_WantBackgroundRender () &&
              lpMsg->message == WM_INPUT &&
              lpMsg->wParam  == RIM_INPUTSINK )
    {
      bool keyboard = false;
      bool mouse    = false;
      bool gamepad  = false;

      SK_Input_ClassifyRawInput (
        reinterpret_cast <HRAWINPUT> (lpMsg->lParam),
          mouse, keyboard, gamepad
      );

      if (gamepad)
      {
        lpMsg->wParam = RIM_INPUT;
        return TRUE;
      }
    } // ---- RAW Input Background Hack ----

    if ( (wRemoveMsg & PM_REMOVE) ==
                       PM_REMOVE )
    {
      SK_EarlyDispatchMessage (lpMsg, true, true);
    }

    return TRUE;
  }

  return FALSE;
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
#if _DEBUG
  SK_LOG_FIRST_CALL
#endif

  auto PeekFunc = NtUserPeekMessage != nullptr ?
                  NtUserPeekMessage :
                        PeekMessageW_Original;

  if ( PeekFunc == nullptr )
  {
    // A nasty kludge to fix the Steam overlay
    static auto         early_PeekMessageW =
      reinterpret_cast <NtUserPeekMessage_pfn> (
             SK_GetProcAddress      (
               SK_GetModuleHandle (   L"user32"  ),
                                       "PeekMessageW"
                                    )          );

    NtUserPeekMessage_pfn early =
      early_PeekMessageW;

    PeekFunc = early != nullptr ?
               early : PeekFunc;


    if (PeekFunc != nullptr)
    {
      return
        PeekFunc ( lpMsg,
                        hWnd,
                             wMsgFilterMin,
                             wMsgFilterMax,
                                           wRemoveMsg );
    }

    return 0;
  }


  if (config.render.dxgi.safe_fullscreen)
    wRemoveMsg |= PM_REMOVE;

  if ( PeekFunc ( lpMsg,
                       hWnd,
                            wMsgFilterMin,
                            wMsgFilterMax,
                                          wRemoveMsg )
     )
  { // ---- RAW Input Background Hack ----
    if ( SK_WantBackgroundRender () &&
         lpMsg->message == WM_INPUT &&
         lpMsg->wParam  == RIM_INPUTSINK )
    {
      bool keyboard = false;
      bool mouse    = false;
      bool gamepad  = false;

      SK_Input_ClassifyRawInput (
        reinterpret_cast <HRAWINPUT> (lpMsg->lParam),
          mouse, keyboard, gamepad
      );

      if (gamepad)
      {
        lpMsg->wParam = RIM_INPUT;
        return TRUE;
      }
    } // ---- RAW Input Background Hack ----

    if ( (wRemoveMsg & PM_REMOVE) ==
                       PM_REMOVE )
    {
      SK_EarlyDispatchMessage (lpMsg, true, true);
    }

    return TRUE;
  }

  return FALSE;
}


BOOL
WINAPI
GetMessageA_Detour (LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax)
{
  SK_LOG_FIRST_CALL

  auto GetFunc = NtUserGetMessage != nullptr ?
                 NtUserGetMessage :
                       GetMessageA_Original;

  if ( GetFunc == nullptr )
  {
    // A nasty kludge to fix the Steam overlay
    static auto         early_GetMessageA =
      reinterpret_cast <NtUserGetMessage_pfn> (
             SK_GetProcAddress      (
               SK_GetModuleHandle (   L"user32"  ),
                                       "GetMessageA"
                                    )          );

    SK_ReleaseAssert ( early_GetMessageA != nullptr );

    GetFunc =
      early_GetMessageA;

    if (GetFunc != nullptr)
    {
      return
        GetFunc ( lpMsg,
                    hWnd,
                      wMsgFilterMin,
                      wMsgFilterMax );
    }

    return 0;
  }

  const BOOL bRet =
    GetFunc ( lpMsg,
                hWnd,
                  wMsgFilterMin,
                  wMsgFilterMax );

  if ( bRet != FALSE )
  {
    if ( SK_EarlyDispatchMessage ( lpMsg, false, false ) )
    {
      lpMsg->message = WM_NULL;
    }
  }

  return
    bRet;
}

BOOL
WINAPI
SK_GetMessageW (LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax)
{
  if (GetMessageW_Original != nullptr)
    return GetMessageW_Original (lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);

  return GetMessageW (lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
}

LRESULT
WINAPI
SK_DispatchMessageW (_In_ const MSG *lpMsg)
{
  if (DispatchMessageW_Original != nullptr)
    return DispatchMessageW_Original (lpMsg);

  return DispatchMessageW (lpMsg);
}

BOOL
WINAPI
GetMessageW_Detour (LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax)
{
  SK_LOG_FIRST_CALL

  auto GetFunc = NtUserGetMessage != nullptr ?
                 NtUserGetMessage :
                       GetMessageW_Original;

  if ( GetFunc == nullptr )
  {
    // A nasty kludge to fix the Steam overlay
    static auto         early_GetMessageW =
      reinterpret_cast <NtUserGetMessage_pfn> (
             SK_GetProcAddress      (
               SK_GetModuleHandle (   L"user32"  ),
                                       "GetMessageW"
                                    )          );

    SK_ReleaseAssert ( early_GetMessageW != nullptr );

    GetFunc =
      early_GetMessageW;

    if (GetFunc != nullptr)
    {
      return
        GetFunc ( lpMsg,
                    hWnd,
                      wMsgFilterMin,
                      wMsgFilterMax );
    }

    return 0;
  }

  const BOOL bRet =
    GetFunc ( lpMsg,
                hWnd,
                  wMsgFilterMin,
                  wMsgFilterMax );

  if ( bRet )
  {
    if ( SK_EarlyDispatchMessage ( lpMsg, false, false ) )
    {

      lpMsg->message = WM_NULL;
    }
  }

  return
    bRet;
}


LRESULT
WINAPI
DispatchMessageA_Detour (_In_ const MSG* lpMsg)
{
  SK_LOG_FIRST_CALL

  MSG orig_msg = *lpMsg; //-V821
  MSG      msg = *lpMsg;

  if ( SK_EarlyDispatchMessage ( &msg, true ) )
  {
    auto DefWindowProc = DefWindowProcA;

    return
      DefWindowProc ( orig_msg.hwnd,   orig_msg.message,
                      orig_msg.wParam, orig_msg.lParam  );
  }

  return
    DispatchMessageA_Original (&msg);
}

LRESULT
WINAPI
DispatchMessageW_Detour (_In_ const MSG* lpMsg)
{
  SK_LOG_FIRST_CALL

  MSG orig_msg = *lpMsg; //-V821
  MSG      msg = *lpMsg;

  if ( SK_EarlyDispatchMessage ( &msg, true ) )
  {
    auto DefWindowProc = DefWindowProcW;

    return
      DefWindowProc ( orig_msg.hwnd,   orig_msg.message,
                      orig_msg.wParam, orig_msg.lParam  );
  }

  return
    DispatchMessageW_Original (&msg);
}


typedef HWND (WINAPI* GetFocus_pfn)(void);
                      GetFocus_pfn
                      GetFocus_Original = nullptr;

HWND
WINAPI
SK_GetFocus (void)
{
  if (GetFocus_Original != nullptr)
    return GetFocus_Original ();

  return GetFocus ();
}

HWND
WINAPI
GetFocus_Detour (void)
{
  SK_LOG_FIRST_CALL

  ///// Overriding this is not a great idea,
  /////   it will enable input to slip through during Alt-Tab.
  ///if (config.window.background_render)
  ///{
  ///  // Keep a cache of non-NULL focus HWNDs for this thread
  ///  concurrency::concurrent_unordered_map <DWORD, HWND>
  ///    focus_windows;
  ///
  ///  HWND hWndFocus =
  ///    SK_GetFocus ( );
  ///  DWORD dwTid    =
  ///    SK_GetCurrentThreadId ();
  ///
  ///  if ( hWndFocus != nullptr )
  ///           focus_windows       [dwTid] = hWndFocus;
  ///  else if (focus_windows.count (dwTid) &&
  ///           focus_windows       [dwTid] == SK_GetGameWindow ())
  ///    return focus_windows       [dwTid];
  ///  else
  ///    return SK_GetGameWindow ();
  ///}

  return
    SK_GetFocus ();
}

typedef HWND (WINAPI *GetActiveWindow_pfn)(void);
                      GetActiveWindow_pfn
                      GetActiveWindow_Original = nullptr;

typedef HWND (WINAPI *SetActiveWindow_pfn)(HWND);
                      SetActiveWindow_pfn
                      SetActiveWindow_Original = nullptr;

BOOL
WINAPI
SK_IsWindowUnicode (HWND hWnd, SK_TLS *pTLS)
{
  UNREFERENCED_PARAMETER (pTLS);

  // Faster
  if (hWnd == game_window.hWnd)
    return game_window.unicode;

  return
    IsWindowUnicode (hWnd);
}

HWND
WINAPI
SK_GetActiveWindow (SK_TLS *pTLS)
{
  UNREFERENCED_PARAMETER (pTLS);
  ////if (pTLS == nullptr)
  ////    pTLS  = SK_TLS_Bottom ();
  ////
  ////if (pTLS != nullptr)
  ////{
  ////  if ((uintptr_t)pTLS->win32->active == (uintptr_t)-1)
  ////  {
  ////    pTLS->win32->active =
  ////      GetActiveWindow_Original != nullptr ?
  ////      GetActiveWindow_Original ()         :
  ////      GetActiveWindow          ();
  ////  }
  ////
  ////  return
  ////    pTLS->win32->active;
  ////}

  return
    GetActiveWindow_Original != nullptr ?
    GetActiveWindow_Original ()         :
    GetActiveWindow          ();
}

HWND
WINAPI
GetActiveWindow_Detour (void)
{
  SK_LOG_FIRST_CALL

  ////SK_TLS
  //// *pTLS =
  ////SK_TLS_Bottom ();
  ////
  ////if (pTLS != nullptr)
  ////{
  ////  // Take this opportunity to update any stale data
  ////  //   since we're making a round-trip anyway.
  ////  pTLS->win32->active =
  ////    GetActiveWindow_Original ();
  ////}
  ////
  ///////if (config.window.background_render)
  ///////{
  ///////  // Keep a cache of non-NULL active HWNDs for this thread
  ///////  concurrency::concurrent_unordered_map <DWORD, HWND>
  ///////    active_windows;
  ///////
  ///////  HWND hWndActive =
  ///////    SK_GetActiveWindow (pTLS);
  ///////  DWORD dwTid     =
  ///////    pTLS->debug.tid;
  ///////
  ///////  if ( hWndActive != nullptr )
  ///////           active_windows       [dwTid] = hWndActive;
  ///////  else if (active_windows.count (dwTid) &&
  ///////           active_windows       [dwTid] == SK_GetGameWindow ())
  ///////    return active_windows       [dwTid];
  ///////  else
  ///////    return SK_GetGameWindow ();
  ///////}

  return
    SK_GetActiveWindow (/*pTLS*/nullptr);
}

HWND
WINAPI
SK_SetActiveWindow (HWND hWnd)
{
  HWND
    hWndRet =
      SetActiveWindow_Original != nullptr ?
      SetActiveWindow_Original (hWnd)     :
      SetActiveWindow          (hWnd);

  if (hWndRet != nullptr)
  {
    SK_TLS* pTLS =
      SK_TLS_Bottom ();

    pTLS->win32->active      = hWnd;
    pTLS->win32->last_active = hWndRet;
  }

  return hWndRet;
}

HWND
WINAPI
SetActiveWindow_Detour (HWND hWnd)
{
  SK_LOG_FIRST_CALL

  return
    SK_SetActiveWindow (hWnd);
}


typedef HWND (WINAPI *GetForegroundWindow_pfn)(void);
                      GetForegroundWindow_pfn
                      GetForegroundWindow_Original = nullptr;

HWND
WINAPI
SK_GetForegroundWindow (void)
{
  if (GetForegroundWindow_Original != nullptr)
    return GetForegroundWindow_Original ();

  return
    GetForegroundWindow ();
}

HWND
WINAPI
GetForegroundWindow_Detour (void)
{
  SK_LOG_FIRST_CALL

  static auto& rb =
    SK_GetCurrentRenderBackend ();

  if (! rb.fullscreen_exclusive)
  {                                    // Frame Pacing Has Problems w/o this
    if ( SK_WantBackgroundRender () || config.window.always_on_top == SmartAlwaysOnTop ||
         config.window.treat_fg_as_active )
    {
      return game_window.hWnd;
    }
  }

  return
    SK_GetForegroundWindow ();
}

typedef BOOL (WINAPI *BringWindowToTop_pfn)(HWND);
                      BringWindowToTop_pfn
                      BringWindowToTop_Original = nullptr;

BOOL
WINAPI
BringWindowToTop_Detour (HWND hWnd)
{
  SK_LOG_FIRST_CALL;

  // This breaks alt-tab and window activation in some cases
#if 0
  DWORD                            dwPid = 0x0;
  GetWindowThreadProcessId (hWnd, &dwPid);

  if (GetCurrentProcessId () == dwPid)
  {
    return
      BringWindowToTop_Original (hWnd);
  }

  return FALSE;
#else
  return
    BringWindowToTop_Original (hWnd);
#endif
}

typedef BOOL (WINAPI *SetForegroundWindow_pfn)(HWND);
                      SetForegroundWindow_pfn
                      SetForegroundWindow_Original = nullptr;

BOOL
WINAPI
SetForegroundWindow_Detour (HWND hWnd)
{
  SK_LOG_FIRST_CALL;

  // This breaks alt-tab and window activation in some cases
#if 0
  DWORD                            dwPid = 0x0;
  GetWindowThreadProcessId (hWnd, &dwPid);

  if (GetCurrentProcessId () == dwPid)
  {
    return
      SetForegroundWindow_Original (hWnd);
  }

  return FALSE;
#else
  return
    SetForegroundWindow_Original (hWnd);
#endif
}

void
RealizeForegroundWindow_Impl (HWND hWndForeground)
{
  DWORD dwPid          = 0;
  HWND  hWndOrig       =
    SK_GetForegroundWindow ();

  const DWORD dwThreadId     =
    SK_Thread_GetCurrentId  ();
  const DWORD dwOrigThreadId =
    GetWindowThreadProcessId (hWndOrig, &dwPid);

  const bool show_hide =
            IsWindow (hWndOrig) &&
    hWndForeground != hWndOrig  && dwPid == GetCurrentProcessId ();

  IsGUIThread       (TRUE);
  AttachThreadInput (dwThreadId, dwOrigThreadId, true );

  if ( show_hide && game_window.hWnd     == SK_HWND_DESKTOP &&
                                hWndOrig != hWndForeground )
  {
    WINDOWINFO   wi        = {                 };
                 wi.cbSize = sizeof (WINDOWINFO);
    SK_GetWindowInfo (
      hWndOrig, &wi  );

    // We only need to do this for fullscreen exclusive mode
    if ( sk::narrow_cast <LONG> (wi.cxWindowBorders) ==
         sk::narrow_cast <LONG> (wi.cyWindowBorders) &&
         sk::narrow_cast <LONG> (wi.rcClient.left)   ==
         sk::narrow_cast <LONG> (wi.rcClient.bottom) &&
         sk::narrow_cast <LONG> (wi.cxWindowBorders) ==
         sk::narrow_cast <LONG> (wi.rcClient.bottom) &&
         sk::narrow_cast <LONG> (wi.cxWindowBorders) == 0L )
    {
      SK_SetWindowPos ( hWndOrig, SK_HWND_DESKTOP, 0, 0, 0, 0,
                        SWP_HIDEWINDOW     | SWP_NOACTIVATE   |
                        SWP_NOSIZE         | SWP_NOMOVE       |
                        SWP_NOSENDCHANGING | SWP_NOREPOSITION );
    }
  }

  SetForegroundWindow (hWndForeground);
  SK_SetWindowPos     (hWndForeground, SK_HWND_TOPMOST,
                       0, 0,
                       0, 0, SWP_NOSIZE   | SWP_NOMOVE     |
                       SWP_SHOWWINDOW     | SWP_NOACTIVATE |
                       SWP_NOSENDCHANGING | SWP_NOCOPYBITS );

  SetForegroundWindow (hWndForeground);
  SK_SetWindowPos     (hWndForeground, SK_HWND_NOTOPMOST,
                       0, 0,
                       0, 0, SWP_NOSIZE | SWP_NOMOVE     |
                       SWP_SHOWWINDOW   | SWP_NOACTIVATE |
                       SWP_DEFERERASE   | SWP_NOSENDCHANGING );

  SetForegroundWindow (hWndForeground);

  AttachThreadInput   (dwOrigThreadId, dwThreadId, false);

  SK_SetActiveWindow  (hWndForeground);
  BringWindowToTop    (hWndForeground);
  SetFocus            (hWndForeground);
}

HWND
WINAPI
SK_RealizeForegroundWindow (HWND hWndForeground)
{
  HWND hWndOrig =
    SK_GetForegroundWindow ();

        DWORD dwPid;
  const DWORD dwOrigThreadId =
    GetWindowThreadProcessId (hWndForeground, &dwPid);

#if 0
  if (dwOrigThreadId != SK_GetCurrentThreadId ())
  {
    std::future <HWND> resultOfAsync =
                              std::async (
                      std::launch::async,
    [&](HWND hWndForeground) ->
    HWND
    {
      RealizeForegroundWindow_Impl (hWndForeground);

      return hWndForeground;
    }, hWndForeground);

    return
      hWndForeground;
  }
#else
  if (dwOrigThreadId != SK_GetCurrentThreadId ())
  {
    static concurrency::concurrent_queue <HWND> hwnd_queue;
    static SK_AutoHandle                        hwnd_signal (
      CreateEvent (nullptr, FALSE, FALSE, nullptr)
    );

    hwnd_queue.push (hWndForeground);
    SetEvent        (hwnd_signal);

    SK_RunOnce (
    {
      SK_Thread_CreateEx ([](LPVOID) ->
      DWORD
      {
        HANDLE hSignals [] = {
          __SK_DLL_TeardownEvent,
           hwnd_signal
        };

        DWORD dwWait = 0;

        while ( (dwWait = WaitForMultipleObjects (
                            2, hSignals, FALSE, INFINITE )
                ) != WAIT_OBJECT_0 )
        {
          HWND hWnd;

          while (! hwnd_queue.empty ())
          {
            if (hwnd_queue.try_pop (hWnd))
            {
              //SK_ImGui_Warning (L"RealizeForegroundWindow");

              RealizeForegroundWindow_Impl (hWnd);
            }
          }
        }

        SK_Thread_CloseSelf ();

        return 0;
      }, L"[SK] RealizeForegroundWindow", (LPVOID)hWndForeground);
    });
  }
#endif
  else
  {
    RealizeForegroundWindow_Impl (hWndForeground);
  }

  return hWndOrig;
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

float g_fDPIScale = 1.0f;

extern
LRESULT
CALLBACK
SK_ImGui_KeyboardProc (int code, WPARAM wParam, LPARAM lParam);

extern
LRESULT
CALLBACK
SK_ImGui_MouseProc    (int code, WPARAM wParam, LPARAM lParam);


bool
__SKX_WinHook_InstallInputHooks (HWND hWnd)
{
  using  SetWindowsHookEx_pfn    =
    HHOOK (WINAPI *)(int, HOOKPROC, HINSTANCE, DWORD);
  using  UnhookWindowsHookEx_pfn =
    BOOL (WINAPI *)(HHOOK);

  extern SetWindowsHookEx_pfn    SetWindowsHookExA_Original;
  extern SetWindowsHookEx_pfn    SetWindowsHookExW_Original;
  extern UnhookWindowsHookEx_pfn UnhookWindowsHookEx_Original;

  if (SetWindowsHookExW_Original == nullptr)
    return false;

  static HHOOK hHookKeyboard = 0;
  static HHOOK hHookMouse    = 0;

  if (hHookKeyboard != 0 && UnhookWindowsHookEx_Original (hHookKeyboard))
      hHookKeyboard  = 0;

  if (hHookMouse != 0 && UnhookWindowsHookEx_Original (hHookMouse))
      hHookMouse  = 0;

  if (hWnd != 0x0)
  {
     SetWindowsHookEx_pfn
    _SetWindowsHookEx =
      IsWindowUnicode (hWnd)          ?
           SetWindowsHookExW_Original :
           SetWindowsHookExA_Original;

    if (hHookKeyboard == 0)
    {
      hHookKeyboard =
        _SetWindowsHookEx (
          WH_KEYBOARD, SK_ImGui_KeyboardProc,
              0, GetCurrentThreadId ()
                          );
    }

    if (hHookMouse == 0)
    {
      hHookMouse =
        _SetWindowsHookEx (
          WH_MOUSE,    SK_ImGui_MouseProc,
              0, GetCurrentThreadId ()
                          );
    }
  }

  return true;
}


bool __ignore = false;

#ifndef WM_NCUAHDRAWCAPTION
#define WM_NCUAHDRAWCAPTION (0x00AE)
#endif
#ifndef WM_NCUAHDRAWFRAME
#define WM_NCUAHDRAWFRAME   (0x00AF)
#endif

__declspec (noinline)
LRESULT
CALLBACK
SK_DetourWindowProc ( _In_  HWND   hWnd,
                      _In_  UINT   uMsg,
                      _In_  WPARAM wParam,
                      _In_  LPARAM lParam )
{
  if (uMsg == WM_NULL)
  {
#if 0
    HWND unicode_hwnd =
      IsWindowUnicode (hWnd) ? hWnd : 0;

    return (unicode_hwnd == hWnd) ?
            DefWindowProcW (hWnd, uMsg, wParam, lParam) :
            DefWindowProcA (hWnd, uMsg, wParam, lParam);
#else
    // Valve's engine just stack overflows with infinite recursion if we
    //   allow this to work the way it's supposed to... so @#$% it :)
    return 1;
#endif
  }


  // If we are forcing a shutdown, then route any messages through the
  //   default Win32 handler.
  if ( ReadAcquire (&__SK_DLL_Ending) != FALSE && ( uMsg != WM_SYSCOMMAND &&
                                                    uMsg != WM_CLOSE      &&
                                                    uMsg != WM_QUIT )        )
  {
    if (IsWindow (hWnd))
    {
      return
        SK_COMPAT_SafeCallProc ( &game_window, hWnd,
                                               uMsg, wParam,
                                                     lParam );
    }

    return
      game_window.DefWindowProc ( hWnd,
                                  uMsg, wParam,
                                        lParam );
  }


  static bool last_active = game_window.active;

  const bool console_visible =
    SK_Console::getInstance ()->isVisible ();


  static auto& rb =
    SK_GetCurrentRenderBackend ();

  static bool        first_run = true;
  if (std::exchange (first_run, false))
  {
    if (SK_GetFocus () == game_window.hWnd || SK_GetForegroundWindow () == game_window.hWnd)
      ActivateWindow  (game_window.hWnd, true);

    // Start unmuted (in case the game crashed in the background)
    if (config.window.background_mute != false)
      SK_SetGameMute (false);

    rb.windows.setFocus     (hWnd);
    rb.updateOutputTopology (    );

    // ResolutionSelectUI will use the name here for matching purposes,
    //   so update the name immediately.
    rb.assignOutputFromHWND (game_window.hWnd);

    DWORD dwFocus        = 0x0;
    DWORD dwForeground   = 0x0;

    HWND  hWndFocus      = SK_GetFocus            ();
    HWND  hWndForeground = SK_GetForegroundWindow ();

    if (! ( game_window.hWnd == hWndFocus   ||
            game_window.hWnd == hWndForeground ) )
    {
      GetWindowThreadProcessId (hWndFocus,      &dwFocus);
      GetWindowThreadProcessId (hWndForeground, &dwForeground);

      game_window.active       = ( dwFocus      == GetCurrentProcessId () ||
                                   dwForeground == GetCurrentProcessId () );
    }

    else
      game_window.active = true;

    SK_Window_OnFocusChange (game_window.hWnd, hWndFocus);

    // Make sure any pending changes are finished before querying the
    //   actual value
    SK_Window_WaitForAsyncSetWindowLong ();

    game_window.game.style   = game_window.GetWindowLongPtr (game_window.hWnd, GWL_STYLE);
    game_window.actual.style = game_window.GetWindowLongPtr (game_window.hWnd, GWL_STYLE);
    game_window.unicode      =          SK_Window_IsUnicode (game_window.hWnd)   != FALSE;

    SK_GetWindowRect (game_window.hWnd, &game_window.game.window  );
    SK_GetClientRect (game_window.hWnd, &game_window.game.client  );
    SK_GetWindowRect (game_window.hWnd, &game_window.actual.window);
    SK_GetClientRect (game_window.hWnd, &game_window.actual.client);

    SK_InitWindow (hWnd, false);
  }


  extern bool SK_ImGui_WantExit;

  // HDR caps, Refresh Rate and Display Mode may have changed,
  //   let's try to be smart about this (for a change)...
  if ( uMsg == WM_DISPLAYCHANGE ||
       uMsg == WM_SETFOCUS
     )
  {
    if (uMsg == WM_DISPLAYCHANGE)
    {
      // Trigger IndirectX Device Context Reset
      SK_GL_OnD3D11_Reset = true;

      rb.stale_display_info = true;
      rb.queueUpdateOutputs ();
    }

    if (! SK_IsGameWindowActive ())//game_window.active)
    {
      // Using a static kinda prevents us from supporting multiple windows,
      //   but it's good enough for now.
      static ULONG64 ulLastReset = 0;

      if (ulLastReset < SK_GetFramesDrawn () - 2)
      {
        auto *pLimiter =             rb.swapchain.p != nullptr
        ? SK::Framerate::GetLimiter (rb.swapchain.p) : nullptr;

        if (pLimiter != nullptr)
        {
          // Since this may have been the result of an output device change,
          //   it is best to take this opportunity to re-sync the limiter's
          //     clock versus VBLANK and flush the render queue.
          pLimiter->reset (true);

          ulLastReset = SK_GetFramesDrawn ();
        }
      }
    }
  }


#ifdef PARANOID_FOCUS_CHECK
  if (hWnd == game_window.hWnd)
  {
    static HWND hWndForeground =
      SK_GetForegroundWindow ();

    HWND hWndNewForeground =
      SK_GetForegroundWindow ();

    if (hWndForeground != hWndNewForeground)
    {
      if (game_window.hWnd == hWndNewForeground)
        ActivateWindow (game_window.hWnd, true);
      else
        ActivateWindow (game_window.hWnd, false);

      hWndForeground = hWndNewForeground;
    }
  }
#endif


  switch (uMsg)
  {
    case 0xf00f:
    {
      SleepEx (150UL, FALSE);

      extern VOID WINAPI
      SK_mouse_event (
        _In_ DWORD     dwFlags,
        _In_ DWORD     dx,
        _In_ DWORD     dy,
        _In_ DWORD     dwData,
        _In_ ULONG_PTR dwExtraInfo );

      if (SK_GetForegroundWindow () != game_window.hWnd)
      {
        HWND hWndStartMenu =
          FindWindow (L"Windows.UI.Core.CoreWindow", L"Start");

        DWORD                                        dwPidFg, dwPidStart;
        GetWindowThreadProcessId (SK_GetForegroundWindow (), &dwPidFg);
        GetWindowThreadProcessId (hWndStartMenu,             &dwPidStart);

        if (dwPidFg == dwPidStart)
        {
          static BYTE scan_code_ESC =
            (BYTE)MapVirtualKey (VK_ESCAPE, 0);

          SK_keybd_event (VK_ESCAPE, scan_code_ESC, 0,               0);
          SK_keybd_event (VK_ESCAPE, scan_code_ESC, KEYEVENTF_KEYUP, 0);

          SK_SleepEx (150, FALSE);
        }
      }


      POINT             orig = { };
      SK_GetCursorPos (&orig);

      POINT                              activation_pos = { 32, 32 };
      ClientToScreen (game_window.hWnd, &activation_pos);

      if (SK_GetForegroundWindow () != game_window.hWnd)
      {
        if (! GetConsoleWindow ())
        {
          AllocConsole ();
          SetWindowPos (GetConsoleWindow (), 0, 0, 0, 0, 0, SWP_NOZORDER);
          FreeConsole  ();

          SK_SleepEx (150, FALSE);
        }

        if (SK_SetCursorPos (activation_pos.x, activation_pos.y))
        {
          SK_mouse_event (MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, 0);
          SK_mouse_event (MOUSEEVENTF_MIDDLEUP,   0, 0, 0, 0);

          SK_SetCursorPos (orig.x, orig.y);
        }
      }

      BringWindowToTop    (game_window.hWnd);
      SetForegroundWindow (game_window.hWnd);
      SetFocus            (game_window.hWnd);

      return 0;
      }
      break;


    case WM_QUIT:
    case WM_CLOSE:
    {
      if (hWnd == game_window.hWnd || hWnd == game_window.child)
      {
        static auto game_id =
          SK_GetCurrentGameID ();

        if ( game_id == SK_GAME_ID::Yakuza0       ||
             game_id == SK_GAME_ID::YakuzaKiwami2 ||
             game_id == SK_GAME_ID::YakuzaUnderflow )
        {
          SK_SelfDestruct     (   );
          SK_TerminateProcess (0x0);
        }
      }
    } break;

    case WM_MOUSEMOVE:
      if (hWnd == game_window.hWnd || hWnd == game_window.child)
      {
        if (ImGui_WndProcHandler (hWnd, uMsg, wParam, lParam) != 0)
        {
          return 0;
        }
      }
      break;

    case WM_SETCURSOR:
    {
      if ((hWnd == game_window.hWnd || hWnd == game_window.child) && HIWORD (lParam) == WM_MOUSEMOVE)
      {
        if ( LOWORD (lParam) == HTCLIENT ||
             LOWORD (lParam) == HTTRANSPARENT )
        {
          if (ImGui_WndProcHandler (hWnd, uMsg, wParam, lParam) != 0)
          {
            extern bool      __SK_EnableSetCursor;
                   bool bOrig =
              std::exchange (__SK_EnableSetCursor, true);

            SK_ImGui_Cursor.activateWindow (true);

            __SK_EnableSetCursor = bOrig;

            // For a very long time now, we've been handling this message WRONG, return 1 stupid!
            return 1;
          }
        }
      }
    } break;

    case WM_TIMER:
    {
      if (hWnd == game_window.hWnd || hWnd == game_window.child)
      {
        if (ImGui_WndProcHandler (hWnd, uMsg, wParam, lParam) != 0)
        {
          return 0;
        }
      }
    } break;

    case WM_WINDOWPOSCHANGED:
    {
      if (hWnd == game_window.hWnd || hWnd == game_window.child)
      {
        SK_Window_RepositionIfNeeded ();
      }
    } break;

    case WM_SYSCOMMAND:
      if (ImGui_WndProcHandler (hWnd, uMsg, wParam, lParam) != 0)
      {
        return 0;
      }
      break;


    case WM_DEVICECHANGE:
    {
      bool bIgnore = true;

      switch (wParam)
      {
        case DBT_DEVICEARRIVAL:
        case DBT_DEVICEREMOVECOMPLETE:
        {
          const auto pDevHdr =
            (DEV_BROADCAST_HDR *)lParam;

          if (pDevHdr->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE)
          {
            const auto pDevW =
              (DEV_BROADCAST_DEVICEINTERFACE_W *)pDevHdr;

            static constexpr GUID GUID_DEVINTERFACE_HID     =
              { 0x4D1E55B2L, 0xF16F, 0x11CF, { 0x88, 0xCB, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30 } };

            static constexpr GUID GUID_XUSB_INTERFACE_CLASS =
              { 0xEC87F1E3L, 0xC13B, 0x4100, { 0xB5, 0xF7, 0x8B, 0x84, 0xD5, 0x42, 0x60, 0xCB } };

            if (IsEqualGUID (pDevW->dbcc_classguid, GUID_DEVINTERFACE_HID) ||
                IsEqualGUID (pDevW->dbcc_classguid, GUID_XUSB_INTERFACE_CLASS))
            {
              bIgnore = false;
            }
          }
        }

        default:
          bIgnore = false;
          break;
      }

      if (bIgnore)
      {
        SK_LOG0 ( ( L"WM_DEVICECHANGE received for non-gamepad device, "
                    L"hiding it from the game..." ), __SK_SUBSYSTEM__ );

        return IsWindowUnicode (hWnd) ?
                DefWindowProcW (hWnd, uMsg, wParam, lParam) :
                DefWindowProcA (hWnd, uMsg, wParam, lParam);
      }
    } break;


    case WM_DESTROY:
      if (hWnd == game_window.hWnd)
      {
        SK_LOG0 ( ( L"(?) Active window destroyed, our chicken has no head!" ),
                    __SK_SUBSYSTEM__ );
        game_window.hWnd   =    0;
        game_window.active = true; // The headless chicken appears very active...

        if (GetAncestor (hWnd, GA_ROOT) == game_window.hWnd)
        {
          SK_Win32_DestroyBackgroundWindow ();

          extern void SK_Inject_SetFocusWindow (HWND hWndFocus);
                      SK_Inject_SetFocusWindow (0);

          SK_ImGui_WantExit = true;

          if (config.compatibility.shutdown_on_window_close)
            SK_SelfDestruct ();
        }
      }
      break;

    case 0xfa57://WM_USER + WM_SETFOCUS:
      SK_Window_OnFocusChange ((HWND)wParam, (HWND)lParam);
      return 0; // Can't let the game see this, it will be confused
      break;

    case WM_SETFOCUS:
    {
      if (SK_Window_OnFocusChange (hWnd, (HWND)wParam))
      {
        ActivateWindow (hWnd, true, (HWND)wParam);

        // GeForce Experience Overlay is known to paradoxically set
        //   hWnd == wParam when it activates...
        if ( hWnd != (HWND)wParam &&
                           wParam != 0 )
        {
          ActivateWindow (
                     (HWND)wParam, false, hWnd
          );
        }
      }
    } break;


    case WM_KILLFOCUS:
    {
      if (SK_Window_OnFocusChange ((HWND)wParam, hWnd))
      {
      ////SK_ReleaseAssert (hWnd   == game_window.hWnd ||
      ////                  wParam != (WPARAM)nullptr );

        ActivateWindow   (hWnd, false, (HWND)wParam);

        if (   nullptr != (HWND)wParam)
          ActivateWindow ((HWND)wParam, true, hWnd);

        if ( hWnd   == game_window.hWnd ||
             wParam != 0 )
        {
          rb.fullscreen_exclusive = false;

          if (SK_WantBackgroundRender ())
          {
            // Blocking this message helps with many games that
            //   mute audio in the background
            return
              game_window.DefWindowProc ( hWnd, uMsg,
                                            wParam, lParam );
          }
        }
      }
    } break;

    // Ignore (and physically remove) this event from the message queue if background_render = true
    case WM_MOUSEACTIVATE:
    {
      if ( reinterpret_cast <HWND> (wParam) == game_window.hWnd )
      {
        if ((! rb.fullscreen_exclusive) && SK_WantBackgroundRender ())
        {
          SK_LOG2 ( ( L"WM_MOUSEACTIVATE ==> Activate and Eat" ),
                   L"Window Mgr" );

          if (! SK_ImGui_WantMouseCapture ())
            return MA_ACTIVATE;       // We don't want it, and the game doesn't expect it
          else
            return MA_ACTIVATEANDEAT; // We want it, game doesn't need it
        }
      }

      else
      {
        // Game window was deactivated, but the game doesn't need to know this!
        //   in fact, it needs to be told the opposite.
        if ((! rb.fullscreen_exclusive) && SK_WantBackgroundRender ())
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
      if (uMsg == WM_NCACTIVATE && config.window.borderless)
      {
        return
          DefWindowProcW (hWnd, uMsg, wParam, -1);
      }

      if (  uMsg == WM_NCACTIVATE ||
            uMsg == WM_ACTIVATEAPP   )
      {
        if (uMsg == WM_NCACTIVATE)
        {
          if (wParam != FALSE)
          {
            if (! last_active)
              SK_LOG3 ( ( L"Application Activated (Non-Client)" ),
                          L"Window Mgr" );

            if (SK_WantBackgroundRender ())
              SK_DetourWindowProc ( hWnd, WM_SETFOCUS, (WPARAM)nullptr, (LPARAM)nullptr );
          }

          else
          {
            if (last_active)
              SK_LOG3 ( ( L"Application Deactivated (Non-Client)" ),
                          L"Window Mgr" );

            if (  (! rb.fullscreen_exclusive) &&
                    SK_WantBackgroundRender ()
                )
            {
              game_window.DefWindowProc ( hWnd, uMsg,
                                            wParam, lParam );

              SK_COMPAT_SafeCallProc (&game_window,
                hWnd, uMsg, TRUE, lParam
              );

              SK_DetourWindowProc ( hWnd, WM_KILLFOCUS, (WPARAM)nullptr, (LPARAM)nullptr );

              HWND hWndForeground =
                SK_GetForegroundWindow ();

              // Only block the message if we're transferring activation to a different app
              if (hWnd != hWndForeground && (! IsChild (hWnd, hWndForeground)))
                return 0;

              return 1;
            }
          }
        }

        else
        {
          GUITHREADINFO gti        =        {   };
                        gti.cbSize = sizeof (gti);
          GetGUIThreadInfo ((DWORD)lParam,  &gti);

          ActivateWindow (hWnd, wParam, gti.hwndActive);

          if (wParam == FALSE)
          {
            if (  (! rb.fullscreen_exclusive) &&
                    SK_WantBackgroundRender ()
                )
            {
              game_window.DefWindowProc ( hWnd, uMsg,
                                            wParam, lParam );

              SK_COMPAT_SafeCallProc (&game_window,
                hWnd, uMsg, TRUE, 0
              );

              SK_DetourWindowProc ( hWnd, WM_KILLFOCUS, (WPARAM)nullptr, (LPARAM)nullptr );

              return 0;
            }
          }

          else if (SK_WantBackgroundRender ())
            SK_DetourWindowProc ( hWnd, WM_SETFOCUS, (WPARAM)nullptr, (LPARAM)nullptr );
        }
      }

      else //if (uMsg == WM_ACTIVATE)
      {
        const wchar_t* source   = L"UNKKNOWN";
        bool           activate = false;

        switch (LOWORD (wParam))
        {
          case WA_ACTIVE:
          case WA_CLICKACTIVE:
          default: // Unknown
          {
            activate = (                lParam  == 0                 ) ||
             ( reinterpret_cast <HWND> (lParam) != game_window.hWnd  );
            source   = LOWORD (wParam) == 1 ? L"(WM_ACTIVATE [ WA_ACTIVE ])" :
                                              L"(WM_ACTIVATE [ WA_CLICKACTIVE ])";
          } break;

          case WA_INACTIVE:
          {
            activate =
              ( reinterpret_cast <HWND> (lParam) == game_window.hWnd );
            source   = L"(WM_ACTIVATE [ WA_INACTIVE ])";
          } break;
        }

        if (last_active)
        {
          if (! activate)
          {
            ActivateWindow (hWnd, activate);

            SK_LOG2 ( ( L"Application Deactivated %s", source ),
                        L"Window Mgr" );
          }
        }

        else
        {
          if (activate)
          {
            ActivateWindow (hWnd, activate);

            SK_LOG2 ( ( L"Application Activated %s", source ),
                        L"Window Mgr" );
          }
        }

        if ( (! rb.fullscreen_exclusive) && SK_WantBackgroundRender ())
        {
          if (! activate)
          {
            game_window.DefWindowProc ( hWnd, uMsg,
                                        wParam, lParam );

            SK_DetourWindowProc ( hWnd, WM_KILLFOCUS, (WPARAM)nullptr, (LPARAM)nullptr );

            return 1;
          }

          else
            SK_DetourWindowProc ( hWnd, WM_SETFOCUS, (WPARAM)nullptr, (LPARAM)nullptr );
        }
      }
    }
    break;

    case WM_NCUAHDRAWCAPTION:
	  case WM_NCUAHDRAWFRAME:
      if ( config.window.borderless ||
           rb.fullscreen_exclusive ) return 0;
      break;

    case WM_NCPAINT:
      if ( config.window.borderless ||
           rb.fullscreen_exclusive ) return 0;
      break;

    case WM_NCCALCSIZE:
      if (wParam == TRUE)
      {
        if ( config.window.borderless ||
             rb.fullscreen_exclusive )
        {
          return 0;
        }
      }
      break;

    case WM_SHOWWINDOW:
    {
      ActivateWindow (hWnd, wParam == TRUE);

      if ( SK_IsGameWindowActive (    ) || SK_WantBackgroundRender () )
           SK_XInput_Enable      (TRUE);

      if (wParam == FALSE)
      {
        if ( SK_WantBackgroundRender () &&
             (! rb.fullscreen_exclusive)        )
        {
          // What purpose does this serve?
          //
          game_window.DefWindowProc ( hWnd, uMsg,
                                        wParam, lParam );
          return 0;
        }
      }
    } break;
  }

  bool handled = false;



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
  if (uMsg == WM_MOUSEWHEEL && (! recursive_wheel))
  {        handled =
             ImGui_WndProcHandler (hWnd, uMsg, wParam, lParam);

    if ((! handled) && config.input.mouse.fix_synaptics)
    {
      INPUT input        = { };

      input.type         = INPUT_MOUSE;
      input.mi.dwFlags   = MOUSEEVENTF_WHEEL;
      input.mi.mouseData = GET_WHEEL_DELTA_WPARAM (wParam);

      recursive_wheel    = true;

      SK_SendInput (1, &input, sizeof (INPUT));
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
  if (hWnd == game_window.hWnd)
  {
    if (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST && SK_ImGui_WantMouseCapture ())
    {
      game_window.DefWindowProc (hWnd, uMsg, wParam, lParam);
      return 0;
    }

    if (uMsg >= WM_KEYFIRST   && uMsg <= WM_KEYLAST   && SK_ImGui_WantKeyboardCapture ())
    {
      if (uMsg != WM_KEYUP && uMsg != WM_SYSKEYUP)
      {
        game_window.DefWindowProc (hWnd, uMsg, wParam, lParam);
        return 0;
      }
    }
  }

  //
  // DO NOT HOOK THIS FUNCTION outside of SpecialK plug-ins, the ABI is not guaranteed
  //
  if (SK_DetourWindowProc2 (hWnd, uMsg, wParam, lParam) != 0)
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

  else
  {
    return
      game_window.DefWindowProc (hWnd, uMsg, wParam, lParam);
  }


  // Filter this out for fullscreen override safety
  //if (uMsg == WM_DISPLAYCHANGE)    return game_window.DefWindowProc (hWnd, uMsg, wParam, lParam);
  //if (uMsg == WM_WINDOWPOSCHANGED) return game_window.DefWindowProc (hWnd, uMsg, wParam, lParam);


  const LRESULT lRet =
    SK_COMPAT_SafeCallProc (&game_window, hWnd, uMsg, wParam, lParam);


  // Post-Process the game's result to fix any non-compliant behaviors
  //

  if (hWnd == game_window.hWnd)
  {
    // Fix for Skyrim SE beeping when Alt is pressed.
    if (uMsg == WM_MENUCHAR && (HIWORD (lRet) == MNC_IGNORE))
      return MAKEWPARAM (0, MNC_CLOSE);
  }


  return lRet;
}

bool
SK_Window_IsTopMost (HWND hWnd = game_window.hWnd)
{
  WINDOWINFO winfo        = { };
             winfo.cbSize = sizeof (WINDOWINFO);
  SK_GetWindowInfo (
    hWnd,   &winfo
  );

  return
    ( winfo.dwExStyle & WS_EX_TOPMOST );
}

void
SK_Window_SetTopMost (bool bTop, bool bBringToTop, HWND hWnd)
{
  bool _unicode =
    IsWindowUnicode (hWnd);

  auto               SetWindowLongFn =
    ( _unicode ? &SK_SetWindowLongW  :
                 &SK_SetWindowLongA  );

  auto              GetWindowLongFn =
   ( _unicode ? &SK_GetWindowLongW  :
                &SK_GetWindowLongA  );

  HWND      hWndOrder = nullptr;
  DWORD_PTR dwStyleEx =
    GetWindowLongFn (hWnd, GWL_EXSTYLE);
  DWORD_PTR dwStyleExOrig = dwStyleEx;

  extern HWND SK_Inject_GetFocusWindow (void);
  extern void SK_Inject_SetFocusWindow (HWND hWndFocus);

  SK_Inject_SetFocusWindow (game_window.hWnd);

  if (bTop)
  {
    dwStyleEx |=   WS_EX_TOPMOST;
    dwStyleEx &= ~(WS_EX_NOACTIVATE);
    hWndOrder  =   HWND_TOPMOST;
  }

  else
  {
    dwStyleEx &= ~(WS_EX_TOPMOST | WS_EX_NOACTIVATE);
    hWndOrder  =  HWND_NOTOPMOST;
  }

  if (dwStyleEx != dwStyleExOrig)
  {
    SetWindowLongFn ( hWnd, GWL_EXSTYLE, (LONG)dwStyleEx );
  }

  SK_SetWindowPos ( hWnd,
                    hWndOrder,
                    0, 0, 0, 0,
                    SWP_NOACTIVATE     | SWP_NOMOVE         |
                    SWP_NOSIZE         | SWP_NOSENDCHANGING |
                    SWP_ASYNCWINDOWPOS );

  if (bBringToTop)
  {
    SK_SetWindowPos ( hWnd,
                      HWND_TOP,
                      0, 0, 0, 0,
                      SWP_NOACTIVATE     | SWP_NOMOVE         |
                      SWP_NOSIZE         | SWP_NOSENDCHANGING |
                      SWP_ASYNCWINDOWPOS );
  }
}

void
SK_InitWindow (HWND hWnd, bool fullscreen_exclusive)
{
  if (game_window.GetWindowLongPtr == nullptr)
  {
    return SK_InstallWindowHook (hWnd);
  }


  SK_GetWindowRect (hWnd, &game_window.game.window);
  SK_GetClientRect (hWnd, &game_window.game.client);

  CopyRect (&game_window.actual.window, &game_window.game.window);
  CopyRect (&game_window.actual.client, &game_window.game.client);

  SK_GetCursorPos  (      &game_window.cursor_pos);


  // Make sure any pending changes are finished before querying the
  //   actual value
  SK_Window_WaitForAsyncSetWindowLong ();


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


  static auto& rb =
    SK_GetCurrentRenderBackend ();

  auto& windows =
     rb.windows;

  windows.setFocus  (hWnd);
  windows.setDevice (
    windows.device != nullptr ?
         (HWND)windows.device : hWnd);

  if (! fullscreen_exclusive)
  {
    // Next, adjust the border and/or window location if the user
    //   wants an override
    SK_Window_RepositionIfNeeded ();
  }
}


using GetDpiForSystem_pfn               = UINT (WINAPI *)(void);
using GetDpiForWindow_pfn               = UINT (WINAPI *)(HWND   hwnd);
using EnableNonClientDpiScaling_pfn     = BOOL (WINAPI *)(HWND   hwnd);
using GetSystemDpiForProcess_pfn        = UINT (WINAPI *)(HANDLE hProcess);
using GetSystemMetricsForDpi_pfn        = int  (WINAPI *)(int    nIndex,
                                                          UINT   dpi);
using SystemParametersInfoForDpi_pfn    = BOOL (WINAPI *)(UINT   uiAction,
                                                          UINT   uiParam,
                                                          PVOID  pvParam,
                                                          UINT   fWinIni,
                                                          UINT   dpi);
using SetThreadDpiHostingBehavior_pfn   =
                          DPI_HOSTING_BEHAVIOR (WINAPI *)(DPI_HOSTING_BEHAVIOR  value);
using SetThreadDpiAwarenessContext_pfn  =
                         DPI_AWARENESS_CONTEXT (WINAPI *)(DPI_AWARENESS_CONTEXT dpiContext);
using GetThreadDpiAwarenessContext_pfn  =
                         DPI_AWARENESS_CONTEXT (WINAPI *)(void);
using GetAwarenessFromDpiAwarenessContext_pfn =
                         DPI_AWARENESS         (WINAPI *)(DPI_AWARENESS_CONTEXT value);
using SetProcessDpiAwarenessContext_pfn = BOOL (WINAPI *)(DPI_AWARENESS_CONTEXT value);

using AdjustWindowRectExForDpi_pfn      = BOOL (WINAPI *)(LPRECT lpRect,
                                                          DWORD  dwStyle,
                                                          BOOL   bMenu,
                                                          DWORD  dwExStyle,
                                                          UINT   dpi);

GetDpiForSystem_pfn                GetDpiForSystem_Original               = nullptr;
GetDpiForWindow_pfn                GetDpiForWindow_Original               = nullptr;
GetSystemDpiForProcess_pfn         GetSystemDpiForProcess_Original        = nullptr;
GetSystemMetricsForDpi_pfn         GetSystemMetricsForDpi_Original        = nullptr;
AdjustWindowRectExForDpi_pfn       AdjustWindowRectExForDpi_Original      = nullptr;
EnableNonClientDpiScaling_pfn      EnableNonClientDpiScaling_Original     = nullptr;
SystemParametersInfoForDpi_pfn     SystemParametersInfoForDpi_Original    = nullptr;
SetThreadDpiHostingBehavior_pfn    SetThreadDpiHostingBehavior_Original   = nullptr;
SetThreadDpiAwarenessContext_pfn   SetThreadDpiAwarenessContext_Original  = nullptr;
SetProcessDpiAwarenessContext_pfn  SetProcessDpiAwarenessContext_Original = nullptr;

GetThreadDpiAwarenessContext_pfn        GetThreadDpiAwarenessContext        = nullptr;
GetAwarenessFromDpiAwarenessContext_pfn GetAwarenessFromDpiAwarenessContext = nullptr;

DPI_AWARENESS_CONTEXT
SK_GetThreadDpiAwarenessContext (void)
{
  if (GetThreadDpiAwarenessContext == nullptr)
  {
    // TODO: This isn't right...
    return DPI_AWARENESS_CONTEXT_UNAWARE;
  }

  return
    GetThreadDpiAwarenessContext ();
}

DPI_AWARENESS
SK_GetThreadDpiAwareness (void)
{
  if ( GetAwarenessFromDpiAwarenessContext == nullptr ||
       GetThreadDpiAwarenessContext        == nullptr )
  {
    // TODO: This isn't right...
    return DPI_AWARENESS_UNAWARE;
  }

  return
    GetAwarenessFromDpiAwarenessContext (
      SK_GetThreadDpiAwarenessContext ()
    );
}

UINT
WINAPI
GetDpiForSystem_Detour (void)
{
  //SetThreadDpiAwarenessContext_Original  (DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
  //SetProcessDpiAwarenessContext_Original (DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

  SK_LOG_FIRST_CALL

  return
    GetDpiForSystem_Original ();
}

UINT
WINAPI
GetDpiForWindow_Detour (HWND hwnd)
{
  if (config.dpi.per_monitor.aware_on_all_threads)
  {
    SetThreadDpiAwarenessContext_Original  (DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    SetProcessDpiAwarenessContext_Original (DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
  }

  if (config.dpi.disable_scaling)
  {
    //SetThreadDpiAwarenessContext_Original  (DPI_AWARENESS_CONTEXT_UNAWARE);
    //SetProcessDpiAwarenessContext_Original (DPI_AWARENESS_CONTEXT_UNAWARE);
  }

  SK_LOG_FIRST_CALL

  return
    GetDpiForWindow_Original (hwnd);
}

BOOL
WINAPI
EnableNonClientDpiScaling_Detour (HWND hwnd)
{
  if (config.dpi.per_monitor.aware_on_all_threads)
  {
    SetThreadDpiAwarenessContext_Original  (DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    SetProcessDpiAwarenessContext_Original (DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
  }

  if (config.dpi.disable_scaling)
  {
    //SetThreadDpiAwarenessContext_Original  (DPI_AWARENESS_CONTEXT_UNAWARE);
    //SetProcessDpiAwarenessContext_Original (DPI_AWARENESS_CONTEXT_UNAWARE);
  }

  SK_LOG_FIRST_CALL

  return
    EnableNonClientDpiScaling_Original (hwnd);
}

UINT
WINAPI
GetSystemDpiForProcess_Detour (HANDLE hProcess)
{
  SK_LOG_FIRST_CALL

  return
    GetSystemDpiForProcess_Original (hProcess);
}

int
WINAPI
GetSystemMetricsForDpi_Detour (int nIndex, UINT dpi)
{
  if (config.dpi.per_monitor.aware_on_all_threads)
  {
    SetThreadDpiAwarenessContext_Original  (DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    SetProcessDpiAwarenessContext_Original (DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
  }

  if (config.dpi.disable_scaling)
  {
    //SetThreadDpiAwarenessContext_Original  (DPI_AWARENESS_CONTEXT_UNAWARE);
    //SetProcessDpiAwarenessContext_Original (DPI_AWARENESS_CONTEXT_UNAWARE);
  }


  SK_LOG_FIRST_CALL

  //if (config.window.borderless)
  //{
  //  if (nIndex == SM_CYCAPTION)
  //    return 0;
  //  if (nIndex == SM_CYMENU)
  //    return 0;
  //  if (nIndex == SM_CXBORDER)
  //    return 0;
  //  if (nIndex == SM_CYBORDER)
  //    return 0;
  //  if (nIndex == SM_CXDLGFRAME)
  //    return 0;
  //  if (nIndex == SM_CYDLGFRAME)
  //    return 0;
  //  if (nIndex == SM_CXFRAME)
  //    return 0;
  //  if (nIndex == SM_CYFRAME)
  //    return 0;
  //  if (nIndex == SM_CXSIZE)
  //    return 0;
  //  if (nIndex == SM_CYSIZE)
  //    return 0;
  //  if (nIndex == SM_CXPADDEDBORDER)
  //    return 0;
  //}

  //dll_log->Log (L" SysMetrics [ %li ] - <DPI: %lu>", nIndex, dpi);
  //dll_log->Log (L" >> %li", GetSystemMetrics (nIndex));

  return
    //GetSystemMetrics (nIndex);
    GetSystemMetricsForDpi_Original (nIndex, dpi);
}

int
WINAPI
SK_GetSystemMetricsForDefaultDpi (_In_ int nIndex)
{
  if (GetSystemMetricsForDpi_Original != nullptr)
    return GetSystemMetricsForDpi_Original (nIndex, USER_DEFAULT_SCREEN_DPI);

  if (GetSystemMetrics_Original != nullptr)
    return GetSystemMetrics_Original (nIndex);

  return
    SK_GetSystemMetrics (nIndex);

  //SK_LOG_FIRST_CALL
}


BOOL
WINAPI
SystemParametersInfoForDpi_Detour ( UINT  uiAction,
                                    UINT  uiParam,
                                    PVOID pvParam,
                                    UINT  fWinIni,
                                    UINT  dpi )
{
  if (config.dpi.per_monitor.aware_on_all_threads)
  {
    SetThreadDpiAwarenessContext_Original  (DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    SetProcessDpiAwarenessContext_Original (DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
  }

  if (config.dpi.disable_scaling)
  {
    //SetThreadDpiAwarenessContext_Original  (DPI_AWARENESS_CONTEXT_UNAWARE);
    //SetProcessDpiAwarenessContext_Original (DPI_AWARENESS_CONTEXT_UNAWARE);
  }

  SK_LOG_FIRST_CALL

  return
    SystemParametersInfoForDpi_Original (
      uiAction, uiParam,
                pvParam, fWinIni, dpi
    );
}

DPI_HOSTING_BEHAVIOR
WINAPI
SetThreadDpiHostingBehavior_Detour (DPI_HOSTING_BEHAVIOR value)
{
  if (config.dpi.per_monitor.aware_on_all_threads)
  {
    SetThreadDpiAwarenessContext_Original  (DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    SetProcessDpiAwarenessContext_Original (DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
  }

  if (config.dpi.disable_scaling)
  {
    //SetThreadDpiAwarenessContext_Original  (DPI_AWARENESS_CONTEXT_UNAWARE);
    //SetProcessDpiAwarenessContext_Original (DPI_AWARENESS_CONTEXT_UNAWARE);
  }


  SK_LOG_FIRST_CALL

  return
    SetThreadDpiHostingBehavior_Original (value);
}

DPI_AWARENESS_CONTEXT
WINAPI
SetThreadDpiAwarenessContext_Detour (DPI_AWARENESS_CONTEXT dpiContext)
{
  SK_LOG_FIRST_CALL

  return
    SetThreadDpiAwarenessContext_Original (dpiContext);
}

BOOL
WINAPI
SetProcessDpiAwarenessContext_Detour (DPI_AWARENESS_CONTEXT value)
{
  SK_LOG_FIRST_CALL

  return
    SetProcessDpiAwarenessContext_Original (value);
}

BOOL
WINAPI
AdjustWindowRectExForDpi_Detour ( LPRECT lpRect,
                                  DWORD  dwStyle,
                                  BOOL   bMenu,
                                  DWORD  dwExStyle,
                                  UINT   dpi )
{
  if (config.dpi.per_monitor.aware_on_all_threads)
  {
    SetThreadDpiAwarenessContext_Original  (DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    SetProcessDpiAwarenessContext_Original (DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
  }

  if (config.dpi.disable_scaling)
  {
    //SetThreadDpiAwarenessContext_Original  (DPI_AWARENESS_CONTEXT_UNAWARE);
    //SetProcessDpiAwarenessContext_Original (DPI_AWARENESS_CONTEXT_UNAWARE);
  }


  SK_LOG_FIRST_CALL

  return
    AdjustWindowRectExForDpi_Original ( lpRect, dwStyle,
                                        bMenu,  dwExStyle,
                                        dpi );
}

bool
SK_Win32_IsDummyWindowClass (WNDCLASSEXW* pWindowClass)
{
  if (! pWindowClass)
    return false;

  const bool dummy_window =
    StrStrIW (pWindowClass->lpszClassName, L"Kiero DirectX Window")                  || // CyberEngine
    StrStrIW (pWindowClass->lpszClassName, L"RTSSWndClass")                          || // RTSS
    StrStrIW (pWindowClass->lpszClassName, L"Special K Dummy Window Class")          || // ... that's us!
    StrStrIW (pWindowClass->lpszClassName, L"EOSOVHDummyWindowClass")                || // Epic Online Store Overlay
    StrStrIW (pWindowClass->lpszClassName, L"CurseOverlayTemporaryDirect3D11Window") || // Twitch

    // F' it, there's a pattern here, just ignore all dummies.
    StrStrIW (pWindowClass->lpszClassName, L"dummy");

  if (StrStrIW (pWindowClass->lpszClassName, L"Qt"))
    return false;

  return
    dummy_window;
}

bool
SK_Win32_IsDummyWindowClass (HWND hWndInstance)
{
  WNDCLASSEXW wnd_class          = {};
  wchar_t     wszClassName [128] = {};
  HINSTANCE   hInstance          =
    reinterpret_cast <HINSTANCE> (
#ifndef _WIN64
      GetWindowLongW    (hWndInstance,  GWL_HINSTANCE)
#else
      GetWindowLongPtrW (hWndInstance, GWLP_HINSTANCE)
#endif
    );

  if (RealGetWindowClassW (hWndInstance, wszClassName, 127) > 0)
  {
    wnd_class.cbSize = sizeof (WNDCLASSEXW);

    if (GetClassInfoExW (hInstance, wszClassName, &wnd_class))
    {
      return SK_Win32_IsDummyWindowClass (&wnd_class);
    }
  }

  return false; // Indeterminate
}

void
SK_InstallWindowHook (HWND hWnd)
{
  static constexpr int                 _MaxClassLen  = 128;
  wchar_t                    wszClass [_MaxClassLen] = { };
  RealGetWindowClassW (hWnd, wszClass, _MaxClassLen - 1);

  const bool dummy_window =
    SK_Win32_IsDummyWindowClass (hWnd);

  if (dummy_window != false)
    return;


  DWORD                            dwWindowPid = 0;
  GetWindowThreadProcessId (hWnd, &dwWindowPid);

  //
  // If our attempted target belongs to a different process,
  //   it's a trap!
  //
  //     >> Or at least, do not trap said processes input.
  //
  if (dwWindowPid != GetCurrentProcessId ())
    return;


  // Game's window still exists, so, uh... ignore this?
  if (game_window.hWnd != 0 && IsWindow (game_window.hWnd))
  {
    game_window.parent =
      GetAncestor (hWnd, GA_PARENT);

    //return;
  }


  if (config.system.log_level > 0)
    SK_ReleaseAssert (game_window.hWnd == 0 || game_window.hWnd == GetAncestor (hWnd, GA_ROOT));


  if (SK_IsAddressExecutable (game_window.WndProc_Original, true))
    return;

  auto hModUser32 =
    SK_GetModuleHandle (L"user32");

  static volatile LONG               __installed =      FALSE;
  if (! InterlockedCompareExchange (&__installed, TRUE, FALSE))
  {
    // Win32u is faster on systems that dispatch system calls through it
    //
    static sk_import_test_s win32u_test [] = { { "win32u.dll", false } };
    static bool             tested         =                   false;

    if (! tested)
    {
      SK_TestImports (hModUser32, win32u_test,
                          sizeof (win32u_test) /
                          sizeof (sk_import_test_s));
      tested = true;
    }

    GetThreadDpiAwarenessContext = (GetThreadDpiAwarenessContext_pfn)
      SK_GetProcAddress ( hModUser32,
                         "GetThreadDpiAwarenessContext" );

    GetAwarenessFromDpiAwarenessContext = (GetAwarenessFromDpiAwarenessContext_pfn)
      SK_GetProcAddress ( hModUser32,
                  "GetAwarenessFromDpiAwarenessContext" );

    SK_CreateDLLHook2 ( L"user32", "GetDpiForSystem",               GetDpiForSystem_Detour,               static_cast_p2p <void> (&GetDpiForSystem_Original)               );
    SK_CreateDLLHook2 ( L"user32", "GetDpiForWindow",               GetDpiForWindow_Detour,               static_cast_p2p <void> (&GetDpiForWindow_Original)               );
    SK_CreateDLLHook2 ( L"user32", "GetSystemDpiForProcess",        GetSystemDpiForProcess_Detour,        static_cast_p2p <void> (&GetSystemDpiForProcess_Original)        );
    SK_CreateDLLHook2 ( L"user32", "GetSystemMetricsForDpi",        GetSystemMetricsForDpi_Detour,        static_cast_p2p <void> (&GetSystemMetricsForDpi_Original)        );
    SK_CreateDLLHook2 ( L"user32", "AdjustWindowRectExForDpi",      AdjustWindowRectExForDpi_Detour,      static_cast_p2p <void> (&AdjustWindowRectExForDpi_Original)      );
    SK_CreateDLLHook2 ( L"user32", "EnableNonClientDpiScaling",     EnableNonClientDpiScaling_Detour,     static_cast_p2p <void> (&EnableNonClientDpiScaling_Original)     );
    SK_CreateDLLHook2 ( L"user32", "SystemParametersInfoForDpi",    SystemParametersInfoForDpi_Detour,    static_cast_p2p <void> (&SystemParametersInfoForDpi_Original)    );
    SK_CreateDLLHook2 ( L"user32", "SetThreadDpiHostingBehavior",   SetThreadDpiHostingBehavior_Detour,   static_cast_p2p <void> (&SetThreadDpiHostingBehavior_Original)   );
    SK_CreateDLLHook2 ( L"user32", "SetThreadDpiAwarenessContext",  SetThreadDpiAwarenessContext_Detour,  static_cast_p2p <void> (&SetThreadDpiAwarenessContext_Original)  );
    SK_CreateDLLHook2 ( L"user32", "SetProcessDpiAwarenessContext", SetProcessDpiAwarenessContext_Detour, static_cast_p2p <void> (&SetProcessDpiAwarenessContext_Original) );



    // When ImGui is active, we will do character translation internally
    //   for any Raw Input or Win32 key down event -- disable Windows'
    //     translation from WM_KEYDOWN to WM_CHAR while we are doing this.
    //
    SK_CreateDLLHook2 (      L"user32",
                              "TranslateMessage",
                               TranslateMessage_Detour,
      static_cast_p2p <void> (&TranslateMessage_Original) );

    SK_CreateDLLHook2 (      L"user32",
                              "PeekMessageA",
                               PeekMessageA_Detour,
      static_cast_p2p <void> (&PeekMessageA_Original) );

    SK_CreateDLLHook2 (      L"user32",
                              "PeekMessageW",
                               PeekMessageW_Detour,
      static_cast_p2p <void> (&PeekMessageW_Original) );

    SK_CreateDLLHook2 (      L"user32",
                              "GetMessageA",
                               GetMessageA_Detour,
      static_cast_p2p <void> (&GetMessageA_Original) );

    SK_CreateDLLHook2 (      L"user32",
                              "GetMessageW",
                               GetMessageW_Detour,
      static_cast_p2p <void> (&GetMessageW_Original) );


    game_window.WndProc_Original = nullptr;
  }

  if (ReadAcquire (&__SK_Init) > 0)
  {
    SK_ApplyQueuedHooks ();
  }


  // Not sure why this was conditional upon 1 frame drawn, but NOT
  //   continuing beyond this point w/ no frames drawn will crash Disgaea PC
#if 0
  extern bool
         SK_GL_OnD3D11;
  if ((! SK_GL_OnD3D11) && (! SK_GetFramesDrawn ()))
    return;
#endif


  if (config.render.framerate.enable_mmcss)
    SK_DWM_EnableMMCSS (config.render.framerate.enable_mmcss);


  game_window.unicode =
    SK_Window_IsUnicode (hWnd) != FALSE;
  game_window.parent  =
    GetAncestor         (hWnd, GA_PARENT);


  if (game_window.unicode)
  {
    game_window.GetWindowLongPtr = SK_RunLHIfBitness ( 64, SK_GetWindowLongPtrW,
                  reinterpret_cast <GetWindowLongPtr_pfn> (SK_GetWindowLongW)    );

    game_window.SetWindowLongPtr = SK_SetWindowLongPtrW;
    game_window.DefWindowProc    = (DefWindowProc_pfn)
                 SK_GetProcAddress (hModUser32,
                                    "DefWindowProcW");
    game_window.CallWindowProc   = (CallWindowProc_pfn)
                 SK_GetProcAddress (hModUser32,
                                    "CallWindowProcW" );
  }

  else
  {
    game_window.GetWindowLongPtr = SK_RunLHIfBitness ( 64, SK_GetWindowLongPtrA,
                  reinterpret_cast <GetWindowLongPtr_pfn> (SK_GetWindowLongA)    );

    game_window.SetWindowLongPtr = SK_SetWindowLongPtrA;
    game_window.DefWindowProc    = (DefWindowProc_pfn)
                 SK_GetProcAddress (hModUser32,
                                    "DefWindowProcA");
    game_window.CallWindowProc   = (CallWindowProc_pfn)
                 SK_GetProcAddress (hModUser32,
                                    "CallWindowProcA" );
  }



  WNDPROC class_proc = game_window.unicode   ? (WNDPROC)
    GetClassLongPtrW  ( hWnd, GCLP_WNDPROC ) : (WNDPROC)
    GetClassLongPtrA  ( hWnd, GCLP_WNDPROC );

  auto wnd_proc =
    (WNDPROC)(game_window.GetWindowLongPtr (hWnd, GWLP_WNDPROC));


  void SK_MakeWindowHook (   WNDPROC,  WNDPROC, HWND);
       SK_MakeWindowHook (class_proc, wnd_proc, hWnd);

  if ( game_window.WndProc_Original != nullptr )
  {
          bool  has_raw_mouse = false;
          UINT  count         = 0;

    const DWORD dwLast        = GetLastError ();

    SK_GetRegisteredRawInputDevices (
      nullptr, &count, sizeof (RAWINPUTDEVICE)
    );

    if (count > 0)
    {
      SK_TLS *pTLS =
        SK_TLS_Bottom ();

      auto* pDevs =
        pTLS->raw_input->allocateDevices (
          static_cast <size_t> (count) + 1
        );

      SK_GetRegisteredRawInputDevices ( pDevs,
                                       &count,
                                       sizeof (RAWINPUTDEVICE) );

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
    }

    // If no pointing device is setup, go ahead and register our own mouse;
    //   it won't interfere with the game's operation until the ImGui overlay
    //     requires a mouse... at which point third-party software will flip out :)
    ///if (! has_raw_mouse)
    ///{
    ///  RAWINPUTDEVICE rid;
    ///  rid.hwndTarget  = hWnd;
    ///  rid.usUsage     = HID_USAGE_GENERIC_MOUSE;
    ///  rid.usUsagePage = HID_USAGE_PAGE_GENERIC;
    ///  rid.dwFlags     = RIDEV_DEVNOTIFY;
    ///
    ///  SK_RegisterRawInputDevices (&rid, 1, sizeof RAWINPUTDEVICE);
    ///}

    SK_SetLastError (dwLast);

    static SK_RenderBackend& rb =
      SK_GetCurrentRenderBackend ();

    rb.windows.setFocus  (hWnd);
    rb.windows.setDevice (rb.windows.device ?
                           game_window.hWnd : hWnd);

    SK_InitWindow (hWnd);


    // Fix the cursor clipping rect if needed so that we can use Special K's
    //   menu system.
    RECT                 cursor_clip = { };
    if ( GetClipCursor (&cursor_clip) )
    {
      // Run it through the hooked function, which will transform certain
      //   attributes as needed.
      ClipCursor (&cursor_clip);
    }

    gsl::not_null <SK_ICommandProcessor*> cmd (
      SK_GetCommandProcessor ()
    );

    cmd->AddVariable ("Cursor.Manage",           SK_CreateVar (SK_IVariable::Boolean, (bool *)&config.input.cursor.manage));
    cmd->AddVariable ("Cursor.Timeout",          SK_CreateVar (SK_IVariable::Int,     (int  *)&config.input.cursor.timeout));
    cmd->AddVariable ("Cursor.KeysActivate",     SK_CreateVar (SK_IVariable::Boolean, (bool *)&config.input.cursor.keys_activate));

    cmd->AddVariable ("Window.BackgroundRender", SK_CreateVar (SK_IVariable::Boolean, (bool *)&config.window.background_render));

    cmd->AddVariable ("ImGui.Visible",           SK_CreateVar (SK_IVariable::Boolean, (bool *)&SK_ImGui_Visible));
  }
}

HWND
__stdcall
SK_GetGameWindow (void)
{
  ////SK_WINDOW_LOG_CALL3 ();

  return game_window.hWnd;
}




void
SK_MakeWindowHook (WNDPROC class_proc, WNDPROC wnd_proc, HWND hWnd)
{
  hWnd = GetAncestor (hWnd, GA_ROOT);

  wchar_t wszClassName [128] = { };
  wchar_t wszTitle     [128] = { };

  InternalGetWindowText ( hWnd, wszTitle,     127 );
  RealGetWindowClassW   ( hWnd, wszClassName, 127 );

  dll_log->Log ( L"[Window Mgr] Hooking the Window Procedure for "
                 L"%s Window Class ('%s' - \"%s\" * %x)",
                 game_window.unicode ? L"Unicode" : L"ANSI",
                 wszClassName, wszTitle, hWnd );

  dll_log->Log ( L"[Window Mgr]  $ ClassProc:  %s",
                SK_MakePrettyAddress (class_proc).c_str () );
  dll_log->Log ( L"[Window Mgr]  $ WndProc:    %s",
     ( class_proc != wnd_proc ?
                SK_MakePrettyAddress (wnd_proc).c_str   () :
                L"Same" ) );

  WNDPROC target_proc = nullptr;
  bool    ignore_proc = false;

  // OpenGL games almost always have a window proc. located in OpenGL32.dll; we don't want that.
  if      (SK_GetModuleFromAddr   (class_proc) == SK_GetModuleHandle (nullptr)) target_proc = class_proc;
  else if (SK_GetModuleFromAddr   (wnd_proc)   == SK_GetModuleHandle (nullptr)) target_proc = wnd_proc;
  else if (SK_GetModuleFromAddr   (class_proc) != INVALID_HANDLE_VALUE)         target_proc = class_proc;
  //else if (SK_IsAddressExecutable (class_proc,true))                            target_proc = class_proc;
  //else if (SK_IsAddressExecutable (wnd_proc,  true))                            target_proc = wnd_proc;
  else                                                                          ignore_proc = true;// target_proc = wnd_proc;


  // CAPCOM games have stupid DLC anti-piracy that we need to work around.
  if (_wcsicmp (wszClassName, L"via")          == 0 || // Resident Evil
      _wcsicmp (wszClassName, L"MT FRAMEWORK") == 0)   // MT Framework
  {                              
    SK_GetCurrentRenderBackend ().windows.capcom = true;
    config.window.dont_hook_wndproc              = true;

    if (! config.platform.silent)
    {
      if (! StrStrIW (config.steam.dll_path.c_str (), L"kaldaien_api"))
      {
        config.platform.silent                     = true;
        SK_RestartGame (nullptr, L"Game Restart Required to Workaround CRAPCOM DLC Anti-Piracy");
      }
    }
  }


  // In case we cannot hook the target, try the other...
  WNDPROC
    alt_proc = ( target_proc == class_proc ? wnd_proc :
                                           class_proc );

  const bool
    hook_func = (! config.window.dont_hook_wndproc) &&
                (! ignore_proc);


  if (hook_func)
  {
    if ( MH_OK ==
         MH_CreateHook (
                                            target_proc,
                                    SK_DetourWindowProc,
           static_cast_p2p <void> (&game_window.WndProc_Original)
         )
       )
    {
      MH_QueueEnableHook (target_proc);

      dll_log->Log (L"[Window Mgr]  >> Hooked %s.", ( target_proc == class_proc ? L"ClassProc" :
                    L"WndProc" ) );

      game_window.hooked = true;
    }


    // Target didn't work, go with the other one...
    else if ( MH_OK ==
               MH_CreateHook (
                 alt_proc,
                   SK_DetourWindowProc,
      static_cast_p2p <void> (&game_window.WndProc_Original)
                             )
            )
    {
      MH_QueueEnableHook (alt_proc);

      dll_log->Log (L"[Window Mgr]  >> Hooked %s.", ( alt_proc == class_proc ? L"ClassProc" :
                    L"WndProc" ) );

      game_window.hooked = true;
    }

    // Now we're boned!  (Actually, fallback to SetClass/WindowLongPtr :P)
    else
    {
      game_window.WndProc_Original = nullptr;
    }
  }


  if ((! hook_func) || game_window.WndProc_Original == nullptr)
  {
    target_proc = wnd_proc;

    dll_log->Log ( L"[Window Mgr]  >> Hooking was impossible; installing new "
                   L"%s procedure instead (this may be undone "
                   L"by other software).", target_proc == class_proc ?
                   L"Class" : L"Window"   );

    game_window.WndProc_Original = target_proc;

    if (target_proc == class_proc)
    {
      if (game_window.unicode)
        SetClassLongPtrW ( (HWND)hWnd, GCLP_WNDPROC, (LONG_PTR)SK_DetourWindowProc );
      else
        SetClassLongPtrA ( (HWND)hWnd, GCLP_WNDPROC, (LONG_PTR)SK_DetourWindowProc );

      game_window.hooked = false;
    }

    else
    {
      //game_window.SetWindowLongPtr ( hWnd,
      game_window.unicode ?
        SK_SetWindowLongPtrW ( (HWND)hWnd,
                                     GWLP_WNDPROC,
        reinterpret_cast <LONG_PTR> (SK_DetourWindowProc) ) :
        SK_SetWindowLongPtrA ( (HWND)hWnd,
                                     GWLP_WNDPROC,
        reinterpret_cast <LONG_PTR> (SK_DetourWindowProc) );

      game_window.hooked = false;
    }
  }

  game_window.hWnd   = hWnd;

  // Kiss of death for sane window management
  if (! _wcsicmp (wszClassName, L"UnityWndClass"))
    SK_GetCurrentRenderBackend ().windows.unity = true;

  else if (! _wcsicmp (wszClassName, L"UnrealWindow"))
    SK_GetCurrentRenderBackend ().windows.unreal = true;


  if (SK_IsInjected ())
  {
    auto *pRecord =
      SK_Inject_GetRecord (GetCurrentProcessId ());

    wcsncpy (pRecord->process.win_title, wszTitle, 127);

    SK_Inject_AuditRecord ( pRecord->process.id,
                            pRecord,
                   sizeof (*pRecord) );
  }
}

void
SK_HookWinAPI (void)
{
  static volatile LONG hooked = FALSE;

  if (! InterlockedCompareExchange (&hooked, TRUE, FALSE))
  {
    // Initialize the Window Manager
    SK_WindowManager::getInstance ();

    auto hModUser32 =
      SK_GetModuleHandle (L"user32");

#ifdef _DEBUG
    int phys_w         = GetDeviceCaps (GetDC (GetDesktopWindow ()), PHYSICALWIDTH);
    int phys_h         = GetDeviceCaps (GetDC (GetDesktopWindow ()), PHYSICALHEIGHT);
    int phys_off_x     = GetDeviceCaps (GetDC (GetDesktopWindow ()), PHYSICALOFFSETX);
    int phys_off_y     = GetDeviceCaps (GetDC (GetDesktopWindow ()), PHYSICALOFFSETY);
    int scale_factor_x = GetDeviceCaps (GetDC (GetDesktopWindow ()), SCALINGFACTORX);
    int scale_factor_y = GetDeviceCaps (GetDC (GetDesktopWindow ()), SCALINGFACTORY);

    SK_LOG0 ( ( L" Physical Coordinates  ::  { %lux%lu) + <%lu,%lu> * {%lu:%lu}",
             phys_w, phys_h,
             phys_off_x, phys_off_y,
             scale_factor_x, scale_factor_y ),
             L"Window Sys" );
#endif

#if 1
        SK_CreateDLLHook2 (  L"user32",
                              "SetWindowPos",
                               SetWindowPos_Detour,
      static_cast_p2p <void> (&SetWindowPos_Original) );

    SK_CreateDLLHook2 (      L"user32",
                              "SetWindowPlacement",
                               SetWindowPlacement_Detour,
      static_cast_p2p <void> (&SetWindowPlacement_Original) );


    SK_CreateDLLHook2 (      L"user32",
                              "MoveWindow",
                               MoveWindow_Detour,
      static_cast_p2p <void> (&MoveWindow_Original) );

    SK_CreateDLLHook2 ( L"user32",
                         "GetWindowInfo",
                          GetWindowInfo_Detour,
 static_cast_p2p <void> (&GetWindowInfo_Original) );

#else
    SetWindowPos_Original =
      (SetWindowPos_pfn)
      SK_GetProcAddress ( SK_GetModuleHandleW (L"user32"),
                            "SetWindowPos" );
    SetWindowPlacement_Original =
      (SetWindowPlacement_pfn)
      SK_GetProcAddress ( SK_GetModuleHandleW (L"user32"),
                            "SetWindowPlacement" );

    MoveWindow_Original =
      (MoveWindow_pfn)
      SK_GetProcAddress ( SK_GetModuleHandleW (L"user32"),
                            "MoveWindow" );
#endif
    SK_CreateDLLHook2 (      L"user32",
                              "ClipCursor",
                               ClipCursor_Detour,
      static_cast_p2p <void> (&ClipCursor_Original));

    SK_CreateDLLHook2 (       L"user32",
                       "SetWindowLongA",
                       SetWindowLongA_Detour,
                       static_cast_p2p <void> (&SetWindowLongA_Original) );

    SK_CreateDLLHook2 (       L"user32",
                       "SetWindowLongW",
                       SetWindowLongW_Detour,
                       static_cast_p2p <void> (&SetWindowLongW_Original) );

    SK_CreateDLLHook2 (       L"user32",
                       "GetWindowLongA",
                       GetWindowLongA_Detour,
                       static_cast_p2p <void> (&GetWindowLongA_Original) );

    SK_CreateDLLHook2 (       L"user32",
                       "GetWindowLongW",
                       GetWindowLongW_Detour,
                       static_cast_p2p <void> (&GetWindowLongW_Original) );
#ifdef _WIN64
    SK_CreateDLLHook2 (       L"user32",
                       "SetWindowLongPtrA",
                       SetWindowLongPtrA_Detour,
                       static_cast_p2p <void> (&SetWindowLongPtrA_Original) );

    SK_CreateDLLHook2 (       L"user32",
                       "SetWindowLongPtrW",
                       SetWindowLongPtrW_Detour,
                       static_cast_p2p <void> (&SetWindowLongPtrW_Original) );

    SK_CreateDLLHook2 (       L"user32",
                       "GetWindowLongPtrA",
                       GetWindowLongPtrA_Detour,
                       static_cast_p2p <void> (&GetWindowLongPtrA_Original) );

    SK_CreateDLLHook2 (       L"user32",
                       "GetWindowLongPtrW",
                       GetWindowLongPtrW_Detour,
                       static_cast_p2p <void> (&GetWindowLongPtrW_Original) );
#else

    // 32-bit Windows does not have these functions; just route them
    //   through the normal function since pointers and DWORDS have
    //     compatible size.
    SetWindowLongPtrA_Original = SetWindowLongA_Original;
    SetWindowLongPtrW_Original = SetWindowLongW_Original;
    GetWindowLongPtrW_Original = GetWindowLongW_Original;
    GetWindowLongPtrA_Original = GetWindowLongA_Original;
#endif

#if 0
    SK_CreateDLLHook2 (      L"user32",
                              "GetWindowRect",
                               GetWindowRect_Detour,
      static_cast_p2p <void> (&GetWindowRect_Original) );
#else
    GetWindowRect_Original =
      reinterpret_cast <GetWindowRect_pfn> (
        SK_GetProcAddress ( hModUser32, "GetWindowRect" )
      );
#endif

#if 0
    SK_CreateDLLHook2 (      L"user32",
                              "GetClientRect",
                               GetClientRect_Detour,
      static_cast_p2p <void> (&GetClientRect_Original) );
#else
    GetClientRect_Original =
      reinterpret_cast <GetClientRect_pfn> (
        SK_GetProcAddress ( hModUser32,
                       "GetClientRect" )
        );
#endif

    SK_CreateDLLHook2 (      L"user32",
                              "AdjustWindowRect",
                               AdjustWindowRect_Detour,
      static_cast_p2p <void> (&AdjustWindowRect_Original) );

    SK_CreateDLLHook2 (      L"user32",
                              "AdjustWindowRectEx",
                               AdjustWindowRectEx_Detour,
      static_cast_p2p <void> (&AdjustWindowRectEx_Original) );

    SK_CreateDLLHook2 (      L"user32",
                              "GetSystemMetrics",
                               GetSystemMetrics_Detour,
      static_cast_p2p <void> (&GetSystemMetrics_Original) );


    SK_CreateDLLHook2 (       L"user32",
                               "GetForegroundWindow",
                                GetForegroundWindow_Detour,
       static_cast_p2p <void> (&GetForegroundWindow_Original) );

    SK_CreateDLLHook2 (       L"user32",
                               "SetForegroundWindow",
                                SetForegroundWindow_Detour,
       static_cast_p2p <void> (&SetForegroundWindow_Original) );

    SK_CreateDLLHook2 (       L"user32",
                               "BringWindowToTop",
                                BringWindowToTop_Detour,
       static_cast_p2p <void> (&BringWindowToTop_Original) );

    SK_CreateDLLHook2 (       L"user32",
                               "GetActiveWindow",
                                GetActiveWindow_Detour,
       static_cast_p2p <void> (&GetActiveWindow_Original) );

    SK_CreateDLLHook2 (       L"user32",
                               "SetActiveWindow",
                                SetActiveWindow_Detour,
       static_cast_p2p <void> (&SetActiveWindow_Original) );

    SK_CreateDLLHook2 (       L"user32",
                               "GetFocus",
                                GetFocus_Detour,
       static_cast_p2p <void> (&GetFocus_Original) );

     GetWindowBand =
    (GetWindowBand_pfn)SK_GetProcAddress (L"user32.dll",
    "GetWindowBand");

     SetWindowBand =
    (SetWindowBand_pfn)SK_GetProcAddress (L"user32.dll",
    "SetWindowBand");

    InterlockedIncrement (&hooked);
  }

  else
    SK_Thread_SpinUntilAtomicMin (&hooked, 2);
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
    HMONITOR hMonitor = config.display.monitor_handle != 0 ?
                        config.display.monitor_handle      :
      MonitorFromWindow ( hWnd,
                            config.display.monitor_default); //MONITOR_DEFAULTTONEAREST );

    MONITORINFO     mi        = { 0 };
                    mi.cbSize = sizeof (mi);
    GetMonitorInfo (
      hMonitor,    &mi);

    actual.window = mi.rcMonitor;

    actual.client.left   = 0;
    actual.client.right  = actual.window.right - actual.window.left;
    actual.client.top    = 0;
    actual.client.bottom = actual.window.bottom - actual.window.top;
  }

  LONG game_width   = (game.client.right   - game.client.left);
  LONG window_width = (actual.client.right - actual.client.left);

  LONG game_height   = ( game.client.bottom   - game.client.top );
  LONG window_height = ( actual.client.bottom - actual.client.top );

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
CALLBACK
sk_window_s::DefProc (
  _In_ UINT   Msg,
  _In_ WPARAM wParam,
  _In_ LPARAM lParam
)
{
  return
    DefWindowProc (hWnd, Msg, wParam, lParam);
}

LRESULT
WINAPI
sk_window_s::CallProc (
  _In_ HWND    hWnd_,
  _In_ UINT    Msg,
  _In_ WPARAM  wParam,
  _In_ LPARAM  lParam )
{
  // We subclassed the window instead of hooking the game's window proc
  if (! hooked)
    return CallWindowProc (WndProc_Original, hWnd_, Msg, wParam, lParam);
  else
    return WndProc_Original (hWnd_, Msg, wParam, lParam);
}


LRESULT
WINAPI
SK_COMPAT_SafeCallProc (sk_window_s* pWin, HWND hWnd_, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  if (pWin == nullptr || ! IsWindow (hWnd_))
    return 1;

  LRESULT ret = 1;

  __try {
    ret =
      pWin->CallProc (hWnd_, Msg, wParam, lParam);
  }

  __except ( GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ||
             GetExceptionCode () == EXCEPTION_BREAKPOINT       ?
                                    EXCEPTION_EXECUTE_HANDLER  : EXCEPTION_CONTINUE_SEARCH )
  {
  }

  return ret;
}

#include <winternl.h>
#include <winnt.h>

BOOL
SK_Win32_IsGUIThread ( DWORD    dwTid,
                       SK_TLS **ppTLS )
{
  UNREFERENCED_PARAMETER (ppTLS);

  static volatile LONG64 last_result [3] = { 0x0, 0x0, 0x0 };
                  LONG64 test_result0 =
                    ReadAcquire64 (&last_result [0]),
                         test_result1 =
                    ReadAcquire64 (&last_result [1]),
                         test_result2 =
                    ReadAcquire64 (&last_result [2]);

  static volatile LONG write_idx = 0;

  if (     (DWORD)(test_result0 & 0x00000000FFFFFFFFULL) == dwTid)
    return (       test_result0 >> 32) > 0 ? TRUE : FALSE;
  else if ((DWORD)(test_result1 & 0x00000000FFFFFFFFULL) == dwTid)
    return (       test_result1 >> 32) > 0 ? TRUE : FALSE;
  else if ((DWORD)(test_result2 & 0x00000000FFFFFFFFULL) == dwTid)
    return (       test_result2 >> 32) > 0 ? TRUE : FALSE;

  DWORD dwTidOfMe =
    SK_GetCurrentThreadId ();

  BOOL
    bGUI = FALSE;

  if (dwTidOfMe == dwTid)
  {
    bGUI =
      IsGUIThread (FALSE);
  }

  else
  {
    GUITHREADINFO
      gti        = {                    };
      gti.cbSize = sizeof (GUITHREADINFO);

    bGUI =
      GetGUIThreadInfo (dwTid, &gti);
  }

  auto idx =
    InterlockedIncrement (&write_idx);

  LONG64 test_result = (bGUI ? (1ull << 32) : 0)
    | (LONG64)(dwTid & 0xFFFFFFFFUL);

  InterlockedExchange64 (
    &last_result [idx % 3], test_result
  );

  return
    bGUI;
}





HRESULT
WINAPI
SK_DWM_EnableMMCSS (BOOL enable)
{
  static HMODULE hModDwmApi =
    SK_LoadLibraryW (L"dwmapi.dll");

  typedef HRESULT (WINAPI *DwmEnableMMCSS_pfn)(BOOL);
  static auto              DwmEnableMMCSS =
         reinterpret_cast <DwmEnableMMCSS_pfn> (
     SK_GetProcAddress ( hModDwmApi,
                          "DwmEnableMMCSS" )   );

  return
    ( DwmEnableMMCSS != nullptr ) ?
      DwmEnableMMCSS (  enable  ) : E_NOTIMPL;
}

void
WINAPI
SK_DWM_Flush (void)
{
  static HMODULE hModDwmApi =
    SK_LoadLibraryW (L"dwmapi.dll");

  typedef HRESULT (WINAPI *DwmFlush_pfn)(void);
  static auto              DwmFlush =
         reinterpret_cast <DwmFlush_pfn> (
   SK_GetProcAddress ( hModDwmApi,
                          "DwmFlush" )   );

  DwmFlush ();
}













BOOL
WINAPI
SK_ClipCursor (const RECT *lpRect)
{
  if (ClipCursor_Original != nullptr)
    return ClipCursor_Original (lpRect);

  return
    ClipCursor (lpRect);
}

BOOL
WINAPI
SK_GetCursorPos (LPPOINT lpPoint)
{
  if (GetCursorPos_Original != nullptr)
    return GetCursorPos_Original (lpPoint);

  return
    GetCursorPos (lpPoint);
}

BOOL
WINAPI
SK_SetCursorPos (int X, int Y)
{
  if (SetCursorPos_Original != nullptr)
    return SetCursorPos_Original (X, Y);

  return
    SetCursorPos (X, Y);
}

UINT
WINAPI
SK_SendInput (UINT nInputs, LPINPUT pInputs, int cbSize)
{
  if (SendInput_Original != nullptr)
    return SendInput_Original (nInputs, pInputs, cbSize);

  return
    SendInput (nInputs, pInputs, cbSize);
}

BOOL
WINAPI
SK_SetWindowPos ( HWND hWnd,
                  HWND hWndInsertAfter,
                  int  X,
                  int  Y,
                  int  cx,
                  int  cy,
                  UINT uFlags )
{
  if (SetWindowPos_Original != nullptr)
  {
    return
      SetWindowPos_Original (hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
  }

  return
    SetWindowPos (hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
}

HWND SK_Win32_BackgroundHWND = HWND_DESKTOP;
void SK_Win32_BringBackgroundWindowToTop (void);


void
SK_Win32_DestroyBackgroundWindow (void)
{
  if (SK_Win32_BackgroundHWND != HWND_DESKTOP)
  {
    if (   IsWindow (SK_Win32_BackgroundHWND))
      DestroyWindow (SK_Win32_BackgroundHWND);

    SK_Win32_BackgroundHWND = HWND_DESKTOP;
  }
}

LRESULT
CALLBACK
SK_Win32_BackgroundWndProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  auto hWndGame =
    SK_GetGameWindow ();

  switch (msg)
  {
    case WM_CLOSE:
      //DestroyWindow (hwnd); // Alt+F4 should be handled by game's main window
      break;
    case WM_SETFOCUS:
    case WM_KILLFOCUS:
      return 0;

    case WM_LBUTTONDBLCLK:
    case WM_LBUTTONUP:
    case WM_LBUTTONDOWN: // Window is not activatable, swallow mouse clicks and activate the game instead.
      game_window.active = true;
      BringWindowToTop           (hwnd);
      SK_RealizeForegroundWindow (hWndGame);
      return 0;

    case WM_ACTIVATE:
    case WM_MOUSEACTIVATE:
    case WM_ACTIVATEAPP:
      //SK_RunOnce (SK_ImGui_Warning (L"Activated Aspect Ratio Background Window"));
      return 0;
      break;

    case WM_DISPLAYCHANGE:
      if (game_window.active)
      {
        SK_Win32_BringBackgroundWindowToTop ();
        SK_RealizeForegroundWindow  (hWndGame);
      }
      break;
    case WM_SETCURSOR:
      if (game_window.active)
      {
        SetCursor (NULL);
        return TRUE;
      }
  }

  return
    DefWindowProcW (hwnd, msg, wParam, lParam);
}

void
SK_Win32_BringBackgroundWindowToTop (void)
{
  if (! config.display.aspect_ratio_stretch)
    return;

  if (SK_Win32_BackgroundHWND == HWND_DESKTOP)
    return;

  HMONITOR hMonitor =
    MonitorFromWindow (SK_GetGameWindow (), MONITOR_DEFAULTTONEAREST);

  MONITORINFO mi = { .cbSize = sizeof (MONITORINFO) };

  RECT                                     wndRect = { };
  GetWindowRect (SK_Win32_BackgroundHWND, &wndRect);

  if (GetMonitorInfo (hMonitor, &mi))
  {
    HWND hWndGame =
      SK_GetGameWindow ();

    if (                                      hWndGame != 0 &&
              IsWindow (                      hWndGame)   //&&
        //(GetNextWindow (hWnd, GW_HWNDPREV) != hWndGame ||
        //   ! EqualRect (&wndRect, &mi.rcMonitor))
       )
    {
      HWND hWndAfter = hWndGame;

      SK_SetWindowPos ( SK_Win32_BackgroundHWND, hWndGame,
                          mi.rcMonitor.left,
                          mi.rcMonitor.top,
                            mi.rcMonitor.right  - mi.rcMonitor.left,
                            mi.rcMonitor.bottom - mi.rcMonitor.top,
      (hWndAfter != hWndGame) ? SWP_NOREPOSITION
                              : 0x0
                              | SWP_NOSENDCHANGING | SWP_NOACTIVATE |
      
      ( config.display.aspect_ratio_stretch ? SWP_SHOWWINDOW
                                            : SWP_HIDEWINDOW ) );

      // Unreal Engine has bad window management during startup videos,
      //   so we need to raise the game above this secondary window.
      SK_RunOnce (
        BringWindowToTop (hWndGame)
      );
    }
  }
}

void
SK_Win32_CreateBackgroundWindow (void)
{
  if (! config.display.aspect_ratio_stretch)
    return;

  static bool once = false;

  if (std::exchange (once, true))
  {
    return;
  }

  WNDCLASSEXW wc  = { };

  wc.cbSize        = sizeof (WNDCLASSEXW);
  wc.lpfnWndProc   = SK_Win32_BackgroundWndProc;
  wc.hInstance     = SK_GetModuleHandle (nullptr);
  wc.hCursor       = LoadCursor         (nullptr, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)GetStockObject (BLACK_BRUSH);
  wc.lpszClassName = L"SK_AspectRatioWindow";

  if (! RegisterClassExW (&wc))
  {
    MessageBoxW (NULL, L"Window Registration Failed!", L"Error!",
      MB_ICONEXCLAMATION | MB_OK);

    return;
  }

  SK_Thread_CreateEx ([](LPVOID) -> DWORD
  { 
    SK_Win32_BackgroundHWND =
      CreateWindowExW ( WS_EX_NOACTIVATE,
          L"SK_AspectRatioWindow",
          L"Special K Aspect Ratio Background",
            SK_BORDERLESS,
            CW_USEDEFAULT, CW_USEDEFAULT,
                        0,             0,
                            nullptr,  nullptr,
        SK_GetModuleHandle (nullptr), nullptr
      );

    if (SK_Win32_BackgroundHWND == nullptr)
    {
      MessageBoxW (NULL, L"Window Creation Failed!", L"Error!",
        MB_ICONEXCLAMATION | MB_OK);

      SK_Thread_CloseSelf ();

      return 0;
    }

    ShowWindow   (SK_Win32_BackgroundHWND, SW_SHOW);
    UpdateWindow (SK_Win32_BackgroundHWND);

    // Wakes up a lot, to do nothing...
    SK_Thread_ScopedPriority priority (
      THREAD_PRIORITY_BELOW_NORMAL
    );

    // When all possible events that require resizing the window and changing
    //   its Z-Order are identified, the spurious wake behavior will be removed

    do
    {
      if ( WAIT_OBJECT_0 ==
             MsgWaitForMultipleObjects (0, nullptr, FALSE, 250UL, QS_ALLINPUT) )
      {
        MSG                   msg = { };
        while (PeekMessageW (&msg, nullptr, 0, 0, PM_REMOVE) != 0)
        {
          TranslateMessage (&msg);
          DispatchMessageW (&msg);
        }
      }

      static bool virtual_fullscreen = false;

      if ( std::exchange (virtual_fullscreen, config.display.aspect_ratio_stretch) !=
                                              config.display.aspect_ratio_stretch  ||
                                              config.display.aspect_ratio_stretch )
      {
        SK_Win32_BringBackgroundWindowToTop ();

        static bool last_state = false;

        if ( std::exchange (last_state, config.display.aspect_ratio_stretch) !=
                                        config.display.aspect_ratio_stretch )
        {
          ShowWindow ( SK_Win32_BackgroundHWND,
            config.display.aspect_ratio_stretch ? SW_SHOWNA
                                                : SW_HIDE );
        }
      }
    } while (true);

    SK_Thread_CloseSelf ();

    return 0;
  }, L"[SK] Aspect Ratio Window");
}

bool SK_Window_OnFocusChange (HWND hWndNewTarget, HWND hWndOld)
{
  // We don't really care about other windows, sorry ;)
  if (game_window.hWnd != 0)
  {
    if (config.system.log_level > 0)
      OutputDebugStringW (SK_FormatStringW (L"OnFocusChange (%x, %x), Game=%x", hWndNewTarget, hWndOld, game_window.hWnd).c_str ());

    // We may need to re-order windows upon focus change.
    HWND hWndNewTop = 0;

    bool bOrigTopMost = SK_Window_IsTopMost (game_window.hWnd);
    bool bTopMost     = bOrigTopMost;

    //if (hWndNewTarget != 0)
    {
      switch (config.window.always_on_top)
      {
        case AlwaysOnTop:
          bTopMost = true;
          break;
        case PreventAlwaysOnTop:
          bTopMost = false;
          break;

        case SmartAlwaysOnTop:
          // We have to remove TopMost to display a window (that has taken input focus away),
          //   because the game window would cover it otherwise.
          if (hWndNewTarget != game_window.hWnd && hWndNewTarget != 0 && SK_Window_TestOverlap (hWndNewTarget, game_window.hWnd))
          {
            hWndNewTop = hWndNewTarget;
            //dll_log->Log (L"Smart Always On Top: Decay From TopMost {1}");
            bTopMost   = false; // Game will cease to be top-most
          }

          else if (hWndNewTarget == game_window.hWnd)
          {
          //dll_log->Log (L"Smart Always On Top: Promotion to TopMost {0}");
            bTopMost = true; // Game is promoted to top-most
          }

          else if (hWndNewTarget != 0)
          {
            BOOL     bGameIsTopMostOnMonitor
                                  = TRUE;
            HMONITOR hMonitorGame = MonitorFromWindow (game_window.hWnd, MONITOR_DEFAULTTONEAREST);
            HWND        hWndAbove =
              GetWindow (game_window.hWnd, GW_HWNDPREV);

            std::set <HWND> hWndTopLevel,
                            hWndTopLevelOnGameMonitor;

            EnumWindows ([](HWND hWnd, LPARAM lParam)
           -> BOOL
            {
              std::set <HWND>* pTopLevelSet =
                (std::set <HWND> *)lParam;

              pTopLevelSet->emplace (hWnd);

              return TRUE;
            },      (LPARAM)&hWndTopLevel);
            for (auto hWnd : hWndTopLevel)
            {
              RECT                  rcWindow = { };
              GetWindowRect (hWnd, &rcWindow);

              POINT pt;

              pt.x = rcWindow.left + (rcWindow.right  - rcWindow.left) / 2;
              pt.y = rcWindow.top  + (rcWindow.bottom - rcWindow.top)  / 2;

              if (MonitorFromPoint (pt, MONITOR_DEFAULTTONEAREST) == hMonitorGame)
              //if (MonitorFromWindow (hWnd, MONITOR_DEFAULTTONEAREST) == hMonitorGame)
                hWndTopLevelOnGameMonitor.emplace (hWnd);
            }

            while (hWndAbove != nullptr && IsWindow (hWndAbove))
            {
              if (IsWindowVisible (hWndAbove) && hWndTopLevelOnGameMonitor.count (hWndAbove))
              {
                wchar_t wszWindowTitle [128] = { };

                // NOTE: GetWindowText calls SendMessage, which will deadlock Unity engine games
                if (config.system.log_level > 0)
                  GetWindowTextW (hWndAbove, wszWindowTitle, 128);

                if (config.system.log_level <= 0 || wszWindowTitle [0] != L'\0')
                {
                  bGameIsTopMostOnMonitor = FALSE;

                  if (config.system.log_level > 0)
                    dll_log->Log (L"Window: '%ws' is above the game on its monitor...", wszWindowTitle);

                  break;
                }
              }

              hWndAbove =
                GetWindow (hWndAbove, GW_HWNDPREV);
            }

            //  We're only interested in windows that spill-over on top of the game window
            //                       not windows that are completely disjoint on a different monitor
            if (                                         bGameIsTopMostOnMonitor &&
                 (! SK_Window_TestOverlap (hWndNewTarget, game_window.hWnd)) )
            {
              if ( MonitorFromWindow (hWndNewTarget,    MONITOR_DEFAULTTONEAREST) !=
                   MonitorFromWindow (game_window.hWnd, MONITOR_DEFAULTTONEAREST) )
              {
              //dll_log->Log (L"Smart Always On Top: Promotion to TopMost {2}");
                //hWndNewTop = game_window.hWnd;
                bTopMost   = true; // Game is promoted to top-most
              }
            }
          }
          break;
        default:
          bTopMost = bOrigTopMost;
          break;
      }
    }

    if (bOrigTopMost != bTopMost)
    {
    //dll_log->Log (L"SK_Window_OnFocusChange: bOrigTopMost != bTopMost");

      SK_Window_SetTopMost (
        bTopMost, bTopMost, game_window.hWnd
      );
    }

    if (hWndNewTop != nullptr)
    {
    //dll_log->Log (L"SK_Window_OnFocusChange: BringWindowToTop (hWndNewTop)");
      BringWindowToTop (hWndNewTop);
    }

    if (bTopMost)
    {
      extern void SK_Inject_SetFocusWindow (HWND);
                  SK_Inject_SetFocusWindow (game_window.hWnd);
    }

    // Misnomer; brings a secondary window up the Z-Order to just below
    //   the game window.
    SK_Win32_BringBackgroundWindowToTop ();
  }

  return true;
}


GetWindowBand_pfn GetWindowBand = nullptr;
SetWindowBand_pfn SetWindowBand = nullptr;