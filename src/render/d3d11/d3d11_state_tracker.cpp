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

#include <SpecialK/control_panel/d3d11.h>
#include <SpecialK/render/d3d11/d3d11_state_tracker.h>

//// Only valid for the thread driving an immediate context.
struct SK_ImGui_DrawCtx
{
  BOOL drawing = FALSE;
} static SK_ImGui_DrawState [SK_D3D11_MAX_DEV_CONTEXTS + 1];


//volatile LONG SK_ImGui_Drawing = 0;
//
//bool
//SK_ImGui_EnterDrawCycle (SK_TLS *pTLS_to_Update = nullptr, bool global = true)
//{
//  if (global)
//    InterlockedIncrement (&SK_ImGui_Drawing);
//
//  return true;
//}
//
//bool
//SK_ImGui_ExitDrawCycle (SK_TLS *pTLS_to_Update = nullptr, bool global = true)
//{
//  if (global)
//    InterlockedIncrement (&SK_ImGui_Drawing);
//
//  return true;
//}

bool SK_D3D11_EnableTracking      = false;
bool SK_D3D11_EnableMMIOTracking  = false;
volatile LONG
     SK_D3D11_DrawTrackingReqs    = 0L;
volatile LONG
     SK_D3D11_CBufferTrackingReqs = 0L;

bool
SK_ImGui_IsDrawing_OnD3D11Ctx (UINT& dev_idx, ID3D11DeviceContext* pDevCtx)
{
  if (pDevCtx == nullptr || dev_idx == UINT_MAX)
  {
    const SK_RenderBackend& rb =
      SK_GetCurrentRenderBackend ();

    pDevCtx =
                                       rb.d3d11.immediate_ctx;
    dev_idx =
      SK_D3D11_GetDeviceContextHandle (rb.d3d11.immediate_ctx);
  }

  if (SK_D3D11_IsDevCtxDeferred (pDevCtx))
  {
    return false;
  }


  if ( dev_idx >= SK_D3D11_MAX_DEV_CONTEXTS )
  {
    return false;
  }


  return
    SK_ImGui_DrawState [dev_idx].drawing != FALSE;
}

std::pair <BOOL*, BOOL>
SK_ImGui_FlagDrawing_OnD3D11Ctx (UINT dev_idx)
{
  const SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  if ( dev_idx == UINT_MAX )
  {
       dev_idx =
    SK_D3D11_GetDeviceContextHandle (rb.d3d11.immediate_ctx);
  }

  if ( dev_idx >= SK_D3D11_MAX_DEV_CONTEXTS )
  {
    static BOOL _FALSE;
          _FALSE = FALSE;

    return
      std::make_pair (&_FALSE, FALSE);
  }

  BOOL* pRet =
    &(SK_ImGui_DrawState [dev_idx].drawing);

  return
    std::make_pair (pRet, TRUE);
}

bool
SK_D3D11_ShouldTrackSetShaderResources ( ID3D11DeviceContext* pDevCtx,
                                         UINT                 dev_idx )
{
  if (pDevCtx == nullptr)
    return false;

  if (! SK::ControlPanel::D3D11::show_shader_mod_dlg && ReadAcquire (&SK_D3D11_TrackingCount->TextureBased) == 0)
    return false;

  const bool                                   bIsDeferred = SK_D3D11_IsDevCtxDeferred (pDevCtx);
  if (SK_D3D11_IgnoreWrappedOrDeferred (false, bIsDeferred, pDevCtx))
  {
    return false;
  }

  if (bIsDeferred)
    return true;


  if (dev_idx == UINT_MAX)
  {
    dev_idx =
      SK_D3D11_GetDeviceContextHandle (pDevCtx);
  }

  if ( SK_ImGui_IsDrawing_OnD3D11Ctx (dev_idx, pDevCtx) )
  {
    return false;
  }

  return true;
}

bool
SK_D3D11_ShouldTrackMMIO ( ID3D11DeviceContext* pDevCtx,
                           SK_TLS**             ppTLS )
{
  UNREFERENCED_PARAMETER (pDevCtx); UNREFERENCED_PARAMETER (ppTLS);

  return
    (! SK::ControlPanel::D3D11::show_shader_mod_dlg);
}


HMODULE hModReShade = (HMODULE)-2;


bool& SK_D3D11_DontTrackUnlessModToolsAreOpen = config.render.dxgi.low_spec_mode;

bool SK_D3D11_IsTrackingRequired (void)
{
  static auto& _shaders =
    SK_D3D11_Shaders.get();

  return
    ReadAcquire (&SK_D3D11_DrawTrackingReqs)    > 0 ||
    ReadAcquire (&SK_D3D11_CBufferTrackingReqs) > 0 ||
    (_shaders.hasReShadeTriggers () && (! _shaders.reshade_triggered));
}

