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

#include <SpecialK/log.h>
#include <SpecialK/config.h>

#include <SpecialK/render/dxgi/dxgi_swapchain.h>
#include <algorithm>

#include <assert.h>
#include <d3d11.h>


volatile LONG SK_DXGI_LiveWrappedSwapChains  = 0;
volatile LONG SK_DXGI_LiveWrappedSwapChain1s = 0;

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


// IDXGISwapChain
HRESULT STDMETHODCALLTYPE
IWrapDXGISwapChain::QueryInterface (REFIID riid, void **ppvObj)
{
  if (ppvObj == nullptr)
  {
    return E_POINTER;
  }

  else if (
  //riid == __uuidof (this)                 ||
    riid == __uuidof (IUnknown)             ||
    riid == __uuidof (IDXGIObject)          ||
    riid == __uuidof (IDXGIDeviceSubObject) ||
    riid == __uuidof (IDXGISwapChain)       ||
    riid == __uuidof (IDXGISwapChain1)      ||
    riid == __uuidof (IDXGISwapChain2)      ||
    riid == __uuidof (IDXGISwapChain3)      ||
    riid == __uuidof (IDXGISwapChain4) )
	{
		#pragma region Update to IDXGISwapChain1 interface
		if (riid == __uuidof(IDXGISwapChain1) && ver_ < 1)
		{
			IDXGISwapChain1 *swapchain1 = nullptr;

			if (FAILED(pReal->QueryInterface(&swapchain1)))
			{
				return E_NOINTERFACE;
			}

			pReal->Release();

			pReal = swapchain1;
			ver_ = 1;
		}
		#pragma endregion
		#pragma region Update to IDXGISwapChain2 interface
		if (riid == __uuidof(IDXGISwapChain2) && ver_ < 2)
		{
			IDXGISwapChain2 *swapchain2 = nullptr;

			if (FAILED(pReal->QueryInterface(&swapchain2)))
			{
				return E_NOINTERFACE;
			}

			pReal->Release();

			pReal = swapchain2;
			ver_ = 2;
		}
		#pragma endregion
		#pragma region Update to IDXGISwapChain3 interface
		if (riid == __uuidof(IDXGISwapChain3) && ver_ < 3)
		{
			IDXGISwapChain3 *swapchain3 = nullptr;

			if (FAILED(pReal->QueryInterface(&swapchain3)))
			{
				return E_NOINTERFACE;
			}

			pReal->Release();

			pReal = swapchain3;
			ver_ = 3;
		}
		#pragma endregion
		#pragma region Update to IDXGISwapChain4 interface
		if (riid == __uuidof(IDXGISwapChain4) && ver_ < 4)
		{
			IDXGISwapChain4 *swapchain4 = nullptr;

			if (FAILED(pReal->QueryInterface(&swapchain4)))
			{
				return E_NOINTERFACE;
			}

			pReal->Release();

			pReal = swapchain4;
			ver_ = 4;
		}
		#pragma endregion

    pReal->AddRef ();
    InterlockedExchange (&refs_, pReal->Release ());
    AddRef ();

    *ppvObj = this;

    return S_OK;
  }

  return pReal->QueryInterface (riid, ppvObj);
}

ULONG
STDMETHODCALLTYPE
IWrapDXGISwapChain::AddRef (void)
{
  InterlockedIncrement (&refs_);

  return pReal->AddRef ();
}

ULONG
STDMETHODCALLTYPE
IWrapDXGISwapChain::Release (void)
{
    if (InterlockedDecrement (&refs_) == 0)
    {
      //assert(_runtime != nullptr);
    }

	  ULONG refs = pReal->Release ();

    if (ReadAcquire (&refs_) == 0 && refs != 0)
    {
      //SK_LOG0 ( (L"Reference count for 'IDXGISwapChain" << (ver_ > 0 ? std::to_string(ver_) : "") << "' object " << this << " is inconsistent: " << ref << ", but expected 0.";

      refs = 0;
    }

    if (refs == 0)
    {
      assert (ReadAcquire (&refs_) <= 0);

      if (ReadAcquire (&refs_) == 0)
      {
        if (ver_ > 0)
          InterlockedDecrement (&SK_DXGI_LiveWrappedSwapChain1s);
        else
          InterlockedDecrement (&SK_DXGI_LiveWrappedSwapChains);

        delete this;
      }
    }

    return refs;
}

