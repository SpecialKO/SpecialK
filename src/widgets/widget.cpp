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


extern iSK_INI* osd_ini;


void
SK_Widget::run_base (void)
{
  if (! run_once__)
  {
    run_once__ = true;

    toggle_key_val =
      LoadWidgetKeybind ( &toggle_key, osd_ini,
                            SK_FormatStringW   (L"Widget Toggle Keybinding (%hs)", name.c_str ()).c_str (),
                              SK_FormatStringW (L"Widget.%hs",                     name.c_str ()).c_str (),
                                L"ToggleKey" );

    param_visible =
      LoadWidgetBool ( &visible, osd_ini,
                         SK_FormatStringW   (L"Widget Visible (%hs)", name.c_str ()).c_str (),
                           SK_FormatStringW (L"Widget.%hs",           name.c_str ()).c_str (),
                             L"Visible" );

    param_movable =
      LoadWidgetBool ( &movable, osd_ini,
                         SK_FormatStringW   (L"Widget Movable (%hs)", name.c_str ()).c_str (),
                           SK_FormatStringW (L"Widget.%hs",           name.c_str ()).c_str (),
                             L"Movable" );

    param_resizable =
      LoadWidgetBool ( &resizable, osd_ini,
                         SK_FormatStringW   (L"Widget Resizable (%hs)", name.c_str ()).c_str (),
                           SK_FormatStringW (L"Widget.%hs",             name.c_str ()).c_str (),
                             L"Resizable" );

    param_autofit =
      LoadWidgetBool ( &autofit, osd_ini,
                         SK_FormatStringW   (L"Widget AutoFitted (%hs)", name.c_str ()).c_str (),
                           SK_FormatStringW (L"Widget.%hs",              name.c_str ()).c_str (),
                             L"AutoFit" );

    param_clickthrough =
      LoadWidgetBool ( &click_through, osd_ini,
                         SK_FormatStringW   (L"Widget Ignores Clicks (%hs)", name.c_str ()).c_str (),
                           SK_FormatStringW (L"Widget.%hs",                  name.c_str ()).c_str (),
                             L"ClickThrough" );

    param_docking =
      LoadWidgetDocking ( &docking, osd_ini,
                            SK_FormatStringW   (L"Widget Docks to... (%hs)", name.c_str ()).c_str (),
                              SK_FormatStringW (L"Widget.%hs",               name.c_str ()).c_str (),
                                L"DockingPoint" );

    param_minsize =
      LoadWidgetVec2 ( &min_size, osd_ini,
                         SK_FormatStringW   (L"Widget Min Size (%hs)", name.c_str ()).c_str (),
                           SK_FormatStringW (L"Widget.%hs",            name.c_str ()).c_str (),
                             L"MinSize" );

    param_maxsize =
      LoadWidgetVec2 ( &max_size, osd_ini,
                         SK_FormatStringW   (L"Widget Max Size (%hs)", name.c_str ()).c_str (),
                           SK_FormatStringW (L"Widget.%hs",            name.c_str ()).c_str (),
                             L"MaxSize" );

    param_size =
      LoadWidgetVec2 ( &size, osd_ini,
                         SK_FormatStringW   (L"Widget Size (%hs)", name.c_str ()).c_str (),
                           SK_FormatStringW (L"Widget.%hs",        name.c_str ()).c_str (),
                             L"Size" );

    param_pos =
      LoadWidgetVec2 ( &pos, osd_ini,
                         SK_FormatStringW   (L"Widget Position (%hs)", name.c_str ()).c_str (),
                           SK_FormatStringW (L"Widget.%hs",            name.c_str ()).c_str (),
                             L"Position" );

    return;
  }

  run ();
}

