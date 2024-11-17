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
#pragma once

#include <com_util.h>

extern const GUID SKID_D3D11Texture2D_DISCARD;

// The texture was removed from cache and no further attempts to cache should be made
static constexpr GUID SKID_D3D11TextureUncacheable =
// {5C5398CA-0F9C-5932-A19D-A2E69792AE04}
  { 0x5c5398ca, 0xf9c,  0x5932, { 0xa1, 0x9d, 0xa2, 0xe6, 0x97, 0x92, 0xae, 0x4 } };

struct cache_params_s {
  uint32_t max_entries       = 4096UL;
  uint32_t min_entries       = 1024UL;
  uint32_t max_size          = 2048UL; // Measured in MiB
  uint32_t min_size          = 512UL;
  uint32_t min_evict         = 16;
  uint32_t max_evict         = 64;
      bool ignore_non_mipped = false;
} extern cache_opts;

struct resample_job_s {
  DirectX::ScratchImage *data;
  ID3D11Texture2D       *texture;

  uint32_t               crc32c;

  struct {
    int64_t preprocess; // Time performing various work required for submission
                        //   (i.e. decompression)

    int64_t received;
    int64_t started;
    int64_t finished;
  } time;
};

// Temporary staging for memory-mapped texture uploads
//
struct mapped_resources_s {
  std::unordered_map <ID3D11Resource*,  D3D11_MAPPED_SUBRESOURCE> textures;
  std::unordered_map <ID3D11Resource*,  uint64_t>                 texture_times;

  std::unordered_map <ID3D11Resource*,  uint32_t>                 dynamic_textures;
  std::unordered_map <ID3D11Resource*,  uint32_t>                 dynamic_texturesx;
  std::map           <uint32_t,         uint64_t>                 dynamic_times2;
  std::map           <uint32_t,         size_t>                   dynamic_sizes2;
  std::map           <uint32_t,         std::wstring>             dynamic_files2; // Single-LOD Staged Upload
};

struct SK_D3D11_TEXTURE2D_DESC
{
  UINT                  Width;
  UINT                  Height;
  UINT                  MipLevels;
  UINT                  ArraySize;
  DXGI_FORMAT           Format;
  DXGI_SAMPLE_DESC      SampleDesc;
  D3D11_USAGE           Usage;
  D3D11_BIND_FLAG       BindFlags;
  D3D11_CPU_ACCESS_FLAG CPUAccessFlags;
  UINT                  MiscFlags;

  explicit SK_D3D11_TEXTURE2D_DESC (D3D11_TEXTURE2D_DESC& descFrom) noexcept
  {
    Width          = descFrom.Width;
    Height         = descFrom.Height;
    MipLevels      = descFrom.MipLevels;
    ArraySize      = descFrom.ArraySize;
    Format         = descFrom.Format;
    SampleDesc     = descFrom.SampleDesc;
    Usage          = descFrom.Usage;
    BindFlags      = (D3D11_BIND_FLAG      )descFrom.BindFlags;
    CPUAccessFlags = (D3D11_CPU_ACCESS_FLAG)descFrom.CPUAccessFlags;
    MiscFlags      = descFrom.MiscFlags;
  }
};

extern std::unique_ptr <SK_Thread_HybridSpinlock> tex_cs;
extern std::unique_ptr <SK_Thread_HybridSpinlock> hash_cs;
extern std::unique_ptr <SK_Thread_HybridSpinlock> dump_cs;
extern std::unique_ptr <SK_Thread_HybridSpinlock> cache_cs;
extern std::unique_ptr <SK_Thread_HybridSpinlock> inject_cs;
extern std::unique_ptr <SK_Thread_HybridSpinlock> preload_cs;

// Can only be accessed from the thread drawing ImGui
extern SK_LazyGlobal <std::unordered_set <ID3D11Texture2D *>>
                     used_textures;

extern SK_LazyGlobal <std::unordered_map <ID3D11DeviceContext *, mapped_resources_s>>
                     mapped_resources;

extern SK_LazyGlobal <std::wstring>
                     SK_D3D11_res_root;
extern bool          SK_D3D11_need_tex_reset;
extern bool          SK_D3D11_try_tex_reset;
extern int32_t       SK_D3D11_amount_to_purge;

extern bool          SK_D3D11_dump_textures;
extern bool          SK_D3D11_inject_textures_ffx;
extern bool          SK_D3D11_inject_textures;
extern bool          SK_D3D11_cache_textures;
extern bool          SK_D3D11_mark_textures;

extern
  volatile LONG      SK_D3D11_TexRefCount_Failures;

