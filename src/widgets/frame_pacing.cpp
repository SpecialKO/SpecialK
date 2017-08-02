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
#include <SpecialK/widgets/widget.h>
#include <SpecialK/parameter.h>
#include <SpecialK/config.h>

extern iSK_INI* osd_ini;

extern void SK_ImGui_DrawGraph_FramePacing (void);

class SKWG_FramePacing : public SK_Widget
{
public:
  SKWG_FramePacing (void) : SK_Widget ("###Widget_FramePacing")
  {
    setResizable    (                false).setAutoFit      (true).setMovable (false).
    setDockingPoint (DockAnchor::SouthEast).setClickThrough (true);

    SK_ImGui_Widgets.frame_pacing = this;
  };

  void run (void)
  {
    static bool first = true;

    if (first)
    {
      toggle_key_val =
        LoadWidgetKeybind ( &toggle_key, osd_ini,
                              L"Widget Toggle Keybinding (FramePacing)",
                                L"Widget.FramePacing",
                                  L"ToggleKey" );

      param_visible =
        LoadWidgetBool ( &visible, osd_ini,
                           L"Widget Visible (FramePacing)",
                             L"Widget.FramePacing",
                               L"Visible" );

      param_movable =
        LoadWidgetBool ( &movable, osd_ini,
                           L"Widget Movable (FramePacing)",
                             L"Widget.FramePacing",
                               L"Movable" );

      param_autofit =
        LoadWidgetBool ( &autofit, osd_ini,
                           L"Widget AutoFitted (FramePacing)",
                             L"Widget.FramePacing",
                               L"AutoFit" );

      param_clickthrough =
        LoadWidgetBool ( &click_through, osd_ini,
                           L"Widget Ignores Clicks (FramePacing)",
                             L"Widget.FramePacing",
                               L"ClickThrough" );

      param_docking =
        LoadWidgetDocking ( &docking, osd_ini,
                              L"Widget Docks to ... (FramePacing)",
                                L"Widget.FramePacing",
                                  L"DockingPoint" );

      //sk::ParameterVec2f* param_minsize;
      //sk::ParameterVec2f* param_maxsize;
      //sk::ParameterVec2f* param_size;

      //sk::ParameterInt*   param_docking;
      //sk::ParameterFloat* param_scale;

      first = false;

      return;
    }

    ImGuiIO& io (ImGui::GetIO ());

    const  float font_size           =             ImGui::GetFont  ()->FontSize                        * io.FontGlobalScale;
    const  float font_size_multiline = font_size + ImGui::GetStyle ().ItemSpacing.y + ImGui::GetStyle ().ItemInnerSpacing.y;

    ImVec2 new_size (font_size * 35, font_size_multiline * 5.44f);
    setSize (new_size);

    if (isVisible ())
      ImGui::SetNextWindowSize (new_size, ImGuiSetCond_Always);
  }

  void draw (void)
  {
    static bool move = true;
    
    if (move)
    {
      ImGuiIO& io (ImGui::GetIO ());

      const  float font_size           =             ImGui::GetFont  ()->FontSize                        * io.FontGlobalScale;
      const  float font_size_multiline = font_size + ImGui::GetStyle ().ItemSpacing.y + ImGui::GetStyle ().ItemInnerSpacing.y;

      ImGui::SetWindowPos (ImVec2 (io.DisplaySize.x - getSize ().x, io.DisplaySize.y - getSize ().y ));
      move = false;
    }

    SK_ImGui_DrawGraph_FramePacing ();
  }

  void OnConfig (ConfigEvent event)
  {
    switch (event)
    {
      case SK_Widget::ConfigEvent::LoadComplete:
        break;

      case SK_Widget::ConfigEvent::SaveStart:
        break;
    }
  }
} __frame_pacing__;