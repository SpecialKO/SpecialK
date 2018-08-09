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

#include <SpecialK/render/dxgi/dxgi_backend.h>
#include <SpecialK/core.h>
#include <SpecialK/log.h>
#include <SpecialK/ini.h>
#include <SpecialK/hooks.h>
#include <SpecialK/config.h>
#include <SpecialK/utility.h>
#include <SpecialK/command.h>
#include <SpecialK/parameter.h>
#include <SpecialK/framerate.h>
#include <SpecialK/steam_api.h>

#include <SpecialK/input/input.h>
#include <SpecialK/input/xinput.h>

#include <SpecialK/hooks.h>
#include <SpecialK/core.h>
#include <process.h>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_d3d11.h>

#include <atlbase.h>

#define SOM_VERSION_NUM L"0.0.1"
#define SOM_VERSION_STR L"Secret of Mantrap v " SOM_VERSION_NUM

volatile LONG __SOM_init = FALSE;

extern void
__stdcall
SK_SetPluginName (std::wstring name);

static D3D11Dev_CreateShaderResourceView_pfn  _D3D11Dev_CreateShaderResourceView_Original  = nullptr;
static D3D11Dev_CreateTexture2D_pfn           _D3D11Dev_CreateTexture2D_Original           = nullptr;

struct som_cfg_s
{
  sk::ParameterFactory factory;

  struct shadows_s
  {
    sk::ParameterFloat* scale = nullptr;
  } shadows;
} som_config;

static float shadow_scale = 1.0f;


using D3D11Dev_CreateTexture2D_pfn = HRESULT (WINAPI *)(
  _In_            ID3D11Device            *This,
  _In_      const D3D11_TEXTURE2D_DESC    *pDesc,
  _In_opt_  const D3D11_SUBRESOURCE_DATA  *pInitialData,
  _Out_opt_       ID3D11Texture2D        **ppTexture2D
);

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

void __stdcall SK_SOM_ControlPanel (void);

static SK_PlugIn_ControlPanelWidget_pfn SK_PlugIn_ControlPanelWidget_Original = nullptr;

__declspec (noinline)
HRESULT
WINAPI
SK_SOM_CreateTexture2D (
  _In_            ID3D11Device           *This,
  _In_      const D3D11_TEXTURE2D_DESC   *pDesc,
  _In_opt_  const D3D11_SUBRESOURCE_DATA *pInitialData,
  _Out_opt_       ID3D11Texture2D        **ppTexture2D )
{
  if (ppTexture2D == nullptr || pDesc == nullptr)
    return _D3D11Dev_CreateTexture2D_Original ( This, pDesc, pInitialData, ppTexture2D);

  D3D11_TEXTURE2D_DESC copy (*pDesc);

  switch (pDesc->Format)
  {
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
    {
      //if (pDesc->BindFlags & D3D11_BIND_RENDER_TARGET)
      {
        if (pDesc->Width == 2048 && pDesc->Height == 2048)
        {
          copy.Width  = UINT (static_cast <float> (copy.Width)  * shadow_scale);
          copy.Height = UINT (static_cast <float> (copy.Height) * shadow_scale);

          pDesc = &copy;
        }
      }
    } break;
  
    default:
      break;
  };

  return _D3D11Dev_CreateTexture2D_Original ( This, pDesc, pInitialData, ppTexture2D );
}

void
__stdcall
SK_SOM_ControlPanel (void)
{
  if (ImGui::CollapsingHeader ("Secret of Mana", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::TreePush ("");

    ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.40f, 0.40f, 0.45f));
    ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.45f, 0.45f, 0.80f));
    ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.53f, 0.53f, 0.80f));

    if (ImGui::CollapsingHeader ("Shadows", ImGuiTreeNodeFlags_DefaultOpen))
    {
      ImGui::TreePush ("");

      bool changed = false;
      bool enhance = false;
      enhance = (shadow_scale == 2.0f);

      if (ImGui::Checkbox ("Reduce Shadowmap Aliasing", &enhance))
      {
        changed = true;
        som_config.shadows.scale->store (enhance ? 2.0f : 1.0f);

        shadow_scale = enhance ? 2.0f : 1.0f;
      }

      if (changed)
      {
        SK_GetDLLConfig ()->write (SK_GetDLLConfig ()->get_filename ());
      }

      ImGui::TreePop  ();
    }

    ImGui::PopStyleColor (3);
    ImGui::TreePop ();
  }
}



HRESULT
STDMETHODCALLTYPE
SK_SOM_PresentFirstFrame (IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
  UNREFERENCED_PARAMETER (pSwapChain);
  UNREFERENCED_PARAMETER (SyncInterval);
  UNREFERENCED_PARAMETER (Flags);

  while (! InterlockedAdd (&__SOM_init, 0)) SleepEx (16, FALSE);

  return S_OK;
}

void
SK_SOM_InitPlugin (void)
{
  SK_SetPluginName (SOM_VERSION_STR);

  SK_CreateFuncHook (       L"ID3D11Device::CreateTexture2D",
                               D3D11Dev_CreateTexture2D_Override,
                                 SK_SOM_CreateTexture2D,
     static_cast_p2p <void> (&_D3D11Dev_CreateTexture2D_Original) );
  MH_QueueEnableHook (         D3D11Dev_CreateTexture2D_Override  );

  SK_CreateFuncHook (       L"SK_PlugIn_ControlPanelWidget",
                              SK_PlugIn_ControlPanelWidget,
                                 SK_SOM_ControlPanel,
     static_cast_p2p <void> (&SK_PlugIn_ControlPanelWidget_Original) );
  MH_QueueEnableHook (        SK_PlugIn_ControlPanelWidget           );

  
  som_config.shadows.scale =
      dynamic_cast <sk::ParameterFloat *>
        (som_config.factory.create_parameter <float> (L"Shadow Rescale"));

  som_config.shadows.scale->register_to_ini ( SK_GetDLLConfig (),
                                                L"SecretOfMana.Shadows",
                                                  L"Scale" );

  som_config.shadows.scale->load (shadow_scale);

  SK_ApplyQueuedHooks ();

  InterlockedExchange (&__SOM_init, 1);
};