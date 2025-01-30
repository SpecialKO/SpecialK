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

#include <SpecialK/stdafx.h>
#include <SpecialK/render/d3d11/d3d11_core.h>
#include <imgui/font_awesome.h>

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"Tales Plug"

bool SK_TGFix_HookMonoInit        (void);
bool SK_TGFix_SetupFramerateHooks (void);
void SK_TGFix_SetMSAASampleCount  (int);
void SK_TGFix_SetRenderScale      (float);
void SK_TGFix_SetCameraAA         (void);
void SK_TGFix_SetInputPollingFreq (float PollingHz);
void SK_TGFix_EnableInternalHDR   (bool);

extern volatile LONG SK_D3D11_DrawTrackingReqs;

template <typename _T>
class PlugInParameter {
public:
  typedef _T           value_type;

  PlugInParameter (const value_type& value) : cfg_val (value) { };

  operator value_type&  (void) noexcept { return  cfg_val; }
  value_type* operator& (void) noexcept { return &cfg_val; }

  value_type& operator= (const value_type& val) noexcept { return (cfg_val = val); }

  void        store     (void)                  {        if (ini_param != nullptr) ini_param->store ( cfg_val       ); }
  void        store     (const value_type& val) {        if (ini_param != nullptr) ini_param->store ((cfg_val = val)); }
  value_type& load      (void)                  { return     ini_param != nullptr? ini_param->load  ( cfg_val)
                                                                                 :                    cfg_val;         }

  bool bind_to_ini (sk::Parameter <value_type>* _ini) noexcept
  {
    if (!  ini_param) ini_param = _ini;
    return ini_param           == _ini;
  }

private:
  sk::Parameter <value_type>* ini_param = nullptr;
  value_type                  cfg_val   = {     };
};

struct sk_tgfix_cfg_s {
  PlugInParameter <bool>  disable_dof        =  true;
  PlugInParameter <bool>  disable_bloom      = false;
  PlugInParameter <bool>  disable_heat_haze  = false;
  PlugInParameter <bool>  sharpen_outlines   = false;

  PlugInParameter <bool>  disable_fxaa       = false;
  PlugInParameter <bool>  disable_fog        =  true;

  PlugInParameter <bool>  use_taa            = false;
  PlugInParameter <float> taa_jitter         =  0.9f;//Not configurable yet
  PlugInParameter <float> taa_blend_power    = 0.75f;//Not configurable yet
  PlugInParameter <float> taa_frame_influence= 0.33f;//Not configurable yet
  PlugInParameter <int>   taa_quality        =     2;//Not configurable yet

  PlugInParameter <int>   hdr_quality        =     1; // 1 = 32-bit, 2 = 64-bit
  PlugInParameter <int>   msaa_sample_count  =     1;
  PlugInParameter <float> render_scale       =  1.0f;
  PlugInParameter <bool>  achievement_earned = false; // Don't show achievement bug warnings after first one unlocked.

  // Special K's Windows.Gaming.Input emulation can easily
  //   poll at 1 kHz with zero performance overhead, so just
  //     default to 1 kHz and everyone profits!
  // 
  //  * Game is hardcoded to poll gamepad input at 60 Hz
  //      regardless of device caps or framerate limit, ugh!
  PlugInParameter <float> gamepad_polling_hz = 10000.0f;

  struct {
    PlugInParameter <bool> constant_visibility = true;
    PlugInParameter <bool> fix_shadow_scissors = true;
  } hacks;
} SK_TGFix_Cfg;

constexpr
float SK_TGFix_NativeAspect          = 16.0f / 9.0f;
float SK_TGFix_AspectRatio           =  0.0f;
float SK_TGFix_AspectMultiplier      =  0.0f;
UINT  SK_TGFix_ScreenWidth           =    0U;
UINT  SK_TGFix_ScreenHeight          =    0U;
float SK_TGFix_InputPollingFrequency = 60.0f;

bool
SK_TGFix_PlugInCfg (void)
{
  if (ImGui::CollapsingHeader ("Tales of Graces f Remastered", ImGuiTreeNodeFlags_DefaultOpen))
  {
    bool cfg_changed = false;

    ImGui::TreePush       ("");
    ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.02f, 0.68f, 0.90f, 0.45f));
    ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.07f, 0.72f, 0.90f, 0.80f));
    ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.14f, 0.78f, 0.87f, 0.80f));

    static int  restart_reqs = 0;

    ImGui::BeginGroup (  );
    if (ImGui::CollapsingHeader (ICON_FA_PORTRAIT " Graphics Settings", ImGuiTreeNodeFlags_DefaultOpen |
                                                                        ImGuiTreeNodeFlags_AllowOverlap))
    {
      ImGui::TreePush ("");

      int                  msaa_idx = 0;
      switch (SK_TGFix_Cfg.msaa_sample_count)
      {
        default:
        case  1: msaa_idx = 0; break;
        case  2: msaa_idx = 1; break;
        case  4: msaa_idx = 2; break;
        case  8: msaa_idx = 3; break;
      }

      if (ImGui::Combo ("Multisampling", &msaa_idx, "Not Active\0 2x MSAA\0 4x MSAA\0 8x MSAA"))
      {
        SK_TGFix_Cfg.msaa_sample_count.store (1 << msaa_idx);

        SK_TGFix_SetMSAASampleCount (SK_TGFix_Cfg.msaa_sample_count);

        // Turn off FXAA to prevent MSAA from breaking the minimap
        if (SK_TGFix_Cfg.msaa_sample_count > 1)
        {
          bool
              orig_disable_fxaa  = std::exchange (SK_TGFix_Cfg.disable_fxaa, true);
          if (orig_disable_fxaa !=                SK_TGFix_Cfg.disable_fxaa)
          {
            SK_TGFix_Cfg.disable_fxaa.store ();

            SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0xbe80dda2); InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
            SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0xef92e3e1); InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
          }

          bool
              orig_disable_dof  = std::exchange (SK_TGFix_Cfg.disable_dof, true);
          if (orig_disable_dof !=                SK_TGFix_Cfg.disable_dof)
          {
            SK_TGFix_Cfg.disable_dof.store ();

            SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0x54f44c1a);
            InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
          }
        }

        cfg_changed = true;
      }

      ImGui::SetItemTooltip ("Temporal AA cannot be used with MSAA, and there are various unsolved artifacts that will occur using MSAA...");

      if (ImGui::SliderFloat ("Render Scale", &SK_TGFix_Cfg.render_scale, 0.5f, 2.0f)) // Unity supports down to 0.1, but this game expects minimum=0.5
      {
        SK_TGFix_Cfg.render_scale.store ();

        SK_TGFix_SetRenderScale (SK_TGFix_Cfg.render_scale);

        cfg_changed = true;
      }

      ImGui::SetItemTooltip ("Some parts of the game's UI are buggy and scale w/ Render Scale, you should only use this with Temporal AA.");

      if (SK_TGFix_Cfg.msaa_sample_count > 1 && SK_TGFix_Cfg.use_taa)
          SK_TGFix_Cfg.use_taa.store (false);

      int                                  postfx_aa_sel = 0;
      if (      SK_TGFix_Cfg.use_taa)      postfx_aa_sel = 2;
      else if (!SK_TGFix_Cfg.disable_fxaa) postfx_aa_sel = 1;

      if (ImGui::Combo ("PostFX Anti-Aliasing", &postfx_aa_sel, SK_TGFix_Cfg.msaa_sample_count <= 1 ? "None\0Morphological (FXAA)\0Temporal AA\0\0" 
                                                                                                    : "None\0Morphological (FXAA)\0\0"))
      {
        bool orig_disable_fxaa = SK_TGFix_Cfg.disable_fxaa;

        switch (postfx_aa_sel)
        {
          case 0:
            SK_TGFix_Cfg.use_taa.store      (false);
            SK_TGFix_Cfg.disable_fxaa.store (true);
            break;
          case 1:
            SK_TGFix_Cfg.use_taa.store      (false);
            SK_TGFix_Cfg.disable_fxaa.store (false);
            break;
          case 2:
            SK_TGFix_Cfg.use_taa.store      (true);
            SK_TGFix_Cfg.disable_fxaa.store (false);
            break;
        }

        if (SK_TGFix_Cfg.disable_fxaa != orig_disable_fxaa)
        {
          if (SK_TGFix_Cfg.disable_fxaa)
          {
            SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0xbe80dda2); InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
            SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0xef92e3e1); InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
          }

          else
          {
            SK_D3D11_Shaders->pixel.releaseTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0xbe80dda2); InterlockedDecrement (&SK_D3D11_DrawTrackingReqs);
            SK_D3D11_Shaders->pixel.releaseTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0xef92e3e1); InterlockedDecrement (&SK_D3D11_DrawTrackingReqs);
          }
        }

        SK_TGFix_SetCameraAA ();

        cfg_changed = true;
      }
      
      if (ImGui::SliderInt ("Anistropic Level", &config.render.d3d12.max_anisotropy, -1, 16,
                                                 config.render.d3d12.max_anisotropy > 0 ? config.render.d3d12.max_anisotropy != 9 ? "%dx" : "Game Default" : "Game Default"))
      {
        cfg_changed = true;
        restart_reqs++;
      }
      
      ImGui::SetItemTooltip ("Force maximum anisotropic filtering level (game's default = 9x), for native anisotropic "
                             "filtered render passes as well as any forced.");

      if (ImGui::Checkbox ("Force Anisotropic Filtering", &config.render.d3d12.force_anisotropic))
      {
        cfg_changed = true;
        restart_reqs++;
      }
      
      ImGui::SetItemTooltip ("Upgrade standard bilinear or trilinear filtering to anisotropic");

      ImGui::SeparatorText ("Post-Processing Passes");

      bool                                    dof = !SK_TGFix_Cfg.disable_dof;
      if (ImGui::Checkbox ("Depth of Field", &dof))
      {
              SK_TGFix_Cfg.disable_dof = !dof;
        if (! SK_TGFix_Cfg.disable_dof)
        {
          SK_D3D11_Shaders->pixel.releaseTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0x54f44c1a);
          InterlockedDecrement (&SK_D3D11_DrawTrackingReqs);
        }

        else
        {
          SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0x54f44c1a);
          InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
        }

        SK_TGFix_Cfg.disable_dof.store ();

        cfg_changed = true;
      }
      ImGui::SetItemTooltip ("Depth of Field may be unusually strong at DSR resolutions and when using MSAA.");

      const auto fItemSpacing = 
        ImGui::GetStyle ().ItemSpacing.x * 3.0f;

      ImGui::SameLine (0.0f, fItemSpacing);

      bool                               heat_haze = !SK_TGFix_Cfg.disable_heat_haze;
      if (ImGui::Checkbox ("Heat Haze", &heat_haze))
      {
            SK_TGFix_Cfg.disable_heat_haze = !heat_haze;
        if (SK_TGFix_Cfg.disable_heat_haze)
        {
          SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0x4ea47fea);

          InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
        }

        else
        {
          SK_D3D11_Shaders->pixel.releaseTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0x4ea47fea);

          InterlockedDecrement (&SK_D3D11_DrawTrackingReqs);
        }

        SK_TGFix_Cfg.disable_heat_haze.store ();

        cfg_changed = true;
      }
      ImGui::SetItemTooltip ("Heat Haze may be unusually strong at DSR resolutions and when using MSAA.");

      ImGui::SameLine (0.0f, fItemSpacing);

      bool                           bloom = !SK_TGFix_Cfg.disable_bloom;
      if (ImGui::Checkbox ("Bloom", &bloom))
      {
            SK_TGFix_Cfg.disable_bloom = !bloom;
        if (SK_TGFix_Cfg.disable_bloom)
        {
          SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0x5bcdb543);
          SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0xa9ca2e76);
          SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0xafcf335b);
          SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0xd70959df);

          InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
          InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
          InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
          InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);

          restart_reqs++;
        }

        else
        {
          SK_D3D11_Shaders->pixel.releaseTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0x5bcdb543);
          SK_D3D11_Shaders->pixel.releaseTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0xa9ca2e76);
          SK_D3D11_Shaders->pixel.releaseTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0xafcf335b);
          SK_D3D11_Shaders->pixel.releaseTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0xd70959df);

          InterlockedDecrement (&SK_D3D11_DrawTrackingReqs);
          InterlockedDecrement (&SK_D3D11_DrawTrackingReqs);
          InterlockedDecrement (&SK_D3D11_DrawTrackingReqs);
          InterlockedDecrement (&SK_D3D11_DrawTrackingReqs);
        }

        SK_TGFix_Cfg.disable_bloom.store ();

        cfg_changed = true;
      }

      ImGui::SameLine (0.0f, fItemSpacing);

      bool                         fog = !SK_TGFix_Cfg.disable_fog;
      if (ImGui::Checkbox ("Fog", &fog))
      {
        SK_TGFix_Cfg.disable_fog = !fog;
        SK_TGFix_Cfg.disable_fog.store ();

        cfg_changed = true;
      }

      ImGui::SetItemTooltip (
        "Some areas of the game have an additional layer of fogginess that detracts from HDR and "
        "does not cover the entire screen in 21:9.\r\n\r\n"
        "\tYou are encouraged to disable \"Fog\" if you are using RenoDX or a 21:9 screen."
      );

      if (ImGui::Checkbox ("Sharpen Outlines", &SK_TGFix_Cfg.sharpen_outlines))
      {
        if (SK_TGFix_Cfg.sharpen_outlines)
        {
          SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.on_top, 0x2dfcf950);
          SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.on_top, 0x9a0e24eb);
          SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.on_top, 0xf751273e);

          InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
          InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
          InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
        }

        else
        {
          SK_D3D11_Shaders->pixel.releaseTrackingRef (SK_D3D11_Shaders->pixel.on_top, 0x2dfcf950);
          SK_D3D11_Shaders->pixel.releaseTrackingRef (SK_D3D11_Shaders->pixel.on_top, 0x9a0e24eb);
          SK_D3D11_Shaders->pixel.releaseTrackingRef (SK_D3D11_Shaders->pixel.on_top, 0xf751273e);

          InterlockedDecrement (&SK_D3D11_DrawTrackingReqs);
          InterlockedDecrement (&SK_D3D11_DrawTrackingReqs);
          InterlockedDecrement (&SK_D3D11_DrawTrackingReqs);
        }

        SK_TGFix_Cfg.sharpen_outlines.store ();

        cfg_changed = true;
      }

      if (SK_TGFix_AspectRatio != SK_TGFix_NativeAspect)
      {
        ImGui::SeparatorText ("Aspect Ratio");

        if (ImGui::Checkbox ("Use Visibility Hack", &SK_TGFix_Cfg.hacks.constant_visibility))
        {
          SK_TGFix_Cfg.hacks.constant_visibility.store ();

          cfg_changed = true;
        }

        ImGui::SetItemTooltip (
          "Enables a special hack to keep objects off to the side from becoming invisible prematurely."
        );
      }

      if (cfg_changed)
      {
        config.utility.save_async ();
      }

      ImGui::TreePop  (  );
    }
    ImGui::EndGroup   (  );
    ImGui::SameLine   (  );
    ImGui::BeginGroup (  );
    if (ImGui::CollapsingHeader (ICON_FA_GAMEPAD " Gamepad Input", ImGuiTreeNodeFlags_DefaultOpen |
                                                                   ImGuiTreeNodeFlags_AllowOverlap))
    {
      ImGui::TreePush ("");

      if (ImGui::SliderFloat ("Polling Frequency", &SK_TGFix_Cfg.gamepad_polling_hz, 30.0f, 1000.0f, "%.3f Hz", ImGuiSliderFlags_Logarithmic))
      {
        SK_TGFix_SetInputPollingFreq (SK_TGFix_Cfg.gamepad_polling_hz);

        SK_TGFix_Cfg.gamepad_polling_hz.store ();

        config.utility.save_async ();
      }

      ImGui::SetItemTooltip ("Game defaults to 60 Hz, regardless of framerate cap!");

      if (SK_ImGui_HasPlayStationController ())
      {
        ImGui::SeparatorText (ICON_FA_PLAYSTATION " PlayStation Controllers");

        static std::error_code                    ec = { };
        static std::filesystem::path pathPlayStation =
          SK_Resource_GetRoot () / LR"(inject/textures/8E9DD816.dds)";

        static bool fetching_buttons = false;
        static bool has_buttons      = std::filesystem::exists (pathPlayStation, ec);

        bool disable_native_playstation =
          (config.input.gamepad.disable_hid && config.input.gamepad.xinput.emulate);

        if (ImGui::Checkbox ("Disable Native PlayStation Support", &disable_native_playstation))
        {
          if (disable_native_playstation)
          {
            restart_reqs++;
            config.input.gamepad.disable_hid    = true;
            config.input.gamepad.xinput.emulate = true;
          }

          else
          {
            if (std::filesystem::remove (pathPlayStation, ec))
            {
              has_buttons = false;
              restart_reqs++;
            }

            config.input.gamepad.disable_hid    = false;
            config.input.gamepad.xinput.emulate = false;
          }

          config.utility.save_async ();
        }

        if (ImGui::BeginItemTooltip ())
        {
          ImGui::TextUnformatted ("Native PlayStation support in this game does not work with Rumble");
          ImGui::Separator       ();
          ImGui::BulletText      ("Special K can translate PlayStation to XInput and correct the button icons.");
          ImGui::BulletText      ("Disabling the game's native support will break support for local multiplayer.");
          ImGui::EndTooltip      ();
        }

        if (disable_native_playstation)
        {
          if (! has_buttons)
          {
            if (! fetching_buttons)
            {
              if (ImGui::Button ("Download " ICON_FA_PLAYSTATION " Icons\t\t DualShock 4 "))
              {
                fetching_buttons = true;

                SK_Network_EnqueueDownload (
                     sk_download_request_s (
                       pathPlayStation.wstring (),
                         R"(https://sk-data.special-k.info/addon/TalesOfGraces/DualShock4.dds)",
                         []( const std::vector <uint8_t>&&,
                             const std::wstring_view )
                          -> bool
                             {
                               fetching_buttons = false;
                               has_buttons      = true;
                               restart_reqs++;
                         
                               return false;
                             }
                         ),
                     true);
              }

              if (ImGui::Button ("Download " ICON_FA_PLAYSTATION " Icons\t\t DualSense "))
              {
                fetching_buttons = true;

                SK_Network_EnqueueDownload (
                     sk_download_request_s (
                       pathPlayStation.wstring (),
                         R"(https://sk-data.special-k.info/addon/TalesOfGraces/DualSense.dds)",
                         []( const std::vector <uint8_t>&&,
                             const std::wstring_view )
                          -> bool
                             {
                               fetching_buttons = false;
                               has_buttons      = true;
                               restart_reqs++;
                         
                               return false;
                             }
                         ),
                     true);
              }
            }

            else
            {
              ImGui::BulletText ("Downloading New Button Icons...");
            }
          }

          else
          {
            if (ImGui::Button ("Revert to " ICON_FA_XBOX " Icons"))
            {
              if (std::filesystem::remove (pathPlayStation, ec))
              {
                has_buttons = false;
                restart_reqs++;
              }
            }
          }
        }
      }

      ImGui::TreePop  (  );
    }
    ImGui::EndGroup   (  );

