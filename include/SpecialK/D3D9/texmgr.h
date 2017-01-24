#pragma once

#include <Windows.h>

#include "../log.h"
extern iSK_Logger tex_log;

//#include "render.h"
#include <d3d9.h>

#include <set>

#include <map>
#include <unordered_map>

interface ISKTextureD3D9;

namespace sk {
namespace d3d9 {

extern HMODULE d3dx9_dll;

#if 0
  class Sampler {
    int                id;
    IDirect3DTexture9* pTex;
  };
#endif

#if 0
    struct sk_draw_states_s {
      bool         has_aniso      = false; // Has he game even once set anisotropy?!
      int          max_aniso      = 4;
      bool         has_msaa       = false;
      bool         use_msaa       = true;  // Allow MSAA toggle via console
                                           //  without changing the swapchain.
      D3DVIEWPORT9 vp             = { 0 };
      bool         postprocessing = false;
      bool         fullscreen     = false;

      DWORD        srcblend       = 0;
      DWORD        dstblend       = 0;
      DWORD        srcalpha       = 0;     // Separate Alpha Blend Eq: Src
      DWORD        dstalpha       = 0;     // Separate Alpha Blend Eq: Dst
      bool         alpha_test     = false; // Test Alpha?
      DWORD        alpha_ref      = 0;     // Value to test.
      bool         zwrite         = false; // Depth Mask

      int          last_vs_vec4   = 0; // Number of vectors in the last call to
                                       //   set vertex shader constant...

      int          draws          = 0; // Number of draw calls
      int          frames         = 0;
    } extern draw_state;
#endif

  class Texture {
  public:
    Texture (void) {
      crc32     = 0;
      size      = 0;
      refs      = 0;
      load_time = 0.0f;
      d3d9_tex  = nullptr;
    }

    uint32_t        crc32;
    size_t          size;
    int             refs;
    float           load_time;
    ISKTextureD3D9* d3d9_tex;
  };

  class TextureManager {
  public:
    void Init     (void);
    void Shutdown (void);

    void                     removeTexture   (ISKTextureD3D9* pTexD3D9);

    sk::d3d9::Texture*       getTexture (uint32_t crc32);
    void                     addTexture (uint32_t crc32, sk::d3d9::Texture* pTex, size_t size);

    // Record a cached reference
    void                     refTexture (sk::d3d9::Texture* pTex);

    void                     reset (void);
    void                     purge (void); // WIP

    size_t                   numTextures (void) {
      return textures.size ();
    }
    int                      numInjectedTextures (void);

    int64_t                  cacheSizeTotal    (void);
    int64_t                  cacheSizeBasic    (void);
    int64_t                  cacheSizeInjected (void);

    int                      numMSAASurfs (void);

    void                     addInjected (size_t size) {
      InterlockedIncrement (&injected_count);
      InterlockedAdd64     (&injected_size, size);
    }

    std::string              osdStats  (void) { return osd_stats; }
    void                     updateOSD (void);

  private:
    std::unordered_map <uint32_t, sk::d3d9::Texture*> textures;
    float                                             time_saved     = 0.0f;
    ULONG                                             hits           = 0UL;

    LONG64                                            basic_size     = 0LL;
    LONG64                                            injected_size  = 0LL;
    ULONG                                             injected_count = 0UL;

    std::string                                       osd_stats      = "";

    CRITICAL_SECTION                                  cs_cache;
  } extern tex_mgr;
}
}

#pragma comment (lib, "dxguid.lib")

const GUID IID_SKTextureD3D9 = { 0xace1f81b, 0x5f3f, 0x45f4, 0xbf, 0x9f, 0x1b, 0xaf, 0xdf, 0xba, 0x11, 0x9b };

