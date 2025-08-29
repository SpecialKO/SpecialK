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

#ifndef __SK__CPL_PLATFORM_H__
#define __SK__CPL_PLATFORM_H__

namespace SK
{
  namespace ControlPanel
  {
    namespace Platform
    {
      bool Draw              (void);
      bool DrawMenu          (void);
      bool WarnIfUnsupported (void);
    };
  };
};

// If real, then use the state of the platform's actual overlay rather than the
//   state that Special K fakes in order to pause games...
bool SK_Platform_GetOverlayState (bool real = false);
bool SK_Platform_SetOverlayState (bool active      ); // Returns the previous state
bool SK_Platform_IsOverlayAware  (void);
void SK_Platform_SetNotifyCorner (void);

int32_t SK_Platform_GetNumPlayers (void);

#endif /* __SK__CPL_PLATFORM_H__ */
