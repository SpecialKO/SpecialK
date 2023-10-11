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

#include <SpecialK/render/ngx/ngx.h>
#include <imgui/font_awesome.h>

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L" NGX Core "

bool __SK_HasDLSSGStatusSupport = false;
bool __SK_IsDLSSGActive         = false;
bool __SK_DoubleUpOnReflex      = false;
bool __SK_ForceDLSSGPacing      = false;

SK_LazyGlobal <NGX_ThreadSafety> SK_NGX_Threading;

static unsigned int SK_NGX_GameSetPerfQuality = 0;

// NGX return-code conversion-to-string utility only as a helper for debugging/logging - not for official use.
GetNGXResultAsString_pfn                  GetNGXResultAsString                     = nullptr;

NVSDK_NGX_Parameter_SetF_pfn           NVSDK_NGX_Parameter_SetF_Original           = nullptr;
NVSDK_NGX_Parameter_SetI_pfn           NVSDK_NGX_Parameter_SetI_Original           = nullptr;
NVSDK_NGX_Parameter_SetUI_pfn          NVSDK_NGX_Parameter_SetUI_Original          = nullptr;
NVSDK_NGX_Parameter_GetUI_pfn          NVSDK_NGX_Parameter_GetUI_Original          = nullptr;
NVSDK_NGX_Parameter_GetVoidPointer_pfn NVSDK_NGX_Parameter_GetVoidPointer_Original = nullptr;

void
NVSDK_CONV
NVSDK_NGX_Parameter_SetF_Detour (NVSDK_NGX_Parameter* InParameter, const char* InName, float InValue)
{
  SK_LOG_FIRST_CALL

  SK_LOGi1 (L"NGX_Parameter_SetF (%hs, %f) - %ws", InName, InValue, SK_GetCallerName ().c_str ());

  if (! _stricmp (InName, NVSDK_NGX_Parameter_Sharpness))
  {
    if (config.nvidia.dlss.use_sharpening != -1)
    {
      float sharpness = config.nvidia.dlss.use_sharpening == 1 ?
                        config.nvidia.dlss.forced_sharpness  : 0;

      if (InValue != sharpness)
      {
        SK_LOGi0 (
          L"Overriding DLSS Sharpness (Requested: %4.2f, Forced: %4.2f)",
            InValue, sharpness
        );

        InValue = sharpness;
      }
    }
  }

  NVSDK_NGX_Parameter_SetF_Original (InParameter, InName, InValue);
}

void
NVSDK_CONV
NVSDK_NGX_Parameter_SetI_Detour (NVSDK_NGX_Parameter* InParameter, const char* InName, int InValue)
{
  SK_LOG_FIRST_CALL

  SK_LOGi1 (L"NGX_Parameter_SetI (%hs, %i) - %ws", InName, InValue, SK_GetCallerName ().c_str ());

  if (! _stricmp (InName, NVSDK_NGX_Parameter_DLSS_Feature_Create_Flags))
  {
    if (config.nvidia.dlss.use_sharpening != -1)
    {
      if ((InValue & NVSDK_NGX_DLSS_Feature_Flags_DoSharpening) ==
                     NVSDK_NGX_DLSS_Feature_Flags_DoSharpening)
      {
        if (config.nvidia.dlss.use_sharpening == 0)
        {
          SK_LOGi0 (L"Forcing DLSS Sharpening OFF");

          InValue &= ~NVSDK_NGX_DLSS_Feature_Flags_DoSharpening;
        }
      }

      else
      {
        if (config.nvidia.dlss.use_sharpening == 1)
        {
          SK_LOGi0 (L"Forcing DLSS Sharpening ON");

          InValue |= NVSDK_NGX_DLSS_Feature_Flags_DoSharpening;
        }
      }
    }
  }

  if (! _stricmp (InName, NVSDK_NGX_Parameter_PerfQualityValue))
  {
    SK_NGX_GameSetPerfQuality = InValue;

    if (config.nvidia.dlss.force_dlaa)
    {
      if (SK_DLSS_Context::dlss_s::hasDLAAQualityLevel ())
      {
        InValue = NVSDK_NGX_PerfQuality_Value_DLAA;
      }
    }
  }

  NVSDK_NGX_Parameter_SetI_Original (InParameter, InName, InValue);
}

