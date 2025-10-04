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

#ifndef __SK__GOG_H__
#define __SK__GOG_H__

#define GALAXY_NODLL

#include <cstdint>
#include <string>
#include <vector>

#include <SpecialK/log.h>
#include <SpecialK/command.h>

#include <galaxy/IGalaxy.h>

class galaxy::api::IUser;

namespace SK
{
  namespace Galaxy
  {
    void Init     (void);
    void Shutdown (void);

    void  __stdcall SetOverlayState  (bool active);
    bool  __stdcall GetOverlayState  (bool real);
    bool  __stdcall IsOverlayAware   (void); // Did the game install a callback?

    std::string&      AppName         (void);
    std::string_view  PlayerName      (void);
    std::string_view  PlayerNickname  (void);

    LONGLONG GetTicksRetired (void);

    float __stdcall PercentOfAchievementsUnlocked (void);
    int   __stdcall NumberOfAchievementsUnlocked  (void);


    // The state that we are explicitly telling the game
    //   about, not the state of the actual overlay...
    extern bool       overlay_state;
  }
}

bool
__stdcall
SK_Galaxy_GetOverlayState (bool real);



class SK_GalaxyContext
{
public:
  class galaxy::api::IListenerRegistrar;

  virtual ~SK_GalaxyContext (void) noexcept
  {
    if (              sdk_dll_ != nullptr)
      SK_FreeLibrary (sdk_dll_);
  };

  void PreInit                (HMODULE hGalaxyDLL);
  void Init                   (galaxy::api::IStats* stats, galaxy::api::IUtils* utils, galaxy::api::IUser* user, galaxy::api::IListenerRegistrar* registrar);
  //bool InitEpicOnlineServices (HMODULE hEOSDLL, EOS_HPlatform platform = nullptr);

  void Shutdown (bool bGameRequested = false);

  galaxy::api::IStats*             Stats     (void) noexcept { return stats_;     }
  galaxy::api::IFriends*           Friends   (void) noexcept { return friends_;   }
  galaxy::api::IUtils*             Utils     (void) noexcept { return utils_;     }
  galaxy::api::IUser*              User      (void) noexcept { return user_;      }
  galaxy::api::IListenerRegistrar* Registrar (void) noexcept { return registrar_; }

  const char*            GetGalaxyInstallPath (void);
  HMODULE                GetGalaxyDLL         (void) const { return sdk_dll_; }

  std::string_view       GetDisplayName (void) const { return user_names.display_name; }
  std::string_view       GetNickName    (void) const { return user_names.nickname;     }

//protected:
  struct
  {
    std::string display_name;
    std::string nickname;
  } user_names;

  enum versions {
    Version_1_152_1,
    Version_1_152_10
  } version = Version_1_152_1;

private:
  galaxy::api::IGalaxy*            galaxy_    = nullptr;
  galaxy::api::IStats*             stats_     = nullptr;
  galaxy::api::IFriends*           friends_   = nullptr;
  galaxy::api::IUtils*             utils_     = nullptr;
  galaxy::api::IUser*              user_      = nullptr;
  galaxy::api::IListenerRegistrar* registrar_ = nullptr;

  HMODULE                          sdk_dll_   = nullptr;
};

extern SK_LazyGlobal <SK_GalaxyContext> gog;


// The Galaxy client is not capable of reporting this... wtf?!
size_t SK_Galaxy_GetNumPossibleAchievements (void);

void
SK_Galaxy_UnlockAchievement (uint32_t idx);

void
SK_Galaxy_PlayUnlockSound (void);

void
SK_Galaxy_LoadUnlockSound (const wchar_t* wszUnlockSound);

int
SK_Galaxy_DrawOSD (void);

SK_AchievementManager* SK_Galaxy_GetAchievementManager (void);

#include <galaxy/GalaxyID.h>
#include <galaxy/IStats.h>
uint32_t SK_Galaxy_Stats_GetUserTimePlayed (galaxy::api::IStats *);

void     SK_Galaxy_Stats_GetAchievementNameCopy          (galaxy::api::IStats*, uint32_t index, char* buffer, uint32_t bufferLength);
void     SK_Galaxy_Stats_GetAchievement                  (galaxy::api::IStats*, const char* name, bool& unlocked, uint32_t& unlockTime, galaxy::api::GalaxyID userID = galaxy::api::GalaxyID());
void     SK_Galaxy_Stats_GetAchievementDisplayNameCopy   (galaxy::api::IStats*, const char* name, char* buffer, uint32_t bufferLength);
void     SK_Galaxy_Stats_GetAchievementDescriptionCopy   (galaxy::api::IStats*, const char* name, char* buffer, uint32_t bufferLength);
bool     SK_Galaxy_Stats_IsAchievementVisibleWhileLocked (galaxy::api::IStats*, const char* name);
void     SK_Galaxy_Stats_RequestUserTimePlayed           (galaxy::api::IStats*, galaxy::api::GalaxyID userID = galaxy::api::GalaxyID(), galaxy::api::IUserTimePlayedRetrieveListener*           const listener = NULL);
void     SK_Galaxy_Stats_RequestUserStatsAndAchievements (galaxy::api::IStats*, galaxy::api::GalaxyID userID = galaxy::api::GalaxyID(), galaxy::api::IUserStatsAndAchievementsRetrieveListener* const listener = NULL);
void     SK_Galaxy_Stats_SetAchievement                  (galaxy::api::IStats*, const char* name);
void     SK_Galaxy_Stats_ClearAchievement                (galaxy::api::IStats*, const char* name);

#endif /* __SK__GOG_H__ */