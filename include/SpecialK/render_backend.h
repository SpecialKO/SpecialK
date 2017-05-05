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

#include <Windows.h>
#include <nvapi.h>

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

enum {
  SK_FRAMEBUFFER_FLAG_SRGB  = 0x1,
  SK_FRAMEBUFFER_FLAG_FLOAT = 0x2
};

struct SK_RenderBackend_V2 : SK_RenderBackend_V1 {
  IUnknown*               device               = nullptr;
  IUnknown*               swapchain            = nullptr;
  NVDX_ObjectHandle       surface              = 0;
  bool                    fullscreen_exclusive = false;
  uint64_t                framebuffer_flags    = 0x00;

  struct gsync_s {
    void update (void);

    BOOL                  capable      = FALSE;
    BOOL                  active       = FALSE;
    DWORD                 last_checked = 0;
  } gsync_state;
};

typedef SK_RenderBackend_V2 SK_RenderBackend;

SK_RenderBackend&
__stdcall
SK_GetCurrentRenderBackend (void);

void
__stdcall
SK_InitRenderBackends (void);

void SK_BootD3D9   (void);
void SK_BootDXGI   (void);
void SK_BootOpenGL (void);
void SK_BootVulkan (void);

#endif /* __SK__RENDER_BACKEND__H__ */