#if 0
    if (ImGui::CollapsingHeader (ICON_FA_MAGIC " ThirdParty AddOns", ImGuiTreeNodeFlags_DefaultOpen |
                                                                     ImGuiTreeNodeFlags_AllowOverlap))
    {
      ImGui::TreePush ("");
      
      static bool bHasVersionDLL = 
        PathFileExistsW (L"version.dll");

      if (! bHasVersionDLL)
      {
        ImGui::Button ("Download Tales of Graces f \"Fix\"");
      }

      static bool bHasReShade64DLL = 
        PathFileExistsW (L"ReShade64.dll");

      if (! bHasReShade64DLL)
      {
        ImGui::Button ("Download RenoDX HDR AddOn for ReShade");
      }
      ImGui::TreePop  (  );
    }
#endif

    if (restart_reqs > 0)
    {
      ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (.3f, .8f, .9f).Value);
      ImGui::BulletText     ("Game Restart Required");
      ImGui::PopStyleColor  ();
    }

    ImGui::PopStyleColor (3);
    ImGui::TreePop       ( );
  }

  return true;
}

HRESULT STDMETHODCALLTYPE SK_TGFix_PresentFirstFrame (IUnknown* pSwapChain, UINT SyncInterval, UINT Flags);
void    STDMETHODCALLTYPE SK_TGFix_EndFrame          (void);

#include <SpecialK/render/dxgi/dxgi_hdr.h>

void
SK_TGFix_OnAchievementUnlocked (SK_AchievementManager::Achievement* achv)
{
  std::ignore = achv;

  SK_TGFix_Cfg.achievement_earned.store (true);

  config.utility.save_async ();
}
void
SK_TGFix_InitPlugin (void)
{
  SK_RunOnce (
    SK_TGFix_HookMonoInit ();

    SK_D3D11_DeclHUDShader_Pix (0x1399301e);
    SK_D3D11_DeclHUDShader_Pix (0x171c9bf7);

    SK_TGFix_Cfg.disable_dof.bind_to_ini (
      _CreateConfigParameterBool ( L"TGFix.Render",
                                   L"DisableDepthOfField",  SK_TGFix_Cfg.disable_dof,
                                   L"Disable Depth of Field" )
    );

    SK_TGFix_Cfg.disable_bloom.bind_to_ini (
      _CreateConfigParameterBool ( L"TGFix.Render",
                                   L"DisableBloom",  SK_TGFix_Cfg.disable_bloom,
                                   L"Disable Bloom Lighting" )
    );

    SK_TGFix_Cfg.disable_heat_haze.bind_to_ini (
      _CreateConfigParameterBool ( L"TGFix.Render",
                                   L"DisableHeatHaze",  SK_TGFix_Cfg.disable_heat_haze,
                                   L"Disable Heat Haze" )
    );

    SK_TGFix_Cfg.disable_fxaa.bind_to_ini (
      _CreateConfigParameterBool ( L"TGFix.Render",
                                   L"DisableFXAA",  SK_TGFix_Cfg.disable_fxaa,
                                   L"Disable FXAA" )
    );

    SK_TGFix_Cfg.use_taa.bind_to_ini (
      _CreateConfigParameterBool ( L"TGFix.Render",
                                   L"UseTemporalAA",  SK_TGFix_Cfg.use_taa,
                                   L"Use Temporal AA instead of FXAA" )
    );

    SK_TGFix_Cfg.sharpen_outlines.bind_to_ini (
      _CreateConfigParameterBool ( L"TGFix.Render",
                                   L"SharpenOutlines",  SK_TGFix_Cfg.sharpen_outlines,
                                   L"Sharpen Character Outlines" )
    );

    SK_TGFix_Cfg.msaa_sample_count.bind_to_ini (
      _CreateConfigParameterInt ( L"TGFix.Render",
                                  L"MSAASampleCount",  SK_TGFix_Cfg.msaa_sample_count,
                                  L"Sample count for Multisample Anti-Aliasing" )
    );

    SK_TGFix_Cfg.render_scale.bind_to_ini (
      _CreateConfigParameterFloat ( L"TGFix.Render",
                                    L"RenderScale",  SK_TGFix_Cfg.render_scale,
                                    L"Internal Render Scale (0.1-2.0)" )
    );

    SK_TGFix_Cfg.disable_fog.bind_to_ini (
      _CreateConfigParameterBool  ( L"TGFix.Render",
                                    L"DisableFog",  SK_TGFix_Cfg.disable_fog,
                                    L"Disables partial screen fog effect that is not desirable for HDR or 21:9." )
    );

    SK_TGFix_Cfg.hacks.constant_visibility.bind_to_ini (
      _CreateConfigParameterBool  ( L"TGFix.Render",
                                    L"WidescreenCullingHack",  SK_TGFix_Cfg.hacks.constant_visibility,
                                    L"Ensures objects that are close to off-screen, but not actually off-screen, remain visible." )
    );

    SK_TGFix_Cfg.gamepad_polling_hz.bind_to_ini (
      _CreateConfigParameterFloat ( L"TGFix.Input",
                                    L"PollingFrequency",  SK_TGFix_Cfg.gamepad_polling_hz,
                                    L"Gamepad Polling Frequency (30.0 Hz - 1000.0Hz; 60.0 Hz == hard-coded game default)" )
    );

    SK_TGFix_Cfg.achievement_earned.bind_to_ini(
      _CreateConfigParameterBool  ( L"TGFix.Bugs",
                                    L"UnlockedAnAchievement", SK_TGFix_Cfg.achievement_earned,
                                    L"Keep track of the achievement bug, and only display warnings while it applies." )
    );

    if (SK_TGFix_Cfg.msaa_sample_count > 1)
    {
      // Implicitly disable Depth of Field if MSAA is enabled
      SK_TGFix_Cfg.disable_dof = true;
    }

    if (SK_TGFix_Cfg.disable_dof)
    {
      SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0x54f44c1a);

      InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
    }

    if (SK_TGFix_Cfg.disable_bloom)
    {
      SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0x5bcdb543);
      SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0xa9ca2e76);
      SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0xafcf335b);
      SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0xd70959df);

      InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
      InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
      InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
      InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
    }

    if (SK_TGFix_Cfg.disable_fxaa)
    {
      SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0xbe80dda2);
      SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0xef92e3e1);

      InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
      InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
    }

    if (SK_TGFix_Cfg.sharpen_outlines)
    {
      SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.on_top, 0x2dfcf950);
      SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.on_top, 0x9a0e24eb);
      SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.on_top, 0xf751273e);

      InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
      InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
      InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
    }

    if (SK_TGFix_Cfg.disable_heat_haze)
    {
      SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0x4ea47fea);

      InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
    }


    SK_TGFix_EnableInternalHDR (true);


    plugin_mgr->config_fns.emplace             (SK_TGFix_PlugInCfg);
    plugin_mgr->first_frame_fns.emplace        (SK_TGFix_PresentFirstFrame);
    plugin_mgr->end_frame_fns.emplace          (SK_TGFix_EndFrame);
    plugin_mgr->achievement_unlock_fns.emplace (SK_TGFix_OnAchievementUnlocked);
  );
}

#include <mono/metadata/assembly.h>
#include <mono/metadata/threads.h>
#include <mono/metadata/object.h>

typedef MonoThread*     (*mono_thread_attach_pfn)(MonoDomain* domain);
typedef MonoThread*     (*mono_thread_current_pfn)(void);
typedef void            (*mono_thread_detach_pfn)(MonoThread* thread);
typedef mono_bool       (*mono_thread_is_foreign_pfn)(MonoThread* thread);
typedef MonoDomain*     (*mono_get_root_domain_pfn)(void);
typedef MonoAssembly*   (*mono_domain_assembly_open_pfn)(MonoDomain* doamin, const char* name);
typedef MonoImage*      (*mono_assembly_get_image_pfn)(MonoAssembly* assembly);
typedef MonoString*     (*mono_string_new_pfn)(MonoDomain* domain, const char* text);
typedef MonoObject*     (*mono_object_new_pfn)(MonoDomain* domain, MonoClass* klass);
typedef void*           (*mono_object_unbox_pfn)(MonoObject* obj);
typedef MonoDomain*     (*mono_object_get_domain_pfn)(MonoObject* obj);
typedef MonoClass*      (*mono_object_get_class_pfn)(MonoObject* obj);
typedef MonoClass*      (*mono_class_from_name_pfn)(MonoImage* image, const char* name_space, const char* name);
typedef MonoMethod*     (*mono_class_get_method_from_name_pfn)(MonoClass* klass, const char* name, int param_count);
typedef MonoType*       (*mono_class_get_type_pfn)(MonoClass* klass);
typedef const char*     (*mono_class_get_name_pfn)(MonoClass* klass);
typedef MonoReflectionType*
                        (*mono_type_get_object_pfn)(MonoDomain* domain, MonoType* type);
