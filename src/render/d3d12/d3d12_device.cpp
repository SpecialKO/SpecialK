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
#define __SK_SUBSYSTEM__ L"DX12Device"

#include <SpecialK/render/d3d12/d3d12_pipeline_library.h>
#include <SpecialK/render/d3d12/d3d12_command_queue.h>
#include <SpecialK/render/d3d12/d3d12_dxil_shader.h>
#include <SpecialK/render/d3d12/d3d12_device.h>

#include <DirectXTex/d3dx12.h>

// Various device-resource hacks are here for HDR
#include <SpecialK/render/dxgi/dxgi_hdr.h>

#include <concurrent_unordered_map.h>
#include <concurrent_unordered_set.h>

extern volatile LONG  __d3d12_hooked;
LPVOID pfnD3D12CreateDevice = nullptr;

D3D12Device_CreateGraphicsPipelineState_pfn
D3D12Device_CreateGraphicsPipelineState_Original = nullptr;
D3D12Device2_CreatePipelineState_pfn
D3D12Device2_CreatePipelineState_Original        = nullptr;
D3D12Device_CreateRenderTargetView_pfn
D3D12Device_CreateRenderTargetView_Original      = nullptr;
D3D12Device_GetResourceAllocationInfo_pfn
D3D12Device_GetResourceAllocationInfo_Original   = nullptr;
D3D12Device_CreateCommittedResource_pfn
D3D12Device_CreateCommittedResource_Original     = nullptr;
D3D12Device_CreatePlacedResource_pfn
D3D12Device_CreatePlacedResource_Original        = nullptr;
D3D12Device_CreateCommandAllocator_pfn
D3D12Device_CreateCommandAllocator_Original      = nullptr;

                                                                                                  // {4D5298CA-D9F0-6133-A19D-B1D597920000}
static constexpr GUID SKID_D3D12KnownVtxShaderDigest = { 0x4d5298ca, 0xd9f0,  0x6133, { 0xa1, 0x9d, 0xb1, 0xd5, 0x97, 0x92, 0x00, 0x00 } };
static constexpr GUID SKID_D3D12KnownPixShaderDigest = { 0x4d5298ca, 0xd9f0,  0x6133, { 0xa1, 0x9d, 0xb1, 0xd5, 0x97, 0x92, 0x00, 0x01 } };
static constexpr GUID SKID_D3D12KnownGeoShaderDigest = { 0x4d5298ca, 0xd9f0,  0x6133, { 0xa1, 0x9d, 0xb1, 0xd5, 0x97, 0x92, 0x00, 0x02 } };
static constexpr GUID SKID_D3D12KnownHulShaderDigest = { 0x4d5298ca, 0xd9f0,  0x6133, { 0xa1, 0x9d, 0xb1, 0xd5, 0x97, 0x92, 0x00, 0x03 } };
static constexpr GUID SKID_D3D12KnownDomShaderDigest = { 0x4d5298ca, 0xd9f0,  0x6133, { 0xa1, 0x9d, 0xb1, 0xd5, 0x97, 0x92, 0x00, 0x04 } };
static constexpr GUID SKID_D3D12KnownComShaderDigest = { 0x4d5298ca, 0xd9f0,  0x6133, { 0xa1, 0x9d, 0xb1, 0xd5, 0x97, 0x92, 0x00, 0x05 } };
static constexpr GUID SKID_D3D12KnownMshShaderDigest = { 0x4d5298ca, 0xd9f0,  0x6133, { 0xa1, 0x9d, 0xb1, 0xd5, 0x97, 0x92, 0x00, 0x06 } };
static constexpr GUID SKID_D3D12KnownAmpShaderDigest = { 0x4d5298ca, 0xd9f0,  0x6133, { 0xa1, 0x9d, 0xb1, 0xd5, 0x97, 0x92, 0x00, 0x07 } };

static constexpr GUID SKID_D3D12DisablePipelineState = { 0x3d5298cb, 0xd9f0,  0x6133, { 0xa1, 0x9d, 0xb1, 0xd5, 0x97, 0x92, 0x00, 0x70 } };

concurrency::concurrent_unordered_set <ID3D12PipelineState*> _criticalVertexShaders;
concurrency::concurrent_unordered_map <ID3D12PipelineState*, bool>   _vertexShaders;
concurrency::concurrent_unordered_map <ID3D12PipelineState*, bool>    _pixelShaders;

void
__stdcall
SK_D3D12_PipelineDrainPlug (void *pBpOil) // Don't let it spill
{
  SK_LOG0 (
    ( L"ID3D12PipelineState (%p) Destroyed", pBpOil ),
      __SK_SUBSYSTEM__ );

  if (_vertexShaders.count ((ID3D12PipelineState *)pBpOil))
      _vertexShaders [      (ID3D12PipelineState *)pBpOil] = false;

  if (_pixelShaders.count ((ID3D12PipelineState *)pBpOil))
      _pixelShaders [      (ID3D12PipelineState *)pBpOil]  = false;
}

struct SK_D3D12_ShaderRepo
{
  struct {
    const char*         name;
    SK_D3D12_ShaderType mask;
  } type;

  struct hash_s {
    struct dxilHashTest
    {
      // std::hash <DxilContainerHash>
      //
      size_t operator ()( const DxilContainerHash& h ) const
      {
        size_t      __h = 0;
        for (size_t __i = 0; __i < DxilContainerHashSize; ++__i)
        {
          __h = h.Digest [__i] +
                  (__h << 06)  +  (__h << 16)
                               -   __h;
        }

        return __h;
      }

      // std::equal_to <DxilContainerHash>
      //
      bool operator ()( const DxilContainerHash& h1,
                        const DxilContainerHash& h2 ) const
      {
        return
          ( 0 == memcmp ( h1.Digest,
                          h2.Digest, DxilContainerHashSize ) );
      }
    };

    concurrency::concurrent_unordered_set <DxilContainerHash, dxilHashTest, dxilHashTest>
          used;
    GUID  guid;

    hash_s (const GUID& guid_)
    {   memcpy ( &guid,&guid_, sizeof (GUID) );
    };
  } hash;
};


static volatile LONG
  __SK_HUD_YesOrNo = 1L;

