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

#include <SpecialK/widgets/widget.h>


extern iSK_INI* osd_ini;


extern bool
SK_ImGui_IsWindowRightClicked (const ImGuiIO& io = ImGui::GetIO ());


extern LONG SK_D3D11_ToggleGameHUD          (void);
extern void SK_TriggerHudFreeScreenshot     (void) noexcept;

#ifdef _M_AMD64
extern void SK_YS0_TriggerHudFreeScreenshot (void);
#endif


void
SK_Widget::run_base (void)
{
  if ((! run_once__ )&& osd_ini)
  {
    run_once__ = true;

    toggle_key_val =
      LoadWidgetKeybind ( &toggle_key, osd_ini,
                            L"",
                              SK_FormatStringW (L"Widget.%hs", name.c_str ()),
                                L"ToggleKey" );

    param_visible =
      LoadWidgetBool ( &visible, osd_ini,
                         L"",
                           SK_FormatStringW (L"Widget.%hs", name.c_str ()),
                             L"Visible" );

    param_movable =
      LoadWidgetBool ( &movable, osd_ini,
                         L"",
                           SK_FormatStringW (L"Widget.%hs", name.c_str ()),
                             L"Movable" );

    param_resizable =
      LoadWidgetBool ( &resizable, osd_ini,
                         L"",
                           SK_FormatStringW (L"Widget.%hs", name.c_str ()),
                             L"Resizable" );

    param_autofit =
      LoadWidgetBool ( &autofit, osd_ini,
                         L"",
                           SK_FormatStringW (L"Widget.%hs", name.c_str ()),
                             L"AutoFit" );

    param_border =
      LoadWidgetBool ( &border, osd_ini,
                         L"",
                           SK_FormatStringW (L"Widget.%hs", name.c_str ()),
                             L"Border" );

    param_clickthrough =
      LoadWidgetBool ( &click_through, osd_ini,
                         L"",
                           SK_FormatStringW (L"Widget.%hs", name.c_str ()),
                             L"ClickThrough" );

    param_docking =
      LoadWidgetDocking ( &docking, osd_ini,
                            L"",
                              SK_FormatStringW (L"Widget.%hs", name.c_str ()),
                                L"DockingPoint" );

    param_minsize =
      LoadWidgetVec2 ( &min_size, osd_ini,
                         L"",
                           SK_FormatStringW (L"Widget.%hs",    name.c_str ()),
                             L"MinSize" );

    param_maxsize =
      LoadWidgetVec2 ( &max_size, osd_ini,
                         L"",
                           SK_FormatStringW (L"Widget.%hs",    name.c_str ()),
                             L"MaxSize" );

    param_size =
      LoadWidgetVec2 ( &size, osd_ini,
                         L"",
                           SK_FormatStringW (L"Widget.%hs",    name.c_str ()),
                             L"Size" );

    param_pos =
      LoadWidgetVec2 ( &pos, osd_ini,
                         L"",
                           SK_FormatStringW (L"Widget.%hs",    name.c_str ()),
                             L"Position" );
  }

  run ();
}