typedef void*           (*mono_compile_method_pfn)(MonoMethod* method);
typedef MonoObject*     (*mono_runtime_invoke_pfn)(MonoMethod* method, void* obj, void** params, MonoObject** exc);
typedef char*           (*mono_array_addr_with_size_pfn)(MonoArray* array, int size, uintptr_t idx);
typedef uintptr_t       (*mono_array_length_pfn)(MonoArray* array);

typedef MonoClassField* (*mono_class_get_field_from_name_pfn)(MonoClass* klass, const char* name);
typedef void*           (*mono_field_get_value_pfn)(void* obj, MonoClassField* field, void* value);
typedef void            (*mono_field_set_value_pfn)(MonoObject* obj, MonoClassField* field, void* value);
typedef MonoObject*     (*mono_field_get_value_object_pfn)(MonoDomain* domain, MonoClassField* field, MonoObject* obj);
typedef MonoClass*      (*mono_method_get_class_pfn)(MonoMethod* method);
typedef MonoVTable*     (*mono_class_vtable_pfn)(MonoDomain* domain, MonoClass* klass);
typedef void*           (*mono_vtable_get_static_field_data_pfn)(MonoVTable* vt);
typedef uint32_t        (*mono_field_get_offset_pfn)(MonoClassField* field);

typedef MonoMethodDesc* (*mono_method_desc_new_pfn)(const char* name, mono_bool include_namespace);
typedef MonoMethod*     (*mono_method_desc_search_in_image_pfn)(MonoMethodDesc* desc, MonoImage* image);

typedef MonoGCHandle    (*mono_gchandle_new_v2_pfn)(MonoObject* obj, mono_bool pinned);
typedef MonoGCHandle    (*mono_gchandle_free_v2_pfn)(MonoGCHandle gchandle);
typedef MonoObject*     (*mono_gchandle_get_target_v2_pfn)(MonoGCHandle gchandle);

typedef MonoObject*     (*mono_property_get_value_pfn)(MonoProperty *prop, void *obj, void **params, MonoObject **exc);
typedef MonoProperty*   (*mono_class_get_property_from_name_pfn)(MonoClass *klass, const char *name);
typedef MonoMethod*     (*mono_property_get_get_method_pfn)(MonoProperty *prop);
typedef MonoImage*      (*mono_image_loaded_pfn)(const char *name);

#define SK_mono_array_addr(array,type,index) ((type*)SK_mono_array_addr_with_size ((array), sizeof (type), (index)))
#define SK_mono_array_get(array,type,index) (*(type*)SK_mono_array_addr           ((array),         type,  (index)))
#define SK_mono_array_set(array,type,index,value)	                  \
do {                                                                \
  type *__p = (type *) SK_mono_array_addr ((array), type, (index));	\
  *__p = (value);	                                                  \
} while (0)

mono_thread_attach_pfn                SK_mono_thread_attach                = nullptr;
mono_thread_current_pfn               SK_mono_thread_current               = nullptr;
mono_thread_detach_pfn                SK_mono_thread_detach                = nullptr;
mono_thread_is_foreign_pfn            SK_mono_thread_is_foreign            = nullptr;
mono_get_root_domain_pfn              SK_mono_get_root_domain              = nullptr;
mono_domain_assembly_open_pfn         SK_mono_domain_assembly_open         = nullptr;
mono_assembly_get_image_pfn           SK_mono_assembly_get_image           = nullptr;
mono_string_new_pfn                   SK_mono_string_new                   = nullptr;
mono_object_new_pfn                   SK_mono_object_new                   = nullptr;
mono_object_get_domain_pfn            SK_mono_object_get_domain            = nullptr;
mono_object_get_class_pfn             SK_mono_object_get_class             = nullptr;
mono_object_unbox_pfn                 SK_mono_object_unbox                 = nullptr;
mono_class_from_name_pfn              SK_mono_class_from_name              = nullptr;
mono_class_get_method_from_name_pfn   SK_mono_class_get_method_from_name   = nullptr;
mono_class_get_type_pfn               SK_mono_class_get_type               = nullptr;
mono_class_get_name_pfn               SK_mono_class_get_name               = nullptr;
mono_type_get_object_pfn              SK_mono_type_get_object              = nullptr;
mono_compile_method_pfn               SK_mono_compile_method               = nullptr;
mono_runtime_invoke_pfn               SK_mono_runtime_invoke               = nullptr;
mono_array_addr_with_size_pfn         SK_mono_array_addr_with_size         = nullptr;
mono_array_length_pfn                 SK_mono_array_length                 = nullptr;

mono_property_get_value_pfn           SK_mono_property_get_value           = nullptr;
mono_class_get_property_from_name_pfn SK_mono_class_get_property_from_name = nullptr;
mono_property_get_get_method_pfn      SK_mono_property_get_get_method      = nullptr;
mono_property_get_get_method_pfn      SK_mono_property_get_set_method      = nullptr;
mono_image_loaded_pfn                 SK_mono_image_loaded                 = nullptr;

mono_class_get_field_from_name_pfn    SK_mono_class_get_field_from_name    = nullptr;
mono_field_get_value_pfn              SK_mono_field_get_value              = nullptr;
mono_field_set_value_pfn              SK_mono_field_set_value              = nullptr;
mono_field_get_value_object_pfn       SK_mono_field_get_value_object       = nullptr;
mono_method_get_class_pfn             SK_mono_method_get_class             = nullptr;
mono_class_vtable_pfn                 SK_mono_class_vtable                 = nullptr;
mono_vtable_get_static_field_data_pfn SK_mono_vtable_get_static_field_data = nullptr;
mono_field_get_offset_pfn             SK_mono_field_get_offset             = nullptr;

mono_gchandle_new_v2_pfn              SK_mono_gchandle_new_v2              = nullptr;
mono_gchandle_free_v2_pfn             SK_mono_gchandle_free_v2             = nullptr;
mono_gchandle_get_target_v2_pfn       SK_mono_gchandle_get_target_v2       = nullptr;

mono_method_desc_new_pfn              SK_mono_method_desc_new              = nullptr;
mono_method_desc_search_in_image_pfn  SK_mono_method_desc_search_in_image  = nullptr;

MonoDomain* SK_TGFix_MonoDomain = nullptr;

using mono_jit_init_pfn              = MonoDomain* (*)(const char *file);
      mono_jit_init_pfn
      mono_jit_init_Original         = nullptr;
using mono_jit_init_version_pfn      = MonoDomain* (*)(const char *root_domain_name, const char *runtime_version);
      mono_jit_init_version_pfn
      mono_jit_init_version_Original = nullptr;

void SK_TGFix_OnInitMono (MonoDomain* domain);

MonoDomain*
mono_jit_init_Detour (const char *file)
{
  SK_LOG_FIRST_CALL

  auto domain =
    mono_jit_init_Original (file);

  if (domain != nullptr)
  {
    SK_TGFix_OnInitMono (domain);
  }

  return domain;
}

MonoDomain*
mono_jit_init_version_Detour (const char *root_domain_name, const char *runtime_version)
{
  SK_LOG_FIRST_CALL

  auto domain =
    mono_jit_init_version_Original (root_domain_name, runtime_version);

  if (domain != nullptr)
  {
    SK_TGFix_OnInitMono (domain);
  }

  return domain;
}

void AttachThread (void)
{
  SK_LOGi1 (L"Attaching Mono to Thread: %x", GetCurrentThreadId ());

  SK_mono_thread_attach (SK_TGFix_MonoDomain);
}

bool DetachCurrentThreadIfNotNative (void)
{
  MonoThread* this_thread =
    SK_mono_thread_current ();

  if (this_thread != nullptr)
  {
    if (SK_mono_thread_is_foreign (this_thread))
    {
      SK_mono_thread_detach (this_thread);

      SK_LOGi1 (L"Detached Non-Native Mono Thread: tid=%x", GetCurrentThreadId ());

      return true;
    }
  }

  return false;
}

static constexpr wchar_t* mono_dll  = L"mono-2.0-bdwgc.dll";
static constexpr wchar_t* mono_path = LR"(MonoBleedingEdge\EmbedRuntime\mono-2.0-bdwgc.dll)";

bool
SK_TGFix_HookMonoInit (void)
{
  SK_LoadLibraryW (mono_path);

  HMODULE hMono =
    GetModuleHandleW (mono_dll);

  if (hMono == NULL)
    return false;

  SK_mono_domain_assembly_open         = reinterpret_cast <mono_domain_assembly_open_pfn>         (SK_GetProcAddress (hMono, "mono_domain_assembly_open"));
  SK_mono_assembly_get_image           = reinterpret_cast <mono_assembly_get_image_pfn>           (SK_GetProcAddress (hMono, "mono_assembly_get_image"));
  SK_mono_class_from_name              = reinterpret_cast <mono_class_from_name_pfn>              (SK_GetProcAddress (hMono, "mono_class_from_name"));
  SK_mono_class_get_method_from_name   = reinterpret_cast <mono_class_get_method_from_name_pfn>   (SK_GetProcAddress (hMono, "mono_class_get_method_from_name"));
  SK_mono_class_get_type               = reinterpret_cast <mono_class_get_type_pfn>               (SK_GetProcAddress (hMono, "mono_class_get_type"));
  SK_mono_class_get_name               = reinterpret_cast <mono_class_get_name_pfn>               (SK_GetProcAddress (hMono, "mono_class_get_name"));
  SK_mono_type_get_object              = reinterpret_cast <mono_type_get_object_pfn>              (SK_GetProcAddress (hMono, "mono_type_get_object"));
  SK_mono_compile_method               = reinterpret_cast <mono_compile_method_pfn>               (SK_GetProcAddress (hMono, "mono_compile_method"));
  SK_mono_runtime_invoke               = reinterpret_cast <mono_runtime_invoke_pfn>               (SK_GetProcAddress (hMono, "mono_runtime_invoke"));
  SK_mono_array_addr_with_size         = reinterpret_cast <mono_array_addr_with_size_pfn>         (SK_GetProcAddress (hMono, "mono_array_addr_with_size"));
  SK_mono_array_length                 = reinterpret_cast <mono_array_length_pfn>                 (SK_GetProcAddress (hMono, "mono_array_length"));

  SK_mono_class_get_field_from_name    = reinterpret_cast <mono_class_get_field_from_name_pfn>    (SK_GetProcAddress (hMono, "mono_class_get_field_from_name"));
  SK_mono_field_get_value              = reinterpret_cast <mono_field_get_value_pfn>              (SK_GetProcAddress (hMono, "mono_field_get_value"));
  SK_mono_field_set_value              = reinterpret_cast <mono_field_set_value_pfn>              (SK_GetProcAddress (hMono, "mono_field_set_value"));
  SK_mono_field_get_value_object       = reinterpret_cast <mono_field_get_value_object_pfn>       (SK_GetProcAddress (hMono, "mono_field_get_value_object"));
  SK_mono_method_get_class             = reinterpret_cast <mono_method_get_class_pfn>             (SK_GetProcAddress (hMono, "mono_method_get_class"));
  SK_mono_class_vtable                 = reinterpret_cast <mono_class_vtable_pfn>                 (SK_GetProcAddress (hMono, "mono_class_vtable"));
  SK_mono_vtable_get_static_field_data = reinterpret_cast <mono_vtable_get_static_field_data_pfn> (SK_GetProcAddress (hMono, "mono_vtable_get_static_field_data"));
  SK_mono_field_get_offset             = reinterpret_cast <mono_field_get_offset_pfn>             (SK_GetProcAddress (hMono, "mono_field_get_offset"));

  SK_mono_property_get_value           = reinterpret_cast <mono_property_get_value_pfn>           (SK_GetProcAddress (hMono, "mono_property_get_value"));
  SK_mono_class_get_property_from_name = reinterpret_cast <mono_class_get_property_from_name_pfn> (SK_GetProcAddress (hMono, "mono_class_get_property_from_name"));
  SK_mono_property_get_get_method      = reinterpret_cast <mono_property_get_get_method_pfn>      (SK_GetProcAddress (hMono, "mono_property_get_get_method"));
  SK_mono_property_get_set_method      = reinterpret_cast <mono_property_get_get_method_pfn>      (SK_GetProcAddress (hMono, "mono_property_get_set_method"));
  SK_mono_image_loaded                 = reinterpret_cast <mono_image_loaded_pfn>                 (SK_GetProcAddress (hMono, "mono_image_loaded"));
  SK_mono_string_new                   = reinterpret_cast <mono_string_new_pfn>                   (SK_GetProcAddress (hMono, "mono_string_new"));
  SK_mono_object_new                   = reinterpret_cast <mono_object_new_pfn>                   (SK_GetProcAddress (hMono, "mono_object_new"));
  SK_mono_object_get_domain            = reinterpret_cast <mono_object_get_domain_pfn>            (SK_GetProcAddress (hMono, "mono_object_get_domain"));
  SK_mono_object_get_class             = reinterpret_cast <mono_object_get_class_pfn>             (SK_GetProcAddress (hMono, "mono_object_get_class"));
  SK_mono_object_unbox                 = reinterpret_cast <mono_object_unbox_pfn>                 (SK_GetProcAddress (hMono, "mono_object_unbox"));

  SK_mono_method_desc_new              = reinterpret_cast <mono_method_desc_new_pfn>              (SK_GetProcAddress (hMono, "mono_method_desc_new"));
  SK_mono_method_desc_search_in_image  = reinterpret_cast <mono_method_desc_search_in_image_pfn>  (SK_GetProcAddress (hMono, "mono_method_desc_search_in_image"));

  SK_mono_gchandle_new_v2              = reinterpret_cast <mono_gchandle_new_v2_pfn>              (SK_GetProcAddress (hMono, "SK_mono_gchandle_new_v2"));
  SK_mono_gchandle_free_v2             = reinterpret_cast <mono_gchandle_free_v2_pfn>             (SK_GetProcAddress (hMono, "SK_mono_gchandle_free_v2"));
  SK_mono_gchandle_get_target_v2       = reinterpret_cast <mono_gchandle_get_target_v2_pfn>       (SK_GetProcAddress (hMono, "SK_mono_gchandle_get_target_v2"));

  SK_mono_thread_attach                = reinterpret_cast <mono_thread_attach_pfn>                (SK_GetProcAddress (hMono, "mono_thread_attach"));
  SK_mono_get_root_domain              = reinterpret_cast <mono_get_root_domain_pfn>              (SK_GetProcAddress (hMono, "mono_get_root_domain"));
  SK_mono_thread_current               = reinterpret_cast <mono_thread_current_pfn>               (SK_GetProcAddress (hMono, "mono_thread_current"));
  SK_mono_thread_detach                = reinterpret_cast <mono_thread_detach_pfn>                (SK_GetProcAddress (hMono, "mono_thread_detach"));
  SK_mono_thread_is_foreign            = reinterpret_cast <mono_thread_is_foreign_pfn>            (SK_GetProcAddress (hMono, "mono_thread_is_foreign"));

  void*                   pfnMonoJitInitVersion = nullptr;
  SK_CreateDLLHook (         mono_dll,
                            "mono_jit_init_version",
                             mono_jit_init_version_Detour,
    static_cast_p2p <void> (&mono_jit_init_version_Original),
                           &pfnMonoJitInitVersion );
  SK_EnableHook    (        pfnMonoJitInitVersion );

  // If this was pre-loaded, then the above hooks never run and we should initialize everything immediately...
  if (SK_GetModuleHandleW (L"BepInEx.Core.dll"))
  {
    if ( nullptr !=        SK_mono_get_root_domain )
      SK_TGFix_OnInitMono (SK_mono_get_root_domain ());
  }


  return true;
}

