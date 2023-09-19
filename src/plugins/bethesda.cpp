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
#include <string>
#include <json/json.hpp>

bool __SK_HasDLSSGStatusSupport = false;
bool __SK_IsDLSSGActive         = false;
bool __SK_DoubleUpOnReflex      = false;

extern iSK_INI *dll_ini;

static iSK_INI* game_ini       = nullptr;
static iSK_INI* gameCustom_ini = nullptr;

#ifdef _WIN64

static sk::ParameterFloat* sf_1stFOV  = nullptr;
static sk::ParameterFloat* sf_3rdFOV  = nullptr;
static sk::ParameterFloat* sf_MipBias = nullptr;

static float* pf1stFOV  = nullptr;
static float* pf3rdFOV  = nullptr;
static float* pfMipBias = nullptr;

static sk::ParameterBool* __SK_SF_BasicRemastering       = nullptr;
static sk::ParameterBool* __SK_SF_ExtendedRemastering    = nullptr;
static sk::ParameterBool* __SK_SF_HDRRemastering         = nullptr;
static sk::ParameterBool* __SK_SF_PhotoModeCompatibility = nullptr;
static sk::ParameterBool* __SK_SF_DisableFPSLimit        = nullptr;

static sk::ParameterInt64 *__SK_SF_ImageAddr0    = nullptr;
static sk::ParameterInt64 *__SK_SF_BufferDefAddr = nullptr;
static sk::ParameterInt64 *__SK_SF_FPSLimitAddr  = nullptr;

static int64_t pImageAddr0    = -1;
static int64_t pBufferDefAddr = -1;
static int64_t pFPSLimitAddr  = -1;

static uint8_t sf_cOriginalFPSLimitInsts [5] = { 0x0 };
static bool    sf_bDisableFPSLimit           =   false; // Assume DLSS-G mods already do this

static bool sf_bRemasterBasicRTs       = true;
static bool sf_bRemasterExtendedRTs    = false;
static bool sf_bRemasterHDRRTs         = false;
static bool sf_bPhotoModeCompatibility = false;

extern bool __SK_HDR_16BitSwap;
extern bool __SK_HDR_10BitSwap;

enum class BS_DXGI_FORMAT
{
  BS_DXGI_FORMAT_UNKNOWN0  = 0,
  BS_DXGI_FORMAT_R8_UNORM1 = 1,
  BS_DXGI_FORMAT_R8_SNORM2 = 2,
  BS_DXGI_FORMAT_R8_UINT3,
  BS_DXGI_FORMAT_R8_SINT4,
  BS_DXGI_FORMAT_UNKNOWN5,
  BS_DXGI_FORMAT_UNKNOWN6,
  BS_DXGI_FORMAT_B4G4R4A4_UNORM7,
  BS_DXGI_FORMAT_UNKNOWN8,
  BS_DXGI_FORMAT_UNKNOWN9,
  BS_DXGI_FORMAT_B5G6R5_UNORM10,
  BS_DXGI_FORMAT_B5G6R5_UNORM11,
  BS_DXGI_FORMAT_UNKNOWN12,
  BS_DXGI_FORMAT_B5G5R5A1_UNORM13,
  BS_DXGI_FORMAT_R8G8_UNORM14,
  BS_DXGI_FORMAT_R8G8_SNORM15,
  BS_DXGI_FORMAT_UNKNOWN16,
  BS_DXGI_FORMAT_UNKNOWN17,
  BS_DXGI_FORMAT_R8G8_UINT18,
  BS_DXGI_FORMAT_R8G8_SINT19,
  BS_DXGI_FORMAT_UNKNOWN20,
  BS_DXGI_FORMAT_R16_UNORM21,
  BS_DXGI_FORMAT_R16_SNORM22,
  BS_DXGI_FORMAT_R16_UINT23,
  BS_DXGI_FORMAT_R16_SINT24,
  BS_DXGI_FORMAT_R16_FLOAT25,
  BS_DXGI_FORMAT_UNKNOWN26,
  BS_DXGI_FORMAT_UNKNOWN27,
  BS_DXGI_FORMAT_UNKNOWN28,
  BS_DXGI_FORMAT_UNKNOWN29,
  BS_DXGI_FORMAT_UNKNOWN30,
  BS_DXGI_FORMAT_UNKNOWN31,
  BS_DXGI_FORMAT_UNKNOWN32,
  BS_DXGI_FORMAT_UNKNOWN33,
  BS_DXGI_FORMAT_UNKNOWN34,
  BS_DXGI_FORMAT_UNKNOWN35,
  BS_DXGI_FORMAT_R8G8B8A8_UNORM36,
  BS_DXGI_FORMAT_R8G8B8A8_SNORM37,
  BS_DXGI_FORMAT_R8G8B8A8_UINT38,
  BS_DXGI_FORMAT_R8G8B8A8_SINT39,
  BS_DXGI_FORMAT_R8G8B8A8_UNORM_SRGB40,
  BS_DXGI_FORMAT_B8G8R8A8_UNORM41,
  BS_DXGI_FORMAT_UNKNOWN42,
  BS_DXGI_FORMAT_UNKNOWN43,
  BS_DXGI_FORMAT_UNKNOWN44,
  BS_DXGI_FORMAT_B8G8R8A8_UNORM_SRGB45,
  BS_DXGI_FORMAT_UNKNOWN46,
  BS_DXGI_FORMAT_B8G8R8X8_UNORM47,
  BS_DXGI_FORMAT_R16G16_UNORM48,
  BS_DXGI_FORMAT_UNKNOWN49,
  BS_DXGI_FORMAT_R16G16_SNORM50,
  BS_DXGI_FORMAT_UNKNOWN51,
  BS_DXGI_FORMAT_R16G16_UINT52,
  BS_DXGI_FORMAT_R16G16_SINT53,
  BS_DXGI_FORMAT_R16G16_FLOAT54,
  BS_DXGI_FORMAT_R32_UINT55,
  BS_DXGI_FORMAT_R32_SINT56,
  BS_DXGI_FORMAT_R32_FLOAT57,
  BS_DXGI_FORMAT_UNKNOWN58,
  BS_DXGI_FORMAT_UNKNOWN59,
  BS_DXGI_FORMAT_UNKNOWN60,
  BS_DXGI_FORMAT_UNKNOWN61,
  BS_DXGI_FORMAT_R10G10B10A2_UNORM62,
  BS_DXGI_FORMAT_R10G10B10A2_UINT63,
  BS_DXGI_FORMAT_UNKNOWN64,
  BS_DXGI_FORMAT_UNKNOWN65,
  BS_DXGI_FORMAT_R11G11B10_FLOAT66,
  BS_DXGI_FORMAT_R9G9B9E5_SHAREDEXP67,
  BS_DXGI_FORMAT_UNKNOWN68,
  BS_DXGI_FORMAT_UNKNOWN69,
  BS_DXGI_FORMAT_UNKNOWN70,
  BS_DXGI_FORMAT_UNKNOWN71,
  BS_DXGI_FORMAT_UNKNOWN72,
  BS_DXGI_FORMAT_R16G16B16A16_UNORM73,
  BS_DXGI_FORMAT_R16G16B16A16_SNORM74,
  BS_DXGI_FORMAT_R16G16B16A16_UINT75,
  BS_DXGI_FORMAT_R16G16B16A16_SINT76,
  BS_DXGI_FORMAT_R16G16B16A16_FLOAT77,
  BS_DXGI_FORMAT_R32G32_UINT78,
  BS_DXGI_FORMAT_R32G32_SINT79,
  BS_DXGI_FORMAT_R32G32_FLOAT80,
  BS_DXGI_FORMAT_R32G32B32_UINT81,
  BS_DXGI_FORMAT_R32G32B32_SINT82,
  BS_DXGI_FORMAT_R32G32B32_FLOAT83,
  BS_DXGI_FORMAT_R32G32B32A32_UINT84,
  BS_DXGI_FORMAT_R32G32B32A32_SINT85,
  BS_DXGI_FORMAT_R32G32B32A32_FLOAT86,
  BS_DXGI_FORMAT_UNKNOWN87,
  BS_DXGI_FORMAT_UNKNOWN88,
  BS_DXGI_FORMAT_UNKNOWN89,
  BS_DXGI_FORMAT_UNKNOWN90,
  BS_DXGI_FORMAT_UNKNOWN91,
  BS_DXGI_FORMAT_UNKNOWN92,
  BS_DXGI_FORMAT_UNKNOWN93,
  BS_DXGI_FORMAT_UNKNOWN94,
  BS_DXGI_FORMAT_UNKNOWN95,
  BS_DXGI_FORMAT_UNKNOWN96,
  BS_DXGI_FORMAT_UNKNOWN97,
  BS_DXGI_FORMAT_UNKNOWN98,
  BS_DXGI_FORMAT_D16_UNORM99,
  BS_DXGI_FORMAT_D24_UNORM_S8_UINT100,
  BS_DXGI_FORMAT_D32_FLOAT101,
  BS_DXGI_FORMAT_D24_UNORM_S8_UINT102,
  BS_DXGI_FORMAT_D24_UNORM_S8_UINT103,
  BS_DXGI_FORMAT_D32_FLOAT_S8X24_UINT104,
  BS_DXGI_FORMAT_BC1_UNORM105,
  BS_DXGI_FORMAT_BC1_UNORM_SRGB106,
  BS_DXGI_FORMAT_BC1_UNORM107,
  BS_DXGI_FORMAT_BC1_UNORM_SRGB108,
  BS_DXGI_FORMAT_BC2_UNORM109,
  BS_DXGI_FORMAT_BC2_UNORM_SRGB110,
  BS_DXGI_FORMAT_BC3_UNORM111,
  BS_DXGI_FORMAT_BC3_UNORM_SRGB112,
  BS_DXGI_FORMAT_BC4_UNORM113,
  BS_DXGI_FORMAT_BC4_SNORM114,
  BS_DXGI_FORMAT_BC5_UNORM115,
  BS_DXGI_FORMAT_BC5_SNORM116,
  BS_DXGI_FORMAT_BC6H_UF16_117,
  BS_DXGI_FORMAT_BC6H_SF16_118,
  BS_DXGI_FORMAT_BC7_UNORM119,
  BS_DXGI_FORMAT_BC7_UNORM_SRGB120
};

