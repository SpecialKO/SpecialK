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

#include <SpecialK/render/d3d11/d3d11_core.h>
#include <SpecialK/render/d3d11/d3d11_shader.h>

// For really wacky engines that wrap shader interfaces w/o correct layering
static constexpr GUID SKID_D3D11KnownShaderCrc32c =
// {5C5298BB-0F9D-5022-A19D-A2E69792AE14}
  { 0x5c5298bb, 0xf9d,  0x5022, { 0xa1, 0x9d, 0xa2, 0xe6, 0x97, 0x92, 0xae, 0x14 } };

// ID3D11DeviceContext* private data used for indexing various per-ctx lookups
static constexpr GUID SKID_D3D11DeviceContextHandle =
// {5C5298CA-0F9C-5932-A19D-A2E69792AE03}
  { 0x5c5298ca, 0xf9c,  0x5932, { 0xa1, 0x9d, 0xa2, 0xe6, 0x97, 0x92, 0xae, 0x3 } };

// The device context a command list was built using
static constexpr GUID SKID_D3D11DeviceContextOrigin =
// {5C5298CA-0F9D-5022-A19D-A2E69792AE03}
{ 0x5c5298ca, 0xf9d,  0x5022, { 0xa1, 0x9d, 0xa2, 0xe6, 0x97, 0x92, 0xae, 0x03 } };

// The device context a command list was built using
static constexpr GUID SKID_D3D11DeviceBasePtr =
// {DF92B6F4-997C-42D5-A509-B1BD987B3D7B}
{ 0xdf92b6f4, 0x997c, 0x42d5, { 0xa5, 0x9, 0xb1, 0xbd, 0x98, 0x7b, 0x3d, 0x7b } };

// ReShade's wrapper GUID
static constexpr GUID SKID_ReShade_D3D11Device =
// {72299288-2C68-4AD8-945D-2BFB5AA9C609}
  { 0x72299288, 0x2C68,  0x4AD8, { 0x94, 0x5D, 0x2B, 0xFB, 0x5A, 0xA9, 0xC6, 0x09 } };

// Stash wrapped ID3D11DeviceContext* pointers inside the underlying device so that
//   GetImmediateContext will return the wrapped interface always
static constexpr GUID SKID_D3D11WrappedImmediateContext =
// {F87115E6-A0F9-4E62-97F7-1487A2E34A47}
{ 0xf87115e6, 0xa0f9, 0x4e62, { 0x97, 0xf7, 0x14, 0x87, 0xa2, 0xe3, 0x4a, 0x47 } };


enum class SK_D3D11DispatchType
{
  Standard,
  Indirect
};

enum SK_D3D11_DrawHandlerState
{
  Normal,
  Override,
  Skipped
};

enum class SK_D3D11DrawType
{
  Auto,
  PrimList,
  Indexed,
  IndexedInstanced,
  IndexedInstancedIndirect,
  Instanced,
  InstancedIndirect
};

struct SK_D3D11_ContextResources
{
//std::unordered_set <ID3D11Texture2D*> used_textures;
//std::unordered_set <IUnknown*       > temp_resources;
  std::unordered_set <SK_ComPtr <ID3D11Texture2D> > used_textures;
  std::unordered_set <SK_ComPtr <IUnknown       > > temp_resources;

  volatile LONG                writing_ = 0;
  UINT                         ctx_id_  = 0;
};


struct memory_tracking_s
{
  static const int __types = 5;

  memory_tracking_s (void) : cs (cs_mmio.get ())
  {
  }

  struct history_s
  {
    history_s (void) = default;
    //{
    //vertex_buffers.reserve   ( 128);
    //index_buffers.reserve    ( 256);
    //constant_buffers.reserve (2048);
    //}

    void clear (SK_Thread_CriticalSection* /*cs*/)
    {
      ///std::scoped_lock <SK_Thread_CriticalSection> auto_lock (*cs);

      for (int i = 0; i < __types; i++)
      {
        map_types      [i] = 0;
        resource_types [i] = 0;
        buffer_types   [i] = 0;
      }

      bytes_written = 0ULL;
      bytes_read    = 0ULL;
      bytes_copied  = 0ULL;

      active_set.clear_using (empty_set);
    }

    int pinned_frames = 0;

    // Clearing these containers performs dynamic heap allocation irrespective of
    //   prior state (i.e. clearing a set whose size is zero is the same as 100),
    //     so we want to swap an already cleared set whenever possible to avoid
    //       disrupting SK's thread-optimized memory placement.
    struct datastore_s {
      concurrency::concurrent_unordered_set <ID3D11Resource *> mapped_resources;
      concurrency::concurrent_unordered_set <ID3D11Buffer *>   index_buffers;
      concurrency::concurrent_unordered_set <ID3D11Buffer *>   vertex_buffers;
      concurrency::concurrent_unordered_set <ID3D11Buffer *>   constant_buffers;

