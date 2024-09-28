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
#include <hidclass.h>
#include <resource.h>

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"Input Mgr."

bool
SK_InputUtil_IsHWCursorVisible (void)
{
  if (SK_GetCursor () != nullptr)
  {
    CURSORINFO cursor_info        = { };
               cursor_info.cbSize = sizeof (CURSORINFO);

    SK_GetCursorInfo (&cursor_info);

    return
      ( (cursor_info.flags & CURSOR_SHOWING) && (cursor_info.hCursor != nullptr) );
  }

  return false;
}

ShowCursor_pfn ShowCursor_Original = nullptr;

BOOL
WINAPI
SK_SendMsgShowCursor (BOOL bShow)
{
  if (game_window.hWnd != 0 && IsWindow (game_window.hWnd))
  {
         if (  bShow) PostMessageA (game_window.hWnd, game_window.messages [game_window.messages->ShowCursor].uiMessage, 0, 0);
    else if (! bShow) PostMessageA (game_window.hWnd, game_window.messages [game_window.messages->HideCursor].uiMessage, 0, 0);

    if (GetActiveWindow () != game_window.hWnd)
      return TRUE;
  }

  static constexpr auto          _MaxTries = 25;
  for ( UINT tries = 0 ; tries < _MaxTries ; ++tries )
  {
    if (        bShow  && SK_ShowCursor (TRUE) >= 0)
      return TRUE;

    else if ((! bShow) && SK_ShowCursor (FALSE) < 0)
      return TRUE;
  }

  return FALSE;
}

HCURSOR
WINAPI
SK_SendMsgSetCursor (HCURSOR hCursor)
{
  if (game_window.hWnd != 0 && IsWindow (game_window.hWnd))
  {
    HCURSOR hLastCursor =
      SK_GetCursor ();

    if (hLastCursor == hCursor)
      return hLastCursor;

    if (GetActiveWindow () != game_window.hWnd)
    {
      PostMessageA ( game_window.hWnd,
                     game_window.messages [game_window.messages->SetCursorImg].uiMessage,
                       (WPARAM)(hCursor), 0 );
      return hLastCursor;
    }
  }

  return
    SK_SetCursor (hCursor);
}

int
WINAPI
SK_ShowCursor (BOOL bShow)
{
  return (ShowCursor_Original != nullptr) ?
          ShowCursor_Original (bShow)     :
          ShowCursor          (bShow);
}

int
WINAPI
ShowCursor_Detour (BOOL bShow)
{
  CURSORINFO
    cursor_info        = { };
    cursor_info.cbSize = sizeof (CURSORINFO);

  SK_GetCursorInfo (&cursor_info);

  const bool bIsShowing =
    (cursor_info.flags & CURSOR_SHOWING);

  const bool bIsCapturing =
    (SK_ImGui_WantMouseCapture () && SK_ImGui_IsAnythingHovered ());

  const bool bCanHide =
    ((config.input.cursor.manage == false || config.input.cursor.timeout == 0 || SK_Window_IsCursorActive () == false) &&
                ((! bIsCapturing) || config.input.ui.use_hw_cursor == false)) || SK_ImGui_Cursor.force == sk_cursor_state::Hidden;
  const bool bCanShow =
    ((config.input.cursor.manage == false ||(config.input.cursor.timeout != 0 && SK_Window_IsCursorActive () == true)) &&
                ((! bIsCapturing) || config.input.ui.use_hw_cursor == true )) || SK_ImGui_Cursor.force == sk_cursor_state::Visible;


  static int expected_val = 0;

  SK_RunOnce (expected_val = ShowCursor_Original (FALSE) + 1;
                             ShowCursor_Original (TRUE));

  static constexpr int  _MAX_TRIES = 8;

  int ret = 0;

  ret = bShow ? ++expected_val
              : --expected_val;


  if (     SK_ImGui_Cursor.force == sk_cursor_state::Visible)
    bShow = true;
  else if (SK_ImGui_Cursor.force == sk_cursor_state::Hidden )
    bShow = false;


  if (config.input.cursor.manage && SK_ImGui_Cursor.force == sk_cursor_state::None)
  {
    if (bIsCapturing)
    {
      bShow = config.input.ui.use_hw_cursor;
    }

    if (bIsShowing)
    {
      if (bShow)
      {
        return ret;
      }

      else
      {
        if (! bCanHide)
        {
          return ret;
        }

        ShowCursor_Original (FALSE);

        return ret;
      }
    }

    else
    {
      if (! bShow)
      {
        return ret;
      }

      else
      {
        if (! bCanShow)
          return ret;

        ShowCursor_Original (TRUE);

        return ret;
      }
    }
  }


  else if (bIsCapturing || SK_ImGui_Cursor.force != sk_cursor_state::None)
  {
    if (SK_ImGui_Cursor.force == sk_cursor_state::None)
      bShow = config.input.ui.use_hw_cursor;

    if (bShow)
    {
      if (! (cursor_info.flags & CURSOR_SHOWING))
      {
        for ( int i = 0 ; i < _MAX_TRIES ; ++i )
        {
          if (ShowCursor_Original (TRUE) >= 0)
            break;
        }
      }

      return ret;
    }
  
    else
    {
      if (cursor_info.flags & CURSOR_SHOWING)
      {
        for ( int i = 0 ; i < _MAX_TRIES ; ++i )
        {
          if (ShowCursor_Original (FALSE) < 0)
            break;
        }
      }

      return ret;
    }
  }


  for ( int x = 0 ; x < 512 ; ++x )
  {
    int real_val =
      ShowCursor_Original (bShow);

    if (     real_val < ret) bShow = true;
    else if (real_val > ret) bShow = false;
    else break;
  }


  return
    ret;
}

