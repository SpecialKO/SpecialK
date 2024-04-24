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

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"Input Mgr."


SetWindowsHookEx_pfn    SetWindowsHookExA_Original   = nullptr;
SetWindowsHookEx_pfn    SetWindowsHookExW_Original   = nullptr;
UnhookWindowsHookEx_pfn UnhookWindowsHookEx_Original = nullptr;


class SK_Win32_WindowHookManager {
public:
std::map <
  DWORD, HOOKPROC > _RealMouseProcs;
         HOOKPROC   _RealMouseProc    = nullptr;
std::map <
  DWORD, HHOOK >    _RealMouseHooks;
         HHOOK      _RealMouseHook    = nullptr;

std::map <
  DWORD, HOOKPROC > _RealKeyboardProcs;
         HOOKPROC   _RealKeyboardProc = nullptr;
std::map <
  DWORD, HHOOK >    _RealKeyboardHooks;
         HHOOK      _RealKeyboardHook = nullptr;
} __hooks;

static POINTS last_pos;

LRESULT
CALLBACK
SK_Proxy_MouseProc   (
  _In_ int    nCode,
  _In_ WPARAM wParam,
  _In_ LPARAM lParam )
{
  if (nCode >= 0)
  {
    switch (wParam)
    {
      case WM_MOUSEMOVE:
      case WM_LBUTTONDOWN:
      case WM_LBUTTONDBLCLK:
      case WM_RBUTTONDOWN:
      case WM_RBUTTONDBLCLK:
      case WM_MBUTTONDOWN:
      case WM_MBUTTONDBLCLK:
      case WM_XBUTTONDOWN:
      case WM_XBUTTONDBLCLK:
      {
        MOUSEHOOKSTRUCT *mhs =
          (MOUSEHOOKSTRUCT *)lParam;

        static auto& io =
          ImGui::GetIO ();

        io.KeyCtrl  |= ((mhs->dwExtraInfo & MK_CONTROL) != 0);
        io.KeyShift |= ((mhs->dwExtraInfo & MK_SHIFT  ) != 0);

        switch (wParam)
        {
          case WM_MOUSEMOVE:
          {
            // No TrackMouseEvent available, have to do this manually
            if (! game_window.mouse.can_track)
            {
              POINT                                          pt (mhs->pt);
              ScreenToClient             (game_window.child != nullptr ?
                                          game_window.child            :
                                          game_window.hWnd, &pt);
              if (ChildWindowFromPointEx (game_window.child != nullptr ?
                                          game_window.child            :
                                          game_window.hWnd,  pt, CWP_SKIPDISABLED) == (game_window.child != nullptr ?
                                                                                       game_window.child            :
                                                                                       game_window.hWnd))
              {
                SK_ImGui_Cursor.ClientToLocal (&pt);
                SK_ImGui_Cursor.pos =           pt;

                io.MousePos.x = (float)SK_ImGui_Cursor.pos.x;
                io.MousePos.y = (float)SK_ImGui_Cursor.pos.y;
              }

              else
                io.MousePos = ImVec2 (-FLT_MAX, -FLT_MAX);
            }

            // Install a mouse tracker to get WM_MOUSELEAVE
            if (! (game_window.mouse.tracking && game_window.mouse.inside))
            {
              if (SK_ImGui_WantMouseCapture ())
              {
                SK_ImGui_UpdateMouseTracker ();
              }
            }
          } break;

          case WM_LBUTTONDOWN:
          case WM_LBUTTONDBLCLK:
            io.MouseDown [0] = true;
            break;

          case WM_RBUTTONDOWN:
          case WM_RBUTTONDBLCLK:
            io.MouseDown [1] = true;
            break;

          case WM_MBUTTONDOWN:
          case WM_MBUTTONDBLCLK:
            io.MouseDown [2] = true;
            break;

          case WM_XBUTTONDOWN:
          case WM_XBUTTONDBLCLK:
          {
            MOUSEHOOKSTRUCTEX* mhsx =
              (MOUSEHOOKSTRUCTEX*)lParam;

            if ((HIWORD (mhsx->mouseData)) == XBUTTON1) io.MouseDown [3] = true;
            if ((HIWORD (mhsx->mouseData)) == XBUTTON2) io.MouseDown [4] = true;
          } break;
        }
      } break;
    }

    if (SK_ImGui_WantMouseCapture ())
    {
      switch (wParam)
      {
        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONDBLCLK:
        case WM_XBUTTONDOWN:
        case WM_XBUTTONDBLCLK:
        case WM_MOUSEWHEEL:
        case WM_MOUSEHWHEEL:
          SK_WinHook_Backend->markHidden (sk_input_dev_type::Mouse);

          return
            CallNextHookEx (
                nullptr, nCode,
                 wParam, lParam );
      }
    }

    else
    {
      // Game uses a mouse hook for input that the Steam overlay cannot block
      if (SK_GetStoreOverlayState (true))
      {
        SK_WinHook_Backend->markHidden (sk_input_dev_type::Mouse);

        return
          CallNextHookEx (0, nCode, wParam, lParam);
      }

      SK_WinHook_Backend->markRead (sk_input_dev_type::Mouse);

      DWORD dwTid =
        GetCurrentThreadId ();

      using MouseProc =
        LRESULT (CALLBACK *)(int,WPARAM,LPARAM);

      return
        ((MouseProc)__hooks._RealMouseProcs.count (dwTid) ?
                    __hooks._RealMouseProcs.at    (dwTid) :
                    __hooks._RealMouseProc)( nCode, wParam,
                                                    lParam );
    }
  }

  return
    CallNextHookEx (
        nullptr, nCode,
         wParam, lParam );
}