bool
SK_D3D11_ShouldTrackRenderOp ( ID3D11DeviceContext* pDevCtx,
                               UINT                 dev_idx )
{
  if (pDevCtx == nullptr)
    return false;


  const SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  // Don't let D3D11On12 confuse things
  if (rb.api == SK_RenderAPI::D3D11 && (! rb.in_present_call))
  {
    const auto frame_id =
      SK_GetFramesDrawn ();

    static ULONG64
        last_frame = 0;
    if (std::exchange (last_frame, frame_id) < frame_id)
    {
      if (InterlockedExchange (&SK_Reflex_LastFrameMarked, frame_id) < frame_id)
      {
        rb.setLatencyMarkerNV (SIMULATION_END);
        rb.setLatencyMarkerNV (RENDERSUBMIT_START);
      }
    }
  }

  if (SK_D3D11_DontTrackUnlessModToolsAreOpen && (! SK_D3D11_EnableTracking) && (! SK_D3D11_IsTrackingRequired ()))
    return false;

  if ( rb.d3d11.immediate_ctx == nullptr ||
       rb.device              == nullptr ||
       rb.swapchain           == nullptr )
  {
    //SK_ReleaseAssert (! "ShouldTrackRenderOp: RenderBackend State");

    return false;
  }

  bool deferred =
    SK_D3D11_IsDevCtxDeferred (pDevCtx);

  if ((! config.render.dxgi.deferred_isolation) && deferred)
    return false;

  if (deferred)
    return true;


  if (dev_idx != UINT_MAX)
  {
    return
      (SK_ImGui_DrawState [dev_idx].drawing == FALSE);
  }

  return true;
}


// Only accessed by the swapchain thread and only to clear any outstanding
//   references prior to a buffer resize
SK_LazyGlobal <std::vector <SK_ComPtr <ID3D11View>> >                              SK_D3D11_TempResources;
SK_LazyGlobal <std::array <SK_D3D11_KnownTargets, SK_D3D11_MAX_DEV_CONTEXTS + 1> > SK_D3D11_RenderTargets;

void
SK_D3D11_KnownThreads::clear_all (void)
{
  if (use_lock)
  {
    std::scoped_lock <SK_Thread_HybridSpinlock>
          scope_lock (*_lock);

    ids.clear ();

    return;
  }

  ids.clear ();
}

size_t
SK_D3D11_KnownThreads::count_all (void)
{
  if (use_lock)
  {
    std::scoped_lock <SK_Thread_HybridSpinlock>
          scope_lock (*_lock);

    return ids.size ();
  }

  return ids.size ();
}

void
SK_D3D11_KnownThreads::mark (void)
{
  //#ifndef _DEBUG
#if 1
  return;
#else
  if (! SK_D3D11_EnableTracking)
    return;

  if (use_lock)
  {
      std::scoped_lock <SK_Thread_HybridSpinlock>
            scope_lock (*_lock);

    ids.emplace    (SK_Thread_GetCurrentId ());
    active.emplace (SK_Thread_GetCurrentId ());

    return;
  }

  ids.emplace    (SK_Thread_GetCurrentId ());
  active.emplace (SK_Thread_GetCurrentId ());
#endif
}

// This is not a smart ptr., it may point to something, but we're not
//   holding any references. The pointer is used only for comparison.
ID3D11Texture2D *SK_D3D11_TrackedTexture       = nullptr;
DWORD            tracked_tex_blink_duration    = 666UL;
DWORD            tracked_shader_blink_duration = 666UL;

struct SK_DisjointTimerQueryD3D11 d3d11_shader_tracking_s::disjoint_query;



