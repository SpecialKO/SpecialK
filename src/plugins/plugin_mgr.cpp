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
#include <SpecialK/plugin/plugin_mgr.h>
#include <SpecialK/parameter.h>
#include <SpecialK/config.h>
#include <SpecialK/ini.h>
#include <SpecialK/control_panel.h>
#include <SpecialK/render/dxgi/dxgi_backend.h>

SK_LazyGlobal <SK_PluginRegistry> plugin_mgr;

// FIXME: For the love of @#$% do not pass std::wstring objects across
//          DLL boundaries !!
void
__stdcall
SK_SetPluginName (std::wstring name)
{
  plugin_mgr->plugin_name = std::move (name);
  plugin_mgr->isPlugin    = true;
}

void
__stdcall
SKX_SetPluginName (const wchar_t* wszName)
{
  plugin_mgr->plugin_name = wszName;
  plugin_mgr->isPlugin    = true;
}

std::wstring
__stdcall
SK_GetPluginName (void)
{
  if (plugin_mgr->isPlugin)
    return plugin_mgr->plugin_name;

  return L"Special K";
}

bool
__stdcall
SK_HasPlugin (void)
{
  return
    plugin_mgr->isPlugin;
}

const wchar_t*
SK_GetPlugInDirectory ( SK_PlugIn_Type type )
{
  extern const wchar_t*
  __stdcall
  SK_GetDebugSymbolPath (void);

  static std::wstring base_dir = L"",
                      third_party,
                      unofficial;

  if (base_dir.empty ())
  {
    base_dir    = SK_GetDebugSymbolPath ();
    third_party = base_dir + LR"(PlugIns\ThirdParty\)";
    unofficial  = base_dir + LR"(PlugIns\Unofficial\)";
  }

  switch (type)
  {
    case SK_PlugIn_Type::Custom:
      return unofficial.c_str ();
    case SK_PlugIn_Type::ThirdParty:
      return third_party.c_str ();
    default:
    case SK_PlugIn_Type::Unspecified:
      return base_dir.c_str ();
  }
}





#if 0
  static HMODULE hModAuraSDK =
    SK_Modules.LoadLibraryLL (
      SK_FormatStringW ( LR"(%ws\ASUS Aura\AURA_SDK64.dll)",
                         SK_GetPlugInDirectory (SK_PlugIn_Type::ThirdParty) ).c_str ()
    );

  if ( hModAuraSDK != skModuleRegistry::INVALID_MODULE &&
       ImGui::CollapsingHeader ("ASUS Aura") )
  {
    typedef void* AURAHandle_t;
    typedef BYTE  AURAColor_t[3];

    struct AURALight_s
    {
      AURAHandle_t controller;
      AURAColor_t  color;
    };

    struct AURALightController_s
    {
      AURAHandle_t handle;
      DWORD        mode;

      std::vector <AURALight_s> lights;
    };

    static std::vector <AURALightController_s> mb_controllers;
    static std::vector <AURALightController_s> vga_controllers;
    static std::vector <AURALightController_s> ram_controllers;
    static std::vector <AURALightController_s> keyboard_controllers;
    static std::vector <AURALightController_s> mouse_controllers;

      static EnumerateDramFunc SK_AURA_EnumerateDram =
        (EnumerateDramFunc)GetProcAddress (
           hModAuraSDK,
        "EnumerateDram"
      );
      static SetDramModeFunc SK_AURA_SetDramModeFunc =
        (SetDramModeFunc)GetProcAddress (
           hModAuraSDK,
        "SetDramMode"
      );
      static GetDramLedCountFunc SK_AURA_GetDramLedCount =
        (GetDramLedCountFunc)GetProcAddress (
           hModAuraSDK,
        "GetDramLedCount"
      );
      static GetDramColorFunc SK_AURA_GetDramColor =
        (GetDramColorFunc)GetProcAddress (
           hModAuraSDK,
        "GetDramColor"
      );
      static GetDramColorFunc SK_AURA_SetDramColor =
        (SetDramColorFunc)GetProcAddress (
           hModAuraSDK,
        "SetDramColor"
      );

    if (ram_controllers.empty ())
    {
      DWORD dwSizeNeeded =
        SK_AURA_EnumerateDram (nullptr, 0);

      AURAHandle_t *handles =
        new AURAHandle_t [dwSizeNeeded];

      SK_AURA_EnumerateDram (handles, dwSizeNeeded);

      for (int i = 0; i < dwSizeNeeded; i++)
      {
        AURALightController_s controller;
        controller.handle = handles [i];
        controller.mode   = 1;

        int dwLights =
          SK_AURA_GetDramLedCount (controller.handle);

        AURAColor_t* colors =
          new AURAColor_t [dwLights];

        SK_AURA_SetDramColor (handles [i], (BYTE *)colors, dwLights);

        for (int j = 0; j < dwLights; j++)
        {
          AURALight_s light;

          light.controller = handles  [i];

          light.color [0] = colors [j][0];
          light.color [1] = colors [j][1];
          light.color [2] = colors [j][2];
        }

        delete [] colors;
      }

      delete [] handles;
    }

    int  dram_idx = 0;

    for ( auto& controller : ram_controllers )
    {
      bool dram_changed = false;

      ImGui::PushID (dram_idx++);

      int led_idx = 0;

      for ( auto& light : controller.lights )
      {
        ImGui::PushID (led_idx++);

        float fColor [3];

        fColor [0] = (float)((BYTE *)light.color)[0] / 255.0f;
        fColor [1] = (float)((BYTE *)light.color)[1] / 255.0f;
        fColor [2] = (float)((BYTE *)light.color)[2] / 255.0f;

        if (ImGui::ColorEdit3 ("", fColor))
        {
          dram_changed = true;

          light.color [0] = fColor [0] * 255.0f;
          light.color [1] = fColor [1] * 255.0f;
          light.color [2] = fColor [2] * 255.0f;
        }

        ImGui::SameLine ();
        ImGui::PopID    ();
      }

      ImGui::Spacing    ();

      ImGui::PopID ();

      if (dram_changed)
      {
        int idx = 0;

        AURAColor_t *colors =
          new AURAColor_t [controller.lights.size ()];

        for ( auto& light : controller.lights )
        {
          colors [idx  ][0] = light.color [0];
          colors [idx  ][1] = light.color [1];
          colors [idx++][2] = light.color [2];
        }

        SK_AURA_SetDramColor ( controller.handle,
                       (BYTE *)colors,
                               controller.lights.size () );

        delete [] colors;
      }
    }
  }
