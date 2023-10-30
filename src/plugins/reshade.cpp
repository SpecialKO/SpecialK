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
#include <SpecialK/plugin/reshade.h>

HMODULE
__stdcall
SK_ReShade_GetDLL (void)
{
  static HMODULE hModReShade =
    sk::narrow_cast <HMODULE> (nullptr);

  static bool tried_once = false;

  if (! tried_once)
  {
    tried_once = true;

    for (int i = 0; i < SK_MAX_IMPORTS; i++)
    {
      auto& import =
        imports->imports [i];

      if (import.hLibrary != nullptr)
      {
        if (StrStrIW (import.filename->get_value_ref ().c_str (), L"ReShade"))
        {
          typedef HMODULE (__stdcall *SK_SHIM_GetReShade_pfn)(void);

          SK_SHIM_GetReShade_pfn SK_SHIM_GetReShade =
            (SK_SHIM_GetReShade_pfn)SK_GetProcAddress (import.hLibrary, "SK_SHIM_GetReShade");

          if (SK_SHIM_GetReShade != nullptr)
          {
            HMODULE hModReal =
              SK_SHIM_GetReShade ();

            if (hModReal)
            {
              hModReShade =
                hModReal;

              break;
            }
          }

          hModReShade =
            import.hLibrary;

          break;
        }
      }
    }
  }

  return
    hModReShade;
};

void
SK_ReShade_LoadIfPresent (void)
{
  const wchar_t *wszDLL =
    SK_RunLHIfBitness (64, L"ReShade64.dll",
                           L"ReShade32.dll");

  if (PathFileExistsW (wszDLL))
  {
    if (! PathFileExistsW (L"ReShade.ini"))
    {
      FILE *fINI =
        fopen ("ReShade.ini", "w+");

      if (fINI != nullptr)
      {
        fputs (R"(
[GENERAL]
EffectSearchPaths=.\,.\reshade-shaders\Shaders
TextureSearchPaths=.\,.\reshade-shaders\Textures
PresetPath=.\ReShadePreset.ini

[OVERLAY]
NoFontScaling=1

[STYLE]
EditorFont=ProggyClean.ttf
EditorFontSize=13
EditorStyleIndex=0
Font=ProggyClean.ttf
FontSize=13
StyleIndex=2)", fINI);
        fclose (fINI);
      }
    }

    LoadLibraryW (wszDLL);
  }

  SK_ReShadeAddOn_Init ();
}

#undef IMGUI_VERSION_NUM

#include <reshade/reshade.hpp>
#include <reshade/reshade_api.hpp>

#include <concurrent_unordered_map.h>

Concurrency::concurrent_unordered_map <HWND, reshade::api::effect_runtime *> ReShadeRuntimes;

