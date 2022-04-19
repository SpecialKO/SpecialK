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
  int     iClockMultiplier              = 0x1A;
  float   fResolutionScale              = 1.0f;

  struct {
    sk::ParameterBool*  unlock_framerate = nullptr;
    sk::ParameterInt*   time_flag0       = nullptr;
    sk::ParameterInt*   time_flag1       = nullptr;
    sk::ParameterInt*   clock_multiplier = nullptr;
    sk::ParameterFloat* resolution_scale = nullptr;
  } ini;

  std::unordered_map <
    std::wstring, address_cache_s
  > addresses;

  cc_code_patch_s clock_tick0, clock_tick1, clock_multi;
} SK_CC_PlugIn;

float __SK_CC_ResMultiplier = 1.0f;

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

static float fDelta  = 66.0f;
static int   iFrames = 0;

void
__stdcall
SK_CC_EndFrame (void)
{
  if (SK_CC_PlugIn.bUnlockFramerate)
  {
    if ( SK_CC_PlugIn.iTimeFlag1 == 0 &&
         SK_CC_PlugIn.iTimeFlag0 == 0 )
    {
      static DWORD dwLastMs =
        SK_timeGetTime ();
  
      static volatile ULONG64 ullLastChanged = 0;
      
      if (SK_CC_PlugIn.bUnlockFramerate)
      {
        if (std::exchange (dwLastMs, SK_timeGetTime ()) < SK_timeGetTime () - fDelta)
        {
          SK_CC_PlugIn.clock_tick1.apply (&SK_CC_PlugIn.clock_tick1.original);

          InterlockedExchange (&ullLastChanged, SK_GetFramesDrawn ());
        }

        else if (ReadULong64Acquire (&ullLastChanged) < SK_GetFramesDrawn () - iFrames)
        {
          SK_CC_PlugIn.clock_tick1.apply (&SK_CC_PlugIn.clock_tick1.replacement);
        }
      }
    }
  }
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
                             &SK_CC_PlugIn.clock_tick1,
                             &SK_CC_PlugIn.clock_multi } )
        {
          if (SK_CC_PlugIn.bUnlockFramerate)
          {
            auto pFlagAddr =
               ((uint8_t *)patch->pAddr + 3);
            patch->apply (&patch->replacement);

            DWORD                                                 dwOldProt;
            VirtualProtect (pFlagAddr, 1, PAGE_EXECUTE_READWRITE,&dwOldProt);
            if (     patch == &SK_CC_PlugIn.clock_tick0)
              *pFlagAddr = (uint8_t)SK_CC_PlugIn.iTimeFlag0;
            else if (patch == &SK_CC_PlugIn.clock_tick1)
              *pFlagAddr = (uint8_t)SK_CC_PlugIn.iTimeFlag1;
            else
              *pFlagAddr = (uint8_t)SK_CC_PlugIn.iClockMultiplier;
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
        ImGui::BeginGroup   ();
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
        if (ImGui::InputInt ("mul", &SK_CC_PlugIn.iClockMultiplier))
        {
                      SK_CC_PlugIn.iClockMultiplier =
          std::clamp (SK_CC_PlugIn.iClockMultiplier, 0, 127);

          SK_CC_PlugIn.ini.clock_multiplier->store (SK_CC_PlugIn.iClockMultiplier);

          _RewriteClockCode ();
        }
        ImGui::EndGroup ();

        if (SK_CC_PlugIn.iTimeFlag1 == 0 && config.system.log_level > 0)
        {
          ImGui::InputFloat ("Hitch Delta",  &fDelta);
          ImGui::InputInt   ("Hitch Frames", &iFrames);
        }
      }

      ImGui::TreePop     (  );
    }

    if ( ImGui::CollapsingHeader (
           ICON_FA_IMAGE "\tGraphics Settings",
                          ImGuiTreeNodeFlags_DefaultOpen )
       )
    {
      static bool need_restart = false;

      ImGui::TreePush     ("");

      int resolution =
        static_cast <int> (SK_CC_PlugIn.fResolutionScale);

      ImGui::BeginGroup ();

      ImGui::Text ("Internal Resolution: "); ImGui::SameLine ();

      bool clicked  = false;
           clicked |= ImGui::RadioButton (" 720p ",  &resolution, 1); ImGui::SameLine ();
           clicked |= ImGui::RadioButton (" 1440p ", &resolution, 2); ImGui::SameLine ();
           clicked |= ImGui::RadioButton (" 4K ",    &resolution, 4);

      ImGui::EndGroup ();

      if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("Some UI elements are misaligned if set above 720p... [ TURN MSAA OFF ]");

      if (clicked)
      {
        SK_CC_PlugIn.fResolutionScale =
          static_cast <float> (resolution);

        SK_CC_PlugIn.ini.resolution_scale->store (
           SK_CC_PlugIn.fResolutionScale
        );

        need_restart = true;
      }

      if (need_restart)
        ImGui::BulletText ("Game Restart Required");
      ImGui::TreePop      (  );
    }

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

  SK_CC_PlugIn.ini.clock_multiplier =
    _CreateConfigParameterInt ( L"ChronoCross.PlugIn",
                                 L"ClockMultiplier", SK_CC_PlugIn.iClockMultiplier,
                                                          L"Misc. Timing" );

  if (! SK_CC_PlugIn.ini.clock_multiplier->load  (SK_CC_PlugIn.iClockMultiplier))
        SK_CC_PlugIn.ini.clock_multiplier->store (0x1A);

  SK_CC_PlugIn.ini.resolution_scale =
    _CreateConfigParameterFloat ( L"ChronoCross.PlugIn",
                                  L"ResolutionScale", SK_CC_PlugIn.fResolutionScale,
                                                                  L"Resolution" );

  if (! SK_CC_PlugIn.ini.resolution_scale->load  (SK_CC_PlugIn.fResolutionScale))
        SK_CC_PlugIn.ini.resolution_scale->store (1.0f);

  __SK_CC_ResMultiplier =
    std::clamp (SK_CC_PlugIn.fResolutionScale, 1.0f, 4.0f);

  auto& addresses =
    SK_CC_PlugIn.addresses;

  addresses [L"CHRONO CROSS  1.0.0.0"].
   cached =
        { { "clock_tick0", 0x007116F }, { "clock_tick1", 0x0071B16 },
          { "clock_multi", 0x004FA5F } };

  std::wstring game_ver_str =
    SK_GetDLLVersionStr (SK_GetHostApp ());

  auto& addr_cache =
    SK_CC_PlugIn.addresses [game_ver_str].cached;

  for ( auto &[record, name] :
          std::initializer_list <
            std::pair <cc_code_patch_s&, std::string> >
              { { SK_CC_PlugIn.clock_tick0, "clock_tick0" },
                { SK_CC_PlugIn.clock_tick1, "clock_tick1" },
                { SK_CC_PlugIn.clock_multi, "clock_multi" } } )
  {
    record = {
      .pAddr       = (void *)addr_cache [name],
      .original    = code_bytes_t <4> {0   ,0   ,0,   0   },
      .replacement = code_bytes_t <4> {0x83,0x46,0x14,0x01} };
  }

  SK_CC_PlugIn.clock_multi.replacement.inst_bytes = code_bytes_t <4> {0x10,0x6B,0xC6,0x1A};

  DWORD dwOriginal = 0x0;

  if (*(uint8_t *)((uintptr_t)SK_Debug_GetImageBaseAddr () + 0x731c7) == 0x75)
  {
    VirtualProtect ((void *)((uintptr_t)SK_Debug_GetImageBaseAddr () + 0x731c7), 2, PAGE_EXECUTE_READWRITE, &dwOriginal);
    memcpy         ((void *)((uintptr_t)SK_Debug_GetImageBaseAddr () + 0x731c7), "\x90\x90", 2);
    VirtualProtect ((void *)((uintptr_t)SK_Debug_GetImageBaseAddr () + 0x731c7), 2, dwOriginal, &dwOriginal);
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
                           &SK_CC_PlugIn.clock_tick1,
                           &SK_CC_PlugIn.clock_multi } )
      {
        if (SK_CC_PlugIn.bUnlockFramerate)
        {
          auto pFlagAddr =
             ((uint8_t *)patch->pAddr + 3);
          patch->apply (&patch->replacement);

          DWORD                                                 dwOldProt;
          VirtualProtect (pFlagAddr, 1, PAGE_EXECUTE_READWRITE,&dwOldProt);
          if (     patch == &SK_CC_PlugIn.clock_tick0)
            *pFlagAddr = (uint8_t)SK_CC_PlugIn.iTimeFlag0;
          else if (patch == &SK_CC_PlugIn.clock_tick1)
            *pFlagAddr = (uint8_t)SK_CC_PlugIn.iTimeFlag1;
          else if (patch == &SK_CC_PlugIn.clock_multi)
            *pFlagAddr = (uint8_t)SK_CC_PlugIn.iClockMultiplier;
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
              { SK_CC_PlugIn.clock_tick1, "clock_tick1" },
              { SK_CC_PlugIn.clock_multi, "clock_multi" } } )
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

    plugin_mgr->begin_frame_fns.emplace (SK_CC_EndFrame );
    plugin_mgr->config_fns.emplace      (SK_CC_PlugInCfg);

    return;
  }

  SK_ImGui_Warning (
    L"This version of Chrono Cross is not Compatible with Special K's Plug-In"
  );
}