interface ISKTextureD3D9 : public IDirect3DTexture9
{
public:
     ISKTextureD3D9 (IDirect3DTexture9 **ppTex, SIZE_T size, uint32_t crc32) {
         pTexOverride  = nullptr;
         can_free      = true;
         override_size = 0;
         last_used.QuadPart
                       = 0ULL;
         pTex          = *ppTex;
       *ppTex          =  this;
         tex_size      = size;
         tex_crc32     = crc32;
         must_block    = false;
         refs          =  1;
     };

    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj) {
      if (IsEqualGUID (riid, IID_SKTextureD3D9)) {
        return S_OK;
      }

#if 1
      if ( IsEqualGUID (riid, IID_IUnknown)              ||
           IsEqualGUID (riid, IID_IDirect3DTexture9)     ||
           IsEqualGUID (riid, IID_IDirect3DBaseTexture9)    )
      {
        AddRef ();
        *ppvObj = this;
        return S_OK;
      }

      return E_FAIL;
#else
      return pTex->QueryInterface (riid, ppvObj);
#endif
    }
    STDMETHOD_(ULONG,AddRef)(THIS) {
      ULONG ret = InterlockedIncrement (&refs);

      can_free = false;

      return ret;
    }
    STDMETHOD_(ULONG,Release)(THIS) {
      ULONG ret = InterlockedDecrement (&refs);

      if (ret == 1) {
        can_free = true;
      }

      if (ret == 0) {
        // Does not delete this immediately; defers the
        //   process until the next cached texture load.
        sk::d3d9::tex_mgr.removeTexture (this);
      }

      return ret;
    }

    /*** IDirect3DBaseTexture9 methods ***/
    STDMETHOD(GetDevice)(THIS_ IDirect3DDevice9** ppDevice) {
      tex_log.Log (L"[ Tex. Mgr ] ISKTextureD3D9::GetDevice (%ph)", ppDevice);
      return pTex->GetDevice (ppDevice);
    }
    STDMETHOD(SetPrivateData)(THIS_ REFGUID refguid,CONST void* pData,DWORD SizeOfData,DWORD Flags) {
      tex_log.Log ( L"[ Tex. Mgr ] ISKTextureD3D9::SetPrivateData (%p, %ph, %lu, %x)",
                      refguid,
                        pData,
                          SizeOfData,
                            Flags );
      return pTex->SetPrivateData (refguid, pData, SizeOfData, Flags);
    }
    STDMETHOD(GetPrivateData)(THIS_ REFGUID refguid,void* pData,DWORD* pSizeOfData) {
      tex_log.Log ( L"[ Tex. Mgr ] ISKTextureD3D9::GetPrivateData (%p, %ph, %lu)",
                      refguid,
                        pData,
                          *pSizeOfData );

      return pTex->GetPrivateData (refguid, pData, pSizeOfData);
    }
    STDMETHOD(FreePrivateData)(THIS_ REFGUID refguid) {
      tex_log.Log ( L"[ Tex. Mgr ] ISKTextureD3D9::FreePrivateData (%p)",
                      refguid );

      return pTex->FreePrivateData (refguid);
    }
    STDMETHOD_(DWORD, SetPriority)(THIS_ DWORD PriorityNew) {
      tex_log.Log ( L"[ Tex. Mgr ] ISKTextureD3D9::SetPriority (%lu)",
                      PriorityNew );

      return pTex->SetPriority (PriorityNew);
    }
    STDMETHOD_(DWORD, GetPriority)(THIS) {
      tex_log.Log ( L"[ Tex. Mgr ] ISKTextureD3D9::GetPriority ()" );

      return pTex->GetPriority ();
    }
    STDMETHOD_(void, PreLoad)(THIS) {
      tex_log.Log ( L"[ Tex. Mgr ] ISKTextureD3D9::PreLoad ()" );

      pTex->PreLoad ();
    }
    STDMETHOD_(D3DRESOURCETYPE, GetType)(THIS) {
      tex_log.Log ( L"[ Tex. Mgr ] ISKTextureD3D9::GetType ()" );

      return pTex->GetType ();
    }
    STDMETHOD_(DWORD, SetLOD)(THIS_ DWORD LODNew) {
      tex_log.Log ( L"[ Tex. Mgr ] ISKTextureD3D9::SetLOD (%lu)",
                      LODNew );

      return pTex->SetLOD (LODNew);
    }
    STDMETHOD_(DWORD, GetLOD)(THIS) {
      tex_log.Log ( L"[ Tex. Mgr ] ISKTextureD3D9::GetLOD ()" );

      return pTex->GetLOD ();
    }
    STDMETHOD_(DWORD, GetLevelCount)(THIS) {
      tex_log.Log ( L"[ Tex. Mgr ] ISKTextureD3D9::GetLevelCount ()" );

      return pTex->GetLevelCount ();
    }
    STDMETHOD(SetAutoGenFilterType)(THIS_ D3DTEXTUREFILTERTYPE FilterType) {
      tex_log.Log ( L"[ Tex. Mgr ] ISKTextureD3D9::SetAutoGenFilterType (%x)",
                      FilterType );

      return pTex->SetAutoGenFilterType (FilterType);
    }
    STDMETHOD_(D3DTEXTUREFILTERTYPE, GetAutoGenFilterType)(THIS) {
      tex_log.Log ( L"[ Tex. Mgr ] ISKTextureD3D9::GetAutoGenFilterType ()" );

      return pTex->GetAutoGenFilterType ();
    }
    STDMETHOD_(void, GenerateMipSubLevels)(THIS) {
      tex_log.Log ( L"[ Tex. Mgr ] ISKTextureD3D9::GenerateMipSubLevels ()" );

      pTex->GenerateMipSubLevels ();
    }
    STDMETHOD(GetLevelDesc)(THIS_ UINT Level,D3DSURFACE_DESC *pDesc) {
      tex_log.Log ( L"[ Tex. Mgr ] ISKTextureD3D9::GetLevelDesc (%lu, %ph)",
                      Level,
                        pDesc );
      return pTex->GetLevelDesc (Level, pDesc);
    }
    STDMETHOD(GetSurfaceLevel)(THIS_ UINT Level,IDirect3DSurface9** ppSurfaceLevel) {
      tex_log.Log ( L"[ Tex. Mgr ] ISKTextureD3D9::GetSurfaceLevel (%lu, %ph)",
                      Level,
                        ppSurfaceLevel );

      return pTex->GetSurfaceLevel (Level, ppSurfaceLevel);
    }
    STDMETHOD(LockRect)(THIS_ UINT Level,D3DLOCKED_RECT* pLockedRect,CONST RECT* pRect,DWORD Flags) {
      tex_log.Log ( L"[ Tex. Mgr ] ISKTextureD3D9::LockRect (%lu, %ph, %ph, %x)",
                      Level,
                        pLockedRect,
                          pRect,
                            Flags );

      return pTex->LockRect (Level, pLockedRect, pRect, Flags);
    }
    STDMETHOD(UnlockRect)(THIS_ UINT Level) {
      tex_log.Log ( L"[ Tex. Mgr ] ISKTextureD3D9::UnlockRect (%lu)", Level );

      return pTex->UnlockRect (Level);
    }
    STDMETHOD(AddDirtyRect)(THIS_ CONST RECT* pDirtyRect) {
      tex_log.Log ( L"[ Tex. Mgr ] ISKTextureD3D9::SetDirtyRect (...)" );

      return pTex->AddDirtyRect (pDirtyRect);
    }

    bool               can_free;      // Whether or not we can free this texture
    bool               must_block;    // Whether or not to draw using this texture before its
                                      //  override finishes streaming

    IDirect3DTexture9* pTex;          // The original texture data
    SSIZE_T            tex_size;      //   Original data size
    uint32_t           tex_crc32;     //   Original data checksum

    IDirect3DTexture9* pTexOverride;  // The overridden texture data (nullptr if unchanged)
    SSIZE_T            override_size; //   Override data size

    ULONG              refs;
    LARGE_INTEGER      last_used;     // The last time this texture was used (for rendering)
                                      //   different from the last time referenced, this is
                                      //     set when SetTexture (...) is called.
};

