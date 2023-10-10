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
        SK_LOGi1 (
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
      InValue = NVSDK_NGX_PerfQuality_Value_DLAA;
    }
  }

  NVSDK_NGX_Parameter_SetI_Original (InParameter, InName, InValue);
}

void
NVSDK_CONV
NVSDK_NGX_Parameter_SetUI_Detour (NVSDK_NGX_Parameter* InParameter, const char* InName, unsigned int InValue)
{
  SK_LOG_FIRST_CALL

  SK_LOGi1 (L"NGX_Parameter_SetUI (%hs, %ui) - %ws", InName, InValue, SK_GetCallerName ().c_str ());

  if (! _stricmp (InName, NVSDK_NGX_Parameter_PerfQualityValue))
  {
    SK_NGX_GameSetPerfQuality = InValue;

    if (config.nvidia.dlss.force_dlaa)
    {
      InValue = NVSDK_NGX_PerfQuality_Value_DLAA;
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
      if (! _stricmp (InName, NVSDK_NGX_Parameter_OutWidth))                           { NVSDK_NGX_Parameter_GetUI_Original (InParameter, NVSDK_NGX_Parameter_Width,     OutValue); /*SK_LOGi0 (L"NVSDK_NGX_Parameter_OutWidth=%d", *OutValue);*/  }
      if (! _stricmp (InName, NVSDK_NGX_Parameter_OutHeight))                          { NVSDK_NGX_Parameter_GetUI_Original (InParameter, NVSDK_NGX_Parameter_Height,    OutValue); /*SK_LOGi0 (L"NVSDK_NGX_Parameter_OutHeight=%d", *OutValue);*/ }
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
SK_NGX_IsUsingDLSS_RR (void)
{
  return                 SK_NGX_DLSS12.super_sampling.Handle     != nullptr &&
    ReadULong64Acquire (&SK_NGX_DLSS12.super_sampling.LastFrame) >= SK_GetFramesDrawn () - 8 &&
                         SK_NGX_DLSS12.super_sampling.DLSS_Type == NVSDK_NGX_Feature_RayReconstruction;
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

    int perf_quality =
      NVSDK_NGX_PerfQuality_Value_DLAA;

    NVSDK_NGX_Parameter_SetI_Original (InParameters, NVSDK_NGX_Parameter_PerfQualityValue, perf_quality);
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