void
SK_Widget_ProcessDocking (SK_Widget* pWidget, bool n, bool s, bool e, bool w)
{
  ImGuiIO& io (ImGui::GetIO ());

  // Docking alignment visualiztion
  bool draw_horz_ruler = false;
  bool draw_vert_ruler = false;

  ImVec2 pos  = pWidget->getPos  ();
  ImVec2 size = pWidget->getSize ();


  if (n || s)
  {
    if (pWidget->isMovable () && ImGui::IsMouseDragging (0) && ImGui::IsWindowHovered ())
    {
      draw_horz_ruler = true;
    }

    if (n)
      pos.y = 0.0;

    if (s)
      pos.y = io.DisplaySize.y - size.y;
  }


  if (e || w)
  {
    if (pWidget->isMovable () && ImGui::IsMouseDragging (0) && ImGui::IsWindowHovered ())
    {
      draw_vert_ruler = true;
    }

    if (e)
      pos.x = io.DisplaySize.x - size.x;

    if (w)
      pos.x = 0.0;
  }


  if (                   size.x > 0 &&                    size.y > 0 &&
      io.DisplaySize.x - size.x > 0 && io.DisplaySize.y - size.y > 0)
  {
    pos.x = std::max (0.0f, std::min (pos.x, io.DisplaySize.x - size.x));
    pos.y = std::max (0.0f, std::min (pos.y, io.DisplaySize.y - size.y));
  }


  if (pWidget->getDockingPoint () != SK_Widget::DockAnchor::None)
  {
        pWidget->setPos (pos);
    ImGui::SetWindowPos (pos, ImGuiSetCond_Always);
  }


  if (draw_horz_ruler ^ draw_vert_ruler)
  {
    ImVec2 horz_pos       = ImVec2 (       e ? io.DisplaySize.x - size.x : size.x, 0.0f );
    ImVec2 vert_pos       = ImVec2 ( 0.0f, s ? io.DisplaySize.y - size.y : size.y       );

    ImVec2 xy0, xy1;

    if (draw_vert_ruler)
    {
      xy0 = ImVec2 ( horz_pos.x, 0.0f             );
      xy1 = ImVec2 ( horz_pos.x, io.DisplaySize.y );
    }

    if (draw_horz_ruler)
    {
      xy0 = ImVec2 ( 0.0f,             vert_pos.y );
      xy1 = ImVec2 ( io.DisplaySize.x, vert_pos.y );
    }

    ImVec4 col = ImColor::HSV ( 0.133333f, 
                                    std::min ( (float)(0.161616f +  (timeGetTime () % 250) / 250.0f -
                                                            floor ( (timeGetTime () % 250) / 250.0f) ), 1.0f),
                                        std::min ( (float)(0.333 +  (timeGetTime () % 500) / 500.0f -
                                                            floor ( (timeGetTime () % 500) / 500.0f) ), 1.0f) );
    const ImU32 col32 =
      ImColor (col);
    
    ImDrawList* draw_list =
      ImGui::GetWindowDrawList ();
    
    draw_list->PushClipRectFullScreen (                                   );
    draw_list->AddRect                ( xy0, xy1, col32, 0.0f, 0x00, 2.5f );
    draw_list->PopClipRect            (                                   );
  }
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
    ImGui::SetNextWindowPos  (pos);

    initialized.emplace (this);
  }


  ImGuiIO& io (ImGui::GetIO ());

  int flags = ImGuiWindowFlags_NoTitleBar  | ImGuiWindowFlags_NoCollapse         |
              ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoFocusOnAppearing |
              ImGuiWindowFlags_NoSavedSettings;

  if (autofit)
    flags |= ImGuiWindowFlags_AlwaysAutoResize;

  if (! movable)
    flags |= ImGuiWindowFlags_NoMove;

  if (! resizable)
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
  }

  else
  {
    if ((! autofit) && (! resizable))
    {
      ImGui::SetNextWindowSize (ImVec2 ( std::min ( max_size.x, std::max ( size.x, min_size.x ) ),
                                         std::min ( max_size.y, std::max ( size.y, min_size.y ) ) ) );
    }
  }


  ImGui::Begin           ( SK_FormatString ("###Widget_%s", name.c_str ()).c_str (),
                             nullptr,
                               flags );

  pos  = ImGui::GetWindowPos  ();
  size = ImGui::GetWindowSize ();

  bool n = (int)docking & (int)DockAnchor::North,
       s = (int)docking & (int)DockAnchor::South,
       e = (int)docking & (int)DockAnchor::East,
       w = (int)docking & (int)DockAnchor::West;

  SK_Widget_ProcessDocking (this, n, s, e, w);

  ImGui::PushItemWidth (0.5f * ImGui::GetWindowWidth ());

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
SK_Widget::save (iSK_INI* ini)
{
  OnConfig (ConfigEvent::SaveStart);

  if (param_visible)
  {
    param_visible->set_value  (visible);
    param_visible->store      ();
  }

  else
    return;

  param_movable->set_value      (     movable      );
  param_clickthrough->set_value (     click_through);
  param_autofit->set_value      (     autofit      );
  param_resizable->set_value    (     resizable    );
  param_docking->set_value      ((int)docking      );
  param_minsize->set_value      (     min_size     );
  param_maxsize->set_value      (     max_size     );
  param_size->set_value         (     size         );
  param_pos->set_value          (     pos          );

  param_movable->store      ();
  param_clickthrough->store ();
  param_autofit->store      ();
  param_resizable->store    ();
  param_docking->store      ();
  param_minsize->store      ();
  param_maxsize->store      ();
  param_size->store         ();
  param_pos->store          ();

  ini->write (ini->get_filename ());

  OnConfig (ConfigEvent::SaveComplete);
}

