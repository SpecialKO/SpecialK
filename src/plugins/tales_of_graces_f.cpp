// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
//
// Copyright 2025 Andon "Kaldaien" Coleman
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
#include <imgui/font_awesome.h>

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"Tales Plug"

bool SK_TGFix_HookMonoInit        (void);
bool SK_TGFix_SetupFramerateHooks (void);

extern volatile LONG SK_D3D11_DrawTrackingReqs;

sk::ParameterBool* _SK_TGFix_DisableDepthOfField;
bool              __SK_TGFix_DisableDepthOfField = true;

sk::ParameterBool* _SK_TGFix_DisableBloom;
bool              __SK_TGFix_DisableBloom = false;

sk::ParameterBool* _SK_TGFix_DisableHeatHaze;
bool              __SK_TGFix_DisableHeatHaze = false;

sk::ParameterBool* _SK_TGFix_DisableFXAA;
bool              __SK_TGFix_DisableFXAA = false;

sk::ParameterBool* _SK_TGFix_SharpenOutlines;
bool              __SK_TGFix_SharpenOutlines = false;

sk::ParameterInt* _SK_TGFix_MSAASampleCount;
bool             __SK_TGFix_MSAASampleCount = 1;

bool
SK_TGFix_PlugInCfg (void)
{
  if (ImGui::CollapsingHeader ("Tales of Graces f Remastered", ImGuiTreeNodeFlags_DefaultOpen))
  {
    bool cfg_changed = false;

    ImGui::TreePush ("");
    ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.02f, 0.68f, 0.90f, 0.45f));
    ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.07f, 0.72f, 0.90f, 0.80f));
    ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.14f, 0.78f, 0.87f, 0.80f));

    static int  restart_reqs = 0;

    ImGui::BeginGroup ();
    if (ImGui::CollapsingHeader (ICON_FA_PORTRAIT " Post-Processing", ImGuiTreeNodeFlags_DefaultOpen |
                                                                      ImGuiTreeNodeFlags_AllowOverlap))
    {
      ImGui::TreePush ("");

      if (ImGui::Checkbox ("Disable Depth of Field", &__SK_TGFix_DisableDepthOfField))
      {
        if (! __SK_TGFix_DisableDepthOfField)
        {
          SK_D3D11_Shaders->pixel.releaseTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0x54f44c1a);
          InterlockedDecrement (&SK_D3D11_DrawTrackingReqs);
        }

        else
        {
          SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0x54f44c1a);
          InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
        }

        _SK_TGFix_DisableDepthOfField->store (__SK_TGFix_DisableDepthOfField);

        cfg_changed = true;
      }
      ImGui::SetItemTooltip ("Depth of Field may be unusually strong at DSR resolutions and when using MSAA.");

      if (ImGui::Checkbox ("Disable FXAA", &__SK_TGFix_DisableFXAA))
      {
        if (__SK_TGFix_DisableFXAA)
        {
          SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0xbe80dda2);
          SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0xef92e3e1);

          InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
          InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
        }

        else
        {
          SK_D3D11_Shaders->pixel.releaseTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0xbe80dda2);
          SK_D3D11_Shaders->pixel.releaseTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0xef92e3e1);

          InterlockedDecrement (&SK_D3D11_DrawTrackingReqs);
          InterlockedDecrement (&SK_D3D11_DrawTrackingReqs);
        }

        _SK_TGFix_DisableFXAA->store (__SK_TGFix_DisableFXAA);

        cfg_changed = true;
      }
      ImGui::SetItemTooltip ("FXAA should be disabled when using MSAA or the minimap will break.");

      if (ImGui::Checkbox ("Disable Heat Haze", &__SK_TGFix_DisableHeatHaze))
      {
        if (__SK_TGFix_DisableHeatHaze)
        {
          SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0x4ea47fea);

          InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
        }

        else
        {
          SK_D3D11_Shaders->pixel.releaseTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0x4ea47fea);

          InterlockedDecrement (&SK_D3D11_DrawTrackingReqs);
        }

        _SK_TGFix_DisableHeatHaze->store (__SK_TGFix_DisableHeatHaze);

        cfg_changed = true;
      }
      ImGui::SetItemTooltip ("Heat Haze may be unusually strong at DSR resolutions and when using MSAA.");

      if (ImGui::Checkbox ("Disable Atmospheric Bloom", &__SK_TGFix_DisableBloom))
      {
        if (__SK_TGFix_DisableBloom)
        {
          SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0x5bcdb543);
          SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0xa9ca2e76);
          SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0xafcf335b);
          SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0xd70959df);

          InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
          InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
          InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
          InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);

          restart_reqs++;
        }

        else
        {
          SK_D3D11_Shaders->pixel.releaseTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0x5bcdb543);
          SK_D3D11_Shaders->pixel.releaseTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0xa9ca2e76);
          SK_D3D11_Shaders->pixel.releaseTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0xafcf335b);
          SK_D3D11_Shaders->pixel.releaseTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0xd70959df);

          InterlockedDecrement (&SK_D3D11_DrawTrackingReqs);
          InterlockedDecrement (&SK_D3D11_DrawTrackingReqs);
          InterlockedDecrement (&SK_D3D11_DrawTrackingReqs);
          InterlockedDecrement (&SK_D3D11_DrawTrackingReqs);
        }

        _SK_TGFix_DisableBloom->store (__SK_TGFix_DisableBloom);

        cfg_changed = true;
      }

      if (ImGui::Checkbox ("Sharpen Outlines", &__SK_TGFix_SharpenOutlines))
      {
        if (__SK_TGFix_SharpenOutlines)
        {
          SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.on_top, 0x2dfcf950);
          SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.on_top, 0x9a0e24eb);
          SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.on_top, 0xf751273e);

          InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
          InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
          InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
        }

        else
        {
          SK_D3D11_Shaders->pixel.releaseTrackingRef (SK_D3D11_Shaders->pixel.on_top, 0x2dfcf950);
          SK_D3D11_Shaders->pixel.releaseTrackingRef (SK_D3D11_Shaders->pixel.on_top, 0x9a0e24eb);
          SK_D3D11_Shaders->pixel.releaseTrackingRef (SK_D3D11_Shaders->pixel.on_top, 0xf751273e);

          InterlockedDecrement (&SK_D3D11_DrawTrackingReqs);
          InterlockedDecrement (&SK_D3D11_DrawTrackingReqs);
          InterlockedDecrement (&SK_D3D11_DrawTrackingReqs);
        }

        _SK_TGFix_SharpenOutlines->store (__SK_TGFix_SharpenOutlines);

        cfg_changed = true;
      }

      if (cfg_changed)
      {
        config.utility.save_async ();
      }

      ImGui::TreePop  ();
    } ImGui::EndGroup ();

    if (SK_ImGui_HasPlayStationController ())
    {
      ImGui::SameLine   ();
      ImGui::BeginGroup ();

      if (ImGui::CollapsingHeader (ICON_FA_PLAYSTATION " PlayStation Controllers", ImGuiTreeNodeFlags_DefaultOpen |
                                                                                   ImGuiTreeNodeFlags_AllowOverlap))
      {
        ImGui::TreePush ("");

        static std::error_code                    ec = { };
        static std::filesystem::path pathPlayStation =
          SK_Resource_GetRoot () / LR"(inject/textures/8E9DD816.dds)";

        static bool fetching_buttons = false;
        static bool has_buttons      = std::filesystem::exists (pathPlayStation, ec);

        bool disable_native_playstation =
          (config.input.gamepad.disable_hid && config.input.gamepad.xinput.emulate);

        if (ImGui::Checkbox ("Disable Native PlayStation Support", &disable_native_playstation))
        {
          if (disable_native_playstation)
          {
            restart_reqs++;
            config.input.gamepad.disable_hid    = true;
            config.input.gamepad.xinput.emulate = true;
          }

          else
          {
            if (std::filesystem::remove (pathPlayStation, ec))
            {
              has_buttons = false;
              restart_reqs++;
            }

            config.input.gamepad.disable_hid    = false;
            config.input.gamepad.xinput.emulate = false;
          }

          config.utility.save_async ();
        }

        if (ImGui::BeginItemTooltip ())
        {
          ImGui::TextUnformatted ("Native PlayStation support in this game does not work with Rumble");
          ImGui::Separator       ();
          ImGui::BulletText      ("Special K can translate PlayStation to XInput and correct the button icons.");
          ImGui::BulletText      ("Disabling the game's native support will break support for local multiplayer.");
          ImGui::EndTooltip      ();
        }

        if (disable_native_playstation)
        {
          if (! has_buttons)
          {
            if (! fetching_buttons)
            {
              if (ImGui::Button ("Download " ICON_FA_PLAYSTATION " Icons\t\t DualShock 4 "))
              {
                fetching_buttons = true;

                SK_Network_EnqueueDownload (
                     sk_download_request_s (
                       pathPlayStation.wstring (),
                         R"(https://sk-data.special-k.info/addon/TalesOfGraces/DualShock4.dds)",
                         []( const std::vector <uint8_t>&&,
                             const std::wstring_view )
                          -> bool
                             {
                               fetching_buttons = false;
                               has_buttons      = true;
                               restart_reqs++;
                         
                               return false;
                             }
                         ),
                     true);
              }

              if (ImGui::Button ("Download " ICON_FA_PLAYSTATION " Icons\t\t DualSense "))
              {
                fetching_buttons = true;

                SK_Network_EnqueueDownload (
                     sk_download_request_s (
                       pathPlayStation.wstring (),
                         R"(https://sk-data.special-k.info/addon/TalesOfGraces/DualSense.dds)",
                         []( const std::vector <uint8_t>&&,
                             const std::wstring_view )
                          -> bool
                             {
                               fetching_buttons = false;
                               has_buttons      = true;
                               restart_reqs++;
                         
                               return false;
                             }
                         ),
                     true);
              }
            }

            else
            {
              ImGui::BulletText ("Downloading New Button Icons...");
            }
          }

          else
          {
            if (ImGui::Button ("Revert to " ICON_FA_XBOX " Icons"))
            {
              if (std::filesystem::remove (pathPlayStation, ec))
              {
                has_buttons = false;
                restart_reqs++;
              }
            }
          }
        }

        ImGui::TreePop  (  );
      }

      ImGui::EndGroup    ( );
    }

