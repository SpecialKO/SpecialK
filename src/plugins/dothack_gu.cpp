//
// Copyright 2017  Andon  "Kaldaien" Coleman
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

#include <SpecialK/dxgi_backend.h>
#include <SpecialK/config.h>
#include <SpecialK/command.h>
#include <SpecialK/framerate.h>
#include <SpecialK/ini.h>
#include <SpecialK/parameter.h>
#include <SpecialK/utility.h>
#include <SpecialK/log.h>
#include <SpecialK/steam_api.h>

#include <SpecialK/input/input.h>
#include <SpecialK/input/xinput.h>

#include <SpecialK/hooks.h>
#include <SpecialK/core.h>
#include <process.h>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_d3d11.h>

#include <atlbase.h>

#define DGPU_VERSION_NUM L"0.1.1"
#define DGPU_VERSION_STR L".hack//G.P.U. v " DGPU_VERSION_NUM

volatile LONG __DGPU_init = FALSE;


//
//
// It's hideous, don't look!
//
//


extern HMODULE
__stdcall
SK_ReShade_GetDLL (void);

extern bool
__stdcall
SK_FetchVersionInfo (const wchar_t* wszProduct);

extern HRESULT
__stdcall
SK_UpdateSoftware   (const wchar_t* wszProduct);

typedef void (__stdcall *SK_ReShade_SetResolutionScale_pfn)(float fScale);
static SK_ReShade_SetResolutionScale_pfn SK_ReShade_SetResolutionScale = nullptr;

using SK_PlugIn_ControlPanelWidget_pfn = void (__stdcall         *)(void);
static SK_PlugIn_ControlPanelWidget_pfn SK_PlugIn_ControlPanelWidget_Original = nullptr;

size_t SK_DGPU_MipmapCacheSize = 0ULL;

struct dpgu_cfg_s
{
  sk::ParameterFactory factory;

  struct antialiasing_s
  {
    sk::ParameterFloat* scale = nullptr;
  } antialiasing;

  struct mipmaps_s
  {
    sk::ParameterBool* cache_mipmaps        = nullptr;
    sk::ParameterBool* uncompressed_mipmaps = nullptr;
  } mipmaps;
} dgpu_config;


struct
{
  float scale = 0.0f;
} aa_prefs;

extern void
__stdcall
SK_SetPluginName (std::wstring name);

unsigned int
__stdcall
SK_DGPU_CheckVersion (LPVOID user)
{
  UNREFERENCED_PARAMETER (user);

  extern volatile LONG   SK_bypass_dialog_active;
  InterlockedIncrement (&SK_bypass_dialog_active);

  if (SK_FetchVersionInfo (L"dGPU"))
    SK_UpdateSoftware (L"dGPU");

  InterlockedDecrement (&SK_bypass_dialog_active);

  return 0;
}

void
__stdcall
SK_DGPU_ControlPanel (void)
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

        dgpu_config.antialiasing.scale->store (aa_prefs.scale);

        SK_GetDLLConfig ()->write (SK_GetDLLConfig ()->get_filename ());

        if (SK_ReShade_SetResolutionScale != nullptr)
        {
          SK_ReShade_SetResolutionScale (aa_prefs.scale);
        }
      }

      ImGui::TreePop  ();
    }


    extern LONG SK_D3D11_Resampler_GetActiveJobCount  (void);
    extern LONG SK_D3D11_Resampler_GetWaitingJobCount (void);
    extern LONG SK_D3D11_Resampler_GetRetiredCount    (void);
    extern LONG SK_D3D11_Resampler_GetErrorCount      (void);

    bool tex_manage = ImGui::CollapsingHeader ("Texture Management##DotHack", ImGuiTreeNodeFlags_DefaultOpen);

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
      ImGui::Checkbox    ("Generate Mipmaps", &config.textures.d3d11.generate_mips);

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

          SK_DGPU_MipmapCacheSize -= SK_DeleteTemporaryFiles (wszPath, L"*.dds");
          SK_DGPU_MipmapCacheSize  = 0;
        }

        ImGui::SameLine ();
        ImGui::Text     ("Current Cache Size: %.2f MiB", (double)SK_DGPU_MipmapCacheSize / (1024.0 * 1024.0));
        ImGui::EndGroup ();

        if (changed)
        {
          config.textures.d3d11.uncompressed_mips = (sel == 1 ? true : false);

          dgpu_config.mipmaps.cache_mipmaps->store        (config.textures.d3d11.cache_gen_mips);
          dgpu_config.mipmaps.uncompressed_mipmaps->store (config.textures.d3d11.uncompressed_mips);

          SK_GetDLLConfig ()->write (SK_GetDLLConfig ()->get_filename ());
        }
      }

      else
        ImGui::EndGroup ();

      ImGui::TreePop    ();
    }

    ImGui::PopStyleColor (3);
    ImGui::TreePop       ( );
  }
}


