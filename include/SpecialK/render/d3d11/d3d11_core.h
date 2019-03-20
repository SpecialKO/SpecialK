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

#define SK_D3D11_IsDevCtxDeferred(ctx) (ctx)->GetType () == D3D11_DEVICE_CONTEXT_DEFERRED

extern const GUID IID_ID3D11Device2;
extern const GUID IID_ID3D11Device3;
extern const GUID IID_ID3D11Device4;
extern const GUID IID_ID3D11Device5;




__declspec (noinline)
HRESULT
WINAPI
D3D11CreateDevice_Detour (
  _In_opt_                            IDXGIAdapter         *pAdapter,
                                      D3D_DRIVER_TYPE       DriverType,
                                      HMODULE               Software,
                                      UINT                  Flags,
  _In_opt_                      const D3D_FEATURE_LEVEL    *pFeatureLevels,
                                      UINT                  FeatureLevels,
                                      UINT                  SDKVersion,
  _Out_opt_                           ID3D11Device        **ppDevice,
  _Out_opt_                           D3D_FEATURE_LEVEL    *pFeatureLevel,
  _Out_opt_                           ID3D11DeviceContext **ppImmediateContext);

__declspec (noinline)
HRESULT
WINAPI
D3D11CreateDeviceAndSwapChain_Detour (IDXGIAdapter          *pAdapter,
                                      D3D_DRIVER_TYPE        DriverType,
                                      HMODULE                Software,
                                      UINT                   Flags,
 _In_reads_opt_ (FeatureLevels) CONST D3D_FEATURE_LEVEL     *pFeatureLevels,
                                      UINT                   FeatureLevels,
                                      UINT                   SDKVersion,
 _In_opt_                       CONST DXGI_SWAP_CHAIN_DESC  *pSwapChainDesc,
 _Out_opt_                            IDXGISwapChain       **ppSwapChain,
 _Out_opt_                            ID3D11Device         **ppDevice,
 _Out_opt_                            D3D_FEATURE_LEVEL     *pFeatureLevel,
 _Out_opt_                            ID3D11DeviceContext  **ppImmediateContext);


// The device context a command list was built using
extern const GUID SKID_D3D11DeviceContextOrigin;

extern std::unique_ptr <SK_Thread_HybridSpinlock> cs_shader;
extern std::unique_ptr <SK_Thread_HybridSpinlock> cs_shader_vs;
extern std::unique_ptr <SK_Thread_HybridSpinlock> cs_shader_ps;
extern std::unique_ptr <SK_Thread_HybridSpinlock> cs_shader_gs;
extern std::unique_ptr <SK_Thread_HybridSpinlock> cs_shader_hs;
extern std::unique_ptr <SK_Thread_HybridSpinlock> cs_shader_ds;
extern std::unique_ptr <SK_Thread_HybridSpinlock> cs_shader_cs;
extern std::unique_ptr <SK_Thread_HybridSpinlock> cs_mmio;
extern std::unique_ptr <SK_Thread_HybridSpinlock> cs_render_view;


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


void
SK_D3D11_MergeCommandLists ( ID3D11DeviceContext *pSurrogate,
                             ID3D11DeviceContext *pMerge );

void
SK_D3D11_ResetContextState (ID3D11DeviceContext* pDevCtx);


struct shader_stage_s {
  void nulBind (int dev_ctx_handle, int slot, ID3D11ShaderResourceView* pView)
  {
    if (skipped_bindings [dev_ctx_handle][slot] != nullptr)
    {
      skipped_bindings [dev_ctx_handle][slot]->Release ();
      skipped_bindings [dev_ctx_handle][slot] = nullptr;
    }

    skipped_bindings [dev_ctx_handle][slot] = pView;

    if (pView != nullptr)
    {
      pView->AddRef ();
    }
  };

  void Bind (int dev_ctx_handle, int slot, ID3D11ShaderResourceView* pView)
  {
    if (skipped_bindings [dev_ctx_handle][slot] != nullptr)
    {
      skipped_bindings [dev_ctx_handle][slot]->Release ();
      skipped_bindings [dev_ctx_handle][slot] = nullptr;
    }

    skipped_bindings [dev_ctx_handle][slot] = nullptr;

    // The D3D11 Runtime is holding a reference if this is non-null.
    real_bindings    [dev_ctx_handle][slot] = pView;
  };

