// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
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
#include <storefront/epic.h>
#include <storefront/achievements.h>

#include "EOS/eos_auth.h"

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"EpicOnline"

class SK_EOS_AchievementManager : public SK_AchievementManager
{
public:
  void unlock (const char* szAchievement)
  {
    if (szAchievement == nullptr)
      return;

    size_t index   = (size_t)-1;
    bool   numeric = true;

    for ( const char* ch = szAchievement ;
                     *ch != '\0'         ;
                      ch = CharNextA (ch) )
    {
      if ((! isalnum (ch [0])) || isalpha (ch [0]))
      {
        numeric = false;
        break;
      }
    }

    if (numeric)
    {
      index = atoi (szAchievement);
    }

    for (uint32 i = 0; i < SK_EOS_GetNumPossibleAchievements (); i++)
    {
      Achievement* achievement =
                   achievements.list [i];

      if (achievement == nullptr || achievement->name_.empty ())
        continue;

      if (! _stricmp (achievement->name_.c_str (), szAchievement))
      {
        index = i;
        break;
      }
    }

    if (index >= SK_EOS_GetNumPossibleAchievements ())
      return;

    Achievement* achievement =
                 achievements.list [index];

    if ( config.platform.achievements.popup.show
              && achievement != nullptr )
    {
      if (platform_popup_cs != nullptr)
          platform_popup_cs->lock ();

      SK_AchievementPopup popup = { };

      // Little bit of sanity goes a long way
      if (achievement->progress_.current ==
          achievement->progress_.max)
      {
        // It's implicit
        achievement->unlocked_ = true;

        if (achievement->time_ == 0)
            achievement->time_ = time (nullptr);
      }

      popup.window      = nullptr;
      popup.final_pos   = false;
      popup.achievement = achievement;

      popups.push_back (popup);

      if (platform_popup_cs != nullptr)
          platform_popup_cs->unlock ();
    }

    if (config.platform.achievements.play_sound && (! unlock_sound.empty ()))
    {
      playSound ();
    }

    // If the user wants a screenshot, but no popups (why?!), this is when
    //   the screenshot needs to be taken.
    if (       config.platform.achievements.take_screenshot )
    {  if ( (! config.platform.achievements.popup.show) )
       {
         SK::SteamAPI::TakeScreenshot (
           SK_ScreenshotStage::EndOfFrame, false,
             SK_FormatString ("Achievements\\%hs", achievement->text_.unlocked.human_name.c_str ())
         );
       }
    }

    log_all_achievements ();
  }

  void log_all_achievements (void) const
  {
    for (uint32 i = 0; i < SK_EOS_GetNumPossibleAchievements (); i++)
    {
      const Achievement* achievement =
                         achievements.list [i];

      if (achievement == nullptr || achievement->name_.empty ())
        continue;

      epic_log->LogEx  (false, L"\n [%c] Achievement %03lu......: '%hs'\n",
                        achievement->unlocked_ ? L'X' : L' ',
                     i, achievement->name_.data ());
      epic_log->LogEx  (false,
                        L"  + Human Readable Name...: %hs\n",
                        achievement->      unlocked_                    ?
                        achievement->text_.unlocked.human_name.c_str () :
                        achievement->text_.  locked.human_name.c_str ());
      if (! (achievement->unlocked_ && achievement->text_.locked.desc.empty ()))
      {
        epic_log->LogEx (false,
                        L"  *- Detailed Description.: %hs\n",
                        achievement->text_.locked.desc.c_str ());
      }
      else if ((achievement->unlocked_ && !achievement->text_.unlocked.desc.empty ()))
      {
        epic_log->LogEx (false,
                        L"  *- Detailed Description.: %hs\n",
                        achievement->text_.unlocked.desc.c_str ());
      }

      if (achievement->global_percent_ > 0.0f)
      {
        epic_log->LogEx (false,
                         L"  #-- Rarity (Global).....: %6.2f%%\n",
                         achievement->global_percent_);
      }
      if (achievement->friends_.possible > 0)
      {
        epic_log->LogEx (false,
                         L"  #-- Rarity (Friend).....: %6.2f%%\n",
          100.0 * (static_cast <double> (achievement->friends_.unlocked) /
                   static_cast <double> (achievement->friends_.possible)) );
      }

      if (achievement->progress_.current != achievement->progress_.max &&
          achievement->progress_.max     != 1) // Don't log achievements that are simple yes/no
      {
        epic_log->LogEx (false,
                         L"  @--- Progress To Unlock.: % 6.2f%%　　[ %d / %d ]\n",
          100.0 * (static_cast <double> (achievement->progress_.current) /
                   static_cast <double> (achievement->progress_.max)),
                                         achievement->progress_.current,
                                         achievement->progress_.max);
      }

      if (achievement->unlocked_)
      {
        epic_log->LogEx (false,
                         L"  @--- Player Unlocked At.: %s",
                         _wctime64 (&achievement->time_));
      }
    }

    epic_log->LogEx (false, L"\n");
  }

  static EOS_Achievements_GetPlayerAchievementCount_pfn                  GetPlayerAchievementCount;
  static EOS_Achievements_GetUnlockedAchievementCount_pfn                GetUnlockedAchievementCount;
  static EOS_Achievements_GetAchievementDefinitionCount_pfn              GetAchievementDefinitionCount;
  static EOS_Achievements_AddNotifyAchievementsUnlockedV2_pfn            AddNotifyAchievementsUnlockedV2;
  static EOS_Achievements_CopyPlayerAchievementByIndex_pfn               CopyPlayerAchievementByIndex;
  static EOS_Achievements_QueryPlayerAchievements_pfn                    QueryPlayerAchievements;
  static EOS_Achievements_PlayerAchievement_Release_pfn                  PlayerAchievement_Release;
  static EOS_Achievements_CopyAchievementDefinitionV2ByIndex_pfn         CopyAchievementDefinitionV2ByIndex;
  static EOS_Achievements_CopyAchievementDefinitionV2ByAchievementId_pfn CopyAchievementDefinitionV2ByAchievementId;
  static EOS_Achievements_QueryDefinitions_pfn                           QueryDefinitions;
  static EOS_Achievements_DefinitionV2_Release_pfn                       DefinitionV2_Release;
};

void EOS_CALL
SK_EOS_UI_OnDisplaySettingsUpdatedCallback (const EOS_UI_OnDisplaySettingsUpdatedCallbackInfo* Data);

class SK_EOS_OverlayManager
{
public:
  bool isActive (void) const {
    return active_;
  }

