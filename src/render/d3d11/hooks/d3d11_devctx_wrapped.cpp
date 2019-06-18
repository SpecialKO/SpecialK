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

#define __SK_SUBSYSTEM__ L"D3D11 Wrap"

#include <SpecialK/render/d3d11/d3d11_tex_mgr.h>
#include <SpecialK/render/d3d11/d3d11_state_tracker.h>

#if (VER_PRODUCTBUILD < 10011)
const GUID IID_ID3D11DeviceContext2 =
{ 0x420d5b32, 0xb90c, 0x4da4, { 0xbe, 0xf0, 0x35, 0x9f, 0x6a, 0x24, 0xa8, 0x3a } };
const GUID IID_ID3D11DeviceContext3 =
{ 0xb4e3c01d, 0xe79e, 0x4637, { 0x91, 0xb2, 0x51, 0x0e, 0x9f, 0x4c, 0x9b, 0x8f } };
const GUID IID_ID3D11DeviceContext4 =
{ 0x917600da, 0xf58c, 0x4c33, { 0x98, 0xd8, 0x3e, 0x15, 0xb3, 0x90, 0xfa, 0x24 } };

const GUID IID_ID3D11Fence   =
{ 0xaffde9d1, 0x1df7, 0x4bb7, { 0x8a, 0x34, 0x0f, 0x46, 0x25, 0x1d, 0xab, 0x80 } };
#endif

// {9A222196-4D44-45C3-AAA4-2FD47915CC70}
const GUID IID_IUnwrappedD3D11DeviceContext =
{ 0xe8a22a3f, 0x1405, 0x424c, { 0xae, 0x99, 0xd, 0x3e, 0x9d, 0x54, 0x7c, 0x32 } };



extern SK_LazyGlobal <memory_tracking_s> mem_map_stats;
extern SK_LazyGlobal <target_tracking_s> tracked_rtv;

extern SK_LazyGlobal <std::unordered_set <ID3D11Texture2D *>>                         used_textures;
extern SK_LazyGlobal <std::unordered_map <ID3D11DeviceContext *, mapped_resources_s>> mapped_resources;


#define track_immediate config.render.dxgi.deferred_isolation

//#define SK_D3D11_LAZY_WRAP


class SK_IWrapD3D11DeviceContext : public ID3D11DeviceContext4
{
public:
  explicit SK_IWrapD3D11DeviceContext (ID3D11DeviceContext* dev_ctx) : pReal (dev_ctx)
  {
    InterlockedExchange (
      &refs_, pReal->AddRef  () - 1
    );        pReal->Release ();
    AddRef ();

    ver_ = 0;

    IUnknown *pPromotion = nullptr;

    if (SUCCEEDED (pReal->QueryInterface (IID_ID3D11DeviceContext4, (void **)&pPromotion)))
    {
      ver_ = 4;
    }

    else if (SUCCEEDED (pReal->QueryInterface (IID_ID3D11DeviceContext3, (void **)&pPromotion)))
    {
      ver_ = 3;
    }

    else if (SUCCEEDED (pReal->QueryInterface (IID_ID3D11DeviceContext2, (void **)&pPromotion)))
    {
      ver_ = 2;
    }

    else if (SUCCEEDED (pReal->QueryInterface (IID_ID3D11DeviceContext1, (void **)&pPromotion)))
    {
      ver_ = 1;
    }

    if (ver_ != 0)
    {
      Release ();

      pReal = (ID3D11DeviceContext *)pPromotion;

      SK_LOG0 ( ( L"Promoted ID3D11DeviceContext to ID3D11DeviceContext%li", ver_),
                  __SK_SUBSYSTEM__ );
    }

    dev_ctx_handle_ =
      SK_D3D11_GetDeviceContextHandle (this);

    SK_D3D11_CopyContextHandle (this, pReal);

    deferred_ =
      SK_D3D11_IsDevCtxDeferred (pReal);
  };

  explicit SK_IWrapD3D11DeviceContext (ID3D11DeviceContext1* dev_ctx) : pReal (dev_ctx)
  {
    InterlockedExchange (
      &refs_, pReal->AddRef  () - 1
    );        pReal->Release ();
    AddRef ();

    ver_ = 1;

    IUnknown *pPromotion = nullptr;

    if ( SUCCEEDED (pReal->QueryInterface (IID_ID3D11DeviceContext4, (void **)&pPromotion)) ||
         SUCCEEDED (pReal->QueryInterface (IID_ID3D11DeviceContext3, (void **)&pPromotion)) ||
         SUCCEEDED (pReal->QueryInterface (IID_ID3D11DeviceContext2, (void **)&pPromotion)) )
    {
      pPromotion->Release ();

      SK_LOG0 ( ( L"Promoted ID3D11DeviceContext1 to ID3D11DeviceContext%li", ver_),
                  __SK_SUBSYSTEM__ );
    }

    dev_ctx_handle_ =
      SK_D3D11_GetDeviceContextHandle (this);

    SK_D3D11_CopyContextHandle (this, pReal);

    deferred_ =
      SK_D3D11_IsDevCtxDeferred (pReal);
  };

  explicit SK_IWrapD3D11DeviceContext (ID3D11DeviceContext2* dev_ctx) : pReal (dev_ctx)
  {
    InterlockedExchange (
      &refs_, pReal->AddRef  () - 1
    );        pReal->Release ();
    AddRef ();


    ver_ = 2;

    IUnknown *pPromotion = nullptr;

    if ( SUCCEEDED (pReal->QueryInterface (IID_ID3D11DeviceContext4, (void **)&pPromotion)) ||
         SUCCEEDED (pReal->QueryInterface (IID_ID3D11DeviceContext3, (void **)&pPromotion)) )
    {
      pPromotion->Release ();

      SK_LOG0 ( ( L"Promoted ID3D11DeviceContext2 to ID3D11DeviceContext%li", ver_),
                  __SK_SUBSYSTEM__ );
    }

    dev_ctx_handle_ =
      SK_D3D11_GetDeviceContextHandle (this);

    SK_D3D11_CopyContextHandle (this, pReal);

    deferred_ =
      SK_D3D11_IsDevCtxDeferred (pReal);
  };

  explicit SK_IWrapD3D11DeviceContext (ID3D11DeviceContext3* dev_ctx) : pReal (dev_ctx)
  {
    InterlockedExchange (
      &refs_, pReal->AddRef  () - 1
    );        pReal->Release ();
    AddRef ();

    ver_ = 3;

    IUnknown *pPromotion = nullptr;

    if (SUCCEEDED (pReal->QueryInterface (IID_ID3D11DeviceContext4, (void **)&pPromotion)))
    {
      pPromotion->Release ();

      SK_LOG0 ( ( L"Promoted ID3D11DeviceContext3 to ID3D11DeviceContext%li", ver_),
                  __SK_SUBSYSTEM__ );
    }

    dev_ctx_handle_ =
      SK_D3D11_GetDeviceContextHandle (this);

    SK_D3D11_CopyContextHandle (this, pReal);

    deferred_ =
      SK_D3D11_IsDevCtxDeferred (pReal);
  };

  explicit SK_IWrapD3D11DeviceContext (ID3D11DeviceContext4* dev_ctx) : pReal (dev_ctx)
  {
    InterlockedExchange (
      &refs_, pReal->AddRef  () - 1
    );        pReal->Release ();
    AddRef ();

    ver_ = 4;

    dev_ctx_handle_ =
      SK_D3D11_GetDeviceContextHandle (this);

    SK_D3D11_CopyContextHandle (this, pReal);

    deferred_ =
      SK_D3D11_IsDevCtxDeferred (pReal);
  };

  virtual ~SK_IWrapD3D11DeviceContext (void) = default;

  HRESULT STDMETHODCALLTYPE QueryInterface ( REFIID riid,
                                            _COM_Outptr_ void
                                            __RPC_FAR *__RPC_FAR *ppvObj ) override
  {
    if (ppvObj == nullptr)
    {
      return E_POINTER;
    }

    if ( IsEqualGUID (riid, IID_IUnwrappedD3D11DeviceContext) != 0 )
    {
      //assert (ppvObject != nullptr);
      *ppvObj = pReal;

      pReal->AddRef ();

      return S_OK;
    }

    if (
      //riid == __uuidof (this)                 ||
      riid == __uuidof (IUnknown)             ||
      riid == __uuidof (ID3D11DeviceChild)    ||
      riid == __uuidof (ID3D11DeviceContext)  ||
      riid == __uuidof (ID3D11DeviceContext1) ||
      riid == __uuidof (ID3D11DeviceContext2) ||
      riid == __uuidof (ID3D11DeviceContext3) ||
      riid == __uuidof (ID3D11DeviceContext4))
    {
      auto _GetVersion = [](REFIID riid) ->
        UINT
      {
        if (riid == __uuidof (ID3D11DeviceContext))  return 0;
        if (riid == __uuidof (ID3D11DeviceContext1)) return 1;
        if (riid == __uuidof (ID3D11DeviceContext2)) return 2;
        if (riid == __uuidof (ID3D11DeviceContext3)) return 3;
        if (riid == __uuidof (ID3D11DeviceContext4)) return 4;

        return 0;
      };

      UINT required_ver =
        _GetVersion (riid);

      if (ver_ < required_ver)
      {
        IUnknown *pPromoted = nullptr;

        if ( FAILED (
               pReal->QueryInterface ( riid,
                                         (void **)&pPromoted )
                    ) || pPromoted == nullptr
           )
        {
          return E_NOINTERFACE;
        }

        ver_ =
          SK_COM_PromoteInterface (&pReal, pPromoted) ?
                                         required_ver : ver_;
      }

      AddRef ();

      *ppvObj = this;

      return S_OK;
    }

    return
      pReal->QueryInterface (riid, ppvObj);
  }

  ULONG STDMETHODCALLTYPE AddRef  (void) override
  {
    pReal->AddRef ();

    return
      (ULONG)InterlockedIncrement (&refs_);
  }
  ULONG STDMETHODCALLTYPE Release (void) override
  {
    ULONG xrefs =
      InterlockedDecrement (&refs_),
           refs = pReal->Release ();

    if (ReadAcquire (&refs_) == 0 && refs != 0)
    {
      assert (false);

      //refs = 0;
    }

    if (refs == 0)
    {
      SK_ReleaseAssert (ReadAcquire (&refs_) == 0);

      if (ReadAcquire (&refs_) == 0)
      {
        delete this;
      }
    }

    return xrefs;
  }

  void STDMETHODCALLTYPE GetDevice (
    _Out_  ID3D11Device **ppDevice ) override
  {
    return
      pReal->GetDevice (ppDevice);
  }

  HRESULT STDMETHODCALLTYPE GetPrivateData ( _In_                                 REFGUID guid,
                                             _Inout_                              UINT   *pDataSize,
                                             _Out_writes_bytes_opt_( *pDataSize ) void   *pData ) override
  {
    return
      pReal->GetPrivateData (
                guid, pDataSize,
                      pData
      );
  }

  HRESULT STDMETHODCALLTYPE SetPrivateData (
    _In_                                    REFGUID guid,
    _In_                                    UINT    DataSize,
    _In_reads_bytes_opt_( DataSize )  const void   *pData ) override
  {
    return
      pReal->SetPrivateData (
                guid,  DataSize,
                      pData
      );
  }

  HRESULT STDMETHODCALLTYPE SetPrivateDataInterface (
    _In_           REFGUID   guid,
    _In_opt_ const IUnknown *pData ) override
  {
    return
      pReal->SetPrivateDataInterface (guid, pData);
  }

