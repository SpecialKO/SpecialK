﻿/**
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

#ifndef __SK__EPIC_ONLINE_SERVICES_H__
#define __SK__EPIC_ONLINE_SERVICES_H__

// F*ck it, we'll do it live!
#ifndef EOS_USE_DLLEXPORT
#define EOS_USE_DLLEXPORT 0
#endif
#include <EOS/eos_sdk.h>

#include <cstdint>
#include <string>
#include <vector>

#include <SpecialK/log.h>
#include <SpecialK/command.h>

namespace SK
{
  namespace EOS
  {
    void Init     (bool preload);
    void Shutdown (void);

    void  __stdcall SetOverlayState  (bool active);
    bool  __stdcall GetOverlayState  (bool real);
    bool  __stdcall IsOverlayAware   (void); // Did the game install a callback?

    std::string&      AppName         (void);
    std::string_view  PlayerName      (void);
    std::string_view  PlayerNickname  (void);

    EOS_EpicAccountId UserID          (void);

    LONGLONG          GetTicksRetired (void);

    float __stdcall PercentOfAchievementsUnlocked (void);
    int   __stdcall NumberOfAchievementsUnlocked  (void);


    // The state that we are explicitly telling the game
    //   about, not the state of the actual overlay...
    extern bool       overlay_state;

    struct player_s {
      EOS_EpicAccountId account;
      EOS_ProductUserId user;
    } extern player;

    std::string       GetConfigDir (void);
    std::string       GetDataDir   (void);
  }
}


// Hooks:
//
//  EOS_UI_AddNotifyDisplaySettingsUpdated
//  EOS_UI_RemoveNotifyDisplaySettingsUpdated
//



// Tests the Import Table of hMod for anything Epic-Related
//
//   If found, and this test is performed after the pre-init
//     DLL phase, EOS in one of its many forms will be
//       hooked.
bool
SK_EOS_TestImports (HMODULE hMod);

void
SK_EOS_InitCommandConsoleVariables (void);

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
struct SK_EpicAchievement
{
  const char* name_;          // UTF-8 (I think?)
  const char* human_name_;    // UTF-8
  const char* desc_;          // UTF-8

  // If we were to call ISteamStats::GetAchievementName (...),
  //   this is the index we could use.
  int         idx_;

  float       global_percent_;
  __time32_t  time_;

  struct
  {
    int unlocked; // Number of friends who have unlocked
    int possible; // Number of friends who may be able to unlock
  } friends_;

  struct
  {
    int current;
    int max;

    float getPercent (void) noexcept
    {
      return 100.0F * sk::narrow_cast <float> (current) /
                      sk::narrow_cast <float> (max);
    }
  } progress_;

  bool        unlocked_;
};

size_t SK_EOS_GetNumPossibleAchievements (void);

std::vector <SK_EpicAchievement *>& SK_EOS_GetUnlockedAchievements (void);
std::vector <SK_EpicAchievement *>& SK_EOS_GetLockedAchievements   (void);
std::vector <SK_EpicAchievement *>& SK_EOS_GetAllAchievements      (void);

//float  SK_EOS_GetUnlockedPercentForFriend      (uint32_t friend_idx);
//size_t SK_EOS_GetUnlockedAchievementsForFriend (uint32_t friend_idx, BOOL* pStats);
//size_t SK_EOS_GetLockedAchievementsForFriend   (uint32_t friend_idx, BOOL* pStats);
//size_t SK_EOS_GetSharedAchievementsForFriend   (uint32_t friend_idx, BOOL* pStats);


// Returns true if all friend stats have been pulled from the server
      bool  __stdcall SK_EOS_FriendStatsFinished  (void);

// Percent (0.0 - 1.0) of friend achievement info fetched
     float  __stdcall SK_EOS_FriendStatPercentage (void);

       int  __stdcall SK_EOS_GetNumFriends        (void);
const std::string& // Do not DLL export this, pass a buffer for storage if export is needed
            __stdcall SK_EOS_GetFriendName        (uint32_t friend_idx, size_t* pLen = nullptr);


//bool __stdcall        SK_EOS_TakeScreenshot                (void);
bool __stdcall        SK_IsEpicOverlayActive                    (void);
//bool __stdcall        SK_EpicOverlay_GoToURL                   (const char* szURL, bool bUseWindowsShellIfOverlayFails = false);

float __stdcall       SK_EOS_PercentOfAchievementsUnlocked      (void);
int   __stdcall       SK_EOS_NumberOfAchievementsUnlocked       (void);

void                  SK_EOS_LogAllAchievements                 (void);
void                  SK_UnlockEpicAchievement                  (uint32_t idx);

bool                  SK_EOS_Imported                           (void);

void                  SK_HookEOS                                (void);

EOS_HAchievements     SK_EOS_Achievements                       (void);
EOS_HFriends          SK_EOS_Friends                            (void);

#include <SpecialK/hooks.h>

void SK_EOS_InitManagers            (void);
void SK_EOS_DestroyManagers         (void);



using EOS_Initialize_pfn                                   = EOS_EResult        (EOS_CALL *)(const EOS_InitializeOptions*                                        Options);
using EOS_Shutdown_pfn                                     = EOS_EResult        (EOS_CALL *)(void);
using EOS_Platform_Tick_pfn                                = void               (EOS_CALL *)(      EOS_HPlatform                                                 Handle);
using EOS_Platform_Create_pfn                              = EOS_HPlatform      (EOS_CALL *)(const EOS_Platform_Options*                                         Options);
using EOS_Platform_Release_pfn                             = void               (EOS_CALL *)(      EOS_HPlatform                                                 Handle);

using EOS_UI_SetDisplayPreference_pfn                      = EOS_EResult        (EOS_CALL *)(EOS_HUI                                                             Handle,
                                                                                       const EOS_UI_SetDisplayPreferenceOptions*                                 Options);
using EOS_UI_AddNotifyDisplaySettingsUpdated_pfn           = EOS_NotificationId (EOS_CALL *)(EOS_HUI                                                             Handle,
                                                                                       const EOS_UI_AddNotifyDisplaySettingsUpdatedOptions*                      Options,
                                                                                             void*                                                               ClientData,
                                                                                       const EOS_UI_OnDisplaySettingsUpdatedCallback                             NotificationFn);
using EOS_UI_RemoveNotifyDisplaySettingsUpdated_pfn        = void               (EOS_CALL *)(EOS_HUI                                                             Handle,
                                                                                             EOS_NotificationId                                                  Id);

using EOS_Platform_GetAchievementsInterface_pfn            = EOS_HAchievements  (EOS_CALL *)(EOS_HPlatform                                                       Handle);
using EOS_Platform_GetUIInterface_pfn                      = EOS_HUI            (EOS_CALL *)(EOS_HPlatform                                                       Handle);
using EOS_Platform_GetAuthInterface_pfn                    = EOS_HAuth          (EOS_CALL *)(EOS_HPlatform                                                       Handle);
using EOS_Platform_GetFriendsInterface_pfn                 = EOS_HFriends       (EOS_CALL *)(EOS_HPlatform                                                       Handle);
using EOS_Platform_GetStatsInterface_pfn                   = EOS_HStats         (EOS_CALL *)(EOS_HPlatform                                                       Handle);
using EOS_Platform_GetUserInfoInterface_pfn                = EOS_HUserInfo      (EOS_CALL *)(EOS_HPlatform                                                       Handle);
using EOS_Platform_GetConnectInterface_pfn                 = EOS_HConnect       (EOS_CALL *)(EOS_HPlatform                                                       Handle);

using EOS_ProductUserId_FromString_pfn                     = EOS_ProductUserId  (EOS_CALL *)(const char*                                                         ProductUserIdString);

using EOS_UserInfo_QueryUserInfo_pfn                       = void               (EOS_CALL *)(EOS_HUserInfo                                                       Handle,
                                                                                       const EOS_UserInfo_QueryUserInfoOptions*                                  Options,
                                                                                             void*                                                               ClientData,
                                                                                       const EOS_UserInfo_OnQueryUserInfoCallback                                CompletionDelegate);
using EOS_UserInfo_Release_pfn                             = void               (EOS_CALL *)(EOS_UserInfo*                                                       UserInfo);
using EOS_UserInfo_CopyUserInfo_pfn                        = EOS_EResult        (EOS_CALL *)(EOS_HUserInfo                                                       Handle,
                                                                                       const EOS_UserInfo_CopyUserInfoOptions*                                   Options,
                                                                                             EOS_UserInfo**                                                      OutUserInfo);
using EOS_Achievements_AddNotifyAchievementsUnlockedV2_pfn = EOS_NotificationId (EOS_CALL *)(EOS_HAchievements                                                   Handle,
                                                                                       const EOS_Achievements_AddNotifyAchievementsUnlockedV2Options*            Options,
                                                                                             void*                                                               ClientData,
                                                                                       const EOS_Achievements_OnAchievementsUnlockedCallbackV2                   NotificationFn);
using EOS_Achievements_CopyPlayerAchievementByIndex_pfn    = EOS_EResult        (EOS_CALL *)(EOS_HAchievements                                                   Handle,
                                                                                       const EOS_Achievements_CopyPlayerAchievementByIndexOptions*               Options,
                                                                                             EOS_Achievements_PlayerAchievement**                                OutAchievement);
using EOS_Achievements_GetAchievementDefinitionCount_pfn   = uint32_t           (EOS_CALL *)(EOS_HAchievements                                                   Handle,
                                                                                       const EOS_Achievements_GetAchievementDefinitionCountOptions*              Options);
using EOS_Achievements_QueryDefinitions_pfn                = void               (EOS_CALL *)(EOS_HAchievements                                                   Handle,
                                                                                       const EOS_Achievements_QueryDefinitionsOptions*                           Options,
                                                                                             void*                                                               ClientData,
                                                                                       const EOS_Achievements_OnQueryDefinitionsCompleteCallback                 CompletionDelegate);
using EOS_Achievements_CopyAchievementDefinitionV2ByIndex_pfn
                                                           = EOS_EResult
                                                                                (EOS_CALL *)(EOS_HAchievements                                                   Handle,
                                                                                       const EOS_Achievements_CopyAchievementDefinitionV2ByIndexOptions*         Options,
                                                                                             EOS_Achievements_DefinitionV2**                                     OutDefinition);
using EOS_Achievements_CopyAchievementDefinitionV2ByAchievementId_pfn
                                                           = EOS_EResult        (EOS_CALL *)(EOS_HAchievements                                                   Handle,
                                                                                       const EOS_Achievements_CopyAchievementDefinitionV2ByAchievementIdOptions* Options,
                                                                                             EOS_Achievements_DefinitionV2**                                     OutDefinition);
using EOS_Achievements_DefinitionV2_Release_pfn            = void               (EOS_CALL *)(EOS_Achievements_DefinitionV2*                                      AchievementDefinition);

using EOS_Achievements_GetPlayerAchievementCount_pfn       = uint32_t           (EOS_CALL *)(EOS_HAchievements                                                   Handle,
                                                                                       const EOS_Achievements_GetPlayerAchievementCountOptions*                  Options);
using EOS_Achievements_GetUnlockedAchievementCount_pfn     = uint32_t           (EOS_CALL *)(EOS_HAchievements                                                   Handle,
                                                                                       const EOS_Achievements_GetUnlockedAchievementCountOptions*                Options);
using EOS_Achievements_PlayerAchievement_Release_pfn       = void               (EOS_CALL *)(EOS_Achievements_PlayerAchievement*                                 Achievement);
using EOS_Achievements_QueryPlayerAchievements_pfn         = void               (EOS_CALL *)(EOS_HAchievements                                                   Handle,
                                                                                       const EOS_Achievements_QueryPlayerAchievementsOptions*                    Options,
                                                                                             void*                                                               ClientData,
                                                                                       const EOS_Achievements_OnQueryPlayerAchievementsCompleteCallback          CompletionDelegate);
using EOS_Stats_IngestStat_pfn                             = void               (EOS_CALL *)(EOS_HStats Handle,
                                                                                       const EOS_Stats_IngestStatOptions*                                        Options,
                                                                                             void*                                                               ClientData,
                                                                                       const EOS_Stats_OnIngestStatCompleteCallback                              CompletionDelegate);

using EOS_Auth_GetLoggedInAccountsCount_pfn                = int32_t            (EOS_CALL *)(EOS_HAuth                                                           Handle);
using EOS_Auth_GetLoggedInAccountByIndex_pfn               = EOS_EpicAccountId  (EOS_CALL *)(EOS_HAuth                                                           Handle,
                                                                                             int32_t                                                             Index);

using EOS_Connect_GetLoggedInUserByIndex_pfn               = EOS_ProductUserId  (EOS_CALL *)(EOS_HConnect                                                        Handle,
                                                                                             int32_t                                                             Index);


class SK_EOSContext : public SK_IVariableListener
{
public:
  virtual ~SK_EOSContext (void) noexcept
  {
    if (              sdk_dll_ != nullptr)
      SK_FreeLibrary (sdk_dll_);
  };

  bool OnVarChange (SK_IVariable* var, void* val = nullptr) override;

  void PreInit                (HMODULE hEOSDLL);
  bool InitEpicOnlineServices (HMODULE hEOSDLL, EOS_HPlatform platform = nullptr);

  void Shutdown (bool bGameRequested = false);

  EOS_HAchievements    Achievements         (void) noexcept { return achievements_;       }
  EOS_HUserInfo        UserInfo             (void) noexcept { return user_info_;          }
  EOS_HStats           Stats                (void) noexcept { return stats_;              }
  EOS_HFriends         Friends              (void) noexcept { return friends_;            }
  EOS_HPlatform        Platform             (void) noexcept { return platform_;           }
  EOS_HAuth            Auth                 (void) noexcept { return auth_;               }
  EOS_HConnect         Connect              (void) noexcept { return connect_;            }
  EOS_HUI              UI                   (void) noexcept { return ui_;                 }

  EOS_ProductUserId    UserId               (void) noexcept { return product_user_id_;    }

  SK_IVariable*        popup_origin   = nullptr;
  SK_IVariable*        notify_corner  = nullptr;

  // Backing storage for the human readable variable names,
  //   the actual system uses an integer value but we need
  //     storage for the cvars.
  struct {
    char popup_origin  [16] = { "DontCare" };
    char notify_corner [16] = { "DontCare" };
  } var_strings;

  EOS_ProductUserId    product_user_id_ = nullptr;

  const char*          GetEpicInstallPath (void);
  HMODULE              GetEOSDLL          (void) const { return sdk_dll_; }

  std::string_view     GetDisplayName (void) const { return user.display_name;  }
  std::string_view     GetNickName    (void) const { return user.nickname;      }


  static EOS_Platform_GetAchievementsInterface_pfn            Platform_GetAchievementsInterface;
  static EOS_Platform_GetAuthInterface_pfn                    Platform_GetAuthInterface;
  static EOS_Platform_GetFriendsInterface_pfn                 Platform_GetFriendsInterface;
  static EOS_Platform_GetStatsInterface_pfn                   Platform_GetStatsInterface;
  static EOS_Platform_GetUIInterface_pfn                      Platform_GetUIInterface;
  static EOS_Platform_GetUserInfoInterface_pfn                Platform_GetUserInfoInterface;
  static EOS_Platform_GetConnectInterface_pfn                 Platform_GetConnectInterface;

  static EOS_Auth_GetLoggedInAccountsCount_pfn                Auth_GetLoggedInAccountsCount;
  static EOS_Auth_GetLoggedInAccountByIndex_pfn               Auth_GetLoggedInAccountByIndex;

  static EOS_UserInfo_QueryUserInfo_pfn                       UserInfo_QueryUserInfo;
  static EOS_UserInfo_CopyUserInfo_pfn                        UserInfo_CopyUserInfo;
  static EOS_UserInfo_Release_pfn                             UserInfo_Release;

  static EOS_ProductUserId_FromString_pfn                     ProductUserId_FromString;
  static EOS_Connect_GetLoggedInUserByIndex_pfn               Connect_GetLoggedInUserByIndex;

  static EOS_UI_SetDisplayPreference_pfn                      UI_SetDisplayPreference;

//protected:
  struct
  {
    std::string display_name;
    std::string nickname;
  } user;

private:
  EOS_HPlatform        platform_        = nullptr;

  EOS_HAchievements    achievements_    = nullptr;
  EOS_HUserInfo        user_info_       = nullptr;
  EOS_HStats           stats_           = nullptr;
  EOS_HFriends         friends_         = nullptr;
  EOS_HUI              ui_              = nullptr;
  EOS_HAuth            auth_            = nullptr;
  EOS_HConnect         connect_         = nullptr;

  HMODULE              sdk_dll_         = nullptr;
};

extern SK_LazyGlobal <SK_EOSContext> epic;

void
SK_EOS_SetNotifyCorner (void);


void
__stdcall
SK_EOS_SetOverlayState (bool active);

bool
__stdcall
SK_EOS_GetOverlayState (bool real);


void
SK_EOS_UnlockAchievement (uint32_t idx);

void
SK_EOS_PlayUnlockSound (void);

void
SK_EOS_LoadUnlockSound (const wchar_t* wszUnlockSound);

SK_AchievementManager* SK_EOS_GetAchievementManager (void);

////std::wstring
////SK_Steam_GetApplicationManifestPath (AppId_t appid = 0);
////
////uint32_t
////SK_Steam_GetAppID_NoAPI (void);
////
////std::string
////SK_UseManifestToGetAppName (AppId_t appid = 0);

int
SK_EOS_DrawOSD (void);


#endif /* __SK__EPIC_ONLINE_SERVICES_H__ */