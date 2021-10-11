//
// Copyright 2018 Andon "Kaldaien" Coleman
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

extern volatile
LONG SK_D3D11_DrawTrackingReqs;
extern volatile
LONG SK_D3D11_CBufferTrackingReqs;

extern iSK_INI*             dll_ini;


////struct SK_DQXI_UE4_Tweaks
////{
////  struct {
////    bool  enable    = false;
////    int   samples   = 0;
////    float sharpness = 0;
////
////    sk::ParameterInt   *pAASamples   = nullptr;
////    sk::ParameterFloat *pAASharpness = nullptr;
////
////    void toggle (void)
////    {
////      if (enable)
////      {
////        ue4_cfg.pINI->import (TemporalAA_Default);
////
////        pAASamples->load   (samples);
////        pAASharpness->load (sharpness);
////      }
////
////      else
////      {
////        auto& sec =
////          ue4_cfg.pINI->get_section (
////            L"/script/engine.renderersettings"
////          );
////
////        sec.remove_key (L"r.TemporalAA.Upsampling");
////        sec.remove_key (L"r.TemporalAACurrentFrameWeight");
////        sec.remove_key (L"r.TemporalAASamples");
////        sec.remove_key (L"r.TemporalAASharpness");
////        sec.remove_key (L"r.TemporalAAFilterSize");
////        sec.remove_key (L"r.TemporalAACatmullRom");
////      }
////
////      ue4_cfg.pINI->write (ue4_cfg.pINI->get_filename ());
////
////      //enable = (! enable);
////    }
////  } temporal_aa;
////
////  struct {
////    bool  enabled = false;
////    float sharpen = 0.0f;
////
////    sk::ParameterFloat *pSharpen = nullptr;
////  } tonemap;
////
////  struct {
////    int max_aniso = 1;
////
////    sk::ParameterInt* pMaxAniso = nullptr;
////  } textures;
////
////  iSK_INI* pINI = nullptr;
////
////  void init (void)
////  {
////    pINI =
////      SK_CreateINI   (
////        std::wstring (
////          SK_GetDocumentsDir () +
////            LR"(\my games\DRAGON QUEST XI\Saved\Config\WindowsNoEditor\Engine.ini)"
////        ).c_str ()
////      );
////
////    temporal_aa.pAASamples =
////      dynamic_cast <sk::ParameterInt *>
////      (
////        g_ParameterFactory->create_parameter <int> (L"Temporal AA Samples")
////      );
////    temporal_aa.pAASamples->register_to_ini (
////      pINI,
////            L"/script/engine.renderersettings",
////            L"r.TemporalAASamples"          );
////    temporal_aa.pAASamples->load (temporal_aa.samples);
////
////    temporal_aa.pAASharpness =
////      dynamic_cast <sk::ParameterFloat *>
////      (
////        g_ParameterFactory->create_parameter <float> (L"Temporal AA Sharpness")
////      );
////    temporal_aa.pAASharpness->register_to_ini (
////      pINI,
////            L"/script/engine.renderersettings",
////            L"r.TemporalAASharpness"          );
////    temporal_aa.pAASharpness->load (temporal_aa.sharpness);
////
////
////    textures.pMaxAniso =
////      dynamic_cast <sk::ParameterInt *>
////      (
////        g_ParameterFactory->create_parameter <int> (L"Texture Max Aniso.")
////      );
////    textures.pMaxAniso->register_to_ini (
////      pINI,
////            L"/script/engine.renderersettings",
////            L"r.MaxAnisotropy"          );
////    textures.pMaxAniso->load (textures.max_aniso);
////
////
////    tonemap.pSharpen =
////      dynamic_cast <sk::ParameterFloat *>
////      (
////        g_ParameterFactory->create_parameter <float> (L"Tonemapper.Sharpen")
////      );
////    tonemap.pSharpen->register_to_ini (
////      pINI,
////            L"/script/engine.renderersettings",
////            L"r.Tonemapper.Sharpen"   );
////    tonemap.pSharpen->load (tonemap.sharpen);
////
////    if (temporal_aa.samples != 0)
////    {
////      temporal_aa.enable = true;
////    }
////
////    if (tonemap.sharpen != 0.0f)
////    {
////      tonemap.enabled = true;
////    }
////  }
////} static ue4_cfg;

bool
SK_DQXI_PlugInCfg (void);


