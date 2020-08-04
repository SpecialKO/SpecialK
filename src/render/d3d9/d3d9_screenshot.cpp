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
#include <SpecialK/render/d3d9/d3d9_screenshot.h>
#include <SpecialK/steam_api.h>

SK_D3D9_Screenshot::SK_D3D9_Screenshot (const SK_ComPtr <IDirect3DDevice9>& pDevice) : pDev (pDevice)
{
  if (pDev != nullptr)
  {
    if (SK_GetCurrentRenderBackend ().swapchain != nullptr)
        SK_GetCurrentRenderBackend ().swapchain->QueryInterface <IDirect3DSwapChain9> (&pSwapChain);

    if (pSwapChain != nullptr)
    {
      ulCommandIssuedOnFrame = SK_GetFramesDrawn ();

      if ( SUCCEEDED ( pSwapChain->GetBackBuffer ( 0, D3DBACKBUFFER_TYPE_MONO,
                                                     &pBackbufferSurface.p
                                                 )
                     )
         )
      {
        D3DSURFACE_DESC               backbuffer_desc = { };
        pBackbufferSurface->GetDesc (&backbuffer_desc);

        framebuffer.Width        = backbuffer_desc.Width;
        framebuffer.Height       = backbuffer_desc.Height;
        framebuffer.NativeFormat = backbuffer_desc.Format;

        if (pBackbufferSurface == nullptr)
          return;

        D3DSURFACE_DESC                            desc = { };
        HRESULT hr = pBackbufferSurface->GetDesc (&desc);

        if (FAILED (hr)) return;

        //static D3DXLoadSurfaceFromSurface_pfn
        //  D3DXLoadSurfaceFromSurface =
        //    (D3DXLoadSurfaceFromSurface_pfn)
        //   SK_GetProcAddress ( d3dx9_43_dll, "D3DXLoadSurfaceFromSurface" );

        if (SUCCEEDED ( pDev->CreateRenderTarget ( desc.Width, desc.Height,
                                                     desc.Format, desc.MultiSampleType,
                                                                  desc.MultiSampleQuality,
                                                       TRUE,
                                                         &pSurfScreenshot.p, nullptr
                                               )
                      )
           )
        {
          if ( SUCCEEDED ( pDev->StretchRect ( pBackbufferSurface, nullptr,
                                               pSurfScreenshot,    nullptr,
                                                 D3DTEXF_NONE
                                           )
                         )
             )
          {
            outstanding_screenshots.emplace (pSurfScreenshot);

            return;
          }
        }
      }
    }
  }

  dispose ();
}

void
SK_D3D9_Screenshot::dispose (void)
{
  if (outstanding_screenshots.contains (pBackbufferSurface))
      outstanding_screenshots.erase    (pBackbufferSurface);

  pBackbufferSurface     = nullptr;
  pSwapChain             = nullptr;
  pDev                   = nullptr;

  if (framebuffer.PixelBuffer.m_pData != nullptr)
      framebuffer.PixelBuffer.Free ();
};

bool
SK_D3D9_Screenshot::getData ( UINT     *pWidth,
                              UINT     *pHeight,
                              uint8_t **ppData,
                              UINT     *pPitch,
                              bool      Wait )
{
  auto ReadBack = [&](void) -> bool
  {
    if (ulCommandIssuedOnFrame < SK_GetFramesDrawn () - 2)
    {
      const size_t BytesPerPel =
        SK_D3D9_BytesPerPixel (framebuffer.NativeFormat);

      D3DLOCKED_RECT finished_copy = { };
      UINT           PackedDstPitch;

      if ( SUCCEEDED ( pSurfScreenshot->LockRect ( &finished_copy,
                                                     nullptr, 0x0 )
                     )
         )
      {
        PackedDstPitch = finished_copy.Pitch;

        if ( framebuffer.PixelBuffer.AllocateBytes ( framebuffer.Height *
                                                       PackedDstPitch
                                                   )
           )
        {
          *pWidth  = framebuffer.Width;
          *pHeight = framebuffer.Height;

          uint8_t* pSrc =  (uint8_t *)finished_copy.pBits;
          uint8_t* pDst = framebuffer.PixelBuffer.m_pData;

          if (pSrc != nullptr && pDst != nullptr)
          {
            for ( UINT i = 0; i < framebuffer.Height; ++i )
            {
              memcpy ( pDst, pSrc, finished_copy.Pitch );

              // Eliminate pre-multiplied alpha problems (the stupid way)
              for ( UINT j = 3 ; j < PackedDstPitch ; j += 4 )
              {
                pDst [j] = 255UL;
              }

              pSrc += finished_copy.Pitch;
              pDst +=         PackedDstPitch;
            }
          }

          *pPitch = PackedDstPitch;
        }

        SK_LOG0 ( ( L"Screenshot Readback Complete after %li frames",
                      SK_GetFramesDrawn () - ulCommandIssuedOnFrame ),
                    L"D3D11SShot" );


        HRESULT hr =
          pSurfScreenshot->UnlockRect ();

        if (hr != S_OK) assert (false);

        *ppData = framebuffer.PixelBuffer.m_pData;

        return true;
      }
    }

    return false;
  };


  bool ready_to_read = false;


  if (! Wait)
  {
    if (isReady ())
    {
      ready_to_read = true;
    }
  }

  else if (isValid ())
  {
    ready_to_read = true;
  }


  return ( ready_to_read ? ReadBack () :
                           false         );
}





