/**
 * This file is part of Special K.
 *
 * Special K is free software : you can redistribute it
 * and/or modify it under the terms of the GNU General Public License
 * as published by The Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * Special K is distributed in the hope that it will be useful,
 *
 * But WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Special K.
 *
 *   If not, see <http://www.gnu.org/licenses/>.
 *
**/

#include <SpecialK/stdafx.h>
#include <SpecialK/widgets/widget.h>

#include <SpecialK/render/dxgi/dxgi_backend.h>

#include <SpecialK/plugin/plugin_mgr.h>

#define SK_HDR_SECTION     L"SpecialK.HDR"
#define SK_MISC_SECTION    L"SpecialK.Misc"

extern iSK_INI*             dll_ini;
extern sk::ParameterFactory g_ParameterFactory;


extern int   __SK_HDR_input_gamut;
extern int   __SK_HDR_output_gamut;


auto constexpr
DeclKeybind = [](SK_ConfigSerializedKeybind* binding, iSK_INI* ini, const wchar_t* sec) ->
auto
{
  auto* ret =
    dynamic_cast <sk::ParameterStringW *>
      (g_ParameterFactory.create_parameter <std::wstring> (L"DESCRIPTION HERE"));

  if (ret != nullptr)
  {
    ret->register_to_ini ( ini, sec, binding->short_name );
  }

  return ret;
};

