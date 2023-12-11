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

#include <SpecialK/render/dxgi/dxgi_backend.h>
#include <SpecialK/plugin/reshade.h>

#include <Unknwnbase.h>

#include <d3d12.h>
#include <dxgidebug.h>
#include <D3D11SDKLayers.h>

#include <atomic>

//
// No references are held.
// 
//   This is just used to combine a SwapChain with a Device / Command Queue
//     if we happen to inject SK late and did not witness the D3D12 SwapChain's
//       creation...
// 
//   It could take multiple frames before a working combo of all
//         of these things emerge and we can start drawing.
//   
extern IDXGISwapChain3* pLazyD3D12Chain;
extern ID3D12Device*    pLazyD3D12Device;

typedef
  HRESULT (*D3D12SerializeRootSignature_pfn)(
             const D3D12_ROOT_SIGNATURE_DESC   *pRootSignature,
                   D3D_ROOT_SIGNATURE_VERSION   Version,
                   ID3DBlob                   **ppBlob,
                   ID3DBlob                   **ppErrorBlob);

#if 0
ID3D12Device vftable
--------------------
7  UINT                           STDMETHODCALLTYPE GetNodeCount
8  HRESULT                        STDMETHODCALLTYPE CreateCommandQueue
9  HRESULT                        STDMETHODCALLTYPE CreateCommandAllocator
10 HRESULT                        STDMETHODCALLTYPE CreateGraphicsPipelineState
11 HRESULT                        STDMETHODCALLTYPE CreateComputePipelineState
12 HRESULT                        STDMETHODCALLTYPE CreateCommandList
13 HRESULT                        STDMETHODCALLTYPE CheckFeatureSupport
14 HRESULT                        STDMETHODCALLTYPE CreateDescriptorHeap
15 UINT                           STDMETHODCALLTYPE GetDescriptorHandleIncrementSize
16 HRESULT                        STDMETHODCALLTYPE CreateRootSignature
17 void                           STDMETHODCALLTYPE CreateConstantBufferView
18 void                           STDMETHODCALLTYPE CreateShaderResourceView
19 void                           STDMETHODCALLTYPE CreateUnorderedAccessView
20 void                           STDMETHODCALLTYPE CreateRenderTargetView
21 void                           STDMETHODCALLTYPE CreateDepthStencilView
22 void                           STDMETHODCALLTYPE CreateSampler
23 void                           STDMETHODCALLTYPE CopyDescriptors
24 void                           STDMETHODCALLTYPE CopyDescriptorsSimple
25 D3D12_RESOURCE_ALLOCATION_INFO STDMETHODCALLTYPE GetResourceAllocationInfo
26 D3D12_HEAP_PROPERTIES          STDMETHODCALLTYPE GetCustomHeapProperties
27 HRESULT                        STDMETHODCALLTYPE CreateCommittedResource
28 HRESULT                        STDMETHODCALLTYPE CreateHeap
29 HRESULT                        STDMETHODCALLTYPE CreatePlacedResource
30 HRESULT                        STDMETHODCALLTYPE CreateReservedResource
31 HRESULT                        STDMETHODCALLTYPE CreateSharedHandle
32 HRESULT                        STDMETHODCALLTYPE OpenSharedHandle
33 HRESULT                        STDMETHODCALLTYPE OpenSharedHandleByName
34 HRESULT                        STDMETHODCALLTYPE MakeResident
35 HRESULT                        STDMETHODCALLTYPE Evict
36 HRESULT                        STDMETHODCALLTYPE CreateFence
37 HRESULT                        STDMETHODCALLTYPE GetDeviceRemovedReason
38 void                           STDMETHODCALLTYPE GetCopyableFootprints
39 HRESULT                        STDMETHODCALLTYPE CreateQueryHeap
40 HRESULT                        STDMETHODCALLTYPE SetStablePowerState
41 HRESULT                        STDMETHODCALLTYPE CreateCommandSignature
42 void                           STDMETHODCALLTYPE GetResourceTiling
43 LUID                           STDMETHODCALLTYPE GetAdapterLuid
#endif