bool
SK_D3D12_ShouldSkipHUD (void)
{
  bool ret =
    ( SK_Screenshot_IsCapturingHUDless () ||
       ReadAcquire    (&__SK_HUD_YesOrNo) <= 0 );
  if ( ReadAcquire    (&__SK_HUD_YesOrNo) <= 0 )
  {
    UINT size    = 1;
    bool disable = true;
    
    for ( auto &[ps,live] : _vertexShaders )
    {
      if (live && (! _criticalVertexShaders.count (ps)))
          ps->SetPrivateData ( SKID_D3D12DisablePipelineState, size, &disable );
    }
  }

  return ret;
}

LONG
SK_D3D12_ShowGameHUD (void)
{
  //InterlockedDecrement (&SK_D3D11_DrawTrackingReqs);

  UINT size    = 1;
  bool disable = false;
  
  for ( auto &[ps,live] : _vertexShaders )
  {
    if (live)
      ps->SetPrivateData ( SKID_D3D12DisablePipelineState, size, &disable );
  }

  return
    InterlockedIncrement (&__SK_HUD_YesOrNo);
}

LONG
SK_D3D12_HideGameHUD (void)
{
  //InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);

  UINT size    = 1;
  bool disable = true;
  
  for ( auto &[ps,live] : _vertexShaders )
  {
    if (live && (! _criticalVertexShaders.count (ps)))
      ps->SetPrivateData ( SKID_D3D12DisablePipelineState, size, &disable );
  }

  return
    InterlockedDecrement (&__SK_HUD_YesOrNo);
}

LONG
SK_D3D12_ToggleGameHUD (void)
{
  static volatile LONG last_state =
    (ReadAcquire (&__SK_HUD_YesOrNo) > 0);

  if (ReadAcquire (&last_state))
  {
    SK_D3D12_HideGameHUD ();

    return
      InterlockedDecrement (&last_state);
  }

  SK_D3D12_ShowGameHUD ();

  return
    InterlockedIncrement (&last_state);
}


static SK_D3D12_ShaderRepo
  vertex   { { "Vertex",   SK_D3D12_ShaderType::Vertex   }, { SK_D3D12_ShaderRepo::hash_s (SKID_D3D12KnownVtxShaderDigest) } },
  pixel    { { "Pixel",    SK_D3D12_ShaderType::Pixel    }, { SK_D3D12_ShaderRepo::hash_s (SKID_D3D12KnownPixShaderDigest) } },
  geometry { { "Geometry", SK_D3D12_ShaderType::Geometry }, { SK_D3D12_ShaderRepo::hash_s (SKID_D3D12KnownGeoShaderDigest) } },
  hull     { { "Hull",     SK_D3D12_ShaderType::Hull     }, { SK_D3D12_ShaderRepo::hash_s (SKID_D3D12KnownHulShaderDigest) } },
  domain   { { "Domain",   SK_D3D12_ShaderType::Domain   }, { SK_D3D12_ShaderRepo::hash_s (SKID_D3D12KnownDomShaderDigest) } },
  compute  { { "Compute",  SK_D3D12_ShaderType::Compute  }, { SK_D3D12_ShaderRepo::hash_s (SKID_D3D12KnownComShaderDigest) } },
  mesh     { { "Mesh",     SK_D3D12_ShaderType::Mesh     }, { SK_D3D12_ShaderRepo::hash_s (SKID_D3D12KnownMshShaderDigest) } },
  amplify  { { "Amplify",  SK_D3D12_ShaderType::Amplify  }, { SK_D3D12_ShaderRepo::hash_s (SKID_D3D12KnownAmpShaderDigest) } };

