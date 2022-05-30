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

#define _L2(w)  L ## w
#define  _L(w) _L2(w)

#include <SpecialK/render/dxgi/dxgi_util.h>
#include <SpecialK/render/d3d11/d3d11_shader.h>
#include <SpecialK/render/d3d11/d3d11_tex_mgr.h>
#include <SpecialK/render/d3d11/d3d11_screenshot.h>
#include <SpecialK/render/d3d11/d3d11_state_tracker.h>
#include <SpecialK/render/d3d11/utility/d3d11_texture.h>
#include <SpecialK/render/dxgi/dxgi_hdr.h>

#include <SpecialK/control_panel/d3d11.h>

#include <execution>

#define SK_D3D11_WRAP_IMMEDIATE_CTX
#define SK_D3D11_WRAP_DEFERRED_CTX

// NEVER, under any circumstances, call any functions using this!
ID3D11Device* g_pD3D11Dev = nullptr;

static constexpr auto _MAX_UAVS     = 8;
UINT            start_uav           = 0;
UINT                  uav_count     = _MAX_UAVS;
float                 uav_clear [4] =
    { 0.0f, 0.0f, 0.0f, 0.0f };

// For effects that blink; updated once per-frame.
//DWORD dwFrameTime = SK::ControlPanel::current_time;
// For effects that blink; updated once per-frame.
DWORD& dwFrameTime = SK::ControlPanel::current_time;

DWORD D3D11_GetFrameTime (void)
{
  return dwFrameTime;
}

void
SK_D3D11_SetDebugName (       ID3D11DeviceChild* pDevChild,
                        const std::wstring&      kName )
{
  if (! pDevChild)
    return;

  static int
      __SK_D3D11_DebugLayerActive  = -1;
  if (__SK_D3D11_DebugLayerActive == -1)
  {
    SK_ComPtr <ID3D11Device> pDev;
    pDevChild->GetDevice (  &pDev.p );

    SK_ComQIPtr <ID3D11Debug> pDebug (pDev);
    __SK_D3D11_DebugLayerActive =
                              pDebug.p != nullptr ?
                                                1 : 0;
  }

  // Store object names only if we are logging stuff or if the
  //   D3D11 debug layer is active
  if (kName.empty () /*|| ( __SK_D3D11_DebugLayerActive == 0 &&
                                 config.system.log_level < 2 )*/)
  { // Ignore the comment above,
    return; // debug names are useful in SK's built-in debugger :)
  }

  SK_LOGi2 (
    L"Created D3D11 Object: %ws", kName.c_str ()
  );

  D3D_SET_OBJECT_NAME_N_W ( pDevChild,
                   static_cast <UINT> ( kName.size () ),
                                        kName.data ()
                          );

  std::string utf8_copy =
    SK_WideCharToUTF8 (kName);

  D3D_SET_OBJECT_NAME_N_A ( pDevChild,
                 static_cast <UINT> ( (utf8_copy).size () ),
                                      (utf8_copy).data ()
                          );
}

bool
SK_D3D11_DrawCallFilter (int elem_cnt, int vtx_cnt, uint32_t vtx_shader);

extern HRESULT
  SK_D3D11_InjectSteamHDR ( _In_ ID3D11DeviceContext *pDevCtx,
                            _In_ UINT                 VertexCount,
                            _In_ UINT                 StartVertexLocation,
                            _In_ D3D11_Draw_pfn       pfnD3D11Draw );

extern HRESULT
  SK_D3D11_InjectGenericHDROverlay ( _In_ ID3D11DeviceContext *pDevCtx,
                                     _In_ UINT                 VertexCount,
                                     _In_ UINT                 StartVertexLocation,
                                     _In_ uint32_t             crc32,
                                     _In_ D3D11_Draw_pfn       pfnD3D11Draw );
extern HRESULT
  SK_D3D11_Inject_uPlayHDR ( _In_ ID3D11DeviceContext  *pDevCtx,
                             _In_ UINT                  IndexCount,
                             _In_ UINT                  StartIndexLocation,
                             _In_ INT                   BaseVertexLocation,
                             _In_ D3D11_DrawIndexed_pfn pfnD3D11DrawIndexed );

extern HRESULT
  SK_D3D11_Inject_EpicHDR ( _In_ ID3D11DeviceContext  *pDevCtx,
                            _In_ UINT                  IndexCount,
                            _In_ UINT                  StartIndexLocation,
                            _In_ INT                   BaseVertexLocation,
                            _In_ D3D11_DrawIndexed_pfn pfnD3D11DrawIndexed );

extern bool SK_D3D11_ShowShaderModDlg (void);

LPVOID pfnD3D11CreateDevice             = nullptr;
LPVOID pfnD3D11CreateDeviceAndSwapChain = nullptr;

HMODULE SK::DXGI::hModD3D11 = nullptr;

SK::DXGI::PipelineStatsD3D11 SK::DXGI::pipeline_stats_d3d11 = { };

volatile HANDLE hResampleThread = nullptr;

static SK_LazyGlobal <
Concurrency::concurrent_unordered_map
<                  ID3D11DeviceContext *,
  SK_ComPtr       <ID3D11DeviceContext4> > > wrapped_contexts;

static SK_LazyGlobal <
Concurrency::concurrent_unordered_map
<                  ID3D11Device         *,
  SK_ComPtr       <ID3D11DeviceContext4> > > wrapped_immediates;

#pragma data_seg (".SK_D3D11_Hooks")
extern "C"
{
  // Global DLL's cache
__declspec (dllexport) SK_HookCacheEntryGlobal (D3D11CreateDevice)
__declspec (dllexport) SK_HookCacheEntryGlobal (D3D11CreateDeviceAndSwapChain)
};
#pragma data_seg ()
#pragma comment  (linker, "/SECTION:.SK_D3D11_Hooks,RWS")

// Local DLL's cached addresses
SK_HookCacheEntryLocal ( D3D11CreateDevice,             L"d3d11.dll",
                         D3D11CreateDevice_Detour,
                         nullptr )
SK_HookCacheEntryLocal ( D3D11CreateDeviceAndSwapChain, L"d3d11.dll",
                         D3D11CreateDeviceAndSwapChain_Detour,
                         nullptr )

static sk_hook_cache_array global_d3d11_records =
  { &GlobalHook_D3D11CreateDevice,
    &GlobalHook_D3D11CreateDeviceAndSwapChain };

static sk_hook_cache_array local_d3d11_records =
  { &LocalHook_D3D11CreateDevice,
    &LocalHook_D3D11CreateDeviceAndSwapChain };


SK_LazyGlobal <SK_D3D11_KnownShaders> SK_D3D11_Shaders;
SK_LazyGlobal <target_tracking_s>     tracked_rtv;

volatile LONG __SKX_ComputeAntiStall = 1;

volatile LONG SK_D3D11_NumberOfSeenContexts = 0;
volatile LONG _mutex_init                   = 0;

void
SK_D3D11_InitMutexes (void)
{
  if (ReadAcquire (&_mutex_init) > 1)
    return;

  LocalHook_D3D11CreateDevice.trampoline             = static_cast_p2p <void> (&D3D11CreateDevice_Import);
  LocalHook_D3D11CreateDeviceAndSwapChain.trampoline = static_cast_p2p <void> (&D3D11CreateDeviceAndSwapChain_Import);

  if (0 == InterlockedCompareExchange (&_mutex_init, 1, 0))
  {
    cs_shader      = std::make_unique <SK_Thread_HybridSpinlock> (0x666);
    cs_shader_vs   = std::make_unique <SK_Thread_HybridSpinlock> (0x300);
    cs_shader_ps   = std::make_unique <SK_Thread_HybridSpinlock> (0x200);
    cs_shader_gs   = std::make_unique <SK_Thread_HybridSpinlock> (0x100);
    cs_shader_hs   = std::make_unique <SK_Thread_HybridSpinlock> (0x100);
    cs_shader_ds   = std::make_unique <SK_Thread_HybridSpinlock> (0x100);
    cs_shader_cs   = std::make_unique <SK_Thread_HybridSpinlock> (0x300);
    cs_mmio        = std::make_unique <SK_Thread_HybridSpinlock> (0xe0);
    cs_render_view = std::make_unique <SK_Thread_HybridSpinlock> (0xb0);

    InterlockedIncrement (&_mutex_init);
  }

  else
    SK_Thread_SpinUntilAtomicMin (&_mutex_init, 2);
}

void
SK_D3D11_CleanupMutexes (void)
{
  if ( ReadAcquire (&_mutex_init)      > 1 &&
       ReadAcquire (&__SK_DLL_Ending) != 0 )
  {
    cs_shader.reset      ();
    cs_shader_vs.reset   ();
    cs_shader_ps.reset   ();
    cs_shader_gs.reset   ();
    cs_shader_hs.reset   ();
    cs_shader_ds.reset   ();
    cs_shader_cs.reset   ();
    cs_mmio.reset        ();
    cs_render_view.reset ();
  }
}

extern std::pair <BOOL*, BOOL>
SK_ImGui_FlagDrawing_OnD3D11Ctx (UINT dev_idx);
extern bool
SK_ImGui_IsDrawing_OnD3D11Ctx   (UINT dev_idx, ID3D11DeviceContext* pDevCtx);

SK_LazyGlobal <
   std::array < SK_D3D11_ContextResources,
                SK_D3D11_MAX_DEV_CONTEXTS + 1 >
              > SK_D3D11_PerCtxResources;

SK_LazyGlobal        <
  std::unordered_set <ID3D11Texture2D *>
                     > used_textures;
SK_LazyGlobal        <
  std::unordered_map < ID3D11DeviceContext *,
                       mapped_resources_s  >
                     > mapped_resources;

void
SK_D3D11_MergeCommandLists ( ID3D11DeviceContext *pSurrogate,
                             ID3D11DeviceContext *pMerge )
{
  SK_LOG_FIRST_CALL

  static auto& shaders =
    SK_D3D11_Shaders;

  auto _GetRegistry =
  [&]( SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>** ppShaderDomain,
       sk_shader_class                                    type )
  {
    switch (type)
    {
      default:
      case sk_shader_class::Vertex:
        *ppShaderDomain =
          reinterpret_cast <
            SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*
          >             (&shaders->vertex);
         break;
      case sk_shader_class::Pixel:
        *ppShaderDomain =
          reinterpret_cast <
            SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*
          >             (&shaders->pixel);
         break;
      case sk_shader_class::Geometry:
        *ppShaderDomain =
          reinterpret_cast <
            SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*
          >             (&shaders->geometry);
         break;
      case sk_shader_class::Domain:
        *ppShaderDomain =
          reinterpret_cast <
            SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*
          >             (&shaders->domain);
         break;
      case sk_shader_class::Hull:
        *ppShaderDomain =
          reinterpret_cast <
            SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*
          >             (&shaders->hull);
         break;
      case sk_shader_class::Compute:
        *ppShaderDomain =
          reinterpret_cast <
            SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*
          >             (&shaders->compute);
         break;
    }

    return *ppShaderDomain;
  };

  using _ShaderRepo =
    SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*;

  _ShaderRepo pShaderRepoIn  = nullptr;
  _ShaderRepo pShaderRepoOut = nullptr;

  const UINT dev_ctx_in =
    SK_D3D11_GetDeviceContextHandle (pSurrogate);
  const UINT dev_ctx_out =
    SK_D3D11_GetDeviceContextHandle (pMerge);

  if (dev_ctx_in  > SK_D3D11_MAX_DEV_CONTEXTS  ||
      dev_ctx_out > SK_D3D11_MAX_DEV_CONTEXTS)
  {
    SK_ReleaseAssert (!"Device Context Out of Range");
    return;
  }

  auto& ctx_in_res =
    SK_D3D11_PerCtxResources [dev_ctx_in];
  auto& ctx_out_res =
    SK_D3D11_PerCtxResources [dev_ctx_out];

  ctx_out_res.used_textures.insert  ( ctx_in_res.used_textures.begin  (),
                                      ctx_in_res.used_textures.end    () );
  ctx_out_res.temp_resources.insert ( ctx_in_res.temp_resources.begin (),
                                      ctx_in_res.temp_resources.end   () );

  //dll_log->Log ( L"Ctx: %lu -> Ctx: %lu :: %lu Used Textures, %lu Temp Resources",
  //                 dev_ctx_in, dev_ctx_out, SK_D3D11_PerCtxResources [dev_ctx_in].used_textures.size (),
  //                                          SK_D3D11_PerCtxResources [dev_ctx_in].temp_resources.size () );

  ctx_in_res.used_textures.clear  ();
  ctx_in_res.temp_resources.clear ();

  bool reset = true;// false;

  static const sk_shader_class classes [] = {
    sk_shader_class::Vertex,   sk_shader_class::Pixel,
    sk_shader_class::Geometry, sk_shader_class::Hull,
    sk_shader_class::Domain,   sk_shader_class::Compute
  };

  for ( auto i : classes )
  {

    _GetRegistry ( &pShaderRepoIn,  i );
    _GetRegistry ( &pShaderRepoOut, i )->current.shader [dev_ctx_out] =
                    pShaderRepoIn->current.shader       [dev_ctx_in ];

    if (reset)
    {
      RtlSecureZeroMemory
               (    pShaderRepoIn->current.views        [dev_ctx_in ],
                      128 * sizeof (ptrdiff_t) );
                    pShaderRepoIn->current.shader       [dev_ctx_in ] = 0x0;
                    pShaderRepoIn->tracked.deactivate (pSurrogate, dev_ctx_in);
    }
  }

  for ( int i = 0; i < 6; ++i )
  {
    memcpy ( &d3d11_shader_stages [i][dev_ctx_out].skipped_bindings [0],
             &d3d11_shader_stages [i][ dev_ctx_in].skipped_bindings [0],
             sizeof (ID3D11ShaderResourceView *) * D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT );
    RtlSecureZeroMemory (
             &d3d11_shader_stages [i][ dev_ctx_in].skipped_bindings [0],
             sizeof (ID3D11ShaderResourceView *) * D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT );

    memcpy ( &d3d11_shader_stages [i][dev_ctx_out].real_bindings [0],
             &d3d11_shader_stages [i][ dev_ctx_in].real_bindings [0],
             sizeof (ID3D11ShaderResourceView *) * D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT );
    RtlSecureZeroMemory (
             &d3d11_shader_stages [i][ dev_ctx_in].real_bindings [0],
             sizeof (ID3D11ShaderResourceView *) * D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT );
  }
}

void
SK_D3D11_ResetContextState (ID3D11DeviceContext* pDevCtx, UINT dev_ctx)
{
  static auto& shaders =
    SK_D3D11_Shaders;

  auto _GetRegistry =
  [&]( SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>** ppShaderDomain,
       sk_shader_class                                    type )
  {
    switch (type)
    {
      default:
      case sk_shader_class::Vertex:
        *ppShaderDomain = reinterpret_cast <
          SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *>
                      (&shaders->vertex);
         break;
      case sk_shader_class::Pixel:
        *ppShaderDomain = reinterpret_cast <
          SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*
         >            (&shaders->pixel);
         break;
      case sk_shader_class::Geometry:
        *ppShaderDomain = reinterpret_cast <
          SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*
         >            (&shaders->geometry);
         break;
      case sk_shader_class::Domain:
        *ppShaderDomain = reinterpret_cast <
          SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*
        >             (&shaders->domain);
         break;
      case sk_shader_class::Hull:
        *ppShaderDomain = reinterpret_cast <
          SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*
        >             (&shaders->hull);
         break;
      case sk_shader_class::Compute:
        *ppShaderDomain = reinterpret_cast <
          SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*
        >             (&shaders->compute);
         break;
    }

    return
      *ppShaderDomain;
  };

  SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>* pShaderRepo = nullptr;

  if (dev_ctx == UINT_MAX)
  {
    dev_ctx =
      SK_D3D11_GetDeviceContextHandle (pDevCtx);
  }

  if (dev_ctx > SK_D3D11_MAX_DEV_CONTEXTS)
    return;

  static const sk_shader_class classes [] = {
    sk_shader_class::Vertex,   sk_shader_class::Pixel,
    sk_shader_class::Geometry, sk_shader_class::Hull,
    sk_shader_class::Domain,   sk_shader_class::Compute
  };

  for ( auto i : classes )
  {
    _GetRegistry  ( &pShaderRepo, i )->current.shader         [dev_ctx] = 0x0;
                     pShaderRepo->tracked.deactivate (pDevCtx, dev_ctx);
    RtlSecureZeroMemory
                  (  pShaderRepo->current.views               [dev_ctx],
                     128 * sizeof (ptrdiff_t) );
  }

  for ( UINT i = 0 ; i < 6 ; ++i )
  {
    auto& stage =
      d3d11_shader_stages [i][dev_ctx];


    UINT k = 1;

    // Optimization strategy based on contiguous arrays of binding slots
    //
    //  * Find the lowest NULL slot, this demarcates the size of the array.
    //
    for ( k = 1 ; k < D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT; k *= 2 )
    {
      // Divide the search space in 1/2, continue dividing if NULL
      if (stage.skipped_bindings [k - 1] == nullptr)
      {
        // Count downwards to find the highest non-NULL binding slot
        while (                         k    >= 1  &&
                stage.skipped_bindings [k--] != nullptr )
        {
          ;
        }

        break;
      }
    }

    // We now know the size, let's get busy!
    for ( UINT j = 0 ; j <= k; ++j )
    {
      if (stage.skipped_bindings [j] != nullptr)
      {   stage.skipped_bindings [j]->Release ();
          stage.skipped_bindings [j] = nullptr;
      }
    }

    RtlSecureZeroMemory (
      &stage.real_bindings [0],
        D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT * sizeof (ptrdiff_t)
    );
  }
}

// Only reset once even if we take two trips through wrappers / hooks.
SK_LazyGlobal <std::array <bool,                  SK_D3D11_MAX_DEV_CONTEXTS + 1>>
                                       __SK_D3D11_ContextResetQueue;
SK_LazyGlobal <std::array <ID3D11DeviceContext *, SK_D3D11_MAX_DEV_CONTEXTS + 1>>
                                       __SK_D3D11_ContextResetQueueDevices;

bool
SK_D3D11_QueueContextReset ( ID3D11DeviceContext* pDevCtx,
                                            UINT dev_ctx )
{
  // We can't do anything about this
  SK_ReleaseAssert (dev_ctx != UINT_MAX);

  if (dev_ctx > SK_D3D11_MAX_DEV_CONTEXTS)
    return false;

  bool ret =
    __SK_D3D11_ContextResetQueue [dev_ctx];

  __SK_D3D11_ContextResetQueue        [dev_ctx] = true;
  __SK_D3D11_ContextResetQueueDevices [dev_ctx] = pDevCtx;

  return ret;
}

bool
SK_D3D11_DispatchContextResetQueue (UINT dev_ctx)
{
  if (dev_ctx <= SK_D3D11_MAX_DEV_CONTEXTS)
  {
    if (__SK_D3D11_ContextResetQueue [dev_ctx])
    {   __SK_D3D11_ContextResetQueue [dev_ctx] = false;

      SK_D3D11_ResetContextState (__SK_D3D11_ContextResetQueueDevices [dev_ctx],
                                                                       dev_ctx);
      return true;
    }
  }

  return false;
}

int SK_D3D11_AllocatedDevContexts = 0;

BOOL
SK_D3D11_SetDeviceContextHandle ( ID3D11DeviceContext *pDevCtx,
                                  LONG                 handle )
{
  if ( handle >= SK_D3D11_AllocatedDevContexts )
                 SK_D3D11_AllocatedDevContexts = handle + 1;

  SK_ReleaseAssert (handle < SK_D3D11_MAX_DEV_CONTEXTS);

  constexpr UINT size =
                 sizeof (LONG);

  if ( FAILED (
         pDevCtx->SetPrivateData ( SKID_D3D11DeviceContextHandle,
                                     size, &handle )
       )
     )
  {
    return FALSE;
  }

  return TRUE;
}

LONG
SK_D3D11_GetDeviceContextHandle ( ID3D11DeviceContext *pDevCtx )
{
  if (pDevCtx == nullptr) return SK_D3D11_MAX_DEV_CONTEXTS;

  const LONG RESOLVE_MAX = 64;

  static std::pair <ID3D11DeviceContext*, LONG>
    last_resolve [RESOLVE_MAX];
  static volatile LONG
         resolve_idx = 0;

  const auto early_out =
    &last_resolve [std::min (RESOLVE_MAX, ReadAcquire (&resolve_idx))];

  if (early_out->first == pDevCtx)
    return early_out->second;

  auto _CacheResolution =
    [&](LONG idx, ID3D11DeviceContext* pCtx, LONG handle) ->
    void
    {
      auto new_pair ( std::make_pair (pCtx, handle) );
          std::swap ( last_resolve   [idx],
                      new_pair );

      InterlockedExchange (&resolve_idx, std::min (RESOLVE_MAX, idx));
    };


  UINT size   = sizeof (LONG);
  LONG handle = SK_D3D11_MAX_DEV_CONTEXTS;


  LONG idx =
    ReadAcquire (&resolve_idx) + 1;

  if (idx >= RESOLVE_MAX)
      idx  = 0;


  if ( SUCCEEDED (
         pDevCtx->GetPrivateData ( SKID_D3D11DeviceContextHandle,
                                     &size, &handle )
       )
     )
  {
    _CacheResolution (idx, pDevCtx, handle);

    return handle;
  }

  std::scoped_lock <SK_Thread_HybridSpinlock>
         auto_lock (*cs_shader);

  size   = sizeof (LONG);
  handle = ReadAcquire (&SK_D3D11_NumberOfSeenContexts);

  LONG new_handle =
           handle;

  if ( TRUE != SK_D3D11_SetDeviceContextHandle (pDevCtx, new_handle) )
  {
    new_handle = 0;
  }

  else
  {
    InterlockedIncrement (&SK_D3D11_NumberOfSeenContexts);
  }

  SK_ReleaseAssert (handle < SK_D3D11_MAX_DEV_CONTEXTS);

  _CacheResolution (idx, pDevCtx, handle);

  return handle;
}

void
SK_D3D11_CopyContextHandle ( ID3D11DeviceContext *pSrcCtx,
                             ID3D11DeviceContext *pDstCtx )
{
  LONG src_handle =
    SK_D3D11_GetDeviceContextHandle (pSrcCtx);

  if (src_handle >= 0)
    SK_D3D11_SetDeviceContextHandle (pDstCtx, src_handle);
}

uint32_t
__cdecl
SK_D3D11_ChecksumShaderBytecode ( _In_ const void   *pShaderBytecode,
                                  _In_       SIZE_T  BytecodeLength  )
{
  uint32_t ret = 0x0;

  auto orig_se =
    SK_SEH_ApplyTranslator (
      SK_FilteringStructuredExceptionTranslator (
        EXCEPTION_ACCESS_VIOLATION
      )
    );
  try
  {
    ret =
      crc32c (
        0x00, static_cast <const uint8_t *> (pShaderBytecode),
                                                    BytecodeLength
             );
  }

  catch (const SK_SEH_IgnoredException&)
  {
    SK_LOG0 ( (L" >> Threw out disassembled shader due to access violation"
               L" during hash."),
               L"   DXGI   ");
  }
  SK_SEH_RemoveTranslator (orig_se);

  return ret;
}

std::string
SK_D3D11_DescribeResource (ID3D11Resource* pRes)
{
  if (pRes == nullptr)
  {
    return "N/A";
  }

  D3D11_RESOURCE_DIMENSION rDim;
  pRes->GetType          (&rDim);

  switch (rDim)
  {
    case D3D11_RESOURCE_DIMENSION_BUFFER:
      return "Buffer";
      break;

    case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
    {
      D3D11_TEXTURE2D_DESC desc = { };

      SK_ComQIPtr <ID3D11Texture2D> pTex2D (pRes);

      if (pTex2D != nullptr)
          pTex2D->GetDesc (&desc);

      return (
        SK_FormatString ( "Tex2D: (%lux%lu): %hs { %hs/%hs }",
          desc.Width,
          desc.Height, SK_DXGI_FormatToStr (desc.Format)   . data (),
                    SK_D3D11_DescribeUsage (desc.Usage),
                SK_D3D11_DescribeBindFlags (desc.BindFlags).c_str () )
      );
    } break;

    default:
      return "Other";
  }
};

volatile LONG  __d3d11_ready    = FALSE;

void WaitForInitD3D11 (void)
{
  if (CreateDXGIFactory_Import == nullptr)
  {
    SK_RunOnce (SK_BootDXGI ());
  }

  // Load user-defined DLLs (Plug-In)
  SK_RunLHIfBitness ( 64, SK_LoadPlugIns64 (),
                          SK_LoadPlugIns32 () );

  if (SK_TLS_Bottom ()->render->d3d11->ctx_init_thread == TRUE)
    return;

  if (        0 == ReadAcquire (&__d3d11_ready))
    SK_Thread_SpinUntilFlagged (&__d3d11_ready);
}


struct reshade_coeffs_s {
  int indexed                    = 1;
  int draw                       = 1;
  int auto_draw                  = 0;
  int indexed_instanced          = 1;
  int indexed_instanced_indirect = 4096;
  int instanced                  = 1;
  int instanced_indirect         = 4096;
  int dispatch                   = 1;
  int dispatch_indirect          = 1;
} SK_D3D11_ReshadeDrawFactors;

SK_RESHADE_CALLBACK_DRAW         SK_ReShade_DrawCallback;
SK_RESHADE_CALLBACK_SetDSV       SK_ReShade_SetDepthStencilViewCallback;
SK_RESHADE_CALLBACK_GetDSV       SK_ReShade_GetDepthStencilViewCallback;
SK_RESHADE_CALLBACK_ClearDSV     SK_ReShade_ClearDepthStencilViewCallback;
SK_RESHADE_CALLBACK_CopyResource SK_ReShade_CopyResourceCallback;

uint32_t
__stdcall
SK_D3D11_TextureHashFromCache (ID3D11Texture2D* pTex);

struct d3d11_caps_t {
  struct {
    bool d3d11_1          = false;
  } feature_level;

  bool MapNoOverwriteOnDynamicConstantBuffer = false;
} d3d11_caps;

D3D11CreateDeviceAndSwapChain_pfn D3D11CreateDeviceAndSwapChain_Import = nullptr;
D3D11CreateDevice_pfn             D3D11CreateDevice_Import             = nullptr;

void
SK_D3D11_SetDevice ( ID3D11Device           **ppDevice,
                     D3D_FEATURE_LEVEL        FeatureLevel )
{
  static SK_RenderBackend_V2& rb =
    SK_GetCurrentRenderBackend ();

  if ( ppDevice != nullptr )
  {
    if ( *ppDevice != g_pD3D11Dev )
    {
      SK_LOG0 ( (L" >> Device = %08" _L(PRIxPTR) L"h (Feature Level:%hs)",
                      (uintptr_t)*ppDevice,
                        SK_DXGI_FeatureLevelsToStr ( 1,
                                                      (DWORD *)&FeatureLevel
                                                   ).c_str ()
                  ), __SK_SUBSYSTEM__ );

      // We ARE technically holding a reference, but we never make calls to this
      //   interface - it's just for tracking purposes.
      g_pD3D11Dev = *ppDevice;
    }

    if (config.render.dxgi.exception_mode != -1)
      (*ppDevice)->SetExceptionMode (config.render.dxgi.exception_mode);

    SK_ComQIPtr <IDXGIDevice>  pDXGIDev (ppDevice != nullptr ? *ppDevice : nullptr);
    SK_ComPtr   <IDXGIAdapter> pAdapter;

    if ( pDXGIDev != nullptr )
    {
      const HRESULT hr =
        pDXGIDev->GetParent ( IID_PPV_ARGS (&pAdapter.p) );

      if ( SUCCEEDED ( hr ) )
      {
        if ( pAdapter == nullptr )
          return;

        const int iver =
          SK_GetDXGIAdapterInterfaceVer ( pAdapter );

        // IDXGIAdapter3 = DXGI 1.4 (Windows 10+)
        if ( iver >= 3 )
        {
          SK::DXGI::StartBudgetThread ( &pAdapter.p );
        }
      }
    }
  }
}

UINT
SK_D3D11_RemoveUndesirableFlags (UINT* Flags) noexcept
{
  const UINT original =
    *Flags;

  // The Steam overlay behaves strangely when this is present
  *Flags =
    ( original & ~D3D11_CREATE_DEVICE_PREVENT_ALTERING_LAYER_SETTINGS_FROM_REGISTRY );

  return
    original;
}


volatile LONG __dxgi_plugins_loaded = FALSE;

HRESULT
STDMETHODCALLTYPE
SK_D3D11Dev_CreateRenderTargetView_Finish (
  _In_            ID3D11Device                   *pDev,
  _In_            ID3D11Resource                 *pResource,
  _In_opt_  const D3D11_RENDER_TARGET_VIEW_DESC  *pDesc,
  _Out_opt_       ID3D11RenderTargetView        **ppRTView,
                  BOOL                            bWrapped )
{
  SK_LOG1 ( ( L"CreateRTV, Format: %hs",
                   SK_DXGI_FormatToStr ( pDesc != nullptr ?
                                         pDesc->Format    :
                                          DXGI_FORMAT_UNKNOWN).data () ),
              L"  D3D 11  " );

  HRESULT ret = E_UNEXPECTED;

  auto orig_se =
  SK_SEH_ApplyTranslator (
    SK_FilteringStructuredExceptionTranslator (
      EXCEPTION_ACCESS_VIOLATION
    )
  );
  try
  {
    #ifndef NO_UNITY_HACKS
  if (   pDesc                != nullptr &&
       ( pDesc->ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2D   ||
         pDesc->ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2DMS ) )
  {
    SK_ComQIPtr <ID3D11Texture2D> pTex2D (pResource);

    if (pTex2D.p != nullptr)
    {
      D3D11_TEXTURE2D_DESC
                        texDesc = { };
      pTex2D->GetDesc (&texDesc);

      // For HDR Retrofit, engine may be really stubbornly
      //   insisting this is some other format.
      if ( pDesc->Format  != texDesc.Format &&
           pDesc->Format  != DXGI_FORMAT_UNKNOWN &&
           DirectX::BitsPerColor (texDesc.Format) !=
           DirectX::BitsPerColor ( pDesc->Format) )
      {
        ((D3D11_RENDER_TARGET_VIEW_DESC *)pDesc)->Format =
                                          texDesc.Format;
        //pDesc = nullptr;
      }

      if (texDesc.Format == DXGI_FORMAT_R16G16B16A16_FLOAT)
        ((D3D11_RENDER_TARGET_VIEW_DESC *)pDesc)->Format =
                                          texDesc.Format;
    }
  }
#endif

    ret =
      bWrapped ?
        pDev->CreateRenderTargetView ( pResource, pDesc, ppRTView )
                 :
        D3D11Dev_CreateRenderTargetView_Original ( pDev,  pResource,
                                                   pDesc, ppRTView );
  }

  catch (const SK_SEH_IgnoredException&)
  {
    ret =
      bWrapped ?
        pDev->CreateRenderTargetView ( pResource, nullptr, ppRTView )
                 :
        D3D11Dev_CreateRenderTargetView_Original ( pDev,  pResource,
                                                   nullptr, ppRTView );
  }
  SK_SEH_RemoveTranslator (orig_se);

  return ret;
}

