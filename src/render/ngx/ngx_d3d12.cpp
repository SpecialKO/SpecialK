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

#include <SpecialK/render/ngx/ngx_defs.h>

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L" NGX DX12 "

struct NGX_ThreadSafety {
  struct {
    SK_Thread_HybridSpinlock Params;
  } locks;
};

SK_LazyGlobal <NGX_ThreadSafety> SK_NGX_Threading;

bool __SK_HasDLSSGStatusSupport = false;
bool __SK_IsDLSSGActive         = false;
bool __SK_DoubleUpOnReflex      = false;
bool __SK_ForceDLSSGPacing      = false;

static bool     SK_NGX12_APIsCalled           = false;
static unsigned int SK_NGX_GameSetPerfQuality = 0;

using NVSDK_NGX_Parameter_SetF_pfn           = void             (NVSDK_CONV *)(NVSDK_NGX_Parameter *InParameter, const char* InName,          float InValue);
using NVSDK_NGX_Parameter_SetI_pfn           = void             (NVSDK_CONV *)(NVSDK_NGX_Parameter *InParameter, const char* InName,          int   InValue);
using NVSDK_NGX_Parameter_SetUI_pfn          = void             (NVSDK_CONV *)(NVSDK_NGX_Parameter *InParameter, const char* InName, unsigned int   InValue);
using NVSDK_NGX_Parameter_GetUI_pfn          = NVSDK_NGX_Result (NVSDK_CONV *)(NVSDK_NGX_Parameter *InParameter, const char* InName, unsigned int* OutValue);
using NVSDK_NGX_Parameter_GetVoidPointer_pfn = NVSDK_NGX_Result (NVSDK_CONV *)(NVSDK_NGX_Parameter *InParameter, const char* InName, void**        OutValue);

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
  }

  if ((! _stricmp (InName, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_DLAA))        ||
      (! _stricmp (InName, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Quality))     ||
      (! _stricmp (InName, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Balanced))    ||
      (! _stricmp (InName, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Performance)) ||
      (! _stricmp (InName, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_UltraPerformance)))
  {
    if (config.nvidia.dlss.force_dlaa)
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

using NVSDK_NGX_DLSS_GetStatsCallback_pfn           = NVSDK_NGX_Result (NVSDK_CONV *)(NVSDK_NGX_Parameter *InParams);
using NVSDK_NGX_DLSS_GetOptimalSettingsCallback_pfn = NVSDK_NGX_Result (NVSDK_CONV *)(NVSDK_NGX_Parameter *InParams);

NVSDK_NGX_Result NVSDK_CONV NVSDK_NGX_DLSS_GetOptimalSettingsCallback (NVSDK_NGX_Parameter* InParams);
NVSDK_NGX_Result NVSDK_CONV NVSDK_NGX_DLSS_GetStatsCallback           (NVSDK_NGX_Parameter* InParams);

struct SK_DLSS_Context
{
  struct dlss_s {
    NVSDK_NGX_Handle*    Handle     = nullptr;
    NVSDK_NGX_Parameter* Parameters = nullptr;
    volatile ULONG64     LastFrame  = 0ULL;

    struct callbacks_s {
      NVSDK_NGX_DLSS_GetStatsCallback_pfn           GetStatsCallback_Original           = nullptr;
      NVSDK_NGX_DLSS_GetOptimalSettingsCallback_pfn GetOptimalSettingsCallback_Original = nullptr;

      static NVSDK_NGX_Result NVSDK_CONV GetOptimalSettingsCallback_Override (NVSDK_NGX_Parameter *InParams)
      {
        SK_LOG_FIRST_CALL

        if (SK_NGX_DLSS12.super_sampling.callbacks.GetOptimalSettingsCallback_Original != nullptr)
        {
          NVSDK_NGX_Result result =
            SK_NGX_DLSS12.super_sampling.callbacks.GetOptimalSettingsCallback_Original (InParams);

          if (NVSDK_NGX_Result_Success == result)
          {
          }

          return result;
        }

        SK_LOGi0 (L"No DLSS Optimal Settings Callback Detected");

        return NVSDK_NGX_Result_Success;
      }
    } callbacks;
  } super_sampling;

  struct dlssg_s {
    NVSDK_NGX_Handle*    Handle     = nullptr;
    NVSDK_NGX_Parameter* Parameters = nullptr;
    volatile ULONG64     LastFrame  = 0ULL;
  } frame_gen;
} SK_NGX_DLSS12;

extern SK_DLSS_Context SK_NGX_DLSS11;

NVSDK_NGX_Parameter*
SK_NGX_GetDLSSParameters (void)
{
  if (     SK_NGX12_APIsCalled)
    return SK_NGX_DLSS12.super_sampling.Parameters;

  return
    SK_NGX_DLSS11.super_sampling.Parameters;
}

bool
SK_NGX_IsUsingDLSS (void)
{
  if (                     SK_NGX12_APIsCalled)
    return                 SK_NGX_DLSS12.super_sampling.Handle     != nullptr &&
      ReadULong64Acquire (&SK_NGX_DLSS12.super_sampling.LastFrame) >= SK_GetFramesDrawn () - 8;

  return                 SK_NGX_DLSS11.super_sampling.Handle     != nullptr &&
    ReadULong64Acquire (&SK_NGX_DLSS11.super_sampling.LastFrame) >= SK_GetFramesDrawn () - 8;
}

typedef void (NVSDK_CONV *PFN_NVSDK_NGX_ProgressCallback)(float InCurrentProgress, bool &OutShouldCancel);

// NGX return-code conversion-to-string utility only as a helper for debugging/logging - not for official use.
using  GetNGXResultAsString_pfn = const wchar_t* (NVSDK_CONV *)(NVSDK_NGX_Result InNGXResult);
static GetNGXResultAsString_pfn
       GetNGXResultAsString = nullptr;

using NVSDK_NGX_D3D12_DestroyParameters_pfn       = NVSDK_NGX_Result (NVSDK_CONV *)(NVSDK_NGX_Parameter* InParameters);
using NVSDK_NGX_D3D12_GetCapabilityParameters_pfn = NVSDK_NGX_Result (NVSDK_CONV *)(NVSDK_NGX_Parameter** OutParameters);
using NVSDK_NGX_D3D12_AllocateParameters_pfn      = NVSDK_NGX_Result (NVSDK_CONV *)(NVSDK_NGX_Parameter** OutParameters);

using NVSDK_NGX_D3D12_GetParameters_pfn =
      NVSDK_NGX_Result (NVSDK_CONV *)(NVSDK_NGX_Parameter **OutParameters);

using NVSDK_NGX_D3D12_CreateFeature_pfn =
      NVSDK_NGX_Result (NVSDK_CONV *)( ID3D12GraphicsCommandList *InCmdList,
                                       NVSDK_NGX_Feature          InFeatureID,
                                       NVSDK_NGX_Parameter       *InParameters,
                                       NVSDK_NGX_Handle         **OutHandle );

using NVSDK_NGX_D3D12_ReleaseFeature_pfn =
      NVSDK_NGX_Result (NVSDK_CONV *)( NVSDK_NGX_Handle *InHandle );
using NVSDK_NGX_D3D12_EvaluateFeature_pfn =
      NVSDK_NGX_Result (NVSDK_CONV *)( ID3D12GraphicsCommandList *InCmdList,
                                 const NVSDK_NGX_Handle          *InFeatureHandle,
                                 const NVSDK_NGX_Parameter       *InParameters,
                                   PFN_NVSDK_NGX_ProgressCallback InCallback );

// This is not API-specific...
using NVSDK_NGX_UpdateFeature_pfn =
      NVSDK_NGX_Result (NVSDK_CONV *)( const NVSDK_NGX_Application_Identifier *ApplicationId,
                                       const NVSDK_NGX_Feature                 FeatureID );

static NVSDK_NGX_D3D12_CreateFeature_pfn
       NVSDK_NGX_D3D12_CreateFeature_Original = nullptr;
static NVSDK_NGX_D3D12_EvaluateFeature_pfn
       NVSDK_NGX_D3D12_EvaluateFeature_Original = nullptr;
static NVSDK_NGX_UpdateFeature_pfn
       NVSDK_NGX_UpdateFeature_Original = nullptr;
static NVSDK_NGX_D3D12_AllocateParameters_pfn
       NVSDK_NGX_D3D12_AllocateParameters_Original = nullptr;
static NVSDK_NGX_D3D12_DestroyParameters_pfn
       NVSDK_NGX_D3D12_DestroyParameters_Original = nullptr;
static NVSDK_NGX_D3D12_GetCapabilityParameters_pfn
       NVSDK_NGX_D3D12_GetCapabilityParameters_Original = nullptr;
static NVSDK_NGX_D3D12_GetParameters_pfn
       NVSDK_NGX_D3D12_GetParameters_Original = nullptr;
static NVSDK_NGX_D3D12_ReleaseFeature_pfn
       NVSDK_NGX_D3D12_ReleaseFeature_Original = nullptr;

void
SK_NGX_HookParameters (NVSDK_NGX_Parameter* Params)
{
  if (NVSDK_NGX_Parameter_GetUI_Original != nullptr)
    return;

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
}

NVSDK_NGX_Result
NVSDK_CONV
NVSDK_NGX_D3D12_GetParameters_Detour (NVSDK_NGX_Parameter **InParameters)
{
  SK_NGX12_APIsCalled = true;

  SK_LOG_FIRST_CALL

  std::lock_guard
    lock (SK_NGX_Threading->locks.Params);

  NVSDK_NGX_Result ret =
    NVSDK_NGX_D3D12_GetParameters_Original (InParameters);

  if (ret == NVSDK_NGX_Result_Success)
  {
    SK_RunOnce (
      SK_NGX_HookParameters (*InParameters)
    );
  }

  return ret;
}

NVSDK_NGX_Result
NVSDK_CONV
NVSDK_NGX_D3D12_GetCapabilityParameters_Detour (NVSDK_NGX_Parameter **InParameters)
{
  SK_NGX12_APIsCalled = true;

  SK_LOG_FIRST_CALL

  std::lock_guard
    lock (SK_NGX_Threading->locks.Params);

  NVSDK_NGX_Result ret =
    NVSDK_NGX_D3D12_GetCapabilityParameters_Original (InParameters);

  if (ret == NVSDK_NGX_Result_Success)
  {
    SK_RunOnce (
      SK_NGX_HookParameters (*InParameters)
    );
  }

  return ret;
}

NVSDK_NGX_Result
NVSDK_CONV
NVSDK_NGX_D3D12_AllocateParameters_Detour (NVSDK_NGX_Parameter** InParameters)
{
  SK_NGX12_APIsCalled = true;

  SK_LOG_FIRST_CALL

  std::lock_guard
    lock (SK_NGX_Threading->locks.Params);

  NVSDK_NGX_Result ret =
    NVSDK_NGX_D3D12_AllocateParameters_Original (InParameters);

  if (ret == NVSDK_NGX_Result_Success)
  {
    SK_RunOnce (
      SK_NGX_HookParameters (*InParameters)
    );
  }

  return ret;
}

NVSDK_NGX_Result
NVSDK_CONV
NVSDK_NGX_D3D12_DestroyParameters_Detour (NVSDK_NGX_Parameter* InParameters)
{
  SK_NGX12_APIsCalled = true;

  SK_LOG_FIRST_CALL

  std::lock_guard
    lock (SK_NGX_Threading->locks.Params);

  NVSDK_NGX_Result ret =
    NVSDK_NGX_D3D12_DestroyParameters_Original (InParameters);

  if (ret == NVSDK_NGX_Result_Success)
  {
    SK_RunOnce (
      SK_NGX_HookParameters (InParameters)
    );

    if (InParameters == SK_NGX_DLSS12.frame_gen.Parameters)
    {
      SK_NGX_DLSS12.frame_gen.Parameters = nullptr;
    }

    else if (InParameters == SK_NGX_DLSS12.super_sampling.Parameters)
    {
      SK_NGX_DLSS12.super_sampling.Parameters = nullptr;
    }
  }

  return ret;
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

NVSDK_NGX_Result
NVSDK_CONV
NVSDK_NGX_D3D12_CreateFeature_Detour ( ID3D12GraphicsCommandList *InCmdList,
                                       NVSDK_NGX_Feature          InFeatureID,
                                       NVSDK_NGX_Parameter       *InParameters,
                                       NVSDK_NGX_Handle         **OutHandle )
{
  SK_NGX12_APIsCalled = true;

  SK_LOG_FIRST_CALL

  std::lock_guard
    lock (SK_NGX_Threading->locks.Params);

  SK_RunOnce (
    SK_NGX_HookParameters (InParameters)
  );

  if (InFeatureID == NVSDK_NGX_Feature_SuperSampling)
  {
    SK_NGX_DLSS_CreateFeatureOverrideParams (InParameters);
  }

  NVSDK_NGX_Result ret =
    NVSDK_NGX_D3D12_CreateFeature_Original ( InCmdList,
                                             InFeatureID,
                                             InParameters, OutHandle );

  if ( ret == NVSDK_NGX_Result_Success ||
       ret == NVSDK_NGX_Result_FAIL_FeatureAlreadyExists )
  {
    if (InFeatureID == NVSDK_NGX_Feature_FrameGeneration)
    {
      SK_ReleaseAssert ( SK_NGX_DLSS12.frame_gen.Handle == *OutHandle ||
                         SK_NGX_DLSS12.frame_gen.Handle == nullptr );

      SK_NGX_DLSS12.frame_gen.Handle     = *OutHandle;
      SK_NGX_DLSS12.frame_gen.Parameters = InParameters;

      __SK_HasDLSSGStatusSupport = true;

      UINT uiEnableOFA         = 0;
      UINT uiEnableDLSSGInterp = 0;

      InParameters->Get ("Enable.OFA",         &uiEnableOFA);
      InParameters->Get ("DLSSG.EnableInterp", &uiEnableDLSSGInterp);

      __SK_IsDLSSGActive =
        ( uiEnableOFA && uiEnableDLSSGInterp );

      __SK_ForceDLSSGPacing = __SK_IsDLSSGActive;

      SK_LOGi1 (L"DLSS-G Feature Created!");
    }

    else if (InFeatureID == NVSDK_NGX_Feature_SuperSampling)
    {
      //SK_ReleaseAssert ( SK_NGX_DLSS12.super_sampling.Handle == *OutHandle ||
      //                   SK_NGX_DLSS12.super_sampling.Handle == nullptr );

      SK_NGX_DLSS12.super_sampling.Handle     = *OutHandle;
      SK_NGX_DLSS12.super_sampling.Parameters = InParameters;

      SK_LOGi1 (L"DLSS Feature Created!");
    }
  }

  else
  {
    SK_LOGi0 (
      L"NVSDK_NGX_D3D12_CreateFeature (..., %d, ...) Failed - %x (%ws)",
        InFeatureID, ret, GetNGXResultAsString != nullptr ?
                          GetNGXResultAsString (ret)      :
                          L"Unknown Result"
    );
  }

  return ret;
}

NVSDK_NGX_Result
NVSDK_CONV
NVSDK_NGX_D3D12_EvaluateFeature_Detour (ID3D12GraphicsCommandList *InCmdList, const NVSDK_NGX_Handle *InFeatureHandle, const NVSDK_NGX_Parameter *InParameters, PFN_NVSDK_NGX_ProgressCallback InCallback)
{
  SK_NGX12_APIsCalled = true;

  NVSDK_NGX_Result ret =
    NVSDK_NGX_D3D12_EvaluateFeature_Original (InCmdList, InFeatureHandle, InParameters, InCallback);

  if (ret == NVSDK_NGX_Result_Success)
  {
    if (InFeatureHandle == SK_NGX_DLSS12.frame_gen.Handle)
    {
      WriteULong64Release (&SK_NGX_DLSS12.frame_gen.LastFrame,      SK_GetFramesDrawn ());
    }

    else if (InFeatureHandle == SK_NGX_DLSS12.super_sampling.Handle)
    {
      WriteULong64Release (&SK_NGX_DLSS12.super_sampling.LastFrame, SK_GetFramesDrawn ());
    }
  }

  return ret;
}


NVSDK_NGX_Result
NVSDK_CONV
NVSDK_NGX_D3D12_ReleaseFeature_Detour (NVSDK_NGX_Handle *InHandle)
{
  SK_NGX12_APIsCalled = true;

  SK_LOG_FIRST_CALL

  std::lock_guard
    lock (SK_NGX_Threading->locks.Params);

  NVSDK_NGX_Result ret =
    NVSDK_NGX_D3D12_ReleaseFeature_Original (InHandle);

  if (ret == NVSDK_NGX_Result_Success)
  {
    if (InHandle == SK_NGX_DLSS12.frame_gen.Handle)
    {
      SK_NGX_DLSS12.frame_gen.Parameters = nullptr;
      SK_NGX_DLSS12.frame_gen.Handle     = nullptr;
      __SK_IsDLSSGActive                 = false;
      __SK_ForceDLSSGPacing              = false;

      SK_LOGi1 (L"DLSS-G Feature Released!");
    }

    else if (InHandle == SK_NGX_DLSS12.super_sampling.Handle)
    {
      SK_NGX_DLSS12.super_sampling.Parameters = nullptr;
      SK_NGX_DLSS12.super_sampling.Handle     = nullptr;

      SK_LOGi1 (L"DLSS Feature Released!");
    }
  }

  return ret;
}

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

void
SK_NGX_UpdateDLSSGStatus (void)
{
  extern void SK_NGX12_UpdateDLSSGStatus (void);
  extern void SK_NGX11_UpdateDLSSGStatus (void);

  SK_NGX11_UpdateDLSSGStatus ();
  SK_NGX12_UpdateDLSSGStatus ();
}

void
SK_NGX12_UpdateDLSSGStatus (void)
{
  if (! SK_NGX12_APIsCalled)
    return;

  std::lock_guard
    lock (SK_NGX_Threading->locks.Params);

  UINT uiNumberOfFrames    = 0;
  UINT uiEnableOFA         = 0;
  UINT uiEnableDLSSGInterp = 0;

  if (SK_NGX_DLSS12.frame_gen.Parameters != nullptr)
  {
    SK_NGX_DLSS12.frame_gen.Parameters->Get ("Enable.OFA",         &uiEnableOFA);
    SK_NGX_DLSS12.frame_gen.Parameters->Get ("DLSSG.EnableInterp", &uiEnableDLSSGInterp);
    SK_NGX_DLSS12.frame_gen.Parameters->Get ("DLSSG.NumFrames",    &uiNumberOfFrames);

#if 0
    ID3D12Resource*                                          pMvecsRes = nullptr;
    SK_NGX_DLSS12.frame_gen.Parameters->Get ("DLSSG.MVecs", &pMvecsRes);

    if (pMvecsRes != nullptr)
    {
      SK_LOGi0 (
        L"DLSS-G Motion Vectors: %hs, Format: %hs",
          pMvecsRes->GetDesc ().Dimension ==  D3D12_RESOURCE_DIMENSION_TEXTURE3D ? "3D"
                                                                                 : "2D",
        SK_DXGI_FormatToStr (pMvecsRes->GetDesc ().Format).data ()
      );
    }
#endif

    //{
    //  SK_LOGi0 (L"Failure to get DLSS-G Parameters During SK_NGX_UpdateDLSSGStatus (...)");
    //}
    //
  }

  __SK_IsDLSSGActive =
    ReadULong64Acquire (&SK_NGX_DLSS12.frame_gen.LastFrame) >= SK_GetFramesDrawn () - 8 &&
                       uiNumberOfFrames >= 1 &&
                       uiEnableDLSSGInterp   && uiEnableOFA;

  if (SK_NGX_DLSS12.frame_gen.Handle != nullptr)
  {
    static UINT        uiLastDLSSGState = UINT_MAX;
    if (std::exchange (uiLastDLSSGState, (UINT)__SK_IsDLSSGActive) != (UINT)__SK_IsDLSSGActive)
    {
      SK_LOGi0 ( L"DLSS-G Feature %ws!",
                  __SK_IsDLSSGActive ? L"Enabled"
                                     : L"Disabled" );
    }
  }

  __SK_ForceDLSSGPacing = __SK_IsDLSSGActive;
}

NVSDK_NGX_Result
NVSDK_CONV
NVSDK_NGX_D3D12_Shutdown  (void);

NVSDK_NGX_Result
NVSDK_CONV
NVSDK_NGX_D3D12_Shutdown1 (ID3D12Device *InDevice);

NVSDK_NGX_Result
NVSDK_CONV
NVSDK_NGX_D3D12_AllocateParameters (NVSDK_NGX_Parameter** OutParameters);

NVSDK_NGX_Result
NVSDK_CONV
NVSDK_NGX_D3D12_GetFeatureRequirements ( IDXGIAdapter                   *Adapter,
                                   const NVSDK_NGX_FeatureDiscoveryInfo *FeatureDiscoveryInfo,
                                         NVSDK_NGX_FeatureRequirement   *OutSupported );

void
SK_NGX_Init (void)
{
  extern void SK_NGX_InitD3D11 (void);
  extern void SK_NGX_InitD3D12 (void);

  SK_NGX_InitD3D11 ();
  SK_NGX_InitD3D12 ();
}

void
SK_NGX_InitD3D12 (void)
{
  SK_RunOnce (
  {
    SK_CreateDLLHook2 ( L"_nvngx.dll",
                         "NVSDK_NGX_D3D12_CreateFeature",
                          NVSDK_NGX_D3D12_CreateFeature_Detour,
                (void **)&NVSDK_NGX_D3D12_CreateFeature_Original );

    SK_CreateDLLHook2 ( L"_nvngx.dll",
                           "NVSDK_NGX_UpdateFeature",
                            NVSDK_NGX_UpdateFeature_Detour,
                  (void **)&NVSDK_NGX_UpdateFeature_Original );

    SK_CreateDLLHook2 ( L"_nvngx.dll",
                           "NVSDK_NGX_D3D12_ReleaseFeature",
                            NVSDK_NGX_D3D12_ReleaseFeature_Detour,
                  (void **)&NVSDK_NGX_D3D12_ReleaseFeature_Original );

    SK_CreateDLLHook2 ( L"_nvngx.dll",
                           "NVSDK_NGX_D3D12_EvaluateFeature",
                            NVSDK_NGX_D3D12_EvaluateFeature_Detour,
                  (void **)&NVSDK_NGX_D3D12_EvaluateFeature_Original );

    SK_CreateDLLHook2 ( L"_nvngx.dll",
                           "NVSDK_NGX_D3D12_DestroyParameters",
                            NVSDK_NGX_D3D12_DestroyParameters_Detour,
                  (void **)&NVSDK_NGX_D3D12_DestroyParameters_Original );

    SK_CreateDLLHook2 ( L"_nvngx.dll",
                           "NVSDK_NGX_D3D12_AllocateParameters",
                            NVSDK_NGX_D3D12_AllocateParameters_Detour,
                  (void **)&NVSDK_NGX_D3D12_AllocateParameters_Original );

    SK_CreateDLLHook2 ( L"_nvngx.dll",
                           "NVSDK_NGX_D3D12_GetParameters",
                            NVSDK_NGX_D3D12_GetParameters_Detour,
                  (void **)&NVSDK_NGX_D3D12_GetParameters_Original );

    SK_CreateDLLHook2 ( L"_nvngx.dll",
                           "NVSDK_NGX_D3D12_GetCapabilityParameters",
                            NVSDK_NGX_D3D12_GetCapabilityParameters_Detour,
                  (void **)&NVSDK_NGX_D3D12_GetCapabilityParameters_Original );

    SK_ApplyQueuedHooks ();

    GetNGXResultAsString =
   (GetNGXResultAsString_pfn)SK_GetProcAddress ( L"_nvngx.dll",
   "GetNGXResultAsString" );
  });
}