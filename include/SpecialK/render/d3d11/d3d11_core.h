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

struct IUnknown;
#include <Unknwnbase.h>
#include <Windows.h>
#include <Wincodec.h>

#include <SpecialK/diagnostics/cpu.h>
#include <SpecialK/diagnostics/modules.h>
#include <SpecialK/diagnostics/load_library.h>
#include <SpecialK/render/dxgi/dxgi_backend.h>
#include <SpecialK/render/backend.h>

#include <SpecialK/import.h>

#include <SpecialK/diagnostics/debug_utils.h>
#include <SpecialK/diagnostics/compatibility.h>

#include <SpecialK/plugin/reshade.h>

#include <SpecialK/core.h>
#include <SpecialK/hooks.h>
#include <SpecialK/command.h>
#include <SpecialK/config.h>
#include <SpecialK/log.h>
#include <SpecialK/crc32.h>
#include <SpecialK/utility.h>
#include <SpecialK/thread.h>

#include <DirectXTex/DirectXTex.h>

#include <SpecialK/widgets/widget.h>
#include <SpecialK/framerate.h>
#include <SpecialK/tls.h>

#include <SpecialK/control_panel.h>

#include <gsl/gsl>

#define _SK_WITHOUT_D3DX11

#include <cinttypes>
#include <algorithm>
#include <memory>
#include <atomic>
#include <mutex>
#include <stack>
#include <concurrent_unordered_set.h>
#include <concurrent_queue.h>
#include <atlbase.h>

#include <d3d11.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>

#include <../depends/include/nvapi/nvapi.h>

#define SK_D3D11_IsDevCtxDeferred(ctx) (ctx)->GetType () == D3D11_DEVICE_CONTEXT_DEFERRED

extern SK_Thread_HybridSpinlock* cs_shader;
extern SK_Thread_HybridSpinlock* cs_shader_vs;
extern SK_Thread_HybridSpinlock* cs_shader_ps;
extern SK_Thread_HybridSpinlock* cs_shader_gs;
extern SK_Thread_HybridSpinlock* cs_shader_hs;
extern SK_Thread_HybridSpinlock* cs_shader_ds;
extern SK_Thread_HybridSpinlock* cs_shader_cs;
extern SK_Thread_HybridSpinlock* cs_mmio;
extern SK_Thread_HybridSpinlock* cs_render_view;


// VFTABLE Hooks
extern D3D11Dev_CreateRasterizerState_pfn                  D3D11Dev_CreateRasterizerState_Original;
extern D3D11Dev_CreateSamplerState_pfn                     D3D11Dev_CreateSamplerState_Original;
extern D3D11Dev_CreateBuffer_pfn                           D3D11Dev_CreateBuffer_Original;
extern D3D11Dev_CreateTexture2D_pfn                        D3D11Dev_CreateTexture2D_Original;
extern D3D11Dev_CreateRenderTargetView_pfn                 D3D11Dev_CreateRenderTargetView_Original;
extern D3D11Dev_CreateShaderResourceView_pfn               D3D11Dev_CreateShaderResourceView_Original;
extern D3D11Dev_CreateDepthStencilView_pfn                 D3D11Dev_CreateDepthStencilView_Original;
extern D3D11Dev_CreateUnorderedAccessView_pfn              D3D11Dev_CreateUnorderedAccessView_Original;

extern D3D11Dev_CreateVertexShader_pfn                     D3D11Dev_CreateVertexShader_Original;
extern D3D11Dev_CreatePixelShader_pfn                      D3D11Dev_CreatePixelShader_Original;
extern D3D11Dev_CreateGeometryShader_pfn                   D3D11Dev_CreateGeometryShader_Original;
extern D3D11Dev_CreateGeometryShaderWithStreamOutput_pfn   D3D11Dev_CreateGeometryShaderWithStreamOutput_Original;
extern D3D11Dev_CreateHullShader_pfn                       D3D11Dev_CreateHullShader_Original;
extern D3D11Dev_CreateDomainShader_pfn                     D3D11Dev_CreateDomainShader_Original;
extern D3D11Dev_CreateComputeShader_pfn                    D3D11Dev_CreateComputeShader_Original;