  void OnActivate (const EOS_UI_OnDisplaySettingsUpdatedCallbackInfo* Data)
  {
    std::lock_guard <SK_Thread_HybridSpinlock>
         lock (callback_cs);

    // If the game has an activation callback installed, then
    //   it's also going to see this event... make a note of that when
    //     tracking its believed overlay state.
    if (SK::EOS::IsOverlayAware ())
        SK::EOS::overlay_state = (Data->bIsExclusiveInput);


    // If we want to use this as our own, then don't let the Epic overlay
    //   unpause the game on deactivation unless the control panel is closed.
    if (config.platform.reuse_overlay_pause && SK_Platform_IsOverlayAware ())
    {
      // Deactivating, but we might want to hide this event from the game...
      if (Data->bIsVisible == 0)
      {
        // Undo the event the game is about to receive.
        if (SK_ImGui_Visible) SK_Platform_SetOverlayState (true);
      }
    }

    const bool wasActive =
                  active_;

    active_ = (Data->bIsExclusiveInput != 0);

    if (wasActive != active_)
    {
      auto& io =
        ImGui::GetIO ();

      static bool capture_keys  = io.WantCaptureKeyboard;
      static bool capture_text  = io.WantTextInput;
      static bool capture_mouse = io.WantCaptureMouse;
      static bool nav_active    = io.NavActive;

      // When the overlay activates, stop blocking
      //   input !!
      if (! wasActive)
      {
        capture_keys  =
          io.WantCaptureKeyboard;
          io.WantCaptureKeyboard = false;

        capture_text  =
          io.WantTextInput;
          io.WantTextInput       = false;

        capture_mouse =
          io.WantCaptureMouse;
          io.WantCaptureMouse    = false;

        nav_active    =
          io.NavActive;
          io.NavActive           = false;

         ImGui::SetWindowFocus (nullptr);
      }

      else
      {
        io.WantCaptureKeyboard = SK_ImGui_Visible ? capture_keys  : false;
        io.WantCaptureMouse    = SK_ImGui_Visible ? capture_mouse : false;
        io.NavActive           = SK_ImGui_Visible ? nav_active    : false;

        ImGui::SetWindowFocus (nullptr);
        io.WantTextInput       = false;//capture_text;
      }
    }
  }

  void invokeCallbacks (bool active)
  {
    std::lock_guard <SK_Thread_HybridSpinlock>
         lock (callback_cs);

    EOS_UI_OnDisplaySettingsUpdatedCallbackInfo
      cbi                   = {    };
      cbi.bIsVisible        = active;
      cbi.bIsExclusiveInput = active;

    for ( const auto &[id,callback] : callbacks )
    {
      cbi.ClientData =
        callback.ClientData;

      callback.NotificationFn (&cbi);
    }
  }

  EOS_NotificationId
  AddNotifyDisplaySettingsUpdated (EOS_HUI                                        Handle,
                             const EOS_UI_AddNotifyDisplaySettingsUpdatedOptions* Options,
                                   void*                                          ClientData,
                             const EOS_UI_OnDisplaySettingsUpdatedCallback        NotificationFn)
  {
    SK_LOG_FIRST_CALL

    SK_ReleaseAssert (Options == nullptr || Options->ApiVersion == EOS_UI_ADDNOTIFYDISPLAYSETTINGSUPDATED_API_LATEST);

    static const EOS_NotificationId id =
      AddNotifyDisplaySettingsUpdated_Original (Handle, Options, ClientData, SK_EOS_UI_OnDisplaySettingsUpdatedCallback);

    std::lock_guard <SK_Thread_HybridSpinlock>
         lock (callback_cs);

    if (id != 0)
    {
      static EOS_NotificationId real_id = id;

      ++real_id;

      callbacks [real_id] =
        { real_id, Handle, ClientData, NotificationFn, *Options };

      return real_id;
    }

    return id;
  }

  void
  RemoveNotifyDisplaySettingsUpdated (EOS_HUI            Handle,
                                      EOS_NotificationId Id)
  {
  //RemoveNotifyDisplaySettingsUpdated_Original (Handle, Id);

    std::lock_guard <SK_Thread_HybridSpinlock>
         lock (callback_cs);

    if ( const auto cb  = callbacks.find (Id);
                    cb != callbacks.end  (  ) )
    {
      SK_ReleaseAssert (Handle == cb->second.Handle);

      callbacks.erase (cb);
    }
  }

  bool
  isOverlayAware (void) const
  {
    return
      (! callbacks.empty ());
  }

private:
  bool cursor_visible_ = false; // Cursor visible prior to activation?
  bool active_         = false;

  struct callback_s
  {
    EOS_NotificationId                            Id;
    EOS_HUI                                       Handle;
    void*                                         ClientData;
    EOS_UI_OnDisplaySettingsUpdatedCallback       NotificationFn;
    EOS_UI_AddNotifyDisplaySettingsUpdatedOptions Options;
  };

public:
  static EOS_UI_AddNotifyDisplaySettingsUpdated_pfn    AddNotifyDisplaySettingsUpdated_Original;
  static EOS_UI_RemoveNotifyDisplaySettingsUpdated_pfn RemoveNotifyDisplaySettingsUpdated_Original;

  std::unordered_map <EOS_NotificationId, callback_s> callbacks;
  SK_Thread_HybridSpinlock                            callback_cs;
};

SK_LazyGlobal <SK_EOS_OverlayManager>     eos_overlay;
SK_LazyGlobal <SK_EOS_AchievementManager> eos_achievements;

SK_AchievementManager* SK_EOS_GetAchievementManager (void)
{
  return
    eos_achievements.getPtr ();
}

EOS_UI_AddNotifyDisplaySettingsUpdated_pfn                      SK_EOS_OverlayManager::AddNotifyDisplaySettingsUpdated_Original       = nullptr;
EOS_UI_RemoveNotifyDisplaySettingsUpdated_pfn                   SK_EOS_OverlayManager::RemoveNotifyDisplaySettingsUpdated_Original    = nullptr;

EOS_Achievements_QueryPlayerAchievements_pfn                    SK_EOS_AchievementManager::QueryPlayerAchievements                    = nullptr;
EOS_Achievements_GetPlayerAchievementCount_pfn                  SK_EOS_AchievementManager::GetPlayerAchievementCount                  = nullptr;
EOS_Achievements_PlayerAchievement_Release_pfn                  SK_EOS_AchievementManager::PlayerAchievement_Release                  = nullptr;
EOS_Achievements_GetUnlockedAchievementCount_pfn                SK_EOS_AchievementManager::GetUnlockedAchievementCount                = nullptr;
EOS_Achievements_CopyPlayerAchievementByIndex_pfn               SK_EOS_AchievementManager::CopyPlayerAchievementByIndex               = nullptr;
EOS_Achievements_GetAchievementDefinitionCount_pfn              SK_EOS_AchievementManager::GetAchievementDefinitionCount              = nullptr;
EOS_Achievements_AddNotifyAchievementsUnlockedV2_pfn            SK_EOS_AchievementManager::AddNotifyAchievementsUnlockedV2            = nullptr;
EOS_Achievements_CopyAchievementDefinitionV2ByIndex_pfn         SK_EOS_AchievementManager::CopyAchievementDefinitionV2ByIndex         = nullptr;
EOS_Achievements_CopyAchievementDefinitionV2ByAchievementId_pfn SK_EOS_AchievementManager::CopyAchievementDefinitionV2ByAchievementId = nullptr;
EOS_Achievements_QueryDefinitions_pfn                           SK_EOS_AchievementManager::QueryDefinitions                           = nullptr;
EOS_Achievements_DefinitionV2_Release_pfn                       SK_EOS_AchievementManager::DefinitionV2_Release                       = nullptr;

// Cache this instead of getting it from the Steam client constantly;
//   doing that is far more expensive than you would think.
size_t
SK_EOS_GetNumPossibleAchievements (void)
{
  if (eos_achievements->GetAchievementDefinitionCount == nullptr)
    return 0;

  static std::pair <size_t,bool> possible =
  { 0, false };

  if ( possible.second        == false  &&
       epic->Achievements () != nullptr && epic->UserId () != 0 )
  {
    //epic_achievements->getAchievements (&possible.first);
    //possible = { possible.first, true };

    constexpr EOS_Achievements_GetAchievementDefinitionCountOptions opt = {
              EOS_ACHIEVEMENTS_GETACHIEVEMENTDEFINITIONCOUNT_API_LATEST   };

    possible.first =
      eos_achievements->GetAchievementDefinitionCount (
        epic->Achievements (), &opt
      );

    if (possible.first != 0)
        possible.second = true;
  }

  return possible.first;
}

