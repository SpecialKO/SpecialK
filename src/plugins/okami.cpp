//
// Copyright 2017-2018 Andon "Kaldaien" Coleman
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
#include <SpecialK/render/d3d11/d3d11_core.h>
#include <imgui/imgui.h>

sk::ParameterBool*   sk_okami_grain;

bool SK_Okami_use_grain = true;

extern iSK_INI* dll_ini;

void
WINAPI
SK_D3D11_AddTexHash ( const wchar_t* name, uint32_t top_crc32, uint32_t hash );


typedef bool (__cdecl *m2_WindowControl_resizeBackBuffers_pfn)(LPVOID This, unsigned int, unsigned int, bool);
typedef bool (__cdecl *m2_WindowControl_resizeRenderBuffers_pfn)(LPVOID This, unsigned int, unsigned int, bool);

m2_WindowControl_resizeBackBuffers_pfn   m2_WindowControl_resizeBackBuffers_Original   = nullptr;
m2_WindowControl_resizeRenderBuffers_pfn m2_WindowControl_resizeRenderBuffers_Original = nullptr;

bool
SK_Okami_m2_WindowControl_resizeBackBuffers_Detour (LPVOID This, unsigned short width, unsigned short height, bool unknown)
{
  dll_log->Log (L"m2::WindowControl::resizeBackBuffers (%ph, %ux%u, %lu)", This, width, height, unknown);

  if ((! config.window.res.override.isZero ()) && width > 853)
  {
    width  = (unsigned short)config.window.res.override.x;
    height = (unsigned short)config.window.res.override.y;
    //unknown = false;
  }

  return m2_WindowControl_resizeBackBuffers_Original (This, width, height, unknown);
}

bool
SK_Okami_m2_WindowControl_resizeRenderBuffers_Detour (LPVOID This, unsigned int width, unsigned int height, bool unknown)
{
  dll_log->Log (L"m2::WindowControl::resizeRenderBuffers (%ph, %lux%lu, %lu)", This, width, height, unknown);

  if ((! config.window.res.override.isZero ()) && width > 853)
  {
    width  = config.window.res.override.x;
    height = config.window.res.override.y;
    //unknown = false;
  }

  return m2_WindowControl_resizeRenderBuffers_Original (This, width, height, unknown);
}

#include <SpecialK/utility.h>

void
SK_Okami_LoadConfig (void)
{
  sk_okami_grain =
    dynamic_cast <sk::ParameterBool *> (
      g_ParameterFactory->create_parameter <bool> (
        L"Use Grain?"
      )
    );

  sk_okami_grain->register_to_ini (
    dll_ini,
      L"OKAMI HD / 大神 絶景版",
        L"EnableGrain" );

  sk_okami_grain->load (SK_Okami_use_grain);

  if (SK_Okami_use_grain)
  {
    SK_D3D11_AddTexHash (L"grain.dds", 0xced133fb, 0x00);
  }

  else
  {
    SK_D3D11_AddTexHash (L"no_grain.dds", 0xced133fb, 0x00);
  }

  ////if (SK_IsInjected ())
  ////{
  ////  SK_CreateDLLHook2 (                      L"main.dll",
  ////                                            "?resizeBackBuffers@WindowControl@m2@@QEAA_NII_N@Z",
  ////                    SK_Okami_m2_WindowControl_resizeBackBuffers_Detour,
  ////    static_cast_p2p <void> (&m2_WindowControl_resizeBackBuffers_Original) );
  ////  SK_CreateDLLHook2 (                      L"main.dll",
  ////                                            "?resizeRenderBuffers@WindowControl@m2@@QEAA_NGG_N@Z",
  ////                    SK_Okami_m2_WindowControl_resizeRenderBuffers_Detour,
  ////    static_cast_p2p <void> (&m2_WindowControl_resizeRenderBuffers_Original) );
  ////
  ////  SK_ApplyQueuedHooks ( );
  ////}
}

void
SK_Okami_SaveConfig (void)
{
  sk_okami_grain->store (SK_Okami_use_grain);
  dll_ini->write (dll_ini->get_filename ());
}



