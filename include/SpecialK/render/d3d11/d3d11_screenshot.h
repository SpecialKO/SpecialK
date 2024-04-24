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

#include <../depends/include/glm/glm.hpp>
#include <../depends/include/glm/detail/type_vec4.hpp>
#include <../depends/include/glm/gtc/packing.hpp>

#include <SpecialK/render/dxgi/dxgi_backend.h>
#include <SpecialK/render/d3d11/d3d11_core.h>
#include <SpecialK/com_util.h>
#include <SpecialK/render/backend.h>
#include <SpecialK/render/screenshot.h>

interface ID3D11Device;
interface ID3D11DeviceContext;
interface ID3D11Query;

class SK_D3D11_Screenshot : public SK_Screenshot
{
public:
  explicit SK_D3D11_Screenshot (          SK_D3D11_Screenshot&& moveFrom) : SK_Screenshot (moveFrom.bCopyToClipboard && (! moveFrom.bSaveToDisk)) { *this = std::move (moveFrom); }
  explicit SK_D3D11_Screenshot (const SK_ComPtr <ID3D11Device>& pDevice, bool allow_sound, bool clipboard_only = false, std::string title = "");

          ~SK_D3D11_Screenshot (void) {
            dispose ();
          }

           void dispose (void) noexcept final;
           bool getData ( UINT* const pWidth,
                          UINT* const pHeight,
                          uint8_t   **ppData,
                          bool        Wait ) final;

  __inline bool isValid (void) noexcept { return pImmediateCtx     != nullptr &&
                                                 pPixelBufferFence != nullptr; }
  __inline bool isReady (void) noexcept
  {
    if (                                      (! isValid ()) ||
       (ulCommandIssuedOnFrame > (SK_GetFramesDrawn () - 1))  )
    {
      return false;
    }

    return (
      S_FALSE != pImmediateCtx->GetData     (
              pPixelBufferFence, nullptr,
                                     0,
             D3D11_ASYNC_GETDATA_DONOTFLUSH )
           );
  }

  SK_D3D11_Screenshot& __cdecl operator= (      SK_D3D11_Screenshot&& moveFrom);

  SK_D3D11_Screenshot                    (const SK_D3D11_Screenshot&          ) = delete;
  SK_D3D11_Screenshot&          operator=(const SK_D3D11_Screenshot&          ) = delete;

  __inline
  DXGI_FORMAT
  getInternalFormat (void) noexcept
  {
    return
      framebuffer.dxgi.NativeFormat;
  }

protected:
  SK_ComPtr <ID3D11Device>        pDev                   = nullptr;
  SK_ComPtr <ID3D11DeviceContext> pImmediateCtx          = nullptr;

  SK_ComPtr <IDXGISwapChain>      pSwapChain             = nullptr;
  SK_ComPtr <ID3D11Texture2D>     pBackbufferSurface     = nullptr;
  SK_ComPtr <ID3D11Texture2D>     pStagingBackbufferCopy = nullptr;

  SK_ComPtr <ID3D11Query>         pPixelBufferFence      = nullptr;
};

void SK_D3D11_WaitOnAllScreenshots     (void);
void SK_D3D11_ProcessScreenshotQueue   (SK_ScreenshotStage stage  = SK_ScreenshotStage::EndOfFrame);
void SK_D3D11_ProcessScreenshotQueueEx (SK_ScreenshotStage stage_ = SK_ScreenshotStage::EndOfFrame,
                                        bool               wait   = false,
                                        bool               purge  = false);
bool SK_Screenshot_D3D11_BeginFrame  (void);
void SK_Screenshot_D3D11_EndFrame    (void);
void SK_Screenshot_D3D11_RestoreHUD  (void);