HRESULT
STDMETHODCALLTYPE
D3D12Device_CreateGraphicsPipelineState_Detour (
             ID3D12Device                       *This,
_In_   const D3D12_GRAPHICS_PIPELINE_STATE_DESC *pDesc,
             REFIID                              riid,
_COM_Outptr_ void                              **ppPipelineState )
{
  HRESULT hrPipelineCreate =
    D3D12Device_CreateGraphicsPipelineState_Original (
      This, pDesc,
      riid, ppPipelineState
    );

  if (pDesc == nullptr)
    return hrPipelineCreate;

  static const
    std::unordered_map <SK_D3D12_ShaderType, SK_D3D12_ShaderRepo&>
      repo_map =
        { { SK_D3D12_ShaderType::Vertex,   vertex   },
          { SK_D3D12_ShaderType::Pixel,    pixel    },
          { SK_D3D12_ShaderType::Geometry, geometry },
          { SK_D3D12_ShaderType::Hull,     hull     },
          { SK_D3D12_ShaderType::Domain,   domain   },
          { SK_D3D12_ShaderType::Compute,  compute  },
          { SK_D3D12_ShaderType::Mesh,     mesh     },
          { SK_D3D12_ShaderType::Amplify,  amplify  } };

  auto _StashAHash =
  [&](   SK_D3D12_ShaderType                     type,
      const D3D12_GRAPHICS_PIPELINE_STATE_DESC* pDesc )
  {
    using bytecode_t =
      const D3D12_SHADER_BYTECODE*;
     
    bytecode_t pBytecode = nullptr;

    switch (type)
    {
      case SK_D3D12_ShaderType::Vertex:  pBytecode = (bytecode_t)(
        pDesc->VS.BytecodeLength ? pDesc->VS.pShaderBytecode : nullptr); break;
      case SK_D3D12_ShaderType::Pixel:   pBytecode = (bytecode_t)(
        pDesc->PS.BytecodeLength ? pDesc->PS.pShaderBytecode : nullptr); break;
      case SK_D3D12_ShaderType::Geometry:pBytecode = (bytecode_t)(
        pDesc->GS.BytecodeLength ? pDesc->GS.pShaderBytecode : nullptr); break;
      case SK_D3D12_ShaderType::Domain:  pBytecode = (bytecode_t)(
        pDesc->DS.BytecodeLength ? pDesc->DS.pShaderBytecode : nullptr); break;
      case SK_D3D12_ShaderType::Hull:    pBytecode = (bytecode_t)(
        pDesc->HS.BytecodeLength ? pDesc->HS.pShaderBytecode : nullptr); break;
      default:                           pBytecode = nullptr;            break;
    }

    if (pBytecode != nullptr/* && repo_map.count (type)*/)
    {
      auto FourCC =  ((DxilContainerHeader *)pBytecode)->HeaderFourCC;
      auto pHash  = &((DxilContainerHeader *)pBytecode)->Hash.Digest [0];

      if ( FourCC == DFCC_Container || FourCC == DFCC_DXIL )
      {
        SK_D3D12_ShaderRepo& repo =
          repo_map.at (type);

        if (repo.hash.used.insert (((DxilContainerHeader *)pBytecode)->Hash).second)
        {
          SK_LOG0 ( ( L"%9hs Shader (BytecodeType=%s) [%02x%02x%02x%02x%02x%02x%02x%02x"
                                                     L"%02x%02x%02x%02x%02x%02x%02x%02x]",
                      repo.type.name, FourCC ==
                                        DFCC_Container ?
                                               L"DXBC" : L"DXIL",
              pHash [ 0], pHash [ 1], pHash [ 2], pHash [ 3], pHash [ 4], pHash [ 5],
              pHash [ 6], pHash [ 7], pHash [ 8], pHash [ 9], pHash [10], pHash [11],
              pHash [12], pHash [13], pHash [14], pHash [15] ),
                    __SK_SUBSYSTEM__ );
        }

        if (                          ppPipelineState  != nullptr &&
            (*(ID3D12PipelineState **)ppPipelineState) != nullptr )
        {   (*(ID3D12PipelineState **)
                   ppPipelineState)->SetPrivateData ( repo.hash.guid,
                                          DxilContainerHashSize,
                                                      pHash );

          if (     type == SK_D3D12_ShaderType::Pixel)
            _pixelShaders  [*(ID3D12PipelineState **)ppPipelineState] = true;
          else if (type == SK_D3D12_ShaderType::Vertex)
            _vertexShaders [*(ID3D12PipelineState **)ppPipelineState] = true;
        }
      }

      else
      {
        const SK_D3D12_ShaderRepo& repo =
          repo_map.at (type);

        SK_LOG0 ( ( L"%9hs Shader (FourCC=%hs,%lu)",
                             repo.type.name, (char *)&FourCC, FourCC ),
      __SK_SUBSYSTEM__ );
      }
    }
  };

  if (SUCCEEDED (hrPipelineCreate))
  {
    UINT uiDontCare = 0;

    SK_ComQIPtr <ID3DDestructionNotifier>
                    pDestructomatic (*(ID3D12PipelineState **)ppPipelineState);

    if (pDestructomatic != nullptr)
    {   pDestructomatic->RegisterDestructionCallback (
                   SK_D3D12_PipelineDrainPlug,
                   *(ID3D12PipelineState **)ppPipelineState, &uiDontCare );

      if (pDesc->VS.pShaderBytecode) _StashAHash (SK_D3D12_ShaderType::Vertex,   pDesc);
      if (pDesc->PS.pShaderBytecode) _StashAHash (SK_D3D12_ShaderType::Pixel,    pDesc);
      if (pDesc->GS.pShaderBytecode) _StashAHash (SK_D3D12_ShaderType::Geometry, pDesc);
      if (pDesc->HS.pShaderBytecode) _StashAHash (SK_D3D12_ShaderType::Hull,     pDesc);
      if (pDesc->DS.pShaderBytecode) _StashAHash (SK_D3D12_ShaderType::Domain,   pDesc);

      if (pDesc->CachedPSO.pCachedBlob)
      {
        SK_LOG0 ( ( L"CachedPSO: %zu-bytes (%p)", pDesc->CachedPSO.CachedBlobSizeInBytes,
                                                  pDesc->CachedPSO.pCachedBlob ),
                    __SK_SUBSYSTEM__ );
      }
    }
    
    else
      SK_ReleaseAssert (! "ID3DDestructionNotifier Implemented");
  }

  return
    hrPipelineCreate;
}

struct SK_D3D12_PipelineParser : ID3DX12PipelineParserCallbacks
{
           SK_D3D12_PipelineParser (void) = default;
  virtual ~SK_D3D12_PipelineParser (void) = default;

  inline void
  _StashAHash (SK_D3D12_ShaderType       type,
            const D3D12_SHADER_BYTECODE* pBytecode)
  {
    static const
      std::unordered_map < SK_D3D12_ShaderType,
                           SK_D3D12_ShaderRepo& >
        repo_map =
          { { SK_D3D12_ShaderType::Vertex,   vertex   },
            { SK_D3D12_ShaderType::Pixel,    pixel    },
            { SK_D3D12_ShaderType::Geometry, geometry },
            { SK_D3D12_ShaderType::Hull,     hull     },
            { SK_D3D12_ShaderType::Domain,   domain   },
            { SK_D3D12_ShaderType::Compute,  compute  },
            { SK_D3D12_ShaderType::Mesh,     mesh     },
            { SK_D3D12_ShaderType::Amplify,  amplify  } };

    if (pBytecode != nullptr/* && repo_map.count (type)*/)
    {
      auto FourCC =  ((DxilContainerHeader *)pBytecode)->HeaderFourCC;
      auto pHash  = &((DxilContainerHeader *)pBytecode)->Hash.Digest [0];

      if ( FourCC == DFCC_Container || FourCC == DFCC_DXIL )
      {
        SK_D3D12_ShaderRepo& repo =
          repo_map.at (type);

        if (repo.hash.used.insert (((DxilContainerHeader *)pBytecode)->Hash).second)
        {
          SK_LOG0 ( ( L"PipelineParsed %9hs Shader (BytecodeType=%s) [%02x%02x%02x%02x%02x%02x%02x%02x"
                                                                    L"%02x%02x%02x%02x%02x%02x%02x%02x]",
                      repo.type.name, FourCC ==
                                        DFCC_Container ?
                                               L"DXBC" : L"DXIL",
              pHash [ 0], pHash [ 1], pHash [ 2], pHash [ 3], pHash [ 4], pHash [ 5],
              pHash [ 6], pHash [ 7], pHash [ 8], pHash [ 9], pHash [10], pHash [11],
              pHash [12], pHash [13], pHash [14], pHash [15] ),
                    __SK_SUBSYSTEM__ );
        }

        if (pPipelineState != nullptr)
        {   pPipelineState->SetPrivateData ( repo.hash.guid,
                                      DxilContainerHashSize,
                                                 pHash );

          if (     type == SK_D3D12_ShaderType::Pixel)
            _pixelShaders  [pPipelineState] = true;
          else if (type == SK_D3D12_ShaderType::Vertex)
            _vertexShaders [pPipelineState] = true;
        }

        else
        {
          SK_LOG0 ( ( L"%9hs Shader (FourCC=%hs,%lu)",
                               repo.type.name, (char *)&FourCC, FourCC ),
        __SK_SUBSYSTEM__ );
        }
      }
    }
  };

