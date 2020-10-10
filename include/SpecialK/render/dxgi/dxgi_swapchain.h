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

extern volatile LONG SK_DXGI_LiveWrappedSwapChains;
extern volatile LONG SK_DXGI_LiveWrappedSwapChain1s;

interface ID3D11Device;
interface ID3D12Device;
#include <atlbase.h>

#ifndef __SK_SUBSYSTEM__
#define __SK_SUBSYSTEM__ L"IDXGI Wrap"
#endif

struct IWrapDXGISwapChain : IDXGISwapChain4
{
  IWrapDXGISwapChain ( ID3D11Device   *pDevice,
                       IDXGISwapChain *pSwapChain ) :
                                                      pReal (pSwapChain),
                                                      pDev  (pDevice),
                                                      ver_  (0)
  {
    if (! pSwapChain)
      return;

    AddRef ();

    InterlockedIncrement (&SK_DXGI_LiveWrappedSwapChains);

    IUnknown *pPromotion = nullptr;

    if (SUCCEEDED (pReal->QueryInterface (IID_IDXGISwapChain4, (void **)&pPromotion)))
    {
      ver_ = 4;
    }

    else if (SUCCEEDED (pReal->QueryInterface (IID_IDXGISwapChain3, (void **)&pPromotion)))
    {
      ver_ = 3;
    }

    else if (SUCCEEDED (pReal->QueryInterface (IID_IDXGISwapChain2, (void **)&pPromotion)))
    {
      ver_ = 2;
    }

    else if (SUCCEEDED (pReal->QueryInterface (IID_IDXGISwapChain1, (void **)&pPromotion)))
    {
      ver_ = 1;
    }

    if (ver_ != 0)
    {
      Release ();

      pReal = (IDXGISwapChain *)pPromotion;

      SK_LOG0 ( ( L"Promoted IDXGISwapChain to IDXGISwapChain%lu", ver_),
                  __SK_SUBSYSTEM__ );
    }
  }

  IWrapDXGISwapChain ( ID3D11Device    *pDevice,
                       IDXGISwapChain1 *pSwapChain ) :
                                                       pReal (pSwapChain),
                                                       pDev  (pDevice),
                                                       ver_  (1)
  {
    if (! pSwapChain)
      return;

    AddRef ();

    InterlockedIncrement (&SK_DXGI_LiveWrappedSwapChain1s);

    IUnknown *pPromotion = nullptr;

    if (SUCCEEDED (pReal->QueryInterface (IID_IDXGISwapChain4, (void **)&pPromotion)))
    {
      ver_ = 4;
    }

    else if (SUCCEEDED (pReal->QueryInterface (IID_IDXGISwapChain3, (void **)&pPromotion)))
    {
      ver_ = 3;
    }

    else if (SUCCEEDED (pReal->QueryInterface (IID_IDXGISwapChain2, (void **)&pPromotion)))
    {
      ver_ = 2;
    }

    if (ver_ != 1)
    {
      Release ();

      pReal = (IDXGISwapChain *)pPromotion;

      SK_LOG0 ( ( L"Promoted IDXGISwapChain1 to IDXGISwapChain%lu", ver_),
                  __SK_SUBSYSTEM__ );
    }
  }

  //IWrapDXGISwapChain ( ID3D12Device    *pDevice12,
  //                     IDXGISwapChain1 *pSwapChain ) :
  //  pReal  (pSwapChain),
  //  pDev12 ( pDevice12),
  //  ver_   (    0     )
  //{
  //  if (! pSwapChain)
  //    return;
  //
  //  InterlockedExchange (
  //    &refs_, pReal->AddRef  () - 1
  //  );        pReal->Release ();
  //  AddRef ();
  //
  //  InterlockedIncrement (&SK_DXGI_LiveWrappedSwapChain1s);
  //
  //  IUnknown *pPromotion = nullptr;
  //
  //  if (SUCCEEDED (pReal->QueryInterface (IID_IDXGISwapChain4, (void **)&pPromotion)))
  //  {
  //    ver_ = 4;
  //  }
  //
  //  else if (SUCCEEDED (pReal->QueryInterface (IID_IDXGISwapChain3, (void **)&pPromotion)))
  //  {
  //    ver_ = 3;
  //  }
  //
  //  else if (SUCCEEDED (pReal->QueryInterface (IID_IDXGISwapChain2, (void **)&pPromotion)))
  //  {
  //    ver_ = 2;
  //  }
  //
  //  else if (SUCCEEDED (pReal->QueryInterface (IID_IDXGISwapChain1, (void **)&pPromotion)))
  //  {
  //    ver_ = 1;
  //  }
  //
  //  if (ver_ != 0)
  //  {
  //    Release ();
  //
  //    pReal = (IDXGISwapChain1 *)pPromotion;
  //
  //    SK_LOG0 ( ( L"Promoted IDXGISwapChain1 to IDXGISwapChain%li", ver_),
  //                __SK_SUBSYSTEM__ );
  //  }
  //}



  virtual ~IWrapDXGISwapChain (void)
  {
  }


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

  volatile LONG   refs_ = 0;
  IDXGISwapChain *pReal;
  IUnknown       *pDev;
  unsigned int    ver_;
};


#endif /* __SK__DXGI_SWAPCHAIN_H__ */