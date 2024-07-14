//
// Copyright 2022-2024 Andon "Kaldaien" Coleman
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
#include <SpecialK/render/d3d12/d3d12_interfaces.h>
#include <SpecialK/render/d3d12/d3d12_screenshot.h>
#include <SpecialK/utility.h>
#include <imgui/font_awesome.h>
#include <algorithm>

struct address_cache_s {
  std::unordered_map <std::string, uintptr_t> cached;
};

template <auto _N>
  using code_bytes_t =
     boost::container::static_vector <uint8_t, _N>;

struct code_patch_s {
  void* pAddr = nullptr;

  struct executable_code_s {
    code_bytes_t <8> inst_bytes = { };
  } original, replacement;
  
  void apply (executable_code_s *pExec);
};

struct {
  float fSpeed            = 1.0f;
  bool  bFixPrioInversion = true;
  bool  bUncapFramerate   = true;

  struct {
    sk::ParameterFloat* game_speed         = nullptr;
    sk::ParameterBool*  fix_prio_inversion = nullptr;
    sk::ParameterBool*  uncap_framerate    = nullptr;
  } ini;

  std::unordered_map <std::wstring, address_cache_s> addresses;

  code_patch_s clock_tick0, clock_tick1, clock_tick2,
               clock_tick3, clock_tick4, clock_tick5;
} SK_ER_PlugIn;

static auto
SK_VirtualProtect =
[]( LPVOID lpAddress,
    SIZE_T dwSize,
    DWORD  flNewProtect )
{
  DWORD dwRet = 0x0;

  VirtualProtect (
    lpAddress,     dwSize,
    flNewProtect, &dwRet);

  return dwRet;
};

void
code_patch_s::apply (code_patch_s::executable_code_s *pExec)
{
  if (pAddr == nullptr)
    return;

  DWORD dwOldProtect =
    SK_VirtualProtect (
          pAddr, pExec->inst_bytes.size (), PAGE_EXECUTE_READWRITE);
  memcpy (pAddr, pExec->inst_bytes.data (),
                 pExec->inst_bytes.size ());
    SK_VirtualProtect (
          pAddr, pExec->inst_bytes.size (), dwOldProtect);
}

void
__stdcall
SK_ER_EndFrame (void)
{
  if (SK_ER_PlugIn.bFixPrioInversion)
  {
    config.render.framerate.sleepless_window = false;

    if (GetThreadPriority (GetCurrentThread ()) == THREAD_PRIORITY_BELOW_NORMAL)
        SetThreadPriority (GetCurrentThread (),    THREAD_PRIORITY_LOWEST);
  }

  static auto& addresses = 
    SK_ER_PlugIn.addresses [
       SK_GetDLLVersionStr (SK_GetHostApp ())
                           ].cached;
  
  static float* fAddr = addresses.contains ("dt_float") ?
      (float *)((uintptr_t)SK_Debug_GetImageBaseAddr () + addresses ["dt_float"])
                                                        : nullptr;
  
  SK_RunOnce (fAddr = SK_ValidatePointer (fAddr, true) ? fAddr : nullptr);
  
  if (fAddr != nullptr)
  {
    *fAddr =
      SK_ER_PlugIn.fSpeed * static_cast <float> (
        std::min (  2.00000,
         std::max ( 0.00005,
           static_cast <double> (SK_GetCurrentRenderBackend ().frame_delta.getDeltaTime ()) /
           static_cast <double> (SK_PerfFreq) ) ) );
  }
}