float
__stdcall
SK_EOS_PercentOfAchievementsUnlocked (void)
{
  return eos_achievements->getPercentOfAchievementsUnlocked ();
}

float
__stdcall
SK::EOS::PercentOfAchievementsUnlocked (void)
{
  return SK_EOS_PercentOfAchievementsUnlocked ();
}

int
__stdcall
SK_EOS_NumberOfAchievementsUnlocked (void)
{
  return eos_achievements->getNumberOfAchievementsUnlocked ();
}

int
__stdcall
SK::EOS::NumberOfAchievementsUnlocked (void)
{
  return SK_EOS_NumberOfAchievementsUnlocked ();
}

void
SK_EOS_PlayUnlockSound (void)
{
  eos_achievements->playSound ();
}

void
SK_EOS_LoadUnlockSound (const wchar_t* wszUnlockSound)
{
  eos_achievements->loadSound (wszUnlockSound);
}

void
SK_EOS_LogAllAchievements (void)
{
  eos_achievements->log_all_achievements ();
}

void
SK_EOS_UnlockAchievement (uint32_t idx)
{
  eos_achievements->unlock (std::to_string (idx).c_str ());
}

void
EOS_CALL
SK_EOS_UI_OnDisplaySettingsUpdatedCallback_Proxy (const EOS_UI_OnDisplaySettingsUpdatedCallbackInfo* Data)
{
  SK_ReleaseAssert (Data->ClientData == nullptr || Data->ClientData == eos_overlay.getPtr ());

  epic_log->Log (
    L"SK_EOS_UI_OnDisplaySettingsUpdatedCallback_Proxy ({ bIsVisible=%i, bIsExclusiveInput=%i }",
      Data->bIsVisible, Data->bIsExclusiveInput
  );

  eos_overlay->OnActivate (Data);
}

int
SK_EOS_DrawOSD ()
{
  if (eos_achievements.getPtr () != nullptr)
  {
    return
      eos_achievements->drawPopups ();
  }

  return 0;
}

static bool has_unlock_callback = false;

// Unlike Steam, EGS implements interface versions in the actual DLL, and
//   this means that we need to use the oldest versions of this stuff possible
//     or the calls will fail.
static auto constexpr EOS_ACHIEVEMENTS_QUERYPLAYERACHIEVEMENTS_API_OLDEST         = 1;
static auto constexpr EOS_ACHIEVEMENTS_GETPLAYERACHIEVEMENTCOUNT_API_OLDEST       = 1;
static auto constexpr EOS_ACHIEVEMENTS_COPYPLAYERACHIEVEMENTBYINDEX_API_OLDEST    = 1;
static auto constexpr EOS_ACHIEVEMENTS_ADDNOTIFYACHIEVEMENTSUNLOCKEDV2_API_OLDEST = 2;

void
SK_EOS_Achievements_RefreshPlayerStats (void)
{
  if (config.system.log_level > 1)
    epic_log->Log (L"!! SK_EOS_Achievements_RefreshPlayerStats");

  const EOS_Achievements_QueryPlayerAchievementsOptions query_opts =
      { EOS_ACHIEVEMENTS_QUERYPLAYERACHIEVEMENTS_API_OLDEST, epic->UserId (),
                                                             epic->UserId () };

  eos_achievements->QueryPlayerAchievements ( epic->Achievements (), &query_opts,
                                              epic.getPtr        (),
  [](const EOS_Achievements_OnQueryPlayerAchievementsCompleteCallbackInfo* Data)
  {
    const gsl::not_null <SK_EOSContext *> This =
            static_cast <SK_EOSContext *> (Data->ClientData);

    auto hAchievements = This->Achievements ();
    auto UserId        = This->UserId       ();

    SK_ReleaseAssert (Data->UserId == UserId);

    if ( Data->ResultCode == EOS_EResult::EOS_Success &&
         Data->UserId     == UserId )
    {
      const EOS_Achievements_GetPlayerAchievementCountOptions get_opts =
          { EOS_ACHIEVEMENTS_GETPLAYERACHIEVEMENTCOUNT_API_OLDEST, UserId };

      const uint32_t
          num_achievements =
          eos_achievements->GetPlayerAchievementCount (hAchievements, &get_opts);
      int unlock_count     = 0;

      EOS_Achievements_CopyPlayerAchievementByIndexOptions copy_opts = {
      EOS_ACHIEVEMENTS_COPYPLAYERACHIEVEMENTBYINDEX_API_OLDEST, UserId, 0,
                                                                UserId };

      for (   copy_opts.AchievementIndex = 0;
              copy_opts.AchievementIndex < num_achievements;
            ++copy_opts.AchievementIndex )
      {
              EOS_Achievements_PlayerAchievement* achv = nullptr;
        const EOS_EResult                         result =
          eos_achievements->CopyPlayerAchievementByIndex (hAchievements, &copy_opts, &achv);

        if (result == EOS_EResult::EOS_Success)
        {
          auto managed_achievement =
            eos_achievements->getAchievement (achv->AchievementId);

          managed_achievement->unlocked_               =
            (achv->UnlockTime != EOS_ACHIEVEMENTS_ACHIEVEMENT_UNLOCKTIME_UNDEFINED);
          managed_achievement->time_                   = achv->UnlockTime;
          managed_achievement->progress_.precalculated = achv->Progress;

          for ( size_t i = 0 ; i < managed_achievement->tracked_stats_.data.size () ; ++i )
          {
            for ( size_t j = 0 ; j < (size_t)achv->StatInfoCount ; ++j )
            {
              if (! _stricmp (managed_achievement->tracked_stats_.data [i].name.c_str (), achv->StatInfo [j].Name))
              {
                managed_achievement->tracked_stats_.data [i].threshold = achv->StatInfo [j].ThresholdValue;
                managed_achievement->tracked_stats_.data [i].current   = achv->StatInfo [j].CurrentValue;
                break;
              }
            }
          }

          if (achv->StatInfoCount == 1)
          {
            if (managed_achievement->                progress_.current != (uint32_t)managed_achievement->tracked_stats_.data [0].current &&
                managed_achievement->tracked_stats_.data [0].threshold !=           managed_achievement->tracked_stats_.data [0].current)
            {
              if (managed_achievement->progress_.last_update_ms != 0 && managed_achievement->tracked_)
              {
                if (auto tracker  = SK_Widget_GetAchievementTracker ();
                         tracker != nullptr)
                         tracker->flashVisible (2.5f);
              }

              managed_achievement->progress_.last_update_ms =
                SK::ControlPanel::current_time;
            }

            managed_achievement->progress_.current       = managed_achievement->tracked_stats_.data [0].current;
            managed_achievement->progress_.max           = managed_achievement->tracked_stats_.data [0].threshold;
            managed_achievement->progress_.precalculated = 100.0 * (static_cast <double> (managed_achievement->progress_.current) /
                                                                    static_cast <double> (managed_achievement->progress_.max));
          }

          if (managed_achievement->unlocked_)
          {
            ++unlock_count;
          }

          eos_achievements->PlayerAchievement_Release (achv);
        }
      }

      eos_achievements->log_all_achievements ();

      if (! std::exchange (has_unlock_callback, true))
      {
        eos_achievements->loadSound (config.platform.achievements.sound_file.c_str ());

        constexpr EOS_Achievements_AddNotifyAchievementsUnlockedV2Options
          notify_opts = { EOS_ACHIEVEMENTS_ADDNOTIFYACHIEVEMENTSUNLOCKEDV2_API_OLDEST };

        //
        // Install a Lambda Callback
        //
        eos_achievements->AddNotifyAchievementsUnlockedV2 ( epic->Achievements (), &notify_opts,
                                                                     eos_achievements.getPtr (),
        [](const EOS_Achievements_OnAchievementsUnlockedCallbackV2Info* Data)
        {
          if (Data->UserId == epic->UserId ())
          {
            SK_ReleaseAssert ( Data->UnlockTime !=
                                 EOS_ACHIEVEMENTS_ACHIEVEMENT_UNLOCKTIME_UNDEFINED );

            auto pAchievement =
              eos_achievements->getAchievement (Data->AchievementId);

            if (pAchievement != nullptr && Data->UnlockTime != pAchievement->time_)
            {
              // This callback gets sent for achievements that are already unlocked...
              if (! pAchievement->unlocked_)
              {
                epic_log->Log ( L" Achievement: '%hs' (%hs) - Unlocked!",
                                   pAchievement->text_.unlocked.human_name.c_str (),
                                   pAchievement->text_.unlocked.desc      .c_str () );

                SK_EOS_Achievements_RefreshPlayerStats ();

                eos_achievements->unlock (Data->AchievementId);
              }
            }

            else
            {
              epic_log->Log (
                L"EOS_Achievements_OnAchievementsUnlockedCallbackV2 ({ Achievement=%hs })",
                  Data->AchievementId
              );
            }
          }
        });
      }

      eos_achievements->total_unlocked   = unlock_count;
      eos_achievements->percent_unlocked =
        static_cast <float> (
          static_cast <double> (eos_achievements->total_unlocked) /
          static_cast <double> (SK_EOS_GetNumPossibleAchievements ())
        );
    }
  });
}

