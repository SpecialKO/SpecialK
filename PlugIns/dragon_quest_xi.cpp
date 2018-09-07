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

#include <imgui/imgui.h>

#include <SpecialK/config.h>
#include <SpecialK/parameter.h>
#include <SpecialK/control_panel.h>
#include <SpecialK/render/dxgi/dxgi_backend.h>

extern volatile
LONG SK_D3D11_DrawTrackingReqs;
extern volatile
LONG SK_D3D11_CBufferTrackingReqs;

#include <SpecialK/steam_api.h>

extern iSK_INI*             dll_ini;
extern sk::ParameterFactory g_ParameterFactory;


volatile LONG __SK_DQXI_QueuedShots         = 0;
volatile LONG __SK_DQXI_InitiateHudFreeShot = 0;

extern
volatile LONG __SK_ScreenShot_CapturingHUDless;

#include <SpecialK\widgets\widget.h>


static
std::unordered_set <uint32_t>
  __SK_DQXI_UI_Vtx_Shaders =
  {
    0x6f046ebc, 0x711c9eeb
  };

static
std::unordered_set <uint32_t>
  __SK_DQXI_UI_Pix_Shaders =
  {
    0x0fbd3754, 0x26c739a9,
    0x7dc782b6, 0xd95be234
  };

void
SK_DQXI_BeginFrame (void)
{
  if ( ReadAcquire (&__SK_DQXI_QueuedShots)          > 0 ||
       ReadAcquire (&__SK_DQXI_InitiateHudFreeShot) != 0    )
  {
        InterlockedExchange        (&__SK_ScreenShot_CapturingHUDless, 1);
    if (InterlockedCompareExchange (&__SK_DQXI_InitiateHudFreeShot, -2, 1) == 1)
    {
      static auto& shaders =
        SK_D3D11_Shaders;

      for ( auto& it : __SK_DQXI_UI_Vtx_Shaders )
      {  shaders.vertex.blacklist.emplace (it); }

      for ( auto& it : __SK_DQXI_UI_Pix_Shaders )
      {  shaders.pixel.blacklist.emplace (it);  }
    }

    // 1-frame Delay for SDR->HDR Upconversion
    else if (InterlockedCompareExchange (&__SK_DQXI_InitiateHudFreeShot, -1, -2) == -2)
    {
      SK::SteamAPI::TakeScreenshot (SK::ScreenshotStage::EndOfFrame);
      InterlockedDecrement (&SK_D3D11_DrawTrackingReqs);
    }

    else if (! ReadAcquire (&__SK_DQXI_InitiateHudFreeShot))
    {
      InterlockedDecrement (&__SK_DQXI_QueuedShots);
      InterlockedExchange  (&__SK_DQXI_InitiateHudFreeShot, 1);

      return
        SK_DQXI_BeginFrame ();
    }

    return;
  }

  InterlockedExchange (&__SK_ScreenShot_CapturingHUDless, 0);
}

void
SK_DQXI_EndFrame (void)
{
  if (InterlockedCompareExchange (&__SK_DQXI_InitiateHudFreeShot, 0, -1) == -1)
  {
    static auto& shaders =
      SK_D3D11_Shaders;

    for ( auto& it : __SK_DQXI_UI_Vtx_Shaders )
    {  shaders.vertex.blacklist.erase (it); }

    for ( auto& it : __SK_DQXI_UI_Pix_Shaders )
    {  shaders.pixel.blacklist.erase (it);  }
  }
}


sk::ParameterBool* _SK_DQXI_10BitSwapChain;
bool __SK_DQXI_10BitSwap = false;

sk::ParameterBool* _SK_DQXI_16BitSwapChain;
bool __SK_DQXI_16BitSwap = false;

float __SK_DQXI_HDR_Luma = 300.0_Nits;
float __SK_DQXI_HDR_Exp  = 1.0f;

sk::ParameterInt* _SK_DQXI_ActiveHDRPreset;
int __SK_DQXI_HDRPreset = 0;

sk::ParameterBool* _SK_DQXI_IgnoreExitKeys;
bool __SK_DQXI_IgnoreExit = true;

sk::ParameterBool* _SK_DQXI_AliasArrowsAndWASD;
bool __SK_DQXI_AliasArrowsAndWASD = true;

