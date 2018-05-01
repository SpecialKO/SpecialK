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
#include <SpecialK/tls.h>
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

#include <cinttypes>

#define YS8_VERSION_NUM L"0.2.0"
#define YS8_VERSION_STR L"Ys8 Fixin' Stuff v " YS8_VERSION_NUM

volatile LONG __YS8_init = FALSE;

extern void
__stdcall
SK_SetPluginName (std::wstring name);


using SK_PlugIn_ControlPanelWidget_pfn = void (__stdcall *)(void);

static D3D11Dev_CreateShaderResourceView_pfn  _D3D11Dev_CreateShaderResourceView_Original = nullptr;


static D3D11Dev_CreateSamplerState_pfn        _D3D11Dev_CreateSamplerState_Original       = nullptr;
static D3D11Dev_CreateTexture2D_pfn           _D3D11Dev_CreateTexture2D_Original          = nullptr;
static D3D11_RSSetViewports_pfn               _D3D11_RSSetViewports_Original              = nullptr;

static SK_PlugIn_ControlPanelWidget_pfn       SK_PlugIn_ControlPanelWidget_Original       = nullptr;



struct ys8_cfg_s
{
  sk::ParameterFactory factory;

  struct shadows_s
  {
    sk::ParameterFloat* scale = nullptr;
  } shadows;

  struct mipmaps_s
  {
    sk::ParameterBool* generate    = nullptr;
    sk::ParameterBool* cache       = nullptr;
    sk::ParameterInt*  stream_rate = nullptr;
  } mipmaps;
} ys8_config;




bool disable_negative_lod_bias = true;
int  max_anisotropy            = 16;
bool anisotropic_filter        = true;

float explicit_lod_bias = 0.0f;
bool  nearest_neighbor  = false;


int SK_D3D11_TexStreamPriority = 1; // 0=Low (Pop-In), 1=Balanced, 2=High (No Pop-In)



extern
HRESULT
WINAPI
D3D11Dev_CreateSamplerState_Override (
  _In_            ID3D11Device        *This,
  _In_      const D3D11_SAMPLER_DESC  *pSamplerDesc,
  _Out_opt_       ID3D11SamplerState **ppSamplerState );

extern 
HRESULT
WINAPI
D3D11Dev_CreateTexture2D_Override (
  _In_            ID3D11Device           *This,
  _In_      const D3D11_TEXTURE2D_DESC   *pDesc,
  _In_opt_  const D3D11_SUBRESOURCE_DATA *pInitialData,
  _Out_opt_       ID3D11Texture2D        **ppTexture2D );

extern 
void
WINAPI
D3D11_RSSetViewports_Override (
        ID3D11DeviceContext* This,
        UINT                 NumViewports,
  const D3D11_VIEWPORT*      pViewports );

extern
void
__stdcall
SK_PlugIn_ControlPanelWidget (void);


float __SK_YS8_ShadowScale  = 1.0f;
float __SK_YS8_ShadowScale0 = 1.0f; // Initial value

Concurrency::concurrent_unordered_set <ID3D11Texture2D *> __SK_YS8_RescaledTextures;