SetCursor_pfn     SetCursor_Original     = nullptr;
GetCursor_pfn     GetCursor_Original     = nullptr;
GetCursorInfo_pfn GetCursorInfo_Original = nullptr;

/////////////////////////////////////////////////
//
// ImGui Cursor Management
//
/////////////////////////////////////////////////
sk_imgui_cursor_s SK_ImGui_Cursor;

HCURSOR GetGameCursor (void);

bool
SK_ImGui_IsAnythingHovered (void)
{
  auto& screenshot_mgr = SK_GetCurrentRenderBackend ().screenshot_mgr;
  bool snipping        = (screenshot_mgr->getSnipState () != SK_ScreenshotManager::SnippingInactive &&
                          screenshot_mgr->getSnipState () != SK_ScreenshotManager::SnippingComplete);

  return
                      snipping ||
    ImGui::IsAnyItemHovered () ||
    ImGui::IsWindowHovered  (
               ImGuiHoveredFlags_AnyWindow                    |
               ImGuiHoveredFlags_AllowWhenBlockedByActiveItem |
               ImGuiHoveredFlags_AllowWhenBlockedByPopup
                            );
}

bool
SK_ImGui_IsMouseRelevantEx (void)
{
  bool relevant =
    config.input.mouse.disabled_to_game || SK_ImGui_Active ();

  if (! relevant)
  {
    // SK_ImGui_Active () returns true for the full-blown config UI;
    //   we also have floating widgets that may capture mouse input.
    relevant =
      SK_ImGui_IsAnythingHovered ();
  }

  return relevant;
}

bool
SK_ImGui_IsMouseRelevant (void)
{
  return SK_ImGui_IsMouseRelevantEx ();
}

void
sk_imgui_cursor_s::update (void)
{
}

//__inline
void
sk_imgui_cursor_s::showImGuiCursor (void)
{
}

void
sk_imgui_cursor_s::LocalToScreen (LPPOINT lpPoint)
{
  LocalToClient  (lpPoint);
  ClientToScreen (game_window.child != 0 ? game_window.child : game_window.hWnd, lpPoint);
}

void
sk_imgui_cursor_s::LocalToClient (LPPOINT lpPoint)
{
  if (! SK_GImDefaultContext ())
    return;

  static const auto& io =
    ImGui::GetIO ();

  RECT                                                                           real_client = { };
  GetClientRect (game_window.child != 0 ? game_window.child : game_window.hWnd, &real_client);

  ImVec2 local_dims =
    io.DisplayFramebufferScale;

  struct {
    float width  = 1.0f,
          height = 1.0f;
  } in, out;

  in.width   = local_dims.x;
  in.height  = local_dims.y;

  out.width  = (float)(real_client.right  - real_client.left);
  out.height = (float)(real_client.bottom - real_client.top);

  float x = 2.0f * ((float)lpPoint->x / std::max (1.0f, in.width )) - 1.0f;
  float y = 2.0f * ((float)lpPoint->y / std::max (1.0f, in.height)) - 1.0f;

  lpPoint->x = (LONG)( ( x * out.width  + out.width  ) / 2.0f );
  lpPoint->y = (LONG)( ( y * out.height + out.height ) / 2.0f );
}

void
sk_imgui_cursor_s::ClientToLocal    (LPPOINT lpPoint)
{
  if (! SK_GImDefaultContext ())
    return;

  static const auto& io =
    ImGui::GetIO ();

  RECT                                                                           real_client = { };
  GetClientRect (game_window.child != 0 ? game_window.child : game_window.hWnd, &real_client);

  const ImVec2 local_dims =
    io.DisplayFramebufferScale;

  struct {
    float width  = 1.0f,
          height = 1.0f;
  } in, out;

  out.width  = local_dims.x;
  out.height = local_dims.y;

  in.width   = (float)(real_client.right  - real_client.left);
  in.height  = (float)(real_client.bottom - real_client.top);

  float x =     2.0f * ((float)lpPoint->x /
            std::max (1.0f, in.width )) - 1.0f;
  float y =     2.0f * ((float)lpPoint->y /
            std::max (1.0f, in.height)) - 1.0f;
            // Avoid division-by-zero, this should be a signaling NAN but
            //   some games alter FPU behavior and will turn this into a non-continuable exception.

  lpPoint->x = (LONG)( ( x * out.width  + out.width  ) / 2.0f );
  lpPoint->y = (LONG)( ( y * out.height + out.height ) / 2.0f );
}

//__inline
void
sk_imgui_cursor_s::ScreenToLocal (LPPOINT lpPoint)
{
  ScreenToClient (game_window.child != 0 ? game_window.child : game_window.hWnd, lpPoint);
  ClientToLocal  (lpPoint);
}

