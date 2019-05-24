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

#define __SK_SUBSYSTEM__ L"  D3D 11  "

#include <SpecialK/control_panel/d3d11.h>
#include <SpecialK/render/d3d11/d3d11_core.h>
#include <SpecialK/render/d3d11/d3d11_state_tracker.h>

// Only valid for the thread driving an immediate context.
class SK_ImGui_DrawCtx
{
public:
  BOOL drawing = FALSE;
};

SK_LazyGlobal <std::array <SK_ImGui_DrawCtx, SK_D3D11_MAX_DEV_CONTEXTS + 1> > SK_ImGui_DrawState;


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
SK_ImGui_IsDrawing_OnD3D11Ctx (size_t dev_idx)
{
  if ((SSIZE_T)dev_idx == -1)
  {   dev_idx =
        SK_D3D11_GetDeviceContextHandle (
          SK_GetCurrentRenderBackend ().d3d11.immediate_ctx
        );
  }

  if ( (SSIZE_T)dev_idx < 0 ||
                dev_idx >= SK_D3D11_MAX_DEV_CONTEXTS )
  {
    return false;
  }

  return
    SK_ImGui_DrawState.get ()[dev_idx].drawing;
}

std::pair <BOOL*, BOOL>
SK_ImGui_FlagDrawing_OnD3D11Ctx (size_t dev_idx)
{
  static auto& rb =
    SK_GetCurrentRenderBackend ();

  try
  {
    if ( dev_idx == -1 )
    {
         dev_idx =
      SK_D3D11_GetDeviceContextHandle (rb.d3d11.immediate_ctx);
    }

    if ( (SSIZE_T)dev_idx < 0 ||
                  dev_idx >= SK_D3D11_MAX_DEV_CONTEXTS )
    {
      static BOOL _FALSE;
            _FALSE = FALSE;

      return
        std::make_pair (&_FALSE, FALSE);
    }

    BOOL* pRet =
      &(SK_ImGui_DrawState->at (dev_idx).drawing);

    return
      std::make_pair (pRet, TRUE);
  }

  catch (const std::out_of_range&)
  {
    static BOOL _FALSE;
                _FALSE = FALSE;

    return
      std::make_pair (&_FALSE, FALSE);
  }
}

bool
SK_D3D11_ShouldTrackSetShaderResources ( ID3D11DeviceContext* pDevCtx,
                                         UINT                 dev_idx )
{
  if (! SK::ControlPanel::D3D11::show_shader_mod_dlg)
    return false;


  if (dev_idx == (UINT)-1)
  {
    dev_idx =
      SK_D3D11_GetDeviceContextHandle (pDevCtx);
  }

  if ( SK_ImGui_IsDrawing_OnD3D11Ctx (dev_idx) )
  {
    return false;
  }


  ///SK_TLS *pTLS = nullptr;
  ///
  ///if (ppTLS != nullptr)
  ///{
  ///  if (*ppTLS != nullptr)
  ///  {
  ///    pTLS = *ppTLS;
  ///  }
  ///
  ///  else
  ///  {
  ///      pTLS = SK_TLS_Bottom ();
  ///    *ppTLS = pTLS;
  ///  }
  ///}
  ///
  ///else
  ///  pTLS = SK_TLS_Bottom ();
  ///
  ///if (pTLS->imgui->drawing)
  ///  return false;

  return true;
}

bool
SK_D3D11_ShouldTrackMMIO ( ID3D11DeviceContext* pDevCtx,
                           SK_TLS**             ppTLS )
{
  UNREFERENCED_PARAMETER (pDevCtx); UNREFERENCED_PARAMETER (ppTLS);

  if (! SK::ControlPanel::D3D11::show_shader_mod_dlg)
    return false;

  return true;
}


HMODULE hModReShade = (HMODULE)-2;


extern bool SK_D3D11_DontTrackUnlessModToolsAreOpen;

bool
SK_D3D11_ShouldTrackRenderOp ( ID3D11DeviceContext* pDevCtx,
                               UINT                 dev_idx )
{
  if (pDevCtx == nullptr)
    return false;

  if (SK_D3D11_DontTrackUnlessModToolsAreOpen && (! SK_D3D11_EnableTracking))
    return false;

  if ((! config.render.dxgi.deferred_isolation) && SK_D3D11_IsDevCtxDeferred (pDevCtx))
    return false;

  static const SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  if ( rb.d3d11.immediate_ctx == nullptr ||
       rb.device              == nullptr ||
       rb.swapchain           == nullptr )
  {
    //SK_ReleaseAssert (! "ShouldTrackRenderOp: RenderBackend State");

    return false;
  }


  // -- Fast Path Cache --
  static ID3D11DeviceContext* last_ctx   = rb.d3d11.immediate_ctx;
  static UINT                 static_idx =
    SK_D3D11_GetDeviceContextHandle (rb.d3d11.immediate_ctx);

  if (last_ctx != rb.d3d11.immediate_ctx)
  {
    last_ctx   = rb.d3d11.immediate_ctx;
    static_idx = SK_D3D11_GetDeviceContextHandle (rb.d3d11.immediate_ctx);
  }
  // -- End Fast Path Maintenance --


  auto& draw_base =
    SK_ImGui_DrawState.get ();

  if (rb.d3d11.immediate_ctx == pDevCtx)
  {
    if (draw_base [static_idx].drawing)
      return false;

    return true;
  }

  else if (dev_idx == (UINT)-1)
  {
    if (draw_base [SK_D3D11_GetDeviceContextHandle (pDevCtx)].drawing)
      return false;
  }

  else //(dev_idx != (UINT)-1)
  {
    if (draw_base [dev_idx].drawing)
      return false;
  }

  return true;
}


