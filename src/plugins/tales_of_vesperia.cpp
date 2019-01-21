//
// Copyright 2019 Andon "Kaldaien" Coleman
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


#include <SpecialK/log.h>
#include <SpecialK/hooks.h>
#include <SpecialK/config.h>
#include <SpecialK/utility.h>
#include <SpecialK/parameter.h>
#include <SpecialK/plugin/plugin_mgr.h>

#include <SpecialK/render/dxgi/dxgi_backend.h>

#include <imgui/imgui.h>

#define TVFIX_VERSION_NUM L"0.4.0"
#define TVFIX_VERSION_STR LR"(Tales of Vesperia "Fix" v )" TVFIX_VERSION_NUM

extern iSK_INI*             dll_ini;
extern sk::ParameterFactory g_ParameterFactory;

struct tv_mem_addr_s
{
  const char*    pattern        = nullptr;
  const char*    pattern_mask   = nullptr;
  size_t         pattern_len    =       0;

  size_t         rep_size       =       0;
  ptrdiff_t      rep_off        =       0;

  bool           enabled        =   false;
  void*          scanned_addr   = nullptr;

  std::vector<BYTE> orig_bytes  =     { };

  const wchar_t* desc           = nullptr;
  void*          expected_addr  = nullptr;

  void scan (void)
  {
    if (! scanned_addr)
    {
      // First try the expected addressexpected_addr
      if (expected_addr != nullptr)
      {
        void* expected =
          (void *)((uintptr_t)GetModuleHandle (nullptr) + (uintptr_t)expected_addr);

        __try
        {
          if (! memcmp (pattern, expected, pattern_len))
            scanned_addr = expected;
        }
        
        __except ( ( GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ) ?
                 EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH )
        {
        }

        if (scanned_addr == nullptr)
        {
          scanned_addr =
            SK_ScanAlignedEx (pattern, pattern_len, pattern_mask, (void *)((uintptr_t)GetModuleHandle (nullptr) + (uintptr_t)expected_addr));
        }
      }

      // Fallback to exhaustive search if not there
      if (scanned_addr == nullptr)
      {
        scanned_addr =
          SK_ScanAlignedEx (pattern, pattern_len, pattern_mask);
      }

      dll_log.Log (L"Scanned address for: %s: %p (alignment=%lu)", desc, scanned_addr, (uintptr_t)scanned_addr % 16);

      if (scanned_addr != nullptr)
      {
        orig_bytes.clear ();

        for ( UINT i = 0; i < rep_size; i++ )
        {
          orig_bytes.push_back (*(uint8_t *)((uintptr_t)scanned_addr + rep_off + i));
        }
      }
    }
  }

  bool toggle (void)
  {
    if (enabled)
      disable ();
    else
      enable ();

    return
      enabled;
  }

  void enable (void)
  {
    scan ();

    if (scanned_addr != nullptr && (! enabled))
    {
      DWORD dwProtect;

      VirtualProtect ((void*)((intptr_t)scanned_addr + rep_off), rep_size, PAGE_EXECUTE_READWRITE, &dwProtect);

      if (rep_size == 8 && (! ((intptr_t)scanned_addr % 8)))
      {
        InterlockedExchange64 ((volatile LONG64*)((intptr_t)scanned_addr + rep_off), *(__int64 *)orig_bytes.data ());
      }

      else if (rep_size == 2 && (! ((intptr_t)scanned_addr % 2)))
      {
        InterlockedExchange16 ((volatile SHORT*)((intptr_t)scanned_addr + rep_off), *(SHORT *)"\x77\xBD");
      }

      // Other sizes are not atomic, so... not threadsafe
      else
      {
        memcpy ((void *)((intptr_t)scanned_addr + rep_off), orig_bytes.data (), rep_size);
      }

      VirtualProtect ((void*)((intptr_t)scanned_addr + rep_off), rep_size, dwProtect, &dwProtect);

      enabled = true;
    }
  }