HCURSOR
ImGui_DesiredCursor (void)
{
  if (! config.input.ui.use_hw_cursor)
    return 0;

  static HCURSOR last_cursor = nullptr;

  static const std::map <UINT, HCURSOR>
    __cursor_cache =
    {
      { ImGuiMouseCursor_Arrow,
          LoadCursor (SK_GetDLL (), (LPCWSTR)IDC_CURSOR_POINTER) },
      { ImGuiMouseCursor_TextInput,
          LoadCursor (nullptr,               IDC_IBEAM)          },
      { ImGuiMouseCursor_ResizeEW,
          LoadCursor (SK_GetDLL (), (LPCWSTR)IDC_CURSOR_HORZ)    },
      { ImGuiMouseCursor_ResizeNWSE,
          LoadCursor (nullptr,               IDC_SIZENWSE)       }
    };

  if (__SK_EnableSetCursor)
  {
    const auto&
        it  = __cursor_cache.find (static_cast <unsigned int> (ImGui::GetMouseCursor ()));
    if (it != __cursor_cache.cend ())
    {
      last_cursor =
             it->second;
      return it->second;
    }

    else
      return last_cursor;
  }

  return
    GetGameCursor ();
}

void
ImGuiCursor_Impl (void)
{
  static auto& io =
    ImGui::GetIO ();

  //
  // Hardware Cursor
  //
  if (config.input.ui.use_hw_cursor)
  {
    io.MouseDrawCursor =
      ( (! SK_ImGui_Cursor.idle) && SK_ImGui_IsMouseRelevant () && (! SK_InputUtil_IsHWCursorVisible ()));
  }

  //
  // Software
  //
  else
  {
    if (SK_ImGui_IsMouseRelevant ())
    {
      SK_SendMsgSetCursor (0);
    }

    io.MouseDrawCursor = (! SK_ImGui_Cursor.idle) && (! SK_InputUtil_IsHWCursorVisible ());
  }
}

static HCURSOR wait_cursor  = nullptr;
static HCURSOR arrow_cursor = nullptr;

void
sk_imgui_cursor_s::showSystemCursor (bool system)
{
  // Refactoring; function pending removal
  UNREFERENCED_PARAMETER (system);

  static auto& io =
    ImGui::GetIO ();

  if (wait_cursor == nullptr)
    wait_cursor = LoadCursor (nullptr, IDC_WAIT);

  if (arrow_cursor == nullptr)
    arrow_cursor = LoadCursor (nullptr, IDC_ARROW);

  ImGuiCursor_Impl ();
}


void
sk_imgui_cursor_s::activateWindow (bool active)
{
  if (active && config.input.ui.use_hw_cursor)
  {
    if (SK_ImGui_IsAnythingHovered ())//SK_ImGui_IsMouseRelevant ())
    {
      if (SK_ImGui_WantMouseCapture ())
      {
        SK_SetCursor (ImGui_DesiredCursor ());
      }
    }
  }
}

static constexpr const DWORD REASON_DISABLED = 0x4;

bool
SK_ImGui_WantMouseCaptureEx (DWORD dwReasonMask)
{
  if (! SK_GImDefaultContext ())
    return false;

  // Block mouse input while snipping
  auto& screenshot_mgr = SK_GetCurrentRenderBackend ().screenshot_mgr;
  bool snipping        = (screenshot_mgr->getSnipState () != SK_ScreenshotManager::SnippingInactive &&
                          screenshot_mgr->getSnipState () != SK_ScreenshotManager::SnippingComplete);
  if ( snipping )
    return true;

  // Allow mouse input while ReShade overlay is active
  if (SK_ReShadeAddOn_IsOverlayActive ())
    return false;

  // Allow mouse input while Steam /EOS overlays are active
  if (SK::SteamAPI::GetOverlayState (true) ||
           SK::EOS::GetOverlayState (true))
  {
    return false;
  }

  bool imgui_capture = false;

  if (SK_ImGui_IsMouseRelevantEx ())
  {
    static const auto& io =
      ImGui::GetIO ();

    if (io.WantCaptureMouse || (config.input.ui.capture_mouse && SK_ImGui_Active ()))
      imgui_capture = true;

    else if ((dwReasonMask & REASON_DISABLED) && config.input.mouse.disabled_to_game == SK_InputEnablement::Disabled)
      imgui_capture = true;

    if (game_window.mouse.can_track && (! game_window.mouse.inside) && config.input.mouse.disabled_to_game == SK_InputEnablement::DisabledInBackground)
      imgui_capture = true;

    else if (config.input.ui.capture_hidden && (! SK_InputUtil_IsHWCursorVisible ()))
      imgui_capture = true;

    if (game_window.active && ReadULong64Acquire (&config.input.mouse.temporarily_allow) > SK_GetFramesDrawn () - 20)
      imgui_capture = false;
  }

  if ((! SK_IsGameWindowActive ()) && config.input.mouse.disabled_to_game == SK_InputEnablement::DisabledInBackground)
    imgui_capture = true;

  return imgui_capture;
}



bool
SK_ImGui_WantMouseCapture (void)
{
  return
    SK_ImGui_WantMouseCaptureEx (0xFFFF);
}



HCURSOR GetGameCursor (void)
{
  return SK_GetCursor ();

  static HCURSOR sk_imgui_arrow = LoadCursor (SK_GetDLL (), (LPCWSTR)IDC_CURSOR_POINTER);
  static HCURSOR sk_imgui_horz  = LoadCursor (SK_GetDLL (), (LPCWSTR)IDC_CURSOR_HORZ);
  static HCURSOR sk_imgui_ibeam = LoadCursor (nullptr, IDC_IBEAM);
  static HCURSOR sys_arrow      = LoadCursor (nullptr, IDC_ARROW);
  static HCURSOR sys_wait       = LoadCursor (nullptr, IDC_WAIT);

  static HCURSOR hCurLast       = nullptr;
         HCURSOR hCur           = SK_GetCursor ();

  if ( hCur != sk_imgui_horz && hCur != sk_imgui_arrow && hCur != sk_imgui_ibeam &&
       hCur != sys_arrow     && hCur != sys_wait )
  {
    hCurLast = hCur;
  }

  return hCurLast;
}