  // We have to hold references to these things, because the D3D11 runtime would have.
  std::array <std::array <ID3D11ShaderResourceView*, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT>, SK_D3D11_MAX_DEV_CONTEXTS+1> skipped_bindings = { };
  std::array <std::array <ID3D11ShaderResourceView*, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT>, SK_D3D11_MAX_DEV_CONTEXTS+1> real_bindings    = { };
};


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

struct SK_RESHADE_CALLBACK_SetDSV
{
  SK_ReShade_OnSetDepthStencilViewD3D11_pfn fn   = nullptr;
  void*                                     data = nullptr;

  __forceinline void try_call (ID3D11DepthStencilView *&depthstencil)
  {
    if ((uintptr_t)data + (uintptr_t)fn != 0)
      return call (depthstencil);
  }

  __forceinline void call (ID3D11DepthStencilView *&depthstencil, SK_TLS* pTLS = nullptr)
  { if (data != nullptr && fn != nullptr && (pTLS == nullptr || (! pTLS->imgui.drawing))) fn (data, depthstencil); }
} extern SK_ReShade_SetDepthStencilViewCallback;

struct SK_RESHADE_CALLBACK_GetDSV
{
  SK_ReShade_OnGetDepthStencilViewD3D11_pfn fn   = nullptr;
  void*                                     data = nullptr;

  __forceinline void try_call (ID3D11DepthStencilView *&depthstencil)
  {
    if ((uintptr_t)data + (uintptr_t)fn != 0)
      return call (depthstencil);
  }

  __forceinline void call (ID3D11DepthStencilView *&depthstencil, SK_TLS* pTLS = nullptr)
  { if (data != nullptr && fn != nullptr && (pTLS == nullptr || (! pTLS->imgui.drawing))) fn (data, depthstencil); }
} extern SK_ReShade_GetDepthStencilViewCallback;

struct SK_RESHADE_CALLBACK_ClearDSV
{
  SK_ReShade_OnClearDepthStencilViewD3D11_pfn fn   = nullptr;
  void*                                       data = nullptr;

  __forceinline void try_call (ID3D11DepthStencilView *&depthstencil)
  {
    if ((uintptr_t)data + (uintptr_t)fn != 0)
      return call (depthstencil);
  }

  __forceinline void call (ID3D11DepthStencilView *&depthstencil, SK_TLS* pTLS = nullptr)
  { if (data != nullptr && fn != nullptr && (pTLS == nullptr || (! pTLS->imgui.drawing))) fn (data, depthstencil); }
} extern SK_ReShade_ClearDepthStencilViewCallback;

struct SK_RESHADE_CALLBACK_CopyResource
{
  SK_ReShade_OnCopyResourceD3D11_pfn fn   = nullptr;
  void*                              data = nullptr;
  __forceinline void call (ID3D11Resource *&dest, ID3D11Resource *&source, SK_TLS* pTLS = nullptr)
  { if (data != nullptr && fn != nullptr && (pTLS == nullptr || (! pTLS->imgui.drawing))) fn (data, dest, source); }
} extern SK_ReShade_CopyResourceCallback;


uint32_t
__cdecl
SK_D3D11_ChecksumShaderBytecode ( _In_      const void                *pShaderBytecode,
                                  _In_            SIZE_T               BytecodeLength  );

std::wstring
SK_D3D11_DescribeResource (ID3D11Resource* pRes);


HRESULT
STDMETHODCALLTYPE
SK_D3D11Dev_CreateRenderTargetView_Impl (
  _In_            ID3D11Device                   *pDev,
  _In_            ID3D11Resource                 *pResource,
  _In_opt_  const D3D11_RENDER_TARGET_VIEW_DESC  *pDesc,
  _Out_opt_       ID3D11RenderTargetView        **ppRTView,
                  BOOL                            bWrapped );

HRESULT
WINAPI
D3D11Dev_CreateTexture2D_Impl (
  _In_              ID3D11Device            *This,
  _Inout_opt_       D3D11_TEXTURE2D_DESC    *pDesc,
  _In_opt_    const D3D11_SUBRESOURCE_DATA  *pInitialData,
  _Out_opt_         ID3D11Texture2D        **ppTexture2D,
                    LPVOID                   lpCallerAddr,
                    SK_TLS                  *pTLS = SK_TLS_Bottom () );