bool
WINAPI
SK_IsEpicOverlayActive (void)
{
  return eos_overlay->isActive ();
}

void
SK_EOS_InvokeOverlayActivationCallback (bool active)
{
  auto orig_se =
  SK_SEH_ApplyTranslator (
    SK_FilteringStructuredExceptionTranslator (EXCEPTION_ACCESS_VIOLATION)
  );
  try
  {
    eos_overlay->invokeCallbacks (active);

    SK::EOS::overlay_state = active;
  }

  catch (const SK_SEH_IgnoredException&)
  {
    // Oh well, we literally tried...
  }
  SK_SEH_RemoveTranslator (orig_se);
}

void
__stdcall
SK::EOS::SetOverlayState (bool active)
{
  if (config.platform.silent)
    return;

  eos_overlay->invokeCallbacks (active);

  overlay_state = active;
}

bool
__stdcall
SK::EOS::GetOverlayState (bool real)
{
  return real ? SK_IsEpicOverlayActive () :
    overlay_state;
}

bool
__stdcall
SK::EOS::IsOverlayAware (void)
{
  if (! eos_overlay.getPtr ())
    return false;

  return
    eos_overlay->isOverlayAware ();
}


EOS_NotificationId
EOS_CALL
EOS_UI_AddNotifyDisplaySettingsUpdated_Detour (EOS_HUI                                        Handle,
                                         const EOS_UI_AddNotifyDisplaySettingsUpdatedOptions* Options,
                                               void*                                          ClientData,
                                         const EOS_UI_OnDisplaySettingsUpdatedCallback        NotificationFn)
{
  epic_log->Log (L"EOS_UI_AddNotifyDisplaySettingsUpdated");

  return
    eos_overlay->AddNotifyDisplaySettingsUpdated (Handle, Options, ClientData, NotificationFn);
}

void
EOS_CALL
EOS_UI_RemoveNotifyDisplaySettingsUpdated_Detour (EOS_HUI            Handle,
                                                  EOS_NotificationId Id)
{
  epic_log->Log (L"EOS_UI_RemoveNotifyDisplaySettingsUpdated");

  return
    eos_overlay->RemoveNotifyDisplaySettingsUpdated (Handle, Id);
}

void
EOS_CALL
SK_EOS_UI_OnDisplaySettingsUpdatedCallback (const EOS_UI_OnDisplaySettingsUpdatedCallbackInfo* Data)
{
  if (Data->bIsExclusiveInput)
  {
    std::lock_guard <SK_Thread_HybridSpinlock>
         lock (eos_overlay->callback_cs);

    for ( const auto &[id,callback] : eos_overlay->callbacks )
    {
      if (callback.ClientData     == Data->ClientData &&
          callback.NotificationFn != nullptr)
      {
        callback.NotificationFn (Data);
      }
    }
  }

  else if (! Data->bIsVisible)
  {
    std::lock_guard <SK_Thread_HybridSpinlock>
         lock (eos_overlay->callback_cs);

    for ( const auto &[id,callback] : eos_overlay->callbacks )
    {
      if (callback.ClientData     == Data->ClientData &&
          callback.NotificationFn != nullptr)
      {
        callback.NotificationFn (Data);
      }
    }
  }
}

EOS_Initialize_pfn       EOS_Initialize_Original       = nullptr;
EOS_Shutdown_pfn         EOS_Shutdown_Original         = nullptr;
EOS_Platform_Tick_pfn    EOS_Platform_Tick_Original    = nullptr;
EOS_Platform_Create_pfn  EOS_Platform_Create_Original  = nullptr;
EOS_Platform_Release_pfn EOS_Platform_Release_Original = nullptr;

EOS_EResult
EOS_CALL
EOS_Initialize_Detour (const EOS_InitializeOptions* Options)
{
  epic_log->Log (L"EOS_Initialize");

  return
    EOS_Initialize_Original (Options);
}

EOS_EResult
EOS_CALL
EOS_Shutdown_Detour (void)
{
  epic_log->Log (L"EOS_Shutdown");

  return
    EOS_Shutdown_Original ();
}

volatile LONGLONG __SK_EOS_Ticks = 0;

LONGLONG
SK::EOS::GetTicksRetired (void)
{
  return
    ReadAcquire64 (&__SK_EOS_Ticks);
}