auto constexpr
Keybinding = [] (SK_Keybind* binding, sk::ParameterStringW* param) ->
auto
{
  if (param == nullptr)
    return false;

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


void SK_ImGui_DrawGamut (void);

sk::ParameterBool* _SK_HDR_10BitSwapChain;
sk::ParameterBool* _SK_HDR_16BitSwapChain;
sk::ParameterInt*  _SK_HDR_ActivePreset;
sk::ParameterBool* _SK_HDR_FullRange;

bool __SK_HDR_10BitSwap = false;
bool __SK_HDR_16BitSwap = false;

float __SK_HDR_Luma      = 80.0_Nits;//300.0_Nits;
float __SK_HDR_Exp       = 1.0f;
int   __SK_HDR_Preset    = 0;
bool  __SK_HDR_FullRange = false;

float __SK_HDR_UI_Luma       =   1.0f;
float __SK_HDR_HorizCoverage = 100.0f;
float __SK_HDR_VertCoverage  = 100.0f;


#define MAX_HDR_PRESETS 4

struct SK_HDR_Preset_s {
  const char*  preset_name;
  int          preset_idx;

  float        peak_white_nits;
  float        eotf;

  struct {
    int in  = 0,
        out = 0;
  } colorspace;

  std::wstring annotation = L"";

  sk::ParameterFloat*   cfg_nits       = nullptr;
  sk::ParameterFloat*   cfg_eotf       = nullptr;
  sk::ParameterStringW* cfg_notes      = nullptr;
  sk::ParameterInt*     cfg_in_cspace  = nullptr;
  sk::ParameterInt*     cfg_out_cspace = nullptr;

  SK_ConfigSerializedKeybind
    preset_activate = {
      SK_Keybind {
        preset_name, L"",
          false, false, false, 'F1'
      }, L"Activate"
    };

  int activate (void)
  {
    __SK_HDR_Preset =
      preset_idx;

    __SK_HDR_Luma = peak_white_nits;
    __SK_HDR_Exp  = eotf;

    __SK_HDR_input_gamut  = colorspace.in;
    __SK_HDR_output_gamut = colorspace.out;

    if (_SK_HDR_ActivePreset != nullptr)
    {   _SK_HDR_ActivePreset->store (preset_idx);

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
        _CreateConfigParameterFloat ( SK_HDR_SECTION,
                   SK_FormatStringW (L"scRGBLuminance_[%lu]", preset_idx).c_str (),
                    peak_white_nits, L"scRGB Luminance" );
      cfg_eotf =
        _CreateConfigParameterFloat ( SK_HDR_SECTION,
                   SK_FormatStringW (L"scRGBGamma_[%lu]",     preset_idx).c_str (),
                               eotf, L"scRGB Gamma" );

      cfg_in_cspace =
        _CreateConfigParameterInt ( SK_HDR_SECTION,
                 SK_FormatStringW (L"InputColorSpace_[%lu]", preset_idx).c_str (),
                    colorspace.in, L"Input ColorSpace" );

      cfg_out_cspace =
        _CreateConfigParameterInt ( SK_HDR_SECTION,
                 SK_FormatStringW (L"OutputColorSpace_[%lu]", preset_idx).c_str (),
                   colorspace.out, L"Output ColorSpace" );

      wcsncpy_s ( preset_activate.short_name,                           32,
        SK_FormatStringW (L"Activate%lu", preset_idx).c_str (), _TRUNCATE );

      preset_activate.param =
        DeclKeybind (&preset_activate, SK_GetDLLConfig (), L"HDR.Presets");

      if (! preset_activate.param->load (preset_activate.human_readable))
      {
        preset_activate.human_readable =
          SK_FormatStringW (L"F%lu", preset_idx + 1); // F0 isn't a key :P
      }

      preset_activate.parse ();
      preset_activate.param->store (preset_activate.human_readable);

      SK_GetDLLConfig   ()->write (
        SK_GetDLLConfig ()->get_filename ()
                                  );
    }
  }
} static hdr_presets [4] = { { "HDR Preset 0", 0, 80.0_Nits, 1.0f, 0,0, L"F1" },
                             { "HDR Preset 1", 1, 80.0_Nits, 1.0f, 0,0, L"F2" },
                             { "HDR Preset 2", 2, 80.0_Nits, 1.0f, 0,0, L"F3" },
                             { "HDR Preset 3", 3, 80.0_Nits, 1.0f, 0,0, L"F4" } };

BOOL
CALLBACK
SK_HDR_KeyPress ( BOOL Control,
                  BOOL Shift,
                  BOOL Alt,
                  BYTE vkCode )
{
  #define SK_MakeKeyMask(vKey,ctrl,shift,alt)           \
    static_cast <UINT>((vKey) | (((ctrl) != 0) <<  9) | \
                                (((shift)!= 0) << 10) | \
                                (((alt)  != 0) << 11))

  for ( auto& it : hdr_presets )
  {
    if ( it.preset_activate.masked_code ==
           SK_MakeKeyMask ( vkCode,
                            Control != 0 ? 1 : 0,
                            Shift   != 0 ? 1 : 0,
                            Alt     != 0 ? 1 : 0 )
       )
    {
      it.activate ();

      return TRUE;
    }
  }

  return FALSE;
}

extern iSK_INI* osd_ini;

////SK_DXGI_HDRControl*
////SK_HDR_GetControl (void)
////{
////  static SK_DXGI_HDRControl hdr_ctl = { };
////  return                   &hdr_ctl;
////}

class SKWG_HDR_Control : public SK_Widget
{
public:
  SKWG_HDR_Control (void) noexcept : SK_Widget ("DXGI_HDR")
  {
    SK_ImGui_Widgets->hdr_control = this;

    setAutoFit (true).setDockingPoint (DockAnchor::NorthEast).setClickThrough (false);
  };

  void run (void) override
  {
    static bool first_widget_run = true;

    static auto& rb =
      SK_GetCurrentRenderBackend ();

    if ( __SK_HDR_10BitSwap ||
         __SK_HDR_16BitSwap )
    {
      if (__SK_HDR_16BitSwap)
      {
        rb.scanout.colorspace_override =
          DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709;
      }

      else
      {
        rb.scanout.colorspace_override =
          DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
      }

      SK_ComQIPtr <IDXGISwapChain3> pSwap3 (
        rb.swapchain.p
      );

      if (pSwap3 != nullptr)
      {
        SK_RunOnce (pSwap3->SetColorSpace1 (
          (DXGI_COLOR_SPACE_TYPE)
          rb.scanout.colorspace_override )
        );
      }
    }

    if (first_widget_run) first_widget_run = false;
    else return;

    hdr_presets [0].setup (); hdr_presets [1].setup ();
    hdr_presets [2].setup (); hdr_presets [3].setup ();

    _SK_HDR_10BitSwapChain =
      _CreateConfigParameterBool ( SK_HDR_SECTION,
                                  L"Use10BitSwapChain",  __SK_HDR_10BitSwap,
                                  L"10-bit SwapChain" );
    _SK_HDR_16BitSwapChain =
      _CreateConfigParameterBool ( SK_HDR_SECTION,
                                  L"Use16BitSwapChain",  __SK_HDR_16BitSwap,
                                  L"16-bit SwapChain" );

    _SK_HDR_FullRange =
      _CreateConfigParameterBool ( SK_HDR_SECTION,
                                  L"AllowFullLuminance",  __SK_HDR_FullRange,
                                  L"Slider will use full-range." );

    _SK_HDR_ActivePreset =
      _CreateConfigParameterInt ( SK_HDR_SECTION,
                                 L"Preset",               __SK_HDR_Preset,
                                 L"Light Adaptation Preset" );

    if (! config.render.framerate.flip_discard)
      __SK_HDR_16BitSwap = false;

    if ( __SK_HDR_Preset < 0 ||
         __SK_HDR_Preset >= MAX_HDR_PRESETS )
    {
      __SK_HDR_Preset = 0;
    }

    if (_SK_HDR_10BitSwapChain->load (__SK_HDR_10BitSwap))
    {
      if (__SK_HDR_10BitSwap)
      {
        rb.scanout.colorspace_override =
          DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
      }
    }

    else if (config.render.framerate.flip_discard && _SK_HDR_16BitSwapChain->load (__SK_HDR_16BitSwap))
    {
      if (__SK_HDR_16BitSwap)
      {
        //dll_log.Log (L"Test");

        rb.scanout.colorspace_override =
          DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709;
      }
    }

    else
    {
      __SK_HDR_16BitSwap = false;
       _SK_HDR_16BitSwapChain->store (__SK_HDR_16BitSwap);
    }

    if (__SK_HDR_16BitSwap ||
        __SK_HDR_10BitSwap)
    {
      hdr_presets [__SK_HDR_Preset].activate ();
    }
  }

  void draw (void) override
  {
    if (ImGui::GetFont () == nullptr)
      return;

    static auto& rb =
      SK_GetCurrentRenderBackend ();


    static SKTL_BidirectionalHashMap <int, std::wstring>
      __SK_HDR_ColorSpaceMap =
      {
        { 0, L"Unaltered" }, { 1, L"Rec709"  },
        { 2, L"DCI-P3"    }, { 3, L"Rec2020" },
        { 4, L"CIE_XYZ"   }
      };

    static const char* __SK_HDR_ColorSpaceComboStr =
      "Unaltered\0Rec. 709\0DCI-P3\0Rec. 2020\0CIE XYZ\0\0";


    if (! rb.isHDRCapable ())
      return;

    ImVec2 v2Min (
      ImGui::GetIO ().DisplaySize.x / 6.0f, ImGui::GetIO ().DisplaySize.y / 4.0f
    );

    ImVec2 v2Max (
      ImGui::GetIO ().DisplaySize.x / 4.0f, ImGui::GetIO ().DisplaySize.y / 3.0f
    );

    setMinSize (v2Min);
    setMaxSize (v2Max);

    ////SK_DXGI_HDRControl* pHDRCtl =
    ////  SK_HDR_GetControl ();
    ////
    ////bool sync_metadata = false;
    ////
    ////ImGui::Checkbox ("###HDR_Override_MinMasterLevel", &pHDRCtl->overrides.MinMaster); ImGui::SameLine ();
    ////float fMinMaster = (float)pHDRCtl->meta.MinMasteringLuminance / 10000.0f;
    ////if (ImGui::SliderFloat ("Minimum Luminance", &fMinMaster, pHDRCtl->devcaps.MinLuminance, pHDRCtl->devcaps.MaxLuminance))
    ////{
    ////  if (pHDRCtl->overrides.MinMaster)
    ////  {
    ////    sync_metadata = true;
    ////    pHDRCtl->meta.MinMasteringLuminance = (UINT)(fMinMaster * 10000);
    ////  }
    ////}
    ////
    ////ImGui::Checkbox ("###HDR_Override_MaxMasterLevel", &pHDRCtl->overrides.MaxMaster); ImGui::SameLine ();
    ////float fMaxMaster = (float)pHDRCtl->meta.MaxMasteringLuminance / 10000.0f;
    ////if (ImGui::SliderFloat ("Maximum Luminance", &fMaxMaster, pHDRCtl->devcaps.MinLuminance, pHDRCtl->devcaps.MaxLuminance))
    ////{
    ////  if (pHDRCtl->overrides.MaxMaster)
    ////  {
    ////    sync_metadata = true;
    ////    pHDRCtl->meta.MaxMasteringLuminance = (UINT)(fMaxMaster * 10000);
    ////  }
    ////}
    ////
    ////ImGui::Separator ();
    ////
    ////ImGui::Checkbox ("###HDR_Override_MaxContentLevel", &pHDRCtl->overrides.MaxContentLightLevel); ImGui::SameLine ();
    ////float fBrightest = (float)pHDRCtl->meta.MaxContentLightLevel;
    ////if (ImGui::SliderFloat ("Max. Content Light Level (nits)",       &fBrightest,          pHDRCtl->devcaps.MinLuminance, pHDRCtl->devcaps.MaxLuminance))
    ////{
    ////  if (pHDRCtl->overrides.MaxContentLightLevel)
    ////  {
    ////    sync_metadata = true;
    ////    pHDRCtl->meta.MaxContentLightLevel = (UINT16)fBrightest;
    ////  }
    ////}
    ////
    ////ImGui::Checkbox ("###HDR_Override_MaxFrameAverageLightLevel", &pHDRCtl->overrides.MaxFrameAverageLightLevel); ImGui::SameLine ();
    ////float fBrightestLastFrame = (float)pHDRCtl->meta.MaxFrameAverageLightLevel;
    ////if (ImGui::SliderFloat ("Max. Frame Average Light Level (nits)", &fBrightestLastFrame, pHDRCtl->devcaps.MinLuminance, pHDRCtl->devcaps.MaxLuminance))
    ////{
    ////  if (pHDRCtl->overrides.MaxFrameAverageLightLevel)
    ////  {
    ////    sync_metadata = true;
    ////    pHDRCtl->meta.MaxFrameAverageLightLevel = (UINT16)fBrightestLastFrame;
    ////  }
    ////}
    ////
    ////if (sync_metadata)
    ////{
    ////  CComQIPtr <IDXGISwapChain4> pSwapChain (rb.swapchain);
    ////
    ////  if (pSwapChain != nullptr)
    ////  {
    ////    pSwapChain->SetHDRMetaData (
    ////         DXGI_HDR_METADATA_TYPE_HDR10,
    ////      sizeof (DXGI_HDR_METADATA_HDR10),
    ////           nullptr
    ////    );
    ////  }
    ////}
    ////
    ////ImGui::TreePush   ("");
    ////ImGui::BulletText ("Game has adjusted HDR Metadata %lu times...", pHDRCtl->meta._AdjustmentCount);
    ////ImGui::TreePop    (  );

    ////bool bRetro =
    ////  true && ImGui::CollapsingHeader ("HDR Retrofit", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowOverlapMode);

    ///if ( rb.isHDRCapable () && (rb.framebuffer_flags & SK_FRAMEBUFFER_FLAG_HDR) &&
    ///     ImGui::CollapsingHeader ("HDR Overlays",  ImGuiTreeNodeFlags_DefaultOpen))
    ///{
    ///  ImGui::TreePush ("");
    ///  ImGui::TreePop  ();
    ///}

    static bool TenBitSwap_Original     = __SK_HDR_10BitSwap;
    static bool SixteenBitSwap_Original = __SK_HDR_16BitSwap;

    static int sel = __SK_HDR_16BitSwap ? 2 :
                     __SK_HDR_10BitSwap ? 1 : 0;

    if (! rb.isHDRCapable ())
    {
      if ( __SK_HDR_10BitSwap ||
           __SK_HDR_16BitSwap   )
      {
        SK_RunOnce (SK_ImGui_Warning (
          L"Please Restart the Game\n\n\t\tHDR Features were Enabled on a non-HDR Display!")
        );

        __SK_HDR_16BitSwap = false;
        __SK_HDR_10BitSwap = false;

        SK_RunOnce (_SK_HDR_16BitSwapChain->store (__SK_HDR_16BitSwap));
        SK_RunOnce (_SK_HDR_10BitSwapChain->store (__SK_HDR_10BitSwap));

        SK_RunOnce (dll_ini->write (dll_ini->get_filename ()));
      }

      //DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709
    }

    else if ( rb.isHDRCapable () && ImGui::CollapsingHeader ("HDR Adjust", ImGuiTreeNodeFlags_DefaultOpen) )
    {
      if (ImGui::RadioButton ("None", &sel, 0))
      {
        __SK_HDR_10BitSwap = false;
        __SK_HDR_16BitSwap = false;

        _SK_HDR_10BitSwapChain->store (__SK_HDR_10BitSwap);
        _SK_HDR_16BitSwapChain->store (__SK_HDR_16BitSwap);


        dll_ini->write (dll_ini->get_filename ());
      }

      ImGui::SameLine ();

      if (ImGui::RadioButton ("scRGB HDR (16-bit)", &sel, 2))
      {
        __SK_HDR_16BitSwap = true;

        if (__SK_HDR_16BitSwap) __SK_HDR_10BitSwap = false;

        _SK_HDR_10BitSwapChain->store (__SK_HDR_10BitSwap);
        _SK_HDR_16BitSwapChain->store (__SK_HDR_16BitSwap);

        if (__SK_HDR_16BitSwap)
        {
          config.window.borderless               = true;
          config.window.fullscreen               = true;
          config.render.framerate.flip_discard   = true;
          config.render.framerate.buffer_count   = 3;
          config.render.framerate.swapchain_wait = 66;
        }

        dll_ini->write (dll_ini->get_filename ());
      }
    }

    ////if (bRetro)
    ////{
    ////  ImGui::Separator (  );
    ////  ImGui::TreePush  ("");
    ////
    ////  ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.68f, 0.02f, 0.45f));
    ////  ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.72f, 0.07f, 0.80f));
    ////  ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.78f, 0.14f, 0.80f));
    ////  ImGui::PopStyleColor (3);
    ////  ImGui::TreePop       ( );
    ////}

    if (rb.isHDRCapable () && (rb.framebuffer_flags & SK_FRAMEBUFFER_FLAG_HDR))
    {
      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip ();
        {
          ImGui::TextUnformatted ("For best image quality, ensure your desktop is using 10-bit color");
          ImGui::Separator ();
          ImGui::BulletText ("Either RGB Full-Range or YCbCr-4:4:4 will work");
          ImGui::EndTooltip ();
        }
      }

      //ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.23f, 0.86f, 1.f));
      //ImGui::Text ("NOTE: ");
      //ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.4f, 0.28f, 0.94f)); ImGui::SameLine ();
      //ImGui::Text ("This is pure magic"); if (ImGui::IsItemHovered ()) ImGui::SetTooltip ("Essence of Pirate reasoning is required; harvesting takes time and quite possibly your sanity.");
      //ImGui::PopStyleColor  (2);

      ////if (ImGui::IsItemHovered ())
      ////{
      ////  ImGui::BeginTooltip ();
      ////  ImGui::Text         (" Future Work");
      ////  ImGui::Separator    ();
      ////  ImGui::BulletText   ("Automagic Dark and Light Adaptation are VERY Important and VERY Absent :P");
      ////  ImGui::BulletText   ("Separable HUD luminance (especially weapon cross-hairs) also must be implemented");
      ////  ImGui::EndTooltip   ();
      ////}
    }

    if ( ( TenBitSwap_Original     != __SK_HDR_10BitSwap ||
           SixteenBitSwap_Original != __SK_HDR_16BitSwap) )
    {
      ImGui::PushStyleColor (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (.3f, .8f, .9f));
      ImGui::BulletText     ("Game Restart Required");
      ImGui::PopStyleColor  ();
    }

    if ( __SK_HDR_10BitSwap ||
         __SK_HDR_16BitSwap    )
    {
      SK_ComQIPtr <IDXGISwapChain4> pSwap4 (rb.swapchain);

      if (pSwap4.p != nullptr)
      {
        DXGI_OUTPUT_DESC1     out_desc = { };
        DXGI_SWAP_CHAIN_DESC swap_desc = { };
           pSwap4->GetDesc (&swap_desc);

        if (out_desc.BitsPerColor == 0)
        {
          SK_ComPtr <IDXGIOutput>
            pOutput = nullptr;

          if ( SUCCEEDED (
                pSwap4->GetContainingOutput (&pOutput.p)
                         )
             )
          {
            SK_ComQIPtr <IDXGIOutput6> pOutput6 (pOutput);
                                       pOutput6->GetDesc1 (&out_desc);
          }

          else
          {
            out_desc.BitsPerColor = 8;
          }
        }

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
            { 0.70800f, 0.29200f, 0.17000f, 0.79700f, 0.13100f, 0.04600f, 0.31270f, 0.32900f }, // Display Gamut Rec2020
            { out_desc.RedPrimary   [0], out_desc.RedPrimary   [1],
              out_desc.GreenPrimary [0], out_desc.GreenPrimary [1],
              out_desc.BluePrimary  [0], out_desc.BluePrimary  [1],
              out_desc.WhitePoint   [0], out_desc.WhitePoint   [1] }
            };

          ImGui::TreePush ("");

          bool hdr_gamut_support (false);

          if (swap_desc.BufferDesc.Format == DXGI_FORMAT_R16G16B16A16_FLOAT)
          {
            hdr_gamut_support = true;
          }

          if ( swap_desc.BufferDesc.Format == DXGI_FORMAT_R10G10B10A2_UNORM ||
               rb.scanout.getEOTF ()       == SK_RenderBackend::scan_out_s::SMPTE_2084 )
          {
            hdr_gamut_support = true;
            ImGui::RadioButton ("Rec 709",  &cspace, 0); ImGui::SameLine ();
          }
          //else if (cspace == 0) cspace = 1;

          if ( swap_desc.BufferDesc.Format == DXGI_FORMAT_R10G10B10A2_UNORM ||
               rb.scanout.getEOTF ()       == SK_RenderBackend::scan_out_s::SMPTE_2084 )
          {
            hdr_gamut_support = true;
            ImGui::RadioButton ("Rec 2020", &cspace, 1); ImGui::SameLine ();
          }
          //else if (cspace == 1) cspace = 0;

          if ( swap_desc.BufferDesc.Format == DXGI_FORMAT_R10G10B10A2_UNORM ||
               rb.scanout.getEOTF ()       == SK_RenderBackend::scan_out_s::SMPTE_2084 )
          {
            ImGui::RadioButton ("Native",   &cspace, 2); ImGui::SameLine ();
          }

          if ( swap_desc.BufferDesc.Format == DXGI_FORMAT_R10G10B10A2_UNORM ||
               rb.scanout.getEOTF ()       == SK_RenderBackend::scan_out_s::SMPTE_2084 )// hdr_gamut_support)
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

              static float fLuma [4] = { out_desc.MaxLuminance,
                                         out_desc.MinLuminance,
                                         2000.0f,       600.0f };

              ImGui::InputFloat4 ("Luminance Coefficients", fLuma);// , 1);

              HDR10MetaData.MaxMasteringLuminance     = static_cast <UINT>   (fLuma [0] * 10000.0f);
              HDR10MetaData.MinMasteringLuminance     = static_cast <UINT>   (fLuma [1] * 10000.0f);
              HDR10MetaData.MaxContentLightLevel      = static_cast <UINT16> (fLuma [2]);
              HDR10MetaData.MaxFrameAverageLightLevel = static_cast <UINT16> (fLuma [3]);
          }

          ImGui::Separator ();

          ImGui::BeginGroup ();
          if (ImGui::Checkbox ("Enable FULL HDR Luminance", &__SK_HDR_FullRange))
          {
            _SK_HDR_FullRange->store (__SK_HDR_FullRange);
          }

          if (ImGui::IsItemHovered ())
            ImGui::SetTooltip ("Brighter values are possible with this on, but may clip white/black detail");

          ////ImGui::SameLine ();
          ////
          ////static bool open = false;
          ////
          ////open ^= ImGui::Button ("Edit Shaders");
          ////
          ////ImGui::Begin ( "Shader Editor",
          ////                 &open, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_ShowBorders );
          ////
          ////if (open)
          ////{
          ////  static char szBuf [16384 * 4] = { };
          ////
          ////  ImGui::InputTextMultiline  ("Shader Editor", szBuf, 16384 * 4 - 2);
          ////  ImGui::Button              ("Save");
          ////  ImGui::SameLine            ();
          ////  ImGui::Button              ("Reload");
          ////}
          ////
          ////ImGui::End ();

          ////ImGui::SameLine ();
          ////
          ////ImGui::SliderFloat ("###DQXI_UI_LUMINANCE", &__SK_DQXI_UI_Luma, -50.0f, 50.0f, "UI Luminance: %.3f");

          auto& pINI = dll_ini;

          float nits =
            __SK_HDR_Luma / 1.0_Nits;

          if (ImGui::SliderFloat ( "###SK_HDR_LUMINANCE", &nits, 80.0f, __SK_HDR_FullRange ?
                                                                          rb.display_gamut.maxLocalY :
                                                                          rb.display_gamut.maxY,
              u8"Peak White Luminance: %.1f cd/m²" ))
          {
            __SK_HDR_Luma = nits * 1.0_Nits;

            auto& preset =
              hdr_presets [__SK_HDR_Preset];

            preset.peak_white_nits =
              nits * 1.0_Nits;
            preset.cfg_nits->store (preset.peak_white_nits);

            pINI->write (pINI->get_filename ());
          }

          //ImGui::SameLine ();

          if (ImGui::SliderFloat ("###SK_HDR_GAMMA", &__SK_HDR_Exp, 1.0f, 2.4f, "SDR -> HDR Gamma: %.3f"))
          {
            auto& preset =
              hdr_presets [__SK_HDR_Preset];

            preset.eotf =
              __SK_HDR_Exp;
            preset.cfg_eotf->store (preset.eotf);

            pINI->write (pINI->get_filename ());
          }
          ImGui::EndGroup   ();
          ImGui::SameLine   ();
          ImGui::BeginGroup ();
          for ( int i = 0 ; i < MAX_HDR_PRESETS ; i++ )
          {
            bool selected =
              (__SK_HDR_Preset == i);

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
            ImGui::Text ( u8"Peak White: %5.1f cd/m²",
                          hdr_presets [i].peak_white_nits / 1.0_Nits );
          }
          ImGui::EndGroup   ();
          ImGui::SameLine   (); ImGui::Spacing ();
          ImGui::SameLine   (); ImGui::Spacing ();
          ImGui::SameLine   (); ImGui::Spacing (); ImGui::SameLine ();
          ImGui::BeginGroup ();
          for ( int i = 0 ; i < MAX_HDR_PRESETS ; i++ )
          {
            ImGui::Text ( u8"Power-Law ɣ: %3.1f",
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
          ////ImGui::Separator  ();
          ////
          ////static bool success = true;
          ////if (ImGui::Button ("Recompile HDR Shaders"))
          ////{
          ////  extern bool SK_HDR_RecompileShaders (void);
          ////  success =   SK_HDR_RecompileShaders (    );
          ////}
          ////
          ////if (! success) { ImGui::SameLine (); ImGui::TextUnformatted ("You dun screwed up!"); }

          ImGui::Separator ();

          extern int   __SK_HDR_visualization;
          extern float __SK_HDR_user_sdr_Y;

          ImGui::BeginGroup  ();
          ImGui::BeginGroup  ();

          auto& preset =
            hdr_presets [__SK_HDR_Preset];

          if ( ImGui::Combo ( "Source ColorSpace##SK_HDR_GAMUT_IN",
                                             &preset.colorspace.in,
                __SK_HDR_ColorSpaceComboStr)                        )
          {
            if (__SK_HDR_ColorSpaceMap.count (preset.colorspace.in))
            {
              __SK_HDR_input_gamut     =   preset.colorspace.in;
              preset.cfg_in_cspace->store (preset.colorspace.in);

              pINI->write (pINI->get_filename ());
            }

            else
            {
              preset.colorspace.in = __SK_HDR_input_gamut;
            }
          }

          if ( ImGui::Combo ( "Output ColorSpace##SK_HDR_GAMUT_OUT",
                                             &preset.colorspace.out,
                __SK_HDR_ColorSpaceComboStr )                       )
          {
            if (__SK_HDR_ColorSpaceMap.count (preset.colorspace.out))
            {
              __SK_HDR_output_gamut     =   preset.colorspace.out;
              preset.cfg_out_cspace->store (preset.colorspace.out);

              pINI->write (pINI->get_filename ());
            }

            else
            {
              preset.colorspace.out = __SK_HDR_output_gamut;
            }
          }
          ImGui::EndGroup    ();
          //ImGui::SameLine    ();
          ImGui::BeginGroup  ();
          ImGui::SliderFloat ("Horz. (%) HDR Processing", &__SK_HDR_HorizCoverage, 0.0f, 100.f);
          ImGui::SliderFloat ("Vert. (%) HDR Processing", &__SK_HDR_VertCoverage,  0.0f, 100.f);
          ImGui::EndGroup    ();

          ImGui::BeginGroup  ();
          ImGui::SliderFloat ("Non-Std. SDR Peak White", &__SK_HDR_user_sdr_Y, 80.0f, 400.0f, u8"%.3f cd/m²");

          if (ImGui::IsItemHovered ())
          {
            ImGui::BeginTooltip    (  );
            ImGui::TextUnformatted (u8"Technically 80.0 cd/m² is the Standard Peak White Luminance for sRGB");
            ImGui::Separator       (  );
            ImGui::BulletText      ("It is doubtful you are accustomed to an image that dim, so...");
            ImGui::BulletText      ("Supply your own expected luminance (i.e. a familiar display's rated peak brightness)");
            ImGui::Separator       (  );
            ImGui::TreePush        ("");
            ImGui::TextColored     (ImVec4 (1.f, 1.f, 1.f, 1.f),
                                    "This will assist during visualization and split-screen HDR/SDR comparisons");
            ImGui::TreePop         (  );
            ImGui::EndTooltip      (  );
          }
          //ImGui::SameLine    ();

          ImGui::Combo       ("HDR Visualization##SK_HDR_VIZ",  &__SK_HDR_visualization, "None\0SDR=Monochrome//HDR=FalseColors\0\0");
          ImGui::EndGroup    ();
          ImGui::EndGroup    ();

          ImGui::SameLine    ();

          ImGui::BeginGroup  ();
          SK_ImGui_DrawGamut ();
          ImGui::EndGroup    ();


          if ( swap_desc.BufferDesc.Format == DXGI_FORMAT_R10G10B10A2_UNORM ||
               rb.scanout.getEOTF ()       == SK_RenderBackend::scan_out_s::SMPTE_2084 )
          {
            ImGui::BulletText ("HDR May Not be Working Correctly Until you Engage Fullscreen Exclusive Mode...");

            ImGui::Separator ();
            if (ImGui::Button ("Inject HDR10 Metadata"))
            {
              //if (cspace == 2)
              //  swap_desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
              //else if (cspace == 1)
              //  swap_desc.BufferDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
              //else

              /////if (rb.scanout.bpc == 10)
              /////  swap_desc.BufferDesc.Format = DXGI_FORMAT_R10G10B10A2_UNORM;

              //pSwap4->SetHDRMetaData (DXGI_HDR_METADATA_TYPE_NONE, 0, nullptr);
              //
              //if (swap_desc.BufferDesc.Format == DXGI_FORMAT_R16G16B16A16_FLOAT)
              //{
              //  pSwap4->SetColorSpace1 (DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709);
              //}

              //else if (cspace == 0) pSwap4->SetColorSpace1 (DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709);
              //else                  pSwap4->SetColorSpace1 (DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709);

              pSwap4->SetHDRMetaData (DXGI_HDR_METADATA_TYPE_HDR10, sizeof (HDR10MetaData), &HDR10MetaData);
              pSwap4->SetColorSpace1 (DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020);
            }
          }

          /////else
          /////{
          /////  ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.075f, 1.0f, 1.0f));
          /////  ImGui::BulletText     ("A waitable swapchain is required for HDR10 (D3D11 Settings/SwapChain | {Flip Model + Waitable}");
          /////  ImGui::PopStyleColor  ();
          /////}

          ImGui::TreePop ();
        }
      }
    }
  }

  void OnConfig (ConfigEvent event) override
  {
    switch (event)
    {
      case SK_Widget::ConfigEvent::LoadComplete:
        break;

      case SK_Widget::ConfigEvent::SaveStart:
        break;
    }
  }