void SK_TGFix_OnInitMono (MonoDomain* domain)
{
  if ((! domain) || SK_mono_thread_attach == nullptr)
    return;

  static bool
        init = false;
  if (! init)
  {
    SK_TGFix_MonoDomain = domain;

    SK_ReleaseAssert (domain == SK_mono_get_root_domain ());

    init = SK_TGFix_SetupFramerateHooks ();
  }
}

MonoObject* SK_TGFix_PrimitiveManagerSingleton = nullptr;
MonoObject* SK_TGFix_CameraManagerSingleton    = nullptr;
MonoObject* SK_TGFix_FrameRateManagerSingleton = nullptr;
float       SK_TGFix_LastSetFrameRateLimit     = 0.0f;

struct {
  MonoImage* assemblyCSharp            = nullptr;
  MonoImage* UnityEngine_CoreModule    = nullptr;
} SK_TGFix_MonoAssemblies;

struct {
  struct {
    struct {
      MonoClassField*   m_Viewport      = nullptr;
      struct {
        MonoClassField* type            = nullptr;
      } PRIM_PARAM;
      struct {
        MonoClassField* db              = nullptr;
      } PRIM_TYPE;
    } PrimitiveManager;
    struct {
      MonoClassField*   vertexCount     = nullptr;
      MonoClassField*   m_PrimitiveType = nullptr;
    } ObjPrimitiveBase;
    struct {
      MonoClassField*   cameraview      = nullptr;
    } NobleMovieRendererPass;
  } Noble;
  struct {
    MonoClassField*     mRenderShadowParamUseNum = nullptr;
    MonoClassField*     mRenderShadowParam       = nullptr;
  } NobleLegacyShadowRenderParameter;
} SK_TGFix_MonoFields;

struct {
  struct {
    struct {
      MonoMethod* SetVisibilityFlags     = nullptr;
      MonoMethod* IsDirty                = nullptr;
      MonoMethod* IsVisible              = nullptr;
    } Object;
    struct {
      MonoMethod* SetCameraAspect        = nullptr;
      MonoMethod* SetCameraViewportRect  = nullptr;
    } CameraManager;
    struct {
      MonoMethod* CalcUIOrthoMatrix      = nullptr;
    } PrimitiveManager;
  } Noble;
  struct {
    MonoMethod*   get_mLegacyShadowParam = nullptr;
  } NoblePostEffectLegacyShadowParam;
} SK_TGFix_MonoMethods;

HRESULT
STDMETHODCALLTYPE
SK_TGFix_PresentFirstFrame (IUnknown* pSwapChain, UINT SyncInterval, UINT Flags)
{
  UNREFERENCED_PARAMETER (pSwapChain);
  UNREFERENCED_PARAMETER (SyncInterval);
  UNREFERENCED_PARAMETER (Flags);

  // Better late initialization than never...
  if ( nullptr !=        SK_mono_get_root_domain )
    SK_TGFix_OnInitMono (SK_mono_get_root_domain ());

  if (SK_TGFix_Cfg.msaa_sample_count > 1)
  {
    SK_TGFix_SetMSAASampleCount (SK_TGFix_Cfg.msaa_sample_count);
  }

  if (SK_TGFix_Cfg.render_scale != 1.0f)
  {
    SK_TGFix_SetRenderScale (SK_TGFix_Cfg.render_scale);
  }

  if (SK_TGFix_Cfg.use_taa)
  {
    SK_TGFix_SetCameraAA ();
  }

  if (SK_TGFix_Cfg.gamepad_polling_hz != 60.0f)
  {
    SK_TGFix_SetInputPollingFreq (SK_TGFix_Cfg.gamepad_polling_hz);
  }

  SK_TGFix_EnableInternalHDR (true);

  return S_OK;
}

//
// Various poorly-written examples on using Mono embedded framework from:
//    https://www.unknowncheats.me/forum/unity/603179-hacking-mono-games.html
// 
//  This code all needs to be re-written if it is to be useful in future projects;
//    it is terrible :)
//
bool LoadMonoAssembly (const char* assemblyName)
{
  MonoImage* pImage =
    SK_mono_image_loaded (assemblyName);

  if (pImage != nullptr)
    return true;

  MonoDomain* pDomain =
    SK_TGFix_MonoDomain;

  if (pDomain == nullptr)
    return false;
 
  MonoAssembly* pAssembly =
    SK_mono_domain_assembly_open (pDomain, assemblyName);

  if (pAssembly == nullptr)
    return false;
 
  pImage =
    SK_mono_assembly_get_image (pAssembly);

  return
    (pImage != nullptr);
}

void* GetCompiledMethod (const char* nameSpace, const char* className, const char* methodName, int param_count = 0, const char* assemblyName = "Assembly-CSharp")
{
  MonoImage* pImage =
    SK_mono_image_loaded (assemblyName);

  if (! pImage)
  {
    MonoDomain* pDomain =
      SK_TGFix_MonoDomain;

    if (pDomain == nullptr)
      return nullptr;
 
    MonoAssembly* pAssembly =
      SK_mono_domain_assembly_open (pDomain, assemblyName);

    if (pAssembly == nullptr)
      return nullptr;
 
    pImage =
      SK_mono_assembly_get_image (pAssembly);

    if (pImage == nullptr)
      return nullptr;
  }
 
  MonoClass* pKlass =
    SK_mono_class_from_name (pImage, nameSpace != nullptr ? nameSpace : "", className);

  if (pKlass == nullptr)
    return nullptr;
 
  MonoMethod* pMethod =
    SK_mono_class_get_method_from_name (pKlass, methodName, param_count);

  if (pMethod == nullptr)
    return nullptr;
 
  return
    SK_mono_compile_method (pMethod);
}

MonoMethod* GetMethod (const char* className, const char* methodName, int param_count = 0, const char* assemblyName = "Assembly-CSharp", const char* nameSpace = "")
{
  MonoImage* pImage =
    SK_mono_image_loaded (assemblyName);

  if (! pImage)
  {
    MonoDomain* pDomain =
      SK_TGFix_MonoDomain;

    if (pDomain == nullptr)
      return nullptr;
 
    MonoAssembly* pAssembly =
      SK_mono_domain_assembly_open (pDomain, assemblyName);

    if (pAssembly == nullptr)
      return nullptr;
 
    pImage =
      SK_mono_assembly_get_image (pAssembly);

    if (pImage == nullptr)
      return nullptr;
  }
 
  MonoClass* pKlass =
    SK_mono_class_from_name (pImage, nameSpace, className);

  if (pKlass == nullptr)
    return nullptr;
 
  return
    SK_mono_class_get_method_from_name (pKlass, methodName, param_count);
}

MonoClass* GetClass (const char* className, const char* assemblyName = "Assembly-CSharp", const char* nameSpace = "")
{
  MonoImage* pImage =
    SK_mono_image_loaded (assemblyName);

  if (! pImage)
  {
    MonoDomain* pDomain =
      SK_TGFix_MonoDomain;

    if (pDomain == nullptr)
      return nullptr;
 
    MonoAssembly* pAssembly =
      SK_mono_domain_assembly_open (pDomain, assemblyName);

    if (pAssembly == nullptr)
      return nullptr;
 
    pImage =
      SK_mono_assembly_get_image (pAssembly);

    if (pImage == nullptr)
      return nullptr;
  }
 
  MonoClass* pKlass =
    SK_mono_class_from_name (pImage, nameSpace, className);

  return pKlass;
}

MonoClass* GetClassFromMethod (MonoMethod* method)
{
  return
    SK_mono_method_get_class (method);
}

MonoClassField* GetField (const char* className, const char* fieldName, const char* assemblyName = "Assembly-CSharp", const char* nameSpace = "")
{
  MonoImage* pImage =
    SK_mono_image_loaded (assemblyName);

  if (! pImage)
  {
    MonoDomain* pDomain =
      SK_TGFix_MonoDomain;

    if (pDomain == nullptr)
      return nullptr;
 
    MonoAssembly* pAssembly =
      SK_mono_domain_assembly_open (pDomain, assemblyName);

    if (pAssembly == nullptr)
      return nullptr;
 
    pImage =
      SK_mono_assembly_get_image (pAssembly);

    if (pImage == nullptr)
      return nullptr;
  }
 
  MonoClass* pKlass =
    SK_mono_class_from_name (pImage, nameSpace, className);

  if (pKlass == nullptr)
    return nullptr;
 
  MonoClassField* pField =
    SK_mono_class_get_field_from_name (pKlass, fieldName);

  return pField;
}

// Helpers for GetStaticFieldValue
MonoClassField* GetField (MonoClass* pKlass, const char* fieldName)
{
  MonoClassField* pField =
    SK_mono_class_get_field_from_name (pKlass, fieldName);

  return pField;
}

uint32_t GetFieldOffset (MonoClassField* field)
{
  return
    SK_mono_field_get_offset (field);
}

void GetFieldValue (MonoObject* obj, MonoClassField* field, void* out)
{
  AttachThread ();

  SK_mono_field_get_value (obj, field, out);
}

void SetFieldValue (MonoObject* obj, MonoClassField* field, void* value)
{
  SK_mono_field_set_value (obj, field, value);
}

MonoVTable* GetVTable (MonoClass* pKlass)
{
  return
    SK_mono_class_vtable (SK_TGFix_MonoDomain, pKlass);
}

void* GetStaticFieldData (MonoVTable* pVTable)
{
  return
    SK_mono_vtable_get_static_field_data (pVTable);
}

void* GetStaticFieldData (MonoClass* pKlass)
{
  MonoVTable* pVTable =
    GetVTable (pKlass);

  if (pVTable == nullptr)
    return nullptr;
 
  return
    SK_mono_vtable_get_static_field_data (pVTable);
}

void* GetStaticFieldValue (const char* className, const char* fieldName, const char* assemblyName = "Assembly-CSharp", const char* nameSpace = "")
{
  MonoClass* pKlass =
    GetClass (className, assemblyName, nameSpace);

  if (pKlass == nullptr)
    return nullptr;
 
  MonoClassField* pField =
    GetField (pKlass, fieldName);

  if (pField == nullptr)
    return nullptr;
 
  DWORD_PTR addr   = (DWORD_PTR)GetStaticFieldData (pKlass);
  uint32_t  offset =            GetFieldOffset     (pField);
 
  void* value = (void *)(addr + offset);

  return value;
}

MonoImage* GetImage (const char* name)
{
  return
    SK_mono_image_loaded (name);
}

MonoClass* GetClass (MonoImage* image, const char* _namespace, const char* _class)
{
  return
    SK_mono_class_from_name (image, _namespace, _class);
}

MonoMethod* GetClassMethod (MonoClass* class_, const char* name, int params)
{
  return
    SK_mono_class_get_method_from_name (class_, name, params);
}

MonoMethod* GetClassMethod (MonoImage* image, const char* namespace_, const char* class_, const char* name, int params)
{
  MonoClass* hClass =
    GetClass (image, namespace_, class_);

  return hClass ? GetClassMethod (hClass, name, params) : NULL;
}

MonoMethod* GetClassMethod (const char* namespace_, const char* class_, const char* name, int params, const char* image_ = "Assembly-CSharp")
{
  MonoClass* hClass = GetClass (GetImage (image_), namespace_, class_);
  return     hClass ? GetClassMethod (hClass, name, params) : NULL;
}

MonoProperty* GetProperty (MonoImage* image, const char* namespace_, const char* class_, const char* name)
{
  return
    SK_mono_class_get_property_from_name (GetClass (image, namespace_, class_), name);
}
 
MonoObject* GetPropertyValue (MonoProperty* property, uintptr_t instance = 0)
{
  return
    SK_mono_property_get_value (property, reinterpret_cast <void *>(instance), nullptr, 0);
}

MonoObject* GetPropertyValue (MonoImage* image, const char* namespace_, const char* class_, const char* name, uintptr_t instance = 0)
{
  return
    GetPropertyValue (GetProperty (image, namespace_, class_, name), instance);
}

MonoObject* InvokeMethod (const char* namespace_, const char* class_, const char* method, int paramsCount, void* instance, const char* image_ = "Assembly-CSharp", void** params = nullptr)
{
  if (instance == nullptr)
           return nullptr;

  AttachThread ();

  return
    SK_mono_runtime_invoke (GetClassMethod (GetImage (image_), namespace_, class_, method, paramsCount), instance, params, nullptr);
}
 