  void FlagsCb (D3D12_PIPELINE_STATE_FLAGS flags) override
  {
    dll_log->Log (L"Flags: %x", flags);
  }

  void VSCb (const D3D12_SHADER_BYTECODE& VS) override
  {
    const D3D12_SHADER_BYTECODE* pBytecode =
      ( VS.BytecodeLength > 0 ) ? (const D3D12_SHADER_BYTECODE *)
        VS.pShaderBytecode      : nullptr;

    if (                                        pBytecode != nullptr)
      _StashAHash (SK_D3D12_ShaderType::Vertex, pBytecode);
  }

  void PSCb (const D3D12_SHADER_BYTECODE& PS) override
  {
    const D3D12_SHADER_BYTECODE* pBytecode =
      ( PS.BytecodeLength > 0 ) ? (const D3D12_SHADER_BYTECODE *)
        PS.pShaderBytecode      : nullptr;

    if (                                       pBytecode != nullptr)
      _StashAHash (SK_D3D12_ShaderType::Pixel, pBytecode);
  }

  void CachedPSOCb (const D3D12_CACHED_PIPELINE_STATE& cachedPSO) override
  {
    dll_log->Log (L"CachedPSO: %zu-bytes (%p)", cachedPSO.CachedBlobSizeInBytes,
                                                cachedPSO.pCachedBlob);
  }

  ID3D12PipelineState* pPipelineState = nullptr;
};

HRESULT
STDMETHODCALLTYPE
D3D12Device2_CreatePipelineState_Detour ( 
              ID3D12Device2                    *This,
        const D3D12_PIPELINE_STATE_STREAM_DESC *pDesc,
              REFIID                            riid,
_COM_Outptr_  void                            **ppPipelineState )
{
  HRESULT hr =
    D3D12Device2_CreatePipelineState_Original (
           This, pDesc, riid, ppPipelineState );

  if (riid == IID_ID3D12PipelineState && SUCCEEDED (hr) && ppPipelineState != nullptr)
  {
    UINT uiDontCare = 0;

    SK_ComQIPtr <ID3DDestructionNotifier>
                    pDestructomatic (*(ID3D12PipelineState **)ppPipelineState);

    if (pDestructomatic != nullptr)
    {   pDestructomatic->RegisterDestructionCallback (
                   SK_D3D12_PipelineDrainPlug,
                   *(ID3D12PipelineState **)ppPipelineState, &uiDontCare );

      SK_D3D12_PipelineParser
        SK_D3D12_PipelineParse;
        SK_D3D12_PipelineParse.pPipelineState =
        *(ID3D12PipelineState **)ppPipelineState;

      if ( SUCCEEDED (
             D3DX12ParsePipelineStream (*pDesc, &SK_D3D12_PipelineParse)
                     )
         )
      {
        return hr;
      }
    }
  }

  return hr;
}




HRESULT
STDMETHODCALLTYPE
D3D12Device_CreateCommandAllocator_Detour (
  ID3D12Device            *This,
  D3D12_COMMAND_LIST_TYPE  type,
  REFIID                   riid,
  _COM_Outptr_  void      **ppCommandAllocator )
{
  if (riid == __uuidof (ID3D12CommandAllocator))
  {
    struct allocator_cache_s {
      using heap_type_t =
        concurrency::concurrent_unordered_map <ID3D12Device*,
        concurrency::concurrent_unordered_map <ID3D12CommandAllocator*, ULONG64>>;

      const char* type   = "Unknown";
      heap_type_t heap   = heap_type_t ();
      int         cycles = 0;
    } static
      direct_ { .type = "Direct" }, bundle_  { .type = "Bundle"  },
      copy_   { .type = "Copy"   }, compute_ { .type = "Compute" };

    allocator_cache_s*
             pCache = nullptr;

    switch (type)
    {
      case D3D12_COMMAND_LIST_TYPE_DIRECT:  pCache = &direct_;  break;
      case D3D12_COMMAND_LIST_TYPE_BUNDLE:  pCache = &bundle_;  break;
      case D3D12_COMMAND_LIST_TYPE_COPY:    pCache = &copy_;    break;
      case D3D12_COMMAND_LIST_TYPE_COMPUTE: pCache = &compute_; break;
      default:
        break;
    }

    if (pCache != nullptr)
    {
      int extra = 0;

      for ( auto &[pAllocator, LastFrame] : pCache->heap [This] )
      {
        if (pAllocator != nullptr && LastFrame != 0)
        {
          if (pAllocator->AddRef () == 2)
          {
            if (extra++ == 0)
            {
              *ppCommandAllocator =
                         pAllocator;

              if (std::exchange (LastFrame, SK_GetFramesDrawn ()) !=
                                            SK_GetFramesDrawn ())
              {
                pAllocator->Reset ();
              }
              
              SK_LOG1 ( ( L"%hs Command Allocator Reused", pCache->type ),
                            __SK_SUBSYSTEM__ );
            }

            // We found a cached allocator, but let's continue looking for any
            // dead allocators to free.
            if (extra > 4)
            {
              // Hopefully this is not still live...
              pAllocator->SetName (
                SK_FormatStringW ( L"Zombie %hs Allocator (%d)",
                                     pCache->type, pCache->cycles ).c_str ()
                                  );

              pAllocator->Release ();
              pAllocator->Release ();

              LastFrame = 0; // Dead

              SK_LOG1 ( ( L"%hs Command Allocator Released", pCache->type ),
                            __SK_SUBSYSTEM__ );
            }
          }

          else
          {
            ++pCache->cycles;

            pAllocator->Release ();
          }
        }
      }

      if (extra > 0) // Cache hit
        return S_OK;

      if ( SUCCEEDED (
             D3D12Device_CreateCommandAllocator_Original (
               This, type,
               riid, ppCommandAllocator
             )       )
         )
      {
        ((ID3D12CommandAllocator *)*ppCommandAllocator)->AddRef ();

        pCache->heap [This].insert (
          std::make_pair ( (ID3D12CommandAllocator *)*ppCommandAllocator,
                               SK_GetFramesDrawn () ) );

        return S_OK;
      }
    }
  }

  return
    D3D12Device_CreateCommandAllocator_Original ( This,
        type, riid, ppCommandAllocator
    );
}