static uintptr_t pBaseAddr = 0;

static uintptr_t CalculateOffset (uintptr_t uAddr)
{
  if (pBaseAddr == 0)
      pBaseAddr = reinterpret_cast <uintptr_t>(SK_Debug_GetImageBaseAddr ());

  return pBaseAddr + uAddr - 0x140000000;
}

void SK_SEH_EnableStarfieldFPSLimit (bool bEnable, void *pAddr)
{
  __try
  {
    DWORD                                              dwOrigProtect = 0x0;
    VirtualProtect (pAddr, 5, PAGE_EXECUTE_READWRITE, &dwOrigProtect);

    if (! bEnable) std::memset (pAddr,                      0x90, 5);
    else           std::memcpy (pAddr, sf_cOriginalFPSLimitInsts, 5);

    VirtualProtect (pAddr, 5, dwOrigProtect,          &dwOrigProtect);
  }

  __except (EXCEPTION_EXECUTE_HANDLER)
  {
  }
}

void SK_SF_EnableFPSLimiter (bool bEnable)
{
  void* pFPSLimit =
    (void *)(pFPSLimitAddr);

  if (pFPSLimit == nullptr)
    return;

  SK_SEH_EnableStarfieldFPSLimit (bEnable, pFPSLimit);
}

enum class DLSSGMode : uint32_t
{
  eOff,
  eOn,
  eCount
};

enum class DLSSMode : uint32_t
{
  eOff,
  eMaxPerformance,
  eBalanced,
  eMaxQuality,
  eUltraPerformance,
//eUltraQuality,
  eDLAA,
  eCount,
};

enum class DLSSPreset : uint32_t
{
  //! Default behavior, may or may not change after an OTA
  eDefault,
  //! Fixed DL models
  ePresetA,
  ePresetB,
  ePresetC,
  ePresetD,
  ePresetE,
  ePresetF,
  ePresetG,
};

NLOHMANN_JSON_SERIALIZE_ENUM (
   DLSSMode, {
 { DLSSMode::eOff,              "Off"              },
 { DLSSMode::eMaxPerformance,   "MaxPerformance"   },
 { DLSSMode::eBalanced,         "Balanced"         },
 { DLSSMode::eMaxQuality,       "MaxQuality"       },
 { DLSSMode::eUltraPerformance, "UltraPerformance" },
 { DLSSMode::eDLAA,             "DLAA"             },
 { DLSSMode::eDLAA,             "UltraQuality"     }, // Does not exist, just alias to DLAA
             }               );

NLOHMANN_JSON_SERIALIZE_ENUM (
  DLSSPreset, {
 { DLSSPreset::eDefault, "Default" },
 { DLSSPreset::ePresetA, "A"       },
 { DLSSPreset::ePresetB, "B"       },
 { DLSSPreset::ePresetC, "C"       },
 { DLSSPreset::ePresetD, "D"       },
 { DLSSPreset::ePresetE, "E"       },
 { DLSSPreset::ePresetF, "F"       },
 { DLSSPreset::ePresetG, "G"       },
 		          }             );

#define VERSION_DLSSGINFO_1 1
#define VERSION_DLSSGINFO_MAX VERSION_DLSSGINFO_1