LRESULT
CALLBACK
SK_Proxy_LLMouseProc   (
  _In_ int    nCode,
  _In_ WPARAM wParam,
  _In_ LPARAM lParam )
{
  if (nCode >= 0)
  {
    switch (wParam)
    {
      case WM_MOUSEMOVE:
      case WM_LBUTTONDOWN:
      case WM_LBUTTONDBLCLK:
      case WM_RBUTTONDOWN:
      case WM_RBUTTONDBLCLK:
      case WM_MBUTTONDOWN:
      case WM_MBUTTONDBLCLK:
      case WM_XBUTTONDOWN:
      case WM_XBUTTONDBLCLK:
      {
        MSLLHOOKSTRUCT *mhs =
          (MSLLHOOKSTRUCT *)lParam;

        static auto& io =
          ImGui::GetIO ();

        io.KeyCtrl  |= ((mhs->dwExtraInfo & MK_CONTROL) != 0);
        io.KeyShift |= ((mhs->dwExtraInfo & MK_SHIFT  ) != 0);

        switch (wParam)
        {
          case WM_LBUTTONDOWN:
          case WM_LBUTTONDBLCLK:
            io.MouseDown [0] = true;
            break;

          case WM_RBUTTONDOWN:
          case WM_RBUTTONDBLCLK:
            io.MouseDown [1] = true;
            break;

          case WM_MBUTTONDOWN:
          case WM_MBUTTONDBLCLK:
            io.MouseDown [2] = true;
            break;

          case WM_XBUTTONDOWN:
          case WM_XBUTTONDBLCLK:
            if ((HIWORD (mhs->mouseData)) == XBUTTON1) io.MouseDown [3] = true;
            if ((HIWORD (mhs->mouseData)) == XBUTTON2) io.MouseDown [4] = true;
            break;
        }
      } break;
    }

    if (SK_ImGui_WantMouseCapture ())
    {
      switch (wParam)
      {
        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONDBLCLK:
        case WM_XBUTTONDOWN:
        case WM_XBUTTONDBLCLK:
        case WM_MOUSEWHEEL:
        case WM_MOUSEHWHEEL:
          SK_WinHook_Backend->markHidden (sk_input_dev_type::Mouse);

          return
            CallNextHookEx (
                nullptr, nCode,
                 wParam, lParam );
      }
    }

    else
    {
      // Game uses a mouse hook for input that the Steam overlay cannot block
      if (SK_GetStoreOverlayState (true))
      {
        SK_WinHook_Backend->markHidden (sk_input_dev_type::Mouse);

        return
          CallNextHookEx (0, nCode, wParam, lParam);
      }

      SK_WinHook_Backend->markRead (sk_input_dev_type::Mouse);

      DWORD dwTid =
        GetCurrentThreadId ();

      using MouseProc =
        LRESULT (CALLBACK *)(int,WPARAM,LPARAM);

      return
        ((MouseProc)__hooks._RealMouseProcs.count (dwTid) ?
                    __hooks._RealMouseProcs.at    (dwTid) :
                    __hooks._RealMouseProc)( nCode, wParam,
                                                    lParam );
    }
  }

  return
    CallNextHookEx (
        nullptr, nCode,
         wParam, lParam );
}


