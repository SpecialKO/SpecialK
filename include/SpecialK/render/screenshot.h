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
#include <DirectXMath.h>

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

  enum SnippingState {
    SnippingInactive,
    SnippingRequested,
    SnippingActive,
    SnippingComplete
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

  SnippingState            getSnipState (void) const;
  void                     setSnipState (SnippingState state);
  void                     setSnipRect  (const DirectX::Rect& rect);
  DirectX::Rect            getSnipRect  (void) const;

protected:
  screenshot_repository_s screenshots = { };
  SnippingState           snip_state  = SnippingInactive;
  DirectX::Rect           snip_rect   = { 0,0,0,0 };

private:
};

float             LinearToPQY   (float N);
float             PQToLinearY   (float N);
DirectX::XMVECTOR Rec709toICtCp (DirectX::XMVECTOR N);
DirectX::XMVECTOR ICtCptoRec709 (DirectX::XMVECTOR N);

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
bool SK_Screenshot_SaveJXL  (DirectX::ScratchImage &src_image, const wchar_t *wszFilePath);
void SK_Screenshot_SaveUHDR (const DirectX::Image& hdr10_image, const DirectX::Image& sdr_image, const wchar_t* wszFileName);

void SK_WIC_SetMaximumQuality (IPropertyBag2 *props);
void SK_WIC_SetBasicMetadata  (IWICMetadataQueryWriter *pMQW);
void SK_WIC_SetMetadataTitle  (IWICMetadataQueryWriter *pMQW, std::string& title);

bool SK_PNG_MakeHDR (const wchar_t*        wszFilePath,
                     const DirectX::Image& encoded_img,
                     const DirectX::Image& raw_img);

bool SK_PNG_CopyToClipboard   (const DirectX::Image& image, const void *pData, size_t data_size);
bool SK_HDR_ConvertImageToPNG (const DirectX::Image& raw_hdr_img, DirectX::ScratchImage& png_img);
bool SK_HDR_SavePNGToDisk     (const wchar_t* wszPNGPath, const DirectX::Image* png_image,
                                                          const DirectX::Image* raw_image,
                                  const char* szUtf8MetadataTitle = nullptr);

extern volatile LONG __SK_ScreenShot_CapturingHUDless;

extern volatile LONG __SK_D3D11_QueuedShots;
extern volatile LONG __SK_D3D12_QueuedShots;

extern volatile LONG __SK_D3D11_InitiateHudFreeShot;
extern volatile LONG __SK_D3D12_InitiateHudFreeShot;

extern          bool   SK_D3D12_ShouldSkipHUD (void);

struct parallel_tonemap_job_s {
  HANDLE             hStartEvent;
  HANDLE             hCompletionEvent;

  DirectX::XMVECTOR* pFirstPixel;
  DirectX::XMVECTOR* pLastPixel;

  DirectX::XMVECTOR  maxTonemappedRGB;
  float              SDR_YInPQ;
  float              maxYInPQ;

  int                job_id;
};

void SK_Image_EnqueueTonemapTask ( DirectX::ScratchImage&                image,
                                   std::vector <parallel_tonemap_job_s>& jobs,
                                   std::vector <DirectX::XMVECTOR>&      pixels,
                                   float                                 maxLuminance,
                                   float                                 sdrLuminance);
void SK_Image_InitializeTonemap   (std::vector <parallel_tonemap_job_s>& jobs,
                                   std::vector <HANDLE>&       parallel_start,
                                   std::vector <HANDLE>&      parallel_finish);
void SK_Image_DispatchTonemapJobs (std::vector <parallel_tonemap_job_s>& jobs);
void SK_Image_GetTonemappedPixels (DirectX::ScratchImage&                output,
                                   DirectX::ScratchImage&                source,
                                   std::vector <DirectX::XMVECTOR>&      pixels,
                                   std::vector <HANDLE>&                 fence);

struct ParamsPQ
{
  DirectX::XMVECTOR N,       M;
  DirectX::XMVECTOR C1, C2, C3;
  DirectX::XMVECTOR MaxPQ;
  DirectX::XMVECTOR RcpN, RcpM;
};

extern const ParamsPQ PQ;

constexpr DirectX::XMMATRIX c_from709to2020 = // Transposed
{
  { 0.627403914928436279296875f,     0.069097287952899932861328125f,    0.01639143936336040496826171875f, 0.0f },
  { 0.3292830288410186767578125f,    0.9195404052734375f,               0.08801330626010894775390625f,    0.0f },
  { 0.0433130674064159393310546875f, 0.011362315155565738677978515625f, 0.895595252513885498046875f,      0.0f },
  { 0.0f,                            0.0f,                              0.0f,                             1.0f }
};

constexpr DirectX::XMMATRIX c_from2020toXYZ = // Transposed
{
  { 0.636958062648773193359375f,  0.26270020008087158203125f,      0.0f,                           0.0f },
  { 0.144616901874542236328125f,  0.677998065948486328125f,        0.028072692453861236572265625f, 0.0f },
  { 0.1688809692859649658203125f, 0.0593017153441905975341796875f, 1.060985088348388671875f,       0.0f },
  { 0.0f,                         0.0f,                            0.0f,                           1.0f }
};

constexpr DirectX::XMMATRIX c_from709toXYZ = // Transposed
{
  { 0.4123907983303070068359375f,  0.2126390039920806884765625f,   0.0193308182060718536376953125f, 0.0f },
  { 0.3575843274593353271484375f,  0.715168654918670654296875f,    0.119194783270359039306640625f,  0.0f },
  { 0.18048079311847686767578125f, 0.072192318737506866455078125f, 0.950532138347625732421875f,     0.0f },
  { 0.0f,                          0.0f,                           0.0f,                            1.0f }
};