void
STDMETHODCALLTYPE
D3D12Device_CreateRenderTargetView_Detour (
                ID3D12Device                  *This,
_In_opt_        ID3D12Resource                *pResource,
_In_opt_  const D3D12_RENDER_TARGET_VIEW_DESC *pDesc,
_In_            D3D12_CPU_DESCRIPTOR_HANDLE    DestDescriptor )
{
  // HDR Fix-Ups
  if ( pResource != nullptr && __SK_HDR_16BitSwap   &&
       pDesc     != nullptr && pDesc->ViewDimension ==
                                D3D12_RTV_DIMENSION_TEXTURE2D )
  {
    auto desc =
      pResource->GetDesc ();

    if ( desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D &&
         desc.Format    == DXGI_FORMAT_R16G16B16A16_FLOAT )
    {
      if (                       pDesc->Format  != DXGI_FORMAT_UNKNOWN &&
          DirectX::MakeTypeless (pDesc->Format) != DXGI_FORMAT_R16G16B16A16_TYPELESS)
      {
        SK_LOG_FIRST_CALL

        auto fixed_desc = *pDesc;
             fixed_desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;

        return
          D3D12Device_CreateRenderTargetView_Original ( This,
            pResource, &fixed_desc,
              DestDescriptor
          );
      }
    }
  }

  return
    D3D12Device_CreateRenderTargetView_Original ( This,
       pResource, pDesc,
         DestDescriptor
    );
}

D3D12_RESOURCE_ALLOCATION_INFO
STDMETHODCALLTYPE
D3D12Device_GetResourceAllocationInfo_Detour (
      ID3D12Device        *This,
      UINT                 visibleMask,
      UINT                 numResourceDescs,
const D3D12_RESOURCE_DESC *pResourceDescs)
{
  SK_LOG_FIRST_CALL

  if (numResourceDescs > 0 && numResourceDescs < 64 && pResourceDescs != nullptr)
  {
    D3D12_RESOURCE_DESC res_desc [64] = { };

    for ( UINT iRes = 0 ; iRes < numResourceDescs ; ++iRes )
    {
      res_desc [iRes] = pResourceDescs [iRes];

      switch (res_desc [iRes].Dimension)
      {
        case D3D12_RESOURCE_DIMENSION_BUFFER:
          res_desc [iRes].Alignment  = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
          break;
        case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
          if (res_desc [iRes].Alignment == 4096)
              res_desc [iRes].Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
          break;
        default:
          break;
      }
    }

    auto ret =
      D3D12Device_GetResourceAllocationInfo_Original ( This,
        visibleMask, numResourceDescs, res_desc/*pResourceDescs*/);

    if (ret.SizeInBytes != UINT64_MAX)
      return ret;
  }

  return
    D3D12Device_GetResourceAllocationInfo_Original ( This,
      visibleMask, numResourceDescs, pResourceDescs);
}


const GUID IID_ITrackD3D12Resource =
{ 0x696442be, 0xa72e, 0x4059, { 0xac, 0x78, 0x5b, 0x5c, 0x98, 0x04, 0x0f, 0xad } };

concurrency::concurrent_unordered_map <ID3D12Device*, std::tuple <ID3D12Fence*, volatile UINT64, UINT64>> _downstreamFence;

struct DECLSPEC_UUID("696442be-a72e-4059-ac78-5b5c98040fad")
SK_ITrackD3D12Resource final : IUnknown
{
  SK_ITrackD3D12Resource ( ID3D12Device   *pDevice,
                           ID3D12Resource *pResource,
                           ID3D12Fence    *pFence_,
                  volatile UINT64         *puiFenceValue_ ) :
                                   pReal  (pResource),
                                   pDev   (pDevice),
                                   pFence (pFence_),
                                   ver_   (0)
  {
    pResource->SetPrivateDataInterface (
      IID_ITrackD3D12Resource, this
    );

    NextFrame =
      SK_GetFramesDrawn () + _d3d12_rbk->frames_.size ();

    pCmdQueue =
      _d3d12_rbk->_pCommandQueue;
  }

  virtual ~SK_ITrackD3D12Resource (void)
  {
  }

  SK_ITrackD3D12Resource            (const SK_ITrackD3D12Resource &) = delete;
  SK_ITrackD3D12Resource &operator= (const SK_ITrackD3D12Resource &) = delete;

#pragma region IUnknown
  HRESULT STDMETHODCALLTYPE QueryInterface (REFIID riid, void **ppvObj) override; // 0
  ULONG   STDMETHODCALLTYPE AddRef         (void)                       override; // 1
  ULONG   STDMETHODCALLTYPE Release        (void)                       override; // 2
#pragma endregion

  volatile LONG                  refs_ = 1;
  unsigned int                   ver_;
  ID3D12Resource                *pReal;
  SK_ComPtr <ID3D12Device>       pDev;
  SK_ComPtr <ID3D12CommandQueue> pCmdQueue;
  SK_ComPtr <ID3D12Fence>        pFence;
  UINT64                         uiFence   = 0;
  UINT64                         NextFrame = 0;
};

HRESULT
STDMETHODCALLTYPE
SK_ITrackD3D12Resource::QueryInterface (REFIID riid, void **ppvObj)
{
  if (ppvObj == nullptr)
  {
    return E_POINTER;
  }

  if (
    riid == __uuidof (this)      ||
    riid == __uuidof (IUnknown)  ||
    riid == IID_ITrackD3D12Resource )
  {
    auto _GetVersion = [](REFIID riid) ->
    UINT
    {
      if (riid == __uuidof (ID3D12Resource))  return 0;

      return 0;
    };

    UINT required_ver =
      _GetVersion (riid);

    if (ver_ < required_ver)
    {
      IUnknown* pPromoted = nullptr;

      if ( FAILED (
             pReal->QueryInterface ( riid,
                           (void **)&pPromoted )
                  ) || pPromoted == nullptr
         )
      {
        return E_NOINTERFACE;
      }

      ver_ =
        SK_COM_PromoteInterface (&pReal, pPromoted) ?
                                       required_ver : ver_;
    }

    else
    {
      AddRef ();
    }

    *ppvObj = this;

    return S_OK;
  }

  HRESULT hr =
    pReal->QueryInterface (riid, ppvObj);

  if ( riid != IID_IUnknown &&
       riid != IID_ID3DUserDefinedAnnotation )
  {
    static
      std::unordered_set <std::wstring> reported_guids;

    wchar_t                wszGUID [41] = { };
    StringFromGUID2 (riid, wszGUID, 40);

    bool once =
      reported_guids.count (wszGUID) > 0;

    if (! once)
    {
      reported_guids.emplace (wszGUID);

      SK_LOG0 ( ( L"QueryInterface on tracked D3D12 Resource for Mystery UUID: %s",
                      wszGUID ), L"   DXGI   " );
    }
  }

  return hr;
}

