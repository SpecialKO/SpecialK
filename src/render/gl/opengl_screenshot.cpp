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

#include <SpecialK/render/gl/opengl_screenshot.h>


extern void SK_Screenshot_PlaySound            (void);
extern void SK_Steam_CatastropicScreenshotFail (void);

SK_GL_Screenshot& SK_GL_Screenshot::operator= (SK_GL_Screenshot&& moveFrom)
{
  if (this != &moveFrom)
  {
    dispose ();

    hglrc                           = moveFrom.hglrc;

    pixel_buffer_object             = moveFrom.pixel_buffer_object;
    pixel_buffer_fence              = moveFrom.pixel_buffer_fence;

    ulCommandIssuedOnFrame          = moveFrom.ulCommandIssuedOnFrame;
    bPlaySound                      = moveFrom.bPlaySound;

    framebuffer.Width               = moveFrom.framebuffer.Width;
    framebuffer.Height              = moveFrom.framebuffer.Height;
    framebuffer.NativeFormat        = moveFrom.framebuffer.NativeFormat;
    framebuffer.PackedDstPitch      = moveFrom.framebuffer.PackedDstPitch;
    framebuffer.PackedDstSlicePitch = moveFrom.framebuffer.PackedDstSlicePitch;
    framebuffer.AlphaMode           = moveFrom.framebuffer.AlphaMode;
    framebuffer.PBufferSize         = moveFrom.framebuffer.PBufferSize;

    framebuffer.PixelBuffer.reset (
      moveFrom.framebuffer.PixelBuffer.get () // XXX: Shouldn't this be release?
    );

    //framebuffer.PixelBuffer.reset   ( moveFrom.framebuffer.PixelBuffer.release () );

    moveFrom.hglrc                           = nullptr;
    moveFrom.pixel_buffer_object             = 0;
    moveFrom.pixel_buffer_fence              = nullptr;

    moveFrom.ulCommandIssuedOnFrame          = 0;

    moveFrom.framebuffer.Width               = 0;
    moveFrom.framebuffer.Height              = 0;
    moveFrom.framebuffer.NativeFormat        = DXGI_FORMAT_UNKNOWN;
    moveFrom.framebuffer.PackedDstPitch      = 0;
    moveFrom.framebuffer.PackedDstSlicePitch = 0;
    moveFrom.framebuffer.AlphaMode           = DXGI_ALPHA_MODE_UNSPECIFIED;
    moveFrom.framebuffer.PBufferSize         = 0;
  }
  return *this;
}