extern D3D11Dev_CreateDeferredContext_pfn                  D3D11Dev_CreateDeferredContext_Original;
extern D3D11Dev_CreateDeferredContext1_pfn                 D3D11Dev_CreateDeferredContext1_Original;
extern D3D11Dev_CreateDeferredContext2_pfn                 D3D11Dev_CreateDeferredContext2_Original;
extern D3D11Dev_CreateDeferredContext3_pfn                 D3D11Dev_CreateDeferredContext3_Original;
extern D3D11Dev_GetImmediateContext_pfn                    D3D11Dev_GetImmediateContext_Original;
extern D3D11Dev_GetImmediateContext1_pfn                   D3D11Dev_GetImmediateContext1_Original;
extern D3D11Dev_GetImmediateContext2_pfn                   D3D11Dev_GetImmediateContext2_Original;
extern D3D11Dev_GetImmediateContext3_pfn                   D3D11Dev_GetImmediateContext3_Original;

extern D3D11_RSSetScissorRects_pfn                         D3D11_RSSetScissorRects_Original;
extern D3D11_RSSetViewports_pfn                            D3D11_RSSetViewports_Original;
extern D3D11_VSSetConstantBuffers_pfn                      D3D11_VSSetConstantBuffers_Original;
extern D3D11_VSSetShaderResources_pfn                      D3D11_VSSetShaderResources_Original;
extern D3D11_PSSetShaderResources_pfn                      D3D11_PSSetShaderResources_Original;
extern D3D11_PSSetConstantBuffers_pfn                      D3D11_PSSetConstantBuffers_Original;
extern D3D11_GSSetShaderResources_pfn                      D3D11_GSSetShaderResources_Original;
extern D3D11_HSSetShaderResources_pfn                      D3D11_HSSetShaderResources_Original;
extern D3D11_DSSetShaderResources_pfn                      D3D11_DSSetShaderResources_Original;
extern D3D11_CSSetShaderResources_pfn                      D3D11_CSSetShaderResources_Original;
extern D3D11_CSSetUnorderedAccessViews_pfn                 D3D11_CSSetUnorderedAccessViews_Original;
extern D3D11_UpdateSubresource_pfn                         D3D11_UpdateSubresource_Original;
extern D3D11_DrawIndexed_pfn                               D3D11_DrawIndexed_Original;
extern D3D11_Draw_pfn                                      D3D11_Draw_Original;
extern D3D11_DrawAuto_pfn                                  D3D11_DrawAuto_Original;
extern D3D11_DrawIndexedInstanced_pfn                      D3D11_DrawIndexedInstanced_Original;
extern D3D11_DrawIndexedInstancedIndirect_pfn              D3D11_DrawIndexedInstancedIndirect_Original;
extern D3D11_DrawInstanced_pfn                             D3D11_DrawInstanced_Original;
extern D3D11_DrawInstancedIndirect_pfn                     D3D11_DrawInstancedIndirect_Original;
extern D3D11_Dispatch_pfn                                  D3D11_Dispatch_Original;
extern D3D11_DispatchIndirect_pfn                          D3D11_DispatchIndirect_Original;
extern D3D11_Map_pfn                                       D3D11_Map_Original;
extern D3D11_Unmap_pfn                                     D3D11_Unmap_Original;

extern D3D11_OMSetRenderTargets_pfn                        D3D11_OMSetRenderTargets_Original;
extern D3D11_OMSetRenderTargetsAndUnorderedAccessViews_pfn D3D11_OMSetRenderTargetsAndUnorderedAccessViews_Original;
extern D3D11_OMGetRenderTargets_pfn                        D3D11_OMGetRenderTargets_Original;
extern D3D11_OMGetRenderTargetsAndUnorderedAccessViews_pfn D3D11_OMGetRenderTargetsAndUnorderedAccessViews_Original;
extern D3D11_ClearDepthStencilView_pfn                     D3D11_ClearDepthStencilView_Original;

