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


#define DGPU_VERSION_NUM L"0.2.0.2"
#define DGPU_VERSION_STR L".hack//G.P.U. v " DGPU_VERSION_NUM

volatile LONG __DGPU_init = FALSE;


//
//
// It's hideous, don't look!
//
//

typedef void (__stdcall *SK_ReShade_SetResolutionScale_pfn)(float fScale);
static SK_ReShade_SetResolutionScale_pfn SK_ReShade_SetResolutionScale = nullptr;

struct SK_DGPU_ScreenFlare_Inst {
  struct
  {
    float gBlendType  [4] = { .0f, .0f, .0f, .0f };
    float flare_color [4] = { .0f, .0f, .0f, .0f };
    float flare_param [4] = { .0f, .0f, .0f, .0f }; // N/A for Global
    float flare_pos   [4] = { .0f, .0f, .0f, .0f }; // N/A for Global
  } data;
  ID3D11Buffer* buffer   = nullptr;
  bool          override = false;
  bool          stale    = true;
  const char*   type     = "";
} SK_DGPU_ScreenFlare_Global,
  SK_DGPU_ScreenFlare_Local;

struct dgpu_cfg_s
{
  struct antialiasing_s
  {
    sk::ParameterFloat* scale = nullptr;
  } antialiasing;

  struct mipmaps_s
  {
    sk::ParameterBool* cache_mipmaps        = nullptr;
    sk::ParameterBool* uncompressed_mipmaps = nullptr;
  } mipmaps;

  struct flares_s
  {
    struct flare_type_s
    {
      flare_type_s (const char*               type,
                    SK_DGPU_ScreenFlare_Inst* backing_store)
      {
        data_       = backing_store;
        data_->type = type;
      };

      sk::ParameterBool* override = nullptr;
      sk::ParameterInt*  blend_eq = nullptr;
      sk::ParameterInt*  color_r  = nullptr;
      sk::ParameterInt*  color_g  = nullptr;
      sk::ParameterInt*  color_b  = nullptr;
      sk::ParameterInt*  color_a  = nullptr;

      void init  (void)
      {
        override =
          dynamic_cast <sk::ParameterBool *>
            (g_ParameterFactory->create_parameter <bool> (L"Override Flares"));

        override->register_to_ini ( SK_GetDLLConfig (),
                                      SK_FormatStringW (L"dGPU.Flares{%hs}", data_->type),
                                        L"Override" );

        blend_eq =
          dynamic_cast <sk::ParameterInt *>
            (g_ParameterFactory->create_parameter <int> (L"Blend Equation"));

        blend_eq->register_to_ini ( SK_GetDLLConfig (),
                                      SK_FormatStringW (L"dGPU.Flares{%hs}", data_->type),
                                        L"BlendEq" );

        color_r =
          dynamic_cast <sk::ParameterInt *>
            (g_ParameterFactory->create_parameter <int> (L"Constant Red Value"));

        color_r->register_to_ini ( SK_GetDLLConfig (),
                                     SK_FormatStringW (L"dGPU.Flares{%hs}", data_->type),
                                       L"Red" );

        color_g =
          dynamic_cast <sk::ParameterInt *>
            (g_ParameterFactory->create_parameter <int> (L"Constant Green Value"));

        color_g->register_to_ini ( SK_GetDLLConfig (),
                                     SK_FormatStringW (L"dGPU.Flares{%hs}", data_->type),
                                       L"Green" );

        color_b =
          dynamic_cast <sk::ParameterInt *>
            (g_ParameterFactory->create_parameter <int> (L"Constant Blue Value"));

        color_b->register_to_ini ( SK_GetDLLConfig (),
                                     SK_FormatStringW (L"dGPU.Flares{%hs}", data_->type),
                                       L"Blue" );

        color_a =
          dynamic_cast <sk::ParameterInt *>
            (g_ParameterFactory->create_parameter <int> (L"Constant Alpha Value"));

        color_a->register_to_ini ( SK_GetDLLConfig (),
                                     SK_FormatStringW (L"dGPU.Flares{%hs}", data_->type),
                                       L"Alpha" );
      }

      void load (void)
      {
        override->load (data_->override);

        int             eq = 0;
        blend_eq->load (eq);

        data_->data.gBlendType [0] = static_cast <float> (eq);

        int r,g,b,a;

        color_r->load (r); color_g->load (g);
        color_b->load (b); color_a->load (a);

        data_->data.flare_color [0] = static_cast <float> (r) / 255.0f;
        data_->data.flare_color [1] = static_cast <float> (g) / 255.0f;
        data_->data.flare_color [2] = static_cast <float> (b) / 255.0f;
        data_->data.flare_color [3] = static_cast <float> (a) / 255.0f;
      }

