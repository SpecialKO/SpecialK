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

#include <SpecialK/window.h>

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

//#include <SpecialK/framerate.h>
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
    SK_DrawExternalOSD ("SpecialK Console", output);
  //}
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