extern D3D11_PSSetSamplers_pfn                             D3D11_PSSetSamplers_Original;

extern D3D11_VSSetShader_pfn                               D3D11_VSSetShader_Original;
extern D3D11_PSSetShader_pfn                               D3D11_PSSetShader_Original;
extern D3D11_GSSetShader_pfn                               D3D11_GSSetShader_Original;
extern D3D11_HSSetShader_pfn                               D3D11_HSSetShader_Original;
extern D3D11_DSSetShader_pfn                               D3D11_DSSetShader_Original;
extern D3D11_CSSetShader_pfn                               D3D11_CSSetShader_Original;

extern D3D11_VSGetShader_pfn                               D3D11_VSGetShader_Original;
extern D3D11_PSGetShader_pfn                               D3D11_PSGetShader_Original;
extern D3D11_GSGetShader_pfn                               D3D11_GSGetShader_Original;
extern D3D11_HSGetShader_pfn                               D3D11_HSGetShader_Original;
extern D3D11_DSGetShader_pfn                               D3D11_DSGetShader_Original;
extern D3D11_CSGetShader_pfn                               D3D11_CSGetShader_Original;

extern D3D11_GetData_pfn                                   D3D11_GetData_Original;

extern D3D11_CopyResource_pfn                              D3D11_CopyResource_Original;
extern D3D11_CopySubresourceRegion_pfn                     D3D11_CopySubresourceRegion_Original;
extern D3D11_UpdateSubresource1_pfn                        D3D11_UpdateSubresource1_Original;




using SK_ReShade_PresentCallback_pfn              = bool (__stdcall *)(void *user);
using SK_ReShade_OnCopyResourceD3D11_pfn          = void (__stdcall *)(void *user, ID3D11Resource         *&dest,    ID3D11Resource *&source);
using SK_ReShade_OnClearDepthStencilViewD3D11_pfn = void (__stdcall *)(void *user, ID3D11DepthStencilView *&depthstencil);