HRESULT
STDMETHODCALLTYPE
SK_D3D11Dev_CreateRenderTargetView_Impl (
  _In_            ID3D11Device                   *pDev,
  _In_            ID3D11Resource                 *pResource,
  _In_opt_  const D3D11_RENDER_TARGET_VIEW_DESC  *pDesc,
  _Out_opt_       ID3D11RenderTargetView        **ppRTView,
                  BOOL                            bWrapped )
{
  auto _Finish =
  [&](const D3D11_RENDER_TARGET_VIEW_DESC* pDesc_) ->
  HRESULT
  {
    return
      SK_D3D11Dev_CreateRenderTargetView_Finish (
        pDev, pResource,
          pDesc_, ppRTView,
            bWrapped
      );
  };

  ///if (bWrapped)
  ///  return _Finish (pDesc);


  // Unity throws around NULL for pResource
  if (pResource != nullptr)
  {
    D3D11_RENDER_TARGET_VIEW_DESC desc = { };
    D3D11_RESOURCE_DIMENSION      dim  = { };

    pResource->GetType (&dim);

    if (dim == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
    {
      if (pDesc != nullptr)
        desc = *pDesc;

      DXGI_FORMAT newFormat =
        desc.Format;

      SK_ComQIPtr <ID3D11Texture2D>
          pTex (pResource);
      if (pTex != nullptr)
      {
        D3D11_TEXTURE2D_DESC  tex_desc = { };
        pTex->GetDesc       (&tex_desc);

        // If the SwapChain was sRGB originally, and this RTV is
        //   the SwapChain's backbuffer, create an sRGB view
        //     (of the now linear SwapChain).
        extern bool bOriginallysRGB;
        if (        bOriginallysRGB && (! __SK_HDR_16BitSwap))
        {                              // HDR handles the mismatch on its own
          SK_ComPtr   <ID3D11Texture2D> pBackbuffer;
          SK_ComQIPtr <IDXGISwapChain>  pSwapChain (
            SK_GetCurrentRenderBackend ().swapchain);

          if (pSwapChain.p != nullptr)
          {
            if ( SUCCEEDED (
                   pSwapChain->GetBuffer ( 0, IID_ID3D11Texture2D,
                   (void **)&pBackbuffer.p) )
               )
            {
              if (pTex.IsEqualObject (pBackbuffer.p))
              {
                desc.Format     = DXGI_FORMAT_420_OPAQUE;
                tex_desc.Format = DirectX::MakeSRGB (tex_desc.Format);
              }
            }
          }
        }

        if (                        desc.Format      != DXGI_FORMAT_UNKNOWN &&
             DirectX::MakeTypeless (tex_desc.Format) !=
             DirectX::MakeTypeless (desc.Format) )
          desc.Format = tex_desc.Format;

        if ( SK_D3D11_OverrideDepthStencil (newFormat) )
          desc.Format = newFormat;

        if (pDesc != nullptr)
        {
          const HRESULT hr =
            _Finish (&desc);

          if (SUCCEEDED (hr))
            return hr;
        }
      }
    }
  }

  return
    _Finish (pDesc);
}

void
STDMETHODCALLTYPE
SK_D3D11_UpdateSubresource_Impl (
  _In_           ID3D11DeviceContext *pDevCtx,
  _In_           ID3D11Resource      *pDstResource,
  _In_           UINT                 DstSubresource,
  _In_opt_ const D3D11_BOX           *pDstBox,
  _In_     const void                *pSrcData,
  _In_           UINT                 SrcRowPitch,
  _In_           UINT                 SrcDepthPitch,
                 BOOL                 bWrapped )
{
  SK_WRAP_AND_HOOK

  const auto _Finish = [&](void) ->
  void
  {
    bWrapped ?
      pDevCtx->UpdateSubresource ( pDstResource, DstSubresource,
                                     pDstBox, pSrcData, SrcRowPitch,
                                       SrcDepthPitch )
             :
      D3D11_UpdateSubresource_Original ( pDevCtx, pDstResource, DstSubresource,
                                           pDstBox, pSrcData, SrcRowPitch,
                                             SrcDepthPitch );
  };

  bool early_out =
    ( SK_D3D11_IgnoreWrappedOrDeferred (bWrapped, pDevCtx) ||
    (! bMustNotIgnore) );

  SK_TLS *pTLS = nullptr;

  // UB: If it's happening, pretend we never saw this...
  if (pDstResource == nullptr || pSrcData == nullptr)
  {
    early_out = true;
  }

  // Partial updates are of little interest.
  if ( DstSubresource != 0 )
  {
    early_out = true;
  }


  static const bool __attempt_to_cache = config.textures.d3d11.cache;

  if ( pDstBox != nullptr && ( pDstBox->left  >= pDstBox->right  ||
                               pDstBox->top   >= pDstBox->bottom ||
                               pDstBox->front >= pDstBox->back )    )
  {
    early_out = true;
  }

  D3D11_RESOURCE_DIMENSION rdim = D3D11_RESOURCE_DIMENSION_UNKNOWN;
  // Not an optional param, but the D3D11 runtime doesn't crash if
  //   NULL is passed and neither should we ...
  if (pDstResource)
      pDstResource->GetType  (&rdim);

  if (! early_out)
  {
    early_out =
    (
      rdim !=
         D3D11_RESOURCE_DIMENSION_TEXTURE2D ||
      SK_D3D11_IsTexInjectThread (
          (pTLS = SK_TLS_Bottom ())
      )
    );
  }

  if (early_out)
  {
    return
      _Finish ();
  }

  if ( __attempt_to_cache && (    (rdim == D3D11_RESOURCE_DIMENSION_TEXTURE2D) ||
      SK_D3D11_IsStagingCacheable (rdim, pDstResource) ) && DstSubresource == 0 )
  {
    auto& textures =
      SK_D3D11_Textures;

    SK_ComQIPtr <ID3D11Texture2D> pTex (pDstResource);

    if (pTex != nullptr)
    {
      D3D11_TEXTURE2D_DESC desc = { };
           pTex->GetDesc (&desc);

      if (__attempt_to_cache)
      {
        const bool skip =
          ( (desc.Usage == D3D11_USAGE_STAGING    && (! SK_D3D11_IsStagingCacheable (rdim, pDstResource))) ||
             desc.Usage == D3D11_USAGE_DYNAMIC    ||
             SK_D3D11_IsTextureUncacheable (pTex) ||
            (! DirectX::IsCompressed (desc.Format)) );

        if (skip)
        {
          return
            _Finish ();
        }
      }


      D3D11_SUBRESOURCE_DATA srd = { };

      srd.pSysMem           = pSrcData;
      srd.SysMemPitch       = SrcRowPitch;
      srd.SysMemSlicePitch  = 0;

      size_t   size         = 0;
      uint32_t top_crc32c   = 0x0;

      uint32_t checksum     =
        crc32_tex   (&desc, &srd, &size, &top_crc32c, false);

      auto _StripTransientProperties =
        [](D3D11_TEXTURE2D_DESC& desc) ->
           D3D11_TEXTURE2D_DESC
        {
          D3D11_TEXTURE2D_DESC
            stripped_desc = desc;

          stripped_desc.Format             = DirectX::MakeTypeless (desc.Format);
          stripped_desc.BindFlags          =              D3D11_BIND_SHADER_RESOURCE;
          stripped_desc.Usage              = (D3D11_USAGE)D3D11_USAGE_DEFAULT;
          stripped_desc.CPUAccessFlags     =   0;
          stripped_desc.MiscFlags          = 0x0;
          stripped_desc.SampleDesc.Count   =   1;
          stripped_desc.SampleDesc.Quality =   0;
          stripped_desc.ArraySize          =   1;
          stripped_desc.MipLevels          =   0;

          return stripped_desc;
        };

      const auto transient_desc =
        _StripTransientProperties (desc);

const uint32_t cache_tag    =
        safe_crc32c (top_crc32c, (uint8_t *)(&transient_desc), sizeof (D3D11_TEXTURE2D_DESC));

      SK_ScopedBool decl_tex_scope (
        SK_D3D11_DeclareTexInjectScope (pTLS)
      );

      const auto start =
        SK_QueryPerf ().QuadPart;

      SK_ComPtr <ID3D11Texture2D> pCachedTex;
      pCachedTex.Attach (
        textures->getTexture2D ( cache_tag, &desc,
                                   nullptr, nullptr,
                                               pTLS )
      );

      if (pCachedTex != nullptr)
      {
        SK_ComQIPtr <ID3D11Resource> pCachedResource (pCachedTex);

        bWrapped ?
          pDevCtx->CopyResource (pDstResource,                pCachedResource)
                 :
          D3D11_CopyResource_Original (pDevCtx, pDstResource, pCachedResource);

        SK_LOG1 ( ( L"Texture Cache Hit (Slow Path): (%lux%lu) -- %x",
                      desc.Width, desc.Height, top_crc32c ),
                    L"DX11TexMgr" );

        textures->recordCacheHit (pCachedTex);

        return;
      }

      else
      {
        if (SK_D3D11_TextureIsCached (pTex))
        {
          SK_LOG0 ( (L"Cached texture was updated (UpdateSubresource)... removing from cache! - <%s>",
                         SK_GetCallerName ().c_str ()), L"DX11TexMgr" );
          SK_D3D11_RemoveTexFromCache     (pTex, true);
          SK_D3D11_MarkTextureUncacheable (pTex);
        }

        _Finish ();

        const auto end     = SK_QueryPerf ().QuadPart;
              auto elapsed = end - start;

        if (desc.Usage == D3D11_USAGE_STAGING)
        {
          auto& map_ctx = (*mapped_resources)[pDevCtx];

          map_ctx.dynamic_textures  [pDstResource] = checksum;
          map_ctx.dynamic_texturesx [pDstResource] = top_crc32c;

          SK_LOG1 ( ( L"New Staged Texture: (%lux%lu) -- %x",
                        desc.Width, desc.Height, top_crc32c ),
                      L"DX11TexMgr" );

          map_ctx.dynamic_times2    [checksum]  = elapsed;
          map_ctx.dynamic_sizes2    [checksum]  = size;

          return;
        }

        if (desc.Usage == D3D11_USAGE_DEFAULT)
        {
        //-------------------

          bool cacheable = ((desc.MiscFlags <= 4 || desc.MiscFlags == D3D11_RESOURCE_MISC_RESOURCE_CLAMP) &&
                             desc.Width      > 0 &&
                             desc.Height     > 0 &&
                             desc.ArraySize == 1 //||
                           //((desc.ArraySize  % 6 == 0) && (desc.MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE))
                           );

          const bool compressed =
            DirectX::IsCompressed (desc.Format);

          // If this isn't an injectable texture, then filter out non-mipmapped
          //   textures.
          if (/*(! injectable) && */cache_opts.ignore_non_mipped)
            cacheable &= (desc.MipLevels > 1 || compressed);

          if (cacheable)
          {
            bool injected = false;

            // -----------------------------
            if (SK_D3D11_res_root->length ())
            {
              wchar_t     wszTex [MAX_PATH + 2] = { };
              wcsncpy_s ( wszTex, MAX_PATH,
                          SK_D3D11_TexNameFromChecksum (top_crc32c, checksum, 0x0).c_str (),
                          _TRUNCATE );

              if (                  *wszTex  != L'\0' &&
                  GetFileAttributes (wszTex) != INVALID_FILE_ATTRIBUTES )
              {
                HRESULT hr = E_UNEXPECTED;

                DirectX::TexMetadata mdata = { };

                if (SUCCEEDED ((hr = DirectX::GetMetadataFromDDSFile (wszTex, 0, mdata))))
                {
                  if ( ( DirectX::MakeTypeless      (mdata.format) != DirectX::MakeTypeless      (desc.Format) &&
                         DirectX::MakeTypelessUNORM (mdata.format) != DirectX::MakeTypelessUNORM (desc.Format) &&
                         DirectX::MakeTypelessFLOAT (mdata.format) != DirectX::MakeTypelessFLOAT (desc.Format) &&
                         DirectX::MakeSRGB          (mdata.format) != DirectX::MakeSRGB          (desc.Format) )
                      || mdata.width  != desc.Width
                      || mdata.height != desc.Height )
                  {
                    SK_LOG0 ( ( L"Texture injection for texture '%x' failed due to format / size mismatch.",
                                  top_crc32c ), L"DX11TexMgr" );
                  }

                  else
                  {
                    DirectX::ScratchImage img;

                    if (SUCCEEDED ((hr = DirectX::LoadFromDDSFile (wszTex, 0, &mdata, img))))
                    {
                      SK_ComPtr <ID3D11Texture2D>
                                               pSurrogateTexture2D = nullptr;
                      SK_ComPtr <ID3D11Device> pDev                = nullptr;

                          pDevCtx->GetDevice (&pDev);

                      if (SUCCEEDED ((hr = DirectX::CreateTexture (pDev,
                                             img.GetImages     (),
                                             img.GetImageCount (), mdata,
                                             reinterpret_cast <ID3D11Resource **> (&pSurrogateTexture2D.p))))
                         )
                      {
                        const LARGE_INTEGER load_end =
                          SK_QueryPerf ();

                        pTex  =  nullptr;
                        pTex.Attach (
                          reinterpret_cast <ID3D11Texture2D *> (pDstResource)
                        );

                        bWrapped ?
                          pDevCtx->CopyResource                (pDstResource, pSurrogateTexture2D)
                                 :
                          D3D11_CopyResource_Original (pDevCtx, pDstResource, pSurrogateTexture2D);

                        injected = true;
                      }

                      else
                      {
                        SK_LOG0 ( (L"*** Texture '%s' failed DirectX::CreateTexture (...) -- (HRESULT=%s), skipping!",
                                     SK_ConcealUserDir (wszTex), _com_error (hr).ErrorMessage () ),
                                   L"DX11TexMgr" );
                      }
                    }
                    else
                    {
                      SK_LOG0 ( (L"*** Texture '%s' failed DirectX::LoadFromDDSFile (...) -- (HRESULT=%s), skipping!",
                                   SK_ConcealUserDir (wszTex), _com_error (hr).ErrorMessage () ),
                                  L"DX11TexMgr" );
                    }
                  }
                }

                else
                {
                  SK_LOG0 ( (L"*** Texture '%s' failed DirectX::GetMetadataFromDDSFile (...) -- (HRESULT=%s), skipping!",
                               SK_ConcealUserDir (wszTex), _com_error (hr).ErrorMessage () ),
                             L"DX11TexMgr" );
                }
              }
            }

            SK_LOG1 ( ( L"New Cacheable Texture: (%lux%lu) -- %x",
                          desc.Width, desc.Height, top_crc32c ),
                        L"DX11TexMgr" );

            textures->CacheMisses_2D++;
            textures->refTexture2D (pTex, &desc, cache_tag, size, elapsed, top_crc32c,
                                     L"", nullptr, (HMODULE)(intptr_t)-1/*SK_GetCallingDLL ()*/, pTLS);

            if (injected)
            {
              textures->Textures_2D [pTex].injected = true;
            }
          }
        }

        return;
      }
    }
  }

  return
    _Finish ();
}

void
STDMETHODCALLTYPE
SK_D3D11_CopySubresourceRegion_Impl (
            ID3D11DeviceContext *pDevCtx,
  _In_           ID3D11Resource *pDstResource,
  _In_           UINT            DstSubresource,
  _In_           UINT            DstX,
  _In_           UINT            DstY,
  _In_           UINT            DstZ,
  _In_           ID3D11Resource *pSrcResource,
  _In_           UINT            SrcSubresource,
  _In_opt_ const D3D11_BOX      *pSrcBox,
                 BOOL            bWrapped )
{
  SK_WRAP_AND_HOOK

  const auto _Finish = [&](void) ->
  void
  {
    bWrapped ?
      pDevCtx->CopySubresourceRegion ( pDstResource, DstSubresource, DstX, DstY, DstZ,
                                       pSrcResource, SrcSubresource, pSrcBox )
             :
      D3D11_CopySubresourceRegion_Original ( pDevCtx, pDstResource, DstSubresource, DstX, DstY, DstZ,
                                                      pSrcResource, SrcSubresource, pSrcBox );
  };

  bool early_out =
    ( SK_D3D11_IgnoreWrappedOrDeferred (bWrapped, pDevCtx) ||
    (! bMustNotIgnore) );

  if (early_out)
  {
    return
      _Finish ();
  }

  // UB: If it's happening, pretend we never saw this...
  if (pDstResource == nullptr || pSrcResource == nullptr)
  {
    return;
  }

  if ( (! config.render.dxgi.deferred_isolation) &&
          SK_D3D11_IsDevCtxDeferred (pDevCtx) )
  {
    return
      _Finish ();
  }

  // ImGui gets to pass-through without invoking the hook
  ////if (! config.textures.cache.allow_staging)
  ////{
  ////  if (! SK_D3D11_ShouldTrackRenderOp (pDevCtx, dev_ctx_handle_))
  ////  {
  ////    early_out = true;
  ////  }
  ////}
  ////
  ////if (early_out)
  ////{
  ////  return
  ////    _Finish ();
  ////}


  D3D11_RESOURCE_DIMENSION res_dim = { };
  pSrcResource->GetType  (&res_dim);


  if (SK_D3D11_EnableMMIOTracking)
  {
    SK_D3D11_MemoryThreads->mark ();

    switch (res_dim)
    {
      case D3D11_RESOURCE_DIMENSION_UNKNOWN:
        mem_map_stats->last_frame.resource_types [D3D11_RESOURCE_DIMENSION_UNKNOWN]++;
        break;
      case D3D11_RESOURCE_DIMENSION_BUFFER:
      {
        mem_map_stats->last_frame.resource_types [D3D11_RESOURCE_DIMENSION_BUFFER]++;

        SK_ComQIPtr <ID3D11Buffer> pBuffer (pSrcResource);

        if (pBuffer.p != nullptr)
        {
          D3D11_BUFFER_DESC  buf_desc = { };
          pBuffer->GetDesc (&buf_desc);
          {
            ////std::scoped_lock <SK_Thread_CriticalSection> auto_lock (cs_mmio);

            if (buf_desc.BindFlags & D3D11_BIND_INDEX_BUFFER)
              mem_map_stats->last_frame.buffer_types [0]++;
            //mem_map_stats->last_frame.index_buffers.insert (pBuffer);

            if (buf_desc.BindFlags & D3D11_BIND_VERTEX_BUFFER)
              mem_map_stats->last_frame.buffer_types [1]++;
            //mem_map_stats->last_frame.vertex_buffers.insert (pBuffer);

            if (buf_desc.BindFlags & D3D11_BIND_CONSTANT_BUFFER)
              mem_map_stats->last_frame.buffer_types [2]++;
            //mem_map_stats->last_frame.constant_buffers.insert (pBuffer);
          }

          mem_map_stats->last_frame.bytes_copied += buf_desc.ByteWidth;
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

  _Finish ();

  if (res_dim != D3D11_RESOURCE_DIMENSION_TEXTURE2D)
    return;

#pragma region CacheStagingMappedResources
  if (config.textures.d3d11.cache && config.textures.cache.allow_staging)
  {
    if ( /*( SK_D3D11_IsStagingCacheable (res_dim, pDstResource)/*||
             SK_D3D11_IsStagingCacheable (res_dim, pSrcResource))*/
             res_dim == D3D11_RESOURCE_DIMENSION_TEXTURE2D && SrcSubresource == 0 && DstSubresource == 0)
    {
      auto& map_ctx = (*mapped_resources)[pDevCtx];

      if (map_ctx.dynamic_textures.count (pSrcResource) > 0 /*&& (! SK_D3D11_TextureIsCached (pDstTex))*/)
      {
        SK_ComQIPtr <ID3D11Texture2D>
                       pDstTex (pDstResource);

        const uint32_t top_crc32 = map_ctx.dynamic_texturesx [pSrcResource];
        const uint32_t checksum  = map_ctx.dynamic_textures  [pSrcResource];

        D3D11_TEXTURE2D_DESC dst_desc = { };
        pDstTex->GetDesc   (&dst_desc);

        const uint32_t cache_tag =
          safe_crc32c (top_crc32, (uint8_t *)(&dst_desc), sizeof (D3D11_TEXTURE2D_DESC));

        if (checksum != 0x00 && dst_desc.Usage != D3D11_USAGE_STAGING)
        {
          SK_TLS* pTLS =
            SK_TLS_Bottom ();

          static auto& textures =
            SK_D3D11_Textures;

          std::wstring filename;

          // Temp hack for 1-LOD Staging Texture Uploads
          bool injectable = (
            SK_D3D11_IsInjectable (top_crc32, checksum) ||
            SK_D3D11_IsInjectable (top_crc32, 0x00)
          );

          if (injectable && (! SK_D3D11_TextureIsCached (pDstTex)))
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

                    ///SK_ReleaseAssert (
                    ///  map_ctx.dynamic_sizes2 [checksum] == SK_D3D11_ComputeTextureSize (&new_desc)
                    ///);

                    std::scoped_lock <SK_Thread_HybridSpinlock>
                          scope_lock (*cache_cs);

                    pDevCtx->CopyResource (pDstResource, pOverrideTex);

                    //const ULONGLONG load_end =
                    //  (ULONGLONG)SK_QueryPerf ().QuadPart;
                    //
                    //time_elapsed = load_end - map_ctx.texture_times [pDstResource];
                    filename     = wszTex;

                    SK_LOG0 ( ( L" *** Texture Injected Late... %x :: %x  { %ws }",
                                                        top_crc32, cache_tag, wszTex ),
                                L"StagingTex" );
                  }
                }
              }
            }
          }

          if ((! config.render.dxgi.low_spec_mode) || (! filename.empty ()))
          {
            if (! SK_D3D11_TextureIsCached (pDstTex))
            {
              textures->CacheMisses_2D++;

              textures->refTexture2D ( pDstTex,
                                        &dst_desc,
                                          cache_tag,
                                            map_ctx.dynamic_sizes2   [checksum],
                                              map_ctx.dynamic_times2 [checksum],
                                                top_crc32,
                                                  filename.empty () ? map_ctx.dynamic_files2 [checksum].c_str () :
                                                  filename.c_str (),
                                                    nullptr, (HMODULE)(intptr_t)-1/*SK_GetCallingDLL ()*/,
                                                      pTLS );
            }

            else
              textures->recordCacheHit (pDstTex);

            if ((! filename.empty ()) || (! map_ctx.dynamic_files2 [checksum].empty ()))
              textures->Textures_2D [pDstTex].injected = true;
          }

          ////map_ctx.dynamic_textures.erase  (pSrcResource);
          ////map_ctx.dynamic_texturesx.erase (pSrcResource);
          ////
          ////map_ctx.dynamic_sizes2.erase    (checksum);
          ////map_ctx.dynamic_times2.erase    (checksum);
          ////map_ctx.dynamic_files2.erase    (checksum);
        }

        map_ctx.dynamic_textures.erase  (pSrcResource);
        map_ctx.dynamic_texturesx.erase (pSrcResource);

        map_ctx.dynamic_sizes2.erase    (checksum);
        map_ctx.dynamic_times2.erase    (checksum);
        map_ctx.dynamic_files2.erase    (checksum);
      }
    }
  }
#pragma endregion CacheStagingMappedResources
}

void
STDMETHODCALLTYPE
SK_D3D11_CopyResource_Impl (
       ID3D11DeviceContext *pDevCtx,
  _In_ ID3D11Resource      *pDstResource,
  _In_ ID3D11Resource      *pSrcResource,
       BOOL                 bWrapped )
{
  SK_WRAP_AND_HOOK

  const auto _Finish = [&](void) ->
  void
  {
    bWrapped ?
      pDevCtx->CopyResource (pDstResource, pSrcResource)
             :
      D3D11_CopyResource_Original (pDevCtx, pDstResource, pSrcResource);
  };

  bool early_out =
    ( SK_D3D11_IgnoreWrappedOrDeferred (bWrapped, pDevCtx) ||
    (! bMustNotIgnore) );

  if (early_out)
  {
    return
      _Finish ();
  }


  // UB: If it's happening, pretend we never saw this...
  if (pDstResource == nullptr || pSrcResource == nullptr)
  {
    return;
  }

  if ( (! config.render.dxgi.deferred_isolation) &&
          SK_D3D11_IsDevCtxDeferred (pDevCtx) )
  {
    return
      _Finish ();
  }


  D3D11_RESOURCE_DIMENSION res_dim = { };
   pSrcResource->GetType (&res_dim);


  if (SK_D3D11_EnableMMIOTracking)
  {
    SK_D3D11_MemoryThreads->mark ();

    switch (res_dim)
    {
      case D3D11_RESOURCE_DIMENSION_UNKNOWN:
        mem_map_stats->last_frame.resource_types [D3D11_RESOURCE_DIMENSION_UNKNOWN]++;
        break;
      case D3D11_RESOURCE_DIMENSION_BUFFER:
      {
        mem_map_stats->last_frame.resource_types [D3D11_RESOURCE_DIMENSION_BUFFER]++;

        ID3D11Buffer* pBuffer = nullptr;

        if (SUCCEEDED (pSrcResource->QueryInterface <ID3D11Buffer> (&pBuffer)))
        {
          D3D11_BUFFER_DESC  buf_desc = { };
          pBuffer->GetDesc (&buf_desc);
          {
            if (buf_desc.BindFlags & D3D11_BIND_INDEX_BUFFER)
              mem_map_stats->last_frame.buffer_types [0]++;
              //mem_map_stats->last_frame.index_buffers.insert (pBuffer);

            if (buf_desc.BindFlags & D3D11_BIND_VERTEX_BUFFER)
              mem_map_stats->last_frame.buffer_types [1]++;
              //mem_map_stats->last_frame.vertex_buffers.insert (pBuffer);

            if (buf_desc.BindFlags & D3D11_BIND_CONSTANT_BUFFER)
              mem_map_stats->last_frame.buffer_types [2]++;
              //mem_map_stats->last_frame.constant_buffers.insert (pBuffer);
          }

          mem_map_stats->last_frame.bytes_copied += buf_desc.ByteWidth;

          pBuffer->Release ();
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


  // What if we're trying to copy to a dest texture that needs format changed to R16G16B16A16 (HDR?)
  //
  //   Needs a stretchrect type of deal, suggest user override fmt.
  //
  if (res_dim == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
  {
    SK_ComQIPtr <ID3D11Texture2D> pDstTex (pDstResource);
    SK_ComQIPtr <ID3D11Texture2D> pSrcTex (pSrcResource);

    D3D11_TEXTURE2D_DESC dst_desc = { };
    D3D11_TEXTURE2D_DESC src_desc = { };

    pSrcTex->GetDesc (&src_desc);
    pDstTex->GetDesc (&dst_desc);

    if (src_desc.Format == DXGI_FORMAT_R16G16B16A16_FLOAT && src_desc.Format != dst_desc.Format)
    {
      extern bool SK_D3D11_BltCopySurface (ID3D11Texture2D *pSrcTex, ID3D11Texture2D *pDstTex);

      if (! SK_D3D11_BltCopySurface (pSrcTex, pDstTex))
      {
        if (config.system.log_level > 0)
        {
          static bool          warned_once = false;
          if (! std::exchange (warned_once, true))
          {
            SK_ImGui_Warning (
              SK_FormatStringW (
                L"HDR Format Mismatch During CopyResource: Src=%hs, Dst=%hs",
                  SK_DXGI_FormatToStr (src_desc.Format).data (),
                  SK_DXGI_FormatToStr (dst_desc.Format).data ()
              ).c_str ()
            );
          }
        }
      }

      else
      {
        if (config.system.log_level > 0)
        {
          static bool          warned_once = false;
          if (! std::exchange (warned_once, true))
          {
            SK_ImGui_Warning (
              L"HDR Format Mismatch Corrected using SK_D3D11_BltCopySurface (!!)"
            );
          }
        }
      }

      return;
    }
  }


  _Finish ();


  if (res_dim != D3D11_RESOURCE_DIMENSION_TEXTURE2D)
    return;

  SK_TLS *pTLS =
    nullptr;

  ////if (res_dim == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
  ////{
  ////  SK_ComQIPtr <ID3D11Texture2D> pDstTex = pDstResource;
  ////
  ////  if (! SK_D3D11_IsTexInjectThread (pTLS))
  ////  {
  ////    if (SK_D3D11_TextureIsCached (pDstTex))
  ////    {
  ////      //SK_LOG0 ( (L"Cached texture was modified (CopyResource)... removing from cache! - <%s>",
  ////      //               SK_GetCallerName ().c_str ()), L"DX11TexMgr" );
  ////      //SK_D3D11_RemoveTexFromCache (pDstTex, true);
  ////    }
  ////  }
  ////}


  // ImGui gets to pass-through without invoking the hook
  if ((! config.textures.cache.allow_staging) && (! SK_D3D11_ShouldTrackRenderOp (pDevCtx)))
    return;


  if ( SK_D3D11_IsStagingCacheable (res_dim, pSrcResource) ||
       SK_D3D11_IsStagingCacheable (res_dim, pDstResource) )
  {
    auto& map_ctx = (*mapped_resources)[pDevCtx];

    SK_ComQIPtr <ID3D11Texture2D> pSrcTex (pSrcResource);

    if (pSrcTex != nullptr)
    {
      if (SK_D3D11_TextureIsCached (pSrcTex))
      {
        dll_log->Log (
          L"Copying from cached source with checksum: %x",
          SK_D3D11_TextureHashFromCache (pSrcTex)
        );
      }
    }

    SK_ComQIPtr <ID3D11Texture2D> pDstTex (pDstResource);

    if (pDstTex != nullptr && map_ctx.dynamic_textures.count (pSrcResource))
    {
      const uint32_t top_crc32 = map_ctx.dynamic_texturesx [pSrcResource];
      const uint32_t checksum  = map_ctx.dynamic_textures  [pSrcResource];

      D3D11_TEXTURE2D_DESC dst_desc = { };
        pDstTex->GetDesc (&dst_desc);

      const uint32_t cache_tag =
        safe_crc32c (top_crc32, (uint8_t *)(&dst_desc), sizeof (D3D11_TEXTURE2D_DESC));

      if (checksum != 0x00 && dst_desc.Usage != D3D11_USAGE_STAGING)
      {
        static auto& textures =
          SK_D3D11_Textures;

        if (! SK_D3D11_TextureIsCached (pDstTex))
        {
          textures->CacheMisses_2D++;

          textures->refTexture2D ( pDstTex,
                                    &dst_desc,
                                      cache_tag,
                                        map_ctx.dynamic_sizes2   [checksum],
                                          map_ctx.dynamic_times2 [checksum],
                                            top_crc32,
                                              map_ctx.dynamic_files2 [checksum].c_str (),
                                                nullptr, (HMODULE)(intptr_t)-1/*SK_GetCallingDLL ()*/,
                                                  pTLS );
        }

        else
          textures->recordCacheHit (pDstTex);

        if (! map_ctx.dynamic_files2 [checksum].empty ())
          textures->Textures_2D [pDstTex].injected = true;

        map_ctx.dynamic_textures.erase  (pSrcResource);
        map_ctx.dynamic_texturesx.erase (pSrcResource);

        map_ctx.dynamic_sizes2.erase    (checksum);
        map_ctx.dynamic_times2.erase    (checksum);
        map_ctx.dynamic_files2.erase    (checksum);
      }
    }
  }
}

void
SK_D3D11_ClearResidualDrawState (UINT& d_idx, SK_TLS* pTLS = SK_TLS_Bottom ())
{
  if (pTLS == nullptr)
      pTLS  = SK_TLS_Bottom ();

  auto& pTLS_d3d11 =
    pTLS->render->d3d11.get ();

    d_idx =
  ( d_idx == UINT_MAX ? SK_D3D11_GetDeviceContextHandle (pTLS_d3d11.pDevCtx) :
    d_idx );

  if ( d_idx >= SK_D3D11_MAX_DEV_CONTEXTS )
  {
    return;
  }

  SK_ComQIPtr <ID3D11DeviceContext> pDevCtx (pTLS_d3d11.pDevCtx);

  //if (pTLS_d3d11.pOrigBlendState != nullptr)
  //{
  //  ID3D11BlendState* pOrigBlendState =
  //    pTLS_d3d11.pOrigBlendState;
  //
  //  if (pDevCtx != nullptr)
  //  {
  //    pDevCtx->OMSetBlendState ( pOrigBlendState,
  //                               pTLS_d3d11.fOrigBlendFactors,
  //                               pTLS_d3d11.uiOrigBlendMask );
  //  }
  //
  //  pTLS_d3d11.pOrigBlendState = nullptr;
  //}

  if (pTLS_d3d11.pRTVOrig != nullptr)
  {
    ID3D11RenderTargetView* pRTVOrig =
      pTLS_d3d11.pRTVOrig;

    if (pDevCtx != nullptr)
    {
      pDevCtx->OMSetRenderTargets (1, &pRTVOrig, nullptr);
    }

    pTLS_d3d11.pRTVOrig = nullptr;
  }

  if (pTLS_d3d11.pDSVOrig != nullptr)
  {
    ID3D11DepthStencilView *pDSVOrig =
      pTLS_d3d11.pDSVOrig;

    if (pDevCtx != nullptr)
    {
      SK_ComPtr <ID3D11RenderTargetView> pRTV [8] = { };
      SK_ComPtr <ID3D11DepthStencilView> pDSV;

      pDevCtx->OMGetRenderTargets ( 8, &pRTV [0].p,
                                       &pDSV    .p );

      const UINT OMRenderTargetCount =
        calc_count (&pRTV [0].p, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT);

      pDevCtx->OMSetRenderTargets (OMRenderTargetCount, &pRTV [0].p, pDSVOrig);

      for (UINT i = 0; i < OMRenderTargetCount; i++)
      {
        pRTV [i] = nullptr;
      }

      pTLS_d3d11.pDSVOrig = nullptr;
    }
  }

  if (pTLS_d3d11.pDepthStencilStateNew != nullptr)
  {
    ID3D11DepthStencilState *pOrig =
      pTLS_d3d11.pDepthStencilStateOrig;

    if (pDevCtx != nullptr)
    {
      pDevCtx->OMSetDepthStencilState (pOrig, pTLS_d3d11.StencilRefOrig);
    }

    pTLS_d3d11.pDepthStencilStateNew  = nullptr;
    pTLS_d3d11.pDepthStencilStateOrig = nullptr;
  }


  if (pTLS_d3d11.pRasterStateNew != nullptr)
  {
    ID3D11RasterizerState *pOrig =
      pTLS_d3d11.pRasterStateOrig;

    if (pDevCtx != nullptr)
    {
      pDevCtx->RSSetState (pOrig);
    }

    pTLS_d3d11.pRasterStateNew  = nullptr;
    pTLS_d3d11.pRasterStateOrig = nullptr;
  }


  for (int i = 0; i < 5; i++)
  {
    if ( pTLS_d3d11.pOriginalCBuffers [i][0] != nullptr ||
         pTLS_d3d11.empty_cbuffers    [i] )
    {
      if ( pTLS_d3d11.empty_cbuffers  [i] )
      {
        pTLS_d3d11.pOriginalCBuffers  [i][0] = nullptr;
        pTLS_d3d11.empty_cbuffers     [i]    = false;
      }

      switch (i)
      {
        default: SK_ReleaseAssert (!"Bad Codepath Encountered"); break;
        case 0:
          pDevCtx->VSSetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, &(pTLS_d3d11.pOriginalCBuffers [i][0]).p);
          break;
        case 1:
          pDevCtx->PSSetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, &(pTLS_d3d11.pOriginalCBuffers [i][0]).p);
          break;
        case 2:
          pDevCtx->GSSetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, &(pTLS_d3d11.pOriginalCBuffers [i][0]).p);
          break;
        case 3:
          pDevCtx->HSSetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, &(pTLS_d3d11.pOriginalCBuffers [i][0]).p);
          break;
        case 4:
          pDevCtx->DSSetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, &(pTLS_d3d11.pOriginalCBuffers [i][0]).p);
          break;
      }

      for (int j = 0; j < D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT; j++)
      {
        if (pTLS_d3d11.pOriginalCBuffers [i][j] != nullptr)
        {   pTLS_d3d11.pOriginalCBuffers [i][j]  = nullptr;
        }
      }
    }
  }
}


