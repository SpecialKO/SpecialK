// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
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

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"  D3D 11  "

#include <SpecialK/render/d3d11/d3d11_core.h>
#include <SpecialK/render/d3d11/d3d11_tex_mgr.h>
#include <SpecialK/render/d3d11/utility/d3d11_texture.h>
#include <SpecialK/render/d3d11/d3d11_state_tracker.h>
#include <SpecialK/control_panel/d3d11.h>

extern
SK_LazyGlobal <
  std::unordered_map < ID3D11DeviceContext *,
                       mapped_resources_s  >
              >        mapped_resources;

SK_LazyGlobal < memory_tracking_s >
  mem_map_stats;

HRESULT
STDMETHODCALLTYPE
SK_D3D11_Map_Impl (
  _In_      ID3D11DeviceContext      *pDevCtx,
  _In_      ID3D11Resource           *pResource,
  _In_      UINT                      Subresource,
  _In_      D3D11_MAP                 MapType,
  _In_      UINT                      MapFlags,
  _Out_opt_ D3D11_MAPPED_SUBRESOURCE *pMappedResource,
            BOOL                      bWrapped )
{
  SK_WRAP_AND_HOOK

  D3D11_RESOURCE_DIMENSION rdim =
  D3D11_RESOURCE_DIMENSION_UNKNOWN;

  if (Subresource == 0 && pResource != nullptr)
  {
    pResource->GetType (&rdim);

    if (SK_D3D11_IsStagingCacheable (rdim, pResource))
    {
      MapType = D3D11_MAP_READ_WRITE;
    }
  }

  const HRESULT hr =
    bWrapped ?
      pDevCtx->Map ( pResource, Subresource,
                       MapType, MapFlags, pMappedResource )
             :
      D3D11_Map_Original ( pDevCtx, pResource, Subresource,
                             MapType, MapFlags, pMappedResource );

  const bool early_out =
    (! bMustNotIgnore);

  if (early_out)
  {
    return
      hr;
  }

  // UB: If it's happening, pretend we never saw this...
  if (pResource == nullptr || bIsDevCtxDeferred )
  {
    assert (pResource != nullptr);

    return hr;
  }

  // ImGui gets to pass-through without invoking the hook
  if (! config.textures.cache.allow_staging)
  {
    const bool track =
      ( SK_D3D11_ShouldTrackMMIO (pDevCtx) && SK_D3D11_ShouldTrackRenderOp (pDevCtx) );

    if (! track)
      return hr;
  }

  if (SK_D3D11_IgnoreWrappedOrDeferred (bWrapped, bIsDevCtxDeferred, pDevCtx))
  {
    return
      hr;
  }

  if (SUCCEEDED (hr))
  {
    SK_D3D11_MemoryThreads->mark ();

    if (Subresource == 0 && SK_D3D11_IsStagingCacheable (rdim, pResource))
    {
      SK_ComQIPtr <ID3D11Texture2D> pTex (pResource);

                      D3D11_TEXTURE2D_DESC    d3d11_desc = { };
      pTex->GetDesc ((D3D11_TEXTURE2D_DESC *)&d3d11_desc);

      const SK_D3D11_TEXTURE2D_DESC desc (d3d11_desc);

      //dll_log->Log ( L"Staging Map: Type=%x, BindFlags: %s, Res: %lux%lu",
      //                 MapType, SK_D3D11_DescribeBindFlags (desc.BindFlags).c_str (),
      //        desc.Width, desc.Height, SK_DXGI_FormatToStr (desc.Format).c_str    () );

      if (MapType == D3D11_MAP_WRITE_DISCARD)
      {
        auto& textures =
          SK_D3D11_Textures;

        auto it  = textures->Textures_2D.find (pTex);
        if ( it != textures->Textures_2D.cend (    ) && it->second.crc32c != 0x00 )
        {
                                                                             it->second.discard = true;
          pTex->SetPrivateData (SKID_D3D11Texture2D_DISCARD, sizeof (bool), &it->second.discard);

          SK_D3D11_RemoveTexFromCache (pTex, true);

          bool use_l3 =
            config.textures.d3d11.use_l3_hash;

          if (use_l3) textures->HashMap_Fmt [it->second.orig_desc.Format].map [it->second.orig_desc.MipLevels].erase (it->second.tag);
          else        textures->HashMap_2D                                    [it->second.orig_desc.MipLevels].erase (it->second.tag);

          SK_LOG4 ( ( L"Removing discarded texture from cache (it has been memory-mapped as discard)." ),
                      L"DX11TexMgr" );
        }
      }

      UINT  private_size = sizeof (bool);
      bool  private_data = false;

      bool discard = false;

      if (SUCCEEDED (pTex->GetPrivateData (SKID_D3D11Texture2D_DISCARD, &private_size, &private_data)) && private_size == sizeof (bool))
      {
        discard = private_data;
      }

      if ((desc.BindFlags & D3D11_BIND_SHADER_RESOURCE ) || desc.Usage == D3D11_USAGE_STAGING)
      {
        if (! discard)
        {
          // Keep cached, but don't update -- it is now discarded and we should ignore it
          if (SK_D3D11_TextureIsCached (pTex))
          {
            auto& texDesc =
              SK_D3D11_Textures->Textures_2D [pTex];
                                                                               texDesc.discard = true;
            pTex->SetPrivateData (SKID_D3D11Texture2D_DISCARD, sizeof (bool), &texDesc.discard);

            SK_LOG1 ( ( L"Removing discarded texture from cache (it has been memory-mapped multiple times)." ),
                        L"DX11TexMgr" );
            SK_D3D11_RemoveTexFromCache (pTex, true);
          }

          else if (pMappedResource != nullptr)
          {
            std::scoped_lock <SK_Thread_CriticalSection> auto_lock (*cs_mmio);

            auto& map_ctx = (*mapped_resources)[pDevCtx];

            map_ctx.textures.try_emplace      (pResource, *pMappedResource);
            map_ctx.texture_times.try_emplace (pResource, SK_QueryPerf ().QuadPart);

            //dll_log->Log (L"[DX11TexMgr] Mapped 2D texture...");
          }
        }

        else
        {
          if (SK_D3D11_TextureIsCached (pTex))
          {
            SK_LOG1 ( ( L"Removing discarded texture from cache." ),
                        L"DX11TexMgr" );
            SK_D3D11_RemoveTexFromCache (pTex, true);
          }
          //dll_log->Log (L"[DX11TexMgr] Skipped 2D texture...");
        }
      }
    }

    if (! SK_D3D11_EnableMMIOTracking)
      return hr;

    const bool read =  ( MapType == D3D11_MAP_READ      ||
                         MapType == D3D11_MAP_READ_WRITE );

    const bool write = ( MapType == D3D11_MAP_WRITE             ||
                         MapType == D3D11_MAP_WRITE_DISCARD     ||
                         MapType == D3D11_MAP_READ_WRITE        ||
                         MapType == D3D11_MAP_WRITE_NO_OVERWRITE );

    mem_map_stats->last_frame.map_types [MapType-1]++;

    if (rdim == D3D11_RESOURCE_DIMENSION_UNKNOWN)
      pResource->GetType (&rdim);

    switch (rdim)
    {
      case D3D11_RESOURCE_DIMENSION_UNKNOWN:
        mem_map_stats->last_frame.resource_types [D3D11_RESOURCE_DIMENSION_UNKNOWN]++;
        break;
      case D3D11_RESOURCE_DIMENSION_BUFFER:
      {
        mem_map_stats->last_frame.resource_types [D3D11_RESOURCE_DIMENSION_BUFFER]++;

        ID3D11Buffer* pBuffer =
          static_cast <ID3D11Buffer *> (pResource);

        //if (SUCCEEDED (pResource->QueryInterface <ID3D11Buffer> (&pBuffer)))
        {
          D3D11_BUFFER_DESC  buf_desc = { };
          pBuffer->GetDesc (&buf_desc);
          {
            ///std::scoped_lock <SK_Thread_CriticalSection> auto_lock (cs_mmio);

            // Extra memory allocation pressure for no good reason -- kill it.
            //
            if (buf_desc.BindFlags & D3D11_BIND_INDEX_BUFFER)
              mem_map_stats->last_frame.buffer_types [0]++;
            ////  mem_map_stats->last_frame.index_buffers.insert (pBuffer);
            ////
            if (buf_desc.BindFlags & D3D11_BIND_VERTEX_BUFFER)
              mem_map_stats->last_frame.buffer_types [1]++;
            ////  mem_map_stats->last_frame.vertex_buffers.insert (pBuffer);
            ////
            if (buf_desc.BindFlags & D3D11_BIND_CONSTANT_BUFFER)
              mem_map_stats->last_frame.buffer_types [2]++;
            ////  mem_map_stats->last_frame.constant_buffers.insert (pBuffer);
          }

          if (read)
            mem_map_stats->last_frame.bytes_read    += buf_desc.ByteWidth;

          if (write)
            mem_map_stats->last_frame.bytes_written += buf_desc.ByteWidth;
        }
      } break;
      case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
        mem_map_stats->last_frame.resource_types [D3D11_RESOURCE_DIMENSION_TEXTURE1D]++;
        break;
      case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
        mem_map_stats->last_frame.resource_types [D3D11_RESOURCE_DIMENSION_TEXTURE2D]++;
        break;
      case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
        mem_map_stats->last_frame.resource_types [D3D11_RESOURCE_DIMENSION_TEXTURE3D]++;
        break;
    }
  }

  return hr;
}

