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

#define __SK_SUBSYSTEM__ L"  D3D 11  "

#include <SpecialK/render/d3d11/d3d11_core.h>

extern "C" __declspec (dllexport) FARPROC D3D11CreateDeviceForD3D12              = nullptr;
extern "C" __declspec (dllexport) FARPROC CreateDirect3D11DeviceFromDXGIDevice   = nullptr;
extern "C" __declspec (dllexport) FARPROC CreateDirect3D11SurfaceFromDXGISurface = nullptr;
extern "C" __declspec (dllexport) FARPROC D3D11On12CreateDevice                  = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTCloseAdapter                     = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTDestroyAllocation                = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTDestroyContext                   = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTDestroyDevice                    = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTDestroySynchronizationObject     = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTQueryAdapterInfo                 = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTSetDisplayPrivateDriverFormat    = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTSignalSynchronizationObject      = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTUnlock                           = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTWaitForSynchronizationObject     = nullptr;
extern "C" __declspec (dllexport) FARPROC EnableFeatureLevelUpgrade              = nullptr;
extern "C" __declspec (dllexport) FARPROC OpenAdapter10                          = nullptr;
extern "C" __declspec (dllexport) FARPROC OpenAdapter10_2                        = nullptr;
extern "C" __declspec (dllexport) FARPROC D3D11CoreCreateLayeredDevice           = nullptr;
extern "C" __declspec (dllexport) FARPROC D3D11CoreGetLayeredDeviceSize          = nullptr;
extern "C" __declspec (dllexport) FARPROC D3D11CoreRegisterLayers                = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTCreateAllocation                 = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTCreateContext                    = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTCreateDevice                     = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTCreateSynchronizationObject      = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTEscape                           = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTGetContextSchedulingPriority     = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTGetDeviceState                   = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTGetDisplayModeList               = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTGetMultisampleMethodList         = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTGetRuntimeData                   = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTGetSharedPrimaryHandle           = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTLock                             = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTOpenAdapterFromHdc               = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTOpenResource                     = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTPresent                          = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTQueryAllocationResidency         = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTQueryResourceInfo                = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTRender                           = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTSetAllocationPriority            = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTSetContextSchedulingPriority     = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTSetDisplayMode                   = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTSetGammaRamp                     = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTSetVidPnSourceOwner              = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTWaitForVerticalBlankEvent        = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DPerformance_BeginEvent              = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DPerformance_EndEvent                = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DPerformance_GetStatus               = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DPerformance_SetMarker               = nullptr;