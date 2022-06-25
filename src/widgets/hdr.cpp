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
#include <SpecialK/render/dxgi/dxgi_hdr.h>

#define SK_HDR_SECTION     L"SpecialK.HDR"
#define SK_MISC_SECTION    L"SpecialK.Misc"

extern iSK_INI*             dll_ini;
iSK_INI*                    hdr_ini = nullptr;


enum {
  SK_HDR_TONEMAP_NONE              = 0,
  SK_HDR_TONEMAP_FILMIC            = 1,
  SK_HDR_TONEMAP_HDR10_PASSTHROUGH = 2,
  SK_HDR_TONEMAP_HDR10_FILMIC      = 3,
};

static auto
DeclKeybind = [](SK_ConfigSerializedKeybind* binding, iSK_INI* ini, const wchar_t* sec) ->
auto
{
  auto* ret =
    dynamic_cast <sk::ParameterStringW *>
      (g_ParameterFactory->create_parameter <std::wstring> (L"DESCRIPTION HERE"));

  if (ret != nullptr)
  {
    ret->register_to_ini ( ini, sec, binding->short_name );
  }

  return ret;
};

static auto
_HDRKeybinding = [] ( SK_Keybind*           binding,
                      sk::ParameterStringW* param ) ->
auto
{
  if (param == nullptr)
    return false;

  ImGui::PushID (binding->bind_name);

  std::string label =
    SK_WideCharToUTF8 (binding->human_readable);

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


void SK_ImGui_DrawGamut (void);

sk::ParameterBool*  _SK_HDR_10BitSwapChain            = nullptr;
sk::ParameterBool*  _SK_HDR_16BitSwapChain            = nullptr;
sk::ParameterBool*  _SK_HDR_Promote10BitRGBATo16BitFP = nullptr;
sk::ParameterBool*  _SK_HDR_Promote11BitRGBTo16BitFP  = nullptr;
sk::ParameterBool*  _SK_HDR_Promote8BitRGBxTo16BitFP  = nullptr;
sk::ParameterInt*   _SK_HDR_ActivePreset              = nullptr;
sk::ParameterBool*  _SK_HDR_FullRange                 = nullptr;
sk::ParameterInt*   _SK_HDR_sRGBBypassBehavior        = nullptr;
sk::ParameterBool*  _SK_HDR_AdaptiveToneMap           = nullptr;

bool  __SK_HDR_10BitSwap        = false;
bool  __SK_HDR_16BitSwap        = false;

#include <SpecialK/render/dxgi/dxgi_hdr.h>

SK_LazyGlobal <SK_HDR_RenderTargetManager> SK_HDR_RenderTargets_8bpc;
SK_LazyGlobal <SK_HDR_RenderTargetManager> SK_HDR_RenderTargets_10bpc;
SK_LazyGlobal <SK_HDR_RenderTargetManager> SK_HDR_RenderTargets_11bpc;

float __SK_HDR_Luma            = 80.0_Nits;
float __SK_HDR_Exp             = 1.0f;
float __SK_HDR_Saturation      = 1.0f;
extern float                   
      __SK_HDR_user_sdr_Y;     
float __SK_HDR_MiddleLuma      = 100.0_Nits;
int   __SK_HDR_Preset          = 0;
bool  __SK_HDR_FullRange       = true;
                               
float __SK_HDR_UI_Luma         = 1.0f;
float __SK_HDR_HorizCoverage   = 100.0f;
float __SK_HDR_VertCoverage    = 100.0f;
                               
float __SK_HDR_PQBoost0        = -7.0f;
float __SK_HDR_PQBoost1        =  9.0f;
float __SK_HDR_PQBoost2        =  1.1333f;
float __SK_HDR_PQBoost3        =  0.666f;
bool  __SK_HDR_AdaptiveToneMap =  false;



#define MAX_HDR_PRESETS 4

struct SK_HDR_Preset_s {
  const char*  preset_name = "uninitialized";
  int          preset_idx  = 0;

  float        peak_white_nits  = 0.0f;
  float        paper_white_nits = 0.0f;
  float        middle_gray_nits = 100.0_Nits;
  float        eotf             = 0.0f;
  float        saturation       = 1.0f;

  struct {
    int tonemap = SK_HDR_TONEMAP_FILMIC;
  } colorspace;

  float        pq_boost0       = __SK_HDR_PQBoost0;
  float        pq_boost1       = __SK_HDR_PQBoost1;
  float        pq_boost2       = __SK_HDR_PQBoost2;
  float        pq_boost3       = __SK_HDR_PQBoost3;

  std::wstring annotation = L"";

  sk::ParameterFloat*   cfg_nits         = nullptr;
  sk::ParameterFloat*   cfg_paperwhite   = nullptr;
  sk::ParameterFloat*   cfg_eotf         = nullptr;
  sk::ParameterFloat*   cfg_saturation   = nullptr;
  sk::ParameterFloat*   cfg_middlegray   = nullptr;
  sk::ParameterInt*     cfg_tonemap      = nullptr;

  sk::ParameterFloat*   cfg_pq_boost0    = nullptr;
  sk::ParameterFloat*   cfg_pq_boost1    = nullptr;
  sk::ParameterFloat*   cfg_pq_boost2    = nullptr;
  sk::ParameterFloat*   cfg_pq_boost3    = nullptr;

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

    __SK_HDR_Luma          = peak_white_nits;
    __SK_HDR_PaperWhite    = paper_white_nits;
    __SK_HDR_MiddleLuma    = middle_gray_nits / 1.0_Nits;
    __SK_HDR_user_sdr_Y    = middle_gray_nits / 1.0_Nits;
    __SK_HDR_Exp           = eotf;
    __SK_HDR_Saturation    = saturation;
    __SK_HDR_tonemap       = colorspace.tonemap;
    __SK_HDR_PQBoost0      = pq_boost0;
    __SK_HDR_PQBoost1      = pq_boost1;
    __SK_HDR_PQBoost2      = pq_boost2;
    __SK_HDR_PQBoost3      = pq_boost3;

    if (_SK_HDR_ActivePreset != nullptr)
    {   _SK_HDR_ActivePreset->store (preset_idx);

      store ();
    }

    return preset_idx;
  }

  void store (void)
  {
    for (                        sk::iParameter *pParam :
          std::initializer_list <sk::iParameter *>
          {
            cfg_nits,       cfg_paperwhite, cfg_eotf,
            cfg_saturation, cfg_middlegray, cfg_tonemap,
            cfg_pq_boost0,  cfg_pq_boost1,
            cfg_pq_boost2,  cfg_pq_boost3
          }
        )
    {
      if (pParam != nullptr)
          pParam->store ();
    }

    SK_GetDLLConfig ()->write ();
  }

  void setup (void)
  {
    if (cfg_nits == nullptr)
    {
      cfg_nits =
        _CreateConfigParameterFloat ( SK_HDR_SECTION,
                   SK_FormatStringW (L"scRGBLuminance_[%lu]", preset_idx).c_str (),
                    peak_white_nits, L"scRGB Luminance" );

      cfg_paperwhite =
        _CreateConfigParameterFloat ( SK_HDR_SECTION,
                   SK_FormatStringW (L"scRGBPaperWhite_[%lu]", preset_idx).c_str (),
                   paper_white_nits, L"scRGB Paper White" );
      cfg_eotf =
        _CreateConfigParameterFloat ( SK_HDR_SECTION,
                   SK_FormatStringW (L"scRGBGamma_[%lu]",     preset_idx).c_str (),
                               eotf, L"scRGB Gamma" );

      cfg_tonemap =
        _CreateConfigParameterInt ( SK_HDR_SECTION,
                 SK_FormatStringW (L"ToneMapper_[%lu]", preset_idx).c_str (),
               colorspace.tonemap, L"ToneMap Mode" );

      cfg_saturation =
        _CreateConfigParameterFloat ( SK_HDR_SECTION,
                 SK_FormatStringW (L"Saturation_[%lu]", preset_idx).c_str (),
                   saturation,     L"Saturation Coefficient" );

      cfg_middlegray =
        _CreateConfigParameterFloat ( SK_HDR_SECTION,
                 SK_FormatStringW   (L"MiddleGray_[%lu]", preset_idx).c_str (),
                   middle_gray_nits, L"Middle Gray Luminance" );

      cfg_pq_boost0 =
        _CreateConfigParameterFloat ( SK_HDR_SECTION,
                 SK_FormatStringW   (L"PerceptualBoost0_[%lu]", preset_idx).c_str (),
                        pq_boost0,   L"PerceptualBoost Param #0" );

      cfg_pq_boost1 =
        _CreateConfigParameterFloat ( SK_HDR_SECTION,
                 SK_FormatStringW   (L"PerceptualBoost1_[%lu]", preset_idx).c_str (),
                        pq_boost1,   L"PerceptualBoost Param #1" );

      cfg_pq_boost2 =
        _CreateConfigParameterFloat ( SK_HDR_SECTION,
                 SK_FormatStringW   (L"PerceptualBoost2_[%lu]", preset_idx).c_str (),
                        pq_boost2,   L"PerceptualBoost Param #2" );

      cfg_pq_boost3 =
        _CreateConfigParameterFloat ( SK_HDR_SECTION,
                 SK_FormatStringW   (L"PerceptualBoost3_[%lu]", preset_idx).c_str (),
                        pq_boost3,   L"PerceptualBoost Param #3" );

      wcsncpy_s ( preset_activate.short_name,                         32,
        SK_FormatStringW (L"Activate%lu", preset_idx).c_str (), _TRUNCATE );

      preset_activate.param =
        DeclKeybind (&preset_activate, SK_GetDLLConfig (), L"HDR.Presets");

      if (! preset_activate.param->load (preset_activate.human_readable))
      {
        preset_activate.human_readable =
          SK_FormatStringW (L"Shift+F%lu", preset_idx + 1); // F0 isn't a key :P
      }

      preset_activate.parse ();
      preset_activate.param->store (preset_activate.human_readable);

      store ();
    }
  }
} static hdr_presets  [4] = { { "HDR Preset 0", 0,   89.7_Nits,  85.8_Nits, 101.33296_Nits, 1.0f, 1.033f, { SK_HDR_TONEMAP_NONE   }, -__SK_HDR_PQBoost0, __SK_HDR_PQBoost1, __SK_HDR_PQBoost2, __SK_HDR_PQBoost3, L"Shift+F1" },
                              { "HDR Preset 1", 1,  200.0_Nits, 100.0_Nits,     100.0_Nits, 1.0f,   1.0f, { SK_HDR_TONEMAP_NONE   },  __SK_HDR_PQBoost0, __SK_HDR_PQBoost1, __SK_HDR_PQBoost2, __SK_HDR_PQBoost3, L"Shift+F2" },
                              { "HDR Preset 2", 2,   80.0_Nits,  80.0_Nits,     100.0_Nits, 1.0f,   1.0f, { SK_HDR_TONEMAP_NONE   },  __SK_HDR_PQBoost0, __SK_HDR_PQBoost1, __SK_HDR_PQBoost2, __SK_HDR_PQBoost3, L"Shift+F3" },
                              { "HDR Preset 3", 3,  300.0_Nits, 150.0_Nits,     100.0_Nits, 1.0f,   1.0f, { SK_HDR_TONEMAP_FILMIC },  __SK_HDR_PQBoost0, __SK_HDR_PQBoost1, __SK_HDR_PQBoost2, __SK_HDR_PQBoost3, L"Shift+F4" } },
         hdr_defaults [4] = { { "HDR Preset 0", 0,   89.7_Nits,  85.8_Nits, 101.33296_Nits, 1.0f, 1.033f, { SK_HDR_TONEMAP_NONE   }, -__SK_HDR_PQBoost0, __SK_HDR_PQBoost1, __SK_HDR_PQBoost2, __SK_HDR_PQBoost3, L"Shift+F1" },
                              { "HDR Preset 1", 1,  200.0_Nits, 100.0_Nits,     100.0_Nits, 1.0f,   1.0f, { SK_HDR_TONEMAP_NONE   },  __SK_HDR_PQBoost0, __SK_HDR_PQBoost1, __SK_HDR_PQBoost2, __SK_HDR_PQBoost3, L"Shift+F2" },
                              { "HDR Preset 2", 2,   80.0_Nits,  80.0_Nits,     100.0_Nits, 1.0f,   1.0f, { SK_HDR_TONEMAP_NONE   },  __SK_HDR_PQBoost0, __SK_HDR_PQBoost1, __SK_HDR_PQBoost2, __SK_HDR_PQBoost3, L"Shift+F3" },
                              { "HDR Preset 3", 3,  300.0_Nits, 150.0_Nits,     100.0_Nits, 1.0f,   1.0f, { SK_HDR_TONEMAP_FILMIC },  __SK_HDR_PQBoost0, __SK_HDR_PQBoost1, __SK_HDR_PQBoost2, __SK_HDR_PQBoost3, L"Shift+F4" } };

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

