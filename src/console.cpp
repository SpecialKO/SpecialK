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
#include <SpecialK/widgets/widget.h>

#include <mmsystem.h>
#pragma comment (lib, "winmm.lib")

#include <comdef.h>
#include <process.h>

#include <windowsx.h>

SK_Console::SK_Console (void)
{
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
  static LARGE_INTEGER last_time = { 0ULL };

           std::string output    = "";

  if (visible)
  {
    output += text;

    LARGE_INTEGER now;
    LARGE_INTEGER freq;

    QueryPerformanceFrequency        (&freq);

    extern LARGE_INTEGER SK_QueryPerf (void);
    now = SK_QueryPerf ();

    // Blink the Carret
    if ((now.QuadPart - last_time.QuadPart) > (freq.QuadPart / 3))
    {
      carret = ! carret;

      last_time.QuadPart = now.QuadPart;
    }

    if (carret)
      output += "-";

    // Show Command Results
    if (command_issued)
    {
      output += "\n";
      output += result_str;
    }
  }

  else
  {
    output = "";
  }

  SK_DrawExternalOSD ("SpecialK Console", output);
}

void
SK_Console::Start (void)
{
  static HMODULE hModPPrinny =
    GetModuleHandle (L"PrettyPrinny.dll");

  // STUPID HACK UNTIL WE PROPERLY UNIFY SK AND TSFIX'S CONSOLE.
  if (hModPPrinny)
  {
    bNoConsole = true;
    return;
  }

  ZeroMemory (text, 4096);

  text [0] = '>';
}

void
SK_Console::End (void)
{
  static HMODULE hModPPrinny =
    GetModuleHandle (L"PrettyPrinny.dll");

  // STUPID HACK UNTIL WE PROPERLY UNIFY SK AND TZFIX'S CONSOLE.
  if (hModPPrinny)
  {
    bNoConsole = true;
  }
}

void
SK_Console::reset (void)
{
  memset (keys_, 0, 256);
}

// Plugins can hook this if they do not have their own input handler
__declspec (noinline)
void
CALLBACK
SK_PluginKeyPress (BOOL Control, BOOL Shift, BOOL Alt, BYTE vkCode)
{
  UNREFERENCED_PARAMETER (Control);
  UNREFERENCED_PARAMETER (Shift);
  UNREFERENCED_PARAMETER (Alt);

  SK_ImGui_Widgets.DispatchKeybinds (Control, Shift, Alt, vkCode);
}

BOOL
SK_ImGui_KeyPress (BOOL Control, BOOL Shift, BOOL Alt, BYTE vkCode)
{
  UNREFERENCED_PARAMETER (Alt);

SHORT SK_ImGui_ToggleKeys [4] = {
    VK_BACK,              //Primary button (backspace)
  
    // Modifier Keys
    VK_CONTROL, VK_SHIFT, // Ctrl + Shift
    0
  };

  if ( vkCode == SK_ImGui_ToggleKeys [0] &&
       Control && Shift )
  {
    extern void SK_ImGui_Toggle (void);

    SK_ImGui_Toggle ();
    return FALSE;
  }

  return TRUE;
}

extern SHORT SK_ImGui_ToggleKeys [4];