      void store (void)
      {
        override->store (data_->override);

        int              eq = static_cast <int> (data_->data.gBlendType [0]);
        blend_eq->store (eq);

        data_->data.gBlendType [0] = static_cast <float> (eq);

        int r,g,b,a;

        r = static_cast <int> (data_->data.flare_color [0] * 255.0f);
        g = static_cast <int> (data_->data.flare_color [1] * 255.0f);
        b = static_cast <int> (data_->data.flare_color [2] * 255.0f);
        a = static_cast <int> (data_->data.flare_color [3] * 255.0f);

        color_r->store (r); color_g->store (g);
        color_b->store (b); color_a->store (a);

        SK_GetDLLConfig ()->write (SK_GetDLLConfig ()->get_filename ());
      }

      SK_DGPU_ScreenFlare_Inst* data_ = nullptr;
    } global { "Global", &SK_DGPU_ScreenFlare_Global },
      local  { "Local",  &SK_DGPU_ScreenFlare_Local  };
  } flares;
};

SK_LazyGlobal <dgpu_cfg_s> dgpu_config;


struct
{
  float scale = 0.0f;
} aa_prefs;



void
SK_DGPU_UpdateFlareBuffers (void)
{
  IUnknown_AtomicRelease ((void **)&SK_DGPU_ScreenFlare_Global.buffer);
  IUnknown_AtomicRelease ((void **)&SK_DGPU_ScreenFlare_Local.buffer );

  D3D11_BUFFER_DESC      FlareDesc   = { };
  D3D11_SUBRESOURCE_DATA FlareData_G = { };
  D3D11_SUBRESOURCE_DATA FlareData_L = { };

  FlareDesc.BindFlags           = D3D11_BIND_CONSTANT_BUFFER;
  FlareDesc.ByteWidth           = 64;
  FlareDesc.StructureByteStride = 64;
  FlareDesc.CPUAccessFlags      = 0;
  FlareDesc.MiscFlags           = 0;
  FlareDesc.Usage               = D3D11_USAGE_IMMUTABLE;

  FlareData_G.pSysMem           = &SK_DGPU_ScreenFlare_Global.data.gBlendType [0];
  FlareData_L.pSysMem           = &SK_DGPU_ScreenFlare_Local.data.gBlendType  [0];

  SK_ComQIPtr <ID3D11Device> pDev (SK_GetCurrentRenderBackend ().device);

  if (pDev != nullptr)
  {
    pDev->CreateBuffer (&FlareDesc, &FlareData_G, &SK_DGPU_ScreenFlare_Global.buffer);
    pDev->CreateBuffer (&FlareDesc, &FlareData_L, &SK_DGPU_ScreenFlare_Local.buffer);
  }
}



unsigned int
__stdcall
SK_DGPU_CheckVersion (LPVOID user)
{
  UNREFERENCED_PARAMETER (user);

  // 12/28/20: Disabled version checks, since I don't intend to ever update this thing again.
  //
  ////if (SK_FetchVersionInfo (L"dGPU"))
  ////{
  ////  SK_UpdateSoftware (L"dGPU");
  ////}

  return 0;
}

