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

#include <SpecialK/render/dxgi/dxgi_util.h>
#include <SpecialK/render/d3d11/d3d11_core.h>
#include <SpecialK/render/d3d11/d3d11_shader.h>
#include <SpecialK/render/d3d11/d3d11_state_tracker.h>

SK_LazyGlobal <SK_D3D11_ShaderStageArray> d3d11_shader_stages;

SK_LazyGlobal <std::array <bool, SK_D3D11_MAX_DEV_CONTEXTS+1>> reshade_trigger_before;
SK_LazyGlobal <std::array <bool, SK_D3D11_MAX_DEV_CONTEXTS+1>> reshade_trigger_after;

void
SK_D3D11_ReleaseCachedShaders (ID3D11Device *This, sk_shader_class type)
{
  const auto GetResources =
  [&]( gsl::not_null <SK_Thread_HybridSpinlock**>                         ppCritical,
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

  SK_Thread_HybridSpinlock*                         pCritical   = nullptr;
  SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>* pShaderRepo = nullptr;

  GetResources (
    &pCritical, &pShaderRepo
  );

  if (pCritical != nullptr)
  {
    pCritical->lock ();     // Lock during cache check

    static volatile ULONG ulDeviceCheck = 1;

    ULONG ulSetValue =
      InterlockedIncrement (&ulDeviceCheck);

    // {AC67C1F9-F795-4DCC-BBC5-97F539BD43D1}
    static const GUID GUID_TestDeviceEquality = 
      { 0xac67c1f9, 0xf795, 0x4dcc, { 0xbb, 0xc5, 0x97, 0xf5, 0x39, 0xbd, 0x43, 0xd1 } };
    
    if (SUCCEEDED (This->SetPrivateData (GUID_TestDeviceEquality, sizeof (ULONG), &ulSetValue)))
    {
      for ( auto& pDevice : pShaderRepo->descs )
      {
        if (! pDevice.second.empty ())
        {
          UINT  ulTestSize = sizeof (ULONG);
          ULONG ulTestValue = 0;

          pDevice.first->GetPrivateData (GUID_TestDeviceEquality, &ulTestSize, &ulTestValue);

          if (ulTestValue == ulSetValue)
          {
            for ( auto& pShader : pDevice.second )
            {
              pShader.second.pShader->Release ();
              pShader.second.bytecode.clear   ();
              pShader.second.name.clear       ();
            }

            pShaderRepo->descs [pDevice.first].clear ();
            pShaderRepo->rev   [pDevice.first].clear ();
          }
        }
      }
    }

    pCritical->unlock ();
  }
}

void
SK_D3D11_SetShader_Impl ( ID3D11DeviceContext        *pDevCtx,
                          IUnknown                   *pShader,
                          sk_shader_class             type,
                          ID3D11ClassInstance *const *ppClassInstances,
                          UINT                        NumClassInstances,
                          bool                        bWrapped,
                          UINT                        dev_idx )
{
  SK_WRAP_AND_HOOK

  //SK_LOG0 ( (L"DevCtx=%p, ShaderClass=%lu, Shader=%p", pDevCtx, type, pShader ),
  //           L"D3D11 Shader" );

  auto& shaders =
    SK_D3D11_Shaders;

  const auto _Finish =
  [&]
  {
    switch (type)
    {
      case sk_shader_class::Vertex:
        return bWrapped ? pDevCtx->VSSetShader (
                           (ID3D11VertexShader *)pShader,
                             ppClassInstances, NumClassInstances )
                       :
          D3D11_VSSetShader_Original ( pDevCtx,
                 (ID3D11VertexShader *)pShader,
                             ppClassInstances, NumClassInstances );

      case sk_shader_class::Pixel:
        return bWrapped ? pDevCtx->PSSetShader (
                           (ID3D11PixelShader *)pShader,
                             ppClassInstances, NumClassInstances )
                       :
          D3D11_PSSetShader_Original ( pDevCtx,
                  (ID3D11PixelShader *)pShader,
                             ppClassInstances, NumClassInstances );

      case sk_shader_class::Geometry:
        return bWrapped ? pDevCtx->GSSetShader (
                           (ID3D11GeometryShader *)pShader,
                             ppClassInstances, NumClassInstances )
                       :
          D3D11_GSSetShader_Original ( pDevCtx,
                           (ID3D11GeometryShader *)pShader,
                             ppClassInstances, NumClassInstances );

      case sk_shader_class::Hull:
        return bWrapped ? pDevCtx->HSSetShader (
                           (ID3D11HullShader *)pShader,
                             ppClassInstances, NumClassInstances )
                       :
          D3D11_HSSetShader_Original ( pDevCtx,
                           (ID3D11HullShader *)pShader,
                             ppClassInstances, NumClassInstances );

      case sk_shader_class::Domain:
        return bWrapped ? pDevCtx->DSSetShader (
                           (ID3D11DomainShader *)pShader,
                             ppClassInstances, NumClassInstances )
                       :
          D3D11_DSSetShader_Original ( pDevCtx,
                           (ID3D11DomainShader *)pShader,
                             ppClassInstances, NumClassInstances );

      case sk_shader_class::Compute:
        return bWrapped ? pDevCtx->CSSetShader (
                           (ID3D11ComputeShader *)pShader,
                             ppClassInstances, NumClassInstances )
                       :
          D3D11_CSSetShader_Original ( pDevCtx,
                           (ID3D11ComputeShader *)pShader,
                             ppClassInstances, NumClassInstances );
    }
  };

  SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  bool early_out =
    (! bMustNotIgnore) || rb.api == SK_RenderAPI::D3D12; // Ignore D3D11On12 overlays    

  bool implicit_track = false;

  if ( rb.in_present_call  &&
       rb.isHDRCapable ()  &&
       rb.isHDRActive  () )
  {
    if ( type == sk_shader_class::Vertex ||
         type == sk_shader_class::Pixel     )
    {
      // Needed for Steam Overlay HDR fix
      implicit_track = true;
    }
  }

  if (early_out && (! implicit_track))
  {
    return
      _Finish ();
  }


  if (! ( implicit_track                           ||
          // This has extra overhead from Reflex, avoid checking it for now
          //SK_D3D11_ShouldTrackDrawCall (pDevCtx,
                     //SK_D3D11DrawType::PrimList) ||
          SK_D3D11_ShouldTrackRenderOp (pDevCtx) ) )
  {
    return
      _Finish ();
  }


  const auto GetResources =
  [&]( SK_Thread_HybridSpinlock**                         ppCritical,
       SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>** ppShaderDomain )
  {
    switch (type)
    {
      default:
      case sk_shader_class::Vertex:
        *ppCritical     = cs_shader_vs.get ();
        *ppShaderDomain =
          reinterpret_cast <
            SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*
          >             (&shaders->vertex);
         break;
      case sk_shader_class::Pixel:
        *ppCritical     = cs_shader_ps.get ();
        *ppShaderDomain =
          reinterpret_cast <
            SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*
          >             (&shaders->pixel);
         break;
      case sk_shader_class::Geometry:
        *ppCritical     = cs_shader_gs.get ();
        *ppShaderDomain =
          reinterpret_cast <
            SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*
          >             (&shaders->geometry);
         break;
      case sk_shader_class::Hull:
        *ppCritical     = cs_shader_hs.get ();
        *ppShaderDomain =
          reinterpret_cast <
            SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*
          >             (&shaders->hull);
         break;
      case sk_shader_class::Domain:
        *ppCritical     = cs_shader_ds.get ();
        *ppShaderDomain =
          reinterpret_cast <
            SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*
          >             (&shaders->domain);
         break;
      case sk_shader_class::Compute:
        *ppCritical     = cs_shader_cs.get ();
        *ppShaderDomain =
          reinterpret_cast <
            SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*
          >             (&shaders->compute);
         break;
    }
  };

  if (dev_idx == UINT_MAX)
  {
    dev_idx =
      SK_D3D11_GetDeviceContextHandle (pDevCtx);
  }

  SK_Thread_HybridSpinlock*                         pCritical   = nullptr;
  SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>* pShaderRepo = nullptr;

  GetResources (&pCritical, &pShaderRepo);

  if (! implicit_track)
  {
    // ImGui gets to pass-through without invoking the hook
    if (! SK_D3D11_ShouldTrackRenderOp (pDevCtx, dev_idx))
    {  pShaderRepo->tracked.deactivate (pDevCtx, dev_idx);

      return
        _Finish ();
    }
  }

  SK_D3D11_ShaderDesc
     *pDesc    = nullptr;
  if (pShader != nullptr)
  {
    SK_ComPtr <ID3D11Device> pDevice;
    pDevCtx->GetDevice     (&pDevice.p);

    UINT size = sizeof (pDesc);

    if ( FAILED ( ((ID3D11DeviceChild *)pShader)->GetPrivateData (
                          SKID_D3D11KnownShaderDesc, &size, &pDesc )
                )
       )
    {
      pCritical->lock ();

      auto&  rev_map     = pShaderRepo->rev [pDevice];
      auto   shader_desc =
        rev_map.find (pShader);

      if (shader_desc != rev_map.end ())
      {
        ((ID3D11DeviceChild *)pShader)->SetPrivateData (
                SKID_D3D11KnownShaderDesc, sizeof (pDesc),
                                             &shader_desc->second
        );

        pDesc =
          shader_desc->second;
      }

      pCritical->unlock ();
    }

    if (pDesc == nullptr)
    {
      size            =
        sizeof (uint32_t);
      uint32_t crc32c = 0x0;

      if ( SUCCEEDED ( ((ID3D11DeviceChild *)pShader)->GetPrivateData (
                         SKID_D3D11KnownShaderCrc32c, &size,
                                                      &crc32c ) ) )
      {
        dll_log->Log (L"Shader not in cache, so adding it!");

        pCritical->lock ();

        pShaderRepo->descs [pDevice][crc32c].pShader = pShader;
        pShaderRepo->descs [pDevice][crc32c].crc32c  = crc32c;
        pShaderRepo->descs [pDevice][crc32c].type    = pShaderRepo->type_;
        pShader->AddRef ();

        auto& rev_map     = pShaderRepo->rev [pDevice];
        rev_map [pShader] =
          (pDesc = &pShaderRepo->descs [pDevice][crc32c]);

        pCritical->unlock ();
      }

      // We need a way to get the bytecode after the shader was created...
      //   but I'm completely at a loss for how to do that :(
      else
      {
        pDesc = nullptr;

        //dll_log->Log (L"Shader not in cache, and no bytecode available, so ignoring!");
        //
        //std::scoped_lock <SK_Thread_HybridSpinlock> auto_lock (*pCritical);
        //
        //SK_D3D11_ShaderDesc pseudo_desc = { };
        //pseudo_desc.type    = SK_D3D11_ShaderType::Invalid;
        //pseudo_desc.crc32c  = 0x0;
        //pseudo_desc.name    = "Undocumented Alien";
        //pseudo_desc.bytecode.clear ();
        //pseudo_desc.pShader = pShader;
        //pseudo_desc.usage   = { };
        //
        //pShaderRepo->descs [0x0]     = pseudo_desc;
        //pShaderRepo->rev   [pShader] = 0x0;
        //
        //pDesc =
        //  &pShaderRepo->descs [0x0];
      }

      /// We don't know about this shader
      ///else // Late Injection?
    }
  }

  if (pDesc != nullptr)
  {
    InterlockedIncrement (
      &pShaderRepo->changes_last_frame
    );

    const uint32_t checksum =
      pDesc->crc32c;

    if ( checksum !=
           pShaderRepo->tracked.crc32c )
    {
      pShaderRepo->tracked.deactivate (
        pDevCtx, dev_idx
      );
    }

    pShaderRepo->current.shader [dev_idx] = checksum;

  //dll_log->Log (L"Set Shader: %x", checksum);

    ULONG64 frames =
      SK_GetFramesDrawn ();

    if (frames != ReadULong64Acquire (&pDesc->usage.last_frame))
    {
      pDesc->usage.last_time =
             (__time64_t)SK_QueryPerf ().QuadPart;
      WriteULong64Release (&pDesc->usage.last_frame, frames);
    }

    if (! shaders->reshade_triggered)
    {
      if (! pShaderRepo->trigger_reshade.before.empty ())
      {
        reshade_trigger_before [dev_idx] =
          pShaderRepo->trigger_reshade.before.count (checksum) > 0;
      }

      if (! pShaderRepo->trigger_reshade.after.empty ())
      {
        reshade_trigger_after [dev_idx] =
          pShaderRepo->trigger_reshade.after.count (checksum) > 0;
      }
    }

    else
    {
      //reshade_trigger_after  [dev_idx] = false;
      //reshade_trigger_before [dev_idx] = false;
    }

    if (checksum == pShaderRepo->tracked.crc32c)
    {
      pShaderRepo->tracked.activate (
        pDevCtx,
          ppClassInstances,
         NumClassInstances, dev_idx
      );
    }
  }

  else
  {
    if (config.system.log_level > 0 && pShader != nullptr)
      dll_log->Log (L"Shader not found in cache, so draw call missed!");

    pShaderRepo->current.shader [dev_idx] = 0x0;
  }

  return
    _Finish ();

}

#if 0
__forceinline
//__declspec (noinline)
bool
SK_D3D11_ActivateSRVOnSlot (
  UINT                      dev_ctx_handle,
  shader_stage_s&           stage,
  ID3D11ShaderResourceView* pSRV,
  int                       SLOT )
{
  D3D11_RESOURCE_DIMENSION  rDim    = D3D11_RESOURCE_DIMENSION_UNKNOWN;
  ID3D11Resource*           pRes    = nullptr;
  ID3D11Texture2D*          pTex    = nullptr;

  // Our texture wrapper's AddRef method is somewhat heavy,
  //  get an implicit reference from GetResource (...) and
  //    either keep or release it, but do not add another.
  bool                      release = true;

  auto _BindTo = [&]( shader_stage_s::bind_fn   impl,
                      ID3D11ShaderResourceView *pBindingView = nullptr ) ->
  void
  {
    auto bind_func =
      std::bind (impl, &stage, _1, _2, _3);

    if (SLOT < D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT)
    {
      bind_func (
        SLOT, pBindingView
      );
    }

    if (release && pRes != nullptr)
    {
      pRes->Release ();
    }
  };


  if (pSRV == nullptr)
  {
    _BindTo (&shader_stage_s::Bind, pSRV);

    return true;
  }


  D3D11_SHADER_RESOURCE_VIEW_DESC
                  srv_desc = { };
  pSRV->GetDesc (&srv_desc);

  if (srv_desc.ViewDimension == D3D_SRV_DIMENSION_TEXTURE2D)
  {
    pSRV->GetResource (&pRes);

    if (pRes != nullptr)
    {
      pRes->GetType (&rDim);
      assert        ( rDim == D3D11_RESOURCE_DIMENSION_TEXTURE2D );

      if (rDim == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
      {
        pTex =
          static_cast <ID3D11Texture2D *> (pRes);

        int spins = 0;

        auto& res_ctx =
          SK_D3D11_PerCtxResources [dev_ctx_handle];

        while ( 0 !=
                  InterlockedCompareExchange ( &res_ctx.writing_, 1, 0 )
              )
        {
          if ( ++spins > 0x1000 )
          {
            SK_Sleep (1);
            spins  =  0;
          }
        }

        if  (res_ctx.used_textures.insert  (pTex).second)
        {if (res_ctx.temp_resources.insert (pTex).second)
            release = false;
        }

        InterlockedExchange (
          &res_ctx.writing_, 0
        );

        if ( SK_D3D11_TrackedTexture == pTex    &&
             config.textures.d3d11.highlight_debug )
        {
          if ( ( dwFrameTime % tracked_tex_blink_duration ) >
                             ( tracked_tex_blink_duration / 2 ) )
          {
            _BindTo (&shader_stage_s::nulBind, pSRV);

            return false;
          }
        }
      }

      _BindTo (&shader_stage_s::Bind, pSRV);
    }
  }

  else
    _BindTo (&shader_stage_s::nulBind, nullptr);

  return true;
}
#else
//__declspec (noinline)
bool
SK_D3D11_ActivateSRVOnSlot ( shader_stage_s&            stage,
                             ID3D11ShaderResourceView*  pSRV,
                             SK_D3D11_ContextResources& ctx_res,
                             int                        SLOT )
{
  if (pSRV == nullptr)
  {
    if (SLOT < D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT)
      stage.Bind (SLOT, pSRV);

    return true;
  }

  D3D11_RESOURCE_DIMENSION  rDim    = D3D11_RESOURCE_DIMENSION_UNKNOWN;
  ID3D11Resource*           pRes    = nullptr;
  ID3D11Texture2D*          pTex    = nullptr;
  bool                      release = true; // Our texture wrapper's AddRef method is somewhat heavy,
                                            //  get an implicit reference from GetResource (...) and
                                            //    either keep or release it, but do not add another.

  D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = { };
                  pSRV->GetDesc (&srv_desc);

  if ( D3D_SRV_DIMENSION_TEXTURE2D      == srv_desc.ViewDimension ||
       D3D_SRV_DIMENSION_TEXTURE2DMS    == srv_desc.ViewDimension )
  {
    pSRV->GetResource
      (&pRes);
    if (pRes != nullptr)
    {
      pRes->GetType (&rDim);
      assert        ( rDim == D3D11_RESOURCE_DIMENSION_TEXTURE2D);
      if (            rDim == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
      {
        pTex =
          static_cast <ID3D11Texture2D *> (pRes);

        // If underlying texture has no name, maybe this SRV has one?
        if (SK_D3D11_EnableTracking) // Could be expensive, only do
        {                            //   if the user has mod tools open
          if ((! SK_D3D11_HasDebugName (pTex)) &&
                 SK_D3D11_HasDebugName (pSRV))
          {
            std::wstring wide_name =
              SK_D3D11_GetDebugNameW (pSRV);

            if (wide_name.empty ())
                wide_name = SK_UTF8ToWideChar (
                              SK_D3D11_GetDebugNameA (pSRV) );

            SK_D3D11_SetDebugName (pTex, wide_name);
          }
        }

        int spins = 0;

        while ( 0 !=
                  InterlockedCompareExchange (
                    &ctx_res.writing_,
                      1, 0 )
              )
        {
          if ( ++spins > 0x1000 )
          {
            YieldProcessor ();
            spins     =     0;
          }
        }

        if ( ctx_res.used_textures.emplace  (pTex).second )
        {    ctx_res.temp_resources.emplace (pTex);
        }

        InterlockedExchange (
          &ctx_res.writing_, 0
        );

        if ( SK_D3D11_TrackedTexture == pTex    &&
             config.textures.d3d11.highlight_debug )
        {
          if ( ( SK::ControlPanel::current_time % tracked_tex_blink_duration ) >
                                                ( tracked_tex_blink_duration / 2 ) )
          {
            if (SLOT < D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT)
              stage.nulBind (SLOT, pSRV);

            if (release) pRes->Release ();

            return false;
          }
        }
      }


      if (SLOT < D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT)
        stage.Bind (SLOT, pSRV);

      if (release) pRes->Release ();
    }
  }
  else
    stage.nulBind (SLOT, nullptr);

  return true;
}
#endif

void
STDMETHODCALLTYPE
SK_D3D11_SetShaderResources_Impl (
   SK_D3D11_ShaderType  ShaderType,
   BOOL                 Deferred,
   ID3D11DeviceContext *This,       // non-null indicates hooked function
   ID3D11DeviceContext *Wrapper,    // non-null indicates a wrapper
   UINT                 StartSlot,
   UINT                 NumViews,
   _In_opt_             ID3D11ShaderResourceView* const *ppShaderResourceViews,
   UINT                 dev_idx )
{
  const bool bWrapped = Wrapper != nullptr;

  UNREFERENCED_PARAMETER (Deferred);

  auto& shaders =
    SK_D3D11_Shaders;

  bool
    hooked    = ( This != nullptr );
  LPVOID
    pTargetFn = nullptr;
  LPVOID*
    _vftable  =
      ( Wrapper != nullptr ?
          *reinterpret_cast <LPVOID **>(Wrapper) :
                   nullptr );
  ID3D11DeviceContext*
    pDevContext =
      ( hooked ?
          This : Wrapper );

  if (! (hooked || _vftable))
    return;

  SK_Thread_HybridSpinlock*
      cs_lock     = nullptr;

  SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*
      shader_base = nullptr;

  int stage_id    = 0;

  switch (ShaderType)
  {
    case SK_D3D11_ShaderType::Vertex:
      pTargetFn   = ( hooked ? D3D11_VSSetShaderResources_Original :
                               _vftable [25] );
      break;
    case SK_D3D11_ShaderType::Pixel:
      pTargetFn   = ( hooked ? D3D11_PSSetShaderResources_Original :
                               _vftable [8] );
      break;
    case SK_D3D11_ShaderType::Geometry:
      pTargetFn   = ( hooked ? D3D11_GSSetShaderResources_Original :
                               _vftable [31] );
      break;
    case SK_D3D11_ShaderType::Hull:
      pTargetFn   = ( hooked ? D3D11_HSSetShaderResources_Original :
                               _vftable [59] );
      break;
    case SK_D3D11_ShaderType::Domain:
      pTargetFn   = ( hooked ? D3D11_DSSetShaderResources_Original :
                               _vftable [63] );
      break;
    case SK_D3D11_ShaderType::Compute:
      pTargetFn   = ( hooked ? D3D11_CSSetShaderResources_Original :
                               _vftable [67] );
      break;

    default:
      break;
  }

  assert (shader_base != nullptr);
  assert (cs_lock     != nullptr);

  auto _Finish =
  [&](void) -> void
   {
     static_cast <D3D11_VSSetShaderResources_pfn> (pTargetFn) (
       pDevContext, StartSlot, NumViews, ppShaderResourceViews);
  };

  if (! SK_D3D11_ShouldTrackSetShaderResources (pDevContext, dev_idx))
  {
    return
      _Finish ();
  }

  switch (ShaderType)
  {
    case SK_D3D11_ShaderType::Vertex:
    { static auto vs_base =
        reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*>
                   (&shaders->vertex);
      shader_base =  vs_base;
      stage_id    = VERTEX_SHADER_STAGE;
      cs_lock     = cs_shader_vs.get ();
    } break;

    case SK_D3D11_ShaderType::Pixel:
    {
      static auto ps_base =
        reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*>
                   (&shaders->pixel);
      shader_base =  ps_base;
      stage_id    = PIXEL_SHADER_STAGE;
      cs_lock     = cs_shader_ps.get ();
    } break;

    case SK_D3D11_ShaderType::Geometry:
    {
      static auto gs_base =
        reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*>
                   (&shaders->geometry);
      shader_base =  gs_base;
      stage_id    = GEOMETRY_SHADER_STAGE;
      cs_lock     = cs_shader_gs.get ();
    } break;

    case SK_D3D11_ShaderType::Hull:
    {
      static auto hs_base =
        reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*>
                   (&shaders->hull);
      shader_base =  hs_base;
      stage_id    = HULL_SHADER_STAGE;
      cs_lock     = cs_shader_hs.get ();
    } break;

    case SK_D3D11_ShaderType::Domain:
    {
      static auto ds_base =
        reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*>
                   (&shaders->domain);
      shader_base =  ds_base;
      stage_id    = DOMAIN_SHADER_STAGE;
      cs_lock     = cs_shader_ds.get ();
    } break;

    case SK_D3D11_ShaderType::Compute:
    {
      static auto cs_base =
        reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*>
                   (&shaders->compute);
      shader_base =  cs_base;
      stage_id    = COMPUTE_SHADER_STAGE;
      cs_lock     = cs_shader_cs.get ();
    } break;

    default:
      break;
  }

  assert (shader_base != nullptr);
  assert (cs_lock     != nullptr);

#define pDevCtx pDevContext
  SK_WRAP_AND_HOOK
#undef  pDevCtx

  bool early_out = (! bMustNotIgnore) || shader_base == nullptr ||
  !SK_D3D11_ShouldTrackRenderOp           (pDevContext, dev_idx)||
    SK_D3D11_IgnoreWrappedOrDeferred (bWrapped,
                                      bIsDevCtxDeferred,
                                      pDevContext);

  if (early_out)
  {
    return
      _Finish ();
  }

  if (dev_idx == UINT_MAX)
      dev_idx  = SK_D3D11_GetDeviceContextHandle (pDevContext);


  auto& views =
    shader_base->current.views [dev_idx];

  if (ppShaderResourceViews != nullptr && NumViews > 0)
  {
    RtlCopyMemory (
         &views [StartSlot],
      ppShaderResourceViews,
                   NumViews * sizeof (ID3D11ShaderResourceView*)
    );
  }

  else
    ZeroMemory (shader_base->current.views [dev_idx], NumViews * sizeof (ID3D11ShaderResourceView));


  if (ppShaderResourceViews != nullptr && NumViews > 0)
  {
    auto&& newResourceViews =
      shader_base->current.tmp_views [dev_idx];

    RtlCopyMemory (
           newResourceViews,
      ppShaderResourceViews,
                   NumViews * sizeof (ID3D11ShaderResourceView*)
    );

    d3d11_shader_tracking_s& tracked =
      shader_base->tracked;

    auto& stage = d3d11_shader_stages [stage_id][dev_idx];

    const uint32_t shader_crc32c = tracked.crc32c.load ();
    const bool     active        = tracked.active.get  (dev_idx);

    auto& set_resources = tracked.set_of_res;
    auto& set_views     = tracked.set_of_views;

    auto& ctx_res  =
      SK_D3D11_PerCtxResources [dev_idx];

    auto& temp_res =
          ctx_res.temp_resources;

    for ( UINT i = StartSlot;
               i < std::min (StartSlot + NumViews, (UINT)D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);
             ++i )
    {
      if (! SK_D3D11_ActivateSRVOnSlot ( stage,
                                           ppShaderResourceViews [i - StartSlot],
                                             ctx_res,
                                               i )
         )
      {
        newResourceViews [i - StartSlot] = nullptr;
      }
    }

    // Deeper dive through resource tracking if the
    //   current shader matches the debug tracker
    if ( (active && shader_crc32c == tracked.crc32c) && cs_lock != nullptr)
    {
      std::lock                                             (*cs_lock,        *cs_render_view);
      std::lock_guard <SK_Thread_HybridSpinlock> auto_lock1 (*cs_lock,        std::adopt_lock);
      std::lock_guard <SK_Thread_HybridSpinlock> auto_lock2 (*cs_render_view, std::adopt_lock);

      for (UINT i = 0; i < NumViews; i++)
      {
        if (StartSlot + i >= D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT)
          break;

        auto* pSRV =
          ppShaderResourceViews [i];

        if (pSRV != nullptr)
        {
          if (   set_views.insert     ( pSRV ).second)
          {           temp_res.insert ( pSRV );
            SK_ComPtr <ID3D11Resource>  pRes;
                   pSRV->GetResource  (&pRes.p );
            if ( set_resources.insert ( pRes.p ).second)
                      temp_res.insert ( pRes.p );
          }
        }
      }
    }

    ppShaderResourceViews =
      newResourceViews;

    // Quick, before that goes out of scope!
    return
      _Finish ();
  }

  return
    _Finish ();
}

void
SK_D3D11_ClearShaderState (void)
{
  auto& shaders =
    SK_D3D11_Shaders;

  for (int i = 0; i < 6; i++)
  {
    const
     auto
      shader_record =
      [&]
      {
        switch (i)
        {
          default:
          case 0:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders->vertex;
          case 1:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders->pixel;
          case 2:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders->geometry;
          case 3:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders->hull;
          case 4:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders->domain;
          case 5:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders->compute;
        }
      };

    auto record =
      shader_record ();

    size_t always_tracked =
      record->blacklist.size             () + record->blacklist_if_texture.size   () +
      record->wireframe.size             () + record->on_top.size                 () +
      record->trigger_reshade.after.size () + record->trigger_reshade.before.size ();

    size_t texture_based = record->blacklist_if_texture.size ();

    record->blacklist.clear              ();
    record->blacklist_if_texture.clear   ();
    record->wireframe.clear              ();
    record->on_top.clear                 ();
    record->trigger_reshade.before.clear ();
    record->trigger_reshade.after.clear  ();

    ///SK_ReleaseAssert (always_tracked <= (size_t)ReadAcquire (&SK_D3D11_DrawTrackingReqs));
    ///SK_ReleaseAssert (always_tracked <= (size_t)ReadAcquire (&SK_D3D11_TrackingCount->Always));

    InterlockedAdd (      &SK_D3D11_DrawTrackingReqs,
      -static_cast <LONG> (always_tracked) );
    InterlockedAdd (      &SK_D3D11_TrackingCount->Always,
      -static_cast <LONG> (always_tracked) );
    InterlockedAdd (      &SK_D3D11_TrackingCount->TextureBased,
      -static_cast <LONG> (texture_based)  );

#if 0
    LONG conditionally_tracked =
      record->hud.size                   ();

    record->hud.clear                    ();

    SK_ReleaseAssert (conditionally_tracked <= ReadAcquire (&SK_D3D11_TrackingCount->Conditional));

    InterlockedAdd (      &SK_D3D11_TrackingCount->Conditional,
      -static_cast <LONG> (conditionally_tracked) );
#endif
  }
};

std::unordered_map <std::wstring, bool>&
_SK_D3D11_LoadedShaderConfigs (void)
{
  static std::unordered_map <std::wstring, bool> __loaded;
  return                                         __loaded;
}

void
SK_D3D11_LoadShaderStateEx (const std::wstring& name, bool clear)
{
  auto& requested =
    _SK_D3D11_LoadedShaderConfigs ();

  auto wszShaderConfigFile =
    SK_FormatStringW ( LR"(%s\%s)",
                         SK_GetConfigPath (), name.c_str () );

  if (GetFileAttributesW (wszShaderConfigFile.c_str ()) == INVALID_FILE_ATTRIBUTES)
  {
    // No shader config file, nothing to do here...
    return;
  }

  std::unique_ptr <iSK_INI> d3d11_shaders_ini (
    SK_CreateINI (wszShaderConfigFile.c_str ())
  );

  //d3d11_shaders_ini->parse ();

  int shader_class = 0;

  iSK_INI::_TSectionMap& sections =
    d3d11_shaders_ini->get_sections ();

  auto sec =
    sections.begin ();

  struct draw_state_s {
    std::set <uint32_t>                       wireframe;
    std::set <uint32_t>                       on_top;
    std::set <uint32_t>                       disable;
    std::map <uint32_t, std::set <uint32_t> > disable_if_texture;
    std::set <uint32_t>                       trigger_reshade_before;
    std::set <uint32_t>                       trigger_reshade_after;
    std::set <uint32_t>                       hud_shader;
  } draw_states [7];

  auto shader_class_idx =
  [](const std::wstring& _name)
  {
    if (_name == L"Vertex")   return 0;
    if (_name == L"Pixel")    return 1;
    if (_name == L"Geometry") return 2;
    if (_name == L"Hull")     return 3;
    if (_name == L"Domain")   return 4;
    if (_name == L"Compute")  return 5;
                              return 6;
  };

  while (sec != sections.end ())
  {
    if (std::wcsstr ((*sec).first.c_str (), L"DrawState."))
    {
      wchar_t wszClass [32] = { };

      _snwscanf ((*sec).first.c_str (), 31, L"DrawState.%31s", wszClass);

      shader_class =
        shader_class_idx (wszClass);

      for ( auto& it : (*sec).second.keys )
      {
        unsigned int                        shader = 0U;
        swscanf (it.first.c_str (), L"%x", &shader);

        CHeapPtr <wchar_t> wszState (
          _wcsdup (it.second.c_str ())
        );

        wchar_t* wszBuf = nullptr;
        wchar_t* wszTok =
          std::wcstok (wszState, L",", &wszBuf);

        while (wszTok)
        {
          if (     0 == _wcsicmp (wszTok, L"Wireframe"))
            draw_states [shader_class].wireframe.emplace (shader);
          else if (0 == _wcsicmp (wszTok, L"OnTop"))
            draw_states [shader_class].on_top.emplace (shader);
          else if (0 == _wcsicmp (wszTok, L"Disable"))
            draw_states [shader_class].disable.emplace (shader);
          else if (0 == _wcsicmp (wszTok, L"TriggerReShade"))
          {
            draw_states [shader_class].trigger_reshade_before.emplace (shader);
          }
          else if (0 == _wcsicmp (wszTok, L"TriggerReShadeAfter"))
          {
            draw_states [shader_class].trigger_reshade_after.emplace (shader);
          }
          else if ( StrStrIW (wszTok, L"DisableIfTexture=") == wszTok &&
                 std::wcslen (wszTok) >
                         std::wcslen (L"DisableIfTexture=") ) // Skip the degenerate case
          {
            uint32_t                                  crc32c;
            swscanf (wszTok, L"DisableIfTexture=%x", &crc32c);

            draw_states [shader_class].disable_if_texture [shader].insert (crc32c);
          }
          else if (0 == _wcsicmp (wszTok, L"HUD"))
          {
            draw_states [shader_class].hud_shader.emplace (shader);
          }

          wszTok =
            std::wcstok (nullptr, L",", &wszBuf);
        }
      }
    }

    ++sec;
  }

  auto& shaders =
    SK_D3D11_Shaders;


  size_t always_tracked_in_ini = 0;
//size_t always_tracked_orig   = ReadAcquire (&SK_D3D11_TrackingCount->Always);

  if (clear)
  {
    SK_D3D11_ClearShaderState ();
    sections.clear            ();
  }

  static const
    std::pair <int, SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*>
      shader_records [] =
      {
        { 0, (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders->vertex   },
        { 1, (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders->pixel    },
        { 2, (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders->geometry },
        { 3, (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders->hull     },
        { 4, (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders->domain   },
        { 5, (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders->compute  }
      };

  for ( auto& pair : shader_records )
  {
    int   i      = pair.first;
    auto& record = pair.second;

#pragma region StateTrackingCount
    size_t num_always_tracked_per_stage = 0;

    num_always_tracked_per_stage += draw_states [i].disable.size            ();
    num_always_tracked_per_stage += draw_states [i].disable_if_texture.size ();
    num_always_tracked_per_stage += draw_states [i].wireframe.size          ();
    num_always_tracked_per_stage += draw_states [i].on_top.size             ();

    if ((intptr_t)SK_ReShade_GetDLL () > 0)
    {
      num_always_tracked_per_stage += draw_states [i].trigger_reshade_before.size ();
      num_always_tracked_per_stage += draw_states [i].trigger_reshade_after.size  ();
    }

    size_t num_conditionally_tracked  = 0;
           num_conditionally_tracked += draw_states [i].hud_shader.size ();

    size_t num_texture_tracked  = 0;
           num_texture_tracked += draw_states [i].disable_if_texture.size ();
#pragma endregion

    for ( auto& it : draw_states [i].disable)    record->addTrackingRef (record->blacklist, it);
    for ( auto& it : draw_states [i].wireframe)  record->addTrackingRef (record->wireframe, it);
    for ( auto& it : draw_states [i].on_top)     record->addTrackingRef (record->on_top,    it);
    for ( auto& it : draw_states [i].hud_shader) record->addTrackingRef (record->hud,       it);

    for ( auto& it : draw_states [i].trigger_reshade_before)
     record->addTrackingRef (record->trigger_reshade.before, it);
    for ( auto& it : draw_states [i].trigger_reshade_after)
     record->addTrackingRef (record->trigger_reshade.after,  it);

    for ( auto& it : draw_states [i].disable_if_texture )
    {
      record->blacklist_if_texture [it.first].insert (
        it.second.cbegin (),
          it.second.cend ()
      );
    }

    if ( num_always_tracked_per_stage > 0 ||
         num_conditionally_tracked    > 0   )
    {
      always_tracked_in_ini += num_always_tracked_per_stage;

      auto shader_class_name = [](int i)
      {
        switch (i)
        {
          default:
          case 0: return L"Vertex";
          case 1: return L"Pixel";
          case 2: return L"Geometry";
          case 3: return L"Hull";
          case 4: return L"Domain";
          case 5: return L"Compute";
        };
      };


      SK_LOG0 ( ( L"Shader Stage '%s' Contributes %zu Always-Tracked and "
                                                L"%zu Conditionally-Tracked States "
                  L"to D3D11 State Tracker",
                    shader_class_name (i),
                      num_always_tracked_per_stage,
                      num_conditionally_tracked     ),
                  L"StateTrack"
              );

      if (! clear)
      {
        InterlockedAdd (&SK_D3D11_TrackingCount->Always,       sk::narrow_cast <LONG> (num_always_tracked_per_stage));
        InterlockedAdd (&SK_D3D11_TrackingCount->Conditional,  sk::narrow_cast <LONG> (num_conditionally_tracked));
        InterlockedAdd (&SK_D3D11_TrackingCount->TextureBased, sk::narrow_cast <LONG> (num_texture_tracked));
      }
    }
  }

  if ( sections.empty () || clear )
  {
    LONG to_dec = 0L;

    if ( requested.count (name) &&
               requested [name] == true )
    {
      requested [name] = false;

#if 0
      SK_ReleaseAssert (! sections.empty ());
#endif

      to_dec =
        SK_D3D11_TrackingCount->Always;
    }

    InterlockedAdd (&SK_D3D11_DrawTrackingReqs, -to_dec);
  }

  if (always_tracked_in_ini > 0 || (! sections.empty ()))
  {
    if ( (! requested.count (name)) ||
                  requested [name]  == false )
    {
      requested [name] = true;

      LONG to_inc =
        sk::narrow_cast <LONG> (always_tracked_in_ini);

      InterlockedAdd (&SK_D3D11_DrawTrackingReqs, to_inc);
    }
  }
}

void
SK_D3D11_LoadShaderState (bool clear)
{
  SK_D3D11_LoadShaderStateEx (L"d3d11_shaders.ini", clear);
}

void
SK_D3D11_UnloadShaderState (std::wstring& name)
{
  auto wszShaderConfigFile =
    SK_FormatStringW ( LR"(%s\%s)",
                         SK_GetConfigPath (), name.c_str () );

  if (GetFileAttributesW (wszShaderConfigFile.c_str ()) == INVALID_FILE_ATTRIBUTES)
  {
    // No shader config file, nothing to do here...
    return;
  }

  std::unique_ptr <iSK_INI> d3d11_shaders_ini (
    SK_CreateINI (wszShaderConfigFile.c_str ())
  );

  int shader_class = 0;

  const iSK_INI::_TSectionMap& sections =
    d3d11_shaders_ini->get_sections ();

  auto sec =
    sections.begin ();

  struct draw_state_s {
    std::set <uint32_t>                       wireframe;
    std::set <uint32_t>                       on_top;
    std::set <uint32_t>                       disable;
    std::map <uint32_t, std::set <uint32_t> > disable_if_texture;
    std::set <uint32_t>                       trigger_reshade_before;
    std::set <uint32_t>                       trigger_reshade_after;
    std::set <uint32_t>                       hud_shader;
  } draw_states [7];

  auto shader_class_idx = [](const std::wstring& name)
  {
    if (name == L"Vertex")   return 0;
    if (name == L"Pixel")    return 1;
    if (name == L"Geometry") return 2;
    if (name == L"Hull")     return 3;
    if (name == L"Domain")   return 4;
    if (name == L"Compute")  return 5;
                             return 6;
  };

  while (sec != sections.cend ())
  {
    if (std::wcsstr ((*sec).first.c_str (), L"DrawState."))
    {
      wchar_t wszClass [32] = { };

      _snwscanf ((*sec).first.c_str (), 31, L"DrawState.%31s", wszClass);

      shader_class =
        shader_class_idx (wszClass);

      for ( auto& it : (*sec).second.keys )
      {
        unsigned int                        shader = 0U;
        swscanf (it.first.c_str (), L"%x", &shader);

        CHeapPtr <wchar_t> wszState (
          _wcsdup (it.second.c_str ())
        );

        wchar_t* wszBuf = nullptr;
        wchar_t* wszTok =
          std::wcstok (wszState, L",", &wszBuf);

        while (wszTok)
        {
          if (     0 == _wcsicmp (wszTok, L"Wireframe"))
            draw_states [shader_class].wireframe.emplace (shader);
          else if (0 == _wcsicmp (wszTok, L"OnTop"))
            draw_states [shader_class].on_top.emplace (shader);
          else if (0 == _wcsicmp (wszTok, L"Disable"))
            draw_states [shader_class].disable.emplace (shader);
          else if (0 == _wcsicmp (wszTok, L"TriggerReShade"))
          {
            draw_states [shader_class].trigger_reshade_before.emplace (shader);
          }
          else if (0 == _wcsicmp (wszTok, L"TriggerReShadeAfter"))
          {
            draw_states [shader_class].trigger_reshade_after.emplace (shader);
          }
          else if ( StrStrIW (wszTok, L"DisableIfTexture=") == wszTok &&
                 std::wcslen (wszTok) >
                         std::wcslen (L"DisableIfTexture=") ) // Skip the degenerate case
          {
            uint32_t                                  crc32c;
            swscanf (wszTok, L"DisableIfTexture=%x", &crc32c);

            draw_states [shader_class].disable_if_texture [shader].emplace (crc32c);
          }
          else if (0 == _wcsicmp (wszTok, L"HUD"))
          {
            draw_states [shader_class].hud_shader.emplace (shader);
          }

          wszTok =
            std::wcstok (nullptr, L",", &wszBuf);
        }
      }
    }

    ++sec;
  }

  auto& shaders =
    SK_D3D11_Shaders;

  size_t tracking_reqs_removed    = 0;
  size_t conditional_reqs_removed = 0;
  size_t texture_reqs_removed     = 0;

  for (int i = 0; i < 6; i++)
  {
   const
    auto
     shader_record =
      [&]
      {
        switch (i)
        {
          default:
          case 0:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders->vertex;
          case 1:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders->pixel;
          case 2:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders->geometry;
          case 3:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders->hull;
          case 4:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders->domain;
          case 5:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders->compute;
        }
      };

    auto record = shader_record ();

    tracking_reqs_removed +=
      ( draw_states [i].disable.size   ()                          +
        draw_states [i].wireframe.size ()                          +
        draw_states [i].on_top.size    ()                          +
        ( (intptr_t)SK_ReShade_GetDLL  () > 0 ?
          ( draw_states [i].trigger_reshade_before.size () +
            draw_states [i].trigger_reshade_after.size  () ) : 0 ) +
        draw_states [i].disable_if_texture.size () );


    for (auto& it : draw_states [i].disable)   record->releaseTrackingRef (record->blacklist, it);
    for (auto& it : draw_states [i].wireframe) record->releaseTrackingRef (record->wireframe, it);
    for (auto& it : draw_states [i].on_top)    record->releaseTrackingRef (record->on_top,    it);

    for (auto& it : draw_states [i].trigger_reshade_before)
      record->releaseTrackingRef (record->trigger_reshade.before, it);

    for (auto& it : draw_states [i].trigger_reshade_after)
      record->releaseTrackingRef (record->trigger_reshade.after, it);

    for ( auto& it : draw_states [i].disable_if_texture )
    {
      for (auto& it2 : it.second)
      {
        record->blacklist_if_texture [it.first].erase (it2);
      }
    }

    texture_reqs_removed +=
      draw_states [i].disable_if_texture.size ();
    conditional_reqs_removed +=
      draw_states [i].hud_shader.size ();

    for (auto& it : draw_states [i].hud_shader)
      record->releaseTrackingRef (record->hud, it);
  }


  auto& requested =
    _SK_D3D11_LoadedShaderConfigs ();

  if (requested.count (name))
  {
    if (requested [name] == true)
    {   requested [name]  = false;
      InterlockedAdd (&SK_D3D11_DrawTrackingReqs, -sk::narrow_cast <LONG> (tracking_reqs_removed));

    //InterlockedDecrement (&SK_D3D11_DrawTrackingReqs);

      SK_ReleaseAssert ( tracking_reqs_removed    > 0 ||
                         conditional_reqs_removed > 0 );

      InterlockedAdd (&SK_D3D11_TrackingCount->Always,       -sk::narrow_cast <LONG> (tracking_reqs_removed));
      InterlockedAdd (&SK_D3D11_TrackingCount->Conditional,  -sk::narrow_cast <LONG> (conditional_reqs_removed));
      InterlockedAdd (&SK_D3D11_TrackingCount->TextureBased, -sk::narrow_cast <LONG> (texture_reqs_removed));
    }

    else
    {
      if (config.system.log_level > 1)
      {
        SK_ReleaseAssert ( tracking_reqs_removed    == 0 &&
                           conditional_reqs_removed == 0 &&
                           texture_reqs_removed     == 0 );
      }
    }
  }
}



void
EnumConstantBuffer ( ID3D11ShaderReflectionConstantBuffer* pConstantBuffer,
                     shader_disasm_s::constant_buffer&     cbuffer )
{
  if (pConstantBuffer == nullptr)
    return;

  D3D11_SHADER_BUFFER_DESC cbuffer_desc = { };

  if (SUCCEEDED (pConstantBuffer->GetDesc (&cbuffer_desc)))
  {
    //if (constant_desc.DefaultValue != nullptr)
    //  memcpy (constant.Data, constant_desc.DefaultValue, std::min (static_cast <size_t> (constant_desc.Bytes), sizeof (float) * 4));

    cbuffer.structs.emplace_back ();

    shader_disasm_s::constant_buffer::struct_ent& unnamed_struct =
      cbuffer.structs.back ();

    for ( UINT j = 0; j < cbuffer_desc.Variables; j++ )
    {
      ID3D11ShaderReflectionVariable* pVar =
        pConstantBuffer->GetVariableByIndex (j);

      shader_disasm_s::constant_buffer::variable var = { };

      if (pVar != nullptr)
      {
        auto* type =
          pVar->GetType ();

        if (SUCCEEDED (type->GetDesc (&var.type_desc)))
        {
          if (SUCCEEDED (pVar->GetDesc (&var.var_desc)) && (var.var_desc.uFlags & D3D_SVF_USED) &&
                                                         !( var.var_desc.uFlags & D3D_SVF_USERPACKED ))
          {
            var.name = var.var_desc.Name;

            if (var.type_desc.Class == D3D_SVC_STRUCT)
            {
              shader_disasm_s::constant_buffer::struct_ent this_struct;
              this_struct.second = var.name;

              this_struct.first.emplace_back (var);

              // Stupid convoluted CBuffer struct design --
              //
              //   It is far simpler to do this ( with proper recursion ) in D3D9.
              for ( UINT k = 0 ; k < var.type_desc.Members ; k++ )
              {
                ID3D11ShaderReflectionType* mem_type = pVar->GetType ()->GetMemberTypeByIndex (k);
                D3D11_SHADER_TYPE_DESC      mem_type_desc = { };

                shader_disasm_s::constant_buffer::variable mem_var = { };

                mem_type->GetDesc (&mem_var.type_desc);

                if ( k == var.type_desc.Members - 1 )
                  mem_var.var_desc.Size = ( var.var_desc.Size / std::max (1ui32, var.type_desc.Elements) ) - mem_var.type_desc.Offset;

                else
                {
                  D3D11_SHADER_TYPE_DESC next_type_desc = { };

                  pVar->GetType ()->GetMemberTypeByIndex (k + 1)->GetDesc (&next_type_desc);

                  mem_var.var_desc.Size = next_type_desc.Offset - mem_var.type_desc.Offset;
                }

                mem_var.var_desc.StartOffset = var.var_desc.StartOffset + mem_var.type_desc.Offset;
                mem_var.name                 = pVar->GetType ()->GetMemberTypeName (k);

                dll_log->Log  (L"%hs.%hs <%lu> {%lu}", this_struct.second.c_str (), mem_var.name.c_str (), mem_var.var_desc.StartOffset, mem_var.var_desc.Size);

                this_struct.first.emplace_back (mem_var);
              }

              cbuffer.structs.emplace_back (this_struct);
            }

            else if (var.type_desc.Class < D3D_SVC_OBJECT)
            {
              ////auto orig_se =
              ////  SK_SEH_ApplyTranslator (
              ////    SK_FilteringStructuredExceptionTranslator (
              ////      EXCEPTION_ACCESS_VIOLATION
              ////    )
              ////  );
              ////
              ////// Disassembler sometimes has dangling pointers ... WTF?!
              ////try
              ////{
              ////  memset ( (void *)var.default_value, 0, sizeof (var.default_value) );
              ////
              ////  if (var.var_desc.DefaultValue != nullptr && SK_ValidatePointer (var.var_desc.DefaultValue))
              ////  {
              ////    memcpy ( (void *)var.default_value,
              ////                     var.var_desc.DefaultValue,
              ////  std::min ( sizeof (var.default_value),
              ////             (size_t)var.var_desc.Size) );
              ////  }
              ////}
              ////
              ////catch (const SK_SEH_IgnoredException&)
              ////{
              ////  dll_log->Log ( L" -> Dangling Pointer for Variable Default Value: '%hs'",
              ////                   var.var_desc.Name );
              ////}
              ////SK_SEH_RemoveTranslator (orig_se);

              unnamed_struct.first.emplace_back (var);
            }

            else
            {
              dll_log->Log ( L" -> Unexpected Shader Type Class: '%lu', for Variable '%hs'",
                            static_cast <UINT> (var.type_desc.Class),
                                                var.var_desc.Name );
            }
          }
        }
      }
    }
  }
};


void
SK_D3D11_StoreShaderState (void)
{
  auto wszShaderConfigFile =
    SK_FormatStringW ( LR"(%s\d3d11_shaders.ini)",
                         SK_GetConfigPath () );

  std::unique_ptr <iSK_INI> d3d11_shaders_ini (
    SK_CreateINI (wszShaderConfigFile.c_str ())
  );

  if (! d3d11_shaders_ini->get_sections ().empty ())
  {
    auto& secs =
      d3d11_shaders_ini->get_sections ();

    for ( auto& it : secs )
    {
      d3d11_shaders_ini->remove_section (it.first.c_str ());
    }
  }

  auto shader_class_name = [](int i)
  {
    switch (i)
    {
      default:
      case 0: return L"Vertex";
      case 1: return L"Pixel";
      case 2: return L"Geometry";
      case 3: return L"Hull";
      case 4: return L"Domain";
      case 5: return L"Compute";
    };
  };

  auto& shaders =
    SK_D3D11_Shaders;

  for (int i = 0; i < 6; i++)
  {
   const
    auto
     shader_record =
      [&]
      {
        switch (i)
        {
          default:
          case 0:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders->vertex;
          case 1:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders->pixel;
          case 2:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders->geometry;
          case 3:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders->hull;
          case 4:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders->domain;
          case 5:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders->compute;
        }
      };

    iSK_INISection& sec =
      d3d11_shaders_ini->get_section_f (L"DrawState.%s", shader_class_name (i));

    std::set <uint32_t> shader_set;

    const auto& record =
      shader_record ();

    for (auto& it : record->blacklist)              shader_set.insert (it.first);
    for (auto& it : record->wireframe)              shader_set.insert (it.first);
    for (auto& it : record->on_top)                 shader_set.insert (it.first);
    for (auto& it : record->trigger_reshade.before) shader_set.insert (it.first);
    for (auto& it : record->trigger_reshade.after)  shader_set.insert (it.first);
    for (auto& it : record->hud)                    shader_set.insert (it.first);

    for (auto& it : record->blacklist_if_texture)
    {
      shader_set.emplace ( it.first );
    }

    for ( auto& it : shader_set )
    {
      std::queue <std::wstring> states;

      if (record->blacklist.count (it))
        states.emplace (L"Disable");

      if (record->wireframe.count (it))
        states.emplace (L"Wireframe");

      if (record->on_top.count (it))
        states.emplace (L"OnTop");

      if (record->blacklist_if_texture.count (it))
      {
        for ( auto& it2 : record->blacklist_if_texture [it] )
        {
          states.emplace (SK_FormatStringW (L"DisableIfTexture=%x", it2));
        }
      }

      if (record->trigger_reshade.before.count (it))
      {
        states.emplace (L"TriggerReShade");
      }

      if (record->trigger_reshade.after.count (it))
      {
        states.emplace (L"TriggerReShadeAfter");
      }

      if (record->hud.count (it))
      {
        states.emplace (L"HUD");
      }

      if (! states.empty ())
      {
        std::wstring state = L"";

        while (! states.empty ())
        {
          state += states.front ();
                   states.pop   ();

          if (! states.empty ()) state += L",";
        }

        sec.add_key_value ( SK_FormatStringW (L"%08x", it).c_str (), state.c_str ());
      }
    }
  }

  d3d11_shaders_ini->get_allow_empty () = true;
  d3d11_shaders_ini->write           ();
}


// Creates a temporary texture with an internal format (i.e. single-sampled or not typeless)
//   that can actually be drawn. The returned SRV holds a reference to the underlying texture,
//     just throw the SRV away and the temporary texture will self-destruct instead of leaking.
HRESULT
SK_D3D11_MakeDrawableCopy ( ID3D11Device              *pDevice,
                            ID3D11Texture2D           *pUndrawableTexture,
                            ID3D11RenderTargetView    *pUndrawableRenderTarget, // Optional
                            ID3D11ShaderResourceView **ppCopyView )
{
  if (pDevice == nullptr || pUndrawableTexture == nullptr || ppCopyView == nullptr)
    return E_POINTER;

  D3D11_TEXTURE2D_DESC          tex_desc = { };
  pUndrawableTexture->GetDesc (&tex_desc);

  D3D11_RENDER_TARGET_VIEW_DESC          rtv_desc = { };
  if (pUndrawableRenderTarget != nullptr)
      pUndrawableRenderTarget->GetDesc (&rtv_desc);

   if (rtv_desc.Format == DXGI_FORMAT_UNKNOWN || pUndrawableRenderTarget == nullptr)
       rtv_desc.Format  = tex_desc.Format;

  D3D11_SHADER_RESOURCE_VIEW_DESC
    srv_desc                           = { };
    srv_desc.ViewDimension             = D3D_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Format                    = SK_DXGI_MakeTypedFormat (rtv_desc.Format);
    srv_desc.Texture2D.MipLevels       = 1;
    srv_desc.Texture2D.MostDetailedMip = 0;

  // Do we need a special copy in the first place?
  if (((tex_desc.BindFlags & D3D11_BIND_SHADER_RESOURCE)
                          != D3D11_BIND_SHADER_RESOURCE)                       ||
                                       tex_desc.Usage   == D3D11_USAGE_DYNAMIC ||
              SK_DXGI_MakeTypedFormat (tex_desc.Format) != tex_desc.Format     ||

        ( pUndrawableRenderTarget != nullptr &&
            ( SK_DXGI_MakeTypedFormat (rtv_desc.Format) != rtv_desc.Format     ||
                                       rtv_desc.Format  != tex_desc.Format ) ) ||

                                       tex_desc.SampleDesc.Count > 1 )
  {
    tex_desc.Format =
    rtv_desc.Format;

    D3D11_TEXTURE2D_DESC
      drawable_desc                = tex_desc;
      drawable_desc.SampleDesc.Count
                                       = 1;
      drawable_desc.SampleDesc.Quality = 0;
      drawable_desc.BindFlags          = D3D11_BIND_SHADER_RESOURCE;
      drawable_desc.Usage              = D3D11_USAGE_DEFAULT;
      drawable_desc.CPUAccessFlags     = 0x0;
      drawable_desc.Format             =
        SK_DXGI_MakeTypedFormat (tex_desc.Format);
      drawable_desc.MipLevels          = 1;
      drawable_desc.MiscFlags          = 0;

    // We're about to create a temporary texture that Special K would
    //   otherwise intercept and try to cache.
    SK_ScopedBool decl_tex_scope (
      SK_D3D11_DeclareTexInjectScope ()
    );

    SK_ComPtr <ID3D11Texture2D>                            pDrawableTex;
    HRESULT hr =
      pDevice->CreateTexture2D ( &drawable_desc, nullptr, &pDrawableTex.p );

    if (SUCCEEDED (hr))
    {
      SK_ComPtr <ID3D11DeviceContext> pDevCtx;
      pDevice->GetImmediateContext  (&pDevCtx.p);

      if (tex_desc.SampleDesc.Count > 1)
      {
        pDevCtx->ResolveSubresource ( pDrawableTex.p,     D3D11CalcSubresource (0, 0, 0),
                                      pUndrawableTexture, D3D11CalcSubresource (0, 0, tex_desc.MipLevels),
                                                          drawable_desc.Format );
      }

      else
      {
        pDevCtx->CopySubresourceRegion ( pDrawableTex.p, D3D11CalcSubresource (0, 0, 0),
                                                                               0, 0, 0,
                                     pUndrawableTexture, D3D11CalcSubresource ( rtv_desc.Texture2D.MipSlice, 0,
                                                                                tex_desc.MipLevels ),
                                                 nullptr );

      }

      return
        pDevice->CreateShaderResourceView ( pDrawableTex.p, &srv_desc, ppCopyView );
    }

    return hr;
  }

  srv_desc.Texture2D.MipLevels       = 1;
  srv_desc.Texture2D.MostDetailedMip = 0;

  ////srv_desc.Texture2D.MipLevels       = tex_desc.MipLevels;
  ////srv_desc.Texture2D.MostDetailedMip = rtv_desc.Texture2D.MipSlice;

  return
    pDevice->CreateShaderResourceView ( pUndrawableTexture, &srv_desc, ppCopyView );
}


class SK_ExecuteReShadeOnReturn
{
public:
   explicit
   SK_ExecuteReShadeOnReturn ( ID3D11DeviceContext* pCtx,
                               UINT                 uiDevCtxHandle,
                               SK_TLS*              pTLS )
                                                           : _ctx        (pCtx),
                                                             _ctx_handle (uiDevCtxHandle),
                                                             _tls        (pTLS) { };
  ~SK_ExecuteReShadeOnReturn (void)
  {
  const
   auto
    TriggerReShade_After = [&]()
    {
      auto flag_result =
        SK_ImGui_FlagDrawing_OnD3D11Ctx (_ctx_handle);

      SK_ScopedBool auto_bool (flag_result.first);
                              *flag_result.first = flag_result.second;

      SK_ScopedBool auto_bool1 (&_tls->imgui->drawing);
                                 _tls->imgui->drawing = TRUE;

      auto& shaders =
        SK_D3D11_Shaders;

      const UINT dev_idx =
        _ctx_handle;

      if ((! SK_D3D11_IsDevCtxDeferred (_ctx)) && config.reshade.is_addon && (! shaders->reshade_triggered))
      {
        //SK_ComPtr <ID3D11DepthStencilView>  pDSV = nullptr;
        //SK_ComPtr <ID3D11DepthStencilState> pDSS = nullptr;
        //SK_ComPtr <ID3D11RenderTargetView>  pRTV = nullptr;
        //
        //                                    UINT ref;
        //   _ctx->OMGetDepthStencilState (&pDSS, &ref);
        //   _ctx->OMGetRenderTargets     (1, &pRTV, &pDSV);

        //if (pDSS)
        //{
        //  D3D11_DEPTH_STENCIL_DESC ds_desc;
        //           pDSS->GetDesc (&ds_desc);
        //
        //  if ((! pDSV) || (! ds_desc.StencilEnable))
        //  {
            for (int i = 0 ; i < 5; i++)
            {
              SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>* pShaderReg = { };

              switch (i)
              {
                default: SK_ReleaseAssert (!"Bad Codepath Encountered"); return;
                case 0:
                  pShaderReg = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *)&shaders->vertex;
                  break;
                case 1:
                  pShaderReg = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *)&shaders->pixel;
                  break;
                case 2:
                  pShaderReg = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *)&shaders->geometry;
                  break;
                case 3:
                  pShaderReg = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *)&shaders->hull;
                  break;
                case 4:
                  pShaderReg = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *)&shaders->domain;
                  break;
              };

              if ( (! pShaderReg->trigger_reshade.after.empty ()) &&
                      pShaderReg->current.shader [dev_idx] != 0x0 &&
                      pShaderReg->trigger_reshade.after.find (
                      pShaderReg->current.shader [dev_idx]
                 ) != pShaderReg->trigger_reshade.after.cend () )
              {
                shaders->reshade_triggered = true;

                if (! (SK_D3D11_IsDevCtxDeferred (_ctx) && (! config.render.dxgi.deferred_isolation)))
                {
                  SK_TLS *pTLS = SK_TLS_Bottom ();

                  SK_ScopedBool autobool0
                  (&pTLS->imgui->drawing);
                    pTLS->imgui->drawing = TRUE;

                  // Don't cache any texture resources ReShade creates.
                  SK_ScopedBool decl_tex_scope (
                    SK_D3D11_DeclareTexInjectScope (pTLS)
                  );

                  SK_ReShadeAddOn_RenderEffectsD3D11 ((IDXGISwapChain1 *)SK_GetCurrentRenderBackend ().swapchain.p);
                }
                break;
              }
            }
          //}
        }
      //}
    };

    //TriggerReShade_After ();
  }

protected:
  ID3D11DeviceContext* _ctx;
  UINT                 _ctx_handle;
  SK_TLS*              _tls;
};
#pragma endregion ReShadeStuff


using _ShaderBundle =
    SK_D3D11_KnownShaders::
                  ShaderRegistry <IUnknown>*;

static std::map < sk_shader_class, _ShaderBundle >&
__SK_D3D11_ShaderBundleMap (void)
{
  static std::map < sk_shader_class, _ShaderBundle >
  _shaders =
  { { sk_shader_class::Vertex,   (_ShaderBundle)&SK_D3D11_Shaders->vertex   },
    { sk_shader_class::Pixel,    (_ShaderBundle)&SK_D3D11_Shaders->pixel    },
    { sk_shader_class::Geometry, (_ShaderBundle)&SK_D3D11_Shaders->geometry },
    { sk_shader_class::Hull,     (_ShaderBundle)&SK_D3D11_Shaders->hull     },
    { sk_shader_class::Domain,   (_ShaderBundle)&SK_D3D11_Shaders->domain   },
    { sk_shader_class::Compute,  (_ShaderBundle)&SK_D3D11_Shaders->compute  } };

  return _shaders;
}

static DWORD& dwFrameTime = SK::ControlPanel::current_time;

auto OnTopColorCycle =
[ ]
{
  return (ImVec4&&)ImColor::HSV ( 0.491666f,
                            std::min ( static_cast <float>(0.161616f +  (dwFrameTime % 250) / 250.0f -
                                                                floor ( (dwFrameTime % 250) / 250.0f) ), 1.0f),
                                std::min ( static_cast <float>(0.333 +  (dwFrameTime % 500) / 500.0f -
                                                                floor ( (dwFrameTime % 500) / 500.0f) ), 1.0f) );
};

auto WireframeColorCycle =
[ ]
{
  return (ImVec4&&)ImColor::HSV ( 0.133333f,
                            std::min ( static_cast <float>(0.161616f +  (dwFrameTime % 250) / 250.0f -
                                                                floor ( (dwFrameTime % 250) / 250.0f) ), 1.0f),
                                std::min ( static_cast <float>(0.333 +  (dwFrameTime % 500) / 500.0f -
                                                                floor ( (dwFrameTime % 500) / 500.0f) ), 1.0f) );
};

auto SkipColorCycle =
[ ]
{
  return (ImVec4&&)ImColor::HSV ( 0.0f,
                            std::min ( static_cast <float>(0.161616f +  (dwFrameTime % 250) / 250.0f -
                                                                floor ( (dwFrameTime % 250) / 250.0f) ), 1.0f),
                                std::min ( static_cast <float>(0.333 +  (dwFrameTime % 500) / 500.0f -
                                                                floor ( (dwFrameTime % 500) / 500.0f) ), 1.0f) );
};

auto HudColorCycle =
[ ]
{
  return (ImVec4&&)ImColor::HSV ( 0.0f, 0.0f,
                                std::min ( static_cast <float>(0.333 +  (dwFrameTime % 500) / 500.0f -
                                                                floor ( (dwFrameTime % 500) / 500.0f) ), 1.0f) );
};

auto IsWireframe = [](sk_shader_class shader_class, uint32_t crc32c)
{
  d3d11_shader_tracking_s* tracker   = nullptr;
  bool                     wireframe = false;

  static auto& _bundles =
    __SK_D3D11_ShaderBundleMap ();

  _ShaderBundle pShader =
    _bundles [shader_class];

  tracker   = &pShader->tracked;
  wireframe =  pShader->wireframe.find (crc32c) !=
               pShader->wireframe.cend (      );

  if ( tracker->crc32c == crc32c &&
       tracker->wireframe )
  {
    return true;
  }

  return wireframe;
};

static auto IsOnTop =
[](sk_shader_class shader_class, uint32_t crc32c)
{
  d3d11_shader_tracking_s* tracker = nullptr;
  bool                     on_top  = false;

  static auto& _bundles =
    __SK_D3D11_ShaderBundleMap ();

  _ShaderBundle pShader =
    _bundles [shader_class];

  tracker = &pShader->tracked;
  on_top  =  pShader->on_top.find (crc32c) !=
             pShader->on_top.cend (      );

  if ( tracker->crc32c == crc32c &&
       tracker->on_top )
  {
    return true;
  }

  return on_top;
};

static auto IsSkipped =
[](sk_shader_class shader_class, uint32_t crc32c)
{
  d3d11_shader_tracking_s* tracker     = nullptr;
  bool                     blacklisted = false;

  static auto& _bundles =
    __SK_D3D11_ShaderBundleMap ();

  _ShaderBundle pShader =
    _bundles [shader_class];

      tracker          = &pShader->tracked;
  if (tracker->crc32c == crc32c && tracker->cancel_draws)
    return true;

  blacklisted = pShader->blacklist.find (crc32c) !=
                pShader->blacklist.cend (      );

  return blacklisted;
};

static auto IsHud =
[](sk_shader_class shader_class, uint32_t crc32c)
{
  d3d11_shader_tracking_s* tracker = nullptr;
  bool                     hud     = false;

  static auto& _bundles =
    __SK_D3D11_ShaderBundleMap ();

  _ShaderBundle pShader =
    _bundles [shader_class];

  tracker = &pShader->tracked;
  hud     =  pShader->hud.find (crc32c) !=
             pShader->hud.cend (      );

  return hud;
};

SK_LazyGlobal <std::unordered_map <uint32_t, shader_disasm_s>> vs_disassembly;
SK_LazyGlobal <std::unordered_map <uint32_t, shader_disasm_s>> ps_disassembly;
SK_LazyGlobal <std::unordered_map <uint32_t, shader_disasm_s>> gs_disassembly;
SK_LazyGlobal <std::unordered_map <uint32_t, shader_disasm_s>> hs_disassembly;
SK_LazyGlobal <std::unordered_map <uint32_t, shader_disasm_s>> ds_disassembly;
SK_LazyGlobal <std::unordered_map <uint32_t, shader_disasm_s>> cs_disassembly;

uint32_t change_sel_vs = 0x00;
uint32_t change_sel_ps = 0x00;
uint32_t change_sel_gs = 0x00;
uint32_t change_sel_hs = 0x00;
uint32_t change_sel_ds = 0x00;
uint32_t change_sel_cs = 0x00;

void
ShaderMenu
( SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*           registrant,
  std::unordered_map <uint32_t, LONG>&                        blacklist,
  SK_D3D11_KnownShaders::conditional_blacklist_t&             cond_blacklist,
      concurrency::concurrent_unordered_set <SK_ComPtr <ID3D11ShaderResourceView> >& used_resources,
const concurrency::concurrent_unordered_set <SK_ComPtr <ID3D11ShaderResourceView> >& set_of_resources,
      uint32_t                                                    shader )
{
  if (! registrant)
    return;

  const SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  if ( blacklist.find (shader) !=
       blacklist.cend (      )  )
  {
    if (ImGui::MenuItem ("Enable Shader"))
    {
      registrant->releaseTrackingRef (blacklist, shader);
      InterlockedDecrement (&SK_D3D11_DrawTrackingReqs);
      InterlockedDecrement (&SK_D3D11_TrackingCount->Always);
    }
  }

  else
  {
    if (ImGui::MenuItem ("Disable Shader"))
    {
      registrant->addTrackingRef (blacklist, shader);
      InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
      InterlockedIncrement (&SK_D3D11_TrackingCount->Always);
    }
  }

  auto DrawSRV = [&](ID3D11ShaderResourceView* pSRV)
  {
    if (pSRV == nullptr)
      return;

    DXGI_FORMAT                     fmt = DXGI_FORMAT_UNKNOWN;
    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;

    pSRV->GetDesc (&srv_desc);

    if (srv_desc.ViewDimension == D3D_SRV_DIMENSION_TEXTURE2D)
    {
      SK_ComPtr <ID3D11Resource>   pRes = nullptr;
               pSRV->GetResource (&pRes.p);
      SK_ComQIPtr <ID3D11Texture2D>
                            pTex ( pRes.p);

      if ( pRes != nullptr &&
           pTex != nullptr )
      {
        D3D11_TEXTURE2D_DESC desc = { };
        pTex->GetDesc      (&desc);
                       fmt = desc.Format;

        if (desc.Height > 0 && desc.Width > 0)
        {
          ImGui::TreePush ("");

          std::scoped_lock <SK_Thread_HybridSpinlock> auto_lock (*cs_render_view);

          SK_ComPtr <ID3D11ShaderResourceView> pSRV2 = nullptr;

          //DXGI_FORMAT orig_fmt =
          //  srv_desc.Format;
          //
          //srv_desc.Format =
          //  DirectX::MakeTypelessFLOAT (orig_fmt);
          //
          //if (srv_desc.Format == orig_fmt)
          //{
          //  srv_desc.Format =
          //    DirectX::MakeTypelessUNORM (orig_fmt);
          //}
          //srv_desc.Texture2D.MipLevels       = (UINT)-1;
          //srv_desc.Texture2D.MostDetailedMip =        desc.MipLevels;
          //
          //srv_desc.Format =
          //  SK_DXGI_MakeTypedFormat (srv_desc.Format);

          auto pDev =
            rb.getDevice <ID3D11Device> ();

          if (                                       pDev     != nullptr &&
               SUCCEEDED (SK_D3D11_MakeDrawableCopy (pDev, pTex, nullptr, &pSRV2.p))
             )
          {
            SK_D3D11_TempResources->push_back (pSRV2.p);

            ImGui::Image ( pSRV2.p,
                                       ImVec2  ( std::max (64.0f, (float)desc.Width / 16.0f),
      ((float)desc.Height / (float)desc.Width) * std::max (64.0f, (float)desc.Width / 16.0f) ),
                                       ImVec2  (0,0),             ImVec2  (1,1),
                                       ImColor (255,255,255,255), ImColor (242,242,13,255) );
          }

          else
          {
            SK_D3D11_TempResources->push_back (pSRV);

            ImGui::Image ( pSRV,       ImVec2  ( std::max (64.0f, (float)desc.Width / 16.0f),
      ((float)desc.Height / (float)desc.Width) * std::max (64.0f, (float)desc.Width / 16.0f) ),
                                       ImVec2  (0,0),             ImVec2  (1,1),
                                       ImColor (255,255,255,255), ImColor (242,242,13,255) );
          }
          ImGui::TreePop  ();
        }
      }
    }
  };

  std::set <uint32_t> listed;

  auto& textures =
    SK_D3D11_Textures;

  for (auto& it : used_resources)
  {
    bool selected = false;

    uint32_t crc32c = 0x0;

    SK_ComPtr <ID3D11Resource> pRes = nullptr;
             it->GetResource (&pRes.p);

    D3D11_RESOURCE_DIMENSION rdim = D3D11_RESOURCE_DIMENSION_UNKNOWN;
             pRes->GetType (&rdim);

    if (rdim == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
    {
      SK_ComQIPtr <ID3D11Texture2D> pTex2D (pRes.p);

      if (pTex2D != nullptr)
      {
        if ( textures->Textures_2D.count (pTex2D.p) &&
             textures->Textures_2D       [pTex2D.p].crc32c != 0x0 )
        {
          crc32c =
             textures->Textures_2D       [pTex2D.p].crc32c;
        }
      }
    }

    if (crc32c == 0x0 || listed.count (crc32c) > 0)
      continue;

    if (cond_blacklist [shader].count (crc32c) > 0)
    {
      ImGui::BeginGroup ();

      ImGui::MenuItem ( SK_FormatString ("Enable Shader for Texture %x", crc32c).c_str (), nullptr, &selected);

      if (set_of_resources.count (it))
        DrawSRV (it);

      ImGui::EndGroup ();
    }

    listed.emplace (crc32c);

    if (selected)
    {
      InterlockedDecrement (&SK_D3D11_TrackingCount->TextureBased);

      cond_blacklist [shader].erase (crc32c);
    }
  }

  listed.clear ();

  for (auto& it : used_resources )
  {
    if (it == nullptr)
      continue;

    bool selected = false;

    uint32_t crc32c = 0x0;

    SK_ComPtr <ID3D11Resource> pRes = nullptr;
             it->GetResource (&pRes.p);

    D3D11_RESOURCE_DIMENSION rdim = D3D11_RESOURCE_DIMENSION_UNKNOWN;
    pRes->GetType          (&rdim);

    if (rdim == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
    {
      SK_ComQIPtr <ID3D11Texture2D> pTex2D (pRes);

      if (pTex2D != nullptr)
      {
        if ( textures->Textures_2D.count (pTex2D.p) &&
             textures->Textures_2D       [pTex2D.p].crc32c != 0x0 )
        {
          crc32c =
            textures->Textures_2D        [pTex2D.p].crc32c;
        }
      }
    }

    if (crc32c == 0x0 || listed.count (crc32c) > 0)
      continue;

    if (cond_blacklist [shader].count (crc32c) == 0)
    {
      ImGui::BeginGroup ();

      ImGui::MenuItem ( SK_FormatString ("Disable Shader for Texture %x", crc32c).c_str (), nullptr, &selected);

      if (set_of_resources.count (it))
        DrawSRV (it);

      ImGui::EndGroup ();

      if (selected)
      {
        InterlockedIncrement (&SK_D3D11_TrackingCount->TextureBased);

        cond_blacklist [shader].emplace (crc32c);
      }
    }

    listed.emplace (crc32c);
  }
};


void
SK_LiveShaderClassView (sk_shader_class shader_type, bool& can_scroll)
{
  ID3D11Device* pDevice =
    (ID3D11Device *)(SK_GetCurrentRenderBackend ().device.p);

  std::scoped_lock <SK_Thread_HybridSpinlock> auto_lock (*cs_shader);

  auto& io =
    ImGui::GetIO ();

  static auto& shaders =
    SK_D3D11_Shaders;

  ImGui::PushID (static_cast <int> (shader_type));

  static float last_width = 256.0f;
  const  float font_size  = ImGui::GetFont ()->FontSize * io.FontGlobalScale;

  struct shader_entry_s
  {
    uint32_t    crc32c  =   0x0;
    bool        enabled = false;
    std::string name;
  };

  struct shader_class_imp_s
  {
    std::vector <
      shader_entry_s
    >                         contents     = {  };

    float                     max_name_len = 0.0f;
    bool                      dirty        = true;
    uint32_t                  last_sel     =    std::numeric_limits <uint32_t>::max ();
    unsigned int                   sel     =    0;
    float                     last_ht      = 256.0f;
    ImVec2                    last_min     = ImVec2 (0.0f, 0.0f);
    ImVec2                    last_max     = ImVec2 (0.0f, 0.0f);
  };

  struct {
    shader_class_imp_s vs;
    shader_class_imp_s ps;
    shader_class_imp_s gs;
    shader_class_imp_s hs;
    shader_class_imp_s ds;
    shader_class_imp_s cs;
  } static list_base;

  auto GetShaderList =
    [](const sk_shader_class type) ->
      shader_class_imp_s*
      {
        switch (type)
        {
          case sk_shader_class::Vertex:   return &list_base.vs;
          case sk_shader_class::Pixel:    return &list_base.ps;
          case sk_shader_class::Geometry: return &list_base.gs;
          case sk_shader_class::Hull:     return &list_base.hs;
          case sk_shader_class::Domain:   return &list_base.ds;
          case sk_shader_class::Compute:  return &list_base.cs;
        }

        assert (false);

        return nullptr;
      };

  shader_class_imp_s*
    list = GetShaderList (shader_type);

  auto GetShaderTracker =
    [](const sk_shader_class type) ->
      d3d11_shader_tracking_s*
      {
        switch (type)
        {
          case sk_shader_class::Vertex:   return &shaders->vertex.tracked;
          case sk_shader_class::Pixel:    return &shaders->pixel.tracked;
          case sk_shader_class::Geometry: return &shaders->geometry.tracked;
          case sk_shader_class::Hull:     return &shaders->hull.tracked;
          case sk_shader_class::Domain:   return &shaders->domain.tracked;
          case sk_shader_class::Compute:  return &shaders->compute.tracked;
        }

        assert (false);

        return nullptr;
      };

  d3d11_shader_tracking_s*
    tracker = GetShaderTracker (shader_type);

  auto GetShaderSet =
    [&](const sk_shader_class type) ->
      std::set <uint32_t>&
      {
        static std::set <uint32_t> set  [6];
        static size_t              size [6] = { 0 };

        switch (type)
        {
          case sk_shader_class::Vertex:
          {
            static auto& vertex =
                shaders->vertex;

            auto& descs = vertex.descs [pDevice];

            if (size [0] == descs.size ())
              return set [0];

            for (auto const& vertex_shader : descs)
            {
              // Ignore ImGui / CEGUI shaders
              if ( vertex_shader.first != 0xb42ede74 &&
                   vertex_shader.first != 0x1f8c62dc )
              {
                if (vertex_shader.first > 0x00000000) set [0].emplace (vertex_shader.first);
              }
            }

                   size [0] = descs.size ();
            return set  [0];
          } break;

          case sk_shader_class::Pixel:
          {
            static auto& pixel =
                shaders->pixel;

            auto& descs = pixel.descs [pDevice];

            if (size [1] == descs.size ())
              return set [1];

            for (auto const& pixel_shader : descs)
            {
              // Ignore ImGui / CEGUI shaders
              if ( pixel_shader.first != 0xd3af3aa0 &&
                   pixel_shader.first != 0xb04a90ba )
              {
                if (pixel_shader.first > 0x00000000) set [1].emplace (pixel_shader.first);
              }
            }

                   size [1] = descs.size ();
            return set  [1];
          } break;

          case sk_shader_class::Geometry:
          {
            static auto& geometry =
                shaders->geometry;

            auto& descs = geometry.descs [pDevice];

            if (size [2] == descs.size ())
              return set [2];

            for (auto const& geometry_shader : descs)
            {
              if (geometry_shader.first > 0x00000000) set [2].emplace (geometry_shader.first);
            }

                   size [2] = descs.size ();
            return set  [2];
          } break;

          case sk_shader_class::Hull:
          {
            static auto& hull =
                shaders->hull;

            auto& descs = hull.descs [pDevice];

            if (size [3] == descs.size ())
              return set [3];

            for (auto const& hull_shader : descs)
            {
              if (hull_shader.first > 0x00000000) set [3].emplace (hull_shader.first);
            }

                   size [3] = descs.size ();
            return set  [3];
          } break;

          case sk_shader_class::Domain:
          {
            static auto& domain =
                shaders->domain;

            auto& descs = domain.descs [pDevice];

            if (size [4] == descs.size ())
              return set [4];

            for (auto const& domain_shader : descs)
            {
              if (domain_shader.first > 0x00000000) set [4].emplace (domain_shader.first);
            }

                   size [4] = descs.size ();
            return set  [4];
          } break;

          case sk_shader_class::Compute:
          {
            static auto& compute =
                shaders->compute;

            auto& descs = compute.descs [pDevice];

            if (size [5] == descs.size ())
              return set [5];

            for (auto const& compute_shader : descs)
            {
              if (compute_shader.first > 0x00000000) set [5].emplace (compute_shader.first);
            }

                   size [5] = descs.size ();
            return set  [5];
          } break;
        }

        return set [0];
      };

  std::set <uint32_t>& set =
    GetShaderSet (shader_type);

  std::vector <uint32_t>
    shader_set ( set.cbegin (), set.cend () );

  auto GetShaderDisasm =
    [](const sk_shader_class type) ->
      std::unordered_map <uint32_t, shader_disasm_s>*
      {
        switch (type)
        {
          case sk_shader_class::Vertex:   return &*vs_disassembly;
          default:
          case sk_shader_class::Pixel:    return &*ps_disassembly;
          case sk_shader_class::Geometry: return &*gs_disassembly;
          case sk_shader_class::Hull:     return &*hs_disassembly;
          case sk_shader_class::Domain:   return &*ds_disassembly;
          case sk_shader_class::Compute:  return &*cs_disassembly;
        }
      };

  std::unordered_map <uint32_t, shader_disasm_s>*
    disassembly = GetShaderDisasm (shader_type);

  auto GetShaderWord =
    [](const sk_shader_class type) ->
      const char*
      {
        switch (type)
        {
          case sk_shader_class::Vertex:   return "Vertex";
          case sk_shader_class::Pixel:    return "Pixel";
          case sk_shader_class::Geometry: return "Geometry";
          case sk_shader_class::Hull:     return "Hull";
          case sk_shader_class::Domain:   return "Domain";
          case sk_shader_class::Compute:  return "Compute";
          default:                        return "Unknown";
        }
      };

  const char*
    szShaderWord = GetShaderWord (shader_type);

  uint32_t invalid = 0x0;

  const
   auto
    GetShaderChange =
    [&](const sk_shader_class type) ->
      uint32_t&
      {
        switch (type)
        {
          case sk_shader_class::Vertex:   return change_sel_vs;
          case sk_shader_class::Pixel:    return change_sel_ps;
          case sk_shader_class::Geometry: return change_sel_gs;
          case sk_shader_class::Hull:     return change_sel_hs;
          case sk_shader_class::Domain:   return change_sel_ds;
          case sk_shader_class::Compute:  return change_sel_cs;
          default:                        return invalid;
        }
      };

  auto GetShaderBlacklist =
    [&](const sk_shader_class type)->
      std::unordered_map <uint32_t, LONG>&
      {
        static std::unordered_map <uint32_t, LONG> invalid;

        switch (type)
        {
          case sk_shader_class::Vertex:   return shaders->vertex.blacklist;
          case sk_shader_class::Pixel:    return shaders->pixel.blacklist;
          case sk_shader_class::Geometry: return shaders->geometry.blacklist;
          case sk_shader_class::Hull:     return shaders->hull.blacklist;
          case sk_shader_class::Domain:   return shaders->domain.blacklist;
          case sk_shader_class::Compute:  return shaders->compute.blacklist;
          default:                        return invalid;
        }
      };

  auto GetShaderBlacklistEx =
    [&](const sk_shader_class type)->
      SK_D3D11_KnownShaders::conditional_blacklist_t&
      {
        static SK_D3D11_KnownShaders::conditional_blacklist_t invalid;

        switch (type)
        {
          case sk_shader_class::Vertex:   return shaders->vertex.blacklist_if_texture;
          case sk_shader_class::Pixel:    return shaders->pixel.blacklist_if_texture;
          case sk_shader_class::Geometry: return shaders->geometry.blacklist_if_texture;
          case sk_shader_class::Hull:     return shaders->hull.blacklist_if_texture;
          case sk_shader_class::Domain:   return shaders->domain.blacklist_if_texture;
          case sk_shader_class::Compute:  return shaders->compute.blacklist_if_texture;
          default:                        return invalid;
        }
      };

  auto GetShaderUsedResourceViews =
    [&](const sk_shader_class type)->
      concurrency::concurrent_unordered_set <SK_ComPtr <ID3D11ShaderResourceView> >&
      {
        static concurrency::concurrent_unordered_set <SK_ComPtr <ID3D11ShaderResourceView>> invalid;

        switch (type)
        {
          case sk_shader_class::Vertex:   return shaders->vertex.tracked.set_of_views;
          case sk_shader_class::Pixel:    return shaders->pixel.tracked.set_of_views;
          case sk_shader_class::Geometry: return shaders->geometry.tracked.set_of_views;
          case sk_shader_class::Hull:     return shaders->hull.tracked.set_of_views;
          case sk_shader_class::Domain:   return shaders->domain.tracked.set_of_views;
          case sk_shader_class::Compute:  return shaders->compute.tracked.set_of_views;
          default:                        return invalid;
        }
      };

  auto GetShaderResourceSet =
    [&](const sk_shader_class type)->
      concurrency::concurrent_unordered_set <SK_ComPtr <ID3D11ShaderResourceView> >&
      {
        static concurrency::concurrent_unordered_set <SK_ComPtr <ID3D11ShaderResourceView>> invalid;

        switch (type)
        {
          case sk_shader_class::Vertex:   return shaders->vertex.tracked.set_of_views;
          case sk_shader_class::Pixel:    return shaders->pixel.tracked.set_of_views;
          case sk_shader_class::Geometry: return shaders->geometry.tracked.set_of_views;
          case sk_shader_class::Hull:     return shaders->hull.tracked.set_of_views;
          case sk_shader_class::Domain:   return shaders->domain.tracked.set_of_views;
          case sk_shader_class::Compute:  return shaders->compute.tracked.set_of_views;
          default:                        return invalid;
        }
      };

  struct sk_shader_state_s {
    unsigned int last_sel      = 0;
    unsigned int active_frames = 3;
            bool sel_changed   = false;
            bool hide_inactive = true;
            bool hovering      = false;
            bool focused       = false;

    static int ClassToIdx (const sk_shader_class shader_class) noexcept
    {
      // nb: shader_class is a bitmask, we need indices
      switch (shader_class)
      {
        case sk_shader_class::Vertex:   return 0;
        default:
        case sk_shader_class::Pixel:    return 1;
        case sk_shader_class::Geometry: return 2;
        case sk_shader_class::Hull:     return 3;
        case sk_shader_class::Domain:   return 4;
        case sk_shader_class::Compute:  return 5;

        // Masked combinations are, of course, invalid :)
      }
    }

    static const wchar_t* ClassToPrefix (const sk_shader_class shader_class) noexcept
    {
      // nb: shader_class is a bitmask, we need indices
      switch (shader_class)
      {
        case sk_shader_class::Vertex:   return L"vs";
        default:
        case sk_shader_class::Pixel:    return L"ps";
        case sk_shader_class::Geometry: return L"gs";
        case sk_shader_class::Hull:     return L"hs";
        case sk_shader_class::Domain:   return L"ds";
        case sk_shader_class::Compute:  return L"cs";

        // Masked combinations are, of course, invalid :)
      }
    }
  } static shader_state [6];

  unsigned int& last_sel      =  shader_state [sk_shader_state_s::ClassToIdx (shader_type)].last_sel;
  bool&         sel_changed   =  shader_state [sk_shader_state_s::ClassToIdx (shader_type)].sel_changed;
  bool*         hide_inactive = &shader_state [sk_shader_state_s::ClassToIdx (shader_type)].hide_inactive;
  unsigned int& active_frames =  shader_state [sk_shader_state_s::ClassToIdx (shader_type)].active_frames;
  bool scrolled = false;

  int dir = 0;

  if (ImGui::IsMouseHoveringRect (list->last_min, list->last_max))
  {
         if (ImGui::IsKeyPressed (ImGuiKey_LeftBracket,  false)) { dir = -1;  io.WantCaptureKeyboard = true; scrolled = true; }
    else if (ImGui::IsKeyPressed (ImGuiKey_RightBracket, false)) { dir = +1;  io.WantCaptureKeyboard = true; scrolled = true; }
  }

  ImGui::BeginGroup   ();

  if (ImGui::Checkbox ("Hide Inactive", hide_inactive))
  {
    list->dirty = true;
  }

  ImGui::SameLine     ();

  ImGui::TextDisabled ("   Tracked Shader: ");
  ImGui::SameLine     ();
  ImGui::BeginGroup   ();

  bool cancel =
    tracker->cancel_draws;

  if ( ImGui::Checkbox     ( shader_type != sk_shader_class::Compute ? "Skip Draws" :
                                                                       "Skip Dispatches",
                               &cancel )
     )
  {
    tracker->cancel_draws = cancel;
  }

  if (! cancel)
  {
    ImGui::SameLine   ();

    bool blink =
      tracker->highlight_draws;

    if (ImGui::Checkbox   ( "Blink", &blink ))
      tracker->highlight_draws = blink;

    if (shader_type != sk_shader_class::Compute)
    {
      bool wireframe =
        tracker->wireframe;

      ImGui::SameLine ();

      if ( ImGui::Checkbox ( "Draw in Wireframe", &wireframe ) )
        tracker->wireframe = wireframe;

      bool on_top =
        tracker->on_top;

      ImGui::SameLine ();

      if (ImGui::Checkbox ( "Draw on Top", &on_top))
        tracker->on_top = on_top;

      ImGui::SameLine ();

      bool probe_target =
        tracker->pre_hud_source;

      if (ImGui::Checkbox ("Probe Outputs", &probe_target))
      {
        tracker->pre_hud_source = probe_target;
        tracker->pre_hud_rtv    =      nullptr;
      }

      if (probe_target)
      {
        int rt_slot = tracker->pre_hud_rt_slot + 1;

        if ( ImGui::Combo ( "Examine RenderTarget", &rt_slot,
                               "N/A\0"
                               "Slot 0\0Slot 1\0Slot 2\0"
                               "Slot 3\0Slot 4\0Slot 5\0"
                               "Slot 6\0Slot 7\0\0" )
           )
        {
          tracker->pre_hud_rt_slot = ( rt_slot - 1 );
          tracker->pre_hud_rtv     =   nullptr;
        }


        if (ImGui::IsItemHovered () && tracker->pre_hud_rtv != nullptr)
        {
          SK_ComPtr <ID3D11RenderTargetView> rt_view =
            tracker->pre_hud_rtv;

          D3D11_RENDER_TARGET_VIEW_DESC rtv_desc = { };
          rt_view->GetDesc            (&rtv_desc);

          if ( rtv_desc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2D ||
               rtv_desc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2DMS )
          {
            SK_ComPtr   <ID3D11Resource>        pRes = nullptr;
                         rt_view->GetResource (&pRes.p);
            SK_ComQIPtr <ID3D11Texture2D> pTex (pRes.p);

            if ( pRes != nullptr &&
                 pTex != nullptr )
            {
              const SK_RenderBackend& rb =
                SK_GetCurrentRenderBackend ();

              auto pDev =
                rb.getDevice <ID3D11Device> ();
              SK_ComPtr      <ID3D11ShaderResourceView> pSRV;

              D3D11_TEXTURE2D_DESC   desc = { };
              pTex->GetDesc        (&desc);
              //srv_desc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
              //srv_desc.Format                    = desc.Format;
              //srv_desc.Texture2D.MipLevels       = (UINT)-1;
              //srv_desc.Texture2D.MostDetailedMip =  0;

              if (pDev != nullptr && SUCCEEDED (SK_D3D11_MakeDrawableCopy (pDev, pTex, rt_view, &pSRV.p)))
              {
                ImGui::BeginTooltip    ();
                ImGui::PushStyleColor  (ImGuiCol_Border, ImVec4 (0.5f, 0.5f, 0.5f, 1.0f));

                ImGui::BeginGroup (                  );
                ImGui::Text       ( "Dimensions:   " );
                ImGui::Text       ( "Format:       " );
                ImGui::EndGroup   (                  );

                ImGui::SameLine   ( );

                ImGui::BeginGroup (                                             );
                ImGui::Text       ( "%lux%lu",
                                      desc.Width, desc.Height/*, effective_width, effective_height, 0.9875f * content_avail_y - ((float)(bottom_list + 3) * font_size * 1.125f), content_avail_y*//*,
                                        pTex->d3d9_tex->GetLevelCount ()*/      );
                ImGui::Text       ( "%hs",
                                      SK_DXGI_FormatToStr (desc.Format).data () );
                ImGui::EndGroup   (                                             );

                if (pSRV != nullptr)
                {
                  const float effective_width  = 512.0f;
                  const float effective_height = effective_width / ((float)desc.Width / (float)desc.Height);

                  ImGui::Separator  ( );

                  ImGui::PushStyleColor (ImGuiCol_Border, ImVec4 (0.95f, 0.95f, 0.05f, 1.0f));
                  ImGui::BeginChildFrame   (ImGui::GetID ("ShaderResourceView_RT_Frame"),
                                            ImVec2 (effective_width + 8.0f, effective_height + 8.0f),
                                            ImGuiWindowFlags_AlwaysAutoResize );

                  SK_D3D11_TempResources->push_back (pSRV.p);

                  ImGui::Image             ( pSRV,
                                               ImVec2 (effective_width, effective_height),
                                                 ImVec2  (0,0),             ImVec2  (1,1),
                                                 ImColor (255,255,255,255), ImColor (255,255,255,128)
                                           );

                  ImGui::EndChildFrame     (    );
                  ImGui::PopStyleColor     (    );
                }

                ImGui::PopStyleColor       (    );
                ImGui::EndTooltip          (    );
              }
            }
          }
        }
      }
    }

    else
    {
      bool clear =
        tracker->clear_output.load ();

      ImGui::SameLine   ();
      ImGui::BeginGroup ();
      if ( ImGui::Checkbox ( "Clear UAVs",
                             &clear )
         )
      {
        tracker->clear_output.store (clear);
      }

      if (tracker->clear_output.load ())
      {
        extern UINT  start_uav;
        extern UINT  uav_count;
        extern float uav_clear [4];

        ImGui::BeginGroup   ();
        ImGui::InputScalarN ("Clear Value", ImGuiDataType_Float, &uav_clear, 4);
        ImGui::InputInt     ("StartSlot",                 (int *)&start_uav);
        ImGui::InputInt     ("UAV Count",                 (int *)&uav_count);
        ImGui::EndGroup     ();
      }
      ImGui::EndGroup ();
    }
  }
  ImGui::EndGroup ();

  if (shader_set.size () != list->contents.size ())
  {
    list->dirty = true;
  }

  const
   auto
    ShaderBase = [](sk_shader_class& shader_class) ->
    void*
    {
      switch (shader_class)
      {
        case sk_shader_class::Vertex:   return &shaders->vertex;
        case sk_shader_class::Pixel:    return &shaders->pixel;
        case sk_shader_class::Geometry: return &shaders->geometry;
        case sk_shader_class::Hull:     return &shaders->hull;
        case sk_shader_class::Domain:   return &shaders->domain;
        case sk_shader_class::Compute:  return &shaders->compute;
        default:
        return nullptr;
      }
    };


  auto GetShaderDesc = [&](sk_shader_class type, uint32_t crc32c) ->
    SK_D3D11_ShaderDesc&
      {
        return
          ((SK_D3D11_KnownShaders::ShaderRegistry <ID3D11VertexShader>*)ShaderBase (type))->descs [pDevice][
            crc32c
          ];
      };

  const
   auto
    ShaderIsActive = [&](sk_shader_class type, uint32_t crc32c) ->
    bool
      {
        return GetShaderDesc (type, crc32c).usage.last_frame > SK_GetFramesDrawn () - active_frames;
      };

  if (list->dirty || GetShaderChange (shader_type) != 0)
  {
        list->sel = 0;
    int idx       = 0;
        list->contents.clear ();

    list->max_name_len = 0.0f;

    for ( auto& it : shader_set )
    {
      const bool disabled =
        ( GetShaderBlacklist (shader_type).count (it) != 0 );

      auto& rkShader =
        GetShaderDesc (shader_type, it);
      auto pShader =
        rkShader.pShader;

      if (rkShader.name.empty ())
      {
        UINT     uiDescLen    =  127;
        char     szDesc [128] = { };
        wchar_t wszDesc [128] = { };

        if (pShader)
        {
          uiDescLen = sizeof (wszDesc) - sizeof (wchar_t);

          if ( SUCCEEDED ( ((ID3D11Resource *)pShader)->GetPrivateData (
                                    WKPDID_D3DDebugObjectNameW,
                                      &uiDescLen, wszDesc               )
                         )           &&uiDescLen > sizeof (wchar_t)
             )
          {
            rkShader.name = SK_WideCharToUTF8 (wszDesc);
          }

          else
          {
            uiDescLen = sizeof (szDesc) - sizeof (char);

            if ( SUCCEEDED ( ((ID3D11Resource *)pShader)->GetPrivateData (
                                         WKPDID_D3DDebugObjectName,
                                           &uiDescLen, szDesc            )
                           )              &&uiDescLen > sizeof (char)
               )
            {
              rkShader.name = szDesc;
            }
          }
        }

        if (rkShader.name.empty ())
            rkShader.name = SK_FormatString ("%08x", it);
      }

      const char* szDesc =
        rkShader.name.c_str ();

      list->max_name_len =
        std::max (list->max_name_len, ImGui::CalcTextSize (szDesc, nullptr, true).x);

      list->contents.emplace_back (shader_entry_s { it, (! disabled), szDesc });

      if ( ((! GetShaderChange (shader_type)) && list->last_sel == (uint32_t)it) ||
               GetShaderChange (shader_type)                    == (uint32_t)it )
      {
        list->sel = idx;

        // Allow other parts of the mod UI to change the selected shader
        //
        if (GetShaderChange (shader_type))
        {
          if (list->last_sel != GetShaderChange (shader_type))
            list->last_sel = std::numeric_limits <uint32_t>::max ();

          GetShaderChange (shader_type) = 0x00;
        }

        tracker->crc32c = it;
      }

      ++idx;
    }

    list->dirty = false;
  }

  ImGui::PushStyleVar   (ImGuiStyleVar_ChildRounding, 0.0f);
  ImGui::PushStyleColor (ImGuiCol_Border, ImVec4 (0.9f, 0.7f, 0.5f, 1.0f));

  bool& hovering = shader_state [sk_shader_state_s::ClassToIdx (shader_type)].hovering;
  bool& focused  = shader_state [sk_shader_state_s::ClassToIdx (shader_type)].focused;

  const float text_spacing = 2.5f * ImGui::GetStyle ().ItemSpacing.x +
                                    ImGui::GetStyle ().ScrollbarSize;

  ImGui::BeginChild ( ImGui::GetID (GetShaderWord (shader_type)),
                      ImVec2 ( list->max_name_len * io.FontGlobalScale +
                                     text_spacing * io.FontGlobalScale, std::max (font_size * 15.0f, list->last_ht)),
                        true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NavFlattened );

  if (hovering || focused)
  {
    can_scroll = false;

    if (! focused)
    {
      ImGui::BeginTooltip ();
      ImGui::TextColored  (ImVec4 (0.9f, 0.6f, 0.2f, 1.0f), "You can cancel all render passes using the selected %s shader to disable an effect", szShaderWord);
      ImGui::Separator    ();
      ImGui::BulletText   ("Press %hs while the mouse is hovering this list to select the previous shader", SK_WideCharToUTF8 (virtKeyCodeToHumanKeyName [VK_OEM_4]).c_str ());
      ImGui::BulletText   ("Press %hs while the mouse is hovering this list to select the next shader",     SK_WideCharToUTF8 (virtKeyCodeToHumanKeyName [VK_OEM_6]).c_str ());
      ImGui::EndTooltip   ();
    }

    else
    {
      ImGui::BeginTooltip ();
      ImGui::TextColored  (ImVec4 (0.9f, 0.6f, 0.2f, 1.0f), "You can cancel all render passes using the selected %s shader to disable an effect", szShaderWord);
      ImGui::Separator    ();
      ImGui::BulletText   ("Press LB to select the previous shader");
      ImGui::BulletText   ("Press RB to select the next shader");
      ImGui::EndTooltip   ();
    }

    if (! scrolled)
    {
           if (io.NavInputs [ImGuiNavInput_FocusPrev] != 0.0f) { dir = -1; }
      else if (io.NavInputs [ImGuiNavInput_FocusNext] != 0.0f) { dir =  1; }

      else
      {
             if (ImGui::IsKeyPressed (ImGuiKey_LeftBracket,  false)) { dir = -1;  io.WantCaptureKeyboard = true; scrolled = true; }
        else if (ImGui::IsKeyPressed (ImGuiKey_RightBracket, false)) { dir = +1;  io.WantCaptureKeyboard = true; scrolled = true; }
      }
    }
  }

  if (! shader_set.empty ())
  {
    // User wants to cycle list elements, we know only the direction, not how many indices in the list
    //   we need to increment to get an unfiltered list item.
    if (dir != 0)
    {
      if (list->sel == 0)                      // Can't go lower
        dir = std::max (0, dir);
      if (list->sel >= shader_set.size () - 1) // Can't go higher
        dir = std::min (0, dir);

      int last_active = list->sel;

      do
      {
        list->sel += dir;

        size_t&& sel =
          static_cast <size_t&&> (list->sel);

        if ( sel >= shader_set.size () )
        {
          sel =
            static_cast <size_t>       (
              std::min   (
                std::max (
                  static_cast <size_t> (0),            sel ),
                  static_cast <size_t> (shader_set.size ()
                         )
                         )             );
        }

        if (*hide_inactive && list->sel < shader_set.size ())
        {
          if (! ShaderIsActive (shader_type, (uint32_t)shader_set [list->sel]))
          {
            continue;
          }
        }

        last_active = list->sel;

        break;
      } while (list->sel > 0 && list->sel < shader_set.size () - 1);

      // Don't go higher/lower if we're already at the limit
      list->sel = last_active;
    }


    if (list->sel != last_sel)
      sel_changed = true;

    last_sel = list->sel;

    auto ChangeSelectedShader = [](       shader_class_imp_s*  list,
                                    d3d11_shader_tracking_s*   tracker,
                                    SK_D3D11_ShaderDesc&       rDesc ) noexcept
    {
      list->last_sel              = rDesc.crc32c;
    //tracker->clear               (   );
      tracker->crc32c             = rDesc.crc32c;
      tracker->runtime_ms         = 0.0;
      tracker->last_runtime_ms    = 0.0;
      tracker->runtime_ticks.store (0LL);
    };

    // Line counting; assume the list sort order is unchanged
    unsigned int line = 0;

    ImGui::PushID ((int)shader_type);

    for ( auto& it : list->contents )
    {
      SK_D3D11_ShaderDesc& rDesc =
        GetShaderDesc (shader_type, it.crc32c);

      const bool active =
        ShaderIsActive (shader_type, it.crc32c);

      if ((! active) && (*hide_inactive))
      {
        line++;
        continue;
      }


      if (IsSkipped        (shader_type, it.crc32c))
      {
        ImGui::PushStyleColor (ImGuiCol_Text, SkipColorCycle ());
      }

      else if (IsHud       (shader_type, it.crc32c))
      {
        ImGui::PushStyleColor (ImGuiCol_Text, HudColorCycle ());
      }

      else if (IsOnTop     (shader_type, it.crc32c))
      {
        ImGui::PushStyleColor (ImGuiCol_Text, OnTopColorCycle ());
      }

      else if (IsWireframe (shader_type, it.crc32c))
      {
        ImGui::PushStyleColor (ImGuiCol_Text, WireframeColorCycle ());
      }

      else
      {
        if (active)
          ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.95f, 0.95f, 0.95f, 1.0f));
        else
          ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.425f, 0.425f, 0.425f, 0.9f));
      }

      std::string line_text =
        SK_FormatString ("%c%s", it.enabled ? ' ' : '*', it.name.c_str ());

      ImGui::PushID (it.crc32c);

      if (line == list->sel)
      {
        bool selected = true;

        ImGui::Selectable (line_text.c_str (), &selected);

        if (sel_changed)
        {
          if (! ImGui::IsItemVisible  (    ))
                ImGui::SetScrollHereY (0.5f);
          ImGui::SetKeyboardFocusHere (    );

          sel_changed = false;

          ChangeSelectedShader (list, tracker, rDesc);
        }
      }

      else
      {
        bool selected    = false;

        if (ImGui::Selectable (line_text.c_str (), &selected))
        {
          sel_changed = true;
          list->sel   = line;

          ChangeSelectedShader (list, tracker, rDesc);
        }
      }

      if (SK_ImGui_IsItemRightClicked ())
      {
        ImGui::SetNextWindowSize (ImVec2 (-1.0f, -1.0f), ImGuiCond_Always);
        ImGui::OpenPopup         ("ShaderSubMenu");
      }

      if (ImGui::BeginPopup      ("ShaderSubMenu"))
      {
        static concurrency::concurrent_unordered_set <SK_ComPtr <ID3D11ShaderResourceView> > empty_list;
        static concurrency::concurrent_unordered_set <SK_ComPtr <ID3D11ShaderResourceView> > empty_set;

        ShaderMenu ( (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)
                        ShaderBase                                  (shader_type),
                     GetShaderBlacklist                             (shader_type),
                     GetShaderBlacklistEx                           (shader_type),
                     line == list->sel ? GetShaderUsedResourceViews (shader_type) :
                                         empty_list,
                     line == list->sel ? GetShaderResourceSet       (shader_type) :
                                         empty_set,
                     it.crc32c );
        list->dirty = true;

        ImGui::EndPopup  ();
      }
      ImGui::PopID       ();

      line++;

      ImGui::PopStyleColor ();
    }
    ImGui::PopID ();

    SK_ComPtr <ID3DBlob>               pDisasm  = nullptr;
    SK_ComPtr <ID3D11ShaderReflection> pReflect = nullptr;

    HRESULT hr = E_FAIL;

    if (                         tracker->crc32c  != 0 &&
           (*disassembly).count (tracker->crc32c) == 0 )
    {
      static SK_Thread_HybridSpinlock* locks [6] = {
        cs_shader_vs.get (), cs_shader_ps.get (), cs_shader_gs.get (),
        cs_shader_ds.get (), cs_shader_hs.get (), cs_shader_cs.get ()
      };

      {
        std::scoped_lock <SK_Thread_HybridSpinlock>
             disasm_lock (*locks [sk_shader_state_s::ClassToIdx (shader_type)]);

        static SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>* repos [6] = {
          SK_D3D11_Shaders->vertex, SK_D3D11_Shaders->pixel,
          SK_D3D11_Shaders->geometry,
          SK_D3D11_Shaders->domain, SK_D3D11_Shaders->hull,
          SK_D3D11_Shaders->compute
        };

        auto repo =
          repos [sk_shader_state_s::ClassToIdx (shader_type)];

        hr =
         SK_D3D_Disassemble ( repo->descs [pDevice][tracker->crc32c].bytecode.data (),
                              repo->descs [pDevice][tracker->crc32c].bytecode.size (),
                                D3D_DISASM_ENABLE_DEFAULT_VALUE_PRINTS, "", &pDisasm);

        SK_ComPtr <ID3DBlob> pColorDisasm = nullptr;

        if ( SUCCEEDED (
               SK_D3D_Disassemble ( repo->descs [pDevice][tracker->crc32c].bytecode.data (),
                                    repo->descs [pDevice][tracker->crc32c].bytecode.size (),
                                       D3D_DISASM_ENABLE_COLOR_CODE |
                                       D3D_DISASM_ENABLE_DEFAULT_VALUE_PRINTS, "", &pColorDisasm ) ) )
        {
          auto path =
            SK_FormatStringW (
              LR"(%ws\dump\shaders\%s_%8x.html)",
                SK_D3D11_res_root->c_str (),
                sk_shader_state_s::ClassToPrefix (shader_type),
                  tracker->crc32c.load ()
            );

          SK_CreateDirectories (path.c_str ());

          FILE *fShader =
            _wfopen (path.c_str (), L"w");

          if (fShader != nullptr)
          {
            fwrite (               pColorDisasm->GetBufferPointer (),
              strlen ((const char*)pColorDisasm->GetBufferPointer ()),
                 1, fShader);
            fclose (fShader);
          }
        }

        if (SUCCEEDED (hr))
        {
         SK_D3D_Reflect     ( repo->descs [pDevice][tracker->crc32c].bytecode.data (),
                              repo->descs [pDevice][tracker->crc32c].bytecode.size (),
                                IID_ID3D11ShaderReflection, (void **)&pReflect);
        }
      }

      size_t len = SUCCEEDED (hr) ?
        strlen ((const char *)pDisasm->GetBufferPointer ())
                                  : 0;

      if (SUCCEEDED (hr) && len > 0)
      {
        char* szDisasm      = _strdup ((const char *)pDisasm->GetBufferPointer ());
        char* szDisasmEnd   = szDisasm != nullptr ?
                              szDisasm + len      : nullptr;

        char* comments_end  = nullptr;

        if (szDisasm != nullptr)
        {
          comments_end = strstr (szDisasm,          "\nvs_");
          if (comments_end == nullptr)
            comments_end    =                strstr (szDisasm,          "\nps_");
          if (comments_end == nullptr)
            comments_end    =                strstr (szDisasm,          "\ngs_");
          if (comments_end == nullptr)
            comments_end    =                strstr (szDisasm,          "\nhs_");
          if (comments_end == nullptr)
            comments_end    =                strstr (szDisasm,          "\nds_");
          if (comments_end == nullptr)
            comments_end    =                strstr (szDisasm,          "\ncs_");
          char*
              footer_begins =
             (comments_end != nullptr)   ?   strstr (comments_end + 1, "\n//") : nullptr;

          if (comments_end != nullptr)              *comments_end  = '\0';
          if (footer_begins!= nullptr)              *footer_begins = '\0';

          disassembly->try_emplace ( ReadAcquire ((volatile LONG *)&tracker->crc32c),
                                       shader_disasm_s { szDisasm,
                                                           comments_end    ? comments_end  + 1 : szDisasmEnd,
                                                             footer_begins ? footer_begins + 1 : szDisasmEnd
                                                       }
                                   );

          if (pReflect != nullptr)
          {
            D3D11_SHADER_DESC desc = { };

            if (SUCCEEDED (pReflect->GetDesc (&desc)))
            {
              for (UINT i = 0; i < desc.BoundResources; i++)
              {
                D3D11_SHADER_INPUT_BIND_DESC bind_desc = { };

                if (SUCCEEDED (pReflect->GetResourceBindingDesc (i, &bind_desc)))
                {
                  if (bind_desc.Type == D3D_SIT_CBUFFER)
                  {
                    ID3D11ShaderReflectionConstantBuffer* pReflectedCBuffer =
                      pReflect->GetConstantBufferByName (bind_desc.Name);

                    if (pReflectedCBuffer != nullptr)
                    {
                      D3D11_SHADER_BUFFER_DESC buffer_desc = { };

                      if (SUCCEEDED (pReflectedCBuffer->GetDesc (&buffer_desc)))
                      {
                        shader_disasm_s::constant_buffer cbuffer = { };

                        cbuffer.name = buffer_desc.Name;
                        cbuffer.size = buffer_desc.Size;
                        cbuffer.Slot = bind_desc.BindPoint;

                        EnumConstantBuffer (pReflectedCBuffer, cbuffer);

                        (*disassembly) [ReadAcquire ((volatile LONG *)&tracker->crc32c)].cbuffers.emplace_back (cbuffer);
                      }
                    }
                  }
                }
              }
            }
          }

          free (szDisasm);
        }
      }
    }
  }

  ImGui::EndChild      ();

  hovering = ImGui::IsItemHovered ();
  focused  = ImGui::IsItemFocused ();

  ImGui::PopStyleVar   ();
  ImGui::PopStyleColor ();

  ImGui::SameLine      ();
  ImGui::BeginGroup    ();

  if (ImGui::IsItemHovered (ImGuiHoveredFlags_RectOnly))
  {
    if (! scrolled)
    {
           if (ImGui::IsKeyPressed (ImGuiKey_LeftBracket,  false)) list->sel--;
      else if (ImGui::IsKeyPressed (ImGuiKey_RightBracket, false)) list->sel++;
    }
  }

  static bool wireframe_dummy = false;
  bool&       wireframe       = wireframe_dummy;

  static bool on_top_dummy = false;
  bool&       on_top       = on_top_dummy;

  // For the life of me, I don't remember why the code works this way :P
  static bool hud_dummy = false;
  bool&       hud       = hud_dummy;



  SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>* pShader = nullptr;

  switch (shader_type)
  {
    case sk_shader_class::Vertex:   pShader = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders->vertex;   break;
    case sk_shader_class::Pixel:    pShader = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders->pixel;    break;
    case sk_shader_class::Geometry: pShader = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders->geometry; break;
    case sk_shader_class::Hull:     pShader = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders->hull;     break;
    case sk_shader_class::Domain:   pShader = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders->domain;   break;
    case sk_shader_class::Compute:  pShader = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders->compute;  break;
  }


  if (                                              pShader != nullptr &&
       ReadAcquire ((volatile LONG *)&tracker->crc32c)      != 0x00    &&
                       sk::narrow_cast <SSIZE_T>(list->sel) >= 0       &&
                                                 list->sel  < (int)list->contents.size () )
  {
    ImGui::BeginGroup ();

    int num_used_textures = 0;

    std::set <ID3D11ShaderResourceView *> unique_views;

    if (! tracker->set_of_views.empty ())
    {
      for ( auto& it : tracker->set_of_views )
      {
        if (! unique_views.insert (it).second)
          continue;

        D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;

        it->GetDesc (&srv_desc);

        if (srv_desc.ViewDimension == D3D_SRV_DIMENSION_TEXTURE2D)
        {
          ++num_used_textures;
        }
      }
    }

    static char szDraws      [128] = { };
    static char szTextures   [ 64] = { };
    static char szRuntime    [ 32] = { };
    static char szAvgRuntime [ 32] = { };

    int immediate_draws = tracker->num_draws.load          ();
    int deferred_draws  = tracker->num_deferred_draws.load ();
    int draws =
       immediate_draws + deferred_draws;

    if (shader_type != sk_shader_class::Compute)
    {
      if (tracker->cancel_draws)
        snprintf (szDraws, 128, "%3i Skipped Draw%sLast Frame",  draws, draws != 1 ? "s " : " ");
      else
        snprintf (szDraws, 128, "%3i Draw%sLast Frame        " , draws, draws != 1 ? "s " : " ");
    }

    else
    {
      if (tracker->cancel_draws)
        snprintf (szDraws, 128, "%3i Skipped Dispatch%sLast Frame", draws, draws != 1 ? "es " : " ");
      else
        snprintf (szDraws, 128, "%3i Dispatch%sLast Frame        ", draws, draws != 1 ? "es " : " ");
    }

    snprintf (szTextures,   64, "(%i %s)",   num_used_textures, num_used_textures != 1 ? "textures" : "texture");
    snprintf (szRuntime,    32,  "%0.4f ms", tracker->runtime_ms);
    snprintf (szAvgRuntime, 32,  "%0.4f ms", tracker->runtime_ms / immediate_draws);

    ImGui::BeginGroup   ();
    ImGui::Separator    ();

    ImGui::Columns      (2, nullptr, false);
    ImGui::BeginGroup   ( );

    const bool skip =
      pShader->blacklist.count (tracker->crc32c);

    ImGui::PushID (tracker->crc32c);

    if (skip)
    {
      ImGui::PushStyleColor (ImGuiCol_Text, SkipColorCycle ());
    }

    bool cancel_ = skip;

    if (ImGui::Checkbox ( shader_type == sk_shader_class::Compute ? "Never Dispatch" :
                                                                    "Never Draw", &cancel_))
    {
      if (cancel_)
      {
        pShader->addTrackingRef (pShader->blacklist, tracker->crc32c);
        InterlockedIncrement    (&SK_D3D11_DrawTrackingReqs);
      }
      else if (pShader->blacklist.count (tracker->crc32c) != 0)
      {
        do {
          InterlockedDecrement                 (&SK_D3D11_DrawTrackingReqs);
        } while (! pShader->releaseTrackingRef (pShader->blacklist, tracker->crc32c));
      }
    }

    if (skip)
      ImGui::PopStyleColor ();

    if (shader_type != sk_shader_class::Compute)
    {
      wireframe = pShader->wireframe.count (tracker->crc32c) != 0;
      on_top    = pShader->on_top.count    (tracker->crc32c) != 0;
      hud       = pShader->hud.count       (tracker->crc32c) != 0;

      const bool wire = wireframe;

      if (wire)
        ImGui::PushStyleColor (ImGuiCol_Text, WireframeColorCycle ());

      if (ImGui::Checkbox ("Always Draw in Wireframe", &wireframe))
      {
        if (wireframe)
        {
          pShader->addTrackingRef (pShader->wireframe, tracker->crc32c);
          InterlockedIncrement    (&SK_D3D11_DrawTrackingReqs);
        }
        else if (pShader->wireframe.count (tracker->crc32c) != 0)
        {
          do {
            InterlockedDecrement                 (&SK_D3D11_DrawTrackingReqs);
          } while (! pShader->releaseTrackingRef (pShader->wireframe, tracker->crc32c));
        }
      }

      if (wire)
        ImGui::PopStyleColor ();

      const bool top = on_top;

      if (top)
        ImGui::PushStyleColor (ImGuiCol_Text, OnTopColorCycle ());

      if (ImGui::Checkbox ("Always Draw on Top", &on_top))
      {
        if (on_top)
        {
          pShader->addTrackingRef (pShader->on_top, tracker->crc32c);
          InterlockedIncrement    (&SK_D3D11_DrawTrackingReqs);
        }

        else if (pShader->on_top.count (tracker->crc32c) != 0)
        {
          do {
            InterlockedDecrement                 (&SK_D3D11_DrawTrackingReqs);
          } while (! pShader->releaseTrackingRef (pShader->on_top, tracker->crc32c));
        }
      }

      if (top)
        ImGui::PopStyleColor ();

      const bool hud_shader = hud;

      if (hud_shader)
        ImGui::PushStyleColor (ImGuiCol_Text, HudColorCycle ());

      if (ImGui::Checkbox ("Shader Belongs to HUD", &hud))
      {
        if (hud)
        {
          pShader->addTrackingRef     (pShader->hud, tracker->crc32c);

          InterlockedIncrement (&SK_D3D11_TrackingCount->Conditional);
        }

        else if (pShader->hud.count (tracker->crc32c) != 0)
        {
          InterlockedDecrement (&SK_D3D11_TrackingCount->Conditional);

          while (! pShader->releaseTrackingRef (pShader->hud, tracker->crc32c))
            ;
        }
      }

      if (hud_shader)
        ImGui::PopStyleColor ();
    }
    else
    {
      ImGui::Columns  (1);
    }

    if (config.reshade.is_addon && ( tracker->first_rtv.Format != DXGI_FORMAT_UNKNOWN &&
                                     ( static_cast <float> (tracker->first_rtv.Width) /
                                       static_cast <float> (tracker->first_rtv.Height) ) < (io.DisplaySize.x / io.DisplaySize.y) + 0.1f &&
                                     ( static_cast <float> (tracker->first_rtv.Width) /
                                       static_cast <float> (tracker->first_rtv.Height) ) > (io.DisplaySize.x / io.DisplaySize.y) - 0.1f && ( DirectX::BitsPerColor (tracker->first_rtv.Format) * 3 <=
                                                                                                                                             DirectX::BitsPerPixel (tracker->first_rtv.Format) ) ) && shader_type != sk_shader_class::Compute)
    {
      bool reshade_before = pShader->trigger_reshade.before.count (tracker->crc32c) != 0;
    //bool reshade_after  = pShader->trigger_reshade.after.count  (tracker->crc32c) != 0;

      bool toggle_by_keyboard =
        (io.KeyCtrl && ImGui::GetKeyData (ImGuiKey_R)->DownDuration == 0.0f);

      if (ImGui::Checkbox ("Trigger ReShade On First Draw", &reshade_before) || toggle_by_keyboard)
      {
        if (toggle_by_keyboard)
        {
          reshade_before = !reshade_before;
        }

        if (reshade_before)
        {
          pShader->addTrackingRef (pShader->trigger_reshade.before, tracker->crc32c);
          InterlockedIncrement    (&SK_D3D11_DrawTrackingReqs);
        }
        else if (pShader->trigger_reshade.before.count (tracker->crc32c) != 0)
        {
          do {
            InterlockedDecrement                 (&SK_D3D11_DrawTrackingReqs);
          }  while (!pShader->releaseTrackingRef (pShader->trigger_reshade.before, tracker->crc32c));
        }
      }

#if 0
      if (ImGui::Checkbox ("Trigger ReShade After First Draw", &reshade_after))
      {
        if (reshade_after)
        {
          pShader->addTrackingRef (pShader->trigger_reshade.after, tracker->crc32c);
          InterlockedIncrement    (&SK_D3D11_DrawTrackingReqs);
        }
        else if (pShader->trigger_reshade.after.count (tracker->crc32c) != 0)
        {
          do {
            InterlockedDecrement                 (&SK_D3D11_DrawTrackingReqs);
          } while (! pShader->releaseTrackingRef (pShader->trigger_reshade.after, tracker->crc32c));
        }
      }
#endif
    }

    ///else
    ///{
    ///  bool antistall = (ReadAcquire (&__SKX_ComputeAntiStall) != 0);
    ///
    ///  if (ImGui::Checkbox ("Use ComputeShader Anti-Stall For Buffer Injection", &antistall))
    ///  {
    ///    InterlockedExchange (&__SKX_ComputeAntiStall, antistall ? 1 : 0);
    ///  }
    ///}


    ImGui::PopID      ();
    ImGui::EndGroup   ();
    ImGui::EndGroup   ();

    ImGui::NextColumn ();

    ImGui::BeginGroup ();
    ImGui::BeginGroup ();

    ImGui::TextDisabled (szDraws);
    ImGui::TextDisabled ("Total GPU Runtime:");

    if (shader_type != sk_shader_class::Compute)
      ImGui::TextDisabled ("Avg Time per-Draw:");
    else
      ImGui::TextDisabled ("Avg Time per-Dispatch:");
    ImGui::EndGroup   ();

    ImGui::SameLine   ();

    ImGui::BeginGroup   ( );
    ImGui::TextDisabled (szTextures);
    ImGui::TextDisabled (immediate_draws > 0 ? szRuntime    : "N/A");
    ImGui::TextDisabled (immediate_draws > 0 ? szAvgRuntime : "N/A");
    ImGui::EndGroup     ( );
    ImGui::EndGroup     ( );

    ImGui::Columns      (1);
    ImGui::Separator    ( );

    SK_ImGui_AutoFont fixed_font (io.Fonts->Fonts[1]);

    ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.80f, 0.80f, 1.0f, 1.0f));
    ImGui::TextWrapped    ((*disassembly) [tracker->crc32c].header.c_str ());
    ImGui::PopStyleColor  ();

    ImGui::SameLine       ();
    ImGui::BeginGroup     ();

    ImGui::TreePush       ("");
    ImGui::Spacing        (); ImGui::Spacing ();


    ImGui::PushID (tracker->crc32c);

    size_t min_size = 0;

    for (auto& it : (*disassembly) [tracker->crc32c].cbuffers)
    {
      ImGui::PushID (it.Slot);

      if (! tracker->overrides.empty ())
      {
        if (tracker->overrides [0].parent != tracker->crc32c)
        {
          tracker->overrides.clear ();
        }
      }

      for (auto& itS : it.structs)
      {
        if (! itS.second.empty ())
        {
          ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.3f, 0.6f, 0.9f, 1.0f));
          ImGui::Text           (itS.second.c_str ());
          ImGui::PopStyleColor  (  );
          ImGui::TreePush       ("");
        }
      for (auto& it2 : itS.first)
      {
        ImGui::PushID (it2.var_desc.StartOffset);

        if (tracker->overrides.size () < ++min_size)
        {
          tracker->overrides.push_back ( { tracker->crc32c, it.size,
                                             false, it.Slot,
                                                   it2.var_desc.StartOffset,
                                                   it2.var_desc.Size, { 0.0f, 0.0f, 0.0f, 0.0f,
                                                                        0.0f, 0.0f, 0.0f, 0.0f,
                                                                        0.0f, 0.0f, 0.0f, 0.0f,
                                                                        0.0f, 0.0f, 0.0f, 0.0f } } );

          memcpy ( (void *)(tracker->overrides.back ().Values),
                                 (void *)it2.default_value,
                                   std::min ((size_t)it2.var_desc.Size, sizeof (float) * 128) );
        }

        auto& override_inst = tracker->overrides [min_size-1];

        ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.9f, 0.9f, 0.9f, 1.0f));


      const
       auto
        EnableButton = [&](void) -> void
        {
          ImGui::Checkbox ("##Enable", &override_inst.Enable);

          if (ImGui::IsItemHovered ())
          {
            ImGui::BeginTooltip ();
            ImGui::Separator    ();
            ImGui::TextColored  ( ImVec4 (1.f, 1.f, 1.f, 1.f),
                                  "CBuffer [%lu] => { Offset: %lu, Value Size: %lu }",
                                  override_inst.Slot,
                                                    override_inst.StartAddr,
                                                    override_inst.Size );
            ImGui::EndTooltip   ();
          }
        };


        if ( it2.type_desc.Class == D3D_SVC_SCALAR ||
             it2.type_desc.Class == D3D_SVC_VECTOR )
        {
          EnableButton ();

          if (it2.type_desc.Type == D3D_SVT_FLOAT)
          {
            ImGui::SameLine    ();
            ImGui::BeginGroup  ();
            ImGui::InputScalarN (it2.name.c_str (), ImGuiDataType_Float, &override_inst.Values [0], it2.type_desc.Columns);

            if (override_inst.Enable)
            {
              if (it2.type_desc.Elements > 1)
              {
                int idx = it2.type_desc.Columns;

                for ( UINT i = 1 ; i < it2.type_desc.Elements ; ++i )
                {
                  ImGui::PushID       (i);
                  ImGui::InputScalarN (SK_FormatString ("%s [%lu]", it2.name.c_str (), i).c_str (),
                                       ImGuiDataType_Float, &override_inst.Values [idx],  it2.type_desc.Columns);
                                                                                   idx += it2.type_desc.Columns;
                  ImGui::PopID        ( );
                }
              }
            }
            ImGui::EndGroup         ( );
          }

          else if ( it2.type_desc.Type == D3D_SVT_INT  ||
                    it2.type_desc.Type == D3D_SVT_UINT ||
                    it2.type_desc.Type == D3D_SVT_UINT8 )
          {
            if (it2.type_desc.Type == D3D_SVT_INT)
            {
              ImGui::SameLine  ();
              ImGui::InputInt4 (it2.name.c_str (), (int *)&override_inst.Values [0]);
            }

            else
            {
              for ( UINT k = 0 ; k < it2.type_desc.Columns ; k++ )
              {
                ImGui::SameLine  ( );
                ImGui::PushID    (k);

                int val = it2.type_desc.Type == D3D_SVT_UINT8 ? (int)((uint8_t *)override_inst.Values) [k] :
                                                                    ((uint32_t *)override_inst.Values) [k];

                if (ImGui::InputInt (it2.name.c_str (), &val))
                {
                  if (val < 0) val = 0;

                  if (it2.type_desc.Type == D3D_SVT_UINT8)
                  {
                    if (val > 255) val = 255;

                    (( uint8_t *)override_inst.Values) [k] = (uint8_t)val;
                  }

                  else
                    ((uint32_t *)override_inst.Values) [k] = (uint32_t)val;

                }
                ImGui::PopID     ( );
              }
            }
          }

          else if (it2.type_desc.Type == D3D_SVT_BOOL)
          {
            for ( UINT k = 0 ; k < it2.type_desc.Columns ; k++ )
            {
              ImGui::SameLine ( );
              ImGui::PushID   (k);
              ImGui::Checkbox (it2.name.c_str (), (bool *)&((BOOL *)override_inst.Values) [k]);
              ImGui::PopID    ( );
            }
          }
        }

        else if ( it2.type_desc.Class == D3D_SVC_MATRIX_ROWS ||
                  it2.type_desc.Class == D3D_SVC_MATRIX_COLUMNS )
        {
          EnableButton ();

          ImGui::SameLine ();

          ImGui::BeginGroup ();
          float* fMatrixPtr = override_inst.Values;

          for ( UINT k = 0 ; k < it2.type_desc.Rows; k++ )
          {
            ImGui::Text     (SK_FormatString ("%s <m%lu>", it2.name.c_str (), k).c_str ());
            ImGui::SameLine ( );
            ImGui::PushID   (k);
            ImGui::InputFloat4 ("##Matrix_Row",
                                 fMatrixPtr );
            ImGui::PopID    ( );

            fMatrixPtr += it2.type_desc.Columns;
          }
          ImGui::EndGroup   ( );
        }

        else if (it2.type_desc.Class == D3D_SVC_STRUCT)
        {
          ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.3f, 0.6f, 0.9f, 1.0f));
          ImGui::Text           (it2.name.c_str ());
          ImGui::PopStyleColor  ();
        }

        else
        {
          ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.9f, 0.6f, 0.3f, 1.0f));
          ImGui::Text           (it2.name.c_str ());
          ImGui::PopStyleColor  ();
        }

        ImGui::PopStyleColor  ();
        ImGui::PopID          ();
      }

      if (! itS.second.empty ())
        ImGui::TreePop ();
    }

      ImGui::PopID ();
    }

    ImGui::PopID ();

    ImGui::TreePop    ();
    ImGui::EndGroup   ();
    ImGui::EndGroup   ();

    if (ImGui::IsItemHovered (ImGuiHoveredFlags_RectOnly) && (! tracker->set_of_views.empty ()))
    {
      ImGui::BeginTooltip ();

      DXGI_FORMAT fmt = DXGI_FORMAT_UNKNOWN;

      unique_views.clear ();

      for ( auto& it : tracker->set_of_views )
      {
        D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = { };
                          it->GetDesc (&srv_desc);

        if (srv_desc.ViewDimension == D3D_SRV_DIMENSION_TEXTURE2D)
        {
          SK_ComPtr   <ID3D11Resource>        pRes = nullptr;
                            it->GetResource (&pRes.p);
          SK_ComQIPtr <ID3D11Texture2D> pTex (pRes.p);

          if ( pRes != nullptr &&
               pTex != nullptr )
          {
            D3D11_TEXTURE2D_DESC desc = { };
            pTex->GetDesc      (&desc);
                           fmt = desc.Format;

            if (desc.Height > 0 && desc.Width > 0)
            {
              if (! unique_views.insert (it).second)
                continue;

              SK_D3D11_TempResources->push_back (it.p);

              ImGui::Image ( it,         ImVec2  ( std::max (64.0f, (float)desc.Width / 16.0f),
        ((float)desc.Height / (float)desc.Width) * std::max (64.0f, (float)desc.Width / 16.0f) ),
                                         ImVec2  (0,0),             ImVec2  (1,1),
                              desc.Format == DXGI_FORMAT_R8_UNORM ? ImColor (0, 255, 255, 255) :
                                         ImColor (255,255,255,255), ImColor (242,242,13,255) );
            }

            ImGui::SameLine   ();

            ImGui::BeginGroup ();
            ImGui::Text       ("Texture:    ");
            ImGui::Text       ("Format:     ");
            ImGui::Text       ("References: ");
            ImGui::EndGroup   ();

            ImGui::SameLine   ();

            LONG refs =
              pTex.p->AddRef  () - 1;
              pTex.p->Release ();

            ImGui::BeginGroup ();
            ImGui::Text       ("%08lx", pTex.p);
            ImGui::Text       ("%hs",   SK_DXGI_FormatToStr (fmt).data ());
            ImGui::Text       ("%li",   refs);
            ImGui::EndGroup   ();
          }
        }
      }

      ImGui::EndTooltip ();
    }