  void disable (void)
  {
    scan ();

    if (scanned_addr != nullptr && enabled)
    {
      DWORD dwProtect;

      VirtualProtect ((void*)((intptr_t)scanned_addr + rep_off), rep_size, PAGE_EXECUTE_READWRITE, &dwProtect);

      if (rep_size == 8 && (! ((intptr_t)scanned_addr % 8)))
      {
        InterlockedExchange64 ((volatile LONG64*)((intptr_t)scanned_addr + rep_off), *(LONG64 *)"\x90\x90\x90\x90\x90\x90\x90\x90");
      }

      else if (rep_size == 2 && (! ((intptr_t)scanned_addr % 2)))
      {
        InterlockedExchange16 ((volatile SHORT*)((intptr_t)scanned_addr + rep_off), *(SHORT *)"\x90\x90");
      }

      else
      {
        memset ((void *)((intptr_t)scanned_addr + rep_off), 0x90, rep_size);
      }

      VirtualProtect ((void*)((intptr_t)scanned_addr + rep_off), rep_size, dwProtect, &dwProtect);

      enabled = false;
    }
  }
};


bool __SK_TVFix_NoRenderSleep = true;

tv_mem_addr_s instn__model_animation  = { };
tv_mem_addr_s instn__particle_effects = { };
tv_mem_addr_s instn__blur             = { };
tv_mem_addr_s instn__depth_of_field   = { };
tv_mem_addr_s instn__bloom            = { };
tv_mem_addr_s instn__draw_HUD         = { };

// TOV_DE.exe+72AE99 - E8 92C0FCFF           - call TOV_DE.exe+6F6F30 { Depth of Field }
// TOV_DE.exe+6F632A - E8 C15C0300           - call TOV_DE.exe+72BFF0 { Bloom Lighting }
// TOV_DE.exe+6F6375 - E8 A69E0300           - call TOV_DE.exe+730220 { Bloom Lighting 2 }


sk::ParameterBool* _SK_TVFix_DisableDepthOfField;
bool              __SK_TVFix_DisableDepthOfField = false;

sk::ParameterBool* _SK_TVFix_DisableBlur;
bool              __SK_TVFix_DisableBlur = false;

sk::ParameterBool* _SK_TVFix_DisableBloom;
bool              __SK_TVFix_DisableBloom = false;

sk::ParameterBool* _SK_TVFix_ActiveAntiStutter;
bool              __SK_TVFix_ActiveAntiStutter = true;

static volatile LONG __TVFIX_init = 0;

HRESULT
STDMETHODCALLTYPE
SK_TVFIX_PresentFirstFrame (IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
  UNREFERENCED_PARAMETER (pSwapChain);
  UNREFERENCED_PARAMETER (SyncInterval);
  UNREFERENCED_PARAMETER (Flags);

  if (! InterlockedCompareExchange (&__TVFIX_init, 1, 0))
  { 
    SK_D3D11_DeclHUDShader_Vtx (0xb0831a43);
    SK_D3D11_DeclHUDShader_Vtx (0xf4dac9d5);
  //SK_D3D11_DeclHUDShader_Pix (0x6d243285);

    instn__model_animation.scan  ();
    instn__particle_effects.scan ();
    instn__depth_of_field.scan   ();
    instn__blur.scan             ();
    instn__bloom.scan            ();

    _SK_TVFix_DisableDepthOfField =
      _CreateConfigParameterBool ( L"TVFix.Render",
                                   L"DisableDepthOfField",  __SK_TVFix_DisableDepthOfField,
                                   L"Disable Depth of Field" );

    _SK_TVFix_DisableBloom =
      _CreateConfigParameterBool ( L"TVFix.Render",
                                   L"DisableBloom",  __SK_TVFix_DisableBloom,
                                   L"Disable Bloom Lighting" );

    _SK_TVFix_DisableBlur =
      _CreateConfigParameterBool ( L"TVFix.Render",
                                   L"DisableBlur",  __SK_TVFix_DisableBlur,
                                   L"Disable Blur" );

    _SK_TVFix_ActiveAntiStutter =
      _CreateConfigParameterBool ( L"TVFix.FrameRate",
                                   L"EliminateMicroStutter", __SK_TVFix_ActiveAntiStutter,
                                   L"Active Anti-Stutter" );

    if (__SK_TVFix_DisableDepthOfField)
    {
      instn__depth_of_field.disable ();
    }

    if (__SK_TVFix_DisableBloom)
    {
      instn__bloom.disable ();
    }

    if (__SK_TVFix_DisableBlur)
    {
      instn__blur.disable ();
    }
  }

  return S_OK;
}