void
SK_Widget_CalcClipRect ( SK_Widget* pWidget,
                             bool n, bool s,
                             bool e, bool w,
                                ImVec2& min,
                                ImVec2& max )
{
  if (pWidget == nullptr)
    return;

  static auto& io =
    ImGui::GetIO ();

  // Docking alignment visualiztion
  bool draw_horz_ruler = false;
  bool draw_vert_ruler = false;

        ImVec2 pos  = pWidget->getPos  ();
  const ImVec2 size = pWidget->getSize ();

  const ImGuiContext& g =
    *ImGui::GetCurrentContext ();

  if (n || s)
  {
    if ( pWidget->isMovable () && ( ( ImGui::IsMouseDragging  (0)      && ImGui::IsWindowHovered () ) ||
                                    ( ImGui::IsNavDragging    (0,0.0f) && g.NavWindowingTarget        ==
                                      ImGui::GetCurrentWindow ( )                                   )
                                  )
       )
    {
      draw_horz_ruler = true;
    }

    if (n)
      pos.y = 0.0;

    if (s)
    {
      pos.y =
        io.DisplaySize.y - size.y;
    }
  }


  if (e || w)
  {
    if ( pWidget->isMovable () && ( ( ImGui::IsMouseDragging  (0)      && ImGui::IsWindowHovered () ) ||
                                    ( ImGui::IsNavDragging    (0,0.0f) && g.NavWindowingTarget        ==
                                      ImGui::GetCurrentWindow ( )                                   )
                                  )
       )
    {
      draw_vert_ruler = true;
    }

    if (e)
    {
      pos.x =
        io.DisplaySize.x - size.x;
    }

    if (w)
      pos.x = 0.0;
  }


  if (                    size.x > 0 &&                    size.y > 0 &&
       io.DisplaySize.x - size.x > 0 && io.DisplaySize.y - size.y > 0  )
  {
    pos.x = std::max (0.0f, std::min (pos.x, io.DisplaySize.x - size.x));
    pos.y = std::max (0.0f, std::min (pos.y, io.DisplaySize.y - size.y));
  }

  if (  draw_horz_ruler ^ draw_vert_ruler )
  { if (draw_vert_ruler)
    { const ImVec2 horz_pos =
        ImVec2 ( ( e  ?  io.DisplaySize.x - size.x
                      :                     size.x ),
                     0.0f );                         min =
            ImVec2 ( 0.0f,       0.0f             ); max =
            ImVec2 ( horz_pos.x, io.DisplaySize.y );     }

    if (draw_horz_ruler)
    { const ImVec2 vert_pos =
        ImVec2 ( 0.0f, ( s ? io.DisplaySize.y - size.y
                           :                    size.y)
               );         min =
                 ImVec2 ( 0.0f,             0          ); max =
                 ImVec2 ( io.DisplaySize.x, vert_pos.y );     }
  }
}

