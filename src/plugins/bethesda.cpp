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

extern bool __SK_HasDLSSGStatusSupport;
extern bool __SK_IsDLSSGActive;
extern bool __SK_DoubleUpOnReflex;
extern bool __SK_ForceDLSSGPacing;

static iSK_INI* game_ini       = nullptr;
static iSK_INI* gameCustom_ini = nullptr;

#ifdef _WIN64

struct address_cache_s {
  std::unordered_map <std::string, uintptr_t> steam;
  std::unordered_map <std::string, uintptr_t> microsoft;
};

template <auto _N>
  using code_bytes_t =
     boost::container::static_vector <uint8_t, _N>;

struct sf_code_patch_s {
  void* pAddr = nullptr;

  struct executable_code_s {
    code_bytes_t <4> inst_bytes;
  } original, replacement;
  
  void apply (executable_code_s *pExec);
};

struct {
  auto &getAddresses (bool bSteam)
  {
    return                    bSteam ?
      addresses [game_ver_str].steam :
      addresses [game_ver_str].microsoft;
  }

  bool   bDisableFPSLimit = false; // Assume DLSS-G mods already do this

  float* pfMipBias        = nullptr;

  int64_t cachedImageAddr0     = -1;
  int64_t cachedBufferDefAddr  = -1;
  int64_t cachedFPSLimitAddr   = -1;
  int64_t cachedMipBiasAddr    = -1;

  bool bRemasterBasicRTs       = true;
  bool bRemasterExtendedRTs    = false;
  bool bRemasterHDRRTs         = false;
  bool bPhotoModeCompatibility = false;
  bool bAlternateThreadSched   = true;

  struct {
    sk::ParameterBool* basic_remastering        = nullptr;
    sk::ParameterBool* extended_remastering     = nullptr;
    sk::ParameterBool* hdr_remastering          = nullptr;
    sk::ParameterBool* photo_mode_compatibility = nullptr;
    sk::ParameterBool* disable_fps_limit        = nullptr;
    sk::ParameterBool* alternate_thread_sched   = nullptr;
    sk::ParameterBool* combined_reflex_sk_limit = nullptr;

    sk::ParameterInt64* image_addr      = nullptr;
    sk::ParameterInt64* buffer_def_addr = nullptr;
    sk::ParameterInt64* fps_limit_addr  = nullptr;
    sk::ParameterInt64* mip_bias_addr   = nullptr;
  } ini;

  struct {
    sk::ParameterFloat* sf_MipBias  = nullptr;
    sk::ParameterFloat* sf_ResScale = nullptr;
  } game_ini;

  std::unordered_map < std::wstring,
    address_cache_s
  > addresses;

  std::wstring game_ver_str;
} SK_SF_PlugIn;

static uint8_t sf_cOriginalFPSLimitInsts [5] = { 0x0 };

