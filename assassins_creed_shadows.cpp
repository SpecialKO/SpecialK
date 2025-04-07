// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
//
// Copyright 2025 Andon "Kaldaien" Coleman
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

#include <concurrent_unordered_set.h>
#include <safetyhook/safetyhook.hpp>
#include <SpecialK/hooks.h>
#include <SpecialK/utility.h>
#include <SpecialK/log.h>
#include <SpecialK/nvapi.h>
#include <SpecialK/plugin/plugin_mgr.h>
#include <SpecialK/control_panel.h>
#include <SpecialK/render/ngx/streamline.h>
#include <imgui/imgui.h>

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"AC Shadows"

bool               __SK_ACS_IsMultiFrameCapable   = false;
bool               __SK_ACS_AlwaysUseFrameGen     =  true;
bool               __SK_ACS_ShowFMVIndicator      = false;
bool               __SK_ACS_UncapFramerate        =  true;
int                __SK_ACS_DLSSG_MultiFrameCount =     1;
bool               __SK_ACS_RemoveBlackBars       =  true;
bool               __SK_ACS_ExpandFOVRange        =  true;

sk::ParameterBool*  _SK_ACS_AlwaysUseFrameGen;
sk::ParameterBool*  _SK_ACS_ShowFMVIndicator;
sk::ParameterBool*  _SK_ACS_UncapFramerate;
sk::ParameterBool*  _SK_ACS_DynamicCloth;
sk::ParameterBool*  _SK_ACS_RemoveBlackBars;
sk::ParameterBool*  _SK_ACS_ExpandFOVRange;
sk::ParameterInt*   _SK_ACS_DLSSG_MultiFrameCount;

using slGetPluginFunction_pfn = void*      (*)(const char* functionName);
using slDLSSGSetOptions_pfn   = sl::Result (*)(const sl::ViewportHandle& viewport, sl::DLSSGOptions& options);
      slDLSSGSetOptions_pfn
      slDLSSGSetOptions_ACS_Original = nullptr;

static          DWORD  LastTimeFMVChecked     = 0;
static          HANDLE LastFMVHandle          = 0;
static volatile ULONG  FrameGenDisabledForFMV = FALSE;
static bool*          pFrameGenEnabled        = nullptr;

uintptr_t      __SK_ACS_BlackBarsAddr       =       0;
uintptr_t      __SK_ACS_ClothSimAddr        =       0;
uintptr_t      __SK_ACS_FOVSliderAddr       =       0;
uintptr_t      __SK_ACS_FOVMultiplierAddr   =       0;
bool           __SK_ACS_DynamicCloth        =    true;

void
SK_ACS_ApplyClothPhysicsFix (bool enable)
{
  __SK_ACS_DynamicCloth = enable;

  if (__SK_ACS_ClothSimAddr != 0)
  {
    // From ACShadowsFix (https://github.com/Lyall/ACShadowsFix/blob/5b53a3c84fa8dee39a1ed49b60995b334a119d97/src/dllmain.cpp#L246C1-L256C20)
    static SafetyHookMid ClothPhysicsMidHook =
      safetyhook::create_mid (__SK_ACS_ClothSimAddr,
        [](SafetyHookContext& ctx)
        {
          if (! ctx.rcx)
            return;

          // By default the game appears to use 60fps cloth physics during gameplay and 30fps cloth physics during cutscenes.

          if (__SK_ACS_DynamicCloth)
          {
            // Use Special K's target framerate and lock to that, this keeps
            //   a fixed timestep, but uses SK's framerate limit as the timestep.
            if (__target_fps > 0.0f)
            {
              ctx.xmm0.f32 [0] =
                (1.0f / (__target_fps * (pFrameGenEnabled != nullptr &&
                                        *pFrameGenEnabled ? 0.5f : 1.0f)));
            }

            else
            {
              // Use current frametime for cloth physics instead of fixed 0.01666/0.03333 values.
              if (uintptr_t pFramerate = *reinterpret_cast <uintptr_t *>(ctx.rcx + 0x70))
                ctx.xmm0.f32 [0] = *reinterpret_cast<float *>(pFramerate + 0x78); // Current frametime
            }
          }
        }
      );
  }
}

void
SK_ACS_ApplyBlackBarRemoval (bool remove)
{
  if (__SK_ACS_BlackBarsAddr != 0)
  {
    DWORD                                                                             dwOrigProt = 0x0;
    if (VirtualProtect ((void *) __SK_ACS_BlackBarsAddr,  4, PAGE_EXECUTE_READWRITE, &dwOrigProt))
    { memcpy           ((void *)(__SK_ACS_BlackBarsAddr + 2), (unsigned char *)(remove ? "\xEB"
                                                                                       : "\x74"), 1);
      VirtualProtect   ((void *) __SK_ACS_BlackBarsAddr,  4, dwOrigProt,             &dwOrigProt);
    }
  }
}

