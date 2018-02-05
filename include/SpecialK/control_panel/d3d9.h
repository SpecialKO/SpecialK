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

#ifndef __SK__CPL_D3D9_H__
#define __SK__CPL_D3D9_H__

namespace SK
{
  namespace ControlPanel
  {
    namespace D3D9
    {
      bool Draw (void);

      extern bool show_shader_mod_dlg;
    };
  };
};

struct IDirect3DSwapChain9;

void SK_ImGui_SummarizeD3D9Swapchain (IDirect3DSwapChain9 *pSwap9);

#endif /* __SK__CPL_D3D9_H__ */