void
__stdcall
SK_YS8_ControlPanel (void)
{
  if (ImGui::CollapsingHeader ("Ys VIII Lacrimosa of Dana", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::TreePush ("");

    ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.40f, 0.40f, 0.45f));
    ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.45f, 0.45f, 0.80f));
    ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.53f, 0.53f, 0.80f));

    bool changed = false;

    extern LONG SK_D3D11_Resampler_GetActiveJobCount  (void);
    extern LONG SK_D3D11_Resampler_GetWaitingJobCount (void);
  //extern LONG SK_D3D11_Resampler_GetRetiredCount    (void);
    extern LONG SK_D3D11_Resampler_GetErrorCount      (void);

    bool tex_manage = ImGui::CollapsingHeader ("Texture Management##Ys8", ImGuiTreeNodeFlags_DefaultOpen);

    LONG jobs = SK_D3D11_Resampler_GetActiveJobCount () + SK_D3D11_Resampler_GetWaitingJobCount ();

    static DWORD dwLastActive = 0;

    if (jobs != 0 || dwLastActive > timeGetTime () - 500)
    {
      if (jobs > 0)
        dwLastActive = timeGetTime ();

      if (jobs > 0)
        ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.4f - (0.4f * (SK_D3D11_Resampler_GetActiveJobCount ()) / (float)jobs), 0.15f, 1.0f));
      else
        ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.4f - (0.4f * (timeGetTime () - dwLastActive) / 500.0f), 1.0f, 0.8f));

      ImGui::SameLine       ();
      if (SK_D3D11_Resampler_GetErrorCount ())
        ImGui::Text         ("       Textures ReSampling (%li / %li) { Error Count: %li }",
                               SK_D3D11_Resampler_GetActiveJobCount (),
                                 jobs,
                               SK_D3D11_Resampler_GetErrorCount     ()
                            );
      else
        ImGui::Text         ("       Textures ReSampling (%li / %li)",
                               SK_D3D11_Resampler_GetActiveJobCount (),
                                 jobs );
      ImGui::PopStyleColor  ();
    }

    if (tex_manage)
    {
      ImGui::TreePush    ("");
      ImGui::BeginGroup  ();
      changed |= ImGui::Checkbox    ("Generate Mipmaps", &config.textures.d3d11.generate_mips);

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip    ();
        ImGui::PushStyleColor  (ImGuiCol_Text, ImColor::HSV (0.5f, 0.f, 1.f, 1.f));
        ImGui::TextUnformatted ("Builds Complete Mipchains (Mipmap LODs) for all Textures");
        ImGui::Separator       ();
        ImGui::PopStyleColor   ();
        ImGui::Bullet          (); ImGui::SameLine ();
        ImGui::PushStyleColor  (ImGuiCol_Text, ImColor::HSV (0.15f, 1.0f, 1.0f));
        ImGui::TextUnformatted ("SIGNIFICANTLY");
        ImGui::PopStyleColor   (); ImGui::SameLine ();
        ImGui::TextUnformatted ("reduces texture aliasing");
        ImGui::BulletText      ("May increase load-times and/or hurt compatibility with third-party software");
        ImGui::EndTooltip      ();
      }

      ImGui::SameLine ();

      if (
        ImGui::Combo ("Texture Streaming Priority", &SK_D3D11_TexStreamPriority, "Short load screens; (Pop-In)\0"
                                                                                 "Balanced\0"
                                                                                 "Long load screens; (Anti-Pop-In)\0\0")
      )
      {
        ys8_config.mipmaps.stream_rate->store (SK_D3D11_TexStreamPriority);
        changed = true;
      }

      //if (config.textures.d3d11.generate_mips)
      //{
      //  extern uint64_t SK_D3D11_MipmapCacheSize;
      //  
      //  changed |= ImGui::Checkbox ("Cache Mipmaps to Disk", &config.textures.d3d11.cache_gen_mips);
      //  
      //  if (ImGui::IsItemHovered ())
      //    ImGui::SetTooltip ("Eliminates possible texture pop-in, but will slow down loading and consume disk space.");
      //  
      //  ImGui::EndGroup        ();
      //  ImGui::SameLine        ();
      //  ImGui::BeginGroup      ();
      //  
      //  if (ImGui::Button   (" Purge Cache "))
      //  {
      //    wchar_t wszPath [ MAX_PATH + 2 ] = { };
      //  
      //    wcscpy ( wszPath,
      //               SK_EvalEnvironmentVars (config.textures.d3d11.res_root.c_str ()).c_str () );
      //  
      //    lstrcatW (wszPath, L"/inject/textures/MipmapCache/");
      //    lstrcatW (wszPath, SK_GetHostApp ());
      //    lstrcatW (wszPath, L"/");
      //  
      //    SK_D3D11_MipmapCacheSize -= SK_DeleteTemporaryFiles (wszPath, L"*.dds");
      //    SK_D3D11_MipmapCacheSize  = 0;
      //  }
      //
      //  ImGui::SameLine ();
      //  ImGui::Text     ("Current Cache Size: %.2f MiB", (double)SK_D3D11_MipmapCacheSize / (1024.0 * 1024.0));
      //  ImGui::EndGroup ();
      //}
      //
      //else
        ImGui::EndGroup ();

      ImGui::Separator  ();

      ImGui::Checkbox ("Anisotropic Filtering", &anisotropic_filter);

      if (anisotropic_filter) { ImGui::SameLine (); ImGui::SliderInt ("Maximum Anisotropy", &max_anisotropy, 1, 16); }

      ImGui::Checkbox ("Disable Negative LOD Bias", &disable_negative_lod_bias);

      ImGui::SameLine   ();
      ImGui::BeginGroup ();

      if (ImGui::TreeNode ("Debug Settings###SK_YS8_DEBUG"))
      {
        ImGui::Checkbox    ("Point-Sampling",    &nearest_neighbor);
        ImGui::SliderFloat ("Explicit LOD Bias", &explicit_lod_bias, -3.f, 3.f);
        ImGui::TreePop     ();
      }
      ImGui::EndGroup    ();
      ImGui::TreePop     ();
    }

    if (ImGui::CollapsingHeader ("Shadows", ImGuiTreeNodeFlags_DefaultOpen))
    {
      ImGui::TreePush ("");

      static
        std::unordered_map <int, float> fwd_sel {
          { 0l, 1.f }, { 1l, 2.f },
          { 2l, 3.f }, { 3l, 4.f }
        };

      static
        std::unordered_map <float, int> rev_sel {
          { 1.f, 0l }, { 2.f, 1l },
          { 3.f, 2l }, { 4.f, 3l }
        };

      static int sel =
             rev_sel.count ( __SK_YS8_ShadowScale ) ?
             rev_sel [       __SK_YS8_ShadowScale]  : 0;

      static int orig_sel = sel;

      if ( ImGui::Combo ( "Shadow Resolution###SK_YS8_ShadowRes", &sel,
                            "Normal\0"
                            "High\0"
                            "Very High\0"
                            "Ultra\0\0" ) )
      {
        changed = true;

        ys8_config.shadows.scale->store (
          (__SK_YS8_ShadowScale = fwd_sel [sel])
        );
      }

      if (orig_sel != sel)
      {
        ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (.3f, .8f, .9f));
        ImGui::BulletText     ("Game Restart Required");
        ImGui::PopStyleColor  ();
      }

      ImGui::TreePop  ();
    }

    if (changed)
    {
      iSK_INI* pINI =
        SK_GetDLLConfig ();

      pINI->write (pINI->get_filename ());
    }

    ImGui::PopStyleColor (3);
    ImGui::TreePop       ( );
  }
}