LRESULT
CALLBACK
SK_Proxy_KeyboardProc (
  _In_ int    nCode,
  _In_ WPARAM wParam,
  _In_ LPARAM lParam  )
{
  if (nCode >= 0)
  {
    using KeyboardProc =
      LRESULT (CALLBACK *)(int,WPARAM,LPARAM);

    bool wasPressed = (((DWORD)lParam) & (1UL << 30UL)) != 0UL,
          isPressed = (((DWORD)lParam) & (1UL << 31UL)) == 0UL,
          isAltDown = (((DWORD)lParam) & (1UL << 29UL)) != 0UL;

    SHORT vKey =
      static_cast <SHORT> (wParam);

    if ( config.input.keyboard.override_alt_f4 &&
            config.input.keyboard.catch_alt_f4 )
    {
      if (SK_IsGameWindowFocused () && vKey == VK_F4 && isAltDown && isPressed && (! wasPressed))
      {
        SK_ImGui_WantExit = true;

        return 1;
      }
    }

    if (SK_IsGameWindowActive () || (! isPressed))
      ImGui::GetIO ().KeysDown [vKey] = isPressed;

    if (SK_ImGui_WantKeyboardCapture ())
    {
      SK_WinHook_Backend->markHidden (sk_input_dev_type::Keyboard);

      return
        CallNextHookEx (
            nullptr, nCode,
             wParam, lParam );
    }

    else
    {
      // Game uses a keyboard hook for input that the Steam overlay cannot block
      if (SK_GetStoreOverlayState (true) || SK_Console::getInstance ()->isVisible ())
      {
        SK_WinHook_Backend->markHidden (sk_input_dev_type::Keyboard);

        return
          CallNextHookEx (0, nCode, wParam, lParam);
      }

      DWORD dwTid =
        GetCurrentThreadId ();

      SK_WinHook_Backend->markRead (sk_input_dev_type::Keyboard);

      return
        ((KeyboardProc)__hooks._RealKeyboardProcs.count (dwTid) ?
                       __hooks._RealKeyboardProcs.at    (dwTid) :
                       __hooks._RealKeyboardProc)( nCode, wParam,
                                                          lParam );
    }
  }

  return
    CallNextHookEx (
        nullptr, nCode,
         wParam, lParam );
}

LRESULT
CALLBACK
SK_Proxy_LLKeyboardProc (
  _In_ int    nCode,
  _In_ WPARAM wParam,
  _In_ LPARAM lParam  )
{
  if (nCode == HC_ACTION)
  {
    using KeyboardProc =
      LRESULT (CALLBACK *)(int,WPARAM,LPARAM);

    KBDLLHOOKSTRUCT *pHookData =
      (KBDLLHOOKSTRUCT *)lParam;

    bool wasPressed = false,
          isPressed = (wParam == WM_KEYDOWN    || wParam == WM_SYSKEYDOWN),
          isAltDown = (wParam == WM_SYSKEYDOWN || wParam == WM_SYSKEYUP);

    wasPressed = isPressed ^ (bool)((pHookData->flags & 0x7) != 0);

    SHORT vKey =
      static_cast <SHORT> (wParam);

    if ( config.input.keyboard.override_alt_f4 &&
            config.input.keyboard.catch_alt_f4 )
    {
      if (SK_IsGameWindowFocused () && vKey == VK_F4 && isAltDown && isPressed && (! wasPressed))
      {
        SK_ImGui_WantExit = true;

        return 1;
      }
    }

    //
    // Because the game is using a low-level keyboard hook, the chances
    //   that it expects input even when its top-level / render window is
    //     not focused are very high.
    //
    //  Instead of the normal checks for keyboard input focus, in these games
    //    check if the foreground window belongs to the game process.
    // 
    //   * Ignore -which- window is focused; after all, the game's own use of
    //       a low-level hook does not respect window focus in any way.
    //
    bool bWindowActive = 
      SK_IsGameWindowActive ();

    if (! bWindowActive)
    {
      DWORD dwProcId = 0x0;

      GetWindowThreadProcessId (
        SK_GetForegroundWindow (), &dwProcId
      );

      bWindowActive = 
        (dwProcId == GetCurrentProcessId ());
    }

    if (bWindowActive || (! isPressed))
      ImGui::GetIO ().KeysDown [vKey] = isPressed;

    if (SK_ImGui_WantKeyboardCapture ())
    {
      SK_WinHook_Backend->markHidden (sk_input_dev_type::Keyboard);

      return
        CallNextHookEx (
            nullptr, nCode,
             wParam, lParam );
    }

    else
    {
      // Game uses a keyboard hook for input that the Steam overlay cannot block
      if (SK_GetStoreOverlayState (true) || SK_Console::getInstance ()->isVisible ())
      {
        SK_WinHook_Backend->markHidden (sk_input_dev_type::Keyboard);

        return
          CallNextHookEx (0, nCode, wParam, lParam);
      }

      DWORD dwTid =
        GetCurrentThreadId ();

      SK_WinHook_Backend->markRead (sk_input_dev_type::Keyboard);

      return
        ((KeyboardProc)__hooks._RealKeyboardProcs.count (dwTid) ?
                       __hooks._RealKeyboardProcs.at    (dwTid) :
                       __hooks._RealKeyboardProc)( nCode, wParam,
                                                          lParam );
    }
  }

  return
    CallNextHookEx (
        nullptr, nCode,
         wParam, lParam );
}