#if 0
    ImGui::SameLine       ();
    ImGui::BeginGroup     ();
    ImGui::TreePush       ("");
    ImGui::Spacing        (); ImGui::Spacing ();
    ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.666f, 0.666f, 0.666f, 1.0f));

    char szName    [192] = { };
    char szOrdinal [64 ] = { };
    char szOrdEl   [ 96] = { };

    int  el = 0;

    ImGui::PushItemWidth (font_size * 25);

    for ( auto& it : tracker->constants )
    {
      if (it.struct_members.size ())
      {
        ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.9f, 0.1f, 0.7f, 1.0f));
        ImGui::Text           (it.Name);
        ImGui::PopStyleColor  ();

        for ( auto& it2 : it.struct_members )
        {
          snprintf ( szOrdinal, 64, " (%c%-3lu) ",
                        it2.RegisterSet != D3DXRS_SAMPLER ? 'c' : 's',
                          it2.RegisterIndex );
          snprintf ( szOrdEl,  96,  "%s::%lu %c", // Uniquely identify parameters that share registers
                       szOrdinal, el++, shader_type == tbf_shader_class::Pixel ? 'p' : 'v' );
          snprintf ( szName, 192, "[%s] %-24s :%s",
                       shader_type == tbf_shader_class::Pixel ? "ps" :
                                                                "vs",
                         it2.Name, szOrdinal );

          if (it2.Type == D3DXPT_FLOAT && it2.Class == D3DXPC_VECTOR)
          {
            ImGui::Checkbox    (szName,  &it2.Override); ImGui::SameLine ();
            ImGui::InputFloat4 (szOrdEl,  it2.Data);
          }
          else {
            ImGui::TreePush (""); ImGui::TextColored (ImVec4 (0.45f, 0.75f, 0.45f, 1.0f), szName); ImGui::TreePop ();
          }
        }

        ImGui::Separator ();
      }

      else
      {
        snprintf ( szOrdinal, 64, " (%c%-3lu) ",
                     it.RegisterSet != D3DXRS_SAMPLER ? 'c' : 's',
                        it.RegisterIndex );
        snprintf ( szOrdEl,  96,  "%s::%lu %c", // Uniquely identify parameters that share registers
                       szOrdinal, el++, shader_type == tbf_shader_class::Pixel ? 'p' : 'v' );
        snprintf ( szName, 192, "[%s] %-24s :%s",
                     shader_type == tbf_shader_class::Pixel ? "ps" :
                                                              "vs",
                         it.Name, szOrdinal );

        if (it.Type == D3DXPT_FLOAT && it.Class == D3DXPC_VECTOR)
        {
          ImGui::Checkbox    (szName,  &it.Override); ImGui::SameLine ();
          ImGui::InputFloat4 (szOrdEl,  it.Data);
        } else {
          ImGui::TreePush (""); ImGui::TextColored (ImVec4 (0.45f, 0.75f, 0.45f, 1.0f), szName); ImGui::TreePop ();
        }
      }
    }
    ImGui::PopItemWidth ();
    ImGui::TreePop      ();
    ImGui::EndGroup     ();