#include <concurrent_vector.h>
extern concurrency::concurrent_vector <d3d11_shader_tracking_s::cbuffer_override_s> __SK_D3D11_PixelShader_CBuffer_Overrides;
d3d11_shader_tracking_s::cbuffer_override_s* SK_DQXI_CB_Override;

#include <SpecialK/plugin/plugin_mgr.h>

#define SK_DQXI_HDR_SECTION     L"DragonQuestXI.HDR"
#define SK_DQXI_MISC_SECTION    L"DragonQuestXI.Misc"

#include <SpecialK/ini.h>
#include <SpecialK/config.h>

auto DeclKeybind =
[](SK_ConfigSerializedKeybind* binding, iSK_INI* ini, const wchar_t* sec) ->
auto
{
  auto* ret =
    dynamic_cast <sk::ParameterStringW *>
    (g_ParameterFactory.create_parameter <std::wstring> (L"DESCRIPTION HERE"));

  ret->register_to_ini ( ini, sec, binding->short_name );

  return ret;
};


#define MAX_HDR_PRESETS 4

struct SK_HDR_Preset_s {
  const char*  preset_name;
  int          preset_idx;

  float        peak_white_nits;
  float        eotf;

  std::wstring annotation = L"";

  sk::ParameterFloat*   cfg_nits    = nullptr;
  sk::ParameterFloat*   cfg_eotf    = nullptr;
  sk::ParameterStringW* cfg_notes   = nullptr;

  SK_ConfigSerializedKeybind
    preset_activate = {
      SK_Keybind {
        preset_name, L"",
          false, true, true, '0'
      }, L"Activate"
    };

  int activate (void)
  {
    __SK_DQXI_HDRPreset =
      preset_idx;

    __SK_DQXI_HDR_Luma = peak_white_nits;
    __SK_DQXI_HDR_Exp  = eotf;

    if (_SK_DQXI_ActiveHDRPreset != nullptr)
    {   _SK_DQXI_ActiveHDRPreset->store (preset_idx);

      SK_GetDLLConfig   ()->write (
        SK_GetDLLConfig ()->get_filename ()
                                  );
    }

    return preset_idx;
  }

  void setup (void)
  {
    if (cfg_nits == nullptr)
    {
      //preset_activate =
      //  std::move (
      //    SK_ConfigSerializedKeybind {
      //      SK_Keybind {
      //        preset_name, L"",
      //          false, true, true, '0'
      //      }, L"Activate"
      //    }
      //  );

      cfg_nits =
        _CreateConfigParameterFloat ( SK_DQXI_HDR_SECTION,
                   SK_FormatStringW (L"scRGBLuminance_[%lu]", preset_idx).c_str (),
                    peak_white_nits, L"scRGB Luminance" );
      cfg_eotf =
        _CreateConfigParameterFloat ( SK_DQXI_HDR_SECTION,
                   SK_FormatStringW (L"scRGBGamma_[%lu]",     preset_idx).c_str (),
                               eotf, L"scRGB Gamma" );

      wcsncpy_s ( preset_activate.short_name,                           32,
        SK_FormatStringW (L"Activate%lu", preset_idx).c_str (), _TRUNCATE );

      preset_activate.param =
        DeclKeybind (&preset_activate, SK_GetDLLConfig (), L"HDR.Presets");

      if (! preset_activate.param->load (preset_activate.human_readable))
      {
        preset_activate.human_readable =
          SK_FormatStringW (L"Alt+Shift+%lu", preset_idx);
      }

      preset_activate.parse ();
      preset_activate.param->store (preset_activate.human_readable);

      SK_GetDLLConfig   ()->write (
        SK_GetDLLConfig ()->get_filename ()
                                  );
    }
  }
} static
hdr_presets [4] = { { "HDR Preset 0", 0, 300.0_Nits, 1.0f, L"F1" },
                    { "HDR Preset 1", 1, 300.0_Nits, 1.0f, L"F2" },
                    { "HDR Preset 2", 2, 300.0_Nits, 1.0f, L"F3" },
                    { "HDR Preset 3", 3, 300.0_Nits, 1.0f, L"F4" } };


SK_ConfigSerializedKeybind
  escape_keybind = {
    SK_Keybind {
      "Remap Escape Key", L"Backspace",
        false, false, false, VK_BACK
    }, L"RemapEscape"
  };