ULONG
STDMETHODCALLTYPE
SK_ITrackD3D12Resource::AddRef (void)
{
  return
    InterlockedIncrement (&refs_);
}

ULONG
STDMETHODCALLTYPE
SK_ITrackD3D12Resource::Release (void)
{
  ULONG xrefs =
    InterlockedDecrement (&refs_);

  if (xrefs == 1)
  {
    SK_LOG0 ( ( L"(-) Releasing tracked Resource (%08ph)... device=%08ph", pReal, pDev.p),
             __SK_SUBSYSTEM__ );

    delete this;
  }

  return xrefs;
}

concurrency::concurrent_queue <SK_ITrackD3D12Resource *> _resourcesToWrite_Downstream;
concurrency::concurrent_queue <SK_ITrackD3D12Resource *> _resourcesToWrite_Upstream;

void
SK_D3D12_EnqueueResource_Down (SK_ITrackD3D12Resource* pResource)
{
                                     pResource->pReal->AddRef ();
                                     pResource->AddRef        ();
  _resourcesToWrite_Downstream.push (pResource);
}

void
SK_D3D12_EnqueueResource_Up (SK_ITrackD3D12Resource* pResource)
{
                                   pResource->pReal->AddRef ();
                                   pResource->AddRef        ();
  _resourcesToWrite_Upstream.push (pResource);
}

void
SK_D3D12_WriteResources (void)
{
  SK_ITrackD3D12Resource *pRes;

  std::queue <SK_ITrackD3D12Resource *> _rejects;

  while (! _resourcesToWrite_Downstream.empty ())
  {
    if (_resourcesToWrite_Downstream.try_pop (pRes))
    {
      if (pRes->uiFence == 0 && pRes->NextFrame <= SK_GetFramesDrawn ())
      {
        auto &[pFence, uiFenceVal, ulNextFrame] =
          _downstreamFence [pRes->pDev];

        if ( const UINT64 sync_value = ReadULong64Acquire (&uiFenceVal) + 1 ;
             SUCCEEDED ( pRes->pCmdQueue->Signal (
                           pFence, sync_value    )
                       )
         )
        {
          pRes->uiFence =                   sync_value;
          WriteULong64Release (&uiFenceVal, sync_value);
        }
      }

      if (pRes->uiFence > 0 && pRes->pFence->GetCompletedValue () >= pRes->uiFence)
      {
        DirectX::ScratchImage image;

        if ( pRes->pCmdQueue != nullptr &&
               SUCCEEDED (DirectX::CaptureTexture (pRes->pCmdQueue, pRes->pReal, false, image,
                                                     D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                                                     D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)) )
        {
          DirectX::SaveToDDSFile (
            image.GetImages     (),
            image.GetImageCount (),
              image.GetMetadata (),
                               0x0,
              SK_FormatStringW (L"%p.dds", pRes->pReal).c_str ()
          );
        }

        pRes->pReal->Release ();
        pRes->Release        ();
      }

      else
      {
        _rejects.push (pRes);
      }
    }
  }

  // Give it a go again the next frame
  while (! _rejects.empty ())
  {
    auto pReject =
      _rejects.front ();
      _rejects.pop   ();

    _resourcesToWrite_Downstream.push (pReject);
  }

  // Most likely to have complete data CPU-side
  while (! _resourcesToWrite_Upstream.empty ())
  {
    if (_resourcesToWrite_Upstream.try_pop (pRes))
    {
      DirectX::ScratchImage image;

      if ( pRes->pCmdQueue != nullptr &&
             SUCCEEDED (DirectX::CaptureTexture (pRes->pCmdQueue, pRes->pReal, false, image,
                                                   D3D12_RESOURCE_STATE_COMMON,
                                                   D3D12_RESOURCE_STATE_COMMON)) )
      {
        DirectX::SaveToDDSFile (
          image.GetImages     (),
          image.GetImageCount (),
            image.GetMetadata (),
                             0x0,
            SK_FormatStringW (L"%p.dds", pRes->pReal).c_str ()
        );
      }

      pRes->pReal->Release ();
      pRes->Release        ();
    }
  }
}

void SK_D3D12_CopyTexRegion_Dump (ID3D12GraphicsCommandList* This, ID3D12Resource* pResource)
{
  SK_ComPtr <ID3D12Device> pDevice;
  if (SUCCEEDED (This->GetDevice (IID_ID3D12Device, (void **)&pDevice.p)))
  {
    auto& [pFence, UIFenceVal, NextFrame] =
      _downstreamFence [pDevice];

    if (pFence == nullptr)
      pDevice->CreateFence (0, D3D12_FENCE_FLAG_NONE,
                          IID_ID3D12Fence, (void **)&pFence);

    SK_D3D12_EnqueueResource_Down (
      new (std::nothrow) SK_ITrackD3D12Resource ( pDevice, pResource,
                                                        pFence,
                                                      &UIFenceVal )
    );
  }
}