int
SK_HandleConsoleKey (bool keyDown, BYTE vkCode, LPARAM lParam)
{
  bool&                          visible        = SK_Console::getInstance ()->visible;
  char*                          text           = SK_Console::getInstance ()->text;
  BYTE*                          keys_          = SK_Console::getInstance ()->keys_;
  SK_Console::command_history_t& commands       = SK_Console::getInstance ()->commands;
  bool&                          command_issued = SK_Console::getInstance ()->command_issued;
  std::string&                   result_str     = SK_Console::getInstance ()->result_str;

  // This is technically mouse button 0 (left mouse button on default systems),
  //  but for convenience we allow indexing this array using 0 to have a constant
  //    state, allowing algorithms to use 0 to mean "don't care."
  keys_ [0] = TRUE;


  // Short-hand Lambda for make/break input processing
  auto ProcessKeyPress = [&](BYTE vkCode) ->
  bool
  {
    bool new_press =
     keys_ [vkCode] != 0x81;

    keys_ [vkCode] = 0x81;

    if (new_press)
    {
      // First give ImGui a chance to process this
      if (SK_ImGui_KeyPress (keys_ [VK_CONTROL], keys_ [VK_SHIFT], keys_ [VK_MENU], vkCode))
      {
        // Then give any plug-ins a chance
        SK_PluginKeyPress   (keys_ [VK_CONTROL], keys_ [VK_SHIFT], keys_ [VK_MENU], vkCode);

        // Finally, toggle the command console
        if (keys_ [VK_CONTROL] && keys_ [VK_SHIFT] && vkCode == VK_TAB)
        {
          visible = ! visible;

          // This will pause/unpause the game
          SK::SteamAPI::SetOverlayState (visible);

          return 1;
        }
      }

      else
        return 1;
    }

    return 0;
  };


  if (! SK_IsSteamOverlayActive ())
  {
    // Proprietary HACKJOB:  lParam = MAXDWORD indicates a make/break event, not meant for text input
    if (lParam == MAXDWORD && keyDown)
    {
      return (! ProcessKeyPress (vkCode));
    }


    //BYTE   vkCode   = LOWORD (wParam) & 0xFF;
    BYTE     scanCode = HIWORD (lParam) & 0x7F;
    ///SHORT repeated = LOWORD (lParam);
    //bool   keyDown  = ! (lParam & 0x80000000UL);


    // Disable the command console if the ImGui overlay is visible
    if (SK_ImGui_Active ())
      visible = false;


    if (visible && vkCode == VK_BACK)
    {
      if (keyDown)
      {
        size_t len = strlen (text);
               len--;

        if (len < 1)
          len = 1;

        text [len] = '\0';
      }
    }

    else if (vkCode == VK_SHIFT || vkCode == VK_LSHIFT || vkCode == VK_RSHIFT)
    {
      vkCode = VK_SHIFT;

      if (keyDown) keys_ [vkCode] = 0x81;
      else         keys_ [vkCode] = 0x00;
    }

    else if (vkCode == VK_MENU || vkCode == VK_LMENU || vkCode == VK_RMENU)
    {
      vkCode = VK_MENU;

      if (keyDown) keys_ [vkCode] = 0x81;
      else         keys_ [vkCode] = 0x00;
    }

    //else if ((! repeated) && vkCode == VK_CAPITAL) {
      //if (keyDown) if (keys_ [VK_CAPITAL] == 0x00) keys_ [VK_CAPITAL] = 0x81; else keys_ [VK_CAPITAL] = 0x00;
    //}

    else if (vkCode == VK_CONTROL || vkCode == VK_LCONTROL || vkCode == VK_RCONTROL)
    {
      vkCode = VK_CONTROL;

      if (keyDown) keys_ [vkCode] = 0x81;
      else         keys_ [vkCode] = 0x00;
    }

    else if (keyDown && visible && ((vkCode == VK_UP) || (vkCode == VK_DOWN)))
    {
      if (vkCode == VK_UP)
        commands.idx--;
      else
        commands.idx++;

      // Clamp the index
      if (commands.idx < 0)
        commands.idx = 0;
      else if (commands.idx >= commands.history.size ())
        commands.idx = commands.history.size () - 1;

      if (! commands.history.empty ())
      {
        strcpy (&text [1], commands.history [commands.idx].c_str ());
        command_issued = false;
      }
    }

    else if (visible && vkCode == VK_RETURN)
    {
      bool new_press = keys_ [vkCode] != 0x81;

      if (keyDown)
        keys_ [vkCode] = 0x81;
      else
        keys_ [vkCode] = 0x00;

      if (keyDown && ( (lParam & 0x40000000UL) == 0 || new_press ))
      {
        size_t len = strlen (text+1);
        // Don't process empty or pure whitespace command lines
        if (len > 0 && strspn (text+1, " ") != len)
        {
          SK_ICommandResult result =
            SK_GetCommandProcessor ()->ProcessCommandLine (text+1);

          if (result.getStatus ())
          {
            // Don't repeat the same command over and over
            if (commands.history.empty() ||
                commands.history.back () != &text [1])
            {
              commands.history.emplace_back (&text [1]);
            }

            commands.idx =
              commands.history.size ();

            text [1] = '\0';

            command_issued = true;
          }

          else
          {
            command_issued = false;
          }

          result_str = result.getWord () + std::string (" ")   +
                       result.getArgs () + std::string (":  ") +
                       result.getResult ();
        }
      }
    }

    else if (keyDown)
    {
      // First trigger an event if this is a make/break
      if (ProcessKeyPress (vkCode))
        return 0;

      // If not, use the key press for text input
      //

      // Don't print the tab character, it's pretty useless.
      if ( visible && vkCode != VK_TAB )
      {
        keys_ [VK_CAPITAL] = GetKeyState_Original (VK_CAPITAL) & 0xFFUL;

        unsigned char key_str [2];
                      key_str [1] = '\0';

        if (1 == ToAsciiEx ( vkCode,
                              scanCode,
                              keys_,
   reinterpret_cast <LPWORD> (key_str),
                              0,
                              GetKeyboardLayout (0) ) &&
             isprint ( *key_str ) )
        {
          strncat (text, reinterpret_cast <char *> (key_str), 1);
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

void
SK_DrawConsole (void)
{
  if (bNoConsole)
    return;

  SK_Console::getInstance ()->Draw ();
}

BOOL
__stdcall
SK_IsConsoleVisible (void)
{
  return
    SK_Console::getInstance ()->isVisible ();
}

SK_Console* SK_Console::pConsole = nullptr;