#include <SpecialK/render/d3d11/d3d11_state_tracker.h>

void
SK_D3D11_SetShader_Impl ( ID3D11DeviceContext*        pDevCtx,
                          IUnknown*                   pShader,
                          sk_shader_class             type,
                          ID3D11ClassInstance *const *ppClassInstances,
                          UINT                        NumClassInstances,
                          bool                        Wrapped = false );

void
SK_D3D11_SetShaderResources_Impl (
   SK_D3D11_ShaderType  ShaderType,
   BOOL                 Deferred,
   ID3D11DeviceContext *This,       // non-null indicates hooked function
   ID3D11DeviceContext *Wrapper,    // non-null indicates a wrapper
   UINT                 StartSlot,
   UINT                 NumViews,
   _In_opt_             ID3D11ShaderResourceView* const *ppShaderResourceViews );

void
STDMETHODCALLTYPE
SK_D3D11_UpdateSubresource_Impl (
  _In_           ID3D11DeviceContext *pDevCtx,
  _In_           ID3D11Resource      *pDstResource,
  _In_           UINT                 DstSubresource,
  _In_opt_ const D3D11_BOX           *pDstBox,
  _In_     const void                *pSrcData,
  _In_           UINT                 SrcRowPitch,
  _In_           UINT                 SrcDepthPitch,
                 BOOL                 bWrapped );

HRESULT
STDMETHODCALLTYPE
SK_D3D11_Map_Impl (
  _In_      ID3D11DeviceContext      *pDevCtx,
  _In_      ID3D11Resource           *pResource,
  _In_      UINT                      Subresource,
  _In_      D3D11_MAP                 MapType,
  _In_      UINT                      MapFlags,
  _Out_opt_ D3D11_MAPPED_SUBRESOURCE *pMappedResource,
            BOOL                      bWrapped );

void
STDMETHODCALLTYPE
SK_D3D11_Unmap_Impl (
  _In_ ID3D11DeviceContext *pDevCtx,
  _In_ ID3D11Resource      *pResource,
  _In_ UINT                 Subresource,
       BOOL                 bWrapped );

void
STDMETHODCALLTYPE
SK_D3D11_CopyResource_Impl (
       ID3D11DeviceContext *pDevCtx,
  _In_ ID3D11Resource      *pDstResource,
  _In_ ID3D11Resource      *pSrcResource,
       BOOL                 bWrapped );

void
STDMETHODCALLTYPE
SK_D3D11_DrawAuto_Impl (_In_ ID3D11DeviceContext *pDevCtx, BOOL bWrapped);

void
SK_D3D11_Draw_Impl (ID3D11DeviceContext* pDevCtx,
                    UINT                 VertexCount,
                    UINT                 StartVertexLocation,
                    bool                 Wrapped = false );

void
STDMETHODCALLTYPE
SK_D3D11_DrawIndexed_Impl (
  _In_ ID3D11DeviceContext *pDevCtx,
  _In_ UINT                 IndexCount,
  _In_ UINT                 StartIndexLocation,
  _In_ INT                  BaseVertexLocation,
       BOOL                 bWrapped );

void
STDMETHODCALLTYPE
SK_D3D11_DrawIndexedInstanced_Impl (
  _In_ ID3D11DeviceContext *pDevCtx,
  _In_ UINT                 IndexCountPerInstance,
  _In_ UINT                 InstanceCount,
  _In_ UINT                 StartIndexLocation,
  _In_ INT                  BaseVertexLocation,
  _In_ UINT                 StartInstanceLocation,
       BOOL                 bWrapped );

void
STDMETHODCALLTYPE
SK_D3D11_DrawIndexedInstancedIndirect_Impl (
  _In_ ID3D11DeviceContext *pDevCtx,
  _In_ ID3D11Buffer        *pBufferForArgs,
  _In_ UINT                 AlignedByteOffsetForArgs,
       BOOL                 bWrapped );

