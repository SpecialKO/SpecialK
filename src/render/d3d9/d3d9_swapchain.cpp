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

#include <SpecialK/render/d3d9/d3d9_swapchain.h>
#include <SpecialK/render/d3d9/d3d9_device.h>
#include <algorithm>

#include <assert.h>
#include <d3d11.h>

volatile LONG SK_D3D9_LiveWrappedSwapChains   = 0;
volatile LONG SK_D3D9_LiveWrappedSwapChainsEx = 0;

// IDirect3DSwapChain9
HRESULT
STDMETHODCALLTYPE
IWrapDirect3DSwapChain9::QueryInterface (REFIID riid, void **ppvObj)
{
  if (ppvObj == nullptr)
  {
    return E_POINTER;
  }

  else if (
    //riid == __uuidof (this) ||
    riid == __uuidof (IUnknown)            ||
    riid == __uuidof (IDirect3DSwapChain9) ||
    riid == __uuidof (IDirect3DSwapChain9Ex))
  {
    #pragma region Update to IDirect3DSwapChain9Ex interface
    if (! d3d9ex_ && riid == __uuidof (IDirect3DSwapChain9Ex))
    {
      IDirect3DSwapChain9Ex *swapchainex = nullptr;
  
      if (FAILED (pReal->QueryInterface (IID_PPV_ARGS (&swapchainex))))
      {
        return E_NOINTERFACE;
      }

      pReal->Release ();

      pReal   = swapchainex;
      d3d9ex_ = true;
    }
    #pragma endregion

                                 pReal->AddRef  ();
    InterlockedExchange (&refs_, pReal->Release ());
    AddRef ();
    
    *ppvObj = this;
    
    return S_OK;
  }
  
  return pReal->QueryInterface (riid, ppvObj);
}

ULONG
STDMETHODCALLTYPE
IWrapDirect3DSwapChain9::AddRef (void)
{
  InterlockedIncrement (&refs_);

  return pReal->AddRef ();
}

ULONG
STDMETHODCALLTYPE
IWrapDirect3DSwapChain9::Release (void)
{
  if (InterlockedDecrement (&refs_) == 0)
  {
    const auto it = std::find ( pDev->additional_swapchains_.begin (),
                                pDev->additional_swapchains_.end   (), this );

    if (it != pDev->additional_swapchains_.end ())
    {
    	pDev->additional_swapchains_.erase (it);
      pDev->Release ();
    }
  }

	ULONG refs = pReal->Release ();

  if (ReadAcquire (&refs_) == 0 && refs != 0)
  {
    SK_LOG0 ( (L"Reference count for 'ID3D9SwapChain' object is inconsistent: %li, but expected 0.", refs), L"   D3D9   ");

    refs = 0;
  }

  if (refs == 0)
  {
    if (d3d9ex_)
      InterlockedDecrement (&SK_D3D9_LiveWrappedSwapChainsEx);
      InterlockedDecrement (&SK_D3D9_LiveWrappedSwapChains);

    delete this;
  }

  return refs;
}

HRESULT
STDMETHODCALLTYPE
SK_D3D9_DispatchPresent_Chain (IDirect3DSwapChain9      *This,
                         const RECT                     *pSourceRect,
                         const RECT                     *pDestRect,
                               HWND                      hDestWindowOverride,
                         const RGNDATA                  *pDirtyRegion,
                               DWORD                     dwFlags,
                               D3D9PresentSwapChain_pfn  D3D9PresentSwapChain,
                               SK_D3D9_PresentSource     Source);

HRESULT
STDMETHODCALLTYPE
IWrapDirect3DSwapChain9::Present (const RECT *pSourceRect, const RECT *pDestRect, HWND hDestWindowOverride, const RGNDATA *pDirtyRegion, DWORD dwFlags)
{ 
  return SK_D3D9_DispatchPresent_Chain ( pReal, pSourceRect,
                                           pDestRect, hDestWindowOverride,
                                             pDirtyRegion, dwFlags, nullptr,
                                               SK_D3D9_PresentSource::Wrapper );
}
HRESULT
STDMETHODCALLTYPE
IWrapDirect3DSwapChain9::GetFrontBufferData (IDirect3DSurface9 *pDestSurface)
{
	return pReal->GetFrontBufferData (pDestSurface);
}
HRESULT
STDMETHODCALLTYPE
IWrapDirect3DSwapChain9::GetBackBuffer (UINT iBackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface9 **ppBackBuffer)
{
	return pReal->GetBackBuffer (iBackBuffer, Type, ppBackBuffer);
}
HRESULT
STDMETHODCALLTYPE
IWrapDirect3DSwapChain9::GetRasterStatus (D3DRASTER_STATUS *pRasterStatus)
{
	return pReal->GetRasterStatus (pRasterStatus);
}
HRESULT
STDMETHODCALLTYPE
IWrapDirect3DSwapChain9::GetDisplayMode (D3DDISPLAYMODE *pMode)
{
	return pReal->GetDisplayMode (pMode);
}

HRESULT
STDMETHODCALLTYPE
IWrapDirect3DSwapChain9::GetDevice (IDirect3DDevice9 **ppDevice)
{
  if (ppDevice == nullptr)
  {
    return D3DERR_INVALIDCALL;
  }

  pDev->AddRef ();

  *ppDevice = pDev;

  return D3D_OK;
}
HRESULT
STDMETHODCALLTYPE
IWrapDirect3DSwapChain9::GetPresentParameters (D3DPRESENT_PARAMETERS *pPresentationParameters)
{
	return pReal->GetPresentParameters (pPresentationParameters);
}

// IDirect3DSwapChain9Ex
HRESULT
STDMETHODCALLTYPE
IWrapDirect3DSwapChain9::GetLastPresentCount (UINT *pLastPresentCount)
{
  assert(d3d9ex_);
  
  return static_cast <IDirect3DSwapChain9Ex *>(pReal)->GetLastPresentCount (pLastPresentCount);
}
HRESULT
STDMETHODCALLTYPE
IWrapDirect3DSwapChain9::GetPresentStats (D3DPRESENTSTATS *pPresentationStatistics)
{
  assert(d3d9ex_);
  
  return static_cast <IDirect3DSwapChain9Ex *>(pReal)->GetPresentStats (pPresentationStatistics);
}
HRESULT
STDMETHODCALLTYPE
IWrapDirect3DSwapChain9::GetDisplayModeEx (D3DDISPLAYMODEEX *pMode, D3DDISPLAYROTATION *pRotation)
{
  assert(_extended_interface);
  
  return static_cast <IDirect3DSwapChain9Ex *>(pReal)->GetDisplayModeEx(pMode, pRotation);
}