void
d3d11_shader_tracking_s::activate ( ID3D11DeviceContext        *pDevContext,
                                    ID3D11ClassInstance *const *ppClassInstances,
                                    UINT                        NumClassInstances,
                                    UINT                        dev_idx )
{
  if (pDevContext == nullptr) return;

  for ( UINT i = 0 ; i < NumClassInstances ; i++ )
  {
    if (ppClassInstances && ppClassInstances [i])
        addClassInstance   (ppClassInstances [i]);
  }

  if (dev_idx == UINT_MAX)
  {
    dev_idx =
      SK_D3D11_GetDeviceContextHandle (pDevContext);
  }

  const bool is_active =
    active.get (dev_idx);

  if (! is_active)
  {
    auto& shaders =
      SK_D3D11_Shaders;

    active.set (dev_idx, true);

    switch (type_)
    {
      case SK_D3D11_ShaderType::Vertex:
        shaders->vertex.current.shader   [dev_idx] = crc32c.load ();
        break;
      case SK_D3D11_ShaderType::Pixel:
        shaders->pixel.current.shader    [dev_idx] = crc32c.load ();
        break;
      case SK_D3D11_ShaderType::Geometry:
        shaders->geometry.current.shader [dev_idx] = crc32c.load ();
        break;
      case SK_D3D11_ShaderType::Domain:
        shaders->domain.current.shader   [dev_idx] = crc32c.load ();
        break;
      case SK_D3D11_ShaderType::Hull:
        shaders->hull.current.shader     [dev_idx] = crc32c.load ();
        break;
      case SK_D3D11_ShaderType::Compute:
        shaders->compute.current.shader  [dev_idx] = crc32c.load ();
        break;
    }
  }

  else
    return;


  // Timing is very difficult on deferred contexts; will finish later (years?)
  if ( SK_D3D11_IsDevCtxDeferred (pDevContext) )
    return;


  const SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  auto pDev =
    rb.getDevice <ID3D11Device> ();

  if (! pDev)
    return;

  if ( nullptr ==
         disjoint_query.async
           && timers.empty () )
  {
    D3D11_QUERY_DESC query_desc {
      D3D11_QUERY_TIMESTAMP_DISJOINT, 0x00
    };

    SK_ComPtr <ID3D11Query>                         pQuery;
    if (SUCCEEDED (pDev->CreateQuery (&query_desc, &pQuery.p)))
    {
      SK_ComPtr <ID3D11DeviceContext> pImmediateContext;
      pDev->GetImmediateContext     (&pImmediateContext.p);

      disjoint_query.async  =   pQuery;
      pImmediateContext->Begin (pQuery);

      disjoint_query.active = true;
    }
  }

  if (disjoint_query.active)
  {
    // Start a new query
    D3D11_QUERY_DESC query_desc {
      D3D11_QUERY_TIMESTAMP, 0x00
    };

    duration_s duration;

    SK_ComPtr <ID3D11Query>                         pQuery;
    if (SUCCEEDED (pDev->CreateQuery (&query_desc, &pQuery)))
    {
      duration.start.dev_ctx = pDevContext;
      duration.start.async   = pQuery;
      pDevContext->End (       pQuery);

      timers.emplace_back (duration);
    }
  }
}

void
d3d11_shader_tracking_s::deactivate (ID3D11DeviceContext* pDevCtx, UINT dev_idx)
{
  const SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  if (dev_idx == UINT_MAX)
  {
    dev_idx =
      SK_D3D11_GetDeviceContextHandle (pDevCtx);
  }

  const bool is_active =
    active.get (dev_idx);

  if (is_active)
  {
    auto& shaders =
      SK_D3D11_Shaders;

    active.set (dev_idx, false);

    bool end_of_frame = false;

    if (pDevCtx == nullptr)
    {
      end_of_frame = true;
      pDevCtx      = rb.d3d11.immediate_ctx;
    }

    switch (type_)
    {
      case SK_D3D11_ShaderType::Vertex:
        shaders->vertex.current.shader   [dev_idx] = 0x0;
        break;
      case SK_D3D11_ShaderType::Pixel:
        shaders->pixel.current.shader    [dev_idx] = 0x0;
        break;
      case SK_D3D11_ShaderType::Geometry:
        shaders->geometry.current.shader [dev_idx] = 0x0;
        break;
      case SK_D3D11_ShaderType::Domain:
        shaders->domain.current.shader   [dev_idx] = 0x0;
        break;
      case SK_D3D11_ShaderType::Hull:
        shaders->hull.current.shader     [dev_idx] = 0x0;
        break;
      case SK_D3D11_ShaderType::Compute:
        shaders->compute.current.shader  [dev_idx] = 0x0;
        break;
    }

    if (end_of_frame) return;
  }

  else
    return;


  if (pDevCtx == nullptr)
      pDevCtx  = rb.d3d11.immediate_ctx;


  // Timing is very difficult on deferred contexts; will finish later (years?)
  if ( pDevCtx != nullptr && SK_D3D11_IsDevCtxDeferred (pDevCtx) )
    return;

  auto dev =
    rb.getDevice <ID3D11Device> ();

  if ( dev     != nullptr &&
       pDevCtx != nullptr && disjoint_query.active )
  {
    D3D11_QUERY_DESC query_desc {
      D3D11_QUERY_TIMESTAMP, 0x00
    };

    duration_s& duration =
      timers.back ();

    SK_ComPtr <ID3D11Query>                          pQuery;
    if ( SUCCEEDED ( dev->CreateQuery (&query_desc, &pQuery.p ) ) )
    {
      duration.end.dev_ctx = pDevCtx;
      duration.end.async   = pQuery;
      pDevCtx->End (         pQuery);
    }
  }
}

