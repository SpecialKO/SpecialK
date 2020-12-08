#include <Unknwnbase.h>

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

typedef HRESULT (WINAPI *D3D12CreateDevice_pfn)(
  _In_opt_  IUnknown          *pAdapter,
            D3D_FEATURE_LEVEL  MinimumFeatureLevel,
  _In_      REFIID             riid,
  _Out_opt_ void             **ppDevice);

extern          IUnknown*      g_pD3D12Dev;
extern D3D12CreateDevice_pfn   D3D12CreateDevice_Import;