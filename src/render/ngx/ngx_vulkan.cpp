// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
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
#include <vulkan/vulkan.h>

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"NGX Vulkan"

SK_DLSS_Context SK_NGX_VULKAN;

typedef enum NVSDK_NGX_VK_OFA_MODE_REQUEST
{
	INTEROP = 0,
	NATIVE,
	AUTO,
	MODE_NUM
} NVSDK_NGX_VK_OFA_MODE_REQUEST;

typedef void (NVSDK_CONV *PFN_NVSDK_NGX_ProgressCallback_C)(float InCurrentProgress, bool *OutShouldCancel);
typedef void (NVSDK_CONV *PFN_NVSDK_NGX_ProgressCallback  )(float InCurrentProgress, bool &OutShouldCancel);

using NVSDK_NGX_VULKAN_Init_pfn                    = NVSDK_NGX_Result (NVSDK_CONV *)(unsigned long long InApplicationId,                                                      const wchar_t *InApplicationDataPath, VkInstance InInstance, VkPhysicalDevice InPD, VkDevice InDevice, const NVSDK_NGX_FeatureCommonInfo *InFeatureInfo, NVSDK_NGX_Version InSDKVersion);
using NVSDK_NGX_VULKAN_Init_ProjectID_pfn          = NVSDK_NGX_Result (NVSDK_CONV *)(const char *InProjectId, NVSDK_NGX_EngineType InEngineType, const char *InEngineVersion, const wchar_t *InApplicationDataPath, VkInstance InInstance, VkPhysicalDevice InPD, VkDevice InDevice, const NVSDK_NGX_FeatureCommonInfo *InFeatureInfo, NVSDK_NGX_Version InSDKVersion);

using NVSDK_NGX_VULKAN_DestroyParameters_pfn       = NVSDK_NGX_Result (NVSDK_CONV *)(NVSDK_NGX_Parameter*   InParameters);
using NVSDK_NGX_VULKAN_GetParameters_pfn           = NVSDK_NGX_Result (NVSDK_CONV *)(NVSDK_NGX_Parameter** OutParameters);
using NVSDK_NGX_VULKAN_GetCapabilityParameters_pfn = NVSDK_NGX_Result (NVSDK_CONV *)(NVSDK_NGX_Parameter** OutParameters);
using NVSDK_NGX_VULKAN_AllocateParameters_pfn      = NVSDK_NGX_Result (NVSDK_CONV *)(NVSDK_NGX_Parameter** OutParameters);

using NVSDK_NGX_VULKAN_CreateFeature_pfn =
      NVSDK_NGX_Result (NVSDK_CONV *)( VkCommandBuffer            InCmdBuffer,
                                       NVSDK_NGX_Feature          InFeatureID,
                                       NVSDK_NGX_Parameter       *InParameters,
                                       NVSDK_NGX_Handle         **OutHandle );

using NVSDK_NGX_VULKAN_CreateFeature1_pfn =
      NVSDK_NGX_Result (NVSDK_CONV *)( VkDevice                   InDevice,
                                       VkCommandBuffer            InCmdList,
                                       NVSDK_NGX_Feature          InFeatureID,
                                       NVSDK_NGX_Parameter       *InParameters,
                                       NVSDK_NGX_Handle         **OutHandle );

using NVSDK_NGX_VULKAN_ReleaseFeature_pfn =
      NVSDK_NGX_Result (NVSDK_CONV *)( NVSDK_NGX_Handle          *InHandle );
using NVSDK_NGX_VULKAN_EvaluateFeature_pfn =
      NVSDK_NGX_Result (NVSDK_CONV *)( VkCommandBuffer            InCmdList,
                                 const NVSDK_NGX_Handle          *InFeatureHandle,
                                 const NVSDK_NGX_Parameter       *InParameters,
                                   PFN_NVSDK_NGX_ProgressCallback InCallback );
using NVSDK_NGX_VULKAN_EvaluateFeature_C_pfn =
      NVSDK_NGX_Result (NVSDK_CONV *)( VkCommandBuffer            InCmdList,
                                 const NVSDK_NGX_Handle          *InFeatureHandle,
                                 const NVSDK_NGX_Parameter       *InParameters,
                                 PFN_NVSDK_NGX_ProgressCallback_C InCallback );