void
SK_D3D11_PostDraw (UINT& dev_idx, SK_TLS *pTLS)
{
  SK_D3D11_ClearResidualDrawState (dev_idx, pTLS);
}

void*
__cdecl
SK_SEH_Guarded_memcpy (
    _Out_writes_bytes_all_ (_Size) void       *_Dst,
    _In_reads_bytes_       (_Size) void const *_Src,
    _In_                           size_t      _Size )
{
  void* ret = _Dst;

  auto orig_se =
  SK_SEH_ApplyTranslator (
    SK_FilteringStructuredExceptionTranslator (
      EXCEPTION_ACCESS_VIOLATION
    )
  );
  try {
    ret =
      memcpy (_Dst, _Src, _Size);
  }

  catch (const SK_SEH_IgnoredException&)
  {
    ret = _Dst;
  }
  SK_SEH_RemoveTranslator (orig_se);

  return ret;
}

SK_LazyGlobal <concurrency::concurrent_vector <d3d11_shader_tracking_s::cbuffer_override_s>> __SK_D3D11_VertexShader_CBuffer_Overrides;
SK_LazyGlobal <concurrency::concurrent_vector <d3d11_shader_tracking_s::cbuffer_override_s>> __SK_D3D11_PixelShader_CBuffer_Overrides;

bool SK_D3D11_KnownShaders::reshade_triggered;

// Indicates whether the shader mod window is tracking render target refs
extern bool live_rt_view;

bool
SK_D3D11_DrawCallFilter (int elem_cnt, int vtx_cnt, uint32_t vtx_shader)
{
  UNREFERENCED_PARAMETER (elem_cnt);   UNREFERENCED_PARAMETER (vtx_cnt);
  UNREFERENCED_PARAMETER (vtx_shader);

#if 1
  return false;
#else
  if (SK_D3D11_BlacklistDrawcalls->empty ())
    return false;

  const auto& matches =
    SK_D3D11_BlacklistDrawcalls->equal_range (vtx_shader);

  for ( auto& it = matches.first; it != matches.second; ++it )
  {
    if (it->second.if_meshes.have.less_than.vertices.first)
    {
      if (vtx_cnt < it->second.if_meshes.have.less_than.vertices.second)
        return true;
    }

    if (it->second.if_meshes.have.more_than.vertices.first)
    {
      if (vtx_cnt > it->second.if_meshes.have.more_than.vertices.second)
        return true;
    }
  }

  return false;
#endif
}

static auto constexpr SK_CBUFFER_SLOTS =
  D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT;

enum SK_D3D11_DrawHandlerState
{
  Normal,
  Override,
  Skipped
};

static
  UINT NegativeOne = (UINT)-1;

