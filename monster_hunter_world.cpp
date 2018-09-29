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


volatile LONG __SK_MHW_QueuedShots         = 0;
volatile LONG __SK_MHW_InitiateHudFreeShot = 0;

volatile LONG __SK_ScreenShot_CapturingHUDless = 0;

extern volatile LONG
__SK_DQXI_QueuedShots;

extern volatile LONG
__SK_DQXI_InitiateHudFreeShot;

void
SK_TriggerHudFreeScreenshot (void) noexcept
{
  InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);

  InterlockedIncrement (&__SK_MHW_QueuedShots);
  InterlockedIncrement (&__SK_DQXI_QueuedShots);
}

#include <SpecialK\widgets\widget.h>

void
SK_MHW_BeginFrame (void)
{
  if ( ReadAcquire (&__SK_MHW_QueuedShots)          > 0 ||
       ReadAcquire (&__SK_MHW_InitiateHudFreeShot) != 0    )
  {
    InterlockedExchange (&__SK_ScreenShot_CapturingHUDless, 1);

#define SK_MHW_HUD_VS0_CRC32C  0x6f046ebc // General 2D HUD
#define SK_MHW_HUD_VS1_CRC32C  0x711c9eeb // The HUD cursor particles

    if (InterlockedCompareExchange (&__SK_MHW_InitiateHudFreeShot, -2, 1) == 1)
    {
      static auto& shaders =
        SK_D3D11_Shaders;

      shaders.vertex.blacklist.emplace (SK_MHW_HUD_VS0_CRC32C);
      shaders.vertex.blacklist.emplace (SK_MHW_HUD_VS1_CRC32C);
    }

    // 1-frame Delay for SDR->HDR Upconversion
    else if (InterlockedCompareExchange (&__SK_MHW_InitiateHudFreeShot, -1, -2) == -2)
    {
      SK::SteamAPI::TakeScreenshot (SK::ScreenshotStage::EndOfFrame);
      InterlockedDecrement (&SK_D3D11_DrawTrackingReqs);
    }

    else if (! ReadAcquire (&__SK_MHW_InitiateHudFreeShot))
    {
      InterlockedDecrement (&__SK_MHW_QueuedShots);
      InterlockedExchange  (&__SK_MHW_InitiateHudFreeShot, 1);

      return
        SK_MHW_BeginFrame ();
    }

    return;
  }

  InterlockedExchange (&__SK_ScreenShot_CapturingHUDless, 0);
}

void
SK_MHW_EndFrame (void)
{
  if (InterlockedCompareExchange (&__SK_MHW_InitiateHudFreeShot, 0, -1) == -1)
  {
    static auto& shaders =
      SK_D3D11_Shaders;

    shaders.vertex.blacklist.erase (SK_MHW_HUD_VS1_CRC32C);
    shaders.vertex.blacklist.erase (SK_MHW_HUD_VS0_CRC32C);
  }
}

sk::ParameterBool*  _SK_MHW_JobParity;
bool  __SK_MHW_JobParity = true;
sk::ParameterBool*  _SK_MHW_JobParityPhysical;
bool  __SK_MHW_JobParityPhysical = false;

sk::ParameterBool* _SK_MHW_10BitSwapChain;
bool __SK_MHW_10BitSwap = false;

sk::ParameterBool* _SK_MHW_16BitSwapChain;
bool __SK_MHW_16BitSwap = false;

sk::ParameterFloat* _SK_MHW_scRGBLuminance;
float __SK_MHW_HDR_Luma = 172.0_Nits;

sk::ParameterFloat* _SK_MHW_scRGBGamma;
float __SK_MHW_HDR_Exp  = 1.0f;

sk::ParameterBool* _SK_MHW_KillAntiDebug;
bool __SK_MHW_KillAntiDebug  = true;
int  __SK_MHW_AntiDebugSleep = 0;

sk::ParameterInt*   _SK_MHW_AlternateTonemap;

