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
#include <storefront/gog.h>

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

    std::scoped_lock <std::recursive_mutex>
           list_lock (name_list_s::lock);

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
      ImVec2 (std::max (3252.0f, getMaxSize ().x),
              std::max (2048.0f, getMaxSize ().y))
    );
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
    if (! SK_GetFramesDrawn ())
      return;

    static bool tracked_names_empty =
                          tracked.names.empty ();
    if (isActive () || (! tracked.names.empty ()) || tracked_names_empty != tracked.names.empty ())
    {
      if (last_update + update_freq < SK::ControlPanel::current_time)
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
            tracked_names_empty = tracked.names.empty ();

            std::set <std::string> tracked_rejects = tracked.names;
            std::set <std::string> ignored_rejects = ignored.names;

            for ( unsigned int i = 0 ; i < num_achvs ; ++i )
            {
              auto achievement = achievements [i];

#ifdef _TEST_TRACKER_FLASH
              if (achievement->tracked_)
              {
                if (
                static_cast <DWORD> ( ( static_cast <float> (std::rand ()) /
                                        static_cast <float> (RAND_MAX) ) *
                                                1000.0f ) > 950 )
                {
                  achievement->progress_.last_update_ms = SK::ControlPanel::current_time;
                  flashVisible (config.platform.achievements.tracker_flash_seconds);
                }
              }
#endif

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
      config.utility.save_async (           );
    }

    ImGui::SetItemTooltip (
      "Shows achievements that the developer has marked "
      "hidden until unlocked."
    );

    if (ImGui::Checkbox ("Always Show Rarity", &show_rarity))
    {
        show_rarity_pref->store (show_rarity);
      config.utility.save_async (           );
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
      config.utility.save_async (          );
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

    ImGui::PushID (&window);

    // The Table used by this widget often captures input and prevents
    //   normal right-click behavior, so we need to do it manually...
    if (ImGui::IsMouseHoveringRect (window->Rect ().Min,
                                    window->Rect ().Max))
    {
      if (ImGui::GetIO ().MouseDown [ImGuiMouseButton_Right])
        state__ = 1;
    }

    draw_main_view (true);

    //const float ui_scale  = ImGui::GetIO ().FontGlobalScale;
    //const float font_size = ImGui::GetFont ()->FontSize * ui_scale;

    ImGui::PopID ();
  }

  void sort_table_if_needed (size_t num_achvs, SK_Achievement** achievements)
  {
    static int last_column = -1;
    static int last_sort   = -1;

    // Initialize sorting on first-use
    if (sorting.ordered_list.empty ())
    {
      for (unsigned int i = 0 ; i < num_achvs ; ++i)
      {
        sorting.ordered_list.push_back (i);
      }
    }

    if (ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs ())
    {
      // SpecsDirty is not a reliable measure, it is falsely set to true
      //   when no sorting is actually needed.
      if ( sort_specs->SpecsDirty &&
           sort_specs->Specs->SortDirection != ImGuiSortDirection_None )
      {
        if (last_sort   != sort_specs->Specs->SortDirection ||
            last_column != sort_specs->Specs->ColumnIndex)
        {
          switch (sort_specs->Specs->ColumnIndex)
          {
            case 0:
            case 1:
              for (unsigned int i = 0 ; i < num_achvs ; ++i)
              {
                sorting.ordered_list [i] =
                  (sort_specs->Specs->SortDirection == ImGuiSortDirection_Ascending ?
                      i : (unsigned int)num_achvs - i - 1);
              }
              break;
            case 2:
            {
              std::vector <SK_Achievement *>
                       sorted_achievements (num_achvs);
              for (unsigned int i = 0 ; i < num_achvs ; ++i)
              {
                sorted_achievements [i] = achievements [i];
              }

              if (sort_specs->Specs->SortDirection == ImGuiSortDirection_Ascending)
              {
                std::sort ( std::begin (sorted_achievements),
                            std::end   (sorted_achievements),
                  [](SK_Achievement* a, SK_Achievement* b)
                  {
                    return ( a->global_percent_ > b->global_percent_ );
                  }
                );
              }

              else
              {
                std::sort ( std::begin (sorted_achievements),
                            std::end   (sorted_achievements),
                  [](SK_Achievement* a, SK_Achievement* b)
                  {
                    return ( a->global_percent_ < b->global_percent_ );
                  }
                );
              }

              auto ordered_achievement =
                std::begin (sorting.ordered_list);

              for (auto achievement : sorted_achievements)
              {
                *ordered_achievement++ = achievement->idx_;
              }
            } break;
          }

          last_sort   = sort_specs->Specs->SortDirection;
          last_column = sort_specs->Specs->ColumnIndex;
        }
      }
    }
  }

  bool draw_main_view (bool as_widget = false)
  {
    auto achievement_mgr = SK_Platform_GetAchievementManager ();
    if ( achievement_mgr != nullptr )
    {
      size_t num_achvs = 0;

      auto achievements =
        achievement_mgr->getAchievements (&num_achvs);

      if (num_achvs == 0)
        return false;

      // Refresh global achievement stats
      SK_RunOnce (
        SK_Platform_DownloadGlobalAchievementStats ()
      );

      // Longest text that should reasonably be attempted to display on 1 line.
      max_text_width =
        std::max ( max_text_width,
          ImGui::CalcTextSize (
            "Activated all 10 Mana Abilities of Valor, Mettle, or Optimism.").x
            + ImGui::GetStyle ().FramePadding.x * 2
        );

      std::scoped_lock <std::recursive_mutex>
             list_lock (name_list_s::lock);

      size_t locked_count          = 0;
      float  track_checkbox_offset = 0.0f;

      ImGui::PushStyleColor (ImGuiCol_TableRowBg,    ImVec4 (0.00f, 0.00f, 0.00f, 0.666f));
      ImGui::PushStyleColor (ImGuiCol_TableRowBgAlt, ImVec4 (0.15f, 0.15f, 0.15f, 0.666f));

      auto flags = ImGuiTableFlags_Borders        | ImGuiTableFlags_RowBg          |
                   ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoSavedSettings;

      if (! as_widget)
      {
        flags |= ImGuiTableFlags_Sortable;
      }

      // Allow scrolling while the control panel is open.
      if (SK_ImGui_Active ())
        flags |= ImGuiTableFlags_ScrollY;

      const int column_count =
        ((SK_ImGui_Active () && (! as_widget)) || show_rarity) ? 3 : 2;

      ImVec2 table_size (-FLT_MIN, -FLT_MIN);
      if (! as_widget) // Reserve at least 1/3 the height of the table
      {
        const ImVec2 avail_size =
          ImGui::GetContentRegionAvail ();

        float needed_y_min =
          ((float)num_achvs * ImGui::GetTextLineHeightWithSpacing ()) / 3.0f;

        const float needed_y_max =           needed_y_min * 3.0f;
                    needed_y_min = std::max (needed_y_min, 150.0f);

        const float y_size =
          std::max (
            std::min (avail_size.y, needed_y_max),
                                    needed_y_min
                   );
      
        table_size = ImVec2 (-FLT_MIN, y_size);
      }

      if (ImGui::BeginTable ("Achievements", column_count, flags, table_size))
      {
        const auto column_flags =
          ( ImGuiTableColumnFlags_NoResize  |
            ImGuiTableColumnFlags_NoReorder | ImGuiTableColumnFlags_NoHide );

        if (num_achvs > 0)
        {
          if (SK_ImGui_Active ())
          {
            // Table headers
            ImGui::TableSetupScrollFreeze( column_count,
                                            2  );
          }
        }

        ImGui::TableSetupColumn   ("Name",        column_flags);
        ImGui::TableSetupColumn   ("Description", column_flags | ImGuiTableColumnFlags_WidthStretch);
        if (column_count == 3)
          ImGui::TableSetupColumn ("Rarity",      column_flags);

        if (num_achvs > 0)
        {
          if (SK_ImGui_Active ())
          {
            if (! as_widget)
            {
              sort_table_if_needed (num_achvs, achievements);
            }

            // Table headers
            ImGui::TableHeadersRow       (     );
            ImGui::TableNextRow          (     );
            ImGui::TableSetColumnIndex   (  0  );
            ImGui::Separator             (     );
            ImGui::TableSetColumnIndex   (  1  );
            ImGui::Separator             (     );
            if (column_count == 3)
            {
              ImGui::TableSetColumnIndex (  2  );
              ImGui::Separator           (     );
            }
            ImGui::TableSetColumnIndex   (  0  );
          }
        }

        const DWORD _ProgressIntervalInMs =
          static_cast <DWORD> (
            (config.platform.achievements.tracker_flash_seconds + flash_duration) * 1000.0f
          );

        const auto oldest_progression =
          SK::ControlPanel::current_time - _ProgressIntervalInMs;

        bool has_recent_progress_update = false;

        // If the widget is flashed, check if there are any recently progressed
        //   achievements; if so, we will only draw them...
        if (isFlashed () && (! SK_ImGui_Active ()))
        {
          for ( auto i : sorting.ordered_list )
          {
            const auto achievement =
              achievements [i];

            if (achievement->progress_.last_update_ms > oldest_progression)
            {
              has_recent_progress_update = true;
              break;
            }
          }
        }

        // Populate rows (Locked)
        for ( auto i : sorting.ordered_list )
        {
          const auto achievement =
            achievements [i];

          if (! achievement->unlocked_)
            ++locked_count;

          if (show_hidden || (! achievement->hidden_))
          {
            // Skip achievements without recent progress
            if (has_recent_progress_update && ((! achievement->tracked_) || achievement->progress_.last_update_ms < oldest_progression))
              continue;

            ImGui::PushID (achievement->name_.c_str ());

            if ((ignored.show || (! achievement->ignored_)) && (! achievement->unlocked_) && ((! as_widget) || achievement->tracked_))
            {
              if ((achievement->tracked_ && tracked.show) || (SK_ImGui_Active () &&
                ((!achievement->ignored_)|| ignored.show)))
              {
                const auto& state =
                  achievement->unlocked_ ? achievement->text_.unlocked :
                  achievement->hidden_   ? achievement->text_.unlocked :
                                           achievement->text_.  locked;

                ImGui::TableNextRow        ( );
                ImGui::TableSetColumnIndex (0);

                if (SK_ImGui_Active () && (! as_widget))
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

                  ImGui::SetItemTooltip ( (achievement->progress_.max > 1) ?
                    "Display notifications whenever achievement progression occurs."
                      :
                    "Non-progression achievements will appear if the Achievement Tracker is enabled "
                    "or requested using its Widget Flash Key Binding."
                  );

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

                if (column_count == 3)
                {
                  ImGui::TableSetColumnIndex  (                 2);
                  draw_rarity_text            (achievement       );
                }
              }
            }

            ImGui::PopID ();
          }
        }

        // Now for already unlocked achievements...
        //
        //   Do not display these unless the control panel is open,
        //     because there is nothing to actually track.
        if (locked_count != num_achvs && SK_ImGui_Active () && (! as_widget))
        { 
          // Do not fill the widget with a list of unlocked achievements...
          ImGui::TableNextRow           (     );
          ImGui::TableSetColumnIndex    (  0  );
          ImGui::Separator              (     );
          ImGui::TableSetColumnIndex    (  1  );
          ImGui::Separator              (     );

          if (column_count == 3)
          {
            ImGui::TableSetColumnIndex  (  2  );
            ImGui::Separator            (     );
          }

          // Populate rows (Unlocked)
          for ( auto i : sorting.ordered_list )
          {
            const auto achievement =
              achievements [i];

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

              if (column_count == 3)
              {
                ImGui::TableSetColumnIndex  (                 2);
                draw_rarity_text            (achievement       );
              }
            }

            ImGui::PopID ();
          }
        }

        ImGui::EndTable ();
      }

      ImGui::PopStyleColor (2);

      return true;
    }

    return false;
  }

  void draw_list_config_footer (void)
  {
    const char* szLabel =
      show_hidden ? "Show Hidden ( " ICON_FA_EYE       " )###ShowHidden"
                  : "Show Hidden ( " ICON_FA_EYE_SLASH " )###ShowHidden";

    if (ImGui::Checkbox (szLabel, &show_hidden))
    {
        show_hidden_pref->store (show_hidden);
      config.utility.save_async (           );
    }

    if (ImGui::IsItemClicked (ImGuiMouseButton_Right))
    {
      cheat_mode = true;
    }

    ImGui::SetItemTooltip (
      "Shows achievements that the developer has marked "
      "hidden until unlocked."
    );

    ImGui::SameLine ();

    if (ImGui::Button (ICON_FA_CLIPBOARD_LIST " Achievement Tracker"))
    {
      setVisible (! visible);
    }

    ImGui::SetItemTooltip (
      "Right-click the tracker to configure it."
    );

    ImGui::SameLine    ();
    ImGui::SeparatorEx (ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine    ();

    static char          szSearchURL [512] = {};
    SK_RunOnce (sprintf (szSearchURL, "%ws", search_url.c_str ()));

    ImGui::BeginGroup       ();
    ImGui::TextUnformatted  ("Achievement Search URL");
    ImGui::SetNextItemWidth (-FLT_MIN);
    ImGui::SameLine         ();

    if (ImGui::InputText ("##Achievement Search URL", szSearchURL, 512))
    {
      search_url =
        SK_UTF8ToWideChar (szSearchURL);

         search_url_pref->store (search_url);
      config.utility.save_async (          );
    }

    ImGui::EndGroup         ();
    ImGui::SetItemTooltip   (
      "Clicking on an achievement will use this URL to do a web search.\n\n"
      " " ICON_FA_INFO_CIRCLE " Clear the text to disable this feature."
    );
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
  bool  cheat_mode  = false;
  DWORD last_update = 0UL;

  bool Selectable (const char* label)
  {
    bool orig_selected = false;

    const ImVec2 label_size =
      ImGui::CalcTextSize (label, nullptr, false, 0.0f);

    return
      ImGui::Selectable ( label, &orig_selected,
                            ImGuiSelectableFlags_DontClosePopups,
                              ImVec2 (0, label_size.y) );
  }

  void
  draw_achievement_name ( const SK_Achievement*                  achievement,
                          const SK_Achievement::text_s::state_s& state )
  {
    ImGui::BeginGroup  ();
    ImGui::TextColored ( ImColor (1.0f, 1.0f, 1.0f, 1.0f),
                            achievement->unlocked_ ?
                                    ICON_FA_UNLOCK : ICON_FA_LOCK );
    ImGui::SameLine    ();

    if (! search_url.empty ())
    {
      if (Selectable (state.human_name.c_str ()))
      {
        std::string url =
          SK_FormatString (
            "%ws %hs Achievement in %hs",
                    search_url.c_str (),
              state.human_name.c_str (), SK_GetFriendlyAppName ().c_str ()
          );

        SK_PlatformOverlay_GoToURL (url.c_str (), true);
      }
    }

    else
    {
      ImGui::TextUnformatted (state.human_name.c_str ());
    }

    if (achievement->hidden_)
    {
      ImGui::SameLine        ();
      ImGui::TextUnformatted (ICON_FA_EYE " ");
    }

    if (achievement->unlocked_)
    {
      ImGui::PushStyleColor (ImGuiCol_Text,        ImVec4 (0.7f, 0.7f, 0.7f, 1.f));
      ImGui::Text           (ICON_FA_CLOCK " %hs", _ctime64 (&achievement->time_));
      ImGui::PopStyleColor  ();
    }
    ImGui::EndGroup ();

    //
    // Special Right-Click Context Menu for Unlocking/Resetting Achievements
    //
    //   * Not offered on Epic, since there is no API to Reset Achievements.
    //
    struct {
      const char* label    = "";
      int  achievement_ctx =     0;
      bool confirming      = false;
    } static locked_popup { "AchievementCtxMenu_Locked"   },
           unlocked_popup { "AchievementCtxMenu_Unlocked" };

    if ( SK::SteamAPI::GetCallbacksRun () > 0 ||
         SK::  Galaxy::GetTicksRetired () > 0 )
    {
      if (ImGui::IsItemClicked (ImGuiMouseButton_Right))
      {
        if (achievement->unlocked_)
        {
          unlocked_popup.achievement_ctx = achievement->idx_;

          ImGui::OpenPopup (unlocked_popup.label);
        }

        // Do not expose the ability to unlock arbitrary achievements
        //   by default; it is intended for debugging, not cheating.
        else if (cheat_mode)
        {
          locked_popup.achievement_ctx = achievement->idx_;

          ImGui::OpenPopup (locked_popup.label);
        }
      }
    }

    void SK_Steam_UnlockAchievement  (const char* szName);
    void SK_Steam_ClearAchievement   (const char* szName);
    void SK_Galaxy_UnlockAchievement (const char* szName);
    void SK_Galaxy_ClearAchievement  (const char* szName);

    if (achievement->idx_ == unlocked_popup.achievement_ctx)
    {
      if (ImGui::BeginPopup (unlocked_popup.label))
      {
        if (unlocked_popup.confirming)
        {
          static const char* szConfirm    = " Confirm Reset? ";
          static const char* szDisclaimer =
            "\n This will lock the achievement and cause permanent loss of original unlock time. \n\n";

          ImGui::TextColored (ImColor::HSV (0.075f, 1.0f, 1.0f), "%hs", szDisclaimer);
          ImGui::Separator   ();
          ImGui::TextColored (ImColor::HSV (0.15f, 1.0f, 1.0f),  "%hs", szConfirm);
          ImGui::SameLine    ();
          ImGui::Spacing     ();
          ImGui::SameLine    ();

          if (ImGui::Button  ("Okay"))
          {
            unlocked_popup.confirming = false;
            ImGui::CloseCurrentPopup ();

            SK_Steam_ClearAchievement  (achievement->name_.c_str ());
            SK_Galaxy_ClearAchievement (achievement->name_.c_str ());
          }

          ImGui::SameLine    ();

          if (ImGui::Button  ("Cancel"))
          {
            unlocked_popup.confirming = false;
          }

          ImGui::SameLine        ();
          ImGui::TextUnformatted (achievement->text_.unlocked.human_name.c_str ());
        }

        else
        {
          if (ImGui::Button ("Reset Achievement"))
          {
            unlocked_popup.confirming = true;
          }

          ImGui::SameLine        ();
          ImGui::TextUnformatted (achievement->text_.unlocked.human_name.c_str ());
        }

        ImGui::EndPopup ();
      }
    }

    if (achievement->idx_ == locked_popup.achievement_ctx)
    {
      if (ImGui::BeginPopup (locked_popup.label))
      {
        if (locked_popup.confirming)
        {
          static const char* szConfirm    = " Confirm Unlock? ";
          static const char* szDisclaimer =
            "\n You can reset it after unlocking, usually with no negative consequences. \n\n";

          ImGui::TextColored (ImColor::HSV (0.075f, 1.0f, 1.0f), "%hs", szDisclaimer);
          ImGui::Separator   ();
          ImGui::TextColored (ImColor::HSV (0.15f, 1.0f, 1.0f),  "%hs", szConfirm);
          ImGui::SameLine    ();
          ImGui::Spacing     ();
          ImGui::SameLine    ();

          if (ImGui::Button  ("Okay"))
          {
            locked_popup.confirming = false;
            ImGui::CloseCurrentPopup ();

            SK_Steam_UnlockAchievement  (achievement->name_.c_str ());
            SK_Galaxy_UnlockAchievement (achievement->name_.c_str ());
          }

          ImGui::SameLine    ();

          if (ImGui::Button  ("Cancel"))
          {
            locked_popup.confirming = false;
          }

          ImGui::SameLine        ();
          ImGui::TextUnformatted (achievement->text_.locked.human_name.c_str ());
        }

        else
        {
          if (ImGui::Button ("Unlock Achievement"))
          {
            locked_popup.confirming = true;
          }

          ImGui::SameLine        ();
          ImGui::TextUnformatted (achievement->text_.locked.human_name.c_str ());
        }

        ImGui::EndPopup ();
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


    // Add padding to deal with ImGui columns being truncated by 1 character per-line
    char* desc =
      (char *)SK_TLS_Bottom ()->scratch_memory->log.formatted_output.alloc (state.desc.length () * 4);

    char* desc_iter = desc;

    for ( auto& ch : state.desc )
    {
      if (ch == '\n') {
        *desc_iter++ = ' '; *desc_iter++ = ' ';
        *desc_iter++ = ' '; *desc_iter++ = ' ';
      }

      *desc_iter++ = ch;
    }

    *desc_iter++ = ' '; *desc_iter++ = ' ';
    *desc_iter++ = ' '; *desc_iter++ = ' ';
    *desc_iter++ = '\0';


    if ( achievement->progress_.max != achievement->progress_.current &&
         achievement->progress_.max != 1                              &&
         ! state.desc.empty () )
    {
      std::string str_progress =
        SK_FormatString ( "%hs  %.0f%%  [%d / %d]",           desc,
                              achievement->progress_.getPercent (),
                              achievement->progress_.current,
                              achievement->progress_.max );
      ImGui::ProgressBar (    achievement->progress_.getPercent () / 100.0F,
                                                       ImVec2 (max_text_width, 0),
                                       str_progress.c_str () );

      // The entire description text should fit within the progress bar,
      //   expand the size of a column if necessary...
      max_text_width =
        std::max ( max_text_width,
          ImGui::CalcTextSize (str_progress.c_str ()).x + ImGui::GetStyle ().FramePadding.x * 2
        );
    }
    
    else if (! state.desc.empty ())
    {
      const float x_orig =
        ImGui::GetCursorPosX ();

      ImGui::PushTextWrapPos (x_orig + max_text_width - ImGui::CalcTextSize ("  ").x);
      ImGui::TextWrapped     ("%hs",   desc);
      ImGui::PopTextWrapPos  (             );

      max_text_width =
        std::max ( max_text_width,
          (ImGui::GetCursorPosX () - x_orig) + ImGui::GetStyle ().FramePadding.x
        );
    }
    ImGui::EndGroup ();
  }

  void
  draw_rarity_text (const SK_Achievement* achievement)
  {
    if (! (SK_ImGui_Active () || show_rarity))
      return;

    extern const char* SK_Achievement_RarityToName (float percent);

    ImVec4 color =
      ImColor::HSV (0.4f * (achievement->global_percent_ / 100.0f), 1.0f, 1.0f);

    const char* szRarity =
      SK_Achievement_RarityToName (achievement->global_percent_);

    const ImVec2 label_size =
      ImGui::CalcTextSize (szRarity, nullptr, false, 0.0f);

    ImGui::BeginGroup       ();
    ImGui::SetNextItemWidth (label_size.x);
    ImGui::TextColored      (color, "%hs", szRarity);
    ImGui::SameLine         ();
    ImGui::Spacing          ();
    ImGui::EndGroup         ();
    
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

    config.utility.save_async ();
  }

  void save_ignored_list (void)
  {
    std::wstring ignored_names =
      serialize (ignored.names);
    
    if (ignored.ini_pref != nullptr)
        ignored.ini_pref->store (ignored_names);
    
    config.utility.save_async ();
  }

protected:
  const DWORD           update_freq      = 250UL;
  bool                  show_icons       = false; // Unused for now
  float                 max_text_width   = 150.0f;

  sk::ParameterBool*    show_hidden_pref = nullptr;
  bool                  show_hidden      = false;

  sk::ParameterStringW* search_url_pref  = nullptr;
  std::wstring          search_url       = L"https://duckduckgo.com/?q=!trueachievements";

  sk::ParameterBool*    show_rarity_pref = nullptr;
  bool                  show_rarity      = true;

  struct name_list_s {
    static std::recursive_mutex lock;
    bool                        show;
    sk::ParameterStringW*       ini_pref;
    std::set <std::string>      names;
  } tracked { true,  nullptr, {} },
    ignored { false, nullptr, {} };

  struct sorting_s {
    std::vector <unsigned int>  ordered_list;
  } sorting;
};
SK_LazyGlobal <SKWG_AchievementTracker> __achievement_tracker__;

SK_Widget* SK_Widget_GetAchievementTracker (void)
{
  return
    __achievement_tracker__.getPtr ();
}

void SK_Widget_InitAchieveTracker (void)
{
  SK_RunOnce (__achievement_tracker__.getPtr ());
}

void SK_ImGui_DrawAchievementList (void)
{
  SKWG_AchievementTracker* tracker =
    (SKWG_AchievementTracker *)SK_Widget_GetAchievementTracker ();

  if (! tracker)
    return;

  ImGui::PushID (&tracker);

  if (tracker->draw_main_view ())
  {
    ImGui::Separator                 ();
    tracker->draw_list_config_footer ();
  }

  ImGui::PopID ();
}

std::recursive_mutex SKWG_AchievementTracker::name_list_s::lock;