#endif

    ImGui::Separator      ();

    ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.99f, 0.99f, 0.01f, 1.0f));
    ImGui::TextWrapped    ((*disassembly) [ReadAcquire ((volatile LONG *)&tracker->crc32c)].code.c_str ());

    ImGui::Separator      ();

    ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.5f, 0.95f, 0.5f, 1.0f));
    ImGui::TextWrapped    ((*disassembly) [ReadAcquire ((volatile LONG *)&tracker->crc32c)].footer.c_str ());

    ImGui::PopStyleColor (2);
  }
  else
    tracker->cancel_draws = false;

  ImGui::EndGroup ();

  list->last_ht    = ImGui::GetItemRectSize ().y;

  ImGui::EndGroup ();

  list->last_min   = ImGui::GetItemRectMin ();
  list->last_max   = ImGui::GetItemRectMax ();

  ImGui::PopID    ();
}


std::unordered_map < sk_shader_class,
                     std::tuple < std::pair <const ImGuiCol, const ImColor>,
                                  std::pair <const ImGuiCol, const ImColor>,
                                  std::pair <const ImGuiCol, const ImColor> > >&
SK_D3D11_ShaderColors (void)
{
  static
    std::unordered_map < sk_shader_class,
                     std::tuple < std::pair <const ImGuiCol, const ImColor>,
                                  std::pair <const ImGuiCol, const ImColor>,
                                  std::pair <const ImGuiCol, const ImColor> > >
    colors =
      { { sk_shader_class::Unknown,  ShaderColorDecl (-1) },
        { sk_shader_class::Vertex,   ShaderColorDecl ( 0) },
        { sk_shader_class::Pixel,    ShaderColorDecl ( 1) },
        { sk_shader_class::Geometry, ShaderColorDecl ( 2) },
        { sk_shader_class::Hull,     ShaderColorDecl ( 3) },
        { sk_shader_class::Domain,   ShaderColorDecl ( 4) },
        { sk_shader_class::Compute,  ShaderColorDecl ( 5) } };

  return colors;
}