#if 0
    if (ImGui::CollapsingHeader (ICON_FA_MAGIC " ThirdParty AddOns", ImGuiTreeNodeFlags_DefaultOpen |
                                                                     ImGuiTreeNodeFlags_AllowOverlap))
    {
      ImGui::TreePush ("");
      
      static bool bHasVersionDLL = 
        PathFileExistsW (L"version.dll");

      if (! bHasVersionDLL)
      {
        ImGui::Button ("Download Tales of Graces f \"Fix\"");
      }

      static bool bHasReShade64DLL = 
        PathFileExistsW (L"ReShade64.dll");

      if (! bHasReShade64DLL)
      {
        ImGui::Button ("Download RenoDX HDR AddOn for ReShade");
      }
      ImGui::TreePop  (  );
    }
#endif

    if (restart_reqs > 0)
    {
      ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (.3f, .8f, .9f).Value);
      ImGui::BulletText     ("Game Restart Required");
      ImGui::PopStyleColor  ();
    }

    ImGui::PopStyleColor (3);
    ImGui::TreePop       ( );
  }

  return true;
}

HRESULT STDMETHODCALLTYPE SK_TGFix_PresentFirstFrame (IUnknown* pSwapChain, UINT SyncInterval, UINT Flags);
void    STDMETHODCALLTYPE SK_TGFix_EndFrame          (void);

void
SK_TGFix_InitPlugin (void)
{
  SK_RunOnce (
    SK_TGFix_HookMonoInit ();

    SK_D3D11_DeclHUDShader_Pix (0x1399301e);
    SK_D3D11_DeclHUDShader_Pix (0x171c9bf7);

    plugin_mgr->config_fns.emplace      (SK_TGFix_PlugInCfg);
    plugin_mgr->first_frame_fns.emplace (SK_TGFix_PresentFirstFrame);
    plugin_mgr->end_frame_fns.emplace   (SK_TGFix_EndFrame);

    _SK_TGFix_DisableDepthOfField =
      _CreateConfigParameterBool ( L"TGFix.Render",
                                   L"DisableDepthOfField",  __SK_TGFix_DisableDepthOfField,
                                   L"Disable Depth of Field" );

    _SK_TGFix_DisableBloom =
      _CreateConfigParameterBool ( L"TGFix.Render",
                                   L"DisableBloom",  __SK_TGFix_DisableBloom,
                                   L"Disable Bloom Lighting" );

    _SK_TGFix_DisableHeatHaze =
      _CreateConfigParameterBool ( L"TGFix.Render",
                                   L"DisableHeatHaze",  __SK_TGFix_DisableHeatHaze,
                                   L"Disable Heat Haze" );

    _SK_TGFix_DisableFXAA =
      _CreateConfigParameterBool ( L"TGFix.Render",
                                   L"DisableFXAA",  __SK_TGFix_DisableFXAA,
                                   L"Disable FXAA" );

    _SK_TGFix_SharpenOutlines =
      _CreateConfigParameterBool ( L"TGFix.Render",
                                   L"SharpenOutlines",  __SK_TGFix_SharpenOutlines,
                                   L"Sharpen Character Outlines" );

    if (__SK_TGFix_DisableDepthOfField)
    {
      SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0x54f44c1a);

      InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
    }

    if (__SK_TGFix_DisableBloom)
    {
      SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0x5bcdb543);
      SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0xa9ca2e76);
      SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0xafcf335b);
      SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0xd70959df);

      InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
      InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
      InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
      InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
    }

    if (__SK_TGFix_DisableFXAA)
    {
      SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0xbe80dda2);
      SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0xef92e3e1);

      InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
      InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
    }

    if (__SK_TGFix_SharpenOutlines)
    {
      SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.on_top, 0x2dfcf950);
      SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.on_top, 0x9a0e24eb);
      SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.on_top, 0xf751273e);

      InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
      InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
      InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
    }

    if (__SK_TGFix_DisableHeatHaze)
    {
      SK_D3D11_Shaders->pixel.addTrackingRef (SK_D3D11_Shaders->pixel.blacklist, 0x4ea47fea);

      InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
    }
  );
}

