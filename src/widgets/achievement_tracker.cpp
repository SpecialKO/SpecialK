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
        ImVec2 (std::max (100.0f, getMinSize ().x),
                std::max ( 50.0f, getMinSize ().y))
      ).
      setMaxSize (
        ImVec2 (std::max (1024.0f, getMaxSize ().x),
                std::max ( 768.0f, getMaxSize ().y))
      );
    }

    if (active)
    {
      if (last_update < SK::ControlPanel::current_time - update_freq)
      {
        auto achievement_mgr = SK_Platform_GetAchievementManager ();
        if ( achievement_mgr != nullptr )
        {
          std::scoped_lock <std::recursive_mutex> list_lock (name_list_s::lock);

          size_t                             num_achvs = 0;
          auto                achievements             =
          achievement_mgr->getAchievements (&num_achvs);

          locked = (num_achvs == 0) || (tracked.names.empty () && (! SK_ImGui_Active ()));

          if (num_achvs > 0)
          {
            max_text_width = 0.0f;

            for ( unsigned int i = 0 ; i < num_achvs ; ++i )
            {
              auto achievement = achievements [i];

              achievement->tracked_ = (tracked.names.contains (achievement->name_));
              achievement->ignored_ = (ignored.names.contains (achievement->name_));

              // Remove from tracking after unlock...
              if (achievement->tracked_ && achievement->unlocked_)
              {
                                     achievement->tracked_ = false;
                tracked.names.erase (achievement->name_);
              }

              const auto& state =
                  achievement->unlocked_ ? achievement->text_.unlocked :
                                           achievement->text_.  locked;

              max_text_width =
                std::max ( max_text_width,
                  ImGui::CalcTextSize (state.desc.c_str ()).x + ImGui::GetStyle ().FramePadding.x * 2
                );
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

    simple_bg =
      (! SK_ImGui_Active ());

    auto achievement_mgr = SK_Platform_GetAchievementManager ();
    if ( achievement_mgr != nullptr )
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

      size_t locked_count          = 0;
      float  track_checkbox_offset = 0.0f;

      ImGui::PushStyleColor (ImGuiCol_TableRowBg,    ImVec4 (0.000f, 0.000f, 0.000f, 0.5f));
      ImGui::PushStyleColor (ImGuiCol_TableRowBgAlt, ImVec4 (0.175f, 0.175f, 0.175f, 0.5f));

      auto flags = ImGuiTableFlags_Borders        | ImGuiTableFlags_RowBg          |
                   ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoSavedSettings;

      // Allow scrolling while the control panel is open.
      if (SK_ImGui_Active ())
        flags |= ImGuiTableFlags_ScrollY;

      if (ImGui::BeginTable ("Achievements", 3, flags))
      {
        ImGui::TableSetupColumn ("Name");
        ImGui::TableSetupColumn ("Description");
        ImGui::TableSetupColumn ("Rarity");

        if (num_achvs > 0)
        {
          if (SK_ImGui_Active ())
          {
            // Table headers
            ImGui::TableSetupScrollFreeze ( 0,2 );
            ImGui::TableHeadersRow        (     );
            ImGui::TableNextRow           (     );
            ImGui::TableSetColumnIndex    (  0  );
            ImGui::Separator              (     );
            ImGui::TableSetColumnIndex    (  1  );
            ImGui::Separator              (     );
            ImGui::TableSetColumnIndex    (  2  );
            ImGui::Separator              (     );
          }

          else
          {
            ImGui::ScrollToItem ();
          }
        }

        extern const char* SK_Achievement_RarityToName (float percent);

        // Populate rows (Locked)
        for ( unsigned int i = 0 ; i < num_achvs ; ++i )
        {
          const auto achievement =
            achievements [i];

          if (! achievement->unlocked_)
            ++locked_count;

          if (show_hidden || (! achievement->hidden_))
          {
            ImGui::PushID (achievement->name_.c_str ());

            if ((ignored.show || (! achievement->ignored_)) && (! achievement->unlocked_))
            {
              if ((achievement->tracked_ && tracked.show) || (SK_ImGui_Active () &&
                ((!achievement->ignored_)|| ignored.show)))
              {
                const auto& state =
                  achievement->unlocked_ ? achievement->text_.unlocked :
                                           achievement->text_.  locked;

                ImGui::TableNextRow        ( );
                ImGui::TableSetColumnIndex (0);

                if (SK_ImGui_Active ())
                {
                  const auto x_orig =
                    ImGui::GetCursorPosX ();
                  
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
                  ImGui::SameLine   ();

                  track_checkbox_offset =
                    ImGui::GetCursorPosX () - x_orig;
                }

                ImGui::TextColored ( ImColor (1.0f, 1.0f, 1.0f, 1.0f),
                                        achievement->unlocked_ ?
                                                ICON_FA_UNLOCK : ICON_FA_LOCK );
                ImGui::SameLine    ();
                ImGui::Text        ("%hs ", state.human_name.c_str ());


                ImGui::TableSetColumnIndex (1);

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

                ImGui::SetNextItemWidth (max_text_width);

                if ( achievement->progress_.max != achievement->progress_.current &&
                     achievement->progress_.max != 1                              &&
                     ! state.desc.empty () )
                {
                  std::string str_progress =
                    SK_FormatString ( "%hs  %.0f%%  [%d / %d]", state.desc.c_str (),
                                          achievement->progress_.getPercent        (),
                                          achievement->progress_.current,
                                          achievement->progress_.max );
                  ImGui::ProgressBar (    achievement->progress_.getPercent () / 100.0F,
                                                                   ImVec2 (max_text_width, 0),
                                                   str_progress.c_str () );
                  ImGui::SameLine        (   );
                  ImGui::TextUnformatted (" ");
                }

                else if (! state.desc.empty ())
                {
                  ImGui::Text ("%hs ", state.desc.c_str ());
                }


                ImGui::TableSetColumnIndex (2);

                ImVec4 color =
                  ImColor::HSV (0.4f * (achievement->global_percent_ / 100.0f), 1.0f, 1.0f);

                ImGui::TextColored ( color, "%hs",
                  SK_Achievement_RarityToName (achievement->global_percent_)
                );
              }
            }

            ImGui::PopID ();
          }
        }

        // Now for already unlocked achievements...
        //
        //   Do not display these unless the control panel is open,
        //     because there is nothing to actually track.
        if (locked_count != num_achvs && SK_ImGui_Active ())
        {
          ImGui::TableNextRow        ( );
          ImGui::TableSetColumnIndex (0);
          ImGui::Separator           ( );
          ImGui::TableSetColumnIndex (1);
          ImGui::Separator           ( );
          ImGui::TableSetColumnIndex (2);
          ImGui::Separator           ( );

          // Populate rows (Unlocked)
          for ( unsigned int i = 0 ; i < num_achvs ; ++i )
          {
            const auto achievement = achievements [i];

            if (show_hidden || (! achievement->hidden_))
            {
              ImGui::PushID (achievement->name_.c_str ());

              if ((ignored.show || (! achievement->ignored_)) && achievement->unlocked_)
              {
                ImGui::TableNextRow        ( );
                ImGui::TableSetColumnIndex (0);

                ImGui::SetCursorPosX (ImGui::GetCursorPosX () + track_checkbox_offset);

                const auto& state =
                  achievement->unlocked_ ? achievement->text_.unlocked :
                                           achievement->text_.  locked;

                ImGui::TextColored ( ImColor (1.0f, 1.0f, 1.0f, 1.0f),
                                        achievement->unlocked_ ?
                                                ICON_FA_UNLOCK : ICON_FA_LOCK );
                ImGui::SameLine    ();
                ImGui::Text        ("%hs ", state.human_name.c_str ());


                ImGui::TableSetColumnIndex (1);

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

                ImGui::SetNextItemWidth (max_text_width);

                if ( achievement->progress_.max != achievement->progress_.current &&
                     achievement->progress_.max != 1                              &&
                     ! state.desc.empty () )
                {
                  std::string str_progress =
                    SK_FormatString ( "%hs  %.0f%%  [%d / %d]", state.desc.c_str (),
                                          achievement->progress_.getPercent      (),
                                          achievement->progress_.current,
                                          achievement->progress_.max );
                  ImGui::ProgressBar (    achievement->progress_.getPercent () / 100.0F,
                                                                   ImVec2 (max_text_width, 0),
                                                   str_progress.c_str () );
                  ImGui::SameLine        (   );
                  ImGui::TextUnformatted (" ");
                }

                else if (! state.desc.empty ())
                {
                  ImGui::Text ("%hs ", state.desc.c_str ());
                }


                ImGui::TableSetColumnIndex (2);

                ImVec4 color =
                  ImColor::HSV (0.4f * (achievement->global_percent_ / 100.0f), 1.0f, 1.0f);

                ImGui::TextColored ( color, "%hs",
                  SK_Achievement_RarityToName (achievement->global_percent_)
                );
              }

              ImGui::PopID ();
            }
          }
        }

        ImGui::EndTable ();
      }

      ImGui::PopStyleColor (2);
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
  const DWORD update_freq = 250UL;
  bool        show_hidden = false;
  bool        show_icons  = false;

  struct name_list_s {
    static std::recursive_mutex lock;
    bool                        show     = true;
    sk::ParameterStringW*       ini_pref = nullptr;
    std::set <std::string>      names;
  } tracked,
    ignored;

  float       max_text_width = 0.0f;
};
SK_LazyGlobal <SKWG_AchievementTracker> __achievement_tracker__;

void SK_Widget_InitAchieveTracker (void)
{
  SK_RunOnce (__achievement_tracker__.getPtr ());
}

std::recursive_mutex SKWG_AchievementTracker::name_list_s::lock;