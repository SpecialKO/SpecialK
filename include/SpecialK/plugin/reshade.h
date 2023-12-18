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

#ifndef __SK__RESHADE_H__
#define __SK__RESHADE_H__

#include <Unknwnbase.h>

#include <Windows.h>

#include <../depends/include/ReShade/reshade_api.hpp>

HMODULE
__stdcall
SK_ReShade_GetDLL (void);

void
SK_ReShade_LoadIfPresent (void);

UINT64 SK_ReShadeAddOn_RenderEffectsD3D12   (IDXGISwapChain1*, ID3D12Resource*, ID3D12Fence*, D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_CPU_DESCRIPTOR_HANDLE);
bool   SK_ReShadeAddOn_RenderEffectsD3D11   (IDXGISwapChain1*); // TODO: Make generic to post-process non-SwapChain buffers
bool   SK_ReShadeAddOn_RenderEffectsD3D11Ex (IDXGISwapChain1 *pSwapChain, ID3D11RenderTargetView *pRTV, ID3D11RenderTargetView *pRTV_sRGB);
bool   SK_ReShadeAddOn_Init               (HMODULE          reshade_module = nullptr);
void   SK_ReShadeAddOn_ActivateOverlay    (bool             activate);
bool   SK_ReShadeAddOn_IsOverlayActive    (void);

reshade::api::effect_runtime*
     SK_ReShadeAddOn_GetRuntimeForHWND      (HWND hWnd);
reshade::api::effect_runtime*
     SK_ReShadeAddOn_GetRuntimeForSwapChain (IDXGISwapChain* pSwapChain);
void SK_ReShadeAddOn_CleanupRTVs            (reshade::api::effect_runtime *runtime, bool must_wait = false);

reshade::api::effect_runtime*
SK_ReShadeAddOn_CreateEffectRuntime_D3D12 (ID3D12Device *pDevice, ID3D12CommandQueue *pCmdQueue, IDXGISwapChain *pSwapChain);

reshade::api::effect_runtime*
SK_ReShadeAddOn_CreateEffectRuntime_D3D11 (ID3D11Device *pDevice, ID3D11DeviceContext *pDevCtx, IDXGISwapChain *pSwapChain);

void SK_ReShadeAddOn_UpdateAndPresentEffectRuntime (reshade::api::effect_runtime *runtime);
void SK_ReShadeAddOn_DestroyEffectRuntime          (reshade::api::effect_runtime *runtime);

void SK_ReShadeAddOn_CleanupConfigAndLogs (void);

#endif /* __SK__RESHADE_H__ */