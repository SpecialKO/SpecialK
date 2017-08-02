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
#define NOMINMAX

#include <SpecialK/widgets/widget.h>
#include <SpecialK/utility.h>

#include <unordered_set>


void
SK_Widget::run_base (void)
{ 
  run ();
}

void
SK_Widget::draw_base (void)
{
  static std::unordered_set <SK_Widget *> initialized;

  if (! initialized.count (this))
  {
    setVisible (visible).setAutoFit      (autofit      ).setResizable (resizable).
    setMovable (movable).setClickThrough (click_through);

    ImGui::SetNextWindowSize (ImVec2 ( std::min ( max_size.x, std::max ( size.x, min_size.x ) ),
                                       std::min ( max_size.y, std::max ( size.y, min_size.y ) ) ) );

    initialized.emplace (this);
  }


  ImGuiIO& io (ImGui::GetIO ());

  int flags = ImGuiWindowFlags_NoTitleBar  | ImGuiWindowFlags_NoCollapse |
              ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoFocusOnAppearing;

  if (autofit)
    flags |= ImGuiWindowFlags_AlwaysAutoResize;

  if (locked || (! movable))
    flags |= ImGuiWindowFlags_NoMove;

  if (locked || (! resizable))
    flags |= ImGuiWindowFlags_NoResize;

  if (click_through && (! SK_ImGui_Visible) && state__ != 1)
    flags |= ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;


  static  std::unordered_set <SK_Widget *> last_state_was_config;

  // Modal State:  Config
  if (state__ == 1)
  {
    flags &= ~(ImGuiWindowFlags_AlwaysAutoResize);
    flags |=  (ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::SetNextWindowSize (ImVec2 (std::max (size.x, 420.0f), std::max (size.y, 190.0f)));

    //last_state_was_config.emplace (this);
  }

  else
  {
    //if (last_state_was_config.count (this))
    //{
    //  last_state_was_config.erase (this);
    //
    //}

    if (! autofit)
    {
      ImGui::SetNextWindowSize (ImVec2 ( std::min ( max_size.x, std::max ( size.x, min_size.x ) ),
                                         std::min ( max_size.y, std::max ( size.y, min_size.y ) ) ) );
    }
  }


  ImGui::Begin           (name.c_str (), nullptr, flags);

  ImGui::PushItemWidth (0.5f * ImGui::GetWindowWidth ());

  pos  = ImGui::GetWindowPos  ();
  size = ImGui::GetWindowSize ();

  bool n = (int)docking & (int)DockAnchor::North,
       s = (int)docking & (int)DockAnchor::South,
       e = (int)docking & (int)DockAnchor::East,
       w = (int)docking & (int)DockAnchor::West;

  if (n || s)
  {
    if (n)
      pos.y = 0.0;

    if (s)
      pos.y = io.DisplaySize.y - size.y;
  }

  if (e || w)
  {
    if (e)
      pos.x = io.DisplaySize.x - size.x;

    if (w)
      pos.x = 0.0;
  }

  if (docking != DockAnchor::None)
  {
    ImGui::SetWindowPos (pos, ImGuiSetCond_Always);
  }


  // Modal State:  Normal drawing
  if (state__ == 0)
  {
    draw ();
  }

  // Modal State:  Config
  else
  {
    config_base ();
  }

  ImGui::PopItemWidth ();


  if (ImGui::IsMouseClicked (1) && ImGui::IsWindowHovered ())
  {
    state__ = 1;
  }



  ImGui::End ();
}

extern void
SK_ImGui_KeybindDialog (SK_Keybind* keybind);

void
SK_Widget::config_base (void)
{
  static bool changed = false;

  ImGuiIO& io (ImGui::GetIO ());

  const  float font_size           =             ImGui::GetFont  ()->FontSize                        * io.FontGlobalScale;
  const  float font_size_multiline = font_size + ImGui::GetStyle ().ItemSpacing.y + ImGui::GetStyle ().ItemInnerSpacing.y;

  bool lock = (! isMovable ());

  if (ImGui::Checkbox ("Lock Position and Scale",  &lock))
  {
    setMovable (! lock);
    changed = true;
  }

  ImGui::SameLine ();
  if (ImGui::Checkbox ("Auto-Fit", &autofit))
  {
    setAutoFit (autofit);
    changed = true;
  }

  ImGui::SameLine ();
  if (ImGui::Checkbox ("Click Through", &click_through))
  {
    setClickThrough (click_through);
    changed = true;
  }

  bool n = (int)docking & (int)DockAnchor::North,
       s = (int)docking & (int)DockAnchor::South,
       e = (int)docking & (int)DockAnchor::East,
       w = (int)docking & (int)DockAnchor::West;

  const char* anchors = "Undocked\0North\0South\0\0";

  int dock = 0;

       if (n) dock = 1;
  else if (s) dock = 2;

  if (ImGui::Combo ("Vertical Docking Anchor", &dock, anchors, 3))
  {
    docking = (SK_Widget::DockAnchor)((int)docking & ~( (int)DockAnchor::North |
                                                        (int)DockAnchor::South ) );

    int mask = (dock == 1 ? (int)DockAnchor::North : 0x0) |
               (dock == 2 ? (int)DockAnchor::South : 0x0);

    docking = (DockAnchor)(mask | (int)docking);
    changed = true;
  }

  anchors = "Undocked\0West\0East\0\0";

  dock = 0;

       if (w) dock = 1;
  else if (e) dock = 2;

  if (ImGui::Combo ("Horizonal Docking Anchor", &dock, anchors, 3))
  {
    docking = (SK_Widget::DockAnchor)((int)docking & ~( (int)DockAnchor::West |
                                                        (int)DockAnchor::East ) );

    int mask = (dock == 1 ? (int)DockAnchor::West : 0x0) |
               (dock == 2 ? (int)DockAnchor::East : 0x0);

    docking = (DockAnchor)(mask | (int)docking);
    changed = true;
  }

  ImGui::Separator ();

  auto Keybinding = [](SK_Keybind* binding, sk::ParameterStringW* param) ->
    auto
    {
      std::string label  = SK_WideCharToUTF8 (binding->human_readable) + "###";
                  label += binding->bind_name;

      if (ImGui::Selectable (label.c_str (), false))
      {
        ImGui::OpenPopup (binding->bind_name);
      }

      std::wstring original_binding = binding->human_readable;

      extern void SK_ImGui_KeybindDialog (SK_Keybind* keybind);
      SK_ImGui_KeybindDialog (binding);

      if (original_binding != binding->human_readable)
      {
        param->set_value (binding->human_readable);
        param->store     ();

        std::wstring osd_config =
          SK_GetDocumentsDir () + L"\\My Mods\\SpecialK\\Global\\osd.ini";

        extern iSK_INI* osd_ini;

        osd_ini->write (osd_config.c_str ());

        return true;
      }

      return false;
    };

  ImGui::Text       ("Key Bindings");
  ImGui::TreePush   ("");

  ImGui::BeginGroup (  );
  if (toggle_key_val != nullptr)
    ImGui::Text       ("Widget Toggle");
  if (focus_key_val != nullptr)
    ImGui::Text       ("Widget Focus");
  ImGui::EndGroup   (  );

  ImGui::SameLine   (  );

  ImGui::BeginGroup (  );
  if (toggle_key_val != nullptr)
    Keybinding        (&toggle_key, toggle_key_val);
  if (focus_key_val != nullptr)
    Keybinding        (&focus_key,  focus_key_val);
  ImGui::EndGroup   (  );

  ImGui::TreePop    (  );

  ImGui::Separator ();

  bool done = false;

  done |= ImGui::Button (" Save ");

  if (done)
  {
    if (changed)
    {
      param_visible->set_value      (visible);
      param_movable->set_value      (movable);
      param_clickthrough->set_value (click_through);
      param_autofit->set_value      (autofit);
      param_docking->set_value      ((int)docking);

      param_visible->store      ();
      param_movable->store      ();
      param_clickthrough->store ();
      param_autofit->store      ();
      param_docking->store      ();

      std::wstring osd_config =
        SK_GetDocumentsDir () + L"\\My Mods\\SpecialK\\Global\\osd.ini";

      extern iSK_INI* osd_ini;

      osd_ini->write (osd_config.c_str ());

      changed = false;
    }

    state__ = 0;
  }
}



void
SK_Widget::load (iSK_INI* config)
{
  UNREFERENCED_PARAMETER (config);

  OnConfig (ConfigEvent::LoadStart);
  // ...
  OnConfig (ConfigEvent::LoadComplete);
}

void
SK_Widget::save (iSK_INI* config)
{
  UNREFERENCED_PARAMETER (config);

  OnConfig (ConfigEvent::SaveStart);
  // ...
  OnConfig (ConfigEvent::SaveComplete);
}


#define SK_MakeKeyMask(vKey,ctrl,shift,alt) \
  (UINT)((vKey) | (((ctrl) != 0) <<  9) |   \
                  (((shift)!= 0) << 10) |   \
                  (((alt)  != 0) << 11))

BOOL
SK_ImGui_WidgetRegistry::DispatchKeybinds (BOOL Control, BOOL Shift, BOOL Alt, BYTE vkCode)
{
  UINT uiMaskedKeyCode =
    SK_MakeKeyMask (vkCode, Control, Shift, Alt);

  static std::array <SK_Widget *, 3> widgets { frame_pacing, volume_control, gpu_monitor };

  for (auto widget : widgets)
  {
    if (uiMaskedKeyCode == widget->getToggleKey ().masked_code)
    {
      widget->setVisible (! widget->isVisible ());

      return TRUE;
    }
  }

  return FALSE;
}

sk::ParameterFactory SK_Widget_ParameterFactory;