bool
__stdcall
SK_IsGameWindowActive (void)
{
  bool bActive =
    game_window.active;

  if (! bActive)
  {
    HWND hWndForeground = SK_GetForegroundWindow ();

    bActive =
      (   game_window.hWnd        == hWndForeground ||
        ( SK_Win32_BackgroundHWND == hWndForeground &&
          SK_Win32_BackgroundHWND != 0 ) );
  }

#if 0
  if (! bActive)
  {
    const SK_RenderBackend&
      SK_GetCurrentRenderBackend ();

    if ( rb.windows.device.hwnd   != 0 &&
         rb.windows.device.hwnd   != game_window.hWnd &&
         rb.windows.device.parent != 0 )
    {
      SK_ReleaseAssert (game_window.hWnd == rb.windows.focus.hwnd);

      HWND hWndForeground =
        SK_GetForegroundWindow ();

      bActive =
        hWndForeground == rb.windows.device.hwnd ||
        hWndForeground == rb.windows.device.parent;

      if (bActive && (! game_window.active))
      {
        SetWindowPos ( hWndForeground, game_window.hWnd,
                         0, 0,
                         0, 0,
                           SWP_NOMOVE | SWP_NOSIZE |
                           SWP_ASYNCWINDOWPOS );
      }
    }
  }
#endif

  // Background Window (Aspect Ratio Stretch) is Foreground...
  //   we don't want that, make the GAME the foreground.
  if (bActive && (! game_window.active))
  {
    game_window.active = true;

    // This only activates the window if performed on the same thread as the
    //   game's window, so don't do this if called from a different thread.
    if (                         0 != SK_Win32_BackgroundHWND &&
         SK_GetForegroundWindow () == SK_Win32_BackgroundHWND &&
             GetCurrentThreadId () == GetWindowThreadProcessId (game_window.hWnd, nullptr) )
    {
      game_window.active = true;

      BringWindowToTop    (game_window.hWnd);
      SetWindowPos        ( SK_Win32_BackgroundHWND, game_window.hWnd,
                                  0, 0,
                                  0, 0,
                                    SWP_NOMOVE     | SWP_NOSIZE |
                             SWP_NOACTIVATE );
      SetForegroundWindow (game_window.hWnd);
      SetFocus            (game_window.hWnd);
    }
  }

  return bActive;
}

bool
__stdcall
SK_IsGameWindowFocused (void)
{
  auto
    hWndAtCenter = [&](void)
 -> HWND
    {
      return
        WindowFromPoint (
                  POINT {                     game_window.actual.window.left +
          (game_window.actual.window.right  - game_window.actual.window.left) / 2,
                                              game_window.actual.window.top  +
          (game_window.actual.window.bottom - game_window.actual.window.top)  / 2
                        }
                        );
    };

  return (
    SK_IsGameWindowActive () && (SK_GetFocus () == game_window.hWnd || 
                                hWndAtCenter () == game_window.hWnd)
  );
}

void
ImGui_ToggleCursor (void)
{
  static auto& io =
    ImGui::GetIO ();

  if (! SK_ImGui_Cursor.visible)
  {
    POINT                           pos = { };
    SK_GetCursorPos               (&pos);
    SK_ImGui_Cursor.ScreenToLocal (&pos);

    // Save original cursor position
    SK_ImGui_Cursor.orig_pos    =   pos;
    SK_ImGui_Cursor.idle        = false;
    io.WantCaptureMouse         =  true;

    // Move the cursor if it's not over any of SK's UI
    if (! SK_ImGui_IsAnythingHovered ())
    {
      SK_ImGui_CenterCursorOnWindow ();
    }
  }

  else
  {
    SK_ImGui_Cursor.idle = true;

    if (! SK_ImGui_IsAnythingHovered ())
    {
      POINT                            screen =
       SK_ImGui_Cursor.orig_pos;
       SK_ImGui_Cursor.LocalToScreen (&screen);
       SK_SetCursorPos               ( screen.x,
                                       screen.y );
    }
  }

  SK_ImGui_Cursor.visible = (! SK_ImGui_Cursor.visible);
}

GetMouseMovePointsEx_pfn GetMouseMovePointsEx_Original = nullptr;

int
WINAPI
GetMouseMovePointsEx_Detour(
  _In_  UINT             cbSize,
  _In_  LPMOUSEMOVEPOINT lppt,
  _Out_ LPMOUSEMOVEPOINT lpptBuf,
  _In_  int              nBufPoints,
  _In_  DWORD            resolution )
{
  SK_LOG_FIRST_CALL

  if (SK_ImGui_IsMouseRelevant ())
  {
    bool implicit_capture = false;

    // Depending on warp prefs, we may not allow the game to know about mouse movement
    //   (even if ImGui doesn't want mouse capture)
    if ( ( SK_ImGui_Cursor.prefs.no_warp.ui_open/* && SK_ImGui_IsMouseRelevant   ()*/ ) ||
         ( SK_ImGui_Cursor.prefs.no_warp.visible && SK_InputUtil_IsHWCursorVisible () )    )
    {
      implicit_capture = true;
    }

    if (SK_ImGui_WantMouseCapture () || implicit_capture)
    {
      *lpptBuf = *lppt;

      return 0;
    }
  }

  return
    GetMouseMovePointsEx_Original (cbSize, lppt, lpptBuf, nBufPoints, resolution);
}