#include <mono/metadata/assembly.h>
#include <mono/metadata/threads.h>
#include <mono/metadata/object.h>

typedef MonoThread*     (*mono_thread_attach_pfn)(MonoDomain* domain);
typedef MonoDomain*     (*mono_get_root_domain_pfn)(void);
typedef MonoAssembly*   (*mono_domain_assembly_open_pfn)(MonoDomain* doamin, const char* name);
typedef MonoImage*      (*mono_assembly_get_image_pfn)(MonoAssembly* assembly);
typedef MonoObject*     (*mono_object_new_pfn)(MonoDomain* domain, MonoClass* klass);
typedef MonoDomain*     (*mono_object_get_domain_pfn)(MonoObject* obj);
typedef MonoClass*      (*mono_class_from_name_pfn)(MonoImage* image, const char* name_space, const char* name);
typedef MonoMethod*     (*mono_class_get_method_from_name_pfn)(MonoClass* klass, const char* name, int param_count);
typedef void*           (*mono_compile_method_pfn)(MonoMethod* method);
typedef MonoObject*     (*mono_runtime_invoke_pfn)(MonoMethod* method, void* obj, void** params, MonoObject** exc);

typedef MonoClassField* (*mono_class_get_field_from_name_pfn)(MonoClass* klass, const char* name);
typedef void*           (*mono_field_get_value_pfn)(void* obj, MonoClassField* field, void* value);
typedef void            (*mono_field_set_value_pfn)(MonoObject* obj, MonoClassField* field, void* value);
typedef MonoObject*     (*mono_field_get_value_object_pfn)(MonoDomain* domain, MonoClassField* field, MonoObject* obj);
typedef MonoClass*      (*mono_method_get_class_pfn)(MonoMethod* method);
typedef MonoVTable*     (*mono_class_vtable_pfn)(MonoDomain* domain, MonoClass* klass);
typedef void*           (*mono_vtable_get_static_field_data_pfn)(MonoVTable* vt);
typedef uint32_t        (*mono_field_get_offset_pfn)(MonoClassField* field);

typedef MonoObject*     (*mono_property_get_value_pfn)(MonoProperty *prop, void *obj, void **params, MonoObject **exc);
typedef MonoProperty*   (*mono_class_get_property_from_name_pfn)(MonoClass *klass, const char *name);
typedef MonoImage*      (*mono_image_loaded_pfn)(const char *name);

mono_thread_attach_pfn                SK_mono_thread_attach                = nullptr;
mono_get_root_domain_pfn              SK_mono_get_root_domain              = nullptr;
mono_domain_assembly_open_pfn         SK_mono_domain_assembly_open         = nullptr;
mono_assembly_get_image_pfn           SK_mono_assembly_get_image           = nullptr;
mono_object_new_pfn                   SK_mono_object_new                   = nullptr;
mono_object_get_domain_pfn            SK_mono_object_get_domain            = nullptr;
mono_class_from_name_pfn              SK_mono_class_from_name              = nullptr;
mono_class_get_method_from_name_pfn   SK_mono_class_get_method_from_name   = nullptr;
mono_compile_method_pfn               SK_mono_compile_method               = nullptr;
mono_runtime_invoke_pfn               SK_mono_runtime_invoke               = nullptr;

mono_property_get_value_pfn           SK_mono_property_get_value           = nullptr;
mono_class_get_property_from_name_pfn SK_mono_class_get_property_from_name = nullptr;
mono_image_loaded_pfn                 SK_mono_image_loaded                 = nullptr;

mono_class_get_field_from_name_pfn    SK_mono_class_get_field_from_name    = nullptr;
mono_field_get_value_pfn              SK_mono_field_get_value              = nullptr;
mono_field_set_value_pfn              SK_mono_field_set_value              = nullptr;
mono_field_get_value_object_pfn       SK_mono_field_get_value_object       = nullptr;
mono_method_get_class_pfn             SK_mono_method_get_class             = nullptr;
mono_class_vtable_pfn                 SK_mono_class_vtable                 = nullptr;
mono_vtable_get_static_field_data_pfn SK_mono_vtable_get_static_field_data = nullptr;
mono_field_get_offset_pfn             SK_mono_field_get_offset             = nullptr;


using mono_jit_init_pfn              = MonoDomain* (*)(const char *file);
      mono_jit_init_pfn
      mono_jit_init_Original         = nullptr;
using mono_jit_init_version_pfn      = MonoDomain* (*)(const char *root_domain_name, const char *runtime_version);
      mono_jit_init_version_pfn
      mono_jit_init_version_Original = nullptr;

void SK_TGFix_OnInitMono (MonoDomain* domain);

MonoDomain*
mono_jit_init_Detour (const char *file)
{
  SK_LOG_FIRST_CALL

  auto domain =
    mono_jit_init_Original (file);

  if (domain != nullptr)
  {
    //SK_TGFix_OnInitMono (domain);
  }

  return domain;
}

MonoDomain*
mono_jit_init_version_Detour (const char *root_domain_name, const char *runtime_version)
{
  SK_LOG_FIRST_CALL

  auto domain =
    mono_jit_init_version_Original (root_domain_name, runtime_version);

  if (domain != nullptr)
  {
    //SK_TGFix_OnInitMono (domain);
  }

  return domain;
}

static constexpr wchar_t* mono_dll  = L"mono-2.0-bdwgc.dll";
static constexpr wchar_t* mono_path = LR"(MonoBleedingEdge\EmbedRuntime\mono-2.0-bdwgc.dll)";

