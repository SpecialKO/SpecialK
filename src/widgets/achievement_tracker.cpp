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
#include <imgui/font_awesome.h>

extern iSK_INI* osd_ini;

class SKWG_AchievementTracker : public SK_Widget
{
public:
  SKWG_AchievementTracker (void) noexcept : SK_Widget ("Achievement Tracker")
  {
    SK_ImGui_Widgets->achieve_tracker = this;

    setAutoFit (true).setDockingPoint (DockAnchor::West).
    setBorder  (true);
  };

  void load (iSK_INI* cfg) noexcept override
  {
    SK_Widget::load (cfg);
  }

  void save (iSK_INI* cfg) noexcept override
  {
    if (cfg == nullptr)
      return;

    SK_Widget::save (cfg);

    cfg->write ();
  }

  void run (void) noexcept override
  {
    static bool first_run = true;

    if (first_run)
    {
      first_run = false;

      setMinSize (
        ImVec2 (std::max (175.0f, getMinSize ().x),
                std::max (125.0f, getMinSize ().y))
      ).
      setMaxSize (
        ImVec2 (std::max (175.0f, getMaxSize ().x),
                std::max (125.0f, getMaxSize ().y))
      );
    }

    if (active)
    {
      if (last_update < SK::ControlPanel::current_time - update_freq)
      {
        SK_AchievementManager* SK_EOS_GetAchievementManager (void);
        auto achievement_mgr = SK_EOS_GetAchievementManager ();

        if (achievement_mgr != nullptr)
        {
          size_t                             num_achvs = 0;
          achievement_mgr->getAchievements (&num_achvs);

          locked = (num_achvs == 0);
        }

        last_update = SK::ControlPanel::current_time;
      }
    }
  }

  void config_base (void) noexcept override
  {
    SK_Widget::config_base ();

    ImGui::Separator ();
  }

  void draw (void) noexcept override
  {
    if (ImGui::GetFont () == nullptr) return;

    SK_AchievementManager* SK_EOS_GetAchievementManager (void);
    auto achievement_mgr = SK_EOS_GetAchievementManager ();

    if (achievement_mgr != nullptr)
    {
      size_t num_achvs = 0;

      auto achievements =
        achievement_mgr->getAchievements (&num_achvs);

      if (num_achvs == 0)
        return;

      class SK_ImGui_VerticalSpacing {
      public:
         SK_ImGui_VerticalSpacing (float fLineHt) { orig_y_pos = ImGui::GetCursorPosY (); line_ht = fLineHt; };
        ~SK_ImGui_VerticalSpacing (void)          { ImGui::SetCursorPosY (line_ht + orig_y_pos);             };

        float line_ht;
        float orig_y_pos;
      };

      const float fLineHt =
        (ImGui::GetFontSize () + ImGui::GetStyle ().FramePadding.y * 2.0f + ImGui::GetStyle ().ItemInnerSpacing.y) * ImGui::GetIO ().FontGlobalScale;

      ImGui::BeginGroup ();
      for ( unsigned int i = 0 ; i < num_achvs ; ++i )
      {
        auto achievement = achievements [i];

        if (! achievement->hidden_)
        {
          SK_ImGui_VerticalSpacing spacing (fLineHt);

          ImGui::TextColored ( ImColor (1.0f, 1.0f, 1.0f, 1.0f),
                                  achievement->unlocked_ ?
                                          ICON_FA_UNLOCK : ICON_FA_LOCK );
        }
      }
      ImGui::EndGroup   ();
      ImGui::SameLine   ();
      ImGui::BeginGroup ();
      for ( unsigned int i = 0 ; i < num_achvs ; ++i )
      {
        auto achievement = achievements [i];

        if (! achievement->hidden_)
        {
          SK_ImGui_VerticalSpacing spacing (fLineHt);

          const auto& state =
            achievement->unlocked_ ? achievement->text_.unlocked :
                                     achievement->text_.  locked;

          ImGui::Text ("%ws", state.human_name.c_str ());
        }
      }
      ImGui::EndGroup   ();
      ImGui::SameLine   ();
      ImGui::BeginGroup ();
      for ( unsigned int i = 0 ; i < num_achvs ; ++i )
      {
        auto achievement = achievements [i];

        if (! achievement->hidden_)
        {
          SK_ImGui_VerticalSpacing spacing (fLineHt);

          if (achievement->tracked_stats_.data.size () > 1)
          {
            ImGui::Text (ICON_FA_EXCLAMATION_CIRCLE);
            if (ImGui::BeginItemTooltip ())
            {
              ImGui::Text (
                "This achievement is an aggregate of %d different stats and "
                "progress cannot be shown in the UI.", achievement->tracked_stats_.data.size ()
              );
              ImGui::EndTooltip ();
            }
            ImGui::SameLine ();
          }

          const auto& state =
            achievement->unlocked_ ? achievement->text_.unlocked :
                                     achievement->text_.  locked;

          if ( achievement->progress_.max != achievement->progress_.current &&
               achievement->progress_.max != 1                              &&
               ! state.desc.empty () )
          {
            std::string str_progress =
              SK_FormatString ( "%ws  %.0f%%  [%d / %d]", state.desc.c_str (),
                                  achievement->progress_.getPercent        (),
                                  achievement->progress_.current,
                                  achievement->progress_.max );
            ImGui::ProgressBar (  achievement->progress_.getPercent () / 100.0F,
                                                           ImVec2 (-FLT_MIN, 0),
                                           str_progress.c_str () );
          }

          else if (! state.desc.empty ())
          {
            ImGui::Text ("%ws", state.desc.c_str ());
          }
        }
      }
      ImGui::EndGroup   ();
      ImGui::SameLine   ();
      ImGui::BeginGroup ();
      for ( unsigned int i = 0 ; i < num_achvs ; ++i )
      {
        extern const char* SK_Achievement_RarityToName (float percent);

        auto achievement = achievements [i];

        if (! achievement->hidden_)
        {
          SK_ImGui_VerticalSpacing spacing (fLineHt);

          ImVec4 color =
            ImColor::HSV (0.4f * (achievement->global_percent_ / 100.0f), 1.0f, 1.0f);

          ImGui::TextColored ( color, "%hs",
            SK_Achievement_RarityToName (achievement->global_percent_)
          );
        }
      }
      ImGui::EndGroup   ();
    }

    //const float ui_scale  = ImGui::GetIO ().FontGlobalScale;
    //const float font_size = ImGui::GetFont ()->FontSize * ui_scale;
  }

  void OnConfig (ConfigEvent event) override
  {
    switch (event)
    {
      case SK_Widget::ConfigEvent::LoadComplete:
        break;

      case SK_Widget::ConfigEvent::SaveStart:
        break;
    }
  }

private:
  DWORD last_update = 0UL;

protected:
  const DWORD update_freq = 666UL;
};

SK_LazyGlobal <SKWG_AchievementTracker> __achievement_tracker__;

void SK_Widget_InitAchieveTracker (void)
{
  SK_RunOnce (__achievement_tracker__.getPtr ());
}