#endif

sk::iParameter*
_CreateConfigParameter ( std::type_index type,
                         const wchar_t*  wszSection,
                         const wchar_t*  wszKey,
                                  void*  pBackingStore,
                         const wchar_t*  wszDescription,
                         const wchar_t*  wszOldSectionName,
                         const wchar_t*  wszOldKeyName )
{
  enum class _ParameterType
  {
    Bool, Int, Float, StringW
  };

  static const
    std::unordered_map < std::type_index, _ParameterType >
      __type_map =
      {
        { std::type_index (typeid (bool)),         _ParameterType::Bool    },
        { std::type_index (typeid (int)),          _ParameterType::Int     },
        { std::type_index (typeid (float)),        _ParameterType::Float   },
        { std::type_index (typeid (std::wstring)), _ParameterType::StringW },
      };

  static const
    std::unordered_map < std::type_index, std::type_index >
      __type_reflector =
      {
        { std::type_index (typeid (bool)              ),
          std::type_index (typeid (sk::ParameterBool) )   },
        { std::type_index (typeid (int)               ),
          std::type_index (typeid (sk::ParameterInt)  )   },
        { std::type_index (typeid (float)             ),
          std::type_index (typeid (sk::ParameterFloat))   },
        { std::type_index (typeid (std::wstring)      ),
          std::type_index (typeid (sk::ParameterStringW)) }
      };


  sk::iParameter* pParam = nullptr;

  if (      __type_map.count (type))
  {
    const auto
      specialization =
             __type_map.at (type);

    auto
      _TryLoadParam = [&](void) ->
      bool
      {
        if (! ( pBackingStore && pParam ))
          return false;

        switch (specialization)
        {
          case _ParameterType::Bool:
          {
            sk::ParameterBool *pBoolParam =
              dynamic_cast <sk::ParameterBool *> (
                pParam
              );

            if (pBoolParam != nullptr)
            {
              return pBoolParam->load (
                *static_cast <bool *> (pBackingStore)
              );
            }
          } break;

          case _ParameterType::Int:
          {
            sk::ParameterInt *pIntParam =
              dynamic_cast <sk::ParameterInt *> (
                pParam
              );

            if (pIntParam != nullptr)
            {
              return pIntParam->load (
                *static_cast <int *> (pBackingStore)
              );
            }
          } break;

          case _ParameterType::Float:
          {
            sk::ParameterFloat *pFloatParam =
              dynamic_cast <sk::ParameterFloat *> (
                pParam
              );

            if (pFloatParam != nullptr)
            {
              return
                pFloatParam->load (
                  *static_cast <float *> (pBackingStore)
                );
            }
          } break;

          case _ParameterType::StringW:
          {
            sk::ParameterStringW *pStringParam =
              dynamic_cast <sk::ParameterStringW *> (
                pParam
              );

            if (pStringParam != nullptr)
            {
              return
                pStringParam->load (
                  *static_cast <std::wstring *> (pBackingStore)
                );
            }
          } break;
        }

        return false;
      };

    auto
      _StoreParam = [&](void) ->
      void
      {
        if (! pParam)
          return;

        switch (specialization)
        {
          case _ParameterType::Bool:
          {
            auto *pParamBool =
              dynamic_cast <sk::ParameterBool *> (
                pParam
              );

            SK_ReleaseAssert (pParamBool != nullptr);

            if (pParamBool != nullptr)
            {
              pParamBool->store (*static_cast <bool *> (pBackingStore));
            }
          } break;

          case _ParameterType::Int:
          {
            auto *pParamInt =
              dynamic_cast <sk::ParameterInt *> (
                pParam
              );

            SK_ReleaseAssert (pParamInt != nullptr);

            if (pParamInt != nullptr)
            {
              pParamInt->store (*static_cast <int *> (pBackingStore));
            }
          } break;

          case _ParameterType::Float:
          {
            auto *pParamFloat =
              dynamic_cast <sk::ParameterFloat *> (
                pParam
              );

            SK_ReleaseAssert (pParamFloat != nullptr);

            if (pParamFloat != nullptr)
            {
              pParamFloat->store (*static_cast <float *> (pBackingStore));
            }
          } break;

          case _ParameterType::StringW:
          {
            auto *pParamString =
              dynamic_cast <sk::ParameterStringW *> (
                pParam
              );

            SK_ReleaseAssert (pParamString != nullptr);

            if (pParamString != nullptr)
            {
              pParamString->store (
                *static_cast <std::wstring*> (pBackingStore)
              );
            }
          } break;
        }
      };


    switch (specialization)
    {
      case _ParameterType::Bool:
      {
        pParam =
          g_ParameterFactory->create_parameter <bool>  (wszDescription);
      } break;

      case _ParameterType::Int:
      {
        pParam =
          g_ParameterFactory->create_parameter <int>   (wszDescription);
      } break;

      case _ParameterType::Float:
      {
        pParam =
          g_ParameterFactory->create_parameter <float> (wszDescription);
      } break;

      case _ParameterType::StringW:
      {
        pParam =
          g_ParameterFactory->create_parameter <std::wstring> (wszDescription);
      } break;
    }

    if (pParam != nullptr)
    {
      iSK_INI* pINI = nullptr;

      if (             wszOldSectionName != nullptr &&
          (! _wcsicmp (wszOldSectionName, L"Global/osd.ini") ) )
      {
        extern iSK_INI* SK_GetOSDConfig (void);
          pINI = SK_GetOSDConfig ();
      }

      else pINI =
        SK_GetDLLConfig ();

      pParam->register_to_ini (
        pINI, wszSection, wszKey
      );

      if (! _TryLoadParam ())
      {
        if ( wszOldSectionName != nullptr ||
             wszOldKeyName     != nullptr )
        {
          const wchar_t* wszAltSection = ( wszOldSectionName != nullptr ?
                                           wszOldSectionName : wszSection );
          const wchar_t* wszAltKey     = ( wszOldKeyName     != nullptr ?
                                           wszOldKeyName     : wszKey     );

          pParam->register_to_ini (
            pINI, wszAltSection, wszAltKey
          );

          _TryLoadParam ();

          pParam->register_to_ini (
            pINI, wszSection, wszKey
          );
        }
      }

      _StoreParam ();
    }
  }

  return pParam;
}