void
NVSDK_CONV
NVSDK_NGX_Parameter_SetUI_Detour (NVSDK_NGX_Parameter* InParameter, const char* InName, unsigned int InValue)
{
  SK_LOG_FIRST_CALL

  SK_LOGi1 (L"NGX_Parameter_SetUI (%hs, %u) - %ws", InName, InValue, SK_GetCallerName ().c_str ());

  if (! _stricmp (InName, NVSDK_NGX_Parameter_PerfQualityValue))
  {
    SK_NGX_GameSetPerfQuality = InValue;

    if (config.nvidia.dlss.force_dlaa)
    {
      if (SK_DLSS_Context::dlss_s::hasDLAAQualityLevel ())
      {
        InValue = NVSDK_NGX_PerfQuality_Value_DLAA;
      }
    }

    if (config.nvidia.dlss.forced_preset != -1)
    {
      unsigned int dlss_mode = InValue;

      unsigned int preset =
        static_cast <unsigned int> (config.nvidia.dlss.forced_preset);

      const char *szPresetHint = NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_DLAA;

      switch (dlss_mode)
      {
        case NVSDK_NGX_PerfQuality_Value_MaxPerf:           szPresetHint = NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Performance;      break;
        case NVSDK_NGX_PerfQuality_Value_Balanced:          szPresetHint = NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Balanced;         break;
        case NVSDK_NGX_PerfQuality_Value_MaxQuality:        szPresetHint = NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Quality;          break;
        // Extended PerfQuality modes                                  
        case NVSDK_NGX_PerfQuality_Value_UltraPerformance:  szPresetHint = NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_UltraPerformance; break;
        case NVSDK_NGX_PerfQuality_Value_UltraQuality:      szPresetHint = NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_UltraQuality;     break;
        case NVSDK_NGX_PerfQuality_Value_DLAA:              szPresetHint = NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_DLAA;             break;
        default:
          break;
      }

      NVSDK_NGX_Parameter_SetUI_Original ((NVSDK_NGX_Parameter *)InParameter, szPresetHint, preset);
    }
  }

  if ((! _stricmp (InName, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_DLAA))        ||
      (! _stricmp (InName, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Quality))     ||
      (! _stricmp (InName, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Balanced))    ||
      (! _stricmp (InName, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Performance)) ||
      (! _stricmp (InName, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_UltraPerformance)))
  {
    if (config.nvidia.dlss.force_dlaa && (config.nvidia.dlss.forced_preset == -1))
    {
      InValue = NVSDK_NGX_DLSS_Hint_Render_Preset_F;
    }
  }

  NVSDK_NGX_Parameter_SetUI_Original (InParameter, InName, InValue);
}

NVSDK_NGX_Result
NVSDK_CONV
NVSDK_NGX_Parameter_GetVoidPointer_Detour (NVSDK_NGX_Parameter *InParameter, const char *InName, void **OutValue)
{
  SK_LOG_FIRST_CALL

  SK_LOGi1 (L"NGX_Parameter_GetVoidPointer (%hs) - %ws", InName, SK_GetCallerName ().c_str ());

  return
    NVSDK_NGX_Parameter_GetVoidPointer_Original (InParameter, InName, OutValue);
}

NVSDK_NGX_Result
NVSDK_CONV
NVSDK_NGX_Parameter_GetUI_Detour (NVSDK_NGX_Parameter *InParameter, const char *InName, unsigned int *OutValue)
{
  SK_LOG_FIRST_CALL

  auto ret =
    NVSDK_NGX_Parameter_GetUI_Original (InParameter, InName, OutValue);

  if (ret == NVSDK_NGX_Result_Success)
  {
    SK_LOGi1 (L"NGX_Parameter_GetUI (%hs) - %ws", InName, SK_GetCallerName ().c_str ());

    if (config.nvidia.dlss.force_dlaa)
    {
      if (! _stricmp (InName, NVSDK_NGX_Parameter_OutWidth))                           { NVSDK_NGX_Parameter_GetUI_Original (InParameter, NVSDK_NGX_Parameter_Width,     OutValue); }
      if (! _stricmp (InName, NVSDK_NGX_Parameter_OutHeight))                          { NVSDK_NGX_Parameter_GetUI_Original (InParameter, NVSDK_NGX_Parameter_Height,    OutValue); }
      if (! _stricmp (InName, NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Max_Render_Width))  { NVSDK_NGX_Parameter_GetUI_Detour   (InParameter, NVSDK_NGX_Parameter_OutWidth,  OutValue); *OutValue += 1; }
      if (! _stricmp (InName, NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Max_Render_Height)) { NVSDK_NGX_Parameter_GetUI_Detour   (InParameter, NVSDK_NGX_Parameter_OutHeight, OutValue); *OutValue += 1; }
      if (! _stricmp (InName, NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Min_Render_Width))  { NVSDK_NGX_Parameter_GetUI_Detour   (InParameter, NVSDK_NGX_Parameter_OutWidth,  OutValue); *OutValue -= 1; }
      if (! _stricmp (InName, NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Min_Render_Height)) { NVSDK_NGX_Parameter_GetUI_Detour   (InParameter, NVSDK_NGX_Parameter_OutHeight, OutValue); *OutValue -= 1; }
    }

    if (! _stricmp (InName, NVSDK_NGX_Parameter_PerfQualityValue))
    {
      *OutValue = SK_NGX_GameSetPerfQuality;
    }
  }

  return ret;
}