      // It is only safe to call this at the end of a frame
      void clear_using ( datastore_s& fresh )
      {
        if (! mapped_resources.empty ())
        {
          std::swap (mapped_resources, fresh.mapped_resources);
          fresh.mapped_resources.clear ();
        }

        if (! index_buffers.empty ())
        {
          std::swap (index_buffers, fresh.index_buffers);
          fresh.index_buffers.clear ();
        }

        if (! vertex_buffers.empty ())
        {
          std::swap (vertex_buffers, fresh.vertex_buffers);
          fresh.vertex_buffers.clear ();
        }

        if (! constant_buffers.empty ())
        {
          std::swap (constant_buffers, fresh.constant_buffers);
          fresh.constant_buffers.clear ();
        }
      }
    } empty_set,
      active_set;

    concurrency::concurrent_unordered_set <ID3D11Buffer *>&
      index_buffers    = active_set.index_buffers;
    concurrency::concurrent_unordered_set <ID3D11Buffer *>&
      vertex_buffers   = active_set.vertex_buffers;
    concurrency::concurrent_unordered_set <ID3D11Buffer *>&
      constant_buffers = active_set.constant_buffers;
    concurrency::concurrent_unordered_set <ID3D11Resource *>&
      mapped_resources = active_set.mapped_resources;


    std::atomic <uint32_t> map_types      [__types] = { };
    std::atomic <uint32_t> resource_types [__types] = { };
    std::atomic <uint32_t> buffer_types   [__types] = { };

    std::atomic <uint64_t> bytes_read               = 0ULL;
    std::atomic <uint64_t> bytes_written            = 0ULL;
    std::atomic <uint64_t> bytes_copied             = 0ULL;
  } lifetime, last_frame;


  std::atomic <uint32_t>   num_maps                 = 0UL;
  std::atomic <uint32_t>   num_unmaps               = 0UL; // If does not match, something is pinned.


  void clear (void)
  {
    if (num_maps != num_unmaps)
      ++lifetime.pinned_frames;

    num_maps   = 0UL;
    num_unmaps = 0UL;

    for (int i = 0; i < __types; i++)
    {
      lifetime.map_types      [i] += last_frame.map_types      [i];
      lifetime.resource_types [i] += last_frame.resource_types [i];
      lifetime.buffer_types   [i] += last_frame.buffer_types   [i];
    }

    lifetime.bytes_read    += last_frame.bytes_read;
    lifetime.bytes_written += last_frame.bytes_written;
    lifetime.bytes_copied  += last_frame.bytes_copied;

    last_frame.clear (cs);
  }

  SK_Thread_HybridSpinlock* cs;
};

struct target_tracking_s
{
  target_tracking_s (void)
  {
    for ( int i = 0; i < SK_D3D11_MAX_DEV_CONTEXTS + 1; ++i )
    {
      for (auto& it : active [i])
      {
        it.store (false);
      }

      active_count [i] = 0;
    }

    //ref_vs.reserve (16); ref_ps.reserve (16);
    //ref_gs.reserve (8);
    //ref_hs.reserve (4);  ref_ds.reserve (4);
    //ref_cs.reserve (2);
  };


  struct refs_s {
    concurrency::concurrent_unordered_set <uint32_t> ref_vs;
    concurrency::concurrent_unordered_set <uint32_t> ref_ps;
    concurrency::concurrent_unordered_set <uint32_t> ref_gs;
    concurrency::concurrent_unordered_set <uint32_t> ref_hs;
    concurrency::concurrent_unordered_set <uint32_t> ref_ds;
    concurrency::concurrent_unordered_set <uint32_t> ref_cs;

    // It is only safe to call this at the end of a frame
    void clear_using ( refs_s& fresh )
    {
      //
      // Swap-and-Discard a pre-allocated concurrent set, because
      //   clearing any of the concurrency runtime containers causes heap
      //     allocation even if the container was already empty!
      //
      if (! ref_vs.empty ())
      {
        std::swap (ref_vs, fresh.ref_vs);
        fresh.ref_vs.clear ();
      }

      if (! ref_ps.empty ())
      {
        std::swap (ref_ps, fresh.ref_ps);
        fresh.ref_ps.clear ();
      }

      if (! ref_gs.empty ())
      {
        std::swap (ref_gs, fresh.ref_gs);
        fresh.ref_gs.clear ();
      }

      if (! ref_ds.empty ())
      {
        std::swap (ref_ds, fresh.ref_ds);
        fresh.ref_ds.clear ();
      }

      if (! ref_hs.empty ())
      {
        std::swap (ref_hs, fresh.ref_hs);
        fresh.ref_hs.clear ();
      }

      if (! ref_cs.empty ())
      {
        std::swap (ref_cs, fresh.ref_cs);
        fresh.ref_cs.clear ();
      }
    }
  } empty_set,
    active_set;