void
STDMETHODCALLTYPE
SK_D3D11_DrawInstanced_Impl (
  _In_ ID3D11DeviceContext *pDevCtx,
  _In_ UINT                 VertexCountPerInstance,
  _In_ UINT                 InstanceCount,
  _In_ UINT                 StartVertexLocation,
  _In_ UINT                 StartInstanceLocation,
       BOOL                 bWrapped );

void
STDMETHODCALLTYPE
SK_D3D11_DrawInstancedIndirect_Impl (
  _In_ ID3D11DeviceContext *pDevCtx,
  _In_ ID3D11Buffer        *pBufferForArgs,
  _In_ UINT                 AlignedByteOffsetForArgs,
       BOOL                 bWrapped );

void
STDMETHODCALLTYPE
SK_D3D11_OMSetRenderTargets_Impl (
         ID3D11DeviceContext           *pDevCtx,
_In_     UINT                           NumViews,
_In_opt_ ID3D11RenderTargetView *const *ppRenderTargetViews,
_In_opt_ ID3D11DepthStencilView        *pDepthStencilView,
         BOOL                           bWrapped );



bool
SK_D3D11_DispatchHandler ( ID3D11DeviceContext* pDevCtx,
                           SK_TLS*              pTLS = SK_TLS_Bottom () );

void
SK_D3D11_PostDispatch ( ID3D11DeviceContext* pDevCtx,
                        SK_TLS*              pTLS = SK_TLS_Bottom () );




#if 1
HRESULT
WINAPI
D3D11Dev_CreateRasterizerState_Override (
  ID3D11Device            *This,
  const D3D11_RASTERIZER_DESC   *pRasterizerDesc,
  ID3D11RasterizerState  **ppRasterizerState );

HRESULT
WINAPI
D3D11Dev_CreateSamplerState_Override
(
  _In_            ID3D11Device        *This,
  _In_      const D3D11_SAMPLER_DESC  *pSamplerDesc,
  _Out_opt_       ID3D11SamplerState **ppSamplerState );

__declspec (noinline)
HRESULT
WINAPI
D3D11Dev_CreateBuffer_Override (
  _In_           ID3D11Device            *This,
  _In_     const D3D11_BUFFER_DESC       *pDesc,
  _In_opt_ const D3D11_SUBRESOURCE_DATA  *pInitialData,
  _Out_opt_      ID3D11Buffer           **ppBuffer );

__declspec (noinline)
HRESULT
WINAPI
D3D11Dev_CreateTexture2D_Override (
  _In_            ID3D11Device           *This,
  _In_      const D3D11_TEXTURE2D_DESC   *pDesc,
  _In_opt_  const D3D11_SUBRESOURCE_DATA *pInitialData,
  _Out_opt_       ID3D11Texture2D        **ppTexture2D );

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11Dev_CreateRenderTargetView_Override (
  _In_            ID3D11Device                   *This,
  _In_            ID3D11Resource                 *pResource,
  _In_opt_  const D3D11_RENDER_TARGET_VIEW_DESC  *pDesc,
  _Out_opt_       ID3D11RenderTargetView        **ppRTView );

__declspec (noinline)
HRESULT
WINAPI
D3D11Dev_CreateShaderResourceView_Override (
  _In_           ID3D11Device                     *This,
  _In_           ID3D11Resource                   *pResource,
  _In_opt_ const D3D11_SHADER_RESOURCE_VIEW_DESC  *pDesc,
  _Out_opt_      ID3D11ShaderResourceView        **ppSRView );

__declspec (noinline)
HRESULT
WINAPI
D3D11Dev_CreateDepthStencilView_Override (
  _In_            ID3D11Device                  *This,
  _In_            ID3D11Resource                *pResource,
  _In_opt_  const D3D11_DEPTH_STENCIL_VIEW_DESC *pDesc,
  _Out_opt_       ID3D11DepthStencilView        **ppDepthStencilView );

__declspec (noinline)
HRESULT
WINAPI
D3D11Dev_CreateUnorderedAccessView_Override (
  _In_            ID3D11Device                     *This,
  _In_            ID3D11Resource                   *pResource,
  _In_opt_  const D3D11_UNORDERED_ACCESS_VIEW_DESC *pDesc,
  _Out_opt_       ID3D11UnorderedAccessView       **ppUAView );

