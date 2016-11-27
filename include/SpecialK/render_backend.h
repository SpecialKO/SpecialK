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

#ifndef __SK__RENDER_BACKEND_H__
#define __SK__RENDER_BACKEND_H__

enum class SK_RenderAPI {
  Reserved = 0x01,
  OpenGL   = 0x02,
  Vulkan   = 0x04,
  D3D9     = 0x08,
  D3D9Ex   = 0x18,
  D3D10    = 0x20, // Don't care
  D3D11    = 0x40,
  D3D12    = 0x80
};

struct SK_RenderBackend_V1 {
  enum class SK_RenderAPI api       = SK_RenderAPI::Reserved;
             wchar_t      name [16] = { L'\0' };
};

typedef SK_RenderBackend_V1 SK_RenderBackend;

SK_RenderBackend
__stdcall
SK_GetCurrentRenderBackend (void);

#endif /* __SK__RENDER_BACKEND__H__ */