void
SK_D3D11_LiveShaderView (bool& can_scroll)
{
  auto& io =
    ImGui::GetIO ();

  // This causes the stats below to update
  SK_ImGui_Widgets->d3d11_pipeline->setActive (true);

  ImGui::TreePush ("");

  auto& shaders =
    SK_D3D11_Shaders;

  auto ShaderClassMenu = [&](sk_shader_class shader_type) ->
    void
    {
      bool        used_last_frame       = false;
      bool        ui_link_activated     = false;
      char        label           [512] = { };
      char        szPipelineDesc  [512] = { };

      switch (shader_type)
      {
        case sk_shader_class::Vertex:
          ui_link_activated = change_sel_vs != 0x00;
          used_last_frame   = shaders->vertex.changes_last_frame > 0;
          //SK_D3D11_GetVertexPipelineDesc (wszPipelineDesc);
          sprintf (label,     "Vertex Shaders\t\t%hs###LiveVertexShaderTree", szPipelineDesc);
          break;
        case sk_shader_class::Pixel:
          ui_link_activated = change_sel_ps != 0x00;
          used_last_frame   = shaders->pixel.changes_last_frame > 0;
          //SK_D3D11_GetRasterPipelineDesc (wszPipelineDesc);
          //lstrcatW                       (wszPipelineDesc, L"\t\t");
          //SK_D3D11_GetPixelPipelineDesc  (wszPipelineDesc);
          sprintf (label,     "Pixel Shaders\t\t%hs###LivePixelShaderTree", szPipelineDesc);
          break;
        case sk_shader_class::Geometry:
          ui_link_activated = change_sel_gs != 0x00;
          used_last_frame   = shaders->geometry.changes_last_frame > 0;
          //SK_D3D11_GetGeometryPipelineDesc (wszPipelineDesc);
          sprintf (label,     "Geometry Shaders\t\t%hs###LiveGeometryShaderTree", szPipelineDesc);
          break;
        case sk_shader_class::Hull:
          ui_link_activated = change_sel_hs != 0x00;
          used_last_frame   = shaders->hull.changes_last_frame > 0;
          //SK_D3D11_GetTessellationPipelineDesc (wszPipelineDesc);
          sprintf (label,     "Hull Shaders\t\t%hs###LiveHullShaderTree", szPipelineDesc);
          break;
        case sk_shader_class::Domain:
          ui_link_activated = change_sel_ds != 0x00;
          used_last_frame   = shaders->domain.changes_last_frame > 0;
          //SK_D3D11_GetTessellationPipelineDesc (wszPipelineDesc);
          sprintf (label,     "Domain Shaders\t\t%hs###LiveDomainShaderTree", szPipelineDesc);
          break;
        case sk_shader_class::Compute:
          ui_link_activated = change_sel_cs != 0x00;
          used_last_frame   = shaders->compute.changes_last_frame > 0;
          //SK_D3D11_GetComputePipelineDesc (wszPipelineDesc);
          sprintf (label,     "Compute Shaders\t\t%hs###LiveComputeShaderTree", szPipelineDesc);
          break;
        default:
          break;
      }

      if (used_last_frame)
      {
        if (ui_link_activated)
          ImGui::SetNextItemOpen (true, ImGuiCond_Always);

        static auto& colors =
          SK_D3D11_ShaderColors ();

        ImGui::PushStyleColor ( std::get <0> (colors [shader_type]).first,
                        (ImVec4)std::get <0> (colors [shader_type]).second );
        ImGui::PushStyleColor ( std::get <1> (colors [shader_type]).first,
                        (ImVec4)std::get <1> (colors [shader_type]).second );
        ImGui::PushStyleColor ( std::get <2> (colors [shader_type]).first,
                        (ImVec4)std::get <2> (colors [shader_type]).second );

        if (ImGui::CollapsingHeader (label))
        {
          SK_LiveShaderClassView (shader_type, can_scroll);
        }

        ImGui::PopStyleColor (3);
      }
    };

  ImGui::TreePush    ("");
  SK_ImGui_AutoFont fixed_font (
     io.Fonts->Fonts [SK_IMGUI_FIXED_FONT]
  );
  ImGui::TextColored (ImColor (238, 250, 5), "%hs", SK::DXGI::getPipelineStatsDesc ().c_str ());
  fixed_font.Detach  ();

  ImGui::Separator   ();

  if (ImGui::Button (" Clear Shader State "))
  {
    SK_D3D11_ClearShaderState ();
  }

  ImGui::SameLine ();

  if (ImGui::Button (" Store Shader State "))
  {
    SK_D3D11_StoreShaderState ();
  }

  ImGui::SameLine ();

  if (ImGui::Button (" Add to Shader State "))
  {
    SK_D3D11_LoadShaderState (false);
  }

  ImGui::SameLine ();

  if (ImGui::Button (" Restore FULL Shader State "))
  {
    SK_D3D11_LoadShaderState ();
  }

  //SK_RunOnce ( SK_D3D11_DontTrackUnlessModToolsAreOpen =
  //             ( SK_GetCurrentGameID () == SK_GAME_ID::YakuzaKiwami2 ) );

  ImGui::SameLine ();
  ImGui::Checkbox ("StateTrack Only when Tools are Open",
                   &SK_D3D11_DontTrackUnlessModToolsAreOpen);

  //ImGui::Checkbox ("Convert typeless resource views", &convert_typeless);

  ImGui::TreePop     ();

  ShaderClassMenu (sk_shader_class::Vertex);
  ShaderClassMenu (sk_shader_class::Pixel);
  ShaderClassMenu (sk_shader_class::Geometry);
  ShaderClassMenu (sk_shader_class::Hull);
  ShaderClassMenu (sk_shader_class::Domain);
  ShaderClassMenu (sk_shader_class::Compute);

  ImGui::TreePop ();
}