#if 0
    typedef struct ID3D12GraphicsCommandListVtbl
    {
        BEGIN_INTERFACE

0        HRESULT ( STDMETHODCALLTYPE *QueryInterface )(
            ID3D12GraphicsCommandList * This,
            REFIID riid,
            _COM_Outptr_  void **ppvObject);

1        ULONG ( STDMETHODCALLTYPE *AddRef )(
            ID3D12GraphicsCommandList * This);

2        ULONG ( STDMETHODCALLTYPE *Release )(
            ID3D12GraphicsCommandList * This);

3        HRESULT ( STDMETHODCALLTYPE *GetPrivateData )(
            ID3D12GraphicsCommandList * This,
            _In_  REFGUID guid,
            _Inout_  UINT *pDataSize,
            _Out_writes_bytes_opt_( *pDataSize )  void *pData);

4        HRESULT ( STDMETHODCALLTYPE *SetPrivateData )(
            ID3D12GraphicsCommandList * This,
            _In_  REFGUID guid,
            _In_  UINT DataSize,
            _In_reads_bytes_opt_( DataSize )  const void *pData);

5        HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )(
            ID3D12GraphicsCommandList * This,
            _In_  REFGUID guid,
            _In_opt_  const IUnknown *pData);

6        HRESULT ( STDMETHODCALLTYPE *SetName )(
            ID3D12GraphicsCommandList * This,
            _In_z_  LPCWSTR Name);

7        HRESULT ( STDMETHODCALLTYPE *GetDevice )(
            ID3D12GraphicsCommandList * This,
            REFIID riid,
            _COM_Outptr_opt_  void **ppvDevice);

8        D3D12_COMMAND_LIST_TYPE ( STDMETHODCALLTYPE *GetType )(
            ID3D12GraphicsCommandList * This);

9        HRESULT ( STDMETHODCALLTYPE *Close )(
            ID3D12GraphicsCommandList * This);

10        HRESULT ( STDMETHODCALLTYPE *Reset )(
            ID3D12GraphicsCommandList * This,
            _In_  ID3D12CommandAllocator *pAllocator,
            _In_opt_  ID3D12PipelineState *pInitialState);

11        void ( STDMETHODCALLTYPE *ClearState )(
            ID3D12GraphicsCommandList * This,
            _In_opt_  ID3D12PipelineState *pPipelineState);

12        void ( STDMETHODCALLTYPE *DrawInstanced )(
            ID3D12GraphicsCommandList * This,
            _In_  UINT VertexCountPerInstance,
            _In_  UINT InstanceCount,
            _In_  UINT StartVertexLocation,
            _In_  UINT StartInstanceLocation);

13        void ( STDMETHODCALLTYPE *DrawIndexedInstanced )(
            ID3D12GraphicsCommandList * This,
            _In_  UINT IndexCountPerInstance,
            _In_  UINT InstanceCount,
            _In_  UINT StartIndexLocation,
            _In_  INT BaseVertexLocation,
            _In_  UINT StartInstanceLocation);

14        void ( STDMETHODCALLTYPE *Dispatch )(
            ID3D12GraphicsCommandList * This,
            _In_  UINT ThreadGroupCountX,
            _In_  UINT ThreadGroupCountY,
            _In_  UINT ThreadGroupCountZ);

15        void ( STDMETHODCALLTYPE *CopyBufferRegion )(
            ID3D12GraphicsCommandList * This,
            _In_  ID3D12Resource *pDstBuffer,
            UINT64 DstOffset,
            _In_  ID3D12Resource *pSrcBuffer,
            UINT64 SrcOffset,
            UINT64 NumBytes);

16        void ( STDMETHODCALLTYPE *CopyTextureRegion )(
            ID3D12GraphicsCommandList * This,
            _In_  const D3D12_TEXTURE_COPY_LOCATION *pDst,
            UINT DstX,
            UINT DstY,
            UINT DstZ,
            _In_  const D3D12_TEXTURE_COPY_LOCATION *pSrc,
            _In_opt_  const D3D12_BOX *pSrcBox);

