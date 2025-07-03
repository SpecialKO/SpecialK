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

#ifndef __SK__XBOX_H__
#define __SK__XBOX_H__

#include <windows.gaming.ui.h>

#include <cstdint>
#include <string>
#include <vector>

#include <SpecialK/log.h>
#include <SpecialK/command.h>

namespace SK
{
  namespace Xbox
  {
    void Init     (void);
    void Shutdown (void);

    bool  __stdcall GetOverlayState (bool real);
  }
}

bool
__stdcall
SK_Xbox_GetOverlayState (bool real);


#endif /* __SK__XBOX_H__ */