SK_D3D11_DrawHandlerState
SK_D3D11_DrawHandler ( ID3D11DeviceContext  *pDevCtx,
                       SK_TLS              **ppTLS   = nullptr,
                       UINT&                 dev_idx = NegativeOne )
{
  static SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  static auto game_type = SK_GetCurrentGameID ();

  // Make sure state cleanup happens on the same context, or deferred
  //   rendering will make life miserable!

  if ( rb.d3d11.immediate_ctx == nullptr ||
       rb.device              == nullptr ||
       rb.swapchain           == nullptr )
  {
    if ( rb.d3d11.immediate_ctx == nullptr &&
            pDevCtx->GetType () != D3D11_DEVICE_CONTEXT_DEFERRED )
    {
      rb.d3d11.immediate_ctx = pDevCtx;
    }

    if (rb.d3d11.immediate_ctx != nullptr)
    {
      if (! rb.device)
      {
                    SK_ComPtr <ID3D11Device> pDevice;
        rb.d3d11.immediate_ctx->GetDevice ( &pDevice.p );

        if ( nullptr != pDevice.p )
          rb.setDevice (pDevice);
      }
    }

    if (! rb.device)
    {
      return Normal;
    }
  }

  ///if (SK_D3D11_IsDevCtxDeferred (pDevCtx))
  ///  return false;

  dev_idx = ( dev_idx == NegativeOne ? SK_D3D11_GetDeviceContextHandle (pDevCtx)
            : dev_idx );

  // ImGui gets to pass-through without invoking the hook
  if (SK_ImGui_IsDrawing_OnD3D11Ctx (dev_idx, pDevCtx))
  {
    return Normal;
  }

  using _Registry =
    SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*;

  static auto& shaders =
    SK_D3D11_Shaders;

  static auto& vertex   = shaders->vertex;
  static auto& pixel    = shaders->pixel;
  static auto& geometry = shaders->geometry;
  static auto& hull     = shaders->hull;
  static auto& domain   = shaders->domain;

  uint32_t current_vs = vertex.current.shader   [dev_idx];
  uint32_t current_ps = pixel.current.shader    [dev_idx];
  uint32_t current_gs = geometry.current.shader [dev_idx];
  uint32_t current_hs = hull.current.shader     [dev_idx];
  uint32_t current_ds = domain.current.shader   [dev_idx];

  static auto&
    _reshade_trigger_before =
     reshade_trigger_before.get ();

const
 auto
  TriggerReShade_Before = [&]
  {

    if (              nullptr != SK_ReShade_PresentCallback->fn &&
         (pDevCtx->GetType () != D3D11_DEVICE_CONTEXT_DEFERRED) &&
                             (! shaders->reshade_triggered )       )
    {
      if (_reshade_trigger_before [dev_idx])
      {
        if (ppTLS != nullptr)
        {
          auto flag_result =
            SK_ImGui_FlagDrawing_OnD3D11Ctx (dev_idx);

          SK_ScopedBool auto_bool (flag_result.first);
                                  *flag_result.first = flag_result.second;

          if (*ppTLS == nullptr)
              *ppTLS  = SK_TLS_Bottom ();

          if (*ppTLS != nullptr)
          {
            SK_ScopedBool auto_bool1 (&(*ppTLS)->imgui->drawing);
                                       (*ppTLS)->imgui->drawing = true;

            SK_ScopedBool decl_tex_scope (
              SK_D3D11_DeclareTexInjectScope ()
            );

            shaders->reshade_triggered                = true;
                    _reshade_trigger_before [dev_idx] = false;

            SK_ReShade_PresentCallback->explicit_draw.calls++;
            SK_ReShade_PresentCallback->explicit_draw.src_ctx = pDevCtx;

            SK_ReShade_PresentCallback->fn (
           &SK_ReShade_PresentCallback->explicit_draw
            );
          }
        }
      }
    }
  };


  TriggerReShade_Before ();


  if (SK_D3D11_EnableTracking)
  {
    SK_D3D11_DrawThreads->mark ();

    bool rtv_active = false;

    if (tracked_rtv->active_count [dev_idx] > 0)
    {
      rtv_active = true;

      // Reference tracking is only used when the mod tool window is open,
      //   so skip lengthy work that would otherwise be necessary.
      if ( live_rt_view && SK::ControlPanel::D3D11::show_shader_mod_dlg &&
           SK_ImGui_Visible )
      {
        if (current_vs != 0x00) tracked_rtv->ref_vs.insert (current_vs);
        if (current_ps != 0x00) tracked_rtv->ref_ps.insert (current_ps);
        if (current_gs != 0x00) tracked_rtv->ref_gs.insert (current_gs);
        if (current_hs != 0x00) tracked_rtv->ref_hs.insert (current_hs);
        if (current_ds != 0x00) tracked_rtv->ref_ds.insert (current_ds);
      }
    }
  }

        bool on_top           = false;
        bool wireframe        = false;
  const bool highlight_shader =
                 (dwFrameTime % tracked_shader_blink_duration) >
                               (tracked_shader_blink_duration  / 2);

  auto GetTracker = [&](int i)
  {
    switch (i)
    {
      default: return &((SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&vertex)->tracked;
      case 1:  return &((SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&pixel)->tracked;
      case 2:  return &((SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&geometry)->tracked;
      case 3:  return &((SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&hull)->tracked;
      case 4:  return &((SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&domain)->tracked;
    }
  };

  d3d11_shader_tracking_s* trackers [5] = {
    GetTracker (0), GetTracker (1),
    GetTracker (2),
    GetTracker (3), GetTracker (4)
  };

  for ( auto* tracker : trackers )
  {
    const bool active =
      tracker->active.get (dev_idx);

    if (active)
    {
      if (pDevCtx->GetType () != D3D11_DEVICE_CONTEXT_DEFERRED)
        tracker->use (nullptr);
      else
        tracker->use_cmdlist (nullptr);

      if (tracker->cancel_draws)
      {
        return Skipped;
      }

      if (tracker->wireframe)
      {            wireframe = tracker->highlight_draws  ?
                                        highlight_shader : true; }
      if (tracker->on_top)
      {            on_top    = tracker->highlight_draws  ?
                                        highlight_shader : true; }

      if (! (wireframe || on_top))
      {
        if ( tracker->highlight_draws &&
                      highlight_shader )
        {
          return
            ( highlight_shader ?
                       Skipped : Normal );
        }
      }
    }
  }



#ifndef _WIN64
  static const auto game_id =
    SK_GetCurrentGameID ();

  switch (game_id)
  {
    case SK_GAME_ID::Persona4:
      SK_Persona4_DrawHandler (pDevCtx, current_vs, current_ps);
      break;
    case SK_GAME_ID::ChronoCross:
      SK_CC_DrawHandler       (pDevCtx, current_vs, current_ps);
      break;

    default:
      break;
  }
#endif



  bool
  SK_D3D11_ShouldSkipHUD (void);

  if (SK_D3D11_ShouldSkipHUD ())
  {
    if (   vertex.hud.find (current_vs) !=   vertex.hud.cend () ||
            pixel.hud.find (current_ps) !=    pixel.hud.cend () ||
         geometry.hud.find (current_gs) != geometry.hud.cend () ||
             hull.hud.find (current_hs) !=     hull.hud.cend () ||
           domain.hud.find (current_ds) !=   domain.hud.cend ()  )
    {
      return Skipped;
    }
  }

  SK_GetFramesDrawn ();


  struct filter_cache_s {
    ULONG64 ulLastFrame = 0;
    size_t  count       = 0;
  };

  static filter_cache_s blacklist_cache;
  static filter_cache_s on_top_cache;
  static filter_cache_s wireframe_cache;

  ULONG64 ulThisFrame =
    SK_GetFramesDrawn ();

  if (blacklist_cache.ulLastFrame != ulThisFrame)
  {   blacklist_cache.ulLastFrame  = ulThisFrame;
      blacklist_cache.count =
    vertex.blacklist.size   () + pixel.blacklist.size () +
    geometry.blacklist.size () + hull.blacklist.size  () +
    domain.blacklist.size   ();
  }

  if ( blacklist_cache.count > 0 )
  {
    if (   vertex.blacklist.find (current_vs) !=   vertex.blacklist.cend () ||
            pixel.blacklist.find (current_ps) !=    pixel.blacklist.cend () ||
         geometry.blacklist.find (current_gs) != geometry.blacklist.cend () ||
             hull.blacklist.find (current_hs) !=     hull.blacklist.cend () ||
           domain.blacklist.find (current_ds) !=   domain.blacklist.cend ()  )
    {
      return Skipped;
    }
  }

  static auto& Textures_2D =
    SK_D3D11_Textures->Textures_2D;

  std::pair <_Registry, uint32_t>
    blacklists_by_class        [] =
      { { (_Registry)&vertex,   current_vs },
        { (_Registry)&pixel,    current_ps },
        { (_Registry)&geometry, current_gs } };

  for ( auto& blacklist : blacklists_by_class )
  {
    auto& blacklist_if_texture =
      blacklist.first->blacklist_if_texture;

    // Does the currently bound shader object have any associated
    //   conditional rendering lists ?
    if ( blacklist_if_texture.find (blacklist.second) !=
         blacklist_if_texture.cend (                ) )
    {
      auto& views =
        blacklist.first->current.views [dev_idx];

      for (auto& it2 : views)
      {
        if (it2 == nullptr)
          continue;

        SK_ComPtr <ID3D11Resource> pRes = nullptr;
                it2->GetResource (&pRes);

        D3D11_RESOURCE_DIMENSION rdv  = D3D11_RESOURCE_DIMENSION_UNKNOWN;
                 pRes->GetType (&rdv);

        if (rdv == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
        {
          SK_ComQIPtr <ID3D11Texture2D> pTex (pRes.p);

          auto tex2d =
            Textures_2D.find (pTex.p);

          if ( ( tex2d != Textures_2D.cend () ) &&
                 tex2d->second.crc32c           != 0x0 )
          {
            // Conditional render: One or more textures are not allowed
            auto& skip_if_found =
              blacklist_if_texture [blacklist.second];

            if ( skip_if_found.find (tex2d->second.crc32c) !=
                 skip_if_found.cend (                    ) )
            {
              return Skipped;
            }
          }
        }
      }
    }
  }


  auto _SetupOverrideContext = [&](void) ->
  auto&
  {
    if (*ppTLS == nullptr)
       (*ppTLS) = SK_TLS_Bottom ();

    //// Make sure state cleanup happens on the same context, or deferred
    ////   rendering will make life miserable!
    //(*ppTLS)->render->d3d11->pDevCtx = pDevCtx;
    //
    //return
    //  (*ppTLS)->render->d3d11.get ();

    auto& pTLS_D3D11 =
      (*ppTLS)->render->d3d11.get ();

    // Make sure state cleanup happens on the same context, or deferred
    //   rendering will make life miserable!
    pTLS_D3D11.pDevCtx = pDevCtx;

    return
      pTLS_D3D11;
  };

  bool has_overrides = false;

  if (on_top_cache.ulLastFrame != ulThisFrame)
  {   on_top_cache.ulLastFrame  = ulThisFrame;
      on_top_cache.count =
    vertex.on_top.size   () + pixel.on_top.size () +
    geometry.on_top.size () + hull.on_top.size  () +
    domain.on_top.size   ();
  }

  if ( on_top_cache.count > 0 )
  {
    if (!      on_top)                              on_top   = (
        vertex.on_top.find (current_vs) !=   vertex.on_top.cend () ||
         pixel.on_top.find (current_ps) !=    pixel.on_top.cend () ||
      geometry.on_top.find (current_gs) != geometry.on_top.cend () ||
          hull.on_top.find (current_hs) !=     hull.on_top.cend () ||
        domain.on_top.find (current_ds) !=   domain.on_top.cend () );
  }

  if (on_top)
  {
    SK_ComPtr <ID3D11Device> pDev = nullptr;
       pDevCtx->GetDevice  (&pDev.p);

    if (pDev != nullptr)
    {
      auto& pTLS_d3d11 =
        _SetupOverrideContext ();

      D3D11_DEPTH_STENCIL_DESC desc = {
          TRUE, D3D11_DEPTH_WRITE_MASK_ZERO,
                D3D11_COMPARISON_ALWAYS,
          FALSE,

          D3D11_DEFAULT_STENCIL_READ_MASK,
          D3D11_DEFAULT_STENCIL_WRITE_MASK,

        { D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, // Front
          D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_ALWAYS },
        { D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, // Back
          D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_ALWAYS }
      };

      pDevCtx->OMGetDepthStencilState (
        &pTLS_d3d11.pDepthStencilStateOrig,
        &pTLS_d3d11.StencilRefOrig
      );

      if (pTLS_d3d11.pDepthStencilStateOrig != nullptr)
      {   pTLS_d3d11.pDepthStencilStateOrig->GetDesc (&desc);
          desc.DepthEnable    = TRUE;
          desc.DepthFunc      = D3D11_COMPARISON_ALWAYS;
          desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
          desc.StencilEnable  = FALSE;
      }

      if ( SUCCEEDED ( pDev->CreateDepthStencilState    ( &desc,
                      &pTLS_d3d11.pDepthStencilStateNew )
                     )
         )
      {
        has_overrides = true;

        pDevCtx->OMSetDepthStencilState (
          pTLS_d3d11.pDepthStencilStateNew, 0
        );
      }
    }
  }

  if (wireframe_cache.ulLastFrame != ulThisFrame)
  {   wireframe_cache.ulLastFrame  = ulThisFrame;
      wireframe_cache.count =
    vertex.wireframe.size   () + pixel.wireframe.size () +
    geometry.wireframe.size () + hull.wireframe.size  () +
    domain.wireframe.size   ();
  }

  if ( wireframe_cache.count > 0 )
  {
    if (!      wireframe)                              wireframe   = (
        vertex.wireframe.find (current_vs) !=   vertex.wireframe.cend () ||
         pixel.wireframe.find (current_ps) !=    pixel.wireframe.cend () ||
      geometry.wireframe.find (current_gs) != geometry.wireframe.cend () ||
          hull.wireframe.find (current_hs) !=     hull.wireframe.cend () ||
        domain.wireframe.find (current_ds) !=   domain.wireframe.cend () );
  }

  if (wireframe)
  {
    SK_ComPtr <ID3D11Device> pDev = nullptr;
       pDevCtx->GetDevice  (&pDev.p);

    if (pDev != nullptr)
    {
      auto& pTLS_d3d11 =
        _SetupOverrideContext ();

      pDevCtx->RSGetState (&pTLS_d3d11.pRasterStateOrig);

      D3D11_RASTERIZER_DESC desc = {
        D3D11_FILL_WIREFRAME,
        D3D11_CULL_NONE, FALSE,
        D3D11_DEFAULT_DEPTH_BIAS,
        D3D11_DEFAULT_DEPTH_BIAS_CLAMP,
        D3D11_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
        TRUE, FALSE, FALSE, FALSE
      };

      if (pTLS_d3d11.pRasterStateOrig != nullptr)
      {   pTLS_d3d11.pRasterStateOrig->GetDesc (&desc);
          desc.FillMode      = D3D11_FILL_WIREFRAME;
          desc.CullMode      = D3D11_CULL_NONE;
        //desc.ScissorEnable = FALSE;
      }

      if ( SUCCEEDED ( pDev->CreateRasterizerState ( &desc,
                      &pTLS_d3d11.pRasterStateNew  )
                     )
         )
      {
        has_overrides = true;

        pDevCtx->RSSetState (
          pTLS_d3d11.pRasterStateNew
        );
      }
    }
  }

  if (                 SK_D3D11_EnableTracking    ||
       ( ReadAcquire (&SK_D3D11_CBufferTrackingReqs) > 0 )
     )
  {
    const uint32_t current_shaders [5] =
    {
      current_vs, current_ps,
      current_gs,
      current_hs, current_ds
    };

    static
      concurrency::concurrent_vector <
        d3d11_shader_tracking_s::cbuffer_override_s
      >*
    _global_cbuffer_overrides [] = {
         __SK_D3D11_VertexShader_CBuffer_Overrides.getPtr (),
          __SK_D3D11_PixelShader_CBuffer_Overrides.getPtr ()
    };

    // Temporary storage for any CBuffer that needs staging
    std::array
      <d3d11_shader_tracking_s::cbuffer_override_s*, 128>
        overrides = { nullptr };

    for (int i = 0; i < 5; i++)
    {
      if (current_shaders [i] == 0x00)
        continue;

      auto* tracker =
            trackers [i];

      int num_overrides = 0;

      if (tracker->crc32c == current_shaders [i])
      {
        for ( auto& ovr : tracker->overrides )
        {
          if ( ovr.Enable &&
               ovr.parent == tracker->crc32c )
          {
            if ( ovr.Slot >= 0 &&
                 ovr.Slot <  SK_CBUFFER_SLOTS )
            {
              overrides [num_overrides++] = &ovr;
            }
          }
        }
      }

      if ( i < ( sizeof ( _global_cbuffer_overrides) /
                 sizeof (*_global_cbuffer_overrides) ) )
      {
        if (! _global_cbuffer_overrides [i]->empty ())
        {
          for ( auto& ovr : *(_global_cbuffer_overrides [i]) )
          {
            if ( ovr.parent == current_shaders [i] &&
                 ovr.Enable )
            {
              if ( ovr.Slot >= 0 &&
                   ovr.Slot < SK_CBUFFER_SLOTS )
              {
                overrides [num_overrides++] = &ovr;
              }
            }
          }
        }
      }

      if (num_overrides > 0)
      {
        has_overrides = true;

        auto& pTLS_d3d11 =
          _SetupOverrideContext ();

        SK_ComPtr <ID3D11Buffer> pConstantBuffers [SK_CBUFFER_SLOTS] = { };
        UINT                     pFirstConstant   [SK_CBUFFER_SLOTS] = { },
                                 pNumConstants    [SK_CBUFFER_SLOTS] = { };
        SK_ComPtr <ID3D11Buffer> pConstantCopies  [SK_CBUFFER_SLOTS] = { };

        // Where did this line come from?
        //pTLS_d3d11.pOriginalCBuffers [i][j] = pConstantBuffers [j];

    const
      auto
       _GetConstantBuffers =
        [&](           unsigned int             i,
             _Notnull_ SK_ComPtr <ID3D11Buffer> *ppConstantBuffers,
                       UINT* const               pFirstConstant,
                       UINT                     *pNumConstants )  ->
        void
        {
          SK_ComQIPtr <ID3D11DeviceContext1> pDevCtx1 (pDevCtx);

          // Vtx/Pix/Geo/Hul/Dom/[Com] :: UNDEFINED
          if (i >= 5)
          {
            SK_ReleaseAssert (!"Bad Codepath Encountered"); return;
          }

          if (pDevCtx1 != nullptr)
          {
            auto
              _D3D11DevCtx1_GetConstantBuffers1 =
                std::bind (
                  std::get <1> (GetConstantBuffers_FnTbl [i]),
                    pDevCtx1, _1, _2, _3, _4, _5
                );

            _D3D11DevCtx1_GetConstantBuffers1 (
              0, SK_CBUFFER_SLOTS, &ppConstantBuffers [0].p,
                                pFirstConstant,
                                  pNumConstants
            );

            for ( int j = 0 ; j < SK_CBUFFER_SLOTS ; ++j )
            {
              if (ppConstantBuffers [j] != nullptr)
              {
                // Expected non-D3D11.1+ behavior ( Any game that supplies a different value is using code that REQUIRES the D3D11.1 runtime
                //                                    and will require additional state tracking in future versions of Special K )
                if (pFirstConstant [j] != 0)
                {
                  dll_log->Log ( L"Detected non-zero first constant offset: %lu on CBuffer slot %lu for shader type %lu",
                                   pFirstConstant [j], j, i );
                }

#define _DXVK_COMPAT
#ifndef _DXVK_COMPAT
                // Expected non-D3D11.1+ behavior
                if (pNumConstants [j] != 4096)
                {
                  dll_log->Log ( L"Detected non-4096 num constants: %lu on CBuffer slot %lu for shader type %lu",
                                   pNumConstants [j], j, i );
                }
#endif
              }
            }
          }

          else
          {
            auto
              _D3D11DevCtx_GetConstantBuffers =
                std::bind (
                  std::get <0> (GetConstantBuffers_FnTbl [i]),
                    pDevCtx, _1, _2, _3
                );

            _D3D11DevCtx_GetConstantBuffers (
              0, SK_CBUFFER_SLOTS, &ppConstantBuffers [0].p
            );
          }
        };

    const
      auto
       _SetConstantBuffers =
        [&](           UINT                      i,
             _Notnull_ SK_ComPtr <ID3D11Buffer> *ppConstantBuffers )  ->
        void
        {
          // Vtx/Pix/Geo/Hul/Dom/[Com] :: UNDEFINED
          if (i >= 5)
          {
            SK_ReleaseAssert (!"Bad Codepath Encountered"); return;
          }

          auto
            _D3D11DevCtx_SetConstantBuffers =
              std::bind (SetConstantBuffers_FnTbl [i], pDevCtx,
                            _1, _2, _3);

          _D3D11DevCtx_SetConstantBuffers (
            0, SK_CBUFFER_SLOTS, &ppConstantBuffers [0].p
          );
        };

        _GetConstantBuffers ( i,
           pConstantBuffers,
             pFirstConstant,
               pNumConstants
        );

        for ( UINT j = 0 ; j < SK_CBUFFER_SLOTS ; ++j )
        {
          if (j == 0)
          {
            pTLS_d3d11.empty_cbuffers [i] =
                   ( pConstantBuffers [j] == nullptr );
          }

          if (pConstantBuffers [j] == nullptr) continue;

          D3D11_BUFFER_DESC                   buff_desc  = { };
              pConstantBuffers [j]->GetDesc (&buff_desc);

          buff_desc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
          buff_desc.Usage               = D3D11_USAGE_DYNAMIC;
          buff_desc.BindFlags          |= D3D11_BIND_CONSTANT_BUFFER;
        //buff_desc.MiscFlags           = 0x0;
        //buff_desc.StructureByteStride = 0;

          SK_ComPtr <ID3D11Device> pDev;
          pDevCtx->GetDevice     (&pDev.p);

    const D3D11_MAP                map_type   = D3D11_MAP_WRITE_DISCARD;
          D3D11_MAPPED_SUBRESOURCE mapped_sub = { };
          HRESULT                  hrMap      = E_FAIL;

          bool used       = false;
          UINT start_addr = buff_desc.ByteWidth-1;
          UINT end_addr   = 0;

          for ( int k = 0 ; k < num_overrides ; ++k )
          {
            if (! overrides [k])
              continue;

            auto& ovr =
              *(overrides [k]);

            if ( ovr.Slot == j && ovr.Enable )
            {
              if (ovr.StartAddr < start_addr)
                start_addr = ovr.StartAddr;

              if (ovr.Size + ovr.StartAddr > end_addr)
                end_addr   = ovr.Size + ovr.StartAddr;

              used = true;
            }
          }

          if (used)
          {
            if ( SUCCEEDED ( pDev->CreateBuffer ( &buff_desc,
                                                    nullptr,
                                                      &pConstantCopies [j].p
                                                )
                           )
               )
            {
              if (pConstantBuffers [j] != nullptr)
              {
                if (false)//ReadAcquire (&__SKX_ComputeAntiStall))
                {
                  D3D11_BOX src = { };

                  src.left   = 0;
                  src.right  = buff_desc.ByteWidth;
                  src.top    = 0;
                  src.bottom = 1;
                  src.front  = 0;
                  src.back   = 1;

                  pDevCtx->CopySubresourceRegion (
                    pConstantCopies  [j], 0, 0, 0, 0,
                    pConstantBuffers [j], 0, &src
                  );
                }

                else
                {
                  pDevCtx->CopyResource (
                    pConstantCopies  [j],
                    pConstantBuffers [j]
                  );
                }
              }

              hrMap =
                pDevCtx->Map ( pConstantCopies [j], 0,
                                        map_type, 0x0,
                                          &mapped_sub );
            }

            if (SUCCEEDED (hrMap))
            {
              for ( int k = 0 ; k < num_overrides ; ++k )
              {
                auto& ovr =
                  *(overrides [k]);

                if ( ovr.Slot == j )//&& /*mapped_sub.pData != nullptr &&*/ buff_desc.ByteWidth >= (UINT)ovr.BufferSize )
                {
                  void*   pBase  =
                    ((uint8_t *)mapped_sub.pData   +   ovr.StartAddr);
                  SK_SEH_Guarded_memcpy (
                          pBase,                       ovr.Values,
                                             std::min (ovr.Size, 64ui32)
                                        );
                }
              }

              pDevCtx->Unmap (
                pConstantCopies [j], 0
              );
            }
          }
        }

        _SetConstantBuffers (i, pConstantCopies);

        for ( auto& pConstantCopy : pConstantCopies )
        {
          pConstantCopy = nullptr;
        }
      }
    }
  }

  ////SK_ExecuteReShadeOnReturn easy_reshade (pDevCtx, dev_idx, pTLS);

  if (has_overrides)
    return Override;

  return Normal;
}

void
SK_D3D11_PostDispatch ( ID3D11DeviceContext* pDevCtx,
                        UINT&                dev_idx,
                        SK_TLS*              pTLS )
{
  if (dev_idx == UINT_MAX)
  {   dev_idx =
        SK_D3D11_GetDeviceContextHandle (pDevCtx);
  }

  static auto& compute =
    SK_D3D11_Shaders->compute;

  if (compute.tracked.active.get (dev_idx))
  {
    if (pTLS == nullptr)
        pTLS = SK_TLS_Bottom ();

    auto& pTLS_d3d11 =
      pTLS->render->d3d11.get ();

    if ( pTLS_d3d11.pOriginalCBuffers [5][0] != nullptr ||
         pTLS_d3d11.empty_cbuffers    [5] )
    {
      if ( pTLS_d3d11.empty_cbuffers  [5] )
           pTLS_d3d11.empty_cbuffers  [5] = false;

      pDevCtx->CSSetConstantBuffers (
        0, SK_CBUFFER_SLOTS,
          &(pTLS_d3d11.pOriginalCBuffers [5][0]).p
      );

      for (int j = 0; j < SK_CBUFFER_SLOTS; ++j)
      {
        pTLS_d3d11.pOriginalCBuffers [5][j] = nullptr;
      }
    }
  }
}

bool
SK_D3D11_DispatchHandler ( ID3D11DeviceContext* pDevCtx,
                           UINT&                dev_idx,
                           SK_TLS**             ppTLS )
{
  SK_D3D11_DispatchThreads->mark ();

  dev_idx =
  dev_idx == (UINT)-1 ? SK_D3D11_GetDeviceContextHandle (pDevCtx) :
  dev_idx;

  static auto& compute =
    SK_D3D11_Shaders->compute;

  if (SK_D3D11_EnableTracking)
  {
    bool rtv_active = false;

    if (tracked_rtv->active_count [dev_idx] > 0)
    {
      rtv_active = true;

      if (compute.current.shader [dev_idx] != 0x00)
      {
        tracked_rtv->ref_cs.insert (
          compute.current.shader [dev_idx]
        );
      }
    }

    if (compute.tracked.active.get (dev_idx)) {
        compute.tracked.use        (nullptr);
    }
  }

  const bool highlight_shader =
    ( dwFrameTime % tracked_shader_blink_duration >
                  ( tracked_shader_blink_duration / 2 ) );

  const uint32_t current_cs =
    compute.current.shader [dev_idx];

  if ( compute.blacklist.find (current_cs) !=
       compute.blacklist.cend (          )  )
  {
    return true;
  }

  d3d11_shader_tracking_s*
    tracker = &compute.tracked;

  if (tracker->crc32c == current_cs)
  {
    if ( compute.tracked.clear_output.load () &&
           start_uav >= 0                     &&
           start_uav < D3D11_PS_CS_UAV_REGISTER_COUNT )
    {
      ID3D11UnorderedAccessView* pUAVs [D3D11_PS_CS_UAV_REGISTER_COUNT]={};

      pDevCtx->CSGetUnorderedAccessViews (
        0, D3D11_PS_CS_UAV_REGISTER_COUNT,
                     &pUAVs [0]
      );

      for ( UINT i = start_uav ;
                 i < std::min (
                       start_uav + uav_count,
                       (UINT)D3D11_PS_CS_UAV_REGISTER_COUNT
                     );
               ++i )
      {
        if (pUAVs [i] != nullptr)
        {
          D3D11_UNORDERED_ACCESS_VIEW_DESC
                               uav_desc = { };
          pUAVs [i]->GetDesc (&uav_desc);

          if (uav_desc.ViewDimension == D3D11_UAV_DIMENSION_TEXTURE2D)
          {
            pDevCtx->ClearUnorderedAccessViewFloat ( pUAVs [i],
              uav_clear
            );
          }

          pUAVs [i]->Release ();
          pUAVs [i] = nullptr;
        }
      }
    }

    if (compute.tracked.highlight_draws && highlight_shader) return true;
    if (compute.tracked.cancel_draws)                        return true;

    std::vector <d3d11_shader_tracking_s::cbuffer_override_s> overrides;
    size_t used_slots [SK_CBUFFER_SLOTS] = { };

    for ( auto& ovr : tracker->overrides )
    {
      if ( ovr.Enable &&
           ovr.parent == tracker->crc32c )
      {
        if ( ovr.Slot >= 0 &&
             ovr.Slot <  SK_CBUFFER_SLOTS )
        {
                      used_slots [ovr.Slot] =
                                  ovr.BufferSize;
          overrides.emplace_back (ovr);
        }
      }
    }

    if (! overrides.empty ())
    {
      SK_ComPtr <ID3D11Buffer> pConstantBuffers [SK_CBUFFER_SLOTS] = { };
      SK_ComPtr <ID3D11Buffer> pConstantCopies  [SK_CBUFFER_SLOTS] = { };

      pDevCtx->CSGetConstantBuffers (
        0, SK_CBUFFER_SLOTS, &pConstantBuffers [0].p );
      pDevCtx->CSSetConstantBuffers (
        0, SK_CBUFFER_SLOTS, &pConstantCopies  [0].p );

      if (*ppTLS == nullptr)
          *ppTLS = SK_TLS_Bottom ();

      auto& pTLS_d3d11 =
         (*ppTLS)->render->d3d11.get ();

      for ( UINT j = 0 ; j < SK_CBUFFER_SLOTS ; j++ )
      {
        pTLS_d3d11.pOriginalCBuffers [5][j] = pConstantBuffers [j];

        if (j == 0)
        {
          pTLS_d3d11.empty_cbuffers [5] =
                 ( pConstantBuffers [j] == nullptr );
        }

            pConstantCopies  [j]  = nullptr;
        if (pConstantBuffers [j] == nullptr && (! used_slots [j])) continue;

        D3D11_BUFFER_DESC                   buff_desc  = { };

        if (pConstantBuffers [j])
            pConstantBuffers [j]->GetDesc (&buff_desc);

        buff_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        buff_desc.Usage          = D3D11_USAGE_DYNAMIC;
        buff_desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;

        SK_ComPtr <ID3D11Device> pDev;
        pDevCtx->GetDevice     (&pDev.p);

  const D3D11_MAP                map_type   = D3D11_MAP_WRITE_NO_OVERWRITE;
        D3D11_MAPPED_SUBRESOURCE mapped_sub = { };
        HRESULT                  hrMap      = E_FAIL;

        bool used       = false;
        UINT start_addr = buff_desc.ByteWidth-1;
        UINT end_addr   = 0;

        for ( auto& ovr : overrides )
        {
          if ( ovr.Slot == j && ovr.Enable )
          {
            if (ovr.StartAddr < start_addr)
              start_addr = ovr.StartAddr;

            if (ovr.Size + ovr.StartAddr > end_addr)
              end_addr   = ovr.Size + ovr.StartAddr;

            used = true;
          }
        }

        if (used)
        {
          if ( SUCCEEDED ( pDev->CreateBuffer ( &buff_desc,
                             nullptr, &pConstantCopies [j].p )
                         )
             )
          {
            if (pConstantBuffers [j] != nullptr)
            {
              if (ReadAcquire (&__SKX_ComputeAntiStall) != 0)
              {
                D3D11_BOX src = { };

                src.left   = 0;
                src.right  = buff_desc.ByteWidth;
                src.top    = 0;
                src.bottom = 1;
                src.front  = 0;
                src.back   = 1;

                pDevCtx->CopySubresourceRegion (
                  pConstantCopies  [j], 0, 0, 0, 0,
                  pConstantBuffers [j], 0, &src );
              }

              else
              {
                pDevCtx->CopyResource (
                  pConstantCopies  [j],
                  pConstantBuffers [j]
                );
              }
            }

            hrMap =
                pDevCtx->Map ( pConstantCopies [j], 0,
                                        map_type, 0x0,
                                          &mapped_sub );
          }

          if (SUCCEEDED (hrMap))
          {
            for ( auto& ovr : overrides )
            {
              if ( ovr.Slot == j && mapped_sub.pData != nullptr && buff_desc.ByteWidth >= (UINT)ovr.BufferSize )
              {
                void*   pBase  =
                    ((uint8_t *)mapped_sub.pData   +   ovr.StartAddr);
                  SK_SEH_Guarded_memcpy (
                          pBase,                       ovr.Values,
                                             std::min (ovr.Size, 64ui32)
                                        );
              }
            }

            pDevCtx->Unmap (
              pConstantCopies [j], 0
            );
          }

          else if (pConstantCopies [j] != nullptr)
          {
            dll_log->Log (L"Failure To Copy Resource");
            pConstantCopies [j] = nullptr;
          }
        }
      }

      pDevCtx->CSSetConstantBuffers (
        0, SK_CBUFFER_SLOTS, &pConstantCopies [0].p
      );

      for (auto& pConstantCopy : pConstantCopies)
      {
        pConstantCopy = nullptr;
      }
    }
  }

  return false;
}

DEFINE_GUID(IID_ID3D11On12Device,0x85611e73,0x70a9,0x490e,0x96,0x14,0xa9,0xe3,0x02,0x77,0x79,0x04);

bool
SK_D3D11_IgnoreWrappedOrDeferred ( bool                 bWrapped,
                                   ID3D11DeviceContext* pDevCtx )
{
         const bool  bDeferred  =   SK_D3D11_IsDevCtxDeferred (pDevCtx);
  static const bool& bIsolation = config.render.dxgi.deferred_isolation;

  if (  (  bDeferred  && (! bIsolation) ) ||
      ( (! bDeferred) && (  bWrapped  )
                      && (! bIsolation) )
     )
  {
    return
      true;
  }


  static auto& rb =
    SK_GetCurrentRenderBackend ();

  SK_ComPtr <ID3D11Device> pDevice;

  if ((! bWrapped) && pDevCtx != rb.d3d11.immediate_ctx)
  {
    pDevCtx->GetDevice (&pDevice.p);

    // TOO HIGH OVERHEAD: Use direct compare and expect a few misses
    //if (! rb.getDevice <ID3D11Device> ().IsEqualObject (pDevice))
    if (rb.device.p != pDevice.p)
    {
      if (config.system.log_level > 0)
      {
        SK_ReleaseAssert (!"Hooked command ignored because it happened on the wrong device");
      }

      return true;
    }
  }


  //
  // Handle D3D11On12, but only if the active API is not already D3D11 :)
  //
  if (rb.api != SK_RenderAPI::D3D11)
  {
    SK_ComPtr <IUnknown> pD3D11On12Device;
    if (pDevice)
        pDevice->QueryInterface (
          IID_ID3D11On12Device,
    (void **)&pD3D11On12Device.p);

    if (pD3D11On12Device.p != nullptr)
    {
    //SK_ReleaseAssert (!"D3D11On12");
      return true;
    }
  }


  return
    false;
}



void
STDMETHODCALLTYPE
SK_D3D11_Dispatch_Impl (
  _In_ ID3D11DeviceContext *pDevCtx,
  _In_ UINT                 ThreadGroupCountX,
  _In_ UINT                 ThreadGroupCountY,
  _In_ UINT                 ThreadGroupCountZ,
       BOOL                 bWrapped,
       UINT                 dev_idx )
{
  SK_WRAP_AND_HOOK


  auto _Finish =
   [&](void) -> void
    {
      return
        bWrapped ?
          pDevCtx->Dispatch ( ThreadGroupCountX,
                                ThreadGroupCountY,
                                  ThreadGroupCountZ )
                 :
          D3D11_Dispatch_Original ( pDevCtx,
                                      ThreadGroupCountX,
                                        ThreadGroupCountY,
                                          ThreadGroupCountZ );
    };

  bool early_out =
    ( SK_D3D11_IgnoreWrappedOrDeferred (bWrapped, pDevCtx) ||
    (! bMustNotIgnore) );

  if (early_out)
  {
    return
      _Finish ();
  }

  if (! SK_D3D11_ShouldTrackComputeDispatch (
          pDevCtx,
            SK_D3D11DispatchType::Standard,
              dev_idx
        )
     )
  {
    return
      _Finish ();
  }

  SK_TLS* pTLS = nullptr;

  if (! SK_D3D11_DispatchHandler (pDevCtx, dev_idx, &pTLS))
  {
    _Finish ();
  }
  SK_D3D11_PostDispatch (pDevCtx, dev_idx, pTLS);
}

void
STDMETHODCALLTYPE
SK_D3D11_DispatchIndirect_Impl (
  _In_ ID3D11DeviceContext *pDevCtx,
  _In_ ID3D11Buffer        *pBufferForArgs,
  _In_ UINT                 AlignedByteOffsetForArgs,
       BOOL                 bWrapped,
       UINT                 dev_idx )
{
  SK_WRAP_AND_HOOK

  auto _Finish =
  [&](void) -> void
   {
     return
       bWrapped ?
         pDevCtx->DispatchIndirect (
                     pBufferForArgs,
           AlignedByteOffsetForArgs
         )
                :
         D3D11_DispatchIndirect_Original (
           pDevCtx,    pBufferForArgs,
             AlignedByteOffsetForArgs
         );
   };

  bool early_out =
    ( SK_D3D11_IgnoreWrappedOrDeferred (bWrapped, pDevCtx) ||
    (! bMustNotIgnore) );

  if (early_out)
  {
    return
      _Finish ();
  }

  if (! SK_D3D11_ShouldTrackComputeDispatch (
          pDevCtx, SK_D3D11DispatchType::Indirect, dev_idx
                                            )
     )
  {
    return
      _Finish ();
  }

  SK_TLS* pTLS = nullptr;

  if (! SK_D3D11_DispatchHandler (pDevCtx, dev_idx, &pTLS))
  {
    _Finish ();
  }
  SK_D3D11_PostDispatch          (pDevCtx, dev_idx, pTLS);
}

void
STDMETHODCALLTYPE
SK_D3D11_DrawAuto_Impl (_In_ ID3D11DeviceContext *pDevCtx, BOOL bWrapped, UINT dev_idx)
{
  SK_WRAP_AND_HOOK

  auto _Finish =
  [&](void) -> void
   {
     return
       bWrapped ?
         pDevCtx->DrawAuto ()
                :
       D3D11_DrawAuto_Original (pDevCtx);
   };

  bool early_out =
    ( SK_D3D11_IgnoreWrappedOrDeferred (bWrapped, pDevCtx) ||
    (! bMustNotIgnore) );

  if (early_out)
  {
    return
      _Finish ();
  }

  SK_TLS *pTLS = nullptr;

  if (! SK_D3D11_ShouldTrackDrawCall (pDevCtx, SK_D3D11DrawType::Auto))
  {
    return
      _Finish ();
  }

  auto draw_action =
    SK_D3D11_DrawHandler (pDevCtx, &pTLS, dev_idx);

  if (draw_action == Skipped)
    return;

  _Finish ();

  if (draw_action == Override)
    SK_D3D11_PostDraw (dev_idx, pTLS);
}

void
STDMETHODCALLTYPE
SK_D3D11_Draw_Impl (ID3D11DeviceContext* pDevCtx,
                    UINT                 VertexCount,
                    UINT                 StartVertexLocation,
                    bool                 bWrapped,
                    UINT                 dev_idx )
{
  SK_WRAP_AND_HOOK

  auto _Finish =
  [&](void)-> void
   {
     return
       bWrapped ?
         pDevCtx->Draw ( VertexCount,
                           StartVertexLocation )
               :
       D3D11_Draw_Original ( pDevCtx,
                               VertexCount,
                                 StartVertexLocation );
   };


  //--------------------------------------------------
      //UINT dev_idx = SK_D3D11_GetDeviceContextHandle (this);
    //if (SK_D3D11_DrawCallFilter ( VertexCount, VertexCount,
    //    SK_D3D11_Shaders->vertex.current.shader [dev_idx]) )
    //{
    //  return;
    //}

    ////extern volatile LONG __SK_SHENMUE_FullAspectCutscenes;
    ////if (ReadAcquire (&__SK_SHENMUE_FullAspectCutscenes))
    ////{
    ////  if (SK_D3D11_Shaders->pixel.current.shader [dev_idx] == 0x29b11b07)
    ////  {
    ////    if (VertexCount == 30)
    ////    {
    ////      dll_log->Log ( L"Vertex Count: %lu, Start Vertex Loc: %lu",
    ////                     VertexCount, StartVertexLocation );
    ////      return;
    ////    }
    ////  }
    ////}

#ifdef _M_AMD64
    static const auto game_id =
      SK_GetCurrentGameID ();

    switch (game_id)
    {
      case SK_GAME_ID::Tales_of_Vesperia:
      {
        extern bool SK_TVFix_SharpenShadows (void);

        if (    SK_TVFix_SharpenShadows   (       ) &&
             (! SK_D3D11_IsDevCtxDeferred (pDevCtx)    )
           )
        {
          if (dev_idx == UINT_MAX)
          {
            dev_idx =
              SK_D3D11_GetDeviceContextHandle (pDevCtx);
          }

          uint32_t ps_crc32 =
            SK_D3D11_Shaders->pixel.current.shader [dev_idx];

          if (ps_crc32 == 0x84da24a5)
          {
            SK_ComPtr <ID3D11ShaderResourceView>  pSRV = nullptr;
            pDevCtx->PSGetShaderResources (0, 1, &pSRV.p);

            if (pSRV != nullptr)
            {
              SK_ComPtr <ID3D11Resource> pRes;
                     pSRV->GetResource (&pRes.p);

              SK_ComQIPtr <ID3D11Texture2D> pTex (pRes);

              if (pTex != nullptr)
              {
                D3D11_TEXTURE2D_DESC texDesc = { };
                pTex->GetDesc      (&texDesc);

                if (  texDesc.Width == texDesc.Height &&
                    ( texDesc.Width == 64  ||
                      texDesc.Width == 128 ||
                      texDesc.Width == 256 ) )
                {
                  return;
                }
              }
            }
          }
        }
      } break;
    }
#endif
    // -------------------------------------------------------

  static auto& rb =
    SK_GetCurrentRenderBackend ();

  SK_TLS *pTLS  = nullptr;

  if (ReadAcquire (&SK_D3D11_DrawTrackingReqs) > 0)
  {
    if (dev_idx == UINT_MAX)
    {
      dev_idx =
        SK_D3D11_GetDeviceContextHandle (pDevCtx);
    }

    if (SK_D3D11_DrawCallFilter ( 0, VertexCount,
                    SK_D3D11_Shaders->vertex.current.shader [dev_idx]) )
    {
      return;
    }
  }


  bool early_out =
    ( SK_D3D11_IgnoreWrappedOrDeferred (bWrapped, pDevCtx) ||
    (! bMustNotIgnore) );

  if (early_out)
  {
    return _Finish ();
  }

  // Render-state tracking needs to be forced-on for the
  //   Steam Overlay HDR fix to work.
  if ( rb.isHDRCapable ()  &&
       rb.isHDRActive  () )
  {
    if (dev_idx == UINT_MAX)
    {
      dev_idx =
        SK_D3D11_GetDeviceContextHandle (pDevCtx);
    }

    uint32_t vs_crc =
      SK_D3D11_Shaders->vertex.current.shader [dev_idx];

#define STEAM_OVERLAY_VS_CRC32C   0xf48cf597
#define DISCORD_OVERLAY_VS_CRC32C 0x085ee17b
#define RTSS_OVERLAY_VS_CRC32C    0x671afc2f
#define EPIC_OVERLAY_VS_CRC32C    0xa7ee5199

    if ( STEAM_OVERLAY_VS_CRC32C ==
                       vs_crc )
    {
      if ( SUCCEEDED (
             SK_D3D11_InjectSteamHDR ( pDevCtx, VertexCount,
                                         StartVertexLocation,
                                           D3D11_Draw_Original )
           )
         )
      {
        return;
      }
    }

    else if ( DISCORD_OVERLAY_VS_CRC32C == vs_crc ||
                 RTSS_OVERLAY_VS_CRC32C == vs_crc ||
                 EPIC_OVERLAY_VS_CRC32C == vs_crc )
    {
      if ( SUCCEEDED (
             SK_D3D11_InjectGenericHDROverlay ( pDevCtx, VertexCount,
                                                  StartVertexLocation,
                                                    vs_crc, D3D11_Draw_Original )
           )
         )
      {
        return;
      }
    }
  }

  if (! SK_D3D11_ShouldTrackDrawCall ( pDevCtx,
            SK_D3D11DrawType::PrimList, dev_idx ))
  {
    return
      _Finish ();
  }

  auto draw_action =
    SK_D3D11_DrawHandler (pDevCtx, &pTLS, dev_idx);

  if (draw_action == Skipped)
    return;

  _Finish ();

  if (draw_action == Override)
    SK_D3D11_PostDraw (dev_idx, pTLS);
}

void
STDMETHODCALLTYPE
SK_D3D11_DrawIndexed_Impl (
  _In_ ID3D11DeviceContext *pDevCtx,
  _In_ UINT                 IndexCount,
    _In_ UINT                 StartIndexLocation,
  _In_ INT                  BaseVertexLocation,
       BOOL                 bWrapped,
       UINT                 dev_idx )
{
  SK_WRAP_AND_HOOK

  auto _Finish =
   [&](void) -> void
    {
      return
        bWrapped ?
          pDevCtx->DrawIndexed ( IndexCount, StartIndexLocation,
                                 BaseVertexLocation )
                 :
          D3D11_DrawIndexed_Original ( pDevCtx,            IndexCount,
                                       StartIndexLocation, BaseVertexLocation );
    };

  bool early_out =
    ( SK_D3D11_IgnoreWrappedOrDeferred (bWrapped, pDevCtx) ||
    (! bMustNotIgnore) );

  if (early_out)
  {
    return _Finish ();
  }

  //-----------------------------------------
#ifdef _M_AMD64
  static bool bIsShenmue =
    (SK_GetCurrentGameID () == SK_GAME_ID::Shenmue);

  static bool bIsShenmueOrDQXI =
    bIsShenmue ||
      (SK_GetCurrentGameID () == SK_GAME_ID::DragonQuestXI);

  if (bIsShenmue)
  {
    extern volatile LONG  __SK_SHENMUE_FinishedButNotPresented;
    InterlockedExchange (&__SK_SHENMUE_FinishedButNotPresented, 1L);
  }
#endif

  if (ReadAcquire (&SK_D3D11_DrawTrackingReqs) > 0)
  {
    if (dev_idx == UINT_MAX)
    {
      dev_idx =
        SK_D3D11_GetDeviceContextHandle (pDevCtx);
    }

    if (SK_D3D11_DrawCallFilter ( IndexCount, IndexCount,
        SK_D3D11_Shaders->vertex.current.shader [dev_idx]) )
    {
      return;
    }
  }
  //------

  static auto& rb =
    SK_GetCurrentRenderBackend ();

  // Render-state tracking needs to be forced-on for the
  //   Steam Overlay HDR fix to work.
  if ( rb.isHDRCapable ()  &&
       rb.isHDRActive  () )
  {
#define EPIC_OVERLAY_VS_CRC32C  0xa7ee5199
#define UPLAY_OVERLAY_PS_CRC32C 0x35ae281c

    if (dev_idx == UINT_MAX)
    {
      dev_idx =
        SK_D3D11_GetDeviceContextHandle (pDevCtx);
    }

    if ( UPLAY_OVERLAY_PS_CRC32C ==
           SK_D3D11_Shaders->pixel.current.shader [dev_idx] )
    {
      if ( SUCCEEDED (
             SK_D3D11_Inject_uPlayHDR ( pDevCtx, IndexCount,
                                         StartIndexLocation,
                                           BaseVertexLocation,
                                             D3D11_DrawIndexed_Original )
           )
         )
      {
        return;
      }
    }
    
    else if ( EPIC_OVERLAY_VS_CRC32C ==
                SK_D3D11_Shaders->vertex.current.shader [dev_idx] )
    {
      if ( SUCCEEDED (
             SK_D3D11_Inject_EpicHDR ( pDevCtx, IndexCount,
                                         StartIndexLocation,
                                           BaseVertexLocation,
                                             D3D11_DrawIndexed_Original )
           )
         )
      {
        return;
      }
    }
  }

  if (! SK_D3D11_ShouldTrackDrawCall  (pDevCtx, SK_D3D11DrawType::Indexed))
  {
    return _Finish ();
  }

  SK_TLS* pTLS = nullptr;

  auto draw_action =
    SK_D3D11_DrawHandler (pDevCtx, &pTLS, dev_idx);

  if (draw_action == Skipped)
    return;

  _Finish ();

  if (draw_action == Override)
    SK_D3D11_PostDraw (dev_idx, pTLS);
}

void
STDMETHODCALLTYPE
SK_D3D11_DrawIndexedInstanced_Impl (
  _In_ ID3D11DeviceContext *pDevCtx,
  _In_ UINT                 IndexCountPerInstance,
  _In_ UINT                 InstanceCount,
  _In_ UINT                 StartIndexLocation,
  _In_ INT                  BaseVertexLocation,
  _In_ UINT                 StartInstanceLocation,
       BOOL                 bWrapped,
       UINT                 dev_idx )
{
  SK_WRAP_AND_HOOK

  auto _Finish =
   [&](void) -> void
    {
      return
        bWrapped ?
          pDevCtx->DrawIndexedInstanced ( IndexCountPerInstance,
                                          InstanceCount, StartIndexLocation,
                                          BaseVertexLocation, StartInstanceLocation )
                 :
          D3D11_DrawIndexedInstanced_Original ( pDevCtx, IndexCountPerInstance,
                                                InstanceCount, StartIndexLocation,
                                                BaseVertexLocation, StartInstanceLocation );
    };

  bool early_out =
    ( SK_D3D11_IgnoreWrappedOrDeferred (bWrapped, pDevCtx) ||
    (! bMustNotIgnore) );

  if (early_out)
  {
    return
      _Finish ();
  }

  if (! SK_D3D11_ShouldTrackDrawCall (pDevCtx, SK_D3D11DrawType::IndexedInstanced))
  {
    return
      _Finish ();
  }

  SK_TLS *pTLS = nullptr;

  auto draw_action =
    SK_D3D11_DrawHandler (pDevCtx, &pTLS, dev_idx);

  if (draw_action == Skipped)
    return;

  _Finish ();

  if (draw_action == Override)
    SK_D3D11_PostDraw (dev_idx, pTLS);
}

void
STDMETHODCALLTYPE
SK_D3D11_DrawIndexedInstancedIndirect_Impl (
  _In_ ID3D11DeviceContext *pDevCtx,
  _In_ ID3D11Buffer        *pBufferForArgs,
  _In_ UINT                 AlignedByteOffsetForArgs,
       BOOL                 bWrapped,
       UINT                 dev_idx )
{
  SK_WRAP_AND_HOOK

  auto _Finish =
  [&](void) -> void
   {
     return
       bWrapped ?
         pDevCtx->DrawIndexedInstancedIndirect (pBufferForArgs, AlignedByteOffsetForArgs)
                :
       D3D11_DrawIndexedInstancedIndirect_Original ( pDevCtx,
                                                       pBufferForArgs,
                                                         AlignedByteOffsetForArgs );
   };

  bool early_out =
    ( SK_D3D11_IgnoreWrappedOrDeferred (bWrapped, pDevCtx) ||
    (! bMustNotIgnore) );

  if (early_out)
  {
    return
      _Finish ();
  }

  if (! SK_D3D11_ShouldTrackDrawCall (pDevCtx, SK_D3D11DrawType::IndexedInstancedIndirect))
  {
    return
      _Finish ();
  }

  SK_TLS *pTLS = nullptr;

  auto draw_action =
    SK_D3D11_DrawHandler (pDevCtx, &pTLS, dev_idx);

  if (draw_action == Skipped)
    return;

  _Finish ();

  if (draw_action == Override)
    SK_D3D11_PostDraw (dev_idx, pTLS);
}

void
STDMETHODCALLTYPE
SK_D3D11_DrawInstanced_Impl (
  _In_ ID3D11DeviceContext *pDevCtx,
  _In_ UINT                 VertexCountPerInstance,
  _In_ UINT                 InstanceCount,
  _In_ UINT                 StartVertexLocation,
  _In_ UINT                 StartInstanceLocation,
       BOOL                 bWrapped,
       UINT                 dev_idx )
{
  SK_WRAP_AND_HOOK

  auto _Finish =
  [&](void) -> void
   {
     return
       bWrapped ?
         pDevCtx->DrawInstanced ( VertexCountPerInstance,
                                  InstanceCount, StartVertexLocation,
                                                 StartInstanceLocation )
                :
         D3D11_DrawInstanced_Original ( pDevCtx,       VertexCountPerInstance,
                                        InstanceCount, StartVertexLocation,
                                        StartInstanceLocation );
   };

  bool early_out =
    ( SK_D3D11_IgnoreWrappedOrDeferred (bWrapped, pDevCtx) ||
    (! bMustNotIgnore) );

  if (early_out)
  {
    return
      _Finish ();
  }

  if (! SK_D3D11_ShouldTrackDrawCall (pDevCtx, SK_D3D11DrawType::Instanced))
  {
    return
      _Finish ();
  }

  SK_TLS *pTLS = nullptr;

  auto draw_action =
    SK_D3D11_DrawHandler (pDevCtx, &pTLS, dev_idx);

  if (draw_action == Skipped)
    return;

  _Finish ();

  if (draw_action == Override)
    SK_D3D11_PostDraw (dev_idx, pTLS);
}

void
STDMETHODCALLTYPE
SK_D3D11_DrawInstancedIndirect_Impl (
  _In_ ID3D11DeviceContext *pDevCtx,
  _In_ ID3D11Buffer        *pBufferForArgs,
  _In_ UINT                 AlignedByteOffsetForArgs,
       BOOL                 bWrapped,
       UINT                 dev_idx )
{
  SK_WRAP_AND_HOOK

  auto _Finish =
  [&](void) -> void
   {
     return
       bWrapped ?
         pDevCtx->DrawInstancedIndirect (pBufferForArgs, AlignedByteOffsetForArgs)
                :
       D3D11_DrawInstancedIndirect_Original ( pDevCtx, pBufferForArgs,
                                             AlignedByteOffsetForArgs );
   };

  bool early_out =
    ( SK_D3D11_IgnoreWrappedOrDeferred (bWrapped, pDevCtx) ||
    (! bMustNotIgnore) );

  if (early_out)
  {
    return
      _Finish ();
  }

  if (! SK_D3D11_ShouldTrackDrawCall (pDevCtx, SK_D3D11DrawType::InstancedIndirect))
  {
    return
      _Finish ();
  }

  SK_TLS *pTLS = nullptr;

  auto draw_action =
    SK_D3D11_DrawHandler (pDevCtx, &pTLS, dev_idx);

  if (draw_action == Skipped)
    return;

  _Finish ();

  if (draw_action == Override)
    SK_D3D11_PostDraw (dev_idx, pTLS);
}

void
STDMETHODCALLTYPE
SK_D3D11_OMSetRenderTargetsAndUnorderedAccessViews_Impl (
  _In_                                        ID3D11DeviceContext              *pDevCtx,
  _In_                                        UINT                              NumRTVs,
  _In_reads_opt_ (NumRTVs)                    ID3D11RenderTargetView    *const *ppRenderTargetViews,
  _In_opt_                                    ID3D11DepthStencilView           *pDepthStencilView,
  _In_range_ (0, D3D11_1_UAV_SLOT_COUNT - 1)  UINT                              UAVStartSlot,
  _In_                                        UINT                              NumUAVs,
  _In_reads_opt_ (NumUAVs)                    ID3D11UnorderedAccessView *const *ppUnorderedAccessViews,
  _In_reads_opt_ (NumUAVs)              const UINT                             *pUAVInitialCounts,
                                              BOOL                              bWrapped,
                                              UINT                              dev_idx )
{
  ID3D11DepthStencilView *pDSV =
       pDepthStencilView;

  SK_WRAP_AND_HOOK

  auto _Finish = [&](void) ->
  void
  {
    // This SEH translator hack is necessary for Yakuza's crazy engine, it
    //   can recover from what will effectively be no render target bound,
    //     and SK is capable of purging the memory it incorrectly deleted
    //       from its caches after the first exception is raised.
    auto orig_se =
      SK_SEH_ApplyTranslator (
        SK_FilteringStructuredExceptionTranslator (
          EXCEPTION_ACCESS_VIOLATION
        )
      );
    try
    {
      bWrapped ?
        pDevCtx->OMSetRenderTargetsAndUnorderedAccessViews (
          NumRTVs, ppRenderTargetViews, pDSV, UAVStartSlot,
          NumUAVs, ppUnorderedAccessViews, pUAVInitialCounts )
               :
        D3D11_OMSetRenderTargetsAndUnorderedAccessViews_Original ( pDevCtx,
          NumRTVs, ppRenderTargetViews, pDSV, UAVStartSlot,
          NumUAVs, ppUnorderedAccessViews, pUAVInitialCounts );
    }
    catch (const SK_SEH_IgnoredException&) {};
    SK_SEH_RemoveTranslator (orig_se);
  };

  bool early_out =
    ( SK_D3D11_IgnoreWrappedOrDeferred (bWrapped, pDevCtx) ||
    (! bMustNotIgnore) );

  if (early_out)
  {
    return
      _Finish ();
  }

  if ( dev_idx == UINT_MAX)
  {    dev_idx  =
         SK_D3D11_GetDeviceContextHandle (pDevCtx);
  }

  if (SK_ImGui_IsDrawing_OnD3D11Ctx (dev_idx, pDevCtx))
  {
    return
      _Finish ();
  }

#ifdef _M_AMD64
  static bool yakuza = ( SK_GetCurrentGameID () == SK_GAME_ID::Yakuza0 ||
                         SK_GetCurrentGameID () == SK_GAME_ID::YakuzaKiwami ||
                         SK_GetCurrentGameID () == SK_GAME_ID::YakuzaKiwami2 ||
                         SK_GetCurrentGameID () == SK_GAME_ID::YakuzaUnderflow );
  extern bool __SK_Yakuza_TrackRTVs;
#endif

  // ImGui gets to pass-through without invoking the hook
  if (
#ifdef _M_AMD64
    (yakuza && (! __SK_Yakuza_TrackRTVs)) ||
#endif
    (! SK_D3D11_ShouldTrackRenderOp (pDevCtx, dev_idx)))
  {
    for (auto& i : tracked_rtv->active       [dev_idx]) i.store (false);
                   tracked_rtv->active_count [dev_idx] = 0;

    return
      _Finish ();
  }

  _Finish ();

  if (NumRTVs > 0)
  {
    if (ppRenderTargetViews != nullptr)
    {
      static auto&                      _want_rt_list =
        SK_D3D11_KnownTargets::_mod_tool_wants;

      auto&                                rt_views =
        (*SK_D3D11_RenderTargets)[dev_idx].rt_views;

      const auto* tracked_rtv_res =
        static_cast <ID3D11RenderTargetView *> (
          ReadPointerAcquire ((volatile PVOID *)&tracked_rtv->resource)
          );

      for (UINT i = 0; i < NumRTVs; i++)
      {
        if (_want_rt_list && ppRenderTargetViews [i] != nullptr)
        {  rt_views.emplace (ppRenderTargetViews [i]);         }

        const bool active_before =
          tracked_rtv->active_count [dev_idx] > 0 ?
          tracked_rtv->active       [dev_idx][i].load ()
                                                  : false;

        const bool active =
          ( tracked_rtv_res == ppRenderTargetViews [i] ) ?
                                                    true :
                                                    false;

        if (active_before != active)
        {
          tracked_rtv->active [dev_idx][i] = active;

          if      (             active                    ) tracked_rtv->active_count [dev_idx]++;
          else if (tracked_rtv->active_count [dev_idx] > 0) tracked_rtv->active_count [dev_idx]--;
        }
      }
    }

#ifdef _PERSIST_DS_VIEWS
    if (pDSV != nullptr)
    {
      auto& ds_views =
        SK_D3D11_RenderTargets [dev_idx].ds_views;

            ds_views.emplace (pDepthStencilView);
    }
#endif
  }
}

void
STDMETHODCALLTYPE
SK_D3D11_OMSetRenderTargets_Impl (
         ID3D11DeviceContext           *pDevCtx,
_In_     UINT                           NumViews,
_In_opt_ ID3D11RenderTargetView *const *ppRenderTargetViews,
_In_opt_ ID3D11DepthStencilView        *pDepthStencilView,
         BOOL                           bWrapped,
         UINT                           dev_idx )
{
  ID3D11DepthStencilView *pDSV =
       pDepthStencilView;

  SK_WRAP_AND_HOOK

  auto _Finish = [&](void) ->
  void
  {
    // This SEH translator hack is necessary for Yakuza's crazy engine, it
    //   can recover from what will effectively be no render target bound,
    //     and SK is capable of purging the memory it incorrectly deleted
    //       from its caches after the first exception is raised.
    auto orig_se =
      SK_SEH_ApplyTranslator (
        SK_FilteringStructuredExceptionTranslator (
          EXCEPTION_ACCESS_VIOLATION
        )
      );
    try
    {
      bWrapped ?
        pDevCtx->OMSetRenderTargets ( NumViews, ppRenderTargetViews, pDSV )
               :
        D3D11_OMSetRenderTargets_Original (
          pDevCtx, NumViews, ppRenderTargetViews,
            pDSV );
    }
    catch (const SK_SEH_IgnoredException&) {};
    SK_SEH_RemoveTranslator (orig_se);
  };

  bool early_out =
    ( SK_D3D11_IgnoreWrappedOrDeferred (bWrapped, pDevCtx) ||
    (! bMustNotIgnore) );

  if (early_out)
  {
    return
      _Finish ();
  }

  if (dev_idx == UINT_MAX)
  {
    dev_idx =
      SK_D3D11_GetDeviceContextHandle (pDevCtx);
  }

  if (SK_ImGui_IsDrawing_OnD3D11Ctx (dev_idx, pDevCtx))
  {
    return
      _Finish ();
  }

#ifdef _M_AMD64
  static bool yakuza = ( SK_GetCurrentGameID () == SK_GAME_ID::Yakuza0 ||
                         SK_GetCurrentGameID () == SK_GAME_ID::YakuzaKiwami ||
                         SK_GetCurrentGameID () == SK_GAME_ID::YakuzaKiwami2 ||
                         SK_GetCurrentGameID () == SK_GAME_ID::YakuzaUnderflow );
  extern bool __SK_Yakuza_TrackRTVs;
#endif

  if (
#ifdef _M_AMD64
     (yakuza && (! __SK_Yakuza_TrackRTVs)) ||
#endif
     (! SK_D3D11_ShouldTrackRenderOp (pDevCtx, dev_idx)))
  {
    if (tracked_rtv->active_count [dev_idx] > 0)
    {
      for (auto& rtv : tracked_rtv->active [dev_idx])
                 rtv.store (false);

      tracked_rtv->active_count [dev_idx] = 0;
    }

    return
      _Finish ();
 }

  _Finish ();

  if (NumViews > 0)
  {
    if (ppRenderTargetViews != nullptr)
    {
      static auto&                      _want_rt_list =
        SK_D3D11_KnownTargets::_mod_tool_wants;

      auto&                                rt_views =
        (*SK_D3D11_RenderTargets)[dev_idx].rt_views;

      const auto* tracked_rtv_res  =
        static_cast <ID3D11RenderTargetView *> (
          ReadPointerAcquire ((volatile PVOID *)&tracked_rtv->resource)
        );

      for (UINT i = 0; i < NumViews; i++)
      {
        if (_want_rt_list && ppRenderTargetViews [i] != nullptr)
        {  rt_views.emplace (ppRenderTargetViews [i]);         }

        const bool active_before =
          tracked_rtv->active_count [dev_idx] > 0 ?
          tracked_rtv->active       [dev_idx][i].load ()
                                                  : false;

        const bool active =
          ( tracked_rtv_res == ppRenderTargetViews [i] ) ?
                       true : false;

        if (active_before != active)
        {
          tracked_rtv->active [dev_idx][i] = active;

          if      (             active                    )
                   tracked_rtv->active_count [dev_idx]++;
          else if (tracked_rtv->active_count [dev_idx] > 0)
                   tracked_rtv->active_count [dev_idx]--;
        }
      }

      /////for ( UINT j = 0; j < 5 ; j++ )
      /////{
      /////  switch (j)
      /////  { case 0:
      /////    {
      /////      static auto& vertex =
      /////        SK_D3D11_Shaders->vertex;
      /////
      /////      INT  pre_hud_slot  = vertex.tracked.pre_hud_rt_slot;
      /////      if ( pre_hud_slot >= 0 && pre_hud_slot < (INT)NumViews )
      /////      {
      /////        if (vertex.tracked.pre_hud_rtv == nullptr &&
      /////            vertex.current.shader [dev_idx] ==
      /////            vertex.tracked.crc32c.load () )
      /////        {
      /////          if (ppRenderTargetViews [pre_hud_slot] != nullptr)
      /////          {
      /////            vertex.tracked.pre_hud_rtv = ppRenderTargetViews [pre_hud_slot];
      /////          }
      /////        }
      /////      }
      /////    } break;
      /////
      /////    case 1:
      /////    {
      /////      static auto& pixel =
      /////        SK_D3D11_Shaders->pixel;
      /////
      /////      INT  pre_hud_slot  = pixel.tracked.pre_hud_rt_slot;
      /////      if ( pre_hud_slot >= 0 && pre_hud_slot < (INT)NumViews )
      /////      {
      /////        if (pixel.tracked.pre_hud_rtv == nullptr &&
      /////            pixel.current.shader [dev_idx] ==
      /////            pixel.tracked.crc32c.load () )
      /////        {
      /////          if (ppRenderTargetViews [pre_hud_slot] != nullptr)
      /////          {
      /////            pixel.tracked.pre_hud_rtv = ppRenderTargetViews [pre_hud_slot];
      /////          }
      /////        }
      /////      }
      /////    } break;
      /////
      /////    default:
      /////     break;
      /////  }
      /////}
    }

#ifdef _PERSIST_DS_VIEWS
    if (pDepthStencilView != nullptr)
    {
     SK_D3D11_RenderTargets [dev_idx].ds_views.emplace (pDepthStencilView);
    }
#endif
  }
}

[[deprecated]]
HRESULT
SK_D3DX11_SAFE_GetImageInfoFromFileW (const wchar_t* wszTex, D3DX11_IMAGE_INFO* pInfo)
{
  UNREFERENCED_PARAMETER (wszTex);
  UNREFERENCED_PARAMETER (pInfo);

  return E_NOTIMPL;
}

[[deprecated]]
HRESULT
SK_D3DX11_SAFE_CreateTextureFromFileW ( ID3D11Device*           pDevice,   LPCWSTR           pSrcFile,
                                        D3DX11_IMAGE_LOAD_INFO* pLoadInfo, ID3D11Resource** ppTexture )
{
  UNREFERENCED_PARAMETER (pDevice),   UNREFERENCED_PARAMETER (pSrcFile),
  UNREFERENCED_PARAMETER (pLoadInfo), UNREFERENCED_PARAMETER (ppTexture);

  return E_NOTIMPL;
}

HRESULT
WINAPI
D3D11Dev_CreateTexture2D_Impl (
  _In_              ID3D11Device            *This,
  _Inout_opt_       D3D11_TEXTURE2D_DESC    *pDesc,
  _In_opt_    const D3D11_SUBRESOURCE_DATA  *pInitialData,
  _Out_opt_         ID3D11Texture2D        **ppTexture2D,
                    LPVOID                   lpCallerAddr,
                    SK_TLS                  *pTLS )
{
  static auto& rb =
    SK_GetCurrentRenderBackend ();

  SK_ComQIPtr <ID3D11Device>        pDev    (This);
  SK_ComQIPtr <ID3D11DeviceContext> pDevCtx (rb.d3d11.immediate_ctx);

  SK_ComPtr <IUnknown>                                      pD3D11On12Device;
  if (This != nullptr)
      This->QueryInterface (IID_ID3D11On12Device, (void **)&pD3D11On12Device.p);

                                       // Ansel would deadlock us while calling QueryInterface in getDevice <...>
  if (pD3D11On12Device.p != nullptr || (rb.api != SK_RenderAPI::D3D11 && rb.api != SK_RenderAPI::Reserved))
  {
    return
      D3D11Dev_CreateTexture2D_Original (This, pDesc, pInitialData, ppTexture2D);
  }

  auto rb_device =
    rb.getDevice <ID3D11Device> ();

  if (rb_device.p != nullptr && (! rb_device.IsEqualObject (This)))
  {
    if (config.system.log_level > 0)
    {
      SK_ReleaseAssert (!"Texture upload not cached because it happened on the wrong device");
    }

    return
      D3D11Dev_CreateTexture2D_Original (This, pDesc, pInitialData, ppTexture2D);
  }

  if (rb.device.p == nullptr)
  {
    // Better late than never
    if (! pDevCtx)
    {
      // Only from devices belonging to the game, no wrappers or Ansel
      SK_ComQIPtr <IDXGISwapChain> pSwapChain (rb.swapchain);

      if (pSwapChain.p != nullptr)
      {
        SK_ComPtr <ID3D11Device> pSwapDev;

        if (SUCCEEDED (pSwapChain->GetDevice (IID_PPV_ARGS (&pSwapDev.p))) && pDev.IsEqualObject (pSwapDev))
        {
          This->GetImmediateContext (&pDevCtx.p);
             rb.d3d11.immediate_ctx = pDevCtx;
             rb.setDevice            (pDev);

          SK_LOG0 ( (L"Active D3D11 Device Context Established on first Texture Upload" ),
                     L"  D3D 11  " );
        }
      }
    }
  }

  // Compat hack needed in all Unity games
  if (rb.windows.unity)
    config.textures.cache.ignore_nonmipped = true;


  static auto& textures =
    SK_D3D11_Textures;

  if (pDesc != nullptr)
  {
    // Make all staging textures read/write so that we can perform injection
    //  -- NieR: Replicant lacks write access on some
    if (pDesc->Usage == D3D11_USAGE_STAGING)
        pDesc->CPUAccessFlags |= ( D3D11_CPU_ACCESS_WRITE |
                                   D3D11_CPU_ACCESS_READ );

    static const auto game_id =
      SK_GetCurrentGameID ();

    switch (game_id)
    {
#ifdef _M_AMD64
      case SK_GAME_ID::Tales_of_Vesperia:
      {
        extern void SK_TVFix_CreateTexture2D (
          D3D11_TEXTURE2D_DESC    *pDesc
        );

        if (SK_GetCallingDLL (lpCallerAddr) == SK_GetModuleHandle (nullptr))
            SK_TVFix_CreateTexture2D (pDesc);
      } break;

      case SK_GAME_ID::GalGunReturns:
      {
        if (! config.window.res.override.isZero ())
        {
          if ((pDesc->BindFlags & D3D11_BIND_RENDER_TARGET) == D3D11_BIND_RENDER_TARGET ||
              (pDesc->BindFlags & D3D11_BIND_DEPTH_STENCIL) == D3D11_BIND_DEPTH_STENCIL)
          {
            if (pDesc->Width == 480  && pDesc->Height == 270)
            {
              pDesc->Width  = config.window.res.override.x / 4;
              pDesc->Height = config.window.res.override.y / 4;
            }

            if (pDesc->Width == 1920 && pDesc->Height == 1080)
            {
              pDesc->Width  = config.window.res.override.x;
              pDesc->Height = config.window.res.override.y;
            }
          }
        }
      } break;

      case SK_GAME_ID::ShinMegamiTensei3:
      {
        //if (! DirectX::IsCompressed (pDesc->Format) )
        if ( ( (pDesc->BindFlags & D3D11_BIND_DEPTH_STENCIL) ||
               (pDesc->BindFlags & D3D11_BIND_RENDER_TARGET) || 
               (pDesc->BindFlags & D3D11_BIND_UNORDERED_ACCESS) ) )
        {
          if ( pDesc->Width  == 1280 &&
               pDesc->Height == 720 )
          {
            SK_ComQIPtr <IDXGISwapChain>
                     pSwapChain (
                   rb.swapchain );
            DXGI_SWAP_CHAIN_DESC  swapDesc = { };
          
            if (pSwapChain != nullptr)
            {   pSwapChain->GetDesc (&swapDesc);
          
              pDesc->Width  = swapDesc.BufferDesc.Width;
              pDesc->Height = swapDesc.BufferDesc.Height;
            }
          }
          ////
          ////else
          ////if ( pDesc->Width  == 1920 &&
          ////     pDesc->Height == 1080 )
          ////{
          ////  SK_ComQIPtr <IDXGISwapChain>
          ////           pSwapChain (
          ////         rb.swapchain );
          ////  DXGI_SWAP_CHAIN_DESC  swapDesc = { };
          ////
          ////  if (pSwapChain != nullptr)
          ////  {   pSwapChain->GetDesc (&swapDesc);
          ////
          ////    pDesc->Width  = swapDesc.BufferDesc.Width;
          ////    pDesc->Height = swapDesc.BufferDesc.Height;
          ////  }
          ////}
          ////
          ////else
          if ( pDesc->Width  == 1080 &&
               pDesc->Height == 720 )
          {
            SK_ComQIPtr <IDXGISwapChain>
                     pSwapChain (
                   rb.swapchain );
            DXGI_SWAP_CHAIN_DESC  swapDesc = { };
          
            if (pSwapChain != nullptr)
            {   pSwapChain->GetDesc (&swapDesc);
          
              pDesc->Width  *= 2;
              pDesc->Height *= 2;
            }
          }

          else
          if ( pDesc->Width  == 512 &&
               pDesc->Height == 512 )
          {
          //pDesc->Width  = 8192;
          //pDesc->Height = 8192;
          }

          else if (pDesc->Width <= 640)
          {
            //pDesc->Width  *= 3;
            //pDesc->Height *= 3;
          }
          
          // 8x MSAA baby !!
          if (pDesc->SampleDesc.Count == 2)
          {   pDesc->SampleDesc.Count = 8;
          }
        }
      } break;
#else
      case SK_GAME_ID::ChronoCross:
      {
        extern float
            __SK_CC_ResMultiplier;
        if (__SK_CC_ResMultiplier)
        {
          if (pDesc->Format != DXGI_FORMAT_R16_UINT &&
              pDesc->Width == 4096 && pDesc->Height == 2048 && ((pDesc->BindFlags & D3D11_BIND_DEPTH_STENCIL) ||
                                                                (pDesc->BindFlags & D3D11_BIND_RENDER_TARGET) ||
                                                                (pDesc->BindFlags & D3D11_BIND_UNORDERED_ACCESS)))
          {
            pDesc->Width  = 4096 * static_cast <UINT> (__SK_CC_ResMultiplier);
            pDesc->Height = 2048 * static_cast <UINT> (__SK_CC_ResMultiplier);
          }
        }
      } break;
#endif
      default:
        break;
    }
  }

  SK_ComQIPtr <IDXGISwapChain>
                   pSwapChain (
                     rb.swapchain );
  DXGI_SWAP_CHAIN_DESC  swapDesc = { };

  if (pSwapChain != nullptr)
      pSwapChain->GetDesc (&swapDesc);
  else if (pDesc != nullptr &&
          (pDesc->BindFlags & D3D11_BIND_RENDER_TARGET) != 0)
    SK_LOGi0 (L"Render Target allocated while SK had no active SwapChain...");


  // Handle stuff like DSV textures created for SwapChains that had their
  //   MSAA status removed to be compatible with Flip
  extern bool
        bOriginallyFlip;
  if (! bOriginallyFlip)
  {
    extern UINT
        uiOriginalBltSampleCount;
    if (uiOriginalBltSampleCount == pDesc->SampleDesc.Count && pDesc->SampleDesc.Count > 1)
    {
      if ( (pDesc->BindFlags & D3D11_BIND_DEPTH_STENCIL) ==
                               D3D11_BIND_DEPTH_STENCIL )
      {
        bool bSwapChainResized = false;

        if (rb.swapchain != nullptr)
        {
          if (swapDesc.BufferDesc.Width  == pDesc->Width &&
              swapDesc.BufferDesc.Height == pDesc->Height)
          {
            SK_ComPtr <ID3D11Texture2D>                              pBuffer0;
            pSwapChain->GetBuffer (0, IID_ID3D11Texture2D, (void **)&pBuffer0.p);

            if (pBuffer0 != nullptr)
            {
              D3D11_TEXTURE2D_DESC texDesc = { };
              pBuffer0->GetDesc  (&texDesc);

              // We have a mismatch, despite knowledge of the requested MSAA level
              //   when the swapchain was wrapped.  Odd, but deal with it.
              if (texDesc.SampleDesc.Count != uiOriginalBltSampleCount)
                bSwapChainResized = true;
            }
            else
              bSwapChainResized = true;
          }
        }

        if (rb.swapchain == nullptr || bSwapChainResized)
        {
          dll_log->Log (
            L"Flip Override [ Orig Depth/Stencil Sample Count: %d, New: %d ]",
                                          pDesc->SampleDesc.Count, 1
          );
          pDesc->SampleDesc.Count = 1;
        }
      }
    }
  }


  //--- HDR Format Wars (or at least format re-training) ---
  if ( pDesc                 != nullptr             &&
       pDesc->Usage          != D3D11_USAGE_STAGING &&
       pDesc->Usage          != D3D11_USAGE_DYNAMIC &&
                   ( pInitialData          == nullptr ||
                     pInitialData->pSysMem == nullptr ) )
  {
    static constexpr UINT _UnwantedFlags =
        ( D3D11_BIND_VERTEX_BUFFER   | D3D11_BIND_INDEX_BUFFER     |
          D3D11_BIND_CONSTANT_BUFFER | D3D11_BIND_STREAM_OUTPUT    |
          D3D11_BIND_DEPTH_STENCIL   |/*D3D11_BIND_UNORDERED_ACCESS|*/
          D3D11_BIND_DECODER         | D3D11_BIND_VIDEO_ENCODER );

    if ( ( (pDesc->BindFlags & D3D11_BIND_RENDER_TARGET)     ==
                               D3D11_BIND_RENDER_TARGET      ||
          //// UAVs also need special treatment for Compute Shader work
           (pDesc->BindFlags & D3D11_BIND_UNORDERED_ACCESS)  ==
                               D3D11_BIND_UNORDERED_ACCESS ) &&
           (pDesc->BindFlags & _UnwantedFlags) == 0 )
    {
      if ( __SK_HDR_16BitSwap &&
             (! ( DirectX::IsVideo        (pDesc->Format) ||
                  DirectX::IsCompressed   (pDesc->Format) ||
                  DirectX::IsDepthStencil (pDesc->Format) ||
                  SK_DXGI_IsFormatInteger (pDesc->Format) ) )
         )
      {
        if (pDesc->SampleDesc.Count == swapDesc.SampleDesc.Count ||
            pDesc->SampleDesc.Count == 1)
        {
          size_t bpp =
            DirectX::BitsPerPixel (pDesc->Format);
          size_t bpc =
            DirectX::BitsPerColor (pDesc->Format);

          if (     bpc == 11)
            InterlockedIncrement (&SK_HDR_RenderTargets_11bpc->CandidatesSeen);
          else if (bpc == 10)
            InterlockedIncrement (&SK_HDR_RenderTargets_10bpc->CandidatesSeen);

          // 11-bit FP (or 10-bit fixed?) -> 16-bit FP
          if ( (SK_HDR_RenderTargets_11bpc->PromoteTo16Bit && bpc == 11) ||
               (SK_HDR_RenderTargets_10bpc->PromoteTo16Bit && bpc == 10) )
          {
            // 32-bit total -> 64-bit
            SK_ReleaseAssert (bpp == 32);

            if (bpc == 11)
            {
              InterlockedAdd64     (&SK_HDR_RenderTargets_11bpc->BytesAllocated, 4LL * pDesc->Width * pDesc->Height);
              InterlockedIncrement (&SK_HDR_RenderTargets_11bpc->TargetsUpgraded);
            }
            else if (bpc == 10)
            {
              InterlockedAdd64     (&SK_HDR_RenderTargets_10bpc->BytesAllocated, 4LL * pDesc->Width * pDesc->Height);
              InterlockedIncrement (&SK_HDR_RenderTargets_10bpc->TargetsUpgraded);
            }

            if (config.system.log_level > 4)
            {
              dll_log->Log ( L"HDR Override [ Orig Fmt: %hs, New Fmt: %hs ]",
                SK_DXGI_FormatToStr (pDesc->Format).                 data (),
                SK_DXGI_FormatToStr (DXGI_FORMAT_R16G16B16A16_FLOAT).data () );
            }

            pDesc->Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
          }

          else
          {
            auto _typeless =
              DirectX::MakeTypeless (pDesc->Format);

            // The HDR formats are RGB(A), they do not play nicely with BGR{A|x}
            bool rgba =
              ( _typeless == DXGI_FORMAT_R8G8B8A8_TYPELESS ||
                _typeless == DXGI_FORMAT_B8G8R8X8_TYPELESS );

            if (! rb.windows.unreal)
              rgba = ( rgba || _typeless == DXGI_FORMAT_B8G8R8A8_TYPELESS );

            // 8-bit RGB(x) -> 16-bit FP
            if ( bpc == 8  && rgba &&
                 bpp == 32 )
            {
              // NieR: Automata is tricky, do not change the format of the bloom
              //   reduction series of targets.
              static const bool bNier =
                ( SK_GetCurrentGameID () == SK_GAME_ID::NieRAutomata );
              if (                                    (! bNier) ||
                  ( pDesc->Width  == swapDesc.BufferDesc.Width &&
                    pDesc->Height == swapDesc.BufferDesc.Height )
                 )
              {
                if (SK_HDR_RenderTargets_8bpc->PromoteTo16Bit)
                {
                  if (config.system.log_level > 4)
                  {
                    dll_log->Log ( L"HDR Override [ Orig Fmt: %hs, New Fmt: %hs ]",
                      SK_DXGI_FormatToStr (pDesc->Format).                 data (),
                      SK_DXGI_FormatToStr (DXGI_FORMAT_R16G16B16A16_FLOAT).data () );
                  }

                  // 32-bit total -> 64-bit
                  InterlockedAdd64     (&SK_HDR_RenderTargets_8bpc->BytesAllocated, 4LL * pDesc->Width * pDesc->Height);
                  InterlockedIncrement (&SK_HDR_RenderTargets_8bpc->TargetsUpgraded);

                  pDesc->Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
                }

                InterlockedIncrement (&SK_HDR_RenderTargets_8bpc->CandidatesSeen);
              }
            }
          }
        }
      }
    }
  }
  //---

  if (pTLS == nullptr) pTLS = SK_TLS_Bottom ();
  bool  bIgnoreThisUpload = SK_D3D11_IsTexInjectThread (pTLS) ||
                                                        pTLS->imgui->drawing;
  if (! bIgnoreThisUpload) bIgnoreThisUpload = (! (SK_D3D11_cache_textures ||
                                                   SK_D3D11_dump_textures  ||
                                                   SK_D3D11_inject_textures));

  if (! ( pDev    != nullptr &&
          pDevCtx != nullptr ) )
  {
    assert (false);

    return
      D3D11Dev_CreateTexture2D_Original ( This,            pDesc,
                                            pInitialData, ppTexture2D );
  }
  //// -----------

  if (  bIgnoreThisUpload)
  {
    return
      D3D11Dev_CreateTexture2D_Original ( This,            pDesc,
                                            pInitialData, ppTexture2D );
  }

  if (pDesc == nullptr || ppTexture2D == nullptr)
  {
    return
      D3D11Dev_CreateTexture2D_Original (This, pDesc,
                                         pInitialData, ppTexture2D);
  }

  SK_D3D11_Resampler_ProcessFinished (This, pDevCtx, pTLS);

  SK_D3D11_MemoryThreads->mark ();


  DXGI_FORMAT newFormat =
    pDesc->Format;

  if ( pInitialData          == nullptr ||
       pInitialData->pSysMem == nullptr )
  {
    if (SK_D3D11_OverrideDepthStencil (newFormat))
    {
      pDesc->Format = newFormat;
      pInitialData  = nullptr;
    }
  }


  uint32_t checksum   = 0;
  uint32_t cache_tag  = 0;
  size_t   size       = 0;

  SK_ComPtr <ID3D11Texture2D> pCachedTex = nullptr;

  bool cacheable = ( config.textures.d3d11.cache &&
                     pInitialData            != nullptr &&
                     pInitialData->pSysMem   != nullptr &&
                     pDesc->SampleDesc.Count == 1       &&
                    (pDesc->MiscFlags        == 0x00 ||
                     pDesc->MiscFlags        == D3D11_RESOURCE_MISC_RESOURCE_CLAMP)
                                                        &&
                     //pDesc->MiscFlags        != 0x01  &&
                     pDesc->CPUAccessFlags   == 0x0     &&
                     pDesc->Width             > 0       &&
                     pDesc->Height            > 0       &&
                     pDesc->ArraySize        == 1 //||
                   //((pDesc->ArraySize  % 6 == 0) && ( pDesc->MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE ))
                   );

  ///if ( cacheable && pDesc->MipLevels == 0 &&
  ///                  pDesc->MiscFlags == D3D11_RESOURCE_MISC_GENERATE_MIPS )
  ///{
  ///  SK_LOG0 ( ( L"Skipping Texture because of Mipmap Autogen" ),
  ///              L" TexCache " );
  ///}

  bool injectable = false;

  cacheable = cacheable &&
   (!  (pDesc->BindFlags & ( D3D11_BIND_DEPTH_STENCIL |
                             D3D11_BIND_RENDER_TARGET ) )    != 0x0
    )&&(pDesc->BindFlags & ( D3D11_BIND_SHADER_RESOURCE  |
                             D3D11_BIND_UNORDERED_ACCESS ) ) != 0x0
     &&(pDesc->Usage     <   D3D11_USAGE_DYNAMIC); // Cancel out Staging
                                                   //   They will be handled through a
                                                   //     different codepath.

#ifdef _M_AMD64
  static const bool __sk_yk =
    (SK_GetCurrentGameID () == SK_GAME_ID::Yakuza0) ||
    (SK_GetCurrentGameID () == SK_GAME_ID::YakuzaKiwami) ||
    (SK_GetCurrentGameID () == SK_GAME_ID::YakuzaKiwami2) ||
    (SK_GetCurrentGameID () == SK_GAME_ID::YakuzaUnderflow);

  if (__sk_yk)
  {
    if (pDesc->Usage != D3D11_USAGE_IMMUTABLE)
      cacheable = false;

    // The number of immutable textures the engine tries to allocate at once
    //   is responsible for hitching, default is a better memory usage for this
    //     use-case.
    else
        pDesc->Usage  = D3D11_USAGE_DEFAULT;
  }
#else
  static const bool __sk_p4 =
    (SK_GetCurrentGameID () == SK_GAME_ID::Persona4);

  if (__sk_p4)
  {
    if ( (pDesc->BindFlags  & D3D11_BIND_UNORDERED_ACCESS) &&
          pDesc->MipLevels == 1                            &&
          pDesc->Width     == 64                           &&
          pDesc->Width     == pDesc->Height )
    {
      cacheable = false;
    }
  }
#endif


  //if (cacheable)
  //{
  //  //dll_log->Log (L"Misc Flags: %x, Bind: %x", pDesc->MiscFlags, pDesc->BindFlags);
  //}

  bool gen_mips = false;

  if ( config.textures.d3d11.generate_mips && cacheable &&
      ( pDesc->MipLevels != CalcMipmapLODs (pDesc->Width, pDesc->Height) ) )
  {
    gen_mips = true;
  }

  if (config.textures.d3d11.cache && (! cacheable))
  {
    SK_LOG1 ( ( L"Impossible to cache texture (Code Origin: '%s') -- Misc Flags: %x, MipLevels: %lu, "
                L"ArraySize: %lu, CPUAccess: %x, BindFlags: %x, Usage: %x, pInitialData: %08"
                _L(PRIxPTR) L" (%08" _L(PRIxPTR) L")",
                  SK_GetModuleName (SK_GetCallingDLL (lpCallerAddr)).c_str (), pDesc->MiscFlags, pDesc->MipLevels, pDesc->ArraySize,
                    pDesc->CPUAccessFlags, pDesc->BindFlags, pDesc->Usage, (uintptr_t)pInitialData,
                      pInitialData ? (uintptr_t)pInitialData->pSysMem : (uintptr_t)nullptr
              ),
              L"DX11TexMgr" );
  }

  const bool dumpable =
              cacheable;

  cacheable =
    cacheable && ( D3D11_CPU_ACCESS_WRITE != (pDesc->CPUAccessFlags & D3D11_CPU_ACCESS_WRITE) );


  uint32_t top_crc32 = 0x00;
  uint32_t ffx_crc32 = 0x00;

  if (cacheable)
  {
    checksum =
      crc32_tex (pDesc, pInitialData, &size, &top_crc32);

    if (SK_D3D11_inject_textures_ffx)
    {
      ffx_crc32 =
        crc32_ffx (pDesc, pInitialData, &size);
    }

    injectable = (
      checksum != 0x00 &&
       ( SK_D3D11_IsInjectable     (top_crc32, checksum) ||
         SK_D3D11_IsInjectable     (top_crc32, 0x00)     ||
         SK_D3D11_IsInjectable_FFX (ffx_crc32)
       )
    );

    if ( checksum != 0x00 &&
         ( SK_D3D11_cache_textures ||
           injectable
         )
       )
    {
      // If this isn't an injectable texture, then filter out non-mipmapped
      //   textures.
      if ((! injectable) && cache_opts.ignore_non_mipped)
        cacheable &= (pDesc->MipLevels > 1 || DirectX::IsCompressed (pDesc->Format));

      if (cacheable)
      {
        cache_tag  =
          safe_crc32c (top_crc32, (uint8_t *)(pDesc), sizeof D3D11_TEXTURE2D_DESC);

        // Adds and holds a reference
        pCachedTex =
          textures->getTexture2D ( cache_tag, pDesc, nullptr, nullptr,
                                    pTLS );
      }
    }

    else
    {
      cacheable = false;
    }
  }

  if (pCachedTex != nullptr)
  {
    //dll_log->Log ( L"[DX11TexMgr] >> Redundant 2D Texture Load "
                  //L" (Hash=0x%08X [%05.03f MiB]) <<",
                  //checksum, (float)size / (1024.0f * 1024.0f) );

    (*ppTexture2D = pCachedTex);// ->AddRef();

    return S_OK;
  }

  // The concept of a cache-miss only applies if the texture had data at the time
  //   of creation...
  if ( cacheable )
  {
    if (config.textures.cache.vibrate_on_miss)
      SK_XInput_PulseController (config.input.gamepad.xinput.ui_slot, 1.0f, 0.0f);

    textures->CacheMisses_2D++;
  }


  const LARGE_INTEGER load_start =
    SK_QueryPerf ();

  if (injectable)
  {
    if (! SK_D3D11_res_root->empty ())
    {
      wchar_t     wszTex [MAX_PATH + 2] = { };
      wcsncpy_s ( wszTex, MAX_PATH,
                  SK_D3D11_TexNameFromChecksum (top_crc32, checksum, ffx_crc32).c_str (),
                          _TRUNCATE );

      if (                   *wszTex  != L'\0' &&
           GetFileAttributes (wszTex) != INVALID_FILE_ATTRIBUTES )
      {

        HRESULT hr = E_UNEXPECTED;

        DirectX::TexMetadata mdata;

        if (SUCCEEDED ((hr = DirectX::GetMetadataFromDDSFile (wszTex, 0, mdata))))
        {
          DirectX::ScratchImage img;

          if (SUCCEEDED ((hr = DirectX::LoadFromDDSFile (wszTex, 0, &mdata, img))))
          {
            SK_ScopedBool decl_tex_scope (
              SK_D3D11_DeclareTexInjectScope (pTLS)
            );

            if (SUCCEEDED ((hr = DirectX::CreateTexture (This,
                                      img.GetImages     (),
                                      img.GetImageCount (), mdata,
                                        reinterpret_cast <ID3D11Resource **> (ppTexture2D))))
               )
            {
              const LARGE_INTEGER load_end =
                SK_QueryPerf ();

              D3D11_TEXTURE2D_DESC orig_desc = *pDesc;
              D3D11_TEXTURE2D_DESC new_desc  = {    };

              (*ppTexture2D)->GetDesc (&new_desc);

              pDesc->BindFlags      = new_desc.BindFlags;
              pDesc->CPUAccessFlags = new_desc.CPUAccessFlags;
              pDesc->ArraySize      = new_desc.ArraySize;
              pDesc->Format         = new_desc.Format;
              pDesc->Height         = new_desc.Height;
              pDesc->MipLevels      = new_desc.MipLevels;
              pDesc->MiscFlags      = new_desc.MiscFlags;
              pDesc->Usage          = new_desc.Usage;
              pDesc->Width          = new_desc.Width;

              // To allow texture reloads, we cannot allow immutable usage on these textures.
              //
              if (pDesc->Usage == D3D11_USAGE_IMMUTABLE)
              {   pDesc->Usage  = D3D11_USAGE_DEFAULT; }

              size =
                SK_D3D11_ComputeTextureSize (pDesc);

              std::scoped_lock <SK_Thread_HybridSpinlock>
                    scope_lock (*cache_cs);

              textures->refTexture2D (
                *ppTexture2D,
                  pDesc,
                    cache_tag,
                      size,
                        load_end.QuadPart - load_start.QuadPart,
                          top_crc32,
                            wszTex,
                              &orig_desc,  (HMODULE)lpCallerAddr,
                                pTLS );


              textures->Textures_2D [*ppTexture2D].injected = true;

              return ( ( hr = S_OK ) );
            }

            SK_LOG0 ( (L"*** Texture '%s' failed DirectX::CreateTexture (...) -- (HRESULT=%s), skipping!",
                       SK_ConcealUserDir (wszTex), _com_error (hr).ErrorMessage () ),
                       L"DX11TexMgr" );
          }

          else
          {
            SK_LOG0 ( (L"*** Texture '%s' failed DirectX::LoadFromDDSFile (...) -- (HRESULT=%s), skipping!",
                       SK_ConcealUserDir (wszTex), _com_error (hr).ErrorMessage () ),
                       L"DX11TexMgr" );
          }
        }

        else
        {
          SK_LOG0 ( (L"*** Texture '%s' failed DirectX::GetMetadataFromDDSFile (...) -- (HRESULT=%s), skipping!",
                     SK_ConcealUserDir (wszTex), _com_error (hr).ErrorMessage () ),
                     L"DX11TexMgr" );
        }
      }
    }
  }


  HRESULT              ret       = E_NOT_VALID_STATE;
  D3D11_TEXTURE2D_DESC orig_desc = *pDesc;


  static const bool bYs8 =
    (SK_GetCurrentGameID () == SK_GAME_ID::Ys_Eight);

  //
  // Texture has one mipmap, but we want a full mipmap chain
  //
  //   Be smart about this, stream the other mipmap LODs in over time
  //     and adjust the min/max LOD levels while the texture is incomplete.
  //
  if (bYs8 && gen_mips)
  {
    if (pDesc->MipLevels == 1)
    {
      // Various UI textures that only contribute additional load-time
      //   and no benefits if we were to generate mipmaps for them.
      if ( (pDesc->Width == 2048 && pDesc->Height == 1024)
         ||(pDesc->Width == 2048 && pDesc->Height == 2048)
         ||(pDesc->Width == 2048 && pDesc->Height == 4096)
         ||(pDesc->Width == 4096 && pDesc->Height == 4096))
      {
        gen_mips = false;
      }
    }
  }

  if (gen_mips && pInitialData != nullptr)
  {
    SK_LOG4 ( ( L"Generating mipmaps for texture with crc32c: %x", top_crc32 ),
                L" Tex Hash " );

    static bool bToV =
      (SK_GetCurrentGameID () == SK_GAME_ID::Tales_of_Vesperia);

    if (bToV)
    {
      SK_ScopedBool decl_tex_scope (
        SK_D3D11_DeclareTexInjectScope (pTLS)
      );

      SK_ComPtr <ID3D11Texture2D> pTempTex = nullptr;

      ret =
        D3D11Dev_CreateTexture2D_Original ( This, &orig_desc,
                                              pInitialData, &pTempTex.p );

      if (SUCCEEDED (ret))
      {
        if (! SK_D3D11_IsDumped (top_crc32, checksum))
        {
          if ( SUCCEEDED (
                 SK_D3D11_MipmapCacheTexture2D ( pTempTex, top_crc32,
                                                 pTLS, pDevCtx, This )
               )
             )
          {
            // Temporarily violate the scope of texture injection so this command
            //   will pass through our wrapper / hook interfaces
            pTLS->texture_management.injection_thread = FALSE;

            ret =
              D3D11Dev_CreateTexture2D_Impl (
                This, (D3D11_TEXTURE2D_DESC *)&orig_desc, pInitialData,
                               ppTexture2D, lpCallerAddr, pTLS );

            if (SUCCEEDED (ret))
              return ret;
          }
        }
        //ret =
        //  SK_D3D11_MipmapMakeTexture2D (This, pDevCtx, pTempTex, ppTexture2D, pTLS);
      }
    }

    else
    {
      SK_ComPtr <ID3D11Resource>/*ID3D11Texture2D>*/     pTempTex  = nullptr;

      // We will return this, but when it is returned, it will be missing mipmaps
      //   until the resample job (scheduled onto a worker thread) finishes.
      //
      //   Minimum latency is 1 frame before the texture is `.
      //
      SK_ComPtr <ID3D11Texture2D>     pFinalTex = nullptr;

      const D3D11_TEXTURE2D_DESC original_desc =
        *pDesc;
         pDesc->MipLevels = CalcMipmapLODs (pDesc->Width, pDesc->Height);

      if (pDesc->Usage == D3D11_USAGE_IMMUTABLE)
        pDesc->Usage    = D3D11_USAGE_DEFAULT;

      DirectX::TexMetadata mdata = { };

      mdata.width      = pDesc->Width;
      mdata.height     = pDesc->Height;
      mdata.depth      =((pDesc->MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE) ==
                                             D3D11_RESOURCE_MISC_TEXTURECUBE) ?
                                                                            6 : 1;
      mdata.arraySize  = 1;
      mdata.mipLevels  = 1;
      mdata.miscFlags  =((pDesc->MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE) ==
                                             D3D11_RESOURCE_MISC_TEXTURECUBE) ?
                                               DirectX::TEX_MISC_TEXTURECUBE  : 0x0;
      mdata.miscFlags2 = 0;
      mdata.format     = pDesc->Format;
      mdata.dimension  = DirectX::TEX_DIMENSION_TEXTURE2D;


      resample_job_s resample = { };
                     resample.time.preprocess = SK_QueryPerf ().QuadPart;

      auto* image = new DirectX::ScratchImage;
            image->Initialize (mdata);

      bool error = false;

      do
      {
        size_t slice = 0;
        size_t lod   = 0;

        size_t height =
          mdata.height;

        const DirectX::Image* img =
          image->GetImage (lod, slice, 0);

        if ( img         == nullptr ||
             img->pixels == nullptr )
        {
          error = true;
          break;
        }

        const size_t lines =
          DirectX::ComputeScanlines (mdata.format, height);

        if (lines == 0)
        {
          error = true;
          break;
        }

        auto sptr =
          static_cast <const uint8_t *>(
            pInitialData [lod].pSysMem
          );

        if (sptr == nullptr)
          break;

        uint8_t* dptr =
          img->pixels;

        for (size_t h = 0; h < lines; ++h)
        {
          const size_t msize =
            std::min <size_t> ( img->rowPitch,
                                  pInitialData [lod].SysMemPitch );

          memcpy_s (dptr, img->rowPitch, sptr, msize);

          sptr += pInitialData [lod].SysMemPitch;
          dptr += img->rowPitch;
        }

        if (height > 1) height >>= 1;

        if (error)
          break;
      } while (false);

      const DirectX::Image* orig_img =
        image->GetImage (0, 0, 0);

      const bool compressed =
        DirectX::IsCompressed (pDesc->Format);

      if (config.textures.d3d11.uncompressed_mips && compressed)
      {
        auto* decompressed =
          new DirectX::ScratchImage;

        ret =
          DirectX::Decompress (orig_img, 1, image->GetMetadata (), DXGI_FORMAT_UNKNOWN, *decompressed);

        if (SUCCEEDED (ret))
        {
          ret =
            DirectX::CreateTexture ( This,
                                       decompressed->GetImages   (), decompressed->GetImageCount (),
                                       decompressed->GetMetadata (), reinterpret_cast <ID3D11Resource **> (&pTempTex.p) );
          if (SUCCEEDED (ret))
          {
            pDesc->Format =
              decompressed->GetMetadata ().format;

            delete image;
                   image = decompressed;
          }
        }

        if (FAILED (ret))
          delete decompressed;
      }

      else
      {
        D3D11_TEXTURE2D_DESC newDesc = original_desc;
        newDesc.MiscFlags |= D3D11_RESOURCE_MISC_RESOURCE_CLAMP;

        ret =
          D3D11Dev_CreateTexture2D_Original (This, &newDesc, pInitialData, (ID3D11Texture2D **)&pTempTex.p);
      }

      if (SUCCEEDED (ret))
      {
        pDesc->MiscFlags |= D3D11_RESOURCE_MISC_RESOURCE_CLAMP;

        ret =
          D3D11Dev_CreateTexture2D_Original (This, pDesc, nullptr, &pFinalTex);

        if (SUCCEEDED (ret))
        {
          D3D11_CopySubresourceRegion_Original (pDevCtx,
            pFinalTex,
              D3D11CalcSubresource (0, 0, pDesc->MipLevels),
                0, 0, 0,
                  pTempTex,
                    D3D11CalcSubresource (0, 0, 0),
                      nullptr
          );

          size =
            SK_D3D11_ComputeTextureSize (pDesc);
        }
      }

      if (FAILED (ret))
      {
        SK_LOG0 ( (L"Mipmap Generation Failed [%s]",
                    _com_error (ret).ErrorMessage () ), L"DX11TexMgr");
      }

      else
      {
        resample.time.preprocess =
          ( SK_QueryPerf ().QuadPart - resample.time.preprocess );

        (*ppTexture2D)   = pFinalTex;
        (*ppTexture2D)->AddRef ();

        pDevCtx->SetResourceMinLOD (pFinalTex, 0.0F);

        resample.crc32c  = top_crc32;
        resample.data    = image;
        resample.texture = pFinalTex;

        if (resample.data->GetMetadata ().IsCubemap ())
          SK_LOG0 ( (L"Neat, a Cubemap!"), L"DirectXTex" );

        SK_D3D11_Resampler_PostJob (resample);

        // It's the thread pool's problem now, don't free this.
        image = nullptr;
      }

      delete image;
    }
  }


  // Auto-gen or some other process failed, fallback to normal texture upload
  if (FAILED (ret))
  {
    assert (ret == S_OK || ret == E_NOT_VALID_STATE);

      ret =
        D3D11Dev_CreateTexture2D_Original (This, &orig_desc, pInitialData, ppTexture2D);
  }


  const LARGE_INTEGER load_end =
    SK_QueryPerf ();

  if ( SUCCEEDED (ret) &&
          dumpable     &&
      checksum != 0x00 &&
      SK_D3D11_dump_textures )
  {
    if (! SK_D3D11_IsDumped (top_crc32, checksum))
    {
      SK_ScopedBool decl_tex_scope (
        SK_D3D11_DeclareTexInjectScope (pTLS)
      );

      //SK_D3D11_MipmapCacheTexture2D ((*ppTexture2D), top_crc32, pTLS);

      SK_D3D11_DumpTexture2D (&orig_desc, pInitialData, top_crc32, checksum);
    }
  }

  cacheable &=
    (SK_D3D11_cache_textures || injectable);

  if ( SUCCEEDED (ret) && cacheable )
  {
    std::scoped_lock <SK_Thread_HybridSpinlock>
          scope_lock (*cache_cs);

    auto& blacklist =
      textures->Blacklist_2D [orig_desc.MipLevels];

    if ( blacklist.find (checksum) ==
         blacklist.cend (        )  )
    {
      textures->refTexture2D (
        *ppTexture2D,
          pDesc,
            cache_tag,
              size,
                load_end.QuadPart - load_start.QuadPart,
                  top_crc32,
                    L"",
                      &orig_desc,  (HMODULE)(intptr_t)lpCallerAddr,
                        pTLS
      );
    }
  }

  return ret;
}

volatile LONG SK_D3D11_initialized = FALSE;

#define D3D11_STUB(_Return, _Name, _Proto, _Args)                           \
  extern "C"                                                                \
  __declspec (dllexport)                                                    \
  _Return STDMETHODCALLTYPE                                                 \
  _Name _Proto {                                                            \
    WaitForInit ();                                                         \
                                                                            \
    typedef _Return (STDMETHODCALLTYPE *passthrough_pfn) _Proto;            \
    static passthrough_pfn _default_impl = nullptr;                         \
                                                                            \
    if (_default_impl == nullptr) {                                         \
      static constexpr char* szName = #_Name;                               \
     _default_impl=(passthrough_pfn)SK_GetProcAddress (backend_dll, szName);\
                                                                            \
      if (_default_impl == nullptr) {                                       \
        dll_log->Log (                                                      \
          L"Unable to locate symbol  %s in d3d11.dll",                      \
          L#_Name);                                                         \
        return (_Return)E_NOTIMPL;                                          \
      }                                                                     \
    }                                                                       \
                                                                            \
    SK_LOG0 ( (L"[!] %s %s - "                                              \
               L"[Calling Thread: 0x%04x]",                                 \
      L#_Name, L#_Proto, SK_Thread_GetCurrentId ()),                        \
                 __SK_SUBSYSTEM__ );                                        \
                                                                            \
    return _default_impl _Args;                                             \
}

#define D3D11_STUB_(_Name, _Proto, _Args)                                   \
  extern "C"                                                                \
  __declspec (dllexport)                                                    \
  void STDMETHODCALLTYPE                                                    \
  _Name _Proto {                                                            \
    WaitForInit ();                                                         \
                                                                            \
    typedef void (STDMETHODCALLTYPE *passthrough_pfn) _Proto;               \
    static passthrough_pfn _default_impl = nullptr;                         \
                                                                            \
    if (_default_impl == nullptr) {                                         \
      static constexpr char* szName = #_Name;                               \
      _default_impl=(passthrough_pfn)SK_GetProcAddress(backend_dll, szName);\
                                                                            \
      if (_default_impl == nullptr) {                                       \
        dll_log->Log (                                                      \
          L"Unable to locate symbol  %s in d3d11.dll",                      \
          L#_Name );                                                        \
        return;                                                             \
      }                                                                     \
    }                                                                       \
                                                                            \
    SK_LOG0 ( (L"[!] %s %s - "                                              \
               L"[Calling Thread: 0x%04x]",                                 \
      L#_Name, L#_Proto, SK_Thread_GetCurrentId ()),                        \
                 __SK_SUBSYSTEM__ );                                        \
                                                                            \
    _default_impl _Args;                                                    \
}

//extern "C" __declspec (dllexport) D3D11On12CreateDevice_pfn D3D11On12CreateDevice;

bool
SK_D3D11_Init (void)
{
  bool success = false;

  if (! InterlockedCompareExchangeAcquire (&SK_D3D11_initialized, TRUE, FALSE))
  {
    HMODULE hBackend =
      ( (SK_GetDLLRole () & DLL_ROLE::D3D11) ==
                            DLL_ROLE::D3D11 ) ?
                                  backend_dll :
         SK_Modules->LoadLibraryLL (L"d3d11.dll");

    SK::DXGI::hModD3D11 = hBackend;

    D3D11CreateDeviceForD3D12              = SK_GetProcAddress (SK::DXGI::hModD3D11, "D3D11CreateDeviceForD3D12");
    CreateDirect3D11DeviceFromDXGIDevice   = SK_GetProcAddress (SK::DXGI::hModD3D11, "CreateDirect3D11DeviceFromDXGIDevice");
    CreateDirect3D11SurfaceFromDXGISurface = SK_GetProcAddress (SK::DXGI::hModD3D11, "CreateDirect3D11SurfaceFromDXGISurface");
    D3D11On12CreateDevice                  =
                  (D3D11On12CreateDevice_pfn)SK_GetProcAddress (SK::DXGI::hModD3D11, "D3D11On12CreateDevice");
    D3DKMTCloseAdapter                     = SK_GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTCloseAdapter");
    D3DKMTDestroyAllocation                = SK_GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTDestroyAllocation");
    D3DKMTDestroyContext                   = SK_GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTDestroyContext");
    D3DKMTDestroyDevice                    = SK_GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTDestroyDevice ");
    D3DKMTDestroySynchronizationObject     = SK_GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTDestroySynchronizationObject");
    D3DKMTQueryAdapterInfo                 = SK_GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTQueryAdapterInfo");
    D3DKMTSetDisplayPrivateDriverFormat    = SK_GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTSetDisplayPrivateDriverFormat");
    D3DKMTSignalSynchronizationObject      = SK_GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTSignalSynchronizationObject");
    D3DKMTUnlock                           = SK_GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTUnlock");
    D3DKMTWaitForSynchronizationObject     = SK_GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTWaitForSynchronizationObject");
    EnableFeatureLevelUpgrade              = SK_GetProcAddress (SK::DXGI::hModD3D11, "EnableFeatureLevelUpgrade");
    OpenAdapter10                          = SK_GetProcAddress (SK::DXGI::hModD3D11, "OpenAdapter10");
    OpenAdapter10_2                        = SK_GetProcAddress (SK::DXGI::hModD3D11, "OpenAdapter10_2");
    D3D11CoreCreateLayeredDevice           = SK_GetProcAddress (SK::DXGI::hModD3D11, "D3D11CoreCreateLayeredDevice");
    D3D11CoreGetLayeredDeviceSize          = SK_GetProcAddress (SK::DXGI::hModD3D11, "D3D11CoreGetLayeredDeviceSize");
    D3D11CoreRegisterLayers                = SK_GetProcAddress (SK::DXGI::hModD3D11, "D3D11CoreRegisterLayers");
    D3DKMTCreateAllocation                 = SK_GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTCreateAllocation");
    D3DKMTCreateContext                    = SK_GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTCreateContext");
    D3DKMTCreateDevice                     = SK_GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTCreateDevice");
    D3DKMTCreateSynchronizationObject      = SK_GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTCreateSynchronizationObject");
    D3DKMTEscape                           = SK_GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTEscape");
    D3DKMTGetContextSchedulingPriority     = SK_GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTGetContextSchedulingPriority");
    D3DKMTGetDeviceState                   = SK_GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTGetDeviceState");
    D3DKMTGetDisplayModeList               = SK_GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTGetDisplayModeList");
    D3DKMTGetMultisampleMethodList         = SK_GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTGetMultisampleMethodList");
    D3DKMTGetRuntimeData                   = SK_GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTGetRuntimeData");
    D3DKMTGetSharedPrimaryHandle           = SK_GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTGetSharedPrimaryHandle");
    D3DKMTLock                             = SK_GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTLock");
    D3DKMTOpenAdapterFromHdc               = SK_GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTOpenAdapterFromHdc");
    D3DKMTOpenResource                     = SK_GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTOpenResource");
    D3DKMTPresent                          = SK_GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTPresent");
    D3DKMTQueryAllocationResidency         = SK_GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTQueryAllocationResidency");
    D3DKMTQueryResourceInfo                = SK_GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTQueryResourceInfo");
    D3DKMTRender                           = SK_GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTRender");
    D3DKMTSetAllocationPriority            = SK_GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTSetAllocationPriority");
    D3DKMTSetContextSchedulingPriority     = SK_GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTSetContextSchedulingPriority");
    D3DKMTSetDisplayMode                   = SK_GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTSetDisplayMode");
    D3DKMTSetGammaRamp                     = SK_GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTSetGammaRamp");
    D3DKMTSetVidPnSourceOwner              = SK_GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTSetVidPnSourceOwner");
    D3DKMTWaitForVerticalBlankEvent        = SK_GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTWaitForVerticalBlankEvent");
    D3DPerformance_BeginEvent              = SK_GetProcAddress (SK::DXGI::hModD3D11, "D3DPerformance_BeginEvent");
    D3DPerformance_EndEvent                = SK_GetProcAddress (SK::DXGI::hModD3D11, "D3DPerformance_EndEvent");
    D3DPerformance_GetStatus               = SK_GetProcAddress (SK::DXGI::hModD3D11, "D3DPerformance_GetStatus");
    D3DPerformance_SetMarker               = SK_GetProcAddress (SK::DXGI::hModD3D11, "D3DPerformance_SetMarker");


    if (! config.apis.dxgi.d3d11.hook)
      return false;

    SK_LOGi0 (L"Importing D3D11CreateDevice[AndSwapChain]");
    SK_LOGi0 (L"=========================================");

    if ( 0 == _wcsicmp ( SK_GetModuleName (
                             SK_GetDLL () ).c_str (),
          L"d3d11.dll" )
       )
    {
      if (LocalHook_D3D11CreateDevice.active == FALSE)
      {
        D3D11CreateDevice_Import            =  \
         (D3D11CreateDevice_pfn)               \
           SK_GetProcAddress (hBackend, "D3D11CreateDevice");
      }

      if (LocalHook_D3D11CreateDeviceAndSwapChain.active == FALSE)
      {
        D3D11CreateDeviceAndSwapChain_Import            =  \
         (D3D11CreateDeviceAndSwapChain_pfn)               \
           SK_GetProcAddress (hBackend, "D3D11CreateDeviceAndSwapChain");
      }

      SK_LOGi0 ( L"  D3D11CreateDevice:             %s",
                     SK_MakePrettyAddress (D3D11CreateDevice_Import).c_str () );
      SK_LogSymbolName                    (D3D11CreateDevice_Import);

      SK_LOGi0 ( L"  D3D11CreateDeviceAndSwapChain: %s",
                      SK_MakePrettyAddress (D3D11CreateDeviceAndSwapChain_Import).c_str () );
      SK_LogSymbolName                     (D3D11CreateDeviceAndSwapChain_Import);

      pfnD3D11CreateDeviceAndSwapChain = D3D11CreateDeviceAndSwapChain_Import;
      pfnD3D11CreateDevice             = D3D11CreateDevice_Import;

      InterlockedIncrementRelease (&SK_D3D11_initialized);
    }

    else
    {
      if ( LocalHook_D3D11CreateDevice.active == TRUE ||
          ( MH_OK ==
             SK_CreateDLLHook2 (      L"d3d11.dll",
                                       "D3D11CreateDevice",
                                        D3D11CreateDevice_Detour,
               static_cast_p2p <void> (&D3D11CreateDevice_Import),
                                    &pfnD3D11CreateDevice )
          )
         )
      {
              SK_LOGi0 ( L"  D3D11CreateDevice:              %s  %s",
        SK_MakePrettyAddress (pfnD3D11CreateDevice ? pfnD3D11CreateDevice :
                                                        D3D11CreateDevice_Import).c_str (),
                              pfnD3D11CreateDevice ? L"{ Hooked }" :
                                                     L"{ Cached }" );
      }

      if ( LocalHook_D3D11CreateDeviceAndSwapChain.active == TRUE ||
          ( MH_OK ==
             SK_CreateDLLHook2 (    L"d3d11.dll",
                                     "D3D11CreateDeviceAndSwapChain",
                                      D3D11CreateDeviceAndSwapChain_Detour,
             static_cast_p2p <void> (&D3D11CreateDeviceAndSwapChain_Import),
                                  &pfnD3D11CreateDeviceAndSwapChain )
          )
         )
      {
            SK_LOGi0 ( L"  D3D11CreateDeviceAndSwapChain:  %s  %s",
        SK_MakePrettyAddress (pfnD3D11CreateDeviceAndSwapChain ? pfnD3D11CreateDeviceAndSwapChain :
                                                                    D3D11CreateDeviceAndSwapChain_Import).c_str (),
                            pfnD3D11CreateDeviceAndSwapChain ? L"{ Hooked }" :
                                                               L"{ Cached }" );
        SK_LogSymbolName     (pfnD3D11CreateDeviceAndSwapChain);

        if ( (SK_GetDLLRole () & DLL_ROLE::D3D11 ) ==
                                 DLL_ROLE::D3D11 )
        {
          SK_RunLHIfBitness ( 64, SK_LoadPlugIns64 (),
                                  SK_LoadPlugIns32 () );
        }

        if ( ( LocalHook_D3D11CreateDevice.active             == TRUE  ||
               MH_OK == MH_QueueEnableHook (pfnD3D11CreateDevice) ) &&
             ( LocalHook_D3D11CreateDeviceAndSwapChain.active == TRUE  ||
               MH_OK == MH_QueueEnableHook (pfnD3D11CreateDeviceAndSwapChain) ) )
        {
          InterlockedIncrementRelease (&SK_D3D11_initialized);

          success =
            ( MH_OK == SK_ApplyQueuedHooks () );
        }
      }

      if (! success)
      {
        SK_LOGi0 ( L"Something went wrong hooking D3D11 -- "
                   L"need better errors." );
      }
    }

    LocalHook_D3D11CreateDeviceAndSwapChain.target.addr =
           pfnD3D11CreateDeviceAndSwapChain ?
           pfnD3D11CreateDeviceAndSwapChain :
              D3D11CreateDeviceAndSwapChain_Import;
    LocalHook_D3D11CreateDeviceAndSwapChain.active      = TRUE;

    LocalHook_D3D11CreateDevice.target.addr =
           pfnD3D11CreateDevice ?
           pfnD3D11CreateDevice :
              D3D11CreateDevice_Import;
    LocalHook_D3D11CreateDevice.active      = TRUE;
  }

  else
    SK_Thread_SpinUntilAtomicMin (&SK_D3D11_initialized, 2);

  return
    success;
}

void
SK_D3D11_Shutdown (void)
{
  static auto& textures =
    SK_D3D11_Textures;

  if (! InterlockedCompareExchangeAcquire (
          &SK_D3D11_initialized, FALSE, TRUE )
     )
  {
    return;
  }

  if (textures->RedundantLoads_2D > 0)
  {
    SK_LOGs0 ( L"Perf Stats",
               L"At shutdown: %7.2f seconds and %7.2f MiB of"
                  L" CPU->GPU I/O avoided by %lu texture cache hits.",
                    textures->RedundantTime_2D / 1000.0f,
                      (float)textures->RedundantData_2D.load () /
                                 (1024.0f * 1024.0f),
                             textures->RedundantLoads_2D.load () );
  }

  textures->reset ();

  // Stop caching while we shutdown
  SK_D3D11_cache_textures = false;

  if (SK_FreeLibrary (SK::DXGI::hModD3D11))
  {
    //DeleteCriticalSection (&tex_cs);
    //DeleteCriticalSection (&hash_cs);
    //DeleteCriticalSection (&dump_cs);
    //DeleteCriticalSection (&cache_cs);
    //DeleteCriticalSection (&inject_cs);
    //DeleteCriticalSection (&preload_cs);
  }
}

void
SK_D3D11_EnableHooks (void)
{
  WriteRelease (&__d3d11_ready, TRUE);
}


void
SK_D3D11_HookDevCtx (sk_hook_d3d11_t *pHooks)
{
  static          bool hooked = false;
  if (! std::exchange (hooked,  true))
  {
    ///if (config.apis.last_known == SK_RenderAPI::D3D12)
    ///{
    ///  SK_LOG0 ( ( L" Last known render API was D3D12, reducing D3D11 DevCtx hook level to avoid D3D11On12 insanity." ),
    ///              L"*D3D11On12" );
    ///
    ///  return;
    ///}

#if 0
    DXGI_VIRTUAL_OVERRIDE ( pHooks->ppImmediateContext, 7, "ID3D11DeviceContext::VSSetConstantBuffers",
                             D3D11_VSSetConstantBuffers_Override, D3D11_VSSetConstantBuffers_Original,
                             D3D11_VSSetConstantBuffers_pfn);
#else
    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,    7,
                          "ID3D11DeviceContext::VSSetConstantBuffers",
                                          D3D11_VSSetConstantBuffers_Override,
                                          D3D11_VSSetConstantBuffers_Original,
                                          D3D11_VSSetConstantBuffers_pfn );
#endif

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,    8,
                          "ID3D11DeviceContext::PSSetShaderResources",
                                          D3D11_PSSetShaderResources_Override,
                                          D3D11_PSSetShaderResources_Original,
                                          D3D11_PSSetShaderResources_pfn );

#if 0
    DXGI_VIRTUAL_OVERRIDE (pHooks->ppImmediateContext, 9,
                            "ID3D11DeviceContext::PSSetShader",
                             D3D11_PSSetShader_Override, D3D11_PSSetShader_Original,
                             D3D11_PSSetShader_pfn);
#else
    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,    9,
                          "ID3D11DeviceContext::PSSetShader",
                                          D3D11_PSSetShader_Override,
                                          D3D11_PSSetShader_Original,
                                          D3D11_PSSetShader_pfn );
#endif

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   10,
                          "ID3D11DeviceContext::PSSetSamplers",
                                          D3D11_PSSetSamplers_Override,
                                          D3D11_PSSetSamplers_Original,
                                          D3D11_PSSetSamplers_pfn );

#if 0
    DXGI_VIRTUAL_OVERRIDE ( pHooks->ppImmediateContext, 11, "ID3D11DeviceContext::VSSetShader",
                             D3D11_VSSetShader_Override, D3D11_VSSetShader_Original,
                             D3D11_VSSetShader_pfn);
#else
    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   11,
                          "ID3D11DeviceContext::VSSetShader",
                                          D3D11_VSSetShader_Override,
                                          D3D11_VSSetShader_Original,
                                          D3D11_VSSetShader_pfn );
#endif

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   12,
                          "ID3D11DeviceContext::DrawIndexed",
                                          D3D11_DrawIndexed_Override,
                                          D3D11_DrawIndexed_Original,
                                          D3D11_DrawIndexed_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   13,
                          "ID3D11DeviceContext::Draw",
                                          D3D11_Draw_Override,
                                          D3D11_Draw_Original,
                                          D3D11_Draw_pfn );

    //
    // Third-party software frequently causes these hooks to become corrupted, try installing a new
    //   vFtable pointer instead of hooking the function.
    //
#if 0
    DXGI_VIRTUAL_OVERRIDE ( pHooks->ppImmediateContext, 14, "ID3D11DeviceContext::Map",
                             D3D11_Map_Override, D3D11_Map_Original,
                             D3D11_Map_pfn);
#else
    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   14,
                          "ID3D11DeviceContext::Map",
                                             D3D11_Map_Override,
                                             D3D11_Map_Original,
                                             D3D11_Map_pfn );

      DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   15,
                            "ID3D11DeviceContext::Unmap",
                                            D3D11_Unmap_Override,
                                            D3D11_Unmap_Original,
                                            D3D11_Unmap_pfn );
#endif

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   16,
                          "ID3D11DeviceContext::PSSetConstantBuffers",
                                          D3D11_PSSetConstantBuffers_Override,
                                          D3D11_PSSetConstantBuffers_Original,
                                          D3D11_PSSetConstantBuffers_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   20,
                          "ID3D11DeviceContext::DrawIndexedInstanced",
                                          D3D11_DrawIndexedInstanced_Override,
                                          D3D11_DrawIndexedInstanced_Original,
                                          D3D11_DrawIndexedInstanced_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   21,
                          "ID3D11DeviceContext::DrawInstanced",
                                          D3D11_DrawInstanced_Override,
                                          D3D11_DrawInstanced_Original,
                                          D3D11_DrawInstanced_pfn);

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   23,
                          "ID3D11DeviceContext::GSSetShader",
                                          D3D11_GSSetShader_Override,
                                          D3D11_GSSetShader_Original,
                                          D3D11_GSSetShader_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   25,
                          "ID3D11DeviceContext::VSSetShaderResources",
                                          D3D11_VSSetShaderResources_Override,
                                          D3D11_VSSetShaderResources_Original,
                                          D3D11_VSSetShaderResources_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext, 29,
                          "ID3D11DeviceContext::GetData",
                                          D3D11_GetData_Override,
                                          D3D11_GetData_Original,
                                          D3D11_GetData_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   31,
                          "ID3D11DeviceContext::GSSetShaderResources",
                                          D3D11_GSSetShaderResources_Override,
                                          D3D11_GSSetShaderResources_Original,
                                          D3D11_GSSetShaderResources_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   33,
                          "ID3D11DeviceContext::OMSetRenderTargets",
                                          D3D11_OMSetRenderTargets_Override,
                                          D3D11_OMSetRenderTargets_Original,
                                          D3D11_OMSetRenderTargets_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   34,
                          "ID3D11DeviceContext::OMSetRenderTargetsAndUnorderedAccessViews",
                                          D3D11_OMSetRenderTargetsAndUnorderedAccessViews_Override,
                                          D3D11_OMSetRenderTargetsAndUnorderedAccessViews_Original,
                                          D3D11_OMSetRenderTargetsAndUnorderedAccessViews_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   38,
                          "ID3D11DeviceContext::DrawAuto",
                                          D3D11_DrawAuto_Override,
                                          D3D11_DrawAuto_Original,
                                          D3D11_DrawAuto_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   39,
                          "ID3D11DeviceContext::DrawIndexedInstancedIndirect",
                                          D3D11_DrawIndexedInstancedIndirect_Override,
                                          D3D11_DrawIndexedInstancedIndirect_Original,
                                          D3D11_DrawIndexedInstancedIndirect_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   40,
                          "ID3D11DeviceContext::DrawInstancedIndirect",
                                          D3D11_DrawInstancedIndirect_Override,
                                          D3D11_DrawInstancedIndirect_Original,
                                          D3D11_DrawInstancedIndirect_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   41,
                          "ID3D11DeviceContext::Dispatch",
                                          D3D11_Dispatch_Override,
                                          D3D11_Dispatch_Original,
                                          D3D11_Dispatch_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   42,
                          "ID3D11DeviceContext::DispatchIndirect",
                                          D3D11_DispatchIndirect_Override,
                                          D3D11_DispatchIndirect_Original,
                                          D3D11_DispatchIndirect_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   44,
                          "ID3D11DeviceContext::RSSetViewports",
                                          D3D11_RSSetViewports_Override,
                                          D3D11_RSSetViewports_Original,
                                          D3D11_RSSetViewports_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   45,
                          "ID3D11DeviceContext::RSSetScissorRects",
                                          D3D11_RSSetScissorRects_Override,
                                          D3D11_RSSetScissorRects_Original,
                                          D3D11_RSSetScissorRects_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   46,
                          "ID3D11DeviceContext::CopySubresourceRegion",
                                          D3D11_CopySubresourceRegion_Override,
                                          D3D11_CopySubresourceRegion_Original,
                                          D3D11_CopySubresourceRegion_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   47,
                          "ID3D11DeviceContext::CopyResource",
                                          D3D11_CopyResource_Override,
                                          D3D11_CopyResource_Original,
                                          D3D11_CopyResource_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   48,
                          "ID3D11DeviceContext::UpdateSubresource",
                                          D3D11_UpdateSubresource_Override,
                                          D3D11_UpdateSubresource_Original,
                                          D3D11_UpdateSubresource_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   53,
                          "ID3D11DeviceContext::ClearDepthStencilView",
                                          D3D11_ClearDepthStencilView_Override,
                                          D3D11_ClearDepthStencilView_Original,
                                          D3D11_ClearDepthStencilView_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   58,
                          "ID3D11DeviceContext::ExecuteCommandList",
                                          D3D11_ExecuteCommandList_Override,
                                          D3D11_ExecuteCommandList_Original,
                                          D3D11_ExecuteCommandList_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   59,
                          "ID3D11DeviceContext::HSSetShaderResources",
                                          D3D11_HSSetShaderResources_Override,
                                          D3D11_HSSetShaderResources_Original,
                                          D3D11_HSSetShaderResources_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   60,
                          "ID3D11DeviceContext::HSSetShader",
                                          D3D11_HSSetShader_Override,
                                          D3D11_HSSetShader_Original,
                                          D3D11_HSSetShader_pfn);

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   63,
                          "ID3D11DeviceContext::DSSetShaderResources",
                                          D3D11_DSSetShaderResources_Override,
                                          D3D11_DSSetShaderResources_Original,
                                          D3D11_DSSetShaderResources_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   64,
                          "ID3D11DeviceContext::DSSetShader",
                                          D3D11_DSSetShader_Override,
                                          D3D11_DSSetShader_Original,
                                          D3D11_DSSetShader_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   67,
                          "ID3D11DeviceContext::CSSetShaderResources",
                                          D3D11_CSSetShaderResources_Override,
                                          D3D11_CSSetShaderResources_Original,
                                          D3D11_CSSetShaderResources_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   68,
                          "ID3D11DeviceContext::CSSetUnorderedAccessViews",
                                          D3D11_CSSetUnorderedAccessViews_Override,
                                          D3D11_CSSetUnorderedAccessViews_Original,
                                          D3D11_CSSetUnorderedAccessViews_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   69,
                          "ID3D11DeviceContext::CSSetShader",
                                          D3D11_CSSetShader_Override,
                                          D3D11_CSSetShader_Original,
                                          D3D11_CSSetShader_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   74,
                          "ID3D11DeviceContext::PSGetShader",
                                          D3D11_PSGetShader_Override,
                                          D3D11_PSGetShader_Original,
                                          D3D11_PSGetShader_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   76,
                          "ID3D11DeviceContext::VSGetShader",
                                          D3D11_VSGetShader_Override,
                                          D3D11_VSGetShader_Original,
                                          D3D11_VSGetShader_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   82,
                          "ID3D11DeviceContext::GSGetShader",
                                          D3D11_GSGetShader_Override,
                                          D3D11_GSGetShader_Original,
                                          D3D11_GSGetShader_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   89,
                          "ID3D11DeviceContext::OMGetRenderTargets",
                                          D3D11_OMGetRenderTargets_Override,
                                          D3D11_OMGetRenderTargets_Original,
                                          D3D11_OMGetRenderTargets_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   90,
                          "ID3D11DeviceContext::OMGetRenderTargetsAndUnorderedAccessViews",
                                          D3D11_OMGetRenderTargetsAndUnorderedAccessViews_Override,
                                          D3D11_OMGetRenderTargetsAndUnorderedAccessViews_Original,
                                          D3D11_OMGetRenderTargetsAndUnorderedAccessViews_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   98,
                           "ID3D11DeviceContext::HSGetShader",
                                           D3D11_HSGetShader_Override,
                                           D3D11_HSGetShader_Original,
                                           D3D11_HSGetShader_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,  102,
                          "ID3D11DeviceContext::DSGetShader",
                                          D3D11_DSGetShader_Override,
                                          D3D11_DSGetShader_Original,
                                          D3D11_DSGetShader_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,  107,
                          "ID3D11DeviceContext::CSGetShader",
                                          D3D11_CSGetShader_Override,
                                          D3D11_CSGetShader_Original,
                                          D3D11_CSGetShader_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   110,
                      "ID3D11DeviceContext::ClearState",
                                      D3D11_ClearState_Override,
                                      D3D11_ClearState_Original,
                                      D3D11_ClearState_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppImmediateContext,   114,
                      "ID3D11DeviceContext::FinishCommandList",
                                      D3D11_FinishCommandList_Override,
                                      D3D11_FinishCommandList_Original,
                                      D3D11_FinishCommandList_pfn );
  }
}


