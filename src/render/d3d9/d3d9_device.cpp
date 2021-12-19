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
#include <SpecialK/utility.h>

#ifndef __SK_SUBSYSTEM__
#define __SK_SUBSYSTEM__ L"D3D9Device"
#endif

#include <SpecialK/render/d3d9/d3d9_swapchain.h>
#include <SpecialK/render/d3d9/d3d9_device.h>

volatile LONG SK_D3D9_LiveWrappedDevices   = 0;
volatile LONG SK_D3D9_LiveWrappedDevicesEx = 0;

HRESULT
STDMETHODCALLTYPE
IWrapDirect3DDevice9::QueryInterface (REFIID riid, void **ppvObj)
{
  if (ppvObj == nullptr)
  {
    return E_POINTER;
  }

  else if (
    riid == IID_IWrapDirect3DDevice9Ex  ||
    riid == IID_IWrapDirect3DDevice9    ||
    riid == __uuidof (IUnknown)         ||
    riid == __uuidof (IDirect3DDevice9) ||
    riid == __uuidof (IDirect3DDevice9Ex) )
  {
#pragma region Update to IDirect3DDevice9Ex interface
    if ((! d3d9ex_) && riid == __uuidof (IDirect3DDevice9Ex))
    {
      IDirect3DDevice9Ex *deviceex = nullptr;

      if (FAILED (pReal->QueryInterface (IID_PPV_ARGS (&deviceex))) || deviceex == nullptr)
      {
        return E_NOINTERFACE;
      }

      pReal->Release ();

      pReal   = deviceex;
      d3d9ex_ = true;
    }
    #pragma endregion

    // We're not wrapping that!
    if ((! d3d9ex_) && riid == IID_IWrapDirect3DDevice9Ex)
    {
      return E_NOINTERFACE;
    }

    AddRef ();

    *ppvObj = this;

    return S_OK;
  }

  HRESULT hr =
    pReal->QueryInterface (riid, ppvObj);

  if (SUCCEEDED (hr))
    InterlockedIncrement (&refs_);

  if ( riid != IID_IUnknown &&
       riid != IID_ID3DUserDefinedAnnotation &&
       riid != IID_ID3D11Device )
  {
    static bool once = false;

    if (! once)
    {
      once = true;

      wchar_t                wszGUID [41] = { };
      StringFromGUID2 (riid, wszGUID, 40);

      SK_LOG0 ( ( L"QueryInterface on wrapped D3D9 Device Context for Mystery UUID: %s",
                      wszGUID ), L"   D3D9   " );
    }
  }

  return
    hr;
}

ULONG
STDMETHODCALLTYPE
IWrapDirect3DDevice9::AddRef (void)
{
  InterlockedIncrement (&refs_);

  return
    pReal->AddRef ();
}

ULONG
STDMETHODCALLTYPE
IWrapDirect3DDevice9::Release (void)
{
  if (pReal == nullptr) // <-- Hack for "Indivisible"
    return 0;

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

    InterlockedDecrement ( d3d9ex_  ?
      &SK_D3D9_LiveWrappedDevicesEx :
      &SK_D3D9_LiveWrappedDevices );

    implicit_swapchain_ = nullptr;

    pReal = nullptr;

    SK_LOG0 ( ( L"Destroying D3D9 Device" ), L"   D3D9   ");

    delete this;
  }

  return refs;
}

HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::TestCooperativeLevel()
{
  return pReal->TestCooperativeLevel();
}
UINT STDMETHODCALLTYPE IWrapDirect3DDevice9::GetAvailableTextureMem()
{
  return pReal->GetAvailableTextureMem();
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::EvictManagedResources()
{
  return pReal->EvictManagedResources();
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::GetDirect3D(IDirect3D9 **ppD3D9)
{
  return pReal->GetDirect3D(ppD3D9);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::GetDeviceCaps(D3DCAPS9 *pCaps)
{
  return pReal->GetDeviceCaps(pCaps);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::GetDisplayMode(UINT iSwapChain, D3DDISPLAYMODE *pMode)
{
#if 1
  if (iSwapChain > 0)
  {
    if (iSwapChain < GetNumberOfSwapChains ())
    {
      return additional_swapchains_ [iSwapChain - 1]->GetDisplayMode (pMode);
    }
    else
      return D3DERR_INVALIDCALL;
  }

  return implicit_swapchain_->GetDisplayMode (pMode);
#else
  return pReal->GetDisplayMode (iSwapChain, pMode);
#endif
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS *pParameters)
{
  return pReal->GetCreationParameters(pParameters);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::SetCursorProperties(UINT XHotSpot, UINT YHotSpot, IDirect3DSurface9 *pCursorBitmap)
{
  return pReal->SetCursorProperties(XHotSpot, YHotSpot, pCursorBitmap);
}
void STDMETHODCALLTYPE IWrapDirect3DDevice9::SetCursorPosition(int X, int Y, DWORD Flags)
{
  return pReal->SetCursorPosition(X, Y, Flags);
}
BOOL STDMETHODCALLTYPE IWrapDirect3DDevice9::ShowCursor(BOOL bShow)
{
  return pReal->ShowCursor(bShow);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS *pPresentationParameters, IDirect3DSwapChain9 **ppSwapChain)
{
  SK_LOG_FIRST_CALL

#if 1
  IDirect3DSwapChain9* pTemp = nullptr;

  HRESULT hr = pReal->CreateAdditionalSwapChain (pPresentationParameters, &pTemp);

  if (SUCCEEDED (hr))
  {
    *ppSwapChain = new IWrapDirect3DSwapChain9 (this, pTemp);

    additional_swapchains_.push_back ((IWrapDirect3DSwapChain9 *)*ppSwapChain);
  }
#else
    HRESULT hr = pReal->CreateAdditionalSwapChain (pPresentationParameters, ppSwapChain);
#endif

  return hr;
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::GetSwapChain(UINT iSwapChain, IDirect3DSwapChain9 **ppSwapChain)
{
#if 1
  if (iSwapChain > 0)
  {
    if (iSwapChain < GetNumberOfSwapChains ())
    {
                                                          additional_swapchains_ [iSwapChain - 1]->AddRef ();
      *ppSwapChain = static_cast <IDirect3DSwapChain9 *> (additional_swapchains_ [iSwapChain - 1]);
      return D3D_OK;
    }
    else
      return D3DERR_INVALIDCALL;
  }
                                                      implicit_swapchain_.p->AddRef ();
  *ppSwapChain = static_cast <IDirect3DSwapChain9 *> (implicit_swapchain_.p);
  return D3D_OK;
#else
  return pReal->GetSwapChain (iSwapChain, ppSwapChain);
#endif
}
UINT STDMETHODCALLTYPE IWrapDirect3DDevice9::GetNumberOfSwapChains (void)
{
  return pReal->GetNumberOfSwapChains ();
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::Reset(D3DPRESENT_PARAMETERS *pPresentationParameters)
{
#if 0
  additional_swapchains_.clear ();
#endif

  HRESULT hr =
    static_cast <IDirect3DDevice9 *>(pReal)->Reset (pPresentationParameters);

  return hr;
}

HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::Present(const RECT *pSourceRect, const RECT *pDestRect, HWND hDestWindowOverride, const RGNDATA *pDirtyRegion)
{
  sk_d3d9_swap_dispatch_s dispatch =
  {
    pReal,               implicit_swapchain_,
    pSourceRect,         pDestRect,
    hDestWindowOverride, pDirtyRegion,
    0x00,
    nullptr,
    SK_D3D9_PresentSource::Wrapper,
    SK_D3D9_PresentType::Device9_Present
  };

  return
    SK_D3D9_Present_GrandCentral (&dispatch);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::GetBackBuffer(UINT iSwapChain, UINT iBackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface9 **ppBackBuffer)
{
#if 1
  if (iSwapChain > 0)
  {
    if (iSwapChain < GetNumberOfSwapChains ())
    {
      return additional_swapchains_ [iSwapChain - 1]->GetBackBuffer (iBackBuffer, Type, ppBackBuffer);
    }
    else
      return D3DERR_INVALIDCALL;
  }

  return implicit_swapchain_->GetBackBuffer (iBackBuffer, Type, ppBackBuffer);
#else
  return pReal->GetBackBuffer (iSwapChain, iBackBuffer, Type, ppBackBuffer);
#endif
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::GetRasterStatus(UINT iSwapChain, D3DRASTER_STATUS *pRasterStatus)
{
  return pReal->GetRasterStatus (iSwapChain, pRasterStatus);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::SetDialogBoxMode(BOOL bEnableDialogs)
{
  return pReal->SetDialogBoxMode(bEnableDialogs);
}
void STDMETHODCALLTYPE IWrapDirect3DDevice9::SetGammaRamp(UINT iSwapChain, DWORD Flags, const D3DGAMMARAMP *pRamp)
{
  // How to implement this?!
  if (iSwapChain > 0)
  {
    return;
  }

  return pReal->SetGammaRamp (iSwapChain, Flags, pRamp);
}
void STDMETHODCALLTYPE IWrapDirect3DDevice9::GetGammaRamp(UINT iSwapChain, D3DGAMMARAMP *pRamp)
{
  // How to implement this?!
  if (iSwapChain > 0)
  {
    return;
  }

  return pReal->GetGammaRamp(iSwapChain, pRamp);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::CreateTexture(UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture9 **ppTexture, HANDLE *pSharedHandle)
{
  return pReal->CreateTexture(Width, Height, Levels, Usage, Format, Pool, ppTexture, pSharedHandle);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::CreateVolumeTexture(UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DVolumeTexture9 **ppVolumeTexture, HANDLE *pSharedHandle)
{
  return pReal->CreateVolumeTexture(Width, Height, Depth, Levels, Usage, Format, Pool, ppVolumeTexture, pSharedHandle);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::CreateCubeTexture(UINT EdgeLength, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DCubeTexture9 **ppCubeTexture, HANDLE *pSharedHandle)
{
  return pReal->CreateCubeTexture(EdgeLength, Levels, Usage, Format, Pool, ppCubeTexture, pSharedHandle);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::CreateVertexBuffer(UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer9 **ppVertexBuffer, HANDLE *pSharedHandle)
{
  return pReal->CreateVertexBuffer(Length, Usage, FVF, Pool, ppVertexBuffer, pSharedHandle);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::CreateIndexBuffer(UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DIndexBuffer9 **ppIndexBuffer, HANDLE *pSharedHandle)
{
  return pReal->CreateIndexBuffer(Length, Usage, Format, Pool, ppIndexBuffer, pSharedHandle);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::CreateRenderTarget(UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Lockable, IDirect3DSurface9 **ppSurface, HANDLE *pSharedHandle)
{
  return pReal->CreateRenderTarget(Width, Height, Format, MultiSample, MultisampleQuality, Lockable, ppSurface, pSharedHandle);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::CreateDepthStencilSurface(UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Discard, IDirect3DSurface9 **ppSurface, HANDLE *pSharedHandle)
{
  return pReal->CreateDepthStencilSurface(Width, Height, Format, MultiSample, MultisampleQuality, Discard, ppSurface, pSharedHandle);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::UpdateSurface(IDirect3DSurface9 *pSourceSurface, const RECT *pSourceRect, IDirect3DSurface9 *pDestinationSurface, const POINT *pDestPoint)
{
  return pReal->UpdateSurface(pSourceSurface, pSourceRect, pDestinationSurface, pDestPoint);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::UpdateTexture(IDirect3DBaseTexture9 *pSourceTexture, IDirect3DBaseTexture9 *pDestinationTexture)
{
  return pReal->UpdateTexture(pSourceTexture, pDestinationTexture);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::GetRenderTargetData(IDirect3DSurface9 *pRenderTarget, IDirect3DSurface9 *pDestSurface)
{
  return pReal->GetRenderTargetData(pRenderTarget, pDestSurface);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::GetFrontBufferData(UINT iSwapChain, IDirect3DSurface9 *pDestSurface)
{
#if 1
  if (iSwapChain > 0)
  {
    if (iSwapChain < GetNumberOfSwapChains ())
    {
      return additional_swapchains_ [iSwapChain - 1]->GetFrontBufferData (pDestSurface);
    }
    else
      return D3DERR_INVALIDCALL;
  }

  return implicit_swapchain_->GetFrontBufferData (pDestSurface);
#else
  return pReal->GetFrontBufferData (iSwapChain, pDestSurface);
#endif
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::StretchRect(IDirect3DSurface9 *pSourceSurface, const RECT *pSourceRect, IDirect3DSurface9 *pDestSurface, const RECT *pDestRect, D3DTEXTUREFILTERTYPE Filter)
{
  return pReal->StretchRect(pSourceSurface, pSourceRect, pDestSurface, pDestRect, Filter);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::ColorFill(IDirect3DSurface9 *pSurface, const RECT *pRect, D3DCOLOR color)
{
  return pReal->ColorFill(pSurface, pRect, color);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::CreateOffscreenPlainSurface(UINT Width, UINT Height, D3DFORMAT Format, D3DPOOL Pool, IDirect3DSurface9 **ppSurface, HANDLE *pSharedHandle)
{
  return pReal->CreateOffscreenPlainSurface(Width, Height, Format, Pool, ppSurface, pSharedHandle);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::SetRenderTarget(DWORD RenderTargetIndex, IDirect3DSurface9 *pRenderTarget)
{
  return pReal->SetRenderTarget(RenderTargetIndex, pRenderTarget);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::GetRenderTarget(DWORD RenderTargetIndex, IDirect3DSurface9 **ppRenderTarget)
{
  return pReal->GetRenderTarget(RenderTargetIndex, ppRenderTarget);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::SetDepthStencilSurface(IDirect3DSurface9 *pNewZStencil)
{
  return pReal->SetDepthStencilSurface(pNewZStencil);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::GetDepthStencilSurface(IDirect3DSurface9 **ppZStencilSurface)
{
  return pReal->GetDepthStencilSurface (ppZStencilSurface);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::BeginScene()
{
  return pReal->BeginScene();
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::EndScene()
{
  return pReal->EndScene();
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::Clear(DWORD Count, const D3DRECT *pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil)
{
  return pReal->Clear(Count, pRects, Flags, Color, Z, Stencil);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::SetTransform(D3DTRANSFORMSTATETYPE State, const D3DMATRIX *pMatrix)
{
  return pReal->SetTransform(State, pMatrix);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::GetTransform(D3DTRANSFORMSTATETYPE State, D3DMATRIX *pMatrix)
{
  return pReal->GetTransform(State, pMatrix);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::MultiplyTransform(D3DTRANSFORMSTATETYPE State, const D3DMATRIX *pMatrix)
{
  return pReal->MultiplyTransform(State, pMatrix);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::SetViewport(const D3DVIEWPORT9 *pViewport)
{
  return pReal->SetViewport(pViewport);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::GetViewport(D3DVIEWPORT9 *pViewport)
{
  return pReal->GetViewport(pViewport);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::SetMaterial(const D3DMATERIAL9 *pMaterial)
{
  return pReal->SetMaterial(pMaterial);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::GetMaterial(D3DMATERIAL9 *pMaterial)
{
  return pReal->GetMaterial(pMaterial);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::SetLight(DWORD Index, const D3DLIGHT9 *pLight)
{
  return pReal->SetLight(Index, pLight);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::GetLight(DWORD Index, D3DLIGHT9 *pLight)
{
  return pReal->GetLight(Index, pLight);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::LightEnable(DWORD Index, BOOL Enable)
{
  return pReal->LightEnable(Index, Enable);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::GetLightEnable(DWORD Index, BOOL *pEnable)
{
  return pReal->GetLightEnable(Index, pEnable);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::SetClipPlane(DWORD Index, const float *pPlane)
{
  return pReal->SetClipPlane(Index, pPlane);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::GetClipPlane(DWORD Index, float *pPlane)
{
  return pReal->GetClipPlane(Index, pPlane);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::SetRenderState(D3DRENDERSTATETYPE State, DWORD Value)
{
  return pReal->SetRenderState(State, Value);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::GetRenderState(D3DRENDERSTATETYPE State, DWORD *pValue)
{
  return pReal->GetRenderState(State, pValue);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::CreateStateBlock(D3DSTATEBLOCKTYPE Type, IDirect3DStateBlock9 **ppSB)
{
  return pReal->CreateStateBlock(Type, ppSB);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::BeginStateBlock()
{
  return pReal->BeginStateBlock();
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::EndStateBlock(IDirect3DStateBlock9 **ppSB)
{
  return pReal->EndStateBlock(ppSB);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::SetClipStatus(const D3DCLIPSTATUS9 *pClipStatus)
{
  return pReal->SetClipStatus(pClipStatus);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::GetClipStatus(D3DCLIPSTATUS9 *pClipStatus)
{
  return pReal->GetClipStatus(pClipStatus);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::GetTexture(DWORD Stage, IDirect3DBaseTexture9 **ppTexture)
{
  return pReal->GetTexture(Stage, ppTexture);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::SetTexture(DWORD Stage, IDirect3DBaseTexture9 *pTexture)
{
  return pReal->SetTexture(Stage, pTexture);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::GetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD *pValue)
{
  return pReal->GetTextureStageState(Stage, Type, pValue);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::SetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value)
{
  return pReal->SetTextureStageState(Stage, Type, Value);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::GetSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD *pValue)
{
  return pReal->GetSamplerState(Sampler, Type, pValue);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::SetSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value)
{
  return pReal->SetSamplerState(Sampler, Type, Value);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::ValidateDevice(DWORD *pNumPasses)
{
  return pReal->ValidateDevice(pNumPasses);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::SetPaletteEntries(UINT PaletteNumber, const PALETTEENTRY *pEntries)
{
  return pReal->SetPaletteEntries(PaletteNumber, pEntries);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::GetPaletteEntries(UINT PaletteNumber, PALETTEENTRY *pEntries)
{
  return pReal->GetPaletteEntries(PaletteNumber, pEntries);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::SetCurrentTexturePalette(UINT PaletteNumber)
{
  return pReal->SetCurrentTexturePalette(PaletteNumber);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::GetCurrentTexturePalette(UINT *PaletteNumber)
{
  return pReal->GetCurrentTexturePalette(PaletteNumber);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::SetScissorRect(const RECT *pRect)
{
  return pReal->SetScissorRect(pRect);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::GetScissorRect(RECT *pRect)
{
  return pReal->GetScissorRect(pRect);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::SetSoftwareVertexProcessing(BOOL bSoftware)
{
  return pReal->SetSoftwareVertexProcessing(bSoftware);
}
BOOL STDMETHODCALLTYPE IWrapDirect3DDevice9::GetSoftwareVertexProcessing()
{
  return pReal->GetSoftwareVertexProcessing();
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::SetNPatchMode(float nSegments)
{
  return pReal->SetNPatchMode(nSegments);
}
float STDMETHODCALLTYPE IWrapDirect3DDevice9::GetNPatchMode()
{
  return pReal->GetNPatchMode();
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::DrawPrimitive(D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount)
{
  return pReal->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::DrawIndexedPrimitive(D3DPRIMITIVETYPE PrimitiveType, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT StartIndex, UINT PrimitiveCount)
{
  return pReal->DrawIndexedPrimitive(PrimitiveType, BaseVertexIndex, MinVertexIndex, NumVertices, StartIndex, PrimitiveCount);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::DrawPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, const void *pVertexStreamZeroData, UINT VertexStreamZeroStride)
{
  return pReal->DrawPrimitiveUP(PrimitiveType, PrimitiveCount, pVertexStreamZeroData, VertexStreamZeroStride);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertices, UINT PrimitiveCount, const void *pIndexData, D3DFORMAT IndexDataFormat, const void *pVertexStreamZeroData, UINT VertexStreamZeroStride)
{
  return pReal->DrawIndexedPrimitiveUP(PrimitiveType, MinVertexIndex, NumVertices, PrimitiveCount, pIndexData, IndexDataFormat, pVertexStreamZeroData, VertexStreamZeroStride);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::ProcessVertices(UINT SrcStartIndex, UINT DestIndex, UINT VertexCount, IDirect3DVertexBuffer9 *pDestBuffer, IDirect3DVertexDeclaration9 *pVertexDecl, DWORD Flags)
{
  return pReal->ProcessVertices(SrcStartIndex, DestIndex, VertexCount, pDestBuffer, pVertexDecl, Flags);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::CreateVertexDeclaration(const D3DVERTEXELEMENT9 *pVertexElements, IDirect3DVertexDeclaration9 **ppDecl)
{
  return pReal->CreateVertexDeclaration(pVertexElements, ppDecl);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::SetVertexDeclaration(IDirect3DVertexDeclaration9 *pDecl)
{
  return pReal->SetVertexDeclaration(pDecl);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::GetVertexDeclaration(IDirect3DVertexDeclaration9 **ppDecl)
{
  return pReal->GetVertexDeclaration(ppDecl);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::SetFVF(DWORD FVF)
{
  return pReal->SetFVF(FVF);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::GetFVF(DWORD *pFVF)
{
  return pReal->GetFVF(pFVF);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::CreateVertexShader(const DWORD *pFunction, IDirect3DVertexShader9 **ppShader)
{
  return pReal->CreateVertexShader(pFunction, ppShader);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::SetVertexShader(IDirect3DVertexShader9 *pShader)
{
  return pReal->SetVertexShader(pShader);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::GetVertexShader(IDirect3DVertexShader9 **ppShader)
{
  return pReal->GetVertexShader(ppShader);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::SetVertexShaderConstantF(UINT StartRegister, const float *pConstantData, UINT Vector4fCount)
{
  return pReal->SetVertexShaderConstantF(StartRegister, pConstantData, Vector4fCount);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::GetVertexShaderConstantF(UINT StartRegister, float *pConstantData, UINT Vector4fCount)
{
  return pReal->GetVertexShaderConstantF(StartRegister, pConstantData, Vector4fCount);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::SetVertexShaderConstantI(UINT StartRegister, const int *pConstantData, UINT Vector4iCount)
{
  return pReal->SetVertexShaderConstantI(StartRegister, pConstantData, Vector4iCount);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::GetVertexShaderConstantI(UINT StartRegister, int *pConstantData, UINT Vector4iCount)
{
  return pReal->GetVertexShaderConstantI(StartRegister, pConstantData, Vector4iCount);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::SetVertexShaderConstantB(UINT StartRegister, const BOOL *pConstantData, UINT  BoolCount)
{
  return pReal->SetVertexShaderConstantB(StartRegister, pConstantData, BoolCount);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::GetVertexShaderConstantB(UINT StartRegister, BOOL *pConstantData, UINT BoolCount)
{
  return pReal->GetVertexShaderConstantB(StartRegister, pConstantData, BoolCount);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::SetStreamSource(UINT StreamNumber, IDirect3DVertexBuffer9 *pStreamData, UINT OffsetInBytes, UINT Stride)
{
  return pReal->SetStreamSource(StreamNumber, pStreamData, OffsetInBytes, Stride);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::GetStreamSource(UINT StreamNumber, IDirect3DVertexBuffer9 **ppStreamData, UINT *OffsetInBytes, UINT *pStride)
{
  return pReal->GetStreamSource(StreamNumber, ppStreamData, OffsetInBytes, pStride);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::SetStreamSourceFreq(UINT StreamNumber, UINT Divider)
{
  return pReal->SetStreamSourceFreq(StreamNumber, Divider);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::GetStreamSourceFreq(UINT StreamNumber, UINT *Divider)
{
  return pReal->GetStreamSourceFreq(StreamNumber, Divider);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::SetIndices(IDirect3DIndexBuffer9 *pIndexData)
{
  return pReal->SetIndices(pIndexData);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::GetIndices(IDirect3DIndexBuffer9 **ppIndexData)
{
  return pReal->GetIndices(ppIndexData);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::CreatePixelShader(const DWORD *pFunction, IDirect3DPixelShader9 **ppShader)
{
  return pReal->CreatePixelShader(pFunction, ppShader);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::SetPixelShader(IDirect3DPixelShader9 *pShader)
{
  return pReal->SetPixelShader(pShader);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::GetPixelShader(IDirect3DPixelShader9 **ppShader)
{
  return pReal->GetPixelShader(ppShader);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::SetPixelShaderConstantF(UINT StartRegister, const float *pConstantData, UINT Vector4fCount)
{
  return pReal->SetPixelShaderConstantF(StartRegister, pConstantData, Vector4fCount);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::GetPixelShaderConstantF(UINT StartRegister, float *pConstantData, UINT Vector4fCount)
{
  return pReal->GetPixelShaderConstantF(StartRegister, pConstantData, Vector4fCount);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::SetPixelShaderConstantI(UINT StartRegister, const int *pConstantData, UINT Vector4iCount)
{
  return pReal->SetPixelShaderConstantI(StartRegister, pConstantData, Vector4iCount);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::GetPixelShaderConstantI(UINT StartRegister, int *pConstantData, UINT Vector4iCount)
{
  return pReal->GetPixelShaderConstantI(StartRegister, pConstantData, Vector4iCount);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::SetPixelShaderConstantB(UINT StartRegister, const BOOL *pConstantData, UINT  BoolCount)
{
  return pReal->SetPixelShaderConstantB(StartRegister, pConstantData, BoolCount);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::GetPixelShaderConstantB(UINT StartRegister, BOOL *pConstantData, UINT BoolCount)
{
  return pReal->GetPixelShaderConstantB(StartRegister, pConstantData, BoolCount);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::DrawRectPatch(UINT Handle, const float *pNumSegs, const D3DRECTPATCH_INFO *pRectPatchInfo)
{
  return pReal->DrawRectPatch(Handle, pNumSegs, pRectPatchInfo);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::DrawTriPatch(UINT Handle, const float *pNumSegs, const D3DTRIPATCH_INFO *pTriPatchInfo)
{
  return pReal->DrawTriPatch(Handle, pNumSegs, pTriPatchInfo);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::DeletePatch(UINT Handle)
{
  return pReal->DeletePatch(Handle);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::CreateQuery(D3DQUERYTYPE Type, IDirect3DQuery9 **ppQuery)
{
  return pReal->CreateQuery(Type, ppQuery);
}

// IDirect3DDevice9Ex
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::SetConvolutionMonoKernel(UINT width, UINT height, float *rows, float *columns)
{
  assert (d3d9ex_);

  return static_cast<IDirect3DDevice9Ex *>(pReal)->SetConvolutionMonoKernel(width, height, rows, columns);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::ComposeRects(IDirect3DSurface9 *pSrc, IDirect3DSurface9 *pDst, IDirect3DVertexBuffer9 *pSrcRectDescs, UINT NumRects, IDirect3DVertexBuffer9 *pDstRectDescs, D3DCOMPOSERECTSOP Operation, int Xoffset, int Yoffset)
{
  assert (d3d9ex_);

  return static_cast<IDirect3DDevice9Ex *>(pReal)->ComposeRects(pSrc, pDst, pSrcRectDescs, NumRects, pDstRectDescs, Operation, Xoffset, Yoffset);
}

HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::PresentEx(const RECT *pSourceRect, const RECT *pDestRect, HWND hDestWindowOverride, const RGNDATA *pDirtyRegion, DWORD dwFlags)
{
  assert (d3d9ex_);

  sk_d3d9_swap_dispatch_s dispatch =
  {
    pReal,               implicit_swapchain_,
    pSourceRect,         pDestRect,
    hDestWindowOverride, pDirtyRegion,
    dwFlags,
    nullptr,
    SK_D3D9_PresentSource::Wrapper,
    SK_D3D9_PresentType::Device9Ex_PresentEx
  };

  return
    SK_D3D9_Present_GrandCentral (&dispatch);
}

HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::GetGPUThreadPriority(INT *pPriority)
{
  assert (d3d9ex_);

  return static_cast<IDirect3DDevice9Ex *>(pReal)->GetGPUThreadPriority(pPriority);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::SetGPUThreadPriority(INT Priority)
{
  assert (d3d9ex_);

  return static_cast<IDirect3DDevice9Ex *>(pReal)->SetGPUThreadPriority(Priority);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::WaitForVBlank(UINT iSwapChain)
{
#if 0
  if (iSwapChain > 0)
  {
    if (iSwapChain < GetNumberOfSwapChains ())
    {
      return additional_swapchains_ [iSwapChain - 1]->GetFrontBufferData (pDestSurface);
    }
    else
      return D3DERR_INVALIDCALL;
  }
#endif

  return static_cast<IDirect3DDevice9Ex *>(pReal)->WaitForVBlank (iSwapChain);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::CheckResourceResidency(IDirect3DResource9 **pResourceArray, UINT32 NumResources)
{
  assert (d3d9ex_);

  return static_cast<IDirect3DDevice9Ex *>(pReal)->CheckResourceResidency(pResourceArray, NumResources);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::SetMaximumFrameLatency(UINT MaxLatency)
{
  assert (d3d9ex_);

  return static_cast<IDirect3DDevice9Ex *>(pReal)->SetMaximumFrameLatency(MaxLatency);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::GetMaximumFrameLatency(UINT *pMaxLatency)
{
  assert (d3d9ex_);

  return static_cast<IDirect3DDevice9Ex *>(pReal)->GetMaximumFrameLatency(pMaxLatency);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::CheckDeviceState(HWND hDestinationWindow)
{
  assert (d3d9ex_);

  return static_cast<IDirect3DDevice9Ex *>(pReal)->CheckDeviceState(hDestinationWindow);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::CreateRenderTargetEx(UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Lockable, IDirect3DSurface9 **ppSurface, HANDLE *pSharedHandle, DWORD Usage)
{
  assert (d3d9ex_);

  return static_cast<IDirect3DDevice9Ex *>(pReal)->CreateRenderTargetEx(Width, Height, Format, MultiSample, MultisampleQuality, Lockable, ppSurface, pSharedHandle, Usage);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::CreateOffscreenPlainSurfaceEx(UINT Width, UINT Height, D3DFORMAT Format, D3DPOOL Pool, IDirect3DSurface9 **ppSurface, HANDLE *pSharedHandle, DWORD Usage)
{
  assert (d3d9ex_);

  return static_cast<IDirect3DDevice9Ex *>(pReal)->CreateOffscreenPlainSurfaceEx(Width, Height, Format, Pool, ppSurface, pSharedHandle, Usage);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::CreateDepthStencilSurfaceEx(UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Discard, IDirect3DSurface9 **ppSurface, HANDLE *pSharedHandle, DWORD Usage)
{
  assert (d3d9ex_);

  return static_cast<IDirect3DDevice9Ex *>(pReal)->CreateDepthStencilSurfaceEx(Width, Height, Format, MultiSample, MultisampleQuality, Discard, ppSurface, pSharedHandle, Usage);
}
HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::ResetEx(D3DPRESENT_PARAMETERS *pPresentationParameters, D3DDISPLAYMODEEX *pFullscreenDisplayMode)
{
  assert (d3d9ex_);

  HRESULT hr =
    static_cast <IDirect3DDevice9Ex *>(pReal)->ResetEx (pPresentationParameters, pFullscreenDisplayMode);

  return hr;
}

HRESULT STDMETHODCALLTYPE IWrapDirect3DDevice9::GetDisplayModeEx(UINT iSwapChain, D3DDISPLAYMODEEX *pMode, D3DDISPLAYROTATION *pRotation)
{
  assert (d3d9ex_);

#if 1
  if (iSwapChain > 0)
  {
    if (iSwapChain < GetNumberOfSwapChains ())
    {
      return additional_swapchains_ [iSwapChain - 1]->GetDisplayModeEx (pMode, pRotation);
    }
    else
      return D3DERR_INVALIDCALL;
  }


  return
    implicit_swapchain_->GetDisplayModeEx (
                         pMode, pRotation );
#else
  return static_cast <IDirect3DDevice9Ex *>(pReal)->GetDisplayModeEx (iSwapChain, pMode, pRotation);
#endif
}
