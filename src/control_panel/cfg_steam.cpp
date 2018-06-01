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

#include <imgui/imgui.h>

#include <SpecialK/control_panel.h>
#include <SpecialK/control_panel/steam.h>

#include <SpecialK/core.h>
#include <SpecialK/config.h>
#include <SpecialK/utility.h>

#include <SpecialK/steam_api.h>

extern volatile LONG SK_SteamAPI_CallbackRateLimit;

using namespace SK::ControlPanel;

extern bool
SK_Denuvo_UsedByGame (bool retest = false);

struct denuvo_file_s
{
  AppId_t      app;
  CSteamID     user;
  uint64       hash;
  std::wstring path;
  FILETIME     ft_key;
  SYSTEMTIME   st_local;
};

extern std::vector <denuvo_file_s> denuvo_files;


 extern void SKX_Steam_ScreenshotMgr_Reinit (void);


bool
SK::ControlPanel::Steam::Draw (void)
{
  ImGuiIO& io =
    ImGui::GetIO ();

  if (SK::SteamAPI::AppID () != 0)
  {
    if ( ImGui::CollapsingHeader ("Steam Enhancements", ImGuiTreeNodeFlags_CollapsingHeader | 
                                                        ImGuiTreeNodeFlags_DefaultOpen ) )
    {
      ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.02f, 0.68f, 0.90f, 0.45f));
      ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.07f, 0.72f, 0.90f, 0.80f));
      ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.14f, 0.78f, 0.87f, 0.80f));
      ImGui::TreePush       ("");

      if (SK_SteamAPI_GetNumPossibleAchievements () > 0)
      {
        static char szProgress [128] = { };

        float  ratio            = SK::SteamAPI::PercentOfAchievementsUnlocked ();
        size_t num_achievements = SK_SteamAPI_GetNumPossibleAchievements      ();

        snprintf ( szProgress, 127, "%.2f%% of Achievements Unlocked (%u/%u)",
                     ratio * 100.0f, static_cast <uint32_t> ((ratio * static_cast <float> (num_achievements))),
                                     static_cast <uint32_t> (                              num_achievements) );

        ImGui::PushStyleColor ( ImGuiCol_PlotHistogram, ImColor (0.90f, 0.72f, 0.07f, 0.80f) ); 
        ImGui::ProgressBar    ( ratio,
                                  ImVec2 (-1, 0),
                                    szProgress );
        ImGui::PopStyleColor  ();

        int friends =
          SK_SteamAPI_GetNumFriends ();

        if (friends && ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip   ();

        //static int num_records = 0;

          auto max_lines   = static_cast <int> ((io.DisplaySize.y * 0.725f) / (font.size_multiline * 0.9f));
          int  cur_line    = 0;
          int  num_records = 0;

          ImGui::BeginGroup     ();

          ImGui::PushStyleColor ( ImGuiCol_Text,          ImColor (255, 255, 255)              ); 
          ImGui::PushStyleColor ( ImGuiCol_PlotHistogram, ImColor (0.90f, 0.72f, 0.07f, 0.80f) ); 

          for (int i = 0; i < (int)((float)friends * SK_SteamAPI_FriendStatPercentage ()); i++)
          {
            size_t      len     = 0;
            const char* szName  = SK_SteamAPI_GetFriendName (i, &len);

            float       percent =
              SK_SteamAPI_GetUnlockedPercentForFriend (i);

            if (percent > 0.0f)
            {
              ImGui::ProgressBar    ( percent, ImVec2 (io.DisplaySize.x * 0.0816f, 0.0f) );
              ImGui::SameLine       ( );
              ImGui::PushStyleColor (ImGuiCol_Text, ImColor (.81f, 0.81f, 0.81f));
              ImGui::Text           (szName);
              ImGui::PopStyleColor  (1);

              ++num_records;

              if (cur_line >= max_lines)
              {
                ImGui::EndGroup     ( );
                ImGui::SameLine     ( );
                ImGui::BeginGroup   ( );
                cur_line = 0;
              }

              else
                ++cur_line;
            }
          }

          ImGui::PopStyleColor  (2);
          ImGui::EndGroup       ( );
          ImGui::EndTooltip     ( );
        }

        if (ImGui::CollapsingHeader ("Achievements") )
        {
          ImGui::TreePush ("");
          ImGui::BeginGroup ();

          if (ImGui::Button (" Test Unlock "))
            SK_Steam_UnlockAchievement (0);

          if (ImGui::IsItemHovered ())
            ImGui::SetTooltip ("Perform a FAKE unlock so that you can tune your preferences.");

          ImGui::SameLine ();

          ImGui::Checkbox ("Play Sound ", &config.steam.achievements.play_sound);

          if (config.steam.achievements.play_sound) {
            ImGui::SameLine ();
            
            int i = 0;
            
            if (! _wcsicmp (config.steam.achievements.sound_file.c_str (), L"xbox"))
              i = 1;
            else
              i = 0;
            
            if (ImGui::Combo ("", &i, "PlayStation Network\0Xbox Live\0\0", 2))
            {
              if (i == 0) config.steam.achievements.sound_file = L"psn";
                     else config.steam.achievements.sound_file = L"xbox";
            
              SK_Steam_LoadUnlockSound (config.steam.achievements.sound_file.c_str ());
            }
          }

          ImGui::EndGroup ();
          ImGui::SameLine ();

          ImGui::Checkbox ("Take Screenshot", &config.steam.achievements.take_screenshot);

          ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.68f, 0.02f, 0.45f));
          ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.72f, 0.07f, 0.80f));
          ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.78f, 0.14f, 0.80f));

          bool uncollapsed = ImGui::CollapsingHeader ("Enhanced Popup");

          ImGui::SameLine (); ImGui::Checkbox        ("   Fetch Friend Unlock Stats", &config.steam.achievements.pull_friend_stats);

          if (uncollapsed)
          {
            ImGui::TreePush ("");

            int  mode    = (config.steam.achievements.popup.show + config.steam.achievements.popup.animate);
            bool changed = false;

            ImGui::Text          ("Draw Mode:");                              ImGui::SameLine ();

            changed |=
              ImGui::RadioButton ("Disabled ##AchievementPopup",   &mode, 0); ImGui::SameLine ();
            changed |=
              ImGui::RadioButton ("Stationary ##AchievementPopup", &mode, 1); ImGui::SameLine ();
            ImGui::BeginGroup    ( );
            changed |=
              ImGui::RadioButton ("Animated ##AchievementPopup",   &mode, 2);

              ImGui::SameLine    ( );
              ImGui::Combo       ( "##PopupLoc",         &config.steam.achievements.popup.origin,
                                           "Top-Left\0"
                                           "Top-Right\0"
                                           "Bottom-Left\0"
                                           "Bottom-Right\0\0" );

            if ( changed )
            {
              config.steam.achievements.popup.show    = (mode > 0);
              config.steam.achievements.popup.animate = (mode > 1);

              // Make sure the duration gets set non-zero when this changes
              if (config.steam.achievements.popup.show)
              {
                if ( config.steam.achievements.popup.duration == 0 )
                  config.steam.achievements.popup.duration = 6666UL;
              }
            }

            if (config.steam.achievements.popup.show)
            {
              ImGui::BeginGroup ( );
              ImGui::TreePush   ("");
              ImGui::Text       ("Duration:"); ImGui::SameLine ();

              float duration =
                std::max ( 1.0f, ( (float)config.steam.achievements.popup.duration / 1000.0f ) );

              if ( ImGui::SliderFloat ( "##PopupDuration", &duration, 1.0f, 30.0f, "%.2f Seconds" ) )
              {
                config.steam.achievements.popup.duration =
                  static_cast <LONG> ( duration * 1000.0f );
              }
              ImGui::TreePop   ( );
              ImGui::EndGroup  ( );
            }
            ImGui::EndGroup    ( );
            
            //ImGui::SliderFloat ("Inset Percentage",    &config.steam.achievements.popup.inset, 0.0f, 1.0f, "%.3f%%", 0.01f);
            ImGui::TreePop     ( );
          }

          ImGui::TreePop       ( );
          ImGui::PopStyleColor (3);
        }
      }

      ISteamUtils* utils =
        SK_SteamAPI_Utils ();

      if (utils != nullptr && utils->IsOverlayEnabled () && ImGui::CollapsingHeader ("Overlay Notifications"))
      {
        ImGui::TreePush  ("");

        if (ImGui::Combo ( " ", &config.steam.notify_corner,
                                  "Top-Left\0"
                                  "Top-Right\0"
                                  "Bottom-Left\0"
                                  "Bottom-Right\0"
                                  "(Let Game Decide)\0\0" ))
        {
          SK_Steam_SetNotifyCorner ();
        }

        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("Applies Only to Traditional Overlay (not Big Picture)");

        ImGui::TreePop ();
      }




      bool app_has_cloud_storage =
        ( SK_SteamAPI_RemoteStorage () != nullptr ?
          ReadAcquire64 (&SK_SteamAPI_CallbackRunCount)             &&
          SK_SteamAPI_RemoteStorage ()->IsCloudEnabledForAccount () &&
          SK_SteamAPI_RemoteStorage ()->IsCloudEnabledForApp     () :
            false );

      struct sk_steam_cloud_entry_s {
        std::string filename;
        std::string detail;   // Temporary, formatted string.
        int32_t     size;     // Apparently 2 GiB is a hard limit, but we can
                              //   always use negative storage if we run out ;)
        int64_t     timestamp;
        bool        blacklisted;
      };

      static std::vector <sk_steam_cloud_entry_s> files;

      // Empty
      if ( files.size () == 1 && files [0].size == 0 )
        app_has_cloud_storage = false;

      else if (app_has_cloud_storage && files.empty ( ))
      {
        ISteamRemoteStorage* pRemote =
          SK_SteamAPI_RemoteStorage ();

        int32_t num_files =
          pRemote->GetFileCount ();

        for (int i = 0; i < num_files; i++)
        {
          sk_steam_cloud_entry_s file = { };

          file.filename  =
            pRemote->GetFileNameAndSize (i, &file.size);
          file.timestamp =
            pRemote->GetFileTimestamp     (file.filename.c_str ());

          file.blacklisted =
            (config.steam.cloud.blacklist.count (file.filename) != 0);

          files.emplace_back (file);
        }

        if (files.empty ())
          files.emplace_back (sk_steam_cloud_entry_s { "No Files", "", 0L, 0LL, false });
      }


      static uint64_t redist_size  = UINT64_MAX;
      static int      redist_count = -1;

      if (redist_count == -1 || redist_size == UINT64_MAX)
      {
        uint64_t finished_size =
          SK_Steam_ScrubRedistributables (redist_count, false);

        if (redist_count != -1)
          redist_size = finished_size;
      }

      if (redist_count > 0)
      {
        ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.68f, 0.02f, 0.45f));
        ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.72f, 0.07f, 0.80f));
        ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.78f, 0.14f, 0.80f));

        bool summarize = ImGui::CollapsingHeader ("Wasted Disk Space", ImGuiTreeNodeFlags_DefaultOpen);

        ImGui::PopStyleColor (3);

        if (summarize)
        {
          ImGui::TreePush ("");

          if (
            ImGui::MenuItem ( SK_FormatString (
                                "Steam is currently wasting %ws on %i redistributable files!",
                                  SK_File_SizeToStringF (redist_size, 5, 2).c_str (),
                                    redist_count
                              ).c_str (), "" )
             )
          {
            redist_size =
              SK_Steam_ScrubRedistributables (redist_count, true);
          }

          if (ImGui::IsItemHovered ())
            ImGui::SetTooltip ("Click here to free the wasted space.");

          ImGui::TreePop  ();
        }
      }


      if (ImGui::CollapsingHeader ("Screenshots"))
      {
        ImGui::TreePush ("");

        if (ImGui::Checkbox ("Enable Smart Capture Mode", &config.steam.screenshots.enable_hook))
        {
           SKX_Steam_ScreenshotMgr_Reinit ();
        }

        if (ImGui::IsItemHovered ())
        {
          ImGui::SetTooltip ( "In D3D11 games with typical framebuffer formats, this eliminates "
                              "hitching during screenshot capture." );
        }

        ImGui::TreePop ();
      }

      if (app_has_cloud_storage && ImGui::CollapsingHeader ("Cloud Storage"))
      {
        bool dirty = false;

        const  DWORD cycle_freq = 2250UL;
        static DWORD last_cycle = current_time;
        static int   detail_idx = 0;

        if (last_cycle < current_time - cycle_freq)
        {
          detail_idx = ~detail_idx;
          last_cycle =  current_time;
          dirty      =  true;
        }

        ImGui::BeginChild ( "CloudStorageList",
                              ImVec2 ( -1.0f, font.size_multiline * files.size () +
                                       0.1f * font.size_multiline ),
                                true,
                                  ImGuiWindowFlags_AlwaysAutoResize );
        for ( auto& it : files )
        {
          ImGui::PushID   (it.filename.c_str ());

          bool allow_sync = false;

          if (it.size != 0)
          {
            allow_sync =
              (! it.blacklisted);
          }

          if (dirty)
          {
            switch (detail_idx)
            {
              case 0:
                it.detail = SK_FormatString ("%i Bytes", it.size);
                break;
              default:
              {
                struct tm *t = _localtime64 (&it.timestamp);
                it.detail    =      asctime (  t);
              } break;
            }
          }

          if ( ImGui::MenuItem ( it.filename.c_str (),
                                 it.detail.c_str   (), &allow_sync ) )
          {
            if (        allow_sync  && config.steam.cloud.blacklist.count (it.filename) != 0)
            {
              config.steam.cloud.blacklist.erase (it.filename);
              it.blacklisted = false;
            }

            else if ((! allow_sync) && config.steam.cloud.blacklist.count (it.filename) == 0)
            {
              config.steam.cloud.blacklist.emplace (it.filename);
              it.blacklisted = true;
            }

            SK_SaveConfig ();
          }

          ImGui::PopID    ();
        }
        ImGui::EndChild   ();
      }




      if (SK_Denuvo_UsedByGame ())
      {
        ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.00f, 0.00f, 0.00f, 1.00f));
        ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.00f, 0.00f, 0.00f, 1.00f));
        ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.00f, 0.00f, 0.00f, 1.00f));
        ImGui::PushStyleColor (ImGuiCol_Text,          ImColor::HSV (0.15f, 1.0f, 1.0f));

        if (ImGui::CollapsingHeader ("Denuvo"))
        {
          ImGui::PopStyleColor (4);

          ImGui::TreePush ("");

          size_t idx = 0;

          ImGui::BeginGroup ();
          for ( auto it : denuvo_files )
          {
            size_t found =
              it.path.find_last_of (L'\\');

            ImGui::BeginGroup      ();
            ImGui::Text            ( "Key %lu:",
                                           idx++ );
            ImGui::TextUnformatted ( "First Activated:" );
            ImGui::EndGroup        ();

            ImGui::SameLine        ();

            ImGui::BeginGroup      ();
            ImGui::Text            ( "%ws", &it.path.c_str ()[found + 1] );

            if (denuvo_files.size () > idx)
            {
              ImGui::SameLine      ();
              ImGui::TextColored   (ImColor::HSV (0.08f, 1.f, 1.f), " [ Expired ]");
            }

            ImGui::Text            ( "%02d/%02d/%d  %02d:%02d",
                                       it.st_local.wMonth, it.st_local.wDay, it.st_local.wYear,
                                       it.st_local.wHour,  it.st_local.wMinute );

            ImGui::EndGroup        ();
          }
          ImGui::EndGroup          ();

          ImGui::SameLine          ();

          ImGui::BeginGroup        ();
          for ( auto it : denuvo_files )
          {
            size_t found =
              it.path.find_last_of (L'\\');

            ImGui::PushID (_wtol (&it.path.c_str ()[found + 1]));

            if (ImGui::Button      ("  Delete Me  "))
            {
              DeleteFileW (it.path.c_str ());

              SK_Denuvo_UsedByGame (true); // Re-Test
            }

            if (ImGui::IsItemHovered ())
            {
              ImGui::BeginTooltip  ();
              ImGui::Text          ("Force Denuvo to Re-Activate the next time the Game Starts");
              ImGui::Separator     ();
              ImGui::BulletText    ("Useful if you plan to go offline for an extended period.");
              ImGui::BulletText    (" >> RESTART the game immediately after doing this to re-activate <<");
              ImGui::EndTooltip    ();
            }

            ImGui::TextUnformatted ( "" );

            ImGui::PopID           ();
          }
          ImGui::EndGroup          ();

          ImGui::TreePop  ();
        }

        else
        {
          ImGui::PopStyleColor   (4);
        }
      }

      ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.40f, 0.40f, 0.45f));
      ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.45f, 0.45f, 0.80f));
      ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.53f, 0.53f, 0.80f));

      if (ImGui::CollapsingHeader ("Compatibility"))
      {
        ImGui::TreePush ("");
        ImGui::Checkbox (" Bypass Online DRM Checks  ",          &config.steam.spoof_BLoggedOn);

        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip ();
          ImGui::TextColored  (ImColor::HSV (0.159f, 1.0f, 1.0f), "Fixes pesky games that use SteamAPI to deny Offline mode");
          ImGui::Separator    ();
          ImGui::BulletText   ("This is a much larger problem than you would believe.");
          ImGui::BulletText   ("This also fixes some games that crash when Steam disconnects (unrelated to DRM).");
          ImGui::EndTooltip   ();
        }

        ImGui::Checkbox (" Load Steam Overlay Early  ",          &config.steam.preload_overlay);

        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("Can make the Steam Overlay work in situations it otherwise would not.");

        ImGui::SameLine ();

        ImGui::Checkbox (" Load Steam Client DLL Early  ",       &config.steam.preload_client);

        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("May prevent some Steam DRM-based games from hanging at startup.");

        ImGui::Checkbox (" Disable User Stats Receipt Callback", &config.steam.block_stat_callback);

        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip ();
          ImGui::Text         ("Fix for Games that Panic when Flooded with Achievement Data");
          ImGui::Separator    ();
          ImGui::BulletText   ("These Games may shutdown SteamAPI when Special K fetches Friend Achievements");
          ImGui::BulletText   ("If SteamAPI Frame Counter is STUCK, turn this option ON and restart the Game");
          ImGui::EndTooltip   ();
        }

        ImGui::TreePop  ();
      }

      ImGui::PopStyleColor (3);

      bool valid = (! config.steam.silent);

      valid = valid && (! SK_Steam_PiratesAhoy ());

      if (valid)
      {
        bool publisher_is_stupid = false;

        if (config.steam.spoof_BLoggedOn)
        {
          auto status =
            static_cast <int> (SK_SteamUser_BLoggedOn ());

          if (status & static_cast <int> (SK_SteamUser_LoggedOn_e::Spoofing))
          {
            publisher_is_stupid = true;

            ImGui::PushStyleColor (ImGuiCol_TextDisabled,  ImColor::HSV (0.074f, 1.f, 1.f));
            ImGui::MenuItem       ("This game's publisher may consider you a pirate! :(", "", nullptr, false);
            ImGui::PopStyleColor  ();
          }
        }

        if (! publisher_is_stupid)
          ImGui::MenuItem ("I am not a pirate!", "", &valid, false);
      }

      else
      {
        ImGui::MenuItem (u8"I am probably a pirate moron™", "", &valid, false);
        {
          // Delete the CPY config, and push user back onto legitimate install,
          //   to prevent repeated pirate detection.
          //
          //  If stupid user presses this button for any other reason, that is
          //    their own problem. Crackers should make a better attempt to
          //      randomize their easily detectable hackjobs.
          if (GetFileAttributes (L"CPY.ini") != INVALID_FILE_ATTRIBUTES)
          {
            DeleteFileW (L"CPY.ini");
            
            MoveFileW   (L"steam_api64.dll",   L"CPY.ini");
            MoveFileW   (L"steamclient64.dll", L"steam_api64.dll");
            MoveFileW   (L"CPY.ini",           L"steamclient64.dll");
          }
        }
      }

      ImGui::PopStyleColor (3);
      ImGui::TreePop       ( );
    }

    ImGui::Columns    ( 1 );
    ImGui::Separator  (   );

    if (SK::SteamAPI::GetNumPlayers () > 1)
    {
      ImGui::Columns    ( 2, "SteamSep", true );

      static char szNumber       [16] = { };
      static char szPrettyNumber [32] = { };

      const NUMBERFMTA fmt = { 0, 0, 3, ".", ",", 0 };

      snprintf (szNumber, 15, "%li", SK::SteamAPI::GetNumPlayers ());

      GetNumberFormatA ( MAKELCID (LOCALE_USER_DEFAULT, SORT_DEFAULT),
                           0x00,
                             szNumber, &fmt,
                               szPrettyNumber, 32 );

      ImGui::Text       (" %s Players in-Game on Steam  ", szPrettyNumber);
      ImGui::NextColumn (   );
    }

    ImGui::Bullet     ();   ImGui::SameLine ();

    bool pause =
      SK_SteamAPI_GetOverlayState (false);

    if (ImGui::Selectable ( "SteamAPI Frame", &pause, SK::SteamAPI::IsOverlayAware () ? 0 : ImGuiSelectableFlags_Disabled) &&
                                                      SK::SteamAPI::IsOverlayAware ())
    {
      SK_SteamAPI_SetOverlayState (pause);
    }

    auto SteamCallbackThrottleSubMenu = [&](void) ->
    void
    {
      int rate = ReadAcquire (&SK_SteamAPI_CallbackRateLimit);
      
      if (ImGui::SliderInt ("Limit SteamAPI Rate", &rate, -1, 240))
          InterlockedExchange (&SK_SteamAPI_CallbackRateLimit, rate);
    };


    bool right_clicked = SK_ImGui_IsItemRightClicked ();

    if (SK::SteamAPI::IsOverlayAware ())
    {
      if (right_clicked)
      {
        ImGui::OpenPopup         ("SteamOverlayPauseMenu");
        ImGui::SetNextWindowSize (ImVec2 (-1.0f, -1.0f), ImGuiSetCond_Always);
      }

      else if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip   (       );
        ImGui::Text           ( "In"  );                 ImGui::SameLine ();
        ImGui::PushStyleColor ( ImGuiCol_Text, ImColor (0.95f, 0.75f, 0.25f, 1.0f) ); 
        ImGui::Text           ( "Steam Overlay Aware");  ImGui::SameLine ();
        ImGui::PopStyleColor  (       );
        ImGui::Text           ( "software, click to toggle the game's overlay pause mode." );
        ImGui::EndTooltip     (       );
      }

      if (ImGui::BeginPopup ("SteamOverlayPauseMenu"))
      {                     
        if (ImGui::Checkbox ("Pause Game while Control Panel is Visible",
            &config.steam.reuse_overlay_pause))
        {
          SK::SteamAPI::SetOverlayState (config.steam.reuse_overlay_pause);
        }

        SteamCallbackThrottleSubMenu ();

        ImGui::EndPopup     ();
      }
    }

    else if (right_clicked)
    {
      ImGui::OpenPopup         ("SteamCallbackRateMenu");
      ImGui::SetNextWindowSize (ImVec2 (-1.0f, -1.0f), ImGuiSetCond_Always);
    }

    if (ImGui::BeginPopup ("SteamCallbackRateMenu"))
    {                     
      SteamCallbackThrottleSubMenu ();

      ImGui::EndPopup     ();
    }

    ImGui::SameLine ();
    ImGui::Text     ( ": %10llu  ", ReadAcquire64 (&SK_SteamAPI_CallbackRunCount) );
    ImGui::Columns(1, nullptr, false);

    // If fetching stats and online...
    if ( (! SK_SteamAPI_FriendStatsFinished  ())        &&
            SK_SteamAPI_GetNumPlayers        ()  > 1    && 
            SK_SteamAPI_FriendStatPercentage () != 0.0f && 
            config.steam.achievements.pull_friend_stats )
    {
      float ratio   = SK_SteamAPI_FriendStatPercentage ();
      int   friends = SK_SteamAPI_GetNumFriends        ();

      static char szLabel [512] = { };

      snprintf ( szLabel, 511,
                   "Fetching Achievements... %.2f%% (%u/%u) : %s",
                     100.0f * ratio,
static_cast <uint32_t> (
                      ratio * static_cast <float>    (friends)
                     ),
                              static_cast <uint32_t> (friends),
                    SK_SteamAPI_GetFriendName (
static_cast <uint32_t> (
                      ratio * static_cast <float>    (friends))
                    )
               );
      
      ImGui::PushStyleColor ( ImGuiCol_PlotHistogram, ImColor (0.90f, 0.72f, 0.07f, 0.80f) ); 
      ImGui::PushStyleColor ( ImGuiCol_Text,          ImColor (255, 255, 255)              ); 
      ImGui::ProgressBar (
        SK_SteamAPI_FriendStatPercentage (),
          ImVec2 (-1, 0), szLabel );
      ImGui::PopStyleColor  (2);
    }

    return true;
  }

  return false;
}