struct DLSSGInfo
{
  uint32_t  version                    = VERSION_DLSSGINFO_MAX;

#pragma region Version 1
	uint32_t  numFramesActuallyPresented = { };
	DLSSGMode currentMode                = DLSSGMode::eCount;
	bool      dlssgSupported             = { };
	bool      dlssgEnabled               = { };
#pragma endregion

};

enum ConfigOption
{
	OPTION_ENABLED = 0,
	OPTION_REFLEX_SLEEP,
	OPTION_FRAME_GENERATION_ENABLED,
	OPTION_DLSS_MODE,
	OPTION_REFLEX_MODE,
	OPTION_REFLEX_FPS_CAP,
	OPTION_NIS_SHARPENING,
	OPTION_DLSS_PRESET,
	OPTION_DYNAMIC_RESOLUTION,
	OPTION_STREAMLINE_OTA,
	OPTION_CRASH_DELAY_WORKAROUND,
	OPTION_COUNT
};

enum ConfigValueType
{
	OPTION_TYPE_BOOL = 0,
	OPTION_TYPE_UINT,
	OPTION_TYPE_COUNT
};

#define VERSION_CONFIGVALUE_1 1
#define VERSION_CONFIGVALUE_MAX VERSION_CONFIGVALUE_1

struct ConfigValue
{
	uint32_t        version = VERSION_CONFIGVALUE_1;
#pragma region Version 1
  ConfigValueType type;
  ConfigOption    option;
  void* const     value;
#pragma endregion
};

#define VERSION_RESOLUTIONINFO_1 1
#define VERSION_RESOLUTIONINFO_MAX VERSION_RESOLUTIONINFO_1

	struct Resolution
	{
		uint32_t width;
		uint32_t height;
	};

	struct ResolutionInfo
	{
		uint32_t version = VERSION_RESOLUTIONINFO_1;
#pragma region Version 1
		Resolution optimal;
		Resolution min;
		Resolution max;
		Resolution current;
#pragma endregion
	};


typedef void (__stdcall* PFN_RESOLUTION_ERROR_CALLBACK)(ResolutionInfo* info, void* data);

using  f2sRegisterResolutionErrorCallback_pfn = bool (*)(PFN_RESOLUTION_ERROR_CALLBACK callback, void* data, uint32_t* outCallbackId);
static f2sRegisterResolutionErrorCallback_pfn
       f2sRegisterResolutionErrorCallback = nullptr;

using  f2sGetDLSSGInfo_pfn = bool (*)(DLSSGInfo *info);
static f2sGetDLSSGInfo_pfn
       f2sGetDLSSGInfo = nullptr;

using  f2sLoadConfig_pfn = bool (*)(const char *config);
static f2sLoadConfig_pfn
       f2sLoadConfig = nullptr;

using  f2sSetConfigValue_pfn = bool (*)(const ConfigValue* value);
using  f2sGetConfigValue_pfn = bool (*)(const ConfigValue* value);

static f2sSetConfigValue_pfn
       f2sSetConfigValue = nullptr;
static f2sGetConfigValue_pfn
       f2sGetConfigValue = nullptr;

static DLSSGInfo dlssg_state;