extern "C" __declspec (dllexport) extern FARPROC D3D11CreateDeviceForD3D12;
extern "C" __declspec (dllexport) extern FARPROC CreateDirect3D11DeviceFromDXGIDevice;
extern "C" __declspec (dllexport) extern FARPROC CreateDirect3D11SurfaceFromDXGISurface;
extern "C" __declspec (dllexport) extern FARPROC D3D11On12CreateDevice;
extern "C" __declspec (dllexport) extern FARPROC D3DKMTCloseAdapter;
extern "C" __declspec (dllexport) extern FARPROC D3DKMTDestroyAllocation;
extern "C" __declspec (dllexport) extern FARPROC D3DKMTDestroyContext;
extern "C" __declspec (dllexport) extern FARPROC D3DKMTDestroyDevice;
extern "C" __declspec (dllexport) extern FARPROC D3DKMTDestroySynchronizationObject;
extern "C" __declspec (dllexport) extern FARPROC D3DKMTQueryAdapterInfo;
extern "C" __declspec (dllexport) extern FARPROC D3DKMTSetDisplayPrivateDriverFormat;
extern "C" __declspec (dllexport) extern FARPROC D3DKMTSignalSynchronizationObject;
extern "C" __declspec (dllexport) extern FARPROC D3DKMTUnlock;
extern "C" __declspec (dllexport) extern FARPROC D3DKMTWaitForSynchronizationObject;
extern "C" __declspec (dllexport) extern FARPROC EnableFeatureLevelUpgrade;
extern "C" __declspec (dllexport) extern FARPROC OpenAdapter10;
extern "C" __declspec (dllexport) extern FARPROC OpenAdapter10_2;
extern "C" __declspec (dllexport) extern FARPROC D3D11CoreCreateLayeredDevice;
extern "C" __declspec (dllexport) extern FARPROC D3D11CoreGetLayeredDeviceSize;
extern "C" __declspec (dllexport) extern FARPROC D3D11CoreRegisterLayers;
extern "C" __declspec (dllexport) extern FARPROC D3DKMTCreateAllocation;
extern "C" __declspec (dllexport) extern FARPROC D3DKMTCreateContext;
extern "C" __declspec (dllexport) extern FARPROC D3DKMTCreateDevice;
extern "C" __declspec (dllexport) extern FARPROC D3DKMTCreateSynchronizationObject;
extern "C" __declspec (dllexport) extern FARPROC D3DKMTEscape;
extern "C" __declspec (dllexport) extern FARPROC D3DKMTGetContextSchedulingPriority;
extern "C" __declspec (dllexport) extern FARPROC D3DKMTGetDeviceState;
extern "C" __declspec (dllexport) extern FARPROC D3DKMTGetDisplayModeList;
extern "C" __declspec (dllexport) extern FARPROC D3DKMTGetMultisampleMethodList;
extern "C" __declspec (dllexport) extern FARPROC D3DKMTGetRuntimeData;
extern "C" __declspec (dllexport) extern FARPROC D3DKMTGetSharedPrimaryHandle;
extern "C" __declspec (dllexport) extern FARPROC D3DKMTLock;
extern "C" __declspec (dllexport) extern FARPROC D3DKMTOpenAdapterFromHdc;
extern "C" __declspec (dllexport) extern FARPROC D3DKMTOpenResource;
extern "C" __declspec (dllexport) extern FARPROC D3DKMTPresent;
extern "C" __declspec (dllexport) extern FARPROC D3DKMTQueryAllocationResidency;
extern "C" __declspec (dllexport) extern FARPROC D3DKMTQueryResourceInfo;
extern "C" __declspec (dllexport) extern FARPROC D3DKMTRender;
extern "C" __declspec (dllexport) extern FARPROC D3DKMTSetAllocationPriority;
extern "C" __declspec (dllexport) extern FARPROC D3DKMTSetContextSchedulingPriority;
extern "C" __declspec (dllexport) extern FARPROC D3DKMTSetDisplayMode;
extern "C" __declspec (dllexport) extern FARPROC D3DKMTSetGammaRamp;
extern "C" __declspec (dllexport) extern FARPROC D3DKMTSetVidPnSourceOwner;
extern "C" __declspec (dllexport) extern FARPROC D3DKMTWaitForVerticalBlankEvent;
extern "C" __declspec (dllexport) extern FARPROC D3DPerformance_BeginEvent;
extern "C" __declspec (dllexport) extern FARPROC D3DPerformance_EndEvent;
extern "C" __declspec (dllexport) extern FARPROC D3DPerformance_GetStatus;
extern "C" __declspec (dllexport) extern FARPROC D3DPerformance_SetMarker;

using SK_ReShade_OnGetDepthStencilViewD3D11_pfn   = void (__stdcall *)(void *user, ID3D11DepthStencilView *&depthstencil);
using SK_ReShade_OnSetDepthStencilViewD3D11_pfn   = void (__stdcall *)(void *user, ID3D11DepthStencilView *&depthstencil);
using SK_ReShade_OnDrawD3D11_pfn                  = void (__stdcall *)(void *user, ID3D11DeviceContext     *context, unsigned int   vertices);

struct SK_RESHADE_CALLBACK_DRAW
{
  SK_ReShade_OnDrawD3D11_pfn fn   = nullptr;
  void*                      data = nullptr;
  __forceinline void call (ID3D11DeviceContext *context, unsigned int vertices, SK_TLS* pTLS = nullptr)
  { if (data != nullptr && fn != nullptr && (pTLS == nullptr || (! pTLS->imgui.drawing))) fn (data, context, vertices); }
} extern SK_ReShade_DrawCallback;


static
uint32_t
__cdecl
SK_D3D11_ChecksumShaderBytecode ( _In_      const void                *pShaderBytecode,
                                  _In_            SIZE_T               BytecodeLength  )
{
  __try
  {
    return
      safe_crc32c (0x00, static_cast <const uint8_t *> (pShaderBytecode), BytecodeLength);
  }

  __except ( ( GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ) ?
             EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH )
  {
    SK_LOG0 ( (L" >> Threw out disassembled shader due to access violation during hash."),
               L"   DXGI   ");
    return 0x00;
  }
}