using NVSDK_NGX_UpdateFeature_pfn =
      NVSDK_NGX_Result (NVSDK_CONV *)( const NVSDK_NGX_Application_Identifier *ApplicationId,
                                       const NVSDK_NGX_Feature                 FeatureID );

static NVSDK_NGX_UpdateFeature_pfn
       NVSDK_NGX_UpdateFeature_Original = nullptr;

NVSDK_NGX_Result
NVSDK_CONV
NVSDK_NGX_UpdateFeature_Detour ( const NVSDK_NGX_Application_Identifier *ApplicationId,
                                 const NVSDK_NGX_Feature                 FeatureID )
{
  SK_LOG_FIRST_CALL

  NVSDK_NGX_Result ret =
    NVSDK_NGX_UpdateFeature_Original (ApplicationId, FeatureID);

  if (ret != NVSDK_NGX_Result_Success)
  {
    SK_LOGi0 (
      L"NVSDK_NGX_UpdateFeature (..., %d, ...) Failed - %x (%ws)",
        FeatureID, ret, GetNGXResultAsString != nullptr ?
                        GetNGXResultAsString (ret)      :
                        L"Unknown Result"
    );
  }

  return ret;
}

using NVSDK_NGX_DLSS_GetStatsCallback_pfn           = NVSDK_NGX_Result (NVSDK_CONV *)(NVSDK_NGX_Parameter *InParams);
using NVSDK_NGX_DLSS_GetOptimalSettingsCallback_pfn = NVSDK_NGX_Result (NVSDK_CONV *)(NVSDK_NGX_Parameter *InParams);

NVSDK_NGX_Result NVSDK_CONV NVSDK_NGX_DLSS_GetOptimalSettingsCallback (NVSDK_NGX_Parameter* InParams);
NVSDK_NGX_Result NVSDK_CONV NVSDK_NGX_DLSS_GetStatsCallback           (NVSDK_NGX_Parameter* InParams);

NVSDK_NGX_Parameter*
SK_NGX_GetDLSSParameters (void)
{
  if (     SK_NGX_DLSS12.apis_called)
    return SK_NGX_DLSS12.super_sampling.Parameters;

  return
    SK_NGX_DLSS11.super_sampling.Parameters;
}

bool
SK_NGX_IsUsingDLSS (void)
{
  if (                     SK_NGX_DLSS12.apis_called)
    return                 SK_NGX_DLSS12.super_sampling.Handle     != nullptr &&
      ReadULong64Acquire (&SK_NGX_DLSS12.super_sampling.LastFrame) >= SK_GetFramesDrawn () - 8;

  return                 SK_NGX_DLSS11.super_sampling.Handle     != nullptr &&
    ReadULong64Acquire (&SK_NGX_DLSS11.super_sampling.LastFrame) >= SK_GetFramesDrawn () - 8;
}

bool
SK_NGX_IsUsingDLSS_D (void)
{
  return                 SK_NGX_DLSS12.super_sampling.Handle     != nullptr &&
    ReadULong64Acquire (&SK_NGX_DLSS12.super_sampling.LastFrame) >= SK_GetFramesDrawn () - 8 &&
                         SK_NGX_DLSS12.super_sampling.DLSS_Type == NVSDK_NGX_Feature_RayReconstruction;
}

bool
SK_NGX_IsUsingDLSS_G (void)
{
  return /// TODO Refactor
    __SK_IsDLSSGActive;
}

SK_DLSS_Context::version_s SK_DLSS_Context::dlss_s::Version;
SK_DLSS_Context::version_s SK_DLSS_Context::dlssg_s::Version;

void
SK_NGX_EstablishDLSSVersion (void) noexcept
{
  static bool bHasVersion = false;

  if (bHasVersion)
    return;

  if (! GetModuleHandleW (L"nvngx_dlss.dll"))
    return;

  std::swscanf (
    SK_GetDLLVersionShort (L"nvngx_dlss.dll").c_str (), L"%d,%d,%d,%d",
      &SK_DLSS_Context::dlss_s::Version.major, &SK_DLSS_Context::dlss_s::Version.minor,
      &SK_DLSS_Context::dlss_s::Version.build, &SK_DLSS_Context::dlss_s::Version.revision
  );

  bHasVersion = true;
}

