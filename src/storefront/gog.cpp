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
#include <storefront/gog.h>
#include <storefront/achievements.h>

#include <json/json.hpp>

#include <galaxy/IListenerRegistrar.h>
#include <galaxy/IUtils.h>
#include <galaxy/IUser.h>
#include <galaxy/IStats.h>
#include <galaxy/IFriends.h>
#include <galaxy/1_152_10/IStats.h>
#include <galaxy/1_152_1/IStats.h>
#include <galaxy/1_151_0/IUser.h>
#include <galaxy/1_152_1/IUser.h>
#include <galaxy/1_152_10/IUser.h>
#include <galaxy/1_121_2/InitOptions.h>

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"GOG Galaxy"

#define GALAXY_VIRTUAL_HOOK(_Base,_Index,_Name,_Override,_Original,_Type) {   \
  void** _vftable = *(void***)*(_Base);                                       \
                                                                              \
  if ((_Original) == nullptr) {                                               \
    SK_CreateVFTableHook2 ( L##_Name,                                         \
                              _vftable,                                       \
                                (_Index),                                     \
                                  (_Override),                                \
                                    (LPVOID *)&(_Original));                  \
  }                                                                           \
}

class SK_Galaxy_OverlayManager : public galaxy::api::IOverlayStateChangeListener
{
public:
  bool isActive (void) const {
    return active_;
  }

  void OnOverlayStateChanged (bool overlayIsActive)
  {
    std::lock_guard <SK_Thread_HybridSpinlock>
         lock (callback_cs);

    // If the game has an activation callback installed, then
    //   it's also going to see this event... make a note of that when
    //     tracking its believed overlay state.
    if (SK::Galaxy::IsOverlayAware ())
        SK::Galaxy::overlay_state = overlayIsActive;


    // If we want to use this as our own, then don't let the Epic overlay
    //   unpause the game on deactivation unless the control panel is closed.
    if (config.platform.reuse_overlay_pause && SK_Platform_IsOverlayAware ())
    {
      // Deactivating, but we might want to hide this event from the game...
      if (overlayIsActive == false)
      {
        // Undo the event the game is about to receive.
        if (SK_ImGui_Visible) SK_Platform_SetOverlayState (true);
      }
    }

    const bool wasActive =
                  active_;

    active_ = overlayIsActive;

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

    for ( const auto &[listener,callback_data] : callbacks )
    {
      if (listener == nullptr)
        continue;

      auto callback =
        (galaxy::api::IOverlayStateChangeListener *)listener;

      callback->OnOverlayStateChanged (active);
    }
  }

  void
  Register (galaxy::api::IGalaxyListener* listener)
  {
    SK_LOG_FIRST_CALL

    std::lock_guard <SK_Thread_HybridSpinlock>
         lock (callback_cs);

    if (listener != nullptr)
    {
      callbacks [listener] = { };
    }
  }

  void
  Unregister (galaxy::api::IGalaxyListener* listener)
  {
    std::lock_guard <SK_Thread_HybridSpinlock>
         lock (callback_cs);

    if ( const auto cb  = callbacks.find (listener);
                    cb != callbacks.end  (  ) )
    {
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
    // TODO
  };

public:
  std::unordered_map <galaxy::api::IGalaxyListener*, callback_s> callbacks;
  SK_Thread_HybridSpinlock                                       callback_cs;
};

class SK_Galaxy_AchievementManager :
      public SK_AchievementManager, public galaxy::api::IUserStatsAndAchievementsRetrieveListener,
                                    public galaxy::api::IStatsAndAchievementsStoreListener,
                                    public galaxy::api::IAchievementChangeListener,
                                    public galaxy::api::IUserTimePlayedRetrieveListener
{
public:
  SK_Galaxy_AchievementManager(void)
  {
    achievements_path =
      std::filesystem::path (
        SK_GetConfigPath () )
      / LR"(SK_Res/Achievements)";

    global_stats_filename =
      achievements_path / LR"(GlobalStatsForGame.json)";
  }

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

    for (uint32 i = 0; i < SK_Galaxy_GetNumPossibleAchievements (); i++)
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

    if (index >= SK_Galaxy_GetNumPossibleAchievements ())
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
    for (uint32 i = 0; i < SK_Galaxy_GetNumPossibleAchievements (); i++)
    {
      const Achievement* achievement =
                         achievements.list [i];

      if (achievement == nullptr || achievement->name_.empty ())
        continue;

      gog_log->LogEx  (false, L"\n [%c] Achievement %03lu......: '%hs'\n",
                       achievement->unlocked_ ? L'X' : L' ',
                    i, achievement->name_.data ());
      gog_log->LogEx  (false,
                       L"  + Human Readable Name...: %hs\n",
                       achievement->      unlocked_                    ?
                       achievement->text_.unlocked.human_name.c_str () :
                       achievement->text_.  locked.human_name.c_str ());
      if (! (achievement->unlocked_ && achievement->text_.locked.desc.empty ()))
      {
        gog_log->LogEx (false,
                       L"  *- Detailed Description.: %hs\n",
                       achievement->text_.locked.desc.c_str ());
      }
      else if ((achievement->unlocked_ && !achievement->text_.unlocked.desc.empty ()))
      {
        gog_log->LogEx (false,
                        L"  *- Detailed Description.: %hs\n",
                        achievement->text_.unlocked.desc.c_str ());
      }

      if (achievement->global_percent_ > 0.0f)
      {
        gog_log->LogEx (false,
                        L"  #-- Rarity (Global).....: %6.2f%%\n",
                        achievement->global_percent_);
      }
      if (achievement->friends_.possible > 0)
      {
        gog_log->LogEx (false,
                        L"  #-- Rarity (Friend).....: %6.2f%%\n",
          100.0 * (static_cast <double> (achievement->friends_.unlocked) /
                   static_cast <double> (achievement->friends_.possible)) );
      }

      if (achievement->progress_.current != achievement->progress_.max &&
          achievement->progress_.max     != 1) // Don't log achievements that are simple yes/no
      {
        gog_log->LogEx (false,
                        L"  @--- Progress To Unlock.: % 6.2f%%　　[ %d / %d ]\n",
         100.0 * (static_cast <double> (achievement->progress_.current) /
                  static_cast <double> (achievement->progress_.max)),
                                        achievement->progress_.current,
                                        achievement->progress_.max);
      }

      if (achievement->unlocked_)
      {
        gog_log->LogEx (false,
                        L"  @--- Player Unlocked At.: %s",
                        _wctime64 (&achievement->time_));
      }
    }

    epic_log->LogEx (false, L"\n");
  }

  void clear_achievement (const char* szName)
  {
    SK_Galaxy_Stats_ClearAchievement (gog->Stats (), szName);
  }

  int possible = 0;

  virtual void OnAchievementUnlocked (const char* name) final
  {
    auto pAchievement =
      getAchievement (name);

    if (pAchievement != nullptr)
    {
      // This callback gets sent for achievements that are already unlocked...
      if (! pAchievement->unlocked_)
      {
        gog_log->Log ( L" Achievement: '%hs' (%hs) - Unlocked!",
                         pAchievement->text_.unlocked.human_name.c_str (),
                         pAchievement->text_.unlocked.desc      .c_str () );

        total_unlocked++;
        percent_unlocked =
          static_cast <float> (
            static_cast <double> (total_unlocked) /
            static_cast <double> (possible)
          );

        SK_Galaxy_Stats_RequestUserStatsAndAchievements (gog->Stats ());

        unlock (name);
      }
    }
  }