bool
SK_DGPU_PlugInCfg (void)
{
  if (ImGui::CollapsingHeader (".hack//G.U.", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::TreePush ("");

    ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.40f, 0.40f, 0.45f));
    ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.45f, 0.45f, 0.80f));
    ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.53f, 0.53f, 0.80f));

    bool aa = ImGui::CollapsingHeader ("Anti-Aliasing", ImGuiTreeNodeFlags_DefaultOpen);

    if (ImGui::IsItemHovered ())
      ImGui::SetTooltip ("This does not change anything in-game, set this to match your in-game setting so that ReShade works correctly.");

    if (aa)
    {
      bool changed = false;

      ImGui::TreePush ("");

      int level = aa_prefs.scale == 1.0f ? 1 :
                  aa_prefs.scale == 2.0f ? 2 :
                  aa_prefs.scale == 0.0f ? 0 :
                                           0;

      changed |= ImGui::RadioButton ("Low##AntiAliasLevel_DotHack",    &level, 0);
      ImGui::SameLine ();
      changed |= ImGui::RadioButton ("Medium##AntiAliasLevel_DotHack", &level, 1);
      if (ImGui::IsItemHovered ()) ImGui::SetTooltip ("Not recommended, please consider Low or High");
      ImGui::SameLine ();
      changed |= ImGui::RadioButton ("High##AntiAliasLevel_DotHack",   &level, 2);

      if (changed)
      {
        aa_prefs.scale = ( level == 0 ? 0.0f :
                           level == 1 ? 1.0f :
                           level == 2 ? 2.0f :
                                        1.0f );

        dgpu_config->antialiasing.scale->store (aa_prefs.scale);

        SK_GetDLLConfig ()->write (SK_GetDLLConfig ()->get_filename ());

        if (SK_ReShade_SetResolutionScale != nullptr)
        {
          SK_ReShade_SetResolutionScale (aa_prefs.scale);
        }
      }

      ImGui::TreePop  ();
    }


    extern uint64_t SK_D3D11_MipmapCacheSize;

    extern LONG SK_D3D11_Resampler_GetActiveJobCount  (void);
    extern LONG SK_D3D11_Resampler_GetWaitingJobCount (void);
  //extern LONG SK_D3D11_Resampler_GetRetiredCount    (void);
    extern LONG SK_D3D11_Resampler_GetErrorCount      (void);

    bool tex_manage = ImGui::CollapsingHeader ("Texture Management##DotHack", ImGuiTreeNodeFlags_DefaultOpen);

    LONG jobs = SK_D3D11_Resampler_GetActiveJobCount () + SK_D3D11_Resampler_GetWaitingJobCount ();

    static DWORD dwLastActive = 0;

    if (jobs != 0 || dwLastActive > SK_timeGetTime () - 500)
    {
      if (jobs > 0)
        dwLastActive = SK_timeGetTime ();

      if (jobs > 0)
        ImGui::PushStyleColor (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (0.4f - (0.4f * (SK_D3D11_Resampler_GetActiveJobCount ()) / (float)jobs), 0.15f, 1.0f));
      else
        ImGui::PushStyleColor (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (0.4f - (0.4f * (SK_timeGetTime () - dwLastActive) / 500.0f), 1.0f, 0.8f));

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
      ImGui::Checkbox    ("Generate Mipmaps", &config.textures.d3d11.generate_mips);

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip    ();
        ImGui::PushStyleColor  (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (0.5f, 0.f, 1.f, 1.f));
        ImGui::TextUnformatted ("Builds Complete Mipchains (Mipmap LODs) for all Textures");
        ImGui::Separator       ();
        ImGui::PopStyleColor   ();
        ImGui::Bullet          (); ImGui::SameLine ();
        ImGui::PushStyleColor  (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (0.15f, 1.0f, 1.0f));
        ImGui::TextUnformatted ("SIGNIFICANTLY");
        ImGui::PopStyleColor   (); ImGui::SameLine ();
        ImGui::TextUnformatted ("reduces texture aliasing");
        ImGui::BulletText      ("May increase load-times and/or hurt compatibility with third-party software");
        ImGui::EndTooltip      ();
      }

      if (config.textures.d3d11.generate_mips)
      {
        bool changed  = false;
             changed |= ImGui::Checkbox ("Cache Mipmaps to Disk", &config.textures.d3d11.cache_gen_mips);

        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("Eliminates possible texture pop-in, but will slow down loading and consume disk space.");

        ImGui::EndGroup        ();
        ImGui::SameLine        ();
        ImGui::BeginGroup      ();

        ImGui::TextUnformatted ("Mipmap Quality "); ImGui::SameLine ();

        int sel = config.textures.d3d11.uncompressed_mips ? 1 : 0;

        changed |= ImGui::Combo ("###dGPU_MipmapQuality", &sel, "Compressed (Low)\0Uncompressed (High)\0\0", 2);

        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("Uncompressed textures stutter less at load-time, but use significantly more memory.");

        if (ImGui::Button   (" Purge Cache "))
        {
          wchar_t wszPath [ MAX_PATH + 2 ] = { };

          wcscpy ( wszPath,
                     SK_EvalEnvironmentVars (config.textures.d3d11.res_root.c_str ()).c_str () );

          lstrcatW (wszPath, L"/inject/textures/MipmapCache/");
          lstrcatW (wszPath, SK_GetHostApp ());
          lstrcatW (wszPath, L"/");

          SK_D3D11_MipmapCacheSize -= SK_DeleteTemporaryFiles (wszPath, L"*.dds");
        //SK_D3D11_MipmapCacheSize  = 0;
        }

        ImGui::SameLine ();
        ImGui::Text     ("Current Cache Size: %.2f MiB", (double)SK_D3D11_MipmapCacheSize / (1024.0 * 1024.0));
        ImGui::EndGroup ();

        if (changed)
        {
          config.textures.d3d11.uncompressed_mips = (sel == 1 ? true : false);

          dgpu_config->mipmaps.cache_mipmaps->store        (config.textures.d3d11.cache_gen_mips);
          dgpu_config->mipmaps.uncompressed_mipmaps->store (config.textures.d3d11.uncompressed_mips);

          SK_GetDLLConfig ()->write (SK_GetDLLConfig ()->get_filename ());
        }
      }

      else
        ImGui::EndGroup ();

      ImGui::TreePop    ();
    }

    if (ImGui::CollapsingHeader ("Screen Flares", ImGuiTreeNodeFlags_DefaultOpen))
    {
      ImGui::TreePush ("");

      auto FlareMenu =
      [](const char* name, SK_DGPU_ScreenFlare_Inst* pInst) -> bool
      {
        ImGui::BeginGroup (    );
        ImGui::PushID     (name);

        pInst->stale |=
          ImGui::Checkbox (SK_FormatString ("Override %hs", name).c_str (), &pInst->override);

        if (pInst->override || pInst->stale)
        {
          int type = (int)pInst->data.gBlendType [0];

              pInst->stale |= ImGui::Combo ( "Blend Method", &type, "Modulate\0Add\0Subtract\0"
                                                                     "Unknown0\0Unknown1\0\0", 5 );
          if (pInst->stale) { pInst->data.gBlendType [0] = (float)type; }

          pInst->stale |= ImGui::ColorEdit4 ("Flare Color", pInst->data.flare_color);

          if (pInst->stale)
          {
            SK_DGPU_UpdateFlareBuffers ();
            pInst->stale = false;

            ImGui::PopID    ();
            ImGui::EndGroup ();
            return true;
          }
        }

        ImGui::PopID    ();
        ImGui::EndGroup ();
        return false;
      };

      if (FlareMenu ("Stationary", &SK_DGPU_ScreenFlare_Global))
        dgpu_config->flares.global.store ();

      ImGui::SameLine ();

      if (FlareMenu ("Positional", &SK_DGPU_ScreenFlare_Local))
        dgpu_config->flares.local.store ();

      ImGui::TreePop  ();
    }

    ImGui::PopStyleColor (3);
    ImGui::TreePop       ( );
  }

  return true;
}