bool SK_SF_PlugInCfg (void)
{
  static std::string  utf8VersionString =
    SK_WideCharToUTF8 (SK_GetDLLVersionStr (SK_GetHostApp ()));

  if (ImGui::CollapsingHeader (utf8VersionString.data (), ImGuiTreeNodeFlags_DefaultOpen))
  {
    ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.40f, 0.40f, 0.45f));
    ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.45f, 0.45f, 0.80f));
    ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.53f, 0.53f, 0.80f));
    ImGui::TreePush       ("");

    if (ImGui::CollapsingHeader ("Render Quality", ImGuiTreeNodeFlags_DefaultOpen))
    {
      bool changed                   = false;
      bool changed_no_restart_needed = false;

      ImGui::TreePush ("");

      ImGui::BeginGroup ();
      changed |= ImGui::Checkbox ("Upgrade Base 8-bpc RTs to 16-bpc",       &sf_bRemasterBasicRTs);

      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("Eliminates banding on UI at the cost of (negligible) extra VRAM");

      if (sf_bRemasterBasicRTs)
      {
        changed |= ImGui::Checkbox ("PhotoMode Compatibility", &sf_bPhotoModeCompatibility);

        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("Reduce dynamic range to 8-bpc on Image Space Buffers so that Photo Mode does not crash.");
      }

      ImGui::EndGroup   ();
      ImGui::SameLine   ();
      ImGui::BeginGroup ();

      changed |= ImGui::Checkbox ("Upgrade Most 8-bpc RTs to 16-bpc", &sf_bRemasterExtendedRTs);

      if (ImGui::IsItemHovered ())
      {
        ImGui::SetTooltip ("May further reduce banding and improve HDR, but at high memory cost");
      }

      changed |= ImGui::Checkbox ("Upgrade HDR (11-bpc) RTs to 16-bpc", &sf_bRemasterHDRRTs);

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip ();
        ImGui::Text         ("Improves quality of HDR effects such as Atmospheric Bloom");
        ImGui::Separator    ();
        ImGui::BulletText   ("Unlikely to reduce banding, but will produce richer color than 11-bpc");
        ImGui::BulletText   ("The format replaced uses 11-bit Red, 11-bit Green, 10-bit Blue and has trouble with pure white");
        ImGui::EndTooltip   ();
      }

      ImGui::EndGroup ();

      if (pfMipBias != nullptr)
      {
        changed_no_restart_needed |=
          ImGui::SliderFloat ("Mipmap Bias", pfMipBias, -2.f, 2.f, "%.1f");

        if (ImGui::IsItemHovered ())
        {
          ImGui::SetTooltip ("A negative bias is potentially required when using FSR/DLSS");
        }
      }

      ImGui::TreePop ();

      static bool restart_needed = false;

      if (changed || changed_no_restart_needed)
      {
        if (changed)
          restart_needed = true;

        __SK_SF_BasicRemastering->store       (sf_bRemasterBasicRTs);
        __SK_SF_ExtendedRemastering->store    (sf_bRemasterExtendedRTs);
        __SK_SF_HDRRemastering->store         (sf_bRemasterHDRRTs);
        __SK_SF_PhotoModeCompatibility->store (sf_bPhotoModeCompatibility);

        if (pfMipBias != nullptr)
        {
           sf_MipBias->store (*pfMipBias);
           gameCustom_ini->write ();
        }

        dll_ini->write ();
      }

      if (restart_needed)
      {
        ImGui::PushStyleColor (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (.3f, .8f, .9f));
        ImGui::BulletText     ("Game Restart Required");
        ImGui::PopStyleColor  ();
      }
    }

    if (pf1stFOV != nullptr)
    {
      bool changed = false;

      if (ImGui::CollapsingHeader ("Field of View"))
      {
        ImGui::TreePush ("");

        changed |= ImGui::SliderFloat ("1st Person FOV", pf1stFOV, 1, 120, "%.0f");
        changed |= ImGui::SliderFloat ("3rd Person FOV", pf3rdFOV, 1, 120, "%.0f");

        ImGui::TreePop ();
      }

      if (changed)
      {
        sf_1stFOV->store (*pf1stFOV);
        sf_3rdFOV->store (*pf3rdFOV);

        gameCustom_ini->write ();
      }
    }

    if (pFPSLimitAddr > 0)
    {
      if (ImGui::CollapsingHeader ("Performance"))
      {
        ImGui::TreePush ("");

        if (ImGui::Checkbox ("Disable In-Game Framerate Limit", &sf_bDisableFPSLimit))
        {
          SK_SF_EnableFPSLimiter (! sf_bDisableFPSLimit);

          __SK_SF_DisableFPSLimit->store (sf_bDisableFPSLimit);

          dll_ini->write ();
        }

        ImGui::TreePop ();
      }
    }

    if (f2sGetDLSSGInfo != nullptr && ImGui::CollapsingHeader ( dlssg_state.dlssgSupported ? "FSR2Streamline DLSS-G"
                                                                                           : "FSR2Streamline DLSS", ImGuiTreeNodeFlags_DefaultOpen ))
    {
      ImGui::TreePush ("");

      if (f2sGetDLSSGInfo (&dlssg_state))
      {
        ImGui::TextUnformatted ("DLSS-G State: ");
        ImGui::SameLine        ();

        if (! dlssg_state.dlssgSupported)
          ImGui::TextColored             (ImVec4 (.666f, .666f, 0.f, 1.f), "Unsupported");
        else
        {
          if (      dlssg_state.dlssgEnabled)
            switch (dlssg_state.currentMode)
            {
              case DLSSGMode::eOn:
                ImGui::TextColored       (ImVec4 (0.f, 1.f, 0.f, 1.f), "Active");
                ImGui::SameLine          ();
                ImGui::VerticalSeparator ();
                ImGui::SameLine          ();
                ImGui::Text              ("Frame Generation Rate: %dx", dlssg_state.numFramesActuallyPresented);
                break;
              case DLSSGMode::eOff:
                ImGui::TextColored       (ImVec4 (1.f, 1.f, 0.f, 1.f), "Inactive");
                break;
              case DLSSGMode::eCount:
              default:
                ImGui::TextColored       (ImVec4 (1.f, 0.f, 0.f, 1.f), "Unknown?!");
                break;
            }
          else  ImGui::TextColored       (ImColor::HSV (0.1f, 1.f, 1.f), "Disabled by User");
        }
      } else    ImGui::Text              ("Unknown Malfunction");

      static nlohmann::json f2s_config;

                                       std::error_code                                                  ec;
      static bool    new_config_file = std::filesystem::is_regular_file (L"FSR2Streamline_config.json", ec);
      static bool    old_config_file = std::filesystem::is_regular_file (L"config.json",                ec);
      static const char* config_file = new_config_file ? "FSR2Streamline_config.json" :
                                       old_config_file ? "config.json"                : nullptr;

      struct {
        DLSSMode   mode               = DLSSMode::eOff;
        DLSSPreset preset             = DLSSPreset::eDefault;
        bool       enable_frame_gen   = false;
        bool       enable_nis         = false;
        bool       enable_ota_updates = false;
        bool       enable_dynamic_res = false;
      } static dlss_prefs;

      if (config_file != nullptr)
      {
        static bool   config_file_dirty = true;
        static HANDLE config_file_watch = nullptr;

        SK_RunOnce (
        {
          std::filesystem::path config_path =
            std::filesystem::current_path (ec);

          config_file_watch =
            FindFirstChangeNotificationW (config_path.c_str (), FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE);
        });

        if (                     config_file_watch     != 0 &&
            WaitForSingleObject (config_file_watch, 0) != WAIT_TIMEOUT)
        {
                                      config_file_dirty = true;
          FindNextChangeNotification (config_file_watch);
        }

        if (config_file_dirty)
        {
          try {
            std::ifstream config_json (config_file);

            f2s_config =
              nlohmann::json::parse (config_json);

            f2s_config ["dlssMode"].get_to   (dlss_prefs.mode);
            f2s_config ["dlssPreset"].get_to (dlss_prefs.preset);

            if (! f2s_config ["enabled"])
              dlss_prefs.mode = DLSSMode::eOff;

            f2s_config ["enableNisSharpening"].get_to      <bool> (dlss_prefs.enable_nis);
            f2s_config ["enableFrameGeneration"].get_to    <bool> (dlss_prefs.enable_frame_gen);
            f2s_config ["enableStreamlineOta"].get_to      <bool> (dlss_prefs.enable_ota_updates);
            f2s_config ["dynamicResolutionEnabled"].get_to <bool> (dlss_prefs.enable_dynamic_res);

            config_file_dirty = false;
          }
        
          catch (const std::exception& e)
          {
            if (config.system.log_level > 0)
            {
#ifdef __CPP20
              const auto&& src_loc =
                std::source_location::current ();
              
              steam_log->Log ( L"%hs (%d;%d): json parse failure: %hs",
                                           src_loc.file_name     (),
                                           src_loc.line          (),
                                           src_loc.column        (), e.what ());
              steam_log->Log (L"%hs",      src_loc.function_name ());
              steam_log->Log (L"%hs",
                std::format ( std::string ("{:*>") +
                           std::to_string (src_loc.column        ()), 'x').c_str ());
#else
              std::ignore = e;
#endif
            }
          }
        }
      }

      try
      {
        bool changed = false;

        ImGui::BeginGroup ();

        static bool restart_required = false;
        if (! dlssg_state.dlssgSupported)
        {
          changed |=
            ImGui::Checkbox ("DLSS Frame Generation Support", &dlss_prefs.enable_frame_gen);

          if (ImGui::IsItemHovered ())
            ImGui::SetTooltip ("This will not toggle DLSS-G, in the current release use it to enable DLSS-G support pending a game restart.");
        }

        if (changed)
          restart_required = true;

        if (dlssg_state.dlssgSupported && f2sSetConfigValue != nullptr)
        {
          if (ImGui::Checkbox ("Use DLSS Frame Generation", &dlssg_state.dlssgEnabled))
          {
            ConfigValue cfg_val = { .version = VERSION_CONFIGVALUE_MAX,
                                       .type = OPTION_TYPE_BOOL,
                                     .option = OPTION_FRAME_GENERATION_ENABLED,
                                      .value = &dlssg_state.dlssgEnabled };

            f2sSetConfigValue (&cfg_val);
          }

          if (dlssg_state.dlssgEnabled)
          {
            ImGui::Checkbox ("Enable SK + Reflex Limiter", &__SK_DoubleUpOnReflex);
          }
        }

        changed |=
          ImGui::Combo ("DLSS Mode", (int *)&dlss_prefs.mode, "Off\0"
                                                              "Max Performance\t(50% Scale)\0"
                                                              "Balanced\t\t\t\t  (58% Scale)\0"
                                                              "Max Quality\t\t\t  (66% Scale)\0"
                                                              "Ultra Performance\t(12% Scale)\0"
                                                              "DLAA\t\t\t\t\t\t(100% Scale)\0\0");

        changed |=
          ImGui::Combo ("DLSS Preset", (int *)&dlss_prefs.preset, "Default\0"
                                                                  "A\0"
                                                                  "B\0"
                                                                  "C\0"
                                                                  "D\0"
                                                                  "E\0"
                                                                  "F\0"
                                                                  "G\0\0");
        ImGui::EndGroup   ();
        ImGui::SameLine   ();
        ImGui::BeginGroup ();
        changed |=
          ImGui::Checkbox ("Dynamic Resolution", &dlss_prefs.enable_dynamic_res);

        changed |=
          ImGui::Checkbox ("NIS Sharpening", &dlss_prefs.enable_nis);

        changed |=
          ImGui::Checkbox ("DLSS OTA Updates", &dlss_prefs.enable_ota_updates);
        ImGui::EndGroup   ();

        if (restart_required)
          ImGui::BulletText ("Game Restart Required");

        if (changed)
        {
          f2s_config ["enabled"]                  = dlss_prefs.mode != DLSSMode::eOff;
          if (dlss_prefs.mode == DLSSMode::eOff)
              dlss_prefs.mode = DLSSMode::eBalanced; // Set a safe default value to prevent crashing
          f2s_config ["dlssMode"]                 = dlss_prefs.mode;
          f2s_config ["dlssPreset"]               = dlss_prefs.preset;
          f2s_config ["enableFrameGeneration"]    = dlss_prefs.enable_frame_gen;
          f2s_config ["dynamicResolutionEnabled"] = dlss_prefs.enable_dynamic_res;
          f2s_config ["enableStreamlineOta"]      = dlss_prefs.enable_ota_updates;
          f2s_config ["enableNisSharpening"]      = dlss_prefs.enable_nis;

          // Write the config manually if this doesn't exist
          if ( nullptr == f2sLoadConfig ||
               false   == f2sLoadConfig (f2s_config.dump ().c_str ()) )
          {
            if (std::ofstream of (config_file); of)
              of << std::setw (4) << f2s_config;
          }
        }
      }

      catch (const std::exception &e)
      {
        std::ignore = e;
      }

      ImGui::TreePop  (  );
    }

    ImGui::Separator     ( );
    ImGui::PopStyleColor (3);
    ImGui::TreePop       ( );
  }

  return true;
}