SK_DLSS_Context::version_s
SK_NGX_GetDLSSVersion (void)
{
  if (     SK_NGX_DLSS12.apis_called)
    return SK_NGX_DLSS12.super_sampling.Version;

  return SK_NGX_DLSS11.super_sampling.Version;
}

bool
SK_NGX_HookParameters (NVSDK_NGX_Parameter* Params)
{
  if (Params == nullptr)
    return false;

  if (NVSDK_NGX_Parameter_GetUI_Original != nullptr)
    return true;

  void** vftable = *(void***)*&Params;

  SK_CreateFuncHook (      L"NVSDK_NGX_Parameter::GetUI",
                             vftable [12],
                             NVSDK_NGX_Parameter_GetUI_Detour,
    static_cast_p2p <void> (&NVSDK_NGX_Parameter_GetUI_Original) );

  SK_CreateFuncHook (      L"NVSDK_NGX_Parameter::GetVoidPointer",
                             vftable [8],
                             NVSDK_NGX_Parameter_GetVoidPointer_Detour,
    static_cast_p2p <void> (&NVSDK_NGX_Parameter_GetVoidPointer_Original) );

  SK_CreateFuncHook (      L"NVSDK_NGX_Parameter::SetF",
                             vftable [6],
                             NVSDK_NGX_Parameter_SetF_Detour,
    static_cast_p2p <void> (&NVSDK_NGX_Parameter_SetF_Original) );

  SK_CreateFuncHook (      L"NVSDK_NGX_Parameter::SetI",
                             vftable [3],
                             NVSDK_NGX_Parameter_SetI_Detour,
    static_cast_p2p <void> (&NVSDK_NGX_Parameter_SetI_Original) );

  SK_CreateFuncHook (      L"NVSDK_NGX_Parameter::SetUI",
                             vftable [4],
                             NVSDK_NGX_Parameter_SetUI_Detour,
    static_cast_p2p <void> (&NVSDK_NGX_Parameter_SetUI_Original) );

  MH_QueueEnableHook (vftable [12]);
  MH_QueueEnableHook (vftable [8]);
  MH_QueueEnableHook (vftable [6]);
  MH_QueueEnableHook (vftable [3]);
  MH_QueueEnableHook (vftable [4]);

  SK_ApplyQueuedHooks ();

  return true;
}

void
SK_NGX_DLSS_CreateFeatureOverrideParams (NVSDK_NGX_Parameter *InParameters)
{
  int                                                                create_flags = 0x0;
  InParameters->Get (NVSDK_NGX_Parameter_DLSS_Feature_Create_Flags, &create_flags);

  // Trigger our hook in case we missed the setup of creation flags earlier
  NVSDK_NGX_Parameter_SetI_Detour (InParameters, NVSDK_NGX_Parameter_DLSS_Feature_Create_Flags, create_flags);

  if (config.nvidia.dlss.use_sharpening == 1)
    InParameters->Set (NVSDK_NGX_Parameter_Sharpness, config.nvidia.dlss.forced_sharpness);

  //InParameters->Set (NVSDK_NGX_Parameter_PerfQualityValue, NVSDK_NGX_PerfQuality_Value_DLAA);
  //InParameters->Set (NVSDK_NGX_Parameter_RTXValue,         false);
  //InParameters->Set (NVSDK_NGX_Parameter_Sharpness,        1.0f);

  if (config.nvidia.dlss.force_dlaa)
  {
    unsigned int preset =
      NVSDK_NGX_DLSS_Hint_Render_Preset_F;

    NVSDK_NGX_Parameter_SetUI_Original (InParameters, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_DLAA,             preset);
    NVSDK_NGX_Parameter_SetUI_Original (InParameters, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Quality,          preset);
    NVSDK_NGX_Parameter_SetUI_Original (InParameters, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Balanced,         preset);
    NVSDK_NGX_Parameter_SetUI_Original (InParameters, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Performance,      preset);
    NVSDK_NGX_Parameter_SetUI_Original (InParameters, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_UltraPerformance, preset);

    if (SK_DLSS_Context::dlss_s::hasDLAAQualityLevel ())
    {
      int perf_quality =
        NVSDK_NGX_PerfQuality_Value_DLAA;

      NVSDK_NGX_Parameter_SetI_Original (InParameters, NVSDK_NGX_Parameter_PerfQualityValue, perf_quality);
    }
  }
}

