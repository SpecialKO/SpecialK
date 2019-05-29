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
#include <SpecialK/render/d3d11/d3d11_core.h>


#define IT_VERSION_NUM L"0.0.5"
#define IT_VERSION_STR L"Indigo Translation v " IT_VERSION_NUM

volatile LONG __IT_init = FALSE;

static D3D11Dev_CreateBuffer_pfn              _D3D11Dev_CreateBuffer_Original              = nullptr;
static D3D11Dev_CreateShaderResourceView_pfn  _D3D11Dev_CreateShaderResourceView_Original  = nullptr;
static D3D11Dev_CreateTexture2D_pfn           _D3D11Dev_CreateTexture2D_Original           = nullptr;
static D3D11_Unmap_pfn                        _D3D11_Unmap_Original                        = nullptr;
static D3D11_Map_pfn                          _D3D11_Map_Original                          = nullptr;

struct it_cfg_s
{
  sk::ParameterFactory factory;

  struct godrays_s
  {
    sk::ParameterFloat* min = nullptr;
    sk::ParameterFloat* max = nullptr;
  } godrays;

  struct shadows_s
  {
    sk::ParameterFloat* min_bias = nullptr;
  } shadows;
};

SK_LazyGlobal <it_cfg_s> it_config;

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

extern HRESULT
STDMETHODCALLTYPE
D3D11_Map_Override (
   _In_ ID3D11DeviceContext      *This,
   _In_ ID3D11Resource           *pResource,
   _In_ UINT                      Subresource,
   _In_ D3D11_MAP                 MapType,
   _In_ UINT                      MapFlags,
_Out_opt_ D3D11_MAPPED_SUBRESOURCE *pMappedResource );

extern void
STDMETHODCALLTYPE
D3D11_Unmap_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ ID3D11Resource      *pResource,
  _In_ UINT                 Subresource );

extern
HRESULT
WINAPI
D3D11Dev_CreateBuffer_Override (
  _In_           ID3D11Device            *This,
  _In_     const D3D11_BUFFER_DESC       *pDesc,
  _In_opt_ const D3D11_SUBRESOURCE_DATA  *pInitialData,
  _Out_opt_      ID3D11Buffer           **ppBuffer );

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

struct light_shaft_s {
  float pos      [4];
  float power    [4];
  float img_size [2];
  float weights  [4][4];
};

struct shaft_min_max_s
{
  float min_intensity = 0.0f;
  float max_intensity = 30.0f;
};

SK_LazyGlobal <shaft_min_max_s> shaft;
SK_LazyGlobal <light_shaft_s>   shaft_prefs;

float fSteps [3] =  { 0.0002f, 0.0004f, 0.001f };

float fBiasMultipliers [3][4] = { { 1.3000f, 1.1000f, 1.0000f, 0.6800f },
                                  { 2.1000f, 1.3400f, 1.0910f, 1.3483f },
                                  { 1.1500f, 1.2600f, 1.0000f, 0.9300f } };

std::array <std::map <ID3D11Resource*, D3D11_MAPPED_SUBRESOURCE>, SK_D3D11_MAX_DEV_CONTEXTS> _mapped_shafts (void)
{
  static std::array <std::map <ID3D11Resource*, D3D11_MAPPED_SUBRESOURCE>, SK_D3D11_MAX_DEV_CONTEXTS> mapped_shafts;

  return mapped_shafts;
}

std::array <std::map <ID3D11Resource*, D3D11_MAPPED_SUBRESOURCE>, SK_D3D11_MAX_DEV_CONTEXTS> _mapped_shadows (void)
{
  static std::array <std::map <ID3D11Resource*, D3D11_MAPPED_SUBRESOURCE>, SK_D3D11_MAX_DEV_CONTEXTS> mapped_shadows;

  return mapped_shadows;
}

#define mapped_shafts  _mapped_shafts()
#define mapped_shadows _mapped_shadows()

bool  dump_bias       = false;
float min_shadow_bias = 0.0f;