void
SK_ACS_ApplyExpandedFOV (bool enable)
{
  __SK_ACS_ExpandFOVRange = enable;

  // From ACShadowsFix (https://github.com/Lyall/ACShadowsFix/)

  if ( __SK_ACS_FOVSliderAddr     != 0 &&
       __SK_ACS_FOVMultiplierAddr != 0 )
  {
    static SafetyHookMid FOVSliderPercentageMidHook =
      safetyhook::create_mid (__SK_ACS_FOVSliderAddr + 0x8,
        [](SafetyHookContext& ctx)
        {
          if (! ctx.rdi)
            return;

          if (! __SK_ACS_ExpandFOVRange)
            return;

          // Get slider min/max
          float* fMin = reinterpret_cast <float *> (ctx.rdi + 0x38);
          float* fMax = reinterpret_cast <float *> (ctx.rdi + 0x3C);
          
          // FOV slider is 85-115
          if (*fMin == 85.00f && *fMax == 115.00f)
          {
            // Widen to 70-150
            *fMin = 70.00f;
            *fMax = 150.00f;
          }
        }
      );

    static SafetyHookMid FOVSliderMultiplierMidHook =
      safetyhook::create_mid (__SK_ACS_FOVMultiplierAddr + 0x9,
        [](SafetyHookContext& ctx)
        {
          if (! ctx.rcx)
            return; // RCX is loaded from RSI in the previous instruction

          if (! __SK_ACS_ExpandFOVRange)
            return;

          // Widen the FOV multiplier range to match
          const char*  currentSetting = *reinterpret_cast <char **> (ctx.rsi + 0x18);
          if (strncmp (currentSetting, "FieldOfViewMultiplier", sizeof ("FieldOfViewMultiplier") - 1) == 0)
          {
            *reinterpret_cast <float *> (ctx.rcx + 0x8) = 0.70f;
            *reinterpret_cast <float *> (ctx.rcx + 0xC) = 1.50f;
          }
        }
      );
  }
}

bool
SK_ACS_ApplyFrameGenOverride (bool enable)
{
  static uintptr_t base_addr =
    (uintptr_t)SK_Debug_GetImageBaseAddr ();

  static void* FrameGenTestAddr = nullptr;

  SK_RunOnce (
    /*
    ACShadows.exe+346B5B4 - C1 E0 03              - shl eax,03 { 3 }
    ACShadows.exe+346B5B7 - 48 8B 15 0A2EC407     - mov rdx,[ACShadows.exe+B0AE3C8] { (0F507BC0) }  <---   RIP + 0x07c42e0a
    ACShadows.exe+346B5BE - 80 7A 24 00           - cmp byte ptr [rdx+24],00 { 0 }
    ACShadows.exe+346B5C2 - 48 8B 04 01           - mov rax,[rcx+rax]
    ACShadows.exe+346B5C6 - 48 89 84 24 80000000  - mov [rsp+00000080],rax
    */

    void* code_addr =
      SK_ScanAlignedExec ("\xc1\xe0\x03\x48\x8B\x15\x00\x00\x00\x00\x80\x7A\x24\x00\x48\x8B\x04\x01\x48\x89\x84\x24", 22,
                          "\xff\xff\xff\xff\xff\xff\x00\x00\x00\x00\xff\xff\xff\x00\x48\x8B\x04\x01\x48\x89\x84\x24", (void*)base_addr, 4);
  
    if (code_addr != nullptr)
    {
      uint32_t  ptr_offset =                                    0x24;
      uint32_t  rip_offset = *(uint32_t *)((uintptr_t)code_addr + 6);
      uintptr_t rip        =               (uintptr_t)code_addr + 10;
  
      pFrameGenEnabled =
        *(bool **)(rip + rip_offset) + ptr_offset;
    }

    /*
    ACShadows.exe+3397C4C - 83 FD 0A              - cmp ebp,0A { 10 }
    ACShadows.exe+3397C4F - 0F94 C0               - sete al
    ACShadows.exe+3397C52 - 44 89 66 20           - mov [rsi+20],r12d
    ACShadows.exe+3397C56 - 44 88 6E 24           - mov [rsi+24],r13b  <---  (base + 10)
    ACShadows.exe+3397C5A - 89 E9                 - mov ecx,ebp
    ACShadows.exe+3397C5C - 83 E1 FE              - and ecx,-02 { 254 }
    */

    code_addr =
      SK_ScanAlignedExec ("\x83\xFD\x0A\x0F\x94\xC0\x44\x89\x66\x20\x44\x88\x6E\x24\x89\xE9\x83\xE1\xFE", 19,
                          "\x83\xFD\x0A\x0F\x94\xC0\x44\x89\x66\x20\x44\x88\x6E\x24\x89\xE9\x83\xE1\xFE", (void*)base_addr);

    if (code_addr != nullptr)
    {
      FrameGenTestAddr =
        (void *)((uintptr_t)code_addr + 10);
    }
  );

  if (pFrameGenEnabled == nullptr || FrameGenTestAddr == nullptr)
  {
    SK_RunOnce (SK_ImGui_Warning (L"Could not find Frame Generation code?!"));
    return false;
  }

  LastTimeFMVChecked = SK::ControlPanel::current_time;

  DWORD                                                             dwOrigProt = 0x0;
  if (VirtualProtect (FrameGenTestAddr, 4, PAGE_EXECUTE_READWRITE, &dwOrigProt))
  { memcpy           (FrameGenTestAddr, (unsigned char *)(enable ? "\x90\x90\x90\x90"
                                                                 : "\x44\x88\x6E\x24"), 4);
    VirtualProtect   (FrameGenTestAddr,  4, dwOrigProt,             &dwOrigProt);

    *pFrameGenEnabled = enable;
  }

  return  
    *pFrameGenEnabled;
}