__forceinline uint32_t
SK_NextPowerOfTwo (uint32_t n)
{
   n--;    n |= n >> 1;
           n |= n >> 2;   n |= n >> 4;
           n |= n >> 8;   n |= n >> 16;

  return ++n;
}


__declspec (noinline)
void
STDMETHODCALLTYPE
SK_YS8_RSSetViewports (
        ID3D11DeviceContext* This,
        UINT                 NumViewports,
  const D3D11_VIEWPORT*      pViewports )
{
  if (NumViewports > 0)
  {
    // Safer than alloca
    D3D11_VIEWPORT* vps =
   (D3D11_VIEWPORT *)SK_TLS_Bottom ()->scratch_memory.cmd.alloc (
                                         sizeof (D3D11_VIEWPORT) * NumViewports
                     );

    for (UINT i = 0; i < NumViewports; i++)
    {
      vps [i] = pViewports [i];

      if ( vps [i].Width  == 4096.0f &&
           vps [i].Height == 4096.0f )
      {
          vps [i].Width  *= __SK_YS8_ShadowScale0;
          vps [i].Height *= __SK_YS8_ShadowScale0;
      }
    }

    _D3D11_RSSetViewports_Original (This, NumViewports, vps);
  }

  else
    _D3D11_RSSetViewports_Original (This, NumViewports, pViewports);
}