HRESULT
STDMETHODCALLTYPE
SK_IT_Map (
   _In_ ID3D11DeviceContext        *This,
   _In_ ID3D11Resource             *pResource,
   _In_ UINT                        Subresource,
   _In_ D3D11_MAP                   MapType,
   _In_ UINT                        MapFlags,
_Out_opt_ D3D11_MAPPED_SUBRESOURCE *pMappedResource )
{
  D3D11_MAPPED_SUBRESOURCE desc;

  if (! pMappedResource)
    pMappedResource = &desc;

  HRESULT hr =
    _D3D11_Map_Original (This, pResource, Subresource, MapType, MapFlags, pMappedResource);

  if (SUCCEEDED (hr))
  {
    int dev_idx = SK_D3D11_GetDeviceContextHandle (This);

    if (pMappedResource != &desc)
      desc = *pMappedResource;

    D3D11_RESOURCE_DIMENSION rDim;

    pResource->GetType (&rDim);

    if (rDim == D3D11_RESOURCE_DIMENSION_BUFFER)
    {
      CComQIPtr <ID3D11Buffer> pBuffer (pResource);

      D3D11_BUFFER_DESC  buffer_desc;
      pBuffer->GetDesc (&buffer_desc);

      if (buffer_desc.ByteWidth == sizeof (light_shaft_s) + 8)
      {
        mapped_shafts [dev_idx].emplace (std::make_pair (pResource, *pMappedResource));
      }

      if (buffer_desc.ByteWidth == 16)
      {
        mapped_shadows [dev_idx].emplace (std::make_pair (pResource, *pMappedResource));
      }
    }
  }

  return hr;
}


void
STDMETHODCALLTYPE
SK_IT_Unmap (
  _In_ ID3D11DeviceContext *This,
  _In_ ID3D11Resource      *pResource,
  _In_ UINT                 Subresource)
{
  D3D11_RESOURCE_DIMENSION rDim;

  pResource->GetType (&rDim);

  if (rDim == D3D11_RESOURCE_DIMENSION_BUFFER)
  {
    int dev_idx = SK_D3D11_GetDeviceContextHandle (This);

    if (mapped_shafts [dev_idx].count (pResource))
    {
      auto * pShaft =
     (light_shaft_s *)mapped_shafts [dev_idx][pResource].pData;

      if (! pShaft)
        return;

      //dll_log.Log ( L"Original light shaft resolution: (%.6fx%.6f)",
      //                pShaft->img_size [0], pShaft->img_size [1] );

      auto& DisplaySize = ImGui::GetIO ().DisplaySize;

      if ( pShaft->img_size [0] >= DisplaySize.x - 0.1 &&
           pShaft->img_size [0] <= DisplaySize.x + 0.1 &&
           pShaft->img_size [1] <= DisplaySize.y + 0.1 &&
           pShaft->img_size [1] >= DisplaySize.y - 0.1 )
      {
        if (pShaft->power [3] > shaft->max_intensity)
            pShaft->power [3] = shaft->max_intensity;
        if (pShaft->power [3] < shaft->min_intensity)
            pShaft->power [3] = shaft->min_intensity;

        *shaft_prefs = *pShaft;
      }

      mapped_shafts [dev_idx].erase (pResource);
    }

    else if (mapped_shadows [dev_idx].count (pResource))
    {
      if (SK_D3D11_Shaders->pixel.current.shader [dev_idx] == 0x2117b8e3)
      {
        auto* pShadow =
          static_cast <float *> (mapped_shadows [dev_idx][pResource].pData);

        if (dump_bias)
        {
          dll_log->Log (L"Shadowmap Bias: %f, %f, %f, %f",
                          pShadow [0], pShadow [1], pShadow [2], pShadow [3]);
        }

        // Landscape shadows (need an increased bias)
        if ( (pShadow [1] == fSteps [0]
           || pShadow [1] == fSteps [1]
           || pShadow [1] == fSteps [2]) && min_shadow_bias != 0.0f )
        {
          if (pShadow [1] == fSteps [0])
          {
            for (int i = 0 ; i < 4; i++)
            {
              pShadow [i] *= fBiasMultipliers [0][i];
            }
          }

          if (pShadow [1] == fSteps [1])
          {
            for (int i = 0 ; i < 4; i++)
            {
              pShadow [i] *= fBiasMultipliers [1][i];
            }
          }

          if (pShadow [1] == fSteps [2])
          {
            for (int i = 0 ; i < 4; i++)
            {
              pShadow [i] *= fBiasMultipliers [2][i];
            }
          }
        }

        if (pShadow [1] * pShadow [3] < min_shadow_bias)
          pShadow [1] = min_shadow_bias / pShadow [3];
      }

      mapped_shadows [dev_idx].erase (pResource);
    }
  }

  _D3D11_Unmap_Original (This, pResource, Subresource);
}