HRESULT
STDMETHODCALLTYPE
D3D12Device_CreateCommittedResource_Detour (
                 ID3D12Device           *This,
_In_       const D3D12_HEAP_PROPERTIES  *pHeapProperties,
                 D3D12_HEAP_FLAGS        HeapFlags,
_In_       const D3D12_RESOURCE_DESC    *pDesc,
                 D3D12_RESOURCE_STATES   InitialResourceState,
_In_opt_   const D3D12_CLEAR_VALUE      *pOptimizedClearValue,
                 REFIID                  riidResource,
_COM_Outptr_opt_ void                  **ppvResource )
{
  if (pDesc != nullptr) // Not optional, but some games try it anyway :)
  {
    if (     ppvResource      != nullptr            &&
            riidResource      == IID_ID3D12Resource &&
        pDesc->Dimension      == D3D12_RESOURCE_DIMENSION_TEXTURE2D &&
        pHeapProperties->Type == D3D12_HEAP_TYPE_DEFAULT &&
   ( ( pDesc->Flags & ( D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET |
                        D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL |
                        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS /*|
                          D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE*/ ) ) == 0x0 ) )
    {
      ID3D12Resource *pResource = nullptr;
      HRESULT hrCreateCommitted =
        D3D12Device_CreateCommittedResource_Original ( This,
          pHeapProperties, HeapFlags, pDesc, InitialResourceState,
            pOptimizedClearValue, riidResource, (void **)&pResource );

      if (SUCCEEDED (hrCreateCommitted))
      {
#ifdef _DUMP_TEXTURES
        auto& [pFence, UIFenceVal, NextFrame] =
          _downstreamFence [This];

        if (pFence == nullptr)
          This->CreateFence (0, D3D12_FENCE_FLAG_NONE,
                            IID_ID3D12Fence, (void **)&pFence);

        SK_D3D12_EnqueueResource_Down (
          new (std::nothrow) SK_ITrackD3D12Resource ( This, pResource,
                                                            pFence,
                                                          &UIFenceVal )
        );
#endif

        *ppvResource = pResource;
      }

      return hrCreateCommitted;
    }

    else if
       (     ppvResource      != nullptr            &&
            riidResource      == IID_ID3D12Resource &&
        pDesc->Dimension      == D3D12_RESOURCE_DIMENSION_TEXTURE2D &&
        pHeapProperties->Type == D3D12_HEAP_TYPE_UPLOAD )
    {
      ID3D12Resource *pResource = nullptr;
      HRESULT hrCreateCommitted =
        D3D12Device_CreateCommittedResource_Original ( This,
          pHeapProperties, HeapFlags, pDesc, InitialResourceState,
            pOptimizedClearValue, riidResource, (void **)&pResource );

      if (SUCCEEDED (hrCreateCommitted))
      {
#ifdef _DUMP_TEXTURES
        SK_D3D12_EnqueueResource_Up (
          new (std::nothrow) SK_ITrackD3D12Resource (This, pResource)
        );
#endif

        *ppvResource = pResource;
      }

      return hrCreateCommitted;
    }
  }

  return
    D3D12Device_CreateCommittedResource_Original ( This,
      pHeapProperties, HeapFlags, pDesc, InitialResourceState,
        pOptimizedClearValue, riidResource, ppvResource );
}

HRESULT
STDMETHODCALLTYPE
D3D12Device_CreatePlacedResource_Detour (
                 ID3D12Device           *This,
_In_             ID3D12Heap             *pHeap,
                 UINT64                  HeapOffset,
_In_       const D3D12_RESOURCE_DESC    *pDesc,
                 D3D12_RESOURCE_STATES   InitialState,
_In_opt_   const D3D12_CLEAR_VALUE      *pOptimizedClearValue,
                 REFIID                  riid,
_COM_Outptr_opt_ void                  **ppvResource )
{
  return
    D3D12Device_CreatePlacedResource_Original ( This,
      pHeap, HeapOffset, pDesc, InitialState,
        pOptimizedClearValue, riid, ppvResource );
}