extern
unsigned int __stdcall HookD3D12 (LPVOID user);

DWORD
__stdcall
HookD3D11 (LPVOID user)
{
  if (! config.apis.dxgi.d3d11.hook)
    return 0;

  // Wait for DXGI to boot
  if (CreateDXGIFactory_Import == nullptr)
  {
    static volatile ULONG implicit_init = FALSE;

    // If something called a D3D11 function before DXGI was initialized,
    //   begin the process, but ... only do this once.
    if (! InterlockedCompareExchange (&implicit_init, TRUE, FALSE))
    {
      SK_LOG0 ( (L" >> Implicit Initialization Triggered <<"), __SK_SUBSYSTEM__ );
      SK_BootDXGI ();
    }

    while (CreateDXGIFactory_Import == nullptr)
      MsgWaitForMultipleObjectsEx (0, nullptr, 16UL, QS_ALLINPUT, MWMO_INPUTAVAILABLE);

    // TODO: Handle situation where CreateDXGIFactory is unloadable
  }

  static volatile LONG __d3d11_hooked = 0;

  // This only needs to be done once
  if (InterlockedCompareExchange (&__d3d11_hooked, 1, 0) == 0)
  {
    SK_LOGi0 (L"  Hooking D3D11");

    auto* pHooks =
      static_cast <sk_hook_d3d11_t *> (user);

  //  3 CreateBuffer
  //  4 CreateTexture1D
  //  5 CreateTexture2D
  //  6 CreateTexture3D
  //  7 CreateShaderResourceView
  //  8 CreateUnorderedAccessView
  //  9 CreateRenderTargetView
  // 10 CreateDepthStencilView
  // 11 CreateInputLayout
  // 12 CreateVertexShader
  // 13 CreateGeometryShader
  // 14 CreateGeometryShaderWithStreamOutput
  // 15 CreatePixelShader
  // 16 CreateHullShader
  // 17 CreateDomainShader
  // 18 CreateComputeShader
  // 19 CreateClassLinkage
  // 20 CreateBlendState
  // 21 CreateDepthStencilState
  // 22 CreateRasterizerState
  // 23 CreateSamplerState
  // 24 CreateQuery
  // 25 CreatePredicate
  // 26 CreateCounter
  // 27 CreateDeferredContext
  // 28 OpenSharedResource
  // 29 CheckFormatSupport
  // 30 CheckMultisampleQualityLevels
  // 31 CheckCounterInfo
  // 32 CheckCounter
  // 33 CheckFeatureSupport
  // 34 GetPrivateData
  // 35 SetPrivateData
  // 36 SetPrivateDataInterface
  // 37 GetFeatureLevel
  // 38 GetCreationFlags
  // 39 GetDeviceRemovedReason
  // 40 GetImmediateContext
  // 41 SetExceptionMode
  // 42 GetExceptionMode

  // 43 GetImmediateContext1
  // 44 CreateDeferredContext1

  // 45 CreateBlendState1
  // 46 CreateRasterizerState1
  // 47 CreateDeviceContextState
  // 48 OpenSharedResource1
  // 49 OpenSharedResourceByName

  // 50 GetImmediateContext2
  // 51 CreateDeferredContext2

  // 52 GetResourceTiling
  // 53 CheckMultisampleQualityLevels1
  // 54 CreateTexture2D1
  // 55 CreateTexture3D1
  // 56 CreateRasterizerState2
  // 57 CreateShaderResourceView1
  // 58 CreateUnorderedAccessView1
  // 59 CreateRenderTargetView1
  // 60 CreateQuery1

  // 61 GetImmediateContext3
  // 62 CreateDeferredContext3

#if 1
  if ( pHooks->ppDevice           != nullptr &&
       pHooks->ppImmediateContext != nullptr )
  {
    ////// Minimum functionality mode in order to prevent chaos caused by D3D11On12
    ////if (config.apis.last_known == SK_RenderAPI::D3D12)
    ////{
    ////  DXGI_VIRTUAL_HOOK ( pHooks->ppDevice,   15,
    ////                    "ID3D11Device::CreatePixelShader",
    ////                          D3D11Dev_CreatePixelShader_Override,
    ////                          D3D11Dev_CreatePixelShader_Original,
    ////                          D3D11Dev_CreatePixelShader_pfn );
    ////
    ////  SK_LOG0 ( ( L" Last known render API was D3D12, reducing D3D11 hook level to avoid D3D11On12 insanity." ),
    ////              L"*D3D11On12" );
    ////  return true;
    ////}



    DXGI_VIRTUAL_HOOK ( pHooks->ppDevice,    3,
                          "ID3D11Device::CreateBuffer",
                                D3D11Dev_CreateBuffer_Override,
                                D3D11Dev_CreateBuffer_Original,
                                D3D11Dev_CreateBuffer_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppDevice,    5,
                          "ID3D11Device::CreateTexture2D",
                                D3D11Dev_CreateTexture2D_Override,
                                D3D11Dev_CreateTexture2D_Original,
                                D3D11Dev_CreateTexture2D_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppDevice,    7,
                          "ID3D11Device::CreateShaderResourceView",
                                D3D11Dev_CreateShaderResourceView_Override,
                                D3D11Dev_CreateShaderResourceView_Original,
                                D3D11Dev_CreateShaderResourceView_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppDevice,    8,
                          "ID3D11Device::CreateUnorderedAccessView",
                                D3D11Dev_CreateUnorderedAccessView_Override,
                                D3D11Dev_CreateUnorderedAccessView_Original,
                                D3D11Dev_CreateUnorderedAccessView_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppDevice,    9,
                          "ID3D11Device::CreateRenderTargetView",
                                D3D11Dev_CreateRenderTargetView_Override,
                                D3D11Dev_CreateRenderTargetView_Original,
                                D3D11Dev_CreateRenderTargetView_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppDevice,   10,
                          "ID3D11Device::CreateDepthStencilView",
                                D3D11Dev_CreateDepthStencilView_Override,
                                D3D11Dev_CreateDepthStencilView_Original,
                                D3D11Dev_CreateDepthStencilView_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppDevice,   12,
                          "ID3D11Device::CreateVertexShader",
                                D3D11Dev_CreateVertexShader_Override,
                                D3D11Dev_CreateVertexShader_Original,
                                D3D11Dev_CreateVertexShader_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppDevice,   13,
                          "ID3D11Device::CreateGeometryShader",
                                D3D11Dev_CreateGeometryShader_Override,
                                D3D11Dev_CreateGeometryShader_Original,
                                D3D11Dev_CreateGeometryShader_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppDevice,   14,
                          "ID3D11Device::CreateGeometryShaderWithStreamOutput",
                                D3D11Dev_CreateGeometryShaderWithStreamOutput_Override,
                                D3D11Dev_CreateGeometryShaderWithStreamOutput_Original,
                                D3D11Dev_CreateGeometryShaderWithStreamOutput_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppDevice,   15,
                          "ID3D11Device::CreatePixelShader",
                                D3D11Dev_CreatePixelShader_Override,
                                D3D11Dev_CreatePixelShader_Original,
                                D3D11Dev_CreatePixelShader_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppDevice,   16,
                          "ID3D11Device::CreateHullShader",
                                D3D11Dev_CreateHullShader_Override,
                                D3D11Dev_CreateHullShader_Original,
                                D3D11Dev_CreateHullShader_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppDevice,   17,
                          "ID3D11Device::CreateDomainShader",
                                D3D11Dev_CreateDomainShader_Override,
                                D3D11Dev_CreateDomainShader_Original,
                                D3D11Dev_CreateDomainShader_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppDevice,   18,
                          "ID3D11Device::CreateComputeShader",
                                D3D11Dev_CreateComputeShader_Override,
                                D3D11Dev_CreateComputeShader_Original,
                                D3D11Dev_CreateComputeShader_pfn );

    //DXGI_VIRTUAL_HOOK (pHooks->ppDevice, 19, "ID3D11Device::CreateClassLinkage",
    //                       D3D11Dev_CreateClassLinkage_Override, D3D11Dev_CreateClassLinkage_Original,
    //                       D3D11Dev_CreateClassLinkage_pfn);

    DXGI_VIRTUAL_HOOK ( pHooks->ppDevice,    22,
                    "ID3D11Device::CreateRasterizerState",
                          D3D11Dev_CreateRasterizerState_Override,
                          D3D11Dev_CreateRasterizerState_Original,
                          D3D11Dev_CreateRasterizerState_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppDevice,    23,
                          "ID3D11Device::CreateSamplerState",
                                D3D11Dev_CreateSamplerState_Override,
                                D3D11Dev_CreateSamplerState_Original,
                                D3D11Dev_CreateSamplerState_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppDevice,     27,
                          "ID3D11Device::CreateDeferredContext",
                                D3D11Dev_CreateDeferredContext_Override,
                                D3D11Dev_CreateDeferredContext_Original,
                                D3D11Dev_CreateDeferredContext_pfn );

    DXGI_VIRTUAL_HOOK ( pHooks->ppDevice,     40,
                          "ID3D11Device::GetImmediateContext",
                                D3D11Dev_GetImmediateContext_Override,
                                D3D11Dev_GetImmediateContext_Original,
                                D3D11Dev_GetImmediateContext_pfn );

#if 0
    IUnknown *pDev1 = nullptr;
    IUnknown *pDev2 = nullptr;
    IUnknown *pDev3 = nullptr;

    (*pHooks->ppDevice)->QueryInterface (IID_ID3D11Device1, (void **)&pDev1);
    (*pHooks->ppDevice)->QueryInterface (IID_ID3D11Device2, (void **)&pDev2);
    (*pHooks->ppDevice)->QueryInterface (IID_ID3D11Device3, (void **)&pDev3);

    if (pDev1 != nullptr)
    {
      DXGI_VIRTUAL_HOOK ( &pDev1, 43,
                            "ID3D11Device1::GetImmediateContext1",
                                             D3D11Dev_GetImmediateContext1_Override,
                                             D3D11Dev_GetImmediateContext1_Original,
                                             D3D11Dev_GetImmediateContext1_pfn );
      DXGI_VIRTUAL_HOOK ( &pDev1, 44,
                            "ID3D11Device1::CreateDeferredContext1",
                                             D3D11Dev_CreateDeferredContext1_Override,
                                             D3D11Dev_CreateDeferredContext1_Original,
                                             D3D11Dev_CreateDeferredContext1_pfn );
    }

    if (pDev2 != nullptr)
    {
      DXGI_VIRTUAL_HOOK ( &pDev2, 50,
                            "ID3D11Device2::GetImmediateContext2",
                                             D3D11Dev_GetImmediateContext2_Override,
                                             D3D11Dev_GetImmediateContext2_Original,
                                             D3D11Dev_GetImmediateContext2_pfn );
      DXGI_VIRTUAL_HOOK ( &pDev2, 51,
                            "ID3D11Device2::CreateDeferredContext2",
                                             D3D11Dev_CreateDeferredContext2_Override,
                                             D3D11Dev_CreateDeferredContext2_Original,
                                             D3D11Dev_CreateDeferredContext2_pfn );
    }

    if (pDev3 != nullptr)
    {
      DXGI_VIRTUAL_HOOK ( &pDev3, 61,
                            "ID3D11Device3::GetImmediateContext3",
                                             D3D11Dev_GetImmediateContext3_Override,
                                             D3D11Dev_GetImmediateContext3_Original,
                                             D3D11Dev_GetImmediateContext3_pfn );
      DXGI_VIRTUAL_HOOK ( &pDev3, 62,
                            "ID3D11Device3::CreateDeferredContext3",
                                             D3D11Dev_CreateDeferredContext3_Override,
                                             D3D11Dev_CreateDeferredContext3_Original,
                                             D3D11Dev_CreateDeferredContext3_pfn );
    }

    if (pDev3 != nullptr) pDev3->Release ();
    if (pDev2 != nullptr) pDev2->Release ();
    if (pDev1 != nullptr) pDev1->Release ();
#endif

    SK_D3D11_HookDevCtx (pHooks);

    //SK_ComQIPtr <ID3D11DeviceContext1> pDevCtx1 (*pHooks->ppImmediateContext);
    //
    //if (pDevCtx1 != nullptr)
    //{
    //  DXGI_VIRTUAL_HOOK ( &pDevCtx1,  116,
    //                        "ID3D11DeviceContext1::UpdateSubresource1",
    //                                         D3D11_UpdateSubresource1_Override,
    //                                         D3D11_UpdateSubresource1_Original,
    //                                         D3D11_UpdateSubresource1_pfn );
    //}
  }
#endif

  InterlockedIncrement (&__d3d11_hooked);
  }

  else
    SK_Thread_SpinUntilAtomicMin (&__d3d11_hooked, 2);

#ifdef _WIN64
  if (config.apis.dxgi.d3d12.hook)
    HookD3D12 (nullptr);
#endif

  return 0;
}

