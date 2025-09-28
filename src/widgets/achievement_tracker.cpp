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

    if (strlen (substr.c_str ()))
    {
      tokens.emplace (substr);
    }

    start =
      delim_pos + delim.length ();
  }

  auto substr =
    input.substr (start);

  if (strlen (substr.c_str ()))
  {
    // Add the last token (or the whole string if no delimiter was found)
    tokens.emplace (substr);
  }

  if (config.system.log_level > 0)
  {
    for ( auto& token : tokens )
    {
      SK_LOGi1 (L"Token: %hs", token.c_str ());
    }
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

  size_t i = 0;

  for (auto& str : input)
  {
    if (! str.empty ())
    {
      if (processed.emplace (str.c_str ()).second)
      {
        serialized += str.c_str ();

        if (i < input.size () - 1)
          serialized += delim;
      }
    }

    ++i;
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
      std::scoped_lock <std::recursive_mutex>
             list_lock (name_list_s::lock);

      ignored.show = false;
      tracked.show = true;

      show_hidden_pref =
        dynamic_cast <sk::ParameterBool *> (
          SK_Widget_ParameterFactory->create_parameter <bool> (L"Show Hidden")
        );

      show_rarity_pref =
        dynamic_cast <sk::ParameterBool *> (
          SK_Widget_ParameterFactory->create_parameter <bool> (L"Always Show Rarity")
        );

      search_url_pref =
        dynamic_cast <sk::ParameterStringW *> (
          SK_Widget_ParameterFactory->create_parameter <std::wstring> (L"Achievement Guide Search URL")
        );

      tracked.ini_pref =
        dynamic_cast <sk::ParameterStringW *> (
          SK_Widget_ParameterFactory->create_parameter <std::wstring> (L"Tracked Achievements")
        );

      ignored.ini_pref =
        dynamic_cast <sk::ParameterStringW *> (
          SK_Widget_ParameterFactory->create_parameter <std::wstring> (L"Ignored Achievements")
        );

      show_hidden_pref->register_to_ini (osd_ini, L"Widget.Achievement Tracker",           L"ShowHidden");
      show_rarity_pref->register_to_ini (osd_ini, L"Widget.Achievement Tracker",     L"AlwaysShowRarity");
       search_url_pref->register_to_ini (osd_ini, L"Widget.Achievement Tracker", L"AchievementSearchURL");
      tracked.ini_pref->register_to_ini (dll_ini,        L"Achievement.Tracker",  L"TrackedAchievements");
      ignored.ini_pref->register_to_ini (dll_ini,        L"Achievement.Tracker",  L"IgnoredAchievements");

      show_hidden_pref->load (show_hidden);
      show_rarity_pref->load (show_rarity);
      search_url_pref->load  (search_url);

      std::wstring            tracked_str;
      tracked.ini_pref->load (tracked_str);

      auto tokenized_trackers =
        tokenize (tracked_str);

      for (auto& token : tokenized_trackers)
        tracked.names.emplace (token.c_str ());

      std::wstring            ignored_str;
      ignored.ini_pref->load (ignored_str);

      auto tokenized_ignored =
        tokenize (ignored_str);

      for (auto& token : tokenized_ignored)
        ignored.names.emplace (token.c_str ());

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
          std::scoped_lock <std::recursive_mutex>
                 list_lock (name_list_s::lock);

          size_t                             num_achvs = 0;
          auto                achievements             =
          achievement_mgr->getAchievements (&num_achvs);

          locked = (num_achvs == 0) || (tracked.names.empty () && (! SK_ImGui_Active ()));

          if (num_achvs > 0)
          {
            max_text_width = 0.0f;

            std::set <std::string> tracked_rejects = tracked.names;
            std::set <std::string> ignored_rejects = ignored.names;

            for ( unsigned int i = 0 ; i < num_achvs ; ++i )
            {
              auto achievement = achievements [i];

              if (tracked_rejects.contains (achievement->name_))
                  tracked_rejects.erase    (achievement->name_);

              if (ignored_rejects.contains (achievement->name_))
                  ignored_rejects.erase    (achievement->name_);

              achievement->tracked_ = (tracked.names.contains (achievement->name_));
              achievement->ignored_ = (ignored.names.contains (achievement->name_));

              // Remove from tracking after unlock...
              if (achievement->tracked_ && achievement->unlocked_)
              {
                tracked_rejects.emplace (achievement->name_.c_str ());
                                         achievement->tracked_ = false;
                  tracked.names.erase   (achievement->name_);
              }

              const auto& state =
                  achievement->unlocked_ ? achievement->text_.unlocked :
                                           achievement->text_.  locked;

              max_text_width =
                std::max ( max_text_width,
                  ImGui::CalcTextSize (state.desc.c_str ()).x + ImGui::GetStyle ().FramePadding.x * 2
                );
            }

            for (auto& reject : tracked_rejects)
                   tracked.names.erase (reject);

            for (auto& reject : ignored_rejects)
                   ignored.names.erase (reject);

            if (! tracked_rejects.empty ())
            {
              save_tracked_list ();
            }

            if (! ignored_rejects.empty ())
            {
              save_ignored_list ();
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

    std::scoped_lock <std::recursive_mutex>
           list_lock (name_list_s::lock);

#if 0
    ImGui::Checkbox ("Show Ignored", &ignored.show);
    if (! ignored.names.empty ())
    {
      ImGui::SameLine ();
      ImGui::Text     ("\t[%2d Achievements Ignored]", ignored.names.size ());
    }
#endif

    ImGui::Checkbox ("Show Tracked", &tracked.show);
    if (! tracked.names.empty ())
    {
      ImGui::SameLine ();
      ImGui::Text     ("\t[%2d Achievements Tracked]", tracked.names.size ());
    }

    const char* szLabel =
      show_hidden ? "Show Hidden ( " ICON_FA_EYE       " )"
                  : "Show Hidden ( " ICON_FA_EYE_SLASH " )";

    if (ImGui::Checkbox (szLabel, &show_hidden))
    {
      show_hidden_pref->store (show_hidden);
               osd_ini->write (           );
    }

    ImGui::SetItemTooltip (
      "Shows achievements that the developer has marked "
      "hidden until unlocked."
    );

    if (ImGui::Checkbox ("Always Show Rarity", &show_rarity))
    {
      show_rarity_pref->store (show_rarity);
               osd_ini->write (           );
    }

    ImGui::SetItemTooltip (
      "Shows achievement rarity even in the simplified tracker-only view."
    );

    static char          szSearchURL [512] = {};
    SK_RunOnce (sprintf (szSearchURL, "%ws", search_url.c_str ()));

    if (ImGui::InputText ("Achievement Search URL", szSearchURL, 512))
    {
      search_url =
        SK_UTF8ToWideChar (szSearchURL);

      search_url_pref->store (search_url);
              osd_ini->write (          );
    }

    ImGui::SetItemTooltip (
      "Clicking on an achievement will use this URL to do a web search.\n\n"
      " " ICON_FA_INFO_CIRCLE " Clear the text to disable this feature."
    );

  //ImGui::Checkbox ("Show Icons",                 &show_icons);
  }

  void draw (void) noexcept override
  {
    if (ImGui::GetFont () == nullptr) return;

    simple_bg =
      (! SK_ImGui_Active ());

    const auto window =
      ImGui::GetCurrentWindow ();

    // The Table used by this widget often captures input and prevents
    //   normal right-click behavior, so we need to do it manually...
    if (ImGui::IsMouseHoveringRect (window->Rect ().Min,
                                    window->Rect ().Max))
    {
      if (ImGui::GetIO ().MouseDown [ImGuiMouseButton_Right])
        state__ = 1;
    }

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

      std::scoped_lock <std::recursive_mutex>
             list_lock (name_list_s::lock);

      size_t locked_count          = 0;
      float  track_checkbox_offset = 0.0f;

      ImGui::PushStyleColor (ImGuiCol_TableRowBg,    ImVec4 (0.00f, 0.00f, 0.00f, 0.666f));
      ImGui::PushStyleColor (ImGuiCol_TableRowBgAlt, ImVec4 (0.15f, 0.15f, 0.15f, 0.666f));

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

                    if      ( achievement->tracked_ && !tracked.names.contains (achievement->name_)) tracked.names.emplace (achievement->name_.c_str ());
                    else if (!achievement->tracked_ &&  tracked.names.contains (achievement->name_)) tracked.names.erase   (achievement->name_);
                    else                                                                                                        changed = false;

                    if (changed)
                    {
                      save_tracked_list ();
                    }
                  }

                  if (ignored.show)
                  {
                    ImGui::SameLine ();

                    if (ImGui::Checkbox ("Ignore", &achievement->ignored_))
                    {
                      bool changed = true;

                      if      ( achievement->ignored_ && !ignored.names.contains (achievement->name_)) ignored.names.emplace (achievement->name_.c_str ());
                      else if (!achievement->ignored_ &&  ignored.names.contains (achievement->name_)) ignored.names.erase   (achievement->name_);
                      else                                                                                                        changed = false;

                      if (changed)
                      {
                        save_ignored_list ();
                      }
                    }
                  }

                  ImGui::SameLine ();

                  track_checkbox_offset =
                    ImGui::GetCursorPosX () - x_orig;
                }

                draw_achievement_name         (achievement, state);
                ImGui::TableSetColumnIndex    (                 1);
                draw_description_and_progress (achievement, state);
                ImGui::TableSetColumnIndex    (                 2);
                draw_rarity_text              (achievement       );
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
            const auto achievement =
              achievements [i];

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

                draw_achievement_name         (achievement, state);
                ImGui::TableSetColumnIndex    (                 1);
                draw_description_and_progress (achievement, state);
                ImGui::TableSetColumnIndex    (                 2);
                draw_rarity_text              (achievement       );
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

  void
  draw_achievement_name ( const SK_Achievement*                  achievement,
                          const SK_Achievement::text_s::state_s& state )
  {
    ImGui::BeginGroup    ();
    ImGui::TextColored ( ImColor (1.0f, 1.0f, 1.0f, 1.0f),
                            achievement->unlocked_ ?
                                    ICON_FA_UNLOCK : ICON_FA_LOCK );
    ImGui::SameLine    ();
    ImGui::Text        ("%hs ", state.human_name.c_str ());

    if (achievement->hidden_)
    {
      ImGui::SameLine        ();
      ImGui::TextUnformatted (ICON_FA_EYE " ");
    }
    ImGui::EndGroup    ();

    if (! search_url.empty ())
    {
      if (ImGui::IsItemClicked ())
      {
        std::string url =
          SK_FormatString (
            "%ws %hs Achievement in %hs",
                    search_url.c_str (),
              state.human_name.c_str (), SK_GetFriendlyAppName ().c_str ()
          );

        SK_SteamOverlay_GoToURL (url.c_str (), true);
      }

      if (ImGui::IsItemHovered ())
      {
        ImGui::TableSetBgColor (ImGuiTableBgTarget_CellBg, IM_COL32(128, 128, 128, 255));
      }
    }
  }

  void
  draw_description_and_progress ( const SK_Achievement*                  achievement,
                                  const SK_Achievement::text_s::state_s& state )
  {
    ImGui::BeginGroup ();

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
    ImGui::EndGroup ();

    if (! search_url.empty ())
    {
      if (ImGui::IsItemClicked ())
      {
        std::string url =
          SK_FormatString (
            "%ws %hs Achievement in %hs",
                    search_url.c_str (),
              state.human_name.c_str (), SK_GetFriendlyAppName ().c_str ()
          );

        SK_SteamOverlay_GoToURL (url.c_str (), true);
      }

      if (ImGui::IsItemHovered ())
      {
        ImGui::TableSetBgColor (ImGuiTableBgTarget_CellBg, IM_COL32(128, 128, 128, 255));
      }
    }
  }

  void
  draw_rarity_text (const SK_Achievement* achievement)
  {
    if (! (SK_ImGui_Active () || show_rarity))
      return;

    extern const char* SK_Achievement_RarityToName (float percent);

    ImVec4 color =
      ImColor::HSV (0.4f * (achievement->global_percent_ / 100.0f), 1.0f, 1.0f);

    ImGui::TextColored ( color, "%hs",
      SK_Achievement_RarityToName (achievement->global_percent_)
    );
    
    ImGui::SetItemTooltip (
      "%4.1f%% of players have unlocked this achievement.",
                              achievement->global_percent_
    );
  }

  void save_tracked_list (void)
  {
    std::wstring tracked_names =
      serialize (tracked.names);
    
    if (tracked.ini_pref != nullptr)
        tracked.ini_pref->store (tracked_names);
    
    dll_ini->write ();
  }

  void save_ignored_list (void)
  {
    std::wstring ignored_names =
      serialize (ignored.names);
    
    if (ignored.ini_pref != nullptr)
        ignored.ini_pref->store (ignored_names);
    
    dll_ini->write ();
  }

protected:
  const DWORD update_freq = 250UL;
  bool        show_icons  = false; // Unused for now

  sk::ParameterBool*    show_hidden_pref = nullptr;
  bool                  show_hidden      = false;

  sk::ParameterStringW* search_url_pref  = nullptr;
  std::wstring          search_url       = L"https://duckduckgo.com/?q=!trueachievements";

  sk::ParameterBool*    show_rarity_pref = nullptr;
  bool                  show_rarity      = true;

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