constexpr DirectX::XMMATRIX c_from709toDCIP3 = // Transposed
{
  { 0.82246196269989013671875f,    0.03319419920444488525390625f, 0.017082631587982177734375f,  0.0f },
  { 0.17753803730010986328125f,    0.96680581569671630859375f,    0.0723974406719207763671875f, 0.0f },
  { 0.0f,                          0.0f,                          0.91051995754241943359375f,   0.0f },
  { 0.0f,                          0.0f,                          0.0f,                         1.0f }
};

constexpr DirectX::XMMATRIX c_from709toAP0 = // Transposed
{
  { 0.4339316189289093017578125f, 0.088618390262126922607421875f, 0.01775003969669342041015625f,  0.0f },
  { 0.3762523829936981201171875f, 0.809275329113006591796875f,    0.109447620809078216552734375f, 0.0f },
  { 0.1898159682750701904296875f, 0.10210628807544708251953125f,  0.872802317142486572265625f,    0.0f },
  { 0.0f,                         0.0f,                           0.0f,                           1.0f }
};

constexpr DirectX::XMMATRIX c_from709toAP1 = // Transposed
{
  { 0.61702883243560791015625f,       0.333867609500885009765625f,    0.04910354316234588623046875f,     0.0f },
  { 0.069922320544719696044921875f,   0.91734969615936279296875f,     0.012727967463433742523193359375f, 0.0f },
  { 0.02054978720843791961669921875f, 0.107552029192447662353515625f, 0.871898174285888671875f,          0.0f },
  { 0.0f,                             0.0f,                           0.0f,                              1.0f }
};

constexpr DirectX::XMMATRIX c_fromXYZto709 = // Transposed
{
  {  3.2409698963165283203125f,    -0.96924364566802978515625f,       0.055630080401897430419921875f, 0.0f },
  { -1.53738319873809814453125f,    1.875967502593994140625f,        -0.2039769589900970458984375f,   0.0f },
  { -0.4986107647418975830078125f,  0.0415550582110881805419921875f,  1.05697154998779296875f,        0.0f },
  {  0.0f,                          0.0f,                             0.0f,                           1.0f }
};

constexpr DirectX::XMMATRIX c_fromXYZtoLMS = // Transposed
{
  {  0.3592f, -0.1922f, 0.0070f, 0.0f },
  {  0.6976f,  1.1004f, 0.0749f, 0.0f },
  { -0.0358f,  0.0755f, 0.8434f, 0.0f },
  {  0.0f,     0.0f,    0.0f,    1.0f }
};

constexpr DirectX::XMMATRIX c_fromLMStoXYZ = // Transposed
{
  {  2.070180056695613509600f,  0.364988250032657479740f, -0.049595542238932107896f, 0.0f },
  { -1.326456876103021025500f,  0.680467362852235141020f, -0.049421161186757487412f, 0.0f },
  {  0.206616006847855170810f, -0.045421753075853231409f,  1.187995941732803439400f, 0.0f },
  {  0.0f,                      0.0f,                      0.0f,                     1.0f }
};

constexpr DirectX::XMMATRIX c_scRGBtoBt2100 = // Transposed
{
  { 2939026994.0f /  585553224375.0f,   76515593.0f / 138420033750.0f,    12225392.0f /   93230009375.0f, 0.0f },
  { 9255011753.0f / 3513319346250.0f, 6109575001.0f / 830520202500.0f,  1772384008.0f / 2517210253125.0f, 0.0f },
  {  173911579.0f /  501902763750.0f,   75493061.0f / 830520202500.0f, 18035212433.0f / 2517210253125.0f, 0.0f },
  {                             0.0f,                            0.0f,                              0.0f, 1.0f }
};

constexpr DirectX::XMMATRIX c_Bt2100toscRGB = // Transposed
{
  {  348196442125.0f / 1677558947.0f, -579752563250.0f / 37238079773.0f,  -12183628000.0f /  5369968309.0f, 0.0f },
  { -123225331250.0f / 1677558947.0f, 5273377093000.0f / 37238079773.0f, -472592308000.0f / 37589778163.0f, 0.0f },
  {  -15276242500.0f / 1677558947.0f,  -38864558125.0f / 37238079773.0f, 5256599974375.0f / 37589778163.0f, 0.0f },
  {                             0.0f,                              0.0f,                              0.0f, 1.0f }
};

static auto LinearToPQ = [](DirectX::XMVECTOR N)
{
  using namespace DirectX;

  XMVECTOR ret;

  ret =
    XMVectorPow (XMVectorDivide (XMVectorMax (N, g_XMZero), PQ.MaxPQ), PQ.N);

  XMVECTOR nd =
    XMVectorDivide (
       XMVectorAdd (  PQ.C1, XMVectorMultiply (PQ.C2, ret)),
       XMVectorAdd (g_XMOne, XMVectorMultiply (PQ.C3, ret))
    );

  return
    XMVectorPow (nd, PQ.M);
};

static auto PQToLinear = [](DirectX::XMVECTOR N)
{
  using namespace DirectX;

  XMVECTOR ret;

  ret =
    XMVectorPow (XMVectorMax (N, g_XMZero), PQ.RcpM);

  XMVECTOR nd;

  nd =
    XMVectorDivide (
      XMVectorMax (XMVectorSubtract (ret, PQ.C1), g_XMZero),
                   XMVectorSubtract (     PQ.C2,
            XMVectorMultiply (PQ.C3, ret)));

  ret =
    XMVectorMultiply (
      XMVectorPow (nd, PQ.RcpN), PQ.MaxPQ
    );

  return ret;
};