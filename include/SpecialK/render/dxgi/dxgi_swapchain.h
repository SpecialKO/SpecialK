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

#ifndef __SK__DXGI_SWAPCHAIN_H__
#define __SK__DXGI_SWAPCHAIN_H__

#include <SpecialK/render/dxgi/dxgi_interfaces.h>
#include <SpecialK/render/dxgi/dxgi_util.h>

extern volatile LONG SK_DXGI_LiveWrappedSwapChains;
extern volatile LONG SK_DXGI_LiveWrappedSwapChain1s;

interface ID3D11Device;
interface ID3D12Device;
#include <atlbase.h>

#ifndef __SK_SUBSYSTEM__
#define __SK_SUBSYSTEM__ L"IDXGI Wrap"
#endif

const GUID IID_IUnwrappedDXGISwapChain =
{ 0xe8a33b4a, 0x1405, 0x424c, { 0xae, 0x88, 0xd, 0x3e, 0x9d, 0x46, 0xc9, 0x14 } };

// {24430A12-6E3C-4706-AFFA-B3EEF2DF4102}
const GUID IID_IWrapDXGISwapChain =
{ 0x24430a12, 0x6e3c, 0x4706, { 0xaf, 0xfa, 0xb3, 0xee, 0xf2, 0xdf, 0x41, 0x2 } };

void
__stdcall
SK_DXGI_SwapChainDestructionCallback (void *pSwapChain);

