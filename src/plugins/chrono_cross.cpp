//
// Copyright 2022 Andon "Kaldaien" Coleman
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
#include <SpecialK/utility.h>
#include <imgui/font_awesome.h>
#include <algorithm>

struct address_cache_s {
  std::unordered_map <std::string, uintptr_t> cached;
};

template <auto _N>
  using code_bytes_t =
     boost::container::static_vector <uint8_t, _N>;

struct cc_code_patch_s {
  void* pAddr;

  struct executable_code_s {
    code_bytes_t <4> inst_bytes;
  } original, replacement;
  
  void apply (executable_code_s *pExec);
};

struct {
  bool    bUnlockFramerate              = true;
  int     iTimeFlag0                    = 0x00;
  int     iTimeFlag1                    = 0x07;

  struct {
    sk::ParameterBool* unlock_framerate = nullptr;
    sk::ParameterInt*  time_flag0       = nullptr;
    sk::ParameterInt*  time_flag1       = nullptr;
  } ini;

  std::unordered_map <
    std::wstring, address_cache_s
  > addresses;

  cc_code_patch_s clock_tick0, clock_tick1;
} SK_CC_PlugIn;

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
cc_code_patch_s::apply (cc_code_patch_s::executable_code_s *pExec)
{
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
SK_CC_EndFrame (void)
{
}

bool
SK_CC_PlugInCfg (void)
{
  static
    std::string utf8VersionString =
        SK_WideCharToUTF8 (
          SK_GetDLLVersionStr (
             SK_GetHostApp () ) ) +
              "###ChronoCrossHeader\0";

  static auto& addresses =
    SK_CC_PlugIn.addresses [
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
      auto _RewriteClockCode = [&](void)
   -> void
      {
        for ( auto patch : { &SK_CC_PlugIn.clock_tick0,
                             &SK_CC_PlugIn.clock_tick1 } )
        {
          if (SK_CC_PlugIn.bUnlockFramerate)
          {
            auto pFlagAddr =
               ((uint8_t *)patch->pAddr + 3);
            patch->apply (&patch->replacement);

            DWORD                                                 dwOldProt;
            VirtualProtect (pFlagAddr, 1, PAGE_EXECUTE_READWRITE,&dwOldProt);
            if (patch == &SK_CC_PlugIn.clock_tick0)
              *pFlagAddr = (uint8_t)SK_CC_PlugIn.iTimeFlag0;
            else
              *pFlagAddr = (uint8_t)SK_CC_PlugIn.iTimeFlag1;
            VirtualProtect (pFlagAddr, 1, dwOldProt,             &dwOldProt);
          }

          else
            patch->apply (&patch->original);
        }
      };
      ImGui::TreePush    ("");
      int                              sel = SK_CC_PlugIn.bUnlockFramerate ? 0 : 1;
      if (ImGui::Combo ("Frame Pacing", &sel, "Unlocked\0Normal\0\0"))
      {
        SK_CC_PlugIn.bUnlockFramerate = (sel == 0);

        _RewriteClockCode ();

        SK_CC_PlugIn.ini.unlock_framerate->store (SK_CC_PlugIn.bUnlockFramerate);
      }

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip    (  );
        ImGui::TextUnformatted ("Big Thanks to Isa for the initial implementation");
        ImGui::Separator       (  );
        ImGui::TextUnformatted ("");
        ImGui::TextUnformatted ("For Best Results when using Unlocked Framerate");
        ImGui::TextUnformatted ("");
        ImGui::Separator       (  );
        ImGui::BulletText      ("Use Borderless Window Mode");
        ImGui::BulletText      ("Configure Refresh Rate using SK's Display Menu");
        ImGui::BulletText      ("Right-Click Framerate Limit to use Latent Sync (VSYNC Off Mode)");
        ImGui::EndTooltip      (  );
      }

      if (SK_CC_PlugIn.bUnlockFramerate)
      {     ImGui::SameLine ();
        if (ImGui::InputInt ("t0", &SK_CC_PlugIn.iTimeFlag0))
        {
                      SK_CC_PlugIn.iTimeFlag0 =
          std::clamp (SK_CC_PlugIn.iTimeFlag0, 0, 127);

          SK_CC_PlugIn.ini.time_flag0->store (SK_CC_PlugIn.iTimeFlag0);

          _RewriteClockCode ();
        }   ImGui::SameLine ();
        if (ImGui::InputInt ("t1", &SK_CC_PlugIn.iTimeFlag1))
        {
                      SK_CC_PlugIn.iTimeFlag1 =
          std::clamp (SK_CC_PlugIn.iTimeFlag1, 0, 127);

          SK_CC_PlugIn.ini.time_flag1->store (SK_CC_PlugIn.iTimeFlag1);

          _RewriteClockCode ();
        }
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
                                 ImColor (0.5f,0.5f,0.5f,1.0f) :
                                 ImColor (1.0f,1.0f,1.0f,1.0f) );

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
          /  L"d3d12_sk0_crc32c_ae7c1bb2.dds",
                                   pathPlayStation_Old =
        SK_Resource_GetRoot () / LR"(inject/textures)"
          /  L"d3d12_sk0_crc32c_0041d76d.dds"; // Hash changed in 1.3.0, then changed back in 1.3.2

      // Remove the old texture mod, since there's extra overhead until they all load
      static bool had_old =
        std::filesystem::exists (pathPlayStation_Old, ec) ?
        std::filesystem::remove (pathPlayStation_Old, ec) : false;

      static bool                   bPlayStation_AtStart =
        std::filesystem::exists (pathPlayStation, ec),
                                    bPlayStation =
                                    bPlayStation_AtStart,
                                    bFetching   = false;

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
                               R"(buttons/d3d12_sk0_crc32c_ae7c1bb2.dds)",
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

      ImGui::TreePop     ( );
    }
#endif

    ImGui::PopStyleColor (3);
    ImGui::TreePop       ( );
  }

  return true;
}

