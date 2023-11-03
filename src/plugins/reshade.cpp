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

  size_t search_path_len = 0;

  static char search_path_buf [   MAX_PATH * 16] = { };
  memset (    search_path_buf, 0, MAX_PATH * 16);

  reshade::get_config_value (runtime, "GENERAL", "EffectSearchPaths", nullptr,         &search_path_len);
  reshade::get_config_value (runtime, "GENERAL", "EffectSearchPaths", search_path_buf, &search_path_len);

  static std::filesystem::path install_path (SK_GetInstallPath ());
  static std::filesystem::path shader_path =
    install_path / LR"(Global\ReShade\Shaders\)";
  static std::filesystem::path texture_path =
    install_path / LR"(Global\ReShade\Textures\)";

  if (search_path_len == 0 || !StrStrIA (search_path_buf, (const char *)shader_path.u8string ().data ()))
  {
    ////// Default search path is ".\", we can ignore it
    if (search_path_len <= 3)
        search_path_len = 0;

    // There is a bug in ReShade's INI setter for std::filesystem::path lists, we cannot
    //   append entries to that list without breaking the INI file... so only do this when
    //     initializing ReShade's INI.
    if (search_path_len == 0 && PathFileExistsW (shader_path.c_str ()))
    {
      std::string search_path =
        SK_FormatString ( search_path_len == 0 ? R"(%ws**)"
                                               : R"(%ws**,%*hs)", shader_path.c_str (),
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

  if (search_path_len == 0 || !StrStrIA (search_path_buf, (const char *)texture_path.u8string ().data ()))
  {
    //// Default search path is ".\", we can ignore it
    if (search_path_len <= 3)
        search_path_len = 0;

    // There is a bug in ReShade's INI setter for std::filesystem::path lists, we cannot
    //   append entries to that list without breaking the INI file... so only do this when
    //     initializing ReShade's INI.
    if (search_path_len == 0 && PathFileExistsW (texture_path.c_str ()))
    {
      std::string search_path =
        SK_FormatString ( search_path_len == 0 ? R"(%ws**)"
                                               : R"(%ws**,%*hs)", texture_path.c_str (),
                                                                  search_path_len,
                                                                  search_path_buf );

      reshade::set_config_value (runtime, "GENERAL", "TextureSearchPaths", search_path.c_str ());
      SK_LOGs0 (L"ReShadeExt", L"Updated Global Texture Search Path: %hs", search_path.c_str ());

      changed = true;
    }
  }

  ReShadeRuntimes [(HWND)runtime->get_hwnd ()] = runtime;

  // ReShade doesn't properly handle changes to this without a full restart
  if (changed)
  {
    reshade::set_config_value (runtime, "OVERLAY", "TutorialProgress", "4");

    _flushall ();

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

static          bool ReShadeOverlayActive     = false; // Current overlay state
static volatile LONG ReShadeOverlayActivating = 0;     // Allow keyboard activation even if keyboard input is blocked

void
SK_ReShadeAddOn_ActivateOverlay (bool activate)
{
  static auto &rb =
    SK_GetCurrentRenderBackend ();

  SK_ComQIPtr <IDXGISwapChain1> pSwapChain1 (rb.swapchain);

  if (pSwapChain1.p != nullptr)
  {
    HWND                   hWnd = 0;
    pSwapChain1->GetHwnd (&hWnd);

    if (ReShadeRuntimes.count (hWnd))
    {
      InterlockedExchange (&ReShadeOverlayActivating, TRUE);

      reshade::activate_overlay (ReShadeRuntimes [hWnd], activate, reshade::api::input_source::keyboard);
    }
  }
}

void
SK_ReShadeAddOn_ToggleOverlay (void)
{
  SK_ReShadeAddOn_ActivateOverlay (!ReShadeOverlayActive);
}

bool
__cdecl
SK_ReShadeAddOn_OverlayActivation (reshade::api::effect_runtime *runtime, bool *activate, reshade::api::input_source source)
{
  std::ignore = runtime;

  // Block Keyboard Activation?
  if (source == reshade::api::input_source::keyboard && ( SK_ImGui_WantKeyboardCapture () ||
                                                          SK_ImGui_WantTextCapture     () ))
  {
    // Allow activation via keyboard if we are expecting it...
    if (! InterlockedCompareExchange (&ReShadeOverlayActivating, FALSE, TRUE))
    {
      *activate = !*activate;

      return true;
    }
  }

  // Block Gamepad Activation?
  if (source == reshade::api::input_source::gamepad && SK_ImGui_WantGamepadCapture ())
  {
    *activate = !*activate;
    return true;
  }

  auto& io =
    ImGui::GetIO ();
  
  static bool capture_keys      = io.WantCaptureKeyboard;
  static bool capture_text      = io.WantTextInput;
  static bool capture_mouse     = io.WantCaptureMouse;
  static bool nav_active        = io.NavActive;
  static bool imgui_visible     = SK_ImGui_Visible;
  static bool imgui_vis_changed = false;

  static bool last_active = !(*activate);
  
  if (std::exchange (last_active, *activate) != *activate)
  {
    // When the overlay activates, stop blocking
    //   input !!
    if (*activate)
    {
      capture_keys  =
        io.WantCaptureKeyboard;
        io.WantCaptureKeyboard = false;
    
      capture_text  =
        io.WantTextInput;
        io.WantTextInput       = false;
    
      capture_mouse =
        io.WantCaptureMouse;
        io.WantCaptureMouse    = false;
    
      nav_active    =
        io.NavActive;
        io.NavActive           = false;
    
       ImGui::SetWindowFocus (nullptr);

       ReShadeOverlayActive    = true;
    }
    
    else
    {
      io.WantCaptureKeyboard = SK_ImGui_Visible ? capture_keys  : false;
      io.WantCaptureMouse    = SK_ImGui_Visible ? capture_mouse : false;
      io.NavActive           = SK_ImGui_Visible ? nav_active    : false;
    
      ImGui::SetWindowFocus (nullptr);
      io.WantTextInput       = false;//capture_text;

      ReShadeOverlayActive   = false;
    }
  }

  return false;
}

bool SK_ReShadeAddOn_IsOverlayActive (void)
{
  return ReShadeOverlayActive;
}

reshade::api::resource_view dxgi_rtv;

void
SK_ReShadeAddOn_FinishFrameDXGI (IDXGISwapChain1 *pSwapChain)
{
  std::ignore = pSwapChain;

  //if (! pSwapChain)
  //  return;
  //
  //HWND                  hWnd = 0;
  //pSwapChain->GetHwnd (&hWnd);
  //
  //if (! hWnd)
  //  return;
  //
  //DXGI_SWAP_CHAIN_DESC  swapDesc = { };
  //pSwapChain->GetDesc (&swapDesc);
  //
  //auto runtime =
  //  ReShadeRuntimes [hWnd];
  //
  //if (runtime != nullptr)
  //{
  //  const auto device =
  //    runtime->get_device ();
  //
  //  const auto cmd_queue =
  //    runtime->get_command_queue ();
  //
  //  if (dxgi_rtv.handle != 0)
  //  {
  //    cmd_queue->wait_idle          (        ); // The temporary RTV created above must live to completion
  //    device->destroy_resource_view (dxgi_rtv);
  //
  //    dxgi_rtv.handle = 0;
  //  }
  //}
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
    bool has_effects =
      runtime->get_effects_state ();

    const auto device =
      runtime->get_device ();

    const auto cmd_queue =
      runtime->get_command_queue ();

    if (dxgi_rtv.handle != 0)
    {
      //
      // Cyberpunk 2077 needs a full CPU-side wait or it will crash,
      //   other games generally don't need this...
      //
      //  A fence would be best, but ReShade has no support for that.
      //
      if (SK_GetCurrentGameID () == SK_GAME_ID::Cyberpunk2077)
      {
        cmd_queue->wait_idle ();
      }

      device->destroy_resource_view (dxgi_rtv);
                                     dxgi_rtv.handle = 0;
    }

    if (has_effects)
    {
      auto backbuffer =
        runtime->get_back_buffer (runtime->get_current_back_buffer_index ());

      auto rtvDesc =
        reshade::api::resource_view_desc (static_cast <reshade::api::format> (swapDesc.BufferDesc.Format));

      if (! device->create_resource_view (backbuffer, reshade::api::resource_usage::render_target, rtvDesc, &dxgi_rtv))
        return false;

      runtime->render_effects (
        cmd_queue->get_immediate_command_list (), dxgi_rtv, { 0 }
      );
    }

    cmd_queue->flush_immediate_command_list ();

    return true;
  }

  return false;
}

bool
SK_ReShadeAddOn_Init (HMODULE reshade_module)
{
  static bool registered = false;

  if (registered)
    return true;

  registered =
    reshade::register_addon (SK_GetDLL (), reshade_module);

  if (registered)
  {
    std::wstring shared_base_path =
      SK_FormatStringW (LR"(%ws\Global\ReShade\)", SK_GetInstallPath ());

    // Create this directory so users have an easier time putting Textures and Shaders in-place.
    SK_CreateDirectories (shared_base_path.c_str ());

    config.reshade.is_addon = true;

    reshade::register_event <reshade::addon_event::init_effect_runtime>        (SK_ReShadeAddOn_InitRuntime);
    reshade::register_event <reshade::addon_event::destroy_effect_runtime>     (SK_ReShadeAddOn_DestroyRuntime);
    reshade::register_event <reshade::addon_event::reshade_overlay_activation> (SK_ReShadeAddOn_OverlayActivation);
  }

  return
    registered;
}