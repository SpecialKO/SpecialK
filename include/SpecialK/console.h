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
public:
//private:
  HANDLE               hMsgPump = nullptr;

  static SK_Console*   pConsole;

  char          text  [4096]   = {      };
                               
  BYTE          keys_ [256]    = {      };
  bool          visible        = false;

  bool          command_issued = false;
  std::string   result_str     = "";

  struct command_history_t {
    std::vector <std::string> history;
    size_t                    idx     = MAXSIZE_T;
  } commands;

protected:
  SK_Console (void);

public:
  static SK_Console* getInstance (void);

  void Draw        (void);

  void Start       (void);
  void End         (void);

  int KeyUp       (BYTE vkCode, LPARAM lParam);
  int KeyDown     (BYTE vkCode, LPARAM lParam);

  void reset       (void); // Call when window activation changes

  bool isVisible (void) { return visible; }
};

void SK_DrawConsole (void);

#endif /* __SK__CONSOLE_H__ */