  void STDMETHODCALLTYPE VSSetConstantBuffers (
    _In_range_     (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)         UINT                        StartSlot,
    _In_range_     (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot) UINT                        NumBuffers,
    _In_reads_opt_ (NumBuffers)                                                       ID3D11Buffer *const *ppConstantBuffers ) override
  {
    pReal->VSSetConstantBuffers ( StartSlot,
                     NumBuffers,
              ppConstantBuffers
    );
  }

  void STDMETHODCALLTYPE PSSetShaderResources (
    _In_range_     (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)          UINT                                          StartSlot,
    _In_range_     (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT                                          NumViews,
    _In_reads_opt_ (NumViews)                                                     ID3D11ShaderResourceView *const *ppShaderResourceViews ) override
  {
  #ifndef SK_D3D11_LAZY_WRAP
    if (! SK_D3D11_IgnoreWrappedOrDeferred (true, pReal))
        SK_D3D11_SetShaderResources_Impl (
           SK_D3D11_ShaderType::Pixel,
                       deferred_,
                  nullptr, pReal,
             StartSlot, NumViews,
           ppShaderResourceViews, dev_ctx_handle_
        );
    else
#endif
      pReal->PSSetShaderResources (StartSlot, NumViews, ppShaderResourceViews);
  }

  void STDMETHODCALLTYPE PSSetShader (
    _In_opt_                           ID3D11PixelShader           *pPixelShader,
    _In_reads_opt_ (NumClassInstances) ID3D11ClassInstance *const *ppClassInstances,
                                       UINT                       NumClassInstances ) override
  {
#ifndef SK_D3D11_LAZY_WRAP
if (! SK_D3D11_IgnoreWrappedOrDeferred (true, pReal))
      SK_D3D11_SetShader_Impl     (pReal,
            pPixelShader, sk_shader_class::Pixel,
                                  ppClassInstances,
                                 NumClassInstances, true,
                                 dev_ctx_handle_
      );
    else
#endif
      pReal->PSSetShader (pPixelShader, ppClassInstances, NumClassInstances);
  }

  void STDMETHODCALLTYPE PSSetSamplers (
    _In_range_     (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)          UINT                       StartSlot,
    _In_range_     (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT                       NumSamplers,
    _In_reads_opt_ (NumSamplers)                                           ID3D11SamplerState *const *ppSamplers ) override
  {
    pReal->PSSetSamplers ( StartSlot,
             NumSamplers,
              ppSamplers
    );
  }

  void STDMETHODCALLTYPE VSSetShader (
    _In_opt_                            ID3D11VertexShader         *pVertexShader,
    _In_reads_opt_ (NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
                                        UINT                       NumClassInstances ) override
  {
#ifndef SK_D3D11_LAZY_WRAP
  if (! SK_D3D11_IgnoreWrappedOrDeferred (true, pReal))
        SK_D3D11_SetShader_Impl   (pReal,
             pVertexShader, sk_shader_class::Vertex,
                                    ppClassInstances,
                                   NumClassInstances, true,
                                           dev_ctx_handle_
        );
    else
#endif
      pReal->VSSetShader (pVertexShader, ppClassInstances, NumClassInstances);
  }

  void STDMETHODCALLTYPE DrawIndexed (
    _In_  UINT IndexCount,
    _In_  UINT StartIndexLocation,
    _In_  INT  BaseVertexLocation ) override
  {
    SK_LOG_FIRST_CALL

#ifndef SK_D3D11_LAZY_WRAP
  if (! SK_D3D11_IgnoreWrappedOrDeferred (true, pReal))
        SK_D3D11_DrawIndexed_Impl (pReal,
                                       IndexCount,
                                  StartIndexLocation,
                                  BaseVertexLocation, true,
                                    dev_ctx_handle_
        );
    else
#endif
      pReal->DrawIndexed (
                 IndexCount,
            StartIndexLocation,
            BaseVertexLocation
      );
  }

  void STDMETHODCALLTYPE Draw (
    _In_  UINT VertexCount,
    _In_  UINT StartVertexLocation ) override
  {
    SK_LOG_FIRST_CALL

#ifndef SK_D3D11_LAZY_WRAP
  if (! SK_D3D11_IgnoreWrappedOrDeferred (true, pReal))
        SK_D3D11_Draw_Impl (       pReal,
                               VertexCount,
                          StartVertexLocation, true,
                       dev_ctx_handle_
        );
    else
#endif
      pReal->Draw ( VertexCount,
               StartVertexLocation );
  }

  HRESULT STDMETHODCALLTYPE Map (
    _In_   ID3D11Resource           *pResource,
    _In_   UINT                      Subresource,
    _In_   D3D11_MAP                 MapType,
    _In_   UINT                      MapFlags,
    _Out_  D3D11_MAPPED_SUBRESOURCE *pMappedResource ) override
  {
#ifndef SK_D3D11_LAZY_WRAP
    if (! SK_D3D11_IgnoreWrappedOrDeferred (true, pReal))
      return
        SK_D3D11_Map_Impl         (pReal,
            pResource,
          Subresource,
                 MapType,
                 MapFlags,
                pMappedResource, true
        );
    else
#endif
      return
      pReal->Map (
          pResource,
        Subresource,
               MapType,
               MapFlags,
              pMappedResource );
  }

  void STDMETHODCALLTYPE Unmap (
    _In_ ID3D11Resource *pResource,
    _In_ UINT          Subresource ) override
  {
#ifndef SK_D3D11_LAZY_WRAP
  if (! SK_D3D11_IgnoreWrappedOrDeferred (true, pReal))
        SK_D3D11_Unmap_Impl       (pReal,
            pResource,
          Subresource, true
        );
    else
#endif
      pReal->Unmap (pResource, Subresource);
  }

  void STDMETHODCALLTYPE PSSetConstantBuffers (
    _In_range_     (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)         UINT                        StartSlot,
    _In_range_     (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot) UINT                        NumBuffers,
    _In_reads_opt_ (NumBuffers)                                                       ID3D11Buffer *const *ppConstantBuffers ) override
  {
      pReal->PSSetConstantBuffers ( StartSlot,
                       NumBuffers,
                ppConstantBuffers
      );
  }

  void STDMETHODCALLTYPE IASetInputLayout (
    _In_opt_  ID3D11InputLayout *pInputLayout ) override
  {
    pReal->IASetInputLayout (pInputLayout);
  }

  void STDMETHODCALLTYPE IASetVertexBuffers (
    _In_range_     ( 0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - 1 )           UINT                 StartSlot,
    _In_range_     ( 0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - StartSlot )   UINT                 NumBuffers,
    _In_reads_opt_ (NumBuffers)                                                   ID3D11Buffer *const *ppVertexBuffers,
    _In_reads_opt_ (NumBuffers)                                             const UINT                *pStrides,
    _In_reads_opt_ (NumBuffers)                                             const UINT                *pOffsets ) override
  {
    pReal->IASetVertexBuffers ( StartSlot,
                   NumBuffers,
              ppVertexBuffers,
               pStrides,
               pOffsets
    );
  }

  void STDMETHODCALLTYPE IASetIndexBuffer (
    _In_opt_  ID3D11Buffer *pIndexBuffer,
    _In_      DXGI_FORMAT   Format,
    _In_      UINT          Offset ) override
  {
    pReal->IASetIndexBuffer (
               pIndexBuffer, Format, Offset
    );
  }

  void STDMETHODCALLTYPE DrawIndexedInstanced (
    _In_ UINT IndexCountPerInstance,
    _In_ UINT InstanceCount,
    _In_ UINT StartIndexLocation,
    _In_ INT  BaseVertexLocation,
    _In_ UINT StartInstanceLocation ) override
  {
    SK_LOG_FIRST_CALL

#ifndef SK_D3D11_LAZY_WRAP
      if (! SK_D3D11_IgnoreWrappedOrDeferred (true, pReal))
        SK_D3D11_DrawIndexedInstanced_Impl ( pReal,
                                             IndexCountPerInstance, InstanceCount,
                                             StartIndexLocation,  BaseVertexLocation,
                                             StartInstanceLocation, true, dev_ctx_handle_ );
    else
#endif
      pReal->DrawIndexedInstanced (IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
  }

  void STDMETHODCALLTYPE DrawInstanced (
    _In_ UINT VertexCountPerInstance,
    _In_ UINT InstanceCount,
    _In_ UINT StartVertexLocation,
    _In_ UINT StartInstanceLocation ) override
  {
    SK_LOG_FIRST_CALL

#ifndef SK_D3D11_LAZY_WRAP
      if (! SK_D3D11_IgnoreWrappedOrDeferred (true, pReal))
        SK_D3D11_DrawInstanced_Impl (pReal,
                                        VertexCountPerInstance,
                                                      InstanceCount,
                                   StartVertexLocation,
                                                 StartInstanceLocation,
                                     true, dev_ctx_handle_
        );
    else
#endif
      pReal->DrawInstanced (
        VertexCountPerInstance,
                      InstanceCount,
   StartVertexLocation,
                 StartInstanceLocation
      );
  }

  void STDMETHODCALLTYPE GSSetConstantBuffers (
    _In_range_     ( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )         UINT                        StartSlot,
    _In_range_     ( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot ) UINT                        NumBuffers,
    _In_reads_opt_ (NumBuffers)                                                         ID3D11Buffer *const *ppConstantBuffers ) override
  {
    pReal->GSSetConstantBuffers ( StartSlot,
                     NumBuffers,
              ppConstantBuffers
    );
  }

  void STDMETHODCALLTYPE GSSetShader (
    _In_opt_                          ID3D11GeometryShader        *pGeometryShader,
    _In_reads_opt_(NumClassInstances) ID3D11ClassInstance *const *ppClassInstances,
                                      UINT                       NumClassInstances ) override
  {
#ifndef SK_D3D11_LAZY_WRAP
  if (! SK_D3D11_IgnoreWrappedOrDeferred (true, pReal))
        SK_D3D11_SetShader_Impl   (pReal,
           pGeometryShader, sk_shader_class::Geometry,
                                    ppClassInstances,
                                   NumClassInstances, true,
               dev_ctx_handle_
        );
    else
#endif
      pReal->GSSetShader (pGeometryShader, ppClassInstances, NumClassInstances);
  }

  void STDMETHODCALLTYPE IASetPrimitiveTopology (
    _In_  D3D11_PRIMITIVE_TOPOLOGY Topology ) override
  {
    pReal->IASetPrimitiveTopology (Topology);
  }

  void STDMETHODCALLTYPE VSSetShaderResources (
    _In_range_     ( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )         UINT                                          StartSlot,
    _In_range_     ( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot ) UINT                                          NumViews,
    _In_reads_opt_ (NumViews)                                                      ID3D11ShaderResourceView *const *ppShaderResourceViews ) override
  {
#ifndef SK_D3D11_LAZY_WRAP
  if (! SK_D3D11_IgnoreWrappedOrDeferred (true, pReal))
        SK_D3D11_SetShaderResources_Impl (
           SK_D3D11_ShaderType::Vertex,
                        deferred_,
                        nullptr, pReal,
             StartSlot, NumViews,
           ppShaderResourceViews, dev_ctx_handle_
        );
    else
#endif
      pReal->VSSetShaderResources (StartSlot, NumViews, ppShaderResourceViews);
  }

  void STDMETHODCALLTYPE VSSetSamplers (
    _In_range_     (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)         UINT                       StartSlot,
    _In_range_     (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot) UINT                       NumSamplers,
    _In_reads_opt_ (NumSamplers)                                          ID3D11SamplerState *const *ppSamplers ) override
  {
    pReal->VSSetSamplers ( StartSlot,
             NumSamplers,
              ppSamplers
    );
  }

  void STDMETHODCALLTYPE Begin (_In_ ID3D11Asynchronous *pAsync) override
  {
    pReal->Begin (pAsync);
  }

  void STDMETHODCALLTYPE End (_In_  ID3D11Asynchronous *pAsync) override
  {
    pReal->End (pAsync);
  }

  HRESULT STDMETHODCALLTYPE GetData (
    _In_                                ID3D11Asynchronous *pAsync,
    _Out_writes_bytes_opt_( DataSize )  void               *pData,
    _In_                                UINT                 DataSize,
    _In_                                UINT              GetDataFlags ) override
  {
    return
      pReal->GetData ( pAsync,
               pData,
                DataSize,
             GetDataFlags
      );
  };

  void STDMETHODCALLTYPE SetPredication (
    _In_opt_ ID3D11Predicate *pPredicate,
    _In_     BOOL              PredicateValue ) override
  {
    pReal->SetPredication (
             pPredicate,
              PredicateValue
    );
  }

  void STDMETHODCALLTYPE GSSetShaderResources (
    _In_range_     ( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )         UINT                                          StartSlot,
    _In_range_     ( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot ) UINT                                          NumViews,
    _In_reads_opt_ (NumViews)                                                      ID3D11ShaderResourceView *const *ppShaderResourceViews ) override
  {
#ifndef SK_D3D11_LAZY_WRAP
  if (! SK_D3D11_IgnoreWrappedOrDeferred (true, pReal))
        SK_D3D11_SetShaderResources_Impl (
           SK_D3D11_ShaderType::Geometry,
                               deferred_,
                          nullptr, pReal,
             StartSlot, NumViews,
           ppShaderResourceViews, dev_ctx_handle_
        );
    else
#endif
      pReal->GSSetShaderResources (StartSlot, NumViews, ppShaderResourceViews);
  }

  void STDMETHODCALLTYPE GSSetSamplers (
    _In_range_     (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)         UINT                       StartSlot,
    _In_range_     (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot) UINT                       NumSamplers,
    _In_reads_opt_ (NumSamplers)                                          ID3D11SamplerState *const *ppSamplers ) override
  {
    pReal->GSSetSamplers ( StartSlot,
             NumSamplers,
              ppSamplers
    );
  }

  void STDMETHODCALLTYPE OMSetRenderTargets (
    _In_range_     (0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT) UINT                           NumViews,
    _In_reads_opt_ (NumViews)                                  ID3D11RenderTargetView *const *ppRenderTargetViews,
    _In_opt_                                                   ID3D11DepthStencilView        *pDepthStencilView ) override
  {
#ifndef SK_D3D11_LAZY_WRAP
  if (! SK_D3D11_IgnoreWrappedOrDeferred (true, pReal))
        SK_D3D11_OMSetRenderTargets_Impl (pReal,
                               NumViews,
                    ppRenderTargetViews,
                     pDepthStencilView, true,
                     dev_ctx_handle_
        );
    else
#endif
      pReal->OMSetRenderTargets (NumViews, ppRenderTargetViews, pDepthStencilView);
  }

  void STDMETHODCALLTYPE OMSetRenderTargetsAndUnorderedAccessViews (
    _In_                                        UINT                              NumRTVs,
    _In_reads_opt_ (NumRTVs)                    ID3D11RenderTargetView    *const *ppRenderTargetViews,
    _In_opt_                                    ID3D11DepthStencilView           *pDepthStencilView,
    _In_range_ (0, D3D11_1_UAV_SLOT_COUNT - 1)  UINT                              UAVStartSlot,
    _In_                                        UINT                              NumUAVs,
    _In_reads_opt_ (NumUAVs)                    ID3D11UnorderedAccessView *const *ppUnorderedAccessViews,
    _In_reads_opt_ (NumUAVs)              const UINT                             *pUAVInitialCounts ) override
  {
#ifndef SK_D3D11_LAZY_WRAP
  if (! SK_D3D11_IgnoreWrappedOrDeferred (true, pReal))
        SK_D3D11_OMSetRenderTargetsAndUnorderedAccessViews_Impl (pReal,
                   NumRTVs,
                    ppRenderTargetViews,
                     pDepthStencilView,  UAVStartSlot, NumUAVs,
                               ppUnorderedAccessViews,   pUAVInitialCounts,
          true, dev_ctx_handle_
        );
    else
#endif
      pReal->OMSetRenderTargetsAndUnorderedAccessViews (
        NumRTVs, ppRenderTargetViews, pDepthStencilView,
          UAVStartSlot, NumUAVs, ppUnorderedAccessViews, pUAVInitialCounts
      );
  }

  void STDMETHODCALLTYPE OMSetBlendState (
    _In_opt_       ID3D11BlendState *pBlendState,
    _In_opt_ const FLOAT             BlendFactor [4],
    _In_           UINT              SampleMask ) override
  {
    pReal->OMSetBlendState (
               pBlendState,
                BlendFactor,
                  SampleMask
    );
  }

  void STDMETHODCALLTYPE OMSetDepthStencilState (
    _In_opt_ ID3D11DepthStencilState *pDepthStencilState,
    _In_     UINT                           StencilRef ) override
  {
    pReal->OMSetDepthStencilState (
               pDepthStencilState,
                     StencilRef
    );
  }

  void STDMETHODCALLTYPE SOSetTargets (
    _In_range_     (0, D3D11_SO_BUFFER_SLOT_COUNT) UINT                 NumBuffers,
    _In_reads_opt_ (NumBuffers)                    ID3D11Buffer *const *ppSOTargets,
    _In_reads_opt_ (NumBuffers)              const UINT                *pOffsets ) override
  {
    pReal->SOSetTargets ( NumBuffers,
            ppSOTargets,
             pOffsets
    );
  }

  void STDMETHODCALLTYPE DrawAuto (void) override
  {
    SK_LOG_FIRST_CALL

#ifndef SK_D3D11_LAZY_WRAP
  if (! SK_D3D11_IgnoreWrappedOrDeferred (true, pReal))
        SK_D3D11_DrawAuto_Impl    (pReal, true, dev_ctx_handle_);
    else
#endif
      pReal->DrawAuto ();
  }

  void STDMETHODCALLTYPE DrawIndexedInstancedIndirect (
    _In_ ID3D11Buffer           *pBufferForArgs,
    _In_ UINT          AlignedByteOffsetForArgs ) override
  {
    SK_LOG_FIRST_CALL

#ifndef SK_D3D11_LAZY_WRAP
  if (! SK_D3D11_IgnoreWrappedOrDeferred (true, pReal))
        SK_D3D11_DrawIndexedInstancedIndirect_Impl (pReal,
                           pBufferForArgs,
                 AlignedByteOffsetForArgs,
                    true, dev_ctx_handle_
        );
    else
#endif
      pReal->DrawIndexedInstancedIndirect (
                  pBufferForArgs,
        AlignedByteOffsetForArgs
      );
  }

  void STDMETHODCALLTYPE DrawInstancedIndirect (
    _In_ ID3D11Buffer *pBufferForArgs,
    _In_ UINT          AlignedByteOffsetForArgs ) override
  {
    SK_LOG_FIRST_CALL

#ifndef SK_D3D11_LAZY_WRAP
  if (! SK_D3D11_IgnoreWrappedOrDeferred (true, pReal))
        SK_D3D11_DrawInstancedIndirect_Impl (pReal,
                    pBufferForArgs,
          AlignedByteOffsetForArgs, true, dev_ctx_handle_
        );
    else
#endif
      pReal->DrawInstancedIndirect ( pBufferForArgs,
                           AlignedByteOffsetForArgs
      );
  }

  void STDMETHODCALLTYPE Dispatch (
    _In_ UINT ThreadGroupCountX,
    _In_ UINT ThreadGroupCountY,
    _In_ UINT ThreadGroupCountZ ) override
  {
    SK_LOG_FIRST_CALL

#ifndef SK_D3D11_LAZY_WRAP
  if (! SK_D3D11_IgnoreWrappedOrDeferred (true, pReal))
        SK_D3D11_Dispatch_Impl ( pReal,
          ThreadGroupCountX,
          ThreadGroupCountY,
          ThreadGroupCountZ, TRUE,
                               dev_ctx_handle_
        );
    else
#endif
      pReal->Dispatch ( ThreadGroupCountX,
                          ThreadGroupCountY,
                            ThreadGroupCountZ );
  }

  void STDMETHODCALLTYPE DispatchIndirect (
    _In_ ID3D11Buffer           *pBufferForArgs,
    _In_ UINT          AlignedByteOffsetForArgs ) override
  {
    SK_LOG_FIRST_CALL

#ifndef SK_D3D11_LAZY_WRAP
  if (! SK_D3D11_IgnoreWrappedOrDeferred (true, pReal))
        SK_D3D11_DispatchIndirect_Impl ( pReal,
                    pBufferForArgs,
          AlignedByteOffsetForArgs, TRUE,
                                      dev_ctx_handle_
        );
    else
#endif
      pReal->DispatchIndirect ( pBufferForArgs,
                      AlignedByteOffsetForArgs );
  }

  void STDMETHODCALLTYPE RSSetState (
    _In_opt_ ID3D11RasterizerState *pRasterizerState ) override
  {
    pReal->RSSetState (
     pRasterizerState
    );
  }

  void STDMETHODCALLTYPE RSSetViewports (
    _In_range_     (0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE) UINT          NumViewports,
    _In_reads_opt_ (NumViewports)                                          const D3D11_VIEWPORT *pViewports ) override
  {
    ///static const bool bVesperia =
    ///  (SK_GetCurrentGameID () == SK_GAME_ID::Tales_of_Vesperia);
    ///
    ///if (bVesperia)
    ///{
    ///  bool
    ///  STDMETHODCALLTYPE
    ///  SK_TVFix_D3D11_RSSetViewports_Callback (
    ///          ID3D11DeviceContext *This,
    ///          UINT                 NumViewports,
    ///    const D3D11_VIEWPORT      *pViewports );
    ///
    ///  if (SK_TVFix_D3D11_RSSetViewports_Callback (pReal, NumViewports, pViewports))
    ///    return;
    ///}

      pReal->RSSetViewports (
               NumViewports,
                 pViewports
      );
  }

  void STDMETHODCALLTYPE RSSetScissorRects (
    _In_range_     (0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE) UINT      NumRects,
    _In_reads_opt_ (NumRects)                                              const D3D11_RECT *pRects ) override
  {
    ///static const bool bVesperia =
    ///  (SK_GetCurrentGameID () == SK_GAME_ID::Tales_of_Vesperia);
    ///
    ///if (bVesperia)
    ///{
    ///  bool
    ///  STDMETHODCALLTYPE
    ///  SK_TVFix_D3D11_RSSetScissorRects_Callback (
    ///          ID3D11DeviceContext *This,
    ///          UINT                 NumRects,
    ///    const D3D11_RECT          *pRects );
    ///
    ///  if (SK_TVFix_D3D11_RSSetScissorRects_Callback (pReal, NumRects, pRects))
    ///    return;
    ///}

    pReal->RSSetScissorRects (NumRects, pRects);
  }

  void STDMETHODCALLTYPE CopySubresourceRegion (
    _In_           ID3D11Resource *pDstResource,
    _In_           UINT            DstSubresource,
    _In_           UINT            DstX,
    _In_           UINT            DstY,
    _In_           UINT            DstZ,
    _In_           ID3D11Resource *pSrcResource,
    _In_           UINT            SrcSubresource,
    _In_opt_ const D3D11_BOX      *pSrcBox ) override
  {
    // UB: If it's happening, pretend we never saw this...
    if (pDstResource == nullptr || pSrcResource == nullptr)
    {
      return;
    }


    ///if (SK_GetCurrentGameID() == SK_GAME_ID::Ys_Eight)
    ///{
    ///  SK_ComQIPtr <ID3D11Texture2D> pTex (pSrcResource);
    ///
    ///  if (pTex)
    ///  {
    ///    D3D11_BOX box = { };
    ///
    ///    if (pSrcBox != nullptr)
    ///        box = *pSrcBox;
    ///
    ///    else
    ///    {
    ///      D3D11_TEXTURE2D_DESC tex_desc = {};
    ///           pTex->GetDesc (&tex_desc);
    ///
    ///      box.left  = 0; box.right  = tex_desc.Width;
    ///      box.top   = 0; box.bottom = tex_desc.Height;
    ///      box.front = 0; box.back   = 1;
    ///    }
    ///
    ///    dll_log.Log ( L"CopySubresourceRegion:  { %s <%lu> [ %lu/%lu, %lu/%lu, %lu/%lu ] } -> { %s <%lu> (%lu,%lu,%lu) }",
    ///                    DescribeResource (pSrcResource).c_str (), SrcSubresource, box.left,box.right, box.top,box.bottom, box.front,box.back,
    ///                    DescribeResource (pDstResource).c_str (), DstSubresource, DstX, DstY, DstZ );
    ///  }
    ///}


    ///if ( (! config.render.dxgi.deferred_isolation)    &&
    ///          pReal->GetType () == D3D11_DEVICE_CONTEXT_DEFERRED )
    ///{
    ///  return
    ///    pReal->CopySubresourceRegion ( pDstResource, DstSubresource,
    ///                                     DstX, DstY, DstZ,
    ///                                       pSrcResource, SrcSubresource,
    ///                                         pSrcBox );
    ///}

    SK_TLS* pTLS =
      SK_TLS_Bottom ();

    static auto& textures =
      SK_D3D11_Textures;

    SK_ComQIPtr <ID3D11Texture2D> pDstTex (pDstResource);

    if (pDstTex != nullptr)
    {
      if (! SK_D3D11_IsTexInjectThread (pTLS))
      {
        //if (SK_GetCurrentGameID () == SK_GAME_ID::PillarsOfEternity2)
        //{
        //  extern          bool SK_POE2_NopSubresourceCopy;
        //  extern volatile LONG SK_POE2_SkippedCopies;
        //
        //  if (SK_POE2_NopSubresourceCopy)
        //  {
        //    D3D11_TEXTURE2D_DESC desc_out = { };
        //      pDstTex->GetDesc (&desc_out);
        //
        //    if (pSrcBox != nullptr)
        //    {
        //      dll_log.Log (L"Copy (%lu-%lu,%lu-%lu : %lu,%lu,%lu : %s : {%p,%p})",
        //        pSrcBox->left, pSrcBox->right, pSrcBox->top, pSrcBox->bottom,
        //          DstX, DstY, DstZ,
        //            SK_D3D11_DescribeUsage (desc_out.Usage),
        //              pSrcResource, pDstResource );
        //    }
        //
        //    else
        //    {
        //      dll_log.Log (L"Copy (%lu,%lu,%lu : %s)",
        //                   DstX, DstY, DstZ,
        //                   SK_D3D11_DescribeUsage (desc_out.Usage) );
        //    }
        //
        //    if (pSrcBox == nullptr || ( pSrcBox->right != 3840 || pSrcBox->bottom != 2160 ))
        //    {
        //      if (desc_out.Usage == D3D11_USAGE_STAGING || pSrcBox == nullptr)
        //      {
        //        InterlockedIncrement (&SK_POE2_SkippedCopies);
        //
        //        return;
        //      }
        //    }
        //  }
        //}

        if (DstSubresource == 0 && SK_D3D11_TextureIsCached (pDstTex))
        {
          SK_LOG0 ( (L"Cached texture was modified (CopySubresourceRegion)... removing from cache! - <%s>",
                   SK_GetCallerName ().c_str ()), L"DX11TexMgr" );
          SK_D3D11_RemoveTexFromCache (pDstTex, true);
        }
      }
    }


    // ImGui gets to pass-through without invoking the hook
    if (! SK_D3D11_ShouldTrackRenderOp (pReal, dev_ctx_handle_))
    {
      return
        pReal->CopySubresourceRegion (
                  pDstResource,
                   DstSubresource,
                   DstX, DstY, DstZ,
                  pSrcResource,
                   SrcSubresource,
                  pSrcBox
        );
    }


    D3D11_RESOURCE_DIMENSION res_dim = { };
    pSrcResource->GetType  (&res_dim);


    if (SK_D3D11_EnableMMIOTracking)
    {
      SK_D3D11_MemoryThreads->mark ();

      switch (res_dim)
      {
        case D3D11_RESOURCE_DIMENSION_UNKNOWN:
          mem_map_stats->last_frame.resource_types [D3D11_RESOURCE_DIMENSION_UNKNOWN]++;
          break;
        case D3D11_RESOURCE_DIMENSION_BUFFER:
        {
          mem_map_stats->last_frame.resource_types [D3D11_RESOURCE_DIMENSION_BUFFER]++;

          ID3D11Buffer* pBuffer = nullptr;

          if (SUCCEEDED (pSrcResource->QueryInterface <ID3D11Buffer> (&pBuffer)))
          {
            D3D11_BUFFER_DESC  buf_desc = { };
            pBuffer->GetDesc (&buf_desc);
            {
              ////std::scoped_lock <SK_Thread_CriticalSection> auto_lock (cs_mmio);

              if (buf_desc.BindFlags & D3D11_BIND_INDEX_BUFFER)
                mem_map_stats->last_frame.buffer_types [0]++;
              //mem_map_stats->last_frame.index_buffers.insert (pBuffer);

              if (buf_desc.BindFlags & D3D11_BIND_VERTEX_BUFFER)
                mem_map_stats->last_frame.buffer_types [1]++;
              //mem_map_stats->last_frame.vertex_buffers.insert (pBuffer);

              if (buf_desc.BindFlags & D3D11_BIND_CONSTANT_BUFFER)
                mem_map_stats->last_frame.buffer_types [2]++;
              //mem_map_stats->last_frame.constant_buffers.insert (pBuffer);
            }

            mem_map_stats->last_frame.bytes_copied += buf_desc.ByteWidth;

            pBuffer->Release ();
          }
        } break;
        case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
          mem_map_stats->last_frame.resource_types [D3D11_RESOURCE_DIMENSION_TEXTURE1D]++;
          break;
        case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
          mem_map_stats->last_frame.resource_types [D3D11_RESOURCE_DIMENSION_TEXTURE2D]++;
          break;
        case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
          mem_map_stats->last_frame.resource_types [D3D11_RESOURCE_DIMENSION_TEXTURE3D]++;
          break;
      }
    }


    pReal->CopySubresourceRegion ( pDstResource, DstSubresource, DstX, DstY, DstZ,
                                   pSrcResource, SrcSubresource, pSrcBox );

    if ( ( SK_D3D11_IsStagingCacheable (res_dim, pSrcResource) ||
           SK_D3D11_IsStagingCacheable (res_dim, pDstResource) ) && SrcSubresource == 0 && DstSubresource == 0)
    {
      auto& map_ctx = (*mapped_resources)[pReal];

      if (pDstTex != nullptr && map_ctx.dynamic_textures.count (pSrcResource))
      {
        const uint32_t top_crc32 = map_ctx.dynamic_texturesx [pSrcResource];
        const uint32_t checksum  = map_ctx.dynamic_textures  [pSrcResource];

        D3D11_TEXTURE2D_DESC dst_desc = { };
        pDstTex->GetDesc (&dst_desc);

        const uint32_t cache_tag =
          safe_crc32c (top_crc32, (uint8_t *)(&dst_desc), sizeof (D3D11_TEXTURE2D_DESC));

        if (checksum != 0x00 && dst_desc.Usage != D3D11_USAGE_STAGING)
        {
          textures->refTexture2D ( pDstTex,
                                   &dst_desc,
                                   cache_tag,
                                   map_ctx.dynamic_sizes2 [checksum],
                                   map_ctx.dynamic_times2 [checksum],
                                   top_crc32,
                                   L"", nullptr, (HMODULE)(intptr_t)-1/*SK_GetCallingDLL ()*/,
                                   pTLS );

          map_ctx.dynamic_textures.erase  (pSrcResource);
          map_ctx.dynamic_texturesx.erase (pSrcResource);

          map_ctx.dynamic_sizes2.erase    (checksum);
          map_ctx.dynamic_times2.erase    (checksum);
        }
      }
    }
  }

  void STDMETHODCALLTYPE CopyResource (
    _In_ ID3D11Resource *pDstResource,
    _In_ ID3D11Resource *pSrcResource ) override
  {
#ifndef SK_D3D11_LAZY_WRAP
  if (! SK_D3D11_IgnoreWrappedOrDeferred (true, pReal))
        SK_D3D11_CopyResource_Impl (pReal,
                 pDstResource,
                 pSrcResource, true
        );
    else
#endif
      pReal->CopyResource (pDstResource, pSrcResource);
  }

  void STDMETHODCALLTYPE UpdateSubresource (
    _In_           ID3D11Resource *pDstResource,
    _In_           UINT            DstSubresource,
    _In_opt_ const D3D11_BOX      *pDstBox,
    _In_     const void           *pSrcData,
    _In_           UINT            SrcRowPitch,
    _In_           UINT            SrcDepthPitch ) override
  {
#ifndef SK_D3D11_LAZY_WRAP
  if (! SK_D3D11_IgnoreWrappedOrDeferred (true, pReal))
        SK_D3D11_UpdateSubresource_Impl (pReal,
                      pDstResource,
                       DstSubresource,
                      pDstBox,
                      pSrcData,
                       SrcRowPitch,
                       SrcDepthPitch, true
        );
    else
#endif
      pReal->UpdateSubresource (pDstResource, DstSubresource, pDstBox, pSrcData, SrcRowPitch, SrcDepthPitch);
  }

  void STDMETHODCALLTYPE CopyStructureCount (
    _In_ ID3D11Buffer              *pDstBuffer,
    _In_ UINT                        DstAlignedByteOffset,
    _In_ ID3D11UnorderedAccessView *pSrcView ) override
  {
    pReal->CopyStructureCount ( pDstBuffer,
                                 DstAlignedByteOffset,
                                pSrcView
    );
  }

  void STDMETHODCALLTYPE ClearRenderTargetView (
    _In_       ID3D11RenderTargetView *pRenderTargetView,
    _In_ const FLOAT                   ColorRGBA [4] ) override
  {
    pReal->ClearRenderTargetView (
               pRenderTargetView, ColorRGBA
    );
  }

  void STDMETHODCALLTYPE ClearUnorderedAccessViewUint (
    _In_       ID3D11UnorderedAccessView *pUnorderedAccessView,
    _In_ const UINT                       Values [4] ) override
  {
    pReal->ClearUnorderedAccessViewUint (
               pUnorderedAccessView, Values
    );
  }

  void STDMETHODCALLTYPE ClearUnorderedAccessViewFloat (
    _In_       ID3D11UnorderedAccessView *pUnorderedAccessView,
    _In_ const FLOAT                      Values [4] ) override
  {
    pReal->ClearUnorderedAccessViewFloat (
               pUnorderedAccessView, Values
    );
  }

  void STDMETHODCALLTYPE ClearDepthStencilView (
    _In_  ID3D11DepthStencilView *pDepthStencilView,
    _In_  UINT                    ClearFlags,
    _In_  FLOAT                   Depth,
    _In_  UINT8                   Stencil ) override
  {
    return
      pReal->ClearDepthStencilView (
                 pDepthStencilView,
             ClearFlags,
                  Depth,
                       Stencil
      );
  }

  void STDMETHODCALLTYPE GenerateMips (
    _In_ ID3D11ShaderResourceView *pShaderResourceView ) override
  {
    pReal->GenerateMips (pShaderResourceView);
  }

  void STDMETHODCALLTYPE SetResourceMinLOD (
    _In_ ID3D11Resource *pResource,
    FLOAT           MinLOD ) override
  {
    pReal->SetResourceMinLOD (pResource, MinLOD);
  }

  FLOAT STDMETHODCALLTYPE GetResourceMinLOD (
    _In_  ID3D11Resource *pResource ) override
  {
    return
      pReal->GetResourceMinLOD (pResource);
  }

  void STDMETHODCALLTYPE ResolveSubresource (
    _In_ ID3D11Resource *pDstResource,
    _In_ UINT            DstSubresource,
    _In_ ID3D11Resource *pSrcResource,
    _In_ UINT            SrcSubresource,
    _In_ DXGI_FORMAT     Format ) override
  {
    SK_LOG_FIRST_CALL

    pReal->ResolveSubresource (pDstResource, DstSubresource, pSrcResource, SrcSubresource, Format);
  }

  void STDMETHODCALLTYPE ExecuteCommandList (
    _In_  ID3D11CommandList *pCommandList,
          BOOL               RestoreContextState ) override
  {
    SK_LOG_FIRST_CALL

    SK_ComPtr <ID3D11DeviceContext>
         pBuildContext (nullptr);
    UINT  size        =        0;


    // Fix for Yakuza0, why the hell is it passing nullptr?!
    if (pCommandList == nullptr)
    {
      pReal->ExecuteCommandList (
        nullptr,
          RestoreContextState
      );

      if (RestoreContextState == FALSE)
      {
        SK_D3D11_ResetContextState
        (
          pReal, dev_ctx_handle_
        );
      }

      return;
    }




    pCommandList->GetPrivateData (
      SKID_D3D11DeviceContextOrigin,
         &size,   &pBuildContext.p
    );

    if (pBuildContext.p != nullptr)
    {
      if (! pBuildContext.IsEqualObject (pReal))
      {
        SK_D3D11_MergeCommandLists (
          pBuildContext,
            pReal
        );
      }

      pBuildContext->SetPrivateData (
        SKID_D3D11DeviceContextOrigin,
            0,      nullptr
      );
    }

    pReal->ExecuteCommandList (
      pCommandList,
          RestoreContextState
    );

    if (RestoreContextState == FALSE)
    {
      SK_D3D11_ResetContextState (
        pReal, dev_ctx_handle_
      );
    }
  }

  void STDMETHODCALLTYPE HSSetShaderResources (
    _In_range_     (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)         UINT                                          StartSlot,
    _In_range_     (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot) UINT                                          NumViews,
    _In_reads_opt_ (NumViews)                                                    ID3D11ShaderResourceView *const *ppShaderResourceViews ) override
  {
#ifndef SK_D3D11_LAZY_WRAP
  if (! SK_D3D11_IgnoreWrappedOrDeferred (true, pReal))
        SK_D3D11_SetShaderResources_Impl (
           SK_D3D11_ShaderType::Hull,
                           deferred_,
                      nullptr, pReal,
             StartSlot, NumViews,
           ppShaderResourceViews, dev_ctx_handle_
        );
    else
#endif
      pReal->HSSetShaderResources (StartSlot, NumViews, ppShaderResourceViews);
  }

  void STDMETHODCALLTYPE HSSetShader (
    _In_opt_                           ID3D11HullShader            *pHullShader,
    _In_reads_opt_ (NumClassInstances) ID3D11ClassInstance *const *ppClassInstances,
                                       UINT                       NumClassInstances ) override
  {
#ifndef SK_D3D11_LAZY_WRAP
  if (! SK_D3D11_IgnoreWrappedOrDeferred (true, pReal))
        SK_D3D11_SetShader_Impl   (pReal,
               pHullShader, sk_shader_class::Hull,
                                    ppClassInstances,
                                   NumClassInstances, true,
               dev_ctx_handle_
        );
    else
#endif
      pReal->HSSetShader (pHullShader, ppClassInstances, NumClassInstances);
  }

  void STDMETHODCALLTYPE HSSetSamplers (
    _In_range_ (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)         UINT                      StartSlot,
    _In_range_ (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot) UINT                      NumSamplers,
    _In_reads_opt_ (NumSamplers)                                      ID3D11SamplerState *const *ppSamplers ) override
  {
    return
      pReal->HSSetSamplers ( StartSlot,
               NumSamplers,
                ppSamplers
      );
  }

  void STDMETHODCALLTYPE HSSetConstantBuffers (
    _In_range_     (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)         UINT                        StartSlot,
    _In_range_     (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot) UINT                        NumBuffers,
    _In_reads_opt_ (NumBuffers)                                                       ID3D11Buffer *const *ppConstantBuffers ) override
  {
    pReal->HSSetConstantBuffers ( StartSlot,
                     NumBuffers,
              ppConstantBuffers
    );
  }

  void STDMETHODCALLTYPE DSSetShaderResources (
    _In_range_     (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)         UINT                                          StartSlot,
    _In_range_     (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot) UINT                                          NumViews,
    _In_reads_opt_ (NumViews)                                                    ID3D11ShaderResourceView *const *ppShaderResourceViews ) override
  {
#ifndef SK_D3D11_LAZY_WRAP
  if (! SK_D3D11_IgnoreWrappedOrDeferred (true, pReal))
        SK_D3D11_SetShaderResources_Impl (
           SK_D3D11_ShaderType::Domain,
                             deferred_,
                        nullptr, pReal,
             StartSlot, NumViews,
           ppShaderResourceViews, dev_ctx_handle_
        );
    else
#endif
      pReal->DSSetShaderResources (StartSlot, NumViews, ppShaderResourceViews);
  }

  void STDMETHODCALLTYPE DSSetShader (
    _In_opt_                           ID3D11DomainShader          *pDomainShader,
    _In_reads_opt_ (NumClassInstances) ID3D11ClassInstance *const *ppClassInstances,
                                       UINT                       NumClassInstances ) override
  {
#ifndef SK_D3D11_LAZY_WRAP
  if (! SK_D3D11_IgnoreWrappedOrDeferred (true, pReal))
        SK_D3D11_SetShader_Impl   (pReal,
             pDomainShader, sk_shader_class::Domain,
                                    ppClassInstances,
                                   NumClassInstances, true,
               dev_ctx_handle_
        );
    else
#endif
      pReal->DSSetShader (pDomainShader, ppClassInstances, NumClassInstances);
  }

  void STDMETHODCALLTYPE DSSetSamplers (
    _In_range_ (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)         UINT                      StartSlot,
    _In_range_ (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot) UINT                      NumSamplers,
    _In_reads_opt_ (NumSamplers)                                      ID3D11SamplerState *const *ppSamplers ) override
  {
    pReal->DSSetSamplers ( StartSlot,
             NumSamplers,
              ppSamplers
    );
  }

  void STDMETHODCALLTYPE DSSetConstantBuffers (
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)         UINT                 StartSlot,
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot) UINT                 NumBuffers,
    _In_reads_opt_ (NumBuffers)                                                   ID3D11Buffer *const *ppConstantBuffers ) override
  {
    pReal->DSSetConstantBuffers ( StartSlot,
                     NumBuffers,
              ppConstantBuffers
    );
  }

  void STDMETHODCALLTYPE CSSetShaderResources (
    _In_range_     (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)         UINT                                          StartSlot,
    _In_range_     (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot) UINT                                          NumViews,
    _In_reads_opt_ (NumViews)                                                    ID3D11ShaderResourceView *const *ppShaderResourceViews ) override
  {
#ifndef SK_D3D11_LAZY_WRAP
  if (! SK_D3D11_IgnoreWrappedOrDeferred (true, pReal))
        SK_D3D11_SetShaderResources_Impl (
           SK_D3D11_ShaderType::Compute,
                              deferred_,
                         nullptr, pReal,
             StartSlot, NumViews,
           ppShaderResourceViews, dev_ctx_handle_
        );
    else
#endif
      pReal->CSSetShaderResources (StartSlot, NumViews, ppShaderResourceViews);
  }

  void STDMETHODCALLTYPE CSSetUnorderedAccessViews (
    _In_range_( 0, D3D11_1_UAV_SLOT_COUNT - 1 )         UINT                              StartSlot,
    _In_range_( 0, D3D11_1_UAV_SLOT_COUNT - StartSlot ) UINT                              NumUAVs,
    _In_reads_opt_(NumUAVs)                             ID3D11UnorderedAccessView *const *ppUnorderedAccessViews,
    _In_reads_opt_(NumUAVs)                       const UINT                             *pUAVInitialCounts ) override
  {
    pReal->CSSetUnorderedAccessViews (
      StartSlot, NumUAVs,
        ppUnorderedAccessViews,
         pUAVInitialCounts
    );
  }

  void STDMETHODCALLTYPE CSSetShader (
    _In_opt_                           ID3D11ComputeShader         *pComputeShader,
    _In_reads_opt_ (NumClassInstances) ID3D11ClassInstance *const *ppClassInstances,
                                       UINT                       NumClassInstances ) override
  {
#ifndef SK_D3D11_LAZY_WRAP
  if (! SK_D3D11_IgnoreWrappedOrDeferred (true, pReal))
        SK_D3D11_SetShader_Impl   (pReal,
             pComputeShader, sk_shader_class::Compute,
                                     ppClassInstances,
                                    NumClassInstances, true,
                                    dev_ctx_handle_
        );
    else
#endif
      pReal->CSSetShader (pComputeShader, ppClassInstances, NumClassInstances);
  }

  void STDMETHODCALLTYPE CSSetSamplers (
    _In_range_     (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)         UINT                      StartSlot,
    _In_range_     (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot) UINT                      NumSamplers,
    _In_reads_opt_ (NumSamplers)                                          ID3D11SamplerState *const *ppSamplers ) override
  {
    pReal->CSSetSamplers ( StartSlot,
             NumSamplers,
              ppSamplers
    );
  }

  void STDMETHODCALLTYPE CSSetConstantBuffers (
    _In_range_     (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)         UINT                        StartSlot,
    _In_range_     (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot) UINT                        NumBuffers,
    _In_reads_opt_ (NumBuffers)                                                       ID3D11Buffer *const *ppConstantBuffers ) override
  {
    pReal->CSSetConstantBuffers ( StartSlot,
                     NumBuffers,
              ppConstantBuffers
    );
  }

  void STDMETHODCALLTYPE VSGetConstantBuffers (
    _In_range_       (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)         UINT             StartSlot,
    _In_range_       (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot) UINT             NumBuffers,
    _Out_writes_opt_ (NumBuffers)                                                       ID3D11Buffer **ppConstantBuffers ) override
  {
    pReal->VSGetConstantBuffers ( StartSlot,
                     NumBuffers,
              ppConstantBuffers
    );
  }

  void STDMETHODCALLTYPE PSGetShaderResources (
    _In_range_       (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)         UINT                         StartSlot,
    _In_range_       (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot) UINT                         NumViews,
    _Out_writes_opt_ (NumViews)                                                    ID3D11ShaderResourceView **ppShaderResourceViews ) override
  {
    pReal->PSGetShaderResources ( StartSlot,
                           NumViews,
              ppShaderResourceViews
    );
  }

  void STDMETHODCALLTYPE PSGetShader (
    _Out_                                  ID3D11PixelShader   **ppPixelShader,
    _Out_writes_opt_ (*pNumClassInstances) ID3D11ClassInstance **ppClassInstances,
    _Inout_opt_                            UINT                 *pNumClassInstances ) override
  {
    return
      pReal->PSGetShader (
           ppPixelShader,
           ppClassInstances,
         pNumClassInstances
      );
  }

  void STDMETHODCALLTYPE PSGetSamplers (
    _In_range_ (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)         UINT                StartSlot,
    _In_range_ (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot) UINT                NumSamplers,
    _Out_writes_opt_ (NumSamplers)                                    ID3D11SamplerState **ppSamplers ) override
  {
    return
      pReal->PSGetSamplers ( StartSlot,
               NumSamplers,
                ppSamplers
      );
  }

  void STDMETHODCALLTYPE VSGetShader (
    _Out_                                  ID3D11VertexShader  **ppVertexShader,
    _Out_writes_opt_ (*pNumClassInstances) ID3D11ClassInstance **ppClassInstances,
    _Inout_opt_                            UINT               *pNumClassInstances ) override
  {
    return
      pReal->VSGetShader (
          ppVertexShader,
          ppClassInstances,
        pNumClassInstances
      );
  }

  void STDMETHODCALLTYPE PSGetConstantBuffers (
    _In_range_       (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)          UINT                  StartSlot,
    _In_range_       (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT                  NumBuffers,
    _Out_writes_opt_ (NumBuffers)                                                        ID3D11Buffer **ppConstantBuffers ) override
  {
    pReal->PSGetConstantBuffers ( StartSlot,
                     NumBuffers,
              ppConstantBuffers
    );
  }

  void STDMETHODCALLTYPE IAGetInputLayout (
    _Out_  ID3D11InputLayout **ppInputLayout ) override
  {
    pReal->IAGetInputLayout (ppInputLayout);
  }

  void STDMETHODCALLTYPE IAGetVertexBuffers (
    _In_range_       (0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - 1)          UINT                StartSlot,
    _In_range_       (0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT                NumBuffers,
    _Out_writes_opt_ (NumBuffers)                                                ID3D11Buffer **ppVertexBuffers,
    _Out_writes_opt_ (NumBuffers)                                                UINT          *pStrides,
    _Out_writes_opt_ (NumBuffers)                                                UINT          *pOffsets ) override
  {
    return
      pReal->IAGetVertexBuffers ( StartSlot,
                     NumBuffers,
                ppVertexBuffers,
                 pStrides,
                 pOffsets
      );
  }

  void STDMETHODCALLTYPE IAGetIndexBuffer (
    _Out_opt_  ID3D11Buffer **pIndexBuffer,
    _Out_opt_  DXGI_FORMAT   *Format,
    _Out_opt_  UINT          *Offset ) override
  {
    return
      pReal->IAGetIndexBuffer (
                 pIndexBuffer, Format, Offset
      );
  }

  void STDMETHODCALLTYPE GSGetConstantBuffers(
    _In_range_       ( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )         UINT                  StartSlot,
    _In_range_       ( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot ) UINT                  NumBuffers,
    _Out_writes_opt_ (NumBuffers)                                                         ID3D11Buffer **ppConstantBuffers ) override
  {
    return
      pReal->GSGetConstantBuffers ( StartSlot,
                       NumBuffers,
                ppConstantBuffers
      );
  }

  void STDMETHODCALLTYPE GSGetShader (
    _Out_                                  ID3D11GeometryShader **ppGeometryShader,
    _Out_writes_opt_ (*pNumClassInstances) ID3D11ClassInstance  **ppClassInstances,
    _Inout_opt_                            UINT                  *pNumClassInstances ) override
  {
    return
      pReal->GSGetShader (
        ppGeometryShader,
        ppClassInstances,
      pNumClassInstances );
  }

  void STDMETHODCALLTYPE IAGetPrimitiveTopology (
    _Out_  D3D11_PRIMITIVE_TOPOLOGY *pTopology ) override
  {
    return
      pReal->IAGetPrimitiveTopology (pTopology);
  }

  void STDMETHODCALLTYPE VSGetShaderResources (
    _In_range_       (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)         UINT                         StartSlot,
    _In_range_       (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot) UINT                         NumViews,
    _Out_writes_opt_ (NumViews)                                                    ID3D11ShaderResourceView **ppShaderResourceViews ) override
  {
    return
      pReal->VSGetShaderResources ( StartSlot,
                             NumViews,
                ppShaderResourceViews
      );
  }

  void STDMETHODCALLTYPE VSGetSamplers(
    _In_range_       ( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )         UINT                   StartSlot,
    _In_range_       ( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot ) UINT                   NumSamplers,
    _Out_writes_opt_ ( NumSamplers )                                          ID3D11SamplerState **ppSamplers ) override
  {
    return
      pReal->VSGetSamplers ( StartSlot,
               NumSamplers,
                ppSamplers
      );
  }

  void STDMETHODCALLTYPE GetPredication (
    _Out_opt_ ID3D11Predicate **ppPredicate,
    _Out_opt_ BOOL             *pPredicateValue ) override
  {
    return
      pReal->GetPredication (
              ppPredicate,
               pPredicateValue
      );
  }

  void STDMETHODCALLTYPE GSGetShaderResources (
    _In_range_       ( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
    _In_range_       ( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
    _Out_writes_opt_ (NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews) override
  {
    return
      pReal->GSGetShaderResources ( StartSlot,
                             NumViews,
                ppShaderResourceViews
      );
  }

  void STDMETHODCALLTYPE GSGetSamplers (
    _In_range_       (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)         UINT                   StartSlot,
    _In_range_       (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot) UINT                   NumSamplers,
    _Out_writes_opt_ (NumSamplers)                                          ID3D11SamplerState **ppSamplers ) override
  {
    return
      pReal->GSGetSamplers ( StartSlot,
               NumSamplers,
                ppSamplers
      );
  }

  void STDMETHODCALLTYPE OMGetRenderTargets (
    _In_range_       (0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT) UINT                       NumViews,
    _Out_writes_opt_ (NumViews)                                  ID3D11RenderTargetView **ppRenderTargetViews,
    _Out_opt_                                                    ID3D11DepthStencilView **ppDepthStencilView ) override
  {
    pReal->OMGetRenderTargets (
                         NumViews,
              ppRenderTargetViews,
              ppDepthStencilView
    );
  }

  void STDMETHODCALLTYPE OMGetRenderTargetsAndUnorderedAccessViews (
    _In_range_       (0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT)        UINT                          NumRTVs,
    _Out_writes_opt_ (NumRTVs)                                          ID3D11RenderTargetView    **ppRenderTargetViews,
    _Out_opt_                                                           ID3D11DepthStencilView    **ppDepthStencilView,
    _In_range_       (0, D3D11_PS_CS_UAV_REGISTER_COUNT - 1)            UINT                          UAVStartSlot,
    _In_range_       (0, D3D11_PS_CS_UAV_REGISTER_COUNT - UAVStartSlot) UINT                          NumUAVs,
    _Out_writes_opt_ (NumUAVs)                                          ID3D11UnorderedAccessView **ppUnorderedAccessViews ) override
  {
    pReal->OMGetRenderTargetsAndUnorderedAccessViews ( NumRTVs,
              ppRenderTargetViews,
              ppDepthStencilView,
                UAVStartSlot,
             NumUAVs,
              ppUnorderedAccessViews
    );
  }

  void STDMETHODCALLTYPE OMGetBlendState (
    _Out_opt_ ID3D11BlendState **ppBlendState,
    _Out_opt_ FLOAT              BlendFactor [4],
    _Out_opt_ UINT              *pSampleMask ) override
  {
    return
      pReal->OMGetBlendState (
        ppBlendState,
          BlendFactor,
            pSampleMask
      );
  }

  void STDMETHODCALLTYPE OMGetDepthStencilState (
    _Out_opt_ ID3D11DepthStencilState **ppDepthStencilState,
    _Out_opt_ UINT                     *pStencilRef ) override
  {
    return
      pReal->OMGetDepthStencilState (
        ppDepthStencilState,
              pStencilRef
      );
  }

  void STDMETHODCALLTYPE SOGetTargets(
    _In_range_       ( 0, D3D11_SO_BUFFER_SLOT_COUNT ) UINT             NumBuffers,
    _Out_writes_opt_ (NumBuffers)                      ID3D11Buffer **ppSOTargets ) override
  {
    return
      pReal->SOGetTargets (
        NumBuffers,
           ppSOTargets
      );
  }

  void STDMETHODCALLTYPE RSGetState (
    _Out_ ID3D11RasterizerState **ppRasterizerState ) override
  {
    return
      pReal->RSGetState (ppRasterizerState);
  }

  void STDMETHODCALLTYPE RSGetViewports(
    _Inout_ /*_range(0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE )*/   UINT *pNumViewports,
    _Out_writes_opt_(*pNumViewports)  D3D11_VIEWPORT *pViewports) override
  {
    return
      pReal->RSGetViewports (pNumViewports, pViewports);
  }

  void STDMETHODCALLTYPE RSGetScissorRects (
    _Inout_ /*_range(0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE )*/   UINT *pNumRects,
    _Out_writes_opt_ (*pNumRects)  D3D11_RECT *pRects) override
  {
    return
      pReal->RSGetScissorRects (pNumRects, pRects);
  }

  void STDMETHODCALLTYPE HSGetShaderResources(
    _In_range_       ( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )         UINT                         StartSlot,
    _In_range_       ( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot ) UINT                         NumViews,
    _Out_writes_opt_ (NumViews)                                                      ID3D11ShaderResourceView **ppShaderResourceViews ) override
  {
    return
      pReal->HSGetShaderResources ( StartSlot,
                             NumViews,
                ppShaderResourceViews
      );
  }

  void STDMETHODCALLTYPE HSGetShader (
    _Out_                                  ID3D11HullShader    **ppHullShader,
    _Out_writes_opt_ (*pNumClassInstances) ID3D11ClassInstance **ppClassInstances,
    _Inout_opt_                            UINT               *pNumClassInstances ) override
  {
    return
      pReal->HSGetShader (
           ppHullShader,
           ppClassInstances,
         pNumClassInstances
      );
  }

  void STDMETHODCALLTYPE HSGetSamplers (
    _In_range_       (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)         UINT                   StartSlot,
    _In_range_       (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot) UINT                   NumSamplers,
    _Out_writes_opt_ (NumSamplers)                                          ID3D11SamplerState **ppSamplers ) override
  {
    return
      pReal->HSGetSamplers ( StartSlot,
               NumSamplers,
                ppSamplers
      );
  }

  void STDMETHODCALLTYPE HSGetConstantBuffers (
    _In_range_       (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)         UINT             StartSlot,
    _In_range_       (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot) UINT             NumBuffers,
    _Out_writes_opt_ (NumBuffers)                                                       ID3D11Buffer **ppConstantBuffers ) override
  {
    return
      pReal->HSGetConstantBuffers ( StartSlot,
                       NumBuffers,
                ppConstantBuffers
      );
  }

  void STDMETHODCALLTYPE DSGetShaderResources (
    _In_range_       (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)         UINT                         StartSlot,
    _In_range_       (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot) UINT                         NumViews,
    _Out_writes_opt_ (NumViews)                                                    ID3D11ShaderResourceView **ppShaderResourceViews ) override
  {
    return
      pReal->DSGetShaderResources ( StartSlot,
                             NumViews,
                ppShaderResourceViews
      );
  }

  void STDMETHODCALLTYPE DSGetShader (
    _Out_                                  ID3D11DomainShader  **ppDomainShader,
    _Out_writes_opt_ (*pNumClassInstances) ID3D11ClassInstance **ppClassInstances,
    _Inout_opt_                            UINT               *pNumClassInstances ) override
  {
    return
      pReal->DSGetShader (
          ppDomainShader,
          ppClassInstances,
        pNumClassInstances
      );
  }

  void STDMETHODCALLTYPE DSGetSamplers (
    _In_range_       (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)         UINT                   StartSlot,
    _In_range_       (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot) UINT                   NumSamplers,
    _Out_writes_opt_ (NumSamplers)                                          ID3D11SamplerState **ppSamplers ) override
  {
    return
      pReal->DSGetSamplers ( StartSlot,
               NumSamplers,
                ppSamplers
      );
  }

  void STDMETHODCALLTYPE DSGetConstantBuffers (
    _In_range_       (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)         UINT             StartSlot,
    _In_range_       (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot) UINT             NumBuffers,
    _Out_writes_opt_ (NumBuffers)                                                       ID3D11Buffer **ppConstantBuffers ) override
  {
    return
      pReal->DSGetConstantBuffers ( StartSlot,
                       NumBuffers,
                ppConstantBuffers
      );
  }

  void STDMETHODCALLTYPE CSGetShaderResources (
    _In_range_       (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)         UINT                         StartSlot,
    _In_range_       (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot) UINT                         NumViews,
    _Out_writes_opt_ (NumViews)                                                    ID3D11ShaderResourceView **ppShaderResourceViews ) override
  {
    return
      pReal->CSGetShaderResources ( StartSlot,
                             NumViews,
                ppShaderResourceViews
      );
  }

  void STDMETHODCALLTYPE CSGetUnorderedAccessViews (
    _In_range_       (0, D3D11_PS_CS_UAV_REGISTER_COUNT - 1)         UINT                          StartSlot,
    _In_range_       (0, D3D11_PS_CS_UAV_REGISTER_COUNT - StartSlot) UINT                          NumUAVs,
    _Out_writes_opt_ (NumUAVs)                                       ID3D11UnorderedAccessView **ppUnorderedAccessViews ) override
  {
    return
      pReal->CSGetUnorderedAccessViews ( StartSlot,
               NumUAVs,
                ppUnorderedAccessViews
      );
  }

  void STDMETHODCALLTYPE CSGetShader (
    _Out_                                  ID3D11ComputeShader **ppComputeShader,
    _Out_writes_opt_ (*pNumClassInstances) ID3D11ClassInstance **ppClassInstances,
    _Inout_opt_                            UINT               *pNumClassInstances ) override
  {
    return
      pReal->CSGetShader (
          ppComputeShader,
          ppClassInstances,
        pNumClassInstances
      );
  }

  void STDMETHODCALLTYPE CSGetSamplers (
    _In_range_       (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)         UINT                StartSlot,
    _In_range_       (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot) UINT                NumSamplers,
    _Out_writes_opt_ (NumSamplers)                                          ID3D11SamplerState **ppSamplers ) override
  {
    return
      pReal->CSGetSamplers ( StartSlot,
               NumSamplers,
                ppSamplers
      );
  }

  void STDMETHODCALLTYPE CSGetConstantBuffers (
    _In_range_       (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)         UINT             StartSlot,
    _In_range_       (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot) UINT             NumBuffers,
    _Out_writes_opt_ (NumBuffers)                                                       ID3D11Buffer **ppConstantBuffers ) override
  {
    return
      pReal->CSGetConstantBuffers ( StartSlot,
                       NumBuffers,
                ppConstantBuffers
      );
  }

  void STDMETHODCALLTYPE ClearState (void) override
  {
    SK_LOG_FIRST_CALL

    bool SK_D3D11_QueueContextReset (ID3D11DeviceContext* pDevCtx, UINT dev_ctx);
    bool SK_D3D11_DispatchContextResetQueue (UINT dev_ctx);

  //SK_D3D11_QueueContextReset  (pReal, dev_ctx_handle_);
    pReal->ClearState           (                      );
  //SK_D3D11_DispatchContextResetQueue (dev_ctx_handle_);
  }

  void STDMETHODCALLTYPE Flush (void) override
  {
    SK_LOG_FIRST_CALL

    return
      pReal->Flush ();
  }

  D3D11_DEVICE_CONTEXT_TYPE STDMETHODCALLTYPE GetType (void) override
  {
    return
      pReal->GetType ();
  }

  UINT STDMETHODCALLTYPE GetContextFlags (void) override
  {
    return
      pReal->GetContextFlags ();
  }

  HRESULT STDMETHODCALLTYPE FinishCommandList (
              BOOL                RestoreDeferredContextState,
    _Out_opt_ ID3D11CommandList **ppCommandList ) override
  {
    SK_LOG_FIRST_CALL

    HRESULT hr =
      pReal->FinishCommandList (RestoreDeferredContextState, ppCommandList);

    if (SUCCEEDED (hr) && (ppCommandList               != nullptr &&
                          (RestoreDeferredContextState == FALSE)))
    {
      (*ppCommandList)->SetPrivateData ( SKID_D3D11DeviceContextOrigin,
                                           sizeof (ptrdiff_t), pReal );
      SK_D3D11_ResetContextState (pReal, dev_ctx_handle_);
    }
    else
      SK_D3D11_ResetContextState (pReal, dev_ctx_handle_);

    return hr;
  }




  void STDMETHODCALLTYPE CopySubresourceRegion1 (
    _In_            ID3D11Resource *pDstResource,
    _In_            UINT            DstSubresource,
    _In_            UINT            DstX,
    _In_            UINT            DstY,
    _In_            UINT            DstZ,
    _In_            ID3D11Resource *pSrcResource,
    _In_            UINT            SrcSubresource,
    _In_opt_  const D3D11_BOX      *pSrcBox,
    _In_            UINT            CopyFlags ) override
  {
    assert (ver_ >= 1);

    return
      static_cast <ID3D11DeviceContext1 *>(pReal)->CopySubresourceRegion1 (pDstResource, DstSubresource, DstX, DstY, DstZ, pSrcResource, SrcSubresource, pSrcBox, CopyFlags);
  }
  void STDMETHODCALLTYPE UpdateSubresource1 (
    _In_           ID3D11Resource *pDstResource,
    _In_           UINT            DstSubresource,
    _In_opt_ const D3D11_BOX      *pDstBox,
    _In_     const void           *pSrcData,
    _In_           UINT            SrcRowPitch,
    _In_           UINT            SrcDepthPitch,
    _In_           UINT            CopyFlags ) override
  {
    assert (ver_ >= 1);

    return
      static_cast <ID3D11DeviceContext1 *>(pReal)->UpdateSubresource1 (pDstResource, DstSubresource, pDstBox, pSrcData, SrcRowPitch, SrcDepthPitch, CopyFlags);
  }
  void STDMETHODCALLTYPE DiscardResource (
    _In_  ID3D11Resource *pResource ) override
  {
    assert (ver_ >= 1);

    return
      static_cast <ID3D11DeviceContext1 *>(pReal)->DiscardResource (pResource);
  }
  void STDMETHODCALLTYPE DiscardView (
    _In_ ID3D11View *pResourceView ) override
  {
    assert (ver_ >= 1);

    return
      static_cast <ID3D11DeviceContext1 *>(pReal)->DiscardView (pResourceView);
  }
  void STDMETHODCALLTYPE VSSetConstantBuffers1 (
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)         UINT                 StartSlot,
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot) UINT                 NumBuffers,
    _In_reads_opt_ (NumBuffers)                                                   ID3D11Buffer *const *ppConstantBuffers,
    _In_reads_opt_ (NumBuffers)                                             const UINT                *pFirstConstant,
    _In_reads_opt_ (NumBuffers)                                             const UINT                *pNumConstants ) override
  {
    assert (ver_ >= 1);

    return
      static_cast <ID3D11DeviceContext1 *>(pReal)->VSSetConstantBuffers1 (StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants);
  }
  void STDMETHODCALLTYPE HSSetConstantBuffers1 (
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)         UINT                 StartSlot,
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot) UINT                 NumBuffers,
    _In_reads_opt_ (NumBuffers)                                                   ID3D11Buffer *const *ppConstantBuffers,
    _In_reads_opt_ (NumBuffers)                                             const UINT                *pFirstConstant,
    _In_reads_opt_ (NumBuffers)                                             const UINT                *pNumConstants ) override
  {
    assert (ver_ >= 1);

    return
      static_cast <ID3D11DeviceContext1 *>(pReal)->HSSetConstantBuffers1 (StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants);
  }
  void STDMETHODCALLTYPE DSSetConstantBuffers1 (
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)         UINT                 StartSlot,
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot) UINT                 NumBuffers,
    _In_reads_opt_ (NumBuffers)                                                   ID3D11Buffer *const *ppConstantBuffers,
    _In_reads_opt_ (NumBuffers)                                             const UINT                *pFirstConstant,
    _In_reads_opt_ (NumBuffers)                                             const UINT                *pNumConstants ) override
  {
    assert (ver_ >= 1);

    return
      static_cast <ID3D11DeviceContext1 *>(pReal)->DSSetConstantBuffers1 (StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants);
  }

  void STDMETHODCALLTYPE GSSetConstantBuffers1 (
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)         UINT                 StartSlot,
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot) UINT                 NumBuffers,
    _In_reads_opt_ (NumBuffers)                                                   ID3D11Buffer *const *ppConstantBuffers,
    _In_reads_opt_ (NumBuffers)                                             const UINT                *pFirstConstant,
    _In_reads_opt_ (NumBuffers)                                             const UINT                *pNumConstants ) override
  {
    assert (ver_ >= 1);

    return
      static_cast <ID3D11DeviceContext1 *>(pReal)->GSSetConstantBuffers1 (StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants);
  }
  void STDMETHODCALLTYPE PSSetConstantBuffers1 (
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)         UINT                 StartSlot,
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot) UINT                 NumBuffers,
    _In_reads_opt_ (NumBuffers)                                                   ID3D11Buffer *const *ppConstantBuffers,
    _In_reads_opt_ (NumBuffers)                                             const UINT                *pFirstConstant,
    _In_reads_opt_ (NumBuffers)                                             const UINT                *pNumConstants ) override
  {
    assert (ver_ >= 1);

    return
      static_cast <ID3D11DeviceContext1 *>(pReal)->PSSetConstantBuffers1 (StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants);
  }
  void STDMETHODCALLTYPE CSSetConstantBuffers1 (
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)         UINT                 StartSlot,
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot) UINT                 NumBuffers,
    _In_reads_opt_ (NumBuffers)                                                   ID3D11Buffer *const *ppConstantBuffers,
    _In_reads_opt_ (NumBuffers)                                             const UINT                *pFirstConstant,
    _In_reads_opt_ (NumBuffers)                                             const UINT                *pNumConstants ) override
  {
    assert (ver_ >= 1);

    return
      static_cast <ID3D11DeviceContext1 *>(pReal)->CSSetConstantBuffers1 (StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants);
  }
  void STDMETHODCALLTYPE VSGetConstantBuffers1 (
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
    _Out_writes_opt_ (NumBuffers)  ID3D11Buffer **ppConstantBuffers,
    _Out_writes_opt_ (NumBuffers)  UINT *pFirstConstant,
    _Out_writes_opt_ (NumBuffers)  UINT *pNumConstants) override
  {
    assert (ver_ >= 1);

    return
      static_cast <ID3D11DeviceContext1 *>(pReal)->VSGetConstantBuffers1 (StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants);
  }
  void STDMETHODCALLTYPE HSGetConstantBuffers1 (
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
    _Out_writes_opt_ (NumBuffers)  ID3D11Buffer **ppConstantBuffers,
    _Out_writes_opt_ (NumBuffers)  UINT *pFirstConstant,
    _Out_writes_opt_ (NumBuffers)  UINT *pNumConstants) override
  {
    assert (ver_ >= 1);

    return
      static_cast <ID3D11DeviceContext1 *>(pReal)->HSGetConstantBuffers1 (StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants);
  }
  void STDMETHODCALLTYPE DSGetConstantBuffers1(
    _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
    _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
    _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers,
    _Out_writes_opt_(NumBuffers)  UINT *pFirstConstant,
    _Out_writes_opt_(NumBuffers)  UINT *pNumConstants) override
  {
    assert (ver_ >= 1);

    return
      static_cast <ID3D11DeviceContext1 *>(pReal)->DSGetConstantBuffers1 (StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants);
  }
  void STDMETHODCALLTYPE GSGetConstantBuffers1 (
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
    _Out_writes_opt_ (NumBuffers)  ID3D11Buffer **ppConstantBuffers,
    _Out_writes_opt_ (NumBuffers)  UINT *pFirstConstant,
    _Out_writes_opt_ (NumBuffers)  UINT *pNumConstants) override
  {
    assert (ver_ >= 1);

    return
      static_cast <ID3D11DeviceContext1 *>(pReal)->GSGetConstantBuffers1 (StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants);
  }
  void STDMETHODCALLTYPE PSGetConstantBuffers1 (
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
    _Out_writes_opt_ (NumBuffers)  ID3D11Buffer **ppConstantBuffers,
    _Out_writes_opt_ (NumBuffers)  UINT *pFirstConstant,
    _Out_writes_opt_ (NumBuffers)  UINT *pNumConstants) override
  {
    assert (ver_ >= 1);

    return
      static_cast <ID3D11DeviceContext1 *>(pReal)->PSGetConstantBuffers1 (StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants);
  }
  void STDMETHODCALLTYPE CSGetConstantBuffers1 (
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
    _Out_writes_opt_ (NumBuffers)  ID3D11Buffer **ppConstantBuffers,
    _Out_writes_opt_ (NumBuffers)  UINT *pFirstConstant,
    _Out_writes_opt_ (NumBuffers)  UINT *pNumConstants) override
  {
    assert (ver_ >= 1);

    return
      static_cast <ID3D11DeviceContext1 *>(pReal)->CSGetConstantBuffers1 (StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants);
  }
  void STDMETHODCALLTYPE SwapDeviceContextState (
    _In_      ID3DDeviceContextState  *pState,
    _Out_opt_ ID3DDeviceContextState **ppPreviousState ) override
  {
    assert (ver_ >= 1);

    return
      static_cast <ID3D11DeviceContext1 *>(pReal)->SwapDeviceContextState (pState, ppPreviousState);
  }

  void STDMETHODCALLTYPE ClearView (
    _In_                            ID3D11View *pView,
    _In_                      const FLOAT       Color [4],
    _In_reads_opt_ (NumRects) const D3D11_RECT *pRect,
    UINT        NumRects ) override
  {
    assert (ver_ >= 1);

    return
      static_cast <ID3D11DeviceContext1 *>(pReal)->ClearView (pView, Color, pRect, NumRects);
  }
  void STDMETHODCALLTYPE DiscardView1 (
    _In_                            ID3D11View *pResourceView,
    _In_reads_opt_ (NumRects) const D3D11_RECT *pRects,
    UINT        NumRects ) override
  {
    assert (ver_ >= 1);

    return
      static_cast <ID3D11DeviceContext1 *>(pReal)->DiscardView1 (pResourceView, pRects, NumRects);
  }




  HRESULT STDMETHODCALLTYPE UpdateTileMappings (
    _In_  ID3D11Resource *pTiledResource,
    _In_  UINT NumTiledResourceRegions,
    _In_reads_opt_ (NumTiledResourceRegions)
             const D3D11_TILED_RESOURCE_COORDINATE *pTiledResourceRegionStartCoordinates,
    _In_reads_opt_ (NumTiledResourceRegions)
             const D3D11_TILE_REGION_SIZE          *pTiledResourceRegionSizes,
    _In_opt_                          ID3D11Buffer *pTilePool,
    _In_                              UINT        NumRanges,
    _In_reads_opt_ (NumRanges)  const UINT         *pRangeFlags,
    _In_reads_opt_ (NumRanges)  const UINT         *pTilePoolStartOffsets,
    _In_reads_opt_ (NumRanges)  const UINT         *pRangeTileCounts,
    _In_                              UINT           Flags ) override
  {
    assert (ver_ >= 2);

    return
      static_cast <ID3D11DeviceContext2 *>(pReal)->UpdateTileMappings (pTiledResource, NumTiledResourceRegions, pTiledResourceRegionStartCoordinates, pTiledResourceRegionSizes, pTilePool, NumRanges, pRangeFlags, pTilePoolStartOffsets, pRangeTileCounts, Flags);
  }
  HRESULT STDMETHODCALLTYPE CopyTileMappings (
    _In_  ID3D11Resource *pDestTiledResource,
    _In_  const D3D11_TILED_RESOURCE_COORDINATE *pDestRegionStartCoordinate,
    _In_  ID3D11Resource *pSourceTiledResource,
    _In_  const D3D11_TILED_RESOURCE_COORDINATE *pSourceRegionStartCoordinate,
    _In_  const D3D11_TILE_REGION_SIZE *pTileRegionSize,
    _In_  UINT Flags) override
  {
    assert (ver_ >= 2);

    return
      static_cast <ID3D11DeviceContext2 *>(pReal)->CopyTileMappings (pDestTiledResource, pDestRegionStartCoordinate, pSourceTiledResource, pSourceRegionStartCoordinate, pTileRegionSize, Flags);
  }
  void STDMETHODCALLTYPE CopyTiles (
    _In_  ID3D11Resource *pTiledResource,
    _In_  const D3D11_TILED_RESOURCE_COORDINATE *pTileRegionStartCoordinate,
    _In_  const D3D11_TILE_REGION_SIZE *pTileRegionSize,
    _In_  ID3D11Buffer *pBuffer,
    _In_  UINT64 BufferStartOffsetInBytes,
    _In_  UINT Flags) override
  {
    assert (ver_ >= 2);

    return
      static_cast <ID3D11DeviceContext2 *>(pReal)->CopyTiles (pTiledResource, pTileRegionStartCoordinate, pTileRegionSize, pBuffer, BufferStartOffsetInBytes, Flags);
  }
  void STDMETHODCALLTYPE UpdateTiles (
    _In_  ID3D11Resource *pDestTiledResource,
    _In_  const D3D11_TILED_RESOURCE_COORDINATE *pDestTileRegionStartCoordinate,
    _In_  const D3D11_TILE_REGION_SIZE *pDestTileRegionSize,
    _In_  const void *pSourceTileData,
    _In_  UINT Flags) override
  {
    assert (ver_ >= 2);

    return
      static_cast <ID3D11DeviceContext2 *>(pReal)->UpdateTiles (pDestTiledResource, pDestTileRegionStartCoordinate, pDestTileRegionSize, pSourceTileData, Flags);
  }
  HRESULT STDMETHODCALLTYPE ResizeTilePool (
    _In_  ID3D11Buffer *pTilePool,
    _In_  UINT64 NewSizeInBytes) override
  {
    assert (ver_ >= 2);

    return
      static_cast <ID3D11DeviceContext2 *>(pReal)->ResizeTilePool (pTilePool, NewSizeInBytes);
  }
  void STDMETHODCALLTYPE TiledResourceBarrier (
    _In_opt_  ID3D11DeviceChild *pTiledResourceOrViewAccessBeforeBarrier,
    _In_opt_  ID3D11DeviceChild *pTiledResourceOrViewAccessAfterBarrier) override
  {
    assert (ver_ >= 2);

    return
      static_cast <ID3D11DeviceContext2 *>(pReal)->TiledResourceBarrier (pTiledResourceOrViewAccessBeforeBarrier, pTiledResourceOrViewAccessAfterBarrier);
  }
  BOOL STDMETHODCALLTYPE IsAnnotationEnabled (void) override
  {
    assert (ver_ >= 2);

    return
      static_cast <ID3D11DeviceContext2 *>(pReal)->IsAnnotationEnabled ();
  }
  void STDMETHODCALLTYPE SetMarkerInt (
    _In_  LPCWSTR pLabel,
    INT     Data) override
  {
    assert (ver_ >= 2);

    return
      static_cast <ID3D11DeviceContext2 *>(pReal)->SetMarkerInt (pLabel, Data);
  }
  void STDMETHODCALLTYPE BeginEventInt (
    _In_  LPCWSTR pLabel,
          INT     Data) override
  {
    assert (ver_ >= 2);

    return
      static_cast <ID3D11DeviceContext2 *>(pReal)->BeginEventInt (pLabel, Data);
  }
  void STDMETHODCALLTYPE EndEvent (void) override
  {
    assert (ver_ >= 2);

    return
      static_cast <ID3D11DeviceContext2 *>(pReal)->EndEvent ();
  }



  void STDMETHODCALLTYPE Flush1 (
    D3D11_CONTEXT_TYPE ContextType,
    _In_opt_  HANDLE             hEvent) override
  {
    assert (ver_ >= 3);

    return
      static_cast <ID3D11DeviceContext3 *>(pReal)->Flush1 (ContextType, hEvent);
  }
  void STDMETHODCALLTYPE SetHardwareProtectionState (
    _In_  BOOL HwProtectionEnable) override
  {
    assert (ver_ >= 3);

    return
      static_cast <ID3D11DeviceContext3 *>(pReal)->SetHardwareProtectionState (HwProtectionEnable);
  }
  void STDMETHODCALLTYPE GetHardwareProtectionState (
    _Out_  BOOL *pHwProtectionEnable) override
  {
    assert (ver_ >= 3);

    return
      static_cast <ID3D11DeviceContext3 *>(pReal)->GetHardwareProtectionState (pHwProtectionEnable);
  }



  HRESULT STDMETHODCALLTYPE Signal (
    _In_  ID3D11Fence *pFence,
    _In_  UINT64       Value) override
  {
    assert (ver_ >= 4);

    return
      static_cast <ID3D11DeviceContext4 *>(pReal)->Signal (pFence, Value);
  }
  HRESULT STDMETHODCALLTYPE Wait (
    _In_  ID3D11Fence *pFence,
    _In_  UINT64       Value) override
  {
    assert (ver_ >= 4);

    return
      static_cast <ID3D11DeviceContext4 *>(pReal)->Wait (pFence, Value);
  }


protected:
private:
  volatile LONG        refs_           = 1;
  unsigned int         ver_            = 0;
  bool                 deferred_       = false;
  UINT                 dev_ctx_handle_ = UINT_MAX;
  ID3D11DeviceContext* pReal           = nullptr;
};


ID3D11DeviceContext4*
SK_D3D11_Wrapper_Factory::wrapDeviceContext (ID3D11DeviceContext* dev_ctx)
{
  return
    new SK_IWrapD3D11DeviceContext (dev_ctx);
}

SK_LazyGlobal <SK_D3D11_Wrapper_Factory> SK_D3D11_WrapperFactory;