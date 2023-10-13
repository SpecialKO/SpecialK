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

    else if (config.nvidia.dlss.forced_preset != -1)
    {
      InValue = config.nvidia.dlss.forced_preset;
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
      if (! _stricmp (InName, NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Max_Render_Width))  { NVSDK_NGX_Parameter_GetUI_Detour   (InParameter, NVSDK_NGX_Parameter_OutWidth,  OutValue); *OutValue += 2; }
      if (! _stricmp (InName, NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Max_Render_Height)) { NVSDK_NGX_Parameter_GetUI_Detour   (InParameter, NVSDK_NGX_Parameter_OutHeight, OutValue); *OutValue += 2; }
      if (! _stricmp (InName, NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Min_Render_Width))  { NVSDK_NGX_Parameter_GetUI_Detour   (InParameter, NVSDK_NGX_Parameter_OutWidth,  OutValue); *OutValue -= 2; }
      if (! _stricmp (InName, NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Min_Render_Height)) { NVSDK_NGX_Parameter_GetUI_Detour   (InParameter, NVSDK_NGX_Parameter_OutHeight, OutValue); *OutValue -= 2; }
    }

    else if (StrStrIA (InName, "Width") || StrStrIA (InName, "Height"))
    {
      unsigned int dlss_perf_qual;

      InParameter->Get (NVSDK_NGX_Parameter_PerfQualityValue, &dlss_perf_qual);

      float scale         = 0.0f;
      float default_scale = 1.0f;

      switch (dlss_perf_qual)
      {
        case NVSDK_NGX_PerfQuality_Value_MaxPerf:          scale = config.nvidia.dlss.scale.performance;       default_scale = 0.5f;      break;
        case NVSDK_NGX_PerfQuality_Value_Balanced:         scale = config.nvidia.dlss.scale.balanced;          default_scale = 0.58f;     break;
        case NVSDK_NGX_PerfQuality_Value_MaxQuality:       scale = config.nvidia.dlss.scale.quality;           default_scale = 0.666667f; break;
        case NVSDK_NGX_PerfQuality_Value_UltraPerformance: scale = config.nvidia.dlss.scale.ultra_performance; default_scale = 0.333333f; break;
        default:
          break;
      }

      if (scale != 0.0f)
      {
        if (! _stricmp (InName, NVSDK_NGX_Parameter_OutWidth))  { NVSDK_NGX_Parameter_GetUI_Original (InParameter, NVSDK_NGX_Parameter_Width,  OutValue); *OutValue = sk::narrow_cast <UINT> (*OutValue * scale); NVSDK_NGX_Parameter_SetUI_Original (InParameter, NVSDK_NGX_Parameter_OutWidth,  *OutValue); }
        if (! _stricmp (InName, NVSDK_NGX_Parameter_OutHeight)) { NVSDK_NGX_Parameter_GetUI_Original (InParameter, NVSDK_NGX_Parameter_Height, OutValue); *OutValue = sk::narrow_cast <UINT> (*OutValue * scale); NVSDK_NGX_Parameter_SetUI_Original (InParameter, NVSDK_NGX_Parameter_OutHeight, *OutValue); }

        if (! _stricmp (InName, NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Max_Render_Width))  { NVSDK_NGX_Parameter_GetUI_Detour (InParameter, NVSDK_NGX_Parameter_OutWidth,  OutValue); *OutValue = sk::narrow_cast <UINT> (*OutValue * config.nvidia.dlss.scale.dynamic_max) + 2; }
        if (! _stricmp (InName, NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Max_Render_Height)) { NVSDK_NGX_Parameter_GetUI_Detour (InParameter, NVSDK_NGX_Parameter_OutHeight, OutValue); *OutValue = sk::narrow_cast <UINT> (*OutValue * config.nvidia.dlss.scale.dynamic_max) + 2; }
        if (! _stricmp (InName, NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Min_Render_Width))  { NVSDK_NGX_Parameter_GetUI_Detour (InParameter, NVSDK_NGX_Parameter_OutWidth,  OutValue); *OutValue = sk::narrow_cast <UINT> (*OutValue * config.nvidia.dlss.scale.dynamic_min) - 2; }
        if (! _stricmp (InName, NVSDK_NGX_Parameter_DLSS_Get_Dynamic_Min_Render_Height)) { NVSDK_NGX_Parameter_GetUI_Detour (InParameter, NVSDK_NGX_Parameter_OutHeight, OutValue); *OutValue = sk::narrow_cast <UINT> (*OutValue * config.nvidia.dlss.scale.dynamic_min) - 2; }
      }
    }

    if (! _stricmp (InName, NVSDK_NGX_Parameter_PerfQualityValue))
    {
      *OutValue = SK_NGX_GameSetPerfQuality;
    }

    if (config.nvidia.dlss.forced_preset != -1 && StrStrIA (InName, "DLSS.Hint.Render.Preset."))
    {
      *OutValue = config.nvidia.dlss.forced_preset;
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

  // Turn off overrides before we break stuff!
  if (SK_DLSS_Context::dlss_s::Version.major < 2)
  {
    config.nvidia.dlss.auto_redirect_dlss = false;
    config.nvidia.dlss.forced_preset      = -1;
    config.nvidia.dlss.use_sharpening     = -1;
    config.nvidia.dlss.force_dlaa         = false;
  }
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

  if (config.nvidia.dlss.force_dlaa)
  {
    if (config.nvidia.dlss.forced_preset == -1)
    {
      unsigned int preset =
        NVSDK_NGX_DLSS_Hint_Render_Preset_F;

      NVSDK_NGX_Parameter_SetUI_Original (InParameters, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_DLAA,             preset);
      NVSDK_NGX_Parameter_SetUI_Original (InParameters, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Quality,          preset);
      NVSDK_NGX_Parameter_SetUI_Original (InParameters, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Balanced,         preset);
      NVSDK_NGX_Parameter_SetUI_Original (InParameters, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Performance,      preset);
      NVSDK_NGX_Parameter_SetUI_Original (InParameters, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_UltraPerformance, preset);
    }

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

DWORD SK_DLSS_Context::dlss_s::IndicatorFlags  = DWORD_MAX;
DWORD SK_DLSS_Context::dlssg_s::IndicatorFlags = DWORD_MAX;

void
SK_DLSS_Context::dlss_s::showIndicator (bool show)
{
  const wchar_t* wszPath =
    LR"(HKEY_LOCAL_MACHINE\SOFTWARE\NVIDIA Corporation\Global\NGXCore)";
  const wchar_t *wszKey  = L"ShowDlssIndicator";
  const wchar_t *wszType = L"dword";
  
  DWORD dwValue =
    ( show ? 0x400 : 0x0 );

  wchar_t   wszValue [64] = { };
  wsprintf (wszValue, L"%08x", dwValue);

  SK_ImportRegistryValue (wszPath, wszKey, wszType, wszValue, true);

  SK_DLSS_Context::dlss_s::IndicatorFlags = dwValue;
}

bool
SK_DLSS_Context::dlss_s::isIndicatorShown (void)
{
  if (SK_DLSS_Context::dlss_s::IndicatorFlags != DWORD_MAX)
  {
    return
      (SK_DLSS_Context::dlss_s::IndicatorFlags & 0x400) == 0x400;
  }

  DWORD     len    =      MAX_PATH;
  LSTATUS   status =
    RegGetValueW ( HKEY_LOCAL_MACHINE,
                     LR"(SOFTWARE\NVIDIA Corporation\Global\NGXCore\)",
                                      L"ShowDlssIndicator",
                       RRF_RT_REG_DWORD,
                         nullptr,
                           (PVOID)&SK_DLSS_Context::dlss_s::IndicatorFlags,
                             (LPDWORD)&len );

  if (status == ERROR_SUCCESS)
    return (SK_DLSS_Context::dlss_s::IndicatorFlags & 0x400) == 0x400;
  else
  {
    // Don't call this repeatedly on failure
    SK_DLSS_Context::dlss_s::IndicatorFlags = 0x0;

    return false;
  }
}

void
SK_DLSS_Context::dlssg_s::showIndicator (bool show)
{
  const wchar_t* wszPath =
    LR"(HKEY_LOCAL_MACHINE\SOFTWARE\NVIDIA Corporation\Global\NGXCore)";
  const wchar_t *wszKey  = L"DLSSG_IndicatorText";
  const wchar_t *wszType = L"dword";

  DWORD dwValue =
    ( show ? 0x2 : 0x0 );

  wchar_t   wszValue [64] = { };
  wsprintf (wszValue, L"%08x", dwValue);

  SK_ImportRegistryValue (wszPath, wszKey, wszType, wszValue, true);

  SK_DLSS_Context::dlssg_s::IndicatorFlags = dwValue;
}

bool
SK_DLSS_Context::dlssg_s::isIndicatorShown (void)
{
  if (SK_DLSS_Context::dlssg_s::IndicatorFlags != DWORD_MAX)
  {
    return
      (SK_DLSS_Context::dlssg_s::IndicatorFlags & 0x2) == 0x2;
  }

  DWORD     len    =      MAX_PATH;
  LSTATUS   status =
    RegGetValueW ( HKEY_LOCAL_MACHINE,
                     LR"(SOFTWARE\NVIDIA Corporation\Global\NGXCore\)",
                                      L"DLSSG_IndicatorText",
                       RRF_RT_REG_DWORD,
                         nullptr,
                           (PVOID)&SK_DLSS_Context::dlssg_s::IndicatorFlags,
                             (LPDWORD)&len );

  if (status == ERROR_SUCCESS)
    return (SK_DLSS_Context::dlssg_s::IndicatorFlags & 0x2) == 0x2;
  else
  {
    // Don't call this repeatedly on failure
    SK_DLSS_Context::dlssg_s::IndicatorFlags = 0x0;

    return false;
  }
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
      if (config.nvidia.dlss.show_active_features)
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
      }

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

        static std::wstring path_to_dlss =
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
        ImGui::Spacing ();
        ImGui::
          TextUnformatted ("DLSS Preset:         ");
        ImGui::Spacing ();
        ImGui::
          TextUnformatted ("DLSS Perf/Quality:   ");
        if (! config.nvidia.dlss.force_dlaa)
        {
          if (ImGui::IsItemHovered ())
          {
            ImGui::SetTooltip ("Right-click to define custom resolution scales.");
          }

          if (ImGui::IsItemClicked (ImGuiPopupFlags_MouseButtonRight))
          {
            ImGui::OpenPopup ("DLSS_PerfQuality_Popup");
          }
        }
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

        const char *szPreset = "DLSS Default";

        switch (preset)
        {
          case NVSDK_NGX_DLSS_Hint_Render_Preset_Default: szPreset = "DLSS Default"; break;
          case NVSDK_NGX_DLSS_Hint_Render_Preset_A:       szPreset = "A";            break;
          case NVSDK_NGX_DLSS_Hint_Render_Preset_B:       szPreset = "B";            break;
          case NVSDK_NGX_DLSS_Hint_Render_Preset_C:       szPreset = "C";            break;
          case NVSDK_NGX_DLSS_Hint_Render_Preset_D:       szPreset = "D";            break;
          case NVSDK_NGX_DLSS_Hint_Render_Preset_E:       szPreset = "E";            break;
          case NVSDK_NGX_DLSS_Hint_Render_Preset_F:       szPreset = "F";            break;
          case NVSDK_NGX_DLSS_Hint_Render_Preset_G:       szPreset = "G";            break;
          default:
            break;
        }

        ImGui::BeginGroup      ();
        ImGui::Spacing         ();
        ImGui::TextUnformatted (szPreset);
        ImGui::EndGroup        ();

        ImGui::SameLine ();

        int preset_override = config.nvidia.dlss.forced_preset + 1;

        ImGui::SetNextItemWidth (
          ImGui::CalcTextSize ("DLSS Default\t  ").x + ImGui::GetStyle ().FramePadding.x * 2
        );

        if ( ImGui::Combo ( "",
                            &preset_override, "Game Default\0"
                                              "DLSS Default\0"
                                              "Override: A\0"
                                              "Override: B\0"
                                              "Override: C\0"
                                              "Override: D\0"
                                              "Override: E\0"
                                              "Override: F\0"
                                              "Override: G\0" )
           )
        {
          config.nvidia.dlss.forced_preset = preset_override - 1;

          if (config.nvidia.dlss.forced_preset != -1)
          {
            NVSDK_NGX_Parameter_SetUI_Original (params, szPresetHint, config.nvidia.dlss.forced_preset);
          }

          SK_NGX_Reset ();

          restart_required = true;

          SK_SaveConfig ();
        }
  
        ImGui::BeginGroup ();
        ImGui::Spacing    ();
        if (config.nvidia.dlss.force_dlaa && (! SK_DLSS_Context::dlss_s::hasDLAAQualityLevel ()))
        {
          ImGui::TextColored (ImVec4 (.55f, .55f, 1.f, 1.f), ICON_FA_INFO);
          ImGui::SameLine ();
        }

        switch (perf_quality)
        {
          case NVSDK_NGX_PerfQuality_Value_MaxPerf:
            ImGui::TextUnformatted ("Performance");
            break;
          case NVSDK_NGX_PerfQuality_Value_Balanced:
            ImGui::TextUnformatted ("Balanced");
            break;
          case NVSDK_NGX_PerfQuality_Value_MaxQuality:
            ImGui::TextUnformatted ("Quality");
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
              "The DLSS version in-use does not have a DLAA Perf/Quality level, "
              "but DLAA is active if Internal/Upscaled resolution are equal."
            );
          }
        }

        else if (! config.nvidia.dlss.force_dlaa)
        {
          if (ImGui::IsItemHovered ())
          {
            ImGui::SetTooltip ("Right-click to define custom resolution scales.");
          }

          if (ImGui::IsItemClicked (ImGuiPopupFlags_MouseButtonRight))
          {
            ImGui::OpenPopup ("DLSS_PerfQuality_Popup");
          }

          if (ImGui::BeginPopup ("DLSS_PerfQuality_Popup"))
          {
            static auto& rb =
              SK_GetCurrentRenderBackend ();

            SK_ComQIPtr <IDXGISwapChain>
               pSwapChain (rb.swapchain);

            DXGI_SWAP_CHAIN_DESC swapDesc = { };

            if (pSwapChain.p != nullptr)
                pSwapChain->GetDesc (&swapDesc);

            auto _ScaleOverride = [&](float &cfg_var, float fDefaultScale, const char *szName)
            {
              bool override = cfg_var != 0.0f;

              ImGui::PushID (szName);
              if (ImGui::Checkbox ("", &override))
              {
                if (override)
                  cfg_var = fDefaultScale;
                else
                  cfg_var = 0.0f;

                restart_required = true;

                SK_SaveConfig ();
              }

              ImGui::SameLine ();

              if (! override)
                ImGui::BeginDisabled ();

              float* scale = override ?
                             &cfg_var : &fDefaultScale;

              char      fmt [128] = { };
              sprintf ( fmt, "%%6.4f\t(%ux%u)", static_cast <UINT> (swapDesc.BufferDesc.Width  * *scale),
                                                static_cast <UINT> (swapDesc.BufferDesc.Height * *scale) );

              if (ImGui::SliderFloat (szName, scale, 0.01f, 0.999f, fmt))
              {
                restart_required = true;

                SK_SaveConfig ();
              }

              if (! override)
                ImGui::EndDisabled ();
              ImGui::PopID ();
            };

            ImGui::BeginGroup ();
            _ScaleOverride (config.nvidia.dlss.scale.ultra_performance, 0.333333f, "Ultra Performance");
            _ScaleOverride (config.nvidia.dlss.scale.performance,       0.5f,      "Performance");
            _ScaleOverride (config.nvidia.dlss.scale.balanced,          0.58f,     "Balanced");
            _ScaleOverride (config.nvidia.dlss.scale.quality,           0.666667f, "Quality");
            ImGui::EndGroup  ();
            ImGui::Separator ();

            ImGui::BulletText ("Dynamic Resolution Min/Max Applies only to Overridden Resolution Scales");
            ImGui::BulletText ("Set both to 100%% to disable Dynamic Resolution Scaling");
            ImGui::BulletText ("These settings do nothing if the game does not use DRS");

            float percent =
              config.nvidia.dlss.scale.dynamic_min * 100.0f;

            if (ImGui::SliderFloat ("Dynamic Resolution Minimum", &percent, 1.0f, 100.0f, "%6.2f%%"))
            {
              config.nvidia.dlss.scale.dynamic_min = percent / 100.0f;

              restart_required = true;

              SK_SaveConfig ();
            }

            percent =
              config.nvidia.dlss.scale.dynamic_max * 100.0f;

            if (ImGui::SliderFloat ("Dynamic Resolution Maximum", &percent, 100.0f, 200.0f, "%6.2f%%"))
            {
              config.nvidia.dlss.scale.dynamic_max = percent / 100.0f;

              restart_required = true;

              SK_SaveConfig ();
            }

            ImGui::EndPopup ();
          }
        }

        ImGui::SameLine ();

        if (dlss_version.major > 1)
        {
          if (ImGui::Checkbox ("Force DLAA", &config.nvidia.dlss.force_dlaa))
          {
            restart_required = true;

            SK_NGX_Reset ();

            SK_SaveConfig ();
          }

          if (ImGui::IsItemHovered ())
          {
            if (bHasDLAAQualityLevel)
              ImGui::SetTooltip ("For best results, set game's DLSS mode = Auto/Ultra Performance if it has them.");
            else
              ImGui::SetTooltip ("For best results, upgrade DLSS DLL to 3.1.13 or newer.");
          }
        }

        ImGui::EndGroup ();
  
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

              SK_NGX_Reset ();

              SK_SaveConfig ();
            }
          }
        }
  
        if (restart_required)
        {
          ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (.3f, .8f, .9f).Value);
          ImGui::BulletText     ("Game Restart (or Alt+Enter) May Be Required");
          ImGui::PopStyleColor  ();
        }
        ImGui::EndGroup   ();
        ImGui::SameLine   ();
        ImGui::BeginGroup ();

        ImGui::Text ( "DLSS Version:\t%d.%d.%d", dlss_version.major, dlss_version.minor,
                                                 dlss_version.build );

        static bool bRestartNeeded = false;

        // Only offer option to replace DLSS DLLs in DLSS 2.x games
        if (dlss_version.major > 1)
        {
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

          if ((! bHasPlugInDLSS) || (! config.nvidia.dlss.auto_redirect_dlss))
          {
            if (ImGui::Button ("Browse DLSS Directory"))
            {
              SK_Util_ExplorePath (dlss_directory.wstring ().c_str ());
            }

            ImGui::SameLine ();
          }
        }
        
        if (ImGui::TreeNode ("DLSS Indicators"))
        {
          ImGui::BeginGroup ();

          bool show_dlss =
            SK_DLSS_Context::dlss_s::isIndicatorShown ();
          bool show_dlssg =
            SK_DLSS_Context::dlssg_s::isIndicatorShown ();

          if (ImGui::Checkbox ("Show DLSS", &show_dlss))
          {
            SK_DLSS_Context::dlss_s::showIndicator (show_dlss);

            bRestartNeeded = true;
          }

          if (ImGui::IsItemHovered ())
          {
            ImGui::SetTooltip ("Shows the official NVIDIA text overlay (bottom-left) for DLSS / DLSS Ray Reconstruction.");
          }

          if (ImGui::Checkbox ("Show DLSS-G", &show_dlssg))
          {
            SK_DLSS_Context::dlssg_s::showIndicator (show_dlssg);

            bRestartNeeded = true;
          }

          if (ImGui::IsItemHovered ())
          {
            ImGui::SetTooltip ("Shows the official NVIDIA text overlay (top-left) for DLSS Frame Generation.");
          }

          ImGui::EndGroup    ();
          ImGui::SameLine    ();
          ImGui::SeparatorEx (ImGuiSeparatorFlags_Vertical);
          ImGui::SameLine    ();
          ImGui::BeginGroup  ();
          if (ImGui::Checkbox ("Show Active Features", &config.nvidia.dlss.show_active_features))
          {
            SK_SaveConfig ();
          }

          if (ImGui::IsItemHovered ())
          {
            ImGui::SetTooltip ("Displays which DLSS extensions are in use in the DLSS header.");
          }
        }
        else
          ImGui::BeginGroup ();

        if (bRestartNeeded)
        {
          ImGui::PushStyleColor (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (.1f, .8f, .9f));
          ImGui::BulletText     ("Game Restart Required");
          ImGui::PopStyleColor  ();
        }
        ImGui::EndGroup   ();
        ImGui::EndGroup   ();
      }

      ImGui::TreePop     ( );
    }
  
    ImGui::TreePop       ( );
    ImGui::PopStyleColor (3);
  }
}

void
SK_NGX_Reset (void)
{
  WriteULong64Release (&SK_NGX_DLSS11.super_sampling.ResetFrame, SK_GetFramesDrawn () + 1);
  WriteULong64Release (&SK_NGX_DLSS12.super_sampling.ResetFrame, SK_GetFramesDrawn () + 1);
}