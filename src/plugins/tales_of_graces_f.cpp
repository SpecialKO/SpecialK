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

extern volatile LONG SK_D3D11_DrawTrackingReqs;

sk::ParameterBool* _SK_TGFix_DisableDepthOfField;
bool              __SK_TGFix_DisableDepthOfField = true;

sk::ParameterBool* _SK_TGFix_DisableBloom;
bool              __SK_TGFix_DisableBloom = false;

sk::ParameterBool* _SK_TGFix_DisableHeatHaze;
bool              __SK_TGFix_DisableHeatHaze = false;

sk::ParameterBool* _SK_TGFix_DisableFXAA;
bool              __SK_TGFix_DisableFXAA = false;

sk::ParameterBool* _SK_TGFix_SharpenOutlines;
bool              __SK_TGFix_SharpenOutlines = false;

bool
SK_TGFix_PlugInCfg (void)
{
  if (ImGui::CollapsingHeader ("Tales of Graces f Remastered", ImGuiTreeNodeFlags_DefaultOpen))
  {
    bool cfg_changed = false;

    ImGui::TreePush ("");
    ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.02f, 0.68f, 0.90f, 0.45f));
    ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.07f, 0.72f, 0.90f, 0.80f));
    ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.14f, 0.78f, 0.87f, 0.80f));

    static int  restart_reqs = 0;

    ImGui::BeginGroup ();
    if (ImGui::CollapsingHeader (ICON_FA_PORTRAIT " Post-Processing", ImGuiTreeNodeFlags_DefaultOpen |
                                                                      ImGuiTreeNodeFlags_AllowOverlap))
    {
      ImGui::TreePush ("");

      if (ImGui::Checkbox ("Disable Depth of Field", &__SK_TGFix_DisableDepthOfField))
      {
        if (! __SK_TGFix_DisableDepthOfField)
        {
          SK_D3D11_Shaders->pixel.releaseTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0x54f44c1a);
          InterlockedDecrement (&SK_D3D11_DrawTrackingReqs);
        }

        else
        {
          SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0x54f44c1a);
          InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
        }

        _SK_TGFix_DisableDepthOfField->store (__SK_TGFix_DisableDepthOfField);

        cfg_changed = true;
      }
      ImGui::SetItemTooltip ("Depth of Field may be unusually strong at DSR resolutions and when using MSAA.");

      if (ImGui::Checkbox ("Disable FXAA", &__SK_TGFix_DisableFXAA))
      {
        if (__SK_TGFix_DisableFXAA)
        {
          SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0xbe80dda2);
          SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0xef92e3e1);

          InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
          InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
        }

        else
        {
          SK_D3D11_Shaders->pixel.releaseTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0xbe80dda2);
          SK_D3D11_Shaders->pixel.releaseTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0xef92e3e1);

          InterlockedDecrement (&SK_D3D11_DrawTrackingReqs);
          InterlockedDecrement (&SK_D3D11_DrawTrackingReqs);
        }

        _SK_TGFix_DisableFXAA->store (__SK_TGFix_DisableFXAA);

        cfg_changed = true;
      }
      ImGui::SetItemTooltip ("FXAA should be disabled when using MSAA or the minimap will break.");

      if (ImGui::Checkbox ("Disable Heat Haze", &__SK_TGFix_DisableHeatHaze))
      {
        if (__SK_TGFix_DisableHeatHaze)
        {
          SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0x4ea47fea);

          InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
        }

        else
        {
          SK_D3D11_Shaders->pixel.releaseTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0x4ea47fea);

          InterlockedDecrement (&SK_D3D11_DrawTrackingReqs);
        }

        _SK_TGFix_DisableHeatHaze->store (__SK_TGFix_DisableHeatHaze);

        cfg_changed = true;
      }
      ImGui::SetItemTooltip ("Heat Haze may be unusually strong at DSR resolutions and when using MSAA.");

      if (ImGui::Checkbox ("Disable Atmospheric Bloom", &__SK_TGFix_DisableBloom))
      {
        if (__SK_TGFix_DisableBloom)
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

        _SK_TGFix_DisableBloom->store (__SK_TGFix_DisableBloom);

        cfg_changed = true;
      }

      if (ImGui::Checkbox ("Sharpen Outlines", &__SK_TGFix_SharpenOutlines))
      {
        if (__SK_TGFix_SharpenOutlines)
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

        _SK_TGFix_SharpenOutlines->store (__SK_TGFix_SharpenOutlines);

        cfg_changed = true;
      }

      if (cfg_changed)
      {
        config.utility.save_async ();
      }

      ImGui::TreePop  ();
    } ImGui::EndGroup ();

    if (SK_ImGui_HasPlayStationController ())
    {
      ImGui::SameLine   ();
      ImGui::BeginGroup ();

      if (ImGui::CollapsingHeader (ICON_FA_PLAYSTATION " PlayStation Controllers", ImGuiTreeNodeFlags_DefaultOpen |
                                                                                   ImGuiTreeNodeFlags_AllowOverlap))
      {
        ImGui::TreePush ("");

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

        ImGui::TreePop  (  );
      }

      ImGui::EndGroup    ( );
    }

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

