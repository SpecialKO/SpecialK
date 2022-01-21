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
#include <SpecialK/storefront/epic.h>
#include <SpecialK/steam_api.h>

void SK_Platform_SetNotifyCorner (void)
{
  // 4 == Don't Care
  if (config.steam.notify_corner != 4)
  {
    if (SK::EOS::UserID () != nullptr) SK_EOS_SetNotifyCorner   ();
    else                               SK_Steam_SetNotifyCorner ();
  }
}