// Only accessed by the swapchain thread and only to clear any outstanding
//   references prior to a buffer resize
SK_LazyGlobal <std::vector <SK_ComPtr <IUnknown> > >                               SK_D3D11_TempResources;
SK_LazyGlobal <std::array <SK_D3D11_KnownTargets, SK_D3D11_MAX_DEV_CONTEXTS + 1> > SK_D3D11_RenderTargets;

void
SK_D3D11_KnownThreads::clear_all (void)
{
  if (use_lock)
  {
    SK_AutoCriticalSection auto_cs (&cs);
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
    SK_AutoCriticalSection auto_cs (&cs);
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
    SK_AutoCriticalSection auto_cs (&cs);
    ids.emplace    (SK_Thread_GetCurrentId ());
    active.emplace (SK_Thread_GetCurrentId ());
    return;
  }

  ids.emplace    (SK_Thread_GetCurrentId ());
  active.emplace (SK_Thread_GetCurrentId ());
#endif
}

extern SK_LazyGlobal <memory_tracking_s> mem_map_stats;
extern SK_LazyGlobal <target_tracking_s> tracked_rtv;

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
  if (! pDevContext) return;

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
    static auto& shaders =
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


  static SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  SK_ComQIPtr <ID3D11Device> pDev (rb.device);

  if (! pDev)
    return;

  if ( nullptr ==
      ReadPointerAcquire ((void **)&disjoint_query.async)
      && timers.empty () )
  {
    D3D11_QUERY_DESC query_desc {
      D3D11_QUERY_TIMESTAMP_DISJOINT, 0x00
    };

    ID3D11Query                                    *pQuery = nullptr;
    if (SUCCEEDED (pDev->CreateQuery (&query_desc, &pQuery)))
    {
      SK_ComPtr <ID3D11DeviceContext>  pImmediateContext;
      pDev->GetImmediateContext      (&pImmediateContext);

      InterlockedExchangePointer ((void **)&disjoint_query.async, pQuery);
      pImmediateContext->Begin                                   (pQuery);
      InterlockedExchange (&disjoint_query.active, TRUE);
    }
  }

  if (ReadAcquire (&disjoint_query.active))
  {
    // Start a new query
    D3D11_QUERY_DESC query_desc {
      D3D11_QUERY_TIMESTAMP, 0x00
    };

    duration_s duration;

    ID3D11Query                                    *pQuery = nullptr;
    if (SUCCEEDED (pDev->CreateQuery (&query_desc, &pQuery)))
    {
      InterlockedExchangePointer ((void **)&duration.start.dev_ctx, pDevContext);
      InterlockedExchangePointer ((void **)&duration.start.async,   pQuery);
      pDevContext->End                                             (pQuery);
      timers.emplace_back (duration);
    }
  }
}

void
d3d11_shader_tracking_s::deactivate (ID3D11DeviceContext* pDevCtx, UINT dev_idx)
{
  static SK_RenderBackend& rb =
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
    static auto& shaders =
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


  SK_ComQIPtr <ID3D11Device> dev (rb.device);

  if ( dev     != nullptr &&
       pDevCtx != nullptr && ReadAcquire (&disjoint_query.active) )
  {
    D3D11_QUERY_DESC query_desc {
      D3D11_QUERY_TIMESTAMP, 0x00
    };

    duration_s& duration =
      timers.back ();

    ID3D11Query* pQuery = nullptr;
    if ( SUCCEEDED ( dev->CreateQuery (&query_desc, &pQuery ) ) )
    { InterlockedExchangePointer (
      (PVOID *) (&duration.end.dev_ctx), pDevCtx
    );
    InterlockedExchangePointer (
      (PVOID *) (&duration.end.async),   pQuery
    );                     pDevCtx->End (pQuery);
    }
  }
}

void
d3d11_shader_tracking_s::use (IUnknown* pShader)
{
  UNREFERENCED_PARAMETER (pShader);

  num_draws++;
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
  //// Engine is way too parallel with its 8 queues, don't even try to
  ////   track the deferred command stream!
  //if ( SK_D3D11_IsDevCtxDeferred (pDevCtx) &&
  //     SK_GetCurrentGameID () == SK_GAME_ID::AssassinsCreed_Odyssey )
  //{
  //  return false;
  //}

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

  if ( SK_ReShade_DrawCallback.fn != nullptr &&
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
       SK_D3D11_ShouldTrackRenderOp (pDevCtx, dev_idx);
  }

  return
    process;
}

SK_LazyGlobal <SK_D3D11_KnownThreads> SK_D3D11_MemoryThreads;
SK_LazyGlobal <SK_D3D11_KnownThreads> SK_D3D11_DrawThreads;
SK_LazyGlobal <SK_D3D11_KnownThreads> SK_D3D11_DispatchThreads;
SK_LazyGlobal <SK_D3D11_KnownThreads> SK_D3D11_ShaderThreads;





bool SKX_D3D11_IsVtxShaderLoaded (uint32_t crc32c)
{
  auto crit_sec =
    cs_shader_vs.get ();

  crit_sec->lock ();

  bool bRet =
    SK_D3D11_Shaders->vertex.descs.count (crc32c) != 0;

  crit_sec->unlock ();

  return bRet;
}

bool SKX_D3D11_IsPixShaderLoaded (uint32_t crc32c)
{
  auto crit_sec =
    cs_shader_ps.get ();

  crit_sec->lock ();

  bool bRet =
    SK_D3D11_Shaders->pixel.descs.count (crc32c) != 0;

  crit_sec->unlock ();

  return bRet;
}