17        void ( STDMETHODCALLTYPE *CopyResource )(
            ID3D12GraphicsCommandList * This,
            _In_  ID3D12Resource *pDstResource,
            _In_  ID3D12Resource *pSrcResource);

        void ( STDMETHODCALLTYPE *CopyTiles )(
            ID3D12GraphicsCommandList * This,
            _In_  ID3D12Resource *pTiledResource,
            _In_  const D3D12_TILED_RESOURCE_COORDINATE *pTileRegionStartCoordinate,
            _In_  const D3D12_TILE_REGION_SIZE *pTileRegionSize,
            _In_  ID3D12Resource *pBuffer,
            UINT64 BufferStartOffsetInBytes,
            D3D12_TILE_COPY_FLAGS Flags);

        void ( STDMETHODCALLTYPE *ResolveSubresource )(
            ID3D12GraphicsCommandList * This,
            _In_  ID3D12Resource *pDstResource,
            _In_  UINT DstSubresource,
            _In_  ID3D12Resource *pSrcResource,
            _In_  UINT SrcSubresource,
            _In_  DXGI_FORMAT Format);

        void ( STDMETHODCALLTYPE *IASetPrimitiveTopology )(
            ID3D12GraphicsCommandList * This,
            _In_  D3D12_PRIMITIVE_TOPOLOGY PrimitiveTopology);

        void ( STDMETHODCALLTYPE *RSSetViewports )(
            ID3D12GraphicsCommandList * This,
            _In_range_(0, D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)  UINT NumViewports,
            _In_reads_( NumViewports)  const D3D12_VIEWPORT *pViewports);

        void ( STDMETHODCALLTYPE *RSSetScissorRects )(
            ID3D12GraphicsCommandList * This,
            _In_range_(0, D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)  UINT NumRects,
            _In_reads_( NumRects)  const D3D12_RECT *pRects);

        void ( STDMETHODCALLTYPE *OMSetBlendFactor )(
            ID3D12GraphicsCommandList * This,
            _In_opt_  const FLOAT BlendFactor[ 4 ]);

        void ( STDMETHODCALLTYPE *OMSetStencilRef )(
            ID3D12GraphicsCommandList * This,
            _In_  UINT StencilRef);

        void ( STDMETHODCALLTYPE *SetPipelineState )(
            ID3D12GraphicsCommandList * This,
            _In_  ID3D12PipelineState *pPipelineState);

        void ( STDMETHODCALLTYPE *ResourceBarrier )(
            ID3D12GraphicsCommandList * This,
            _In_  UINT NumBarriers,
            _In_reads_(NumBarriers)  const D3D12_RESOURCE_BARRIER *pBarriers);

        void ( STDMETHODCALLTYPE *ExecuteBundle )(
            ID3D12GraphicsCommandList * This,
            _In_  ID3D12GraphicsCommandList *pCommandList);

        void ( STDMETHODCALLTYPE *SetDescriptorHeaps )(
            ID3D12GraphicsCommandList * This,
            _In_  UINT NumDescriptorHeaps,
            _In_reads_(NumDescriptorHeaps)  ID3D12DescriptorHeap **ppDescriptorHeaps);

        void ( STDMETHODCALLTYPE *SetComputeRootSignature )(
            ID3D12GraphicsCommandList * This,
            _In_  ID3D12RootSignature *pRootSignature);

        void ( STDMETHODCALLTYPE *SetGraphicsRootSignature )(
            ID3D12GraphicsCommandList * This,
            _In_  ID3D12RootSignature *pRootSignature);

        void ( STDMETHODCALLTYPE *SetComputeRootDescriptorTable )(
            ID3D12GraphicsCommandList * This,
            _In_  UINT RootParameterIndex,
            _In_  D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor);

        void ( STDMETHODCALLTYPE *SetGraphicsRootDescriptorTable )(
            ID3D12GraphicsCommandList * This,
            _In_  UINT RootParameterIndex,
            _In_  D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor);

        void ( STDMETHODCALLTYPE *SetComputeRoot32BitConstant )(
            ID3D12GraphicsCommandList * This,
            _In_  UINT RootParameterIndex,
            _In_  UINT SrcData,
            _In_  UINT DestOffsetIn32BitValues);

        void ( STDMETHODCALLTYPE *SetGraphicsRoot32BitConstant )(
            ID3D12GraphicsCommandList * This,
            _In_  UINT RootParameterIndex,
            _In_  UINT SrcData,
            _In_  UINT DestOffsetIn32BitValues);

        void ( STDMETHODCALLTYPE *SetComputeRoot32BitConstants )(
            ID3D12GraphicsCommandList * This,
            _In_  UINT RootParameterIndex,
            _In_  UINT Num32BitValuesToSet,
            _In_reads_(Num32BitValuesToSet*sizeof(UINT))  const void *pSrcData,
            _In_  UINT DestOffsetIn32BitValues);

        void ( STDMETHODCALLTYPE *SetGraphicsRoot32BitConstants )(
            ID3D12GraphicsCommandList * This,
            _In_  UINT RootParameterIndex,
            _In_  UINT Num32BitValuesToSet,
            _In_reads_(Num32BitValuesToSet*sizeof(UINT))  const void *pSrcData,
            _In_  UINT DestOffsetIn32BitValues);

        void ( STDMETHODCALLTYPE *SetComputeRootConstantBufferView )(
            ID3D12GraphicsCommandList * This,
            _In_  UINT RootParameterIndex,
            _In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation);

        void ( STDMETHODCALLTYPE *SetGraphicsRootConstantBufferView )(
            ID3D12GraphicsCommandList * This,
            _In_  UINT RootParameterIndex,
            _In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation);

        void ( STDMETHODCALLTYPE *SetComputeRootShaderResourceView )(
            ID3D12GraphicsCommandList * This,
            _In_  UINT RootParameterIndex,
            _In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation);

        void ( STDMETHODCALLTYPE *SetGraphicsRootShaderResourceView )(
            ID3D12GraphicsCommandList * This,
            _In_  UINT RootParameterIndex,
            _In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation);

        void ( STDMETHODCALLTYPE *SetComputeRootUnorderedAccessView )(
            ID3D12GraphicsCommandList * This,
            _In_  UINT RootParameterIndex,
            _In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation);

        void ( STDMETHODCALLTYPE *SetGraphicsRootUnorderedAccessView )(
            ID3D12GraphicsCommandList * This,
            _In_  UINT RootParameterIndex,
            _In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation);

        void ( STDMETHODCALLTYPE *IASetIndexBuffer )(
            ID3D12GraphicsCommandList * This,
            _In_opt_  const D3D12_INDEX_BUFFER_VIEW *pView);

        void ( STDMETHODCALLTYPE *IASetVertexBuffers )(
            ID3D12GraphicsCommandList * This,
            _In_  UINT StartSlot,
            _In_  UINT NumViews,
            _In_reads_opt_(NumViews)  const D3D12_VERTEX_BUFFER_VIEW *pViews);

        void ( STDMETHODCALLTYPE *SOSetTargets )(
            ID3D12GraphicsCommandList * This,
            _In_  UINT StartSlot,
            _In_  UINT NumViews,
            _In_reads_opt_(NumViews)  const D3D12_STREAM_OUTPUT_BUFFER_VIEW *pViews);

        void ( STDMETHODCALLTYPE *OMSetRenderTargets )(
            ID3D12GraphicsCommandList * This,
            _In_  UINT NumRenderTargetDescriptors,
            _In_  const D3D12_CPU_DESCRIPTOR_HANDLE *pRenderTargetDescriptors,
            _In_  BOOL RTsSingleHandleToDescriptorRange,
            _In_opt_  const D3D12_CPU_DESCRIPTOR_HANDLE *pDepthStencilDescriptor);

        void ( STDMETHODCALLTYPE *ClearDepthStencilView )(
            ID3D12GraphicsCommandList * This,
            _In_  D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView,
            _In_  D3D12_CLEAR_FLAGS ClearFlags,
            _In_  FLOAT Depth,
            _In_  UINT8 Stencil,
            _In_  UINT NumRects,
            _In_reads_(NumRects)  const D3D12_RECT *pRects);

        void ( STDMETHODCALLTYPE *ClearRenderTargetView )(
            ID3D12GraphicsCommandList * This,
            _In_  D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetView,
            _In_  const FLOAT ColorRGBA[ 4 ],
            _In_  UINT NumRects,
            _In_reads_(NumRects)  const D3D12_RECT *pRects);

        void ( STDMETHODCALLTYPE *ClearUnorderedAccessViewUint )(
            ID3D12GraphicsCommandList * This,
            _In_  D3D12_GPU_DESCRIPTOR_HANDLE ViewGPUHandleInCurrentHeap,
            _In_  D3D12_CPU_DESCRIPTOR_HANDLE ViewCPUHandle,
            _In_  ID3D12Resource *pResource,
            _In_  const UINT Values[ 4 ],
            _In_  UINT NumRects,
            _In_reads_(NumRects)  const D3D12_RECT *pRects);

        void ( STDMETHODCALLTYPE *ClearUnorderedAccessViewFloat )(
            ID3D12GraphicsCommandList * This,
            _In_  D3D12_GPU_DESCRIPTOR_HANDLE ViewGPUHandleInCurrentHeap,
            _In_  D3D12_CPU_DESCRIPTOR_HANDLE ViewCPUHandle,
            _In_  ID3D12Resource *pResource,
            _In_  const FLOAT Values[ 4 ],
            _In_  UINT NumRects,
            _In_reads_(NumRects)  const D3D12_RECT *pRects);

        void ( STDMETHODCALLTYPE *DiscardResource )(
            ID3D12GraphicsCommandList * This,
            _In_  ID3D12Resource *pResource,
            _In_opt_  const D3D12_DISCARD_REGION *pRegion);

        void ( STDMETHODCALLTYPE *BeginQuery )(
            ID3D12GraphicsCommandList * This,
            _In_  ID3D12QueryHeap *pQueryHeap,
            _In_  D3D12_QUERY_TYPE Type,
            _In_  UINT Index);

        void ( STDMETHODCALLTYPE *EndQuery )(
            ID3D12GraphicsCommandList * This,
            _In_  ID3D12QueryHeap *pQueryHeap,
            _In_  D3D12_QUERY_TYPE Type,
            _In_  UINT Index);

        void ( STDMETHODCALLTYPE *ResolveQueryData )(
            ID3D12GraphicsCommandList * This,
            _In_  ID3D12QueryHeap *pQueryHeap,
            _In_  D3D12_QUERY_TYPE Type,
            _In_  UINT StartIndex,
            _In_  UINT NumQueries,
            _In_  ID3D12Resource *pDestinationBuffer,
            _In_  UINT64 AlignedDestinationBufferOffset);

        void ( STDMETHODCALLTYPE *SetPredication )(
            ID3D12GraphicsCommandList * This,
            _In_opt_  ID3D12Resource *pBuffer,
            _In_  UINT64 AlignedBufferOffset,
            _In_  D3D12_PREDICATION_OP Operation);

        void ( STDMETHODCALLTYPE *SetMarker )(
            ID3D12GraphicsCommandList * This,
            UINT Metadata,
            _In_reads_bytes_opt_(Size)  const void *pData,
            UINT Size);

        void ( STDMETHODCALLTYPE *BeginEvent )(
            ID3D12GraphicsCommandList * This,
            UINT Metadata,
            _In_reads_bytes_opt_(Size)  const void *pData,
            UINT Size);

        void ( STDMETHODCALLTYPE *EndEvent )(
            ID3D12GraphicsCommandList * This);

        void ( STDMETHODCALLTYPE *ExecuteIndirect )(
            ID3D12GraphicsCommandList * This,
            _In_  ID3D12CommandSignature *pCommandSignature,
            _In_  UINT MaxCommandCount,
            _In_  ID3D12Resource *pArgumentBuffer,
            _In_  UINT64 ArgumentBufferOffset,
            _In_opt_  ID3D12Resource *pCountBuffer,
            _In_  UINT64 CountBufferOffset);

        END_INTERFACE
    } ID3D12GraphicsCommandListVtbl;

    interface ID3D12GraphicsCommandList
    {
        CONST_VTBL struct ID3D12GraphicsCommandListVtbl *lpVtbl;
    };
