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
#ifndef __SK__STEAM_API_H__
#define __SK__STEAM_API_H__

#define _CRT_SECURE_NO_WARNINGS
#include <steamapi/steam_api.h>

#include <stdint.h>
#include <string>

namespace SK
{
  namespace SteamAPI
  {
    void Init     (bool preload);
    void Shutdown (void);
    void Pump     (void);

    void __stdcall SetOverlayState (bool active);
    bool __stdcall GetOverlayState (bool real);

    bool __stdcall TakeScreenshot  (void);

    uint32_t    AppID   (void);
    std::string AppName (void);

    // The state that we are explicitly telling the game
    //   about, not the state of the actual overlay...
    extern bool overlay_state;
  }
}

bool __stdcall SK_SteamAPI_TakeScreenshot           (void);
bool __stdcall SK_IsSteamOverlayActive              (void);
                                                    
void           SK_SteamAPI_LogAllAchievements       (void);
void           SK_UnlockSteamAchievement            (uint32_t idx);
                                                    
bool           SK_SteamImported                     (void);
void           SK_TestSteamImports                  (HMODULE hMod);
                                                    
void           SK_HookCSteamworks                   (void);
void           SK_HookSteamAPI                      (void);
                                                    
void           SK_Steam_ClearPopups                 (void);
void           SK_Steam_DrawOSD                     (void);

void           SK_Steam_InitCommandConsoleVariables (void);


#endif /* __SK__STEAM_API_H__ */