static NVSDK_NGX_VULKAN_Init_pfn
       NVSDK_NGX_VULKAN_Init_Original                    = nullptr;
static NVSDK_NGX_VULKAN_Init_ProjectID_pfn
       NVSDK_NGX_VULKAN_Init_ProjectID_Original          = nullptr;
static NVSDK_NGX_VULKAN_CreateFeature_pfn
       NVSDK_NGX_VULKAN_CreateFeature_Original           = nullptr;
static NVSDK_NGX_VULKAN_CreateFeature1_pfn
       NVSDK_NGX_VULKAN_CreateFeature1_Original          = nullptr;
static NVSDK_NGX_VULKAN_EvaluateFeature_pfn              
       NVSDK_NGX_VULKAN_EvaluateFeature_Original         = nullptr;
static NVSDK_NGX_VULKAN_AllocateParameters_pfn           
       NVSDK_NGX_VULKAN_AllocateParameters_Original      = nullptr;
static NVSDK_NGX_VULKAN_DestroyParameters_pfn            
       NVSDK_NGX_VULKAN_DestroyParameters_Original       = nullptr;
static NVSDK_NGX_VULKAN_GetCapabilityParameters_pfn
       NVSDK_NGX_VULKAN_GetCapabilityParameters_Original = nullptr;
static NVSDK_NGX_VULKAN_GetParameters_pfn
       NVSDK_NGX_VULKAN_GetParameters_Original           = nullptr;
static NVSDK_NGX_VULKAN_ReleaseFeature_pfn               
       NVSDK_NGX_VULKAN_ReleaseFeature_Original          = nullptr;

NVSDK_NGX_Result
NVSDK_CONV
NVSDK_NGX_VULKAN_Init_Detour (unsigned long long InApplicationId, const wchar_t *InApplicationDataPath, VkInstance InInstance, VkPhysicalDevice InPD, VkDevice InDevice, const NVSDK_NGX_FeatureCommonInfo *InFeatureInfo, NVSDK_NGX_Version InSDKVersion)
{                             
  SK_LOG_FIRST_CALL

  if (config.nvidia.dlss.compat.override_appid != -1)
  {
    InApplicationId = config.nvidia.dlss.compat.override_appid;
  }

  return
    NVSDK_NGX_VULKAN_Init_Original (
      InApplicationId, InApplicationDataPath, InInstance,
       InPD, InDevice, InFeatureInfo, InSDKVersion );
}

NVSDK_NGX_Result
NVSDK_CONV
NVSDK_NGX_VULKAN_Init_ProjectID_Detour (const char *InProjectId, NVSDK_NGX_EngineType InEngineType, const char *InEngineVersion, const wchar_t *InApplicationDataPath, VkInstance InInstance, VkPhysicalDevice InPD, VkDevice InDevice, const NVSDK_NGX_FeatureCommonInfo *InFeatureInfo, NVSDK_NGX_Version InSDKVersion)
{
  SK_LOG_FIRST_CALL

  if (config.nvidia.dlss.compat.override_appid != -1)
  {
    InProjectId = "24480451-f00d-face-1304-0308dabad187";
  }

  return
    NVSDK_NGX_VULKAN_Init_ProjectID_Original (
      InProjectId, InEngineType, InEngineVersion,
        InApplicationDataPath, InInstance, InPD,
      InDevice, InFeatureInfo, InSDKVersion );
}

NVSDK_NGX_Result
NVSDK_CONV
NVSDK_NGX_VULKAN_GetParameters_Detour (NVSDK_NGX_Parameter **InParameters)
{
  SK_LOG_FIRST_CALL

  SK_NGX_VULKAN.log_call ();

  std::lock_guard
    lock (SK_NGX_Threading->locks.Params);

  NVSDK_NGX_Result ret =
    NVSDK_NGX_VULKAN_GetParameters_Original (InParameters);

  if (NVSDK_NGX_SUCCEED (ret))
  {
    SK_RunOnce (
      SK_NGX_HookParameters (*InParameters)
    );
  }

  return ret;
}