void
SK_TVFix_InitPlugin (void)
{
  SK_SetPluginName (TVFIX_VERSION_STR);

    //instn__draw_HUD

  instn__model_animation =
  { "\xE8\xCE\x1C\x00\x00",
    //----------------------------------------------//
    "\xFF\xFF\xFF\xFF\xFF",
    5, 5, 0,    true, nullptr,

    { }, L"Enable Model Animation", (void *)0x6F7EED };

  instn__particle_effects =
  { "\xE8\xA6\xD3\xFF\xFF",
    //----------------------------------------------//
    "\xFF\xFF\xFF\xFF\xFF",
    5, 5, 0,    true, nullptr,

    { }, L"Enable Particle Effects", (void *) 0x6F7EF5 };

  //"\xE8\xC6\xE2\xFF\xFF", -- All Post-Processing

  instn__blur =
  {
    "\xE8\x92\xC0\xFC\xFF",
    
    //----------------------------------------------//
    "\xFF\xFF\xFF\xFF\xFF",
    5, 5, 0,    true, nullptr,

    { }, L"Enable Blur", (void *)0x72AE99 };

  instn__depth_of_field =
  {
    "\xE8\xA6\x9E\x03\x00",
    //----------------------------------------------//
    "\xFF\xFF\xFF\xFF\xFF",
    5, 5, 0,    true, nullptr,

    { }, L"Enable Depth of Field", (void *)0x6F6375 };

  instn__bloom =
  {
    "\xE8\xC1\x5C\x03\x00",

    //----------------------------------------------//
    "\xFF\xFF\xFF\xFF\xFF",
    5, 5, 0,    true, nullptr,

    { }, L"Enable Bloom Lighting", (void *)0x6f632a };
}

