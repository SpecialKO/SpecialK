//
// Copyright 2024 Andon "Kaldaien" Coleman
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//

#include <SpecialK/stdafx.h>

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"FFXVI Plug"

#include <SpecialK/plugin/plugin_mgr.h>

static float SK_FFXVI_JXLQuality    = 99.95f;
static int   SK_FFXVI_JXLMaxThreads = 5;

typedef enum {
  JXL_ENC_SUCCESS          = 0,
  JXL_ENC_ERROR            = 1,
  JXL_ENC_NEED_MORE_OUTPUT = 2,
} JxlEncoderStatus;

typedef enum {
  JXL_NATIVE_ENDIAN = 0,
  JXL_LITTLE_ENDIAN = 1,
  JXL_BIG_ENDIAN    = 2,
} JxlEndianness;

typedef enum {
  JXL_TYPE_FLOAT   = 0,
  JXL_TYPE_UINT8   = 2,
  JXL_TYPE_UINT16  = 3,
  JXL_TYPE_FLOAT16 = 5,
} JxlDataType;

typedef struct JxlEncoderFrameSettingsStruct JxlEncoderFrameSettings;
typedef struct JxlEncoderStruct              JxlEncoder;

typedef struct {
  uint32_t      num_channels;
  JxlDataType   data_type;
  JxlEndianness endianness;
  size_t        align;
} JxlPixelFormat;

#define JXL_BOOL  int
#define JXL_TRUE  1
#define JXL_FALSE 0

typedef enum {
  JXL_BIT_DEPTH_FROM_PIXEL_FORMAT = 0,
  JXL_BIT_DEPTH_FROM_CODESTREAM   = 1,
  JXL_BIT_DEPTH_CUSTOM            = 2,
} JxlBitDepthType;

typedef struct {
  JxlBitDepthType type;
  uint32_t        bits_per_sample;
  uint32_t        exponent_bits_per_sample;
} JxlBitDepth;

typedef enum {
  JXL_ORIENT_IDENTITY        = 1,
  JXL_ORIENT_FLIP_HORIZONTAL = 2,
  JXL_ORIENT_ROTATE_180      = 3,
  JXL_ORIENT_FLIP_VERTICAL   = 4,
  JXL_ORIENT_TRANSPOSE       = 5,
  JXL_ORIENT_ROTATE_90_CW    = 6,
  JXL_ORIENT_ANTI_TRANSPOSE  = 7,
  JXL_ORIENT_ROTATE_90_CCW   = 8,
} JxlOrientation;

typedef struct {
  uint32_t xsize;
  uint32_t ysize;
} JxlPreviewHeader;

typedef struct {
  uint32_t tps_numerator;
  uint32_t tps_denominator;
  uint32_t num_loops;
  JXL_BOOL have_timecodes;
} JxlAnimationHeader;

typedef struct {
  JXL_BOOL           have_container;
  uint32_t           xsize;
  uint32_t           ysize;
  uint32_t           bits_per_sample;
  uint32_t           exponent_bits_per_sample;
  float              intensity_target;
  float              min_nits;
  JXL_BOOL           relative_to_max_display;
  float              linear_below;
  JXL_BOOL           uses_original_profile;
  JXL_BOOL           have_preview;
  JXL_BOOL           have_animation;
  JxlOrientation     orientation;
  uint32_t           num_color_channels;
  uint32_t           num_extra_channels;
  uint32_t           alpha_bits;
  uint32_t           alpha_exponent_bits;
  JXL_BOOL           alpha_premultiplied;
  JxlPreviewHeader   preview;
  JxlAnimationHeader animation;
  uint32_t           intrinsic_xsize;
  uint32_t           intrinsic_ysize;
  uint8_t            padding [100];
} JxlBasicInfo;

using JxlEncoderSetBasicInfo_pfn  = JxlEncoderStatus (*)(JxlEncoder* enc, const JxlBasicInfo* info);
      JxlEncoderSetBasicInfo_pfn
      JxlEncoderSetBasicInfo_Original = nullptr;

using JxlEncoderAddImageFrame_pfn = JxlEncoderStatus (*)(
  JxlEncoderFrameSettings *frame_settings, const JxlPixelFormat *pixel_format,
               const void *buffer,               size_t          size);
      JxlEncoderAddImageFrame_pfn
      JxlEncoderAddImageFrame_Original = nullptr;

using JxlThreadParallelRunnerDefaultNumWorkerThreads_pfn = size_t (*)(void);
      JxlThreadParallelRunnerDefaultNumWorkerThreads_pfn
      JxlThreadParallelRunnerDefaultNumWorkerThreads_Original = nullptr;

// Reduce the number of threads libjxl uses to prevent hitching when using
//   the game's screenshot feature...
size_t
JxlThreadParallelRunnerDefaultNumWorkerThreads_Detour (void)
{
  SK_LOG_FIRST_CALL

  size_t real_default =
    JxlThreadParallelRunnerDefaultNumWorkerThreads_Original ();

  SK_RunOnce (
    SK_LOGi0 (
      L"JxlThreadParallelRunnerDefaultNumWorkerThreads: %d", real_default));

  return
    std::min (real_default, static_cast <size_t> (SK_FFXVI_JXLMaxThreads));
}

