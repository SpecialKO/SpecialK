//
// Copyright 2017 Andon "Kaldaien" Coleman
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
#include <SpecialK/render/dxgi/dxgi_backend.h>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_d3d11.h>

#define MSS_VERSION_NUM L"0.0.1"
#define MSS_VERSION_STR L"MARVEL SuperSampled v " MSS_VERSION_NUM

volatile LONG __MSS_init = FALSE;

extern void
__stdcall
SK_SetPluginName (std::wstring name);

static D3D11Dev_CreateShaderResourceView_pfn  _D3D11Dev_CreateShaderResourceView_Original  = nullptr;
static D3D11Dev_CreateTexture2D_pfn           _D3D11Dev_CreateTexture2D_Original           = nullptr;
static D3D11_RSSetViewports_pfn               _D3D11_RSSetViewports_Original               = nullptr;
static D3D11_RSSetScissorRects_pfn            _D3D11_RSSetScissorRects_Original            = nullptr;

struct mss_cfg_s
{
  sk::ParameterFactory factory;

  struct ui_s
  {
    sk::ParameterInt* res_x = nullptr;
    sk::ParameterInt* res_y = nullptr;
  } ui;
} mss_config;

struct {
  int x, y;
} mss_res;

std::unordered_set <ID3D11Resource *> mss_ui_render_buffers;

using D3D11Dev_CreateBuffer_pfn = HRESULT (WINAPI *)(
  _In_           ID3D11Device            *This,
  _In_     const D3D11_BUFFER_DESC       *pDesc,
  _In_opt_ const D3D11_SUBRESOURCE_DATA  *pInitialData,
  _Out_opt_      ID3D11Buffer           **ppBuffer
);
using D3D11Dev_CreateTexture2D_pfn = HRESULT (WINAPI *)(
  _In_            ID3D11Device            *This,
  _In_      const D3D11_TEXTURE2D_DESC    *pDesc,
  _In_opt_  const D3D11_SUBRESOURCE_DATA  *pInitialData,
  _Out_opt_       ID3D11Texture2D        **ppTexture2D
);

extern
void
STDMETHODCALLTYPE
D3D11_RSSetScissorRects_Override (
        ID3D11DeviceContext *This,
        UINT                 NumRects,
  const D3D11_RECT          *pRects );

extern
void
STDMETHODCALLTYPE
D3D11_RSSetViewports_Override (
        ID3D11DeviceContext* This,
        UINT                 NumViewports,
  const D3D11_VIEWPORT*      pViewports );

extern
HRESULT
WINAPI
D3D11Dev_CreateTexture2D_Override (
  _In_            ID3D11Device           *This,
  _In_      const D3D11_TEXTURE2D_DESC   *pDesc,
  _In_opt_  const D3D11_SUBRESOURCE_DATA *pInitialData,
  _Out_opt_       ID3D11Texture2D        **ppTexture2D );



using SK_PlugIn_ControlPanelWidget_pfn = void (__stdcall         *)(void);
using SK_EndFrame_pfn                  = void (STDMETHODCALLTYPE *)(void);
using SK_PluginKeyPress_pfn            = void (CALLBACK          *)( BOOL Control,
                                                                     BOOL Shift,
                                                                     BOOL Alt,
                                                                     BYTE vkCode );

void __stdcall SK_IT_ControlPanel (void);

static SK_PlugIn_ControlPanelWidget_pfn SK_PlugIn_ControlPanelWidget_Original = nullptr;

__declspec (noinline)
HRESULT
WINAPI
SK_MSS_CreateTexture2D (
  _In_            ID3D11Device           *This,
  _In_      const D3D11_TEXTURE2D_DESC   *pDesc,
  _In_opt_  const D3D11_SUBRESOURCE_DATA *pInitialData,
  _Out_opt_       ID3D11Texture2D        **ppTexture2D )
{
  if (ppTexture2D == nullptr || pDesc == nullptr)
    return _D3D11Dev_CreateTexture2D_Original ( This, pDesc, pInitialData, ppTexture2D);

  D3D11_TEXTURE2D_DESC copy (*pDesc);

  bool ui_tex = false;

  if ( ( ( pDesc->BindFlags & D3D11_BIND_DEPTH_STENCIL ) ||
         ( pDesc->BindFlags & D3D11_BIND_RENDER_TARGET ) ) &&
         ( pDesc->Width    == 1920 && pDesc->Height == 1080 ) )
  {
    if (mss_res.x == 0) mss_res.x = 1920;
    if (mss_res.y == 0) mss_res.y = 1080;

    if ( (pDesc->BindFlags & D3D11_BIND_RENDER_TARGET) &&
          pDesc->Format == DXGI_FORMAT_R8G8B8A8_UNORM )
    {
      copy.Width  = gsl::narrow_cast <UINT> (3840);
      copy.Height = gsl::narrow_cast <UINT> (2160);

      ui_tex = true;
    }

    else if (pDesc->BindFlags & D3D11_BIND_DEPTH_STENCIL)
    {
      copy.Width  = gsl::narrow_cast <UINT> (3840);
      copy.Height = gsl::narrow_cast <UINT> (2160);

      ui_tex = true;
    }
  }

  HRESULT hr
    = _D3D11Dev_CreateTexture2D_Original ( This, &copy, pInitialData, ppTexture2D );


  if (ui_tex && SUCCEEDED (hr))
  {
    CComQIPtr <ID3D11Resource> pRes (*ppTexture2D);

    mss_ui_render_buffers.insert (pRes);
  }

  return hr;
}

