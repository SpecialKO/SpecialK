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

// For really wacky engines that wrap shader interfaces w/o correct layering
static const GUID SKID_D3D11KnownShaderCrc32c =
// {5C5298BB-0F9D-5022-A19D-A2E69792AE14}
  { 0x5c5298bb, 0xf9d,  0x5022, { 0xa1, 0x9d, 0xa2, 0xe6, 0x97, 0x92, 0xae, 0x14 } };

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


enum class sk_shader_class {
  Unknown  = 0x00,
  Vertex   = 0x01,
  Pixel    = 0x02,
  Geometry = 0x04,
  Hull     = 0x08,
  Domain   = 0x10,
  Compute  = 0x20
};

struct SK_D3D11_ContextResources
{
  std::unordered_set <ID3D11Texture2D *> used_textures;
  std::unordered_set <IUnknown        *> temp_resources;

  volatile LONG                writing_ = 0;
  UINT                         ctx_id_  = 0;
};


struct memory_tracking_s
{
  static const int __types = 5;

  memory_tracking_s (void)  : cs (cs_mmio)
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
      ///std::lock_guard <SK_Thread_CriticalSection> auto_lock (*cs);

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

  SK_Thread_HybridSpinlock*& cs;
} extern mem_map_stats;

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

  volatile ID3D11RenderTargetView*       resource     =  reinterpret_cast <ID3D11RenderTargetView *>(INTPTR_MAX);

  std::atomic_bool                       active [SK_D3D11_MAX_DEV_CONTEXTS+1][D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT]
    = { };
  std::atomic <uint32_t>                 active_count [SK_D3D11_MAX_DEV_CONTEXTS+1]
    = { };

  std::atomic <uint32_t>                 num_draws    =     0;

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
} extern tracked_rtv;

struct SK_D3D11_KnownThreads
{
  SK_D3D11_KnownThreads (void) ///
  {
    InitializeCriticalSectionEx (&cs, 0x400, RTL_CRITICAL_SECTION_FLAG_DYNAMIC_SPIN | SK_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);

    //ids.reserve    (16);
    //active.reserve (16);
  }

  void   clear_all    (void);
  size_t count_all    (void);

  void   clear_active (void)
  {
    if (use_lock)
    {
      SK_AutoCriticalSection auto_cs (&cs);
      active.clear ();
      return;
    }

    active.clear ();
  }

  size_t count_active (void)
  {
    if (use_lock)
    {
      SK_AutoCriticalSection auto_cs (&cs);
      return active.size ();
    }

    return active.size ();
  }

  float  active_ratio (void)
  {
    if (use_lock)
    {
      SK_AutoCriticalSection auto_cs (&cs);
      return static_cast <float> (active.size ())/
        static_cast <float> (ids.size    ());
    }

    return static_cast <float> (active.size ())/
      static_cast <float> (ids.size    ());
  }

  static void mark (void) ;

private:
  std::unordered_set <DWORD> ids;
  std::unordered_set <DWORD> active;
  CRITICAL_SECTION           cs;
  bool             use_lock = true;
};

