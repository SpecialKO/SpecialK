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
#include <SpecialK/resource.h>

#include <SpecialK/render/backend.h>
#include <SpecialK/render/gl/opengl_screenshot.h>
#include <SpecialK/render/d3d9/d3d9_screenshot.h>
#include <SpecialK/render/d3d11/d3d11_screenshot.h>
#include <SpecialK/render/d3d12/d3d12_screenshot.h>

#include <filesystem>

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"Screenshot"

SK_Screenshot::framebuffer_s::PinnedBuffer
SK_Screenshot::framebuffer_s::root_;

SK_ScreenshotQueue::MemoryTotals SK_ScreenshotQueue::pooled;
SK_ScreenshotQueue::MemoryTotals SK_ScreenshotQueue::completed;

SK_ScreenshotQueue enqueued_screenshots { 0, 0, 0, 0, 0 };
SK_ScreenshotQueue enqueued_sounds      { 0, 0, 0, 0, 0 };

void SK_Screenshot_PlaySound (void)
{
  if (config.screenshots.play_sound)
  {
    static const auto sound_file =
      std::filesystem::path (SK_GetInstallPath ()) /
           LR"(Assets\Shared\Sounds\screenshot.wav)";

    std::error_code                          ec;
    if (std::filesystem::exists (sound_file, ec))
    {
      SK_PlaySound ( sound_file.c_str (),
                       nullptr, SND_ASYNC |
                                SND_FILENAME );
    }
  }
}

