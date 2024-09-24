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

#include <filesystem>

#include <DirectXTex/d3dx12.h>

// Various device-resource hacks are here for HDR
#include <SpecialK/render/dxgi/dxgi_hdr.h>

#include <concurrent_unordered_map.h>
#include <concurrent_unordered_set.h>

LPVOID pfnD3D12CreateDevice = nullptr;

D3D12Device_CreateGraphicsPipelineState_pfn
D3D12Device_CreateGraphicsPipelineState_Original = nullptr;
D3D12Device2_CreatePipelineState_pfn
D3D12Device2_CreatePipelineState_Original        = nullptr;
D3D12Device_CreateShaderResourceView_pfn
D3D12Device_CreateShaderResourceView_Original    = nullptr;
D3D12Device_CreateUnorderedAccessView_pfn
D3D12Device_CreateUnorderedAccessView_Original   = nullptr;
D3D12Device_CreateRenderTargetView_pfn
D3D12Device_CreateRenderTargetView_Original      = nullptr;
D3D12Device_CreateSampler_pfn
D3D12Device_CreateSampler_Original               = nullptr;
D3D12Device11_CreateSampler2_pfn
D3D12Device11_CreateSampler2_Original            = nullptr;
D3D12Device_GetResourceAllocationInfo_pfn
D3D12Device_GetResourceAllocationInfo_Original   = nullptr;
D3D12Device_CreateCommittedResource_pfn
D3D12Device_CreateCommittedResource_Original     = nullptr;
D3D12Device_CreatePlacedResource_pfn
D3D12Device_CreatePlacedResource_Original        = nullptr;
D3D12Device_CreateHeap_pfn
D3D12Device_CreateHeap_Original                  = nullptr;
D3D12Device_CreateCommandAllocator_pfn
D3D12Device_CreateCommandAllocator_Original      = nullptr;
D3D12Device_CheckFeatureSupport_pfn
D3D12Device_CheckFeatureSupport_Original         = nullptr;

D3D12Device4_CreateCommittedResource1_pfn
D3D12Device4_CreateCommittedResource1_Original   = nullptr;

D3D12Device8_CreateCommittedResource2_pfn
D3D12Device8_CreateCommittedResource2_Original   = nullptr;

concurrency::concurrent_unordered_set <ID3D12PipelineState*> _criticalVertexShaders;
concurrency::concurrent_unordered_map <ID3D12PipelineState*, bool> _vertexShaders;
concurrency::concurrent_unordered_map <ID3D12PipelineState*, bool> _pixelShaders;

concurrency::concurrent_queue         <SK_ComPtr <ID3D12PipelineState>>   _pendingPSOBlobs;
concurrency::concurrent_unordered_map <ID3D12PipelineState*, std::string> _latePSOBlobs;

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
      _pixelShaders [      (ID3D12PipelineState *)pBpOil] = false;
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

// Track a Pipeline State Object that we never saw the creation of
void
SK_D3D12_AddMissingPipelineState ( ID3D12Device        *pDevice,
                                   ID3D12PipelineState *pPipelineState )
{
  if (! pPipelineState)
    return;

  std::ignore = pDevice;

  static const uint8_t empty [DxilContainerHashSize] = { };

  pPipelineState->SetPrivateData ( SKID_D3D12KnownVtxShaderDigest,
                                            DxilContainerHashSize,
                                                          empty );

  _pendingPSOBlobs.push (pPipelineState);

  static SK_AutoHandle hNotify (
      SK_CreateEvent ( nullptr, FALSE,
                                FALSE, nullptr ) );

  SK_RunOnce
  ( SK_Thread_CreateEx ([](LPVOID)
 -> DWORD
    {
      SK_Thread_SetCurrentPriority (THREAD_PRIORITY_LOWEST);

      HANDLE hWaitObjs [] = {
             hNotify.m_h,
             __SK_DLL_TeardownEvent
      };

      while ( WAIT_OBJECT_0 ==
                WaitForMultipleObjects ( 2, hWaitObjs, FALSE, INFINITE ) )
      {
        while (! _pendingPSOBlobs.empty ())
        {
          SK_ComPtr <ID3D12PipelineState> pPipelineState;
          if (_pendingPSOBlobs.try_pop (  pPipelineState))
          {
            SK_ComPtr <ID3DBlob>                           pBlob;
            if (SUCCEEDED (pPipelineState->GetCachedBlob (&pBlob.p)))
            {
              _latePSOBlobs [pPipelineState] =
                SK_FormatString ( "%08x",
                        crc32c ( 0x0, pBlob->GetBufferPointer (),
                                      pBlob->GetBufferSize    () )
                                );
            }
          }
        }
      }

      SK_Thread_CloseSelf ();

      return 0;
    }, L"[SK] D3D12 Pipeline Cache Coordinator")
  );

  SetEvent (hNotify);
}

HRESULT
STDMETHODCALLTYPE
D3D12Device_CreateGraphicsPipelineState_Detour (
             ID3D12Device                       *This,
_In_   const D3D12_GRAPHICS_PIPELINE_STATE_DESC *pDesc_,
             REFIID                              riid,
_COM_Outptr_ void                              **ppPipelineState )
{
  if (ppPipelineState == nullptr)
    return E_INVALIDARG;

  *ppPipelineState = nullptr;

  if (riid != __uuidof (ID3D12PipelineState) || ppPipelineState == nullptr)
  {
    wchar_t                wszGUID [41] = { };
    StringFromGUID2 (riid, wszGUID, 40);

    SK_LOGi0 (
      L"ID3D12Device::CreateGraphicsPipelineState (...) for unknown riid=%ws",
        wszGUID
    );

    return
      D3D12Device_CreateGraphicsPipelineState_Original (
        This, pDesc_,
        riid, ppPipelineState
      );
  }

  D3D12_GRAPHICS_PIPELINE_STATE_DESC
    desc_ = *pDesc_;
       auto *pDesc =
             &desc_;

  // HDR Fix-Ups
  if (__SK_HDR_16BitSwap)
  {
    switch (SK_GetCurrentGameID ())
    {
      case SK_GAME_ID::PathOfExile:
      {
        // Change R8G8B8A8_UNORM_SRGB RTs to R16G16B16A16_FLOAT (when applicable)
        if ( desc_.NumRenderTargets == 1 &&
             desc_.RTVFormats   [0] == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB )
        {
          // This is only a partially working solution...
          if (desc_.InputLayout.NumElements == 1)
          {
            desc_.RTVFormats [0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
          }
        }
      } break;

      default:
        break;
    }
  }

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
    // Do not enable in other games for now, needs more testing
    //
    static const bool bEldenRing =
    //(SK_GetCurrentGameID () == SK_GAME_ID::EldenRing);
      false;

    if (bEldenRing || config.render.dxgi.allow_d3d12_footguns)
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
  }

  return
    hrPipelineCreate;
}


//------------------------------------------------------------------------------------------------
// Stream Parser Helpers

struct ID3DX12PipelineParserCallbacks_SK
{
    // Subobject Callbacks
    virtual void FlagsCb(D3D12_PIPELINE_STATE_FLAGS) {}
    virtual void NodeMaskCb(UINT) {}
    virtual void RootSignatureCb(ID3D12RootSignature*) {}
    virtual void InputLayoutCb(const D3D12_INPUT_LAYOUT_DESC&) {}
    virtual void IBStripCutValueCb(D3D12_INDEX_BUFFER_STRIP_CUT_VALUE) {}
    virtual void PrimitiveTopologyTypeCb(D3D12_PRIMITIVE_TOPOLOGY_TYPE) {}
    virtual void VSCb(const D3D12_SHADER_BYTECODE&) {}
    virtual void GSCb(const D3D12_SHADER_BYTECODE&) {}
    virtual void StreamOutputCb(const D3D12_STREAM_OUTPUT_DESC&) {}
    virtual void HSCb(const D3D12_SHADER_BYTECODE&) {}
    virtual void DSCb(const D3D12_SHADER_BYTECODE&) {}
    virtual void PSCb(const D3D12_SHADER_BYTECODE&) {}
    virtual void CSCb(const D3D12_SHADER_BYTECODE&) {}
    virtual void BlendStateCb(const D3D12_BLEND_DESC&) {}
    virtual void DepthStencilStateCb(const D3D12_DEPTH_STENCIL_DESC&) {}
    virtual void DepthStencilState1Cb(const D3D12_DEPTH_STENCIL_DESC1&) {}
    virtual void DSVFormatCb(DXGI_FORMAT&) {}
    virtual void RasterizerStateCb(const D3D12_RASTERIZER_DESC&) {}
    virtual void RTVFormatsCb(D3D12_RT_FORMAT_ARRAY&) {}
    virtual void SampleDescCb(const DXGI_SAMPLE_DESC&) {}
    virtual void SampleMaskCb(UINT) {}
    virtual void CachedPSOCb(const D3D12_CACHED_PIPELINE_STATE&) {}

    // Error Callbacks
    virtual void ErrorBadInputParameter(UINT /*ParameterIndex*/) {}
    virtual void ErrorDuplicateSubobject(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE /*DuplicateType*/) {}
    virtual void ErrorUnknownSubobject(UINT /*UnknownTypeValue*/) {}

};

struct CD3DX12_PIPELINE_STATE_STREAM_PARSE_HELPER_SK : public ID3DX12PipelineParserCallbacks_SK
{
    CD3DX12_PIPELINE_STATE_STREAM PipelineStream;