typedef enum D3DXIMAGE_FILEFORMAT { 
  D3DXIFF_BMP          = 0,
  D3DXIFF_JPG          = 1,
  D3DXIFF_TGA          = 2,
  D3DXIFF_PNG          = 3,
  D3DXIFF_DDS          = 4,
  D3DXIFF_PPM          = 5,
  D3DXIFF_DIB          = 6,
  D3DXIFF_HDR          = 7,
  D3DXIFF_PFM          = 8,
  D3DXIFF_FORCE_DWORD  = 0x7fffffff
} D3DXIMAGE_FILEFORMAT, *LPD3DXIMAGE_FILEFORMAT;

#define D3DX_DEFAULT ((UINT) -1)
typedef struct D3DXIMAGE_INFO {
  UINT                 Width;
  UINT                 Height;
  UINT                 Depth;
  UINT                 MipLevels;
  D3DFORMAT            Format;
  D3DRESOURCETYPE      ResourceType;
  D3DXIMAGE_FILEFORMAT ImageFileFormat;
} D3DXIMAGE_INFO, *LPD3DXIMAGE_INFO;
typedef HRESULT (STDMETHODCALLTYPE *D3DXCreateTextureFromFileInMemoryEx_pfn)
(
  _In_        LPDIRECT3DDEVICE9  pDevice,
  _In_        LPCVOID            pSrcData,
  _In_        UINT               SrcDataSize,
  _In_        UINT               Width,
  _In_        UINT               Height,
  _In_        UINT               MipLevels,
  _In_        DWORD              Usage,
  _In_        D3DFORMAT          Format,
  _In_        D3DPOOL            Pool,
  _In_        DWORD              Filter,
  _In_        DWORD              MipFilter,
  _In_        D3DCOLOR           ColorKey,
  _Inout_opt_ D3DXIMAGE_INFO     *pSrcInfo,
  _Out_opt_   PALETTEENTRY       *pPalette,
  _Out_       LPDIRECT3DTEXTURE9 *ppTexture
);