  void clear (void)
  {
    for ( int i = 0; i < SK_D3D11_MAX_DEV_CONTEXTS + 1; ++i )
    {
      for (auto& it : active [i]) it.store (false);

      active_count [i] = 0;
    }

    num_draws = 0;

    active_set.clear_using (empty_set);
  }

  volatile ID3D11RenderTargetView* resource     = nullptr;
  std::atomic_bool                 active       [SK_D3D11_MAX_DEV_CONTEXTS+1][D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT]
    = { };
  std::atomic <uint32_t>           active_count [SK_D3D11_MAX_DEV_CONTEXTS+1]
    = { };

  std::atomic <uint32_t>           num_draws    =     0;

  concurrency::concurrent_unordered_set <uint32_t>& ref_vs =
    active_set.ref_vs;
  concurrency::concurrent_unordered_set <uint32_t>& ref_ps =
    active_set.ref_ps;
  concurrency::concurrent_unordered_set <uint32_t>& ref_gs =
    active_set.ref_gs;
  concurrency::concurrent_unordered_set <uint32_t>& ref_hs =
    active_set.ref_hs;
  concurrency::concurrent_unordered_set <uint32_t>& ref_ds =
    active_set.ref_ds;
  concurrency::concurrent_unordered_set <uint32_t>& ref_cs =
    active_set.ref_cs;
};

struct SK_D3D11_KnownThreads
{
  SK_D3D11_KnownThreads (void) noexcept (false)///
  {
    _lock =
      std::make_unique <SK_Thread_HybridSpinlock> (0x400);

    //ids.reserve    (16);
    //active.reserve (16);
  }

  void   clear_all    (void);
  size_t count_all    (void);

  void   clear_active (void)
  {
    if (use_lock)
    {
      std::scoped_lock <SK_Thread_HybridSpinlock>
            scope_lock (*_lock);

      active.clear ();
      return;
    }

    active.clear ();
  }

  size_t count_active (void)
  {
    if (use_lock)
    {
      std::scoped_lock <SK_Thread_HybridSpinlock>
            scope_lock (*_lock);

      return active.size ();
    }

    return active.size ();
  }

  float  active_ratio (void)
  {
    if (use_lock)
    {
      std::scoped_lock <SK_Thread_HybridSpinlock>
            scope_lock (*_lock);

      return static_cast <float> (active.size ())/
             static_cast <float> (ids.size    ());
    }

    return static_cast <float> (active.size ())/
           static_cast <float> (ids.size    ());
  }

  static void mark (void) ;

private:
  std::unordered_set <DWORD>                     ids;
  std::unordered_set <DWORD>                     active;
  std::unique_ptr    <SK_Thread_HybridSpinlock> _lock;
  bool                                       use_lock = true;
};

extern SK_LazyGlobal <SK_D3D11_KnownThreads> SK_D3D11_MemoryThreads;
extern SK_LazyGlobal <SK_D3D11_KnownThreads> SK_D3D11_DrawThreads;
extern SK_LazyGlobal <SK_D3D11_KnownThreads> SK_D3D11_DispatchThreads;
extern SK_LazyGlobal <SK_D3D11_KnownThreads> SK_D3D11_ShaderThreads;

template <typename _T>
class SK_D3D11_IsShaderLoaded
{
public:
  SK_D3D11_IsShaderLoaded (ID3D11Device* pDevice, uint32_t crc32c)
  {
    auto& shaders =
      SK_D3D11_Shaders;

    static
      std::map < std::type_index,
                 std::pair < SK_Thread_CriticalSection*,
                             SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*
                           >
               >
      _ShaderMap =
      {
        { std::type_index (typeid (ID3D11VertexShader)),   { cs_shader_vs.get (), reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*> (&shaders->vertex)   } },
        { std::type_index (typeid (ID3D11PixelShader)),    { cs_shader_ps.get (), reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*> (&shaders->pixel)    } },
        { std::type_index (typeid (ID3D11GeometryShader)), { cs_shader_gs.get (), reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*> (&shaders->geometry) } },
        { std::type_index (typeid (ID3D11DomainShader)),   { cs_shader_ds.get (), reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*> (&shaders->domain)   } },
        { std::type_index (typeid (ID3D11HullShader)),     { cs_shader_hs.get (), reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*> (&shaders->hull)     } },
        { std::type_index (typeid (ID3D11ComputeShader)),  { cs_shader_cs.get (), reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*> (&shaders->compute)  } }
      };

    const auto& map_entry =
      _ShaderMap.find (std::type_index (typeid (_T)));

    if (map_entry == _ShaderMap.cend ())
      loaded = false;

    else
    {
      auto& crit_sec =
        map_entry->second.first;
      auto& repository =
        map_entry->second.second;

      crit_sec->lock ();

      loaded =
        ( repository->descs [pDevice].find (crc32c) !=
          repository->descs [pDevice].cend (      )  );

      crit_sec->unlock ();
    }
  }

  operator bool (void) const { return loaded; }

private:
  bool loaded;
};

