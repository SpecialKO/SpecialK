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
  explicit SK_D3D9_Screenshot (SK_D3D9_Screenshot&& rkMove) noexcept
  {
    pDev                     = rkMove.pDev;
    pSwapChain               = rkMove.pSwapChain;
    pBackbufferSurface       = rkMove.pBackbufferSurface;
    pSurfScreenshot          = rkMove.pSurfScreenshot;
    ulCommandIssuedOnFrame   = rkMove.ulCommandIssuedOnFrame;


    framebuffer.Width        = rkMove.framebuffer.Width;
    framebuffer.Height       = rkMove.framebuffer.Height;
    framebuffer.NativeFormat = rkMove.framebuffer.NativeFormat;

    framebuffer.PixelBuffer.Attach (rkMove.framebuffer.PixelBuffer.Detach ());
  }

  SK_D3D9_Screenshot            (const SK_D3D9_Screenshot&) = delete;
  SK_D3D9_Screenshot &operator= (const SK_D3D9_Screenshot&) = delete;


  explicit SK_D3D9_Screenshot (const SK_ComQIPtr <IDirect3DDevice9> pDevice);

    ~SK_D3D9_Screenshot (void) { dispose (); }


  __inline bool isValid (void) { return true; }
  __inline bool isReady (void)
  {
    if ( (! isValid ())                                       ||
         (ulCommandIssuedOnFrame > (SK_GetFramesDrawn () - 1)) )
    {
      return false;
    }

    return false;
  }

  void dispose (void);


  bool getData ( UINT     *pWidth,
                 UINT     *pHeight,
                 uint8_t **ppData,
                 UINT     *pPitch,
                 bool      Wait = false );

  __inline D3DFORMAT getInternalFormat (void) {
    return framebuffer.NativeFormat;
  }


protected:
  SK_ComPtr <IDirect3DDevice9>    pDev                   = nullptr;
  SK_ComPtr <IDirect3DSwapChain9> pSwapChain             = nullptr;
  SK_ComPtr <IDirect3DSurface9>   pBackbufferSurface     = nullptr;
  SK_ComPtr <IDirect3DSurface9>   pSurfScreenshot        = nullptr;
  ULONG                           ulCommandIssuedOnFrame = 0;

  struct framebuffer_s
  {
    UINT               Width        = 0,
                       Height       = 0;
    D3DFORMAT          NativeFormat = D3DFMT_UNKNOWN;

    CHeapPtr <uint8_t> PixelBuffer;
  } framebuffer;
};

bool SK_D3D9_CaptureSteamScreenshot (SK_ScreenshotStage when  = SK_ScreenshotStage::EndOfFrame);
void SK_D3D9_ProcessScreenshotQueue (SK_ScreenshotStage stage = SK_ScreenshotStage::EndOfFrame);