void
__cdecl
SK_ReShadeAddOn_InitRuntime (reshade::api::effect_runtime *runtime)
{
  bool changed = false;

  static SK_Thread_HybridSpinlock                   _init_lock;
  std::scoped_lock <SK_Thread_HybridSpinlock> lock (_init_lock);

  SK_LOGs0 (L"ReShadeExt", L"Runtime Initialized");

  SK_RunOnce (
  {
    size_t search_path_len = 0;

    static char search_path_buf [   MAX_PATH * 16] = { };
    memset (    search_path_buf, 0, MAX_PATH * 16);

    reshade::get_config_value (runtime, "GENERAL", "EffectSearchPaths", nullptr,         &search_path_len);
    reshade::get_config_value (runtime, "GENERAL", "EffectSearchPaths", search_path_buf, &search_path_len);

    static std::string install_path (SK_WideCharToUTF8 (SK_GetInstallPath ()));

    if (search_path_len == 0 || !StrStrIA (search_path_buf, install_path.c_str ()))
    {
      //// Default search path is ".\", we can ignore it
      if (search_path_len <= 3)
          search_path_len = 0;

      std::string shared_path =
        SK_FormatString (R"(%ws\Global\ReShade\Shaders\)", SK_GetInstallPath ());

      // There is a bug in ReShade's INI setter for std::filesystem::path lists, we cannot
      //   append entries to that list without breaking the INI file... so only do this when
      //     initializing ReShade's INI.
      if (search_path_len == 0 && PathFileExistsA (shared_path.c_str ()))
      {
        std::string search_path =
          SK_FormatString ( search_path_len == 0 ? R"(%hs**)"
                                                 : R"(%hs**,%*hs)", shared_path.c_str (),
                                                                  search_path_len,
                                                                  search_path_buf );

        reshade::set_config_value (runtime, "GENERAL", "EffectSearchPaths", search_path.c_str ());
        SK_LOGs0 (L"ReShadeExt", L"Updated Global Effect Search Path: %hs", search_path.c_str ());

        changed = true;
      }
    }

    memset (search_path_buf, 0, MAX_PATH * 16);
            search_path_len = 0;

    reshade::get_config_value (runtime, "GENERAL", "TextureSearchPaths", nullptr,         &search_path_len);
    reshade::get_config_value (runtime, "GENERAL", "TextureSearchPaths", search_path_buf, &search_path_len);

    if (search_path_len == 0 || !StrStrIA (search_path_buf, install_path.c_str ()))
    {
      // Default search path is ".\", we can ignore it
      if (search_path_len <= 3)
          search_path_len = 0;

      std::string shared_path =
        SK_FormatString (R"(%ws\Global\ReShade\Textures\)", SK_GetInstallPath ());

      // There is a bug in ReShade's INI setter for std::filesystem::path lists, we cannot
      //   append entries to that list without breaking the INI file... so only do this when
      //     initializing ReShade's INI.
      if (search_path_len == 0 && PathFileExistsA (shared_path.c_str ()))
      {

        std::string search_path =
          SK_FormatString ( search_path_len == 0 ? R"(%hs**)"
                                                 : R"(%hs**,%*hs)", shared_path.c_str (),
                                                                    search_path_len,
                                                                    search_path_buf );

        reshade::set_config_value (runtime, "GENERAL", "TextureSearchPaths", search_path.c_str ());
        SK_LOGs0 (L"ReShadeExt", L"Updated Global Texture Search Path: %hs", search_path.c_str ());

        changed = true;
      }
    }
  });

  ReShadeRuntimes [(HWND)runtime->get_hwnd ()] = runtime;

  // ReShade doesn't properly handle changes to this without a full restart
  if (changed)
  {
    SK_ImGui_Warning (L"ReShade Effect Reload Required for AddOn Setup To Complete");
  }
}

void
__cdecl
SK_ReShadeAddOn_DestroyRuntime (reshade::api::effect_runtime *runtime)
{
  SK_LOGs0 (L"ReShadeExt", L"Runtime Destroyed");

  ReShadeRuntimes [(HWND)runtime->get_hwnd ()] = nullptr;
}

bool
SK_ReShadeAddOn_RenderEffectsDXGI (IDXGISwapChain1 *pSwapChain)
{
  if (! pSwapChain)
    return false;

  HWND                  hWnd = 0;
  pSwapChain->GetHwnd (&hWnd);

  if (! hWnd)
    return false;

  DXGI_SWAP_CHAIN_DESC  swapDesc = { };
  pSwapChain->GetDesc (&swapDesc);

  auto runtime =
    ReShadeRuntimes [hWnd];

  if (runtime != nullptr)
  {
    auto device =
      runtime->get_device ();

    auto backbuffer =
      runtime->get_back_buffer (runtime->get_current_back_buffer_index ());

    auto rtvDesc =
      reshade::api::resource_view_desc ((reshade::api::format)swapDesc.BufferDesc.Format);

    reshade::api::resource_view rtv;

    if (! device->create_resource_view (backbuffer, reshade::api::resource_usage::render_target, rtvDesc, &rtv))
      return false;

    runtime->render_effects (
      runtime->get_command_queue ()->get_immediate_command_list (), rtv
    );

    return true;
  }

  return false;
}

bool
SK_ReShadeAddOn_Init (HMODULE reshade_module)
{
  static bool registered = false;

  SK_RunOnce ({
    registered =
      reshade::register_addon (SK_GetDLL (), reshade_module);

    if (registered)
    {
      std::wstring shared_base_path =
        SK_FormatStringW (LR"(%ws\Global\ReShade\)", SK_GetInstallPath ());

      // Create this directory so users have an easier time putting Textures and Shaders in-place.
      SK_CreateDirectories (shared_base_path.c_str ());

      config.reshade.is_addon = true;

      reshade::register_event <reshade::addon_event::init_effect_runtime>    (SK_ReShadeAddOn_InitRuntime);
      reshade::register_event <reshade::addon_event::destroy_effect_runtime> (SK_ReShadeAddOn_DestroyRuntime);
    }
  });

  return
    registered;
}