//
// Copyright 2019 Andon "Kaldaien" Coleman
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
#include <SpecialK/render/d3d11/d3d11_state_tracker.h>
#include <SpecialK/diagnostics/debug_utils.h>

#define OPT_VERSION_NUM L"0.1.4"
#define OPT_VERSION_STR L"Octopus Trapper v " OPT_VERSION_NUM

volatile LONG __OPT_init = FALSE;

static uint32_t ps_crc32c_dof     = 0xa0b81fbf;
static uint32_t ps_crc32c_bloom   = 0x793d2877;
static uint32_t ps_crc32c_godrays = 0x5239007b;

struct opt_cfg_s
{
  struct {
    sk::ParameterBool*  no_dof;
    sk::ParameterBool*  no_bloom;
    sk::ParameterBool*  no_godrays;
    sk::ParameterInt64* last_framelimit_bug_addr;
  } ini_params;

  struct {
    bool dof     = false;
    bool bloom   = false;
    bool godrays = false;
  } postprocess;

  struct {
    int64_t framerate_bug_addr = 0;
  };

  opt_cfg_s (void) = default;
};

SK_LazyGlobal <opt_cfg_s> opt_config;

bool
SK_OPT_PlugInCfg (void)
{
  if (ImGui::CollapsingHeader ("OCTOPATH TRAVELER", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::TreePush ("");

    ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.40f, 0.40f, 0.45f));
    ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.45f, 0.45f, 0.80f));
    ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.53f, 0.53f, 0.80f));

    if (ImGui::CollapsingHeader ("Post-Processing", ImGuiTreeNodeFlags_DefaultOpen))
    {
      ImGui::TreePush ("");

      bool changed = false;

      bool dof     = (! opt_config->postprocess.dof);
      bool bloom   = (! opt_config->postprocess.bloom);
      bool godrays = (! opt_config->postprocess.godrays);

      ImGui::BeginGroup ();

      if (ImGui::Checkbox ("Enable Depth of Field", &dof))
      {
        if (! dof)
        {
          SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.blacklist, ps_crc32c_dof);
          InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
        }

        else
        {
          SK_D3D11_Shaders->pixel.releaseTrackingRef (SK_D3D11_Shaders->pixel.blacklist, ps_crc32c_dof);
          InterlockedDecrement (&SK_D3D11_DrawTrackingReqs);
        }


        opt_config->postprocess.dof = (! dof);
        opt_config->ini_params.no_dof->store (
          opt_config->postprocess.dof
        );

        changed = true;
      }

      if (ImGui::Checkbox ("Enable Lightshafts", &godrays))
      {
        if (! godrays)
        {
          SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.blacklist, ps_crc32c_godrays);
          InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
        }

        else
        {
          SK_D3D11_Shaders->pixel.releaseTrackingRef (SK_D3D11_Shaders->pixel.blacklist, ps_crc32c_godrays);
          InterlockedDecrement (&SK_D3D11_DrawTrackingReqs);
        }


        opt_config->postprocess.godrays = (! godrays);
        opt_config->ini_params.no_godrays->store (
          opt_config->postprocess.godrays
        );

        changed = true;
      }

      ImGui::EndGroup   ();
      ImGui::SameLine   ();
      ImGui::BeginGroup ();

      bool ghost =
        ImGui::Checkbox ("Enable Ghost Foliage?", &bloom);

      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("Disables annoying shadows cast by foliage that doesn't look like it belongs in the game.");

      if (ghost)
      {
        if (! bloom)
        {
          SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.blacklist, ps_crc32c_bloom);
          InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
        }

        else
        {
          SK_D3D11_Shaders->pixel.releaseTrackingRef (SK_D3D11_Shaders->pixel.blacklist, ps_crc32c_bloom);
          InterlockedDecrement (&SK_D3D11_DrawTrackingReqs);
        }


        opt_config->postprocess.bloom = (! bloom);
        opt_config->ini_params.no_bloom->store (
          opt_config->postprocess.bloom
        );

        changed = true;
      }

      ImGui::EndGroup ();

      if (changed)
      {
        SK_GetDLLConfig ()->write (SK_GetDLLConfig ()->get_filename ());
      }

      ImGui::TreePop  ();
    }

    if (ImGui::CollapsingHeader ("Window Management##Octopus_BugPlug", ImGuiTreeNodeFlags_DefaultOpen))
    {
      bool changed = false;

      ImGui::TreePush   ("");
      ImGui::BeginGroup (  );

      if (ImGui::Checkbox ("Continue Rendering in Background",        &config.window.background_render))
        changed = true;

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip ();
        ImGui::Text         ("Allows Audio to Continue Playing while Alt-Tabbed");
        ImGui::Separator    ();
        ImGui::BulletText   ("Also Accepts Gamepad Input while Alt-Tabbed.");
        ImGui::BulletText   ("Enable to use KB&M in one application (i.e. Web Browser) and gamepad in-game.");
        ImGui::EndTooltip   ();
      }

      bool mouse_exit =
        ImGui::Checkbox ("Allow Mouse Cursor to Leave Game Window", &config.window.unconfine_cursor);

      if (ImGui::IsItemHovered ()) ImGui::SetTooltip ("For users with two monitors who would like to use their mouse for something productive.");

      if (mouse_exit)
      {
        if (config.window.unconfine_cursor)
        {
          config.window.confine_cursor   = 0;
          config.window.unconfine_cursor = 1;
        }

        else
        {
          config.window.confine_cursor   = 0;
          config.window.unconfine_cursor = 0;
        }

        SK_ImGui_AdjustCursor ();

        changed = true;
      }

      ImGui::EndGroup ();
      ImGui::SameLine ();
      ImGui::BeginGroup ();

      if (ImGui::Checkbox ("Hide Mouse Cursor", &config.input.cursor.manage))
      {
        if (config.input.cursor.manage)
        {   config.input.cursor.timeout       = 0;
            config.input.cursor.keys_activate = false;
        }

        changed = true;
      }

      ImGui::EndGroup ();

      if (changed)
      {
        SK_GetDLLConfig ()->write (SK_GetDLLConfig ()->get_filename ());
      }

      ImGui::TreePop  ();
    }

    ImGui::PopStyleColor (3);
    ImGui::TreePop ();
  }

  return true;
}

