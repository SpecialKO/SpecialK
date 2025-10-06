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
#define __SK_SUBSYSTEM__ L"Unity Plug"

template <typename _T>
class PlugInParameter {
public:
  typedef _T           value_type;

  PlugInParameter (const value_type& value) : cfg_val (value) { };

  operator value_type&  (void) noexcept { return  cfg_val; }
  value_type* operator& (void) noexcept { return &cfg_val; }

  value_type& operator= (const value_type& val) noexcept { return (cfg_val = val); }

  void        store     (void)                  {        if (ini_param != nullptr) ini_param->store ( cfg_val       ); }
  void        store     (const value_type& val) {        if (ini_param != nullptr) ini_param->store ((cfg_val = val)); }
  value_type& load      (void)                  { return     ini_param != nullptr? ini_param->load  ( cfg_val)
                                                                                 :                    cfg_val;         }

  bool bind_to_ini (sk::Parameter <value_type>* _ini) noexcept
  {
    if (!  ini_param) ini_param = _ini;
    return ini_param           == _ini;
  }

private:
  sk::Parameter <value_type>* ini_param = nullptr;
  value_type                  cfg_val   = {     };
};

struct sk_unity_cfg_s {
  // Special K's Windows.Gaming.Input emulation can easily
  //   poll at 1 kHz with zero performance overhead, so just
  //     default to 1 kHz and everyone profits!
  // 
  //  * Game is hardcoded to poll gamepad input at 60 Hz
  //      regardless of device caps or framerate limit, ugh!
  PlugInParameter <float> gamepad_polling_hz = 10000.0f;
} SK_Unity_Cfg;

float SK_Unity_InputPollingFrequency = 60.0f;

bool SK_Unity_HookMonoInit        (void);
void SK_Unity_SetInputPollingFreq (float PollingHz);
bool SK_Unity_SetupFramerateHooks (void);

bool
SK_Unity_PlugInCfg (void)
{
  if (ImGui::CollapsingHeader ("Unity Engine", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::TreePush       ("");
    ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.02f, 0.68f, 0.90f, 0.45f));
    ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.07f, 0.72f, 0.90f, 0.80f));
    ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.14f, 0.78f, 0.87f, 0.80f));

    static int  restart_reqs = 0;

    ImGui::BeginGroup (  );
    if (ImGui::CollapsingHeader (ICON_FA_GAMEPAD " Gamepad Input", ImGuiTreeNodeFlags_DefaultOpen |
                                                                   ImGuiTreeNodeFlags_AllowOverlap))
    {
      ImGui::TreePush ("");

      if (ImGui::SliderFloat ("Polling Frequency", &SK_Unity_Cfg.gamepad_polling_hz, 30.0f, 1000.0f, "%.3f Hz", ImGuiSliderFlags_Logarithmic))
      {
        SK_Unity_SetInputPollingFreq (SK_Unity_Cfg.gamepad_polling_hz);

        SK_Unity_Cfg.gamepad_polling_hz.store ();

        config.utility.save_async ();
      }

      ImGui::SetItemTooltip ("Game defaults to 60 Hz, regardless of framerate cap!");

#if 0
      if (SK_ImGui_HasPlayStationController ())
      {
        ImGui::SeparatorText (ICON_FA_PLAYSTATION " PlayStation Controllers");

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
      }
#endif

      ImGui::TreePop  (  );
    }
    ImGui::EndGroup   (  );

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

HRESULT
STDMETHODCALLTYPE
SK_Unity_PresentFirstFrame (IUnknown* pSwapChain, UINT SyncInterval, UINT Flags);