SK_GL_Screenshot::SK_GL_Screenshot (const HGLRC hDevice, bool allow_sound) : hglrc (hDevice)
{
  bPlaySound = allow_sound;

  auto& io =
    ImGui::GetIO ();

  GLint red_bits,  green_bits,
        blue_bits, alpha_bits;

  glGetIntegerv (GL_RED_BITS,   &red_bits);
  glGetIntegerv (GL_GREEN_BITS, &green_bits);
  glGetIntegerv (GL_BLUE_BITS,  &blue_bits);
  glGetIntegerv (GL_ALPHA_BITS, &alpha_bits);

  // We're being lazy here and only handling RGBA8 framebuffers, so...
  //   we need to make it very obvious to the end-user if they're running
  //     software with a different format.
  SK_ReleaseAssert ( red_bits   == green_bits &&
                     green_bits == blue_bits  &&
                     alpha_bits == 8 ||
                     alpha_bits == 0 );

  bool bValidColorDepth =
    ( red_bits + green_bits + blue_bits == 24 &&
                            ( alpha_bits == 0 || alpha_bits == 8 ) );

  if (! bValidColorDepth)
  {
    SK_LOG0 ( ( L"Unsupported GL Framebuffer Format:  R%d, G%d, B%d, A%d",
                  red_bits, green_bits, blue_bits, alpha_bits ),
                L"OpenGLShot" );
  }

  if (hDevice != nullptr && bValidColorDepth)
  {
    static SK_RenderBackend& rb =
      SK_GetCurrentRenderBackend ();

    framebuffer.Width        = static_cast <UINT> (io.DisplaySize.x);
    framebuffer.Height       = static_cast <UINT> (io.DisplaySize.y);
    framebuffer.NativeFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

    DirectX::ComputePitch (
      framebuffer.NativeFormat,
        framebuffer.Width, framebuffer.Height,
          framebuffer.PackedDstPitch, framebuffer.PackedDstSlicePitch
     );

    size_t allocSize =
      (framebuffer.Height + 1)
                          *
       framebuffer.PackedDstPitch;

    GLboolean         PACK_SWAP_BYTES;
    glGetBooleanv (GL_PACK_SWAP_BYTES,   &PACK_SWAP_BYTES);
    glPixelStorei (GL_PACK_SWAP_BYTES,   false);
    GLboolean         PACK_LSB_FIRST;
    glGetBooleanv (GL_PACK_LSB_FIRST,	   &PACK_LSB_FIRST);
    glPixelStorei (GL_PACK_LSB_FIRST,    false);
    GLint             PACK_ROW_LENGTH;
    glGetIntegerv (GL_PACK_ROW_LENGTH,   &PACK_ROW_LENGTH);
    glPixelStorei (GL_PACK_ROW_LENGTH,       0);
    GLint             PACK_IMAGE_HEIGHT;
    glGetIntegerv (GL_PACK_IMAGE_HEIGHT, &PACK_IMAGE_HEIGHT);
    glPixelStorei (GL_PACK_IMAGE_HEIGHT,     0);
    GLint             PACK_SKIP_ROWS;
    glGetIntegerv (GL_PACK_SKIP_ROWS,    &PACK_SKIP_ROWS);
    glPixelStorei (GL_PACK_SKIP_ROWS,        0);
    GLint             PACK_SKIP_PIXELS;
    glGetIntegerv (GL_PACK_SKIP_PIXELS,  &PACK_SKIP_PIXELS);
    glPixelStorei (GL_PACK_SKIP_PIXELS,      0);
    GLint             PACK_SKIP_IMAGES;
    glGetIntegerv (GL_PACK_SKIP_IMAGES,  &PACK_SKIP_IMAGES);
    glPixelStorei (GL_PACK_SKIP_IMAGES,      0);
    GLint             PACK_ALIGNMENT;
    glGetIntegerv (GL_PACK_ALIGNMENT,    &PACK_ALIGNMENT);
    glPixelStorei (GL_PACK_ALIGNMENT,        4);

    GLint                                         orig_pixel_pack_buffer = 0,
                                                  orig_read_buffer       = 0;
    glGetIntegerv (GL_PIXEL_PACK_BUFFER_BINDING, &orig_pixel_pack_buffer);
    glGetIntegerv (GL_READ_BUFFER,               &orig_read_buffer);

    glGenBuffers  (1,                   &pixel_buffer_object);
    glBindBuffer  (GL_PIXEL_PACK_BUFFER, pixel_buffer_object);
    glBufferData  (GL_PIXEL_PACK_BUFFER, allocSize, nullptr, GL_STREAM_READ);

    glReadBuffer  (GL_BACK);
    glReadPixels  (0, 0, framebuffer.Width,
                         framebuffer.Height, GL_RGBA,
                                             GL_UNSIGNED_BYTE, nullptr);
    pixel_buffer_fence =
      glFenceSync (GL_SYNC_GPU_COMMANDS_COMPLETE, 0x0);

    glReadBuffer  (                      orig_read_buffer);
    glBindBuffer  (GL_PIXEL_PACK_BUFFER, orig_pixel_pack_buffer);

    glPixelStorei (GL_PACK_SWAP_BYTES,    PACK_SWAP_BYTES);
    glPixelStorei (GL_PACK_LSB_FIRST,     PACK_LSB_FIRST);
    glPixelStorei (GL_PACK_ROW_LENGTH,    PACK_ROW_LENGTH);
    glPixelStorei (GL_PACK_IMAGE_HEIGHT,  PACK_IMAGE_HEIGHT);
    glPixelStorei (GL_PACK_SKIP_ROWS,     PACK_SKIP_ROWS);
    glPixelStorei (GL_PACK_SKIP_PIXELS,   PACK_SKIP_PIXELS);
    glPixelStorei (GL_PACK_SKIP_IMAGES,   PACK_SKIP_IMAGES);
    glPixelStorei (GL_PACK_ALIGNMENT,     PACK_ALIGNMENT);

    ulCommandIssuedOnFrame = SK_GetFramesDrawn ();

    if (bPlaySound)
      SK_Screenshot_PlaySound ();

    return;
  }

  SK_Steam_CatastropicScreenshotFail ();

  // Something went wrong, crap!
  dispose ();
}

SK_GL_Screenshot::framebuffer_s::PinnedBuffer
SK_GL_Screenshot::framebuffer_s::root_;

void
SK_GL_Screenshot::dispose (void)
{
  hglrc = nullptr;

  if (pixel_buffer_fence != nullptr)
  {
    GLint64                                       max_timeout;
    glGetInteger64v (GL_MAX_SERVER_WAIT_TIMEOUT, &max_timeout);

    glWaitSync   (   pixel_buffer_fence, 0x0,
                          static_cast <GLuint64> (max_timeout));
    glClientWaitSync(pixel_buffer_fence, 0x0, 250000000); // 250 ms

    glDeleteSync (
      std::exchange (pixel_buffer_fence, nullptr)
    );
  }

  if (pixel_buffer_object != 0)
  {
    glDeleteBuffers (1, &pixel_buffer_object);
                         pixel_buffer_object = 0;
  }

  if ( framebuffer.PixelBuffer ==
       framebuffer.root_.bytes )
  {
    framebuffer.PixelBuffer.release ();
    framebuffer.PBufferSize =
      framebuffer.root_.size;
  }

  framebuffer.PixelBuffer.reset (nullptr);

  size_t before =
    SK_ScreenshotQueue::pooled.capture_bytes.load ();

  SK_ScreenshotQueue::pooled.capture_bytes -=
    std::exchange (
      framebuffer.PBufferSize, 0
    );

  if (before < SK_ScreenshotQueue::pooled.capture_bytes.load  ( ))
  {            SK_ScreenshotQueue::pooled.capture_bytes.store (0); if (config.system.log_level > 1) SK_ReleaseAssert (false && "capture underflow"); }
};