void
SK_OPT_FixUpLimiter (void)
{
  /*
  Octopath+11E5AF39 - F3 0F10 00 - movss xmm0,[rax]
  Octopath+11E5AF3D - 41 0F2F C0 - comiss xmm0,xmm8
  Octopath+11E5AF41 - 77 03      - ja Octopath+11E5AF46
  Octopath+11E5AF43 - 0F28 C6    - movaps xmm0,xmm6
  */

  uintptr_t
    addr_FramerateJitterBranch =
      opt_config->framerate_bug_addr;

  bool valid = false;

  __try {
    valid =
      (0 == memcmp (
           "\xF3\x0F\x10\x00\x41\x0F\x2F\xC0\x77\x03\x0F\x28\xC6",
           (LPVOID)addr_FramerateJitterBranch, 13 )
         );
  }

  __except (EXCEPTION_EXECUTE_HANDLER) { };

  if (! valid)
  {
    __try {
      addr_FramerateJitterBranch =
        reinterpret_cast <uintptr_t>       (
          SK_ScanAlignedEx               (
            "\xF3\x0F\x10\x00\x41\x0F\x2F\xC0\x77\x03\x0F\x28\xC6",
            13,                   nullptr,
            SK_Debug_GetImageBaseAddr ( ),
                                       1 )
                                           );
    } __except (EXCEPTION_EXECUTE_HANDLER) { return; };
  }

  LPVOID ja_instn =
    reinterpret_cast <LPVOID>
    (  ( 1 << 3 ) + addr_FramerateJitterBranch );

  __try {
    DWORD orig = 0x0;
    VirtualProtect (ja_instn,             2, PAGE_EXECUTE_READWRITE, &orig);
    memcpy         (ja_instn, "\x90\x90", 2);
    VirtualProtect (ja_instn,             2,                   orig, &orig);

    opt_config->ini_params.last_framelimit_bug_addr->store (
      addr_FramerateJitterBranch
    );

    if (! valid)
    {
      SK_GetDLLConfig ()->write (SK_GetDLLConfig ()->get_filename ());
    }
  }

  __except (EXCEPTION_EXECUTE_HANDLER)
  {
  }
}


