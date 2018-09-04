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

void
SK_DQXI_BeginFrame (void)
{
  if ( ReadAcquire (&__SK_DQXI_QueuedShots)          > 0 ||
       ReadAcquire (&__SK_DQXI_InitiateHudFreeShot) != 0    )
  {
    InterlockedExchange (&__SK_ScreenShot_CapturingHUDless, 1);

#define SK_DQXI_HUD_VS0_CRC32C  0x6f046ebc // General 2D HUD
#define SK_DQXI_HUD_VS1_CRC32C  0x711c9eeb // The HUD cursor particles

    if (InterlockedCompareExchange (&__SK_DQXI_InitiateHudFreeShot, -2, 1) == 1)
    {
      static auto& shaders =
        SK_D3D11_Shaders;

      shaders.vertex.blacklist.emplace (SK_DQXI_HUD_VS0_CRC32C);
      shaders.vertex.blacklist.emplace (SK_DQXI_HUD_VS1_CRC32C);
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

    //shaders.vertex.blacklist.erase (SK_DQXI_HUD_VS1_CRC32C);
    //shaders.vertex.blacklist.erase (SK_DQXI_HUD_VS0_CRC32C);
  }
}


sk::ParameterBool* _SK_DQXI_10BitSwapChain;
bool __SK_DQXI_10BitSwap = false;

sk::ParameterBool* _SK_DQXI_16BitSwapChain;
bool __SK_DQXI_16BitSwap = false;

sk::ParameterFloat* _SK_DQXI_scRGBLuminance;
float __SK_DQXI_HDR_Luma = 172.0_Nits;

sk::ParameterFloat* _SK_DQXI_scRGBGamma;
float __SK_DQXI_HDR_Exp  = 2.116f;

#include <concurrent_vector.h>
extern concurrency::concurrent_vector <d3d11_shader_tracking_s::cbuffer_override_s> __SK_D3D11_PixelShader_CBuffer_Overrides;
d3d11_shader_tracking_s::cbuffer_override_s* SK_DQXI_CB_Override;

#include <SpecialK/plugin/plugin_mgr.h>

void
SK_DQXI_PlugInInit (void)
{
  config.render.framerate.enable_mmcss = false;

#define SK_DQXI_HDR_SECTION     L"DragonQuestXI.HDR"

  _SK_DQXI_10BitSwapChain =
    _CreateConfigParameterBool ( SK_DQXI_HDR_SECTION,
                                L"Use10BitSwapChain",  __SK_DQXI_10BitSwap,
                                L"10-bit SwapChain" );
  _SK_DQXI_16BitSwapChain =
    _CreateConfigParameterBool ( SK_DQXI_HDR_SECTION,
                                L"Use16BitSwapChain",  __SK_DQXI_16BitSwap,
                                L"16-bit SwapChain" );

  _SK_DQXI_scRGBLuminance =
    _CreateConfigParameterFloat ( SK_DQXI_HDR_SECTION,
                                 L"scRGBLuminance",  __SK_DQXI_HDR_Luma,
                                 L"scRGB Luminance" );
  _SK_DQXI_scRGBGamma =
    _CreateConfigParameterFloat ( SK_DQXI_HDR_SECTION,
                                 L"scRGBGamma",      __SK_DQXI_HDR_Exp,
                                 L"scRGB Gamma"     );

  //if (SK_MHW_CB_Override->Enable)
  //{
  //  InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
  //  InterlockedIncrement (&SK_D3D11_CBufferTrackingReqs);
  //}

  iSK_INI* pINI =
    SK_GetDLLConfig ();

  pINI->write (pINI->get_filename ());
}

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

    if (ImGui::CollapsingHeader ("HDR Retrofit", ImGuiTreeNodeFlags_DefaultOpen))
    {
      static bool TenBitSwap_Original     = __SK_DQXI_10BitSwap;
      static bool SixteenBitSwap_Original = __SK_DQXI_16BitSwap;

      static int sel = __SK_DQXI_16BitSwap ? 2 :
                       __SK_DQXI_10BitSwap ? 1 : 0;

      if (ImGui::RadioButton ("None", &sel, 0))
      {
        __SK_DQXI_10BitSwap = false;
        __SK_DQXI_16BitSwap = false;

        _SK_DQXI_10BitSwapChain->store (__SK_DQXI_10BitSwap);
        _SK_DQXI_16BitSwapChain->store (__SK_DQXI_16BitSwap);

        pINI->write (pINI->get_filename ());
      }

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
        ImGui::SameLine ();

        if (ImGui::RadioButton ("scRGB HDR (16-bit)", &sel, 2))
        {
          __SK_DQXI_16BitSwap = true;

          if (__SK_DQXI_16BitSwap) __SK_DQXI_10BitSwap = false;

          _SK_DQXI_10BitSwapChain->store (__SK_DQXI_10BitSwap);
          _SK_DQXI_16BitSwapChain->store (__SK_DQXI_16BitSwap);

          pINI->write (pINI->get_filename ());
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
                float nits =
                  __SK_DQXI_HDR_Luma / 1.0_Nits;


                if (ImGui::SliderFloat ( "###DQXI_LUMINANCE", &nits, 80.0f, rb.display_gamut.maxLocalY,
                    "Peak White Luminance: %.1f Nits" ))
                {
                  __SK_DQXI_HDR_Luma = nits * 1.0_Nits;

                  _SK_DQXI_scRGBLuminance->store (__SK_DQXI_HDR_Luma);

                  pINI->write (pINI->get_filename ());
                }

                ImGui::SameLine ();

                if (ImGui::SliderFloat ("SDR -> HDR Gamma", &__SK_DQXI_HDR_Exp, 1.6f, 2.9f))
                {
                  _SK_DQXI_scRGBGamma->store (__SK_DQXI_HDR_Exp);
                  pINI->write (pINI->get_filename ());
                }

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