HRESULT STDMETHODCALLTYPE IWrapDXGISwapChain::SetPrivateData(REFGUID Name, UINT DataSize, const void *pData)
{
	return pReal->SetPrivateData(Name, DataSize, pData);
}
HRESULT STDMETHODCALLTYPE IWrapDXGISwapChain::SetPrivateDataInterface(REFGUID Name, const IUnknown *pUnknown)
{
	return pReal->SetPrivateDataInterface(Name, pUnknown);
}
HRESULT STDMETHODCALLTYPE IWrapDXGISwapChain::GetPrivateData(REFGUID Name, UINT *pDataSize, void *pData)
{
	return pReal->GetPrivateData(Name, pDataSize, pData);
}
HRESULT STDMETHODCALLTYPE IWrapDXGISwapChain::GetParent(REFIID riid, void **ppParent)
{
	return pReal->GetParent(riid, ppParent);
}
HRESULT STDMETHODCALLTYPE IWrapDXGISwapChain::GetDevice(REFIID riid, void **ppDevice)
{
	if (ppDevice == nullptr)
	{
		return DXGI_ERROR_INVALID_CALL;
	}

	return ((ID3D11Device *)pDev)->QueryInterface (riid, ppDevice);
}
HRESULT STDMETHODCALLTYPE IWrapDXGISwapChain::Present(UINT SyncInterval, UINT Flags)
{
  return SK_DXGI_DispatchPresent ( pReal, SyncInterval, Flags,
                                     nullptr, SK_DXGI_PresentSource::Wrapper );
}
HRESULT STDMETHODCALLTYPE IWrapDXGISwapChain::GetBuffer(UINT Buffer, REFIID riid, void **ppSurface)
{
	return pReal->GetBuffer(Buffer, riid, ppSurface);
}
HRESULT STDMETHODCALLTYPE IWrapDXGISwapChain::SetFullscreenState(BOOL Fullscreen, IDXGIOutput *pTarget)
{
	return pReal->SetFullscreenState(Fullscreen, pTarget);
}
HRESULT STDMETHODCALLTYPE IWrapDXGISwapChain::GetFullscreenState(BOOL *pFullscreen, IDXGIOutput **ppTarget)
{
	return pReal->GetFullscreenState(pFullscreen, ppTarget);
}
HRESULT STDMETHODCALLTYPE IWrapDXGISwapChain::GetDesc(DXGI_SWAP_CHAIN_DESC *pDesc)
{
	return pReal->GetDesc(pDesc);
}
HRESULT STDMETHODCALLTYPE IWrapDXGISwapChain::ResizeBuffers(UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
  //if (SUCCEEDED (SK_DXGI_ValidateSwapChainResize (pReal, BufferCount, Width, Height, NewFormat)))
  //  return S_OK;

	const HRESULT hr =
    pReal->ResizeBuffers (BufferCount, Width, Height, NewFormat, SwapChainFlags);

	if (hr == DXGI_ERROR_INVALID_CALL)
	{
	}
	else if (FAILED(hr))
	{
		return hr;
	}

  //DXGI_SWAP_CHAIN_DESC desc = { };
	//pReal->GetDesc     (&desc);

	return hr;
}
HRESULT STDMETHODCALLTYPE IWrapDXGISwapChain::ResizeTarget(const DXGI_MODE_DESC *pNewTargetParameters)
{
  //if (SUCCEEDED (SK_DXGI_ValidateSwapChainResize (this, 0, pNewTargetParameters->Width, pNewTargetParameters->Height, pNewTargetParameters->Format)))
  //  return S_OK;

	return pReal->ResizeTarget(pNewTargetParameters);
}
HRESULT STDMETHODCALLTYPE IWrapDXGISwapChain::GetContainingOutput(IDXGIOutput **ppOutput)
{
	return pReal->GetContainingOutput(ppOutput);
}
HRESULT STDMETHODCALLTYPE IWrapDXGISwapChain::GetFrameStatistics(DXGI_FRAME_STATISTICS *pStats)
{
	return pReal->GetFrameStatistics(pStats);
}
HRESULT STDMETHODCALLTYPE IWrapDXGISwapChain::GetLastPresentCount(UINT *pLastPresentCount)
{
	return pReal->GetLastPresentCount(pLastPresentCount);
}

