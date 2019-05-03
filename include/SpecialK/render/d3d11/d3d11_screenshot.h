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

#include <SpecialK/com_util.h>
#include <SpecialK/render/backend.h>
#include <SpecialK/render/screenshot.h>

class SK_D3D11_Screenshot
{
public:
  explicit SK_D3D11_Screenshot (const SK_D3D11_Screenshot&         rkMove);
  explicit SK_D3D11_Screenshot (const SK_ComQIPtr <ID3D11Device>& pDevice);

          ~SK_D3D11_Screenshot (void) {
            dispose ();
          }

  __inline bool isValid (void) { return pPixelBufferFence != nullptr; }
  __inline bool isReady (void)
  {
    if (                                      (! isValid ()) ||
       (ulCommandIssuedOnFrame > (SK_GetFramesDrawn () - 1))  )
    {
      return false;
    }

    if (pPixelBufferFence != nullptr)
    {
      return (
        S_FALSE != pImmediateCtx->GetData (
                     pPixelBufferFence, nullptr,
                     0,               0x0 )
      );
    }

    return false;
  }

  void dispose (void);
  bool getData ( UINT     *pWidth,
                 UINT     *pHeight,
                 uint8_t **ppData,
                 bool      Wait = false );

  __inline
  DXGI_FORMAT
  getInternalFormat (void) { return framebuffer.NativeFormat; }

  struct framebuffer_s
  {
    UINT               Width        = 0,
                       Height       = 0;
    DXGI_FORMAT        NativeFormat = DXGI_FORMAT_UNKNOWN;

    LONG               PBufferSize         = 0L;
    size_t             PackedDstPitch      = 0L,
                       PackedDstSlicePitch = 0L;

    CHeapPtr <uint8_t> PixelBuffer = { };
  };

  __inline
  framebuffer_s*
  getFinishedData (void)
  {
    return
      ( framebuffer.PixelBuffer.m_pData != nullptr ) ?
       &framebuffer :                      nullptr;
  }

  __inline
    ULONG
  getStartFrame (void) const
  {
    return
      ulCommandIssuedOnFrame;
  }


protected:
  SK_ComPtr <ID3D11Device>        pDev                   = nullptr;
  SK_ComPtr <ID3D11DeviceContext> pImmediateCtx          = nullptr;

  SK_ComPtr <IDXGISwapChain>      pSwapChain             = nullptr;
  SK_ComPtr <ID3D11Texture2D>     pBackbufferSurface     = nullptr;
  SK_ComPtr <ID3D11Texture2D>     pStagingBackbufferCopy = nullptr;

  SK_ComPtr <ID3D11Query>         pPixelBufferFence      = nullptr;
  ULONG                           ulCommandIssuedOnFrame = 0;
  BOOL                            bRecycled              = FALSE;

  framebuffer_s                   framebuffer            = {     };

  static SK_LazyGlobal <concurrency::concurrent_queue <std::pair <LONG, uint8_t*>>>
                                                       recycled_buffers;
  static volatile LONG                                 recycled_records;
  static volatile LONG                                 recycled_size;
};

void SK_D3D11_WaitOnAllScreenshots   (void);
void SK_D3D11_ProcessScreenshotQueue (SK_ScreenshotStage stage = SK_ScreenshotStage::EndOfFrame);
bool SK_Screenshot_D3D11_BeginFrame  (void);
void SK_Screenshot_D3D11_EndFrame    (void);