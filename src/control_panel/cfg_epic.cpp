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

#include <SpecialK/control_panel/epic.h>
#include <SpecialK/storefront/epic.h>
#include <SpecialK/control_panel/platform.h>

using namespace SK::ControlPanel;

bool
SK::ControlPanel::Epic::Draw (void)
{
  if (SK::EOS::GetTicksRetired () > 0)
  {
    return true;
  }

  return false;
}

bool
SK::ControlPanel::Epic::DrawFooter (void)
{
  if (SK::EOS::GetTicksRetired () > 0)
  {
    ImGui::Columns    ( 1 );
    ImGui::Separator  (   );

    //if (SK::SteamAPI::GetNumPlayers () > 1)
    //{
    //  ImGui::Columns    ( 2, "EpicSep", true );
    //
    //  static char szNumber       [16] = { };
    //  static char szPrettyNumber [32] = { };
    //
    //  const NUMBERFMTA fmt = { 0, 0, 3, (char *)".", (char *)",", 0 };
    //
    //  snprintf (szNumber, 15, "%i", SK::SteamAPI::GetNumPlayers ());
    //
    //  GetNumberFormatA ( MAKELCID (LOCALE_USER_DEFAULT, SORT_DEFAULT),
    //                       0x00,
    //                         szNumber, &fmt,
    //                           szPrettyNumber, 32 );
    //
    //  ImGui::Text       (" %s Players in-Game on Epic  ", szPrettyNumber);
    //  ImGui::NextColumn (   );
    //}

    ImGui::Bullet     ();   ImGui::SameLine ();

    bool pause =
      SK_Platform_GetOverlayState (false);

    if ( ImGui::Selectable ( "Epic Online Services Tick", &pause) &&
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
        ImGui::OpenPopup         ("EpicOverlayPauseMenu");
        ImGui::SetNextWindowSize (ImVec2 (-1.0f, -1.0f), ImGuiCond_Always);
      }

      else if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip   (       );
        ImGui::Text           ( "In"  );                ImGui::SameLine ();
        ImGui::PushStyleColor ( ImGuiCol_Text, ImVec4 (0.95f, 0.75f, 0.25f, 1.0f) );
        ImGui::Text           ( "Epic Overlay Aware");  ImGui::SameLine ();
        ImGui::PopStyleColor  (       );
        ImGui::Text           ( "software, click to toggle the game's overlay pause mode." );
        ImGui::EndTooltip     (       );
      }

      if (ImGui::BeginPopup ("EpicOverlayPauseMenu"))
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
    ImGui::Text     ( ": %10llu  ", SK::EOS::GetTicksRetired () );
    ImGui::Columns  (1, nullptr, false);

    return true;
  }

  return false;
}

bool
SK::ControlPanel::Epic::DrawMenu (void)
{
  return false;
}


bool
SK::ControlPanel::Epic::WarnIfUnsupported (void)
{
  return false;
}