  virtual void OnUserStatsAndAchievementsRetrieveSuccess (galaxy::api::GalaxyID userID) final
  {
    SK_RunOnce (
      gog_log->Log (L"OnUserStatsAndAchievementsRetrieveSuccess (...)")
    );

    if (userID != gog->GetGalaxyID ())
    {
      gog_log->Log (
        L"userID=%d does not match Stats and Achievements userID=%d",
        userID.ToUint64 (), gog->GetGalaxyID ().ToUint64 ()
      );
    }

    if (! global_stats_loaded)
    {
      auto stats = gog->Stats ();

      static FILE*   fGlobalStats  = nullptr;
      fGlobalStats = fGlobalStats != nullptr ? fGlobalStats :
            _wfopen (global_stats_filename.c_str (), L"rb+");

      if (fGlobalStats != nullptr)
      {
        static bool loaded = false;

        try
        {
          static std::vector <BYTE> data;
  
                       fseek (fGlobalStats, 0, SEEK_END);
          data.resize (ftell (fGlobalStats));
                      rewind (fGlobalStats);

          if (! data.empty ())
          {
            static nlohmann::json jsonStats;

            if (! loaded)
            {
              if (0 != fread (data.data (), data.size (), 1, fGlobalStats))
              {
                jsonStats =
                  std::move (
                    nlohmann::json::parse ( data.cbegin (),
                                            data.cend   (), nullptr, true )
                  );

                loaded              = true;
                global_stats_loaded = true;
              }
            }

            if ( jsonStats.contains ("achievementpercentages") &&
                 jsonStats          ["achievementpercentages"].contains ("achievements") )
            {
              const auto& achievements_ =
                jsonStats ["achievementpercentages"]["achievements"];

              int idx = 0;

              for ( const auto& achievement : achievements_ )
              {
                auto galaxy_achievement =
                  new SK_AchievementManager::Achievement (
                    idx++, achievement ["name"].get <std::string_view> ().data (), (galaxy::api::IStats *)stats
                  );

                galaxy_achievement->global_percent_ = static_cast <float> (
                  atof (achievement ["percent"].get <std::string_view> ().data ())
                );

                possible++;
                addAchievement (galaxy_achievement);
              }
            }
          }

          else
          {
            throw (std::exception ());
          }
        }

        catch (const std::exception& e)
        {
          std::ignore = e;

          loaded = false;

          fclose (fGlobalStats);
                  fGlobalStats = nullptr;

          DeleteFileW (global_stats_filename.c_str ());

          gog_log->Log (
            L"Global Achievement Stats JSON was corrupted and has been deleted."
          );

          SK_Platform_DownloadGlobalAchievementStats ();

          need_stats_refresh  =  true;
          global_stats_loaded = false;
        }
      }
    };

    int    unlock_count = 0;
    size_t num_achvs    = achievements.list.size ();

    for ( size_t i = 0 ; i < num_achvs ; ++i )
    {
      auto galaxy_achievement =
        achievements.list [i];

      uint32_t time;
      SK_Galaxy_Stats_GetAchievement ( gog->Stats (),
        galaxy_achievement->name_.c_str (),
        galaxy_achievement->unlocked_,
        time
      );

      if (galaxy_achievement->unlocked_) {
          galaxy_achievement->time_ = time;
          unlock_count++;
      }
    }

    log_all_achievements ();

    static bool          has_unlock_callback = false;
    if (! std::exchange (has_unlock_callback, true))
    {
      loadSound (config.platform.achievements.sound_file.c_str ());

      gog->Registrar ()->Register (
         galaxy::api::ACHIEVEMENT_CHANGE,
        (galaxy::api::IAchievementChangeListener *)this
      );
    }

    total_unlocked   = unlock_count;
    percent_unlocked =
      static_cast <float> (
        static_cast <double> (total_unlocked) /
        static_cast <double> (SK_Galaxy_GetNumPossibleAchievements ())
      );
  }

  virtual void OnUserStatsAndAchievementsRetrieveFailure (
    galaxy::api::GalaxyID                                                 userID,
    galaxy::api::IUserStatsAndAchievementsRetrieveListener::FailureReason failureReason ) final
  {
    gog_log->Log (
      L"RequestUserStatsAndAchievements (...) Failed! userID=%d | failureReason=%x",
        userID.ToUint64 (), failureReason
    );
  }

  virtual void OnUserStatsAndAchievementsStoreSuccess (void) final
  {
    // Refresh the stats on next call to ProcessData (...)
    need_stats_refresh = true;
    refresh_count      =    0;
  }

  virtual void OnUserStatsAndAchievementsStoreFailure (
    galaxy::api::IStatsAndAchievementsStoreListener::FailureReason failureReason ) final
  {
    gog_log->Log (
      L"StoreStatsAndAchievements (...) Failed! failureReason=%x", failureReason
    );
  }

  virtual void OnUserTimePlayedRetrieveSuccess (galaxy::api::GalaxyID userID) final
  {
    std::ignore = userID;

    auto stats = gog->Stats ();

    uint32_t playing_time = SK_Galaxy_Stats_GetUserTimePlayed (stats);
    if (playing_time != 0)
    {
      gog_log->Log (
        L"User has played this game for %5.2f Hours", (float)playing_time / 60.0f
      );
    }
  }

  virtual void OnUserTimePlayedRetrieveFailure (
    galaxy::api::GalaxyID                                       userID,
    galaxy::api::IUserTimePlayedRetrieveListener::FailureReason failureReason ) final
  {
    gog_log->Log (L"RequestUserTimePlayed Failed=%x", failureReason);

    std::ignore = userID;
    std::ignore = failureReason;
  }

  int  refresh_count       =     0;
  bool need_stats_refresh  =  true;
  bool global_stats_loaded = false;

  std::filesystem::path achievements_path;
  std::filesystem::path global_stats_filename;
};

SK_LazyGlobal <SK_Galaxy_OverlayManager>     galaxy_overlay;
SK_LazyGlobal <SK_Galaxy_AchievementManager> galaxy_achievements;

bool
WINAPI
SK_IsGalaxyOverlayActive (void)
{
  return
    galaxy_overlay->isActive ();
}

void
__stdcall
SK::Galaxy::SetOverlayState (bool active)
{
  if (config.platform.silent)
    return;

  galaxy_overlay->invokeCallbacks (active);

  overlay_state = active;
}

bool
__stdcall
SK::Galaxy::GetOverlayState (bool real)
{
  return real ? SK_IsGalaxyOverlayActive () :
    overlay_state;
}

bool
__stdcall
SK::Galaxy::IsOverlayAware (void)
{
  if (! galaxy_overlay.getPtr ())
    return false;

  return
    galaxy_overlay->isOverlayAware ();
}

SK_AchievementManager*
SK_Galaxy_GetAchievementManager (void)
{
  return
    galaxy_achievements.getPtr ();
}

// The Galaxy client is not capable of reporting this... wtf?!
size_t
SK_Galaxy_GetNumPossibleAchievements (void)
{
  return
    galaxy_achievements->possible;
}

float
__stdcall
SK_Galaxy_PercentOfAchievementsUnlocked (void)
{
  return galaxy_achievements->getPercentOfAchievementsUnlocked ();
}

float
__stdcall
SK::Galaxy::PercentOfAchievementsUnlocked (void)
{
  return SK_Galaxy_PercentOfAchievementsUnlocked ();
}

int
__stdcall
SK_Galaxy_NumberOfAchievementsUnlocked (void)
{
  return galaxy_achievements->getNumberOfAchievementsUnlocked ();
}

int
__stdcall
SK::Galaxy::NumberOfAchievementsUnlocked (void)
{
  return SK_Galaxy_NumberOfAchievementsUnlocked ();
}

void
SK_Galaxy_PlayUnlockSound (void)
{
  galaxy_achievements->playSound ();
}

void
SK_Galaxy_LoadUnlockSound (const wchar_t* wszUnlockSound)
{
  galaxy_achievements->loadSound (wszUnlockSound);
}

void
SK_Galaxy_LogAllAchievements (void)
{
  galaxy_achievements->log_all_achievements ();
}

