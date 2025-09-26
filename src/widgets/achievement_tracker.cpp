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

#define __SK_SUBSYSTEM__ "   Test   "

extern iSK_INI* dll_ini;
extern iSK_INI* osd_ini;

static
std::set <std::string>
tokenize ( const std::wstring& input_wc,
           const std::string   delim = "|~|" )
{
  std::string input =
    SK_WideCharToUTF8 (input_wc);

  std::set <std::string> tokens;

  std::string::size_type start = 0;
  std::string::size_type delim_pos;

  while ( (delim_pos = input.find (delim, start)) != std::string::npos )
  {
    auto substr =
      input.substr (start, delim_pos - start);

    if (! substr.empty ())
    {
      tokens.insert (substr);
    }

    start =
      delim_pos + delim.length ();
  }

  auto substr =
    input.substr (start);

  if (! substr.empty ())
  {
    // Add the last token (or the whole string if no delimiter was found)
    tokens.insert (substr);
  }

  return
    tokens;
}

static
std::wstring
serialize ( const std::set <std::string>& input,
            const           std::string   delim = "|~|" )
{
  std::string            serialized;
  std::set <std::string> processed;

  for (auto& str : input)
  {
    if (! str.empty ())
    {
      if (processed.emplace (str.c_str ()).second)
      {
        serialized += str.c_str ();
        serialized += delim;
      }
    }
  }

  SK_LOGi1 (L"serialize=%hs [From %d entries]", serialized.c_str (), input.size ());

  return
    SK_UTF8ToWideChar (serialized);
}

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
    static bool        first_run = true;
    if (std::exchange (first_run , false))
    {
      std::scoped_lock <std::recursive_mutex> list_lock (name_list_s::lock);

      ignored.show = false;
      tracked.show = true;

      tracked.ini_pref =
        dynamic_cast <sk::ParameterStringW *> (
          SK_Widget_ParameterFactory->create_parameter <std::wstring> (L"Tracked Achievements")
        );

      ignored.ini_pref =
        dynamic_cast <sk::ParameterStringW *> (
          SK_Widget_ParameterFactory->create_parameter <std::wstring> (L"Ignored Achievements")
        );

      tracked.ini_pref->register_to_ini (dll_ini, L"Achievement.Tracker", L"TrackedAchievements");
      ignored.ini_pref->register_to_ini (dll_ini, L"Achievement.Tracker", L"IgnoredAchievements");

      std::wstring            tracked_str;
      tracked.ini_pref->load (tracked_str);

      auto tokenized_trackers =
        tokenize (tracked_str);

      for (auto& token : tokenized_trackers)
        tracked.names.emplace (token);

      std::wstring            ignored_str;
      ignored.ini_pref->load (ignored_str);

      auto tokenized_ignored =
        tokenize (ignored_str);

      for (auto& token : tokenized_ignored)
        ignored.names.emplace (token);

      setMinSize (
        ImVec2 (std::max (25.0f, getMinSize ().x),
                std::max (25.0f, getMinSize ().y))
      ).
      setMaxSize (
        ImVec2 (std::max (100.0f, getMaxSize ().x),
                std::max ( 50.0f, getMaxSize ().y))
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
          std::scoped_lock <std::recursive_mutex> list_lock (name_list_s::lock);

          size_t                             num_achvs = 0;
          auto                achievements             =
          achievement_mgr->getAchievements (&num_achvs);

          locked = (num_achvs == 0);

          if (num_achvs > 0)
          {
            for ( unsigned int i = 0 ; i < num_achvs ; ++i )
            {
              auto achievement = achievements [i];

              achievement->tracked_ = (tracked.names.contains (achievement->name_));
              achievement->ignored_ = (ignored.names.contains (achievement->name_));
            }
          }

          last_update = SK::ControlPanel::current_time;
        }
      }
    }
  }

  void config_base (void) noexcept override
  {
    SK_Widget::config_base ();

    ImGui::Separator ();

    std::scoped_lock <std::recursive_mutex> list_lock (name_list_s::lock);

    ImGui::Checkbox ("Show Ignored", &ignored.show);
    if (! ignored.names.empty ())
    {
      ImGui::SameLine ();
      ImGui::Text     ("\t[%2d Achievements Ignored]", ignored.names.size ());
    }

    ImGui::Checkbox ("Show Tracked", &tracked.show);
    if (! tracked.names.empty ())
    {
      ImGui::SameLine ();
      ImGui::Text     ("\t[%2d Achievements Tracked]", tracked.names.size ());
    }

    ImGui::Checkbox ("Show Hidden (by Game Dev.)", &show_hidden);
    ImGui::Checkbox ("Show Icons",                 &show_icons);
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

      std::scoped_lock <std::recursive_mutex> list_lock (name_list_s::lock);

      const auto& style = ImGui::GetStyle ();
      const float fLineHt =
        (ImGui::GetFontSize () + style.FramePadding    .y  * 2.0f +
                                 style.ItemInnerSpacing.y);// * ImGui::GetIO ().FontGlobalScale;

      if (SK_ImGui_Active ())
      {
        ImGui::BeginGroup ();
        for ( unsigned int i = 0 ; i < num_achvs ; ++i )
        {
          const auto achievement = achievements [i];

          if (show_hidden || (! achievement->hidden_))
          {
            SK_ImGui_VerticalSpacing spacing (fLineHt);

            ImGui::PushID (achievement->name_.c_str ());

            if (ignored.show || (! achievement->ignored_))
            {
              if (ImGui::Checkbox ("Track", &achievement->tracked_))
              {
                bool changed = true;

                if      ( achievement->tracked_ && !tracked.names.contains (achievement->name_)) tracked.names.insert (achievement->name_);
                else if (!achievement->tracked_ &&  tracked.names.contains (achievement->name_)) tracked.names.erase  (achievement->name_);
                else                                                                                                       changed = false;

                if (changed)
                {
                  std::wstring tracked_names =
                    serialize (tracked.names);

                  if (tracked.ini_pref != nullptr)
                      tracked.ini_pref->store (tracked_names);

                  dll_ini->write ();
                }
              }

              //ImGui::SameLine ();
              //
              //if (ImGui::Checkbox ("Ignore", &achievement->ignored_))
              //{
              //  if (achievement->ignored_) ignored.names.emplace (achievement->name_);
              //  else                       ignored.names.erase   (achievement->name_);
              //
              //  std::wstring ignored_names = serialize (ignored.names);
              //
              //  if (ignored.ini_pref != nullptr) ignored.ini_pref->store (ignored_names);
              //
              //  dll_ini->write ();
              //}
            }

            ImGui::PopID ();
          }
        }
        ImGui::EndGroup   ();
        ImGui::SameLine   ();
      }

      ImGui::BeginGroup ();
      for ( unsigned int i = 0 ; i < num_achvs ; ++i )
      {
        const auto achievement = achievements [i];

        if (show_hidden || (! achievement->hidden_))
        {
          if ((achievement->tracked_ && tracked.show) || (SK_ImGui_Active () &&
            ((!achievement->ignored_)|| ignored.show)))
          {
            SK_ImGui_VerticalSpacing spacing (fLineHt);

            ImGui::TextColored ( ImColor (1.0f, 1.0f, 1.0f, 1.0f),
                                    achievement->unlocked_ ?
                                            ICON_FA_UNLOCK : ICON_FA_LOCK );
          }
        }
      }
      ImGui::EndGroup   ();
      ImGui::SameLine   ();
      ImGui::BeginGroup ();
      for ( unsigned int i = 0 ; i < num_achvs ; ++i )
      {
        const auto achievement = achievements [i];

        if (show_hidden || (! achievement->hidden_))
        {
          if ((achievement->tracked_ && tracked.show) || (SK_ImGui_Active () &&
            ((!achievement->ignored_)|| ignored.show)))
          {
            SK_ImGui_VerticalSpacing spacing (fLineHt);

            const auto& state =
              achievement->unlocked_ ? achievement->text_.unlocked :
                                       achievement->text_.  locked;

            ImGui::Text ("%ws\t", state.human_name.c_str ());
          }
        }
      }
      ImGui::EndGroup   ();
      ImGui::SameLine   ();
      ImGui::BeginGroup ();
      for ( unsigned int i = 0 ; i < num_achvs ; ++i )
      {
        const auto achievement = achievements [i];

        if (show_hidden || (! achievement->hidden_))
        {
          if ((achievement->tracked_ && tracked.show) || (SK_ImGui_Active () &&
            ((!achievement->ignored_)|| ignored.show)))
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
                SK_FormatString ( "%ws  %.0f%%  [%d / %d]\t", state.desc.c_str (),
                                      achievement->progress_.getPercent        (),
                                      achievement->progress_.current,
                                      achievement->progress_.max );
              ImGui::ProgressBar (    achievement->progress_.getPercent () / 100.0F,
                                                               ImVec2 (-FLT_MIN, 0),
                                               str_progress.c_str () );
            }

            else if (! state.desc.empty ())
            {
              ImGui::Text ("%ws\t", state.desc.c_str ());
            }
          }
        }
      }
      ImGui::EndGroup   ();
      ImGui::SameLine   ();
      ImGui::BeginGroup ();
      for ( unsigned int i = 0 ; i < num_achvs ; ++i )
      {
        extern const char* SK_Achievement_RarityToName (float percent);

        const auto achievement = achievements [i];

        if (show_hidden || (! achievement->hidden_))
        {
          if ((achievement->tracked_ && tracked.show) || (SK_ImGui_Active () &&
            ((!achievement->ignored_)|| ignored.show)))
          {
            SK_ImGui_VerticalSpacing spacing (fLineHt);

            ImVec4 color =
              ImColor::HSV (0.4f * (achievement->global_percent_ / 100.0f), 1.0f, 1.0f);

            ImGui::TextColored ( color, "%hs",
              SK_Achievement_RarityToName (achievement->global_percent_)
            );
          }
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
  bool        show_hidden = false;
  bool        show_icons  = false;

  struct name_list_s {
    static std::recursive_mutex lock;
    bool                        show     = true;
    sk::ParameterStringW*       ini_pref = nullptr;
    std::set <std::string>      names;
  } tracked,
    ignored;
};
SK_LazyGlobal <SKWG_AchievementTracker> __achievement_tracker__;

void SK_Widget_InitAchieveTracker (void)
{
  SK_RunOnce (__achievement_tracker__.getPtr ());
}

std::recursive_mutex SKWG_AchievementTracker::name_list_s::lock;