JxlEncoderStatus
JxlEncoderSetBasicInfo_Detour (JxlEncoder* enc, const JxlBasicInfo* info)
{
  SK_LOG_FIRST_CALL

  if (info != nullptr)
  {
    JxlBasicInfo info_copy = *info;

    SK_ComQIPtr <IDXGISwapChain> pSwapChain (
    SK_GetCurrentRenderBackend ().swapchain );

    DXGI_SWAP_CHAIN_DESC  swapDesc = {};
    pSwapChain->GetDesc (&swapDesc);

    info_copy.bits_per_sample          = 16;
    info_copy.exponent_bits_per_sample = 5;
    info_copy.alpha_bits               = 0;

    return
      JxlEncoderSetBasicInfo_Original (enc, &info_copy);
  }

  return
      JxlEncoderSetBasicInfo_Original (enc, info);
}

JxlEncoderStatus
JxlEncoderAddImageFrame_Detour (       JxlEncoderFrameSettings *frame_settings,
                                 const JxlPixelFormat          *pixel_format,
                                 const void                    *buffer,
                                       size_t                   size )
{
  SK_LOG_FIRST_CALL

  using  JxlEncoderSetFrameLossless_pfn = JxlEncoderStatus (*)(JxlEncoderFrameSettings *frame_settings, JXL_BOOL lossless);
  static JxlEncoderSetFrameLossless_pfn
         JxlEncoderSetFrameLossless =
        (JxlEncoderSetFrameLossless_pfn)GetProcAddress (LoadLibraryW (L"jxl.dll"), "JxlEncoderSetFrameLossless");

  using  JxlEncoderSetFrameDistance_pfn = JxlEncoderStatus (*)(JxlEncoderFrameSettings* frame_settings, float distance);
  static JxlEncoderSetFrameDistance_pfn
         JxlEncoderSetFrameDistance =
        (JxlEncoderSetFrameDistance_pfn)GetProcAddress (LoadLibraryW (L"jxl.dll"), "JxlEncoderSetFrameDistance");

  using  JxlEncoderSetFrameBitDepth_pfn = JxlEncoderStatus (*)(JxlEncoderFrameSettings *frame_settings, const JxlBitDepth *bit_depth);
  static JxlEncoderSetFrameBitDepth_pfn
         JxlEncoderSetFrameBitDepth =
        (JxlEncoderSetFrameBitDepth_pfn)GetProcAddress (LoadLibraryW (L"jxl.dll"), "JxlEncoderSetFrameBitDepth");

  using  JxlEncoderDistanceFromQuality_pfn = float (*)(float quality);
  static JxlEncoderDistanceFromQuality_pfn
         JxlEncoderDistanceFromQuality =
        (JxlEncoderDistanceFromQuality_pfn)GetProcAddress (LoadLibraryW (L"jxl.dll"), "JxlEncoderDistanceFromQuality");

  if (SK_FFXVI_JXLQuality == 100.0f)
  {
    if (JxlEncoderSetFrameLossless != nullptr)
        JxlEncoderSetFrameLossless (frame_settings, JXL_TRUE);
  }


  if (JxlEncoderSetFrameDistance != nullptr && JxlEncoderDistanceFromQuality != nullptr)
      JxlEncoderSetFrameDistance (frame_settings, JxlEncoderDistanceFromQuality (SK_FFXVI_JXLQuality));

  JxlBitDepth jbd = { };
  
  jbd.type                     = JXL_BIT_DEPTH_FROM_PIXEL_FORMAT;
  jbd.bits_per_sample          = 16;
  jbd.exponent_bits_per_sample = 5;
  
  if (JxlEncoderSetFrameBitDepth != nullptr)
      JxlEncoderSetFrameBitDepth (frame_settings, &jbd);

  //SK_LOGi0 (L"AddImageFrame (...): PixelFormat->DataType=%d", pixel_format->data_type);

  return
    JxlEncoderAddImageFrame_Original (frame_settings, pixel_format, buffer, size);
}

HRESULT
STDMETHODCALLTYPE
SK_FFXVI_PresentFirstFrame (IUnknown* pSwapChain, UINT SyncInterval, UINT Flags)
{
  UNREFERENCED_PARAMETER (pSwapChain);
  UNREFERENCED_PARAMETER (SyncInterval);
  UNREFERENCED_PARAMETER (Flags);

  SK_CreateDLLHook2 (      L"jxl.dll",
                            "JxlEncoderAddImageFrame",
                             JxlEncoderAddImageFrame_Detour,
    static_cast_p2p <void> (&JxlEncoderAddImageFrame_Original) );

  SK_CreateDLLHook2 (      L"jxl.dll",
                            "JxlEncoderSetBasicInfo",
                             JxlEncoderSetBasicInfo_Detour,
    static_cast_p2p <void> (&JxlEncoderSetBasicInfo_Original) );

  SK_CreateDLLHook2 (      L"jxl_threads.dll",
                            "JxlThreadParallelRunnerDefaultNumWorkerThreads",
                             JxlThreadParallelRunnerDefaultNumWorkerThreads_Detour,
    static_cast_p2p <void> (&JxlThreadParallelRunnerDefaultNumWorkerThreads_Original) );

  SK_ApplyQueuedHooks ();

  return S_OK;
}