typedef void (CALLBACK *SK_PluginKeyPress_pfn)(BOOL Control, BOOL Shift, BOOL Alt, BYTE vkCode);
                        SK_PluginKeyPress_pfn
                        SK_PluginKeyPress_Original = nullptr;

void
CALLBACK
SK_DQXI_PluginKeyPress (BOOL Control, BOOL Shift, BOOL Alt, BYTE vkCode)
{
#define SK_MakeKeyMask(vKey,ctrl,shift,alt) \
  static_cast <UINT>((vKey) | (((ctrl) != 0) <<  9) |   \
                              (((shift)!= 0) << 10) |   \
                              (((alt)  != 0) << 11))

  for ( auto& it : hdr_presets )
  {
    if ( it.preset_activate.masked_code ==
           SK_MakeKeyMask ( vkCode,
                            Control != 0 ? 1 : 0,
                            Shift   != 0 ? 1 : 0,
                            Alt     != 0 ? 1 : 0 ) )
    {
      it.activate ();
      return;
    }     
  }

  return
    SK_PluginKeyPress_Original (
      Control, Shift, Alt, vkCode
    );
}

extern bool
SK_ImGui_HandlesMessage (LPMSG lpMsg, bool, bool);

typedef bool (*SK_WindowMessageFilter_pfn)(LPMSG, bool, bool);
               SK_WindowMessageFilter_pfn
               SK_WindowMessageFilter = nullptr;

#include <SpecialK/window.h>

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
  hdr_presets [0].setup (); hdr_presets [1].setup ();
  hdr_presets [2].setup (); hdr_presets [3].setup ();

  extern void WINAPI SK_PluginKeyPress (BOOL,BOOL,BOOL,BYTE);
  SK_CreateFuncHook (      L"SK_PluginKeyPress",
                             SK_PluginKeyPress,
                        SK_DQXI_PluginKeyPress,
    static_cast_p2p <void> (&SK_PluginKeyPress_Original) );
  MH_QueueEnableHook   (     SK_PluginKeyPress           );

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

  _SK_DQXI_10BitSwapChain =
    _CreateConfigParameterBool ( SK_DQXI_HDR_SECTION,
                                L"Use10BitSwapChain",  __SK_DQXI_10BitSwap,
                                L"10-bit SwapChain" );
  _SK_DQXI_16BitSwapChain =
    _CreateConfigParameterBool ( SK_DQXI_HDR_SECTION,
                                L"Use16BitSwapChain",  __SK_DQXI_16BitSwap,
                                L"16-bit SwapChain" );

  _SK_DQXI_ActiveHDRPreset =
    _CreateConfigParameterInt ( SK_DQXI_HDR_SECTION,
                                L"Preset",           __SK_DQXI_HDRPreset,
                                L"Light Adaptation Preset" );

  _SK_DQXI_IgnoreExitKeys =
    _CreateConfigParameterBool ( SK_DQXI_MISC_SECTION,
                                L"DisableExitKeys",  __SK_DQXI_IgnoreExit,
                                L"Prevent Exit Confirmations" );

  _SK_DQXI_AliasArrowsAndWASD =
    _CreateConfigParameterBool ( SK_DQXI_MISC_SECTION,
                                L"AliasArrowsAndWASD", __SK_DQXI_AliasArrowsAndWASD,
                                L"Allow Arrow Keys and WASD to serve the same function" );

  if ( __SK_DQXI_HDRPreset < 0 ||
       __SK_DQXI_HDRPreset >= MAX_HDR_PRESETS )
  {
    __SK_DQXI_HDRPreset = 0;
  }

  hdr_presets [__SK_DQXI_HDRPreset].activate ();

  iSK_INI* pINI =
    SK_GetDLLConfig ();

  pINI->write (pINI->get_filename ());
}