HRESULT
STDMETHODCALLTYPE
SK_OPT_PresentFirstFrame (IUnknown* pSwapChain, UINT SyncInterval, UINT Flags)
{
  UNREFERENCED_PARAMETER (pSwapChain);
  UNREFERENCED_PARAMETER (SyncInterval);
  UNREFERENCED_PARAMETER (Flags);

  while (0 == InterlockedAdd (&__OPT_init, 0)) SK_Sleep (16);

  // Creates frametime inconsistency since there are two threads
  //   that the game is known to present the swapchain from.
  config.render.framerate.sleepless_render = false;

  SK_Thread_Create ([](LPVOID) ->
    DWORD
    {
      do {
        SK_SleepEx (15, FALSE);
      } while (SK_GetFramesDrawn () < 240);

      SK_OPT_FixUpLimiter ();

      SK_Thread_CloseSelf ();

      return 0;
    }
  );

  return S_OK;
}

void
SK_OPT_InitPlugin (void)
{
  plugin_mgr->config_fns.emplace      (SK_OPT_PlugInCfg);
  plugin_mgr->first_frame_fns.emplace (SK_OPT_PresentFirstFrame);

  SK_SetPluginName (OPT_VERSION_STR);

  SK_D3D11_DeclHUDShader_Vtx (0x71532076);
  SK_D3D11_DeclHUDShader_Vtx (0x9cb67cc4);

  static auto& opt_cfg =
    opt_config.get ();

  opt_cfg.ini_params.last_framelimit_bug_addr =
    dynamic_cast <sk::ParameterInt64 *> (
      g_ParameterFactory->create_parameter <int64_t> (L"Framerate Limiter Addr")
    );

  opt_cfg.ini_params.no_bloom =
    dynamic_cast <sk::ParameterBool *> (
      g_ParameterFactory->create_parameter <bool> (L"No Bloom")
    );

  opt_cfg.ini_params.no_dof =
    dynamic_cast <sk::ParameterBool *> (
      g_ParameterFactory->create_parameter <bool> (L"No Depth of Field")
    );

  opt_cfg.ini_params.no_godrays =
    dynamic_cast <sk::ParameterBool *> (
      g_ParameterFactory->create_parameter <bool> (L"No Godrays")
    );

  opt_cfg.ini_params.last_framelimit_bug_addr->register_to_ini (
    SK_GetDLLConfig (), L"OctopusTrapper.Framerate", L"DestabilizingBranchAddr"
  );

  opt_cfg.ini_params.no_dof->register_to_ini (
    SK_GetDLLConfig (), L"OctopusTrapper.PostProcess", L"DisableDOF"
  );

  opt_cfg.ini_params.no_bloom->register_to_ini (
    SK_GetDLLConfig (), L"OctopusTrapper.PostProcess", L"DisableBloom"
  );

  opt_cfg.ini_params.no_godrays->register_to_ini (
    SK_GetDLLConfig (), L"OctopusTrapper.PostProcess", L"DisableGodrays"
  );

  static auto& shaders =
    SK_D3D11_Shaders.get ();

  if (opt_cfg.ini_params.no_dof->load (opt_cfg.postprocess.dof))
  {
    if (opt_cfg.postprocess.dof)
    {
      shaders.pixel.addTrackingRef (shaders.pixel.blacklist, ps_crc32c_dof);
      InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
    }
  }

  if (opt_cfg.ini_params.no_bloom->load (opt_cfg.postprocess.bloom))
  {
    if (opt_cfg.postprocess.bloom)
    {
      shaders.pixel.addTrackingRef (shaders.pixel.blacklist, ps_crc32c_bloom);
      InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
    }
  }

  if (opt_cfg.ini_params.no_godrays->load (opt_cfg.postprocess.godrays))
  {
    if (opt_cfg.postprocess.godrays)
    {
      shaders.pixel.addTrackingRef (shaders.pixel.blacklist, ps_crc32c_godrays);
      InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
    }
  }

  opt_cfg.ini_params.last_framelimit_bug_addr->load (opt_cfg.framerate_bug_addr);


  InterlockedExchange (&__OPT_init, 1);
};