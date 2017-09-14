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

#undef D3D_DISABLE_9EX

#include <Windows.h>
#include <d3d9.h>
#include <d3dx9shader.h>

#include <unordered_map>
#include <unordered_set>
#include <cstdint>

namespace SK   {
namespace D3D9 {
  struct KnownShaders
  {
    //typedef std::unordered_map <uint32_t, std::unordered_set <ID3D11ShaderResourceView *>> conditional_blacklist_t;
  
    template <typename _T>
    struct ShaderRegistry
    {
      void clear (void)
      {
        rev.clear   ();
        clear_state ();
  
        changes_last_frame = 0;
      }
  
      void clear_state (void)
      {
        current.crc32c = 0x0;
        current.ptr    = nullptr;
      }
  
      std::unordered_map <_T*, uint32_t>      rev;
      //std::unordered_map <uint32_t, SK_D3D11_ShaderDesc> descs;
  
      std::unordered_set <uint32_t>           wireframe;
      std::unordered_set <uint32_t>           blacklist;
  
      //conditional_blacklist_t                 blacklist_if_texture;
  
      struct
      {
        uint32_t                              crc32c = 0x00;
        _T*                                   ptr    = nullptr;
      } current;
  
      //d3d11_shader_tracking_s                 tracked;
  
      ULONG                                   changes_last_frame = 0;
    };
  
    ShaderRegistry <IDirect3DPixelShader9>    pixel;
    ShaderRegistry <IDirect3DVertexShader9>   vertex;
  } extern Shaders;

// Dumb macro in WinUser.h
#undef DrawState

  struct DrawState
  {
    // Most of these states are not tracked
    DWORD           srcblend        = 0;
    DWORD           dstblend        = 0;
    DWORD           srcalpha        = 0;     // Separate Alpha Blend Eq: Src
    DWORD           dstalpha        = 0;     // Separate Alpha Blend Eq: Dst
    bool            alpha_test      = false; // Test Alpha?
    DWORD           alpha_ref       = 0;     // Value to test.
    bool            zwrite          = false; // Depth Mask
  
    volatile ULONG  draws           = 0; // Number of draw calls
    volatile ULONG  frames          = 0;
  
    bool            cegui_active    = false;
    
    int             instances       = 0;
  
    uint32_t        current_tex  [256];
    float           viewport_off [4]  = { 0.0f }; // Most vertex shaders use this and we can
                                                  //   test the set values to determine if a draw
                                                  //     is HUD or world-space.
                                                  //     
    volatile ULONG  draw_count  = 0;
    volatile ULONG  next_draw   = 0;
    volatile ULONG  scene_count = 0;
  } extern draw_state;


  struct FrameState
  {
    void clear (void) { pixel_shaders.clear (); vertex_shaders.clear (); vertex_buffers.dynamic.clear (); vertex_buffers.immutable.clear (); }
  
    std::unordered_set <uint32_t>                 pixel_shaders;
    std::unordered_set <uint32_t>                 vertex_shaders;
  
    struct {
      std::unordered_set <IDirect3DVertexBuffer9 *> dynamic;
      std::unordered_set <IDirect3DVertexBuffer9 *> immutable;
    } vertex_buffers;
  } extern last_frame;
  
  struct KnownObjects
  {
    void clear (void) { static_vbs.clear (); dynamic_vbs.clear (); };
  
    std::unordered_set <IDirect3DVertexBuffer9 *> static_vbs;
    std::unordered_set <IDirect3DVertexBuffer9 *> dynamic_vbs;
  } extern known_objs;
  
  struct RenderTargetTracker
  {
    void clear (void) { pixel_shaders.clear (); vertex_shaders.clear (); active = false; }
  
    IDirect3DBaseTexture9*        tracking_tex  = nullptr;
  
    std::unordered_set <uint32_t> pixel_shaders;
    std::unordered_set <uint32_t> vertex_shaders;
  
    bool                          active        = false;
  } extern tracked_rt;
  
  struct ShaderTracker
  {
    void clear (void)
    {
      active    = false;
      InterlockedExchange (&num_draws, 0);
      used_textures.clear ();
  
      for (auto& current_texture : current_textures)
        current_texture = 0x00;
    }
  
    void use (IUnknown* pShader);
  
    uint32_t                      crc32c       =  0x00;
    bool                          cancel_draws = false;
    bool                          clamp_coords = false;
    bool                          active       = false;
    volatile ULONG                num_draws    =     0;
    std::unordered_set <IDirect3DBaseTexture9*> used_textures;
                        IDirect3DBaseTexture9*  current_textures [16];
  
    //std::vector <IDirect3DBaseTexture9 *> samplers;
  
    IUnknown*                     shader_obj  = nullptr;
    ID3DXConstantTable*           ctable      = nullptr;
  
    struct shader_constant_s
    {
      char                Name [128];
      D3DXREGISTER_SET    RegisterSet;
      UINT                RegisterIndex;
      UINT                RegisterCount;
      D3DXPARAMETER_CLASS Class;
      D3DXPARAMETER_TYPE  Type;
      UINT                Rows;
      UINT                Columns;
      UINT                Elements;
      std::vector <shader_constant_s>
                          struct_members;
      bool                Override;
      float               Data [4]; // TEMP HACK
    };
  
    std::vector <shader_constant_s> constants;
  };
  
  extern ShaderTracker tracked_vs;
  extern ShaderTracker tracked_ps;
  
  struct VertexBufferTracker
  {
    void clear (void);
    void use   (void);
  
    IDirect3DVertexBuffer9*       vertex_buffer = nullptr;
  
    std::unordered_set <
      IDirect3DVertexDeclaration9*
    >                             vertex_decls;
  
    //uint32_t                      crc32c       =  0x00;
    bool                          cancel_draws  = false;
    bool                          wireframe     = false;
    bool                          active        = false;
    volatile ULONG                num_draws     =     0;
    volatile ULONG                instanced     =     0;
    int                           instances     =     1;
  
    std::unordered_set <uint32_t> vertex_shaders;
    std::unordered_set <uint32_t> pixel_shaders;
    std::unordered_set <uint32_t> textures;
  
    std::unordered_set <IDirect3DVertexBuffer9 *>
                                  wireframes;
  } extern tracked_vb;
  