bool
SK_TGFix_HookMonoInit (void)
{
  SK_LoadLibraryW (mono_path);

  HMODULE hMono =
    GetModuleHandleW (mono_dll);

  if (hMono == NULL)
    return false;

  SK_mono_domain_assembly_open         = reinterpret_cast <mono_domain_assembly_open_pfn>         (SK_GetProcAddress (hMono, "mono_domain_assembly_open"));
  SK_mono_assembly_get_image           = reinterpret_cast <mono_assembly_get_image_pfn>           (SK_GetProcAddress (hMono, "mono_assembly_get_image"));
  SK_mono_class_from_name              = reinterpret_cast <mono_class_from_name_pfn>              (SK_GetProcAddress (hMono, "mono_class_from_name"));
  SK_mono_class_get_method_from_name   = reinterpret_cast <mono_class_get_method_from_name_pfn>   (SK_GetProcAddress (hMono, "mono_class_get_method_from_name"));
  SK_mono_compile_method               = reinterpret_cast <mono_compile_method_pfn>               (SK_GetProcAddress (hMono, "mono_compile_method"));
  SK_mono_runtime_invoke               = reinterpret_cast <mono_runtime_invoke_pfn>               (SK_GetProcAddress (hMono, "mono_runtime_invoke"));
  
  SK_mono_class_get_field_from_name    = reinterpret_cast <mono_class_get_field_from_name_pfn>    (SK_GetProcAddress (hMono, "mono_class_get_field_from_name"));
  SK_mono_field_get_value              = reinterpret_cast <mono_field_get_value_pfn>              (SK_GetProcAddress (hMono, "mono_field_get_value"));
  SK_mono_field_set_value              = reinterpret_cast <mono_field_set_value_pfn>              (SK_GetProcAddress (hMono, "mono_field_set_value"));
  SK_mono_field_get_value_object       = reinterpret_cast <mono_field_get_value_object_pfn>       (SK_GetProcAddress (hMono, "mono_field_get_value_object"));
  SK_mono_method_get_class             = reinterpret_cast <mono_method_get_class_pfn>             (SK_GetProcAddress (hMono, "mono_method_get_class"));
  SK_mono_class_vtable                 = reinterpret_cast <mono_class_vtable_pfn>                 (SK_GetProcAddress (hMono, "mono_class_vtable"));
  SK_mono_vtable_get_static_field_data = reinterpret_cast <mono_vtable_get_static_field_data_pfn> (SK_GetProcAddress (hMono, "mono_vtable_get_static_field_data"));
  SK_mono_field_get_offset             = reinterpret_cast <mono_field_get_offset_pfn>             (SK_GetProcAddress (hMono, "mono_field_get_offset"));
  
  SK_mono_property_get_value           = reinterpret_cast <mono_property_get_value_pfn>           (SK_GetProcAddress (hMono, "mono_property_get_value"));
  SK_mono_class_get_property_from_name = reinterpret_cast <mono_class_get_property_from_name_pfn> (SK_GetProcAddress (hMono, "mono_class_get_property_from_name"));
  SK_mono_image_loaded                 = reinterpret_cast <mono_image_loaded_pfn>                 (SK_GetProcAddress (hMono, "mono_image_loaded"));
  SK_mono_object_new                   = reinterpret_cast <mono_object_new_pfn>                   (SK_GetProcAddress (hMono, "mono_object_new"));
  SK_mono_object_get_domain            = reinterpret_cast <mono_object_get_domain_pfn>            (SK_GetProcAddress (hMono, "mono_object_get_domain"));
 
  SK_mono_thread_attach                = reinterpret_cast <mono_thread_attach_pfn>                (SK_GetProcAddress (hMono, "mono_thread_attach"));
  SK_mono_get_root_domain              = reinterpret_cast <mono_get_root_domain_pfn>              (SK_GetProcAddress (hMono, "mono_get_root_domain"));


  void* pfnMonoJitInit        = nullptr;
  void* pfnMonoJitInitVersion = nullptr;

  SK_CreateDLLHook (         mono_dll,
                            "mono_jit_init",
                             mono_jit_init_Detour,
    static_cast_p2p <void> (&mono_jit_init_Original),
                           &pfnMonoJitInit );

  SK_CreateDLLHook (         mono_dll,
                            "mono_jit_init_version",
                             mono_jit_init_version_Detour,
    static_cast_p2p <void> (&mono_jit_init_version_Original),
                           &pfnMonoJitInitVersion );

  SK_EnableHook (pfnMonoJitInit);
  SK_EnableHook (pfnMonoJitInitVersion);

  // If this was pre-loaded, then the above hooks never run and we should initialize everything immediately...
  if (SK_GetModuleHandleW (L"BepInEx.Core.dll"))
  {
    //if ( nullptr !=        SK_mono_get_root_domain )
    //  SK_TGFix_OnInitMono (SK_mono_get_root_domain ());
  }


  return true;
}

void SK_TGFix_OnInitMono (MonoDomain* domain)
{
  if ((! domain) || SK_mono_thread_attach == nullptr)
    return;

  static bool
        init = false;
  if (! init)
  {
    SK_mono_thread_attach (domain);

    SK_ReleaseAssert (domain == SK_mono_get_root_domain ());

    init = SK_TGFix_SetupFramerateHooks ();
  }
}

HRESULT
STDMETHODCALLTYPE
SK_TGFix_PresentFirstFrame (IUnknown* pSwapChain, UINT SyncInterval, UINT Flags)
{
  UNREFERENCED_PARAMETER (pSwapChain);
  UNREFERENCED_PARAMETER (SyncInterval);
  UNREFERENCED_PARAMETER (Flags);

  // Better late initialization than never...
  if ( nullptr !=        SK_mono_get_root_domain )
    SK_TGFix_OnInitMono (SK_mono_get_root_domain ());

  return S_OK;
}

bool LoadMonoAssembly (const char* assemblyName)
{
  MonoImage* pImage =
    SK_mono_image_loaded (assemblyName);

  if (pImage != nullptr)
    return true;

  MonoDomain* pDomain =
    SK_mono_get_root_domain ();

  if (pDomain == nullptr)
    return false;
 
  MonoAssembly* pAssembly =
    SK_mono_domain_assembly_open (pDomain, assemblyName);

  if (pAssembly == nullptr)
    return false;
 
  pImage =
    SK_mono_assembly_get_image (pAssembly);

  return
    (pImage != nullptr);
}

// Used to get the address of a game function for hooking
void* GetCompiledMethod (const char* nameSpace, const char* className, const char* methodName, int param_count = 0, const char* assemblyName = "Assembly-CSharp")
{
  MonoImage* pImage =
    SK_mono_image_loaded (assemblyName);

  if (! pImage)
  {
    MonoDomain* pDomain =
      SK_mono_get_root_domain ();

    if (pDomain == nullptr)
      return nullptr;
 
    MonoAssembly* pAssembly =
      SK_mono_domain_assembly_open (pDomain, assemblyName);

    if (pAssembly == nullptr)
      return nullptr;
 
    pImage =
      SK_mono_assembly_get_image (pAssembly);

    if (pImage == nullptr)
      return nullptr;
  }
 
  MonoClass* pKlass =
    SK_mono_class_from_name (pImage, nameSpace != nullptr ? nameSpace : "", className);

  if (pKlass == nullptr)
    return nullptr;
 
  MonoMethod* pMethod =
    SK_mono_class_get_method_from_name (pKlass, methodName, param_count);

  if (pMethod == nullptr)
    return nullptr;
 
  return
    SK_mono_compile_method (pMethod);
}

// Get a game method to call using mono_runtime_invoke
MonoMethod* GetMethod (const char* className, const char* methodName, int param_count = 0, const char* assemblyName = "Assembly-CSharp", const char* nameSpace = "")
{
  MonoImage* pImage =
    SK_mono_image_loaded (assemblyName);

  if (! pImage)
  {
    MonoDomain* pDomain =
      SK_mono_get_root_domain ();

    if (pDomain == nullptr)
      return nullptr;
 
    MonoAssembly* pAssembly =
      SK_mono_domain_assembly_open (pDomain, assemblyName);

    if (pAssembly == nullptr)
      return nullptr;
 
    pImage =
      SK_mono_assembly_get_image (pAssembly);

    if (pImage == nullptr)
      return nullptr;
  }
 
  MonoClass* pKlass =
    SK_mono_class_from_name (pImage, nameSpace, className);

  if (pKlass == nullptr)
    return nullptr;
 
  return
    SK_mono_class_get_method_from_name (pKlass, methodName, param_count);
}