bool
__SK_DQXI_MakeAsyncObjectsGreatAgain = true;

sk::ParameterBool* _SK_DQXI_IgnoreExitKeys;
bool __SK_DQXI_IgnoreExit = true;

sk::ParameterBool* _SK_DQXI_AliasArrowsAndWASD;
bool __SK_DQXI_AliasArrowsAndWASD = true;

extern SK_LazyGlobal <concurrency::concurrent_vector <d3d11_shader_tracking_s::cbuffer_override_s>> __SK_D3D11_PixelShader_CBuffer_Overrides;
d3d11_shader_tracking_s::cbuffer_override_s* SK_DQXI_CB_Override;

#define SK_DQXI_HDR_SECTION     L"DragonQuestXI.HDR"
#define SK_DQXI_MISC_SECTION    L"DragonQuestXI.Misc"

auto DeclKeybind =
[](SK_ConfigSerializedKeybind* binding, iSK_INI* ini, const wchar_t* sec) ->
sk::ParameterStringW*
{
  if (binding == nullptr || ini == nullptr || sec == nullptr)
    return nullptr;

  auto* ret =
    dynamic_cast <sk::ParameterStringW *>
    (g_ParameterFactory->create_parameter <std::wstring> (L"DESCRIPTION HERE"));

  if (ret != nullptr)
    ret->register_to_ini ( ini, sec, binding->short_name );

  return ret;
};

static
SK_ConfigSerializedKeybind&
escape_keybind_getter (void)
{
  static SK_ConfigSerializedKeybind
    keybind = {
      SK_Keybind {
        "Remap Escape Key", L"Backspace",
         { FALSE, FALSE, FALSE }, VK_BACK
      }, L"RemapEscape"
    };

  return keybind;
};

#define escape_keybind escape_keybind_getter()

extern bool
SK_ImGui_HandlesMessage (LPMSG lpMsg, bool, bool);

typedef bool (*SK_WindowMessageFilter_pfn)(LPMSG, bool, bool);
               SK_WindowMessageFilter_pfn
               SK_WindowMessageFilter = nullptr;

bool
SK_DQXI_WindowMessageFilter (LPMSG lpMsg, bool bReserved0, bool bReserved1)
{
  switch (lpMsg->message)
  {
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    {
      bool key_release = ( lpMsg->message == WM_KEYUP ||
                           lpMsg->message == WM_SYSKEYUP );

      if (__SK_DQXI_AliasArrowsAndWASD)
      {
        BYTE vKey        = 0;

        switch (lpMsg->wParam)
        {
          case 'W':
            vKey = VK_UP;
            break;
          case 'A':
            vKey = VK_LEFT;
            break;
          case 'S':
            vKey = VK_DOWN;
            break;
          case 'D':
            vKey = VK_RIGHT;
            break;

          case VK_UP:
            vKey = 'W';
            break;
          case VK_LEFT:
            vKey = 'A';
            break;
          case VK_DOWN:
            vKey = 'S';
            break;
          case VK_RIGHT:
            vKey = 'D';
            break;
        }

        if (vKey != 0)
        {
          if (! key_release)
          {
            game_window.WndProc_Original (lpMsg->hwnd, WM_KEYDOWN, vKey, lpMsg->lParam);
          //keybd_event_Original (vKey, 0, 0x0,             0);
          }

          else
          {
            game_window.WndProc_Original (lpMsg->hwnd, WM_KEYUP, vKey, lpMsg->lParam);
          //keybd_event_Original (vKey, 0, KEYEVENTF_KEYUP, 0);
          }
        }
      }

      if (__SK_DQXI_IgnoreExit)
      {
        if ( lpMsg->wParam == VK_ESCAPE ||
             lpMsg->wParam == VK_F4        )
        {
          if (lpMsg->wParam == VK_ESCAPE)
          {
            SK_WindowMessageFilter (lpMsg, bReserved0, bReserved1);

            if (! key_release)
            {
              keybd_event_Original ((BYTE)escape_keybind.vKey, 0, 0x0,             0);
            }

            else
            {
              keybd_event_Original ((BYTE)escape_keybind.vKey, 0, KEYEVENTF_KEYUP, 0);
            }

            return true;
          }

          else
          {
            SK_WindowMessageFilter (lpMsg, bReserved0, bReserved1);

            return true;
          }
        }
      }
    }
  }

  return
    SK_WindowMessageFilter (lpMsg, bReserved0, bReserved1);
}