bool
SK_Okami_PlugInCfg (void)
{
  static auto cp =
    SK_GetCommandProcessor ();

  if (ImGui::CollapsingHeader (u8"OKAMI HD / 大神 絶景版", ImGuiTreeNodeFlags_DefaultOpen))
  {
    struct patch_addr_s {
      void*       addr        = nullptr;
      const char* orig_bytes  = nullptr;
      const char* patch_bytes = nullptr;
      size_t      size        = 0;
      bool        enabled     = false;
    };

    void*
    __stdcall
    SK_ScanAlignedEx2 (const void* pattern, size_t len, const void* mask, void* after, int align, uint8_t* base_addr);

    static patch_addr_s addrs [] = {
      { nullptr, "\xC6\x05\x48\x0A\xA2\x00\x02", "\x90\x90\x90\x90\x90\x90\x90", 7, false },
      { nullptr, "\xC6\x05\xED\x0B\xA2\x00\x02", "\x90\x90\x90\x90\x90\x90\x90", 7, false },
      { nullptr, "\xC6\x05\xDD\x64\x6D\x00\x02", "\x90\x90\x90\x90\x90\x90\x90", 7, false },
      { nullptr, "\xC6\x05\x99\x65\x75\x00\x02", "\x90\x90\x90\x90\x90\x90\x90", 7, false },

      { nullptr, "\xC7\x05\x42\x49\x6B\x00\x00\x00\x80\x3F", "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90", 10, false },
      { nullptr, "\xC7\x05\x16\x49\x6B\x00\x00\x00\x00\x3F", "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90", 10, false }
    };

    if (addrs [0].addr == nullptr)
    {
      int idx = 0;

      for ( auto& entry : addrs )
      {
        entry.addr =
          SK_ScanAlignedEx2 (entry.orig_bytes, entry.size, nullptr, nullptr, 1, reinterpret_cast <uint8_t *> (GetModuleHandle (L"main.dll")));
        dll_log->Log (L"[Okami60FPS] Patch Address #%lu: %ph", idx++, entry.addr);
      }

      if (addrs [0].addr == nullptr)
      {
        addrs [0].addr = (void *)0x1;
      }
    }


    if (addrs [0].addr != nullptr && reinterpret_cast <uintptr_t> (addrs [0].addr) != 0x1)
    {
      bool enabled =
        addrs [0].enabled;

      if (ImGui::Checkbox ("60 FPS", &enabled))
      {
        std::queue <DWORD> tids =
          SK_SuspendAllOtherThreads ();

        int idx = 0;

        for ( auto& entry : addrs )
        {
          if ( entry.addr != nullptr)
          {
            DWORD dwOrig = 0x0;

            VirtualProtect ( entry.addr,           entry.size, PAGE_EXECUTE_READWRITE, &dwOrig );
            memcpy         ( entry.addr, enabled ? entry.patch_bytes : entry.orig_bytes,
                                                   entry.size                                  );
            VirtualProtect ( entry.addr,           entry.size, dwOrig,                 &dwOrig );

            entry.enabled = enabled;
          }

          else
          {
            dll_log->Log (L" Missing Pattern #%lu", idx);
          }

          idx++;
        }

        static DWORD*   dwTick0     = (DWORD *)(((uint8_t *)GetModuleHandle (L"main.dll"))   + 0xB6AC3C);
        static float*   fTickScale  = (float *)(((uint8_t *)GetModuleHandle (L"main.dll"))   + 0xB6AC38);
        static float*   fTickScale0 = (float *)(((uint8_t *)GetModuleHandle (L"main.dll"))   + 0xB6ACC0);
        static uint8_t* bTick1      = (uint8_t *)(((uint8_t *)GetModuleHandle (L"main.dll")) + 0xB6AC45);

        DWORD dwOrig = 0x0;
        //VirtualProtect ( dwTick0, 4, PAGE_EXECUTE_READWRITE, &dwOrig );
        //*dwTick0 = enabled ? 2 : 4;
        //VirtualProtect ( dwTick0, 4, dwOrig, &dwOrig);

        VirtualProtect ( fTickScale, 4, PAGE_EXECUTE_READWRITE, &dwOrig );
        *fTickScale = enabled ? 0.5f : 1.0f;
        VirtualProtect ( fTickScale, 4, dwOrig, &dwOrig);

        VirtualProtect ( fTickScale0, 4, PAGE_EXECUTE_READWRITE, &dwOrig );
        *fTickScale0 = enabled ? 0.5f : 1.0f;
        VirtualProtect ( fTickScale0, 4, dwOrig, &dwOrig);

        VirtualProtect ( bTick1, 1, PAGE_EXECUTE_READWRITE, &dwOrig );
        *bTick1 = enabled ? 1 : 2;
        VirtualProtect ( bTick1, 1, dwOrig, &dwOrig);

        if (enabled) { cp->ProcessCommandLine ("TargetFPS 60.0"); cp->ProcessCommandLine ("PresentationInterval 1"); }
        else         { cp->ProcessCommandLine ("TargetFPS 30.0"); cp->ProcessCommandLine ("PresentationInterval 2"); }

        SK_ResumeThreads (tids);
      }

      if (enabled)
      {
        DWORD dwOrig = 0x0;

        static float*   fTickScale  = (float *)(((uint8_t *)GetModuleHandle (L"main.dll"))   + 0xB6AC38);
        static float*   fTickScale0 = (float *)(((uint8_t *)GetModuleHandle (L"main.dll"))   + 0xB6ACC0);

        ImGui::BeginGroup ();
        VirtualProtect ( fTickScale, 4, PAGE_EXECUTE_READWRITE, &dwOrig );
        ImGui::SliderFloat ("Physics", fTickScale, 0.25f, 2.5f);
        VirtualProtect ( fTickScale, 4, dwOrig, &dwOrig);

        VirtualProtect ( fTickScale0, 4, PAGE_EXECUTE_READWRITE, &dwOrig );
        ImGui::SliderFloat ("Clockrate", fTickScale0, 0.15f, 5.0f);
        VirtualProtect ( fTickScale0, 4, dwOrig, &dwOrig);
        ImGui::EndGroup ();

        ImGui::SameLine ();

        ImGui::BeginGroup ();
        ImGui::BeginGroup ();
        if (ImGui::Button ("Bullet Time"))        *fTickScale  = 0.25f;
        if (ImGui::Button ("Paint dries faster")) *fTickScale0 = 0.15f;
        ImGui::EndGroup   ();
        ImGui::SameLine   ();
        ImGui::BeginGroup ();
        if (ImGui::Button ("Bouncy Castle"))                   *fTickScale  = 2.5f;
        if (ImGui::Button ("Ain't nobody got time for that!")) *fTickScale0 = 5.0f;
        ImGui::EndGroup   ();
        ImGui::EndGroup   ();
        VirtualProtect ( fTickScale , 4, dwOrig, &dwOrig);
        VirtualProtect ( fTickScale0, 4, dwOrig, &dwOrig);
      }

      ImGui::Separator ();
    }


    bool motion_blur, bloom,
         smoke,       HUD;

    motion_blur = (! SK_D3D11_Shaders->pixel.blacklist.count  (0x06ef081f));
    bloom       = (! SK_D3D11_Shaders->pixel.blacklist.count  (0x939da69c));
    smoke       = (! SK_D3D11_Shaders->vertex.blacklist.count (0xbe4b62c2));

    HUD         = (SK_D3D11_Shaders->pixel.blacklist_if_texture [0xec31f12f].empty ());

    ImGui::TreePush ("");

    bool changed = false;

    ImGui::BeginGroup ();

    if (ImGui::Checkbox ("Motion Blur", &motion_blur))
    {
      changed = true;

      cp->ProcessCommandLine ("D3D11.ShaderMods.Toggle Pixel 06ef081f Disable");
    }

    if (ImGui::Checkbox ("Smoke", &smoke))
    {
      changed = true;

      cp->ProcessCommandLine ("D3D11.ShaderMods.Toggle Vertex be4b62c2 Disable");
    }

    ImGui::EndGroup   ();
    ImGui::SameLine   ();
    ImGui::BeginGroup ();

    if (ImGui::Checkbox ("Bloom", &bloom))
    {
      changed = true;

      cp->ProcessCommandLine ("D3D11.ShaderMods.Toggle Pixel 939da69c Disable");
      cp->ProcessCommandLine ("D3D11.ShaderMods.Toggle Pixel fa2af8ba Disable");
      cp->ProcessCommandLine ("D3D11.ShaderMods.Toggle Pixel c75b0341 Disable");
    }

    if (ImGui::Checkbox ("Grain", &SK_Okami_use_grain))
    {
      void
      WINAPI
      SK_D3D11_AddTexHash ( const wchar_t* name, uint32_t top_crc32, uint32_t hash );

      void
      WINAPI
      SK_D3D11_RemoveTexHash (uint32_t top_crc32, uint32_t hash);

      if (SK_Okami_use_grain)
      {
        SK_D3D11_RemoveTexHash (              0xced133fb, 0x00);
        SK_D3D11_AddTexHash    (L"grain.dds", 0xced133fb, 0x00);
      }

      else
      {
        SK_D3D11_RemoveTexHash (                 0xced133fb, 0x00);
        SK_D3D11_AddTexHash    (L"no_grain.dds", 0xced133fb, 0x00);
      }

      extern int
      SK_D3D11_ReloadAllTextures (void);
      SK_D3D11_ReloadAllTextures ();

      SK_Okami_SaveConfig ();
    }

    ImGui::EndGroup ();
    ImGui::SameLine ();

    if (ImGui::Checkbox ("HUD", &HUD))
    {
      changed = true;

      cp->ProcessCommandLine ("D3D11.ShaderMods.ToggleConfig d3d11_shaders-nohud.ini");
    }

    //ImGui::InputInt2 ("Forced Internal Resolution", )

    if (changed)
    {
      extern void
      SK_D3D11_StoreShaderState (void);
      SK_D3D11_StoreShaderState ();
    }

    ImGui::TreePop ();

    return true;
  }

  return false;
}