void
SK_HDR_DisplayProfilerDialog (bool draw = true)
{
  static auto pINI =
      SK_CreateINI (
        (std::wstring (SK_GetInstallPath ()) + LR"(\Global\hdr.ini)").c_str ()
      );

  static auto& rb =
    SK_GetCurrentRenderBackend ();
  
  if (! draw)
  {
    static std::wstring
          last_path = L"";
    if (! last_path._Equal (rb.displays [rb.active_display].path_name))
    {
      if (pINI->contains_section (rb.displays [rb.active_display].path_name))
      {
        auto& sec =
          pINI->get_section (rb.displays [rb.active_display].path_name);

        if (sec.contains_key (L"MaxLuminance"))
        {
          rb.display_gamut.maxLocalY =
            std::max (80.0f, static_cast <float> (_wtof (sec.get_cvalue (L"MaxLuminance").c_str ())));
          rb.display_gamut.maxY      =
            rb.display_gamut.maxLocalY;

          if (rb.display_gamut.maxAverageY > rb.display_gamut.maxY / 2)
              rb.display_gamut.maxAverageY = rb.display_gamut.maxY / 2;

          last_path = rb.displays [rb.active_display].path_name;
        }
      }
    }

    return;
  }

  const char* SK_ImGui_ControlPanelTitle (void);

  static float fOrigHDRLuma  = __SK_HDR_Luma;
  static int   iOrigVisual   = __SK_HDR_visualization;
  static bool  bLastOpen     = false;
  static bool  disable_alpha = config.imgui.render.disable_alpha;

  //
  // We have to turn Adaptive Tonemapping Off while displaying the
  //   test patterns; otherwise clipping would never occur.
  //
  static bool  bOrigAdaptive =
           __SK_HDR_AdaptiveToneMap;

  bool bOpen =
    ImGui::IsPopupOpen ("HDR Display Profiler");

  if (bOpen)
  {
    auto pHDRWidget          =
      ImGui::FindWindowByName ("DXGI_HDR##Widget_DXGI_HDR");
    auto pControlPanelWindow =
      ImGui::FindWindowByName (SK_ImGui_ControlPanelTitle ());

    static ImVec2 vDialogPos =
           ImVec2 (0.0f, 0.0f);

    if (! bLastOpen)
    {
      bOrigAdaptive =
        std::exchange (__SK_HDR_AdaptiveToneMap, false);
      fOrigHDRLuma  =
        std::exchange (__SK_HDR_Luma, rb.display_gamut.maxLocalY * 1.0_Nits);
      iOrigVisual   =
        std::exchange (__SK_HDR_visualization, 13);
      disable_alpha =
        std::exchange (config.imgui.render.disable_alpha, true);

      vDialogPos =
        ImGui::GetCursorScreenPos ();
    }

    ImGui::SetNextWindowPos (vDialogPos, ImGuiCond_Always);

    if (pControlPanelWindow != nullptr) pControlPanelWindow->Hidden = true;
    if (         pHDRWidget != nullptr)          pHDRWidget->Hidden = true;
  }

  else
  {
    if (bLastOpen)
    { 
      auto pHDRWidget          =
        ImGui::FindWindowByName ("DXGI_HDR##Widget_DXGI_HDR");
      auto pControlPanelWindow =
        ImGui::FindWindowByName (SK_ImGui_ControlPanelTitle ());

      if (pControlPanelWindow != nullptr) pControlPanelWindow->Hidden = false;
      if (         pHDRWidget != nullptr)          pHDRWidget->Hidden = false;

      config.imgui.render.disable_alpha =
                          disable_alpha;

      __SK_HDR_Luma            = std::min (rb.display_gamut.maxLocalY * 1.0_Nits, fOrigHDRLuma);
      __SK_HDR_visualization   = iOrigVisual;
      __SK_HDR_AdaptiveToneMap = bOrigAdaptive;
    }

    else
    {
      bOrigAdaptive = __SK_HDR_AdaptiveToneMap;
      iOrigVisual   = __SK_HDR_visualization;
      fOrigHDRLuma  = __SK_HDR_Luma;
      disable_alpha = config.imgui.render.disable_alpha;
    }
  }

  bLastOpen = bOpen;

  ImGui::PushStyleVar (ImGuiStyleVar_Alpha, 1.0f);

  if (ImGui::BeginPopup ("HDR Display Profiler", ImGuiWindowFlags_AlwaysAutoResize |
                                                 ImGuiWindowFlags_NoCollapse       | ImGuiWindowFlags_NoSavedSettings))
  {
    float peak_nits =
      std::min (10000.0f,
        std::max (80.0f, __SK_HDR_Luma / 1.0_Nits)
               );

    if ( ImGui::InputFloat (
           "Luminance Clipping Point (cd/m²)###SK_HDR_LUMINANCE",
             &peak_nits, 1.0f, 10.0f, 2, ImGuiInputTextFlags_CharsDecimal )
       )
    {
      __SK_HDR_Luma =
        std::min (10000.0_Nits,
          std::max (80.0_Nits, peak_nits * 1.0_Nits)
                 );
    }

    if (ImGui::IsItemHovered ())
    {
      ImGui::BeginTooltip ();
      ImGui::Text         ("For Best Results");
      ImGui::Separator    ();
      ImGui::BulletText   ("Enter exact values until you find the smallest "
                           "value that makes the pattern invisible.");
      ImGui::BulletText   ("Consider saving the clipping point as 2/3 (LCD) "
                           "or 1/2 (OLED) the value found above to minimize "
                           "ABL and Local Dimming problems.");
      ImGui::EndTooltip   ();
    }

    int idx =
      std::min (
        std::max (12, __SK_HDR_visualization) - 12,
             2 );

    if (
      ImGui::Combo ("Test Pattern##SK_HDR_PATTERN", &idx, "HDR Luminance Clip v0\0"
                                                          "HDR Luminance Clip v1\0"
                                                          "HDR Luminance Clip v2\0\0")
       )
    {
      __SK_HDR_visualization =
                         idx + 12;
    }

    ImGui::Separator ();

    bool bEnd =
      ImGui::Button ("Okay");
    if (ImGui::IsItemHovered ())
    {
      ImGui::BeginTooltip ();
      ImGui::Text         ("Remember to re-calibrate SK HDR presets using the 'Tonemap Curve and Grayscale' HDR Visualization");
      ImGui::Separator    ();
      ImGui::BulletText   ("User-calibration should focus on the Max Luminance / Paper White / Middle-Gray Contrast");
      ImGui::BulletText   ("A correctly configured HDR preset will show up as a curve from bottom-left to top-right");
      ImGui::EndTooltip   ();
    }
    bool bSave =
         bEnd;
    bool bReset = false;
    ImGui::SameLine ();
    bEnd |=
      ImGui::Button ("Cancel");

    if ( pINI->get_section (rb.displays [rb.active_display].path_name).
          contains_key     (L"MaxLuminance") )
    {
      ImGui::SameLine ();
      bReset =
        ImGui::Button ("Reset");
      bEnd |= bReset;
    }

    if (bEnd)
    {
      if (bSave || bReset)
      {
        if (! bReset)
        {
          pINI->get_section (rb.displays [rb.active_display].path_name).
              add_key_value (L"MaxLuminance", std::to_wstring (__SK_HDR_Luma / 1.0_Nits));
        }

        else
        {
          pINI->get_section (rb.displays [rb.active_display].path_name).
                 remove_key (L"MaxLuminance");

          __SK_HDR_Luma = 1499.0_Nits;
        }

        pINI->write ();

        rb.display_gamut.maxLocalY   = __SK_HDR_Luma / 1.0_Nits;
        rb.display_gamut.maxAverageY = // TODO: Actual Max Average
          std::min ( rb.display_gamut.maxAverageY,
                     rb.display_gamut.maxLocalY );
      }

      ImGui::CloseCurrentPopup ();
    }

    ImGui::EndPopup ();
  }
  ImGui::PopStyleVar ();
}


