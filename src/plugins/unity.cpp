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
  PlugInParameter <float>        gamepad_polling_hz      = 1000.0f;
  PlugInParameter <bool>         gamepad_fix_playstation = true;
  PlugInParameter <std::wstring> gamepad_glyphs          = std::wstring (L"Game Default");
  std::string                    gamepad_glyphs_utf8     =                "Game Default";
  PlugInParameter <float>        time_fixed_delta_time   = 0.0f;
  PlugInParameter <bool>         fixed_delta_auto_sync   = false;
} SK_Unity_Cfg;

float SK_Unity_InputPollingFrequency    = 60.0f;
bool  SK_Unity_FixablePlayStationRumble = false;
bool  SK_Unity_CustomizableGlyphs       = false;
float SK_Unity_OriginalFixedDeltaTime   =  0.0f;
int   SK_Unity_GlyphEnumVal             =    -1;
bool  SK_Unity_GlyphCacheDirty          = false;

HANDLE SK_Unity_GetFrameStatsWaitEvent = 0;
bool   SK_Unity_PaceGameThread         = true;

bool SK_Unity_HookMonoInit        (void);
void SK_Unity_SetInputPollingFreq (float PollingHz);
bool SK_Unity_SetupInputHooks     (void);
void SK_Unity_UpdateGlyphOverride (void);
void SK_Unity_SetFixedDeltaTime   (float fixed_delta_time);

bool
SK_Unity_PlugInCfg (void)
{
  bool show_controller_cfg = true;

  if (! SK_Unity_FixablePlayStationRumble)
    show_controller_cfg = false;

  if (! (SK_ImGui_HasPlayStationController () || SK_XInput_PollController (0)))
    show_controller_cfg = false;

  if (ImGui::CollapsingHeader ("Unity Engine", ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::TreePush       ("");

    if (SK_Unity_Cfg.time_fixed_delta_time == 0.0f && SK_Unity_OriginalFixedDeltaTime != 0.0f)
    {   SK_Unity_Cfg.time_fixed_delta_time =          SK_Unity_OriginalFixedDeltaTime;       }
    
    float delta_hz =
      (1.0f / SK_Unity_Cfg.time_fixed_delta_time);

    if (SK_Unity_Cfg.fixed_delta_auto_sync) ImGui::BeginDisabled ();
    {
      if (SK_Unity_Cfg.time_fixed_delta_time == SK_Unity_OriginalFixedDeltaTime)
      {
        ImGui::TextColored    (ImVec4 (0.333f, 0.666f, 0.999f, 1.f), ICON_FA_INFO_CIRCLE);
        ImGui::SameLine       ();
        ImGui::SetItemTooltip ("Unity games will run smoother if you match Framerate to Fixed Delta Time.");
        ImGui::SameLine       ();
      }

      if (ImGui::SliderFloat ("Unity Fixed Delta Time", &delta_hz, 1.0f, 240.0f, "%.3f Hz"))
      {
        SK_Unity_Cfg.time_fixed_delta_time = delta_hz > 0.0f ? 1.0f / delta_hz : SK_Unity_OriginalFixedDeltaTime;
        SK_Unity_Cfg.time_fixed_delta_time.store ();

        config.utility.save_async ();

        SK_Unity_SetFixedDeltaTime (SK_Unity_Cfg.time_fixed_delta_time);
      }
      
      if (SK_ImGui_IsItemRightClicked ())
      {
        if (__target_fps > 0.0f)
        {
          SK_Unity_Cfg.time_fixed_delta_time = 1.0f / __target_fps;
          SK_Unity_Cfg.time_fixed_delta_time.store ();

          config.utility.save_async ();

          SK_Unity_SetFixedDeltaTime (SK_Unity_Cfg.time_fixed_delta_time);
        }
      }

      if (ImGui::BeginItemTooltip ())
      { ImGui::TextUnformatted    ("Set the animation rate for Unity.");
        ImGui::Separator          ();
        ImGui::BulletText         ("This may cause physics issues in some games if changed, but can be reset easily.");
        if (__target_fps > 0.0f)
        { ImGui::Separator        ();
          ImGui::TextUnformatted  (" " ICON_FA_MOUSE " Right-click to Match Framerate Limit");
        } ImGui::EndTooltip       ();
      }
      ImGui::SameLine ();
    }

    auto _Reset = [&](void)
    {
      SK_Unity_Cfg.time_fixed_delta_time = SK_Unity_OriginalFixedDeltaTime;
      SK_Unity_Cfg.time_fixed_delta_time.store ();

      SK_Unity_Cfg.fixed_delta_auto_sync = false;
      SK_Unity_Cfg.fixed_delta_auto_sync.store ();

      config.utility.save_async ();

      SK_Unity_SetFixedDeltaTime (SK_Unity_OriginalFixedDeltaTime);
    };

    if (SK_Unity_Cfg.fixed_delta_auto_sync) ImGui::EndDisabled ();

    if (ImGui::Checkbox ("Match Framerate Limit", &SK_Unity_Cfg.fixed_delta_auto_sync))
    {
      if (SK_Unity_Cfg.fixed_delta_auto_sync && __target_fps > 0.0f)
      {
        SK_Unity_Cfg.time_fixed_delta_time = 1.0f / __target_fps;
        SK_Unity_Cfg.time_fixed_delta_time.store ();
        SK_Unity_Cfg.fixed_delta_auto_sync.store ();

        config.utility.save_async ();

        SK_Unity_SetFixedDeltaTime (SK_Unity_Cfg.time_fixed_delta_time);
      }

      else if (! SK_Unity_Cfg.fixed_delta_auto_sync) _Reset ();
    }

    ImGui::SetItemTooltip ("Enable for Smoothest Frame Pacing");

    if (SK_Unity_OriginalFixedDeltaTime != SK_Unity_Cfg.time_fixed_delta_time)
    {
      ImGui::SameLine   (       );
      if (ImGui::Button ("Reset"))
      {
        _Reset ();
      }
    }

    if (SK_Unity_GetFrameStatsWaitEvent != 0)
    {
      ImGui::Checkbox ("Pace Unity Game Thread", &SK_Unity_PaceGameThread);

      ImGui::SetItemTooltip ("Experimental synchronization between render thread and game thread using framerate limiter timing.");
    }

    if (show_controller_cfg)
    {
    ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.02f, 0.68f, 0.90f, 0.45f));
    ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.07f, 0.72f, 0.90f, 0.80f));
    ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.14f, 0.78f, 0.87f, 0.80f));

    static int  restart_reqs = 0;

    ImGui::BeginGroup (  );
    if (ImGui::CollapsingHeader (ICON_FA_GAMEPAD " Gamepad Input", ImGuiTreeNodeFlags_DefaultOpen |
                                                                   ImGuiTreeNodeFlags_AllowOverlap))
    {
      ImGui::TreePush ("");

      static std::map <int, std::string> table_fwd = {
        {  0, "Game Default"     },
        {  1, "Xbox360"          },
        {  2, "XboxOne"          },
        {  3, "XboxSeriesX"      },
        {  4, "PlayStation2"     },
        {  5, "PlayStation3"     },
        {  6, "PlayStation4"     },
        {  7, "PlayStation5"     },
        {  8, "PlayStationVita"  },
        {  9, "PlayStationMove"  },
        { 10, "Steam"            },
        { 11, "SteamDeck"        },
        { 12, "AppleMFi"         },
        { 13, "AmazonFireTV"     },
        { 14, "NVIDIAShield"     },
        { 15, "NintendoNES"      },
        { 16, "NintendoSNES"     },
        { 17, "Nintendo64"       },
        { 18, "NintendoGameCube" },
        { 19, "NintendoWii"      },
        { 20, "NintendoWiiU"     },
        { 21, "NintendoSwitch"   },
        { 22, "GoogleStadia"     },
        { 23, "Vive"             },
        { 24, "Oculus"           },
        { 25, "Logitech"         },
        { 26, "Thrustmaster"     }
      };

      static std::map <std::string, int> table_rev = {
        { "Game Default",      0 },
        { "Xbox360",           1 },
        { "XboxOne",           2 },
        { "XboxSeriesX",       3 },
        { "PlayStation2",      4 },
        { "PlayStation3",      5 },
        { "PlayStation4",      6 },
        { "PlayStation5",      7 },
        { "PlayStationVita",   8 },
        { "PlayStationMove",   9 },
        { "Steam",            10 },
        { "SteamDeck",        11 },
        { "AppleMFi",         12 },
        { "AmazonFireTV",     13 },
        { "NVIDIAShield",     14 },
        { "NintendoNES",      15 },
        { "NintendoSNES",     16 },
        { "Nintendo64",       17 },
        { "NintendoGameCube", 18 },
        { "NintendoWii",      19 },
        { "NintendoWiiU",     20 },
        { "NintendoSwitch",   21 },
        { "GoogleStadia",     22 },
        { "Vive",             23 },
        { "Oculus",           24 },
        { "Logitech",         25 },
        { "Thrustmaster",     26 }
      };

      static int sel =
        std::max (0, table_rev [SK_Unity_Cfg.gamepad_glyphs_utf8.c_str ()]);

      if (SK_Unity_CustomizableGlyphs)
      {
      if (ImGui::Combo ("Button Prompts", &sel, "Game Default\0"
                                                "Xbox 360\0"
                                                "Xbox One\0"
                                                "Xbox Series X\0"
                                                "PlayStation 2\0"
                                                "PlayStation 3\0"
                                                "PlayStation 4\0"
                                                "PlayStation 5\0"
                                                "PlayStation Vita\0"
                                                "PlayStation Move\0"
                                                "Steam\0" 
                                                "Steam Deck\0"
                                                "Apple MFi\0"
                                                "Amazon FireTV\0"
                                                "NVIDIA Shield\0"
                                                "Nintendo NES\0"
                                                "Nintendo SNES\0"
                                                "Nintendo 64\0"
                                                "Nintendo GameCube\0"
                                                "Nintendo Wii\0"
                                                "Nintendo WiiU\0"
                                                "Nintendo Switch\0"
                                                "Google Stadia\0"
                                                "Vive\0"
                                                "Oculus\0"
                                                "Logitech\0"
                                                "Thrustmaster\0\0"))
      {
        SK_Unity_Cfg.gamepad_glyphs_utf8 = table_fwd [sel];
        SK_Unity_Cfg.gamepad_glyphs      =
          SK_UTF8ToWideChar (SK_Unity_Cfg.gamepad_glyphs_utf8);

        SK_Unity_Cfg.gamepad_glyphs.store ();

        config.utility.save_async ();

        SK_Unity_UpdateGlyphOverride ();
      }

      if (ImGui::BeginItemTooltip ())
      {
        ImGui::TextUnformatted (
          "Games will typically revert to Xbox 360 prompts or closest match "
          "if they do not have the exact buttons requested." );
        ImGui::Separator  ( );
        ImGui::BulletText (
          "If changes do not take effect immediately, try "
          "disconnecting and reconnecting the controller." );
        ImGui::EndTooltip ( );
      }
      }
#if 0
      if (ImGui::SliderFloat ("Polling Frequency", &SK_Unity_Cfg.gamepad_polling_hz, 30.0f, 1000.0f, "%.3f Hz", ImGuiSliderFlags_Logarithmic))
      {
        ////SK_Unity_SetInputPollingFreq (SK_Unity_Cfg.gamepad_polling_hz);

        SK_Unity_Cfg.gamepad_polling_hz.store ();

        config.utility.save_async ();
      }

      ImGui::SetItemTooltip ("Game defaults to 60 Hz, regardless of framerate cap!");
#endif

      if (SK_ImGui_HasPlayStationController () && SK_Unity_FixablePlayStationRumble)
      {
        ImGui::SeparatorText (ICON_FA_PLAYSTATION " PlayStation Controllers");

        if (ImGui::Checkbox ("Fix Missing Vibration", &SK_Unity_Cfg.gamepad_fix_playstation))
        {
          SK_Unity_Cfg.gamepad_fix_playstation.store ();

          config.utility.save_async ();
        }

        ImGui::SetItemTooltip (
          "Unity Engine has non-functional vibration on PlayStation controllers.\r\n\r\n  "
          ICON_FA_INFO_CIRCLE " Rather than emulating Xbox, this will fix Unity's native PlayStation code."
        );
      }

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
    }

    ImGui::TreePop       ( );
  }

  return true;
}