void SK_SEH_InitStarfieldFPS (void)
{
  // Assume that DLSS-G mods are going to turn the framerate limiter off always
  if (SK_GetModuleHandle (L"sl.interposer.dll"))
    return;

  __try
  {
    void *scan = nullptr;

    static const char *szFPSLimitPattern =
      "\xC6\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x8D\x00\x00\xE8\x00\x00\x00\x00\x33\x00\xE8";
    static const char *szFPSLimitMask    =
      "\xFF\x00\x00\x00\xFF\x00\x00\x00\x00\xFF\xFF\x00\x00\xFF\x00\x00\x00\x00\xFF\x00\xFF";

    // Try previously cached address first
    if (pFPSLimitAddr > 0)
    {
      scan =
        SK_ScanAlignedEx (
          szFPSLimitPattern, 21,
          szFPSLimitMask, (void *)((uintptr_t)pFPSLimitAddr - 0xD - 1)
        );
    }

    if (scan == nullptr)
    {
      scan =
        SK_ScanAlignedEx (
          szFPSLimitPattern, 21,
          szFPSLimitMask );
    }

    SK_LOGs0 (L"Starfield ", L"Scanned FPS Limit Address: %p", scan);

    if (scan != nullptr)
    {
      pFPSLimitAddr = (uintptr_t)scan + 0xD;

      memcpy (sf_cOriginalFPSLimitInsts, (void *)(pFPSLimitAddr), 5);

      SK_SF_EnableFPSLimiter (! sf_bDisableFPSLimit);

      return;
    }
  }

  __except (EXCEPTION_EXECUTE_HANDLER)
  {
  }

  pFPSLimitAddr = 0;
}

