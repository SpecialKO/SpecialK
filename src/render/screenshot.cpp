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

#include <SpecialK/stdafx.h>
#include <SpecialK/render/backend.h>
#include <SpecialK/render/d3d9/d3d9_screenshot.h>
#include <SpecialK/render/d3d11/d3d11_screenshot.h>

SK_ScreenshotQueue enqueued_screenshots { 0, 0, 0 };

void SK_Screenshot_ProcessQueue ( SK_ScreenshotStage stage,
                            const SK_RenderBackend&  rb )
{
  if ( gsl::narrow_cast <int> (rb.api) &
       gsl::narrow_cast <int> (SK_RenderAPI::D3D11) )
  {
    SK_D3D11_ProcessScreenshotQueue (stage);
  }

  if ( gsl::narrow_cast <int> (rb.api) &
       gsl::narrow_cast <int> (SK_RenderAPI::D3D9) )
  {
    SK_D3D9_ProcessScreenshotQueue (stage);
  }
}

bool
SK_Screenshot_IsCapturingHUDless (void)
{
  extern volatile
               LONG __SK_ScreenShot_CapturingHUDless;
  if (ReadAcquire (&__SK_ScreenShot_CapturingHUDless))
  {
    return true;
  }

  if ( ReadAcquire (&enqueued_screenshots.pre_game_hud) ||
       ReadAcquire (&enqueued_screenshots.without_sk_osd) )
  {
    return true;
  }

  return false;
}

bool
SK_Screenshot_IsCapturing (void)
{
  return
    ( ReadAcquire (&enqueued_screenshots.stages [0]) > 0 ||
      ReadAcquire (&enqueued_screenshots.stages [1]) > 0 ||
      ReadAcquire (&enqueued_screenshots.stages [2]) > 0   );
}