__declspec (noinline)
HRESULT
WINAPI
D3D11Dev_CreateVertexShader_Override (
  _In_            ID3D11Device        *This,
  _In_      const void                *pShaderBytecode,
  _In_            SIZE_T               BytecodeLength,
  _In_opt_        ID3D11ClassLinkage  *pClassLinkage,
  _Out_opt_       ID3D11VertexShader **ppVertexShader );

__declspec (noinline)
HRESULT
WINAPI
D3D11Dev_CreatePixelShader_Override (
  _In_            ID3D11Device        *This,
  _In_      const void                *pShaderBytecode,
  _In_            SIZE_T               BytecodeLength,
  _In_opt_        ID3D11ClassLinkage  *pClassLinkage,
  _Out_opt_       ID3D11PixelShader  **ppPixelShader );

__declspec (noinline)
HRESULT
WINAPI
D3D11Dev_CreateGeometryShader_Override (
  _In_            ID3D11Device          *This,
  _In_      const void                  *pShaderBytecode,
  _In_            SIZE_T                 BytecodeLength,
  _In_opt_        ID3D11ClassLinkage    *pClassLinkage,
  _Out_opt_       ID3D11GeometryShader **ppGeometryShader );

__declspec (noinline)
HRESULT
WINAPI
D3D11Dev_CreateGeometryShaderWithStreamOutput_Override (
  _In_            ID3D11Device               *This,
  _In_      const void                       *pShaderBytecode,
  _In_            SIZE_T                     BytecodeLength,
  _In_opt_  const D3D11_SO_DECLARATION_ENTRY *pSODeclaration,
  _In_            UINT                       NumEntries,
  _In_opt_  const UINT                       *pBufferStrides,
  _In_            UINT                       NumStrides,
  _In_            UINT                       RasterizedStream,
  _In_opt_        ID3D11ClassLinkage         *pClassLinkage,
  _Out_opt_       ID3D11GeometryShader      **ppGeometryShader );

__declspec (noinline)
HRESULT
WINAPI
D3D11Dev_CreateHullShader_Override (
  _In_            ID3D11Device        *This,
  _In_      const void                *pShaderBytecode,
  _In_            SIZE_T               BytecodeLength,
  _In_opt_        ID3D11ClassLinkage  *pClassLinkage,
  _Out_opt_       ID3D11HullShader   **ppHullShader );

__declspec (noinline)
HRESULT
WINAPI
D3D11Dev_CreateDomainShader_Override (
  _In_            ID3D11Device        *This,
  _In_      const void                *pShaderBytecode,
  _In_            SIZE_T               BytecodeLength,
  _In_opt_        ID3D11ClassLinkage  *pClassLinkage,
  _Out_opt_       ID3D11DomainShader **ppDomainShader );

__declspec (noinline)
HRESULT
WINAPI
D3D11Dev_CreateComputeShader_Override (
  _In_            ID3D11Device         *This,
  _In_      const void                 *pShaderBytecode,
  _In_            SIZE_T                BytecodeLength,
  _In_opt_        ID3D11ClassLinkage   *pClassLinkage,
  _Out_opt_       ID3D11ComputeShader **ppComputeShader );

interface ID3D11Device2;
interface ID3D11Device3;

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11Dev_CreateDeferredContext_Override (
  _In_            ID3D11Device         *This,
  _In_            UINT                  ContextFlags,
  _Out_opt_       ID3D11DeviceContext **ppDeferredContext);

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11Dev_CreateDeferredContext1_Override (
  _In_            ID3D11Device1         *This,
  _In_            UINT                   ContextFlags,
  _Out_opt_       ID3D11DeviceContext1 **ppDeferredContext1);

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11Dev_CreateDeferredContext2_Override (
  _In_            ID3D11Device2         *This,
  _In_            UINT                   ContextFlags,
  _Out_opt_       ID3D11DeviceContext2 **ppDeferredContext2);

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11Dev_CreateDeferredContext3_Override (
  _In_            ID3D11Device3         *This,
  _In_            UINT                   ContextFlags,
  _Out_opt_       ID3D11DeviceContext3 **ppDeferredContext3);