#include <concurrent_vector.h>
extern concurrency::concurrent_vector <d3d11_shader_tracking_s::cbuffer_override_s> __SK_D3D11_PixelShader_CBuffer_Overrides;
d3d11_shader_tracking_s::cbuffer_override_s* SK_MHW_CB_Override;

#include <SpecialK/plugin/plugin_mgr.h>

void
SK_MHW_PlugInInit (void)
{
  config.render.framerate.enable_mmcss = false;

#define SK_MHW_CPU_SECTION     L"MonsterHunterWorld.CPU"
#define SK_MHW_CPU_SECTION_OLD L"MonsterHuntersWorld.CPU"

  _SK_MHW_JobParity =
    _CreateConfigParameterBool ( SK_MHW_CPU_SECTION,
                                 L"LimitJobThreads",      __SK_MHW_JobParity,
                                                          L"Job Parity",
                                SK_MHW_CPU_SECTION_OLD );
  _SK_MHW_JobParityPhysical =
    _CreateConfigParameterBool ( SK_MHW_CPU_SECTION,
                                 L"LimitToPhysicalCores", __SK_MHW_JobParityPhysical,
                                                          L"Job Parity (Physical)",
                                SK_MHW_CPU_SECTION_OLD );

#define SK_MHW_HDR_SECTION     L"MonsterHunterWorld.HDR"
#define SK_MHW_HDR_SECTION_OLD L"MonsterHuntersWorld.HDR"

  _SK_MHW_10BitSwapChain =
    _CreateConfigParameterBool ( SK_MHW_HDR_SECTION,
                                 L"Use10BitSwapChain",  __SK_MHW_10BitSwap,
                                                        L"10-bit SwapChain",
                                 SK_MHW_HDR_SECTION_OLD );
  _SK_MHW_16BitSwapChain =
    _CreateConfigParameterBool ( SK_MHW_HDR_SECTION,
                                 L"Use16BitSwapChain",  __SK_MHW_16BitSwap,
                                                        L"16-bit SwapChain",
                                 SK_MHW_HDR_SECTION_OLD );

  _SK_MHW_scRGBLuminance =
    _CreateConfigParameterFloat ( SK_MHW_HDR_SECTION,
                                  L"scRGBLuminance",  __SK_MHW_HDR_Luma,
                                                      L"scRGB Luminance",
                                  SK_MHW_HDR_SECTION_OLD );
  _SK_MHW_scRGBGamma =
    _CreateConfigParameterFloat ( SK_MHW_HDR_SECTION,
                                  L"scRGBGamma",      __SK_MHW_HDR_Exp,
                                                      L"scRGB Gamma",
                                  SK_MHW_HDR_SECTION_OLD );


  __SK_D3D11_PixelShader_CBuffer_Overrides.push_back
  (
/*
 * 0: Hash,    1: CBuffer Size
 * 2: Enable?, 3: Binding Slot,
 * 4: Offset,  5: Value List Size (in bytes),
 * 6: Value List
 */
    { 0x08cc13a6, 52,
      false,      3,
      0,          4,
      { 0.0f }
    }
  );


  SK_MHW_CB_Override =
    &__SK_D3D11_PixelShader_CBuffer_Overrides.back ();

  *(reinterpret_cast <UINT *> (SK_MHW_CB_Override->Values)) =
         static_cast <UINT  > (-1);

  int* pCBufferOverrideVal =
    reinterpret_cast <int *> (SK_MHW_CB_Override->Values);

  _SK_MHW_AlternateTonemap =
    _CreateConfigParameterInt ( SK_MHW_HDR_SECTION,
                                L"AlternateTonemap", *pCBufferOverrideVal,
                                                     L"Tonemap Type",
                                SK_MHW_HDR_SECTION_OLD );

  if (*(reinterpret_cast <int *> (SK_MHW_CB_Override->Values)) > -1)
  {
    SK_MHW_CB_Override->Enable = true;
  }

  else
    SK_MHW_CB_Override->Enable = false;

  if (SK_MHW_CB_Override->Enable)
  {
    InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
    InterlockedIncrement (&SK_D3D11_CBufferTrackingReqs);
  }



  _SK_MHW_KillAntiDebug =
    _CreateConfigParameterBool ( SK_MHW_CPU_SECTION,
                                 L"KillAntiDebugCode",    __SK_MHW_KillAntiDebug,
                                                          L"Anti-Debug Kill Switch",
                                 SK_MHW_CPU_SECTION_OLD );


  if (__SK_MHW_16BitSwap)
  {
    SK_GetCurrentRenderBackend ().scanout.colorspace_override =
      DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709;
  }


  iSK_INI* pINI =
    SK_GetDLLConfig ();

  pINI->remove_section (SK_MHW_CPU_SECTION_OLD);
  pINI->remove_section (SK_MHW_HDR_SECTION_OLD);

  pINI->write (pINI->get_filename ());
}