void
SK_Unity_InitPlugin (void)
{
  SK_RunOnce (
    SK_Unity_HookMonoInit ();

    SK_Unity_Cfg.gamepad_polling_hz.bind_to_ini (
      _CreateConfigParameterFloat ( L"Unity.Input",
                                    L"PollingFrequency",  SK_Unity_Cfg.gamepad_polling_hz,
                                    L"Gamepad Polling Frequency (30.0 Hz - 1000.0Hz; 60.0 Hz == hard-coded game default)" )
    );

    plugin_mgr->config_fns.emplace      (SK_Unity_PlugInCfg);
    plugin_mgr->first_frame_fns.emplace (SK_Unity_PresentFirstFrame);
  );
}

#include <mono/metadata/assembly.h>
#include <mono/metadata/threads.h>
#include <mono/metadata/object.h>

typedef MonoThread*     (*mono_thread_attach_pfn)(MonoDomain* domain);
typedef MonoThread*     (*mono_thread_current_pfn)(void);
typedef void            (*mono_thread_detach_pfn)(MonoThread* thread);
typedef mono_bool       (*mono_thread_is_foreign_pfn)(MonoThread* thread);
typedef MonoDomain*     (*mono_get_root_domain_pfn)(void);
typedef MonoAssembly*   (*mono_domain_assembly_open_pfn)(MonoDomain* doamin, const char* name);
typedef MonoImage*      (*mono_assembly_get_image_pfn)(MonoAssembly* assembly);
typedef MonoString*     (*mono_string_new_pfn)(MonoDomain* domain, const char* text);
typedef MonoObject*     (*mono_object_new_pfn)(MonoDomain* domain, MonoClass* klass);
typedef void*           (*mono_object_unbox_pfn)(MonoObject* obj);
typedef MonoDomain*     (*mono_object_get_domain_pfn)(MonoObject* obj);
typedef MonoClass*      (*mono_object_get_class_pfn)(MonoObject* obj);
typedef MonoClass*      (*mono_class_from_name_pfn)(MonoImage* image, const char* name_space, const char* name);
typedef MonoMethod*     (*mono_class_get_method_from_name_pfn)(MonoClass* klass, const char* name, int param_count);
typedef MonoType*       (*mono_class_get_type_pfn)(MonoClass* klass);
typedef const char*     (*mono_class_get_name_pfn)(MonoClass* klass);
typedef MonoReflectionType*
                        (*mono_type_get_object_pfn)(MonoDomain* domain, MonoType* type);
typedef void*           (*mono_compile_method_pfn)(MonoMethod* method);
typedef MonoObject*     (*mono_runtime_invoke_pfn)(MonoMethod* method, void* obj, void** params, MonoObject** exc);
typedef char*           (*mono_array_addr_with_size_pfn)(MonoArray* array, int size, uintptr_t idx);
typedef uintptr_t       (*mono_array_length_pfn)(MonoArray* array);

typedef MonoClassField* (*mono_class_get_field_from_name_pfn)(MonoClass* klass, const char* name);
typedef void*           (*mono_field_get_value_pfn)(void* obj, MonoClassField* field, void* value);
typedef void            (*mono_field_set_value_pfn)(MonoObject* obj, MonoClassField* field, void* value);
typedef MonoObject*     (*mono_field_get_value_object_pfn)(MonoDomain* domain, MonoClassField* field, MonoObject* obj);
typedef MonoClass*      (*mono_method_get_class_pfn)(MonoMethod* method);
typedef MonoVTable*     (*mono_class_vtable_pfn)(MonoDomain* domain, MonoClass* klass);
typedef void*           (*mono_vtable_get_static_field_data_pfn)(MonoVTable* vt);
typedef uint32_t        (*mono_field_get_offset_pfn)(MonoClassField* field);

typedef MonoMethodDesc* (*mono_method_desc_new_pfn)(const char* name, mono_bool include_namespace);
typedef MonoMethod*     (*mono_method_desc_search_in_image_pfn)(MonoMethodDesc* desc, MonoImage* image);

typedef MonoGCHandle    (*mono_gchandle_new_v2_pfn)(MonoObject* obj, mono_bool pinned);
typedef MonoGCHandle    (*mono_gchandle_free_v2_pfn)(MonoGCHandle gchandle);
typedef MonoObject*     (*mono_gchandle_get_target_v2_pfn)(MonoGCHandle gchandle);