HCURSOR
WINAPI
SK_GetCursor (VOID)
{
  if (GetCursor_Original != nullptr)
    return GetCursor_Original ();

  return GetCursor ();
}

bool __SK_EnableSetCursor = false;

HCURSOR
WINAPI
SK_SetCursor (
  _In_opt_ HCURSOR hCursor)
{
  HCURSOR hCursorRet = SK_GetCursor ();

  if (__SK_EnableSetCursor)
  {
    if (SetCursor_Original       != nullptr )
    {   SetCursor_Original       (  hCursor );
        SK_ImGui_Cursor.real_img  = hCursor; }
    else
    {   SK_ImGui_Cursor.real_img  = hCursor ;
        SetCursor                  (hCursor); }
  }

  return hCursorRet;
}

HCURSOR
WINAPI
SetCursor_Detour (
  _In_opt_ HCURSOR hCursor )
{
  SK_LOG_FIRST_CALL

  if ((SK_ImGui_WantMouseCapture () && SK_ImGui_IsAnythingHovered ()) || ImGui::GetIO ().WantCaptureMouse)
  {
    if (! config.input.ui.use_hw_cursor)
      return 0;

    hCursor =
      ImGui_DesiredCursor ();
  }

  return
    SetCursor_Original (hCursor);
}

HCURSOR
WINAPI
GetCursor_Detour (VOID)
{
  SK_LOG_FIRST_CALL

  //return
  //  GetCursor_Original ();

  static auto& io =
    ImGui::GetIO ();

  if (! (SK_ImGui_IsMouseRelevant () ||
                io.WantCaptureMouse)   )
  {
    SK_ImGui_Cursor.img =
      GetCursor_Original ();
  }

  return
    SK_ImGui_Cursor.img;
}

BOOL
WINAPI
SK_GetCursorInfo (PCURSORINFO pci)
{
  if ( pci != nullptr )
  {
    if (pci->cbSize == 0)
    {
      // Fix-Up:
      //
      //    It is Undocumented, but Windows sometimes
      //      zeros-out the cbSize field such that two
      //        successive uses of the same data structure
      //          will fail because cbSize does not match.
      //
      pci->cbSize =
        sizeof (CURSORINFO);
      // Should be the same size in modern software.
    }

    if (     GetCursorInfo_Original != nullptr)
      return GetCursorInfo_Original (pci);

    else if (GetCursorInfo_Original != nullptr)
      return GetCursorInfo_Original (pci);

    return
      GetCursorInfo (pci);
  }

  return FALSE;
}

BOOL
WINAPI
GetCursorInfo_Detour (PCURSORINFO pci)
{
  SK_LOG_FIRST_CALL

  POINT pt  = pci->ptScreenPos;
  BOOL  ret = SK_GetCursorInfo (pci);

  struct state_backup
  {
    explicit state_backup (PCURSORINFO pci) :
      hCursor     (pci->hCursor),
      ptScreenPos (pci->ptScreenPos) { };

    HCURSOR hCursor;
    POINT   ptScreenPos;
  } actual (pci);

  pci->ptScreenPos = pt;


  bool bMouseRelevant =
    SK_ImGui_IsMouseRelevant ();

  if (bMouseRelevant)
    pci->hCursor = ImGui_DesiredCursor ();


  if (ret && bMouseRelevant)
  {
    bool implicit_capture = false;

    // Depending on warp prefs, we may not allow the game to know about mouse movement
    //   (even if ImGui doesn't want mouse capture)
    if ( ( SK_ImGui_Cursor.prefs.no_warp.ui_open/* && SK_ImGui_IsMouseRelevant   ()*/ ) ||
         ( SK_ImGui_Cursor.prefs.no_warp.visible && SK_InputUtil_IsHWCursorVisible () )    )
      implicit_capture = true;

    if (SK_ImGui_WantMouseCapture () || implicit_capture)
    {
      pci->ptScreenPos = SK_ImGui_Cursor.orig_pos;
    }

    else
    {
      pci->ptScreenPos = SK_ImGui_Cursor.pos;
    }

    SK_ImGui_Cursor.LocalToScreen (&pci->ptScreenPos);

    return TRUE;
  }

  pci->hCursor     =
             SK_ImGui_IsMouseRelevant () ?
                ImGui_DesiredCursor   () :
                     actual.hCursor;
  pci->ptScreenPos = actual.ptScreenPos;

  return ret;
}

float SK_SO4_MouseScale = 2.467f;