HRESULT
STDMETHODCALLTYPE
SK_Unity_PresentFirstFrame (IUnknown* pSwapChain, UINT SyncInterval, UINT Flags);

void
__stdcall
SK_Unity_EndFrame (void);

bool SK_Unity_Hookil2cppInit         (void);
bool SK_Unity_SetupInputHooks_il2cpp (void);

void
SK_Unity_InitPlugin (void)
{
  SK_RunOnce (
/*
    SK_Unity_Cfg.gamepad_polling_hz.bind_to_ini (
      _CreateConfigParameterFloat ( L"Unity.Input",
                                    L"PollingFrequency",  SK_Unity_Cfg.gamepad_polling_hz,
                                    L"Gamepad Polling Frequency (30.0 Hz - 1000.0Hz; 60.0 Hz == hard-coded game default)" )
    );
*/

    // Needs this setting or judder is out of control.
    if (SK_IsCurrentGame (SK_GAME_ID::PrinceOfPersia_TheLostCrown))
      SK_Unity_Cfg.fixed_delta_auto_sync = true;

    SK_Unity_Cfg.gamepad_fix_playstation.bind_to_ini (
      _CreateConfigParameterBool ( L"Unity.Input",
                                   L"FixPlayStationVibration",  SK_Unity_Cfg.gamepad_fix_playstation,
                                   L"Add Vibration Support for DualShock 4 and DualSense" )
    );

    SK_Unity_Cfg.gamepad_glyphs.bind_to_ini (
      _CreateConfigParameterStringW ( L"Unity.Input",
                                      L"ButtonPrompts",  SK_Unity_Cfg.gamepad_glyphs,
                                      L"Override a game's displayed prompts" )
    );

    SK_Unity_Cfg.time_fixed_delta_time.bind_to_ini (
      _CreateConfigParameterFloat ( L"Unity.Framerate",
                                    L"FixedDeltaTimeInMsec",  SK_Unity_Cfg.time_fixed_delta_time,
                                    L"Fixed Delta Time (in Msec)" )
    );

    SK_Unity_Cfg.fixed_delta_auto_sync.bind_to_ini (
      _CreateConfigParameterBool ( L"Unity.Framerate",
                                   L"MatchDeltaTimeToLimit",  SK_Unity_Cfg.fixed_delta_auto_sync,
                                   L"Adjust Delte Time to match SK's configured Framerate Limit" )
    );

    SK_Unity_Cfg.gamepad_glyphs_utf8 = SK_WideCharToUTF8 (SK_Unity_Cfg.gamepad_glyphs);
  );

  bool init  = SK_Unity_HookMonoInit   ();
       init |= SK_Unity_Hookil2cppInit ();

  if (init)
  {
    SK_RunOnce (
      plugin_mgr->config_fns.emplace      (SK_Unity_PlugInCfg);
      plugin_mgr->first_frame_fns.emplace (SK_Unity_PresentFirstFrame);
      plugin_mgr->end_frame_fns.emplace   (SK_Unity_EndFrame);
    );
  }
}

#include <mono/metadata/assembly.h>
#include <mono/metadata/threads.h>
#include <mono/metadata/object.h>

#define SK_MONO_API

typedef MonoThread*     (SK_MONO_API *mono_thread_attach_pfn)(MonoDomain* domain);
typedef MonoThread*     (SK_MONO_API *mono_thread_current_pfn)(void);
typedef void            (SK_MONO_API *mono_thread_detach_pfn)(MonoThread* thread);
typedef mono_bool       (SK_MONO_API *mono_thread_is_foreign_pfn)(MonoThread* thread);
typedef MonoDomain*     (SK_MONO_API *mono_get_root_domain_pfn)(void);
typedef MonoAssembly*   (SK_MONO_API *mono_domain_assembly_open_pfn)(MonoDomain* doamin, const char* name);
typedef MonoImage*      (SK_MONO_API *mono_assembly_get_image_pfn)(MonoAssembly* assembly);
typedef MonoString*     (SK_MONO_API *mono_string_new_pfn)(MonoDomain* domain, const char* text);
typedef MonoObject*     (SK_MONO_API *mono_object_new_pfn)(MonoDomain* domain, MonoClass* klass);
typedef void*           (SK_MONO_API *mono_object_unbox_pfn)(MonoObject* obj);
typedef MonoDomain*     (SK_MONO_API *mono_object_get_domain_pfn)(MonoObject* obj);
typedef MonoClass*      (SK_MONO_API *mono_object_get_class_pfn)(MonoObject* obj);
typedef MonoClass*      (SK_MONO_API *mono_class_from_name_pfn)(MonoImage* image, const char* name_space, const char* name);
typedef MonoClass*      (SK_MONO_API *mono_class_get_parent_pfn)(MonoClass* klass);
typedef MonoMethod*     (SK_MONO_API *mono_class_get_method_from_name_pfn)(MonoClass* klass, const char* name, int param_count);
typedef MonoType*       (SK_MONO_API *mono_class_get_type_pfn)(MonoClass* klass);
typedef const char*     (SK_MONO_API *mono_class_get_name_pfn)(MonoClass* klass);
typedef MonoReflectionType*
                        (SK_MONO_API *mono_type_get_object_pfn)(MonoDomain* domain, MonoType* type);
typedef void*           (SK_MONO_API *mono_compile_method_pfn)(MonoMethod* method);
typedef MonoObject*     (SK_MONO_API *mono_runtime_invoke_pfn)(MonoMethod* method, void* obj, void** params, MonoObject** exc);
typedef char*           (SK_MONO_API *mono_array_addr_with_size_pfn)(MonoArray* array, int size, uintptr_t idx);
typedef uintptr_t       (SK_MONO_API *mono_array_length_pfn)(MonoArray* array);

typedef MonoClassField* (SK_MONO_API *mono_class_get_field_from_name_pfn)(MonoClass* klass, const char* name);
typedef void*           (SK_MONO_API *mono_field_get_value_pfn)(void* obj, MonoClassField* field, void* value);
typedef void            (SK_MONO_API *mono_field_set_value_pfn)(MonoObject* obj, MonoClassField* field, void* value);
typedef MonoObject*     (SK_MONO_API *mono_field_get_value_object_pfn)(MonoDomain* domain, MonoClassField* field, MonoObject* obj);
typedef MonoClass*      (SK_MONO_API *mono_method_get_class_pfn)(MonoMethod* method);
typedef MonoVTable*     (SK_MONO_API *mono_class_vtable_pfn)(MonoDomain* domain, MonoClass* klass);
typedef void*           (SK_MONO_API *mono_vtable_get_static_field_data_pfn)(MonoVTable* vt);
typedef uint32_t        (SK_MONO_API *mono_field_get_offset_pfn)(MonoClassField* field);

typedef MonoMethodDesc* (SK_MONO_API *mono_method_desc_new_pfn)(const char* name, mono_bool include_namespace);
typedef MonoMethod*     (SK_MONO_API *mono_method_desc_search_in_image_pfn)(MonoMethodDesc* desc, MonoImage* image);

typedef MonoGCHandle    (SK_MONO_API *mono_gchandle_new_v2_pfn)(MonoObject* obj, mono_bool pinned);
typedef MonoGCHandle    (SK_MONO_API *mono_gchandle_free_v2_pfn)(MonoGCHandle gchandle);
typedef MonoObject*     (SK_MONO_API *mono_gchandle_get_target_v2_pfn)(MonoGCHandle gchandle);

typedef MonoObject*     (SK_MONO_API *mono_property_get_value_pfn)(MonoProperty *prop, void *obj, void **params, MonoObject **exc);
typedef void            (SK_MONO_API *mono_property_set_value_pfn)(MonoProperty *prop, void *obj, void **params, MonoObject **exc);
typedef MonoProperty*   (SK_MONO_API *mono_class_get_property_from_name_pfn)(MonoClass *klass, const char *name);
typedef MonoMethod*     (SK_MONO_API *mono_property_get_get_method_pfn)(MonoProperty *prop);
typedef MonoImage*      (SK_MONO_API *mono_image_loaded_pfn)(const char *name);

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
static mono_class_get_parent_pfn             SK_mono_class_get_parent             = nullptr;
static mono_class_get_type_pfn               SK_mono_class_get_type               = nullptr;
static mono_class_get_name_pfn               SK_mono_class_get_name               = nullptr;
static mono_type_get_object_pfn              SK_mono_type_get_object              = nullptr;
static mono_compile_method_pfn               SK_mono_compile_method               = nullptr;
static mono_runtime_invoke_pfn               SK_mono_runtime_invoke               = nullptr;
static mono_array_addr_with_size_pfn         SK_mono_array_addr_with_size         = nullptr;
static mono_array_length_pfn                 SK_mono_array_length                 = nullptr;

static mono_property_get_value_pfn           SK_mono_property_get_value           = nullptr;
static mono_property_set_value_pfn           SK_mono_property_set_value           = nullptr;
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

using  mono_jit_exec_pfn              = int         (SK_MONO_API *)(MonoDomain *domain, MonoAssembly *assembly, int argc, char *argv[]);
static mono_jit_exec_pfn
       mono_jit_exec_Original         = nullptr;
using  mono_jit_init_pfn              = MonoDomain* (SK_MONO_API *)(const char *file);
static mono_jit_init_pfn
       mono_jit_init_Original         = nullptr;