void
SK_Widget::config_base (void)
{
  static bool changed = false;

  ImGuiIO& io (ImGui::GetIO ());

  const  float font_size           =             ImGui::GetFont  ()->FontSize                        * io.FontGlobalScale;
  const  float font_size_multiline = font_size + ImGui::GetStyle ().ItemSpacing.y + ImGui::GetStyle ().ItemInnerSpacing.y;

  if (ImGui::Checkbox ("Movable", &movable))
  {
    setMovable (movable);
    changed = true;
  }

  ImGui::SameLine ();
  if (ImGui::Checkbox ("Resizable", &resizable))
  {
    setResizable (resizable);
    changed = true;
  }

  if (! resizable)
  {
    ImGui::SameLine ();
    if (ImGui::Checkbox ("Auto-Fit", &autofit))
    {
      setAutoFit (autofit);
      changed = true;
    }
  }

  else if (changed)
    setAutoFit (false);

  ImGui::SameLine ();
  if (ImGui::Checkbox ("Click-Through", &click_through))
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
    int mask = (dock == 1 ? (int)DockAnchor::North : 0x0) |
               (dock == 2 ? (int)DockAnchor::South : 0x0);

    docking = (DockAnchor)(mask | (int)docking & ~( (int)DockAnchor::North |
                                                    (int)DockAnchor::South ) );
    changed = true;
  }

  anchors = "Undocked\0West\0East\0\0";

  dock = 0;

       if (w) dock = 1;
  else if (e) dock = 2;

  if (ImGui::Combo ("Horizonal Docking Anchor", &dock, anchors, 3))
  {
    int mask = (dock == 1 ? (int)DockAnchor::West : 0x0) |
               (dock == 2 ? (int)DockAnchor::East : 0x0);

    docking = (DockAnchor)(mask | (int)docking & ~( (int)DockAnchor::West |
                                                    (int)DockAnchor::East ) );
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

  done |= ImGui::Button ("  Save  ");

  if (done)
  {
    if (changed)
    {
      extern iSK_INI* osd_ini;

      save (osd_ini);

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


#define SK_MakeKeyMask(vKey,ctrl,shift,alt) \
  (UINT)((vKey) | (((ctrl) != 0) <<  9) |   \
                  (((shift)!= 0) << 10) |   \
                  (((alt)  != 0) << 11))

BOOL
SK_ImGui_WidgetRegistry::DispatchKeybinds (BOOL Control, BOOL Shift, BOOL Alt, BYTE vkCode)
{
  UINT uiMaskedKeyCode =
    SK_MakeKeyMask (vkCode, Control, Shift, Alt);

  static std::array <SK_Widget *, 5> widgets { frame_pacing, volume_control, gpu_monitor, cpu_monitor, d3d11_pipeline };

  for (auto widget : widgets)
  {
    if (widget && uiMaskedKeyCode == widget->getToggleKey ().masked_code)
    {
      widget->setVisible (! widget->isVisible ());

      return TRUE;
    }
  }

  return FALSE;
}

BOOL
SK_ImGui_WidgetRegistry::SaveConfig (void)
{
  static std::array <SK_Widget *, 5> widgets { frame_pacing, volume_control, gpu_monitor, cpu_monitor, d3d11_pipeline };

  for (auto widget : widgets)
  {
    if (widget)
    {
      widget->save (osd_ini);
    }
  }

  return TRUE;
}

sk::ParameterFactory SK_Widget_ParameterFactory;