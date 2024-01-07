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

#include <SpecialK/control_panel/window.h>
#include <imgui/font_awesome.h>

bool
SK::ControlPanel::Window::Draw (void)
{
  static auto& rb =
    SK_GetCurrentRenderBackend ();

  if ( ImGui::CollapsingHeader ("Window Management") )
  {
    ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.02f, 0.68f, 0.90f, 0.45f));
    ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.07f, 0.72f, 0.90f, 0.80f));
    ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.14f, 0.78f, 0.87f, 0.80f));
    ImGui::TreePush       ("");

    if ((! rb.fullscreen_exclusive) && ImGui::CollapsingHeader ("Style and Position", ImGuiTreeNodeFlags_DefaultOpen))
    {
      ImGui::TreePush ("");

      bool borderless        = config.window.borderless;
      bool center            = config.window.center;
      bool fullscreen        = config.window.fullscreen;

      if ( ImGui::Checkbox ( "Borderless  ", &borderless ) )
      {
          //config.window.fullscreen = false;

        if (! config.window.fullscreen)
          SK_DeferCommand ("Window.Borderless toggle");
      }

      if (ImGui::IsItemHovered ())
      {
        if (! config.window.fullscreen)
        {
          ImGui::BeginTooltip ();
          if (borderless)
            ImGui::TextUnformatted ("Add/Restore Window Borders");
          else
            ImGui::TextUnformatted ("Remove Window Borders");
          ImGui::Separator  ();
          ImGui::BulletText ("Game needs to be set to its Regular Windowed mode for these overrides to work correctly.");
          ImGui::EndTooltip ();
        }

        else
          ImGui::SetTooltip ("Cannot be Changed while Fullscreen is Checked");
      }

      if (borderless)
      {
        ImGui::SameLine ();

        if ( ImGui::Checkbox ( "Fullscreen Borderless", &fullscreen ) )
        {
          config.window.fullscreen = fullscreen;
          SK_ImGui_AdjustCursor ();
        }

        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip ();
          ImGui::Text         ("Stretch Borderless Window to Fill Monitor");
          //ImGui::Separator    ();
          //ImGui::BulletText   ("Framebuffer Resolution Unchanged (GPU-Side Upscaling)");
          //ImGui::BulletText   ("Upscaling to Match Desktop Resolution Adds AT LEAST 1 Frame of Input Latency!");
          ImGui::EndTooltip   ();
        }
      }

      if (! config.window.fullscreen)
      {
        ImGui::InputInt2 ("Override Resolution", (int *)&config.window.res.override.x);

        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip ();
          ImGui::Text         ("Set if Game's Window Resolution is Reported Wrong");
          ImGui::Separator    ();
          ImGui::BulletText   ("0x0 = Disable");
          ImGui::BulletText   ("Applied the next time a Style/Position setting is changed");
          ImGui::EndTooltip   ();
        }
      }

      if (! (config.window.borderless && config.window.fullscreen))
      {
        if ( ImGui::Checkbox ( "Center", &center ) ) {
          config.window.center = center;
          SK_ImGui_AdjustCursor ();
          //SK_DeferCommand ("Window.Center toggle");
        }

        if (ImGui::IsItemHovered ()) {
          ImGui::BeginTooltip ();
          ImGui::Text         ("Keep the Render Window Centered at All Times");
          ImGui::Separator    ();
          ImGui::BulletText   ( "The Drag-Lock feature cannot be used while Window "
                                "Centering is turned on." );
          ImGui::EndTooltip   ();
        }

        ImGui::SameLine ();

        if (ImGui::Checkbox ("Multi-Monitor Mode", &config.window.multi_monitor_mode))
        {
          config.window.center = false;
          SK_ImGui_AdjustCursor ();
        }

        if (ImGui::IsItemHovered ()) {
          ImGui::BeginTooltip ();
          ImGui::Text         ("Allows Resolution Overrides that Span Multiple Monitors");
          ImGui::Separator    ();
          ImGui::BulletText   ("Fullscreen and Center Modes cannot be used in Multi-Monitor Mode");
          ImGui::BulletText   ("This may introduce performance penalties, pay attention to Presentation Mode");
          ImGui::EndTooltip   ();
        }

        if (! (config.window.center || config.window.multi_monitor_mode))
        {
          ImGui::TreePush    ("");
          ImGui::TextColored (ImVec4 (1.0f, 1.0f, 0.0f, 1.0f), "\nPress Ctrl + Shift + ScrollLock to Toggle Drag-Lock Mode");
          ImGui::BulletText  ("Useful for Positioning Borderless Windows.");
          ImGui::Text        ("");
          ImGui::TreePop     ();
        }

        bool pixel_perfect = ( config.window.offset.x.percent == 0.0 &&
                               config.window.offset.y.percent == 0.0 );

        if (ImGui::Checkbox ("Pixel-Aligned Placement", &pixel_perfect))
        {
          if (pixel_perfect) {
            config.window.offset.x.absolute = 0;
            config.window.offset.y.absolute = 0;
            config.window.offset.x.percent  = 0.0f;
            config.window.offset.y.percent  = 0.0f;
          }

          else {
            config.window.offset.x.percent  = 0.000001f;
            config.window.offset.y.percent  = 0.000001f;
            config.window.offset.x.absolute = 0;
            config.window.offset.y.absolute = 0;
          }
        }

        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ( "Pixel-Aligned Placement behaves inconsistently "
                              "when Desktop Resolution changes");

        if (! config.window.center)
        {
          ImGui::SameLine ();

          ImGui::Checkbox   ("Remember Dragged Position", &config.window.persistent_drag);

          if (ImGui::IsItemHovered ())
            ImGui::SetTooltip ( "Store the location of windows moved using "
                                "Drag-Lock and apply at game startup" );
        }

        bool moved = false;

        HMONITOR hMonitor =
          MonitorFromWindow ( rb.windows.device,
                                /*config.display.monitor_default*/ MONITOR_DEFAULTTONEAREST );

        MONITORINFO mi  = { };
        mi.cbSize       = sizeof (mi);
        GetMonitorInfo (hMonitor, &mi);

        if (pixel_perfect)
        {
          int  x_pos        = std::abs (config.window.offset.x.absolute);
          int  y_pos        = std::abs (config.window.offset.y.absolute);

          bool right_align  = config.window.offset.x.absolute < 0;
          bool bottom_align = config.window.offset.y.absolute < 0;

          float extent_x    = sk::narrow_cast <float> (mi.rcMonitor.right  - mi.rcMonitor.left) / 2.0f + 1.0f;
          float extent_y    = sk::narrow_cast <float> (mi.rcMonitor.bottom - mi.rcMonitor.top)  / 2.0f + 1.0f;

          if (config.window.center) {
            extent_x /= 2.0f;
            extent_y /= 2.0;
          }

          // Do NOT Apply Immediately or the Window Will Oscillate While
          //   Adjusting the Slider
          static bool queue_move = false;

          float fx_pos =
            sk::narrow_cast <float> (x_pos),
                fy_pos =
            sk::narrow_cast <float> (y_pos);

          moved  = ImGui::SliderFloat ("X Offset##WindowPix",       &fx_pos, 0.0f, extent_x, "%.0f pixels"); ImGui::SameLine ();
          moved |= ImGui::Checkbox    ("Right-aligned##WindowPix",  &right_align);
          moved |= ImGui::SliderFloat ("Y Offset##WindowPix",       &fy_pos, 0.0f, extent_y, "%.0f pixels"); ImGui::SameLine ();
          moved |= ImGui::Checkbox    ("Bottom-aligned##WindowPix", &bottom_align);

          if (moved)
          {
            x_pos = sk::narrow_cast <int> (fx_pos);
            y_pos = sk::narrow_cast <int> (fy_pos);

            queue_move = true;
          }

          // We need to set pixel offset to 1 to do what the user expects
          //   these values to do... 0 = NO OFFSET, but the slider may move
          //     from right-to-left skipping 1.
          static bool reset_x_to_zero = false;
          static bool reset_y_to_zero = false;

          if (moved)
          {
            config.window.offset.x.absolute = x_pos * (right_align  ? -1 : 1);
            config.window.offset.y.absolute = y_pos * (bottom_align ? -1 : 1);

            if (right_align && config.window.offset.x.absolute >= 0)
              config.window.offset.x.absolute = -1;

            if (bottom_align && config.window.offset.y.absolute >= 0)
              config.window.offset.y.absolute = -1;

            if (config.window.offset.x.absolute == 0)
            {
              config.window.offset.x.absolute = 1;
              reset_x_to_zero = true;
            }

            if (config.window.offset.y.absolute == 0)
            {
              config.window.offset.y.absolute = 1;
              reset_y_to_zero = true;
            }
          }

          if (queue_move && (! ImGui::IsMouseDown (0)))
          {
            queue_move = false;

            SK_ImGui_AdjustCursor ();

            if (reset_x_to_zero) config.window.offset.x.absolute = 0;
            if (reset_y_to_zero) config.window.offset.y.absolute = 0;

            if (reset_x_to_zero || reset_y_to_zero)
              SK_ImGui_AdjustCursor ();

            reset_x_to_zero = false; reset_y_to_zero = false;
          }
        }

        else
        {
          float x_pos = std::abs (config.window.offset.x.percent);
          float y_pos = std::abs (config.window.offset.y.percent);

          x_pos *= 100.0f;
          y_pos *= 100.0f;

          bool right_align  = config.window.offset.x.percent < 0.0f;
          bool bottom_align = config.window.offset.y.percent < 0.0f;

          float extent_x = 50.05f;
          float extent_y = 50.05f;

          if (config.window.center) {
            extent_x /= 2.0f;
            extent_y /= 2.0f;
          }

          // Do NOT Apply Immediately or the Window Will Oscillate While
          //   Adjusting the Slider
          static bool queue_move = false;

          moved  = ImGui::SliderFloat ("X Offset##WindowRel",       &x_pos, 0.0f, extent_x, "%.3f %%"); ImGui::SameLine ();
          moved |= ImGui::Checkbox    ("Right-aligned##WindowRel",  &right_align);
          moved |= ImGui::SliderFloat ("Y Offset##WindowRel",       &y_pos, 0.0f, extent_y, "%.3f %%"); ImGui::SameLine ();
          moved |= ImGui::Checkbox    ("Bottom-aligned##WindowRel", &bottom_align);

          // We need to set pixel offset to 1 to do what the user expects
          //   these values to do... 0 = NO OFFSET, but the slider may move
          //     from right-to-left skipping 1.
          static bool reset_x_to_zero = false;
          static bool reset_y_to_zero = false;

          if (moved)
          {
            queue_move = true;

            x_pos /= 100.0f;
            y_pos /= 100.0f;

            config.window.offset.x.percent = x_pos * (right_align  ? -1.0f : 1.0f);
            config.window.offset.y.percent = y_pos * (bottom_align ? -1.0f : 1.0f);

            if (right_align && config.window.offset.x.percent >= 0.0f)
              config.window.offset.x.percent = -0.01f;

            if (bottom_align && config.window.offset.y.percent >= 0.0f)
              config.window.offset.y.percent = -0.01f;

            if ( config.window.offset.x.percent <  0.000001f &&
                 config.window.offset.x.percent > -0.000001f )
            {
              config.window.offset.x.absolute = 1;
              reset_x_to_zero = true;
            }

            if ( config.window.offset.y.percent <  0.000001f &&
                 config.window.offset.y.percent > -0.000001f )
            {
              config.window.offset.y.absolute = 1;
              reset_y_to_zero = true;
            }
          }

          if (queue_move && (! ImGui::IsMouseDown (0)))
          {
            queue_move = false;

            SK_ImGui_AdjustCursor ();

            if (reset_x_to_zero) config.window.offset.x.absolute = 0;
            if (reset_y_to_zero) config.window.offset.y.absolute = 0;

            if (reset_x_to_zero || reset_y_to_zero)
              SK_ImGui_AdjustCursor ();

            reset_x_to_zero = false; reset_y_to_zero = false;
          }
        }
      }

      ImGui::Text     ("Window Layering");
      ImGui::TreePush ("");

      bool changed = false;

      changed |= ImGui::RadioButton ("No Preference",         &config.window.always_on_top,  NoPreferenceOnTop); ImGui::SameLine ();
      changed |= ImGui::RadioButton ("Prevent Always-On-Top", &config.window.always_on_top, PreventAlwaysOnTop); ImGui::SameLine ();
      changed |= ImGui::RadioButton ("Force Always-On-Top",   &config.window.always_on_top,        AlwaysOnTop); ImGui::SameLine ();
      changed |= ImGui::RadioButton ("Multitasking-On-Top",   &config.window.always_on_top,   SmartAlwaysOnTop);

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip ();
        ImGui::Text         ("Intelligently Raises and Lowers the Game Window for Optimum Multitasking");
        ImGui::Separator    ();
        ImGui::BulletText   ("Improves framepacing when KB&M input is given to other applications");
        ImGui::BulletText   ("Enables G-Sync / FreeSync / VRR in overlapping multi-monitor scenarios");
        ImGui::Separator    ();
        if (! config.window.background_render)
          ImGui::Text       (ICON_FA_INFO_CIRCLE " Enable 'Continue Rendering' mode for Ultra-tasking");
        ImGui::Text         (ICON_FA_EXCLAMATION_TRIANGLE " Advanced feature: Leave Global Injection running to raise windows dragged over the game");
        ImGui::EndTooltip   ();
      }

      if (changed)
      {
        switch (config.window.always_on_top)
        {
          case AlwaysOnTop:
          case SmartAlwaysOnTop: // Window is in foreground if user is interacting with it
            SK_DeferCommand ("Window.TopMost true");
            break;
          case PreventAlwaysOnTop:
            SK_DeferCommand ("Window.TopMost false");
            break;
          default:
            break;
        }
      }

      ImGui::TreePop ();
      ImGui::TreePop ();
    }

    if (ImGui::CollapsingHeader ("Input/Output Behavior", ImGuiTreeNodeFlags_DefaultOpen))
    {
      ImGui::TreePush ("");

      bool background_render = config.window.background_render;
      bool background_mute   = config.window.background_mute;

      ImGui::Text     ("Background Behavior");
      ImGui::TreePush ("");

      if ( ImGui::Checkbox ( "Mute Game ", &background_mute ) )
        SK_DeferCommand    ("Window.BackgroundMute toggle");

      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("Mute the Game when Another Window has Input Focus");

      if (! rb.fullscreen_exclusive)
      {
        ImGui::SameLine ();

        if ( ImGui::Checkbox ( "Continue Rendering", &background_render ) )
          SK_DeferCommand    ("Window.BackgroundRender toggle");

        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip ();
          ImGui::Text         ("Block Application Switch Notifications to the Game");
          ImGui::Separator    ();
          ImGui::BulletText   ("Most Games will Continue Rendering");
          ImGui::BulletText   ("Disables a Game's Built-in Mute-on-Alt+Tab Functionality");
          ImGui::BulletText   ("See \"Input Management | Enable / Disable Devices\" to Configure Background Behavior");

          if (config.input.gamepad.steam.is_native && config.steam.appid != 0)
          {
            ImGui::Separator   ();
            ImGui::TextColored (ImVec4 (1.f, 1.f, 0.f, 1.f), ICON_FA_EXCLAMATION_TRIANGLE " WARNING:\t");
            ImGui::SameLine    ();
            ImGui::TextColored (ImVec4 (.85f, .85f, .85f, 1.f),
              "This may break input in Steam Input native games (this appears to be one)."
            );
          }
          ImGui::EndTooltip   ();
        }

        ImGui::SameLine    ();
        ImGui::SeparatorEx (ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine    ();

        ImGui::Checkbox ("Treat Foreground as Active", &config.window.treat_fg_as_active);

        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip ();
          ImGui::Text         ("Game Considers the Foreground Window as Active");
          ImGui::Separator    ();
          ImGui::BulletText   ("Try this if \"Continue Rendering\" does not behave as expected");
          ImGui::BulletText   ("Some games check the Foreground Window to determine if they are Active");
          ImGui::BulletText   ("Normal games will track Keyboard Focus and Activation Events instead");
          ImGui::EndTooltip   ();
        }
      }

      ImGui::TreePop ();

      SK_ImGui_CursorBoundaryConfig ();

      ImGui::Checkbox ("Disable Screensaver", &config.window.disable_screensaver);

      ImGui::TreePop ();
    }

    ImGui::TreePop       ( );
    ImGui::PopStyleColor (3);

    return true;
  }

  return false;
}