// IDXGISwapChain1
HRESULT STDMETHODCALLTYPE IWrapDXGISwapChain::GetDesc1(DXGI_SWAP_CHAIN_DESC1 *pDesc)
{
	assert(ver_ >= 1);

	return static_cast<IDXGISwapChain1 *>(pReal)->GetDesc1(pDesc);
}
HRESULT STDMETHODCALLTYPE IWrapDXGISwapChain::GetFullscreenDesc(DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pDesc)
{
	assert(ver_ >= 1);

	return static_cast<IDXGISwapChain1 *>(pReal)->GetFullscreenDesc(pDesc);
}
HRESULT STDMETHODCALLTYPE IWrapDXGISwapChain::GetHwnd(HWND *pHwnd)
{
	assert(ver_ >= 1);

	return static_cast<IDXGISwapChain1 *>(pReal)->GetHwnd(pHwnd);
}
HRESULT STDMETHODCALLTYPE IWrapDXGISwapChain::GetCoreWindow(REFIID refiid, void **ppUnk)
{
	assert(ver_ >= 1);

	return static_cast<IDXGISwapChain1 *>(pReal)->GetCoreWindow(refiid, ppUnk);
}
HRESULT STDMETHODCALLTYPE IWrapDXGISwapChain::Present1(UINT SyncInterval, UINT PresentFlags, const DXGI_PRESENT_PARAMETERS *pPresentParameters)
{
	assert(ver_ >= 1);

  return
    SK_DXGI_DispatchPresent1 ( (IDXGISwapChain1 *)pReal, SyncInterval, PresentFlags, pPresentParameters,
                               nullptr, SK_DXGI_PresentSource::Wrapper );
}
BOOL STDMETHODCALLTYPE IWrapDXGISwapChain::IsTemporaryMonoSupported()
{
	assert(ver_ >= 1);

	return static_cast<IDXGISwapChain1 *>(pReal)->IsTemporaryMonoSupported();
}
HRESULT STDMETHODCALLTYPE IWrapDXGISwapChain::GetRestrictToOutput(IDXGIOutput **ppRestrictToOutput)
{
	assert(ver_ >= 1);

	return static_cast<IDXGISwapChain1 *>(pReal)->GetRestrictToOutput(ppRestrictToOutput);
}
HRESULT STDMETHODCALLTYPE IWrapDXGISwapChain::SetBackgroundColor(const DXGI_RGBA *pColor)
{
	assert(ver_ >= 1);

	return static_cast<IDXGISwapChain1 *>(pReal)->SetBackgroundColor(pColor);
}
HRESULT STDMETHODCALLTYPE IWrapDXGISwapChain::GetBackgroundColor(DXGI_RGBA *pColor)
{
	assert(ver_ >= 1);

	return static_cast<IDXGISwapChain1 *>(pReal)->GetBackgroundColor(pColor);
}
HRESULT STDMETHODCALLTYPE IWrapDXGISwapChain::SetRotation(DXGI_MODE_ROTATION Rotation)
{
	assert(ver_ >= 1);

	return static_cast<IDXGISwapChain1 *>(pReal)->SetRotation(Rotation);
}
HRESULT STDMETHODCALLTYPE IWrapDXGISwapChain::GetRotation(DXGI_MODE_ROTATION *pRotation)
{
	assert(ver_ >= 1);

	return static_cast<IDXGISwapChain1 *>(pReal)->GetRotation(pRotation);
}

// IDXGISwapChain2
HRESULT STDMETHODCALLTYPE IWrapDXGISwapChain::SetSourceSize(UINT Width, UINT Height)
{
	assert(ver_ >= 2);

	return static_cast<IDXGISwapChain2 *>(pReal)->SetSourceSize(Width, Height);
}
HRESULT STDMETHODCALLTYPE IWrapDXGISwapChain::GetSourceSize(UINT *pWidth, UINT *pHeight)
{
	assert(ver_ >= 2);

	return static_cast<IDXGISwapChain2 *>(pReal)->GetSourceSize(pWidth, pHeight);
}
HRESULT STDMETHODCALLTYPE IWrapDXGISwapChain::SetMaximumFrameLatency(UINT MaxLatency)
{
	assert(ver_ >= 2);

	return static_cast<IDXGISwapChain2 *>(pReal)->SetMaximumFrameLatency(MaxLatency);
}
HRESULT STDMETHODCALLTYPE IWrapDXGISwapChain::GetMaximumFrameLatency(UINT *pMaxLatency)
{
	assert(ver_ >= 2);

	return static_cast<IDXGISwapChain2 *>(pReal)->GetMaximumFrameLatency(pMaxLatency);
}
HANDLE STDMETHODCALLTYPE IWrapDXGISwapChain::GetFrameLatencyWaitableObject()
{
	assert(ver_ >= 2);

	return static_cast<IDXGISwapChain2 *>(pReal)->GetFrameLatencyWaitableObject();
}
HRESULT STDMETHODCALLTYPE IWrapDXGISwapChain::SetMatrixTransform(const DXGI_MATRIX_3X2_F *pMatrix)
{
	assert(ver_ >= 2);

	return static_cast<IDXGISwapChain2 *>(pReal)->SetMatrixTransform(pMatrix);
}
HRESULT STDMETHODCALLTYPE IWrapDXGISwapChain::GetMatrixTransform(DXGI_MATRIX_3X2_F *pMatrix)
{
	assert(ver_ >= 2);

	return static_cast<IDXGISwapChain2 *>(pReal)->GetMatrixTransform(pMatrix);
}