void SK_SEH_InitStarfieldRTs (void)
{
  __try
  {
    if ( sf_bRemasterBasicRTs || sf_bRemasterExtendedRTs ||
         sf_bRemasterHDRRTs   ||      __SK_HDR_16BitSwap || __SK_HDR_10BitSwap )
    {
      void *scan = nullptr;

      // Try previously cached address first
      if (pImageAddr0 > 0)
      {
        scan =
          SK_ScanAlignedEx ( "\x44\x8B\x05\x00\x00\x00\x00\x89\x55\xFB", 10,
                             "\xFF\xFF\xFF\x00\x00\x00\x00\xFF\xFF\xFF", (void *)((uintptr_t)pImageAddr0 - 1) );
      }

      if (scan == nullptr)
      {
        scan =
          SK_ScanAlignedEx ( "\x44\x8B\x05\x00\x00\x00\x00\x89\x55\xFB", 10,
                             "\xFF\xFF\xFF\x00\x00\x00\x00\xFF\xFF\xFF", nullptr, 8 );
      }

      SK_LOGs0 (L"Starfield ", L"Scanned Image Address:     %p", scan);

      if (sf_bRemasterBasicRTs)
      {
        const auto offset                      = *reinterpret_cast < int32_t *>((uintptr_t)scan + 3);
        uint32_t  *imageSpaceBufferPtr         =  reinterpret_cast <uint32_t *>((uintptr_t)scan + 7 + offset);            // 5079A70
        uint32_t  *scaleformCompositeBufferPtr =  reinterpret_cast <uint32_t *>((uintptr_t)imageSpaceBufferPtr + 0x280);  // 5079CF0

        if (! sf_bPhotoModeCompatibility)
          *imageSpaceBufferPtr       = 77;

        *scaleformCompositeBufferPtr = 77;
      }

      if (scan != nullptr)
        pImageAddr0 = (int64_t)scan;

      if ( sf_bRemasterExtendedRTs || sf_bRemasterHDRRTs ||
                __SK_HDR_16BitSwap || __SK_HDR_10BitSwap )
      {
        scan = nullptr;

        // Try previously cached address first
        if (pBufferDefAddr > 0)
        {
          scan =
            SK_ScanAlignedEx ( "\x4C\x8D\x15\x00\x00\x00\x00\xBE\x00\x00\x00\x00", 12,
                               "\xFF\xFF\xFF\x00\x00\x00\x00\xFF\x00\x00\x00\x00", (void *)((uintptr_t)pBufferDefAddr - 1) );
        }

        if (scan == nullptr)
        {
          scan =
            SK_ScanAlignedEx ( "\x4C\x8D\x15\x00\x00\x00\x00\xBE\x00\x00\x00\x00", 12,
                               "\xFF\xFF\xFF\x00\x00\x00\x00\xFF\x00\x00\x00\x00", (void *)pImageAddr0 );
        }

        SK_LOGs0 (L"Starfield ", L"Scanned Buffer Array:      %p", scan);

        if (scan != nullptr)
          pBufferDefAddr = (int64_t)scan;

			  const auto offset  = *reinterpret_cast < int32_t *>((uintptr_t)scan  + 3);
			  const auto address =  reinterpret_cast < uintptr_t>(           scan) + 7 + offset;

        struct BufferDefinition
        {
          BS_DXGI_FORMAT format;
          uint32_t       unk04;
          const char*    bufferName;
          uint16_t       unk10;
          uint16_t       unk12;
          uint32_t       unk14;
          uint16_t       unk18;
          uint16_t       unk1A;
          uint32_t       unk1C;
          uint16_t       unk20;
          uint16_t       unk22;
          uint32_t       unk24;
          uint16_t       unk28;
          uint16_t       unk2A;
          uint32_t       unk2C;
          uint32_t       unk30;
          float          unk34;
          float          unk38;
          float          unk3C;
          uint32_t       unk40;
          uint32_t       unk44;
          uint32_t       unk48;
          uint32_t       unk4C;
        };

        BufferDefinition **buffer_defs =
          reinterpret_cast <BufferDefinition **>(address);  // 4718E40

        const char *buffers_to_remaster [] =
        {
          //"ImageSpaceBuffer",

          "NativeResolutionColorBuffer01",
          "HDRImagespaceBuffer",
          "ImageSpaceBufferR10G10B10A2",
          "ImageSpaceBufferB10G11R11",
          "ImageSpaceBufferE5B9G9R9",
          "GBuffer_Normal_EmissiveIntensity",
          "SF_ColorBuffer",

          "TAA_idTech7HistoryColorTarget",

          "EnvBRDF",

          "GBuffer_AlbedoMisc",
          "GBuffer_AO_Rough_Metal",
          "GBuffer_Optional",
          "LightingBufferUV",
          "SAORawAO",
          "DownsampleOutputPrevFrame",
          "DownsampleOutput",
          "SobelOutput",
          "SpaceGlareBlur",
          "SeparableSSSBufferUV",

          "ThinGBuffer_Albedo",
          "ThinGBuffer_Optional",
          "ThinGBuffer_AlbedoArray",
          "ThinGBuffer_OptionalArray",
          "SkyCubemapThinGBuffer_Albedo",
          "SkyCubemapThinGBuffer_Optional",
          "CelestialBodyThinGBuffer_Albedo",
          "CelestialBodyThinGBuffer_Optional",
          "EpipolarExtinction",
          "ImageProcessColorTarget"
        };

        for (UINT i = 0 ; i < 200 ; ++i)
        {
          __try
          {
            if (0 == strcmp (buffer_defs [i]->bufferName, "FrameBuffer"))
            {
              if (__SK_HDR_16BitSwap) buffer_defs [i]->format = BS_DXGI_FORMAT::BS_DXGI_FORMAT_R16G16B16A16_FLOAT77;
              if (__SK_HDR_10BitSwap) buffer_defs [i]->format = BS_DXGI_FORMAT::BS_DXGI_FORMAT_R10G10B10A2_UNORM62;

              continue;
            }

            if (sf_bRemasterHDRRTs)
            {
              if (buffer_defs [i]->format == BS_DXGI_FORMAT::BS_DXGI_FORMAT_R11G11B10_FLOAT66)
              {   buffer_defs [i]->format  = BS_DXGI_FORMAT::BS_DXGI_FORMAT_R16G16B16A16_FLOAT77;
              
                SK_LOGs0 (L"Starfield ", L"Remastered Buffer: %36hs (%3d) using FP16", buffer_defs [i]->bufferName, i);
                continue;
              }
            }

            if (! sf_bRemasterExtendedRTs)
              continue;

            if (0 == strcmp (buffer_defs [i]->bufferName, "FSR2_RESAMPLED_LUMA_HISTORY"))
            {
              buffer_defs [i]->format = BS_DXGI_FORMAT::BS_DXGI_FORMAT_R16G16B16A16_FLOAT77;

              SK_LOGs0 (L"Starfield ", L"Remastered Buffer: %36hs (%3d) using FP16", buffer_defs [i]->bufferName, i);
            }

            for (auto remaster : buffers_to_remaster)
            {
              if (0 == strcmp (buffer_defs [i]->bufferName, remaster))
              {
                buffer_defs [i]->format = BS_DXGI_FORMAT::BS_DXGI_FORMAT_R16G16B16A16_FLOAT77;

                SK_LOGs0 (L"Starfield ", L"Remastered Buffer: %36hs (%3d) using FP16", buffer_defs [i]->bufferName, i);
                break;
              }
            }
          }

          __except (EXCEPTION_EXECUTE_HANDLER)
          {
          };
        }
      }
    }
  }
  
  __except (EXCEPTION_EXECUTE_HANDLER)
  {
    SK_LOGs0 (L"Starfield ", L"Structured Exception During HDR Init");
  };
}
#else
typedef HRESULT (WINAPI * D3DXCreateCubeTextureFromFileInMemoryEx_pfn)(
    LPDIRECT3DDEVICE9, LPCVOID, UINT, UINT, UINT, DWORD, D3DFORMAT, D3DPOOL, DWORD, DWORD, D3DCOLOR, LPCVOID, LPCVOID, LPCVOID
);

typedef HRESULT(WINAPI * D3DXCreateTextureFromFileInMemoryEx_pfn)(
    LPDIRECT3DDEVICE9, LPCVOID, UINT, UINT, UINT, UINT, DWORD, D3DFORMAT, D3DPOOL, DWORD, DWORD, D3DCOLOR, LPCVOID, LPCVOID, LPCVOID
);

typedef HRESULT(WINAPI * D3DXCreateVolumeTextureFromFileInMemoryEx_pfn)(
    LPDIRECT3DDEVICE9, LPCVOID, UINT, UINT, UINT, UINT, UINT, DWORD, D3DFORMAT, D3DPOOL, DWORD, DWORD, D3DCOLOR, LPCVOID, LPCVOID, LPCVOID
);

D3DXCreateCubeTextureFromFileInMemoryEx_pfn BGS_CreateCube;
D3DXCreateTextureFromFileInMemoryEx_pfn BGS_CreateTexture;
D3DXCreateVolumeTextureFromFileInMemoryEx_pfn BGS_CreateVolume;

