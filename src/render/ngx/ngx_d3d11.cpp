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
#include <SpecialK/render/ngx/ngx_dlss.h>

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L" NGX DX11 "

SK_DLSS_Context SK_NGX_DLSS11;

typedef void (NVSDK_CONV *PFN_NVSDK_NGX_ProgressCallback)(float InCurrentProgress, bool &OutShouldCancel);

using NVSDK_NGX_D3D11_Init_pfn                    = NVSDK_NGX_Result (NVSDK_CONV *)(unsigned long long InApplicationId, const wchar_t *InApplicationDataPath, ID3D11Device *InDevice, const NVSDK_NGX_FeatureCommonInfo *InFeatureInfo, NVSDK_NGX_Version InSDKVersion);
using NVSDK_NGX_D3D11_Init_Ext_pfn                = NVSDK_NGX_Result (NVSDK_CONV *)(unsigned long long InApplicationId, const wchar_t *InApplicationDataPath, ID3D11Device *InDevice, const NVSDK_NGX_FeatureCommonInfo *InFeatureInfo, void* Unknown5);
using NVSDK_NGX_D3D11_Init_ProjectID_pfn          = NVSDK_NGX_Result (NVSDK_CONV *)(const char *InProjectId, NVSDK_NGX_EngineType InEngineType, const char *InEngineVersion, const wchar_t *InApplicationDataPath, ID3D11Device *InDevice, const NVSDK_NGX_FeatureCommonInfo *InFeatureInfo, NVSDK_NGX_Version InSDKVersion);

using NVSDK_NGX_D3D11_DestroyParameters_pfn       = NVSDK_NGX_Result (NVSDK_CONV *)(NVSDK_NGX_Parameter*   InParameters);
using NVSDK_NGX_D3D11_GetParameters_pfn           = NVSDK_NGX_Result (NVSDK_CONV *)(NVSDK_NGX_Parameter** OutParameters);
using NVSDK_NGX_D3D11_GetCapabilityParameters_pfn = NVSDK_NGX_Result (NVSDK_CONV *)(NVSDK_NGX_Parameter** OutParameters);
using NVSDK_NGX_D3D11_AllocateParameters_pfn      = NVSDK_NGX_Result (NVSDK_CONV *)(NVSDK_NGX_Parameter** OutParameters);

using NVSDK_NGX_D3D11_CreateFeature_pfn =
      NVSDK_NGX_Result (NVSDK_CONV *)( ID3D11DeviceContext       *InDevCtx,
                                       NVSDK_NGX_Feature          InFeatureID,
                                       NVSDK_NGX_Parameter       *InParameters,
                                       NVSDK_NGX_Handle         **OutHandle );

using NVSDK_NGX_D3D11_ReleaseFeature_pfn =
      NVSDK_NGX_Result (NVSDK_CONV *)( NVSDK_NGX_Handle *InHandle );
using NVSDK_NGX_D3D11_EvaluateFeature_pfn =
      NVSDK_NGX_Result (NVSDK_CONV *)( ID3D11DeviceContext       *InDevCtx,
                                 const NVSDK_NGX_Handle          *InFeatureHandle,
                                 const NVSDK_NGX_Parameter       *InParameters,
                                   PFN_NVSDK_NGX_ProgressCallback InCallback );

struct IDXGIAdapter;

using NVSDK_NGX_D3D11_GetFeatureRequirements_pfn =
      NVSDK_NGX_Result (NVSDK_CONV *)( IDXGIAdapter *Adapter,
                                 const NVSDK_NGX_FeatureDiscoveryInfo *FeatureDiscoveryInfo,
                                       NVSDK_NGX_FeatureRequirement   *OutSupported );

static NVSDK_NGX_D3D11_Init_pfn
       NVSDK_NGX_D3D11_Init_Original                    = nullptr;
static NVSDK_NGX_D3D11_Init_Ext_pfn
       NVSDK_NGX_D3D11_Init_Ext_Original                = nullptr;