void* CompileMethod (MonoMethod* method)
{
  return
    SK_mono_compile_method (method);
}
 
void* CompileMethod (const char* namespace_, const char* class_, const char* name, int params, const char* image_ = "Assembly-CSharp")
{
  return
    CompileMethod (GetClassMethod (GetImage (image_), namespace_, class_, name, params));
}

MonoObject* NewObject (MonoClass* klass)
{
  SK_mono_thread_attach (SK_TGFix_MonoDomain);

  return
    SK_mono_object_new (SK_TGFix_MonoDomain, klass);
}

MonoObject* Invoke (MonoMethod* method, void* obj, void** params)
{
  SK_mono_thread_attach (SK_TGFix_MonoDomain);
 
  MonoObject* exc;

  return
    SK_mono_runtime_invoke (method, obj, params, &exc);
}

MonoObject* ConstructNewObject (MonoClass* klass, int num_args, void** args)
{
  auto ctor =
    SK_mono_class_get_method_from_name (klass, ".ctor", num_args);

  MonoObject*
      instance = NewObject (klass);
  if (instance != nullptr)
  {
    return
      SK_mono_runtime_invoke (ctor, instance, args, nullptr);
  }

  return nullptr;
}

MonoObject* ConstructObject (MonoClass* klass, MonoObject* instance, int num_args, void** args)
{
  auto ctor =
    SK_mono_class_get_method_from_name (klass, ".ctor", num_args);

  if (instance != nullptr)
  {
    return
      SK_mono_runtime_invoke (ctor, instance, args, nullptr);
  }

  return nullptr;
}

using NobleFrameRateManager_SetQualitySettingFrameRate_pfn = void (*)(MonoObject*, float);
using NobleFrameRateManager_SetTargetFrameRate_pfn         = void (*)(MonoObject*, float);

NobleFrameRateManager_SetQualitySettingFrameRate_pfn NobleFrameRateManager_SetQualitySettingFrameRate_Original = nullptr;
NobleFrameRateManager_SetTargetFrameRate_pfn         NobleFrameRateManager_SetTargetFrameRate_Original         = nullptr;

void
SK_TGFix_NobleFrameRateManager_SetQualitySettingFrameRate_Detour (MonoObject* __this, float rate)
{
  SK_LOG_FIRST_CALL

  SK_TGFix_FrameRateManagerSingleton = __this;

#if 0
  auto& rb =
    SK_GetCurrentRenderBackend ();

  const float fActiveRefresh =
    static_cast <float> (rb.displays [rb.active_display].signal.timing.vsync_freq.Numerator) /
    static_cast <float> (rb.displays [rb.active_display].signal.timing.vsync_freq.Denominator);

  if (__target_fps > 0.0f || fActiveRefresh >= 120.0f)
  {
    if (__target_fps <= 0.0f)
    {
      SK_TGFix_LastSetFrameRateLimit = fActiveRefresh;

      return
        NobleFrameRateManager_SetQualitySettingFrameRate_Original (__this, fActiveRefresh * 1.008f);
    }

    SK_TGFix_LastSetFrameRateLimit = __target_fps;

    return
      NobleFrameRateManager_SetQualitySettingFrameRate_Original (__this, __target_fps * 1.01f);
  }
#else
  //SK_TGFix_LastSetFrameRateLimit = rate;
  rate                           = 10000.0f;
#endif

  return
    NobleFrameRateManager_SetQualitySettingFrameRate_Original (__this, rate);
}

void
SK_TGFix_NobleFrameRateManager_SetTargetFrameRate_Detour (MonoObject* __this, float rate)
{
  SK_LOG_FIRST_CALL

  SK_TGFix_FrameRateManagerSingleton = __this;

#if 0
  auto& rb =
    SK_GetCurrentRenderBackend ();

  const float fActiveRefresh =
    static_cast <float> (rb.displays [rb.active_display].signal.timing.vsync_freq.Numerator) /
    static_cast <float> (rb.displays [rb.active_display].signal.timing.vsync_freq.Denominator);

  if (__target_fps > 0.0f || fActiveRefresh >= 120.0f)
  {
    if (__target_fps <= 0.0f)
    {
      SK_TGFix_LastSetFrameRateLimit = fActiveRefresh;

      return
        NobleFrameRateManager_SetTargetFrameRate_Original (__this, fActiveRefresh * 1.008f);
    }

    SK_TGFix_LastSetFrameRateLimit = __target_fps;

    return
      NobleFrameRateManager_SetTargetFrameRate_Original (__this, __target_fps * 1.01f);
  }
#else
  //SK_TGFix_LastSetFrameRateLimit = rate;
  rate                           = 10000.0f;
#endif

  return
    NobleFrameRateManager_SetTargetFrameRate_Original (__this, rate);
}

using UnityEngine_Screen_SetResolution_pfn = void (*)(int, int, int);
      UnityEngine_Screen_SetResolution_pfn
      UnityEngine_Screen_SetResolution_Original = nullptr;

void
SK_TGFix_UnityEngine_Screen_SetResolution_Detour (int width, int height, int fullscreen)
{
  SK_LOG_FIRST_CALL

  SK_TGFix_ScreenWidth  = width;
  SK_TGFix_ScreenHeight = height;

  SK_TGFix_AspectRatio      = (float)width / (float)height;
  SK_TGFix_AspectMultiplier = SK_TGFix_AspectRatio / SK_TGFix_NativeAspect;

  SK_LOGi0 ("Current Resolution: %dx%d",                 width, height);
  SK_LOGi0 ("Current Resolution: Aspect Ratio: %f",      SK_TGFix_AspectRatio);
  SK_LOGi0 ("Current Resolution: Aspect Multiplier: %f", SK_TGFix_AspectMultiplier);

  UnityEngine_Screen_SetResolution_Original (width, height, fullscreen);
}

struct Unity_Rect
{
  float x;
  float y;
  float width;
  float height;
};

struct Unity_Color
{
  float r;
  float g;
  float b;
  float a;
};

struct Unity_Vector2
{
  float x;
  float y;
};

struct Unity_Vector4
{
  float x;
  float y;
  float z;
  float w;
};

struct Unity_Matrix4x4
{
  float m00; float m10; float m20; float m30;
  float m01; float m11; float m21; float m31;
  float m02; float m12; float m22; float m32;
  float m03; float m13; float m23; float m33;
};

struct Noble_LegacyShadowRenderParam
{
  Unity_Matrix4x4 mMatrix;
  Unity_Color     mColor;
  Unity_Vector2   mParam;
  Unity_Vector2   mClipZ;
  Unity_Vector4   mScissor;
  uint16_t        mShadowTextureIndex;
  uint16_t        mShadowRenderingLayerNo;
};

using Noble_CameraManager_SetCameraViewportRect_pfn  = void (*)(MonoObject*, Unity_Rect*);
using Noble_CameraManager_SetCameraAspect_pfn        = void (*)(MonoObject*, float aspect);
using Noble_CameraManager_SetBackGoundColor_pfn      = void (*)(MonoObject*, MonoObject*);
using Noble_CameraManager_OnPostNativeGameUpdate_pfn = void (*)(MonoObject*);

Noble_CameraManager_SetCameraViewportRect_pfn  Noble_CameraManager_SetCameraViewportRect_Original  = nullptr;
Noble_CameraManager_SetCameraAspect_pfn        Noble_CameraManager_SetCameraAspect_Original        = nullptr;
Noble_CameraManager_SetBackGoundColor_pfn      Noble_CameraManager_SetBackGoundColor_Original      = nullptr;
Noble_CameraManager_OnPostNativeGameUpdate_pfn Noble_CameraManager_OnPostNativeGameUpdate_Original = nullptr;

void
SK_TGFix_Noble_CameraManager_SetCameraViewportRect_Detour (MonoObject* __this, Unity_Rect* rect)
{
  SK_LOG_FIRST_CALL

  SK_TGFix_CameraManagerSingleton = __this;

  if (SK_TGFix_AspectRatio != SK_TGFix_NativeAspect && SK_TGFix_AspectRatio != 0.0f)
  {
    rect->width  = 1.0f;
    rect->height = 1.0f;
    rect->x      = 0.0f;
    rect->y      = 0.0f;
  }

  Noble_CameraManager_SetCameraViewportRect_Original (__this, rect);
}

void
SK_TGFix_Noble_CameraManager_SetCameraAspect_Detour (MonoObject* __this, float aspect)
{
  SK_LOG_FIRST_CALL

  SK_TGFix_CameraManagerSingleton = __this;

  if (SK_TGFix_AspectRatio != SK_TGFix_NativeAspect && SK_TGFix_AspectRatio != 0.0f)
  {
    return
      Noble_CameraManager_SetCameraAspect_Original (__this, SK_TGFix_AspectRatio);
  }

  Noble_CameraManager_SetCameraAspect_Original (__this, aspect);
}

template <typename _T>
std::optional     <_T>
SK_Mono_InvokeAndUnbox (MonoMethod* method, MonoObject* obj, void** params, MonoObject** exc = nullptr)
{
  if (method != nullptr && obj != nullptr)
  {
    const auto result =
      SK_mono_runtime_invoke (method, obj, params, exc);

    if (result != nullptr)
    {
      _T* unboxed =
        static_cast <_T*> (SK_mono_object_unbox (result));

      if (unboxed != nullptr)
        return *(_T *)unboxed;
    }
  }

  return
    std::nullopt;
}

void
SK_TGFix_Noble_CameraManager_SetBackGoundColor_Detour (MonoObject* __this, MonoObject* color)
{
  SK_LOG_FIRST_CALL

  std::ignore = color;

  SK_TGFix_CameraManagerSingleton = __this;
}

using Noble_Object_ApplyCachedParameters_pfn = void (*)(MonoObject*);
      Noble_Object_ApplyCachedParameters_pfn
      Noble_Object_ApplyCachedParameters_Original = nullptr;

void
SK_TGFix_Noble_Object_ApplyCachedParameters_Detour (MonoObject* __this)
{
  SK_LOG_FIRST_CALL

  if (SK_TGFix_Cfg.hacks.constant_visibility && SK_TGFix_AspectRatio != SK_TGFix_NativeAspect && SK_TGFix_AspectRatio != 0.0f)
  {
    const auto IsDirty            = SK_TGFix_MonoMethods.Noble.Object.IsDirty;
    const auto IsVisible          = SK_TGFix_MonoMethods.Noble.Object.IsVisible;
    const auto SetVisibilityFlags = SK_TGFix_MonoMethods.Noble.Object.SetVisibilityFlags;

    int  flag = 0x1; // kObjectVisible
    bool set  = true;

    void* params [2] = { &flag, &set };

    bool is_dirty =                           SK_Mono_InvokeAndUnbox <bool> (IsDirty,   __this, params).value_or (false);
    if (!is_dirty &&                          SK_Mono_InvokeAndUnbox <bool> (IsVisible, __this, params).value_or (false))
{                                                                                                 flag = 0x2;
      const bool bCachedRenderRequest       = SK_Mono_InvokeAndUnbox <bool> (IsVisible, __this, params).value_or (false);
                                                                                                  flag = 0x4;
      const bool bCurrentFrameRenderRequest = SK_Mono_InvokeAndUnbox <bool> (IsVisible, __this, params).value_or (false);

      is_dirty = bCachedRenderRequest != bCurrentFrameRenderRequest;
    }

    if (is_dirty)
    {
      flag = 0x5; // kObjectVisible | kCurrentFrameRenderRequest

      const bool is_visible =
        SK_Mono_InvokeAndUnbox <bool> (IsVisible, __this, params).value_or (false);
      
      if (is_visible)
      {
        flag = 0x7; // kObjectVisible | kCachedRenderRequest
                    //                | kCurrentFrameRenderRequest

        SK_mono_runtime_invoke (SetVisibilityFlags, __this, params, nullptr);
      }
    }
  }

  Noble_Object_ApplyCachedParameters_Original (__this);
}


using Noble_PrimitiveManager_CalcUIOrthoMatrix_pfn = void (*)(MonoObject*, bool forceproc);
      Noble_PrimitiveManager_CalcUIOrthoMatrix_pfn
      Noble_PrimitiveManager_CalcUIOrthoMatrix_Original = nullptr;

using Noble_PrimitiveManager_OnBeginUpdateNativeGameMain_pfn = void (*)(MonoObject*);
      Noble_PrimitiveManager_OnBeginUpdateNativeGameMain_pfn
      Noble_PrimitiveManager_OnBeginUpdateNativeGameMain_Original = nullptr;

using Noble_PrimitiveManager_OnEndUpdateNativeGameMain_pfn = void (*)(MonoObject*);
      Noble_PrimitiveManager_OnEndUpdateNativeGameMain_pfn
      Noble_PrimitiveManager_OnEndUpdateNativeGameMain_Original = nullptr;

bool SK_TGFix_LastCalcUIOrthoMatrixWasForced = false;

void
SK_TGFix_Noble_PrimitiveManager_CalcUIOrthoMatrix_Detour (MonoObject* __this, bool forceproc)
{
  SK_LOG_FIRST_CALL

  SK_TGFix_PrimitiveManagerSingleton = __this;

  Noble_PrimitiveManager_CalcUIOrthoMatrix_Original (__this, forceproc);

  SK_TGFix_LastCalcUIOrthoMatrixWasForced = forceproc;
}

