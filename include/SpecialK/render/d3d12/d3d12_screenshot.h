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
//#include <SpecialK/render/d3d11/d3d11_core.h>
#include <SpecialK/com_util.h>
#include <SpecialK/render/backend.h>
#include <SpecialK/render/screenshot.h>

class SK_D3D12_Screenshot : public SK_Screenshot
{
public:
  struct readback_ctx_s {
    SK_ComPtr <ID3D12CommandQueue>        pCmdQueue              = nullptr;
    SK_ComPtr <ID3D12GraphicsCommandList> pCmdList               = nullptr;
    SK_ComPtr <ID3D12CommandAllocator>    pCmdAlloc              = nullptr;

    SK_ComPtr <ID3D12Resource>            pBackbufferSurface     = nullptr;
    SK_ComPtr <ID3D12Resource>            pStagingBackbufferCopy = nullptr;

    SK_ComPtr <ID3D12Fence>               pFence                 = nullptr;
    UINT64                                uiFenceVal             =       0;

    framebuffer_s*                        pBackingStore          = nullptr;
  };

  explicit SK_D3D12_Screenshot (           SK_D3D12_Screenshot&& moveFrom) : SK_Screenshot (moveFrom.bCopyToClipboard && (! moveFrom.bSaveToDisk)) { *this = std::move (moveFrom); }
  explicit SK_D3D12_Screenshot ( const SK_ComPtr <ID3D12Device>&       pDevice,
                                 const SK_ComPtr <ID3D12CommandQueue>& pCmdQueue,
                                 const SK_ComPtr <IDXGISwapChain3>&    pSwapChain,
                                       bool                            allow_sound,
                                       bool                            clipboard_only = false );

          ~SK_D3D12_Screenshot (void) {
            dispose ();
          }

           void dispose (void) noexcept final;
           bool getData ( UINT* const pWidth,
                          UINT* const pHeight,
                          uint8_t   **ppData,
                          bool        Wait ) final;

  __inline bool isValid (void) noexcept { return readback_ctx.pFence.p != nullptr; }
  __inline bool isReady (void) noexcept
  {
    if (                                      (! isValid ()) ||
       (ulCommandIssuedOnFrame > (SK_GetFramesDrawn () - 1))  )
    {
      return false;
    }

    return (
      readback_ctx.pFence->GetCompletedValue () >
      readback_ctx.uiFenceVal
           );
  }

  SK_D3D12_Screenshot& __cdecl operator= (      SK_D3D12_Screenshot&& moveFrom);

  SK_D3D12_Screenshot                    (const SK_D3D12_Screenshot&          ) = delete;
  SK_D3D12_Screenshot&          operator=(const SK_D3D12_Screenshot&          ) = delete;

  __inline
  DXGI_FORMAT
  getInternalFormat (void) noexcept
  {
    return
      framebuffer.dxgi.NativeFormat;
  }

  readback_ctx_s*
  getReadbackContext (void) noexcept
  {
    return &readback_ctx;
  }

protected:
  readback_ctx_s readback_ctx = { };

  using readback_ptr = std::shared_ptr <readback_ctx_s>&;
};

void SK_D3D12_WaitOnAllScreenshots   (void);
void SK_D3D12_ProcessScreenshotQueue (SK_ScreenshotStage stage = SK_ScreenshotStage::EndOfFrame);
bool SK_Screenshot_D3D12_BeginFrame  (void);
void SK_Screenshot_D3D12_EndFrame    (void);
void SK_Screenshot_D3D12_RestoreHUD  (void);

LONG SK_D3D12_HideGameHUD   (void);
LONG SK_D3D12_ShowGameHUD   (void);
LONG SK_D3D12_ToggleGameHUD (void);