void
EOS_CALL
SK_EOS_Platform_Tick (EOS_HPlatform Handle)
{
  if (ReadAcquire (&__SK_DLL_Ending))
    return;

  if (config.steam.appid == 0 && !wcscmp (config.platform.type.c_str (), SK_Platform_Unknown))
  {
    config.platform.type = SK_Platform_Epic;
  }

  // Temporarily incompatible
  SK_RunOnce (config.platform.reuse_overlay_pause = false);
  SK_RunOnce (epic_log->Log (L"EOS_Platform_Tick"));

  if (config.threads.enable_dynamic_spinlocks)
  {
    if (! std::exchange (config.epic.warned_online, true))
    {
      SK_ImGui_Warning (L"Special K does not support online mode in EOS games "
                        L"that have one.");
    }
  }

  static bool init_once = false;

  // Initialize various things on the first successful tick,
  //   this may happen multiple times until actual log-in...
  if ( epic->Platform () == nullptr ||
       epic->UserId   () == nullptr )
  {
    if (! init_once)
      epic->InitEpicOnlineServices (nullptr, Handle);
  }

  else if (init_once = true; epic->UserId () != nullptr && (! has_unlock_callback))
  {
    if ( epic->Achievements () != nullptr &&
         epic->UserId       () != nullptr )
    {
      constexpr auto EOS_ACHIEVEMENTS_QUERYDEFINITIONS_API_MINIMUM = 2;

      EOS_Achievements_QueryDefinitionsOptions
        query_opts             = { };
        query_opts.ApiVersion  = EOS_ACHIEVEMENTS_QUERYDEFINITIONS_API_MINIMUM;
        query_opts.LocalUserId = epic->UserId ();

      static bool          query_once = false;
      if (! std::exchange (query_once, true))
      {
        if (config.system.log_level > 1)
                epic_log->Log (L"?? eos_achievements->QueryDefinitions");

        eos_achievements->QueryDefinitions ( epic->Achievements (), &query_opts,
                                             epic.getPtr        (),
        [](const EOS_Achievements_OnQueryDefinitionsCompleteCallbackInfo* Data)
        {
          if (Data->ResultCode == EOS_EResult::EOS_Success)
          {
            static bool          copy_once = false;
            if (! std::exchange (copy_once, true))
            {
              EOS_Achievements_GetAchievementDefinitionCountOptions def_count_opts = {
              EOS_ACHIEVEMENTS_GETACHIEVEMENTDEFINITIONCOUNT_API_LATEST              };

              uint32_t num_achievements =
                eos_achievements->GetAchievementDefinitionCount (
                  epic->Achievements (), &def_count_opts        );

              EOS_Achievements_CopyAchievementDefinitionV2ByIndexOptions def_copy_opts = {
              EOS_ACHIEVEMENTS_COPYACHIEVEMENTDEFINITIONV2BYINDEX_API_LATEST, 0          };

              for (   def_copy_opts.AchievementIndex = 0 ;
                      def_copy_opts.AchievementIndex < num_achievements ;
                    ++def_copy_opts.AchievementIndex )
              {
                EOS_Achievements_DefinitionV2*
                   pAchievement = nullptr;

                if ( EOS_EResult::EOS_Success ==
                       eos_achievements->CopyAchievementDefinitionV2ByIndex (
                         epic->Achievements (), &def_copy_opts, &pAchievement )
                   )
                {
                  eos_achievements->addAchievement (
                                new SK_AchievementManager::Achievement (
                                             def_copy_opts.AchievementIndex,
                                                          pAchievement));
                  eos_achievements->DefinitionV2_Release (pAchievement);
                }
              }

              if (num_achievements > 0)
                SK_EOS_Achievements_RefreshPlayerStats ();
            }
          }

          else query_once = false;
        });
      }
    }
  }

  InterlockedIncrement64 (&__SK_EOS_Ticks);

  SK_EOS_SetNotifyCorner ();

  EOS_Platform_Tick_Original (Handle);
}

void
EOS_CALL
EOS_Platform_Tick_Detour (EOS_HPlatform Handle)
{
  __try
  {
    return
      SK_EOS_Platform_Tick (Handle);
  }

  __except (EXCEPTION_EXECUTE_HANDLER)
  {
    if (epic_log.isAllocated ())
        epic_log->Log (L"Structured Exception Caught during EOS_Platform_Tick Hook Execution!");
  }
}

EOS_HPlatform
EOS_CALL
EOS_Platform_Create_Detour (const EOS_Platform_Options* Options)
{
  epic_log->Log (L"EOS_Platform_Create");

  return
    EOS_Platform_Create_Original (Options);
}

void
EOS_CALL
EOS_Platform_Release_Detour (EOS_HPlatform Handle)
{
  epic_log->Log (L"EOS_Platform_Release");

  return
    EOS_Platform_Release_Original (Handle);
}

