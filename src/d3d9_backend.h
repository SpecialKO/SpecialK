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

#ifndef __SK__D3D9_BACKEND_H__
#define __SK__D3D9_BACKEND_H__

#include <Windows.h>
#include <d3d9.h>

class SK_IDirect3D9 : public IDirect3D9
{
public:
  SK_IDirect3D9 (IDirect3D9* pImpl) : d3d9_ (pImpl) {
    //refs_ = 1;
  }

  virtual ~SK_IDirect3D9 (void) {
    //Assert (refs_ == 0)
    //refs_ = 0;
  }

  /*** IUnknown methods ***/
  virtual
  HRESULT
  WINAPI
    QueryInterface (REFIID riid, void** ppvObj) {
      return d3d9_->QueryInterface (riid, ppvObj);
    }

  virtual
  ULONG
  WINAPI
    AddRef (void) {
      //++refs_;
      return d3d9_->AddRef ();
    }

  virtual
  ULONG
  WINAPI
    Release (void) {
      //--refs_;
      return d3d9_->Release ();
    }


  /*** IDirect3D9 methods ***/
  virtual
  HRESULT
  WINAPI
    RegisterSoftwareDevice (void* pInitializeFunction) {
      return d3d9_->RegisterSoftwareDevice (pInitializeFunction);
    }

  virtual
  UINT
  WINAPI
    GetAdapterCount(void) {
      return d3d9_->GetAdapterCount ();
    }

  virtual
  HRESULT
  WINAPI
    GetAdapterIdentifier ( UINT                    Adapter,
                           DWORD                   Flags,
                           D3DADAPTER_IDENTIFIER9* pIdentifier ) {
      return d3d9_->GetAdapterIdentifier (Adapter, Flags, pIdentifier);
    }

  virtual
  UINT
  WINAPI
    GetAdapterModeCount (UINT Adapter, D3DFORMAT Format) {
      return d3d9_->GetAdapterModeCount (Adapter, Format);
    }

  virtual
  HRESULT
  WINAPI
    EnumAdapterModes ( UINT            Adapter,
                       D3DFORMAT       Format,
                       UINT            Mode,
                       D3DDISPLAYMODE* pMode) {
      return d3d9_->EnumAdapterModes (Adapter, Format, Mode, pMode);
    }

  virtual
  HRESULT
  WINAPI
    GetAdapterDisplayMode (UINT Adapter, D3DDISPLAYMODE* pMode) {
      return d3d9_->GetAdapterDisplayMode (Adapter, pMode);
    }

  virtual
  HRESULT
  WINAPI
    CheckDeviceType ( UINT       Adapter,
                      D3DDEVTYPE DevType,
                      D3DFORMAT  AdapterFormat,
                      D3DFORMAT  BackBufferFormat,
                      BOOL       bWindowed ) {
      return d3d9_->CheckDeviceType ( Adapter,
                                        DevType,
                                          AdapterFormat,
                                            BackBufferFormat,
                                              bWindowed );
    }

  virtual
  HRESULT
  WINAPI
    CheckDeviceFormat ( UINT            Adapter,
                        D3DDEVTYPE      DeviceType,
                        D3DFORMAT       AdapterFormat,
                        DWORD           Usage,
                        D3DRESOURCETYPE RType,
                        D3DFORMAT       CheckFormat) {
      return d3d9_->CheckDeviceFormat ( Adapter,
                                          DeviceType,
                                            AdapterFormat,
                                              Usage,
                                                RType,
                                                  CheckFormat );
    }

  virtual
  HRESULT
  WINAPI
    CheckDeviceMultiSampleType ( UINT                Adapter,
                                 D3DDEVTYPE          DeviceType,
                                 D3DFORMAT           SurfaceFormat,
                                 BOOL                Windowed,
                                 D3DMULTISAMPLE_TYPE MultiSampleType,
                                 DWORD*              pQualityLevels ) {
      return d3d9_->CheckDeviceMultiSampleType ( Adapter,
                                                   DeviceType,
                                                     SurfaceFormat,
                                                       Windowed,
                                                         MultiSampleType,
                                                           pQualityLevels );
    }

  virtual
  HRESULT
  WINAPI
    CheckDepthStencilMatch ( UINT       Adapter,
                             D3DDEVTYPE DeviceType,
                             D3DFORMAT  AdapterFormat,
                             D3DFORMAT  RenderTargetFormat,
                             D3DFORMAT  DepthStencilFormat ) {
      return d3d9_->CheckDepthStencilMatch ( Adapter,
                                               DeviceType,
                                                 AdapterFormat,
                                                   RenderTargetFormat,
                                                     DepthStencilFormat );
    }