void
STDMETHODCALLTYPE
SK_D3D11_Unmap_Impl (
  _In_ ID3D11DeviceContext *pDevCtx,
  _In_ ID3D11Resource      *pResource,
  _In_ UINT                 Subresource,
       BOOL                 bWrapped )
{
  SK_WRAP_AND_HOOK

  auto _Finish = [&](void) ->
  void
  {
    return
      bWrapped ?
        pDevCtx->Unmap (pResource, Subresource)
               :
        D3D11_Unmap_Original (pDevCtx, pResource, Subresource);
  };

  bool early_out =
    (! bMustNotIgnore);

  if (early_out)
  {
    return
      _Finish ();
  }

  // UB: If it's happening, pretend we never saw this...
  if (pResource == nullptr || bIsDevCtxDeferred)
  {
    assert (pResource != nullptr);

    if (pResource == nullptr)
      return;

    return
      _Finish ();
  }

  SK_TLS *pTLS = nullptr;

  // ImGui gets to pass-through without invoking the hook
  if (config.textures.d3d11.cache && (! config.textures.cache.allow_staging))
  {
    const bool track =
      ( SK_D3D11_ShouldTrackMMIO (pDevCtx) && SK_D3D11_ShouldTrackRenderOp (pDevCtx) );

    if (! track)
    {
      return
        _Finish ();
    }
  }

  if (SK_D3D11_IgnoreWrappedOrDeferred (bWrapped, bIsDevCtxDeferred, pDevCtx))
  {
    return
      _Finish ();
  }

  SK_D3D11_MemoryThreads->mark ();

  D3D11_RESOURCE_DIMENSION rdim =
  D3D11_RESOURCE_DIMENSION_UNKNOWN;

  if (Subresource == 0)
  {
    pResource->GetType (&rdim);

    if (SK_D3D11_IsStagingCacheable (rdim, pResource))
    {
      std::scoped_lock <SK_Thread_CriticalSection> auto_lock (*cs_mmio);

      auto& map_ctx = (*mapped_resources)[pDevCtx];

      // More of an assertion, if this fails something's screwy!
      if (map_ctx.textures.count (pResource))
      {
        uint64_t time_elapsed =
          SK_QueryPerf ().QuadPart - map_ctx.texture_times [pResource];

        SK_ComQIPtr <ID3D11Texture2D> pTex (pResource);

        D3D11_TEXTURE2D_DESC desc = { };
             pTex->GetDesc (&desc);

        if ( (desc.BindFlags & D3D11_BIND_SHADER_RESOURCE) ||
              desc.Usage    == D3D11_USAGE_STAGING )
        {
          uint32_t checksum  = 0;
          size_t   size      = 0;
          uint32_t top_crc32 = 0x00;

          int levels = desc.MipLevels;

          desc.MipLevels = 1;

          checksum =
            crc32_tex ( &desc,
              reinterpret_cast <D3D11_SUBRESOURCE_DATA *>(
                          &map_ctx.textures [pResource]
                       ), &size, &top_crc32 );

          //dll_log->Log (L"[DX11TexMgr] Mapped 2D texture... (%x -- %lu bytes)", checksum, size);

          if (checksum != 0x0)
          {
            auto& textures =
              SK_D3D11_Textures;

            // Temp hack for 1-LOD Staging Texture Uploads
            bool injectable = (
              desc.Usage == D3D11_USAGE_STAGING &&
              levels     ==  1                  &&
               ( SK_D3D11_IsInjectable (top_crc32, checksum) ||
                 SK_D3D11_IsInjectable (top_crc32, 0x00)
               )
            );

            std::wstring filename;

            if (injectable)
            {
              if (! SK_D3D11_res_root->empty ())
              {
                wchar_t     wszTex [MAX_PATH + 2] = { };
                wcsncpy_s ( wszTex, MAX_PATH,
                      SK_D3D11_TexNameFromChecksum (
                               top_crc32, checksum,
                                               0x0 ).c_str (),
                                   _TRUNCATE );

                if (PathFileExistsW (wszTex))
                {
                  HRESULT hr = E_UNEXPECTED;

                  DirectX::TexMetadata  mdata;
                  DirectX::ScratchImage img;

                  if ( SUCCEEDED ((hr = DirectX::GetMetadataFromDDSFile (wszTex, 0,  mdata     ))) &&
                       SUCCEEDED ((hr = DirectX::LoadFromDDSFile        (wszTex, 0, &mdata, img))) )
                  {
                    SK_ComPtr <ID3D11Texture2D> pOverrideTex;
                    SK_ComPtr <ID3D11Device>    pDevice;
                    pDevCtx->GetDevice        (&pDevice.p);

                      pTLS =
                    SK_TLS_Bottom ();

                    SK_ScopedBool decl_tex_scope (
                      SK_D3D11_DeclareTexInjectScope (pTLS)
                    );

                    if ( SUCCEEDED ((hr = DirectX::CreateTexture (pDevice.p,
                                                                  img.GetImages     (),
                                                                  img.GetImageCount (), mdata,
                            reinterpret_cast <ID3D11Resource**> (&pOverrideTex.p)))) )
                    {
                      D3D11_TEXTURE2D_DESC    new_desc = { };
                      pOverrideTex->GetDesc (&new_desc);

                      if (size != SK_D3D11_ComputeTextureSize (&new_desc))
                      {
                        SK_LOG0 ( ( L"Unexpected size for mapped texture, %d bytes (Expected: %d) ",
                                    size, SK_D3D11_ComputeTextureSize (&new_desc) ),
                                   L"StagingTex" );
                      }
                      //SK_ReleaseAssert (
                      //  size == SK_D3D11_ComputeTextureSize (&new_desc)
                      //);

                      std::scoped_lock <SK_Thread_HybridSpinlock>
                            scope_lock (*cache_cs);

                      pDevCtx->CopyResource (
                                  pResource,
                               pOverrideTex );

                      const ULONGLONG load_end =
                        (ULONGLONG)SK_QueryPerf ().QuadPart;

                      desc.MipLevels = levels;

                      const uint32_t cache_tag =
                        safe_crc32c (top_crc32, (uint8_t*)(&desc), sizeof (D3D11_TEXTURE2D_DESC));

                      time_elapsed = load_end - map_ctx.texture_times [pResource];
                      filename     = wszTex;

                      SK_LOG0 ( ( L" *** Texture Injected Early... %x :: %x  { %ws }",
                                                          top_crc32, cache_tag, wszTex ),
                                  L"StagingTex" );
                    }
                  }
                }
              }
            }

            if (desc.Usage != D3D11_USAGE_STAGING)
            {
              desc.MipLevels = levels;

              const uint32_t cache_tag    =
                safe_crc32c (top_crc32, (uint8_t *)(&desc), sizeof (D3D11_TEXTURE2D_DESC));

              textures->CacheMisses_2D++;

              if (! pTLS)
                    pTLS =
                  SK_TLS_Bottom ();

              textures->refTexture2D ( pTex,
                                        &desc,
                                          cache_tag,
                                            size,
                                              time_elapsed,
                                                top_crc32,
                                           L"", nullptr, (HMODULE)(intptr_t)-1/*SK_GetCallingDLL ()*/,
                                                  pTLS );
            }

            else
            {
              map_ctx.dynamic_textures  [pResource] = checksum;
              map_ctx.dynamic_texturesx [pResource] = top_crc32;

              map_ctx.dynamic_times2    [checksum]  = time_elapsed;
              map_ctx.dynamic_sizes2    [checksum]  = size;
              map_ctx.dynamic_files2    [checksum]  = std::move (filename);
            }
          }
        }

        map_ctx.textures.erase      (pResource);
        map_ctx.texture_times.erase (pResource);
      }
    }
  }

  _Finish ();
}