void
SK_Galaxy_UnlockAchievement (uint32_t idx)
{
  galaxy_achievements->unlock (std::to_string (idx).c_str ());
}

int
SK_Galaxy_DrawOSD ()
{
  if (galaxy_achievements.getPtr () != nullptr)
  {
    return
      galaxy_achievements->drawPopups ();
  }

  return 0;
}

bool
SK_Galaxy_User_SignedIn (galaxy::api::IUser* This)
{
  //
  // Galaxy API is full of all kinds of breaking API changes, so
  //   crashing is inevitable on older builds of Galaxy SDK until
  //     SK has reviewed all of the official releases...
  // 
  //  * Try to avoid a crash for now.
  //
  __try {
    switch (gog->version)
    {
      default:
      case SK_GalaxyContext::Version_1_151_0:
        return ((galaxy::api::IUser_1_151_0*)This)->SignedIn ();

      case SK_GalaxyContext::Version_1_152_1:
        return ((galaxy::api::IUser_1_152_1*)This)->SignedIn ();

      case SK_GalaxyContext::Version_1_152_10:
        return ((galaxy::api::IUser_1_152_10*)This)->SignedIn ();
    }
  } __except (GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ?
                                     EXCEPTION_EXECUTE_HANDLER  :
                                     EXCEPTION_CONTINUE_SEARCH ){}

  return false;
}

bool
SK_Galaxy_User_IsLoggedOn (galaxy::api::IUser* This)
{
  //
  // Galaxy API is full of all kinds of breaking API changes, so
  //   crashing is inevitable on older builds of Galaxy SDK until
  //     SK has reviewed all of the official releases...
  // 
  //  * Try to avoid a crash for now.
  //
  __try {
    switch (gog->version)
    {
      default:
      case SK_GalaxyContext::Version_1_151_0:
        return ((galaxy::api::IUser_1_151_0*)This)->IsLoggedOn ();

      case SK_GalaxyContext::Version_1_152_1:
        return ((galaxy::api::IUser_1_152_1*)This)->IsLoggedOn ();

      case SK_GalaxyContext::Version_1_152_10:
        return ((galaxy::api::IUser_1_152_10*)This)->IsLoggedOn ();
    }
  } __except (GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ?
                                     EXCEPTION_EXECUTE_HANDLER  :
                                     EXCEPTION_CONTINUE_SEARCH ){}

  return false;
}

void
SK_Galaxy_User_SignInGalaxy ( galaxy::api::IUser* This,
                                         bool     requireOnline = false,
                                         uint32_t timeout       =    15,
                      galaxy::api::IAuthListener* listener      = nullptr )
{
  //
  // Galaxy API is full of all kinds of breaking API changes, so
  //   crashing is inevitable on older builds of Galaxy SDK until
  //     SK has reviewed all of the official releases...
  // 
  //  * Try to avoid a crash for now.
  //
  __try {
    switch (gog->version)
    {
      default:
      case SK_GalaxyContext::Version_1_151_0:
        return ((galaxy::api::IUser_1_151_0*)This)->SignInGalaxy (requireOnline, listener);
        break;
      case SK_GalaxyContext::Version_1_152_1:
        return ((galaxy::api::IUser_1_152_1*)This)->SignInGalaxy (requireOnline, listener);
        break;
      case SK_GalaxyContext::Version_1_152_10:
        return ((galaxy::api::IUser_1_152_10*)This)->SignInGalaxy (requireOnline, timeout, listener);
        break;
    }
  } __except (GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ?
                                     EXCEPTION_EXECUTE_HANDLER  :
                                     EXCEPTION_CONTINUE_SEARCH ){}
}

namespace galaxy
{
  namespace api
  {
    using ProcessData_pfn         = void (__cdecl *)(void);
    using Init_pfn                = void (__cdecl *)(struct galaxy::api::InitOptions const&);
    using Shutdown_pfn            = void (__cdecl *)(void);
    using CreateInstance_pfn      = galaxy::api::IGalaxy* (__cdecl *)(void);
    using ProcessData_IGalaxy_pfn = void (__cdecl *)(IGalaxy* This);
    using Shutdown_IGalaxy_pfn    = void (__cdecl *)(IGalaxy* This);

    static Init_pfn                Init_Original                = nullptr;
    static Shutdown_pfn            Shutdown_Original            = nullptr;
    static ProcessData_pfn         ProcessData_Original         = nullptr;
    static CreateInstance_pfn      CreateInstance_Original      = nullptr;
    static ProcessData_IGalaxy_pfn ProcessData_IGalaxy_Original = nullptr;
    static Shutdown_IGalaxy_pfn    Shutdown_IGalaxy_Original    = nullptr;

    static volatile LONGLONG ticks = 0;

    void
    ProcessDataHook_Impl (IGalaxy* This = nullptr)
    {
      SK_LOG_FIRST_CALL

      if (This == nullptr && gog->Stats () == nullptr)
      {
        const auto Stats     = (IStats*             (__cdecl *)(void))
          SK_GetProcAddress (gog->GetGalaxyDLL (), "?Stats@api@galaxy@@YAPEAVIStats@12@XZ");
        const auto Utils     = (IUtils*             (__cdecl *)(void))
          SK_GetProcAddress (gog->GetGalaxyDLL (), "?Utils@api@galaxy@@YAPEAVIUtils@12@XZ");
        const auto User      = (IUser*              (__cdecl *)(void))
          SK_GetProcAddress (gog->GetGalaxyDLL (), "?User@api@galaxy@@YAPEAVIUser@12@XZ");
        const auto Friends   = (IFriends*           (__cdecl *)(void))
          SK_GetProcAddress (gog->GetGalaxyDLL (), "?Friends@api@galaxy@@YAPEAVIFriends@12@XZ");
        const auto Registrar = (IListenerRegistrar* (__cdecl *)(void))
          SK_GetProcAddress (gog->GetGalaxyDLL (), "?ListenerRegistrar@api@galaxy@@YAPEAVIListenerRegistrar@12@XZ");

        IStats*             pStats     = Stats     != nullptr ? Stats     () : nullptr;
        IUtils*             pUtils     = Utils     != nullptr ? Utils     () : nullptr;
        IUser*              pUser      = User      != nullptr ? User      () : nullptr;
        IFriends*           pFriends   = Friends   != nullptr ? Friends   () : nullptr;
        IListenerRegistrar* pRegistrar = Registrar != nullptr ? Registrar () : nullptr;

        gog->Init (pStats, pUtils, pUser, pFriends, pRegistrar);
      }

      else if (This != nullptr && gog->Stats () == nullptr)
      {
        IStats*             pStats     = This->GetStats             ();
        IUtils*             pUtils     = This->GetUtils             ();
        IUser*              pUser      = This->GetUser              ();
        IFriends*           pFriends   = This->GetFriends           ();
        IListenerRegistrar* pRegistrar = This->GetListenerRegistrar ();

        gog->Init (pStats, pUtils, pUser, pFriends, pRegistrar);
      }

      auto stats = gog->Stats ();

      IListenerRegistrar* registrar =
        gog->Registrar ();

      SK_RunOnce (
        //registrar->Register (            USER_TIME_PLAYED_RETRIEVE,
        //  (galaxy::api::IUserTimePlayedRetrieveListener           *)galaxy_achievements.getPtr () );
        registrar->Register ( USER_STATS_AND_ACHIEVEMENTS_RETRIEVE, dynamic_cast <
                         galaxy::api::IUserStatsAndAchievementsRetrieveListener *> (
                         galaxy_achievements.getPtr ()                             )
        );

        stats->RequestUserTimePlayed (
          gog->GetGalaxyID (),             dynamic_cast <
          galaxy::api::IUserTimePlayedRetrieveListener *> (
          galaxy_achievements.getPtr ()                   )
        );
      );

      static bool logged_in =
        SK_Galaxy_User_IsLoggedOn (gog->User ()) &&
        SK_Galaxy_User_SignedIn   (gog->User ());

      if (logged_in == false && SK_Galaxy_User_IsLoggedOn (gog->User ()) &&
                                SK_Galaxy_User_SignedIn   (gog->User ()) )
      {
        logged_in                               = true;
        galaxy_achievements->need_stats_refresh = true;
        galaxy_achievements->refresh_count      =    0;
        gog->galaxy_id_ = gog->User ()->GetGalaxyID ();
      }

      // Attempt to auto-recover from errors, but throttle any attempt to do so
      //   in order to avoid runaway filesystem checks for the JSON file.
      static DWORD
          dwLastChecked = 0;
      if (dwLastChecked < SK::ControlPanel::current_time - 500UL)
      {   dwLastChecked = SK::ControlPanel::current_time;
        if (galaxy_achievements->need_stats_refresh && PathFileExistsW (galaxy_achievements->global_stats_filename.c_str ()))
        {   galaxy_achievements->need_stats_refresh = false;

          // Give up after a few tries...
          if (++galaxy_achievements->refresh_count < 4)
          {
            SK_Galaxy_Stats_RequestUserStatsAndAchievements (
              stats, gog->GetGalaxyID (),                       dynamic_cast <
                     galaxy::api::IUserStatsAndAchievementsRetrieveListener *> (
                     galaxy_achievements.getPtr ()                             )
            );
          }
        }
      }
    }