using  mono_jit_init_version_pfn      = MonoDomain* (SK_MONO_API *)(const char *root_domain_name, const char *runtime_version);
static mono_jit_init_version_pfn
       mono_jit_init_version_Original = nullptr;

void SK_Unity_OnInitMono (MonoDomain* domain = nullptr);

static
int
SK_MONO_API
mono_jit_exec_Detour (MonoDomain *domain, MonoAssembly *assembly, int argc, char *argv[])
{
  auto ret =
    mono_jit_exec_Original (domain, assembly, argc, argv);

  if (domain != nullptr)
  {
    SK_Unity_OnInitMono (domain);
  }

  return ret;
}

static
MonoDomain*
SK_MONO_API
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
SK_MONO_API
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

#include <aetherim/image.hpp>
#include <aetherim/field.hpp>
#include <aetherim/class.hpp>
#include <aetherim/wrapper.hpp>

#include <aetherim/api.hpp>

static
MonoThread* AttachThread (void)
{
  if (! SK_mono_thread_attach)
    return nullptr;

  SK_LOGi3 (L"Attaching Mono to Thread: %x", GetCurrentThreadId ());

  if (SK_Unity_MonoDomain == nullptr)
      SK_Unity_MonoDomain = SK_mono_get_root_domain ();

  return
    SK_mono_thread_attach (SK_Unity_MonoDomain);
}

static
bool DetachCurrentThreadIfNotNative (void)
{
  if (! SK_mono_thread_current)
  {
    if (Il2cpp::thread_detach != nullptr)
      Il2cpp::thread_detach (Il2cpp::thread_current ());

    return false;
  }

  MonoThread* this_thread =
    SK_mono_thread_current ();

  if (this_thread != nullptr)
  {
    // This API doesn't exist in older versions of Mono.
    if (SK_mono_thread_is_foreign == nullptr ||
        SK_mono_thread_is_foreign (this_thread))
    {
      SK_mono_thread_detach (this_thread);

      SK_LOGi1 (L"Detached Non-Native Mono Thread: tid=%x", GetCurrentThreadId ());

      return true;
    }
  }

  return false;
}

class SK_Mono_ScopedThreadAttach
{
public:
  SK_Mono_ScopedThreadAttach (void)
  {
    if (SK_mono_thread_current != nullptr)
    {
      if (SK_mono_thread_current () == nullptr)
         attached_thread = AttachThread ();
    }
  }

  ~SK_Mono_ScopedThreadAttach (void)
  {
    if (SK_mono_thread_detach != nullptr)
    {
      if (attached_thread != nullptr)
      {
        SK_mono_thread_detach (attached_thread);
      }
    }
  }

private:
  MonoThread* attached_thread = nullptr;
};

static constexpr wchar_t* mono_dll  = L"mono-2.0-bdwgc.dll";
static constexpr wchar_t* mono_path = LR"(MonoBleedingEdge\EmbedRuntime\mono-2.0-bdwgc.dll)";

static constexpr wchar_t* mono_alt_dll  = L"mono.dll";
static constexpr wchar_t* mono_alt_path = LR"(Mono\EmbedRuntime\mono.dll)";

bool
SK_Unity_HookMonoInit (void)
{
  static bool
      once = false;
  if (once)
    return once;

  const wchar_t* loaded_mono_dll = mono_dll;

  SK_LoadLibraryW (mono_path);

  HMODULE hMono =
    GetModuleHandleW (mono_dll);

  if (hMono == NULL)
  {
    SK_LoadLibraryW (mono_alt_path);

    hMono =
      GetModuleHandleW (mono_alt_dll);

    if (hMono == NULL)
    {
      return false;
    }

    loaded_mono_dll = mono_alt_dll;
  }

  once = true;

  SK_mono_domain_assembly_open         = reinterpret_cast <mono_domain_assembly_open_pfn>         (SK_GetProcAddress (hMono, "mono_domain_assembly_open"));
  SK_mono_assembly_get_image           = reinterpret_cast <mono_assembly_get_image_pfn>           (SK_GetProcAddress (hMono, "mono_assembly_get_image"));
  SK_mono_class_from_name              = reinterpret_cast <mono_class_from_name_pfn>              (SK_GetProcAddress (hMono, "mono_class_from_name"));
  SK_mono_class_get_method_from_name   = reinterpret_cast <mono_class_get_method_from_name_pfn>   (SK_GetProcAddress (hMono, "mono_class_get_method_from_name"));
  SK_mono_class_get_parent             = reinterpret_cast <mono_class_get_parent_pfn>             (SK_GetProcAddress (hMono, "mono_class_get_parent"));
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
  SK_mono_property_set_value           = reinterpret_cast <mono_property_set_value_pfn>           (SK_GetProcAddress (hMono, "mono_property_set_value"));
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
  SK_CreateDLLHook (  loaded_mono_dll,
                            "mono_jit_init_version",
                             mono_jit_init_version_Detour,
    static_cast_p2p <void> (&mono_jit_init_version_Original),
                           &pfnMonoJitInitVersion );
  SK_EnableHook    (        pfnMonoJitInitVersion );

  void*                   pfnMonoJitExec = nullptr;
  SK_CreateDLLHook (  loaded_mono_dll,
                            "mono_jit_exec",
                             mono_jit_exec_Detour,
    static_cast_p2p <void> (&mono_jit_exec_Original),
                           &pfnMonoJitExec );
  SK_EnableHook    (        pfnMonoJitExec );

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
  if (domain == nullptr && SK_mono_get_root_domain != nullptr)
      domain = SK_mono_get_root_domain ();

  if ((! domain) || SK_mono_thread_attach == nullptr)
    return;

  static bool
        init = false;
  if (! init)
  {
    SK_Unity_MonoDomain = domain;

    SK_ReleaseAssert (domain == SK_mono_get_root_domain ());

    init = SK_Unity_SetupInputHooks ();
  }
}

struct {
  MonoImage* assemblyCSharp    = nullptr;
  MonoImage* assemblyInControl = nullptr;
  MonoImage* assemblyRewired   = nullptr;
} SK_Unity_MonoAssemblies;

struct {
  struct {
    struct {
      MonoClassField* DeviceStyle = nullptr;
    } InputDevice;
  } InControl;
} SK_Unity_MonoFields;

struct {
  struct {
    MonoClass* InputDevice = nullptr;
  } InControl;
} SK_Unity_MonoClasses;

typedef int (__fastcall *il2cpp_init_pfn)(const char* domain_name);
typedef int (__fastcall *il2cpp_init_utf16_pfn)(const wchar_t* domain_name);
typedef void (__fastcall *il2cpp_shutdown_pfn)(void);

static il2cpp_init_pfn       il2cpp_init_Original       = nullptr;
static il2cpp_init_utf16_pfn il2cpp_init_utf16_Original = nullptr;
static il2cpp_shutdown_pfn   il2cpp_shutdown_Original   = nullptr;

int
__fastcall
il2cpp_init_Detour (const char* domain_name)
{
  SK_LOG_FIRST_CALL

  auto ret =
    il2cpp_init_Original (domain_name);

  SK_Unity_SetupInputHooks_il2cpp ();

  return ret;
}

int
__fastcall
il2cpp_init_utf16_Detour (const wchar_t* domain_name)
{
  SK_LOG_FIRST_CALL

  auto ret =
    il2cpp_init_utf16_Original (domain_name);

  SK_Unity_SetupInputHooks_il2cpp ();

  return ret;
}

void
__fastcall
il2cpp_shutdown_Detour (void)
{
  SK_LOG_FIRST_CALL

  return
    il2cpp_shutdown_Original ();
}

bool
SK_Unity_Hookil2cppInit (void)
{
  static bool
      once = false;
  if (once)
    return once;

  HMODULE hModIl2Cpp =
    GetModuleHandleW (L"GameAssembly.dll");

  if (hModIl2Cpp == NULL)
    return false;

  once = true;

  Il2cpp::initialize ();

  void*                   pfnil2cpp_init = nullptr;
  SK_CreateDLLHook (       L"GameAssembly.dll",
                            "il2cpp_init",
                             il2cpp_init_Detour,
    static_cast_p2p <void> (&il2cpp_init_Original),
                         &pfnil2cpp_init );
  SK_EnableHook    (      pfnil2cpp_init );

  void*                   pfnil2cpp_init_utf16 = nullptr;
  SK_CreateDLLHook (       L"GameAssembly.dll",
                            "il2cpp_init_utf16",
                             il2cpp_init_utf16_Detour,
    static_cast_p2p <void> (&il2cpp_init_utf16_Original),
                         &pfnil2cpp_init_utf16 );
  SK_EnableHook    (      pfnil2cpp_init_utf16 );

  return true;
}

il2cpp::Wrapper* SK_il2cpp_Wrapper = nullptr;

struct {  
  Image* assemblyCSharp    = nullptr;
  Image* assemblyInControl = nullptr;
  Image* assemblyRewired   = nullptr;
} SK_Unity_il2cppAssemblies;

struct {
  struct {
    struct {
      Field* DeviceStyle = nullptr;
    } InputDevice;
  } InControl;
} SK_Unity_il2cppFields;

struct {
  struct {
    Class* InputDevice = nullptr;
  } InControl;
} SK_Unity_il2cppClasses;

void
__stdcall
SK_Unity_EndFrame (void)
{
  static float last_fps = 0;

  bool forced_update =
    (SK_Unity_Cfg.fixed_delta_auto_sync) && last_fps != __target_fps && __target_fps > 0.0f;

  // Stupid hack to ensure this is applied initially; may take many frames.
  if (SK_GetFramesDrawn () >= 15 && (forced_update || SK_GetFramesDrawn () < 1500))
  {
    static DWORD
        dwLastForced = 0;
    if (dwLastForced < SK::ControlPanel::current_time - 2500UL || forced_update)
    {   dwLastForced = SK::ControlPanel::current_time;
      SK_RunOnce (SK_Unity_SetFixedDeltaTime (0.0f));

      if (forced_update)
      {
        SK_Unity_Cfg.time_fixed_delta_time = 1.0f/__target_fps;
      }

      if (SK_Unity_Cfg.time_fixed_delta_time != 0.0f &&
          SK_Unity_Cfg.time_fixed_delta_time != SK_Unity_OriginalFixedDeltaTime)
      {
        if (SK_Unity_Cfg.fixed_delta_auto_sync) last_fps = __target_fps;
        SK_Unity_SetFixedDeltaTime (SK_Unity_Cfg.time_fixed_delta_time);
      }

      if (SK_Unity_Cfg.gamepad_polling_hz != 60.0f)
      {
        SK_Unity_SetInputPollingFreq (SK_Unity_Cfg.gamepad_polling_hz);
      }
    }
  }
}