__declspec (noinline)
HRESULT
WINAPI
SK_YS8_CreateSamplerState (
  _In_            ID3D11Device        *This,
  _In_      const D3D11_SAMPLER_DESC  *pSamplerDesc,
  _Out_opt_       ID3D11SamplerState **ppSamplerState )
{
  extern D3D11Dev_CreateSamplerState_pfn D3D11Dev_CreateSamplerState_Original;

  D3D11_SAMPLER_DESC new_desc (*pSamplerDesc);

  //dll_log.Log ( L"CreateSamplerState - Filter: %x, MaxAniso: %lu, MipLODBias: %f, MinLOD: %f, MaxLOD: %f, Comparison: %x, U:%x,V:%x,W:%x - %ws",
  //              new_desc.Filter, new_desc.MaxAnisotropy, new_desc.MipLODBias, new_desc.MinLOD, new_desc.MaxLOD,
  //              new_desc.ComparisonFunc, new_desc.AddressU, new_desc.AddressV, new_desc.AddressW, SK_SummarizeCaller ().c_str () );

  if (new_desc.Filter <= D3D11_FILTER_ANISOTROPIC)
  {
    if (new_desc.Filter != D3D11_FILTER_MIN_MAG_MIP_POINT)
    {
      new_desc.Filter        = anisotropic_filter ? D3D11_FILTER_ANISOTROPIC :
                                                    D3D11_FILTER_MIN_MAG_MIP_LINEAR;
      new_desc.MaxAnisotropy = anisotropic_filter ? max_anisotropy : 1;

      if (disable_negative_lod_bias && new_desc.MipLODBias < 0.0f)
                                       new_desc.MipLODBias = 0.0f;

      if (explicit_lod_bias != 0.0f)
      {
        new_desc.MipLODBias = explicit_lod_bias;
      }

      if (nearest_neighbor)
      {
        new_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
      }

      if ( new_desc.ComparisonFunc > D3D11_COMPARISON_NEVER &&
           new_desc.ComparisonFunc < D3D11_COMPARISON_ALWAYS )
      {
        new_desc.Filter = anisotropic_filter ? D3D11_FILTER_COMPARISON_ANISOTROPIC :
                                               D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
      }
      
      pSamplerDesc = &new_desc;
    }
  }

  return
    _D3D11Dev_CreateSamplerState_Original ( This, pSamplerDesc,
                                              ppSamplerState );
}


__declspec (noinline)
HRESULT
WINAPI
SK_YS8_CreateTexture2D (
  _In_            ID3D11Device           *This,
  _In_      const D3D11_TEXTURE2D_DESC   *pDesc,
  _In_opt_  const D3D11_SUBRESOURCE_DATA *pInitialData,
  _Out_opt_       ID3D11Texture2D        **ppTexture2D)
{
  if (ppTexture2D == nullptr || pDesc == nullptr)
    return _D3D11Dev_CreateTexture2D_Original ( This, pDesc, pInitialData, ppTexture2D );

  D3D11_TEXTURE2D_DESC newDesc (*pDesc);

  bool depth_format =
    pDesc->Format == DXGI_FORMAT_R24G8_TYPELESS ||
    pDesc->Format == DXGI_FORMAT_B8G8R8X8_UNORM ||
    pDesc->Format == DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

  if ( pInitialData == nullptr )
  {
    if ( pDesc->Width == 4096 && pDesc->Height == 4096 && depth_format )
    {
      ///SK_LOG0 ( ( L"Ys8 Shadow(?) Origin: '%s' -- Format: %lu, Misc Flags: %x, MipLevels: %lu, "
      ///            L"ArraySize: %lu, CPUAccess: %x, BindFlags: %x, Usage: %x",
      ///              SK_GetCallerName ().c_str (), pDesc->Format,
      ///                pDesc->MiscFlags,      pDesc->MipLevels, pDesc->ArraySize,
      ///                pDesc->CPUAccessFlags, pDesc->BindFlags, pDesc->Usage
      ///          ),
      ///          L"DX11TexMgr" );

      newDesc.Width  = static_cast <UINT> ( static_cast <double> (newDesc.Width)  * __SK_YS8_ShadowScale0 );
      newDesc.Height = static_cast <UINT> ( static_cast <double> (newDesc.Height) * __SK_YS8_ShadowScale0 );
    }

    pDesc = &newDesc;
  }

  return
    _D3D11Dev_CreateTexture2D_Original ( This,
                                  pDesc, pInitialData,
                                         ppTexture2D );
}



