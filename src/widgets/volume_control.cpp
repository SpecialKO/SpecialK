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

extern iSK_INI* osd_ini;

extern void SK_ImGui_VolumeManager (void);

class SKWG_VolumeControl : public SK_Widget
{
public:
  SKWG_VolumeControl (void) : SK_Widget ("VolumeControl")
  {
    SK_ImGui_Widgets.volume_control = this;

    setResizable    (                false).setAutoFit      (true).setMovable (false).
    setDockingPoint (DockAnchor::SouthWest).setClickThrough (false);
  };

  void run (void)
  {
    static bool first = true;

    if (first)
    {
      focus_key_val =
        LoadWidgetKeybind ( &focus_key, osd_ini,
                              L"Widget Focus Keybinding (Volume Control)",
                                L"Widget.VolumeControl",
                                  L"FocusKey" );

      first = false;
    }
  }

  void draw (void)
  {
    SK_ImGui_VolumeManager ();
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
} __volume_control__;