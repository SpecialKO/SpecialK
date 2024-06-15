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

SK_ScreenshotQueue      enqueued_screenshots { 0,   0,  0,  0,  0 };
SK_ScreenshotQueue      enqueued_sounds      { 0,   0,  0,  0,  0 };
SK_ScreenshotTitleQueue enqueued_titles      { "", "", "", "", "" };

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
SK_HDR_ConvertImageToPNG (const DirectX::Image& raw_hdr_img, DirectX::ScratchImage& png_img)
{
  using namespace DirectX;

  if (auto typeless_fmt = DirectX::MakeTypeless (raw_hdr_img.format);
           typeless_fmt == DXGI_FORMAT_R10G10B10A2_TYPELESS  ||
           typeless_fmt == DXGI_FORMAT_R16G16B16A16_TYPELESS ||
           typeless_fmt == DXGI_FORMAT_R32G32B32A32_TYPELESS)
  {
    if (png_img.GetImageCount () == 0)
    {
      if (FAILED (png_img.Initialize2D (DXGI_FORMAT_R16G16B16A16_UNORM,
              raw_hdr_img.width,
              raw_hdr_img.height, 1, 1)))
      {
        return false;
      }
    }

    if (png_img.GetMetadata ().format != DXGI_FORMAT_R16G16B16A16_UNORM)
      return false;

    uint16_t* rgb16_pixels =
      reinterpret_cast <uint16_t *> (png_img.GetPixels ());

    if (rgb16_pixels == nullptr)
      return false;

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

    static const XMMATRIX c_from709to2020 =
    {
      { 0.627225305694944f,  0.0690418812810714f, 0.0163911702607078f, 0.0f },
      { 0.329476882715808f,  0.919605681354755f,  0.0880887513437058f, 0.0f },
      { 0.0432978115892484f, 0.0113524373641739f, 0.895520078395586f,  0.0f },
      { 0.0f,                0.0f,                0.0f,                1.0f }
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

    EvaluateImage ( raw_hdr_img,
    [&](const XMVECTOR* pixels, size_t width, size_t y)
    {
      UNREFERENCED_PARAMETER(y);

      static const XMVECTOR pq_range_10bpc = XMVectorReplicate (1023.0f),
                            pq_range_12bpc = XMVectorReplicate (4095.0f),
                            pq_range_16bpc = XMVectorReplicate (65535.0f),
                            pq_range_32bpc = XMVectorReplicate (4294967295.0f);

      const auto pq_range_out =
        (typeless_fmt == DXGI_FORMAT_R10G10B10A2_TYPELESS)  ? pq_range_10bpc :
        (typeless_fmt == DXGI_FORMAT_R16G16B16A16_TYPELESS) ? pq_range_12bpc :
                                                              pq_range_12bpc;//pq_range_16bpc;
      const auto pq_range_in  =
        (typeless_fmt == DXGI_FORMAT_R10G10B10A2_TYPELESS)  ? pq_range_10bpc :
        (typeless_fmt == DXGI_FORMAT_R16G16B16A16_TYPELESS) ? pq_range_16bpc :
                                                              pq_range_32bpc;

      int intermediate_bits = 16;
      int output_bits       = 
        (typeless_fmt == DXGI_FORMAT_R10G10B10A2_TYPELESS)  ? 10 :
        (typeless_fmt == DXGI_FORMAT_R16G16B16A16_TYPELESS) ? 12 :
                                                              12;//16;

      for (size_t j = 0; j < width; ++j)
      {
        XMVECTOR v =
          *pixels++;

        // Assume scRGB for any FP32 input, though uncommon
        if (typeless_fmt == DXGI_FORMAT_R16G16B16A16_TYPELESS ||
            typeless_fmt == DXGI_FORMAT_R32G32B32A32_TYPELESS)
        {
          XMVECTOR  value = XMVector3Transform (v, c_from709to2020);
          XMVECTOR nvalue = XMVectorDivide     ( XMVectorMax (g_XMZero,
                                                   XMVectorMin (value, PQ.MaxPQ)),
                                                                       PQ.MaxPQ );
                        v = LinearToPQ (nvalue);
        }

        v = // Quantize to 10- or 12-bpc before expanding to 16-bpc in order to improve
          XMVectorRound ( // compression efficiency
            XMVectorMultiply (
              XMVectorSaturate (v), pq_range_out));

        *(rgb16_pixels++) =
          static_cast <uint16_t> (DirectX::XMVectorGetX (v)) << (intermediate_bits - output_bits);
        *(rgb16_pixels++) =
          static_cast <uint16_t> (DirectX::XMVectorGetY (v)) << (intermediate_bits - output_bits);
        *(rgb16_pixels++) =
          static_cast <uint16_t> (DirectX::XMVectorGetZ (v)) << (intermediate_bits - output_bits);
          rgb16_pixels++; // We have an unused alpha channel that needs skipping
      }
    });
  }

  return true;
}

bool
SK_HDR_SavePNGToDisk (const wchar_t* wszPNGPath, const DirectX::Image* png_image,
                                                 const DirectX::Image* raw_image,
                         const char* szUtf8MetadataTitle)
{
  if ( wszPNGPath == nullptr ||
        png_image == nullptr ||
        raw_image == nullptr )
  {
    return false;
  }

  std::string metadata_title (
         szUtf8MetadataTitle != nullptr ?
         szUtf8MetadataTitle            :
         "HDR10 PNG" );

  if (SUCCEEDED (
    DirectX::SaveToWICFile (*png_image, DirectX::WIC_FLAGS_NONE,
                           GetWICCodec (DirectX::WIC_CODEC_PNG),
                               wszPNGPath, &GUID_WICPixelFormat48bppRGB,
                                              SK_WIC_SetMaximumQuality,
                                            [&](IWICMetadataQueryWriter *pMQW)
                                            {
                                              SK_WIC_SetMetadataTitle (pMQW, metadata_title);
                                            })))
  {
    return
      SK_PNG_MakeHDR (wszPNGPath, *png_image, *raw_image);
  }

  return false;
}

// The parameters are screwy here because currently the only successful way
//   of doing this copy involves passing the path to a file, but the intention
//     is actually to pass raw image data and transfer it using OLE.
bool
SK_PNG_CopyToClipboard (const DirectX::Image& image, const void *pData, size_t data_size)
{
  std::ignore = image;

  SK_ReleaseAssert (data_size <= DWORD_MAX);

  if (OpenClipboard (nullptr))
  {
    int clpSize = sizeof (DROPFILES);

    clpSize += sizeof (wchar_t) * static_cast <int> (wcslen ((wchar_t *)pData) + 1); // + 1 => '\0'
    clpSize += sizeof (wchar_t);                                                     // two \0 needed at the end

    HDROP hdrop =
      (HDROP)GlobalAlloc (GHND, clpSize);

    DROPFILES* df =
      (DROPFILES *)GlobalLock (hdrop);

    df->pFiles = sizeof (DROPFILES);
    df->fWide  = TRUE;

    wcscpy ((wchar_t*)&df [1], (const wchar_t *)pData);

    GlobalUnlock     (hdrop);
    EmptyClipboard   ();
    SetClipboardData (CF_HDROP, hdrop);
    CloseClipboard   ();

    return true;
  }

  return false;
}

bool
SK_ScreenshotManager::copyToClipboard ( const DirectX::Image& image,
                                        const DirectX::Image* pOptionalHDR,
                                        const wchar_t*        wszOptionalFilename ) const
{
  if (! config.screenshots.copy_to_clipboard)
    return false;

  if (pOptionalHDR != nullptr && config.screenshots.allow_hdr_clipboard)
  {
    auto snip = 
      getSnipRect ();

    const DirectX::Image *pImg = pOptionalHDR;
    DirectX::ScratchImage sub_img;

    if (snip.w != 0 && snip.h != 0)
    {
      if (SUCCEEDED (sub_img.Initialize2D (pImg->format, snip.w, snip.h, 1, 1)))
      {
        if (SUCCEEDED (DirectX::CopyRectangle (*pOptionalHDR, snip, *sub_img.GetImage (0,0,0), 0, 0, 0)))
        {
          pImg =
            sub_img.GetImages ();

          SK_GetCurrentRenderBackend ().screenshot_mgr->setSnipRect ({0,0,0,0});
        }
      }
    }


    DirectX::ScratchImage                hdr10_img;
    if (SK_HDR_ConvertImageToPNG (*pImg, hdr10_img))
    {
      wchar_t                   wszPNGPath [MAX_PATH + 2] = { };
      if (                                  wszOptionalFilename != nullptr)
      { wcsncpy_s (             wszPNGPath, wszOptionalFilename, MAX_PATH);
        PathRemoveExtensionW   (wszPNGPath);
        PathRemoveFileSpecW    (wszPNGPath);
      } else {
        wcsncpy_s              (wszPNGPath, getBasePath (), MAX_PATH);
      }
      PathAppendW              (wszPNGPath, L"hdr10_clipboard");
      PathAddExtensionW        (wszPNGPath, L".png");
      if (SK_HDR_SavePNGToDisk (wszPNGPath, hdr10_img.GetImages (), pOptionalHDR))
      {
        if (SK_PNG_CopyToClipboard (*hdr10_img.GetImage (0,0,0), wszPNGPath, 0))
        {
          return true;
        }
      }
    }
  }

  if (OpenClipboard (nullptr))
  {
    auto snip = 
      getSnipRect ();

    const DirectX::Image *pImg = &image;
    DirectX::ScratchImage sub_img;

    if (snip.w != 0 && snip.h != 0)
    {
      if (SUCCEEDED (sub_img.Initialize2D (pImg->format, snip.w, snip.h, 1, 1)))
      {
        if (SUCCEEDED (DirectX::CopyRectangle (image, snip, *sub_img.GetImage (0,0,0), 0, 0, 0)))
        {
          pImg =
            sub_img.GetImages ();

          SK_GetCurrentRenderBackend ().screenshot_mgr->setSnipRect ({0,0,0,0});
        }
      }
    }

    const int
        _bpc    =
      sk::narrow_cast <int> (DirectX::BitsPerPixel (pImg->format)),
        _width  =
      sk::narrow_cast <int> (                       pImg->width),
        _height =
      sk::narrow_cast <int> (                       pImg->height);

    SK_ReleaseAssert (pImg->format == DXGI_FORMAT_B8G8R8X8_UNORM ||
                      pImg->format == DXGI_FORMAT_B8G8R8A8_UNORM);

    HBITMAP hBitmapCopy =
       CreateBitmap (
         _width, _height, 1,
           _bpc, pImg->pixels
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
               pImg->pixels,
        static_cast <size_t> (_bpc / 8) *
        static_cast <size_t> (_width  ) *
        static_cast <size_t> (_height )
           );

    HDC hdcSrc = CreateCompatibleDC (GetDC (nullptr));
    HDC hdcDst = CreateCompatibleDC (GetDC (nullptr));

    if ( hBitmap    != nullptr &&
        hBitmapCopy != nullptr )
    {
      auto hbmpSrc = (HBITMAP)SelectObject (hdcSrc, hBitmap);
      auto hbmpDst = (HBITMAP)SelectObject (hdcDst, hBitmapCopy);

      BitBlt (hdcDst, 0, 0, _width,
                            _height, hdcSrc, 0, 0, SRCCOPY);

      SelectObject     (hdcSrc, hbmpSrc);
      SelectObject     (hdcDst, hbmpDst);

      EmptyClipboard   ();
      SetClipboardData (CF_BITMAP, hBitmapCopy);
    }

    CloseClipboard   ();

    DeleteDC         (hdcSrc);
    DeleteDC         (hdcDst);
    DeleteDC         (hdcDIB);

    if ( hBitmap     != nullptr &&
         hBitmapCopy != nullptr )
    {
      DeleteBitmap   (hBitmap);
      DeleteBitmap   (hBitmapCopy);

      return true;
    }

    return false;
  }

  return false;
}

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
      InterlockedIncrement (&__SK_D3D11_QueuedShots);
    }
  }

  else if (SK_GetCurrentRenderBackend ().api == SK_RenderAPI::D3D12)
  {
    //if (ReadAcquire (&SK_D3D11_TrackingCount->Conditional) > 0)
    //{
      InterlockedIncrement (&__SK_D3D12_QueuedShots);
    //}
  }
}