#endif

extern bool SK_D3D12_Init         (void);
extern void SK_D3D12_Shutdown     (void);
extern void SK_D3D12_EnableHooks  (void);

using D3D12CreateDevice_pfn =
  HRESULT(WINAPI *)(IUnknown*,D3D_FEATURE_LEVEL,REFIID,void**);

extern          IUnknown*      g_pD3D12Dev;
extern D3D12CreateDevice_pfn   D3D12CreateDevice_Import;

struct SK_D3D12_StateTransition : D3D12_RESOURCE_BARRIER
{
  SK_D3D12_StateTransition ( D3D12_RESOURCE_STATES before,
                             D3D12_RESOURCE_STATES after ) noexcept :
           D3D12_RESOURCE_BARRIER ( { D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, D3D12_RESOURCE_BARRIER_FLAG_NONE,
                                    { nullptr, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
                                               before,
                                               after
                                    }
                                    }
           )
  { };
};

struct SK_D3D12_RenderCtx {
  SK_Thread_HybridSpinlock                _ctx_lock;

  SK_ComPtr <ID3D12Device>                _pDevice          = nullptr;
  SK_ComPtr <ID3D12CommandQueue>          _pCommandQueue    = nullptr;
  SK_ComPtr <IDXGISwapChain3>             _pSwapChain       = nullptr;

