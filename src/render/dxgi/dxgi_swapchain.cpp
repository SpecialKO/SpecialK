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

#define SK_LOG_ONCE(x) { static bool logged = false; if (! logged) \
                       { dll_log->Log ((x)); logged = true; } }


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


// IDXGISwapChain
HRESULT
STDMETHODCALLTYPE
IWrapDXGISwapChain::QueryInterface (REFIID riid, void **ppvObj)
{
  if (ppvObj == nullptr)
  {
    return E_POINTER;
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

  HRESULT hr =
    pReal->QueryInterface (riid, ppvObj);

  if ( riid != IID_IUnknown &&
       riid != IID_ID3DUserDefinedAnnotation )
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
  InterlockedIncrement (&refs_);

  return
    pReal->AddRef ();
}

ULONG
STDMETHODCALLTYPE
IWrapDXGISwapChain::Release (void)
{
  ULONG xrefs =
    InterlockedDecrement (&refs_);

  ULONG refs =
    pReal->Release ();

  if (xrefs == 1 || (xrefs == 2 && (SK_GetCurrentRenderBackend ().swapchain.p == this ||
                                    SK_GetCurrentRenderBackend ().swapchain.p == pReal)))
  {
    if (xrefs == 1)
    {
      SK_LOG0 ( ( L"Releasing wrapped SwapChain (%p)... device=%p, hwnd=%p", pReal, pDev.p, hWnd_),
                  L"   DXGI   ");
    }

    // We're going to make this available for recycling
    if (hWnd_ != 0)
    {
      UINT
      SK_DXGI_ReleaseSwapChainOnHWnd (
        IDXGISwapChain1* pChain,
        HWND             hWnd,
        IUnknown*        pDevice
      );

      SK_DXGI_ReleaseSwapChainOnHWnd (
        this, std::exchange (hWnd_, (HWND)0), pDev
      );

      if (SK_GetCurrentRenderBackend ().swapchain.p == this ||
          SK_GetCurrentRenderBackend ().swapchain.p == pReal)
      {
        SK_ReleaseAssert (SK_GetCurrentRenderBackend ().device.IsEqualObject (pDev) ||
                          SK_GetCurrentRenderBackend ().device.p == nullptr);

        SK_GetCurrentRenderBackend ().releaseOwnedResources ();
      }

      if (xrefs == 1)
        SK_ReleaseAssert (refs == 0);

      if (refs == 0)
      {
        if (ver_ > 0)
          InterlockedDecrement (&SK_DXGI_LiveWrappedSwapChain1s);
        else
          InterlockedDecrement (&SK_DXGI_LiveWrappedSwapChains);

        pDev.Release ();

        //delete this;
      }
    }
  }

  return refs;
}

HRESULT
STDMETHODCALLTYPE
IWrapDXGISwapChain::SetPrivateData (REFGUID Name, UINT DataSize, const void *pData)
{
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

  //return
  //  pReal->GetDevice (riid, ppDevice);
  return
    pDev->QueryInterface (riid, ppDevice);
}

HRESULT
STDMETHODCALLTYPE
IWrapDXGISwapChain::Present (UINT SyncInterval, UINT Flags)
{
  return
    SK_DXGI_DispatchPresent ( pReal, SyncInterval, Flags,
                                nullptr, SK_DXGI_PresentSource::Wrapper );
}

HRESULT
STDMETHODCALLTYPE
IWrapDXGISwapChain::GetBuffer (UINT Buffer, REFIID riid, void **ppSurface)
{
  return
    pReal->GetBuffer (Buffer, riid, ppSurface);
}

UINT
SK_DXGI_FixUpLatencyWaitFlag ( IDXGISwapChain *pSwapChain,
                               UINT             Flags,
                               BOOL            bCreation = FALSE )
{
  // This flag can only be assigned at the time of swapchain creation,
  //   if it's not present in the swapchain's description, its too late.

  DXGI_SWAP_CHAIN_DESC  desc = { };
  pSwapChain->GetDesc (&desc);

  if ( (desc.Flags &  DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT) || (bCreation && config.render.framerate.swapchain_wait > 0) )
            Flags |=  DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
  else      Flags &= ~DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

  if (_NO_ALLOW_MODE_SWITCH)
            Flags &= ~DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

  return    Flags;
}


HRESULT
STDMETHODCALLTYPE
IWrapDXGISwapChain::SetFullscreenState (BOOL Fullscreen, IDXGIOutput *pTarget)
{
  DXGI_SWAP_CHAIN_DESC  sd = { };
  pReal->GetDesc      (&sd);

  if (sd.Flags & DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT)
  {
    if (Fullscreen == TRUE)
    {
      SK_RunOnce (
        SK_ImGui_Warning ( L"Fullscreen Exclusive Mode is not Supported on Waitable SwapChains.\r\n\r\n\t"
                           L">> Please Change In-Game Settings to Borderless Window Mode ASAP to Prevent Compat. Issues" )
      );

      SK_LOG_ONCE ( L" >> Ignoring SetFullscreenState (...) on a Latency Waitable SwapChain." );

      //if (! std::exchange (fakeFullscreen_, true))
      return S_OK;
    }

    ////fakeFullscreen_ = false;
    ////
    ////// If we're latency-waitable, then we're already in windowed mode... nothing to do here.
    ////return S_OK;
  }

  return
    pReal->SetFullscreenState (Fullscreen, pTarget);
}

HRESULT
STDMETHODCALLTYPE
IWrapDXGISwapChain::GetFullscreenState (BOOL *pFullscreen, IDXGIOutput **ppTarget)
{
  ///DXGI_SWAP_CHAIN_DESC swapDesc = { };
  ///pReal->GetDesc     (&swapDesc);
  ///
  ///if (swapDesc.Flags & DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT)
  ///{
  ///  if (fakeFullscreen_)
  ///  {
  ///    if (pFullscreen)
  ///       *pFullscreen = TRUE;
  ///
  ///    if (ppTarget != nullptr)
  ///      pReal->GetContainingOutput (ppTarget);
  ///
  ///    return S_OK;
  ///  }
  ///
  ///  if ( ppTarget != nullptr)
  ///      *ppTarget =  nullptr;
  ///
  ///  if (pFullscreen)
  ///     *pFullscreen = FALSE;
  ///
  ///  return
  ///    S_OK;
  ///}

  return
    pReal->GetFullscreenState (pFullscreen, ppTarget);
}

HRESULT
STDMETHODCALLTYPE
IWrapDXGISwapChain::GetDesc (DXGI_SWAP_CHAIN_DESC *pDesc)
{
  HRESULT hr =
    pReal->GetDesc (pDesc);

  ///if (SUCCEEDED (hr))
  ///{
  ///  if (lastRequested_ == DXGI_FORMAT_UNKNOWN)
  ///      lastRequested_ =  pDesc->BufferDesc.Format;
  ///
  ///  //if (fakeFullscreen_)
  ///  //{
  ///    if (pDesc->Flags & DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT)
  ///      pDesc->Windowed = FALSE;
  ///  //}
  ///
  ///  extern bool
  ///      __SK_HDR_16BitSwap;
  ///  if (__SK_HDR_16BitSwap && pDesc->BufferDesc.Format == DXGI_FORMAT_R16G16B16A16_FLOAT)
  ///                            pDesc->BufferDesc.Format =  lastRequested_;
  ///}

  return hr;
}

HRESULT
STDMETHODCALLTYPE
IWrapDXGISwapChain::ResizeBuffers ( UINT        BufferCount,
                                    UINT        Width,     UINT Height,
                                    DXGI_FORMAT NewFormat, UINT SwapChainFlags )
{
  HRESULT hr =
    pReal->ResizeBuffers (BufferCount, Width, Height, NewFormat, SwapChainFlags);

  if (SUCCEEDED (hr))
  {
    if (Height != 0)
    gameHeight_ =
        Height;

    if (Width != 0)
    gameWidth_ =
        Width;
  }

  return hr;
}

HRESULT
STDMETHODCALLTYPE
IWrapDXGISwapChain::ResizeTarget (const DXGI_MODE_DESC *pNewTargetParameters)
{
  return
    pReal->ResizeTarget (pNewTargetParameters);
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

  return
    static_cast <IDXGISwapChain1 *>(pReal)->GetDesc1 (pDesc);
}

HRESULT
STDMETHODCALLTYPE
IWrapDXGISwapChain::GetFullscreenDesc (DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pDesc)
{
  assert (ver_ >= 1);

  return
    static_cast <IDXGISwapChain1 *>(pReal)->GetFullscreenDesc (pDesc);
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

  return
    static_cast <IDXGISwapChain2 *>(pReal)->SetMaximumFrameLatency (MaxLatency);
}

HRESULT
STDMETHODCALLTYPE
IWrapDXGISwapChain::GetMaximumFrameLatency (UINT *pMaxLatency)
{
  assert (ver_ >= 2);

  return
    static_cast <IDXGISwapChain2 *>(pReal)->GetMaximumFrameLatency (pMaxLatency);
}

HANDLE
STDMETHODCALLTYPE
IWrapDXGISwapChain::GetFrameLatencyWaitableObject(void)
{
  assert(ver_ >= 2);

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

  HRESULT
  STDMETHODCALLTYPE
  SK_DXGISwap3_SetColorSpace1_Impl (
    IDXGISwapChain3       *pSwapChain3,
    DXGI_COLOR_SPACE_TYPE  ColorSpace,
    BOOL                   bWrapped = FALSE
  );

  return
    SK_DXGISwap3_SetColorSpace1_Impl (
      static_cast <IDXGISwapChain3 *>(pReal),
                    ColorSpace, TRUE );
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

  dll_log->Log (L"[ DXGI HDR ] <*> HDR Metadata");

  //SK_LOG_FIRST_CALL

  const HRESULT hr =
    static_cast <IDXGISwapChain4 *>(pReal)->
      SetHDRMetaData (Type, Size, pMetaData);

  ////////SK_HDR_GetControl ()->meta._AdjustmentCount++;

  return hr;
}