  struct ShaderDisassembly {
    std::string header;
    std::string code;
    std::string footer;
  };

  enum class ShaderClass {
    Unknown = 0x00,
    Pixel   = 0x01,
    Vertex  = 0x02
  };
}
}

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
    QueryInterface (REFIID riid, void** ppvObj) override {
      return d3d9_->QueryInterface (riid, ppvObj);
    }

  virtual
  ULONG
  WINAPI
    AddRef (void) override {
      //++refs_;
      return d3d9_->AddRef ();
    }

  virtual
  ULONG
  WINAPI
    Release (void) override {
      //--refs_;
      return d3d9_->Release ();
    }


  /*** IDirect3D9 methods ***/
  virtual
  HRESULT
  WINAPI
    RegisterSoftwareDevice (void* pInitializeFunction) override {
      return d3d9_->RegisterSoftwareDevice (pInitializeFunction);
    }

  virtual
  UINT
  WINAPI
    GetAdapterCount(void) override {
      return d3d9_->GetAdapterCount ();
    }

  virtual
  HRESULT
  WINAPI
    GetAdapterIdentifier ( UINT                    Adapter,
                           DWORD                   Flags,
                           D3DADAPTER_IDENTIFIER9* pIdentifier ) override {
      return d3d9_->GetAdapterIdentifier (Adapter, Flags, pIdentifier);
    }

  virtual
  UINT
  WINAPI
    GetAdapterModeCount (UINT Adapter, D3DFORMAT Format) override {
      return d3d9_->GetAdapterModeCount (Adapter, Format);
    }

  virtual
  HRESULT
  WINAPI
    EnumAdapterModes ( UINT            Adapter,
                       D3DFORMAT       Format,
                       UINT            Mode,
                       D3DDISPLAYMODE* pMode) override {
      return d3d9_->EnumAdapterModes (Adapter, Format, Mode, pMode);
    }

  virtual
  HRESULT
  WINAPI
    GetAdapterDisplayMode (UINT Adapter, D3DDISPLAYMODE* pMode) override {
      return d3d9_->GetAdapterDisplayMode (Adapter, pMode);
    }

  virtual
  HRESULT
  WINAPI
    CheckDeviceType ( UINT       Adapter,
                      D3DDEVTYPE DevType,
                      D3DFORMAT  AdapterFormat,
                      D3DFORMAT  BackBufferFormat,
                      BOOL       bWindowed ) override {
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
                        D3DFORMAT       CheckFormat) override {
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
                                 DWORD*              pQualityLevels ) override {
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
                             D3DFORMAT  DepthStencilFormat ) override {
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
                                  D3DFORMAT  TargetFormat ) override {
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
                    D3DCAPS9*  pCaps ) override {
      return d3d9_->GetDeviceCaps (Adapter, DeviceType, pCaps);
    }

  virtual
  HMONITOR
  WINAPI
    GetAdapterMonitor (UINT Adapter) override {
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
                   IDirect3DDevice9**     ppReturnedDeviceInterface ) override;

protected:
  //int         refs_;:
  IDirect3D9* d3d9_;
};

class SK_IDirect3D9Ex : public SK_IDirect3D9
{
public:
  SK_IDirect3D9Ex (IDirect3D9Ex* pImpl) : SK_IDirect3D9 (pImpl) {
  }