extern void SK_NGX_InitD3D11 (void);
extern void SK_NGX_InitD3D12 (void);

extern void SK_NGX12_UpdateDLSSGStatus (void);
extern void SK_NGX11_UpdateDLSSGStatus (void);

void
SK_NGX_UpdateDLSSGStatus (void)
{
  SK_NGX11_UpdateDLSSGStatus ();
  SK_NGX12_UpdateDLSSGStatus ();
}

void
SK_NGX_Init (void)
{
  SK_RunOnce (
  {
    GetNGXResultAsString =
   (GetNGXResultAsString_pfn)SK_GetProcAddress (L"_nvngx.dll",
   "GetNGXResultAsString");

    using NVSDK_NGX_D3D12_GetParameters_pfn = NVSDK_NGX_Result (NVSDK_CONV *)(NVSDK_NGX_Parameter **OutParameters);

    SK_NGX_InitD3D11 ();
    SK_NGX_InitD3D12 ();

    SK_CreateDLLHook2 (L"_nvngx.dll",
                         "NVSDK_NGX_UpdateFeature",
                          NVSDK_NGX_UpdateFeature_Detour,
                (void **)&NVSDK_NGX_UpdateFeature_Original);

    auto _GetParameters =
      (NVSDK_NGX_D3D12_GetParameters_pfn)SK_GetProcAddress (L"_nvngx.dll",
                                                             "NVSDK_NGX_D3D12_GetParameters");

    if (NVSDK_NGX_Parameter *Params = nullptr;
                         _GetParameters != nullptr)
    {
      if (NVSDK_NGX_Result_Success == _GetParameters (&Params) && Params != nullptr)
      {
        // If successful, this applies the hooks queued in InitD3D11 and InitD3D12
        if (SK_NGX_HookParameters (Params))
          return;
      }
    }

    // HookParameters was not successful -- apply queued hooks before returning.
    SK_ApplyQueuedHooks ();
  });
}