extern std::string
  __SK_MakeSteamPS ( bool  hdr10,
                     bool  scRGB,
                     float max_luma );
extern
  ID3D10Blob*
  __SK_MakeSteamPS_Bytecode ( bool  hdr10,
                              bool  scRGB,
                              float max_luma );


extern std::string
__SK_MakeEpicPS ( bool  hdr10,
                  bool  scRGB,
                  float max_luma );

extern
  ID3D10Blob*
  __SK_MakeEpicPS_Bytecode ( bool  hdr10,
                             bool  scRGB,
                             float max_luma );

extern
  ID3D10Blob*
  __SK_MakeRTSS_PS1_Bytecode ( bool  hdr10,
                               bool  scRGB,
                               float max_luma );

extern std::string
__SK_MakeRTSSPS ( bool  hdr10,
                  bool  scRGB,
                  float max_luma );

extern
  ID3D10Blob*
  __SK_MakeRTSS_PS_Bytecode ( bool  hdr10,
                              bool  scRGB,
                              float max_luma );

void
SK_D3D11_ReleaseCachedShaders (ID3D11Device *This, sk_shader_class type);

static
__declspec (noinline)
HRESULT
SK_D3D11_CreateShader_Impl (
  _In_            ID3D11Device         *This,
  _In_      const void                 *pShaderBytecode,
  _In_            SIZE_T                BytecodeLength,
  _In_opt_        ID3D11ClassLinkage   *pClassLinkage,
  _Out_opt_       IUnknown            **ppShader,
                  sk_shader_class       type )
{
  if (pShaderBytecode == nullptr)
  {
    return
      DXGI_ERROR_INVALID_CALL;
  }

  const auto InvokeCreateRoutine =
  [&]
  {
    switch (type)
    {
      default:
      case sk_shader_class::Vertex:
        return
          D3D11Dev_CreateVertexShader_Original ( This, pShaderBytecode,
                                                   BytecodeLength, pClassLinkage,
                             (ID3D11VertexShader **)(ppShader) );
      case sk_shader_class::Pixel:
        return
          D3D11Dev_CreatePixelShader_Original ( This, pShaderBytecode,
                                                  BytecodeLength, pClassLinkage,
                             (ID3D11PixelShader **)(ppShader) );
      case sk_shader_class::Geometry:
        return
          D3D11Dev_CreateGeometryShader_Original ( This, pShaderBytecode,
                                                     BytecodeLength, pClassLinkage,
                             (ID3D11GeometryShader **)(ppShader) );
      case sk_shader_class::Domain:
        return
          D3D11Dev_CreateDomainShader_Original ( This, pShaderBytecode,
                                                   BytecodeLength, pClassLinkage,
                             (ID3D11DomainShader **)(ppShader) );
      case sk_shader_class::Hull:
        return
          D3D11Dev_CreateHullShader_Original ( This, pShaderBytecode,
                                                 BytecodeLength, pClassLinkage,
                             (ID3D11HullShader **)(ppShader) );
      case sk_shader_class::Compute:
        return
          D3D11Dev_CreateComputeShader_Original ( This, pShaderBytecode,
                                                    BytecodeLength, pClassLinkage,
                             (ID3D11ComputeShader **)(ppShader) );
    }
  };

  // If nullptr, then we don't really care about this, pass it along to the
  //   D3D11 runtime; the game will get whatever information (but no shader)
  //     it was after and we avoid an awkward mess hashing bytecode for no
  //       good reason!
  if (ppShader == nullptr)
  {
    return
      InvokeCreateRoutine ();
  }


  uint32_t checksum =
    SK_D3D11_ChecksumShaderBytecode (
                    pShaderBytecode,
                           BytecodeLength );


  static auto& rb =
    SK_GetCurrentRenderBackend ();

  const bool hash_only =
    (                                             rb.api == SK_RenderAPI::D3D12||
     (SK_GetFramesDrawn () < 2 && config.apis.last_known == SK_RenderAPI::D3D12));

  if ( hash_only )
  {
#define STEAM_OVERLAY_PS_CRC32C      0x9aefe985
#define RTSS_OVERLAY_PS_CRC32C       0x995f6505
#define RTSS_OVERLAY_PS1_CRC32C      0x4777629e
    // Not sure why Epic uses a different pixel shader for
    //   D3D12 and D3D11, when both codepaths are layered on D3D11...
#define EPIC_OVERLAY_PS_CRC32C       0xbff8dffc
#define EPIC_OVERLAY_D3D12_PS_CRC32C 0x51c67279

    // RTSS
    //
    if (type == sk_shader_class::Pixel && ( checksum == RTSS_OVERLAY_PS_CRC32C ||
                                            checksum == RTSS_OVERLAY_PS1_CRC32C ))
    {
      extern bool __SK_HDR_16BitSwap;
      extern bool __SK_HDR_10BitSwap;
      if (        __SK_HDR_16BitSwap || ( rb.hdr_capable &&
                                          rb.scanout.dxgi_colorspace == DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709 ))
      {
        static std::string rtss_ps_scRGB =
          __SK_MakeSteamPS (false, true, config.rtss.overlay_luminance);
        static std::string rtss1_ps_scRGB =
          __SK_MakeRTSSPS  (false, true, config.rtss.overlay_luminance);

        SK_RunOnce (
           SK_LOG0 ( ( L"RTSS Replacement Pixel Shader <scRGB %f cd/m�>",
                          config.rtss.overlay_luminance * 80.0 ),
                       L"RTSS Range" )
        );

        static ID3D10Blob* rtss_blob_scRGB =
        __SK_MakeSteamPS_Bytecode  (false, true, config.rtss.overlay_luminance);
        static ID3D10Blob* rtss1_blob_scRGB =
        __SK_MakeRTSS_PS1_Bytecode (false, true, config.rtss.overlay_luminance);

        switch (checksum)
        {
          case RTSS_OVERLAY_PS_CRC32C:
            pShaderBytecode = rtss_blob_scRGB->GetBufferPointer ();
            BytecodeLength  = rtss_blob_scRGB->GetBufferSize    ();
            break;
          default:
          case RTSS_OVERLAY_PS1_CRC32C:
            pShaderBytecode = rtss1_blob_scRGB->GetBufferPointer ();
            BytecodeLength  = rtss1_blob_scRGB->GetBufferSize    ();
            break;
        }
      }

      else if ( __SK_HDR_10BitSwap || ( rb.hdr_capable &&
                                        rb.scanout.dxgi_colorspace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020 ))
      {
        static std::string rtss_ps_PQ =
          __SK_MakeSteamPS (true, false, config.rtss.overlay_luminance * 80.0f);
        static std::string rtss1_ps_PQ =
          __SK_MakeRTSSPS  (true, false, config.rtss.overlay_luminance * 80.0f);

        SK_RunOnce (
        {
          SK_LOG0 ( ( L"RTSS Replacement Pixel Shader <PQ %f cd/m�>",
                             config.rtss.overlay_luminance * 80.0f ),
                      L"RTSS Range" );
        });

        static ID3D10Blob* rtss_blob_PQ =
          __SK_MakeSteamPS_Bytecode  (true, false, config.rtss.overlay_luminance * 80.0f);
        static ID3D10Blob* rtss1_blob_PQ =
          __SK_MakeRTSS_PS1_Bytecode (true, false, config.rtss.overlay_luminance * 80.0f);

        switch (checksum)
        {
          case RTSS_OVERLAY_PS_CRC32C:
            pShaderBytecode = rtss_blob_PQ->GetBufferPointer ();
            BytecodeLength  = rtss_blob_PQ->GetBufferSize    ();
            break;
          default:
          case RTSS_OVERLAY_PS1_CRC32C:
            pShaderBytecode = rtss1_blob_PQ->GetBufferPointer ();
            BytecodeLength  = rtss1_blob_PQ->GetBufferSize    ();
            break;
        }
      }
    }

    // Steam
    //
    else if (type == sk_shader_class::Pixel && checksum == STEAM_OVERLAY_PS_CRC32C)
    {
      extern bool __SK_HDR_16BitSwap;
      extern bool __SK_HDR_10BitSwap;
      if (        __SK_HDR_16BitSwap || ( rb.hdr_capable &&
                                          rb.scanout.dxgi_colorspace == DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709 ))
      {
        static std::string steam_ps_scRGB =
          __SK_MakeSteamPS (false, true, config.platform.overlay_hdr_luminance);

        SK_RunOnce (
           SK_LOG0 ( ( L"Steam Replacement Pixel Shader <scRGB %f cd/m�>",
                          config.platform.overlay_hdr_luminance * 80.0 ),
                       L"SteamRange" )
        );

        static ID3D10Blob* steam_blob_scRGB =
        __SK_MakeSteamPS_Bytecode (false, true, config.platform.overlay_hdr_luminance);

        pShaderBytecode = steam_blob_scRGB->GetBufferPointer ();
        BytecodeLength  = steam_blob_scRGB->GetBufferSize    ();

      }

      else if ( __SK_HDR_10BitSwap || ( rb.hdr_capable &&
                                        rb.scanout.dxgi_colorspace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020 ))
      {
        static std::string steam_ps_PQ =
          __SK_MakeSteamPS (true, false, config.platform.overlay_hdr_luminance * 80.0f);

        SK_RunOnce (
        {
          SK_LOG0 ( ( L"Steam Replacement Pixel Shader <PQ %f cd/m�>",
                             config.platform.overlay_hdr_luminance * 80.0f ),
                      L"SteamRange" );
        });

        static ID3D10Blob* steam_blob_PQ =
          __SK_MakeSteamPS_Bytecode (true, false, config.platform.overlay_hdr_luminance * 80.0f);

        pShaderBytecode = steam_blob_PQ->GetBufferPointer ();
        BytecodeLength  = steam_blob_PQ->GetBufferSize    ();
      }
    }

    // Epic
    //
    else if (type == sk_shader_class::Pixel && checksum == EPIC_OVERLAY_D3D12_PS_CRC32C)
    {

      extern bool __SK_HDR_16BitSwap;
      extern bool __SK_HDR_10BitSwap;
      if (        __SK_HDR_16BitSwap || ( rb.hdr_capable &&
                                          rb.scanout.dxgi_colorspace == DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709 ))
      {
        static std::string epic_ps_scRGB =
          __SK_MakeEpicPS (false, true, config.platform.overlay_hdr_luminance);

        SK_RunOnce (
           SK_LOG0 ( ( L"Epic Replacement Pixel Shader <scRGB %f cd/m�>",
                          config.platform.overlay_hdr_luminance * 80.0 ),
                       L"Epic Range" )
        );

        static ID3D10Blob* epic_blob_scRGB =
        __SK_MakeEpicPS_Bytecode (false, true, config.platform.overlay_hdr_luminance);

        pShaderBytecode = epic_blob_scRGB->GetBufferPointer ();
        BytecodeLength  = epic_blob_scRGB->GetBufferSize    ();

      }

      else if ( __SK_HDR_10BitSwap || ( rb.hdr_capable &&
                                        rb.scanout.dxgi_colorspace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020 ))
      {
        static std::string epic_ps_PQ =
          __SK_MakeEpicPS (true, false, config.platform.overlay_hdr_luminance * 80.0f);

        SK_RunOnce (
        {
          SK_LOG0 ( ( L"Epic Replacement Pixel Shader <PQ %f cd/m�>",
                             config.platform.overlay_hdr_luminance * 80.0f ),
                      L"Epic Range" );
        });

        static ID3D10Blob* epic_blob_PQ =
          __SK_MakeEpicPS_Bytecode (true, false, config.platform.overlay_hdr_luminance * 80.0f);

        pShaderBytecode = epic_blob_PQ->GetBufferPointer ();
        BytecodeLength  = epic_blob_PQ->GetBufferSize    ();
      }
    }

#ifdef OVERLAY_DEBUG
    else if (type == sk_shader_class::Pixel)
    {
      SK_LOGs0 (L"DX12Shader", L"Pixel Shader: %x created by %ws", checksum, SK_GetCallerName ().c_str ());
    }
#endif

    return
      InvokeCreateRoutine ();
  }

  // In debug builds, keep a tally of threads involved in shader management.
  //
  //   (Per-frame and lifetime statistics are tabulated)
  //
  //  ---------------------------------------------------------------------
  //
  //  * D3D11 devices are free-threaded and will perform resource creation
  //      from concurrent threads.
  //
  //   >> Ideally, we must keep our own locking to a minimum otherwise we
  //        can derail game's performance! <<
  //
  SK_D3D11_ShaderThreads->mark ();

  HRESULT hr =
    S_OK;

  const auto GetResources =
  [&]( gsl::not_null <SK_Thread_CriticalSection**>                        ppCritical,
       gsl::not_null <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>**> ppShaderDomain ) noexcept
  {
    auto& shaders =
      SK_D3D11_Shaders;

    switch (type)
    {
      default:
      case sk_shader_class::Vertex:
        *ppCritical     = cs_shader_vs.get ();
        *ppShaderDomain =
          reinterpret_cast <
            SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*
                           > (&shaders->vertex);
         break;
      case sk_shader_class::Pixel:
        *ppCritical     = cs_shader_ps.get ();
        *ppShaderDomain =
          reinterpret_cast <
            SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*
                           > (&shaders->pixel);
         break;
      case sk_shader_class::Geometry:
        *ppCritical     = cs_shader_gs.get ();
        *ppShaderDomain =
          reinterpret_cast <
            SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*
                           > (&shaders->geometry);
         break;
      case sk_shader_class::Domain:
        *ppCritical     = cs_shader_ds.get ();
        *ppShaderDomain =
          reinterpret_cast <
            SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*
                           > (&shaders->domain);
         break;
      case sk_shader_class::Hull:
        *ppCritical     = cs_shader_hs.get ();
        *ppShaderDomain =
          reinterpret_cast <
            SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*
                           > (&shaders->hull);
         break;
      case sk_shader_class::Compute:
        *ppCritical     = cs_shader_cs.get ();
        *ppShaderDomain =
          reinterpret_cast <
            SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*
                           > (&shaders->compute);
         break;
    }
  };

  SK_Thread_CriticalSection*                        pCritical   = nullptr;
  SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>* pShaderRepo = nullptr;

  GetResources (
    &pCritical, &pShaderRepo
  );

  // Checksum failure, just give the data to D3D11 and hope for the best
  if (checksum == 0x00)
  {
    hr =
      InvokeCreateRoutine ();
  }

  else if (pCritical != nullptr)
  {
    pCritical->lock ();     // Lock during cache check

    const bool cached =
      pShaderRepo->descs [This].count (checksum) != 0;

    SK_D3D11_ShaderDesc* pCachedDesc = nullptr;

    if (! cached)
    {
      pCritical->unlock (); // Release lock during resource creation

      hr =
        InvokeCreateRoutine ();

      pCritical->lock ();   // Re-acquire before cache manipulation

      if (SUCCEEDED (hr))
      {
        SK_D3D11_ShaderDesc desc;

        switch (type)
        {
          default:
          case sk_shader_class::Vertex:
            (*ppShader)->QueryInterface <
                ID3D11VertexShader
            > ((ID3D11VertexShader **)
               &desc.pShader);
                desc.type =
           SK_D3D11_ShaderType::Vertex;  break;
          case sk_shader_class::Pixel:
            (*ppShader)->QueryInterface <
                ID3D11PixelShader
            > ((ID3D11PixelShader **)
               &desc.pShader);
                desc.type =
           SK_D3D11_ShaderType::Pixel;   break;
          case sk_shader_class::Geometry:
            (*ppShader)->QueryInterface <
                ID3D11GeometryShader
            > ((ID3D11GeometryShader **)
               &desc.pShader);
                desc.type =
           SK_D3D11_ShaderType::Geometry;break;
          case sk_shader_class::Domain:
            (*ppShader)->QueryInterface <
                ID3D11DomainShader
            > ((ID3D11DomainShader **)
               &desc.pShader);
                desc.type =
           SK_D3D11_ShaderType::Domain;  break;
          case sk_shader_class::Hull:
            (*ppShader)->QueryInterface <
                ID3D11HullShader
            > ((ID3D11HullShader **)
               &desc.pShader);
                desc.type =
           SK_D3D11_ShaderType::Hull;    break;
          case sk_shader_class::Compute:
            (*ppShader)->QueryInterface <
                ID3D11ComputeShader
            > ((ID3D11ComputeShader **)
               &desc.pShader);
                desc.type =
           SK_D3D11_ShaderType::Compute; break;
        }

        desc.pShader->Release ();

        desc.crc32c = checksum;

        // Concurrent shader creation resulted in the same shader twice,
        //   release this one and use the one currently in cache.
        //
        //  (nb: DOES NOT ACCOUNT FOR ALT. SUBROUTINES [Class Linkage])
        //
        if (pShaderRepo->descs [This].count (checksum) != 0)
        {
          SK_LOG1 ( (L"Discarding Concurrent Shader Cache Collision for %x",
                       checksum ), L"DX11Shader" );
        }

        else
        {
             *ppShader =
          desc.pShader;

          ((ID3D11DeviceChild *)desc.pShader)->SetPrivateData (
            SKID_D3D11KnownShaderCrc32c, sizeof (uint32_t), &checksum
          );

          pShaderRepo->descs [This][checksum] = desc;

          // Only store this data if there's a chance render mod tools could access it,
          //   otherwise it's wasting memory in 32-bit games.
          if (SK_Render_GetVulkanInteropSwapChainType (rb.swapchain) == SK_DXGI_VK_INTEROP_TYPE_NONE)
          {
            pShaderRepo->descs [This][checksum].bytecode.resize (BytecodeLength);
            
            std::copy ( &((const uint8_t *) pShaderBytecode) [0],
                        &((const uint8_t *) pShaderBytecode) [BytecodeLength],
                          pShaderRepo->descs [This][checksum].bytecode.begin () );
          }
        }

        pCachedDesc
          = &pShaderRepo->descs [This][checksum];
      }
    }

    // Cache hit
    else
    {
      //
      // NOTE:
      //
      //   Because of this caching system, we alias some render passes that
      //     could ordinarily be distinguished because they use two different
      //       instances of the same shader.
      //
      //  * Consider a tagging system to prevent aliasing in the future.
      //
      pCachedDesc =
        &pShaderRepo->descs [This][checksum];

      SK_LOG3 ( ( L"Shader Class %lu Cache Hit for Checksum %08x", type, checksum ),
                  L"DX11Shader" );
    }

    if ( pCachedDesc          != nullptr &&
         pCachedDesc->pShader != nullptr ){
         pCachedDesc->pShader->AddRef ();
       *ppShader =
         pCachedDesc->pShader;

      // Update cache mapping (see note about aliasing above)
      ///if ( pShaderRepo->rev.count (*ppShader) &&
      ///           pShaderRepo->rev [*ppShader] != checksum )
      ///  pShaderRepo->rev.erase (*ppShader);

      pShaderRepo->rev [This].emplace (
        std::make_pair (*ppShader, pCachedDesc)
      );
    }

    pCritical->unlock ();
  }

  return hr;
}