extern std::wstring  SK_D3D11_TexNameFromChecksum   (uint32_t top_crc32, uint32_t checksum, uint32_t ffx_crc32 = 0x00);
extern bool          SK_D3D11_IsTexInjectThread     (SK_TLS *pTLS = SK_TLS_Bottom ());
extern SK_ScopedBoolFwd
                     SK_D3D11_DeclareTexInjectScope (SK_TLS* pTLS = SK_TLS_Bottom ());


void __stdcall SK_D3D11_AddInjectable    (uint32_t top_crc32,  uint32_t checksum);
void __stdcall SK_D3D11_RemoveInjectable (uint32_t top_crc32,  uint32_t checksum);
void __stdcall SK_D3D11_AddTexHash       (const wchar_t* name, uint32_t top_crc32, uint32_t hash);

bool __stdcall SK_D3D11_IsDumped         (uint32_t top_crc32, uint32_t checksum);
bool __stdcall SK_D3D11_IsInjectable     (uint32_t top_crc32, uint32_t checksum);
bool __stdcall SK_D3D11_IsInjectable_FFX (uint32_t top_crc32);

int     SK_D3D11_ReloadAllTextures (void);
HRESULT SK_D3D11_ReloadTexture     ( ID3D11Texture2D* pTex,
                                     SK_TLS*          pTLS = SK_TLS_Bottom () );

HRESULT
__stdcall
SK_D3D11_MipmapCacheTexture2DEx ( DirectX::ScratchImage&   img,
                                  uint32_t                 crc32c,
                                  ID3D11Texture2D*       /*pOutTex*/,
                                  DirectX::ScratchImage** ppOutImg,
                                  SK_TLS*                  pTLS );

HRESULT
__stdcall
SK_D3D11_MipmapCacheTexture2D ( _In_ ID3D11Texture2D*      pTex,
                                     uint32_t              crc32c,
                                     SK_TLS*               pTLS = SK_TLS_Bottom (),
                                     ID3D11DeviceContext*  pDevCtx = (ID3D11DeviceContext *)SK_GetCurrentRenderBackend ().d3d11.immediate_ctx,
                                     ID3D11Device*         pDev    = (ID3D11Device        *)SK_GetCurrentRenderBackend ().device.p );

HRESULT __stdcall SK_D3D11_DumpTexture2D ( _In_ ID3D11Texture2D* pTex, uint32_t crc32c );
HRESULT __stdcall SK_D3D11_DumpTexture2D ( _In_ const D3D11_TEXTURE2D_DESC   *pDesc,
                                           _In_ const D3D11_SUBRESOURCE_DATA *pInitialData,
                                           _In_       uint32_t                top_crc32,
                                           _In_       uint32_t                checksum );
BOOL              SK_D3D11_DeleteDumpedTexture (uint32_t crc32c);

bool
SK_D3D11_IsStagingCacheable ( D3D11_RESOURCE_DIMENSION  rdim,
                              ID3D11Resource           *pRes,
                              SK_TLS                   *pTLS = nullptr );

BOOL SK_D3D11_MarkTextureUncacheable ( ID3D11Texture2D *pTexture );
BOOL SK_D3D11_IsTextureUncacheable   ( ID3D11Texture2D *pTexture );


struct lod_hash_table_s
{
  lod_hash_table_s (void) noexcept (false)
  {
    mutex =
      std::make_shared <SK_Thread_HybridSpinlock> (120);
  }

  lod_hash_table_s  (const lod_hash_table_s &) = default;

  ~lod_hash_table_s (void)                     = default;

  void              reserve     (size_t   resrv ) { std::scoped_lock <SK_Thread_HybridSpinlock> _lock (*mutex); InterlockedIncrement (&contention_score.reserve);         entries.reserve (resrv ); };
  bool              contains    (uint32_t crc32c) { std::scoped_lock <SK_Thread_HybridSpinlock> _lock (*mutex); InterlockedIncrement (&contention_score.contains); return entries.find    (crc32c) !=
                                                                                                                                                                          entries.cend    (      ); };
  void              erase       (uint32_t crc32c) { std::scoped_lock <SK_Thread_HybridSpinlock> _lock (*mutex); InterlockedIncrement (&contention_score.erase);           entries.erase   (crc32c); };
  ID3D11Texture2D*& operator [] (uint32_t crc32c) { std::scoped_lock <SK_Thread_HybridSpinlock> _lock (*mutex); InterlockedIncrement (&contention_score.index);    return entries         [crc32c]; };

