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

#include <SpecialK/render/d3d12/d3d12_interfaces.h>

using
D3D12Device_CreateGraphicsPipelineState_pfn = HRESULT
(STDMETHODCALLTYPE *)(ID3D12Device*,
                const D3D12_GRAPHICS_PIPELINE_STATE_DESC*,
                      REFIID,void**);

using
D3D12Device2_CreatePipelineState_pfn = HRESULT
(STDMETHODCALLTYPE *)(ID3D12Device2*,
                 const D3D12_PIPELINE_STATE_STREAM_DESC*,
                       REFIID,void**);

using
D3D12Device_CreateShaderResourceView_pfn = void
(STDMETHODCALLTYPE *)(ID3D12Device*,ID3D12Resource*,
                const D3D12_SHADER_RESOURCE_VIEW_DESC*,
                      D3D12_CPU_DESCRIPTOR_HANDLE);

using
D3D12Device_CreateUnorderedAccessView_pfn = void
(STDMETHODCALLTYPE *)(ID3D12Device*,ID3D12Resource*,
                      ID3D12Resource*,
                const D3D12_UNORDERED_ACCESS_VIEW_DESC*,
                      D3D12_CPU_DESCRIPTOR_HANDLE);

using
D3D12Device_CreateRenderTargetView_pfn = void
(STDMETHODCALLTYPE *)(ID3D12Device*,ID3D12Resource*,
                const D3D12_RENDER_TARGET_VIEW_DESC*,
                      D3D12_CPU_DESCRIPTOR_HANDLE);

using
D3D12Device_CreateSampler_pfn = void
(STDMETHODCALLTYPE *)(ID3D12Device*,const D3D12_SAMPLER_DESC*,
                      D3D12_CPU_DESCRIPTOR_HANDLE);

using
D3D12Device11_CreateSampler2_pfn = void
(STDMETHODCALLTYPE *)(ID3D12Device11*,const D3D12_SAMPLER_DESC2*,
                      D3D12_CPU_DESCRIPTOR_HANDLE);

using
D3D12Device_GetResourceAllocationInfo_pfn = D3D12_RESOURCE_ALLOCATION_INFO
(STDMETHODCALLTYPE *)(ID3D12Device*,UINT,UINT,const D3D12_RESOURCE_DESC*);

using
D3D12Device_CreateCommittedResource_pfn = HRESULT
(STDMETHODCALLTYPE *)(ID3D12Device*,const D3D12_HEAP_PROPERTIES*,D3D12_HEAP_FLAGS,
                                    const D3D12_RESOURCE_DESC*,D3D12_RESOURCE_STATES,
                                    const D3D12_CLEAR_VALUE*,REFIID,void**);

using
D3D12Device4_CreateCommittedResource1_pfn = HRESULT
(STDMETHODCALLTYPE *)(ID3D12Device4*,const D3D12_HEAP_PROPERTIES*,D3D12_HEAP_FLAGS,
                                     const D3D12_RESOURCE_DESC1*,D3D12_RESOURCE_STATES,
                                     const D3D12_CLEAR_VALUE*,ID3D12ProtectedResourceSession*,
                                     REFIID,void**);

using
D3D12Device8_CreateCommittedResource2_pfn = HRESULT
(STDMETHODCALLTYPE *)(ID3D12Device8*,const D3D12_HEAP_PROPERTIES*,D3D12_HEAP_FLAGS,
                                     const D3D12_RESOURCE_DESC1*,D3D12_RESOURCE_STATES,
                                     const D3D12_CLEAR_VALUE*,ID3D12ProtectedResourceSession*,
                                     REFIID,void**);

using
D3D12Device9_CreateShaderCacheSession_pfn = HRESULT
(STDMETHODCALLTYPE *)(ID3D12Device9*,const D3D12_SHADER_CACHE_SESSION_DESC*,
                                           REFIID,void**);

using
D3D12Device9_ShaderCacheControl_pfn = HRESULT
(STDMETHODCALLTYPE *)(ID3D12Device9*,D3D12_SHADER_CACHE_KIND_FLAGS,
                                     D3D12_SHADER_CACHE_CONTROL_FLAGS);

using
D3D12Device_CreateHeap_pfn = HRESULT
(STDMETHODCALLTYPE *)(ID3D12Device*,const D3D12_HEAP_DESC*,REFIID,_COM_Outptr_opt_ void**);

using
D3D12Device_CreatePlacedResource_pfn = HRESULT
(STDMETHODCALLTYPE *)(ID3D12Device*,ID3D12Heap*,
                       UINT64,const D3D12_RESOURCE_DESC*,
                                    D3D12_RESOURCE_STATES,
                              const D3D12_CLEAR_VALUE*,REFIID,void**);

using
D3D12Device_CreateCommandAllocator_pfn = HRESULT
(STDMETHODCALLTYPE *)(ID3D12Device*,D3D12_COMMAND_LIST_TYPE,
                      REFIID,void**);

using
D3D12Device_CheckFeatureSupport_pfn = HRESULT
(STDMETHODCALLTYPE *)(ID3D12Device*,D3D12_FEATURE,
                      void*,UINT);

extern D3D12Device_CreateGraphicsPipelineState_pfn
       D3D12Device_CreateGraphicsPipelineState_Original;
extern D3D12Device2_CreatePipelineState_pfn
       D3D12Device2_CreatePipelineState_Original;
extern D3D12Device_CreateShaderResourceView_pfn
       D3D12Device_CreateShaderResourceView_Original;
extern D3D12Device_CreateRenderTargetView_pfn
       D3D12Device_CreateRenderTargetView_Original;
extern D3D12Device_GetResourceAllocationInfo_pfn
       D3D12Device_GetResourceAllocationInfo_Original;
extern D3D12Device_CreateCommittedResource_pfn
       D3D12Device_CreateCommittedResource_Original;
extern D3D12Device_CreatePlacedResource_pfn
       D3D12Device_CreatePlacedResource_Original;
extern D3D12Device_CreateCommandAllocator_pfn
       D3D12Device_CreateCommandAllocator_Original;
extern D3D12Device_CheckFeatureSupport_pfn
       D3D12Device_CheckFeatureSupport_Original;

extern D3D12Device4_CreateCommittedResource1_pfn
       D3D12Device4_CreateCommittedResource1_Original;

extern D3D12Device8_CreateCommittedResource2_pfn
       D3D12Device8_CreateCommittedResource2_Original;

extern D3D12Device9_CreateShaderCacheSession_pfn
       D3D12Device9_CreateShaderCacheSession_Original;
extern D3D12Device9_ShaderCacheControl_pfn
       D3D12Device9_ShaderCacheControl_Original;

bool SK_D3D12_HookDeviceCreation (void);
bool SK_D3D12_InstallDeviceHooks (ID3D12Device* pDev12);

static inline constexpr GUID SKID_D3D12IgnoredTextureCopy = { 0x3d5298cb, 0xd8f0,  0x7233, { 0xa1, 0x9d, 0xb1, 0xd5, 0x97, 0x92, 0x00, 0x70 } };

void SK_D3D12_CommitUploadQueue (ID3D12GraphicsCommandList *pCmdList);
void _InitCopyTextureRegionHook (ID3D12GraphicsCommandList *pCmdList);

extern volatile LONG  __d3d12_hooked;