void
SK_NGX_DLSS_ControlPanel (void)
{
  if (SK_NGX_IsUsingDLSS () || SK_NGX_IsUsingDLSS_G ())
  {
    ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.68f, 0.02f, 0.45f));
    ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.72f, 0.07f, 0.80f));
    ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.78f, 0.14f, 0.80f));
    ImGui::TreePush       ("");

    if (ImGui::CollapsingHeader ("NVIDIA DLSS", ImGuiTreeNodeFlags_DefaultOpen))
    {
      ImGui::SameLine ();

      const bool bFrameGen = SK_NGX_IsUsingDLSS_G ();
      const bool bRayRecon = SK_NGX_IsUsingDLSS_D ();

      ImGui::TextColored     ( bFrameGen ? ImVec4 (0.0f, 0.8f, 0.0f, 1.0f) : ImVec4 (0.8f, 0.0f, 0.0f, 1.0f),
                               bFrameGen ? "     " ICON_FA_CHECK   : "     " ICON_FA_XMARK );
      ImGui::SameLine        ();
      ImGui::TextUnformatted ("Frame Generation\t");
      ImGui::SameLine        ();
      ImGui::TextColored     ( bRayRecon ? ImVec4 (0.0f, 0.8f, 0.0f, 1.0f) : ImVec4 (0.8f, 0.0f, 0.0f, 1.0f),
                               bRayRecon ? ICON_FA_CHECK                   : ICON_FA_XMARK );
      ImGui::SameLine        ();
      ImGui::TextUnformatted ("Ray Reconstruction");

      ImGui::TreePush     ("");

      auto params =
        SK_NGX_GetDLSSParameters ();
  
      if (params != nullptr)
      {
        static auto path_to_plugin_dlss =
          std::filesystem::path (
            SK_GetPlugInDirectory (SK_PlugIn_Type::ThirdParty)
          ) / L"NVIDIA" / L"nvngx_dlss.dll";

        static std::error_code                          ec;
        static bool bHasPlugInDLSS =
          std::filesystem::exists (path_to_plugin_dlss, ec);

        static std::wstring path_to_dlss = bHasPlugInDLSS  ?
                            path_to_plugin_dlss.wstring () :
          SK_GetModuleFullName (SK_GetModuleHandleW (L"nvngx_dlss.dll"));

        static std::filesystem::path dlss_directory =
          std::filesystem::path (path_to_dlss).remove_filename ();

        static auto dlss_version =
          SK_NGX_GetDLSSVersion ();

        // Removed in 2.5.1
        static const bool bHasSharpening =
          SK_DLSS_Context::dlss_s::hasSharpening ();

        static const bool bHasDLAAQualityLevel =
          SK_DLSS_Context::dlss_s::hasDLAAQualityLevel ();

        using NVSDK_NGX_Parameter_SetF_pfn           = void             (NVSDK_CONV *)(NVSDK_NGX_Parameter *InParameter, const char* InName,          float InValue);
        using NVSDK_NGX_Parameter_SetI_pfn           = void             (NVSDK_CONV *)(NVSDK_NGX_Parameter *InParameter, const char* InName,          int   InValue);
        using NVSDK_NGX_Parameter_SetUI_pfn          = void             (NVSDK_CONV *)(NVSDK_NGX_Parameter *InParameter, const char* InName, unsigned int   InValue);
        using NVSDK_NGX_Parameter_GetUI_pfn          = NVSDK_NGX_Result (NVSDK_CONV *)(NVSDK_NGX_Parameter *InParameter, const char* InName, unsigned int* OutValue);
        using NVSDK_NGX_Parameter_GetVoidPointer_pfn = NVSDK_NGX_Result (NVSDK_CONV *)(NVSDK_NGX_Parameter *InParameter, const char* InName, void**        OutValue);

        extern NVSDK_NGX_Parameter_SetF_pfn  NVSDK_NGX_Parameter_SetF_Original;
        extern NVSDK_NGX_Parameter_SetI_pfn  NVSDK_NGX_Parameter_SetI_Original;
        extern NVSDK_NGX_Parameter_SetUI_pfn NVSDK_NGX_Parameter_SetUI_Original;
        extern NVSDK_NGX_Parameter_GetUI_pfn NVSDK_NGX_Parameter_GetUI_Original;

        static bool restart_required = false;
  
        unsigned int     width,     height;
        unsigned int out_width, out_height;
  
        NVSDK_NGX_Parameter_GetUI_Original (params, NVSDK_NGX_Parameter_Width,     &width);
        NVSDK_NGX_Parameter_GetUI_Original (params, NVSDK_NGX_Parameter_Height,    &height);
        NVSDK_NGX_Parameter_GetUI_Original (params, NVSDK_NGX_Parameter_OutWidth,  &out_width);
        NVSDK_NGX_Parameter_GetUI_Original (params, NVSDK_NGX_Parameter_OutHeight, &out_height);

        out_width  = std::max ( width,  out_width);
        out_height = std::max (height, out_height);

        //
        // Get the ACTUAL internal resolution by querying the underlying Direct3D resources
        //   and using their desc. Many games are not setting the Height/Width parameters.
        //
        ID3D12Resource                           *pD3D12Resource = nullptr;
        params->Get (NVSDK_NGX_Parameter_Color, &pD3D12Resource);
        ID3D11Resource                           *pD3D11Resource = nullptr;
        params->Get (NVSDK_NGX_Parameter_Color, &pD3D11Resource);

        if (pD3D12Resource != nullptr)
        {
          width  = sk::narrow_cast <unsigned int> (pD3D12Resource->GetDesc ().Width);
          height = sk::narrow_cast <unsigned int> (pD3D12Resource->GetDesc ().Height);
        }

        else if (pD3D11Resource != nullptr)
        {
          SK_ComQIPtr <ID3D11Texture2D> pTex2D (pD3D11Resource);

          if (pTex2D != nullptr)
          {
            D3D11_TEXTURE2D_DESC texDesc = { };
            pTex2D->GetDesc    (&texDesc);

            width  = texDesc.Width;
            height = texDesc.Height;
          }
        }
  
        unsigned int perf_quality = NVSDK_NGX_PerfQuality_Value_MaxPerf;
  
        NVSDK_NGX_Parameter_GetUI_Original (params, NVSDK_NGX_Parameter_PerfQualityValue, &perf_quality);
  
        ImGui::BeginGroup ();
        ImGui::BeginGroup ();
        ImGui::
          TextUnformatted ("Internal Resolution: ");
        ImGui::
          TextUnformatted ("Upscaled Resolution: ");
        ImGui::
          TextUnformatted ("DLSS Preset:         ");
        ImGui::
          TextUnformatted ("DLSS Perf/Quality:   ");
        ImGui::EndGroup   ();
        ImGui::SameLine   ();
        ImGui::BeginGroup ();
        ImGui::Text       ("%dx%d",     width,     height);
        ImGui::Text       ("%dx%d", out_width, out_height);

        unsigned int preset;

        const char *szPresetHint = NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_DLAA;

        switch (perf_quality)
        {
          case NVSDK_NGX_PerfQuality_Value_MaxPerf:           szPresetHint = NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Performance;      break;
          case NVSDK_NGX_PerfQuality_Value_Balanced:          szPresetHint = NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Balanced;         break;
          case NVSDK_NGX_PerfQuality_Value_MaxQuality:        szPresetHint = NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Quality;          break;
          // Extended PerfQuality modes                                  
          case NVSDK_NGX_PerfQuality_Value_UltraPerformance:  szPresetHint = NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_UltraPerformance; break;
          case NVSDK_NGX_PerfQuality_Value_UltraQuality:      szPresetHint = NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_UltraQuality;     break;
          case NVSDK_NGX_PerfQuality_Value_DLAA:              szPresetHint = NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_DLAA;             break;
          default:
            break;
        }

        NVSDK_NGX_Parameter_GetUI_Original (params, szPresetHint, &preset);

        const char *szPreset = "Default";

        switch (preset)
        {
          case NVSDK_NGX_DLSS_Hint_Render_Preset_Default: szPreset = "Default"; break;
          case NVSDK_NGX_DLSS_Hint_Render_Preset_A:       szPreset = "A";       break;
          case NVSDK_NGX_DLSS_Hint_Render_Preset_B:       szPreset = "B";       break;
          case NVSDK_NGX_DLSS_Hint_Render_Preset_C:       szPreset = "C";       break;
          case NVSDK_NGX_DLSS_Hint_Render_Preset_D:       szPreset = "D";       break;
          case NVSDK_NGX_DLSS_Hint_Render_Preset_E:       szPreset = "E";       break;
          case NVSDK_NGX_DLSS_Hint_Render_Preset_F:       szPreset = "F";       break;
          case NVSDK_NGX_DLSS_Hint_Render_Preset_G:       szPreset = "G";       break;
          default:
            break;
        }

        ImGui::TextUnformatted (szPreset);
  
        ImGui::BeginGroup ();
        if (config.nvidia.dlss.force_dlaa && (! SK_DLSS_Context::dlss_s::hasDLAAQualityLevel ()))
        {
          ImGui::TextColored (ImVec4 (.55f, .55f, 1.f, 1.f), ICON_FA_INFO);
          ImGui::SameLine ();
        }

        switch (perf_quality)
        {
          case NVSDK_NGX_PerfQuality_Value_MaxPerf:
            ImGui::TextUnformatted ("Maximum Performance");
            break;
          case NVSDK_NGX_PerfQuality_Value_Balanced:
            ImGui::TextUnformatted ("Balanced");
            break;
          case NVSDK_NGX_PerfQuality_Value_MaxQuality:
            ImGui::TextUnformatted ("Maximum Quality");
            break;
          case NVSDK_NGX_PerfQuality_Value_UltraPerformance:
            ImGui::TextUnformatted ("Ultra Performance");
            break;
          case NVSDK_NGX_PerfQuality_Value_UltraQuality:
            ImGui::TextUnformatted ("Ultra Quality");
            break;
          case NVSDK_NGX_PerfQuality_Value_DLAA:
            ImGui::TextUnformatted ("DLAA");
            break;
          default:
            ImGui::TextUnformatted ("Unknown Performance/Quality Mode");
            break;
        }
        ImGui::EndGroup ();

        if (config.nvidia.dlss.force_dlaa && (! SK_DLSS_Context::dlss_s::hasDLAAQualityLevel ()))
        {
          if (ImGui::IsItemHovered ())
          {
            ImGui::SetTooltip (
              "The DLSS version in-use does not have a DLAA Pref/Quality level, "
              "but DLAA is active if Internal/Upscaled resolution are equal."
            );
          }
        }

        ImGui::SameLine ();

        if (ImGui::Checkbox ("Force DLAA", &config.nvidia.dlss.force_dlaa))
        {
          restart_required = true;

          SK_SaveConfig ();
        }
        
        if (ImGui::IsItemHovered ())
        {
          if (bHasDLAAQualityLevel)
            ImGui::SetTooltip ("For best results, set game's DLSS mode = Auto/Ultra Performance if it has them.");
          else
            ImGui::SetTooltip ("For best results, upgrade DLSS DLL to 3.1.13 or newer.");
        }

        ImGui::EndGroup ();

#if 0
        int preset_override = config.nvidia.dlss.forced_preset + 1;

        if ( ImGui::Combo ( "Preset Override",
                            &preset_override, "N/A\0"
                                              "Default\0"
                                              "A\0"
                                              "B\0"
                                              "C\0"
                                              "D\0"
                                              "E\0"
                                              "F\0"
                                              "G\0" )
           )
        {
          config.nvidia.dlss.forced_preset = preset_override - 1;

          if (config.nvidia.dlss.forced_preset != -1)
          {
            NVSDK_NGX_Parameter_SetUI_Original (params, szPresetHint, config.nvidia.dlss.forced_preset);
          }

          restart_required = true;

          SK_SaveConfig ();
        }
#endif
  
        if (bHasSharpening)
        {
          float                                        fSharpness;
          params->Get (NVSDK_NGX_Parameter_Sharpness, &fSharpness);
  
          int use_sharpening =
            config.nvidia.dlss.use_sharpening + 1;
  
          if ( ImGui::Combo (
                 "Sharpening",
             &use_sharpening, "Game Default\0"
                              "Force Off\0"
                              "Force On\0\0") )
          {
            config.nvidia.dlss.use_sharpening =
              use_sharpening - 1;
            restart_required = true;

            SK_SaveConfig ();
          }
  
          int                                               dlss_creation_flags = 0x0;
          params->Get (
            NVSDK_NGX_Parameter_DLSS_Feature_Create_Flags, &dlss_creation_flags);
  
          if (use_sharpening == 0 && (dlss_creation_flags & NVSDK_NGX_DLSS_Feature_Flags_DoSharpening))
            ImGui::Text ("Sharpness: %4.2f", fSharpness);
          else if (use_sharpening == 2)
          {
            fSharpness = config.nvidia.dlss.forced_sharpness;

            if (ImGui::SliderFloat ("Sharpness", &fSharpness, -1.0f, 1.0f))
            {
                    config.nvidia.dlss.forced_sharpness = fSharpness;
              params->Set (NVSDK_NGX_Parameter_Sharpness, fSharpness);

              restart_required = true;

              SK_SaveConfig ();
            }
          }
        }
  
        if (restart_required)
        {
          ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (.3f, .8f, .9f).Value);
          ImGui::BulletText     ("Game Restart May Be Required");
          ImGui::PopStyleColor  ();
        }
        ImGui::EndGroup   ();
        ImGui::SameLine   ();
        ImGui::BeginGroup ();

        ImGui::Text ( "DLSS Version:\t%d.%d.%d", dlss_version.major, dlss_version.minor,
                                                 dlss_version.build/*, dlss_version.revision*/ );

        static bool bRestartNeeded = false;

        if (bHasPlugInDLSS)
        {
          if (ImGui::Checkbox ("Use Special K's DLSS instead of Game's DLL", &config.nvidia.dlss.auto_redirect_dlss))
          {
            bRestartNeeded = true;

            SK_SaveConfig ();
          }
        }

        else
        {
          static bool clicked_once = false;

          bool bClicked =
            ImGui::Selectable (ICON_FA_INFO_CIRCLE " Auto-Load a Newer DLSS DLL...");

          clicked_once |= bClicked;

          if (ImGui::IsItemHovered ())
          {
            ImGui::BeginTooltip ();
            ImGui::BulletText   ("Click to Create the Plug-In Directory for Auto-Load.");
            ImGui::BulletText   ("Place nvngx_dlss.dll in the Directory.");
            ImGui::EndTooltip   ();
          }

          if (bClicked)
          {
            static std::filesystem::path dlss_plugin_directory =
                   std::filesystem::path (path_to_plugin_dlss).remove_filename ();

            SK_CreateDirectories (       dlss_plugin_directory.c_str ());
            if (std::filesystem::exists (dlss_plugin_directory, ec))
              SK_Util_ExplorePath (      dlss_plugin_directory.wstring ().c_str ());
            else
            {
              clicked_once = false;
            }
          }

          if (clicked_once)
          {
            bHasPlugInDLSS =
              std::filesystem::exists (path_to_plugin_dlss, ec);
          }
        }

        if (bRestartNeeded)
        {
          ImGui::PushStyleColor (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (.1f, .8f, .9f));
          ImGui::BulletText     ("Game Restart Required");
          ImGui::PopStyleColor  ();
        }

        if ((! bHasPlugInDLSS) || (! config.nvidia.dlss.auto_redirect_dlss))
        {
          if (ImGui::Button ("Browse DLSS Directory"))
          {
            SK_Util_ExplorePath (dlss_directory.wstring ().c_str ());
          }
        }
        ImGui::EndGroup   ();
      }

      ImGui::TreePop     ( );
    }
  
    ImGui::TreePop       ( );
    ImGui::PopStyleColor (3);
  }
}