HRESULT
STDMETHODCALLTYPE
SK_TGFix_PresentFirstFrame (IUnknown* pSwapChain, UINT SyncInterval, UINT Flags)
{
  UNREFERENCED_PARAMETER (pSwapChain);
  UNREFERENCED_PARAMETER (SyncInterval);
  UNREFERENCED_PARAMETER (Flags);

#if 0
  const auto& rb =
    SK_GetCurrentRenderBackend ();

  const float fDisplayHz =
    static_cast   <float>  (
      static_cast <double> (rb.displays [rb.active_display].signal.timing.vsync_freq.Numerator) /
      static_cast <double> (rb.displays [rb.active_display].signal.timing.vsync_freq.Denominator)
                           );

  if (config.render.framerate.target_fps <=  0.0f ||
      config.render.framerate.target_fps > 119.5f ||
      config.render.framerate.target_fps > fDisplayHz)
  {
    if (config.render.framerate.target_fps <= 0.0f)
        config.render.framerate.target_fps  = fDisplayHz;

    config.render.framerate.target_fps =
      std::min ( { std::max (config.render.framerate.target_fps, 29.95f),
                                                     fDisplayHz, 119.5f } );

    if (config.render.framerate.target_fps < 90.0f)
        config.render.framerate.target_fps = std::min (config.render.framerate.target_fps, 59.95f);

    if (config.render.framerate.target_fps < 40.0f)
        config.render.framerate.target_fps = 29.95f;

    __target_fps = config.render.framerate.target_fps;

    config.utility.save_async ();
  }
#endif

  return S_OK;
}

void
SK_TGFix_InitPlugin (void)
{
  SK_RunOnce (
    SK_D3D11_DeclHUDShader_Pix (0x1399301e);
    SK_D3D11_DeclHUDShader_Pix (0x171c9bf7);

    plugin_mgr->config_fns.emplace      (SK_TGFix_PlugInCfg);
    plugin_mgr->first_frame_fns.emplace (SK_TGFix_PresentFirstFrame);

    _SK_TGFix_DisableDepthOfField =
      _CreateConfigParameterBool ( L"TGFix.Render",
                                   L"DisableDepthOfField",  __SK_TGFix_DisableDepthOfField,
                                   L"Disable Depth of Field" );

    _SK_TGFix_DisableBloom =
      _CreateConfigParameterBool ( L"TGFix.Render",
                                   L"DisableBloom",  __SK_TGFix_DisableBloom,
                                   L"Disable Bloom Lighting" );

    _SK_TGFix_DisableHeatHaze =
      _CreateConfigParameterBool ( L"TGFix.Render",
                                   L"DisableHeatHaze",  __SK_TGFix_DisableHeatHaze,
                                   L"Disable Heat Haze" );

    _SK_TGFix_DisableFXAA =
      _CreateConfigParameterBool ( L"TGFix.Render",
                                   L"DisableFXAA",  __SK_TGFix_DisableFXAA,
                                   L"Disable FXAA" );

    _SK_TGFix_SharpenOutlines =
      _CreateConfigParameterBool ( L"TGFix.Render",
                                   L"SharpenOutlines",  __SK_TGFix_SharpenOutlines,
                                   L"Sharpen Character Outlines" );

    if (__SK_TGFix_DisableDepthOfField)
    {
      SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0x54f44c1a);

      InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
    }

    if (__SK_TGFix_DisableBloom)
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

    if (__SK_TGFix_DisableFXAA)
    {
      SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0xbe80dda2);
      SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0xef92e3e1);

      InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
      InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
    }

    if (__SK_TGFix_SharpenOutlines)
    {
      SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.on_top, 0x2dfcf950);
      SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.on_top, 0x9a0e24eb);
      SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.on_top, 0xf751273e);

      InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
      InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
      InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
    }

    if (__SK_TGFix_DisableHeatHaze)
    {
      SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0x4ea47fea);

      InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
    }
  );
}