#include <SpecialK/render/d3d11/d3d11_core.h>

extern iSK_INI* osd_ini;

class SKWG_HDR_Control : public SK_Widget, SK_IVariableListener
{
public:
  SKWG_HDR_Control (void) : SK_Widget ("DXGI_HDR")
  {
    SK_ImGui_Widgets->hdr_control = this;

    setAutoFit (true).setDockingPoint (DockAnchor::NorthEast).setClickThrough (false).
                      setBorder (true);

    const gsl::not_null <
        SK_ICommandProcessor*
      >    pCommandProc =
      SK_GetCommandProcessor ();

    try {
      SK_IVarStub <int>*   preset      = new SK_IVarStub <int>   (&__SK_HDR_Preset, this);
      SK_IVarStub <int>*   tonemap     = new SK_IVarStub <int>   (&__SK_HDR_tonemap);
      SK_IVarStub <int>*   vis         = new SK_IVarStub <int>   (&__SK_HDR_visualization);
      SK_IVarStub <float>* horz        = new SK_IVarStub <float> (&__SK_HDR_HorizCoverage);
      SK_IVarStub <float>* vert        = new SK_IVarStub <float> (&__SK_HDR_VertCoverage);
      SK_IVarStub <bool>*  adaptive    = new SK_IVarStub <bool>  (&__SK_HDR_AdaptiveToneMap);
      SK_IVarStub <int>*   bypass_srgb = new SK_IVarStub <int>   (&__SK_HDR_Bypass_sRGB);

      pCommandProc->AddVariable ( "HDR.Preset",          &preset->setRange      (0, 3)         );
      pCommandProc->AddVariable ( "HDR.Visualization",   &vis->setRange         (0, 11)        );
      pCommandProc->AddVariable ( "HDR.Tonemap",         &tonemap->setRange     (0, 2)         );
      pCommandProc->AddVariable ( "HDR.HorizontalSplit", &horz->setRange        (0.0f, 100.0f) );
      pCommandProc->AddVariable ( "HDR.VerticalSplit",   &vert->setRange        (0.0f, 100.0f) );
      pCommandProc->AddVariable ( "HDR.AdaptiveToneMap", &adaptive->setRange    (false,  true) );
      pCommandProc->AddVariable ( "HDR.Bypass_sRGB",     &bypass_srgb->setRange (   -1,     1) );
    }

    catch (...)
    {

    }
  };

  bool OnVarChange (SK_IVariable* var, void* val = nullptr) override
  {
    if ( val != nullptr &&
         var != nullptr && var->getValuePointer () == &__SK_HDR_Preset )
    {
      __SK_HDR_Preset = *(int *)val;

      hdr_presets [__SK_HDR_Preset].activate ();
    }

    return true;
  }

  void run (void) override
  {
    static bool first_widget_run = true;

    static auto& rb =
      SK_GetCurrentRenderBackend ();


    // Automatically handle sRGB -> Linear if the original SwapChain used it
    extern bool             bOriginallysRGB;
    if (rb.srgb_stripped || bOriginallysRGB)
      __SK_HDR_Bypass_sRGB = 1;


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

      SK_ComQIPtr <IDXGISwapChain3> pSwap3 (rb.swapchain);

      if (pSwap3 != nullptr)
      {
        SK_RunOnce (pSwap3->SetColorSpace1 (
          (DXGI_COLOR_SPACE_TYPE)
          rb.scanout.colorspace_override )
        );
      }

      SK_HDR_DisplayProfilerDialog (false);
    }

    if (first_widget_run) first_widget_run = false;
    else return;

    _SK_HDR_10BitSwapChain =
      _CreateConfigParameterBool ( SK_HDR_SECTION,
                                  L"Use10BitSwapChain",  __SK_HDR_10BitSwap,
                                  L"10-bit SwapChain" );
    _SK_HDR_16BitSwapChain =
      _CreateConfigParameterBool ( SK_HDR_SECTION,
                                  L"Use16BitSwapChain",  __SK_HDR_16BitSwap,
                                  L"16-bit SwapChain" );


    _SK_HDR_Promote8BitRGBxTo16BitFP =
      _CreateConfigParameterBool ( SK_HDR_SECTION,
                                   L"Promote8BitRTsTo16",  SK_HDR_RenderTargets_8bpc->PromoteTo16Bit,
                                   L"8-Bit Precision Increase" );

    _SK_HDR_Promote10BitRGBATo16BitFP =
      _CreateConfigParameterBool ( SK_HDR_SECTION,
                                   L"Promote10BitRTsTo16",  SK_HDR_RenderTargets_10bpc->PromoteTo16Bit,
                                   L"10-Bit Precision Increase" );
    _SK_HDR_Promote11BitRGBTo16BitFP =
      _CreateConfigParameterBool ( SK_HDR_SECTION,
                                   L"Promote11BitRTsTo16",  SK_HDR_RenderTargets_11bpc->PromoteTo16Bit,
                                   L"11-Bit Precision Increase" );

    _SK_HDR_FullRange =
      _CreateConfigParameterBool ( SK_HDR_SECTION,
                                   L"AllowFullLuminance",  __SK_HDR_FullRange,
                                   L"Slider will use full-range." );

    _SK_HDR_sRGBBypassBehavior =
      _CreateConfigParameterInt ( SK_HDR_SECTION,
                                  L"sRGBBypassMode", __SK_HDR_Bypass_sRGB,
                                  L"sRGB Encoding is Implicit" );

    _SK_HDR_AdaptiveToneMap =
      _CreateConfigParameterBool ( SK_HDR_SECTION,
                                   L"AdaptiveToneMap", __SK_HDR_AdaptiveToneMap,
                                   L"HGIG Rules Apply" );

    _SK_HDR_ActivePreset =
      _CreateConfigParameterInt ( SK_HDR_SECTION,
                                  L"Preset",               __SK_HDR_Preset,
                                  L"Light Adaptation Preset" );

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

    else if (_SK_HDR_16BitSwapChain->load (__SK_HDR_16BitSwap))
    {
      if (__SK_HDR_16BitSwap)
      {
        rb.scanout.colorspace_override =
          DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709;
      }
    }

    hdr_presets [0].setup (); hdr_presets [1].setup ();
    hdr_presets [2].setup (); hdr_presets [3].setup ();