typedef MonoObject*     (*mono_property_get_value_pfn)(MonoProperty *prop, void *obj, void **params, MonoObject **exc);
typedef MonoProperty*   (*mono_class_get_property_from_name_pfn)(MonoClass *klass, const char *name);
typedef MonoMethod*     (*mono_property_get_get_method_pfn)(MonoProperty *prop);
typedef MonoImage*      (*mono_image_loaded_pfn)(const char *name);

#define SK_mono_array_addr(array,type,index) ((type*)SK_mono_array_addr_with_size ((array), sizeof (type), (index)))
#define SK_mono_array_get(array,type,index) (*(type*)SK_mono_array_addr           ((array),         type,  (index)))
#define SK_mono_array_set(array,type,index,value)	                  \
do {                                                                \
  type *__p = (type *) SK_mono_array_addr ((array), type, (index));	\
  *__p = (value);	                                                  \
} while (0)

static mono_thread_attach_pfn                SK_mono_thread_attach                = nullptr;
static mono_thread_current_pfn               SK_mono_thread_current               = nullptr;
static mono_thread_detach_pfn                SK_mono_thread_detach                = nullptr;
static mono_thread_is_foreign_pfn            SK_mono_thread_is_foreign            = nullptr;
static mono_get_root_domain_pfn              SK_mono_get_root_domain              = nullptr;
static mono_domain_assembly_open_pfn         SK_mono_domain_assembly_open         = nullptr;
static mono_assembly_get_image_pfn           SK_mono_assembly_get_image           = nullptr;
static mono_string_new_pfn                   SK_mono_string_new                   = nullptr;
static mono_object_new_pfn                   SK_mono_object_new                   = nullptr;
static mono_object_get_domain_pfn            SK_mono_object_get_domain            = nullptr;
static mono_object_get_class_pfn             SK_mono_object_get_class             = nullptr;
static mono_object_unbox_pfn                 SK_mono_object_unbox                 = nullptr;
static mono_class_from_name_pfn              SK_mono_class_from_name              = nullptr;
static mono_class_get_method_from_name_pfn   SK_mono_class_get_method_from_name   = nullptr;
static mono_class_get_type_pfn               SK_mono_class_get_type               = nullptr;
static mono_class_get_name_pfn               SK_mono_class_get_name               = nullptr;
static mono_type_get_object_pfn              SK_mono_type_get_object              = nullptr;
static mono_compile_method_pfn               SK_mono_compile_method               = nullptr;
static mono_runtime_invoke_pfn               SK_mono_runtime_invoke               = nullptr;
static mono_array_addr_with_size_pfn         SK_mono_array_addr_with_size         = nullptr;
static mono_array_length_pfn                 SK_mono_array_length                 = nullptr;

static mono_property_get_value_pfn           SK_mono_property_get_value           = nullptr;
static mono_class_get_property_from_name_pfn SK_mono_class_get_property_from_name = nullptr;
static mono_property_get_get_method_pfn      SK_mono_property_get_get_method      = nullptr;
static mono_property_get_get_method_pfn      SK_mono_property_get_set_method      = nullptr;
static mono_image_loaded_pfn                 SK_mono_image_loaded                 = nullptr;

static mono_class_get_field_from_name_pfn    SK_mono_class_get_field_from_name    = nullptr;
static mono_field_get_value_pfn              SK_mono_field_get_value              = nullptr;
static mono_field_set_value_pfn              SK_mono_field_set_value              = nullptr;
static mono_field_get_value_object_pfn       SK_mono_field_get_value_object       = nullptr;
static mono_method_get_class_pfn             SK_mono_method_get_class             = nullptr;
static mono_class_vtable_pfn                 SK_mono_class_vtable                 = nullptr;
static mono_vtable_get_static_field_data_pfn SK_mono_vtable_get_static_field_data = nullptr;
static mono_field_get_offset_pfn             SK_mono_field_get_offset             = nullptr;