  reshade::api::effect_runtime*           _pReShadeRuntime  = nullptr;

  SK_ComPtr <ID3D12PipelineState>         pHDRPipeline      = nullptr;
  SK_ComPtr <ID3D12RootSignature>         pHDRSignature     = nullptr;

  struct {
    SK_ComPtr <ID3D12DescriptorHeap>      pBackBuffers      = nullptr;
    SK_ComPtr <ID3D12DescriptorHeap>      pImGui            = nullptr;
    SK_ComPtr <ID3D12DescriptorHeap>      pHDR              = nullptr;
    SK_ComPtr <ID3D12DescriptorHeap>      pHDR_CopyAssist   = nullptr;
  } descriptorHeaps;

	struct FrameCtx {
    SK_D3D12_RenderCtx*                   pRoot             = nullptr;

    struct FenceCtx : SK_ComPtr <ID3D12Fence> {
      HANDLE                              event             =       0;
      volatile UINT64                     value             =       0;

      HRESULT SignalSequential (ID3D12CommandQueue *pCmdQueue);
      HRESULT WaitSequential   (void);
    } fence, reshade_fence;

    SK_ComPtr <ID3D12GraphicsCommandList> pCmdList           = nullptr;
		SK_ComPtr <ID3D12CommandAllocator>    pCmdAllocator      = nullptr;
    bool                                  bCmdListRecording  =   false;