void
SK_MHW_ThreadStartStop (HANDLE hThread, int op = 0)
{
  static concurrency::concurrent_unordered_set <HANDLE>
    stopped_threads;

  if (op == 0)
  {
    if (! stopped_threads.count (hThread))
    {
      stopped_threads.insert (hThread);
      SuspendThread          (hThread);
    }
  }

  if (op == 1)
  {
    if (stopped_threads.count (hThread))
    {
      std::unordered_set <HANDLE> stopped_copy {
        stopped_threads.begin (), stopped_threads.end ()
      };

      stopped_threads.clear ();

      for (auto& it : stopped_threads)
      {
        if (it != hThread)
          stopped_threads.insert (hThread);

        else
          ResumeThread (hThread);
      }
    }
  }

  if (op == 2)
  {
    std::unordered_set <HANDLE> stopped_copy {
      stopped_threads.begin (), stopped_threads.end ()
    };

    stopped_threads.clear ();

    for (auto& it : stopped_copy)
    {
      TerminateThread (it, 0x0);
      //ResumeThread (it);
    }
  }
}

extern void
SK_MHW_SuspendThread (HANDLE hThread)
{
  SK_MHW_ThreadStartStop (hThread, 0);
}

void
SK_MHW_PlugIn_Shutdown (void)
{
  // Resume all stopped threads prior
  //   to shutting down
  SK_MHW_ThreadStartStop (0, 2);
}