typedef HRESULT (STDMETHODCALLTYPE *D3DXSaveTextureToFile_pfn)(
  _In_           LPCWSTR                 pDestFile,
  _In_           D3DXIMAGE_FILEFORMAT    DestFormat,
  _In_           LPDIRECT3DBASETEXTURE9  pSrcTexture,
  _In_opt_ const PALETTEENTRY           *pSrcPalette
);

typedef HRESULT (WINAPI *D3DXSaveSurfaceToFile_pfn)
(
  _In_           LPCWSTR               pDestFile,
  _In_           D3DXIMAGE_FILEFORMAT  DestFormat,
  _In_           LPDIRECT3DSURFACE9    pSrcSurface,
  _In_opt_ const PALETTEENTRY         *pSrcPalette,
  _In_opt_ const RECT                 *pSrcRect
);

typedef HRESULT (STDMETHODCALLTYPE *CreateTexture_pfn)
(
  IDirect3DDevice9   *This,
  UINT                Width,
  UINT                Height,
  UINT                Levels,
  DWORD               Usage,
  D3DFORMAT           Format,
  D3DPOOL             Pool,
  IDirect3DTexture9 **ppTexture,
  HANDLE             *pSharedHandle
);

typedef HRESULT (STDMETHODCALLTYPE *CreateRenderTarget_pfn)
(
  IDirect3DDevice9     *This,
  UINT                  Width,
  UINT                  Height,
  D3DFORMAT             Format,
  D3DMULTISAMPLE_TYPE   MultiSample,
  DWORD                 MultisampleQuality,
  BOOL                  Lockable,
  IDirect3DSurface9   **ppSurface,
  HANDLE               *pSharedHandle
);

typedef HRESULT (STDMETHODCALLTYPE *CreateDepthStencilSurface_pfn)
(
  IDirect3DDevice9     *This,
  UINT                  Width,
  UINT                  Height,
  D3DFORMAT             Format,
  D3DMULTISAMPLE_TYPE   MultiSample,
  DWORD                 MultisampleQuality,
  BOOL                  Discard,
  IDirect3DSurface9   **ppSurface,
  HANDLE               *pSharedHandle
);

typedef HRESULT (STDMETHODCALLTYPE *SetTexture_pfn)(
  _In_ IDirect3DDevice9      *This,
  _In_ DWORD                  Sampler,
  _In_ IDirect3DBaseTexture9 *pTexture
);

typedef HRESULT (STDMETHODCALLTYPE *SetRenderTarget_pfn)(
  _In_ IDirect3DDevice9      *This,
  _In_ DWORD                  RenderTargetIndex,
  _In_ IDirect3DSurface9     *pRenderTarget
);

typedef HRESULT (STDMETHODCALLTYPE *SetDepthStencilSurface_pfn)(
  _In_ IDirect3DDevice9      *This,
  _In_ IDirect3DSurface9     *pNewZStencil
);

typedef HRESULT (STDMETHODCALLTYPE *BeginScene_pfn)(
  IDirect3DDevice9 *This
);

typedef HRESULT (STDMETHODCALLTYPE *EndScene_pfn)(
  IDirect3DDevice9 *This
);