HRESULT
STDMETHODCALLTYPE
SK_Unity_PresentFirstFrame (IUnknown* pSwapChain, UINT SyncInterval, UINT Flags)
{
  UNREFERENCED_PARAMETER (pSwapChain);
  UNREFERENCED_PARAMETER (SyncInterval);
  UNREFERENCED_PARAMETER (Flags);

  // Better late initialization than never...
  if (SK_mono_get_root_domain != nullptr)
    SK_Unity_OnInitMono ();
  else
    SK_Unity_SetupInputHooks_il2cpp ();

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
  {
    pDomain = SK_mono_get_root_domain ();

    if (pDomain == nullptr)
    {
      return false;
    }
  }

  AttachThread ();
 
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
  if (method == nullptr)
         return nullptr;

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
  if (pKlass == nullptr)
         return nullptr;

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
  if (obj == nullptr || field == nullptr || out == nullptr)
    return;

  AttachThread ();

  SK_mono_field_get_value (obj, field, out);
}

static
void SetFieldValue (MonoObject* obj, MonoClassField* field, void* value)
{
  if (obj == nullptr || field == nullptr || value == nullptr)
    return;

  SK_mono_field_set_value (obj, field, value);
}

static
MonoVTable* GetVTable (MonoClass* pKlass)
{
  if (pKlass == nullptr)
         return nullptr;

  return
    SK_mono_class_vtable (SK_Unity_MonoDomain, pKlass);
}

static
void* GetStaticFieldData (MonoVTable* pVTable)
{
  if (pVTable == nullptr)
    return nullptr;

  return
    SK_mono_vtable_get_static_field_data (pVTable);
}

static
void* GetStaticFieldData (MonoClass* pKlass)
{
  if (pKlass == nullptr)
    return nullptr;

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
  if (image == nullptr)
    return nullptr;

  return
    SK_mono_class_from_name (image, _namespace, _class);
}

static
MonoMethod* GetClassMethod (MonoClass* class_, const char* name, int params)
{
  if (class_ == nullptr)
    return nullptr;

  return
    SK_mono_class_get_method_from_name (class_, name, params);
}

static
MonoMethod* GetClassMethod (MonoImage* image, const char* namespace_, const char* class_, const char* name, int params)
{
  if (image == nullptr)
    return nullptr;

  const auto
      klass  = GetClass (image, namespace_, class_);
  if (klass == nullptr)
    return nullptr;

  return
    GetClassMethod (klass, name, params);
}

static
MonoMethod* GetClassMethod (const char* namespace_, const char* class_, const char* name, int params, const char* image_ = "Assembly-CSharp")
{
  const auto
      image  = GetImage (image_);
  if (image == nullptr)
    return nullptr;

  const auto
      klass  = GetClass (image, namespace_, class_);
  if (klass == nullptr)
    return nullptr;

  return
    GetClassMethod (klass, name, params);
}

static
MonoProperty* GetProperty (MonoImage* image, const char* namespace_, const char* class_, const char* name)
{
  if (image == nullptr)
    return nullptr;

  const auto
      klass  = GetClass (image, namespace_, class_);
  if (klass == nullptr)
    return nullptr;

  return
    SK_mono_class_get_property_from_name (klass, name);
}

static
MonoObject* GetPropertyValue (MonoProperty* property, uintptr_t instance = 0)
{
  if (property == nullptr)
    return nullptr;

  return
    SK_mono_property_get_value (property, reinterpret_cast <void *>(instance), nullptr, 0);
}

static
MonoObject* GetPropertyValue (MonoImage* image, const char* namespace_, const char* class_, const char* name, uintptr_t instance = 0)
{
  if (image == nullptr)
    return nullptr;

  const auto
      property  = GetProperty (image, namespace_, class_, name);
  if (property == nullptr)
           return nullptr;

  return
    GetPropertyValue (property, instance);
}

static
MonoObject* InvokeMethod (const char* namespace_, const char* class_, const char* method_, int paramsCount, void* instance, const char* image_ = "Assembly-CSharp", void** params = nullptr)
{
  if (instance == nullptr)
    return nullptr;

  const auto
      image  = GetImage (image_);
  if (image == nullptr)
        return nullptr;

  const auto
      method  = GetClassMethod (image, namespace_, class_, method_, paramsCount);
  if (method == nullptr)
    return nullptr;

  AttachThread ();

  return
    SK_mono_runtime_invoke (method, instance, params, nullptr);
}

static
void* CompileMethod (MonoMethod* method)
{
  if (method == nullptr)
    return nullptr;

  return
    SK_mono_compile_method (method);
}

static
void* CompileMethod (const char* namespace_, const char* class_, const char* name, int params, const char* image_ = "Assembly-CSharp")
{
  const auto
      image  = GetImage (image_);
  if (image == nullptr)
    return nullptr;

  const auto
      method  = GetClassMethod (image, namespace_, class_, name, params);
  if (method == nullptr)
    return nullptr;

  return
    CompileMethod (method);
}

static
MonoObject* NewObject (MonoClass* klass)
{
  if (klass == nullptr)
    return nullptr;

  SK_mono_thread_attach (SK_Unity_MonoDomain);

  return
    SK_mono_object_new (SK_Unity_MonoDomain, klass);
}

static
MonoObject* Invoke (MonoMethod* method, void* obj, void** params)
{
  if (method == nullptr)
    return nullptr;

  SK_mono_thread_attach (SK_Unity_MonoDomain);
 
  MonoObject* exc;

  return
    SK_mono_runtime_invoke (method, obj, params, &exc);
}