protected:

private:
};

SK_LazyGlobal <SKWG_HDR_Control> __dxgi_hdr__;
void SK_Widget_InitHDR (void)
{
  SK_RunOnce (__dxgi_hdr__.get ());
}




#include <SpecialK/../../depends/include/glm/glm.hpp>
#include <SpecialK/render/backend.h>

glm::highp_vec3
SK_Color_XYZ_from_RGB ( const SK_ColorSpace& cs, glm::highp_vec3 RGB )
{
  const double Xr =       cs.xr / cs.yr;
  const double Zr = (1. - cs.xr - cs.yr) / cs.yr;

  const double Xg =       cs.xg / cs.yg;
  const double Zg = (1. - cs.xg - cs.yg) / cs.yg;

  const double Xb =       cs.xb / cs.yb;
  const double Zb = (1. - cs.xb - cs.yb) / cs.yb;

  const double Yr = 1.;
  const double Yg = 1.;
  const double Yb = 1.;

  glm::highp_mat3x3 xyz_primary ( Xr, Xg, Xb,
                                  Yr, Yg, Yb,
                                  Zr, Zg, Zb );

  glm::highp_vec3 S       ( glm::inverse (xyz_primary) *
                              glm::highp_vec3 (cs.Xw, cs.Yw, cs.Zw) );

  return RGB *
    glm::highp_mat3x3 ( S.r * Xr, S.g * Xg, S.b * Xb,
                        S.r * Yr, S.g * Yg, S.b * Yb,
                        S.r * Zr, S.g * Zg, S.b * Zb );
}