bool
SK_GL_Screenshot::getData ( UINT* const pWidth,
                            UINT* const pHeight,
                            uint8_t   **ppData,
                            bool        Wait )
{
  auto& pooled =
    SK_ScreenshotQueue::pooled;

  auto ReadBack =
  [&]
  {
    size_t allocSize =
      (framebuffer.Height + 1)
                          *
       framebuffer.PackedDstPitch;

    GLint                                         orig_pixel_pack_buffer = 0;
    glGetIntegerv (GL_PIXEL_PACK_BUFFER_BINDING, &orig_pixel_pack_buffer);
    glBindBuffer  (GL_PIXEL_PACK_BUFFER,          pixel_buffer_object);

    uint8_t *pSrc = (uint8_t *)
      glMapBuffer (GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);

    size_t RowPitch =
      framebuffer.PackedDstPitch;

    if (pSrc != nullptr)
    {
      try
      {
        // Stash in Root?
        //  ( Less dynamic allocation for base-case )
        if ( pooled.capture_bytes.fetch_add  ( allocSize ) == 0 )
        {
          if (framebuffer.root_.size.load () < allocSize)
          {
            framebuffer.root_.bytes =
                std::make_unique <uint8_t []> (allocSize);
            framebuffer.root_.size.store      (allocSize);
          }

          allocSize =
            framebuffer.root_.size;

          framebuffer.PixelBuffer.reset (
            framebuffer.root_.bytes.get ()
          );
        }

        else
        {
          framebuffer.PixelBuffer =
            std::make_unique <uint8_t []> (
                                    allocSize );
        }

        *pWidth  = framebuffer.Width;
        *pHeight = framebuffer.Height;

        framebuffer.PBufferSize = allocSize;

        uint8_t* pDst = framebuffer.PixelBuffer.get ();

        // We have to flip OpenGL's framebuffer
        //
        pSrc = (
          pSrc + RowPitch * framebuffer.Height
               );

        for ( UINT i = 0; i < framebuffer.Height; ++i )
        {
          memcpy ( pDst, pSrc, RowPitch );

          pSrc -=                   RowPitch;
          pDst += framebuffer.PackedDstPitch;
        }
      }

      catch (const std::exception&)
      {
        pooled.capture_bytes   -= allocSize;
      }

      glUnmapBuffer (GL_PIXEL_PACK_BUFFER);
      glBindBuffer  (GL_PIXEL_PACK_BUFFER, orig_pixel_pack_buffer);

      SK_LOG0 ( ( L"Screenshot Readback Complete after %li frames",
                    SK_GetFramesDrawn () - ulCommandIssuedOnFrame ),
                  L"OpenGLSShot" );

      if (framebuffer.PBufferSize > 0)
      {
        *ppData =
          framebuffer.PixelBuffer.get ();

        return true;
      }
    }

    dispose ();

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


static SK_LazyGlobal <concurrency::concurrent_queue <SK_GL_Screenshot *>> screenshot_queue;
static SK_LazyGlobal <concurrency::concurrent_queue <SK_GL_Screenshot *>> screenshot_write_queue;

bool
SK_GL_CaptureScreenshot  ( SK_ScreenshotStage when =
                              SK_ScreenshotStage::EndOfFrame,
                           bool               allow_sound = true )
{
  static const SK_RenderBackend_V2& rb =
    SK_GetCurrentRenderBackend ();

  if ( ((int)rb.api & (int)SK_RenderAPI::OpenGL)
                   == (int)SK_RenderAPI::OpenGL )
  {
    static const
      std::map <SK_ScreenshotStage, int>
        __stage_map = {
          { SK_ScreenshotStage::BeforeGameHUD, 0 },
          { SK_ScreenshotStage::BeforeOSD,     1 },
          { SK_ScreenshotStage::EndOfFrame,    2 }
        };

    const auto it =
               __stage_map.find (when);
    if ( it != __stage_map.cend (    ) )
    {
      const int stage =
        it->second;

      if (when == SK_ScreenshotStage::BeforeGameHUD)
      {
        return false;
      }

      InterlockedIncrement (
        &enqueued_screenshots.stages [stage]
      );

      if (allow_sound)
      {
        InterlockedIncrement (
          &enqueued_sounds.stages [stage]
        );
      }

      return true;
    }
  }

  return false;
}


void
SK_GL_ProcessScreenshotQueueEx ( SK_ScreenshotStage stage_ = SK_ScreenshotStage::EndOfFrame,
                                 bool               wait   = false,
                                 bool               purge  = false )
{
  static auto& rb =
    SK_GetCurrentRenderBackend ();

  constexpr int __MaxStage = 2;
  const     int      stage =
    sk::narrow_cast <int> (stage_);

  assert ( stage >= 0 &&
           stage <= ( __MaxStage + 1 ) );

  if ( stage < 0 ||
       stage > ( __MaxStage + 1 ) )
    return;


  if ( stage == ( __MaxStage + 1 ) && purge )
  {
    // Empty all stage queues first, then we
    //   can wait for outstanding screenshots
    //     to finish.
    for ( int implied_stage =  0          ;
              implied_stage <= __MaxStage ;
            ++implied_stage                 )
    {
      InterlockedExchange (
        &enqueued_screenshots.stages [
              implied_stage          ],   0
      );
    }
  }


  else if (stage <= __MaxStage)
  {
    if (ReadAcquire (&enqueued_screenshots.stages [stage]) > 0)
    {
      // Just kill any queued shots, we need to be quick about this.
      if (purge)
        InterlockedExchange      (&enqueued_screenshots.stages [stage], 0);

      else
      {
        if (InterlockedDecrement (&enqueued_screenshots.stages [stage]) >= 0)
        {    // --
          HGLRC hglrc =
            SK_GL_GetCurrentContext ();

          if ( SK_ScreenshotQueue::pooled.capture_bytes.load  () <
               SK_ScreenshotQueue::maximum.capture_bytes.load () )
          {
            if (SK_GetCurrentRenderBackend ().screenshot_mgr.checkDiskSpace (20ULL * 1024ULL * 1024ULL))
            {
              bool allow_sound =
                ReadAcquire (&enqueued_sounds.stages [stage]) > 0;

              if ( allow_sound )
                InterlockedDecrement (&enqueued_sounds.stages [stage]);

              screenshot_queue->push (
                new SK_GL_Screenshot (
                  hglrc, allow_sound
                )
              );
            }
          }

          else {
            SK_RunOnce (
              SK_ImGui_Warning (L"Too much screenshot data queued")
            );
          }
        }

        else // ++
          InterlockedIncrement   (&enqueued_screenshots.stages [stage]);
      }
    }
  }

  static volatile LONG _lockVal = 0;

  struct signals_s {
    HANDLE capture    = INVALID_HANDLE_VALUE;
    HANDLE hq_encode  = INVALID_HANDLE_VALUE;

    struct {
      HANDLE initiate = INVALID_HANDLE_VALUE;
      HANDLE finished = INVALID_HANDLE_VALUE;
    } abort;
  } static signal = { };

  static HANDLE
    hWriteThread      = INVALID_HANDLE_VALUE;


  if ( 0                    == InterlockedCompareExchange (&_lockVal, 1, 0) &&
       INVALID_HANDLE_VALUE == signal.capture )
  {
    signal.capture        = SK_CreateEvent ( nullptr, FALSE, TRUE,  nullptr );
    signal.abort.initiate = SK_CreateEvent ( nullptr, TRUE,  FALSE, nullptr );
    signal.abort.finished = SK_CreateEvent ( nullptr, FALSE, FALSE, nullptr );
    signal.hq_encode      = SK_CreateEvent ( nullptr, FALSE, FALSE, nullptr );

    hWriteThread =
    SK_Thread_CreateEx ([](LPVOID) -> DWORD
    {
      SetThreadPriority ( SK_GetCurrentThread (), THREAD_PRIORITY_NORMAL );

      do
      {
        HANDLE signals [] = {
          signal.capture,       // Screenshots are waiting for write
          signal.abort.initiate // Screenshot full-abort requested (i.e. for SwapChain Resize)
        };

        DWORD dwWait =
          WaitForMultipleObjects ( 2, signals, FALSE, INFINITE );

        bool
          purge_and_run =
            ( dwWait == ( WAIT_OBJECT_0 + 1 ) );

        static std::vector <SK_GL_Screenshot*> to_write;

        while (! screenshot_write_queue->empty ())
        {
          SK_GL_Screenshot*                     pop_off   = nullptr;
          if ( screenshot_write_queue->try_pop (pop_off) &&
                                                pop_off  != nullptr )
          {
            if (purge_and_run)
            {
              delete pop_off;
              continue;
            }

            if (config.screenshots.png_compress)
              to_write.emplace_back (pop_off);

            SK_GL_Screenshot::framebuffer_s* pFrameData =
              pop_off->getFinishedData ();

            // Why's it on the wait-queue if it's not finished?!
            assert (pFrameData != nullptr);

            if (pFrameData != nullptr)
            {
              using namespace DirectX;

              bool  skip_me = false;
              Image raw_img = {   };

              ComputePitch (
                pFrameData->NativeFormat,
                  pFrameData->Width, pFrameData->Height,
                    raw_img.rowPitch, raw_img.slicePitch
              );

              // Steam wants JPG, smart people want PNG
              DirectX::WICCodecs codec = WIC_CODEC_JPEG;

              raw_img.format = pop_off->getInternalFormat ();
              raw_img.width  = pFrameData->Width;
              raw_img.height = pFrameData->Height;
              raw_img.pixels = pFrameData->PixelBuffer.get ();

              bool hdr = ( rb.isHDRCapable () &&
                           rb.isHDRActive  () );

              SK_RunOnce (SK_SteamAPI_InitManagers ());

              wchar_t       wszAbsolutePathToScreenshot [ MAX_PATH + 2 ] = { };
              wcsncpy_s   ( wszAbsolutePathToScreenshot,  MAX_PATH,
                              rb.screenshot_mgr.getBasePath (),
                                _TRUNCATE );

              if ( config.steam.screenshots.enable_hook &&
                          steam_ctx.Screenshots ()      != nullptr )
              {
                PathAppendW          (wszAbsolutePathToScreenshot, L"SK_SteamScreenshotImport.jpg");
                SK_CreateDirectories (wszAbsolutePathToScreenshot);
              }

              else if ( hdr )
              {
                time_t screenshot_time = 0;
                       codec           = WIC_CODEC_PNG;

                PathAppendW (         wszAbsolutePathToScreenshot,
                  SK_FormatStringW ( LR"(LDR\%lu.png)",
                              time (&screenshot_time) ).c_str () );
                SK_CreateDirectories (wszAbsolutePathToScreenshot);
              }

              // Not HDR and not importing to Steam,
              //   we've got nothing left to do...
              else
              {
                skip_me = true;
              }

              if ((! skip_me) && pop_off != nullptr)
              {
                ScratchImage
                  un_srgb;
                  un_srgb.InitializeFromImage (raw_img);

                DirectX::TexMetadata
                meta           = {           };
                meta.width     = raw_img.width;
                meta.height    = raw_img.height;
                meta.depth     = 1;
                meta.format    = raw_img.format;
                meta.dimension = TEX_DIMENSION_TEXTURE2D;
                meta.arraySize = 1;
                meta.mipLevels = 1;

                switch (pFrameData->AlphaMode)
                {
                  case DXGI_ALPHA_MODE_UNSPECIFIED:
                    meta.SetAlphaMode (TEX_ALPHA_MODE_UNKNOWN);
                    break;
                  case DXGI_ALPHA_MODE_PREMULTIPLIED:
                    meta.SetAlphaMode (TEX_ALPHA_MODE_PREMULTIPLIED);
                    break;
                  case DXGI_ALPHA_MODE_STRAIGHT:
                    meta.SetAlphaMode (TEX_ALPHA_MODE_STRAIGHT);
                    break;
                  case DXGI_ALPHA_MODE_IGNORE:
                    meta.SetAlphaMode (TEX_ALPHA_MODE_OPAQUE);
                    break;
                }

                meta.SetAlphaMode (TEX_ALPHA_MODE_OPAQUE);

                ScratchImage
                  un_scrgb;
                  un_scrgb.Initialize (meta);

                static const XMVECTORF32 c_MaxNitsFor2084 =
                  { 10000.0f, 10000.0f, 10000.0f, 1.f };

                static const XMMATRIX c_from2020to709 =
                {
                  { 1.6604910f,  -0.1245505f, -0.0181508f, 0.f },
                  { -0.5876411f,  1.1328999f, -0.1005789f, 0.f },
                  { -0.0728499f, -0.0083494f,  1.1187297f, 0.f },
                  { 0.f,          0.f,         0.f,        1.f }
                };

                auto RemoveGamma_sRGB = [](XMVECTOR value) ->
                void
                {
                  value.m128_f32 [0] = ( value.m128_f32 [0] < 0.04045f ) ?
                                         value.m128_f32 [0] / 12.92f     :
                                   pow ((value.m128_f32 [0] + 0.055f) / 1.055f, 2.4f);
                  value.m128_f32 [1] = ( value.m128_f32 [1] < 0.04045f ) ?
                                         value.m128_f32 [1] / 12.92f     :
                                   pow ((value.m128_f32 [1] + 0.055f) / 1.055f, 2.4f);
                  value.m128_f32 [2] = ( value.m128_f32 [2] < 0.04045f ) ?
                                         value.m128_f32 [2] / 12.92f     :
                                   pow ((value.m128_f32 [2] + 0.055f) / 1.055f, 2.4f);
                };

                auto ApplyGamma_sRGB = [](XMVECTOR value) ->
                void
                {
                  value.m128_f32 [0] = ( value.m128_f32 [0] < 0.0031308f ) ?
                                         value.m128_f32 [0] * 12.92f      :
                           1.055f * pow (value.m128_f32 [0], 1.0f / 2.4f) - 0.055f;
                  value.m128_f32 [1] = ( value.m128_f32 [1] < 0.0031308f ) ?
                                         value.m128_f32 [1] * 12.92f      :
                           1.055f * pow (value.m128_f32 [1], 1.0f / 2.4f) - 0.055f;
                  value.m128_f32 [2] = ( value.m128_f32 [2] < 0.0031308f ) ?
                                         value.m128_f32 [2] * 12.92f      :
                           1.055f * pow (value.m128_f32 [2], 1.0f / 2.4f) - 0.055f;
                };

                HRESULT hr = S_OK;

                if ( un_srgb.GetImages  () &&
                     hdr                   && raw_img.format != DXGI_FORMAT_R16G16B16A16_FLOAT &&
                     rb.scanout.getEOTF () == SK_RenderBackend::scan_out_s::SMPTE_2084 )
                { // ^^^ EOTF is not always accurate, but we know SMPTE 2084 is not used w/ FP16 color
                  TransformImage ( un_srgb.GetImages     (),
                                   un_srgb.GetImageCount (),
                                   un_srgb.GetMetadata   (),
                    [&](XMVECTOR* outPixels, const XMVECTOR* inPixels, size_t width, size_t y)
                  {
                    UNREFERENCED_PARAMETER(y);

                    for (size_t j = 0; j < width; ++j)
                    {
                      XMVECTOR value  = inPixels [j];
                      XMVECTOR nvalue = XMVector3Transform (value, c_from2020to709);
                                value = XMVectorSelect     (value, nvalue, g_XMSelect1110);

                      ApplyGamma_sRGB (value);

                      outPixels [j]   =                     value;
                    }
                  }, un_scrgb);

                  std::swap (un_scrgb, un_srgb);
                }

                XMVECTOR maxLum = XMVectorZero ();

                hr =              un_srgb.GetImages     () ?
                  EvaluateImage ( un_srgb.GetImages     (),
                                  un_srgb.GetImageCount (),
                                  un_srgb.GetMetadata   (),
                  [&](const XMVECTOR* pixels, size_t width, size_t y)
                  {
                    UNREFERENCED_PARAMETER(y);

                    for (size_t j = 0; j < width; ++j)
                    {
                      static const XMVECTORF32 s_luminance =
                      //{ 0.3f, 0.59f, 0.11f, 0.f };
                      { 0.2125862307855955516f,
                        0.7151703037034108499f,
                        0.07220049864333622685f };

                      XMVECTOR v = *pixels++;
                      RemoveGamma_sRGB          (v);
                               v = XMVector3Dot (v, s_luminance);

                      maxLum =
                        XMVectorMax (v, maxLum);
                    }
                  })                                       : E_POINTER;

#define _CHROMA_SCALE TRUE
#ifndef _CHROMA_SCALE
                  maxLum =
                    XMVector3Length  (maxLum);
#else
                  maxLum =
                    XMVectorMultiply (maxLum, maxLum);
#endif

                  hr =               un_srgb.GetImages     () ?
                    TransformImage ( un_srgb.GetImages     (),
                                     un_srgb.GetImageCount (),
                                     un_srgb.GetMetadata   (),
                    [&](XMVECTOR* outPixels, const XMVECTOR* inPixels, size_t width, size_t y)
                    {
                      UNREFERENCED_PARAMETER(y);

                      for (size_t j = 0; j < width; ++j)
                      {
                        XMVECTOR value = inPixels [j];

                        XMVECTOR scale =
                          XMVectorDivide (
                            XMVectorAdd (
                              g_XMOne, XMVectorDivide ( value,
                                                          maxLum
                                                      )
                                        ),
                            XMVectorAdd (
                              g_XMOne, value
                                        )
                          );

                        XMVECTOR nvalue =
                          XMVectorMultiply (value, scale);
                                  value =
                          XMVectorSelect   (value, nvalue, g_XMSelect1110);
                        outPixels [j]   =   value;
                      }
                    }, un_scrgb)                             : E_POINTER;

                std::swap (un_srgb, un_scrgb);

                if (         un_srgb.GetImages ()) {
                  Convert ( *un_srgb.GetImages (),
                              DXGI_FORMAT_B8G8R8X8_UNORM,
                                TEX_FILTER_DITHER |
                                TEX_FILTER_SRGB,
                                  TEX_THRESHOLD_DEFAULT,
                                    un_scrgb );
                }

                if (un_scrgb.GetImages ())
                {
                  rb.screenshot_mgr.copyToClipboard (*un_scrgb.GetImages ());
                }

                if (               un_scrgb.GetImages () &&
                      SUCCEEDED (
                  SaveToWICFile ( *un_scrgb.GetImages (), WIC_FLAGS_NONE,
                                     GetWICCodec         (codec),
                                      wszAbsolutePathToScreenshot )
                               )
                   )
                {
                  if ( config.steam.screenshots.enable_hook &&
                          steam_ctx.Screenshots () != nullptr )
                  {
                    wchar_t       wszAbsolutePathToThumbnail [ MAX_PATH + 2 ] = { };
                    wcsncpy_s   ( wszAbsolutePathToThumbnail,  MAX_PATH,
                                    rb.screenshot_mgr.getBasePath (),
                                      _TRUNCATE );

                    PathAppendW (wszAbsolutePathToThumbnail, L"SK_SteamThumbnailImport.jpg");

                    float aspect = (float)pFrameData->Height /
                                   (float)pFrameData->Width;

                    ScratchImage thumbnailImage;

                    Resize ( *un_scrgb.GetImages (), 200,
                               static_cast <size_t> (200.0 * aspect),
                                TEX_FILTER_DITHER_DIFFUSION | TEX_FILTER_FORCE_WIC
                              | TEX_FILTER_TRIANGLE,
                                  thumbnailImage );

                    if (               thumbnailImage.GetImages ()) {
                      SaveToWICFile ( *thumbnailImage.GetImages (), WIC_FLAGS_DITHER,
                                        GetWICCodec                (codec),
                                          wszAbsolutePathToThumbnail );

                      std::string ss_path (
                        SK_WideCharToUTF8 (wszAbsolutePathToScreenshot)
                      );

                      std::string ss_thumb (
                        SK_WideCharToUTF8 (wszAbsolutePathToThumbnail)
                      );

                      ScreenshotHandle screenshot =
                        SK_SteamAPI_AddScreenshotToLibraryEx (
                          ss_path.c_str    (),
                            ss_thumb.c_str (),
                              pFrameData->Width, pFrameData->Height,
                                true );

                      SK_LOG1 ( ( L"Finished Steam Screenshot Import for Handle: '%x' (%lu frame latency)",
                                  screenshot, SK_GetFramesDrawn () - pop_off->getStartFrame () ),
                                    L"SteamSShot" );

                      // Remove the temporary files...
                      DeleteFileW (wszAbsolutePathToThumbnail);
                    }
                    DeleteFileW (wszAbsolutePathToScreenshot);
                  }
                }
              }

              if (! config.screenshots.png_compress)
              {
                delete pop_off;
                       pop_off = nullptr;
              }
            }
          }
        }

        if (purge_and_run)
        {
          SetThreadPriority ( SK_GetCurrentThread (), THREAD_PRIORITY_NORMAL );

          purge_and_run = false;

          ResetEvent (signal.abort.initiate); // Abort no longer needed
          SetEvent   (signal.abort.finished); // Abort is complete
        }

        if (config.screenshots.png_compress || config.screenshots.copy_to_clipboard)
        {
          int enqueued_lossless = 0;

          while ( ! to_write.empty () )
          {
            auto it =
              to_write.back ();

            to_write.pop_back ();

            if ( it                     == nullptr ||
                 it->getFinishedData () == nullptr )
            {
              delete it;

              continue;
            }

            static concurrency::concurrent_queue <SK_GL_Screenshot::framebuffer_s*>
              raw_images_;

            SK_GL_Screenshot::framebuffer_s* fb_orig =
              it->getFinishedData ();

            SK_ReleaseAssert (fb_orig != nullptr);

            if (fb_orig != nullptr)
            {
              SK_GL_Screenshot::framebuffer_s* fb_copy =
                new SK_GL_Screenshot::framebuffer_s ();

              fb_copy->Height              = fb_orig->Height; //-V522
              fb_copy->Width               = fb_orig->Width;
              fb_copy->NativeFormat        = fb_orig->NativeFormat;
              fb_copy->AlphaMode           = fb_orig->AlphaMode;
              fb_copy->PBufferSize         = fb_orig->PBufferSize;
              fb_copy->PackedDstPitch      = fb_orig->PackedDstPitch;
              fb_copy->PackedDstSlicePitch = fb_orig->PackedDstSlicePitch;

              fb_copy->PixelBuffer =
                std::move (fb_orig->PixelBuffer);

              raw_images_.push (fb_copy);

              ++enqueued_lossless;
            }

            delete it;

            static volatile HANDLE
              hThread = INVALID_HANDLE_VALUE;

            if (InterlockedCompareExchangePointer (&hThread, 0, INVALID_HANDLE_VALUE) == INVALID_HANDLE_VALUE)
            {                                     SK_Thread_CreateEx ([](LPVOID pUser)->DWORD {
              concurrency::concurrent_queue <SK_GL_Screenshot::framebuffer_s *>*
                images_to_write =
                  (concurrency::concurrent_queue <SK_GL_Screenshot::framebuffer_s *>*)pUser;

              SetThreadPriority           ( SK_GetCurrentThread (), THREAD_MODE_BACKGROUND_BEGIN );
              SetThreadPriority           ( SK_GetCurrentThread (), THREAD_PRIORITY_BELOW_NORMAL );

              while (0 == ReadAcquire (&__SK_DLL_Ending))
              {
                SK_GL_Screenshot::framebuffer_s *pFrameData =
                  nullptr;

                SK_WaitForSingleObject  (
                  signal.hq_encode, INFINITE
                );

                while   (! images_to_write->empty   (          ))
                { while (! images_to_write->try_pop (pFrameData))
                  {
                    SwitchToThread ();
                  }

                  if (ReadAcquire (&__SK_DLL_Ending))
                    break;

                  time_t screenshot_time = 0;

                  wchar_t       wszAbsolutePathToLossless [ MAX_PATH + 2 ] = { };
                  wcsncpy_s   ( wszAbsolutePathToLossless,  MAX_PATH,
                                  rb.screenshot_mgr.getBasePath (),
                                    _TRUNCATE );

                  bool hdr =
                    ( SK_GetCurrentRenderBackend ().isHDRCapable () &&
                      SK_GetCurrentRenderBackend ().isHDRActive  () );

                  if (hdr)
                  {
                    PathAppendW ( wszAbsolutePathToLossless,
                      SK_FormatStringW ( L"HDR\\%lu.jxr",
                                  time (&screenshot_time) ).c_str () );
                  }

                  else
                  {
                    PathAppendW ( wszAbsolutePathToLossless,
                      SK_FormatStringW ( L"Lossless\\%lu.png",
                                  time (&screenshot_time) ).c_str () );
                  }

                  // Why's it on the wait-queue if it's not finished?!
                  assert (pFrameData != nullptr);

                  if (pFrameData != nullptr)
                  {
                    using namespace DirectX;

                    Image raw_img = { };

                    uint8_t* pDst =
                      pFrameData->PixelBuffer.get ();

                    for (UINT i = 0; i < pFrameData->Height; ++i)
                    {
                      // Eliminate pre-multiplied alpha problems (the stupid way)
                      switch (pFrameData->NativeFormat)
                      {
                        case DXGI_FORMAT_B8G8R8A8_UNORM:
                        case DXGI_FORMAT_R8G8B8A8_UNORM:
                        {
                          for ( UINT j = 3                          ;
                                     j < pFrameData->PackedDstPitch ;
                                     j += 4 )
                          {    pDst [j] = 255UL;        }
                        } break;

                        case DXGI_FORMAT_R10G10B10A2_UNORM:
                        {
                          for ( UINT j = 3                          ;
                                     j < pFrameData->PackedDstPitch ;
                                     j += 4 )
                          {    pDst [j]  |=  0x3;       }
                        } break;

                        case DXGI_FORMAT_R16G16B16A16_FLOAT:
                        {
                          for ( UINT j  = 0                          ;
                                     j < pFrameData->PackedDstPitch  ;
                                     j += 8 )
                          {
                            glm::vec4 color =
                              glm::unpackHalf4x16 (*((uint64*)&(pDst [j])));

                            color.a = 1.0f;

                            *((uint64*)& (pDst[j])) =
                              glm::packHalf4x16 (color);
                          }
                        } break;
                      }

                      pDst += pFrameData->PackedDstPitch;
                    }

                    ComputePitch ( pFrameData->NativeFormat,
                                     pFrameData->Width,
                                     pFrameData->Height,
                                       raw_img.rowPitch,
                                       raw_img.slicePitch
                    );

                    raw_img.format = pFrameData->NativeFormat;
                    raw_img.width  = pFrameData->Width;
                    raw_img.height = pFrameData->Height;
                    raw_img.pixels = pFrameData->PixelBuffer.get ();

                    if (config.screenshots.copy_to_clipboard)
                    {
                      ScratchImage
                        clipboard;
                        clipboard.InitializeFromImage (raw_img);

                      if (SUCCEEDED ( Convert ( raw_img,
                                                  DXGI_FORMAT_B8G8R8X8_UNORM,
                                                    TEX_FILTER_DITHER,
                                                      TEX_THRESHOLD_DEFAULT,
                                                        clipboard ) ) )
                      {
                        if (! hdr)
                          rb.screenshot_mgr.copyToClipboard (*clipboard.GetImages ());
                      }
                    }

                    if (config.screenshots.png_compress)
                    {
                      SK_CreateDirectories (wszAbsolutePathToLossless);

                      ScratchImage
                        un_srgb;
                        un_srgb.InitializeFromImage (raw_img);

                      HRESULT hrSaveToWIC =     un_srgb.GetImages () ?
                                SaveToWICFile (*un_srgb.GetImages (), WIC_FLAGS_DITHER,
                                        GetWICCodec (hdr ? WIC_CODEC_WMP :
                                                           WIC_CODEC_PNG),
                                             wszAbsolutePathToLossless,
                                               hdr ?
                                                 &GUID_WICPixelFormat64bppRGBAHalf :
                                                 pFrameData->NativeFormat == DXGI_FORMAT_R10G10B10A2_UNORM ?
                                                                              &GUID_WICPixelFormat48bppRGB :
                                                                              &GUID_WICPixelFormat24bppBGR)
                                                                     : E_POINTER;

                      if (SUCCEEDED (hrSaveToWIC))
                      {
                        // Refresh
                        rb.screenshot_mgr.getRepoStats (true);
                      }

                      else
                      {
                        SK_LOG0 ( ( L"Unable to write Screenshot, hr=%s",
                                                       SK_DescribeHRESULT (hrSaveToWIC) ),
                                    L"OpenGLSShot" );

                        SK_ImGui_Warning ( L"Smart Screenshot Capture Failed.\n\n"
                                           L"\t\t\t\t>> More than likely this is a problem with MSAA or Windows 7\n\n"
                                           L"\t\tTo prevent future problems, disable this under Steam Enhancements / Screenshots" );
                      }
                    }

                    delete pFrameData;
                  }
                }
              }

              SK_Thread_CloseSelf ();

              return 0;
            }, L"[SK] OpenGL Screenshot Encoder",
              (LPVOID)&raw_images_ );
          } }

          if (enqueued_lossless > 0)
          {   enqueued_lossless = 0;
            SetEvent (signal.hq_encode);
          }
        }
      } while (0 == ReadAcquire (&__SK_DLL_Ending));

      SK_Thread_CloseSelf ();

      SK_CloseHandle (signal.capture);
      SK_CloseHandle (signal.abort.initiate);
      SK_CloseHandle (signal.abort.finished);
      SK_CloseHandle (signal.hq_encode);

      return 0;
    }, L"[SK] OpenGL Screenshot Capture" );

    InterlockedIncrement (&_lockVal);
  }

  else
    SK_Thread_SpinUntilAtomicMin (&_lockVal, 2);


  if (stage_ != SK_ScreenshotStage::_FlushQueue && wait == false && purge == false)
    return;


  // Any incomplete captures are pushed onto this queue, and then the pending
  //   queue (once drained) is re-built.
  //
  //  This is faster than iterating a synchronized list in highly multi-threaded engines.
  static concurrency::concurrent_queue <SK_GL_Screenshot *> rejected_screenshots;

  bool new_jobs = false;

  do
  {
    do
    {
      SK_GL_Screenshot*               pop_off   = nullptr;
      if ( screenshot_queue->try_pop (pop_off) &&
                                      pop_off  != nullptr )
      {
        if (purge)
          delete pop_off;

        else
        {
          UINT     Width, Height;
          uint8_t* pData;

          // There is a condition in which waiting is necessary;
          //   after a swapchain resize or fullscreen mode switch.
          //
          //  * For now, ignore this problem until it actuall poses one.
          //
          if (pop_off->getData (&Width, &Height, &pData, wait))
          {
            screenshot_write_queue->push (pop_off);
            new_jobs = true;
          }

          else
            rejected_screenshots.push (pop_off);
        }
      }
    } while ((! screenshot_queue->empty ()) && (purge || wait));

    do
    {
      SK_GL_Screenshot*                  push_back   = nullptr;
      if ( rejected_screenshots.try_pop (push_back) &&
                                         push_back  != nullptr )
      {
        if (purge)
          delete push_back;

        else
          screenshot_queue->push (push_back);
      }
    } while ((! rejected_screenshots.empty ()) && (purge || wait));

    if ( wait ||
                 purge )
    {
      if ( screenshot_queue->empty    () &&
           rejected_screenshots.empty ()    )
      {
        if ( purge && (! screenshot_write_queue->empty ()) )
        {
          SetThreadPriority   ( hWriteThread,          THREAD_PRIORITY_TIME_CRITICAL );
          SignalObjectAndWait ( signal.abort.initiate,
                                signal.abort.finished, INFINITE,              FALSE  );
        }

        wait  = false;
        purge = false;

        break;
      }
    }
  } while ( wait ||
                    purge );

  if (new_jobs)
    SetEvent (signal.capture);
}

void
SK_GL_ProcessScreenshotQueue (SK_ScreenshotStage stage)
{
  SK_GL_ProcessScreenshotQueueEx (stage, false);
}