sl::Result
SK_ACS_slDLSSGSetOptions_Detour (const sl::ViewportHandle& viewport, sl::DLSSGOptions& options)
{
  SK_LOG_FIRST_CALL

  static bool enabled_once = false;

  if (enabled_once)
  {
    if (__SK_ACS_IsMultiFrameCapable &&
        __SK_ACS_DLSSG_MultiFrameCount >= 1)
    {
      options.numFramesToGenerate =
        __SK_ACS_DLSSG_MultiFrameCount;
    }

    if (__SK_ACS_AlwaysUseFrameGen && !ReadULongAcquire (&FrameGenDisabledForFMV))
    {
      options.mode                = sl::DLSSGMode::eOn;
      options.numFramesToGenerate = std::max (1u, options.numFramesToGenerate);
      SK_ACS_ApplyFrameGenOverride (true);
    }

    return
      slDLSSGSetOptions_ACS_Original (viewport, options);
  }

  auto ret =
    slDLSSGSetOptions_ACS_Original (viewport, options);

  if (ret == sl::Result::eOk)
  {
    enabled_once =
      (options.mode == sl::DLSSGMode::eOn);
  }

  return ret;
}

CloseHandle_pfn __SK_ACS_CloseHandle_Original = nullptr;

BOOL
WINAPI
SK_ACS_CloseHandle_Detour (HANDLE hObject)
{
  SK_LOG_FIRST_CALL

  BOOL bRet =
    __SK_ACS_CloseHandle_Original (hObject);

  if (bRet && hObject == LastFMVHandle)
  {
    LastFMVHandle = nullptr;
    SK_LOGi0 (L"FMV Closed");
  }

  return bRet;
}

#include <imgui/font_awesome.h>