  virtual ~SK_IDirect3D9Ex (void) override {
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

DECLARE_INTERFACE_(ISKD3DDevice9, IDirect3DDevice9)
{
  /*** IUnknown methods ***/
  STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj) {
    return dev_->QueryInterface (riid, ppvObj);
  }
  STDMETHOD_(ULONG,AddRef)(THIS) {
    return dev_->AddRef ();
  }
  STDMETHOD_(ULONG,Release)(THIS) {
    return dev_->Release ();
  }

  /*** IDirect3DDevice9 methods ***/
  STDMETHOD(TestCooperativeLevel)(THIS) {
    return dev_->TestCooperativeLevel ();
  }
  STDMETHOD_(UINT, GetAvailableTextureMem)(THIS) {
    return dev_->GetAvailableTextureMem ();
  }
  STDMETHOD(EvictManagedResources)(THIS) {
    return dev_->EvictManagedResources ();
  }
  STDMETHOD(GetDirect3D)(THIS_ IDirect3D9** ppD3D9) {
    return dev_->GetDirect3D (ppD3D9);
  }
  STDMETHOD(GetDeviceCaps)(THIS_ D3DCAPS9* pCaps) {
    return dev_->GetDeviceCaps (pCaps);
  }
  STDMETHOD(GetDisplayMode)(THIS_ UINT iSwapChain,D3DDISPLAYMODE* pMode) {
    return dev_->GetDisplayMode (iSwapChain, pMode);
  }
  STDMETHOD(GetCreationParameters)(THIS_ D3DDEVICE_CREATION_PARAMETERS *pParameters) {
    return dev_->GetCreationParameters (pParameters);
  }
  STDMETHOD(SetCursorProperties)(THIS_ UINT XHotSpot,UINT YHotSpot,IDirect3DSurface9* pCursorBitmap) {
    return dev_->SetCursorProperties (XHotSpot, YHotSpot, pCursorBitmap);
  }
  STDMETHOD_(void, SetCursorPosition)(THIS_ int X,int Y,DWORD Flags) {
    return dev_->SetCursorPosition (X, Y, Flags);
  }
  STDMETHOD_(BOOL, ShowCursor)(THIS_ BOOL bShow) {
    return dev_->ShowCursor (bShow);
  }
  STDMETHOD(CreateAdditionalSwapChain)(THIS_ D3DPRESENT_PARAMETERS* pPresentationParameters,IDirect3DSwapChain9** pSwapChain) {
    return dev_->CreateAdditionalSwapChain (pPresentationParameters, pSwapChain);
  }
  STDMETHOD(GetSwapChain)(THIS_ UINT iSwapChain,IDirect3DSwapChain9** pSwapChain) {
    return dev_->GetSwapChain (iSwapChain, pSwapChain);
  }
  STDMETHOD_(UINT, GetNumberOfSwapChains)(THIS) {
    return dev_->GetNumberOfSwapChains ();
  }
  STDMETHOD(Reset)(THIS_ D3DPRESENT_PARAMETERS* pPresentationParameters) {
    return dev_->Reset (pPresentationParameters);
  }
  STDMETHOD(Present)(THIS_ CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion) {
    return dev_->Present (pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
  }
  STDMETHOD(GetBackBuffer)(THIS_ UINT iSwapChain,UINT iBackBuffer,D3DBACKBUFFER_TYPE Type,IDirect3DSurface9** ppBackBuffer) {
    return dev_->GetBackBuffer (iSwapChain, iBackBuffer, Type, ppBackBuffer);
  }
  STDMETHOD(GetRasterStatus)(THIS_ UINT iSwapChain,D3DRASTER_STATUS* pRasterStatus) {
    return dev_->GetRasterStatus (iSwapChain, pRasterStatus);
  }
  STDMETHOD(SetDialogBoxMode)(THIS_ BOOL bEnableDialogs) {
    return dev_->SetDialogBoxMode (bEnableDialogs);
  }
  STDMETHOD_(void, SetGammaRamp)(THIS_ UINT iSwapChain,DWORD Flags,CONST D3DGAMMARAMP* pRamp) {
    return dev_->SetGammaRamp (iSwapChain, Flags, pRamp);
  }
  STDMETHOD_(void, GetGammaRamp)(THIS_ UINT iSwapChain,D3DGAMMARAMP* pRamp) {
    return dev_->GetGammaRamp (iSwapChain, pRamp);
  }
  STDMETHOD(CreateTexture)(THIS_ UINT Width,UINT Height,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DTexture9** ppTexture,HANDLE* pSharedHandle) {
    return dev_->CreateTexture (Width, Height, Levels, Usage, Format, Pool, ppTexture, pSharedHandle);
  }
  STDMETHOD(CreateVolumeTexture)(THIS_ UINT Width,UINT Height,UINT Depth,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DVolumeTexture9** ppVolumeTexture,HANDLE* pSharedHandle) {
    return dev_->CreateVolumeTexture (Width, Height, Depth, Levels, Usage, Format, Pool, ppVolumeTexture, pSharedHandle);
  }
  STDMETHOD(CreateCubeTexture)(THIS_ UINT EdgeLength,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DCubeTexture9** ppCubeTexture,HANDLE* pSharedHandle) {
    return dev_->CreateCubeTexture (EdgeLength, Levels, Usage, Format, Pool, ppCubeTexture, pSharedHandle);
  }
  STDMETHOD(CreateVertexBuffer)(THIS_ UINT Length,DWORD Usage,DWORD FVF,D3DPOOL Pool,IDirect3DVertexBuffer9** ppVertexBuffer,HANDLE* pSharedHandle) {
    return dev_->CreateVertexBuffer (Length, Usage, FVF, Pool, ppVertexBuffer, pSharedHandle);
  }
  STDMETHOD(CreateIndexBuffer)(THIS_ UINT Length,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DIndexBuffer9** ppIndexBuffer,HANDLE* pSharedHandle) {
    return dev_->CreateIndexBuffer (Length, Usage, Format, Pool, ppIndexBuffer, pSharedHandle);
  }
  STDMETHOD(CreateRenderTarget)(THIS_ UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Lockable,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle) {
    return dev_->CreateRenderTarget (Width, Height, Format, MultiSample, MultisampleQuality, Lockable, ppSurface, pSharedHandle);
  }
  STDMETHOD(CreateDepthStencilSurface)(THIS_ UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Discard,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle) {
    return dev_->CreateDepthStencilSurface (Width, Height, Format, MultiSample, MultisampleQuality, Discard, ppSurface, pSharedHandle);
  }
  STDMETHOD(UpdateSurface)(THIS_ IDirect3DSurface9* pSourceSurface,CONST RECT* pSourceRect,IDirect3DSurface9* pDestinationSurface,CONST POINT* pDestPoint) {
    return dev_->UpdateSurface (pSourceSurface, pSourceRect, pDestinationSurface, pDestPoint);
  }
  STDMETHOD(UpdateTexture)(THIS_ IDirect3DBaseTexture9* pSourceTexture,IDirect3DBaseTexture9* pDestinationTexture) {
    return dev_->UpdateTexture (pSourceTexture, pDestinationTexture);
  }
  STDMETHOD(GetRenderTargetData)(THIS_ IDirect3DSurface9* pRenderTarget,IDirect3DSurface9* pDestSurface) {
    return dev_->GetRenderTargetData (pRenderTarget, pDestSurface);
  }
  STDMETHOD(GetFrontBufferData)(THIS_ UINT iSwapChain,IDirect3DSurface9* pDestSurface) {
    return dev_->GetFrontBufferData (iSwapChain, pDestSurface);
  }
  STDMETHOD(StretchRect)(THIS_ IDirect3DSurface9* pSourceSurface,CONST RECT* pSourceRect,IDirect3DSurface9* pDestSurface,CONST RECT* pDestRect,D3DTEXTUREFILTERTYPE Filter) {
    return dev_->StretchRect (pSourceSurface, pSourceRect, pDestSurface, pDestRect, Filter);
  }
  STDMETHOD(ColorFill)(THIS_ IDirect3DSurface9* pSurface,CONST RECT* pRect,D3DCOLOR color) {
    return dev_->ColorFill (pSurface, pRect, color);
  }
  STDMETHOD(CreateOffscreenPlainSurface)(THIS_ UINT Width,UINT Height,D3DFORMAT Format,D3DPOOL Pool,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle) {
    return dev_->CreateOffscreenPlainSurface (Width, Height, Format, Pool, ppSurface, pSharedHandle);
  }
  STDMETHOD(SetRenderTarget)(THIS_ DWORD RenderTargetIndex,IDirect3DSurface9* pRenderTarget) {
    return dev_->SetRenderTarget (RenderTargetIndex, pRenderTarget);
  }
  STDMETHOD(GetRenderTarget)(THIS_ DWORD RenderTargetIndex,IDirect3DSurface9** ppRenderTarget) {
    return dev_->GetRenderTarget (RenderTargetIndex, ppRenderTarget);
  }
  STDMETHOD(SetDepthStencilSurface)(THIS_ IDirect3DSurface9* pNewZStencil) {
    return dev_->SetDepthStencilSurface (pNewZStencil);
  }
  STDMETHOD(GetDepthStencilSurface)(THIS_ IDirect3DSurface9** ppZStencilSurface) {
    return dev_->GetDepthStencilSurface (ppZStencilSurface);
  }
  STDMETHOD(BeginScene)(THIS) {
    return dev_->BeginScene ();
  }
  STDMETHOD(EndScene)(THIS) {
    return dev_->EndScene ();
  }
  STDMETHOD(Clear)(THIS_ DWORD Count,CONST D3DRECT* pRects,DWORD Flags,D3DCOLOR Color,float Z,DWORD Stencil) {
    return dev_->Clear (Count, pRects, Flags, Color, Z, Stencil);
  }
  STDMETHOD(SetTransform)(THIS_ D3DTRANSFORMSTATETYPE State,CONST D3DMATRIX* pMatrix) {
    return dev_->SetTransform (State, pMatrix);
  }
  STDMETHOD(GetTransform)(THIS_ D3DTRANSFORMSTATETYPE State,D3DMATRIX* pMatrix) {
    return dev_->GetTransform (State, pMatrix);
  }
  STDMETHOD(MultiplyTransform)(THIS_ D3DTRANSFORMSTATETYPE type, CONST D3DMATRIX* mat) {
    return dev_->MultiplyTransform (type, mat);
  }
  STDMETHOD(SetViewport)(THIS_ CONST D3DVIEWPORT9* pViewport) {
    return dev_->SetViewport (pViewport);
  }
  STDMETHOD(GetViewport)(THIS_ D3DVIEWPORT9* pViewport) {
    return dev_->GetViewport (pViewport);
  }
  STDMETHOD(SetMaterial)(THIS_ CONST D3DMATERIAL9* pMaterial) {
    return dev_->SetMaterial (pMaterial);
  }
  STDMETHOD(GetMaterial)(THIS_ D3DMATERIAL9* pMaterial) {
    return dev_->GetMaterial (pMaterial);
  }
  STDMETHOD(SetLight)(THIS_ DWORD Index,CONST D3DLIGHT9* light) {
    return dev_->SetLight (Index, light);
  }
  STDMETHOD(GetLight)(THIS_ DWORD Index,D3DLIGHT9* light) {
    return dev_->GetLight (Index, light);
  }
  STDMETHOD(LightEnable)(THIS_ DWORD Index,BOOL Enable) {
    return dev_->LightEnable (Index, Enable);
  }
  STDMETHOD(GetLightEnable)(THIS_ DWORD Index,BOOL* pEnable) {
    return dev_->GetLightEnable (Index, pEnable);
  }
  STDMETHOD(SetClipPlane)(THIS_ DWORD Index,CONST float* pPlane) {
    return dev_->SetClipPlane (Index, pPlane);
  }
  STDMETHOD(GetClipPlane)(THIS_ DWORD Index,float* pPlane) {
    return dev_->GetClipPlane (Index, pPlane);
  }
  STDMETHOD(SetRenderState)(THIS_ D3DRENDERSTATETYPE State,DWORD Value) {
    return dev_->SetRenderState (State, Value);
  }
  STDMETHOD(GetRenderState)(THIS_ D3DRENDERSTATETYPE State,DWORD* pValue) {
    return dev_->GetRenderState (State, pValue);
  }
  STDMETHOD(CreateStateBlock)(THIS_ D3DSTATEBLOCKTYPE Type,IDirect3DStateBlock9** ppSB) {
    return dev_->CreateStateBlock (Type, ppSB);
  }
  STDMETHOD(BeginStateBlock)(THIS) {
    return dev_->BeginStateBlock ();
  }
  STDMETHOD(EndStateBlock)(THIS_ IDirect3DStateBlock9** ppSB) {
    return dev_->EndStateBlock (ppSB);
  }
  STDMETHOD(SetClipStatus)(THIS_ CONST D3DCLIPSTATUS9* pClipStatus) {
    return dev_->SetClipStatus (pClipStatus);
  }
  STDMETHOD(GetClipStatus)(THIS_ D3DCLIPSTATUS9* pClipStatus) {
    return dev_->GetClipStatus (pClipStatus);
  }
  STDMETHOD(GetTexture)(THIS_ DWORD Stage,IDirect3DBaseTexture9** ppTexture) {
    return dev_->GetTexture (Stage, ppTexture);
  }
  STDMETHOD(SetTexture)(THIS_ DWORD Stage,IDirect3DBaseTexture9* pTexture) {
    return dev_->SetTexture (Stage, pTexture);
  }
  STDMETHOD(GetTextureStageState)(THIS_ DWORD Stage,D3DTEXTURESTAGESTATETYPE Type,DWORD* pValue) {
    return dev_->GetTextureStageState (Stage, Type, pValue);
  }
  STDMETHOD(SetTextureStageState)(THIS_ DWORD Stage,D3DTEXTURESTAGESTATETYPE Type,DWORD Value) {
    return dev_->SetTextureStageState (Stage, Type, Value);
  }
  STDMETHOD(GetSamplerState)(THIS_ DWORD Sampler,D3DSAMPLERSTATETYPE Type,DWORD* pValue) {
    return dev_->GetSamplerState (Sampler, Type, pValue);
  }
  STDMETHOD(SetSamplerState)(THIS_ DWORD Sampler,D3DSAMPLERSTATETYPE Type,DWORD Value) {
    return dev_->SetSamplerState (Sampler, Type, Value);
  }
  STDMETHOD(ValidateDevice)(THIS_ DWORD* pNumPasses) {
    return dev_->ValidateDevice (pNumPasses);
  }
  STDMETHOD(SetPaletteEntries)(THIS_ UINT PaletteNumber,CONST PALETTEENTRY* pEntries) {
    return dev_->SetPaletteEntries (PaletteNumber, pEntries);
  }
  STDMETHOD(GetPaletteEntries)(THIS_ UINT PaletteNumber,PALETTEENTRY* pEntries) {
    return dev_->GetPaletteEntries (PaletteNumber, pEntries);
  }
  STDMETHOD(SetCurrentTexturePalette)(THIS_ UINT PaletteNumber) {
    return dev_->SetCurrentTexturePalette (PaletteNumber);
  }
  STDMETHOD(GetCurrentTexturePalette)(THIS_ UINT *PaletteNumber) {
    return dev_->GetCurrentTexturePalette (PaletteNumber);
  }
  STDMETHOD(SetScissorRect)(THIS_ CONST RECT* pRect) {
    return dev_->SetScissorRect (pRect);
  }
  STDMETHOD(GetScissorRect)(THIS_ RECT* pRect) {
    return dev_->GetScissorRect (pRect);
  }
  STDMETHOD(SetSoftwareVertexProcessing)(THIS_ BOOL bSoftware) {
    return dev_->SetSoftwareVertexProcessing (bSoftware);
  }
  STDMETHOD_(BOOL, GetSoftwareVertexProcessing)(THIS) {
    return dev_->GetSoftwareVertexProcessing ();
  }
  STDMETHOD(SetNPatchMode)(THIS_ float nSegments) {
    return dev_->SetNPatchMode (nSegments);
  }
  STDMETHOD_(float, GetNPatchMode)(THIS) {
    return dev_->GetNPatchMode ();
  }
  STDMETHOD(DrawPrimitive)(THIS_ D3DPRIMITIVETYPE PrimitiveType,UINT StartVertex,UINT PrimitiveCount) {
    return dev_->DrawPrimitive (PrimitiveType, StartVertex, PrimitiveCount);
  }
  STDMETHOD(DrawIndexedPrimitive)(THIS_ D3DPRIMITIVETYPE type,INT BaseVertexIndex,UINT MinVertexIndex,UINT NumVertices,UINT startIndex,UINT primCount) {
    return dev_->DrawIndexedPrimitive (type, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
  }
  STDMETHOD(DrawPrimitiveUP)(THIS_ D3DPRIMITIVETYPE PrimitiveType,UINT PrimitiveCount,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride) {
    return dev_->DrawPrimitiveUP (PrimitiveType, PrimitiveCount, pVertexStreamZeroData, VertexStreamZeroStride);
  }
  STDMETHOD(DrawIndexedPrimitiveUP)(THIS_ D3DPRIMITIVETYPE PrimitiveType,UINT MinVertexIndex,UINT NumVertices,UINT PrimitiveCount,CONST void* pIndexData,D3DFORMAT IndexDataFormat,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride) {
    return dev_->DrawIndexedPrimitiveUP (PrimitiveType, MinVertexIndex, NumVertices, PrimitiveCount, pIndexData, IndexDataFormat, pVertexStreamZeroData, VertexStreamZeroStride);
  }
  STDMETHOD(ProcessVertices)(THIS_ UINT SrcStartIndex,UINT DestIndex,UINT VertexCount,IDirect3DVertexBuffer9* pDestBuffer,IDirect3DVertexDeclaration9* pVertexDecl,DWORD Flags) {
    return dev_->ProcessVertices (SrcStartIndex, DestIndex, VertexCount, pDestBuffer, pVertexDecl, Flags);
  }
  STDMETHOD(CreateVertexDeclaration)(THIS_ CONST D3DVERTEXELEMENT9* pVertexElements,IDirect3DVertexDeclaration9** ppDecl) {
    return dev_->CreateVertexDeclaration (pVertexElements, ppDecl);
  }
  STDMETHOD(SetVertexDeclaration)(THIS_ IDirect3DVertexDeclaration9* pDecl) {
    return dev_->SetVertexDeclaration (pDecl);
  }
  STDMETHOD(GetVertexDeclaration)(THIS_ IDirect3DVertexDeclaration9** ppDecl) {
    return dev_->GetVertexDeclaration (ppDecl);
  }
  STDMETHOD(SetFVF)(THIS_ DWORD FVF) {
    return dev_->SetFVF (FVF);
  }
  STDMETHOD(GetFVF)(THIS_ DWORD* pFVF) {
    return dev_->GetFVF (pFVF);
  }
  STDMETHOD(CreateVertexShader)(THIS_ CONST DWORD* pFunction,IDirect3DVertexShader9** ppShader) {
    return dev_->CreateVertexShader (pFunction, ppShader);
  }
  STDMETHOD(SetVertexShader)(THIS_ IDirect3DVertexShader9* pShader) {
    return dev_->SetVertexShader (pShader);
  }
  STDMETHOD(GetVertexShader)(THIS_ IDirect3DVertexShader9** ppShader) {
    return dev_->GetVertexShader (ppShader);
  }
  STDMETHOD(SetVertexShaderConstantF)(THIS_ UINT StartRegister,CONST float* pConstantData,UINT Vector4fCount) {
    return dev_->SetVertexShaderConstantF (StartRegister, pConstantData, Vector4fCount);
  }
  STDMETHOD(GetVertexShaderConstantF)(THIS_ UINT StartRegister,float* pConstantData,UINT Vector4fCount) {
    return dev_->GetVertexShaderConstantF (StartRegister, pConstantData, Vector4fCount);
  }
  STDMETHOD(SetVertexShaderConstantI)(THIS_ UINT StartRegister,CONST int* pConstantData,UINT Vector4iCount) {
    return dev_->SetVertexShaderConstantI (StartRegister, pConstantData, Vector4iCount);
  }
  STDMETHOD(GetVertexShaderConstantI)(THIS_ UINT StartRegister,int* pConstantData,UINT Vector4iCount) {
    return dev_->GetVertexShaderConstantI (StartRegister, pConstantData, Vector4iCount);
  }
  STDMETHOD(SetVertexShaderConstantB)(THIS_ UINT StartRegister,CONST BOOL* pConstantData,UINT  BoolCount) {
    return dev_->SetVertexShaderConstantB (StartRegister, pConstantData, BoolCount);
  }
  STDMETHOD(GetVertexShaderConstantB)(THIS_ UINT StartRegister,BOOL* pConstantData,UINT BoolCount) {
    return dev_->GetVertexShaderConstantB (StartRegister, pConstantData, BoolCount);
  }
  STDMETHOD(SetStreamSource)(THIS_ UINT StreamNumber,IDirect3DVertexBuffer9* pStreamData,UINT OffsetInBytes,UINT Stride) {
    return dev_->SetStreamSource (StreamNumber, pStreamData, OffsetInBytes, Stride);
  }
  STDMETHOD(GetStreamSource)(THIS_ UINT StreamNumber,IDirect3DVertexBuffer9** ppStreamData,UINT* pOffsetInBytes,UINT* pStride) {
    return dev_->GetStreamSource (StreamNumber, ppStreamData, pOffsetInBytes, pStride);
  }
  STDMETHOD(SetStreamSourceFreq)(THIS_ UINT StreamNumber,UINT Setting) {
    return dev_->SetStreamSourceFreq (StreamNumber, Setting);
  }
  STDMETHOD(GetStreamSourceFreq)(THIS_ UINT StreamNumber,UINT* pSetting) {
    return dev_->GetStreamSourceFreq (StreamNumber, pSetting);
  }
  STDMETHOD(SetIndices)(THIS_ IDirect3DIndexBuffer9* pIndexData) {
    return dev_->SetIndices (pIndexData);
  }
  STDMETHOD(GetIndices)(THIS_ IDirect3DIndexBuffer9** ppIndexData) {
    return dev_->GetIndices (ppIndexData);
  }
  STDMETHOD(CreatePixelShader)(THIS_ CONST DWORD* pFunction,IDirect3DPixelShader9** ppShader) {
    return dev_->CreatePixelShader (pFunction, ppShader);
  }
  STDMETHOD(SetPixelShader)(THIS_ IDirect3DPixelShader9* pShader) {
    return dev_->SetPixelShader (pShader);
  }
  STDMETHOD(GetPixelShader)(THIS_ IDirect3DPixelShader9** ppShader) {
    return dev_->GetPixelShader (ppShader);
  }
  STDMETHOD(SetPixelShaderConstantF)(THIS_ UINT StartRegister,CONST float* pConstantData,UINT Vector4fCount) {
    return dev_->SetPixelShaderConstantF (StartRegister, pConstantData, Vector4fCount);
  }
  STDMETHOD(GetPixelShaderConstantF)(THIS_ UINT StartRegister,float* pConstantData,UINT Vector4fCount) {
    return dev_->GetPixelShaderConstantF (StartRegister, pConstantData, Vector4fCount);
  }
  STDMETHOD(SetPixelShaderConstantI)(THIS_ UINT StartRegister,CONST int* pConstantData,UINT Vector4iCount) {
    return dev_->SetPixelShaderConstantI (StartRegister, pConstantData, Vector4iCount);
  }
  STDMETHOD(GetPixelShaderConstantI)(THIS_ UINT StartRegister,int* pConstantData,UINT Vector4iCount) {
    return dev_->GetPixelShaderConstantI (StartRegister, pConstantData, Vector4iCount);
  }
  STDMETHOD(SetPixelShaderConstantB)(THIS_ UINT StartRegister,CONST BOOL* pConstantData,UINT  BoolCount) {
    return dev_->SetPixelShaderConstantB (StartRegister, pConstantData, BoolCount);
  }
  STDMETHOD(GetPixelShaderConstantB)(THIS_ UINT StartRegister,BOOL* pConstantData,UINT BoolCount) {
    return dev_->GetPixelShaderConstantB (StartRegister, pConstantData, BoolCount);
  }
  STDMETHOD(DrawRectPatch)(THIS_ UINT Handle,CONST float* pNumSegs,CONST D3DRECTPATCH_INFO* pRectPatchInfo) {
    return dev_->DrawRectPatch (Handle, pNumSegs, pRectPatchInfo);
  }
  STDMETHOD(DrawTriPatch)(THIS_ UINT Handle,CONST float* pNumSegs,CONST D3DTRIPATCH_INFO* pTriPatchInfo) {
    return dev_->DrawTriPatch (Handle, pNumSegs, pTriPatchInfo);
  }
  STDMETHOD(DeletePatch)(THIS_ UINT Handle) {
    return dev_->DeletePatch (Handle);
  }
  STDMETHOD(CreateQuery)(THIS_ D3DQUERYTYPE Type,IDirect3DQuery9** ppQuery) {
    return dev_->CreateQuery (Type, ppQuery);
  }

#if 0
    STDMETHOD(SetConvolutionMonoKernel)(THIS_ UINT width,UINT height,float* rows,float* columns) PURE;
    STDMETHOD(ComposeRects)(THIS_ IDirect3DSurface9* pSrc,IDirect3DSurface9* pDst,IDirect3DVertexBuffer9* pSrcRectDescs,UINT NumRects,IDirect3DVertexBuffer9* pDstRectDescs,D3DCOMPOSERECTSOP Operation,int Xoffset,int Yoffset) PURE;
    STDMETHOD(PresentEx)(THIS_ CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion,DWORD dwFlags) PURE;
    STDMETHOD(GetGPUThreadPriority)(THIS_ INT* pPriority) PURE;
    STDMETHOD(SetGPUThreadPriority)(THIS_ INT Priority) PURE;
    STDMETHOD(WaitForVBlank)(THIS_ UINT iSwapChain) PURE;
    STDMETHOD(CheckResourceResidency)(THIS_ IDirect3DResource9** pResourceArray,UINT32 NumResources) PURE;
    STDMETHOD(SetMaximumFrameLatency)(THIS_ UINT MaxLatency) PURE;
    STDMETHOD(GetMaximumFrameLatency)(THIS_ UINT* pMaxLatency) PURE;
    STDMETHOD(CheckDeviceState)(THIS_ HWND hDestinationWindow) PURE;
    STDMETHOD(CreateRenderTargetEx)(THIS_ UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Lockable,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle,DWORD Usage) PURE;
    STDMETHOD(CreateOffscreenPlainSurfaceEx)(THIS_ UINT Width,UINT Height,D3DFORMAT Format,D3DPOOL Pool,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle,DWORD Usage) PURE;
    STDMETHOD(CreateDepthStencilSurfaceEx)(THIS_ UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Discard,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle,DWORD Usage) PURE;
    STDMETHOD(ResetEx)(THIS_ D3DPRESENT_PARAMETERS* pPresentationParameters,D3DDISPLAYMODEEX *pFullscreenDisplayMode) PURE;
    STDMETHOD(GetDisplayModeEx)(THIS_ UINT iSwapChain,D3DDISPLAYMODEEX* pMode,D3DDISPLAYROTATION* pRotation) PURE;
#endif

  IDirect3DDevice9* dev_;
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


using D3D9PresentDevice_pfn = HRESULT (STDMETHODCALLTYPE *)(
           IDirect3DDevice9    *This,
_In_ const RECT                *pSourceRect,
_In_ const RECT                *pDestRect,
_In_       HWND                 hDestWindowOverride,
_In_ const RGNDATA             *pDirtyRegion);

using D3D9PresentDeviceEx_pfn = HRESULT (STDMETHODCALLTYPE *)(
           IDirect3DDevice9Ex  *This,
_In_ const RECT                *pSourceRect,
_In_ const RECT                *pDestRect,
_In_       HWND                 hDestWindowOverride,
_In_ const RGNDATA             *pDirtyRegion,
_In_       DWORD                dwFlags);

using D3D9PresentSwapChain_pfn = HRESULT (STDMETHODCALLTYPE *)(
             IDirect3DSwapChain9 *This,
  _In_ const RECT                *pSourceRect,
  _In_ const RECT                *pDestRect,
  _In_       HWND                 hDestWindowOverride,
  _In_ const RGNDATA             *pDirtyRegion,
  _In_       DWORD                dwFlags);

using D3D9PresentSwapChainEx_pfn = HRESULT (STDMETHODCALLTYPE *)(
  IDirect3DDevice9Ex  *This,
  _In_ const RECT                *pSourceRect,
  _In_ const RECT                *pDestRect,
  _In_       HWND                 hDestWindowOverride,
  _In_ const RGNDATA             *pDirtyRegion,
  _In_       DWORD                dwFlags);

using D3D9CreateDevice_pfn = HRESULT (STDMETHODCALLTYPE *)(
           IDirect3D9             *This,
           UINT                    Adapter,
           D3DDEVTYPE              DeviceType,
           HWND                    hFocusWindow,
           DWORD                   BehaviorFlags,
           D3DPRESENT_PARAMETERS  *pPresentationParameters,
           IDirect3DDevice9      **ppReturnedDeviceInterface);

using D3D9CreateDeviceEx_pfn = HRESULT (STDMETHODCALLTYPE *)(
           IDirect3D9Ex           *This,
           UINT                    Adapter,
           D3DDEVTYPE              DeviceType,
           HWND                    hFocusWindow,
           DWORD                   BehaviorFlags,
           D3DPRESENT_PARAMETERS  *pPresentationParameters,
           D3DDISPLAYMODEEX       *pFullscreenDisplayMode,
           IDirect3DDevice9Ex    **ppReturnedDeviceInterface);

using D3D9Reset_pfn = HRESULT (STDMETHODCALLTYPE *)(
           IDirect3DDevice9      *This,
           D3DPRESENT_PARAMETERS *pPresentationParameters);

using D3D9ResetEx_pfn = HRESULT (STDMETHODCALLTYPE *)(
           IDirect3DDevice9Ex    *This,
           D3DPRESENT_PARAMETERS *pPresentationParameters,
           D3DDISPLAYMODEEX      *pFullscreenDisplayMode );

using SetGammaRamp_pfn = void (STDMETHODCALLTYPE *)(
           IDirect3DDevice9      *This,
_In_       UINT                   iSwapChain,
_In_       DWORD                  Flags,
_In_ const D3DGAMMARAMP          *pRamp );

using DrawPrimitive_pfn = HRESULT (STDMETHODCALLTYPE *)
  ( IDirect3DDevice9* This,
    D3DPRIMITIVETYPE  PrimitiveType,
    UINT              StartVertex,
    UINT              PrimitiveCount );

using DrawIndexedPrimitive_pfn = HRESULT (STDMETHODCALLTYPE *)
  ( IDirect3DDevice9* This,
    D3DPRIMITIVETYPE  Type,
    INT               BaseVertexIndex,
    UINT              MinVertexIndex,
    UINT              NumVertices,
    UINT              startIndex,
    UINT              primCount );

using DrawPrimitiveUP_pfn = HRESULT (STDMETHODCALLTYPE *)
  ( IDirect3DDevice9* This,
    D3DPRIMITIVETYPE  PrimitiveType,
    UINT              PrimitiveCount,
    const void       *pVertexStreamZeroData,
    UINT              VertexStreamZeroStride );

using DrawIndexedPrimitiveUP_pfn = HRESULT (STDMETHODCALLTYPE *)
  ( IDirect3DDevice9* This,
    D3DPRIMITIVETYPE  PrimitiveType,
    UINT              MinVertexIndex,
    UINT              NumVertices,
    UINT              PrimitiveCount,
    const void       *pIndexData,
    D3DFORMAT         IndexDataFormat,
    const void       *pVertexStreamZeroData,
    UINT              VertexStreamZeroStride );

using GetTexture_pfn = HRESULT (STDMETHODCALLTYPE *)
  (     IDirect3DDevice9      *This,
   _In_ DWORD                  Sampler,
  _Out_ IDirect3DBaseTexture9 *pTexture);

using SetTexture_pfn = HRESULT (STDMETHODCALLTYPE *)
  (     IDirect3DDevice9      *This,
   _In_ DWORD                  Sampler,
   _In_ IDirect3DBaseTexture9 *pTexture);

using SetSamplerState_pfn = HRESULT (STDMETHODCALLTYPE *)
  (IDirect3DDevice9*   This,
   DWORD               Sampler,
   D3DSAMPLERSTATETYPE Type,
   DWORD               Value);

using SetViewport_pfn = HRESULT (STDMETHODCALLTYPE *)
  (      IDirect3DDevice9* This,
   CONST D3DVIEWPORT9*     pViewport);

using SetRenderState_pfn = HRESULT (STDMETHODCALLTYPE *)
  (IDirect3DDevice9*  This,
   D3DRENDERSTATETYPE State,
   DWORD              Value);

using SetVertexShaderConstantF_pfn = HRESULT (STDMETHODCALLTYPE *)
  (IDirect3DDevice9* This,
    UINT             StartRegister,
    CONST float*     pConstantData,
    UINT             Vector4fCount);

using SetPixelShaderConstantF_pfn = HRESULT (STDMETHODCALLTYPE *)
  (IDirect3DDevice9* This,
    UINT             StartRegister,
    CONST float*     pConstantData,
    UINT             Vector4fCount);

struct IDirect3DPixelShader9;

using SetPixelShader_pfn = HRESULT (STDMETHODCALLTYPE *)
  (IDirect3DDevice9*      This,
   IDirect3DPixelShader9* pShader);

struct IDirect3DVertexShader9;

using SetVertexShader_pfn = HRESULT (STDMETHODCALLTYPE *)
  (IDirect3DDevice9*       This,
   IDirect3DVertexShader9* pShader);

using SetScissorRect_pfn = HRESULT (STDMETHODCALLTYPE *)
  (IDirect3DDevice9* This,
   CONST RECT*       pRect);

using CreateTexture_pfn = HRESULT (STDMETHODCALLTYPE *)
  (IDirect3DDevice9   *This,
   UINT                Width,
   UINT                Height,
   UINT                Levels,
   DWORD               Usage,
   D3DFORMAT           Format,
   D3DPOOL             Pool,
   IDirect3DTexture9 **ppTexture,
   HANDLE             *pSharedHandle);

struct IDirect3DSurface9;

using CreateVertexBuffer_pfn = HRESULT (STDMETHODCALLTYPE *)
(
  _In_  IDirect3DDevice9        *This,
  _In_  UINT                     Length,
  _In_  DWORD                    Usage,
  _In_  DWORD                    FVF,
  _In_  D3DPOOL                  Pool,
  _Out_ IDirect3DVertexBuffer9 **ppVertexBuffer,
  _In_  HANDLE                  *pSharedHandle
);

using SetStreamSource_pfn = HRESULT (STDMETHODCALLTYPE *)
(
  IDirect3DDevice9       *This,
  UINT                    StreamNumber,
  IDirect3DVertexBuffer9 *pStreamData,
  UINT                    OffsetInBytes,
  UINT                    Stride
);

using SetStreamSourceFreq_pfn = HRESULT (STDMETHODCALLTYPE *)
( _In_ IDirect3DDevice9 *This,
  _In_ UINT              StreamNumber,
  _In_ UINT              FrequencyParameter
);

using SetFVF_pfn = HRESULT (STDMETHODCALLTYPE *)
(
  IDirect3DDevice9 *This,
  DWORD             FVF
);

using SetVertexDeclaration_pfn = HRESULT (STDMETHODCALLTYPE *)
(
  IDirect3DDevice9            *This,
  IDirect3DVertexDeclaration9 *pDecl
);

using CreateRenderTarget_pfn = HRESULT (STDMETHODCALLTYPE *)
  (IDirect3DDevice9     *This,
   UINT                  Width,
   UINT                  Height,
   D3DFORMAT             Format,
   D3DMULTISAMPLE_TYPE   MultiSample,
   DWORD                 MultisampleQuality,
   BOOL                  Lockable,
   IDirect3DSurface9   **ppSurface,
   HANDLE               *pSharedHandle);

using CreateDepthStencilSurface_pfn = HRESULT (STDMETHODCALLTYPE *)
  (IDirect3DDevice9     *This,
   UINT                  Width,
   UINT                  Height,
   D3DFORMAT             Format,
   D3DMULTISAMPLE_TYPE   MultiSample,
   DWORD                 MultisampleQuality,
   BOOL                  Discard,
   IDirect3DSurface9   **ppSurface,
   HANDLE               *pSharedHandle);

using SetRenderTarget_pfn           = HRESULT (STDMETHODCALLTYPE *)
  (IDirect3DDevice9  *This,
   DWORD              RenderTargetIndex,
   IDirect3DSurface9 *pRenderTarget);

using SetDepthStencilSurface_pfn    = HRESULT (STDMETHODCALLTYPE *)
  (IDirect3DDevice9  *This,
   IDirect3DSurface9 *pNewZStencil);

struct IDirect3DBaseTexture9;

using UpdateTexture_pfn             = HRESULT (STDMETHODCALLTYPE *)
  (IDirect3DDevice9      *This,
   IDirect3DBaseTexture9 *pSourceTexture,
   IDirect3DBaseTexture9 *pDestinationTexture);

using StretchRect_pfn               = HRESULT (STDMETHODCALLTYPE *)
  (      IDirect3DDevice9    *This,
         IDirect3DSurface9   *pSourceSurface,
   const RECT                *pSourceRect,
         IDirect3DSurface9   *pDestSurface,
   const RECT                *pDestRect,
         D3DTEXTUREFILTERTYPE Filter);

using SetCursorPosition_pfn         = HRESULT (STDMETHODCALLTYPE *)
(
       IDirect3DDevice9 *This,
  _In_ INT               X,
  _In_ INT               Y,
  _In_ DWORD             Flags
);

using TestCooperativeLevel_pfn      = HRESULT (STDMETHODCALLTYPE *)(IDirect3DDevice9* This);

using BeginScene_pfn                = HRESULT (STDMETHODCALLTYPE *)
  (IDirect3DDevice9* This);

using EndScene_pfn                  = HRESULT (STDMETHODCALLTYPE *)
  (IDirect3DDevice9* This);

using CreateVertexDeclaration_pfn   = HRESULT (STDMETHODCALLTYPE *)
(
        IDirect3DDevice9             *This,
  CONST D3DVERTEXELEMENT9            *pVertexElements,
        IDirect3DVertexDeclaration9 **ppDecl
);


using GetSwapChain_pfn              = HRESULT (STDMETHODCALLTYPE *)
  (IDirect3DDevice9* This, UINT iSwapChain, IDirect3DSwapChain9** pSwapChain);

using CreateAdditionalSwapChain_pfn = HRESULT (STDMETHODCALLTYPE *)
  (IDirect3DDevice9* This, D3DPRESENT_PARAMETERS* pPresentationParameters,
   IDirect3DSwapChain9** pSwapChain);




std::wstring
SK_D3D9_UsageToStr (DWORD dwUsage);

std::wstring
SK_D3D9_FormatToStr (D3DFORMAT Format, bool include_ordinal = true);

const wchar_t*
SK_D3D9_PoolToStr (D3DPOOL pool);

std::wstring
SK_D3D9_SwapEffectToStr (D3DSWAPEFFECT Effect);

std::wstring
SK_D3D9_PresentParameterFlagsToStr (DWORD dwFlags);


#endif /* __SK__D3D9_BACKEND_H__ */