// IDXGISwapChain3
UINT STDMETHODCALLTYPE IWrapDXGISwapChain::GetCurrentBackBufferIndex()
{
	assert(ver_ >= 3);

	return static_cast<IDXGISwapChain3 *>(pReal)->GetCurrentBackBufferIndex();
}
HRESULT STDMETHODCALLTYPE IWrapDXGISwapChain::CheckColorSpaceSupport(DXGI_COLOR_SPACE_TYPE ColorSpace, UINT *pColorSpaceSupport)
{
	assert(ver_ >= 3);

	return static_cast<IDXGISwapChain3 *>(pReal)->CheckColorSpaceSupport(ColorSpace, pColorSpaceSupport);
}
HRESULT STDMETHODCALLTYPE IWrapDXGISwapChain::SetColorSpace1(DXGI_COLOR_SPACE_TYPE ColorSpace)
{
	assert(ver_ >= 3);

	return static_cast<IDXGISwapChain3 *>(pReal)->SetColorSpace1(ColorSpace);
}
HRESULT STDMETHODCALLTYPE IWrapDXGISwapChain::ResizeBuffers1 (UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT Format, UINT SwapChainFlags, const UINT *pCreationNodeMask, IUnknown *const *ppPresentQueue)
{
	assert(ver_ >= 3);

  SK_LOG0 ( (L"ResizeBuffers1 (...)" ), L"   DXGI   ");

  //if (SUCCEEDED (SK_DXGI_ValidateSwapChainResize (pReal, BufferCount, Width, Height, Format)))
  //  return S_OK;

  const HRESULT hr =
    static_cast<IDXGISwapChain3 *>(pReal)->ResizeBuffers1 (BufferCount, Width, Height, Format, SwapChainFlags, pCreationNodeMask, ppPresentQueue);

	if (hr == DXGI_ERROR_INVALID_CALL)
	{
	}
	else if (FAILED(hr))
	{
		return hr;
	}

	return hr;
}

#include <SpecialK/render/dxgi/dxgi_hdr.h>

HRESULT STDMETHODCALLTYPE IWrapDXGISwapChain::SetHDRMetaData(DXGI_HDR_METADATA_TYPE Type,UINT Size,void *pMetaData)
{
	assert(ver_ >= 4);

  dll_log.Log (L"HDR Metadata");

  //SK_LOG_FIRST_CALL

  if (Type == DXGI_HDR_METADATA_TYPE_HDR10)
  {
    if (Size == sizeof (DXGI_HDR_METADATA_HDR10))
    {
      DXGI_HDR_METADATA_HDR10* pData =
        (DXGI_HDR_METADATA_HDR10*)pMetaData;

      SK_DXGI_HDRControl* pHDRCtl =
        SK_HDR_GetControl ();


      if (! pHDRCtl->overrides.MaxContentLightLevel)
        pHDRCtl->meta.MaxContentLightLevel = pData->MaxContentLightLevel;
      else
        pData->MaxContentLightLevel = pHDRCtl->meta.MaxContentLightLevel;

      if (! pHDRCtl->overrides.MaxFrameAverageLightLevel)
        pHDRCtl->meta.MaxFrameAverageLightLevel = pData->MaxFrameAverageLightLevel;
      else
        pData->MaxFrameAverageLightLevel = pHDRCtl->meta.MaxFrameAverageLightLevel;


      if (! pHDRCtl->overrides.MinMaster)
        pHDRCtl->meta.MinMasteringLuminance = pData->MinMasteringLuminance;
      else
        pData->MinMasteringLuminance = pHDRCtl->meta.MinMasteringLuminance;

      if (! pHDRCtl->overrides.MaxMaster)
        pHDRCtl->meta.MaxMasteringLuminance = pData->MaxMasteringLuminance;
      else
        pData->MaxMasteringLuminance = pHDRCtl->meta.MaxMasteringLuminance;
    }
  }

	const HRESULT hr =
    static_cast<IDXGISwapChain4 *>(pReal)->SetHDRMetaData(Type,Size,pMetaData);

  SK_HDR_GetControl ()->meta._AdjustmentCount++;

  if (FAILED (hr))
  {
    if (config.render.dxgi.spoof_hdr)
      return S_OK;
  }

	return hr;
}