static SK_LazyGlobal <concurrency::concurrent_queue <SK_D3D9_Screenshot*>> screenshot_queue;


bool
SK_D3D9_CaptureSteamScreenshot  ( SK_ScreenshotStage when )
{
  if ( ((int)SK_GetCurrentRenderBackend ().api & (int)SK_RenderAPI::D3D9) != 0 )
  {
    int stage = 0;

    static const std::map <SK_ScreenshotStage, int> __stage_map = {
      { SK_ScreenshotStage::BeforeGameHUD, 0 },
      { SK_ScreenshotStage::BeforeOSD,     1 },
      { SK_ScreenshotStage::EndOfFrame,    2 }
    };

    const auto it = __stage_map.find (when);

    if (it != __stage_map.cend ())
    {
      stage = it->second;

      InterlockedIncrement (&enqueued_screenshots.stages [stage]);

      return true;
    }
  }

  return false;
}

#include <wincodec.h>
#include <../depends/include/DirectXTex/DirectXTex.h>

void
SK_D3D9_ProcessScreenshotQueue (SK_ScreenshotStage stage_)
{
  int stage =
    gsl::narrow_cast <int> (stage_);

  const int __MaxStage = 2;

  assert (stage >= 0 && stage <= __MaxStage);

  if (ReadAcquire (&enqueued_screenshots.stages [stage]) > 0)
  {
    if (InterlockedDecrement (&enqueued_screenshots.stages [stage]) >= 0)
    {
      SK_ComQIPtr <IDirect3DDevice9> pDev (SK_GetCurrentRenderBackend ().device);

      if (pDev != nullptr)
      {
        screenshot_queue->push (
          new SK_D3D9_Screenshot (pDev)
        );
      }
    }

    else InterlockedIncrement (&enqueued_screenshots.stages [stage]);
  }


  if (! screenshot_queue->empty ())
  {
    static volatile
      HANDLE hSignalScreenshot = INVALID_HANDLE_VALUE;

    if ( INVALID_HANDLE_VALUE ==
           InterlockedCompareExchangePointer (&hSignalScreenshot, nullptr, INVALID_HANDLE_VALUE) )
    {
      InterlockedExchangePointer ( (void **)&hSignalScreenshot,
                                     SK_CreateEvent ( nullptr,
                                                        FALSE, TRUE,
                                                          nullptr ) );

      SK_Thread_Create ([](LPVOID) -> DWORD
      {
        SetCurrentThreadDescription (          L"[SK] D3D9 Screenshot Capture Thread" );
        SetThreadPriority           ( SK_GetCurrentThread (), THREAD_PRIORITY_BELOW_NORMAL |
                                                              THREAD_MODE_BACKGROUND_BEGIN );

        HANDLE hSignal =
          ReadPointerAcquire (&hSignalScreenshot);

        // Any incomplete captures are pushed onto this queue, and then the pending
        //   queue (once drained) is re-built.
        //
        //  This is faster than iterating a synchronized list in highly multi-threaded engines.
        static concurrency::concurrent_queue <SK_D3D9_Screenshot*> rejected_screenshots;


        while (ReadAcquire (&__SK_DLL_Ending) == 0)
        {
          MsgWaitForMultipleObjectsEx ( 1, &hSignal, INFINITE, 0x0, 0x0 );

          while (! screenshot_queue->empty ())
          {
            SK_D3D9_Screenshot* pop_off = nullptr;

            if (screenshot_queue->try_pop (pop_off) && pop_off != nullptr)
            {
              UINT     Width, Height, Pitch;
              uint8_t* pData;

              if (pop_off->getData (&Width, &Height, &pData, &Pitch))
              {
                //HRESULT hr = E_UNEXPECTED;


                wchar_t      wszAbsolutePathToScreenshot [ MAX_PATH + 2 ] = { };
                wcsncpy_s   (wszAbsolutePathToScreenshot,  MAX_PATH, SK_GetConfigPath (), _TRUNCATE);
                //PathAppendW (wszAbsolutePathToScreenshot, L"SK_SteamScreenshotImport.png");
                PathAppendW (wszAbsolutePathToScreenshot, L"SK_SteamScreenshotImport.jpg");

                //if ( SUCCEEDED (
                //  SaveToWICFile ( raw_img, WIC_FLAGS_NONE,
                //                     GetWICCodec (WIC_CODEC_JPEG),
                //                    //GetWICCodec (WIC_CODEC_PNG),
                //                      wszAbsolutePathToScreenshot )
                //               )
                //   )
                //{
                  wchar_t      wszAbsolutePathToThumbnail [ MAX_PATH + 2 ] = { };
                  wcsncpy_s   (wszAbsolutePathToThumbnail,  MAX_PATH, SK_GetConfigPath (), _TRUNCATE);
                  PathAppendW (wszAbsolutePathToThumbnail, L"SK_SteamThumbnailImport.jpg");

                  float aspect = (float)Height /
                                 (float)Width;

#if 0
                static SK_ComPtr <IWICImagingFactory> pFactory = nullptr;

                SK_ComPtr <IWICBitmapEncoder>     pEncoder = nullptr;
                SK_ComPtr <IWICBitmapFrameEncode> pFrame   = nullptr;
                SK_ComPtr <IWICStream>            pStream  = nullptr;

                if (! pFactory)
                {
                  if ( FAILED (
                        CoCreateInstance ( CLSID_WICImagingFactory, nullptr,
                                           CLSCTX_INPROC_SERVER,
                                           IID_PPV_ARGS (&pFactory)
                                         )
                              )
                     )
                  {
                    continue;
                  }
                }

                GUID guidPixelFormat = GUID_WICPixelFormat24bppRGB;

                if ( SUCCEEDED (pFactory->CreateStream          (&pStream))                                     &&
                     SUCCEEDED (pStream->InitializeFromFilename (wszAbsolutePathToScreenshot, GENERIC_WRITE))   &&
                     SUCCEEDED (pFactory->CreateEncoder         (GUID_ContainerFormatJpeg, nullptr, &pEncoder)) &&
                     SUCCEEDED (pEncoder->Initialize            (pStream, WICBitmapEncoderNoCache))             &&
                     SUCCEEDED (pEncoder->CreateNewFrame        (&pFrame, nullptr))                             &&
                     SUCCEEDED (pFrame->Initialize              (nullptr))                                      &&
                     SUCCEEDED (pFrame->SetSize                 (Width, Height))                                &&
                     SUCCEEDED (pFrame->SetPixelFormat          (&guidPixelFormat))                             &&
                     SUCCEEDED (pFrame->WritePixels             (Height, Pitch, Pitch * Height, pData))         &&
                     SUCCEEDED (pFrame->Commit                  ())                                             &&
                     SUCCEEDED (pEncoder->Commit                ()) )
                {
#else
                {
                  using namespace DirectX;

                  DXGI_FORMAT dxgi_format =
                    SK_D3D9_FormatToDXGI (pop_off->getInternalFormat ());

                  Image raw_img = { };

                  ComputePitch (
                    dxgi_format,
                      Width, Height,
                        raw_img.rowPitch, raw_img.slicePitch
                  );

                  raw_img.format = dxgi_format;
                  raw_img.width  = Width;
                  raw_img.height = Height;
                  raw_img.pixels = pData;


                    ScratchImage thumbnailImage;

                    Resize ( raw_img, 200, (size_t)(200 * aspect), TEX_FILTER_TRIANGLE, thumbnailImage );

                    SaveToWICFile ( *thumbnailImage.GetImages (), WIC_FLAGS_NONE,
                                      GetWICCodec (WIC_CODEC_JPEG),
                                        wszAbsolutePathToThumbnail );
#endif

                  ScreenshotHandle screenshot =
                    SK_SteamAPI_AddScreenshotToLibraryEx ( SK_WideCharToUTF8 (wszAbsolutePathToScreenshot).c_str  (),
                                                             nullptr,//SK_WideCharToUTF8 (wszAbsolutePathToThumbnail).c_str (),
                                                               Width, Height, true );

                  SK_LOG0 ( ( L"Finished Steam Screenshot Import for Handle: '%x'",
                              screenshot ), L"SteamSShot" );

                  // Remove the temporary files...
                  DeleteFileW (wszAbsolutePathToScreenshot);
                  DeleteFileW (wszAbsolutePathToThumbnail);

                  delete pop_off;

                  continue;
                }
              }

              rejected_screenshots.push (pop_off);
            }
          }

          while (! rejected_screenshots.empty ())
          {
            SK_D3D9_Screenshot* push_back = nullptr;

            if ( rejected_screenshots.try_pop (push_back) &&
                                               push_back != nullptr )
            {
              screenshot_queue->push (push_back);
            }
          }
        }

        SK_Thread_CloseSelf ();

        CloseHandle (hSignal);

        return 0;
      });
    }

    else
      SetEvent (ReadPointerAcquire (&hSignalScreenshot));
  }
}