// When you need to get a class pointer
MonoClass* GetClass (const char* className, const char* assemblyName = "Assembly-CSharp", const char* nameSpace = "")
{
  MonoImage* pImage =
    SK_mono_image_loaded (assemblyName);

  if (! pImage)
  {
    MonoDomain* pDomain =
      SK_mono_get_root_domain ();

    if (pDomain == nullptr)
      return nullptr;
 
    MonoAssembly* pAssembly =
      SK_mono_domain_assembly_open (pDomain, assemblyName);

    if (pAssembly == nullptr)
      return nullptr;
 
    pImage =
      SK_mono_assembly_get_image (pAssembly);

    if (pImage == nullptr)
      return nullptr;
  }
 
  MonoClass* pKlass =
    SK_mono_class_from_name (pImage, nameSpace, className);

  return pKlass;
}

// When you need the class from a method*
MonoClass* GetClassFromMethod (MonoMethod* method)
{
  return
    SK_mono_method_get_class (method);
}

// When you need data from a class field like if something IsActive
MonoClassField* GetField (const char* className, const char* fieldName, const char* assemblyName = "Assembly-CSharp", const char* nameSpace = "")
{
  MonoImage* pImage =
    SK_mono_image_loaded (assemblyName);

  if (! pImage)
  {
    MonoDomain* pDomain =
      SK_mono_get_root_domain ();

    if (pDomain == nullptr)
      return nullptr;
 
    MonoAssembly* pAssembly =
      SK_mono_domain_assembly_open (pDomain, assemblyName);

    if (pAssembly == nullptr)
      return nullptr;
 
    pImage =
      SK_mono_assembly_get_image (pAssembly);

    if (pImage == nullptr)
      return nullptr;
  }
 
  MonoClass* pKlass =
    SK_mono_class_from_name (pImage, nameSpace, className);

  if (pKlass == nullptr)
    return nullptr;
 
  MonoClassField* pField =
    SK_mono_class_get_field_from_name (pKlass, fieldName);

  return pField;
}

// Helpers for GetStaticFieldValue
MonoClassField* GetField (MonoClass* pKlass, const char* fieldName)
{
  MonoClassField* pField =
    SK_mono_class_get_field_from_name (pKlass, fieldName);

  return pField;
}

uint32_t GetFieldOffset (MonoClassField* field)
{
  return
    SK_mono_field_get_offset (field);
}

void GetFieldValue (void* instance, MonoClassField* field, void* out)
{
  SK_mono_field_get_value (instance, field, out);
}

void SetFieldValue (MonoObject* obj, MonoClassField* field, void* value)
{
  SK_mono_field_set_value (obj, field, value);
}

MonoVTable* GetVTable (MonoClass* pKlass)
{
  return
    SK_mono_class_vtable (SK_mono_get_root_domain (), pKlass);
}

void* GetStaticFieldData (MonoVTable* pVTable)
{
  return
    SK_mono_vtable_get_static_field_data (pVTable);
}

void* GetStaticFieldData (MonoClass* pKlass)
{
  MonoVTable* pVTable =
    GetVTable (pKlass);

  if (pVTable == nullptr)
    return nullptr;
 
  return
    SK_mono_vtable_get_static_field_data (pVTable);
}
// End helpers

void* GetStaticFieldValue (const char* className, const char* fieldName, const char* assemblyName = "Assembly-CSharp", const char* nameSpace = "")
{
  MonoClass* pKlass =
    GetClass (className, assemblyName, nameSpace);

  if (pKlass == nullptr)
    return nullptr;
 
  MonoClassField* pField =
    GetField (pKlass, fieldName);

  if (pField == nullptr)
    return nullptr;
 
  DWORD_PTR addr   = (DWORD_PTR)GetStaticFieldData (pKlass);
  uint32_t  offset =            GetFieldOffset     (pField);
 
  void* value = (void *)(addr + offset);

  return value;
}

MonoImage* GetImage (const char* name)
{
  return
    SK_mono_image_loaded (name);
}

MonoClass* GetClass (MonoImage* image, const char* _namespace, const char* _class)
{
  return
    SK_mono_class_from_name (image, _namespace, _class);
}

MonoMethod* GetClassMethod (MonoClass* class_, const char* name, int params)
{
  return
    SK_mono_class_get_method_from_name (class_, name, params);
}

MonoMethod* GetClassMethod (MonoImage* image, const char* namespace_, const char* class_, const char* name, int params)
{
  MonoClass* hClass =
    GetClass (image, namespace_, class_);

  return hClass ? GetClassMethod (hClass, name, params) : NULL;
}

MonoMethod* GetClassMethod (const char* namespace_, const char* class_, const char* name, int params, const char* image_ = "Assembly-CSharp")
{
  MonoClass* hClass = GetClass (GetImage (image_), namespace_, class_);
  return     hClass ? GetClassMethod (hClass, name, params) : NULL;
}

MonoProperty* GetProperty (MonoImage* image, const char* namespace_, const char* class_, const char* name)
{
  return
    SK_mono_class_get_property_from_name (GetClass (image, namespace_, class_), name);
}
 
MonoObject* GetPropertyValue (MonoProperty* property, uintptr_t instance = 0)
{
  return
    SK_mono_property_get_value (property, reinterpret_cast <void *>(instance), nullptr, 0);
}

MonoObject* GetPropertyValue (MonoImage* image, const char* namespace_, const char* class_, const char* name, uintptr_t instance = 0)
{
  return
    GetPropertyValue (GetProperty (image, namespace_, class_, name), instance);
}
 
MonoObject* InvokeMethod (const char* namespace_, const char* class_, const char* method, int paramsCount, void* instance, const char* image_ = "Assembly-CSharp", void** params = nullptr)
{
  return
    SK_mono_runtime_invoke (GetClassMethod (GetImage (image_), namespace_, class_, method, paramsCount), instance, params, nullptr);
}
 
void* CompileMethod (MonoMethod* method)
{
  return
    SK_mono_compile_method (method);
}
 
void* CompileMethod (const char* namespace_, const char* class_, const char* name, int params, const char* image_ = "Assembly-CSharp")
{
  return
    CompileMethod (GetClassMethod (GetImage (image_), namespace_, class_, name, params));
}

MonoObject* NewObject (MonoClass* klass)
{
  SK_mono_thread_attach (SK_mono_get_root_domain ());

  return
    SK_mono_object_new (SK_mono_get_root_domain (), klass);
}

// This is probably one of the most useful functions, for when you need to call one of the games functions easily
MonoObject* Invoke (MonoMethod* method, void* obj, void** params)
{
  SK_mono_thread_attach (SK_mono_get_root_domain ());
 
  MonoObject* exc;

  return
    SK_mono_runtime_invoke (method, obj, params, &exc);
}

MonoObject* ConstructNewObject (MonoClass* klass, int num_args, void** args)
{
  auto ctor =
    SK_mono_class_get_method_from_name (klass, ".ctor", num_args);

  MonoObject*
      instance = NewObject (klass);
  if (instance != nullptr)
  {
    return
      Invoke (ctor, instance, args);
  }

  return nullptr;
}

MonoObject* ConstructObject (MonoClass* klass, MonoObject* instance, int num_args, void** args)
{
  auto ctor =
    SK_mono_class_get_method_from_name (klass, ".ctor", num_args);

  if (instance != nullptr)
  {
    return
      Invoke (ctor, instance, args);
  }

  return nullptr;
}