bool
SK_ACS_PlugInCfg (void)
{
  if (ImGui::CollapsingHeader ("Assassin's Creed Shadows", ImGuiTreeNodeFlags_DefaultOpen))
  {
    static bool restart_required = false;

    ImGui::TreePush ("");

    bool changed = false;

    bool always_use_framegen =
      __SK_ACS_AlwaysUseFrameGen;

    ImGui::SeparatorText ("Framerate");
    ImGui::TreePush      ("Framerate");

    ImGui::BeginGroup ();

    if (ImGui::Checkbox ("Allow Cutscene Frame Generation",
                                          &__SK_ACS_AlwaysUseFrameGen))
    { if (SK_ACS_ApplyFrameGenOverride    (__SK_ACS_AlwaysUseFrameGen) != always_use_framegen)
      {  _SK_ACS_AlwaysUseFrameGen->store (__SK_ACS_AlwaysUseFrameGen);

        changed = true;
      }
    }

    if (ImGui::BeginItemTooltip ())
    {
      ImGui::TextUnformatted ("Enable Frame Generation during Cutscenes and in menus, such as the Map Screen.");
      ImGui::Separator       ();
      ImGui::BulletText      ("Cutscene Frame Generation will self-disable when FMVs begin playing (to prevent crashes).");
      ImGui::EndTooltip      ();
    }

    if (__SK_ACS_AlwaysUseFrameGen)
    {
      ImGui::SameLine ();

      if (ImGui::Checkbox ("Identify FMVs", &__SK_ACS_ShowFMVIndicator))
      {     _SK_ACS_ShowFMVIndicator->store (__SK_ACS_ShowFMVIndicator);
        changed = true;
      }

      ImGui::SetItemTooltip ("Identify when cutscenes are running at low framerate because they are pre-rendered.");
    }

    if (__SK_HasDLSSGStatusSupport)
    {
      static bool has_used_dlssg  = __SK_IsDLSSGActive;
                  has_used_dlssg |= __SK_IsDLSSGActive;

      if (has_used_dlssg)
      {
        if (__SK_ACS_IsMultiFrameCapable)
        {
          changed |= ImGui::RadioButton ("2x FrameGen", &__SK_ACS_DLSSG_MultiFrameCount, 1);
          ImGui::SetItemTooltip ("May require opening and closing game menus to take effect.");
          ImGui::SameLine    ( );
          changed |= ImGui::RadioButton ("3x FrameGen", &__SK_ACS_DLSSG_MultiFrameCount, 2);
          ImGui::SetItemTooltip ("May require opening and closing game menus to take effect; note that SK reports the pre-framegen framerate in 3x mode.");
          ImGui::SameLine    ( );
          changed |= ImGui::RadioButton ("4x FrameGen", &__SK_ACS_DLSSG_MultiFrameCount, 3);
          ImGui::SetItemTooltip ("May require opening and closing game menus to take effect; note that SK reports the pre-framegen framerate in 4x mode.");

          if (changed)
          {
            _SK_ACS_DLSSG_MultiFrameCount->store (__SK_ACS_DLSSG_MultiFrameCount);
          }
        }

        if (ImGui::Checkbox ("Allow DLSS Flip Metering", &config.nvidia.dlss.allow_flip_metering))
        {
          config.utility.save_async ();

          restart_required = true;
        }

        if (ImGui::BeginItemTooltip ())
        {
          ImGui::TextUnformatted ("Generate DLSS4 Frames Early and Use Hardware Flip Queue to Pace their Presentation");
          ImGui::Separator       (  );
          ImGui::BulletText      ("SK's overlay will appear blurred for rapidly changing text, but frame generation smoothness is improved.");
          ImGui::BulletText      ("Disabling helps software that cannot tell generated and real frames apart (i.e. RTSS), but is discouraged.");
          ImGui::BulletText      ("Use Special K's \"Native Pacing\" DLSS Frame Generation mode when Flip Metering is enabled.");
          ImGui::Separator       (  );
          ImGui::TextUnformatted ("Ignore extra frames in SK's \"Render Latency\" stat -- HW Flip Queue takes care of those.");
          ImGui::EndTooltip      (  );
        }
      }
    }

    static float last_game_fps_limit = __target_fps;
           float y_pos               = ImGui::GetCursorPosY ();

    if (ImGui::Checkbox ("Uncap Framerate", &__SK_ACS_UncapFramerate))
    {
      changed = true;

      _SK_ACS_UncapFramerate->store (__SK_ACS_UncapFramerate);

      if (! __SK_ACS_UncapFramerate)
      {
        __target_fps =
          last_game_fps_limit;
      }
    }

    if (ImGui::BeginItemTooltip ())
    {
      ImGui::TextUnformatted ("Uncap Framerate in Menus and Cutscenes");
      ImGui::Separator       ();
      ImGui::BulletText      ("ersh and Lyall have similar standalone mods that you may use.");
      ImGui::BulletText      ("Turning this back on may require opening and closing the game's menu before limits reapply.");
      ImGui::EndTooltip      ();
    }

    if (__SK_ACS_ClothSimAddr != 0)
    {
      if (ImGui::Checkbox ("Dynamic Cloth Rate", &__SK_ACS_DynamicCloth))
      {
        changed = true;

        _SK_ACS_DynamicCloth->store (__SK_ACS_DynamicCloth);

        SK_ACS_ApplyClothPhysicsFix (__SK_ACS_DynamicCloth);
      }

      if (ImGui::BeginItemTooltip ())
      {
        ImGui::TextUnformatted ("Use dynamic cloth framerate rather than fixed 30/60 in cutscenes/gameplay.");
        ImGui::Separator       ();
        ImGui::BulletText      ("It is critical to set a sustainable framerate limit, cloth simulation will ALWAYS run at the framerate limit (even if the game runs slower).");
        ImGui::BulletText      ("Original feature thanks to Lyall's ACShadowsFix mod, refer to GitHub.");
        ImGui::EndTooltip      ();
      }
    }

    ImGui::EndGroup ();
    
    if (__target_fps != config.render.framerate.target_fps)
    {
      last_game_fps_limit = __target_fps;

      ImGui::SameLine        (  );
      ImGui::BeginGroup      (  );
      ImGui::SetCursorPosY   (y_pos);
      ImGui::PushStyleColor  (ImGuiCol_Text, ImColor::HSV (0.075f, 0.8f, 0.9f).Value);
      ImGui::BulletText      ("Using game-defined framerate limit:  ");
      ImGui::SameLine        (  );
      ImGui::SetCursorPosY   (y_pos);
      ImGui::TextColored     (ImColor (1.f, 1.f, 0.f).Value, "%3.0f fps", __target_fps);
      ImGui::PopStyleColor   (  );
      ImGui::EndGroup        (  );
    }

    ImGui::TreePop ();

    ImGui::SeparatorText ("Aspect Ratio");
    ImGui::TreePush      ("Aspect Ratio");

    if (__SK_ACS_BlackBarsAddr != 0)
    {
      if (ImGui::Checkbox ("Disable Cutscene Blackbars", &__SK_ACS_RemoveBlackBars))
      {
        changed = true;

        _SK_ACS_RemoveBlackBars->store (__SK_ACS_RemoveBlackBars);

        SK_ACS_ApplyBlackBarRemoval (__SK_ACS_RemoveBlackBars);
      }

      if (ImGui::BeginItemTooltip ())
      {
        ImGui::TextUnformatted ("Remove Black Bars During Cutscenes.");
        ImGui::Separator       ();
        ImGui::BulletText      ("This feature comes thanks to Lyall's ACShadowsFix mod, refer to GitHub.");
        ImGui::EndTooltip      ();
      }
    }

    if ( __SK_ACS_FOVSliderAddr     != 0 &&
         __SK_ACS_FOVMultiplierAddr != 0 )
    {
      ImGui::SameLine ();

      if (ImGui::Checkbox ("Expand FOV Range", &__SK_ACS_ExpandFOVRange))
      {
        changed = true;

        _SK_ACS_ExpandFOVRange->store (__SK_ACS_ExpandFOVRange);

        SK_ACS_ApplyExpandedFOV (__SK_ACS_ExpandFOVRange);
      }

      if (ImGui::BeginItemTooltip ())
      {
        ImGui::TextUnformatted ("Expand FOV Slider Range from 85-115 to 70-150.");
        ImGui::Separator       ();
        ImGui::BulletText      ("This feature comes thanks to Lyall's ACShadowsFix mod, refer to GitHub.");
        ImGui::EndTooltip      ();
      }
    }

    ImGui::TreePop ();

    if (restart_required)
    {
      ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (.3f, .8f, .9f).Value);
      ImGui::BulletText     ("Game Restart Required");
      ImGui::PopStyleColor  ();
    }

    if (changed)
    {
      SK_SaveConfig ();
    }

    ImGui::TreePop ();
  }

  return true;
}