static mono_gchandle_new_v2_pfn              SK_mono_gchandle_new_v2              = nullptr;
static mono_gchandle_free_v2_pfn             SK_mono_gchandle_free_v2             = nullptr;
static mono_gchandle_get_target_v2_pfn       SK_mono_gchandle_get_target_v2       = nullptr;

static mono_method_desc_new_pfn              SK_mono_method_desc_new              = nullptr;
static mono_method_desc_search_in_image_pfn  SK_mono_method_desc_search_in_image  = nullptr;

static MonoDomain* SK_Unity_MonoDomain = nullptr;

using  mono_jit_init_pfn              = MonoDomain* (*)(const char *file);
static mono_jit_init_pfn
       mono_jit_init_Original         = nullptr;
using  mono_jit_init_version_pfn      = MonoDomain* (*)(const char *root_domain_name, const char *runtime_version);
static mono_jit_init_version_pfn
       mono_jit_init_version_Original = nullptr;

void SK_Unity_OnInitMono (MonoDomain* domain);

static
MonoDomain*
mono_jit_init_Detour (const char *file)
{
  SK_LOG_FIRST_CALL

  auto domain =
    mono_jit_init_Original (file);

  if (domain != nullptr)
  {
    SK_Unity_OnInitMono (domain);
  }

  return domain;
}

static
MonoDomain*
mono_jit_init_version_Detour (const char *root_domain_name, const char *runtime_version)
{
  SK_LOG_FIRST_CALL

  auto domain =
    mono_jit_init_version_Original (root_domain_name, runtime_version);

  if (domain != nullptr)
  {
    SK_Unity_OnInitMono (domain);
  }

  return domain;
}

static
void AttachThread (void)
{
  SK_LOGi1 (L"Attaching Mono to Thread: %x", GetCurrentThreadId ());

  SK_mono_thread_attach (SK_Unity_MonoDomain);
}

static
bool DetachCurrentThreadIfNotNative (void)
{
  MonoThread* this_thread =
    SK_mono_thread_current ();

  if (this_thread != nullptr)
  {
    if (SK_mono_thread_is_foreign (this_thread))
    {
      SK_mono_thread_detach (this_thread);

      SK_LOGi1 (L"Detached Non-Native Mono Thread: tid=%x", GetCurrentThreadId ());

      return true;
    }
  }

  return false;
}

static constexpr wchar_t* mono_dll  = L"mono-2.0-bdwgc.dll";
static constexpr wchar_t* mono_path = LR"(MonoBleedingEdge\EmbedRuntime\mono-2.0-bdwgc.dll)";