bool
SK_ER_PlugInCfg (void)
{
  static
    std::string utf8VersionString =
        SK_WideCharToUTF8 (
          SK_GetDLLVersionStr (
             SK_GetHostApp () ) ) +
              "###EldenRingHeader\0";

  static auto& addresses =
    SK_ER_PlugIn.addresses [
       SK_GetDLLVersionStr (SK_GetHostApp ())
                           ].cached;

  if ( ImGui::CollapsingHeader ( utf8VersionString.data (),
                                   ImGuiTreeNodeFlags_DefaultOpen )
     )
  {
    ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.40f, 0.40f, 0.45f));
    ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.45f, 0.45f, 0.80f));
    ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.53f, 0.53f, 0.80f));
    ImGui::TreePush       ("");

    if ( ImGui::CollapsingHeader (
           ICON_FA_TACHOMETER_ALT "\tPerformance Settings",
                          ImGuiTreeNodeFlags_DefaultOpen )
       )
    {
      ImGui::TreePush    ("");
      ImGui::BeginGroup  (  );
  if (ImGui::Checkbox    ("Fix Render Thread Priority Inversion", &SK_ER_PlugIn.bFixPrioInversion))
      {
        if ((! SK_ER_PlugIn.bFixPrioInversion) && GetThreadPriority (GetCurrentThread ()) == THREAD_PRIORITY_LOWEST)
          SetThreadPriority (GetCurrentThread (), THREAD_PRIORITY_BELOW_NORMAL); // Game default
        else if (                                 GetThreadPriority (GetCurrentThread ()) == THREAD_PRIORITY_BELOW_NORMAL)
          SetThreadPriority (GetCurrentThread (), THREAD_PRIORITY_LOWEST);       // Sensible default

        SK_ER_PlugIn.ini.fix_prio_inversion->store (SK_ER_PlugIn.bFixPrioInversion);
      }
  if (addresses.contains ("dt_float"))
  {
      ImGui::SameLine    (  );
  if (ImGui::SliderFloat ("Game Speed",                           &SK_ER_PlugIn.fSpeed, 0.75f, 1.5f, "%.3fx"))
      {
        SK_ER_PlugIn.ini.game_speed->store (SK_ER_PlugIn.fSpeed);
      }
  }
      ImGui::EndGroup    (  );

      int                             sel = SK_ER_PlugIn.bUncapFramerate ? 0 : 1;
  if (ImGui::Combo ("Framerate Cap", &sel, "Unlocked\0Normal\0\0"))
      {
        SK_ER_PlugIn.bUncapFramerate = (sel == 0);

        for ( auto patch : { &SK_ER_PlugIn.clock_tick0, &SK_ER_PlugIn.clock_tick1,
                             &SK_ER_PlugIn.clock_tick2, &SK_ER_PlugIn.clock_tick3,
                             &SK_ER_PlugIn.clock_tick4, &SK_ER_PlugIn.clock_tick5 } )
        {
          if (SK_ER_PlugIn.bUncapFramerate) patch->apply (&patch->replacement);
          else                              patch->apply (&patch->original);
        }

        SK_ER_PlugIn.ini.uncap_framerate->store (SK_ER_PlugIn.bUncapFramerate);
      }

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip    ();
        ImGui::TextUnformatted ("For Best Unlocked Results");
        ImGui::Separator       ();
        ImGui::BulletText      ("Use Borderless Window Mode");
        ImGui::BulletText      ("Configure Refresh Rate using SK's Display Menu");
        ImGui::BulletText      ("Right-Click Framerate Limit to use Latent Sync (VSYNC Off Mode)");
        ImGui::EndTooltip      ();
      }

      ImGui::TreePop     (  );
    }