SK_LazyGlobal <SK_D3D11_StateTrackingCounters> SK_D3D11_TrackingCount;

extern void
SK_D3D11_LiveTextureView (bool& can_scroll, SK_TLS* pTLS);

UINT _GetStashedRTVIndex (ID3D11RenderTargetView* pRTV)
{
  UINT size = 4;
  UINT idx  = std::numeric_limits <UINT>::max ();

  __try
  {
    pRTV->GetPrivateData  (SKID_D3D11DeviceContextHandle, &size, &idx );
  }
  __except ( GetExceptionCode ()     ==     EXCEPTION_ACCESS_VIOLATION ?
             EXCEPTION_CONTINUE_EXECUTION : EXCEPTION_CONTINUE_SEARCH  )
  {
  }

  return idx;
};


SK_ImGui_AutoFont::SK_ImGui_AutoFont (ImFont* pFont)
{
  if (pFont != nullptr)
  {
    ImGui::PushFont (pFont);
             font_ = pFont;
  }
}

SK_ImGui_AutoFont::~SK_ImGui_AutoFont (void)
{
  Detach ();
}

bool
SK_ImGui_AutoFont::Detach (void)
{
  if (font_ != nullptr)
  {
    font_ = nullptr;
    ImGui::PopFont ();

    return true;
  }

  return false;
}








