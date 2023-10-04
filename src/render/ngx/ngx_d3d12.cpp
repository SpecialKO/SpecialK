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

typedef void (NVSDK_CONV *PFN_NVSDK_NGX_ProgressCallback)(float InCurrentProgress, bool &OutShouldCancel);

NVSDK_NGX_Handle*    SK_NGX_DLSSG_Handle     = nullptr;
NVSDK_NGX_Parameter* SK_NGX_DLSSG_Parameters = nullptr;
volatile ULONG64     SK_NGX_DLSSG_LastFrame  = 0ULL;

// NGX return-code conversion-to-string utility only as a helper for debugging/logging - not for official use.
using  GetNGXResultAsString_pfn = const wchar_t* (NVSDK_CONV *)(NVSDK_NGX_Result InNGXResult);
static GetNGXResultAsString_pfn
       GetNGXResultAsString = nullptr;

using NVSDK_NGX_D3D12_DestroyParameters_pfn       = NVSDK_NGX_Result (NVSDK_CONV *)(NVSDK_NGX_Parameter* InParameters);
using NVSDK_NGX_D3D12_GetCapabilityParameters_pfn = NVSDK_NGX_Result (NVSDK_CONV *)(NVSDK_NGX_Parameter** OutParameters);

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
static NVSDK_NGX_D3D12_DestroyParameters_pfn
       NVSDK_NGX_D3D12_DestroyParameters_Original = nullptr;
static NVSDK_NGX_D3D12_GetCapabilityParameters_pfn
       NVSDK_NGX_D3D12_GetCapabilityParameters = nullptr;
static NVSDK_NGX_D3D12_GetParameters_pfn
       NVSDK_NGX_D3D12_GetParameters = nullptr;
static NVSDK_NGX_D3D12_ReleaseFeature_pfn
       NVSDK_NGX_D3D12_ReleaseFeature_Original = nullptr;

NVSDK_NGX_Result
NVSDK_CONV
NVSDK_NGX_D3D12_DestroyParameters_Detour (NVSDK_NGX_Parameter* InParameters)
{
  SK_LOG_FIRST_CALL

  std::lock_guard
    lock (SK_NGX_Threading->locks.Params);

  NVSDK_NGX_Result ret =
    NVSDK_NGX_D3D12_DestroyParameters_Original (InParameters);

  if (InParameters == SK_NGX_DLSSG_Parameters)
  {
    if (ret == NVSDK_NGX_Result_Success)
    {
      SK_NGX_DLSSG_Parameters = nullptr;
    }
  }

  return ret;
}

NVSDK_NGX_Result
NVSDK_CONV
NVSDK_NGX_D3D12_CreateFeature_Detour ( ID3D12GraphicsCommandList *InCmdList,
                                       NVSDK_NGX_Feature          InFeatureID,
                                       NVSDK_NGX_Parameter       *InParameters,
                                       NVSDK_NGX_Handle         **OutHandle )
{
  SK_LOG_FIRST_CALL

  std::lock_guard
    lock (SK_NGX_Threading->locks.Params);

  NVSDK_NGX_Result ret =
    NVSDK_NGX_D3D12_CreateFeature_Original ( InCmdList,
                                             InFeatureID,
                                             InParameters, OutHandle );

  if ( ret == NVSDK_NGX_Result_Success ||
       ret == NVSDK_NGX_Result_FAIL_FeatureAlreadyExists )
  {
    if (InFeatureID == NVSDK_NGX_Feature_FrameGeneration)
    {
      SK_ReleaseAssert ( SK_NGX_DLSSG_Handle == *OutHandle ||
                         SK_NGX_DLSSG_Handle == nullptr );

      SK_NGX_DLSSG_Handle     = *OutHandle;
      SK_NGX_DLSSG_Parameters = InParameters;

      __SK_HasDLSSGStatusSupport = true;

      UINT uiEnableOFA         = 0;
      UINT uiEnableDLSSGInterp = 0;

      InParameters->Get ("Enable.OFA",         &uiEnableOFA);
      InParameters->Get ("DLSSG.EnableInterp", &uiEnableDLSSGInterp);

      __SK_IsDLSSGActive =
        ( uiEnableOFA && uiEnableDLSSGInterp );

      __SK_ForceDLSSGPacing = __SK_IsDLSSGActive;

      SK_LOGi0 (L"DLSS-G Feature Created!");
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
  NVSDK_NGX_Result ret =
    NVSDK_NGX_D3D12_EvaluateFeature_Original (InCmdList, InFeatureHandle, InParameters, InCallback);

  if (ret == NVSDK_NGX_Result_Success)
  {
    if (InFeatureHandle == SK_NGX_DLSSG_Handle)
    {
      WriteULong64Release (&SK_NGX_DLSSG_LastFrame, SK_GetFramesDrawn ());
    }
  }

  return ret;
}


NVSDK_NGX_Result
NVSDK_CONV
NVSDK_NGX_D3D12_ReleaseFeature_Detour (NVSDK_NGX_Handle *InHandle)
{
  SK_LOG_FIRST_CALL

  std::lock_guard
    lock (SK_NGX_Threading->locks.Params);

  NVSDK_NGX_Result ret =
    NVSDK_NGX_D3D12_ReleaseFeature_Original (InHandle);

  if (ret == NVSDK_NGX_Result_Success)
  {
    if (InHandle == SK_NGX_DLSSG_Handle)
    {
      SK_NGX_DLSSG_Parameters = nullptr;
      SK_NGX_DLSSG_Handle     = nullptr;
      __SK_IsDLSSGActive      = false;
      __SK_ForceDLSSGPacing   = false;

      SK_LOGi0 (L"DLSS-G Feature Released!");
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
  std::lock_guard
    lock (SK_NGX_Threading->locks.Params);

  UINT uiNumberOfFrames    = 0;
  UINT uiEnableOFA         = 0;
  UINT uiEnableDLSSGInterp = 0;

  if (SK_NGX_DLSSG_Parameters != nullptr)
  {
    SK_NGX_DLSSG_Parameters->Get ("Enable.OFA",                   &uiEnableOFA);
    SK_NGX_DLSSG_Parameters->Get ("DLSSG.EnableInterp",           &uiEnableDLSSGInterp);
    SK_NGX_DLSSG_Parameters->Get ("DLSSG.NumFrames",              &uiNumberOfFrames);

#if 0
    ID3D12Resource*                               pMvecsRes = nullptr;
    SK_NGX_DLSSG_Parameters->Get ("DLSSG.MVecs", &pMvecsRes);

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
    ReadULong64Acquire (&SK_NGX_DLSSG_LastFrame) >= SK_GetFramesDrawn () - 8 &&
                       uiNumberOfFrames >= 1 &&
                       uiEnableDLSSGInterp   && uiEnableOFA;

  if (SK_NGX_DLSSG_Handle != nullptr)
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

    SK_ApplyQueuedHooks ();

    GetNGXResultAsString =
   (GetNGXResultAsString_pfn)SK_GetProcAddress ( L"_nvngx.dll",
   "GetNGXResultAsString" );

    NVSDK_NGX_D3D12_GetParameters =
   (NVSDK_NGX_D3D12_GetParameters_pfn)SK_GetProcAddress ( L"_nvngx.dll",
   "NVSDK_NGX_D3D12_GetParameters" );

    NVSDK_NGX_D3D12_GetCapabilityParameters =
   (NVSDK_NGX_D3D12_GetCapabilityParameters_pfn)SK_GetProcAddress ( L"_nvngx.dll",
   "NVSDK_NGX_D3D12_GetCapabilityParameters" );
  });
}