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

#ifndef __SK__DEBUG_UTILS_H__
#define __SK__DEBUG_UTILS_H__

#include <Windows.h>
#include "../log.h"

extern iSK_Logger game_debug;

namespace SK
{
  namespace Diagnostics
  {
    namespace Debugger
    {
      bool Allow        (bool bAllow = true);
      void SpawnConsole (void);
    }
  }
}

BOOL WINAPI SK_IsDebuggerPresent (void);

#endif /* __SK__DEBUG_UTILS_H__ */