__declspec (noinline)
void
STDMETHODCALLTYPE
SK_MSS_RSSetScissorRects (
        ID3D11DeviceContext *This,
        UINT                 NumRects,
  const D3D11_RECT          *pRects )
{
  if (NumRects > 0)
  {
    D3D11_RECT rects [16] = { };

    CComPtr <ID3D11RenderTargetView> pRTV = nullptr;
    CComPtr <ID3D11DepthStencilView> pDSV = nullptr;

    This->OMGetRenderTargets (1, &pRTV, &pDSV);

    CComPtr <ID3D11Resource> pRes = nullptr;

    if (pRTV != nullptr)
    {
      pRTV->GetResource (&pRes);

      if (mss_ui_render_buffers.count (pRes))
      {
        for (UINT i = 0; i < NumRects; i++)
        {
          float left_ndc = 2.0f * (static_cast <FLOAT> (pRects [i].left) / 1920.0f) - 1.0f;
          float top_ndc  = 2.0f * (static_cast <FLOAT> (pRects [i].top)  / 1080.0f) - 1.0f;

          float right_ndc  = 2.0f * (static_cast <FLOAT> (pRects [i].right)  / 1920.0f) - 1.0f;
          float bottom_ndc = 2.0f * (static_cast <FLOAT> (pRects [i].bottom) / 1080.0f) - 1.0f;

          rects [i].left   = static_cast <UINT> ((left_ndc  * static_cast <float> (mss_res.x) + static_cast <float> (mss_res.x)) / 2.0f);
          rects [i].right  = static_cast <UINT> ((right_ndc * static_cast <float> (mss_res.x) + static_cast <float> (mss_res.x)) / 2.0f);

          rects [i].top    = static_cast <UINT> ((top_ndc    * static_cast <float> (mss_res.y) + static_cast <float> (mss_res.y)) / 2.0f);
          rects [i].bottom = static_cast <UINT> ((bottom_ndc * static_cast <float> (mss_res.y) + static_cast <float> (mss_res.y)) / 2.0f);
        }

        return _D3D11_RSSetScissorRects_Original (This, NumRects, rects);
      }
    }
  }

  return _D3D11_RSSetScissorRects_Original (This, NumRects, pRects);
}

__declspec (noinline)
void
STDMETHODCALLTYPE
SK_MSS_RSSetViewports (
        ID3D11DeviceContext* This,
        UINT                 NumViewports,
  const D3D11_VIEWPORT*      pViewports )
{
  if (NumViewports > 0)
  {
    D3D11_VIEWPORT vps [16] = { };

    CComPtr <ID3D11RenderTargetView> pRTV = nullptr;
    CComPtr <ID3D11DepthStencilView> pDSV = nullptr;

    This->OMGetRenderTargets (1, &pRTV, &pDSV);

    CComPtr <ID3D11Resource> pRes = nullptr;

    if (pRTV != nullptr)
    {
      pRTV->GetResource (&pRes);

      if (mss_ui_render_buffers.count (pRes))
      {
        for (UINT i = 0; i < NumViewports; i++)
        {
          vps [i] = pViewports [i];

          if (vps [i].Width == 1920 && vps [i].Height == 1080)
          {
            float left_ndc = 2.0f * (vps [i].TopLeftX / vps [i].Width)  - 1.0f;
            float top_ndc  = 2.0f * (vps [i].TopLeftY / vps [i].Height) - 1.0f;

            vps [i].Width  = static_cast <FLOAT> (mss_res.x);
            vps [i].Height = static_cast <FLOAT> (mss_res.y);

            vps [i].TopLeftX = ((top_ndc  * vps [i].Height + vps [i].Height) / 2.0f);
            vps [i].TopLeftY = ((left_ndc * vps [i].Width  + vps [i].Width)  / 2.0f);
          }
        }

        D3D11_RSSetViewports_Original (This, NumViewports, vps);

        return;
      }
    }
  }

  D3D11_RSSetViewports_Original (This, NumViewports, pViewports);
}