HRESULT
STDMETHODCALLTYPE
SK_DGPU_PresentFirstFrame (IUnknown* pSwapChain, UINT SyncInterval, UINT Flags)
{
  UNREFERENCED_PARAMETER (pSwapChain);
  UNREFERENCED_PARAMETER (SyncInterval);
  UNREFERENCED_PARAMETER (Flags);

  if (! InterlockedCompareExchange (&__DGPU_init, 1, 0))
  {
    if (SK_ReShade_SetResolutionScale == nullptr)
    {
      SK_ReShade_SetResolutionScale =
        (SK_ReShade_SetResolutionScale_pfn)SK_GetProcAddress (
          SK_GetModuleHandle (L"dxgi.dll"),
            "SK_ReShade_SetResolutionScale"
        );
    }

    if (SK_ReShade_SetResolutionScale == nullptr)
    {
      SK_ReShade_SetResolutionScale =
        (SK_ReShade_SetResolutionScale_pfn)SK_GetProcAddress (
          SK_GetModuleHandle (L"d3d11.dll"),
            "SK_ReShade_SetResolutionScale"
        );
    }

    if (SK_ReShade_SetResolutionScale != nullptr)
      SK_ReShade_SetResolutionScale (aa_prefs.scale);

    if (! SK_IsInjected ())
      SK_DGPU_CheckVersion (nullptr);
  }

  return S_OK;
}


#include <render/d3d11/d3d11_state_tracker.h>

 extern SK_D3D11_DrawHandlerState   SK_D3D11_DrawHandler     (ID3D11DeviceContext* pDevCtx, SK_D3D11DrawType, UINT verts, SK_TLS** ppTLS, UINT&);