HRESULT
WINAPI
SK_IT_CreateBuffer (
  _In_           ID3D11Device            *This,
  _In_     const D3D11_BUFFER_DESC       *pDesc,
  _In_opt_ const D3D11_SUBRESOURCE_DATA  *pInitialData,
  _Out_opt_      ID3D11Buffer           **ppBuffer )
{
  D3D11_SUBRESOURCE_DATA new_data =
    ( (pInitialData != nullptr)  ?  (*pInitialData)  :
                                      D3D11_SUBRESOURCE_DATA { } );

  D3D11_BUFFER_DESC      new_desc =
    ( (pDesc        != nullptr)  ?  (*pDesc)         :
                                      D3D11_BUFFER_DESC      { } );

  if (ppBuffer != nullptr && pDesc != nullptr)
  {
  }

  else if (pDesc != nullptr)
  {
    //dll_log.Log (L"Byte Width: %lu, Stride: %lu", pDesc->ByteWidth, pDesc->StructureByteStride);
  }

  return _D3D11Dev_CreateBuffer_Original ( This, pDesc, pInitialData, ppBuffer );
}

__declspec (noinline)
HRESULT
WINAPI
SK_IT_CreateTexture2D (
  _In_            ID3D11Device           *This,
  _In_      const D3D11_TEXTURE2D_DESC   *pDesc,
  _In_opt_  const D3D11_SUBRESOURCE_DATA *pInitialData,
  _Out_opt_       ID3D11Texture2D        **ppTexture2D )
{
  if (ppTexture2D == nullptr || pDesc == nullptr)
    return _D3D11Dev_CreateTexture2D_Original ( This, pDesc, pInitialData, ppTexture2D);

  D3D11_TEXTURE2D_DESC copy (*pDesc);

  //switch (pDesc->Format)
  //{
  //  case DXGI_FORMAT_R11G11B10_FLOAT:
  //  {
  //    if (pDesc->BindFlags & D3D11_BIND_RENDER_TARGET)
  //    {
  //      if (pDesc->Width == 1280 && pDesc->Height == 720)
  //      {
  //        copy.Width  = 3840;
  //        copy.Height = 2160;
  //
  //        pDesc = &copy;
  //      }
  //    }
  //  } break;
  //
  //  default:
  //    break;
  //};

  return _D3D11Dev_CreateTexture2D_Original ( This, pDesc, pInitialData, ppTexture2D );
}


void
__stdcall
SK_IT_ControlPanel (void)
{
  dump_bias = false;

  if (ImGui::CollapsingHeader ("Blue Reflection", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::TreePush ("");

    ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.40f, 0.40f, 0.45f));
    ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.45f, 0.45f, 0.80f));
    ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.53f, 0.53f, 0.80f));

    if (ImGui::CollapsingHeader ("God Rays", ImGuiTreeNodeFlags_DefaultOpen))
    {
      bool changed = false;

      ImGui::TreePush ("");

      changed |= ImGui::SliderFloat ("Minimum Godray Intensity", &shaft->min_intensity, 0.0f,  shaft->max_intensity);
      changed |= ImGui::SliderFloat ("Maximum Godray Intensity", &shaft->max_intensity, shaft->min_intensity, 30.0f);
      changed |= ImGui::SliderFloat ("Current Godray Intensity", &shaft_prefs->power [3], shaft->min_intensity, shaft->max_intensity);

      if (changed)
      {
        it_config->godrays.max->store (shaft->max_intensity);
        it_config->godrays.min->store (shaft->min_intensity);

        SK_GetDLLConfig ()->write (SK_GetDLLConfig ()->get_filename ());
      }

      ImGui::TreePop  ();
    }

    if (ImGui::CollapsingHeader ("Shadows", ImGuiTreeNodeFlags_DefaultOpen))
    {
      ImGui::TreePush ("");

      bool changed = false;
      bool enhance = false;
      enhance = (min_shadow_bias == 0.000133f);

      if (ImGui::Checkbox ("Reduce Shadowmap Aliasing", &enhance))
      {
        changed = true;
        min_shadow_bias = enhance ? 0.000133f : 0.0f;
      }

      if (changed)
      {
        it_config->shadows.min_bias->store (min_shadow_bias);

        SK_GetDLLConfig ()->write (SK_GetDLLConfig ()->get_filename ());
      }

      if (config.system.log_level > 0)
      {
        if (ImGui::Button ("Trace Shadowmap Biases in Log"))
        {
          dump_bias = true;
        }

        ImGui::SliderFloat ("Minimum Shadow Bias", &min_shadow_bias, 0.0f, 0.001f);

        ImGui::InputFloat3 ("Shadowmap Steps", fSteps);

        ImGui::InputFloat4 ("Bias Multipliers (LOD0)", fBiasMultipliers [0]);
        ImGui::InputFloat4 ("Bias Multipliers (LOD1)", fBiasMultipliers [1]);
        ImGui::InputFloat4 ("Bias Multipliers (LOD2)", fBiasMultipliers [2]);
      }


      //ImGui::SliderFloat ("Minimum Shadow Bias", &min_shadow_bias, 0.0f, 0.001f);

      ImGui::TreePop  ();
    }

    ImGui::PopStyleColor (3);
    ImGui::TreePop ();
  }
}



