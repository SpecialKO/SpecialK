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

class SK_D3D9_Screenshot
{
public:
  explicit SK_D3D9_Screenshot (               SK_D3D9_Screenshot&& moveFrom) { *this = std::move (moveFrom); }
  explicit SK_D3D9_Screenshot (const SK_ComPtr <IDirect3DDevice9>& pDevice);
          ~SK_D3D9_Screenshot (void) {
            dispose ();
          }

  SK_D3D9_Screenshot& __cdecl operator= (      SK_D3D9_Screenshot&& moveFrom);

  SK_D3D9_Screenshot                    (const SK_D3D9_Screenshot&          ) = delete;
  SK_D3D9_Screenshot&          operator=(const SK_D3D9_Screenshot&          ) = delete;

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

  void dispose (void);


  bool getData ( UINT     *pWidth,
                 UINT     *pHeight,
                 uint8_t **ppData,
                 bool      Wait = false );

  __inline D3DFORMAT getInternalFormat (void) noexcept {
    return framebuffer.NativeFormat;
  }

  struct framebuffer_s
  {
    // One-time alloc, prevents allocating and freeing memory on the thread
    //   that memory-maps the GPU for perfect wait-free capture.
    struct PinnedBuffer {
      std::atomic_size_t           size    = 0L;
      std::unique_ptr <uint8_t []> bytes   = nullptr;
    } static root_;

    ~framebuffer_s (void) noexcept
    {
      if (PixelBuffer.get () == root_.bytes.get ())
        PixelBuffer.release (); // Does not free

      PixelBuffer.reset ();
    }

    UINT               Width               = 0,
                       Height              = 0;
    D3DFORMAT          NativeFormat        = D3DFMT_UNKNOWN;

    size_t             PBufferSize         = 0L;
    size_t             PackedDstPitch      = 0L,
                       PackedDstSlicePitch = 0L;

    std::unique_ptr
      <uint8_t []>     PixelBuffer         = nullptr;
  };

  __inline
  framebuffer_s*
  getFinishedData (void) noexcept
  {
    return
      ( framebuffer.PixelBuffer.get () != nullptr ) ?
       &framebuffer :                     nullptr;
  }

  __inline
    ULONG64
  getStartFrame (void) const noexcept
  {
    return
      ulCommandIssuedOnFrame;
  }

protected:
  SK_ComPtr <IDirect3DDevice9>    pDev                   = nullptr;
  SK_ComPtr <IDirect3DSwapChain9> pSwapChain             = nullptr;
  SK_ComPtr <IDirect3DSurface9>   pBackbufferSurface     = nullptr;
  SK_ComPtr <IDirect3DSurface9>   pSurfScreenshot        = nullptr;
  ULONG64                         ulCommandIssuedOnFrame = 0;
  framebuffer_s                   framebuffer            = {     };
};

bool SK_D3D9_CaptureSteamScreenshot (SK_ScreenshotStage when  = SK_ScreenshotStage::EndOfFrame);
void SK_D3D9_ProcessScreenshotQueue (SK_ScreenshotStage stage = SK_ScreenshotStage::EndOfFrame);