void
SK_Widget_ProcessDocking ( SK_Widget* pWidget,
                               bool n, bool s,
                               bool e, bool w )
{
  static auto& io =
    ImGui::GetIO ();

  // Docking alignment visualization
  bool draw_horz_ruler = false;
  bool draw_vert_ruler = false;

        ImVec2 pos  = pWidget->getPos  ();
  const ImVec2 size = pWidget->getSize ();

  const ImGuiContext& g =
    *ImGui::GetCurrentContext ();

  if (n || s)
  {
    if ( pWidget->isMovable () && ( ( ImGui::IsMouseDragging (0)      && ImGui::IsWindowHovered  ( ) )  ||
                                    ( ImGui::IsNavDragging   (0,0.0f) && ImGui::GetCurrentWindow ( )    ==
                                                                             g.NavWindowingTarget  ) )
       )
    {
      draw_horz_ruler = true;
    }

    if (n)
      pos.y = 0.0;

    if (s)
    {
      pos.y =
        io.DisplaySize.y - size.y;
    }
  }


  if (e || w)
  {
    if (pWidget->isMovable () && ( ( ImGui::IsMouseDragging (0)      && ImGui::IsWindowHovered () ) ||
                                   ( ImGui::IsNavDragging   (0,0.0f) && g.NavWindowingTarget == ImGui::GetCurrentWindow () ) ))
    {
      draw_vert_ruler = true;
    }

    if (e)
      pos.x = io.DisplaySize.x - size.x;

    if (w)
      pos.x = 0.0;
  }


  if ( (                    size.x > 0 ) && (                    size.y ) > 0 &&
       ( io.DisplaySize.x - size.x > 0 ) && ( io.DisplaySize.y - size.y ) > 0 )
  {
    pos.x
    = std::max (0.0f, std::min (pos.x, io.DisplaySize.x - size.x));
    pos.y
    = std::max (0.0f, std::min (pos.y, io.DisplaySize.y - size.y));
  }


  if (pWidget->getDockingPoint () != SK_Widget::DockAnchor::None)
  {
        pWidget->setPos (pos);
    ImGui::SetWindowPos (pos, ImGuiCond_Always);
  }


  if ( draw_horz_ruler ^
       draw_vert_ruler   )
  {
    ImVec2 xy0, xy1;

    if (draw_vert_ruler)
    {
      const ImVec2 horz_pos =
        ImVec2 ( (e ? io.DisplaySize.x - size.x :
                                         size.x   ),
                   0.0f );

      xy0 = ImVec2 ( horz_pos.x, 0.0f             );
      xy1 = ImVec2 ( horz_pos.x, io.DisplaySize.y );
    }

    if (draw_horz_ruler)
    {
      const ImVec2 vert_pos =
        ImVec2 ( 0.0f,
                ( s ? io.DisplaySize.y - size.y :
                                         size.y   )
               );

      xy0 = ImVec2 ( 0.0f,             vert_pos.y );
      xy1 = ImVec2 ( io.DisplaySize.x, vert_pos.y );
    }

    DWORD dwNow = timeGetTime ();

    const ImVec4 col =
      ImColor::HSV ( 0.133333f,
                       std::min ( static_cast <float>(0.161616f +  (dwNow % 250) / 250.0f -
                                                           floor ( (dwNow % 250) / 250.0f) ), 1.0f),
                           std::min ( static_cast <float>(0.333 +  (dwNow % 500) / 500.0f -
                                                           floor ( (dwNow % 500) / 500.0f) ), 1.0f) );
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
  if (SK_ImGui_Widgets->hide_all)
    return;

  static auto& io =
    ImGui::GetIO ();

  //extern volatile LONG __SK_ScreenShot_CapturingHUDless;
  //if (ReadAcquire (&__SK_ScreenShot_CapturingHUDless))
  //    return;

  if (ImGui::GetFont () == nullptr)
    return;


  const float fScale =
    ImGui::GetFont ()->Scale;

                ImGui::GetFont ()->Scale = scale;
  SK_ImGui_AutoFont
    local_font (ImGui::GetFont ());

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


  int flags = ImGuiWindowFlags_NoTitleBar      | ImGuiWindowFlags_NoCollapse         |
              ImGuiWindowFlags_NoScrollbar     | ImGuiWindowFlags_NoFocusOnAppearing |
              ImGuiWindowFlags_NoSavedSettings | (border ? 0x0 : ImGuiWindowFlags_NoBackground);

  if (autofit)
    flags |= ImGuiWindowFlags_AlwaysAutoResize;

  if (! movable)
    flags |= ImGuiWindowFlags_NoMove;

  if (! resizable)
    flags |= ImGuiWindowFlags_NoResize;

  if (click_through && (! SK_ImGui_Active ()) && state__ != 1)
    flags |= ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMove
                                       | ImGuiWindowFlags_NoResize;


  // Modal State:  Config
  if (state__ == 1)
  {
    flags &= ~( ImGuiWindowFlags_AlwaysAutoResize );
    flags |=  ( ImGuiWindowFlags_NoResize       |
                ImGuiWindowFlags_AlwaysAutoResize );

    ImGui::SetNextWindowSize (
      ImVec2 ( std::max ( size.x, 420.0f ),
               std::max ( size.y, 190.0f )
             )               );

    if (! SK_ImGui_Active ())
      nav_usable = true;
  }

  else
  {
    if (! SK_ImGui_Active ())
      nav_usable = false;

    if ( (! autofit) &&
         (! resizable) )
    {
      ImGui::SetNextWindowSize (
        ImVec2 ( std::min (         max_size.x,
                 std::max ( size.x, min_size.x ) ),
                 std::min (         max_size.y,
                 std::max ( size.y, min_size.y ) )
               )               );
    }
  }


  bool n = ( static_cast <int> (docking) & static_cast <int> (DockAnchor::North) ) != 0,
       s = ( static_cast <int> (docking) & static_cast <int> (DockAnchor::South) ) != 0,
       e = ( static_cast <int> (docking) & static_cast <int> (DockAnchor::East ) ) != 0,
       w = ( static_cast <int> (docking) & static_cast <int> (DockAnchor::West ) ) != 0;

  float fAlpha =
    ImGui::GetStyle ().Alpha;

  int pushed_style_vars = 0;

  // Since the Tobii widget is used to configure gazing, it should
  //   not be subject to gazing.
  if ( this != SK_ImGui_Widgets->tobii )
  {
    extern bool    SK_Tobii_WantWidgetGazing (void);
    extern ImVec2& SK_Tobii_GetGazePosition  (void);
           ImVec2 vTobiiPos =
                   SK_Tobii_GetGazePosition  (    );

    if (SK_Tobii_WantWidgetGazing ())
    {
      const ImVec2 vPos =
        getPos ();

      const ImVec2 vSize =
        getSize ();

      const bool bad_data =
        (    vTobiiPos.x == 0.0f &&
             vTobiiPos.y == 0.0f    );

      const ImVec2 vMousePos =
        ImGui::GetMousePos ();

      // First test the regular cursor -- if that's hovering the widget,
      //   it's pretty darn important the widget be bright enough to use.
      bool outside  =
        (! ( vMousePos.x <= (vPos.x + vSize.x) &&
             vMousePos.x >=  vPos.x            &&
             vMousePos.y <= (vPos.y + vSize.y) &&
             vMousePos.y >=  vPos.y          )    );

      if (outside)
      {
        // Cursor's not hovering, maybe Tobii is?
        outside =
          (! ( vTobiiPos.x <= (vPos.x + vSize.x + vSize.x * 0.125f) &&
               vTobiiPos.x >=  vPos.x -           vSize.x * 0.125f  &&
               vTobiiPos.y <= (vPos.y + vSize.y + vSize.y * 0.125f) &&
               vTobiiPos.y >=  vPos.y -           vSize.y * 0.125f)    );

        // Nope, nothing's hovering, so dim this widget
        if (bad_data || outside)
        {
          fAlpha *= 0.125f;
        }
      }
    }
  }

  else
  {
    // Hack because the widget has to be drawn at all times
    //   in order to support the gazing cursor
    if (! isVisible ())
    {
      ImGui::PushStyleVar (ImGuiStyleVar_FramePadding,        ImVec2 (0.0f, 0.0f));
      ImGui::PushStyleVar (ImGuiStyleVar_FrameRounding,                     0.0f );
      ImGui::PushStyleVar (ImGuiStyleVar_WindowMinSize,       ImVec2 (0.0f, 0.0f));
      ImGui::PushStyleVar (ImGuiStyleVar_WindowPadding,       ImVec2 (0.0f, 0.0f));
      ImGui::PushStyleVar (ImGuiStyleVar_WindowRounding,                    0.0f );
      ImGui::PushStyleVar (ImGuiStyleVar_GrabMinSize,                       0.0f );
      ImGui::PushStyleVar (ImGuiStyleVar_ChildRounding,                     0.0f );
      ImGui::PushStyleVar (ImGuiStyleVar_ItemSpacing,         ImVec2 (0.0f, 0.0f));
      ImGui::PushStyleVar (ImGuiStyleVar_ItemInnerSpacing,    ImVec2 (0.0f, 0.0f));
      ImGui::PushStyleVar (ImGuiStyleVar_IndentSpacing,                     0.0f );

      ImGui::SetNextWindowSize (ImVec2 (    0.0f, 0.0f    ),    ImGuiCond_Once);

      // No border, because it would be a white dot
      flags &= ImGuiWindowFlags_NoBackground;///~ImGuiWindowFlags_ShowBorders;

      pushed_style_vars = 10;
    }
  }


  char
    hashed_name [128];
   *hashed_name = '\0';

  lstrcatA (hashed_name, name.c_str ());
  lstrcatA (hashed_name, "##Widget_");
  lstrcatA (hashed_name, name.c_str ());

  ImGui::PushStyleVar (ImGuiStyleVar_Alpha, fAlpha);
  ImGui::Begin        ( hashed_name,
                          nullptr, flags );

  ImGui::SetWindowFontScale (SK_ImGui_Widgets->scale);

  static SK_Widget*
       focus_widget = nullptr;
  bool focus_change = false;

  if ( ImGui::IsWindowFocused ()   &&
                      focus_widget != this )
  {
    focus_widget = this;
    focus_change = true;
  }

  ImGui::PushItemWidth (
    0.5f * ImGui::GetWindowWidth ()
  );

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

  const bool right_clicked =
    SK_ImGui_IsWindowRightClicked ();

  pos  = ImGui::GetWindowPos  ();
  size = ImGui::GetWindowSize ();

  SK_Widget_ProcessDocking (this, n, s, e, w);

  if (right_clicked || focus_change)
  {
    const ImVec2 min (pos.x,          pos.y);
    const ImVec2 max (pos.x + size.x, pos.y + size.y);

    extern bool SK_ControlPanel_Activated;

    if (SK_ControlPanel_Activated)
    {
      if (ImGui::IsWindowAppearing ())
      {
        if (! ImGui::IsMouseHoveringRect (min, max)/* && ImGui::IsWindowFocused ()*/)
        {
          io.WantSetMousePos = true;
          io.MousePos        = ImVec2 ( ( pos.x + size.x ) / 2.0f,
                                        ( pos.y + size.y ) / 2.0f );
        }
      }
    }

    ImGui::SetWindowFocus ();

    if (right_clicked)
      state__ = 1;
  }

  ImGui::End         ();
  ImGui::PopStyleVar (pushed_style_vars + 1);

  ImGui::GetFont ()->Scale = fScale;
}

void
SK_Widget::save (iSK_INI* /*ini*/)
{
  OnConfig (ConfigEvent::SaveStart);

  //run_base ();

  if (run_once__)
  {
    param_visible->store      (     visible      );
    param_movable->store      (     movable      );
    param_border->store       (     border       );
    param_clickthrough->store (     click_through);
    param_autofit->store      (     autofit      );
    param_resizable->store    (     resizable    );
    param_docking->store      ((int)docking      );
    param_minsize->store      (     min_size     );
    param_maxsize->store      (     max_size     );
    param_size->store         (     size         );
    param_pos->store          (     pos          );
  }

  OnConfig (ConfigEvent::SaveComplete);
}

class SKWG_D3D11_Pipeline : public SK_Widget { };
class SKWG_CPU_Monitor    : public SK_Widget { };


extern SKWG_D3D11_Pipeline* SK_Widget_GetD3D11Pipeline (void);
extern SKWG_CPU_Monitor*    SK_Widget_GetCPU           (void);

void
SK_Widget::config_base (void)
{
  static bool changed = false;

  const  float font_size           =             ImGui::GetFont  ()->FontSize;//                        * scale;//io.FontGlobalScale;
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

  ImGui::SameLine ();

  if (ImGui::Checkbox ("Draw Border", &border))
  {
    setBorder (border);
    changed = true;
  }

  const bool n = ( static_cast <int> (docking) & static_cast <int> (DockAnchor::North) ) != 0,
             s = ( static_cast <int> (docking) & static_cast <int> (DockAnchor::South) ) != 0,
             e = ( static_cast <int> (docking) & static_cast <int> (DockAnchor::East ) ) != 0,
             w = ( static_cast <int> (docking) & static_cast <int> (DockAnchor::West ) ) != 0;

  const char* anchors =
    "Undocked\0North\0South\0\0";

          int dock = 0;
       if (n) dock = 1;
  else if (s) dock = 2;

  if (ImGui::Combo ("Vertical Docking Anchor", &dock, anchors, 3))
  {
    const int mask = ( dock == 1 ? static_cast <int> (DockAnchor::North) : 0x0 ) |
                     ( dock == 2 ? static_cast <int> (DockAnchor::South) : 0x0 );

    docking =
      static_cast <DockAnchor> (
             mask | static_cast <int> (     docking     ) & ~(
                    static_cast <int> (DockAnchor::North) |
                    static_cast <int> (DockAnchor::South)    )
                               );
    changed = true;
  }

  anchors =
    "Undocked\0West\0East\0\0";

              dock = 0;
       if (w) dock = 1;
  else if (e) dock = 2;

  if (ImGui::Combo ("Horizontal Docking Anchor", &dock, anchors, 3))
  {
    const int mask = (dock == 1 ? static_cast <int> (DockAnchor::West) : 0x0) |
                     (dock == 2 ? static_cast <int> (DockAnchor::East) : 0x0);

    docking =
      static_cast <DockAnchor> (
             mask | static_cast <int> (     docking    ) & ~(
                    static_cast <int> (DockAnchor::West) |
                    static_cast <int> (DockAnchor::East)    )
                               );
    changed = true;
  }

  ImGui::Separator ();

  auto Keybinding =
    [](SK_Keybind* binding, sk::ParameterStringW* param) ->
    auto
    {
      if (param == nullptr || binding == nullptr)
        return false;

      std::string label  =
        SK_WideCharToUTF8 (binding->human_readable) + "###";
                  label += binding->bind_name;

      if (ImGui::Selectable (label.c_str (), false))
      {
        ImGui::OpenPopup (binding->bind_name);
      }

      std::wstring original_binding =
                            binding->human_readable;

      SK_ImGui_KeybindDialog (binding);

      if (original_binding != binding->human_readable)
      {
        param->store         (binding->human_readable);

        osd_ini->write (osd_ini->get_filename ());

        return true;
      }

      return false;
    };

  ImGui::Text       ("Key Bindings");
  ImGui::TreePush   ("");
  ImGui::BeginGroup (  );

  if (toggle_key_val != nullptr)
    ImGui::Text     ("Widget Toggle");
  if (focus_key_val != nullptr)
    ImGui::Text     ("Widget Focus");

  ImGui::EndGroup   (  );
  ImGui::SameLine   (  );
  ImGui::BeginGroup (  );

  if (toggle_key_val != nullptr)
    Keybinding      (&toggle_key, toggle_key_val);
  if (focus_key_val != nullptr)
    Keybinding      (&focus_key,  focus_key_val);

  ImGui::EndGroup   (  );
  ImGui::TreePop    (  );
  ImGui::Separator  (  );
  ImGui::SliderFloat("Widget Scale", &scale, 0.25f, 2.0f);
  ImGui::Separator  (  );

  const bool done =
    ImGui::Button   ("  Save  ");

  if (done)      {
    if (changed) { save (osd_ini);
        changed = false;         }
        state__ = 0;             }
}



void
SK_Widget::load (iSK_INI*)
{
  OnConfig (ConfigEvent::LoadStart);
  // ...
  OnConfig (ConfigEvent::LoadComplete);
}


#define SK_MakeKeyMask(vKey,ctrl,shift,alt) \
  static_cast <UINT>((vKey) | (((ctrl) != 0) <<  9) |   \
                              (((shift)!= 0) << 10) |   \
                              (((alt)  != 0) << 11))

SK_Widget*
SK_HDR_GetWidget (void)
{
  return
    SK_ImGui_Widgets->hdr_control;
}

extern
BOOL
CALLBACK
SK_HDR_KeyPress ( BOOL Control,
                  BOOL Shift,
                  BOOL Alt,
                  BYTE vkCode );

BOOL
SK_ImGui_WidgetRegistry::DispatchKeybinds ( BOOL Control,
                                            BOOL Shift,
                                            BOOL Alt,
                                            BYTE vkCode )
{
  BOOL dispatched = FALSE;

  const auto uiMaskedKeyCode =
    SK_MakeKeyMask (vkCode, Control, Shift, Alt);

  auto widgets =
  { SK_ImGui_Widgets->frame_pacing,   SK_ImGui_Widgets->volume_control,
    SK_ImGui_Widgets->gpu_monitor,    SK_ImGui_Widgets->cpu_monitor,
    SK_ImGui_Widgets->d3d11_pipeline, SK_ImGui_Widgets->thread_profiler,
    SK_ImGui_Widgets->hdr_control,    SK_ImGui_Widgets->tobii
  };

  for (auto& widget : widgets)
  {
    if (widget)
    {
      if (uiMaskedKeyCode == widget->getToggleKey ().masked_code)
      {
        widget->setVisible (! widget->isVisible ());

        dispatched = TRUE;
      }

      if (widget->keyboard (Control, Shift, Alt, vkCode))
      {
        dispatched = TRUE;
      }
    }
  }

  if ( SK_HDR_KeyPress ( Control, Shift,
                         Alt,     vkCode )
     )
  {
    dispatched = TRUE;
  }


  //
  // TEMP HACK: Screenshots
  //
  static
    const
      std::set <SK_ConfigSerializedKeybind *>
        screenshot_keys = {
          &config.steam.screenshots.game_hud_free_keybind,
          &config.steam.screenshots.sk_osd_free_keybind,
          &config.steam.screenshots.sk_osd_insertion_keybind
        };

  if ( uiMaskedKeyCode ==
         SK_MakeKeyMask ( 'H', false, true,
                                      true )
     )
  {
    SK_D3D11_ToggleGameHUD ();

    dispatched = TRUE;
  }

  static const auto& game_id =
    SK_GetCurrentGameID ();

  for ( auto& keybind : screenshot_keys )
  {
    if ( uiMaskedKeyCode == keybind->masked_code )
    {
      if (       keybind == &config.steam.screenshots.game_hud_free_keybind )
      {
#ifdef _M_AMD64
        static bool __yakuza0 =
          (game_id == SK_GAME_ID::Yakuza0);

        if (__yakuza0)
        {
          SK_YS0_TriggerHudFreeScreenshot ();
        }

        else
#endif
          SK_TriggerHudFreeScreenshot ();
      }

      else if (  keybind == &config.steam.screenshots.sk_osd_free_keybind )
      {
        SK::SteamAPI::TakeScreenshot (
          SK_ScreenshotStage::BeforeOSD
        );
      }

      else if (  keybind == &config.steam.screenshots.sk_osd_insertion_keybind )
      {
        SK::SteamAPI::TakeScreenshot (
          SK_ScreenshotStage::EndOfFrame
        );
      }

      dispatched = TRUE;
    }
  }

  return
    dispatched;
}

BOOL
SK_ImGui_WidgetRegistry::SaveConfig (void)
{
  SK_RunOnce (SK_ImGui_Widgets->d3d11_pipeline = SK_Widget_GetD3D11Pipeline ());
  SK_RunOnce (SK_ImGui_Widgets->cpu_monitor    = SK_Widget_GetCPU ());

  auto widgets =
  { SK_ImGui_Widgets->frame_pacing,   SK_ImGui_Widgets->volume_control,
    SK_ImGui_Widgets->gpu_monitor,    SK_ImGui_Widgets->cpu_monitor,
    SK_ImGui_Widgets->d3d11_pipeline, SK_ImGui_Widgets->thread_profiler,
    SK_ImGui_Widgets->hdr_control,    SK_ImGui_Widgets->tobii
  };

  for ( auto& widget : widgets )
  {
    assert (widget != nullptr);

    if (widget != nullptr)
        widget->save (osd_ini);
  }

  return TRUE;
}

SK_LazyGlobal <sk::ParameterFactory> SK_Widget_ParameterFactory;