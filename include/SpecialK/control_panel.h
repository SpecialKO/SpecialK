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

#ifndef __SK__CONTROL_PANEL_H__
#define __SK__CONTROL_PANEL_H__

struct IUnknown;
#include <Unknwnbase.h>

#include <SpecialK/render/backend.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h> // For ImGuiSelectableFlags_Disabled

namespace SK
{
  namespace ControlPanel
  {
    extern SK_RenderAPI render_api;
    extern uint64_t     current_tick;
    extern DWORD        current_time;

    struct font_cfg_s
    {
      float size;
      float size_multiline;
    } extern             font;
  };
};

DWORD SK_GetCurrentMS (void);

void SK_ImGui_Warning          (const wchar_t* wszMessage);
void SK_ImGui_WarningWithTitle (const wchar_t* wszMessage,
                                const wchar_t* wszTitle);

bool SK_ImGui_IsItemClicked      (void);
bool SK_ImGui_IsItemRightClicked (void);
void SK_ImGui_AdjustCursor       (void);

// Special K Extensions to ImGui (Dialog Controls)
//
namespace SK_ImGui
{
  bool
  VerticalToggleButton (const char *text, bool *v);

  // Should return true when clicked, this is not consistent with
  //   the rest of the ImGui API.
  bool BatteryMeter (void);
};

#endif /* __SK__CONTROL_PANEL_H__ */