    void
    __cdecl
    ProcessData_Detour (void)
    {
      InterlockedIncrement64 (&ticks);

      static bool
            crashed = false;
      if (! crashed)
      {
        __try {
          ProcessDataHook_Impl ();
        }

        __except (GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ?
                                         EXCEPTION_EXECUTE_HANDLER  :
                                         EXCEPTION_CONTINUE_SEARCH)
        {
          crashed = true;
        }
      }

      // The game's callback handlers might not be expecting to be invoked,
      //   and while that's a bug in the game itself, we can't let it crash.
      __try {
        ProcessData_Original ();
      }

      __except (GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ?
                                       EXCEPTION_EXECUTE_HANDLER  :
                                       EXCEPTION_CONTINUE_SEARCH)
      {
        gog_log->Log (
          L"Structured Exception encountered during ProcessData (...)"
        );
      }
    }

    void
    __cdecl
    ProcessData_IGalaxy_Detour (IGalaxy* This)
    {
      InterlockedIncrement64 (&ticks);

      static bool
            crashed = false;
      if (! crashed)
      {
        __try {
          ProcessDataHook_Impl ();
        }

        __except (GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ?
                                         EXCEPTION_EXECUTE_HANDLER  :
                                         EXCEPTION_CONTINUE_SEARCH)
        {
          crashed = true;
        }
      }

      // The game's callback handlers might not be expecting to be invoked,
      //   and while that's a bug in the game itself, we can't let it crash.
      __try {
        ProcessData_IGalaxy_Original (This);
      }

      __except (GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ?
                                       EXCEPTION_EXECUTE_HANDLER  :
                                       EXCEPTION_CONTINUE_SEARCH)
      {
        gog_log->Log (
          L"Structured Exception encountered during ProcessData (...)"
        );
      }
    }

    void
    __cdecl
    Shutdown_Detour (void)
    {
      SK_LOG_FIRST_CALL

      gog->Shutdown (true);

      Shutdown_Original ();
    }

    void
    __cdecl
    Shutdown_IGalaxy_Detour (IGalaxy* This)
    {
      SK_LOG_FIRST_CALL

      gog->Shutdown (true);

      Shutdown_IGalaxy_Original (This);
    }

    galaxy::api::IGalaxy*
    __cdecl
    CreateInstance_Detour (void)
    {
      SK_LOG_FIRST_CALL

      auto instance =
        CreateInstance_Original ();

      if (instance != nullptr)
      {
        GALAXY_VIRTUAL_HOOK ( &instance,      3,
                            "IGalaxy::Shutdown",
                             Shutdown_IGalaxy_Detour,
                             Shutdown_IGalaxy_Original,
                             Shutdown_IGalaxy_pfn );

        GALAXY_VIRTUAL_HOOK ( &instance,     16,
                            "IGalaxy::ProcessData",
                             ProcessData_IGalaxy_Detour,
                             ProcessData_IGalaxy_Original,
                             ProcessData_IGalaxy_pfn );

        bool bEnable = SK_EnableApplyQueuedHooks ();
                             SK_ApplyQueuedHooks ();
        if (!bEnable) SK_DisableApplyQueuedHooks ();
      }

      return
        instance;
    }

    void
    __cdecl
    Init_Detour (struct galaxy::api::InitOptions const& initOptions)
    {
      SK_LOG_FIRST_CALL

      const char* clientID     = initOptions.clientID;
      const char* clientSecret = initOptions.clientSecret;

      // The API changed at 1.121.2 to use a slightly altered structure defined
      //   in InitOptions.h instead of IGalaxy.h
      if (gog->version >= SK_GalaxyContext::Version_1_121_2)
      {
        InitOptions_1_121_2& initOptions_Versioned =
          (InitOptions_1_121_2 &)initOptions;

        clientID     = initOptions_Versioned.clientID;
        clientSecret = initOptions_Versioned.clientSecret;
      }

      gog_log->Log (L"clientID.......: %hs", clientID);
      gog_log->Log (L"clientSecret...: %hs", clientSecret);

      Init_Original (initOptions);

      const auto Stats     = (IStats*             (__cdecl *)(void))
        SK_GetProcAddress (gog->GetGalaxyDLL (), "?Stats@api@galaxy@@YAPEAVIStats@12@XZ"); // 64-bit
      const auto Utils     = (IUtils*             (__cdecl *)(void))
        SK_GetProcAddress (gog->GetGalaxyDLL (), "?Utils@api@galaxy@@YAPEAVIUtils@12@XZ"); // 64-bit
      const auto User      = (IUser*              (__cdecl *)(void))
        SK_GetProcAddress (gog->GetGalaxyDLL (), "?User@api@galaxy@@YAPEAVIUser@12@XZ"); // 64-bit
      const auto Friends   = (IFriends*           (__cdecl *)(void))
        SK_GetProcAddress (gog->GetGalaxyDLL (), "?Friends@api@galaxy@@YAPEAVIFriends@12@XZ"); // 64-bit
      const auto Registrar = (IListenerRegistrar* (__cdecl *)(void))
        SK_GetProcAddress (gog->GetGalaxyDLL (), "?ListenerRegistrar@api@galaxy@@YAPEAVIListenerRegistrar@12@XZ"); // 64-bit

      IStats*             pStats     = Stats     != nullptr ? Stats     () : nullptr;
      IUtils*             pUtils     = Utils     != nullptr ? Utils     () : nullptr;
      IUser*              pUser      = User      != nullptr ? User      () : nullptr;
      IFriends*           pFriends   = Friends   != nullptr ? Friends   () : nullptr;
      IListenerRegistrar* pRegistrar = Registrar != nullptr ? Registrar () : nullptr;

      gog->Init (pStats, pUtils, pUser, pFriends, pRegistrar);
    }
  }
}