#if 0
    if (ImGui::CollapsingHeader (ICON_FA_IMAGE "\tGraphics Settings", ImGuiTreeNodeFlags_DefaultOpen))
    {
      static bool show_hud = true;

      ImGui::TreePush   ("");
      ImGui::BeginGroup (  );
      
      if (ImGui::Checkbox ("Show HUD", &show_hud))
      {
        if (show_hud) SK_D3D12_ShowGameHUD ();
        else          SK_D3D12_HideGameHUD ();
      }

      ImGui::SameLine ();

      const auto Keybinding =
      [] (SK_ConfigSerializedKeybind *binding, sk::ParameterStringW* param)
   -> auto
      {
        if (! (binding != nullptr && param != nullptr))
          return false;

        std::string label =
          SK_WideCharToUTF8 (binding->human_readable);

        ImGui::PushID (binding->bind_name);

        if (SK_ImGui_KeybindSelect (binding, label.c_str ()))
          ImGui::OpenPopup (        binding->bind_name);

        std::wstring original_binding =
                                binding->human_readable;

        SK_ImGui_KeybindDialog (binding);

        ImGui::PopID ();

        if (original_binding != binding->human_readable)
        {
          param->store (binding->human_readable);

          SK_SaveConfig ();

          return true;
        }

        return false;
      };

      ImGui::BeginGroup ();
        ImGui::Text     ("HUD Toggle:  "         );
        ImGui::Text     ("HUD Free Screenshot:  ");
      ImGui::EndGroup   ();
      ImGui::SameLine   ();
      ImGui::BeginGroup ();
        Keybinding      (&config.render.keys.hud_toggle,
                          config.render.keys.hud_toggle.param);
        Keybinding      (&config.screenshots.game_hud_free_keybind,
                          config.screenshots.game_hud_free_keybind.param);
      ImGui::EndGroup   ();
      ImGui::Separator  ();

      auto currentFrame =
            SK_GetFramesDrawn ();

#pragma region "Advanced"
      if ( //config.system.log_level > 0
           ImGui::TreeNode ("Recently Used Shaders")
         )
      {
        static auto constexpr _RECENT_USE_THRESHOLD = 30;
        static auto constexpr _ACTIVE_THRESHOLD     = 300;

        static constexpr GUID SKID_D3D12DisablePipelineState =
          { 0x3d5298cb, 0xd9f0,  0x6133, { 0xa1, 0x9d, 0xb1, 0xd5, 0x97, 0x92, 0x00, 0x70 } };

        static constexpr GUID SKID_D3D12KnownVtxShaderDigest =
          { 0x4d5298ca, 0xd9f0,  0x6133, { 0xa1, 0x9d, 0xb1, 0xd5, 0x97, 0x92, 0x00, 0x00 } };

        static constexpr GUID SKID_D3D12KnownPixShaderDigest =
          { 0x4d5298ca, 0xd9f0,  0x6133, { 0xa1, 0x9d, 0xb1, 0xd5, 0x97, 0x92, 0x00, 0x01 } };

        static auto constexpr   DxilContainerHashSize  = 16;
        std::multimap <uint32_t, ID3D12PipelineState *> shaders;

        extern
          concurrency::concurrent_unordered_map <ID3D12PipelineState*, bool>
                                 _pixelShaders;
        for ( auto &[ps, live] : _pixelShaders )
        {
          if (! live) continue;

          UINT   size      = 8UL;
          UINT64 lastFrame = 0ULL;

          ps->GetPrivateData (SKID_D3D12LastFrameUsed, &size, &lastFrame);

        //if (currentFrame > lastFrame + _RECENT_USE_THRESHOLD) continue;

                  size =  DxilContainerHashSize;
          uint8_t digest [DxilContainerHashSize];

          ps->GetPrivateData ( SKID_D3D12KnownPixShaderDigest, &size, digest );

          shaders.emplace (
            crc32c (0x0, digest, 16), ps
          );
        }

        std::string name (" ", MAX_PATH + 1);

        ImGui::BeginGroup ();
        extern
          concurrency::concurrent_unordered_map <ID3D12PipelineState*, bool>
                                 _pixelShaders;

        for ( const auto &[bucket, dump] : shaders )
        {
          bool   disable = false;
          UINT   size    = sizeof (bool);

          if ( FAILED ( dump->GetPrivateData (
                          SKID_D3D12DisablePipelineState, &size, &disable
             )        )                    ) {             size=1;disable=false; }

          bool enable =
            (! disable);

          ImGui::PushID ((int)(intptr_t)dump);

          auto range =
            shaders.equal_range (bucket);

          if (ImGui::Checkbox (" Pixel::", &enable))
          {
            disable =
              (! enable);

            for ( auto it = range.first ; it != range.second ; ++it )
            {
              it->second->SetPrivateData (
                SKID_D3D12DisablePipelineState, size, &disable
              );
            }
          }
          
                  size =  DxilContainerHashSize;
          uint8_t digest [DxilContainerHashSize];

            dump->GetPrivateData ( SKID_D3D12KnownPixShaderDigest, &size, digest );

            std::string out =
              std::format ("{:x}{:x}{:x}{:x}{:x}{:x}{:x}{:x}"
                           "{:x}{:x}{:x}{:x}{:x}{:x}{:x}{:x}",
                             digest[ 0],digest[ 1],digest[ 2],digest[ 3],
                             digest[ 4],digest[ 5],digest[ 6],digest[ 7],
                             digest[ 8],digest[ 9],digest[10],digest[11],
                             digest[12],digest[13],digest[14],digest[15]);

          if (ImGui::IsItemClicked (1))
          {
            SK_LOG0 ( ( L"%hs", out.c_str () ), L"   DXGI   " );
          }

          ImGui::SameLine ();
          ImGui::Text     ("  %hs", out.c_str ());
          ImGui::PopID    ();
        }
        ImGui::EndGroup   ();
        //ImGui::SameLine   ();
        //ImGui::BeginGroup ();
        //
        //for ( auto &[ps, live] : _vertexShaders )
        //{
        //  if (! live) continue;
        //
        //  UINT   size      = 8UL;
        //  UINT64 lastFrame = 0ULL;
        //
        //  ps->GetPrivateData (SKID_D3D12LastFrameUsed, &size, &lastFrame);
        //
        //  if (currentFrame > lastFrame + _RECENT_USE_THRESHOLD) continue;
        //
        //  *name.data () = '\0';
        //
        //  if ( UINT                                               uiStrLen = MAX_PATH ;
        //FAILED ( ps->GetPrivateData ( WKPDID_D3DDebugObjectName, &uiStrLen, name.data () )
        //     ) ) { *name.data () = '\0'; }
        //  
        //  ImGui::PushID ((int)(intptr_t)ps);
        //
        //  if (ImGui::InputText (SK_FormatString ("  %08x", ps).c_str (),
        //                                                  name.data  (), MAX_PATH))
        //    SK_D3D12_SetDebugName (ps, SK_UTF8ToWideChar (name));
        //
        //  ImGui::PopID ();
        //}
        //ImGui::EndGroup ();

        ImGui::Separator ();

        ImGui::BeginGroup ();
        extern
          concurrency::concurrent_unordered_map <ID3D12PipelineState*, std::string>
                                _latePSOBlobs;
        for ( auto &[ps, str] : _latePSOBlobs )
        {
          bool   disable   = false;
          UINT   size      = sizeof (UINT64);
          UINT64 lastFrame = 0;

          ps->GetPrivateData (SKID_D3D12LastFrameUsed, &size, &lastFrame);

          if (currentFrame > lastFrame + _ACTIVE_THRESHOLD) continue;

          if ( FAILED ( ps->GetPrivateData (
                          SKID_D3D12DisablePipelineState, &size, &disable
             )        )                    ) {             size=1;disable=false; }

          bool enable =
            (! disable);

          ImGui::PushID ((int)(intptr_t)ps);

          if (ImGui::Checkbox (" Other::", &enable))
          {
            disable =
              (! enable);

            ps->SetPrivateData (SKID_D3D12DisablePipelineState, size, &disable);
          }

          ImGui::PopID  ();
        }
        ImGui::EndGroup   ();
        ImGui::SameLine   ();
        ImGui::BeginGroup ();

        for ( auto &[ps, str] : _latePSOBlobs )
        {
          UINT   size      = sizeof (UINT64);
          UINT64 lastFrame = 0;

          ps->GetPrivateData (SKID_D3D12LastFrameUsed, &size, &lastFrame);

          if (currentFrame > lastFrame + _ACTIVE_THRESHOLD) continue;

          *name.data () = '\0';

          ImGui::PushStyleColor ( ImGuiCol_Text,
            currentFrame > lastFrame - _RECENT_USE_THRESHOLD   ?
                                  ImVec4 (0.5f,0.5f,0.5f,1.0f) :
                                  ImVec4 (1.0f,1.0f,1.0f,1.0f) );

          if ( UINT                                               uiStrLen = MAX_PATH ;
        FAILED ( ps->GetPrivateData ( WKPDID_D3DDebugObjectName, &uiStrLen, name.data () )
             ) ) { *name.data () = '\0'; }

          ImGui::PopStyleColor ();
          
          ImGui::PushID ((int)(intptr_t)ps);

          if (ImGui::InputText (str.c_str (),             name.data (), MAX_PATH))
            SK_D3D12_SetDebugName (ps, SK_UTF8ToWideChar (name));

          ImGui::PopID  ();
        }
        ImGui::EndGroup ();
        ImGui::TreePop  ();
      }
      ImGui::EndGroup   ();
#pragma endregion

      static std::error_code                    ec = { };
      static std::filesystem::path pathPlayStation =
        SK_Resource_GetRoot () / LR"(inject/textures)"
          /  L"d3d12_sk0_crc32c_b42f89d4.dds", // Current hash being used as of 2023-09-01
                                   pathPlayStation_Old2 =
        SK_Resource_GetRoot () / LR"(inject/textures)"
          /  L"d3d12_sk0_crc32c_ae7c1bb2.dds",
                                   pathPlayStation_Old =
        SK_Resource_GetRoot () / LR"(inject/textures)"
          /  L"d3d12_sk0_crc32c_0041d76d.dds"; // Hash changed in 1.3.0, then changed back in 1.3.2

      // Remove the old texture mod, since there's extra overhead until they all load
      static bool had_old =
        (std::filesystem::exists (pathPlayStation_Old2, ec) ?
         std::filesystem::remove (pathPlayStation_Old2, ec) : false) &&
        (std::filesystem::exists (pathPlayStation_Old,  ec) ?
         std::filesystem::remove (pathPlayStation_Old,  ec) : false);

      static bool                   bPlayStation_AtStart =
        std::filesystem::exists (pathPlayStation, ec),
                                    bPlayStation =
                                    bPlayStation_AtStart,
                                    bFetching   = false;

#if 0
      if (ImGui::Checkbox ("Enable " ICON_FA_PLAYSTATION " Buttons", &bPlayStation))
      {
        if (! bFetching)
        {
          if (  std::filesystem::exists (pathPlayStation, ec))
          { if (std::filesystem::remove (pathPlayStation, ec))
                                            bPlayStation = false;
          }

          else
          {
            bFetching = true;

            SK_Network_EnqueueDownload (
                 sk_download_request_s (
                   pathPlayStation.wstring (),
                     R"(https://sk-data.special-k.info/addon/EldenRing/)"
                               R"(buttons/d3d12_sk0_crc32c_b42f89d4.dds)",
            []( const std::vector <uint8_t>&&,
                const std::wstring_view )
             -> bool
                {
                  bFetching    = false;
                  bPlayStation = true;
            
                  return false;
                }
              ), true
            );
          }
        }
      }

      if ( bFetching || bPlayStation_AtStart !=
                        bPlayStation )
      {
        ImGui::SameLine ();

        if (bFetching) ImGui::TextColored (ImVec4 (.1f,.9f,.1f,1.f), "Downloading...");
        else
        {              ImGui::Bullet      ();
                       ImGui::SameLine    ();
                       ImGui::TextColored (ImVec4 (1.f,1.f,0.f,1.f), "Game Restart Required");
        }
      }
#endif

      ImGui::TreePop     ( );
    }