namespace BS_DXGI
{
  enum FORMAT {
    UNKNOWN0 = 0,
    R8_UNORM = 1,
    R8_SNORM = 2,
    R8_UINT,
    R8_SINT,
    UNKNOWN5,
    UNKNOWN6,
    B4G4R4A4_UNORM,
    UNKNOWN8,
    UNKNOWN9,
    B5G6R5_UNORM,
    B5G6R5_UNORM_,
    UNKNOWN12,
    B5G5R5A1_UNORM,
    R8G8_UNORM,
    R8G8_SNORM,
    UNKNOWN16,
    UNKNOWN17,
    R8G8_UINT,
    R8G8_SINT,
    UNKNOWN20,
    R16_UNORM,
    R16_SNORM,
    R16_UINT,
    R16_SINT,
    R16_FLOAT,
    UNKNOWN26,
    UNKNOWN27,
    UNKNOWN28,
    UNKNOWN29,
    UNKNOWN30,
    UNKNOWN31,
    UNKNOWN32,
    UNKNOWN33,
    UNKNOWN34,
    UNKNOWN35,
    R8G8B8A8_UNORM,
    R8G8B8A8_SNORM,
    R8G8B8A8_UINT,
    R8G8B8A8_SINT,
    R8G8B8A8_UNORM_SRGB,
    B8G8R8A8_UNORM,
    UNKNOWN42,
    UNKNOWN43,
    UNKNOWN44,
    B8G8R8A8_UNORM_SRGB,
    UNKNOWN46,
    B8G8R8X8_UNORM,
    R16G16_UNORM,
    UNKNOWN49,
    R16G16_SNORM,
    UNKNOWN51,
    R16G16_UINT,
    R16G16_SINT,
    R16G16_FLOAT,
    R32_UINT,
    R32_SINT,
    R32_FLOAT,
    UNKNOWN58,
    UNKNOWN59,
    UNKNOWN60,
    UNKNOWN61,
    R10G10B10A2_UNORM,
    R10G10B10A2_UINT,
    UNKNOWN64,
    UNKNOWN65,
    R11G11B10_FLOAT,
    R9G9B9E5_SHAREDEXP,
    UNKNOWN68,
    UNKNOWN69,
    UNKNOWN70,
    UNKNOWN71,
    UNKNOWN72,
    R16G16B16A16_UNORM,
    R16G16B16A16_SNORM,
    R16G16B16A16_UINT,
    R16G16B16A16_SINT,
    R16G16B16A16_FLOAT,
    R32G32_UINT,
    R32G32_SINT,
    R32G32_FLOAT,
    R32G32B32_UINT,
    R32G32B32_SINT,
    R32G32B32_FLOAT,
    R32G32B32A32_UINT,
    R32G32B32A32_SINT,
    R32G32B32A32_FLOAT,
    UNKNOWN87,
    UNKNOWN88,
    UNKNOWN89,
    UNKNOWN90,
    UNKNOWN91,
    UNKNOWN92,
    UNKNOWN93,
    UNKNOWN94,
    UNKNOWN95,
    UNKNOWN96,
    UNKNOWN97,
    UNKNOWN98,
    D16_UNORM,
    D24_UNORM_S8_UINT,
    D32_FLOAT,
    D24_UNORM_S8_UINT_,
    D24_UNORM_S8_UINT__,
    D32_FLOAT_S8X24_UINT,
    BC1_UNORM,
    BC1_UNORM_SRGB,
    BC1_UNORM_,
    BC1_UNORM_SRGB_,
    BC2_UNORM,
    BC2_UNORM_SRGB,
    BC3_UNORM,
    BC3_UNORM_SRGB,
    BC4_UNORM,
    BC4_SNORM,
    BC5_UNORM,
    BC5_SNORM,
    BC6H_UF16,
    BC6H_SF16,
    BC7_UNORM,
    BC7_UNORM_SRGB
  };
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
    (void *)(SK_SF_PlugIn.cachedFPSLimitAddr);

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
//eUltraPerformance,
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
 { DLSSMode::eOff,              "UltraPerformance" },
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

void __stdcall SK_SF_ResolutionCallback (ResolutionInfo *info, void *);

bool
SK_SF_InitFSR2Streamline (void)
{
  if (f2sRegisterResolutionErrorCallback != nullptr)
    return true;

  static constexpr int _MAX_INIT_ATTEMPTS = 30;
  static int tries = 0;
  if (     ++tries >   _MAX_INIT_ATTEMPTS )
    return false;

  if (f2sRegisterResolutionErrorCallback == nullptr)
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

      return true;
    }
  }

  return true;
}

