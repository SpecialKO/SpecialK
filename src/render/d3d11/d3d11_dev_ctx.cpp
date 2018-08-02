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

#include <SpecialK/render/d3d11/d3d11_interfaces.h>
#include <SpecialK/render/d3d11/d3d11_3.h>

#include <atlbase.h>
#include <comdef.h>

extern volatile LONG __SKX_ComputeAntiStall;

extern void
SK_D3D11_ResetContextState (ID3D11DeviceContext* pDevCtx);

extern void
SK_D3D11_MergeCommandLists ( ID3D11DeviceContext *pSurrogate,
                             ID3D11DeviceContext *pMerge );

// THe device context a command list was built using
const GUID SKID_D3D11DeviceContextOrigin =
// {5C5298CA-0F9D-5022-A19D-A2E69792AE03}
  { 0x5c5298ca, 0xf9d,  0x5022, { 0xa1, 0x9d, 0xa2, 0xe6, 0x97, 0x92, 0xae, 0x3 } };


class SK_IWrapD3D11DeviceContext : public ID3D11DeviceContext
{
public:
  SK_IWrapD3D11DeviceContext (ID3D11DeviceContext* dev_ctx) : pReal (dev_ctx)
  {
    InterlockedExchange (&refs_, dev_ctx->AddRef  ());
                                 dev_ctx->Release ();
  };

  virtual HRESULT STDMETHODCALLTYPE QueryInterface (REFIID riid,_COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject) override
  {
    if ( IsEqualGUID (riid, IID_IUnwrappedD3D11DeviceContext) )
    {
    //assert (ppvObject != nullptr);
      *ppvObject = pReal;

      pReal->AddRef ();

      return S_OK;
    }

    return pReal->QueryInterface (riid, ppvObject);
  };

  virtual ULONG STDMETHODCALLTYPE AddRef  (void) override { return pReal->AddRef  (); }
  virtual ULONG STDMETHODCALLTYPE Release (void) override { return pReal->Release (); }

  virtual void STDMETHODCALLTYPE GetDevice (
            /* [annotation] */
    _Out_  ID3D11Device **ppDevice) override
  {
    return pReal->GetDevice (ppDevice);
  }
        
  virtual HRESULT STDMETHODCALLTYPE GetPrivateData ( _In_    REFGUID guid,
                                                     _Inout_ UINT   *pDataSize,
                       _Out_writes_bytes_opt_( *pDataSize )  void   *pData ) override {
    return pReal->GetPrivateData (guid, pDataSize, pData);
  }
        
  virtual HRESULT STDMETHODCALLTYPE SetPrivateData (
            /* [annotation] */ 
            _In_  REFGUID guid,
            /* [annotation] */ 
            _In_  UINT DataSize,
            /* [annotation] */ 
            _In_reads_bytes_opt_( DataSize )  const void *pData) override
  {
    return pReal->SetPrivateData (guid, DataSize, pData);
  }
        
  virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface (
            /* [annotation] */
    _In_  REFGUID guid,
    /* [annotation] */
    _In_opt_  const IUnknown *pData) override
  {
    return pReal->SetPrivateDataInterface (guid, pData);
  }

  virtual void STDMETHODCALLTYPE VSSetConstantBuffers (
      /* [annotation] */
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
    /* [annotation] */
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
    /* [annotation] */
    _In_reads_opt_ (NumBuffers)  ID3D11Buffer *const *ppConstantBuffers) override
  {
    return pReal->VSSetConstantBuffers (StartSlot, NumBuffers, ppConstantBuffers);
  }
        