void
__stdcall
SK_MSS_ControlPanel (void)
{
  if (ImGui::CollapsingHeader ("LEGO MARVEL Super Heroes 2", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::TreePush ("");

    ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.40f, 0.40f, 0.45f));
    ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.45f, 0.45f, 0.80f));
    ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.53f, 0.53f, 0.80f));

    if (ImGui::CollapsingHeader ("UI Resolution", ImGuiTreeNodeFlags_DefaultOpen))
    {
      static bool need_reset = false;
             bool changed    = false;

      ImGui::TreePush ("");

      changed |= ImGui::InputInt2 ("UI Resolution###MSS", &mss_res.x);

      if (changed)
      {
        mss_config.ui.res_x->store (mss_res.x);
        mss_config.ui.res_y->store (mss_res.y);

        SK_GetDLLConfig ()->write (SK_GetDLLConfig ()->get_filename ());

        need_reset = true;
      }

      if (need_reset)
        ImGui::BulletText ("Setting changes require application restart.");

      ImGui::TreePop  ();
    }

    ImGui::PopStyleColor (3);
    ImGui::TreePop       ( );
  }
}



HRESULT
STDMETHODCALLTYPE
SK_MSS_PresentFirstFrame (IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
  UNREFERENCED_PARAMETER (pSwapChain);
  UNREFERENCED_PARAMETER (SyncInterval);
  UNREFERENCED_PARAMETER (Flags);

  while (! InterlockedAdd (&__MSS_init, 0)) SleepEx (16, FALSE);

  return S_OK;
}

void
SK_MSS_InitPlugin (void)
{
  SK_SetPluginName (MSS_VERSION_STR);

  SK_CreateFuncHook (       L"ID3D11DeviceContext::RSSetViewports",
                               D3D11_RSSetViewports_Override,
                                 SK_MSS_RSSetViewports,
     static_cast_p2p <void> (&_D3D11_RSSetViewports_Original) );
  MH_QueueEnableHook (         D3D11_RSSetViewports_Override  );
  SK_CreateFuncHook (       L"ID3D11DeviceContext::RSSetScissorRects",
                               D3D11_RSSetScissorRects_Override,
                                 SK_MSS_RSSetScissorRects,
     static_cast_p2p <void> (&_D3D11_RSSetScissorRects_Original) );
  MH_QueueEnableHook (         D3D11_RSSetScissorRects_Override  );

  SK_CreateFuncHook (       L"ID3D11Device::CreateTexture2D",
                               D3D11Dev_CreateTexture2D_Override,
                                 SK_MSS_CreateTexture2D,
     static_cast_p2p <void> (&_D3D11Dev_CreateTexture2D_Original) );
  MH_QueueEnableHook (         D3D11Dev_CreateTexture2D_Override  );

  SK_CreateFuncHook (       L"SK_PlugIn_ControlPanelWidget",
                              SK_PlugIn_ControlPanelWidget,
                                  SK_MSS_ControlPanel,
     static_cast_p2p <void> (&SK_PlugIn_ControlPanelWidget_Original) );
  MH_QueueEnableHook (        SK_PlugIn_ControlPanelWidget           );


  mss_config.ui.res_x =
    dynamic_cast <sk::ParameterInt *>
      (mss_config.factory.create_parameter <int> (L"X Resolution"));

  mss_config.ui.res_y =
    dynamic_cast <sk::ParameterInt *>
      (mss_config.factory.create_parameter <int> (L"Y Resolution"));

  mss_config.ui.res_x->register_to_ini ( SK_GetDLLConfig (),
                                           L"MSS.UI",
                                             L"ResX" );
  mss_config.ui.res_y->register_to_ini ( SK_GetDLLConfig (),
                                           L"MSS.UI",
                                             L"ResY" );

  mss_config.ui.res_x->load (mss_res.x);
  mss_config.ui.res_y->load (mss_res.y);

   SK_ApplyQueuedHooks ();

  InterlockedExchange (&__MSS_init, 1);
};