#endif

    ImGui::PopStyleColor (3);
    ImGui::TreePop       ( );
  }

  return true;
}

void
SK_ER_InitConfig (void)
{
  SK_ER_PlugIn.ini.game_speed =
    _CreateConfigParameterFloat ( L"EldenRing.PlugIn",
                                  L"GameSpeed", SK_ER_PlugIn.fSpeed,
                                                       L"Game Speed" );

  if (! SK_ER_PlugIn.ini.game_speed->load  (SK_ER_PlugIn.fSpeed) || SK_ER_PlugIn.fSpeed < 0.1f)
        SK_ER_PlugIn.ini.game_speed->store (1.0f);

  SK_ER_PlugIn.ini.fix_prio_inversion =
    _CreateConfigParameterBool ( L"EldenRing.PlugIn",
                                 L"FixPrioInversion", SK_ER_PlugIn.bFixPrioInversion,
                                                                     L"Priority Fix" );

  if (! SK_ER_PlugIn.ini.fix_prio_inversion->load  (SK_ER_PlugIn.bFixPrioInversion))
        SK_ER_PlugIn.ini.fix_prio_inversion->store (true);

  SK_ER_PlugIn.ini.uncap_framerate =
    _CreateConfigParameterBool ( L"EldenRing.PlugIn",
                                 L"UncapFramerate", SK_ER_PlugIn.bUncapFramerate,
                                                          L"Remove 60 FPS Limit" );

  if (! SK_ER_PlugIn.ini.uncap_framerate->load  (SK_ER_PlugIn.bUncapFramerate))
        SK_ER_PlugIn.ini.uncap_framerate->store (true);

  auto& addresses =
    SK_ER_PlugIn.addresses;

  addresses [L"ELDEN RING™  1.2.1.0"].
   cached =
        { { "clock_tick0", 0x0DFEF87 }, { "clock_tick1", 0x0DFEFA3 },
          { "clock_tick2", 0x0DFEFAF }, { "clock_tick3", 0x0DFEFC0 },
          { "clock_tick4", 0x0DFEFCD }, { "clock_tick5", 0x0DFEFDD },
          { "write_delta", 0x25A7F72 }, { "dt_float",    0x3B4FE28 } };
  addresses [L"ELDEN RING™  1.2.2.0"].
   cached =
        { { "clock_tick0", 0x0DFF397 }, { "clock_tick1", 0x0DFF3B3 },
          { "clock_tick2", 0x0DFF3BF }, { "clock_tick3", 0x0DFF3D0 },
          { "clock_tick4", 0x0DFF3DD }, { "clock_tick5", 0x0DFF3ED },
          { "write_delta", 0x25A8412 }, { "dt_float",    0x3B4FE38 } };
  addresses [L"ELDEN RING™  1.2.3.0"].
   cached =
        { { "clock_tick0", 0x0DFFE77 }, { "clock_tick1", 0x0DFFE93 },
          { "clock_tick2", 0x0DFFECD }, { "clock_tick3", 0x0DFFE9F },
          { "clock_tick4", 0x0DFFEB0 }, { "clock_tick5", 0x0DFFEBD },
          { "write_delta", 0x25A9752 }, { "dt_float",    0x3B52E78 } };
  addresses [L"ELDEN RING™  1.3.1.0"].
   cached =
        { { "clock_tick0",  0x0E07F47 }, { "clock_tick1",  0x0E07F63 },
          { "clock_tick2",  0x0E07F9D }, { "clock_tick3",  0x0E07F6F },
          { "clock_tick4",  0x0E07F80 }, { "clock_tick5",  0x0E07F8D },
          { "write_delta",  0x25B2D62 }, { "dt_float",     0x3B63FE8 },
          { "write_delta0", 0x0D75074 }, { "write_delta1", 0x0D75086 } };
  addresses [L"ELDEN RING™  1.3.2.0"].
   cached =
        { { "clock_tick0",  0x0E07F27 }, { "clock_tick1",  0x0E07F43 },
          { "clock_tick2",  0x0E07F7D }, { "clock_tick3",  0x0E07F4F },
          { "clock_tick4",  0x0E07F60 }, { "clock_tick5",  0x0E07F6D },
          { "write_delta",  0x25B2D42 }, { "dt_float",     0x3B63FE8 } };
  addresses [L"ELDEN RING™  2.2.0.0"].
   cached =
        { { "clock_tick0",  0x0E82692 }, { "clock_tick1",  0x0E826B1 },
          { "clock_tick2",  0x0E826E0 }, { "clock_tick3",  0x0E821AD },
          { "clock_tick4",  0x0E826BD }, { "clock_tick5",  0x0F8CD18 } };
  addresses [L"ELDEN RING™  2.2.3.0"].
   cached =
       { { "clock_tick0",  0x0E826B2 }, { "clock_tick1",  0x0E826D1 },
         { "clock_tick2",  0x0       }, { "clock_tick3",  0x0E821CD },
         { "clock_tick4",  0x0E826DD }, { "clock_tick5",  0x0F8CD38 } };

  uint8_t stack_size = 0x1C; // 2.2.0.0+

  std::wstring game_ver_str =
    SK_GetDLLVersionStr (SK_GetHostApp ());

  if (StrStrIW (game_ver_str.c_str (), L"ELDEN RING™  1.") ||
      StrStrIW (game_ver_str.c_str (), L"ELDEN RING™  2.0."))
  {
    stack_size = 0x20; // 2.0 and older.
  }

  if (! addresses.contains (game_ver_str))
  {
    void* pBaseAddr =
      SK_Debug_GetImageBaseAddr ();

    const uint8_t pattern0 [] = { 0xC7, 0x43, 0x18, 0x02, 0x00,
                                  0x00, 0x00, 0xc7, 0x43, stack_size, 0x89, 0x88, 0x08 };
    const uint8_t pattern1 [] = { 0x18, 0xC7, 0x43, 0x00, 0x89 };
    const uint8_t mask1    [] = { 0xFF, 0xFF, 0xFF, 0x00, 0xFF };
    const uint8_t pattern2 [] = { 0x7B, 0x18, 0xC7, 0x43, 0x20 };

    auto
      p0 = (void *)( 7 + static_cast <uint8_t *>
       ( SK_ScanAlignedEx ( pattern0, 13, nullptr, nullptr ) ) );

    void* p1 = nullptr;
    void* p2 = nullptr;

    if (p0 != nullptr)
    {
      p1 = (void *)( 7 + static_cast <uint8_t *>
       ( SK_ScanAlignedEx ( pattern0, 13, nullptr, p0 ) ) );
      p2 = (void *)( 7 + static_cast <uint8_t *>
       ( SK_ScanAlignedEx ( pattern0, 13, nullptr, p1 ) ) );
    }

    auto
      p3 = (void *)( 1 + static_cast <uint8_t *>
       ( SK_ScanAlignedEx ( pattern1, 5, mask1, nullptr ) ) ),
      p4 = (void *)( 1 + static_cast <uint8_t *>
       ( SK_ScanAlignedEx ( pattern1, 5, mask1, p3 ) ) );

    auto
      p5 = (void *)( 2 + static_cast <uint8_t *>
       ( SK_ScanAlignedEx ( pattern2, 5, nullptr, p0 ) ) );

    addresses [game_ver_str].
     cached =
          { { "clock_tick0", (uintptr_t)p0 - (uintptr_t)pBaseAddr },
            { "clock_tick1", (uintptr_t)p1 - (uintptr_t)pBaseAddr },
            { "clock_tick2", (uintptr_t)p2 - (uintptr_t)pBaseAddr },
            { "clock_tick3", (uintptr_t)p3 - (uintptr_t)pBaseAddr },
            { "clock_tick4", (uintptr_t)p4 - (uintptr_t)pBaseAddr },
            { "clock_tick5", (uintptr_t)p5 - (uintptr_t)pBaseAddr } };

    for ( auto &[name, address] :
                             addresses [game_ver_str].cached )
    {
      if (address > 2 * (uintptr_t)p0)
      {
        address = 0;

        SK_LOG0 (
          ( L"Skipping Incorrect Framerate Limit Memory Address: %hs",
              name.c_str () ), L"Elden Ring" );
      }

      else
      {
        SK_LOG0 ( ( L"Uncached Address (%hs) => %ph", name.c_str (), address ),
                    L"Elden Ring" );
      }
    }
  }

  auto& addr_cache =
    SK_ER_PlugIn.addresses [game_ver_str].cached;

  for ( auto &[record, name] :
          std::initializer_list <
            std::pair <code_patch_s&, std::string> >
              { { SK_ER_PlugIn.clock_tick0, "clock_tick0" },
                { SK_ER_PlugIn.clock_tick1, "clock_tick1" },
                { SK_ER_PlugIn.clock_tick2, "clock_tick2" },
                { SK_ER_PlugIn.clock_tick3, "clock_tick3" },
                { SK_ER_PlugIn.clock_tick4, "clock_tick4" },
                { SK_ER_PlugIn.clock_tick5, "clock_tick5" } } )
  {
    record = {
      .pAddr       = (void *)addr_cache [name],
      .original    = code_bytes_t <8> {0   ,0   ,0,         0,0,0,0},
      .replacement = code_bytes_t <8> {0xC7,0x43,stack_size,0,0,0,0} };
  }
}