typedef SK_D3D11_DrawHandlerState (*SK_D3D11_DrawHandler_pfn)(ID3D11DeviceContext* pDevCtx, SK_D3D11DrawType, UINT verts, SK_TLS** ppTLS, UINT&);
                                    SK_D3D11_DrawHandler_pfn
                                    SK_D3D11_DrawHandler_Original = nullptr;

SK_D3D11_DrawHandlerState
SK_DGPU_DrawHandler (ID3D11DeviceContext* pDevCtx, SK_D3D11DrawType type, UINT verts, SK_TLS** ppTLS, UINT& d_idx)
{
  if ( SK_DGPU_ScreenFlare_Local.override ||
       SK_DGPU_ScreenFlare_Global.override )
  {
    if (d_idx == -1)
        d_idx = SK_D3D11_GetDeviceContextHandle (pDevCtx);

    uint32_t current_pixel_shader =
      SK_D3D11_Shaders->pixel.current.shader [d_idx];

    const uint32_t PS_GLOBAL_FLARE = 0xD18AEDF1;  // Has position, but we're not going to bother with that.
    const uint32_t PS_LOCAL_FLARE  = 0xBDCAA539;  // No position, just one constant color screwing up the whole screen!

    if ( SK_DGPU_ScreenFlare_Global.override && current_pixel_shader == PS_GLOBAL_FLARE )
      pDevCtx->PSSetConstantBuffers (0, 1, &SK_DGPU_ScreenFlare_Global.buffer);

    else if ( SK_DGPU_ScreenFlare_Local.override && current_pixel_shader == PS_LOCAL_FLARE )
      pDevCtx->PSSetConstantBuffers (0, 1, &SK_DGPU_ScreenFlare_Local.buffer);
  }


  return
    SK_D3D11_DrawHandler_Original (pDevCtx, type, verts, ppTLS, d_idx);
}


void
SK_DGPU_InitPlugin (void)
{
  SK_SetPluginName (DGPU_VERSION_STR);

  plugin_mgr->config_fns.emplace      (SK_DGPU_PlugInCfg);
  plugin_mgr->first_frame_fns.emplace (SK_DGPU_PresentFirstFrame);

  dgpu_config->antialiasing.scale =
      dynamic_cast <sk::ParameterFloat *>
        (g_ParameterFactory->create_parameter <float> (L"Anti-Aliasing Scale"));

  dgpu_config->antialiasing.scale->register_to_ini ( SK_GetDLLConfig (),
                                                      L"dGPU.Antialiasing",
                                                        L"Scale" );

  dgpu_config->antialiasing.scale->load (aa_prefs.scale);


  dgpu_config->mipmaps.cache_mipmaps =
      dynamic_cast <sk::ParameterBool *>
        (g_ParameterFactory->create_parameter <bool> (L"Cache Mipmaps on Disk"));

  dgpu_config->mipmaps.cache_mipmaps->register_to_ini ( SK_GetDLLConfig (),
                                                         L"dGPU.Mipmaps",
                                                           L"CacheOnDisk" );

  dgpu_config->mipmaps.uncompressed_mipmaps =
      dynamic_cast <sk::ParameterBool *>
        (g_ParameterFactory->create_parameter <bool> (L"Do not compress mipmaps"));

  dgpu_config->mipmaps.uncompressed_mipmaps->register_to_ini ( SK_GetDLLConfig (),
                                                                L"dGPU.Mipmaps",
                                                                  L"Uncompressed" );


  dgpu_config->mipmaps.cache_mipmaps->load        (config.textures.d3d11.cache_gen_mips);
  dgpu_config->mipmaps.uncompressed_mipmaps->load (config.textures.d3d11.uncompressed_mips);


  dgpu_config->flares.global.init ();
  dgpu_config->flares.local.init  ();

  dgpu_config->flares.global.load ();
  dgpu_config->flares.local.load  ();


  SK_CreateFuncHook (        L"SK_D3D11_DrawHandler",
                               SK_D3D11_DrawHandler,
                                SK_DGPU_DrawHandler,
     static_cast_p2p <void> (&SK_D3D11_DrawHandler_Original) );
  MH_QueueEnableHook (        SK_D3D11_DrawHandler           );

  SK_ReShade_SetResolutionScale =
    (SK_ReShade_SetResolutionScale_pfn)SK_GetProcAddress (
      SK_GetModuleHandle (L"dxgi.dll"),
        "SK_ReShade_SetResolutionScale"
  );

  SK_ApplyQueuedHooks ();
};