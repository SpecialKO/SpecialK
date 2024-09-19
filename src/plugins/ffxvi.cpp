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
#include <imgui/font_awesome.h>

static float SK_FFXVI_JXLQuality    = 99.95f;
static int   SK_FFXVI_JXLMaxThreads = 5;

static bool SK_FFXVI_UncapCutscenes     = true;
static bool SK_FFXVI_FramegenCutscenes  = true;
static bool SK_FFXVI_AllowGraphicsDebug = true;
static bool SK_FFXVI_ActiveAntiStutter  = true;

static INT64 SK_FFXVI_CutsceneFPSAddr       = 0x0;
static INT64 SK_FFXVI_CutsceneFGAddr        = 0x0;
static INT64 SK_FFXVI_AntiGraphicsDebugAddr = 0x0;

struct {
  sk::ParameterFloat* jxl_quality        = nullptr;
  sk::ParameterInt*   jxl_max_threads    = nullptr;
  sk::ParameterBool*  uncap_cutscene_fps = nullptr;
  sk::ParameterBool*  allow_cutscene_fg  = nullptr;
  sk::ParameterBool*  allow_gr_debug     = nullptr;
  sk::ParameterInt64* cutscene_fps_addr  = nullptr;
  sk::ParameterInt64* cutscene_fg_addr   = nullptr;
  sk::ParameterInt64* anti_gr_debug_addr = nullptr;
  sk::ParameterBool*  active_antistutter = nullptr;
} static ini;