void
SK_TGFix_Noble_PrimitiveManager_OnBeginUpdateNativeGameMain_Detour (MonoObject* __this)
{
  SK_LOG_FIRST_CALL

  SK_TGFix_PrimitiveManagerSingleton = __this;

  Noble_PrimitiveManager_OnBeginUpdateNativeGameMain_Original (__this);

  auto cameraManager  = SK_TGFix_CameraManagerSingleton;
  if ( cameraManager != nullptr )
  {
    const auto SetCameraAspect       = SK_TGFix_MonoMethods.Noble.CameraManager.SetCameraAspect;
    const auto SetCameraViewportRect = SK_TGFix_MonoMethods.Noble.CameraManager.SetCameraViewportRect;

    void* aspect_args [] = { &SK_TGFix_AspectRatio };

    SK_mono_runtime_invoke (SetCameraAspect, cameraManager, aspect_args, nullptr);

    float              value = 1.0f;
    void* args [] = { &value };

    Unity_Rect _rect =
      { 0.0f, 0.0f,
        1.0f, 1.0f };

    args [0] = { &_rect };

    SK_mono_runtime_invoke (SetCameraViewportRect, cameraManager, args, nullptr);

    auto primitiveManager = SK_TGFix_PrimitiveManagerSingleton;
    if ( primitiveManager != nullptr )
    {
      const auto CalcUIOrthoMatrix = SK_TGFix_MonoMethods.Noble.PrimitiveManager.CalcUIOrthoMatrix;
      const auto m_Viewport        = SK_TGFix_MonoFields.Noble.PrimitiveManager.m_Viewport;

      bool          forceproc = SK_TGFix_LastCalcUIOrthoMatrixWasForced;
      args [0] = { &forceproc };

      SK_mono_runtime_invoke (CalcUIOrthoMatrix, primitiveManager, args, nullptr);

      SK_TGFix_LastCalcUIOrthoMatrixWasForced = false;

      const float width  =
        (SK_TGFix_AspectRatio > SK_TGFix_NativeAspect) ? (float)SK_TGFix_ScreenWidth  / SK_TGFix_AspectMultiplier :
                                                         (float)SK_TGFix_ScreenWidth;
      const float height =
        (SK_TGFix_AspectRatio < SK_TGFix_NativeAspect) ? (float)SK_TGFix_ScreenHeight * SK_TGFix_AspectMultiplier :
                                                         (float)SK_TGFix_ScreenHeight;

      _rect.width  = width;
      _rect.height = height;

      SK_mono_field_set_value (primitiveManager, m_Viewport, &_rect);
    }
  }
}

void
SK_TGFix_Noble_PrimitiveManager_OnEndUpdateNativeGameMain_Detour (MonoObject* __this)
{
  SK_LOG_FIRST_CALL

  SK_TGFix_PrimitiveManagerSingleton = __this;

  Noble_PrimitiveManager_OnEndUpdateNativeGameMain_Original (__this);
}

using Noble_PrimitiveManager_PrimitiveRenderExecute_Internal_pfn = void (*)(MonoObject*,MonoObject*,MonoObject*,MonoObject*,MonoObject*);
      Noble_PrimitiveManager_PrimitiveRenderExecute_Internal_pfn
      Noble_PrimitiveManager_PrimitiveRenderExecute_Internal_Original = nullptr;

void
SK_TGFix_Noble_PrimitiveManager_PrimitiveRenderExecute_Internal_Detour (MonoObject* __this, MonoObject* context, MonoObject* obj, MonoObject* commandBuffer, MonoObject* renderingData)
{
  SK_LOG_FIRST_CALL

  if (obj != nullptr && SK_TGFix_Cfg.disable_fog)
  {
    const auto vertexCount     = SK_TGFix_MonoFields.Noble.ObjPrimitiveBase.vertexCount;
    const auto m_PrimitiveType = SK_TGFix_MonoFields.Noble.ObjPrimitiveBase.m_PrimitiveType;
    const auto type            = SK_TGFix_MonoFields.Noble.PrimitiveManager.PRIM_PARAM.type;
    const auto db              = SK_TGFix_MonoFields.Noble.PrimitiveManager.PRIM_TYPE.db;

    // Get the vertex count first, let's -try- to be efficient here... Mono makes that difficult.
    int                           obj_vertexCount = 0;
    SK_mono_field_get_value (obj,     vertexCount,
                                 &obj_vertexCount);

    // Pause Background
    //if (__1.m_PrimitiveType.type.db[0] == 2) return false;
    
    // Text Bubble  (and some screenspace garbage)
    if (obj_vertexCount == 3) // Type.db[0] == 4
    {
      MonoObject* primitive_type      =
        SK_mono_field_get_value_object (SK_mono_object_get_domain (obj), m_PrimitiveType,
                                                                   obj);
      MonoObject* primitive_type_type =
        SK_mono_field_get_value_object (SK_mono_object_get_domain (primitive_type), type,
                                                                   primitive_type);

      MonoArray*                                         obj_m_PrimitiveType_type_db = nullptr;
      SK_mono_field_get_value (primitive_type_type, db, &obj_m_PrimitiveType_type_db);

      if (SK_mono_array_get (obj_m_PrimitiveType_type_db, int, 0) == 4)
      {
        return;
      }
    }
    
    // Star dream sequences
    //if (__1.m_PrimitiveType.type.db[0] == 6) return false;
  }
  
  Noble_PrimitiveManager_PrimitiveRenderExecute_Internal_Original (__this, context, obj, commandBuffer, renderingData);
}

using NobleMovieRendererPass_Execute_pfn = void (*)(MonoObject*, MonoObject*, MonoObject*);
      NobleMovieRendererPass_Execute_pfn
      NobleMovieRendererPass_Execute_Original = nullptr;

void
SK_TGFix_NobleMovieRendererPass_Execute_Detour (MonoObject* __this, MonoObject* context, MonoObject* renderingData)
{
  SK_LOG_FIRST_CALL

  NobleMovieRendererPass_Execute_Original (__this, context, renderingData);
  
  if (SK_TGFix_AspectRatio != SK_TGFix_NativeAspect && SK_TGFix_AspectRatio != 0.0f)
  {
    const auto cameraview =
      SK_TGFix_MonoFields.Noble.NobleMovieRendererPass.cameraview;

    Unity_Matrix4x4 view;
    SK_mono_field_get_value (__this, cameraview, &view);

    float& m11 = view.m11;
    float& m33 = view.m33;

    if      (SK_TGFix_AspectRatio > SK_TGFix_NativeAspect) m33 = SK_TGFix_AspectMultiplier;
    else if (SK_TGFix_AspectRatio < SK_TGFix_NativeAspect) m11 = SK_TGFix_AspectMultiplier;

    SK_mono_field_set_value (__this, cameraview, &view);
  }
}

using NoblePostEffectLegacyShadow_BeginRender_pfn = void (*)(MonoObject* __this, MonoObject* renderPass, MonoObject* cmd, MonoObject* renderingData);
      NoblePostEffectLegacyShadow_BeginRender_pfn
      NoblePostEffectLegacyShadow_BeginRender_Original = nullptr;

void
SK_TGFix_NoblePostEffectLegacyShadow_BeginRender_Detour (MonoObject* __this, MonoObject* renderPass, MonoObject* cmd, MonoObject* renderingData)
{
  SK_LOG_FIRST_CALL

  #if 0
  if (SK_TGFix_Cfg.render_scale != 1.0f)
  {
    int        mLegacyShadowParam_mRenderShadowParamUseNum = 0;
    MonoArray* mLegacyShadowParam_mRenderShadowParam       = nullptr;

    MonoObject* mLegacyShadowParam =
      SK_mono_runtime_invoke (
        SK_TGFix_MonoMethods.NoblePostEffectLegacyShadowParam.get_mLegacyShadowParam,
          nullptr, nullptr, nullptr
      );

    SK_mono_field_get_value (mLegacyShadowParam,
      SK_TGFix_MonoFields.NobleLegacyShadowRenderParameter.mRenderShadowParam,
                                       &mLegacyShadowParam_mRenderShadowParam);

    SK_mono_field_get_value (mLegacyShadowParam,
      SK_TGFix_MonoFields.NobleLegacyShadowRenderParameter.mRenderShadowParamUseNum,
                                       &mLegacyShadowParam_mRenderShadowParamUseNum);

    const int num =
      std::min (mLegacyShadowParam_mRenderShadowParamUseNum,
        (int)SK_mono_array_length (mLegacyShadowParam_mRenderShadowParam));

    for (int i = 0; i < num; i++)
    {
      auto mRenderShadowParam =
        SK_mono_array_get (mLegacyShadowParam_mRenderShadowParam,
                                         Noble_LegacyShadowRenderParam, i);

      mRenderShadowParam.mScissor.x *= SK_TGFix_Cfg.render_scale;
      mRenderShadowParam.mScissor.y *= SK_TGFix_Cfg.render_scale;
      mRenderShadowParam.mScissor.z /= SK_TGFix_Cfg.render_scale;
      mRenderShadowParam.mScissor.w /= SK_TGFix_Cfg.render_scale;

      SK_mono_array_set (mLegacyShadowParam_mRenderShadowParam,
                                         Noble_LegacyShadowRenderParam, i,
                                            mRenderShadowParam);
    }
  }
  #endif

  NoblePostEffectLegacyShadow_BeginRender_Original (__this, renderPass, cmd, renderingData);
}


using UnityEngine_Rendering_CommandBuffer_EnableScissorRect_pfn = void (*)(MonoObject*, Unity_Rect*);
      UnityEngine_Rendering_CommandBuffer_EnableScissorRect_pfn
      UnityEngine_Rendering_CommandBuffer_EnableScissorRect_Original = nullptr;

#if 0
void
SK_TGFix_ComputeAspectCoeffs (float& x, float& y, float& xoff, float& yoff)
{
  yoff = 0.0f;
  xoff = 0.0f;

  x = 1.0f;
  y = 1.0f;

  ////float rescale = (1.77777778f / SK_TGFix_AspectRatio);

  // Wider
  if (SK_TGFix_AspectRatio > 1.7777f) {
    int width  = (int)((16.0f / 9.0f) * SK_TGFix_ScreenHeight);
    int x_off  = (int)((SK_TGFix_ScreenWidth - width) / 2);

    x    = (float)SK_TGFix_ScreenWidth / (float)width;
    xoff = (float)x_off;
  } else {
    int height = (int)((9.0f / 16.0f) * SK_TGFix_ScreenWidth);
    int y_off  = (int)((SK_TGFix_ScreenHeight - height) / 2);

    y    = (float)SK_TGFix_ScreenHeight / (float)height;
    yoff = (float)y_off;
  }
}
#endif

void
SK_TGFix_UnityEngine_Rendering_CommandBuffer_EnableScissorRect_Detour (MonoObject* __this, Unity_Rect* Scissor)
{
  SK_LOG_FIRST_CALL

  if (SK_TGFix_Cfg.hacks.fix_shadow_scissors && (SK_TGFix_Cfg.render_scale != 1.0f || (SK_TGFix_AspectRatio != SK_TGFix_NativeAspect && SK_TGFix_AspectRatio != 0.0f)))
  {
    const float scale = SK_TGFix_Cfg.render_scale;

    if (SK_TGFix_AspectRatio != SK_TGFix_NativeAspect)
    {
      // Disable scissor, we can't reasonably fix what went wrong here!
      return;
#if 0
      float x_scale, y_scale;
      float x_off,   y_off;
      SK_TGFix_ComputeAspectCoeffs (x_scale, y_scale, x_off, y_off);

      // Wider
      if (SK_TGFix_AspectRatio > 1.7777f) {
        float left  =        Scissor->x;
        float right = left + Scissor->width;

        float left_ndc  = 2.0f * (left  / (float)(SK_TGFix_ScreenWidth)) - 1.0f;
        float right_ndc = 2.0f * (right / (float)(SK_TGFix_ScreenWidth)) - 1.0f;

        int width = (int)((16.0f / 9.0f) * SK_TGFix_ScreenHeight);

        left  = (left_ndc  * width + width) / 2.0f + x_off;
        right = (right_ndc * width + width) / 2.0f + x_off;

        Scissor->x     =       left;
        Scissor->width = right-left;
      } else {
        //float top_ndc    = 2.0f * ((float)pRect->top    / (float)tzf::RenderFix::height) - 1.0f;
        //float bottom_ndc = 2.0f * ((float)pRect->bottom / (float)tzf::RenderFix::height) - 1.0f;
        //
        //int height = (9.0f / 16.0f) * tzf::RenderFix::width;
        //
        //fixed_scissor.top    = (top_ndc    * height + height) / 2.0f + y_off;
        //fixed_scissor.bottom = (bottom_ndc * height + height) / 2.0f + y_off;
      }
#endif
    }

    // A simple rescale done in NDC-space to properly stretch the scissor rectangle
    if (SK_TGFix_Cfg.render_scale != 1.0f)
    {
      float NewWidth  = (float)SK_TGFix_ScreenWidth  * scale;
      float NewHeight = (float)SK_TGFix_ScreenHeight * scale;

      float left_ndc = 2.0f * ( Scissor->x / (float)SK_TGFix_ScreenWidth  ) - 1.0f;
      float top_ndc  = 2.0f * ( Scissor->y / (float)SK_TGFix_ScreenHeight ) - 1.0f;

      Scissor->x = (left_ndc * NewWidth  + NewWidth)  / 2.0f;
      Scissor->y = (top_ndc  * NewHeight + NewHeight) / 2.0f;

      Scissor->width  = Scissor->width  * scale;
      Scissor->height = Scissor->height * scale;
    }
  }

    //// We're nop'ing this entire thing because it breaks shadows if we don't.
    //return;

  UnityEngine_Rendering_CommandBuffer_EnableScissorRect_Original (__this, Scissor);
}

using Steamworks_SteamUtils_IsOverlayEnabled_pfn = bool (*)(void);
      Steamworks_SteamUtils_IsOverlayEnabled_pfn
      Steamworks_SteamUtils_IsOverlayEnabled_Original = nullptr;

bool
SK_TGFix_Steamworks_SteamUtils_IsOverlayEnabled_Detour (void)
{
  SK_LOG_FIRST_CALL

  if (! Steamworks_SteamUtils_IsOverlayEnabled_Original ())
  {
    if (! SK_TGFix_Cfg.achievement_earned)
    {
      SK_RunOnce (
      {
        SK_ImGui_CreateNotification ( "TalesOfGraces.CriticalSteamBug",
                                            SK_ImGui_Toast::Warning,
                  "This game normally requires the Steam overlay to be enabled to unlock achievements\r\n\r\n\t"
                  "Special K has fixed the problem -- but -- you will not retroactively unlock missed achievements!!\r\n\r\n"
                  " * This is a serious bug, please report it to Bandai Namco ASAP!",
                  "Steamworks Achievement Bug (Partially) Fixed",
                                              25000UL,
                                            SK_ImGui_Toast::UseDuration |
                                            SK_ImGui_Toast::ShowTitle   |
                                            SK_ImGui_Toast::ShowCaption |
                                            SK_ImGui_Toast::ShowOnce );
      });
    }
  }

  return true;
}

