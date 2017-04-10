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

    void __stdcall UpdateNumPlayers (void);
    int  __stdcall GetNumPlayers    (void);

    float __stdcall PercentOfAchievementsUnlocked (void);

    bool __stdcall TakeScreenshot  (void);

    uint32_t    AppID   (void);
    std::string AppName (void);

    // The state that we are explicitly telling the game
    //   about, not the state of the actual overlay...
    extern bool overlay_state;
  }
}

//
// Internal data stored in the Achievement Manager, this is
//   the publicly visible data...
//
//   I do not want to expose the poorly designed interface
//     of the full achievement manager outside the DLL,
//       so there exist a few flattened API functions
//         that can communicate with it and these are
//           the data they provide.
//
struct SK_SteamAchievement
{
  // If we were to call ISteamStats::GetAchievementName (...),
  //   this is the index we could use.
  int         idx_;

  const char* name_;          // UTF-8 (I think?)
  const char* human_name_;    // UTF-8
  const char* desc_;          // UTF-8
  
  float       global_percent_;
  
  struct
  {
    int unlocked; // Number of friends who have unlocked
    int possible; // Number of friends who may be able to unlock
  } friends_;
  
  struct
  {
    uint8_t*  achieved;
    uint8_t*  unachieved;
  } icons_;
  
  struct
  {
    int current;
    int max;
  
    __forceinline float getPercent (void)
    {
      return 100.0f * (float)current / (float)max;
    }
  } progress_;
  
  bool        unlocked_;
  __time32_t  time_;
};


size_t SK_SteamAPI_GetNumPossibleAchievements (void);

std::vector <SK_SteamAchievement *>& SK_SteamAPI_GetUnlockedAchievements (void);
std::vector <SK_SteamAchievement *>& SK_SteamAPI_GetLockedAchievements   (void);
std::vector <SK_SteamAchievement *>& SK_SteamAPI_GetAllAchievements      (void);

size_t SK_SteamAPI_GetUnlockedAchievementsForFriend (uint32_t friend_idx, BOOL* pStats);
size_t SK_SteamAPI_GetLockedAchievementsForFriend   (uint32_t friend_idx, BOOL* pStats);
size_t SK_SteamAPI_GetSharedAchievementsForFriend   (uint32_t friend_idx, BOOL* pStats);

// Returns true if all friend stats have been pulled from the server
bool  SK_SteamAPI_FriendStatsFinished  (void);

// Percent (0.0 - 1.0) of friend achievement info fetched
float SK_SteamAPI_FriendStatPercentage (void);

int   SK_SteamAPI_GetNumFriends        (void);


bool __stdcall SK_SteamAPI_TakeScreenshot           (void);
bool __stdcall SK_IsSteamOverlayActive              (void);

void    __stdcall SK_SteamAPI_UpdateNumPlayers      (void);
int32_t __stdcall SK_SteamAPI_GetNumPlayers         (void);

float __stdcall SK_SteamAPI_PercentOfAchievementsUnlocked (void);
                                                    
void           SK_SteamAPI_LogAllAchievements       (void);
void           SK_UnlockSteamAchievement            (uint32_t idx);
                                                    
bool           SK_SteamImported                     (void);
void           SK_TestSteamImports                  (HMODULE hMod);
                                                    
void           SK_HookCSteamworks                   (void);
void           SK_HookSteamAPI                      (void);
                                                    
void           SK_Steam_ClearPopups                 (void);
void           SK_Steam_DrawOSD                     (void);

void           SK_Steam_InitCommandConsoleVariables (void);

uint32_t __stdcall SK_Steam_PiratesAhoy                 (void);


#endif /* __SK__STEAM_API_H__ */