HRESULT
STDMETHODCALLTYPE
SK_DGPU_PresentFirstFrame (IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
  UNREFERENCED_PARAMETER (pSwapChain);
  UNREFERENCED_PARAMETER (SyncInterval);
  UNREFERENCED_PARAMETER (Flags);

  while (! InterlockedAdd (&__DGPU_init, 0))
    SleepEx (16, FALSE);

  if (SK_ReShade_SetResolutionScale == nullptr)
  {
    SK_ReShade_SetResolutionScale =
      (SK_ReShade_SetResolutionScale_pfn)GetProcAddress (
        GetModuleHandle (L"dxgi.dll"),
          "SK_ReShade_SetResolutionScale"
      );
  }

  if (SK_ReShade_SetResolutionScale != nullptr)
    SK_ReShade_SetResolutionScale (aa_prefs.scale);

  return S_OK;
}

void
SK_DGPU_InitPlugin (void)
{
  SK_SetPluginName (DGPU_VERSION_STR);

  dgpu_config.antialiasing.scale =
      dynamic_cast <sk::ParameterFloat *>
        (dgpu_config.factory.create_parameter <float> (L"Anti-Aliasing Scale"));

  dgpu_config.antialiasing.scale->register_to_ini ( SK_GetDLLConfig (),
                                                      L"dGPU.Antialiasing",
                                                        L"Scale" );

  dgpu_config.antialiasing.scale->load (aa_prefs.scale);


  dgpu_config.mipmaps.cache_mipmaps =
      dynamic_cast <sk::ParameterBool *>
        (dgpu_config.factory.create_parameter <bool> (L"Cache Mipmaps on Disk"));

  dgpu_config.mipmaps.cache_mipmaps->register_to_ini ( SK_GetDLLConfig (),
                                                         L"dGPU.Mipmaps",
                                                           L"CacheOnDisk" );

  dgpu_config.mipmaps.uncompressed_mipmaps =
      dynamic_cast <sk::ParameterBool *>
        (dgpu_config.factory.create_parameter <bool> (L"Do not compress mipmaps"));

  dgpu_config.mipmaps.uncompressed_mipmaps->register_to_ini ( SK_GetDLLConfig (),
                                                                L"dGPU.Mipmaps",
                                                                  L"Uncompressed" );


  dgpu_config.mipmaps.cache_mipmaps->load        (config.textures.d3d11.cache_gen_mips);
  dgpu_config.mipmaps.uncompressed_mipmaps->load (config.textures.d3d11.uncompressed_mips);

  SK_CreateFuncHook (       L"SK_PlugIn_ControlPanelWidget",
                              SK_PlugIn_ControlPanelWidget,
                                SK_DGPU_ControlPanel,
     static_cast_p2p <void> (&SK_PlugIn_ControlPanelWidget_Original) );
  MH_QueueEnableHook (        SK_PlugIn_ControlPanelWidget           );

  SK_ReShade_SetResolutionScale =
    (SK_ReShade_SetResolutionScale_pfn)GetProcAddress (
      GetModuleHandle (L"dxgi.dll"),
        "SK_ReShade_SetResolutionScale"
  );

  MH_ApplyQueued ();

  InterlockedExchange (&__DGPU_init, 1);

  if (! SK_IsInjected ())
    SK_DGPU_CheckVersion (nullptr);
};