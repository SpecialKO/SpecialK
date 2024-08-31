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

#include <SpecialK/render/d3d9/d3d9_backend.h>
#include <SpecialK/render/d3d9/d3d9_texmgr.h>

#include <d3d9.h>

#define __SK_SUBSYSTEM__ L" D3D9 Tex "

using namespace SK::D3D9;

extern SK_LazyGlobal <std::wstring> SK_D3D11_res_root;

static D3D9Device_BeginScene_pfn                 D3D9BeginScene_Original                = nullptr;
static D3D9Device_EndScene_pfn                   D3D9EndScene_Original                  = nullptr;
static D3D9Device_SetRenderState_pfn             D3D9SetRenderState_Original            = nullptr;

static D3D9Device_StretchRect_pfn                D3D9StretchRect_Original               = nullptr;
static D3D9Device_CreateTexture_pfn              D3D9CreateTexture_Original             = nullptr;
static D3D9Device_CreateRenderTarget_pfn         D3D9CreateRenderTarget_Original        = nullptr;
static D3D9Device_CreateDepthStencilSurface_pfn  D3D9CreateDepthStencilSurface_Original = nullptr;

static D3D9Device_SetTexture_pfn                 D3D9SetTexture_Original                = nullptr;
static D3D9Device_SetRenderTarget_pfn            D3D9SetRenderTarget_Original           = nullptr;
static D3D9Device_SetDepthStencilSurface_pfn     D3D9SetDepthStencilSurface_Original    = nullptr;
extern D3D9Device_SetSamplerState_pfn            D3D9Device_SetSamplerState_Original;
extern D3DXCreateTextureFromFileInMemoryEx_pfn   D3DXCreateTextureFromFileInMemoryEx_Original;
extern D3DXCreateCubeTextureFromFileInMemoryEx_pfn
                                                 D3DXCreateCubeTextureFromFileInMemoryEx_Original;
extern D3DXCreateVolumeTextureFromFileInMemoryEx_pfn
                                                 D3DXCreateVolumeTextureFromFileInMemoryEx_Original;
extern D3DXCreateTextureFromFileExW_pfn          D3DXCreateTextureFromFileExW_Original;
extern D3DXCreateTextureFromFileExA_pfn          D3DXCreateTextureFromFileExA_Original;

// D3DXSaveSurfaceToFile issues a StretchRect, but we don't want to log that...
bool dumping          = false;
bool __remap_textures = true;
bool __need_purge     = false;
bool __log_used       = false;
bool __show_cache     = true;//false;

SK::D3D9::TextureManager&
SK_D3D9_GetTextureManager (void)
{
  static SK::D3D9::TextureManager tex_mgr;
  return                          tex_mgr;
}

SK_D3D9_TextureStorageBase&
SK_D3D9_GetBasicTextureDataStore (void)
{
  static SK_D3D9_TextureStorageBase _datastore;
  return                            _datastore;
}


size_t
SK::D3D9::TextureManager::getTextureArchives (std::vector <std::wstring>& arcs)
{
  arcs = archives;

  return arcs.size ();
}

size_t
SK::D3D9::TextureManager::getInjectableTextures (SK::D3D9::TexList& texture_list) const
{
  texture_list.clear ();

  for ( auto& it : injectable_textures )
  {
    if (ReadAcquire (&it.second.removed) == 0)
    {
      texture_list.emplace_back (
        std::make_pair ( it.first,
                         it.second )
      );
    }
  }

  return texture_list.size ();
}

SK::D3D9::TexRecord&
SK::D3D9::TextureManager::getInjectableTexture (uint32_t checksum)
{
  static TexRecord  nulref     = {    };
         TexRecord& injectable = nulref;

  bool new_tex = false;

  if (         (0 == injectable_textures.count (checksum)) ||
       ReadAcquire (&injectable_textures       [checksum].removed) )
  {
    new_tex = true;
  }

  else
  {
    injectable =
      injectable_textures [checksum];
  }

  if (new_tex)
  {
    injectable = { };

    InterlockedExchange (
      &injectable.removed, FALSE
    );
  }

  return injectable;
}

#if 0
COM_DECLSPEC_NOTHROW
__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9StretchRect_Detour (      IDirect3DDevice9    *This,
                              IDirect3DSurface9   *pSourceSurface,
                        const RECT                *pSourceRect,
                              IDirect3DSurface9   *pDestSurface,
                        const RECT                *pDestRect,
                              D3DTEXTUREFILTERTYPE Filter )
{
#if 0
  if (tzf::RenderFix::tracer.log && (! dumping))
  {
    RECT source, dest;

    if (pSourceRect == nullptr) {
      D3DSURFACE_DESC desc;
      pSourceSurface->GetDesc (&desc);
      source.left   = 0;
      source.top    = 0;
      source.bottom = desc.Height;
      source.right  = desc.Width;
    } else
      source = *pSourceRect;

    if (pDestRect == nullptr) {
      D3DSURFACE_DESC desc;
      pDestSurface->GetDesc (&desc);
      dest.left   = 0;
      dest.top    = 0;
      dest.bottom = desc.Height;
      dest.right  = desc.Width;
    } else
      dest = *pDestRect;

    dll_log->Log ( L"[FrameTrace] StretchRect      - "
                   L"%s[%lu,%lu/%lu,%lu] ==> %s[%lu,%lu/%lu,%lu]",
                   pSourceRect != nullptr ?
                     L" " : L" *",
                   source.left, source.top, source.right, source.bottom,
                   pDestRect != nullptr ?
                     L" " : L" *",
                   dest.left,   dest.top,   dest.right,   dest.bottom );
  }
#endif

  dumping = false;

  return D3D9StretchRect (This, pSourceSurface, pSourceRect,
                                         pDestSurface,   pDestRect,
                                         Filter);
}
#endif

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9CreateRenderTarget_Detour (IDirect3DDevice9     *This,
                                UINT                  Width,
                                UINT                  Height,
                                D3DFORMAT             Format,
                                D3DMULTISAMPLE_TYPE   MultiSample,
                                DWORD                 MultisampleQuality,
                                BOOL                  Lockable,
                                IDirect3DSurface9   **ppSurface,
                                HANDLE               *pSharedHandle)
{
  tex_log->Log (L"[Unexpected][!] IDirect3DDevice9::CreateRenderTarget (%lu, %lu, "
                        L"%lu, %lu, %lu, %lu, %08ph, %08ph)",
                   Width, Height, Format, MultiSample, MultisampleQuality,
                   Lockable, ppSurface, pSharedHandle);

  return D3D9CreateRenderTarget_Original (This, Width, Height, Format,
                                          MultiSample, MultisampleQuality,
                                          Lockable, ppSurface, pSharedHandle);
}

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9CreateDepthStencilSurface_Detour (IDirect3DDevice9     *This,
                                      UINT                  Width,
                                      UINT                  Height,
                                      D3DFORMAT             Format,
                                      D3DMULTISAMPLE_TYPE   MultiSample,
                                      DWORD                 MultisampleQuality,
                                      BOOL                  Discard,
                                      IDirect3DSurface9   **ppSurface,
                                      HANDLE               *pSharedHandle)
{
  tex_log->Log (L"[Unexpected][!] IDirect3DDevice9::CreateDepthStencilSurface (%lu, %lu, "
                        L"%lu, %lu, %lu, %lu, %08ph, %08ph)",
                   Width, Height, Format, MultiSample, MultisampleQuality,
                   Discard, ppSurface, pSharedHandle);

  return
    D3D9CreateDepthStencilSurface_Original ( This, Width, Height, Format,
                                               MultiSample, MultisampleQuality,
                                                 Discard, ppSurface, pSharedHandle );
}

int
SK::D3D9::TextureManager::numInjectedTextures (void) const
{
  return
    ReadAcquire (&injected_count);
}

int64_t
SK::D3D9::TextureManager::cacheSizeInjected (void) const
{
  return
    ReadAcquire64 (&injected_size);
}

int64_t
SK::D3D9::TextureManager::cacheSizeBasic (void) const
{
  return
    ReadAcquire64 (&basic_size);
}

int64_t
SK::D3D9::TextureManager::cacheSizeTotal (void) const
{
  return
    std::max (0LL, cacheSizeBasic    ()) +
    std::max (0LL, cacheSizeInjected ());
}

bool
SK::D3D9::TextureManager::isRenderTarget (IDirect3DBaseTexture9* pTex) const
{
  return
    known.render_targets.count (pTex) != 0;
}

void
SK::D3D9::TextureManager::trackRenderTarget (IDirect3DBaseTexture9* pTex)
{
  if (! known.render_targets.count (pTex))
  {
    known.render_targets.insert (
      std::make_pair ( pTex,
               (uint32_t)known.render_targets.size ()
                     )
    );
  }
}

void
SK::D3D9::TextureManager::applyTexture (IDirect3DBaseTexture9* pTex)
{
  if (known.render_targets.count (pTex) != 0)
  {
    used.render_targets.insert (pTex);
  }
}

bool
SK::D3D9::TextureManager::isUsedRenderTarget (IDirect3DBaseTexture9* pTex) const
{
  return used.render_targets.count (pTex) != 0;
}

void
SK::D3D9::TextureManager::resetUsedTextures (void)
{
  used.render_targets.clear ();
}

size_t
SK::D3D9::TextureManager::getUsedRenderTargets (std::vector <IDirect3DBaseTexture9 *>& targets) const
{
  targets = {
    used.render_targets.cbegin (),
    used.render_targets.cend   ()
  };

  return
    targets.size ();
}

uint32_t
SK::D3D9::TextureManager::getRenderTargetCreationTime (IDirect3DBaseTexture9* pTex)
{
  if (known.render_targets.count (pTex))
    return  known.render_targets [pTex];

  return 0xFFFFFFFFUL;
}

COM_DECLSPEC_NOTHROW
__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D9StretchRect_Detour (      IDirect3DDevice9    *This,
                               IDirect3DSurface9   *pSourceSurface,
                         const RECT                *pSourceRect,
                               IDirect3DSurface9   *pDestSurface,
                         const RECT                *pDestRect,
                               D3DTEXTUREFILTERTYPE Filter )
{
  dumping = false;

  return
    D3D9StretchRect_Original ( This, pSourceSurface, pSourceRect,
                                 pDestSurface,   pDestRect,
                                   Filter );
}


SK_LazyGlobal <std::set <UINT>> SK::D3D9::active_samplers;

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9SetDepthStencilSurface_Detour (
                  _In_ IDirect3DDevice9  *This,
                  _In_ IDirect3DSurface9 *pNewZStencil
)
{
  return D3D9SetDepthStencilSurface_Original (This, pNewZStencil);
}


uint32_t debug_tex_id      =   0UL;
uint32_t current_tex [256] = { 0ui32 };


COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9SetTexture_Detour (
                   _In_ IDirect3DDevice9      *This,
                   _In_ DWORD                  Sampler,
                   _In_ IDirect3DBaseTexture9 *pTexture
)
{
  static auto& _Shaders    = Shaders.get    ();
  static auto& _tracked_rt = tracked_rt.get ();
  static auto& _tracked_vs = tracked_vs.get ();
  static auto& _tracked_ps = tracked_ps.get ();

  auto& tex_mgr =
    SK_D3D9_GetTextureManager ();

  if (tex_mgr.injector.isInjectionThread ())
  {
    return
      D3D9SetTexture_Original (This, Sampler, pTexture);
  }

  //if (tzf::RenderFix::tracer.log) {
    //dll_log.Log ( L"[FrameTrace] SetTexture      - Sampler: %lu, pTexture: %ph",
    //               Sampler, pTexture );
  //}

  tex_mgr.applyTexture  (pTexture);
  _tracked_rt.active  = (pTexture == _tracked_rt.tracking_tex);

  if (_tracked_rt.active)
  {
    _tracked_rt.vertex_shaders.emplace (_Shaders.vertex.current.crc32c);
    _tracked_rt.pixel_shaders.emplace  (_Shaders.pixel.current.crc32c);
  }


  if (_Shaders.vertex.current.crc32c == _tracked_vs.crc32c)
    _tracked_vs.current_textures [std::min (15UL, Sampler)] = pTexture;

  if (_Shaders.pixel.current.crc32c == _tracked_ps.crc32c)
    _tracked_ps.current_textures [std::min (15UL, Sampler)] = pTexture;



  uint32_t tex_crc32c = 0x0;
  void*    dontcare   = nullptr;

  HRESULT hr =
    pTexture != nullptr ?
    pTexture->QueryInterface (IID_SKTextureD3D9, &dontcare)
                        : S_OK;

  if (hr == S_OK && pTexture != nullptr)
  {
    auto* pSKTex =
      static_cast <ISKTextureD3D9 *> (pTexture);

    current_tex [std::min (255UL, Sampler)] = pSKTex->tex_crc32c;

    //if (Shaders.vertex.current.crc32c == tracked_vs.crc32c)
    //    tracked_vs.current_textures [std::min (15UL, Sampler)] = pSKTex->tex_crc32c;
    //
    //if (Shaders.pixel.current.crc32c  == tracked_ps.crc32c)
    //    tracked_ps.current_textures [std::min (15UL, Sampler)] = pSKTex->tex_crc32c;

    if (pSKTex->tex_crc32c != 0x00)
    {
      tex_mgr.textures_used.insert (pSKTex->tex_crc32c);

      ////tex_log->Log (L"Used: %x", pSKTex->tex_crc32c);
    }

    tex_crc32c =
      pSKTex->tex_crc32c;

    pSKTex->use ();

    //
    // This is how blocking is implemented -- only do it when a texture that needs
    //                                          this feature is being applied.
    //
    while ( __remap_textures && pSKTex->must_block &&
                                pSKTex->pTexOverride == nullptr )
    {
      if (tex_mgr.injector.hasPendingLoads ())
      {         tex_mgr.loadQueuedTextures ();
      }

      else
      {
        SwitchToThread ();
        //YieldProcessor ();
        break;
      }
    }

    // XXX: Why are we assigning this a second time?
    pTexture = pSKTex->getDrawTexture ();
  }

#if 0
  if (pTexture != nullptr) tsf::RenderFix::active_samplers.insert (Sampler);
  else                     tsf::RenderFix::active_samplers.erase  (Sampler);
#endif

  bool clamp = false;

  if (_Shaders.pixel.current.crc32c  == _tracked_ps.crc32c && _tracked_ps.clamp_coords)
    clamp = true;

  if (_Shaders.vertex.current.crc32c == _tracked_vs.crc32c && _tracked_vs.clamp_coords)
    clamp = true;

  if ( clamp )
  {
    float fMin = -3.0f;

    D3D9Device_SetSamplerState_Original (This, Sampler, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
    D3D9Device_SetSamplerState_Original (This, Sampler, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );
    D3D9Device_SetSamplerState_Original (This, Sampler, D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP );
    D3D9Device_SetSamplerState_Original (This, Sampler, D3DSAMP_MIPMAPLODBIAS, *reinterpret_cast <DWORD *>(&fMin) );
  }

  return
    D3D9SetTexture_Original (This, Sampler, pTexture);
}

IDirect3DSurface9* pOld     = nullptr;

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9CreateTexture_Detour (IDirect3DDevice9    *This,
                           UINT                Width,
                           UINT                Height,
                           UINT                Levels,
                           DWORD               Usage,
                           D3DFORMAT           Format,
                           D3DPOOL             Pool,
                           IDirect3DTexture9 **ppTexture,
                           HANDLE             *pSharedHandle)
{
  SK_ComQIPtr <IDirect3DDevice9Ex>
       pDev9Ex (This);
  if ( pDev9Ex.p != nullptr &&
            Pool == D3DPOOL_MANAGED )
  {
    SK_LOGi1 (L" >> Reassigning Managed 2D Texture to 'Default' Pool (D3D9Ex Override)");

    Pool   =  D3DPOOL_DEFAULT;
    Usage |= D3DUSAGE_DYNAMIC;
  }

  if (config.system.log_level > 1)
  {
    char  szSymbol [1024] = { };
    ULONG ulLen  =  1024;

    ulLen =
      SK_GetSymbolNameFromModuleAddr ( SK_GetCallingDLL (),
                              (uintptr_t)_ReturnAddress (),
                                           szSymbol,
                                             ulLen );

    SK_LOGi0 ( L"CreateTexture (Usage=%hs, Format=%hs, Pool=%hs) [%hs]",
               SK_D3D9_UsageToStr  (Usage).c_str  (),
               SK_D3D9_FormatToStr (Format).c_str (),
               SK_D3D9_PoolToStr   (Pool), szSymbol );
  }

  SK::D3D9::TextureManager& tex_mgr =
    SK_D3D9_GetTextureManager ();

  if (config.textures.d3d9_mod || tex_mgr.init)
  {
    if ( (Usage & D3DUSAGE_RENDERTARGET) !=
                  D3DUSAGE_RENDERTARGET )
    {
      if ( Format >= D3DFMT_DXT1     && Format <= D3DFMT_DXT5 &&
           Pool   == D3DPOOL_DEFAULT && ( (Usage & D3DUSAGE_DYNAMIC) !=
                                                   D3DUSAGE_DYNAMIC ) )
      {
        // So we can dump this thing
        Usage |= D3DUSAGE_DYNAMIC;
      }
    }
  }

#if 0
  if (Usage == D3DUSAGE_RENDERTARGET)
  dll_log->Log (L" [!] IDirect3DDevice9::CreateTexture (%lu, %lu, %lu, %lu, "
                                                   L"%lu, %lu, %08Xh, %08Xh)",
                  Width, Height, Levels, Usage, Format, Pool, ppTexture,
                  pSharedHandle);
#endif
//#endif

  if (SK_GetCurrentGameID () == SK_GAME_ID::YS_Seven)
  {
    if ( ( Usage & D3DUSAGE_RENDERTARGET ) )
    {
      if (Format == D3DFMT_R5G6B5 && ( Width != 2048 ) )
      {
        Format   = D3DFMT_X8R8G8B8;
#if 0
          Width  <<= 1;
          Height <<= 1;
#endif
      }
    }
  }

  HRESULT result =
    D3D9CreateTexture_Original ( This,
                                   Width, Height, Levels,
                                     Usage, Format, Pool,
                                       ppTexture,
                                         pSharedHandle );

  if (SUCCEEDED (result) && (! tex_mgr.injector.isInjectionThread ()) && ppTexture != nullptr)
  {
    //if (config.textures.log) {
    //  tex_log->Log ( L"[Load-Trace] >> Creating Texture: "
    //                 L"(%d x %d), Format: %hs, Usage: [%hs], Pool: %hs",
    //                   Width, Height,
    //                     SK_D3D9_FormatToStr (Format).c_str (),
    //                     SK_D3D9_UsageToStr  (Usage).c_str (),
    //                     SK_D3D9_PoolToStr   (Pool) );
    //}

    if ( ( Usage & D3DUSAGE_RENDERTARGET ) == D3DUSAGE_RENDERTARGET ||
         ( Usage & D3DUSAGE_DEPTHSTENCIL ) == D3DUSAGE_DEPTHSTENCIL )
    {
      tex_mgr.trackRenderTarget (*ppTexture);
    }

    else
    {
      ISKTextureD3D9* dontcare;
      if (FAILED ((*ppTexture)->QueryInterface (IID_SKTextureD3D9, (void **)&dontcare)))
      {
        new ISKTextureD3D9 (ppTexture, 0, 0x00);
      }
    }
  }

  return result;
}

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9BeginScene_Detour (IDirect3DDevice9* This)
{
  //// Ignore anything that's not the primary render device.
  //if (! SK_GetCurrentRenderBackend ().device.IsEqualObject (This))
  //{
  //  dll_log.Log (L"[D3D9 BkEnd] >> WARNING: D3D9 BeginScene came from unknown IDirect3DDevice9! (expected %p, got %p) << ", SK_GetCurrentRenderBackend ().device.p, This);
  //
  //  return D3D9BeginScene_Original (This);
  //}

  draw_state->draws = 0;

  const HRESULT result =
    D3D9BeginScene_Original (This);

  return result;
}

#if 0
COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9EndScene_Detour (IDirect3DDevice9* This)
{
  // Ignore anything that's not the primary render device.
  if (This != tzf::RenderFix::pDevice) {
    return D3D9EndScene_Original (This);
  }
  return D3D9EndScene_Original (This);
}
#endif

#define __PTR_SIZE   sizeof LPCVOID
#define __PAGE_PRIVS PAGE_EXECUTE_READWRITE

#define D3D9_VIRTUAL_OVERRIDE(_Base,_Index,_Name,_Override,_Original,_Type) { \
  void** vftable = *(void***)*(_Base);                                        \
                                                                              \
  if (vftable [_Index] != (_Override)) {                                      \
    DWORD dwProtect;                                                          \
                                                                              \
    VirtualProtect (&vftable [_Index], __PTR_SIZE, __PAGE_PRIVS, &dwProtect); \
                                                                              \
    /*dll_log->Log (L" Old VFTable entry for %s: %08Xh  (Memory Policy: %s)",*/\
                 /*L##_Name, vftable [_Index],                              */\
                 /*SK_DescribeVirtualProtectFlags (dwProtect));             */\
                                                                              \
    if (_Original == NULL)                                                    \
      (_Original) = (##_Type)vftable [_Index];                                \
                                                                              \
    /*dll_log->Log (L"  + %s: %08Xh", L#_Original, _Original);*/              \
                                                                              \
    vftable [_Index] = (_Override);                                           \
                                                                              \
    VirtualProtect (&vftable [_Index], __PTR_SIZE, dwProtect, &dwProtect);    \
                                                                              \
    /*dll_log->Log (L" New VFTable entry for %s: %08Xh  (Memory Policy: %s)\n",*/\
                  /*L##_Name, vftable [_Index],                               */\
                  /*SK_DescribeVirtualProtectFlags (dwProtect));              */\
  }                                                                           \
}

SK::D3D9::TextureWorkerThread::TextureWorkerThread (SK::D3D9::TextureThreadPool* pool)
{
  pool_ = pool;
  job_  = nullptr;

  control_.start =
    SK_CreateEvent (nullptr, FALSE, FALSE, nullptr);
  control_.trim =
    SK_CreateEvent (nullptr, FALSE, FALSE, nullptr);
  control_.shutdown =
    SK_CreateEvent (nullptr, FALSE, FALSE, nullptr);

  static volatile LONG worker = 0;

  name_ =
      SK_FormatStringW ( L"[SK] D3D9 Texture Worker < %02lu >",
        (uint32_t)(-1 + InterlockedIncrement (&worker))
      );

  thread_id_ =
    GetThreadId ( ( thread_ =
                      SK_Thread_CreateEx ( ThreadProc,
                                             name_.c_str (),
                                               this ) ) );
}

SK::D3D9::TextureWorkerThread::~TextureWorkerThread (void)
{
  shutdown ();

  SK_WaitForSingleObject (thread_, INFINITE);

  SK_CloseHandle (control_.shutdown);
  SK_CloseHandle (control_.trim);
  SK_CloseHandle (control_.start);

  SK_CloseHandle (thread_);
}

void
SK::D3D9::TextureManager::Injector::init (void)
{
  InitializeCriticalSectionAndSpinCount (&cs_tex_blacklist, 100000);
  InitializeCriticalSectionAndSpinCount (&cs_tex_resample,  10000);
  InitializeCriticalSectionAndSpinCount (&cs_tex_stream,    10000);
  InitializeCriticalSectionAndSpinCount (&cs_tex_dump,      1000);

  streaming         = 0L;
  streaming_bytes   = 0UL;

  resampling        = 0L;
}

bool
SK::D3D9::TextureManager::Injector::hasPendingLoads (void) const
{
  //bool ret = false;

  return
    ( stream_pool.working () );//||
        //( resample_pool != nullptr && resample_pool->working () ) );

//  EnterCriticalSection (&cs_tex_inject);
//  ret = (! finished_loads.empty ());
//  LeaveCriticalSection (&cs_tex_inject);

//return ret;
}

bool
SK::D3D9::TextureManager::Injector::beginLoad (void)
{
  auto pTLS =
    SK_TLS_Bottom ();

  BOOL bOriginal =
    std::exchange (pTLS->texture_management.injection_thread, TRUE);

  return
    ( bOriginal != FALSE );
}

void
SK::D3D9::TextureManager::Injector::endLoad (bool bOriginal)
{
  SK_TLS_Bottom ()->texture_management.injection_thread = bOriginal ?
                                                               TRUE : FALSE;
}


bool
SK::D3D9::TextureManager::Injector::hasPendingStreams (void) const
{
  bool ret = false;

  if (ReadAcquire (&streaming) != 0 || stream_pool.queueLength () /*|| (resample_pool && resample_pool->queueLength ())*/)
    ret = true;

  return ret;
}

bool
SK::D3D9::TextureManager::Injector::isStreaming (uint32_t checksum) const
{
  bool ret = false;

  if ( textures_in_flight.count (checksum) &&
       textures_in_flight.at    (checksum) != nullptr )
    ret = true;

  return ret;
}

void
SK::D3D9::TextureManager::Injector::finishedStreaming (uint32_t checksum)
{
  textures_in_flight [checksum] = nullptr;
}

void
SK::D3D9::TextureManager::Injector::addTextureInFlight (SK::D3D9::TexLoadRequest *load_op)
{
  textures_in_flight.insert (
    std::make_pair (load_op->checksum, load_op)
  );
}

SK::D3D9::TexLoadRequest*
SK::D3D9::TextureManager::Injector::getTextureInFlight (uint32_t checksum)
{
  SK::D3D9::TexLoadRequest* pLoadRequest = nullptr;

  // What to do if this load finishes before the thing that acquired the lock is done?
  if (isStreaming (checksum))
    pLoadRequest = textures_in_flight [checksum];

  return pLoadRequest;
}


HANDLE decomp_semaphore;

#include <SpecialK/tls.h>

// Keep a pool of memory around so that we are not allocating and freeing
//  memory constantly...
namespace streaming_memory {
  void* heap_alloc (size_t len, bool temp = false)
  {
    SK_TLS::tex_mgmt_s::stream_pool_s* mpool =
      &SK_TLS_Bottom ()->texture_management.streaming_memory;

    auto& heap =
      ( temp ? mpool->heap.temporary :
               mpool->heap.primary );

    if (heap == INVALID_HANDLE_VALUE)
    {
      heap =
        HeapCreate (0, 0, 0);
    }

    if (heap == nullptr)
      return nullptr;

    return
      HeapAlloc (heap, 0x0, len);
  }

  bool heap_free (void* p, bool temp = false)
  {
    SK_TLS::tex_mgmt_s::stream_pool_s* mpool =
      &SK_TLS_Bottom ()->texture_management.streaming_memory;

    auto& heap =
      ( temp ? mpool->heap.temporary :
               mpool->heap.primary );

    if (heap == INVALID_HANDLE_VALUE)
    {
      return false;
    }

    return
      HeapFree (heap, 0x0, p) != FALSE;
  }

  bool alloc (size_t len)
  {
    SK_TLS::tex_mgmt_s::stream_pool_s* mpool =
      &SK_TLS_Bottom ()->texture_management.streaming_memory;

    if (mpool->data_len < len)
    {
      delete [] mpool->data;

      if (len < 8192 * 1024)
        mpool->data_len = 8192 * 1024;
      else
        mpool->data_len = len;

      mpool->data =
        new (std::nothrow) uint8_t [mpool->data_len];

      mpool->data_age = SK_timeGetTime ();

      if (mpool->data != nullptr)
      {
        return true;
      }

      mpool->data_len = 0;
      return false;
    }

    return true;
  }

  uint8_t*& data (void)
  {
    SK_TLS::tex_mgmt_s::stream_pool_s* mpool =
      &SK_TLS_Bottom ()->texture_management.streaming_memory;

    return mpool->data;
  }

  size_t& data_len (void)
  {
    SK_TLS::tex_mgmt_s::stream_pool_s* mpool =
      &SK_TLS_Bottom ()->texture_management.streaming_memory;

    return mpool->data_len;
  }

  uint32_t& data_age (void)
  {
    SK_TLS::tex_mgmt_s::stream_pool_s* mpool =
      &SK_TLS_Bottom ()->texture_management.streaming_memory;

    return mpool->data_age;
  }

  void trim (size_t max_size, uint32_t min_age)
  {
    SK_TLS::tex_mgmt_s::stream_pool_s* mpool =
      &SK_TLS_Bottom ()->texture_management.streaming_memory;

    if (  mpool->data_age < min_age )
    {
      if (mpool->data_len > max_size)
      {
        delete []
          std::exchange (mpool->data, nullptr);

        if (max_size > 0)
          mpool->data = new (std::nothrow) uint8_t [max_size];

        if (mpool->data != nullptr)
        {
          mpool->data_len = max_size;
          mpool->data_age = SK_timeGetTime ();
        }

        else
        {
          mpool->data_len = 0;
          mpool->data_age = 0;
        }
      }
    }
  }
}

bool
SK::D3D9::TextureManager::isTextureBlacklisted (uint32_t/* checksum*/) const
{
  return false;

//  bool bRet = false;
//
//  injector.lockBlacklist ();
//
//  bRet = ( inject_blacklist.count (checksum) != 0 );
//
//  injector.unlockBlacklist ();
//
//  return bRet;
}

bool
SK::D3D9::TextureManager::isTextureInjectable (uint32_t checksum) const
{
  return             ( injectable_textures.count (checksum) != 0 &&
         ReadAcquire (&injectable_textures.at    (checksum).removed) == FALSE );
}

bool
SK::D3D9::TextureManager::removeInjectableTexture (uint32_t checksum)
{
  return                         injectable_textures.count (checksum) != 0 &&
    InterlockedCompareExchange (&injectable_textures.at    (checksum).removed, TRUE, FALSE);
}

HRESULT
SK::D3D9::TextureManager::injectTexture (TexLoadRequest* load)
{
  D3DXIMAGE_INFO img_info = {    };
  bool           streamed =  false;
  size_t         size     =      0;
  HRESULT        hr       = E_FAIL;

  auto inject =
    injectable_textures.find (load->checksum);

  if (inject == injectable_textures.cend ())
  {
    tex_log->Log ( L"[Inject Tex]  >> Load Request for Checksum: %X "
                   L"has no Injection Record !!",
                     load->checksum );

    return E_NOT_VALID_STATE;
  }

  const TexRecord* inj_tex =
    &(*inject).second;

  streamed =
    (inj_tex->method == Streaming);

  //
  // Load:  From Regular Filesystem
  //
  if ( inj_tex->archive == std::numeric_limits <unsigned int>::max () )
  {
    HANDLE hTexFile =
      CreateFile ( load->wszFilename,
                     GENERIC_READ,
                       FILE_SHARE_READ,
                         nullptr,
                           OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL |
                             FILE_FLAG_SEQUENTIAL_SCAN,
                               nullptr );

    DWORD read = 0UL;

    if (hTexFile != INVALID_HANDLE_VALUE)
    {
      size =
        GetFileSize (hTexFile, nullptr);

      if (size > 0 && streaming_memory::alloc (size))
      {
        load->pSrcData = streaming_memory::data ();

        if (ReadFile (hTexFile, load->pSrcData, (DWORD)size, &read, nullptr))
        {
          load->SrcDataSize = read;

          if (streamed && size > (128 * 1024))
          {
            SetThreadPriority ( SK_GetCurrentThread (),
                                  THREAD_PRIORITY_BELOW_NORMAL |
                                  THREAD_MODE_BACKGROUND_BEGIN );
          }

          auto _LogD3DXFailure = []( wchar_t *wszFunc,
                                     wchar_t *wszFile,
                                     uint32_t checksum,
                                     HRESULT  hr )
          {
            tex_log->Log (
                L"[Inject Tex]  >> %ws (...) failed for file '%ws' "
                L"(crc32c=%x); hr=%x",  wszFunc, wszFile,
                                       checksum, hr );
          };

          hr =
            SK_D3DXGetImageInfoFromFileInMemory (
              load->pSrcData,
                load->SrcDataSize,
                  &img_info );

          if (SUCCEEDED (hr))
          {
            auto& tex_mgr =
              SK_D3D9_GetTextureManager ();

            bool bLoading =
              tex_mgr.injector.beginLoad ();

            hr =
              SK_D3DXCreateTextureFromFileInMemoryEx (
                load->pDevice,
                  load->pSrcData, load->SrcDataSize,
                    D3DX_DEFAULT, D3DX_DEFAULT, img_info.MipLevels,
                      0, D3DFMT_FROM_FILE, load->mem_pool,
                          D3DX_DEFAULT, D3DX_DEFAULT,
                            0,
                              &img_info, nullptr,
                                &load->pSrc );

            if (FAILED (hr))
            {
              _LogD3DXFailure (
                L"SK_D3DXCreateTextureFromFileInMemoryEx",
                  load->wszFilename, load->checksum, hr );
            }

            tex_mgr.injector.endLoad (bLoading);

            load->pSrcData = nullptr;
          }

          else
          {
            _LogD3DXFailure (
              L"SK_D3DXGetImageInfoFromFileInMemory",
                load->wszFilename, load->checksum, hr );
          }
        }
      }

      else {
        // OUT OF MEMORY ?!
        hr = E_POINTER;
      }

      SK_CloseHandle (hTexFile);
    }

    else
      hr = E_HANDLE;
  }

  //
  // Load:  From (Compressed) Archive (.7z or .zip)
  //
  else
  {
    wchar_t       arc_name [MAX_PATH + 2] = { };

    static auto _SzAlloc = [](void* p, size_t size)->void*
    {
      std::ignore = p;

      return
        streaming_memory::heap_alloc (size);
    };
    static auto _SzFree = [](void *p, void *address)->void
    {
      std::ignore = p;

      streaming_memory::heap_free (address);
    };

    static auto _SzAllocTemp = [](void* p, size_t size)->void*
    {
      std::ignore = p;

      return
        streaming_memory::heap_alloc (size, true);
    };
    static auto _SzFreeTemp = [](void *p, void *address)->void
    {
      std::ignore = p;

      streaming_memory::heap_free (address, true);
    };

    CFileInStream arc_stream              = { };
    ISzAlloc      thread_alloc            = { };
    ISzAlloc      thread_tmp_alloc        = { };

    CLookToRead   look_stream;

    FileInStream_CreateVTable (&arc_stream);
    LookToRead_CreateVTable   (&look_stream, True);

    look_stream.realStream = &arc_stream.s;
    LookToRead_Init         (&look_stream);

    thread_alloc.Alloc     = _SzAlloc;
    thread_alloc.Free      = _SzFree;

    thread_tmp_alloc.Alloc = _SzAllocTemp;
    thread_tmp_alloc.Free  = _SzFreeTemp;

    CSzArEx      arc    = { };
                 size   = inj_tex->size;
    int          fileno = inj_tex->fileno;

    if (inj_tex->archive < archives.size ())
      wcscpy (arc_name, archives [inj_tex->archive].c_str ());
    else
      wcscpy (arc_name, L"INVALID");

    if (streamed && size > (128 * 1024))
    {
      SetThreadPriority ( SK_GetCurrentThread (),
                            THREAD_PRIORITY_LOWEST |
                            THREAD_MODE_BACKGROUND_BEGIN );
    }

    if (InFile_OpenW (&arc_stream.file, arc_name))
    {
      tex_log->Log ( L"[Inject Tex]  ** Cannot open archive file: %s",
                       arc_name );

      return E_FAIL;
    }

    SzArEx_Init (&arc);

    if (SzArEx_Open (&arc, &look_stream.s, &thread_alloc, &thread_tmp_alloc) != SZ_OK)
    {
      tex_log->Log ( L"[Inject Tex]  ** Cannot open archive file: %s",
                       arc_name );

      File_Close  (&arc_stream.file);
      SzArEx_Free (&arc, &thread_alloc);

      return E_INVALIDARG;
    }

    load->pSrcData = nullptr;
    bool wait      = true;

    while (wait)
    {
      DWORD dwResult = WAIT_OBJECT_0;

      if (streamed && size > (128 * 1024))
      {
        dwResult =
          SK_WaitForSingleObject ( decomp_semaphore, INFINITE );
      }

      switch (dwResult)
      {
        case WAIT_OBJECT_0:
        {
          UInt32   block_idx     = 0xFFFFFFFF;
          auto*    out           = (Byte *)nullptr;//static_cast <Byte *> (streaming_memory::data     ());
          size_t   out_len       =                       0;//size;//streaming_memory::data_len ();
          size_t   offset        = 0;
          size_t   decomp_size   = 0;

          if (SZ_OK == SzArEx_Extract ( &arc,          &look_stream.s, fileno,
                                        &block_idx,    &out,        &out_len,
                                        &offset,       &decomp_size,
                                        &thread_alloc, &thread_tmp_alloc ) )
          {
            if (streamed && size > (128 * 1024))
              ReleaseSemaphore (decomp_semaphore, 1, nullptr);

            wait = false;

            load->pSrcData    = sk::narrow_cast <Byte *> (out);//streaming_memory::data ());
            load->SrcDataSize = sk::narrow_cast <UINT>   (out_len);

            bool bLoading =
              injector.beginLoad ();

            hr = 
              SK_D3DXGetImageInfoFromFileInMemory (
                              load->pSrcData,
                                load->SrcDataSize,
                                  &img_info );

            injector.endLoad (bLoading);

            if (SUCCEEDED (hr))
            {
              if (load->pSrc != nullptr)
              {
                // For proper operation of injection cache,
                //   we need to match the game's memory pool
                D3DSURFACE_DESC               surfDesc = { };
                load->pSrc->GetLevelDesc (0, &surfDesc);
                load->mem_pool =              surfDesc.Pool;

                load->pSrc->Release (); // Release wasn't here before, but it looks important...
                load->pSrc = nullptr;
              }


              bLoading =
                injector.beginLoad ();

              hr =
                SK_D3DXCreateTextureFromFileInMemoryEx (
                  load->pDevice,
                    load->pSrcData, load->SrcDataSize,
                      img_info.Width, img_info.Height, img_info.MipLevels,
                        0, img_info.Format,
                          load->mem_pool,
                            D3DX_DEFAULT, D3DX_DEFAULT,
                              0,
                                &img_info, nullptr,
                                  &load->pSrc );

              injector.endLoad (bLoading);
            }
          }

          else
          {
            tex_log->Log ( L"[  Tex. Mgr  ] Unable to read from 7-Zip File... for texture %x",
                           load->checksum );
          }
        } break;

        default:
          tex_log->Log ( L"[  Tex. Mgr  ] Unexpected Wait Status: %X (crc32=%x)",
                           dwResult,
                             load->checksum );
          wait = false;
          break;
      }
    }

    load->pSrcData = nullptr;
    load->SrcDataSize = 0;

    File_Close  (&arc_stream.file);
    SzArEx_Free (&arc, &thread_alloc);
  }

  if (streamed && size > (128 * 1024))
  {
    SetThreadPriority ( SK_GetCurrentThread (),
                          THREAD_MODE_BACKGROUND_END );
  }

  return hr;
}

CRITICAL_SECTION osd_cs            = { };
DWORD            last_queue_update =   0;

SK_LazyGlobal <std::string> mod_text;

void
SK::D3D9::TextureManager::updateQueueOSD (void)
{
  if (false)//true)//config.textures.show_loading_text)
  {
    DWORD dwTime =
         SK_timeGetTime ();

    //if (TryEnterCriticalSection (&osd_cs))
    {
      LONG resample_count = ReadAcquire (&injector.resampling); size_t queue_len = 0;//resample_pool->queueLength ();
      LONG stream_count   = ReadAcquire (&injector.streaming);  size_t to_stream = injector.textures_to_stream.unsafe_size ();

      bool is_resampling = false;//(resample_pool->working () || resample_count || queue_len);
      bool is_streaming  = (stream_pool.working    () || stream_count   || to_stream);

      static std::string resampling_text; static DWORD dwLastResample = 0;
      static std::string streaming_text;  static DWORD dwLastStream   = 0;

      if (is_resampling)
      {
        size_t count = queue_len + resample_count;

            char szFormatted [64];
        sprintf (szFormatted, "  Resampling: %zu texture", count);

        resampling_text  = szFormatted;
        resampling_text += (count != 1) ? 's' : ' ';

        if (queue_len)
        {
          sprintf (szFormatted, " (%zu queued)", queue_len);
          resampling_text += szFormatted;
        }

        resampling_text += "\n";

        if (count)
          dwLastResample = dwTime;
      }

      if (is_streaming)
      {
        size_t count = stream_count + to_stream;

            char szFormatted [64];
        sprintf (szFormatted, "  Streaming:  %zu texture", count);

        streaming_text  = szFormatted;
        streaming_text += (count != 1) ? 's' : ' ';

        sprintf (szFormatted, " [%7.2f MiB]", (double)ReadAcquire ((volatile LONG *)&injector.streaming_bytes) / (1024.0 * 1024.0));
        streaming_text += szFormatted;

        if (to_stream)
        {
          sprintf (szFormatted, " (%zu queued)", to_stream);
          streaming_text += szFormatted;
        }

        if (count)
          dwLastStream = dwTime;
      }

      if (dwLastResample < dwTime - 150)
        resampling_text.clear ();

      if (dwLastStream < dwTime - 150)
        streaming_text.clear ();

      *mod_text =
        ( resampling_text + streaming_text );

      if (! mod_text->empty ())
        last_queue_update = dwTime;

      //LeaveCriticalSection (&osd_cs);
    }
  }
}

int
SK::D3D9::TextureManager::loadQueuedTextures (void)
{
  updateQueueOSD ();

  int loads = 0;

  ///std::vector <TexLoadRequest *> finished_resamples;
  std::vector <TexLoadRequest *> finished_streams;

  stream_pool.getFinished (finished_streams);

///  if (resample_pool != nullptr)
///    resample_pool->getFinished (finished_resamples);

#if 0
  for ( auto it : finished_resamples )
  {
    TexLoadRequest* load =
      it;

    QueryPerformanceCounter_Original (&load->end);

    if (true)
    {
      tex_log->Log ( L"[%s] Finished %s texture %08x (%5.2f MiB in %9.4f ms)",
                       (load->type == TexLoadRequest::Stream) ? L"Inject Tex" :
                         (load->type == TexLoadRequest::Immediate) ? L"Inject Tex" :
                                                                     L" Resample ",
                       (load->type == TexLoadRequest::Stream) ? L"streaming" :
                         (load->type == TexLoadRequest::Immediate) ? L"loading" :
                                                                     L"filtering",
                         load->checksum,
                           (double)load->SrcDataSize / (1024.0f * 1024.0f),
                             1000.0f * (double)(load->end.QuadPart - load->start.QuadPart) /
                                       (double)SK_GetPerfFreq ().QuadPart );
    }

    Texture* pTex =
      getTexture (load->checksum);

    if (pTex != nullptr)
    {
      pTex->load_time = (float)(1000.0 * (double)(load->end.QuadPart - load->start.QuadPart) /
                                          (double)SK_GetPerfFreq ().QuadPart);
    }

    auto* pSKTex =
      static_cast <ISKTextureD3D9 *> (load->pDest);

    if (pSKTex != nullptr)
    {
      if (pSKTex->refs == 0 && load->pSrc != nullptr)
      {
        tex_log->Log (L"[ Tex. Mgr ] >> Original texture no longer referenced, discarding new one!");
        load->pSrc->Release ();
      }

      else
      {
        QueryPerformanceCounter_Original (&pSKTex->last_used);

        pSKTex->pTexOverride  = load->pSrc;
        pSKTex->override_size = load->SrcDataSize;

        addInjected (load->SrcDataSize);
      }

      injector.finishedStreaming (load->checksum);

      updateOSD ();

      ++loads;

      // Remove the temporary reference
      load->pDest->Release ();
    }

    delete load;
  }
#endif


  // Really extensive texture mods might flood us with textures, so
  //   only let a few through each frame and defer the rest.
  int max_per_frame = 20;
  int loaded        = 0;

  static int frame  = 0;

  frame++;

  for ( auto& it : finished_streams )
  {
    if ((! __need_purge) && (loaded > max_per_frame))// || (frame % 3) != 0))
    {
      it->pDest->AddRef ();
      stream_pool.sm_tex->postFinished (it);
      continue;
    }

    TexLoadRequest* load =
      it;

    load->end = SK_QueryPerf ();

    if (true)
    {
      tex_log->Log ( L"[%s] Finished %s texture %08x (%5.2f MiB in %9.4f ms)",
                       (load->type == TexLoadRequest::Stream) ? L"Inject Tex" :
                         (load->type == TexLoadRequest::Immediate) ? L"Inject Tex" :
                                                                     L" Resample ",
                       (load->type == TexLoadRequest::Stream) ? L"streaming" :
                         (load->type == TexLoadRequest::Immediate) ? L"loading" :
                                                                     L"filtering",
                         load->checksum,
                           (double)load->SrcDataSize / (1024.0 * 1024.0),
                             1000.0 * (double)(load->end.QuadPart - load->start.QuadPart) /
                                      (double)SK_PerfFreq );
    }

    Texture* pTex =
      getTexture (load->checksum);

    if (pTex != nullptr)
    {
      pTex->load_time = (float)(1000.0 * (double)(load->end.QuadPart - load->start.QuadPart) /
                                         (double)SK_PerfFreq);

      auto* pSKTex =
        static_cast <ISKTextureD3D9 *> (load->pDest);

      if (pSKTex != nullptr)
      {
        if (pSKTex->refs == 0 && load->pSrc != nullptr)
        {
          tex_log->Log (L"[ Tex. Mgr ] >> Original texture no longer referenced, discarding new one!");
          load->pSrc->Release ();
        }

        else
        {
          addInjected (pSKTex, load->pSrc, load->SrcDataSize, pTex->load_time);
        }

        injector.finishedStreaming (load->checksum);

        updateOSD ();

        ++loads;

        // Remove the temporary reference
        load->pDest->Release ();

        ++loaded;
      }
    }

    ////delete load;
  }

  //
  // If the size changes, check to see if we need a purge - if so, schedule one.
  //
  static int64_t last_size = 0LL;

  if (last_size != cacheSizeTotal ())
  {
    last_size = std::max (0LL, cacheSizeTotal ());

    if ( last_size >
           (1024LL * 1024LL) * (int64_t)config.textures.cache.max_size )
      __need_purge = true;
  }

  if ( (! ReadAcquire (&injector.streaming))  &&
       (! ReadAcquire (&injector.resampling)) &&
       (! injector.hasPendingLoads ()) )
  {
    if (__need_purge)
    {
      __need_purge = false;
      purge ();
    }
  }

  osdStats ();

  return loads;
}

#include <set>

uint32_t
__cdecl
safe_crc32c (uint32_t seed, _Notnull_ const void *buf, size_t size)
{
  // Current limit == 2 GiB
  if (size > (1024ULL * 1024ULL * 1024ULL * 2) || buf == nullptr)
    return seed;

  uint32_t ret = 0x0;

  auto orig_se =
    SK_SEH_ApplyTranslator (
      SK_FilteringStructuredExceptionTranslator (
                            EXCEPTION_ACCESS_VIOLATION
                           )                    );
  try
  {
    ret =
      crc32c (seed, buf, size);
  }

  catch (const SK_SEH_IgnoredException &e)
  {
    SK_ImGui_Warning
    ( SK_FormatStringW
      ( L"Access Violation Exception During safe_crc32c: "
        L"%hs", e.what () )
                 .c_str()
    );
  }

  SK_SEH_RemoveTranslator (
    orig_se
  );

  return
    ret;
}

SK_LazyGlobal <std::set <uint32_t>> resample_blacklist;
bool                                resample_blacklist_init = false;

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3DXCreateTextureFromFileInMemoryEx_Detour (
  _In_    LPDIRECT3DDEVICE9  pDevice,
  _In_    LPCVOID            pSrcData,
  _In_    UINT               SrcDataSize,
  _In_    UINT               Width,
  _In_    UINT               Height,
  _In_    UINT               MipLevels,
  _In_    DWORD              Usage,
  _In_    D3DFORMAT          Format,
  _In_    D3DPOOL            Pool,
  _In_    DWORD              Filter,
  _In_    DWORD              MipFilter,
  _In_    D3DCOLOR           ColorKey,
  _Inout_ D3DXIMAGE_INFO     *pSrcInfo,
  _Out_   PALETTEENTRY       *pPalette,
  _Out_   LPDIRECT3DTEXTURE9 *ppTexture
)
{
  SK_ComQIPtr <IDirect3DDevice9Ex>
       pDev9Ex (pDevice);
  if ( pDev9Ex.p != nullptr &&
            Pool == D3DPOOL_MANAGED )
  {
    Pool  =  D3DPOOL_DEFAULT;
    Usage = D3DUSAGE_DYNAMIC;
  }

  SK::D3D9::TextureManager& tex_mgr =
    SK_D3D9_GetTextureManager ();

  // Injection would recurse slightly and cause impossible to diagnose reference counting problems
  //   with texture caching if we did not check for this!
  if (tex_mgr.injector.isInjectionThread ())
  {
    return
      SK_D3DXCreateTextureFromFileInMemoryEx (
        pDevice,
          pSrcData, SrcDataSize,
            Width, Height, MipLevels,
              Usage,
                Format,
                  Pool,
                    Filter, MipFilter, ColorKey,
                      pSrcInfo, pPalette,
                        ppTexture
      );
  }

  resample_blacklist_init = true;

  // Performance statistics for caching system
  LARGE_INTEGER start, end;

  start = SK_QueryPerf ();

  uint32_t checksum = 0xdeadbeef;

  // Don't dump or cache these

  if ( (Usage & D3DUSAGE_RENDERTARGET) ||
                   pSrcData == nullptr || SrcDataSize == 0 )
    checksum = 0x00;
  else
    checksum = safe_crc32c (0, pSrcData, SrcDataSize);

  if (config.textures.d3d11.cache && checksum != 0x00)
  {
    Texture* pTex =
      tex_mgr.getTexture (checksum);

    if (pTex != nullptr)
    {
      tex_mgr.refTexture (pTex);

      *ppTexture = pTex->d3d9_tex;

      return S_OK;
    }

    tex_mgr.missTexture ();
  }

  bool resample = false;

  // Necessary to make D3DX texture write functions work
  if ( Pool == D3DPOOL_DEFAULT && ( config.textures.dump_on_load &&
        (! tex_mgr.isTextureDumped     (checksum))         &&
        (! tex_mgr.isTextureInjectable (checksum)) ) )//|| (
                                    //config.textures.d3d11.cache /*||
                                    //config.textures.on_demand_dump*/ ) )
    Usage = D3DUSAGE_DYNAMIC;


  bool bLoading =
    tex_mgr.injector.beginLoad ();

  D3DXIMAGE_INFO info = { };
  SK_D3DXGetImageInfoFromFileInMemory (pSrcData, SrcDataSize, &info);

  tex_mgr.injector.endLoad     (bLoading);

#if 0
  D3DFORMAT fmt_real = info.Format;

  bool power_of_two_in_one_way =
    (! (info.Width  & (info.Width  - 1)))  !=  (! (info.Height & (info.Height - 1)));


  // Textures that would be incorrectly filtered if resampled
  if (power_of_two_in_one_way)
    tex_mgr.addNonPowerOfTwoTexture (checksum);


  // Generate complete mipmap chains for best image quality
  //  (will increase load-time on uncached textures)
  if ((Pool == D3DPOOL_DEFAULT) && false)//config.textures.remaster)
  {
    {
      bool power_of_two_in =
        (! (info.Width  & (info.Width  - 1)))  &&  (! (info.Height & (info.Height - 1)));

      bool power_of_two_out =
        (! (Width  & (Width  - 1)))            &&  (! (Height & (Height - 1)));

      if (power_of_two_in && power_of_two_out)
      {
        if (true)//info.MipLevels > 1/* || config.textures.uncompressed*/)
        {
          if ( resample_blacklist->count (checksum) == 0 )
            resample = true;
        }
      }
    }
  }
#endif

  HRESULT         hr      = E_FAIL;
  TexLoadRequest* load_op = nullptr;

  bool remap_stream =
    tex_mgr.injector.isStreaming (checksum);

  //
  // Generic injectable textures
  //
  if ( tex_mgr.isTextureInjectable (checksum) )
  {
    tex_log->LogEx ( true, L"[Inject Tex] Injectable texture for checksum (%08x)... ",
                       checksum );

    wchar_t wszInjectFileName [MAX_PATH + 2] = { };

    TexRecord& record =
      tex_mgr.getInjectableTexture (checksum);

    if (record.method == TexLoadMethod::DontCare)
      record.method = TexLoadMethod::Streaming;

    // If -1, load from disk...
    if (record.archive == -1)
    {
      if (record.method == TexLoadMethod::Streaming)
      {
        swprintf ( wszInjectFileName, L"%s\\inject\\textures\\streaming\\%08x%s",
                     SK_D3D11_res_root->c_str (),
                       checksum,
                         L".dds" );
      }

      else if (record.method == TexLoadMethod::Blocking)
      {
        swprintf ( wszInjectFileName, L"%s\\inject\\textures\\blocking\\%08x%s",
                     SK_D3D11_res_root->c_str (),
                       checksum,
                         L".dds" );
      }
    }

    load_op           = new TexLoadRequest ();

    load_op->pDevice  = pDevice;
    load_op->checksum = checksum;
    load_op->mem_pool = Pool;

    if (record.method == TexLoadMethod::Streaming)
      load_op->type   = TexLoadRequest::Stream;
    else
      load_op->type   = TexLoadRequest::Immediate;

    wcscpy (load_op->wszFilename, wszInjectFileName);

    if (load_op->type == TexLoadRequest::Stream)
    {
      if ((! remap_stream))
        tex_log->LogEx ( false, L"streaming\n" );
      else
        tex_log->LogEx ( false, L"in-flight already\n" );
    }

    else
    {
      tex_log->LogEx ( false, L"blocking (deferred)\n" );
    }
  }

  bool will_replace = (load_op != nullptr || resample);

  bLoading =
    tex_mgr.injector.beginLoad ();

  hr =
    SK_D3DXCreateTextureFromFileInMemoryEx ( pDevice,
                                               pSrcData,         SrcDataSize,
                                                 Width,          Height,    will_replace ? 1 : MipLevels,
                                                   Usage,        Format,    Pool,
                                                     Filter,     MipFilter, ColorKey,
                                                       pSrcInfo, pPalette,
                                                         ppTexture );

  tex_mgr.injector.endLoad (bLoading);


  if (SUCCEEDED (hr))
  {
    new ISKTextureD3D9 (ppTexture, SrcDataSize, checksum);

    if ( load_op != nullptr && ( load_op->type == TexLoadRequest::Stream ||
                                 load_op->type == TexLoadRequest::Immediate ) )
    {
      load_op->SrcDataSize =
        static_cast <UINT> (
          tex_mgr.isTextureInjectable (checksum)         ?
            tex_mgr.getInjectableTexture (checksum).size : 0
        );

      load_op->pDest =
        *ppTexture;

      if (load_op->type == TexLoadRequest::Immediate)
        ((ISKTextureD3D9 *)*ppTexture)->must_block = true;

      if (tex_mgr.injector.isStreaming (load_op->checksum))
      {
        auto* pTexOrig =
          static_cast <ISKTextureD3D9 *> (
            tex_mgr.injector.getTextureInFlight (load_op->checksum)->pDest
          );

        //tex_mgr.injector.unlockStreaming ();
        //tex_mgr.injector.lockStreaming   ();

        // Remap the output of the in-flight texture
        tex_mgr.injector.getTextureInFlight (load_op->checksum)->pDest =
          *ppTexture;

        Texture* pTex =
          tex_mgr.getTexture (load_op->checksum);

        if (pTex != nullptr)
        {
          for ( int i = 0;
                    i < pTex->refs;
                  ++i )
          {
            (*ppTexture)->AddRef ();
          }
        }

        tex_mgr.removeTexture (pTexOrig);
      }

      else
      {
        tex_mgr.injector.addTextureInFlight (load_op);
        (*ppTexture)->AddRef ();
        stream_pool.postJob                 (load_op);
        //resample_pool->postJob (load_op);
      }
    }

#if 0
    //
    // TODO:  Actually stream these, but block if the game tries to call SetTexture (...)
    //          while the texture is in-flight.
    //
    else if (load_op != nullptr && load_op->type == tsf_tex_load_s::Immediate) {
      QueryPerformanceFrequency        (&load_op->freq);
      QueryPerformanceCounter_Original (&load_op->start);

      EnterCriticalSection (&cs_tex_inject);
      inject_tids.insert   (GetCurrentThreadId ());
      LeaveCriticalSection (&cs_tex_inject);

      load_op->pDest = *ppTexture;

      hr = InjectTexture (load_op);

      EnterCriticalSection (&cs_tex_inject);
      inject_tids.erase    (GetCurrentThreadId ());
      LeaveCriticalSection (&cs_tex_inject);

      QueryPerformanceCounter_Original (&load_op->end);

      if (SUCCEEDED (hr)) {
        tex_log->Log ( L"[Inject Tex] Finished synchronous texture %08x (%5.2f MiB in %9.4f ms)",
                        load_op->checksum,
                          (double)load_op->SrcDataSize / (1024.0f * 1024.0f),
                            1000.0f * (double)(load_op->end.QuadPart - load_op->start.QuadPart) /
                                      (double) load_op->freq.QuadPart );
        ISKTextureD3D9* pSKTex =
          (ISKTextureD3D9 *)*ppTexture;

        pSKTex->pTexOverride  = load_op->pSrc;
        pSKTex->override_size = load_op->SrcDataSize;

        pSKTex->last_used     = load_op->end;

        tsf::RenderFix::tex_mgr.addInjected (load_op->SrcDataSize);
      } else {
        tex_log->Log ( L"[Inject Tex] *** FAILED synchronous texture %08x",
                        load_op->checksum );
      }

      delete load_op;
      load_op = nullptr;
    }
#endif

    else if (resample)
    {
      load_op              = new TexLoadRequest ();

      load_op->pDevice     = pDevice;
      load_op->checksum    = checksum;
      load_op->type        = TexLoadRequest::Resample;

      load_op->pSrcData    = new (std::nothrow) uint8_t [SrcDataSize];
      load_op->SrcDataSize = SrcDataSize;

      swprintf (load_op->wszFilename, L"Resample_%x.dds", checksum);

      memcpy (load_op->pSrcData, pSrcData, SrcDataSize);

      (*ppTexture)->AddRef ();
      load_op->pDest       = *ppTexture;

      ///resample_pool->postJob (load_op);
    }

    else
      (*ppTexture)->AddRef ();
  }

  else if (load_op != nullptr)
  {
    delete load_op;
    load_op = nullptr;
  }

  end = SK_QueryPerf ();

  if (SUCCEEDED (hr))
  {
    if (/*config.textures.cache &&*/ checksum != 0x00 && (! tex_mgr.getTexture (checksum)))
    {
      auto* pTex =
        new Texture ();

      pTex->crc32c = checksum;

      pTex->d3d9_tex = *(ISKTextureD3D9 **)ppTexture;
      pTex->refs++;

      pTex->original_pool = Pool;

      pTex->load_time = (float)( 1000.0 *
                          (double)(end.QuadPart - start.QuadPart) /
                          (double)SK_PerfFreq );

      tex_mgr.addTexture (checksum, pTex, SrcDataSize);
    }

    if (true)
    {//config.textures.log) {
      //tex_log->Log ( L"[Load Trace] Texture:   (%lu x %lu) * <LODs: %lu> - FAST_CRC32: %X",
      //                info.Width, info.Height, (*ppTexture)->GetLevelCount (), checksum );
      //tex_log->Log ( L"[Load Trace]              Usage: %-20hs - Format: %-20hs",
      //                SK_D3D9_UsageToStr    (Usage).c_str (),
      //                  SK_D3D9_FormatToStr (Format).c_str () );
      //tex_log->Log ( L"[Load Trace]                Pool: %hs",
      //                SK_D3D9_PoolToStr (Pool) );
      //tex_log->Log ( L"[Load Trace]      Load Time: %6.4f ms",
      //                1000.0f * (double)(end.QuadPart - start.QuadPart) / (double)SK_GetPerfFreq ().QuadPart );
    }
  }

  bool dump = config.textures.dump_on_load;

  if ( dump && (! tex_mgr.isTextureInjectable (checksum)) &&
               (! tex_mgr.isTextureDumped     (checksum)) )
  {
    D3DXIMAGE_INFO info_ = { };

    bLoading =
      tex_mgr.injector.beginLoad ();

    SK_D3DXGetImageInfoFromFileInMemory (pSrcData, SrcDataSize, &info_);

    tex_mgr.dumpTexture (info_.Format, checksum, *ppTexture);

    tex_mgr.injector.endLoad     (bLoading);
  }

  return hr;
}

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3DXCreateTextureFromFileExA_Detour (
  _In_    LPDIRECT3DDEVICE9  pDevice,
  _In_    LPCSTR             pSrcFile,
  _In_    UINT               Width,
  _In_    UINT               Height,
  _In_    UINT               MipLevels,
  _In_    DWORD              Usage,
  _In_    D3DFORMAT          Format,
  _In_    D3DPOOL            Pool,
  _In_    DWORD              Filter,
  _In_    DWORD              MipFilter,
  _In_    D3DCOLOR           ColorKey,
  _Inout_ D3DXIMAGE_INFO     *pSrcInfo,
  _Out_   PALETTEENTRY       *pPalette,
  _Out_   LPDIRECT3DTEXTURE9 *ppTexture
)
{
  SK_ComQIPtr <IDirect3DDevice9Ex>
       pDev9Ex (pDevice);
  if ( pDev9Ex.p != nullptr &&
            Pool == D3DPOOL_MANAGED )
  {
    Pool   =  D3DPOOL_DEFAULT;
    Usage |= D3DUSAGE_DYNAMIC;
  }

//Width     = D3DX_DEFAULT;
//Height    = D3DX_DEFAULT;
//MipLevels = D3DX_DEFAULT;
//Usage     = 0x0;
//Format    = D3DFMT_UNKNOWN;
//Pool      = D3DPOOL_DEFAULT;
//Filter    = D3DX_DEFAULT;
//MipFilter = D3DX_DEFAULT;
//ColorKey  = 0x0;

  return
    D3DXCreateTextureFromFileExA_Original (
      pDevice, pSrcFile,
        Width, Height, MipLevels,
          Usage,
            Format,
              Pool,
                Filter, MipFilter, ColorKey,
                  pSrcInfo, pPalette,
                    ppTexture
    );
}

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3DXCreateTextureFromFileExW_Detour (
  _In_    LPDIRECT3DDEVICE9  pDevice,
  _In_    LPCWSTR            pSrcFile,
  _In_    UINT               Width,
  _In_    UINT               Height,
  _In_    UINT               MipLevels,
  _In_    DWORD              Usage,
  _In_    D3DFORMAT          Format,
  _In_    D3DPOOL            Pool,
  _In_    DWORD              Filter,
  _In_    DWORD              MipFilter,
  _In_    D3DCOLOR           ColorKey,
  _Inout_ D3DXIMAGE_INFO     *pSrcInfo,
  _Out_   PALETTEENTRY       *pPalette,
  _Out_   LPDIRECT3DTEXTURE9 *ppTexture
)
{
  SK_ComQIPtr <IDirect3DDevice9Ex>
       pDev9Ex (pDevice);
  if ( pDev9Ex.p != nullptr &&
            Pool == D3DPOOL_MANAGED )
  {
    Pool   =  D3DPOOL_DEFAULT;
    Usage |= D3DUSAGE_DYNAMIC;
  }

//Width     = D3DX_DEFAULT;
//Height    = D3DX_DEFAULT;
//MipLevels = D3DX_DEFAULT;
//Usage     = 0x0;
//Format    = D3DFMT_UNKNOWN;
//Pool      = D3DPOOL_DEFAULT;
//Filter    = D3DX_DEFAULT;
//MipFilter = D3DX_DEFAULT;
//ColorKey  = 0x0;

  return
    D3DXCreateTextureFromFileExW_Original (
      pDevice, pSrcFile,
        Width, Height, MipLevels,
          Usage,
            Format,
              Pool,
                Filter, MipFilter, ColorKey,
                  pSrcInfo, pPalette,
                    ppTexture
    );
}

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3DXCreateVolumeTextureFromFileInMemoryEx_Detour (
  _In_    LPDIRECT3DDEVICE9        pDevice,
  _In_    LPCVOID                  pSrcData,
  _In_    UINT                     SrcDataSize,
  _In_    UINT                     Width,
  _In_    UINT                     Height,
  _In_    UINT                     Depth,
  _In_    UINT                     MipLevels,
  _In_    DWORD                    Usage,
  _In_    D3DFORMAT                Format,
  _In_    D3DPOOL                  Pool,
  _In_    DWORD                    Filter,
  _In_    DWORD                    MipFilter,
  _In_    D3DCOLOR                 ColorKey,
  _Inout_ D3DXIMAGE_INFO           *pSrcInfo,
  _Out_   PALETTEENTRY             *pPalette,
  _Out_   LPDIRECT3DVOLUMETEXTURE9 *ppVolumeTexture
)
{
  SK_ComQIPtr <IDirect3DDevice9Ex>
       pDev9Ex (pDevice);
  if ( pDev9Ex.p != nullptr &&
            Pool == D3DPOOL_MANAGED )
  {
    Pool  =  D3DPOOL_DEFAULT;
    Usage = D3DUSAGE_DYNAMIC;
  }

//Width     = D3DX_DEFAULT;
//Height    = D3DX_DEFAULT;
//Depth     = D3DX_DEFAULT;
//MipLevels = D3DX_DEFAULT;
//Usage     = 0x0;
//Format    = D3DFMT_UNKNOWN;
//Pool      = D3DPOOL_DEFAULT;
//Filter    = D3DX_DEFAULT;
//MipFilter = D3DX_DEFAULT;
//ColorKey  = 0x0;

  return
    D3DXCreateVolumeTextureFromFileInMemoryEx_Original (
      pDevice, pSrcData, SrcDataSize,
        Width, Height, Depth, MipLevels,
          Usage, Format, Pool,
            Filter, MipFilter, ColorKey,
              pSrcInfo, pPalette, ppVolumeTexture
    );
}

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3DXCreateCubeTextureFromFileInMemoryEx_Detour (
  _In_    LPDIRECT3DDEVICE9      pDevice,
  _In_    LPCVOID                pSrcData,
  _In_    UINT                   SrcDataSize,
  _In_    UINT                   Size,
  _In_    UINT                   MipLevels,
  _In_    DWORD                  Usage,
  _In_    D3DFORMAT              Format,
  _In_    D3DPOOL                Pool,
  _In_    DWORD                  Filter,
  _In_    DWORD                  MipFilter,
  _In_    D3DCOLOR               ColorKey,
  _Inout_ D3DXIMAGE_INFO         *pSrcInfo,
  _Out_   PALETTEENTRY           *pPalette,
  _Out_   LPDIRECT3DCUBETEXTURE9 *ppCubeTexture
)
{
  SK_ComQIPtr <IDirect3DDevice9Ex>
       pDev9Ex (pDevice);
  if ( pDev9Ex.p != nullptr &&
            Pool == D3DPOOL_MANAGED )
  {
    Pool  =  D3DPOOL_DEFAULT;
    Usage = D3DUSAGE_DYNAMIC;
  }

//Size      = D3DX_DEFAULT;
//MipLevels = D3DX_DEFAULT;
//Usage     = 0x0;
//Format    = D3DFMT_UNKNOWN;
//Pool      = D3DPOOL_DEFAULT;
//Filter    = D3DX_DEFAULT;
//MipFilter = D3DX_DEFAULT;
//ColorKey  = 0x0;

  return
    D3DXCreateCubeTextureFromFileInMemoryEx_Original (
      pDevice, pSrcData, SrcDataSize,
        Size, MipLevels,
          Usage, Format, Pool,
            Filter, MipFilter, ColorKey,
              pSrcInfo, pPalette, ppCubeTexture
    );
}

bool
SK::D3D9::TextureManager::deleteDumpedTexture (D3DFORMAT fmt, uint32_t checksum)
{
    wchar_t wszPath     [MAX_PATH + 2] = { };
    wchar_t wszFileName [MAX_PATH + 2] = { };

    swprintf ( wszPath, L"%s\\dump\\textures\\%s\\%hs",
                 SK_D3D11_res_root->c_str (),
                   SK_GetHostApp (),
                     SK_D3D9_FormatToStr (fmt, false).c_str () );

    swprintf ( wszFileName, L"%s\\%08x%s",
                 wszPath,
                   checksum,
                     L".dds" );

  if (GetFileAttributesW (wszFileName) != INVALID_FILE_ATTRIBUTES)
  {
    if (DeleteFileW (wszFileName))
    {
      dumped_textures [checksum] = false;
      return true;
    }
  }

  return false;
}

bool
SK::D3D9::TextureManager::isTextureDumped (uint32_t checksum)
{
  return
    ( dumped_textures [checksum] != false );
}

HRESULT
SK::D3D9::TextureManager::dumpTexture (D3DFORMAT fmt, uint32_t checksum, IDirect3DTexture9* pTex)
{
  HRESULT hr = E_FAIL;

  if (! isTextureDumped (checksum))
  {
    D3DFORMAT fmt_real = fmt;

//    bool compressed =
//      (fmt_real >= D3DFMT_DXT1 && fmt_real <= D3DFMT_DXT5);

    wchar_t wszPath     [MAX_PATH + 2] = { };
    wchar_t wszFileName [MAX_PATH + 2] = { };

    swprintf ( wszPath, L"%s\\dump\\textures\\%s\\%hs",
                 SK_D3D11_res_root->c_str (),
                   SK_GetHostApp (),
                     SK_D3D9_FormatToStr (fmt_real, false).c_str () );

    swprintf ( wszFileName, L"%s\\%08x%s",
                 wszPath,
                   checksum,
                     L".dds" );

    if (GetFileAttributesW (wszFileName) != FILE_ATTRIBUTE_DIRECTORY)
      SK_CreateDirectories (wszFileName);

    bool bLoading =
      injector.beginLoad ();

    D3DSURFACE_DESC                        surfDesc = { };
    if (SUCCEEDED (pTex->GetLevelDesc (0, &surfDesc)))
    {
      if (D3DUSAGE_RENDERTARGET == (surfDesc.Usage & D3DUSAGE_RENDERTARGET))
      {
        SK_ComPtr <IDirect3DDevice9>
                          pDev9;
        pTex->GetDevice (&pDev9.p);

        SK_ComPtr <IDirect3DTexture9> pTexCopy;
        pDev9->CreateTexture ( surfDesc.Width,
                               surfDesc.Height,
                                 pTex->GetLevelCount (), D3DUSAGE_DYNAMIC,
                                   surfDesc.Format,
                                     D3DPOOL_DEFAULT, &pTexCopy.p, nullptr );

        for ( UINT lvl = 0 ; lvl < pTex->GetLevelCount () ; ++lvl )
        {
          SK_ComPtr <IDirect3DSurface9> pSurf9_In;
          SK_ComPtr <IDirect3DSurface9> pSurf9_Out;

          pTex->GetSurfaceLevel     (lvl, &pSurf9_In.p);
          pTexCopy->GetSurfaceLevel (lvl, &pSurf9_Out.p);

          pDev9->GetRenderTargetData (pSurf9_In, pSurf9_Out);
        }

        hr =
          SK_D3DXSaveTextureToFileW (wszFileName, D3DXIFF_DDS, pTexCopy, nullptr);
      }
    }

    if (hr != S_OK)
        hr =
          SK_D3DXSaveTextureToFileW (wszFileName, D3DXIFF_DDS, pTex, nullptr);

    injector.endLoad (bLoading);

    if (SUCCEEDED (hr))
      dumped_textures [checksum] = true;
  }

  return hr;
}

SK_LazyGlobal <concurrent_unordered_map <ISKTextureD3D9 *, bool>> remove_textures;

void
SK::D3D9::TextureManager::addInjected ( ISKTextureD3D9*    pWrapperTex,
                                        IDirect3DTexture9* pOverrideTex,
                                        size_t             size,
                                        float              load_time )
{
  if (pWrapperTex == nullptr)
    return;

  pWrapperTex->last_used = SK_QueryPerf ();

  pWrapperTex->pTexOverride  = pOverrideTex;
  pWrapperTex->override_size = size;

  InterlockedIncrement (&injected_count);
  InterlockedAdd64     (&injected_size, size);

  injected_textures   [pWrapperTex->tex_crc32c] = pWrapperTex->pTexOverride;
  injected_sizes      [pWrapperTex->tex_crc32c] = pWrapperTex->override_size;
  injected_load_times [pWrapperTex->tex_crc32c] = load_time;
  injected_refs       [pWrapperTex->tex_crc32c] = 1;
}

SK::D3D9::Texture*
SK::D3D9::TextureManager::getTexture (uint32_t checksum)
{
  if (checksum == 0x00)
  {
    std::vector <ISKTextureD3D9 *> unremove_textures;

    static auto& _remove_textures = remove_textures.get ();

    auto  rem   = _remove_textures.cbegin ();
    while (rem != _remove_textures.cend   ())
    {
      if (rem->second == false)
      {
        ++rem;
        continue;
      }

      if (! rem->first->can_free)
      {
        unremove_textures.emplace_back (rem->first);
        ++rem;
        continue;
      }

      if (rem->first->pTex)
      {
        if (! rem->first->pTex->Release ())
        {
          rem->first->pTex = nullptr;

          InterlockedAdd64 (&basic_size,  -rem->first->tex_size);
          {
            textures [rem->first->tex_crc32c] = nullptr;
          }

          if (injected_refs.count (checksum) && injected_refs [checksum] > 0)
              injected_refs       [checksum]--;

          delete rem->first;
          ++rem;
          continue;
        }
      }


      //if (! injected_textures.count ((*rem)->tex_crc32c))
      //{
      //  if ((*rem)->pTexOverride != nullptr)
      //  {
      //    InterlockedDecrement (&injected_count);
      //    InterlockedAdd64     (&injected_size, -(*rem)->override_size);
      //
      //    (*rem)->pTexOverride->Release ();
      //  }
      //}

      //(*rem)->pTexOverride = nullptr;

      ++rem;
    }

    _remove_textures.clear ();

    for ( auto& it : unremove_textures )
    {
      _remove_textures.insert (
        std::make_pair (it, true)
      );
    }
  }

  const auto tex =
    textures.find (checksum);

  if (tex != textures.cend ())
  {
    return tex->second;
  }

  return nullptr;
}

void
SK::D3D9::TextureManager::removeTexture (ISKTextureD3D9* pTexD3D9)
{
  SK_ReleaseAssert (pTexD3D9 != nullptr);

  remove_textures->insert (
    std::make_pair (pTexD3D9, true)
  );

  updateOSD ();
}

void
SK::D3D9::TextureManager::addTexture (uint32_t checksum, Texture* pTex, size_t size)
{
  // D3D9Ex override hooks a few D3DX functions, which might
  //   call into this while it is not initialized
  if (! init)
    return;

  SK_ReleaseAssert (pTex != nullptr);

  pTex->size = size;

  {
    EnterCriticalSection (&cs_cache);

    if ( textures.count (checksum) &&
         textures       [checksum] != nullptr )
    {
      if (textures [checksum] != pTex)
      {
        if (config.system.log_level > 1)
        {
          tex_log->Log (
            L"[Cached Tex] Removing existing texture with checksum %x"
            L" on call to addTexture (...)", checksum );
        }

        //removeTexture (textures [checksum]->d3d9_tex);
                       textures [checksum]->d3d9_tex->Release ();
                       getTexture (0);
      }
    }

    else
      InterlockedAdd64 (&basic_size, pTex->size);

    textures [checksum] = pTex;

    LeaveCriticalSection (&cs_cache);
  }

  updateOSD ();
}

void
SK::D3D9::TextureManager::refTexture (Texture* pTex)
{
  refTextureEx (pTex, true);
}

void
SK::D3D9::TextureManager::refTextureEx (Texture* pTex, bool add_to_ref_count)
{
  SK_ReleaseAssert (pTex != nullptr);

  if (add_to_ref_count)
  {
    pTex->d3d9_tex->AddRef ();
    pTex->refs++;
  }

  if ( injected_textures.count (pTex->crc32c) &&
       injected_textures       [pTex->crc32c] != nullptr )
       injected_refs           [pTex->crc32c]++;

//  pTex->d3d9_tex->can_free = false;

  InterlockedIncrement (&hits);

#if 0
  if (true)
  {//config.textures.log) {
    tex_log->Log ( L"[CacheTrace] Cache hit (%X), saved %2.1f ms",
                     pTex->crc32c,
                       pTex->load_time );
  }
#endif

  InterlockedAdd64 ( &bytes_saved,
                       std::max ( pTex->size, static_cast <size_t> (
                                                pTex->d3d9_tex->override_size )
                                )
                    );
                    time_saved += pTex->load_time;

  updateOSD ();
}

COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9SetRenderTarget_Detour (
                  _In_ IDirect3DDevice9  *This,
                  _In_ DWORD              RenderTargetIndex,
                  _In_ IDirect3DSurface9 *pRenderTarget
)
{
  static int draw_counter = 0;

  // Ignore anything that's not the primary render device.
  //if (! SK_GetCurrentRenderBackend ().device.IsEqualObject (This))
  //{
  //  return D3D9SetRenderTarget_Original (This, RenderTargetIndex, pRenderTarget);
  //}

  //if (tsf::RenderFix::tracer.log) {
#ifdef DUMP_RT
    if (D3DXSaveSurfaceToFileW == nullptr) {
      D3DXSaveSurfaceToFileW =
        (D3DXSaveSurfaceToFile_pfn)
       SK_GetProcAddress ( tsf::RenderFix::d3dx9_43_dll,
                             "D3DXSaveSurfaceToFileW" );
    }

    wchar_t wszDumpName [MAX_PATH + 2] = { };

    if (pRenderTarget != pOld) {
      if (pOld != nullptr) {
        swprintf (wszDumpName, L"dump\\%03d_out_%p.png", draw_counter, pOld);

        dll_log->Log ( L"[FrameTrace] >>> Dumped: Output RT to %s >>>", wszDumpName );

        dumping = true;
        //D3DXSaveSurfaceToFile (wszDumpName, D3DXIFF_PNG, pOld, nullptr, nullptr);
      }
    }
#endif

    //dll_log->Log ( L"[FrameTrace] SetRenderTarget - RenderTargetIndex: %lu, pRenderTarget: %ph",
                    //RenderTargetIndex, pRenderTarget );

#ifdef DUMP_RT
    if (pRenderTarget != pOld) {
      pOld = pRenderTarget;

      swprintf (wszDumpName, L"dump\\%03d_in_%p.png", ++draw_counter, pRenderTarget);

      dll_log->Log ( L"[FrameTrace] <<< Dumped: Input RT to  %s  <<<", wszDumpName );

      dumping = true;
      //D3DXSaveSurfaceToFile (wszDumpName, D3DXIFF_PNG, pRenderTarget, nullptr, nullptr);
    }
#endif
  //}

  return D3D9SetRenderTarget_Original (This, RenderTargetIndex, pRenderTarget);
}

void
SK::D3D9::TextureManager::Init (void)
{
  //textures.reserve                    (4096);
  //textures_used.reserve               (2048);
  textures_last_frame.reserve         (1024);
//non_power_of_two_textures.reserve   (512);
  tracked_ps->used_textures.reserve    (512);
  tracked_vs->used_textures.reserve    (512);
  //known.render_targets.reserve        (128);
  //used.render_targets.reserve         (64);
  //injector.textures_in_flight.reserve (32);
  tracked_rt->pixel_shaders.reserve    (32);
  tracked_rt->vertex_shaders.reserve   (32);

  thread_id = SK_Thread_GetCurrentId ();

  injector.init ();

  InitializeCriticalSectionAndSpinCount (&cs_cache,        10240UL);
  InitializeCriticalSectionAndSpinCount (&cs_free_list,     6144UL);
  InitializeCriticalSectionAndSpinCount (&cs_unreferenced, 16384UL);

  InitializeCriticalSectionAndSpinCount (&osd_cs,   32UL);

  void WINAPI SK_D3D11_SetResourceRoot      (const wchar_t* root);
  SK_D3D11_SetResourceRoot (config.textures.d3d11.res_root.c_str ());

  // Create the directory to store dumped textures
  if (config.textures.d3d11.dump)
    CreateDirectoryW (SK_D3D11_res_root->c_str (), nullptr);

  tex_log->init (L"logs/textures.log", L"wt+,ccs=UTF-8");

  init = true;

  refreshDataSources ();

  wchar_t    wszDumpBase [MAX_PATH + 2] = { };
  swprintf ( wszDumpBase,
               L"%s\\dump\\textures\\%s\\",
                 SK_D3D11_res_root->c_str (), SK_GetHostApp () );

  if ( GetFileAttributesW (wszDumpBase) != INVALID_FILE_ATTRIBUTES )
  {
    WIN32_FIND_DATA fd;
    WIN32_FIND_DATA fd_sub;
    int             files    = 0;
    LARGE_INTEGER   liSize   = { 0 };
    HANDLE          hSubFind = INVALID_HANDLE_VALUE;
    HANDLE          hFind =
                  FindFirstFileW (
                    ( std::wstring (wszDumpBase) +
                                        LR"(\*)" ).c_str (),
                        &fd      );

    tex_log->LogEx ( true,
                       L"[ Dump Tex ] Enumerating dumped textures..." );

    if (hFind != INVALID_HANDLE_VALUE)
    {
      do
      {
        if (fd.dwFileAttributes != INVALID_FILE_ATTRIBUTES)
        {
          wchar_t    wszSubDir [MAX_PATH + 2] = { };
          swprintf ( wszSubDir,
                       L"%s\\%s\\*",
                         wszDumpBase, fd.cFileName );

          hSubFind =
            FindFirstFileW (wszSubDir, &fd_sub);

          if (hSubFind != INVALID_HANDLE_VALUE)
          {
            do
            {
              if (wcsstr (_wcslwr (fd_sub.cFileName), L".dds"))
              {
                uint32_t checksum = 0x00;
                swscanf (fd_sub.cFileName, L"%08x.dds", &checksum);

                ++files;

                LARGE_INTEGER fsize;

                fsize.HighPart = fd_sub.nFileSizeHigh;
                fsize.LowPart  = fd_sub.nFileSizeLow;

                liSize.QuadPart += fsize.QuadPart;

                dumped_textures [checksum] = true;
              }
            } while (FindNextFileW (hSubFind, &fd_sub) != 0);

            FindClose (hSubFind);
          }
        }
      } while (FindNextFileW (hFind, &fd) != 0);

      FindClose (hFind);
    }

    tex_log->LogEx ( false, L" %li files (%3.1f MiB)\n",
                       files, (double)liSize.QuadPart / (1024.0 * 1024.0) );
  }

  InterlockedExchange64 (&bytes_saved, 0LL);

  time_saved  = 0.0f;

  decomp_semaphore =
    CreateSemaphore ( nullptr,
                        5,//config.textures.worker_threads,
                          5,//config.textures.worker_threads,
                            nullptr );

  stream_pool.lrg_tex = std::make_unique <TextureThreadPool> ();
  stream_pool.sm_tex  = std::make_unique <TextureThreadPool> ();

  SK_ICommandProcessor* pCommandProc = nullptr;

  SK_RunOnce (
    pCommandProc =
      SK_Render_InitializeSharedCVars ()
  );

  // TODO: These kinda overlap with D3D11 variables of the same name,
  //         could (... will?) make for an awkward bug years from now.
  //
  if (pCommandProc != nullptr)
  {
    pCommandProc->AddVariable (
      "Textures.Remap",
        SK_CreateVar (SK_IVariable::Boolean, &__remap_textures)
    );
    pCommandProc->AddVariable (
      "Textures.Purge",
        SK_CreateVar (SK_IVariable::Boolean, &__need_purge)
    );
    pCommandProc->AddVariable (
      "Textures.Trace",
        SK_CreateVar (SK_IVariable::Boolean, &__log_used)
    );
    pCommandProc->AddVariable (
      "Textures.ShowCache",
        SK_CreateVar (SK_IVariable::Boolean, &__show_cache)
    );
    pCommandProc->AddVariable (
      "Textures.MaxCacheSize",
        SK_CreateVar (SK_IVariable::Int,     &config.textures.cache.max_size)
    );
  }
}

#include <utility.h>

extern
COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9SetTexture_Override (
  _In_ IDirect3DDevice9      *This,
  _In_ DWORD                  Sampler,
  _In_ IDirect3DBaseTexture9 *pTexture );

extern
COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9SetRenderTarget_Override (
  _In_ IDirect3DDevice9  *This,
  _In_ DWORD              RenderTargetIndex,
  _In_ IDirect3DSurface9 *pRenderTarget );

extern
COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9CreateTexture_Override (IDirect3DDevice9   *This,
                            UINT                Width,
                            UINT                Height,
                            UINT                Levels,
                            DWORD               Usage,
                            D3DFORMAT           Format,
                            D3DPOOL             Pool,
                            IDirect3DTexture9 **ppTexture,
                            HANDLE             *pSharedHandle);

extern
COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9CreateDepthStencilSurface_Override (IDirect3DDevice9     *This,
                                        UINT                  Width,
                                        UINT                  Height,
                                        D3DFORMAT             Format,
                                        D3DMULTISAMPLE_TYPE   MultiSample,
                                        DWORD                 MultisampleQuality,
                                        BOOL                  Discard,
                                        IDirect3DSurface9   **ppSurface,
                                        HANDLE               *pSharedHandle);

extern
COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9CreateRenderTarget_Override (IDirect3DDevice9     *This,
                                 UINT                  Width,
                                 UINT                  Height,
                                 D3DFORMAT             Format,
                                 D3DMULTISAMPLE_TYPE   MultiSample,
                                 DWORD                 MultisampleQuality,
                                 BOOL                  Lockable,
                                 IDirect3DSurface9   **ppSurface,
                                 HANDLE               *pSharedHandle);

extern
COM_DECLSPEC_NOTHROW
HRESULT
STDMETHODCALLTYPE
D3D9SetDepthStencilSurface_Override (
  _In_ IDirect3DDevice9  *This,
  _In_ IDirect3DSurface9 *pNewZStencil
);

void
SK::D3D9::TextureManager::Hook (void)
{
  SK_CreateDLLHook2 (      SK_GetModuleFullName (SK_GetDLL ()).c_str (),
                            "D3D9BeginScene_Override",
                             D3D9BeginScene_Detour,
    static_cast_p2p <void> (&D3D9BeginScene_Original) );

  SK_CreateDLLHook2 (      SK_GetModuleFullName (SK_GetDLL ()).c_str (),
                            "D3D9StretchRect_Override",
                             D3D9StretchRect_Detour,
    static_cast_p2p <void> (&D3D9StretchRect_Original) );


  SK_CreateFuncHook (      L"D3D9CreateRenderTarget_Override",
                            &D3D9CreateRenderTarget_Override,
                            &D3D9CreateRenderTarget_Detour,
    static_cast_p2p <void> (&D3D9CreateRenderTarget_Original) );
  MH_QueueEnableHook (       D3D9CreateRenderTarget_Override );

  SK_CreateFuncHook (      L"D3D9CreateDepthStencilSurface_Override",
                            &D3D9CreateDepthStencilSurface_Override,
                            &D3D9CreateDepthStencilSurface_Detour,
    static_cast_p2p <void> (&D3D9CreateDepthStencilSurface_Original) );
  MH_QueueEnableHook (       D3D9CreateDepthStencilSurface_Override );

  SK_CreateFuncHook (      L"D3D9CreateTexture_Override",
                            &D3D9CreateTexture_Override,
                            &D3D9CreateTexture_Detour,
    static_cast_p2p <void> (&D3D9CreateTexture_Original) );
  MH_QueueEnableHook (       D3D9CreateTexture_Override );

  SK_CreateFuncHook (      L"D3D9SetTexture_Override",
                            &D3D9SetTexture_Override,
                            &D3D9SetTexture_Detour,
    static_cast_p2p <void> (&D3D9SetTexture_Original) );
  MH_QueueEnableHook (       D3D9SetTexture_Override );

  SK_CreateFuncHook (      L"D3D9SetRenderTarget_Override",
                            &D3D9SetRenderTarget_Override,
                            &D3D9SetRenderTarget_Detour,
    static_cast_p2p <void> (&D3D9SetRenderTarget_Original) );
  MH_QueueEnableHook (       D3D9SetRenderTarget_Override );

  //SK_CreateFuncHook ( L"D3D9SetDepthStencilSurface_Override",
  //                     &D3D9SetDepthStencilSurface_Override,
  //                     &D3D9SetDepthStencilSurface_Detour,
  //                     &D3D9SetDepthStencilSurface);
  //MH_QueueEnableHook (  D3D9SetDepthStencilSurface_Detour);

  if (GetModuleHandle (      L"D3DX9_43.DLL") != nullptr )
  { SK_CreateDLLHook2 (      L"D3DX9_43.DLL",
                              "D3DXCreateTextureFromFileInMemoryEx",
                               D3DXCreateTextureFromFileInMemoryEx_Detour,
      static_cast_p2p <void> (&D3DXCreateTextureFromFileInMemoryEx_Original) );
    SK_CreateDLLHook2 (      L"D3DX9_43.DLL",
                              "D3DXCreateTextureFromFileExW",
                               D3DXCreateTextureFromFileExW_Detour,
      static_cast_p2p <void> (&D3DXCreateTextureFromFileExW_Original) );
    SK_CreateDLLHook2 (      L"D3DX9_43.DLL",
                              "D3DXCreateTextureFromFileExA",
                               D3DXCreateTextureFromFileExA_Detour,
      static_cast_p2p <void> (&D3DXCreateTextureFromFileExA_Original) );
    SK_CreateDLLHook2 (      L"D3DX9_43.DLL",
                              "D3DXCreateVolumeTextureFromFileInMemoryEx",
                               D3DXCreateVolumeTextureFromFileInMemoryEx_Detour,
      static_cast_p2p <void> (&D3DXCreateVolumeTextureFromFileInMemoryEx_Original) );
    SK_CreateDLLHook2 (      L"D3DX9_43.DLL",
                              "D3DXCreateCubeTextureFromFileInMemoryEx",
                               D3DXCreateCubeTextureFromFileInMemoryEx_Detour,
      static_cast_p2p <void> (&D3DXCreateCubeTextureFromFileInMemoryEx_Original) );
  }
}

// Skip the purge step on shutdown
bool shutting_down = false;

void
SK::D3D9::TextureManager::Shutdown (void)
{
  // Seriously?  WTF was I smoking?!
  SK::D3D9::TextureManager& tex_mgr =
    SK_D3D9_GetTextureManager ();

  // 16.6 ms per-frame (60 FPS)
  constexpr float frame_time = 16.6f;

  while (! injector.textures_to_stream.empty ())
  {
    SK::D3D9::TexLoadRef ref (nullptr);

    while (! injector.textures_to_stream.try_pop (ref)) ;
  }

  shutting_down = true;

  tex_mgr.reset ();

  DeleteCriticalSection (&injector.cs_tex_stream);
  DeleteCriticalSection (&injector.cs_tex_resample);
  DeleteCriticalSection (&injector.cs_tex_blacklist);

  DeleteCriticalSection (&cs_cache);
  DeleteCriticalSection (&osd_cs);

  SK_CloseHandle (decomp_semaphore);

  tex_log->Log ( L"[Perf Stats] At shutdown: %7.2f seconds (%7.2f frames)"
                 L" saved by cache",
                   time_saved / 1000.0f,
                     time_saved / frame_time );
  tex_log->close ();

  auto& to_delete =
    screenshots_to_delete;

  while (! to_delete.empty ())
  {
    std::wstring
      file_to_delete = to_delete.front ();
                       to_delete.pop   ();
    DeleteFileW ( file_to_delete.c_str () );
  }
}

void
SK::D3D9::TextureManager::purge (void)
{
  if (shutting_down)
    return;

  static auto& _remove_textures = remove_textures.get ();

  int     released           = 0;
  int     released_injected  = 0;
  int64_t reclaimed          = 0;
  int64_t reclaimed_injected = 0;

  tex_log->Log (L"[ Tex. Mgr ] -- TextureManager::purge (...) -- ");

  tex_log->Log ( L"[ Tex. Mgr ]  ***  Current Cache Size: %6.2f MiB "
                                          L"(User Limit: %6.2f MiB)",
                  (double)cacheSizeTotal () / (1024.0 * 1024.0),
                    (double)config.textures.cache.max_size );

  tex_log->Log (L"[ Tex. Mgr ]   Releasing textures...");

  std::vector <Texture *> unreferenced_textures;

  for ( auto& it : textures )
  {
    //if (remove_textures.count (it.second->d3d9_tex))
    if (it.second != nullptr)
    {
      if (it.second->d3d9_tex->can_free)
        unreferenced_textures.emplace_back (it.second);
    }
  }

  std::sort ( unreferenced_textures.begin (),
              unreferenced_textures.end   (),
      []( const Texture* const a,
          const Texture* const b )
    {
      return a->d3d9_tex->last_used.QuadPart <
             b->d3d9_tex->last_used.QuadPart;
    }
  );

  auto free_it =
    unreferenced_textures.cbegin ();

  // We need to over-free, or we will likely be purging every other texture load
  int64_t target_size =
    std::max (128, (int)(config.textures.cache.max_size - 0.15 *
                         config.textures.cache.max_size)) * 1024LL * 1024LL;
  int64_t start_size  =
    cacheSizeTotal ();

  std::set <IDirect3DTexture9 *> cleared;

  while ( start_size - reclaimed > target_size &&
            free_it != unreferenced_textures.end () )
  {
    int             tex_refs = -1;
    ISKTextureD3D9* pSKTex   = (*free_it)->d3d9_tex;

    if (pSKTex == nullptr)
    {
      ++free_it;
      continue;
    }

    //
    // Skip loads that are in-flight so that we do not hitch
    //
    if (injector.isStreaming ((*free_it)->crc32c))
    {
      ++free_it;
      continue;
    }

    //
    // Do not evict blocking loads, they are generally small and
    //   will cause performance problems if we have to reload them
    //     again later.
    //
    if (pSKTex->must_block)
    {
      ++free_it;
      continue;
    }

    if (cleared.count (pSKTex->pTex))
    {
      ++free_it;
      continue;
    }

    int64_t ovr_size  = 0;
    int64_t base_size = 0;

    ++free_it;

    uint32_t checksum = pSKTex->tex_crc32c;

    base_size = pSKTex->tex_size;
    ovr_size  = pSKTex->override_size;
    tex_refs  = pSKTex->pTex ? pSKTex->pTex->Release () : -1;

    if (tex_refs <= 0)
    {
      InterlockedAdd64 (&basic_size,  -base_size);

      if (! pSKTex->freed && pSKTex->pTexOverride)
        injected_refs [checksum]--;

      if (               pSKTex->pTex != nullptr)
        cleared.emplace (pSKTex->pTex);

       textures [checksum]      = nullptr;
      _remove_textures [pSKTex] = false;

      pSKTex->freed = true;
    }

    else
    {
      tex_log->Log (
        L"[ Tex. Mgr ] Invalid reference count (%li)!", tex_refs
      );
    }

    ++released;

    if (tex_refs != -1)
      reclaimed  += base_size;
  }

  //start_size  =
  //  cacheSizeTotal ();

  std::map <uint32_t, IDirect3DTexture9 *> injected_remove;

  for ( auto& it : injected_textures )
  {
    if ( injected_textures [it.first] != nullptr &&
         injected_refs     [it.first] == 0 )
    {
      injected_remove.emplace (
        std::make_pair (it.first, it.second)
      );
    }
  }

  for ( auto& it : injected_remove )
  {
    if (start_size - reclaimed <= target_size)
      break;

    if (                it.second != nullptr &&
         injected_refs [it.first] == 0 )
    {
      it.second->Release ();

      reclaimed          += injected_sizes [it.first];
      reclaimed_injected += injected_sizes [it.first];
      released_injected++;

      InterlockedAdd64     (&injected_size, -(LONG64)injected_sizes [it.first]);
      InterlockedDecrement (&injected_count);

      // Concurrent maps don't support remove
      injected_textures [it.first] = nullptr;
      //injected_load_times.erase (it.first);
      //injected_sizes.erase      (it.first);
    }
  }

  tex_log->Log ( L"[ Tex. Mgr ]   %4d textures (%4zu remain)",
                   released,
                     textures.size () );

  tex_log->Log ( L"[ Tex. Mgr ]   >> Reclaimed %6.2f MiB of memory (%6.2f MiB from %li inject)",
         sk::narrow_cast <double> (reclaimed)          / (1024.0 * 1024.0),
         sk::narrow_cast <double> (reclaimed_injected) / (1024.0 * 1024.0),
                                    released_injected );
  updateOSD ();

  tex_log->Log (L"[ Tex. Mgr ] ----------- Finished ------------ ");
}

LARGE_INTEGER liLastReset = { 0LL };

void
SK::D3D9::TextureManager::reset (void)
{
  if (! init)
    return;

  liLastReset = SK_QueryPerf ();

  static auto& _remove_textures = remove_textures.get ();

  if (! outstanding_screenshots.empty ())
  {
    tex_log->LogEx ( true,
      L"[Screenshot] A queued screenshot has not finished,"
      L" delaying device reset..."
    );

    while (! outstanding_screenshots.empty ())
      ;

    tex_log->LogEx (false, L"done!\n");
  }

  int iters = 0;

  while (true)
  {
    getTexture (0);

    auto keep_going = [&]
    {
      if (injector.hasPendingLoads ())
        return true;
      
      for ( auto& it : injector.textures_in_flight )
      {
        if (it.second != nullptr)
          return true;
      }
      
      return false;
    };

    if (! keep_going ())
    {
      __need_purge = false;
      break;
    }

    __need_purge = true;
    loadQueuedTextures ( );

    ++iters;
  }

  known.render_targets.clear ();
  used.render_targets.clear  ();

  //int underflows       = 0;

  int ext_refs         = 0;
  int ext_textures     = 0;

  int release_count    = 0;
  int unreleased_count = 0;
  int ref_count        = 0;

  int      released_injected  = 0;
   int64_t reclaimed          = 0;
   int64_t reclaimed_injected = 0;
   int64_t persisted          = 0;
   int64_t persisted_injected = 0;

  tex_log->Log (L"[ Tex. Mgr ] -- TextureManager::reset (...) -- ");

  int64_t original_cache_size = cacheSizeTotal ();

  tex_log->Log (L"[ Tex. Mgr ]   Releasing textures...");

  for (auto& it : injected_textures)
  {
    if (it.second == nullptr)
      continue;

    D3DSURFACE_DESC              surfDesc = { };
    it.second->GetLevelDesc (0, &surfDesc);

    SK_ComPtr <IDirect3DDevice9>     pDev9;
    it.second->GetDevice           (&pDev9);
    SK_ComQIPtr <IDirect3DDevice9Ex> pDev9Ex
                                    (pDev9);

    // Managed memory survives device reset
    if (surfDesc.Pool == D3DPOOL_MANAGED || pDev9Ex.p != nullptr)
    {                                    // Also D3D9Ex devices
      persisted_injected += injected_sizes [it.first];
      continue;
    }

    it.second->Release ();

    reclaimed          += injected_sizes [it.first];
    released_injected++;
    reclaimed_injected += injected_sizes [it.first];

    InterlockedDecrement (&this->injected_count);
    InterlockedAdd64     (&this->basic_size, -(SSIZE_T)injected_sizes [it.first]);
  }

  injected_textures.clear   ();
  injected_load_times.clear ();
  injected_sizes.clear      ();
  injected_refs.clear       ();

  InterlockedExchange64 (&injected_size,  0);
  InterlockedExchange   (&injected_count, 0);

  for ( auto& it : textures )
  {
    if (it.second == nullptr)
      continue;

    ISKTextureD3D9* pSKTex =
      it.second->d3d9_tex;

    // The game isn't expecting managed resources to poof when
    //   a device is reset, so we need to skip those.
    SK_ComPtr <IDirect3DDevice9>     pDev9;
    pSKTex->GetDevice              (&pDev9);
    SK_ComQIPtr <IDirect3DDevice9Ex> pDev9Ex
                                    (pDev9);

    // Managed memory survives device reset
    if (it.second->original_pool == D3DPOOL_MANAGED || pDev9Ex.p != nullptr)
    {                                    // Also D3D9Ex devices
             ref_count++; // This is not accurate, but good enough
      unreleased_count++;

      persisted += it.second->size;

      continue;
    }

    bool    can_free  = false;
    int64_t base_size = 0;
    int64_t ovr_size  = 0;

    int tex_refs =
      pSKTex->refs;

    if (pSKTex->can_free)
    {
      if (tex_refs > 0)
        tex_refs = pSKTex->Release ();

      can_free  = true;
      base_size = pSKTex->tex_size;
      ovr_size  = pSKTex->override_size;
    }

    else
    {
      ext_refs     += pSKTex->refs;
      ext_textures ++;

      ++unreleased_count;
      continue;
    }

    if (tex_refs <= 1)
    {
      ++release_count;

      ref_count += 1;

       textures        [pSKTex->tex_crc32c] = nullptr;
      _remove_textures [pSKTex]             = true;

      InterlockedAdd64     (&this->basic_size, -(SSIZE_T)base_size);

      reclaimed += base_size;
    }

    else
    {
      ++unreleased_count;
      ext_refs     += tex_refs;
      ext_textures ++;
    }
  }

  getTexture         (0);
  loadQueuedTextures ( );

  tex_log->Log ( L"[ Tex. Mgr ]   %4d textures (%4d references)",
                   release_count + unreleased_count,
                       ref_count + ext_refs );

  if (ext_refs > 0)
  {
    tex_log->Log ( L"[ Tex. Mgr ] >> WARNING: The game is still holding references (%d) to %d textures !!!",
                     ext_refs, ext_textures );
  }

  else
  {
    remove_textures->clear ();
  }

  if ((int32_t)cacheSizeBasic () < 0)
    InterlockedExchange64 (&basic_size, 0);

  static constexpr auto
    _BytesPerMiB = 1048576.0;

  tex_log->Log ( L"[ Mem. Mgr ] === Memory Management Summary ===");

  tex_log->Log ( L"[ Mem. Mgr ]  %12.2f MiB Freed",
                    (double)std::max (0LL, reclaimed) / (_BytesPerMiB) );
  tex_log->Log ( L"[ Mem. Mgr ]  %12.2f MiB Leaked",
                   (double)(original_cache_size       -
                            std::max (0LL, reclaimed) -
                            std::max (0LL, persisted  +  persisted_injected))
                                                      / (_BytesPerMiB) );
  tex_log->Log ( L"[ Mem. Mgr ]  %12.2f MiB In-Use",
                    (double)std::max (0LL, persisted  +  persisted_injected)
                                                      / (_BytesPerMiB) );

  updateOSD ();

  textures_used.clear           ();
  textures_last_frame.clear     ();

  tex_log->Log (L"[ Tex. Mgr ] ----------- Finished ------------ ");
}

void
SK::D3D9::TextureManager::updateOSD (void)
{
#if 0
  return;

  if (! init)
    return;

  double cache_basic    = (double)cacheSizeBasic    () / (1048576.0f);
  double cache_injected = (double)cacheSizeInjected () / (1048576.0f);
  double cache_total    = cache_basic + cache_injected;

  osd_stats = "";

  char      szFormatted [64] = { };
  sprintf ( szFormatted, "%6zu Total Textures : %8.2f MiB",
              numTextures () + numInjectedTextures (),
                cache_total );
  osd_stats += szFormatted;

  SK_ComPtr <IDirect3DDevice9> pDevice = nullptr;

  SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  if (            rb.device != nullptr                                     &&
       SUCCEEDED (rb.device->QueryInterface <IDirect3DDevice9> (&pDevice)) &&
                    pDevice->GetAvailableTextureMem () / 1048576UL != 4095    )
  {
    sprintf ( szFormatted, "    (%4lu MiB Available)\n",
              pDevice->GetAvailableTextureMem () / 1048576UL );
  }
  else
    sprintf (szFormatted, "\n");

  osd_stats += szFormatted;

  sprintf ( szFormatted, "%6zu  Base Textures : %8.2f MiB    %s\n",
              numTextures (),
                cache_basic,
                  __remap_textures ? "" : "<----" );

  osd_stats += szFormatted;

  sprintf ( szFormatted, "%6li   New Textures : %8.2f MiB    %s\n",
              numInjectedTextures (),
                cache_injected,
                  __remap_textures ? "<----" : "" );

  osd_stats += szFormatted;

  sprintf ( szFormatted, "%6lu Cache Hits     : %8.2f Seconds Saved",
              hits,
                time_saved / 1000.0f );

  osd_stats += szFormatted;

  if (debug_tex_id != 0x00)
  {
    osd_stats += "\n\n";

    sprintf ( szFormatted, " Debug Texture : %08x",
                debug_tex_id );

    osd_stats += szFormatted;
  }
#endif
}

std::set <uint32_t> textures_used_last_dump;
           int32_t  tex_dbg_idx              = 0L;

void
SK::D3D9::TextureManager::logUsedTextures (void)
{
  if (! init)
    return;

  if (__log_used)
  {
    textures_used_last_dump.clear ();
    tex_dbg_idx = 0;

    tex_log->Log (L"[ Tex. Log ] ---------- FrameTrace ----------- ");

    for (const uint32_t it : textures_used)
    {
      auto tex_record =
        getTexture (it);

      // Handle the RARE case where a purge happens immediately following
      //   the last frame
      if ( tex_record           != nullptr &&
           tex_record->d3d9_tex != nullptr )
      {
        auto* pSKTex =
          tex_record->d3d9_tex;

        textures_used_last_dump.emplace (it);

        tex_log->Log ( L"[ Tex. Log ] %08x.dds  { Base: %6.2f MiB,  "
                       L"Inject: %6.2f MiB,  Load Time: %8.3f ms }",
                         it,
                           (double)pSKTex->tex_size /
                             (1024.0 * 1024.0),

                     pSKTex->override_size != 0 ?
                       (double)pSKTex->override_size /
                             (1024.0 * 1024.0) : 0.0,

                           getTexture (it)->load_time );
      }
    }

    tex_log->Log (L"[ Tex. Log ] ---------- FrameTrace ----------- ");

    __log_used = false;
  }

  textures_used.clear ();
}


volatile LONG TextureWorkerThread::num_threads_init = 0UL;

HRESULT
WINAPI
ResampleTexture (TexLoadRequest* load)
{
  load->start = SK_QueryPerf ();

  D3DXIMAGE_INFO img_info = { };

  HRESULT hr =
    SK_D3DXGetImageInfoFromFileInMemory (
      load->pSrcData,
        load->SrcDataSize,
          &img_info );

  if (SUCCEEDED (hr) && img_info.Depth == 1)
  {
    hr =
      SK_D3DXCreateTextureFromFileInMemoryEx (
        load->pDevice,
          load->pSrcData, load->SrcDataSize,
            img_info.Width, img_info.Height, 0,
              0, false/*config.textures.uncompressed*/ ? D3DFMT_A8R8G8B8 : img_info.Format,
                D3DPOOL_DEFAULT,
                  D3DX_FILTER_TRIANGLE | D3DX_FILTER_DITHER,
                  D3DX_FILTER_BOX      | D3DX_FILTER_DITHER,
                    0,
                      nullptr, nullptr,
                        &load->pSrc );
  }

  else
  {
    tex_log->Log (L"[ Tex. Mgr ] Will not resample cubemap...");
  }

  delete []
    std::exchange (load->pSrcData, nullptr);

  return hr;
}

DWORD
__stdcall
SK::D3D9::TextureWorkerThread::ThreadProc (LPVOID user)
{
  if (! streaming_memory::data_len ())
  {
    streaming_memory::data_len () = 0;
    streaming_memory::data     () = nullptr;
    streaming_memory::data_age () = 0;
  }

  SYSTEM_INFO        sysinfo = { };
  SK_GetSystemInfo (&sysinfo);


  ULONG thread_num =
    InterlockedIncrement (&num_threads_init);

  // If a system has more than 4 CPUs (logical or otherwise), let the last one
  //   be dedicated to rendering.
  ULONG processor_num = thread_num % ( sysinfo.dwNumberOfProcessors > 4 ?
                                         sysinfo.dwNumberOfProcessors - 1 :
                                         sysinfo.dwNumberOfProcessors );

  // Tales of Symphonia and Zestiria both pin the render thread to the last
  //   CPU... let's try to keep our worker threads OFF that CPU.

  SK_SetThreadIdealProcessor (SK_GetCurrentThread (),         processor_num);
     SetThreadAffinityMask   (SK_GetCurrentThread (), (1UL << processor_num) & 0xFFFFFFFF);

  auto* pThread =
    static_cast <TextureWorkerThread *> (user);

  DWORD dwWaitStatus = 0;

  struct {
    const DWORD job_start  = WAIT_OBJECT_0;
    const DWORD mem_trim   = WAIT_OBJECT_0 + 1;
    const DWORD thread_end = WAIT_OBJECT_0 + 2;
  } wait;

  do
  {
    dwWaitStatus =
      WaitForMultipleObjectsEx ( 3,
                                 pThread->control_.ops,
                                   FALSE,
                                     INFINITE, FALSE );

    // New Work Ready
    if (dwWaitStatus == wait.job_start)
    {
      SK::D3D9::TextureManager& tex_mgr =
        SK_D3D9_GetTextureManager ();

      TexLoadRequest* pStream = pThread->job_;

      auto bLoading =
        tex_mgr.injector.beginLoad ();
      {
        if (pStream->type == TexLoadRequest::Resample)
        {
          InterlockedIncrement      (&tex_mgr.injector.resampling);

          pStream->start = SK_QueryPerf ();

          HRESULT hr =
            ResampleTexture (pStream);

          pStream->end   = SK_QueryPerf ();

          InterlockedDecrement      (&tex_mgr.injector.resampling);

          if (SUCCEEDED (hr))
            pThread->pool_->postFinished (pStream);

          else
          {
            tex_log->Log ( L"[ Tex. Mgr ] Texture Resample Failure (hr=%x) for texture %x, blacklisting from future resamples...",
                             hr, pStream->checksum );
            resample_blacklist->emplace (pStream->checksum);

            pStream->pDest->Release ();
            pStream->pSrc = pStream->pDest;

            ((ISKTextureD3D9 *)pStream->pSrc)->must_block = false;
            ((ISKTextureD3D9 *)pStream->pSrc)->refs--;

            tex_mgr.injector.finishedStreaming (pStream->checksum);
          }

          pThread->finishJob ();
        }

        else
        {
          SK_ReleaseAssert (pStream->type == TexLoadRequest::Immediate ||
                            pStream->type == TexLoadRequest::Stream);

          InterlockedIncrement        (&tex_mgr.injector.streaming);
          InterlockedExchangeAdd      (&tex_mgr.injector.streaming_bytes, pStream->SrcDataSize);

          pStream->start = SK_QueryPerf ();

          HRESULT hr =
            tex_mgr.injectTexture     ( pStream);

          pStream->end   = SK_QueryPerf ();

          InterlockedExchangeSubtract (&tex_mgr.injector.streaming_bytes, pStream->SrcDataSize);
          InterlockedDecrement        (&tex_mgr.injector.streaming);

          if (SUCCEEDED (hr))
          {
            pThread->pool_->postFinished (pStream);
          }

          else
          {
            tex_log->Log ( L"[ Tex. Mgr ] Texture Injection Failure (hr=%x) for texture %x, removing from injectable list...",
                             hr, pStream->checksum);

            if (tex_mgr.isTextureInjectable     (pStream->checksum))
                tex_mgr.removeInjectableTexture (pStream->checksum);

            if (pStream->pSrc != nullptr) pStream->pSrc->Release ();

                pStream->pSrc = pStream->pDest;

            if (pStream->pSrc != nullptr)
            {
              ((ISKTextureD3D9 *)pStream->pSrc)->must_block = false;
              ((ISKTextureD3D9 *)pStream->pSrc)->refs--;
            }

            tex_mgr.injector.finishedStreaming (pStream->checksum);
          }

          pThread->finishJob ();
        }
      }
      tex_mgr.injector.endLoad (bLoading);
    }

    else if (dwWaitStatus == (wait.mem_trim))
    {
      // Yay for magic numbers :P   ==> (8 MiB Min Size, 5 Seconds Between Trims)
      //
      constexpr size_t   MIN_SIZE = 8192 * 1024;
      constexpr uint32_t MIN_AGE  = 5000UL;

      size_t before = streaming_memory::data_len ();

      streaming_memory::trim ( MIN_SIZE,
                                 SK_timeGetTime () - MIN_AGE );

      size_t now    = streaming_memory::data_len ();

      if (before != now)
      {
#ifdef _WIN64
        tex_log->Log ( L"[ Mem. Mgr ]  Trimmed %9lzu bytes of temporary memory for tid=%x",
                         before - now,
                           GetCurrentThreadId () );
#else
        tex_log->Log ( L"[ Mem. Mgr ]  Trimmed %9zu bytes of temporary memory for tid=%x",
                         before - now,
                           GetCurrentThreadId () );
#endif
      }
    }

    else if (dwWaitStatus != (wait.thread_end))
    {
      dll_log->Log ( L"[ Tex. Mgr ] Unexpected Worker Thread Wait Status: %X",
                       dwWaitStatus );
    }
  } while (dwWaitStatus != (wait.thread_end));

  streaming_memory::trim (0, SK_timeGetTime ());

  SK_Thread_CloseSelf ();
  return 0;
}

DWORD
__stdcall
SK::D3D9::TextureThreadPool::Spooler (LPVOID user)
{
  auto* pPool =
    static_cast <TextureThreadPool *> (user);

  SK_WaitForSingleObject (pPool->events_.jobs_added, INFINITE);

  while (SK_WaitForSingleObject (pPool->events_.shutdown, 0) == WAIT_TIMEOUT)
  {
    TexLoadRequest* pJob =
      pPool->getNextJob ();

    while (pJob != nullptr)
    {
      bool started = false;

      for ( auto& it : pPool->workers_ )
      {
        if (! it->isBusy ())
        {
          if (! started)
          {
            it->startJob (pJob);
            started = true;
          }

          else
          {
            it->trim ();
          }
        }
      }

      // All worker threads are busy, so wait...
      if (! started)
      {
        SK_WaitForSingleObject (pPool->events_.results_waiting, INFINITE);
      }

      else
      {
        pJob =
          pPool->getNextJob ();
      }
    }

    constexpr int MAX_TIME_BETWEEN_TRIMS = 1500UL;
    while ( SK_WaitForSingleObject (
              pPool->events_.jobs_added,
                MAX_TIME_BETWEEN_TRIMS ) ==
                               WAIT_TIMEOUT )
    {
      for ( auto& it : pPool->workers_ )
      {
        if (! it->isBusy ())
        {
          it->trim ();
        }
      }
    }
  }

  SK_Thread_CloseSelf ();
  return 0;
}

void
SK::D3D9::TextureWorkerThread::finishJob (void)
{
  InterlockedExchangeAdd64   (&bytes_loaded_,
           ((TexLoadRequest *)ReadPointerAcquire ((PVOID *)&job_))
                             ->SrcDataSize);
  InterlockedIncrement       (&jobs_retired_);
  InterlockedExchangePointer ((PVOID *)&job_, nullptr);
}




void
SK::D3D9::TextureManager::refreshDataSources (void)
{
  if (! init)
    return;

  // NEED SYNC BARRIER:  Wait for pending loads to finish

  static bool
        crc_init = false;
  if (! crc_init)
  {
    CrcGenerateTable ();
    crc_init = true;
  }

  CFileInStream arc_stream  = { };
  CLookToRead   look_stream = { };

  FileInStream_CreateVTable (&arc_stream);
  LookToRead_CreateVTable   (&look_stream, FALSE);

  look_stream.realStream = &arc_stream.s;
  LookToRead_Init         (&look_stream);

  injectable_textures.clear ();
  archives.clear            ();

  //
  // Walk injectable textures so we don't have to query the filesystem on every
  //   texture load to check if a injectable one exists.
  //
  if ( GetFileAttributesW ((*SK_D3D11_res_root + L"\\inject").c_str ()) !=
         INVALID_FILE_ATTRIBUTES )
  {
    WIN32_FIND_DATA fd     = {   };
    int             files  =   0;
    LARGE_INTEGER   liSize = { 0 };

    tex_log->LogEx ( true, L"[Inject Tex] Enumerating injectable textures..." );

    HANDLE hFind =
      FindFirstFileW ((*SK_D3D11_res_root + L"\\inject\\textures\\blocking\\*").c_str (), &fd);

    if (hFind != INVALID_HANDLE_VALUE)
    {
      do
      {
        if (fd.dwFileAttributes != INVALID_FILE_ATTRIBUTES)
        {
          if (wcsstr (_wcslwr (fd.cFileName), L".dds"))
          {
            uint32_t                           checksum = 0;
            swscanf (fd.cFileName, L"%x.dds", &checksum);

            // Already got this texture...
            if (TRUE == ReadAcquire (&injectable_textures [checksum].removed))
                continue;

            ++files;

            LARGE_INTEGER fsize;

            fsize.HighPart = fd.nFileSizeHigh;
            fsize.LowPart  = fd.nFileSizeLow;

            liSize.QuadPart += fsize.QuadPart;

            TexRecord rec = { };

            rec.size    = liSize.LowPart;
            rec.archive = std::numeric_limits <unsigned int>::max ();
            rec.method  = Blocking;
            InterlockedExchange (
           &rec.removed, FALSE  );

            injectable_textures [checksum] = rec;
          }
        }
      } while (FindNextFileW (hFind, &fd) != 0);

      FindClose (hFind);
    }

    hFind =
      FindFirstFileW ((*SK_D3D11_res_root + L"\\inject\\textures\\streaming\\*").c_str (), &fd);

    if (hFind != INVALID_HANDLE_VALUE)
    {
      do
      {
        if (fd.dwFileAttributes != INVALID_FILE_ATTRIBUTES)
        {
          if (wcsstr (_wcslwr (fd.cFileName), L".dds"))
          {
            uint32_t                           checksum = 0;
            swscanf (fd.cFileName, L"%x.dds", &checksum);

            // Already got this texture...
            if (TRUE == ReadAcquire (&injectable_textures [checksum].removed))
                continue;

            ++files;

            LARGE_INTEGER fsize;

            fsize.HighPart = fd.nFileSizeHigh;
            fsize.LowPart  = fd.nFileSizeLow;

            liSize.QuadPart += fsize.QuadPart;

            TexRecord rec = { };

            rec.size    = fsize.LowPart;
            rec.archive = std::numeric_limits <unsigned int>::max ();
            rec.method  = Streaming;
            InterlockedExchange (
           &rec.removed, FALSE  );

            injectable_textures [checksum] = rec;
          }
        }
      } while (FindNextFileW (hFind, &fd) != 0);

      FindClose (hFind);
    }

    hFind =
      FindFirstFileW ((*SK_D3D11_res_root + L"\\inject\\textures\\*").c_str (), &fd);

    if (hFind != INVALID_HANDLE_VALUE)
    {
      do
      {
        if (fd.dwFileAttributes != INVALID_FILE_ATTRIBUTES)
        {
          if (wcsstr (_wcslwr (fd.cFileName), L".dds"))
          {
            uint32_t                           checksum = 0;
            swscanf (fd.cFileName, L"%x.dds", &checksum);

            // Already got this texture...
            if (TRUE == ReadAcquire (&injectable_textures [checksum].removed))
                continue;

            ++files;

            LARGE_INTEGER fsize;

            fsize.HighPart = fd.nFileSizeHigh;
            fsize.LowPart  = fd.nFileSizeLow;

            liSize.QuadPart += fsize.QuadPart;

            TexRecord rec = { };

            rec.size    = fsize.LowPart;
            rec.archive = std::numeric_limits <unsigned int>::max ();
            InterlockedExchange (
           &rec.removed, FALSE  );

            injectable_textures [checksum] = rec;
          }
        }
      } while (FindNextFileW (hFind, &fd) != 0);

      FindClose (hFind);
    }

    hFind =
      FindFirstFileW ((*SK_D3D11_res_root + L"\\inject\\*.*").c_str (), &fd);

    if (hFind != INVALID_HANDLE_VALUE)
    {
      int archive = 0;

      do
      {
        if (fd.dwFileAttributes != INVALID_FILE_ATTRIBUTES)
        {
          wchar_t* wszArchiveNameLwr =
            _wcslwr (_wcsdup (fd.cFileName));

          if ( wcsstr (wszArchiveNameLwr, L".7z") )
          {
            int tex_count = 0;

            CSzArEx       arc = { };
            ISzAlloc      thread_alloc;
            ISzAlloc      thread_tmp_alloc;

            thread_alloc.Alloc     = SzAlloc;
            thread_alloc.Free      = SzFree;

            thread_tmp_alloc.Alloc = SzAllocTemp;
            thread_tmp_alloc.Free  = SzFreeTemp;

            wchar_t wszQualifiedArchiveName [MAX_PATH + 2] = { };

            swprintf ( wszQualifiedArchiveName,
                         L"%s\\inject\\%s",
                           SK_D3D11_res_root->c_str (),
                             fd.cFileName );

            if (InFile_OpenW (&arc_stream.file, wszQualifiedArchiveName))
            {
              tex_log->Log ( L"[Inject Tex]  ** Cannot open archive file: %s",
                               wszQualifiedArchiveName );
              continue;
            }

            SzArEx_Init (&arc);

            if ( SzArEx_Open ( &arc,
                                 &look_stream.s,
                                   &thread_alloc,
                                     &thread_tmp_alloc ) == SZ_OK )
            {
              uint32_t i;

              wchar_t wszEntry [MAX_PATH + 2] = { };

              for (i = 0; i < arc.NumFiles; i++)
              {
                if (SzArEx_IsDir (&arc, i))
                  continue;

                SzArEx_GetFileNameUtf16 (&arc, i, (UInt16 *)wszEntry);

                // Truncate to 32-bits --> there's no way in hell a texture will ever be >= 2 GiB
                UInt64 fileSize = SzArEx_GetFileSize (&arc, i);

                wchar_t* wszFullName =
                  _wcslwr (_wcsdup (wszEntry));

                if ( wcsstr ( wszFullName, L".dds") )
                {
                  TexLoadMethod method = DontCare;

                  uint32_t checksum = 0x00;
                  wchar_t* wszUnqualifiedEntry =
                    wszFullName + wcslen (wszFullName);

                  // Strip the path
                  while (  wszUnqualifiedEntry >= wszFullName &&
                          *wszUnqualifiedEntry != L'/')
                    wszUnqualifiedEntry = SK_CharPrevW (wszFullName, wszUnqualifiedEntry);

                  if (*wszUnqualifiedEntry == L'/')
                    wszUnqualifiedEntry = SK_CharNextW (wszUnqualifiedEntry);

                  swscanf (wszUnqualifiedEntry, L"%x.dds", &checksum);

                  // Already got this texture...
                  if ( isTextureInjectable  (checksum) ||
                       isTextureBlacklisted (checksum) )
                  {
                    free (wszFullName);
                    continue;
                  }

                  if (wcsstr (wszFullName, L"streaming"))
                    method = Streaming;
                  else if (wcsstr (wszFullName, L"blocking"))
                    method = Blocking;

                  TexRecord rec = { };

                  rec.size    = (uint32_t)fileSize;
                  rec.archive = archive;
                  rec.fileno  = i;
                  rec.method  = method;

                  injectable_textures.insert (std::make_pair (checksum, rec));

                  ++tex_count;
                  ++files;

                  liSize.QuadPart += rec.size;
                }

                free (wszFullName);
              }

              if (tex_count > 0)
              {
                ++archive;
                archives.emplace_back (wszQualifiedArchiveName);
              }
            }

            SzArEx_Free (&arc, &thread_alloc);
            File_Close  (&arc_stream.file);
          }

          free (wszArchiveNameLwr);
        }
      } while (FindNextFileW (hFind, &fd) != 0);

      FindClose (hFind);
    }

    tex_log->LogEx ( false, L" %li files (%3.1f MiB)\n",
                       files, (double)liSize.QuadPart / (1024.0 * 1024.0) );
  }

  File_Close  (&arc_stream.file);
}


bool
SK::D3D9::TextureManager::TextureManager::reloadTexture (uint32_t checksum)
{
  if (! init)
    return false;

  if ( ! isTextureInjectable (checksum) )
    return false;

  D3DPOOL pool = D3DPOOL_DEFAULT;

  Texture* pCacheTex =
    getTexture (checksum);

  ISKTextureD3D9* pTex =
    pCacheTex ? pCacheTex->d3d9_tex :
                nullptr;

  if (pTex != nullptr && pTex->pTexOverride != nullptr)
  {
    tex_log->LogEx ( true, L"[Inject Tex] Reloading texture for checksum (%08x)... ",
                       checksum );

    InterlockedDecrement (&injected_count);
    InterlockedAdd64     (&injected_size, -pTex->override_size);

    D3DSURFACE_DESC         surfDesc = { };
    pTex->GetLevelDesc (0, &surfDesc);
    pool =                  surfDesc.Pool;

    pTex->pTexOverride->Release ();
    pTex->pTexOverride = nullptr;
  }

  else
  {
    return false;
  }

  TexRecord record =
    getInjectableTexture (checksum);

  if (record.method == DontCare)
    record.method = Streaming;

  TexLoadRequest* load_op = nullptr;

  wchar_t wszInjectFileName [MAX_PATH + 2] = { };

  bool remap_stream =
    injector.isStreaming (checksum);

  // If -1, load from disk...
  if (record.archive == std::numeric_limits <unsigned int>::max ())
  {
    if (record.method == Streaming)
    {
      swprintf ( wszInjectFileName, L"%s\\inject\\textures\\streaming\\%08x%s",
                   SK_D3D11_res_root->c_str (),
                     checksum,
                       L".dds" );
    }

    else if (record.method == Blocking)
    {
      swprintf ( wszInjectFileName, L"%s\\inject\\textures\\blocking\\%08x%s",
                   SK_D3D11_res_root->c_str (),
                     checksum,
                       L".dds" );
    }
  }

  load_op           = new TexLoadRequest ();

  SK_GetCurrentRenderBackend ().device->QueryInterface <IDirect3DDevice9> (&load_op->pDevice);

  load_op->checksum = checksum;
  load_op->type     = TexLoadRequest::Stream;

  wcscpy (load_op->wszFilename, wszInjectFileName);

  if (load_op->type == TexLoadRequest::Stream)
  {
    if ((! remap_stream))
      tex_log->LogEx ( false, L"streaming\n" );
    else
      tex_log->LogEx ( false, L"in-flight already\n" );
  }

  load_op->SrcDataSize =
    isTextureInjectable (checksum) ?
      (UINT)injectable_textures [checksum].size : 0;

  load_op->pDest    = pTex;
  load_op->mem_pool = pool;

  pTex->must_block = false;

  if (injector.isStreaming (load_op->checksum))
  {
    //ISKTextureD3D9* pTexOrig =
    //  (ISKTextureD3D9 *)injector.getTextureInFlight (load_op->checksum)->pDest;

    // Remap the output of the in-flight texture
    injector.getTextureInFlight (load_op->checksum)->pDest =
      pTex;

    Texture* pTexCache = getTexture (load_op->checksum);

    if (pTexCache != nullptr)
    {
      for ( int i = 0;
                i < pTexCache->refs;
              ++i )
      {
        pTex->AddRef ();
      }
    }
  }

  else
  {
    injector.addTextureInFlight (load_op);
    load_op->pDest->AddRef      (       );
    stream_pool.postJob         (load_op);
  }

  if (injector.hasPendingLoads ())
    loadQueuedTextures ();

  return true;
}



void
SK::D3D9::TextureManager::getThreadStats (std::vector <TexThreadStats>&/* stats*/)
{
////  stats =
////    resample_pool->getWorkerStats ();

  // For Inject (Small, Large) -> Push Back
}


void
SK::D3D9::TextureThreadPool::postJob (TexLoadRequest* job)
{
  if (! job)
    return;

  // Defer the creation of this until the first job is posted
  if (! spool_thread_)
  {
    spool_thread_ =
      SK_Thread_CreateEx ( Spooler, L"[SK] D3D9 Texture Dispatch", this );
  }

  // Don't let the game free this while we are working on it...
  job->pDest->AddRef ();

  jobs_.push           (job);
  InterlockedIncrement (&jobs_waiting_);

  SetEvent             (events_.jobs_added);
}



IDirect3DTexture9*
ISKTextureD3D9::getDrawTexture (void) const
{
  IDirect3DTexture9* pTexToUse = nullptr;

  switch (img_to_use)
  {
    case ContentPreference::DontCare:
    {
      if (__remap_textures && pTexOverride != nullptr)
        pTexToUse = pTexOverride;

      else
        pTexToUse = pTex;
    } break;

    case ContentPreference::Original:
    {
      pTexToUse = pTex;
    } break;

    case ContentPreference::Override:
    {
      if (pTexOverride != nullptr)
        pTexToUse = pTexOverride;
      else
        pTexToUse = pTex;
    } break;
  };

  if (debug_tex_id > 0 && tex_crc32c == (uint32_t)debug_tex_id && config.textures.highlight_debug_tex)
  {
    extern DWORD tracked_tex_blink_duration;

    if (SK_timeGetTime () % tracked_tex_blink_duration > tracked_tex_blink_duration / 2)
      pTexToUse = nullptr;
  }

  if (freed)
    return nullptr;

  return pTexToUse;
}

IDirect3DTexture9*
ISKTextureD3D9::use (void)
{
  if (config.textures.dump_on_load && pTex != nullptr && uses > 0 && uses < 2)
  {
    SK::D3D9::TextureManager& tex_mgr =
      SK_D3D9_GetTextureManager ();

    D3DSURFACE_DESC desc = { };

    if ( SUCCEEDED (pTex->GetLevelDesc (0, &desc)) &&
      ( ! ( tex_mgr.isTextureDumped     (tex_crc32c) ||
            tex_mgr.isTextureInjectable (tex_crc32c) ) ) &&
          (desc.Pool != D3DPOOL_MANAGED) )
    {
      tex_log->Log ( L"[Dump Trace] Texture:   (%lu x %lu) * <LODs: %lu> - CRC32C: %08X",
                        desc.Width, desc.Height, pTex->GetLevelCount (), tex_crc32c);
      tex_log->Log ( L"[Dump Trace]              Usage: %-20hs - Format: %-20hs",
                        SK_D3D9_UsageToStr  (desc.Usage).c_str  (),
                        SK_D3D9_FormatToStr (desc.Format).c_str ());
      tex_log->Log ( L"[Dump Trace]               Pool: %hs",
                        SK_D3D9_PoolToStr (desc.Pool));

      bool bLoading =
        tex_mgr.injector.beginLoad ();

      tex_mgr.dumpTexture (desc.Format, tex_crc32c, pTex);

      tex_mgr.injector.endLoad (bLoading);
    }
  }

  ++uses;

  last_used = SK_QueryPerf ();

  return getDrawTexture ();
}

void
ISKTextureD3D9::toggleOverride (void)
{
  if (img_to_use != ContentPreference::Override)
    img_to_use = ContentPreference::Override;
  else
    img_to_use = ContentPreference::DontCare;
}

void
ISKTextureD3D9::toggleOriginal (void)
{
  if (img_to_use != ContentPreference::Original)
    img_to_use = ContentPreference::Original;
  else
    img_to_use = ContentPreference::DontCare;
}

COM_DECLSPEC_NOTHROW
HRESULT
ISKTextureD3D9::UnlockRect (UINT Level)
{
  if (config.system.log_level > 1)
  {
    tex_log->Log ( L"[ Tex. Mgr ] ISKTextureD3D9::UnlockRect (%lu)", Level );
  }

  SK_ComPtr <IDirect3DDevice9> pDev;
  this->GetDevice (&pDev.p);

  if (pTex == nullptr)
    return S_OK;

  HRESULT hr = E_FAIL;

  if (this->tex_crc32c == 0x00 && dirty && Level == 0   && config.textures.d3d9_mod)
  {
    auto sptr =
      static_cast <const uint8_t *> (
        lock_lvl0.pBits
      );

    uint32_t crc32c_ = 0x00;

    D3DSURFACE_DESC             desc = { };
    pTex->GetLevelDesc (Level, &desc);

    INT stride =
      SK_D3D9_BytesPerPixel (desc.Format);

    // BCn compressed textures have to be read 4-rows at a time.
    bool BCn_tex = stride < 0;

    if (stride < 0)
    {
      if (stride == -1)
        stride = std::max (1UL, ((desc.Width + 3UL) / 4UL) ) * 8UL;
      else
        stride = std::max (1UL, ((desc.Width + 3UL) / 4UL) ) * 16UL;
    }

    else
      stride *= desc.Width;



    // Fast-path:  Data is packed in accordance to alignment rules
    if (lock_lvl0.Pitch == stride)
    {
      unsigned int lod_size = BCn_tex ? stride * (desc.Height / 4 +
                                                  desc.Height % 4) :
                                        stride * desc.Height;

      crc32c_ =
        safe_crc32c (crc32c_, sptr, lod_size );
    }

    // Slow-path:  Must checksum 1 or more rows over multiple scans
    else
    {
      SK_RunOnce (tex_log->Log (L"Slow hash for unoptimally aligned texture memory activated."));

      for (size_t h = 0; h < desc.Height; BCn_tex ? h += 4 : ++h)
      {
        crc32c_ =
          safe_crc32c (crc32c_, sptr, stride * ( BCn_tex ? 4 : 1 ));

        sptr += lock_lvl0.Pitch;
      }
    }


    this->tex_crc32c = crc32c_;
    this->tex_size   = 0;

    INT W = desc.Width; INT H = desc.Height;

    for (UINT i = 0; i < pTex->GetLevelCount (); i++)
    {
      this->tex_size += ( lock_lvl0.Pitch * W );

      if (W > 1) W >>= 1;
      if (H > 1) H >>= 1;
    }

    hr =
      pTex->UnlockRect (Level);

    SK::D3D9::TextureManager& tex_mgr =
      SK_D3D9_GetTextureManager ();

    if (SUCCEEDED (hr) && crc32c_ != 0x0 && (! tex_mgr.injector.isInjectionThread ()) && (! tex_mgr.getTexture (crc32c_)))
    {
      this->last_used.QuadPart = SK_QueryPerf ().QuadPart;
      this->uses++;

      auto* pCacheTex =
        new SK::D3D9::Texture ();

      pCacheTex->original_pool = desc.Pool;
      pCacheTex->d3d9_tex      = this;
      pCacheTex->size          = this->tex_size;

      pCacheTex->crc32c               = crc32c_;
      pCacheTex->d3d9_tex->tex_crc32c = crc32c_;

      pCacheTex->d3d9_tex->AddRef ();
      pCacheTex->refs++;

      end_map = SK_QueryPerf ();

      pCacheTex->load_time = (float)( 1000.0 *
                               (double)( end_map.QuadPart - begin_map.QuadPart ) /
                               (double)(   SK_PerfFreq      ) );

      tex_mgr.addTexture (this->tex_crc32c, pCacheTex, this->tex_size);

      TexLoadRequest* load_op = nullptr;

      bool remap_stream =
        tex_mgr.injector.isStreaming (crc32c_);

#if 1
      //if (injected_textures.count (crc32c_) && injected_textures [crc32c_] != nullptr)
      //{
      //  this->pTexOverride   = injected_textures   [this->tex_crc32c];
      //  this->override_size  = injected_sizes      [this->tex_crc32c];
      //  pCacheTex->load_time = injected_load_times [this->tex_crc32c];
      //  
      //  tex_mgr.refTextureEx (pCacheTex);
      //}

      //
      // Generic injectable textures
      //
      //else
      {
        tex_mgr.missTexture ();

        if ( tex_mgr.isTextureInjectable (crc32c_) )
        {
          tex_log->LogEx ( true, L"[Inject Tex] Injectable texture for checksum (%08x)... ",
                             crc32c_ );

          wchar_t wszInjectFileName [MAX_PATH + 2] = { };

          TexRecord& record =
            tex_mgr.getInjectableTexture (crc32c_);

          if (record.method == TexLoadMethod::DontCare)
            record.method = TexLoadMethod::Streaming;

          // If -1, load from disk...
          if (record.archive == -1)
          {
            if (record.method == TexLoadMethod::Streaming)
            {
              swprintf ( wszInjectFileName, L"%s\\inject\\textures\\streaming\\%08x%s",
                           SK_D3D11_res_root->c_str (),
                             crc32c_,
                               L".dds" );
            }

            else if (record.method == TexLoadMethod::Blocking)
            {
              swprintf ( wszInjectFileName, L"%s\\inject\\textures\\blocking\\%08x%s",
                           SK_D3D11_res_root->c_str (),
                             crc32c_,
                               L".dds" );
            }
          }

          load_op           = new TexLoadRequest ();

          load_op->pDevice  = pDev;
          load_op->checksum = crc32c_;
          load_op->mem_pool = desc.Pool;

          if (record.method == TexLoadMethod::Streaming)
            load_op->type   = TexLoadRequest::Stream;
          else
            load_op->type   = TexLoadRequest::Immediate;

          wcscpy (load_op->wszFilename, wszInjectFileName);

          if (load_op->type == TexLoadRequest::Stream)
          {
            if ((! remap_stream))
              tex_log->LogEx ( false, L"streaming\n" );
            else
              tex_log->LogEx ( false, L"in-flight already\n" );
          }

          else
          {
            tex_log->LogEx ( false, L"blocking (deferred)\n" );
          }

          if ( load_op != nullptr && ( load_op->type == TexLoadRequest::Stream ||
                                       load_op->type == TexLoadRequest::Immediate ) )
          {
            load_op->SrcDataSize =
              static_cast <UINT> (
                tex_mgr.isTextureInjectable (crc32c_)         ?
                  tex_mgr.getInjectableTexture (crc32c_).size : 0
              );
            
            load_op->pDest =
              this;
            
            if (load_op->type == TexLoadRequest::Immediate)
              this->must_block = true;

            tex_mgr.injector.lockStreaming ();
            
            if (tex_mgr.injector.isStreaming (load_op->checksum))
            {
              auto* pTexOrig =
                static_cast <ISKTextureD3D9 *> (
                  tex_mgr.injector.getTextureInFlight (load_op->checksum)->pDest
                );
              
              //tex_mgr.injector.unlockStreaming ();
              //tex_mgr.injector.lockStreaming   ();
              
              // Remap the output of the in-flight texture
              tex_mgr.injector.getTextureInFlight (load_op->checksum)->pDest =
                this->pTexOverride;
              
              Texture* pBaseTex =
                tex_mgr.getTexture (load_op->checksum);
              
              if (pBaseTex != nullptr)
              {
                for ( int i = 0;
                          i < pBaseTex->refs;
                        ++i )
                {
                  this->AddRef ();
                }
              }
              
              tex_mgr.removeTexture (pTexOrig);
            }
            
            else
            {
              tex_mgr.injector.addTextureInFlight (load_op);
              this->AddRef ();
              stream_pool.postJob                 (load_op);
            }

            tex_mgr.injector.unlockStreaming ();
          }
        }
      }

#if 0
      //
      // TODO:  Actually stream these, but block if the game tries to call SetTexture (...)
      //          while the texture is in-flight.
      //
      else if (load_op != nullptr && load_op->type == tsf_tex_load_s::Immediate) {
        QueryPerformanceFrequency        (&load_op->freq);
        QueryPerformanceCounter_Original (&load_op->start);

        EnterCriticalSection (&cs_tex_inject);
        inject_tids.insert   (GetCurrentThreadId ());
        LeaveCriticalSection (&cs_tex_inject);

        load_op->pDest = *ppTexture;

        hr = InjectTexture (load_op);

        EnterCriticalSection (&cs_tex_inject);
        inject_tids.erase    (GetCurrentThreadId ());
        LeaveCriticalSection (&cs_tex_inject);

        QueryPerformanceCounter_Original (&load_op->end);

        if (SUCCEEDED (hr)) {
          tex_log->Log ( L"[Inject Tex] Finished synchronous texture %08x (%5.2f MiB in %9.4f ms)",
                          load_op->checksum,
                            (double)load_op->SrcDataSize / (1024.0f * 1024.0f),
                              1000.0f * (double)(load_op->end.QuadPart - load_op->start.QuadPart) /
                                        (double) load_op->freq.QuadPart );
          ISKTextureD3D9* pSKTex =
            (ISKTextureD3D9 *)*ppTexture;

          pSKTex->pTexOverride  = load_op->pSrc;
          pSKTex->override_size = load_op->SrcDataSize;

          pSKTex->last_used     = load_op->end;

          tsf::RenderFix::tex_mgr.addInjected (load_op->SrcDataSize);
        } else {
          tex_log->Log ( L"[Inject Tex] *** FAILED synchronous texture %08x",
                          load_op->checksum );
        }

        delete load_op;
        load_op = nullptr;
      }
#endif
#endif
    }
  }
  else
  {
    hr = pTex->UnlockRect (Level);
  }
  if (Level == 0)
    dirty = false;

  return hr;
}