using NobleFrameRateManager_SetQualitySettingFrameRate_pfn = void (*)(MonoObject*, float);
using NobleFrameRateManager_SetTargetFrameRate_pfn         = void (*)(MonoObject*, float);

NobleFrameRateManager_SetQualitySettingFrameRate_pfn NobleFrameRateManager_SetQualitySettingFrameRate_Original = nullptr;
NobleFrameRateManager_SetTargetFrameRate_pfn         NobleFrameRateManager_SetTargetFrameRate_Original         = nullptr;

MonoObject* SK_TGFix_NobleFrameRateManagerSingleton = nullptr;
float       SK_TGFix_LastSetFrameRateLimit          = 0.0f;

void
SK_TGFix_NobleFrameRateManager_SetQualitySettingFrameRate_Detour (MonoObject* __this, float rate)
{
  SK_LOG_FIRST_CALL

  SK_TGFix_NobleFrameRateManagerSingleton = __this;

  auto& rb =
    SK_GetCurrentRenderBackend ();

  const float fActiveRefresh =
    static_cast <float> (rb.displays [rb.active_display].signal.timing.vsync_freq.Numerator) /
    static_cast <float> (rb.displays [rb.active_display].signal.timing.vsync_freq.Denominator);

  if (__target_fps > 0.0f || fActiveRefresh >= 120.0f)
  {
    if (__target_fps <= 0.0f)
    {
      SK_TGFix_LastSetFrameRateLimit = fActiveRefresh;

      return
        NobleFrameRateManager_SetQualitySettingFrameRate_Original (__this, fActiveRefresh * 1.008f);
    }

    SK_TGFix_LastSetFrameRateLimit = __target_fps;

    return
      NobleFrameRateManager_SetQualitySettingFrameRate_Original (__this, __target_fps * 1.01f);
  }

  SK_TGFix_LastSetFrameRateLimit = rate;

  return
    NobleFrameRateManager_SetQualitySettingFrameRate_Original (__this, rate);
}

void
SK_TGFix_NobleFrameRateManager_SetTargetFrameRate_Detour (MonoObject* __this, float rate)
{
  SK_LOG_FIRST_CALL

  SK_TGFix_NobleFrameRateManagerSingleton = __this;

  auto& rb =
    SK_GetCurrentRenderBackend ();

  const float fActiveRefresh =
    static_cast <float> (rb.displays [rb.active_display].signal.timing.vsync_freq.Numerator) /
    static_cast <float> (rb.displays [rb.active_display].signal.timing.vsync_freq.Denominator);

  if (__target_fps > 0.0f || fActiveRefresh >= 120.0f)
  {
    if (__target_fps <= 0.0f)
    {
      SK_TGFix_LastSetFrameRateLimit = fActiveRefresh;

      return
        NobleFrameRateManager_SetTargetFrameRate_Original (__this, fActiveRefresh * 1.008f);
    }

    SK_TGFix_LastSetFrameRateLimit = __target_fps;

    return
      NobleFrameRateManager_SetTargetFrameRate_Original (__this, __target_fps * 1.01f);
  }

  SK_TGFix_LastSetFrameRateLimit = rate;

  return
    NobleFrameRateManager_SetTargetFrameRate_Original (__this, rate);
}

constexpr
float SK_TGFix_NativeAspect     = 16.0f / 9.0f;
float SK_TGFix_AspectRatio      = 0.0f;
float SK_TGFix_AspectMultiplier = 0.0f;
int   SK_TGFix_ScreenWidth      = 0;
int   SK_TGFix_ScreenHeight     = 0;

using UnityEngine_Screen_SetResolution_pfn = void (*)(int, int, int);
      UnityEngine_Screen_SetResolution_pfn
      UnityEngine_Screen_SetResolution_Original = nullptr;

void
SK_TGFix_UnityEngine_Screen_SetResolution_Detour (int width, int height, int fullscreen)
{
  SK_LOG_FIRST_CALL

  SK_TGFix_ScreenWidth  = width;
  SK_TGFix_ScreenHeight = height;

  SK_TGFix_AspectRatio      = (float)width / (float)height;
  SK_TGFix_AspectMultiplier = SK_TGFix_AspectRatio / SK_TGFix_NativeAspect;

  SK_LOGi0 ("Current Resolution: %dx%d",                 width, height);
  SK_LOGi0 ("Current Resolution: Aspect Ratio: %f",      SK_TGFix_AspectRatio);
  SK_LOGi0 ("Current Resolution: Aspect Multiplier: %f", SK_TGFix_AspectMultiplier);

  UnityEngine_Screen_SetResolution_Original (width, height, fullscreen);
}

struct Unity_Rect
{
  float x;
  float y;
  float width;
  float height;
};

using Noble_CameraManager_SetCameraViewportRect_pfn = void (*)(MonoObject*, Unity_Rect*);
using Noble_CameraManager_SetCameraAspect_pfn       = void (*)(MonoObject*, float aspect);

Noble_CameraManager_SetCameraViewportRect_pfn Noble_CameraManager_SetCameraViewportRect_Original = nullptr;
Noble_CameraManager_SetCameraAspect_pfn       Noble_CameraManager_SetCameraAspect_Original       = nullptr;

void
SK_TGFix_Noble_CameraManager_SetCameraViewportRect_Detour (MonoObject* __this, Unity_Rect* rect)
{
  SK_LOG_FIRST_CALL

  if (SK_TGFix_AspectRatio != SK_TGFix_NativeAspect)
  {
    rect->width  = 1.0f;
    rect->height = 1.0f;
    rect->x      = 0.0f;
    rect->y      = 0.0f;
  }

  Noble_CameraManager_SetCameraViewportRect_Original (__this, rect);
}

void
SK_TGFix_Noble_CameraManager_SetCameraAspect_Detour (MonoObject* __this, float aspect)
{
  SK_LOG_FIRST_CALL

  if (SK_TGFix_AspectRatio != SK_TGFix_NativeAspect)
  {
    return
      Noble_CameraManager_SetCameraAspect_Original (__this, SK_TGFix_AspectRatio);
  }

  Noble_CameraManager_SetCameraAspect_Original (__this, aspect);
}

using Noble_PrimitiveManager_CalcUIOrthoMatrix_pfn = void (*)(MonoObject*, bool forceproc);
      Noble_PrimitiveManager_CalcUIOrthoMatrix_pfn
      Noble_PrimitiveManager_CalcUIOrthoMatrix_Original = nullptr;