struct {
  sk::ParameterFloat* jxl_quality     = nullptr;
  sk::ParameterInt*   jxl_max_threads = nullptr;
} static ini;

bool
SK_FFXVI_PlugInCfg (void)
{
  auto dll_ini =
    SK_GetDLLConfig ();

  std::ignore = dll_ini;

  if (ImGui::CollapsingHeader ("Final Fantasy XVI", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::TreePush ("");

    bool bChanged = 
      ImGui::SliderFloat ( "JPEG XL Screenshot Quality", &SK_FFXVI_JXLQuality, 96.0f, 100.0f,
                                                          SK_FFXVI_JXLQuality != 100.0f ?
                           "%4.1f%%" : "Lossless" );

    bChanged |=
      ImGui::SliderInt ( "Maximum JPEG XL Threads", &SK_FFXVI_JXLMaxThreads,
                           2, config.priority.available_cpu_cores );

    if (ImGui::IsItemHovered ())
    {
      ImGui::BeginTooltip    ();
      ImGui::TextUnformatted ("Reduce thread count to eliminate stutter in photo mode.");
      ImGui::Separator       ();
      ImGui::BulletText      ("Default Max Threads (too many): %d",
                                JxlThreadParallelRunnerDefaultNumWorkerThreads_Original ());
      ImGui::EndTooltip      ();
    }

    if (bChanged)
    {
      ini.jxl_quality->store     (SK_FFXVI_JXLQuality);
      ini.jxl_max_threads->store (SK_FFXVI_JXLMaxThreads);

      config.utility.save_async ();
    }

    ImGui::TreePop  (  );

    return true;
  }

  return false;
}

void
SK_FFXVI_InitPlugin (void)
{
  // Game always crashes at shutdown
  config.system.silent_crash = true;

  SK_FFXVI_JXLMaxThreads =
    config.screenshots.avif.max_threads;

  plugin_mgr->first_frame_fns.emplace (SK_FFXVI_PresentFirstFrame);
  plugin_mgr->config_fns.emplace      (SK_FFXVI_PlugInCfg);

  ini.jxl_quality =
    _CreateConfigParameterFloat ( L"FFXVI.PlugIn",
                                  L"JXLQualityLevel", SK_FFXVI_JXLQuality,
                                  L"Traditional JPEG Quality %" );

  if (! ini.jxl_quality->load  (SK_FFXVI_JXLQuality))
        ini.jxl_quality->store (SK_FFXVI_JXLQuality);

  ini.jxl_max_threads =
    _CreateConfigParameterInt ( L"FFXVI.PlugIn",
                                L"JXLMaxThreads", SK_FFXVI_JXLMaxThreads,
                                  L"Maximum Worker Threads" );

  if (! ini.jxl_max_threads->load  (SK_FFXVI_JXLMaxThreads))
        ini.jxl_max_threads->store (SK_FFXVI_JXLMaxThreads);

  SK_FFXVI_JXLMaxThreads =
    std::min ( static_cast <DWORD> (SK_FFXVI_JXLMaxThreads),
                      config.priority.available_cpu_cores );

#if 1
  uint16_t* pAntiDebugBranch =
    (uint16_t *)((uintptr_t)(SK_Debug_GetImageBaseAddr ()) + 0x957223);

  DWORD                                                          dwOrigProtection = 0x0;
  VirtualProtect (pAntiDebugBranch, 2, PAGE_EXECUTE_READWRITE, &dwOrigProtection);
  if (*pAntiDebugBranch == 0x0d74)
      *pAntiDebugBranch  = 0x0deb;
  VirtualProtect (pAntiDebugBranch, 2, dwOrigProtection,       &dwOrigProtection);
#else
  uint8_t* pAntiDebugStart = (uint8_t *)0x1409C37DD;
  uint8_t* pAntiDebugEnd   = (uint8_t *)0x1409C383B;

  size_t size = 
    (uintptr_t)pAntiDebugEnd - (uintptr_t)pAntiDebugStart;

  DWORD                                                           dwOrigProtection = 0x0;
  VirtualProtect (pAntiDebugStart, size, PAGE_EXECUTE_READWRITE, &dwOrigProtection);
  for ( UINT i = 0 ; i < size ; ++i )
  {
    *(pAntiDebugStart + i) = 0x90;
  }
  VirtualProtect (pAntiDebugStart, size, dwOrigProtection,       &dwOrigProtection);
#endif
}