void
SK_DQXI_PlugInInit (void)
{
  plugin_mgr->config_fns.emplace (SK_DQXI_PlugInCfg);


    SK_CreateFuncHook (      L"SK_ImGui_HandlesMessage",
                               SK_ImGui_HandlesMessage,
                                 SK_DQXI_WindowMessageFilter,
             static_cast_p2p <void> (&SK_WindowMessageFilter) );
  MH_QueueEnableHook   (        SK_ImGui_HandlesMessage       );
  SK_ApplyQueuedHooks  ();

  escape_keybind.param =
    DeclKeybind (&escape_keybind, SK_GetDLLConfig (), SK_DQXI_MISC_SECTION);

  if (! escape_keybind.param->load (escape_keybind.human_readable))
  {
    escape_keybind.human_readable = L"Backspace";
  }

  escape_keybind.parse ();
  escape_keybind.param->store (escape_keybind.human_readable);

  SK_GetDLLConfig   ()->write (
    SK_GetDLLConfig ()->get_filename ()
  );

  config.render.framerate.enable_mmcss = false;

  _SK_DQXI_IgnoreExitKeys =
    _CreateConfigParameterBool ( SK_DQXI_MISC_SECTION,
                                L"DisableExitKeys",  __SK_DQXI_IgnoreExit,
                                L"Prevent Exit Confirmations" );

  _SK_DQXI_AliasArrowsAndWASD =
    _CreateConfigParameterBool ( SK_DQXI_MISC_SECTION,
                                L"AliasArrowsAndWASD", __SK_DQXI_AliasArrowsAndWASD,
                                L"Allow Arrow Keys and WASD to serve the same function" );

  iSK_INI* pINI =
    SK_GetDLLConfig ();

  pINI->write (pINI->get_filename ());

  std::unordered_set <uint32_t>
    __SK_DQXI_UI_Vtx_Shaders =
    {
      0x56f732c6, 0xf3c19cc1,
      0x6f046ebc, 0x711c9eeb
    };

  std::unordered_set <uint32_t>
    __SK_DQXI_UI_Pix_Shaders =
    {
      0x0fbd3754, 0x26c739a9, 0x7a28c784,
      0x7dc782b6, 0xd95be234
    };


  for (               auto  it : __SK_DQXI_UI_Vtx_Shaders)
  SK_D3D11_DeclHUDShader   (it,        ID3D11VertexShader);

  for (               auto  it : __SK_DQXI_UI_Pix_Shaders)
  SK_D3D11_DeclHUDShader   (it,         ID3D11PixelShader);
}

static auto
Keybinding = [] (SK_Keybind* binding, sk::ParameterStringW* param) ->
auto
{
  std::string label =
    SK_WideCharToUTF8 (binding->human_readable) + "##";

  label += binding->bind_name;

  if (SK_ImGui_KeybindSelect (binding, label.c_str ()))
    ImGui::OpenPopup (        binding->bind_name);

  std::wstring original_binding =
    binding->human_readable;

  SK_ImGui_KeybindDialog (binding);

  if (original_binding != binding->human_readable)
  {
    param->store (binding->human_readable);

    SK_SaveConfig ();

    return true;
  }

  return false;
};

