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

#pragma once

#include <render/ngx/ngx_defs.h>

NVSDK_NGX_Parameter* SK_NGX_GetDLSSParameters (void);
bool                 SK_NGX_IsUsingDLSS       (void);
bool                 SK_NGX_IsUsingDLSS_D     (void);
bool                 SK_NGX_IsUsingDLSS_G     (void);
void                 SK_NGX_DLSS_CreateFeatureOverrideParams (NVSDK_NGX_Parameter *InParameters);
void                 SK_NGX_DLSS_ControlPanel (void);
void                 SK_NGX_DLSS_GetInternalResolution    (int& x, int& y);
const char*          SK_NGX_DLSS_GetCurrentPerfQualityStr (void);
const char*          SK_NGX_DLSS_GetCurrentPresetStr      (void);

extern bool __SK_HasDLSSGStatusSupport;
extern bool __SK_IsDLSSGActive;
extern bool __SK_DoubleUpOnReflex;
extern bool __SK_ForceDLSSGPacing;

void SK_NGX12_DumpBuffers_DLSSG (ID3D12GraphicsCommandList *pCommandList);