static auto
Keybinding = [] (SK_Keybind* binding, sk::ParameterStringW* param) ->
auto
{
  std::string label =
    SK_WideCharToUTF8 (binding->human_readable) + "###";

  label += binding->bind_name;

  if (ImGui::Selectable (label.c_str (), false))
  {
    ImGui::OpenPopup (binding->bind_name);
  }

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
  iSK_INI* pINI =
    SK_GetDLLConfig ();

  if ( ImGui::CollapsingHeader (
         u8R"(DRAGON QUEST® XI: Echoes of an Elusive Age™)",
           ImGuiTreeNodeFlags_DefaultOpen
                               )
     )
  {
    ImGui::TreePush ("");

                                 extern bool SK_Shenmue_UseNtDllQPC;
    ImGui::Checkbox ("Use user-mode timer", &SK_Shenmue_UseNtDllQPC);

    if (ImGui::IsItemHovered ())
    {
      ImGui::BeginTooltip    ();
      ImGui::TextUnformatted ("Optimization strategy for Shenmue I & II");
      ImGui::Separator       ();
      ImGui::BulletText      ("The option is included in this game because the feature needs more rigorous testing.");
      ImGui::BulletText      ("I do not expect any performance gains/losses but possibly weird timing glitches.");
      ImGui::EndTooltip      ();
    }

    if (config.steam.screenshots.enable_hook)
    {
      ImGui::PushID    ("DQXI_Screenshots");
      ImGui::Separator ();

      static std::set <SK_ConfigSerializedKeybind *>
        keybinds = {
        &config.steam.screenshots.game_hud_free_keybind
      };

      ImGui::SameLine   ();
      ImGui::BeginGroup ();
      for ( auto& keybind : keybinds )
      {
        ImGui::Text          ( "%s:  ",
                              keybind->bind_name );
      }
      ImGui::EndGroup   ();
      ImGui::SameLine   ();
      ImGui::BeginGroup ();
      for ( auto& keybind : keybinds )
      {
        Keybinding ( keybind, keybind->param );
      }
      ImGui::EndGroup   ();

      bool png_changed = false;

      if (config.steam.screenshots.enable_hook)
      {
        png_changed =
          ImGui::Checkbox ( "Keep Lossless .PNG Screenshots",
                           &config.steam.screenshots.png_compress      );
      }

      if ( ( screenshot_manager != nullptr &&
          screenshot_manager->getExternalScreenshotRepository ().files > 0 ) )
      {
        ImGui::SameLine ();

        const SK_Steam_ScreenshotManager::screenshot_repository_s& repo =
          screenshot_manager->getExternalScreenshotRepository (png_changed);

        ImGui::BeginGroup (  );
        ImGui::TreePush   ("");
        ImGui::Text ( "%lu files using %ws",
                     repo.files,
                     SK_File_SizeToString (repo.liSize.QuadPart).c_str  ()
        );

        if (ImGui::IsItemHovered ())
        {
          ImGui::SetTooltip ( "Steam does not support .png screenshots, so "
                             "SK maintains its own storage for lossless screenshots." );
        }

        ImGui::SameLine ();

        if (ImGui::Button ("Browse"))
        {
          ShellExecuteW ( GetActiveWindow (),
                           L"explore",
                             screenshot_manager->getExternalScreenshotPath (),
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

    bool bRetro =
      ImGui::CollapsingHeader ("HDR Retrofit", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowOverlapMode);

    static bool TenBitSwap_Original     = __SK_DQXI_10BitSwap;
    static bool SixteenBitSwap_Original = __SK_DQXI_16BitSwap;

    static int sel = __SK_DQXI_16BitSwap ? 2 :
      __SK_DQXI_10BitSwap ? 1 : 0;

        ImGui::SameLine ();
    if (ImGui::RadioButton ("None", &sel, 0))
    {
      __SK_DQXI_10BitSwap = false;
      __SK_DQXI_16BitSwap = false;

      _SK_DQXI_10BitSwapChain->store (__SK_DQXI_10BitSwap);
      _SK_DQXI_16BitSwapChain->store (__SK_DQXI_16BitSwap);

      pINI->write (pINI->get_filename ());
    }

        ImGui::SameLine ();
    if (ImGui::RadioButton ("scRGB HDR (16-bit)", &sel, 2))
    {
      __SK_DQXI_16BitSwap = true;

      if (__SK_DQXI_16BitSwap) __SK_DQXI_10BitSwap = false;

      _SK_DQXI_10BitSwapChain->store (__SK_DQXI_10BitSwap);
      _SK_DQXI_16BitSwapChain->store (__SK_DQXI_16BitSwap);

      pINI->write (pINI->get_filename ());
    }

    if (bRetro)
    {
      ////if (ImGui::RadioButton ("HDR10 (10-bit + Metadata)", &sel, 1))
      ////{
      ////  __SK_MHW_10BitSwap = true;
      ////
      ////  if (__SK_MHW_10BitSwap) __SK_MHW_16BitSwap = false;
      ////
      ////  _SK_MHW_10BitSwapChain->store (__SK_MHW_10BitSwap);
      ////  _SK_MHW_16BitSwapChain->store (__SK_MHW_16BitSwap);
      ////
      ////  pINI->write (pINI->get_filename ());
      ////}

      auto& rb =
        SK_GetCurrentRenderBackend ();

      if (rb.hdr_capable)
      {
        ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.23f, 0.86f, 1.f));
        ImGui::Text ("NOTE: ");
        ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.4f, 0.28f, 0.94f)); ImGui::SameLine ();
        ImGui::Text ("This is far from a final Magic-HDR solution!");
        ImGui::PopStyleColor  (2);
        
        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip ();
          ImGui::Text         (" Future Work");
          ImGui::Separator    ();
          ImGui::BulletText   ("Automagic Dark and Light Adaptation are VERY Important and VERY Absent :P");
          ImGui::BulletText   ("Separable HUD luminance (especially weapon cross-hairs) also must be implemented");
          ImGui::EndTooltip   ();
        }
      }

      if ( (TenBitSwap_Original     != __SK_DQXI_10BitSwap ||
            SixteenBitSwap_Original != __SK_DQXI_16BitSwap) )
      {
        ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (.3f, .8f, .9f));
        ImGui::BulletText     ("Game Restart Required");
        ImGui::PopStyleColor  ();
      }

      if ((__SK_DQXI_10BitSwap || __SK_DQXI_16BitSwap) && rb.hdr_capable)
      {
        CComQIPtr <IDXGISwapChain4> pSwap4 (rb.swapchain);

        if (pSwap4 != nullptr)
        {
          DXGI_OUTPUT_DESC1     out_desc = { };
          DXGI_SWAP_CHAIN_DESC swap_desc = { };
          pSwap4->GetDesc (&swap_desc);

          if (out_desc.BitsPerColor == 0)
          {
            CComPtr <IDXGIOutput>
              pOutput = nullptr;

            if ( SUCCEEDED (
                  pSwap4->GetContainingOutput (&pOutput.p)
                           ) 
               )
            {
              CComQIPtr <IDXGIOutput6> pOutput6 (pOutput);
                                       pOutput6->GetDesc1 (&out_desc);
            }

            else
            {
              out_desc.BitsPerColor = 8;
            }
          }

          if (out_desc.BitsPerColor >= 10)
          {
            //const DisplayChromacities& Chroma = DisplayChromacityList[selectedChroma];
            DXGI_HDR_METADATA_HDR10 HDR10MetaData = {};

            static int cspace = 1;

            struct DisplayChromacities
            {
              float RedX;
              float RedY;
              float GreenX;
              float GreenY;
              float BlueX;
              float BlueY;
              float WhiteX;
              float WhiteY;
            } const DisplayChromacityList [] =
            {
              { 0.64000f, 0.33000f, 0.30000f, 0.60000f, 0.15000f, 0.06000f, 0.31270f, 0.32900f }, // Display Gamut Rec709 
              { 0.64000f, 0.33000f, 0.30000f, 0.60000f, 0.15000f, 0.06000f, 0.31270f, 0.32900f }, // Display Gamut Rec709 

              { out_desc.RedPrimary   [0], out_desc.RedPrimary   [1],
                out_desc.GreenPrimary [0], out_desc.GreenPrimary [1],
                out_desc.BluePrimary  [0], out_desc.BluePrimary  [1],
                out_desc.WhitePoint   [0], out_desc.WhitePoint   [1] }
              };

            ImGui::TreePush ("");

            bool hdr_gamut_support = false;

            if (swap_desc.BufferDesc.Format == DXGI_FORMAT_R16G16B16A16_FLOAT)
            {
              hdr_gamut_support = true;
            }

            if ( swap_desc.BufferDesc.Format == DXGI_FORMAT_R10G10B10A2_UNORM )
            {
              hdr_gamut_support = true;
              ImGui::RadioButton ("Rec 709",  &cspace, 0); ImGui::SameLine (); 
            }
            else if (cspace == 0) cspace = 1;

            if ( swap_desc.BufferDesc.Format == DXGI_FORMAT_R10G10B10A2_UNORM )
            {
              hdr_gamut_support = true;
              ImGui::RadioButton ("Rec 2020", &cspace, 1); ImGui::SameLine ();
            }
            else if (cspace == 1) cspace = 0;
            ////ImGui::RadioButton ("Native",   &cspace, 2); ImGui::SameLine ();

            if (! (config.render.framerate.swapchain_wait              != 0 &&
                                           swap_desc.BufferDesc.Format == DXGI_FORMAT_R10G10B10A2_UNORM))// hdr_gamut_support)
            {
              HDR10MetaData.RedPrimary   [0] =
                static_cast <UINT16> (DisplayChromacityList [cspace].RedX * 50000.0f);
              HDR10MetaData.RedPrimary   [1] =
                static_cast <UINT16> (DisplayChromacityList [cspace].RedY * 50000.0f);

              HDR10MetaData.GreenPrimary [0] =
                static_cast <UINT16> (DisplayChromacityList [cspace].GreenX * 50000.0f);
              HDR10MetaData.GreenPrimary [1] =
                static_cast <UINT16> (DisplayChromacityList [cspace].GreenY * 50000.0f);

              HDR10MetaData.BluePrimary  [0] =
                static_cast <UINT16> (DisplayChromacityList [cspace].BlueX * 50000.0f);
              HDR10MetaData.BluePrimary  [1] =
                static_cast <UINT16> (DisplayChromacityList [cspace].BlueY * 50000.0f);

              HDR10MetaData.WhitePoint   [0] =
                static_cast <UINT16> (DisplayChromacityList [cspace].WhiteX * 50000.0f);
              HDR10MetaData.WhitePoint   [1] =
                static_cast <UINT16> (DisplayChromacityList [cspace].WhiteY * 50000.0f);

              static float fLuma [4] = { out_desc.MaxLuminance, out_desc.MinLuminance,
                2000.0f,               600.0f };

              if (hdr_gamut_support && swap_desc.BufferDesc.Format == DXGI_FORMAT_R10G10B10A2_UNORM)
                ImGui::InputFloat4 ("Luminance Coefficients", fLuma, 1);

              HDR10MetaData.MaxMasteringLuminance     = static_cast <UINT>   (fLuma [0] * 10000.0f);
              HDR10MetaData.MinMasteringLuminance     = static_cast <UINT>   (fLuma [1] * 10000.0f);
              HDR10MetaData.MaxContentLightLevel      = static_cast <UINT16> (fLuma [2]);
              HDR10MetaData.MaxFrameAverageLightLevel = static_cast <UINT16> (fLuma [3]);

              if (hdr_gamut_support && swap_desc.BufferDesc.Format == DXGI_FORMAT_R16G16B16A16_FLOAT)
              {
                ImGui::Separator ();

                float nits =
                  __SK_DQXI_HDR_Luma / 1.0_Nits;

                if (ImGui::SliderFloat ( "###DQXI_LUMINANCE", &nits, 80.0f, rb.display_gamut.maxLocalY,
                    "Peak White Luminance: %.1f Nits" ))
                {
                  __SK_DQXI_HDR_Luma = nits * 1.0_Nits;

                  auto& preset =
                    hdr_presets [__SK_DQXI_HDRPreset];

                  preset.peak_white_nits =
                    nits * 1.0_Nits;
                  preset.cfg_nits->store (preset.peak_white_nits);

                  pINI->write (pINI->get_filename ());
                }

                ImGui::SameLine ();

                if (ImGui::SliderFloat ("SDR -> HDR Gamma", &__SK_DQXI_HDR_Exp, 1.0f, 2.4f))
                {
                  auto& preset =
                    hdr_presets [__SK_DQXI_HDRPreset];

                  preset.eotf =
                    __SK_DQXI_HDR_Exp;
                  preset.cfg_eotf->store (preset.eotf);

                  pINI->write (pINI->get_filename ());
                }

                ImGui::BeginGroup ();
                for ( int i = 0 ; i < MAX_HDR_PRESETS ; i++ )
                {
                  bool selected =
                    (__SK_DQXI_HDRPreset == i);

                  if (ImGui::Selectable ( hdr_presets [i].preset_name, &selected, ImGuiSelectableFlags_SpanAllColumns ))
                  {
                    hdr_presets [i].activate ();
                  }
                }
                ImGui::EndGroup   ();
                ImGui::SameLine   (); ImGui::Spacing ();
                ImGui::SameLine   (); ImGui::Spacing ();
                ImGui::SameLine   (); ImGui::Spacing (); ImGui::SameLine (); 
                ImGui::BeginGroup ();
                for ( int i = 0 ; i < MAX_HDR_PRESETS ; i++ )
                {
                  ImGui::Text ( "Peak White: %5.1f Nits",
                                hdr_presets [i].peak_white_nits / 1.0_Nits );
                }
                ImGui::EndGroup   ();
                ImGui::SameLine   (); ImGui::Spacing ();
                ImGui::SameLine   (); ImGui::Spacing ();
                ImGui::SameLine   (); ImGui::Spacing (); ImGui::SameLine ();
                ImGui::BeginGroup ();
                for ( int i = 0 ; i < MAX_HDR_PRESETS ; i++ )
                {
                  ImGui::Text ( "Power-Law EOTF: %3.1f",
                                  hdr_presets [i].eotf );
                }
                ImGui::EndGroup   ();
                ImGui::SameLine   (); ImGui::Spacing ();
                ImGui::SameLine   (); ImGui::Spacing ();
                ImGui::SameLine   (); ImGui::Spacing ();
                ImGui::SameLine   (); ImGui::Spacing ();
                ImGui::SameLine   (); ImGui::Spacing ();
                ImGui::SameLine   (); ImGui::Spacing (); ImGui::SameLine (); 
                ImGui::BeginGroup ();
                for ( int i = 0 ; i < MAX_HDR_PRESETS ; i++ )
                {
                  Keybinding ( &hdr_presets [i].preset_activate,
                              (&hdr_presets [i].preset_activate)->param );
                }
                ImGui::EndGroup   ();

                //ImGui::SameLine ();
                //ImGui::Checkbox ("Explicit LinearRGB -> sRGB###IMGUI_SRGB", &rb.ui_srgb);
              }

              ///if (swap_desc.BufferDesc.Format == DXGI_FORMAT_R10G10B10A2_UNORM && ImGui::Button ("Inject HDR10 Metadata"))
              ///{
              ///  //if (cspace == 2)
              ///  //  swap_desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
              ///  //else if (cspace == 1)
              ///  //  swap_desc.BufferDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
              ///  //else
              ///  //  swap_desc.BufferDesc.Format = DXGI_FORMAT_R10G10B10A2_UNORM;
              ///
              ///  pSwap4->SetHDRMetaData (DXGI_HDR_METADATA_TYPE_NONE, 0, nullptr);
              ///
              ///  if (swap_desc.BufferDesc.Format == DXGI_FORMAT_R16G16B16A16_FLOAT)
              ///  {
              ///    pSwap4->SetColorSpace1 (DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709);
              ///  }
              ///
              ///  if      (cspace == 1) pSwap4->SetColorSpace1 (DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020);
              ///  else if (cspace == 0) pSwap4->SetColorSpace1 (DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709);
              ///  else                  pSwap4->SetColorSpace1 (DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709);
              ///
              ///  if (cspace == 1 || cspace == 0)
              ///    pSwap4->SetHDRMetaData (DXGI_HDR_METADATA_TYPE_HDR10, sizeof (HDR10MetaData), &HDR10MetaData);
              ///}
            }

            else
            {
              ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.075f, 1.0f, 1.0f));
              ImGui::BulletText     ("A waitable swapchain is required for HDR10 (D3D11 Settings/SwapChain | {Flip Model + Waitable}");
              ImGui::PopStyleColor  ();
            }

            ImGui::TreePop ();
          }
        }
      }

      ImGui::Separator (  );
      ImGui::TreePush  ("");

      ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.68f, 0.02f, 0.45f));
      ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.72f, 0.07f, 0.80f));
      ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.78f, 0.14f, 0.80f));
      ImGui::PopStyleColor (3);
      ImGui::TreePop       ( );
    }

    ImGui::TreePop ();

    return true;
  }

  return false;
}