void
SK::Galaxy::Init (void)
{
  if (config.platform.silent)
    return;

  SK_PROFILE_FIRST_CALL

  static auto _InitGOGLog = [&](void)
  {
    if (gog_log->fLog == nullptr)
    {
      gog_log->init (L"logs/galaxy.log", L"wt+,ccs=UTF-8");
      gog_log->silent = config.platform.silent;
    }
  };

  SK_RunOnce (
    int             gog_gameId =  0;
    WIN32_FIND_DATA fd         = {};

    auto _FindGameInfo = [&](const wchar_t* wszPattern)
    {
      HANDLE hFind =
        FindFirstFileW (wszPattern, &fd);

      if (hFind != INVALID_HANDLE_VALUE)
      {
        do
        {
          if (fd.dwFileAttributes != INVALID_FILE_ATTRIBUTES)
          {
            if (1 == swscanf (fd.cFileName, L"goggame-%d.info", &gog_gameId))
            {
              _InitGOGLog ();

              gog_log->Log (
                L"GOG gameId: %d", gog_gameId
              );

              if (gog_gameId != 0)
              {
                config.platform.type = SK_Platform_GOG;
              }

              if (config.platform.equivalent_steam_app == -1)
              {
                std::wstring url =
                  SK_FormatStringW (
                    LR"(https://www.pcgamingwiki.com/w/api.php?action=cargoquery&format=json&tables=Infobox_game&fields=Steam_AppID&where=GOGcom_ID+HOLDS+%%27%d%%27)",
                                                                                                                                         gog_gameId
                  );

                SK_Network_EnqueueDownload (
                  sk_download_request_s (L"pcgw_appid_map.json", url.data (),
                    []( const std::vector <uint8_t>&& data,
                        const std::wstring_view       file )
                    {
                      if (data.empty ())
                        return true;

                      std::ignore = file;

                      try {
                        gog_log->Log (L"PCGW Queried Data: %hs", data.data ());

                        nlohmann::json json =
                          std::move (
                            nlohmann::json::parse ( data.cbegin (),
                                                    data.cend   (), nullptr, true )
                          );

                        for ( auto& query : json ["cargoquery"] )
                        {
                          if ( query.contains ("title") &&
                               query.at       ("title").contains ("Steam AppID") )
                          {
                            config.platform.equivalent_steam_app =
                              atoi (
                                query.at ("title").
                                      at ("Steam AppID").
                                      get <std::string_view> ().
                                                        data () );
                            break;
                          }
                        }

                        gog_log->Log (L"Steam AppID: %d", config.platform.equivalent_steam_app);

                        if (config.platform.equivalent_steam_app != -1)
                        {   config.utility.save_async ();
                        }

                        else
                        {
                          config.platform.equivalent_steam_app = 0;
                        }
                      }

                      catch (const std::exception& e)
                      {
                        if (config.system.log_level > 0)
                        {
/*
#ifdef __CPP20
                          const auto&& src_loc =
                            std::source_location::current ();

                          steam_log->Log ( L"%hs (%d;%d): json parse failure: %hs",
                                                       src_loc.file_name     (),
                                                       src_loc.line          (),
                                                       src_loc.column        (), e.what ());
                          steam_log->Log (L"%hs",      src_loc.function_name ());
                          steam_log->Log (L"%hs",
                            std::format ( std::string ("{:*>") +
                                       std::to_string (src_loc.column        ()), 'x').c_str ());
#else
*/
                          std::ignore = e;
/*#endif*/
                       }
                     }

                     return true;
                   }
                 )
               );
             }

             break;
           }
         }
       } while (FindNextFileW (hFind, &fd) != 0);

       FindClose (hFind);
      }
    };

    if (config.platform.equivalent_steam_app == -1)
    {
      // Stupid way of dealing with Unreal Engine games
                                     _FindGameInfo (               L"goggame-*.info");
      if         (gog_gameId == 0) { _FindGameInfo (            L"../goggame-*.info");
        if       (gog_gameId == 0) { _FindGameInfo (         L"../../goggame-*.info");
          if     (gog_gameId == 0) { _FindGameInfo (      L"../../../goggame-*.info");
            if   (gog_gameId == 0) { _FindGameInfo (   L"../../../../goggame-*.info");
              if (gog_gameId == 0) { _FindGameInfo (L"../../../../../goggame-*.info"); } } } } }

      if (gog_gameId != 0)
      {
        config.platform.type = SK_Platform_GOG;
      }
    }

    if (config.platform.type._Equal (SK_Platform_GOG))
    {
      _InitGOGLog ();

      if (config.galaxy.spawn_mini_client)
      {
        SK_Thread_CreateEx ([](LPVOID)->DWORD
        {
          if (! SK_IsProcessRunning (L"GalaxyCommunication.exe"))
          {
            std::filesystem::path exec_dir (SK_GetProgramDataDir ());
                                  exec_dir /= LR"(GOG.com\Galaxy\redists\)";
            std::filesystem::path exec_target
                                 (exec_dir / L"GalaxyCommunication.exe");

            gog_log->Log (L"Attempting to start: %ws...", exec_target.c_str ());
            gog_log->Log (L"-------------------");

            SK_ShellExecuteW (
              HWND_DESKTOP,
                L"open", exec_target.c_str (), nullptr,
                         exec_dir   .c_str (), SW_SHOWNORMAL
            );
          }

          SK_Thread_CloseSelf ();

          return 0;
        }, L"[SK] Galaxy Kickstart Thread");
      }
    }
  );

  static const wchar_t*
    wszGalaxyDLLName =
      SK_RunLHIfBitness ( 64, L"Galaxy64.dll",
                                L"Galaxy.dll" );

  static HMODULE     hModGalaxy = nullptr;
  if (std::exchange (hModGalaxy, SK_LoadLibraryW (wszGalaxyDLLName)) == nullptr && hModGalaxy != nullptr)
  {
    _InitGOGLog ();

    gog->PreInit (hModGalaxy);
    gog_log->Log (
      L"Galaxy DLL.....: %ws",
        SK_GetModuleFullName (gog->GetGalaxyDLL ()).c_str ()
    );

    std::wstring ver_str =
      SK_GetDLLVersionShort (wszGalaxyDLLName);

    int                                         major,  minor,  build,  rev = 0;
    swscanf (ver_str.c_str (), L"%d.%d.%d.%d", &major, &minor, &build, &rev);

    if (rev == 0)
      gog_log->Log (L"Galaxy Version.: %d.%d.%d",    major, minor, build);
    else // This is never expected to be non-zero
      gog_log->Log (L"Galaxy Version.: %d.%d.%d.%d", major, minor, build, rev);

    //
    // Find the highest version implemented and assign the appropriate enum value;
    //
    //   Enum values are sorted in ascending order so inequalities can be used.
    //
    if (     (major == 1 && (minor > 152 || (minor == 152 && build >= 10))) || major > 1)
      gog->version = SK_GalaxyContext::Version_1_152_10;
    else if ((major == 1 && (minor > 152 || (minor == 152 && build >= 1))))
      gog->version = SK_GalaxyContext::Version_1_152_1;
    else if ((major == 1 && (minor > 151 || (minor == 151 && build >= 0))))
      gog->version = SK_GalaxyContext::Version_1_151_0;
    else if ((major == 1 && (minor > 121 || (minor == 121 && build >= 2))))
      gog->version = SK_GalaxyContext::Version_1_121_2;

    SK_ICommandProcessor* cmd = nullptr;

    #define cmdAddAliasedVar(name,pVar)                 \
      for ( const char* alias : { "GOG."      #name,    \
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
    }
  }

  auto _SetupGalaxy =
  [&](void)
  {
    // Hook code here (i.e. Listener registrar Register/Unregister and IGalaxy::ProcessData (...))
#ifdef _M_AMD64
    SK_CreateDLLHook2 (     wszGalaxyDLLName, "?Init@api@galaxy@@YAXAEBUInitOptions@12@@Z",
                               galaxy::api::Init_Detour,
      static_cast_p2p <void> (&galaxy::api::Init_Original) );
    SK_CreateDLLHook2 (     wszGalaxyDLLName, "?ProcessData@api@galaxy@@YAXXZ",
                               galaxy::api::ProcessData_Detour,
      static_cast_p2p <void> (&galaxy::api::ProcessData_Original) );
    SK_CreateDLLHook2 (     wszGalaxyDLLName, "?Shutdown@api@galaxy@@YAXXZ",
                               galaxy::api::Shutdown_Detour,
      static_cast_p2p <void> (&galaxy::api::Shutdown_Original) );

#else
    //SK_CreateDLLHook2 (     wszGalaxyDLLName, "?Init@api@galaxy@@YAXAEBUInitOptions@12@@Z", //64-bit name
    //                           galaxy::api::Init_Detour,
    //  static_cast_p2p <void> (&galaxy::api::Init_Original) );
    SK_CreateDLLHook2 (     wszGalaxyDLLName, "?CreateInstance@GalaxyFactory@api@galaxy@@SAPAVIGalaxy@23@XZ",
                               galaxy::api::CreateInstance_Detour,
      static_cast_p2p <void> (&galaxy::api::CreateInstance_Original) );
    //SK_CreateDLLHook2 (     wszGalaxyDLLName, "?ProcessData@api@galaxy@@YAXXZ", // 64-bit name
    //                           galaxy::api::ProcessData_Detour,
    //  static_cast_p2p <void> (&galaxy::api::ProcessData_Original) );
    //SK_CreateDLLHook2 (     wszGalaxyDLLName, "?Shutdown@api@galaxy@@YAXXZ", // 64-bit name
    //                           galaxy::api::Shutdown_Detour,
    //  static_cast_p2p <void> (&galaxy::api::Shutdown_Original) );

    galaxy::api::IGalaxy** ppInstance =
      (galaxy::api::IGalaxy **)SK_GetProcAddress (wszGalaxyDLLName, "?instance@GalaxyFactory@api@galaxy@@0PAVIGalaxy@23@A");

    if (ppInstance != nullptr && *ppInstance != nullptr)
    {
      galaxy::api::IStats*             pStats     = (*ppInstance)->GetStats             ();
      galaxy::api::IUtils*             pUtils     = (*ppInstance)->GetUtils             ();
      galaxy::api::IUser*              pUser      = (*ppInstance)->GetUser              ();
      galaxy::api::IFriends*           pFriends   = (*ppInstance)->GetFriends           ();
      galaxy::api::IListenerRegistrar* pRegistrar = (*ppInstance)->GetListenerRegistrar ();

      gog->Init (pStats, pUtils, pUser, pFriends, pRegistrar);

      GALAXY_VIRTUAL_HOOK ( ppInstance,      3,
                          "IGalaxy::Shutdown",
                           galaxy::api::Shutdown_IGalaxy_Detour,
                           galaxy::api::Shutdown_IGalaxy_Original,
                                        Shutdown_IGalaxy_pfn );

      GALAXY_VIRTUAL_HOOK ( ppInstance,     16,
                          "IGalaxy::ProcessData",
                           galaxy::api::ProcessData_IGalaxy_Detour,
                           galaxy::api::ProcessData_IGalaxy_Original,
                                        ProcessData_IGalaxy_pfn );
    }
#endif

    SK_ApplyQueuedHooks ();
  };

  if (hModGalaxy != nullptr)
  {
    SK_RunOnce (
      if (SK::Galaxy::GetTicksRetired () == 0)
        _SetupGalaxy ();
    );
  }
}

void
SK::Galaxy::Shutdown (void)
{
}

void
SK_GalaxyContext::PreInit (HMODULE hGalaxyDLL)
{
  sdk_dll_ = hGalaxyDLL;
}

class SK_IPersonaDataChangedListener : public galaxy::api::IPersonaDataChangedListener
{
public:
  virtual void OnPersonaDataChanged ( galaxy::api::GalaxyID userID,
                                                   uint32_t personaStateChange ) final
  {
    if (userID == gog->GetGalaxyID ())
    {
      if (personaStateChange == PERSONA_CHANGE_NAME ||
          personaStateChange == PERSONA_CHANGE_NONE)
      {
// Does not work, needs more debuggin...
#if 0
        char                                 persona_name [512] = {};
        gog->Friends ()->GetPersonaNameCopy (persona_name, 511);
        gog->user_names.display_name    =    persona_name;
        gog->user_names.nickname        =    persona_name;
#endif
      }
    }
  }
} static persona_data_change;

void
SK_GalaxyContext::Init ( galaxy::api::IStats*             stats,
                         galaxy::api::IUtils*             utils,
                         galaxy::api::IUser*              user,
                         galaxy::api::IFriends*           friends,
                         galaxy::api::IListenerRegistrar* registrar )
{
  //gog_log->Log (
  //  L"SK_GalaxyContext::Init (%p, %p, %p, %p, %p)",
  //    stats, utils, user, friends, registrar );

  stats_     = stats;
  utils_     = utils;
  user_      = user;
  registrar_ = registrar;
  friends_   = friends;

  if (registrar_ != nullptr)
  {
    registrar_->Register ( persona_data_change.GetListenerType (),
                          &persona_data_change );
    registrar_->Register ( galaxy_overlay->GetListenerType (),
                           galaxy_overlay.getPtr           () );
  }

  bool logged_on = false;

  if (user_ != nullptr)
  {
    logged_on =
      SK_Galaxy_User_IsLoggedOn (user_) &&
      SK_Galaxy_User_SignedIn   (user_);

    if (! logged_on)
    {
      class SK_IAuthListener : public galaxy::api::IAuthListener
      {
      public:
        virtual void OnAuthSuccess (void) final
        {
          gog->galaxy_id_ =
            gog->User ()->GetGalaxyID ();

          gog_log->Log (
            L"SignInGalaxy (...) success! GalaxyID=%d",
                                  gog->GetGalaxyID ().ToUint64 () );

// Does not work, needs more debuggin...
#if 0
          if (gog->Friends () != nullptr)
          {
            char                                  persona_name [512] = {};
            gog->Friends ()->GetPersonaNameCopy ( persona_name, 511 );
            gog->user_names.display_name        = persona_name;
            gog->user_names.nickname            = persona_name;

            gog->Friends ()->RequestUserInformation (gog->GetGalaxyID ());
          }
#endif

          SK_Galaxy_Stats_RequestUserStatsAndAchievements ( gog->Stats       (),
                                                            gog->GetGalaxyID () );
        }

        virtual void OnAuthFailure (
          galaxy::api::IAuthListener::FailureReason
                                      failureReason ) final
        {
          gog_log->Log (
            L"LogIn Authorization Failed, Error=%d", failureReason
          );
        }

        virtual void OnAuthLost (void) final
        {
          gog_log->Log (
            L"Authorization Lost?!"
          );
        }
      } static auth_listener;

      if (SK_IsProcessRunning (L"GalaxyCommunication.exe"))
      {
        SK_Galaxy_User_SignInGalaxy ( user_, config.galaxy.require_online_mode,
                                             config.galaxy.require_online_mode ? 15 : 5, &auth_listener );
      }
    }

    galaxy_id_ = user_->GetGalaxyID ();
  }

  // Does not work, needs more debuggin...
#if 0
  if (friends_ != nullptr)
  {
    char                           persona_name [512] = {};
    friends_->GetPersonaNameCopy ( persona_name, 511 );
    gog->user_names.display_name = persona_name;
    gog->user_names.nickname     = persona_name;

    friends_->RequestUserInformation (gog->GetGalaxyID ());
  }
#endif
}

void
SK_GalaxyContext::Shutdown (bool bGameRequested)
{
  if (! ReadAcquire (&__SK_DLL_Ending))
  {
    gog_log->Log (L"SK_GalaxyContext::Shutdown (...)");
  }

  std::ignore = bGameRequested;

  if (registrar_ != nullptr) {
      registrar_->Unregister ( persona_data_change.GetListenerType (),
                              &persona_data_change );
      registrar_->Unregister ( galaxy_overlay->GetListenerType (),
                               galaxy_overlay.getPtr           () );
  }

  stats_     = nullptr;
  utils_     = nullptr;
  user_      = nullptr;
  registrar_ = nullptr;
  friends_   = nullptr;
}

bool
__stdcall
SK_Galaxy_GetOverlayState (bool real)
{
  return
    SK::Galaxy::GetOverlayState (real);
}


SK_LazyGlobal <SK_GalaxyContext> gog;
bool SK::Galaxy::overlay_state = false;

galaxy::api::GalaxyID
SK::Galaxy::UserID (void)
{
  return gog->GetGalaxyID ().ToUint64 () != 0 ?
         gog->GetGalaxyID ()                  :
    galaxy::api::GalaxyID ();
}

std::string_view
SK::Galaxy::PlayerName (void)
{
  std::string_view view =
    gog->GetDisplayName ();

  if (view.empty ())
    return "";

  return view;
}

std::string_view
SK::Galaxy::PlayerNickname (void)
{
  std::string_view view =
    gog->GetNickName ();

  if (view.empty ())
  {
    view =
      gog->GetDisplayName ();

    if (view.empty ())
      return "";
  }

  return view;
}

LONGLONG
SK::Galaxy::GetTicksRetired (void)
{
  return
    ReadAcquire64 (&galaxy::api::ticks);
}

bool
SK::Galaxy::IsSignedIn (void)
{
  auto user =
    gog->User ();

  if (user != nullptr)
  {
    return
      SK_Galaxy_User_SignedIn (user);
  }

  return false;
}

bool
SK::Galaxy::IsLoggedOn (void)
{
  auto user =
    gog->User ();

  if (user != nullptr)
  {
    return
      SK_Galaxy_User_IsLoggedOn (user);
  }

  return false;
}

uint32_t
SK_Galaxy_Stats_GetUserTimePlayed (galaxy::api::IStats* This)
{
  //
  // Galaxy API is full of all kinds of breaking API changes, so
  //   crashing is inevitable on older builds of Galaxy SDK until
  //     SK has reviewed all of the official releases...
  // 
  //  * Try to avoid a crash for now.
  //
  __try {
    switch (gog->version)
    {
      default:
      case SK_GalaxyContext::Version_1_152_1:
        return ((galaxy::api::IStats_1_152_1 *)This)->GetUserTimePlayed ();

      case SK_GalaxyContext::Version_1_152_10:
        return ((galaxy::api::IStats_1_152_10*)This)->GetUserTimePlayed ();
    }
  } __except (GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ?
                                     EXCEPTION_EXECUTE_HANDLER  :
                                     EXCEPTION_CONTINUE_SEARCH ){}

  return 0;
}

void
SK_Galaxy_Stats_GetAchievementNameCopy ( galaxy::api::IStats* This,
                                                    uint32_t  index,
                                                        char* buffer,
                                                    uint32_t  bufferLength )
{
  //
  // Galaxy API is full of all kinds of breaking API changes, so
  //   crashing is inevitable on older builds of Galaxy SDK until
  //     SK has reviewed all of the official releases...
  // 
  //  * Try to avoid a crash for now.
  //
  __try {
    switch (gog->version)
    {
      default:
      case SK_GalaxyContext::Version_1_152_1:
        // Not Implemented
        //((galaxy::api::IStats_1_152_1 *)This)->GetAchievementNameCopy (index, buffer, bufferLength);
        break;

      case SK_GalaxyContext::Version_1_152_10:
        ((galaxy::api::IStats_1_152_10 *)This)->GetAchievementNameCopy (index, buffer, bufferLength);
        break;
    }
  } __except (GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ?
                                     EXCEPTION_EXECUTE_HANDLER  :
                                     EXCEPTION_CONTINUE_SEARCH ){}
}
void
SK_Galaxy_Stats_GetAchievement ( galaxy::api::IStats* This,
                                          const char* name,
                                                bool& unlocked,
                                            uint32_t& unlockTime,
                               galaxy::api::GalaxyID  userID )
{
  //
  // Galaxy API is full of all kinds of breaking API changes, so
  //   crashing is inevitable on older builds of Galaxy SDK until
  //     SK has reviewed all of the official releases...
  // 
  //  * Try to avoid a crash for now.
  //
  __try {
    switch (gog->version)
    {
      default:
      case SK_GalaxyContext::Version_1_152_1:
        ((galaxy::api::IStats_1_152_1 *)This)->GetAchievement (name, unlocked, unlockTime, userID);
        break;

      case SK_GalaxyContext::Version_1_152_10:
        ((galaxy::api::IStats_1_152_10*)This)->GetAchievement (name, unlocked, unlockTime, userID);
        break;
    }
  } __except (GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ?
                                     EXCEPTION_EXECUTE_HANDLER  :
                                     EXCEPTION_CONTINUE_SEARCH ){}
}

void
SK_Galaxy_Stats_GetAchievementDisplayNameCopy ( galaxy::api::IStats* This,
                                                         const char* name,
                                                               char* buffer,
                                                           uint32_t  bufferLength )
{
  //
  // Galaxy API is full of all kinds of breaking API changes, so
  //   crashing is inevitable on older builds of Galaxy SDK until
  //     SK has reviewed all of the official releases...
  // 
  //  * Try to avoid a crash for now.
  //
  __try {
    switch (gog->version)
    {
      default:
      case SK_GalaxyContext::Version_1_152_1:
        ((galaxy::api::IStats_1_152_1 *)This)->GetAchievementDisplayNameCopy (name, buffer, bufferLength);
        break;

      case SK_GalaxyContext::Version_1_152_10:
        ((galaxy::api::IStats_1_152_10*)This)->GetAchievementDisplayNameCopy (name, buffer, bufferLength);
        break;
    }
  } __except (GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ?
                                     EXCEPTION_EXECUTE_HANDLER  :
                                     EXCEPTION_CONTINUE_SEARCH ){}
}
void
SK_Galaxy_Stats_GetAchievementDescriptionCopy ( galaxy::api::IStats* This,
                                                         const char* name,
                                                               char* buffer,
                                                           uint32_t  bufferLength )
{
  //
  // Galaxy API is full of all kinds of breaking API changes, so
  //   crashing is inevitable on older builds of Galaxy SDK until
  //     SK has reviewed all of the official releases...
  // 
  //  * Try to avoid a crash for now.
  //
  __try {
    switch (gog->version)
    {
      default:
      case SK_GalaxyContext::Version_1_152_1:
        ((galaxy::api::IStats_1_152_1 *)This)->GetAchievementDescriptionCopy (name, buffer, bufferLength);
        break;

      case SK_GalaxyContext::Version_1_152_10:
        ((galaxy::api::IStats_1_152_10*)This)->GetAchievementDescriptionCopy (name, buffer, bufferLength);
        break;
    }
  } __except (GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ?
                                     EXCEPTION_EXECUTE_HANDLER  :
                                     EXCEPTION_CONTINUE_SEARCH ){}
}

bool
SK_Galaxy_Stats_IsAchievementVisibleWhileLocked ( galaxy::api::IStats* This,
                                                           const char* name )
{
  //
  // Galaxy API is full of all kinds of breaking API changes, so
  //   crashing is inevitable on older builds of Galaxy SDK until
  //     SK has reviewed all of the official releases...
  // 
  //  * Try to avoid a crash for now.
  //
  __try {
    switch (gog->version)
    {
      default:
      case SK_GalaxyContext::Version_1_152_1:
        return ((galaxy::api::IStats_1_152_1 *)This)->IsAchievementVisibleWhileLocked (name);

      case SK_GalaxyContext::Version_1_152_10:
        return ((galaxy::api::IStats_1_152_10*)This)->IsAchievementVisibleWhileLocked (name);
    }
  } __except (GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ?
                                     EXCEPTION_EXECUTE_HANDLER  :
                                     EXCEPTION_CONTINUE_SEARCH ){}

  return false;
}

void
SK_Galaxy_Stats_RequestUserTimePlayed ( galaxy::api::IStats*                                This,
                                        galaxy::api::GalaxyID                               userID,
                                        galaxy::api::IUserTimePlayedRetrieveListener* const listener )
{
  //
  // Galaxy API is full of all kinds of breaking API changes, so
  //   crashing is inevitable on older builds of Galaxy SDK until
  //     SK has reviewed all of the official releases...
  // 
  //  * Try to avoid a crash for now.
  //
  __try {
    switch (gog->version)
    {
      default:
      case SK_GalaxyContext::Version_1_152_1:
        ((galaxy::api::IStats_1_152_1 *)This)->RequestUserTimePlayed (userID, listener);
        break;

      case SK_GalaxyContext::Version_1_152_10:
        ((galaxy::api::IStats_1_152_10*)This)->RequestUserTimePlayed (userID, listener);
        break;
    }
  } __except (GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ?
                                     EXCEPTION_EXECUTE_HANDLER  :
                                     EXCEPTION_CONTINUE_SEARCH ){}
}

void
SK_Galaxy_Stats_RequestUserStatsAndAchievements (
  galaxy::api::IStats*                                          This,
  galaxy::api::GalaxyID                                         userID,
  galaxy::api::IUserStatsAndAchievementsRetrieveListener* const listener )
{
  SK_RunOnce (
    gog_log->Log (L"galaxy::api::IStats::RequestUserStatsAndAchievements (...)")
  );

  //
  // Galaxy API is full of all kinds of breaking API changes, so
  //   crashing is inevitable on older builds of Galaxy SDK until
  //     SK has reviewed all of the official releases...
  // 
  //  * Try to avoid a crash for now.
  //
  __try {
    switch (gog->version)
    {
      default:
      case SK_GalaxyContext::Version_1_152_1:
        ((galaxy::api::IStats_1_152_1 *)This)->RequestUserStatsAndAchievements (userID, listener);
        break;

      case SK_GalaxyContext::Version_1_152_10:
        ((galaxy::api::IStats_1_152_10*)This)->RequestUserStatsAndAchievements (userID, listener);
        break;
    }
  } __except (GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ?
                                     EXCEPTION_EXECUTE_HANDLER  :
                                     EXCEPTION_CONTINUE_SEARCH ){}
}

void
SK_Galaxy_Stats_StoreStatsAndAchievements ( galaxy::api::IStats*       This,
                galaxy::api::IStatsAndAchievementsStoreListener* const _listener = nullptr )
{
  //
  // Galaxy API is full of all kinds of breaking API changes, so
  //   crashing is inevitable on older builds of Galaxy SDK until
  //     SK has reviewed all of the official releases...
  // 
  //  * Try to avoid a crash for now.
  //
  __try {
    galaxy::api::IStatsAndAchievementsStoreListener* listener =
                                                    _listener;

    if (listener == nullptr)
    {
      listener =
        (galaxy::api::IStatsAndAchievementsStoreListener *)galaxy_achievements.getPtr ();
    }

    switch (gog->version)
    {
      default:
      case SK_GalaxyContext::Version_1_152_1:
        ((galaxy::api::IStats_1_152_1 *)This)->StoreStatsAndAchievements (listener);
        break;

      case SK_GalaxyContext::Version_1_152_10:
        ((galaxy::api::IStats_1_152_10*)This)->StoreStatsAndAchievements (listener);
        break;
    }
  } __except (GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ?
                                     EXCEPTION_EXECUTE_HANDLER  :
                                     EXCEPTION_CONTINUE_SEARCH ){}
}

void
SK_Galaxy_Stats_SetAchievement ( galaxy::api::IStats* This,
                                          const char* name )
{
  //
  // Galaxy API is full of all kinds of breaking API changes, so
  //   crashing is inevitable on older builds of Galaxy SDK until
  //     SK has reviewed all of the official releases...
  // 
  //  * Try to avoid a crash for now.
  //
  __try {
    switch (gog->version)
    {
      default:
      case SK_GalaxyContext::Version_1_152_1:
        ((galaxy::api::IStats_1_152_1 *)This)->SetAchievement (name);
        break;

      case SK_GalaxyContext::Version_1_152_10:
        ((galaxy::api::IStats_1_152_10*)This)->SetAchievement (name);
        break;
    }

    SK_Galaxy_Stats_StoreStatsAndAchievements (This);
  } __except (GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ?
                                     EXCEPTION_EXECUTE_HANDLER  :
                                     EXCEPTION_CONTINUE_SEARCH ){}
}

void
SK_Galaxy_ClearAchievement (const char* szName)
{
  SK_Galaxy_Stats_ClearAchievement (gog->Stats (), szName);
}

void
SK_Galaxy_UnlockAchievement (const char* szName)
{
  SK_Galaxy_Stats_SetAchievement (gog->Stats (), szName);
}

void
SK_Galaxy_Stats_ClearAchievement ( galaxy::api::IStats* This,
                                            const char* name )
{
  //
  // Galaxy API is full of all kinds of breaking API changes, so
  //   crashing is inevitable on older builds of Galaxy SDK until
  //     SK has reviewed all of the official releases...
  // 
  //  * Try to avoid a crash for now.
  //
  __try {
    switch (gog->version)
    {
      default:
      case SK_GalaxyContext::Version_1_152_1:
        ((galaxy::api::IStats_1_152_1 *)This)->ClearAchievement (name);
        break;

      case SK_GalaxyContext::Version_1_152_10:
        ((galaxy::api::IStats_1_152_10*)This)->ClearAchievement (name);
        break;
    }

    SK_Galaxy_Stats_StoreStatsAndAchievements (This);
  } __except (GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ?
                                     EXCEPTION_EXECUTE_HANDLER  :
                                     EXCEPTION_CONTINUE_SEARCH )
  {
  }
}

bool
SK_GalaxyOverlay_GoToURL (const char* szURL, bool bUseWindowsShellIfOverlayFails)
{
  // This functionality appears to be broken, it just claims the GOG client is
  //   not connected... so fallback to the Windows shell "open" command.
#if 0
  auto utils  = gog->Utils ();
  if ( utils != nullptr )
  {
    if (SK_GetModuleHandleW (L"overlay_mediator_x64_Release.dll"))
    {
      utils->ShowOverlayWithWebPage (szURL);
      return true;
    }
  }
#endif

  if (bUseWindowsShellIfOverlayFails)
  {
    SK_ShellExecuteA ( nullptr, "open",
                         szURL, nullptr, nullptr, SW_NORMAL );
    return true;
  }

  return false;
}

const wchar_t*
SK_GetGalaxyDir (void)
{
  static wchar_t
       wszGalaxyPath [MAX_PATH + 2] = { };
  if (*wszGalaxyPath == L'\0')
  {
    // Don't keep querying the registry if Galaxy is not installed
    wszGalaxyPath [0] = L'?';

    DWORD     len    =      MAX_PATH;
    LSTATUS   status =
      RegGetValueW ( HKEY_CURRENT_USER,
                       LR"(SOFTWARE\WOW6432Node\GOG.com\GalaxyClient\paths\)",
                                                            L"client",
                         RRF_RT_REG_SZ,
                           nullptr,
                             wszGalaxyPath,
                               (LPDWORD)&len );

    if (status == ERROR_SUCCESS)
      return wszGalaxyPath;
    else
    {
      len    = MAX_PATH;
      status =
        RegGetValueW ( HKEY_CURRENT_USER,
                         LR"(SOFTWARE\GOG.com\GalaxyClient\paths\)",
                                                  L"client",
                           RRF_RT_REG_SZ,
                             nullptr,
                               wszGalaxyPath,
                                 (LPDWORD)&len );

      if (status == ERROR_SUCCESS)
        return wszGalaxyPath;
      else
        return L"";
    }
  }

  return wszGalaxyPath;
}