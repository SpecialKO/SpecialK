#pragma once
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

#include <tuple>
#include <concurrent_vector.h>
#include <concurrent_queue.h>

#include <SpecialK/com_util.h>
#include <SpecialK/render/backend.h>
#include <SpecialK/render/screenshot.h>
#include <SpecialK/render/d3d9/d3d9_backend.h>
#include <SpecialK/render/d3d9/d3d9_texmgr.h>

#include <tuple>

class SK_D3D9_Screenshot : public SK_Screenshot
{
public:
  explicit SK_D3D9_Screenshot (               SK_D3D9_Screenshot&& moveFrom) : SK_Screenshot (moveFrom.bCopyToClipboard && (! moveFrom.bSaveToDisk)) { *this = std::move (moveFrom); }
  explicit SK_D3D9_Screenshot (const SK_ComPtr <IDirect3DDevice9>& pDevice,
                                     bool                          allow_sound,
                                     bool                          clipboard_only = false,
                                     std::string                   title          = "");
          ~SK_D3D9_Screenshot (void) {
            dispose ();
          }

  SK_D3D9_Screenshot& __cdecl operator= (      SK_D3D9_Screenshot&& moveFrom);

  SK_D3D9_Screenshot                    (const SK_D3D9_Screenshot&          ) = delete;
  SK_D3D9_Screenshot&          operator=(const SK_D3D9_Screenshot&          ) = delete;

           void dispose (void) noexcept final;
           bool getData ( UINT* const pWidth,
                          UINT* const pHeight,
                          uint8_t   **ppData,
                          bool        Wait ) final;

  __inline bool isValid (void) noexcept { return true; }
  __inline bool isReady (void) noexcept
  {
    if ( (! isValid ())                                       ||
         (ulCommandIssuedOnFrame > (SK_GetFramesDrawn () - 1)) )
    {
      return false;
    }

    return true;
  }

  __inline D3DFORMAT getInternalFormat (void) noexcept {
    return framebuffer.d3d9.NativeFormat;
  }

protected:
  SK_ComPtr <IDirect3DDevice9>    pDev               = nullptr;
  SK_ComPtr <IDirect3DSwapChain9> pSwapChain         = nullptr;
  SK_ComPtr <IDirect3DSurface9>   pBackbufferSurface = nullptr;
  SK_ComPtr <IDirect3DSurface9>   pSurfScreenshot    = nullptr;
};

bool SK_D3D9_CaptureSteamScreenshot (SK_ScreenshotStage when  = SK_ScreenshotStage::EndOfFrame);
void SK_D3D9_ProcessScreenshotQueue (SK_ScreenshotStage stage = SK_ScreenshotStage::EndOfFrame);