// Code by karut https://github.com/carxt
HRESULT __stdcall CreateCubeTextureFromFileInMemoryHookForD3D9(LPDIRECT3DDEVICE9 pDevice, LPCVOID pSrcData, UINT SrcDataSize, LPDIRECT3DCUBETEXTURE9* ppCubeTexture) {
    return BGS_CreateCube(pDevice, pSrcData, SrcDataSize, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, ppCubeTexture);
}
HRESULT __stdcall CreateTextureFromFileInMemoryHookForD3D9(LPDIRECT3DDEVICE9 pDevice, LPCVOID pSrcData, UINT SrcDataSize, LPDIRECT3DTEXTURE9* ppTexture) {
    return BGS_CreateTexture(pDevice, pSrcData, SrcDataSize, D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, ppTexture);
}
HRESULT __stdcall CreateVolumeTextureFromFileInMemoryHookForD3D9(LPDIRECT3DDEVICE9 pDevice, LPCVOID pSrcFile, UINT SrcData, LPDIRECT3DVOLUMETEXTURE9* ppVolumeTexture) {
    return BGS_CreateVolume(pDevice, pSrcFile, SrcData, D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, ppVolumeTexture);
}
#endif

#ifdef _WIN64
void
__stdcall
SK_SF_ResolutionCallback (ResolutionInfo *info, void*)
{
  RECT rcMonitor =
    SK_GetCurrentRenderBackend ().displays [SK_GetCurrentRenderBackend ().active_display].rect;

  SK_ComQIPtr <IDXGISwapChain>
      pSwapChain (SK_GetCurrentRenderBackend ().swapchain);
  if (pSwapChain != nullptr)
  {
    DXGI_SWAP_CHAIN_DESC  swapDesc = { };
    pSwapChain->GetDesc (&swapDesc);

    rcMonitor.left = 0;
    rcMonitor.right = swapDesc.BufferDesc.Width;

    rcMonitor.top    = 0;
    rcMonitor.bottom = swapDesc.BufferDesc.Height;
  }

  SK_ImGui_WarningWithTitle (
    SK_FormatStringW ( L"Current Resolution: %dx%d\r\n"
                       L"\tIdeal Resolution:\t%dx%d\t\t(%4.2f%% Scale)\r\n\r\n"
                       L" * Please Use a Different In-Game Resolution Scale...",
                         info->current.width,  info->current.height,
                         info->optimal.width,  info->optimal.height, 100.0f *
                (double)(info->optimal.width * info->optimal.height) / ((double)(rcMonitor.right  - rcMonitor.left) *
                                                                        (double)(rcMonitor.bottom - rcMonitor.top)) ).c_str (),
                       L"Unsupported Resolution Scale for DLSS Mode"
  );
}

void
__stdcall
SK_SF_EndOfFrame (void)
{
  SK_RunOnce (
  {
    HMODULE hMod =
      SK_GetModuleHandleW (L"FSR2Streamline.asi");
  
    if (hMod != 0)
    {
      f2sGetDLSSGInfo =
     (f2sGetDLSSGInfo_pfn) SK_GetProcAddress ( L"FSR2Streamline.asi",
     "f2sGetDLSSGInfo" );
  
      f2sLoadConfig =
     (f2sLoadConfig_pfn) SK_GetProcAddress ( L"FSR2Streamline.asi",
      "f2sLoadConfig" );
  
      f2sSetConfigValue =
     (f2sSetConfigValue_pfn) SK_GetProcAddress ( L"FSR2Streamline.asi",
     "f2sSetConfigValue" );

      f2sGetConfigValue =
     (f2sGetConfigValue_pfn) SK_GetProcAddress ( L"FSR2Streamline.asi",
     "f2sGetConfigValue" );

      f2sRegisterResolutionErrorCallback =
     (f2sRegisterResolutionErrorCallback_pfn) SK_GetProcAddress ( L"FSR2Streamline.asi",
     "f2sRegisterResolutionErrorCallback" );

      if (f2sRegisterResolutionErrorCallback != nullptr)
      {
        static uint32_t callback_id = 0;

        f2sRegisterResolutionErrorCallback (SK_SF_ResolutionCallback, nullptr, &callback_id);
      }
    }
  });

  if (f2sGetDLSSGInfo != nullptr)
  {
    if (f2sGetDLSSGInfo (&dlssg_state))
    {
      __SK_HasDLSSGStatusSupport = dlssg_state.dlssgSupported;
      __SK_IsDLSSGActive         = dlssg_state.dlssgEnabled && dlssg_state.currentMode == DLSSGMode::eOn;
    }
  }
}
#endif