static NVSDK_NGX_D3D11_Init_ProjectID_pfn
       NVSDK_NGX_D3D11_Init_ProjectID_Original          = nullptr;
static NVSDK_NGX_D3D11_CreateFeature_pfn
       NVSDK_NGX_D3D11_CreateFeature_Original           = nullptr;
static NVSDK_NGX_D3D11_EvaluateFeature_pfn              
       NVSDK_NGX_D3D11_EvaluateFeature_Original         = nullptr;
static NVSDK_NGX_D3D11_AllocateParameters_pfn           
       NVSDK_NGX_D3D11_AllocateParameters_Original      = nullptr;
static NVSDK_NGX_D3D11_DestroyParameters_pfn            
       NVSDK_NGX_D3D11_DestroyParameters_Original       = nullptr;
static NVSDK_NGX_D3D11_GetCapabilityParameters_pfn
       NVSDK_NGX_D3D11_GetCapabilityParameters_Original = nullptr;
static NVSDK_NGX_D3D11_GetParameters_pfn
       NVSDK_NGX_D3D11_GetParameters_Original           = nullptr;
static NVSDK_NGX_D3D11_ReleaseFeature_pfn               
       NVSDK_NGX_D3D11_ReleaseFeature_Original          = nullptr;
static NVSDK_NGX_D3D11_GetFeatureRequirements_pfn
       NVSDK_NGX_D3D11_GetFeatureRequirements_Original  = nullptr;

NVSDK_NGX_Result
NVSDK_CONV
NVSDK_NGX_D3D11_Init_Detour (unsigned long long InApplicationId, const wchar_t *InApplicationDataPath, ID3D11Device *InDevice, const NVSDK_NGX_FeatureCommonInfo *InFeatureInfo, NVSDK_NGX_Version InSDKVersion)
{
  SK_LOG_FIRST_CALL

  if (config.nvidia.dlss.compat.override_appid != -1)
  {
    InApplicationId = config.nvidia.dlss.compat.override_appid;
  }

  return
    NVSDK_NGX_D3D11_Init_Original (
      InApplicationId, InApplicationDataPath,
             InDevice, InFeatureInfo, InSDKVersion );
}

NVSDK_NGX_Result
NVSDK_CONV
NVSDK_NGX_D3D11_Init_Ext_Detour (unsigned long long InApplicationId, const wchar_t *InApplicationDataPath, ID3D11Device *InDevice, const NVSDK_NGX_FeatureCommonInfo *InFeatureInfo, void* Unknown5)
{
  SK_LOG_FIRST_CALL

  if (config.nvidia.dlss.compat.override_appid != -1)
  {
    InApplicationId = config.nvidia.dlss.compat.override_appid;
  }

  return
    NVSDK_NGX_D3D11_Init_Ext_Original (
      InApplicationId, InApplicationDataPath,
             InDevice, InFeatureInfo, Unknown5 );
}

NVSDK_NGX_Result
NVSDK_CONV
NVSDK_NGX_D3D11_Init_ProjectID_Detour (const char *InProjectId, NVSDK_NGX_EngineType InEngineType, const char *InEngineVersion, const wchar_t *InApplicationDataPath, ID3D11Device *InDevice, const NVSDK_NGX_FeatureCommonInfo *InFeatureInfo, NVSDK_NGX_Version InSDKVersion)
{
  SK_LOG_FIRST_CALL

  if (config.nvidia.dlss.compat.override_appid != -1)
  {
    InProjectId = "24480451-f00d-face-1304-0308dabad187";
  }

  return
    NVSDK_NGX_D3D11_Init_ProjectID_Original (
      InProjectId, InEngineType, InEngineVersion,
        InApplicationDataPath, InDevice, InFeatureInfo, InSDKVersion );
}