void SK_Screenshot_ProcessQueue ( SK_ScreenshotStage stage,
                            const SK_RenderBackend&  rb = SK_GetCurrentRenderBackend () )
{
  if ( sk::narrow_cast <int> (rb.api) &
       sk::narrow_cast <int> (SK_RenderAPI::D3D12) )
  {
    SK_D3D12_ProcessScreenshotQueue (stage);
  }

  else
  if ( sk::narrow_cast <int> (rb.api) &
       sk::narrow_cast <int> (SK_RenderAPI::D3D11) )
  {
    SK_D3D11_ProcessScreenshotQueue (stage);
  }

  else
  if ( sk::narrow_cast <int> (rb.api) &
       sk::narrow_cast <int> (SK_RenderAPI::D3D9) )
  {
    SK_D3D9_ProcessScreenshotQueue (stage);
  }

  else
  if ( sk::narrow_cast <int> (rb.api) &
       sk::narrow_cast <int> (SK_RenderAPI::OpenGL) )
  {
    SK_GL_ProcessScreenshotQueue (stage);
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

  if ( ReadAcquire (&enqueued_screenshots.pre_game_hud)   > 0 ||
       ReadAcquire (&enqueued_screenshots.without_sk_osd) > 0 )
  {
    return true;
  }

  return false;
}

bool
SK_Screenshot_IsCapturing (void)
{
  return
    ( ReadAcquire (&enqueued_screenshots.stages [0]) +
      ReadAcquire (&enqueued_screenshots.stages [1]) +
      ReadAcquire (&enqueued_screenshots.stages [2]) +
      ReadAcquire (&enqueued_screenshots.stages [3]) +
      ReadAcquire (&enqueued_screenshots.stages [4]) ) > 0;
}

void
SK_ScreenshotManager::init (void) noexcept
{
  __try {
    const auto& repo =
      getRepoStats (true);

    std::ignore = repo;
  }

  __except (EXCEPTION_EXECUTE_HANDLER) {
    // Don't care
  };
}

#include <filesystem>

const wchar_t*
SK_ScreenshotManager::getBasePath (void) const
{
  static wchar_t
    wszAbsolutePathToScreenshots [ MAX_PATH + 2 ] = { };

  if (*wszAbsolutePathToScreenshots != L'\0')
    return wszAbsolutePathToScreenshots;

  if (! config.screenshots.override_path.empty ())
  {
    std::error_code                                                ec;
    if (std::filesystem::exists (config.screenshots.override_path, ec))
    {
      auto path_to_app =
        SK_GetFullyQualifiedApp ();

      std::wstring profile_path =
        std::wstring (LR"(\)") + SK_GetHostApp () + std::wstring (LR"(\XXX)");

      if (! SK_IsInjected ())
      {
        if (app_cache_mgr->loadAppCacheForExe       (path_to_app))
        {
          auto path =
            app_cache_mgr->getConfigPathFromAppPath (path_to_app);

          if (! path._Equal (SK_GetNaiveConfigPath ()))
            profile_path = path;
        }
      }

      else
        profile_path = SK_GetConfigPath ();

      const wchar_t *wszProfileName =
        std::filesystem::path (profile_path).parent_path ().c_str ();

      while (StrStrIW (wszProfileName, LR"(\)") != nullptr)
                       wszProfileName = StrStrIW (wszProfileName, LR"(\)") + 1;

      auto override_path =
        std::filesystem::path (config.screenshots.override_path) / wszProfileName;

      wcsncpy_s    ( wszAbsolutePathToScreenshots, MAX_PATH,
                     override_path.c_str (),       _TRUNCATE );
      wcscat       ( wszAbsolutePathToScreenshots, L"\\"     );
      SK_LOGi0 ( L"@ Using Custom Screenshot Path: %ws",
                     wszAbsolutePathToScreenshots );
      return         wszAbsolutePathToScreenshots;
    }
  }

  wcsncpy_s    ( wszAbsolutePathToScreenshots, MAX_PATH,
                 SK_GetConfigPath (),          _TRUNCATE        );
  PathAppendW  ( wszAbsolutePathToScreenshots, L"Screenshots\\" );

  return         wszAbsolutePathToScreenshots;
}

bool
SK_ScreenshotManager::copyToClipboard (const DirectX::Image& image) const
{
  if (! config.screenshots.copy_to_clipboard)
    return false;

  if (OpenClipboard (nullptr))
  {
    const int
        _bpc    =
      sk::narrow_cast <int> (DirectX::BitsPerPixel (image.format)),
        _width  =
      sk::narrow_cast <int> (                       image.width),
        _height =
      sk::narrow_cast <int> (                       image.height);

    SK_ReleaseAssert (image.format == DXGI_FORMAT_B8G8R8X8_UNORM ||
                      image.format == DXGI_FORMAT_B8G8R8A8_UNORM);

    HBITMAP hBitmapCopy =
       CreateBitmap (
         _width, _height, 1,
           _bpc, image.pixels
       );

    BITMAPINFOHEADER
      bmh                 = { };
      bmh.biSize          = sizeof (BITMAPINFOHEADER);
      bmh.biWidth         =   _width;
      bmh.biHeight        =  -_height;
      bmh.biPlanes        =  1;
      bmh.biBitCount      = sk::narrow_cast <WORD> (_bpc);
      bmh.biCompression   = BI_RGB;
      bmh.biXPelsPerMeter = 10;
      bmh.biYPelsPerMeter = 10;

    BITMAPINFO
      bmi                 = { };
      bmi.bmiHeader       = bmh;

    HDC hdcDIB =
      CreateCompatibleDC (GetDC (nullptr));

    void* bitplane = nullptr;

    HBITMAP
      hBitmap =
        CreateDIBSection ( hdcDIB, &bmi, DIB_RGB_COLORS,
            &bitplane, nullptr, 0 );
    memcpy ( bitplane,
               image.pixels,
        static_cast <size_t> (_bpc / 8) *
        static_cast <size_t> (_width  ) *
        static_cast <size_t> (_height )
           );

    HDC  hdcSrc  = CreateCompatibleDC (GetDC (nullptr));
    HDC  hdcDst  = CreateCompatibleDC (GetDC (nullptr));

    auto hbmpSrc = (HBITMAP)SelectObject (hdcSrc, hBitmap);
    auto hbmpDst = (HBITMAP)SelectObject (hdcDst, hBitmapCopy);

    BitBlt (hdcDst, 0, 0, _width,
                          _height, hdcSrc, 0, 0, SRCCOPY);

    SelectObject     (hdcSrc, hbmpSrc);
    SelectObject     (hdcDst, hbmpDst);

    EmptyClipboard   ();
    SetClipboardData (CF_BITMAP, hBitmapCopy);
    CloseClipboard   ();

    DeleteDC         (hdcSrc);
    DeleteDC         (hdcDst);
    DeleteDC         (hdcDIB);

    DeleteBitmap     (hBitmap);
    DeleteBitmap     (hBitmapCopy);

    return true;
  }

  return false;
}

#if 0
  if (IsClipboardFormatAvailable (CF_BITMAP))
  {

#endif

bool
SK_ScreenshotManager::checkDiskSpace (uint64_t bytes_needed) const
{
  ULARGE_INTEGER
    useable  = { }, // Amount the current user's quota allows
    capacity = { },
    free     = { };

  SK_CreateDirectoriesEx (getBasePath (), false);

  const BOOL bRet =
    GetDiskFreeSpaceExW (
      getBasePath (),
        &useable, &capacity, &free );

  if (! bRet)
  {
    SK_ImGui_Warning (
      SK_FormatStringW (
        L"GetDiskFreeSpaceExW (%ws) Failed\r\n\r\n"
        L"\t>> Reason: %ws",
          getBasePath (),
          _com_error (
            HRESULT_FROM_WIN32 (GetLastError ())
          ).ErrorMessage ()
      ).c_str ()
    );

    return false;
  }

  // Don't take screenshots if the storage for one single screenshot is > 85% of remaining space
  if (static_cast <double> (bytes_needed) > static_cast <double> (useable.QuadPart) * .85)
  {
    SK_ImGui_Warning (L"Not enough Disk Space to take Screenshot, please free up or disable feature.");
    return false;
  }

  // Don't take screenshots if the general free space is < .25%
  if (static_cast <double> (free.QuadPart) / static_cast <double> (capacity.QuadPart) < 0.0025)
  {
    SK_ImGui_Warning (L"Free space on Screenshot Drive is < 0.25%, disabling Screenshots.");
    return false;
  }

  return true;
}

SK_ScreenshotManager::screenshot_repository_s&
SK_ScreenshotManager::getRepoStats (bool refresh)
{
  static auto _ConstructPath =
  [](auto&& path_base, const wchar_t* path_end)
  {
    std::array <wchar_t, MAX_PATH + 1>
                     path { };
    SK_PathCombineW (path.data (), path_base.data (), path_end);
    return           path;
  };

  static bool init = false;

  if (refresh || (! init))
  {
    screenshots.files           = 0;
    screenshots.liSize.QuadPart = 0ULL;

    WIN32_FIND_DATA fd        = {   };
    std::wstring    directory = getBasePath ();
    HANDLE          hFind     =
      FindFirstFileW   (_ConstructPath (directory, L"*").data (), &fd);

    if (hFind != INVALID_HANDLE_VALUE)
    {
      do
      {
        if (         (fd.dwFileAttributes  & FILE_ATTRIBUTE_DIRECTORY) ==
                                             FILE_ATTRIBUTE_DIRECTORY  &&
          (  wcsncmp (fd.cFileName, L"." , MAX_PATH) != 0)             &&
            (wcsncmp (fd.cFileName, L"..", MAX_PATH) != 0) )
        {
          unsigned int
          SK_Steam_RecursiveFileScrub ( std::wstring   directory,
                                        unsigned int  &files,
                                        LARGE_INTEGER &liSize,
                                  const wchar_t       *wszPattern,
                                  const wchar_t       *wszAntiPattern,     // Exact match only
                                        bool           erase = false );

          SK_Steam_RecursiveFileScrub ( _ConstructPath   (
                                          _ConstructPath ( directory, fd.cFileName ),
                                          L""            ).data (),
                                          screenshots.files,
                                          screenshots.liSize,
            L"*",                       L"SK_SteamScreenshotImport.jpg",
            false                     );
        }
      } while (FindNextFile (hFind, &fd));

      FindClose (hFind);

      init = true;
    }
  }

  return screenshots;
}

void
SK_TriggerHudFreeScreenshot (void) noexcept
{
  if (SK_API_IsLayeredOnD3D11 (SK_GetCurrentRenderBackend ().api))
  {
    if (ReadAcquire (&SK_D3D11_TrackingCount->Conditional) > 0)
    {
      extern volatile LONG   __SK_D3D11_QueuedShots;
      InterlockedIncrement (&__SK_D3D11_QueuedShots);
    }
  }

  else if (SK_GetCurrentRenderBackend ().api == SK_RenderAPI::D3D12)
  {
    //if (ReadAcquire (&SK_D3D11_TrackingCount->Conditional) > 0)
    //{
      extern volatile LONG   __SK_D3D12_QueuedShots;
      InterlockedIncrement (&__SK_D3D12_QueuedShots);
    //}
  }
}

SK_Screenshot::SK_Screenshot (bool clipboard_only)
{
  bPlaySound       = config.screenshots.play_sound;
  bCopyToClipboard = config.screenshots.copy_to_clipboard;

  if (clipboard_only)
  {
    bCopyToClipboard = true;
    bSaveToDisk      = false;
  }

  else
    bSaveToDisk = true;

  framebuffer.AllowCopyToClipboard = bCopyToClipboard;
  framebuffer.AllowSaveToDisk      = bSaveToDisk;
}

#include <../depends/include/DirectXTex/d3dx12.h>
#include <../depends/include/avif/avif.h>

using namespace DirectX;

using avifImageCreate_pfn            = avifImage*  (*)(uint32_t width, uint32_t height, uint32_t depth, avifPixelFormat yuvFormat);
using avifRGBImageSetDefaults_pfn    = void        (*)(avifRGBImage* rgb, const avifImage* image);
using avifRGBImageAllocatePixels_pfn = avifResult  (*)(avifRGBImage* rgb);
using avifImageRGBToYUV_pfn          = avifResult  (*)(avifImage* image, const avifRGBImage* rgb);
using avifEncoderCreate_pfn          = avifEncoder*(*)(void);
using avifEncoderAddImage_pfn        = avifResult  (*)(avifEncoder* encoder, const avifImage* image, uint64_t durationInTimescales, avifAddImageFlags addImageFlags);
using avifEncoderFinish_pfn          = avifResult  (*)(avifEncoder* encoder, avifRWData* output);
using avifImageDestroy_pfn           = void        (*)(avifImage* image);
using avifEncoderDestroy_pfn         = void        (*)(avifEncoder* encoder);
using avifRGBImageFreePixels_pfn     = void        (*)(avifRGBImage* rgb);

static avifImageCreate_pfn            SK_avifImageCreate            = nullptr;
static avifRGBImageSetDefaults_pfn    SK_avifRGBImageSetDefaults    = nullptr;
static avifRGBImageAllocatePixels_pfn SK_avifRGBImageAllocatePixels = nullptr;
static avifImageRGBToYUV_pfn          SK_avifImageRGBToYUV          = nullptr;
static avifEncoderCreate_pfn          SK_avifEncoderCreate          = nullptr;
static avifEncoderAddImage_pfn        SK_avifEncoderAddImage        = nullptr;
static avifEncoderFinish_pfn          SK_avifEncoderFinish          = nullptr;
static avifImageDestroy_pfn           SK_avifImageDestroy           = nullptr;
static avifEncoderDestroy_pfn         SK_avifEncoderDestroy         = nullptr;
static avifRGBImageFreePixels_pfn     SK_avifRGBImageFreePixels     = nullptr;

bool
SK_Screenshot_SaveAVIF (DirectX::ScratchImage &src_image, const wchar_t *wszFilePath, uint16_t max_cll, uint16_t max_pall)
{
  static bool init = false;

  SK_RunOnce (
  {
    std::filesystem::path avif_dll =
      SK_GetPlugInDirectory (SK_PlugIn_Type::ThirdParty);

    avif_dll /=
      SK_RunLHIfBitness ( 64, LR"(Image Codecs\libavif\libavif_x64.dll)",
                              LR"(Image Codecs\libavif\libavif_x86.dll)" );

    HMODULE hModAVIF =
      SK_LoadLibraryW (avif_dll.c_str ());

    if (hModAVIF != 0)
    {
      SK_avifImageCreate            = (avifImageCreate_pfn)           SK_GetProcAddress (hModAVIF, "avifImageCreate");
      SK_avifRGBImageSetDefaults    = (avifRGBImageSetDefaults_pfn)   SK_GetProcAddress (hModAVIF, "avifRGBImageSetDefaults");
      SK_avifRGBImageAllocatePixels = (avifRGBImageAllocatePixels_pfn)SK_GetProcAddress (hModAVIF, "avifRGBImageAllocatePixels");
      SK_avifImageRGBToYUV          = (avifImageRGBToYUV_pfn)         SK_GetProcAddress (hModAVIF, "avifImageRGBToYUV");
      SK_avifEncoderCreate          = (avifEncoderCreate_pfn)         SK_GetProcAddress (hModAVIF, "avifEncoderCreate");
      SK_avifEncoderAddImage        = (avifEncoderAddImage_pfn)       SK_GetProcAddress (hModAVIF, "avifEncoderAddImage");
      SK_avifEncoderFinish          = (avifEncoderFinish_pfn)         SK_GetProcAddress (hModAVIF, "avifEncoderFinish");
      SK_avifImageDestroy           = (avifImageDestroy_pfn)          SK_GetProcAddress (hModAVIF, "avifImageDestroy");
      SK_avifEncoderDestroy         = (avifEncoderDestroy_pfn)        SK_GetProcAddress (hModAVIF, "avifEncoderDestroy");
      SK_avifRGBImageFreePixels     = (avifRGBImageFreePixels_pfn)    SK_GetProcAddress (hModAVIF, "avifRGBImageFreePixels");

      init =
        ( SK_avifImageCreate            != nullptr &&
          SK_avifRGBImageSetDefaults    != nullptr &&
          SK_avifRGBImageAllocatePixels != nullptr &&
          SK_avifImageRGBToYUV          != nullptr &&
          SK_avifEncoderCreate          != nullptr &&
          SK_avifEncoderAddImage        != nullptr &&
          SK_avifEncoderFinish          != nullptr &&
          SK_avifImageDestroy           != nullptr &&
          SK_avifEncoderDestroy         != nullptr &&
          SK_avifRGBImageFreePixels     != nullptr );
    }
  });

  if (! init)
    return false;

  auto& metadata =
    src_image.GetMetadata ();

  uint32_t width  = sk::narrow_cast <uint32_t> (metadata.width);
  uint32_t height = sk::narrow_cast <uint32_t> (metadata.height);

  int             bit_depth  = 10;
  avifPixelFormat yuv_format = AVIF_PIXEL_FORMAT_YUV444;

  switch (config.screenshots.avif.yuv_subsampling)
  {
    default:
      config.screenshots.avif.yuv_subsampling = 444; // Write a valid value to INI
      [[fallthrough]];
    case 444:
      yuv_format = AVIF_PIXEL_FORMAT_YUV444;
      break;
    case 422:
      yuv_format = AVIF_PIXEL_FORMAT_YUV422;
      break;
    case 420:
      yuv_format = AVIF_PIXEL_FORMAT_YUV420;
      break;
    case 400: // lol
      yuv_format = AVIF_PIXEL_FORMAT_YUV400;
      break;
  }

  if (metadata.format == DXGI_FORMAT_R16G16B16A16_FLOAT)
  {
    bit_depth =
      std::clamp (config.screenshots.avif.scrgb_bit_depth, 8, 12);

    // 8, 10, 12... nothing else.
    if ( bit_depth == 9 ||
         bit_depth == 11 )
         bit_depth++;

    // Write a valid value back to INI
    config.screenshots.avif.scrgb_bit_depth = bit_depth;
  }

  avifResult rgbToYuvResult = AVIF_RESULT_NO_CONTENT;
  avifResult addResult      = AVIF_RESULT_NO_CONTENT;
  avifResult encodeResult   = AVIF_RESULT_NO_CONTENT;

  avifRWData   avifOutput = AVIF_DATA_EMPTY;
  avifRGBImage rgb        = { };
  avifEncoder* encoder    = nullptr;
  avifImage*   image      =
    SK_avifImageCreate (width, height, bit_depth, yuv_format);

  if (image != nullptr)
  {
    image->yuvRange =
      config.screenshots.avif.full_range ? AVIF_RANGE_FULL
                                         : AVIF_RANGE_LIMITED;

    switch (metadata.format)
    {
      case DXGI_FORMAT_R10G10B10A2_UNORM:
      case DXGI_FORMAT_R16G16B16A16_FLOAT:
        image->colorPrimaries          = AVIF_COLOR_PRIMARIES_BT2020;
        image->transferCharacteristics = AVIF_TRANSFER_CHARACTERISTICS_SMPTE2084;
        image->matrixCoefficients      = AVIF_MATRIX_COEFFICIENTS_BT2020_NCL;
        break;
      default:
        return false;
    }

    SK_avifRGBImageSetDefaults (&rgb, image);

    rgb.depth       = metadata.format == DXGI_FORMAT_R10G10B10A2_UNORM ? 10 : 16;
    rgb.ignoreAlpha = true;
    rgb.isFloat     = false;
    rgb.format      = AVIF_RGB_FORMAT_RGB;

    SK_avifRGBImageAllocatePixels (&rgb);

    switch (metadata.format)
    {
      case DXGI_FORMAT_R10G10B10A2_UNORM:
      {
        uint16_t* rgb_pixels = (uint16_t *)rgb.pixels;

        EvaluateImage ( src_image.GetImages     (),
                        src_image.GetImageCount (),
                        metadata,
        [&](const DirectX::XMVECTOR* pixels, size_t width, size_t y)
        {
          UNREFERENCED_PARAMETER(y);

          for (size_t j = 0; j < width; ++j)
          {
            DirectX::XMVECTOR v = *pixels++;

            *(rgb_pixels++) = static_cast <uint16_t> (v.m128_f32 [0] * 1023.0f);
            *(rgb_pixels++) = static_cast <uint16_t> (v.m128_f32 [1] * 1023.0f);
            *(rgb_pixels++) = static_cast <uint16_t> (v.m128_f32 [2] * 1023.0f);
          }
        } );
      } break;

      case DXGI_FORMAT_R16G16B16A16_FLOAT:
      {
        static const XMMATRIX c_from709to2020 =
        {
          { 0.627225305694944,  0.329476882715808,  0.0432978115892484, 0.0 },
          { 0.0690418812810714, 0.919605681354755,  0.0113524373641739, 0.0 },
          { 0.0163911702607078, 0.0880887513437058, 0.895520078395586,  0.0 },
          { 0.0,                0.0,                0.0,                1.0 }
        };

        struct ParamsPQ
        {
          XMVECTOR N, M;
          XMVECTOR C1, C2, C3;
          XMVECTOR MaxPQ;
        };

        static const ParamsPQ PQ =
        {
          XMVectorReplicate (2610.0 / 4096.0 / 4.0),   // N
          XMVectorReplicate (2523.0 / 4096.0 * 128.0), // M
          XMVectorReplicate (3424.0 / 4096.0),         // C1
          XMVectorReplicate (2413.0 / 4096.0 * 32.0),  // C2
          XMVectorReplicate (2392.0 / 4096.0 * 32.0),  // C3
          XMVectorReplicate (125.0),
        };

        auto LinearToPQ = [](XMVECTOR N)
        {
          XMVECTOR ret;

          ret =
            XMVectorPow (N, PQ.N);

          XMVECTOR nd =
            XMVectorDivide (
               XMVectorAdd (  PQ.C1, XMVectorMultiply (PQ.C2, ret)),
               XMVectorAdd (g_XMOne, XMVectorMultiply (PQ.C3, ret))
            );

          return
            XMVectorPow (nd, PQ.M);
        };

        uint16_t* rgb_pixels = (uint16_t *)rgb.pixels;

        EvaluateImage ( src_image.GetImages     (),
                        src_image.GetImageCount (),
                        metadata,
        [&](const XMVECTOR* pixels, size_t width, size_t y)
        {
          UNREFERENCED_PARAMETER(y);

          for (size_t j = 0; j < width; ++j)
          {
            XMVECTOR  value = pixels [j];
            XMVECTOR nvalue = XMVectorDivide ( XMVector3Transform (   value, c_from709to2020),
                                               XMVector3Transform (PQ.MaxPQ, c_from709to2020) );
                      value = LinearToPQ (XMVectorClamp (nvalue, g_XMZero, g_XMOne));

            *(rgb_pixels++) = static_cast <uint16_t> (value.m128_f32 [0] * 65535.0f);
            *(rgb_pixels++) = static_cast <uint16_t> (value.m128_f32 [1] * 65535.0f);
            *(rgb_pixels++) = static_cast <uint16_t> (value.m128_f32 [2] * 65535.0f);
          }
        } );
      } break;
    }

    rgbToYuvResult =
      SK_avifImageRGBToYUV (image, &rgb);
  }

  if (rgbToYuvResult == AVIF_RESULT_OK)
  {
    encoder =
      SK_avifEncoderCreate ();

    if (encoder != nullptr)
    {
      encoder->quality         = config.screenshots.compression_quality;
      encoder->qualityAlpha    = config.screenshots.compression_quality; // N/A?
      encoder->timescale       = 1;
      encoder->repetitionCount = AVIF_REPETITION_COUNT_INFINITE;
      encoder->maxThreads      = config.screenshots.avif.max_threads;
      encoder->speed           = config.screenshots.avif.compression_speed;
      encoder->minQuantizer    = AVIF_QUANTIZER_BEST_QUALITY;
      encoder->maxQuantizer    = AVIF_QUANTIZER_BEST_QUALITY;
      encoder->codecChoice     = AVIF_CODEC_CHOICE_AUTO;

      image->clli.maxCLL  = max_cll;
      image->clli.maxPALL = max_pall;

      addResult    = SK_avifEncoderAddImage (encoder, image, 1, AVIF_ADD_IMAGE_FLAG_SINGLE);
      encodeResult = SK_avifEncoderFinish   (encoder, &avifOutput);
    }
  }

  if ( rgbToYuvResult != AVIF_RESULT_OK ||
       addResult      != AVIF_RESULT_OK ||
       encodeResult   != AVIF_RESULT_OK )
  {
    SK_LOGi0 (L"rgbToYUV: %d, addImage: %d, encode: %d", rgbToYuvResult, addResult, encodeResult);
  }

  if (encodeResult == AVIF_RESULT_OK)
  {
    wchar_t    wszAVIFPath [MAX_PATH + 2] = { };
    wcsncpy_s (wszAVIFPath, wszFilePath, MAX_PATH);

    PathRemoveExtensionW (wszAVIFPath);
    PathAddExtensionW    (wszAVIFPath, L".avif");

    FILE* fAVIF =
      _wfopen (wszAVIFPath, L"wb");

    if (fAVIF != nullptr)
    {
      fwrite (avifOutput.data, 1, avifOutput.size, fAVIF);
      fclose (fAVIF);
    }
  }

  if (image   != nullptr) SK_avifImageDestroy   (image);
  if (encoder != nullptr) SK_avifEncoderDestroy (encoder);

  SK_avifRGBImageFreePixels (&rgb);

  return
    ( encodeResult == AVIF_RESULT_OK );
}

void
SK_WIC_SetMaximumQuality (IPropertyBag2 *props)
{
  if (props == nullptr)
    return;

  PROPBAG2 opt = {   .pstrName = L"ImageQuality"   };
  VARIANT  var = { VT_R4,0,0,0, { .fltVal = 1.0f } };

  props->Write (1, &opt, &var);
}

void
SK_WIC_SetBasicMetadata (IWICMetadataQueryWriter *pMQW)
{
  extern std::string SK_GetFriendlyAppName (void);

  PROPVARIANT       value;
  PropVariantInit (&value);

  std::string app_name =
    SK_GetFriendlyAppName ();

  value.vt     = VT_LPSTR;
  value.pszVal = const_cast <char *> (app_name.c_str ());

  pMQW->SetMetadataByName (
    L"System.ApplicationName",
    &value
  );

  UINT SK_DPI_Update (void);
  UINT dpi =
    SK_DPI_Update ();

  value.vt     = VT_R8;
  value.dblVal = static_cast <double> (dpi);

  pMQW->SetMetadataByName (
    L"System.Image.HorizontalResolution",
    &value
  );

  pMQW->SetMetadataByName (
    L"System.Image.VerticalResolution",
    &value
  );

  value.vt     = VT_LPSTR;
  value.pszVal = "Special K";

  pMQW->SetMetadataByName (
    L"System.Media.CreatorApplication",
    &value
  );

  value.vt     = VT_LPSTR;
  value.pszVal = const_cast <char *> (SK_VersionStrA);

  pMQW->SetMetadataByName (
    L"System.Media.CreatorApplicationVersion",
    &value
  );

  value.vt     = VT_LPSTR;
  value.pszVal = const_cast <char *> (
    SK_FormatString ("Captured using Special K %hs", SK_VersionStrA).c_str ()
  );

  pMQW->SetMetadataByName (
    L"System.Comment",
    &value
  );

  value.vt = VT_DATE;

  SYSTEMTIME      st;
  GetSystemTime (&st);

  SystemTimeToVariantTime (&st, &value.dblVal);

  pMQW->SetMetadataByName (
    L"System.Photo.DateTaken",
    &value
  );

  extern void SK_Platform_GetUserName (char *pszName, int max_len);

  static char                          szName [512] = { };
  SK_RunOnce (SK_Platform_GetUserName (szName, 511));

  if (*szName != '\0')
  {
    char* names [] = { szName };

    value.vt             = VT_VECTOR | VT_LPSTR;
    value.calpstr.cElems = 1;
    value.calpstr.pElems = names;

    pMQW->SetMetadataByName (
      L"System.Author",
      &value
    );
  }
}