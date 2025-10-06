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

#pragma once

#ifndef __SK__ACHIEVEMENTS_H__
#define __SK__ACHIEVEMENTS_H__

#include <SpecialK/command.h>
#include <EOS/eos_achievements_types.h>

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
struct SK_Achievement
{
  std::string name_ = "";             // UTF-8 (I think?)

  struct text_s
  {
    struct state_s
    {
      std::string human_name = "";  // UTF-8
      std::string desc       = "";  // UTF-8
    } unlocked,
      locked;
  } text_;

  struct tracked_stats_s
  {
    struct trackable_s
    {
      std::string name      =    ""; // Human readable name
      int32_t     current   =     0; // Current stat
      int32_t     threshold =     0; // Unlock after
      bool        trackable = false; // Is this more than a lock/unlock 2-state thing?
    };

    std::vector <trackable_s> data;
  } tracked_stats_;

  // If we were to call ISteamStats::GetAchievementName (...),
  //   this is the index we could use.
  int         idx_            = -1;

  float       global_percent_ = 0.0f;
  __time64_t  time_           =    0;

  struct
  {
    int       unlocked        = 0; // Number of friends who have unlocked
    int       possible        = 0; // Number of friends who may be able to unlock
  } friends_;

  struct
  {
    uint32_t  current         = 0;
    uint32_t  max             = 0;
    double    precalculated   = 0.0;
    uint32_t  last_update_ms  = 0;

    float getPercent (void) const noexcept
    {
      if (precalculated != 0.0)
        return sk::narrow_cast <float> (precalculated);

      return 100.0F * sk::narrow_cast <float> (current) /
                      sk::narrow_cast <float> (max);
    }
  } progress_;

  bool        unlocked_       = false;
  bool        hidden_         = false;

  // Metadata for the Achievement Tracker widget
  bool        tracked_        = false;
  bool        ignored_        = false;

  // Hash value so that unchanged achievements do not
  //   flood log files when logging achievement updates.
  uint32_t    crc32c_         =     0;
};

struct ImGuiWindow;
#include <SpecialK/steam_api.h>

class SK_AchievementManager : public SK_IVariableListener
{
public:
  SK_AchievementManager (void);

  class Achievement : public SK_Achievement
  {
  public:
    Achievement (int idx, const char* szName, ISteamUserStats*               stats);
    Achievement (int idx,                     EOS_Achievements_DefinitionV2* def);
    Achievement (int idx, const char* szName, void*                          stats);

     Achievement (const Achievement& copy) = delete;
    ~Achievement (void)                    = delete;

    void update (ISteamUserStats* stats)
    {
      if (stats == nullptr)
        return;

      stats->GetAchievementAndUnlockTime ( name_.c_str (),
                                          &unlocked_,
                              (uint32_t *)&time_ );
    }

    void update_global (ISteamUserStats* stats)
    {
      if (stats == nullptr)
        return;

      // Reset to 0.0 on read failure
      if (! stats->GetAchievementAchievedPercent (
              name_.c_str (),
                &global_percent_                 )
         )
      {
        steam_log->Log (
          L" Global Achievement Read Failure For '%hs'", name_.c_str ()
        );
        global_percent_ = 0.0f;
      }
    }
  };

  bool OnVarChange (SK_IVariable* var, void* val = nullptr) override;

  float getPercentOfAchievementsUnlocked (void) const;
  int   getNumberOfAchievementsUnlocked  (void) const;

  void             loadSound       (const wchar_t* wszUnlockSound);
  bool             playSound       (void);

  void             addAchievement  (Achievement* achievement);
  Achievement*     getAchievement  (const char* szName                  ) const;
  SK_Achievement** getAchievements (    size_t* pnAchievements = nullptr);

  void             clearPopups     (void);
  int              drawPopups      (void);

  // Make protected once Epic enumerates achievements
  float percent_unlocked = 0.0f;
  int   total_unlocked   =    0;

protected:
  struct SK_AchievementPopup
  {
    IUnknown*       icon_texture = nullptr; // Native graphics API texture handle (e.g. SRV)
    ID3D12Resource* d3d12_tex    = nullptr; // D3D12 Texture, for proper cleanup
    ImGuiWindow*    window       = nullptr;
    Achievement*    achievement  = nullptr;
    DWORD           time         =       0; // Time (milliseconds) when first visible on screen.
    bool            final_pos    =   false; // When the animation is finished, this will be set.
  };

  ImGuiWindow* createPopupWindow (SK_AchievementPopup* popup);

  struct SK_AchievementStorage
  {
    std::vector        <             Achievement*> list;
    std::unordered_map <std::string, Achievement*> string_map;
  } achievements; // SELF

  std::vector <SK_AchievementPopup> popups;
  int                               lifetime_popups;

  SK_IVariable*         achievement_test = nullptr;

  bool                  default_loaded = false;
  std::vector <uint8_t> unlock_sound; // A .WAV (PCM) file
};

class SK_Widget;

SK_Widget*
SK_Widget_GetAchievementTracker (void);

void SK_Platform_DownloadGlobalAchievementStats (void);

#endif /* __SK__ACHIEVEMENTS_H__ */