    // ID3DX12PipelineParserCallbacks
    void FlagsCb(D3D12_PIPELINE_STATE_FLAGS Flags) {PipelineStream.Flags = Flags;}
    void NodeMaskCb(UINT NodeMask) {PipelineStream.NodeMask = NodeMask;}
    void RootSignatureCb(ID3D12RootSignature* pRootSignature) {PipelineStream.pRootSignature = pRootSignature;}
    void InputLayoutCb(const D3D12_INPUT_LAYOUT_DESC& InputLayout) {PipelineStream.InputLayout = InputLayout;}
    void IBStripCutValueCb(D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBStripCutValue) {PipelineStream.IBStripCutValue = IBStripCutValue;}
    void PrimitiveTopologyTypeCb(D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType) {PipelineStream.PrimitiveTopologyType = PrimitiveTopologyType;}
    void VSCb(const D3D12_SHADER_BYTECODE& VS) {PipelineStream.VS = VS;}
    void GSCb(const D3D12_SHADER_BYTECODE& GS) {PipelineStream.GS = GS;}
    void StreamOutputCb(const D3D12_STREAM_OUTPUT_DESC& StreamOutput) {PipelineStream.StreamOutput = StreamOutput;}
    void HSCb(const D3D12_SHADER_BYTECODE& HS) {PipelineStream.HS = HS;}
    void DSCb(const D3D12_SHADER_BYTECODE& DS) {PipelineStream.DS = DS;}
    void PSCb(const D3D12_SHADER_BYTECODE& PS) {PipelineStream.PS = PS;}
    void CSCb(const D3D12_SHADER_BYTECODE& CS) {PipelineStream.CS = CS;}
    void BlendStateCb(const D3D12_BLEND_DESC& BlendState) {PipelineStream.BlendState = CD3DX12_BLEND_DESC(BlendState);}
    void DepthStencilStateCb(const D3D12_DEPTH_STENCIL_DESC& DepthStencilState) {PipelineStream.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC1(DepthStencilState);}
    void DepthStencilState1Cb(const D3D12_DEPTH_STENCIL_DESC1& DepthStencilState) {PipelineStream.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC1(DepthStencilState);}
    void DSVFormatCb(DXGI_FORMAT& DSVFormat) {PipelineStream.DSVFormat = DSVFormat;}
    void RasterizerStateCb(const D3D12_RASTERIZER_DESC& RasterizerState) {PipelineStream.RasterizerState = CD3DX12_RASTERIZER_DESC(RasterizerState);}
    void RTVFormatsCb(D3D12_RT_FORMAT_ARRAY& RTVFormats) {PipelineStream.RTVFormats = RTVFormats;}
    void SampleDescCb(const DXGI_SAMPLE_DESC& SampleDesc) {PipelineStream.SampleDesc = SampleDesc;}
    void SampleMaskCb(UINT SampleMask) {PipelineStream.SampleMask = SampleMask;}
    void CachedPSOCb(const D3D12_CACHED_PIPELINE_STATE& CachedPSO) {PipelineStream.CachedPSO = CachedPSO;}
    void ErrorBadInputParameter(UINT) {}
    void ErrorDuplicateSubobject(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE) {}
    void ErrorUnknownSubobject(UINT) {}
};

inline HRESULT D3DX12ParsePipelineStream_SK(D3D12_PIPELINE_STATE_STREAM_DESC& Desc, ID3DX12PipelineParserCallbacks_SK* pCallbacks)
{
    if (Desc.SizeInBytes == 0 || Desc.pPipelineStateSubobjectStream == nullptr)
    {
        pCallbacks->ErrorBadInputParameter(1); // first parameter issue
        return E_INVALIDARG;
    }

    if (pCallbacks == nullptr)
    {
        pCallbacks->ErrorBadInputParameter(2); // second parameter issue
        return E_INVALIDARG;
    }

    bool SubobjectSeen[D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MAX_VALID] = {0};
    for (SIZE_T CurOffset = 0, SizeOfSubobject = 0; CurOffset < Desc.SizeInBytes; CurOffset += SizeOfSubobject)
    {
        BYTE* pStream = static_cast<BYTE*>(Desc.pPipelineStateSubobjectStream)+CurOffset;
        auto SubobjectType = *reinterpret_cast<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE*>(pStream);
        if (SubobjectType >= D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_MAX_VALID)
        {
            pCallbacks->ErrorUnknownSubobject(SubobjectType);
            return E_INVALIDARG;
        }
        if (SubobjectSeen[D3DX12GetBaseSubobjectType(SubobjectType)])
        {
            pCallbacks->ErrorDuplicateSubobject(SubobjectType);
            return E_INVALIDARG; // disallow subobject duplicates in a stream
        }
        SubobjectSeen[SubobjectType] = true;
        switch (SubobjectType)
        {
        case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE: 
            pCallbacks->RootSignatureCb(*reinterpret_cast<decltype(CD3DX12_PIPELINE_STATE_STREAM::pRootSignature)*>(pStream));
            SizeOfSubobject = sizeof(CD3DX12_PIPELINE_STATE_STREAM::pRootSignature);
            break;
        case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS:
            pCallbacks->VSCb(*reinterpret_cast<decltype(CD3DX12_PIPELINE_STATE_STREAM::VS)*>(pStream));
            SizeOfSubobject = sizeof(CD3DX12_PIPELINE_STATE_STREAM::VS);
            break;
        case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PS: 
            pCallbacks->PSCb(*reinterpret_cast<decltype(CD3DX12_PIPELINE_STATE_STREAM::PS)*>(pStream));
            SizeOfSubobject = sizeof(CD3DX12_PIPELINE_STATE_STREAM::PS);
            break;
        case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DS: 
            pCallbacks->DSCb(*reinterpret_cast<decltype(CD3DX12_PIPELINE_STATE_STREAM::DS)*>(pStream));
            SizeOfSubobject = sizeof(CD3DX12_PIPELINE_STATE_STREAM::DS);
            break;
        case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_HS: 
            pCallbacks->HSCb(*reinterpret_cast<decltype(CD3DX12_PIPELINE_STATE_STREAM::HS)*>(pStream));
            SizeOfSubobject = sizeof(CD3DX12_PIPELINE_STATE_STREAM::HS);
            break;
        case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_GS: 
            pCallbacks->GSCb(*reinterpret_cast<decltype(CD3DX12_PIPELINE_STATE_STREAM::GS)*>(pStream));
            SizeOfSubobject = sizeof(CD3DX12_PIPELINE_STATE_STREAM::GS);
            break;
        case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CS:
            pCallbacks->CSCb(*reinterpret_cast<decltype(CD3DX12_PIPELINE_STATE_STREAM::CS)*>(pStream));
            SizeOfSubobject = sizeof(CD3DX12_PIPELINE_STATE_STREAM::CS);
            break;
        case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_STREAM_OUTPUT: 
            pCallbacks->StreamOutputCb(*reinterpret_cast<decltype(CD3DX12_PIPELINE_STATE_STREAM::StreamOutput)*>(pStream));
            SizeOfSubobject = sizeof(CD3DX12_PIPELINE_STATE_STREAM::StreamOutput);
            break;
        case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_BLEND: 
            pCallbacks->BlendStateCb(*reinterpret_cast<decltype(CD3DX12_PIPELINE_STATE_STREAM::BlendState)*>(pStream));
            SizeOfSubobject = sizeof(CD3DX12_PIPELINE_STATE_STREAM::BlendState);
            break;
        case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_MASK: 
            pCallbacks->SampleMaskCb(*reinterpret_cast<decltype(CD3DX12_PIPELINE_STATE_STREAM::SampleMask)*>(pStream));
            SizeOfSubobject = sizeof(CD3DX12_PIPELINE_STATE_STREAM::SampleMask);
            break;
        case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RASTERIZER: 
            pCallbacks->RasterizerStateCb(*reinterpret_cast<decltype(CD3DX12_PIPELINE_STATE_STREAM::RasterizerState)*>(pStream));
            SizeOfSubobject = sizeof(CD3DX12_PIPELINE_STATE_STREAM::RasterizerState);
            break;
        case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL: 
            pCallbacks->DepthStencilStateCb(*reinterpret_cast<CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL*>(pStream));
            SizeOfSubobject = sizeof(CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL);
            break;
        case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL1: 
            pCallbacks->DepthStencilState1Cb(*reinterpret_cast<decltype(CD3DX12_PIPELINE_STATE_STREAM::DepthStencilState)*>(pStream));
            SizeOfSubobject = sizeof(CD3DX12_PIPELINE_STATE_STREAM::DepthStencilState);
            break;
        case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT: 
            pCallbacks->InputLayoutCb(*reinterpret_cast<decltype(CD3DX12_PIPELINE_STATE_STREAM::InputLayout)*>(pStream));
            SizeOfSubobject = sizeof(CD3DX12_PIPELINE_STATE_STREAM::InputLayout);
            break;
        case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_IB_STRIP_CUT_VALUE: 
            pCallbacks->IBStripCutValueCb(*reinterpret_cast<decltype(CD3DX12_PIPELINE_STATE_STREAM::IBStripCutValue)*>(pStream));
            SizeOfSubobject = sizeof(CD3DX12_PIPELINE_STATE_STREAM::IBStripCutValue);
            break;
        case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY: 
            pCallbacks->PrimitiveTopologyTypeCb(*reinterpret_cast<decltype(CD3DX12_PIPELINE_STATE_STREAM::PrimitiveTopologyType)*>(pStream));
            SizeOfSubobject = sizeof(CD3DX12_PIPELINE_STATE_STREAM::PrimitiveTopologyType);
            break;
        case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS: 
            pCallbacks->RTVFormatsCb(*reinterpret_cast<D3D12_RT_FORMAT_ARRAY*>(pStream));
            SizeOfSubobject = sizeof(CD3DX12_PIPELINE_STATE_STREAM::RTVFormats);
            break;
        case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT: 
            pCallbacks->DSVFormatCb(*reinterpret_cast<DXGI_FORMAT*>(pStream));
            SizeOfSubobject = sizeof(CD3DX12_PIPELINE_STATE_STREAM::DSVFormat);
            break;
        case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_SAMPLE_DESC: 
            pCallbacks->SampleDescCb(*reinterpret_cast<decltype(CD3DX12_PIPELINE_STATE_STREAM::SampleDesc)*>(pStream));
            SizeOfSubobject = sizeof(CD3DX12_PIPELINE_STATE_STREAM::SampleDesc);
            break;
        case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_NODE_MASK: 
            pCallbacks->NodeMaskCb(*reinterpret_cast<decltype(CD3DX12_PIPELINE_STATE_STREAM::NodeMask)*>(pStream));
            SizeOfSubobject = sizeof(CD3DX12_PIPELINE_STATE_STREAM::NodeMask);
            break;
        case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_CACHED_PSO: 
            pCallbacks->CachedPSOCb(*reinterpret_cast<decltype(CD3DX12_PIPELINE_STATE_STREAM::CachedPSO)*>(pStream));
            SizeOfSubobject = sizeof(CD3DX12_PIPELINE_STATE_STREAM::CachedPSO);
            break;
        case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_FLAGS:
            pCallbacks->FlagsCb(*reinterpret_cast<decltype(CD3DX12_PIPELINE_STATE_STREAM::Flags)*>(pStream));
            SizeOfSubobject = sizeof(CD3DX12_PIPELINE_STATE_STREAM::Flags);
            break;
        default:
            pCallbacks->ErrorUnknownSubobject(SubobjectType);
            return E_INVALIDARG;
            break;
        }
    }

    return S_OK;
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

          static const bool bEldenRing =
          //(SK_GetCurrentGameID () == SK_GAME_ID::EldenRing);
            false;

          // Do not enable in other games for now, needs more testing
          //
          if (bEldenRing || config.render.dxgi.allow_d3d12_footguns)
          {
            if (     type == SK_D3D12_ShaderType::Pixel)
              _pixelShaders  [pPipelineState] = true;
            else if (type == SK_D3D12_ShaderType::Vertex)
              _vertexShaders [pPipelineState] = true;
          }
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
       const  D3D12_PIPELINE_STATE_STREAM_DESC *pDesc,
              REFIID                            riid,
_COM_Outptr_  void                            **ppPipelineState )
{
  SK_LOG_FIRST_CALL

  if (ppPipelineState == nullptr)
    return E_INVALIDARG;

  HRESULT hr =
    D3D12Device2_CreatePipelineState_Original (
           This, pDesc, riid, ppPipelineState );

  // Do not enable in other games for now, needs more testing
  //
  static const bool bEldenRing =
  //(SK_GetCurrentGameID () == SK_GAME_ID::EldenRing);
    false;

  if (bEldenRing || config.render.dxgi.allow_d3d12_footguns)
  {
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
  }

  return hr;
}