  virtual
  HRESULT
  WINAPI
    CheckDeviceFormatConversion ( UINT       Adapter,
                                  D3DDEVTYPE DeviceType,
                                  D3DFORMAT  SourceFormat,
                                  D3DFORMAT  TargetFormat ) {
      return d3d9_->CheckDeviceFormatConversion ( Adapter,
                                                    DeviceType,
                                                      SourceFormat,
                                                        TargetFormat );
    }

  virtual
  HRESULT
  WINAPI
    GetDeviceCaps ( UINT       Adapter,
                    D3DDEVTYPE DeviceType,
                    D3DCAPS9*  pCaps ) {
      return d3d9_->GetDeviceCaps (Adapter, DeviceType, pCaps);
    }

  virtual
  HMONITOR
  WINAPI
    GetAdapterMonitor (UINT Adapter) {
      return d3d9_->GetAdapterMonitor (Adapter);
    }

  virtual
  HRESULT
  WINAPI
    CreateDevice ( UINT                   Adapter,
                   D3DDEVTYPE             DeviceType,
                   HWND                   hFocusWindow,
                   DWORD                  BehaviorFlags,
                   D3DPRESENT_PARAMETERS* pPresentationParameters,
                   IDirect3DDevice9**     ppReturnedDeviceInterface );

protected:
  //int         refs_;:
  IDirect3D9* d3d9_;
};

class SK_IDirect3D9Ex : public SK_IDirect3D9
{
public:
  SK_IDirect3D9Ex (IDirect3D9Ex* pImpl) : SK_IDirect3D9 (pImpl) {
  }

  virtual ~SK_IDirect3D9Ex (void) {
    //Assert (refs == 0)
  }

  virtual
  UINT
  WINAPI
    GetAdapterModeCountEx ( UINT                  Adapter,
                      CONST D3DDISPLAYMODEFILTER* pFilter ) {
      return ((IDirect3D9Ex *)d3d9_)->GetAdapterModeCountEx (Adapter, pFilter);
    }

  virtual
  HRESULT
  WINAPI
    EnumAdapterModesEx ( UINT                  Adapter,
                   CONST D3DDISPLAYMODEFILTER* pFilter,
                         UINT                  Mode,
                         D3DDISPLAYMODEEX*     pMode) {
      return ((IDirect3D9Ex *)d3d9_)->EnumAdapterModesEx (Adapter, pFilter, Mode, pMode);
    }

  virtual
  HRESULT
  WINAPI
    GetAdapterDisplayModeEx ( UINT                Adapter,
                              D3DDISPLAYMODEEX*   pMode,
                              D3DDISPLAYROTATION* pRotation) {
      return ((IDirect3D9Ex *)d3d9_)->GetAdapterDisplayModeEx (Adapter, pMode, pRotation);
    }

  virtual
  HRESULT
  WINAPI
    CreateDeviceEx ( UINT                   Adapter,
                     D3DDEVTYPE             DeviceType,
                     HWND                   hFocusWindow,
                     DWORD                  BehaviorFlags,
                     D3DPRESENT_PARAMETERS* pPresentationParameters,
                     D3DDISPLAYMODEEX*      pFullscreenDisplayMode,
                     IDirect3DDevice9Ex**   ppReturnedDeviceInterface) {
      return ((IDirect3D9Ex *)d3d9_)->CreateDeviceEx ( Adapter,
                                         DeviceType,
                                           hFocusWindow,
                                             BehaviorFlags,
                                               pPresentationParameters,
                                                 pFullscreenDisplayMode,
                                                   ppReturnedDeviceInterface );
    }

  virtual
  HRESULT
  WINAPI
    GetAdapterLUID (UINT Adapter, LUID* pLUID) {
      return ((IDirect3D9Ex *)d3d9_)->GetAdapterLUID (Adapter, pLUID);
    }
};

#include <string>

namespace SK
{
  namespace D3D9
  {

    bool Startup  (void);
    bool Shutdown (void);

    std::wstring WINAPI getPipelineStatsDesc (void);

    struct PipelineStatsD3D9 {
      struct StatQueryD3D9 {
        IDirect3DQuery9* object = nullptr;
        bool             active = false;
      } query;

      D3DDEVINFO_D3D9PIPELINETIMINGS 
                 last_results;
    } extern pipeline_stats_d3d9;
  }
}

#endif /* __SK__D3D9_BACKEND_H__ */