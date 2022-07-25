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

class SK_D3D11_Screenshot
{
public:
  explicit SK_D3D11_Screenshot (          SK_D3D11_Screenshot&& moveFrom) { *this = std::move (moveFrom); }
  explicit SK_D3D11_Screenshot (const SK_ComPtr <ID3D11Device>& pDevice);

          ~SK_D3D11_Screenshot (void) {
            dispose ();
          }

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

  void dispose (void);
  bool getData ( UINT* const pWidth,
                 UINT* const pHeight,
                 uint8_t   **ppData,
                 bool        Wait = false );

  __inline
  DXGI_FORMAT
  getInternalFormat (void) noexcept
  {
    return
      framebuffer.NativeFormat;
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
    DXGI_FORMAT        NativeFormat        = DXGI_FORMAT_UNKNOWN;
    DXGI_ALPHA_MODE    AlphaMode           = DXGI_ALPHA_MODE_IGNORE;

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
  SK_ComPtr <ID3D11Device>        pDev                   = nullptr;
  SK_ComPtr <ID3D11DeviceContext> pImmediateCtx          = nullptr;

  SK_ComPtr <IDXGISwapChain>      pSwapChain             = nullptr;
  SK_ComPtr <ID3D11Texture2D>     pBackbufferSurface     = nullptr;
  SK_ComPtr <ID3D11Texture2D>     pStagingBackbufferCopy = nullptr;

  SK_ComPtr <ID3D11Query>         pPixelBufferFence      = nullptr;
  ULONG64                         ulCommandIssuedOnFrame = 0;

  framebuffer_s                   framebuffer            = {     };
};

void SK_D3D11_WaitOnAllScreenshots   (void);
void SK_D3D11_ProcessScreenshotQueue (SK_ScreenshotStage stage = SK_ScreenshotStage::EndOfFrame);
bool SK_Screenshot_D3D11_BeginFrame  (void);
void SK_Screenshot_D3D11_EndFrame    (void);
void SK_Screenshot_D3D11_RestoreHUD  (void);