void
SK_Screenshot::sanitizeFilename (bool allow_subdirs)
{
  const wchar_t *wszInvalidFileChars =
    allow_subdirs ?   LR"(:*?"<>|&!.	)"
                  : LR"(\/:*?"<>|&!.	)";

  static const
    std::unordered_map <wchar_t, wchar_t> fs_safe_alt_chars =
    {
      { L'\\',L'⧵' }, { L'/', L'⧸'  }, { L':', L'ː'  },
      { L'|', L'｜'}, { L'<', L'﹤' }, { L'>', L'﹥' },
      { L'"', L'“' }, { L'?', L'﹖' }, { L'*', L'﹡' },
      { L'.', L'․' }, { L'	', L' ' }, { L'&', L'＆' },
      { L'!', L'！' }
    };

   size_t start = 0; // Remove (replace with _) invalid filesystem characters
  while ((start = framebuffer.file_name.find_first_of (wszInvalidFileChars, start)) != std::wstring::npos)
  {
    if (fs_safe_alt_chars.contains (framebuffer.file_name [start]))
    {                               framebuffer.file_name [start] =
              fs_safe_alt_chars.at (framebuffer.file_name [start]);
    } else {                        framebuffer.file_name [start] = L'_'; }

    ++start;
  }

          start = 0; // Consolidate all runs of multiple _'s into a single _
  while ((start = framebuffer.file_name.find (L"__", start)) != std::wstring::npos)
  {
    framebuffer.file_name.replace (start++, 2, L"_");
  }

          start = 0; // If we have _ followed by a space, transform it to a single space
  while ((start = framebuffer.file_name.find (L"_ ", start)) != std::wstring::npos)
  {
    framebuffer.file_name.replace (start++, 2, L" ");
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

  std::wstring format_str = config.screenshots.filename_format;

  const std::wstring wide_name =
    SK_UTF8ToWideChar (SK_GetFriendlyAppName ());

  size_t  start = 0;
  while ((start = format_str.find (L"%G", start)) != std::wstring::npos)
  {
    format_str.replace (start, 2, wide_name.c_str ());
                        start  += wide_name.length ();
  }

  wchar_t   name [MAX_PATH] = { };   time_t   now;      time (&now);
  wcsftime (name, MAX_PATH-1, format_str.c_str (), localtime (&now));

  framebuffer.file_name = name;

  sanitizeFilename ();
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
            DirectX::XMVECTOR v =
              XMVectorSaturate (*pixels++);

            *(rgb_pixels++) = static_cast <uint16_t> (roundf (XMVectorGetX (v) * 1023.0f));
            *(rgb_pixels++) = static_cast <uint16_t> (roundf (XMVectorGetY (v) * 1023.0f));
            *(rgb_pixels++) = static_cast <uint16_t> (roundf (XMVectorGetZ (v) * 1023.0f));
          }
        } );
      } break;

      case DXGI_FORMAT_R16G16B16A16_FLOAT:
      {
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

        static const XMMATRIX c_from709to2020 = // Transposed
        {
          { 0.627225305694944f,  0.0690418812810714f, 0.0163911702607078f, 0.0f },
          { 0.329476882715808f,  0.919605681354755f,  0.0880887513437058f, 0.0f },
          { 0.0432978115892484f, 0.0113524373641739f, 0.895520078395586f,  0.0f },
          { 0.0f,                0.0f,                0.0f,                1.0f }
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

        DirectX::ScratchImage
          hdr10_img;
          hdr10_img.InitializeFromImage (*src_image.GetImages ());


        EvaluateImage ( src_image.GetImages     (),
                        src_image.GetImageCount (),
                        metadata,
        [&](_In_reads_ (width) const XMVECTOR* pixels, size_t width, size_t y)
        {
          UNREFERENCED_PARAMETER (y);

          for (size_t j = 0; j < width; ++j)
          {
            XMVECTOR  value = XMVector3Transform (pixels [j], c_from709to2020);
            XMVECTOR nvalue = XMVectorDivide ( XMVectorMax (g_XMZero,
                                               XMVectorMin (value, PQ.MaxPQ)),
                                                                   PQ.MaxPQ);

                      value = XMVectorSaturate (LinearToPQ (nvalue));

            *(rgb_pixels++) = static_cast <uint16_t> (roundf (XMVectorGetX (value) * 65535.0f));
            *(rgb_pixels++) = static_cast <uint16_t> (roundf (XMVectorGetY (value) * 65535.0f));
            *(rgb_pixels++) = static_cast <uint16_t> (roundf (XMVectorGetZ (value) * 65535.0f));
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
SK_WIC_SetMetadataTitle (IWICMetadataQueryWriter *pMQW, std::string& title)
{
  SK_WIC_SetBasicMetadata (pMQW);

  if (! title.empty ())
  {
    PROPVARIANT       value;
    PropVariantInit (&value);

    value.vt     = VT_LPSTR;
    value.pszVal = const_cast <char *> (title.c_str ());

    pMQW->SetMetadataByName (
      L"System.Title",
      &value
    );
  }
}

void
SK_WIC_SetBasicMetadata (IWICMetadataQueryWriter *pMQW)
{
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
  );;

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

  if (config.screenshots.embed_nickname)
  {
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
}


uint32_t png_crc_table [256] = { };

// Stores a running CRC (initialized with the CRC of "IDAT" string). When
// you write this to the PNG, write as a big-endian value
//static uint idatCrc = Crc32(new byte[] { (byte)'I', (byte)'D', (byte)'A', (byte)'T' }, 0, 4, 0);

uint32_t
png_crc32 (const void* typeless_data, size_t offset, size_t len, uint32_t crc)
{
  auto data =
    (const BYTE *)typeless_data;

  uint32_t c;

  if (png_crc_table [0] == 0)
  {
    for (auto i = 0 ; i < 256 ; ++i)
    {
      c = i;

      for (auto j = 0 ; j < 8 ; ++j)
      {
        if ((c & 1) == 1)
          c = (0xEDB88320 ^ ((c >> 1) & 0x7FFFFFFF));
        else
          c = ((c >> 1) & 0x7FFFFFFF);
      }

      png_crc_table [i] = c;
    }
  }

  c =
    (crc ^ 0xffffffff);

  for (auto k = offset ; k < (offset + len) ; ++k)
  {
    c =
      png_crc_table [(c ^ data [k]) & 255] ^
                    ((c >> 8)       & 0xFFFFFF);
  }
  
  return
    (c ^ 0xffffffff);
}

/* This is for compression type. PNG 1.0-1.2 only define the single type. */
constexpr uint8_t PNG_COMPRESSION_TYPE_BASE = 0; /* Deflate method 8, 32K window */
#define PNG_COMPRESSION_TYPE_DEFAULT PNG_COMPRESSION_TYPE_BASE

//
// To convert an image passed to an encoder that does not understand HDR,
//   but that we actually fed HDR pixels to... perform the following:
//
//  1. Remove gAMA chunk  (Prevents SKIV from recognizing as HDR)
//  2. Remove sRGB chunk  (Prevents Discord from rendering in HDR)
//
//  3. Add cICP  (The primary way of defining HDR10) 
//  4. Add iCCP  (Required for Discord to render in HDR)
//
//  (5) Add cLLi  [Unnecessary, but probably a good idea]
//  (6) Add cHRM  [Unnecessary, but probably a good idea]
//

struct SK_PNG_HDR_cHRM_Payload
{
  uint32_t white_x = 31270;
  uint32_t white_y = 32900;
  uint32_t red_x   = 70800;
  uint32_t red_y   = 29200;
  uint32_t green_x = 17000;
  uint32_t green_y = 79700;
  uint32_t blue_x  = 13100;
  uint32_t blue_y  = 04600;
};

struct SK_PNG_HDR_sBIT_Payload
{
  uint8_t red_bits   = 10; // 12 if source was scRGB (compression optimization)
  uint8_t green_bits = 10; // 12 if source was scRGB (compression optimization)
  uint8_t blue_bits  = 10; // 12 if source was scRGB (compression optimization)
  uint8_t alpha_bits =  1; // Spec says it must be > 0... :shrug:
};

struct SK_PNG_HDR_mDCv_Payload
{
  struct {
    uint32_t red_x   = 35400; // 0.708 / 0.00002
    uint32_t red_y   = 14600; // 0.292 / 0.00002
    uint32_t green_x =  8500; // 0.17  / 0.00002
    uint32_t green_y = 39850; // 0.797 / 0.00002
    uint32_t blue_x  =  6550; // 0.131 / 0.00002
    uint32_t blue_y  =  2300; // 0.046 / 0.00002
  } primaries;

  struct {
    uint32_t x       = 15635; // 0.3127 / 0.00002
    uint32_t y       = 16450; // 0.3290 / 0.00002
  } white_point;

  // The only real data we need to fill-in
  struct {
    uint32_t maximum = 10000000; // 1000.0 cd/m^2
    uint32_t minimum = 1;        // 0.0001 cd/m^2
  } luminance;
};

struct SK_PNG_HDR_cLLi_Payload
{
  uint32_t max_cll  = 10000000; // 1000 / 0.0001
  uint32_t max_fall =  2500000; //  250 / 0.0001
};

struct SK_PNG_HDR_iCCP_Payload
{
  char          profile_name [20]   = "RGB_D65_202_Rel_PeQ";
  uint8_t       compression_type    = PNG_COMPRESSION_TYPE_DEFAULT;
  unsigned char profile_data [2195] =
  {
    0x58, 0x85, 0xED, 0x98, 0x79, 0x54, 0x53, 0x57, 0x1E, 0xC7, 0x7F, 0x80,
    0xCA, 0xA8, 0x54, 0x61, 0x6C, 0xA7, 0x1D, 0x44, 0xA5, 0x02, 0x01, 0x11,
    0x6A, 0xD8, 0x5C, 0x40, 0x14, 0x50, 0xC1, 0xAD, 0x1A, 0x41, 0x94, 0x1D,
    0x21, 0x84, 0x6D, 0x04, 0x21, 0x01, 0x12, 0xB6, 0xF2, 0x40, 0x45, 0x04,
    0x59, 0x13, 0x96, 0x20, 0x21, 0x6C, 0x09, 0x8B, 0xAC, 0xB2, 0x25, 0x20,
    0x9B, 0x10, 0x96, 0x84, 0x80, 0x12, 0x44, 0xAA, 0xD5, 0xA9, 0x22, 0xB8,
    0x6B, 0x08, 0xB8, 0xCC, 0x58, 0xA4, 0x27, 0xA1, 0x96, 0xD0, 0xE9, 0xD8,
    0x73, 0xE6, 0xCC, 0x7F, 0xF6, 0x9E, 0xCF, 0x1F, 0xBF, 0x77, 0xBF, 0xF7,
    0xDD, 0xF3, 0x3E, 0xEF, 0xBE, 0x3F, 0xEE, 0x7D, 0x00, 0x8A, 0x6F, 0x7D,
    0x49, 0x27, 0x55, 0x17, 0x99, 0x01, 0xF8, 0xF9, 0x07, 0xE1, 0xAD, 0xAD,
    0x2C, 0x54, 0x0F, 0xBA, 0xB9, 0xAB, 0xCA, 0x8F, 0x81, 0x02, 0xC8, 0x80,
    0xA4, 0xB9, 0x61, 0x09, 0x01, 0xE6, 0x18, 0xCC, 0x41, 0xF8, 0xFD, 0x26,
    0x03, 0xF0, 0x7A, 0x64, 0x6E, 0xAC, 0x40, 0x57, 0x3C, 0xD7, 0x9A, 0xE0,
    0x7D, 0x49, 0xCA, 0xD3, 0xCD, 0xC7, 0x37, 0x2F, 0x8F, 0x7F, 0xF9, 0x15,
    0x5A, 0xE7, 0x34, 0x7C, 0xBC, 0x2D, 0xF3, 0xC0, 0x11, 0xB0, 0x00, 0xF0,
    0x13, 0x00, 0xEC, 0xC6, 0x06, 0xE0, 0x83, 0x00, 0x64, 0xCC, 0x00, 0x40,
    0x9D, 0x18, 0x14, 0x20, 0xAE, 0x3D, 0x00, 0xE0, 0x73, 0xAC, 0xB7, 0x9B,
    0x07, 0x80, 0x0C, 0x09, 0x00, 0x74, 0xB0, 0x3E, 0xD8, 0x00, 0x00, 0x99,
    0x22, 0x00, 0x50, 0xC0, 0xDB, 0xD9, 0x3B, 0x00, 0xC8, 0x54, 0x8B, 0xC7,
    0x78, 0xCD, 0xD5, 0x5D, 0xE2, 0xDA, 0x7D, 0xAE, 0x1E, 0x15, 0xD7, 0xE6,
    0xFA, 0x16, 0x68, 0x00, 0x99, 0x67, 0x00, 0x2B, 0xAE, 0x58, 0xE8, 0x9B,
    0xA3, 0x01, 0x14, 0x4B, 0x00, 0x00, 0xE3, 0x77, 0x32, 0x18, 0x3B, 0xEF,
    0x00, 0x0A, 0x38, 0x7F, 0x5B, 0x1B, 0x00, 0x40, 0x01, 0x80, 0x0A, 0x58,
    0x83, 0x15, 0x58, 0x80, 0x2B, 0xEC, 0x86, 0xCD, 0x60, 0x04, 0xAE, 0xA0,
    0x0F, 0x68, 0xD0, 0x07, 0x57, 0xB0, 0x06, 0x1C, 0x9C, 0x04, 0x57, 0xC0,
    0x00, 0x0E, 0x8E, 0x00, 0xFC, 0x97, 0x39, 0x96, 0x48, 0xE6, 0xD8, 0x05,
    0xBB, 0x00, 0x0D, 0x60, 0x67, 0xEF, 0xA0, 0x3A, 0x37, 0x64, 0xFE, 0x3D,
    0x11, 0x3C, 0x0D, 0xF4, 0xE7, 0xEE, 0x52, 0x30, 0x03, 0x58, 0x7C, 0x67,
    0x76, 0x76, 0x4A, 0x1B, 0x40, 0x3E, 0x0D, 0x60, 0x26, 0x65, 0x76, 0xF6,
    0x5D, 0xE1, 0xEC, 0xEC, 0x4C, 0x21, 0x80, 0xDC, 0x6D, 0x80, 0xF6, 0xE8,
    0x39, 0x5F, 0x80, 0xA5, 0x8A, 0x20, 0x33, 0x3F, 0x57, 0xB9, 0x37, 0x80,
    0xD5, 0xA9, 0xD9, 0xD9, 0xD9, 0x98, 0xF9, 0x3E, 0x6D, 0x1F, 0x80, 0xF2,
    0x31, 0x00, 0xF9, 0x8A, 0xF9, 0x3E, 0x55, 0x79, 0x80, 0xE5, 0x95, 0x00,
    0xFD, 0xAB, 0xFC, 0x3C, 0x83, 0xF4, 0xC4, 0x3D, 0x72, 0x72, 0x4B, 0xE7,
    0x9E, 0xF6, 0x37, 0xAB, 0xF8, 0x1F, 0xD7, 0x32, 0xB2, 0x72, 0x8B, 0x16,
    0x2F, 0x91, 0xFF, 0xCB, 0xD2, 0x65, 0xCB, 0x15, 0x3E, 0x5B, 0xB1, 0x52,
    0x51, 0xE9, 0xAF, 0xAB, 0x3E, 0xFF, 0xE2, 0x6F, 0x5F, 0x7E, 0xF5, 0x77,
    0xE5, 0xD5, 0x2A, 0x6B, 0xD6, 0xAE, 0x53, 0xFD, 0x7A, 0xBD, 0x9A, 0xBA,
    0x06, 0x4A, 0x53, 0x6B, 0x83, 0xF6, 0x46, 0x1D, 0xDD, 0x6F, 0x36, 0xA1,
    0xF5, 0xF4, 0x0D, 0x0C, 0x8D, 0x36, 0x6F, 0xD9, 0xBA, 0xCD, 0xD8, 0x64,
    0xBB, 0xE9, 0x8E, 0x9D, 0x66, 0xE6, 0x16, 0xBB, 0x76, 0xEF, 0xB1, 0xB4,
    0xDA, 0xBB, 0x6F, 0xFF, 0x81, 0x83, 0xDF, 0x1E, 0x3A, 0x8C, 0x39, 0x62,
    0x6D, 0x73, 0xD4, 0xF6, 0xD8, 0x71, 0x3B, 0x7B, 0x07, 0x47, 0x27, 0x67,
    0x17, 0xD7, 0x13, 0x6E, 0xEE, 0x58, 0x0F, 0x9C, 0xA7, 0x97, 0xB7, 0x8F,
    0xEF, 0x3F, 0x4E, 0xFA, 0xF9, 0x9F, 0x0A, 0x08, 0xC4, 0x13, 0x82, 0x82,
    0x43, 0x88, 0xA4, 0xD0, 0xB0, 0xF0, 0x88, 0xC8, 0xEF, 0xA2, 0x90, 0xE8,
    0x98, 0xD3, 0x67, 0xCE, 0xC6, 0x9E, 0x8B, 0x3B, 0x1F, 0x9F, 0x70, 0x21,
    0x31, 0x29, 0x39, 0x25, 0x35, 0x8D, 0x4C, 0x49, 0xCF, 0xC8, 0xCC, 0xA2,
    0x66, 0x5F, 0xCC, 0xA1, 0xE5, 0xD2, 0xF3, 0xF2, 0x0B, 0x0A, 0x8B, 0x18,
    0xCC, 0xE2, 0x92, 0xD2, 0xB2, 0x4B, 0xE5, 0x15, 0x95, 0x55, 0xD5, 0x35,
    0x97, 0x6B, 0xEB, 0xEA, 0x1B, 0x1A, 0x59, 0xEC, 0xA6, 0xE6, 0x2B, 0x2D,
    0xAD, 0x6D, 0xED, 0x1D, 0x57, 0x3B, 0xBB, 0x38, 0xDD, 0x3D, 0xBD, 0x7D,
    0x5C, 0x5E, 0x3F, 0x7F, 0x60, 0xF0, 0xDA, 0xF5, 0x21, 0xC1, 0xF0, 0x8D,
    0x91, 0x9B, 0xA3, 0xDF, 0xDF, 0xBA, 0xFD, 0xC3, 0x9D, 0xBB, 0xFF, 0xFC,
    0xF1, 0xDE, 0xFD, 0xB1, 0x07, 0xE3, 0x13, 0x0F, 0x1F, 0x3D, 0x7E, 0xF2,
    0xF4, 0xD9, 0xF3, 0x17, 0x2F, 0x85, 0x93, 0xA2, 0xA9, 0xE9, 0x57, 0xAF,
    0xDF, 0xBC, 0xFD, 0xD7, 0xBF, 0xDF, 0xFD, 0x34, 0xF3, 0x7E, 0xF6, 0x4F,
    0xFF, 0x4F, 0x7C, 0xFD, 0x11, 0x04, 0xA2, 0x23, 0x65, 0xCF, 0x79, 0x2D,
    0xCB, 0xB5, 0x52, 0xAB, 0xD7, 0xD8, 0xDB, 0xB5, 0x04, 0x57, 0xA5, 0x1C,
    0x58, 0xB2, 0x3E, 0x84, 0x81, 0x5A, 0x14, 0x1E, 0xB3, 0x28, 0x22, 0x6A,
    0x49, 0x8C, 0xCF, 0x0A, 0xEA, 0x7E, 0x8D, 0xCB, 0x9A, 0xFB, 0x3A, 0xE5,
    0x71, 0x55, 0xCA, 0x78, 0x49, 0xB4, 0xD6, 0x81, 0xBC, 0xCE, 0x31, 0x45,
    0xD5, 0x3D, 0x4C, 0x23, 0xDC, 0xDE, 0x90, 0xA6, 0x87, 0x61, 0x2B, 0x78,
    0x97, 0xAB, 0xE0, 0x8B, 0xD5, 0x88, 0x0C, 0xD4, 0xE1, 0xCD, 0xD5, 0x87,
    0xB7, 0x94, 0x63, 0x4C, 0xB2, 0xAC, 0x0F, 0x86, 0xD8, 0x9D, 0xDA, 0xE7,
    0x7D, 0x51, 0x25, 0xB0, 0x70, 0x7D, 0x48, 0x91, 0x06, 0xA9, 0x48, 0x33,
    0x69, 0x25, 0x3F, 0x69, 0x25, 0x37, 0x49, 0xE9, 0x4A, 0xB2, 0x32, 0x3D,
    0x55, 0x2F, 0x30, 0xDD, 0xC3, 0x38, 0x3E, 0xC9, 0x00, 0xC9, 0xD1, 0x09,
    0xCF, 0xDF, 0xC0, 0x81, 0x07, 0x1C, 0x18, 0xE3, 0xC0, 0x0F, 0x1C, 0xE8,
    0xED, 0x86, 0xA2, 0x1E, 0x65, 0xBF, 0x7A, 0xFF, 0xC3, 0xD9, 0xF1, 0xA6,
    0xB1, 0x54, 0xF4, 0xB8, 0x5A, 0x9D, 0x84, 0xDA, 0x71, 0xB5, 0x9A, 0x71,
    0xF5, 0xB2, 0x09, 0xAD, 0xAC, 0x87, 0xFA, 0x01, 0x5D, 0x04, 0x7B, 0x7A,
    0xBC, 0x99, 0xF0, 0x08, 0xF5, 0x03, 0x59, 0xC2, 0x23, 0x99, 0x42, 0xEB,
    0x74, 0xE1, 0xD1, 0x0B, 0x22, 0x47, 0x62, 0x6F, 0x90, 0x93, 0xC8, 0x9E,
    0xB2, 0x00, 0x07, 0xB2, 0xC8, 0x21, 0x75, 0xCA, 0x29, 0xFE, 0x15, 0x36,
    0x54, 0xF6, 0x4C, 0xB4, 0xEC, 0xD9, 0xEF, 0x16, 0x25, 0x78, 0x2B, 0xE4,
    0xEF, 0x55, 0x6F, 0x40, 0xED, 0xE5, 0xC8, 0xFF, 0xAA, 0xBC, 0x38, 0xEA,
    0xF4, 0x62, 0x04, 0x91, 0x8F, 0xF5, 0x55, 0xCC, 0x39, 0x80, 0xBA, 0xAC,
    0x25, 0xAD, 0xBC, 0xCE, 0x95, 0xA2, 0x7A, 0x22, 0xF5, 0x6B, 0xAF, 0x70,
    0x54, 0xA4, 0x83, 0x51, 0xAE, 0x1E, 0xA6, 0x49, 0x4A, 0x79, 0x5B, 0xF5,
    0x61, 0xE3, 0x0A, 0xCC, 0x8E, 0x2C, 0xEB, 0x43, 0x44, 0xFB, 0x80, 0x85,
    0xCA, 0x8A, 0xFC, 0x24, 0x25, 0x6E, 0xD2, 0xAA, 0x96, 0xE4, 0xD5, 0xF4,
    0x54, 0x7D, 0x7C, 0x86, 0x44, 0x39, 0x3A, 0x47, 0x27, 0xE2, 0x23, 0xCA,
    0xD9, 0xFF, 0x7F, 0xE5, 0xCF, 0x72, 0xCF, 0xAF, 0xA0, 0xC7, 0xAE, 0x2C,
    0x08, 0xF8, 0xB2, 0x0C, 0xB3, 0x91, 0xBD, 0xF1, 0x40, 0xF7, 0x52, 0xCF,
    0xEA, 0xD5, 0xF8, 0x12, 0xB5, 0x10, 0x06, 0x4A, 0x89, 0x9A, 0xA0, 0x94,
    0x1D, 0xB7, 0x2A, 0x07, 0xBF, 0x9A, 0x61, 0xAD, 0xDB, 0xA0, 0x73, 0x90,
    0xB3, 0xD4, 0xB3, 0xEA, 0x97, 0x48, 0x33, 0x82, 0xAA, 0x19, 0x91, 0xA1,
    0x85, 0x20, 0xBA, 0x17, 0x5C, 0xB6, 0x15, 0x19, 0xDA, 0x34, 0xAF, 0xF0,
    0xA9, 0x50, 0x21, 0x48, 0x94, 0xAD, 0xBF, 0xBD, 0x6C, 0x7D, 0xA8, 0xCA,
    0xE6, 0x48, 0xB6, 0xAD, 0x53, 0xA8, 0x23, 0xF1, 0x80, 0x6F, 0xEE, 0x1A,
    0x7C, 0x91, 0x5A, 0x48, 0x11, 0x8A, 0x54, 0xA4, 0x99, 0xAC, 0x3C, 0x90,
    0xBC, 0x9A, 0x97, 0xAC, 0xD2, 0x92, 0xA2, 0x9E, 0x97, 0xB6, 0x15, 0x9F,
    0xE1, 0x65, 0x92, 0x90, 0x6C, 0x10, 0x9D, 0xA3, 0x2B, 0xA5, 0x7C, 0x87,
    0x03, 0x7D, 0xDD, 0x32, 0x45, 0x3D, 0x6B, 0xFC, 0x1B, 0x4E, 0x61, 0x7E,
    0x47, 0x59, 0xE3, 0xD2, 0xC7, 0x95, 0x27, 0x6D, 0xFF, 0x58, 0x19, 0xDD,
    0x90, 0xAF, 0xD7, 0x90, 0xAB, 0xD7, 0x78, 0xCE, 0xA8, 0xC9, 0xC3, 0xAC,
    0x73, 0xDB, 0x71, 0x9E, 0x92, 0x6F, 0xCD, 0x1A, 0x42, 0x89, 0x3A, 0x91,
    0x89, 0xD2, 0xAF, 0x2D, 0xD0, 0xAF, 0xA3, 0x1B, 0xD4, 0xC5, 0x6D, 0x61,
    0xE1, 0xCC, 0xAF, 0x1A, 0xDB, 0x71, 0x95, 0x7C, 0x6B, 0xD6, 0xCE, 0x45,
    0xA6, 0x85, 0xA5, 0x3B, 0x0A, 0x19, 0x3B, 0x18, 0xC9, 0xE6, 0xA5, 0x27,
    0xF7, 0xD7, 0x9B, 0x39, 0x5F, 0xFD, 0xC2, 0xAF, 0x72, 0x5D, 0x50, 0x89,
    0x3A, 0x91, 0x81, 0x72, 0x09, 0x6D, 0x72, 0x09, 0x6D, 0x74, 0x09, 0x2F,
    0x38, 0x81, 0x20, 0xB8, 0x64, 0x6B, 0xF1, 0x07, 0xC3, 0xD4, 0x20, 0x32,
    0xC4, 0xCA, 0x64, 0x93, 0xEB, 0x64, 0x93, 0x01, 0xF2, 0xF6, 0x0E, 0x8A,
    0x79, 0x61, 0x06, 0x26, 0x24, 0x9B, 0x60, 0x9A, 0x98, 0x6A, 0x14, 0x43,
    0xD3, 0x8D, 0x28, 0xD8, 0xD0, 0x2D, 0x3B, 0xDE, 0x2D, 0xFB, 0xA0, 0x5B,
    0xEE, 0x6E, 0xCF, 0x12, 0x6E, 0xCF, 0x4A, 0x46, 0xAF, 0xD6, 0xA9, 0xC6,
    0x40, 0x4C, 0x76, 0xC2, 0x8E, 0x73, 0xD9, 0xE8, 0x71, 0x8D, 0xFA, 0x71,
    0x8D, 0x3A, 0x09, 0x97, 0x27, 0x50, 0x97, 0x26, 0xB4, 0xA9, 0x8F, 0x0C,
    0xE7, 0x94, 0xCD, 0x17, 0x28, 0x5B, 0x7F, 0x50, 0x76, 0x22, 0xF6, 0x06,
    0x2F, 0x54, 0x76, 0x58, 0xA0, 0xEC, 0x3E, 0xDC, 0xE9, 0x3E, 0xDC, 0xEE,
    0x3E, 0x5C, 0xEA, 0x31, 0x72, 0xC6, 0xE7, 0x96, 0x6D, 0xD0, 0xA8, 0x3A,
    0xB1, 0x1E, 0x45, 0x2A, 0xD3, 0x24, 0x31, 0x35, 0xB1, 0x82, 0x2E, 0xAC,
    0xA0, 0x1D, 0x2B, 0x28, 0xC5, 0xDD, 0x38, 0xEB, 0xFB, 0xBD, 0x6D, 0xF0,
    0xA8, 0xC6, 0xAF, 0x91, 0x37, 0xAF, 0xDB, 0x9B, 0xD7, 0xE9, 0xDD, 0x5F,
    0xEE, 0xCB, 0x8F, 0xF3, 0x1F, 0xB2, 0x23, 0x0D, 0x69, 0x86, 0xD6, 0x6A,
    0x86, 0x96, 0x6A, 0x86, 0x32, 0x35, 0xC3, 0xAB, 0x07, 0xC3, 0xAB, 0x79,
    0xE1, 0x35, 0x0D, 0x11, 0xB5, 0xE4, 0x28, 0xB6, 0x47, 0x6C, 0x3B, 0x1A,
    0x29, 0xD7, 0x89, 0x60, 0x6E, 0x08, 0x63, 0x68, 0xD1, 0x43, 0x6E, 0xD3,
    0x43, 0x46, 0xE9, 0x21, 0xDC, 0xBC, 0xD0, 0x8A, 0x7C, 0x04, 0x61, 0x90,
    0xF7, 0x50, 0xA8, 0xC6, 0xB1, 0xB9, 0xE8, 0xC8, 0x02, 0x6D, 0xAE, 0xCE,
    0x63, 0xAE, 0xCE, 0x23, 0xAE, 0xCE, 0x3D, 0xEE, 0x37, 0x7C, 0xAE, 0x61,
    0x29, 0xCF, 0x92, 0xD0, 0x44, 0xB2, 0xA1, 0x25, 0xEE, 0x8C, 0xCB, 0xD6,
    0x7B, 0xB8, 0xB1, 0xF1, 0x03, 0xF5, 0x0F, 0x75, 0x2A, 0x1F, 0x6E, 0xBA,
    0xF8, 0x78, 0x6B, 0x20, 0x27, 0xD8, 0x21, 0x2F, 0xC1, 0x5C, 0x68, 0x43,
    0x95, 0x22, 0x4B, 0x78, 0x34, 0x7D, 0xF2, 0x58, 0xE2, 0x94, 0x33, 0x49,
    0xAC, 0x2C, 0xD6, 0x94, 0x86, 0x2C, 0x72, 0x4C, 0x9D, 0x72, 0x16, 0x2B,
    0xD3, 0x84, 0x7C, 0x9A, 0x90, 0x47, 0x13, 0x5E, 0xA5, 0x09, 0xCB, 0xE8,
    0x53, 0xD1, 0x45, 0xEF, 0xF6, 0x90, 0xDB, 0xB6, 0x9D, 0xAD, 0xD8, 0x14,
    0x59, 0xAC, 0x4D, 0x7B, 0xC9, 0xA7, 0xBD, 0xEC, 0xA7, 0xBD, 0xBC, 0x9A,
    0x2B, 0x89, 0x18, 0x52, 0x11, 0xFD, 0xD9, 0x20, 0xFD, 0x19, 0x9F, 0xFE,
    0xBC, 0x8B, 0xFE, 0xBC, 0x3C, 0x5F, 0x18, 0xC3, 0x7C, 0x6B, 0x49, 0x69,
    0x35, 0x3E, 0x5B, 0x81, 0x8E, 0x2C, 0xD6, 0x66, 0xDE, 0xBB, 0xC5, 0xBC,
    0x37, 0xCA, 0xBC, 0xCF, 0x2F, 0x1E, 0xAB, 0x2D, 0x99, 0x88, 0x2B, 0x7F,
    0x71, 0x80, 0xCA, 0x32, 0x8D, 0x2B, 0xD3, 0xFB, 0x8E, 0xB9, 0x91, 0xD5,
    0x22, 0x64, 0xB5, 0xBC, 0x60, 0xB5, 0xFC, 0xC8, 0x6A, 0xED, 0x64, 0xB7,
    0x67, 0x35, 0xF7, 0x3A, 0x17, 0x97, 0x5A, 0xA6, 0x14, 0x6E, 0x89, 0x29,
    0xD4, 0x15, 0x10, 0x44, 0x02, 0xC2, 0xA4, 0x80, 0xF0, 0x44, 0x10, 0x74,
    0x53, 0x40, 0xAC, 0x1B, 0x46, 0x90, 0xAE, 0x04, 0xFB, 0x22, 0xF2, 0xEE,
    0x84, 0x1C, 0xC3, 0x27, 0x26, 0x5D, 0x12, 0x3A, 0x9F, 0x6C, 0xEF, 0x78,
    0x6A, 0xCA, 0x7A, 0xBA, 0xB3, 0xF0, 0x99, 0x25, 0xA1, 0x2F, 0xDC, 0xA9,
    0x20, 0x71, 0xD7, 0xE4, 0x31, 0xEA, 0x3C, 0xC7, 0xB3, 0x26, 0x8F, 0xA7,
    0x8B, 0x1C, 0x12, 0xA7, 0x4F, 0x90, 0xFA, 0x88, 0xCE, 0x22, 0x47, 0x8A,
    0x34, 0x53, 0x8E, 0xE4, 0x29, 0xA7, 0xD4, 0x29, 0x97, 0xF8, 0x57, 0x1E,
    0xA1, 0xAC, 0x81, 0x3C, 0xD6, 0x00, 0x5D, 0x42, 0x0E, 0x7B, 0x80, 0xD2,
    0x3C, 0x18, 0xD5, 0x21, 0xB0, 0xCB, 0xAF, 0xB7, 0x88, 0x2F, 0x33, 0xF8,
    0x58, 0xC4, 0xCF, 0x93, 0x40, 0x67, 0xF3, 0x73, 0xD8, 0x7C, 0x4A, 0xF3,
    0x20, 0xD2, 0x31, 0x64, 0x97, 0x5F, 0xB7, 0x2B, 0xBE, 0xCC, 0xA0, 0xB9,
    0x2F, 0x4F, 0x02, 0xBD, 0x99, 0x4B, 0xBB, 0xC2, 0x4D, 0x6F, 0xE5, 0x45,
    0x77, 0x0E, 0xDA, 0x17, 0xD6, 0xEE, 0x4E, 0x28, 0x35, 0xE8, 0x6C, 0x2B,
    0xE8, 0x6C, 0xCB, 0x97, 0x40, 0xEF, 0x6A, 0xCF, 0xE4, 0x74, 0x9C, 0xE1,
    0x76, 0xBB, 0x94, 0x54, 0x5A, 0x25, 0x16, 0x1B, 0x8D, 0x30, 0x8B, 0x7F,
    0xA1, 0x98, 0x31, 0x52, 0x9C, 0x7B, 0xB3, 0x34, 0xF1, 0x56, 0xB5, 0x77,
    0x43, 0x1E, 0x26, 0x3D, 0xCF, 0x58, 0x84, 0x9F, 0x95, 0x30, 0x23, 0xC2,
    0x4F, 0x8B, 0x08, 0x77, 0x45, 0x21, 0x6C, 0x11, 0x82, 0x0C, 0xC5, 0xB9,
    0x97, 0xA6, 0x59, 0x4D, 0xBB, 0x65, 0x4B, 0x41, 0x7D, 0xE5, 0x9E, 0xF9,
    0x0A, 0x9B, 0xFC, 0xC6, 0x27, 0x8C, 0x1F, 0xE1, 0x3A, 0xE5, 0x42, 0x91,
    0x66, 0xDA, 0x85, 0x3C, 0xED, 0x9A, 0x3A, 0xED, 0x96, 0xF0, 0xDA, 0x33,
    0x8C, 0xD3, 0x96, 0x29, 0x21, 0x43, 0x02, 0xA5, 0xBB, 0x2D, 0xA9, 0xB7,
    0x2D, 0x6A, 0xA0, 0xE3, 0x04, 0xA3, 0x66, 0x0F, 0xA7, 0x35, 0x53, 0xCC,
    0x47, 0xA2, 0xD6, 0x0C, 0x09, 0xF3, 0x11, 0xB3, 0x66, 0x4F, 0x77, 0x4B,
    0xA6, 0x84, 0x0C, 0x09, 0x94, 0x9E, 0x96, 0xA4, 0xBE, 0x56, 0x64, 0xA0,
    0xDD, 0x8D, 0x59, 0x6D, 0xD9, 0xDB, 0x94, 0xF9, 0x81, 0x8C, 0xBE, 0x26,
    0x4A, 0x5F, 0x53, 0x32, 0xAF, 0x39, 0xFA, 0x7A, 0x8B, 0x7B, 0x49, 0xA5,
    0xD5, 0xB5, 0xEA, 0x4C, 0x29, 0xD2, 0xAF, 0x57, 0xA7, 0x0C, 0xD5, 0x9C,
    0xBE, 0x59, 0xE7, 0x59, 0x51, 0xB6, 0xFF, 0xFE, 0xC5, 0x2C, 0x29, 0x32,
    0xEF, 0x5F, 0x24, 0x8F, 0xD1, 0xCE, 0x4F, 0xE4, 0x05, 0x34, 0xD1, 0x6C,
    0x66, 0x48, 0x65, 0x52, 0x94, 0xCE, 0x84, 0x16, 0xCF, 0x84, 0xE5, 0xCD,
    0x20, 0xC8, 0x8D, 0x73, 0xB8, 0xD7, 0x5E, 0xE9, 0x52, 0x50, 0xC4, 0x78,
    0xA7, 0xBD, 0xF1, 0xB9, 0xF0, 0xD6, 0x3F, 0xBC, 0x8F, 0x4D, 0x91, 0x82,
    0xCC, 0x65, 0xA7, 0x71, 0xD9, 0x89, 0xFD, 0x4D, 0xC8, 0x50, 0x33, 0xF6,
    0x7F, 0x8B, 0xB8, 0x6C, 0x8A, 0x14, 0xE2, 0x88, 0xC7, 0x4E, 0xEC, 0x67,
    0x23, 0x43, 0x4D, 0x58, 0x2E, 0x8B, 0x22, 0x05, 0x99, 0xC7, 0x4A, 0xE3,
    0xB1, 0x12, 0xF9, 0x2C, 0x44, 0xD0, 0xE4, 0xC1, 0xAF, 0xA3, 0x48, 0x41,
    0xE6, 0xD7, 0xA7, 0x0D, 0xD4, 0x27, 0x5E, 0xAB, 0x8F, 0xBE, 0xD1, 0x88,
    0x1B, 0x2E, 0xA7, 0x2C, 0x24, 0xED, 0x46, 0x79, 0xD2, 0x48, 0xC5, 0xE9,
    0xDB, 0x55, 0x3E, 0xE3, 0x59, 0xE9, 0x52, 0x50, 0xC6, 0xB3, 0xC8, 0xE3,
    0xD4, 0x94, 0x89, 0xEC, 0xB8, 0xC7, 0x34, 0xFC, 0xFB, 0x88, 0x0C, 0x29,
    0xD2, 0x25, 0x90, 0xDF, 0x47, 0x26, 0xCD, 0x22, 0xC8, 0x9F, 0x5B, 0xD1,
    0x4F, 0x7C, 0x2B, 0xFA, 0xA9, 0xFB, 0xFB, 0x59, 0x98, 0x4B, 0xCE, 0xB3,
    0x72, 0x72, 0x73, 0x87, 0xDA, 0xDF, 0xFE, 0x44, 0x08, 0x70, 0xC3, 0xBB,
    0x49, 0x9F, 0x5D, 0xFF, 0xE8, 0xFA, 0x67, 0x0B, 0x10, 0x3B, 0xD9
  };
};

bool
sk_png_remove_chunk (const char* szName, void* data, size_t& size)
{
  if (szName == nullptr || data == nullptr || size < 12 || strlen (szName) < 4)
  {
    return false;
  }

  size_t   erase_pos = 0;
  uint8_t* erase_ptr = nullptr;

  // Effectively a string search, but ignoring nul-bytes in both
  //   the character array being searched and the pattern...
  std::string_view data_view ((const char *)data, size);
  if (erase_pos  = data_view.find (szName, 0, 4);
      erase_pos == data_view.npos)
  {
    return false;
  }

  erase_pos -= 4; // Rollback to the chunk's length field
  erase_ptr =
    ((uint8_t *)data + erase_pos);

  uint32_t chunk_size = *(uint32_t *)erase_ptr;

// Length is Big Endian, Intel/AMD CPUs are Little Endian
#if (defined _M_IX86) || (defined _M_X64)
  chunk_size = _byteswap_ulong (chunk_size);
#endif

  size_t size_to_erase = (size_t)12 + chunk_size;

  memmove ( erase_ptr,
            erase_ptr             + size_to_erase,
                 size - erase_pos - size_to_erase );

  size -= size_to_erase;

  return true;
}

SK_PNG_HDR_cLLi_Payload
SK_HDR_CalculateContentLightInfo (const DirectX::Image& img)
{
  SK_PNG_HDR_cLLi_Payload clli;

  static const XMMATRIX c_from709to2020 =
  {
    { 0.627225305694944f,  0.0690418812810714f, 0.0163911702607078f, 0.0f },
    { 0.329476882715808f,  0.919605681354755f,  0.0880887513437058f, 0.0f },
    { 0.0432978115892484f, 0.0113524373641739f, 0.895520078395586f,  0.0f },
    { 0.0f,                0.0f,                0.0f,                1.0f }
  };

  static const XMMATRIX c_from709toXYZ =
  {
    { 0.4123907983303070068359375f,  0.2126390039920806884765625f,   0.0193308182060718536376953125f, 0.0f },
    { 0.3575843274593353271484375f,  0.715168654918670654296875f,    0.119194783270359039306640625f,  0.0f },
    { 0.18048079311847686767578125f, 0.072192318737506866455078125f, 0.950532138347625732421875f,     0.0f },
    { 0.0f,                          0.0f,                           0.0f,                            1.0f }
  };

  static const XMMATRIX c_from2020toXYZ =
  {
    { 0.636958062f, 0.2627002000f, 0.0000000000f, 0.0f },
    { 0.144616901f, 0.6779980650f, 0.0280726924f, 0.0f },
    { 0.168880969f, 0.0593017153f, 1.0609850800f, 0.0f },
    { 0.0f,         0.0f,          0.0f,          1.0f }
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
  
  auto PQToLinear = [](XMVECTOR N)
  {
    XMVECTOR ret;
  
    ret =
      XMVectorPow (N, XMVectorDivide (g_XMOne, PQ.M));
  
    XMVECTOR nd;
  
    nd =
      XMVectorDivide (
        XMVectorMax (XMVectorSubtract (ret, PQ.C1), g_XMZero),
                     XMVectorSubtract (     PQ.C2,
              XMVectorMultiply (PQ.C3, ret)));
  
    ret =
      XMVectorMultiply (
        XMVectorPow (nd, XMVectorDivide (g_XMOne, PQ.N)), PQ.MaxPQ
      );
  
    return ret;
  };

  float N         = 0.0f;
  float fLumAccum = 0.0f;
  XMVECTOR vMaxLum = g_XMZero;

  EvaluateImage ( img,
    [&](const XMVECTOR* pixels, size_t width, size_t y)
    {
      UNREFERENCED_PARAMETER(y);

      float fScanlineLum = 0.0f;

      switch (img.format)
      {
        case DXGI_FORMAT_R10G10B10A2_UNORM:
        {
          for (size_t j = 0; j < width; ++j)
          {
            XMVECTOR v =
              *pixels++;

            v =
              XMVector3Transform (
                PQToLinear (XMVectorSaturate (v)), c_from2020toXYZ
              );

            vMaxLum =
              XMVectorMax (vMaxLum, v);

            fScanlineLum += XMVectorGetY (v);
          }
        } break;

        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
        {
          for (size_t j = 0; j < width; ++j)
          {
            XMVECTOR v =
              *pixels++;

            v =
              XMVector3Transform (v, c_from709toXYZ);

            vMaxLum =
              XMVectorMax (vMaxLum, v);

            fScanlineLum += XMVectorGetY (v);
          }
        } break;

        default:
          SK_ReleaseAssert (!"Unsupported CLLI input format");
          break;
      }

      fLumAccum +=
        (fScanlineLum / static_cast <float> (width));
      ++N;
    }
  );

  if (N > 0.0)
  {
    clli.max_cll  =
      static_cast <uint32_t> (round ((80.0f * XMVectorGetY (vMaxLum)) / 0.0001f));
    clli.max_fall = 
      static_cast <uint32_t> (round ((80.0f * (fLumAccum / N))        / 0.0001f));
  }

  return clli;
}

bool
SK_PNG_MakeHDR ( const wchar_t*        wszFilePath,
                 const DirectX::Image& encoded_img,
                 const DirectX::Image& raw_img )
{
  std::ignore = encoded_img;

  static const BYTE _test [] = { 0x49, 0x45, 0x4E, 0x44 };

  if (png_crc32 ((const BYTE *)_test, 0, 4, 0) == 0xae426082)
  {
    FILE*
        fPNG = _wfopen (wszFilePath, L"r+b");
    if (fPNG != nullptr)
    {
      fseek  (fPNG, 0, SEEK_END);
      size_t size = ftell (fPNG);
      rewind (fPNG);

      auto data =
        std::make_unique <uint8_t []> (size);

      if (! data)
      {
        fclose (fPNG);
        return false;
      }

      fread (data.get (), size, 1, fPNG);
      rewind (                     fPNG);

      sk_png_remove_chunk ("sRGB", data.get (), size);
      sk_png_remove_chunk ("gAMA", data.get (), size);

      fwrite (data.get (), size, 1, fPNG);

      // Truncate the file
      _chsize (_fileno (fPNG), static_cast <long> (size));

      size_t         insert_pos = 0;
      const uint8_t* insert_ptr = nullptr;

      // Effectively a string search, but ignoring nul-bytes in both
      //   the character array being searched and the pattern...
      std::string_view  data_view ((const char *)data.get (), size);
      if (insert_pos  = data_view.find ("IDAT", 0, 4);
          insert_pos == data_view.npos)
      {
        fclose (fPNG);
        return false;
      }

      insert_pos -= 4; // Rollback to the chunk's length field
      insert_ptr =
        (data.get () + insert_pos);

      fseek (fPNG, static_cast <long> (insert_pos), SEEK_SET);

      struct SK_PNG_Chunk {
        uint32_t      len;
        unsigned char name [4];
        void*         data;
        uint32_t      crc;
        uint32_t      _native_len;

        void write (FILE* fStream)
        {
          // Length is Big Endian, Intel/AMD CPUs are Little Endian
          if (_native_len == 0)
          {
            _native_len = len;
#if (defined _M_IX86) || (defined _M_X64)
            len         = _byteswap_ulong (_native_len);
#endif
          }

          crc =
            png_crc32 (data, 0, _native_len, png_crc32 (name, 0, 4, 0x0));

          fwrite (&len, 8,           1, fStream);
          fwrite (data, _native_len, 1, fStream);
          fwrite (&crc, 4,           1, fStream);
        };
      };

      uint8_t cicp_data [] = {
        9,  // BT.2020 Color Primaries
        16, // ST.2084 EOTF (PQ)
        0,  // Identity Coefficients
        1,  // Full Range
      };

      // Embedded ICC Profile so that Discord will render in HDR
      SK_PNG_HDR_iCCP_Payload iccp_data;

      SK_PNG_HDR_cHRM_Payload chrm_data; // Rec 2020 chromaticity
      SK_PNG_HDR_sBIT_Payload sbit_data; // Bits in original source (max=12)
      SK_PNG_HDR_mDCv_Payload mdcv_data; // Display capabilities
      SK_PNG_HDR_cLLi_Payload clli_data; // Content light info

      clli_data =
        SK_HDR_CalculateContentLightInfo (raw_img);

      sbit_data = {
        static_cast <unsigned char> (DirectX::BitsPerColor (raw_img.format)),
        static_cast <unsigned char> (DirectX::BitsPerColor (raw_img.format)),
        static_cast <unsigned char> (DirectX::BitsPerColor (raw_img.format)), 1
      };

      // If using compression optimization, max bits = 12
      sbit_data.red_bits   = std::min (sbit_data.red_bits,   12ui8);
      sbit_data.green_bits = std::min (sbit_data.green_bits, 12ui8);
      sbit_data.blue_bits  = std::min (sbit_data.blue_bits,  12ui8);

      auto& rb =
        SK_GetCurrentRenderBackend ();

      auto& active_display =
        rb.displays [rb.active_display];

      mdcv_data.luminance.minimum =
        static_cast <uint32_t> (round (active_display.gamut.minY / 0.0001f));
      mdcv_data.luminance.maximum =
        static_cast <uint32_t> (round (active_display.gamut.maxY / 0.0001f));

      mdcv_data.primaries.red_x =
        static_cast <uint32_t> (round (active_display.gamut.xr / 0.00002));
      mdcv_data.primaries.red_y =
        static_cast <uint32_t> (round (active_display.gamut.yr / 0.00002));

      mdcv_data.primaries.green_x =
        static_cast <uint32_t> (round (active_display.gamut.xg / 0.00002));
      mdcv_data.primaries.green_y =
        static_cast <uint32_t> (round (active_display.gamut.yg / 0.00002));

      mdcv_data.primaries.blue_x =
        static_cast <uint32_t> (round (active_display.gamut.xb / 0.00002));
      mdcv_data.primaries.blue_y =
        static_cast <uint32_t> (round (active_display.gamut.yb / 0.00002));

      mdcv_data.white_point.x =
        static_cast <uint32_t> (round (active_display.gamut.Xw / 0.00002));
      mdcv_data.white_point.y =
        static_cast <uint32_t> (round (active_display.gamut.Yw / 0.00002));

      SK_PNG_Chunk iccp_chunk = { sizeof (SK_PNG_HDR_iCCP_Payload), { 'i','C','C','P' }, &iccp_data };
      SK_PNG_Chunk cicp_chunk = { sizeof (cicp_data),               { 'c','I','C','P' }, &cicp_data };
      SK_PNG_Chunk clli_chunk = { sizeof (clli_data),               { 'c','L','L','i' }, &clli_data };
      SK_PNG_Chunk sbit_chunk = { sizeof (sbit_data),               { 's','B','I','T' }, &sbit_data };
      SK_PNG_Chunk chrm_chunk = { sizeof (chrm_data),               { 'c','H','R','M' }, &chrm_data };
      SK_PNG_Chunk mdcv_chunk = { sizeof (mdcv_data),               { 'm','D','C','v' }, &mdcv_data };

      iccp_chunk.write (fPNG);
      cicp_chunk.write (fPNG);
      clli_chunk.write (fPNG);
      sbit_chunk.write (fPNG);
      chrm_chunk.write (fPNG);
      mdcv_chunk.write (fPNG);

      // Write the remainder of the original file
      fwrite (insert_ptr, size - insert_pos, 1, fPNG);

      auto final_size =
        ftell (fPNG);

      auto full_png =
        std::make_unique <unsigned char []> (final_size);

      rewind (fPNG);
      fread  (full_png.get (), final_size, 1, fPNG);
      fclose (fPNG);

      SK_LOGi1 (L"Applied HDR10 PNG chunks to %ws.", wszFilePath);
      SK_LOGi1 (L" >> MaxCLL: %.6f nits, MaxFALL: %.6f nits",
                static_cast <double> (clli_data.max_cll)  * 0.0001,
                static_cast <double> (clli_data.max_fall) * 0.0001);
      SK_LOGi1 (L" >> Mastering Display Min/Max Luminance: %.6f/%.6f nits",
                static_cast <double> (mdcv_data.luminance.minimum) * 0.0001,
                static_cast <double> (mdcv_data.luminance.maximum) * 0.0001);

      return true;
    }
  }

  return false;
}



SK_ScreenshotManager::SnippingState
SK_ScreenshotManager::getSnipState (void) const
{
  return snip_state;
}

void
SK_ScreenshotManager::setSnipState (SK_ScreenshotManager::SnippingState state)
{
  snip_state = state;
}

DirectX::Rect
SK_ScreenshotManager::getSnipRect (void) const
{
  return snip_rect;
}

void
SK_ScreenshotManager::setSnipRect (const DirectX::Rect& rect)
{
  snip_rect = rect;
}