bool
SK_DQXI_PlugInCfg (void)
{
  ////iSK_INI* pINI =
  ////  SK_GetDLLConfig ();

  static auto& rb =
    SK_GetCurrentRenderBackend ();

  if ( ImGui::CollapsingHeader (
         (const char *)u8R"(DRAGON QUEST® XI: Echoes of an Elusive Age™)",
           ImGuiTreeNodeFlags_DefaultOpen
                               )
     )
  {
    bool hdr_open =
      SK_ImGui_Widgets->hdr_control->isVisible ();

    if ( ImGui::Button ( hdr_open ? "Close the stupid HDR Widget!" :
                                    "Open the astonishing HDR Widget!" ) )
    {
      SK_ImGui_Widgets->hdr_control->setVisible (! hdr_open);

      if (! hdr_open)
      {
        SK_RunOnce (
          SK_ImGui_Warning ( L"Congratulations: You Opened a Widget -- the Hard Way!\n\n\t"
                             L"Pro Tip:\tRight-click the Widget and then Assign a Toggle Keybind!" )
        );
      }
    }

    ImGui::TreePush ("");
    //ImGui::Checkbox ("Prevent UE4 from Using ID3D11Async Synchronously",
    //                   &__SK_DQXI_MakeAsyncObjectsGreatAgain );
    //
    //ImGui::SameLine ();
    //
    //                             extern bool SK_Shenmue_UseNtDllQPC;
    //ImGui::Checkbox ("Use user-mode timer", &SK_Shenmue_UseNtDllQPC);
    //
    //if (ImGui::IsItemHovered ())
    //{
    //  ImGui::BeginTooltip    ();
    //  ImGui::TextUnformatted ("Optimization strategy for Shenmue I & II");
    //  ImGui::Separator       ();
    //  ImGui::BulletText      ("The option is included in this game because the feature needs more rigorous testing.");
    //  ImGui::BulletText      ("I do not expect any performance gains/losses but possibly weird timing glitches.");
    //  ImGui::EndTooltip      ();
    //}

    ////if ( ue4_cfg.pINI != nullptr &&
    ////       ImGui::CollapsingHeader (
    ////         (const char *)u8"Unreal® Engine 4:  Advanced Engine Tuning",
    ////           ImGuiTreeNodeFlags_DefaultOpen
    ////                               )
    ////   )
    ////{
    ////  ImGui::TreePush ("");
    ////
    ////  static bool need_restart = false;
    ////
    ////  if (ImGui::Checkbox ("Temporal Anti-Aliasing", &ue4_cfg.temporal_aa.enable))
    ////  {
    ////    ue4_cfg.temporal_aa.toggle ();
    ////  }
    ////
    ////  if (ue4_cfg.temporal_aa.enable)
    ////  {
    ////    bool changed = false;
    ////
    ////    ImGui::SameLine   ();
    ////    ImGui::BeginGroup ();
    ////
    ////    int quality_item =
    ////      ( ue4_cfg.temporal_aa.samples <= 16 ? 0 :
    ////        ue4_cfg.temporal_aa.samples <= 32 ? 1 : 2 );
    ////
    ////    changed |=
    ////      ImGui::Combo ( "Temporal AA Quality", &quality_item,
    ////                     "Low (16)\0Medium (32)\0High (64)\0\0" );
    ////    if (changed)
    ////    {
    ////      ue4_cfg.temporal_aa.samples =
    ////       ( quality_item == 0 ? 16 :
    ////         quality_item == 1 ? 32 : 64 );
    ////    }
    ////
    ////    changed |=
    ////      ImGui::SliderFloat ( "Temporal AA Sharpness",
    ////                             &ue4_cfg.temporal_aa.sharpness,
    ////                               0.0f, 1.0f );
    ////
    ////    ImGui::EndGroup   ();
    ////
    ////    if (changed)
    ////    {
    ////      need_restart = true;
    ////
    ////      ue4_cfg.temporal_aa.pAASamples->store   (ue4_cfg.temporal_aa.samples);
    ////      ue4_cfg.temporal_aa.pAASharpness->store (ue4_cfg.temporal_aa.sharpness);
    ////
    ////      ue4_cfg.pINI->write (ue4_cfg.pINI->get_filename ());
    ////    }
    ////  }
    ////
    ////  if (ImGui::Checkbox ("Enhanced Tonemap Quality", &ue4_cfg.tonemap.enabled))
    ////  {
    ////    need_restart = true;
    ////
    ////    if (ue4_cfg.tonemap.enabled)
    ////    {
    ////      ue4_cfg.pINI->import (Tonemap_Default);
    ////      ue4_cfg.pINI->write  (ue4_cfg.pINI->get_filename ());
    ////
    ////      ue4_cfg.tonemap.pSharpen->load (ue4_cfg.tonemap.sharpen);
    ////    }
    ////
    ////    else
    ////    {
    ////      auto& sec =
    ////        ue4_cfg.pINI->get_section (
    ////          L"[/script/engine.renderersettings]"
    ////        );
    ////
    ////      sec.remove_key (L"r.EyeAdaptationQuality");
    ////      sec.remove_key (L"r.Tonemapper.Sharpen");
    ////      sec.remove_key (L"r.Tonemapper.GrainQuantization");
    ////
    ////      ue4_cfg.pINI->write (ue4_cfg.pINI->get_filename ());
    ////    }
    ////  }
    ////
    ////  if (ImGui::SliderInt ("Maximum Texture Anisotropy", &ue4_cfg.textures.max_aniso, 1, 16))
    ////  {
    ////    need_restart = true;
    ////
    ////    ue4_cfg.textures.pMaxAniso->store (ue4_cfg.textures.max_aniso);
    ////    ue4_cfg.pINI->write (ue4_cfg.pINI->get_filename ());
    ////  }
    ////
    ////  if (need_restart)
    ////  {
    ////    ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (.3f, .8f, .9f));
    ////    ImGui::BulletText     ("Game Restart Required");
    ////    ImGui::PopStyleColor  ();
    ////  }
    ////
    ////  ImGui::TreePop ();
    ////}
    ////
    ////ImGui::Separator ();

    if (config.steam.screenshots.enable_hook)
    {
      ImGui::PushID    ("DQXI_Screenshots");
      ImGui::Separator ();

      static std::set <SK_ConfigSerializedKeybind *>
        keybinds = {
        &config.screenshots.game_hud_free_keybind
      };

      ImGui::SameLine   ();
      ImGui::BeginGroup ();
      for ( auto keybind : keybinds )
      {
        ImGui::Text          ( "%s:  ",
                              keybind->bind_name );
      }
      ImGui::EndGroup   ();
      ImGui::SameLine   ();
      ImGui::BeginGroup ();
      for ( auto keybind : keybinds )
      {
        Keybinding ( keybind, keybind->param );
      }
      ImGui::EndGroup   ();

      bool png_changed = false;

      if (config.steam.screenshots.enable_hook)
      {
        png_changed =
          ImGui::Checkbox ( "Keep Lossless .PNG Screenshots",
                           &config.screenshots.png_compress      );
      }

      if ( rb.screenshot_mgr.getRepoStats ().files > 0 )
      {
        ImGui::SameLine ();

        const SK_ScreenshotManager::screenshot_repository_s& repo =
          rb.screenshot_mgr.getRepoStats (png_changed);

        ImGui::BeginGroup (  );
        ImGui::TreePush   ("");
        ImGui::Text ( "%ui files using %ws",
                     repo.files,
                     SK_File_SizeToString (repo.liSize.QuadPart).data ()
        );

        if (ImGui::IsItemHovered ())
        {
          ImGui::SetTooltip ( "Steam does not support .png screenshots, so "
                             "SK maintains its own storage for lossless screenshots." );
        }

        ImGui::SameLine ();

        if (ImGui::Button ("Browse"))
        {
          SK_ShellExecuteW ( nullptr,
                              L"explore",
                                rb.screenshot_mgr.getBasePath (),
                                  nullptr, nullptr,
                                    SW_NORMAL
          );
        }

        ImGui::TreePop  ();
        ImGui::EndGroup ();
      }
      ImGui::PopID      ();
    }

    if (ImGui::CollapsingHeader ("Input Processing", ImGuiTreeNodeFlags_DefaultOpen))
    {
      ImGui::TreePush ("");

      if (ImGui::Checkbox ("Remap Escape Key", &__SK_DQXI_IgnoreExit))
      {
        _SK_DQXI_IgnoreExitKeys->store (__SK_DQXI_IgnoreExit);
      }

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip ();
        ImGui::Text         ("Disables the windows dialog asking you to exit");
        ImGui::Separator    ();
        ImGui::BulletText   ("You can still fast-exit using Alt+F4");
        ImGui::BulletText   ("Special K's Alt+F4 confirmation is presented in-game!");
        ImGui::EndTooltip   ();
      }

      if (__SK_DQXI_IgnoreExit)
      {
        ImGui::SameLine (); ImGui::Spacing ();
        ImGui::SameLine (); ImGui::Spacing ();
        ImGui::SameLine (); ImGui::Spacing ();
        ImGui::SameLine ();
        ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.66f, 0.66f, 0.66f, 1.f));
        ImGui::TextUnformatted("Remapped Key: ");
        ImGui::PopStyleColor  ();
        ImGui::SameLine       ();
        Keybinding ( &escape_keybind,
                    (&escape_keybind)->param );
      }

      if (ImGui::Checkbox ("Alias WASD and Arrow Keys", &__SK_DQXI_AliasArrowsAndWASD))
      {
        _SK_DQXI_AliasArrowsAndWASD->store (__SK_DQXI_AliasArrowsAndWASD);
      }

      ImGui::TreePop ();
    }

    ImGui::TreePop ();

    return true;
  }

  return false;
}