// Garbage dump, need to hook these functions
//
#if 0
  void STDMETHODCALLTYPE RSSetViewports (
    _In_range_     (0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE) UINT          NumViewports,
    _In_reads_opt_ (NumViewports)                                          const D3D11_VIEWPORT *pViewports ) override
  {
    TraceAPI

#ifndef _M_AMD64
    static auto game_id = SK_GetCurrentGameID ();

    if (game_id == SK_GAME_ID::ChronoCross)
    {
      extern float
          __SK_CC_ResMultiplier;
      if (__SK_CC_ResMultiplier)
      {
        if (NumViewports == 1)
        {
          SK_ComPtr <ID3D11RenderTargetView> rtv;
          OMGetRenderTargets            (1, &rtv, nullptr);

          SK_ComPtr <ID3D11Texture2D> pTex;

          if (rtv.p != nullptr)
          {
            SK_ComPtr <ID3D11Resource> pRes;
            rtv->GetResource         (&pRes.p);

            if (pRes.p != nullptr)
              pRes->QueryInterface <ID3D11Texture2D> (&pTex.p);

            if (pTex.p != nullptr)
            {
              D3D11_VIEWPORT vp = *pViewports;

              D3D11_TEXTURE2D_DESC texDesc = { };
              pTex->GetDesc      (&texDesc);

              if ( texDesc.Width  == 4096 * __SK_CC_ResMultiplier &&
                   texDesc.Height == 2048 * __SK_CC_ResMultiplier )
              {
                static float NewWidth  = 4096.0f * __SK_CC_ResMultiplier;
                static float NewHeight = 2048.0f * __SK_CC_ResMultiplier;

                float left_ndc = 2.0f * ( vp.TopLeftX / 4096.0f) - 1.0f;
                float top_ndc  = 2.0f * ( vp.TopLeftY / 2048.0f) - 1.0f;

                vp.TopLeftX = (left_ndc * NewWidth  + NewWidth)  / 2.0f;
                vp.TopLeftY = (top_ndc  * NewHeight + NewHeight) / 2.0f;
                vp.Width    = __SK_CC_ResMultiplier * vp.Width;
                vp.Height   = __SK_CC_ResMultiplier * vp.Height;

                vp.TopLeftX = std::min (vp.TopLeftX, 32767.0f);
                vp.TopLeftY = std::min (vp.TopLeftY, 32767.0f);

                vp.Width    = std::min (vp.Width,  32767.0f);
                vp.Height   = std::min (vp.Height, 32767.0f);

                pReal->RSSetViewports (1, &vp);

                return;
              }
            }
          }
        }
      }
    }
#endif

    pReal->RSSetViewports (
             NumViewports,
               pViewports
    );
  }

  void STDMETHODCALLTYPE CopySubresourceRegion (
    _In_           ID3D11Resource *pDstResource,
    _In_           UINT            DstSubresource,
    _In_           UINT            DstX,
    _In_           UINT            DstY,
    _In_           UINT            DstZ,
    _In_           ID3D11Resource *pSrcResource,
    _In_           UINT            SrcSubresource,
    _In_opt_ const D3D11_BOX      *pSrcBox ) override
  {
    TraceAPI

#ifndef _M_AMD64
    static auto game_id = SK_GetCurrentGameID ();

    D3D11_BOX newBox = { };

    if (game_id == SK_GAME_ID::ChronoCross)
    {
      extern float
          __SK_CC_ResMultiplier;
      if (__SK_CC_ResMultiplier > 1.0f)
      {
        SK_ComQIPtr <ID3D11Texture2D>
          pSrcTex (pSrcResource),
          pDstTex (pDstResource);

        if ( pSrcTex.p != nullptr &&
             pDstTex.p != nullptr )
        {
          D3D11_TEXTURE2D_DESC srcDesc = { },
                               dstDesc = { };
          pSrcTex->GetDesc   (&srcDesc);
          pDstTex->GetDesc   (&dstDesc);

          if (srcDesc.Width == __SK_CC_ResMultiplier * 4096 && srcDesc.Height == __SK_CC_ResMultiplier * 2048 && pSrcBox != nullptr &&
              dstDesc.Width == __SK_CC_ResMultiplier * 4096 && dstDesc.Height == __SK_CC_ResMultiplier * 2048)
          {
            newBox = *pSrcBox;

            static float NewWidth  = 2048 * __SK_CC_ResMultiplier;
            static float NewHeight = 1024 * __SK_CC_ResMultiplier;

            float left_ndc   = 2.0f * ( static_cast <float> (std::clamp (newBox.left,   0U, 4096U)) / 4096.0f ) - 1.0f;
            float top_ndc    = 2.0f * ( static_cast <float> (std::clamp (newBox.top,    0U, 2048U)) / 2048.0f ) - 1.0f;
            float right_ndc  = 2.0f * ( static_cast <float> (std::clamp (newBox.right,  0U, 4096U)) / 4096.0f ) - 1.0f;
            float bottom_ndc = 2.0f * ( static_cast <float> (std::clamp (newBox.bottom, 0U, 2048U)) / 2048.0f ) - 1.0f;

            newBox.left   = static_cast <UINT> (std::max (0.0f, (left_ndc   * NewWidth  + NewWidth)  ));
            newBox.top    = static_cast <UINT> (std::max (0.0f, (top_ndc    * NewHeight + NewHeight) ));
            newBox.right  = static_cast <UINT> (std::max (0.0f, (right_ndc  * NewWidth  + NewWidth)  ));
            newBox.bottom = static_cast <UINT> (std::max (0.0f, (bottom_ndc * NewHeight + NewHeight) ));

            DstX *= __SK_CC_ResMultiplier;
            DstY *= __SK_CC_ResMultiplier;

            pSrcBox = &newBox;
          }

          else if (pSrcBox != nullptr && ( pSrcBox->right  > srcDesc.Width ||
                                           pSrcBox->bottom > srcDesc.Height ))
          {
            SK_LOGi0 ( L"xxxDesc={%dx%d}, Box={%d/%d::%d,%d}",
                         srcDesc.Width, srcDesc.Height,
                           pSrcBox->left,  pSrcBox->top,
                           pSrcBox->right, pSrcBox->bottom );

            return;
          }
        }
      }
    }
#endif

#ifndef SK_D3D11_LAZY_WRAP
  if (! SK_D3D11_IgnoreWrappedOrDeferred (true, pReal))
        SK_D3D11_CopySubresourceRegion_Impl (pReal,
                 pDstResource, DstSubresource, DstX, DstY, DstZ,
                 pSrcResource, SrcSubresource, pSrcBox, TRUE
        );
    else
#endif
      pReal->CopySubresourceRegion ( pDstResource, DstSubresource, DstX, DstY, DstZ,
                                     pSrcResource, SrcSubresource, pSrcBox );
  }
#endif