		SK_ComPtr <ID3D12Resource>            pRenderOutput      = nullptr;
		D3D12_CPU_DESCRIPTOR_HANDLE           hRenderOutput      =  { 0 };
    D3D12_CPU_DESCRIPTOR_HANDLE           hRenderOutputsRGB  =  { 0 };
    D3D12_CPU_DESCRIPTOR_HANDLE           hReShadeOutput     =  { 0 };
    D3D12_CPU_DESCRIPTOR_HANDLE           hReShadeOutputsRGB =  { 0 };
    UINT                                  iBufferIdx         =UINT_MAX;

    struct {
      SK_ComPtr <ID3D12Resource>          pSwapChainCopy     = nullptr;
      D3D12_CPU_DESCRIPTOR_HANDLE         hSwapChainCopy_CPU = { 0 };
      D3D12_GPU_DESCRIPTOR_HANDLE         hSwapChainCopy_GPU = { 0 };
      D3D12_CPU_DESCRIPTOR_HANDLE         hBufferCopy_CPU    =  { 0 };
      D3D12_GPU_DESCRIPTOR_HANDLE         hBufferCopy_GPU    =  { 0 };
      D3D12_RECT                          scissor            = {     };
      D3D12_VIEWPORT                      vp                 = {     };

      struct {
        SK_D3D12_StateTransition          process  [2]       = {
          { D3D12_RESOURCE_STATE_COPY_DEST,   D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE },
          { D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET         }
        },                                copy_end [1]      = {
          { D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST   }
        };
      } barriers;
    } hdr;

