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

  EndOfFrame    = 2,    // Generally captures all add-on overlays (including the Steam overlay)

  _FlushQueue   = 3     // Causes any screenshots in progress to complete before the next frame,
                        //   typically needed when Alt+Tabbing or resizing the swapchain.
};

struct SK_ScreenshotQueue
{
  union
  {
    // Queue Array
    volatile LONG stages [3];

    struct
    {
      volatile LONG pre_game_hud;

      volatile LONG without_sk_osd;
      volatile LONG with_sk_osd;
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

extern SK_ScreenshotQueue
 enqueued_screenshots;

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
  bool                     copyToClipboard (const DirectX::Image& image) const;

protected:
  screenshot_repository_s screenshots = { };

private:
};