BOOL
WINAPI
UnhookWindowsHookEx_Detour ( _In_ HHOOK hhk )
{
  for ( auto& hook : __hooks._RealMouseHooks )
  {
    if (hook.second == hhk)
    {
      __hooks._RealMouseHooks.erase (hook.first);
      __hooks._RealMouseProcs.erase (hook.first);

      return
        UnhookWindowsHookEx_Original (hhk);
    }
  }

  if (hhk == __hooks._RealMouseHook)
  {
    __hooks._RealMouseProc = nullptr;
    __hooks._RealMouseHook = nullptr;

    return
      UnhookWindowsHookEx_Original (hhk);
  }

  for ( auto& hook : __hooks._RealKeyboardHooks )
  {
    if (hook.second == hhk)
    {
      __hooks._RealKeyboardHooks.erase (hook.first);
      __hooks._RealKeyboardProcs.erase (hook.first);

      return
        UnhookWindowsHookEx_Original (hhk);
    }
  }

  if (hhk == __hooks._RealKeyboardHook)
  {
    __hooks._RealKeyboardProc = nullptr;
    __hooks._RealKeyboardHook = nullptr;

    return
      UnhookWindowsHookEx_Original (hhk);
  }

  for ( auto& hook : __hooks._RealKeyboardHooks )
  {
    if (hook.second == hhk)
    {
      __hooks._RealKeyboardProcs.erase (hook.first);

      return
        UnhookWindowsHookEx_Original (hhk);
    }
  }

  return
    UnhookWindowsHookEx_Original (hhk);
}

HHOOK
WINAPI
SetWindowsHookExW_Detour (
  int       idHook,
  HOOKPROC  lpfn,
  HINSTANCE hmod,
  DWORD     dwThreadId )
{
  wchar_t                   wszHookMod [MAX_PATH] = { };
  GetModuleFileNameW (hmod, wszHookMod, MAX_PATH);

  switch (idHook)
  {
    case WH_KEYBOARD:
    case WH_KEYBOARD_LL:
    {
      SK_LOG0 ( ( L" <Unicode>: Game module ( %ws ) uses a%wsKeyboard Hook...",
                       wszHookMod,
                        idHook == WH_KEYBOARD_LL ?
                                   L" Low-Level " : L" " ),
                                           L"Input Hook" );

      // Game seems to be using keyboard hooks instead of a normal Window Proc;
      //   that makes life more complicated for SK/ImGui... but we got this!
      if (idHook == WH_KEYBOARD || idHook == WH_KEYBOARD_LL)
      {
        bool install = false;

        if (dwThreadId != 0)
        {
          if (! __hooks._RealKeyboardProcs.count (dwThreadId))
          {     __hooks._RealKeyboardProcs       [dwThreadId] = lpfn;
                                                      install = true;
          }
        }

        else if (__hooks._RealKeyboardProc == nullptr)
        {        __hooks._RealKeyboardProc = lpfn;
                                   install = true;
        }

        if (install)
          lpfn = (idHook == WH_KEYBOARD ? SK_Proxy_KeyboardProc
                                        : SK_Proxy_LLKeyboardProc);
      }
    } break;

    case WH_MOUSE:
    case WH_MOUSE_LL:
    {
      SK_LOG0 ( ( L" <Unicode>: Game module ( %ws ) uses a%wsMouse Hook...",
                 wszHookMod,
                  idHook == WH_MOUSE_LL    ?
                            L" Low-Level " : L" " ),
                                    L"Input Hook" );

      // Game seems to be using mouse hooks instead of a normal Window Proc;
      //   that makes life more complicated for SK/ImGui... but we got this!
      if (idHook == WH_MOUSE || idHook == WH_MOUSE_LL)
      {
        bool install = false;

        if (dwThreadId != 0)
        {
          if (! __hooks._RealMouseProcs.count (dwThreadId))
          {     __hooks._RealMouseProcs       [dwThreadId] = lpfn;
                                                   install = true;
          }
        }

        else if (__hooks._RealMouseProc == nullptr)
        {        __hooks._RealMouseProc = lpfn;
                                install = true;
        }

        if (install)
          lpfn = (idHook == WH_MOUSE ? SK_Proxy_MouseProc
                                     : SK_Proxy_LLMouseProc);
      }
    } break;
  }

  return
    SetWindowsHookExW_Original (
      idHook, lpfn,
              hmod, dwThreadId
    );
}