void
SK_TGFix_Noble_PrimitiveManager_CalcUIOrthoMatrix_Detour (MonoObject* __this, bool forceproc)
{
  SK_LOG_FIRST_CALL

  forceproc = true;

  if (SK_TGFix_AspectRatio == SK_TGFix_NativeAspect)
    Noble_PrimitiveManager_CalcUIOrthoMatrix_Original (__this, forceproc);

  // These are called out-of-order sometimes
  if (SK_TGFix_AspectMultiplier == 0.0f)
  {
    return;
  }

  static auto m_Viewport     =
    GetField ("PrimitiveManager", "m_Viewport", "Assembly-CSharp", "Noble");
  static auto m_screenWidth  =
    GetField ("PrimitiveManager", "m_screenWidth", "Assembly-CSharp", "Noble");
  static auto m_screenHeight =
    GetField ("PrimitiveManager", "m_screenHeight", "Assembly-CSharp", "Noble");

  static auto UnityEngine_Rect =
    GetClass (GetImage ("UnityEngine.CoreModule"), "UnityEngine", "Rect");

  if (SK_TGFix_AspectRatio > SK_TGFix_NativeAspect)
  {
    float x      = 0.0f,      y = 0.0f,
          width  = 0.0f, height = 0.0f;

    auto domain   = SK_mono_object_get_domain                          (__this);
    auto viewport = SK_mono_field_get_value_object (domain, m_Viewport, __this);

    static auto _x      = GetField ("Rect", "m_XMin",   "UnityEngine.CoreModule", "UnityEngine");
    static auto _y      = GetField ("Rect", "m_YMin",   "UnityEngine.CoreModule", "UnityEngine");
    static auto _width  = GetField ("Rect", "m_Width",  "UnityEngine.CoreModule", "UnityEngine");
    static auto _height = GetField ("Rect", "m_Height", "UnityEngine.CoreModule", "UnityEngine");

    GetFieldValue (viewport, _x,            &x);
    GetFieldValue (viewport, _y,            &y);
    GetFieldValue (__this,  m_screenWidth,  &width);
    GetFieldValue (__this,  m_screenHeight, &height);

    width = width / SK_TGFix_AspectMultiplier;

    SetFieldValue (viewport, _x,      &x);
    SetFieldValue (viewport, _y,      &y);
    SetFieldValue (viewport, _width,  &width);
    SetFieldValue (viewport, _height, &height);
  }

  else if (SK_TGFix_AspectRatio < SK_TGFix_NativeAspect)
  {
    float x      = 0.0f,      y = 0.0f,
          width  = 0.0f, height = 0.0f;

    auto domain   = SK_mono_object_get_domain                          (__this);
    auto viewport = SK_mono_field_get_value_object (domain, m_Viewport, __this);

    static auto _x      = GetField ("Rect", "m_XMin",   "UnityEngine.CoreModule", "UnityEngine");
    static auto _y      = GetField ("Rect", "m_YMin",   "UnityEngine.CoreModule", "UnityEngine");
    static auto _width  = GetField ("Rect", "m_Width",  "UnityEngine.CoreModule", "UnityEngine");
    static auto _height = GetField ("Rect", "m_Height", "UnityEngine.CoreModule", "UnityEngine");

    GetFieldValue (viewport, _x,            &x);
    GetFieldValue (viewport, _y,            &y);
    GetFieldValue (__this,  m_screenWidth,  &width);
    GetFieldValue (__this,  m_screenHeight, &height);

    height = height * SK_TGFix_AspectMultiplier;

    SetFieldValue (viewport, _x,      &x);
    SetFieldValue (viewport, _y,      &y);
    SetFieldValue (viewport, _width,  &width);
    SetFieldValue (viewport, _height, &height);
  }
}

using NobleMovieRendereFeature_Create_pfn = void (*)(MonoObject*);
      NobleMovieRendereFeature_Create_pfn
      NobleMovieRendereFeature_Create_Original = nullptr;

void
SK_TGFix_NobleMovieRendereFeature_Create_Detour (MonoObject* __this)
{
  SK_LOG_FIRST_CALL

  NobleMovieRendereFeature_Create_Original (__this);

  //static auto _pass =
  //  GetField ("NobleMovieRendereFeature", "_pass", "Assembly-CSharp", "Noble");
  //
  //auto domain = SK_mono_object_get_domain                     (__this);
  //auto pass   = SK_mono_field_get_value_object (domain, _pass, __this);
  //  
  //if (pass != nullptr)
  //{
  //  static auto UnityEngine_Matrix4x4 =
  //    GetClass (GetImage ("UnityEngine.CoreModule"), "UnityEngine", "Matrix4x4");
  //
  //  static auto cameraview =
  //    GetField ("NobleMovieRendererPass", "cameraview", "Assembly-CSharp", "Noble");
  //
  //  auto view =
  //    SK_mono_field_get_value_object (domain, cameraview, pass);
  //
  //  static auto m33 = GetField (UnityEngine_Matrix4x4, "m33");
  //  static auto m11 = GetField (UnityEngine_Matrix4x4, "m11");
  //
  //  if (     SK_TGFix_AspectRatio > SK_TGFix_NativeAspect)
  //    SetFieldValue (view, m33, &SK_TGFix_AspectMultiplier);
  //  else if (SK_TGFix_AspectRatio < SK_TGFix_NativeAspect)
  //    SetFieldValue (view, m11, &SK_TGFix_AspectMultiplier);
  //}
}

using UnityEngine_Rendering_Volume_OnEnable_pfn = void (*)(MonoObject*);
      UnityEngine_Rendering_Volume_OnEnable_pfn
      UnityEngine_Rendering_Volume_OnEnable_Original = nullptr;

void
SK_TGFix_UnityEngine_Rendering_Volume_OnEnable_Detour (MonoObject* __this)
{
  SK_LOG_FIRST_CALL

  UnityEngine_Rendering_Volume_OnEnable_Original (__this);

  auto pipeline =
    GetStaticFieldValue ("GraphicsSettings", "currentRenderPipeline", "UnityEngine.CoreModule", "UnityEngine.Rendering");

  auto msaa_sample_count =
    GetField ("UniversalRenderPipelineAsset", "msaaSampleCount", "Unity.RenderPipelines.Universal.Runtime", "UnityEngine.Rendering.Universal");

  std::ignore = pipeline, msaa_sample_count;
}

using UnityEngine_Rendering_CommandBuffer_EnableScissorRect_pfn = void (*)(MonoObject*, Unity_Rect*);
      UnityEngine_Rendering_CommandBuffer_EnableScissorRect_pfn
      UnityEngine_Rendering_CommandBuffer_EnableScissorRect_Original = nullptr;

void
SK_TGFix_UnityEngine_Rendering_CommandBuffer_EnableScissorRect_Detour (MonoObject* __this, Unity_Rect* Scissor)
{
  SK_LOG_FIRST_CALL

  // We're nop'ing this entire thing because it breaks shadows if we don't.

  std::ignore = __this, Scissor;
}

