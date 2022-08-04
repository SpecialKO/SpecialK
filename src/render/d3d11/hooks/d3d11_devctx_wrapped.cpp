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

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"D3D11 Wrap"

#include <SpecialK/render/d3d11/d3d11_tex_mgr.h>
#include <SpecialK/render/d3d11/d3d11_state_tracker.h>

#define FRAME_TRACE
#ifdef  FRAME_TRACE
# define TraceAPI SK_LOG_FIRST_CALL
#else
# define TraceAPI 
#endif

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

// {F674A27D-A14E-48BA-9E1E-A72D20C018B4}
static const GUID IID_IUnwrappedD3D11Multithread =
{ 0xf674a27d, 0xa14e, 0x48ba, { 0x9e, 0x1e, 0xa7, 0x2d, 0x20, 0xc0, 0x18, 0xb4 } };

#define track_immediate config.render.dxgi.deferred_isolation

//#define SK_D3D11_LAZY_WRAP

DEFINE_GUID ( IID_ID3D10Multithread,
  0x9B7E4E00, 0x342C, 0x4106, 0xA1,
  0x9F, 0x4F, 0x27,   0x04,   0xF6,
  0x89, 0xF0 );
DEFINE_GUID ( IID_ID3D11Multithread,
  0x9B7E4E00, 0x342C, 0x4106, 0xA1,
  0x9F, 0x4F, 0x27,   0x04,   0xF6,
  0x89, 0xF0 );

// ^^^ They are the same COM object (!!)

class SK_IWrapD3D11Multithread : public ID3D11Multithread
{
public:
  explicit SK_IWrapD3D11Multithread ( SK_IWrapD3D11DeviceContext* parent_ )
  {
    pWrapperParent = (ID3D11DeviceContext *)parent_;

    if (parent_ != nullptr)
    {
      if (
        SUCCEEDED (
          pWrapperParent->QueryInterface
            (IID_IUnwrappedD3D11DeviceContext,
              (void **)&pRealParent.p)
                  )
         )
      {
        if (
          SUCCEEDED (
            pRealParent->QueryInterface
              <ID3D11Multithread>
                (&pReal.p)
                    )
           )
        {
          refs_ = 1;
        }
      }
    }
  };

  HRESULT
    STDMETHODCALLTYPE QueryInterface ( REFIID   riid,
                          _COM_Outptr_ void   **ppvObj ) override
  {
    HRESULT hr = E_POINTER;

    if (ppvObj == nullptr)
    {
      return hr;
    }

    if ( IsEqualGUID (riid, IID_IUnwrappedD3D11Multithread) != 0 )
    {
      //assert (ppvObject != nullptr);
      *ppvObj = pReal;

      pReal.p->AddRef ();

      hr = S_OK;

      return hr;
    }

    if ( riid == __uuidof (IUnknown)   ||
         riid == __uuidof (ID3D11Multithread) )
    {
      AddRef ();

      *ppvObj   = this;
             hr = S_OK;
      return hr;
    }

    hr =
      pReal->QueryInterface (riid, ppvObj);

    ///if (SUCCEEDED (hr))
    ///  InterlockedIncrement (&refs_);

    if ( riid != IID_ID3DUserDefinedAnnotation &&
         riid != IID_ID3D11VideoContext )      // 61F21C45-3C0E-4A74-9CEA-67100D9AD5E4
    {
      static
        std::unordered_set <std::wstring> reported_guids;

      wchar_t                wszGUID [41] = { };
      StringFromGUID2 (riid, wszGUID, 40);

      bool once =
        reported_guids.count (wszGUID) > 0;

      if (! once)
      {
        reported_guids.emplace (wszGUID);

        SK_LOG0 ( ( L"QueryInterface on wrapped D3D11Multithread thingy for Mystery UUID: %s",
                        wszGUID ), L"  D3D 11  " );
      }
    }

    return
      hr;
  }

  ULONG STDMETHODCALLTYPE AddRef  (void) override
  {
    InterlockedIncrement (&refs_);

    return
      pReal.p->AddRef ();
  }
  ULONG STDMETHODCALLTYPE Release (void) override
  {
    ULONG xrefs =
      InterlockedDecrement (&refs_);
    ULONG  refs = pReal.p->Release ();

    if (refs == 0 && xrefs != 0)
    {
      // Assertion always fails, we just want to be vocal about
      //   any code that causes this.
      SK_ReleaseAssert (xrefs == 0);
    }

    if (refs == 0)
    {
      SK_ReleaseAssert (ReadAcquire (&refs_) == 0);
    }

    if (xrefs == 0 && refs == 0 && refs_ == 0)
    {
      //delete this;
      return    0;
    }

    SK_LOG0 ( ( L"ID3D11Multithread::Release (...) -> xrefs=%lu, "
                                                     L"refs=%lu, refs_=%lu",
                xrefs, refs, refs_ ), __SK_SUBSYSTEM__
            );

    return refs;
  }

  void STDMETHODCALLTYPE Enter (void) override
  {
    pReal->Enter ();
  }
  void STDMETHODCALLTYPE Leave (void) override
  {
    pReal->Leave ();
  }
  BOOL STDMETHODCALLTYPE SetMultithreadProtected
  ( /* [annotation] */
    _In_            BOOL bMTProtect ) override
  {
    return
      pReal->SetMultithreadProtected (bMTProtect);
  }
  BOOL STDMETHODCALLTYPE GetMultithreadProtected (void) override
  {
    return
      pReal->GetMultithreadProtected ();
  }

private:
  volatile
    LONG                            refs_          =       1;
    SK_ComPtr <ID3D11DeviceContext> pWrapperParent = nullptr;
    SK_ComPtr <ID3D11DeviceContext> pRealParent    = nullptr;
    SK_ComPtr <ID3D11Multithread>   pReal          = nullptr;
};