struct DECLSPEC_UUID("24430A12-6E3C-4706-AFFA-B3EEF2DF4102")
IWrapDXGISwapChain : IDXGISwapChain4
{
  IWrapDXGISwapChain ( ID3D11Device   *pDevice,
                       IDXGISwapChain *pSwapChain ) : pReal (pSwapChain),
                                                      pDev  (pDevice),
                                                      ver_  (0)
  {
    if (pSwapChain == nullptr || pDevice == nullptr)
      return;

    auto& rb =
      SK_GetCurrentRenderBackend ();

    DXGI_SWAP_CHAIN_DESC            sd = { };
    if (SUCCEEDED (pReal->GetDesc (&sd)))
    {
      hWnd_     = sd.OutputWindow;
      waitable_ = sd.Flags & DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

      state_cache_s state_cache;
      
      if (sd.BufferDesc.Format != DXGI_FORMAT_R16G16B16A16_FLOAT)
        state_cache.lastNonHDRFormat = sd.BufferDesc.Format;

      SK_DXGI_SetPrivateData ( pReal, SKID_DXGI_SwapChain_StateCache,
                                                  sizeof (state_cache_s),
                                                         &state_cache );

      void *pThis = this;
      SK_DXGI_SetPrivateData (pReal, SKID_DXGI_WrappedSwapChain, sizeof (void *), &pThis);
    }

    IUnknown *pPromotion = nullptr;

    if (SUCCEEDED (QueryInterface (IID_IDXGISwapChain4, reinterpret_cast <void **> (&pPromotion))))
    {
      ver_ = 4;
    }

    else if (SUCCEEDED (QueryInterface (IID_IDXGISwapChain3, reinterpret_cast <void **> (&pPromotion))))
    {
      ver_ = 3;
    }

    else if (SUCCEEDED (QueryInterface (IID_IDXGISwapChain2, reinterpret_cast <void **> (&pPromotion))))
    {
      ver_ = 2;
    }

    else if (SUCCEEDED (QueryInterface (IID_IDXGISwapChain1, reinterpret_cast <void **> (&pPromotion))))
    {
      ver_ = 1;
    }

    if (ver_ != 0)
    {
      static_cast <IDXGISwapChain1 *>
        (pReal)->GetHwnd (&hWnd_);

      SK_ReleaseAssert (sd.OutputWindow == 0 || sd.OutputWindow == hWnd_);

      SK_LOG0 ( ( L"Promoted IDXGISwapChain to IDXGISwapChain%lu", ver_),
                  __SK_SUBSYSTEM__ );
    }

    InterlockedIncrement (ver_ > 0 ? &SK_DXGI_LiveWrappedSwapChain1s
                                   : &SK_DXGI_LiveWrappedSwapChains);

    SK_ComQIPtr <ID3D12Device>
             pDev12 ( pDevice );
    d3d12_ = pDev12.p != nullptr;

    flip_model.active =
      ( sd.SwapEffect == DXGI_SWAP_EFFECT_FLIP_DISCARD ||
        sd.SwapEffect == DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL );
    flip_model.native = rb.active_traits.bOriginallyFlip;

    SetPrivateDataInterface (IID_IUnwrappedDXGISwapChain, pReal);

    SK_DXGI_SetDebugName ( pReal,
        SK_FormatStringW ( L"SK_IWrapDXGISwapChain: pReal=%p", pReal ) );

    SK_ComQIPtr <IDXGISwapChain2>
        pSwapChain2 (pReal);
    if (pSwapChain2.p != nullptr && (sd.Flags & DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT))
        pSwapChain2->GetMaximumFrameLatency (&gameFrameLatency_);
    else if (pDev12.p != nullptr)
    {
      SK_ComQIPtr <IDXGIDevice1>
           pDXGIDev12 (pDev12.p);
           pDXGIDev12->GetMaximumFrameLatency (&gameFrameLatency_);
    }

    RegisterDestructionCallback ();

    if (! d3d12_)
    {
      SK_ComQIPtr <IDXGIDevice1>
           pDXGIDev11 (pDevice);
           pDXGIDev11->GetMaximumFrameLatency (&gameFrameLatency_);

      SK_ComPtr <ID3D11DeviceContext> pDevCtx;
      pDevice->GetImmediateContext  (&pDevCtx);

      SK_ComQIPtr <ID3D11DeviceContext1>
          pDevCtx1 (pDevCtx);
      if (pDevCtx1.p != nullptr)
      {
        if (pDevice->GetFeatureLevel () >= D3D_FEATURE_LEVEL_11_1)
        {
          pDevice->CheckFeatureSupport (
             D3D11_FEATURE_D3D11_OPTIONS, &_d3d11_feature_opts, 
               sizeof (D3D11_FEATURE_DATA_D3D11_OPTIONS)
          );
        }
      }
    }
  }

  IWrapDXGISwapChain ( ID3D11Device         *pDevice,
                       IDXGISwapChain1      *pSwapChain ) :
                       ///DXGI_SWAP_CHAIN_DESC *pOrigDesc,
                       ///DXGI_SWAP_CHAIN_DESC *pOverrideDesc ) :
    pReal (pSwapChain),
    pDev  (pDevice),
    ver_  (1)
  {
    if (pSwapChain == nullptr || pDevice == nullptr)
      return;

    auto& rb =
      SK_GetCurrentRenderBackend ();

    ///creation_desc.actual    = *pOverrideDesc;
    ///creation_desc.requested = *pOrigDesc;

    DXGI_SWAP_CHAIN_DESC            sd = { };
    if (SUCCEEDED (pReal->GetDesc (&sd)))
    {
      hWnd_     = sd.OutputWindow;
      waitable_ = sd.Flags & DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

      state_cache_s state_cache;

      if (sd.BufferDesc.Format != DXGI_FORMAT_R16G16B16A16_FLOAT)
        state_cache.lastNonHDRFormat  = sd.BufferDesc.Format;

      SK_DXGI_SetPrivateData ( pReal, SKID_DXGI_SwapChain_StateCache,
                                            sizeof (state_cache_s),
                                                   &state_cache );

      void *pThis = this;
      SK_DXGI_SetPrivateData (pReal, SKID_DXGI_WrappedSwapChain, sizeof (void *), &pThis);
    }

    IUnknown *pPromotion = nullptr;

    if (SUCCEEDED (QueryInterface (IID_IDXGISwapChain4, reinterpret_cast <void **> (&pPromotion))))
    {
      ver_ = 4;
    }

    else if (SUCCEEDED (QueryInterface (IID_IDXGISwapChain3, reinterpret_cast <void **> (&pPromotion))))
    {
      ver_ = 3;
    }

    else if (SUCCEEDED (QueryInterface (IID_IDXGISwapChain2, reinterpret_cast <void **> (&pPromotion))))
    {
      ver_ = 2;
    }

    if (ver_ != 1)
    {
      SK_LOG0 ( ( L"Promoted IDXGISwapChain1 to IDXGISwapChain%lu", ver_),
                  __SK_SUBSYSTEM__ );
    }

    InterlockedIncrement (&SK_DXGI_LiveWrappedSwapChain1s);

    static_cast <IDXGISwapChain1 *>
      (pReal)->GetHwnd (&hWnd_);

    SK_ReleaseAssert (sd.OutputWindow == 0 || sd.OutputWindow == hWnd_);

    SK_ComQIPtr <ID3D12Device>
             pDev12 ( pDevice );
    d3d12_ = pDev12.p != nullptr;

    flip_model.active =
      ( sd.SwapEffect == DXGI_SWAP_EFFECT_FLIP_DISCARD ||
        sd.SwapEffect == DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL );
    flip_model.native = rb.active_traits.bOriginallyFlip;

    SetPrivateDataInterface (IID_IUnwrappedDXGISwapChain, pReal);

    SK_DXGI_SetDebugName ( pReal,
        SK_FormatStringW ( L"SK_IWrapDXGISwapChain: pReal=%p", pReal ) );

    SK_ComQIPtr <IDXGISwapChain2>
        pSwapChain2 (pReal);
    if (pSwapChain2.p != nullptr && (sd.Flags & DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT))
        pSwapChain2->GetMaximumFrameLatency (&gameFrameLatency_);
    else if (pDev12.p != nullptr)
    {
      SK_ComQIPtr <IDXGIDevice1>
           pDXGIDev12 (pDev12.p);
           pDXGIDev12->GetMaximumFrameLatency (&gameFrameLatency_);
    }

    RegisterDestructionCallback ();

    if (! d3d12_)
    {
      SK_ComQIPtr <IDXGIDevice1>
           pDXGIDev11 (pDevice);
           pDXGIDev11->GetMaximumFrameLatency (&gameFrameLatency_);

      SK_ComPtr <ID3D11DeviceContext> pDevCtx;
      pDevice->GetImmediateContext  (&pDevCtx);

      SK_ComQIPtr <ID3D11DeviceContext1>
          pDevCtx1 (pDevCtx);
      if (pDevCtx1.p != nullptr)
      {
        if (pDevice->GetFeatureLevel () >= D3D_FEATURE_LEVEL_11_1)
        {
          pDevice->CheckFeatureSupport (
             D3D11_FEATURE_D3D11_OPTIONS, &_d3d11_feature_opts, 
               sizeof (D3D11_FEATURE_DATA_D3D11_OPTIONS)
          );
        }
      }
    }
  }


  //virtual ~IWrapDXGISwapChain (void)
  //{
  //}


  IWrapDXGISwapChain            (const IWrapDXGISwapChain &) = delete;
  IWrapDXGISwapChain &operator= (const IWrapDXGISwapChain &) = delete;

  #pragma region IUnknown
  virtual HRESULT STDMETHODCALLTYPE QueryInterface (REFIID riid, void **ppvObj) override; // 0
  virtual ULONG   STDMETHODCALLTYPE AddRef         (void)                       override; // 1
  virtual ULONG   STDMETHODCALLTYPE Release        (void)                       override; // 2
  #pragma endregion

  #pragma region IDXGIObject
  virtual HRESULT STDMETHODCALLTYPE SetPrivateData          (REFGUID Name,        UINT       DataSize, const void *pData) override; // 3
  virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface (REFGUID Name, const  IUnknown  *pUnknown)                    override; // 4
  virtual HRESULT STDMETHODCALLTYPE GetPrivateData          (REFGUID Name,        UINT      *pDataSize,      void *pData) override; // 5
  virtual HRESULT STDMETHODCALLTYPE GetParent               (REFIID  riid,        void     **ppParent)                    override; // 6
  #pragma endregion

  #pragma region IDXGIDeviceSubObject
  virtual HRESULT STDMETHODCALLTYPE GetDevice (REFIID riid, void **ppDevice) override; // 7
  #pragma endregion

  #pragma region IDXGISwapChain
  virtual HRESULT STDMETHODCALLTYPE Present             (      UINT                   SyncInterval, UINT          Flags)                   override; // 8
  virtual HRESULT STDMETHODCALLTYPE GetBuffer           (      UINT                   Buffer,       REFIID        riid,  void **ppSurface) override; // 9
  virtual HRESULT STDMETHODCALLTYPE SetFullscreenState  (      BOOL                   Fullscreen,   IDXGIOutput  *pTarget)                 override; // 10
  virtual HRESULT STDMETHODCALLTYPE GetFullscreenState  (      BOOL                   *pFullscreen, IDXGIOutput **ppTarget)                override; // 11
  virtual HRESULT STDMETHODCALLTYPE GetDesc             (      DXGI_SWAP_CHAIN_DESC   *pDesc)                                              override; // 12
  virtual HRESULT STDMETHODCALLTYPE ResizeBuffers       (      UINT                    BufferCount, UINT          Width, UINT   Height,
                                                               DXGI_FORMAT             NewFormat,   UINT          SwapChainFlags )         override; // 13
  virtual HRESULT STDMETHODCALLTYPE ResizeTarget        (const DXGI_MODE_DESC         *pNewTargetParameters)                               override; // 14
  virtual HRESULT STDMETHODCALLTYPE GetContainingOutput (      IDXGIOutput           **ppOutput)                                           override; // 15
  virtual HRESULT STDMETHODCALLTYPE GetFrameStatistics  (      DXGI_FRAME_STATISTICS  *pStats)                                             override; // 16
  virtual HRESULT STDMETHODCALLTYPE GetLastPresentCount (      UINT                   *pLastPresentCount)                                  override; // 17
  #pragma endregion

  #pragma region IDXGISwapChain1
  virtual HRESULT STDMETHODCALLTYPE GetDesc1                 (      DXGI_SWAP_CHAIN_DESC1            *pDesc)                override; // 18
  virtual HRESULT STDMETHODCALLTYPE GetFullscreenDesc        (      DXGI_SWAP_CHAIN_FULLSCREEN_DESC  *pDesc)                override; // 19
  virtual HRESULT STDMETHODCALLTYPE GetHwnd                  (      HWND                             *pHwnd)                override; // 20
  virtual HRESULT STDMETHODCALLTYPE GetCoreWindow            (      REFIID                            refiid, void **ppUnk) override; // 21
  virtual HRESULT STDMETHODCALLTYPE Present1                 (      UINT                              SyncInterval,
                                                                    UINT                              PresentFlags,
                                                              const DXGI_PRESENT_PARAMETERS          *pPresentParameters)   override; // 22
  virtual BOOL    STDMETHODCALLTYPE IsTemporaryMonoSupported (      void)                                                   override; // 23
  virtual HRESULT STDMETHODCALLTYPE GetRestrictToOutput      (      IDXGIOutput                     **ppRestrictToOutput)   override; // 24
  virtual HRESULT STDMETHODCALLTYPE SetBackgroundColor       (const DXGI_RGBA                        *pColor)               override; // 25
  virtual HRESULT STDMETHODCALLTYPE GetBackgroundColor       (      DXGI_RGBA                        *pColor)               override; // 26
  virtual HRESULT STDMETHODCALLTYPE SetRotation              (      DXGI_MODE_ROTATION                Rotation)             override; // 27
  virtual HRESULT STDMETHODCALLTYPE GetRotation              (      DXGI_MODE_ROTATION               *pRotation)            override; // 28
  #pragma endregion

  #pragma region IDXGISwapChain2
  virtual HRESULT STDMETHODCALLTYPE SetSourceSize                 (      UINT               Width,  UINT  Height)  override; // 29
  virtual HRESULT STDMETHODCALLTYPE GetSourceSize                 (      UINT              *pWidth, UINT *pHeight) override; // 30
  virtual HRESULT STDMETHODCALLTYPE SetMaximumFrameLatency        (      UINT               MaxLatency)            override; // 31
  virtual HRESULT STDMETHODCALLTYPE GetMaximumFrameLatency        (      UINT              *pMaxLatency)           override; // 32
  virtual HANDLE  STDMETHODCALLTYPE GetFrameLatencyWaitableObject (      void)                                     override; // 33
  virtual HRESULT STDMETHODCALLTYPE SetMatrixTransform            (const DXGI_MATRIX_3X2_F *pMatrix)               override; // 34
  virtual HRESULT STDMETHODCALLTYPE GetMatrixTransform            (      DXGI_MATRIX_3X2_F *pMatrix)               override; // 35
  #pragma endregion
  #pragma region IDXGISwapChain3
  virtual UINT    STDMETHODCALLTYPE GetCurrentBackBufferIndex (void)                                                                         override; // 36
  virtual HRESULT STDMETHODCALLTYPE CheckColorSpaceSupport    (DXGI_COLOR_SPACE_TYPE  ColorSpace,           UINT        *pColorSpaceSupport) override; // 37
  virtual HRESULT STDMETHODCALLTYPE SetColorSpace1            (DXGI_COLOR_SPACE_TYPE  ColorSpace)                                            override; // 38
  virtual HRESULT STDMETHODCALLTYPE ResizeBuffers1            (UINT                   BufferCount,          UINT         Width,
                                                               UINT                   Height,               DXGI_FORMAT  Format,
                                                               UINT                   SwapChainFlags, const UINT        *pCreationNodeMask,
                                                               IUnknown *const       *ppPresentQueue)                                        override; // 39
  #pragma endregion
  #pragma region IDXGISwapChain4
  virtual HRESULT STDMETHODCALLTYPE SetHDRMetaData (DXGI_HDR_METADATA_TYPE Type, UINT Size, void *pMetaData) override; // 40
  #pragma endregion

  volatile LONG         refs_ = 1;
  IDXGISwapChain       *pReal;
  SK_ComPtr <IUnknown>  pDev;
  unsigned int          ver_;
  HWND                  hWnd_ = 0;

  bool                  d3d12_          = false;
  bool                  waitable_       = false;
  ID3D12CommandQueue*   d3d12_queue_    = nullptr;

  struct {
    bool                active          = false;
    bool                native          = false;
    int                 last_srgb_mode  =    -2;

    bool                isOverrideActive (void)
    {
      return 
        active && (! native);
    }
  } flip_model;

  UINT                  gameWidth_        = 0;
  UINT                  gameHeight_       = 0;
  UINT                  gameFrameLatency_ = 2; // The latency the game expects

  struct state_cache_s {
    DXGI_FORMAT           lastRequested_      = DXGI_FORMAT_UNKNOWN;
    DXGI_FORMAT           lastNonHDRFormat    = DXGI_FORMAT_UNKNOWN;
    DXGI_COLOR_SPACE_TYPE lastColorSpace_     = DXGI_COLOR_SPACE_RESERVED;

    UINT                  lastWidth           = 0;
    UINT                  lastHeight          = 0;
    DXGI_FORMAT           lastFormat          = DXGI_FORMAT_UNKNOWN;
    UINT                  lastBufferCount     = 0;
    bool                  lastFullscreenState = false;

    bool                  _fakefullscreen     = false;
    bool                  _stalebuffers       = false;
  };

  std::recursive_mutex  _backbufferLock;
  std::unordered_map <UINT, SK_ComPtr <ID3D11Texture2D>>
                        _backbuffers;

  D3D11_FEATURE_DATA_D3D11_OPTIONS _d3d11_feature_opts = { };

  // Shared logic between Present (...) and Present1 (...)
  int                     PresentBase (void);

  HRESULT                 RegisterDestructionCallback (void);
};

bool SK_DXGI_IsSwapChainReal (const DXGI_SWAP_CHAIN_DESC& desc) noexcept;
void ResetImGui_D3D12        (IDXGISwapChain *This);
void ResetImGui_D3D11        (IDXGISwapChain *This);

extern SetFullscreenState_pfn SetFullscreenState_Original;
extern ResizeTarget_pfn       ResizeTarget_Original;
extern ResizeBuffers_pfn      ResizeBuffers_Original;


#endif /* __SK__DXGI_SWAPCHAIN_H__ */