struct patch_byte_s {
  void*   address  = nullptr;
  uint8_t original = 0;
  uint8_t override = 0;
  bool enable (void)
  {
    if (address)
    {    
      DWORD dwOrigProtection = 0x0;
      if (VirtualProtect (address, 1, PAGE_EXECUTE_READWRITE, &dwOrigProtection))
      {
        *(uint8_t *)address = override;
        VirtualProtect   (address, 1, PAGE_EXECUTE_READWRITE, &dwOrigProtection);
        return true;
      }
    }

    return false;
  }
  bool disable (void)
  {
    if (address)
    {    
      DWORD dwOrigProtection = 0x0;
      if (VirtualProtect (address, 1, PAGE_EXECUTE_READWRITE, &dwOrigProtection))
      {
        *(uint8_t *)address = original;
        VirtualProtect   (address, 1, PAGE_EXECUTE_READWRITE, &dwOrigProtection);
        return true;
      }
    }

    return false;
  }
} cutscene_unlock,
  cutscene_fg;

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

  SK_Thread_Create ([](LPVOID)->DWORD
  {
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

    ini.uncap_cutscene_fps =
      _CreateConfigParameterBool ( L"FFXVI.PlugIn",
                                   L"UncapCutsceneFPS", SK_FFXVI_UncapCutscenes,
                                   L"Remove 30 FPS Cutscene Limit" );

    if (! ini.uncap_cutscene_fps->load  (SK_FFXVI_UncapCutscenes))
          ini.uncap_cutscene_fps->store (SK_FFXVI_UncapCutscenes);

    ini.allow_cutscene_fg =
      _CreateConfigParameterBool ( L"FFXVI.PlugIn",
                                   L"AllowCutsceneFG", SK_FFXVI_FramegenCutscenes,
                                   L"Allow Frame Generation in Cutscenes" );

    if (! ini.allow_cutscene_fg->load  (SK_FFXVI_FramegenCutscenes))
          ini.allow_cutscene_fg->store (SK_FFXVI_FramegenCutscenes);

    ini.active_antistutter =
      _CreateConfigParameterBool ( L"FFXVI.PlugIn",
                                   L"ActiveAntiStutter", SK_FFXVI_ActiveAntiStutter,
                                   L"Decrease CPU Idle Time For Better Frametime Consistency" );

    if (! ini.active_antistutter->load  (SK_FFXVI_ActiveAntiStutter))
          ini.active_antistutter->store (SK_FFXVI_ActiveAntiStutter);

    if (SK_FFXVI_ActiveAntiStutter)
    {
      config.render.framerate.max_delta_time   = 3;
      config.render.framerate.sleepless_render = true;
      config.render.framerate.sleepless_window = false;
    }

    ini.cutscene_fps_addr =
      _CreateConfigParameterInt64 ( L"FFXVI.PlugIn",
                                    L"CutsceneFPSAddr", SK_FFXVI_CutsceneFPSAddr,
                                    L"Cache Last Known Address" );

    ini.cutscene_fg_addr =
      _CreateConfigParameterInt64 ( L"FFXVI.PlugIn",
                                    L"CutsceneFGAddr", SK_FFXVI_CutsceneFGAddr,
                                    L"Cache Last Known Address" );

    ini.anti_gr_debug_addr =
      _CreateConfigParameterInt64 ( L"FFXVI.PlugIn",
                                    L"AntiGraphicsDebugAddr", SK_FFXVI_AntiGraphicsDebugAddr,
                                    L"Cache Last Known Address" );

    void *limit_addr = (void *)SK_FFXVI_CutsceneFPSAddr;

    if (SK_FFXVI_CutsceneFPSAddr != 0)
    {
      DWORD                                                                               dwOrigProt;
      if (! VirtualProtect ((void *)SK_FFXVI_CutsceneFPSAddr, 3, PAGE_EXECUTE_READWRITE, &dwOrigProt))
        SK_FFXVI_CutsceneFPSAddr = 0;
    }

    if (                     limit_addr == nullptr || 
        !SK_ValidatePointer (limit_addr, true)     ||
               (* (uint8_t *)limit_addr    != 0x75 ||
                *((uint8_t *)limit_addr+2) != 0x85))
    {
      limit_addr =                              //\x01, but may have been patched, so ignore
        SK_Scan ("\x75\x00\x85\x00\x74\x00\x40\x00\x00\x41\x00\x00\x00\x00\x00\x00\x00", 17,
                 "\xff\x00\xff\x00\xff\x00\xff\x00\x00\xff\x00\x00\x00\x00\x00\x00\x00");

      if (limit_addr != nullptr)
        ini.cutscene_fps_addr->store ((int64_t)limit_addr);
    }

    else SK_LOGi0 (L"Skipped memory address scan for FPS Limit Branch");

    if (limit_addr != nullptr)
    {
      cutscene_unlock.address  = ((uint8_t *)limit_addr)+0x8;
      cutscene_unlock.original = 1;
      cutscene_unlock.override = 0;

      if (SK_FFXVI_UncapCutscenes)
        cutscene_unlock.enable ();
    }

    void *fg_enablement_addr = (void *)SK_FFXVI_CutsceneFGAddr;

    if (SK_FFXVI_CutsceneFGAddr != 0)
    {
      DWORD                                                                              dwOrigProt;
      if (! VirtualProtect ((void *)SK_FFXVI_CutsceneFGAddr, 6, PAGE_EXECUTE_READWRITE, &dwOrigProt))
        SK_FFXVI_CutsceneFGAddr = 0;
    }

    if (                     fg_enablement_addr == nullptr ||
        !SK_ValidatePointer (fg_enablement_addr, true)     ||
               (* (uint8_t *)fg_enablement_addr    != 0x41 ||
                *((uint8_t *)fg_enablement_addr+5) != 0x33))
    {
      fg_enablement_addr =  //\x74, but this may have been patched by other software, so count it as dontcare
        SK_Scan ("\x41\x00\x00\x00\x00\x33\x00\x48\x00\x00\xE8\x00\x00\x00\x00\x8B\x00\x00\x00\x00\x00\xD1\x00", 23,
                 "\xff\x00\x00\x00\x00\xff\x00\xff\x00\x00\xff\x00\x00\x00\x00\xff\x00\x00\x00\x00\x00\xff\x00");

      if (fg_enablement_addr != nullptr)
        ini.cutscene_fg_addr->store ((int64_t)fg_enablement_addr);
    }

    else SK_LOGi0 (L"Skipped memory address scan for Cutscene Frame Generation Branch");

    if (fg_enablement_addr != nullptr)
    {
      cutscene_fg.address  = ((uint8_t *)fg_enablement_addr)+0x3;
      cutscene_fg.original = 0x74;
      cutscene_fg.override = 0xEB;

      if (SK_FFXVI_FramegenCutscenes)
        cutscene_fg.enable ();
    }

    void *anti_gr_debug_addr =
      (void *)SK_FFXVI_AntiGraphicsDebugAddr;

    if (                     anti_gr_debug_addr == nullptr ||
        !SK_ValidatePointer (anti_gr_debug_addr, true)     ||
               (*((uint8_t *)anti_gr_debug_addr+1) != 0x85 ||
                *((uint8_t *)anti_gr_debug_addr+2) != 0xc0 ||
                *((uint8_t *)anti_gr_debug_addr+3) != 0x0f))
    {
      anti_gr_debug_addr =
        SK_Scan ("\x00\x85\xc0\x0f\x85\x00\x00\x00\x00\x48\x8b\x0d\x00\x00\x00\x00\x48\x85\xc9\x00\x0d\xe8\x00\x00\x00\x00\x84\xc0", 28,
                 "\x00\x85\xc0\x0f\x85\x00\x00\x00\x00\x48\x8b\x0d\x00\x00\x00\x00\x48\x85\xc9\x00\x0d\xe8\x00\x00\x00\x00\x84\xc0");

      if (anti_gr_debug_addr != nullptr)
        ini.anti_gr_debug_addr->store ((int64_t)anti_gr_debug_addr);
    }

    else SK_LOGi0 (L"Skipped memory address scan for Graphics Debugger Branch");

    if (anti_gr_debug_addr != 0x0 && SK_FFXVI_AllowGraphicsDebug)
    {
      DWORD dwOrigProt = 0x0;

      uint16_t* branch_addr =
        (uint16_t *)((uintptr_t)anti_gr_debug_addr + 0x13);

      if (*branch_addr == 0x0d74) // je
      {
        if (VirtualProtect (branch_addr, 2, PAGE_EXECUTE_READWRITE, &dwOrigProt))
        {
          *branch_addr  = 0x0deb; // jmp

          VirtualProtect (branch_addr, 2, dwOrigProt, &dwOrigProt);

          SK_LOGi0 (L"Patched out graphics debugger check");
        }
      }
    }

    config.utility.save_async ();

    SK_Thread_CloseSelf ();

    return 0;
  });

  return S_OK;
}