bool SK_D3D11_QueueContextReset (ID3D11DeviceContext* pDevCtx, UINT dev_ctx);
bool SK_D3D11_DispatchContextResetQueue                       (UINT dev_ctx);

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

    IUnknown *pPromotion;

    if ( SUCCEEDED (QueryInterface (IID_ID3D11DeviceContext4, (void **)&pPromotion)) ||
         SUCCEEDED (QueryInterface (IID_ID3D11DeviceContext3, (void **)&pPromotion)) ||
         SUCCEEDED (QueryInterface (IID_ID3D11DeviceContext2, (void **)&pPromotion)) ||
         SUCCEEDED (QueryInterface (IID_ID3D11DeviceContext1, (void **)&pPromotion)) )
    {
      SK_LOG0 ( ( L"Promoted ID3D11DeviceContext to ID3D11DeviceContext%li", ver_),
                  __SK_SUBSYSTEM__ );
    }

    dev_ctx_handle_ =
      SK_D3D11_GetDeviceContextHandle (this);

    SK_D3D11_CopyContextHandle (this, pReal);

    deferred_ =
      SK_D3D11_IsDevCtxDeferred (pReal);

    ////pMultiThread.p =
    ////  SK_D3D11_WrapperFactory->wrapMultithread (this);
  };

  explicit SK_IWrapD3D11DeviceContext (ID3D11DeviceContext1* dev_ctx) : pReal (dev_ctx)
  {
    InterlockedExchange (
      &refs_, pReal->AddRef  () - 1
    );        pReal->Release ();
    AddRef ();

    ver_ = 1;

    IUnknown *pPromotion = nullptr;

    if ( SUCCEEDED (QueryInterface (IID_ID3D11DeviceContext4, (void **)&pPromotion)) ||
         SUCCEEDED (QueryInterface (IID_ID3D11DeviceContext3, (void **)&pPromotion)) ||
         SUCCEEDED (QueryInterface (IID_ID3D11DeviceContext2, (void **)&pPromotion)) )
    {
      SK_LOG0 ( ( L"Promoted ID3D11DeviceContext1 to ID3D11DeviceContext%li", ver_),
                  __SK_SUBSYSTEM__ );
    }

    dev_ctx_handle_ =
      SK_D3D11_GetDeviceContextHandle (this);

    SK_D3D11_CopyContextHandle (this, pReal);

    deferred_ =
      SK_D3D11_IsDevCtxDeferred (pReal);

    ////pMultiThread =
    ////  SK_D3D11_WrapperFactory->wrapMultithread (this);
  };

  explicit SK_IWrapD3D11DeviceContext (ID3D11DeviceContext2* dev_ctx) : pReal (dev_ctx)
  {
    InterlockedExchange (
      &refs_, pReal->AddRef  () - 1
    );        pReal->Release ();
    AddRef ();


    ver_ = 2;

    IUnknown *pPromotion = nullptr;

    if ( SUCCEEDED (QueryInterface (IID_ID3D11DeviceContext4, (void **)&pPromotion)) ||
         SUCCEEDED (QueryInterface (IID_ID3D11DeviceContext3, (void **)&pPromotion)) )
    {
      SK_LOG0 ( ( L"Promoted ID3D11DeviceContext2 to ID3D11DeviceContext%li", ver_),
                  __SK_SUBSYSTEM__ );
    }

    dev_ctx_handle_ =
      SK_D3D11_GetDeviceContextHandle (this);

    SK_D3D11_CopyContextHandle (this, pReal);

    deferred_ =
      SK_D3D11_IsDevCtxDeferred (pReal);

    ////pMultiThread.p =
    ////  SK_D3D11_WrapperFactory->wrapMultithread (this);
  };

  explicit SK_IWrapD3D11DeviceContext (ID3D11DeviceContext3* dev_ctx) : pReal (dev_ctx)
  {
    InterlockedExchange (
      &refs_, pReal->AddRef  () - 1
    );        pReal->Release ();
    AddRef ();

    ver_ = 3;

    IUnknown *pPromotion = nullptr;

    if (SUCCEEDED (QueryInterface (IID_ID3D11DeviceContext4, (void **)&pPromotion)))
    {
      SK_LOG0 ( ( L"Promoted ID3D11DeviceContext3 to ID3D11DeviceContext%li", ver_),
                  __SK_SUBSYSTEM__ );
    }

    dev_ctx_handle_ =
      SK_D3D11_GetDeviceContextHandle (this);

    SK_D3D11_CopyContextHandle (this, pReal);

    deferred_ =
      SK_D3D11_IsDevCtxDeferred (pReal);

    ////pMultiThread.p =
    ////  SK_D3D11_WrapperFactory->wrapMultithread (this);
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

    ////pMultiThread.p =
    ////  SK_D3D11_WrapperFactory->wrapMultithread (this);
  };

  virtual ~SK_IWrapD3D11DeviceContext (void) = default;

  HRESULT STDMETHODCALLTYPE QueryInterface ( REFIID   riid,
                                _COM_Outptr_ void   **ppvObj ) override
  {
    HRESULT hr = E_POINTER;

    if (ppvObj == nullptr)
    {
      return hr;
    }

    if ( IsEqualGUID (riid, IID_IUnwrappedD3D11DeviceContext) != 0 )
    {
      //assert (ppvObject != nullptr);
      *ppvObj = pReal;

      pReal->AddRef ();

      hr = S_OK;

      return hr;
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
          *ppvObj   = nullptr;
                 hr = E_NOINTERFACE;
          return hr;
        }

        ver_ =
          SK_COM_PromoteInterface (&pReal, pPromoted) ?
                                         required_ver : ver_;
      }

      else
      {
        AddRef ();
      }

      InterlockedIncrement (&refs_);

      *ppvObj   = this;
             hr = S_OK;
      return hr;
    }

    ////else if (riid == IID_ID3D11Multithread)
    ////{
    ////  hr =
    ////    ( pMultiThread.p != nullptr ) ?
    ////                             S_OK : E_NOINTERFACE;
    ////
    ////  if (hr == E_NOINTERFACE)
    ////  {
    ////    hr =
    ////      pReal->QueryInterface (riid, (void**)&pMultiThread.p);
    ////  }
    ////
    ////  else
    ////  {
    ////    hr = S_OK;
    ////
    ////    pMultiThread.p->AddRef ();
    ////  }
    ////
    ////  if (SUCCEEDED (hr))
    ////  {
    ////    *ppvObj = pMultiThread;
    ////
    ////    return hr;
    ////  }
    ////}

    if (riid == IID_ID3D11Device)
    {
      TraceAPI

      GetDevice ((ID3D11Device **)ppvObj);
      return S_OK;
    }

    hr =
      pReal->QueryInterface (riid, ppvObj);

    //if (SUCCEEDED (hr))
    //  InterlockedIncrement (&refs_);

    if ( riid == IID_ID3D11VideoContext )
      SK_LOG0 ( (L" * Game is using ID3D11VideoContext..."),
                 L"D3D11Video" );

    if ( riid != IID_ID3DUserDefinedAnnotation &&
         riid != IID_ID3D11Multithread         &&
         riid != IID_ID3D11VideoContext )      // 61F21C45-3C0E-4A74-9CEA-67100D9AD5E4)
    {
      static
        std::unordered_set <std::wstring> reported_guids;

      wchar_t                wszGUID [41] = { };
      StringFromGUID2 (riid, wszGUID, 40);

      bool once =
        reported_guids.count (wszGUID) > 0;

      if (! once)
      {
        reported_guids.emplace (wszGUID);

        SK_LOG0 ( ( L"QueryInterface on wrapped D3D11 Device Context for Mystery UUID: %s",
                        wszGUID ), L"  D3D 11  " );
      }
    }

    return
      hr;
  }

  ULONG STDMETHODCALLTYPE AddRef  (void) override
  {
    InterlockedIncrement (&refs_);

    return
      pReal->AddRef ();
  }
  ULONG STDMETHODCALLTYPE Release (void) override
  {
    ULONG xrefs =
      InterlockedDecrement (&refs_);
    ULONG  refs = pReal->Release ();

    if (refs == 0 && xrefs != 0)
    {
      // Assertion always fails, we just want to be vocal about
      //   any code that causes this.
      SK_ReleaseAssert (xrefs == 0);
    }

    if (refs == 0)
    {
      SK_ReleaseAssert (ReadAcquire (&refs_) == 0);
    }

    return refs;
  }

  void STDMETHODCALLTYPE GetDevice (
    _Out_  ID3D11Device **ppDevice ) override
  {
    TraceAPI

    return
      pReal->GetDevice (ppDevice);
  }

  HRESULT STDMETHODCALLTYPE GetPrivateData ( _In_                                 REFGUID guid,
                                             _Inout_                              UINT   *pDataSize,
                                             _Out_writes_bytes_opt_( *pDataSize ) void   *pData ) override
  {
    TraceAPI

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
    TraceAPI

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
    TraceAPI

    return
      pReal->SetPrivateDataInterface (guid, pData);
  }

  void STDMETHODCALLTYPE VSSetConstantBuffers (
    _In_range_     (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)         UINT                        StartSlot,
    _In_range_     (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot) UINT                        NumBuffers,
    _In_reads_opt_ (NumBuffers)                                                       ID3D11Buffer *const *ppConstantBuffers ) override
  {
    TraceAPI

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
    TraceAPI

  #ifndef SK_D3D11_LAZY_WRAP
    if (! SK_D3D11_IgnoreWrappedOrDeferred (true, pReal))
          SK_D3D11_SetShaderResources_Impl (
           SK_D3D11_ShaderType::Pixel,
                       deferred_ ?
                            TRUE : FALSE,
                  nullptr, pReal,
             StartSlot, NumViews,
           ppShaderResourceViews, dev_ctx_handle_ );
    else
#endif
      pReal->PSSetShaderResources (StartSlot, NumViews, ppShaderResourceViews);
  }

  void STDMETHODCALLTYPE PSSetShader (
    _In_opt_                           ID3D11PixelShader           *pPixelShader,
    _In_reads_opt_ (NumClassInstances) ID3D11ClassInstance *const *ppClassInstances,
                                       UINT                       NumClassInstances ) override
  {
    TraceAPI

    if (     ppClassInstances != nullptr && NumClassInstances > 256)
    {
      SK_ReleaseAssert (!"Too many class instances, is hook corrupted?");
    }
    else if (ppClassInstances == nullptr)   NumClassInstances = 0;

#ifndef SK_D3D11_LAZY_WRAP
    if (! SK_D3D11_IgnoreWrappedOrDeferred (true, pReal))
          SK_D3D11_SetShader_Impl     (pReal,
                pPixelShader, sk_shader_class::Pixel,
                                      ppClassInstances,
                                     NumClassInstances, true,
                                     dev_ctx_handle_);
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
    if (     ppClassInstances != nullptr && NumClassInstances > 256)
    {
      SK_ReleaseAssert (!"Too many class instances, is hook corrupted?");
    }
    else if (ppClassInstances == nullptr)   NumClassInstances = 0;

#ifndef SK_D3D11_LAZY_WRAP
    if (! SK_D3D11_IgnoreWrappedOrDeferred (true, pReal))
          SK_D3D11_SetShader_Impl   (pReal,
               pVertexShader, sk_shader_class::Vertex,
                                      ppClassInstances,
                                     NumClassInstances, true,
                                             dev_ctx_handle_);
    else
#endif
      pReal->VSSetShader (pVertexShader, ppClassInstances, NumClassInstances);
  }

  void STDMETHODCALLTYPE DrawIndexed (
    _In_  UINT IndexCount,
    _In_  UINT StartIndexLocation,
    _In_  INT  BaseVertexLocation ) override
  {
    TraceAPI

#ifndef SK_D3D11_LAZY_WRAP
    if (! SK_D3D11_IgnoreWrappedOrDeferred (true, pReal))
          SK_D3D11_DrawIndexed_Impl (pReal,
                                         IndexCount,
                                    StartIndexLocation,
                                    BaseVertexLocation, TRUE,
                                      dev_ctx_handle_);
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
    TraceAPI

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
    TraceAPI

#ifndef SK_D3D11_LAZY_WRAP
    if (! SK_D3D11_IgnoreWrappedOrDeferred (true, pReal))
      return
        SK_D3D11_Map_Impl         (pReal,
            pResource,
          Subresource,
                 MapType,
                 MapFlags,
                pMappedResource, TRUE
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
    TraceAPI

#ifndef SK_D3D11_LAZY_WRAP
    if (! SK_D3D11_IgnoreWrappedOrDeferred (true, pReal))
          SK_D3D11_Unmap_Impl       (pReal,
              pResource,
            Subresource, TRUE       );
    else
#endif
      pReal->Unmap (pResource, Subresource);
  }

  void STDMETHODCALLTYPE PSSetConstantBuffers (
    _In_range_     (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)         UINT                        StartSlot,
    _In_range_     (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot) UINT                        NumBuffers,
    _In_reads_opt_ (NumBuffers)                                                       ID3D11Buffer *const *ppConstantBuffers ) override
  {
    TraceAPI

    pReal->PSSetConstantBuffers ( StartSlot,
                     NumBuffers,
              ppConstantBuffers
    );
  }

  void STDMETHODCALLTYPE IASetInputLayout (
    _In_opt_  ID3D11InputLayout *pInputLayout ) override
  {
    TraceAPI

    pReal->IASetInputLayout (pInputLayout);
  }

  void STDMETHODCALLTYPE IASetVertexBuffers (
    _In_range_     ( 0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - 1 )           UINT                 StartSlot,
    _In_range_     ( 0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - StartSlot )   UINT                 NumBuffers,
    _In_reads_opt_ (NumBuffers)                                                   ID3D11Buffer *const *ppVertexBuffers,
    _In_reads_opt_ (NumBuffers)                                             const UINT                *pStrides,
    _In_reads_opt_ (NumBuffers)                                             const UINT                *pOffsets ) override
  {
    TraceAPI

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
    TraceAPI

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
    TraceAPI

#ifndef SK_D3D11_LAZY_WRAP
    if (! SK_D3D11_IgnoreWrappedOrDeferred (true, pReal))
      SK_D3D11_DrawIndexedInstanced_Impl ( pReal,
                                           IndexCountPerInstance, InstanceCount,
                                           StartIndexLocation,  BaseVertexLocation,
                                           StartInstanceLocation, TRUE, dev_ctx_handle_ );
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
    TraceAPI

#ifndef SK_D3D11_LAZY_WRAP
    if (! SK_D3D11_IgnoreWrappedOrDeferred (true, pReal))
          SK_D3D11_DrawInstanced_Impl (pReal,
                                      VertexCountPerInstance,
                                                    InstanceCount,
                                 StartVertexLocation,
                                               StartInstanceLocation,
                                   TRUE, dev_ctx_handle_
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
    TraceAPI

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
    TraceAPI

    if (     ppClassInstances != nullptr && NumClassInstances > 256)
    {
      SK_ReleaseAssert (!"Too many class instances, is hook corrupted?");
    }
    else if (ppClassInstances == nullptr)   NumClassInstances = 0;

#ifndef SK_D3D11_LAZY_WRAP
    if (! SK_D3D11_IgnoreWrappedOrDeferred (true, pReal))
          SK_D3D11_SetShader_Impl   (pReal,
             pGeometryShader, sk_shader_class::Geometry,
                                      ppClassInstances,
                                     NumClassInstances, true,
                 dev_ctx_handle_    );
    else
#endif
      pReal->GSSetShader (pGeometryShader, ppClassInstances, NumClassInstances);
  }

  void STDMETHODCALLTYPE IASetPrimitiveTopology (
    _In_  D3D11_PRIMITIVE_TOPOLOGY Topology ) override
  {
    TraceAPI

    pReal->IASetPrimitiveTopology (Topology);
  }

  void STDMETHODCALLTYPE VSSetShaderResources (
    _In_range_     ( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )         UINT                                          StartSlot,
    _In_range_     ( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot ) UINT                                          NumViews,
    _In_reads_opt_ (NumViews)                                                      ID3D11ShaderResourceView *const *ppShaderResourceViews ) override
  {
    TraceAPI

#ifndef SK_D3D11_LAZY_WRAP
    if (! SK_D3D11_IgnoreWrappedOrDeferred (true, pReal))
          SK_D3D11_SetShaderResources_Impl (
             SK_D3D11_ShaderType::Vertex,
                          deferred_ ?
                               TRUE : FALSE,
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
    TraceAPI

    pReal->VSSetSamplers ( StartSlot,
             NumSamplers,
              ppSamplers
    );
  }

  void STDMETHODCALLTYPE Begin (_In_ ID3D11Asynchronous *pAsync) override
  {
    TraceAPI

    pReal->Begin (pAsync);
  }

  void STDMETHODCALLTYPE End (_In_  ID3D11Asynchronous *pAsync) override
  {
    TraceAPI

    pReal->End (pAsync);
  }

  HRESULT STDMETHODCALLTYPE GetData (
    _In_                                ID3D11Asynchronous *pAsync,
    _Out_writes_bytes_opt_( DataSize )  void               *pData,
    _In_                                UINT                 DataSize,
    _In_                                UINT              GetDataFlags ) override
  {
    TraceAPI

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
    TraceAPI

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
    TraceAPI

#ifndef SK_D3D11_LAZY_WRAP
    if (! SK_D3D11_IgnoreWrappedOrDeferred (true, pReal))
          SK_D3D11_SetShaderResources_Impl (
             SK_D3D11_ShaderType::Geometry,
                                 deferred_ ?
                                      TRUE : FALSE,
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
    TraceAPI

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
    TraceAPI

#ifndef SK_D3D11_LAZY_WRAP
    if (! SK_D3D11_IgnoreWrappedOrDeferred (true, pReal))
          SK_D3D11_OMSetRenderTargets_Impl (pReal,
                                 NumViews,
                      ppRenderTargetViews,
                       pDepthStencilView, TRUE,
                       dev_ctx_handle_     );
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
    TraceAPI

#ifndef SK_D3D11_LAZY_WRAP
    if (! SK_D3D11_IgnoreWrappedOrDeferred (true, pReal))
          SK_D3D11_OMSetRenderTargetsAndUnorderedAccessViews_Impl (pReal,
                     NumRTVs,
                      ppRenderTargetViews,
                       pDepthStencilView,  UAVStartSlot, NumUAVs,
                                 ppUnorderedAccessViews,   pUAVInitialCounts,
            TRUE, dev_ctx_handle_
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
    TraceAPI

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
    TraceAPI

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
    TraceAPI

    pReal->SOSetTargets ( NumBuffers,
            ppSOTargets,
             pOffsets
    );
  }

  void STDMETHODCALLTYPE DrawAuto (void) override
  {
    TraceAPI

#ifndef SK_D3D11_LAZY_WRAP
  if (! SK_D3D11_IgnoreWrappedOrDeferred (true, pReal))
        SK_D3D11_DrawAuto_Impl    (pReal, TRUE, dev_ctx_handle_);
    else
#endif
      pReal->DrawAuto ();
  }

  void STDMETHODCALLTYPE DrawIndexedInstancedIndirect (
    _In_ ID3D11Buffer           *pBufferForArgs,
    _In_ UINT          AlignedByteOffsetForArgs ) override
  {
    TraceAPI

#ifndef SK_D3D11_LAZY_WRAP
  if (! SK_D3D11_IgnoreWrappedOrDeferred (true, pReal))
        SK_D3D11_DrawIndexedInstancedIndirect_Impl (pReal,
                           pBufferForArgs,
                 AlignedByteOffsetForArgs,
                    TRUE, dev_ctx_handle_
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
    TraceAPI

#ifndef SK_D3D11_LAZY_WRAP
  if (! SK_D3D11_IgnoreWrappedOrDeferred (true, pReal))
        SK_D3D11_DrawInstancedIndirect_Impl (pReal,
                    pBufferForArgs,
          AlignedByteOffsetForArgs, TRUE, dev_ctx_handle_
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
    TraceAPI

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
    TraceAPI

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
    TraceAPI

    pReal->RSSetState (
     pRasterizerState
    );
  }

  void STDMETHODCALLTYPE RSSetViewports (
    _In_range_     (0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE) UINT          NumViewports,
    _In_reads_opt_ (NumViewports)                                          const D3D11_VIEWPORT *pViewports ) override
  {
    TraceAPI

#ifndef _M_AMD64
    static const auto game_id =
          SK_GetCurrentGameID ();

    switch (game_id)
    {
      case SK_GAME_ID::ChronoCross:
      {
        extern float
            __SK_CC_ResMultiplier;
        if (__SK_CC_ResMultiplier)
        {
          if (NumViewports == 1)
          {
            SK_ComPtr <ID3D11RenderTargetView> rtv;
            OMGetRenderTargets            (1, &rtv, nullptr);

            SK_ComPtr <ID3D11Texture2D> pTex;

            if (rtv.p != nullptr)
            {
              SK_ComPtr <ID3D11Resource> pRes;
              rtv->GetResource         (&pRes.p);

              if (pRes.p != nullptr)
                pRes->QueryInterface <ID3D11Texture2D> (&pTex.p);

              if (pTex.p != nullptr)
              {
                D3D11_VIEWPORT vp = *pViewports;

                D3D11_TEXTURE2D_DESC texDesc = { };
                pTex->GetDesc      (&texDesc);

                if ( texDesc.Width  == 4096 * __SK_CC_ResMultiplier &&
                     texDesc.Height == 2048 * __SK_CC_ResMultiplier )
                {
                  static float NewWidth  = 4096.0f * __SK_CC_ResMultiplier;
                  static float NewHeight = 2048.0f * __SK_CC_ResMultiplier;

                  float left_ndc = 2.0f * ( vp.TopLeftX / 4096.0f) - 1.0f;
                  float top_ndc  = 2.0f * ( vp.TopLeftY / 2048.0f) - 1.0f;

                  vp.TopLeftX = (left_ndc * NewWidth  + NewWidth)  / 2.0f;
                  vp.TopLeftY = (top_ndc  * NewHeight + NewHeight) / 2.0f;
                  vp.Width    = __SK_CC_ResMultiplier * vp.Width;
                  vp.Height   = __SK_CC_ResMultiplier * vp.Height;

                  vp.TopLeftX = std::min (vp.TopLeftX, 32767.0f);
                  vp.TopLeftY = std::min (vp.TopLeftY, 32767.0f);

                  vp.Width    = std::min (vp.Width,    32767.0f);
                  vp.Height   = std::min (vp.Height,   32767.0f);

                  pReal->RSSetViewports (1, &vp);

                  return;
                }
              }
            }
          }
        }
      } break;

      default:
        break;
    }
#endif

    pReal->RSSetViewports (
             NumViewports,
               pViewports
    );
  }

  void STDMETHODCALLTYPE RSSetScissorRects (
    _In_range_     (0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE) UINT      NumRects,
    _In_reads_opt_ (NumRects)                                              const D3D11_RECT *pRects ) override
  {
    TraceAPI

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
    TraceAPI

#ifndef _M_AMD64
    static const auto game_id =
          SK_GetCurrentGameID ();

    D3D11_BOX newBox = { };

    switch (game_id)
    {
      case SK_GAME_ID::ChronoCross:
      {
        extern float
            __SK_CC_ResMultiplier;
        if (__SK_CC_ResMultiplier > 1.0f)
        {
          SK_ComQIPtr <ID3D11Texture2D>
            pSrcTex (pSrcResource),
            pDstTex (pDstResource);

          if ( pSrcTex.p != nullptr &&
               pDstTex.p != nullptr )
          {
            D3D11_TEXTURE2D_DESC srcDesc = { },
                                 dstDesc = { };
            pSrcTex->GetDesc   (&srcDesc);
            pDstTex->GetDesc   (&dstDesc);

            if (srcDesc.Width == __SK_CC_ResMultiplier * 4096 && srcDesc.Height == __SK_CC_ResMultiplier * 2048 && pSrcBox != nullptr &&
                dstDesc.Width == __SK_CC_ResMultiplier * 4096 && dstDesc.Height == __SK_CC_ResMultiplier * 2048)
            {
              newBox = *pSrcBox;

              static float NewWidth  = 2048 * __SK_CC_ResMultiplier;
              static float NewHeight = 1024 * __SK_CC_ResMultiplier;

              float left_ndc   = 2.0f * ( static_cast <float> (std::clamp (newBox.left,   0U, 4096U)) / 4096.0f ) - 1.0f;
              float top_ndc    = 2.0f * ( static_cast <float> (std::clamp (newBox.top,    0U, 2048U)) / 2048.0f ) - 1.0f;
              float right_ndc  = 2.0f * ( static_cast <float> (std::clamp (newBox.right,  0U, 4096U)) / 4096.0f ) - 1.0f;
              float bottom_ndc = 2.0f * ( static_cast <float> (std::clamp (newBox.bottom, 0U, 2048U)) / 2048.0f ) - 1.0f;

              newBox.left   = static_cast <UINT> (std::max (0.0f, (left_ndc   * NewWidth  + NewWidth)  ));
              newBox.top    = static_cast <UINT> (std::max (0.0f, (top_ndc    * NewHeight + NewHeight) ));
              newBox.right  = static_cast <UINT> (std::max (0.0f, (right_ndc  * NewWidth  + NewWidth)  ));
              newBox.bottom = static_cast <UINT> (std::max (0.0f, (bottom_ndc * NewHeight + NewHeight) ));

              DstX *= static_cast <UINT> (__SK_CC_ResMultiplier);
              DstY *= static_cast <UINT> (__SK_CC_ResMultiplier);

              pSrcBox = &newBox;
            }

            else if (pSrcBox != nullptr && ( pSrcBox->right  > srcDesc.Width ||
                                             pSrcBox->bottom > srcDesc.Height ))
            {
              SK_LOGi0 ( L"xxxDesc={%dx%d}, Box={%d/%d::%d,%d}",
                           srcDesc.Width, srcDesc.Height,
                             pSrcBox->left,  pSrcBox->top,
                             pSrcBox->right, pSrcBox->bottom );

              return;
            }
          }
        }
      } break;

      default:
        break;
    }
#endif

#ifndef SK_D3D11_LAZY_WRAP
  if (! SK_D3D11_IgnoreWrappedOrDeferred (true, pReal))
        SK_D3D11_CopySubresourceRegion_Impl (pReal,
                 pDstResource, DstSubresource, DstX, DstY, DstZ,
                 pSrcResource, SrcSubresource, pSrcBox, TRUE
        );
    else
#endif
      pReal->CopySubresourceRegion ( pDstResource, DstSubresource, DstX, DstY, DstZ,
                                     pSrcResource, SrcSubresource, pSrcBox );
  }

  void STDMETHODCALLTYPE CopyResource (
    _In_ ID3D11Resource *pDstResource,
    _In_ ID3D11Resource *pSrcResource ) override
  {
    TraceAPI

    D3D11_RESOURCE_DIMENSION
      dim_dst = D3D11_RESOURCE_DIMENSION_UNKNOWN,
      dim_src = D3D11_RESOURCE_DIMENSION_UNKNOWN;

    pDstResource->GetType (&dim_dst);
    pSrcResource->GetType (&dim_src);

    if (dim_dst == D3D11_RESOURCE_DIMENSION_TEXTURE2D &&
        dim_src == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
    {
      SK_ComQIPtr <ID3D11Texture2D> pSrcTex (pSrcResource);
      SK_ComQIPtr <ID3D11Texture2D> pDstTex (pDstResource);

      if (pSrcTex.p != nullptr &&
          pDstTex.p != nullptr)
      {
        D3D11_TEXTURE2D_DESC srcDesc = { };
        D3D11_TEXTURE2D_DESC dstDesc = { };

        pSrcTex->GetDesc (&srcDesc);
        pDstTex->GetDesc (&dstDesc);

        if (srcDesc.Width            != dstDesc.Width  ||
            srcDesc.Height           != dstDesc.Height ||
            DirectX::MakeTypeless      (srcDesc.Format) !=
            DirectX::MakeTypeless      (dstDesc.Format) ||
            srcDesc.SampleDesc.Count != dstDesc.SampleDesc.Count)
        {
          extern bool SK_D3D11_BltCopySurface (
                  ID3D11Texture2D* pSrcTex,
                  ID3D11Texture2D* pDstTex
          );

          if (SK_D3D11_BltCopySurface (pSrcTex, pDstTex))
            return;
        }
      }
    }

#ifndef SK_D3D11_LAZY_WRAP
  if (! SK_D3D11_IgnoreWrappedOrDeferred (true, pReal))
        SK_D3D11_CopyResource_Impl (pReal,
                 pDstResource,
                 pSrcResource, TRUE
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
    TraceAPI

#ifndef SK_D3D11_LAZY_WRAP
  if (! SK_D3D11_IgnoreWrappedOrDeferred (true, pReal))
        SK_D3D11_UpdateSubresource_Impl (pReal,
                      pDstResource,
                       DstSubresource,
                      pDstBox,
                      pSrcData,
                       SrcRowPitch,
                       SrcDepthPitch, TRUE
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
    TraceAPI

    pReal->CopyStructureCount ( pDstBuffer,
                                 DstAlignedByteOffset,
                                pSrcView
    );
  }

  void STDMETHODCALLTYPE ClearRenderTargetView (
    _In_       ID3D11RenderTargetView *pRenderTargetView,
    _In_ const FLOAT                   ColorRGBA [4] ) override
  {
    TraceAPI

    pReal->ClearRenderTargetView (
               pRenderTargetView, ColorRGBA
    );
  }

  void STDMETHODCALLTYPE ClearUnorderedAccessViewUint (
    _In_       ID3D11UnorderedAccessView *pUnorderedAccessView,
    _In_ const UINT                       Values [4] ) override
  {
    TraceAPI

    pReal->ClearUnorderedAccessViewUint (
               pUnorderedAccessView, Values
    );
  }

  void STDMETHODCALLTYPE ClearUnorderedAccessViewFloat (
    _In_       ID3D11UnorderedAccessView *pUnorderedAccessView,
    _In_ const FLOAT                      Values [4] ) override
  {
    TraceAPI

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
    TraceAPI

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
    TraceAPI

    pReal->GenerateMips (pShaderResourceView);
  }

  void STDMETHODCALLTYPE SetResourceMinLOD (
    _In_ ID3D11Resource *pResource,
    FLOAT           MinLOD ) override
  {
    TraceAPI

    pReal->SetResourceMinLOD (pResource, MinLOD);
  }

  FLOAT STDMETHODCALLTYPE GetResourceMinLOD (
    _In_  ID3D11Resource *pResource ) override
  {
    TraceAPI

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
    TraceAPI

  #ifndef SK_D3D11_LAZY_WRAP
    if (! SK_D3D11_IgnoreWrappedOrDeferred (true, pReal))
        SK_D3D11_ResolveSubresource_Impl ( pReal,
                      pDstResource, DstSubresource,
                      pSrcResource, SrcSubresource,
                            Format, TRUE
        );
      else
  #endif
      pReal->ResolveSubresource ( pDstResource, DstSubresource,
                                  pSrcResource, SrcSubresource, Format );
  }

  void STDMETHODCALLTYPE ExecuteCommandList (
    _In_  ID3D11CommandList *pCommandList,
          BOOL               RestoreContextState ) override
  {
    TraceAPI

    SK_ComPtr <ID3D11DeviceContext>
         pBuildContext (nullptr);
    UINT  size        =        sizeof (LPVOID);


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


    // Broken
#if 0
    if ( SUCCEEDED (
      pCommandList->GetPrivateData (
      SKID_D3D11DeviceContextOrigin,
         &size,   &pBuildContext.p )
       )           )
    {
      if (! pBuildContext.IsEqualObject (pReal))
      {
        SK_D3D11_MergeCommandLists (
          pBuildContext,
            pReal
        );
      }

      pCommandList->SetPrivateDataInterface (SKID_D3D11DeviceContextOrigin, nullptr);
    }
#else
    UNREFERENCED_PARAMETER (size);
#endif

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
    TraceAPI

#ifndef SK_D3D11_LAZY_WRAP
  if (! SK_D3D11_IgnoreWrappedOrDeferred (true, pReal))
        SK_D3D11_SetShaderResources_Impl (
           SK_D3D11_ShaderType::Hull,
                           deferred_ ?
                                TRUE : FALSE,
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
    TraceAPI

    if (     ppClassInstances != nullptr && NumClassInstances > 256)
    {
      SK_ReleaseAssert (!"Too many class instances, is hook corrupted?");
    }
    else if (ppClassInstances == nullptr)   NumClassInstances = 0;

#ifndef SK_D3D11_LAZY_WRAP
  if (! SK_D3D11_IgnoreWrappedOrDeferred (true, pReal))
        SK_D3D11_SetShader_Impl          (      pReal,
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
    TraceAPI

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
    TraceAPI

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
    TraceAPI

#ifndef SK_D3D11_LAZY_WRAP
  if (! SK_D3D11_IgnoreWrappedOrDeferred (true, pReal))
        SK_D3D11_SetShaderResources_Impl (
           SK_D3D11_ShaderType::Domain,
                             deferred_ ?
                                  TRUE : FALSE,
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
    TraceAPI

    if (     ppClassInstances != nullptr && NumClassInstances > 256)
    {
      SK_ReleaseAssert (!"Too many class instances, is hook corrupted?");
    }
    else if (ppClassInstances == nullptr)   NumClassInstances = 0;

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
    TraceAPI

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
    TraceAPI

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
    TraceAPI

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
    TraceAPI

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
    TraceAPI

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
    TraceAPI

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
    TraceAPI

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
    TraceAPI

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
    TraceAPI

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
    TraceAPI

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
    TraceAPI

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
    TraceAPI

    pReal->PSGetConstantBuffers ( StartSlot,
                     NumBuffers,
              ppConstantBuffers
    );
  }

  void STDMETHODCALLTYPE IAGetInputLayout (
    _Out_  ID3D11InputLayout **ppInputLayout ) override
  {
    TraceAPI

    pReal->IAGetInputLayout (ppInputLayout);
  }

  void STDMETHODCALLTYPE IAGetVertexBuffers (
    _In_range_       (0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - 1)          UINT                StartSlot,
    _In_range_       (0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT                NumBuffers,
    _Out_writes_opt_ (NumBuffers)                                                ID3D11Buffer **ppVertexBuffers,
    _Out_writes_opt_ (NumBuffers)                                                UINT          *pStrides,
    _Out_writes_opt_ (NumBuffers)                                                UINT          *pOffsets ) override
  {
    TraceAPI

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
    TraceAPI

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
    TraceAPI

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
    TraceAPI

    return
      pReal->GSGetShader (
        ppGeometryShader,
        ppClassInstances,
      pNumClassInstances );
  }

  void STDMETHODCALLTYPE IAGetPrimitiveTopology (
    _Out_  D3D11_PRIMITIVE_TOPOLOGY *pTopology ) override
  {
    TraceAPI

    return
      pReal->IAGetPrimitiveTopology (pTopology);
  }

  void STDMETHODCALLTYPE VSGetShaderResources (
    _In_range_       (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)         UINT                         StartSlot,
    _In_range_       (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot) UINT                         NumViews,
    _Out_writes_opt_ (NumViews)                                                    ID3D11ShaderResourceView **ppShaderResourceViews ) override
  {
    TraceAPI

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
    TraceAPI

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
    TraceAPI

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
    TraceAPI

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
    TraceAPI

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
    TraceAPI

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
    TraceAPI

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
    TraceAPI

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
    TraceAPI

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
    TraceAPI

    return
      pReal->SOGetTargets (
        NumBuffers,
           ppSOTargets
      );
  }

  void STDMETHODCALLTYPE RSGetState (
    _Out_ ID3D11RasterizerState **ppRasterizerState ) override
  {
    TraceAPI

    return
      pReal->RSGetState (ppRasterizerState);
  }

  void STDMETHODCALLTYPE RSGetViewports(
    _Inout_ /*_range(0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE )*/   UINT *pNumViewports,
    _Out_writes_opt_(*pNumViewports)  D3D11_VIEWPORT *pViewports) override
  {
    TraceAPI

    return
      pReal->RSGetViewports (pNumViewports, pViewports);
  }

  void STDMETHODCALLTYPE RSGetScissorRects (
    _Inout_ /*_range(0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE )*/   UINT *pNumRects,
    _Out_writes_opt_ (*pNumRects)  D3D11_RECT *pRects) override
  {
    TraceAPI

    return
      pReal->RSGetScissorRects (pNumRects, pRects);
  }

  void STDMETHODCALLTYPE HSGetShaderResources(
    _In_range_       ( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )         UINT                         StartSlot,
    _In_range_       ( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot ) UINT                         NumViews,
    _Out_writes_opt_ (NumViews)                                                      ID3D11ShaderResourceView **ppShaderResourceViews ) override
  {
    TraceAPI

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
    TraceAPI

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
    TraceAPI

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
    TraceAPI

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
    TraceAPI

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
    TraceAPI

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
    TraceAPI

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
    TraceAPI

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
    TraceAPI

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
    TraceAPI

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
    TraceAPI

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
    TraceAPI

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
    TraceAPI

    return
      pReal->CSGetConstantBuffers ( StartSlot,
                       NumBuffers,
                ppConstantBuffers
      );
  }

  void STDMETHODCALLTYPE ClearState (void) override
  {
    TraceAPI

    SK_D3D11_QueueContextReset  (pReal, dev_ctx_handle_);
    pReal->ClearState           (                      );
    SK_D3D11_DispatchContextResetQueue (dev_ctx_handle_);
  }

  void STDMETHODCALLTYPE Flush (void) override
  {
    TraceAPI

    return
      pReal->Flush ();
  }

  D3D11_DEVICE_CONTEXT_TYPE STDMETHODCALLTYPE GetType (void) override
  {
    TraceAPI

    return
      pReal->GetType ();
  }

  UINT STDMETHODCALLTYPE GetContextFlags (void) override
  {
    TraceAPI

    return
      pReal->GetContextFlags ();
  }

  HRESULT STDMETHODCALLTYPE FinishCommandList (
              BOOL                RestoreDeferredContextState,
    _Out_opt_ ID3D11CommandList **ppCommandList ) override
  {
    TraceAPI

    HRESULT hr =
      pReal->FinishCommandList (RestoreDeferredContextState, ppCommandList);

    if (SUCCEEDED (hr) && (ppCommandList               != nullptr &&
                          (RestoreDeferredContextState == FALSE)))
    {
      (*ppCommandList)->SetPrivateData ( SKID_D3D11DeviceContextOrigin,
                                           sizeof (std::uintptr_t), pReal );
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
    TraceAPI

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
    TraceAPI

    assert (ver_ >= 1);

    return
      static_cast <ID3D11DeviceContext1 *>(pReal)->UpdateSubresource1 (pDstResource, DstSubresource, pDstBox, pSrcData, SrcRowPitch, SrcDepthPitch, CopyFlags);
  }
  void STDMETHODCALLTYPE DiscardResource (
    _In_  ID3D11Resource *pResource ) override
  {
    TraceAPI

    assert (ver_ >= 1);

    return
      static_cast <ID3D11DeviceContext1 *>(pReal)->DiscardResource (pResource);
  }
  void STDMETHODCALLTYPE DiscardView (
    _In_ ID3D11View *pResourceView ) override
  {
    TraceAPI

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
    TraceAPI

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
    TraceAPI

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
    TraceAPI

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
    TraceAPI

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
    TraceAPI

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
    TraceAPI

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
    TraceAPI

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
    TraceAPI

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
    TraceAPI

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
    TraceAPI

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
    TraceAPI

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
    TraceAPI

    assert (ver_ >= 1);

    return
      static_cast <ID3D11DeviceContext1 *>(pReal)->CSGetConstantBuffers1 (StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants);
  }
  void STDMETHODCALLTYPE SwapDeviceContextState (
    _In_      ID3DDeviceContextState  *pState,
    _Out_opt_ ID3DDeviceContextState **ppPreviousState ) override
  {
    TraceAPI

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
    TraceAPI

    assert (ver_ >= 1);

    return
      static_cast <ID3D11DeviceContext1 *>(pReal)->ClearView (pView, Color, pRect, NumRects);
  }
  void STDMETHODCALLTYPE DiscardView1 (
    _In_                            ID3D11View *pResourceView,
    _In_reads_opt_ (NumRects) const D3D11_RECT *pRects,
    UINT        NumRects ) override
  {
    TraceAPI

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
    TraceAPI

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
    TraceAPI

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
    TraceAPI

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
    TraceAPI

    assert (ver_ >= 2);

    return
      static_cast <ID3D11DeviceContext2 *>(pReal)->UpdateTiles (pDestTiledResource, pDestTileRegionStartCoordinate, pDestTileRegionSize, pSourceTileData, Flags);
  }
  HRESULT STDMETHODCALLTYPE ResizeTilePool (
    _In_  ID3D11Buffer *pTilePool,
    _In_  UINT64 NewSizeInBytes) override
  {
    TraceAPI

    assert (ver_ >= 2);

    return
      static_cast <ID3D11DeviceContext2 *>(pReal)->ResizeTilePool (pTilePool, NewSizeInBytes);
  }
  void STDMETHODCALLTYPE TiledResourceBarrier (
    _In_opt_  ID3D11DeviceChild *pTiledResourceOrViewAccessBeforeBarrier,
    _In_opt_  ID3D11DeviceChild *pTiledResourceOrViewAccessAfterBarrier) override
  {
    TraceAPI

    assert (ver_ >= 2);

    return
      static_cast <ID3D11DeviceContext2 *>(pReal)->TiledResourceBarrier (pTiledResourceOrViewAccessBeforeBarrier, pTiledResourceOrViewAccessAfterBarrier);
  }
  BOOL STDMETHODCALLTYPE IsAnnotationEnabled (void) override
  {
    TraceAPI

    assert (ver_ >= 2);

    return
      static_cast <ID3D11DeviceContext2 *>(pReal)->IsAnnotationEnabled ();
  }
  void STDMETHODCALLTYPE SetMarkerInt (
    _In_  LPCWSTR pLabel,
    INT     Data) override
  {
    TraceAPI

    assert (ver_ >= 2);

    return
      static_cast <ID3D11DeviceContext2 *>(pReal)->SetMarkerInt (pLabel, Data);
  }
  void STDMETHODCALLTYPE BeginEventInt (
    _In_  LPCWSTR pLabel,
          INT     Data) override
  {
    TraceAPI

    assert (ver_ >= 2);

    return
      static_cast <ID3D11DeviceContext2 *>(pReal)->BeginEventInt (pLabel, Data);
  }
  void STDMETHODCALLTYPE EndEvent (void) override
  {
    TraceAPI

    assert (ver_ >= 2);

    return
      static_cast <ID3D11DeviceContext2 *>(pReal)->EndEvent ();
  }



  void STDMETHODCALLTYPE Flush1 (
    D3D11_CONTEXT_TYPE ContextType,
    _In_opt_  HANDLE             hEvent) override
  {
    TraceAPI

    assert (ver_ >= 3);

    return
      static_cast <ID3D11DeviceContext3 *>(pReal)->Flush1 (ContextType, hEvent);
  }
  void STDMETHODCALLTYPE SetHardwareProtectionState (
    _In_  BOOL HwProtectionEnable) override
  {
    TraceAPI

    assert (ver_ >= 3);

    return
      static_cast <ID3D11DeviceContext3 *>(pReal)->SetHardwareProtectionState (HwProtectionEnable);
  }
  void STDMETHODCALLTYPE GetHardwareProtectionState (
    _Out_  BOOL *pHwProtectionEnable) override
  {
    TraceAPI

    assert (ver_ >= 3);

    return
      static_cast <ID3D11DeviceContext3 *>(pReal)->GetHardwareProtectionState (pHwProtectionEnable);
  }



  HRESULT STDMETHODCALLTYPE Signal (
    _In_  ID3D11Fence *pFence,
    _In_  UINT64       Value) override
  {
    TraceAPI

    assert (ver_ >= 4);

    return
      static_cast <ID3D11DeviceContext4 *>(pReal)->Signal (pFence, Value);
  }
  HRESULT STDMETHODCALLTYPE Wait (
    _In_  ID3D11Fence *pFence,
    _In_  UINT64       Value) override
  {
    TraceAPI

    assert (ver_ >= 4);

    return
      static_cast <ID3D11DeviceContext4 *>(pReal)->Wait (pFence, Value);
  }


protected:
private:
  volatile LONG         refs_           = 1;
  unsigned int          ver_            = 0;
  bool                  deferred_       = false;
  UINT                  dev_ctx_handle_ = UINT_MAX;
  ID3D11DeviceContext*  pReal           = nullptr;
  ///SK_ComPtr
  ///  <SK_IWrapD3D11Multithread>
  ///                      pMultiThread    = nullptr;
};

ID3D11DeviceContext4*
SK_D3D11_Wrapper_Factory::wrapDeviceContext (ID3D11DeviceContext* dev_ctx)
{
  return
    new SK_IWrapD3D11DeviceContext (dev_ctx);
}

SK_IWrapD3D11Multithread*
SK_D3D11_Wrapper_Factory::wrapMultithread (SK_IWrapD3D11DeviceContext* wrapped_ctx)
{
  return
    new SK_IWrapD3D11Multithread (wrapped_ctx);
}



SK_LazyGlobal <SK_D3D11_Wrapper_Factory> SK_D3D11_WrapperFactory;