bool
SK_TVFix_PlugInCfg (void)
{
  if (ImGui::CollapsingHeader ("Tales of Vesperia Definitive Edition", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::TreePush ("");

    if (ImGui::Checkbox ("Aggressive Anti-Stutter", &__SK_TVFix_ActiveAntiStutter))
    {
      _SK_TVFix_ActiveAntiStutter->store (__SK_TVFix_ActiveAntiStutter);
    }

    if (ImGui::IsItemHovered ())
      ImGui::SetTooltip ("Eliminate Microstutter, but will raise CPU usage %");

    ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.02f, 0.68f, 0.90f, 0.45f));
    ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.07f, 0.72f, 0.90f, 0.80f));
    ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.14f, 0.78f, 0.87f, 0.80f));

    ImGui::BeginGroup ();
    if (ImGui::CollapsingHeader ("Post-Processing", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowOverlapMode))
    {
      ImGui::TreePush ("");

      if ( instn__depth_of_field.scanned_addr != nullptr &&
           ImGui::Checkbox ("Enable Depth of Field", &instn__depth_of_field.enabled) )
      {
        instn__depth_of_field.enabled = !instn__depth_of_field.enabled;
        instn__depth_of_field.toggle ();

        _SK_TVFix_DisableDepthOfField->store (! instn__depth_of_field.enabled);
      }

      if ( instn__bloom.scanned_addr != nullptr &&
           ImGui::Checkbox ("Enable Atmospheric Bloom", &instn__bloom.enabled) )
      {
        instn__bloom.enabled = !instn__bloom.enabled;
        instn__bloom.toggle ();

        _SK_TVFix_DisableBloom->store (! instn__bloom.enabled);
      }

      if ( instn__blur.scanned_addr != nullptr &&
           ImGui::Checkbox ("Enable Fullscene Blur", &instn__blur.enabled) )
      {
        instn__blur.enabled = !instn__blur.enabled;
        instn__blur.toggle ();

        _SK_TVFix_DisableBlur->store (! instn__blur.enabled);
      }
      ImGui::TreePop ();
    }

    ImGui::EndGroup   ();
    ImGui::SameLine   ();
    ImGui::BeginGroup ();

    const bool tex_manage =
      ImGui::CollapsingHeader ("Texture Management##ToV", ImGuiTreeNodeFlags_DefaultOpen);

    bool changed = false;

    if (tex_manage)
    {
      ImGui::TreePush    ("");
      ImGui::BeginGroup  (  );
      changed |= ImGui::Checkbox ("Generate Mipmaps", &config.textures.d3d11.generate_mips);

      if (ImGui::IsItemHovered ())
      {
        ImGui::SetTooltip ("NOTE: This is broken on some systems. Do not use it for now and make sure to purge any existing cache.");
        //ImGui::BeginTooltip    ();
        //ImGui::PushStyleColor  (ImGuiCol_Text, ImColor::HSV (0.5f, 0.f, 1.f, 1.f));
        //ImGui::TextUnformatted ("Builds Complete Mipchains (Mipmap LODs) for all Textures");
        //ImGui::Separator       ();
        //ImGui::PopStyleColor   ();
        //ImGui::Bullet          (); ImGui::SameLine ();
        //ImGui::PushStyleColor  (ImGuiCol_Text, ImColor::HSV (0.15f, 1.0f, 1.0f));
        //ImGui::TextUnformatted ("SIGNIFICANTLY");
        //ImGui::PopStyleColor   (); ImGui::SameLine ();
        //ImGui::TextUnformatted ("reduces texture aliasing");
        //ImGui::BulletText      ("May increase load-times");
        //ImGui::EndTooltip      ();
      }

      if (config.textures.d3d11.generate_mips)
      {
        ImGui::SameLine ();

        extern uint64_t SK_D3D11_MipmapCacheSize;
        
        ///if (ImGui::Checkbox ("Cache Mipmaps to Disk", &config.textures.d3d11.cache_gen_mips))
        ///{
        ///  changed = true;
        ///
        ///  ys8_config.mipmaps.cache->store (config.textures.d3d11.cache_gen_mips);
        ///}
        ///
        ///if (ImGui::IsItemHovered ())
        ///{
        ///  ImGui::BeginTooltip ();
        ///  ImGui::Text         ("Eliminates almost all Texture Pop-In");
        ///  ImGui::Separator    ();
        ///  ImGui::BulletText   ("Will quickly consume a LOT of disk space!");
        ///  ImGui::EndTooltip   ();
        ///}

        static wchar_t wszPath [ MAX_PATH + 2 ] = { };

        if (*wszPath == L'\0')
        {
          extern std::wstring SK_D3D11_res_root;

          wcscpy ( wszPath,
                     SK_EvalEnvironmentVars (SK_D3D11_res_root.c_str ()).c_str () );
        
          lstrcatW (wszPath, LR"(\inject\textures\MipmapCache\)");
          lstrcatW (wszPath, SK_GetHostApp ());
          lstrcatW (wszPath, LR"(\)");
        }

        if (SK_D3D11_MipmapCacheSize > 0)
        {
              ImGui::SameLine (               );
          if (ImGui::Button   (" Purge Cache "))
          {        
            SK_D3D11_MipmapCacheSize -= SK_DeleteTemporaryFiles (wszPath, L"*.dds");

            assert ((int64_t)SK_D3D11_MipmapCacheSize > 0LL);

            void
            WINAPI
            SK_D3D11_PopulateResourceList (bool refresh);

            SK_D3D11_PopulateResourceList (true);
          }

          if (ImGui::IsItemHovered ())
              ImGui::SetTooltip    ("%ws", wszPath);
        }

        //// For safety, never allow a user to touch the final 256 MiB of storage on their device
        //const ULONG FILESYSEM_RESERVE_MIB = 256UL;

        if (SK_D3D11_MipmapCacheSize > 0)
        {
          ImGui::SameLine ();
          ImGui::Text     ("Current Cache Size: %.2f MiB", (double)SK_D3D11_MipmapCacheSize / (1024.0 * 1024.0));
        }

        static uint64_t       last_MipmapCacheSize =         0ULL;
        static ULARGE_INTEGER ulBytesAvailable     = { 0UL, 0UL },
                              ulBytesTotal         = { 0UL, 0UL };

        //
        // GetDiskFreeSpaceEx has highly unpredictable call overhead
        //  
        //   ... so try to be careful with it!
        //
        if (SK_D3D11_MipmapCacheSize != last_MipmapCacheSize)
        {
          last_MipmapCacheSize      = SK_D3D11_MipmapCacheSize;

          GetDiskFreeSpaceEx ( wszPath, &ulBytesAvailable,
                                        &ulBytesTotal, nullptr);
        }

        if (SK_D3D11_MipmapCacheSize > 0)
        {
          ImGui::ProgressBar ( static_cast <float> (static_cast <long double> (ulBytesAvailable.QuadPart) /
                                                    static_cast <long double> (ulBytesTotal.QuadPart)       ),  
                                 ImVec2 (-1, 0),
              SK_WideCharToUTF8 (
                SK_File_SizeToStringF (ulBytesAvailable.QuadPart, 2, 3) + L" Remaining Storage Capacity"
              ).c_str ()
          );
        }
      }
      ImGui::EndGroup  ();
      ImGui::TreePop   ();
    }

    ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.68f, 0.02f, 0.45f));
    ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.72f, 0.07f, 0.80f));
    ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.78f, 0.14f, 0.80f));

    if (ImGui::CollapsingHeader ("Gameplay", ImGuiTreeNodeFlags_DefaultOpen))
    {
      ImGui::TreePush ("");
      ImGui::Checkbox ("Continue Running in Background###TVFIX_BackgroundRender", &config.window.background_render);

      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip (R"(Only works correctly if the game is set to "Borderless")");

      ImGui::TreePop  (  );
    }

    ImGui::EndGroup (  );

    ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.40f, 0.40f, 0.45f));
    ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.45f, 0.45f, 0.80f));
    ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.53f, 0.53f, 0.80f));

    if (ImGui::CollapsingHeader ("Debug"))
    {
      ImGui::TreePush   ("");
      ImGui::BeginGroup (  );

      ImGui::Checkbox ("Reduce Microstutter", &__SK_TVFix_NoRenderSleep);

      if ( instn__model_animation.scanned_addr != nullptr &&
           ImGui::Checkbox ("Enable Model Animation", &instn__model_animation.enabled) )
      {
        instn__model_animation.enabled = !instn__model_animation.enabled;
        instn__model_animation.toggle ();
      }

    //ImGui::SameLine ();

      if ( instn__particle_effects.scanned_addr != nullptr &&
           ImGui::Checkbox ("Enable Particle Effects", &instn__particle_effects.enabled) )
      {
        instn__particle_effects.enabled = !instn__particle_effects.enabled;
        instn__particle_effects.toggle ();
      }

      ImGui::EndGroup (  );
      ImGui::TreePop  (  );
    }

    ImGui::PopStyleColor (9);
    ImGui::TreePop       ( );
  }

  return true;
}
