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
  static SK_Thread_HybridSpinlock                   _init_lock;
  std::scoped_lock <SK_Thread_HybridSpinlock> lock (_init_lock);

  SK_LOGs0 (L"ReShadeExt", L"Runtime Initialized");

#if 0
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
#endif

  ReShadeRuntimes [(HWND)runtime->get_hwnd ()] = runtime;

#if 0
  // ReShade doesn't properly handle changes to this without a full restart
  if (changed)
  {
    reshade::set_config_value (runtime, "OVERLAY", "TutorialProgress", "4");

    _flushall ();

    SK_ImGui_Warning (L"ReShade Effect Reload Required for AddOn Setup To Complete");
  }
#endif
}

struct dxgi_rtv_s {
  // Parent Information
  reshade::api::device*       device   = nullptr;

  // Resource and Fence
  reshade::api::resource_view rtv      = { 0 };
  reshade::api::resource_view rtv_srgb = { 0 };
  reshade::api::fence         fence    = { 0 };

  bool isFinished (void) const { return device->get_completed_fence_value (fence) == 1; }
  bool isValid    (void) const { return device       != nullptr &&
                                        rtv.handle   != 0       &&
                                        fence.handle != 0; }
  bool waitForGPU (void) const
  {
    while (device->get_completed_fence_value (fence) < 1)
    {
      SK_Sleep (0);
    }
    
    return true;
  }
};

Concurrency::concurrent_queue <dxgi_rtv_s> dxgi_rtvs;

reshade::api::effect_runtime*
SK_ReShadeAddOn_GetRuntimeForHWND (HWND hWnd)
{
  if (     ReShadeRuntimes.count (hWnd))
    return ReShadeRuntimes       [hWnd];

  return nullptr;
}

reshade::api::effect_runtime*
SK_ReShadeAddOn_GetRuntimeForSwapChain (IDXGISwapChain* pSwapChain)
{
  if (! pSwapChain)
    return nullptr;

  SK_ComQIPtr <IDXGISwapChain1> pSwapChain1 (pSwapChain);

  HWND                  hWnd = 0;
  pSwapChain1->GetHwnd (&hWnd);

  if (! hWnd)
    return nullptr;

  DXGI_SWAP_CHAIN_DESC  swapDesc = { };
  pSwapChain->GetDesc (&swapDesc);

  auto runtime =
    ReShadeRuntimes [hWnd];

  return runtime;
}

void
SK_ReShadeAddOn_CleanupRTVs (reshade::api::device *device, bool must_wait)
{
  if (! device)
    return;

  dxgi_rtv_s  dxgi_rtv;
  std::queue <dxgi_rtv_s> busy_rtvs;

  auto api =
    device->get_api ();

  if (api != reshade::api::device_api::d3d12 &&
      api != reshade::api::device_api::d3d11 &&
      api != reshade::api::device_api::d3d10)
  {
    return;
  }

  if (must_wait)
  {
    for ( auto& frame : _d3d12_rbk->frames_ )
      frame.wait_for_gpu ();
  }
  
  while (! dxgi_rtvs.empty ())
  {
    while (! dxgi_rtvs.try_pop (dxgi_rtv))
      ;

    if (dxgi_rtv.isValid ())
    {
      if (dxgi_rtv.isFinished () || (must_wait && dxgi_rtv.waitForGPU ()))
      {
        device->destroy_fence         (dxgi_rtv.fence);
        device->destroy_resource_view (dxgi_rtv.rtv);
        device->destroy_resource_view (dxgi_rtv.rtv_srgb);

        dxgi_rtv.rtv.handle      = 0;
        dxgi_rtv.rtv_srgb.handle = 0;
        dxgi_rtv.fence.handle    = 0;
        dxgi_rtv.device          = nullptr;

        continue;
      }
    }

    if (dxgi_rtv.isValid ())
    {
      busy_rtvs.push (dxgi_rtv);
    }
  }

  while (! busy_rtvs.empty ())
  {
    dxgi_rtvs.push (busy_rtvs.front ());
                    busy_rtvs.pop   ();
  }
}