bool SK_SF_PlugInCfg (void)
{
  static auto& plugin =
    SK_SF_PlugIn;

  static std::string  utf8VersionString =
    SK_WideCharToUTF8 (SK_GetDLLVersionStr (SK_GetHostApp ()));

  SK_SF_InitFSR2Streamline ();

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
      changed |= ImGui::Checkbox ("Upgrade Base 8-bpc RTs to 16-bpc", &plugin.bRemasterBasicRTs);

      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("Eliminates banding on UI at the cost of (negligible) extra VRAM");

      if (plugin.bRemasterBasicRTs)
      {
        changed |= ImGui::Checkbox ("PhotoMode Compatibility", &plugin.bPhotoModeCompatibility);

        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("Reduce dynamic range to 8-bpc on Image Space Buffers so that Photo Mode does not crash.");
      }

      ImGui::EndGroup   ();
      ImGui::SameLine   ();
      ImGui::BeginGroup ();

      changed |= ImGui::Checkbox ("Upgrade Most 8-bpc RTs to 16-bpc", &plugin.bRemasterExtendedRTs);

      if (ImGui::IsItemHovered ())
      {
        ImGui::SetTooltip ("May further reduce banding and improve HDR, but at high memory cost");
      }

      changed |= ImGui::Checkbox ("Upgrade HDR (11-bpc) RTs to 16-bpc", &plugin.bRemasterHDRRTs);

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

      if (plugin.pfMipBias != nullptr)
      {
        changed_no_restart_needed |=
          ImGui::SliderFloat ("Mipmap Bias", plugin.pfMipBias, -2.f, 2.f, "%.1f");

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

        plugin.ini.basic_remastering->store        (plugin.bRemasterBasicRTs);
        plugin.ini.extended_remastering->store     (plugin.bRemasterExtendedRTs);
        plugin.ini.hdr_remastering->store          (plugin.bRemasterHDRRTs);
        plugin.ini.photo_mode_compatibility->store (plugin.bPhotoModeCompatibility);

        if (plugin.pfMipBias != nullptr)
        {
           plugin.game_ini.sf_MipBias->store (*plugin.pfMipBias);
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

    if (plugin.cachedFPSLimitAddr > 0)
    {
      if (ImGui::CollapsingHeader ("Performance"))
      {
        ImGui::TreePush ("");

        if (ImGui::Checkbox ("Disable In-Game Framerate Limit", &plugin.bDisableFPSLimit))
        {
          SK_SF_EnableFPSLimiter (! plugin.bDisableFPSLimit);

          plugin.ini.disable_fps_limit->store (plugin.bDisableFPSLimit);

          dll_ini->write ();
        }

        static bool game_restart_required = false;

        if (ImGui::Checkbox ("Use Alternate Thread Scheduling", &plugin.bAlternateThreadSched))
        {
          game_restart_required = true;

          plugin.ini.alternate_thread_sched->store (plugin.bAlternateThreadSched);

          dll_ini->write ();
        }

        if (ImGui::IsItemHovered ())
        {
          ImGui::SetTooltip ("May improve disk throughput and general CPU efficiency.");
        }

        if (game_restart_required)
        {
          ImGui::PushStyleColor (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (.3f, .8f, .9f));
          ImGui::BulletText     ("Game Restart Required");
          ImGui::PopStyleColor  ();
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
                ImGui::TextColored (ImVec4 (0.f, 1.f, 0.f, 1.f), "Active");
                ImGui::SameLine    ();
                ImGui::SeparatorEx (ImGuiSeparatorFlags_Vertical);
                ImGui::SameLine    ();
                ImGui::Text        ("Frame Generation Rate: %dx", dlssg_state.numFramesActuallyPresented);
                break;
              case DLSSGMode::eOff:
                ImGui::TextColored (ImVec4 (1.f, 1.f, 0.f, 1.f), "Inactive");
                break;
              case DLSSGMode::eCount:
              default:
                ImGui::TextColored (ImVec4 (1.f, 0.f, 0.f, 1.f), "Unknown?!");
                break;
            }
          else  ImGui::TextColored (ImColor::HSV (0.1f, 1.f, 1.f), "Disabled by User");
        }
      } else    ImGui::Text        ("Unknown Malfunction");

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
            if (ImGui::Checkbox ("Combine SK + Reflex Limiter", &__SK_DoubleUpOnReflex))
            {
              plugin.ini.combined_reflex_sk_limit->store (__SK_DoubleUpOnReflex);

              dll_ini->write ();
            }

            if (ImGui::IsItemHovered ())
            {
              ImGui::SetTooltip ("May improve frame pacing, for marginal increase in latency.");
            }
          }
        }

        bool changed_scale =
          ImGui::Combo ("DLSS Mode", (int *)&dlss_prefs.mode, "Off\0"
                                                              "Max Performance\t(50% Scale)\0"
                                                              "Balanced\t\t\t\t  (58% Scale)\0"
                                                              "Max Quality\t\t\t  (66% Scale)\0"
                                                              "DLAA\t\t\t\t\t\t(100% Scale)\0\0");

        changed |= changed_scale;

        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip ();
          ImGui::TextUnformatted (
            "The % indicates the DLSS Mode's ideal resolution scale." );
          ImGui::Separator ();
          ImGui::BulletText ("The game's settings will be changed, but may not apply immediately.");
          ImGui::BulletText ("To apply changes, move the resolution slider in the settings menu.");
          ImGui::EndTooltip ();
        }

        changed |=
          ImGui::Combo ("DLSS Preset", (int *)&dlss_prefs.preset, "Default\t(For Current DLSS Mode)\0"
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
        //f2s_config ["enabled"]                  = dlss_prefs.mode != DLSSMode::eOff;
          if (dlss_prefs.mode == DLSSMode::eOff)
              dlss_prefs.mode = DLSSMode::eDLAA; // Set a safe default value to prevent crashing
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

          if (changed_scale)
          {
            switch (dlss_prefs.mode)
            {
              case DLSSMode::eMaxPerformance:
                plugin.game_ini.sf_ResScale->store (0.5f);
                break;
              case DLSSMode::eBalanced:
                plugin.game_ini.sf_ResScale->store (0.58f);
                break;
              case DLSSMode::eMaxQuality:
                plugin.game_ini.sf_ResScale->store (0.66f);
                break;
              case DLSSMode::eDLAA:
                plugin.game_ini.sf_ResScale->store (1.0f);
                break;
            }

            game_ini->write ();
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

static SetThreadPriority_pfn
       SetThreadPriority_Original      = nullptr;
static SetThreadPriorityBoost_pfn
       SetThreadPriorityBoost_Original = nullptr;

BOOL
WINAPI
SK_SF_SetThreadPriorityHook ( HANDLE hThread,
                              int    nPriority )
{
  SK_SetThreadPriorityBoost (hThread, FALSE);

  //if (SK_Thread_GetCurrentPriority () > nPriority)
  //  return TRUE;

  return
    SetThreadPriority_Original (hThread, nPriority);
}

BOOL
WINAPI
SK_SF_SetThreadPriorityBoostHook ( HANDLE hThread,
                                   BOOL   bDisableBoost )
{
  wchar_t                                          *wszThreadName = nullptr;
  if (SUCCEEDED (SK_GetThreadDescription (hThread, &wszThreadName)))
  {
    if (StrStrW (wszThreadName, L"FileStreamerControl"))
    {
      SK_SetThreadPriority (hThread, THREAD_PRIORITY_ABOVE_NORMAL);
    }

    else if (StrStrW (wszThreadName, L"IOManager"))
    {
      SK_SetThreadPriority (hThread, THREAD_PRIORITY_HIGHEST);
    }

    else if (StrStrW (wszThreadName, L"SaveLoad thread"))
    {
      SK_SetThreadPriority (hThread, THREAD_PRIORITY_TIME_CRITICAL);
    }
  }

  std::ignore = bDisableBoost;

  return
    SetThreadPriorityBoost_Original (hThread, FALSE);
}

void SK_SF_InitFPS (void)
{
  static auto& plugin =
    SK_SF_PlugIn;

  // Assume that DLSS-G mods are going to turn the framerate limiter off always
  if (SK_IsModuleLoaded (L"sl.interposer.dll"))
    return;

  const bool bIsSteam =
    SK_IsModuleLoaded (L"steam_api64.dll");

  if (plugin.addresses.contains (plugin.game_ver_str))
  {
    auto& known_addresses =
      plugin.getAddresses (bIsSteam);

    if (                          known_addresses.contains ("fps_limit_addr") &&
                                  known_addresses.at       ("fps_limit_addr") > 0)
      plugin.cachedFPSLimitAddr = known_addresses.at       ("fps_limit_addr") + (int64_t)SK_Debug_GetImageBaseAddr ();
  }

  void *scan = nullptr;

  static const char *acFPSLimitPattern =
    "\xC6\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x8D\x00\x00\xE8\x00\x00\x00\x00\x33\x00\xE8";
  static const char *acFPSLimitMask    =
    "\xFF\x00\x00\x00\xFF\x00\x00\x00\x00\xFF\xFF\x00\x00\xFF\x00\x00\x00\x00\xFF\x00\xFF";

  // Try previously cached address first
  if (plugin.cachedFPSLimitAddr > 0)
  {
    scan =
      SK_ScanAlignedExec (
        acFPSLimitPattern, 21,
        acFPSLimitMask, (void *)((uintptr_t)plugin.cachedFPSLimitAddr - 0xD - 1)
      );
  }

  if (scan == nullptr)
  {
    scan =
      SK_ScanAlignedExec (
        acFPSLimitPattern, 21,
        acFPSLimitMask );
  }

  SK_LOGs0 (L"Starfield ", L"Scanned FPS Limit Address: %p", scan);

  if (scan != nullptr)
  {
    plugin.cachedFPSLimitAddr = (uintptr_t)scan + 0xD;

    memcpy (sf_cOriginalFPSLimitInsts, (void *)(plugin.cachedFPSLimitAddr), 5);

    SK_SF_EnableFPSLimiter (! plugin.bDisableFPSLimit);

    return;
  }

  plugin.cachedFPSLimitAddr = 0;
}

void SK_SF_InitRTs (void)
{
  static auto& plugin =
    SK_SF_PlugIn;

  const bool bIsSteam =
    SK_IsModuleLoaded (L"steam_api64.dll");

  if (plugin.addresses.contains (plugin.game_ver_str))
  {
    auto& known_addresses =
      plugin.getAddresses (bIsSteam);

    for ( auto& addr : { std::pair <int64_t&, const char *> (plugin.cachedImageAddr0,    "image_def_addr"),
                         std::pair <int64_t&, const char *> (plugin.cachedBufferDefAddr, "buffer_def_addr") } )

    {
      if (           known_addresses.contains (addr.second) &&
                     known_addresses.at       (addr.second) > 0 )
        addr.first = known_addresses.at       (addr.second) + (int64_t)SK_Debug_GetImageBaseAddr ();
    }
  }

  if ( plugin.bRemasterBasicRTs || plugin.bRemasterExtendedRTs ||
       plugin.bRemasterHDRRTs   ||          __SK_HDR_16BitSwap ||
                                            __SK_HDR_10BitSwap )
  {
    void *scan = nullptr;

    // Try previously cached address first
    if (plugin.cachedImageAddr0 > 0)
    {
      scan =
        SK_ScanAlignedExec ( "\x44\x8B\x05\x00\x00\x00\x00\x89\x55\xFB", 10,
                             "\xFF\xFF\xFF\x00\x00\x00\x00\xFF\xFF\xFF", (void *)((uintptr_t)plugin.cachedImageAddr0 - 1) );
    }

    if (scan == nullptr)
    {
      scan =
        SK_ScanAlignedExec ( "\x44\x8B\x05\x00\x00\x00\x00\x89\x55\xFB", 10,
                             "\xFF\xFF\xFF\x00\x00\x00\x00\xFF\xFF\xFF", nullptr, 8 );
    }

    SK_LOGs0 (L"Starfield ", L"Scanned Image Address:     %p", scan);

    if (plugin.bRemasterBasicRTs)
    {
      const auto offset                      = *reinterpret_cast < int32_t *>((uintptr_t)scan + 3);
      uint32_t  *imageSpaceBufferPtr         =  reinterpret_cast <uint32_t *>((uintptr_t)scan + 7 + offset);            // 5079A70
      uint32_t  *scaleformCompositeBufferPtr =  reinterpret_cast <uint32_t *>((uintptr_t)imageSpaceBufferPtr + 0x280);  // 5079CF0

      if (! plugin.bPhotoModeCompatibility)
        *imageSpaceBufferPtr       = BS_DXGI::FORMAT::R16G16B16A16_FLOAT;

      *scaleformCompositeBufferPtr = BS_DXGI::FORMAT::R16G16B16A16_FLOAT;
    }

    if (scan != nullptr)
      plugin.cachedImageAddr0 = (int64_t)scan;

    if ( plugin.bRemasterExtendedRTs || plugin.bRemasterHDRRTs ||
                  __SK_HDR_16BitSwap || __SK_HDR_10BitSwap )
    {
      scan = nullptr;

      // Try previously cached address first
      if (plugin.cachedBufferDefAddr > 0)
      {
        scan =
          SK_ScanAlignedExec ( "\x4C\x8D\x15\x00\x00\x00\x00\xBE\x00\x00\x00\x00", 12,
                               "\xFF\xFF\xFF\x00\x00\x00\x00\xFF\x00\x00\x00\x00", (void *)((uintptr_t)plugin.cachedBufferDefAddr - 1) );
      }

      if (scan == nullptr)
      {
        scan =
          SK_ScanAlignedExec ( "\x4C\x8D\x15\x00\x00\x00\x00\xBE\x00\x00\x00\x00", 12,
                               "\xFF\xFF\xFF\x00\x00\x00\x00\xFF\x00\x00\x00\x00", (void *)plugin.cachedImageAddr0 );
      }

      SK_LOGs0 (L"Starfield ", L"Scanned Buffer Array:      %p", scan);

      if (scan != nullptr)
        plugin.cachedBufferDefAddr = (int64_t)scan;

		  const auto offset  = *reinterpret_cast < int32_t *>((uintptr_t)scan  + 3);
		  const auto address =  reinterpret_cast < uintptr_t>(           scan) + 7 + offset;

      struct BufferDefinition
      {
        BS_DXGI::FORMAT format;
        uint32_t        unk04;
        const char*     bufferName;
        uint16_t        unk10;
        uint16_t        unk12;
        uint32_t        unk14;
        uint16_t        unk18;
        uint16_t        unk1A;
        uint32_t        unk1C;
        uint16_t        unk20;
        uint16_t        unk22;
        uint32_t        unk24;
        uint16_t        unk28;
        uint16_t        unk2A;
        uint32_t        unk2C;
        uint32_t        unk30;
        float           unk34;
        float           unk38;
        float           unk3C;
        uint32_t        unk40;
        uint32_t        unk44;
        uint32_t        unk48;
        uint32_t        unk4C;
      };

      BufferDefinition **buffer_defs =
        reinterpret_cast <BufferDefinition **>(address);  // 4718E40

      const char *buffers_to_remaster [] =
      {
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

      const char *r8_unorm_buffers_to_remaster [] =
      {
        "SAOFinalAO",
        "AlphaBuffer",
        "ProcGenDensityMap"
      };

      for (UINT i = 0 ; i < 200 ; ++i)
      {
        if (0 == strcmp (buffer_defs [i]->bufferName, "FrameBuffer"))
        {
          if (__SK_HDR_16BitSwap) buffer_defs [i]->format = BS_DXGI::FORMAT::R16G16B16A16_FLOAT;
          if (__SK_HDR_10BitSwap) buffer_defs [i]->format = BS_DXGI::FORMAT::R10G10B10A2_UNORM;

          continue;
        }

        if (plugin.bRemasterHDRRTs)
        {
          if (buffer_defs [i]->format == BS_DXGI::FORMAT::R8_UNORM)
          {
            if (0 == strcmp (buffer_defs [i]->bufferName, "ImageProcessR8Target"))
            {
              buffer_defs [i]->format  = BS_DXGI::FORMAT::R16_FLOAT;
              SK_LOGs0 (L"Starfield ", L"Remastered Buffer: %36hs (%3d) using R16_FLOAT", buffer_defs [i]->bufferName, i);
              continue;
            }

            for (auto remaster_r8 : r8_unorm_buffers_to_remaster)
            {
              if (0 == strcmp (buffer_defs [i]->bufferName, remaster_r8))
              {
                buffer_defs [i]->format  = BS_DXGI::FORMAT::R16_FLOAT;
                SK_LOGs0 (L"Starfield ", L"Remastered Buffer: %36hs (%3d) using R16_FLOAT", buffer_defs [i]->bufferName, i);
                break;
              }
            }
          }

          if (buffer_defs [i]->format == BS_DXGI::FORMAT::R11G11B10_FLOAT)
          {   buffer_defs [i]->format  = BS_DXGI::FORMAT::R16G16B16A16_FLOAT;
          
            SK_LOGs0 (L"Starfield ", L"Remastered Buffer: %36hs (%3d) using FP16", buffer_defs [i]->bufferName, i);
            continue;
          }
        }

        if (! plugin.bRemasterExtendedRTs)
          continue;

        if (0 == strcmp (buffer_defs [i]->bufferName, "FSR2_RESAMPLED_LUMA_HISTORY"))
        {
          buffer_defs [i]->format = BS_DXGI::FORMAT::R16G16B16A16_FLOAT;

          SK_LOGs0 (L"Starfield ", L"Remastered Buffer: %36hs (%3d) using FP16", buffer_defs [i]->bufferName, i);
        }

        for (auto remaster : buffers_to_remaster)
        {
          if (0 == strcmp (buffer_defs [i]->bufferName, remaster))
          {
            buffer_defs [i]->format = BS_DXGI::FORMAT::R16G16B16A16_FLOAT;

            SK_LOGs0 (L"Starfield ", L"Remastered Buffer: %36hs (%3d) using FP16", buffer_defs [i]->bufferName, i);
            break;
          }
        }
      }
    }
  }
}

void SK_SF_InitMipLODs (void)
{
  static auto& plugin =
    SK_SF_PlugIn;

  const bool bIsSteam =
    SK_IsModuleLoaded (L"steam_api64.dll");

  if (plugin.addresses.contains (plugin.game_ver_str))
  {
    auto& known_addresses =
      plugin.getAddresses (bIsSteam);

    for ( auto& addr : { std::pair <int64_t&, const char *> (plugin.cachedMipBiasAddr, "mip_bias_addr") } )

    {
      if (           known_addresses.contains (addr.second) &&
                     known_addresses.at       (addr.second) > 0 )
        addr.first = known_addresses.at       (addr.second) + (int64_t)SK_Debug_GetImageBaseAddr ();
    }
  }

  void *scan = nullptr;

  static const char *acMipBiasPattern =
    "\x80\x3D\x00\x00\x00\x00\x00\x74\x13\xC5\xFA\x10\x05\x00\x00\x00\x00\xC5\xFA\x58\x8C\x24\xB8\x00\x00\x00";
  static const char *acMipBiasMask    =
    "\xFF\xFF\x00\x00\x00\x00\x00\xFF\xFF\xFF\xFF\xFF\xFF\x00\x00\x00\x00\xFF\xFF\xFF\xFF\xFF\xFF\x00\x00\x00";

  // Try previously cached address first
  if (plugin.cachedMipBiasAddr > 0)
  {
    scan =
      SK_ScanAlignedExec (
        acMipBiasPattern, 26,
        acMipBiasMask, (void *)((uintptr_t)plugin.cachedMipBiasAddr - 1)
      );
  }

  if (scan == nullptr)
  {
    scan =
      SK_ScanAlignedExec (
        acMipBiasPattern, 26,
        acMipBiasMask );
  }

  SK_LOGs0 (L"Starfield ", L"Scanned Mip Bias Address: %p", scan);

  if (scan != nullptr)
  {
    plugin.cachedMipBiasAddr = (uintptr_t)scan;

    const uintptr_t offset   = 13;                // From base of scanned pattern
    const uintptr_t ptr_size = sizeof (uint32_t); // Instruction's address size
    const uint32_t* ptr      = (uint32_t *)(plugin.cachedMipBiasAddr + offset);

    // Next instruction's address for RIP-relative addressing
    uintptr_t rip =
      plugin.cachedMipBiasAddr + offset + ptr_size;

    plugin.pfMipBias =
      reinterpret_cast <float *> ((uintptr_t)*ptr + rip);
  }

  if (plugin.pfMipBias != nullptr)
  {
    return;
  }

  plugin.cachedMipBiasAddr = 0;
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

    rcMonitor.left  = 0;
    rcMonitor.right = swapDesc.BufferDesc.Width;

    rcMonitor.top    = 0;
    rcMonitor.bottom = swapDesc.BufferDesc.Height;
  }

  SK_ImGui_WarningWithTitle (
    SK_FormatStringW ( L"Current Resolution: %dx%d\r\n"
                       L"\tIdeal Resolution:\t%dx%d\t\t(%4.2f%% Scale)\r\n\r\n"
                       L" * Please Use a Different In-Game Resolution Scale...",
                         info->current.width,  info->current.height,
                         info->optimal.width,  info->optimal.height, 100.0f * std::sqrt (
                (double)(info->optimal.width * info->optimal.height) / ((double)(rcMonitor.right  - rcMonitor.left) *
                                                                        (double)(rcMonitor.bottom - rcMonitor.top)) ) ).c_str (),
                       L"Unsupported Resolution Scale for DLSS Mode"
  );
}

void
__stdcall
SK_SF_EndOfFrame (void)
{
  SK_SF_InitFSR2Streamline ();

  if (f2sGetDLSSGInfo != nullptr)
  {
    if (f2sGetDLSSGInfo (&dlssg_state))
    {
      __SK_HasDLSSGStatusSupport = dlssg_state.dlssgSupported;
      __SK_IsDLSSGActive         = dlssg_state.dlssgEnabled && dlssg_state.currentMode == DLSSGMode::eOn;
      __SK_ForceDLSSGPacing      = __SK_IsDLSSGActive;
    }
  }
}

bool
SK_SEH_InitStarfieldUntrusted (void)
{
  __try {
    SK_SF_InitRTs     ();
    SK_SF_InitFPS     ();
    SK_SF_InitMipLODs ();
  }

  __except (EXCEPTION_EXECUTE_HANDLER)
  {
    SK_LOGs0 (
      L"Starfield ",
        L"Encountered a Structured Exception during Plug-In Init..."
    );

    return false;
  }

  return true;
}
#endif

void
SK_BGS_InitPlugin (void)
{
  if (PathFileExistsW (L"SpecialK.NoPlugIns"))
    return;

  SK_GAME_ID gameID =
    SK_GetCurrentGameID ();
  
#ifdef _WIN64
  if (gameID == SK_GAME_ID::Starfield)
  {
    auto &plugin =
      SK_SF_PlugIn;

    plugin.game_ver_str =
      SK_GetDLLVersionStr (SK_GetHostApp ());

    SK_SF_InitFSR2Streamline ();

    // Default these to on if user is using HDR
    plugin.bRemasterHDRRTs      = (__SK_HDR_10BitSwap || __SK_HDR_16BitSwap);
    plugin.bRemasterExtendedRTs = (__SK_HDR_10BitSwap || __SK_HDR_16BitSwap);

    plugin.ini.basic_remastering =
      _CreateConfigParameterBool ( L"Starfield.PlugIn",
                                   L"BasicRTUpgrades", plugin.bRemasterBasicRTs,
                                                       L"Promote Simple RTs to FP16" );
  
    plugin.ini.extended_remastering =
      _CreateConfigParameterBool ( L"Starfield.PlugIn",
                                   L"ExtendedRTUpgrades", plugin.bRemasterExtendedRTs,
                                                          L"Promote Most 8-bit RTs to FP16" );

    plugin.ini.hdr_remastering =
      _CreateConfigParameterBool ( L"Starfield.PlugIn",
                                   L"HDRRTUpgrades", plugin.bRemasterHDRRTs,
                                                     L"Promote 11-bit HDR RTs to FP16" );

    plugin.ini.photo_mode_compatibility =
      _CreateConfigParameterBool ( L"Starfield.PlugIn",
                                   L"PhotoModeCompatibility", plugin.bPhotoModeCompatibility,
                                                              L"Ignore Image Space Buffer When Promotion RTs to FP16" );

    plugin.ini.disable_fps_limit =
      _CreateConfigParameterBool ( L"Starfield.PlugIn",
                                   L"DisableFPSLimit", plugin.bDisableFPSLimit,
                                                       L"Disable the game's framerate limiter" );

    plugin.ini.image_addr =
      _CreateConfigParameterInt64 ( L"Starfield.PlugIn",
                                    L"ImageAddr0",
                         plugin.cachedImageAddr0 );

    plugin.ini.buffer_def_addr =
      _CreateConfigParameterInt64 ( L"Starfield.PlugIn",
                                    L"BufferDefAddr",
                         plugin.cachedBufferDefAddr );

    plugin.ini.fps_limit_addr =
      _CreateConfigParameterInt64 ( L"Starfield.PlugIn",
                                    L"FPSLimitAddr",
                         plugin.cachedFPSLimitAddr );

    plugin.ini.mip_bias_addr =
      _CreateConfigParameterInt64 ( L"Starfield.PlugIn",
                                    L"MipBiasAddr",
                         plugin.cachedMipBiasAddr );

    plugin.ini.alternate_thread_sched =
      _CreateConfigParameterBool ( L"Starfield.PlugIn",
                                   L"AlternateThreadScheduling",
                         plugin.bAlternateThreadSched );

    // Default this to on, it works well in Starfield.
    __SK_DoubleUpOnReflex = true;

    plugin.ini.combined_reflex_sk_limit =
      _CreateConfigParameterBool ( L"Starfield.PlugIn",
                                   L"CombineReflexAndSKLimiters",
                         __SK_DoubleUpOnReflex );

    if (plugin.bAlternateThreadSched)
    {
      extern BOOL WINAPI
      SetThreadPriority_Detour ( HANDLE hThread,
                                 int    nPriority );
      extern DWORD WINAPI
      SetThreadPriorityBoost_Detour ( HANDLE hThread,
                                      BOOL   bDisableBoost );

      SK_CreateFuncHook ( L"SetThreadPriorityBoost_Detour",
                            SetThreadPriorityBoost_Detour,
                      SK_SF_SetThreadPriorityBoostHook,
                  (void **)&SetThreadPriorityBoost_Original );

      SK_CreateFuncHook ( L"SetThreadPriority_Detour",
                            SetThreadPriority_Detour,
                      SK_SF_SetThreadPriorityHook,
                  (void **)&SetThreadPriority_Original );

      MH_QueueEnableHook (SetThreadPriorityBoost_Detour);
      MH_QueueEnableHook (SetThreadPriority_Detour);

      SK_ApplyQueuedHooks ();
    }


    auto iBaseAddr =
      (int64_t)SK_Debug_GetImageBaseAddr ();

    if (plugin.cachedImageAddr0    > 0) plugin.cachedImageAddr0    += iBaseAddr;
    if (plugin.cachedBufferDefAddr > 0) plugin.cachedBufferDefAddr += iBaseAddr;
    if (plugin.cachedFPSLimitAddr  > 0) plugin.cachedFPSLimitAddr  += iBaseAddr;
    if (plugin.cachedMipBiasAddr   > 0) plugin.cachedMipBiasAddr   += iBaseAddr;
  
    SK_ThreadSuspension_Ctx threads;

    //if (plugin.cachedImageAddr0 == -1)
      threads = SK_SuspendAllOtherThreads ();

    plugin.addresses [L"Starfield  1.7.23.0"].steam =
    {
      { "image_def_addr",  0x00000000 }, { "buffer_def_addr", 0x00000000 },
      { "fps_limit_addr",  0x00000000 }, { "mip_bias_addr",   0x00000000 }
    };

    plugin.addresses [L"Starfield  1.7.29.0"].steam =
    {
      { "image_def_addr",  0x00000000 }, { "buffer_def_addr", 0x00000000 },
      { "fps_limit_addr",  0x00000000 }, { "mip_bias_addr",   0x00000000 }
    };

    plugin.addresses [L"Starfield  1.7.33.0"].steam =
    {
      { "image_def_addr",  0x3287B88 }, { "buffer_def_addr", 0x33516FA },
      { "fps_limit_addr",  0x23FA5E9 }, { "mip_bias_addr",   0x32D6ED2 }
    };

    plugin.addresses [L"Starfield  1.7.33.0"].microsoft =
    {
      { "image_def_addr",  0x32A79E8 }, { "buffer_def_addr", 0x337155A },
      { "fps_limit_addr",  0x23F7F61 }, { "mip_bias_addr",   0x32F6D32 }
    };

    SK_SEH_InitStarfieldUntrusted ();

    plugin.ini.image_addr->store               (plugin.cachedImageAddr0    - iBaseAddr);
    plugin.ini.buffer_def_addr->store          (plugin.cachedBufferDefAddr - iBaseAddr);
    plugin.ini.fps_limit_addr->store           (plugin.cachedFPSLimitAddr  - iBaseAddr);
    plugin.ini.mip_bias_addr->store            (plugin.cachedMipBiasAddr   - iBaseAddr);
    plugin.ini.alternate_thread_sched->store   (plugin.bAlternateThreadSched);
    plugin.ini.combined_reflex_sk_limit->store (__SK_DoubleUpOnReflex);

    dll_ini->write ();
  
    if (game_ini == nullptr) {
        game_ini = SK_CreateINI ((SK_GetDocumentsDir () + LR"(\My Games\Starfield\StarfieldPrefs.ini)").c_str ());
    }
  
    if (gameCustom_ini == nullptr) {
        gameCustom_ini = SK_CreateINI ((SK_GetDocumentsDir () + LR"(\My Games\Starfield\StarfieldCustom.ini)").c_str ());
    }
  
    game_ini->set_encoding       (iSK_INI::INI_UTF8NOBOM);
    gameCustom_ini->set_encoding (iSK_INI::INI_UTF8NOBOM);
  
    plugin.game_ini.sf_MipBias  = dynamic_cast <sk::ParameterFloat *> (g_ParameterFactory->create_parameter <float> (L"Mipmap Bias"));
    plugin.game_ini.sf_ResScale = dynamic_cast <sk::ParameterFloat *> (g_ParameterFactory->create_parameter <float> (L"Resolution Scale"));
    
    plugin.game_ini.sf_MipBias->register_to_ini  (gameCustom_ini, L"Display", L"fMipBiasOffset");
    plugin.game_ini.sf_ResScale->register_to_ini (game_ini,       L"Display", L"fRenderResolutionScaleFactor");

    //else
    //  SK_LOGs0 (L"Starfield ", L"Incompatible Executable Detected");
  
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