bool
SK_TGFix_SetupFramerateHooks (void)
{
  // Mono not init'd
  if (SK_mono_thread_attach == nullptr)
    return false;

  if (! LoadMonoAssembly ("Assembly-CSharp"))
    return false;

  AttachThread ();

  SK_RunOnce (
  {
    //
    // Framerate Hooks
    //
    auto pfnNobleFrameRateManager_SetQualitySettingFrameRate =
      CompileMethod ("Noble", "FrameRateManager",
             "SetQualitySettingFrameRate", 1);

    auto pfnNobleFrameRateManager_SetTargetFrameRate =
      CompileMethod ("Noble", "FrameRateManager",
                     "SetTargetFrameRate", 1);

    SK_CreateFuncHook (              L"Noble.FrameRateManager.SetQualitySettingFrameRate",
                                     pfnNobleFrameRateManager_SetQualitySettingFrameRate,
                               SK_TGFix_NobleFrameRateManager_SetQualitySettingFrameRate_Detour,
      static_cast_p2p <void> (&         NobleFrameRateManager_SetQualitySettingFrameRate_Original) );

    SK_CreateFuncHook (              L"Noble.FrameRateManager.SetTargetFrameRate",
                                     pfnNobleFrameRateManager_SetTargetFrameRate,
                               SK_TGFix_NobleFrameRateManager_SetTargetFrameRate_Detour,
      static_cast_p2p <void> (&         NobleFrameRateManager_SetTargetFrameRate_Original) );

    SK_QueueEnableHook (pfnNobleFrameRateManager_SetQualitySettingFrameRate);
    SK_QueueEnableHook (pfnNobleFrameRateManager_SetTargetFrameRate);

    //
    // Aspect Ratio Hooks
    //
    auto pfnNoble_CameraManager_SetCameraViewportRect =
      CompileMethod ("Noble", "CameraManager",
                           "SetCameraViewportRect", 1);

    auto pfnNoble_CameraManager_SetCameraAspect =
      CompileMethod ("Noble", "CameraManager",
                           "SetCameraAspect", 1);

    auto pfnNoble_CameraManager_SetBackGoundColor =
      CompileMethod ("Noble", "CameraManager",
                           "SetBackGoundColor", 1);

    SK_CreateFuncHook (               L"Noble.CameraManager.SetCameraViewportRect",
                                     pfnNoble_CameraManager_SetCameraViewportRect,
                               SK_TGFix_Noble_CameraManager_SetCameraViewportRect_Detour,
      static_cast_p2p <void> (&         Noble_CameraManager_SetCameraViewportRect_Original) );

    SK_CreateFuncHook (               L"Noble.CameraManager.SetCameraAspect",
                                     pfnNoble_CameraManager_SetCameraAspect,
                               SK_TGFix_Noble_CameraManager_SetCameraAspect_Detour,
      static_cast_p2p <void> (&         Noble_CameraManager_SetCameraAspect_Original) );

    SK_CreateFuncHook (               L"Noble.CameraManager.SetBackGoundColor",
                                     pfnNoble_CameraManager_SetBackGoundColor,
                               SK_TGFix_Noble_CameraManager_SetBackGoundColor_Detour,
      static_cast_p2p <void> (&         Noble_CameraManager_SetBackGoundColor_Original) );

    SK_QueueEnableHook (pfnNoble_CameraManager_SetCameraViewportRect);
    SK_QueueEnableHook (pfnNoble_CameraManager_SetCameraAspect);
    SK_QueueEnableHook (pfnNoble_CameraManager_SetBackGoundColor);


    // Keep all objects visible at all times to workaround bad camera culling
    auto pfnNoble_Object_ApplyCachedParameters =
      CompileMethod ("Noble", "Object",
                           "ApplyCachedParameters", 0);
    
    SK_CreateFuncHook (               L"Noble.Object.ApplyCachedParameters",
                                     pfnNoble_Object_ApplyCachedParameters,
                               SK_TGFix_Noble_Object_ApplyCachedParameters_Detour,
      static_cast_p2p <void> (&         Noble_Object_ApplyCachedParameters_Original) );
    
    SK_QueueEnableHook (pfnNoble_Object_ApplyCachedParameters);

    auto pfnNoble_PrimitiveManager_CalcUIOrthoMatrix =
      CompileMethod ("Noble", "PrimitiveManager",
                     "CalcUIOrthoMatrix", 1);

    SK_CreateFuncHook (               L"Noble.PrimitiveManager.CalcUIOrthoMatrix",
                                     pfnNoble_PrimitiveManager_CalcUIOrthoMatrix,
                               SK_TGFix_Noble_PrimitiveManager_CalcUIOrthoMatrix_Detour,
      static_cast_p2p <void> (&         Noble_PrimitiveManager_CalcUIOrthoMatrix_Original) );

    SK_QueueEnableHook (pfnNoble_PrimitiveManager_CalcUIOrthoMatrix);

    auto pfnNoble_PrimitiveManager_OnBeginUpdateNativeGameMain =
      CompileMethod ("Noble",   "PrimitiveManager",
                     "OnBeginUpdateNativeGameMain", 0);

    SK_CreateFuncHook (               L"Noble.PrimitiveManager.OnBeginUpdateNativeGameMain",
                                     pfnNoble_PrimitiveManager_OnBeginUpdateNativeGameMain,
                               SK_TGFix_Noble_PrimitiveManager_OnBeginUpdateNativeGameMain_Detour,
      static_cast_p2p <void> (&         Noble_PrimitiveManager_OnBeginUpdateNativeGameMain_Original) );

    SK_QueueEnableHook (pfnNoble_PrimitiveManager_OnBeginUpdateNativeGameMain);

    ////auto pfnNoble_PrimitiveManager_OnEndUpdateNativeGameMain =
    ////  CompileMethod ("Noble", "PrimitiveManager",
    ////                 "OnEndUpdateNativeGameMain", 0);
    ////
    ////SK_CreateFuncHook (               L"Noble.PrimitiveManager.OnEndUpdateNativeGameMain",
    ////                                 pfnNoble_PrimitiveManager_OnEndUpdateNativeGameMain,
    ////                           SK_TGFix_Noble_PrimitiveManager_OnEndUpdateNativeGameMain_Detour,
    ////  static_cast_p2p <void> (&         Noble_PrimitiveManager_OnEndUpdateNativeGameMain_Original) );
    ////
    ////SK_QueueEnableHook (pfnNoble_PrimitiveManager_OnEndUpdateNativeGameMain);

    auto pfnNoble_PrimitiveManager_PrimitiveRenderExecute_Internal =
      CompileMethod ("Noble", "PrimitiveManager",
                     "PrimitiveRenderExecute_Internal", 4);

    SK_CreateFuncHook (               L"Noble.PrimitiveManager.PrimitiveRenderExecute_Internal",
                                     pfnNoble_PrimitiveManager_PrimitiveRenderExecute_Internal,
                               SK_TGFix_Noble_PrimitiveManager_PrimitiveRenderExecute_Internal_Detour,
      static_cast_p2p <void> (&         Noble_PrimitiveManager_PrimitiveRenderExecute_Internal_Original) );

    SK_QueueEnableHook (pfnNoble_PrimitiveManager_PrimitiveRenderExecute_Internal);

    auto pfnNobleMovieRendererPass_Execute =
      CompileMethod ("",
           "NobleMovieRendererPass",
                                  "Execute", 2);

    SK_CreateFuncHook (               L"NobleMovieRendererPass.Execute",
                                     pfnNobleMovieRendererPass_Execute,
                               SK_TGFix_NobleMovieRendererPass_Execute_Detour,
      static_cast_p2p <void> (&         NobleMovieRendererPass_Execute_Original) );

    SK_QueueEnableHook (pfnNobleMovieRendererPass_Execute);


    auto pfnNoblePostEffectLegacyShadow_BeginRender =
      CompileMethod ("",
           "NoblePostEffectLegacyShadow",
                                       "BeginRender", 3);

    SK_CreateFuncHook (               L"NoblePostEffectLegacyShadow.BeginRender",
                                     pfnNoblePostEffectLegacyShadow_BeginRender,
                               SK_TGFix_NoblePostEffectLegacyShadow_BeginRender_Detour,
      static_cast_p2p <void> (&         NoblePostEffectLegacyShadow_BeginRender_Original) );

    SK_QueueEnableHook (pfnNoblePostEffectLegacyShadow_BeginRender);

    SK_ApplyQueuedHooks ();
  });

  SK_TGFix_MonoAssemblies.assemblyCSharp         = SK_mono_image_loaded ("Assembly-CSharp");
  SK_TGFix_MonoAssemblies.UnityEngine_CoreModule = SK_mono_image_loaded ("UnityEngine.CoreModule");

  auto& assemblyCSharp                           = SK_TGFix_MonoAssemblies.assemblyCSharp;

  MonoClass* Noble_PrimitiveManagerClass         =
    SK_mono_class_from_name (assemblyCSharp, "Noble", "PrimitiveManager");
  MonoClass* Noble_PrimitiveManager_PRIM_PARAM   =
    SK_mono_class_from_name (assemblyCSharp, "Noble", "PrimitiveManager/PRIM_PARAM");
  MonoClass* Noble_PrimitiveManager_PRIM_TYPE    =
    SK_mono_class_from_name (assemblyCSharp, "Noble", "PrimitiveManager/PRIM_TYPE");
  MonoClass* Noble_ObjPrimitiveBaseClass         =
    SK_mono_class_from_name (assemblyCSharp, "Noble", "ObjPrimitiveBase");
  MonoClass* Noble_CameraManagerClass            =
    SK_mono_class_from_name (assemblyCSharp, "Noble", "CameraManager");

  SK_TGFix_MonoMethods.Noble.PrimitiveManager.CalcUIOrthoMatrix  = SK_mono_class_get_method_from_name (Noble_PrimitiveManagerClass,       "CalcUIOrthoMatrix", 1);
  SK_TGFix_MonoFields. Noble.PrimitiveManager.m_Viewport         = SK_mono_class_get_field_from_name  (Noble_PrimitiveManagerClass,       "m_Viewport");
  SK_TGFix_MonoFields. Noble.PrimitiveManager.PRIM_PARAM.type    = SK_mono_class_get_field_from_name  (Noble_PrimitiveManager_PRIM_PARAM, "type");
  SK_TGFix_MonoFields. Noble.PrimitiveManager.PRIM_TYPE.db       = SK_mono_class_get_field_from_name  (Noble_PrimitiveManager_PRIM_TYPE,  "db");

  SK_TGFix_MonoMethods.Noble.CameraManager.SetCameraAspect       = SK_mono_class_get_method_from_name (Noble_CameraManagerClass, "SetCameraAspect",       1);
  SK_TGFix_MonoMethods.Noble.CameraManager.SetCameraViewportRect = SK_mono_class_get_method_from_name (Noble_CameraManagerClass, "SetCameraViewportRect", 1);

  SK_TGFix_MonoMethods.Noble.Object.SetVisibilityFlags           = GetMethod ("Object", "SetVisibilityFlags", 2, "Assembly-CSharp", "Noble");
  SK_TGFix_MonoMethods.Noble.Object.IsDirty                      = GetMethod ("Object", "IsDirty",            1, "Assembly-CSharp", "Noble");
  SK_TGFix_MonoMethods.Noble.Object.IsVisible                    = GetMethod ("Object", "IsVisible",          1, "Assembly-CSharp", "Noble");

  SK_TGFix_MonoFields. Noble.NobleMovieRendererPass.cameraview   = GetField  ("NobleMovieRendererPass", "cameraview", "Assembly-CSharp");

  SK_TGFix_MonoFields. Noble.ObjPrimitiveBase.vertexCount        = SK_mono_class_get_field_from_name (Noble_ObjPrimitiveBaseClass, "vertexCount");
  SK_TGFix_MonoFields. Noble.ObjPrimitiveBase.m_PrimitiveType    = SK_mono_class_get_field_from_name (Noble_ObjPrimitiveBaseClass, "m_PrimitiveType");

  MonoClass                                                              *NoblePostEffectLegacyShadowParam_Class =
    SK_mono_class_from_name (SK_TGFix_MonoAssemblies.assemblyCSharp, "", "NoblePostEffectLegacyShadowParam"),
                                                                         *NobleLegacyShadowRenderParameter_Class =
    SK_mono_class_from_name (SK_TGFix_MonoAssemblies.assemblyCSharp, "", "NobleLegacyShadowRenderParameter");

  SK_TGFix_MonoMethods.NoblePostEffectLegacyShadowParam.get_mLegacyShadowParam  =
    SK_mono_class_get_method_from_name (NoblePostEffectLegacyShadowParam_Class, "get_mLegacyShadowParam", 0);

  SK_TGFix_MonoFields.NobleLegacyShadowRenderParameter.mRenderShadowParamUseNum =
    SK_mono_class_get_field_from_name (NobleLegacyShadowRenderParameter_Class, "mRenderShadowParamUseNum");
  SK_TGFix_MonoFields.NobleLegacyShadowRenderParameter.mRenderShadowParam       =
    SK_mono_class_get_field_from_name (NobleLegacyShadowRenderParameter_Class, "mRenderShadowParam");

  DetachCurrentThreadIfNotNative ();

  if (! LoadMonoAssembly ("com.rlabrecque.steamworks.net"))
    return false;

  AttachThread ();

  SK_RunOnce(
  {
    auto pfnSteamworks_SteamUtils_IsOverlayEnabled =
      CompileMethod ("Steamworks", "SteamUtils",
                     "IsOverlayEnabled", 0, "com.rlabrecque.steamworks.net");

    SK_CreateFuncHook (               L"Steamworks.SteamUtils.IsOverlayEnabled",
                                     pfnSteamworks_SteamUtils_IsOverlayEnabled,
                               SK_TGFix_Steamworks_SteamUtils_IsOverlayEnabled_Detour,
      static_cast_p2p <void> (&         Steamworks_SteamUtils_IsOverlayEnabled_Original) );

    SK_QueueEnableHook (pfnSteamworks_SteamUtils_IsOverlayEnabled);
  });

  DetachCurrentThreadIfNotNative ();

  if (! LoadMonoAssembly ("UnityEngine.CoreModule"))
    return false;

  AttachThread ();

  SK_RunOnce (
  {
    auto pfnUnityEngine_Screen_SetResolution =
      CompileMethod ("UnityEngine", "Screen",
                     "SetResolution", 3, "UnityEngine.CoreModule");

    SK_CreateFuncHook (               L"UnityEngine.Screen.SetResolution",
                                     pfnUnityEngine_Screen_SetResolution,
                               SK_TGFix_UnityEngine_Screen_SetResolution_Detour,
      static_cast_p2p <void> (&         UnityEngine_Screen_SetResolution_Original) );

    SK_QueueEnableHook (pfnUnityEngine_Screen_SetResolution);

    auto pfnUnityEngine_Rendering_CommandBuffer_EnableScissorRect =
      CompileMethod ("UnityEngine.Rendering", "CommandBuffer", "EnableScissorRect_Injected", 1,
                     "UnityEngine.CoreModule");

    SK_CreateFuncHook (               L"UnityEngine.Rendering.CommandBuffer.EnableScissorRect",
                                     pfnUnityEngine_Rendering_CommandBuffer_EnableScissorRect,
                               SK_TGFix_UnityEngine_Rendering_CommandBuffer_EnableScissorRect_Detour,
      static_cast_p2p <void> (&         UnityEngine_Rendering_CommandBuffer_EnableScissorRect_Original) );

    SK_QueueEnableHook (pfnUnityEngine_Rendering_CommandBuffer_EnableScissorRect);

    SK_ApplyQueuedHooks ();
  });

  DetachCurrentThreadIfNotNative ();

  if (! (LoadMonoAssembly ("Unity.RenderPipelines.Core.Runtime") &&
         LoadMonoAssembly ("Unity.RenderPipelines.Universal.Runtime")))
    return false;

  return true;
}

