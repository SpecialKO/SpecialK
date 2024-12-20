// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
//
// Copyright 2020 Andon "Kaldaien" Coleman
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

static float                              SK_Persona4_fClearColor [] = { .45f, .45f, .45f, .65f };
static bool                               SK_Persona4_DisableBlur    = false;

static SK_ComPtr <ID3D11RenderTargetView> SK_Persona4_pBlurRTV;
static SK_ComPtr <ID3D11DeviceContext>    SK_Persona4_pDevCtx;

static constexpr uint32_t ps_crc32c_RtClear = 0xc3c9a336;
static constexpr uint32_t vs_crc32c_RtClear = 0x4ab0b9c4;

void
SK_Persona4_DrawHandler ( ID3D11DeviceContext* pDevCtx,
                          uint32_t             current_vs,
                          uint32_t             current_ps )
{
  if ( current_ps == ps_crc32c_RtClear &&
       current_vs == vs_crc32c_RtClear )
  {
    SK_ComPtr <ID3D11RenderTargetView>   pRTV;
    SK_ComPtr <ID3D11DepthStencilView>   pDSV;

    pDevCtx->OMGetRenderTargets ( 1,    &pRTV.p,
                                        &pDSV.p );

    if (! pRTV.p)
      return;

    SK_ComPtr <ID3D11Resource>           pRes;
    pRTV->GetResource                 (&pRes.p);

    if (! pRes.p)
      return;

    SK_ComQIPtr <ID3D11Texture2D> pTex (pRes);

    if (! pTex.p)
      return;

    D3D11_TEXTURE2D_DESC tex_desc;
    pTex->GetDesc      (&tex_desc);

    // Shadow maps are square ( 2048x2048, ?, ? )
    if ( tex_desc.Width  != tex_desc.Height )
    {
      SK_Persona4_pDevCtx  = pDevCtx;
      SK_Persona4_pBlurRTV = pRTV.p;

      ///D3D11_RENDER_TARGET_VIEW_DESC rtv_desc = { };
      ///pRTV->GetDesc               (&rtv_desc);

      //dll_log->Log ( L"vs=%x, Fmt=%s [ Fmt=%s, (%lux%lu) { %p } ]",
      //                 current_vs, SK_DXGI_FormatToStr (rtv_desc.Format).c_str (),
      //                             SK_DXGI_FormatToStr (tex_desc.Format).c_str (),
      //                                                  tex_desc.Width,
      //                                                  tex_desc.Height,
      //                                                  pTex.p );
    }
  }
}

struct {
  struct {
    void init  (void)
    {
      bypass_blur =
        _CreateConfigParameterBool ( L"Persona4.PlugIn",
                                     L"BypassBlur",      SK_Persona4_DisableBlur,
                                                                  L"Bypass Blur" );
      clear0 =
        _CreateConfigParameterFloat ( L"Persona4.PlugIn",
                                      L"ClearColor0",    SK_Persona4_fClearColor [0],
                                                                    L"Clear Color 0" );
      clear1 =
        _CreateConfigParameterFloat ( L"Persona4.PlugIn",
                                      L"ClearColor1",    SK_Persona4_fClearColor [1],
                                                                    L"Clear Color 1" );
      clear2 =
        _CreateConfigParameterFloat ( L"Persona4.PlugIn",
                                      L"ClearColor2",    SK_Persona4_fClearColor [2],
                                                                    L"Clear Color 2" );
      clear3 =
        _CreateConfigParameterFloat ( L"Persona4.PlugIn",
                                      L"ClearColor3",    SK_Persona4_fClearColor [3],
                                                                    L"Clear Color 3" );
    }

    sk::ParameterBool*  bypass_blur;
    sk::ParameterFloat* clear0;
    sk::ParameterFloat* clear1;
    sk::ParameterFloat* clear2;
    sk::ParameterFloat* clear3;
  } ini_params;
} SK_Persona4;

bool
SK_Persona4_PlugInCfg (void)
{
  if (ImGui::CollapsingHeader ("Persona 4 Golden", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::TreePush ("");

    if (! SK_PE32_IsLargeAddressAware ())
    {
      ImGui::BeginChildFrame (ImGui::GetID ("###LAAPATCH"), ImVec2 (0.f, ImGui::GetTextLineHeightWithSpacing () * 2.1f));
      ImGui::PushStyleColor  (ImGuiCol_Text, (ImVec4)ImColor::HSV(0.08F, 0.99F, 1.F));
      ImGui::BulletText      ("Executable is not Large Address Aware");
      ImGui::PopStyleColor   ();

      ImGui::TreePush        ("");
      ImGui::TextUnformatted ("Game will be unstable until manually patched, click here for details.");
      if (ImGui::IsItemClicked ())
      {
        SK_RunOnce (
          SK_ShellExecuteW     ( nullptr, L"open",
                                   L"https://ntcore.com/?page_id=371", nullptr,
                                     nullptr, SW_SHOWNORMAL
          )
        );
      }
      ImGui::TreePop         ();
      ImGui::EndChild        ();
    }

    bool bChanged =
      ImGui::Checkbox ("Disable Motion Blur###SK_Persona4_DisableBlur",       &SK_Persona4_DisableBlur);

    if (SK_Persona4_DisableBlur)
    {             ImGui::SameLine    ();
      bChanged |= ImGui::InputFloat4 ("Clear Color###SK_Persona4_ClearColor",  SK_Persona4_fClearColor);
    }

    if (bChanged)
    {
      SK_Persona4.ini_params.bypass_blur->store (SK_Persona4_DisableBlur);
      SK_Persona4.ini_params.clear0->store      (SK_Persona4_fClearColor [0]);
      SK_Persona4.ini_params.clear1->store      (SK_Persona4_fClearColor [1]);
      SK_Persona4.ini_params.clear2->store      (SK_Persona4_fClearColor [2]);
      SK_Persona4.ini_params.clear3->store      (SK_Persona4_fClearColor [3]);

      SK_GetDLLConfig ()->write (
        SK_GetDLLConfig ()->get_filename ()
      );
    }

    ImGui::TreePop  (  );
  }

  return true;
}

void __stdcall
SK_Persona4_EndFrame (void)
{
  if (SK_Persona4_pBlurRTV != nullptr)
  {
    if (SK_Persona4_DisableBlur)
    {
      SK_Persona4_pDevCtx->ClearRenderTargetView (
        SK_Persona4_pBlurRTV, SK_Persona4_fClearColor
      );
    }

    SK_Persona4_pDevCtx  = nullptr;
    SK_Persona4_pBlurRTV = nullptr;
  }
}

void
SK_Persona4_InitPlugin (void)
{
  SK_RunOnce (
  {
    SK_Persona4.ini_params.init ();

    plugin_mgr->config_fns.insert    (SK_Persona4_PlugInCfg);
    plugin_mgr->end_frame_fns.insert (SK_Persona4_EndFrame);
  });
}