  bool              contains    (ID3D11Texture2D *pTex) { std::scoped_lock <SK_Thread_HybridSpinlock> _lock (*mutex); InterlockedIncrement (&contention_score.contains); return reventries.find    (pTex) !=
                                                                                                                                                                                reventries.cend    (    ); };
  void              erase       (ID3D11Texture2D *pTex) { std::scoped_lock <SK_Thread_HybridSpinlock> _lock (*mutex); InterlockedIncrement (&contention_score.erase);           reventries.erase   (pTex); };
  uint32_t&         operator [] (ID3D11Texture2D *pTex) { std::scoped_lock <SK_Thread_HybridSpinlock> _lock (*mutex); InterlockedIncrement (&contention_score.index);    return reventries         [pTex]; };

  void              touch       (ID3D11Texture2D *pTex);

  std::unordered_map < uint32_t,
                       ID3D11Texture2D * > entries;
  std::unordered_map < ID3D11Texture2D *,
                       uint32_t         >  reventries;
  concurrency::concurrent_unordered_map
                     < ID3D11Texture2D *,
                       ULONG64          >  last_frame;
  std::shared_ptr
               <SK_Thread_HybridSpinlock>  mutex;

  struct {
    volatile LONG reserve  = 0L;
    volatile LONG contains = 0L;
    volatile LONG erase    = 0L;
    volatile LONG index    = 0L;
  } contention_score;
};

struct fmt_hash_matrix_s
{
  fmt_hash_matrix_s (void)
  {
    map.resize (20);

    map [ 1].reserve  ( 256); // Only      1x1
    map [ 2].reserve  ( 512); // Up to     2x2
    map [ 3].reserve  ( 512); // Up to     4x4
    map [ 4].reserve  ( 512); // Up to     8x8
    map [ 5].reserve  (1024); // Up to    16x16
    map [ 6].reserve  ( 512); // Up to    32x32
    map [ 7].reserve  ( 256); // Up to    64x64
    map [ 8].reserve  ( 256); // Up to   128x128
    map [ 9].reserve  (1280); // Up to   256x256
    map [10].reserve  (2048); // Up to   512x512
    map [11].reserve  (4096); // Up to  1024x1024
    map [12].reserve  (4096); // Up to  2048x2048
    map [13].reserve  (1024); // Up to  4096x4096
    map [14].reserve  (  16); // Up to  8192x8192
    map [15].reserve  (   8); // Up to 16384x16384
    map [16].reserve  (   4); // Up to 32768x32768
    map [17].reserve  (   2); // Up to 65536x65536
  }

  std::vector <lod_hash_table_s> map;
};

// Actually more of a cache manager at the moment...
class SK_D3D11_TexMgr {
public:
  SK_D3D11_TexMgr (void) {
    HashMap_Fmt.resize  (200);
    HashMap_2D.resize   (20);
    Blacklist_2D.resize (20);

    //TexRefs_2D.reserve       (8192);
    //Textures_2D.reserve      (8192);
    HashMap_2D.at ( 1).reserve  ( 256); // Only      1x1
    HashMap_2D.at ( 2).reserve  ( 512); // Up to     2x2
    HashMap_2D.at ( 3).reserve  ( 512); // Up to     4x4
    HashMap_2D.at ( 4).reserve  ( 512); // Up to     8x8
    HashMap_2D.at ( 5).reserve  (1024); // Up to    16x16
    HashMap_2D.at ( 6).reserve  ( 512); // Up to    32x32
    HashMap_2D.at ( 7).reserve  ( 256); // Up to    64x64
    HashMap_2D.at ( 8).reserve  ( 256); // Up to   128x128
    HashMap_2D.at ( 9).reserve  (1280); // Up to   256x256
    HashMap_2D.at (10).reserve  (2048); // Up to   512x512
    HashMap_2D.at (11).reserve  (4096); // Up to  1024x1024
    HashMap_2D.at (12).reserve  (4096); // Up to  2048x2048
    HashMap_2D.at (13).reserve  (1024); // Up to  4096x4096
    HashMap_2D.at (14).reserve  (  16); // Up to  8192x8192
    HashMap_2D.at (15).reserve  (   8); // Up to 16384x16384
    HashMap_2D.at (16).reserve  (   4); // Up to 32768x32768
    HashMap_2D.at (17).reserve  (   2); // Up to 65536x65536

    AggregateSize_2D  = 0ULL;
    RedundantData_2D  = 0ULL;
    RedundantLoads_2D = 0UL;
    Entries_2D        = 0UL;
    CacheMisses_2D    = 0UL;
    Evicted_2D        = 0UL;
    Budget            = 0ULL;
  }

  bool             isTexture2D  (uint32_t crc32, const D3D11_TEXTURE2D_DESC *pDesc);

  ID3D11Texture2D* getTexture2D ( uint32_t              tag,
                            const D3D11_TEXTURE2D_DESC *pDesc,
                                  size_t               *pMemSize   = nullptr,
                                  float                *pTimeSaved = nullptr,
                                  SK_TLS               *pTLS       = SK_TLS_Bottom () );