BOOL
WINAPI
GetCursorPos_Detour (LPPOINT lpPoint)
{
  SK_LOG_FIRST_CALL

  if (SK_WantBackgroundRender () && (! SK_IsGameWindowActive ()))
  {
    SK_Win32_Backend->markHidden (sk_win32_func::GetCursorPos);

    *lpPoint = SK_ImGui_Cursor.orig_pos;

    SK_ImGui_Cursor.LocalToScreen (lpPoint);

    return TRUE;
  }


  if (SK_ImGui_IsMouseRelevant ())
  {
    bool implicit_capture = false;

    // Depending on warp prefs, we may not allow the game to know about mouse movement
    //   (even if ImGui doesn't want mouse capture)
    if ( ( SK_ImGui_Cursor.prefs.no_warp.ui_open/* && SK_ImGui_IsMouseRelevant   ()*/ ) ||
         ( SK_ImGui_Cursor.prefs.no_warp.visible && SK_InputUtil_IsHWCursorVisible () )    )
    {
      implicit_capture = true;
    }

    if (SK_ImGui_WantMouseCapture () || implicit_capture)
    {
      SK_Win32_Backend->markHidden (sk_win32_func::GetCursorPos);

      *lpPoint = SK_ImGui_Cursor.orig_pos;

      SK_ImGui_Cursor.LocalToScreen (lpPoint);

      return TRUE;
    }
  }


  BOOL bRet =
    SK_GetCursorPos (lpPoint);

  if (bRet)
  {
    SK_Win32_Backend->markRead (sk_win32_func::GetCursorPos);

    auto pos = *lpPoint;

    SK_ImGui_Cursor.ScreenToLocal (&pos);
    SK_ImGui_Cursor.pos           = pos;
  }


  static bool bStarOcean4 =
    (SK_GetCurrentGameID () == SK_GAME_ID::StarOcean4);

  if (bStarOcean4 && GetActiveWindow () == SK_GetGameWindow ())
  {
    static const auto& io =
      ImGui::GetIO ();

    static ULONG64 last_frame = SK_GetFramesDrawn ();
    static POINT   last_pos   = *lpPoint;

    POINT new_pos = *lpPoint;

    new_pos.x = (LONG)(io.DisplayFramebufferScale.x * 0.5f);
    new_pos.y = (LONG)(io.DisplayFramebufferScale.y * 0.5f);

    float ndc_x = 2.0f * (((float)(lpPoint->x - (float)last_pos.x) + (float)new_pos.x) / io.DisplayFramebufferScale.x) - 1.0f;
    float ndc_y = 2.0f * (((float)(lpPoint->y - (float)last_pos.y) + (float)new_pos.y) / io.DisplayFramebufferScale.y) - 1.0f;

    bool new_frame = false;

    if (last_frame != SK_GetFramesDrawn ())
    {
      last_pos   = *lpPoint;
      last_frame = SK_GetFramesDrawn ();
      new_frame  = true;
    }

    lpPoint->x = (LONG)((ndc_x * SK_SO4_MouseScale * io.DisplayFramebufferScale.x + io.DisplayFramebufferScale.x) / 2.0f);
    lpPoint->y = (LONG)((ndc_y * SK_SO4_MouseScale * io.DisplayFramebufferScale.y + io.DisplayFramebufferScale.y) / 2.0f);

    static int calls = 0;

    POINT get_pos; // 4 pixel cushion to avoid showing the cursor
    if (new_frame && ( (calls++ % 8) == 0 || ( SK_GetCursorPos (&get_pos) && (get_pos.x <= 4L || (float)get_pos.x >= (io.DisplayFramebufferScale.x - 4.0f) ||
                                                                              get_pos.y <= 4L || (float)get_pos.y >= (io.DisplayFramebufferScale.y - 4.0f)) ) ))
    {
      last_pos =       new_pos;
      SK_SetCursorPos (new_pos.x, new_pos.y);
      *lpPoint     = { new_pos.x, new_pos.y };
    }
  }

  return bRet;
}

BOOL
WINAPI
GetPhysicalCursorPos_Detour (LPPOINT lpPoint)
{
  SK_LOG_FIRST_CALL

  POINT                     pt;
  if (GetCursorPos_Detour (&pt))
  {
    if (LogicalToPhysicalPoint (0, &pt))
    {
      if (lpPoint != nullptr)
         *lpPoint = pt;

      return TRUE;
    }
  }

  return FALSE;
}

DWORD
WINAPI
GetMessagePos_Detour (void)
{
  SK_LOG_FIRST_CALL

  static DWORD dwLastPos =
           GetMessagePos_Original ();

  bool implicit_capture = false;

  // Depending on warp prefs, we may not allow the game to know about mouse movement
  //   (even if ImGui doesn't want mouse capture)
  if ( SK_ImGui_IsMouseRelevant () && ( SK_ImGui_Cursor.prefs.no_warp.ui_open/* && SK_ImGui_IsMouseRelevant   ()*/ ) ||
                                      ( SK_ImGui_Cursor.prefs.no_warp.visible && SK_InputUtil_IsHWCursorVisible () )    )
  {
    implicit_capture = true;
  }

  // TODO: Use the message time to determine whether to fake this or not
  if (SK_ImGui_WantMouseCapture () || implicit_capture)
  {
    return
      dwLastPos;
  }

  dwLastPos =
    GetMessagePos_Original ();

  return
    dwLastPos;
}

BOOL
WINAPI
SetCursorPos_Detour (_In_ int x, _In_ int y)
{
  SK_LOG_FIRST_CALL

  // Game WANTED to change its position, so remember that.
  POINT                           pt { x, y };
  SK_ImGui_Cursor.ScreenToLocal (&pt);
  SK_ImGui_Cursor.orig_pos =      pt;

  // Don't let the game continue moving the cursor while
  //   Alt+Tabbed out
  if (SK_WantBackgroundRender () && (! SK_IsGameWindowActive ()))
  {
    return TRUE;
  }

  // Prevent Mouse Look while Drag Locked
  if (config.window.drag_lock)
    return TRUE;

  if ( SK_ImGui_IsMouseRelevant () && ( SK_ImGui_Cursor.prefs.no_warp.ui_open/* && SK_ImGui_IsMouseRelevant   ()*/ ) ||
                                      ( SK_ImGui_Cursor.prefs.no_warp.visible && SK_InputUtil_IsHWCursorVisible () )    )
  {
    //game_mouselook = SK_GetFramesDrawn ();
  }

  else if (! SK_ImGui_WantMouseCapture ())
  {
    return
      SK_SetCursorPos (x, y);
  }

  return
    TRUE;
}