using D3D12Device_CreateCommandQueue_pfn =
  HRESULT (STDMETHODCALLTYPE *)(ID3D12Device*,const D3D12_COMMAND_QUEUE_DESC*,
                                REFIID,void**);

static D3D12Device_CreateCommandQueue_pfn
       D3D12Device_CreateCommandQueue_Original = nullptr;

HRESULT
STDMETHODCALLTYPE
D3D12Device_CreateCommandQueue_Detour (ID3D12Device             *This,
                                 const D3D12_COMMAND_QUEUE_DESC *pDesc,
                                       REFIID                     riid,
                                       void           **ppCommandQueue)
{
  SK_LOG_FIRST_CALL

  auto _descCopy = pDesc != nullptr ?
                  *pDesc            : D3D12_COMMAND_QUEUE_DESC { };

  // This requires special privileges that the user probably doesn't have
  if (_descCopy.Priority == D3D12_COMMAND_QUEUE_PRIORITY_GLOBAL_REALTIME)
  {
    if (FAILED (ModifyPrivilege (SE_INC_BASE_PRIORITY_NAME, TRUE)))
    {
      SK_LOGi0 (
        L" >> Lowering priority on Command Queue created by game from Global Realtime to High,"
        L" because the user's account lacks SeIncreaseBasePriorityPrivilege;"
        L" admin accounts have this privilege, BTW."
      );
      _descCopy.Priority = D3D12_COMMAND_QUEUE_PRIORITY_HIGH;
    }
  }

  if (pDesc != nullptr)
  {
    return
      D3D12Device_CreateCommandQueue_Original (This, &_descCopy, riid, ppCommandQueue);
  }

  return
    D3D12Device_CreateCommandQueue_Original (This, pDesc, riid, ppCommandQueue);
}