bool
SK_Unity_HookMonoInit (void)
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
  SK_mono_class_get_type               = reinterpret_cast <mono_class_get_type_pfn>               (SK_GetProcAddress (hMono, "mono_class_get_type"));
  SK_mono_class_get_name               = reinterpret_cast <mono_class_get_name_pfn>               (SK_GetProcAddress (hMono, "mono_class_get_name"));
  SK_mono_type_get_object              = reinterpret_cast <mono_type_get_object_pfn>              (SK_GetProcAddress (hMono, "mono_type_get_object"));
  SK_mono_compile_method               = reinterpret_cast <mono_compile_method_pfn>               (SK_GetProcAddress (hMono, "mono_compile_method"));
  SK_mono_runtime_invoke               = reinterpret_cast <mono_runtime_invoke_pfn>               (SK_GetProcAddress (hMono, "mono_runtime_invoke"));
  SK_mono_array_addr_with_size         = reinterpret_cast <mono_array_addr_with_size_pfn>         (SK_GetProcAddress (hMono, "mono_array_addr_with_size"));
  SK_mono_array_length                 = reinterpret_cast <mono_array_length_pfn>                 (SK_GetProcAddress (hMono, "mono_array_length"));

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
  SK_mono_property_get_get_method      = reinterpret_cast <mono_property_get_get_method_pfn>      (SK_GetProcAddress (hMono, "mono_property_get_get_method"));
  SK_mono_property_get_set_method      = reinterpret_cast <mono_property_get_get_method_pfn>      (SK_GetProcAddress (hMono, "mono_property_get_set_method"));
  SK_mono_image_loaded                 = reinterpret_cast <mono_image_loaded_pfn>                 (SK_GetProcAddress (hMono, "mono_image_loaded"));
  SK_mono_string_new                   = reinterpret_cast <mono_string_new_pfn>                   (SK_GetProcAddress (hMono, "mono_string_new"));
  SK_mono_object_new                   = reinterpret_cast <mono_object_new_pfn>                   (SK_GetProcAddress (hMono, "mono_object_new"));
  SK_mono_object_get_domain            = reinterpret_cast <mono_object_get_domain_pfn>            (SK_GetProcAddress (hMono, "mono_object_get_domain"));
  SK_mono_object_get_class             = reinterpret_cast <mono_object_get_class_pfn>             (SK_GetProcAddress (hMono, "mono_object_get_class"));
  SK_mono_object_unbox                 = reinterpret_cast <mono_object_unbox_pfn>                 (SK_GetProcAddress (hMono, "mono_object_unbox"));

  SK_mono_method_desc_new              = reinterpret_cast <mono_method_desc_new_pfn>              (SK_GetProcAddress (hMono, "mono_method_desc_new"));
  SK_mono_method_desc_search_in_image  = reinterpret_cast <mono_method_desc_search_in_image_pfn>  (SK_GetProcAddress (hMono, "mono_method_desc_search_in_image"));

  SK_mono_gchandle_new_v2              = reinterpret_cast <mono_gchandle_new_v2_pfn>              (SK_GetProcAddress (hMono, "SK_mono_gchandle_new_v2"));
  SK_mono_gchandle_free_v2             = reinterpret_cast <mono_gchandle_free_v2_pfn>             (SK_GetProcAddress (hMono, "SK_mono_gchandle_free_v2"));
  SK_mono_gchandle_get_target_v2       = reinterpret_cast <mono_gchandle_get_target_v2_pfn>       (SK_GetProcAddress (hMono, "SK_mono_gchandle_get_target_v2"));

  SK_mono_thread_attach                = reinterpret_cast <mono_thread_attach_pfn>                (SK_GetProcAddress (hMono, "mono_thread_attach"));
  SK_mono_get_root_domain              = reinterpret_cast <mono_get_root_domain_pfn>              (SK_GetProcAddress (hMono, "mono_get_root_domain"));
  SK_mono_thread_current               = reinterpret_cast <mono_thread_current_pfn>               (SK_GetProcAddress (hMono, "mono_thread_current"));
  SK_mono_thread_detach                = reinterpret_cast <mono_thread_detach_pfn>                (SK_GetProcAddress (hMono, "mono_thread_detach"));
  SK_mono_thread_is_foreign            = reinterpret_cast <mono_thread_is_foreign_pfn>            (SK_GetProcAddress (hMono, "mono_thread_is_foreign"));

  void*                   pfnMonoJitInitVersion = nullptr;
  SK_CreateDLLHook (         mono_dll,
                            "mono_jit_init_version",
                             mono_jit_init_version_Detour,
    static_cast_p2p <void> (&mono_jit_init_version_Original),
                           &pfnMonoJitInitVersion );
  SK_EnableHook    (        pfnMonoJitInitVersion );

  // If this was pre-loaded, then the above hooks never run and we should initialize everything immediately...
  if (SK_GetModuleHandleW (L"BepInEx.Core.dll"))
  {
    if ( nullptr !=        SK_mono_get_root_domain )
      SK_Unity_OnInitMono (SK_mono_get_root_domain ());
  }


  return true;
}

