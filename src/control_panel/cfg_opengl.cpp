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

#include <SpecialK/control_panel/opengl.h>
#include <SpecialK/control_panel/osd.h>


using namespace SK::ControlPanel;

bool
SK::ControlPanel::OpenGL::Draw (void)
{
  if ( ( static_cast <int> (render_api) & static_cast <int> (SK_RenderAPI::OpenGL) ) ==
                                          static_cast <int> (SK_RenderAPI::OpenGL)   &&
       ImGui::CollapsingHeader ("OpenGL Settings", ImGuiTreeNodeFlags_DefaultOpen) )
  {
    ImGui::TreePush ("");

    OSD::DrawVideoCaptureOptions ();

    ImGui::TreePop  ();

    return true;
  }

  return false;
}