BOOL
WINAPI
SetPhysicalCursorPos_Detour (_In_ int x, _In_ int y)
{
  SK_LOG_FIRST_CALL

  POINT                             pt = { x, y };
  if (! PhysicalToLogicalPoint (0, &pt))
    return FALSE;

  return
    SetCursorPos_Detour (x, y);
}

struct SK_MouseTimer {
  POINTS  pos      = {     }; // POINT (Short) - Not POINT plural ;)
  DWORD   sampled  =     0UL;
  bool    cursor   =    true;

  int     init     =   FALSE;
  UINT    timer_id = 0x68993;
  HCURSOR
      class_cursor =       0;
} static last_mouse;

bool
SK_Window_IsCursorActive (void)
{
  return
    last_mouse.cursor;
}

bool
SK_Window_ActivateCursor (bool changed = false)
{
  const bool was_active = last_mouse.cursor;

  if (! was_active)
  {
    if ((! SK_IsSteamOverlayActive ()))
    {
      if (config.input.ui.use_hw_cursor)
      {
        SK_SendMsgShowCursor (TRUE);

        // Deliberately call SetCursor's _hooked_ function, so we can determine whether to
        //   activate the window using the game's cursor or our override
        SetClassLongPtrW (game_window.hWnd, GCLP_HCURSOR, (LONG_PTR)last_mouse.class_cursor);
        SK_SetCursor                                               (last_mouse.class_cursor);

        SK_SendMsgSetCursor (last_mouse.class_cursor);
      }

      else
      {
        // Deliberately call SetCursor's _hooked_ function, so we can determine whether to
        //   activate the window using the game's cursor or our override
        SetClassLongPtrW (game_window.hWnd, GCLP_HCURSOR, (LONG_PTR)0);
        SK_SetCursor                                               (0);

        SK_SendMsgSetCursor  ((HCURSOR)0);
        SK_SendMsgShowCursor (FALSE);
      }

      last_mouse.cursor = true;
    }
  }

  if (changed && (! SK_IsSteamOverlayActive ()))
    last_mouse.sampled = SK::ControlPanel::current_time;

  return (last_mouse.cursor != was_active);
};

bool
SK_Window_DeactivateCursor (bool ignore_imgui)
{
  if (! ignore_imgui)
  {
    if ( SK_ImGui_WantMouseCaptureEx (0xFFFFFFFF & ~REASON_DISABLED) ||
         (! last_mouse.cursor) )
    {
      last_mouse.sampled = SK::ControlPanel::current_time;

      return false;
    }
  }

  bool was_active = last_mouse.cursor;

  if (ignore_imgui || last_mouse.sampled < SK::ControlPanel::current_time - config.input.cursor.timeout)
  {
    if ((! SK_IsSteamOverlayActive ()))
    {
      if (was_active)
      {
        HCURSOR cursor = //SK_GetCursor ();
          (HCURSOR)GetClassLongPtrW (game_window.hWnd, GCLP_HCURSOR);

        if (cursor != 0)
          last_mouse.class_cursor = cursor;
      }

      SetClassLongPtrW (game_window.hWnd, GCLP_HCURSOR, 0);

      SK_SendMsgSetCursor  (0);
      SK_SendMsgShowCursor (FALSE);

      last_mouse.cursor  = false;
      last_mouse.sampled = SK::ControlPanel::current_time;

      last_mouse.pos =
        { (SHORT)SK_ImGui_Cursor.pos.x,
          (SHORT)SK_ImGui_Cursor.pos.y };
    }
  }

  return (last_mouse.cursor != was_active);
};

void
SK_ImGui_UpdateMouseTracker (void)
{
  bool bWasInside =
    game_window.mouse.inside;

  game_window.mouse.last_move_msg = SK::ControlPanel::current_time;
  game_window.mouse.inside        = true;

  if (game_window.hWnd == 0 || game_window.top != 0)
    return;
  
  if ((! (bWasInside && game_window.mouse.tracking && game_window.mouse.can_track)) && game_window.hWnd != 0)
  {
    TRACKMOUSEEVENT tme = { .cbSize      = sizeof (TRACKMOUSEEVENT),
                            .dwFlags     = TME_LEAVE,
                            .hwndTrack   = game_window.hWnd,
                            .dwHoverTime = HOVER_DEFAULT };
  
    if (TrackMouseEvent (&tme))
    {
      game_window.mouse.can_track = true;
      game_window.mouse.tracking  = true;
    }
  }
}