void
SK_D3D11_ShaderModDlg_RTVContributors (void)
{
  ImGui::BeginChild ( ImGui::GetID ("RenderTargetContributors"),
                    ImVec2 ( -1.0f ,//std::max (font_size * 30.0f, effective_width + 24.0f),
                             -1.0f ),
                      true,
                        ImGuiWindowFlags_AlwaysAutoResize |
                        ImGuiWindowFlags_NavFlattened );

  static auto& colors =
    SK_D3D11_ShaderColors ();

  auto _SetupShaderHeaderColors =
  [&](sk_shader_class type)
  {
    ImGui::PushStyleColor ( std::get <0> (colors [type]).first,
                            std::get <0> (colors [type]).second.Value );
    ImGui::PushStyleColor ( std::get <1> (colors [type]).first,
                            std::get <1> (colors [type]).second.Value );
    ImGui::PushStyleColor ( std::get <2> (colors [type]).first,
                            std::get <2> (colors [type]).second.Value );
  };

  struct ref_list_base {
    sk_shader_class
                type         = sk_shader_class::Vertex;
    const char* namespace_id = "Vertex";
    const char* header_title = "Vertex Shaders##rtv_refs";

    concurrency::concurrent_unordered_set <uint32_t>&
                ref_tracker  = tracked_rtv->ref_vs;
    SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>&
                shader       = SK_D3D11_Shaders->vertex;
    uint32_t&   selected_id  = change_sel_vs;

    struct {
      bool      separated    = false;
      concurrency::concurrent_unordered_set <uint32_t>*
                sibling      = nullptr;
    } ui;
  } lists [] = {
    {  sk_shader_class::Vertex,          "Vertex",
                       "Vertex Shaders##rtv_refs",
                                tracked_rtv->ref_vs,
      SK_D3D11_Shaders->vertex,       change_sel_vs,
                                             { true,
                               &tracked_rtv->ref_ps } },

    {  sk_shader_class::Pixel,           "Pixel",
                       "Pixel Shaders##rtv_refs",
                               tracked_rtv->ref_ps,
      SK_D3D11_Shaders->pixel,       change_sel_ps },

    {  sk_shader_class::Geometry,        "Geometry",
                       "Geometry Shaders##rtv_refs",
                                  tracked_rtv->ref_gs,
      SK_D3D11_Shaders->geometry,       change_sel_gs,
                                                 true },

    {  sk_shader_class::Hull,            "Hull",
                       "Hull Shaders##rtv_refs",
                              tracked_rtv->ref_hs,
      SK_D3D11_Shaders->hull,       change_sel_hs,
                                           { true,
                             &tracked_rtv->ref_ds } },

    {  sk_shader_class::Domain,          "Domain",
                       "Domain Shaders##rtv_refs",
                                tracked_rtv->ref_ds,
      SK_D3D11_Shaders->domain,       change_sel_ds },

    {  sk_shader_class::Compute,         "Compute",
                       "Compute Shaders##rtv_refs",
                                tracked_rtv->ref_cs,
      SK_D3D11_Shaders->compute,      change_sel_cs,
                                               true }
  };

  for ( auto& it : lists )
  {
    if (  it.ui.sibling != nullptr) ImGui::Columns    (2);
    if (! it.ui.separated)          ImGui::NextColumn ( );

    if ( it.ref_tracker.empty () &&
           ( it.ui.sibling == nullptr ||
             it.ui.sibling->empty () )
       ) continue;

    // Draw the separator if any of the two columns has data
    if (  it.ui.separated)          ImGui::Separator  ( );
    if (  it.ref_tracker.empty ()) // Now bail-out if no data
      continue;

    d3d11_shader_tracking_s&
      tracker = it.shader.tracked;

    _SetupShaderHeaderColors (it.type);

    if ( ImGui::CollapsingHeader ( it.header_title,
                          ImGuiTreeNodeFlags_DefaultOpen )
       )
    {
      ImGui::TreePush (it.namespace_id);

      for ( auto& tracked : it.ref_tracker )
      {
        bool selected =
          ( tracker.crc32c.load () == tracked );

        if (IsSkipped (it.type, tracked))
        {
          ImGui::PushStyleColor ( ImGuiCol_Text,
                                     SkipColorCycle () );
        }

        else if (IsOnTop (it.type, tracked))
        {
          ImGui::PushStyleColor ( ImGuiCol_Text,
                                    OnTopColorCycle () );
        }

        else if (IsWireframe (it.type, tracked))
        {
          ImGui::PushStyleColor ( ImGuiCol_Text,
                                    WireframeColorCycle () );
        }

        else
        {
          ImGui::PushStyleColor ( ImGuiCol_Text,
                                    ImVec4 ( 0.95f, 0.95f,
                                             0.95f, 1.0f )
                                );
        }

        const bool disabled =
          it.shader.blacklist.count (tracked) != 0;

        if ( ImGui::Selectable (
               SK_FormatString ( "%s%08lx", disabled ? "*" :
                                                       " ",
                                             tracked
                               ).c_str (), &selected
                               )
           )
        {
          it.selected_id = tracked;
        }

        ImGui::PopStyleColor ();

        if (SK_ImGui_IsItemRightClicked ())
        {
          ImGui::SetNextWindowSize ( ImVec2 (-1.0f, -1.0f),
                                       ImGuiCond_Always );
          ImGui::OpenPopup         ( "ShaderSubMenu");
        }

        if (ImGui::BeginPopup ("ShaderSubMenu"))
        {
          ShaderMenu (
            &it.shader,
             it.shader.blacklist,
             it.shader.blacklist_if_texture,
             it.shader.tracked.set_of_views,
             it.shader.tracked.set_of_views,
             tracked
          );
          ImGui::EndPopup ();
        }
      }

      ImGui::TreePop     ( );
    }

    ImGui::PopStyleColor (3);
  }

  ImGui::Columns     (1);
  ImGui::EndChild    ( );
}