void SK_Unity_OnInitMono (MonoDomain* domain)
{
  if ((! domain) || SK_mono_thread_attach == nullptr)
    return;

  static bool
        init = false;
  if (! init)
  {
    SK_Unity_MonoDomain = domain;

    SK_ReleaseAssert (domain == SK_mono_get_root_domain ());

    init = SK_Unity_SetupFramerateHooks ();
  }
}

struct {
  MonoImage* assemblyCSharp            = nullptr;
  MonoImage* UnityEngine_CoreModule    = nullptr;
} SK_Unity_MonoAssemblies;

HRESULT
STDMETHODCALLTYPE
SK_Unity_PresentFirstFrame (IUnknown* pSwapChain, UINT SyncInterval, UINT Flags)
{
  UNREFERENCED_PARAMETER (pSwapChain);
  UNREFERENCED_PARAMETER (SyncInterval);
  UNREFERENCED_PARAMETER (Flags);

  // Better late initialization than never...
  if ( nullptr !=        SK_mono_get_root_domain )
    SK_Unity_OnInitMono (SK_mono_get_root_domain ());

  if (SK_Unity_Cfg.gamepad_polling_hz != 60.0f)
  {
    SK_Unity_SetInputPollingFreq (SK_Unity_Cfg.gamepad_polling_hz);
  }

  return S_OK;
}