void
SK_YS8_InitPlugin (void)
{
  SK_SetPluginName (YS8_VERSION_STR);

  SK_CreateFuncHook (       L"ID3D11Device::CreateTexture2D",
                               D3D11Dev_CreateTexture2D_Override,
                                 SK_YS8_CreateTexture2D,
     static_cast_p2p <void> (&_D3D11Dev_CreateTexture2D_Original) );
  MH_QueueEnableHook (         D3D11Dev_CreateTexture2D_Override  );

  SK_CreateFuncHook (       L"ID3D11Device::CreateSamplerState",
                               D3D11Dev_CreateSamplerState_Override,
                                 SK_YS8_CreateSamplerState,
     static_cast_p2p <void> (&_D3D11Dev_CreateSamplerState_Original) );
  MH_QueueEnableHook (         D3D11Dev_CreateSamplerState_Override  );

  SK_CreateFuncHook (       L"ID3D11DeviceContext::RSSetViewports",
                               D3D11_RSSetViewports_Override,
                                 SK_YS8_RSSetViewports,
     static_cast_p2p <void> (&_D3D11_RSSetViewports_Original) );
  MH_QueueEnableHook (         D3D11_RSSetViewports_Override  );

  SK_CreateFuncHook (       L"SK_PlugIn_ControlPanelWidget",
                              SK_PlugIn_ControlPanelWidget,
                                 SK_YS8_ControlPanel,
     static_cast_p2p <void> (&SK_PlugIn_ControlPanelWidget_Original) );
  MH_QueueEnableHook (        SK_PlugIn_ControlPanelWidget           );

  
  ys8_config.shadows.scale =
      dynamic_cast <sk::ParameterFloat *>
        (ys8_config.factory.create_parameter <float> (L"Shadow Rescale"));

  ys8_config.shadows.scale->register_to_ini ( SK_GetDLLConfig (),
                                                L"Ys8.Shadows",
                                                  L"Scale" );

  ys8_config.shadows.scale->load (__SK_YS8_ShadowScale);

  __SK_YS8_ShadowScale =
    std::fmax (  1.f,
     std::fmin ( 4.f, __SK_YS8_ShadowScale )
              );

  __SK_YS8_ShadowScale0 = __SK_YS8_ShadowScale;

  ys8_config.mipmaps.stream_rate =
      dynamic_cast <sk::ParameterInt *>
        (ys8_config.factory.create_parameter <int> (L"Streaming Priority"));

  ys8_config.mipmaps.stream_rate->register_to_ini ( SK_GetDLLConfig (),
                                                      L"Ys8.Mipmaps",
                                                        L"StreamingPriority" );

  ys8_config.mipmaps.stream_rate->load (SK_D3D11_TexStreamPriority);

  SK_ApplyQueuedHooks ();

  InterlockedExchange (&__YS8_init, 1);
};




// From  SpecialK/render/backends/d3d11.dll
//
/* 

void
WINAPI
D3D11_PSSetSamplers_Override
(
  _In_     ID3D11DeviceContext       *This,
  _In_     UINT                       StartSlot,
  _In_     UINT                       NumSamplers,
  _In_opt_ ID3D11SamplerState *const *ppSamplers )
{
  if (ppSamplers != nullptr && (ID3D11Device *)SK_GetCurrentRenderBackend ().device.p != nullptr)
  {
    ID3D11SamplerState** pSamplerCopy =
      (ID3D11SamplerState **)SK_TLS_Bottom ()->scratch_memory.cmd.alloc (sizeof (ID3D11SamplerState  *) * 4096);

    for ( int i = 0 ; i < NumSamplers ; i++ )
    {
      pSamplerCopy [i] = ppSamplers [i];

      if (ppSamplers [i] != nullptr)
      {
        D3D11_SAMPLER_DESC        new_desc = { };
        ppSamplers [i]->GetDesc (&new_desc);

        ((ID3D11Device *)SK_GetCurrentRenderBackend ().device.p)->CreateSamplerState (
                                                 &new_desc,
                                                   &pSamplerCopy [i] );
      }
    }

    return
      D3D11_PSSetSamplers_Original (This, StartSlot, NumSamplers, pSamplerCopy);
  }

  D3D11_PSSetSamplers_Original (This, StartSlot, NumSamplers, ppSamplers);
}

*/