void __stdcall
SK_ER_DeferredInit (void)
{
  if (static bool                                 init         = false;
      SK_GetFramesDrawn () > 15 && std::exchange (init, true) == false)
  {
    std::wstring game_ver_str =
      SK_GetDLLVersionStr (SK_GetHostApp ());

    SK_ER_InitConfig ();

    auto& addr_cache =
      SK_ER_PlugIn.addresses [SK_GetDLLVersionStr (SK_GetHostApp ())].cached;

    if (             const auto* pClockTick0 = (uint8_t *)
         SK_Debug_GetImageBaseAddr () +
                              addr_cache ["clock_tick0"] ;
         SK_ValidatePointer     (pClockTick0) &&
         SK_IsAddressExecutable (pClockTick0) &&
                                *pClockTick0  == 0xC7 )
    {
      auto *cp =
        SK_Render_InitializeSharedCVars ();

      if (! cp)
      {
        SK_ImGui_Warning (L"Special K Command Processor is Busted...");

        return;
      }

      cp->AddVariable (
        "EldenRing.fClockMultiplier", SK_CreateVar ( SK_IVariable::Float,
                                                    &SK_ER_PlugIn.fSpeed )
                                         );

      for ( auto &[patch, name] :
          std::initializer_list <
            std::pair <code_patch_s&, std::string> >
              { { SK_ER_PlugIn.clock_tick0, "clock_tick0" },
                { SK_ER_PlugIn.clock_tick1, "clock_tick1" },
                { SK_ER_PlugIn.clock_tick2, "clock_tick2" },
                { SK_ER_PlugIn.clock_tick3, "clock_tick3" },
                { SK_ER_PlugIn.clock_tick4, "clock_tick4" },
                { SK_ER_PlugIn.clock_tick5, "clock_tick5" } } )
      {
        patch.pAddr = (void *)addr_cache [name];

        if ((intptr_t)patch.pAddr > 7)
        {
          (uintptr_t&)patch.pAddr +=
            (uintptr_t)SK_Debug_GetImageBaseAddr ();

          memcpy (
            patch.original.inst_bytes.data (),
            patch.pAddr, 7
          );

          patch.apply ( SK_ER_PlugIn.bUncapFramerate ?
                                  &patch.replacement : &patch.original );
        }
      }


      if (addr_cache.contains ("write_delta"))
      {
        DWORD dwOldProt = 0x0;
        uint8_t* pNOP   = (uint8_t *)SK_Debug_GetImageBaseAddr () + addr_cache ["write_delta"];

        // Disable the code that writes delta time every frame
        VirtualProtect (pNOP,  8, PAGE_EXECUTE_READWRITE, &dwOldProt);
        memcpy (        pNOP, "\x90\x90\x90\x90\x90\x90\x90\x90",  8);
        VirtualProtect (pNOP,  8,              dwOldProt, &dwOldProt);
      }


      // Added in 1.3.1.0
      if (addr_cache.contains ("write_delta0"))
      {
        DWORD dwOldProt = 0x0;
        uint8_t* pNOP   = (uint8_t *)SK_Debug_GetImageBaseAddr () + addr_cache ["write_delta0"];
      
        // Disable the code that writes delta time every frame
        VirtualProtect (pNOP,  6, PAGE_EXECUTE_READWRITE, &dwOldProt);
        memcpy (        pNOP, "\x90\x90\x90\x90\x90\x90",          6);
        VirtualProtect (pNOP,  6,              dwOldProt, &dwOldProt);

                 pNOP   = (uint8_t *)SK_Debug_GetImageBaseAddr () + addr_cache ["write_delta1"];
        
        VirtualProtect (pNOP,  6, PAGE_EXECUTE_READWRITE, &dwOldProt);
        memcpy (        pNOP, "\x90\x90\x90\x90\x90\x90",          6);
        VirtualProtect (pNOP,  6,              dwOldProt, &dwOldProt);
      }


      plugin_mgr->end_frame_fns.emplace (SK_ER_EndFrame );
      plugin_mgr->config_fns.emplace    (SK_ER_PlugInCfg);

      return;
    }

    SK_ImGui_Warning (
      L"This version of Elden Ring is not Compatible with Special K"
    );
  }
}