bool
SK_D3D11_IgnoreWrappedOrDeferred (
  bool                 bWrapped,
  bool                 bDeferred,
  ID3D11DeviceContext* pDevCtx
);

bool
SK_D3D11_ShouldTrackSetShaderResources (
  ID3D11DeviceContext* pDevCtx,
  UINT                 dev_idx = (UINT)-1
);

bool
SK_D3D11_ShouldTrackMMIO (
  ID3D11DeviceContext* pDevCtx,
  SK_TLS**             ppTLS = nullptr
);

bool
SK_D3D11_ShouldTrackRenderOp (
  ID3D11DeviceContext* pDevCtx,
  UINT                 dev_idx = (UINT)-1
);

bool
SK_D3D11_ShouldTrackDrawCall (
        ID3D11DeviceContext* pDevCtx,
  const SK_D3D11DrawType     draw_type,
        UINT                 dev_idx = (UINT)-1
);

bool
SK_D3D11_ShouldTrackComputeDispatch (
         ID3D11DeviceContext* pDevCtx,
  const  SK_D3D11DispatchType dispatch_type,
         UINT                 dev_idx
);

// All known targets are indexed using the calling device context,
//   no internal locking is necessary as long as dev ctx's are one per-thread.
struct SK_D3D11_KnownTargets
{
//#define _PERSIST_DS_VIEWS
  SK_D3D11_KnownTargets (void)
  {
    rt_views.reserve (256);
#ifdef _PERSIST_DS_VIEWS
    ds_views.reserve (128);
#endif
  }