_declspec (noinline)
void
STDMETHODCALLTYPE
D3D11Dev_GetImmediateContext_Override (
  _In_            ID3D11Device         *This,
  _Out_           ID3D11DeviceContext **ppImmediateContext);

_declspec (noinline)
void
STDMETHODCALLTYPE
D3D11Dev_GetImmediateContext1_Override (
  _In_            ID3D11Device1        *This,
  _Out_           ID3D11DeviceContext1 **ppImmediateContext1);

_declspec (noinline)
void
STDMETHODCALLTYPE
D3D11Dev_GetImmediateContext2_Override (
  _In_            ID3D11Device2         *This,
  _Out_           ID3D11DeviceContext2 **ppImmediateContext2);

_declspec (noinline)
void
STDMETHODCALLTYPE
D3D11Dev_GetImmediateContext3_Override (
  _In_            ID3D11Device3         *This,
  _Out_           ID3D11DeviceContext3 **ppImmediateContext3);

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_RSSetScissorRects_Override (
        ID3D11DeviceContext *This,
        UINT                 NumRects,
  const D3D11_RECT          *pRects);

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_RSSetViewports_Override (
        ID3D11DeviceContext* This,
        UINT                 NumViewports,
  const D3D11_VIEWPORT*      pViewports );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_VSSetConstantBuffers_Override (
  ID3D11DeviceContext*  This,
  UINT                  StartSlot,
  UINT                  NumBuffers,
  ID3D11Buffer *const  *ppConstantBuffers );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_VSSetShaderResources_Override (
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumViews,
  _In_opt_       ID3D11ShaderResourceView* const *ppShaderResourceViews );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_PSSetShaderResources_Override (
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumViews,
  _In_opt_       ID3D11ShaderResourceView* const *ppShaderResourceViews );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_PSSetConstantBuffers_Override (
  ID3D11DeviceContext*  This,
  UINT                  StartSlot,
  UINT                  NumBuffers,
  ID3D11Buffer *const  *ppConstantBuffers );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_GSSetShaderResources_Override (
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumViews,
  _In_opt_       ID3D11ShaderResourceView* const *ppShaderResourceViews );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_HSSetShaderResources_Override (
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumViews,
  _In_opt_       ID3D11ShaderResourceView* const *ppShaderResourceViews );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DSSetShaderResources_Override (
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumViews,
  _In_opt_       ID3D11ShaderResourceView* const *ppShaderResourceViews );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_CSSetShaderResources_Override (
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumViews,
  _In_opt_       ID3D11ShaderResourceView* const *ppShaderResourceViews );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_CSSetUnorderedAccessViews_Override (
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumUAVs,
  _In_opt_       ID3D11UnorderedAccessView *const *ppUnorderedAccessViews,
  _In_opt_ const UINT                             *pUAVInitialCounts );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_UpdateSubresource_Override (
  _In_           ID3D11DeviceContext* This,
  _In_           ID3D11Resource      *pDstResource,
  _In_           UINT                 DstSubresource,
  _In_opt_ const D3D11_BOX           *pDstBox,
  _In_     const void                *pSrcData,
  _In_           UINT                 SrcRowPitch,
  _In_           UINT                 SrcDepthPitch);

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DrawIndexed_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 IndexCount,
  _In_ UINT                 StartIndexLocation,
  _In_ INT                  BaseVertexLocation );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_Draw_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 VertexCount,
  _In_ UINT                 StartVertexLocation );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DrawAuto_Override (_In_ ID3D11DeviceContext *This);

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DrawIndexedInstanced_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 IndexCountPerInstance,
  _In_ UINT                 InstanceCount,
  _In_ UINT                 StartIndexLocation,
  _In_ INT                  BaseVertexLocation,
  _In_ UINT                 StartInstanceLocation );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DrawIndexedInstancedIndirect_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ ID3D11Buffer        *pBufferForArgs,
  _In_ UINT                 AlignedByteOffsetForArgs );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DrawInstanced_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 VertexCountPerInstance,
  _In_ UINT                 InstanceCount,
  _In_ UINT                 StartVertexLocation,
  _In_ UINT                 StartInstanceLocation );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DrawInstancedIndirect_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ ID3D11Buffer        *pBufferForArgs,
  _In_ UINT                 AlignedByteOffsetForArgs );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_Dispatch_Override ( _In_ ID3D11DeviceContext *This,
                          _In_ UINT                 ThreadGroupCountX,
                          _In_ UINT                 ThreadGroupCountY,
                          _In_ UINT                 ThreadGroupCountZ );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DispatchIndirect_Override ( _In_ ID3D11DeviceContext *This,
                                  _In_ ID3D11Buffer        *pBufferForArgs,
                                  _In_ UINT                 AlignedByteOffsetForArgs );

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11_Map_Override (
     _In_ ID3D11DeviceContext      *This,
     _In_ ID3D11Resource           *pResource,
     _In_ UINT                      Subresource,
     _In_ D3D11_MAP                 MapType,
     _In_ UINT                      MapFlags,
_Out_opt_ D3D11_MAPPED_SUBRESOURCE *pMappedResource );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_Unmap_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ ID3D11Resource      *pResource,
  _In_ UINT                 Subresource );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_OMSetRenderTargets_Override (
  ID3D11DeviceContext           *This,
  _In_     UINT                           NumViews,
  _In_opt_ ID3D11RenderTargetView *const *ppRenderTargetViews,
  _In_opt_ ID3D11DepthStencilView        *pDepthStencilView );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_OMSetRenderTargetsAndUnorderedAccessViews_Override ( ID3D11DeviceContext              *This,
                                            _In_           UINT                              NumRTVs,
                                            _In_opt_       ID3D11RenderTargetView    *const *ppRenderTargetViews,
                                            _In_opt_       ID3D11DepthStencilView           *pDepthStencilView,
                                            _In_           UINT                              UAVStartSlot,
                                            _In_           UINT                              NumUAVs,
                                            _In_opt_       ID3D11UnorderedAccessView *const *ppUnorderedAccessViews,
                                            _In_opt_ const UINT                             *pUAVInitialCounts );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_OMGetRenderTargets_Override (      ID3D11DeviceContext     *This,
                                   _In_  UINT                     NumViews,
                                   _Out_ ID3D11RenderTargetView **ppRenderTargetViews,
                                   _Out_ ID3D11DepthStencilView **ppDepthStencilView );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_OMGetRenderTargetsAndUnorderedAccessViews_Override (ID3D11DeviceContext              *This,
                                                          _In_  UINT                        NumRTVs,
                                                          _Out_ ID3D11RenderTargetView    **ppRenderTargetViews,
                                                          _Out_ ID3D11DepthStencilView    **ppDepthStencilView,
                                                          _In_  UINT                        UAVStartSlot,
                                                          _In_  UINT                        NumUAVs,
                                                          _Out_ ID3D11UnorderedAccessView **ppUnorderedAccessViews);

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_ClearDepthStencilView_Override (ID3D11DeviceContext         *This,
                                      _In_ ID3D11DepthStencilView *pDepthStencilView,
                                      _In_ UINT                    ClearFlags,
                                      _In_ FLOAT                   Depth,
                                      _In_ UINT8                   Stencil);