bool
SK_FFXVI_PlugInCfg (void)
{
  auto dll_ini =
    SK_GetDLLConfig ();

  std::ignore = dll_ini;

  if (ImGui::CollapsingHeader ("Final Fantasy XVI", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.40f, 0.40f, 0.45f));
    ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.45f, 0.45f, 0.80f));
    ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.53f, 0.53f, 0.80f));
    ImGui::TreePush       ("");

    if (ImGui::CollapsingHeader (ICON_FA_IMAGE "\tScreenshots", 0x0))
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

      ImGui::TreePop ();
    }

    if (ImGui::CollapsingHeader (ICON_FA_TACHOMETER_ALT "\tPerformance", ImGuiTreeNodeFlags_DefaultOpen))
    {
      static bool restart_warning = false;

      ImGui::TreePush ("");

      if (cutscene_unlock.address != nullptr && ImGui::Checkbox ("Uncap Cutscene FPS", &SK_FFXVI_UncapCutscenes))
      {
        restart_warning = true;

        ini.uncap_cutscene_fps->store (SK_FFXVI_UncapCutscenes);

        if (SK_FFXVI_UncapCutscenes)
          cutscene_unlock.enable ();
        else
          cutscene_unlock.disable ();

        config.utility.save_async ();
      }

      if (SK_FFXVI_UncapCutscenes && cutscene_fg.address != nullptr)
      {
        ImGui::SameLine ();

        if (ImGui::Checkbox ("Allow Frame Generation in Cutscenes", &SK_FFXVI_FramegenCutscenes))
        {
          restart_warning = true;

          ini.allow_cutscene_fg->store (SK_FFXVI_FramegenCutscenes);

          if (SK_FFXVI_FramegenCutscenes)
            cutscene_fg.enable ();
          else
            cutscene_fg.disable ();

          config.utility.save_async ();
        }
      }

      ImGui::BeginGroup ();

      if (SK_FFXVI_AntiGraphicsDebugAddr != 0)
      {
        if (ImGui::Checkbox ("Disable Graphics Debugger Checks", &SK_FFXVI_AllowGraphicsDebug))
        {
          restart_warning = true;

          ini.allow_gr_debug->store (SK_FFXVI_AllowGraphicsDebug);

          config.utility.save_async ();
        }

        if (ImGui::IsItemHovered ())
        {
          ImGui::SetTooltip (
            "The game checks for graphics debuggers every frame, "
            "which has performance penalties when running on SteamOS/Wine."
          );
        }
      }

      if (ImGui::Checkbox ("Optimize Fullscreen Mode", &config.render.dxgi.fake_fullscreen_mode))
      {
        config.utility.save_async ();
      }

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip    ();
        ImGui::TextUnformatted ("Use Borderless Fullscreen instead of D3D12's Fake Fullscreen");
        ImGui::Separator       ();
        ImGui::BulletText      ("The game cannot use HDR unless it thinks it is in Fullscreen mode.");
        ImGui::BulletText      ("This option prevents running the game at a different resolution than your desktop.");
        ImGui::EndTooltip      ();
      }

      //if (__SK_IsDLSSGActive)
      //  ImGui::Checkbox ("Manually Calculate DLSS DeltaTimeMs", &config.nvidia.dlss.calculate_delta_ms);

      ImGui::EndGroup    ();
      ImGui::SameLine    ();
      ImGui::SeparatorEx (ImGuiSeparatorFlags_Vertical);
      ImGui::SameLine    ();
      ImGui::BeginGroup  ();

      if (ImGui::SliderInt (
            "DirectStorage Work Submit",
              &config.render.dstorage.submit_threads,   -1,
            2/*config.priority.available_cpu_cores*/,
               config.render.dstorage.submit_threads == -1 ?
                               "Default Number of Threads" : "%d Threads"))
      {
        if (config.render.dstorage.submit_threads == 0)
            config.render.dstorage.submit_threads = -1;

        restart_warning = true;
        config.utility.save_async ();
      }

      if (ImGui::IsItemHovered ())
      {
        ImGui::SetTooltip (
          "May improve throughput and shorter loads / less stutter, but "
          "may also cause instability."
        );
      }

      if (ImGui::SliderInt (
            "DirectStorage CPU Decompression",
              &config.render.dstorage.cpu_decomp_threads,   -1,
               config.priority.available_cpu_cores,
               config.render.dstorage.cpu_decomp_threads == -1 ?
                                   "Default Number of Threads" : "%d Threads"))
      {
        if (config.render.dstorage.cpu_decomp_threads == 0)
            config.render.dstorage.cpu_decomp_threads = -1;

        restart_warning = true;
        config.utility.save_async ();
      }

      if (ImGui::IsItemHovered ())
      {
        ImGui::SetTooltip (
          "May improve performance if GPU Decompression is forcefully disabled."
        );
      }

      ImGui::EndGroup ();

      if (restart_warning)
      {
        ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (.3f, .8f, .9f).Value);
        ImGui::BulletText     ("Game Restart May Be Required");
        ImGui::PopStyleColor  ();
      }

      if (ImGui::Checkbox ("Aggressive Anti-Stutter", &SK_FFXVI_ActiveAntiStutter))
      {
        if (SK_FFXVI_ActiveAntiStutter)
        {
          config.render.framerate.max_delta_time   = 3;
          config.render.framerate.sleepless_render = true;
          config.render.framerate.sleepless_window = false;
        }

        else
        {
          config.render.framerate.max_delta_time   = 0;
          config.render.framerate.sleepless_render = false;
          config.render.framerate.sleepless_window = false;
        }

        ini.active_antistutter->store (SK_FFXVI_ActiveAntiStutter);

        config.utility.save_async ();
      }

      if (ImGui::IsItemHovered ())
      {
        ImGui::SetTooltip (
          "Increases overall CPU load on threads that are poorly synchronized; "
          "these threads sleep for short intervals instead of being signaled "
          "when they have actual work to do..."
        );
      }

      ImGui::TreePop ();
    }

    ImGui::PopStyleColor (3);
    ImGui::TreePop       ( );

    return true;
  }

  return false;
}

void
SK_FFXVI_InitPlugin (void)
{
  // Game frequently crashes at shutdown
  config.system.silent_crash = true;

  // Avoid Steam Offline Warnings
  config.platform.achievements.pull_friend_stats = false;

  // We must subclass the window, not hook it, due to the splashscreen
  config.window.dont_hook_wndproc = true;

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

  ini.allow_gr_debug =
    _CreateConfigParameterBool ( L"FFXVI.PlugIn",
                                 L"AllowGraphicsDebuggers", SK_FFXVI_AllowGraphicsDebug,
                                 L"Bypass Graphics Debugger Checks" );

  if (! ini.allow_gr_debug->load  (SK_FFXVI_AllowGraphicsDebug))
        ini.allow_gr_debug->store (SK_FFXVI_AllowGraphicsDebug);
}