void
SK_ReShadeAddOn_CleanupRTVs (reshade::api::effect_runtime *runtime, bool must_wait)
{
  if (! runtime)
    return;

  const auto device =
    runtime->get_device ();

  auto api =
    device->get_api ();

  if (api != reshade::api::device_api::d3d12 &&
      api != reshade::api::device_api::d3d11 &&
      api != reshade::api::device_api::d3d10)
  {
    return;
  }

  SK_ReShadeAddOn_CleanupRTVs (device, must_wait);
}

void
__cdecl
SK_ReShadeAddOn_DestroyRuntime (reshade::api::effect_runtime *runtime)
{
  SK_LOGs0 (L"ReShadeExt", L"Runtime Destroyed");

  SK_ReShadeAddOn_CleanupRTVs (runtime, true);

  if (! _d3d12_rbk->frames_.empty ())
  {
    for ( auto& frame : _d3d12_rbk->frames_ )
    {
      frame.wait_for_gpu ();
    }
  }

  ReShadeRuntimes [(HWND)runtime->get_hwnd ()] = nullptr;
}

void
__cdecl
SK_ReShadeAddOn_DestroyDevice (reshade::api::device *device)
{
  if (! device) return;

  if (! _d3d12_rbk->frames_.empty ())
  {
    for ( auto& frame : _d3d12_rbk->frames_ )
    {
      frame.wait_for_gpu ();
    }
  }

  auto api =
    device->get_api ();

  switch (api)
  {
    case reshade::api::device_api::d3d10:
    case reshade::api::device_api::d3d11:
    case reshade::api::device_api::d3d12:
    {
      SK_ReShadeAddOn_CleanupRTVs (
        device, true
      );
    } break;

    default:
      // Ignore D3D9, OpenGL and Vulkan
      break;
  }
}

void
__cdecl
SK_ReShadeAddOn_DestroySwapChain (reshade::api::swapchain *swapchain)
{
  if (! swapchain) return;

  if (! _d3d12_rbk->frames_.empty ())
  {
    for ( auto& frame : _d3d12_rbk->frames_ )
    {
      frame.wait_for_gpu ();
    }
  }

  auto api =
    swapchain->get_device ()->get_api ();

  switch (api)
  {
    case reshade::api::device_api::d3d10:
    case reshade::api::device_api::d3d11:
    case reshade::api::device_api::d3d12:
    {
      IDXGISwapChain *pSwapChain =
        reinterpret_cast <IDXGISwapChain *> (swapchain->get_native ());

      if (pSwapChain != nullptr)
      {
        SK_ReShadeAddOn_CleanupRTVs (
          SK_ReShadeAddOn_GetRuntimeForSwapChain (pSwapChain), true
        );
      }

    } break;

    default:
      // Ignore D3D9, OpenGL and Vulkan
      break;
  }
}


void
__cdecl
SK_ReShadeAddOn_DestroyCmdQueue (reshade::api::command_queue *queue)
{
  auto device =
    queue->get_device ();

  auto api =
    device->get_api ();

  if (api != reshade::api::device_api::d3d12 &&
      api != reshade::api::device_api::d3d11 &&
      api != reshade::api::device_api::d3d10)
  {
    return;
  }

  dxgi_rtv_s              dxgi_rtv;
  std::queue <dxgi_rtv_s> busy_rtvs;
  
  while (! dxgi_rtvs.empty ())
  {
    while (! dxgi_rtvs.try_pop (dxgi_rtv))
      ;

    if (dxgi_rtv.isValid ())
    {
      if (dxgi_rtv.isFinished () || dxgi_rtv.waitForGPU ())
      {
        device->destroy_fence         (dxgi_rtv.fence);
        device->destroy_resource_view (dxgi_rtv.rtv);
        device->destroy_resource_view (dxgi_rtv.rtv_srgb);

        dxgi_rtv.rtv.handle      = 0;
        dxgi_rtv.rtv_srgb.handle = 0;
        dxgi_rtv.fence.handle    = 0;
        dxgi_rtv.device          = nullptr;

        continue;
      }
    }

    else if (dxgi_rtv.isValid ())
    {
      busy_rtvs.push (dxgi_rtv);
    }
  }

  while (! busy_rtvs.empty ())
  {
    dxgi_rtvs.push (busy_rtvs.front ());
                    busy_rtvs.pop   ();
  }
}

static          bool ReShadeOverlayActive     = false; // Current overlay state
static volatile LONG ReShadeOverlayActivating = 0;     // Allow keyboard activation even if keyboard input is blocked