void
_InstallDeviceHooksImpl (ID3D12Device* pDev12)
{
  assert (pDev12 != nullptr);

  if (pDev12 == nullptr)
    return;

  SK_CreateVFTableHook2 ( L"ID3D12Device::CreateCommandAllocator",
                            *(void ***)*(&pDev12), 9,
                             D3D12Device_CreateCommandAllocator_Detour,
                   (void **)&D3D12Device_CreateCommandAllocator_Original );

  SK_CreateVFTableHook2 ( L"ID3D12Device::CreateGraphicsPipelineState",
                            *(void ***)*(&pDev12), 10,
                             D3D12Device_CreateGraphicsPipelineState_Detour,
                   (void **)&D3D12Device_CreateGraphicsPipelineState_Original );

  SK_CreateVFTableHook2 ( L"ID3D12Device::CreateRenderTargetView",
                           *(void ***)*(&pDev12), 20,
                            D3D12Device_CreateRenderTargetView_Detour,
                  (void **)&D3D12Device_CreateRenderTargetView_Original );

  ////
  // Hooking this causes crashes, it needs to be wrapped...
  //

  //SK_CreateVFTableHook2 ( L"ID3D12Device::GetResourceAllocationInfo",
  //                         *(void ***)*(&pDev12), 25,
  //                          D3D12Device_GetResourceAllocationInfo_Detour,
  //                (void **)&D3D12Device_GetResourceAllocationInfo_Original );
  
  SK_CreateVFTableHook2 ( L"ID3D12Device::CreateCommittedResource",
                           *(void ***)*(&pDev12), 27,
                            D3D12Device_CreateCommittedResource_Detour,
                  (void **)&D3D12Device_CreateCommittedResource_Original );
  
  SK_CreateVFTableHook2 ( L"ID3D12Device::CreatePlacedResource",
                           *(void ***)*(&pDev12), 29,
                            D3D12Device_CreatePlacedResource_Detour,
                  (void **)&D3D12Device_CreatePlacedResource_Original );

  // 7  UINT    STDMETHODCALLTYPE GetNodeCount
  // 8  HRESULT STDMETHODCALLTYPE CreateCommandQueue
  // 9  HRESULT STDMETHODCALLTYPE CreateCommandAllocator
  // 10 HRESULT STDMETHODCALLTYPE CreateGraphicsPipelineState
  // 11 HRESULT STDMETHODCALLTYPE CreateComputePipelineState
  // 12 HRESULT STDMETHODCALLTYPE CreateCommandList
  // 13 HRESULT STDMETHODCALLTYPE CheckFeatureSupport
  // 14 HRESULT STDMETHODCALLTYPE CreateDescriptorHeap
  // 15 UINT    STDMETHODCALLTYPE GetDescriptorHandleIncrementSize

  // 21 CreateDepthStencilView
  // 22 CreateSampler
  // 23 CopyDescriptors
  // 24 CopyDescriptorsSimple
  // 25 GetResourceAllocationInfo
  // 26 GetCustomHeapProperties
  // 27 CreateCommittedResource
  // 28 CreateHeap
  // 29 CreatePlacedResource
  // 30 CreateReservedResource
  // 31 CreateSharedHandle
  // 32 OpenSharedHandle
  // 33 OpenSharedHandleByName
  // 34 MakeResident
  // 35 Evict
  // 36 CreateFence
  // 37 GetDeviceRemovedReason
  // 38 GetCopyableFootprints 
  // 39 CreateQueryHeap
  // 40 SetStablePowerState
  // 41 CreateCommandSignature
  // 42 GetResourceTiling
  // 43 GetAdapterLuid

  // ID3D12Device1
  //---------------
  // 44 CreatePipelineLibrary
  // 45 SetEventOnMultipleFenceCompletion
  // 46 SetResidencyPriority

  SK_ComQIPtr <ID3D12Device1>
                    pDevice1 (pDev12);
  if (   nullptr != pDevice1 )
  {
    SK_D3D12_HookPipelineLibrary (pDevice1.p);
  }

  // ID3D12Device2
  //---------------
  // 47 CreatePipelineState

  SK_ComQIPtr <ID3D12Device2>
                    pDevice2 (pDev12);
  if (   nullptr != pDevice2 )
  {
    SK_CreateVFTableHook2 ( L"ID3D12Device2::CreatePipelineState",
                             *(void ***)*(&pDevice2.p), 47,
                              D3D12Device2_CreatePipelineState_Detour,
                    (void **)&D3D12Device2_CreatePipelineState_Original );
  }

  // ID3D12Device3
  //---------------
  // 48 OpenExistingHeapFromAddress
  // 49 OpenExistingHeapFromFileMapping
  // 50 EnqueueMakeResident

  // ID3D12Device4
  //---------------
  // 51 CreateCommandList1
  // 52 CreateProtectedResourceSession
  // 53 CreateCommittedResource1
  // 54 CreateHeap1
  // 55 CreateReservedResource1
  // 56 GetResourceAllocationInfo1

  // ID3D12Device5
  //---------------
  // 57 CreateLifetimeTracker
  // 58 RemoveDevice
  // 59 EnumerateMetaCommand
  // 60 EnumerateMetaCommandParameters
  // 61 CreateMetaCommand
  // 62 CreateStateObject
  // 63 GetRaytracingAccelerationStructurePrebuildInfo
  // 64 CheckDriverMatchingIdentifier

  // ID3D12Device6
  //---------------
  // 65 SetBackgroundProcessingMode

  // ID3D12Device7
  //---------------
  // 66 AddToStateObject
  // 67 CreateProtectedResourceSession1

  // ID3D12Device8
  //---------------
  // 68 GetResourceAllocationInfo2
  // 69 CreateCommittedResource2
  // 70 CreatePlacedResource1
  // 71 CreateSamplerFeedbackUnorderedAccessView
  // 72 GetCopyableFootprints1

  // ID3D12Device9
  //---------------
  // 73 CreateShaderCacheSession
  // 74 ShaderCacheControl
  // 75 CreateCommandQueue1

  SK_ApplyQueuedHooks ();
}
void
SK_D3D12_InstallDeviceHooks (ID3D12Device *pDev12)
{
  SK_RunOnce (_InstallDeviceHooksImpl (pDev12));
}

D3D12CreateDevice_pfn D3D12CreateDevice_Import = nullptr;
volatile LONG         __d3d12_ready            = FALSE;

void
WaitForInitD3D12 (void)
{
  SK_Thread_SpinUntilFlagged (&__d3d12_ready);
}

HRESULT
WINAPI
D3D12CreateDevice_Detour (
  _In_opt_  IUnknown          *pAdapter,
            D3D_FEATURE_LEVEL  MinimumFeatureLevel,
  _In_      REFIID             riid,
  _Out_opt_ void             **ppDevice )
{
  WaitForInitD3D12 ();

  DXGI_LOG_CALL_0 ( L"D3D12CreateDevice" );

  dll_log->LogEx ( true,
                     L"[  D3D 12  ]  <~> Minimum Feature Level - %s\n",
                         SK_DXGI_FeatureLevelsToStr (
                           1,
                             (DWORD *)&MinimumFeatureLevel
                         ).c_str ()
                 );

  if ( pAdapter != nullptr )
  {
    int iver =
      SK_GetDXGIAdapterInterfaceVer ( pAdapter );

    // IDXGIAdapter3 = DXGI 1.4 (Windows 10+)
    if ( iver >= 3 )
    {
      SK::DXGI::StartBudgetThread ( (IDXGIAdapter **)&pAdapter );
    }
  }

  HRESULT res;

  DXGI_CALL (res,
    D3D12CreateDevice_Import ( pAdapter,
                                 MinimumFeatureLevel,
                                   riid,
                                     ppDevice )
  );

  if ( SUCCEEDED ( res ) )
  {
    if ( ppDevice != nullptr )
    {
      //if ( *ppDevice != g_pD3D12Dev )
      //{
        // TODO: This isn't the right way to get the feature level
        dll_log->Log ( L"[  D3D 12  ] >> Device = %ph (Feature Level:%hs)",
                         *ppDevice,
                           SK_DXGI_FeatureLevelsToStr ( 1,
                                                         (DWORD *)&MinimumFeatureLevel//(DWORD *)&ret_level
                                                      ).c_str ()
                     );

        //g_pD3D12Dev =
        //  (IUnknown *)*ppDevice;
      //}
    }
  }

  return res;
}

bool
SK_D3D12_HookDeviceCreation (void)
{
  static bool hooked = false;
  if (        hooked)
    return    hooked;

  if ( MH_OK ==
         SK_CreateDLLHook ( L"d3d12.dll",
                             "D3D12CreateDevice",
                              D3D12CreateDevice_Detour,
                   (LPVOID *)&D3D12CreateDevice_Import,
                          &pfnD3D12CreateDevice )
     )
  {
    std::exchange (hooked, true);
    
    InterlockedIncrement (&__d3d12_ready);
  }

  return hooked;
}

void
SK_D3D12_EnableHooks (void)
{
  if (pfnD3D12CreateDevice != nullptr)
    SK_EnableHook (pfnD3D12CreateDevice);

  InterlockedIncrement (&__d3d12_hooked);
}