void
SK_ER_InitPlugin (void)
{
  plugin_mgr->end_frame_fns.emplace (SK_ER_DeferredInit);
}




// 32-bit Launcher Bypass Code
void
SK_SEH_LaunchEldenRing (void)
{
  __try {
    STARTUPINFOW        sinfo = { };
    PROCESS_INFORMATION pinfo = { };

    sinfo.cb          = sizeof (STARTUPINFOW);
    sinfo.wShowWindow = SW_SHOWNORMAL;
    sinfo.dwFlags     = STARTF_USESHOWWINDOW | STARTF_FORCEOFFFEEDBACK;

    // EAC Launcher has SteamNoOverlayUIDrawing set to 1, we don't want
    //   to inherit that (!!)
    SetEnvironmentVariable (L"SteamNoOverlayUIDrawing", L"0");

    CreateProcess ( L"eldenring.exe", nullptr, nullptr, nullptr,
                    TRUE,    CREATE_SUSPENDED, nullptr, SK_GetHostPath (),
                    &sinfo,  &pinfo );

    if (pinfo.hProcess != 0)
    {
      // Save config prior to comitting suicide
      SK_SelfDestruct ();

      ResumeThread     (pinfo.hThread);
      SK_CloseHandle   (pinfo.hThread);
      SK_CloseHandle   (pinfo.hProcess);

      SK_TerminateProcess (0x00);
    }
  }

  __except (EXCEPTION_EXECUTE_HANDLER) {
    // Swallow _all_ exceptions, EAC deserves a swift death
  }
}