void
d3d11_shader_tracking_s::use (ID3D11DeviceContext* pDevCtx)
{
  if (pDevCtx != nullptr && first_rtv.Format == DXGI_FORMAT_UNKNOWN)
  {
    SK_ComPtr <ID3D11RenderTargetView> pRTV;
    pDevCtx->OMGetRenderTargets (  1, &pRTV, nullptr  );
  
    if (pRTV != nullptr)
    {
      SK_ComPtr <ID3D11Resource>
                          pResource;
      pRTV->GetResource (&pResource.p);
  
      SK_ComQIPtr <ID3D11Texture2D>
                        pTexture2D (pResource);
  
      if (pTexture2D != nullptr)
      {
        D3D11_TEXTURE2D_DESC  texDesc = { };
        pTexture2D->GetDesc (&texDesc);
  
        first_rtv.Format = texDesc.Format;
        first_rtv.Width  = texDesc.Width;
        first_rtv.Height = texDesc.Height;
      }
    }
  }

  num_draws++;
}

void
d3d11_shader_tracking_s::use_cmdlist (ID3D11DeviceContext* pDevCtx)
{
  if (pDevCtx != nullptr && first_rtv.Format == DXGI_FORMAT_UNKNOWN)
  {
    SK_ComPtr <ID3D11RenderTargetView> pRTV;
    pDevCtx->OMGetRenderTargets (  1, &pRTV, nullptr  );
  
    if (pRTV != nullptr)
    {
      SK_ComPtr <ID3D11Resource>
                          pResource;
      pRTV->GetResource (&pResource.p);
  
      SK_ComQIPtr <ID3D11Texture2D>
                        pTexture2D (pResource);
  
      if (pTexture2D != nullptr)
      {
        D3D11_TEXTURE2D_DESC  texDesc = { };
        pTexture2D->GetDesc (&texDesc);
  
        first_rtv.Format = texDesc.Format;
        first_rtv.Width  = texDesc.Width;
        first_rtv.Height = texDesc.Height;
      }
    }
  }

  num_deferred_draws++;
}

bool
SK_D3D11_ShouldTrackComputeDispatch ( ID3D11DeviceContext* pDevCtx,
                               const  SK_D3D11DispatchType dispatch_type,
                                      UINT                 dev_idx )
{
  UNREFERENCED_PARAMETER (dispatch_type);

  const bool
    process =      // No separation of Draw from Dispatch for now
      ( ReadAcquire (&SK_D3D11_DrawTrackingReqs) > 0 )
                ||
        SK_D3D11_ShouldTrackRenderOp (pDevCtx, dev_idx);

  return
    process;
}

bool
SK_D3D11_ShouldTrackDrawCall ( ID3D11DeviceContext* pDevCtx,
                         const SK_D3D11DrawType     draw_type,
                               UINT                 dev_idx )
{
  // If ReShade (custom version) is loaded, state tracking is non-optional
  if ( (intptr_t)hModReShade < (intptr_t)nullptr )
                 hModReShade = SK_ReShade_GetDLL ();

  bool process = false;

  auto reshadable =
   [&](void) ->
  bool
  {
    return ( draw_type == SK_D3D11DrawType::PrimList ||
             draw_type == SK_D3D11DrawType::Indexed );
  };

  if ( config.reshade.is_addon &&
   SK_D3D11_HasReShadeTriggers &&
                 reshadable () &&
       (! SK_D3D11_Shaders->reshade_triggered) )
  {
    process = true;
  }

  else
  {
    process =
     ( ReadAcquire (&SK_D3D11_DrawTrackingReqs) > 0 )
               ||
       SK_D3D11_ShouldTrackRenderOp (pDevCtx, dev_idx)
               ||
      SK_Screenshot_IsCapturingHUDless ();
  }

  return
    process;
}

SK_LazyGlobal <SK_D3D11_KnownThreads> SK_D3D11_MemoryThreads;
SK_LazyGlobal <SK_D3D11_KnownThreads> SK_D3D11_DrawThreads;
SK_LazyGlobal <SK_D3D11_KnownThreads> SK_D3D11_DispatchThreads;
SK_LazyGlobal <SK_D3D11_KnownThreads> SK_D3D11_ShaderThreads;





bool SKX_D3D11_IsVtxShaderLoaded (ID3D11Device* pDevice, uint32_t crc32c)
{
  std::scoped_lock <SK_Thread_HybridSpinlock> _lock (
    *cs_shader_vs.get ()
  );

  bool bRet =
    SK_D3D11_Shaders->vertex.descs [pDevice].count (crc32c) != 0;

  return bRet;
}

bool SKX_D3D11_IsPixShaderLoaded (ID3D11Device* pDevice, uint32_t crc32c)
{
  std::scoped_lock <SK_Thread_HybridSpinlock> _lock(
    *cs_shader_ps.get()
  );

  bool bRet =
    SK_D3D11_Shaders->pixel.descs [pDevice].count (crc32c) != 0;

  return bRet;
}