bool
SK_Input_DetermineMouseIdleState (MSG* lpMsg)
{
  static DWORD dwLastSampled = DWORD_MAX;

  static HWND hWndTimer  = 0;
  if (        hWndTimer != game_window.hWnd )
  {
    if (IsWindow (hWndTimer))
      KillTimer ( hWndTimer, sk::narrow_cast <UINT_PTR> (last_mouse.timer_id) );

    UINT _timeout =           config.input.cursor.timeout != 0 ?
      sk::narrow_cast <UINT> (config.input.cursor.timeout) / 2 :
                                                          250UL;

    // This was being installed on the wrong HWND, consider a dedicated timer
    //   in the future.
    SetTimer ( game_window.hWnd,
                 sk::narrow_cast <UINT_PTR> (last_mouse.timer_id),
                   _timeout, nullptr );

    hWndTimer = game_window.hWnd;
  }


  bool activation_event =
    ( lpMsg->message == WM_MOUSEMOVE /*|| lpMsg->message == WM_SETCURSOR*/) && (!SK_IsSteamOverlayActive ());

  bool bCapturingMouse =
    SK_ImGui_WantMouseCaptureEx (0xFFFFFFFF & ~REASON_DISABLED);

  // Don't blindly accept that WM_MOUSEMOVE actually means the mouse moved...
  if (! bCapturingMouse)
  {
    if (activation_event)
    {
      static constexpr const short threshold = 3;

      // Filter out small movements
      if ( abs (last_mouse.pos.x - SK_ImGui_Cursor.pos.x) < threshold &&
           abs (last_mouse.pos.y - SK_ImGui_Cursor.pos.y) < threshold )
      {
        activation_event = false;
      }
    }
  }

  if (config.input.cursor.keys_activate)
    activation_event |= ( lpMsg->message >= WM_KEYFIRST &&
                          lpMsg->message <= WM_KEYLAST );

  // If timeout is 0, just hide the thing indefinitely
  if (activation_event)
  {
    if ( (config.input.cursor.manage && config.input.cursor.timeout != 0) ||
            bCapturingMouse )
    {
      SK_Window_ActivateCursor (true);
    }
  }

  else if ( lpMsg->message == WM_TIMER            &&
            lpMsg->wParam  == last_mouse.timer_id &&
            (! SK_IsSteamOverlayActive ()) )
  {
    // If for some reason we get the event multiple times in a frame,
    //   use the results of the first test only
    if (std::exchange (dwLastSampled, SK::ControlPanel::current_time) !=
                                      SK::ControlPanel::current_time)
    {
      if (! bCapturingMouse)
      {
        if (config.input.cursor.manage)
        {
          static constexpr const short threshold = 3;

          if (config.input.cursor.timeout == 0)
          {
            SK_Window_DeactivateCursor ();
          }

          // Filter out small movements
          else if (abs (last_mouse.pos.x - SK_ImGui_Cursor.pos.x) < threshold &&
                   abs (last_mouse.pos.y - SK_ImGui_Cursor.pos.y) < threshold)
          {
            SK_Window_DeactivateCursor ();
          }
        }
      }

      else
        SK_Window_ActivateCursor (true);

      // Record the cursor pos (at time of timer fire)
      last_mouse.pos =
      { (SHORT)SK_ImGui_Cursor.pos.x,
        (SHORT)SK_ImGui_Cursor.pos.y };
    }

    return true;
  }

  return false;
}

void
SK_Input_PreHookCursor (void)
{
  SK_RunOnce (
  {
    SK_CreateDLLHook2 (       L"user32",
                             "GetCursorPos",
                              GetCursorPos_Detour,
     static_cast_p2p <void> (&GetCursorPos_Original) );


    // Win 8.1 and newer aliases these
    if (SK_GetProcAddress (L"user32", "GetPhysicalCursorPos") !=
        SK_GetProcAddress (L"user32", "GetCursorPos"))
    {
      SK_CreateDLLHook2 (       L"user32",
                                 "GetPhysicalCursorPos",
                                  GetPhysicalCursorPos_Detour,
         static_cast_p2p <void> (&GetPhysicalCursorPos_Original) );
    }

/*
  SK_CreateDLLHook2 (      L"user32",
                            "GetCursorInfo",
                             GetCursorInfo_Detour,
    static_cast_p2p <void> (&GetCursorInfo_Original) );
  
  SK_CreateDLLHook2 (       L"user32",
                             "GetMouseMovePointsEx",
                              GetMouseMovePointsEx_Detour,
     static_cast_p2p <void> (&GetMouseMovePointsEx_Original) );
*/

  SK_CreateDLLHook2 (       L"user32",
                             "SetCursor",
                              SetCursor_Detour,
     static_cast_p2p <void> (&SetCursor_Original) );

  SK_CreateDLLHook2 (       L"user32",
                             "ShowCursor",
                              ShowCursor_Detour,
     static_cast_p2p <void> (&ShowCursor_Original) );

/*
  SK_CreateDLLHook2 (       L"user32",
                             "GetCursor",
                             GetCursor_Detour,
    static_cast_p2p <void> (&GetCursor_Original) );
*/

    SK_CreateDLLHook2 (       L"user32",
                               "SetCursorPos",
                                SetCursorPos_Detour,
       static_cast_p2p <void> (&SetCursorPos_Original) );

    // Win 8.1 and newer aliases these
    if (SK_GetProcAddress (L"user32", "SetPhysicalCursorPos") !=
        SK_GetProcAddress (L"user32", "SetCursorPos"))
    {
      SK_CreateDLLHook2 (       L"user32",
                                 "SetPhysicalCursorPos",
                                  SetPhysicalCursorPos_Detour,
         static_cast_p2p <void> (&SetPhysicalCursorPos_Original) );
    }
  });
}