void
SK_CC_InitConfig (void)
{
  SK_CC_PlugIn.ini.unlock_framerate =
    _CreateConfigParameterBool ( L"ChronoCross.PlugIn",
                                 L"UnlockFramerate", SK_CC_PlugIn.bUnlockFramerate,
                                                          L"Remove Internal FPS Limit" );

  if (! SK_CC_PlugIn.ini.unlock_framerate->load  (SK_CC_PlugIn.bUnlockFramerate))
        SK_CC_PlugIn.ini.unlock_framerate->store (true);

  SK_CC_PlugIn.ini.time_flag0 =
    _CreateConfigParameterInt ( L"ChronoCross.PlugIn",
                                 L"TimeFlag0", SK_CC_PlugIn.iTimeFlag0,
                                                          L"Misc. Timing" );

  if (! SK_CC_PlugIn.ini.time_flag0->load  (SK_CC_PlugIn.iTimeFlag0))
        SK_CC_PlugIn.ini.time_flag0->store (0x1);

  SK_CC_PlugIn.ini.time_flag1 =
    _CreateConfigParameterInt ( L"ChronoCross.PlugIn",
                                 L"TimeFlag1", SK_CC_PlugIn.iTimeFlag1,
                                                          L"Misc. Timing" );

  if (! SK_CC_PlugIn.ini.time_flag1->load  (SK_CC_PlugIn.iTimeFlag1))
        SK_CC_PlugIn.ini.time_flag1->store (0x1);

  auto& addresses =
    SK_CC_PlugIn.addresses;

  addresses [L"CHRONO CROSS  1.0.0.0"].
   cached =
        { { "clock_tick0", 0x007116F }, { "clock_tick1", 0x0071B16 } };

  std::wstring game_ver_str =
    SK_GetDLLVersionStr (SK_GetHostApp ());

#if 0
  if (! addresses.contains (game_ver_str))
  {
    void* pBaseAddr =
      SK_Debug_GetImageBaseAddr ();

    const uint8_t pattern0 [] = { 0xC7, 0x43, 0x18, 0x02, 0x00,
                                  0x00, 0x00, 0xc7, 0x43, 0x20 };
    const uint8_t pattern1 [] = { 0x18, 0xC7, 0x43, 0x20, 0x89 };
    const uint8_t pattern2 [] = { 0x7B, 0x18, 0xC7, 0x43, 0x20 };

    auto
      p0 = (void *)( 7 + static_cast <uint8_t *>
       ( SK_ScanAlignedEx ( pattern0, 10, nullptr, pBaseAddr ) ) ),
      p1 = (void *)( 7 + static_cast <uint8_t *>
       ( SK_ScanAlignedEx ( pattern0, 10, nullptr, p0 ) ) ),
      p2 = (void *)( 7 + static_cast <uint8_t *>
       ( SK_ScanAlignedEx ( pattern0, 10, nullptr, p1 ) ) );

    auto
      p3 = (void *)( 1 + static_cast <uint8_t *>
       ( SK_ScanAlignedEx ( pattern1, 5, nullptr, pBaseAddr ) ) ),
      p4 = (void *)( 1 + static_cast <uint8_t *>
       ( SK_ScanAlignedEx ( pattern1, 5, nullptr, p3 ) ) );

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

    for ( const auto &[name, address] :
                             addresses [game_ver_str].cached )
    {
      SK_LOG0 ( ( L"Uncached Address (%hs) => %ph", name.c_str (), address ),
                  L"Elden Ring" );
    }
  }
#endif

  auto& addr_cache =
    SK_CC_PlugIn.addresses [game_ver_str].cached;

  for ( auto &[record, name] :
          std::initializer_list <
            std::pair <cc_code_patch_s&, std::string> >
              { { SK_CC_PlugIn.clock_tick0, "clock_tick0" },
                { SK_CC_PlugIn.clock_tick1, "clock_tick1" } } )
  {
    record = {
      .pAddr       = (void *)addr_cache [name],
      .original    = code_bytes_t <4> {0   ,0   ,0,   0   },
      .replacement = code_bytes_t <4> {0x83,0x46,0x14,0x01} };
  }
}