    bool wait_for_gpu   (void) noexcept;
    bool begin_cmd_list (const SK_ComPtr <ID3D12PipelineState> &state = nullptr);
	  bool exec_cmd_list  (void);
    bool flush_cmd_list (void);

              ~FrameCtx (void);
	};

  std::vector <FrameCtx>                frames_;

  void present (IDXGISwapChain3*    pSwapChain);
  void release (IDXGISwapChain*     pSwapChain);
  bool init    (IDXGISwapChain3*    pSwapChain,
                ID3D12CommandQueue* pCommandQueue);

  static void
    transition_state (
		  const SK_ComPtr <ID3D12GraphicsCommandList>& list,
		  const SK_ComPtr <ID3D12Resource>&            res,
		                         D3D12_RESOURCE_STATES from,
                             D3D12_RESOURCE_STATES to,
		                                          UINT subresource =
                             D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
                      D3D12_RESOURCE_BARRIER_FLAGS flags =
                             D3D12_RESOURCE_BARRIER_FLAG_NONE
    )
	  {
	  	D3D12_RESOURCE_BARRIER
        transition = { D3D12_RESOURCE_BARRIER_TYPE_TRANSITION };
	  	  transition.Transition.pResource   = res.p;
	  	  transition.Transition.Subresource = subresource;
	  	  transition.Transition.StateBefore = from;
	  	  transition.Transition.StateAfter  = to;
        transition.Flags                  = flags;

	  	list->ResourceBarrier
      ( 1,    &transition );
	  }

  // On reset, delay re-initialization
  std::atomic_int frame_delay  = 1;
  volatile ULONG  reset_needed = 0UL;
};

extern SK_LazyGlobal <SK_D3D12_RenderCtx> _d3d12_rbk;

struct SK_ImGui_ResourcesD3D12
{
  SK_ComPtr <ID3D12DescriptorHeap> heap;

  SK_ComPtr <ID3D12PipelineState> pipeline;
  SK_ComPtr <ID3D12RootSignature> signature;

	SK_ComPtr <ID3D12Resource> indices  [DXGI_MAX_SWAP_CHAIN_BUFFERS] = {};
  int                    num_indices  [DXGI_MAX_SWAP_CHAIN_BUFFERS] = {};
  SK_ComPtr <ID3D12Resource> vertices [DXGI_MAX_SWAP_CHAIN_BUFFERS] = {};
	int                    num_vertices [DXGI_MAX_SWAP_CHAIN_BUFFERS] = {};
};

enum class SK_D3D12_ShaderType {
  Vertex   =   1,
  Pixel    =   2,
  Geometry =   4,
  Domain   =   8,
  Hull     =  16,
  Compute  =  32,
  Mesh     =  64,
  Amplify  = 128,

  Invalid  = MAXINT
};

bool SK_D3D12_HotSwapChainHook ( IDXGISwapChain3* pSwapChain,
                                 ID3D12Device*    pDev12 );

void SK_D3D12_BeginFrame (void);
void SK_D3D12_EndFrame   (SK_TLS* pTLS = SK_TLS_Bottom ());



static inline GUID
  SKID_D3D12LastFrameUsed =
    { 0xbaaddaad, 0xf00d,  0xcafe, { 0x13, 0x37, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90 } };