bool
SK_TGFix_SetupFramerateHooks (void)
{
  // Mono not init'd
  if (SK_mono_thread_attach == nullptr)
    return false;

  if (! LoadMonoAssembly ("Assembly-CSharp"))
    return false;

  SK_RunOnce (
  {
    auto pfnNobleFrameRateManager_SetQualitySettingFrameRate =
      CompileMethod ("Noble", "FrameRateManager",
             "SetQualitySettingFrameRate", 1);

    auto pfnNobleFrameRateManager_SetTargetFrameRate =
      CompileMethod ("Noble", "FrameRateManager",
                     "SetTargetFrameRate", 1);

    SK_CreateFuncHook (              L"Noble.FrameRateManager.SetQualitySettingFrameRate",
                                     pfnNobleFrameRateManager_SetQualitySettingFrameRate,
                               SK_TGFix_NobleFrameRateManager_SetQualitySettingFrameRate_Detour,
      static_cast_p2p <void> (&         NobleFrameRateManager_SetQualitySettingFrameRate_Original) );

    SK_CreateFuncHook (              L"Noble.FrameRateManager.SetTargetFrameRate",
                                     pfnNobleFrameRateManager_SetTargetFrameRate,
                               SK_TGFix_NobleFrameRateManager_SetTargetFrameRate_Detour,
      static_cast_p2p <void> (&         NobleFrameRateManager_SetTargetFrameRate_Original) );

    SK_QueueEnableHook (pfnNobleFrameRateManager_SetQualitySettingFrameRate);
    SK_QueueEnableHook (pfnNobleFrameRateManager_SetTargetFrameRate);

    auto pfnNoble_CameraManager_SetCameraViewportRect =
      CompileMethod ("Noble", "CameraManager",
                           "SetCameraViewportRect", 1);

    auto pfnNoble_CameraManager_SetCameraAspect =
      CompileMethod ("Noble", "CameraManager",
                           "SetCameraAspect", 1);

    SK_CreateFuncHook (               L"Noble.CameraManager.SetCameraViewportRect",
                                     pfnNoble_CameraManager_SetCameraViewportRect,
                               SK_TGFix_Noble_CameraManager_SetCameraViewportRect_Detour,
      static_cast_p2p <void> (&         Noble_CameraManager_SetCameraViewportRect_Original) );

    SK_CreateFuncHook (               L"Noble.CameraManager.SetCameraAspect",
                                     pfnNoble_CameraManager_SetCameraAspect,
                               SK_TGFix_Noble_CameraManager_SetCameraAspect_Detour,
      static_cast_p2p <void> (&         Noble_CameraManager_SetCameraAspect_Original) );

    SK_QueueEnableHook (pfnNoble_CameraManager_SetCameraViewportRect);
    SK_QueueEnableHook (pfnNoble_CameraManager_SetCameraAspect);

    auto pfnNoble_PrimitiveManager_CalcUIOrthoMatrix =
      CompileMethod ("Noble", "PrimitiveManager",
                     "CalcUIOrthoMatrix", 1);

    SK_CreateFuncHook (               L"Noble.PrimitiveManager.CalcUIOrthoMatrix",
                                     pfnNoble_PrimitiveManager_CalcUIOrthoMatrix,
                               SK_TGFix_Noble_PrimitiveManager_CalcUIOrthoMatrix_Detour,
      static_cast_p2p <void> (&         Noble_PrimitiveManager_CalcUIOrthoMatrix_Original) );

    SK_QueueEnableHook (pfnNoble_PrimitiveManager_CalcUIOrthoMatrix);

    auto pfnNobleMovieRendereFeature_Create =
      CompileMethod ("",
           "NobleMovieRendereFeature",
                                    "Create", 0);

    SK_CreateFuncHook (               L"NobleMovieRendereFeature.Create",
                                     pfnNobleMovieRendereFeature_Create,
                               SK_TGFix_NobleMovieRendereFeature_Create_Detour,
      static_cast_p2p <void> (&         NobleMovieRendereFeature_Create_Original) );

    SK_QueueEnableHook (pfnNobleMovieRendereFeature_Create);

    bool  enabled = SK_EnableApplyQueuedHooks ();
                          SK_ApplyQueuedHooks ();
    if (! enabled) SK_DisableApplyQueuedHooks ();
  });

  if (! LoadMonoAssembly ("UnityEngine.CoreModule"))
    return false;

  SK_RunOnce (
  {
    auto pfnUnityEngine_Screen_SetResolution =
      CompileMethod ("UnityEngine", "Screen",
                     "SetResolution", 3, "UnityEngine.CoreModule");

    SK_CreateFuncHook (               L"UnityEngine.Screen.SetResolution",
                                     pfnUnityEngine_Screen_SetResolution,
                               SK_TGFix_UnityEngine_Screen_SetResolution_Detour,
      static_cast_p2p <void> (&         UnityEngine_Screen_SetResolution_Original) );

    SK_QueueEnableHook (pfnUnityEngine_Screen_SetResolution);

    auto pfnUnityEngine_Rendering_CommandBuffer_EnableScissorRect =
      CompileMethod ("UnityEngine.Rendering", "CommandBuffer", "EnableScissorRect_Injected", 1,
                     "UnityEngine.CoreModule");

    SK_CreateFuncHook (               L"UnityEngine.Rendering.CommandBuffer.EnableScissorRect",
                                     pfnUnityEngine_Rendering_CommandBuffer_EnableScissorRect,
                               SK_TGFix_UnityEngine_Rendering_CommandBuffer_EnableScissorRect_Detour,
      static_cast_p2p <void> (&         UnityEngine_Rendering_CommandBuffer_EnableScissorRect_Original) );

    SK_QueueEnableHook (pfnUnityEngine_Rendering_CommandBuffer_EnableScissorRect);

    bool  enabled = SK_EnableApplyQueuedHooks ();
                          SK_ApplyQueuedHooks ();
    if (! enabled) SK_DisableApplyQueuedHooks ();

  });

  if (! (LoadMonoAssembly ("Unity.RenderPipelines.Core.Runtime") &&
         LoadMonoAssembly ("Unity.RenderPipelines.Universal.Runtime")))
    return false;

  SK_RunOnce (
  {
    auto pfnUnityEngine_Rendering_Volume_OnEnable =
      CompileMethod ("UnityEngine.Rendering", "Volume", "OnEnable", 0,
                     "Unity.RenderPipelines.Core.Runtime");

    SK_CreateFuncHook (               L"UnityEngine.Rendering.Volume.OnEnable",
                                     pfnUnityEngine_Rendering_Volume_OnEnable,
                               SK_TGFix_UnityEngine_Rendering_Volume_OnEnable_Detour,
      static_cast_p2p <void> (&         UnityEngine_Rendering_Volume_OnEnable_Original) );

    SK_QueueEnableHook (pfnUnityEngine_Rendering_Volume_OnEnable);

    SK_LOGi0 (L"Hooked 'UnityEngine.Rendering.Volume.OnEnable' from 'Unity.RenderPipelines.Core.Runtime'");

    bool  enabled = SK_EnableApplyQueuedHooks ();
                          SK_ApplyQueuedHooks ();
    if (! enabled) SK_DisableApplyQueuedHooks ();
  });

  return true;
}

void STDMETHODCALLTYPE SK_TGFix_EndFrame (void)
{
  if (SK_TGFix_NobleFrameRateManagerSingleton != nullptr)
  {
    if ( __target_fps > 0.0f &&
         __target_fps != SK_TGFix_LastSetFrameRateLimit )
    {
      SK_TGFix_LastSetFrameRateLimit =
                          __target_fps;
      void* args [1] = { &__target_fps };

      InvokeMethod ("Noble", "FrameRateManager", "SetTargetFrameRate", 1,
                SK_TGFix_NobleFrameRateManagerSingleton, "Assembly-CSharp", args);

      InvokeMethod ("Noble", "FrameRateManager", "SetQualitySettingFrameRate", 1,
                SK_TGFix_NobleFrameRateManagerSingleton, "Assembly-CSharp", args);
    }
  }

  return;
}