HHOOK
WINAPI
SetWindowsHookExA_Detour (
  int       idHook,
  HOOKPROC  lpfn,
  HINSTANCE hmod,
  DWORD     dwThreadId )
{
  wchar_t                   wszHookMod [MAX_PATH] = { };
  GetModuleFileNameW (hmod, wszHookMod, MAX_PATH);

  switch (idHook)
  {
    case WH_KEYBOARD:
    case WH_KEYBOARD_LL:
    {
      SK_LOG0 ( ( L" <ANSI>: Game module ( %ws ) uses a%wsKeyboard Hook...",
                       wszHookMod,
                        idHook == WH_KEYBOARD_LL ?
                                   L" Low-Level " : L" " ),
                                           L"Input Hook" );

      // Game seems to be using keyboard hooks instead of a normal Window Proc;
      //   that makes life more complicated for SK/ImGui... but we got this!
      if (idHook == WH_KEYBOARD || idHook == WH_KEYBOARD_LL)
      {
        bool install = false;

        if (dwThreadId != 0)
        {
          if (! __hooks._RealKeyboardProcs.count (dwThreadId))
          {     __hooks._RealKeyboardProcs       [dwThreadId] = lpfn;
                                                      install = true;
          }
        }

        else if (__hooks._RealKeyboardProc == nullptr)
        {        __hooks._RealKeyboardProc = lpfn;
                                   install = true;
        }

        if (install)
          lpfn = (idHook == WH_KEYBOARD ? SK_Proxy_KeyboardProc
                                        : SK_Proxy_LLKeyboardProc);
      }
    } break;

    case WH_MOUSE:
    case WH_MOUSE_LL:
    {
      SK_LOG0 ( ( L" <ANSI>: Game module ( %ws ) uses a%wsMouse Hook...",
                 wszHookMod,
                  idHook == WH_MOUSE_LL    ?
                            L" Low-Level " : L" " ),
                                    L"Input Hook" );

      // Game seems to be using mouse hooks instead of a normal Window Proc;
      //   that makes life more complicated for SK/ImGui... but we got this!
      if (idHook == WH_MOUSE || idHook == WH_MOUSE_LL)
      {
        bool install = false;

        if (dwThreadId != 0)
        {
          if (! __hooks._RealMouseProcs.count (dwThreadId))
          {     __hooks._RealMouseProcs       [dwThreadId] = lpfn;
                                                   install = true;
          }
        }

        else if (__hooks._RealMouseProc == nullptr)
        {        __hooks._RealMouseProc = lpfn;
                                install = true;
        }

        if (install)
          lpfn = (idHook == WH_MOUSE ? SK_Proxy_MouseProc
                                     : SK_Proxy_LLMouseProc);
      }
    } break;
  }

  return
    SetWindowsHookExA_Original (
      idHook, lpfn,
              hmod, dwThreadId
    );
}

void
SK_Input_PreHookWinHook (void)
{
  SK_RunOnce (
  {
    SK_CreateDLLHook2 (      L"User32",
                              "SetWindowsHookExA",
                               SetWindowsHookExA_Detour,
      static_cast_p2p <void> (&SetWindowsHookExA_Original) );

    SK_CreateDLLHook2 (      L"User32",
                              "SetWindowsHookExW",
                               SetWindowsHookExW_Detour,
      static_cast_p2p <void> (&SetWindowsHookExW_Original) );

    SK_CreateDLLHook2 (      L"User32",
                              "UnhookWindowsHookEx",
                               UnhookWindowsHookEx_Detour,
      static_cast_p2p <void> (&UnhookWindowsHookEx_Original) );
  });
}