void
SK_BGS_InitPlugin(void)
{
  SK_GAME_ID gameID = SK_GetCurrentGameID ();
  
#ifdef _WIN64
  if (gameID == SK_GAME_ID::Starfield)
  {
    // Default these to on if user is using HDR
    sf_bRemasterHDRRTs      = (__SK_HDR_10BitSwap || __SK_HDR_16BitSwap);
    sf_bRemasterExtendedRTs = (__SK_HDR_10BitSwap || __SK_HDR_16BitSwap);

    __SK_SF_BasicRemastering =
      _CreateConfigParameterBool ( L"Starfield.PlugIn",
                                   L"BasicRTUpgrades", sf_bRemasterBasicRTs,
                                                       L"Promote Simple RTs to FP16" );
  
    __SK_SF_ExtendedRemastering =
      _CreateConfigParameterBool ( L"Starfield.PlugIn",
                                   L"ExtendedRTUpgrades", sf_bRemasterExtendedRTs,
                                                          L"Promote Most 8-bit RTs to FP16" );

    __SK_SF_HDRRemastering =
      _CreateConfigParameterBool ( L"Starfield.PlugIn",
                                   L"HDRRTUpgrades", sf_bRemasterHDRRTs,
                                                     L"Promote 11-bit HDR RTs to FP16" );

    __SK_SF_PhotoModeCompatibility =
      _CreateConfigParameterBool ( L"Starfield.PlugIn",
                                   L"PhotoModeCompatibility", sf_bPhotoModeCompatibility,
                                                              L"Ignore Image Space Buffer When Promotion RTs to FP16" );

    __SK_SF_DisableFPSLimit =
      _CreateConfigParameterBool ( L"Starfield.PlugIn",
                                   L"DisableFPSLimit", sf_bDisableFPSLimit,
                                                       L"Disable the game's framerate limiter" );

    __SK_SF_ImageAddr0 =
      _CreateConfigParameterInt64 ( L"Starfield.PlugIn",
                                    L"ImageAddr0",
                                     pImageAddr0 );

    __SK_SF_BufferDefAddr =
      _CreateConfigParameterInt64 ( L"Starfield.PlugIn",
                                    L"BufferDefAddr",
                                     pBufferDefAddr );

    __SK_SF_FPSLimitAddr =
      _CreateConfigParameterInt64 ( L"Starfield.PlugIn",
                                    L"FPSLimitAddr",
                                     pFPSLimitAddr );
  
    std::queue <DWORD> threads;

    if (pImageAddr0 == -1)
      threads = SK_SuspendAllOtherThreads ();

    SK_SEH_InitStarfieldRTs ();
    SK_SEH_InitStarfieldFPS ();

    __SK_SF_ImageAddr0->store    (pImageAddr0);
    __SK_SF_BufferDefAddr->store (pBufferDefAddr);
    __SK_SF_FPSLimitAddr->store  (pFPSLimitAddr);

    dll_ini->write ();
  
    if (game_ini == nullptr) {
        game_ini = SK_CreateINI (LR"(.\Starfield.ini)");
    }
  
    if (gameCustom_ini == nullptr) {
        gameCustom_ini = SK_CreateINI ((SK_GetDocumentsDir () + LR"(\My Games\Starfield\StarfieldCustom.ini)").c_str ());
    }
  
    game_ini->set_encoding       (iSK_INI::INI_UTF8NOBOM);
    gameCustom_ini->set_encoding (iSK_INI::INI_UTF8NOBOM);
  
    sf_1stFOV  = dynamic_cast <sk::ParameterFloat *> (g_ParameterFactory->create_parameter <float> (L"First Person FOV"));
    sf_3rdFOV  = dynamic_cast <sk::ParameterFloat *> (g_ParameterFactory->create_parameter <float> (L"Third Person FOV"));
    sf_MipBias = dynamic_cast <sk::ParameterFloat *> (g_ParameterFactory->create_parameter <float> (L"Mipmap Bias"));
    
    sf_1stFOV->register_to_ini  (gameCustom_ini, L"Camera",  L"fFPWorldFOV");
    sf_3rdFOV->register_to_ini  (gameCustom_ini, L"Camera",  L"fTPWorldFOV");
    sf_MipBias->register_to_ini (gameCustom_ini, L"Display", L"fMipBiasOffset");


    if (SK_GetDLLVersionStr (SK_GetHostApp ()).find (L"1.7.23.0") != std::wstring::npos)
    {
      if (SK_GetModuleHandle (L"steam_api64.dll")) // Steam
      {
        pf1stFOV  = reinterpret_cast<float *>(CalculateOffset (0x14557B930) + 8);
        pf3rdFOV  = reinterpret_cast<float *>(CalculateOffset (0x14557B910) + 8);
        pfMipBias = reinterpret_cast<float *>(CalculateOffset (0x1455FDE70) + 8);
      }

      else // Microsoft Store
      {
        pf1stFOV  = reinterpret_cast<float *>(CalculateOffset (0x14559E7F0) + 8);
        pf3rdFOV  = reinterpret_cast<float *>(CalculateOffset (0x14559E7D0) + 8);
        pfMipBias = reinterpret_cast<float *>(CalculateOffset (0x145620ED0) + 8);
      }
    }

    else if (SK_GetDLLVersionStr (SK_GetHostApp ()).find (L"1.7.29.0") != std::wstring::npos)
    {
      if (SK_GetModuleHandle (L"steam_api64.dll")) // Steam
      {
        pf1stFOV  = reinterpret_cast<float *>(CalculateOffset (0x14557C7B0) + 8);
        pf3rdFOV  = reinterpret_cast<float *>(CalculateOffset (0x14557C790) + 8);
        pfMipBias = reinterpret_cast<float *>(CalculateOffset (0x1455FECF0) + 8);
      }

      else
        SK_LOGs0 (L"Starfield ", L"Incompatible Executable Detected");
    }

    else
      SK_LOGs0 (L"Starfield ", L"Incompatible Executable Detected");
  
    plugin_mgr->config_fns.emplace    (SK_SF_PlugInCfg);
    plugin_mgr->end_frame_fns.emplace (SK_SF_EndOfFrame);

    SK_ResumeThreads (threads);
  }
#else

    // Forces D3DPOOL_DEFAULT pool in order to allow texture debugging and caching
    if (config.textures.d3d9_mod) {
        BGS_CreateCube      = reinterpret_cast<D3DXCreateCubeTextureFromFileInMemoryEx_pfn>  (SK_GetProcAddress(L"D3DX9_43.dll", "D3DXCreateCubeTextureFromFileInMemoryEx"));
        BGS_CreateTexture   = reinterpret_cast<D3DXCreateTextureFromFileInMemoryEx_pfn>      (SK_GetProcAddress(L"D3DX9_43.dll", "D3DXCreateTextureFromFileInMemoryEx"));
        BGS_CreateVolume    = reinterpret_cast<D3DXCreateVolumeTextureFromFileInMemoryEx_pfn>(SK_GetProcAddress(L"D3DX9_43.dll", "D3DXCreateVolumeTextureFromFileInMemoryEx"));

        if (BGS_CreateCube && BGS_CreateTexture && BGS_CreateVolume) {
            uintptr_t baseTexture   = 0;
            uintptr_t cubeTexture   = 0;
            uintptr_t volumeTexture = 0;

            switch (gameID) {
            case SK_GAME_ID::FalloutNewVegas:
                baseTexture   = 0xFDF3FC;
                cubeTexture   = 0xFDF400;
                volumeTexture = 0xFDF404;
                break;
            case SK_GAME_ID::Fallout3:
                baseTexture   = 0xD9B3F0;
                cubeTexture   = 0xD9B3F4;
                volumeTexture = 0xD9B3FC;
                break;
            case SK_GAME_ID::Oblivion:
                baseTexture   = 0xA28364;
                cubeTexture   = 0xA28368;
                volumeTexture = 0xA2836C;
                break;
            }

            if (baseTexture != 0) {
                DWORD oldProtect;

                if (VirtualProtect(reinterpret_cast<void*>(baseTexture), 4, PAGE_EXECUTE_READWRITE, &oldProtect)) {
                    *reinterpret_cast<UINT32*>(baseTexture) = reinterpret_cast<UINT32>(CreateTextureFromFileInMemoryHookForD3D9);
                    VirtualProtect(reinterpret_cast<void*>(baseTexture), 4, oldProtect, &oldProtect);
                }

                if (VirtualProtect(reinterpret_cast<void*>(cubeTexture), 4, PAGE_EXECUTE_READWRITE, &oldProtect)) {
                    *reinterpret_cast<UINT32*>(cubeTexture) = reinterpret_cast<UINT32>(CreateCubeTextureFromFileInMemoryHookForD3D9);
                    VirtualProtect(reinterpret_cast<void*>(cubeTexture), 4, oldProtect, &oldProtect);
                }

                if (VirtualProtect(reinterpret_cast<void*>(volumeTexture), 4, PAGE_EXECUTE_READWRITE, &oldProtect)) {
                    *reinterpret_cast<UINT32*>(volumeTexture) = reinterpret_cast<UINT32>(CreateVolumeTextureFromFileInMemoryHookForD3D9);
                    VirtualProtect(reinterpret_cast<void*>(volumeTexture), 4, oldProtect, &oldProtect);
                }
            }
        }
    }
#endif


}