NVSDK_NGX_Result
NVSDK_CONV
NVSDK_NGX_VULKAN_GetCapabilityParameters_Detour (NVSDK_NGX_Parameter **InParameters)
{
  SK_LOG_FIRST_CALL

  SK_NGX_VULKAN.log_call ();

  std::lock_guard
    lock (SK_NGX_Threading->locks.Params);

  NVSDK_NGX_Result ret =
    NVSDK_NGX_VULKAN_GetCapabilityParameters_Original (InParameters);

  if (NVSDK_NGX_SUCCEED (ret))
  {
    SK_RunOnce (
      SK_NGX_HookParameters (*InParameters)
    );
  }

  return ret;
}

NVSDK_NGX_Result
NVSDK_CONV
NVSDK_NGX_VULKAN_AllocateParameters_Detour (NVSDK_NGX_Parameter** InParameters)
{
  SK_LOG_FIRST_CALL

  SK_NGX_VULKAN.log_call ();

  std::lock_guard
    lock (SK_NGX_Threading->locks.Params);

  NVSDK_NGX_Result ret =
    NVSDK_NGX_VULKAN_AllocateParameters_Original (InParameters);

  if (NVSDK_NGX_SUCCEED (ret))
  {
    SK_RunOnce (
      SK_NGX_HookParameters (*InParameters)
    );
  }

  return ret;
}

NVSDK_NGX_Result
NVSDK_CONV
NVSDK_NGX_VULKAN_DestroyParameters_Detour (NVSDK_NGX_Parameter* InParameters)
{
  SK_LOG_FIRST_CALL

  SK_NGX_VULKAN.log_call ();

  std::lock_guard
    lock (SK_NGX_Threading->locks.Params);

  NVSDK_NGX_Result ret =
    NVSDK_NGX_VULKAN_DestroyParameters_Original (InParameters);

  if (NVSDK_NGX_SUCCEED (ret))
  {
    SK_RunOnce (
      SK_NGX_HookParameters (InParameters)
    );

    for ( auto& instance : SK_NGX_VULKAN.frame_gen.Instances )
    {
      if (instance.second.Parameters == InParameters)
      {
        instance.second.Parameters = nullptr;
      }
    }

    for ( auto& instance : SK_NGX_VULKAN.super_sampling.Instances )
    {
      if (instance.second.Parameters == InParameters)
      {
        instance.second.Parameters = nullptr;
      }
    }
  }

  return ret;
}