NVSDK_NGX_Result
NVSDK_CONV
NVSDK_NGX_D3D11_GetFeatureRequirements_Detour ( IDXGIAdapter                   *Adapter,
                                          const NVSDK_NGX_FeatureDiscoveryInfo *FeatureDiscoveryInfo,
                                                NVSDK_NGX_FeatureRequirement   *OutSupported )
{
  SK_LOG_FIRST_CALL

  if (config.nvidia.dlss.spoof_support)
  {
               OutSupported->MinHWArchitecture = (unsigned int)NV_GPU_ARCHITECTURE_NV40;
    strncpy_s (OutSupported->MinOSVersion, "1.0.0.0", 254);
               OutSupported->FeatureSupported  = NVSDK_NGX_FeatureSupportResult_Supported;

    return NVSDK_NGX_Result_Success;
  }

  return
    NVSDK_NGX_D3D11_GetFeatureRequirements_Original (
      Adapter, FeatureDiscoveryInfo, OutSupported
    );
}

NVSDK_NGX_Result
NVSDK_CONV
NVSDK_NGX_D3D11_GetParameters_Detour (NVSDK_NGX_Parameter **InParameters)
{
  SK_LOG_FIRST_CALL

  SK_NGX_DLSS11.log_call ();

  std::lock_guard
    lock (SK_NGX_Threading->locks.Params);

  NVSDK_NGX_Result ret =
    NVSDK_NGX_D3D11_GetParameters_Original (InParameters);

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
NVSDK_NGX_D3D11_GetCapabilityParameters_Detour (NVSDK_NGX_Parameter **InParameters)
{
  SK_LOG_FIRST_CALL

  SK_NGX_DLSS11.log_call ();

  std::lock_guard
    lock (SK_NGX_Threading->locks.Params);

  NVSDK_NGX_Result ret =
    NVSDK_NGX_D3D11_GetCapabilityParameters_Original (InParameters);

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
NVSDK_NGX_D3D11_AllocateParameters_Detour (NVSDK_NGX_Parameter** InParameters)
{
  SK_LOG_FIRST_CALL

  SK_NGX_DLSS11.log_call ();

  std::lock_guard
    lock (SK_NGX_Threading->locks.Params);

  NVSDK_NGX_Result ret =
    NVSDK_NGX_D3D11_AllocateParameters_Original (InParameters);

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
NVSDK_NGX_D3D11_DestroyParameters_Detour (NVSDK_NGX_Parameter* InParameters)
{
  SK_LOG_FIRST_CALL

  SK_NGX_DLSS11.log_call ();

  std::lock_guard
    lock (SK_NGX_Threading->locks.Params);

  NVSDK_NGX_Result ret =
    NVSDK_NGX_D3D11_DestroyParameters_Original (InParameters);

  if (ret == NVSDK_NGX_Result_Success)
  {
    SK_RunOnce (
      SK_NGX_HookParameters (InParameters)
    );

    if (InParameters == SK_NGX_DLSS11.frame_gen.Parameters)
    {
      SK_NGX_DLSS11.frame_gen.Parameters = nullptr;
    }

    else if (InParameters == SK_NGX_DLSS11.super_sampling.Parameters)
    {
      SK_NGX_DLSS11.super_sampling.Parameters = nullptr;
    }
  }

  return ret;
}

NVSDK_NGX_Result
NVSDK_CONV
NVSDK_NGX_D3D11_CreateFeature_Detour ( ID3D11DeviceContext       *InDevCtx,
                                       NVSDK_NGX_Feature          InFeatureID,
                                       NVSDK_NGX_Parameter       *InParameters,
                                       NVSDK_NGX_Handle         **OutHandle )
{
  SK_LOG_FIRST_CALL

  SK_NGX_DLSS11.log_call ();

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
    NVSDK_NGX_D3D11_CreateFeature_Original ( InDevCtx,
                                             InFeatureID,
                                             InParameters, OutHandle );

  if ( ret == NVSDK_NGX_Result_Success ||
       ret == NVSDK_NGX_Result_FAIL_FeatureAlreadyExists )
  {
    if (InFeatureID == NVSDK_NGX_Feature_FrameGeneration)
    {
      SK_NGX_EstablishDLSSGVersion ();

      SK_ReleaseAssert ( SK_NGX_DLSS11.frame_gen.Handle == *OutHandle ||
                         SK_NGX_DLSS11.frame_gen.Handle == nullptr );

      SK_NGX_DLSS11.frame_gen.Handle     = *OutHandle;
      SK_NGX_DLSS11.frame_gen.Parameters = InParameters;

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
      //SK_ReleaseAssert ( SK_NGX_DLSS11.super_sampling.Handle == *OutHandle ||
      //                   SK_NGX_DLSS11.super_sampling.Handle == nullptr );

      SK_NGX_DLSS11.super_sampling.Handle     = *OutHandle;
      SK_NGX_DLSS11.super_sampling.Parameters = InParameters;

      SK_LOGi1 (L"DLSS Feature Created!");
    }
  }

  else
  {
    SK_LOGi0 (
      L"NVSDK_NGX_D3D11_CreateFeature (..., %d, ...) Failed - %x (%ws)",
        InFeatureID, ret, GetNGXResultAsString != nullptr ?
                          GetNGXResultAsString (ret)      :
                          L"Unknown Result"
    );

    if (ret == NVSDK_NGX_Result_FAIL_InvalidParameter ||
        ret == NVSDK_NGX_Result_FAIL_UnsupportedParameter)
    {
      SK_RunOnce (SK_NGX_DumpParameters (InParameters));
    }
  }

  return ret;
}

NVSDK_NGX_Result
NVSDK_CONV
NVSDK_NGX_D3D11_EvaluateFeature_Detour (ID3D11DeviceContext *InDevCtx, const NVSDK_NGX_Handle *InFeatureHandle, const NVSDK_NGX_Parameter *InParameters, PFN_NVSDK_NGX_ProgressCallback InCallback)
{
  SK_LOG_FIRST_CALL

  SK_NGX_DLSS11.log_call ();

  if (InFeatureHandle == SK_NGX_DLSS11.super_sampling.Handle)
  {
    if (config.nvidia.dlss.forced_preset != -1)
    {
      unsigned int dlss_mode;

      InParameters->Get (NVSDK_NGX_Parameter_PerfQualityValue, &dlss_mode);

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

      NVSDK_NGX_Parameter_SetUI_Original ((NVSDK_NGX_Parameter *)InParameters, szPresetHint, preset);
    }

    if (InFeatureHandle == SK_NGX_DLSS11.super_sampling.Handle)
    {
      if (ReadULong64Acquire (&SK_NGX_DLSS11.super_sampling.ResetFrame) >
          ReadULong64Acquire (&SK_NGX_DLSS11.super_sampling.LastFrame))
      {
        NVSDK_NGX_Parameter_SetI_Original ((NVSDK_NGX_Parameter *)InParameters, "Reset", 1);
      }
    }
  }

  NVSDK_NGX_Result ret =
    NVSDK_NGX_D3D11_EvaluateFeature_Original (InDevCtx, InFeatureHandle, InParameters, InCallback);

  if (ret == NVSDK_NGX_Result_Success)
  {
    if (InFeatureHandle == SK_NGX_DLSS11.frame_gen.Handle)
    {
      WriteULong64Release (&SK_NGX_DLSS11.frame_gen.LastFrame,      SK_GetFramesDrawn ());
    }

    else if (InFeatureHandle == SK_NGX_DLSS11.super_sampling.Handle)
    {
      WriteULong64Release (&SK_NGX_DLSS11.super_sampling.LastFrame, SK_GetFramesDrawn ());
    }
  }

  else
  {
    const wchar_t* wszFeatureName =
      (InFeatureHandle == SK_NGX_DLSS11.frame_gen.Handle)      ? L"DLSS Frame Generation" :
      (InFeatureHandle == SK_NGX_DLSS11.super_sampling.Handle) ? L"DLSS"                  :
                                                                 L"Unknown Feature";

    SK_LOGi0 (
      L"NVSDK_NGX_D3D11_EvaluateFeature (%p, %ws, %p, %p) Failed - %x (%ws)",
        InDevCtx, wszFeatureName, InParameters, InCallback, ret,
                          GetNGXResultAsString != nullptr ?
                          GetNGXResultAsString (ret)      :
                          L"Unknown Result"
    );

    if (ret == NVSDK_NGX_Result_FAIL_InvalidParameter ||
        ret == NVSDK_NGX_Result_FAIL_UnsupportedParameter)
    {
      SK_RunOnce (SK_NGX_DumpParameters (InParameters));
    }
  }

  return ret;
}


NVSDK_NGX_Result
NVSDK_CONV
NVSDK_NGX_D3D11_ReleaseFeature_Detour (NVSDK_NGX_Handle *InHandle)
{
  SK_LOG_FIRST_CALL

  SK_NGX_DLSS11.log_call ();

  std::lock_guard
    lock (SK_NGX_Threading->locks.Params);

  NVSDK_NGX_Result ret =
    NVSDK_NGX_D3D11_ReleaseFeature_Original (InHandle);

  if (ret == NVSDK_NGX_Result_Success)
  {
    if (InHandle == SK_NGX_DLSS11.frame_gen.Handle)
    {
      SK_NGX_DLSS11.frame_gen.Parameters = nullptr;
      SK_NGX_DLSS11.frame_gen.Handle     = nullptr;
      __SK_IsDLSSGActive                 = false;
      __SK_ForceDLSSGPacing              = false;

      SK_LOGi1 (L"DLSS-G Feature Released!");
    }

    else if (InHandle == SK_NGX_DLSS11.super_sampling.Handle)
    {
      SK_NGX_DLSS11.super_sampling.Parameters = nullptr;
      SK_NGX_DLSS11.super_sampling.Handle     = nullptr;

      SK_LOGi1 (L"DLSS Feature Released!");
    }
  }

  return ret;
}

void
SK_NGX11_UpdateDLSSGStatus (void)
{
  if (! SK_NGX_DLSS11.apis_called)
    return;

  std::lock_guard
    lock (SK_NGX_Threading->locks.Params);

  UINT uiNumberOfFrames    = 0;
  UINT uiEnableOFA         = 0;
  UINT uiEnableDLSSGInterp = 0;

  if (SK_NGX_DLSS11.frame_gen.Parameters != nullptr)
  {
    SK_NGX_DLSS11.frame_gen.Parameters->Get ("Enable.OFA",         &uiEnableOFA);
    SK_NGX_DLSS11.frame_gen.Parameters->Get ("DLSSG.EnableInterp", &uiEnableDLSSGInterp);
    SK_NGX_DLSS11.frame_gen.Parameters->Get ("DLSSG.NumFrames",    &uiNumberOfFrames);

    //{
    //  SK_LOGi0 (L"Failure to get DLSS-G Parameters During SK_NGX_UpdateDLSSGStatus (...)");
    //}
    //
  }

  __SK_IsDLSSGActive =
    ReadULong64Acquire (&SK_NGX_DLSS11.frame_gen.LastFrame) >= SK_GetFramesDrawn () - 8 &&
                       uiNumberOfFrames >= 1 &&
                       uiEnableDLSSGInterp   && uiEnableOFA;

  if (SK_NGX_DLSS11.frame_gen.Handle != nullptr)
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
NVSDK_NGX_D3D11_Shutdown  (void);

NVSDK_NGX_Result
NVSDK_CONV
NVSDK_NGX_D3D11_Shutdown1 (ID3D11Device *InDevice);

NVSDK_NGX_Result
NVSDK_CONV
NVSDK_NGX_D3D11_GetFeatureRequirements ( IDXGIAdapter                   *Adapter,
                                   const NVSDK_NGX_FeatureDiscoveryInfo *FeatureDiscoveryInfo,
                                         NVSDK_NGX_FeatureRequirement   *OutSupported );

void
SK_NGX_InitD3D11 (void)
{
  SK_RunOnce (
  {
    SK_NGX_EstablishDLSSVersion ();

    SK_CreateDLLHook2 ( L"_nvngx.dll",
                         "NVSDK_NGX_D3D11_Init",
                          NVSDK_NGX_D3D11_Init_Detour,
                (void **)&NVSDK_NGX_D3D11_Init_Original );

    SK_CreateDLLHook2 ( L"_nvngx.dll",
                         "NVSDK_NGX_D3D11_Init_Ext",
                          NVSDK_NGX_D3D11_Init_Ext_Detour,
                (void **)&NVSDK_NGX_D3D11_Init_Ext_Original );

    SK_CreateDLLHook2 ( L"_nvngx.dll",
                         "NVSDK_NGX_D3D11_Init_ProjectID",
                          NVSDK_NGX_D3D11_Init_ProjectID_Detour,
                (void **)&NVSDK_NGX_D3D11_Init_ProjectID_Original );

    SK_CreateDLLHook2 ( L"_nvngx.dll",
                         "NVSDK_NGX_D3D11_CreateFeature",
                          NVSDK_NGX_D3D11_CreateFeature_Detour,
                (void **)&NVSDK_NGX_D3D11_CreateFeature_Original );

    SK_CreateDLLHook2 ( L"_nvngx.dll",
                           "NVSDK_NGX_D3D11_ReleaseFeature",
                            NVSDK_NGX_D3D11_ReleaseFeature_Detour,
                  (void **)&NVSDK_NGX_D3D11_ReleaseFeature_Original );

    SK_CreateDLLHook2 ( L"_nvngx.dll",
                           "NVSDK_NGX_D3D11_EvaluateFeature",
                            NVSDK_NGX_D3D11_EvaluateFeature_Detour,
                  (void **)&NVSDK_NGX_D3D11_EvaluateFeature_Original );

    SK_CreateDLLHook2 ( L"_nvngx.dll",
                           "NVSDK_NGX_D3D11_DestroyParameters",
                            NVSDK_NGX_D3D11_DestroyParameters_Detour,
                  (void **)&NVSDK_NGX_D3D11_DestroyParameters_Original );

    SK_CreateDLLHook2 ( L"_nvngx.dll",
                           "NVSDK_NGX_D3D11_AllocateParameters",
                            NVSDK_NGX_D3D11_AllocateParameters_Detour,
                  (void **)&NVSDK_NGX_D3D11_AllocateParameters_Original );

    SK_CreateDLLHook2 ( L"_nvngx.dll",
                           "NVSDK_NGX_D3D11_GetParameters",
                            NVSDK_NGX_D3D11_GetParameters_Detour,
                  (void **)&NVSDK_NGX_D3D11_GetParameters_Original );

    SK_CreateDLLHook2 ( L"_nvngx.dll",
                           "NVSDK_NGX_D3D11_GetCapabilityParameters",
                            NVSDK_NGX_D3D11_GetCapabilityParameters_Detour,
                  (void **)&NVSDK_NGX_D3D11_GetCapabilityParameters_Original );

    // Do not hook unless overriding, NGX will not return valid
    //   results if the function is hooked
    if (config.nvidia.dlss.spoof_support)
    {
      SK_CreateDLLHook2 ( L"_nvngx.dll",
                             "NVSDK_NGX_D3D11_GetFeatureRequirements",
                              NVSDK_NGX_D3D11_GetFeatureRequirements_Detour,
                    (void **)&NVSDK_NGX_D3D11_GetFeatureRequirements_Original );
    }
  });
}