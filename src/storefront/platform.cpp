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
#include <SpecialK/storefront/xbox.h>
#include <SpecialK/storefront/gog.h>
#include <SpecialK/steam_api.h>

#include <windows.gaming.ui.h>

bool SK_Platform_GetOverlayState (bool real)
{
  using namespace SK;

  bool state =
    ((SteamAPI::AppID () != 0       ) ? SteamAPI::GetOverlayState (real) : false) ||
    ((    EOS::UserID () != nullptr ) ?      EOS::GetOverlayState (real) : false) ||
    ((Galaxy::GetTicksRetired () > 0) ?   Galaxy::GetOverlayState (real) : false);

  if (! state)
  {
    state = SK_Xbox_GetOverlayState (real);
  }

  return state;
}

bool SK_Platform_SetOverlayState (bool active)
{
  using namespace SK;

  const bool previous =
    SK_Platform_GetOverlayState (false);

  if (SteamAPI::AppID () != 0      ) SteamAPI::SetOverlayState (active);
  if (    EOS::UserID () != nullptr)      EOS::SetOverlayState (active);
  // Not fully implemented yet
  if (Galaxy::GetTicksRetired () >0)   Galaxy::SetOverlayState (active);

  return previous;
}

bool SK_Platform_IsOverlayAware (void)
{
  return
    SK::SteamAPI::IsOverlayAware () ||
         SK::EOS::IsOverlayAware () ||
      SK::Galaxy::IsOverlayAware ();
}

void SK_Platform_SetNotifyCorner (void)
{
  // 4 == Don't Care
  if (config.platform.notify_corner != 4)
  {
    if (SK::EOS::UserID () != nullptr) SK_EOS_SetNotifyCorner   ();
    else                               SK_Steam_SetNotifyCorner ();
  }
}

int
SK_Platform_DrawOSD (void)
{
  int ret =
    SK_Steam_DrawOSD ();

  if (! ret)
  {
    ret =
      SK_EOS_DrawOSD ();
  }

  if (! ret)
  {
    ret =
      SK_Galaxy_DrawOSD ();
  }

  return ret;
}

SK_AchievementManager* SK_Steam_GetAchievementManager  (void);
SK_AchievementManager* SK_EOS_GetAchievementManager    (void);
SK_AchievementManager* SK_Galaxy_GetAchievementManager (void);

SK_AchievementManager*
SK_Platform_GetAchievementManager (void)
{
  static SK_AchievementManager*
           pMgr  = nullptr;
  if (     pMgr != nullptr)
    return pMgr;

  if (SK::SteamAPI::GetCallbacksRun () > 0 && SK::SteamAPI::AppID () > 0)
      pMgr = SK_Steam_GetAchievementManager ();

  if (pMgr == nullptr && SK::EOS::GetTicksRetired () > 0)
      pMgr = SK_EOS_GetAchievementManager ();

  if (pMgr == nullptr && SK::Galaxy::GetTicksRetired () > 0)
      pMgr = SK_Galaxy_GetAchievementManager ();

  return pMgr;
}

bool
SK_PlatformOverlay_GoToURL (const char* szURL, bool bUseWindowsShellIfOverlayFails)
{
  if (SK::Galaxy::GetTicksRetired () > 0)
  {
    if (SK_GalaxyOverlay_GoToURL (szURL))
    {
      return true;
    }
  }

  if (SK::SteamAPI::GetCallbacksRun () > 0)
  {
    if (SK_SteamOverlay_GoToURL (szURL))
    {
      return true;
    }
  }

  if (bUseWindowsShellIfOverlayFails)
  {
    SK_ShellExecuteA ( nullptr, "open",
                         szURL, nullptr, nullptr, SW_NORMAL );
    return true;
  }

  return false;
}