// Not thread-safe, I mean this! Don't let the stupid critical section fool you;
//   if you import this and try to call it, your software will explode.
__declspec (dllexport)
void
__stdcall
SKX_ImGui_RegisterDiscardableResource (IUnknown* pRes)
{
  std::scoped_lock <SK_Thread_CriticalSection>
                  auto_lock (*cs_render_view);

  SK_ComQIPtr <ID3D11View>
                    pView (pRes);

  SK_ReleaseAssert (pView.p != nullptr);

  if (pView.p != nullptr)
  {
    pRes->Release ();

    SK_D3D11_TempResources->push_back (
      pView.Detach ()
    );
  }

  // This function is actually intended to be hooked, and you are expected to
  //   append your code immediately following the return of this hook.
}



//std::array <bool, SK_D3D11_MAX_DEV_CONTEXTS+1A SK_D3D11_KnownShaders::reshade_triggered;







void
SK_D3D11_ResetShaders (ID3D11Device* pDevice)
{
  static auto& shaders  = SK_D3D11_Shaders;
  static auto& vertex   = shaders->vertex;
  static auto& pixel    = shaders->pixel;
  static auto& geometry = shaders->geometry;
  static auto& domain   = shaders->domain;
  static auto& hull     = shaders->hull;
  static auto& compute  = shaders->compute;

  for (auto& it : vertex.descs [pDevice])
  {
    if (it.second.pShader->Release () == 0)
    {
      vertex.rev [pDevice].erase   ((ID3D11VertexShader *)it.second.pShader);
      vertex.descs [pDevice].erase (it.first);
    }
  }

  for (auto& it : pixel.descs [pDevice])
  {
    if (it.second.pShader->Release () == 0)
    {
      pixel.rev [pDevice].erase   ((ID3D11PixelShader *)it.second.pShader);
      pixel.descs [pDevice].erase (it.first);
    }
  }

  for (auto& it : geometry.descs [pDevice])
  {
    if (it.second.pShader->Release () == 0)
    {
      geometry.rev [pDevice].erase   ((ID3D11GeometryShader *)it.second.pShader);
      geometry.descs [pDevice].erase (it.first);
    }
  }

  for (auto& it : hull.descs [pDevice])
  {
    if (it.second.pShader->Release () == 0)
    {
      hull.rev [pDevice].erase   ((ID3D11HullShader *)it.second.pShader);
      hull.descs [pDevice].erase (it.first);
    }
  }

  for (auto& it : domain.descs [pDevice])
  {
    if (it.second.pShader->Release () == 0)
    {
      domain.rev [pDevice].erase   ((ID3D11DomainShader *)it.second.pShader);
      domain.descs [pDevice].erase (it.first);
    }
  }

  for (auto& it : compute.descs [pDevice])
  {
    if (it.second.pShader->Release () == 0)
    {
      compute.rev [pDevice].erase   ((ID3D11ComputeShader *)it.second.pShader);
      compute.descs [pDevice].erase (it.first);
    }
  }
}



void
SK_D3D11_BeginFrame (void)
{
  if (SK_Screenshot_D3D11_BeginFrame ())
  {
    // This looks silly, but it lets HUDless screenhots
    //   set shader state before the frame begins... to
    //     remove HUD shaders.
    return
      SK_D3D11_BeginFrame (); // This recursion will end.
  }
}




void
__stdcall
SK_D3D11_PresentFirstFrame (IDXGISwapChain* pSwapChain)
{
  if (! config.apis.dxgi.d3d11.hook) return;

  SK_D3D11_LoadShaderState (false);

  UNREFERENCED_PARAMETER (pSwapChain);

  SK_D3D11_InitShaderMods ();

  LocalHook_D3D11CreateDevice.active             = TRUE;
  LocalHook_D3D11CreateDeviceAndSwapChain.active = TRUE;

  for ( auto& it : local_d3d11_records )
  {
    if (it->active)
    {
      SK_Hook_ResolveTarget (*it);

      // Don't cache addresses that were screwed with by other injectors
      const wchar_t* wszSection =
        StrStrIW (it->target.module_path, LR"(d3d11.dll)") ?
                                            L"D3D11.Hooks" : nullptr;

      if (! wszSection)
      {
        SK_LOGs0 ( L"Hook Cache",
                   L"Hook for '%hs' resides in '%s', "
                   L"will not cache!",    it->target.symbol_name,
         SK_ConcealUserDir (std::wstring (it->target.module_path).data ())
        );
      }

      else
        SK_Hook_CacheTarget ( *it, wszSection );
    }
  }

  if (SK_IsInjected ())
  {
    auto it_local  = std::begin (local_d3d11_records);
    auto it_global = std::begin (global_d3d11_records);

    while ( it_local != std::end (local_d3d11_records) )
    {
      if (( *it_local )->hits && (
StrStrIW (( *it_local )->target.module_path, LR"(d3d11.dll)") ) &&
          ( *it_local )->active)
        SK_Hook_PushLocalCacheOntoGlobal ( **it_local,
                                             **it_global );
      else
      {
        ( *it_global )->target.addr = nullptr;
        ( *it_global )->hits        = 0;
        ( *it_global )->active      = false;
      }

      it_global++, it_local++;
    }
  }
}

static bool quick_hooked = false;

void
SK_D3D11_QuickHook (void)
{
  if (config.steam.preload_overlay)
    return;

  if (! config.apis.dxgi.d3d11.hook)
    return;

  static volatile LONG hooked = FALSE;

  if (! InterlockedCompareExchange (&hooked, TRUE, FALSE))
  {
    sk_hook_cache_enablement_s state =
      SK_Hook_PreCacheModule ( L"D3D11",
                                 local_d3d11_records,
                                   global_d3d11_records );

    if ( state.hooks_loaded.from_shared_dll > 0 ||
         state.hooks_loaded.from_game_ini   > 0 )
    {
      // For early loading UnX
      SK_D3D11_InitTextures ();

      quick_hooked = true;

      SK_ApplyQueuedHooks ();
    }

    else
    {
      for ( auto& it : local_d3d11_records )
      {
        it->active = false;
      }
    }

    InterlockedIncrement (&hooked);
  }

  else
    SK_Thread_SpinUntilAtomicMin (&hooked, 2);
}


bool
SK_D3D11_QuickHooked (void)
{
  return quick_hooked;
}

int
SK_D3D11_PurgeHookAddressCache (void)
{
  int i = 0;

  for ( auto& it : local_d3d11_records )
  {
    SK_Hook_RemoveTarget ( *it, L"D3D11.Hooks" );

    ++i;
  }

  return i;
}

void
SK_D3D11_UpdateHookAddressCache (void)
{
  if (! config.apis.dxgi.d3d11.hook)
    return;

  for ( auto& it : local_d3d11_records )
  {
    if (it->active)
    {
      SK_Hook_ResolveTarget (*it);

      // Don't cache addresses that were screwed with by other injectors
      const wchar_t* wszSection =
        StrStrIW (it->target.module_path, LR"(d3d11.dll)") ?
                                            L"D3D11.Hooks" : nullptr;


      if (! wszSection)
      {
        SK_LOG0 ( ( L"Hook for '%hs' resides in '%s', will not cache!",
                      it->target.symbol_name,
          SK_ConcealUserDir (
            std::wstring (
                      it->target.module_path
                         ).data ()
          )       ),
                    L"Hook Cache" );
      }

      else
        SK_Hook_CacheTarget ( *it, wszSection );
    }
  }

  auto it_local  = std::begin (local_d3d11_records);
  auto it_global = std::begin (global_d3d11_records);

  while ( it_local != std::end (local_d3d11_records) )
  {
    if ( ( *it_local )->hits               &&
         ( *it_local )->target.module_path &&
         ( *it_local )->active)
      SK_Hook_PushLocalCacheOntoGlobal ( **it_local,
                                           **it_global );
    else
    {
      ( *it_global )->target.addr = nullptr;
      ( *it_global )->hits        = 0;
      ( *it_global )->active      = false;
    }

    it_global++, it_local++;
  }
}

#ifdef _WIN64
#pragma comment (linker, "/export:DirectX::ScratchImage::Release=?Release@ScratchImage@DirectX@@QEAAXXZ")
#else
#pragma comment (linker, "/export:DirectX::ScratchImage::Release=?Release@ScratchImage@DirectX@@QAAXXZ")
#endif

HRESULT
__cdecl
SK_DXTex_CreateTexture ( _In_reads_(nimages) const DirectX::Image*       srcImages,
                         _In_                      size_t                nimages,
                         _In_                const DirectX::TexMetadata& metadata,
                         _Outptr_                  ID3D11Resource**      ppResource )
{
  return
    DirectX::CreateTexture ( (ID3D11Device *)SK_Render_GetDevice (),
                               srcImages, nimages, metadata, ppResource );
}


UINT
SK_D3D11_MakeDebugFlags (UINT uiOrigFlags)
{
  if (config.render.dxgi.ignore_thread_flags)
  {
    if ( (uiOrigFlags & D3D11_CREATE_DEVICE_SINGLETHREADED) ||
         (uiOrigFlags & D3D11_CREATE_DEVICE_PREVENT_INTERNAL_THREADING_OPTIMIZATIONS) )
    {
      uiOrigFlags &= ~D3D11_CREATE_DEVICE_SINGLETHREADED;
      uiOrigFlags &= ~D3D11_CREATE_DEVICE_PREVENT_INTERNAL_THREADING_OPTIMIZATIONS;

      SK_LOGi0 (L">> Removing Singlethreaded Device Flags");
    }
  }

  //UINT Flags =  (D3D11_CREATE_DEVICE_DEBUG | uiOrigFlags);
  //     Flags &= ~D3D11_CREATE_DEVICE_PREVENT_ALTERING_LAYER_SETTINGS_FROM_REGISTRY;

  return uiOrigFlags;// Flags;
}

//static concurrency::concurrent_unordered_map <HWND, std::pair <ID3D11Device*, IDXGISwapChain*>> _discarded;
static concurrency::concurrent_unordered_map <HWND, std::pair <ID3D11Device*, IDXGISwapChain*>> _recyclables;

std::pair <ID3D11Device*, IDXGISwapChain*>
SK_D3D11_GetCachedDeviceAndSwapChainForHwnd (HWND hWnd)
{
  std::pair <ID3D11Device*, IDXGISwapChain*> pDevChain =
    std::make_pair <ID3D11Device *, IDXGISwapChain *> (
      nullptr, nullptr
    );

  if ( _recyclables.count (hWnd) != 0 )
  {
    pDevChain =
      _recyclables.at (hWnd);
  }

  return pDevChain;
}

std::pair <ID3D11Device*, IDXGISwapChain*>
SK_D3D11_MakeCachedDeviceAndSwapChainForHwnd (IDXGISwapChain* pSwapChain, HWND hWnd, ID3D11Device* pDevice)
{
  std::pair <ID3D11Device*, IDXGISwapChain*> pDevChain =
    std::make_pair (
      pDevice, pSwapChain
    );

    _recyclables [hWnd] =
      pDevChain;

  return
    _recyclables [hWnd];
}

UINT
SK_D3D11_ReleaseDeviceOnHWnd (IDXGISwapChain1* pChain, HWND hWnd, IUnknown* pDevice)
{
#ifdef _DEBUG
  auto* pValidate =
    _recyclables [hWnd];

  assert (pValidate == pChain);
#endif

  DBG_UNREFERENCED_PARAMETER (pDevice);
  DBG_UNREFERENCED_PARAMETER (pChain);

  UINT ret =
    std::numeric_limits <UINT>::max ();

  if (_recyclables.count (hWnd) != 0)
    ret = 0;

  _recyclables [hWnd] =
    std::make_pair <ID3D11Device*, IDXGISwapChain*> (
      nullptr, nullptr
    );

//_discarded [hWnd][pDevice] = pChain;

  return ret;
}

extern bool
SK_DXGI_IsSwapChainReal (const DXGI_SWAP_CHAIN_DESC& desc) noexcept;

__declspec (noinline)
HRESULT
WINAPI
D3D11CreateDeviceAndSwapChain_Detour (IDXGIAdapter          *pAdapter,
                                      D3D_DRIVER_TYPE        DriverType,
                                      HMODULE                Software,
                                      UINT                   Flags,
 _In_reads_opt_ (FeatureLevels) CONST D3D_FEATURE_LEVEL     *pFeatureLevels,
                                      UINT                   FeatureLevels,
                                      UINT                   SDKVersion,
 _In_opt_                       CONST DXGI_SWAP_CHAIN_DESC  *pSwapChainDesc,
 _Out_opt_                            IDXGISwapChain       **ppSwapChain,
 _Out_opt_                            ID3D11Device         **ppDevice,
 _Out_opt_                            D3D_FEATURE_LEVEL     *pFeatureLevel,
 _Out_opt_                            ID3D11DeviceContext  **ppImmediateContext)
{
    bool bEOSOverlay = false;

#ifdef _M_AMD64
  if ( SK_GetCallingDLL () ==
       SK_GetModuleHandle ( L"EOSOVH-Win64-Shipping.dll" )
     ) bEOSOverlay = true;
#else
    if ( SK_GetCallingDLL () ==
       SK_GetModuleHandle ( L"EOSOVH-Win32-Shipping.dll" )
     ) bEOSOverlay = true;
#endif

  if ((! bEOSOverlay) || config.system.log_level > 0)
  {
    DXGI_LOG_CALL_1 (L"D3D11CreateDeviceAndSwapChain", L"Flags=0x%x", Flags );
  }

  if (bEOSOverlay || (pSwapChainDesc != nullptr && ! SK_DXGI_IsSwapChainReal (*pSwapChainDesc)))
  {
    return
      D3D11CreateDeviceAndSwapChain_Import ( pAdapter,
                                               DriverType,
                                                 Software,
                                                   Flags,
                                                     pFeatureLevels,
                                                       FeatureLevels,
                                                         SDKVersion,
                                                           pSwapChainDesc,
                                                             ppSwapChain,
                                                               ppDevice,
                                                                 pFeatureLevel,
                                                                   ppImmediateContext );
  }

  Flags =
    SK_D3D11_MakeDebugFlags (Flags);

  static SK_RenderBackend_V2& rb =
    SK_GetCurrentRenderBackend ();

  // Even if the game doesn't care about the feature level, we do.
  D3D_FEATURE_LEVEL ret_level  = D3D_FEATURE_LEVEL_10_0;
  ID3D11Device*     ret_device = nullptr;

  // Allow override of swapchain parameters
  auto swap_chain_override =
    std::make_unique <DXGI_SWAP_CHAIN_DESC> ( pSwapChainDesc != nullptr ?
                                                *pSwapChainDesc :
                                                DXGI_SWAP_CHAIN_DESC { }
                                            );

  auto swap_chain_desc =
    swap_chain_override.get ();

  WaitForInitD3D11 ();

  SK_D3D11_Init ();

  dll_log->LogEx ( true,
                     L"[  D3D 11  ]  <~> Preferred Feature Level(s): <%u> - %hs\n",
                       FeatureLevels,
                         SK_DXGI_FeatureLevelsToStr (
                           FeatureLevels,
                             reinterpret_cast <const DWORD *> (pFeatureLevels)
                         ).c_str ()
                 );

  // Optionally Enable Debug Layer
  if (ReadAcquire (&__d3d11_ready) != 0)
  {
    if (config.render.dxgi.debug_layer && ((Flags & D3D11_CREATE_DEVICE_DEBUG)
                                                 != D3D11_CREATE_DEVICE_DEBUG))
    {
      SK_LOG0 ( ( L" ==> Enabling D3D11 Debug layer" ),
                  __SK_SUBSYSTEM__ );

      Flags |= D3D11_CREATE_DEVICE_DEBUG;
    }
  }


  SK_D3D11_RemoveUndesirableFlags (&Flags);


  //
  // DXGI Adapter Override (for performance)
  //

  SK_DXGI_AdapterOverride ( &pAdapter, &DriverType );

  if ( pSwapChainDesc != nullptr &&
          ppSwapChain != nullptr )
  {
    wchar_t wszMSAA [128] = { };

    swprintf ( wszMSAA, swap_chain_desc->SampleDesc.Count > 1 ?
                          L"%u Samples" :
                          L"Not Used (or Offscreen)",
                 swap_chain_desc->SampleDesc.Count );

    dll_log->LogEx ( true,
      L"[Swap Chain]\n"
      L"  +-------------+-------------------------------------------------------------------------+\n"
      L"  | Resolution. |  %4lux%4lu @ %6.2f Hz%-50ws|\n"
      L"  | Format..... |  %-71hs|\n"
      L"  | Buffers.... |  %-2lu%-69ws|\n"
      L"  | MSAA....... |  %-71ws|\n"
      L"  | Mode....... |  %-71ws|\n"
      L"  | Scaling.... |  %-71ws|\n"
      L"  | Scanlines.. |  %-71ws|\n"
      L"  | Flags...... |  0x%04x%-65ws|\n"
      L"  | SwapEffect. |  %-71ws|\n"
      L"  +-------------+-------------------------------------------------------------------------+\n",
          swap_chain_desc->BufferDesc.Width,
          swap_chain_desc->BufferDesc.Height,
          swap_chain_desc->BufferDesc.RefreshRate.Denominator != 0 ?
            static_cast <float> (swap_chain_desc->BufferDesc.RefreshRate.Numerator) /
            static_cast <float> (swap_chain_desc->BufferDesc.RefreshRate.Denominator) :
              std::numeric_limits <float>::quiet_NaN (), L" ",
    SK_DXGI_FormatToStr (swap_chain_desc->BufferDesc.Format).data (),
          swap_chain_desc->BufferCount, L" ",
          wszMSAA,
          swap_chain_desc->Windowed ? L"Windowed" : L"Fullscreen",
          swap_chain_desc->BufferDesc.Scaling == DXGI_MODE_SCALING_UNSPECIFIED ?
            L"Unspecified" :
            swap_chain_desc->BufferDesc.Scaling == DXGI_MODE_SCALING_CENTERED ?
              L"Centered" :
              L"Stretched",
          swap_chain_desc->BufferDesc.ScanlineOrdering == DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED ?
            L"Unspecified" :
            swap_chain_desc->BufferDesc.ScanlineOrdering == DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE ?
              L"Progressive" :
              swap_chain_desc->BufferDesc.ScanlineOrdering == DXGI_MODE_SCANLINE_ORDER_UPPER_FIELD_FIRST ?
                L"Interlaced Even" :
                L"Interlaced Odd",
          swap_chain_desc->Flags, L" ",
          swap_chain_desc->SwapEffect         == 0 ?
            L"Discard" :
            swap_chain_desc->SwapEffect       == 1 ?
              L"Sequential" :
              swap_chain_desc->SwapEffect     == 2 ?
                L"<Unknown>" :
                swap_chain_desc->SwapEffect   == 3 ?
                  L"Flip Sequential" :
                  swap_chain_desc->SwapEffect == 4 ?
                    L"Flip Discard" :
                    L"<Unknown>" );

    ///swap_chain_override = *swap_chain_desc;
    ///swap_chain_desc     = &swap_chain_override;

    if ( config.render.dxgi.scaling_mode      != -1 &&
          swap_chain_desc->BufferDesc.Scaling !=
            static_cast <DXGI_MODE_SCALING> (config.render.dxgi.scaling_mode) )
    {
      SK_LOG0 ( ( L" >> Scaling Override "
                  L"(Requested: %hs, Using: %hs)",
                      SK_DXGI_DescribeScalingMode (
                        swap_chain_desc->BufferDesc.Scaling
                      ),
                        SK_DXGI_DescribeScalingMode (
            static_cast <DXGI_MODE_SCALING> (config.render.dxgi.scaling_mode)
                                                    ) ), __SK_SUBSYSTEM__ );

      swap_chain_desc->BufferDesc.Scaling =
        static_cast <DXGI_MODE_SCALING> (config.render.dxgi.scaling_mode);
    }

    if (! config.window.res.override.isZero ())
    {
      swap_chain_desc->BufferDesc.Width  = config.window.res.override.x;
      swap_chain_desc->BufferDesc.Height = config.window.res.override.y;
    }

    else
    {
      SK_DXGI_BorderCompensation (
        swap_chain_desc->BufferDesc.Width,
          swap_chain_desc->BufferDesc.Height
      );
    }
  }

  auto pDevCache =
    SK_D3D11_GetCachedDeviceAndSwapChainForHwnd (swap_chain_desc->OutputWindow);

  if (pDevCache.first != nullptr)
  {
    if (ppDevice != nullptr) {
       *ppDevice = pDevCache.first;
      (*ppDevice)->AddRef ();
    }

    if (ppSwapChain != nullptr) {
      *ppSwapChain = pDevCache.second;
     (*ppSwapChain)->AddRef ();
    }

    dll_log->Log (L" ### Returned Cached D3D11 Device");

    return S_OK;
  }


  HRESULT res = E_UNEXPECTED;

  DXGI_CALL (res,
    D3D11CreateDeviceAndSwapChain_Import ( pAdapter,
                                             DriverType,
                                               Software,
                                                 Flags,
                                                   pFeatureLevels,
                                                     FeatureLevels,
                                                       SDKVersion,
                                                         swap_chain_desc,
                                                           ppSwapChain,
                                                             &ret_device,
                                                               &ret_level,
                                                                 ppImmediateContext )
            );


  if (SUCCEEDED (res) && ppDevice != nullptr)
  {
    if ( ppSwapChain    != nullptr &&
         pSwapChainDesc != nullptr    )
    {
      const bool dummy_window =
        SK_Win32_IsDummyWindowClass (swap_chain_desc->OutputWindow);

      if (! dummy_window)
      {
        auto& windows =
          rb.windows;

        if ( ReadULongAcquire (&rb.thread) == 0x00 ||
             ReadULongAcquire (&rb.thread) == SK_Thread_GetCurrentId () )
        {
          if (                windows.device != nullptr    &&
               swap_chain_desc->OutputWindow != nullptr    &&
               swap_chain_desc->OutputWindow != windows.device )
            SK_LOG0 ( (L"Game created a new window?!"), __SK_SUBSYSTEM__ );
        }

        else
        {
          wchar_t                          wszClass [128] = { };
          RealGetWindowClassW (
            swap_chain_desc->OutputWindow, wszClass, 127 );

          SK_LOG0 ( ( L"Installing Window Hooks for Window Class: '%ws'", wszClass ),
                      __SK_SUBSYSTEM__ );

          static HWND hWndLast =
            swap_chain_desc->OutputWindow;

          if ((! IsWindow (windows.getDevice ())) || hWndLast != swap_chain_desc->OutputWindow)
          {
            hWndLast            = swap_chain_desc->OutputWindow;
            windows.setDevice    (swap_chain_desc->OutputWindow);
            SK_InstallWindowHook (swap_chain_desc->OutputWindow);
          }

          else
          {
            SK_LOG0 ( ( L"Ignored because a window hook already exists..."),
                        __SK_SUBSYSTEM__ );
          }
        }
      }
    }

    ////if (SK_GetCurrentGameID () == SK_GAME_ID::Tales_of_Vesperia)
    {
#ifdef SK_D3D11_WRAP_IMMEDIATE_CTX
      if (ppImmediateContext != nullptr)
      {
        ID3D11DeviceContext *pImmediate =
          *ppImmediateContext;

        if ( wrapped_contexts->find (pImmediate) ==
             wrapped_contexts->cend (          )  )
        {
          (*wrapped_contexts)[pImmediate] =
            SK_ComPtr <ID3D11DeviceContext4> (
              SK_D3D11_WrapperFactory->wrapDeviceContext (pImmediate)
            );
        }

        (*wrapped_contexts)[pImmediate].
          QueryInterface <ID3D11DeviceContext> (
                           ppImmediateContext
          );

        (*wrapped_immediates)[*ppDevice] =
        (*wrapped_contexts)[pImmediate];
      }
#endif
    }

    // Assume the first thing to create a D3D11 render device is
    //   the game and that devices never migrate threads; for most games
    //     this assumption holds.
    if ( ReadULongAcquire (&rb.thread) == 0x00 ||
         ReadULongAcquire (&rb.thread) == SK_Thread_GetCurrentId () )
    {
      WriteULongRelease (&rb.thread, SK_Thread_GetCurrentId ());
    }

    SK_D3D11_SetDevice ( &ret_device, ret_level );

    if (swap_chain_desc != nullptr && swap_chain_desc->OutputWindow != 0) {
      SK_D3D11_MakeCachedDeviceAndSwapChainForHwnd ( ppSwapChain != nullptr ?
                                                               *ppSwapChain : nullptr,
                                                       swap_chain_desc->OutputWindow,
                                                              ret_device );
                                                              ret_device->AddRef ();
    }
  }

  if (ppDevice != nullptr)
    *ppDevice   = ret_device;

  if (pFeatureLevel != nullptr)
    *pFeatureLevel   = ret_level;

  if (ppDevice != nullptr && SUCCEEDED (res))
  {
    D3D11_FEATURE_DATA_D3D11_OPTIONS options;
    (*ppDevice)->CheckFeatureSupport ( D3D11_FEATURE_D3D11_OPTIONS,
                                         &options, sizeof (options) );

    d3d11_caps.MapNoOverwriteOnDynamicConstantBuffer =
      (options.MapNoOverwriteOnDynamicConstantBuffer != FALSE);
  }

  return res;
}