  ~SK_D3D11_KnownTargets (void)
  {
    clear ();
  }

  void clear (void)
  {
    max_rt_views =
      std::max (max_rt_views, rt_views.max_size ());

    rt_views.clear ();

    if (rt_views.max_size () < max_rt_views)
        rt_views.reserve      (max_rt_views);

#ifdef _PERSIST_DS_VIEWS
    max_ds_views =
      std::max (max_ds_views, ds_views.size ());

    ds_views.clear ();

    if (ds_views.max_size () < max_ds_views)
        ds_views.reserve      (max_ds_views);
#endif
  }

  std::unordered_set <ID3D11RenderTargetView *> rt_views;
                                     size_t max_rt_views = 256;
#ifdef _PERSIST_DS_VIEWS
  std::unordered_set <ID3D11DepthStencilView *> ds_views;
                                     size_t max_ds_views = 128;
#endif

  static bool   _mod_tool_wants;
};

extern SK_LazyGlobal <std::array <SK_D3D11_KnownTargets,
  SK_D3D11_MAX_DEV_CONTEXTS + 1>> SK_D3D11_RenderTargets;

extern ID3D11Texture2D* SK_D3D11_TrackedTexture;
extern SK_LazyGlobal <
  target_tracking_s  >  tracked_rtv;