bool
SK_MHW_PlugInCfg (void)
{
  iSK_INI* pINI =
    SK_GetDLLConfig ();

  if (ImGui::CollapsingHeader ("MONSTER HUNTER: WORLD", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::TreePush ("");

    static bool parity_orig = __SK_MHW_JobParity;

    if (ImGui::Checkbox ("Limit Job Threads to number of CPU cores", &__SK_MHW_JobParity))
    {
      _SK_MHW_JobParity->store (__SK_MHW_JobParity);
      pINI->write (pINI->get_filename ());
    }

    static bool rule_orig =
      __SK_MHW_JobParityPhysical;

    if (__SK_MHW_JobParity)
    {
      ImGui::SameLine (); ImGui::Spacing ();
      ImGui::SameLine (); ImGui::Spacing ();
      ImGui::SameLine (); ImGui::Text ("Limit: ");
      ImGui::SameLine (); ImGui::Spacing ();
      ImGui::SameLine ();

      int rule = __SK_MHW_JobParityPhysical ? 1 : 0;

      bool changed = false;

      extern size_t
      SK_CPU_CountLogicalCores (void);

      static bool has_logical_processors =
        SK_CPU_CountLogicalCores () > 0;

      if (has_logical_processors)
      {
        changed |=
          ImGui::RadioButton ("Logical Cores", &rule, 0);
        ImGui::SameLine ();
      }
      else
        rule = 1;

      changed |=
        ImGui::RadioButton ("Physical Cores", &rule, 1);

      if (changed)
      {
        __SK_MHW_JobParityPhysical = (rule == 1);
        _SK_MHW_JobParityPhysical->store (__SK_MHW_JobParityPhysical);
        pINI->write (pINI->get_filename ());
      }
    }

    if ( parity_orig != __SK_MHW_JobParity ||
         rule_orig   != __SK_MHW_JobParityPhysical )
    {
      ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (.3f, .8f, .9f));
      ImGui::BulletText ("Game Restart Required");
      ImGui::PopStyleColor ();
    }

    if (ImGui::IsItemHovered ())
      ImGui::SetTooltip ("Without this option, the game spawns 32 job threads and nobody can get that many running efficiently.");

    if (ImGui::Checkbox ("Anti-Debug Killswitch", &__SK_MHW_KillAntiDebug))
    {
      _SK_MHW_KillAntiDebug->store (__SK_MHW_KillAntiDebug);
      pINI->write (pINI->get_filename ());
    }

    if (ImGui::IsItemHovered ())
      ImGui::SetTooltip ("Eliminate the kernel bottleneck Capcom added to prevent debugging.");

    if (__SK_MHW_KillAntiDebug)
    {
      ImGui::SameLine ();
      ImGui::DragInt  ("Sleep (ms) per-frame", &__SK_MHW_AntiDebugSleep, 1.0f, 0, 15);
    }


    if (ImGui::CollapsingHeader ("HDR Fix", ImGuiTreeNodeFlags_DefaultOpen))
    {
      static bool TenBitSwap_Original     = __SK_MHW_10BitSwap;
      static bool SixteenBitSwap_Original = __SK_MHW_16BitSwap;
      
      static int sel = __SK_MHW_16BitSwap ? 2 :
                       __SK_MHW_10BitSwap ? 1 : 0;

      if (ImGui::RadioButton ("None", &sel, 0))
      {
        __SK_MHW_10BitSwap = false;
        __SK_MHW_16BitSwap = false;

        _SK_MHW_10BitSwapChain->store (__SK_MHW_10BitSwap);
        _SK_MHW_16BitSwapChain->store (__SK_MHW_16BitSwap);

        pINI->write (pINI->get_filename ());
      }
      ImGui::SameLine ();


      if (ImGui::RadioButton ("HDR10 (10-bit + Metadata)", &sel, 1))
      {
        __SK_MHW_10BitSwap = true;

        if (__SK_MHW_10BitSwap) __SK_MHW_16BitSwap = false;

        _SK_MHW_10BitSwapChain->store (__SK_MHW_10BitSwap);
        _SK_MHW_16BitSwapChain->store (__SK_MHW_16BitSwap);

        pINI->write (pINI->get_filename ());
      }

      auto& rb =
        SK_GetCurrentRenderBackend ();

      if (rb.hdr_capable)
      {
        ImGui::SameLine ();

        if (ImGui::RadioButton ("scRGB HDR (16-bit)", &sel, 2))
        {
          __SK_MHW_16BitSwap = true;

          if (__SK_MHW_16BitSwap) __SK_MHW_10BitSwap = false;

          _SK_MHW_10BitSwapChain->store (__SK_MHW_10BitSwap);
          _SK_MHW_16BitSwapChain->store (__SK_MHW_16BitSwap);

          pINI->write (pINI->get_filename ());
        }

        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("This is the superior HDR format -- use it ;)");
      }

      if ( (TenBitSwap_Original     != __SK_MHW_10BitSwap ||
            SixteenBitSwap_Original != __SK_MHW_16BitSwap) )
      {
        ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (.3f, .8f, .9f));
        ImGui::BulletText     ("Game Restart Required");
        ImGui::PopStyleColor  ();
      }

      if ((__SK_MHW_10BitSwap || __SK_MHW_16BitSwap) && rb.hdr_capable)
      {
        CComQIPtr <IDXGISwapChain4> pSwap4 (rb.swapchain);

        if (pSwap4 != nullptr)
        {
          DXGI_OUTPUT_DESC1     out_desc = { };
          DXGI_SWAP_CHAIN_DESC swap_desc = { };
             pSwap4->GetDesc (&swap_desc);

          if (out_desc.BitsPerColor == 0)
          {
            CComPtr <IDXGIOutput> pOutput = nullptr;

            if (SUCCEEDED ((pSwap4->GetContainingOutput (&pOutput.p))))
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
              ///{ 0.70800f, 0.29200f, 0.17000f, 0.79700f, 0.13100f, 0.04600f, 0.31270f, 0.32900f }, // Display Gamut Rec2020
              //( out_desc.RedPrimary   [0], out_desc.RedPrimary   [1],
              //out_desc.GreenPrimary [0], out_desc.GreenPrimary [1],
              //out_desc.BluePrimary  [0], out_desc.BluePrimary  [1],
              //out_desc.WhitePoint   [0], out_desc.WhitePoint   [1] ),
              //( out_desc.RedPrimary   [0], out_desc.RedPrimary   [1],
              //out_desc.GreenPrimary [0], out_desc.GreenPrimary [1],
              //out_desc.BluePrimary  [0], out_desc.BluePrimary  [1],
              //out_desc.WhitePoint   [0], out_desc.WhitePoint   [1] ),
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

            if (! (config.render.framerate.swapchain_wait != 0 && swap_desc.BufferDesc.Format == DXGI_FORMAT_R10G10B10A2_UNORM))// hdr_gamut_support)
            {
              HDR10MetaData.RedPrimary [0] = static_cast <UINT16> (DisplayChromacityList [cspace].RedX * 50000.0f);
              HDR10MetaData.RedPrimary [1] = static_cast <UINT16> (DisplayChromacityList [cspace].RedY * 50000.0f);

              HDR10MetaData.GreenPrimary [0] = static_cast <UINT16> (DisplayChromacityList [cspace].GreenX * 50000.0f);
              HDR10MetaData.GreenPrimary [1] = static_cast <UINT16> (DisplayChromacityList [cspace].GreenY * 50000.0f);

              HDR10MetaData.BluePrimary [0] = static_cast <UINT16> (DisplayChromacityList [cspace].BlueX * 50000.0f);
              HDR10MetaData.BluePrimary [1] = static_cast <UINT16> (DisplayChromacityList [cspace].BlueY * 50000.0f);

              HDR10MetaData.WhitePoint [0] = static_cast <UINT16> (DisplayChromacityList [cspace].WhiteX * 50000.0f);
              HDR10MetaData.WhitePoint [1] = static_cast <UINT16> (DisplayChromacityList [cspace].WhiteY * 50000.0f);

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
                float nits =
                  __SK_MHW_HDR_Luma / 1.0_Nits;

                
                if (ImGui::SliderFloat ( "###MHW_LUMINANCE", &nits, 80.0f, rb.display_gamut.maxLocalY,
                                           "Peak White Luminance: %.1f Nits" ))
                {
                  __SK_MHW_HDR_Luma = nits * 1.0_Nits;

                  _SK_MHW_scRGBLuminance->store (__SK_MHW_HDR_Luma);
                  pINI->write (pINI->get_filename ());
                }

                ImGui::SameLine ();

                if (ImGui::SliderFloat ("SDR -> HDR Gamma", &__SK_MHW_HDR_Exp, 1.0f, 2.5f))
                {
                  _SK_MHW_scRGBGamma->store (__SK_MHW_HDR_Exp);
                  pINI->write (pINI->get_filename ());
                }

                //ImGui::SameLine ();
                //ImGui::Checkbox ("Explicit LinearRGB -> sRGB###IMGUI_SRGB", &rb.ui_srgb);
              }

              if (swap_desc.BufferDesc.Format == DXGI_FORMAT_R10G10B10A2_UNORM && ImGui::Button ("Inject HDR10 Metadata"))
              {
                //if (cspace == 2)
                //  swap_desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
                //else if (cspace == 1)
                //  swap_desc.BufferDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
                //else
                //  swap_desc.BufferDesc.Format = DXGI_FORMAT_R10G10B10A2_UNORM;

                pSwap4->SetHDRMetaData (DXGI_HDR_METADATA_TYPE_NONE, 0, nullptr);

                if (swap_desc.BufferDesc.Format == DXGI_FORMAT_R16G16B16A16_FLOAT)
                {
                  pSwap4->SetColorSpace1 (DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709);
                }

                if      (cspace == 1) pSwap4->SetColorSpace1 (DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020);
                else if (cspace == 0) pSwap4->SetColorSpace1 (DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709);
                else                  pSwap4->SetColorSpace1 (DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709);

                if (cspace == 1 || cspace == 0)
                  pSwap4->SetHDRMetaData (DXGI_HDR_METADATA_TYPE_HDR10, sizeof (HDR10MetaData), &HDR10MetaData);
              }
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

      bool changed =
        ImGui::Checkbox ("Enable Alternate Tonemap", &SK_MHW_CB_Override->Enable);

      if (changed)
      {
        if (SK_MHW_CB_Override->Enable)
        {
          InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
          InterlockedIncrement (&SK_D3D11_CBufferTrackingReqs);
        }
        else
        {
          InterlockedDecrement (&SK_D3D11_DrawTrackingReqs);
          InterlockedDecrement (&SK_D3D11_CBufferTrackingReqs);
        }
      }

      if (ImGui::IsItemHovered ())
      {
        ImGui::SetTooltip ("You can significantly improve the washed out image by using an alternate tonemap.");
      }

      if (SK_MHW_CB_Override->Enable)
      {
        if (*(int *)(SK_MHW_CB_Override->Values) < 0)
        {
               *(int *)(SK_MHW_CB_Override->Values) = 1 +
          abs (*(int *) SK_MHW_CB_Override->Values);
        }

        ImGui::SameLine    ();
        ImGui::BeginGroup  ();
        changed |=
          ImGui::SliderInt ("Tonemap Type##SK_MHW_TONEMAP", (int *)SK_MHW_CB_Override->Values, 0, 8);
        ImGui::EndGroup    ();
      }

      if (changed)
      {
        int tonemap =
          ( SK_MHW_CB_Override->Enable ?        abs (*(int *)SK_MHW_CB_Override->Values)
                                       : (-1) - abs (*(int *)SK_MHW_CB_Override->Values) );
          
        _SK_MHW_AlternateTonemap->store (tonemap);
        pINI->write (pINI->get_filename ());
      }

      ImGui::PopStyleColor (3);
      ImGui::TreePop       ( );
    }

    ///static int orig =
    ///  config.render.framerate.override_num_cpus;
    ///
    ///bool spoof = (config.render.framerate.override_num_cpus != -1);
    ///
    ///static SYSTEM_INFO             si = { };
    ///SK_RunOnce (SK_GetSystemInfo (&si));
    ///
    ///if ((! spoof) || static_cast <DWORD> (config.render.framerate.override_num_cpus) > (si.dwNumberOfProcessors / 2))
    ///{
    ///  ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (.14f, .8f, .9f));
    ///  ImGui::BulletText     ("It is strongly suggested that you reduce threads to 1/2 max. or lower");
    ///  ImGui::PopStyleColor  ();
    ///}
    ///
    ///if ( ImGui::Checkbox   ("Reduce Reported CPU Core Count", &spoof) )
    ///{
    ///  config.render.framerate.override_num_cpus =
    ///    ( spoof ? si.dwNumberOfProcessors : -1 );
    ///}
    ///
    ///if (spoof)
    ///{
    ///  ImGui::SameLine  (                                             );
    ///  ImGui::SliderInt ( "",
    ///                    &config.render.framerate.override_num_cpus,
    ///                    1, si.dwNumberOfProcessors              );
    ///}
    ///
    ///if (config.render.framerate.override_num_cpus != orig)
    ///{
    ///  ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (.3f, .8f, .9f));
    ///  ImGui::BulletText     ("Game Restart Required");
    ///  ImGui::PopStyleColor  ();
    ///}

    ImGui::TreePop ();

    return true;
  }

  return false;
}