void STDMETHODCALLTYPE SK_TGFix_EndFrame (void)
{
  if (SK_TGFix_FrameRateManagerSingleton != nullptr)
  {
    if ( __target_fps > 0.0f &&
         __target_fps != SK_TGFix_LastSetFrameRateLimit )
    {
      SK_TGFix_LastSetFrameRateLimit =
                          __target_fps;
      void* args [1] = { &__target_fps };

      InvokeMethod ("Noble", "FrameRateManager", "SetTargetFrameRate",         1,
                     SK_TGFix_FrameRateManagerSingleton, "Assembly-CSharp", args);

      InvokeMethod ("Noble", "FrameRateManager", "SetQualitySettingFrameRate", 1,
                     SK_TGFix_FrameRateManagerSingleton, "Assembly-CSharp", args);

      // We don't want garbage collection overhead on this thread just because we called a function once!
      DetachCurrentThreadIfNotNative ();
    }
  }

  static auto& rb =
    SK_GetCurrentRenderBackend ();

  SK_ComQIPtr <IDXGISwapChain1>
      pSwapChain (rb.swapchain.p);
  if (pSwapChain)
  {
    DXGI_SWAP_CHAIN_DESC  swapDesc = {};
    pSwapChain->GetDesc (&swapDesc);

    static UINT last_width  = 0;
    static UINT last_height = 0;
    if ((UINT)(std::exchange (last_width,  swapDesc.BufferDesc.Width)  != swapDesc.BufferDesc.Width) |
        (UINT)(std::exchange (last_height, swapDesc.BufferDesc.Height) != swapDesc.BufferDesc.Height))
    {
      // Cause a recalculation of this matrix, at the beginning of the next frame.
      SK_TGFix_LastCalcUIOrthoMatrixWasForced = true;

      SK_TGFix_ScreenWidth      = swapDesc.BufferDesc.Width;
      SK_TGFix_ScreenHeight     = swapDesc.BufferDesc.Height;

      SK_TGFix_AspectRatio      = (float)SK_TGFix_ScreenWidth / (float)SK_TGFix_ScreenHeight;
      SK_TGFix_AspectMultiplier =        SK_TGFix_AspectRatio /        SK_TGFix_NativeAspect;

      SK_LOGi0 ("Current Resolution: %dx%d",                 swapDesc.BufferDesc.Width,
                                                             swapDesc.BufferDesc.Height);
      SK_LOGi0 ("Current Resolution: Aspect Ratio: %f",      SK_TGFix_AspectRatio);
      SK_LOGi0 ("Current Resolution: Aspect Multiplier: %f", SK_TGFix_AspectMultiplier);
    }
  }

  return;
}

MonoObject*
SK_TGFix_NobleQualitySettings (void)
{
  static MonoObject*
        NobleQualitySettings = nullptr;
  if (! NobleQualitySettings)
  {
    AttachThread ();

    MonoClass*  gameObjectClass    = SK_mono_class_from_name            (SK_TGFix_MonoAssemblies.UnityEngine_CoreModule, "UnityEngine", "GameObject");
    MonoMethod* findMethod         = SK_mono_class_get_method_from_name (gameObjectClass, "Find",         1);
    MonoMethod* getComponentMethod = SK_mono_class_get_method_from_name (gameObjectClass, "GetComponent", 1);

    MonoClass* Noble_NobleQualitySettingsClass =
      SK_mono_class_from_name (SK_TGFix_MonoAssemblies.assemblyCSharp, "Noble", "NobleQualitySettings");

    void* find_args [1] =
      { SK_mono_string_new (SK_TGFix_MonoDomain, "NobleQualitySettings") };

    auto gameObject =
      SK_mono_runtime_invoke (findMethod, nullptr, find_args, nullptr);

    void* get_component_args [1] =
      { SK_mono_type_get_object (SK_TGFix_MonoDomain, SK_mono_class_get_type (Noble_NobleQualitySettingsClass)) };

    NobleQualitySettings =
      SK_mono_runtime_invoke (getComponentMethod, gameObject, get_component_args, nullptr);

    // The object we just got is a singleton and persistent whether we garbage collect on this thread or not
    DetachCurrentThreadIfNotNative ();
  }

  return NobleQualitySettings;
}

MonoObject*
SK_TGFix_GetMainCameraAdditionalData (void)
{
  static MonoObject*
        UniversalAdditionalCameraData = nullptr;
  if (! UniversalAdditionalCameraData)
  {
    AttachThread ();

    MonoImage*  assemblyRenderPipes = SK_mono_image_loaded               ("Unity.RenderPipelines.Universal.Runtime");
    MonoClass*  gameObjectClass     = SK_mono_class_from_name            (SK_mono_image_loaded ("UnityEngine.CoreModule"), "UnityEngine", "GameObject");
    MonoMethod* findMethod          = SK_mono_class_get_method_from_name (gameObjectClass, "Find",         1);
    MonoMethod* getComponentMethod  = SK_mono_class_get_method_from_name (gameObjectClass, "GetComponent", 1);

    MonoClass* UnityEngine_Rendering_Universal_UniversalAdditionalCameraDataClass =
      SK_mono_class_from_name (assemblyRenderPipes, "UnityEngine.Rendering.Universal", "UniversalAdditionalCameraData");

    void* find_args [1] =
      { SK_mono_string_new (SK_TGFix_MonoDomain, "Main Camera") };

    auto gameObject =
      SK_mono_runtime_invoke (findMethod, nullptr, find_args, nullptr);

    void* get_component_args [1] =
      { SK_mono_type_get_object (SK_TGFix_MonoDomain, SK_mono_class_get_type (UnityEngine_Rendering_Universal_UniversalAdditionalCameraDataClass)) };

    UniversalAdditionalCameraData =
      SK_mono_runtime_invoke (getComponentMethod, gameObject, get_component_args, nullptr);

    // The object we just got is a singleton and persistent whether we garbage collect on this thread or not
    DetachCurrentThreadIfNotNative ();
  }

  return UniversalAdditionalCameraData;
}

void
SK_TGFix_SetMSAASampleCount (int sample_count)
{
  static MonoObject* pipeline =
    InvokeMethod ("Noble", "NobleQualitySettings", "get_m_urpAsset", 0, SK_TGFix_NobleQualitySettings ());

  void* params [1] = { &sample_count };

  SK_LOGi0 (L"Setting MSAA Sample Count: %d", sample_count);

  InvokeMethod ("UnityEngine.Rendering.Universal", "UniversalRenderPipelineAsset", "set_msaaSampleCount", 1, pipeline, "Unity.RenderPipelines.Universal.Runtime", params);

  if (sample_count > 1)
  {
    int none     = 0;
    int lut_size = 32;

    SetFieldValue (pipeline, GetField (GetClass ("UniversalRenderPipelineAsset", "Unity.RenderPipelines.Universal.Runtime", "UnityEngine.Rendering.Universal"), "m_OpaqueDownsampling"),  &none);
    SetFieldValue (pipeline, GetField (GetClass ("UniversalRenderPipelineAsset", "Unity.RenderPipelines.Universal.Runtime", "UnityEngine.Rendering.Universal"), "m_ColorGradingLutSize"), &lut_size);
  }

  DetachCurrentThreadIfNotNative ();
}

void
SK_TGFix_SetRenderScale (float render_scale)
{
  // Avoid VERY expensive repeated calls while user is holding ImGui slider
  static float fRenderScale = 0.0f;

  // We _could_ override the range that Unity allows, but probably not worth it
  render_scale = std::clamp (render_scale, 0.1f, 2.0f);

  if (std::exchange (fRenderScale, render_scale) == render_scale)
    return;

  static MonoObject* pipeline =
    InvokeMethod ("Noble", "NobleQualitySettings", "get_m_urpAsset", 0, SK_TGFix_NobleQualitySettings ());

  void* params [1] = { &render_scale };

  SK_LOGi0 (L"Setting Render Scale: %f", render_scale);

  InvokeMethod ("UnityEngine.Rendering.Universal", "UniversalRenderPipelineAsset", "set_renderScale", 1, pipeline, "Unity.RenderPipelines.Universal.Runtime", params);

  if (render_scale != 1.0f)
  {
    int none     = 0;
    int lut_size = 32;

    SetFieldValue (pipeline, GetField (GetClass ("UniversalRenderPipelineAsset", "Unity.RenderPipelines.Universal.Runtime", "UnityEngine.Rendering.Universal"), "m_OpaqueDownsampling"),  &none);
    SetFieldValue (pipeline, GetField (GetClass ("UniversalRenderPipelineAsset", "Unity.RenderPipelines.Universal.Runtime", "UnityEngine.Rendering.Universal"), "m_ColorGradingLutSize"), &lut_size);
  }

  // We don't want garbage collection overhead on this thread just because we called a function once!
  DetachCurrentThreadIfNotNative ();
}

void
SK_TGFix_SetCameraAA (void)
{
  MonoObject* MainCameraAdditionalData =
  SK_TGFix_GetMainCameraAdditionalData ();

  // SubpixelMorphologicalAntiAliasing = 2
  // TemporalAntiAliasing              = 3

  int                                  mode = 0;
  if (      SK_TGFix_Cfg.use_taa)      mode = 3;
  else if (!SK_TGFix_Cfg.disable_fxaa) mode = 2;

  void* params [1] = { &mode };
  
  SK_LOGi0 (L"Setting PostFX AA Type: %d", mode);

  InvokeMethod ("UnityEngine.Rendering.Universal", "UniversalAdditionalCameraData", "set_antialiasing", 1,
                MainCameraAdditionalData, "Unity.RenderPipelines.Universal.Runtime", params);

  // We don't want garbage collection overhead on this thread just because we called a function once!
  DetachCurrentThreadIfNotNative ();
}

void
SK_TGFix_EnableInternalHDR (bool enable)
{
  if (! enable)
    return;

  // Enable HDR remastering for final out
  SK_HDR_RenderTargets_8bpc.getPtr ()->PromoteTo16Bit = true;

  static bool          skipped_once = false;
  if (! std::exchange (skipped_once, true))
    return;

  // Do not turn on internal HDR without RenoDX
  if (! GetModuleHandleW (L"renodx-talesofgracesf.addon64"))
    return;

  SK_LOGi0 (L"Enabling Native HDR");

  static MonoObject* pipeline =
    InvokeMethod ("Noble", "NobleQualitySettings", "get_m_urpAsset", 0, SK_TGFix_NobleQualitySettings ());

  bool                 enable_hdr = true;
  void* params [] = { &enable_hdr };

  // Enable native HDR internally for most render passes
  InvokeMethod ("UnityEngine.Rendering.Universal", "UniversalRenderPipelineAsset", "set_supportsHDR", 1, pipeline,
                "Unity.RenderPipelines.Universal.Runtime", params);

  // We don't want garbage collection overhead on this thread just because we called a function once!
  DetachCurrentThreadIfNotNative ();
}

void
SK_TGFix_SetInputPollingFreq (float PollingHz)
{
  if (std::exchange (SK_TGFix_InputPollingFrequency, PollingHz) == PollingHz)
    return;

  AttachThread ();

  auto NativeInputRuntime_instance =
    GetStaticFieldValue ("NativeInputRuntime", "instance", "Unity.InputSystem", "UnityEngine.InputSystem.LowLevel");

  void* params [1] = { &PollingHz };

  SK_LOGi0 (L"Setting Input Polling Hz: %f", PollingHz);

  InvokeMethod ("UnityEngine.InputSystem.LowLevel", "NativeInputRuntime", "set_pollingFrequency", 1, NativeInputRuntime_instance, "Unity.InputSystem", params);

  // We don't want garbage collection overhead on this thread just because we called a function once!
  DetachCurrentThreadIfNotNative ();
}