HRESULT
STDMETHODCALLTYPE
SK_IT_PresentFirstFrame (IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
  UNREFERENCED_PARAMETER (pSwapChain);
  UNREFERENCED_PARAMETER (SyncInterval);
  UNREFERENCED_PARAMETER (Flags);

  while (! InterlockedAdd (&__IT_init, 0)) SK_Sleep (16);

  return S_OK;
}

void
SK_IT_InitPlugin (void)
{
  SK_SetPluginName (IT_VERSION_STR);

  //SK_CreateFuncHook (       L"ID3D11Device::CreateBuffer",
  //                             D3D11Dev_CreateBuffer_Override,
  //                               SK_IT_CreateBuffer,
  //   static_cast_p2p <void> (&_D3D11Dev_CreateBuffer_Original) );
  //MH_QueueEnableHook (         D3D11Dev_CreateBuffer_Override  );

  SK_CreateFuncHook (       L"ID3D11Device::CreateTexture2D",
                               D3D11Dev_CreateTexture2D_Override,
                                 SK_IT_CreateTexture2D,
     static_cast_p2p <void> (&_D3D11Dev_CreateTexture2D_Original) );
  MH_QueueEnableHook (         D3D11Dev_CreateTexture2D_Override  );

  SK_CreateFuncHook (       L"ID3D11Device::Map",
                                  D3D11_Map_Override,
                                  SK_IT_Map,
        static_cast_p2p <void> (&_D3D11_Map_Original) );
  MH_QueueEnableHook (            D3D11_Map_Override  );

  SK_CreateFuncHook (    L"ID3D11Device::Unmap",
                               D3D11_Unmap_Override,
                               SK_IT_Unmap,
     static_cast_p2p <void> (&_D3D11_Unmap_Original) );
  MH_QueueEnableHook (         D3D11_Unmap_Override  );

  SK_CreateFuncHook (       L"SK_PlugIn_ControlPanelWidget",
                              SK_PlugIn_ControlPanelWidget,
                                  SK_IT_ControlPanel,
     static_cast_p2p <void> (&SK_PlugIn_ControlPanelWidget_Original) );
  MH_QueueEnableHook (        SK_PlugIn_ControlPanelWidget           );


  auto& _it_config = it_config.get ();

  _it_config.godrays.min =
      dynamic_cast <sk::ParameterFloat *>
        (_it_config.factory.create_parameter <float> (L"Minimum Godray Intensity"));

  _it_config.godrays.max =
      dynamic_cast <sk::ParameterFloat *>
        (_it_config.factory.create_parameter <float> (L"Maximum Godray Intensity"));

  _it_config.godrays.min->register_to_ini ( SK_GetDLLConfig (),
                                              L"Indigo.Godrays",
                                                L"MinimumIntensity" );
  _it_config.godrays.max->register_to_ini ( SK_GetDLLConfig (),
                                              L"Indigo.Godrays",
                                                L"MaximumIntensity" );

  _it_config.godrays.min->load (shaft->min_intensity);
  _it_config.godrays.max->load (shaft->max_intensity);

  _it_config.shadows.min_bias =
      dynamic_cast <sk::ParameterFloat *>
        (_it_config.factory.create_parameter <float> (L"Minimum Shadowmap Bias"));

  _it_config.shadows.min_bias->register_to_ini ( SK_GetDLLConfig (),
                                                   L"Indigo.Shadows",
                                                     L"MinimumBias" );

  if (! _it_config.shadows.min_bias->load (min_shadow_bias))
    min_shadow_bias = 0.000133f;


  //SK_CreateFuncHook ( L"SK_ImGUI_DrawEULA_PlugIn",
  //                      SK_ImGui_DrawEULA_PlugIn,
   //                            SK_IT_EULA_Insert,
  //                              &dontcare     );
  //
  //MH_QueueEnableHook (SK_ImGui_DrawEULA_PlugIn);

   SK_ApplyQueuedHooks ();

  InterlockedExchange (&__IT_init, 1);
};