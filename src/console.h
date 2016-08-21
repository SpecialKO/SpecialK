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

#ifndef __SK__CONSOLE_H__
#define __SK__CONSOLE_H__

#include <Windows.h>

#include <string>
#include <vector>

#include <cstdint>

class SK_Console
{
private:
  HANDLE               hMsgPump;
  struct hooks_t {
    HHOOK              keyboard;
    HHOOK              mouse;
  } hooks;

  static SK_Console*   pConsole;

  static char          text [4096];

  static BYTE          keys_ [256];
  static bool          visible;

  static bool          command_issued;
  static std::string   result_str;

  struct command_history_t {
    std::vector <std::string> history;
    size_t                    idx     = -1;
  } static commands;

protected:
  SK_Console (void);

public:
  static SK_Console* getInstance (void);

  void Draw        (void);

  void Start       (void);
  void End         (void);

  HANDLE GetThread (void);

  bool isVisible (void) { return visible; }

  static unsigned int
    __stdcall
    MessagePump (LPVOID hook_ptr);

  static LRESULT
    CALLBACK
    MouseProc (int nCode, WPARAM wParam, LPARAM lParam);

  static LRESULT
    CALLBACK
    KeyboardProc (int nCode, WPARAM wParam, LPARAM lParam);
};

#endif /* __SK__CONSOLE_H__ */