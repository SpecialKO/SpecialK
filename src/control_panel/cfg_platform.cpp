﻿// This is an open source non-commercial project. Dear PVS-Studio, please check it.
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


#include <SpecialK/control_panel/platform.h>
#include <SpecialK/control_panel/steam.h>
#include <SpecialK/control_panel/epic.h>
#include <SpecialK/control_panel/galaxy.h>

#include <SpecialK/storefront/epic.h>
#include <SpecialK/storefront/gog.h>
#include <imgui/imfilebrowser.h>


using namespace SK::ControlPanel;

bool
SK::ControlPanel::Platform::Draw (void)
{
  const ImGuiIO& io =
    ImGui::GetIO ();

  if (SK::SteamAPI::AppID () != 0 || SK::EOS::GetTicksRetired () > 0 || SK::Galaxy::GetTicksRetired () > 0)
  {
    // Steam AppID never changes, but EOS tick count might...
    //   also some Steam games use EOS but aren't themselves an Epic game
    static bool bSteam =  (SK::SteamAPI::AppID         () != 0);
           bool bEpic  = ((SK::EOS::GetTicksRetired    () >  0) && (! bSteam));
           bool bGOG   = ((SK::Galaxy::GetTicksRetired () >  0) && (! bSteam) && (! bEpic));

    static const std::string header_label =
      SK_FormatString ( "%s Enhancements###Platform_Enhancements",
                        bSteam ? "Steam" :
                         bEpic ? "Epic"  :
                          bGOG ? "GOG"   :
                             "Platform" );

    if ( ImGui::CollapsingHeader (header_label.c_str (), ImGuiTreeNodeFlags_CollapsingHeader |
                                                         ImGuiTreeNodeFlags_DefaultOpen ) )
    {
      ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.02f, 0.68f, 0.90f, 0.45f));
      ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.07f, 0.72f, 0.90f, 0.80f));
      ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.14f, 0.78f, 0.87f, 0.80f));
      ImGui::TreePush       ("");

      static bool bHasAchievements = false;

      SK_RunOnce (
      {
        // Will test whether SteamAPI is going to throw an exception,
        //   we need to not make these API calls if it does or NVIDIA
        //     Streamline will cause games to crash!
        SK_SteamAPI_GetNumPlayers ();

        bool bSteamWorks =
          (! config.platform.steam_is_b0rked);

        bHasAchievements =
            ( bSteamWorks && SK_SteamAPI_GetNumPossibleAchievements () > 0 ) ||
            ( bEpic       &&      SK_EOS_GetNumPossibleAchievements () > 0 ) ||
            ( bGOG        &&   SK_Galaxy_GetNumPossibleAchievements () > 0 );
      });


      if (bHasAchievements)
      {
        static char szProgress [128] = { };

        const float  ratio            = bEpic ? SK::EOS::PercentOfAchievementsUnlocked      () :
                                        bGOG  ? SK::Galaxy::PercentOfAchievementsUnlocked   ()
                                              : SK::SteamAPI::PercentOfAchievementsUnlocked ();

        const int    unlock_count     = bEpic ? SK::EOS::NumberOfAchievementsUnlocked      () :
                                        bGOG  ? SK::Galaxy::NumberOfAchievementsUnlocked   ()
                                              : SK::SteamAPI::NumberOfAchievementsUnlocked ();

        const size_t num_achievements = bEpic ? SK_EOS_GetNumPossibleAchievements           () :
                                        bGOG  ? SK_Galaxy_GetNumPossibleAchievements        ()
                                              : SK_SteamAPI_GetNumPossibleAchievements      ();

        snprintf ( szProgress, 127, "%.2f%% of Achievements Unlocked (%u/%u)",
                     100.0 * ratio,  sk::narrow_cast <uint32_t> (unlock_count),
                                     sk::narrow_cast <uint32_t> (num_achievements) );

        ImGui::PushStyleColor ( ImGuiCol_PlotHistogram, ImVec4 (0.90f, 0.72f, 0.07f, 0.80f) );
        ImGui::ProgressBar    ( ratio,
                                  ImVec2 (-1, 0),
                                    szProgress );
        ImGui::PopStyleColor  ();

        const int friends =
          SK_SteamAPI_GetNumFriends ();

        if (friends > 0 && ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip   ();

          const auto max_lines =
            static_cast <int> ((io.DisplaySize.y * 0.725f) / (font.size_multiline * 0.9f));
          int  cur_line    = 0;

          ImGui::BeginGroup     ();

          ImGui::PushStyleColor ( ImGuiCol_Text,          ImVec4 (1.f, 1.f, 1.f, 1.f)         );
          ImGui::PushStyleColor ( ImGuiCol_PlotHistogram, ImVec4 (0.90f, 0.72f, 0.07f, 0.80f) );

          for (uint32_t i = 0; i < (uint32_t)((float)friends * SK_SteamAPI_FriendStatPercentage ()); i++)
          {
            const float percent =
              SK_SteamAPI_GetUnlockedPercentForFriend (i);

            if (percent > 0.0f)
            {
              size_t            len   = 0;
              const std::string name  = SK_SteamAPI_GetFriendName (i, &len);

              ImGui::ProgressBar     ( percent, ImVec2 (io.DisplaySize.x * 0.0816f, 0.0f) );
              ImGui::SameLine        ( );
              ImGui::PushStyleColor  (ImGuiCol_Text, ImVec4 (.81f, 0.81f, 0.81f, 1.f));
              ImGui::TextUnformatted (name.c_str ());
              ImGui::PopStyleColor   (1);

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
            bEpic ? SK_EOS_UnlockAchievement    (0) :
            bGOG  ? SK_Galaxy_UnlockAchievement (0) :
                    SK_Steam_UnlockAchievement  (0);

          if (ImGui::IsItemHovered ())
            ImGui::SetTooltip ("Perform a FAKE unlock so that you can tune your preferences.");

          ImGui::SameLine ();

          ImGui::Checkbox (ICON_FA_MUSIC " Play Sound ", &config.platform.achievements.play_sound);

          if (config.platform.achievements.play_sound)
          {
            ImGui::SameLine ();

            static constexpr auto _Picking = 4;
            static constexpr auto _Custom  = 3;

            static SKTL_BidirectionalHashMap <std::wstring, int> sound_map
            {  { L"psn", 0 }, { L"xbox", 1 }, { L"dream_theater", 2 }  };

            static int i = 0;

            const auto& it =
              sound_map.find (config.platform.achievements.sound_file);

            if (it != sound_map.cend ())
            {
              if (! config.platform.achievements.sound_file.empty ())
                i = it->second;
            }

            else
            {
              if (i != _Picking)
                  i  = _Custom;
            }

            static ImGui::FileBrowser
                          fileDialog (ImGuiFileBrowserFlags_CloseOnEsc);

            auto SK_Platform_LoadUnlockSound = [](void)
            {
              if (SK::EOS::UserID () != nullptr)
                   SK_EOS_LoadUnlockSound    (config.platform.achievements.sound_file.c_str ());
              else if (SK::Galaxy::GetTicksRetired () > 0)
                   SK_Galaxy_LoadUnlockSound (config.platform.achievements.sound_file.c_str ());
              else SK_Steam_LoadUnlockSound  (config.platform.achievements.sound_file.c_str ());
            };

            if (ImGui::Combo ("###AchievementSound", &i, "PlayStation Network\0"
                                                                   "Xbox Live\0"
                                                               "Dream Theater\0"
                                                  "Custom\t[Refer to Tooltip]\0"
                                                            "Pick a .wav File\0\0", 5))
            {
              if (i <= 2)
              {
                config.platform.achievements.sound_file.assign (
                  sound_map [i]
                );

                SK_Platform_LoadUnlockSound ();
              }

              if (i == _Picking)
              {
                fileDialog.SetTitle       ("Pick a .wav File");
                fileDialog.SetTypeFilters (     { ".wav" }   );
                fileDialog.Open           (                  );
              }
            }

            if (i == _Custom)
            {
              ImGui::SetItemTooltip (
                SK_WideCharToUTF8 (config.platform.achievements.sound_file).c_str ()
              );
            }

            fileDialog.Display ();

            if (fileDialog.HasSelected ())
            {
              if (PathFileExistsW (fileDialog.GetSelected ().wstring ().c_str ()))
              {
                config.platform.achievements.sound_file.assign (
                  fileDialog.GetSelected ().wstring ()
                );

                SK_SaveConfig ();

                SK_Platform_LoadUnlockSound ();

                if (SK::EOS::UserID () != nullptr)
                     SK_EOS_PlayUnlockSound    ();
                else if (SK::Galaxy::GetTicksRetired () > 0)
                     SK_Galaxy_PlayUnlockSound ();
                else SK_Steam_PlayUnlockSound  ();

                i = _Custom;
              }

              fileDialog.ClearSelected ();
            }
          }

          ImGui::EndGroup ();
          ImGui::SameLine ();

          ImGui::Checkbox (ICON_FA_CAMERA " Take Screenshot", &config.platform.achievements.take_screenshot);

          ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.68f, 0.02f, 0.45f));
          ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.72f, 0.07f, 0.80f));
          ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.78f, 0.14f, 0.80f));

          const bool uncollapsed =
            ImGui::CollapsingHeader ("Enhanced Popup", ImGuiTreeNodeFlags_AllowOverlap);

          if (bSteam) {
            ImGui::SameLine (); ImGui::Checkbox        ("   Fetch Friend Unlock Stats", &config.platform.achievements.pull_friend_stats);
          }

          if (uncollapsed)
          {
            ImGui::TreePush ("");

            int  mode    = (config.platform.achievements.popup.show + config.platform.achievements.popup.animate);
            bool changed = false;

            ImGui::TextUnformatted
                               ("Draw Mode:");
            ImGui::SameLine    (            );

            // Animated popups are not supported in ImGui
            //
            if (mode > 1) mode = 1;

            changed |= ImGui::RadioButton ("Disabled ##AchievementPopup",   &mode, 0); ImGui::SameLine ();
            changed |= ImGui::RadioButton ("Stationary ##AchievementPopup", &mode, 1); ImGui::SameLine ();

            //changed |=
            //  ImGui::RadioButton ("Animated ##AchievementPopup",   &mode, 2);

            ImGui::SameLine    ( );
            ImGui::BeginGroup  ( );
            changed |=
            ImGui::Combo       ( "Location##PopupLoc", &config.platform.achievements.popup.origin,
                                         "Top-Left\0"
                                         "Top-Right\0"
                                         "Bottom-Left\0"
                                         "Bottom-Right\0\0" );

            if ( changed )
            {
              config.platform.achievements.popup.show    = (mode > 0);
              config.platform.achievements.popup.animate = (mode > 1);

              // Make sure the duration gets set non-zero when this changes
              if (config.platform.achievements.popup.show)
              {
                if ( config.platform.achievements.popup.duration == 0 )
                  config.platform.achievements.popup.duration = 6666UL;
              }
            }

            if (config.platform.achievements.popup.show)
            {
              float duration =
                std::max ( 1.0f, ( (float)config.platform.achievements.popup.duration / 1000.0f ) );

              if ( ImGui::SliderFloat ( "Duration##PopupDuration", &duration, 1.0f, 30.0f, "%.2f Seconds" ) )
              {
                changed = true;
                config.platform.achievements.popup.duration =
                  static_cast <LONG> ( duration * 1000.0f );
              }

              changed |=
                ImGui::SliderInt ("Maximum On Screen##PopupMaxOnScreen", &config.platform.achievements.popup.max_on_screen, 1, 10);
              changed |=
                ImGui::SliderInt ("Maximum Columns##PopupMaxColumns",    &config.platform.achievements.popup.max_columns,   1,  3);

              ImGui::SetItemTooltip (
                "If there are too many achievement popups to fit on screen, overflow will create new columns..."
              );
            }
            ImGui::EndGroup    ( );

            //ImGui::SliderFloat ("Inset Percentage",    &config.platform.achievements.popup.inset, 0.0f, 1.0f, "%.3f%%", 0.01f);
            ImGui::TreePop     ( );

            config.utility.save_async_if (changed);
          }

          void SK_ImGui_DrawAchievementList (void);
               SK_ImGui_DrawAchievementList ();

          ImGui::TreePop       ( );
          ImGui::PopStyleColor (3);
        }
      }

      else if (! config.platform.steam_is_b0rked)
      {
        // Handle late init situations
        bHasAchievements =
         ( ( bSteam && SK_SteamAPI_GetNumPossibleAchievements () > 0 )
        || ( bEpic  &&      SK_EOS_GetNumPossibleAchievements () > 0 )
        || ( bGOG   &&   SK_Galaxy_GetNumPossibleAchievements () > 0 ) );
      }

      static bool bSteamOverlayEnabled = config.platform.steam_is_b0rked == false &&
                                         steam_ctx.Utils () != nullptr            &&
                                         steam_ctx.Utils ()->IsOverlayEnabled ();

      bool bOverlayEnabled =
        ( bSteamOverlayEnabled || SK::EOS::GetTicksRetired () > 0 );

      if (bOverlayEnabled && ImGui::CollapsingHeader ("Overlay Notifications"))
      {
        ImGui::TreePush  ("");

        if (ImGui::Combo ( " ", &config.platform.notify_corner,
                                  "Top-Left\0"
                                  "Top-Right\0"
                                  "Bottom-Left\0"
                                  "Bottom-Right\0"
                                  "(Let Game Decide)\0\0" ))
        {
          SK_Platform_SetNotifyCorner ();
        }

        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("Applies Only to Traditional Overlay (not Big Picture)");

        ImGui::TreePop ();
      }

      SK::ControlPanel::Steam::Draw  ();
      SK::ControlPanel::Epic::Draw   ();
      SK::ControlPanel::Galaxy::Draw ();

      ImGui::TreePop       ( );
      ImGui::PopStyleColor (3);
    }

    SK::ControlPanel::Steam::DrawFooter  ();
    SK::ControlPanel::Epic::DrawFooter   ();
    SK::ControlPanel::Galaxy::DrawFooter ();

    return true;
  }

  else
  {
    auto num_players =
      SK_Platform_GetNumPlayers ();

    if (num_players > 1)
    {
      ImGui::Separator ();

      static char szNumber       [16] = { };
      static char szPrettyNumber [32] = { };

      const NUMBERFMTA fmt = { 0, 0, 3, (char *)".", (char *)",", 0 };

      snprintf (szNumber, 15, "%i", num_players);

      GetNumberFormatA ( MAKELCID (LOCALE_USER_DEFAULT, SORT_DEFAULT),
                           0x00,
                             szNumber, &fmt,
                               szPrettyNumber, 32 );

      ImGui::Text       (" %s Players in-Game  ", szPrettyNumber);
    }
  }

  return false;
}

bool
SK::ControlPanel::Platform::DrawMenu (void)
{
  SK::ControlPanel::Steam::DrawMenu  ();
  SK::ControlPanel::Epic::DrawMenu   ();
  SK::ControlPanel::Galaxy::DrawMenu ();

  return false;
}

bool
SK::ControlPanel::Platform::WarnIfUnsupported (void)
{
  return
    SK::ControlPanel::Steam::WarnIfUnsupported  () ||
    SK::ControlPanel::Epic::WarnIfUnsupported   () ||
    SK::ControlPanel::Galaxy::WarnIfUnsupported ();
}