extern SK_D3D11_KnownThreads SK_D3D11_MemoryThreads;
extern SK_D3D11_KnownThreads SK_D3D11_DrawThreads;
extern SK_D3D11_KnownThreads SK_D3D11_DispatchThreads;
extern SK_D3D11_KnownThreads SK_D3D11_ShaderThreads;


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
    return InvokeCreateRoutine ();

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
  SK_D3D11_ShaderThreads.mark ();

  HRESULT hr =
    S_OK;

  const auto GetResources =
  [&]( SK_Thread_CriticalSection**                        ppCritical,
       SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>** ppShaderDomain )
  {
    static auto& shaders =
      SK_D3D11_Shaders;

    switch (type)
    {
      default:
      case sk_shader_class::Vertex:
        *ppCritical     = cs_shader_vs;
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &shaders.vertex
                          );
         break;
      case sk_shader_class::Pixel:
        *ppCritical     = cs_shader_ps;
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &shaders.pixel
                          );
         break;
      case sk_shader_class::Geometry:
        *ppCritical     = cs_shader_gs;
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &shaders.geometry
                          );
         break;
      case sk_shader_class::Domain:
        *ppCritical     = cs_shader_ds;
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &shaders.domain
                          );
         break;
      case sk_shader_class::Hull:
        *ppCritical     = cs_shader_hs;
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &shaders.hull
                          );
         break;
      case sk_shader_class::Compute:
        *ppCritical     = cs_shader_cs;
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &shaders.compute
                          );
         break;
    }
  };

  SK_Thread_CriticalSection*                        pCritical   = nullptr;
  SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>* pShaderRepo = nullptr;

  GetResources (&pCritical, &pShaderRepo);

 
  uint32_t checksum =
    SK_D3D11_ChecksumShaderBytecode (pShaderBytecode, BytecodeLength);

  // Checksum failure, just give the data to D3D11 and hope for the best
  if (checksum == 0x00)
  {
    hr = InvokeCreateRoutine ();
  }

  else
  {
    pCritical->lock ();     // Lock during cache check

    const bool cached =
      pShaderRepo->descs.count (checksum) != 0;

    SK_D3D11_ShaderDesc* pCachedDesc = nullptr;

    if (! cached)
    {
      pCritical->unlock (); // Release lock during resource creation

      hr = InvokeCreateRoutine ();

      if (SUCCEEDED (hr))
      {
        SK_D3D11_ShaderDesc desc;

        switch (type)
        {
          default:
          case sk_shader_class::Vertex:   desc.type = SK_D3D11_ShaderType::Vertex;   break;
          case sk_shader_class::Pixel:    desc.type = SK_D3D11_ShaderType::Pixel;    break;
          case sk_shader_class::Geometry: desc.type = SK_D3D11_ShaderType::Geometry; break;
          case sk_shader_class::Domain:   desc.type = SK_D3D11_ShaderType::Domain;   break;
          case sk_shader_class::Hull:     desc.type = SK_D3D11_ShaderType::Hull;     break;
          case sk_shader_class::Compute:  desc.type = SK_D3D11_ShaderType::Compute;  break;
        }

        desc.crc32c  =  checksum;
        desc.pShader = *ppShader;

      desc.bytecode.insert ( desc.bytecode.end  (),
        &((uint8_t *) pShaderBytecode) [0],
        &((uint8_t *) pShaderBytecode) [BytecodeLength]
      );

        pCritical->lock (); // Re-acquire before cache manipulation

        // Concurrent shader creation resulted in the same shader twice,
        //   release this one and use the one currently in cache.
        //
        //  (nb: DOES NOT ACCOUNT FOR ALT. SUBROUTINES [Class Linkage])
        //
        if (pShaderRepo->descs.count (checksum) != 0)
        {
          ((IUnknown *)*ppShader)->Release ();

          SK_LOG0 ( (L"Discarding Concurrent Shader Cache Collision for %x",
                       checksum ), L"DX11Shader" );
        }

        else
        {
          ((ID3D11DeviceChild *)*ppShader)->SetPrivateData (
            SKID_D3D11KnownShaderCrc32c, sizeof (uint32_t), &checksum
          );

          pShaderRepo->descs.emplace (std::make_pair (checksum, desc));
        }

        pCachedDesc = &pShaderRepo->descs [checksum];
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
      pCachedDesc = &pShaderRepo->descs [checksum];

      SK_LOG3 ( ( L"Shader Class %lu Cache Hit for Checksum %08x", type, checksum ),
                  L"DX11Shader" );
    }

    if (pCachedDesc != nullptr)
    {
       *ppShader = (IUnknown *)pCachedDesc->pShader;
      (*ppShader)->AddRef ();

      // XXX: Creation does not imply usage
      //
      //InterlockedExchange (&pCachedDesc->usage.last_frame, SK_GetFramesDrawn ());
      //            _time64 (&pCachedDesc->usage.last_time);

      // Update cache mapping (see note about aliasing above)
      ///if ( pShaderRepo->rev.count (*ppShader) &&
      ///           pShaderRepo->rev [*ppShader] != checksum )
      ///  pShaderRepo->rev.erase (*ppShader);

      pShaderRepo->rev.emplace (std::make_pair (*ppShader, checksum));
    }

    pCritical->unlock ();
  }

  return hr;
}


bool
SK_D3D11_ShouldTrackSetShaderResources ( ID3D11DeviceContext* pDevCtx,
                                         SK_TLS**             ppTLS = nullptr );

bool
SK_D3D11_ShouldTrackMMIO ( ID3D11DeviceContext* pDevCtx,
                           SK_TLS**             ppTLS = nullptr );
bool
SK_D3D11_ShouldTrackRenderOp ( ID3D11DeviceContext* pDevCtx,
                               SK_TLS**             ppTLS = nullptr );

bool
SK_D3D11_ShouldTrackDrawCall (       ID3D11DeviceContext* pDevCtx,
                               const SK_D3D11DrawType     draw_type,
                                     SK_TLS**             ppTLS = nullptr );


struct SK_D3D11_KnownTargets
{
  std::unordered_set <ID3D11RenderTargetView *> rt_views;
  std::unordered_set <ID3D11DepthStencilView *> ds_views;

  SK_D3D11_KnownTargets (void)
  {
    rt_views.reserve (128);
    ds_views.reserve ( 64);
  }

  void clear (void)
  {
    std::lock_guard <SK_Thread_CriticalSection> auto_lock (*cs_render_view);

    for (auto it : rt_views)
    {
      if (it != nullptr)
        it->Release ();
    }

    for (auto it : ds_views)
    {
      if (it != nullptr)
        it->Release ();
    }

    rt_views.clear ();
    ds_views.clear ();
  }
};

extern std::array <SK_D3D11_KnownTargets, SK_D3D11_MAX_DEV_CONTEXTS + 1>
                        SK_D3D11_RenderTargets;
extern ID3D11Texture2D* SK_D3D11_TrackedTexture;
extern DWORD            tracked_tex_blink_duration;
extern DWORD            tracked_shader_blink_duration;


extern bool           SK_D3D11_EnableTracking;
extern bool           SK_D3D11_EnableMMIOTracking;
extern volatile LONG  SK_D3D11_DrawTrackingReqs;
extern volatile LONG  SK_D3D11_CBufferTrackingReqs;
extern volatile ULONG SK_D3D11_LiveTexturesDirty;


// Only accessed by the swapchain thread and only to clear any outstanding
//   references prior to a buffer resize
extern std::vector <IUnknown *> SK_D3D11_TempResources;