void
SK_ACS_InitPlugin (void)
{
  // Address issues caused by Steam Input
  config.nvidia.dlss.disable_ota_updates        = false;
  config.input.gamepad.dinput.blackout_gamepads = true;

  static HANDLE hInitThread =
  SK_Thread_CreateEx ([](LPVOID)->DWORD
  {
    SK_Thread_SetCurrentPriority (THREAD_PRIORITY_HIGHEST);

    static void* img_base_addr = 
      SK_Debug_GetImageBaseAddr ();
  
    _SK_ACS_AlwaysUseFrameGen =
      _CreateConfigParameterBool  ( L"AssassinsCreed.FrameRate",
                                    L"AlwaysUseFrameGen", __SK_ACS_AlwaysUseFrameGen,
                                    L"Use FrameGen in Cutscenes" );

    _SK_ACS_ShowFMVIndicator =
      _CreateConfigParameterBool ( L"AssassinsCreed.FrameRate",
                                   L"ShowFMVIndicator", __SK_ACS_ShowFMVIndicator,
                                   L"Show an indicator while FMVs are Playing" );

    _SK_ACS_UncapFramerate =
      _CreateConfigParameterBool ( L"AssassinsCreed.FrameRate",
                                   L"UncapMenusAndCutscenes", __SK_ACS_UncapFramerate,
                                   L"Uncap Framerate in Cutscenes" );

    _SK_ACS_DLSSG_MultiFrameCount =
      _CreateConfigParameterInt  ( L"AssassinsCreed.FrameRate",
                                   L"DLSSGMultiFrameCount", __SK_ACS_DLSSG_MultiFrameCount,
                                   L"Override Multi-Frame Gen" );

    _SK_ACS_DynamicCloth =
      _CreateConfigParameterBool ( L"AssassinsCreed.FrameRate",
                                   L"DynamicClothRate", __SK_ACS_DynamicCloth,
                                   L"Use Dynamic Rate For Cloth" );

    _SK_ACS_RemoveBlackBars =
      _CreateConfigParameterBool ( L"AssassinsCreed.AspectRatio",
                                   L"RemoveCutsceneBlackBars", __SK_ACS_RemoveBlackBars,
                                   L"Remove Aspect Ratio Black Bars from Cutscenes" );

    _SK_ACS_RemoveBlackBars =
      _CreateConfigParameterBool ( L"AssassinsCreed.AspectRatio",
                                   L"RemoveCutsceneBlackBars", __SK_ACS_RemoveBlackBars,
                                   L"Remove Aspect Ratio Black Bars from Cutscenes" );

    _SK_ACS_ExpandFOVRange =
      _CreateConfigParameterBool ( L"AssassinsCreed.AspectRatio",
                                   L"ExpandFOVRange", __SK_ACS_ExpandFOVRange,
                                   L"Enable Wider Range of Adjustment in FOV Slider" );

    plugin_mgr->config_fns.emplace (SK_ACS_PlugInCfg);

    // Pattern scanning timeouts are not required on this thread
    SK_TLS_Bottom ()->memory->memory_scans_should_timeout = FALSE;

    __SK_ACS_ClothSimAddr =
      (uintptr_t)SK_ScanAlignedExec ("\x4C\x00\x00\x00\x49\x00\x00\x00\x45\x0F\x00\x00\x00\x00\x00\x00\x0F\x00\x00\x00\x00\x00\x00\x83\x00\x00\x49\x00\x00\x01", 30,
                                     "\x4C\x00\x00\x00\x49\x00\x00\x00\x45\x0F\x00\x00\x00\x00\x00\x00\x0F\x00\x00\x00\x00\x00\x00\x83\x00\x00\x49\x00\x00\x01",
                                     (void*)img_base_addr);

    if (__SK_ACS_ClothSimAddr != 0)
    {
      SK_ACS_ApplyClothPhysicsFix (__SK_ACS_DynamicCloth);
    }

    __SK_ACS_BlackBarsAddr =
      (uintptr_t)SK_ScanAlignedExec ("\x84\xC0\x74\x00\xC5\xFA\x00\x00\x00\x00\xC5\xFA\x00\x00\x00\x00\xC5\xFA\x00\x00\xC5\x00\x57\x00\xC5\xFA", 26,
                                     "\x84\xC0\x74\x00\xC5\xFA\x00\x00\x00\x00\xC5\xFA\x00\x00\x00\x00\xC5\xFA\x00\x00\xC5\x00\x57\x00\xC5\xFA",
                                     (void*)img_base_addr);

    if (__SK_ACS_BlackBarsAddr != 0)
    {
      SK_ACS_ApplyBlackBarRemoval (__SK_ACS_RemoveBlackBars);
    }

    while (SK_GetFramesDrawn () < 30)
           SK_SleepEx (150UL, FALSE);

    __SK_ACS_FOVSliderAddr =
      (uintptr_t)SK_ScanAlignedExec ("\xE9\x00\x00\x00\x00\x48\x00\x00\x48\x00\x00\x48\x00\x00\xFF\x00\x00\x00\x00\x00\x48\x00\x00\x00\x00\xC5", 26,
                                     "\xFF\x00\x00\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\xFF\x00\x00\x00\x00\x00\xFF\x00\x00\x00\x00\xFF",
                                     (void*)img_base_addr, 0x2);

    __SK_ACS_FOVMultiplierAddr =
      (uintptr_t)SK_ScanAlignedExec ("\x77\x00\x48\x8B\x00\x00\x00\x00\x00\x48\x85\x00\x74\x00\x48\x8B\x00\xFF\x00\x00\x00\x00\x00\xC5\xFA\x00\x00\x00\xC5\xFA\x00\x00\x00\x00\x00\x00\x48\x8B", 38,
                                     "\xFF\x00\xFF\xFF\x00\x00\x00\x00\x00\xFF\xFF\x00\xFF\x00\xFF\xFF\x00\xFF\x00\x00\x00\x00\x00\xFF\xFF\x00\x00\x00\xFF\xFF\x00\x00\x00\x00\x00\x00\xFF\xFF",
                                     (void*)img_base_addr, 0x2);

    config.compatibility.memory_scans_may_timeout = orig_timeout_policy;

    if ( __SK_ACS_FOVSliderAddr     != 0 &&
         __SK_ACS_FOVMultiplierAddr != 0 )
    {
      SK_LOGi1 (L"Found FOV addresses: %p and %p", __SK_ACS_FOVSliderAddr, __SK_ACS_FOVMultiplierAddr);

      SK_ACS_ApplyExpandedFOV (__SK_ACS_ExpandFOVRange);
    }

    while (SK_GetFramesDrawn () < 480)
           SK_SleepEx (150UL, FALSE);

    // Fail-safe incase any code that sets this was missed
    static float* framerate_limit = nullptr;

    static DWORD fmv_timeout_ms = 5000UL;

    static concurrency::concurrent_unordered_set <DWORD64> ContinuableCallSites;
    
    for ( auto& callsite :
            // 1.0.1
            { 0x0000000003724651, 0x0000000003725A7F, 0x00000000006A32F1, 0x00000000006A32F7,
              0x00000000006A330E, 0x00000000006A32B0, 0x00000000006A32B4, 0x00000000006A32B8,
              0x000000000372CD05, 0x000000000372CD0C } )
    {
      ContinuableCallSites.insert (callsite);
    }

    // Self-disable cutscene frame generation if it causes a crash, and then
    //   ignore the crash...
    AddVectoredExceptionHandler (1, [](_EXCEPTION_POINTERS *ExceptionInfo)->LONG
    {
      bool continuable = false;

      if (ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
      {
        auto Context = ExceptionInfo->ContextRecord;

        const DWORD64 addr =
             (DWORD64)Context->Rip - (DWORD64)SK_Debug_GetImageBaseAddr ();

        static concurrency::concurrent_unordered_set <DWORD64> ContinuableCallSitesFound;

        // Turn off frame generation and give a second-chance at life
        if (__SK_ACS_AlwaysUseFrameGen)
        {
          if (LastFMVHandle != 0 && ContinuableCallSites.insert (addr).second)
          {
            SK_LOGi0 (L"FMV Cutscene Exception Found: RIP=%p", addr);
          }

          if (ContinuableCallSites.count       (addr)        &&
              ContinuableCallSitesFound.insert (addr).second &&
              ContinuableCallSitesFound.size   (    )        == 10)
          {
            SK_LOGi0 (L"All 10 FMV Exception CallSites Found!");
            fmv_timeout_ms = 500UL;
          }

                            *pFrameGenEnabled = false;
          WriteULongRelease (&FrameGenDisabledForFMV, TRUE);

          continuable =
            ContinuableCallSites.count (addr) != 0;

          if (! continuable)
          {
            SK_LOGi0 (L"Non-Continuable Exception RIP=%p", addr);
          }

          else
          {
            PVOID SKX_GetNextInstruction (LPVOID addr);

            ExceptionInfo->ContextRecord->Rip =
              (DWORD64)SKX_GetNextInstruction ((void *)ExceptionInfo->ContextRecord->Rip);
          }
        }
      }

      return
        ( continuable ? EXCEPTION_CONTINUE_EXECUTION
                      : EXCEPTION_CONTINUE_SEARCH );
    });

    /*
    ACShadows.exe+F7B0C9 - 48 83 C4 20           - add rsp,20 { 32 }
    ACShadows.exe+F7B0CD - 5E                    - pop rsi
    ACShadows.exe+F7B0CE - C3                    - ret 
    ACShadows.exe+F7B0CF - 80 79 2C 00           - cmp byte ptr [rcx+2C],00 { 0 }
    ACShadows.exe+F7B0D3 - 75 CA                 - jne ACShadows.exe+F7B09F         <---  base + 10
    ACShadows.exe+F7B0D5 - EB 05                 - jmp ACShadows.exe+F7B0DC
    ACShadows.exe+F7B0D7 - 45 84 C0              - test r8b,r8b
    ACShadows.exe+F7B0DA - 75 C3                 - jne ACShadows.exe+F7B09F
    */

    static void* limit_check_addr =
      (void *)((uintptr_t)SK_ScanAlignedExec ("\x48\x83\xC4\x20\x5E\xC3\x80\x79\x2C\x00\x75\xCA\xEB\x05\x45\x84\xC0\x75\xC3", 19,
                                              "\x48\x83\xC4\x20\x5E\xC3\x80\x79\x2C\x00\x75\xCA\xEB\x05\x45\x84\xC0\x75\xC3", (void*)img_base_addr) + 10);
    static void* limit_store_addr = nullptr;

    if (limit_check_addr != (void *)10)
    {
      /*
      ACShadows.exe+F7B0BA - 48 8B 05 9F58D108     - mov rax,[ACShadows.exe+9C90960] { (04864E98) }
      ACShadows.exe+F7B0C1 - C5FA1180 98 000000    - vmovss [rax+00000098],xmm0
      */

      limit_store_addr =
        (void *)((uintptr_t)limit_check_addr - 18);

      uint32_t  ptr_offset =                                           0x98;
      uint32_t  rip_offset = *(uint32_t *)((uintptr_t)limit_store_addr - 4);
      uintptr_t rip        =               (uintptr_t)limit_store_addr;

      framerate_limit =
        (float *)(*(uint8_t **)((uintptr_t)rip + rip_offset) + ptr_offset);
    }

    else
    {
      SK_ImGui_Warning (L"Could not find Framerate Limiter Code?!");
    }

    config.system.silent_crash = true;
    config.utility.save_async ();

    static const bool is_fg_capable =
      StrStrW (sk::NVAPI::EnumGPUs_DXGI ()[0].Description, L"RTX " ) != nullptr &&
      StrStrW (sk::NVAPI::EnumGPUs_DXGI ()[0].Description, L"RTX 2") == nullptr &&
      StrStrW (sk::NVAPI::EnumGPUs_DXGI ()[0].Description, L"RTX 3") == nullptr;

    // Pull out the trump card and eliminate flaky NGX feature support queries, by
    // reporting everything as supported as long as an RTX GPU not 2xxx or 3xxx is
    // installed.
    if (is_fg_capable)
    {
      __SK_ACS_IsMultiFrameCapable =
        StrStrW (sk::NVAPI::EnumGPUs_DXGI ()[0].Description, L"RTX " ) != nullptr &&
        StrStrW (sk::NVAPI::EnumGPUs_DXGI ()[0].Description, L"RTX 2") == nullptr &&
        StrStrW (sk::NVAPI::EnumGPUs_DXGI ()[0].Description, L"RTX 3") == nullptr &&
        StrStrW (sk::NVAPI::EnumGPUs_DXGI ()[0].Description, L"RTX 4") == nullptr;

      config.nvidia.dlss.spoof_support = true;
    }

    void*                            pfnCloseHandle = nullptr;
    SK_CreateDLLHook2 (
           L"kernel32",                "CloseHandle",
                                 SK_ACS_CloseHandle_Detour,
      static_cast_p2p <void> (&__SK_ACS_CloseHandle_Original),
                                    &pfnCloseHandle );
    MH_EnableHook                  ( pfnCloseHandle );

    plugin_mgr->open_file_w_fns.insert ([](LPCWSTR lpFileName, HANDLE hFile)
    {
      if (StrStrIW (lpFileName, L"webm"))
      {
        bool file_is_exempt = false;

        for ( auto exempt_substr : { L"Epilepsy",
                                     L"UbisoftLogo",
                                     L"HUB_Bootflow_AbstergoIntro",
                                     L"ACI_Panel_Red_IMG_UI",
                                     L"ACI_Panel_Gen_IMG_UI" } )
        {
          file_is_exempt =
            StrStrIW (lpFileName, exempt_substr);

          if (file_is_exempt)
            break;
        }

        if (! file_is_exempt)
        {
          LastFMVHandle = hFile;

          WriteULongRelease            (&FrameGenDisabledForFMV, TRUE);
          SK_ACS_ApplyFrameGenOverride (false);

          if (__SK_ACS_AlwaysUseFrameGen)
          {
            SK_LOGi0 (
              L"Temporarily disabling Frame Generation because video '%ws' was opened...",
                lpFileName
            );
          }
        }
      }
    });

    // The pointer base addr is stored in the limit_load_addr instruction
    plugin_mgr->begin_frame_fns.insert ([](void)
    {
      float game_limit =
        (framerate_limit == nullptr) ? -1.0f : *framerate_limit;

      if (limit_check_addr != (void *)10)
      {
        static bool UncapFramerate = !__SK_ACS_UncapFramerate;

        static uint8_t orig_check_bytes [2] = {};
        static uint8_t orig_store_bytes [8] = {};

        SK_RunOnce (
          memcpy (orig_store_bytes, limit_store_addr, 8);
          memcpy (orig_check_bytes, limit_check_addr, 2);
        );

        DWORD dwOrigProt = 0x0;

        if (std::exchange (UncapFramerate, __SK_ACS_UncapFramerate) !=
                                           __SK_ACS_UncapFramerate)
        {
          VirtualProtect (limit_store_addr, 8, PAGE_EXECUTE_READWRITE, &dwOrigProt);
          memcpy         (limit_store_addr, UncapFramerate   ?    (unsigned char *)
                          "\x90\x90\x90\x90\x90\x90\x90\x90" : orig_store_bytes, 8);
          VirtualProtect (limit_store_addr, 8, dwOrigProt,             &dwOrigProt);
          VirtualProtect (limit_check_addr, 2, PAGE_EXECUTE_READWRITE, &dwOrigProt);
          memcpy         (limit_check_addr, UncapFramerate   ?    (unsigned char *)
                                                  "\x90\x90" : orig_check_bytes, 2);
          VirtualProtect (limit_check_addr, 2, dwOrigProt,             &dwOrigProt);
        }
      }

      // Not tested adequately in non-framegen cases
      if (__SK_ACS_AlwaysUseFrameGen)
      {
        // Replace Ubisoft's poor excuse for a framerate limiter in FMVs with SK's.
        if      (ReadULongAcquire (&FrameGenDisabledForFMV))       __target_fps = 30.0f;
        else if ((! __SK_ACS_UncapFramerate) && game_limit > 0.0f) __target_fps = game_limit;
        else                                                       __target_fps = config.render.framerate.target_fps;
      }

      else
      {
        // Replace Ubisoft's poor excuse for a framerate limiter in FMVs with SK's.
        if      (ReadULongAcquire (&FrameGenDisabledForFMV))
        {
          __target_fps = 30.0f;
        }

        else if ((! __SK_ACS_UncapFramerate) && game_limit >   0.0f &&
                                                game_limit < 500.0f)
        {
          __target_fps = game_limit;
        }

        else
        {
          __target_fps = config.render.framerate.target_fps;
        }
      }

      // 3.333 second grace period after an FMV is read to reset frame generation
      if (LastTimeFMVChecked < SK::ControlPanel::current_time - 3333UL)
      {
        if (ReadULongAcquire (&FrameGenDisabledForFMV) != 0 || (pFrameGenEnabled != nullptr &&
                                                               *pFrameGenEnabled == false))
        {
          // Video is done playing, game has unlimited framerate again.
          if (game_limit != 30.0f)
          {
            SK_ACS_ApplyFrameGenOverride (__SK_ACS_AlwaysUseFrameGen);
            WriteULongRelease            (&FrameGenDisabledForFMV, FALSE);
          }
        }

        if (__SK_ACS_UncapFramerate && framerate_limit != nullptr)
        {
          // -1.0f = Unlimited (set by game in special cases)
          *framerate_limit = -1.0f;
        }
      }

      else if (__SK_ACS_AlwaysUseFrameGen && ReadULongAcquire (&FrameGenDisabledForFMV) != 0 && ( pFrameGenEnabled != nullptr &&
                                                                                                 *pFrameGenEnabled == false ))
      {
        if (__SK_ACS_ShowFMVIndicator)
        {
          SK_ImGui_CreateNotificationEx (
            "ACShadows.FMV", SK_ImGui_Toast::Other, nullptr, nullptr,
                   INFINITE, SK_ImGui_Toast::UseDuration  |
                             SK_ImGui_Toast::ShowNewest   | 
                             SK_ImGui_Toast::DoNotSaveINI |
                             SK_ImGui_Toast::Unsilencable,
            [](void*)->bool
            {
              ImColor fmv_color (
                1.0f, 0.941177f, 0.f,
                  static_cast <float> (
                    0.75 + 0.2 * std::cos (3.14159265359 *
                      (static_cast <double> (SK::ControlPanel::current_time % 2500) / 1750.0))
                  )
              );

              ImGui::BeginGroup  ();
              ImGui::TextColored (fmv_color, ICON_FA_FILM "  30 fps ");
              ImGui::EndGroup    ();

              return false;
            }
          );
        }
      }

      else
      {
        if (__SK_ACS_UncapFramerate && framerate_limit != nullptr)
        {
          // -1.0f = Unlimited (set by game in special cases)
          *framerate_limit = -1.0f;
        }
      }

      if (                                        __SK_ACS_AlwaysUseFrameGen)
        SK_RunOnce (SK_ACS_ApplyFrameGenOverride (__SK_ACS_AlwaysUseFrameGen));

      if (__SK_IsDLSSGActive)
      {
        static HMODULE
            hModSLDLSSG  = (HMODULE)-1;
        if (hModSLDLSSG == (HMODULE)-1)GetModuleHandleExW (GET_MODULE_HANDLE_EX_FLAG_PIN, L"sl.dlss_g.dll",
           &hModSLDLSSG);

        if (hModSLDLSSG != nullptr)
        {
          SK_RunOnce (
            slGetPluginFunction_pfn
            slGetPluginFunction =
           (slGetPluginFunction_pfn)SK_GetProcAddress (hModSLDLSSG,
           "slGetPluginFunction");

            slDLSSGSetOptions_pfn                       slDLSSGSetOptions =
           (slDLSSGSetOptions_pfn)slGetPluginFunction ("slDLSSGSetOptions");

            SK_CreateFuncHook   (     L"slDLSSGSetOptions",
                                        slDLSSGSetOptions,
                                 SK_ACS_slDLSSGSetOptions_Detour,
               static_cast_p2p <void> (&slDLSSGSetOptions_ACS_Original) );
            MH_EnableHook       (       slDLSSGSetOptions               );
          );
        }
      }
    });

    SK_Thread_CloseSelf ();

    return 0;
  });
}