const std::unordered_map <std::wstring, SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*>*
__SK_Singleton_D3D11_ShaderClassMap (void)
{
  static auto& shaders  = SK_D3D11_Shaders;
  static auto& vertex   = shaders->vertex;
  static auto& pixel    = shaders->pixel;
  static auto& geometry = shaders->geometry;
  static auto& domain   = shaders->domain;
  static auto& hull     = shaders->hull;
  static auto& compute  = shaders->compute;

  static const
  std::unordered_map <std::wstring, SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*>
    SK_D3D11_ShaderClassMap_ =
    {
      std::make_pair (L"Vertex",   (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&vertex),
      std::make_pair (L"VS",       (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&vertex),

      std::make_pair (L"Pixel",    (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&pixel),
      std::make_pair (L"PS",       (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&pixel),

      std::make_pair (L"Geometry", (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&geometry),
      std::make_pair (L"GS",       (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&geometry),

      std::make_pair (L"Hull",     (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&hull),
      std::make_pair (L"HS",       (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&hull),

      std::make_pair (L"Domain",   (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&domain),
      std::make_pair (L"DS",       (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&domain),

      std::make_pair (L"Compute",  (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&compute),
      std::make_pair (L"CS",       (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&compute)
    };

  return &SK_D3D11_ShaderClassMap_;
}

#define SK_D3D11_ShaderClassMap (*__SK_Singleton_D3D11_ShaderClassMap ())

struct SK_D3D11_CommandBase
{
  struct ShaderMods
  {
    class Load : public SK_ICommand
    {
    public:
      SK_ICommandResult execute (_Notnull_ const char* szArgs)  override
      {
        if (szArgs [0] != '\0')
        {
          std::wstring args =
            SK_UTF8ToWideChar (szArgs);

          SK_D3D11_LoadShaderStateEx (args, false);
        }

        else
          SK_D3D11_LoadShaderState (true);

        return
          SK_ICommandResult ("D3D11.ShaderMods.Load", szArgs, "done", 1, nullptr, this);
      }

      const char* getHelp       (void)       noexcept override
      {
        return "(Re)Load d3d11_shaders.ini";
      }
    };

    class Unload : public SK_ICommand
    {
    public:
      SK_ICommandResult execute (const char* szArgs)  override
      {
        std::wstring args =
          SK_UTF8ToWideChar (szArgs);

        SK_D3D11_UnloadShaderState (args);

        return SK_ICommandResult ("D3D11.ShaderMods.Unload", szArgs, "done", 1, nullptr, this);
      }

      const char* getHelp       (void)       noexcept override
      {
        return "Unload <shader.ini>";
      }
    };

    class ToggleConfig : public SK_ICommand
    {
    public:
      SK_ICommandResult execute (const char* szArgs)  override
      {
        std::wstring args =
          SK_UTF8ToWideChar (szArgs);

        auto& loaded_configs =
          _SK_D3D11_LoadedShaderConfigs ();

        if (    loaded_configs.count (args) &&
                loaded_configs       [args])
        {
          SK_D3D11_UnloadShaderState (args);
        }
        else
        {
          SK_D3D11_LoadShaderStateEx (args, false);
        }

        return SK_ICommandResult ("D3D11.ShaderMods.ToggleConfig", szArgs, "done", 1, nullptr, this);
      }

      const char* getHelp       (void)       noexcept override
      {
        return "Load or Unload <shader.ini>";
      }
    };

    class Store : public SK_ICommand
    {
    public:
      SK_ICommandResult execute (const char* szArgs)  override
      {
        SK_D3D11_StoreShaderState ();

        return SK_ICommandResult ("D3D11.ShaderMods.Store", szArgs, "done", 1, nullptr, this);
      }

      const char* getHelp       (void)       noexcept override
      {
        return "Store d3d11_shaders.ini";
      }
    };

    class Clear : public SK_ICommand
    {
    public:
      SK_ICommandResult execute (const char* szArgs)  override
      {
        SK_D3D11_ClearShaderState ();

        return SK_ICommandResult ("D3D11.ShaderMods.Clear", szArgs, "done", 1, nullptr, this);
      }

      const char* getHelp       (void)       noexcept override
      {
        return "Disable all Shader Mods";
      }
    };

    class Set : public SK_ICommand
    {
    public:
      SK_ICommandResult execute (const char* szArgs)  override
      {
        std::wstring wstr_cpy (SK_UTF8ToWideChar (szArgs));

        wchar_t* wszBuf = nullptr;
        wchar_t* wszTok =
          std::wcstok (wstr_cpy.data (), L" ", &wszBuf);

        int arg = 0;

        SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>* shader_registry = nullptr;
        uint32_t                                          shader_hash     = 0x0;

        bool new_state     = false;
        bool new_condition = false;

        while (wszTok != nullptr)
        {
          switch (arg++)
          {
            case 0:
            {
              if (SK_D3D11_ShaderClassMap.count (wszTok) == 0)
                return SK_ICommandResult ("D3D11.ShaderMod.Set", szArgs, "Invalid Shader Type", 0, nullptr, this);
              else
                shader_registry = SK_D3D11_ShaderClassMap.at (std::wstring (wszTok));
            } break;

            case 1:
            {
              shader_hash = std::wcstoul (wszTok, nullptr, 16);
            } break;

            case 2:
            {
              if (shader_registry != nullptr)
              {
                if (     0 == _wcsicmp (wszTok, L"Wireframe"))
                { new_state = true; shader_registry->addTrackingRef (shader_registry->wireframe, shader_hash); }
                else if (0 == _wcsicmp (wszTok, L"OnTop"))
                { new_state = true; shader_registry->addTrackingRef (shader_registry->on_top,    shader_hash); }
                else if (0 == _wcsicmp (wszTok, L"Disable"))
                { new_state = true; shader_registry->addTrackingRef (shader_registry->blacklist, shader_hash); }
                else if (0 == _wcsicmp (wszTok, L"TriggerReShade"))
                { new_state = (intptr_t)SK_ReShade_GetDLL () > 0;
                  shader_registry->addTrackingRef (shader_registry->trigger_reshade.before,    shader_hash);   }
                else if (0 == _wcsicmp (wszTok, L"HUD"))
                { new_condition = true; shader_registry->addTrackingRef (shader_registry->hud, shader_hash);   }
                else
                  return SK_ICommandResult ("D3D11.ShaderMod.Set", szArgs, "Invalid Shader State", 0, nullptr, this);
              }
            } break;
          }

          wszTok =
            std::wcstok (nullptr, L" ", &wszBuf);
        }

        if (new_state)
        {
          InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
          InterlockedIncrement (&SK_D3D11_TrackingCount->Always);
        }

        else if (new_condition)
        {
          InterlockedIncrement (&SK_D3D11_TrackingCount->Conditional);
        }

        return SK_ICommandResult ("D3D11.ShaderMods.Set", szArgs, "done", 1, nullptr, this);
      }

      const char* getHelp       (void)       noexcept override
      {
        return "(Vertex,VS)|(Pixel,PS)|(Geometry,GS)|(Hull,HS)|(Domain,DS)|(Compute,CS)  <Hash>  {Disable|Wireframe|OnTop|TriggerReShade}";
      }
    };

    class Unset : public SK_ICommand
    {
    public:
      SK_ICommandResult execute (const char* szArgs)  override
      {
        std::wstring wstr_cpy (SK_UTF8ToWideChar (szArgs));

        wchar_t* wszBuf = nullptr;
        wchar_t* wszTok =
          std::wcstok (wstr_cpy.data (), L" ", &wszBuf);

        int arg = 0;

        SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>* shader_registry = nullptr;
        uint32_t                                          shader_hash     = 0x0;

        bool old_state     = false;
        bool old_condition = false;

        while (wszTok != nullptr)
        {
          switch (arg++)
          {
            case 0:
            {
              if (! SK_D3D11_ShaderClassMap.count (wszTok))
                return SK_ICommandResult ("D3D11.ShaderMod.Unset", szArgs, "Invalid Shader Type", 0, nullptr, this);
              else
                shader_registry = SK_D3D11_ShaderClassMap.at (wszTok);
            } break;

            case 1:
            {
              shader_hash = std::wcstoul (wszTok, nullptr, 16);
            } break;

            case 2:
            {
              if (shader_registry != nullptr)
              {
                if (0 == _wcsicmp (wszTok, L"Wireframe"))
                {
                  if (shader_registry->wireframe.count (shader_hash))
                  {
                    old_state = true;
                    shader_registry->releaseTrackingRef (shader_registry->wireframe, shader_hash);
                  }
                }
                else if (0 == _wcsicmp (wszTok, L"OnTop"))
                {
                  if (shader_registry->on_top.count (shader_hash))
                  {
                    old_state = true;
                    shader_registry->releaseTrackingRef (shader_registry->on_top, shader_hash);
                  }
                }
                else if (0 == _wcsicmp (wszTok, L"Disable"))
                {
                  if (shader_registry->blacklist.count (shader_hash))
                  {
                    old_state = true;
                    shader_registry->releaseTrackingRef (shader_registry->blacklist, shader_hash);
                  }
                }
                else if (0 == _wcsicmp (wszTok, L"TriggerReShade"))
                {
                  if (shader_registry->trigger_reshade.before.count (shader_hash))
                  {
                    old_state = (intptr_t)SK_ReShade_GetDLL () > 0;
                    shader_registry->releaseTrackingRef (shader_registry->trigger_reshade.before, shader_hash);
                  }
                }
                else if (0 == _wcsicmp (wszTok, L"HUD"))
                {
                  if (shader_registry->hud.count (shader_hash))
                  {
                    old_condition = true;
                    shader_registry->releaseTrackingRef (shader_registry->hud, shader_hash);
                  }
                }
                else
                  return SK_ICommandResult ("D3D11.ShaderMod.Unset", szArgs, "Invalid Shader State", 0, nullptr, this);
              }
            } break;
          }

          wszTok =
            std::wcstok (nullptr, L" ", &wszBuf);
        }

        if (old_state)
        {
          InterlockedDecrement (&SK_D3D11_DrawTrackingReqs);
          InterlockedDecrement (&SK_D3D11_TrackingCount->Always);
        }

        else if (old_condition)
        {
          InterlockedDecrement (&SK_D3D11_TrackingCount->Conditional);
        }

        return SK_ICommandResult ("D3D11.ShaderMods.Unset", szArgs, "done", 1, nullptr, this);
      }

      const char* getHelp       (void)        noexcept override
      {
        return "(Vertex,VS)|(Pixel,PS)|(Geometry,GS)|(Hull,HS)|(Domain,DS)|(Compute,CS)  <Hash>  {Disable|Wireframe|OnTop|TriggerReShade}";
      }
    };

    class Toggle : public SK_ICommand
    {
    public:
      SK_ICommandResult execute (const char* szArgs)  override
      {
        std::wstring wstr_cpy (SK_UTF8ToWideChar (szArgs));

        wchar_t* wszBuf = nullptr;
        wchar_t* wszTok =
          std::wcstok (wstr_cpy.data (), L" ", &wszBuf);

        int arg = 0;

        SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>* shader_registry = nullptr;
        uint32_t                                          shader_hash     = 0x0;

        bool new_state     = false;
        bool new_condition = false;
        bool old_state     = false;
        bool old_condition = false;

        while (wszTok != nullptr)
        {
          switch (arg++)
          {
            case 0:
            {
              if (SK_D3D11_ShaderClassMap.count (wszTok) == 0)
                return SK_ICommandResult ("D3D11.ShaderMod.Toggle", szArgs, "Invalid Shader Type", 1, nullptr, this);
              else
                shader_registry = SK_D3D11_ShaderClassMap.at (wszTok);
            } break;

            case 1:
            {
              shader_hash = std::wcstoul (wszTok, nullptr, 16);
            } break;

            case 2:
            {
              if (shader_registry != nullptr)
              {
                if (0 == _wcsicmp (wszTok, L"Wireframe"))
                {
                  if (shader_registry->wireframe.count   (shader_hash) == 0)
                  {
                    new_state = true;
                    shader_registry->addTrackingRef      (shader_registry->wireframe, shader_hash);
                  }
                  else
                  {
                    old_state = true;
                    shader_registry->releaseTrackingRef  (shader_registry->wireframe, shader_hash);
                  }
                }
                else if (0 == _wcsicmp (wszTok, L"OnTop"))
                {
                  if (shader_registry->on_top.count      (shader_hash) == 0)
                  {
                    new_state = true;
                    shader_registry->addTrackingRef      (shader_registry->on_top, shader_hash);
                  }
                  else
                  {
                    old_state = true;
                    shader_registry->releaseTrackingRef  (shader_registry->on_top, shader_hash);
                  }
                }
                else if (0 == _wcsicmp (wszTok, L"Disable"))
                {
                  if (shader_registry->blacklist.count   (shader_hash) == 0)
                  {
                    new_state = true;
                    shader_registry->addTrackingRef      (shader_registry->blacklist, shader_hash);
                  }
                  else
                  {
                    old_state = true;
                    shader_registry->releaseTrackingRef  (shader_registry->blacklist, shader_hash);
                  }
                }
                else if (0 == _wcsicmp (wszTok, L"TriggerReShade"))
                {
                  if (shader_registry->trigger_reshade.before.count (shader_hash) == 0)
                  {
                    new_state = (intptr_t)SK_ReShade_GetDLL () > 0;
                    shader_registry->addTrackingRef                   (shader_registry->trigger_reshade.before, shader_hash);
                  }
                  else
                  {
                    old_state = (intptr_t)SK_ReShade_GetDLL () > 0;
                    shader_registry->releaseTrackingRef               (shader_registry->trigger_reshade.before, shader_hash);
                  }
                }
                else if (0 == _wcsicmp (wszTok, L"HUD"))
                {
                  if (shader_registry->hud.count        (shader_hash) == 0)
                  {
                    new_condition = true;
                    shader_registry->addTrackingRef     (shader_registry->hud, shader_hash);
                  }
                  else
                  {
                    old_condition = true;
                    shader_registry->releaseTrackingRef (shader_registry->hud, shader_hash);
                  }
                }
                else
                  return SK_ICommandResult ("D3D11.ShaderMod.Toggle", szArgs, "Invalid Shader State", 1, nullptr, this);
              }
            } break;
          }

          wszTok =
            std::wcstok (nullptr, L" ", &wszBuf);
        }

        if (new_state)
        {
          InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
          InterlockedIncrement (&SK_D3D11_TrackingCount->Always);
        }

        if (new_condition)
        {
          InterlockedIncrement (&SK_D3D11_TrackingCount->Conditional);
        }

        if (old_state)
        {
          InterlockedDecrement (&SK_D3D11_DrawTrackingReqs);
          InterlockedDecrement (&SK_D3D11_TrackingCount->Always);
        }

        if (old_condition)
        {
          InterlockedDecrement (&SK_D3D11_TrackingCount->Conditional);
        }

        return SK_ICommandResult ("D3D11.ShaderMods.Toggle", szArgs, "done", 1, nullptr, this);
      }

      const char* getHelp       (void)        noexcept override
      {
        return "(Vertex,VS)|(Pixel,PS)|(Geometry,GS)|(Hull,HS)|(Domain,DS)|(Compute,CS)  <Hash>  {Disable|Wireframe|OnTop|TriggerReShade}";
      }
    };
  };

  SK_D3D11_CommandBase (void)
  {
    static bool init = false;

    // There should be only one (!!)
    SK_ReleaseAssert (std::exchange (init, true) == false);

    gsl::not_null <SK_ICommandProcessor *> cp =
      SK_Render_InitializeSharedCVars ();

    // XXX: gsl::not_null <...> followed by null check --- lol
    if (! cp)
      return;

    cp->AddCommand ( "D3D11.ShaderMods.Load",
        new (std::nothrow) ShaderMods::Load         () );
    cp->AddCommand ( "D3D11.ShaderMods.Unload",
        new (std::nothrow) ShaderMods::Unload       () );
    cp->AddCommand ( "D3D11.ShaderMods.ToggleConfig",
        new (std::nothrow) ShaderMods::ToggleConfig () );
    cp->AddCommand ( "D3D11.ShaderMods.Store",
        new (std::nothrow) ShaderMods::Store        () );
    cp->AddCommand ( "D3D11.ShaderMods.Clear",
        new (std::nothrow) ShaderMods::Clear        () );
    cp->AddCommand ( "D3D11.ShaderMods.Set",
        new (std::nothrow) ShaderMods::Set          () );
    cp->AddCommand ( "D3D11.ShaderMods.Unset",
        new (std::nothrow) ShaderMods::Unset        () );
    cp->AddCommand ( "D3D11.ShaderMods.Toggle",
        new (std::nothrow) ShaderMods::Toggle       () );
  }
};

SK_LazyGlobal <SK_D3D11_CommandBase> SK_D3D11_Commands;

void* SK_D3D11_InitShaderMods (void) noexcept
{
  static auto *p =
    SK_D3D11_Commands.getPtr ();

  return p; // We're interested in the side-effects of accessing SK_D3D11_Commands,
            //   not the actual object it returns.
}