NVSDK_NGX_Result
NVSDK_CONV
NVSDK_NGX_VULKAN_CreateFeature_Detour ( VkCommandBuffer            InCmdBuffer,
                                        NVSDK_NGX_Feature          InFeatureID,
                                        NVSDK_NGX_Parameter       *InParameters,
                                        NVSDK_NGX_Handle         **OutHandle )
{
  SK_LOG_FIRST_CALL

  SK_NGX_VULKAN.log_call ();

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
    NVSDK_NGX_VULKAN_CreateFeature_Original ( InCmdBuffer,
                                              InFeatureID,
                                              InParameters, OutHandle );

  if ( NVSDK_NGX_SUCCEED (ret) ||
       ret == NVSDK_NGX_Result_FAIL_FeatureAlreadyExists )
  {
    if (InFeatureID == NVSDK_NGX_Feature_FrameGeneration)
    {
      SK_NGX_EstablishDLSSGVersion (L"nvngx_dlssg.dll");

      SK_DLSS_Context::dlssg_s::instance_s instance;

      instance.Handle     = *OutHandle;
      instance.Parameters = InParameters;

      SK_NGX_VULKAN.frame_gen.Instances [*OutHandle] = instance;

      __SK_HasDLSSGStatusSupport = true;

      UINT uiEnableDLSSGInterp = 0;
      UINT uiMultiFrameCount   = 1;

      InParameters->Get ("DLSSG.EnableInterp",    &uiEnableDLSSGInterp);
      InParameters->Get ("DLSSG.MultiFrameCount", &uiMultiFrameCount);

      __SK_IsDLSSGActive =
        ( uiEnableDLSSGInterp );

      __SK_ForceDLSSGPacing = __SK_IsDLSSGActive;

      SK_LOGi0 (L"DLSS-G Feature Created!");
    }

    else if (InFeatureID == NVSDK_NGX_Feature_SuperSampling)
    {
      //SK_ReleaseAssert ( SK_NGX_VULKAN.super_sampling.Handle == *OutHandle ||
      //                   SK_NGX_VULKAN.super_sampling.Handle == nullptr );

      SK_DLSS_Context::dlss_s::instance_s instance;

      instance.Handle     = *OutHandle;
      instance.Parameters = InParameters;
      instance.DLSS_Type  = InFeatureID;

      SK_NGX_VULKAN.super_sampling.Instances [*OutHandle] = instance;

      SK_LOGi0 (L"DLSS Feature Created!");
    }
  }

  else
  {
    SK_LOGi0 (
      L"NVSDK_NGX_VULKAN_CreateFeature (..., %d, ...) Failed - %x (%ws)",
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
NVSDK_NGX_VULKAN_CreateFeature1_Detour ( VkDevice                   InDevice,
                                         VkCommandBuffer            InCmdList,
                                         NVSDK_NGX_Feature          InFeatureID,
                                         NVSDK_NGX_Parameter       *InParameters,
                                         NVSDK_NGX_Handle         **OutHandle )
{
  SK_LOG_FIRST_CALL

  SK_NGX_VULKAN.log_call ();

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
    NVSDK_NGX_VULKAN_CreateFeature1_Original ( InDevice,
                                               InCmdList,
                                               InFeatureID,
                                               InParameters, OutHandle );

  if ( NVSDK_NGX_SUCCEED (ret) ||
       ret == NVSDK_NGX_Result_FAIL_FeatureAlreadyExists )
  {
    if (InFeatureID == NVSDK_NGX_Feature_FrameGeneration)
    {
      SK_NGX_EstablishDLSSGVersion (L"nvngx_dlssg.dll");

      SK_DLSS_Context::dlssg_s::instance_s instance;

      instance.Handle     = *OutHandle;
      instance.Parameters = InParameters;

      SK_NGX_VULKAN.frame_gen.Instances [*OutHandle] = instance;

      __SK_HasDLSSGStatusSupport = true;

      UINT uiEnableDLSSGInterp = 0;
      UINT uiMultiFrameCount   = 1;

      InParameters->Get ("DLSSG.EnableInterp",    &uiEnableDLSSGInterp);
      InParameters->Get ("DLSSG.MultiFrameCount", &uiMultiFrameCount);

      __SK_IsDLSSGActive =
        ( uiEnableDLSSGInterp );

      __SK_ForceDLSSGPacing = __SK_IsDLSSGActive;

      SK_LOGi0 (L"DLSS-G Feature Created1!");
    }

    else if (InFeatureID == NVSDK_NGX_Feature_SuperSampling)
    {
      //SK_ReleaseAssert ( SK_NGX_VULKAN.super_sampling.Handle == *OutHandle ||
      //                   SK_NGX_VULKAN.super_sampling.Handle == nullptr );

      SK_DLSS_Context::dlss_s::instance_s instance;

      instance.Handle     = *OutHandle;
      instance.Parameters = InParameters;
      instance.DLSS_Type  = InFeatureID;

      SK_NGX_VULKAN.super_sampling.Instances [*OutHandle] = instance;

      SK_LOGi0 (L"DLSS Feature Created1!");
    }
  }

  else
  {
    SK_LOGi0 (
      L"NVSDK_NGX_VULKAN_CreateFeature (..., %d, ...) Failed - %x (%ws)",
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
NVSDK_NGX_VULKAN_EvaluateFeature_Detour (VkCommandBuffer InCmdList, const NVSDK_NGX_Handle *InFeatureHandle, const NVSDK_NGX_Parameter *InParameters, PFN_NVSDK_NGX_ProgressCallback InCallback)
{
  SK_LOG_FIRST_CALL

  SK_NGX_VULKAN.log_call ();

  if (SK_NGX_VULKAN.super_sampling.hasInstance (InFeatureHandle))
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

    if (ReadULong64Acquire (&SK_NGX_VULKAN.super_sampling.ResetFrame) >
        ReadULong64Acquire (&SK_NGX_VULKAN.super_sampling.LastFrame))
    {
      NVSDK_NGX_Parameter_SetI_Original ((NVSDK_NGX_Parameter *)InParameters, "Reset", 1);
    }
  }

  NVSDK_NGX_Result ret =
    NVSDK_NGX_VULKAN_EvaluateFeature_Original (InCmdList, InFeatureHandle, InParameters, InCallback);

  if (NVSDK_NGX_SUCCEED (ret))
  {
    auto dlss_g = SK_NGX_VULKAN.frame_gen.getInstance      (InFeatureHandle);
    auto dlss   = SK_NGX_VULKAN.super_sampling.getInstance (InFeatureHandle);

    if (dlss_g != nullptr)
    {
      // These things are unsupported in Vulkan for the time being
      config.render.framerate.streamline.enable_native_limit = false;
    }

    SK_NGX_VULKAN.frame_gen.     evaluateFeature (dlss_g);
    SK_NGX_VULKAN.super_sampling.evaluateFeature (dlss  );
  }

  else
  {
    const wchar_t* wszFeatureName =
      SK_NGX_VULKAN.frame_gen.hasInstance      (InFeatureHandle) != 0 ? L"DLSS Frame Generation" :
      SK_NGX_VULKAN.super_sampling.hasInstance (InFeatureHandle) != 0 ? L"DLSS"                  :
                                                                        L"Unknown Feature";

    SK_LOGi0 (
      L"NVSDK_NGX_VULKAN_EvaluateFeature (%p, %ws, %p, %p) Failed - %x (%ws)",
        InCmdList, wszFeatureName, InParameters, InCallback, ret,
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
NVSDK_NGX_VULKAN_ReleaseFeature_Detour (NVSDK_NGX_Handle *InHandle)
{
  SK_LOG_FIRST_CALL

  SK_NGX_VULKAN.log_call ();

  std::lock_guard
    lock (SK_NGX_Threading->locks.Params);

  NVSDK_NGX_Result ret =
    NVSDK_NGX_VULKAN_ReleaseFeature_Original (InHandle);

  if (NVSDK_NGX_SUCCEED (ret))
  {
    auto pFrameGenInstance  = SK_NGX_VULKAN.frame_gen.getInstance (InHandle);
    if ( pFrameGenInstance != nullptr )
    {
      pFrameGenInstance->Parameters = nullptr;
      pFrameGenInstance->Handle     = nullptr;

      if (SK_NGX_VULKAN.frame_gen.LastInstance == pFrameGenInstance)
      {
        __SK_IsDLSSGActive    = false;
        __SK_ForceDLSSGPacing = false;
      }

      SK_LOGi1 (L"DLSS-G Feature Released!");
    }

    auto pSuperSamplingInstance  = SK_NGX_VULKAN.super_sampling.getInstance (InHandle);
    if ( pSuperSamplingInstance != nullptr )
    {
      pSuperSamplingInstance->Parameters = nullptr;
      pSuperSamplingInstance->Handle     = nullptr;

      SK_LOGi1 (L"DLSS Feature Released!");
    }
  }

  return ret;
}

void
SK_NGXVK_UpdateDLSSGStatus (void)
{
  if (! SK_NGX_VULKAN.apis_called)
    return;

  std::lock_guard
    lock (SK_NGX_Threading->locks.Params);

  UINT uiEnableDLSSGInterp = 0;
  UINT uiMultiFrameCount   = 1;

  auto lastFrameGen = SK_NGX_VULKAN.frame_gen.LastInstance;

  if (lastFrameGen             != nullptr &&
      lastFrameGen->Parameters != nullptr)
  {
    lastFrameGen->Parameters->Get ("DLSSG.EnableInterp",    &uiEnableDLSSGInterp);
    lastFrameGen->Parameters->Get ("DLSSG.MultiFrameCount", &uiMultiFrameCount);
  }

  __SK_IsDLSSGActive =
    ReadULong64Acquire (&SK_NGX_VULKAN.frame_gen.LastFrame) >= SK_GetFramesDrawn () - 8 &&
                       uiEnableDLSSGInterp;

  if (lastFrameGen         != nullptr &&
      lastFrameGen->Handle != nullptr)
  {
    static UINT        uiLastDLSSGState = UINT_MAX;
    if (std::exchange (uiLastDLSSGState, (UINT)__SK_IsDLSSGActive) != (UINT)__SK_IsDLSSGActive)
    {
      SK_LOGi0 ( L"DLSS-G Feature %ws!",
                  __SK_IsDLSSGActive ? L"Enabled"
                                     : L"Disabled" );
    }
  }

  __SK_ForceDLSSGPacing     = __SK_IsDLSSGActive;
  __SK_DLSSGMultiFrameCount = uiMultiFrameCount;
}

NVSDK_NGX_Result
NVSDK_CONV
NVSDK_NGX_VULKAN_Shutdown (void);

NVSDK_NGX_Result
NVSDK_CONV
NVSDK_NGX_VULKAN_Shutdown1 (VkDevice InDevice);

void
SK_NGX_InitVULKAN (void)
{
  SK_RunOnce (
  {
    SK_NGX_EstablishDLSSVersion (L"nvngx_dlss.dll");

    SK_CreateDLLHook2 ( L"_nvngx.dll",
                         "NVSDK_NGX_VULKAN_Init",
                          NVSDK_NGX_VULKAN_Init_Detour,
                (void **)&NVSDK_NGX_VULKAN_Init_Original );

    SK_CreateDLLHook2 ( L"_nvngx.dll",
                         "NVSDK_NGX_VULKAN_Init_ProjectID",
                          NVSDK_NGX_VULKAN_Init_ProjectID_Detour,
                (void **)&NVSDK_NGX_VULKAN_Init_ProjectID_Original );

    SK_CreateDLLHook2 ( L"_nvngx.dll",
                         "NVSDK_NGX_VULKAN_CreateFeature",
                          NVSDK_NGX_VULKAN_CreateFeature_Detour,
                (void **)&NVSDK_NGX_VULKAN_CreateFeature_Original );

    SK_CreateDLLHook2 ( L"_nvngx.dll",
                         "NVSDK_NGX_VULKAN_CreateFeature1",
                          NVSDK_NGX_VULKAN_CreateFeature1_Detour,
                (void **)&NVSDK_NGX_VULKAN_CreateFeature1_Original );

    SK_CreateDLLHook2 ( L"_nvngx.dll",
                           "NVSDK_NGX_VULKAN_ReleaseFeature",
                            NVSDK_NGX_VULKAN_ReleaseFeature_Detour,
                  (void **)&NVSDK_NGX_VULKAN_ReleaseFeature_Original );

    SK_CreateDLLHook2 ( L"_nvngx.dll",
                           "NVSDK_NGX_VULKAN_EvaluateFeature",
                            NVSDK_NGX_VULKAN_EvaluateFeature_Detour,
                  (void **)&NVSDK_NGX_VULKAN_EvaluateFeature_Original );

    SK_CreateDLLHook2 ( L"_nvngx.dll",
                           "NVSDK_NGX_VULKAN_DestroyParameters",
                            NVSDK_NGX_VULKAN_DestroyParameters_Detour,
                  (void **)&NVSDK_NGX_VULKAN_DestroyParameters_Original );

    SK_CreateDLLHook2 ( L"_nvngx.dll",
                           "NVSDK_NGX_VULKAN_AllocateParameters",
                            NVSDK_NGX_VULKAN_AllocateParameters_Detour,
                  (void **)&NVSDK_NGX_VULKAN_AllocateParameters_Original );

    SK_CreateDLLHook2 ( L"_nvngx.dll",
                           "NVSDK_NGX_VULKAN_GetParameters",
                            NVSDK_NGX_VULKAN_GetParameters_Detour,
                  (void **)&NVSDK_NGX_VULKAN_GetParameters_Original );

    SK_CreateDLLHook2 ( L"_nvngx.dll",
                           "NVSDK_NGX_VULKAN_GetCapabilityParameters",
                            NVSDK_NGX_VULKAN_GetCapabilityParameters_Detour,
                  (void **)&NVSDK_NGX_VULKAN_GetCapabilityParameters_Original );
  });
}