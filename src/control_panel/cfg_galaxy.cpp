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

#include <SpecialK/control_panel/galaxy.h>
#include <SpecialK/storefront/gog.h>
#include <SpecialK/control_panel/platform.h>

#include <imgui/font_awesome.h>

using namespace SK::ControlPanel;

bool
SK::ControlPanel::Galaxy::Draw (void)
{
  if (SK::Galaxy::GetTicksRetired () > 0)
  {
    static bool restart_required = false;
    bool        bDefaultOpen     = false;

    // Uncollapse this header by default if the user is forcing the overlay off; make it obvious!
    if ( config.steam.disable_overlay ||
         config.platform.silent ) bDefaultOpen = true;

    if (ImGui::CollapsingHeader ("Compatibility", bDefaultOpen ? ImGuiTreeNodeFlags_DefaultOpen
                                                               : 0x0))
    {
      ImGui::TreePush   ("");
      ImGui::BeginGroup (  );

      if (ImGui::Checkbox (" Disable Galaxy Integration", &config.platform.silent))
      {
        restart_required = true;
        config.utility.save_async_if (restart_required);
      }

      ImGui::SetItemTooltip ("Turns off almost all Galaxy-related features");

      if (ImGui::Checkbox (" Prevent Overlay From Drawing  ",    &config.steam.disable_overlay))
      {
        restart_required = true;
        config.utility.save_async_if (restart_required);
      }
      ImGui::EndGroup   (  );

      if (! config.platform.silent)
      {
        ImGui::SameLine    ();
        ImGui::SeparatorEx (ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine    ();
        ImGui::BeginGroup  ();
        if (ImGui::Checkbox(" Always Enable Galaxy Services", &config.galaxy.spawn_mini_client))
        {
          restart_required = true;
          config.utility.save_async_if (restart_required);
        }
        ImGui::SetItemTooltip
                           ( "Allows Achievements and Multiplayer Matchmaking "
                             "even when the Galaxy client is not running." );
        if (config.galaxy.spawn_mini_client)
        {
          if (ImGui::Checkbox ("Require Online Galaxy Functionality", &config.galaxy.require_online_mode))
          {
            restart_required = true;
            config.utility.save_async_if (restart_required);
          }
          ImGui::SetItemTooltip (
            "Enable this if your game needs to connect to the Internet for some features."
          );
        }
        ImGui::EndGroup    ();
      }

      if (restart_required)
      {
        ImGui::SameLine       ();
        ImGui::SeparatorEx    (ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine       ();
        ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (.15f, .8f, .9f).Value);
        ImGui::BulletText     ("Game Restart Required");
        ImGui::PopStyleColor  ();
      }
  
      ImGui::TreePop ();
    }
    return true;
  }

  return false;
}

bool
SK::ControlPanel::Galaxy::DrawFooter (void)
{
  if (SK::Galaxy::GetTicksRetired () > 0)
  {
    ImGui::BeginGroup (   );
    ImGui::Separator  (   );

    const auto num_players =
      SK_Platform_GetNumPlayers ();

    if (num_players > 1)
    { 
      static char szNumber       [16] = { };
      static char szPrettyNumber [32] = { };
    
      static const NUMBERFMTA fmt = { 0, 0, 3, (char *)".", (char *)",", 0 };
    
      snprintf (szNumber, 15, "%i", num_players);
    
      GetNumberFormatA ( MAKELCID (LOCALE_USER_DEFAULT, SORT_DEFAULT),
                           0x00,
                             szNumber, &fmt,
                               szPrettyNumber, 32 );
    
      ImGui::Text       (" %s Players in-Game  ", szPrettyNumber);

      // Track cumulative total time played in the status bar.
      uint32_t
          minutes = SK::Galaxy::MinutesPlayed ();
      if (minutes > 0)
      {
        ImGui::SameLine    ();
        ImGui::SameLine    ();
        ImGui::SeparatorEx (ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine    ();
        ImGui::SameLine    ();
        ImGui::Text        ("  Your Hours Played :\t %4.1f ",
          static_cast <float> (SK::Galaxy::MinutesPlayed ()) / 60.0f
                            );
      }
    }

    ImGui::SameLine    ();
    ImGui::SameLine    ();
    ImGui::SeparatorEx (ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine    ();
    ImGui::SameLine    ();

    static bool has_winhttp = false;

    ImGui::BeginGroup      ();
    ImGui::TextUnformatted ("  Galaxy Services:  ");
    ImGui::SameLine        ();
    if (! SK::Galaxy::IsSignedIn ())
    {
      SK_RunOnce (
        has_winhttp = PathFileExistsW (L"winhttp.dll");
      );

      if (has_winhttp)
      {
        ImGui::TextColored (
          ImVec4 (1.f, 1.f, 0.f, 1.f), ICON_FA_EXCLAMATION_TRIANGLE
        );
        ImGui::SameLine ();
      }
    }
    ImGui::PushStyleColor  (ImGuiCol_Text, SK::Galaxy::IsSignedIn () ? ImVec4 (0.f, 1.f, 0.f, 1.f)
                                                                     : ImVec4 (1.f, 0.f, 0.f, 1.f));
    ImGui::TextUnformatted ( SK::Galaxy::IsSignedIn () ? "Signed In"
                                                       : "Not Signed In" );
    ImGui::SameLine        ();
    ImGui::PushStyleColor  (ImGuiCol_Text, SK::Galaxy::IsLoggedOn () ? ImVec4 (1.f, 1.f, 1.f, 1.f)
                                                                     : ImVec4 (.5f, .5f, .5f, 1.f));
    ImGui::TextUnformatted ( SK::Galaxy::IsLoggedOn () ? " (Online)"
                                                       : " (Offline)" );
    ImGui::PopStyleColor   (2);
    ImGui::EndGroup        ( );

    if (has_winhttp)
    {
      ImGui::SetItemTooltip (
        "winhttp.dll in the game's install directory is preventing GOG Galaxy from working!"
      );
    }

    ImGui::SameLine        ( );
    ImGui::Bullet          ( );
    ImGui::SameLine        ( );

    bool pause =
      SK_Platform_GetOverlayState (false);

    if ( ImGui::Selectable ( " Tick", &pause) &&
                             SK_Platform_IsOverlayAware () )
    {
      SK_Platform_SetOverlayState (pause);
    }

    const bool right_clicked =
      SK_ImGui_IsItemRightClicked ();

    if (SK_Platform_IsOverlayAware ())
    {
      if (right_clicked)
      {
        ImGui::OpenPopup         ("GalaxyOverlayPauseMenu");
        ImGui::SetNextWindowSize (ImVec2 (-1.0f, -1.0f), ImGuiCond_Always);
      }

      else if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip   (       );
        ImGui::Text           ( "In"  );                  ImGui::SameLine ();
        ImGui::PushStyleColor ( ImGuiCol_Text, ImVec4 (0.95f, 0.75f, 0.25f, 1.0f) );
        ImGui::Text           ( "Galaxy Overlay Aware");  ImGui::SameLine ();
        ImGui::PopStyleColor  (       );
        ImGui::Text           ( "software, click to toggle the game's overlay pause mode." );
        ImGui::EndTooltip     (       );
      }

      if (ImGui::BeginPopup ("GalaxyOverlayPauseMenu"))
      {
        if (ImGui::Checkbox ("Pause Game while Control Panel is Visible",
            &config.platform.reuse_overlay_pause))
        {
          SK_Platform_SetOverlayState (config.platform.reuse_overlay_pause);
        }

        ImGui::EndPopup     ();
      }
    }

    ImGui::SameLine ();
    ImGui::Text     ( ": %10llu  ", SK::Galaxy::GetTicksRetired () );
    ImGui::EndGroup ();

    return true;
  }

  return false;
}

bool
SK::ControlPanel::Galaxy::DrawMenu (void)
{
  return false;
}


bool
SK::ControlPanel::Galaxy::WarnIfUnsupported (void)
{
  return false;
}