  virtual void STDMETHODCALLTYPE PSSetShaderResources (
      /* [annotation] */
    _In_range_ (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
    /* [annotation] */
    _In_range_ (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumViews,
    /* [annotation] */
    _In_reads_opt_ (NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews) override
  {
    return pReal->PSSetShaderResources (StartSlot, NumViews, ppShaderResourceViews);
  }
        
  virtual void STDMETHODCALLTYPE PSSetShader (
      /* [annotation] */
    _In_opt_  ID3D11PixelShader *pPixelShader,
    /* [annotation] */
    _In_reads_opt_ (NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
    UINT NumClassInstances) override
  {
    return pReal->PSSetShader (pPixelShader, ppClassInstances, NumClassInstances);
  }
        
  virtual void STDMETHODCALLTYPE PSSetSamplers (
      /* [annotation] */
    _In_range_ (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,
    /* [annotation] */
    _In_range_ (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,
    /* [annotation] */
    _In_reads_opt_ (NumSamplers)  ID3D11SamplerState *const *ppSamplers) override
  {
    return pReal->PSSetSamplers (StartSlot, NumSamplers, ppSamplers);
  }
        
  virtual void STDMETHODCALLTYPE VSSetShader (
      /* [annotation] */
    _In_opt_  ID3D11VertexShader *pVertexShader,
    /* [annotation] */
    _In_reads_opt_ (NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
    UINT NumClassInstances) override
  {
    return pReal->VSSetShader (pVertexShader, ppClassInstances, NumClassInstances);
  }
        
  virtual void STDMETHODCALLTYPE DrawIndexed (
      /* [annotation] */
    _In_  UINT IndexCount,
    /* [annotation] */
    _In_  UINT StartIndexLocation,
    /* [annotation] */
    _In_  INT BaseVertexLocation) override
  {
    return pReal->DrawIndexed (IndexCount, StartIndexLocation, BaseVertexLocation);
  }
        
  virtual void STDMETHODCALLTYPE Draw (
      /* [annotation] */
    _In_  UINT VertexCount,
    /* [annotation] */
    _In_  UINT StartVertexLocation) override
  {
    return pReal->Draw (VertexCount, StartVertexLocation);
  }
        
  virtual HRESULT STDMETHODCALLTYPE Map (
      /* [annotation] */
    _In_  ID3D11Resource *pResource,
    /* [annotation] */
    _In_  UINT Subresource,
    /* [annotation] */
    _In_  D3D11_MAP MapType,
    /* [annotation] */
    _In_  UINT MapFlags,
    /* [annotation] */
    _Out_  D3D11_MAPPED_SUBRESOURCE *pMappedResource) override
  {
    return pReal->Map (pResource, Subresource, MapType, MapFlags, pMappedResource);
  }
        
  virtual void STDMETHODCALLTYPE Unmap (
      /* [annotation] */
    _In_  ID3D11Resource *pResource,
    /* [annotation] */
    _In_  UINT Subresource) override
  {
    return pReal->Unmap (pResource, Subresource);
  }
        
  virtual void STDMETHODCALLTYPE PSSetConstantBuffers (
      /* [annotation] */
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
    /* [annotation] */
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
    /* [annotation] */
    _In_reads_opt_ (NumBuffers)  ID3D11Buffer *const *ppConstantBuffers) override
  {
    return pReal->PSSetConstantBuffers (StartSlot, NumBuffers, ppConstantBuffers);
  }
        
  virtual void STDMETHODCALLTYPE IASetInputLayout( 
      /* [annotation] */ 
      _In_opt_  ID3D11InputLayout *pInputLayout) override {
    return pReal->IASetInputLayout (pInputLayout);
  }
        
        virtual void STDMETHODCALLTYPE IASetVertexBuffers( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppVertexBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  const UINT *pStrides,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  const UINT *pOffsets) override {
          return pReal->IASetVertexBuffers (StartSlot, NumBuffers, ppVertexBuffers, pStrides, pOffsets);
        }
        
        virtual void STDMETHODCALLTYPE IASetIndexBuffer( 
            /* [annotation] */ 
            _In_opt_  ID3D11Buffer *pIndexBuffer,
            /* [annotation] */ 
            _In_  DXGI_FORMAT Format,
            /* [annotation] */ 
            _In_  UINT Offset) override {
          return pReal->IASetIndexBuffer (pIndexBuffer, Format, Offset);
        }
        
        virtual void STDMETHODCALLTYPE DrawIndexedInstanced (
            /* [annotation] */
          _In_  UINT IndexCountPerInstance,
          /* [annotation] */
          _In_  UINT InstanceCount,
          /* [annotation] */
          _In_  UINT StartIndexLocation,
          /* [annotation] */
          _In_  INT BaseVertexLocation,
          /* [annotation] */
          _In_  UINT StartInstanceLocation) override
        {
          return pReal->DrawIndexedInstanced (IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
        }
        
        virtual void STDMETHODCALLTYPE DrawInstanced (
            /* [annotation] */
          _In_  UINT VertexCountPerInstance,
          /* [annotation] */
          _In_  UINT InstanceCount,
          /* [annotation] */
          _In_  UINT StartVertexLocation,
          /* [annotation] */
          _In_  UINT StartInstanceLocation) override
        {
          return pReal->DrawInstanced (VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
        }
        
        virtual void STDMETHODCALLTYPE GSSetConstantBuffers( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers) override
        {
          return pReal->GSSetConstantBuffers (StartSlot, NumBuffers, ppConstantBuffers);
        }
        
        virtual void STDMETHODCALLTYPE GSSetShader( 
            /* [annotation] */ 
            _In_opt_  ID3D11GeometryShader *pShader,
            /* [annotation] */ 
            _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
            UINT NumClassInstances) override
        {
          return pReal->GSSetShader (pShader, ppClassInstances, NumClassInstances);
        }
        
        virtual void STDMETHODCALLTYPE IASetPrimitiveTopology (
            /* [annotation] */
          _In_  D3D11_PRIMITIVE_TOPOLOGY Topology) override
        {
          return pReal->IASetPrimitiveTopology (Topology);
        }
        
        virtual void STDMETHODCALLTYPE VSSetShaderResources( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _In_reads_opt_(NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews) override
        {
          return pReal->VSSetShaderResources (StartSlot, NumViews, ppShaderResourceViews);
        }
        
        virtual void STDMETHODCALLTYPE VSSetSamplers (
            /* [annotation] */
          _In_range_ (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,
          /* [annotation] */
          _In_range_ (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,
          /* [annotation] */
          _In_reads_opt_ (NumSamplers)  ID3D11SamplerState *const *ppSamplers) override
        {
          return pReal->VSSetSamplers (StartSlot, NumSamplers, ppSamplers);
        }
        
        virtual void STDMETHODCALLTYPE Begin( 
            /* [annotation] */ 
            _In_  ID3D11Asynchronous *pAsync) override
        {
          return pReal->Begin (pAsync);
        }
        
        virtual void STDMETHODCALLTYPE End( 
            /* [annotation] */ 
            _In_  ID3D11Asynchronous *pAsync) override
        {
          return pReal->End (pAsync);
        }
        
        virtual HRESULT STDMETHODCALLTYPE GetData( 
            /* [annotation] */ 
            _In_  ID3D11Asynchronous *pAsync,
            /* [annotation] */ 
            _Out_writes_bytes_opt_( DataSize )  void *pData,
            /* [annotation] */ 
            _In_  UINT DataSize,
            /* [annotation] */ 
            _In_  UINT GetDataFlags) override
        {
          return pReal->GetData (pAsync, pData, DataSize, GetDataFlags);
        };
        
        virtual void STDMETHODCALLTYPE SetPredication( 
            /* [annotation] */ 
            _In_opt_  ID3D11Predicate *pPredicate,
            /* [annotation] */ 
            _In_  BOOL PredicateValue) override
        {
          return pReal->SetPredication (pPredicate, PredicateValue);
        }
        
        virtual void STDMETHODCALLTYPE GSSetShaderResources( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _In_reads_opt_(NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews) override
        {
          return pReal->GSSetShaderResources (StartSlot, NumViews, ppShaderResourceViews);
        }
        
        virtual void STDMETHODCALLTYPE GSSetSamplers (
            /* [annotation] */
          _In_range_ (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,
          /* [annotation] */
          _In_range_ (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,
          /* [annotation] */
          _In_reads_opt_ (NumSamplers)  ID3D11SamplerState *const *ppSamplers) override
        {
          return pReal->GSSetSamplers (StartSlot, NumSamplers, ppSamplers);
        }
        
        virtual void STDMETHODCALLTYPE OMSetRenderTargets (
            /* [annotation] */
          _In_range_ (0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT)  UINT NumViews,
          /* [annotation] */
          _In_reads_opt_ (NumViews)  ID3D11RenderTargetView *const *ppRenderTargetViews,
          /* [annotation] */
          _In_opt_  ID3D11DepthStencilView *pDepthStencilView) override
        {
          return pReal->OMSetRenderTargets (NumViews, ppRenderTargetViews, pDepthStencilView);
        }
        
        virtual void STDMETHODCALLTYPE OMSetRenderTargetsAndUnorderedAccessViews (
            /* [annotation] */
          _In_  UINT NumRTVs,
          /* [annotation] */
          _In_reads_opt_ (NumRTVs)  ID3D11RenderTargetView *const *ppRenderTargetViews,
          /* [annotation] */
          _In_opt_  ID3D11DepthStencilView *pDepthStencilView,
          /* [annotation] */
          _In_range_ (0, D3D11_1_UAV_SLOT_COUNT - 1)  UINT UAVStartSlot,
          /* [annotation] */
          _In_  UINT NumUAVs,
          /* [annotation] */
          _In_reads_opt_ (NumUAVs)  ID3D11UnorderedAccessView *const *ppUnorderedAccessViews,
          /* [annotation] */
          _In_reads_opt_ (NumUAVs)  const UINT *pUAVInitialCounts) override
        {
          return pReal->OMSetRenderTargetsAndUnorderedAccessViews (NumRTVs, ppRenderTargetViews, pDepthStencilView, UAVStartSlot, NumUAVs, ppUnorderedAccessViews, pUAVInitialCounts);
        }
        
        virtual void STDMETHODCALLTYPE OMSetBlendState (
            /* [annotation] */
          _In_opt_  ID3D11BlendState *pBlendState,
          /* [annotation] */
          _In_opt_  const FLOAT BlendFactor [4],
          /* [annotation] */
          _In_  UINT SampleMask) override
        {
          return pReal->OMSetBlendState (pBlendState, BlendFactor, SampleMask);
        }
        
        virtual void STDMETHODCALLTYPE OMSetDepthStencilState (
            /* [annotation] */
          _In_opt_  ID3D11DepthStencilState *pDepthStencilState,
          /* [annotation] */
          _In_  UINT StencilRef) override
        {
          return pReal->OMSetDepthStencilState (pDepthStencilState, StencilRef);
        }
        
        virtual void STDMETHODCALLTYPE SOSetTargets (
            /* [annotation] */
          _In_range_ (0, D3D11_SO_BUFFER_SLOT_COUNT)  UINT NumBuffers,
          /* [annotation] */
          _In_reads_opt_ (NumBuffers)  ID3D11Buffer *const *ppSOTargets,
          /* [annotation] */
          _In_reads_opt_ (NumBuffers)  const UINT *pOffsets) override
        {
          return pReal->SOSetTargets (NumBuffers, ppSOTargets, pOffsets);
        }
        
        virtual void STDMETHODCALLTYPE DrawAuto (void) override
        {
          return pReal->DrawAuto ();
        }
        
        virtual void STDMETHODCALLTYPE DrawIndexedInstancedIndirect (
            /* [annotation] */
          _In_  ID3D11Buffer *pBufferForArgs,
          /* [annotation] */
          _In_  UINT AlignedByteOffsetForArgs) override
        {
          return pReal->DrawIndexedInstancedIndirect (pBufferForArgs, AlignedByteOffsetForArgs);
        }
        
        virtual void STDMETHODCALLTYPE DrawInstancedIndirect (
            /* [annotation] */
          _In_  ID3D11Buffer *pBufferForArgs,
          /* [annotation] */
          _In_  UINT AlignedByteOffsetForArgs) override
        {
          return pReal->DrawInstancedIndirect (pBufferForArgs, AlignedByteOffsetForArgs);
        }
        
        virtual void STDMETHODCALLTYPE Dispatch (
            /* [annotation] */
          _In_  UINT ThreadGroupCountX,
          /* [annotation] */
          _In_  UINT ThreadGroupCountY,
          /* [annotation] */
          _In_  UINT ThreadGroupCountZ) override
        {
          return pReal->Dispatch (ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
        }
        
        virtual void STDMETHODCALLTYPE DispatchIndirect( 
            /* [annotation] */ 
            _In_  ID3D11Buffer *pBufferForArgs,
            /* [annotation] */ 
            _In_  UINT AlignedByteOffsetForArgs) override
        {
          return pReal->DispatchIndirect (pBufferForArgs, AlignedByteOffsetForArgs);
        }
        
        virtual void STDMETHODCALLTYPE RSSetState (
            /* [annotation] */
          _In_opt_  ID3D11RasterizerState *pRasterizerState) override
        {
          return pReal->RSSetState (pRasterizerState);
        }
        
        virtual void STDMETHODCALLTYPE RSSetViewports (
            /* [annotation] */
          _In_range_ (0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)  UINT NumViewports,
          /* [annotation] */
          _In_reads_opt_ (NumViewports)  const D3D11_VIEWPORT *pViewports) override
        {
          return pReal->RSSetViewports (NumViewports, pViewports);
        }
        
        virtual void STDMETHODCALLTYPE RSSetScissorRects (
            /* [annotation] */
          _In_range_ (0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)  UINT NumRects,
          /* [annotation] */
          _In_reads_opt_ (NumRects)  const D3D11_RECT *pRects) override
        {
          return pReal->RSSetScissorRects (NumRects, pRects);
        }
        
        virtual void STDMETHODCALLTYPE CopySubresourceRegion (
            /* [annotation] */
          _In_  ID3D11Resource *pDstResource,
          /* [annotation] */
          _In_  UINT DstSubresource,
          /* [annotation] */
          _In_  UINT DstX,
          /* [annotation] */
          _In_  UINT DstY,
          /* [annotation] */
          _In_  UINT DstZ,
          /* [annotation] */
          _In_  ID3D11Resource *pSrcResource,
          /* [annotation] */
          _In_  UINT SrcSubresource,
          /* [annotation] */
          _In_opt_  const D3D11_BOX *pSrcBox) override
        {
          return pReal->CopySubresourceRegion (pDstResource, DstSubresource, DstX, DstY, DstZ, pSrcResource, SrcSubresource, pSrcBox);
        }
        
        virtual void STDMETHODCALLTYPE CopyResource (
            /* [annotation] */
          _In_  ID3D11Resource *pDstResource,
          /* [annotation] */
          _In_  ID3D11Resource *pSrcResource) override
        {
          return pReal->CopyResource (pDstResource, pSrcResource);
        }
        
        virtual void STDMETHODCALLTYPE UpdateSubresource (
            /* [annotation] */
          _In_  ID3D11Resource *pDstResource,
          /* [annotation] */
          _In_  UINT DstSubresource,
          /* [annotation] */
          _In_opt_  const D3D11_BOX *pDstBox,
          /* [annotation] */
          _In_  const void *pSrcData,
          /* [annotation] */
          _In_  UINT SrcRowPitch,
          /* [annotation] */
          _In_  UINT SrcDepthPitch) override
        {
          return pReal->UpdateSubresource (pDstResource, DstSubresource, pDstBox, pSrcData, SrcRowPitch, SrcDepthPitch);
        }
        
        virtual void STDMETHODCALLTYPE CopyStructureCount (
            /* [annotation] */
          _In_  ID3D11Buffer *pDstBuffer,
          /* [annotation] */
          _In_  UINT DstAlignedByteOffset,
          /* [annotation] */
          _In_  ID3D11UnorderedAccessView *pSrcView) override
        {
          return pReal->CopyStructureCount (pDstBuffer, DstAlignedByteOffset, pSrcView);
        }
        
        virtual void STDMETHODCALLTYPE ClearRenderTargetView (
            /* [annotation] */
          _In_  ID3D11RenderTargetView *pRenderTargetView,
          /* [annotation] */
          _In_  const FLOAT ColorRGBA [4]) override
        {
          return pReal->ClearRenderTargetView (pRenderTargetView, ColorRGBA);
        }
        
        virtual void STDMETHODCALLTYPE ClearUnorderedAccessViewUint (
            /* [annotation] */
          _In_  ID3D11UnorderedAccessView *pUnorderedAccessView,
          /* [annotation] */
          _In_  const UINT Values [4]) override
        {
          return pReal->ClearUnorderedAccessViewUint (pUnorderedAccessView, Values);
        }
        
        virtual void STDMETHODCALLTYPE ClearUnorderedAccessViewFloat (
            /* [annotation] */
          _In_  ID3D11UnorderedAccessView *pUnorderedAccessView,
          /* [annotation] */
          _In_  const FLOAT Values [4]) override
        {
          return pReal->ClearUnorderedAccessViewFloat (pUnorderedAccessView, Values);
        }
        
        virtual void STDMETHODCALLTYPE ClearDepthStencilView (
            /* [annotation] */
          _In_  ID3D11DepthStencilView *pDepthStencilView,
          /* [annotation] */
          _In_  UINT ClearFlags,
          /* [annotation] */
          _In_  FLOAT Depth,
          /* [annotation] */
          _In_  UINT8 Stencil) override
        {
          return pReal->ClearDepthStencilView (pDepthStencilView, ClearFlags, Depth, Stencil);
        }
        
        virtual void STDMETHODCALLTYPE GenerateMips (
            /* [annotation] */
          _In_  ID3D11ShaderResourceView *pShaderResourceView) override
        {
          return pReal->GenerateMips (pShaderResourceView);
        }
        
        virtual void STDMETHODCALLTYPE SetResourceMinLOD (
            /* [annotation] */
          _In_  ID3D11Resource *pResource,
          FLOAT MinLOD) override
        {
          return pReal->SetResourceMinLOD (pResource, MinLOD);
        }
        
        virtual FLOAT STDMETHODCALLTYPE GetResourceMinLOD (
            /* [annotation] */
          _In_  ID3D11Resource *pResource) override
        {
          return pReal->GetResourceMinLOD (pResource);
        }
        
        virtual void STDMETHODCALLTYPE ResolveSubresource (
            /* [annotation] */
          _In_  ID3D11Resource *pDstResource,
          /* [annotation] */
          _In_  UINT DstSubresource,
          /* [annotation] */
          _In_  ID3D11Resource *pSrcResource,
          /* [annotation] */
          _In_  UINT SrcSubresource,
          /* [annotation] */
          _In_  DXGI_FORMAT Format) override
        {
          return pReal->ResolveSubresource (pDstResource, DstSubresource, pSrcResource, SrcSubresource, Format);
        }
        
        virtual void STDMETHODCALLTYPE ExecuteCommandList (
            /* [annotation] */
          _In_  ID3D11CommandList *pCommandList,
          BOOL RestoreContextState) override
        {
          // Fix for Yakuza0, why the hell is it passing nullptr?!
          if (pCommandList == nullptr)
            return pReal->ExecuteCommandList (pCommandList, RestoreContextState);

          CComPtr <ID3D11DeviceContext> pBuildContext;
          UINT                          size = 0;

          if ( SUCCEEDED ( pCommandList->GetPrivateData (
                             SKID_D3D11DeviceContextOrigin,
                               &size, &pBuildContext )
                         )    &&    (! pBuildContext.IsEqualObject (this) )
             )
          {
               SK_D3D11_MergeCommandLists ( pBuildContext,        this    );
            pBuildContext->SetPrivateData ( SKID_D3D11DeviceContextOrigin,
                                              sizeof (ptrdiff_t), nullptr );

          }

          pReal->ExecuteCommandList  (pCommandList, RestoreContextState);
          SK_D3D11_ResetContextState (pBuildContext);

          if (! RestoreContextState)
          {
            SK_D3D11_ResetContextState (pReal);
          }
        }
        
        virtual void STDMETHODCALLTYPE HSSetShaderResources (
            /* [annotation] */
          _In_range_ (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
          /* [annotation] */
          _In_range_ (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumViews,
          /* [annotation] */
          _In_reads_opt_ (NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews) override
        {
          return pReal->HSSetShaderResources (StartSlot, NumViews, ppShaderResourceViews);
        }
        
        virtual void STDMETHODCALLTYPE HSSetShader (
            /* [annotation] */
          _In_opt_  ID3D11HullShader *pHullShader,
          /* [annotation] */
          _In_reads_opt_ (NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
          UINT NumClassInstances) override
        {
          return pReal->HSSetShader (pHullShader, ppClassInstances, NumClassInstances);
        }
        
        virtual void STDMETHODCALLTYPE HSSetSamplers (
            /* [annotation] */
          _In_range_ (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,
          /* [annotation] */
          _In_range_ (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,
          /* [annotation] */
          _In_reads_opt_ (NumSamplers)  ID3D11SamplerState *const *ppSamplers) override
        {
          return pReal->HSSetSamplers (StartSlot, NumSamplers, ppSamplers);
        }
        
        virtual void STDMETHODCALLTYPE HSSetConstantBuffers (
            /* [annotation] */
          _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
          /* [annotation] */
          _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
          /* [annotation] */
          _In_reads_opt_ (NumBuffers)  ID3D11Buffer *const *ppConstantBuffers) override
        {
          return pReal->HSSetConstantBuffers (StartSlot, NumBuffers, ppConstantBuffers);
        }
        
        virtual void STDMETHODCALLTYPE DSSetShaderResources (
            /* [annotation] */
          _In_range_ (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
          /* [annotation] */
          _In_range_ (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumViews,
          /* [annotation] */
          _In_reads_opt_ (NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews) override
        {
          return pReal->DSSetShaderResources (StartSlot, NumViews, ppShaderResourceViews);
        }
        
        virtual void STDMETHODCALLTYPE DSSetShader (
            /* [annotation] */
          _In_opt_  ID3D11DomainShader *pDomainShader,
          /* [annotation] */
          _In_reads_opt_ (NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
          UINT NumClassInstances) override
        {
          return pReal->DSSetShader (pDomainShader, ppClassInstances, NumClassInstances);
        }
        
        virtual void STDMETHODCALLTYPE DSSetSamplers (
            /* [annotation] */
          _In_range_ (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,
          /* [annotation] */
          _In_range_ (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,
          /* [annotation] */
          _In_reads_opt_ (NumSamplers)  ID3D11SamplerState *const *ppSamplers) override
        {
          return pReal->DSSetSamplers (StartSlot, NumSamplers, ppSamplers);
        }
        
        virtual void STDMETHODCALLTYPE DSSetConstantBuffers (
            /* [annotation] */
          _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
          /* [annotation] */
          _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
          /* [annotation] */
          _In_reads_opt_ (NumBuffers)  ID3D11Buffer *const *ppConstantBuffers) override
        {
          return pReal->DSSetConstantBuffers (StartSlot, NumBuffers, ppConstantBuffers);
        }
        
        virtual void STDMETHODCALLTYPE CSSetShaderResources (
            /* [annotation] */
          _In_range_ (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
          /* [annotation] */
          _In_range_ (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumViews,
          /* [annotation] */
          _In_reads_opt_ (NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews) override
        {
          return pReal->CSSetShaderResources (StartSlot, NumViews, ppShaderResourceViews);
        }
        
        virtual void STDMETHODCALLTYPE CSSetUnorderedAccessViews( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_1_UAV_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_1_UAV_SLOT_COUNT - StartSlot )  UINT NumUAVs,
            /* [annotation] */ 
            _In_reads_opt_(NumUAVs)  ID3D11UnorderedAccessView *const *ppUnorderedAccessViews,
            /* [annotation] */ 
            _In_reads_opt_(NumUAVs)  const UINT *pUAVInitialCounts) override {
          return pReal->CSSetUnorderedAccessViews (StartSlot, NumUAVs, ppUnorderedAccessViews, pUAVInitialCounts);
        }
        
        virtual void STDMETHODCALLTYPE CSSetShader (
            /* [annotation] */
          _In_opt_  ID3D11ComputeShader *pComputeShader,
          /* [annotation] */
          _In_reads_opt_ (NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
          UINT NumClassInstances)
        {
          return pReal->CSSetShader (pComputeShader, ppClassInstances, NumClassInstances);
        }
        
        virtual void STDMETHODCALLTYPE CSSetSamplers (
            /* [annotation] */
          _In_range_ (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,
          /* [annotation] */
          _In_range_ (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,
          /* [annotation] */
          _In_reads_opt_ (NumSamplers)  ID3D11SamplerState *const *ppSamplers) override
        {
          return pReal->CSSetSamplers (StartSlot, NumSamplers, ppSamplers);
        }
        
        virtual void STDMETHODCALLTYPE CSSetConstantBuffers (
            /* [annotation] */
          _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
          /* [annotation] */
          _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
          /* [annotation] */
          _In_reads_opt_ (NumBuffers)  ID3D11Buffer *const *ppConstantBuffers) override
        {
          return pReal->CSSetConstantBuffers (StartSlot, NumBuffers, ppConstantBuffers);
        }
        
        virtual void STDMETHODCALLTYPE VSGetConstantBuffers (
            /* [annotation] */
          _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
          /* [annotation] */
          _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
          /* [annotation] */
          _Out_writes_opt_ (NumBuffers)  ID3D11Buffer **ppConstantBuffers) override
        {
          return pReal->VSGetConstantBuffers (StartSlot, NumBuffers, ppConstantBuffers);
        }
        
        virtual void STDMETHODCALLTYPE PSGetShaderResources (
            /* [annotation] */
          _In_range_ (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
          /* [annotation] */
          _In_range_ (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumViews,
          /* [annotation] */
          _Out_writes_opt_ (NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews) override
        {
          return pReal->PSGetShaderResources (StartSlot, NumViews, ppShaderResourceViews);
        }
        
        virtual void STDMETHODCALLTYPE PSGetShader (
            /* [annotation] */
          _Out_  ID3D11PixelShader **ppPixelShader,
          /* [annotation] */
          _Out_writes_opt_ (*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
          /* [annotation] */
          _Inout_opt_  UINT *pNumClassInstances) override
        {
          return pReal->PSGetShader (ppPixelShader, ppClassInstances, pNumClassInstances);
        }
        
        virtual void STDMETHODCALLTYPE PSGetSamplers (
            /* [annotation] */
          _In_range_ (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,
          /* [annotation] */
          _In_range_ (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,
          /* [annotation] */
          _Out_writes_opt_ (NumSamplers)  ID3D11SamplerState **ppSamplers) override
        {
          return pReal->PSGetSamplers (StartSlot, NumSamplers, ppSamplers);
        }
        
        virtual void STDMETHODCALLTYPE VSGetShader (
            /* [annotation] */
          _Out_  ID3D11VertexShader **ppVertexShader,
          /* [annotation] */
          _Out_writes_opt_ (*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
          /* [annotation] */
          _Inout_opt_  UINT *pNumClassInstances) override
        {
          return pReal->VSGetShader (ppVertexShader, ppClassInstances, pNumClassInstances);
        }
        
        virtual void STDMETHODCALLTYPE PSGetConstantBuffers (
            /* [annotation] */
          _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
          /* [annotation] */
          _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
          /* [annotation] */
          _Out_writes_opt_ (NumBuffers)  ID3D11Buffer **ppConstantBuffers) override
        {
          return pReal->PSGetConstantBuffers (StartSlot, NumBuffers, ppConstantBuffers);
        }
        
        virtual void STDMETHODCALLTYPE IAGetInputLayout (
            /* [annotation] */
          _Out_  ID3D11InputLayout **ppInputLayout) override
        {
          return pReal->IAGetInputLayout (ppInputLayout);
        }
        
        virtual void STDMETHODCALLTYPE IAGetVertexBuffers (
            /* [annotation] */
          _In_range_ (0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
          /* [annotation] */
          _In_range_ (0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumBuffers,
          /* [annotation] */
          _Out_writes_opt_ (NumBuffers)  ID3D11Buffer **ppVertexBuffers,
          /* [annotation] */
          _Out_writes_opt_ (NumBuffers)  UINT *pStrides,
          /* [annotation] */
          _Out_writes_opt_ (NumBuffers)  UINT *pOffsets) override
        {
          return pReal->IAGetVertexBuffers (StartSlot, NumBuffers, ppVertexBuffers, pStrides, pOffsets);
        }
        
        virtual void STDMETHODCALLTYPE IAGetIndexBuffer (
            /* [annotation] */
          _Out_opt_  ID3D11Buffer **pIndexBuffer,
          /* [annotation] */
          _Out_opt_  DXGI_FORMAT *Format,
          /* [annotation] */
          _Out_opt_  UINT *Offset) override
        {
          return pReal->IAGetIndexBuffer (pIndexBuffer, Format, Offset);
        }
        
        virtual void STDMETHODCALLTYPE GSGetConstantBuffers( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers) override
        {
          return pReal->GSGetConstantBuffers (StartSlot, NumBuffers, ppConstantBuffers);
        }
        
        virtual void STDMETHODCALLTYPE GSGetShader (
            /* [annotation] */
          _Out_  ID3D11GeometryShader **ppGeometryShader,
          /* [annotation] */
          _Out_writes_opt_ (*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
          /* [annotation] */
          _Inout_opt_  UINT *pNumClassInstances) override
        {
          return pReal->GSGetShader (ppGeometryShader, ppClassInstances, pNumClassInstances);
        }
        
        virtual void STDMETHODCALLTYPE IAGetPrimitiveTopology (
            /* [annotation] */
          _Out_  D3D11_PRIMITIVE_TOPOLOGY *pTopology) override
        {
          return pReal->IAGetPrimitiveTopology (pTopology);
        }
        
        virtual void STDMETHODCALLTYPE VSGetShaderResources (
            /* [annotation] */
          _In_range_ (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
          /* [annotation] */
          _In_range_ (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumViews,
          /* [annotation] */
          _Out_writes_opt_ (NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews) override
        {
          return pReal->VSGetShaderResources (StartSlot, NumViews, ppShaderResourceViews);
        }
        
        virtual void STDMETHODCALLTYPE VSGetSamplers( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
            /* [annotation] */ 
            _Out_writes_opt_(NumSamplers)  ID3D11SamplerState **ppSamplers) override
        {
          return pReal->VSGetSamplers (StartSlot, NumSamplers, ppSamplers);
        }
        
        virtual void STDMETHODCALLTYPE GetPredication (
            /* [annotation] */
          _Out_opt_  ID3D11Predicate **ppPredicate,
          /* [annotation] */
          _Out_opt_  BOOL *pPredicateValue) override
        {
          return pReal->GetPredication (ppPredicate, pPredicateValue);
        }
        
        virtual void STDMETHODCALLTYPE GSGetShaderResources( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews) override
        {
          return pReal->GSGetShaderResources (StartSlot, NumViews, ppShaderResourceViews);
        }
        
        virtual void STDMETHODCALLTYPE GSGetSamplers (
            /* [annotation] */
          _In_range_ (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,
          /* [annotation] */
          _In_range_ (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,
          /* [annotation] */
          _Out_writes_opt_ (NumSamplers)  ID3D11SamplerState **ppSamplers) override
        {
          return pReal->GSGetSamplers (StartSlot, NumSamplers, ppSamplers);
        }
        
        virtual void STDMETHODCALLTYPE OMGetRenderTargets (
            /* [annotation] */
          _In_range_ (0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT)  UINT NumViews,
          /* [annotation] */
          _Out_writes_opt_ (NumViews)  ID3D11RenderTargetView **ppRenderTargetViews,
          /* [annotation] */
          _Out_opt_  ID3D11DepthStencilView **ppDepthStencilView) override
        {
          return pReal->OMGetRenderTargets (NumViews, ppRenderTargetViews, ppDepthStencilView);
        }
        
        virtual void STDMETHODCALLTYPE OMGetRenderTargetsAndUnorderedAccessViews (
            /* [annotation] */
          _In_range_ (0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT)  UINT NumRTVs,
          /* [annotation] */
          _Out_writes_opt_ (NumRTVs)  ID3D11RenderTargetView **ppRenderTargetViews,
          /* [annotation] */
          _Out_opt_  ID3D11DepthStencilView **ppDepthStencilView,
          /* [annotation] */
          _In_range_ (0, D3D11_PS_CS_UAV_REGISTER_COUNT - 1)  UINT UAVStartSlot,
          /* [annotation] */
          _In_range_ (0, D3D11_PS_CS_UAV_REGISTER_COUNT - UAVStartSlot)  UINT NumUAVs,
          /* [annotation] */
          _Out_writes_opt_ (NumUAVs)  ID3D11UnorderedAccessView **ppUnorderedAccessViews) override
        {
          return pReal->OMGetRenderTargetsAndUnorderedAccessViews (NumRTVs, ppRenderTargetViews, ppDepthStencilView, UAVStartSlot, NumUAVs, ppUnorderedAccessViews);
        }
        
        virtual void STDMETHODCALLTYPE OMGetBlendState (
            /* [annotation] */
          _Out_opt_  ID3D11BlendState **ppBlendState,
          /* [annotation] */
          _Out_opt_  FLOAT BlendFactor [4],
          /* [annotation] */
          _Out_opt_  UINT *pSampleMask) override
        {
          return pReal->OMGetBlendState (ppBlendState, BlendFactor, pSampleMask);
        }
        
        virtual void STDMETHODCALLTYPE OMGetDepthStencilState( 
            /* [annotation] */ 
            _Out_opt_  ID3D11DepthStencilState **ppDepthStencilState,
            /* [annotation] */ 
            _Out_opt_  UINT *pStencilRef) override
        {
          return pReal->OMGetDepthStencilState (ppDepthStencilState, pStencilRef);
        }
        
        virtual void STDMETHODCALLTYPE SOGetTargets( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_SO_BUFFER_SLOT_COUNT )  UINT NumBuffers,
            /* [annotation] */ 
            _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppSOTargets) override
        {
          return pReal->SOGetTargets (NumBuffers, ppSOTargets);
        }
        
        virtual void STDMETHODCALLTYPE RSGetState( 
            /* [annotation] */ 
            _Out_  ID3D11RasterizerState **ppRasterizerState) override
        {
          return pReal->RSGetState (ppRasterizerState);
        }
        
        virtual void STDMETHODCALLTYPE RSGetViewports( 
            /* [annotation] */ 
            _Inout_ /*_range(0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE )*/   UINT *pNumViewports,
            /* [annotation] */ 
            _Out_writes_opt_(*pNumViewports)  D3D11_VIEWPORT *pViewports) override
        {
          return pReal->RSGetViewports (pNumViewports, pViewports);
        }
        
        virtual void STDMETHODCALLTYPE RSGetScissorRects (
            /* [annotation] */
          _Inout_ /*_range(0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE )*/   UINT *pNumRects,
          /* [annotation] */
          _Out_writes_opt_ (*pNumRects)  D3D11_RECT *pRects)
        {
          return pReal->RSGetScissorRects (pNumRects, pRects);
        }
        
        virtual void STDMETHODCALLTYPE HSGetShaderResources( 
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
            /* [annotation] */ 
            _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
            /* [annotation] */ 
            _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews) override
        {
          return pReal->HSGetShaderResources (StartSlot, NumViews, ppShaderResourceViews);
        }
        
        virtual void STDMETHODCALLTYPE HSGetShader (
            /* [annotation] */
          _Out_  ID3D11HullShader **ppHullShader,
          /* [annotation] */
          _Out_writes_opt_ (*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
          /* [annotation] */
          _Inout_opt_  UINT *pNumClassInstances) override
        {
          return pReal->HSGetShader (ppHullShader, ppClassInstances, pNumClassInstances);
        }
        
        virtual void STDMETHODCALLTYPE HSGetSamplers (
            /* [annotation] */
          _In_range_ (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,
          /* [annotation] */
          _In_range_ (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,
          /* [annotation] */
          _Out_writes_opt_ (NumSamplers)  ID3D11SamplerState **ppSamplers) override
        {
          return pReal->HSGetSamplers (StartSlot, NumSamplers, ppSamplers);
        }
        
        virtual void STDMETHODCALLTYPE HSGetConstantBuffers (
            /* [annotation] */
          _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
          /* [annotation] */
          _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
          /* [annotation] */
          _Out_writes_opt_ (NumBuffers)  ID3D11Buffer **ppConstantBuffers) override
        {
          return pReal->HSGetConstantBuffers (StartSlot, NumBuffers, ppConstantBuffers);
        }
        
        virtual void STDMETHODCALLTYPE DSGetShaderResources (
            /* [annotation] */
          _In_range_ (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
          /* [annotation] */
          _In_range_ (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumViews,
          /* [annotation] */
          _Out_writes_opt_ (NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews) override
        {
          return pReal->DSGetShaderResources (StartSlot, NumViews, ppShaderResourceViews);
        }
        
        virtual void STDMETHODCALLTYPE DSGetShader (
            /* [annotation] */
          _Out_  ID3D11DomainShader **ppDomainShader,
          /* [annotation] */
          _Out_writes_opt_ (*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
          /* [annotation] */
          _Inout_opt_  UINT *pNumClassInstances) override
        {
          return pReal->DSGetShader (ppDomainShader, ppClassInstances, pNumClassInstances);
        }
        
        virtual void STDMETHODCALLTYPE DSGetSamplers (
            /* [annotation] */
          _In_range_ (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,
          /* [annotation] */
          _In_range_ (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,
          /* [annotation] */
          _Out_writes_opt_ (NumSamplers)  ID3D11SamplerState **ppSamplers) override
        {
          return pReal->DSGetSamplers (StartSlot, NumSamplers, ppSamplers);
        }
        
        virtual void STDMETHODCALLTYPE DSGetConstantBuffers (
            /* [annotation] */
          _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
          /* [annotation] */
          _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
          /* [annotation] */
          _Out_writes_opt_ (NumBuffers)  ID3D11Buffer **ppConstantBuffers) override
        {
          return pReal->DSGetConstantBuffers (StartSlot, NumBuffers, ppConstantBuffers);
        }
        
        virtual void STDMETHODCALLTYPE CSGetShaderResources (
            /* [annotation] */
          _In_range_ (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
          /* [annotation] */
          _In_range_ (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumViews,
          /* [annotation] */
          _Out_writes_opt_ (NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews) override
        {
          return pReal->CSGetShaderResources (StartSlot, NumViews, ppShaderResourceViews);
        }
        
        virtual void STDMETHODCALLTYPE CSGetUnorderedAccessViews (
            /* [annotation] */
          _In_range_ (0, D3D11_PS_CS_UAV_REGISTER_COUNT - 1)  UINT StartSlot,
          /* [annotation] */
          _In_range_ (0, D3D11_PS_CS_UAV_REGISTER_COUNT - StartSlot)  UINT NumUAVs,
          /* [annotation] */
          _Out_writes_opt_ (NumUAVs)  ID3D11UnorderedAccessView **ppUnorderedAccessViews) override
        {
          return pReal->CSGetUnorderedAccessViews (StartSlot, NumUAVs, ppUnorderedAccessViews);
        }
        
        virtual void STDMETHODCALLTYPE CSGetShader (
            /* [annotation] */
          _Out_  ID3D11ComputeShader **ppComputeShader,
          /* [annotation] */
          _Out_writes_opt_ (*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,
          /* [annotation] */
          _Inout_opt_  UINT *pNumClassInstances) override
        {
          return pReal->CSGetShader (ppComputeShader, ppClassInstances, pNumClassInstances);
        }
        
        virtual void STDMETHODCALLTYPE CSGetSamplers (
            /* [annotation] */
          _In_range_ (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,
          /* [annotation] */
          _In_range_ (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,
          /* [annotation] */
          _Out_writes_opt_ (NumSamplers)  ID3D11SamplerState **ppSamplers) override
        {
          return pReal->CSGetSamplers (StartSlot, NumSamplers, ppSamplers);
        }
        
        virtual void STDMETHODCALLTYPE CSGetConstantBuffers (
            /* [annotation] */
          _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
          /* [annotation] */
          _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
          /* [annotation] */
          _Out_writes_opt_ (NumBuffers)  ID3D11Buffer **ppConstantBuffers) override
        {
          return pReal->CSGetConstantBuffers (StartSlot, NumBuffers, ppConstantBuffers);
        }
        
        virtual void STDMETHODCALLTYPE ClearState (void) override
        {
          pReal->ClearState ();

          SK_D3D11_ResetContextState (pReal);
        }
        
        virtual void STDMETHODCALLTYPE Flush (void) override
        {
          return pReal->Flush ();
        }
        
        virtual D3D11_DEVICE_CONTEXT_TYPE STDMETHODCALLTYPE GetType (void) override
        {
          return pReal->GetType ();
        }
        
        virtual UINT STDMETHODCALLTYPE GetContextFlags (void) override
        {
          return pReal->GetContextFlags ();
        }
        
        virtual HRESULT STDMETHODCALLTYPE FinishCommandList (
          BOOL RestoreDeferredContextState,
          /* [annotation] */
          _Out_opt_  ID3D11CommandList **ppCommandList) override
        {
          HRESULT hr =
            pReal->FinishCommandList (RestoreDeferredContextState, ppCommandList);

          //  Lord the documentation is contradictory ; assume that the way it is written,
          //    some kind of reset always happens. Even when "Restore" means Clear (WTF?)
          if (SUCCEEDED (hr) && (ppCommandList != nullptr))// && (! RestoreDeferredContextState))
          {
            (*ppCommandList)->SetPrivateData ( SKID_D3D11DeviceContextOrigin,
                                               sizeof (ptrdiff_t), this );

            //SK_D3D11_ResetContextState (pReal);
          }

          else
          {
            SK_D3D11_ResetContextState (pReal);
          }

          return hr;
        }

protected:
private:
  volatile LONG        refs_ = 1;
  ID3D11DeviceContext* pReal;
};