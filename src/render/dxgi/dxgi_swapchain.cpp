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

#include <SpecialK/render/dxgi/dxgi_swapchain.h>
#include <SpecialK/render/dxgi/dxgi_util.h>
#include <SpecialK/render/d3d11/d3d11_core.h>

#define SK_LOG_ONCE(x) { static bool logged = false; if (! logged) \
                       { dll_log->Log ((x)); logged = true; } }

HRESULT
STDMETHODCALLTYPE
SK_DXGISwap3_SetColorSpace1_Impl (
  IDXGISwapChain3       *pSwapChain3,
  DXGI_COLOR_SPACE_TYPE  ColorSpace,
  BOOL                   bWrapped = FALSE,
  void*                  pCaller  = nullptr
);

// Various compat hacks applied if the game was originally Blt Model
extern BOOL _NO_ALLOW_MODE_SWITCH;

volatile LONG SK_DXGI_LiveWrappedSwapChains  = 0;
volatile LONG SK_DXGI_LiveWrappedSwapChain1s = 0;

struct __declspec (uuid ("{24430A12-6E3C-4706-AFFA-B3EEF2DF4102}") ) IWrapDXGISwapChain;

enum class SK_DXGI_PresentSource
{
  Wrapper = 0,
  Hook    = 1
};


HRESULT
STDMETHODCALLTYPE
SK_DXGI_DispatchPresent1 (IDXGISwapChain1         *This,
                          UINT                    SyncInterval,
                          UINT                    Flags,
           const DXGI_PRESENT_PARAMETERS         *pPresentParameters,
                          Present1SwapChain1_pfn  DXGISwapChain1_Present1,
                          SK_DXGI_PresentSource   Source);

HRESULT
STDMETHODCALLTYPE
SK_DXGI_DispatchPresent (IDXGISwapChain        *This,
                         UINT                   SyncInterval,

                         UINT                   Flags,
                         PresentSwapChain_pfn   DXGISwapChain_Present,
                         SK_DXGI_PresentSource  Source);


// SwapChain recycling cleanup
UINT
SK_DXGI_ReleaseSwapChainOnHWnd (
  IDXGISwapChain1* pChain,
  HWND             hWnd,
  IUnknown*        pDevice
);


void
__stdcall
SK_DXGI_SwapChainDestructionCallback (void *pData)
{
  auto pSwapChain =
    (IWrapDXGISwapChain *)pData;

  if ( SK::Framerate::limiters_->count (pSwapChain) &&
       SK::Framerate::FreeLimiter      (pSwapChain) )
  {
    SK_LOGi0 (
      L"SwapChain (%ph) and Framerate Limiter Destroyed",
       pSwapChain );
  }

  if ( SK::Framerate::limiters_->count (pSwapChain->pReal) &&
       SK::Framerate::FreeLimiter      (pSwapChain->pReal) )
  {
    SK_LOGi0 (
      L"SwapChain (%ph) and Framerate Limiter Destroyed",
       pSwapChain->pReal );
  }
}

HRESULT
IWrapDXGISwapChain::RegisterDestructionCallback (void)
{
  UINT uiDontCare = 0;

  SK_ComQIPtr <ID3DDestructionNotifier>
                  pDestructomatic (pReal);

  if (pDestructomatic != nullptr)
  {
    return
      pDestructomatic->RegisterDestructionCallback (
                SK_DXGI_SwapChainDestructionCallback,
                                 this, &uiDontCare );
  }

  return
    E_NOTIMPL;
}

struct DECLSPEC_UUID ("ADEC44E2-61F0-45C3-AD9F-1B37379284FF")
  IStreamlineBaseInterface : IUnknown { };