sk::ParameterFloat*
_CreateConfigParameterFloat ( const wchar_t* wszSection,
                              const wchar_t* wszKey,
                                      float& backingStore,
                              const wchar_t* wszDescription,
                              const wchar_t* wszOldSectionName,
                              const wchar_t* wszOldKeyName )
{
  return
    dynamic_cast <sk::ParameterFloat *> (
      _CreateConfigParameter ( std::type_index (
                                 typeid (float)
                                ),
                                wszSection,        wszKey,
                               &backingStore,      wszDescription,
                                wszOldSectionName, wszOldKeyName )
    );
}

sk::ParameterBool*
_CreateConfigParameterBool ( const wchar_t* wszSection,
                             const wchar_t* wszKey,
                                      bool& backingStore,
                             const wchar_t* wszDescription,
                             const wchar_t* wszOldSectionName,
                             const wchar_t* wszOldKeyName )
{
  return
    dynamic_cast <sk::ParameterBool *> (
      _CreateConfigParameter ( std::type_index (
                                 typeid (bool)
                                ),
                                wszSection,        wszKey,
                               &backingStore,      wszDescription,
                                wszOldSectionName, wszOldKeyName )
    );
}

sk::ParameterInt*
_CreateConfigParameterInt  ( const wchar_t* wszSection,
                             const wchar_t* wszKey,
                                       int& backingStore,
                             const wchar_t* wszDescription,
                             const wchar_t* wszOldSectionName,
                             const wchar_t* wszOldKeyName )
{
  return
    dynamic_cast <sk::ParameterInt *> (
      _CreateConfigParameter ( std::type_index (
                                 typeid (int)
                                ),
                                wszSection,        wszKey,
                               &backingStore,      wszDescription,
                                wszOldSectionName, wszOldKeyName )
    );
}

sk::ParameterStringW*
_CreateConfigParameterStringW ( const wchar_t* wszSection,
                                const wchar_t* wszKey,
                                 std::wstring& backingStore,
                                const wchar_t* wszDescription,
                                const wchar_t* wszOldSectionName,
                                const wchar_t* wszOldKeyName )
{
  return
    dynamic_cast <sk::ParameterStringW *> (
      _CreateConfigParameter ( std::type_index (
                                  typeid (std::wstring)
                                ),
                                wszSection,        wszKey,
                               &backingStore,      wszDescription,
                                wszOldSectionName, wszOldKeyName )
    );
}