static
MonoObject* ConstructNewObject (MonoClass* klass, int num_args, void** args)
{
  if (klass == nullptr)
    return nullptr;

  auto ctor =
    SK_mono_class_get_method_from_name (klass, ".ctor", num_args);

  if (ctor == nullptr)
    return nullptr;

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
  if (klass == nullptr || instance == nullptr)
    return nullptr;

  auto ctor =
    SK_mono_class_get_method_from_name (klass, ".ctor", num_args);

  if (ctor != nullptr)
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

void
SK_Unity_SetInputPollingFreq (float PollingHz)
{
  return;

  if (std::exchange (SK_Unity_InputPollingFrequency, PollingHz) == PollingHz)
    return;

  AttachThread ();

  auto NativeInputRuntime_instance =
    GetStaticFieldValue ("NativeInputRuntime", "instance", "Unity.InputSystem", "UnityEngine.InputSystem.LowLevel");

  if (NativeInputRuntime_instance != nullptr)
  {
    void* params [1] = { &PollingHz };

    SK_LOGi0 (L"Setting Input Polling Hz: %f", PollingHz);

    InvokeMethod ("UnityEngine.InputSystem.LowLevel", "NativeInputRuntime", "set_pollingFrequency", 1, NativeInputRuntime_instance, "Unity.InputSystem", params);
  }

  // We don't want garbage collection overhead on this thread just because we called a function once!
  DetachCurrentThreadIfNotNative ();
}

void
SK_Unity_SetFixedDeltaTime (float fixed_delta_time)
{
  static float fixed_delta_time_static;
               fixed_delta_time_static = fixed_delta_time;

  if (SK_mono_thread_attach != nullptr)
  {
    SK_Thread_CreateEx ([](LPVOID)->DWORD
    {
      AttachThread ();

      SK_RunOnce (LoadMonoAssembly ("UnityEngine.CoreModule"));
      SK_RunOnce (LoadMonoAssembly ("UnityEngine"));

      static MonoMethod* set_fixedDeltaTime = nullptr;
      static MonoMethod* get_fixedDeltaTime = nullptr;

      SK_RunOnce (
      {
        auto core_module = SK_mono_image_loaded ("UnityEngine.CoreModule");
        if ( core_module == nullptr ) // Older path
             core_module = SK_mono_image_loaded ("UnityEngine");

        if (core_module != nullptr)
        {
          auto
              klass = SK_mono_class_from_name (core_module, "UnityEngine", "Time");
          if (klass != nullptr)
          {
            set_fixedDeltaTime = SK_mono_class_get_method_from_name (klass, "get_fixedDeltaTime", 0);
            get_fixedDeltaTime = SK_mono_class_get_method_from_name (klass, "set_fixedDeltaTime", 1);
          }
        }
      });

      if (set_fixedDeltaTime != nullptr &&
          get_fixedDeltaTime != nullptr)
      {
        if (SK_Unity_OriginalFixedDeltaTime == 0.0f)
        {
          MonoObject* obj =
            SK_mono_runtime_invoke (get_fixedDeltaTime, nullptr, nullptr, nullptr);

          SK_Unity_OriginalFixedDeltaTime = *(float *)SK_mono_object_unbox (obj);

          if (SK_Unity_OriginalFixedDeltaTime == 0.0f)
              SK_Unity_OriginalFixedDeltaTime = 50.0f;
        }

        if (fixed_delta_time_static != 0.0f)
        {
          void* params [1] = { &fixed_delta_time_static };

          SK_mono_runtime_invoke (set_fixedDeltaTime, nullptr, params, nullptr);
        }

        else
        {
          void* params [1] = { &SK_Unity_OriginalFixedDeltaTime };

          SK_mono_runtime_invoke (set_fixedDeltaTime, nullptr, params, nullptr);
        }
      }

      DetachCurrentThreadIfNotNative ();

      SK_Thread_CloseSelf ();

      return 0;
    }, L"[SK] SetFixedDeltaTime_mono");
  }

  else
  {
    SK_Thread_CreateEx ([](LPVOID)->DWORD
    {
      Il2cpp::thread_attach (Il2cpp::get_domain ());

      static il2cpp::Wrapper wrapper;

      static auto image = wrapper.get_image ("UnityEngine.CoreModule.dll");
      static auto klass = image != nullptr ? image->get_class ("Time", "UnityEngine") : nullptr;

      static Method* set_fixedDeltaTime = klass != nullptr ? klass->get_method ("set_fixedDeltaTime", 1) : nullptr;
      static Method* get_fixedDeltaTime = klass != nullptr ? klass->get_method ("get_fixedDeltaTime", 0) : nullptr;

      if (set_fixedDeltaTime != nullptr &&
          get_fixedDeltaTime != nullptr)
      {
        if (SK_Unity_OriginalFixedDeltaTime == 0.0f)
        {
          void* obj = Il2cpp::method_call (get_fixedDeltaTime, nullptr, nullptr, nullptr);

          SK_Unity_OriginalFixedDeltaTime = *(float *)Il2cpp::object_unbox (obj);
        }

        if (fixed_delta_time_static != 0.0f)
        {
          void* params [1] = { &fixed_delta_time_static };

          Il2cpp::method_call (set_fixedDeltaTime, nullptr, params, nullptr);
        }

        else
        {
          void* params [1] = { &SK_Unity_OriginalFixedDeltaTime };

          Il2cpp::method_call (set_fixedDeltaTime, nullptr, params, nullptr);
        }
      }

      Il2cpp::thread_detach (Il2cpp::thread_current ());

      SK_Thread_CloseSelf ();

      return 0;
    }, L"[SK] SetFixedDeltaTime_il2cpp");
  }
}

using InControl_InputDevice_OnAttached_pfn    = void (SK_MONO_API *)(MonoObject*);
using InControl_NativeInputDevice_Vibrate_pfn = void (SK_MONO_API *)(MonoObject*, float leftSpeed, float rightSpeed);
using InControl_NativeInputDevice_Update_pfn  = void (SK_MONO_API *)(MonoObject*, ULONG updateTick, float deltaTime);

static InControl_InputDevice_OnAttached_pfn    InControl_InputDevice_OnAttached_Original    = nullptr;
static InControl_NativeInputDevice_Vibrate_pfn InControl_NativeInputDevice_Vibrate_Original = nullptr;
static InControl_NativeInputDevice_Update_pfn  InControl_NativeInputDevice_Update_Original  = nullptr;

void
SK_Unity_UpdateGlyphOverride (void)
{
  if (SK_Unity_MonoClasses.InControl.InputDevice != nullptr)
  {
    SK_Thread_CreateEx ([](LPVOID)->DWORD
    {
      AttachThread ();

      static auto assemblyInControl =
        SK_Unity_MonoAssemblies.assemblyInControl != nullptr ?
        SK_Unity_MonoAssemblies.assemblyInControl            :
        SK_Unity_MonoAssemblies.assemblyCSharp;

      static MonoClass* DeviceStyle =
        SK_mono_class_from_name (assemblyInControl, "InControl", "InputDeviceStyle");

      if (DeviceStyle != nullptr)
      {
        SK_Unity_CustomizableGlyphs = true;

        MonoClassField* enumValueField =
          SK_mono_class_get_field_from_name (DeviceStyle, SK_Unity_Cfg.gamepad_glyphs_utf8.c_str ());

        if (enumValueField != nullptr)
        {
          MonoObject* enumValueObject =
            SK_mono_field_get_value_object (SK_Unity_MonoDomain, enumValueField, NULL);

          if (enumValueObject != nullptr)
          {
            SK_Unity_GlyphEnumVal =
              *(int32_t *)SK_mono_object_unbox (enumValueObject);
          }

          else
          {
            SK_Unity_Cfg.gamepad_glyphs_utf8 =  "Game Default";
            SK_Unity_Cfg.gamepad_glyphs      = L"Game Default";
            SK_Unity_GlyphEnumVal            =              -1;
          }
        }

        else
        {
          SK_Unity_Cfg.gamepad_glyphs_utf8 =  "Game Default";
          SK_Unity_Cfg.gamepad_glyphs      = L"Game Default";
          SK_Unity_GlyphEnumVal            =              -1;
        }
      }

      SK_Unity_GlyphCacheDirty = true;

      SK_mono_thread_detach (SK_mono_thread_current ());
      SK_Thread_CloseSelf            ();

      return 0;
    }, L"[SK] Mono Temporary Thread");
  }

  else if (SK_Unity_il2cppClasses.InControl.InputDevice != nullptr)
  {
    SK_Thread_CreateEx ([](LPVOID)->DWORD
    {
      Il2cpp::thread_attach (Il2cpp::get_domain ());

      static auto assemblyInControl =
        SK_Unity_il2cppAssemblies.assemblyInControl != nullptr ?
        SK_Unity_il2cppAssemblies.assemblyInControl            :
        SK_Unity_il2cppAssemblies.assemblyCSharp;

      static Class* DeviceStyle =
        assemblyInControl->get_class ("InputDeviceStyle", "InControl");

      if (DeviceStyle != nullptr)
      {
        SK_Unity_CustomizableGlyphs = true;

        Field* enumValueField =
          DeviceStyle->get_field (SK_Unity_Cfg.gamepad_glyphs_utf8.c_str ());

        if (enumValueField != nullptr)
        {
          void* enumValueObject =
            enumValueField->get_object (nullptr);
            //SK_mono_field_get_value_object (SK_Unity_MonoDomain, enumValueField, NULL);

          if (enumValueObject != nullptr)
          {
            SK_Unity_GlyphEnumVal =
              *(int32_t *)Il2cpp::object_unbox (enumValueObject);
          }

          else
          {
            SK_Unity_Cfg.gamepad_glyphs_utf8 =  "Game Default";
            SK_Unity_Cfg.gamepad_glyphs      = L"Game Default";
            SK_Unity_GlyphEnumVal            =              -1;
          }
        }

        else
        {
          SK_Unity_Cfg.gamepad_glyphs_utf8 =  "Game Default";
          SK_Unity_Cfg.gamepad_glyphs      = L"Game Default";
          SK_Unity_GlyphEnumVal            =              -1;
        }
      }

      SK_Unity_GlyphCacheDirty = true;

      Il2cpp::thread_detach (Il2cpp::thread_current ());
      SK_Thread_CloseSelf            ();

      return 0;
    }, L"[SK] il2cpp Temporary Thread");
  }
}

static void SK_Unity_InControl_SetDeviceStyle (MonoObject* device)
{
  if (device == nullptr)
    return;

  if (SK_Unity_GlyphEnumVal == -1)
    return;

  static auto image =
    (SK_Unity_MonoAssemblies.assemblyInControl != nullptr) ?
                                    GetImage ("InControl") :
                              GetImage ("Assembly-CSharp");

  if (image == nullptr)
    return;

  static auto
        method = GetClassMethod (image, "InControl", "InputDevice", "set_DeviceStyle", 1);
  if (! method)
    return;

  void* params [1] = { &SK_Unity_GlyphEnumVal };

  SK_mono_runtime_invoke (method, device, params, nullptr);
}

static void SK_MONO_API InControl_InputDevice_OnAttached_Detour (MonoObject* __this)
{
  SK_LOG_FIRST_CALL

  InControl_InputDevice_OnAttached_Original (__this);

  SK_Unity_InControl_SetDeviceStyle (__this);
}

static void SK_MONO_API InControl_NativeInputDevice_Update_Detour (MonoObject* __this, ULONG updateTick, float deltaTime)
{
  SK_LOG_FIRST_CALL

  if (SK_ImGui_WantGamepadCapture ())
  {
    static MonoMethod* ClearInputState =
      SK_mono_class_get_method_from_name (
        SK_mono_class_get_parent (
          SK_mono_object_get_class (__this)
        ), "ClearInputState", 0);

    if (ClearInputState != nullptr)
    {
      SK_mono_runtime_invoke (ClearInputState, __this, nullptr, nullptr);
    }

#if 0
    if (! config.input.gamepad.disable_rumble)
    {
      InvokeMethod ( "InControl",
                     "NativeInputDevice",
                     "SendStatusUpdates", 0, __this,
        SK_Unity_MonoAssemblies.assemblyInControl != nullptr ?
                                                 "InControl" :
                                           "Assembly-CSharp" );
    }
#endif
    return;
  }

  InControl_NativeInputDevice_Update_Original (__this, updateTick, deltaTime);

  // This should be per-device, but is global; OnAttached (...) will be necessary
  //   to change glyphs on a system with more than one input device active.
  if (std::exchange (SK_Unity_GlyphCacheDirty, false))
    SK_Unity_InControl_SetDeviceStyle (__this);
}

static void SK_MONO_API InControl_NativeInputDevice_Vibrate_Detour (MonoObject* __this, float leftSpeed, float rightSpeed)
{
  SK_LOG_FIRST_CALL

  if (SK_Unity_Cfg.gamepad_fix_playstation)
  {
    if (SK_ImGui_HasPlayStationController ())
        SK_HID_GetActivePlayStationDevice ()->setVibration (
          static_cast <USHORT> (std::clamp (leftSpeed  * static_cast <float> (USHORT_MAX), 0.0f, 65535.0f)),
          static_cast <USHORT> (std::clamp (rightSpeed * static_cast <float> (USHORT_MAX), 0.0f, 65535.0f))
        );
  }

  InControl_NativeInputDevice_Vibrate_Original (__this, leftSpeed, rightSpeed);
}

using InControl_InputDevice_OnAttached_il2cpp_pfn    = void (__fastcall *)(void*);
using InControl_NativeInputDevice_Vibrate_il2cpp_pfn = void (__fastcall *)(void*, float leftSpeed, float rightSpeed);
using InControl_NativeInputDevice_Update_il2cpp_pfn  = void (__fastcall *)(void*, ULONG updateTick, float deltaTime);

static InControl_InputDevice_OnAttached_il2cpp_pfn    InControl_InputDevice_OnAttached_il2cpp_Original    = nullptr;
static InControl_NativeInputDevice_Vibrate_il2cpp_pfn InControl_NativeInputDevice_Vibrate_il2cpp_Original = nullptr;
static InControl_NativeInputDevice_Update_il2cpp_pfn  InControl_NativeInputDevice_Update_il2cpp_Original  = nullptr;

static void SK_Unity_InControl_SetDeviceStyle_il2cpp (void* device)
{
  if (device == nullptr)
    return;

  if (SK_Unity_GlyphEnumVal == -1)
    return;

  static auto image =
    SK_Unity_il2cppAssemblies.assemblyInControl != nullptr ?
    SK_Unity_il2cppAssemblies.assemblyInControl            :
    SK_Unity_il2cppAssemblies.assemblyCSharp;

  if (image == nullptr)
    return;

  static auto
        method = image->get_class ("InputDevice", "InControl")->get_method ("set_DeviceStyle", 1);
  if (! method)
    return;

  void* params [1] = { &SK_Unity_GlyphEnumVal };

  method->invoke (device, params);
}

static void __fastcall InControl_InputDevice_OnAttached_il2cpp_Detour (void* __this)
{
  SK_LOG_FIRST_CALL

  InControl_InputDevice_OnAttached_il2cpp_Original (__this);

  SK_Unity_InControl_SetDeviceStyle_il2cpp (__this);
}

static void __fastcall InControl_NativeInputDevice_Update_il2cpp_Detour (void* __this, ULONG updateTick, float deltaTime)
{
  SK_LOG_FIRST_CALL

  if (SK_ImGui_WantGamepadCapture ())
  {
    static void* ClearInputState =
      Il2cpp::get_method (
        Il2cpp::get_class_get_parent (
          Il2cpp::object_get_class (__this)
        ), "ClearInputState", 0);

    if (ClearInputState != nullptr)
    {
      Il2cpp::method_call ((Method *)ClearInputState, __this, nullptr, nullptr);
    }

#if 0
    if (! config.input.gamepad.disable_rumble)
    {
      InvokeMethod ( "InControl",
                     "NativeInputDevice",
                     "SendStatusUpdates", 0, __this,
        SK_Unity_MonoAssemblies.assemblyInControl != nullptr ?
                                                 "InControl" :
                                           "Assembly-CSharp" );
    }
#endif
    return;
  }

  InControl_NativeInputDevice_Update_il2cpp_Original (__this, updateTick, deltaTime);

  // This should be per-device, but is global; OnAttached (...) will be necessary
  //   to change glyphs on a system with more than one input device active.
  if (std::exchange (SK_Unity_GlyphCacheDirty, false))
    SK_Unity_InControl_SetDeviceStyle_il2cpp (__this);
}

static void __fastcall InControl_NativeInputDevice_Vibrate_il2cpp_Detour (void* __this, float leftSpeed, float rightSpeed)
{
  SK_LOG_FIRST_CALL

  if (SK_Unity_Cfg.gamepad_fix_playstation)
  {
    if (SK_ImGui_HasPlayStationController ())
        SK_HID_GetActivePlayStationDevice ()->setVibration (
          static_cast <USHORT> (std::clamp (leftSpeed  * static_cast <float> (USHORT_MAX), 0.0f, 65535.0f)),
          static_cast <USHORT> (std::clamp (rightSpeed * static_cast <float> (USHORT_MAX), 0.0f, 65535.0f))
        );
  }

  InControl_NativeInputDevice_Vibrate_il2cpp_Original (__this, leftSpeed, rightSpeed);
}


struct {
  float level      = 0.0f;
  float last_level = 0.0f;
  DWORD until      =    0;
} static motors_ [2];

using  VibrationController_Rumble_il2cpp_pfn = void(__fastcall *)(void*,void*,float level, float duration);
static VibrationController_Rumble_il2cpp_pfn
       VibrationController_Rumble_il2cpp_Original = nullptr;

using  Rewired_Joystick_get_supportsVibration_il2cpp_pfn = bool(__fastcall *)(void*);
static Rewired_Joystick_get_supportsVibration_il2cpp_pfn
       Rewired_Joystick_get_supportsVibration_il2cpp_Original = nullptr;

using  Rewired_Joystick_get_vibrationMotorCount_il2cpp_pfn = int(__fastcall *)(void*);
static Rewired_Joystick_get_vibrationMotorCount_il2cpp_pfn
       Rewired_Joystick_get_vibrationMotorCount_il2cpp_Original = nullptr;

using Rewired_Joystick_SetVibration4_il2cpp_pfn  = void(__fastcall *)(void*, float leftMotorLevel, float rightMotorLevel, float leftMotorDuration, float rightMotorDuration);
using Rewired_Joystick_SetVibration2_il2cpp_pfn  = void(__fastcall *)(void*, float leftMotorLevel, float rightMotorLevel);
using Rewired_Joystick_StopVibration_il2cpp_pfn  = void(__fastcall *)(void*);

static Rewired_Joystick_SetVibration4_il2cpp_pfn
       Rewired_Joystick_SetVibration4_il2cpp_Original = nullptr;

static Rewired_Joystick_SetVibration2_il2cpp_pfn
       Rewired_Joystick_SetVibration2_il2cpp_Original = nullptr;

static Rewired_Joystick_StopVibration_il2cpp_pfn
       Rewired_Joystick_StopVibration_il2cpp_Original = nullptr;

void Rewired_Joystick_SetVibration_Impl (float leftMotorLevel, float rightMotorLevel, float leftMotorDuration, float rightMotorDuration)
{
  if (SK_Unity_Cfg.gamepad_fix_playstation)
  {
    SK_RunOnce (
    SK_Thread_CreateEx ([](LPVOID)->DWORD
    {
      while (WaitForSingleObject (__SK_DLL_TeardownEvent, 5) != WAIT_OBJECT_0)
      {
        if (! SK_Unity_Cfg.gamepad_fix_playstation)
          continue;

        auto controller = SK_HID_GetActivePlayStationDevice (false);
        if ( controller != nullptr )
        {
          if (motors_[0].level != motors_[0].last_level || (motors_[0].level != 0.0f && motors_[0].until < SK::ControlPanel::current_time))
          {
            if (motors_[0].until < SK::ControlPanel::current_time && motors_[0].until != 0)
                motors_[0].level = 0.0f;

            controller->setVibration (
              static_cast <USHORT> (std::clamp (motors_[0].level * static_cast <float> (USHORT_MAX), 0.0f, 65535.0f)),
              static_cast <USHORT> (std::clamp (motors_[1].level * static_cast <float> (USHORT_MAX), 0.0f, 65535.0f))
            );

            motors_[0].last_level = motors_[0].level;
          }

          if (motors_[1].level != motors_[1].last_level || (motors_[1].level != 0.0f && motors_[1].until < SK::ControlPanel::current_time))
          {
            if (motors_[1].until < SK::ControlPanel::current_time && motors_[1].until != 0)
                motors_[1].level = 0.0f;

            controller->setVibration (
              static_cast <USHORT> (std::clamp (motors_[0].level * static_cast <float> (USHORT_MAX), 0.0f, 65535.0f)),
              static_cast <USHORT> (std::clamp (motors_[1].level * static_cast <float> (USHORT_MAX), 0.0f, 65535.0f))
            );

            motors_[1].last_level = motors_[1].level;
          }
        }
      }

      SK_Thread_CloseSelf ();

      return 0;
    }, L"[SK] Unity Rumble Thread");
    );
  }

  motors_[0].level = leftMotorLevel;
  motors_[1].level = rightMotorLevel;

  motors_[0].until = leftMotorDuration  > 0.0f ? SK::ControlPanel::current_time + static_cast <DWORD> (1000.0f * leftMotorDuration)  : 0;
  motors_[1].until = rightMotorDuration > 0.0f ? SK::ControlPanel::current_time + static_cast <DWORD> (1000.0f * rightMotorDuration) : 0;
}

static
void
__fastcall
VibrationController_Rumble_il2cpp_Detour (void* __this, void* __player, float level, float duration)
{
  SK_LOG_FIRST_CALL

  Rewired_Joystick_SetVibration_Impl (level, level, duration, duration);

  VibrationController_Rumble_il2cpp_Original (__this, __player, level, duration);
}

static
void
__fastcall
Rewired_Joystick_SetVibration4_il2cpp_Detour (void* __this, float leftMotorLevel, float rightMotorLevel, float leftMotorDuration, float rightMotorDuration)
{
  SK_LOG_FIRST_CALL

  Rewired_Joystick_SetVibration_Impl (leftMotorLevel, rightMotorLevel, leftMotorDuration, rightMotorDuration);

  Rewired_Joystick_SetVibration4_il2cpp_Original (__this, leftMotorLevel, rightMotorLevel, leftMotorDuration, rightMotorDuration);
}

static
void
__fastcall
Rewired_Joystick_SetVibration2_il2cpp_Detour (void* __this, float leftMotorLevel, float rightMotorLevel)
{
  SK_LOG_FIRST_CALL

  Rewired_Joystick_SetVibration_Impl (leftMotorLevel, rightMotorLevel, 0.0f, 0.0f);

  Rewired_Joystick_SetVibration2_il2cpp_Original (__this, leftMotorLevel, rightMotorLevel);
}

static
void
__fastcall
Rewired_Joystick_StopVibration_il2cpp_Detour (void* __this)
{
  SK_LOG_FIRST_CALL

  if (SK_Unity_Cfg.gamepad_fix_playstation)
  {
    motors_[0].level = 0.0f;
    motors_[1].level = 0.0f;
  }

  Rewired_Joystick_StopVibration_il2cpp_Original (__this);
}

static
bool
__fastcall
Rewired_Joystick_get_supportsVibration_il2cpp_Detour (void* __this)
{
  SK_LOG_FIRST_CALL

  if (SK_Unity_Cfg.gamepad_fix_playstation)
  {
    //static MonoClass*      klass = SK_mono_object_get_class (__this);
    //static MonoClassField* field = SK_mono_class_get_field_from_name (klass, "SxXaAJAfPoDKOfgvxNlhugQUrVFq");
    //
    //if (field) {
    //  uint32_t  offset        = SK_mono_field_get_offset (field);
    //  uintptr_t field_address = (uintptr_t)__this + offset;
    //
    //  *(int32_t *)field_address = 2;
    //}

    return true;
  }

  return
    Rewired_Joystick_get_supportsVibration_il2cpp_Original (__this);
}

static
int
__fastcall
Rewired_Joystick_get_vibrationMotorCount_il2cpp_Detour (void* __this)
{
  SK_LOG_FIRST_CALL

  if (SK_Unity_Cfg.gamepad_fix_playstation)
  {
    //static MonoClass*      klass = SK_mono_object_get_class (__this);
    //static MonoClassField* field = SK_mono_class_get_field_from_name (klass, "SxXaAJAfPoDKOfgvxNlhugQUrVFq");
    //
    //if (field) {
    //  uint32_t  offset        = SK_mono_field_get_offset (field);
    //  uintptr_t field_address = (uintptr_t)__this + offset;
    //
    //  *(int32_t *)field_address = 2;
    //}

    return 2;
  }

  return
    Rewired_Joystick_get_vibrationMotorCount_il2cpp_Original (__this);
}

bool
SK_Unity_SetupInputHooks_il2cpp (void)
{
  static auto Aetherim = il2cpp::Wrapper ();
              Aetherim = il2cpp::Wrapper ();

  if (! Aetherim.get_image ("Assembly-CSharp.dll"))
  {
    return false;
  }

  static HANDLE hil2cppInitFinished =
    SK_CreateEvent (nullptr, FALSE, FALSE, nullptr);

  // Create a managed thread to keep assemblies and their JIT'd functions alive.
  SK_RunOnce (
    SK_Thread_CreateEx ([](LPVOID)->DWORD
    {
      Il2cpp::thread_attach (Il2cpp::get_domain ());

      SK_Unity_il2cppAssemblies.assemblyCSharp    = Aetherim.get_image ("Assembly-CSharp.dll");
      SK_Unity_il2cppAssemblies.assemblyInControl = Aetherim.get_image ("InControl.dll");
      SK_Unity_il2cppAssemblies.assemblyRewired   = Aetherim.get_image ("Rewired_Core.dll");

      auto assemblyInControl =
        SK_Unity_il2cppAssemblies.assemblyInControl != nullptr ?
        SK_Unity_il2cppAssemblies.assemblyInControl            :
        SK_Unity_il2cppAssemblies.assemblyCSharp;

      SK_Unity_il2cppClasses.InControl.InputDevice =
                                 assemblyInControl == nullptr ?
                                                      nullptr :
        assemblyInControl->get_class ("InputDevice", "InControl");

      void* pfnInControl_NativeInputDevice_Vibrate = nullptr;
      void* pfnInControl_NativeInputDevice_Update  = nullptr;
      void* pfnInControl_InputDevice_OnAttached    = nullptr;

      if (SK_Unity_il2cppClasses.InControl.InputDevice != nullptr)
      {
        pfnInControl_NativeInputDevice_Vibrate = assemblyInControl->get_class ("NativeInputDevice", "InControl")->get_method ("Vibrate",    2);
        pfnInControl_NativeInputDevice_Update  = assemblyInControl->get_class ("NativeInputDevice", "InControl")->get_method ("Update",     2);
        pfnInControl_InputDevice_OnAttached    = assemblyInControl->get_class (      "InputDevice", "InControl")->get_method ("OnAttached", 0);

        if (pfnInControl_NativeInputDevice_Vibrate != nullptr && *(void**)pfnInControl_NativeInputDevice_Vibrate != nullptr)
        {
          SK_CreateFuncHook (      L"InControl.NativeInputDevice.Vibrate",
                         *(void**)pfnInControl_NativeInputDevice_Vibrate,
                                     InControl_NativeInputDevice_Vibrate_il2cpp_Detour,
            static_cast_p2p <void> (&InControl_NativeInputDevice_Vibrate_il2cpp_Original) );
        }

        if (pfnInControl_NativeInputDevice_Update != nullptr && *(void**)pfnInControl_NativeInputDevice_Update != nullptr)
        {
          SK_CreateFuncHook (      L"InControl.NativeInputDevice.Update",
                         *(void**)pfnInControl_NativeInputDevice_Update,
                                     InControl_NativeInputDevice_Update_il2cpp_Detour,
            static_cast_p2p <void> (&InControl_NativeInputDevice_Update_il2cpp_Original) );
        }

        if (pfnInControl_InputDevice_OnAttached != nullptr && *(void**)pfnInControl_InputDevice_OnAttached != nullptr)
        {
          SK_CreateFuncHook (      L"InControl.InputDevice.OnAttached",
                         *(void**)pfnInControl_InputDevice_OnAttached,
                                     InControl_InputDevice_OnAttached_il2cpp_Detour,
            static_cast_p2p <void> (&InControl_InputDevice_OnAttached_il2cpp_Original) );
        }
      }

      if (SK_Unity_il2cppAssemblies.assemblyRewired != nullptr)
      {
        void* pfnRewired_Joystick_get_supportsVibration   = SK_Unity_il2cppAssemblies.assemblyRewired->get_class ("Joystick", "Rewired")->get_method ("get_supportsVibration",   0);
        void* pfnRewired_Joystick_get_vibrationMotorCount = SK_Unity_il2cppAssemblies.assemblyRewired->get_class ("Joystick", "Rewired")->get_method ("get_vibrationMotorCount", 0);
        void* pfnRewired_Joystick_SetVibration4           = SK_Unity_il2cppAssemblies.assemblyRewired->get_class ("Joystick", "Rewired")->get_method ("SetVibration",            4);
        void* pfnRewired_Joystick_SetVibration2           = SK_Unity_il2cppAssemblies.assemblyRewired->get_class ("Joystick", "Rewired")->get_method ("SetVibration",            2);
        void* pfnRewired_Joystick_StopVibration           = SK_Unity_il2cppAssemblies.assemblyRewired->get_class ("Joystick", "Rewired")->get_method ("StopVibration",           0);
        void* pfnVibrationController_Rumble               = nullptr;

        if (                              SK_Unity_il2cppAssemblies.assemblyCSharp->get_class ("VibrationController", "") != nullptr)
          pfnVibrationController_Rumble = SK_Unity_il2cppAssemblies.assemblyCSharp->get_class ("VibrationController", "")->get_method ("Rumble", 3);

        if (pfnRewired_Joystick_get_supportsVibration != nullptr && *(void**)pfnRewired_Joystick_get_supportsVibration != nullptr)
        {
          SK_CreateFuncHook (      L"Rewired.Joystick.get_supportsVibration",
                         *(void**)pfnRewired_Joystick_get_supportsVibration,
                                     Rewired_Joystick_get_supportsVibration_il2cpp_Detour,
            static_cast_p2p <void> (&Rewired_Joystick_get_supportsVibration_il2cpp_Original) );
        }

        if (pfnVibrationController_Rumble != nullptr && *(void**)pfnVibrationController_Rumble != nullptr)
        {
          SK_CreateFuncHook (      L"VibrationController_Rumble",
                         *(void**)pfnVibrationController_Rumble,
                                     VibrationController_Rumble_il2cpp_Detour,
            static_cast_p2p <void> (&VibrationController_Rumble_il2cpp_Original) );
        }

        else
        {
          if (pfnRewired_Joystick_get_vibrationMotorCount != nullptr && *(void**)pfnRewired_Joystick_get_vibrationMotorCount != nullptr)
          {
            SK_CreateFuncHook (      L"Rewired.Joystick.get_vibrationMotorCount",
                           *(void**)pfnRewired_Joystick_get_vibrationMotorCount,
                                       Rewired_Joystick_get_vibrationMotorCount_il2cpp_Detour,
              static_cast_p2p <void> (&Rewired_Joystick_get_vibrationMotorCount_il2cpp_Original) );
          }

          if (pfnRewired_Joystick_SetVibration4 != nullptr && *(void**)pfnRewired_Joystick_SetVibration4 != nullptr)
          {
            SK_CreateFuncHook (      L"Rewired.Joystick.SetVibration",
                           *(void**)pfnRewired_Joystick_SetVibration4,
                                       Rewired_Joystick_SetVibration4_il2cpp_Detour,
              static_cast_p2p <void> (&Rewired_Joystick_SetVibration4_il2cpp_Original) );
          }

          if (pfnRewired_Joystick_SetVibration2 != nullptr && *(void**)pfnRewired_Joystick_SetVibration2 != nullptr)
          {
            SK_CreateFuncHook (      L"Rewired.Joystick.SetVibration",
                           *(void**)pfnRewired_Joystick_SetVibration2,
                                       Rewired_Joystick_SetVibration2_il2cpp_Detour,
              static_cast_p2p <void> (&Rewired_Joystick_SetVibration2_il2cpp_Original) );
          }
        }

        if (pfnRewired_Joystick_StopVibration != nullptr && *(void **)pfnRewired_Joystick_StopVibration != nullptr)
        {
          SK_CreateFuncHook (      L"Rewired.Joystick.StopVibration",
                         *(void**)pfnRewired_Joystick_StopVibration,
                                     Rewired_Joystick_StopVibration_il2cpp_Detour,
            static_cast_p2p <void> (&Rewired_Joystick_StopVibration_il2cpp_Original) );
        }

        if (pfnRewired_Joystick_get_supportsVibration   != nullptr &&
            pfnRewired_Joystick_StopVibration           != nullptr &&
           (pfnVibrationController_Rumble               != nullptr ||
           (pfnRewired_Joystick_get_vibrationMotorCount != nullptr &&
            pfnRewired_Joystick_SetVibration4           != nullptr &&
            pfnRewired_Joystick_SetVibration2           != nullptr)))
        {
          if (*(void**)pfnRewired_Joystick_get_supportsVibration   != nullptr) SK_QueueEnableHook (*(void**)pfnRewired_Joystick_get_supportsVibration);
          if (*(void**)pfnRewired_Joystick_StopVibration           != nullptr) SK_QueueEnableHook (*(void**)pfnRewired_Joystick_StopVibration);

          if (pfnVibrationController_Rumble)
          {
            if (*(void**)pfnVibrationController_Rumble               != nullptr) SK_QueueEnableHook (*(void**)pfnVibrationController_Rumble);
          }
          else
          {
            if (*(void**)pfnRewired_Joystick_get_vibrationMotorCount != nullptr) SK_QueueEnableHook (*(void**)pfnRewired_Joystick_get_vibrationMotorCount);
            if (*(void**)pfnRewired_Joystick_SetVibration4           != nullptr) SK_QueueEnableHook (*(void**)pfnRewired_Joystick_SetVibration4);
            if (*(void**)pfnRewired_Joystick_SetVibration2           != nullptr) SK_QueueEnableHook (*(void**)pfnRewired_Joystick_SetVibration2);
          }
          

          SK_ApplyQueuedHooks ();

          SK_Unity_FixablePlayStationRumble = true;
        }
      }

      if ( pfnInControl_NativeInputDevice_Vibrate != nullptr &&
           pfnInControl_NativeInputDevice_Update  != nullptr &&
           pfnInControl_InputDevice_OnAttached    != nullptr )
      {
        if (*(void**)pfnInControl_NativeInputDevice_Vibrate != nullptr) SK_QueueEnableHook (*(void**)pfnInControl_NativeInputDevice_Vibrate);
        if (*(void**)pfnInControl_NativeInputDevice_Update  != nullptr) SK_QueueEnableHook (*(void**)pfnInControl_NativeInputDevice_Update);
        if (*(void**)pfnInControl_InputDevice_OnAttached    != nullptr) SK_QueueEnableHook (*(void**)pfnInControl_InputDevice_OnAttached);

        SK_ApplyQueuedHooks ();

        SK_Unity_FixablePlayStationRumble = true;

        SK_Unity_UpdateGlyphOverride ();
      }

      else
      {
        SK_LOGi0 (L"Game does not appear to use InControl for gamepads...");
      }

      SetEvent (hil2cppInitFinished);

      Il2cpp::thread_detach (Il2cpp::thread_current ());
      SK_Thread_CloseSelf            ();

      return 0;
    }, L"[SK] il2cpp Backing Thread");

    WaitForSingleObject (hil2cppInitFinished, 3333);
  );

  return true;
}

using  Rewired_Joystick_get_supportsVibration_pfn = bool(SK_MONO_API *)(MonoObject*);
static Rewired_Joystick_get_supportsVibration_pfn
       Rewired_Joystick_get_supportsVibration_Original = nullptr;

using Rewired_Joystick_SetVibration_pfn  = void(SK_MONO_API *)(MonoObject*, float leftMotorLevel, float rightMotorLevel, float leftMotorDuration, float rightMotorDuration);
using Rewired_Joystick_StopVibration_pfn = void(SK_MONO_API *)(MonoObject*);

static Rewired_Joystick_SetVibration_pfn
       Rewired_Joystick_SetVibration_Original = nullptr;

static Rewired_Joystick_StopVibration_pfn
       Rewired_Joystick_StopVibration_Original = nullptr;

using  VibrationController_Rumble_pfn = void(SK_MONO_API *)(MonoObject*,MonoObject*,float level, float duration);
static VibrationController_Rumble_pfn
       VibrationController_Rumble_Original = nullptr;

static
void
SK_MONO_API
VibrationController_Rumble_Detour (MonoObject* __this, MonoObject* __player, float level, float duration)
{
  SK_LOG_FIRST_CALL

  Rewired_Joystick_SetVibration_Impl (level, level, duration, duration);

  VibrationController_Rumble_Original (__this, __player, level, duration);
}

static
void
SK_MONO_API
Rewired_Joystick_SetVibration_Detour (MonoObject* __this, float leftMotorLevel, float rightMotorLevel, float leftMotorDuration, float rightMotorDuration)
{
  SK_LOG_FIRST_CALL

  Rewired_Joystick_SetVibration_Impl (leftMotorLevel, rightMotorLevel, leftMotorDuration, rightMotorDuration);

  Rewired_Joystick_SetVibration_Original (__this, leftMotorLevel, rightMotorLevel, leftMotorDuration, rightMotorDuration);
}

static
void
SK_MONO_API
Rewired_Joystick_StopVibration_Detour (MonoObject* __this)
{
  SK_LOG_FIRST_CALL

  if (SK_Unity_Cfg.gamepad_fix_playstation)
  {
    motors_[0].level = 0.0f;
    motors_[1].level = 0.0f;
  }

  Rewired_Joystick_StopVibration_Original (__this);
}

static
bool
SK_MONO_API
Rewired_Joystick_get_supportsVibration_Detour (MonoObject* __this)
{
  SK_LOG_FIRST_CALL

  if (SK_Unity_Cfg.gamepad_fix_playstation)
  {
    MonoClass*      klass = SK_mono_object_get_class (__this);
    MonoClassField* field = SK_mono_class_get_field_from_name (klass, "SxXaAJAfPoDKOfgvxNlhugQUrVFq");

    if (field) {
      uint32_t  offset        = SK_mono_field_get_offset (field);
      uintptr_t field_address = (uintptr_t)__this + offset;

      *(int32_t *)field_address = 2;
    }

    field = SK_mono_class_get_field_from_name (klass, "WuMCStPIpMXRuogRDRDpBoDVIzSHA");

    if (field) {
      uint32_t  offset        = SK_mono_field_get_offset (field);
      uintptr_t field_address = (uintptr_t)__this + offset;

      *(int32_t *)field_address = 2;
    }

    return true;
  }

  return
    Rewired_Joystick_get_supportsVibration_Original (__this);
}

using  Rewired_Joystick_get_vibrationMotorCount_pfn = int(SK_MONO_API *)(void*);
static Rewired_Joystick_get_vibrationMotorCount_pfn
       Rewired_Joystick_get_vibrationMotorCount_Original = nullptr;

static
int
SK_MONO_API
Rewired_Joystick_get_vibrationMotorCount_Detour (void* __this)
{
  SK_LOG_FIRST_CALL

  if (SK_Unity_Cfg.gamepad_fix_playstation)
  {
    //static MonoClass*      klass = SK_mono_object_get_class (__this);
    //static MonoClassField* field = SK_mono_class_get_field_from_name (klass, "SxXaAJAfPoDKOfgvxNlhugQUrVFq");
    //
    //if (field) {
    //  uint32_t  offset        = SK_mono_field_get_offset (field);
    //  uintptr_t field_address = (uintptr_t)__this + offset;
    //
    //  *(int32_t *)field_address = 2;
    //}

    return 2;
  }

  return
    Rewired_Joystick_get_vibrationMotorCount_Original (__this);
}

bool
SK_Unity_SetupInputHooks (void)
{
  // Mono not init'd
  if (SK_mono_thread_attach == nullptr)
    return false;

  if (! SK_mono_image_loaded ("Assembly-CSharp"))
  {
    return false;
  }

  static HANDLE hMonoInitFinished =
    SK_CreateEvent (nullptr, FALSE, FALSE, nullptr);

  // Create a managed thread to keep assemblies and their JIT'd functions alive.
  SK_RunOnce (
    SK_Thread_CreateEx ([](LPVOID)->DWORD
    {
      AttachThread ();

      // Optional, may not exist.
      LoadMonoAssembly ("InControl");
      LoadMonoAssembly ("Rewired_Core");
      LoadMonoAssembly ("UnityEngine.CoreModule");

      SK_Unity_MonoAssemblies.assemblyCSharp    = SK_mono_image_loaded ("Assembly-CSharp");
      SK_Unity_MonoAssemblies.assemblyInControl = SK_mono_image_loaded ("InControl");
      SK_Unity_MonoAssemblies.assemblyRewired   = SK_mono_image_loaded ("Rewired_Core");

      auto assemblyInControl =
        SK_Unity_MonoAssemblies.assemblyInControl != nullptr ?
        SK_Unity_MonoAssemblies.assemblyInControl            :
        SK_Unity_MonoAssemblies.assemblyCSharp;

      SK_Unity_MonoClasses.InControl.InputDevice =
                                 assemblyInControl == nullptr ?
                                                      nullptr :
        SK_mono_class_from_name (assemblyInControl, "InControl", "InputDevice");

      void* pfnInControl_NativeInputDevice_Vibrate = nullptr;
      void* pfnInControl_NativeInputDevice_Update  = nullptr;
      void* pfnInControl_InputDevice_OnAttached    = nullptr;

      if (SK_Unity_MonoClasses.InControl.InputDevice != nullptr)
      {
        pfnInControl_NativeInputDevice_Vibrate =
          CompileMethod ("InControl", "NativeInputDevice", "Vibrate",    2);
        pfnInControl_NativeInputDevice_Update =
          CompileMethod ("InControl", "NativeInputDevice", "Update",     2);
        pfnInControl_InputDevice_OnAttached =
          CompileMethod ("InControl",       "InputDevice", "OnAttached", 0);

        // Sometimes this ships as a separate DLL (InControl.dll)
        if (pfnInControl_NativeInputDevice_Vibrate == nullptr)
        {
          pfnInControl_NativeInputDevice_Vibrate =
            CompileMethod ("InControl", "NativeInputDevice", "Vibrate",    2, "InControl");
          pfnInControl_NativeInputDevice_Update =
            CompileMethod ("InControl", "NativeInputDevice", "Update",     2, "InControl");
          pfnInControl_InputDevice_OnAttached =
            CompileMethod ("InControl",       "InputDevice", "OnAttached", 0, "InControl");
        }

        if (pfnInControl_NativeInputDevice_Vibrate != nullptr)
        {
          SK_CreateFuncHook (      L"InControl.NativeInputDevice.Vibrate",
                                  pfnInControl_NativeInputDevice_Vibrate,
                                     InControl_NativeInputDevice_Vibrate_Detour,
            static_cast_p2p <void> (&InControl_NativeInputDevice_Vibrate_Original) );
        }

        if (pfnInControl_NativeInputDevice_Update != nullptr)
        {
          SK_CreateFuncHook (      L"InControl.NativeInputDevice.Update",
                                  pfnInControl_NativeInputDevice_Update,
                                     InControl_NativeInputDevice_Update_Detour,
            static_cast_p2p <void> (&InControl_NativeInputDevice_Update_Original) );
        }

        if (pfnInControl_InputDevice_OnAttached != nullptr)
        {
          SK_CreateFuncHook (      L"InControl.InputDevice.OnAttached",
                                  pfnInControl_InputDevice_OnAttached,
                                     InControl_InputDevice_OnAttached_Detour,
            static_cast_p2p <void> (&InControl_InputDevice_OnAttached_Original) );
        }
      }

      if (SK_Unity_MonoAssemblies.assemblyRewired != nullptr)
      {
        void* pfnVibrationController_Rumble =
          CompileMethod ("", "VibrationController", "Rumble", 3);
        void* pfnRewired_Joystick_get_supportsVibration =
          CompileMethod ("Rewired", "Joystick", "get_supportsVibration",   0, "Rewired_Core");
        void* pfnRewired_Joystick_StopVibration =
          CompileMethod ("Rewired", "Joystick", "StopVibration",           0, "Rewired_Core");

        if (pfnRewired_Joystick_get_supportsVibration != nullptr)
        {
          SK_CreateFuncHook (      L"Rewired.Joystick.get_supportsVibration",
                                  pfnRewired_Joystick_get_supportsVibration,
                                     Rewired_Joystick_get_supportsVibration_Detour,
            static_cast_p2p <void> (&Rewired_Joystick_get_supportsVibration_Original) );
          SK_QueueEnableHook     (pfnRewired_Joystick_get_supportsVibration);

          if (pfnVibrationController_Rumble != nullptr)
          {
            SK_CreateFuncHook (      L"VibrationController.Rumble",
                                    pfnVibrationController_Rumble,
                                       VibrationController_Rumble_Detour,
              static_cast_p2p <void> (&VibrationController_Rumble_Original) );
            SK_QueueEnableHook     (pfnVibrationController_Rumble);
          }

          else
          {
            void* pfnRewired_Joystick_get_vibrationMotorCount =
              CompileMethod ("Rewired", "Joystick", "get_vibrationMotorCount", 0, "Rewired_Core");

            SK_CreateFuncHook (      L"Rewired.Joystick.get_vibrationMotorCount",
                                    pfnRewired_Joystick_get_vibrationMotorCount,
                                       Rewired_Joystick_get_vibrationMotorCount_Detour,
              static_cast_p2p <void> (&Rewired_Joystick_get_vibrationMotorCount_Original) );
            SK_QueueEnableHook     (pfnRewired_Joystick_get_vibrationMotorCount);

            void* pfnRewired_Joystick_SetVibration =
              CompileMethod ("Rewired", "Joystick", "SetVibration", 4, "Rewired_Core");

            SK_CreateFuncHook (      L"Rewired.Joystick.SetVibration",
                                    pfnRewired_Joystick_SetVibration,
                                       Rewired_Joystick_SetVibration_Detour,
              static_cast_p2p <void> (&Rewired_Joystick_SetVibration_Original) );
            SK_QueueEnableHook     (pfnRewired_Joystick_SetVibration);
          }

          SK_CreateFuncHook (      L"Rewired.Joystick.StopVibration",
                                  pfnRewired_Joystick_StopVibration,
                                     Rewired_Joystick_StopVibration_Detour,
            static_cast_p2p <void> (&Rewired_Joystick_StopVibration_Original) );
          SK_QueueEnableHook     (pfnRewired_Joystick_StopVibration);

          SK_ApplyQueuedHooks ();

          SK_Unity_FixablePlayStationRumble = true;
        }
      }

      if ( pfnInControl_NativeInputDevice_Vibrate != nullptr &&
           pfnInControl_NativeInputDevice_Update  != nullptr &&
           pfnInControl_InputDevice_OnAttached    != nullptr )
      {
        SK_QueueEnableHook (pfnInControl_NativeInputDevice_Vibrate);
        SK_QueueEnableHook (pfnInControl_NativeInputDevice_Update);
        SK_QueueEnableHook (pfnInControl_InputDevice_OnAttached);

        SK_ApplyQueuedHooks ();

        SK_Unity_FixablePlayStationRumble = true;

        SK_Unity_UpdateGlyphOverride ();
      }

      else
      {
        SK_LOGi0 (L"Game does not appear to use InControl for gamepads...");
      }

      SetEvent (hMonoInitFinished);

      SK_mono_thread_detach (SK_mono_thread_current ());
      SK_Thread_CloseSelf            ();

      return 0;
    }, L"[SK] Mono Backing Thread");

    WaitForSingleObject (hMonoInitFinished, 3333)
  );

  return true;
}