void
WINAPI
D3D11_PSSetSamplers_Override
(
  _In_     ID3D11DeviceContext       *This,
  _In_     UINT                       StartSlot,
  _In_     UINT                       NumSamplers,
  _In_opt_ ID3D11SamplerState *const *ppSamplers );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_VSSetShader_Override (
  _In_     ID3D11DeviceContext        *This,
  _In_opt_ ID3D11VertexShader         *pVertexShader,
  _In_opt_ ID3D11ClassInstance *const *ppClassInstances,
           UINT                        NumClassInstances );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_PSSetShader_Override (
  _In_     ID3D11DeviceContext        *This,
  _In_opt_ ID3D11PixelShader          *pPixelShader,
  _In_opt_ ID3D11ClassInstance *const *ppClassInstances,
           UINT                        NumClassInstances );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_GSSetShader_Override (
  _In_     ID3D11DeviceContext        *This,
  _In_opt_ ID3D11GeometryShader       *pGeometryShader,
  _In_opt_ ID3D11ClassInstance *const *ppClassInstances,
           UINT                        NumClassInstances );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_HSSetShader_Override (
  _In_     ID3D11DeviceContext        *This,
  _In_opt_ ID3D11HullShader           *pHullShader,
  _In_opt_ ID3D11ClassInstance *const *ppClassInstances,
           UINT                        NumClassInstances );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DSSetShader_Override (
  _In_     ID3D11DeviceContext        *This,
  _In_opt_ ID3D11DomainShader         *pDomainShader,
  _In_opt_ ID3D11ClassInstance *const *ppClassInstances,
           UINT                        NumClassInstances );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_CSSetShader_Override (
  _In_     ID3D11DeviceContext        *This,
  _In_opt_ ID3D11ComputeShader        *pComputeShader,
  _In_opt_ ID3D11ClassInstance *const *ppClassInstances,
           UINT                        NumClassInstances );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_VSGetShader_Override (
  _In_        ID3D11DeviceContext  *This,
  _Out_       ID3D11VertexShader  **ppVertexShader,
  _Out_opt_   ID3D11ClassInstance **ppClassInstances,
  _Inout_opt_ UINT                 *pNumClassInstances );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_PSGetShader_Override (
  _In_        ID3D11DeviceContext  *This,
  _Out_       ID3D11PixelShader   **ppPixelShader,
  _Out_opt_   ID3D11ClassInstance **ppClassInstances,
  _Inout_opt_ UINT                 *pNumClassInstances );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_GSGetShader_Override (
  _In_        ID3D11DeviceContext   *This,
  _Out_       ID3D11GeometryShader **ppGeometryShader,
  _Out_opt_   ID3D11ClassInstance  **ppClassInstances,
  _Inout_opt_ UINT                  *pNumClassInstances );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_HSGetShader_Override (
  _In_        ID3D11DeviceContext  *This,
  _Out_       ID3D11HullShader    **ppHullShader,
  _Out_opt_   ID3D11ClassInstance **ppClassInstances,
  _Inout_opt_ UINT                 *pNumClassInstances );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DSGetShader_Override (
  _In_        ID3D11DeviceContext  *This,
  _Out_       ID3D11DomainShader  **ppDomainShader,
  _Out_opt_   ID3D11ClassInstance **ppClassInstances,
  _Inout_opt_ UINT                 *pNumClassInstances );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_CSGetShader_Override (
  _In_        ID3D11DeviceContext  *This,
  _Out_       ID3D11ComputeShader **ppComputeShader,
  _Out_opt_   ID3D11ClassInstance **ppClassInstances,
  _Inout_opt_ UINT                 *pNumClassInstances );

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11_GetData_Override (
  _In_  ID3D11DeviceContext *This,
  _In_  ID3D11Asynchronous  *pAsync,
  _Out_writes_bytes_opt_   ( DataSize )
  void                *pData,
  _In_  UINT                 DataSize,
  _In_  UINT                 GetDataFlags );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_CopyResource_Override (
  ID3D11DeviceContext *This,
  _In_ ID3D11Resource      *pDstResource,
  _In_ ID3D11Resource      *pSrcResource );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_CopySubresourceRegion_Override (
  _In_           ID3D11DeviceContext *This,
  _In_           ID3D11Resource      *pDstResource,
  _In_           UINT                 DstSubresource,
  _In_           UINT                 DstX,
  _In_           UINT                 DstY,
  _In_           UINT                 DstZ,
  _In_           ID3D11Resource      *pSrcResource,
  _In_           UINT                 SrcSubresource,
  _In_opt_ const D3D11_BOX           *pSrcBox );

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_UpdateSubresource1_Override (
  _In_           ID3D11DeviceContext1 *This,
  _In_           ID3D11Resource       *pDstResource,
  _In_           UINT                  DstSubresource,
  _In_opt_ const D3D11_BOX            *pDstBox,
  _In_     const void                 *pSrcData,
  _In_           UINT                  SrcRowPitch,
  _In_           UINT                  SrcDepthPitch,
  _In_           UINT                  CopyFlags);
#endif




bool
SK_D3D11_OverrideDepthStencil (DXGI_FORMAT& fmt);

class SK_D3D11_Wrapper_Factory
{
public:
  ID3D11DeviceContext4* wrapDeviceContext (ID3D11DeviceContext* dev_ctx);
} extern SK_D3D11_WrapperFactory;