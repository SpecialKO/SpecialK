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

NVSDK_NGX_Result
NVSDK_CONV
NVSDK_NGX_D3D12_CreateFeature ( ID3D12GraphicsCommandList *InCmdList,
                                NVSDK_NGX_Feature          InFeatureID,
                                NVSDK_NGX_Parameter       *InParameters,
                                NVSDK_NGX_Handle         **OutHandle );

NVSDK_NGX_Result
NVSDK_CONV
NVSDK_NGX_D3D12_Shutdown  (void);

NVSDK_NGX_Result
NVSDK_CONV
NVSDK_NGX_D3D12_Shutdown1 (ID3D12Device *InDevice);

NVSDK_NGX_Result
NVSDK_CONV
NVSDK_NGX_D3D12_GetParameters (NVSDK_NGX_Parameter **OutParameters);

NVSDK_NGX_Result
NVSDK_CONV
NVSDK_NGX_D3D12_AllocateParameters (NVSDK_NGX_Parameter** OutParameters);

NVSDK_NGX_Result
NVSDK_CONV
NVSDK_NGX_D3D12_GetCapabilityParameters (NVSDK_NGX_Parameter** OutParameters);

NVSDK_NGX_Result
NVSDK_CONV
NVSDK_NGX_D3D12_GetFeatureRequirements ( IDXGIAdapter                   *Adapter,
                                   const NVSDK_NGX_FeatureDiscoveryInfo *FeatureDiscoveryInfo,
                                         NVSDK_NGX_FeatureRequirement   *OutSupported );

NVSDK_NGX_Result
NVSDK_CONV
NVSDK_NGX_UpdateFeature ( const NVSDK_NGX_Application_Identifier *ApplicationId,
                          const NVSDK_NGX_Feature                 FeatureID );

// NGX return-code conversion-to-string utility only as a helper for debugging/logging - not for official use.
const wchar_t*
NVSDK_CONV
GetNGXResultAsString (NVSDK_NGX_Result InNGXResult);