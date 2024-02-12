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

#include <SpecialK/control_panel/notifications.h>
#include <imgui/font_awesome.h>

using namespace SK::ControlPanel;

bool
SK::ControlPanel::Notifications::Draw (void)
{
  if (SK_ImGui_SilencedNotifications > 0)
  {
    ImGui::TextColored (
      ImVec4 (.9f, .9f, .1f, 1.f), "%d " ICON_FA_INFO_CIRCLE,
      SK_ImGui_SilencedNotifications
    );

    ImGui::SameLine ();
  }

  if (ImGui::CollapsingHeader ("Notifications"/*, ImGuiTreeNodeFlags_DefaultOpen)*/))
  {
    ImGui::TreePush ("");

    ImGui::BeginGroup ();
    if (ImGui::Checkbox ("Silent Mode", &config.notifications.silent ))
    {
      if (! config.notifications.silent)
      {
        SK_ImGui_UnsilenceNotifications ();
      }
    }

    if (SK_ImGui_SilencedNotifications > 0)
    {
      ImGui::SameLine ();
      ImGui::Text     ("%d unseen", SK_ImGui_SilencedNotifications);
      ImGui::SameLine ();
      if (ImGui::Button ("Show Notifications"))
      {
        SK_ImGui_UnsilenceNotifications ();
      }
    }

    ImGui::SameLine    ();
    ImGui::SeparatorEx (ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine    ();

    bool bRelocate =
      ImGui::Combo ( "Display Corner", &config.notifications.location,
                     "Top-Left\0Top-Right\0"
                     "Bottom-Left\0Bottom-Right\0\0" );

    if (bRelocate)
    {
      SK_ImGui_CreateNotification (
        "Notifications.Relocated", SK_ImGui_Toast::Info,
        "Notifications Will Now Be Displayed Here", nullptr,
          3333UL, SK_ImGui_Toast::UseDuration  |
                  SK_ImGui_Toast::ShowCaption  |
                  SK_ImGui_Toast::Unsilencable |
                  SK_ImGui_Toast::DoNotSaveINI );
    }
    ImGui::EndGroup  ();
    ImGui::Separator ();

    int removed = 0;

    auto notify_ini =
      SK_GetNotifyINI ();

    if (ImGui::Button ("Clear Global Preferences"))
    {
      iSK_INI::_TSectionMap sections =
        notify_ini->get_sections ();

      for ( auto& section : sections )
      {
        if (! section.first._Equal (L"Notify.System"))
        {
          ++removed;
          notify_ini->remove_section (section.first);
        }
      }

      if (removed > 0)
      {
        config.utility.save_async ();

        SK_ImGui_CreateNotification (
          "Notifications.Reset", SK_ImGui_Toast::Success,
          "Notification Preferences Reset", nullptr,
            2345UL, SK_ImGui_Toast::UseDuration  |
                    SK_ImGui_Toast::ShowCaption  |
                    SK_ImGui_Toast::Unsilencable |
                    SK_ImGui_Toast::DoNotSaveINI );
      }
    }

    if (removed == 0)
    {
      ImGui::SameLine ();

      bool bConfigIndividual =
        ImGui::TreeNode ("Configure Individual Notifications");

      if (bConfigIndividual)
      { ImGui::TreePop ();

        ImGui::BeginGroup ();

        std::wstring section_to_clear = L"";

        for ( auto& section : notify_ini->get_sections () )
        {
          if (! section.first._Equal (L"Notify.System"))
          {
            std::string str_id =
              SK_WideCharToUTF8 (section.first);

            if (ImGui::TreeNode (str_id.c_str ()))
            {
              if (ImGui::Button ("Clear"))
              {
                section_to_clear = section.first;
              }

              ImGui::SameLine ();

              if (ImGui::Button ("Configure"))
              {
                SK_ImGui_CreateNotification (
                  str_id.c_str (), SK_ImGui_Toast::Other, nullptr, nullptr,
                         INFINITE, SK_ImGui_Toast::ShowCfgPanel |
                                   SK_ImGui_Toast::Unsilencable |
                                   SK_ImGui_Toast::UseDuration  |
                                   SK_ImGui_Toast::ShowNewest );
              }

              ImGui::TreePop ();
            }
          }
        }

        if (! section_to_clear.empty ())
        {
          notify_ini->remove_section (section_to_clear);
          config.utility.save_async ();
        }

        ImGui::EndGroup ();
      }
    }

    ImGui::TreePop  ();

    return true;
  }

  return false;
}