extern DWORD            tracked_tex_blink_duration;
extern DWORD            tracked_shader_blink_duration;


extern bool           SK_D3D11_EnableTracking;
extern bool           SK_D3D11_EnableMMIOTracking;
extern volatile LONG  SK_D3D11_DrawTrackingReqs;
extern volatile LONG  SK_D3D11_CBufferTrackingReqs;
extern volatile ULONG SK_D3D11_LiveTexturesDirty;

extern bool&          SK_D3D11_DontTrackUnlessModToolsAreOpen;


// Only accessed by the swapchain thread and only to clear any outstanding
//   references prior to a buffer resize
extern SK_LazyGlobal <std::vector <SK_ComPtr <ID3D11View>>> SK_D3D11_TempResources;

struct SK_D3D11_ShaderStageArray
{
  using array_s =
    std::array < shader_stage_s,
                   SK_D3D11_MAX_DEV_CONTEXTS + 1 >;

  array_s stage [6];

  inline array_s& operator [] (int idx) { return stage [idx]; }
};

extern SK_LazyGlobal <SK_D3D11_ShaderStageArray> d3d11_shader_stages;

extern SK_LazyGlobal <
      std::array < SK_D3D11_ContextResources,
                   SK_D3D11_MAX_DEV_CONTEXTS + 1 >
                 > SK_D3D11_PerCtxResources;

extern SK_LazyGlobal <std::array <bool, SK_D3D11_MAX_DEV_CONTEXTS+1>> reshade_trigger_before;
extern SK_LazyGlobal <std::array <bool, SK_D3D11_MAX_DEV_CONTEXTS+1>> reshade_trigger_after;

extern SK_LazyGlobal <memory_tracking_s> mem_map_stats;
extern SK_LazyGlobal <target_tracking_s> tracked_rtv;

void
SK_D3D11_SetShader_Impl ( ID3D11DeviceContext*        pDevCtx,
                          IUnknown*                   pShader,
                          sk_shader_class             type,
                          ID3D11ClassInstance *const *ppClassInstances,
                          UINT                        NumClassInstances,
                          bool                        Wrapped = false,
                          UINT                        dev_idx = UINT_MAX );