//
// Various poorly-written examples on using Mono embedded framework from:
//    https://www.unknowncheats.me/forum/unity/603179-hacking-mono-games.html
// 
//  This code all needs to be re-written if it is to be useful in future projects;
//    it is terrible :)
//
static
bool LoadMonoAssembly (const char* assemblyName)
{
  MonoImage* pImage =
    SK_mono_image_loaded (assemblyName);

  if (pImage != nullptr)
    return true;

  MonoDomain* pDomain =
    SK_Unity_MonoDomain;

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

static
void* GetCompiledMethod (const char* nameSpace, const char* className, const char* methodName, int param_count = 0, const char* assemblyName = "Assembly-CSharp")
{
  MonoImage* pImage =
    SK_mono_image_loaded (assemblyName);

  if (! pImage)
  {
    MonoDomain* pDomain =
      SK_Unity_MonoDomain;

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

static
MonoMethod* GetMethod (const char* className, const char* methodName, int param_count = 0, const char* assemblyName = "Assembly-CSharp", const char* nameSpace = "")
{
  MonoImage* pImage =
    SK_mono_image_loaded (assemblyName);

  if (! pImage)
  {
    MonoDomain* pDomain =
      SK_Unity_MonoDomain;

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

static
MonoClass* GetClass (const char* className, const char* assemblyName = "Assembly-CSharp", const char* nameSpace = "")
{
  MonoImage* pImage =
    SK_mono_image_loaded (assemblyName);

  if (! pImage)
  {
    MonoDomain* pDomain =
      SK_Unity_MonoDomain;

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

static
MonoClass* GetClassFromMethod (MonoMethod* method)
{
  return
    SK_mono_method_get_class (method);
}

static
MonoClassField* GetField (const char* className, const char* fieldName, const char* assemblyName = "Assembly-CSharp", const char* nameSpace = "")
{
  MonoImage* pImage =
    SK_mono_image_loaded (assemblyName);

  if (! pImage)
  {
    MonoDomain* pDomain =
      SK_Unity_MonoDomain;

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

static
// Helpers for GetStaticFieldValue
MonoClassField* GetField (MonoClass* pKlass, const char* fieldName)
{
  MonoClassField* pField =
    SK_mono_class_get_field_from_name (pKlass, fieldName);

  return pField;
}

static
uint32_t GetFieldOffset (MonoClassField* field)
{
  return
    SK_mono_field_get_offset (field);
}

static
void GetFieldValue (MonoObject* obj, MonoClassField* field, void* out)
{
  AttachThread ();

  SK_mono_field_get_value (obj, field, out);
}

static
void SetFieldValue (MonoObject* obj, MonoClassField* field, void* value)
{
  SK_mono_field_set_value (obj, field, value);
}

static
MonoVTable* GetVTable (MonoClass* pKlass)
{
  return
    SK_mono_class_vtable (SK_Unity_MonoDomain, pKlass);
}

static
void* GetStaticFieldData (MonoVTable* pVTable)
{
  return
    SK_mono_vtable_get_static_field_data (pVTable);
}

static
void* GetStaticFieldData (MonoClass* pKlass)
{
  MonoVTable* pVTable =
    GetVTable (pKlass);

  if (pVTable == nullptr)
    return nullptr;
 
  return
    SK_mono_vtable_get_static_field_data (pVTable);
}

static
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

static
MonoImage* GetImage (const char* name)
{
  return
    SK_mono_image_loaded (name);
}

static
MonoClass* GetClass (MonoImage* image, const char* _namespace, const char* _class)
{
  return
    SK_mono_class_from_name (image, _namespace, _class);
}

static
MonoMethod* GetClassMethod (MonoClass* class_, const char* name, int params)
{
  return
    SK_mono_class_get_method_from_name (class_, name, params);
}

static
MonoMethod* GetClassMethod (MonoImage* image, const char* namespace_, const char* class_, const char* name, int params)
{
  MonoClass* hClass =
    GetClass (image, namespace_, class_);

  return hClass ? GetClassMethod (hClass, name, params) : NULL;
}

static
MonoMethod* GetClassMethod (const char* namespace_, const char* class_, const char* name, int params, const char* image_ = "Assembly-CSharp")
{
  MonoClass* hClass = GetClass (GetImage (image_), namespace_, class_);
  return     hClass ? GetClassMethod (hClass, name, params) : NULL;
}

static
MonoProperty* GetProperty (MonoImage* image, const char* namespace_, const char* class_, const char* name)
{
  return
    SK_mono_class_get_property_from_name (GetClass (image, namespace_, class_), name);
}

static
MonoObject* GetPropertyValue (MonoProperty* property, uintptr_t instance = 0)
{
  return
    SK_mono_property_get_value (property, reinterpret_cast <void *>(instance), nullptr, 0);
}

static
MonoObject* GetPropertyValue (MonoImage* image, const char* namespace_, const char* class_, const char* name, uintptr_t instance = 0)
{
  return
    GetPropertyValue (GetProperty (image, namespace_, class_, name), instance);
}

static
MonoObject* InvokeMethod (const char* namespace_, const char* class_, const char* method, int paramsCount, void* instance, const char* image_ = "Assembly-CSharp", void** params = nullptr)
{
  if (instance == nullptr)
           return nullptr;

  AttachThread ();

  return
    SK_mono_runtime_invoke (GetClassMethod (GetImage (image_), namespace_, class_, method, paramsCount), instance, params, nullptr);
}

static
void* CompileMethod (MonoMethod* method)
{
  return
    SK_mono_compile_method (method);
}

static
void* CompileMethod (const char* namespace_, const char* class_, const char* name, int params, const char* image_ = "Assembly-CSharp")
{
  return
    CompileMethod (GetClassMethod (GetImage (image_), namespace_, class_, name, params));
}

static
MonoObject* NewObject (MonoClass* klass)
{
  SK_mono_thread_attach (SK_Unity_MonoDomain);

  return
    SK_mono_object_new (SK_Unity_MonoDomain, klass);
}

static
MonoObject* Invoke (MonoMethod* method, void* obj, void** params)
{
  SK_mono_thread_attach (SK_Unity_MonoDomain);
 
  MonoObject* exc;

  return
    SK_mono_runtime_invoke (method, obj, params, &exc);
}

static
MonoObject* ConstructNewObject (MonoClass* klass, int num_args, void** args)
{
  auto ctor =
    SK_mono_class_get_method_from_name (klass, ".ctor", num_args);

  MonoObject*
      instance = NewObject (klass);
  if (instance != nullptr)
  {
    return
      SK_mono_runtime_invoke (ctor, instance, args, nullptr);
  }

  return nullptr;
}

static
MonoObject* ConstructObject (MonoClass* klass, MonoObject* instance, int num_args, void** args)
{
  auto ctor =
    SK_mono_class_get_method_from_name (klass, ".ctor", num_args);

  if (instance != nullptr)
  {
    return
      SK_mono_runtime_invoke (ctor, instance, args, nullptr);
  }

  return nullptr;
}

struct Unity_Rect
{
  float x;
  float y;
  float width;
  float height;
};

struct Unity_Color
{
  float r;
  float g;
  float b;
  float a;
};

struct Unity_Vector2
{
  float x;
  float y;
};

struct Unity_Vector4
{
  float x;
  float y;
  float z;
  float w;
};

struct Unity_Matrix4x4
{
  float m00; float m10; float m20; float m30;
  float m01; float m11; float m21; float m31;
  float m02; float m12; float m22; float m32;
  float m03; float m13; float m23; float m33;
};


template <typename _T>
std::optional     <_T>
static
SK_Mono_InvokeAndUnbox (MonoMethod* method, MonoObject* obj, void** params, MonoObject** exc = nullptr)
{
  if (method != nullptr && obj != nullptr)
  {
    const auto result =
      SK_mono_runtime_invoke (method, obj, params, exc);

    if (result != nullptr)
    {
      _T* unboxed =
        static_cast <_T*> (SK_mono_object_unbox (result));

      if (unboxed != nullptr)
        return *(_T *)unboxed;
    }
  }

  return
    std::nullopt;
}

bool
SK_Unity_SetupFramerateHooks (void)
{
  // Mono not init'd
  if (SK_mono_thread_attach == nullptr)
    return false;

  if (! LoadMonoAssembly ("Assembly-CSharp"))
    return false;

  AttachThread ();

#if 0
  SK_RunOnce (
  {
    //
    // Framerate Hooks
    //
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

    SK_ApplyQueuedHooks ();
  });
#endif

  SK_Unity_MonoAssemblies.assemblyCSharp         = SK_mono_image_loaded ("Assembly-CSharp");
  SK_Unity_MonoAssemblies.UnityEngine_CoreModule = SK_mono_image_loaded ("UnityEngine.CoreModule");

//auto& assemblyCSharp                           = SK_Unity_MonoAssemblies.assemblyCSharp;

  DetachCurrentThreadIfNotNative ();

  if (! LoadMonoAssembly ("UnityEngine.CoreModule"))
    return false;

#if 0
  AttachThread ();

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

    SK_ApplyQueuedHooks ();
  });

  DetachCurrentThreadIfNotNative ();
#endif

  if (! (LoadMonoAssembly ("Unity.RenderPipelines.Core.Runtime") &&
         LoadMonoAssembly ("Unity.RenderPipelines.Universal.Runtime")))
    return false;

  return true;
}

void
SK_Unity_SetInputPollingFreq (float PollingHz)
{
  if (std::exchange (SK_Unity_InputPollingFrequency, PollingHz) == PollingHz)
    return;

  AttachThread ();

  auto NativeInputRuntime_instance =
    GetStaticFieldValue ("NativeInputRuntime", "instance", "Unity.InputSystem", "UnityEngine.InputSystem.LowLevel");

  void* params [1] = { &PollingHz };

  SK_LOGi0 (L"Setting Input Polling Hz: %f", PollingHz);

  InvokeMethod ("UnityEngine.InputSystem.LowLevel", "NativeInputRuntime", "set_pollingFrequency", 1, NativeInputRuntime_instance, "Unity.InputSystem", params);

  // We don't want garbage collection overhead on this thread just because we called a function once!
  DetachCurrentThreadIfNotNative ();
}