    hdr_presets [__SK_HDR_Preset].activate ();
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
        { 0, L"      Passthrough (SDR -> HDR or scRGB) " }, { 1, L"      ACES Filmic (SDR -> HDR)"       },
        { 2, L"HDR10 Passthrough (Native HDR)"           }
      };

    static const char* __SK_HDR_ColorSpaceComboStr =
      (const char *)u8"      Passthrough (SDR -> HDR or scRGB) \0      ACES Filmic (SDR -> HDR)\0"
                    u8"HDR10 Passthrough (Native HDR)\0\0";

    // If override is not enabled and display is not HDR capable, then do nothing.
    if ((! rb.isHDRCapable ()) && (! __SK_HDR_16BitSwap))
      return;

    const auto& io =
      ImGui::GetIO ();

    const ImVec2 v2Min (
      io.DisplaySize.x / 6.0f,
      io.DisplaySize.y / 4.0f
    );

    const ImVec2 v2Max (
      io.DisplaySize.x / 4.0f,
      io.DisplaySize.y / 3.0f
    );

    setMinSize (v2Min);
    setMaxSize (v2Max);

    static bool TenBitSwap_Original     = __SK_HDR_10BitSwap;
    static bool SixteenBitSwap_Original = __SK_HDR_16BitSwap;

    static int sel = __SK_HDR_16BitSwap ? 2 :
                     __SK_HDR_10BitSwap ? 1 : 0;

    SK_HDR_DisplayProfilerDialog ();

    //if (! rb.isHDRCapable ())
    //{
    //  if ( __SK_HDR_10BitSwap ||
    //       __SK_HDR_16BitSwap   )
    //  {
    //    SK_RunOnce (SK_ImGui_Warning (
    //      L"Please Restart the Game\n\n\t\tHDR Features were Enabled on a non-HDR Display!")
    //    );
    //
    //    __SK_HDR_16BitSwap = false;
    //    __SK_HDR_10BitSwap = false;
    //
    //    SK_HDR_RenderTargets_10bpc->PromoteTo16Bit = true;
    //    SK_HDR_RenderTargets_11bpc->PromoteTo16Bit = true;
    //    SK_HDR_RenderTargets_8bpc->PromoteTo16Bit  = false;
    //
    //    SK_RunOnce (_SK_HDR_16BitSwapChain->store            (__SK_HDR_16BitSwap));
    //    SK_RunOnce (_SK_HDR_10BitSwapChain->store            (__SK_HDR_10BitSwap));
    //    SK_RunOnce (_SK_HDR_Promote8BitRGBxTo16BitFP->store  (SK_HDR_RenderTargets_8bpc->PromoteTo16Bit));
    //    SK_RunOnce (_SK_HDR_Promote10BitRGBATo16BitFP->store (SK_HDR_RenderTargets_10bpc->PromoteTo16Bit));
    //    SK_RunOnce (_SK_HDR_Promote11BitRGBTo16BitFP->store  (SK_HDR_RenderTargets_11bpc->PromoteTo16Bit));
    //
    //    SK_RunOnce (dll_ini->write (dll_ini->get_filename ()));
    //  }
    //}
    //
    /*else */if ( /*rb.isHDRCapable () &&*/
                ImGui::CollapsingHeader (
                  "HDR Calibration###SK_HDR_CfgHeader",
                                     ImGuiTreeNodeFlags_DefaultOpen |
                                     ImGuiTreeNodeFlags_Leaf        |
                                     ImGuiTreeNodeFlags_Bullet
                                        )
            )
    {
      bool changed = false;

      if (ImGui::RadioButton ("None###SK_HDR_NONE", &sel, 0))
      {
        changed = true;

        __SK_HDR_10BitSwap = false;
        __SK_HDR_16BitSwap = false;

        _SK_HDR_10BitSwapChain->store (__SK_HDR_10BitSwap);
        _SK_HDR_16BitSwapChain->store (__SK_HDR_16BitSwap);

      }

      ImGui::SameLine ();

      if (ImGui::RadioButton ("scRGB HDR###SK_HDR_scRGB", &sel, 2))
      {
        changed = true;

        __SK_HDR_16BitSwap = true;
        __SK_HDR_10BitSwap = false;

        _SK_HDR_10BitSwapChain->store (__SK_HDR_10BitSwap);
        _SK_HDR_16BitSwapChain->store (__SK_HDR_16BitSwap);

        // D3D12 is already Flip Model, so we're golden (!!)
        if (rb.api != SK_RenderAPI::D3D12)
        {
          config.render.framerate.flip_discard = true;

          //// Should be able to handle this without an explicit override set now
          //config.render.framerate.buffer_count =
          //  std::max ( 2,
          //               config.render.framerate.buffer_count );
        }
      }

      if (changed)
      {
        dll_ini->write ();

        SK_ComQIPtr <IDXGISwapChain3> pSwapChain (rb.swapchain);

        if (pSwapChain)
        {
          rb.scanout.colorspace_override = __SK_HDR_16BitSwap ? DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709
                                                              : DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;//DXGI_COLOR_SPACE_CUSTOM

          // Trigger the game to resize the SwapChain so we can change its format and colorspace
          //
          PostMessage ( game_window.hWnd,                 WM_SIZE,        SIZE_RESTORED,
            MAKELPARAM (game_window.actual.client.right -
                        game_window.actual.client.left,   game_window.actual.client.bottom -
                                                          game_window.actual.client.top )
                      );
          PostMessage ( game_window.hWnd,                 WM_DISPLAYCHANGE, 32,
            MAKELPARAM (game_window.actual.client.right -
                        game_window.actual.client.left,   game_window.actual.client.bottom -
                                                          game_window.actual.client.top )
                      );

          pSwapChain->SetColorSpace1 (__SK_HDR_16BitSwap ? DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709 :
                                                           DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709);
        //rb.scanout.dxgi_colorspace = rb.scanout.colorspace_override;
        }
      }
    }

    if (rb.isHDRCapable () && rb.isHDRActive ())
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
    }

    if (! rb.isHDRCapable ())
    {
      ImGui::SameLine ();
      ImGui::TextColored (ImColor::HSV (0.05f, 0.95f, 0.95f), "HDR Unsupported on Current Monitor / Desktop (!!)");
    }

    if ( ( TenBitSwap_Original     != __SK_HDR_10BitSwap ||
           SixteenBitSwap_Original != __SK_HDR_16BitSwap) )
    {
      // HDR mode selected, but it's not active. Game probably needs a restart
      if (rb.isHDRActive () != (__SK_HDR_16BitSwap || __SK_HDR_10BitSwap))
      {
        ImGui::PushStyleColor (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (.3f, .8f, .9f));
        ImGui::BulletText     ("Game Restart Required");
        ImGui::PopStyleColor  ();
      }
    }

    if ( __SK_HDR_10BitSwap ||
         __SK_HDR_16BitSwap    )
    {
      SK_ComQIPtr <IDXGISwapChain4> pSwap4 (rb.swapchain);

      if (pSwap4 != nullptr)
      {
        static
        DXGI_OUTPUT_DESC1      out_desc  = { };
        DXGI_SWAP_CHAIN_DESC1 swap_desc1 = { };
           pSwap4->GetDesc1 (&swap_desc1);

        if (out_desc.BitsPerColor == 0)
        {
          SK_ComPtr <IDXGIOutput> pOutput;

          if ( SUCCEEDED ( pSwap4->GetContainingOutput (
                              &pOutput                 )
                         )
             )
          {
            SK_ComQIPtr <IDXGIOutput6> pOutput6 (pOutput);

            if ( pOutput6 != nullptr )
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

          ImGui::Separator ();

          ImGui::BeginGroup ();
          if (ImGui::Checkbox ("Enable FULL HDR Luminance###SK_HDR_ShowFullRange", &__SK_HDR_FullRange))
          {
            _SK_HDR_FullRange->store (__SK_HDR_FullRange);
          }

          if (ImGui::IsItemHovered ())
          {
            ImGui::BeginTooltip    (  );
            ImGui::TextUnformatted ((const char *)u8"Brighter HDR highlights are possible when enabled (or by Ctrl-Clicking for manual data input)");
            ImGui::Separator       (  );
            ImGui::BulletText      ("Local contrast behavior may be more consistent with fewer crushed shadow details / highlights when this range is limited.");
            ImGui::EndTooltip      (  );
          }

          static float fSliderWidth = 0.0f;
          static float fWidth0      = 0.0f;
                 float fWidth1      =
            ImGui::GetItemRectSize ().x;

          //float fCursorX = ImGui::GetCursorPosX ();

          ImGui::SameLine ();

          auto& preset =
            hdr_presets [__SK_HDR_Preset];

          bool pboost = (preset.pq_boost0 > 0.0f);

          if (ImGui::Checkbox ("Perceptual Boost", &pboost))
          {
            if (pboost)
            {
              // Store negative values to turn off without losing user's preference
              if (preset.pq_boost0 == 0.0f)
                  preset.pq_boost0 =  8.80582f;
             else preset.pq_boost0 = -preset.pq_boost0;
            }else preset.pq_boost0 = -preset.pq_boost0;

            preset.cfg_pq_boost0->store (
                                preset.pq_boost0);
            __SK_HDR_PQBoost0 = preset.pq_boost0;

            SK_SaveConfig ();
          }

          if (ImGui::IsItemHovered ())
          {
            ImGui::SetTooltip ("NOTE: When active, the luminance sliders (peak white / paper white) do not measure physical brightness.");
          }

          fWidth1     =
            fWidth1 + ImGui::GetItemRectSize ().x;

          bool bypass =
            ( __SK_HDR_Bypass_sRGB == 1 );

          ImGui::SameLine ( 0.0f, fSliderWidth > fWidth0 ?
                                  fSliderWidth - fWidth0 - fWidth1 - ImGui::GetStyle ().ItemSpacing.x
                                                         : -1.0f );

          if (ImGui::Checkbox ("Bypass sRGB Gamma", &bypass))
          {
            __SK_HDR_Bypass_sRGB =
              ( bypass ? 1 : 0 );

            _SK_HDR_sRGBBypassBehavior->store (__SK_HDR_Bypass_sRGB);
          }

          fWidth0 = ImGui::GetItemRectSize ().x;

          if (ImGui::IsItemHovered ())
            ImGui::SetTooltip ("If the game appears absurdly dark, you probably need to turn this on.");


          auto& pINI = dll_ini;

          float peak_nits =
            __SK_HDR_Luma / 1.0_Nits;

          if ( __SK_HDR_tonemap != SK_HDR_TONEMAP_HDR10_PASSTHROUGH )
          {
            if (ImGui::SliderFloat ( "###SK_HDR_LUMINANCE", &peak_nits, 80.0f,
                                          __SK_HDR_FullRange  ?  rb.display_gamut.maxLocalY
                                                              :  rb.display_gamut.maxAverageY,
                (const char *)u8"Peak White Luminance: %.1f cd/m²" ))
            {
              __SK_HDR_Luma = peak_nits * 1.0_Nits;

              preset.peak_white_nits =
                peak_nits * 1.0_Nits;
              preset.cfg_nits->store (preset.peak_white_nits);

              // Paper White needs to respond to changes in Peak White
              if (preset.paper_white_nits > preset.peak_white_nits)
              {
                preset.paper_white_nits =     preset.peak_white_nits;
                preset.cfg_paperwhite->store (preset.paper_white_nits);
                preset.cfg_paperwhite->load  (__SK_HDR_PaperWhite);
              }
            }
          }
          else
          {
            ImGui::BulletText ("Luminance Control Unsupported by Current Tonemap");
            ImGui::SameLine   (0.0f, 135.0f); ImGui::Spacing ();
          }

          float fWhite = __SK_HDR_PaperWhite / 1.0_Nits;

          if (
            ImGui::SliderFloat ("###PaperWhite", &fWhite, 80.0f, std::min ( rb.display_gamut.maxLocalY, __SK_HDR_Luma / 1.0_Nits ),
                   (const char *)u8"Paper White Luminance: %.1f cd/m²")
             )
          {
            __SK_HDR_PaperWhite =
              fWhite * 1.0_Nits;

            preset.paper_white_nits = __SK_HDR_PaperWhite;
            preset.cfg_paperwhite->store (preset.paper_white_nits);
          }

          fSliderWidth = ImGui::GetItemRectSize ().x;

          if (ImGui::IsItemHovered ())
          {
            ImGui::BeginTooltip    (  );
            ImGui::TextUnformatted ("Controls mid-tone luminance");
            ImGui::Separator       (  );
            ImGui::BulletText      ("Keep below 1/2 Peak White for ideal Dynamic Range / Saturation");
            ImGui::BulletText      ("Higher values will bring out shadow detail, but may wash out game HUDs");
            ImGui::BulletText      ("Lower values may correct over-exposure in some washed out SDR games (i.e. NieR: Automata)");
            ImGui::EndTooltip      (  );
          }

          float fMidGray = __SK_HDR_user_sdr_Y - 100.0f;

          if ( __SK_HDR_tonemap == SK_HDR_TONEMAP_FILMIC ||
               __SK_HDR_tonemap == SK_HDR_TONEMAP_HDR10_FILMIC ||
               __SK_HDR_tonemap == SK_HDR_TONEMAP_NONE )
          {
            if (ImGui::SliderFloat ("###SK_HDR_MIDDLE_GRAY", &fMidGray, (__SK_HDR_tonemap == 2) ? -2.5f : -20.0f,
                                                                        (__SK_HDR_tonemap == 2) ?  2.5f :  20.0f,
                              (const char *)u8"Middle Gray Contrast: %+.3f%%"))// %.3f cd/m²"))
            {
              __SK_HDR_user_sdr_Y =
                100.0f + fMidGray;

              preset.middle_gray_nits     = __SK_HDR_user_sdr_Y * 1.0_Nits;
              preset.cfg_middlegray->store (preset.middle_gray_nits);
            }

            if (ImGui::IsItemHovered ())
            {
              ImGui::BeginTooltip    (  );
              //ImGui::TextUnformatted ((const char *)u8"Technically, 80.0 cd/m² and 100.0 cd/m² are Reference SDR Mid-Grays");
              //ImGui::Separator       (  );
              ImGui::BulletText      ("SK boosts mid-gray for improved visibility; mid-gray can balance a game's UI luminance levels.");
              ImGui::BulletText      ("If unhappy with available slider range, Ctrl + Click to override.");
              ImGui::EndTooltip      (  );
            }
          }
          else
            ImGui::BulletText ("Middle-Gray Contrast Unsupported by Current Tonemap");

          ImGui::EndGroup   ();
          ImGui::SameLine   ();
          ImGui::BeginGroup ();
          ImGui::BeginGroup ();
          for ( int i = 0 ; i < MAX_HDR_PRESETS ; i++ )
          {
            const int selected =
              (__SK_HDR_Preset == i) ? 1 : 0;

            char              hashed_name [128] = { };
            strncpy_s (       hashed_name, 128,
              hdr_presets [i].preset_name, _TRUNCATE);

            StrCatBuffA (hashed_name, "###SK_HDR_PresetSel_",      128);
            StrCatBuffA (hashed_name, std::to_string (i).c_str (), 128);

            ImGui::SetNextTreeNodeOpen (false, ImGuiCond_Always);

            if (ImGui::TreeNodeEx (hashed_name, ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_AllowItemOverlap | selected))
            {
              hdr_presets [i].activate ();
            }
          }
          ImGui::EndGroup   ();
          ImGui::SameLine   (); ImGui::Spacing ();
          ImGui::SameLine   (); ImGui::Spacing ();
          ImGui::SameLine   (); ImGui::Spacing (); ImGui::SameLine ();
          ImGui::BeginGroup ();
          for ( const auto& it : hdr_presets )
          {
            ImGui::Text ( (const char *)u8"Peak White: %5.1f cd/m²",
                          it.peak_white_nits / 1.0_Nits );
          }
          ImGui::EndGroup   ();
          ImGui::SameLine   (); ImGui::Spacing ();
          ImGui::SameLine   (); ImGui::Spacing ();
          ImGui::SameLine   (); ImGui::Spacing (); ImGui::SameLine ();
          ImGui::BeginGroup ();
          //for ( auto& it : hdr_presets )
          //{
          //  ImGui::Text ( (const char *)u8"Power-Law ɣ: %3.1f",
          //                  it.eotf );
          //}
          for ( const auto& it : hdr_presets )
          {
            ImGui::Text ( (const char *)u8"Middle Gray: %+5.1f%%",// cd/m²",
                          (it.middle_gray_nits / 1.0_Nits) - 100.0 );
          }
          ImGui::EndGroup   ();
          ImGui::SameLine   (); ImGui::Spacing ();
          ImGui::SameLine   (); ImGui::Spacing ();
          ImGui::SameLine   (); ImGui::Spacing (); ImGui::SameLine ();
          ImGui::BeginGroup ();
          for ( auto& it : hdr_presets )
          {
            _HDRKeybinding ( &it.preset_activate,
                            (&it.preset_activate)->param );
          }
          ImGui::EndGroup  ();

          static auto pGlobalIni =
            SK_CreateINI (
              (std::wstring (SK_GetInstallPath ()) + LR"(\Global\hdr.ini)").c_str ()
            );

          //ImGui::Button     ("Save as Default"); ImGui::SameLine ();
          //ImGui::Button     ("Load Default");    ImGui::SameLine ();

          if (ImGui::Button ("Reset"))
          {
            auto& default_preset =
              hdr_defaults [__SK_HDR_Preset];

            iSK_INISection*
                  pSection = nullptr;

            if (     pGlobalIni->contains_section (rb.displays [rb.active_display].path_name))
              pSection = &pGlobalIni->get_section (rb.displays [rb.active_display].path_name);

            if (pSection != nullptr &&
                pSection->contains_key (                                SK_FormatStringW (L"scRGBLuminance_[%lu]", __SK_HDR_Preset)))
                 preset.cfg_nits->store_str       (pSection->get_value (SK_FormatStringW (L"scRGBLuminance_[%lu]", __SK_HDR_Preset)));
            else preset.cfg_nits->store           (default_preset.peak_white_nits);

            if (pSection != nullptr &&
                pSection->contains_key (                                SK_FormatStringW (L"scRGBPaperWhite_[%lu]", __SK_HDR_Preset)))
                 preset.cfg_paperwhite->store_str (pSection->get_value (SK_FormatStringW (L"scRGBPaperWhite_[%lu]", __SK_HDR_Preset)));
            else preset.cfg_paperwhite->store     (default_preset.paper_white_nits);

            if (pSection != nullptr &&
                pSection->contains_key (                                SK_FormatStringW (L"MiddleGray_[%lu]", __SK_HDR_Preset)))
                 preset.cfg_middlegray->store_str (pSection->get_value (SK_FormatStringW (L"MiddleGray_[%lu]", __SK_HDR_Preset)));
            else preset.cfg_middlegray->store     (default_preset.middle_gray_nits);

            if (pSection != nullptr &&
                pSection->contains_key (                                SK_FormatStringW (L"scRGBGamma_[%lu]", __SK_HDR_Preset)))
                 preset.cfg_eotf->store_str       (pSection->get_value (SK_FormatStringW (L"scRGBGamma_[%lu]", __SK_HDR_Preset)));
            else preset.cfg_eotf->store           (default_preset.eotf);

            if (pSection != nullptr &&
                pSection->contains_key (                                SK_FormatStringW (L"Saturation_[%lu]", __SK_HDR_Preset)))
                 preset.cfg_saturation->store_str (pSection->get_value (SK_FormatStringW (L"Saturation_[%lu]", __SK_HDR_Preset)));
            else preset.cfg_saturation->store     (default_preset.saturation);

            if (pSection != nullptr &&
                pSection->contains_key (                                SK_FormatStringW (L"ToneMapper_[%lu]", __SK_HDR_Preset)))
                 preset.cfg_tonemap->store_str    (pSection->get_value (SK_FormatStringW (L"ToneMapper_[%lu]", __SK_HDR_Preset)));
            else preset.cfg_tonemap->store        (default_preset.colorspace.tonemap);
                                                  
            if (pSection != nullptr &&
                pSection->contains_key (                                SK_FormatStringW (L"PerceptualBoost0_[%lu]", __SK_HDR_Preset)))
                 preset.cfg_pq_boost0->store_str  (pSection->get_value (SK_FormatStringW (L"PerceptualBoost0_[%lu]", __SK_HDR_Preset)));
            else preset.cfg_pq_boost0->store      (default_preset.pq_boost0);
                                                  
            if (pSection != nullptr &&
                pSection->contains_key (                                SK_FormatStringW (L"PerceptualBoost1_[%lu]", __SK_HDR_Preset)))
                 preset.cfg_pq_boost1->store_str  (pSection->get_value (SK_FormatStringW (L"PerceptualBoost1_[%lu]", __SK_HDR_Preset)));
            else preset.cfg_pq_boost1->store      (default_preset.pq_boost1);
                                                  
            if (pSection != nullptr &&
                pSection->contains_key (                                SK_FormatStringW (L"PerceptualBoost2_[%lu]", __SK_HDR_Preset)))
                 preset.cfg_pq_boost2->store_str  (pSection->get_value (SK_FormatStringW (L"PerceptualBoost2_[%lu]", __SK_HDR_Preset)));
            else preset.cfg_pq_boost2->store      (default_preset.pq_boost2);
                                                  
            if (pSection != nullptr &&
                pSection->contains_key (                                SK_FormatStringW (L"PerceptualBoost3_[%lu]", __SK_HDR_Preset)))
                 preset.cfg_pq_boost3->store_str  (pSection->get_value (SK_FormatStringW (L"PerceptualBoost3_[%lu]", __SK_HDR_Preset)));
            else preset.cfg_pq_boost3->store      (default_preset.pq_boost3);

            preset.cfg_nits->load         (preset.peak_white_nits);
            preset.cfg_paperwhite->load   (preset.paper_white_nits);
            preset.cfg_middlegray->load   (preset.middle_gray_nits);
            preset.cfg_eotf->load         (preset.eotf);
            preset.cfg_saturation->load   (preset.saturation);
            preset.cfg_tonemap->load      (preset.colorspace.tonemap);

            preset.cfg_pq_boost0->load    (preset.pq_boost0);
            preset.cfg_pq_boost1->load    (preset.pq_boost1);
            preset.cfg_pq_boost2->load    (preset.pq_boost2);
            preset.cfg_pq_boost3->load    (preset.pq_boost3);

            preset.activate ();
          }

          ImGui::SameLine ();

          if (ImGui::Button ("Export"))
          {
            auto& sec =
              pGlobalIni->get_section (rb.displays [rb.active_display].path_name);

            sec.add_key_value (
              SK_FormatStringW (L"scRGBLuminance_[%lu]", __SK_HDR_Preset),
                                                 std::to_wstring (preset.peak_white_nits) );
            sec.add_key_value (
              SK_FormatStringW (L"scRGBPaperWhite_[%lu]", __SK_HDR_Preset),
                                                  std::to_wstring (preset.paper_white_nits) );
            sec.add_key_value (
              SK_FormatStringW (L"scRGBGamma_[%lu]", __SK_HDR_Preset),
                                             std::to_wstring (preset.eotf) );
            sec.add_key_value (
              SK_FormatStringW (L"ToneMapper_[%lu]", __SK_HDR_Preset),
                                             std::to_wstring (preset.colorspace.tonemap) );
            sec.add_key_value (
              SK_FormatStringW (L"Saturation_[%lu]", __SK_HDR_Preset),
                                             std::to_wstring (preset.saturation) );
            sec.add_key_value (
              SK_FormatStringW (L"MiddleGray_[%lu]", __SK_HDR_Preset),
                                             std::to_wstring (preset.middle_gray_nits) );
            sec.add_key_value (
              SK_FormatStringW (L"PerceptualBoost0_[%lu]", __SK_HDR_Preset),
                                             std::to_wstring (preset.pq_boost0) );
            sec.add_key_value (
              SK_FormatStringW (L"PerceptualBoost1_[%lu]", __SK_HDR_Preset),
                                             std::to_wstring (preset.pq_boost1) );
            sec.add_key_value (
              SK_FormatStringW (L"PerceptualBoost2_[%lu]", __SK_HDR_Preset),
                                             std::to_wstring (preset.pq_boost2) );
            sec.add_key_value (
              SK_FormatStringW (L"PerceptualBoost3_[%lu]", __SK_HDR_Preset),
                                             std::to_wstring (preset.pq_boost3) );

            pGlobalIni->write ();
          }

          if (ImGui::IsItemHovered ())
          {
            ImGui::SetTooltip ("Exporting a preset allows loading it in another game using the \"Reset\" button.");
          }

          ImGui::SameLine ();

          bool bProfile =
            ImGui::Button ("Profile Display Capabilities");

          if (ImGui::IsItemHovered ())
              ImGui::SetTooltip ("Improve HDR results by validating luminance capabilities reported by Windows.");

          if (bProfile)
            ImGui::OpenPopup ("HDR Display Profiler");

          ImGui::EndGroup  ();
          ImGui::Separator ();

          const bool cfg_quality =
            ImGui::CollapsingHeader ( "Performance / Quality",
                                        ImGuiTreeNodeFlags_DefaultOpen );

          if (rb.api != SK_RenderAPI::D3D12)
          {
            if (ImGui::IsItemHovered ())
            {
              ImGui::BeginTooltip    ();
              ImGui::TextUnformatted ("Remastering may improve HDR highlights, but requires more VRAM and GPU horsepower.");
              ImGui::Separator       ();
              ImGui::BulletText      ("Some games need remastering for their UI to work correctly (usually 8-bit remastering)");
              ImGui::BulletText      ("Some games crash when remastering, so troubleshooting should start with enabling/disabling these");
              ImGui::BulletText      ("Remastering can eliminate banding not caused by texture compression (#1 enemy of HDR!)");
              ImGui::EndTooltip      ();
            }
          }

          if (cfg_quality)
          {
            static bool changed_once = false;
                   bool changed      = false;

            ImGui::PushStyleColor (ImGuiCol_PlotHistogram, ImColor::HSV (0.15f, 0.95f, 0.55f));

            if (SK_API_IsLayeredOnD3D11 (rb.api))
            {
              if (ImGui::Checkbox ("Adaptive Tone Mapping",
                                               &__SK_HDR_AdaptiveToneMap))
              { _SK_HDR_AdaptiveToneMap->store (__SK_HDR_AdaptiveToneMap);
                 SK_SaveConfig ();
              }

              if (ImGui::IsItemHovered ())
              {   ImGui::BeginTooltip  ();
                  ImGui::Text          ("Adhere to HGIG Design Guidelines");
                  ImGui::Separator     ();
                  ImGui::BulletText    ("Tonemap keeps Average Frame Light Level from exceeding 'Paper White'");
                  ImGui::BulletText    ("User-calibrated MaxCLL is enforced");
                  ImGui::EndTooltip    ();
              }

              ImGui::SameLine          ();
              ImGui::VerticalSeparator ();
              ImGui::SameLine          ();

              const auto _SummarizeTargets =
              [](DWORD dwPromoted, DWORD dwCandidates, ULONG64 ullBytesExtra)
              {
                const float promoted   = static_cast <float> (dwPromoted);
                const float candidates = static_cast <float> (dwCandidates);

                const float ratio = candidates > 0 ?
                         promoted / candidates     : 0.0f;

                ImGui::ProgressBar ( ratio, ImVec2 (-1, 0),
                  SK_FormatString ( "%lu/%lu Candidates Remastered", dwPromoted, dwCandidates ).c_str ()
                                   );
                ImGui::BulletText  ("%+.2f MiB Additional VRAM Used",
                  static_cast <float> ( ullBytesExtra ) / (1024.0f * 1024.0f));

              };

              changed |= ImGui::Checkbox ("Remaster 8-bit Render Passes",  &SK_HDR_RenderTargets_8bpc->PromoteTo16Bit);

              if (ImGui::IsItemHovered ())
              {
                ImGui::BeginTooltip    ();
                ImGui::TextUnformatted ("May Break FMV in Unreal Engine Games");
                ImGui::Separator       ();
                _SummarizeTargets      (ReadULongAcquire (&SK_HDR_RenderTargets_8bpc->TargetsUpgraded),
                                        ReadULongAcquire (&SK_HDR_RenderTargets_8bpc->CandidatesSeen),
                                        ReadAcquire64    (&SK_HDR_RenderTargets_8bpc->BytesAllocated));
                ImGui::EndTooltip      ();
              }

                         ImGui::SameLine ();
              changed |= ImGui::Checkbox ("Remaster 10-bit Render Passes", &SK_HDR_RenderTargets_10bpc->PromoteTo16Bit);

              if (ImGui::IsItemHovered ())
              {
                ImGui::BeginTooltip    ();
                ImGui::TextUnformatted ("May Strengthen Atmospheric Bloom");
                ImGui::Separator       ();
                _SummarizeTargets      (ReadULongAcquire (&SK_HDR_RenderTargets_10bpc->TargetsUpgraded),
                                        ReadULongAcquire (&SK_HDR_RenderTargets_10bpc->CandidatesSeen),
                                        ReadAcquire64    (&SK_HDR_RenderTargets_10bpc->BytesAllocated));
                ImGui::EndTooltip      ();
              }

                         ImGui::SameLine ();
              changed |= ImGui::Checkbox ("Remaster 11-bit Render Passes", &SK_HDR_RenderTargets_11bpc->PromoteTo16Bit);

              if (ImGui::IsItemHovered ())
              {
                ImGui::BeginTooltip    ();
                ImGui::TextUnformatted ("May Cause -or- Correct Blue / Yellow Hue Shifting");
                ImGui::Separator       ();
                _SummarizeTargets      (ReadULongAcquire (&SK_HDR_RenderTargets_11bpc->TargetsUpgraded),
                                        ReadULongAcquire (&SK_HDR_RenderTargets_11bpc->CandidatesSeen),
                                        ReadAcquire64    (&SK_HDR_RenderTargets_11bpc->BytesAllocated));
                ImGui::EndTooltip      ();
              }
            }

            ImGui::PopStyleColor ();

            if ( preset.pq_boost0 > 0.1f )
            { 
              ImGui::SameLine          ();
              ImGui::VerticalSeparator ();
              ImGui::SameLine          ();
              
              bool bExperimental =
                ImGui::TreeNode ("Experimental");

              if (ImGui::IsItemHovered ())
                  ImGui::SetTooltip ("Default Values Should be Good; but tweak away if you must :)");

              if (bExperimental)
              {
                ImGui::Separator ();

                bool boost_changed = false;

                boost_changed |=
                  ImGui::SliderFloat ("Perceptual Boost 0", &preset.pq_boost0, 3.0f, 20.0f);
                boost_changed |=
                  ImGui::SliderFloat ("Perceptual Boost 1", &preset.pq_boost1, 3.0f, 20.0f);
                boost_changed |=                                                              
                  ImGui::SliderFloat ("Perceptual Boost 2", &preset.pq_boost2, 0.5f, 1.5f);
                boost_changed |=                                                              
                  ImGui::SliderFloat ("Perceptual Boost 3", &preset.pq_boost3, 0.5f, 1.5f);

                if (boost_changed)
                {
                  preset.cfg_pq_boost0->store (
                      preset.pq_boost0 );
                  preset.cfg_pq_boost1->store (
                      preset.pq_boost1 );
                  preset.cfg_pq_boost2->store (
                      preset.pq_boost2 );
                  preset.cfg_pq_boost3->store (
                      preset.pq_boost3 );

                  __SK_HDR_PQBoost0 = preset.pq_boost0;
                  __SK_HDR_PQBoost1 = preset.pq_boost1;
                  __SK_HDR_PQBoost2 = preset.pq_boost2;
                  __SK_HDR_PQBoost3 = preset.pq_boost3;

                  SK_SaveConfig ();
                }

                ImGui::TreePop ();
              }
            }

            changed_once |= changed;

            if (changed_once)
            {
              ImGui::PushStyleColor (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (.3f, .8f, .9f));
              ImGui::BulletText     ("Game Restart Required");
              ImGui::PopStyleColor  ();
            }

            if (changed)
            {
              _SK_HDR_Promote8BitRGBxTo16BitFP->store  (SK_HDR_RenderTargets_8bpc ->PromoteTo16Bit);
              _SK_HDR_Promote10BitRGBATo16BitFP->store (SK_HDR_RenderTargets_10bpc->PromoteTo16Bit);
              _SK_HDR_Promote11BitRGBTo16BitFP->store  (SK_HDR_RenderTargets_11bpc->PromoteTo16Bit);

              dll_ini->write ();
            }
          }

          if (
            ImGui::CollapsingHeader ( "Advanced###HDR_Widget_Advaced",
                                        ImGuiTreeNodeFlags_DefaultOpen )
          )
          {
            ImGui::BeginGroup  ();
            ImGui::BeginGroup  ();

            if ( ImGui::Combo ( "Tonemap Mode##SK_HDR_GAMUT_IN",
                                               &preset.colorspace.tonemap,
                  __SK_HDR_ColorSpaceComboStr)                        )
            {
              if (__SK_HDR_ColorSpaceMap.count (preset.colorspace.tonemap))
              {
                __SK_HDR_tonemap       =        preset.colorspace.tonemap;
                preset.cfg_tonemap->store      (preset.colorspace.tonemap);

                pINI->write ();
              }

              else
              {
                preset.colorspace.tonemap = __SK_HDR_tonemap;
              }
            }

            if ( __SK_HDR_tonemap == SK_HDR_TONEMAP_FILMIC       ||
                 __SK_HDR_tonemap == SK_HDR_TONEMAP_HDR10_FILMIC ||
                 __SK_HDR_tonemap == SK_HDR_TONEMAP_NONE )
            {
              float fSat =
                __SK_HDR_Saturation * 100.0f;

              if (ImGui::SliderFloat ("Color Saturation", &fSat, 0.0f, 133.0f, "%.3f%%"))
              {
                __SK_HDR_Saturation =
                  std::max (0.0f, std::min (2.0f, fSat / 100.0f));

                preset.saturation           = __SK_HDR_Saturation;
                preset.cfg_saturation->store (__SK_HDR_Saturation);
              }
            }
            else
              ImGui::BulletText ("Color Saturation Unsupported for Current Tonemap");

            ImGui::EndGroup    ();

            if ( __SK_HDR_tonemap != SK_HDR_TONEMAP_HDR10_PASSTHROUGH )
            {
              if (ImGui::SliderFloat ("SDR Gamma Boost###SK_HDR_GAMMA", &__SK_HDR_Exp,
                              0.476f, 1.524f, "SDR -> HDR Gamma: %.3f"))
              {
                preset.eotf =
                  __SK_HDR_Exp;
                preset.cfg_eotf->store (preset.eotf);
              }
            }
            else
            {
              bool bKill22 =
                ( __SK_HDR_Exp != 1.0 );

              if (ImGui::Checkbox ("Fix SDR/HDR Black Level Mismatch", &bKill22))
              {
                  __SK_HDR_Exp = ( bKill22 ?
                                       2.2f : 1.0f );
                preset.eotf    =
                  __SK_HDR_Exp;
                preset.cfg_eotf->store (preset.eotf);
              }
              if (ImGui::IsItemHovered ())
                ImGui::SetTooltip ("Ubisoft games (e.g. Watch Dogs Legions) use 0.34 nits as SDR black for video and UI, which is gray in HDR...");
              //ImGui::BulletText ("Gamma Correction Unsupported for Current Tonemap");
            }

            //ImGui::SameLine    ();
            ImGui::BeginGroup  ();
            ImGui::SliderFloat ("X-Axis HDR/SDR Splitter", &__SK_HDR_HorizCoverage, 0.0f, 100.f);
            ImGui::SliderFloat ("Y-Axis HDR/SDR Splitter", &__SK_HDR_VertCoverage,  0.0f, 100.f);
            ImGui::EndGroup    ();

            ImGui::Combo       ("HDR Visualization##SK_HDR_VIZ",  &__SK_HDR_visualization, "None\0Luminance (vs EDID Max-Avg-Y)\0"
                                                                                                 "Luminance (vs EDID Max-Local-Y)\0"
                                                                                                 "Luminance (Exposure)\0"
                                                                                                 "HDR Overbright Bits\0"
                                                                                                 "8-Bit Quantization\0"
                                                                                                 "10-Bit Quantization\0"
                                                                                                 "12-Bit Quantization\0"
                                                                                                 "16-Bit Quantization\0"
                                                                                                 "Gamut Overshoot (vs Rec.709)\0"
                                                                                                 "Gamut Overshoot (vs DCI-P3)\0"
                                                                                                 "Tonemap Curve and Grayscale\0\0");
                                                                                                  //"Maximum Local Clip Point v0\0"
                                                                                                  //"Maximum Local Clip Point v1\0"
                                                                                                  //"Maximum Local Clip Point v2\0\0");
                                                                                                 //"Gamut Overshoot (vs Rec.2020)\0\0");
            ImGui::EndGroup    ();

            ImGui::SameLine    ();

            ImGui::BeginGroup  ();
            SK_ImGui_DrawGamut ();
            ImGui::EndGroup    ();


            struct lumaHistogram {
              float buckets [16384] = { };

              SK_ComPtr <ID3D11Buffer>              pBuffer = nullptr;
              SK_ComPtr <ID3D11UnorderedAccessView> pUAV    = nullptr;

              void cleanup (void)
              {
                if (pBuffer.p != nullptr)
                    pBuffer = nullptr;

                if (pUAV.p != nullptr)
                    pUAV = nullptr;
              }

              void init (void) noexcept
              {
                D3D11_BUFFER_DESC bufferDesc = { };
              }
            } static histogram;


          //ImGui::PlotHistogram ("Luminance", histogram.buckets, 16384);
          }

          if ( (! rb.fullscreen_exclusive) && (
                 swap_desc1.Format     == DXGI_FORMAT_R10G10B10A2_UNORM ||
                 rb.scanout.getEOTF () == SK_RenderBackend::scan_out_s::SMPTE_2084 ) )
          {
            ImGui::BulletText ("HDR May Not be Working Correctly Until you Restart the Game...");
          }

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





glm::highp_vec3
SK_Color_XYZ_from_RGB ( const SK_ColorSpace& cs, glm::highp_vec3 RGB )
{
  const double Xr =        cs.xr / cs.yr;
  const double Zr = (1.0 - cs.xr - cs.yr) / cs.yr;

  const double Xg =        cs.xg / cs.yg;
  const double Zg = (1.0 - cs.xg - cs.yg) / cs.yg;

  const double Xb =        cs.xb / cs.yb;
  const double Zb = (1.0 - cs.xb - cs.yb) / cs.yb;

  constexpr double Yr = 1.0;
  constexpr double Yg = 1.0;
  constexpr double Yb = 1.0;

  const glm::highp_mat3x3 xyz_primary ( Xr, Xg, Xb,
                                        Yr, Yg, Yb,
                                        Zr, Zg, Zb );

  const glm::highp_vec3 S ( glm::inverse (xyz_primary) *
                            glm::highp_vec3 (cs.Xw, cs.Yw, cs.Zw) );

  return RGB *
    glm::highp_mat3x3 ( S.r * Xr, S.g * Xg, S.b * Xb,
                        S.r * Yr, S.g * Yg, S.b * Yb,
                        S.r * Zr, S.g * Zg, S.b * Zb );
}

glm::highp_vec3
SK_Color_xyY_from_RGB ( const SK_ColorSpace& cs, glm::highp_vec3 RGB )
{
  const glm::highp_vec3 XYZ =
    SK_Color_XYZ_from_RGB ( cs, RGB );

  return
    glm::highp_vec3 ( XYZ.x / (XYZ.x + XYZ.y + XYZ.z),
                      XYZ.y / (XYZ.x + XYZ.y + XYZ.z),
                                       XYZ.y );
}

void
SK_ImGui_DrawGamut (void)
{
  static const ImVec2 cie_pts [] = {
    ImVec2 ( 0.174112257f, 0.004963727f ), // 380.0
    ImVec2 ( 0.150985408f, 0.022740193f ), // 455.0
    ImVec2 ( 0.135502671f, 0.039879121f ), // 465.0
    ImVec2 ( 0.124118477f, 0.057802513f ), // 470.0
    ImVec2 ( 0.109594324f, 0.086842511f ), // 475.0
    ImVec2 ( 0.091293516f, 0.132702055f ), // 480.0
    ImVec2 ( 0.068705910f, 0.200723220f ), // 485.0
    ImVec2 ( 0.045390735f, 0.294975965f ), // 490.0
    ImVec2 ( 0.023459943f, 0.412703479f ), // 495.0
    ImVec2 ( 0.008168028f, 0.538423071f ), // 500.0
    ImVec2 ( 0.003858521f, 0.654823151f ), // 505.0
    ImVec2 ( 0.013870246f, 0.750186428f ), // 510.0
    ImVec2 ( 0.038851802f, 0.812016021f ), // 515.0
    ImVec2 ( 0.074302424f, 0.833803082f ), // 520.0
    ImVec2 ( 0.114160721f, 0.826206968f ), // 525.0
    ImVec2 ( 0.154722061f, 0.805863545f ), // 530.0
    ImVec2 ( 0.192876183f, 0.781629131f ), // 535.0
    ImVec2 ( 0.229619673f, 0.754329090f ), // 540.0
    ImVec2 ( 0.265775085f, 0.724323925f ), // 545.0
    ImVec2 ( 0.736842105f, 0.263157895f )  // 780.0
  };


  const ImVec4 col (0.25f, 0.25f, 0.25f, 0.8f);

  const ImU32 col32 =
    ImColor (col);

  ImDrawList* draw_list =
    ImGui::GetWindowDrawList ();

  static auto& rb =
    SK_GetCurrentRenderBackend ();

  struct color_triangle_s {
    std::string     name;
    glm::highp_vec3 r, g,
                    b, w;
    bool            show   = true;

    double          _area  = 0.0;

    struct {
      std::string text;
      float       text_len = 0.0f; // ImGui horizontal size
      float       fIdx     = 0.0f;
    } summary;

    color_triangle_s (const std::string& _name, SK_ColorSpace cs) : name (_name)
    {
      r = SK_Color_xyY_from_RGB (cs, glm::highp_vec3 (1., 0., 0.));
      g = SK_Color_xyY_from_RGB (cs, glm::highp_vec3 (0., 1., 0.));
      b = SK_Color_xyY_from_RGB (cs, glm::highp_vec3 (0., 0., 1.));
      w =                       {cs.Xw,
                                 cs.Yw,
                                 cs.Zw};

      const double r_x = sqrt (r.x * r.x);  const double r_y = sqrt (r.y * r.y);
      const double g_x = sqrt (g.x * g.x);  const double g_y = sqrt (g.y * g.y);
      const double b_x = sqrt (b.x * b.x);  const double b_y = sqrt (b.y * b.y);

      const double A =
        fabs (r_x*(b_y-g_y) + g_x*(b_y-r_y) + b_x*(r_y-g_y));

      _area = A;

      show = true;
    }

    double area (void) const noexcept
    {
      return _area;
    }

    double coverage (const color_triangle_s& target) const noexcept
    {
      return
        100.0 * ( area () / target.area () );
    }
  };

#define D65 0.3127f, 0.329f

  static HMONITOR                       hMonLast = nullptr;
  static glm::vec3                      r, g, b, w;
  static std::vector <color_triangle_s> color_spaces;
  static              color_triangle_s _NativeGamut (
                                       "NativeGamut",
                                   rb.display_gamut );
  static std::string                   _CombinedName;

  const bool new_monitor =
    ( hMonLast != rb.monitor );

  if (new_monitor || ( w.x != rb.display_gamut.Xw ||
                       w.y != rb.display_gamut.Yw ||
                       w.z != rb.display_gamut.Zw ) )
  {
    r = glm::vec3 (SK_Color_xyY_from_RGB (rb.display_gamut, glm::highp_vec3 (1.f, 0.f, 0.f))),
    g = glm::vec3 (SK_Color_xyY_from_RGB (rb.display_gamut, glm::highp_vec3 (0.f, 1.f, 0.f))),
    b = glm::vec3 (SK_Color_xyY_from_RGB (rb.display_gamut, glm::highp_vec3 (0.f, 0.f, 1.f))),
    w = glm::vec3 (                       rb.display_gamut.Xw,
                                          rb.display_gamut.Yw,
                                          rb.display_gamut.Zw );

    if (w.z == 0.0f)
      return;

    hMonLast = rb.monitor;

    _NativeGamut =
      color_triangle_s ( "NativeGamut",   rb.display_gamut );

    color_spaces.clear ();
    color_spaces =
    {
      color_triangle_s ( "DCI-P3",        SK_ColorSpace { 0.68f, 0.32f,  0.2650f,  0.690f,   0.150f, 0.060f,
                                                            D65, 1.00f - 0.3127f - 0.329f,
                                                          0.00f, 0.00f,  0.0000f } ),

      color_triangle_s ( "ITU-R BT.709",  SK_ColorSpace { 0.64f, 0.33f, 0.3f, 0.6f,          0.150f, 0.060f,
                                                            D65, 1.00f,
                                                          0.00f, 0.00f, 0.0f } ),

      color_triangle_s ( "ITU-R BT.2020", SK_ColorSpace { 0.708f, 0.292f,  0.1700f,  0.797f, 0.131f, 0.046f,
                                                            D65,  1.000f - 0.3127f - 0.329f,
                                                            0.0f, 0.000f,  0.0000f } ),

      color_triangle_s ( "Adobe RGB",     SK_ColorSpace { 0.64f, 0.33f,  0.2100f,  0.710f,   0.150f, 0.060f,
                                                            D65, 1.00f - 0.3127f - 0.329f,
                                                          0.00f, 0.00f,  0.0000f } ),

      color_triangle_s { "NTSC",          SK_ColorSpace { 0.67f, 0.330f, 0.21f,  0.71f,      0.140f, 0.080f,
                                                          0.31f, 0.316f, 1.00f - 0.31f - 0.316f,
                                                          0.00f, 0.000f, 0.00f } }
    };

    _CombinedName =
      SK_FormatString ( "%ws  (%s)",
                        rb.display_name,
                      _NativeGamut.name.c_str ()
      );
  }

  const auto current_time =
    SK::ControlPanel::current_time;

  const auto& style =
    ImGui::GetStyle ();

  const float x_pos       = ImGui::GetCursorPosX     () + style.ItemSpacing.x  * 2.0f,
              line_ht     = ImGui::GetTextLineHeight (),
              checkbox_ht =                     line_ht + style.FramePadding.y * 2.0f;

  ImGui::SetCursorPosX   (x_pos);
  ImGui::BeginGroup      ();

  const ImColor self_outline_v4 =
    ImColor::HSV ( std::min (1.0f, 0.85f + (sin ((float)(current_time % 400) / 400.0f))),
                             0.0f,
                     (float)(0.66f + (current_time % 830) / 830.0f ) );

  const ImU32 self_outline =
    self_outline_v4;

  ImGui::TextColored     (self_outline_v4, "%s", rb.displays [rb.active_display].full_name);
  ImGui::Separator       ();

  static float max_len,
               num_spaces;

  if (new_monitor)
  {
    max_len    = 0.0f;
    num_spaces = static_cast <float> (color_spaces.size ());

    // Compute area and store text
    std::for_each ( color_spaces.begin (),
                    color_spaces.end   (),
     [](color_triangle_s& space)
     {
       static int idx = 0;

       space.summary.fIdx     = static_cast <float> (idx++);
       space.summary.text_len =
         ImGui::CalcTextSize (
     ( space.summary.text     =
         SK_FormatString ("%5.2f%%", _NativeGamut.coverage (space))
     ).c_str ()
         ).x;

       max_len =   std::max (
                        max_len,
         space.summary.text_len
       );
     }
    );
  }

  ImGui::BeginGroup      ();
  for ( auto& space : color_spaces )
  {
    const float y_pos         = ImGui::GetCursorPosY (),
                text_offset   = max_len - space.summary.text_len,
                fIdx          =           space.summary.fIdx / num_spaces;

    ImGui::PushStyleColor(ImGuiCol_Text, ImColor::HSV (fIdx, 0.85f, 0.98f));
    ImGui::PushID        (static_cast <int> (space.summary.fIdx));

    ImGui::SetCursorPosX (x_pos  + style.ItemSpacing.x   + text_offset);
    ImGui::SetCursorPosY (y_pos  + style.FramePadding.y  + 0.5f *
                   ((checkbox_ht - style.FramePadding.y) - line_ht));

    ImGui::TextColored   ( ImColor::HSV (fIdx, 0.35f, 0.98f),
                             "%s", space.summary.text.c_str () );

    ImGui::SameLine      ();
    ImGui::SetCursorPosY (y_pos);

    ImGui::Checkbox      (space.name.c_str (), &space.show);

    ImGui::PopID         ();
    ImGui::PopStyleColor ();
  }
  ImGui::EndGroup        ();
  ImGui::SameLine        ();

  const auto _DrawCharts = [&](ImVec2 pos         = ImGui::GetCursorScreenPos    (),
                               ImVec2 size        = ImGui::GetContentRegionAvail (),
                               int    draw_labels = -1)
  {
    ImGui::BeginGroup    ();

    float  X0     =  pos.x;
    float  Y0     =  pos.y;
    float  width  = size.x;
    float  height = size.y;

    static constexpr float _LabelFontSize = 30.0f;

    const ImVec2 label_line2 ( ImGui::GetStyle ().ItemSpacing.x * 4.0f,
      ImGui::GetFont ()->CalcTextSizeA (_LabelFontSize, 1.0f, 1.0f, "T").y );

    draw_list->PushClipRect ( ImVec2 (X0,         Y0),
                              ImVec2 (X0 + width, Y0 + height) );

    // Compute primary color points in CIE xy space
    const auto _MakeTriangleVerts = [&] (const glm::vec3& r,
                                         const glm::vec3& g,
                                         const glm::vec3& b,
                                         const glm::vec3& w) ->
    std::array <ImVec2, 4>
    {
      return {
        ImVec2 (X0 + r.x * width, Y0 + (height - r.y * height)),
        ImVec2 (X0 + g.x * width, Y0 + (height - g.y * height)),
        ImVec2 (X0 + b.x * width, Y0 + (height - b.y * height)),
        ImVec2 (X0 + w.x * width, Y0 + (height - w.y * height))
      };
    };


    std::vector <ImVec2> pts;
                         pts.reserve ( 1 + sizeof (cie_pts) /
                                           sizeof (cie_pts [0]) );
    for ( auto& pt : cie_pts )
    {
      pts.emplace_back ( ImVec2 (X0 +          pt.x * width, Y0 + (height -          pt.y * height)) );
    } pts.emplace_back ( ImVec2 (X0 + cie_pts [0].x * width, Y0 + (height - cie_pts [0].y * height)) );

    auto display_pts =
      _MakeTriangleVerts (r, g, b, w);

    draw_list->AddTriangleFilled ( display_pts [0], display_pts [1], display_pts [2],
                                     col32 );

    for (auto& space : color_spaces)
    {
      if (space.show)
      {
        const float     fIdx =
          space.summary.fIdx / num_spaces;

        const ImU32 outline_color =
          ImColor::HSV (fIdx, 0.85f, 0.98f);

        auto space_pts =
          _MakeTriangleVerts (space.r, space.g, space.b, space.w);

        draw_list->AddTriangle (space_pts [0], space_pts [1], space_pts [2],
                                outline_color, 4.0f);
        draw_list->AddCircleFilled (space_pts [3], 4.0f,
                                    outline_color);
      }
    }

    draw_list->AddPolyline (      pts.data (),
                             (int)pts.size (), ImColor (1.f, 1.f, 1.f, 1.f), true, 6.f );

    for (auto& space : color_spaces)
    {
      if (space.show)
      {
        const float     fIdx =
          space.summary.fIdx / num_spaces;

        const ImU32 outline_color =
          ImColor::HSV (fIdx, 0.85f, 0.98f);

        auto space_pts =
          _MakeTriangleVerts (space.r, space.g, space.b, space.w);

        if ( draw_labels != -1 )
        {
          struct {
            int           anchor_pt = 1;
            unsigned long last_time = 0;

            int getIdx (void) noexcept
            {
              static constexpr unsigned long
                _RelocationInterval = std::numeric_limits <unsigned long>::max ();
                                    //1500ul;

              if ( SK::ControlPanel::current_time - last_time >
                     _RelocationInterval )
              {
                last_time = SK::ControlPanel::current_time,
                anchor_pt =
                  (++anchor_pt > 2) ? 0 : anchor_pt;
              }

              return anchor_pt;
            }
          } static label_locator;

          const auto label_idx =
            label_locator.getIdx ();

          draw_list->AddText ( ImGui::GetFont (), _LabelFontSize,
                                 space_pts [label_idx], outline_color,
                                   space.name.c_str () );
          draw_list->AddText ( ImGui::GetFont (), _LabelFontSize * 0.85f,
                                 { space_pts [label_idx].x + label_line2.x,
                                   space_pts [label_idx].y + label_line2.y },
                                                ImColor::HSV ( fIdx, 0.35f, 0.98f ),
              SK_FormatString ( "Relative Coverage: %s", space.summary.text.c_str () ).c_str () );
        }
      }
    }

    draw_list->AddTriangle     ( display_pts [0], display_pts [1], display_pts [2],
                                   self_outline,  6.0f );
    draw_list->AddCircleFilled ( display_pts [3], 6.0f,
                                   self_outline        );

    if (draw_labels != -1)
    {
      draw_list->AddText ( ImGui::GetFont (), _LabelFontSize * 1.15f,
                             display_pts [0], self_outline,
                               _CombinedName.c_str () );
    }

    draw_list->PopClipRect
                         ();
    ImGui::EndGroup      ();
  };

  _DrawCharts ();

  ImGui::EndGroup        ();

  if (ImGui::IsItemHovered ())
  {
    _DrawCharts (ImVec2 (0.0f, 0.0f), ImGui::GetIO ().DisplaySize, 0);
  }
}