glm::highp_vec3
SK_Color_xyY_from_RGB ( const SK_ColorSpace& cs, glm::highp_vec3 RGB )
{
  glm::highp_vec3 XYZ =
    SK_Color_XYZ_from_RGB ( cs, RGB );

  return
    glm::highp_vec3 ( XYZ.x / (XYZ.x + XYZ.y + XYZ.z),
                      XYZ.y / (XYZ.x + XYZ.y + XYZ.z),
                                       XYZ.y );
}

void
SK_ImGui_DrawGamut (void)
{
  const ImVec4 col (0.25f, 0.25f, 0.25f, 0.8f);

  const ImU32 col32 =
    ImColor (col);

  ImDrawList* draw_list =
    ImGui::GetWindowDrawList ();

  static const SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  struct color_triangle_s {
    std::string     name;
    glm::highp_vec3 r, g,
                    b, w;
    bool            show;

    double          _area = 0.0;

    color_triangle_s (const std::string& _name, SK_ColorSpace cs)
    {
      name = _name;

      r = SK_Color_xyY_from_RGB (cs, glm::highp_vec3 (1., 0., 0.));
      g = SK_Color_xyY_from_RGB (cs, glm::highp_vec3 (0., 1., 0.));
      b = SK_Color_xyY_from_RGB (cs, glm::highp_vec3 (0., 0., 1.));
      w = SK_Color_xyY_from_RGB (cs, glm::highp_vec3 (1., 1., 1.));

      double r_x = sqrt (r.x * r.x);  double r_y = sqrt (r.y * r.y);
      double g_x = sqrt (g.x * g.x);  double g_y = sqrt (g.y * g.y);
      double b_x = sqrt (b.x * b.x);  double b_y = sqrt (b.y * b.y);

      double A =
        fabs (r_x*(b_y-g_y) + g_x*(b_y-r_y) + b_x*(r_y-g_y));

      _area = A;

      show = true;
    }

    double area (void) const
    {
      return _area;
    }

    double coverage (const color_triangle_s& target) const
    {
      return
        100.0 * ( area () / target.area () );
    }
  };

#define D65 0.3127, 0.329

  static
    color_triangle_s
      color_spaces [] =
      {
        color_triangle_s ( "DCI-P3",        SK_ColorSpace { 0.68, 0.32,  0.265, 0.69,  0.15, 0.06,
                                                              D65, 1.0 - 0.3127 - 0.329 } ),

        color_triangle_s ( "ITU-R BT.709",  SK_ColorSpace { 0.64, 0.33,  0.3, 0.6,  0.15, 0.06,
                                                              D65, 1.0 } ),

        color_triangle_s ( "ITU-R BT.2020", SK_ColorSpace { 0.708, 0.292,  0.17, 0.797,  0.131, 0.046,
                                                              D65, 1.0 - 0.3127 - 0.329 } ),

        color_triangle_s ( "Adobe RGB",     SK_ColorSpace { 0.64, 0.33,  0.21, 0.71,  0.15, 0.06,
                                                              D65, 1.0 - 0.3127 - 0.329 } ),

        color_triangle_s { "NTSC",          SK_ColorSpace { 0.67, 0.33,  0.21, 0.71,  0.14, 0.08,
                                                              0.31, 0.316, 1.0 - 0.31 - 0.316 } }
      };

  static const
    glm::vec3 r (SK_Color_xyY_from_RGB (rb.display_gamut, glm::highp_vec3 (1.f, 0.f, 0.f))),
              g (SK_Color_xyY_from_RGB (rb.display_gamut, glm::highp_vec3 (0.f, 1.f, 0.f))),
              b (SK_Color_xyY_from_RGB (rb.display_gamut, glm::highp_vec3 (0.f, 0.f, 1.f)));

  static const color_triangle_s
    _NativeGamut ( "NativeGamut", rb.display_gamut );

  auto current_time =
    SK::ControlPanel::current_time;

  ImGui::Spacing    (); ImGui::SameLine ();
  ImGui::Spacing    (); ImGui::SameLine ();
  ImGui::Spacing    (); ImGui::SameLine ();

  ImGui::BeginGroup ();

  const ImColor self_outline_v4 =
    ImColor::HSV ( std::min (1.0f, 0.85f + (sin ((float)(current_time % 400) / 400.0f))),
                             0.0f,
                     (float)(0.66f + (current_time % 830) / 830.0f ) );

  const ImU32 self_outline =
    self_outline_v4;

  ImGui::TextColored (self_outline_v4, "%ws", rb.display_name);
  ImGui::Separator   ();

  float idx = 0.0f;
  ImGui::BeginGroup ();
  for ( auto& space : color_spaces )
  {
    ImGui::Text ("%5.2f%%", _NativeGamut.coverage (space));
  }
  ImGui::EndGroup   ();
  ImGui::SameLine   ();
  ImGui::BeginGroup ();
  for ( auto& space : color_spaces )
  {
    ImGui::PushID         ((int)idx);
    ImGui::PushStyleColor (ImGuiCol_Text, (ImVec4&&)ImColor::HSV ( idx / 5.0f, 0.85f, 0.98f ));
    ImGui::Checkbox       (space.name.c_str (), &space.show);
    ImGui::PopStyleColor  ();
    ImGui::PopID          ();

    idx += 1.0f;
  }
  ImGui::EndGroup ();

  ImGui::SameLine ();

  ImVec2 pos =
    ImGui::GetCursorScreenPos ();
  ImVec2 size =
    ImGui::GetContentRegionAvail ();

  float X0     = pos.x;
  float Y0     = pos.y;
  float width  = size.x;
  float height = size.y;

  draw_list->PushClipRect ( ImVec2 (X0,         Y0),
                            ImVec2 (X0 + width, Y0 + height) );

  draw_list->AddTriangleFilled ( ImVec2 (X0 + r.x * width, Y0 + (height - r.y * height)),
                                 ImVec2 (X0 + g.x * width, Y0 + (height - g.y * height)),
                                 ImVec2 (X0 + b.x * width, Y0 + (height - b.y * height)), col32 );
  idx = 0.0f;

  for ( auto& space : color_spaces )
  {
    if (space.show)
    {
      const ImU32 outline_color =
        ImColor::HSV ( idx / 5.0f, 0.85f, 0.98f );

      draw_list->AddTriangle ( ImVec2 (X0 + (float)space.r.x * width, Y0 + (height - (float)space.r.y * height)),
                               ImVec2 (X0 + (float)space.g.x * width, Y0 + (height - (float)space.g.y * height)),
                               ImVec2 (X0 + (float)space.b.x * width, Y0 + (height - (float)space.b.y * height)), outline_color, 1.05f );
    }

    idx += 1.0f;
  }

  draw_list->AddTriangle ( ImVec2 (X0 + (float)r.x * width, Y0 + (height - (float)r.y * height)),
                           ImVec2 (X0 + (float)g.x * width, Y0 + (height - (float)g.y * height)),
                           ImVec2 (X0 + (float)b.x * width, Y0 + (height - (float)b.y * height)), self_outline, 2.1f );

  draw_list->PopClipRect ();
         ImGui::EndGroup ();
}