void
SK::EOS::Init (bool pre_load)
{
  //SK_LOGi0 (L"Initializing EOS...");

  if (config.platform.silent)
    return;

  const wchar_t*
    wszEOSDLLName =
      SK_RunLHIfBitness ( 64, L"EOSSDK-Win64-Shipping.dll",
                              L"EOSSDK-Win32-Shipping.dll" );

  static HMODULE     hModEOS = nullptr;
  if (std::exchange (hModEOS, SK_LoadLibraryW (wszEOSDLLName)) == nullptr && hModEOS != nullptr)
  {
    epic_log->init (L"logs/eos.log", L"wt+,ccs=UTF-8");
    epic_log->silent = config.platform.silent;

    epic_log->Log (L"EOS DLL: %p", SK_LoadLibraryW (wszEOSDLLName));

    SK_ICommandProcessor* cmd = nullptr;

    #define cmdAddAliasedVar(name,pVar)                 \
      for ( const char* alias : { "Epic."     #name,    \
                                  "Platform." #name } ) \
        cmd->AddVariable (alias, pVar);

    SK_RunOnce (
      cmd =
        SK_Render_InitializeSharedCVars ()
    );

    if (cmd != nullptr)
    {
      cmdAddAliasedVar (TakeScreenshot,
          SK_CreateVar (SK_IVariable::Boolean,
                          (bool *)&config.platform.achievements.take_screenshot));
      cmdAddAliasedVar (ShowPopup,
          SK_CreateVar (SK_IVariable::Boolean,
                          (bool *)&config.platform.achievements.popup.show));
      cmdAddAliasedVar (PopupDuration,
          SK_CreateVar (SK_IVariable::Int,
                          (int  *)&config.platform.achievements.popup.duration));
      cmdAddAliasedVar (PopupInset,
          SK_CreateVar (SK_IVariable::Float,
                          (float*)&config.platform.achievements.popup.inset));
      cmdAddAliasedVar (ShowPopupTitle,
          SK_CreateVar (SK_IVariable::Boolean,
                          (bool *)&config.platform.achievements.popup.show_title));
      cmdAddAliasedVar (PopupAnimate,
          SK_CreateVar (SK_IVariable::Boolean,
                          (bool *)&config.platform.achievements.popup.animate));
      cmdAddAliasedVar (PlaySound,
          SK_CreateVar (SK_IVariable::Boolean,
                          (bool *)&config.platform.achievements.play_sound));

      epic->popup_origin =
        SK_CreateVar ( SK_IVariable::String,
                         epic->var_strings.popup_origin,
                         epic.getPtr () );
      cmdAddAliasedVar ( PopupOrigin,
                         epic->popup_origin );

      epic->notify_corner =
        SK_CreateVar ( SK_IVariable::String,
                         epic->var_strings.notify_corner,
                         epic.getPtr () );
      cmdAddAliasedVar ( NotifyCorner,
                         epic->notify_corner );

      epic->PreInit (hModEOS);
    }
  }

  auto _SetupEOS =
  [&](void)
  {
    /* Since we probably missed the opportunity to catch EOS_Platform_Create,
         hook EOS_Platform_Tick and watch for the game's EOS_HPlatform */

    if (                                EOS_Initialize_Original == nullptr)
    SK_CreateDLLHook2 ( wszEOSDLLName, "EOS_Initialize",
                                        EOS_Initialize_Detour,
               static_cast_p2p <void> (&EOS_Initialize_Original) );

    if (                                EOS_Shutdown_Original == nullptr)
    SK_CreateDLLHook2 ( wszEOSDLLName, "EOS_Shutdown",
                                        EOS_Shutdown_Detour,
               static_cast_p2p <void> (&EOS_Shutdown_Original) );

    if (                                EOS_Platform_Tick_Original == nullptr)
    SK_CreateDLLHook2 ( wszEOSDLLName, "EOS_Platform_Tick",
                                        EOS_Platform_Tick_Detour,
               static_cast_p2p <void> (&EOS_Platform_Tick_Original) );

    if (                                EOS_Platform_Create_Original == nullptr)
    SK_CreateDLLHook2 ( wszEOSDLLName, "EOS_Platform_Create",
                                        EOS_Platform_Create_Detour,
               static_cast_p2p <void> (&EOS_Platform_Create_Original) );

    if (                                EOS_Platform_Release_Original == nullptr)
    SK_CreateDLLHook2 ( wszEOSDLLName, "EOS_Platform_Release",
                                        EOS_Platform_Release_Detour,
               static_cast_p2p <void> (&EOS_Platform_Release_Original) );


    // Some games using an outdated build of EOS SDK will not have these functions,
    //   check for DLL export before trying to hook.
    if (SK_GetProcAddress (wszEOSDLLName,"EOS_UI_AddNotifyDisplaySettingsUpdated")        != nullptr) {
      if (                          eos_overlay->AddNotifyDisplaySettingsUpdated_Original == nullptr)
      SK_CreateDLLHook2 ( wszEOSDLLName, "EOS_UI_AddNotifyDisplaySettingsUpdated",
                                          EOS_UI_AddNotifyDisplaySettingsUpdated_Detour,
           static_cast_p2p <void> (&eos_overlay->AddNotifyDisplaySettingsUpdated_Original) );
      if (                          eos_overlay->RemoveNotifyDisplaySettingsUpdated_Original == nullptr)
      SK_CreateDLLHook2 ( wszEOSDLLName, "EOS_UI_RemoveNotifyDisplaySettingsUpdated",
                                          EOS_UI_RemoveNotifyDisplaySettingsUpdated_Detour,
           static_cast_p2p <void> (&eos_overlay->RemoveNotifyDisplaySettingsUpdated_Original) );
    }

    else
    {
      SK_LOGi0 (
        L"Game is using a version of EOS SDK much older than Special K expects, if the game crashes "
        L"consider disabling EOS platform integration. ([Steam.Log] Silent=true)"
      );
    }

    eos_achievements->GetUnlockedAchievementCount     = (EOS_Achievements_GetUnlockedAchievementCount_pfn)
      SK_GetProcAddress (wszEOSDLLName,                 "EOS_Achievements_GetUnlockedAchievementCount");
    eos_achievements->GetPlayerAchievementCount       = (EOS_Achievements_GetPlayerAchievementCount_pfn)
      SK_GetProcAddress (wszEOSDLLName,                 "EOS_Achievements_GetPlayerAchievementCount");
    eos_achievements->GetAchievementDefinitionCount   = (EOS_Achievements_GetAchievementDefinitionCount_pfn)
      SK_GetProcAddress (wszEOSDLLName,                 "EOS_Achievements_GetAchievementDefinitionCount");
    eos_achievements->AddNotifyAchievementsUnlockedV2 = (EOS_Achievements_AddNotifyAchievementsUnlockedV2_pfn)
      SK_GetProcAddress (wszEOSDLLName,                 "EOS_Achievements_AddNotifyAchievementsUnlockedV2");
    eos_achievements->CopyPlayerAchievementByIndex    = (EOS_Achievements_CopyPlayerAchievementByIndex_pfn)
      SK_GetProcAddress (wszEOSDLLName,                 "EOS_Achievements_CopyPlayerAchievementByIndex");
    eos_achievements->QueryPlayerAchievements         = (EOS_Achievements_QueryPlayerAchievements_pfn)
      SK_GetProcAddress (wszEOSDLLName,                 "EOS_Achievements_QueryPlayerAchievements");
    eos_achievements->PlayerAchievement_Release       = (EOS_Achievements_PlayerAchievement_Release_pfn)
      SK_GetProcAddress (wszEOSDLLName,                 "EOS_Achievements_PlayerAchievement_Release");
    eos_achievements->CopyAchievementDefinitionV2ByIndex
                                                      = (EOS_Achievements_CopyAchievementDefinitionV2ByIndex_pfn)
      SK_GetProcAddress (wszEOSDLLName,                 "EOS_Achievements_CopyAchievementDefinitionV2ByIndex");
    eos_achievements->CopyAchievementDefinitionV2ByAchievementId
                                                      = (EOS_Achievements_CopyAchievementDefinitionV2ByAchievementId_pfn)
      SK_GetProcAddress (wszEOSDLLName,                 "EOS_Achievements_CopyAchievementDefinitionV2ByAchievementId");
    eos_achievements->QueryDefinitions                = (EOS_Achievements_QueryDefinitions_pfn)
      SK_GetProcAddress (wszEOSDLLName,                 "EOS_Achievements_QueryDefinitions");
    eos_achievements->DefinitionV2_Release            = (EOS_Achievements_DefinitionV2_Release_pfn)
      SK_GetProcAddress (wszEOSDLLName,                 "EOS_Achievements_DefinitionV2_Release");


    epic->Platform_GetAchievementsInterface           = (EOS_Platform_GetAchievementsInterface_pfn)
      SK_GetProcAddress (wszEOSDLLName,                 "EOS_Platform_GetAchievementsInterface");
    epic->Platform_GetAuthInterface                   = (EOS_Platform_GetAuthInterface_pfn)
      SK_GetProcAddress (wszEOSDLLName,                 "EOS_Platform_GetAuthInterface");
    epic->Platform_GetFriendsInterface                = (EOS_Platform_GetFriendsInterface_pfn)
      SK_GetProcAddress (wszEOSDLLName,                 "EOS_Platform_GetFriendsInterface");
    epic->Platform_GetStatsInterface                  = (EOS_Platform_GetStatsInterface_pfn)
      SK_GetProcAddress (wszEOSDLLName,                 "EOS_Platform_GetStatsInterface");
    epic->Platform_GetUIInterface                     = (EOS_Platform_GetUIInterface_pfn)
      SK_GetProcAddress (wszEOSDLLName,                 "EOS_Platform_GetUIInterface");
    epic->Platform_GetUserInfoInterface               = (EOS_Platform_GetUserInfoInterface_pfn)
      SK_GetProcAddress (wszEOSDLLName,                 "EOS_Platform_GetUserInfoInterface");
    epic->Platform_GetConnectInterface                = (EOS_Platform_GetConnectInterface_pfn)
      SK_GetProcAddress (wszEOSDLLName,                 "EOS_Platform_GetConnectInterface");

    epic->Auth_GetLoggedInAccountsCount               = (EOS_Auth_GetLoggedInAccountsCount_pfn)
      SK_GetProcAddress (wszEOSDLLName,                 "EOS_Auth_GetLoggedInAccountsCount");
    epic->Auth_GetLoggedInAccountByIndex              = (EOS_Auth_GetLoggedInAccountByIndex_pfn)
      SK_GetProcAddress (wszEOSDLLName,                 "EOS_Auth_GetLoggedInAccountByIndex");

    epic->UserInfo_QueryUserInfo                      = (EOS_UserInfo_QueryUserInfo_pfn)
      SK_GetProcAddress (wszEOSDLLName,                 "EOS_UserInfo_QueryUserInfo");
    epic->UserInfo_CopyUserInfo                       = (EOS_UserInfo_CopyUserInfo_pfn)
      SK_GetProcAddress (wszEOSDLLName,                 "EOS_UserInfo_CopyUserInfo");
    epic->UserInfo_Release                            = (EOS_UserInfo_Release_pfn)
      SK_GetProcAddress (wszEOSDLLName,                 "EOS_UserInfo_Release");

    epic->ProductUserId_FromString                    = (EOS_ProductUserId_FromString_pfn)
      SK_GetProcAddress (wszEOSDLLName,                 "EOS_ProductUserId_FromString");
    epic->Connect_GetLoggedInUserByIndex              = (EOS_Connect_GetLoggedInUserByIndex_pfn)
      SK_GetProcAddress (wszEOSDLLName,                 "EOS_Connect_GetLoggedInUserByIndex");

    epic->UI_SetDisplayPreference                     = (EOS_UI_SetDisplayPreference_pfn)
      SK_GetProcAddress (wszEOSDLLName,                 "EOS_UI_SetDisplayPreference");

#if 0
    if (epic->ProductUserId_FromString != nullptr && epic->product_user_id_ == nullptr)
    {
      char                                        szUserId [64] = { };
      if ( const char *pszId = StrStrIA (GetCommandLineA (), "-epicuserid=") ;
                       pszId != nullptr &&
          1 == sscanf (pszId, "-epicuserid=%s -", szUserId) )
      {
        EOS_ProductUserId id =
          epic->ProductUserId_FromString (szUserId);

        if (id != nullptr)
        {
          SK_LOGi0 (L"Got Epic Product User ID (%x) by parsing commandline arguments.", id);
          epic->product_user_id_ = id;
        }
      }
    }
#endif

    bool bEnable = SK_EnableApplyQueuedHooks ();
                         SK_ApplyQueuedHooks ();
    if (!bEnable) SK_DisableApplyQueuedHooks ();
  };

  if ((! pre_load) && hModEOS != nullptr)
  {
    SK_RunOnce (
      if (SK::EOS::GetTicksRetired () == 0)
        _SetupEOS ();
    );
  }

  // Preloading not supported, SK has no EOS Credentials
  //
  //if (epic->InitEpicOnlineServices (hModEOS))
  //{
  //  EOS_InitializeOptions
  //}
}

void
SK::EOS::Shutdown (void)
{
  epic_log->Log (L"STUB Shutdown");
  //epic->Shutdown ();
}

void
SK_EOSContext::PreInit (HMODULE hEOSDLL)
{
  sdk_dll_ = hEOSDLL;
}

bool
SK_EOSContext::InitEpicOnlineServices ( HMODULE       hEOSDLL,
                                        EOS_HPlatform platform )
{
  if (ReadAcquire (&__SK_DLL_Ending))
    return false;

  // If we were a registered EOS product, this is where we would init...
  if (platform == nullptr)
  {
    auto Initialize =
    (EOS_Initialize_pfn)SK_GetProcAddress (hEOSDLL,
    "EOS_Initialize");

    if (Initialize == nullptr)
      return false;

    // We need the game to initialize this, we don't have an actual product id :)
    if (Initialize (nullptr) != EOS_EResult::EOS_AlreadyConfigured)
      return false;

    auto Platform_Create =
    (EOS_Platform_Create_pfn)SK_GetProcAddress (hEOSDLL,
    "EOS_Platform_Create");

    if (Platform_Create == nullptr)
      return false;
  }

  if (Platform_GetAuthInterface != nullptr)  auth_ =
      Platform_GetAuthInterface (platform);

  if (auth_ != nullptr)
  {
    int32_t logins =
      ( Auth_GetLoggedInAccountsCount != nullptr ) ?
        Auth_GetLoggedInAccountsCount (auth_)      : 0;

    //SK_ReleaseAssert (logins <= 1);

    SK::EOS::player.account =
      ( logins > 0 ) ? Auth_GetLoggedInAccountByIndex (auth_, 0)
                     : nullptr;

    if (SK::EOS::player.account != nullptr)
    {
      // But we're not, so we will yoink the game's HPlatform instance
      platform_ = platform;

      if (Platform_GetUIInterface != nullptr) ui_ =
          Platform_GetUIInterface (platform_);

      if (ui_ != nullptr)
      {
        // There are games (i.e. Path of Exile 2) using a version of the EOS SDK so old
        //   that this API does not exist, so check that we even have this function
        //     before we crash!
        if (eos_overlay->AddNotifyDisplaySettingsUpdated_Original != nullptr)
        {
          EOS_UI_AddNotifyDisplaySettingsUpdatedOptions opts =
            { EOS_UI_ADDNOTIFYDISPLAYSETTINGSUPDATED_API_LATEST };

          SK_RunOnce (
            eos_overlay->AddNotifyDisplaySettingsUpdated_Original (
              ui_, &opts, eos_overlay.getPtr (),
              SK_EOS_UI_OnDisplaySettingsUpdatedCallback_Proxy)
          );
        }

        SK_EOS_SetNotifyCorner ();
      }

      if (Platform_GetAchievementsInterface != nullptr) achievements_ =
          Platform_GetAchievementsInterface (platform_);

      if (Platform_GetFriendsInterface  != nullptr)  friends_   =
          Platform_GetFriendsInterface  (platform_);
      if (Platform_GetStatsInterface    != nullptr)  stats_     =
          Platform_GetStatsInterface    (platform_);
      if (Platform_GetUserInfoInterface != nullptr)  user_info_ =
          Platform_GetUserInfoInterface (platform_);
      if (Platform_GetConnectInterface  != nullptr)  connect_   =
          Platform_GetConnectInterface  (platform_);

      if (user_info_ != nullptr && UserInfo_QueryUserInfo != nullptr)
      {
        EOS_UserInfo_QueryUserInfoOptions
          opts = { EOS_USERINFO_QUERYUSERINFO_API_LATEST,
                   SK::EOS::player.account,
                   SK::EOS::player.account };

        UserInfo_QueryUserInfo (user_info_, &opts, this, [](const EOS_UserInfo_QueryUserInfoCallbackInfo* Data)
        {
          if (Data->ResultCode == EOS_EResult::EOS_Success)
          {
            SK_ReleaseAssert (Data->ClientData == nullptr || Data->ClientData == epic.getPtr ());

            if ( epic->UserInfo_Release      != nullptr &&
                 epic->UserInfo_CopyUserInfo != nullptr )
            {
              EOS_UserInfo_CopyUserInfoOptions opts =
              {
                EOS_USERINFO_COPYUSERINFO_API_LATEST,
                SK::EOS::player.account,
                SK::EOS::player.account
              };

              EOS_UserInfo*                                             pUserInfo = nullptr;
              EOS_EResult result =
                epic->UserInfo_CopyUserInfo (epic->UserInfo (), &opts, &pUserInfo);

              if (result == EOS_EResult::EOS_Success)
              {
                epic->user.display_name = pUserInfo->DisplayName != nullptr ? pUserInfo->DisplayName : "";
                epic->user.nickname     = pUserInfo->Nickname    != nullptr ? pUserInfo->Nickname    : "";

                SK::EOS::player.user   = epic->Connect_GetLoggedInUserByIndex (epic->Connect (), 0);
                epic->product_user_id_ = epic->Connect_GetLoggedInUserByIndex (epic->Connect (), 0);

#ifdef DEBUG
                SK_LOGi0 (
                  L"Got Epic Product User ID (%x) via late call to EOS_UserInfo_CopyUserInfo (...).",
                       epic->product_user_id_ );
#endif

                epic->UserInfo_Release (pUserInfo);
              }
            }
          }
        });
      }
    }
  }

  return true;
}


SK_LazyGlobal <SK_EOSContext> epic;
bool SK::EOS::overlay_state = false;

bool SK_EOSContext::OnVarChange (SK_IVariable *, void *)
{
  return true;
}



std::string_view
SK::EOS::PlayerName (void)
{
  std::string_view view =
    epic->GetDisplayName ();

  if (view.empty ())
    return "";

  return view;
}

std::string_view
SK::EOS::PlayerNickname (void)
{
  std::string_view view =
    epic->GetNickName ();

  if (view.empty ())
  {
    view =
      epic->GetDisplayName ();

    if (view.empty ())
      return "";
  }

  return view;
}

EOS_EpicAccountId
SK::EOS::UserID (void)
{
  return
    SK::EOS::player.account;
}

#include <filesystem>

std::string&
SK::EOS::AppName (void)
{
  static std::string name = "";

  if (                                                          name.empty ()/*&&
      app_cache_mgr->getAppNameFromPath (SK_GetFullyQualifiedApp ()).empty () */)
  {
    std::filesystem::path path =
      std::filesystem::path (SK_GetFullyQualifiedApp ()).lexically_normal ();

    char szDisplayName [65] = { };
    char szEpicApp     [65] = { };

    try
    {
      while (! std::filesystem::equivalent ( path.parent_path    (),
                                             path.root_directory () ) )
      {
        if (! name.empty ())
          break;

        if (std::filesystem::is_directory (path / L".egstore"))
        {
          for ( const auto& file : std::filesystem::directory_iterator (path / L".egstore") )
          {
            if (! file.is_regular_file ())
              continue;

            if (file.path ().extension ().compare (L".mancpn") == 0)
            {
              CRegKey hkManifestRoot;
                      hkManifestRoot.Open (HKEY_CURRENT_USER, LR"(Software\Epic Games\EOS)");

              wchar_t wszManifestPath [MAX_PATH + 2] = { };
              ULONG   ulManifestLen =  MAX_PATH;
              hkManifestRoot.QueryStringValue (L"ModSdkMetadataDir", wszManifestPath, &ulManifestLen);

              PathAppendW (wszManifestPath, file.path ().stem ().c_str ());
              StrCatW     (wszManifestPath, L".item");

              if (! std::filesystem::exists (wszManifestPath))
                continue;

              if (std::fstream mancpn (wszManifestPath, std::fstream::in);
                               mancpn.is_open ())
              {
                bool executable = false;
                bool skip       = false;

                char                     szLine [512] = { };
                while (! mancpn.getline (szLine, 511).eof ())
                {
                  if (StrStrIA (szLine, "\"DisplayName\"") != nullptr)
                  {
                    const char      *substr =     StrStrIA (szLine, ":");
                    strncpy_s (szDisplayName, 64, StrStrIA (substr, "\"") + 1, _TRUNCATE);
                     *strrchr (szDisplayName, '"') = '\0';
                    continue;
                  }

                  else if (StrStrIA (szLine, "\"AppName\"") != nullptr)
                  {
                    const char      *substr = StrStrIA (szLine, ":");
                    strncpy_s (szEpicApp, 64, StrStrIA (substr, "\"") + 1, _TRUNCATE);
                     *strrchr (szEpicApp, '"') = '\0';

                    if (! PathFileExistsW (
                            SK_FormatStringW ( LR"(%ws\Profiles\AppCache\#EpicApps\%hs\manifest.json)",
                                               SK_GetInstallPath (), szEpicApp ).c_str ()
                       )                  )
                    {
                      executable     = false;
                      *szEpicApp     = '\0';
                      *szDisplayName = '\0';
                      skip           = true;
                      continue;
                    }
                  }

                  else if (StrStrIA (szLine, "\"LaunchExecutable\"") != nullptr)
                  {
                    executable = true;
                    continue;
                  }

                  if (*szDisplayName != '\0' && *szEpicApp != '\0' && executable)
                  {
                    app_cache_mgr->addAppToCache (
                        SK_GetFullyQualifiedApp (),
                                  SK_GetHostApp (),
                              SK_UTF8ToWideChar (szDisplayName).c_str (),
                                                 szEpicApp
                    );

                    name = szDisplayName;
                    break;
                  }
                }

                if (skip)
                  continue;

                path = L"/";
                break;
              }
            }

            if (! name.empty ())
              break;
          }
        }

        path =
          path.parent_path ().lexically_normal ();
      }

      if (config.platform.equivalent_steam_app == -1)
      {
        std::wstring url =
          SK_FormatStringW (
            LR"(https://www.pcgamingwiki.com/w/index.php?search=%ws)", SK_Network_MakeEscapeSequencedURL (SK_Platform_RemoveTrademarkSymbols (SK_UTF8ToWideChar (szDisplayName))).c_str ()
          );

        SK_Network_EnqueueDownload (
          sk_download_request_s (L"pcgw_entry.html", url.data (),
            []( const std::vector <uint8_t>&& data,
                const std::wstring_view       file )
            {
              if (data.empty ())
                return true;

              std::ignore = file;

              auto steamdb_appid =
                StrStrIA ((const char *)data.data (), "https://steamdb.info/app/");

              if (steamdb_appid != nullptr)
              {
                if (1 != sscanf (steamdb_appid, "https://steamdb.info/app/%d/", &config.platform.equivalent_steam_app))
                {
                  config.platform.equivalent_steam_app = 0;
                }
              }

              return true;
            } ),
          true
        );
      }

      app_cache_mgr->saveAppCache       ();
      app_cache_mgr->loadAppCacheForExe (SK_GetFullyQualifiedApp ());

      // Trigger profile migration if necessary
      SK_RunOnce (app_cache_mgr->getConfigPathForEpicApp (szEpicApp));
    }

    catch (const std::exception& e)
    {
      epic_log->Log (L"App Name Parse Failure: %hs", e.what ());
    }
  }

  return
    name;
}

void
SK_EOS_SetNotifyCorner (void)
{
  // 4 == Don't Care
  if (config.platform.notify_corner != 4)
  {
    if ( epic->UI ()                   != nullptr &&
         epic->UI_SetDisplayPreference != nullptr )
    {
      EOS_UI_SetDisplayPreferenceOptions opts = {
      EOS_UI_SETDISPLAYPREFERENCE_API_LATEST,
        static_cast <EOS_UI_ENotificationLocation> (config.platform.notify_corner)
      };

      epic->UI_SetDisplayPreference ( epic->UI (), &opts );
    }
  }
}

SK::EOS::player_s
SK::EOS::player = { };

EOS_Platform_GetAchievementsInterface_pfn SK_EOSContext::Platform_GetAchievementsInterface = nullptr;
EOS_Platform_GetAuthInterface_pfn         SK_EOSContext::Platform_GetAuthInterface         = nullptr;
EOS_Platform_GetFriendsInterface_pfn      SK_EOSContext::Platform_GetFriendsInterface      = nullptr;
EOS_Platform_GetStatsInterface_pfn        SK_EOSContext::Platform_GetStatsInterface        = nullptr;
EOS_Platform_GetUIInterface_pfn           SK_EOSContext::Platform_GetUIInterface           = nullptr;
EOS_Platform_GetUserInfoInterface_pfn     SK_EOSContext::Platform_GetUserInfoInterface     = nullptr;
EOS_Platform_GetConnectInterface_pfn      SK_EOSContext::Platform_GetConnectInterface      = nullptr;

EOS_Auth_GetLoggedInAccountsCount_pfn     SK_EOSContext::Auth_GetLoggedInAccountsCount     = nullptr;
EOS_Auth_GetLoggedInAccountByIndex_pfn    SK_EOSContext::Auth_GetLoggedInAccountByIndex    = nullptr;

EOS_UserInfo_QueryUserInfo_pfn            SK_EOSContext::UserInfo_QueryUserInfo            = nullptr;
EOS_UserInfo_CopyUserInfo_pfn             SK_EOSContext::UserInfo_CopyUserInfo             = nullptr;
EOS_UserInfo_Release_pfn                  SK_EOSContext::UserInfo_Release                  = nullptr;

EOS_ProductUserId_FromString_pfn          SK_EOSContext::ProductUserId_FromString          = nullptr;
EOS_Connect_GetLoggedInUserByIndex_pfn    SK_EOSContext::Connect_GetLoggedInUserByIndex    = nullptr;

// Move to overlay manager
EOS_UI_SetDisplayPreference_pfn           SK_EOSContext::UI_SetDisplayPreference           = nullptr;