// IDXGISwapChain
HRESULT
STDMETHODCALLTYPE
IWrapDXGISwapChain::QueryInterface (REFIID riid, void **ppvObj)
{
  if (ppvObj == nullptr)
  {
    return E_POINTER;
  }

  else if (riid == IID_IUnwrappedDXGISwapChain)
  {
    *ppvObj = pReal;

    pReal->AddRef ();

    return S_OK;
  }

  // Keep SwapChain wrapping the hell away from Streamline!
  else if (riid == __uuidof (IStreamlineBaseInterface))
  {
    SK_LOGi1 (L"Tried to get Streamline Base Interface for a SwapChain that SK has wrapped...");

    return E_NOINTERFACE;
  }

  else if (
    riid == __uuidof (IWrapDXGISwapChain)   ||
    riid == __uuidof (IUnknown)             ||
    riid == __uuidof (IDXGIObject)          ||
    riid == __uuidof (IDXGIDeviceSubObject) ||
    riid == __uuidof (IDXGISwapChain)       ||
    riid == __uuidof (IDXGISwapChain1)      ||
    riid == __uuidof (IDXGISwapChain2)      ||
    riid == __uuidof (IDXGISwapChain3)      ||
    riid == __uuidof (IDXGISwapChain4) )
  {
    auto _GetVersion = [](REFIID riid) ->
    UINT
    {
      if (riid == __uuidof (IDXGISwapChain))  return 0;
      if (riid == __uuidof (IDXGISwapChain1)) return 1;
      if (riid == __uuidof (IDXGISwapChain2)) return 2;
      if (riid == __uuidof (IDXGISwapChain3)) return 3;
      if (riid == __uuidof (IDXGISwapChain4)) return 4;

      return 0;
    };

    UINT required_ver =
      _GetVersion (riid);

    if (ver_ < required_ver)
    {
      IUnknown* pPromoted = nullptr;

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

    else
    {
      AddRef ();
    }

    *ppvObj = this;

    return S_OK;
  }


#if 0
  static IID IID_Mystery0;
  static IID IID_Mystery1;

  SK_RunOnce (IIDFromString (L"{10B90151-4435-4004-9FAD-19361488899A}", &IID_Mystery0));
  SK_RunOnce (IIDFromString (L"{AABDF0C6-6A76-4F65-987D-F2CC4C27ED0E}", &IID_Mystery1));

  if (InlineIsEqualGUID (IID_Mystery0, riid) ||
      InlineIsEqualGUID (IID_Mystery1, riid))
  {
    wchar_t                wszGUID [41] = { };
    StringFromGUID2 (riid, wszGUID, 40);

    SK_LOGi0 (L"Disabling unknown interface: %ws", wszGUID);

    return E_NOINTERFACE;
  }
#endif


  HRESULT hr =
    pReal->QueryInterface (riid, ppvObj);

  if ( riid != IID_IUnknown &&
       riid != IID_ID3DUserDefinedAnnotation )
  {
    static
      concurrency::concurrent_unordered_set <std::wstring> reported_guids;

    wchar_t                wszGUID [41] = { };
    StringFromGUID2 (riid, wszGUID, 40);

    bool once =
      reported_guids.count (wszGUID) > 0;

    if (! once)
    {
      reported_guids.insert (wszGUID);

      SK_LOG0 ( ( L"QueryInterface on wrapped DXGI SwapChain for Mystery UUID: %s",
                      wszGUID ), L"   DXGI   " );
    }
  }

  return hr;
}

ULONG
STDMETHODCALLTYPE
IWrapDXGISwapChain::AddRef (void)
{
  ULONG xrefs =
    InterlockedIncrement (&refs_);

  ULONG refs =
    pReal->AddRef ();

  SK_LOGi3 (
    L"IWrapDXGISwapChain::AddRef (...): "
    L"External=%lu, Actual=%lu",
      xrefs, refs );

  return
    refs;
}

ULONG
STDMETHODCALLTYPE
SK_SEH_ReleaseObject (IUnknown *pUnknown)
{
  __try {
    return
      pUnknown->Release ();
  }

  __except (EXCEPTION_EXECUTE_HANDLER)
  {
    return 0;
  }
}

ULONG
STDMETHODCALLTYPE
IWrapDXGISwapChain::Release (void)
{
  ULONG xrefs =
    InterlockedDecrement (&refs_);

  if (xrefs == 1)
  {
    HWND hWndSwap = HWND_BROADCAST;

    if (ver_ > 0)
      static_cast <IDXGISwapChain1*> (pReal)->GetHwnd (&hWndSwap);

    // Ignore this for stuff like VLC, it's not important
    if (config.system.log_level > 0)
      SK_ReleaseAssert (hWnd_ == hWndSwap);

    SetPrivateDataInterface (IID_IUnwrappedDXGISwapChain, nullptr);
  }

  HWND hwnd = hWnd_;

  // External references are now 0... game expects SwapChain destruction.
  //
  if (xrefs == 0)
  {
    // We're going to make this available for recycling
    if (hWnd_ != 0)
    {
      SK_DXGI_ReleaseSwapChainOnHWnd (
        this, std::exchange (hWnd_, (HWND)0), pDev
      );
    }

    auto& rb =
      SK_GetCurrentRenderBackend ();

    _d3d12_rbk->drain_queue ();

    //  If this SwapChain is the primary one that SK is rendering to,
    //    then we must teardown our render backend to eliminate internal
    //      references preventing the SwapChain from being destroyed.
    if ( rb.swapchain.p == this ||
         rb.swapchain.p == pReal )
    {
      // This is a hard reset, we're going to get a new cmd queue
      _d3d11_rbk->release (_d3d11_rbk->_pSwapChain);
      _d3d12_rbk->release (_d3d12_rbk->_pSwapChain);
      _d3d12_rbk->_pCommandQueue.Release ();

      rb.releaseOwnedResources ();
    }
  }

  ULONG refs =
    pReal->Release ();


  SK_LOGi3 (
    L"IWrapDXGISwapChain::Release (...): "
    L"External=%lu, Actual=%lu",
     xrefs, refs );

  if (xrefs == 0 && refs != 0)
  {
    SK_LOGi0 (
      L"IWrapDXGISwapChain::Release (...) should have returned 0, "
      L"but %lu references remain...", refs
    );
  }


  if (refs == 0)
  {
    SK_LOG0 ( ( L"(-) Releasing wrapped SwapChain (%08ph)... device=%08ph, hwnd=%08ph", pReal, pDev.p, hwnd),
             L"Swap Chain");

    if (ver_ > 0)
      InterlockedDecrement (&SK_DXGI_LiveWrappedSwapChain1s);
    else
      InterlockedDecrement (&SK_DXGI_LiveWrappedSwapChains);

    pDev.Release ();

    if ((! _d3d12_rbk->frames_.empty ()) ||
           _d3d12_rbk->_pCommandQueue.p != nullptr)
    {
      SK_LOGi0 (
        L"Clearing ImGui D3D12 Command Queue because SwapChain using it "
        L"was destroyed..." );

      // Finish up any queued presents, then re-initialize the command queue
      _d3d12_rbk->release (_d3d12_rbk->_pSwapChain);
      _d3d12_rbk->_pCommandQueue.Release ();

      auto& rb =
        SK_GetCurrentRenderBackend ();

      rb.releaseOwnedResources ();
    }

    //delete this;
  }

  else if (hWnd_ == 0)
  {
    if (ver_ > 0)
      static_cast <IDXGISwapChain1*> (pReal)->GetHwnd (&hWnd_);
  }

  return refs;
}

HRESULT
STDMETHODCALLTYPE
IWrapDXGISwapChain::SetPrivateData (REFGUID Name, UINT DataSize, const void *pData)
{
  if (Name == SKID_DXGI_SwapChainSkipBackbufferCopy_D3D11)
  {
  }

  return
    pReal->SetPrivateData (Name, DataSize, pData);
}

HRESULT
STDMETHODCALLTYPE
IWrapDXGISwapChain::SetPrivateDataInterface (REFGUID Name, const IUnknown *pUnknown)
{
  return
    pReal->SetPrivateDataInterface (Name, pUnknown);
}

HRESULT
STDMETHODCALLTYPE IWrapDXGISwapChain::GetPrivateData (REFGUID Name, UINT *pDataSize, void *pData)
{
  std::scoped_lock lock (_backbufferLock);

  if (IsEqualGUID (Name, SKID_DXGI_SwapChainProxyBackbuffer_D3D11) && _backbuffers.contains (0))
  {
    if (SK_ComQIPtr <ID3D11Device> pDev11 (pDev); pDev11.p  != nullptr &&
                                                  pDataSize != nullptr)
    {
      if (pData != nullptr && *pDataSize >= sizeof (void *))
      {
        D3D11_TEXTURE2D_DESC        texDesc = { };
        _backbuffers [0]->GetDesc (&texDesc);

        // Multisampled backbuffers require an extra resolve, we can't do this.
        if (texDesc.SampleDesc.Count == 1)
        {
          auto typed_format =
              DirectX::MakeTypelessUNORM (
              DirectX::MakeTypelessFLOAT (texDesc.Format));

          D3D11_SHADER_RESOURCE_VIEW_DESC
            srvDesc                     = { };
            srvDesc.Format              = typed_format;
            srvDesc.ViewDimension       = D3D_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = 1;

          SK_ComPtr <ID3D11ShaderResourceView>                           pSRV;
          pDev11->CreateShaderResourceView (_backbuffers [0], &srvDesc, &pSRV.p);

          if (pSRV.p != nullptr)
          {
                                    pSRV.p->AddRef ();
            memcpy (pData, (void *)&pSRV.p, sizeof (void *));
            return S_OK;
          }
        }
      }

      if (pData == nullptr)
      {
        *pDataSize = sizeof (void *);
        return S_OK;
      }
    }
  }

  else if (IsEqualGUID (Name, SKID_DXGI_SwapChainRealBackbuffer_D3D11))
  {
    if (SK_ComQIPtr <ID3D11Device> pDev11 (pDev); pDev11.p  != nullptr &&
                                                  pDataSize != nullptr)
    {
      if (pData != nullptr && *pDataSize >= sizeof (void *))
      {
        SK_ComPtr <ID3D11Texture2D>                         pBackBuffer;
        pReal->GetBuffer (0, IID_ID3D11Texture2D, (void **)&pBackBuffer.p);

        if (pBackBuffer.p != nullptr)
        {
          D3D11_TEXTURE2D_DESC   texDesc = { };
          pBackBuffer->GetDesc (&texDesc);

          auto typed_format =
            DirectX::MakeTypelessUNORM (
            DirectX::MakeTypelessFLOAT (texDesc.Format));

          D3D11_RENDER_TARGET_VIEW_DESC
            rtvDesc               = { };
            rtvDesc.Format        = typed_format;
            rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

          SK_ComPtr <ID3D11RenderTargetView>                      pRTV;
          pDev11->CreateRenderTargetView (pBackBuffer, &rtvDesc, &pRTV.p);

          if (pRTV.p != nullptr)
          {
                                    pRTV.p->AddRef ();
            memcpy (pData, (void *)&pRTV.p, sizeof (void *));
            return S_OK;
          }
        }
      }

      if (pData == nullptr)
      {
        *pDataSize = sizeof (void *);
        return S_OK;
      }
    }
  }

  if (pDataSize == nullptr || pData == nullptr)
    return E_INVALIDARG;

  return
    pReal->GetPrivateData (Name, pDataSize, pData);
}

HRESULT
STDMETHODCALLTYPE
IWrapDXGISwapChain::GetParent (REFIID riid, void **ppParent)
{
  return
    pReal->GetParent (riid, ppParent);
}

HRESULT
STDMETHODCALLTYPE
IWrapDXGISwapChain::GetDevice (REFIID riid, void **ppDevice)
{
  SK_ReleaseAssert (pDev.p != nullptr);

  if (pReal != nullptr)
  {
    return
      pReal->GetDevice (riid, ppDevice);
  }

  if (pDev.p != nullptr)
  {
    return
      pDev->QueryInterface (riid, ppDevice);
  }

  return E_NOINTERFACE;
}

// Special optimization for HDR
BOOL SK_DXGI_ZeroCopy = -1;

int
IWrapDXGISwapChain::PresentBase (void)
{
  if (SK_DXGI_ZeroCopy == -1)
      SK_DXGI_ZeroCopy = (__SK_HDR_16BitSwap || __SK_HDR_10BitSwap);

  BOOL bSkipCopy = FALSE;

  SK_DXGI_GetPrivateData ( pReal,
    SKID_DXGI_SwapChainSkipBackbufferCopy_D3D11, sizeof (BOOL), &bSkipCopy
  );

  // D3D12 is already Flip Model and doesn't need this
  if ((flip_model.isOverrideActive () || SK_DXGI_ZeroCopy == TRUE) && (! d3d12_) && (! bSkipCopy))
  {
    SK_ComQIPtr <ID3D11Device>
        pD3D11Dev (pDev);
    if (pD3D11Dev.p != nullptr)
    {
      SK_ComPtr <ID3D11DeviceContext>    pDevCtx;
      pD3D11Dev.p->GetImmediateContext (&pDevCtx);

      if (pDevCtx.p != nullptr)
      {
        std::scoped_lock lock (_backbufferLock);

        std::pair <BOOL*, BOOL>
          SK_ImGui_FlagDrawing_OnD3D11Ctx (UINT dev_idx);

        auto flag_result =
          SK_ImGui_FlagDrawing_OnD3D11Ctx (
            SK_D3D11_GetDeviceContextHandle (pDevCtx)
          );

        SK_ScopedBool auto_bool2 (flag_result.first);
                                 *flag_result.first = flag_result.second;

        SK_ComPtr               <ID3D11Texture2D>           pBackbuffer;
        pReal->GetBuffer (0, IID_ID3D11Texture2D, (void **)&pBackbuffer.p);

        if (pBackbuffer.p != nullptr)
        {
          D3D11_TEXTURE2D_DESC   bufferDesc = { };
          pBackbuffer->GetDesc (&bufferDesc);

          if (_backbuffers.contains (0) &&
              _backbuffers          [0].p != nullptr)
          {
            D3D11_TEXTURE2D_DESC        texDesc = { };
            _backbuffers [0]->GetDesc (&texDesc);

            // XXX: What about sRGB?
            auto typed_format =
              DirectX::MakeTypelessUNORM (
              DirectX::MakeTypelessFLOAT (texDesc.Format));

            if ( texDesc.Width  == bufferDesc.Width &&
                 texDesc.Height == bufferDesc.Height )
            {
              if (texDesc.SampleDesc.Count > 1)
                pDevCtx->ResolveSubresource (pBackbuffer, 0, _backbuffers [0].p, 0, typed_format);
              else
              {
                if (_d3d11_feature_opts.DiscardAPIsSeenByDriver)
                {
                  SK_ComQIPtr <ID3D11DeviceContext1>
                      pDevCtx1 (pDevCtx);
                  if (pDevCtx1.p != nullptr)
                      pDevCtx1->DiscardResource (pBackbuffer);
                }

                pDevCtx->CopyResource (pBackbuffer, _backbuffers [0].p);
              }
            }

            else
            {
              SK_D3D11_BltCopySurface (
                _backbuffers [0],
                pBackbuffer
              );
            }

            if (config.render.framerate.flip_discard && _d3d11_feature_opts.DiscardAPIsSeenByDriver)
            {
              SK_ComQIPtr <ID3D11DeviceContext1>
                  pDevCtx1 (pDevCtx);
              if (pDevCtx1.p != nullptr)
                  pDevCtx1->DiscardResource (_backbuffers [0].p);
            }
          }

          else
          {
            return 0;
          }
        }
      }
    }
  }

  bSkipCopy = FALSE;

  SK_DXGI_SetPrivateData ( pReal,
    SKID_DXGI_SwapChainSkipBackbufferCopy_D3D11, sizeof (BOOL), &bSkipCopy
  );

  return -1;
}

HRESULT
STDMETHODCALLTYPE
IWrapDXGISwapChain::Present (UINT SyncInterval, UINT Flags)
{
  if (0 == PresentBase ())
  {
    SyncInterval = 0;
  }

  return
    SK_DXGI_DispatchPresent ( pReal, SyncInterval, Flags,
                                nullptr, SK_DXGI_PresentSource::Wrapper );
}

HRESULT
STDMETHODCALLTYPE
IWrapDXGISwapChain::GetBuffer (UINT Buffer, REFIID riid, void **ppSurface)
{
  auto& rb =
    SK_GetCurrentRenderBackend ();

  if (SK_DXGI_ZeroCopy == -1)
      SK_DXGI_ZeroCopy = (__SK_HDR_16BitSwap || __SK_HDR_10BitSwap);

  // D3D12 is already Flip Model and doesn't need this
  if ((flip_model.isOverrideActive () || SK_DXGI_ZeroCopy == TRUE) && (! d3d12_))
  {
    // MGS V Compatibility
    Buffer = 0;

    //SK_ReleaseAssert (Buffer == 0);

    if ( riid != IID_ID3D11Texture2D  &&
         riid != IID_ID3D11Texture2D1 &&
         riid != IID_ID3D11Resource )
    {
      SK_RunOnce (
      {
        wchar_t                wszGUID [41] = { };
        StringFromGUID2 (riid, wszGUID, 40);

        SK_LOGi0 ( L"IDXGISwapChain::GetBuffer (...) called with unexpected GUID: %ws",
                     wszGUID );
      });

      if (config.system.log_level > 0)
      {
        SK_ReleaseAssert ( riid == IID_ID3D11Texture2D  ||
                           riid == IID_ID3D11Texture2D1 ||
                           riid == IID_ID3D11Resource );
      }
    }

    SK_ComPtr <IUnknown>                       pUnk;
    pReal->GetBuffer (Buffer, riid, (void **) &pUnk.p);

    SK_ComQIPtr <ID3D11Resource> pBuffer11 (pUnk);
    if (                         pBuffer11 != nullptr)
    {
      SK_LOGi1 (L"GetBuffer (%d)", Buffer);

      std::scoped_lock lock (_backbufferLock);

      D3D11_TEXTURE2D_DESC texDesc  = { };
      DXGI_SWAP_CHAIN_DESC swapDesc = { };
      pReal->GetDesc     (&swapDesc);

      const bool contains =
        _backbuffers.contains (Buffer) &&
        _backbuffers          [Buffer].p != nullptr;

      if (contains)
        _backbuffers [Buffer]->GetDesc (&texDesc);

      if ( contains &&
             texDesc.Width  == swapDesc.BufferDesc.Width  &&
             texDesc.Height == swapDesc.BufferDesc.Height &&
             DirectX::MakeTypeless (            texDesc.Format) ==
             DirectX::MakeTypeless (swapDesc.BufferDesc.Format) &&
             std::exchange (flip_model.last_srgb_mode, config.render.dxgi.srgb_behavior) ==
                                                       config.render.dxgi.srgb_behavior )
      {
        return
          _backbuffers [Buffer]->QueryInterface (riid, ppSurface);
      }


      else
      {
        SK_ComPtr   <ID3D11Texture2D> backbuffer;
        SK_ComQIPtr <ID3D11Device>    pDev11 (pDev);

        if (pDev11.p != nullptr)
        {
          constexpr UINT size =
                 sizeof (DXGI_FORMAT);

          texDesc                    = { };
          texDesc.Width              = swapDesc.BufferDesc.Width;
          texDesc.Height             = swapDesc.BufferDesc.Height;

          auto typeless =
            DirectX::MakeTypeless (swapDesc.BufferDesc.Format);

          auto scrgb_hdr =
            swapDesc.BufferDesc.Format == DXGI_FORMAT_R16G16B16A16_FLOAT;

          // sRGB is being turned off for Flip Model
          if (rb.active_traits.bOriginallysRGB && (! scrgb_hdr))
          {
            if (config.render.dxgi.srgb_behavior >= 1)
              texDesc.Format =
                  DirectX::MakeSRGB (DirectX::MakeTypelessUNORM (typeless));
            else
              texDesc.Format = typeless;

          }

          // HDR Override: Firm override to a typed format is safe
          else if (scrgb_hdr)
          {
            texDesc.Format = swapDesc.BufferDesc.Format;
          }

          // No Overrides: SRV/RTVs on the -real- SwapChain would be type castable,
          //                 we need to simulate that
          else
            texDesc.Format = typeless;

          texDesc.ArraySize          = 1;
          texDesc.SampleDesc.Count   = config.render.dxgi.msaa_samples > 0 ?
                                           config.render.dxgi.msaa_samples : rb.active_traits.uiOriginalBltSampleCount;
          texDesc.SampleDesc.Quality = 0;
          texDesc.MipLevels          = 1;
          texDesc.Usage              = D3D11_USAGE_DEFAULT;
          texDesc.BindFlags          = 0x0; // To be filled in below

          const bool bWrongBindFlags =
            (texDesc.BindFlags & D3D11_BIND_SHADER_RESOURCE) == 0 ||
            (texDesc.BindFlags & D3D11_BIND_RENDER_TARGET)   == 0;

          // TODO: Pass this during wrapping
          //   [Or use IDXGIResource::GetUsage (...)
          if ((swapDesc.BufferUsage & DXGI_USAGE_RENDER_TARGET_OUTPUT) || bWrongBindFlags)
               texDesc.BindFlags   |= D3D11_BIND_RENDER_TARGET;

          if ((swapDesc.BufferUsage & DXGI_USAGE_SHADER_INPUT)         || bWrongBindFlags)
              texDesc.BindFlags    |= D3D11_BIND_SHADER_RESOURCE;

          // MSAA SwapChains can't use UA
          if (texDesc.SampleDesc.Count <= 1)
          {
          //if (swapDesc.BufferUsage & DXGI_USAGE_UNORDERED_ACCESS)
                texDesc.BindFlags   |= D3D11_BIND_UNORDERED_ACCESS;
          }

          // Allow GL/DXGI interop
          texDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;

          if (SUCCEEDED (pDev11->CreateTexture2D (&texDesc, nullptr, &backbuffer.p)))
          {
            backbuffer->SetPrivateData (
              SKID_DXGI_SwapChainBackbufferFormat, size,
                                 &swapDesc.BufferDesc.Format);

            // Set the underlying resource type as the format, unless using HDR...
            //   in which case all access to the REAL backbuffer is handled by a
            //     copy operation at the end of every frame.
            SK_D3D11_FlagResourceFormatManipulated (
              backbuffer, (__SK_HDR_10BitSwap || __SK_HDR_16BitSwap) ? swapDesc.BufferDesc.Format
                                                                     : texDesc.Format
                                                      
            );

            SK_D3D11_SetDebugName ( backbuffer.p,
                 SK_FormatStringW ( L"[SK] Flip Model Backbuffer %d", Buffer ) );
            SK_LOGi1 (L"_backbuffers [%d] = New ( %dx%d [Samples: %d] %hs )",
                          Buffer, swapDesc.BufferDesc.Width,
                                  swapDesc.BufferDesc.Height,
                                   texDesc.SampleDesc.Count,
             SK_DXGI_FormatToStr (swapDesc.BufferDesc.Format).data ());

            if (_backbuffers [Buffer].p != nullptr)
                _backbuffers [Buffer].p->Release ();

            std::swap (
               backbuffer,
              _backbuffers [Buffer]
            );

            _backbuffers [Buffer].p->AddRef ();

            return
              _backbuffers [Buffer]->QueryInterface (riid, ppSurface);
          }
        }
      }
    }
  }

  return
    pReal->GetBuffer (Buffer, riid, ppSurface);
}

UINT
SK_DXGI_FixUpLatencyWaitFlag (
                gsl::not_null <IDXGISwapChain*> pSwapChain,
                               UINT             Flags,
                               BOOL             bCreation = FALSE )
{
  // This flag can only be assigned at the time of swapchain creation,
  //   if it's not present in the swapchain's description, its too late.

  DXGI_SWAP_CHAIN_DESC  desc = { };
  pSwapChain->GetDesc (&desc);

  if ( (desc.Flags &  DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT)
                  ==  DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT ||
                (bCreation && config.render.framerate.swapchain_wait > 0)
     )       Flags |=  DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
  else       Flags &= ~DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

  if (_NO_ALLOW_MODE_SWITCH)
            Flags &= ~DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

  return    Flags;
}

HRESULT
STDMETHODCALLTYPE
IWrapDXGISwapChain::SetFullscreenState (BOOL Fullscreen, IDXGIOutput *pTarget)
{
  HRESULT hr =
    SK_DXGI_SwapChain_SetFullscreenState_Impl (
      pReal, Fullscreen, pTarget, TRUE
    );

  if (SUCCEEDED (hr))
  {
    state_cache_s                                  state_cache;
    SK_DXGI_GetPrivateData <state_cache_s> (this, &state_cache);

    state_cache.lastFullscreenState = Fullscreen;
    state_cache._fakefullscreen     =
      (config.render.dxgi.fake_fullscreen_mode && Fullscreen);

    SK_DXGI_SetPrivateData <state_cache_s> (this, &state_cache);
  }

  return hr;
}

HRESULT
STDMETHODCALLTYPE
IWrapDXGISwapChain::GetFullscreenState (BOOL *pFullscreen, IDXGIOutput **ppTarget)
{
  HRESULT hr =
    pReal->GetFullscreenState (pFullscreen, ppTarget);

  if (SUCCEEDED (hr) && pFullscreen != nullptr)
  {
    if (config.render.dxgi.fake_fullscreen_mode)
    {
      state_cache_s                                   state_cache;
      SK_DXGI_GetPrivateData <state_cache_s> (pReal, &state_cache);

      *pFullscreen = state_cache.lastFullscreenState;
    }
  }

  return hr;
}

HRESULT
STDMETHODCALLTYPE
IWrapDXGISwapChain::GetDesc (DXGI_SWAP_CHAIN_DESC *pDesc)
{
  HRESULT hr =
    pReal->GetDesc (pDesc);

  // Potentially override the Sample Count if behind the scenes we are
  //   handling MSAA resolve for a Flip Model SwapChain
  if (SUCCEEDED (hr))
  {
    std::scoped_lock lock (_backbufferLock);

    if ( (! _backbuffers.empty ()) &&
            _backbuffers [0].p != nullptr )
    {
      D3D11_TEXTURE2D_DESC        texDesc = { };
      _backbuffers [0]->GetDesc (&texDesc);

      pDesc->SampleDesc.Count   = texDesc.SampleDesc.Count;
      pDesc->SampleDesc.Quality = texDesc.SampleDesc.Quality;
    }

    if (config.render.dxgi.fake_fullscreen_mode)
    {
      state_cache_s                                   state_cache;
      SK_DXGI_GetPrivateData <state_cache_s> (pReal, &state_cache);

      pDesc->Windowed = !state_cache.lastFullscreenState;
    }
  }

  return hr;
}

HRESULT
STDMETHODCALLTYPE
IWrapDXGISwapChain::ResizeBuffers ( UINT        BufferCount,
                                    UINT        Width,     UINT Height,
                                    DXGI_FORMAT NewFormat, UINT SwapChainFlags )
{
  if (SK_DXGI_ZeroCopy == -1)
      SK_DXGI_ZeroCopy = (__SK_HDR_16BitSwap || __SK_HDR_10BitSwap);

  state_cache_s                                   state_cache = { };
  SK_DXGI_GetPrivateData <state_cache_s> (pReal, &state_cache);

  DXGI_SWAP_CHAIN_DESC swapDesc = { };
  GetDesc            (&swapDesc);

  HRESULT hr =
    pReal->ResizeBuffers (BufferCount, Width, Height, NewFormat, SwapChainFlags);
    //SK_DXGI_SwapChain_ResizeBuffers_Impl ( pReal, BufferCount, Width, Height,
    //                                         NewFormat, SwapChainFlags, TRUE );

  if (SUCCEEDED (hr))
  {
    state_cache._stalebuffers = false;

    // D3D12 is already Flip Model and doesn't need this
    if ((flip_model.isOverrideActive () || SK_DXGI_ZeroCopy == TRUE) && (! d3d12_))
    {
      std::scoped_lock lock (_backbufferLock);

      if (! _backbuffers.empty ())
      {
        SK_LOGi1 ( L"ResizeBuffers [_backbuffers=%d]",
                                    _backbuffers.size () );

        for ( auto &[slot, backbuffer] : _backbuffers )
        {
          if (backbuffer.p != nullptr)
          {
            D3D11_TEXTURE2D_DESC  texDesc = { };
            backbuffer->GetDesc (&texDesc);

            if ( (texDesc.Width  == Width     || Width     == 0) &&
                 (texDesc.Height == Height    || Height    == 0) &&
                 (texDesc.Format == NewFormat || NewFormat == DXGI_FORMAT_UNKNOWN) )
            {
              SK_LOGi1 (L"ResizeBuffers => NOP");
              continue;
            }

            else
            {
              SK_LOGi1 (L"ResizeBuffers => Remove");
              backbuffer.Release ();
            }
          }
        }

        bool clear = true;

        for ( auto &[slot, backbuffer] : _backbuffers )
        {
          if (backbuffer.p != nullptr)
          {
            clear = false;
            break;
          }
        }

        if (clear)
        {
          SK_LOGi1 (L"ResizeBuffers => Clear");
          _backbuffers.clear ();
        }
      }
    }


    if (Height != 0)
    gameHeight_ =
        Height;

    if (Width != 0)
    gameWidth_ =
        Width;
  }

  else
  {
    if ( config.render.framerate.flip_discard &&
         config.window.res.override.x != 0    &&
         config.window.res.override.y != 0 ) return S_OK;
  }

  SK_DXGI_SetPrivateData <state_cache_s> (pReal, &state_cache);

  return hr;
}

HRESULT
STDMETHODCALLTYPE
IWrapDXGISwapChain::ResizeTarget (const DXGI_MODE_DESC *pNewTargetParameters)
{
  return
    pReal->ResizeTarget (pNewTargetParameters);
    //SK_DXGI_SwapChain_ResizeTarget_Impl (
    //  pReal, pNewTargetParameters, TRUE
    //);
}

HRESULT
STDMETHODCALLTYPE
IWrapDXGISwapChain::GetContainingOutput (IDXGIOutput **ppOutput)
{
  return
    pReal->GetContainingOutput (ppOutput);
}

HRESULT
STDMETHODCALLTYPE
IWrapDXGISwapChain::GetFrameStatistics (DXGI_FRAME_STATISTICS *pStats)
{
  return
    pReal->GetFrameStatistics (pStats);
}

HRESULT
STDMETHODCALLTYPE
IWrapDXGISwapChain::GetLastPresentCount (UINT *pLastPresentCount)
{
  return
    pReal->GetLastPresentCount (pLastPresentCount);
}

// IDXGISwapChain1
HRESULT
STDMETHODCALLTYPE
IWrapDXGISwapChain::GetDesc1 (DXGI_SWAP_CHAIN_DESC1 *pDesc)
{
  assert (ver_ >= 1);

  HRESULT hr =
    static_cast <IDXGISwapChain1 *>(pReal)->GetDesc1 (pDesc);

  if (SUCCEEDED (hr))
  {
    // Potentially override the Sample Count if behind the scenes we are
    //   handling MSAA resolve for a Flip Model SwapChain
    if (SUCCEEDED (hr))
    {
      std::scoped_lock lock (_backbufferLock);

      if ( (! _backbuffers.empty ()) &&
              _backbuffers [0].p != nullptr )
      {
        D3D11_TEXTURE2D_DESC        texDesc = { };
        _backbuffers [0]->GetDesc (&texDesc);

        pDesc->SampleDesc.Count   = texDesc.SampleDesc.Count;
        pDesc->SampleDesc.Quality = texDesc.SampleDesc.Quality;
      }
    }
  }

  return hr;
}

HRESULT
STDMETHODCALLTYPE
IWrapDXGISwapChain::GetFullscreenDesc (DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pDesc)
{
  assert (ver_ >= 1);

  HRESULT hr =
    static_cast <IDXGISwapChain1 *>(pReal)->GetFullscreenDesc (pDesc);

  return hr;
}

HRESULT
STDMETHODCALLTYPE
IWrapDXGISwapChain::GetHwnd (HWND *pHwnd)
{
  assert (ver_ >= 1);

  if (IsWindow (hWnd_))
  {
    if (pHwnd != nullptr)
       *pHwnd  = hWnd_;

    return
      S_OK;
  }

  return
    ((IDXGISwapChain1 *)pReal)->GetHwnd (pHwnd);
}

HRESULT
STDMETHODCALLTYPE
IWrapDXGISwapChain::GetCoreWindow (REFIID refiid, void **ppUnk)
{
  assert (ver_ >= 1);

  return
    static_cast <IDXGISwapChain1 *>(pReal)->GetCoreWindow (refiid, ppUnk);
}

HRESULT
STDMETHODCALLTYPE
IWrapDXGISwapChain::Present1 ( UINT                     SyncInterval,
                               UINT                     PresentFlags,
                         const DXGI_PRESENT_PARAMETERS *pPresentParameters )
{
  assert (ver_ >= 1);

  // Almost never used by anything, so log it if it happens.
  SK_LOG_ONCE (L"Present1 ({Wrapped SwapChain})");

  if (0 == PresentBase ())
  {
    SyncInterval = 0;
  }

  return
    SK_DXGI_DispatchPresent1 ( (IDXGISwapChain1 *)pReal, SyncInterval, PresentFlags, pPresentParameters,
                               nullptr, SK_DXGI_PresentSource::Wrapper );
}

BOOL
STDMETHODCALLTYPE
IWrapDXGISwapChain::IsTemporaryMonoSupported (void)
{
  assert (ver_ >= 1);

  return
    static_cast <IDXGISwapChain1 *>(pReal)->IsTemporaryMonoSupported ();
}

HRESULT
STDMETHODCALLTYPE
IWrapDXGISwapChain::GetRestrictToOutput (IDXGIOutput **ppRestrictToOutput)
{
  assert (ver_ >= 1);

  return
    static_cast <IDXGISwapChain1 *>(pReal)->GetRestrictToOutput (ppRestrictToOutput);
}

HRESULT
STDMETHODCALLTYPE
IWrapDXGISwapChain::SetBackgroundColor (const DXGI_RGBA *pColor)
{
  assert (ver_ >= 1);

  return
    static_cast <IDXGISwapChain1 *>(pReal)->SetBackgroundColor (pColor);
}

HRESULT
STDMETHODCALLTYPE
IWrapDXGISwapChain::GetBackgroundColor (DXGI_RGBA *pColor)
{
  assert (ver_ >= 1);

  return
    static_cast <IDXGISwapChain1 *>(pReal)->GetBackgroundColor (pColor);
}

HRESULT
STDMETHODCALLTYPE
IWrapDXGISwapChain::SetRotation (DXGI_MODE_ROTATION Rotation)
{
  assert (ver_ >= 1);

  return
    static_cast <IDXGISwapChain1 *>(pReal)->SetRotation (Rotation);
}

HRESULT
STDMETHODCALLTYPE
IWrapDXGISwapChain::GetRotation (DXGI_MODE_ROTATION *pRotation)
{
  assert (ver_ >= 1);

  return
    static_cast <IDXGISwapChain1 *>(pReal)->GetRotation (pRotation);
}

// IDXGISwapChain2
HRESULT
STDMETHODCALLTYPE
IWrapDXGISwapChain::SetSourceSize ( UINT Width,
                                    UINT Height )
{
  SK_LOG0 ( ( L"IDXGISwapChain2::SetSourceSize (%u, %u)", Width, Height ),
              L"   DXGI   " );

  assert (ver_ >= 2);

  return
    static_cast <IDXGISwapChain2 *>(pReal)->SetSourceSize (Width, Height);
}

HRESULT
STDMETHODCALLTYPE
IWrapDXGISwapChain::GetSourceSize (UINT *pWidth, UINT *pHeight)
{
  assert (ver_ >= 2);

  return
    static_cast <IDXGISwapChain2 *>(pReal)->GetSourceSize (pWidth, pHeight);
}

HRESULT
STDMETHODCALLTYPE
IWrapDXGISwapChain::SetMaximumFrameLatency (UINT MaxLatency)
{
  assert (ver_ >= 2);

  SK_LOG_FIRST_CALL

  HRESULT hr = E_UNEXPECTED;

  if (config.render.framerate.pre_render_limit > 0)
    hr = S_OK;
  else
    hr = static_cast <IDXGISwapChain2 *>(pReal)->SetMaximumFrameLatency (MaxLatency);

  if (SUCCEEDED (hr))
  {
    gameFrameLatency_ = MaxLatency;

    SK_LOGi0 (
      L"IWrapDXGISwapChain::SetMaximumFrameLatency ({%d Frames})",
        MaxLatency
    );

    if (config.render.framerate.pre_render_limit > 0)
    {
      MaxLatency =         // 16 is valid, but Reflex hates that! :)
        std::clamp (config.render.framerate.pre_render_limit, 1, 14);

      SK_LOGi0 (L" >> Override=%d Frames", MaxLatency);

      hr =
        static_cast <IDXGISwapChain2 *>(pReal)->SetMaximumFrameLatency (MaxLatency);
    }
  }

  return hr;
}

HRESULT
STDMETHODCALLTYPE
IWrapDXGISwapChain::GetMaximumFrameLatency (UINT *pMaxLatency)
{
  assert (ver_ >= 2);

  SK_LOG_FIRST_CALL

  HRESULT hr = S_OK;

  // We are overriding, do not let the game know...
  if (config.render.framerate.pre_render_limit > 0)
  {
    *pMaxLatency = gameFrameLatency_;
  }

  else
  {
    hr =
      static_cast <IDXGISwapChain2 *>(pReal)->GetMaximumFrameLatency (pMaxLatency);
  }

  if (SUCCEEDED (hr))
  {
    SK_LOGi0 (
      L"IWrapDXGISwapChain::GetMaximumFrameLatency ({%d Frames})",
        pMaxLatency != nullptr ? *pMaxLatency : 0
    );

    if (config.render.framerate.pre_render_limit > 0)
    {
      UINT RealMaxLatency = 0;

      static_cast <IDXGISwapChain2 *>(pReal)->GetMaximumFrameLatency (
                                                     &RealMaxLatency );

      SK_LOGi0 (
        L" >> Actual SwapChain Maximum Frame Latency: %d Frames",
          RealMaxLatency
      );
    }
  }

  return hr;
}

HANDLE
STDMETHODCALLTYPE
IWrapDXGISwapChain::GetFrameLatencyWaitableObject (void)
{
  assert(ver_ >= 2);

  SK_LOG_FIRST_CALL

#if 1
  if (config.render.framerate.pre_render_limit > 0)
  {
    // 16 is valid, but Reflex hates that! :)
    static_cast <IDXGISwapChain2 *>(pReal)->SetMaximumFrameLatency (
      std::clamp (config.render.framerate.pre_render_limit, 1, 14) );
  }

  // Disable waitable SwapChains when HW Flip Queue is active, they don't work right...
  if (false)//rb.windows.unity || rb.windows.unreal)// || rb.displays [rb.active_display].wddm_caps._3_0.HwFlipQueueEnabled)
  {
    static HANDLE fake_waitable =
      SK_CreateEvent (nullptr, TRUE, TRUE, nullptr);

    if ( SetHandleInformation ( fake_waitable, HANDLE_FLAG_PROTECT_FROM_CLOSE,
                                               HANDLE_FLAG_PROTECT_FROM_CLOSE ) )
    {
      SetEvent (fake_waitable);
      return    fake_waitable;
    }

    else
    {
      return
        SK_CreateEvent (nullptr, TRUE, TRUE, nullptr);
    }
  }
#endif

  return
    static_cast <IDXGISwapChain2 *>(pReal)->GetFrameLatencyWaitableObject ();
}
HRESULT
STDMETHODCALLTYPE
IWrapDXGISwapChain::SetMatrixTransform (const DXGI_MATRIX_3X2_F *pMatrix)
{
  assert(ver_ >= 2);

  return
    static_cast <IDXGISwapChain2 *>(pReal)->SetMatrixTransform (pMatrix);
}
HRESULT
STDMETHODCALLTYPE
IWrapDXGISwapChain::GetMatrixTransform (DXGI_MATRIX_3X2_F *pMatrix)
{
  assert (ver_ >= 2);

  return
    static_cast <IDXGISwapChain2 *>(pReal)->GetMatrixTransform (pMatrix);
}

// IDXGISwapChain3
UINT
STDMETHODCALLTYPE
IWrapDXGISwapChain::GetCurrentBackBufferIndex (void)
{
  assert (ver_ >= 3);

  return
    static_cast <IDXGISwapChain3 *>(pReal)->GetCurrentBackBufferIndex ();
}

HRESULT
STDMETHODCALLTYPE
IWrapDXGISwapChain::CheckColorSpaceSupport ( DXGI_COLOR_SPACE_TYPE  ColorSpace,
                                                              UINT *pColorSpaceSupport )
{
  assert (ver_ >= 3);

  return
    static_cast <IDXGISwapChain3 *>(pReal)->
      CheckColorSpaceSupport (ColorSpace, pColorSpaceSupport);
}

HRESULT
STDMETHODCALLTYPE
IWrapDXGISwapChain::SetColorSpace1 (DXGI_COLOR_SPACE_TYPE ColorSpace)
{
  assert (ver_ >= 3);

  // Don't let the game do this if SK's HDR overrides are active
  if (__SK_HDR_16BitSwap && __SK_HDR_UserForced)
    ColorSpace = DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709;

  if (__SK_HDR_10BitSwap && __SK_HDR_UserForced)
    ColorSpace = DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;

  return
    SK_DXGISwap3_SetColorSpace1_Impl (
      static_cast <IDXGISwapChain3 *>(pReal),
                    ColorSpace, TRUE, _ReturnAddress () );
}

HRESULT
STDMETHODCALLTYPE
IWrapDXGISwapChain::ResizeBuffers1 ( UINT        BufferCount,
                                     UINT        Width,             UINT             Height,
                                     DXGI_FORMAT Format,            UINT             SwapChainFlags,
                                     const UINT *pCreationNodeMask, IUnknown *const *ppPresentQueue )
{
  assert (ver_ >= 3);

  SK_LOG0 ( (L"ResizeBuffers1 (...)" ), L"   DXGI   ");

  const HRESULT hr =
    static_cast <IDXGISwapChain3 *>(pReal)->
      ResizeBuffers1 ( BufferCount, Width, Height, Format,
                         SwapChainFlags, pCreationNodeMask,
                           ppPresentQueue );

  return hr;
}


HRESULT
STDMETHODCALLTYPE
IWrapDXGISwapChain::SetHDRMetaData ( DXGI_HDR_METADATA_TYPE  Type,
                                     UINT                    Size,
                                     void                   *pMetaData )
{
  assert (ver_ >= 4);

  dll_log->Log (L"[ DXGI HDR ] <*> HDR Metadata: %ws",
                      Type == DXGI_HDR_METADATA_TYPE_NONE ? L"Disabled"
                                                          : L"HDR10");

  DXGI_HDR_METADATA_HDR10 metadata = {};

  if (Type == DXGI_HDR_METADATA_TYPE_NONE || (Size == sizeof (DXGI_HDR_METADATA_HDR10) && Type == DXGI_HDR_METADATA_TYPE_HDR10))
  {
    if (Size == sizeof (DXGI_HDR_METADATA_HDR10) && Type == DXGI_HDR_METADATA_TYPE_HDR10)
    {
      metadata =
        *(DXGI_HDR_METADATA_HDR10 *)pMetaData;

      SK_LOGi0 (
        L"HDR Metadata: Max Mastering=%d nits, Min Mastering=%f nits, MaxCLL=%d nits, MaxFALL=%d nits",
        metadata.MaxMasteringLuminance, (double)metadata.MinMasteringLuminance * 0.0001,
        metadata.MaxContentLightLevel,          metadata.MaxFrameAverageLightLevel
      );
    }

    if (config.render.dxgi.hdr_metadata_override == -1)
    {
      auto& rb =
        SK_GetCurrentRenderBackend ();

      auto& display =
        rb.displays [rb.active_display];

      if (display.gamut.maxLocalY == 0.0f)
      {
        SK_ComPtr <IDXGIOutput>      pOutput;
        pReal->GetContainingOutput (&pOutput.p);

        SK_ComQIPtr <IDXGIOutput6>
            pOutput6 (   pOutput);
        if (pOutput6.p != nullptr)
        {
          DXGI_OUTPUT_DESC1    outDesc1 = { };
          pOutput6->GetDesc1 (&outDesc1);

          display.gamut.maxLocalY   = outDesc1.MaxLuminance;
          display.gamut.maxAverageY = outDesc1.MaxFullFrameLuminance;
          display.gamut.maxY        = outDesc1.MaxLuminance;
          display.gamut.minY        = outDesc1.MinLuminance;
          display.gamut.xb          = outDesc1.BluePrimary  [0];
          display.gamut.yb          = outDesc1.BluePrimary  [1];
          display.gamut.xg          = outDesc1.GreenPrimary [0];
          display.gamut.yg          = outDesc1.GreenPrimary [1];
          display.gamut.xr          = outDesc1.RedPrimary   [0];
          display.gamut.yr          = outDesc1.RedPrimary   [1];
          display.gamut.Xw          = outDesc1.WhitePoint   [0];
          display.gamut.Yw          = outDesc1.WhitePoint   [1];
          display.gamut.Zw          = 1.0f - display.gamut.Xw - display.gamut.Yw;

          SK_ReleaseAssert (outDesc1.Monitor == display.monitor || display.monitor == 0);
        }
      }

      metadata.MinMasteringLuminance     = sk::narrow_cast <UINT>   (display.gamut.minY / 0.0001);
      metadata.MaxMasteringLuminance     = sk::narrow_cast <UINT>   (display.gamut.maxY);
      metadata.MaxContentLightLevel      = sk::narrow_cast <UINT16> (display.gamut.maxLocalY);
      metadata.MaxFrameAverageLightLevel = sk::narrow_cast <UINT16> (display.gamut.maxAverageY);

      metadata.BluePrimary  [0]          = sk::narrow_cast <UINT16> (0.1500/*display.gamut.xb*/ * 50000.0F);
      metadata.BluePrimary  [1]          = sk::narrow_cast <UINT16> (0.0600/*display.gamut.yb*/ * 50000.0F);
      metadata.RedPrimary   [0]          = sk::narrow_cast <UINT16> (0.6400/*display.gamut.xr*/ * 50000.0F);
      metadata.RedPrimary   [1]          = sk::narrow_cast <UINT16> (0.3300/*display.gamut.yr*/ * 50000.0F);
      metadata.GreenPrimary [0]          = sk::narrow_cast <UINT16> (0.3000/*display.gamut.xg*/ * 50000.0F);
      metadata.GreenPrimary [1]          = sk::narrow_cast <UINT16> (0.6000/*display.gamut.yg*/ * 50000.0F);
      metadata.WhitePoint   [0]          = sk::narrow_cast <UINT16> (0.3127/*display.gamut.Xw*/ * 50000.0F);
      metadata.WhitePoint   [1]          = sk::narrow_cast <UINT16> (0.3290/*display.gamut.Yw*/ * 50000.0F);
        
      SK_RunOnce (
        SK_LOGi0 (
          L"Metadata Override: Max Mastering=%d nits, Min Mastering=%f nits, MaxCLL=%d nits, MaxFALL=%d nits",
          metadata.MaxMasteringLuminance, (double)metadata.MinMasteringLuminance * 0.0001,
          metadata.MaxContentLightLevel,          metadata.MaxFrameAverageLightLevel
        )
      );

      if (Size == sizeof (DXGI_HDR_METADATA_HDR10) && Type == DXGI_HDR_METADATA_TYPE_HDR10)
        *(DXGI_HDR_METADATA_HDR10 *)pMetaData = metadata;
      else
      {
        pMetaData = &metadata;
        Size      = sizeof (DXGI_HDR_METADATA_HDR10);
        Type      = DXGI_HDR_METADATA_TYPE_HDR10;
      }
    }
  }

  //SK_LOG_FIRST_CALL

  const HRESULT hr =
    static_cast <IDXGISwapChain4 *>(pReal)->
      SetHDRMetaData (Type, Size, pMetaData);

  ////////SK_HDR_GetControl ()->meta._AdjustmentCount++;

  return hr;
}

bool bSwapChainNeedsResize = false;
bool bRecycledSwapChains   = false;

static LONG lLastRefreshDenom = 0;
static LONG lLastRefreshNum   = 0;

HRESULT
STDMETHODCALLTYPE
SK_DXGI_SwapChain_SetFullscreenState_Impl (
  _In_       IDXGISwapChain *pSwapChain,
  _In_       BOOL            Fullscreen,
  _In_opt_   IDXGIOutput    *pTarget,
             BOOL            bWrapped )
{
  const auto
  _Return =
    [&](HRESULT hr)
    {
      if (SUCCEEDED (hr) && Fullscreen == TRUE)
      {
        if (config.render.dxgi.fake_fullscreen_mode)
        {
          HWND hWnd = game_window.hWnd;

          SK_ComQIPtr <IDXGISwapChain1>
                           pSwapChain1 (pSwapChain);
                                        pSwapChain1->GetHwnd (&hWnd);
          SK_RealizeForegroundWindow (                         hWnd);
                          ShowWindow (hWnd,                 SW_SHOW);
        }
      }

      return hr;
    };

  const auto
  IDXGISwapChain_SetFullscreenState =
    [&](       IDXGISwapChain *pSwapChain,
               BOOL            Fullscreen,
               IDXGIOutput    *pTarget )
    {
      HRESULT hr =
        bWrapped ?
          pSwapChain->SetFullscreenState (         Fullscreen, pTarget)
                 :
          SetFullscreenState_Original (pSwapChain, Fullscreen, pTarget);

      return hr;
    };

  DXGI_SWAP_CHAIN_DESC   sd = { };
  pSwapChain->GetDesc  (&sd);

  SK_ComPtr <ID3D12Device>              pDev12;
  pSwapChain->GetDevice (IID_PPV_ARGS (&pDev12.p));

  //
  // Fake Fullscreen Mode
  //
  if (config.render.dxgi.fake_fullscreen_mode)
  {
    const SK_RenderBackend& rb =
      SK_GetCurrentRenderBackend ();

    if (Fullscreen != FALSE)
    {   Fullscreen  = FALSE;
      auto hMon =
        MonitorFromWindow (game_window.hWnd, 0x0);

      MONITORINFO             mi = { .cbSize = sizeof (MONITORINFO) };
      GetMonitorInfoW (hMon, &mi);

      if ( (UINT)(mi.rcMonitor.right  - mi.rcMonitor.left) >= sd.BufferDesc.Width &&
           (UINT)(mi.rcMonitor.bottom - mi.rcMonitor.top)  >= sd.BufferDesc.Height )
      {
        using state_cache_s = IWrapDXGISwapChain::state_cache_s;
            IWrapDXGISwapChain::state_cache_s                state_cache;

        if (SUCCEEDED (SK_DXGI_GetPrivateData <state_cache_s> ((IDXGIObject *)rb.swapchain.p, &state_cache)))
        {
          state_cache._fakefullscreen = true;

          SK_DXGI_SetPrivateData <state_cache_s> ((IDXGIObject *)rb.swapchain.p, &state_cache);

          SK_Window_RemoveBorders      ();
          SK_Window_RepositionIfNeeded ();

          return S_OK;
        }
      }
    }

    else
    {
      // Add borders back only if transitioning Fullscreen -> Windowed
      if (rb.isFakeFullscreen ())
      {
        SK_Window_RestoreBorders (0,0);
      }
    }
  }

  if (! pDev12.p)
  {
    if (sd.Flags & DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT)
    {
      if (Fullscreen != FALSE)
      {
        SK_LOG_ONCE ( L" >> Ignoring SetFullscreenState (...) on a Latency Waitable SwapChain." );

        // Engine is broken, easier just to add a special-case for it because if we return this to
        //   Unreal, that engine breaks in an even weirder way.
        if (SK_GetCurrentGameID () == SK_GAME_ID::Ys_Eight)
          return
            _Return (DXGI_STATUS_MODE_CHANGED);

        return
          _Return (DXGI_ERROR_INVALID_CALL);
      }

      // If we're latency-waitable, then we're already in windowed mode... nothing to do here.
      return
        _Return (S_OK);
    }
  }

  SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  // Dummy init swapchains (i.e. 1x1 pixel) should not have
  //   override parameters applied or it's usually fatal.
  bool no_override = (! SK_DXGI_IsSwapChainReal (pSwapChain));

  if (! no_override)
  {
    if ((! config.window.background_render) && config.display.force_fullscreen && Fullscreen == FALSE)
    {
      dll_log->Log ( L"[   DXGI   ]  >> Display Override "
                     L"(Requested: Windowed, Using: Fullscreen)" );
      Fullscreen = TRUE;
    }

    else if ((config.window.background_render || config.display.force_windowed) && Fullscreen != FALSE && config.render.dxgi.fake_fullscreen_mode == false)
    {
      Fullscreen = FALSE;
      pTarget    = nullptr;
      dll_log->Log ( L"[   DXGI   ]  >> Display Override "
                     L"(Requested: Fullscreen, Using: Windowed)" );
      SK_Window_RemoveBorders ();
    }

    else if (request_mode_change == mode_change_request_e::Fullscreen &&
                 Fullscreen == FALSE)
    {
      dll_log->Log ( L"[   DXGI   ]  >> Display Override "
               L"User Initiated Fulllscreen Switch" );
      Fullscreen = TRUE;
    }
    else if (request_mode_change == mode_change_request_e::Windowed &&
                      Fullscreen != FALSE)
    {
      dll_log->Log ( L"[   DXGI   ]  >> Display Override "
               L"User Initiated Windowed Switch" );
      Fullscreen = FALSE;
      pTarget    = nullptr;
    }
  }

  SK_COMPAT_FixUpFullscreen_DXGI (Fullscreen);

  BOOL bOrigFullscreen = rb.fullscreen_exclusive;
  BOOL bHadBorders     = SK_Window_HasBorder (game_window.hWnd);

  if ((config.render.dxgi.skip_mode_changes || config.display.force_windowed || config.window.background_render) && (! SK_IsModuleLoaded (L"sl.interposer.dll")))
  {
    // Does not work correctly when recycling swapchains
    if ((! bRecycledSwapChains) || config.display.force_windowed || (config.window.background_render && config.render.dxgi.fake_fullscreen_mode == false))
    {
      SK_ComPtr <IDXGIOutput>                                                  pOutput;
      BOOL                                            _OriginalFullscreen;
      if (SUCCEEDED (pSwapChain->GetFullscreenState (&_OriginalFullscreen,    &pOutput.p)))
      { bool  mode_change =            (Fullscreen != _OriginalFullscreen) || (pOutput.p != pTarget && pTarget != nullptr);
        if (! mode_change)
        {
          SK_LOGi0 ( L"Redundant Fullscreen Mode Change Ignored  ( %s )",
                                 Fullscreen ?
                              L"Fullscreen" : L"Windowed" );

          return
            _Return (S_OK);
        }
      }
    }
  }

  // D3D12's Fullscreen Optimization forgets to remove borders from time to time (lol)
  if (Fullscreen) SK_Window_RemoveBorders ();

  HRESULT    ret = E_UNEXPECTED;
  DXGI_CALL (ret, IDXGISwapChain_SetFullscreenState (pSwapChain, Fullscreen, pTarget))

  if ( SUCCEEDED (ret) )
  {
    if (SK_DXGI_IsFlipModelSwapChain (sd))
    {
      // Any Flip Model game already knows to do this stuff...
      if ((! rb.active_traits.bOriginallyFlip) && (pDev12.p == nullptr))
      {
        HRESULT hr =
          pSwapChain->Present (0, DXGI_PRESENT_TEST);

        if ( FAILED (hr) || hr == DXGI_STATUS_MODE_CHANGE_IN_PROGRESS )
        {
          bSwapChainNeedsResize = true;

          UINT _Flags =
            SK_DXGI_FixUpLatencyWaitFlag (pSwapChain, sd.Flags);

          if (sd.Windowed == Fullscreen && FAILED (pSwapChain->ResizeBuffers (sd.BufferCount, sd.BufferDesc.Width, sd.BufferDesc.Height, sd.BufferDesc.Format, _Flags)))
          {
            rb.fullscreen_exclusive = bOrigFullscreen;

            return
              _Return (DXGI_ERROR_NOT_CURRENTLY_AVAILABLE);
          }

          bSwapChainNeedsResize = false;
        }
      }

      else
        bSwapChainNeedsResize = true;
    }
  }

  // Mode change failed, and we removed borders prematurely... add them back!
  else
  {
    rb.fullscreen_exclusive = bOrigFullscreen;

    if (config.render.dxgi.skip_mode_changes && (! SK_IsModuleLoaded (L"sl.interposer.dll")))
    {
      if (Fullscreen) { if (bHadBorders) SK_Window_RestoreBorders (0x0, 0x0); }
      else                               SK_Window_RemoveBorders  (        );
    }
  }

  BOOL                                            bFinalState = FALSE;
  if (SUCCEEDED (pSwapChain->GetFullscreenState (&bFinalState, nullptr)))
                        rb.fullscreen_exclusive = bFinalState;

  // Trigger mode switch if needed
  if (SUCCEEDED (ret) && bFinalState == TRUE && (sd.Windowed == Fullscreen))
  {
    SK_ComPtr <IDXGIOutput>             pOutput = pTarget;
    if (                                pOutput.p == nullptr)
    {
      pSwapChain->GetContainingOutput (&pOutput.p);
    }

    if (pOutput.p != nullptr)
    {
      DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenDesc = { };
      SK_ComQIPtr <IDXGISwapChain1>
          pSwap1 (pSwapChain);
      if (pSwap1 != nullptr)
          pSwap1->GetFullscreenDesc (&fullscreenDesc);

      DXGI_MODE_DESC
        modeDesc                         = { };
        modeDesc.Width                   = sd.BufferDesc.Width;
        modeDesc.Height                  = sd.BufferDesc.Height;
        modeDesc.Format                  = sd.BufferDesc.Format;
        modeDesc.Scaling                 = sd.BufferDesc.Scaling;//DXGI_MODE_SCALING_UNSPECIFIED;
        modeDesc.ScanlineOrdering        = fullscreenDesc.ScanlineOrdering;
        modeDesc.RefreshRate.Numerator   = lLastRefreshNum;
        modeDesc.RefreshRate.Denominator = lLastRefreshDenom;

      if (config.render.framerate.refresh_rate > 0)
      {
        modeDesc.RefreshRate.Numerator   = config.render.framerate.rescan_.Numerator;
        modeDesc.RefreshRate.Denominator = config.render.framerate.rescan_.Denom;
      }

      DXGI_MODE_DESC   matchedMode = { };
      if ( SUCCEEDED ( pOutput->FindClosestMatchingMode (
           &modeDesc, &matchedMode, nullptr )
                     )                  ) {
                                   matchedMode.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
        pSwapChain->ResizeTarget (&matchedMode);
      }
    }
  }

  return
    _Return (ret);
}

bool
SK_RenderBackend_V2::isFakeFullscreen (void) const
{
  if (! SK_API_IsDXGIBased (api))
    return false;

  return
    SK_DXGI_IsFakeFullscreen (swapchain);
}

bool
SK_RenderBackend_V2::isTrueFullscreen (void) const
{
  return
    fullscreen_exclusive && (! isFakeFullscreen ());
}

bool
SK_DXGI_IsFakeFullscreen (IUnknown *pSwapChain) noexcept
{
  if (SK_ComQIPtr <IDXGISwapChain> pDXGISwapChain (pSwapChain);
                                   pDXGISwapChain != nullptr)
  {
    using state_cache_s = IWrapDXGISwapChain::state_cache_s;
        IWrapDXGISwapChain::state_cache_s                    state_cache;
    SK_DXGI_GetPrivateData <state_cache_s> (pDXGISwapChain, &state_cache);

    return
      (state_cache._fakefullscreen);
  }

  return false;
}

HRESULT
STDMETHODCALLTYPE
SK_DXGI_SwapChain_ResizeBuffers_Impl (
  _In_ IDXGISwapChain *pSwapChain,
  _In_ UINT            BufferCount,
  _In_ UINT            Width,
  _In_ UINT            Height,
  _In_ DXGI_FORMAT     NewFormat,
  _In_ UINT            SwapChainFlags,
       BOOL            bWrapped )
{
  SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  DXGI_SWAP_CHAIN_DESC  swap_desc = { };
  pSwapChain->GetDesc (&swap_desc);

  using state_cache_s = IWrapDXGISwapChain::state_cache_s;
        state_cache_s                                  state_cache;
  SK_DXGI_GetPrivateData <state_cache_s> (pSwapChain, &state_cache);

  const auto
  _Return =
    [&](HRESULT hr)
    {
      SK_DXGI_SetPrivateData <state_cache_s> (pSwapChain, &state_cache);

      if (SUCCEEDED (hr) && rb.swapchain_consistent)
      {
        BOOL                             bFullscreen = FALSE;
        pSwapChain->GetFullscreenState (&bFullscreen, nullptr);

        if (! bFullscreen)
        {
          //SK_DeferCommand ("Window.TopMost true");
          //
          //if (config.window.always_on_top < AlwaysOnTop)
          //  SK_DeferCommand ("Window.TopMost false");
        }
      }

      return hr;
    };

  const auto
  IDXGISwapChain_ResizeBuffers =
    [&]( IDXGISwapChain *pSwapChain,
         UINT            BufferCount,
         UINT            Width,
         UINT            Height,
         DXGI_FORMAT     NewFormat,
         UINT            SwapChainFlags )
    {
      HRESULT hr =
        bWrapped ?
          pSwapChain->ResizeBuffers (            BufferCount, Width, Height,
                                                 NewFormat, SwapChainFlags )
                 :
          ResizeBuffers_Original    (pSwapChain, BufferCount, Width, Height,
                                                 NewFormat, SwapChainFlags );

      rb.swapchain_consistent = SUCCEEDED (hr);

      return hr;
    };

  auto _D3D12_ResetBufferIndexToZero = [&](void)
  {
    SK_ComPtr <ID3D12Device>                           pD3D12Dev;
    pSwapChain->GetDevice (IID_ID3D12Device, (void **)&pD3D12Dev.p);

    bool d3d12 =
      (pD3D12Dev.p != nullptr);

    SK_ComQIPtr <IDXGISwapChain3>
             pSwap3 (pSwapChain);

    // When skipping resize operations in D3D12, there's an important side-effect that
    //   must be reproduced:
    //
    //    * Current Buffer Index reverts to 0 on success
    //
    //  --> We need to make several unsynchronized Present calls until we advance back to
    //        backbuffer index 0.
    if (d3d12 && pSwap3->GetCurrentBackBufferIndex () != 0)
    {
      int iUnsyncedPresents = 0;

      HRESULT hrUnsynced =
        pSwapChain->Present (0, DXGI_PRESENT_RESTART | DXGI_PRESENT_DO_NOT_WAIT);

      while ( SUCCEEDED (hrUnsynced) ||
                         hrUnsynced == DXGI_ERROR_WAS_STILL_DRAWING )
      {
        ++iUnsyncedPresents;

        if (pSwap3->GetCurrentBackBufferIndex () == 0)
          break;

        hrUnsynced =
          pSwapChain->Present (0, DXGI_PRESENT_RESTART | DXGI_PRESENT_DO_NOT_WAIT);
      }

      SK_LOGi0 (
        L"Issued %d unsync'd Presents to reset the SwapChain's current index to 0 "
        L"(required D3D12 ResizeBuffers behavior)", iUnsyncedPresents
      );
    }
  };

  auto _ReleaseResourcesAndRetryResize = [&](HRESULT& ret)
  {
    _D3D12_ResetBufferIndexToZero ();

    if      (rb.api == SK_RenderAPI::D3D12) ResetImGui_D3D12 (pSwapChain);
    else if (rb.api == SK_RenderAPI::D3D11) ResetImGui_D3D11 (pSwapChain);

    rb.releaseOwnedResources ();

    ret =
      IDXGISwapChain_ResizeBuffers ( pSwapChain, BufferCount, Width, Height,
                                       NewFormat, SwapChainFlags );

    if (SUCCEEDED (ret))
    {
      if      (rb.api == SK_RenderAPI::D3D12) _d3d12_rbk->init ((IDXGISwapChain3 *)pSwapChain, _d3d12_rbk->_pCommandQueue);
      else if (rb.api == SK_RenderAPI::D3D11) _d3d11_rbk->init ((IDXGISwapChain3 *)pSwapChain, _d3d11_rbk->_pDevice, _d3d11_rbk->_pDeviceCtx);
    }
  };

  //
  // Fix a number of problems caused by RTSS
  //
  {
    // We can't add or remove this flag, or the API call will fail. So fix it (!!) :)
    if (swap_desc.Flags & DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT)
      SwapChainFlags  |=  DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
    else
      SwapChainFlags  &= ~DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

    if (swap_desc.Flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING)
      SwapChainFlags  |=  DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    else
      SwapChainFlags  &= ~DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

    //
    // Make note of pre-HDR override formats so user can turn HDR on/off even
    //   if the game always calls ResizeBuffers (...) using the current format
    //
    if ( NewFormat != DXGI_FORMAT_UNKNOWN &&
         NewFormat != DXGI_FORMAT_R16G16B16A16_FLOAT )
    {
      state_cache.lastNonHDRFormat = NewFormat;
    }

    else if (NewFormat == DXGI_FORMAT_UNKNOWN)
    {
      // This is wrong... why was it here?
      ////if (swapDesc.BufferDesc.Format != DXGI_FORMAT_R16G16B16A16_FLOAT)
      ////  state_cache.lastNonHDRFormat = NewFormat;
    }


    if (! (__SK_HDR_16BitSwap || __SK_HDR_10BitSwap))
    {
    //if (state_cache.lastNonHDRFormat != DXGI_FORMAT_UNKNOWN)
    //  NewFormat = state_cache.lastNonHDRFormat;
    }

    else
    {
      if (     __SK_HDR_16BitSwap && __SK_HDR_UserForced)
        NewFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
      else if (__SK_HDR_10BitSwap && __SK_HDR_UserForced)
        NewFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
    }
  }

  SK_ComQIPtr <IDXGISwapChain1>
                   pSwapChain1 (pSwapChain);

  HWND hWnd = 0;

  if ( pSwapChain1 == nullptr || FAILED (
       pSwapChain1->GetHwnd (&hWnd)     ) )
  {
    hWnd =
      SK_GetGameWindow ();
  }

  RECT                  rcClient = { };
  GetClientRect (hWnd, &rcClient);

  // Expand 0x0 to window dimensions for the redundancy check
  if (Width  == 0) Width  = rcClient.right  - rcClient.left;
  if (Height == 0) Height = rcClient.bottom - rcClient.top;

  auto origWidth  = Width;
  auto origHeight = Height;

  const bool fake_fullscreen =
    rb.isFakeFullscreen ();

  bool borderless = config.window.borderless || fake_fullscreen;
  bool fullscreen = config.window.fullscreen || fake_fullscreen;

  if ( borderless &&
       fullscreen && (! rb.fullscreen_exclusive) )
  {
    if ( rb.active_display >= 0 &&
         rb.active_display < rb._MAX_DISPLAYS )
    {
      auto w =
        rb.displays [rb.active_display].rect.right -
        rb.displays [rb.active_display].rect.left,
           h =
        rb.displays [rb.active_display].rect.bottom -
        rb.displays [rb.active_display].rect.top;

      Width  = w;
      Height = h;

      if ( swap_desc.BufferDesc.Width  != static_cast <UINT> (w) ||
           swap_desc.BufferDesc.Height != static_cast <UINT> (h) )
      {
        if (w != 0 && h != 0)
        {
          SK_LOGi0 (
            L" >> SwapChain Resolution Override "
            L"(Requested: %dx%d), (Actual: %dx%d) [ Borderless Fullscreen ]",
            origWidth, origHeight,
                Width,     Height );

          state_cache._stalebuffers = true;
        }
      }
    }
  }

  if (! config.window.res.override.isZero ())
  {
    Width  = config.window.res.override.x;
    Height = config.window.res.override.y;

    if ( origWidth  != Width ||
         origHeight != Height )
    {
      SK_LOGi0 (
        L" >> SwapChain Resolution Override "
        L"(Requested: %dx%d), (Actual: %dx%d) [ User-Defined Resolution ]",
        origWidth, origHeight,
            Width,     Height );

      state_cache._stalebuffers = true;
    }
  }

  SwapChainFlags =
    SK_DXGI_FixUpLatencyWaitFlag (pSwapChain, SwapChainFlags);
  NewFormat      =        /*NewFormat == DXGI_FORMAT_UNKNOWN   ? DXGI_FORMAT_UNKNOWN*/
    SK_DXGI_PickHDRFormat ( NewFormat, swap_desc.Windowed,
        SK_DXGI_IsFlipModelSwapEffect (swap_desc.SwapEffect) );

  if (config.render.output.force_10bpc && (! __SK_HDR_16BitSwap))
  {
    if ( DirectX::MakeTypeless (NewFormat) ==
         DirectX::MakeTypeless (DXGI_FORMAT_R8G8B8A8_UNORM) )
    {
      SK_LOGi0 ( L" >> 8-bpc format (%hs) replaced with "
                 L"DXGI_FORMAT_R10G10B10A2_UNORM for 10-bpc override",
                   SK_DXGI_FormatToStr (NewFormat).data () );

      NewFormat =
        DXGI_FORMAT_R10G10B10A2_UNORM;
    }
  }

  //
  // Do not apply backbuffer count overrides in D3D12 unless user presents
  //   a valid footgun license and can afford to lose a few toes.
  //
  if (                                  SK_ComPtr <ID3D12Device> pSwapDev12;
      FAILED (pSwapChain->GetDevice (IID_ID3D12Device, (void **)&pSwapDev12.p)) ||
               config.render.dxgi.allow_d3d12_footguns
     )
  {
    if (       config.render.framerate.buffer_count != SK_NoPreference &&
         (UINT)config.render.framerate.buffer_count !=  BufferCount    &&
         BufferCount                                !=  0              &&

             config.render.framerate.buffer_count   >   0              &&
             config.render.framerate.buffer_count   <   16 )
    {
      BufferCount =
        config.render.framerate.buffer_count;

      dll_log->Log ( L"[   DXGI   ]  >> Buffer Count Override: %lu buffers",
                                        BufferCount );
    }
  }

  // Fix-up BufferCount and Flags in Flip Model
  if (SK_DXGI_IsFlipModelSwapEffect (swap_desc.SwapEffect))
  {
    if (! dxgi_caps.swapchain.allow_tearing)
    {
      if (SwapChainFlags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING)
      {
        SK_ComPtr <IDXGIFactory5> pFactory5;
        if ( SUCCEEDED (
               CreateDXGIFactory_Import (IID_IDXGIFactory5, (void **)&pFactory5.p)
             )
           )
        {
          const HRESULT hr =
            pFactory5->CheckFeatureSupport (
              DXGI_FEATURE_PRESENT_ALLOW_TEARING,
                &dxgi_caps.swapchain.allow_tearing,
                  sizeof (dxgi_caps.swapchain.allow_tearing)
            );

          dxgi_caps.swapchain.allow_tearing =
            SUCCEEDED (hr) && dxgi_caps.swapchain.allow_tearing;
        }

        if (! dxgi_caps.swapchain.allow_tearing)
        {
          SK_ReleaseAssert ( 0 ==
            (SwapChainFlags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING)
                           );

          SwapChainFlags &= ~DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
        }
      }
    }

    else
    {
      // Can't add or remove this after the SwapChain is created,
      //   all calls must have the flag set or clear before & after.
      if (swap_desc.Flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING)
      {
        SwapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
        dll_log->Log ( L"[ DXGI 1.5 ]  >> Tearing Option:  Enable" );
      }
    }

    if (BufferCount != 0 || swap_desc.BufferCount < 2)
    {
      BufferCount =
        std::min (16ui32,
         std::max (2ui32, BufferCount)
                 );
    }
  }

  if (_NO_ALLOW_MODE_SWITCH)
    SwapChainFlags &= ~DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

  if (! config.window.res.override.isZero ())
  {
    Width  = config.window.res.override.x;
    Height = config.window.res.override.y;
  }

  // Clamp the buffer dimensions if the user has a min/max preference
  const UINT
    max_x = config.render.dxgi.res.max.x < Width  ?
            config.render.dxgi.res.max.x : Width,
    min_x = config.render.dxgi.res.min.x > Width  ?
            config.render.dxgi.res.min.x : Width,
    max_y = config.render.dxgi.res.max.y < Height ?
            config.render.dxgi.res.max.y : Height,
    min_y = config.render.dxgi.res.min.y > Height ?
            config.render.dxgi.res.min.y : Height;

  Width   =  std::max ( max_x , min_x );
  Height  =  std::max ( max_y , min_y );

  // If forcing flip-model, we can't allow sRGB formats...
  if ( config.render.framerate.flip_discard &&
       SK_DXGI_IsFlipModelSwapChain (swap_desc) )
  {
    rb.active_traits.bFlipMode =
      dxgi_caps.present.flip_sequential;

    // Format overrides must be performed in some cases (sRGB)
    switch (NewFormat)
    {
      case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        NewFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
        dll_log->Log ( L"[ DXGI 1.2 ]  >> sRGB (B8G8R8A8) Override "
                       L"Required to Enable Flip Model" );
        rb.srgb_stripped = true;
        break;
      case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        NewFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
        dll_log->Log ( L"[ DXGI 1.2 ]  >> sRGB (R8G8B8A8) Override "
                       L"Required to Enable Flip Model" );
        rb.srgb_stripped = true;
        break;
    }
  }

  bool skippable = true;

  if (config.render.dxgi.skip_mode_changes && (! SK_IsModuleLoaded (L"sl.interposer.dll")))
  {
    // Never skip buffer resizes if we have encountered a fullscreen SwapChain
    static bool was_ever_fullscreen = false;

    was_ever_fullscreen |=
      (swap_desc.Windowed == false);

    if (! ((swap_desc.BufferDesc.Width  == Width)                                                   &&
           (swap_desc.BufferDesc.Height == Height)                                                  &&
           (swap_desc.BufferDesc.Format == NewFormat      || NewFormat      == DXGI_FORMAT_UNKNOWN) &&
           (swap_desc.BufferCount       == BufferCount    || BufferCount    == 0)                   &&
           (swap_desc.Flags             == SwapChainFlags || SwapChainFlags == 0x0))
        || (state_cache._stalebuffers) ||
            was_ever_fullscreen
       )
    {
      skippable = false;
    }

    else
    {
      skippable =
        (! bSwapChainNeedsResize);
    }
  }

  else
    skippable = false;

  HRESULT ret = S_OK;

  if (! skippable)
  {
    ResetImGui_D3D12 (pSwapChain);
    ResetImGui_D3D11 (pSwapChain);

    ret =
      IDXGISwapChain_ResizeBuffers ( pSwapChain, BufferCount, Width, Height,
                                       NewFormat, SwapChainFlags );

    if (SUCCEEDED (ret))
      bSwapChainNeedsResize = false;

    // EOS Overlay and DLSS Frame Generation may Cause Unreal Engine Games to
    //   Unncessarily Crash in D3D12, so Swallow and Log the Failure.
    if (FAILED (ret) && config.render.dxgi.suppress_resize_fail)
    {
      if (ret != DXGI_ERROR_DEVICE_REMOVED && ret != E_ACCESSDENIED)
      {
        SK_LOGi0 ( L"SwapChain Resize Failed (%x) - Error Suppressed!",
                    ret );

        _ReleaseResourcesAndRetryResize (ret);
      }
    }


    static bool
      was_hdr = false;

    // Apply scRGB colorspace immediately, 16bpc formats can be either this
    //   or regular Rec709 and Rec709 is washed out after making this switch
    if (__SK_HDR_16BitSwap && NewFormat == DXGI_FORMAT_R16G16B16A16_FLOAT)
    {
      SK_ComQIPtr <IDXGISwapChain4>
          pSwap4 (pSwapChain);
      if (pSwap4 != nullptr) {
          pSwap4->SetColorSpace1 (DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709);
          was_hdr = true;
      }
    }

    if (__SK_HDR_10BitSwap && NewFormat == DXGI_FORMAT_R10G10B10A2_UNORM)
    {
      SK_ComQIPtr <IDXGISwapChain4>
          pSwap4 (pSwapChain);
      if (pSwap4 != nullptr) {
          pSwap4->SetColorSpace1 (DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020);
          was_hdr = true;
      }
    }

    if ((!__SK_HDR_16BitSwap && NewFormat != DXGI_FORMAT_R16G16B16A16_FLOAT) &&
        (!__SK_HDR_10BitSwap && NewFormat != DXGI_FORMAT_R10G10B10A2_UNORM))
    {
      if (std::exchange (was_hdr, false))
      {
        SK_ComQIPtr <IDXGISwapChain4>
            pSwap4 (pSwapChain);
        if (pSwap4 != nullptr)
            pSwap4->SetColorSpace1 (DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709);
      }
    }
  }

  else
  {
    rb.swapchain_consistent = true;

    SK_LOGi0 (
      L"Skipped Redundant ResizeBuffers Operation "
      L"[(%dx%d) x %d Buffers, Format: %hs, Flags: %x]",
             swap_desc.BufferDesc.Width, swap_desc.BufferDesc.Height,
                                         swap_desc.BufferCount,
                    SK_DXGI_FormatToStr (swap_desc.BufferDesc.Format).data (),
                                         swap_desc.Flags
    );

    //extern bool __SK_HDR_UserForced;
    //
    //if (! __SK_HDR_UserForced)
    //{
    //  SK_ComQIPtr <IDXGISwapChain4>
    //        pSwap4 (pSwapChain);
    //    if (pSwap4 != nullptr)
    //        pSwap4->SetColorSpace1 (DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709);
    //}

    _D3D12_ResetBufferIndexToZero ();
  }

  // EOS Overlay May Be Broken in D3D12 Games
  if (config.render.dxgi.suppress_resize_fail && FAILED (ret))
  {
    if (ret != DXGI_ERROR_DEVICE_REMOVED && ret != E_ACCESSDENIED)
    {
      SK_LOGi0 ( L"SwapChain Resize Failed (%x) - Error Suppressed!",
                  ret );

      _ReleaseResourcesAndRetryResize (ret);
    }
  }

  if (FAILED (ret))
  {
    _ReleaseResourcesAndRetryResize (ret);

    rb.swapchain_consistent = SUCCEEDED (ret);

    if (SK_IsDebuggerPresent ())
    {
      SK_DXGI_OutputDebugString ( "IDXGISwapChain::ResizeBuffers (...) failed, look alive...",
                                    DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR );

      SK_ComPtr <IDXGIDevice>               pDevice;
      pSwapChain->GetDevice (IID_PPV_ARGS (&pDevice.p));

      SK_DXGI_ReportLiveObjects ( nullptr != rb.device ?
                                             rb.device : pDevice.p );
    }

    if (config.render.dxgi.suppress_resize_fail && FAILED (ret))
    {
      if (ret != DXGI_ERROR_DEVICE_REMOVED && ret != E_ACCESSDENIED)
          ret = S_OK;
    }
  }

  else
  {
    state_cache._stalebuffers = false;
  }


  if (! skippable)
  { DXGI_CALL      (ret, ret); }
  else
  { DXGI_SKIP_CALL (ret, ret); }


  return
    _Return (ret);
}

HRESULT
STDMETHODCALLTYPE
SK_DXGI_SwapChain_ResizeTarget_Impl (
  _In_       IDXGISwapChain *pSwapChain,
  _In_ const DXGI_MODE_DESC *pNewTargetParameters,
             BOOL            bWrapped )
{
  const auto
  _Return =
    [&](HRESULT hr)
    {
      if (SUCCEEDED (hr))
      {
        BOOL                             bFullscreen = FALSE;
        pSwapChain->GetFullscreenState (&bFullscreen, nullptr);

        if (! bFullscreen)
        {
          //SK_DeferCommand ("Window.TopMost true");
          //
          //if (config.window.always_on_top < AlwaysOnTop)
          //  SK_DeferCommand ("Window.TopMost false");
        }
      }

      return hr;
    };

  const auto
  IDXGISwapChain_ResizeTarget =
    [&](       IDXGISwapChain *pSwapChain,
         const DXGI_MODE_DESC *pNewTargetParameters )
    {
      HRESULT hr =
        bWrapped ?
          pSwapChain->ResizeTarget (pNewTargetParameters)
               :
          ResizeTarget_Original (pSwapChain, pNewTargetParameters);

      return hr;
    };

  SK_ReleaseAssert (pNewTargetParameters != nullptr);

  DXGI_SWAP_CHAIN_DESC  sd = { };
  pSwapChain->GetDesc (&sd);

  const SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  if (rb.scanout.colorspace_override != DXGI_COLOR_SPACE_CUSTOM)
  {
    SK_ComQIPtr <IDXGISwapChain3>
                          pSwap3 (pSwapChain);
    if (       nullptr != pSwap3) SK_DXGI_UpdateColorSpace
                         (pSwap3);
  }

  if (config.render.framerate.swapchain_wait > 0)
  {
    SK_ComPtr <ID3D12Device>              pDev12;
    pSwapChain->GetDevice (IID_PPV_ARGS (&pDev12.p));

    if (pDev12.p == nullptr)
    {
      //
      // Latency waitable hack begin
      //
      if (sd.Flags & DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT)
      {
        dll_log->Log (
          L"[   DXGI   ]  >> Ignoring ResizeTarget (...) on a Latency Waitable SwapChain."
        );

        return
          _Return (S_OK);
      }
      /// Latency Waitable Hack End
    }
  }

  HRESULT ret =
    E_UNEXPECTED;

  DXGI_MODE_DESC new_new_params =
      *pNewTargetParameters;

  bool borderless = config.window.borderless || rb.isFakeFullscreen ();

  if (! config.display.allow_refresh_change)
  {
    if (new_new_params.RefreshRate.Denominator != 0)
    {
      SK_LOGi0 (
        L" >> Ignoring Requested Refresh Rate (%5.2f Hz)...",
          static_cast <float> (new_new_params.RefreshRate.Numerator) /
          static_cast <float> (new_new_params.RefreshRate.Denominator)
      );

      new_new_params.RefreshRate.Numerator   = 0;
      new_new_params.RefreshRate.Denominator = 0;
    }
  }

  // I don't remember why borderless is included here :)
  if ( borderless ||
       ( config.render.dxgi.scaling_mode != SK_NoPreference &&
          pNewTargetParameters->Scaling  !=
            (DXGI_MODE_SCALING)config.render.dxgi.scaling_mode )
                                ||
       ( config.render.framerate.refresh_rate          > 0.0f &&
           pNewTargetParameters->RefreshRate.Numerator != sk::narrow_cast <UINT> (
         config.render.framerate.refresh_rate) )
    )
  {
    if (  config.render.framerate.rescan_.Denom !=  1   ||
        ( config.render.framerate.refresh_rate   > 0.0f &&
          new_new_params.RefreshRate.Numerator  != sk::narrow_cast <UINT> (
          config.render.framerate.refresh_rate) )
       )
    {
      if ( ( config.render.framerate.rescan_.Denom     > 0   &&
             config.render.framerate.rescan_.Numerator > 0 ) &&
         !( new_new_params.RefreshRate.Numerator   == config.render.framerate.rescan_.Numerator &&
            new_new_params.RefreshRate.Denominator == config.render.framerate.rescan_.Denom ) )
      {
        SK_LOGi0 ( L" >> Refresh Override (Requested: %5.2f, Using: %5.2f)",
                               new_new_params.RefreshRate.Denominator != 0 ?
          static_cast <float> (new_new_params.RefreshRate.Numerator)  /
          static_cast <float> (new_new_params.RefreshRate.Denominator)     :
            std::numeric_limits <float>::quiet_NaN (),
          static_cast <float> (config.render.framerate.rescan_.Numerator) /
          static_cast <float> (config.render.framerate.rescan_.Denom)
        );

        if (config.render.framerate.rescan_.Denom != 1)
        {
               new_new_params.RefreshRate.Numerator   =
          config.render.framerate.rescan_.Numerator;
               new_new_params.RefreshRate.Denominator =
          config.render.framerate.rescan_.Denom;
        }

        else
        {
          new_new_params.RefreshRate.Numerator   =
            sk::narrow_cast <UINT> (std::ceilf (config.render.framerate.refresh_rate));
          new_new_params.RefreshRate.Denominator = 1;
        }
      }
    }

    if ( config.render.dxgi.scanline_order        != SK_NoPreference &&
          pNewTargetParameters->ScanlineOrdering  !=
            (DXGI_MODE_SCANLINE_ORDER)config.render.dxgi.scanline_order )
    {
      dll_log->Log (
        L"[   DXGI   ]  >> Scanline Override "
        L"(Requested: %hs, Using: %hs)",
          SK_DXGI_DescribeScanlineOrder (
            pNewTargetParameters->ScanlineOrdering
          ),
            SK_DXGI_DescribeScanlineOrder (
              (DXGI_MODE_SCANLINE_ORDER)config.render.dxgi.scanline_order
            )
      );

      new_new_params.ScanlineOrdering =
        (DXGI_MODE_SCANLINE_ORDER)config.render.dxgi.scanline_order;
    }

    if ( config.render.dxgi.scaling_mode != SK_NoPreference &&
          pNewTargetParameters->Scaling  !=
            (DXGI_MODE_SCALING)config.render.dxgi.scaling_mode )
    {
      dll_log->Log (
        L"[   DXGI   ]  >> Scaling Override "
        L"(Requested: %hs, Using: %hs)",
          SK_DXGI_DescribeScalingMode (
            pNewTargetParameters->Scaling
          ),
            SK_DXGI_DescribeScalingMode (
              (DXGI_MODE_SCALING)config.render.dxgi.scaling_mode
            )
      );

      new_new_params.Scaling =
        (DXGI_MODE_SCALING)config.render.dxgi.scaling_mode;
    }
  }

  if (! config.window.res.override.isZero ())
  {
    new_new_params.Width  = config.window.res.override.x;
    new_new_params.Height = config.window.res.override.y;
  }


  DXGI_MODE_DESC* pNewNewTargetParameters =
    &new_new_params;


  // Clamp the buffer dimensions if the user has a min/max preference
  const UINT
    max_x = config.render.dxgi.res.max.x < pNewNewTargetParameters->Width  ?
            config.render.dxgi.res.max.x : pNewNewTargetParameters->Width,
    min_x = config.render.dxgi.res.min.x > pNewNewTargetParameters->Width  ?
            config.render.dxgi.res.min.x : pNewNewTargetParameters->Width,
    max_y = config.render.dxgi.res.max.y < pNewNewTargetParameters->Height ?
            config.render.dxgi.res.max.y : pNewNewTargetParameters->Height,
    min_y = config.render.dxgi.res.min.y > pNewNewTargetParameters->Height ?
            config.render.dxgi.res.min.y : pNewNewTargetParameters->Height;

  pNewNewTargetParameters->Width   =  std::max ( max_x , min_x );
  pNewNewTargetParameters->Height  =  std::max ( max_y , min_y );


  pNewNewTargetParameters->Format =
    SK_DXGI_PickHDRFormat (pNewNewTargetParameters->Format, FALSE, TRUE);


  SK_ComPtr <IDXGIOutput>           pOutput;
  pSwapChain->GetContainingOutput (&pOutput.p);

  if (pOutput != nullptr)
  {
    DXGI_MODE_DESC                modeDescMatch = { };
    if ( SUCCEEDED (
      pOutput->FindClosestMatchingMode (
                 &new_new_params,&modeDescMatch, nullptr )
                   )
       )
    {
      // Game will take whatever it can get
      if (new_new_params.Scaling == DXGI_MODE_SCALING_UNSPECIFIED)
        modeDescMatch.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

      new_new_params = modeDescMatch;
    }

    // No match, better to remove the refresh rate and try again
    else
    {
      SK_LOGi0 (L"Failed to find matching mode, removing refresh rate...");

      new_new_params.RefreshRate.Denominator = 0;
      new_new_params.RefreshRate.Numerator   = 0;
    }
  }

  static UINT           numModeChanges =  0 ;
  static DXGI_MODE_DESC lastModeSet    = { };

  if (config.render.dxgi.skip_mode_changes && (! SK_IsModuleLoaded (L"sl.interposer.dll")))
  {
    auto bd =
         sd.BufferDesc;

    if ( (bd.Width                   == pNewNewTargetParameters->Width   || pNewNewTargetParameters->Width  == 0)                   &&
         (bd.Height                  == pNewNewTargetParameters->Height  || pNewNewTargetParameters->Height == 0)                   &&
         (bd.Format                  == pNewNewTargetParameters->Format  || pNewNewTargetParameters->Format == DXGI_FORMAT_UNKNOWN) &&
                                          // TODO: How to find the current mode?
                                       (pNewNewTargetParameters->Scaling          == lastModeSet.Scaling          ||
                                                                   numModeChanges == 0)                 &&
                                       (pNewNewTargetParameters->ScanlineOrdering == lastModeSet.ScanlineOrdering ||
                                                                   numModeChanges == 0)                 &&
         (bd.RefreshRate.Numerator   == pNewNewTargetParameters->RefreshRate.Numerator)                           &&
         (bd.RefreshRate.Denominator == pNewNewTargetParameters->RefreshRate.Denominator) )
    {
      *((DXGI_MODE_DESC *)pNewTargetParameters) = new_new_params;

      DXGI_SKIP_CALL (ret, S_OK); return
      _Return        (ret);
    }
  }


  DXGI_CALL ( ret,
                IDXGISwapChain_ResizeTarget (pSwapChain, pNewNewTargetParameters)
            );

  if (SUCCEEDED (ret))
  {
    if (                  new_new_params.RefreshRate.Denominator != 0 &&
                          new_new_params.RefreshRate.Numerator   != 0)
    { lLastRefreshNum   = new_new_params.RefreshRate.Numerator;
      lLastRefreshDenom = new_new_params.RefreshRate.Denominator; }

    lastModeSet = new_new_params,
                  ++numModeChanges;

    auto *pLimiter =
      SK::Framerate::GetLimiter (pSwapChain);

    if (pLimiter != nullptr)
    {
      // Since this may incur an output device change, it's best to take this
      // opportunity to re-sync the limiter's clock versus VBLANK.
      pLimiter->reset (true);
    }

    if ( new_new_params.Width  != 0 &&
         new_new_params.Height != 0 )
    {
      SK_SetWindowResX (new_new_params.Width);
      SK_SetWindowResY (new_new_params.Height);
    }

    bSwapChainNeedsResize = true;

    *((DXGI_MODE_DESC *)pNewTargetParameters) = new_new_params;
  }

  return
    _Return (ret);
}