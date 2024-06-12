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

#include <SpecialK/render/backend.h>

enum class SK_ScreenshotStage
{
  BeforeGameHUD = 0,    // Requires a game profile indicating trigger shader
  BeforeOSD     = 1,    // Before SK draws its OSD

  PrePresent    = 2,    // Before third-party overlays draw, but after SK is done
  EndOfFrame    = 3,    // Generally captures all add-on overlays (including the Steam overlay)
  ClipboardOnly = 4,    // EndOfFrame (Behavior: Not Saved to Disk, Always Copied to Clipboard)

  _FlushQueue   = 5     // Causes any screenshots in progress to complete before the next frame,
                        //   typically needed when Alt+Tabbing or resizing the swapchain.
};

struct SK_ScreenshotQueue
{
  union
  {
    // Queue Array
    volatile LONG stages [5];

    struct
    {
      volatile LONG pre_game_hud;

      volatile LONG without_sk_osd;
      volatile LONG with_sk_osd;
      volatile LONG with_everything;
      volatile LONG only_clipboard;
    };
  };

  struct MemoryTotals {
    std::atomic_size_t capture_bytes = 0;
    std::atomic_size_t encode_bytes  = 0;
    std::atomic_size_t write_bytes   = 0;
  } static pooled,
           completed;

  static constexpr
    MemoryTotals maximum =
    {
#ifdef _M_AMD64
      1024 * 1024 * 1024,
       512 * 1024 * 1024,
       256 * 1024 * 1024
#else
      256 * 1024 * 1024,
      128 * 1024 * 1024,
       64 * 1024 * 1024
#endif
    };
};

struct SK_ScreenshotTitleQueue
{
  union
  {
    // Queue Array
    std::string stages [5];

    struct
    {
      std::string pre_game_hud;

      std::string without_sk_osd;
      std::string with_sk_osd;
      std::string with_everything;
      std::string only_clipboard;
    };
  };

  ~SK_ScreenshotTitleQueue (void)
  {

  }
};

extern SK_ScreenshotQueue
 enqueued_screenshots;
extern SK_ScreenshotQueue
 enqueued_sounds;
extern SK_ScreenshotTitleQueue
 enqueued_titles;

class                    SK_RenderBackend_V2;
using SK_RenderBackend = SK_RenderBackend_V2;

void
SK_Screenshot_ProcessQueue ( SK_ScreenshotStage stage,
                       const SK_RenderBackend&  rb );

bool
SK_Screenshot_IsCapturingHUDless (void);

bool
SK_Screenshot_IsCapturing (void);

#include <concurrent_unordered_map.h>


#include <../depends/include/DirectXTex/DirectXTex.h>

class SK_ScreenshotManager
{
public:
  // No info about the files is managed, this is
  //   just a running tally updated when a new
  //     screenshot is written.
  struct screenshot_repository_s {
    LARGE_INTEGER liSize = { 0, 0 };
    UINT           files =      0U ;
  };

  SK_ScreenshotManager (void) noexcept
  {
    init ();
  }

  ~SK_ScreenshotManager (void) = default;

  void init (void) noexcept;

  const wchar_t*           getBasePath     (void) const;
  screenshot_repository_s& getRepoStats    (bool refresh = false);

  bool                     checkDiskSpace  (uint64_t bytes_needed) const;
  bool                     copyToClipboard (const DirectX::Image& image,
                                            const DirectX::Image* pOptionalHDR        = nullptr,
                                            const wchar_t*        wszOptionalFilename = nullptr) const;

protected:
  screenshot_repository_s screenshots = { };

private:
};

class SK_Screenshot
{
public:
  SK_Screenshot (bool clipboard_only);

  virtual void dispose (void) noexcept              = 0;
  virtual bool getData ( UINT* const pWidth,
                         UINT* const pHeight,
                         uint8_t   **ppData,
                         bool        Wait = false ) = 0;

  void sanitizeFilename (bool allow_subdirs = false);

  struct framebuffer_s
  {
    // One-time alloc, prevents allocating and freeing memory on the thread
    //   that memory-maps the GPU for perfect wait-free capture.
    struct PinnedBuffer {
      std::atomic_size_t           size  = 0L;
      std::unique_ptr <uint8_t []> bytes = nullptr;
    } static root_;

    ~framebuffer_s (void) noexcept
    {
      if (PixelBuffer.get () == root_.bytes.get ())
        PixelBuffer.release (); // Does not free

      PixelBuffer.reset ();
    }

    UINT            Width                = 0UL,
                    Height               = 0UL;
    size_t          PBufferSize          = 0L;
    size_t          PackedDstPitch       = 0L,
                    PackedDstSlicePitch  = 0L;

    struct {
      float         max_cll_nits = 0.0f;
      float         avg_cll_nits = 0.0f;
    } hdr;

    std::unique_ptr
      <uint8_t []>  PixelBuffer          = nullptr;

    union {
      struct { D3DFORMAT       NativeFormat; } d3d9;
      struct { DXGI_FORMAT     NativeFormat;
               DXGI_ALPHA_MODE AlphaMode;    } dxgi, opengl;
    };

    bool            AllowSaveToDisk      = false;
    bool            AllowCopyToClipboard = false;

    std::wstring    file_name;
    std::string     title;
  };

  __inline
  framebuffer_s*
  getFinishedData (void) noexcept
  {
    framebuffer.AllowCopyToClipboard = wantClipboardCopy ();
    framebuffer.AllowSaveToDisk      = wantDiskCopy      ();

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

  __inline bool
  wantClipboardCopy (void) const noexcept
  {
    return bCopyToClipboard;
  }

  __inline bool
  wantDiskCopy (void) const noexcept
  {
    return bSaveToDisk;
  }

protected:
  framebuffer_s
          framebuffer            = { };

  ULONG64 ulCommandIssuedOnFrame = 0;

  bool    bSaveToDisk;
  bool    bPlaySound;
  bool    bCopyToClipboard;
};

void SK_Steam_CatastropicScreenshotFail (void);
void SK_Screenshot_PlaySound            (void);

bool SK_Screenshot_SaveAVIF (DirectX::ScratchImage &src_image, const wchar_t *wszFilePath, uint16_t max_cll, uint16_t max_pall);

void SK_WIC_SetMaximumQuality (IPropertyBag2 *props);
void SK_WIC_SetBasicMetadata  (IWICMetadataQueryWriter *pMQW);
void SK_WIC_SetMetadataTitle  (IWICMetadataQueryWriter *pMQW, std::string& title);

extern volatile LONG __SK_ScreenShot_CapturingHUDless;

extern volatile LONG __SK_D3D11_QueuedShots;
extern volatile LONG __SK_D3D12_QueuedShots;

extern volatile LONG __SK_D3D11_InitiateHudFreeShot;
extern volatile LONG __SK_D3D12_InitiateHudFreeShot;

extern          bool   SK_D3D12_ShouldSkipHUD (void);