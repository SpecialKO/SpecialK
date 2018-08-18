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
#include <SpecialK/plugin/plugin_mgr.h>
#include <string>

//Stupid Hack, rewrite me... (IN PROGRESS - see isPlugin below)
bool isArkhamKnight    = false;
bool isTalesOfZestiria = false;
bool isFallout4        = false;
bool isNieRAutomata    = false;
bool isDarkSouls3      = false;
bool isDivinityOrigSin = false;

bool isPlugin          = false;


std::wstring plugin_name = L"";

// FIXME: For the love of @#$% do not pass std::wstring objects across
//          DLL boundaries !!
void
__stdcall
SK_SetPluginName (std::wstring name)
{
  plugin_name = std::move (name);
  isPlugin    = true;
}

void
__stdcall
SKX_SetPluginName (const wchar_t* wszName)
{
  plugin_name = wszName;
  isPlugin    = true;
}

std::wstring
__stdcall
SK_GetPluginName (void)
{
  if (isPlugin)
    return plugin_name;

  return L"Special K";
}

bool
__stdcall
SK_HasPlugin (void)
{
  return isPlugin;
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