void
SK_ReShadeAddOn_ActivateOverlay (bool activate)
{
  std::ignore = activate;

#if 1
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

      //reshade::activate_overlay (ReShadeRuntimes [hWnd], activate, reshade::api::input_source::keyboard);
    }
  }
#endif
}

void
SK_ReShadeAddOn_ToggleOverlay (void)
{
  SK_ReShadeAddOn_ActivateOverlay (!ReShadeOverlayActive);
}

bool
__cdecl
SK_ReShadeAddOn_OverlayActivation (reshade::api::effect_runtime *runtime, bool open, reshade::api::input_source source)
{
  std::ignore = runtime;

  // Block Keyboard Activation?
  if (source == reshade::api::input_source::keyboard && ( SK_ImGui_WantKeyboardCapture () ||
                                                          SK_ImGui_WantTextCapture     () ))
  {
    // Allow activation via keyboard if we are expecting it...
    if (! InterlockedCompareExchange (&ReShadeOverlayActivating, FALSE, TRUE))
    {
      if (open)
      {
        return true;
      }
    }
  }

  // Block Gamepad Activation?
  if (source == reshade::api::input_source::gamepad && SK_ImGui_WantGamepadCapture ())
  {
    if (open)
    {
      return true;
    }
  }

  auto& io =
    ImGui::GetIO ();
  
  static bool capture_keys      = io.WantCaptureKeyboard;
  static bool capture_text      = io.WantTextInput;
  static bool capture_mouse     = io.WantCaptureMouse;
  static bool nav_active        = io.NavActive;
  static bool imgui_visible     = SK_ImGui_Visible;
  static bool imgui_vis_changed = false;

  static bool last_open = open;
  
  if (std::exchange (last_open, open) != open)
  {
    // When the overlay activates, stop blocking
    //   input !!
    if (open)
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

void
SK_ReShadeAddOn_FinishFrameDXGI (IDXGISwapChain1 *pSwapChain)
{
  std::ignore = pSwapChain;

  if (! pSwapChain)
    return;
  
  HWND                  hWnd = 0;
  pSwapChain->GetHwnd (&hWnd);
  
  if (! hWnd)
    return;
  
  DXGI_SWAP_CHAIN_DESC  swapDesc = { };
  pSwapChain->GetDesc (&swapDesc);
  
  auto runtime =
    ReShadeRuntimes [hWnd];
  
  if (runtime != nullptr)
  {
    SK_ReShadeAddOn_CleanupRTVs (runtime);
  }
}

#include <SpecialK/render/d3d11/d3d11_core.h>
extern SK_LazyGlobal <SK_D3D11_KnownShaders> SK_D3D11_Shaders;

bool
SK_ReShadeAddOn_RenderEffectsD3D11 (IDXGISwapChain1 *pSwapChain)
{
  auto runtime =
    SK_ReShadeAddOn_GetRuntimeForSwapChain (pSwapChain);

  DXGI_SWAP_CHAIN_DESC  swapDesc = { };
  pSwapChain->GetDesc (&swapDesc);

  if (runtime != nullptr)
  {
    const bool has_effects =
      runtime->get_effects_state ();

    const auto device =
      runtime->get_device ();

    const auto cmd_queue =
      runtime->get_command_queue ();

    const auto cmd_list =
      cmd_queue->get_immediate_command_list ();


    SK_ReShadeAddOn_CleanupRTVs (runtime, false);

    //
    // We have ReShade triggers, but none of them triggered... pass a null rtv for both
    //   linear and sRGB, and ReShade will skip processing this frame.
    //
    if (SK_D3D11_Shaders->hasReShadeTriggers () && (! SK_D3D11_Shaders->reshade_triggered))
    {
      runtime->render_effects (
        cmd_list, { 0 }, { 0 }
      );

      return false;
    }

    if (has_effects)
    {
      dxgi_rtv_s dxgi_rtv;

      auto backbuffer =
        runtime->get_back_buffer (runtime->get_current_back_buffer_index ());

      auto rtvDesc =
        reshade::api::resource_view_desc (static_cast <reshade::api::format> (swapDesc.BufferDesc.Format));

      if (! device->create_fence (0, reshade::api::fence_flags::none, &dxgi_rtv.fence))
        return false;

      if (DirectX::IsSRGB ((DXGI_FORMAT)rtvDesc.format))
      {
        if (! device->create_resource_view (backbuffer, reshade::api::resource_usage::render_target, rtvDesc, &dxgi_rtv.rtv_srgb))
        {
          device->destroy_fence (dxgi_rtv.fence);
          return false;
        }

        rtvDesc.format = reshade::api::format_to_default_typed (rtvDesc.format, 0);

        if (! device->create_resource_view (backbuffer, reshade::api::resource_usage::render_target, rtvDesc, &dxgi_rtv.rtv))
        {
          device->destroy_fence (dxgi_rtv.fence);
          return false;
        }
      }

      else
      {
        if (! device->create_resource_view (backbuffer, reshade::api::resource_usage::render_target, rtvDesc, &dxgi_rtv.rtv))
        {
          device->destroy_fence (dxgi_rtv.fence);
          return false;
        }

        if (DirectX::MakeSRGB (swapDesc.BufferDesc.Format) != swapDesc.BufferDesc.Format)
        {
          rtvDesc.format = reshade::api::format_to_default_typed (rtvDesc.format, 1);

          if (! device->create_resource_view (backbuffer, reshade::api::resource_usage::render_target, rtvDesc, &dxgi_rtv.rtv_srgb))
          {
            device->destroy_fence (dxgi_rtv.fence);
            return false;
          }
        }
      }

      cmd_list->barrier ( backbuffer, reshade::api::resource_usage::present,
                                      reshade::api::resource_usage::render_target );

      runtime->render_effects (
        cmd_list, dxgi_rtv.rtv, dxgi_rtv.rtv_srgb
      );

      cmd_list->barrier ( backbuffer, reshade::api::resource_usage::render_target,
                                      reshade::api::resource_usage::present );

      cmd_queue->flush_immediate_command_list ();

      if (cmd_queue->signal (dxgi_rtv.fence, 1))
      {
        dxgi_rtv.device = device;

        dxgi_rtvs.push (dxgi_rtv);

        return true;
      }

      else
      {
        SK_LOGs0 (L"ReShadeExt", L"Failed to signal RTV's fence!");

        cmd_queue->flush_immediate_command_list ();
        cmd_queue->wait_idle ();

        device->destroy_fence         (dxgi_rtv.fence);
        device->destroy_resource_view (dxgi_rtv.rtv);
        device->destroy_resource_view (dxgi_rtv.rtv_srgb);

        return true;
      }
    }
  }

  return false;
}

bool
SK_ReShadeAddOn_RenderEffectsD3D11Ex ( IDXGISwapChain1        *pSwapChain,
                                       ID3D11RenderTargetView *pRTV,
                                       ID3D11RenderTargetView *pRTV_sRGB )
{
  auto runtime =
    SK_ReShadeAddOn_GetRuntimeForSwapChain (pSwapChain);

  if (runtime != nullptr && pRTV != nullptr)
  {
    const bool has_effects =
      runtime->get_effects_state ();

    const auto cmd_queue =
      runtime->get_command_queue ();

    const auto cmd_list =
      cmd_queue->get_immediate_command_list ();


    SK_ReShadeAddOn_CleanupRTVs (runtime, false);



    //
    // We have ReShade triggers, but none of them triggered... pass a null rtv for both
    //   linear and sRGB, and ReShade will skip processing this frame.
    //
    if (SK_D3D11_Shaders->hasReShadeTriggers () && (! SK_D3D11_Shaders->reshade_triggered))
    {
      runtime->render_effects (
        cmd_list, { 0 }, { 0 }
      );

      return false;
    }


    if (has_effects)
    {
      SK_ComPtr <ID3D11Resource> pResource;
             pRTV->GetResource (&pResource.p);

      D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = { };
      pRTV->GetDesc               (&rtvDesc);

      SK_ComQIPtr <ID3D11Texture2D> pTexture (pResource);

      if (! pTexture.p)
        return false;

      bool bHasTemporaryRTV_sRGB =
        false;

      if (pRTV_sRGB == nullptr)
      {
          D3D11_RENDER_TARGET_VIEW_DESC
                             rtvDesc_sRGB = rtvDesc;

          if (rtvDesc_sRGB.Format == DXGI_FORMAT_UNKNOWN)
          {
            D3D11_TEXTURE2D_DESC texDesc = { };
            pTexture->GetDesc  (&texDesc);

            rtvDesc_sRGB.Format = texDesc.Format;
          }

          if (DirectX::IsTypeless        (rtvDesc_sRGB.Format))
                                          rtvDesc_sRGB.Format =
              DirectX::MakeTypelessUNORM (rtvDesc_sRGB.Format);

                             rtvDesc_sRGB.Format =
          DirectX::MakeSRGB (rtvDesc_sRGB.Format);
        if (DirectX::IsSRGB (rtvDesc_sRGB.Format))
        {
          SK_ComPtr <ID3D11Device> pDev;
                 pRTV->GetDevice (&pDev);

          if (pDev.p != nullptr)
          {
            if (
              SUCCEEDED (
                pDev->CreateRenderTargetView (pResource, &rtvDesc_sRGB, &pRTV_sRGB)
                        )
               )
            {
              bHasTemporaryRTV_sRGB = true;
            }
          }
        }
      }

      runtime->render_effects (
        cmd_list, reshade::api::resource_view { (uint64_t) pRTV                      },
                                              { (uint64_t)(pRTV_sRGB != nullptr ?
                                                           pRTV_sRGB            : 0) }
      );

      cmd_queue->flush_immediate_command_list ();

      if (bHasTemporaryRTV_sRGB)
        pRTV_sRGB->Release ();

      return true;
    }
  }

  return false;
}

void
SK_ReShadeAddOn_Present (IDXGISwapChain *pSwapChain)
{
  auto runtime =
    SK_ReShadeAddOn_GetRuntimeForSwapChain (pSwapChain);

  if (runtime != nullptr)
  {
    runtime->render_effects ( nullptr, { 0 }, { 0 });
  }
}

UINT64
SK_ReShadeAddOn_RenderEffectsD3D12 (IDXGISwapChain1 *pSwapChain, ID3D12Resource* pResource, ID3D12Fence* pFence, D3D12_CPU_DESCRIPTOR_HANDLE hRTV, D3D12_CPU_DESCRIPTOR_HANDLE hRTV_sRGB)
{
  if (! _d3d12_rbk->_pReShadeRuntime)
    return 0;

  static volatile LONG64 lLastFrame = 0;

  if (InterlockedExchange64 (&lLastFrame, (LONG64)SK_GetFramesDrawn ()) == (LONG64)SK_GetFramesDrawn ())
    return 0;

  static volatile UINT64 uiFenceVal = 0;

  auto runtime =
    SK_ReShadeAddOn_GetRuntimeForSwapChain (pSwapChain);

  if (runtime != nullptr)
  {
    const bool has_effects = runtime->get_effects_state ();
    const auto cmd_queue   = runtime->get_command_queue ();
          auto cmd_list    = (reshade::api::command_list *)nullptr;

    if ( nullptr ==             cmd_queue ||
         nullptr == (cmd_list = cmd_queue->get_immediate_command_list ()) )
    {
      return 0;
    }

    SK_ReShadeAddOn_CleanupRTVs (runtime, pFence == nullptr);

    if (has_effects)
    {
            auto buffer   = reshade::api::resource      { reinterpret_cast <uint64_t> (pResource)     };
      const auto rtv      = reshade::api::resource_view { static_cast      <uint64_t> (hRTV.     ptr) };
      const auto rtv_srgb = reshade::api::resource_view { static_cast      <uint64_t> (hRTV_sRGB.ptr) };
      const auto fence    = reshade::api::fence         { reinterpret_cast <uint64_t> (pFence)        };

      if (pResource == nullptr)
      {
        buffer = runtime->get_device ()->get_resource_from_view (rtv);
      }


      // Barriers won't be needed if we call this from the correct pre-transitioned state
      //cmd_list->barrier ( buffer, reshade::api::resource_usage::present,
      //                            reshade::api::resource_usage::render_target );
      {
        //if (runtime->get_device ()->get_resource_desc (buffer).texture.width  == ImGui::GetIO ().DisplaySize.x &&
        //    runtime->get_device ()->get_resource_desc (buffer).texture.height == ImGui::GetIO ().DisplaySize.y)
        {
          runtime->render_effects (
            cmd_list, rtv, rtv_srgb
          );
        }

        //else
        //{
        //  InterlockedExchange64 (&lLastFrame, (LONG64)SK_GetFramesDrawn () - 1);
        //}
      }
      //cmd_list->barrier ( buffer, reshade::api::resource_usage::render_target,
      //                            reshade::api::resource_usage::present );

      if (pFence != nullptr)
      {
        const UINT64 uiNextFenceVal =
          InterlockedIncrement (&uiFenceVal);

        if (cmd_queue->signal (fence, uiNextFenceVal))
        {
          cmd_queue->flush_immediate_command_list ();

          return
            uiNextFenceVal;
        }
      }

      SK_LOGs0 (L"ReShadeExt", L"Failed to signal RTV's fence!");

      cmd_queue->flush_immediate_command_list ();
      cmd_queue->wait_idle ();
    }
  }

  return 0;
}

void SK_ReShadeAddOn_Present (reshade::api::command_queue *queue, reshade::api::swapchain *swapchain, const reshade::api::rect *source_rect, const reshade::api::rect *dest_rect, uint32_t dirty_rect_count, const reshade::api::rect *dirty_rects)
{
  if (swapchain->get_device ()->get_api () == reshade::api::device_api::d3d12)
  {
    auto runtime =
      SK_ReShadeAddOn_GetRuntimeForHWND ((HWND)swapchain->get_hwnd ());

    SK_ReShadeAddOn_CleanupRTVs (runtime, false);
  }

  std::ignore = dirty_rects;
  std::ignore = dirty_rect_count;
  std::ignore = dest_rect;
  std::ignore = source_rect;
  std::ignore = queue;
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

    reshade::register_event <reshade::addon_event::present>                (SK_ReShadeAddOn_Present);
    reshade::register_event <reshade::addon_event::init_effect_runtime>    (SK_ReShadeAddOn_InitRuntime);
    reshade::register_event <reshade::addon_event::destroy_effect_runtime> (SK_ReShadeAddOn_DestroyRuntime);
    reshade::register_event <reshade::addon_event::destroy_device>         (SK_ReShadeAddOn_DestroyDevice);
    reshade::register_event <reshade::addon_event::destroy_swapchain>      (SK_ReShadeAddOn_DestroySwapChain);
    reshade::register_event <reshade::addon_event::destroy_command_queue>  (SK_ReShadeAddOn_DestroyCmdQueue);
    reshade::register_event <reshade::addon_event::reshade_open_overlay>   (SK_ReShadeAddOn_OverlayActivation);
  }

  return
    registered;
}


void
SK_ReShadeAddOn_UpdateAndPresentEffectRuntime (reshade::api::effect_runtime *runtime)
{
  static auto &rb =
    SK_GetCurrentRenderBackend ();

  if ( rb.isHDRCapable () &&
       rb.isHDRActive  () )
  {
    switch (rb.scanout.getEOTF ())// == SK_RenderBackend::scan_out_s::SMPTE_2084)
    {
      case SK_RenderBackend::scan_out_s::sRGB:
      case SK_RenderBackend::scan_out_s::G22:
        runtime->set_color_space (reshade::api::color_space::srgb_nonlinear);
        break;
      case SK_RenderBackend::scan_out_s::Linear:
        runtime->set_color_space (reshade::api::color_space::extended_srgb_linear);
        break;
      case SK_RenderBackend::scan_out_s::SMPTE_2084:
        runtime->set_color_space (reshade::api::color_space::hdr10_st2084);
        break;
      default:
        runtime->set_color_space (reshade::api::color_space::unknown);
        break;
    }
  }

  else
  {
    runtime->set_color_space (reshade::api::color_space::srgb_nonlinear);
  }

  reshade::update_and_present_effect_runtime (runtime);
}

void
SK_ReShadeAddOn_DestroyEffectRuntime (reshade::api::effect_runtime *runtime)
{
  runtime->get_command_queue ()->wait_idle ();

  reshade::destroy_effect_runtime (runtime);
}

void SK_ReShadeAddOn_SetupInitialINI (const wchar_t* wszINIFile)
{
  if (! PathFileExistsW (wszINIFile))
  {
    FILE *fReShadeINI =
      _wfopen (wszINIFile, L"w");

    if (fReShadeINI != nullptr)
    {
      std::wstring shared_base_path =
        SK_FormatStringW (LR"(%ws\Global\ReShade\)", SK_GetInstallPath ());

      fputws (L"[GENERAL]",                                                                                                                 fReShadeINI);
      fputws (SK_FormatStringW (LR"(EffectSearchPaths=%wsShaders\**,.\reshade-shaders\Shaders\**)",    shared_base_path.c_str ()).c_str (), fReShadeINI);
      fputws (SK_FormatStringW (LR"(TextureSearchPaths=%wsTextures\**,.\reshade-shaders\Textures\**)", shared_base_path.c_str ()).c_str (), fReShadeINI);

      fclose (fReShadeINI);
    }
  }
}

bool SK_ReShadeAddOn_HadLocalINI = true;

reshade::api::effect_runtime*
SK_ReShadeAddOn_CreateEffectRuntime_D3D11 (ID3D11Device *pDevice, ID3D11DeviceContext *pDevCtx, IDXGISwapChain *pSwapChain)
{
  reshade::api::effect_runtime *runtime = nullptr;

  if (GetEnvironmentVariableW (L"RESHADE_DISABLE_GRAPHICS_HOOK", nullptr, 1) != 0)
  {
    SK_ComQIPtr <IDXGISwapChain3> swapchain (pSwapChain);

    SK_RunOnce (
      SK_ReShadeAddOn_HadLocalINI =
        PathFileExistsW (L"ReShade.ini")
    );

    // Delete the INI file that some versions of ReShade 5.9.3 write (bug) to the wrong path
    if (! SK_ReShadeAddOn_HadLocalINI)
    {
      DeleteFileW (L"ReShade.ini");
    }

    static std::filesystem::path
      reshade_path (SK_ReShadeAddOn_HadLocalINI ?
                                 L"ReShade.ini" : SK_GetConfigPath ());

    SK_RunOnce (
      if (! SK_ReShadeAddOn_HadLocalINI)
        reshade_path /= L"ReShade/ReShade.ini";
    );

    if (! reshade::create_effect_runtime (reshade::api::device_api::d3d11, pDevice, pDevCtx, swapchain, (const char *)reshade_path.u8string ().c_str (), &runtime))
    {
      return nullptr;
    }
  }

  return runtime;
}

reshade::api::effect_runtime*
SK_ReShadeAddOn_CreateEffectRuntime_D3D12 (ID3D12Device *pDevice, ID3D12CommandQueue *pCmdQueue, IDXGISwapChain *pSwapChain)
{
  reshade::api::effect_runtime *runtime = nullptr;

  if (GetEnvironmentVariableW (L"RESHADE_DISABLE_GRAPHICS_HOOK", nullptr, 1) != 0)
  {
    SK_ComQIPtr <IDXGISwapChain3> swapchain (pSwapChain);

    SK_RunOnce (
      SK_ReShadeAddOn_HadLocalINI = PathFileExistsW (L"ReShade.ini");
    );

    // Delete the INI file that some versions of ReShade 5.9.3 write (bug) to the wrong path
    if (! SK_ReShadeAddOn_HadLocalINI)
    {
      DeleteFileW (L"ReShade.ini");
    }

    static std::filesystem::path
      reshade_path (SK_ReShadeAddOn_HadLocalINI ?
                                 L"ReShade.ini" : SK_GetConfigPath ());

    SK_RunOnce (
      if (! SK_ReShadeAddOn_HadLocalINI)
        reshade_path /= L"ReShade/ReShade.ini";
    );

    if (! reshade::create_effect_runtime (reshade::api::device_api::d3d12, pDevice, pCmdQueue, swapchain, (const char *)reshade_path.u8string ().c_str (), &runtime))
    {
      return nullptr;
    }

    runtime->get_command_queue ()->wait_idle ();
  }

  return runtime;
}

void
SK_ReShadeAddOn_CleanupConfigAndLogs (void)
{
  // Fix bug in ReShade 5.9.3, where it writes an INI file to the wrong location
  if (GetEnvironmentVariableW (L"RESHADE_DISABLE_GRAPHICS_HOOK", nullptr, 1) != 0)
  {
    // Didn't have a local INI file originally, and we don't want one now either!
    if (! SK_ReShadeAddOn_HadLocalINI)
    {
      DeleteFileW (L"ReShade.ini");
      DeleteFileW (L"ReShade.log");
    }
  }
}