void
SK_CC_InitPlugin (void)
{
  SK_CC_InitConfig ();

  auto& addr_cache =
    SK_CC_PlugIn.addresses [SK_GetDLLVersionStr (SK_GetHostApp ())].cached;

  if (             const auto* pClockTick0 = (uint8_t *)
       SK_Debug_GetImageBaseAddr () +
                            addr_cache ["clock_tick0"] ;
       SK_ValidatePointer     (pClockTick0) &&
       SK_IsAddressExecutable (pClockTick0) &&
                              *pClockTick0  == 0x83 )
  {
    auto *cp =
      SK_Render_InitializeSharedCVars ();

    if (! cp)
    {
      SK_ImGui_Warning (L"Special K Command Processor is Busted...");

      return;
    }

    auto _RewriteClockCode = [&](void)
 -> void
    {
      for ( auto patch : { &SK_CC_PlugIn.clock_tick0,
                           &SK_CC_PlugIn.clock_tick1 } )
      {
        if (SK_CC_PlugIn.bUnlockFramerate)
        {
          auto pFlagAddr =
             ((uint8_t *)patch->pAddr + 3);
          patch->apply (&patch->replacement);

          DWORD                                                 dwOldProt;
          VirtualProtect (pFlagAddr, 1, PAGE_EXECUTE_READWRITE,&dwOldProt);
          if (patch == &SK_CC_PlugIn.clock_tick0)
            *pFlagAddr = (uint8_t)SK_CC_PlugIn.iTimeFlag0;
          else
            *pFlagAddr = (uint8_t)SK_CC_PlugIn.iTimeFlag1;
          VirtualProtect (pFlagAddr, 1, dwOldProt,             &dwOldProt);
        }

        else
          patch->apply (&patch->original);
      }
    };


    for ( auto &[patch, name] :
        std::initializer_list <
          std::pair <cc_code_patch_s&, std::string> >
            { { SK_CC_PlugIn.clock_tick0, "clock_tick0" },
              { SK_CC_PlugIn.clock_tick1, "clock_tick1" } } )
    {
      patch.pAddr = (void *)addr_cache [name];

      (uintptr_t&)patch.pAddr +=
        (uintptr_t)SK_Debug_GetImageBaseAddr ();

      if (patch.original.inst_bytes [0] == 0x0)
      {
        memcpy (
          patch.original.inst_bytes.data (),
          patch.pAddr, 4
        );
      }
    }

    _RewriteClockCode ();

  //plugin_mgr->end_frame_fns.emplace (SK_CC_EndFrame );
    plugin_mgr->config_fns.emplace    (SK_CC_PlugInCfg);

    return;
  }

  SK_ImGui_Warning (
    L"This version of Chrono Cross is not Compatible with Special K's Plug-In"
  );
}