  void             refTexture2D ( ID3D11Texture2D      *pTex,
                            const D3D11_TEXTURE2D_DESC *pDesc,
                                  uint32_t              tag,
                                  size_t                mem_size,
                                  uint64_t              load_time,
                                  uint32_t              crc32c,
                            const wchar_t              *fileName   = L"",
                            const D3D11_TEXTURE2D_DESC *pOrigDesc  = nullptr,
                         _In_opt_ HMODULE               hModCaller = (HMODULE)(intptr_t)-1,
                         _In_opt_ SK_TLS               *pTLS       = SK_TLS_Bottom () );

  void             updateDebugNames (void);

  // Some texture upload paths (i.e. CopyResource or UpdateSubresoure)
  //   result in cache hits where no new object is created; call this to
  //     indicate a cache hit, but leave the reference count alone.
  LONG             recordCacheHit ( ID3D11Texture2D      *pTex );

  void             reset           (void);
  bool             purgeTextures   (size_t size_to_free, int* pCount, size_t* pFreed);

  struct tex2D_descriptor_s {
    volatile LONG         hits       = 0L;
    ID3D11Texture2D      *texture    = nullptr;
    D3D11_TEXTURE2D_DESC  desc       = { };
    D3D11_TEXTURE2D_DESC  orig_desc  = { };
    size_t                mem_size   = 0L;
    uint64_t              load_time  = 0ULL;
    uint32_t              tag        = 0x00; // Combined data and descriptor hash for collision mitigation
    uint32_t              crc32c     = 0x00;
        bool              injected   = false;
        bool              discard    = false;
    uint64_t              last_frame = 0ULL;
    uint64_t              last_used  = 0ULL;
    std::string           debug_name =  "";
    std::wstring          file_name  = L"";  // If injected, this is the source file
  };

  concurrency::concurrent_unordered_set <
    ID3D11Texture2D *
  >      TexRefs_2D;

  std::vector        < fmt_hash_matrix_s  >   HashMap_Fmt;
  std::vector        < lod_hash_table_s   >   HashMap_2D;
  std::vector        < std::unordered_set <
                        uint32_t          >
                     >                        Blacklist_2D;

#if 1
  concurrency::concurrent_unordered_map < ID3D11Texture2D *,
                                          tex2D_descriptor_s  >  Textures_2D;
#else
  std::unordered_map <ID3D11Texture2D *, tex2D_descriptor_s> Textures_2D;
#endif

  std::atomic_uint64_t                        AggregateSize_2D  = 0ULL;
  std::atomic_uint64_t                        RedundantData_2D  = 0ULL;
  std::atomic_uint32_t                        RedundantLoads_2D = 0UL;
  std::atomic_uint32_t                        Entries_2D        = 0UL;
  std::atomic_uint32_t                        CacheMisses_2D    = 0UL;
  std::atomic_uint32_t                        Evicted_2D        = 0UL;
  std::atomic_uint64_t                        Budget            = 0ULL;
  float                                       RedundantTime_2D  = 0.0f;

  std::atomic_int64_t                         LastModified_2D   = 0ULL;
  std::atomic_int64_t                         LastPurge_2D      = 0ULL;

  std::unordered_map <uint32_t, std::wstring> tex_hashes;
  std::unordered_map <uint32_t, std::wstring> tex_hashes_ex;

  std::unordered_set <uint32_t>               dumped_textures;
  uint64_t                                    dumped_texture_bytes     = 0ULL;
  std::unordered_set <uint32_t>               dumped_collisions;
  std::unordered_set <uint32_t>               injectable_textures;
  uint64_t                                    injectable_texture_bytes = 0ULL;
  std::unordered_set <uint32_t>               injected_collisions;

  std::unordered_set <uint32_t>               injectable_ffx; // HACK FOR FFX
};

extern uint64_t SK_D3D11_MipmapCacheSize;

bool
SK_D3D11_Resampler_PostJob         ( resample_job_s       job );

bool
SK_D3D11_Resampler_ProcessFinished ( ID3D11Device        *pDev,
                                     ID3D11DeviceContext *pDevCtx,
                                     SK_TLS              *pTLS = SK_TLS_Bottom () );

extern LONG        SK_D3D11_Resampler_GetActiveJobCount  (void);
extern LONG        SK_D3D11_Resampler_GetWaitingJobCount (void);
extern LONG        SK_D3D11_Resampler_GetErrorCount      (void);
extern int         SK_D3D11_ReloadAllTextures            (void);
extern std::string SK_D3D11_SummarizeTexCache            (void);

extern SK_LazyGlobal <SK_D3D11_TexMgr> SK_D3D11_Textures;