HRESULT
STDMETHODCALLTYPE
D3D12Device_CreateCommandAllocator_Detour (
  ID3D12Device            *This,
  D3D12_COMMAND_LIST_TYPE  type,
  REFIID                   riid,
  _COM_Outptr_  void      **ppCommandAllocator )
{
  if (ppCommandAllocator != nullptr)
  {  *ppCommandAllocator  = nullptr;

#if 0
    static const bool bEldenRing =
    //(SK_GetCurrentGameID () == SK_GAME_ID::EldenRing);
      false;

    if (bEldenRing && riid == __uuidof (ID3D12CommandAllocator))
    {
      struct allocator_cache_s {
        using heap_type_t =
          concurrency::concurrent_unordered_map <ID3D12Device*,
          concurrency::concurrent_unordered_map <ID3D12CommandAllocator*, volatile ULONG64>>;

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
          if (pAllocator != nullptr && ReadULong64Acquire (&LastFrame) != 0)
          {
            if (pAllocator->AddRef () == 2)
            {
              if (extra++ == 0)
              {
                *ppCommandAllocator =
                         pAllocator;
                         pAllocator->Reset ();

                SK_LOG1 ( ( L"%hs Command Allocator Reused", pCache->type ),
                              __SK_SUBSYSTEM__ );

                SK_LOG1 ( ( L"Allocator Heap Size: %d", pCache->heap [This].size ()),
                              __SK_SUBSYSTEM__ );
              }

              // We found a cached allocator, but let's continue looking for any
              // dead allocators to free.
              else
              {
                // There are an insane number (200+) of live Direct Cmd Allocators
                //   in Elden Ring
                if (pAllocator->Release () == 1 && extra > 256)
                {
                  // Hopefully this is not still live...
                  pAllocator->SetName (
                    SK_FormatStringW ( L"Zombie %hs Allocator (%d)",
                                         pCache->type, pCache->cycles ).c_str ()
                                      );

                  WriteULong64Release (&LastFrame, 0); // Dead

                  pAllocator->Release ();

                  SK_LOG1 ( ( L"%hs Command Allocator Released (%p)", pCache->type,
                                                                      pAllocator ),
                              __SK_SUBSYSTEM__ );
                }
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
          (*(ID3D12CommandAllocator **)ppCommandAllocator)->AddRef ();

          pCache->heap [This].insert (
            std::make_pair ( *(ID3D12CommandAllocator **)ppCommandAllocator,
                                 SK_GetFramesDrawn () + 1 ) );

          return S_OK;
        }
      }
    }
#endif
  }

  return
    D3D12Device_CreateCommandAllocator_Original ( This,
        type, riid, ppCommandAllocator
    );
}

void
STDMETHODCALLTYPE
D3D12Device_CreateShaderResourceView_Detour (
                ID3D12Device                    *This,
_In_opt_        ID3D12Resource                  *pResource,
_In_opt_  const D3D12_SHADER_RESOURCE_VIEW_DESC *pDesc,
_In_            D3D12_CPU_DESCRIPTOR_HANDLE       DestDescriptor )
{
  // HDR Fix-Ups
  if ( pResource != nullptr && ( __SK_HDR_16BitSwap ||
                                 __SK_HDR_10BitSwap ) &&
       pDesc     != nullptr && ( pDesc->ViewDimension ==
                                   D3D12_SRV_DIMENSION_TEXTURE2D ||
                                 pDesc->ViewDimension ==
                                   D3D12_SRV_DIMENSION_TEXTURE2DARRAY ) )
  {
    auto desc =
      pResource->GetDesc ();

    // Handle explicitly defined SRVs, they might expect the SwapChain to be RGBA8
    if (pDesc->Format != DXGI_FORMAT_UNKNOWN || DirectX::IsTypeless (desc.Format))
    {
      auto expectedTyplessFormat =
        __SK_HDR_16BitSwap ? DXGI_FORMAT_R16G16B16A16_TYPELESS // scRGB
                           : DXGI_FORMAT_R10G10B10A2_TYPELESS; // HDR10
      auto expectedTypedFormat   =
        __SK_HDR_16BitSwap ? DXGI_FORMAT_R16G16B16A16_FLOAT    // scRGB
                           : DXGI_FORMAT_R10G10B10A2_UNORM;    // HDR10

      if ( desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D &&
          (desc.Format    == expectedTypedFormat   ||
          (desc.Format    == expectedTyplessFormat &&
            desc.Format   != DirectX::MakeTypeless (pDesc->Format))) )
      {
        SK_LOG_FIRST_CALL

          auto fixed_desc = *pDesc;
               fixed_desc.Format = expectedTypedFormat;

        return
          D3D12Device_CreateShaderResourceView_Original ( This,
             pResource, &fixed_desc,
               DestDescriptor
          );
      }

      // Spider-Man needs the opposite of the fix above.
      if ( desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D &&
           desc.Format    != expectedTypedFormat                &&
          pDesc->Format   == expectedTypedFormat )
      {
        if (! DirectX::IsTypeless (desc.Format))
        {
          SK_LOG_FIRST_CALL

            auto fixed_desc = *pDesc;
                 fixed_desc.Format = desc.Format;

          return
            D3D12Device_CreateShaderResourceView_Original ( This,
               pResource, &fixed_desc,
                 DestDescriptor
            );
        }
      }
    }
  }

  return
    D3D12Device_CreateShaderResourceView_Original ( This,
       pResource, pDesc,
         DestDescriptor
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
  if (pResource != nullptr)
  {
    auto res_desc =
      pResource->GetDesc ();

    if (res_desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)
    {
      switch (SK_GetCurrentGameID ())
      {
        case SK_GAME_ID::Starfield:
#if 0
          SK_LOGi0 (L"D3D12Device_CreateRTV: %hs (%dx%d)", SK_DXGI_FormatToStr (res_desc.Format).data (), res_desc.Width, res_desc.Height);
#endif

          break;

        default:
          break;
      }
    }
  }

  // HDR Fix-Ups
  if ( pResource != nullptr && ( __SK_HDR_16BitSwap ||
                                 __SK_HDR_10BitSwap ) &&
       pDesc     != nullptr && ( pDesc->ViewDimension ==
                                   D3D12_RTV_DIMENSION_TEXTURE2D ||
                                 pDesc->ViewDimension ==
                                   D3D12_RTV_DIMENSION_TEXTURE2DARRAY ) )
  {
    auto desc =
      pResource->GetDesc ();

    if (pDesc->Format != DXGI_FORMAT_UNKNOWN || DirectX::IsTypeless (desc.Format))
    {
      auto expectedTyplessFormat =
        __SK_HDR_16BitSwap ? DXGI_FORMAT_R16G16B16A16_TYPELESS // scRGB
                           : DXGI_FORMAT_R10G10B10A2_TYPELESS; // HDR10
      auto expectedTypedFormat   =
        __SK_HDR_16BitSwap ? DXGI_FORMAT_R16G16B16A16_FLOAT    // scRGB
                           : DXGI_FORMAT_R10G10B10A2_UNORM;    // HDR10

      if ( desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D &&
          (desc.Format    == expectedTypedFormat ||
          (desc.Format    == expectedTyplessFormat &&
           desc.Format    != DirectX::MakeTypeless (pDesc->Format))) )
      {
        SK_LOG_FIRST_CALL

        auto fixed_desc = *pDesc;
             fixed_desc.Format = expectedTypedFormat;

        return
          D3D12Device_CreateRenderTargetView_Original ( This,
            pResource, &fixed_desc,
              DestDescriptor
          );
      }

      // Spider-Man needs the opposite of the fix above.
      if ( desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D &&
           desc.Format    != expectedTypedFormat                &&
          pDesc->Format   == expectedTypedFormat )
      {
        if (! DirectX::IsTypeless (desc.Format))
        {
          SK_LOG_FIRST_CALL

            auto fixed_desc = *pDesc;
                 fixed_desc.Format = desc.Format;

          return
            D3D12Device_CreateRenderTargetView_Original ( This,
               pResource, &fixed_desc,
                 DestDescriptor
            );
        }
      }
    }
  }

  return
    D3D12Device_CreateRenderTargetView_Original ( This,
       pResource, pDesc,
         DestDescriptor
    );
}

void
STDMETHODCALLTYPE
D3D12Device_CreateUnorderedAccessView_Detour (
                ID3D12Device                     *This,
_In_opt_        ID3D12Resource                   *pResource,
_In_opt_        ID3D12Resource                   *pCounterResource,
_In_opt_  const D3D12_UNORDERED_ACCESS_VIEW_DESC *pDesc,
_In_            D3D12_CPU_DESCRIPTOR_HANDLE       DestDescriptor )
{
  // HDR Fix-Ups
  if ( pResource != nullptr && ( __SK_HDR_16BitSwap ||
                                 __SK_HDR_10BitSwap ) &&
       pDesc     != nullptr && ( pDesc->ViewDimension ==
                                   D3D12_UAV_DIMENSION_TEXTURE2D ||
                                 pDesc->ViewDimension ==
                                   D3D12_UAV_DIMENSION_TEXTURE2DARRAY ) )
  {
    auto desc =
      pResource->GetDesc ();

    if (pDesc->Format != DXGI_FORMAT_UNKNOWN || DirectX::IsTypeless (desc.Format))
    {
      auto expectedTyplessFormat =
        __SK_HDR_16BitSwap ? DXGI_FORMAT_R16G16B16A16_TYPELESS // scRGB
                           : DXGI_FORMAT_R10G10B10A2_TYPELESS; // HDR10
      auto expectedTypedFormat   =
        __SK_HDR_16BitSwap ? DXGI_FORMAT_R16G16B16A16_FLOAT    // scRGB
                           : DXGI_FORMAT_R10G10B10A2_UNORM;    // HDR10

      if ( desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D &&
          (desc.Format    == expectedTypedFormat ||
          (desc.Format    == expectedTyplessFormat &&
           desc.Format    != DirectX::MakeTypeless (pDesc->Format))) )
      {
        SK_LOG_FIRST_CALL

        auto fixed_desc = *pDesc;
             fixed_desc.Format = expectedTypedFormat;

        return
          D3D12Device_CreateUnorderedAccessView_Original ( This,
            pResource, pCounterResource, &fixed_desc,
              DestDescriptor
          );
      }

      // Spider-Man needs the opposite of the fix above.
      if ( desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D &&
           desc.Format    != expectedTypedFormat     &&
          pDesc->Format   == expectedTypedFormat )
      {
        if (! DirectX::IsTypeless (desc.Format))
        {
          SK_LOG_FIRST_CALL

            auto fixed_desc = *pDesc;
                 fixed_desc.Format = desc.Format;

          return
            D3D12Device_CreateUnorderedAccessView_Original ( This,
              pResource, pCounterResource, &fixed_desc,
                DestDescriptor
            );
        }
      }
    }
  }

  return
    D3D12Device_CreateUnorderedAccessView_Original ( This,
       pResource, pCounterResource, pDesc,
         DestDescriptor
    );
}

void
STDMETHODCALLTYPE
D3D12Device_CreateSampler_Detour (
            ID3D12Device          *This,
_In_  const D3D12_SAMPLER_DESC    *pDesc,
_In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor)
{
  SK_LOG_FIRST_CALL

  D3D12_SAMPLER_DESC desc =
    (pDesc != nullptr) ?
    *pDesc             : D3D12_SAMPLER_DESC {};

  if (config.render.d3d12.force_anisotropic)
  {
    switch (desc.Filter)
    {
      case D3D12_FILTER_MIN_MAG_MIP_LINEAR:
        desc.Filter = D3D12_FILTER_ANISOTROPIC;
        break;
      case D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT:
        desc.Filter = D3D12_FILTER_MIN_MAG_ANISOTROPIC_MIP_POINT;
        break;
      case D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR:
        desc.Filter = D3D12_FILTER_COMPARISON_ANISOTROPIC;
        break;
      case D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT:
        desc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_ANISOTROPIC_MIP_POINT;
        break;
      case D3D12_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR:
        desc.Filter = D3D12_FILTER_MINIMUM_ANISOTROPIC;
        break;
      case D3D12_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT:
        desc.Filter = D3D12_FILTER_MINIMUM_MIN_MAG_ANISOTROPIC_MIP_POINT;
        break;
      case D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR:
        desc.Filter = D3D12_FILTER_MAXIMUM_ANISOTROPIC;
        break;
      case D3D12_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT:
        desc.Filter = D3D12_FILTER_MAXIMUM_MIN_MAG_ANISOTROPIC_MIP_POINT;
        break;
      default:
        break;
    }
  }

  switch (desc.Filter)
  {
    case D3D12_FILTER_ANISOTROPIC:
    case D3D12_FILTER_MIN_MAG_ANISOTROPIC_MIP_POINT:
    case D3D12_FILTER_COMPARISON_MIN_MAG_ANISOTROPIC_MIP_POINT:
    case D3D12_FILTER_COMPARISON_ANISOTROPIC:
    case D3D12_FILTER_MINIMUM_MIN_MAG_ANISOTROPIC_MIP_POINT:
    case D3D12_FILTER_MINIMUM_ANISOTROPIC:
    case D3D12_FILTER_MAXIMUM_MIN_MAG_ANISOTROPIC_MIP_POINT:
    case D3D12_FILTER_MAXIMUM_ANISOTROPIC:
      if (config.render.d3d12.max_anisotropy > 0)
        desc.MaxAnisotropy = (UINT)config.render.d3d12.max_anisotropy;
      break;
    default:
      break;
  }

  return
    D3D12Device_CreateSampler_Original (This, (pDesc == nullptr) ?
                                                        nullptr : &desc, DestDescriptor);
}

void
STDMETHODCALLTYPE
D3D12Device11_CreateSampler2_Detour (
            ID3D12Device11        *This,
_In_  const D3D12_SAMPLER_DESC2   *pDesc,
_In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor)
{
  SK_LOG_FIRST_CALL

  D3D12_SAMPLER_DESC2 desc =
    (pDesc != nullptr) ?
    *pDesc             : D3D12_SAMPLER_DESC2 {};

  if (config.render.d3d12.force_anisotropic)
  {
    switch (desc.Filter)
    {
      case D3D12_FILTER_MIN_MAG_MIP_LINEAR:
        desc.Filter = D3D12_FILTER_ANISOTROPIC;
        break;
      case D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT:
        desc.Filter = D3D12_FILTER_MIN_MAG_ANISOTROPIC_MIP_POINT;
        break;
      case D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR:
        desc.Filter = D3D12_FILTER_COMPARISON_ANISOTROPIC;
        break;
      case D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT:
        desc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_ANISOTROPIC_MIP_POINT;
        break;
      case D3D12_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR:
        desc.Filter = D3D12_FILTER_MINIMUM_ANISOTROPIC;
        break;
      case D3D12_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT:
        desc.Filter = D3D12_FILTER_MINIMUM_MIN_MAG_ANISOTROPIC_MIP_POINT;
        break;
      case D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR:
        desc.Filter = D3D12_FILTER_MAXIMUM_ANISOTROPIC;
        break;
      case D3D12_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT:
        desc.Filter = D3D12_FILTER_MAXIMUM_MIN_MAG_ANISOTROPIC_MIP_POINT;
        break;
      default:
        break;
    }
  }

  switch (desc.Filter)
  {
    case D3D12_FILTER_ANISOTROPIC:
    case D3D12_FILTER_MIN_MAG_ANISOTROPIC_MIP_POINT:
    case D3D12_FILTER_COMPARISON_MIN_MAG_ANISOTROPIC_MIP_POINT:
    case D3D12_FILTER_COMPARISON_ANISOTROPIC:
    case D3D12_FILTER_MINIMUM_MIN_MAG_ANISOTROPIC_MIP_POINT:
    case D3D12_FILTER_MINIMUM_ANISOTROPIC:
    case D3D12_FILTER_MAXIMUM_MIN_MAG_ANISOTROPIC_MIP_POINT:
    case D3D12_FILTER_MAXIMUM_ANISOTROPIC:
      if (config.render.d3d12.max_anisotropy > 0)
        desc.MaxAnisotropy = (UINT)config.render.d3d12.max_anisotropy;
      break;
    default:
      break;
  }

  return
    D3D12Device11_CreateSampler2_Original (This, (pDesc == nullptr) ?
                                                           nullptr : &desc, DestDescriptor);
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

struct DECLSPEC_UUID("696442be-a72e-4059-ac78-5b5c98040fad")
SK_ITrackD3D12Resource final : IUnknown
{
  SK_ITrackD3D12Resource ( ID3D12Device   *pDevice,
                           ID3D12Resource *pResource,
                           ID3D12Fence    *pFence_,
                           const wchar_t  *wszName,
                  volatile UINT64         */*puiFenceValue_*/) :
                                   pReal  (pResource),
                                   pDev   (pDevice),
                                   pFence (pFence_),
                                   name_  (wszName != nullptr ? wszName
                                                             : L"Unnamed"),
                                   ver_   (0)
  {
    pResource->SetPrivateDataInterface (
      IID_ITrackD3D12Resource, this
    );

    NextFrame =
      SK_GetFramesDrawn ();// + _d3d12_rbk->frames_.size ();

    pCmdQueue =
      _d3d12_rbk->_pCommandQueue;
  }

  SK_ITrackD3D12Resource ( ID3D12Device   *pDevice,
                           ID3D12Resource *pResource,
                           INT             iBufferIdx,
                           ID3D12Fence    *pFence_,
                           const wchar_t  *wszName,
                  volatile UINT64         */*puiFenceValue_*/) :
                                   pReal  (pResource),
                                   pDev   (pDevice),
                                   pFence (pFence_),
                                   name_  (wszName != nullptr ? wszName
                                                             : L"Unnamed"),
                                   ver_   (0)
  {
    pResource->SetPrivateDataInterface (
      IID_ITrackD3D12Resource, this
    );

    NextFrame =
      SK_GetFramesDrawn ();// + _d3d12_rbk->frames_.size ();

    pCmdQueue =
      _d3d12_rbk->_pCommandQueue;

    this->iBufferIdx = iBufferIdx;
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
  std::wstring                   name_;
  ID3D12Resource                *pReal;
  SK_ComPtr <ID3D12Device>       pDev;
  SK_ComPtr <ID3D12CommandQueue> pCmdQueue;
  SK_ComPtr <ID3D12Fence>        pFence;
  UINT64                         uiFence    = 0;
  UINT64                         NextFrame  = 0;
  UINT                           iBufferIdx = 0;
};

void
SK_D3D12_TrackResource (ID3D12Device *pDevice, ID3D12Resource *pResource, UINT iBufferIdx)
{
  new (std::nothrow) SK_ITrackD3D12Resource ( pDevice, pResource, iBufferIdx,
                                                        nullptr, nullptr, nullptr );
}

bool
SK_D3D12_IsTrackedResource (ID3D12Resource *pResource)
{
  SK_ComPtr <SK_ITrackD3D12Resource> pTrackedResource;
  UINT size = sizeof (void *);
  
  return
    SUCCEEDED (pResource->GetPrivateData (IID_ITrackD3D12Resource, &size, (void **)&pTrackedResource.p));
}

bool
SK_D3D12_IsBackBufferOnActiveQueue (ID3D12Resource *pResource, ID3D12CommandQueue *pCmdQueue, UINT iBufferIdx)
{
  SK_ComPtr <SK_ITrackD3D12Resource> pTrackedResource;
  UINT size = sizeof (void *);
  
  if (SUCCEEDED (pResource->GetPrivateData (IID_ITrackD3D12Resource, &size, (void **)&pTrackedResource.p)))
  {
    if (pCmdQueue == pTrackedResource.p->pCmdQueue && pTrackedResource.p->iBufferIdx == iBufferIdx)
      return true;
  }

  return false;
}

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
      concurrency::concurrent_unordered_set <std::wstring> reported_guids;

    wchar_t                wszGUID [41] = { };
    StringFromGUID2 (riid, wszGUID, 40);

    bool once =
      reported_guids.count (wszGUID) > 0;

    if (! once)
    {
      reported_guids.insert (wszGUID);

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

static size_t _num_injected = 0;

static std::unordered_set <uint32_t> _injectable;
static std::unordered_set <uint32_t> _dumped;

concurrency::concurrent_unordered_map <ID3D12Device*, std::tuple <ID3D12Fence*, volatile UINT64, UINT64>> _downstreamFence;

void SK_D3D12_CopyTexRegion_Dump (ID3D12GraphicsCommandList* This, ID3D12Resource* pResource, const wchar_t *wszName = nullptr)
{
  if (pResource == nullptr)
    return;

  if (wszName == nullptr)
  {
    // Early-out if there's no work to be done
    if ((! config.textures.dump_on_load) && (_num_injected >= _injectable.size ()))
        return;
  }

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
                                             wszName, &UIFenceVal )
    );
  }
}

bool SK_D3D12_IsTextureInjectionNeeded (void)
{
  std::error_code ec = { };

  static auto path =
  SK_Resource_GetRoot () / LR"(dump\textures)",
         load_path =
  SK_Resource_GetRoot () / LR"(inject\textures)";

  auto scanInjectable = [&]{
  if (_injectable.empty ())
  {
    for ( auto const& file : std::filesystem::directory_iterator { load_path, ec } )
    {
      if ( file.path ().has_extension () &&
           file.path ().extension     ().compare (L".dds") == 0 )
      {
        std::string name =
          file.path ().stem ().string ();

        if (name.find ("d3d12_sk0_crc32c_") != std::string::npos)
        {
          uint32_t crc;

          if ( 1 ==
                 std::sscanf (
                   name.c_str (),
                     "d3d12_sk0_crc32c_%x",
                               &crc )
             )
          {
            if (_injectable.insert (crc).second)
            {
              SK_LOG1 ( ( L"Injectable %x", crc ),
                          __SK_SUBSYSTEM__ );
            }
          }
        }
      }
    }
  }};

  auto scanDumped = [&]{
  if (_dumped.empty ())
  {
    for ( auto const& file : std::filesystem::directory_iterator { path, ec } )
    {
      if ( file.path ().has_extension () &&
           file.path ().extension     ().compare (L".dds") == 0 )
      {
        std::string name =
          file.path ().stem ().string ();

        if (name.find ("d3d12_sk0_crc32c_") != std::string::npos)
        {
          uint32_t crc;

          if ( 1 ==
                std::sscanf (
                  name.c_str (),
                    "d3d12_sk0_crc32c_%x",
                              &crc )
             )
          {
            if (_dumped.insert (crc).second)
            {
              SK_LOG1 ( ( L"Previously Dumped %x", crc ),
                          __SK_SUBSYSTEM__ );
            }
          }
        }
      }
    }
  }};

  scanInjectable ();
  scanDumped     ();

  return
    (config.textures.dump_on_load || (! _injectable.empty ()));
}

struct d3d12_tex_upload_s
{
  std::vector <D3D12_SUBRESOURCE_DATA> subresources;

  SK_ComPtr   <ID3D12Resource>         pUploadHeap;
  SK_ComPtr   <ID3D12Resource>         pDest;
  SK_ComPtr   <ID3D12Device>           pDevice;
};

static concurrency::concurrent_queue <d3d12_tex_upload_s> _resourcesToReplace;

void
SK_D3D12_WriteResources (void)
{
  SK_ITrackD3D12Resource *pRes;

  std::queue <SK_ITrackD3D12Resource *> _rejects;

  while (! _resourcesToWrite_Downstream.empty ())
  { if (   _resourcesToWrite_Downstream.try_pop (pRes))
    {
      static auto path =
        SK_Resource_GetRoot () / LR"(dump\textures)",
             load_path =
        SK_Resource_GetRoot () / LR"(inject\textures)";

      std::error_code ec = { };

      if (pRes->uiFence == 0 && pRes->NextFrame <= SK_GetFramesDrawn () - 5)
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

      if ( pRes->uiFence                      > 0 &&
           pRes->pFence->GetCompletedValue () >= pRes->uiFence)
      {
        UINT size   = 1;
        bool ignore = true;

        pRes->pReal->SetPrivateData (SKID_D3D12IgnoredTextureCopy, size, &ignore);

        DirectX::ScratchImage image;

        // NOTE: If a texture is loaded, unloaded and later reloaded, then
        //         this current optimization will ail to re-inject
        if ( ((_num_injected < _injectable.size ()) ||
                      config.textures.dump_on_load  ||
                          (! pRes->name_.empty ())) &&
                                                    pRes->pCmdQueue != nullptr
             && SUCCEEDED (DirectX::CaptureTexture (pRes->pCmdQueue, pRes->pReal, false, image,
                                                      D3D12_RESOURCE_STATE_COMMON,
                                                      D3D12_RESOURCE_STATE_COMMON)) )
        {
          pRes->pReal->SetPrivateData (SKID_D3D12IgnoredTextureCopy, size, &ignore);

          uint32_t hash =
            crc32c (
              0x0, image.GetPixels     (),
                   image.GetPixelsSize () );

          if (! std::filesystem::is_directory   (path, ec))
            std::filesystem::create_directories (path, ec);

          std::wstring hashed_name = (! pRes->name_.empty ()) ?
                                        pRes->name_ + L".dds" :
            SK_FormatStringW (L"d3d12_sk0_crc32c_%08x.dds", hash);

          std::wstring
            file_to_dump   = path      / hashed_name,
            file_to_inject = load_path / hashed_name;

          bool injected = false;

          if (_injectable.contains (hash) && std::filesystem::exists (file_to_inject, ec))
          {
            SK_ComPtr <ID3D12Device>               pDev;
            pRes->pReal->GetDevice (IID_PPV_ARGS (&pDev.p));

            d3d12_tex_upload_s upload;

            DirectX::ScratchImage inject_img;

            if ( SUCCEEDED (
              DirectX::LoadFromDDSFile (
                                  file_to_inject.c_str (), 0x0,
                                 nullptr, inject_img ) )
              && SUCCEEDED (
              DirectX::PrepareUpload ( pDev, inject_img.GetImages     (),
                                             1,//inject_img.GetImageCount (),
                                             inject_img.GetMetadata   (),
                                               upload.subresources )
                           )
               )
            {
              upload.pDevice = pDev;
              upload.pDest   = pRes->pReal;

              //const UINT64 buffer_size =
              //  GetRequiredIntermediateSize ( pRes->pReal, 0, 1 );
              //    //static_cast <unsigned int> ( upload.subresources.size () ) );

              {
                UINT width  = static_cast <UINT> (inject_img.GetMetadata ().width);
                UINT height = static_cast <UINT> (inject_img.GetMetadata ().height);

                ///upload.pUploadHeap.p->SetPrivateData (SKID_D3D12IgnoredTextureCopy, size, &ignore);
                ///upload.pUploadHeap.p->AddRef (); // Keep alive until upload finishes

                try {
                  SK_ComPtr <ID3D12Resource>            uploadBuffer;

                  SK_ComPtr <ID3D12Fence>               pFence;
                  SK_AutoHandle                         hEvent (
                                                SK_CreateEvent ( nullptr, FALSE,
                                                                          FALSE, nullptr )
                                                               );

                  SK_ComPtr <ID3D12CommandQueue>        cmdQueue;
                  SK_ComPtr <ID3D12CommandAllocator>    cmdAlloc;
                  SK_ComPtr <ID3D12GraphicsCommandList> cmdList;


                  ThrowIfFailed ( hEvent.m_h != 0 ?
                                             S_OK : E_UNEXPECTED );

                  // Upload texture to graphics system
                  D3D12_HEAP_PROPERTIES
                    props                      = { };

                  D3D12_RESOURCE_DESC
                    desc                       = { };

                  D3D12_RESOURCE_DESC textureDesc {};
                  textureDesc.Format             = inject_img.GetMetadata ().format;
                  textureDesc.Width              = (uint32)inject_img.GetMetadata ().width;
                  textureDesc.Height             = (uint32)inject_img.GetMetadata ().height;
                  textureDesc.Flags              = D3D12_RESOURCE_FLAG_NONE;
                  textureDesc.DepthOrArraySize   = 1;
                  textureDesc.MipLevels          = 1;//(uint16)textureMetaData.mipLevels;
                  textureDesc.SampleDesc.Count   = 1;
                  textureDesc.SampleDesc.Quality = 0;
                  textureDesc.Dimension          = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
                  textureDesc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
                  textureDesc.Alignment          = 0;

                  UINT64 uploadSize = 0;
                  UINT   numRows;
                  UINT64 rowSizesInBytes;
                  D3D12_PLACED_SUBRESOURCE_FOOTPRINT layouts;
                  const uint64 numSubResources = 1;

                  pDev->GetCopyableFootprints ( &textureDesc,     0,
                                         (uint32)numSubResources, 0,
                                                &layouts,
                                                &numRows,
                                                   &rowSizesInBytes,
                                                &uploadSize );

                  UINT uploadPitch = sk::narrow_cast <UINT> (upload.subresources [0].RowPitch);
                //UINT uploadSize  = upload.subresources [0].SlicePitch;
                  ////UINT uploadPitch = (width * 4 + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u)
                  ////                            & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u);
                  ////UINT uploadSize  = height * uploadPitch;

                    desc.Dimension             = D3D12_RESOURCE_DIMENSION_BUFFER;
                    desc.Alignment             = 0;
                    desc.Width                 = uploadSize;
                    desc.Height                = 1;
                    desc.DepthOrArraySize      = 1;
                    desc.MipLevels             = 1;
                    desc.Format                = DXGI_FORMAT_UNKNOWN;
                    desc.SampleDesc.Count      = 1;
                    desc.SampleDesc.Quality    = 0;
                    desc.Layout                = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
                    desc.Flags                 = D3D12_RESOURCE_FLAG_NONE;

                    props.Type                 = D3D12_HEAP_TYPE_UPLOAD;
                    props.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
                    props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

                  ThrowIfFailed (
                    pDev->CreateCommittedResource (
                      &props, D3D12_HEAP_FLAG_NONE,
                      &desc,  D3D12_RESOURCE_STATE_GENERIC_READ,
                      nullptr, IID_PPV_ARGS (&uploadBuffer.p))
                  ); SK_D3D12_SetDebugName (  uploadBuffer.p,
                        L"ImGui D3D12 Texture Upload Buffer" );

                  void        *mapped = nullptr;
                  D3D12_RANGE  range  = { 0, sk::narrow_cast <SIZE_T> (uploadSize) };

                  ThrowIfFailed (uploadBuffer->Map (0, &range, &mapped));

                  for ( UINT y = 0; y < height; y++ )
                  {
                    memcpy ( (void*) ((uintptr_t) mapped    + y * uploadPitch),
                        inject_img.GetImage (0,0,0)->pixels + y *
                        inject_img.GetImage (0,0,0)->rowPitch,
                             static_cast <size_t> (rowSizesInBytes) );
                  }

                  uploadBuffer->Unmap (0, &range);

                  D3D12_TEXTURE_COPY_LOCATION
                    srcLocation                                    = { };
                    srcLocation.pResource                          = uploadBuffer;
                    srcLocation.Type                               = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
                    srcLocation.PlacedFootprint.Footprint.Format   = inject_img.GetMetadata ().format;//DXGI_FORMAT_R8G8B8A8_UNORM;
                    srcLocation.PlacedFootprint.Footprint.Width    = width;
                    srcLocation.PlacedFootprint.Footprint.Height   = height;
                    srcLocation.PlacedFootprint.Footprint.Depth    = 1;
                    srcLocation.PlacedFootprint.Footprint.RowPitch = uploadPitch;

                  D3D12_TEXTURE_COPY_LOCATION
                    dstLocation                  = { };
                    dstLocation.pResource        = upload.pDest.p;
                    dstLocation.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
                    dstLocation.SubresourceIndex = 0;


                  ThrowIfFailed (
                    pDev->CreateFence (
                      0, D3D12_FENCE_FLAG_NONE,
                                IID_PPV_ARGS (&pFence.p))
                   ); SK_D3D12_SetDebugName  ( pFence.p,
                   L"ImGui D3D12 Texture Upload Fence");

                  D3D12_COMMAND_QUEUE_DESC
                    queueDesc          = { };
                    queueDesc.Type     = D3D12_COMMAND_LIST_TYPE_DIRECT;
                    queueDesc.Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE;
                    queueDesc.NodeMask = 1;

                  ThrowIfFailed (
                    pDev->CreateCommandQueue (
                         &queueDesc, IID_PPV_ARGS (&cmdQueue.p))
                    ); SK_D3D12_SetDebugName (      cmdQueue.p,
                      L"ImGui D3D12 Texture Upload Cmd Queue");

                  ThrowIfFailed (
                    pDev->CreateCommandAllocator (
                      D3D12_COMMAND_LIST_TYPE_DIRECT,
                                  IID_PPV_ARGS (&cmdAlloc.p))
                  ); SK_D3D12_SetDebugName (     cmdAlloc.p,
                    L"ImGui D3D12 Texture Upload Cmd Allocator");

                  ThrowIfFailed (
                    pDev->CreateCommandList (
                      0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                         cmdAlloc, nullptr,
                                IID_PPV_ARGS (&cmdList.p))
                  ); SK_D3D12_SetDebugName (   cmdList.p,
                  L"ImGui D3D12 Texture Upload Cmd List");

                  SK_D3D12_RenderCtx::transition_state (
                             cmdList, upload.pDest.p, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                                                      D3D12_RESOURCE_STATE_COPY_DEST,
                                                      D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES );

                  uploadBuffer->SetPrivateData (SKID_D3D12IgnoredTextureCopy, size, &ignore);

                  cmdList->CopyTextureRegion ( &dstLocation, 0, 0, 0,
                                               &srcLocation, nullptr );

                  SK_D3D12_RenderCtx::transition_state (
                             cmdList, upload.pDest.p, D3D12_RESOURCE_STATE_COPY_DEST,
                                                      D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                                                      D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES );

                  ThrowIfFailed (                     cmdList->Close ());

                                 cmdQueue->ExecuteCommandLists (1,
                                           (ID3D12CommandList* const*)
                                                     &cmdList);

                  ThrowIfFailed (
                    cmdQueue->Signal (pFence,                       1));
                                      pFence->SetEventOnCompletion (1,
                                                hEvent.m_h);
                  SK_WaitForSingleObject       (hEvent.m_h, INFINITE);

                  ////_resourcesToReplace.push (upload);
                  injected       = true;
                  _num_injected++;
                }

                catch (const SK_ComException& e) {
                  SK_LOG0 ( ( L" Exception: %hs [%ws]", e.what (), __FUNCTIONW__ ),
                              __SK_SUBSYSTEM__ );
                }
              }
            }
          }

          if (                 (! injected) &&
               (config.textures.dump_on_load || (! pRes->name_.empty ())) &&
                (! _dumped.contains (hash)) && (! std::filesystem::exists (
                                                         file_to_dump, ec ) )
             )
          {
            DirectX::SaveToDDSFile (
              image.GetImages     (),
              image.GetImageCount (),
                image.GetMetadata (),
                                 0x0,
                file_to_dump.c_str()
            );
          }
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

void
SK_D3D12_CommitUploadQueue (ID3D12GraphicsCommandList *pCmdList)
{
  if (pCmdList == nullptr)
    return;

  SK_ComPtr <ID3D12Device>            pDev;
  pCmdList->GetDevice (IID_PPV_ARGS (&pDev.p));

  while (! _resourcesToReplace.empty ())
  {
    d3d12_tex_upload_s               upload;
    if (_resourcesToReplace.try_pop (upload))
    {
      SK_ReleaseAssert (upload.pDevice.IsEqualObject (pDev));

      if (upload.pDevice.IsEqualObject (pDev))
      {
        SK_D3D12_RenderCtx::transition_state (
          pCmdList, upload.pDest.p, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                                    D3D12_RESOURCE_STATE_COPY_DEST,
                                    D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES );

        UpdateSubresources ( pCmdList,
          upload.pDest.p, upload.pUploadHeap.p,
                     0ULL, 0U, 1,//static_cast <unsigned int> (upload.subresources.size ()),
                                                           upload.subresources.data () );

        SK_D3D12_RenderCtx::transition_state (
          pCmdList, upload.pDest.p, D3D12_RESOURCE_STATE_COPY_DEST,
                                    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                                    D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES );
      }
    }
  }
}

HRESULT
STDMETHODCALLTYPE
D3D12Device4_CreateCommittedResource1_Detour (
                  ID3D12Device4                  *This,
  _In_            const D3D12_HEAP_PROPERTIES    *pHeapProperties,
  _In_            D3D12_HEAP_FLAGS               HeapFlags,
  _In_            const D3D12_RESOURCE_DESC1     *pDesc,
  _In_            D3D12_RESOURCE_STATES          InitialResourceState,
  _In_opt_        const D3D12_CLEAR_VALUE        *pOptimizedClearValue,
  _In_opt_        ID3D12ProtectedResourceSession *pProtectedSession,
  _In_            REFIID                         riidResource,
  _Out_opt_       void                           **ppvResource )
{
  SK_LOG_FIRST_CALL

  return
    D3D12Device4_CreateCommittedResource1_Original (This, pHeapProperties, HeapFlags, pDesc, InitialResourceState, pOptimizedClearValue, pProtectedSession, riidResource, ppvResource);
}

HRESULT
STDMETHODCALLTYPE
D3D12Device8_CreateCommittedResource2_Detour (
                  ID3D12Device8                  *This,
  _In_            const D3D12_HEAP_PROPERTIES    *pHeapProperties,
  _In_            D3D12_HEAP_FLAGS               HeapFlags,
  _In_            const D3D12_RESOURCE_DESC1     *pDesc,
  _In_            D3D12_RESOURCE_STATES          InitialResourceState,
  _In_opt_        const D3D12_CLEAR_VALUE        *pOptimizedClearValue,
  _In_opt_        ID3D12ProtectedResourceSession *pProtectedSession,
  _In_            REFIID                         riidResource,
  _Out_opt_       void                           **ppvResource )
{
  SK_LOG_FIRST_CALL

  return
    D3D12Device8_CreateCommittedResource2_Original (This, pHeapProperties, HeapFlags, pDesc, InitialResourceState, pOptimizedClearValue, pProtectedSession, riidResource, ppvResource);
}

HRESULT
STDMETHODCALLTYPE
D3D12Device_CreateHeap_Detour (
                 ID3D12Device    *This,
_In_       const D3D12_HEAP_DESC *pDesc,
                 REFIID           riid,
_COM_Outptr_opt_ void           **ppvHeap)
{
  if (ppvHeap != nullptr)
     *ppvHeap  = nullptr;

  return
    D3D12Device_CreateHeap_Original (This, pDesc, riid, ppvHeap);
}

HRESULT
STDMETHODCALLTYPE
D3D12Device_CreateCommittedResource_Detour (
                 ID3D12Device           *This,
_In_       const D3D12_HEAP_PROPERTIES  *pHeapProperties,
                 D3D12_HEAP_FLAGS        HeapFlags,
_In_      const  D3D12_RESOURCE_DESC    *pDesc,
                 D3D12_RESOURCE_STATES   InitialResourceState,
_In_opt_   const D3D12_CLEAR_VALUE      *pOptimizedClearValue,
                 REFIID                  riidResource,
_COM_Outptr_opt_ void                  **ppvResource )
{
  if (ppvResource != nullptr)
     *ppvResource  = nullptr;

  if (pDesc != nullptr) // Not optional, but some games try it anyway :)
  {
#if 0
    switch (SK_GetCurrentGameID ())
    {
      case SK_GAME_ID::Starfield:
      {
        if (__SK_HDR_16BitSwap && pDesc->Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D && (pDesc->Format == DXGI_FORMAT_R8G8B8A8_TYPELESS || pDesc->Format == DXGI_FORMAT_R10G10B10A2_TYPELESS) && ((pDesc->Flags & (D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS))))
        {  
          SK_LOGi0 (L"D3D12Device_CreateCommittedResource: %hs (%dx%d)", SK_DXGI_FormatToStr (pDesc->Format).data (), pDesc->Width, pDesc->Height);
        
          auto desc = *pDesc;
               desc.Format = DXGI_FORMAT_R16G16B16A16_TYPELESS;
        
          D3D12_CLEAR_VALUE  _optimized_clear_value = { };
          D3D12_CLEAR_VALUE *_pOptimizedClearValue  = nullptr;
          
          if (pOptimizedClearValue != nullptr)
          {
            _optimized_clear_value              = *pOptimizedClearValue;
            _optimized_clear_value.Format       = DXGI_FORMAT_R16G16B16A16_FLOAT;
            _optimized_clear_value.DepthStencil = pOptimizedClearValue->DepthStencil;
            _optimized_clear_value.Color[0]     = pOptimizedClearValue->Color [0]/255.0f;
            _optimized_clear_value.Color[1]     = pOptimizedClearValue->Color [1]/255.0f;
            _optimized_clear_value.Color[2]     = pOptimizedClearValue->Color [2]/255.0f;
            _optimized_clear_value.Color[3]     = pOptimizedClearValue->Color [3]/255.0f;
            _pOptimizedClearValue               = &_optimized_clear_value;
          }
        
          HRESULT hr =
            D3D12Device_CreateCommittedResource_Original ( This,
             pHeapProperties, HeapFlags, &desc, InitialResourceState,
               _pOptimizedClearValue, riidResource, ppvResource );
        
          if (SUCCEEDED (hr))
            return hr;
        }
      } break;
    }
#endif

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
  if (ppvResource != nullptr)
     *ppvResource  = nullptr;

  return
    D3D12Device_CreatePlacedResource_Original ( This,
      pHeap, HeapOffset, pDesc, InitialState,
        pOptimizedClearValue, riid, ppvResource );
}

HRESULT
STDMETHODCALLTYPE
D3D12Device_CheckFeatureSupport_Detour (
          ID3D12Device   *This,
          D3D12_FEATURE    Feature,
  _Inout_ void           *pFeatureSupportData,
          UINT             FeatureSupportDataSize )
{
  SK_LOG_FIRST_CALL

  HRESULT hr =
    D3D12Device_CheckFeatureSupport_Original ( This,
      Feature, pFeatureSupportData, FeatureSupportDataSize );

#ifdef SK_VRS_DEBUG
  if (SUCCEEDED (hr) && Feature == D3D12_FEATURE_FEATURE_LEVELS)
  {
    auto pFeatureLevels =
      static_cast <D3D12_FEATURE_DATA_FEATURE_LEVELS *> (pFeatureSupportData);

    pFeatureLevels->MaxSupportedFeatureLevel = D3D_FEATURE_LEVEL_12_1;
  }

  if (SUCCEEDED (hr) && Feature == D3D12_FEATURE_D3D12_OPTIONS6)
  {
    SK_ReleaseAssert (
      FeatureSupportDataSize == sizeof (D3D12_FEATURE_DATA_D3D12_OPTIONS6)
    );

    SK_LOGi0 (L"CheckFeatureSupport (Variable Rate Shading)");

    auto pVRSCaps =
      static_cast <D3D12_FEATURE_DATA_D3D12_OPTIONS6 *> (pFeatureSupportData);

    pVRSCaps->VariableShadingRateTier = D3D12_VARIABLE_SHADING_RATE_TIER_NOT_SUPPORTED;
  }

  SK_LOGi0 (L"CheckFeatureSupport (%d)", Feature);
#endif

  return hr;

}


D3D12Device9_CreateShaderCacheSession_pfn D3D12Device9_CreateShaderCacheSession_Original = nullptr;
D3D12Device9_ShaderCacheControl_pfn       D3D12Device9_ShaderCacheControl_Original       = nullptr;

HRESULT
STDMETHODCALLTYPE
D3D12Device9_CreateShaderCacheSession_Detour (      ID3D12Device9                   *This,
                                              const D3D12_SHADER_CACHE_SESSION_DESC *pDesc,
                                                    REFIID                           riid,
                                                    void                           **ppvSession)
{
  SK_LOG_FIRST_CALL

  return
    D3D12Device9_CreateShaderCacheSession_Original (This, pDesc, riid, ppvSession);
}

HRESULT
STDMETHODCALLTYPE
D3D12Device9_ShaderCacheControl_Detour ( ID3D12Device9                    *This,
                                         D3D12_SHADER_CACHE_KIND_FLAGS     Kinds,
                                         D3D12_SHADER_CACHE_CONTROL_FLAGS  Control )
{
  SK_LOG_FIRST_CALL

  return
    D3D12Device9_ShaderCacheControl_Original (This, Kinds, Control);
}

void
_InstallDeviceHooksImpl (ID3D12Device* pDevice12)
{
  assert (pDevice12 != nullptr);
  if (    pDevice12 == nullptr)
    return;

  const bool bHasStreamline =
    SK_IsModuleLoaded (L"sl.interposer.dll");

  SK_ComPtr <ID3D12Device> pDev12;

  if (bHasStreamline)
  {
    if (SK_slGetNativeInterface (pDevice12, (void **)&pDev12.p) == sl::Result::eOk)
      SK_LOGi0 (L"Hooking Streamline Native Interface for ID3D12Device...");
    else pDev12 = pDevice12;
  } else pDev12 = pDevice12;

  SK_CreateVFTableHook2 ( L"ID3D12Device::CreateCommandQueue",
                            *(void ***)*(&pDev12), 8,
                             D3D12Device_CreateCommandQueue_Detour,
                   (void **)&D3D12Device_CreateCommandQueue_Original );

  SK_CreateVFTableHook2 ( L"ID3D12Device::CreateCommandAllocator",
                            *(void ***)*(&pDev12), 9,
                             D3D12Device_CreateCommandAllocator_Detour,
                   (void **)&D3D12Device_CreateCommandAllocator_Original );

  SK_CreateVFTableHook2 ( L"ID3D12Device::CreateGraphicsPipelineState",
                            *(void ***)*(&pDev12), 10,
                             D3D12Device_CreateGraphicsPipelineState_Detour,
                   (void **)&D3D12Device_CreateGraphicsPipelineState_Original );

  SK_CreateVFTableHook2 ( L"ID3D12Device::CheckFeatureSupport",
                            *(void ***)*(&pDev12), 13,
                             D3D12Device_CheckFeatureSupport_Detour,
                   (void **)&D3D12Device_CheckFeatureSupport_Original );

  SK_CreateVFTableHook2 ( L"ID3D12Device::CreateShaderResourceView",
                           *(void ***)*(&pDev12), 18,
                            D3D12Device_CreateShaderResourceView_Detour,
                  (void **)&D3D12Device_CreateShaderResourceView_Original );

  SK_CreateVFTableHook2 ( L"ID3D12Device::CreateUnorderedAccessView",
                           *(void ***)*(&pDev12), 19,
                            D3D12Device_CreateUnorderedAccessView_Detour,
                  (void **)&D3D12Device_CreateUnorderedAccessView_Original );

  SK_CreateVFTableHook2 ( L"ID3D12Device::CreateRenderTargetView",
                           *(void ***)*(&pDev12), 20,
                            D3D12Device_CreateRenderTargetView_Detour,
                  (void **)&D3D12Device_CreateRenderTargetView_Original );

  SK_CreateVFTableHook2 ( L"ID3D12Device::CreateSampler",
                           *(void ***)*(&pDev12), 22,
                            D3D12Device_CreateSampler_Detour,
                  (void **)&D3D12Device_CreateSampler_Original );

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

  SK_CreateVFTableHook2 ( L"ID3D12Device::CreateHeap",
                           *(void ***)*(&pDev12), 28,
                            D3D12Device_CreateHeap_Detour,
                  (void **)&D3D12Device_CreateHeap_Original );

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

  SK_ComQIPtr <ID3D12Device4>
      pDevice4 (pDev12);
  if (pDevice4.p != nullptr)
  {
    SK_CreateVFTableHook2 ( L"ID3D12Device4::CreateCommittedResource1",
                           *(void ***)*(&pDevice4.p), 53,
                            D3D12Device4_CreateCommittedResource1_Detour,
                  (void **)&D3D12Device4_CreateCommittedResource1_Original );
  }

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

  SK_ComQIPtr <ID3D12Device6>
      pDevice6 (pDev12);
  if (pDevice6.p != nullptr)
  {
    if (config.render.dxgi.allow_d3d12_footguns)
    {
      pDevice6->SetBackgroundProcessingMode ( D3D12_BACKGROUND_PROCESSING_MODE_ALLOW_INTRUSIVE_MEASUREMENTS,
                                              D3D12_MEASUREMENTS_ACTION_KEEP_ALL,
                                              nullptr, nullptr );
    }
  }

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

  SK_ComQIPtr <ID3D12Device8>
      pDevice8 (pDev12);
  if (pDevice8.p != nullptr)
  {
    SK_CreateVFTableHook2 ( L"ID3D12Device8::CreateCommittedResource2",
                           *(void ***)*(&pDevice8.p), 69,
                            D3D12Device8_CreateCommittedResource2_Detour,
                  (void **)&D3D12Device8_CreateCommittedResource2_Original );
  }

  // ID3D12Device9
  //---------------
  // 73 CreateShaderCacheSession
  // 74 ShaderCacheControl
  // 75 CreateCommandQueue1

  SK_ComQIPtr <ID3D12Device9>
      pDevice9 (pDev12);
  if (pDevice9.p != nullptr)
  {
    SK_CreateVFTableHook2 ( L"ID3D12Device9::CreateShaderCacheSession",
                           *(void ***)*(&pDevice9), 73,
                            D3D12Device9_CreateShaderCacheSession_Detour,
                  (void **)&D3D12Device9_CreateShaderCacheSession_Original );

    SK_CreateVFTableHook2 ( L"ID3D12Device9::ShaderCacheControl",
                           *(void ***)*(&pDevice9), 74,
                            D3D12Device9_ShaderCacheControl_Detour,
                  (void **)&D3D12Device9_ShaderCacheControl_Original );
  }

  // ID3D12Device10
  //---------------
  // 76 CreateCommittedResource3
  // 77 CreatePlacedResource2
  // 78 CreateReservedResource2

  // ID3D12Device11
  //---------------
  // 79 CreateSampler2

  SK_ComQIPtr <ID3D12Device11>
      pDevice11 (pDev12);
  if (pDevice11.p != nullptr)
  {
    SK_CreateVFTableHook2 ( L"ID3D12Device11::CreateSampler2",
                             *(void ***)*(&pDevice11.p), 79,
                              D3D12Device11_CreateSampler2_Detour,
                    (void **)&D3D12Device11_CreateSampler2_Original );
  }

  //
  // Extra hooks are needed to handle SwapChain backbuffer copies between
  //   mismatched formats when using HDR override.
  //
  //if (__SK_HDR_16BitSwap || __SK_HDR_10BitSwap)
  {
    SK_ComPtr <ID3D12CommandAllocator>    pCmdAllocator;
    SK_ComPtr <ID3D12GraphicsCommandList> pCmdList;

    try {
      ThrowIfFailed (
        pDev12->CreateCommandAllocator (
          D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS (&pCmdAllocator.p)));
      ThrowIfFailed (
        pDev12->CreateCommandList ( 0,
          D3D12_COMMAND_LIST_TYPE_DIRECT,                pCmdAllocator.p,
                                 nullptr, IID_PPV_ARGS (&pCmdList.p)));
                                                         pCmdList->Close ();

      _InitCopyTextureRegionHook (pCmdList);
    }

    catch (const SK_ComException& e) {
      SK_LOGi0 ( L" Exception: %hs [%ws]", e.what (), __FUNCTIONW__ );
    }
  }
}
bool
SK_D3D12_InstallDeviceHooks (ID3D12Device *pDev12)
{
  static bool s_Init = false;

  // Check the status of hooks
  if (pDev12 == nullptr)
    return s_Init;

  // Actually install hooks... once.
  if (! std::exchange (s_Init, true))
  {
    _InstallDeviceHooksImpl (pDev12);

    return true;
  }

  return false;
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
                     L"[  D3D 12  ]  <~> Minimum Feature Level - %hs\n",
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
      SK_RunOnce (SK::DXGI::StartBudgetThread ( (IDXGIAdapter **)&pAdapter ));
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
      dll_log->Log ( L"[  D3D 12  ] >> Device = %ph (Feature Level:%hs)",
                       *ppDevice,
                         SK_DXGI_FeatureLevelsToStr ( 1,
                                                       (DWORD *)&MinimumFeatureLevel//(DWORD *)&ret_level
                                                    ).c_str ()
                   );
    }

    bool new_hooks = false;

    new_hooks |= SK_D3D12_InstallDeviceHooks       (*(ID3D12Device **)ppDevice);
    new_hooks |= SK_D3D12_InstallCommandQueueHooks (*(ID3D12Device **)ppDevice);

    if (new_hooks)
      SK_ApplyQueuedHooks ();
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