__declspec (noinline)
HRESULT
WINAPI
D3D11CreateDevice_Detour (
  _In_opt_                            IDXGIAdapter         *pAdapter,
                                      D3D_DRIVER_TYPE       DriverType,
                                      HMODULE               Software,
                                      UINT                  Flags,
  _In_opt_                      const D3D_FEATURE_LEVEL    *pFeatureLevels,
                                      UINT                  FeatureLevels,
                                      UINT                  SDKVersion,
  _Out_opt_                           ID3D11Device        **ppDevice,
  _Out_opt_                           D3D_FEATURE_LEVEL    *pFeatureLevel,
  _Out_opt_                           ID3D11DeviceContext **ppImmediateContext)
{
  Flags =
    SK_D3D11_MakeDebugFlags (Flags);

  SK_TLS *pTLS =
    SK_TLS_Bottom ();

  auto& pTLS_d3d11 =
    pTLS->render->d3d11.get ();


  // Optionally Enable Debug Layer
  if (ReadAcquire (&__d3d11_ready) != 0)
  {
    if (config.render.dxgi.debug_layer && ((Flags & D3D11_CREATE_DEVICE_DEBUG)
                                                 != D3D11_CREATE_DEVICE_DEBUG))
    {
      SK_LOG0 ( ( L" ==> Enabling D3D11 Debug layer" ),
                  __SK_SUBSYSTEM__ );

      Flags |= D3D11_CREATE_DEVICE_DEBUG;
    }
  }

  // Ignore Ansel
  const bool bAnsel =
    StrStrIW (SK_GetModuleName (SK_GetCallingDLL ()).c_str (), L"NvCamera");

  if (pTLS_d3d11.skip_d3d11_create_device != FALSE || bAnsel)
  {
    HRESULT hr =
      D3D11CreateDevice_Import ( pAdapter, DriverType, Software, Flags,
                                   pFeatureLevels, FeatureLevels, SDKVersion,
                                     ppDevice, pFeatureLevel,
                                       ppImmediateContext );

    if (! bAnsel)
      pTLS->render->d3d11->skip_d3d11_create_device = FALSE;

    return hr;
  }

  DXGI_LOG_CALL_1 (L"D3D11CreateDevice            ", L"Flags=0x%x", Flags);

  {
    SK_ScopedBool auto_bool_skip (
      &pTLS->render->d3d11->skip_d3d11_create_device
    ); pTLS->render->d3d11->skip_d3d11_create_device = TRUE;

    SK_D3D11_Init ();
  }

  HRESULT hr =
    D3D11CreateDeviceAndSwapChain_Detour ( pAdapter, DriverType, Software, Flags,
                                             pFeatureLevels, FeatureLevels, SDKVersion,
                                               nullptr, nullptr, ppDevice, pFeatureLevel,
                                                 ppImmediateContext );

  return hr;
}


__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11Dev_CreateDeferredContext_Override (
  _In_            ID3D11Device         *This,
  _In_            UINT                  ContextFlags,
  _Out_opt_       ID3D11DeviceContext **ppDeferredContext )
{
  static bool bACO =
    (SK_GetCurrentGameID () == SK_GAME_ID::AssassinsCreed_Odyssey);

  bool silent = bACO;

  if ((! silent) && SK_GetModuleName (SK_GetCallingDLL ()).find (L"TwitchNativeOverlay") != std::wstring::npos)
    silent = true;

  // STFU and use a single context Twitch
  if (! silent)
  {
    DXGI_LOG_CALL_2 ( L"ID3D11Device::CreateDeferredContext",
                      L"ContextFlags=0x%x, **ppDeferredContext=%p",
                        ContextFlags,        ppDeferredContext );
  }

  else
  {
    return
      D3D11Dev_CreateDeferredContext_Original (This, ContextFlags, ppDeferredContext);
  }

#ifdef SK_D3D11_WRAP_DEFERRED_CTX
  if (config.render.dxgi.deferred_isolation)
  {
    if (ppDeferredContext != nullptr)
    {
            ID3D11DeviceContext* pTemp = nullptr;
      const HRESULT              hr    =
        D3D11Dev_CreateDeferredContext_Original (This, ContextFlags, &pTemp);

      if (SUCCEEDED (hr))
      {
        if ( wrapped_contexts->find (pTemp) ==
             wrapped_contexts->cend (     )  )
        {
          (*wrapped_contexts)[pTemp] =
            SK_ComPtr <ID3D11DeviceContext4> (
              SK_D3D11_WrapperFactory->wrapDeviceContext (pTemp)
            );
        }

        (*wrapped_contexts)[pTemp].
          QueryInterface <ID3D11DeviceContext> (
                            ppDeferredContext
          );

        return hr;
      }

      *ppDeferredContext = pTemp;

      return hr;
    }

    return D3D11Dev_CreateDeferredContext_Original (This, ContextFlags, nullptr);
  }
#endif

  return D3D11Dev_CreateDeferredContext_Original (This, ContextFlags, ppDeferredContext);
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11Dev_CreateDeferredContext1_Override (
  _In_            ID3D11Device1         *This,
  _In_            UINT                   ContextFlags,
  _Out_opt_       ID3D11DeviceContext1 **ppDeferredContext1 )
{
  DXGI_LOG_CALL_2 ( L"ID3D11Device1::CreateDeferredContext1",
                    L"ContextFlags=0x%x, **ppDeferredContext=%p",
                      ContextFlags,        ppDeferredContext1 );

  if (ppDeferredContext1 != nullptr)
  {
          ID3D11DeviceContext1* pTemp = nullptr;
    const HRESULT               hr    =
      D3D11Dev_CreateDeferredContext1_Original (This, ContextFlags, &pTemp);

    if (SUCCEEDED (hr))
    {
      if ( wrapped_contexts->find (pTemp) ==
           wrapped_contexts->cend (     )  )
      {
        (*wrapped_contexts) [pTemp] =
          SK_ComPtr <ID3D11DeviceContext4> (
            SK_D3D11_WrapperFactory->wrapDeviceContext (pTemp)
          );
      }

      (*wrapped_contexts)[pTemp].
        QueryInterface <ID3D11DeviceContext1> (
                          ppDeferredContext1
        );

      return hr;
    }

    *ppDeferredContext1 = pTemp;

    return hr;
  }


  return D3D11Dev_CreateDeferredContext1_Original (This, ContextFlags, nullptr);
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11Dev_CreateDeferredContext2_Override (
  _In_            ID3D11Device2         *This,
  _In_            UINT                   ContextFlags,
  _Out_opt_       ID3D11DeviceContext2 **ppDeferredContext2 )
{
  DXGI_LOG_CALL_2 ( L"ID3D11Device2::CreateDeferredContext2",
                    L"ContextFlags=0x%x, **ppDeferredContext=%p",
                      ContextFlags,        ppDeferredContext2 );

  if (ppDeferredContext2 != nullptr)
  {
          ID3D11DeviceContext2* pTemp = nullptr;
    const HRESULT               hr    =
      D3D11Dev_CreateDeferredContext2_Original (This, ContextFlags, &pTemp);

    if (SUCCEEDED (hr))
    {
      if ( wrapped_contexts->find (pTemp) ==
           wrapped_contexts->cend (     )  )
      {
        (*wrapped_contexts)[pTemp] =
          SK_ComPtr <ID3D11DeviceContext4> (
            SK_D3D11_WrapperFactory->wrapDeviceContext (pTemp)
          );
      }

      (*wrapped_contexts)[pTemp].
        QueryInterface <ID3D11DeviceContext2> (
                          ppDeferredContext2
        );

      return hr;
    }

    *ppDeferredContext2 = pTemp;

    return hr;
  }

  return D3D11Dev_CreateDeferredContext2_Original (This, ContextFlags, nullptr);
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11Dev_CreateDeferredContext3_Override (
  _In_            ID3D11Device3         *This,
  _In_            UINT                   ContextFlags,
  _Out_opt_       ID3D11DeviceContext3 **ppDeferredContext3 )
{
  DXGI_LOG_CALL_2 ( L"ID3D11Device3::CreateDeferredContext3",
                    L"ContextFlags=0x%x, **ppDeferredContext=%p",
                      ContextFlags,        ppDeferredContext3 );

  if (ppDeferredContext3 != nullptr)
  {
          ID3D11DeviceContext3* pTemp = nullptr;
    const HRESULT               hr    =
      D3D11Dev_CreateDeferredContext3_Original (This, ContextFlags, &pTemp);

    if (SUCCEEDED (hr))
    {
      if ( wrapped_contexts->find (pTemp) ==
           wrapped_contexts->cend (     )  )
      {
        (*wrapped_contexts)[pTemp] =
          SK_ComPtr <ID3D11DeviceContext4> (
            SK_D3D11_WrapperFactory->wrapDeviceContext (pTemp)
          );
      }

      (*wrapped_contexts)[pTemp].
        QueryInterface <ID3D11DeviceContext3> (
                          ppDeferredContext3
        );

      return hr;
    }

    *ppDeferredContext3 = pTemp;

    return hr;
  }

  return D3D11Dev_CreateDeferredContext3_Original (This, ContextFlags, nullptr);
}

_declspec (noinline)
void
STDMETHODCALLTYPE
D3D11Dev_GetImmediateContext_Override (
  _In_            ID3D11Device         *This,
  _Out_           ID3D11DeviceContext **ppImmediateContext )
{
  if (config.system.log_level > 1)
  {
    DXGI_LOG_CALL_0 (L"ID3D11Device::GetImmediateContext");
  }

#ifdef SK_D3D11_WRAP_IMMEDIATE_CTX
  if (config.render.dxgi.deferred_isolation)
  {
    if (ppImmediateContext != nullptr && wrapped_immediates->find (This) !=
                                         wrapped_immediates->cend (    ))
    {
      (*wrapped_immediates) [This].
        QueryInterface <ID3D11DeviceContext> (
                         ppImmediateContext
        );
      return;
    }
  }
#endif

  ID3D11DeviceContext* pCtx = nullptr;

  D3D11Dev_GetImmediateContext_Original (This, &pCtx);

  if (ppImmediateContext != nullptr)
     *ppImmediateContext = pCtx;
}

_declspec (noinline)
void
STDMETHODCALLTYPE
D3D11Dev_GetImmediateContext1_Override (
  _In_            ID3D11Device1         *This,
  _Out_           ID3D11DeviceContext1 **ppImmediateContext1 )
{
  if (config.system.log_level > 1)
  {
    DXGI_LOG_CALL_0 (L"ID3D11Device1::GetImmediateContext1");
  }

  ID3D11DeviceContext1* pCtx1 = nullptr;

  D3D11Dev_GetImmediateContext1_Original (This, &pCtx1);

  if (ppImmediateContext1 != nullptr)
     *ppImmediateContext1 = pCtx1;
}

_declspec (noinline)
void
STDMETHODCALLTYPE
D3D11Dev_GetImmediateContext2_Override (
  _In_            ID3D11Device2         *This,
  _Out_           ID3D11DeviceContext2 **ppImmediateContext2 )
{
  if (config.system.log_level > 1)
  {
    DXGI_LOG_CALL_0 (L"ID3D11Device2::GetImmediateContext2");
  }

  ID3D11DeviceContext2* pCtx2 = nullptr;

  D3D11Dev_GetImmediateContext2_Original (This, &pCtx2);

  if (ppImmediateContext2 != nullptr)
     *ppImmediateContext2 = pCtx2;
}

_declspec (noinline)
void
STDMETHODCALLTYPE
D3D11Dev_GetImmediateContext3_Override (
  _In_            ID3D11Device3         *This,
  _Out_           ID3D11DeviceContext3 **ppImmediateContext3 )
{
  if (config.system.log_level > 1)
  {
    DXGI_LOG_CALL_0 (L"ID3D11Device3::GetImmediateContext3");
  }

  ID3D11DeviceContext3* pCtx3 = nullptr;

  D3D11Dev_GetImmediateContext3_Original (This, &pCtx3);

  if (ppImmediateContext3 != nullptr)
     *ppImmediateContext3 = pCtx3;
}


void
SK_D3D11_EndFrame (SK_TLS* pTLS)
{
  for ( auto end_frame_fn : plugin_mgr->end_frame_fns )
  {
    end_frame_fn ();
  }

  // There is generally only one case where this happens:
  //
  //   Late-injection into an already running game
  //
  //  * This is recoverable and by the next full frame
  //      all of SK's Critical Sections will be setup
  //
  SK_ReleaseAssert (cs_render_view != nullptr)

  if (!cs_render_view)      // Skip this frame, we'll get it
  {                         //   on the next go-around.
    SK_D3D11_InitMutexes ();
    SK_LOG0 ( ( L"Critical Sections were not setup prior to drawing!" ),
                L"[  D3D 11  ]");
    return;
  }

  static auto& rb =
    SK_GetCurrentRenderBackend ();

  static auto& shaders =
    SK_D3D11_Shaders;

  dwFrameTime = SK::ControlPanel::current_time;

  SK_Screenshot_D3D11_RestoreHUD ();
  SK_Screenshot_D3D11_EndFrame   ();


#ifdef TRACK_THREADS
  {
    std::scoped_lock <SK_Thread_HybridSpinlock>
           auto_lock (*cs_render_view);

    SK_D3D11_MemoryThreads->clear_active   ();
    SK_D3D11_ShaderThreads->clear_active   ();
    SK_D3D11_DrawThreads->clear_active     ();
    SK_D3D11_DispatchThreads->clear_active ();
  }
#endif

  //for ( auto& it : shaders.reshade_triggered )
  //            it = false;
  shaders->reshade_triggered = false;

  {
    std::scoped_lock <SK_Thread_HybridSpinlock>
           auto_lock (*cs_render_view);

    RtlSecureZeroMemory ( reshade_trigger_before->data (),
                          reshade_trigger_before->size () * sizeof (bool) );
    RtlSecureZeroMemory ( reshade_trigger_after->data  (),
                          reshade_trigger_after->size  () * sizeof (bool) );
  }

  static auto& vertex   = shaders->vertex;
  static auto& pixel    = shaders->pixel;
  static auto& geometry = shaders->geometry;
  static auto& domain   = shaders->domain;
  static auto& hull     = shaders->hull;
  static auto& compute  = shaders->compute;

  {
    const UINT dev_idx =
      SK_D3D11_GetDeviceContextHandle (rb.d3d11.immediate_ctx);

    std::scoped_lock <SK_Thread_HybridSpinlock>
           auto_lock (*cs_render_view);

    vertex.tracked.deactivate   (nullptr, dev_idx);
    pixel.tracked.deactivate    (nullptr, dev_idx);
    geometry.tracked.deactivate (nullptr, dev_idx);
    hull.tracked.deactivate     (nullptr, dev_idx);
    domain.tracked.deactivate   (nullptr, dev_idx);
    compute.tracked.deactivate  (nullptr, dev_idx);

    if (dev_idx < SK_D3D11_MAX_DEV_CONTEXTS)
    {
      RtlSecureZeroMemory (vertex.current.views   [dev_idx], sizeof (ID3D11ShaderResourceView*) * 128);
      RtlSecureZeroMemory (pixel.current.views    [dev_idx], sizeof (ID3D11ShaderResourceView*) * 128);
      RtlSecureZeroMemory (geometry.current.views [dev_idx], sizeof (ID3D11ShaderResourceView*) * 128);
      RtlSecureZeroMemory (domain.current.views   [dev_idx], sizeof (ID3D11ShaderResourceView*) * 128);
      RtlSecureZeroMemory (hull.current.views     [dev_idx], sizeof (ID3D11ShaderResourceView*) * 128);
      RtlSecureZeroMemory (compute.current.views  [dev_idx], sizeof (ID3D11ShaderResourceView*) * 128);
    }
  }


  {
    std::scoped_lock <SK_Thread_HybridSpinlock>
           auto_lock (*cs_render_view);
    tracked_rtv->clear   ();

    ////for ( auto& it : *used_textures ) it->Release ();

    used_textures->clear ();
    mem_map_stats->clear ();
  }

  // True if the disjoint query is complete and we can get the results of
  //   each tracked shader's timing
  static bool disjoint_done = false;

  auto pDev =
    rb.getDevice <ID3D11Device> ();

  SK_ComQIPtr <ID3D11DeviceContext> pDevCtx (rb.d3d11.immediate_ctx);

  if (! ( pDevCtx != nullptr &&
          pDev    != nullptr ) )
    return;


  // End the Query and probe results (when the pipeline has drained)
  if ( pDevCtx != nullptr && (! disjoint_done) &&
       ReadPointerAcquire (
         (volatile PVOID *)&d3d11_shader_tracking_s::disjoint_query.async
                          )
     )
  {
    if (ReadAcquire (&d3d11_shader_tracking_s::disjoint_query.active) != FALSE)
    {
      pDevCtx->End (
        (ID3D11Asynchronous  *)ReadPointerAcquire (
             (volatile PVOID *)&d3d11_shader_tracking_s::disjoint_query.async)
                   );
      InterlockedExchange ( &d3d11_shader_tracking_s::disjoint_query.active,
                              FALSE );
    }

    else
    {
      HRESULT const hr = pDevCtx->GetData (
        (ID3D11Asynchronous *)ReadPointerAcquire (
            (volatile PVOID *)&d3d11_shader_tracking_s::disjoint_query.async),
                              &d3d11_shader_tracking_s::disjoint_query.last_results,
                        sizeof D3D11_QUERY_DATA_TIMESTAMP_DISJOINT, 0x0
                                          );

      if (hr == S_OK)
      {
        ((ID3D11Asynchronous *)ReadPointerAcquire (
          (volatile PVOID*)&d3d11_shader_tracking_s::disjoint_query.async)
        )->Release ();

        InterlockedExchangePointer (
          (void **)&d3d11_shader_tracking_s::disjoint_query.async, nullptr
        );

        // Check for failure, if so, toss out the results.
        if (FALSE == d3d11_shader_tracking_s::disjoint_query.last_results.Disjoint)
          disjoint_done = true;

        else
        {
          auto ClearTimers =
          [](d3d11_shader_tracking_s* tracker)
          {
            for (auto& it : tracker->timers)
            {
              SK_COM_ValidateRelease ((IUnknown **)&it.start.async);
              SK_COM_ValidateRelease ((IUnknown **)&it.end.async);

              SK_COM_ValidateRelease ((IUnknown **)&it.start.dev_ctx);
              SK_COM_ValidateRelease ((IUnknown **)&it.end.dev_ctx);
            }

            tracker->timers.clear ();
          };

          ClearTimers (&vertex.tracked);
          ClearTimers (&pixel.tracked);
          ClearTimers (&geometry.tracked);
          ClearTimers (&hull.tracked);
          ClearTimers (&domain.tracked);
          ClearTimers (&compute.tracked);

          disjoint_done = true;
        }
      }
    }
  }

  if (pDevCtx != nullptr && disjoint_done)
  {
   const
    auto
     GetTimerDataStart =
     []( d3d11_shader_tracking_s::duration_s *duration,
         bool                                &success   ) ->
      UINT64
      {
        ID3D11DeviceContext* dev_ctx =
          (ID3D11DeviceContext *)ReadPointerAcquire (
            (volatile PVOID *)&duration->start.dev_ctx
          );

        if (             dev_ctx != nullptr &&
             SUCCEEDED ( dev_ctx->GetData (
               (ID3D11Query *)ReadPointerAcquire
                 ((volatile PVOID *)&duration->start.async),
                                    &duration->start.last_results,
                                      sizeof UINT64, 0x00
                                         )
                       )
           )
        {
          SK_COM_ValidateRelease ((IUnknown **)&duration->start.async);
          SK_COM_ValidateRelease ((IUnknown **)&duration->start.dev_ctx);

          success = true;

          return duration->start.last_results;
        }

        success = false;

        return 0;
      };

   const
    auto
     GetTimerDataEnd =
     []( d3d11_shader_tracking_s::duration_s *duration,
         bool                                &success ) ->
      UINT64
      {
        if ( (ID3D11Query *)ReadPointerAcquire (
               (volatile PVOID *)&duration->end.async
                                               ) == nullptr )
        {
          return duration->start.last_results;
        }

        ID3D11DeviceContext* dev_ctx =
          (ID3D11DeviceContext *)ReadPointerAcquire (
               (volatile PVOID *)&duration->end.dev_ctx
          );

        if (             dev_ctx != nullptr &&
             SUCCEEDED ( dev_ctx->GetData (
               (ID3D11Query *)ReadPointerAcquire
                    ((volatile PVOID *)&duration->end.async),
                                       &duration->end.last_results,
                                         sizeof UINT64, 0x00
                                          )
                       )
           )
        {
          SK_COM_ValidateRelease ((IUnknown **)&duration->end.async);
          SK_COM_ValidateRelease ((IUnknown **)&duration->end.dev_ctx);

          success = true;

          return duration->end.last_results;
        }

        success = false;

        return 0;
      };

    auto CalcRuntimeMS =
    [ ](d3d11_shader_tracking_s *tracker) noexcept
     {
      if (tracker->runtime_ticks != 0ULL)
      {
        tracker->runtime_ms =
          1000.0 * sk::narrow_cast <double>
          (        static_cast <long double>    (
                 tracker->runtime_ticks.load () ) /
                   static_cast <long double>     (
                 d3d11_shader_tracking_s::disjoint_query.last_results.Frequency)
          );

         // Way too long to be valid, just re-use the last known good value
         if ( tracker->runtime_ms > 12.0 )
              tracker->runtime_ms = tracker->last_runtime_ms;

         tracker->last_runtime_ms =
              tracker->runtime_ms;
       }

       else
       {
         tracker->runtime_ms = 0.0;
       }
     };

    const
     auto
      AccumulateRuntimeTicks =
      [&](       d3d11_shader_tracking_s             *tracker,
           const std::unordered_map <uint32_t, LONG> &blacklist )
      {
        tracker->runtime_ticks = 0ULL;

        for ( auto& it : tracker->timers )
        {
          bool success0 = false,
               success1 = false;

          const UINT64
            time1 = GetTimerDataStart (&it, success0);

          const UINT64 time0 =
                 ( success0 == false ) ? 0ULL :
                      GetTimerDataEnd (&it, success1);

          if ( success0 != false &&
               success1 != false )
          {
            tracker->runtime_ticks +=
              ( time0 - time1 );
          }

          // Data's no good, we need to release the queries manually or
          //   we're going to leak!
          else
          {
            SK_COM_ValidateRelease ((IUnknown **)&it.end.async);
            SK_COM_ValidateRelease ((IUnknown **)&it.end.dev_ctx);

            SK_COM_ValidateRelease ((IUnknown **)&it.start.async);
            SK_COM_ValidateRelease ((IUnknown **)&it.start.dev_ctx);
          }
        }


        if (   tracker->cancel_draws   ||
               tracker->num_draws == 0 || blacklist.count
             ( tracker->crc32c )   > 0
           )
        {
          tracker->runtime_ticks   = 0ULL;
          tracker->runtime_ms      = 0.0;
          tracker->last_runtime_ms = 0.0;
        }

        tracker->timers.clear ();
      };

    AccumulateRuntimeTicks (&vertex.tracked,   vertex.blacklist);
    CalcRuntimeMS          (&vertex.tracked);

    AccumulateRuntimeTicks (&pixel.tracked,    pixel.blacklist);
    CalcRuntimeMS          (&pixel.tracked);

    AccumulateRuntimeTicks (&geometry.tracked, geometry.blacklist);
    CalcRuntimeMS          (&geometry.tracked);

    AccumulateRuntimeTicks (&hull.tracked,     hull.blacklist);
    CalcRuntimeMS          (&hull.tracked);

    AccumulateRuntimeTicks (&domain.tracked,   domain.blacklist);
    CalcRuntimeMS          (&domain.tracked);

    AccumulateRuntimeTicks (&compute.tracked,  compute.blacklist);
    CalcRuntimeMS          (&compute.tracked);

    disjoint_done = false;
  }

  vertex.tracked.clear   ();
  pixel.tracked.clear    ();
  geometry.tracked.clear ();
  hull.tracked.clear     ();
  domain.tracked.clear   ();
  compute.tracked.clear  ();

  vertex.changes_last_frame   = 0;
  pixel.changes_last_frame    = 0;
  geometry.changes_last_frame = 0;
  hull.changes_last_frame     = 0;
  domain.changes_last_frame   = 0;
  compute.changes_last_frame  = 0;


  if (! SK_D3D11_ShowShaderModDlg ())
    SK_D3D11_EnableMMIOTracking = false;

  for (auto& it_ctx : *SK_D3D11_PerCtxResources )
  {
    int spins = 0;

    while (InterlockedCompareExchange (&it_ctx.writing_, 1, 0) != 0)
    {
      if ( ++spins > 0x1000 )
      {
        SleepEx (1, FALSE);
        spins = 0;
      }
    }

    const UINT dev_idx =
      SK_D3D11_GetDeviceContextHandle (rb.d3d11.immediate_ctx);

    if (it_ctx.ctx_id_ == dev_idx)
    {
      it_ctx.temp_resources.clear ();
      it_ctx.used_textures.clear  ();
    }

    SK_D3D11_RenderTargets [it_ctx.ctx_id_].clear ();

    InterlockedExchange (&it_ctx.writing_, 0);
  }

  {
    std::scoped_lock <SK_Thread_HybridSpinlock>
           auto_lock (*cs_render_view);

    SK_D3D11_TempResources->clear ();
  }

  SK_D3D11_Resampler_ProcessFinished (pDev, pDevCtx, pTLS);
}


std::unique_ptr <SK_Thread_HybridSpinlock> cs_shader      = nullptr;
std::unique_ptr <SK_Thread_HybridSpinlock> cs_shader_vs   = nullptr;
std::unique_ptr <SK_Thread_HybridSpinlock> cs_shader_ps   = nullptr;
std::unique_ptr <SK_Thread_HybridSpinlock> cs_shader_gs   = nullptr;
std::unique_ptr <SK_Thread_HybridSpinlock> cs_shader_hs   = nullptr;
std::unique_ptr <SK_Thread_HybridSpinlock> cs_shader_ds   = nullptr;
std::unique_ptr <SK_Thread_HybridSpinlock> cs_shader_cs   = nullptr;
std::unique_ptr <SK_Thread_HybridSpinlock> cs_mmio        = nullptr;
std::unique_ptr <SK_Thread_HybridSpinlock> cs_render_view = nullptr;

#if (VER_PRODUCTBUILD < 10011)
const GUID IID_ID3D11Device2 = { 0x9d06dffa, 0xd1e5, 0x4d07, { 0x83, 0xa8, 0x1b, 0xb1, 0x23, 0xf2, 0xf8, 0x41 } };
const GUID IID_ID3D11Device3 = { 0xa05c8c37, 0xd2c6, 0x4732, { 0xb3, 0xa0, 0x9c, 0xe0, 0xb0, 0xdc, 0x9a, 0xe6 } };
const GUID IID_ID3D11Device4 = { 0x8992ab71, 0x02e6, 0x4b8d, { 0xba, 0x48, 0xb0, 0x56, 0xdc, 0xda, 0x42, 0xc4 } };
const GUID IID_ID3D11Device5 = { 0x8ffde202, 0xa0e7, 0x45df, { 0x9e, 0x01, 0xe8, 0x37, 0x80, 0x1b, 0x5e, 0xa0 } };
#endif