bool
SK::ControlPanel::Steam::DrawMenu (void)
{
  if (SK::SteamAPI::AppID () != 0 && steam_ctx.UserEx () != nullptr)
  {
    if (ImGui::BeginMenu ("Steam"))
    {
      auto* user_ex =
        steam_ctx.UserEx ();

      if (! user_ex->BConnected ())
      {
        if (ImGui::MenuItem ("Connect to Steam"))
        {
          SK_Steam_ConnectUserIfNeeded (user_ex->GetSteamID ());
        }
      }

      else
      {
        auto logon_state = user_ex->GetLogonState ();

        static DWORD dwStartLogOffTime = SK::ControlPanel::current_time;
        static DWORD dwStartLogOnTime  = SK::ControlPanel::current_time;

        switch (logon_state)
        {
          case SK::SteamAPI::k_ELogonStateNotLoggedOn:
          case SK::SteamAPI::k_ELogonStateLoggingOff:
          {
            if (ImGui::MenuItem ("Log On"))
            {
              dwStartLogOnTime = SK::ControlPanel::current_time;
              user_ex->LogOn (user_ex->GetSteamID ());
            }

            if (logon_state == SK::SteamAPI::k_ELogonStateLoggingOff)
            {
              ImGui::Separator  (  );
              ImGui::TreePush   ("");
              ImGui::BulletText ("Logging off... (%3.1f seconds)",
                double(SK::ControlPanel::current_time - dwStartLogOffTime)
                     / 1000.0);
              ImGui::TreePop    (  );
            }
          } break;

          case SK::SteamAPI::k_ELogonStateLoggedOn:
          case SK::SteamAPI::k_ELogonStateLoggingOn:
          {
            if (ImGui::MenuItem ("Log Off"))
            {
              dwStartLogOffTime = SK::ControlPanel::current_time;
              user_ex->LogOff ();
            }

            if (logon_state == SK::SteamAPI::k_ELogonStateLoggingOn)
            {
              ImGui::Separator  (  );
              ImGui::TreePush   ("");
              ImGui::BulletText ("Logging on... (%3.1f seconds)",
                double(SK::ControlPanel::current_time - dwStartLogOnTime)
                     / 1000.0);
              ImGui::TreePop    (  );
            }

            else
            {
              ImGui::Separator ();

              int state =
                SK::SteamAPI::GetPersonaState ();

              bool always_anti_social =
                config.steam.online_status != -1;

              if (ImGui::Checkbox ("Always Appear", &always_anti_social))
              {
                if (always_anti_social) config.steam.online_status = state;
                else                    config.steam.online_status =    -1;
              }

              ImGui::SameLine ();

              // Range restrict -- there are online states that are meaningless while in-game,
              //                     we want to hide these, but not force them on/off.
              state = std::min (2, std::max (0, state));

              if ( ImGui::Combo ( "###Steam_Social_Status",
                                    &state,
                                      "Offline\0"
                                      "Online\0"
                                      "Busy\0\0",
                                        3 ) )
              {
                if (always_anti_social)
                  config.steam.online_status = state;

                SK::SteamAPI::SetPersonaState ((EPersonaState)state);
              }
            }
          } break;
        }
      }

      ImGui::EndMenu ();

      return  true;
    }
  }

  return false;
}