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

#define __SK_SUBSYSTEM__ L"  D3D 11  "

#define _WIN7_PLATFORM_UPDATE

struct IUnknown;
#include <Unknwnbase.h>
#include <Windows.h>
#include <Wincodec.h>

#include <SpecialK/diagnostics/cpu.h>
#include <SpecialK/diagnostics/modules.h>
#include <SpecialK/diagnostics/load_library.h>
#include <SpecialK/render/dxgi/dxgi_backend.h>
#include <SpecialK/render/backend.h>

#include <SpecialK/import.h>

#include <SpecialK/diagnostics/debug_utils.h>
#include <SpecialK/diagnostics/compatibility.h>

#include <SpecialK/plugin/reshade.h>

#include <SpecialK/core.h>
#include <SpecialK/hooks.h>
#include <SpecialK/command.h>
#include <SpecialK/config.h>
#include <SpecialK/log.h>
#include <SpecialK/crc32.h>
#include <SpecialK/utility.h>
#include <SpecialK/thread.h>

#include <DirectXTex/DirectXTex.h>

#include <SpecialK/widgets/widget.h>
#include <SpecialK/framerate.h>
#include <SpecialK/tls.h>

#include <SpecialK/control_panel.h>

#include <gsl/gsl>

#define _SK_WITHOUT_D3DX11

extern LARGE_INTEGER SK_QueryPerf (void);
static DWORD         dwFrameTime = SK::ControlPanel::current_time; // For effects that blink; updated once per-frame.

#include <cinttypes>
#include <algorithm>
#include <memory>
#include <atomic>
#include <mutex>
#include <stack>
#include <concurrent_unordered_set.h>
#include <concurrent_queue.h>
#include <atlbase.h>

#include <d3d11.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>

#include <../src/render/d3d11/d3d11_dev_ctx.cpp>

#include <../depends/include/nvapi/nvapi.h>

bool
SK_D3D11_DrawCallFilter (int elem_cnt, int vtx_cnt, uint32_t vtx_shader);

// Only accessed by the swapchain thread and only to clear any outstanding
//   references prior to a buffer resize
std::vector <IUnknown *> temp_resources;

SK_Thread_HybridSpinlock* cs_shader      = nullptr; 
SK_Thread_HybridSpinlock* cs_shader_vs   = nullptr; 
SK_Thread_HybridSpinlock* cs_shader_ps   = nullptr; 
SK_Thread_HybridSpinlock* cs_shader_gs   = nullptr; 
SK_Thread_HybridSpinlock* cs_shader_hs   = nullptr; 
SK_Thread_HybridSpinlock* cs_shader_ds   = nullptr; 
SK_Thread_HybridSpinlock* cs_shader_cs   = nullptr; 
SK_Thread_HybridSpinlock* cs_mmio        = nullptr; 
SK_Thread_HybridSpinlock* cs_render_view = nullptr; 

CRITICAL_SECTION tex_cs     = { };
CRITICAL_SECTION hash_cs    = { };
CRITICAL_SECTION dump_cs    = { };
CRITICAL_SECTION cache_cs   = { };
CRITICAL_SECTION inject_cs  = { };
CRITICAL_SECTION preload_cs = { };

LPVOID pfnD3D11CreateDevice             = nullptr;
LPVOID pfnD3D11CreateDeviceAndSwapChain = nullptr;
LPVOID pfnD3D11CoreCreateDevice         = nullptr;

HMODULE SK::DXGI::hModD3D11 = nullptr;

SK::DXGI::PipelineStatsD3D11 SK::DXGI::pipeline_stats_d3d11 = { };

volatile HANDLE hResampleThread = nullptr;

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
  _Out_opt_                           ID3D11DeviceContext **ppImmediateContext);

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
 _Out_opt_                            ID3D11DeviceContext  **ppImmediateContext);


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
SK_HookCacheEntryLocal (D3D11CreateDevice,             L"d3d11.dll", D3D11CreateDevice_Detour,             &D3D11CreateDevice_Import)
SK_HookCacheEntryLocal (D3D11CreateDeviceAndSwapChain, L"d3d11.dll", D3D11CreateDeviceAndSwapChain_Detour, &D3D11CreateDeviceAndSwapChain_Import)

static
std::vector <sk_hook_cache_record_s *> global_d3d11_records =
  { &GlobalHook_D3D11CreateDevice,
    &GlobalHook_D3D11CreateDeviceAndSwapChain };

static
std::vector <sk_hook_cache_record_s *> local_d3d11_records =
  { &LocalHook_D3D11CreateDevice,
    &LocalHook_D3D11CreateDeviceAndSwapChain };



extern "C" __declspec (dllexport) FARPROC D3D11CreateDeviceForD3D12              = nullptr;
extern "C" __declspec (dllexport) FARPROC CreateDirect3D11DeviceFromDXGIDevice   = nullptr;
extern "C" __declspec (dllexport) FARPROC CreateDirect3D11SurfaceFromDXGISurface = nullptr;
extern "C" __declspec (dllexport) FARPROC D3D11On12CreateDevice                  = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTCloseAdapter                     = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTDestroyAllocation                = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTDestroyContext                   = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTDestroyDevice                    = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTDestroySynchronizationObject     = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTQueryAdapterInfo                 = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTSetDisplayPrivateDriverFormat    = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTSignalSynchronizationObject      = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTUnlock                           = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTWaitForSynchronizationObject     = nullptr;
extern "C" __declspec (dllexport) FARPROC EnableFeatureLevelUpgrade              = nullptr;
extern "C" __declspec (dllexport) FARPROC OpenAdapter10                          = nullptr;
extern "C" __declspec (dllexport) FARPROC OpenAdapter10_2                        = nullptr;
extern "C" __declspec (dllexport) FARPROC D3D11CoreCreateLayeredDevice           = nullptr;
extern "C" __declspec (dllexport) FARPROC D3D11CoreGetLayeredDeviceSize          = nullptr;
extern "C" __declspec (dllexport) FARPROC D3D11CoreRegisterLayers                = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTCreateAllocation                 = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTCreateContext                    = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTCreateDevice                     = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTCreateSynchronizationObject      = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTEscape                           = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTGetContextSchedulingPriority     = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTGetDeviceState                   = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTGetDisplayModeList               = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTGetMultisampleMethodList         = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTGetRuntimeData                   = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTGetSharedPrimaryHandle           = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTLock                             = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTOpenAdapterFromHdc               = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTOpenResource                     = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTPresent                          = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTQueryAllocationResidency         = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTQueryResourceInfo                = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTRender                           = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTSetAllocationPriority            = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTSetContextSchedulingPriority     = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTSetDisplayMode                   = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTSetGammaRamp                     = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTSetVidPnSourceOwner              = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DKMTWaitForVerticalBlankEvent        = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DPerformance_BeginEvent              = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DPerformance_EndEvent                = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DPerformance_GetStatus               = nullptr;
extern "C" __declspec (dllexport) FARPROC D3DPerformance_SetMarker               = nullptr;

SK_D3D11_KnownShaders_Singleton&
__SK_Singleton_D3D11_Shaders (void)
{
  static SK_D3D11_KnownShaders _SK_D3D11_Shaders;
  return                       _SK_D3D11_Shaders;
}

volatile LONG __SKX_ComputeAntiStall = 1;


INT
__stdcall
SK_D3D11_BytesPerPixel  (DXGI_FORMAT fmt) ;

constexpr BOOL
SK_D3D11_IsFormatBC6Or7 (DXGI_FORMAT fmt) 
{
  return ( fmt >= DXGI_FORMAT_BC6H_TYPELESS &&
           fmt <= DXGI_FORMAT_BC7_UNORM_SRGB   );
}


extern float __SK_HDR_HorizCoverage;
extern float __SK_HDR_VertCoverage;


BOOL
__stdcall
SK_D3D11_IsFormatCompressed (DXGI_FORMAT fmt) 
{
  switch (fmt)
  {
    case DXGI_FORMAT_BC1_TYPELESS:
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC2_TYPELESS:
    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_TYPELESS:
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_BC4_TYPELESS:
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC4_SNORM:
    case DXGI_FORMAT_BC5_TYPELESS:
    case DXGI_FORMAT_BC5_UNORM:
    case DXGI_FORMAT_BC5_SNORM:
    case DXGI_FORMAT_BC6H_TYPELESS:
    case DXGI_FORMAT_BC6H_UF16:
    case DXGI_FORMAT_BC6H_SF16:
    case DXGI_FORMAT_BC7_TYPELESS:
    case DXGI_FORMAT_BC7_UNORM:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
      return true;

    default:
      return false;
  }
}

static
inline UINT
CalcMipmapLODs (UINT width, UINT height)
{
  UINT lods = 1U;

  while ((width > 1U) || (height > 1U))
  {
    if (width  > 1U) width  >>= 1UL;
    if (height > 1U) height >>= 1UL;

    ++lods;
  }

  return lods;
};


extern bool __SK_HDR_10BitSwap;
extern bool __SK_HDR_16BitSwap;


#include <SpecialK/control_panel/d3d11.h>

bool SK_D3D11_EnableTracking      = false;
bool SK_D3D11_EnableMMIOTracking  = false;
volatile LONG
     SK_D3D11_DrawTrackingReqs    = 0L;
volatile LONG
     SK_D3D11_CBufferTrackingReqs = 0L;

__inline
bool
SK_D3D11_ShouldTrackSetShaderResources ( ID3D11DeviceContext* pDevCtx,
                                         SK_TLS**             ppTLS = nullptr )
{
  UNREFERENCED_PARAMETER (pDevCtx); UNREFERENCED_PARAMETER (ppTLS);

  if (! SK::ControlPanel::D3D11::show_shader_mod_dlg)
    return false;

  return true;
}

__inline
bool
SK_D3D11_ShouldTrackMMIO ( ID3D11DeviceContext* pDevCtx,
                           SK_TLS**             ppTLS = nullptr )
{
  UNREFERENCED_PARAMETER (pDevCtx); UNREFERENCED_PARAMETER (ppTLS);

  if (! SK::ControlPanel::D3D11::show_shader_mod_dlg)
    return false;

  return true;
}


HMODULE hModReShade = (HMODULE)-2;

__inline
bool
SK_D3D11_ShouldTrackRenderOp ( ID3D11DeviceContext* pDevCtx,
                               SK_TLS**             ppTLS = nullptr )
{
  if (! SK_D3D11_EnableTracking)
    return false;

  if ((! config.render.dxgi.deferred_isolation) && pDevCtx->GetType () == D3D11_DEVICE_CONTEXT_DEFERRED)
    return false;

  static const SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  if ( rb.d3d11.immediate_ctx == nullptr ||
       rb.device              == nullptr ||
       rb.swapchain           == nullptr )
  {
    return false;
  }

  SK_TLS *pTLS = nullptr;

  if (ppTLS != nullptr)
  {
    if (*ppTLS != nullptr)
    {
      pTLS = *ppTLS;
    }

    else
    {
        pTLS = SK_TLS_Bottom ();
      *ppTLS = pTLS;
    }
  }

  if ( pTLS != nullptr &&
       pTLS->imgui.drawing )
  {
    return false;
  }

  return true;
}

using SK_ReShade_PresentCallback_pfn              = bool (__stdcall *)(void *user);
using SK_ReShade_OnCopyResourceD3D11_pfn          = void (__stdcall *)(void *user, ID3D11Resource         *&dest,    ID3D11Resource *&source);
using SK_ReShade_OnClearDepthStencilViewD3D11_pfn = void (__stdcall *)(void *user, ID3D11DepthStencilView *&depthstencil);
using SK_ReShade_OnGetDepthStencilViewD3D11_pfn   = void (__stdcall *)(void *user, ID3D11DepthStencilView *&depthstencil);
using SK_ReShade_OnSetDepthStencilViewD3D11_pfn   = void (__stdcall *)(void *user, ID3D11DepthStencilView *&depthstencil);
using SK_ReShade_OnDrawD3D11_pfn                  = void (__stdcall *)(void *user, ID3D11DeviceContext     *context, unsigned int   vertices);

struct SK_RESHADE_CALLBACK_DRAW
{
  SK_ReShade_OnDrawD3D11_pfn fn   = nullptr;
  void*                      data = nullptr;
  __forceinline void call (ID3D11DeviceContext *context, unsigned int vertices, SK_TLS* pTLS = nullptr)
  { if (data != nullptr && fn != nullptr && (pTLS == nullptr || (! pTLS->imgui.drawing))) fn (data, context, vertices); }
} SK_ReShade_DrawCallback;

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

__inline
bool
SK_D3D11_ShouldTrackDrawCall ( ID3D11DeviceContext* pDevCtx,
                         const SK_D3D11DrawType     draw_type,
                               SK_TLS**             ppTLS = nullptr )
{
  // If ReShade (custom version) is loaded, state tracking is non-optional
  if ( hModReShade < (HMODULE)0 )
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
       (! SK_D3D11_Shaders.reshade_triggered) )
  {
    process = true;
  }

  else
  {
    process =
     ( ReadAcquire (&SK_D3D11_DrawTrackingReqs) > 0 )
               ||
       SK_D3D11_ShouldTrackRenderOp (pDevCtx, ppTLS);
  }

  if (process && (ppTLS != nullptr && (! (*ppTLS))))
                 *ppTLS = SK_TLS_Bottom ();

  return
    process;
}

// ID3D11DeviceContext* private data used for indexing various per-ctx lookuos
const GUID SKID_D3D11DeviceContextHandle =
// {5C5298CA-0F9C-5932-A19D-A2E69792AE03}
  { 0x5c5298ca, 0xf9c,  0x5932, { 0xa1, 0x9d, 0xa2, 0xe6, 0x97, 0x92, 0xae, 0x3 } };

volatile LONG SK_D3D11_NumberOfSeenContexts = 0;

enum class sk_shader_class {
  Unknown  = 0x00,
  Vertex   = 0x01,
  Pixel    = 0x02,
  Geometry = 0x04,
  Hull     = 0x08,
  Domain   = 0x10,
  Compute  = 0x20
};

void
SK_D3D11_MergeCommandLists ( ID3D11DeviceContext *pSurrogate,
                             ID3D11DeviceContext *pMerge )
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
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &shaders.vertex
                          );
         break;
      case sk_shader_class::Pixel:
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &shaders.pixel
                          );
         break;
      case sk_shader_class::Geometry:
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &shaders.geometry
                          );
         break;
      case sk_shader_class::Domain:
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &shaders.domain
                          );
         break;
      case sk_shader_class::Hull:
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &shaders.hull
                          );
         break;
      case sk_shader_class::Compute:
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &shaders.compute
                          );
         break;
    }

    return *ppShaderDomain;
  };

  SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>* pShaderRepoIn  = nullptr;
  SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>* pShaderRepoOut = nullptr;

  const UINT dev_ctx_in  =
    SK_D3D11_GetDeviceContextHandle (pSurrogate),
       dev_ctx_out =
    SK_D3D11_GetDeviceContextHandle (pMerge);

  _GetRegistry ( &pShaderRepoIn,  sk_shader_class::Vertex   );
  _GetRegistry ( &pShaderRepoOut, sk_shader_class::Vertex   )->current.shader [dev_ctx_out] = 
                                                pShaderRepoIn->current.shader [dev_ctx_in ];
                                                pShaderRepoIn->current.shader [dev_ctx_in ] = 0x0;
    memcpy     (  pShaderRepoIn->current.views      [dev_ctx_in ],
                  pShaderRepoOut->current.views     [dev_ctx_out],
                    128 * sizeof (ptrdiff_t) );
                  pShaderRepoIn->tracked.deactivate (pSurrogate);
    ZeroMemory (  pShaderRepoIn->current.views      [dev_ctx_in],
                    128 * sizeof (ptrdiff_t) );

  _GetRegistry ( &pShaderRepoIn,  sk_shader_class::Pixel    );
  _GetRegistry ( &pShaderRepoOut, sk_shader_class::Pixel    )->current.shader [dev_ctx_out] = 
                                                pShaderRepoIn->current.shader [dev_ctx_in ];
                                                pShaderRepoIn->current.shader [dev_ctx_in ] = 0x0;
    memcpy     (  pShaderRepoIn->current.views      [dev_ctx_in ],
                  pShaderRepoOut->current.views     [dev_ctx_out],
                    128 * sizeof (ptrdiff_t) );
                  pShaderRepoIn->tracked.deactivate (pSurrogate);
    ZeroMemory (  pShaderRepoIn->current.views      [dev_ctx_in],
                    128 * sizeof (ptrdiff_t) );

  _GetRegistry ( &pShaderRepoIn,  sk_shader_class::Geometry   );
  _GetRegistry ( &pShaderRepoOut, sk_shader_class::Geometry   )->current.shader [dev_ctx_out] = 
                                                  pShaderRepoIn->current.shader [dev_ctx_in ];
                                                  pShaderRepoIn->current.shader [dev_ctx_in ] = 0x0;
    memcpy     (  pShaderRepoIn->current.views      [dev_ctx_in ],
                  pShaderRepoOut->current.views     [dev_ctx_out],
                    128 * sizeof (ptrdiff_t) );
                  pShaderRepoIn->tracked.deactivate (pSurrogate);
    ZeroMemory (  pShaderRepoIn->current.views      [dev_ctx_in],
                    128 * sizeof (ptrdiff_t) );


  _GetRegistry ( &pShaderRepoIn,  sk_shader_class::Hull   );
  _GetRegistry ( &pShaderRepoOut, sk_shader_class::Hull   )->current.shader [dev_ctx_out] = 
                                               pShaderRepoIn->current.shader [dev_ctx_in];
                                               pShaderRepoIn->current.shader [dev_ctx_in] = 0x0;
    memcpy     (  pShaderRepoIn->current.views      [dev_ctx_in ],
                  pShaderRepoOut->current.views     [dev_ctx_out],
                    128 * sizeof (ptrdiff_t) );
                  pShaderRepoIn->tracked.deactivate (pSurrogate);
    ZeroMemory (  pShaderRepoIn->current.views      [dev_ctx_in],
                    128 * sizeof (ptrdiff_t) );


  _GetRegistry ( &pShaderRepoIn,  sk_shader_class::Domain   );
  _GetRegistry ( &pShaderRepoOut, sk_shader_class::Domain   )->current.shader [dev_ctx_out] = 
                                                pShaderRepoIn->current.shader [dev_ctx_in ];
                                                pShaderRepoIn->current.shader [dev_ctx_in ] = 0x0;
    memcpy     (  pShaderRepoIn->current.views      [dev_ctx_in ],
                  pShaderRepoOut->current.views     [dev_ctx_out],
                    128 * sizeof (ptrdiff_t) );
                  pShaderRepoIn->tracked.deactivate (pSurrogate);
    ZeroMemory (  pShaderRepoIn->current.views      [dev_ctx_in],
                    128 * sizeof (ptrdiff_t) );

  _GetRegistry ( &pShaderRepoIn,  sk_shader_class::Compute   );
  _GetRegistry ( &pShaderRepoOut, sk_shader_class::Compute   )->current.shader [dev_ctx_out] = 
                                                 pShaderRepoIn->current.shader [dev_ctx_in ];
                                                 pShaderRepoIn->current.shader [dev_ctx_in ] = 0x0;
    memcpy     (  pShaderRepoIn->current.views      [dev_ctx_in ],
                  pShaderRepoOut->current.views     [dev_ctx_out],
                    128 * sizeof (ptrdiff_t) );
                  pShaderRepoIn->tracked.deactivate (pSurrogate);
    ZeroMemory (  pShaderRepoIn->current.views      [dev_ctx_in],
                    128 * sizeof (ptrdiff_t) );
}

void
SK_D3D11_ResetContextState (ID3D11DeviceContext* pDevCtx)
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
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &shaders.vertex
                          );
         break;
      case sk_shader_class::Pixel:
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &shaders.pixel
                          );
         break;
      case sk_shader_class::Geometry:
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &shaders.geometry
                          );
         break;
      case sk_shader_class::Domain:
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &shaders.domain
                          );
         break;
      case sk_shader_class::Hull:
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &shaders.hull
                          );
         break;
      case sk_shader_class::Compute:
        *ppShaderDomain = reinterpret_cast <SK_D3D11_KnownShaders::ShaderRegistry <IUnknown> *> (
                           &shaders.compute
                          );
         break;
    }

    return
      *ppShaderDomain;
  };

  SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>* pShaderRepo = nullptr;

  const UINT dev_ctx =
    SK_D3D11_GetDeviceContextHandle (pDevCtx);

  _GetRegistry ( &pShaderRepo, sk_shader_class::Vertex   )->current.shader [dev_ctx] = 0x0;
                  pShaderRepo->tracked.deactivate (pDevCtx);
    ZeroMemory (  pShaderRepo->current.views      [dev_ctx], 128 * sizeof (ptrdiff_t) );
  _GetRegistry ( &pShaderRepo, sk_shader_class::Pixel    )->current.shader [dev_ctx] = 0x0;
                  pShaderRepo->tracked.deactivate (pDevCtx);
    ZeroMemory (  pShaderRepo->current.views      [dev_ctx], 128 * sizeof (ptrdiff_t) );
  _GetRegistry ( &pShaderRepo, sk_shader_class::Geometry )->current.shader [dev_ctx] = 0x0;
                  pShaderRepo->tracked.deactivate (pDevCtx);
    ZeroMemory (  pShaderRepo->current.views      [dev_ctx], 128 * sizeof (ptrdiff_t) );
  _GetRegistry ( &pShaderRepo, sk_shader_class::Domain   )->current.shader [dev_ctx] = 0x0;
                  pShaderRepo->tracked.deactivate (pDevCtx);
    ZeroMemory (  pShaderRepo->current.views      [dev_ctx], 128 * sizeof (ptrdiff_t) );
  _GetRegistry ( &pShaderRepo, sk_shader_class::Hull     )->current.shader [dev_ctx] = 0x0;
                  pShaderRepo->tracked.deactivate (pDevCtx);
    ZeroMemory (  pShaderRepo->current.views      [dev_ctx], 128 * sizeof (ptrdiff_t) );
  _GetRegistry ( &pShaderRepo, sk_shader_class::Compute  )->current.shader [dev_ctx] = 0x0;
                  pShaderRepo->tracked.deactivate (pDevCtx);
    ZeroMemory (  pShaderRepo->current.views      [dev_ctx], 128 * sizeof (ptrdiff_t) );
}

#if 0
LONG
SK_D3D11_GetDeviceContextHandle ( ID3D11DeviceContext *pDevCtx )
{
  static std::map
    <ID3D11DeviceContext*, LONG> __contexts0, __contexts1;
  static std::map
    <ID3D11DeviceContext*, LONG>& __ctx_active  = __contexts0,
    __ctx_reserve = __contexts1;

  // Polymorphic weirdness - thank you IUnknown promoting to anything it wants
  ////CComQIPtr <ID3D11DeviceContext> pTestCtx (pDevCtx);

  if (pDevCtx == nullptr)
    return SK_D3D11_MAX_DEV_CONTEXTS;

  LONG handle = 0;
  auto     it =
    __ctx_active.find (pDevCtx);

  if (it != __ctx_active.end ())
    return it->second;

  handle =
    ReadAcquire (&SK_D3D11_NumberOfSeenContexts);

  {
    {
      std::lock_guard <SK_Thread_HybridSpinlock> auto_lock (*cs_shader);
    }

    // Concurrent accesses to the same devctx (totally not safe!) need
    //   to be handled, so once we acquire this lock, check if the list
    //     already contains this dev ctx.
    it =
      __ctx_active.find (pDevCtx);

    if (it != __ctx_active.end ())
      return it->second;

    std::lock_guard <SK_Thread_HybridSpinlock> auto_lock (*cs_shader);

    __ctx_reserve.insert (
      { pDevCtx, handle }
    );

    std::swap (
      __ctx_active,
      __ctx_reserve
    );

    __ctx_reserve.insert (
      { pDevCtx, handle }
    );

    InterlockedIncrement (&SK_D3D11_NumberOfSeenContexts);

    assert (handle < SK_D3D11_MAX_DEV_CONTEXTS);
  }

  return handle;
}
#endif

LONG
SK_D3D11_GetDeviceContextHandle ( ID3D11DeviceContext *pDevCtx )
{
  // Polymorphic weirdness - thank you IUnknown promoting to anything it wants
  ////CComQIPtr <ID3D11DeviceContext> pTestCtx (pDevCtx);

  const int RESOLVE_MAX = 32;

  static std::pair <ID3D11DeviceContext*, LONG>
    last_resolve [RESOLVE_MAX];
  static volatile LONG
         resolve_idx = 0;

  auto early_out =
    last_resolve [ReadAcquire (&resolve_idx)];

  if (early_out.first == pDevCtx)
    return early_out.second;


  auto _CacheResolution =
    [&](LONG idx, ID3D11DeviceContext* pCtx, LONG handle) ->
    void
    {
      early_out =
        std::make_pair (pCtx, handle);

      std::swap (last_resolve [idx], early_out);

      InterlockedExchange (&resolve_idx, idx);
    };


  if (pDevCtx == nullptr) return SK_D3D11_MAX_DEV_CONTEXTS;


  UINT size   = sizeof (LONG);
  LONG handle = 0;

  
  LONG idx =
    ReadAcquire (&resolve_idx) + 1;

  if (idx >= RESOLVE_MAX)
    idx = 0;


  if ( SUCCEEDED ( pDevCtx->GetPrivateData ( SKID_D3D11DeviceContextHandle, &size,
                                             &handle ) ) )
  {
    _CacheResolution (idx, pDevCtx, handle);

    return handle;
  }

  std::lock_guard <SK_Thread_HybridSpinlock> auto_lock (*cs_shader);

  size   = sizeof (LONG);
  handle = ReadAcquire (&SK_D3D11_NumberOfSeenContexts);

  LONG* new_handle =
    new LONG (handle);

  if ( SUCCEEDED (
         pDevCtx->SetPrivateData ( SKID_D3D11DeviceContextHandle, size, new_handle )
       )
     )
  {
    InterlockedIncrement (&SK_D3D11_NumberOfSeenContexts);

    _CacheResolution (idx, pDevCtx, *new_handle);

    return *new_handle;
  }

  else delete new_handle;

  assert (handle < SK_D3D11_MAX_DEV_CONTEXTS);

  _CacheResolution (idx, pDevCtx, handle);

  return handle;
}


size_t
__stdcall
SK_D3D11_ComputeTextureSize (const D3D11_TEXTURE2D_DESC* pDesc)
{
  size_t       size       = 0;
  const bool   compressed =
    SK_D3D11_IsFormatCompressed (pDesc->Format);

  if (! compressed)
  {
    for (UINT i = 0; i < pDesc->MipLevels; i++)
    {
      size += static_cast <size_t> (SK_D3D11_BytesPerPixel (pDesc->Format)) *
              static_cast <size_t> (std::max (1U, pDesc->Width  >> i))      *
              static_cast <size_t> (std::max (1U, pDesc->Height >> i));
    }
  }

  else
  {
    const int bpp = ( (pDesc->Format >= DXGI_FORMAT_BC1_TYPELESS &&
                       pDesc->Format <= DXGI_FORMAT_BC1_UNORM_SRGB) ||
                      (pDesc->Format >= DXGI_FORMAT_BC4_TYPELESS &&
                       pDesc->Format <= DXGI_FORMAT_BC4_SNORM) ) ? 0 : 1;

    // Block-Compressed Formats have minimum 4x4 pixel alignment, so 
    //   computing size is non-trivial.
    for (UINT i = 0; i < pDesc->MipLevels; i++)
    {
      const size_t stride = (bpp == 0) ?
               static_cast <size_t> (std::max (1UL, (std::max (1U, (pDesc->Width >> i)) + 3UL) / 4UL)) * 8UL :
               static_cast <size_t> (std::max (1UL, (std::max (1U, (pDesc->Width >> i)) + 3UL) / 4UL)) * 16UL;

      const size_t lod_size =
        static_cast <size_t> (stride) * (static_cast <size_t> (pDesc->Height >> i) / 4 +
                                         static_cast <size_t> (pDesc->Height >> i) % 4);

      size += lod_size;
    }
  }

  return size;
}


std::wstring
DescribeResource (ID3D11Resource* pRes)
{
  std::wstring desc_out = L"";

  D3D11_RESOURCE_DIMENSION rDim;
  pRes->GetType          (&rDim);

  if (rDim == D3D11_RESOURCE_DIMENSION_BUFFER)
    desc_out += L"Buffer";
  else if (rDim == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
  {
    desc_out += L"Tex2D: ";
    CComQIPtr <ID3D11Texture2D> pTex2D (pRes);

    D3D11_TEXTURE2D_DESC desc = {};
    pTex2D->GetDesc (&desc);

    desc_out += SK_FormatStringW (L"(%lux%lu): %s { %s/%s }",
                  desc.Width, desc.Height, SK_DXGI_FormatToStr (desc.Format).c_str (),
                                        SK_D3D11_DescribeUsage (desc.Usage),
                   SK_D3D11_DescribeBindFlags ((D3D11_BIND_FLAG)desc.BindFlags).c_str ());
                  
  }
  else
    desc_out += L"Other";

  return desc_out;
};


HRESULT
__stdcall
SK_D3D11_MipmapCacheTexture2DEx ( const DirectX::ScratchImage&   img,
                                        uint32_t                 crc32c, 
                                        ID3D11Texture2D*         pOutTex,
                                        DirectX::ScratchImage** ppOutImg,
                                        SK_TLS*                  pTLS = SK_TLS_Bottom () );

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

static       HANDLE hResampleWork       = INVALID_HANDLE_VALUE;
static unsigned int dispatch_thread_idx = 0;

struct resample_dispatch_s
{
  void delayInit (void)
  {
    hResampleWork =
      CreateEventW ( nullptr, FALSE,
                              FALSE,
        SK_FormatStringW ( LR"(Local\SK_DX11TextureResample_Dispatch_pid%x)",
                             GetCurrentProcessId () ).c_str ()
      );

    SYSTEM_INFO        sysinfo = { };
    SK_GetSystemInfo (&sysinfo);

    max_workers = std::max (1UL, sysinfo.dwNumberOfProcessors - 1);

    PROCESSOR_NUMBER                                 pnum = { };
    GetThreadIdealProcessorEx (GetCurrentThread (), &pnum);

    dispatch_thread_idx = pnum.Group + pnum.Number;
  }

  bool postJob (resample_job_s& job)
  {
    job.time.received = SK_QueryPerf ().QuadPart;
    waiting_textures.push (job);

    InterlockedIncrement  (&SK_D3D11_TextureResampler.stats.textures_waiting);

    SK_RunOnce (delayInit ());

    bool ret = false;

    if (InterlockedIncrement (&active_workers) <= max_workers)
    {
      SK_Thread_Create ( [](LPVOID) ->
      DWORD
      {
        SetThreadPriority ( SK_GetCurrentThread (),
                            THREAD_MODE_BACKGROUND_BEGIN );

        static volatile LONG thread_num = 0;

        uintptr_t thread_idx = ReadAcquire (&thread_num);
        uintptr_t logic_idx  = 0;

        // There will be an odd-man-out in any SMT implementation since the
        //   SwapChain thread has an ideal _logical_ CPU core.
        bool         disjoint     = false;


        InterlockedIncrement (&thread_num);


        if (thread_idx == dispatch_thread_idx)
        {
          disjoint = true;
          thread_idx++;
          InterlockedIncrement (&thread_num);
        }


        for ( auto& it : SK_CPU_GetLogicalCorePairs () )
        {
          if ( it & ( (uintptr_t)1 << thread_idx ) ) break;
          else                       ++logic_idx;
        }

        const ULONG_PTR logic_mask =
          SK_CPU_GetLogicalCorePairs ()[logic_idx];


        SK_TLS *pTLS =
          SK_TLS_Bottom ();

        SetThreadIdealProcessor ( SK_GetCurrentThread (),
                                    static_cast <DWORD> (thread_idx) );
        if (! disjoint)
          SetThreadAffinityMask ( SK_GetCurrentThread (), logic_mask );

        std::wstring desc = L"[SK] D3D11 Texture Resampling ";


        auto CountSetBits = [](ULONG_PTR bitMask)  -> DWORD
        {
          const DWORD     LSHIFT      = sizeof (ULONG_PTR) * 8 - 1;
                DWORD     bitSetCount = 0;
                ULONG_PTR bitTest     = static_cast <ULONG_PTR> (1) << LSHIFT;
                DWORD     i;
        
          for (i = 0; i <= LSHIFT; ++i)
          {
            bitSetCount += ((bitMask & bitTest) ? 1 : 0);
            bitTest     /= 2;
          }
        
          return bitSetCount;
        };

  const int logical_siblings = CountSetBits (logic_mask);
        if (logical_siblings > 1 && (! disjoint))
        {
          desc += L"HyperThread < ";
          for ( int i = 0,
                    j = 0;
                    i < SK_GetBitness ();
                  ++i )
          {
            if ( (logic_mask & ( static_cast <uintptr_t> (1) << i ) ) )
            {
              desc += std::to_wstring (i);

              if ( ++j < logical_siblings ) desc += L",";
            }
          }
          desc += L" >";
        }

        else
          desc += SK_FormatStringW (L"Thread %lu", thread_idx);


        SetCurrentThreadDescription (desc.c_str ());

        pTLS->texture_management.injection_thread = true;
        pTLS->imgui.drawing                       = true;

        SetThreadPriorityBoost ( SK_GetCurrentThread (),
                                 TRUE );
        SetThreadPriority (      SK_GetCurrentThread (),
                                 THREAD_PRIORITY_TIME_CRITICAL );

        do
        {
          resample_job_s job;

          while (SK_D3D11_TextureResampler.waiting_textures.try_pop (job))
          {
            job.time.started = SK_QueryPerf ().QuadPart;

            InterlockedDecrement (&SK_D3D11_TextureResampler.stats.textures_waiting);
            InterlockedIncrement (&SK_D3D11_TextureResampler.stats.textures_resampling);

            DirectX::ScratchImage* pNewImg = nullptr;

            const HRESULT hr =
              SK_D3D11_MipmapCacheTexture2DEx ( *job.data,
                                                 job.crc32c,
                                                 job.texture,
                                                &pNewImg,
                                                 pTLS );

            delete job.data;

            if (FAILED (hr))
            {
              dll_log.Log (L"Resampler failure (crc32c=%x)", job.crc32c);
              InterlockedIncrement (&SK_D3D11_TextureResampler.stats.error_count);
            }

            else
            {
              job.time.finished = SK_QueryPerf ().QuadPart;
              job.data          = pNewImg;

              SK_D3D11_TextureResampler.finished_textures.push (job);
            }

            InterlockedDecrement (&SK_D3D11_TextureResampler.stats.textures_resampling);
          }

          if (WaitForSingleObject (hResampleWork, 666UL) != WAIT_OBJECT_0)
          {
            SK_Thread_ScopedPriority
              hold_your_horses    (THREAD_PRIORITY_IDLE);
            WaitForSingleObjectEx (hResampleWork, INFINITE, FALSE);
          }
        } while (! ReadAcquire (&__SK_DLL_Ending));

        InterlockedDecrement (&SK_D3D11_TextureResampler.active_workers);

        SK_Thread_CloseSelf ();

        return 0;
      } );

      ret = true;
    }

    SetEvent (hResampleWork);

    return ret;
  };


  bool processFinished ( ID3D11Device        *pDev,
                         ID3D11DeviceContext *pDevCtx,
                         SK_TLS              *pTLS = SK_TLS_Bottom () )
  {
    size_t MAX_TEXTURE_UPLOADS_PER_FRAME;
    int    MAX_UPLOAD_TIME_PER_FRAME_IN_MS;

    extern int SK_D3D11_TexStreamPriority;
    switch    (SK_D3D11_TexStreamPriority)
    {
      case 0:
        MAX_UPLOAD_TIME_PER_FRAME_IN_MS = 3UL;
        MAX_TEXTURE_UPLOADS_PER_FRAME   =
          static_cast <size_t> (ReadAcquire (&SK_D3D11_TextureResampler.stats.textures_waiting) / 7) + 1;
        break;

      case 1:
      default:
        MAX_UPLOAD_TIME_PER_FRAME_IN_MS = 13UL;
        MAX_TEXTURE_UPLOADS_PER_FRAME   =
          static_cast <size_t> (ReadAcquire (&SK_D3D11_TextureResampler.stats.textures_waiting) / 4) + 1;
        break;

      case 2:
        MAX_UPLOAD_TIME_PER_FRAME_IN_MS = 27UL;
        MAX_TEXTURE_UPLOADS_PER_FRAME   =
          static_cast <size_t> (ReadAcquire (&SK_D3D11_TextureResampler.stats.textures_waiting) / 2) + 1;
    }


    static const uint64_t _TicksPerMsec =
      ( SK_GetPerfFreq ().QuadPart / 1000ULL );


          size_t   uploaded   = 0;
    const uint64_t start_tick = SK::ControlPanel::current_tick;//SK_QueryPerf ().QuadPart;
    const uint64_t deadline   = start_tick + MAX_UPLOAD_TIME_PER_FRAME_IN_MS * _TicksPerMsec;

    SK_ScopedBool auto_bool_tex (&pTLS->texture_management.injection_thread);
    SK_ScopedBool auto_bool_mem (&pTLS->imgui.drawing);

    pTLS->texture_management.injection_thread = true;
    pTLS->imgui.drawing                       = true;

    bool processed = false;

    static auto& textures =
      SK_D3D11_Textures;

    //
    // Finish Resampled Texture Uploads (discard if texture is no longer live)
    //
    while (! finished_textures.empty ())
    {
      resample_job_s finished = { };

      if ( pDev    != nullptr &&
           pDevCtx != nullptr    )
      {
        if (finished_textures.try_pop (finished))
        {
          // Due to cache purging behavior and the fact that some crazy games issue back-to-back loads of the same resource,
          //   we need to test the cache health prior to trying to service this request.
          if ( ( ( finished.time.started > textures.LastPurge_2D ) ||
                 ( SK_D3D11_TextureIsCached (finished.texture) ) ) &&
                finished.data         != nullptr                   &&
                finished.texture      != nullptr )
          {
            CComPtr <ID3D11Texture2D> pTempTex = nullptr;

            const HRESULT ret =
              DirectX::CreateTexture (pDev, finished.data->GetImages   (), finished.data->GetImageCount (),
                                            finished.data->GetMetadata (), (ID3D11Resource **)&pTempTex);

            if (SUCCEEDED (ret))
            {
              pDevCtx->CopyResource (finished.texture, pTempTex);

              uploaded++;

              InterlockedIncrement (&stats.textures_finished);

              // Various wait-time statistics;  the job queue is HyperThreading aware and helps reduce contention on
              //                                  the list of finished textures ( Which we need to service from the
              //                                                                   game's original calling thread ).
              const uint64_t wait_in_queue = ( finished.time.started      - finished.time.received );
              const uint64_t work_time     = ( finished.time.finished     - finished.time.started  );
              const uint64_t wait_finished = ( SK_CurrentPerf ().QuadPart - finished.time.finished );

              SK_LOG1 ( (L"ReSample Job %4lu (hash=%08x {%4lux%#4lu}) => %9.3f ms TOTAL :: ( %9.3f ms pre-proc, "
                                                                                           L"%9.3f ms work queue, "
                                                                                           L"%9.3f ms resampling, "
                                                                                           L"%9.3f ms finish queue )",
                           ReadAcquire (&stats.textures_finished), finished.crc32c,
                           finished.data->GetMetadata ().width,    finished.data->GetMetadata ().height,
                         ( (long double)SK_CurrentPerf ().QuadPart - (long double)finished.time.received +
                           (long double)finished.time.preprocess ) / (long double)_TicksPerMsec,
                           (long double)finished.time.preprocess   / (long double)_TicksPerMsec,
                           (long double)wait_in_queue              / (long double)_TicksPerMsec,
                           (long double)work_time                  / (long double)_TicksPerMsec,
                           (long double)wait_finished              / (long double)_TicksPerMsec ),
                       L"DX11TexMgr"  );
            }

            else
              InterlockedIncrement (&stats.error_count);
          }

          else
          {
            SK_LOG0 ( (L"Texture (%x) was loaded too late, discarding...", finished.crc32c),
                       L"DX11TexMgr" );

            InterlockedIncrement (&stats.textures_too_late);
          }
          

          if (finished.data != nullptr)
          {
            //delete finished.data;
                   finished.data = nullptr;
          }

          processed = true;


          if ( uploaded >= MAX_TEXTURE_UPLOADS_PER_FRAME ||
               deadline <  (uint64_t)SK_QueryPerf ().QuadPart )  break;
        }

        else
          break;
      }

      else
        break;
    }

    return processed;
  };


  struct stats_s {
    ~stats_s (void) /// 
    {
      if (hResampleWork != INVALID_HANDLE_VALUE)
      {
        CloseHandle (hResampleWork);
                     hResampleWork = INVALID_HANDLE_VALUE;
      }
    }

    volatile LONG textures_resampled    = 0L;
    volatile LONG textures_compressed   = 0L;
    volatile LONG textures_decompressed = 0L;

    volatile LONG textures_waiting      = 0L;
    volatile LONG textures_resampling   = 0L;
    volatile LONG textures_too_late     = 0L;
    volatile LONG textures_finished     = 0L;

    volatile LONG error_count           = 0L;
  } stats;

           LONG max_workers    = 0;
  volatile LONG active_workers = 0;

  concurrency::concurrent_queue <resample_job_s> waiting_textures;
  concurrency::concurrent_queue <resample_job_s> finished_textures;
} SK_D3D11_TextureResampler;


LONG
SK_D3D11_Resampler_GetActiveJobCount (void)
{
  return ReadAcquire (&SK_D3D11_TextureResampler.stats.textures_resampling);
}

LONG
SK_D3D11_Resampler_GetWaitingJobCount (void)
{
  return ReadAcquire (&SK_D3D11_TextureResampler.stats.textures_waiting);
}

LONG
SK_D3D11_Resampler_GetRetiredCount (void)
{
  return ReadAcquire (&SK_D3D11_TextureResampler.stats.textures_resampled);
}

LONG
SK_D3D11_Resampler_GetErrorCount (void)
{
  return ReadAcquire (&SK_D3D11_TextureResampler.stats.error_count);
}

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

  if (SK_TLS_Bottom ()->d3d11.ctx_init_thread)
    return;

  if (! ReadAcquire (&__d3d11_ready))
    SK_Thread_SpinUntilFlagged (&__d3d11_ready);
}


template <class _T>
static
__forceinline
UINT
calc_count (_T** arr, UINT max_count)
{
  for ( int i = (int)max_count - 1 ;
            i >= 0 ;
          --i )
  {
    if (arr [i] != 0)
      return i;
  }

  return max_count;
}

__declspec (noinline)
uint32_t
__cdecl
crc32_tex (  _In_      const D3D11_TEXTURE2D_DESC   *__restrict pDesc,
             _In_      const D3D11_SUBRESOURCE_DATA *__restrict pInitialData,
             _Out_opt_       size_t                 *__restrict pSize,
             _Out_opt_       uint32_t               *__restrict pLOD0_CRC32,
             _In_opt_        bool                               bAllLODs = true );

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

struct
{
  SK_ReShade_OnSetDepthStencilViewD3D11_pfn fn   = nullptr;
  void*                                     data = nullptr;

  __forceinline void try_call (ID3D11DepthStencilView *&depthstencil)
  {
    if ((uintptr_t)data + (uintptr_t)fn != 0)
      return call (depthstencil);
  }

  __forceinline void call (ID3D11DepthStencilView *&depthstencil, SK_TLS* pTLS = nullptr)
    { if (data != nullptr && fn != nullptr && (pTLS == nullptr || (! pTLS->imgui.drawing))) fn (data, depthstencil); }
} SK_ReShade_SetDepthStencilViewCallback;

struct
{
  SK_ReShade_OnGetDepthStencilViewD3D11_pfn fn   = nullptr;
  void*                                     data = nullptr;
  
  __forceinline void try_call (ID3D11DepthStencilView *&depthstencil)
  {
    if ((uintptr_t)data + (uintptr_t)fn != 0)
      return call (depthstencil);
  }

  __forceinline void call (ID3D11DepthStencilView *&depthstencil, SK_TLS* pTLS = nullptr)
    { if (data != nullptr && fn != nullptr && (pTLS == nullptr || (! pTLS->imgui.drawing))) fn (data, depthstencil); }
} SK_ReShade_GetDepthStencilViewCallback;

struct
{
  SK_ReShade_OnClearDepthStencilViewD3D11_pfn fn   = nullptr;
  void*                                       data = nullptr;

  __forceinline void try_call (ID3D11DepthStencilView *&depthstencil)
  {
    if ((uintptr_t)data + (uintptr_t)fn != 0)
      return call (depthstencil);
  }

  __forceinline void call (ID3D11DepthStencilView *&depthstencil, SK_TLS* pTLS = nullptr)
    { if (data != nullptr && fn != nullptr && (pTLS == nullptr || (! pTLS->imgui.drawing))) fn (data, depthstencil); }
} SK_ReShade_ClearDepthStencilViewCallback;

struct
{
  SK_ReShade_OnCopyResourceD3D11_pfn fn   = nullptr;
  void*                              data = nullptr;
  __forceinline void call (ID3D11Resource *&dest, ID3D11Resource *&source, SK_TLS* pTLS = nullptr)
    { if (data != nullptr && fn != nullptr && (pTLS == nullptr || (! pTLS->imgui.drawing))) fn (data, dest, source); }
} SK_ReShade_CopyResourceCallback;



#ifndef _SK_WITHOUT_D3DX11
D3DX11CreateTextureFromMemory_pfn D3DX11CreateTextureFromMemory = nullptr;
D3DX11CreateTextureFromFileW_pfn  D3DX11CreateTextureFromFileW  = nullptr;
D3DX11GetImageInfoFromFileW_pfn   D3DX11GetImageInfoFromFileW   = nullptr;
HMODULE                           hModD3DX11_43                 = nullptr;
#endif

bool SK_D3D11_IsTexInjectThread (SK_TLS *pTLS = SK_TLS_Bottom ())
{
  return pTLS->texture_management.injection_thread;
}

void
SK_D3D11_ClearTexInjectThread (SK_TLS *pTLS = SK_TLS_Bottom ())
{
  pTLS->texture_management.injection_thread = false;
}

void
SK_D3D11_SetTexInjectThread (SK_TLS* pTLS = SK_TLS_Bottom ())
{
  pTLS->texture_management.injection_thread = true;
}

uint32_t
__stdcall
SK_D3D11_TextureHashFromCache (ID3D11Texture2D* pTex);

using IUnknown_Release_pfn =
  ULONG (WINAPI *)(IUnknown* This);
using IUnknown_AddRef_pfn  =
  ULONG (WINAPI *)(IUnknown* This);

IUnknown_Release_pfn IUnknown_Release_Original = nullptr;
IUnknown_AddRef_pfn  IUnknown_AddRef_Original  = nullptr;

static volatile LONG __SK_D3D11_TexRefCount_Failures = 0;
//
// If reference counting is broken by some weird COM wrapper
//   misbehaving, then for the love of ... don't cache textures
//     using SK *==> it hooks rather than wraps.
//
BOOL
SK_D3D11_TestRefCountHooks (ID3D11Texture2D* pInputTex, SK_TLS* pTLS = nullptr)
{
  if (pInputTex == nullptr)
    return FALSE;

  // This is unsafe, but ... the variable name already told you that!
  if (config.textures.cache.allow_unsafe_refs)
    return TRUE;

  if (pTLS == nullptr) pTLS = SK_TLS_Bottom ();

  auto SanityFail =
  [&](void)  ->
  BOOL
  {
    InterlockedIncrement (&__SK_D3D11_TexRefCount_Failures);
    return FALSE;
  };

  pTLS->texture_management.refcount_obj = pInputTex;

  LONG initial = pTLS->texture_management.refcount_test;

  pInputTex->AddRef ();

  const LONG initial_plus_one =
    pTLS->texture_management.refcount_test;

  pInputTex->Release ();

  const LONG initial_again =
    pTLS->texture_management.refcount_test;

  pTLS->texture_management.refcount_obj = nullptr;

  if ( initial != initial_plus_one - 1 ||
       initial != initial_again )	
  {
    SK_LOG1 ( (L"Expected %li after AddRef (); got %li.",
                 initial + 1, initial_plus_one ),
               L"DX11TexMgr" );
    SK_LOG1 ( (L"Expected %li after Release (); got %li.",
                 initial, initial_again ),
               L"DX11TexMgr" );

    return SanityFail ();
  }


  // Important note: The D3D11 runtime implements QueryInterface on an
  //                   ID3D11Texture2D by returning a SEPARATE object with
  //                     an initial reference count of 1.
  //
  //     Effectively, QueryInterface creates a view with its own lifetime,
  //       never expect to get the original object by making this call!
  //

            pTLS->texture_management.refcount_test = 0;

  // Also validate that the wrapper's QueryInterface method is
  //   invoking our hooks
  //
  ID3D11Texture2D* pReferenced = nullptr;
  pInputTex->QueryInterface <ID3D11Texture2D> (&pReferenced);

  if (! pReferenced)
    return SanityFail ();

  pTLS->texture_management.refcount_obj = pReferenced;

  initial =
    pTLS->texture_management.refcount_test;

  pReferenced->Release ();

  const LONG initial_after_release =
    pTLS->texture_management.refcount_test;
    pTLS->texture_management.refcount_obj = nullptr;

  if ( initial != initial_after_release + 1 )
  {
    SK_LOG1 ( (L"Expected %lu after QueryInterface (...); got %lu.",
                 initial + 1, initial_plus_one ),
               L"DX11TexMgr" );
    SK_LOG1 ( (L"Expected %lu after Release (); got %lu.",
                 initial, initial_again ),
               L"DX11TexMgr" );

    return SanityFail ();
  }


  return TRUE;
}


__declspec (noinline)
ULONG
WINAPI
IUnknown_Release (IUnknown* This)
{
  SK_TLS* pTLS =
    SK_TLS_Bottom ();

  if (This == pTLS->texture_management.refcount_obj)
    pTLS->texture_management.refcount_test--;

  if (! SK_D3D11_IsTexInjectThread ())
  {
    ID3D11Texture2D* pTex = (ID3D11Texture2D *)This;

    LONG count =
      IUnknown_AddRef_Original  (pTex);
      IUnknown_Release_Original (pTex);


    static bool __yk0 = 
      ( SK_GetCurrentGameID () == SK_GAME_ID::Yakuza0 );

    if (__yk0)
    {
      if (count <= 1)
      {
        if (count > 0)
        {
          if (SK_D3D11_RemoveTexFromCache (pTex))
          {
            IUnknown_Release_Original (pTex);
          }
        }

        return 0;
      }
    }


    // If count is == 0, something's screwy
    if (pTex != nullptr && count <= 2)
    {
      if (SK_D3D11_RemoveTexFromCache (pTex))
      {
        if (count < 2)
          SK_LOG0 ( ( L"Unexpected reference count: %lu", count ), L"DX11TexMgr" );

        IUnknown_Release_Original (pTex);
      }
    }
  }

  return
    IUnknown_Release_Original (This);
}

__declspec (noinline)
ULONG
WINAPI
IUnknown_AddRef (IUnknown* This)
{
  SK_TLS* pTLS =
    SK_TLS_Bottom ();

  if (This == pTLS->texture_management.refcount_obj)
    pTLS->texture_management.refcount_test++;

  if (! SK_D3D11_IsTexInjectThread ())
  {
    // Flag this thread so we don't infinitely recurse when querying the interface
    SK_ScopedBool auto_bool (&pTLS->texture_management.injection_thread);
    pTLS->texture_management.injection_thread = true;

    ID3D11Texture2D* pTex = nullptr;

    if (SUCCEEDED (This->QueryInterface <ID3D11Texture2D> (&pTex)))
    {
      pTLS->texture_management.injection_thread = false;

      if (SK_D3D11_TextureIsCached (pTex))
        SK_D3D11_UseTexture (pTex);

      IUnknown_Release_Original (This);
    }
  }

  return IUnknown_AddRef_Original (This);
}


const wchar_t*
SK_D3D11_DescribeUsage (D3D11_USAGE usage) 
{
  switch (usage)
  {
    case D3D11_USAGE_DEFAULT:
      return L"Default";
    case D3D11_USAGE_DYNAMIC:
      return L"Dynamic";
    case D3D11_USAGE_IMMUTABLE:
      return L"Immutable";
    case D3D11_USAGE_STAGING:
      return L"Staging";
    default:
      return L"UNKNOWN";
  }
}


const wchar_t*
SK_D3D11_DescribeFilter (D3D11_FILTER filter) 
{
  switch (filter)
  {
    case D3D11_FILTER_MIN_MAG_MIP_POINT:                            return L"D3D11_FILTER_MIN_MAG_MIP_POINT";
    case D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR:                     return L"D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR";
    case D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT:               return L"D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT";
    case D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR:                     return L"D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR";
    case D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT:                     return L"D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT";
    case D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR:              return L"D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR";
    case D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT:                     return L"D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT";
    case D3D11_FILTER_MIN_MAG_MIP_LINEAR:                           return L"D3D11_FILTER_MIN_MAG_MIP_LINEAR";

    case D3D11_FILTER_ANISOTROPIC:                                  return L"D3D11_FILTER_ANISOTROPIC";

    case D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT:                 return L"D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT";
    case D3D11_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR:          return L"D3D11_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR";
    case D3D11_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT:    return L"D3D11_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT";
    case D3D11_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR:          return L"D3D11_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR";
    case D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT:          return L"D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT";
    case D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR:   return L"D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR";
    case D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT:          return L"D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT";
    case D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR:                return L"D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR";

    case D3D11_FILTER_COMPARISON_ANISOTROPIC:                       return L"D3D11_FILTER_COMPARISON_ANISOTROPIC";

    case D3D11_FILTER_MINIMUM_MIN_MAG_MIP_POINT:                    return L"D3D11_FILTER_MINIMUM_MIN_MAG_MIP_POINT";
    case D3D11_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR:             return L"D3D11_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR";
    case D3D11_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:       return L"D3D11_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT";
    case D3D11_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR:             return L"D3D11_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR";
    case D3D11_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT:             return L"D3D11_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT";
    case D3D11_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR:      return L"D3D11_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR";
    case D3D11_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT:             return L"D3D11_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT";
    case D3D11_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR:                   return L"D3D11_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR";

    case D3D11_FILTER_MINIMUM_ANISOTROPIC:                          return L"D3D11_FILTER_MINIMUM_ANISOTROPIC";

    case D3D11_FILTER_MAXIMUM_MIN_MAG_MIP_POINT:                    return L"D3D11_FILTER_MAXIMUM_MIN_MAG_MIP_POINT";
    case D3D11_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR:             return L"D3D11_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR";
    case D3D11_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:       return L"D3D11_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT";
    case D3D11_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR:             return L"D3D11_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR";
    case D3D11_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT:             return L"D3D11_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT";
    case D3D11_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR:      return L"D3D11_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR";
    case D3D11_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT:             return L"D3D11_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT";
    case D3D11_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR:                   return L"D3D11_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR";

    case D3D11_FILTER_MAXIMUM_ANISOTROPIC:                          return L"D3D11_FILTER_MAXIMUM_ANISOTROPIC";

    default: return L"Unknown";
  };
}

std::wstring
SK_D3D11_DescribeMiscFlags (D3D11_RESOURCE_MISC_FLAG flags)
{
  std::wstring                 out = L"";
  std::stack <const wchar_t *> flag_text;

  if (flags & D3D11_RESOURCE_MISC_GENERATE_MIPS)
    flag_text.push (L"Generates Mipmaps");

  if (flags & D3D11_RESOURCE_MISC_SHARED)
    flag_text.push (L"Shared Resource");

  if (flags & D3D11_RESOURCE_MISC_TEXTURECUBE)
    flag_text.push (L"Cubemap");

  if (flags & D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS)
    flag_text.push (L"Draw Indirect Arguments");

  if (flags & D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS)
    flag_text.push (L"Allows Raw Buffer Views");

  if (flags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED)
    flag_text.push (L"Structured Buffer");

  if (flags & D3D11_RESOURCE_MISC_RESOURCE_CLAMP)
    flag_text.push (L"Resource Clamp");

  if (flags & D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX)
    flag_text.push (L"Shared Key Mutex");

  if (flags & D3D11_RESOURCE_MISC_GDI_COMPATIBLE)
    flag_text.push (L"GDI Compatible");

  if (flags & D3D11_RESOURCE_MISC_SHARED_NTHANDLE)
    flag_text.push (L"Shared Through NT Handles");

  if (flags & D3D11_RESOURCE_MISC_RESTRICTED_CONTENT)
    flag_text.push (L"Content is Encrypted");

  if (flags & D3D11_RESOURCE_MISC_RESTRICT_SHARED_RESOURCE)
    flag_text.push (L"Shared Encrypted Content");

  if (flags & D3D11_RESOURCE_MISC_RESTRICT_SHARED_RESOURCE_DRIVER)
    flag_text.push (L"Driver-level Encrypted Resource Sharing");

  if (flags & D3D11_RESOURCE_MISC_GUARDED)
    flag_text.push (L"Guarded");

  if (flags & D3D11_RESOURCE_MISC_TILE_POOL)
    flag_text.push (L"Stored in Tiled Resource Memory Pool");

  if (flags & D3D11_RESOURCE_MISC_TILED)
    flag_text.push (L"Tiled Resource");

  while (! flag_text.empty ())
  {
    out += flag_text.top ();
           flag_text.pop ();

    if (! flag_text.empty ())
      out += L", ";
  }

  return out;
}

std::wstring
SK_D3D11_DescribeBindFlags (D3D11_BIND_FLAG flags)
{
  std::wstring                 out = L"";
  std::stack <const wchar_t *> flag_text;

  if (flags & D3D11_BIND_CONSTANT_BUFFER)
    flag_text.push (L"Constant Buffer");

  if (flags & D3D11_BIND_DECODER)
    flag_text.push (L"Video Decoder");

  if (flags & D3D11_BIND_DEPTH_STENCIL)
    flag_text.push (L"Depth/Stencil");

  if (flags & D3D11_BIND_INDEX_BUFFER)
    flag_text.push (L"Index Buffer");

  if (flags & D3D11_BIND_RENDER_TARGET)
    flag_text.push (L"Render Target");

  if (flags & D3D11_BIND_SHADER_RESOURCE)
    flag_text.push (L"Shader Resource");

  if (flags & D3D11_BIND_STREAM_OUTPUT)
    flag_text.push (L"Stream Output");

  if (flags & D3D11_BIND_UNORDERED_ACCESS)
    flag_text.push (L"Unordered Access");

  if (flags & D3D11_BIND_VERTEX_BUFFER)
    flag_text.push (L"Vertex Buffer");

  if (flags & D3D11_BIND_VIDEO_ENCODER)
    flag_text.push (L"Video Encoder");

  while (! flag_text.empty ())
  {
    out += flag_text.top ();
           flag_text.pop ();

    if (! flag_text.empty ())
      out += L", ";
  }

  return out;
}


void
WINAPI
SK_D3D11_AddInjectable (uint32_t top_crc32, uint32_t checksum);

void
__stdcall
SK_D3D11_RemoveInjectable (uint32_t top_crc32, uint32_t checksum);

void
WINAPI
SK_D3D11_AddTexHash ( const wchar_t* name, uint32_t top_crc32, uint32_t hash );



// NEVER, under any circumstances, call any functions using this!
ID3D11Device* g_pD3D11Dev = nullptr;

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
      SK_LOG0 ( (L" >> Device = %08" PRIxPTR L"h (Feature Level:%s)",
                      (uintptr_t)*ppDevice,
                        SK_DXGI_FeatureLevelsToStr ( 1,
                                                      (DWORD *)&FeatureLevel
                                                   ).c_str ()
                  ), __SK_SUBSYSTEM__ );

      // We ARE technically holding a reference, but we never make calls to this
      //   interface - it's just for tracking purposes.
      g_pD3D11Dev = *ppDevice;

      rb.api =
        SK_RenderAPI::D3D11;
    }

    if (config.render.dxgi.exception_mode != -1)
      (*ppDevice)->SetExceptionMode (config.render.dxgi.exception_mode);

    CComQIPtr <IDXGIDevice>  pDXGIDev (*ppDevice);
    CComPtr   <IDXGIAdapter> pAdapter = nullptr;

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
          if (SK_GetFramesDrawn () > 0)
            SK::DXGI::StartBudgetThread ( &pAdapter.p );
        }
      }
    }
  }
}

UINT
SK_D3D11_RemoveUndesirableFlags (UINT& Flags) 
{
  const UINT original = Flags;

  // The Steam overlay behaves strangely when this is present
  Flags &= ~D3D11_CREATE_DEVICE_PREVENT_ALTERING_LAYER_SETTINGS_FROM_REGISTRY;

  return original;
}

#include <SpecialK/import.h>

volatile LONG __dxgi_plugins_loaded = FALSE;

//   0 QueryInterface
//   1 AddRef
//   2 Release

//   3 GetDevice
//   4 GetPrivateData
//   5 SetPrivateData
//   6 SetPrivateDataInterface

//   7 VSSetConstantBuffers 
//   8 PSSetShaderResources 
//   9 PSSetShader 
//  10 PSSetSamplers 
//  11 VSSetShader 
//  12 DrawIndexed 
//  13 Draw 
//  14 Map 
//  15 Unmap 
//  16 PSSetConstantBuffers 
//  17 IASetInputLayout 
//  18 IASetVertexBuffers 
//  19 IASetIndexBuffer 
//  20 DrawIndexedInstanced 
//  21 DrawInstanced 
//  22 GSSetConstantBuffers 
//  23 GSSetShader 
//  24 IASetPrimitiveTopology 
//  25 VSSetShaderResources 
//  26 VSSetSamplers 
//  27 Begin
//  28 End
//  29 GetData 
//  30 SetPredication 
//  31 GSSetShaderResources 
//  32 GSSetSamplers 
//  33 OMSetRenderTargets 
//  34 OMSetRenderTargetsAndUnorderedAccessViews 
//  35 OMSetBlendState 
//  36 OMSetDepthStencilState 
//  37 SOSetTargets 
//  38 DrawAuto
//  39 DrawIndexedInstancedIndirect 
//  40 DrawInstancedIndirect 
//  41 Dispatch 
//  42 DispatchIndirect 
//  43 RSSetState 
//  44 RSSetViewports 
//  45 RSSetScissorRects 
//  46 CopySubresourceRegion 
//  47 CopyResource 
//  48 UpdateSubresource 
//  49 CopyStructureCount 
//  50 ClearRenderTargetView 
//  51 ClearUnorderedAccessViewUint 
//  52 ClearUnorderedAccessViewFloat 
//  53 ClearDepthStencilView 
//  54 GenerateMips 
//  55 SetResourceMinLOD 
//  56 GetResourceMinLOD 
//  57 ResolveSubresource 
//  58 ExecuteCommandList 
//  59 HSSetShaderResources 
//  60 HSSetShader 
//  61 HSSetSamplers 
//  62 HSSetConstantBuffers 
//  63 DSSetShaderResources 
//  64 DSSetShader 
//  65 DSSetSamplers 
//  66 DSSetConstantBuffers 
//  67 CSSetShaderResources 
//  68 CSSetUnorderedAccessViews 
//  69 CSSetShader 
//  70 CSSetSamplers 
//  71 CSSetConstantBuffers 
//  72 VSGetConstantBuffers 
//  73 PSGetShaderResources 
//  74 PSGetShader 
//  75 PSGetSamplers 
//  76 VSGetShader 
//  77 PSGetConstantBuffers 
//  78 IAGetInputLayout 
//  79 IAGetVertexBuffers 
//  80 IAGetIndexBuffer 
//  81 GSGetConstantBuffers 
//  82 GSGetShader 
//  83 IAGetPrimitiveTopology 
//  84 VSGetShaderResources
//  85 VSGetSamplers 
//  86 GetPredication
//  87 GSGetShaderResources
//  88 GSGetSamplers
//  89 OMGetRenderTargets 
//  90 OMGetRenderTargetsAndUnorderedAccessViews 
//  91 OMGetBlendState 
//  92 OMGetDepthStencilState 
//  93 SOGetTargets 
//  94 RSGetState 
//  95 RSGetViewports 
//  96 RSGetScissorRects 
//  97 HSGetShaderResources 
//  98 HSGetShader 
//  99 HSGetSamplers 
// 100 HSGetConstantBuffers 
// 101 DSGetShaderResources 
// 102 DSGetShader 
// 103 DSGetSamplers 
// 104 DSGetConstantBuffers 
// 105 CSGetShaderResources 
// 106 CSGetUnorderedAccessViews 
// 107 CSGetShader 
// 108 CSGetSamplers 
// 109 CSGetConstantBuffers        
// 110 ClearState
// 111 Flush
// 112 GetType
// 113 GetContextFlags
// 114 FinishCommandList







__declspec (noinline)
HRESULT
WINAPI
D3D11Dev_CreateTexture2D_Override (
_In_            ID3D11Device           *This,
_In_      const D3D11_TEXTURE2D_DESC   *pDesc,
_In_opt_  const D3D11_SUBRESOURCE_DATA *pInitialData,
_Out_opt_       ID3D11Texture2D        **ppTexture2D );

D3D11Dev_CreateSamplerState_pfn                     D3D11Dev_CreateSamplerState_Original                     = nullptr;
D3D11Dev_CreateBuffer_pfn                           D3D11Dev_CreateBuffer_Original                           = nullptr;
D3D11Dev_CreateTexture2D_pfn                        D3D11Dev_CreateTexture2D_Original                        = nullptr;
D3D11Dev_CreateRenderTargetView_pfn                 D3D11Dev_CreateRenderTargetView_Original                 = nullptr;
D3D11Dev_CreateShaderResourceView_pfn               D3D11Dev_CreateShaderResourceView_Original               = nullptr;
D3D11Dev_CreateDepthStencilView_pfn                 D3D11Dev_CreateDepthStencilView_Original                 = nullptr;
D3D11Dev_CreateUnorderedAccessView_pfn              D3D11Dev_CreateUnorderedAccessView_Original              = nullptr;

NvAPI_D3D11_CreateVertexShaderEx_pfn                NvAPI_D3D11_CreateVertexShaderEx_Original                = nullptr;
NvAPI_D3D11_CreateGeometryShaderEx_2_pfn            NvAPI_D3D11_CreateGeometryShaderEx_2_Original            = nullptr;
NvAPI_D3D11_CreateFastGeometryShader_pfn            NvAPI_D3D11_CreateFastGeometryShader_Original            = nullptr;
NvAPI_D3D11_CreateFastGeometryShaderExplicit_pfn    NvAPI_D3D11_CreateFastGeometryShaderExplicit_Original    = nullptr;
NvAPI_D3D11_CreateHullShaderEx_pfn                  NvAPI_D3D11_CreateHullShaderEx_Original                  = nullptr;
NvAPI_D3D11_CreateDomainShaderEx_pfn                NvAPI_D3D11_CreateDomainShaderEx_Original                = nullptr;

D3D11Dev_CreateVertexShader_pfn                     D3D11Dev_CreateVertexShader_Original                     = nullptr;
D3D11Dev_CreatePixelShader_pfn                      D3D11Dev_CreatePixelShader_Original                      = nullptr;
D3D11Dev_CreateGeometryShader_pfn                   D3D11Dev_CreateGeometryShader_Original                   = nullptr;
D3D11Dev_CreateGeometryShaderWithStreamOutput_pfn   D3D11Dev_CreateGeometryShaderWithStreamOutput_Original   = nullptr;
D3D11Dev_CreateHullShader_pfn                       D3D11Dev_CreateHullShader_Original                       = nullptr;
D3D11Dev_CreateDomainShader_pfn                     D3D11Dev_CreateDomainShader_Original                     = nullptr;
D3D11Dev_CreateComputeShader_pfn                    D3D11Dev_CreateComputeShader_Original                    = nullptr;

D3D11Dev_CreateDeferredContext_pfn                  D3D11Dev_CreateDeferredContext_Original                  = nullptr;
D3D11Dev_GetImmediateContext_pfn                    D3D11Dev_GetImmediateContext_Original                    = nullptr;

D3D11_RSSetScissorRects_pfn                         D3D11_RSSetScissorRects_Original                         = nullptr;
D3D11_RSSetViewports_pfn                            D3D11_RSSetViewports_Original                            = nullptr;
D3D11_VSSetConstantBuffers_pfn                      D3D11_VSSetConstantBuffers_Original                      = nullptr;
D3D11_VSSetShaderResources_pfn                      D3D11_VSSetShaderResources_Original                      = nullptr;
D3D11_PSSetShaderResources_pfn                      D3D11_PSSetShaderResources_Original                      = nullptr;
D3D11_PSSetConstantBuffers_pfn                      D3D11_PSSetConstantBuffers_Original                      = nullptr;
D3D11_GSSetShaderResources_pfn                      D3D11_GSSetShaderResources_Original                      = nullptr;
D3D11_HSSetShaderResources_pfn                      D3D11_HSSetShaderResources_Original                      = nullptr;
D3D11_DSSetShaderResources_pfn                      D3D11_DSSetShaderResources_Original                      = nullptr;
D3D11_CSSetShaderResources_pfn                      D3D11_CSSetShaderResources_Original                      = nullptr;
D3D11_CSSetUnorderedAccessViews_pfn                 D3D11_CSSetUnorderedAccessViews_Original                 = nullptr;
D3D11_UpdateSubresource_pfn                         D3D11_UpdateSubresource_Original                         = nullptr;
D3D11_DrawIndexed_pfn                               D3D11_DrawIndexed_Original                               = nullptr;
D3D11_Draw_pfn                                      D3D11_Draw_Original                                      = nullptr;
D3D11_DrawAuto_pfn                                  D3D11_DrawAuto_Original                                  = nullptr;
D3D11_DrawIndexedInstanced_pfn                      D3D11_DrawIndexedInstanced_Original                      = nullptr;
D3D11_DrawIndexedInstancedIndirect_pfn              D3D11_DrawIndexedInstancedIndirect_Original              = nullptr;
D3D11_DrawInstanced_pfn                             D3D11_DrawInstanced_Original                             = nullptr;
D3D11_DrawInstancedIndirect_pfn                     D3D11_DrawInstancedIndirect_Original                     = nullptr;
D3D11_Dispatch_pfn                                  D3D11_Dispatch_Original                                  = nullptr;
D3D11_DispatchIndirect_pfn                          D3D11_DispatchIndirect_Original                          = nullptr;
D3D11_Map_pfn                                       D3D11_Map_Original                                       = nullptr;
D3D11_Unmap_pfn                                     D3D11_Unmap_Original                                     = nullptr;

D3D11_OMSetRenderTargets_pfn                        D3D11_OMSetRenderTargets_Original                        = nullptr;
D3D11_OMSetRenderTargetsAndUnorderedAccessViews_pfn D3D11_OMSetRenderTargetsAndUnorderedAccessViews_Original = nullptr;
D3D11_OMGetRenderTargets_pfn                        D3D11_OMGetRenderTargets_Original                        = nullptr;
D3D11_OMGetRenderTargetsAndUnorderedAccessViews_pfn D3D11_OMGetRenderTargetsAndUnorderedAccessViews_Original = nullptr;
D3D11_ClearDepthStencilView_pfn                     D3D11_ClearDepthStencilView_Original                     = nullptr;

D3D11_PSSetSamplers_pfn                             D3D11_PSSetSamplers_Original                             = nullptr;

D3D11_VSSetShader_pfn                               D3D11_VSSetShader_Original                               = nullptr;
D3D11_PSSetShader_pfn                               D3D11_PSSetShader_Original                               = nullptr;
D3D11_GSSetShader_pfn                               D3D11_GSSetShader_Original                               = nullptr;
D3D11_HSSetShader_pfn                               D3D11_HSSetShader_Original                               = nullptr;
D3D11_DSSetShader_pfn                               D3D11_DSSetShader_Original                               = nullptr;
D3D11_CSSetShader_pfn                               D3D11_CSSetShader_Original                               = nullptr;

D3D11_VSGetShader_pfn                               D3D11_VSGetShader_Original                               = nullptr;
D3D11_PSGetShader_pfn                               D3D11_PSGetShader_Original                               = nullptr;
D3D11_GSGetShader_pfn                               D3D11_GSGetShader_Original                               = nullptr;
D3D11_HSGetShader_pfn                               D3D11_HSGetShader_Original                               = nullptr;
D3D11_DSGetShader_pfn                               D3D11_DSGetShader_Original                               = nullptr;
D3D11_CSGetShader_pfn                               D3D11_CSGetShader_Original                               = nullptr;

D3D11_GetData_pfn                                   D3D11_GetData_Original                                   = nullptr;

D3D11_CopyResource_pfn                              D3D11_CopyResource_Original                              = nullptr;
D3D11_CopySubresourceRegion_pfn                     D3D11_CopySubresourceRegion_Original                     = nullptr;
D3D11_UpdateSubresource1_pfn                        D3D11_UpdateSubresource1_Original                        = nullptr;


struct cache_params_s {
  uint32_t max_entries       = 4096UL;
  uint32_t min_entries       = 1024UL;
  uint32_t max_size          = 2048UL; // Measured in MiB
  uint32_t min_size          = 512UL;
  uint32_t min_evict         = 16;
  uint32_t max_evict         = 64;
      bool ignore_non_mipped = false;
} cache_opts;


//
// Helpers for typeless DXGI format class view compatibility
//
//   Returns true if this is a valid way of (re)interpreting a sized datatype
bool
SK_D3D11_IsDataSizeClassOf ( DXGI_FORMAT typeless, DXGI_FORMAT typed,
                             int         recursion = 0 )
{
  switch (typeless)
  {
    case DXGI_FORMAT_R24G8_TYPELESS:
      return true;

    case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
      return true;

    case DXGI_FORMAT_D24_UNORM_S8_UINT:
      return typed == DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
      return true;
  }

  if (recursion == 0)
    return SK_D3D11_IsDataSizeClassOf (typed, typeless, 1);

  return false;
}

bool
SK_D3D11_OverrideDepthStencil (DXGI_FORMAT& fmt) 
{
  if (! config.render.dxgi.enhanced_depth)
    return false;

  switch (fmt)
  {
    case DXGI_FORMAT_R24G8_TYPELESS:
      fmt = DXGI_FORMAT_R32G8X24_TYPELESS;
      return true;

    case DXGI_FORMAT_D24_UNORM_S8_UINT:
      fmt = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
      return true;

    case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
      fmt = DXGI_FORMAT_X32_TYPELESS_G8X24_UINT;
      return true;

    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
      fmt = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
      return true;
  }

  return false;
}


__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11Dev_CreateRenderTargetView_Override (
  _In_            ID3D11Device                   *This,
  _In_            ID3D11Resource                 *pResource,
  _In_opt_  const D3D11_RENDER_TARGET_VIEW_DESC  *pDesc,
  _Out_opt_       ID3D11RenderTargetView        **ppRTView )
{
  // Unity throws around NULL for pResource
  if (pDesc != nullptr && pResource != nullptr)
  {
    D3D11_RENDER_TARGET_VIEW_DESC desc = { };
    D3D11_RESOURCE_DIMENSION      dim  = { };

    pResource->GetType (&dim);

    if (dim == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
    {
      desc = *pDesc;

      DXGI_FORMAT newFormat =
        pDesc->Format;

      CComQIPtr <ID3D11Texture2D> pTex (pResource);

      if (pTex != nullptr)
      {
        D3D11_TEXTURE2D_DESC  tex_desc = { };
        pTex->GetDesc       (&tex_desc);

        if ( SK_D3D11_OverrideDepthStencil (newFormat) )
          desc.Format = newFormat;

        if (SK_GetCurrentGameID () == SK_GAME_ID::DotHackGU)
        {
          if ((pDesc->Format == DXGI_FORMAT_B8G8R8A8_UNORM || pDesc->Format == DXGI_FORMAT_B8G8R8A8_TYPELESS))
          {
            newFormat   = pDesc->Format == DXGI_FORMAT_B8G8R8A8_UNORM ? DXGI_FORMAT_R8G8B8A8_UNORM :
                                                                        DXGI_FORMAT_R8G8B8A8_TYPELESS;
            desc.Format = newFormat;
          }
        }

        const HRESULT hr =
          D3D11Dev_CreateRenderTargetView_Original ( This, pResource,
                                                       &desc, ppRTView );

        if (SUCCEEDED (hr))
          return hr;
      }
    }
  }

  return
    D3D11Dev_CreateRenderTargetView_Original ( This,  pResource,
                                                 pDesc, ppRTView );
}

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

std::array <SK_D3D11_KnownTargets, SK_D3D11_MAX_DEV_CONTEXTS + 1> SK_D3D11_RenderTargets;


struct SK_D3D11_KnownThreads
{
  SK_D3D11_KnownThreads (void) ///
  {
    InitializeCriticalSectionAndSpinCount (&cs, 0x400);

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
} SK_D3D11_MemoryThreads,
  SK_D3D11_DrawThreads,
  SK_D3D11_DispatchThreads,
  SK_D3D11_ShaderThreads;


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
    ids.emplace    (GetCurrentThreadId ());
    active.emplace (GetCurrentThreadId ());
    return;
  }

  ids.emplace    (GetCurrentThreadId ());
  active.emplace (GetCurrentThreadId ());
#endif
}

#include <array>

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
        if (mapped_resources.size ())
        {
          std::swap (mapped_resources, fresh.mapped_resources);
                                       fresh.mapped_resources.clear ();
        }

        if (index_buffers.size ())
        {
          std::swap (index_buffers, fresh.index_buffers);
                                    fresh.index_buffers.clear ();
        }

        if (vertex_buffers.size ())
        {
          std::swap (vertex_buffers, fresh.vertex_buffers);
                                     fresh.vertex_buffers.clear ();
        }

        if (constant_buffers.size ())
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
    }

    lifetime.bytes_read    += last_frame.bytes_read;
    lifetime.bytes_written += last_frame.bytes_written;
    lifetime.bytes_copied  += last_frame.bytes_copied;

    last_frame.clear (cs);
  }

  SK_Thread_HybridSpinlock*& cs;
} mem_map_stats;

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
      if (ref_vs.size ())
      {
        std::swap (ref_vs, fresh.ref_vs);
                           fresh.ref_vs.clear ();
      }

      if (ref_ps.size ())
      {
        std::swap (ref_ps, fresh.ref_ps);
                           fresh.ref_ps.clear ();
      }

      if (ref_gs.size ())
      {
        std::swap (ref_gs, fresh.ref_gs);
                           fresh.ref_gs.clear ();
      }

      if (ref_ds.size ())
      {
        std::swap (ref_ds, fresh.ref_ds);
                           fresh.ref_ds.clear ();
      }

      if (ref_hs.size ())
      {
        std::swap (ref_hs, fresh.ref_hs);
                           fresh.ref_hs.clear ();
      }

      if (ref_cs.size ())
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
} tracked_rtv;

ID3D11Texture2D* tracked_texture               = nullptr;
DWORD            tracked_tex_blink_duration    = 666UL;
DWORD            tracked_shader_blink_duration = 666UL;

struct SK_DisjointTimerQueryD3D11 d3d11_shader_tracking_s::disjoint_query;



void
d3d11_shader_tracking_s::activate ( ID3D11DeviceContext        *pDevContext,
                                    ID3D11ClassInstance *const *ppClassInstances,
                                    UINT                        NumClassInstances )
{
  for ( UINT i = 0 ; i < NumClassInstances ; i++ )
  {
    if (ppClassInstances && ppClassInstances [i])
        addClassInstance   (ppClassInstances [i]);
  }

  const UINT dev_idx =
    SK_D3D11_GetDeviceContextHandle (pDevContext);

  const bool is_active =
    active.get (dev_idx);

  static auto& shaders =
    SK_D3D11_Shaders;

  if ((! is_active))
  {
    active.set (dev_idx, true);

    switch (type_)
    {
      case SK_D3D11_ShaderType::Vertex:
        shaders.vertex.current.shader   [dev_idx] = crc32c;
        break;
      case SK_D3D11_ShaderType::Pixel:
        shaders.pixel.current.shader    [dev_idx] = crc32c;
        break;
      case SK_D3D11_ShaderType::Geometry:
        shaders.geometry.current.shader [dev_idx] = crc32c;
        break;
      case SK_D3D11_ShaderType::Domain:
        shaders.domain.current.shader   [dev_idx] = crc32c;
        break;
      case SK_D3D11_ShaderType::Hull:
        shaders.hull.current.shader     [dev_idx] = crc32c;
        break;
      case SK_D3D11_ShaderType::Compute:
        shaders.compute.current.shader  [dev_idx] = crc32c;
        break;
    }
  }

  else
    return;


  // Timing is very difficult on deferred contexts; will finish later (years?)
  if ( pDevContext != nullptr  &&
       pDevContext->GetType () == D3D11_DEVICE_CONTEXT_DEFERRED )
    return;


  static SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  CComQIPtr <ID3D11Device> dev (rb.device);

  if (! dev)
    return;

  if (ReadPointerAcquire ((void **)&disjoint_query.async) == nullptr && timers.empty ())
  {
    D3D11_QUERY_DESC query_desc {
      D3D11_QUERY_TIMESTAMP_DISJOINT, 0x00
    };

    ID3D11Query* pQuery = nullptr;
    if (SUCCEEDED (dev->CreateQuery (&query_desc, &pQuery)))
    {
      CComPtr <ID3D11DeviceContext> pImmediateContext;
      dev->GetImmediateContext    (&pImmediateContext);

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

    ID3D11Query* pQuery = nullptr;
    if (SUCCEEDED (dev->CreateQuery (&query_desc, &pQuery)))
    {
      InterlockedExchangePointer ((void **)&duration.start.dev_ctx, pDevContext);
      InterlockedExchangePointer ((void **)&duration.start.async,   pQuery);
      pDevContext->End                                             (pQuery);
      timers.emplace_back (duration);
    }
  }
}

void
d3d11_shader_tracking_s::deactivate (ID3D11DeviceContext* pDevCtx)
{
  static SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  const UINT dev_idx =
    SK_D3D11_GetDeviceContextHandle (pDevCtx);

  const bool is_active =
    active.get (dev_idx);

  static auto& shaders =
    SK_D3D11_Shaders;

  if (is_active)
  {
    active.set (dev_idx, false);

    bool end_of_frame = false;

    if (pDevCtx == nullptr)
    {
      end_of_frame = true;
      pDevCtx      = static_cast <ID3D11DeviceContext *>(rb.d3d11.immediate_ctx.p);
    }

    switch (type_)
    {
      case SK_D3D11_ShaderType::Vertex:
        shaders.vertex.current.shader [dev_idx]   = 0x0;
        break;
      case SK_D3D11_ShaderType::Pixel:
        shaders.pixel.current.shader  [dev_idx]    = 0x0;
        break;
      case SK_D3D11_ShaderType::Geometry:
        shaders.geometry.current.shader [dev_idx] = 0x0;
        break;
      case SK_D3D11_ShaderType::Domain:
        shaders.domain.current.shader [dev_idx]   = 0x0;
        break;
      case SK_D3D11_ShaderType::Hull:
        shaders.hull.current.shader [dev_idx]     = 0x0;
        break;
      case SK_D3D11_ShaderType::Compute:
        shaders.compute.current.shader [dev_idx]  = 0x0;
        break;
    }

    if (end_of_frame) return;
  }

  else
    return;


  // Timing is very difficult on deferred contexts; will finish later (years?)
  if ( pDevCtx != nullptr && pDevCtx->GetType () == D3D11_DEVICE_CONTEXT_DEFERRED )
    return;


  CComQIPtr <ID3D11Device> dev (rb.device);

  if (dev != nullptr && ReadAcquire (&disjoint_query.active))
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
      );                              pDevCtx->End (pQuery);
    }
  }
}

__forceinline
void
d3d11_shader_tracking_s::use (IUnknown* pShader) 
{
  UNREFERENCED_PARAMETER (pShader);

  num_draws++;
}


bool drawing_cpl = false;


NvAPI_Status
__cdecl
NvAPI_D3D11_CreateVertexShaderEx_Override ( __in        ID3D11Device *pDevice,        __in     const void                *pShaderBytecode, 
                                            __in        SIZE_T        BytecodeLength, __in_opt       ID3D11ClassLinkage  *pClassLinkage, 
                                            __in  const LPVOID                                                            pCreateVertexShaderExArgs,
                                            __out       ID3D11VertexShader                                              **ppVertexShader )
{
  SK_D3D11_ShaderThreads.mark ();


  const NvAPI_Status ret =
    NvAPI_D3D11_CreateVertexShaderEx_Original ( pDevice,
                                                  pShaderBytecode, BytecodeLength,
                                                    pClassLinkage, pCreateVertexShaderExArgs,
                                                      ppVertexShader );

  if (ret == NVAPI_OK)
  {
    static auto& shaders =
      SK_D3D11_Shaders;

    uint32_t checksum =
      safe_crc32c (0x00, static_cast <const uint8_t *> (pShaderBytecode), BytecodeLength);

    cs_shader_vs->lock ();

    if (! shaders.vertex.descs.count (checksum))
    {
      SK_D3D11_ShaderDesc desc;

      desc.type   = SK_D3D11_ShaderType::Vertex;
      desc.crc32c = checksum;

      desc.bytecode.insert ( desc.bytecode.end  (),
        &((uint8_t *) pShaderBytecode) [0],
        &((uint8_t *) pShaderBytecode) [BytecodeLength]
      );

      shaders.vertex.descs.emplace (std::make_pair (checksum, desc));
    }

    if ( shaders.vertex.rev.count (*ppVertexShader) &&
               shaders.vertex.rev [*ppVertexShader] != checksum )
         shaders.vertex.rev.erase (*ppVertexShader);

    shaders.vertex.rev.emplace (std::make_pair (*ppVertexShader, checksum));

    SK_D3D11_ShaderDesc& desc =
      shaders.vertex.descs [checksum];

    cs_shader_vs->unlock ();

    InterlockedExchange (&desc.usage.last_frame, SK_GetFramesDrawn ());
                _time64 (&desc.usage.last_time);
  }

  return ret;
}

NvAPI_Status
__cdecl
NvAPI_D3D11_CreateHullShaderEx_Override ( __in        ID3D11Device *pDevice,        __in const void               *pShaderBytecode, 
                                          __in        SIZE_T        BytecodeLength, __in_opt   ID3D11ClassLinkage *pClassLinkage, 
                                          __in  const LPVOID                                                       pCreateHullShaderExArgs,
                                          __out       ID3D11HullShader                                           **ppHullShader )
{
  SK_D3D11_ShaderThreads.mark ();


  const NvAPI_Status ret =
    NvAPI_D3D11_CreateHullShaderEx_Original ( pDevice,
                                                pShaderBytecode, BytecodeLength,
                                                  pClassLinkage, pCreateHullShaderExArgs,
                                                    ppHullShader );

  if (ret == NVAPI_OK)
  {
    uint32_t checksum =
      safe_crc32c (0x00, static_cast <const uint8_t *> (pShaderBytecode), BytecodeLength);

    cs_shader_hs->lock ();

    static auto& hull =
      SK_D3D11_Shaders.hull;

    if (! hull.descs.count (checksum))
    {
      SK_D3D11_ShaderDesc desc;
      desc.type   = SK_D3D11_ShaderType::Hull;
      desc.crc32c = checksum;

      desc.bytecode.insert ( desc.bytecode.end  (),
        &((uint8_t *) pShaderBytecode) [0],
        &((uint8_t *) pShaderBytecode) [BytecodeLength]
      );

      hull.descs.emplace (std::make_pair (checksum, desc));
    }

    if ( hull.rev.count (*ppHullShader) &&
               hull.rev [*ppHullShader] != checksum )
         hull.rev.erase (*ppHullShader);

    hull.rev.emplace (std::make_pair (*ppHullShader, checksum));

    SK_D3D11_ShaderDesc& desc =
      hull.descs [checksum];

    cs_shader_hs->unlock ();

    InterlockedExchange (&desc.usage.last_frame, SK_GetFramesDrawn ());
                _time64 (&desc.usage.last_time);
  }

  return ret;
}

NvAPI_Status
__cdecl
NvAPI_D3D11_CreateDomainShaderEx_Override ( __in        ID3D11Device *pDevice,        __in     const void               *pShaderBytecode, 
                                            __in        SIZE_T        BytecodeLength, __in_opt       ID3D11ClassLinkage *pClassLinkage, 
                                            __in  const LPVOID                                                           pCreateDomainShaderExArgs,
                                            __out       ID3D11DomainShader                                             **ppDomainShader )
{
  SK_D3D11_ShaderThreads.mark ();


  const NvAPI_Status ret =
    NvAPI_D3D11_CreateDomainShaderEx_Original ( pDevice,
                                                  pShaderBytecode, BytecodeLength,
                                                    pClassLinkage, pCreateDomainShaderExArgs,
                                                      ppDomainShader );

  if (ret == NVAPI_OK)
  {
    uint32_t checksum =
      safe_crc32c (0x00, static_cast <const uint8_t *> (pShaderBytecode), BytecodeLength);

    cs_shader_ds->lock ();

    static auto& domain =
      SK_D3D11_Shaders.domain;

    if (! domain.descs.count (checksum))
    {
      SK_D3D11_ShaderDesc desc;
      desc.type   = SK_D3D11_ShaderType::Domain;
      desc.crc32c = checksum;

      desc.bytecode.insert ( desc.bytecode.end  (),
        &((uint8_t *) pShaderBytecode) [0],
        &((uint8_t *) pShaderBytecode) [BytecodeLength]
      );

      domain.descs.emplace (std::make_pair (checksum, desc));
    }

    if ( domain.rev.count (*ppDomainShader) &&
         domain.rev       [*ppDomainShader] != checksum )
         domain.rev.erase (*ppDomainShader);

    domain.rev.emplace (std::make_pair (*ppDomainShader, checksum));

    SK_D3D11_ShaderDesc& desc =
      domain.descs [checksum];

    cs_shader_ds->unlock ();

    InterlockedExchange (&desc.usage.last_frame, SK_GetFramesDrawn ());
                _time64 (&desc.usage.last_time);
  }

  return ret;
}

NvAPI_Status
__cdecl
NvAPI_D3D11_CreateGeometryShaderEx_2_Override ( __in        ID3D11Device *pDevice,        __in     const void               *pShaderBytecode, 
                                                __in        SIZE_T        BytecodeLength, __in_opt       ID3D11ClassLinkage *pClassLinkage, 
                                                __in  const LPVOID                                                           pCreateGeometryShaderExArgs,
                                                __out       ID3D11GeometryShader                                           **ppGeometryShader )
{
  SK_D3D11_ShaderThreads.mark ();


  NvAPI_Status ret =
    NvAPI_D3D11_CreateGeometryShaderEx_2_Original ( pDevice,
                                                      pShaderBytecode, BytecodeLength,
                                                        pClassLinkage, pCreateGeometryShaderExArgs,
                                                          ppGeometryShader );

  if (ret == NVAPI_OK)
  {
    uint32_t checksum =
      safe_crc32c (0x00, (const uint8_t *)pShaderBytecode, BytecodeLength);

    cs_shader_gs->lock ();

    static auto& geometry =
      SK_D3D11_Shaders.geometry;

    if (! geometry.descs.count (checksum))
    {
      SK_D3D11_ShaderDesc desc;
      desc.type   = SK_D3D11_ShaderType::Geometry;
      desc.crc32c = checksum;

      desc.bytecode.insert ( desc.bytecode.end  (),
        &((uint8_t *) pShaderBytecode) [0],
        &((uint8_t *) pShaderBytecode) [BytecodeLength]
      );

      geometry.descs.emplace (std::make_pair (checksum, desc));
    }

    if ( geometry.rev.count (*ppGeometryShader) &&
         geometry.rev       [*ppGeometryShader] != checksum )
         geometry.rev.erase (*ppGeometryShader);

    geometry.rev.emplace (std::make_pair (*ppGeometryShader, checksum));

    SK_D3D11_ShaderDesc& desc =
     geometry.descs [checksum];

    cs_shader_gs->unlock ();

    InterlockedExchange (&desc.usage.last_frame, SK_GetFramesDrawn ());
                _time64 (&desc.usage.last_time);
  }

  return ret;
}

NvAPI_Status
__cdecl
NvAPI_D3D11_CreateFastGeometryShaderExplicit_Override ( __in        ID3D11Device *pDevice,        __in     const void               *pShaderBytecode,
                                                        __in        SIZE_T        BytecodeLength, __in_opt       ID3D11ClassLinkage *pClassLinkage,
                                                        __in  const LPVOID                                                           pCreateFastGSArgs,
                                                        __out       ID3D11GeometryShader                                           **ppGeometryShader )
{
  SK_D3D11_ShaderThreads.mark ();


  const NvAPI_Status ret =
    NvAPI_D3D11_CreateFastGeometryShaderExplicit_Original ( pDevice,
                                                              pShaderBytecode, BytecodeLength,
                                                                pClassLinkage, pCreateFastGSArgs,
                                                                  ppGeometryShader );

  if (ret == NVAPI_OK)
  {
    uint32_t checksum =
      safe_crc32c (0x00, (const uint8_t *)pShaderBytecode, BytecodeLength);

    cs_shader_gs->lock ();

    static auto& geometry =
      SK_D3D11_Shaders.geometry;

    if (! geometry.descs.count (checksum))
    {
      SK_D3D11_ShaderDesc desc;
      desc.type   = SK_D3D11_ShaderType::Geometry;
      desc.crc32c = checksum;

      desc.bytecode.insert ( desc.bytecode.end  (),
        &((uint8_t *) pShaderBytecode) [0],
        &((uint8_t *) pShaderBytecode) [BytecodeLength]
      );

      geometry.descs.emplace (std::make_pair (checksum, desc));
    }

    if ( geometry.rev.count (*ppGeometryShader) &&
               geometry.rev [*ppGeometryShader] != checksum )
         geometry.rev.erase (*ppGeometryShader);

    geometry.rev.emplace (std::make_pair (*ppGeometryShader, checksum));

    SK_D3D11_ShaderDesc& desc =
      geometry.descs [checksum];

    cs_shader_gs->unlock ();

    InterlockedExchange (&desc.usage.last_frame, SK_GetFramesDrawn ());
                _time64 (&desc.usage.last_time);
  }

  return ret;
}

NvAPI_Status
__cdecl
NvAPI_D3D11_CreateFastGeometryShader_Override ( __in  ID3D11Device *pDevice,        __in     const void                *pShaderBytecode, 
                                                __in  SIZE_T        BytecodeLength, __in_opt       ID3D11ClassLinkage  *pClassLinkage,
                                                __out ID3D11GeometryShader                                            **ppGeometryShader )
{
  SK_D3D11_ShaderThreads.mark ();


  const NvAPI_Status ret =
    NvAPI_D3D11_CreateFastGeometryShader_Original ( pDevice,
                                                      pShaderBytecode, BytecodeLength,
                                                        pClassLinkage, ppGeometryShader );

  if (ret == NVAPI_OK)
  {
    uint32_t checksum =
      safe_crc32c (0x00, (const uint8_t *)pShaderBytecode, BytecodeLength);

    cs_shader_gs->lock ();

    static auto& geometry =
      SK_D3D11_Shaders.geometry;

    if (! geometry.descs.count (checksum))
    {
      SK_D3D11_ShaderDesc desc;
      desc.type   = SK_D3D11_ShaderType::Geometry;
      desc.crc32c = checksum;

      desc.bytecode.insert ( desc.bytecode.end  (),
        &((uint8_t *) pShaderBytecode) [0],
        &((uint8_t *) pShaderBytecode) [BytecodeLength]
      );

      geometry.descs.emplace (std::make_pair (checksum, desc));
    }

    if ( geometry.rev.count (*ppGeometryShader) &&
               geometry.rev [*ppGeometryShader] != checksum )
         geometry.rev.erase (*ppGeometryShader);

    geometry.rev.emplace (std::make_pair (*ppGeometryShader, checksum));

    SK_D3D11_ShaderDesc& desc =
      geometry.descs [checksum];

    cs_shader_gs->unlock ();

    InterlockedExchange (&desc.usage.last_frame, SK_GetFramesDrawn ());
                _time64 (&desc.usage.last_time);
  }

  return ret;
}

uint32_t
__cdecl
SK_D3D11_ChecksumShaderBytecode ( _In_      const void                *pShaderBytecode,
                                  _In_            SIZE_T               BytecodeLength  )
{
  __try
  {
    return
      safe_crc32c (0x00, static_cast <const uint8_t *> (pShaderBytecode), BytecodeLength);
  }

  __except ( ( GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ) ? 
             EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH )
  {
    SK_LOG0 ( (L" >> Threw out disassembled shader due to access violation during hash."),
               L"   DXGI   ");
    return 0x00;
  }
}

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
          pShaderRepo->descs.emplace (std::make_pair (checksum, desc));

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

      pCritical->unlock ();
    }
  }

  return hr;
}


__declspec (noinline)
HRESULT
WINAPI
D3D11Dev_CreateVertexShader_Override (
  _In_            ID3D11Device        *This,
  _In_      const void                *pShaderBytecode,
  _In_            SIZE_T               BytecodeLength,
  _In_opt_        ID3D11ClassLinkage  *pClassLinkage,
  _Out_opt_       ID3D11VertexShader **ppVertexShader ) 
{
  return
    SK_D3D11_CreateShader_Impl ( This,
                                   pShaderBytecode, BytecodeLength,
                                     pClassLinkage,
                         (IUnknown **)(ppVertexShader),
                                         sk_shader_class::Vertex );
}

__declspec (noinline)
HRESULT
WINAPI
D3D11Dev_CreatePixelShader_Override (
  _In_            ID3D11Device        *This,
  _In_      const void                *pShaderBytecode,
  _In_            SIZE_T               BytecodeLength,
  _In_opt_        ID3D11ClassLinkage  *pClassLinkage,
  _Out_opt_       ID3D11PixelShader  **ppPixelShader ) 
{
  return
    SK_D3D11_CreateShader_Impl ( This,
                                   pShaderBytecode, BytecodeLength,
                                     pClassLinkage,
                         (IUnknown **)(ppPixelShader),
                                         sk_shader_class::Pixel );
}

__declspec (noinline)
HRESULT
WINAPI
D3D11Dev_CreateGeometryShader_Override (
  _In_            ID3D11Device          *This,
  _In_      const void                  *pShaderBytecode,
  _In_            SIZE_T                 BytecodeLength,
  _In_opt_        ID3D11ClassLinkage    *pClassLinkage,
  _Out_opt_       ID3D11GeometryShader **ppGeometryShader ) 
{
  return
    SK_D3D11_CreateShader_Impl ( This,
                                   pShaderBytecode, BytecodeLength,
                                     pClassLinkage,
                         (IUnknown **)(ppGeometryShader),
                                         sk_shader_class::Geometry );
}

__declspec (noinline)
HRESULT
WINAPI
D3D11Dev_CreateGeometryShaderWithStreamOutput_Override (
  _In_            ID3D11Device               *This,
  _In_      const void                       *pShaderBytecode,
  _In_            SIZE_T                     BytecodeLength,
  _In_opt_  const D3D11_SO_DECLARATION_ENTRY *pSODeclaration,
  _In_            UINT                       NumEntries,
  _In_opt_  const UINT                       *pBufferStrides,
  _In_            UINT                       NumStrides,
  _In_            UINT                       RasterizedStream,
  _In_opt_        ID3D11ClassLinkage         *pClassLinkage,
  _Out_opt_       ID3D11GeometryShader      **ppGeometryShader ) 
{
  const HRESULT hr =
    D3D11Dev_CreateGeometryShaderWithStreamOutput_Original ( This, pShaderBytecode,
                                                               BytecodeLength,
                                                                 pSODeclaration, NumEntries,
                                                                   pBufferStrides, NumStrides,
                                                                     RasterizedStream, pClassLinkage,
                                                                       ppGeometryShader );

  if (SUCCEEDED (hr) && ppGeometryShader)
  {
    static auto& geo_shaders =
      SK_D3D11_Shaders.geometry;

    uint32_t checksum =
      SK_D3D11_ChecksumShaderBytecode (pShaderBytecode, BytecodeLength);

    if (checksum == 0x00)
      checksum = (uint32_t)BytecodeLength;

    cs_shader_gs->lock ();

    if (! geo_shaders.descs.count (checksum))
    {
      SK_D3D11_ShaderDesc desc;

      desc.type   = SK_D3D11_ShaderType::Geometry;
      desc.crc32c = checksum;

      desc.bytecode.insert ( desc.bytecode.end  (),
        &((uint8_t *) pShaderBytecode) [0],
        &((uint8_t *) pShaderBytecode) [BytecodeLength]
      );

      geo_shaders.descs.emplace (std::make_pair (checksum, desc));
    }

    if ( geo_shaders.rev.count (*ppGeometryShader) &&
               geo_shaders.rev [*ppGeometryShader] != checksum )
         geo_shaders.rev.erase (*ppGeometryShader);

    geo_shaders.rev.emplace (std::make_pair (*ppGeometryShader, checksum));

    SK_D3D11_ShaderDesc& desc =
      geo_shaders.descs [checksum];

    cs_shader_gs->unlock ();

    InterlockedExchange (&desc.usage.last_frame, SK_GetFramesDrawn ());
                _time64 (&desc.usage.last_time);
  }

  return hr;
}

__declspec (noinline)
HRESULT
WINAPI
D3D11Dev_CreateHullShader_Override (
  _In_            ID3D11Device        *This,
  _In_      const void                *pShaderBytecode,
  _In_            SIZE_T               BytecodeLength,
  _In_opt_        ID3D11ClassLinkage  *pClassLinkage,
  _Out_opt_       ID3D11HullShader   **ppHullShader ) 
{
  return
    SK_D3D11_CreateShader_Impl ( This,
                                   pShaderBytecode, BytecodeLength,
                                     pClassLinkage,
                         (IUnknown **)(ppHullShader),
                                         sk_shader_class::Hull );
}

__declspec (noinline)
HRESULT
WINAPI
D3D11Dev_CreateDomainShader_Override (
  _In_            ID3D11Device        *This,
  _In_      const void                *pShaderBytecode,
  _In_            SIZE_T               BytecodeLength,
  _In_opt_        ID3D11ClassLinkage  *pClassLinkage,
  _Out_opt_       ID3D11DomainShader **ppDomainShader ) 
{
  return
    SK_D3D11_CreateShader_Impl ( This,
                                   pShaderBytecode, BytecodeLength,
                                     pClassLinkage,
                         (IUnknown **)(ppDomainShader),
                                         sk_shader_class::Domain );
}

__declspec (noinline)
HRESULT
WINAPI
D3D11Dev_CreateComputeShader_Override (
  _In_            ID3D11Device         *This,
  _In_      const void                 *pShaderBytecode,
  _In_            SIZE_T                BytecodeLength,
  _In_opt_        ID3D11ClassLinkage   *pClassLinkage,
  _Out_opt_       ID3D11ComputeShader **ppComputeShader ) 
{
  return
    SK_D3D11_CreateShader_Impl ( This,
                                   pShaderBytecode, BytecodeLength,
                                     pClassLinkage,
                         (IUnknown **)(ppComputeShader),
                                         sk_shader_class::Compute );
}


std::array <bool, SK_D3D11_MAX_DEV_CONTEXTS+1> reshade_trigger_before;
std::array <bool, SK_D3D11_MAX_DEV_CONTEXTS+1> reshade_trigger_after;

__forceinline
void
SK_D3D11_SetShader_Impl ( ID3D11DeviceContext* pDevCtx, IUnknown* pShader, sk_shader_class type, ID3D11ClassInstance *const *ppClassInstances,
                                                                                                 UINT                        NumClassInstances ) 
{
  static auto& shaders =
    SK_D3D11_Shaders;

  const auto Finish =
  [&]
  {
    switch (type)
    {
      case sk_shader_class::Vertex:
        return D3D11_VSSetShader_Original ( pDevCtx, (ID3D11VertexShader *)pShader,
                                              ppClassInstances, NumClassInstances );
      case sk_shader_class::Pixel:
        return D3D11_PSSetShader_Original ( pDevCtx, (ID3D11PixelShader *)pShader,
                                              ppClassInstances, NumClassInstances );
      case sk_shader_class::Geometry:
        return D3D11_GSSetShader_Original ( pDevCtx, (ID3D11GeometryShader *)pShader,
                                              ppClassInstances, NumClassInstances );
      case sk_shader_class::Domain:
        return D3D11_DSSetShader_Original ( pDevCtx, (ID3D11DomainShader *)pShader,
                                              ppClassInstances, NumClassInstances );
      case sk_shader_class::Hull:
        return D3D11_HSSetShader_Original ( pDevCtx, (ID3D11HullShader *)pShader,
                                              ppClassInstances, NumClassInstances );
      case sk_shader_class::Compute:
        return D3D11_CSSetShader_Original ( pDevCtx, (ID3D11ComputeShader *)pShader,
                                              ppClassInstances, NumClassInstances );
    }
  };


  static const auto& rb =
    SK_GetCurrentRenderBackend ();

  bool implicit_track = false;

  if ( rb.isHDRCapable ()  &&
      (rb.framebuffer_flags & SK_FRAMEBUFFER_FLAG_HDR) )
  {
    if ( type == sk_shader_class::Vertex ||
         type == sk_shader_class::Pixel     )
    {
      // Needed for Steam Overlay HDR fix
      implicit_track = true;
    }
  }

  if (! ( implicit_track                                                     ||
          SK_D3D11_ShouldTrackDrawCall (pDevCtx, SK_D3D11DrawType::PrimList) ||
          SK_D3D11_ShouldTrackRenderOp (pDevCtx) ) )
  {
    return Finish ();
  }


  const auto GetResources =
  [&]( SK_Thread_CriticalSection**                        ppCritical,
       SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>** ppShaderDomain )
  {
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

  const UINT dev_idx =
    SK_D3D11_GetDeviceContextHandle (pDevCtx);


  SK_Thread_CriticalSection*                        pCritical   = nullptr;
  SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>* pShaderRepo = nullptr;

  GetResources (&pCritical, &pShaderRepo);

  // ImGui gets to pass-through without invoking the hook
  /////if (! SK_D3D11_ShouldTrackRenderOp (pDevCtx))
  /////{
  /////  pShaderRepo->tracked.deactivate (pDevCtx);
  /////
  /////  return Finish ();
  /////}


  SK_D3D11_ShaderDesc *pDesc = nullptr;
  if (pShader != nullptr)
  {
    std::lock_guard <SK_Thread_CriticalSection> auto_lock (*pCritical);

    if (pShaderRepo->rev.count (pShader))
      pDesc = &pShaderRepo->descs [pShaderRepo->rev [pShader]];
  }

  if (pDesc != nullptr)
  {
    InterlockedIncrement (&pShaderRepo->changes_last_frame);

    uint32_t checksum =
      pDesc->crc32c;

    pShaderRepo->current.shader [dev_idx] = checksum;

    InterlockedExchange (&pDesc->usage.last_frame, SK_GetFramesDrawn ());
    _time64             (&pDesc->usage.last_time);


    if (! shaders.reshade_triggered)
    {
      if (! pShaderRepo->trigger_reshade.before.empty ())
      {
        reshade_trigger_before [dev_idx] =
          pShaderRepo->trigger_reshade.before.count (checksum) ?
            true : false;
      }

      if (! pShaderRepo->trigger_reshade.after.empty ())
      {
        reshade_trigger_after [dev_idx] =
          pShaderRepo->trigger_reshade.after.count (checksum) ?
            true : false;
      }
    }

    else
    {
      //reshade_trigger_after  [dev_idx] = false;
      //reshade_trigger_before [dev_idx] = false;
    }


    if (checksum == pShaderRepo->tracked.crc32c)
    {
      pShaderRepo->tracked.activate (pDevCtx, ppClassInstances, NumClassInstances);

      return Finish ();
    }
  }

  else
  {
    pShaderRepo->current.shader [dev_idx] = 0x0;
  }

  pShaderRepo->tracked.deactivate (pDevCtx);

  return Finish ();
}

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11_GetData_Override (
 _In_  ID3D11DeviceContext *This,
 _In_  ID3D11Asynchronous  *pAsync,
 _Out_writes_bytes_opt_   ( DataSize )
       void                *pData,
 _In_  UINT                 DataSize,
 _In_  UINT                 GetDataFlags )
{
  HRESULT hr =
    D3D11_GetData_Original (
    This,    pAsync,
      pData, DataSize,
          GetDataFlags     );
  
    //dll_log.Log (L"Query Type: %lu, Misc Flags: %x - Result: %x, Value: %llu", qDesc.Query,
    //             qDesc.MiscFlags, hr, *(uint64_t *)pData);
  
  return hr;
  //dll_log.Log (L"Query - Size: %lu", DataSize);

  ///if (pCounter)
  ///{
  ///  dll_log.Log (L"Counter - Size: %lu", DataSize);
  ///}
  auto isDataReady =
   [&](void)
     -> bool
        {
          HRESULT hr =
            D3D11_GetData_Original (
              This, pAsync,
                    nullptr, 0,
                             GetDataFlags |
                             D3D11_ASYNC_GETDATA_DONOTFLUSH
                                   );
          return
            ( SUCCEEDED (hr) &&
                         hr  != S_FALSE );
        };


  auto finishGetData =
   [&](void)
     -> HRESULT
        {
          return
            D3D11_GetData_Original (
              This,    pAsync,
                pData, DataSize,
                    GetDataFlags
                                   );
        };
  
  extern bool
         __SK_DQXI_MakeAsyncObjectsGreatAgain;
  if ((! __SK_DQXI_MakeAsyncObjectsGreatAgain) ||
       (pData == nullptr && DataSize == 0)     ||
                           isDataReady ())
  {
    return
      finishGetData ();
  }
  
  int spins = 0;
  
  do
  {
    YieldProcessor ();
  
    if (++spins > 3)
    {
      break;
    }
  } while (! isDataReady ());

  return
    finishGetData ();
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_VSSetShader_Override (
 _In_     ID3D11DeviceContext        *This,
 _In_opt_ ID3D11VertexShader         *pVertexShader,
 _In_opt_ ID3D11ClassInstance *const *ppClassInstances,
          UINT                        NumClassInstances ) 
{
  return
    SK_D3D11_SetShader_Impl ( This, pVertexShader,
                                sk_shader_class::Vertex,
                                  ppClassInstances, NumClassInstances );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_VSGetShader_Override (
 _In_        ID3D11DeviceContext  *This,
 _Out_       ID3D11VertexShader  **ppVertexShader,
 _Out_opt_   ID3D11ClassInstance **ppClassInstances,
 _Inout_opt_ UINT                 *pNumClassInstances ) 
{
  return
    D3D11_VSGetShader_Original ( This, ppVertexShader,
                                 ppClassInstances, pNumClassInstances );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_PSSetShader_Override (
 _In_     ID3D11DeviceContext        *This,
 _In_opt_ ID3D11PixelShader          *pPixelShader,
 _In_opt_ ID3D11ClassInstance *const *ppClassInstances,
          UINT                        NumClassInstances ) 
{
  return
    SK_D3D11_SetShader_Impl ( This, pPixelShader,
                                sk_shader_class::Pixel,
                                  ppClassInstances, NumClassInstances );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_PSGetShader_Override (
 _In_        ID3D11DeviceContext  *This,
 _Out_       ID3D11PixelShader   **ppPixelShader,
 _Out_opt_   ID3D11ClassInstance **ppClassInstances,
 _Inout_opt_ UINT                 *pNumClassInstances ) 
{
  return D3D11_PSGetShader_Original ( This, ppPixelShader,
                                        ppClassInstances, pNumClassInstances );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_GSSetShader_Override (
 _In_     ID3D11DeviceContext        *This,
 _In_opt_ ID3D11GeometryShader       *pGeometryShader,
 _In_opt_ ID3D11ClassInstance *const *ppClassInstances,
          UINT                        NumClassInstances ) 
{
  return
    SK_D3D11_SetShader_Impl ( This, pGeometryShader,
                                sk_shader_class::Geometry,
                                  ppClassInstances, NumClassInstances );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_GSGetShader_Override (
 _In_        ID3D11DeviceContext   *This,
 _Out_       ID3D11GeometryShader **ppGeometryShader,
 _Out_opt_   ID3D11ClassInstance  **ppClassInstances,
 _Inout_opt_ UINT                  *pNumClassInstances ) 
{
  return D3D11_GSGetShader_Original (This, ppGeometryShader, ppClassInstances, pNumClassInstances);
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_HSSetShader_Override (
 _In_     ID3D11DeviceContext        *This,
 _In_opt_ ID3D11HullShader           *pHullShader,
 _In_opt_ ID3D11ClassInstance *const *ppClassInstances,
          UINT                        NumClassInstances ) 
{
  return
    SK_D3D11_SetShader_Impl ( This, pHullShader,
                                sk_shader_class::Hull,
                                  ppClassInstances, NumClassInstances );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_HSGetShader_Override (
 _In_        ID3D11DeviceContext  *This,
 _Out_       ID3D11HullShader    **ppHullShader,
 _Out_opt_   ID3D11ClassInstance **ppClassInstances,
 _Inout_opt_ UINT                 *pNumClassInstances ) 
{
  return
    D3D11_HSGetShader_Original ( This, ppHullShader,
                                   ppClassInstances, pNumClassInstances );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DSSetShader_Override (
 _In_     ID3D11DeviceContext        *This,
 _In_opt_ ID3D11DomainShader         *pDomainShader,
 _In_opt_ ID3D11ClassInstance *const *ppClassInstances,
          UINT                        NumClassInstances ) 
{
  return
    SK_D3D11_SetShader_Impl ( This, pDomainShader,
                                sk_shader_class::Domain,
                                  ppClassInstances, NumClassInstances );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DSGetShader_Override (
 _In_        ID3D11DeviceContext  *This,
 _Out_       ID3D11DomainShader  **ppDomainShader,
 _Out_opt_   ID3D11ClassInstance **ppClassInstances,
 _Inout_opt_ UINT                 *pNumClassInstances ) 
{
  return D3D11_DSGetShader_Original ( This, ppDomainShader,
                                        ppClassInstances, pNumClassInstances );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_CSSetShader_Override (
 _In_     ID3D11DeviceContext        *This,
 _In_opt_ ID3D11ComputeShader        *pComputeShader,
 _In_opt_ ID3D11ClassInstance *const *ppClassInstances,
          UINT                        NumClassInstances )
{
  return
    SK_D3D11_SetShader_Impl ( This, pComputeShader,
                                sk_shader_class::Compute,
                                  ppClassInstances, NumClassInstances );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_CSGetShader_Override (
 _In_        ID3D11DeviceContext  *This,
 _Out_       ID3D11ComputeShader **ppComputeShader,
 _Out_opt_   ID3D11ClassInstance **ppClassInstances,
 _Inout_opt_ UINT                 *pNumClassInstances ) 
{
  return
    D3D11_CSGetShader_Original ( This, ppComputeShader,
                                   ppClassInstances, pNumClassInstances );
}

using D3D11_PSSetShader_pfn =
  void (STDMETHODCALLTYPE *)( ID3D11DeviceContext        *This,
                     _In_opt_ ID3D11PixelShader          *pPixelShader,
                     _In_opt_ ID3D11ClassInstance *const *ppClassInstances,
                              UINT                        NumClassInstances );

using D3D11_GSSetShader_pfn =
  void (STDMETHODCALLTYPE *)( ID3D11DeviceContext        *This,
                     _In_opt_ ID3D11GeometryShader       *pGeometryShader,
                     _In_opt_ ID3D11ClassInstance *const *ppClassInstances,
                              UINT                        NumClassInstances );

using D3D11_HSSetShader_pfn =
  void (STDMETHODCALLTYPE *)( ID3D11DeviceContext        *This,
                     _In_opt_ ID3D11HullShader           *pHullShader,
                     _In_opt_ ID3D11ClassInstance *const *ppClassInstances,
                              UINT                        NumClassInstances );

using D3D11_DSSetShader_pfn =
  void (STDMETHODCALLTYPE *)( ID3D11DeviceContext        *This,
                     _In_opt_ ID3D11DomainShader         *pDomainShader,
                     _In_opt_ ID3D11ClassInstance *const *ppClassInstances,
                              UINT                        NumClassInstances );

using D3D11_CSSetShader_pfn =
  void (STDMETHODCALLTYPE *)( ID3D11DeviceContext        *This,
                     _In_opt_ ID3D11ComputeShader        *pComputeShader,
                     _In_opt_ ID3D11ClassInstance *const *ppClassInstances,
                              UINT                        NumClassInstances );


__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_RSSetScissorRects_Override (
        ID3D11DeviceContext *This,
        UINT                 NumRects,
  const D3D11_RECT          *pRects ) 
{
  return
    D3D11_RSSetScissorRects_Original (This, NumRects, pRects);
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_VSSetConstantBuffers_Override (
  ID3D11DeviceContext*  This,
  UINT                  StartSlot,
  UINT                  NumBuffers,
  ID3D11Buffer *const  *ppConstantBuffers ) 
{
  //dll_log.Log (L"[   DXGI   ] [!]D3D11_VSSetConstantBuffers (%lu, %lu, ...)", StartSlot, NumBuffers);
  return
    D3D11_VSSetConstantBuffers_Original (This, StartSlot, NumBuffers, ppConstantBuffers );
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_PSSetConstantBuffers_Override (
  ID3D11DeviceContext*  This,
  UINT                  StartSlot,
  UINT                  NumBuffers,
  ID3D11Buffer *const  *ppConstantBuffers ) 
{
  //dll_log.Log (L"[   DXGI   ] [!]D3D11_VSSetConstantBuffers (%lu, %lu, ...)", StartSlot, NumBuffers);
  return
    D3D11_PSSetConstantBuffers_Original (This, StartSlot, NumBuffers, ppConstantBuffers );
}

std::unordered_set <ID3D11Texture2D *> used_textures;

struct shader_stage_s {
  void nulBind (int dev_ctx_handle, int slot, ID3D11ShaderResourceView* pView)
  {
    if (skipped_bindings [dev_ctx_handle][slot] != nullptr)
    {
      skipped_bindings [dev_ctx_handle][slot]->Release ();
      skipped_bindings [dev_ctx_handle][slot] = nullptr;
    }

    skipped_bindings [dev_ctx_handle][slot] = pView;

    if (pView != nullptr)
    {
      pView->AddRef ();
    }
  };

  void Bind (int dev_ctx_handle, int slot, ID3D11ShaderResourceView* pView)
  {
    if (skipped_bindings [dev_ctx_handle][slot] != nullptr)
    {
      skipped_bindings [dev_ctx_handle][slot]->Release ();
      skipped_bindings [dev_ctx_handle][slot] = nullptr;
    }

    skipped_bindings [dev_ctx_handle][slot] = nullptr;

    // The D3D11 Runtime is holding a reference if this is non-null.
    real_bindings    [dev_ctx_handle][slot] = pView;
  };

  // We have to hold references to these things, because the D3D11 runtime would have.
  std::array <std::array <ID3D11ShaderResourceView*, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT>, SK_D3D11_MAX_DEV_CONTEXTS+1> skipped_bindings = { };
  std::array <std::array <ID3D11ShaderResourceView*, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT>, SK_D3D11_MAX_DEV_CONTEXTS+1> real_bindings    = { };
};

std::array <shader_stage_s, SK_D3D11_MAX_DEV_CONTEXTS+1>*
SK_D3D11_GetShaderStages (void)
{
  static std::array <shader_stage_s, SK_D3D11_MAX_DEV_CONTEXTS+1> _stages [6];
  return                                                          _stages;
}


struct SK_D3D11_ContextResources
{
  std::unordered_set <ID3D11Texture2D *> used_textures;
  std::unordered_set <IUnknown        *> temp_resources;

  volatile LONG                writing_ = 0;
  UINT                         ctx_id_  = 0;
};

std::array < SK_D3D11_ContextResources, SK_D3D11_MAX_DEV_CONTEXTS+1 >
                     SK_D3D11_PerCtxResources;

#define d3d11_shader_stages SK_D3D11_GetShaderStages ()


__forceinline
bool
SK_D3D11_ActivateSRVOnSlot ( UINT                      dev_ctx_handle,
                             shader_stage_s&           stage,
                             ID3D11ShaderResourceView* pSRV,
                             int                       SLOT )
{
  if (! pSRV)
  {
    stage.Bind (dev_ctx_handle, SLOT, pSRV);

    return true;
  }

  D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = { };

  pSRV->GetDesc (&srv_desc);

  CComPtr <ID3D11Resource>  pRes = nullptr;
  ID3D11Texture2D*          pTex = nullptr;

  if (srv_desc.ViewDimension == D3D_SRV_DIMENSION_TEXTURE2D)
  {
    pSRV->GetResource (&pRes);

    if (pRes.p != nullptr && SUCCEEDED (pRes->QueryInterface <ID3D11Texture2D> (&pTex)))
    {
      int spins = 0;

      while (InterlockedCompareExchange (&SK_D3D11_PerCtxResources [dev_ctx_handle].writing_, 1, 0) != 0)
      {
        if ( ++spins > 0x1000 )
        {
          SleepEx (1, FALSE);
          spins = 0;
        }
      }

      if (SK_D3D11_PerCtxResources [dev_ctx_handle].used_textures.emplace  (pTex).second)
          SK_D3D11_PerCtxResources [dev_ctx_handle].temp_resources.emplace (pTex);

      InterlockedExchange (&SK_D3D11_PerCtxResources [dev_ctx_handle].writing_, 0);

      if (tracked_texture == pTex && config.textures.d3d11.highlight_debug)
      {
        if (dwFrameTime % tracked_tex_blink_duration > tracked_tex_blink_duration / 2)
        {
          stage.nulBind (dev_ctx_handle, SLOT, pSRV);
          return false;
        }
      }
    }
  }

  stage.Bind (dev_ctx_handle, SLOT, pSRV);

  return true;
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_VSSetShaderResources_Override (
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumViews,
  _In_opt_       ID3D11ShaderResourceView* const *ppShaderResourceViews )
{
  static auto& shaders =
    SK_D3D11_Shaders;

#if 0
  // ImGui gets to pass-through without invoking the hook
  if (((! config.render.dxgi.deferred_isolation) && This->GetType () == D3D11_DEVICE_CONTEXT_DEFERRED) || SK_TLS_Bottom ()->imgui.drawing || (! SK_D3D11_EnableTracking))
    return D3D11_VSSetShaderResources_Original (This, StartSlot, NumViews, ppShaderResourceViews);

  auto&& newResourceViews = SK_TLS_Bottom ()->d3d11.newResourceViews;

  RtlZeroMemory (newResourceViews, sizeof (ID3D11ShaderResourceView*) * NumViews);

  if (ppShaderResourceViews && NumViews > 0)
  {
    std::lock_guard <SK_Thread_CriticalSection> auto_lock (cs_shader_vs);

    auto&&          views = SK_D3D11_Shaders.vertex.current.views [This];
    shader_stage_s& stage = d3d11_shader_stages [0][This];

    d3d11_shader_tracking_s& tracked =
      SK_D3D11_Shaders.vertex.tracked;

    uint32_t shader_crc32c = tracked.crc32c.load ();
    bool     active        = tracked.active.load ();

    for (UINT i = 0; i < NumViews; i++)
    {
      if (SK_D3D11_ActivateSRVOnSlot (stage, ppShaderResourceViews [i], StartSlot + i))
        newResourceViews [i] =               ppShaderResourceViews [i];
      else
        newResourceViews [i] = nullptr;

      if (shader_crc32c != 0 && active && ppShaderResourceViews [i])            {
        if (! tracked.set_of_views.count (ppShaderResourceViews [i]))           {
            tracked.set_of_views.emplace (ppShaderResourceViews [i]);
                                          ppShaderResourceViews [i]->AddRef (); }
            tracked.used_views.push_back (ppShaderResourceViews [i]);           }
                  views [StartSlot + i] = ppShaderResourceViews [i];
    }
  }

  else if (NumViews == 0 && ppShaderResourceViews == nullptr)
  {
    auto&&          views = SK_D3D11_Shaders.vertex.current.views [This];
    shader_stage_s& stage = d3d11_shader_stages [0][This];

    for (UINT i = 0; i < D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT; i++)
    {
      SK_D3D11_ActivateSRVOnSlot (stage, nullptr, i);

      newResourceViews [i] = nullptr;
      views            [i] = nullptr;
    }
  }

  D3D11_VSSetShaderResources_Original (This, StartSlot, NumViews, newResourceViews);
#else
  constexpr int VERTEX_SHADER_STAGE = 1;

  SK_TLS* pTLS = nullptr;

  // ImGui gets to pass-through without invoking the hook
  if ( (! SK_D3D11_ShouldTrackSetShaderResources (This, &pTLS)) ||
       (! SK_D3D11_ShouldTrackRenderOp           (This, &pTLS)) )
    return D3D11_VSSetShaderResources_Original (This, StartSlot, NumViews, ppShaderResourceViews);

  auto&& newResourceViews = pTLS->d3d11.newResourceViews;

  RtlZeroMemory (newResourceViews, sizeof (ID3D11ShaderResourceView*) * NumViews);

  if (ppShaderResourceViews && NumViews > 0)
  {
    const UINT dev_idx =
      SK_D3D11_GetDeviceContextHandle (This);

    auto&& views =
          shaders.vertex.current.views [dev_idx];
    auto&& stage =
      d3d11_shader_stages [VERTEX_SHADER_STAGE] [dev_idx];

    d3d11_shader_tracking_s& tracked =
      shaders.vertex.tracked;

    const uint32_t shader_crc32c = tracked.crc32c.load ();
    const bool     active        = tracked.active.get  (dev_idx);

    for (UINT i = 0; i < NumViews; i++)
    {
      if (SK_D3D11_ActivateSRVOnSlot (dev_idx, stage,
                                               ppShaderResourceViews [i], StartSlot + i))
        newResourceViews [i] =                 ppShaderResourceViews [i];
      else
        newResourceViews [i] = nullptr;
    }

    if (active && shader_crc32c != 0)
    {
      for (UINT i = 0; i < NumViews; i++)
      {
        if (ppShaderResourceViews [i])                                            {
          std::lock_guard <SK_Thread_CriticalSection> auto_lock (*cs_shader_vs);
          if (tracked.set_of_views.emplace (ppShaderResourceViews [i]).second)    {
                                            ppShaderResourceViews [i]->AddRef (); }
           tracked.used_views.emplace_back (ppShaderResourceViews [i]);           }
                    views [StartSlot + i] = ppShaderResourceViews [i];
      }
    }
  }

  D3D11_VSSetShaderResources_Original (This, StartSlot, NumViews, newResourceViews);
#endif
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_PSSetShaderResources_Override (
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumViews,
  _In_opt_       ID3D11ShaderResourceView* const *ppShaderResourceViews )
{
  static auto& shaders =
    SK_D3D11_Shaders;

  SK_TLS* pTLS = nullptr;

  constexpr int PIXEL_SHADER_STAGE = 1;

  // ImGui gets to pass-through without invoking the hook
  if ( (! SK_D3D11_ShouldTrackSetShaderResources (This, &pTLS)) ||
       (! SK_D3D11_ShouldTrackRenderOp           (This, &pTLS)) )
    return D3D11_PSSetShaderResources_Original (This, StartSlot, NumViews, ppShaderResourceViews);

  auto&& newResourceViews = pTLS->d3d11.newResourceViews;

  RtlZeroMemory (newResourceViews, sizeof (ID3D11ShaderResourceView*) * NumViews);

  if (ppShaderResourceViews && NumViews > 0)
  {
    const UINT dev_idx =
      SK_D3D11_GetDeviceContextHandle (This);

    auto&& views =
          shaders.pixel.current.views [dev_idx];
    auto&& stage =
      d3d11_shader_stages [PIXEL_SHADER_STAGE] [dev_idx];

    d3d11_shader_tracking_s& tracked =
      shaders.pixel.tracked;

    const uint32_t shader_crc32c = tracked.crc32c.load ();
    const bool     active        = tracked.active.get  (dev_idx);

  if (! ReadAcquire (&__SKX_ComputeAntiStall))
  {
    for (UINT i = 0; i < NumViews; i++)
    {
      if (SK_D3D11_ActivateSRVOnSlot ( dev_idx, stage,
                                           ppShaderResourceViews [i], StartSlot + i))
        newResourceViews [i] =             ppShaderResourceViews [i];
      else
        newResourceViews [i] = nullptr;
    }

    if (active && shader_crc32c != 0)
    {
      for (UINT i = 0; i < NumViews; i++)
      {
        if (ppShaderResourceViews [i])                                            {
          std::lock_guard <SK_Thread_CriticalSection> auto_lock (*cs_shader_ps);
          if (tracked.set_of_views.emplace (ppShaderResourceViews [i]).second)    {
                                            ppShaderResourceViews [i]->AddRef (); }
           tracked.used_views.emplace_back (ppShaderResourceViews [i]);           }
                    views [StartSlot + i] = ppShaderResourceViews [i];
      }
    }
  } else {
    for ( UINT i = StartSlot; i < StartSlot + NumViews; ++i )
    {
      if (SK_D3D11_ActivateSRVOnSlot (dev_idx, stage,
                                           ppShaderResourceViews [i - StartSlot], i))
        newResourceViews [i - StartSlot] = ppShaderResourceViews [i - StartSlot];
      else
        newResourceViews [i - StartSlot] = nullptr;
    }

    if (active && shader_crc32c != 0)
    {
      for ( UINT i = StartSlot; i < StartSlot + NumViews; ++i )
      {
        if (ppShaderResourceViews [i - StartSlot])                                            {
          std::lock_guard <SK_Thread_CriticalSection> auto_lock (*cs_shader_ps);
          if (tracked.set_of_views.emplace (ppShaderResourceViews [i - StartSlot]).second)    {
                                            ppShaderResourceViews [i - StartSlot]->AddRef (); }
           tracked.used_views.emplace_back (ppShaderResourceViews [i - StartSlot]);           }
                                views [i] = ppShaderResourceViews [i - StartSlot];
      }
    }
  }
  }

  D3D11_PSSetShaderResources_Original (This, StartSlot, NumViews, newResourceViews);
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_GSSetShaderResources_Override (
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumViews,
  _In_opt_       ID3D11ShaderResourceView* const *ppShaderResourceViews )
{
  static auto& shaders =
    SK_D3D11_Shaders;

  constexpr int GEOMETRY_SHADER_STAGE = 2;

  SK_TLS *pTLS = nullptr;

  // ImGui gets to pass-through without invoking the hook
  if ( (! SK_D3D11_ShouldTrackSetShaderResources (This, &pTLS)) ||
       (! SK_D3D11_ShouldTrackRenderOp           (This, &pTLS)) )
    return D3D11_GSSetShaderResources_Original (This, StartSlot, NumViews, ppShaderResourceViews);

  auto&& newResourceViews = pTLS->d3d11.newResourceViews;

  RtlZeroMemory (newResourceViews, sizeof (ID3D11ShaderResourceView*) * NumViews);

  if (ppShaderResourceViews && NumViews > 0)
  {
    const UINT dev_idx =
      SK_D3D11_GetDeviceContextHandle (This);

    auto&& views =
          shaders.geometry.current.views [dev_idx];
    auto&& stage =
      d3d11_shader_stages [GEOMETRY_SHADER_STAGE] [dev_idx];

    d3d11_shader_tracking_s& tracked =
      shaders.geometry.tracked;

    const uint32_t shader_crc32c = tracked.crc32c.load ();
    const bool     active        = tracked.active.get  (dev_idx);

    for (UINT i = 0; i < NumViews; i++)
    {
      if (SK_D3D11_ActivateSRVOnSlot (dev_idx, stage,
                                          ppShaderResourceViews [i], StartSlot + i))
        newResourceViews [i] =            ppShaderResourceViews [i];
      else
        newResourceViews [i] = nullptr;
    }

    if (active && shader_crc32c != 0)
    {
      for (UINT i = 0; i < NumViews; i++)
      {
        if (ppShaderResourceViews [i])                                            {
          std::lock_guard <SK_Thread_CriticalSection> auto_lock (*cs_shader_gs);
          if (tracked.set_of_views.emplace (ppShaderResourceViews [i]).second)    {
                                            ppShaderResourceViews [i]->AddRef (); }
           tracked.used_views.emplace_back (ppShaderResourceViews [i]);           }
                    views [StartSlot + i] = ppShaderResourceViews [i];
      }
    }
  }

  D3D11_GSSetShaderResources_Original (This, StartSlot, NumViews, newResourceViews);
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_HSSetShaderResources_Override (
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumViews,
  _In_opt_       ID3D11ShaderResourceView* const *ppShaderResourceViews )
{

  static auto& shaders =
    SK_D3D11_Shaders;

 constexpr int HULL_SHADER_STAGE = 3;

  SK_TLS *pTLS = nullptr;

  // ImGui gets to pass-through without invoking the hook
  if ( (! SK_D3D11_ShouldTrackSetShaderResources (This, &pTLS)) ||
       (! SK_D3D11_ShouldTrackRenderOp           (This, &pTLS)) )
    return D3D11_HSSetShaderResources_Original (This, StartSlot, NumViews, ppShaderResourceViews);

  auto&& newResourceViews = pTLS->d3d11.newResourceViews;

  RtlZeroMemory (newResourceViews, sizeof (ID3D11ShaderResourceView*) * NumViews);

  if (ppShaderResourceViews && NumViews > 0)
  {
    UINT dev_idx =
      SK_D3D11_GetDeviceContextHandle (This);

    auto&& views =
          shaders.hull.current.views[dev_idx];
    auto&& stage =
      d3d11_shader_stages [HULL_SHADER_STAGE][dev_idx];

    d3d11_shader_tracking_s& tracked =
      shaders.hull.tracked;

    uint32_t shader_crc32c = tracked.crc32c.load ();
    bool     active        = tracked.active.get  (dev_idx);

    for (UINT i = 0; i < NumViews; i++)
    {
      if (SK_D3D11_ActivateSRVOnSlot (dev_idx, stage,
                                          ppShaderResourceViews [i], StartSlot + i))
        newResourceViews [i] =            ppShaderResourceViews [i];
      else
        newResourceViews [i] = nullptr;
    }

    if (active && shader_crc32c != 0)
    {
      for (UINT i = 0; i < NumViews; i++)
      {
        if (ppShaderResourceViews [i])                                            {
          std::lock_guard <SK_Thread_CriticalSection> auto_lock (*cs_shader_hs);
          if (tracked.set_of_views.emplace (ppShaderResourceViews [i]).second)    {
                                            ppShaderResourceViews [i]->AddRef (); }
           tracked.used_views.emplace_back (ppShaderResourceViews [i]);           }
                    views [StartSlot + i] = ppShaderResourceViews [i];
      }
    }
  }

  D3D11_HSSetShaderResources_Original (This, StartSlot, NumViews, newResourceViews);
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DSSetShaderResources_Override (
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumViews,
  _In_opt_       ID3D11ShaderResourceView* const *ppShaderResourceViews )
{
  static auto& shaders =
    SK_D3D11_Shaders;

  constexpr int DOMAIN_SHADER_STAGE = 4;

  SK_TLS *pTLS = nullptr;

  // ImGui gets to pass-through without invoking the hook
  if ( (! SK_D3D11_ShouldTrackSetShaderResources (This, &pTLS)) ||
       (! SK_D3D11_ShouldTrackRenderOp           (This, &pTLS)) )
    return D3D11_DSSetShaderResources_Original (This, StartSlot, NumViews, ppShaderResourceViews);

  auto&& newResourceViews = pTLS->d3d11.newResourceViews;

  RtlZeroMemory (newResourceViews, sizeof (ID3D11ShaderResourceView*) * NumViews);

  if (ppShaderResourceViews && NumViews > 0)
  {
    UINT dev_idx =
      SK_D3D11_GetDeviceContextHandle (This);

    auto&& views =
          shaders.domain.current.views[dev_idx];
    auto&& stage =
      d3d11_shader_stages [DOMAIN_SHADER_STAGE][dev_idx];

    d3d11_shader_tracking_s& tracked =
      shaders.domain.tracked;

    uint32_t shader_crc32c = tracked.crc32c.load ();
    bool     active        = tracked.active.get  (dev_idx);

    for (UINT i = 0; i < NumViews; i++)
    {
      if (SK_D3D11_ActivateSRVOnSlot (dev_idx, stage,
                                          ppShaderResourceViews [i], StartSlot + i))
        newResourceViews [i] =            ppShaderResourceViews [i];
      else
        newResourceViews [i] = nullptr;
    }

    if (active && shader_crc32c != 0)
    {
      for (UINT i = 0; i < NumViews; i++)
      {
        if (ppShaderResourceViews [i])                                            {
          std::lock_guard <SK_Thread_CriticalSection> auto_lock (*cs_shader_ds);
          if (tracked.set_of_views.emplace (ppShaderResourceViews [i]).second)    {
                                            ppShaderResourceViews [i]->AddRef (); }
           tracked.used_views.emplace_back (ppShaderResourceViews [i]);           }
                    views [StartSlot + i] = ppShaderResourceViews [i];
      }
    }
  }

  D3D11_DSSetShaderResources_Original (This, StartSlot, NumViews, newResourceViews);
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_CSSetShaderResources_Override (
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumViews,
  _In_opt_       ID3D11ShaderResourceView* const *ppShaderResourceViews )
{
  static auto& shaders =
    SK_D3D11_Shaders;

  constexpr int COMPUTE_SHADER_STAGE = 5;

  SK_TLS *pTLS = nullptr;

  // ImGui gets to pass-through without invoking the hook
  if ( (! SK_D3D11_ShouldTrackSetShaderResources (This, &pTLS)) ||
       (! SK_D3D11_ShouldTrackRenderOp           (This, &pTLS)) )
    return D3D11_CSSetShaderResources_Original (This, StartSlot, NumViews, ppShaderResourceViews);

  auto&& newResourceViews = pTLS->d3d11.newResourceViews;

  RtlZeroMemory (newResourceViews, sizeof (ID3D11ShaderResourceView*) * NumViews);

  if (ppShaderResourceViews && NumViews > 0)
  {
    const UINT dev_idx =
      SK_D3D11_GetDeviceContextHandle (This);

    auto&& views =
          shaders.compute.current.views[dev_idx];
    auto&& stage =
      d3d11_shader_stages [COMPUTE_SHADER_STAGE][dev_idx];

    d3d11_shader_tracking_s& tracked =
      shaders.compute.tracked;

    const uint32_t shader_crc32c = tracked.crc32c.load ();
    const bool     active        = tracked.active.get  (dev_idx);

    for (UINT i = 0; i < NumViews; i++)
    {
      if (SK_D3D11_ActivateSRVOnSlot (dev_idx, stage,
                                          ppShaderResourceViews [i], StartSlot + i))
        newResourceViews [i] =            ppShaderResourceViews [i];
      else
        newResourceViews [i] = nullptr;
    }

    if (active && shader_crc32c != 0)
    {
      for (UINT i = 0; i < NumViews; i++)
      {
        if (ppShaderResourceViews [i])                                            {
          std::lock_guard <SK_Thread_CriticalSection> auto_lock (*cs_shader_cs);
          if (tracked.set_of_views.emplace (ppShaderResourceViews [i]).second)    {
                                            ppShaderResourceViews [i]->AddRef (); }
           tracked.used_views.emplace_back (ppShaderResourceViews [i]);           }
                    views [StartSlot + i] = ppShaderResourceViews [i];
      }
    }
  }

  D3D11_CSSetShaderResources_Original (This, StartSlot, NumViews, newResourceViews);
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_CSSetUnorderedAccessViews_Override (
  _In_           ID3D11DeviceContext             *This,
  _In_           UINT                             StartSlot,
  _In_           UINT                             NumUAVs,
  _In_opt_       ID3D11UnorderedAccessView *const *ppUnorderedAccessViews,
  _In_opt_ const UINT                             *pUAVInitialCounts ) 
{
  return
    D3D11_CSSetUnorderedAccessViews_Original (This, StartSlot, NumUAVs, ppUnorderedAccessViews, pUAVInitialCounts);
}



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

  explicit SK_D3D11_TEXTURE2D_DESC (D3D11_TEXTURE2D_DESC& descFrom) 
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

bool
SK_D3D11_IsStagingCacheable ( D3D11_RESOURCE_DIMENSION  rdim,
                              ID3D11Resource           *pRes,
                              SK_TLS                   *pTLS = nullptr
 )
{
  if ( config.textures.cache.allow_staging && pRes != nullptr &&
                                              rdim == D3D11_RESOURCE_DIMENSION_TEXTURE2D )
  {
    CComQIPtr <ID3D11Texture2D> pTex (pRes);

    if (pTex)
    {
      D3D11_TEXTURE2D_DESC tex_desc = { };
           pTex->GetDesc (&tex_desc);

      const SK_D3D11_TEXTURE2D_DESC desc (tex_desc);

      if (desc.Usage != D3D11_USAGE_STAGING || (desc.CPUAccessFlags & D3D11_CPU_ACCESS_WRITE))
      {
        if (pTLS == nullptr)
            pTLS = SK_TLS_Bottom ();

        if (! (pTLS->imgui.drawing || pTLS->texture_management.injection_thread))
          return true;
      }
    }
  }

  return false;
}

// Temporary staging for memory-mapped texture uploads
//
struct mapped_resources_s {
  std::unordered_map <ID3D11Resource*,  D3D11_MAPPED_SUBRESOURCE> textures;
  std::unordered_map <ID3D11Resource*,  uint64_t>                 texture_times;

  std::unordered_map <ID3D11Resource*,  uint32_t>                 dynamic_textures;
  std::unordered_map <ID3D11Resource*,  uint32_t>                 dynamic_texturesx;
  std::map           <uint32_t,         uint64_t>                 dynamic_times2;
  std::map           <uint32_t,         size_t>                   dynamic_sizes2;
};

std::unordered_map <ID3D11DeviceContext *, mapped_resources_s> mapped_resources;


__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_UpdateSubresource1_Override (
  _In_           ID3D11DeviceContext1 *This,
  _In_           ID3D11Resource       *pDstResource,
  _In_           UINT                  DstSubresource,
  _In_opt_ const D3D11_BOX            *pDstBox,
  _In_     const void                 *pSrcData,
  _In_           UINT                  SrcRowPitch,
  _In_           UINT                  SrcDepthPitch,
  _In_           UINT                  CopyFlags)
{
  bool early_out = false;

  SK_TLS *pTLS = nullptr;

  // UB: If it's happening, pretend we never saw this...
  if ( pDstResource == nullptr ||
       pSrcData     == nullptr    )
  {
    early_out = true;
  }

  else
  {             
        early_out = (! SK_D3D11_ShouldTrackRenderOp (This, &pTLS));
    if (early_out)
    {
      early_out = (! SK_D3D11_ShouldTrackMMIO (This, &pTLS));
    }
  }


  if (early_out)
  {
    return
      D3D11_UpdateSubresource1_Original ( This, pDstResource, DstSubresource,
                                            pDstBox, pSrcData, SrcRowPitch,
                                              SrcDepthPitch, CopyFlags );
  }

  dll_log.Log (L"UpdateSubresource1 ({%s}", DescribeResource (pDstResource).c_str ());


  //dll_log.Log (L"[   DXGI   ] [!]D3D11_UpdateSubresource1 (%ph, %lu, %ph, %ph, %lu, %lu, %x)",
  //          pDstResource, DstSubresource, pDstBox, pSrcData, SrcRowPitch, SrcDepthPitch, CopyFlags);

  D3D11_RESOURCE_DIMENSION rdim = D3D11_RESOURCE_DIMENSION_UNKNOWN;
  pDstResource->GetType  (&rdim);

  if (SK_D3D11_IsStagingCacheable (rdim, pDstResource) && DstSubresource == 0)
  {
    static auto& textures =
      SK_D3D11_Textures;

    CComQIPtr <ID3D11Texture2D> pTex (pDstResource);

    if (pTex != nullptr)
    {
      D3D11_TEXTURE2D_DESC desc = { };
           pTex->GetDesc (&desc);

      D3D11_SUBRESOURCE_DATA srd = { };

      srd.pSysMem           = pSrcData;
      srd.SysMemPitch       = SrcRowPitch;
      srd.SysMemSlicePitch  = 0;

      size_t   size         = 0;
      uint32_t top_crc32c   = 0x0;

      uint32_t checksum     =
        crc32_tex   (&desc, &srd, &size, &top_crc32c, false);

      const uint32_t cache_tag    =
        safe_crc32c (top_crc32c, (uint8_t *)(&desc), sizeof D3D11_TEXTURE2D_DESC);

      const auto start            = SK_QueryPerf ().QuadPart;

      CComPtr <ID3D11Texture2D> pCachedTex =
        textures.getTexture2D (cache_tag, &desc, nullptr, nullptr, pTLS);

      if (pCachedTex != nullptr)
      {
        CComQIPtr <ID3D11Resource> pCachedResource (pCachedTex);

        D3D11_CopyResource_Original (This, pDstResource, pCachedResource);

        SK_LOG1 ( ( L"Texture Cache Hit (Slow Path): (%lux%lu) -- %x",
                      desc.Width, desc.Height, top_crc32c ),
                    L"DX11TexMgr" );

        return;
      }

      else
      {
        if (SK_D3D11_TextureIsCached (pTex))
        {
          SK_LOG0 ( (L"Cached texture was updated (UpdateSubresource)... removing from cache! - <%s>",
                         SK_GetCallerName ().c_str ()), L"DX11TexMgr" );
          SK_D3D11_RemoveTexFromCache (pTex, true);
        }

        D3D11_UpdateSubresource_Original ( This, pDstResource, DstSubresource,
                                             pDstBox, pSrcData, SrcRowPitch,
                                               SrcDepthPitch );
        const auto end     = SK_QueryPerf ().QuadPart;
              auto elapsed = end - start;

        if (desc.Usage == D3D11_USAGE_STAGING)
        {
          auto&& map_ctx = mapped_resources [This];

          map_ctx.dynamic_textures  [pDstResource] = checksum;
          map_ctx.dynamic_texturesx [pDstResource] = top_crc32c;

          SK_LOG1 ( ( L"New Staged Texture: (%lux%lu) -- %x",
                        desc.Width, desc.Height, top_crc32c ),
                      L"DX11TexMgr" );

          map_ctx.dynamic_times2    [checksum]  = elapsed;
          map_ctx.dynamic_sizes2    [checksum]  = size;

          return;
        }

        else
        {
          bool cacheable = ( desc.MiscFlags <= 4 &&
                             desc.Width      > 0 && 
                             desc.Height     > 0 &&
                             desc.ArraySize == 1 //||
                           //((desc.ArraySize  % 6 == 0) && (desc.MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE))
                           );

          bool compressed = false;

          if ( (desc.Format >= DXGI_FORMAT_BC1_TYPELESS  &&
                desc.Format <= DXGI_FORMAT_BC5_SNORM)    ||
               (desc.Format >= DXGI_FORMAT_BC6H_TYPELESS &&
                desc.Format <= DXGI_FORMAT_BC7_UNORM_SRGB) )
          {
            compressed = true;
          }

          // If this isn't an injectable texture, then filter out non-mipmapped
          //   textures.
          if (/*(! injectable) && */cache_opts.ignore_non_mipped)
            cacheable &= (desc.MipLevels > 1 || compressed);

          if (cacheable)
          {
            SK_LOG1 ( ( L"New Cacheable Texture: (%lux%lu) -- %x",
                          desc.Width, desc.Height, top_crc32c ),
                        L"DX11TexMgr" );

            textures.CacheMisses_2D++;
            textures.refTexture2D ( pTex, &desc, cache_tag, size, elapsed, top_crc32c,
                                      L"", nullptr, (HMODULE)(intptr_t)-1/*SK_GetCallingDLL ()*/, pTLS );

            return;
          }
        }
      }
    }
  }

  return D3D11_UpdateSubresource1_Original ( This, pDstResource, DstSubresource,
                                               pDstBox, pSrcData, SrcRowPitch,
                                                 SrcDepthPitch, CopyFlags );
}


#include <SpecialK/utility.h>

bool         SK_D3D11_need_tex_reset      = false;
bool         SK_D3D11_try_tex_reset       = false; // Don't need, but would be beneficial to try
int32_t      SK_D3D11_amount_to_purge     = 0L;

bool         SK_D3D11_dump_textures       = false;// true;
bool         SK_D3D11_inject_textures_ffx = false;
bool         SK_D3D11_inject_textures     = false;
bool         SK_D3D11_cache_textures      = false;
bool         SK_D3D11_mark_textures       = false;
std::wstring SK_D3D11_res_root            = L"SK_Res";

std::wstring
SK_D3D11_TexNameFromChecksum (uint32_t top_crc32, uint32_t checksum, uint32_t ffx_crc32 = 0x00);

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_UpdateSubresource_Override (
  _In_           ID3D11DeviceContext* This,
  _In_           ID3D11Resource      *pDstResource,
  _In_           UINT                 DstSubresource,
  _In_opt_ const D3D11_BOX           *pDstBox,
  _In_     const void                *pSrcData,
  _In_           UINT                 SrcRowPitch,
  _In_           UINT                 SrcDepthPitch)
{
  bool early_out =
    ( This->GetType () == D3D11_DEVICE_CONTEXT_DEFERRED );

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

  if ( pDstBox != nullptr && ( pDstBox->left  >= pDstBox->right  ||
                               pDstBox->top   >= pDstBox->bottom ||
                               pDstBox->front >= pDstBox->back )    )
  {
    early_out = true;
  }

  static const bool __sk_dqxi =
    ( SK_GetCurrentGameID () == SK_GAME_ID::DragonQuestXI );

  ////else
  ////early_out |= (! SK_D3D11_ShouldTrackRenderOp (This, &pTLS));
  pTLS = SK_TLS_Bottom ();

  if (pTLS->imgui.drawing || pTLS->texture_management.injection_thread)
    early_out = true;


  if (early_out)
  {
    return
      D3D11_UpdateSubresource_Original ( This, pDstResource, DstSubresource,
                                           pDstBox, pSrcData, SrcRowPitch,
                                             SrcDepthPitch );
  }

  //if (SK_GetCurrentGameID() == SK_GAME_ID::Ys_Eight)
  //{
  //  dll_log.Log ( L"UpdateSubresource:  { [%p] <RowPitch: %5lu> } -> { %s [%lu] }",
  //                pSrcData, SrcRowPitch, DescribeResource (pDstResource).c_str  (), DstSubresource );
  //}

  //dll_log.Log (L"[   DXGI   ] [!]D3D11_UpdateSubresource (%ph, %lu, %ph, %ph, %lu, %lu)",
  //          pDstResource, DstSubresource, pDstBox, pSrcData, SrcRowPitch, SrcDepthPitch);

  D3D11_RESOURCE_DIMENSION rdim = D3D11_RESOURCE_DIMENSION_UNKNOWN;
  pDstResource->GetType  (&rdim);

  if ( ((__sk_dqxi && rdim == D3D11_RESOURCE_DIMENSION_TEXTURE2D) ||
           SK_D3D11_IsStagingCacheable (rdim, pDstResource)) && DstSubresource == 0 )
  {
    static auto& textures =
      SK_D3D11_Textures;

    CComQIPtr <ID3D11Texture2D> pTex (pDstResource);

    if (pTex != nullptr)
    {
      D3D11_TEXTURE2D_DESC desc = { };
           pTex->GetDesc (&desc);

      /// DQ XI Temp Hack
      /// ---------------
      if (__sk_dqxi)
      {
        const bool skip =
          ( desc.Usage == D3D11_USAGE_STAGING ||
            desc.Usage == D3D11_USAGE_DYNAMIC ||
            (! SK_D3D11_IsFormatCompressed (desc.Format)) );

        if (skip)
        {
          return
            D3D11_UpdateSubresource_Original ( This, pDstResource, DstSubresource,
                                                 pDstBox, pSrcData, SrcRowPitch,
                                                   SrcDepthPitch );
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

          stripped_desc.Format         = DirectX::MakeTypeless (desc.Format);
          stripped_desc.BindFlags      =              0xdeadc0de;
          stripped_desc.Usage          = (D3D11_USAGE)0xdeadc0de;
          stripped_desc.CPUAccessFlags =              0xdeadc0de;
          stripped_desc.MiscFlags      =              0xdeadc0de;

          return stripped_desc;
        };

      auto transient_desc =
        _StripTransientProperties (desc);

const uint32_t cache_tag    =
        safe_crc32c (top_crc32c, (uint8_t *)(&transient_desc), sizeof D3D11_TEXTURE2D_DESC);

      const auto start      = SK_QueryPerf ().QuadPart;

      CComPtr <ID3D11Texture2D> pCachedTex =
        textures.getTexture2D (cache_tag, &desc);

      if (pCachedTex != nullptr)
      {
        CComQIPtr <ID3D11Resource> pCachedResource (pCachedTex);

        if (! pCachedTex.IsEqualObject (pTex))
        {
          D3D11_CopyResource_Original (This, pDstResource, pCachedResource);

          SK_LOG1 ( ( L"Texture Cache Hit (Slow Path): (%lux%lu) -- %x",
                        desc.Width, desc.Height, top_crc32c ),
                      L"DX11TexMgr" );
        }

        else
        {
          SK_LOG0 ( ( L"Texture Cache Redundancy: (%lux%lu) -- %x",
                          desc.Width, desc.Height, top_crc32c ),
                        L"DX11TexMgr" );
        }

        textures.recordCacheHit (pCachedTex);

        return;
      }

      else
      {
        ///if (SK_D3D11_TextureIsCached (pTex))
        ///{
        ///  SK_LOG0 ( (L"Cached texture was updated (UpdateSubresource)... removing from cache! - <%s>",
        ///                 SK_GetCallerName ().c_str ()), L"DX11TexMgr" );
        ///  SK_D3D11_RemoveTexFromCache (pTex, true);
        ///}

        D3D11_UpdateSubresource_Original ( This, pDstResource, DstSubresource,
                                             pDstBox, pSrcData, SrcRowPitch,
                                               SrcDepthPitch );

        const auto end     = SK_QueryPerf ().QuadPart;
              auto elapsed = end - start;

        if (desc.Usage == D3D11_USAGE_STAGING)
        {
          auto&& map_ctx = mapped_resources [This];

          map_ctx.dynamic_textures  [pDstResource] = checksum;
          map_ctx.dynamic_texturesx [pDstResource] = top_crc32c;

          SK_LOG1 ( ( L"New Staged Texture: (%lux%lu) -- %x",
                        desc.Width, desc.Height, top_crc32c ),
                      L"DX11TexMgr" );

          map_ctx.dynamic_times2    [checksum]  = elapsed;
          map_ctx.dynamic_sizes2    [checksum]  = size;

          return;
        }

        else if (desc.Usage == D3D11_USAGE_DEFAULT)
        {
        //-------------------

          bool cacheable = ( desc.MiscFlags <= 4 &&
                             desc.Width      > 0 && 
                             desc.Height     > 0 &&
                             desc.ArraySize == 1 //||
                           //((desc.ArraySize  % 6 == 0) && (desc.MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE))
                           );

          bool compressed =
            SK_D3D11_IsFormatCompressed (desc.Format);

          // If this isn't an injectable texture, then filter out non-mipmapped
          //   textures.
          if (/*(! injectable) && */cache_opts.ignore_non_mipped)
            cacheable &= (desc.MipLevels > 1 || compressed);

          if (cacheable)
          {
            bool injected = false;

            // -----------------------------
            if (SK_D3D11_res_root.length ())
            {
              wchar_t   wszTex [MAX_PATH * 2] = { };
              wcsncpy ( wszTex,
                        SK_D3D11_TexNameFromChecksum (top_crc32c, checksum, 0x0).c_str (),
                        MAX_PATH );

              if (                  *wszTex  != L'\0' &&
                  GetFileAttributes (wszTex) != INVALID_FILE_ATTRIBUTES )
              {
#define D3DX11_DEFAULT static_cast <UINT> (-1)

                if (pTLS == nullptr)
                  pTLS = SK_TLS_Bottom ();

                SK_ScopedBool auto_tex_inject (&pTLS->texture_management.injection_thread);
                SK_ScopedBool auto_draw       (&pTLS->imgui.drawing);

                pTLS->texture_management.injection_thread = true;
                pTLS->imgui.drawing                       = true;

                HRESULT hr = E_UNEXPECTED;

                DirectX::TexMetadata mdata;

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
                      CComPtr <ID3D11Texture2D> pSurrogateTexture2D = nullptr;
                      CComPtr <ID3D11Device>    pDevCtx             = nullptr;

                      This->GetDevice (&pDevCtx.p);

                      if (SUCCEEDED ((hr = DirectX::CreateTexture (pDevCtx,
                                             img.GetImages     (),
                                             img.GetImageCount (), mdata, 
                                             reinterpret_cast <ID3D11Resource **> (&pSurrogateTexture2D.p))))
                         )
                      {
                        const LARGE_INTEGER load_end =
                          SK_QueryPerf ();

                        D3D11_CopyResource_Original (
                          This, pDstResource,
                                pSurrogateTexture2D
                        );

                        pTex  =  nullptr;
                        pTex.Attach (
                          reinterpret_cast <ID3D11Texture2D *> (pDstResource)
                        );

                        injected = true;
                      }

                      else
                      {
                        SK_LOG0 ( (L"*** Texture '%s' failed DirectX::CreateTexture (...) -- (HRESULT=%s), skipping!", 
                                 SK_StripUserNameFromPathW (wszTex), _com_error (hr).ErrorMessage () ),
                                 L"DX11TexMgr" );
                      }
                    }
                    else
                    {
                      SK_LOG0 ( (L"*** Texture '%s' failed DirectX::LoadFromDDSFile (...) -- (HRESULT=%s), skipping!", 
                               SK_StripUserNameFromPathW (wszTex), _com_error (hr).ErrorMessage () ),
                               L"DX11TexMgr" );
                    }
                  }
                }

                else
                {
                  SK_LOG0 ( (L"*** Texture '%s' failed DirectX::GetMetadataFromDDSFile (...) -- (HRESULT=%s), skipping!", 
                           SK_StripUserNameFromPathW (wszTex), _com_error (hr).ErrorMessage () ),
                           L"DX11TexMgr" );
                }
              }
            }

            SK_LOG1 ( ( L"New Cacheable Texture: (%lux%lu) -- %x",
                          desc.Width, desc.Height, top_crc32c ),
                        L"DX11TexMgr" );

            textures.CacheMisses_2D++;
            textures.refTexture2D (pTex, &desc, cache_tag, size, elapsed, top_crc32c,
                                     L"", nullptr, (HMODULE)(intptr_t)-1/*SK_GetCallingDLL ()*/, pTLS);

            if (injected)
              textures.Textures_2D [pTex].injected = true;

            return;
          }
        }
      }
    }
  }

  return
    D3D11_UpdateSubresource_Original ( This, pDstResource, DstSubresource,
                                         pDstBox, pSrcData, SrcRowPitch,
                                           SrcDepthPitch );
}

const GUID SKID_D3D11Texture2D_DISCARD =
// {5C5298CA-0F9C-4931-A19D-A2E69792AE02}
  { 0x5c5298ca, 0xf9c,  0x4931, { 0xa1, 0x9d, 0xa2, 0xe6, 0x97, 0x92, 0xae, 0x2 } };

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11Dev_Map_Override (
   _In_ ID3D11DeviceContext      *This,
   _In_ ID3D11Resource           *pResource,
   _In_ UINT                      Subresource,
   _In_ D3D11_MAP                 MapType,
   _In_ UINT                      MapFlags,
_Out_opt_ D3D11_MAPPED_SUBRESOURCE *pMappedResource )
{
  const HRESULT hr =
    D3D11_Map_Original ( This, pResource, Subresource,
                           MapType, MapFlags, pMappedResource );

  // UB: If it's happening, pretend we never saw this...
  if (pResource == nullptr)
  {
    assert (pResource != nullptr);

    return hr;
  }

  D3D11_RESOURCE_DIMENSION rdim = D3D11_RESOURCE_DIMENSION_UNKNOWN;
      pResource->GetType (&rdim);

  bool track =
    ( SK_D3D11_ShouldTrackMMIO (This) && SK_D3D11_ShouldTrackRenderOp (This) );

  // ImGui gets to pass-through without invoking the hook
  if ((! config.textures.cache.allow_staging) && (! track))
  {
    return hr;
  }

  if (SUCCEEDED (hr))
  {
    SK_D3D11_MemoryThreads.mark ();

    if (SK_D3D11_IsStagingCacheable (rdim, pResource))
    {
      // Reference will be released, but we expect the game's going to unmap this at some point anyway...
      CComQIPtr <ID3D11Texture2D> pTex = pResource;

      if (pTex != nullptr)
      {
                        D3D11_TEXTURE2D_DESC    d3d11_desc = { };
        pTex->GetDesc ((D3D11_TEXTURE2D_DESC *)&d3d11_desc);

        const SK_D3D11_TEXTURE2D_DESC desc (d3d11_desc);

        //dll_log.Log ( L"Staging Map: Type=%x, BindFlags: %s, Res: %lux%lu",
        //                MapType, SK_D3D11_DescribeBindFlags (desc.BindFlags).c_str (),
        //       desc.Width, desc.Height, SK_DXGI_FormatToStr (desc.Format).c_str    () );

        if (MapType == D3D11_MAP_WRITE_DISCARD)
        {
          static auto& textures =
            SK_D3D11_Textures;

          auto&& it = textures.Textures_2D.find (pTex);
          if (  it != textures.Textures_2D.end  (    ) && it->second.crc32c != 0x00 )
          {
                                                                               it->second.discard = true;
            pTex->SetPrivateData (SKID_D3D11Texture2D_DISCARD, sizeof (bool), &it->second.discard);

            SK_D3D11_RemoveTexFromCache (pTex, true);
            textures.HashMap_2D [it->second.orig_desc.MipLevels].erase (it->second.tag);

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
                SK_D3D11_Textures.Textures_2D [pTex];
                                                                                 texDesc.discard = true;
              pTex->SetPrivateData (SKID_D3D11Texture2D_DISCARD, sizeof (bool), &texDesc.discard);

              SK_LOG1 ( ( L"Removing discarded texture from cache (it has been memory-mapped multiple times)." ),
                          L"DX11TexMgr" );
              SK_D3D11_RemoveTexFromCache (pTex, true);
            }

            else if (pMappedResource != nullptr)
            {
              auto&& map_ctx = mapped_resources [This];

              map_ctx.textures.emplace      (std::make_pair (pResource, *pMappedResource));
              map_ctx.texture_times.emplace (std::make_pair (pResource, SK_QueryPerf ().QuadPart));

              //dll_log.Log (L"[DX11TexMgr] Mapped 2D texture...");
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
            //dll_log.Log (L"[DX11TexMgr] Skipped 2D texture...");
          }
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

    mem_map_stats.last_frame.map_types [MapType-1]++;

    switch (rdim)
    {
      case D3D11_RESOURCE_DIMENSION_UNKNOWN:
        mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_UNKNOWN]++;
        break;
      case D3D11_RESOURCE_DIMENSION_BUFFER:
      {
        mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_BUFFER]++;

        ID3D11Buffer* pBuffer = nullptr;

        if (SUCCEEDED (pResource->QueryInterface <ID3D11Buffer> (&pBuffer)))
        {
          D3D11_BUFFER_DESC  buf_desc = { };
          pBuffer->GetDesc (&buf_desc);
          {
            ///std::lock_guard <SK_Thread_CriticalSection> auto_lock (cs_mmio);

            // Extra memory allocation pressure for no good reason -- kill it.
            //  
            ////if (buf_desc.BindFlags & D3D11_BIND_INDEX_BUFFER)
            ////  mem_map_stats.last_frame.index_buffers.insert (pBuffer);
            ////
            ////if (buf_desc.BindFlags & D3D11_BIND_VERTEX_BUFFER)
            ////  mem_map_stats.last_frame.vertex_buffers.insert (pBuffer);
            ////
            ////if (buf_desc.BindFlags & D3D11_BIND_CONSTANT_BUFFER)
            ////  mem_map_stats.last_frame.constant_buffers.insert (pBuffer);
          }

          if (read)
            mem_map_stats.last_frame.bytes_read    += buf_desc.ByteWidth;

          if (write)
            mem_map_stats.last_frame.bytes_written += buf_desc.ByteWidth;

          pBuffer->Release ();
        }
      } break;
      case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
        mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_TEXTURE1D]++;
        break;
      case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
        mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_TEXTURE2D]++;
        break;
      case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
        mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_TEXTURE3D]++;
        break;
    }
  }

  return hr;
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_Unmap_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ ID3D11Resource      *pResource,
  _In_ UINT                 Subresource )
{ 
  // UB: If it's happening, pretend we never saw this...
  if (pResource == nullptr)
  {
    assert (pResource != nullptr);

    return
      D3D11_Unmap_Original (This, pResource, Subresource);
  }

  SK_TLS *pTLS = nullptr;

  bool track =
    ( SK_D3D11_ShouldTrackMMIO (This) && SK_D3D11_ShouldTrackRenderOp (This, &pTLS) );

  // ImGui gets to pass-through without invoking the hook
  if ((! config.textures.cache.allow_staging) && (! track))
  {
    D3D11_Unmap_Original (This, pResource, Subresource);
    return;
  }

  pTLS = ( pTLS != nullptr       ?
           pTLS : SK_TLS_Bottom ( ) );

  SK_D3D11_MemoryThreads.mark ();

  D3D11_RESOURCE_DIMENSION rdim = D3D11_RESOURCE_DIMENSION_UNKNOWN;
      pResource->GetType (&rdim);

  if (SK_D3D11_IsStagingCacheable (rdim, pResource) && Subresource == 0)
  {
    if (rdim == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
    {
      auto&& map_ctx = mapped_resources [This];

      // More of an assertion, if this fails something's screwy!
      if (map_ctx.textures.count (pResource))
      {
        std::lock_guard <SK_Thread_CriticalSection> auto_lock  (*cs_mmio);

        uint64_t time_elapsed =
          SK_QueryPerf ().QuadPart - map_ctx.texture_times [pResource];

        CComQIPtr <ID3D11Texture2D> pTex = pResource;

        if (pTex != nullptr)
        {
          uint32_t checksum  = 0;
          size_t   size      = 0;
          uint32_t top_crc32 = 0x00;

          D3D11_TEXTURE2D_DESC desc = { };
               pTex->GetDesc (&desc);

          if ((desc.BindFlags & D3D11_BIND_SHADER_RESOURCE) || desc.Usage == D3D11_USAGE_STAGING)
          {
            checksum =
              crc32_tex ( &desc,
                         (D3D11_SUBRESOURCE_DATA *)(
                            &map_ctx.textures [pResource]
                         ), &size, &top_crc32 );

            //dll_log.Log (L"[DX11TexMgr] Mapped 2D texture... (%x -- %lu bytes)", checksum, size);

            if (checksum != 0x0)
            {
              static auto& textures =
                SK_D3D11_Textures;

              if (desc.Usage != D3D11_USAGE_STAGING)
              {
                const uint32_t cache_tag    =
                  safe_crc32c (top_crc32, (uint8_t *)(&desc), sizeof D3D11_TEXTURE2D_DESC);

                textures.CacheMisses_2D++;

                textures.refTexture2D ( pTex,
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
              }
            }
          }
        }

        map_ctx.textures.erase      (pResource);
        map_ctx.texture_times.erase (pResource);
      }
    }
  }

  D3D11_Unmap_Original (This, pResource, Subresource);
}


__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_CopyResource_Override (
       ID3D11DeviceContext *This,
  _In_ ID3D11Resource      *pDstResource,
  _In_ ID3D11Resource      *pSrcResource )
{
  // UB: If it's happening, pretend we never saw this...
  if (pDstResource == nullptr || pSrcResource == nullptr)
  {
    return;
  }

  SK_TLS *pTLS =
    SK_TLS_Bottom ();

  if (! pTLS->imgui.drawing)
  {
    if (!(This->GetType () == D3D11_DEVICE_CONTEXT_DEFERRED && (!config.render.dxgi.deferred_isolation)))
    {
      SK_ReShade_CopyResourceCallback.call (pDstResource, pSrcResource, pTLS);
    }
  }

  CComQIPtr <ID3D11Texture2D> pDstTex = pDstResource;

  if (pDstTex != nullptr)
  {
    if (! SK_D3D11_IsTexInjectThread (pTLS))
    {
      if (SK_D3D11_TextureIsCached (pDstTex))
      {
        //SK_LOG0 ( (L"Cached texture was modified (CopyResource)... removing from cache! - <%s>",
        //               SK_GetCallerName ().c_str ()), L"DX11TexMgr" );
        //SK_D3D11_RemoveTexFromCache (pDstTex, true);
      }
    }
  }


  // ImGui gets to pass-through without invoking the hook
  if ((! config.textures.cache.allow_staging) && (! SK_D3D11_ShouldTrackRenderOp (This, &pTLS)))
  {
    D3D11_CopyResource_Original (This, pDstResource, pSrcResource);
  
    return;
  }


  D3D11_RESOURCE_DIMENSION res_dim = { };
   pSrcResource->GetType (&res_dim);


  if (SK_D3D11_EnableMMIOTracking)
  {
    SK_D3D11_MemoryThreads.mark ();

    switch (res_dim)
    {
      case D3D11_RESOURCE_DIMENSION_UNKNOWN:
        mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_UNKNOWN]++;
        break;
      case D3D11_RESOURCE_DIMENSION_BUFFER:
      {
        mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_BUFFER]++;
    
        ID3D11Buffer* pBuffer = nullptr;
    
        if (SUCCEEDED (pSrcResource->QueryInterface <ID3D11Buffer> (&pBuffer)))
        {
          D3D11_BUFFER_DESC  buf_desc = { };
          pBuffer->GetDesc (&buf_desc);
          {
            if (buf_desc.BindFlags & D3D11_BIND_INDEX_BUFFER)
              mem_map_stats.last_frame.index_buffers.insert (pBuffer);

            if (buf_desc.BindFlags & D3D11_BIND_VERTEX_BUFFER)
              mem_map_stats.last_frame.vertex_buffers.insert (pBuffer);

            if (buf_desc.BindFlags & D3D11_BIND_CONSTANT_BUFFER)
              mem_map_stats.last_frame.constant_buffers.insert (pBuffer);
          }

          mem_map_stats.last_frame.bytes_copied += buf_desc.ByteWidth;

          pBuffer->Release ();
        }
      } break;
      case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
        mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_TEXTURE1D]++;
        break;
      case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
        mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_TEXTURE2D]++;
        break;
      case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
        mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_TEXTURE3D]++;
        break;
    }
  }

  D3D11_CopyResource_Original (This, pDstResource, pSrcResource);


  if ( SK_D3D11_IsStagingCacheable (res_dim, pSrcResource) ||
       SK_D3D11_IsStagingCacheable (res_dim, pDstResource) )
  {
    auto&& map_ctx = mapped_resources [This];

    CComQIPtr <ID3D11Texture2D> pSrcTex (pSrcResource);

    if (pSrcTex != nullptr)
    {
      if (SK_D3D11_TextureIsCached (pSrcTex))
      {
        dll_log.Log (L"Copying from cached source with checksum: %x", SK_D3D11_TextureHashFromCache (pSrcTex));
      }
    }

    if (pDstTex != nullptr && map_ctx.dynamic_textures.count (pSrcResource))
    {
      const uint32_t top_crc32 = map_ctx.dynamic_texturesx [pSrcResource];
      const uint32_t checksum  = map_ctx.dynamic_textures  [pSrcResource];

      D3D11_TEXTURE2D_DESC dst_desc = { };
        pDstTex->GetDesc (&dst_desc);

      const uint32_t cache_tag =
        safe_crc32c (top_crc32, (uint8_t *)(&dst_desc), sizeof D3D11_TEXTURE2D_DESC);

      if (checksum != 0x00 && dst_desc.Usage != D3D11_USAGE_STAGING)
      {
        static auto& textures =
          SK_D3D11_Textures;

        textures.CacheMisses_2D++;

        textures.refTexture2D ( pDstTex,
                                  &dst_desc,
                                    cache_tag,
                                      map_ctx.dynamic_sizes2   [checksum],
                                        map_ctx.dynamic_times2 [checksum],
                                          top_crc32,
                                 L"", nullptr, (HMODULE)(intptr_t)-1/*SK_GetCallingDLL ()*/,
                                                pTLS );
        map_ctx.dynamic_textures.erase  (pSrcResource);
        map_ctx.dynamic_texturesx.erase (pSrcResource);

        map_ctx.dynamic_sizes2.erase    (checksum);
        map_ctx.dynamic_times2.erase    (checksum);
      }
    }
  }
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_CopySubresourceRegion_Override (
  _In_           ID3D11DeviceContext *This,
  _In_           ID3D11Resource      *pDstResource,
  _In_           UINT                 DstSubresource,
  _In_           UINT                 DstX,
  _In_           UINT                 DstY,
  _In_           UINT                 DstZ,
  _In_           ID3D11Resource      *pSrcResource,
  _In_           UINT                 SrcSubresource,
  _In_opt_ const D3D11_BOX           *pSrcBox )
{
  // UB: If it's happening, pretend we never saw this...
  if (pDstResource == nullptr || pSrcResource == nullptr)
  {
    return;
  }


  ///if (SK_GetCurrentGameID() == SK_GAME_ID::Ys_Eight)
  ///{
  ///  CComQIPtr <ID3D11Texture2D> pTex (pSrcResource);
  ///
  ///  if (pTex)
  ///  {
  ///    D3D11_BOX box = { };
  ///    
  ///    if (pSrcBox != nullptr)
  ///        box = *pSrcBox;
  ///
  ///    else
  ///    {
  ///      D3D11_TEXTURE2D_DESC tex_desc = {};
  ///           pTex->GetDesc (&tex_desc);
  ///
  ///      box.left  = 0; box.right  = tex_desc.Width; 
  ///      box.top   = 0; box.bottom = tex_desc.Height;
  ///      box.front = 0; box.back   = 1;
  ///    }
  ///    
  ///    dll_log.Log ( L"CopySubresourceRegion:  { %s <%lu> [ %lu/%lu, %lu/%lu, %lu/%lu ] } -> { %s <%lu> (%lu,%lu,%lu) }",
  ///                    DescribeResource (pSrcResource).c_str (), SrcSubresource, box.left,box.right, box.top,box.bottom, box.front,box.back,
  ///                    DescribeResource (pDstResource).c_str (), DstSubresource, DstX, DstY, DstZ );
  ///  }
  ///}


  ///if ( (! config.render.dxgi.deferred_isolation)    &&
  ///          This->GetType () == D3D11_DEVICE_CONTEXT_DEFERRED )
  ///{
  ///  return
  ///    D3D11_CopySubresourceRegion_Original ( This, pDstResource, DstSubresource,
  ///                                                   DstX, DstY, DstZ,
  ///                                                     pSrcResource, SrcSubresource,
  ///                                                       pSrcBox );
  ///}


  SK_TLS* pTLS = SK_TLS_Bottom ();

  CComQIPtr <ID3D11Texture2D> pDstTex (pDstResource);

  if (pDstTex != nullptr)
  {
    if (! SK_D3D11_IsTexInjectThread (pTLS))
    {
      //if (SK_GetCurrentGameID () == SK_GAME_ID::PillarsOfEternity2)
      //{
      //  extern          bool SK_POE2_NopSubresourceCopy;
      //  extern volatile LONG SK_POE2_SkippedCopies;
      //
      //  if (SK_POE2_NopSubresourceCopy)
      //  {
      //    D3D11_TEXTURE2D_DESC desc_out = { };
      //      pDstTex->GetDesc (&desc_out);
      //
      //    if (pSrcBox != nullptr)
      //    {
      //      dll_log.Log (L"Copy (%lu-%lu,%lu-%lu : %lu,%lu,%lu : %s : {%p,%p})",
      //        pSrcBox->left, pSrcBox->right, pSrcBox->top, pSrcBox->bottom,
      //          DstX, DstY, DstZ,
      //            SK_D3D11_DescribeUsage (desc_out.Usage),
      //              pSrcResource, pDstResource );
      //    }
      //
      //    else
      //    {
      //      dll_log.Log (L"Copy (%lu,%lu,%lu : %s)",
      //                   DstX, DstY, DstZ,
      //                   SK_D3D11_DescribeUsage (desc_out.Usage) );
      //    }
      //
      //    if (pSrcBox == nullptr || ( pSrcBox->right != 3840 || pSrcBox->bottom != 2160 ))
      //    {
      //      if (desc_out.Usage == D3D11_USAGE_STAGING || pSrcBox == nullptr)
      //      {
      //        InterlockedIncrement (&SK_POE2_SkippedCopies);
      //
      //        return;
      //      }
      //    }
      //  }
      //}

      if (DstSubresource == 0 && SK_D3D11_TextureIsCached (pDstTex))
      {
        SK_LOG0 ( (L"Cached texture was modified (CopySubresourceRegion)... removing from cache! - <%s>",
                       SK_GetCallerName ().c_str ()), L"DX11TexMgr" );
        SK_D3D11_RemoveTexFromCache (pDstTex, true);
      }
    }
  }


  // ImGui gets to pass-through without invoking the hook
  if (! SK_D3D11_ShouldTrackRenderOp (This, &pTLS))
  {
    D3D11_CopySubresourceRegion_Original (This, pDstResource, DstSubresource, DstX, DstY, DstZ, pSrcResource, SrcSubresource, pSrcBox);

    return;
  }


  D3D11_RESOURCE_DIMENSION res_dim = { };
  pSrcResource->GetType  (&res_dim);


  if (SK_D3D11_EnableMMIOTracking)
  {
    SK_D3D11_MemoryThreads.mark ();

    switch (res_dim)
    {
      case D3D11_RESOURCE_DIMENSION_UNKNOWN:
        mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_UNKNOWN]++;
        break;
      case D3D11_RESOURCE_DIMENSION_BUFFER:
      {
        mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_BUFFER]++;

        ID3D11Buffer* pBuffer = nullptr;

        if (SUCCEEDED (pSrcResource->QueryInterface <ID3D11Buffer> (&pBuffer)))
        {
          D3D11_BUFFER_DESC  buf_desc = { };
          pBuffer->GetDesc (&buf_desc);
          {
            ////std::lock_guard <SK_Thread_CriticalSection> auto_lock (cs_mmio);

            if (buf_desc.BindFlags & D3D11_BIND_INDEX_BUFFER)
              mem_map_stats.last_frame.index_buffers.insert (pBuffer);

            if (buf_desc.BindFlags & D3D11_BIND_VERTEX_BUFFER)
              mem_map_stats.last_frame.vertex_buffers.insert (pBuffer);

            if (buf_desc.BindFlags & D3D11_BIND_CONSTANT_BUFFER)
              mem_map_stats.last_frame.constant_buffers.insert (pBuffer);
          }

          mem_map_stats.last_frame.bytes_copied += buf_desc.ByteWidth;

          pBuffer->Release ();
        }
      } break;
      case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
        mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_TEXTURE1D]++;
        break;
      case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
        mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_TEXTURE2D]++;
        break;
      case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
        mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_TEXTURE3D]++;
        break;
    }
  }


  D3D11_CopySubresourceRegion_Original (This, pDstResource, DstSubresource, DstX, DstY, DstZ, pSrcResource, SrcSubresource, pSrcBox);

  if ( ( SK_D3D11_IsStagingCacheable (res_dim, pSrcResource) ||
         SK_D3D11_IsStagingCacheable (res_dim, pDstResource) ) && SrcSubresource == 0 && DstSubresource == 0)
  {
    auto&& map_ctx = mapped_resources [This];

    if (pDstTex != nullptr && map_ctx.dynamic_textures.count (pSrcResource))
    {
      const uint32_t top_crc32 = map_ctx.dynamic_texturesx [pSrcResource];
      const uint32_t checksum  = map_ctx.dynamic_textures  [pSrcResource];

      D3D11_TEXTURE2D_DESC dst_desc = { };
        pDstTex->GetDesc (&dst_desc);

      const uint32_t cache_tag =
        safe_crc32c (top_crc32, (uint8_t *)(&dst_desc), sizeof D3D11_TEXTURE2D_DESC);

      if (checksum != 0x00 && dst_desc.Usage != D3D11_USAGE_STAGING)
      {
        static auto& textures =
          SK_D3D11_Textures;

        textures.refTexture2D ( pDstTex,
                                  &dst_desc,
                                    cache_tag,
                                      map_ctx.dynamic_sizes2   [checksum],
                                        map_ctx.dynamic_times2 [checksum],
                                          top_crc32,
                            L"", nullptr, (HMODULE)(intptr_t)-1/*SK_GetCallingDLL ()*/,
                                            pTLS );

        map_ctx.dynamic_textures.erase  (pSrcResource);
        map_ctx.dynamic_texturesx.erase (pSrcResource);

        map_ctx.dynamic_sizes2.erase    (checksum);
        map_ctx.dynamic_times2.erase    (checksum);
      }
    }
  }
}



void
SK_D3D11_ClearResidualDrawState (SK_TLS* pTLS = SK_TLS_Bottom ())
{
  if (pTLS->d3d11.pOrigBlendState != nullptr)
  {
    CComQIPtr <ID3D11DeviceContext> pDevCtx (
      pTLS->d3d11.pDevCtx
    );

    pDevCtx->OMSetBlendState ( pTLS->d3d11.pOrigBlendState,
                               pTLS->d3d11.fOrigBlendFactors,
                               pTLS->d3d11.uiOrigBlendMask );

    pTLS->d3d11.pOrigBlendState->Release ();
    pTLS->d3d11.pOrigBlendState = nullptr;
  }

  if (pTLS->d3d11.pRTVOrig != nullptr)
  {
    CComQIPtr <ID3D11DeviceContext> pDevCtx (
      pTLS->d3d11.pDevCtx
    );

    if (pDevCtx != nullptr)
    {
      pDevCtx->OMSetRenderTargets (1, &pTLS->d3d11.pRTVOrig, nullptr);
                                     //pTLS->d3d11.pRTVOrig->Release ();
                                       pTLS->d3d11.pRTVOrig = nullptr;
    }
  }

  if (pTLS->d3d11.pDSVOrig != nullptr)
  {
    CComQIPtr <ID3D11DeviceContext> pDevCtx (
      pTLS->d3d11.pDevCtx
    );

    if (pDevCtx != nullptr)
    {
      ID3D11RenderTargetView* pRTV [8] = { };
      CComPtr <ID3D11DepthStencilView> pDSV;
      
      pDevCtx->OMGetRenderTargets (8, &pRTV [0], &pDSV);

      const UINT OMRenderTargetCount =
        calc_count (&pRTV [0], D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT);

      pDevCtx->OMSetRenderTargets (OMRenderTargetCount, &pRTV [0], pTLS->d3d11.pDSVOrig);

      for (UINT i = 0; i < OMRenderTargetCount; i++)
      {
        if (pRTV [i] != 0) pRTV [i]->Release ();
      }

      pTLS->d3d11.pDSVOrig = nullptr;
    }
  }

  if (pTLS->d3d11.pDepthStencilStateOrig != nullptr)
  {
    CComQIPtr <ID3D11DeviceContext> pDevCtx (
      pTLS->d3d11.pDevCtx
    );

    if (pDevCtx != nullptr)
    {
      pDevCtx->OMSetDepthStencilState (pTLS->d3d11.pDepthStencilStateOrig, pTLS->d3d11.StencilRefOrig);

      pTLS->d3d11.pDepthStencilStateOrig->Release ();
      pTLS->d3d11.pDepthStencilStateOrig = nullptr;

      if (pTLS->d3d11.pDepthStencilStateNew != nullptr)
      {
        pTLS->d3d11.pDepthStencilStateNew->Release ();
        pTLS->d3d11.pDepthStencilStateNew = nullptr;
      }
    }
  }


  if (pTLS->d3d11.pRasterStateOrig != nullptr)
  {
    CComQIPtr <ID3D11DeviceContext> pDevCtx (
      pTLS->d3d11.pDevCtx
    );

    if (pDevCtx != nullptr)
    {
      pDevCtx->RSSetState (pTLS->d3d11.pRasterStateOrig);

      pTLS->d3d11.pRasterStateOrig->Release ();
      pTLS->d3d11.pRasterStateOrig = nullptr;

      if (pTLS->d3d11.pRasterStateNew != nullptr)
      {
        pTLS->d3d11.pRasterStateNew->Release ();
        pTLS->d3d11.pRasterStateNew = nullptr;
      }
    }
  }


  for (int i = 0; i < 5; i++)
  {
    if (pTLS->d3d11.pOriginalCBuffers [i][0] != nullptr)
    {
      if (reinterpret_cast <intptr_t>(pTLS->d3d11.pOriginalCBuffers [i][0]) == -1)
        pTLS->d3d11.pOriginalCBuffers [i][0] = nullptr;

      CComQIPtr <ID3D11DeviceContext> pDevCtx (
        pTLS->d3d11.pDevCtx
      );

      switch (i)
      {
        case 0:
          pDevCtx->VSSetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, &pTLS->d3d11.pOriginalCBuffers [i][0]);
          break;
        case 1:
          pDevCtx->PSSetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, &pTLS->d3d11.pOriginalCBuffers [i][0]);
          break;
        case 2:
          pDevCtx->GSSetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, &pTLS->d3d11.pOriginalCBuffers [i][0]);
          break;
        case 3:
          pDevCtx->HSSetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, &pTLS->d3d11.pOriginalCBuffers [i][0]);
          break;
        case 4:
          pDevCtx->DSSetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, &pTLS->d3d11.pOriginalCBuffers [i][0]);
          break;
      }

      for (int j = 0; j < D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT; j++)
      {
        if (pTLS->d3d11.pOriginalCBuffers [i][j] != nullptr)
        {
          pTLS->d3d11.pOriginalCBuffers [i][j]->Release ();
          pTLS->d3d11.pOriginalCBuffers [i][j] = nullptr;
        }
      }
    }
  }
}


void
SK_D3D11_PostDraw (SK_TLS *pTLS)
{
  //if ( SK_GetCurrentRenderBackend ().device              == nullptr ||
  //     SK_GetCurrentRenderBackend ().swapchain           == nullptr ) return;

  SK_D3D11_ClearResidualDrawState (pTLS);
}

struct
{
  SK_ReShade_PresentCallback_pfn fn   = nullptr;
  void*                          data = nullptr;

  struct explict_draw_s
  {
    void*                   ptr     = nullptr;
    ID3D11RenderTargetView* pRTV    = nullptr;
    bool                    pass    = false;
    int                     calls   = 0;
    ID3D11DeviceContext*    src_ctx = nullptr;
  } explicit_draw;
} SK_ReShade_PresentCallback;

__declspec (noinline)
IMGUI_API
void
SK_ReShade_InstallPresentCallback (SK_ReShade_PresentCallback_pfn fn, void* user) 
{
  SK_ReShade_PresentCallback.fn                = fn;
  SK_ReShade_PresentCallback.explicit_draw.ptr = user;
}

__declspec (noinline)
IMGUI_API
void
SK_ReShade_InstallDrawCallback (SK_ReShade_OnDrawD3D11_pfn fn, void* user) 
{
  SK_ReShade_DrawCallback.fn   = fn;
  SK_ReShade_DrawCallback.data = user;
}

__declspec (noinline)
IMGUI_API
void
SK_ReShade_InstallSetDepthStencilViewCallback (SK_ReShade_OnSetDepthStencilViewD3D11_pfn fn, void* user) 
{
  SK_ReShade_SetDepthStencilViewCallback.fn   = fn;
  SK_ReShade_SetDepthStencilViewCallback.data = user;
}

__declspec (noinline)
IMGUI_API
void
SK_ReShade_InstallGetDepthStencilViewCallback (SK_ReShade_OnGetDepthStencilViewD3D11_pfn fn, void* user) 
{
  SK_ReShade_GetDepthStencilViewCallback.fn   = fn;
  SK_ReShade_GetDepthStencilViewCallback.data = user;
}

__declspec (noinline)
IMGUI_API
void
SK_ReShade_InstallClearDepthStencilViewCallback (SK_ReShade_OnClearDepthStencilViewD3D11_pfn fn, void* user) 
{
  SK_ReShade_ClearDepthStencilViewCallback.fn   = fn;
  SK_ReShade_ClearDepthStencilViewCallback.data = user;
}

__declspec (noinline)
IMGUI_API
void
SK_ReShade_InstallCopyResourceCallback (SK_ReShade_OnCopyResourceD3D11_pfn fn, void* user) 
{
  SK_ReShade_CopyResourceCallback.fn   = fn;
  SK_ReShade_CopyResourceCallback.data = user;
}

class SK_D3D11_AbstractBlacklist
{
public:
  operator uint32_t (void) const {
    return for_shader.crc32c;
  };

  struct {
    uint32_t crc32c;
  } for_shader;// [6];

  struct     {
    struct   {
      struct vtx_count_cond_s {
        std::pair <bool,int> vertices;
                              }
        more_than,
        less_than;
             } have;
             } if_meshes;
};

Concurrency::concurrent_unordered_multimap <uint32_t, SK_D3D11_AbstractBlacklist>
SK_D3D11_BlacklistDrawcalls;

void
_make_blacklist_draw_min_verts ( uint32_t crc32c,
                                int      value  )
{
  SK_D3D11_AbstractBlacklist min_verts;

  min_verts.for_shader.crc32c = crc32c;
  min_verts.if_meshes.have.less_than.vertices =
    std::make_pair ( true, value );
  min_verts.if_meshes.have.more_than.vertices =
    std::make_pair ( false, 0    );

  SK_D3D11_BlacklistDrawcalls.insert (std::make_pair (crc32c, min_verts));
}

void
_make_blacklist_draw_max_verts ( uint32_t crc32c,
                                 int      value  )
{
  SK_D3D11_AbstractBlacklist max_verts;

  max_verts.for_shader.crc32c = crc32c;
  max_verts.if_meshes.have.more_than.vertices =
    std::make_pair ( true, value );
  max_verts.if_meshes.have.less_than.vertices =
    std::make_pair ( false, 0    );

  SK_D3D11_BlacklistDrawcalls.insert (std::make_pair (crc32c, max_verts));
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
      SK_ScopedBool auto_bool (&_tls->imgui.drawing);
      _tls->imgui.drawing = true;

      static auto& shaders =
        SK_D3D11_Shaders;
      
      const UINT dev_idx = _ctx_handle;

      if (_ctx->GetType () != D3D11_DEVICE_CONTEXT_DEFERRED && SK_ReShade_PresentCallback.fn && (! shaders.reshade_triggered))
      {
        //CComPtr <ID3D11DepthStencilView>  pDSV = nullptr;
        //CComPtr <ID3D11DepthStencilState> pDSS = nullptr;
        //CComPtr <ID3D11RenderTargetView>  pRTV = nullptr;
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
              SK_D3D11_KnownShaders::ShaderRegistry <IUnknown *>* pShaderReg = { };

              switch (i)
              {
                default:
                case 0:
                  pShaderReg = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown *> *)&shaders.vertex;
                  break;
                case 1:
                  pShaderReg = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown *> *)&shaders.pixel;
                  break;
                case 2:
                  pShaderReg = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown *> *)&shaders.geometry;
                  break;
                case 3:
                  pShaderReg = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown *> *)&shaders.hull;
                  break;
                case 4:
                  pShaderReg = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown *> *)&shaders.domain;
                  break;
              };

              if ( (! pShaderReg->trigger_reshade.after.empty ()) &&
                      pShaderReg->current.shader [dev_idx] != 0x0    &&
                      pShaderReg->trigger_reshade.after.count (pShaderReg->current.shader [dev_idx]) )
              {
                shaders.reshade_triggered = true;

                if (! (_ctx->GetType () == D3D11_DEVICE_CONTEXT_DEFERRED && (! config.render.dxgi.deferred_isolation)))
                {
                  SK_ReShade_PresentCallback.explicit_draw.calls++;
                  SK_ReShade_PresentCallback.explicit_draw.src_ctx = _ctx;
                  SK_ReShade_PresentCallback.fn (&SK_ReShade_PresentCallback.explicit_draw);
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


void*
__cdecl
SK_SEH_Guarded_memcpy (
    _Out_writes_bytes_all_ (_Size) void       *_Dst,
    _In_reads_bytes_       (_Size) void const *_Src,
    _In_                           size_t      _Size ) 
{
  __try {
    return memcpy (_Dst, _Src, _Size);
  } __except ( GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ?
                            EXCEPTION_EXECUTE_HANDLER :
                            EXCEPTION_CONTINUE_SEARCH   )
  {
    return _Dst;
  }
}

#include <concurrent_vector.h>
#include <array>

concurrency::concurrent_vector <d3d11_shader_tracking_s::cbuffer_override_s> __SK_D3D11_VertexShader_CBuffer_Overrides;
concurrency::concurrent_vector <d3d11_shader_tracking_s::cbuffer_override_s> __SK_D3D11_PixelShader_CBuffer_Overrides;

bool SK_D3D11_KnownShaders::reshade_triggered;

// Indicates whether the shader mod window is tracking render target refs
static bool live_rt_view = true;

bool
SK_D3D11_DrawCallFilter (int elem_cnt, int vtx_cnt, uint32_t vtx_shader)
{
  UNREFERENCED_PARAMETER (elem_cnt);

  if (SK_D3D11_BlacklistDrawcalls.empty ())
    return false;

  const auto& matches =
    SK_D3D11_BlacklistDrawcalls.equal_range (vtx_shader);

  for ( auto it = matches.first; it != matches.second; ++it )
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
}

ID3D11ShaderResourceView* g_pRawHDR = nullptr;

bool
SK_D3D11_DrawHandler (ID3D11DeviceContext* pDevCtx, SK_TLS* pTLS = nullptr, INT d_idx = -1)
{
  static const
    SK_RenderBackend& rb =
      SK_GetCurrentRenderBackend ();

  if ( rb.d3d11.immediate_ctx == nullptr ||
       rb.device              == nullptr ||
       rb.swapchain           == nullptr )
  {
    return false;
  }


  if (pDevCtx->GetType () != D3D11_DEVICE_CONTEXT_IMMEDIATE && (! config.render.dxgi.deferred_isolation))
    return false;

  if (! pTLS)
        pTLS = SK_TLS_Bottom ();

  // ImGui gets to pass-through without invoking the hook
  if (pTLS->imgui.drawing)
    return false;

  SK_ScopedBool auto_bool (&pTLS->imgui.drawing);
                            pTLS->imgui.drawing = true;


  // Make sure state cleanup happens on the same context, or deferred rendering
  //   will make life miserable!
  pTLS->d3d11.pDevCtx = pDevCtx;

  ///auto HashFromCtx =
  ///  [] ( std::array <uint32_t, SK_D3D11_MAX_DEV_CONTEXTS>& registry,
  ///       UINT                                              dev_idx  ) ->
  ///uint32_t
  ///{
  ///  return registry [dev_idx];
  ///};

  static auto& shaders =
    SK_D3D11_Shaders;

  static auto& vertex   = shaders.vertex;
  static auto& pixel    = shaders.pixel;
  static auto& geometry = shaders.geometry;
  static auto& hull     = shaders.hull;
  static auto& domain   = shaders.domain;


  UINT dev_idx =                ( d_idx == -1 ?
    SK_D3D11_GetDeviceContextHandle (pDevCtx) : (UINT)d_idx );

  uint32_t current_vs = vertex.current.shader   [dev_idx];
  uint32_t current_ps = pixel.current.shader    [dev_idx];
  uint32_t current_gs = geometry.current.shader [dev_idx];
  uint32_t current_hs = hull.current.shader     [dev_idx];
  uint32_t current_ds = domain.current.shader   [dev_idx];



  //if (current_vs == 0x9b2090af)
  //{
  //  //CB0[19]
  //  //CB1[ 3]
  //  ID3D11Buffer* ConstantBuffers [2] = { nullptr, nullptr };
  //  pDevCtx->VSGetConstantBuffers (0, 2, ConstantBuffers);
  //
  //  ID3D11Buffer* CB0 = ConstantBuffers [0];
  //  ID3D11Buffer* CB1 = ConstantBuffers [1];
  //
  //  D3D11_BUFFER_DESC desc0, desc1;
  //
  //  CB0->GetDesc (&desc0);
  //  CB1->GetDesc (&desc1);
  //
  //  size_t size0 = desc0.ByteWidth;
  //  size_t size1 = desc1.ByteWidth;
  //
  //  CComPtr <ID3D11Device> pDev;
  //    pDevCtx->GetDevice (&pDev);
  //
  //  desc0.Usage          = D3D11_USAGE_STAGING;
  //  desc0.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
  //
  //  CComPtr <ID3D11Buffer> CB0_Copy;
  //  pDev->CreateBuffer (&desc0, nullptr, &CB0_Copy);
  //
  //  desc1.Usage          = D3D11_USAGE_STAGING;
  //  desc1.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
  //
  //  CComPtr <ID3D11Buffer> CB1_Copy;
  //  pDev->CreateBuffer (&desc1, nullptr, &CB1_Copy);
  //
  //  pDevCtx->CopyResource (CB0_Copy, CB0);
  //  pDevCtx->CopyResource (CB1_Copy, CB1);
  //  
  //  
  //  D3D11_MAPPED_SUBRESOURCE mappedCB0 = { };
  //  D3D11_MAPPED_SUBRESOURCE mappedCB1 = { };
  //
  //  pDevCtx->Map   (CB0_Copy, 0, D3D11_MAP_READ, 0x0, &mappedCB0);
  //         float *pfCB0 = (float *)mappedCB0.pData;
  //  for (int i = 0; i < (size0 / sizeof (float)); i++)
  //  {
  //    dll_log.Log (L"CB0[%i]=%f", i, (*pfCB0));
  //    pfCB0++;
  //  }
  //  pDevCtx->Unmap (CB0_Copy, 0);
  //  
  //  pDevCtx->Map   (CB1_Copy, 0, D3D11_MAP_READ, 0x0, &mappedCB1);
  //         float *pfCB1 = (float *)mappedCB1.pData;
  //  for (int i = 0; i < (size1 / sizeof (float)); i++)
  //  {
  //    dll_log.Log (L"CB1[%i]=%f", i, (*pfCB1));
  //    pfCB1++;
  //  }
  //  pDevCtx->Unmap (CB1_Copy, 0);
  //
  //  CB0->Release (); CB1->Release ();
  //}

  
  //if (SK_GetCurrentGameID () == SK_GAME_ID::Yakuza0)
  //{
  //  if (SK_ReShade_PresentCallback.fn != nullptr && current_ps == 0x2e24510d)
  //  {
  //    SK_D3D11_Shaders.reshade_triggered [dev_idx]     = true;
  //    SK_ReShade_PresentCallback.explicit_draw.src_ctx = nullptr;
  //    SK_ReShade_PresentCallback.fn (&SK_ReShade_PresentCallback.explicit_draw);
  //  }
  //}


const
 auto
  TriggerReShade_Before = [&]
  {
    //if (! (pDevCtx->GetType () == D3D11_DEVICE_CONTEXT_DEFERRED && (! config.render.dxgi.deferred_isolation)) && SK_ReShade_PresentCallback.fn && (! shaders.reshade_triggered [dev_idx]))

    if (   SK_ReShade_PresentCallback.fn != nullptr &&
        (! shaders.reshade_triggered )                 )
    {
      if (reshade_trigger_before [dev_idx])
      {
        shaders.reshade_triggered        = true;
        reshade_trigger_before [dev_idx] = false;

        SK_ReShade_PresentCallback.explicit_draw.calls++;
        SK_ReShade_PresentCallback.explicit_draw.src_ctx = pDevCtx;
        SK_ReShade_PresentCallback.fn (&SK_ReShade_PresentCallback.explicit_draw);
      }
    }
  };


  TriggerReShade_Before ();


  if (SK_D3D11_EnableTracking)
  {
    SK_D3D11_DrawThreads.mark ();

    bool rtv_active = false;

    if (tracked_rtv.active_count [dev_idx] > 0)
    {
      rtv_active = true;

      // Reference tracking is only used when the mod tool window is open,
      //   so skip lengthy work that would otherwise be necessary.
      if ( live_rt_view && SK::ControlPanel::D3D11::show_shader_mod_dlg &&
           SK_ImGui_Visible )
      {
        if (current_vs != 0x00) tracked_rtv.ref_vs.insert (current_vs);
        if (current_ps != 0x00) tracked_rtv.ref_ps.insert (current_ps);
        if (current_gs != 0x00) tracked_rtv.ref_gs.insert (current_gs);
        if (current_hs != 0x00) tracked_rtv.ref_hs.insert (current_hs);
        if (current_ds != 0x00) tracked_rtv.ref_ds.insert (current_ds);
      }
    }
  }

        bool on_top           = false;
        bool wireframe        = false;
  const bool highlight_shader = (dwFrameTime % tracked_shader_blink_duration > tracked_shader_blink_duration / 2);

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
  
  for (int i = 0; i < 5; i++)
  {
    auto* tracker = trackers [i];

    const bool active =
      tracker->active.get (dev_idx);

    if (active)
    {
      tracker->use (nullptr);
 
      if (tracker->cancel_draws) return true;

      if (tracker->wireframe) wireframe = tracker->highlight_draws ? highlight_shader : true;
      if (tracker->on_top)    on_top    = tracker->highlight_draws ? highlight_shader : true;

      if (! (wireframe || on_top))
      {
        if (tracker->highlight_draws && highlight_shader) return highlight_shader;
      }
    }
  }


  bool
  SK_D3D11_ShouldSkipHUD (void);

  if (SK_D3D11_ShouldSkipHUD ())
  {
    if ((!   vertex.hud.empty ()) &&   vertex.hud.count (current_vs)) return true;
    if ((!    pixel.hud.empty ()) &&    pixel.hud.count (current_ps)) return true;
    if ((! geometry.hud.empty ()) && geometry.hud.count (current_gs)) return true;
    if ((!     hull.hud.empty ()) &&     hull.hud.count (current_hs)) return true;
    if ((!   domain.hud.empty ()) &&   domain.hud.count (current_ds)) return true;
  }


  if ((! vertex.blacklist.empty   ()) && vertex.blacklist.count   (current_vs)) return true;
  if ((! pixel.blacklist.empty    ()) && pixel.blacklist.count    (current_ps)) return true;
  if ((! geometry.blacklist.empty ()) && geometry.blacklist.count (current_gs)) return true;
  if ((! hull.blacklist.empty     ()) && hull.blacklist.count     (current_hs)) return true;
  if ((! domain.blacklist.empty   ()) && domain.blacklist.count   (current_ds)) return true;

  static auto& Textures_2D =
    SK_D3D11_Textures.Textures_2D;

  /*if ( SK_D3D11_Shaders.vertex.blacklist_if_texture.size   () ||
       SK_D3D11_Shaders.pixel.blacklist_if_texture.size    () ||
       SK_D3D11_Shaders.geometry.blacklist_if_texture.size () )*/

  if ( (! SK_D3D11_Shaders.vertex.blacklist_if_texture.empty () ) &&
          SK_D3D11_Shaders.vertex.blacklist_if_texture.count (current_vs) )
  {
    auto& views =
      vertex.current.views [dev_idx];

    for (auto& it2 : views)
    {
      if (it2 == nullptr)
        continue;

      CComPtr <ID3D11Resource> pRes = nullptr;
            it2->GetResource (&pRes);

      D3D11_RESOURCE_DIMENSION rdv  = D3D11_RESOURCE_DIMENSION_UNKNOWN;
               pRes->GetType (&rdv);

      if (rdv == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
      {
        CComQIPtr <ID3D11Texture2D> pTex (pRes);

        if (Textures_2D.count (pTex) && Textures_2D [pTex].crc32c != 0x0)
        {
          if (vertex.blacklist_if_texture [current_vs].count (Textures_2D [pTex].crc32c))
            return true;
        }
      }
    }
  }

  if ( (! pixel.blacklist_if_texture.empty (          ) ) &&
          pixel.blacklist_if_texture.count (current_ps) )
  {
    auto& views =
      pixel.current.views [dev_idx];

    for (auto& it2 : views)
    {
      if (it2 == nullptr)
        continue;

      CComPtr <ID3D11Resource> pRes = nullptr;
            it2->GetResource (&pRes);

      D3D11_RESOURCE_DIMENSION rdv  = D3D11_RESOURCE_DIMENSION_UNKNOWN;
               pRes->GetType (&rdv);

      if (rdv == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
      {
        CComQIPtr <ID3D11Texture2D> pTex (pRes);

        if (Textures_2D.count (pTex) && Textures_2D [pTex].crc32c != 0x0)
        {
          if (pixel.blacklist_if_texture [current_ps].count (Textures_2D [pTex].crc32c))
            return true;
        }
      }
    }
  }

  if ( (! geometry.blacklist_if_texture.empty (          ) ) &&
          geometry.blacklist_if_texture.count (current_gs) )
  {
    auto& views =
      geometry.current.views [dev_idx];

    for (auto& it2 : views)
    {
      if (it2 == nullptr)
        continue;

      CComPtr <ID3D11Resource> pRes = nullptr;
            it2->GetResource (&pRes);

      D3D11_RESOURCE_DIMENSION rdv  = D3D11_RESOURCE_DIMENSION_UNKNOWN;
               pRes->GetType (&rdv);

      if (rdv == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
      {
        CComQIPtr <ID3D11Texture2D> pTex (pRes);

        if (Textures_2D.count (pTex) && Textures_2D [pTex].crc32c != 0x00)
        {
          if (geometry.blacklist_if_texture [current_gs].count (Textures_2D [pTex].crc32c))
            return true;
        }
      }
    }
  }


  if (! on_top)
  {
         if ((! vertex.on_top.empty   ()) && vertex.on_top.count   (current_vs)) on_top = true;
    else if ((! pixel.on_top.empty    ()) && pixel.on_top.count    (current_ps)) on_top = true;
    else if ((! geometry.on_top.empty ()) && geometry.on_top.count (current_gs)) on_top = true;
    else if ((! hull.on_top.empty     ()) && hull.on_top.count     (current_hs)) on_top = true;
    else if ((! domain.on_top.empty   ()) && domain.on_top.count   (current_ds)) on_top = true;
  }

  if (on_top)
  {
    CComPtr <ID3D11Device> pDev = nullptr;
    pDevCtx->GetDevice   (&pDev);

    if (pDev != nullptr)
    {
      D3D11_DEPTH_STENCIL_DESC desc = { };

      pDevCtx->OMGetDepthStencilState (&pTLS->d3d11.pDepthStencilStateOrig, &pTLS->d3d11.StencilRefOrig);

      if (pTLS->d3d11.pDepthStencilStateOrig != nullptr)
      {
        pTLS->d3d11.pDepthStencilStateOrig->GetDesc (&desc);

        desc.DepthEnable    = TRUE;
        desc.DepthFunc      = D3D11_COMPARISON_ALWAYS;
        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        desc.StencilEnable  = FALSE;

        if (SUCCEEDED (pDev->CreateDepthStencilState (&desc, &pTLS->d3d11.pDepthStencilStateNew)))
        {
          pDevCtx->OMSetDepthStencilState (pTLS->d3d11.pDepthStencilStateNew, 0);
        }
      }
    }
  }


  if (! wireframe)
  {
    if (     (! vertex.wireframe.empty   ()) && vertex.wireframe.count   (current_vs)) wireframe = true;
    else if ((! pixel.wireframe.empty    ()) && pixel.wireframe.count    (current_ps)) wireframe = true;
    else if ((! geometry.wireframe.empty ()) && geometry.wireframe.count (current_gs)) wireframe = true;
    else if ((! hull.wireframe.empty     ()) && hull.wireframe.count     (current_hs)) wireframe = true;
    else if ((! domain.wireframe.empty   ()) && domain.wireframe.count   (current_ds)) wireframe = true;
  }

  if (wireframe)
  {
    CComPtr <ID3D11Device> pDev = nullptr;
    pDevCtx->GetDevice   (&pDev);

    if (pDev != nullptr)
    {
      pDevCtx->RSGetState         (&pTLS->d3d11.pRasterStateOrig);

      D3D11_RASTERIZER_DESC desc = { };

      if (pTLS->d3d11.pRasterStateOrig != nullptr)
      {
        pTLS->d3d11.pRasterStateOrig->GetDesc (&desc);

        desc.FillMode = D3D11_FILL_WIREFRAME;
        desc.CullMode = D3D11_CULL_NONE;
        //desc.FrontCounterClockwise = TRUE;
        //desc.DepthClipEnable       = FALSE;

        if (SUCCEEDED (pDev->CreateRasterizerState (&desc, &pTLS->d3d11.pRasterStateNew)))
        {
          pDevCtx->RSSetState (pTLS->d3d11.pRasterStateNew);
        }
      }
    }
  }

  if (SK_D3D11_EnableTracking || (ReadAcquire (&SK_D3D11_CBufferTrackingReqs) > 0) )
  {
    const uint32_t current_shaders [5] = {
      current_vs, current_ps,
      current_gs,
      current_hs, current_ds
    };

  for (int i = 0; i < 5; i++)
  {
    if (current_shaders [i] == 0x00)
      continue;

    auto* tracker = trackers [i];

    int num_overrides = 0;

    std::array <d3d11_shader_tracking_s::cbuffer_override_s*, 128> overrides;

    if (tracker->crc32c == current_shaders [i])
    {
      for ( auto& ovr : tracker->overrides )
      {
        if (ovr.Enable && ovr.parent == tracker->crc32c)
        {
          if (ovr.Slot >= 0 && ovr.Slot < D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT)
          {
            overrides [num_overrides++] = &ovr;
          }
        }
      }
    }

    if ((1 << i) == (int)sk_shader_class::Pixel && (! __SK_D3D11_PixelShader_CBuffer_Overrides.empty ()) )
    {
      for ( auto& ovr : __SK_D3D11_PixelShader_CBuffer_Overrides )
      {
        if (ovr.parent == current_ps && ovr.Enable)
        {
          if (ovr.Slot >= 0 && ovr.Slot < D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT)
          {
            overrides [num_overrides++] = &ovr;
          }
        }
      }
    }

    if ((1 << i) == (int)sk_shader_class::Vertex && (! __SK_D3D11_VertexShader_CBuffer_Overrides.empty ()) )
    {
      for ( auto& ovr : __SK_D3D11_VertexShader_CBuffer_Overrides )
      {
        if (ovr.parent == current_vs && ovr.Enable)
        {
          if (ovr.Slot >= 0 && ovr.Slot < D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT)
          {
            overrides [num_overrides++] = &ovr;
          }
        }
      }
    }

    if (num_overrides > 0)
    {
      ID3D11Buffer* pConstantBuffers [D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT] = { };
      UINT          pFirstConstant   [D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT] = { },
                    pNumConstants    [D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT] = { };
      ID3D11Buffer* pConstantCopies  [D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT] = { };

  const
    auto
     _GetConstantBuffers =
      [&](int i, ID3D11Buffer** ppConstantBuffers, UINT* pFirstConstant, UINT* pNumConstants)  ->
      void
      {
        CComQIPtr <ID3D11DeviceContext1> pDevCtx1 (pDevCtx);

        if (pDevCtx1)
        {
          switch (i)
          {
            case 0:
              pDevCtx1->VSGetConstantBuffers1 (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, ppConstantBuffers, pFirstConstant, pNumConstants);
              break;
            case 1:
              pDevCtx1->PSGetConstantBuffers1 (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, ppConstantBuffers, pFirstConstant, pNumConstants);
              break;
            case 2:
              pDevCtx1->GSGetConstantBuffers1 (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, ppConstantBuffers, pFirstConstant, pNumConstants);
              break;
            case 3:
              pDevCtx1->HSGetConstantBuffers1 (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, ppConstantBuffers, pFirstConstant, pNumConstants);
              break;
            case 4:
              pDevCtx1->DSGetConstantBuffers1 (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, ppConstantBuffers, pFirstConstant, pNumConstants);
              break;
          }

          for ( int j = 0 ; j < D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT ; ++j )
          {
            if (ppConstantBuffers [j] != nullptr)
            {
              // Expected non-D3D11.1+ behavior ( Any game that supplies a different value is using code that REQUIRES the D3D11.1 runtime
              //                                    and will require additional state tracking in future versions of Special K )
              if (pFirstConstant [j] != 0)
              {
                dll_log.Log ( L"Detected non-zero first constant offset: %lu on CBuffer slot %lu for shader type %lu",
                                pFirstConstant [j], j, i );
              }

              // Expected non-D3D11.1+ behavior
              if (pNumConstants [j] != 4096)
              {
                dll_log.Log ( L"Detected non-4096 num constants: %lu on CBuffer slot %lu for shader type %lu",
                                pNumConstants [j], j, i );
              }
            }
          }
        }

        else
        {
          switch (i)
          { case 0:
              pDevCtx->VSGetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, ppConstantBuffers);
              break;
            case 1:
              pDevCtx->PSGetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, ppConstantBuffers);
              break;
            case 2:
              pDevCtx->GSGetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, ppConstantBuffers);
              break;
            case 3:
              pDevCtx->HSGetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, ppConstantBuffers);
              break;
            case 4:
              pDevCtx->DSGetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, ppConstantBuffers);
              break;
          }
        }
      };

  const
    auto
     _SetConstantBuffers =
      [&](int i, ID3D11Buffer** ppConstantBuffers)  ->
      void
      {
        switch (i)
        { case 0:
            pDevCtx->VSSetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, ppConstantBuffers);
            break;
          case 1:
            pDevCtx->PSSetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, ppConstantBuffers);
            break;
          case 2:
            pDevCtx->GSSetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, ppConstantBuffers);
            break;
          case 3:
            pDevCtx->HSSetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, ppConstantBuffers);
            break;
          case 4:
            pDevCtx->DSSetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, ppConstantBuffers);
            break;
        }
      };

      _GetConstantBuffers (i, pConstantBuffers, pFirstConstant, pNumConstants);

      for ( UINT j = 0 ; j < D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT ; ++j )
      {
        if (pConstantBuffers [j] == nullptr) continue;// || (! used_slots [j])) continue;

        D3D11_BUFFER_DESC                   buff_desc  = { };
            pConstantBuffers [j]->GetDesc (&buff_desc);

        buff_desc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
        buff_desc.Usage               = D3D11_USAGE_DYNAMIC;
        buff_desc.BindFlags           = D3D11_BIND_CONSTANT_BUFFER;
        buff_desc.MiscFlags           = 0x0;
        buff_desc.StructureByteStride = 0;

        CComPtr <ID3D11Device> pDev;
        pDevCtx->GetDevice   (&pDev);

  const D3D11_MAP                map_type   = D3D11_MAP_WRITE_DISCARD;
        D3D11_MAPPED_SUBRESOURCE mapped_sub = { };
        HRESULT                  hrMap      = E_FAIL;

        bool used       = false;
        UINT start_addr = buff_desc.ByteWidth-1;
        UINT end_addr   = 0;

        for ( int k = 0 ; k < num_overrides ; k++ )
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
          pTLS->d3d11.pOriginalCBuffers [i][j] = pConstantBuffers [j];

          if (SUCCEEDED (pDev->CreateBuffer
                                         (&buff_desc, nullptr, &pConstantCopies [j])))
          {
            if (pConstantBuffers [j] != nullptr)
            {
              if (ReadAcquire (&__SKX_ComputeAntiStall))
              {
                D3D11_BOX src = { };

                src.left   = 0;
                src.right  = buff_desc.ByteWidth;
                src.top    = 0;
                src.bottom = 1;
                src.front  = 0;
                src.back   = 1;

                pDevCtx->CopySubresourceRegion ( pConstantCopies  [j], 0, 0, 0, 0,
                                                 pConstantBuffers [j], 0, &src );
              }

              else
              {
                pDevCtx->CopyResource ( pConstantCopies [j], pConstantBuffers [j] );
              }
            }

            hrMap = pDevCtx->Map ( pConstantCopies [j], 0,
                                     map_type, 0x0,
                                       &mapped_sub );
          }

          if (SUCCEEDED (hrMap))
          {
            for ( int k = 0 ; k < num_overrides ; k++ )
            {
              auto& ovr =
                *(overrides [k]);

              if ( ovr.Slot == j && mapped_sub.pData != nullptr )
              {
                void*   pBase  =
                  ((uint8_t *)mapped_sub.pData   +   ovr.StartAddr);
                SK_SEH_Guarded_memcpy (pBase, ovr.Values, std::min (ovr.Size, 64ui32));
              }
            }

            pDevCtx->Unmap (pConstantCopies [j], 0);
          }
        }
      }

      _SetConstantBuffers (i, pConstantCopies);

      for (int k = 0; k < D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT; k++)
      {
        if (pConstantCopies [k] != nullptr)
        {
          pConstantCopies [k]->Release ();
          pConstantCopies [k] = nullptr;
        }
      }
    }
  }}

  ////SK_ExecuteReShadeOnReturn easy_reshade (pDevCtx, dev_idx, pTLS);

  return false;
}

void
SK_D3D11_PostDispatch (ID3D11DeviceContext* pDevCtx, SK_TLS* pTLS = SK_TLS_Bottom ())
{
  UINT dev_idx =
    SK_D3D11_GetDeviceContextHandle (pDevCtx);

  static auto& compute =
    SK_D3D11_Shaders.compute;

  if (compute.tracked.active.get (dev_idx))
  {
    if (pTLS->d3d11.pOriginalCBuffers [5][0] != nullptr)
    {
      if (reinterpret_cast <intptr_t> (pTLS->d3d11.pOriginalCBuffers [5][0]) == -1)
      {
        pTLS->d3d11.pOriginalCBuffers [5][0] = nullptr;
      }

      pDevCtx->CSSetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, &pTLS->d3d11.pOriginalCBuffers [5][0]);

      for (int j = 0; j < D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT; ++j)
      {
        if (pTLS->d3d11.pOriginalCBuffers [5][j] != nullptr)
        {
          pTLS->d3d11.pOriginalCBuffers [5][j]->Release ();
          pTLS->d3d11.pOriginalCBuffers [5][j] = nullptr;
        }
      }
    }
  }
}

bool
SK_D3D11_DispatchHandler ( ID3D11DeviceContext* pDevCtx,
                           SK_TLS*              pTLS = SK_TLS_Bottom () )
{
  SK_D3D11_DispatchThreads.mark ();

  const UINT dev_idx =
    SK_D3D11_GetDeviceContextHandle (pDevCtx);

  static auto& compute =
    SK_D3D11_Shaders.compute;

  if (SK_D3D11_EnableTracking)
  {
    bool rtv_active = false;

    if (tracked_rtv.active_count [dev_idx] > 0)
    {
      rtv_active = true;

      if (compute.current.shader [dev_idx] != 0x00)
        tracked_rtv.ref_cs.insert (compute.current.shader [dev_idx]);
    }

    if (compute.tracked.active.get (dev_idx)) {compute.tracked.use (nullptr); }
  }

  SK_ScopedBool auto_bool (&pTLS->imgui.drawing);
                            pTLS->imgui.drawing = true;

  const bool highlight_shader =
    (dwFrameTime % tracked_shader_blink_duration > tracked_shader_blink_duration / 2);


  const uint32_t current_cs =
    compute.current.shader [dev_idx];

  if (compute.blacklist.count (current_cs)) return true;

  d3d11_shader_tracking_s*
    tracker = &compute.tracked;

  if (tracker->crc32c == current_cs)
  {
    if (compute.tracked.highlight_draws && highlight_shader) return true;
    if (compute.tracked.cancel_draws)                        return true;

    std::vector <d3d11_shader_tracking_s::cbuffer_override_s> overrides;
    size_t used_slots [D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT] = { };

    for ( auto& ovr : tracker->overrides )
    {
      if (ovr.Enable && ovr.parent == tracker->crc32c)
      {
        if (ovr.Slot >= 0 && ovr.Slot < D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT)
        {
                      used_slots [ovr.Slot] = ovr.BufferSize;
          overrides.emplace_back (ovr);
        }
      }
    }

    if (! overrides.empty ())
    {
      ID3D11Buffer* pConstantBuffers [D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT] = { };
      ID3D11Buffer* pConstantCopies  [D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT] = { };

      pDevCtx->CSGetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, pConstantBuffers);
      pDevCtx->CSSetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, pConstantCopies);

      for ( UINT j = 0 ; j < D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT ; j++ )
      {
        pTLS->d3d11.pOriginalCBuffers [5][j] = pConstantBuffers [j];

        if (j == 0 && pConstantBuffers [j] == nullptr) pTLS->d3d11.pOriginalCBuffers [5][j] = (ID3D11Buffer *)(intptr_t)-1;

            pConstantCopies  [j]  = nullptr;
        if (pConstantBuffers [j] == nullptr && (! used_slots [j])) continue;

        D3D11_BUFFER_DESC                   buff_desc  = { };

        if (pConstantBuffers [j])
            pConstantBuffers [j]->GetDesc (&buff_desc);

        buff_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        buff_desc.Usage          = D3D11_USAGE_DYNAMIC;
        buff_desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;

        CComPtr <ID3D11Device> pDev;
        pDevCtx->GetDevice   (&pDev);

  const D3D11_MAP                map_type   = D3D11_MAP_WRITE_DISCARD;
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
          if (SUCCEEDED (pDev->CreateBuffer
                                         (&buff_desc, nullptr, &pConstantCopies [j])))
          {
            if (pConstantBuffers [j] != nullptr)
            {
              if (ReadAcquire (&__SKX_ComputeAntiStall))
              {
                D3D11_BOX src = { };

                src.left   = 0;
                src.right  = buff_desc.ByteWidth;
                src.top    = 0;
                src.bottom = 1;
                src.front  = 0;
                src.back   = 1;

                pDevCtx->CopySubresourceRegion ( pConstantCopies  [j], 0, 0, 0, 0,
                                                 pConstantBuffers [j], 0, &src );
              }

              else
              {
                pDevCtx->CopyResource ( pConstantCopies [j], pConstantBuffers [j] );
              }
            }

            hrMap = pDevCtx->Map ( pConstantCopies [j], 0,
                                     map_type, 0x0,
                                       &mapped_sub );
          }

          if (SUCCEEDED (hrMap))
          {
            for ( auto& ovr : overrides )
            {
              if ( ovr.Slot == j && mapped_sub.pData != nullptr )
              {
                void*   pBase  = ((uint8_t *)mapped_sub.pData + ovr.StartAddr);

                SK_SEH_Guarded_memcpy (pBase, ovr.Values, ovr.Size);
              }
            }

            pDevCtx->Unmap (pConstantCopies [j], 0);
          }

          else if (pConstantCopies [j] != nullptr)
          {
            dll_log.Log (L"Failure To Copy Resource");
            pConstantCopies [j]->Release ();
            pConstantCopies [j] = nullptr;
          }
        }
      }

      pDevCtx->CSSetConstantBuffers (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, pConstantCopies);

      for (int k = 0; k < D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT; k++)
      {
        if (pConstantCopies [k] != nullptr)
        {
          pConstantCopies [k]->Release ();
          pConstantCopies [k] = nullptr;
        }
      }
    }
  }

  return false;
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DrawAuto_Override (_In_ ID3D11DeviceContext *This)
{
  SK_LOG_FIRST_CALL

  SK_TLS *pTLS = nullptr;

  if (! SK_D3D11_ShouldTrackDrawCall (This, SK_D3D11DrawType::Auto, &pTLS))
  {
    return
      D3D11_DrawAuto_Original (This);
  }

  if (SK_D3D11_DrawHandler (This, pTLS))
    return;

  if (! (This->GetType () == D3D11_DEVICE_CONTEXT_DEFERRED && (! config.render.dxgi.deferred_isolation)))
    SK_ReShade_DrawCallback.call (This, SK_D3D11_ReshadeDrawFactors.auto_draw, pTLS);

  D3D11_DrawAuto_Original (This);

  SK_D3D11_PostDraw (pTLS);
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DrawIndexed_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 IndexCount,
  _In_ UINT                 StartIndexLocation,
  _In_ INT                  BaseVertexLocation )
{
  SK_LOG_FIRST_CALL

  SK_TLS *pTLS = nullptr;

  INT d_idx = -1;

  if (ReadAcquire (&SK_D3D11_DrawTrackingReqs) > 0)
  {
    UINT dev_idx =
      SK_D3D11_GetDeviceContextHandle (This);

    d_idx = dev_idx;

    if (SK_D3D11_DrawCallFilter ( IndexCount, IndexCount,
                  SK_D3D11_Shaders.vertex.current.shader [dev_idx]) )
    {
      return;
    }
  }

  if (! SK_D3D11_ShouldTrackDrawCall (This, SK_D3D11DrawType::Indexed, &pTLS))
  {
    return
      D3D11_DrawIndexed_Original ( This, IndexCount,
                                     StartIndexLocation,
                                       BaseVertexLocation );
  }

  if (SK_D3D11_DrawHandler (This, pTLS, d_idx))
    return;

  if (! (This->GetType () == D3D11_DEVICE_CONTEXT_DEFERRED && (! config.render.dxgi.deferred_isolation)))
    SK_ReShade_DrawCallback.call (This, IndexCount * SK_D3D11_ReshadeDrawFactors.indexed, pTLS);


  D3D11_DrawIndexed_Original ( This, IndexCount,
                                       StartIndexLocation,
                                         BaseVertexLocation );

  SK_D3D11_PostDraw (pTLS);
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_Draw_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 VertexCount,
  _In_ UINT                 StartVertexLocation )
{
  SK_LOG_FIRST_CALL

  static const auto& rb =
    SK_GetCurrentRenderBackend ();

  SK_TLS *pTLS = nullptr;

  INT d_idx = -1;

  if (ReadAcquire (&SK_D3D11_DrawTrackingReqs) > 0)
  {
    UINT dev_idx =
      SK_D3D11_GetDeviceContextHandle (This);

    d_idx = dev_idx;

    if (SK_D3D11_DrawCallFilter ( 0, VertexCount,
                    SK_D3D11_Shaders.vertex.current.shader [dev_idx]) )
    {
      return;
    }
  }


  // Render-state tracking needs to be forced-on for the
  //   Steam Overlay HDR fix to work.
  if ( rb.isHDRCapable ()  &&
      (rb.framebuffer_flags & SK_FRAMEBUFFER_FLAG_HDR) )
  {
    UINT dev_idx = ( d_idx != -1 ? d_idx :
                       SK_D3D11_GetDeviceContextHandle (This) );

#define STEAM_OVERLAY_VS_CRC32C 0xf48cf597

    if ( STEAM_OVERLAY_VS_CRC32C == 
           SK_D3D11_Shaders.vertex.current.shader [dev_idx] )
    {
      extern HRESULT
        SK_D3D11_InjectSteamHDR ( _In_ ID3D11DeviceContext *pDevCtx,
                                  _In_ UINT                 VertexCount,
                                  _In_ UINT                 StartVertexLocation,
                                  _In_ D3D11_Draw_pfn       pfnD3D11Draw );

      if ( SUCCEEDED (
             SK_D3D11_InjectSteamHDR ( This, VertexCount,
                                         StartVertexLocation,
                                           D3D11_Draw_Original )
           )
         )
      {
        return;
      }
    }
  }


  if (! SK_D3D11_ShouldTrackDrawCall (This, SK_D3D11DrawType::PrimList, &pTLS))
  {
    return
      D3D11_Draw_Original ( This,
                              VertexCount,
                                StartVertexLocation );
  }

  if (SK_D3D11_DrawHandler (This, pTLS, d_idx))
    return;

  if (! (This->GetType () == D3D11_DEVICE_CONTEXT_DEFERRED && (! config.render.dxgi.deferred_isolation)))
    SK_ReShade_DrawCallback.call (This, VertexCount * SK_D3D11_ReshadeDrawFactors.draw, nullptr);

  D3D11_Draw_Original ( This, VertexCount, StartVertexLocation );

  SK_D3D11_PostDraw (pTLS);
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DrawIndexedInstanced_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 IndexCountPerInstance,
  _In_ UINT                 InstanceCount,
  _In_ UINT                 StartIndexLocation,
  _In_ INT                  BaseVertexLocation,
  _In_ UINT                 StartInstanceLocation )
{
  SK_LOG_FIRST_CALL

  SK_TLS *pTLS = nullptr;

  if (! SK_D3D11_ShouldTrackDrawCall (This, SK_D3D11DrawType::IndexedInstanced, &pTLS))
  {
    return
      D3D11_DrawIndexedInstanced_Original ( This, IndexCountPerInstance,
                                              InstanceCount, StartIndexLocation,
                                                BaseVertexLocation, StartInstanceLocation );
  }

  if (SK_D3D11_DrawHandler (This, pTLS))
    return;

  if (! (This->GetType () == D3D11_DEVICE_CONTEXT_DEFERRED && (! config.render.dxgi.deferred_isolation)))
    SK_ReShade_DrawCallback.call (This, IndexCountPerInstance * InstanceCount * SK_D3D11_ReshadeDrawFactors.indexed_instanced, pTLS);

  D3D11_DrawIndexedInstanced_Original ( This, IndexCountPerInstance,
                                          InstanceCount, StartIndexLocation,
                                            BaseVertexLocation, StartInstanceLocation );

  SK_D3D11_PostDraw (pTLS);
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DrawIndexedInstancedIndirect_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ ID3D11Buffer        *pBufferForArgs,
  _In_ UINT                 AlignedByteOffsetForArgs )
{
  SK_LOG_FIRST_CALL

  SK_TLS *pTLS = nullptr;

  if (! (This->GetType () == D3D11_DEVICE_CONTEXT_DEFERRED && (! config.render.dxgi.deferred_isolation)))
    SK_ReShade_DrawCallback.call (This, SK_D3D11_ReshadeDrawFactors.indexed_instanced_indirect, pTLS);

  if (! SK_D3D11_ShouldTrackDrawCall (This, SK_D3D11DrawType::IndexedInstancedIndirect, &pTLS))
  {
    return
      D3D11_DrawIndexedInstancedIndirect_Original ( This, pBufferForArgs,
                                                      AlignedByteOffsetForArgs );
  }

  if (SK_D3D11_DrawHandler (This, pTLS))
    return;

  D3D11_DrawIndexedInstancedIndirect_Original ( This, pBufferForArgs,
                                                  AlignedByteOffsetForArgs );

  SK_D3D11_PostDraw (pTLS);
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DrawInstanced_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ UINT                 VertexCountPerInstance,
  _In_ UINT                 InstanceCount,
  _In_ UINT                 StartVertexLocation,
  _In_ UINT                 StartInstanceLocation )
{
  SK_LOG_FIRST_CALL

  SK_TLS *pTLS = nullptr;

  if (! SK_D3D11_ShouldTrackDrawCall (This, SK_D3D11DrawType::Instanced, &pTLS))
  {
    return
      D3D11_DrawInstanced_Original ( This, VertexCountPerInstance,
                                       InstanceCount, StartVertexLocation,
                                         StartInstanceLocation );
  }

  if (SK_D3D11_DrawHandler (This, pTLS))
    return;

  if (! (This->GetType () == D3D11_DEVICE_CONTEXT_DEFERRED && (! config.render.dxgi.deferred_isolation)))
    SK_ReShade_DrawCallback.call (This, VertexCountPerInstance * InstanceCount * SK_D3D11_ReshadeDrawFactors.instanced, pTLS);

  D3D11_DrawInstanced_Original ( This, VertexCountPerInstance,
                                   InstanceCount, StartVertexLocation,
                                     StartInstanceLocation );

  SK_D3D11_PostDraw (pTLS);
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DrawInstancedIndirect_Override (
  _In_ ID3D11DeviceContext *This,
  _In_ ID3D11Buffer        *pBufferForArgs,
  _In_ UINT                 AlignedByteOffsetForArgs )
{
  SK_LOG_FIRST_CALL

  SK_TLS *pTLS = nullptr;

  if (! SK_D3D11_ShouldTrackDrawCall (This, SK_D3D11DrawType::InstancedIndirect, &pTLS))
  {
    return
      D3D11_DrawInstancedIndirect_Original ( This, pBufferForArgs,
                                               AlignedByteOffsetForArgs );
  }

  if (SK_D3D11_DrawHandler (This, pTLS))
    return;

  if (! (This->GetType () == D3D11_DEVICE_CONTEXT_DEFERRED && (! config.render.dxgi.deferred_isolation)))
    SK_ReShade_DrawCallback.call (This, SK_D3D11_ReshadeDrawFactors.instanced_indirect, pTLS);

  D3D11_DrawInstancedIndirect_Original ( This, pBufferForArgs,
                                           AlignedByteOffsetForArgs );

  SK_D3D11_PostDraw (pTLS);
}


__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_Dispatch_Override ( _In_ ID3D11DeviceContext *This,
                          _In_ UINT                 ThreadGroupCountX,
                          _In_ UINT                 ThreadGroupCountY,
                          _In_ UINT                 ThreadGroupCountZ )
{
  SK_LOG_FIRST_CALL

  SK_TLS *pTLS = nullptr;

  if (! SK_D3D11_ShouldTrackRenderOp (This, &pTLS))
  {
    return
      D3D11_Dispatch_Original ( This,
                                  ThreadGroupCountX,
                                    ThreadGroupCountY,
                                      ThreadGroupCountZ );
  }

  if (SK_D3D11_DispatchHandler (This, pTLS))
    return;

  D3D11_Dispatch_Original ( This,
                              ThreadGroupCountX,
                                ThreadGroupCountY,
                                  ThreadGroupCountZ );

  SK_D3D11_PostDispatch (This, pTLS);
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_DispatchIndirect_Override ( _In_ ID3D11DeviceContext *This,
                                  _In_ ID3D11Buffer        *pBufferForArgs,
                                  _In_ UINT                 AlignedByteOffsetForArgs )
{
  SK_LOG_FIRST_CALL

  SK_TLS *pTLS = nullptr;

  if (! SK_D3D11_ShouldTrackRenderOp (This, &pTLS))
  {
    return
      D3D11_DispatchIndirect_Original ( This,
                                          pBufferForArgs,
                                            AlignedByteOffsetForArgs );
  }

  if (SK_D3D11_DispatchHandler (This, pTLS))
    return;

  if (! (This->GetType () == D3D11_DEVICE_CONTEXT_DEFERRED && (! config.render.dxgi.deferred_isolation)))
    SK_ReShade_DrawCallback.call (This, 64, pTLS);

  D3D11_DispatchIndirect_Original ( This,
                                      pBufferForArgs,
                                        AlignedByteOffsetForArgs );

  SK_D3D11_PostDispatch (This, pTLS);
}



__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_OMSetRenderTargets_Override (
         ID3D11DeviceContext           *This,
_In_     UINT                           NumViews,
_In_opt_ ID3D11RenderTargetView *const *ppRenderTargetViews,
_In_opt_ ID3D11DepthStencilView        *pDepthStencilView )
{
  ID3D11DepthStencilView *pDSV =
    pDepthStencilView;

  SK_TLS *pTLS = nullptr;

  UINT dev_idx =
    SK_D3D11_GetDeviceContextHandle (This);

  if (pDepthStencilView != nullptr && SK_ReShade_SetDepthStencilViewCallback.fn != nullptr)
  {
    if (!(This->GetType () == D3D11_DEVICE_CONTEXT_DEFERRED && (!config.render.dxgi.deferred_isolation)))
      SK_ReShade_SetDepthStencilViewCallback.call (pDSV, pTLS);
  }


  if (! SK_D3D11_ShouldTrackRenderOp (This, &pTLS))
  {
    if (tracked_rtv.active_count [dev_idx] > 0)
    {
      for (auto&& rtv : tracked_rtv.active [dev_idx]) rtv.store (false);

      tracked_rtv.active_count [dev_idx] = 0;
    }

    D3D11_OMSetRenderTargets_Original (
      This, NumViews, ppRenderTargetViews,
        pDSV );

    return;
  }

  D3D11_OMSetRenderTargets_Original (
    This, NumViews, ppRenderTargetViews,
      pDSV );

  if (NumViews > 0)
  {
    if (ppRenderTargetViews)
    {
      auto&                              rt_views =
        SK_D3D11_RenderTargets [dev_idx].rt_views;

      const auto* tracked_rtv_res  = 
        static_cast <ID3D11RenderTargetView *> (
          ReadPointerAcquire ((volatile PVOID *)&tracked_rtv.resource)
        );

      for (UINT i = 0; i < NumViews; i++)
      {
        if (ppRenderTargetViews [i] && rt_views.emplace (ppRenderTargetViews [i]).second)
            ppRenderTargetViews [i]->AddRef ();

        const bool active_before = tracked_rtv.active_count [dev_idx] > 0 ? 
                                   tracked_rtv.active       [dev_idx][i].load ()
                                                                    : false;

        const bool active = 
          ( tracked_rtv_res == ppRenderTargetViews [i] ) ?
                       true :
                              false;

        if (active_before != active)
        {
          tracked_rtv.active [dev_idx][i] = active;

          if      (            active                    ) tracked_rtv.active_count [dev_idx]++;
          else if (tracked_rtv.active_count [dev_idx] > 0) tracked_rtv.active_count [dev_idx]--;
        }
      }

      for ( UINT j = 0; j < 5 ; j++ )
      {
        switch (j)
        { case 0:
          {
            static auto& vertex =
              SK_D3D11_Shaders.vertex;

            INT  pre_hud_slot  = vertex.tracked.pre_hud_rt_slot;
            if ( pre_hud_slot >= 0 && pre_hud_slot < (INT)NumViews )
            {
              if (vertex.tracked.pre_hud_rtv == nullptr &&
                  vertex.current.shader [dev_idx] == 
                  vertex.tracked.crc32c.load () )
              {
                if (ppRenderTargetViews [pre_hud_slot] != nullptr)
                {
                  vertex.tracked.pre_hud_rtv = ppRenderTargetViews [pre_hud_slot];
                  vertex.tracked.pre_hud_rtv->AddRef ();
                }
              }
            }
          } break;

          case 1:
          {
            static auto& pixel =
              SK_D3D11_Shaders.pixel;

            INT  pre_hud_slot  = pixel.tracked.pre_hud_rt_slot;
            if ( pre_hud_slot >= 0 && pre_hud_slot < (INT)NumViews )
            {
              if (pixel.tracked.pre_hud_rtv == nullptr &&
                  pixel.current.shader [dev_idx] == 
                  pixel.tracked.crc32c.load () )
              {
                if (ppRenderTargetViews [pre_hud_slot] != nullptr)
                {
                  pixel.tracked.pre_hud_rtv = ppRenderTargetViews [pre_hud_slot];
                  pixel.tracked.pre_hud_rtv->AddRef ();
                }
              }
            }
          } break;

          default:
           break;
        }
      }
    }

    if (pDepthStencilView)
    {
      auto& ds_views =
        SK_D3D11_RenderTargets [dev_idx].ds_views;

      if (ds_views.emplace (pDepthStencilView).second)
                            pDepthStencilView->AddRef ();
    }
  }
}

//std::array <bool, SK_D3D11_MAX_DEV_CONTEXTS> SK_D3D11_KnownShaders::reshade_triggered;

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_OMSetRenderTargetsAndUnorderedAccessViews_Override ( ID3D11DeviceContext              *This,
                                            _In_           UINT                              NumRTVs,
                                            _In_opt_       ID3D11RenderTargetView    *const *ppRenderTargetViews,
                                            _In_opt_       ID3D11DepthStencilView           *pDepthStencilView,
                                            _In_           UINT                              UAVStartSlot,
                                            _In_           UINT                              NumUAVs,
                                            _In_opt_       ID3D11UnorderedAccessView *const *ppUnorderedAccessViews,
                                            _In_opt_ const UINT                             *pUAVInitialCounts )
{
  ID3D11DepthStencilView *pDSV = pDepthStencilView;

  SK_TLS *pTLS = nullptr;

  UINT dev_idx =
    SK_D3D11_GetDeviceContextHandle (This);

  if (pDepthStencilView != nullptr && SK_ReShade_SetDepthStencilViewCallback.fn != nullptr)
  {
    if (! (This->GetType () == D3D11_DEVICE_CONTEXT_DEFERRED && (! config.render.dxgi.deferred_isolation)))
      SK_ReShade_SetDepthStencilViewCallback.call (pDSV, pTLS);
  }

  // ImGui gets to pass-through without invoking the hook
  if (! SK_D3D11_ShouldTrackRenderOp (This, &pTLS))
  {
    for (auto&& i : tracked_rtv.active [dev_idx]) i.store (false);

    tracked_rtv.active_count [dev_idx] = 0;
  
    D3D11_OMSetRenderTargetsAndUnorderedAccessViews_Original (
      This, NumRTVs, ppRenderTargetViews,
        pDSV, UAVStartSlot, NumUAVs,
          ppUnorderedAccessViews, pUAVInitialCounts
    );

    return;
  }

  D3D11_OMSetRenderTargetsAndUnorderedAccessViews_Original (
    This, NumRTVs, ppRenderTargetViews,
      pDSV, UAVStartSlot, NumUAVs,
        ppUnorderedAccessViews, pUAVInitialCounts
  );

  if (NumRTVs > 0)
  {
    if (ppRenderTargetViews)
    {
      auto&                              rt_views =
        SK_D3D11_RenderTargets [dev_idx].rt_views;

      const auto* tracked_rtv_res = 
        static_cast <ID3D11RenderTargetView *> (
          ReadPointerAcquire ((volatile PVOID *)&tracked_rtv.resource)
        );

      for (UINT i = 0; i < NumRTVs; i++)
      {
        if (ppRenderTargetViews [i] && rt_views.emplace (ppRenderTargetViews [i]).second)
            ppRenderTargetViews [i]->AddRef ();

        const bool active_before = tracked_rtv.active_count [dev_idx] > 0 ? 
                                   tracked_rtv.active       [dev_idx][i].load ()
                                                                    : false;

        const bool active = 
          ( tracked_rtv_res == ppRenderTargetViews [i] ) ?
                       true :
                              false;

        if (active_before != active)
        {
          tracked_rtv.active [dev_idx][i] = active;

          if      (            active                    ) tracked_rtv.active_count [dev_idx]++;
          else if (tracked_rtv.active_count [dev_idx] > 0) tracked_rtv.active_count [dev_idx]--;
        }
      }
    }

    if (pDepthStencilView)
    {
      auto& ds_views =
        SK_D3D11_RenderTargets [dev_idx].ds_views;

      if (! ds_views.count (pDepthStencilView))
      {
                         pDepthStencilView->AddRef ();
        ds_views.insert (pDepthStencilView);
      }
    }
  }
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_OMGetRenderTargets_Override (ID3D11DeviceContext     *This,
                              _In_ UINT                     NumViews,
                             _Out_ ID3D11RenderTargetView **ppRenderTargetViews,
                             _Out_ ID3D11DepthStencilView **ppDepthStencilView)
{
  D3D11_OMGetRenderTargets_Original (This, NumViews, ppRenderTargetViews, ppDepthStencilView);

  if (ppDepthStencilView != nullptr && SK_ReShade_GetDepthStencilViewCallback.fn != nullptr)
  {
    if (!(This->GetType () == D3D11_DEVICE_CONTEXT_DEFERRED && (!config.render.dxgi.deferred_isolation)))
      SK_ReShade_GetDepthStencilViewCallback.try_call (*ppDepthStencilView);
  }
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_OMGetRenderTargetsAndUnorderedAccessViews_Override (ID3D11DeviceContext        *This,
                                                    _In_  UINT                        NumRTVs,
                                                    _Out_ ID3D11RenderTargetView    **ppRenderTargetViews,
                                                    _Out_ ID3D11DepthStencilView    **ppDepthStencilView,
                                                    _In_  UINT                        UAVStartSlot,
                                                    _In_  UINT                        NumUAVs,
                                                    _Out_ ID3D11UnorderedAccessView **ppUnorderedAccessViews)
{
  D3D11_OMGetRenderTargetsAndUnorderedAccessViews_Original (This, NumRTVs, ppRenderTargetViews, ppDepthStencilView, UAVStartSlot, NumUAVs, ppUnorderedAccessViews);

  if (ppDepthStencilView != nullptr && SK_ReShade_GetDepthStencilViewCallback.fn != nullptr) 
    SK_ReShade_GetDepthStencilViewCallback.try_call (*ppDepthStencilView);
}

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_ClearDepthStencilView_Override (ID3D11DeviceContext    *This,
                                 _In_ ID3D11DepthStencilView *pDepthStencilView,
                                 _In_ UINT                    ClearFlags,
                                 _In_ FLOAT                   Depth,
                                 _In_ UINT8                   Stencil)
{
  if (SK_ReShade_ClearDepthStencilViewCallback.fn != nullptr)
    SK_ReShade_ClearDepthStencilViewCallback.try_call (pDepthStencilView);

  D3D11_ClearDepthStencilView_Original (This, pDepthStencilView, ClearFlags, Depth, Stencil);
}

extern bool __SK_Y0_1024_512;
extern bool __SK_Y0_1024_768;
extern bool __SK_Y0_960_540;

__declspec (noinline)
void
STDMETHODCALLTYPE
D3D11_RSSetViewports_Override (
        ID3D11DeviceContext* This,
        UINT                 NumViewports,
  const D3D11_VIEWPORT*      pViewports )
{
#if 0
  if (NumViewports > 0)
  {
    D3D11_VIEWPORT* vps = new D3D11_VIEWPORT [NumViewports];
    for (UINT i = 0; i < NumViewports; i++)
    {
      vps [i] = pViewports [i];

      if (! (config.render.dxgi.res.max.isZero () || config.render.dxgi.res.min.isZero ()))
      {
        vps [i].Height /= ((float)config.render.dxgi.res.min.y/(float)config.render.dxgi.res.max.y);
        vps [i].Width  /= ((float)config.render.dxgi.res.min.x/(float)config.render.dxgi.res.max.x);
      }
    }

    D3D11_RSSetViewports_Original (This, NumViewports, vps);

    delete [] vps;

    return;
  }

  else
#endif
  D3D11_RSSetViewports_Original (This, NumViewports, pViewports);
}

void WINAPI SK_D3D11_SetResourceRoot      (const wchar_t* root)  ;
void WINAPI SK_D3D11_EnableTexDump        (bool enable)          ;
void WINAPI SK_D3D11_EnableTexInject      (bool enable)          ;
void WINAPI SK_D3D11_EnableTexCache       (bool enable)          ;
void WINAPI SK_D3D11_PopulateResourceList (bool refresh = false) ;

#include <unordered_set>
#include <unordered_map>
#include <map>

__forceinline
SK_D3D11_TexMgr_Singleton&
__SK_Singleton_D3D11_Textures (void)
{
  static SK_D3D11_TexMgr _SK_D3D11_Textures;
  return                 _SK_D3D11_Textures;
}


class SK_D3D11_TexCacheMgr {
public:
};


std::string
SK_D3D11_SummarizeTexCache (void)
{
  char szOut [512] { };

  static SK_D3D11_TexMgr& textures =
    SK_D3D11_Textures;

  snprintf ( szOut, 511, "  Tex Cache: %#5llu MiB   Entries:   %#7li\n"
                         "  Hits:      %#5li       Time Saved: %#7.01lf ms\n"
                         "  Evictions: %#5lu",
               textures.AggregateSize_2D >> 20ULL,
               textures.Entries_2D.load        (),
               textures.RedundantLoads_2D.load (),
               textures.RedundantTime_2D,
               textures.Evicted_2D.load        () );

  return std::move (szOut);
}


bool
SK_D3D11_TexMgr::isTexture2D (uint32_t crc32, const D3D11_TEXTURE2D_DESC *pDesc)
{
  if (! (SK_D3D11_cache_textures && crc32))
    return false;

  return HashMap_2D [pDesc->MipLevels].contains (crc32);
}

void
__stdcall
SK_D3D11_ResetTexCache (void)
{
  extern void
    SK_D3D11_ProcessScreenshotQueueEx ( int  stage = 2,
                                        bool wait  = false,
                                        bool purge = false );

  SK_D3D11_ProcessScreenshotQueueEx (3, false, true);

//  SK_D3D11_need_tex_reset = true;
  SK_D3D11_try_tex_reset = true;
  SK_D3D11_Textures.reset ();
}


static volatile ULONG live_textures_dirty = FALSE;

void
__stdcall
SK_D3D11_TexCacheCheckpoint (void)
{
  static SK_D3D11_TexMgr& textures =
    SK_D3D11_Textures;

  static const auto& rb =
    SK_GetCurrentRenderBackend ();

  static auto& Entries_2D  = textures.Entries_2D;
  static auto& Evicted_2D  = textures.Evicted_2D;
  static auto& Textures_2D = textures.Textures_2D;

  CComQIPtr <ID3D11Device> pDevice  (rb.device);
  CComQIPtr <IDXGIDevice>  pDXGIDev (pDevice.p);

  if ( config.textures.cache.residency_managemnt &&
       pDevice                                   &&
       pDXGIDev )
  {
    static DWORD dwLastEvict = 0;
    static DWORD dwLastTest  = 0;
    static DWORD dwLastSize  = Entries_2D.load ();

    static DWORD dwInitiateEvict = 0;
    static DWORD dwInitiateSize  = 0;

    const DWORD dwNow = timeGetTime ();

    const int MAX_TEXTURES_PER_PASS = 32UL;

    static size_t cur_tex = 0;

    if ( ( Evicted_2D.load () != dwLastEvict || 
           Entries_2D.load () != dwLastSize     ) &&
                      dwLastTest < dwNow - 666L )
    {
      if (cur_tex == 0)
      {
        dwInitiateEvict = Evicted_2D.load ();
        dwInitiateSize  = Entries_2D.load ();
      }

      static LONG fully_resident = 0;
      static LONG shared_memory  = 0;
      static LONG on_disk        = 0;

      static LONG64 size_vram   = 0ULL;
      static LONG64 size_shared = 0ULL;
      static LONG64 size_disk   = 0ULL;

      static std::array <IUnknown *,                           MAX_TEXTURES_PER_PASS + 2> residency_tests;
      static std::array <SK_D3D11_TexMgr::tex2D_descriptor_s*, MAX_TEXTURES_PER_PASS + 2> residency_descs;
      static std::array <DXGI_RESIDENCY, MAX_TEXTURES_PER_PASS + 2> residency_results;

      size_t record_count = 0;

      auto record =
        residency_tests.begin ();
      auto desc =
        residency_descs.begin ();

      const size_t start_idx = cur_tex;
            size_t idx       = 0;
      const size_t max_size  = Textures_2D.size ();

      bool done = false;

      for ( auto& tex : Textures_2D )
      {
        if (++idx < cur_tex)
          continue;

        if (idx > start_idx + MAX_TEXTURES_PER_PASS)
        {
          cur_tex = idx;
          break;
        }

        if (tex.second.crc32c != 0x0 && tex.first != nullptr)
        {
          *(record++) =  tex.first;
          *(desc++)   = &tex.second;
                      ++record_count;
        }
      }

      if (idx >= max_size)
        done = true;

      pDXGIDev->QueryResourceResidency (
        residency_tests.data   (),
        residency_results.data (),
                                   static_cast <UINT> (record_count)
      );

      idx = 0;

      desc = residency_descs.begin ();

      for ( auto it : residency_results )
      {
        // Handle uninit. and corrupted texture entries
        if ( it < DXGI_RESIDENCY_FULLY_RESIDENT ||
             it > DXGI_RESIDENCY_EVICTED_TO_DISK )
          continue;

        if (it != DXGI_RESIDENCY_FULLY_RESIDENT)
        {
          SK_LOG1 ( (L"Texture %x is non-resident, last use: %lu <Residence: %lu>",  (*desc)->crc32c, (*desc)->last_used, it),
                     L"DXGI Cache" );

          const D3D11_TEXTURE2D_DESC* tex_desc =
                               &(*desc)->desc;

          if ((*desc)->texture != nullptr)
          {
            const UINT refs_plus_1 = (*desc)->texture->AddRef  ();
            const UINT refs        = (*desc)->texture->Release ();

            SK_LOG1 ( ( L"(%lux%lu@%lu [%s] - %s, %s, %s : CPU Usage=%x -- refs+1=%lu, refs=%lu",
                        tex_desc->Width, tex_desc->Height, tex_desc->MipLevels,
                        SK_DXGI_FormatToStr (tex_desc->Format).c_str (),
                        SK_D3D11_DescribeBindFlags ((D3D11_BIND_FLAG)tex_desc->BindFlags).c_str (), 
                        SK_D3D11_DescribeMiscFlags ((D3D11_RESOURCE_MISC_FLAG)tex_desc->MiscFlags).c_str (),
                        SK_D3D11_DescribeUsage     (tex_desc->Usage),
                        (UINT)tex_desc->CPUAccessFlags,
                        refs_plus_1, refs ),
                       L"DXGI Cache" );

            // If this texture is _NOT_ injected and also not resident in VRAM, then
            //   remove it from cache.
            //
            //  If it is injected, leave it loaded because this cache's purpose is to prevent
            //    re-loading injected textures.
            if (refs == 1 && (! (*desc)->injected))
              (*desc)->texture->Release ();
          }
        }

        if (it == DXGI_RESIDENCY_FULLY_RESIDENT)            { ++fully_resident; size_vram   += (*(desc))->mem_size; }
        if (it == DXGI_RESIDENCY_RESIDENT_IN_SHARED_MEMORY) { ++shared_memory;  size_shared += (*(desc))->mem_size; }
        if (it == DXGI_RESIDENCY_EVICTED_TO_DISK)           { ++on_disk;        size_disk   += (*(desc))->mem_size; }

        ++desc;

        if (++idx >= record_count)
          break;
      }

      dwLastTest = dwNow;

      if (done)
      {
        InterlockedExchange   (&SK_D3D11_TexCacheResidency.count.InVRAM,   fully_resident);
        InterlockedExchange   (&SK_D3D11_TexCacheResidency.count.Shared,   shared_memory);
        InterlockedExchange   (&SK_D3D11_TexCacheResidency.count.PagedOut, on_disk);

        InterlockedExchange64 (&SK_D3D11_TexCacheResidency.size.InVRAM,   size_vram);
        InterlockedExchange64 (&SK_D3D11_TexCacheResidency.size.Shared,   size_shared);
        InterlockedExchange64 (&SK_D3D11_TexCacheResidency.size.PagedOut, size_disk);

        textures.AggregateSize_2D = size_vram      + size_shared + size_disk;
        textures.Entries_2D       = fully_resident + shared_memory + on_disk;

        fully_resident = 0;
        shared_memory  = 0;
        on_disk        = 0;

        size_vram   = 0ULL;
        size_shared = 0ULL;
        size_disk   = 0ULL;

        dwLastEvict = dwInitiateEvict;
        dwLastSize  = dwInitiateSize;

        cur_tex = 0;
      }
    }
  }



  static int       iter               = 0;

  static bool      init               = false;
  static ULONGLONG ullMemoryTotal_KiB = 0;
  static HANDLE    hProc              = nullptr;

  if (! init)
  {
    init  = true;
    hProc = SK_GetCurrentProcess ();

    GetPhysicallyInstalledSystemMemory (&ullMemoryTotal_KiB);
  }

  ++iter;

  bool reset =
    SK_D3D11_need_tex_reset || SK_D3D11_try_tex_reset;

  static PROCESS_MEMORY_COUNTERS pmc = {   };

  const bool has_non_zero_reserve =
    config.mem.reserve > 0.0f;

  if (! (iter % 5))
  {
                                  pmc.cb = sizeof pmc;
    GetProcessMemoryInfo (hProc, &pmc,     sizeof pmc);

    reset |=
      (          (textures.AggregateSize_2D >> 20ULL) > (uint64_t)cache_opts.max_size    ||
                  textures.Entries_2D                 >           cache_opts.max_entries ||
       ( has_non_zero_reserve && ((config.mem.reserve / 100.0f) * ullMemoryTotal_KiB)
                                                      < (pmc.PagefileUsage >> 10UL)
       )
      );
  }

  if (reset)
  {
    //dll_log.Log (L"[DX11TexMgr] DXGI 1.4 Budget Change: Triggered a texture manager purge...");

    SK_D3D11_amount_to_purge =
      has_non_zero_reserve   ? 
      static_cast <int32_t> (
        (pmc.PagefileUsage >> 10UL) - (float)(ullMemoryTotal_KiB) *
                       (config.mem.reserve / 100.0f)
      )                      : 0;

    textures.reset ();
  }

  else
  {
    //SK_RenderBackend& rb =
    //  SK_GetCurrentRenderBackend ();

    //if (rb.d3d11.immediate_ctx != nullptr)
    //{
    //  SK_D3D11_PreLoadTextures ();
    //}
  }
}

void
SK_D3D11_TexMgr::reset (void)
{
  if (IUnknown_AddRef_Original == nullptr || IUnknown_Release_Original == nullptr)
    return;

  if (SK_GetFramesDrawn () < 1)
    return;


  uint32_t count  = 0;
  int64_t  purged = 0;


  bool must_free =
    SK_D3D11_need_tex_reset;

  SK_D3D11_need_tex_reset = false;


  const LONGLONG time_now =
    SK_QueryPerf ().QuadPart;

  // Additional conditions that may trigger a purge
  if (! must_free)
  {
    // Throttle to at most one potentially unnecessary purge attempt per-ten seconds
    if ( LastModified_2D <=  LastPurge_2D &&
         time_now        < ( LastPurge_2D + ( PerfFreq.QuadPart * 10LL ) ) )
    {
      SK_D3D11_try_tex_reset = true;
      return;
    }


    const float overfill_factor = 1.05f;

    bool no_work = true;


    if ( (uint32_t)AggregateSize_2D >> 20ULL > ( (uint32_t)       cache_opts.max_size +
                                                ((uint32_t)(float)cache_opts.max_size * overfill_factor)) )
      no_work = false;

    if ( SK_D3D11_Textures.Entries_2D > (LONG)       cache_opts.max_entries +
                                        (LONG)(float)cache_opts.max_entries * overfill_factor )
      no_work = false;

    if ( SK_D3D11_amount_to_purge > 0 )
      no_work = false;

    if (no_work) return;
  }


  // Static analysis = anything testing this always takes the same path
  //                     that cannot possibly be intended; re-visit this.
  must_free = false;


  std::set    <ID3D11Texture2D *>                     cleared;// (cache_opts.max_evict);
  std::vector <SK_D3D11_TexMgr::tex2D_descriptor_s *> textures;

  int potential = 0;
  {
    SK_AutoCriticalSection critical (&cache_cs);
    {
      textures.reserve  (Textures_2D.size ());

      for ( auto& desc : Textures_2D )
      {
        if (desc.second.texture == nullptr || desc.second.crc32c == 0x00)
          continue;

        bool can_free = must_free;

        if (! must_free)
          can_free = (IUnknown_AddRef_Original (desc.second.texture) <= 2);

        if (can_free)
        {
          ++potential;
            textures.emplace_back (&desc.second);
        }

        if (! must_free)
          IUnknown_Release_Original (desc.second.texture);
      }
    }

    if (potential > 0)
    {
      std::sort ( textures.begin (),
                    textures.end (),
         [&]( const SK_D3D11_TexMgr::tex2D_descriptor_s* const a,
              const SK_D3D11_TexMgr::tex2D_descriptor_s* const b )
        {
          return ( b->load_time * 10ULL  +  b->last_used / 10ULL  + 
                    static_cast <uint64_t> (b->hits)     * 100ULL   )   <
                 ( a->load_time * 10ULL  +  a->last_used / 10ULL  +
                    static_cast <uint64_t> (a->hits)     * 100ULL   );
        }
      );
    }
  }

  if (potential == 0)
    return;

  dll_log.Log (L"Reset potential = %lu / %lu", potential, textures.size ());

  const uint32_t max_count =
   cache_opts.max_evict;
 
  SK_AutoCriticalSection critical (&cache_cs);

  for ( const auto& desc : textures )
  {
    const auto mem_size =
     static_cast <int64_t> (desc->mem_size) >> 10ULL;
 
   if (desc->texture != nullptr && cleared.count (desc->texture) == 0)
   {
     const int refs =
       desc->texture->AddRef  () - 1;
       desc->texture->Release ();
 
     if (refs <= 1 || must_free)
     {
       // Avoid double-freeing if the hash map somehow contains the same texture multiple times
       cleared.emplace (desc->texture);

       if (must_free && refs != 1)
       {
         SK_LOG0 ( ( L"Unexpected reference count for texture with crc32=%08x; refs=%lu, expected=1 -- removing from cache and praying...",
                       desc->crc32c, refs ),
                    L"DX11TexMgr" );
       }

       SK_D3D11_RemoveTexFromCache (desc->texture);
         IUnknown_Release_Original (desc->texture);
 
       count++;
       purged += mem_size;
 
       if ( ! must_free )
       {
         if ((// Have we over-freed? If so, stop when the minimum number of evicted textures is reached
              (AggregateSize_2D >> 20ULL) <= (uint32_t)cache_opts.min_size &&
                                    count >=           cache_opts.min_evict )
         
              // An arbitrary purge request was issued
                                                                                               ||
             (SK_D3D11_amount_to_purge     >            0                   &&
              SK_D3D11_amount_to_purge     <=           purged              &&
                                     count >=           cache_opts.min_evict ) ||
         
              // Have we evicted as many textures as we can in one pass?
                                     count >=           max_count )
         {
           SK_D3D11_amount_to_purge =
             std::max (0, SK_D3D11_amount_to_purge);
         
           break;
         }
       }
     }
   }
 }
 
 LastPurge_2D.store (time_now);

 if (count > 0)
 {
   SK_D3D11_try_tex_reset = false;

   SK_LOG0 ( ( L"Texture Cache Purge:  Eliminated %.2f MiB of texture data across %lu textures (max. evict per-pass: %lu).",
                 static_cast <float> (purged) / 1024.0f,
                                      count,
                                      cache_opts.max_evict ),
               L"DX11TexMgr" );
 
   if ((AggregateSize_2D >> 20ULL) >= static_cast <uint64_t> (cache_opts.max_size))
   {
     SK_LOG0 ( ( L" >> Texture cache remains %.2f MiB over-budget; will schedule future passes until resolved.",
                   static_cast <double> ( AggregateSize_2D ) / (1024.0 * 1024.0) -
                   static_cast <double> (cache_opts.max_size) ),
                 L"DX11TexMgr" );

     SK_D3D11_try_tex_reset = true;
   }
 
   if (Entries_2D >= cache_opts.max_entries)
   {
     SK_LOG0 ( ( L" >> Texture cache remains %lu entries over-filled; will schedule future passes until resolved.",
                   Entries_2D - cache_opts.max_entries ),
                 L"DX11TexMgr" );

     SK_D3D11_try_tex_reset = true;
   }
 
   InterlockedExchange (&live_textures_dirty, TRUE);
 }
}

LONG
SK_D3D11_TexMgr::recordCacheHit ( ID3D11Texture2D *pTex )
{
  static auto& textures =
    SK_D3D11_Textures;

  const auto& tex2D =
    textures.Textures_2D.find (pTex);

  if ( tex2D                != textures.Textures_2D.cend () &&
       tex2D->second.crc32c != 0x00 )
  {
    auto& desc2d =
      tex2D->second;

    SK_TLS *pTLS =
      SK_TLS_Bottom ();

    // Don't record cache hits caused by the shader mod interface
    if (! pTLS->imgui.drawing)
    {
      desc2d.last_used =
        SK_QueryPerf ().QuadPart;

      return
        InterlockedIncrement (&desc2d.hits);
    }
  }

  return 0;
}

ID3D11Texture2D*
SK_D3D11_TexMgr::getTexture2D ( uint32_t              tag,
                          const D3D11_TEXTURE2D_DESC* pDesc,
                                size_t*               pMemSize,
                                float*                pTimeSaved,
                                SK_TLS*               pTLS )
{
  ID3D11Texture2D* pTex2D =
    nullptr;

  if (isTexture2D (tag, pDesc))
  {
    ID3D11Texture2D*    it =     HashMap_2D [pDesc->MipLevels][tag];
    tex2D_descriptor_s& desc2d (Textures_2D [it]);

    // We use a lockless concurrent hashmap, which makes removal
    //   of key/values an unsafe operation.
    //
    //   Thus, crc32c == 0x0 signals a key that exists in the
    //     map, but has no live association with any real data.
    //
    //     Zombie, in other words.
    //
    const bool zombie =
      ( desc2d.crc32c == 0x00 );

    if ( desc2d.crc32c != 0x00 &&
         desc2d.tag    == tag  &&
      (! desc2d.discard) )
    {
      pTex2D = desc2d.texture;
      pTex2D->AddRef ();

      const size_t  size = desc2d.mem_size;
      const float  fTime = static_cast <float> (desc2d.load_time ) * 1000.0f /
                           static_cast <float> (PerfFreq.QuadPart);

      if (pMemSize)   *pMemSize   = size;
      if (pTimeSaved) *pTimeSaved = fTime;

      desc2d.last_used =
        SK_QueryPerf ().QuadPart;

      if (pTLS == nullptr)
          pTLS = SK_TLS_Bottom ();

      // Don't record cache hits caused by the shader mod interface
      if (! pTLS->imgui.drawing)
      {
        InterlockedIncrement (&desc2d.hits);

        RedundantData_2D += gsl::narrow_cast <LONG64> (size);
        RedundantLoads_2D++;
        RedundantTime_2D += fTime;
      }
    }

    else
    {
      if (! zombie)
      {
        SK_LOG0 ( ( L"Cached texture (tag=%x) found in hash table, but not in texture map", tag),
                    L"DX11TexMgr" );
      }

      HashMap_2D  [pDesc->MipLevels].erase (tag);
      Textures_2D [it].crc32c = 0x00;
    }
  }

  return pTex2D;
}

bool
__stdcall
SK_D3D11_TextureIsCached (ID3D11Texture2D* pTex)
{
  if (! SK_D3D11_cache_textures)
    return false;

  static auto& textures =
    SK_D3D11_Textures;

  const auto& it =
    textures.Textures_2D.find (pTex);

  return ( it                != textures.Textures_2D.cend () &&
           it->second.crc32c != 0x00 );
}

uint32_t
__stdcall
SK_D3D11_TextureHashFromCache (ID3D11Texture2D* pTex)
{
  if (! SK_D3D11_cache_textures)
    return 0x00;

  static auto& textures =
    SK_D3D11_Textures;

  const auto& it =
    textures.Textures_2D.find (pTex);

  if ( it != textures.Textures_2D.cend () )
    return it->second.crc32c;

  return 0x00;
}

void
__stdcall
SK_D3D11_UseTexture (ID3D11Texture2D* pTex)
{
  if (! SK_D3D11_cache_textures)
    return;

  if (SK_D3D11_TextureIsCached (pTex))
  {
    SK_D3D11_Textures.Textures_2D [pTex].last_used =
      SK_QueryPerf ().QuadPart;
  }
}

bool
__stdcall
SK_D3D11_RemoveTexFromCache (ID3D11Texture2D* pTex, bool blacklist)
{
  //SK_AutoCriticalSection critical (&cache_cs);

  if (! SK_D3D11_TextureIsCached (pTex))
    return false;

  static auto& textures =
    SK_D3D11_Textures;

  if (pTex != nullptr)
  {
    SK_D3D11_TexMgr::tex2D_descriptor_s& it =
      textures.Textures_2D [pTex];

          uint32_t              tag  = it.tag;
    const D3D11_TEXTURE2D_DESC& desc = it.orig_desc;

    textures.AggregateSize_2D -=
      it.mem_size;
      it.crc32c = 0x00;

  //SK_D3D11_Textures.Textures_2D.erase (pTex);
    textures.Evicted_2D++;
  //SK_D3D11_Textures.TexRefs_2D.erase (pTex);

    textures.Entries_2D--;

    if (blacklist)
    {
      textures.Blacklist_2D [desc.MipLevels].emplace (tag);
      pTex->Release ();
    }
    else
      textures.HashMap_2D   [desc.MipLevels].erase   (tag);

    live_textures_dirty = true;
  }
   
  // Lightweight signal that something changed and a purge may be needed
  textures.LastModified_2D = SK_QueryPerf ().QuadPart;

  return true;
}

void
SK_D3D11_TexMgr::refTexture2D ( ID3D11Texture2D*      pTex,
                          const D3D11_TEXTURE2D_DESC *pDesc,
                                uint32_t              tag,
                                size_t                mem_size,
                                uint64_t              load_time,
                                uint32_t              crc32c,
                          const wchar_t              *fileName,
                          const D3D11_TEXTURE2D_DESC *pOrigDesc,
                       _In_opt_ HMODULE              /*hModCaller*/,
                       _In_opt_ SK_TLS               *pTLS )
{
  if (! SK_D3D11_cache_textures)
    return;

  static volatile LONG init = FALSE;

  if (! InterlockedCompareExchange (&init, TRUE, FALSE))
  {
    DXGI_VIRTUAL_HOOK ( &pTex, 2, "IUnknown::Release",
                        IUnknown_Release,
                        IUnknown_Release_Original,
                        IUnknown_Release_pfn );
    DXGI_VIRTUAL_HOOK ( &pTex, 1, "IUnknown::AddRef",
                        IUnknown_AddRef,
                        IUnknown_AddRef_Original,
                        IUnknown_AddRef_pfn );

    SK_ApplyQueuedHooks ();

    InterlockedIncrement (&init);
  }

  SK_Thread_SpinUntilAtomicMin (&init, 2);


  if (pTex == nullptr || tag == 0x00)
	  return;



  if (SK_D3D11_TestRefCountHooks (pTex, pTLS))
  {
    SK_LOG2 ( (L"Cached texture (%x)",
                  crc32c ),
               L"DX11TexMgr" );
  }

  else
  {
    SK_LOG0 ( (L"Potentially cacheable texture (%x) is not correctly reference counted; opting out!",
                  crc32c ),
               L"DX11TexMgr" );
    return;
  }


  ///if (pDesc->Usage >= D3D11_USAGE_DYNAMIC)
  ///{
  ///  dll_log.Log ( L"[DX11TexMgr] Texture '%08X' Is Not Cacheable "
  ///                L"Due To Usage: %lu",
  ///                crc32c, pDesc->Usage );
  ///  return;
  ///}

  ///if (pDesc->CPUAccessFlags & D3D11_CPU_ACCESS_WRITE)
  ///{
  ///  dll_log.Log ( L"[DX11TexMgr] Texture '%08X' Is Not Cacheable "
  ///                L"Due To CPUAccessFlags: 0x%X",
  ///                crc32c, pDesc->CPUAccessFlags );
  ///  return;
  ///}

  tex2D_descriptor_s  null_desc = { };
  tex2D_descriptor_s&   texDesc = null_desc;

  if (SK_D3D11_TextureIsCached (pTex) && (! (texDesc = Textures_2D [pTex]).discard))
  {
    // If we are updating once per-frame, then remove the freaking texture :)
    if (HashMap_2D [texDesc.orig_desc.MipLevels].contains (texDesc.tag))
    {
      if (texDesc.last_frame > SK_GetFramesDrawn () - 3)
      {
                                                                           texDesc.discard = true;
        pTex->SetPrivateData (SKID_D3D11Texture2D_DISCARD, sizeof (bool), &texDesc.discard);
      }

      //// This texture is too dynamic, it's best to just cut all ties to it,
      ////   don't try to keep updating the hash because that may kill performance.
      ////////HashMap_2D [texDesc.orig_desc.MipLevels].erase (texDesc.tag);
    }

    texDesc.crc32c     = crc32c;
    texDesc.tag        = tag;
    texDesc.last_used  = SK_QueryPerf      ().QuadPart;
    texDesc.last_frame = SK_GetFramesDrawn ();

    ///InterlockedIncrement (&texDesc.hits);

    dll_log.Log (L"[DX11TexMgr] Texture is already cached?!  { Original: %x, New: %x }",
                   Textures_2D [pTex].crc32c, crc32c );
    return;
  }

  if (texDesc.discard)
  {
    dll_log.Log (L"[DX11TexMgr] Texture was cached, but marked as discard... ignoring" );
    return;
  }


  AggregateSize_2D += mem_size;

  tex2D_descriptor_s desc2d 
                    = { };

  desc2d.desc       = *pDesc;
  desc2d.orig_desc  =  pOrigDesc != nullptr ?
                      *pOrigDesc : *pDesc;
  desc2d.texture    =  pTex;
  desc2d.load_time  =  load_time;
  desc2d.mem_size   =  mem_size;
  desc2d.tag        =  tag;
  desc2d.crc32c     =  crc32c;
  desc2d.last_used  =  SK_QueryPerf      ().QuadPart;
  desc2d.last_frame =  SK_GetFramesDrawn ();
  desc2d.file_name  = fileName;


  if (desc2d.orig_desc.MipLevels >= 18)
  {
    SK_LOG0 ( ( L"Too many mipmap LODs to cache (%lu), will not cache '%x'",
                  desc2d.orig_desc.MipLevels,
                    desc2d.crc32c ),
                L"DX11TexMgr" );
  }

  SK_LOG4 ( ( L"Referencing Texture '%x' with %lu mipmap levels :: (%08" PRIxPTR L"h)",
                desc2d.crc32c,
                  desc2d.orig_desc.MipLevels,
                    (uintptr_t)pTex ),
              L"DX11TexMgr" );

  SK_LOG4 ( ( L"  >> (%ux%u:%u) [CPU Access: %x], Misc Flags: %x, Usage: %u, Bind Flags: %x",
                desc2d.orig_desc.Width,     desc2d.orig_desc.Height,
                desc2d.orig_desc.MipLevels, desc2d.orig_desc.CPUAccessFlags,
                desc2d.orig_desc.MiscFlags, desc2d.orig_desc.Usage,
                desc2d.orig_desc.BindFlags ),
              L"DX11TexMgr" );

  HashMap_2D [desc2d.orig_desc.MipLevels][tag] = pTex;
  Textures_2D.insert            (std::make_pair (pTex, desc2d));
  TexRefs_2D.insert             (                pTex);

  Entries_2D++;

  // Hold a reference ourselves so that the game cannot free it
  pTex->AddRef ();


  // Lightweight signal that something changed and a purge may be needed
  LastModified_2D = SK_QueryPerf ().QuadPart;
}

#include <Shlwapi.h>

std::unordered_map < std::wstring, uint32_t > SK_D3D11_EnumeratedMipmapCache;
                                     uint64_t SK_D3D11_MipmapCacheSize;
void
SK_D3D11_RecursiveEnumAndAddTex ( std::wstring   directory, unsigned int& files,
                                  LARGE_INTEGER& liSize,    wchar_t*      wszPattern = (wchar_t *)L"*" );

void
WINAPI
SK_D3D11_PopulateResourceList (bool refresh)
{
  static bool init = false;

  if (((! refresh) && init) || SK_D3D11_res_root.empty ())
    return;

  SK_AutoCriticalSection critical0 (&tex_cs);
  SK_AutoCriticalSection critical1 (&inject_cs);

  static auto& textures =
    SK_D3D11_Textures;

  if (refresh)
  {
    textures.dumped_textures.clear ();
    textures.dumped_texture_bytes = 0;

    textures.injectable_textures.clear ();
    textures.injectable_texture_bytes = 0;

    SK_D3D11_EnumeratedMipmapCache.clear ();
               SK_D3D11_MipmapCacheSize = 0;
  }

  init = true;

  wchar_t wszTexDumpDir_RAW [ MAX_PATH     ] = { };
  wchar_t wszTexDumpDir     [ MAX_PATH + 2 ] = { };

  lstrcatW (wszTexDumpDir_RAW, SK_D3D11_res_root.c_str ());
  lstrcatW (wszTexDumpDir_RAW, LR"(\dump\textures\)");
  lstrcatW (wszTexDumpDir_RAW, SK_GetHostApp ());

  wcscpy ( wszTexDumpDir,
             SK_EvalEnvironmentVars (wszTexDumpDir_RAW).c_str () );

  //
  // Walk custom textures so we don't have to query the filesystem on every
  //   texture load to check if a custom one exists.
  //
  if ( GetFileAttributesW (wszTexDumpDir) !=
         INVALID_FILE_ATTRIBUTES )
  {
    WIN32_FIND_DATA fd     = {  };
    HANDLE          hFind  = INVALID_HANDLE_VALUE;
    unsigned int    files  =  0UL;
    LARGE_INTEGER   liSize = {  };

    LARGE_INTEGER   liCompressed   = {   };
    LARGE_INTEGER   liUncompressed = {   };

    dll_log.LogEx ( true, L"[DX11TexMgr] Enumerating dumped...    " );

    lstrcatW (wszTexDumpDir, LR"(\*)");

    hFind = FindFirstFileW (wszTexDumpDir, &fd);

    if (hFind != INVALID_HANDLE_VALUE)
    {
      do
      {
        if (fd.dwFileAttributes != INVALID_FILE_ATTRIBUTES)
        {
          // Dumped Metadata has the extension .dds.txt, do not
          //   include these while scanning for textures.
          if (    StrStrIW (fd.cFileName, L".dds")    &&
               (! StrStrIW (fd.cFileName, L".dds.txt") ) )
          {
            uint32_t top_crc32 = 0x00;
            uint32_t checksum  = 0x00;

            bool compressed = false;

            if (StrStrIW (fd.cFileName, L"Uncompressed_"))
            {
              wchar_t *wszFound =
                StrStrIW (fd.cFileName, L"_");

              if (wszFound && StrStrIW (wszFound + 1, L"_"))
              {
                swscanf ( fd.cFileName,
                            L"Uncompressed_%08X_%08X.dds",
                              &top_crc32,
                                &checksum );
              }

              else
              {
                swscanf ( fd.cFileName,
                            L"Uncompressed_%08X.dds",
                              &top_crc32 );
                checksum = 0x00;
              }
            }

            else
            { if (StrStrIW (fd.cFileName, L"_"))
              {
                wchar_t* wszFound =
                  StrStrIW (fd.cFileName, L"_");

                if (wszFound != nullptr && StrStrIW (wszFound + 1, L"_"))
                {
                  swscanf ( fd.cFileName,
                              L"Compressed_%08X_%08X.dds",
                                &top_crc32,
                                  &checksum );
                }
                  
                else
                {
                  swscanf ( fd.cFileName,
                              L"Compressed_%08X.dds",
                                &top_crc32 );
                  checksum = 0x00;
                }

                compressed = true;
              }
            }

            ++files;

            LARGE_INTEGER fsize;

            fsize.HighPart = fd.nFileSizeHigh;
            fsize.LowPart  = fd.nFileSizeLow;

            liSize.QuadPart += fsize.QuadPart;

            if (compressed)
              liCompressed.QuadPart   += fsize.QuadPart;
            else
              liUncompressed.QuadPart += fsize.QuadPart;

            if (textures.dumped_textures.count (top_crc32) >= 1)
              SK_LOG0 ( ( L" >> WARNING: Multiple textures have "
                            L"the same top-level LOD hash (%08X) <<",
                              top_crc32 ), L"DX11TexDmp" );

            if (checksum == 0x00)
              textures.dumped_textures.insert (top_crc32);
            else
              textures.dumped_collisions.insert (safe_crc32c (top_crc32, (uint8_t *)(&checksum), 4));
          }
        }
      } while (FindNextFileW (hFind, &fd) != 0);

      FindClose (hFind);
    }

    textures.dumped_texture_bytes = liSize.QuadPart;

    dll_log.LogEx ( false, L" %lu files (%3.1f MiB -- %3.1f:%3.1f MiB Un:Compressed)\n",
                      files, (double)liSize.QuadPart / (1024.0 * 1024.0),
                               (double)liUncompressed.QuadPart / (1024.0 * 1024.0),
                                 (double)liCompressed.QuadPart /  (1024.0 * 1024.0) );
  }

  wchar_t wszTexInjectDir_RAW [ MAX_PATH + 2 ] = { };
  wchar_t wszTexInjectDir     [ MAX_PATH + 2 ] = { };

  lstrcatW (wszTexInjectDir_RAW, SK_D3D11_res_root.c_str ());
  lstrcatW (wszTexInjectDir_RAW, LR"(\inject\textures)");

  wcscpy ( wszTexInjectDir,
             SK_EvalEnvironmentVars (wszTexInjectDir_RAW).c_str () );

  if ( GetFileAttributesW (wszTexInjectDir) !=
         INVALID_FILE_ATTRIBUTES )
  {
    dll_log.LogEx ( true, L"[DX11TexMgr] Enumerating injectable..." );

    unsigned int    files  =   0;
    LARGE_INTEGER   liSize = {   };

    SK_D3D11_RecursiveEnumAndAddTex (wszTexInjectDir, files, liSize);

    textures.injectable_texture_bytes = liSize.QuadPart;

    dll_log.LogEx ( false, L" %lu files (%3.1f MiB)\n",
                      files, (double)liSize.QuadPart / (1024.0 * 1024.0) );
  }

  wchar_t wszTexInjectDir_FFX_RAW [ MAX_PATH     ] = { };
  wchar_t wszTexInjectDir_FFX     [ MAX_PATH + 2 ] = { };

  lstrcatW (wszTexInjectDir_FFX_RAW, SK_D3D11_res_root.c_str ());
  lstrcatW (wszTexInjectDir_FFX_RAW, LR"(\inject\textures\UnX_Old)");

  wcscpy ( wszTexInjectDir_FFX,
             SK_EvalEnvironmentVars (wszTexInjectDir_FFX_RAW).c_str () );

  if ( GetFileAttributesW (wszTexInjectDir_FFX) !=
         INVALID_FILE_ATTRIBUTES )
  {
    WIN32_FIND_DATA fd     = {   };
    HANDLE          hFind  = INVALID_HANDLE_VALUE;
    int             files  =   0;
    LARGE_INTEGER   liSize = {   };

    dll_log.LogEx ( true, L"[DX11TexMgr] Enumerating FFX inject..." );

    lstrcatW (wszTexInjectDir_FFX, LR"(\*)");

    hFind = FindFirstFileW (wszTexInjectDir_FFX, &fd);

    if (hFind != INVALID_HANDLE_VALUE)
    {
      do
      {
        if (fd.dwFileAttributes != INVALID_FILE_ATTRIBUTES)
        {
          if (StrStrIW (fd.cFileName, L".dds"))
          {
            uint32_t ffx_crc32;

            swscanf (fd.cFileName, L"%08X.dds", &ffx_crc32);

            ++files;

            LARGE_INTEGER fsize;

            fsize.HighPart = fd.nFileSizeHigh;
            fsize.LowPart  = fd.nFileSizeLow;

            liSize.QuadPart += fsize.QuadPart;

            textures.injectable_ffx.insert           (ffx_crc32);
          }
        }
      } while (FindNextFileW (hFind, &fd) != 0);

      FindClose (hFind);
    }

    dll_log.LogEx ( false, L" %li files (%3.1f MiB)\n",
                      files, (double)liSize.QuadPart / (1024.0 * 1024.0) );
  }

  for (auto& it : SK_D3D11_EnumeratedMipmapCache)
  {
    SK_D3D11_AddTexHash (it.first.c_str (), it.second, 0);
  }
}

void
WINAPI
SK_D3D11_SetResourceRoot (const wchar_t* root)
{
  // Non-absolute path (e.g. NOT C:\...\...")
  if (! wcschr (root, L':'))
  {
         wchar_t wszPath [MAX_PATH * 2 + 1] = { };
    wcsncpy_s   (wszPath, MAX_PATH * 2,
                   SK_IsInjected () ? SK_GetConfigPath () :
                                      SK_GetRootPath   (),  _TRUNCATE);
    PathAppendW (wszPath, root);

    SK_D3D11_res_root = wszPath;
  }

  else
    SK_D3D11_res_root = root;
}

void
WINAPI
SK_D3D11_EnableTexDump (bool enable) 
{
  SK_D3D11_dump_textures = enable;
}

void
WINAPI
SK_D3D11_EnableTexInject (bool enable) 
{
  SK_D3D11_inject_textures = enable;
}

void
WINAPI
SK_D3D11_EnableTexInject_FFX (bool enable) 
{
  SK_D3D11_inject_textures     = enable;
  SK_D3D11_inject_textures_ffx = enable;
}

void
WINAPI
SK_D3D11_EnableTexCache (bool enable) 
{
  SK_D3D11_cache_textures = enable;
}

void
WINAPI
SKX_D3D11_MarkTextures (bool x, bool y, bool z) 
{
  UNREFERENCED_PARAMETER (x);
  UNREFERENCED_PARAMETER (y);
  UNREFERENCED_PARAMETER (z);
}

bool
WINAPI
SK_D3D11_IsTexHashed (uint32_t top_crc32, uint32_t hash)
{
  static auto& textures =
    SK_D3D11_Textures;

  static auto& tex_hashes    = textures.tex_hashes;
  static auto& tex_hashes_ex = textures.tex_hashes_ex;

  if (tex_hashes_ex.empty () && tex_hashes.empty ())
    return false;

  SK_AutoCriticalSection critical (&hash_cs);

  if (tex_hashes.count (safe_crc32c (top_crc32, (const uint8_t *)&hash, 4)) != 0)
    return true;

  return
    tex_hashes.count (top_crc32) != 0;
}

void
WINAPI
SK_D3D11_AddInjectable (uint32_t top_crc32, uint32_t checksum);

void
WINAPI
SK_D3D11_AddTexHash ( const wchar_t* name, uint32_t top_crc32, uint32_t hash )
{
  // For early loading UnX
  SK_D3D11_InitTextures ();

  static auto& textures =
    SK_D3D11_Textures;

  if (hash != 0x00)
  {
    if (! SK_D3D11_IsTexHashed (top_crc32, hash))
    {
      {
        SK_AutoCriticalSection critical (&hash_cs);
        textures.tex_hashes_ex.emplace  (safe_crc32c (top_crc32, (const uint8_t *)&hash, 4), name);
      }

      SK_D3D11_AddInjectable (top_crc32, hash);
    }
  }

  if (! SK_D3D11_IsTexHashed (top_crc32, 0x00))
  {
    {
      SK_AutoCriticalSection critical (&hash_cs);
      textures.tex_hashes.emplace (top_crc32, name);
    }

    if (! SK_D3D11_inject_textures_ffx)
      SK_D3D11_AddInjectable (top_crc32, 0x00);
    else
    {
      SK_AutoCriticalSection critical (&inject_cs);
      textures.injectable_ffx.emplace (top_crc32);
    }
  }
}

void
WINAPI
SK_D3D11_RemoveTexHash (uint32_t top_crc32, uint32_t hash)
{
  static auto& textures =
    SK_D3D11_Textures;

  if (textures.tex_hashes_ex.empty () && textures.tex_hashes.empty ())
    return;

  SK_AutoCriticalSection critical (&hash_cs);

  if (hash != 0x00 && SK_D3D11_IsTexHashed (top_crc32, hash))
  {
    textures.tex_hashes_ex.erase (safe_crc32c (top_crc32, (const uint8_t *)&hash, 4));

    SK_D3D11_RemoveInjectable (top_crc32, hash);
  }

  else if (hash == 0x00 && SK_D3D11_IsTexHashed (top_crc32, 0x00)) {
    textures.tex_hashes.erase (top_crc32);

    SK_D3D11_RemoveInjectable (top_crc32, 0x00);
  }
}

std::wstring
__stdcall
SK_D3D11_TexHashToName (uint32_t top_crc32, uint32_t hash)
{
  static auto& textures =
    SK_D3D11_Textures;

  if (textures.tex_hashes_ex.empty () && textures.tex_hashes.empty ())
    return L"";

  SK_AutoCriticalSection critical (&hash_cs);

  std::wstring ret = L"";

  if (hash != 0x00 && SK_D3D11_IsTexHashed (top_crc32, hash))
  {
    ret = textures.tex_hashes_ex [safe_crc32c (top_crc32, (const uint8_t *)&hash, 4)];
  }

  else if (hash == 0x00 && SK_D3D11_IsTexHashed (top_crc32, 0x00))
  {
    ret = textures.tex_hashes [top_crc32];
  }

  return ret;
}

void
SK_D3D11_RecursiveEnumAndAddTex ( std::wstring   directory, unsigned int& files,
                                  LARGE_INTEGER& liSize,    wchar_t*      wszPattern )
{
  WIN32_FIND_DATA fd            = {   };
  HANDLE          hFind         = INVALID_HANDLE_VALUE;
  wchar_t         wszPath        [MAX_PATH * 2] = { };
  wchar_t         wszFullPattern [MAX_PATH * 2] = { };

  PathCombineW (wszFullPattern, directory.c_str (), L"*");

  hFind =
    FindFirstFileW (wszFullPattern, &fd);

  if (hFind != INVALID_HANDLE_VALUE)
  {
    do
    {
      if (          (fd.dwFileAttributes       & FILE_ATTRIBUTE_DIRECTORY) &&
          (_wcsicmp (fd.cFileName, L"." )      != 0) &&
          (_wcsicmp (fd.cFileName, L"..")      != 0) &&
          (_wcsicmp (fd.cFileName, L"UnX_Old") != 0)   )
      {
        PathCombineW                    (wszPath, directory.c_str (), fd.cFileName);
        SK_D3D11_RecursiveEnumAndAddTex (wszPath, files, liSize, wszPattern);
      }
    } while (FindNextFile (hFind, &fd));

    FindClose (hFind);
  }

  PathCombineW (wszFullPattern, directory.c_str (), wszPattern);

  hFind =
    FindFirstFileW (wszFullPattern, &fd);

  //bool preload =
  //  StrStrIW (directory.c_str (), LR"(\Preload\)") != nullptr;

  if (hFind != INVALID_HANDLE_VALUE)
  {
    do
    {
      if (fd.dwFileAttributes != INVALID_FILE_ATTRIBUTES)
      {
        if (StrStrIW (fd.cFileName, L".dds"))
        {
        //bool     preloaded = preload;
          uint32_t top_crc32 = 0x00;
          uint32_t checksum  = 0x00;

          const wchar_t* wszFileName = fd.cFileName;

          //if (StrStrIW (wszFileName, L"Preload") == fd.cFileName)
          //{
          //  const size_t skip =
          //    wcslen (L"Preload");
          //
          //  for ( size_t i = 0; i < skip; i++ )
          //    wszFileName = CharNextW (wszFileName);
          //
          //  preloaded = true;
          //}

          if (StrStrIW (wszFileName, L"_"))
          {
            swscanf (wszFileName, L"%08X_%08X.dds", &top_crc32, &checksum);
          }

          else
          {
            swscanf (wszFileName, L"%08X.dds",    &top_crc32);
          }

          ++files;

          LARGE_INTEGER fsize;

          fsize.HighPart = fd.nFileSizeHigh;
          fsize.LowPart  = fd.nFileSizeLow;

          liSize.QuadPart += fsize.QuadPart;

          PathCombineW        (wszPath, directory.c_str (), wszFileName);

          if (! StrStrIW (wszPath, L"MipmapCache"))
          {
            SK_D3D11_AddTexHash (wszPath, top_crc32, 0);

            if (checksum != 0x00)
              SK_D3D11_AddTexHash (wszPath, top_crc32, checksum);

            //if (preloaded)
            //  SK_D3D11_AddTexPreLoad (top_crc32);

          }

          else
          {
            SK_D3D11_MipmapCacheSize += fsize.QuadPart;
            SK_D3D11_EnumeratedMipmapCache.emplace (wszPath, top_crc32);
          }
        }
      }
    } while (FindNextFileW (hFind, &fd) != 0);

    FindClose (hFind);
  }
}

INT
__stdcall
SK_D3D11_BytesPerPixel (DXGI_FORMAT fmt) 
{
  switch (fmt)
  {
    case DXGI_FORMAT_R32G32B32A32_TYPELESS:      return 16;
    case DXGI_FORMAT_R32G32B32A32_FLOAT:         return 16;
    case DXGI_FORMAT_R32G32B32A32_UINT:          return 16;
    case DXGI_FORMAT_R32G32B32A32_SINT:          return 16;

    case DXGI_FORMAT_R32G32B32_TYPELESS:         return 12;
    case DXGI_FORMAT_R32G32B32_FLOAT:            return 12;
    case DXGI_FORMAT_R32G32B32_UINT:             return 12;
    case DXGI_FORMAT_R32G32B32_SINT:             return 12;

    case DXGI_FORMAT_R16G16B16A16_TYPELESS:      return 8;
    case DXGI_FORMAT_R16G16B16A16_FLOAT:         return 8;
    case DXGI_FORMAT_R16G16B16A16_UNORM:         return 8;
    case DXGI_FORMAT_R16G16B16A16_UINT:          return 8;
    case DXGI_FORMAT_R16G16B16A16_SNORM:         return 8;
    case DXGI_FORMAT_R16G16B16A16_SINT:          return 8;

    case DXGI_FORMAT_R32G32_TYPELESS:            return 8;
    case DXGI_FORMAT_R32G32_FLOAT:               return 8;
    case DXGI_FORMAT_R32G32_UINT:                return 8;
    case DXGI_FORMAT_R32G32_SINT:                return 8;
    case DXGI_FORMAT_R32G8X24_TYPELESS:          return 8;

    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:       return 8;
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:   return 8;
    case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:    return 8;

    case DXGI_FORMAT_R10G10B10A2_TYPELESS:       return 4;
    case DXGI_FORMAT_R10G10B10A2_UNORM:          return 4;
    case DXGI_FORMAT_R10G10B10A2_UINT:           return 4;
    case DXGI_FORMAT_R11G11B10_FLOAT:            return 4;

    case DXGI_FORMAT_R8G8B8A8_TYPELESS:          return 4;
    case DXGI_FORMAT_R8G8B8A8_UNORM:             return 4;
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:        return 4;
    case DXGI_FORMAT_R8G8B8A8_UINT:              return 4;
    case DXGI_FORMAT_R8G8B8A8_SNORM:             return 4;
    case DXGI_FORMAT_R8G8B8A8_SINT:              return 4;

    case DXGI_FORMAT_R16G16_TYPELESS:            return 4;
    case DXGI_FORMAT_R16G16_FLOAT:               return 4;
    case DXGI_FORMAT_R16G16_UNORM:               return 4;
    case DXGI_FORMAT_R16G16_UINT:                return 4;
    case DXGI_FORMAT_R16G16_SNORM:               return 4;
    case DXGI_FORMAT_R16G16_SINT:                return 4;

    case DXGI_FORMAT_R32_TYPELESS:               return 4;
    case DXGI_FORMAT_D32_FLOAT:                  return 4;
    case DXGI_FORMAT_R32_FLOAT:                  return 4;
    case DXGI_FORMAT_R32_UINT:                   return 4;
    case DXGI_FORMAT_R32_SINT:                   return 4;
    case DXGI_FORMAT_R24G8_TYPELESS:             return 4;

    case DXGI_FORMAT_D24_UNORM_S8_UINT:          return 4;
    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:      return 4;
    case DXGI_FORMAT_X24_TYPELESS_G8_UINT:       return 4;

    case DXGI_FORMAT_R8G8_TYPELESS:              return 2;
    case DXGI_FORMAT_R8G8_UNORM:                 return 2;
    case DXGI_FORMAT_R8G8_UINT:                  return 2;
    case DXGI_FORMAT_R8G8_SNORM:                 return 2;
    case DXGI_FORMAT_R8G8_SINT:                  return 2;

    case DXGI_FORMAT_R16_TYPELESS:               return 2;
    case DXGI_FORMAT_R16_FLOAT:                  return 2;
    case DXGI_FORMAT_D16_UNORM:                  return 2;
    case DXGI_FORMAT_R16_UNORM:                  return 2;
    case DXGI_FORMAT_R16_UINT:                   return 2;
    case DXGI_FORMAT_R16_SNORM:                  return 2;
    case DXGI_FORMAT_R16_SINT:                   return 2;

    case DXGI_FORMAT_R8_TYPELESS:                return 1;
    case DXGI_FORMAT_R8_UNORM:                   return 1;
    case DXGI_FORMAT_R8_UINT:                    return 1;
    case DXGI_FORMAT_R8_SNORM:                   return 1;
    case DXGI_FORMAT_R8_SINT:                    return 1;
    case DXGI_FORMAT_A8_UNORM:                   return 1;
    case DXGI_FORMAT_R1_UNORM:                   return 1;

    case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:         return 4;
    case DXGI_FORMAT_R8G8_B8G8_UNORM:            return 4;
    case DXGI_FORMAT_G8R8_G8B8_UNORM:            return 4;

    case DXGI_FORMAT_BC1_TYPELESS:               return -1;
    case DXGI_FORMAT_BC1_UNORM:                  return -1;
    case DXGI_FORMAT_BC1_UNORM_SRGB:             return -1;
    case DXGI_FORMAT_BC2_TYPELESS:               return -2;
    case DXGI_FORMAT_BC2_UNORM:                  return -2;
    case DXGI_FORMAT_BC2_UNORM_SRGB:             return -2;
    case DXGI_FORMAT_BC3_TYPELESS:               return -2;
    case DXGI_FORMAT_BC3_UNORM:                  return -2;
    case DXGI_FORMAT_BC3_UNORM_SRGB:             return -2;
    case DXGI_FORMAT_BC4_TYPELESS:               return -1;
    case DXGI_FORMAT_BC4_UNORM:                  return -1;
    case DXGI_FORMAT_BC4_SNORM:                  return -1;
    case DXGI_FORMAT_BC5_TYPELESS:               return -2;
    case DXGI_FORMAT_BC5_UNORM:                  return -2;
    case DXGI_FORMAT_BC5_SNORM:                  return -2;

    case DXGI_FORMAT_B5G6R5_UNORM:               return 2;
    case DXGI_FORMAT_B5G5R5A1_UNORM:             return 2;
    case DXGI_FORMAT_B8G8R8X8_UNORM:             return 4;
    case DXGI_FORMAT_B8G8R8A8_UNORM:             return 4;
    case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM: return 4;
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:          return 4;
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:        return 4;
    case DXGI_FORMAT_B8G8R8X8_TYPELESS:          return 4;
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:        return 4;

    case DXGI_FORMAT_BC6H_TYPELESS:              return -2;
    case DXGI_FORMAT_BC6H_UF16:                  return -2;
    case DXGI_FORMAT_BC6H_SF16:                  return -2;
    case DXGI_FORMAT_BC7_TYPELESS:               return -2;
    case DXGI_FORMAT_BC7_UNORM:                  return -2;
    case DXGI_FORMAT_BC7_UNORM_SRGB:             return -2;

    case DXGI_FORMAT_AYUV:                       return 0;
    case DXGI_FORMAT_Y410:                       return 0;
    case DXGI_FORMAT_Y416:                       return 0;
    case DXGI_FORMAT_NV12:                       return 0;
    case DXGI_FORMAT_P010:                       return 0;
    case DXGI_FORMAT_P016:                       return 0;
    case DXGI_FORMAT_420_OPAQUE:                 return 0;
    case DXGI_FORMAT_YUY2:                       return 0;
    case DXGI_FORMAT_Y210:                       return 0;
    case DXGI_FORMAT_Y216:                       return 0;
    case DXGI_FORMAT_NV11:                       return 0;
    case DXGI_FORMAT_AI44:                       return 0;
    case DXGI_FORMAT_IA44:                       return 0;
    case DXGI_FORMAT_P8:                         return 1;
    case DXGI_FORMAT_A8P8:                       return 2;
    case DXGI_FORMAT_B4G4R4A4_UNORM:             return 2;

    default: return 0;
  }
}

std::wstring
__stdcall
SK_DXGI_FormatToStr (DXGI_FORMAT fmt)
{
  switch (fmt)
  {
    case DXGI_FORMAT_R32G32B32A32_TYPELESS:      return L"DXGI_FORMAT_R32G32B32A32_TYPELESS";
    case DXGI_FORMAT_R32G32B32A32_FLOAT:         return L"DXGI_FORMAT_R32G32B32A32_FLOAT";
    case DXGI_FORMAT_R32G32B32A32_UINT:          return L"DXGI_FORMAT_R32G32B32A32_UINT";
    case DXGI_FORMAT_R32G32B32A32_SINT:          return L"DXGI_FORMAT_R32G32B32A32_SINT";

    case DXGI_FORMAT_R32G32B32_TYPELESS:         return L"DXGI_FORMAT_R32G32B32_TYPELESS";
    case DXGI_FORMAT_R32G32B32_FLOAT:            return L"DXGI_FORMAT_R32G32B32_FLOAT";
    case DXGI_FORMAT_R32G32B32_UINT:             return L"DXGI_FORMAT_R32G32B32_UINT";
    case DXGI_FORMAT_R32G32B32_SINT:             return L"DXGI_FORMAT_R32G32B32_SINT";

    case DXGI_FORMAT_R16G16B16A16_TYPELESS:      return L"DXGI_FORMAT_R16G16B16A16_TYPELESS";
    case DXGI_FORMAT_R16G16B16A16_FLOAT:         return L"DXGI_FORMAT_R16G16B16A16_FLOAT";
    case DXGI_FORMAT_R16G16B16A16_UNORM:         return L"DXGI_FORMAT_R16G16B16A16_UNORM";
    case DXGI_FORMAT_R16G16B16A16_UINT:          return L"DXGI_FORMAT_R16G16B16A16_UINT";
    case DXGI_FORMAT_R16G16B16A16_SNORM:         return L"DXGI_FORMAT_R16G16B16A16_SNORM";
    case DXGI_FORMAT_R16G16B16A16_SINT:          return L"DXGI_FORMAT_R16G16B16A16_SINT";

    case DXGI_FORMAT_R32G32_TYPELESS:            return L"DXGI_FORMAT_R32G32_TYPELESS";
    case DXGI_FORMAT_R32G32_FLOAT:               return L"DXGI_FORMAT_R32G32_FLOAT";
    case DXGI_FORMAT_R32G32_UINT:                return L"DXGI_FORMAT_R32G32_UINT";
    case DXGI_FORMAT_R32G32_SINT:                return L"DXGI_FORMAT_R32G32_SINT";
    case DXGI_FORMAT_R32G8X24_TYPELESS:          return L"DXGI_FORMAT_R32G8X24_TYPELESS";

    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:       return L"DXGI_FORMAT_D32_FLOAT_S8X24_UINT";
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:   return L"DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS";
    case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:    return L"DXGI_FORMAT_X32_TYPELESS_G8X24_UINT";

    case DXGI_FORMAT_R10G10B10A2_TYPELESS:       return L"DXGI_FORMAT_R10G10B10A2_TYPELESS";
    case DXGI_FORMAT_R10G10B10A2_UNORM:          return L"DXGI_FORMAT_R10G10B10A2_UNORM";
    case DXGI_FORMAT_R10G10B10A2_UINT:           return L"DXGI_FORMAT_R10G10B10A2_UINT";
    case DXGI_FORMAT_R11G11B10_FLOAT:            return L"DXGI_FORMAT_R11G11B10_FLOAT";

    case DXGI_FORMAT_R8G8B8A8_TYPELESS:          return L"DXGI_FORMAT_R8G8B8A8_TYPELESS";
    case DXGI_FORMAT_R8G8B8A8_UNORM:             return L"DXGI_FORMAT_R8G8B8A8_UNORM";
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:        return L"DXGI_FORMAT_R8G8B8A8_UNORM_SRGB";
    case DXGI_FORMAT_R8G8B8A8_UINT:              return L"DXGI_FORMAT_R8G8B8A8_UINT";
    case DXGI_FORMAT_R8G8B8A8_SNORM:             return L"DXGI_FORMAT_R8G8B8A8_SNORM";
    case DXGI_FORMAT_R8G8B8A8_SINT:              return L"DXGI_FORMAT_R8G8B8A8_SINT";

    case DXGI_FORMAT_R16G16_TYPELESS:            return L"DXGI_FORMAT_R16G16_TYPELESS";
    case DXGI_FORMAT_R16G16_FLOAT:               return L"DXGI_FORMAT_R16G16_FLOAT";
    case DXGI_FORMAT_R16G16_UNORM:               return L"DXGI_FORMAT_R16G16_UNORM";
    case DXGI_FORMAT_R16G16_UINT:                return L"DXGI_FORMAT_R16G16_UINT";
    case DXGI_FORMAT_R16G16_SNORM:               return L"DXGI_FORMAT_R16G16_SNORM";
    case DXGI_FORMAT_R16G16_SINT:                return L"DXGI_FORMAT_R16G16_SINT";

    case DXGI_FORMAT_R32_TYPELESS:               return L"DXGI_FORMAT_R32_TYPELESS";
    case DXGI_FORMAT_D32_FLOAT:                  return L"DXGI_FORMAT_D32_FLOAT";
    case DXGI_FORMAT_R32_FLOAT:                  return L"DXGI_FORMAT_R32_FLOAT";
    case DXGI_FORMAT_R32_UINT:                   return L"DXGI_FORMAT_R32_UINT";
    case DXGI_FORMAT_R32_SINT:                   return L"DXGI_FORMAT_R32_SINT";
    case DXGI_FORMAT_R24G8_TYPELESS:             return L"DXGI_FORMAT_R24G8_TYPELESS";

    case DXGI_FORMAT_D24_UNORM_S8_UINT:          return L"DXGI_FORMAT_D24_UNORM_S8_UINT";
    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:      return L"DXGI_FORMAT_R24_UNORM_X8_TYPELESS";
    case DXGI_FORMAT_X24_TYPELESS_G8_UINT:       return L"DXGI_FORMAT_X24_TYPELESS_G8_UINT";

    case DXGI_FORMAT_R8G8_TYPELESS:              return L"DXGI_FORMAT_R8G8_TYPELESS";
    case DXGI_FORMAT_R8G8_UNORM:                 return L"DXGI_FORMAT_R8G8_UNORM";
    case DXGI_FORMAT_R8G8_UINT:                  return L"DXGI_FORMAT_R8G8_UINT";
    case DXGI_FORMAT_R8G8_SNORM:                 return L"DXGI_FORMAT_R8G8_SNORM";
    case DXGI_FORMAT_R8G8_SINT:                  return L"DXGI_FORMAT_R8G8_SINT";

    case DXGI_FORMAT_R16_TYPELESS:               return L"DXGI_FORMAT_R16_TYPELESS";
    case DXGI_FORMAT_R16_FLOAT:                  return L"DXGI_FORMAT_R16_FLOAT";
    case DXGI_FORMAT_D16_UNORM:                  return L"DXGI_FORMAT_D16_UNORM";
    case DXGI_FORMAT_R16_UNORM:                  return L"DXGI_FORMAT_R16_UNORM";
    case DXGI_FORMAT_R16_UINT:                   return L"DXGI_FORMAT_R16_UINT";
    case DXGI_FORMAT_R16_SNORM:                  return L"DXGI_FORMAT_R16_SNORM";
    case DXGI_FORMAT_R16_SINT:                   return L"DXGI_FORMAT_R16_SINT";

    case DXGI_FORMAT_R8_TYPELESS:                return L"DXGI_FORMAT_R8_TYPELESS";
    case DXGI_FORMAT_R8_UNORM:                   return L"DXGI_FORMAT_R8_UNORM";
    case DXGI_FORMAT_R8_UINT:                    return L"DXGI_FORMAT_R8_UINT";
    case DXGI_FORMAT_R8_SNORM:                   return L"DXGI_FORMAT_R8_SNORM";
    case DXGI_FORMAT_R8_SINT:                    return L"DXGI_FORMAT_R8_SINT";
    case DXGI_FORMAT_A8_UNORM:                   return L"DXGI_FORMAT_A8_UNORM";
    case DXGI_FORMAT_R1_UNORM:                   return L"DXGI_FORMAT_R1_UNORM";

    case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:         return L"DXGI_FORMAT_R9G9B9E5_SHAREDEXP";
    case DXGI_FORMAT_R8G8_B8G8_UNORM:            return L"DXGI_FORMAT_R8G8_B8G8_UNORM";
    case DXGI_FORMAT_G8R8_G8B8_UNORM:            return L"DXGI_FORMAT_G8R8_G8B8_UNORM";

    case DXGI_FORMAT_BC1_TYPELESS:               return L"DXGI_FORMAT_BC1_TYPELESS";
    case DXGI_FORMAT_BC1_UNORM:                  return L"DXGI_FORMAT_BC1_UNORM";
    case DXGI_FORMAT_BC1_UNORM_SRGB:             return L"DXGI_FORMAT_BC1_UNORM_SRGB";
    case DXGI_FORMAT_BC2_TYPELESS:               return L"DXGI_FORMAT_BC2_TYPELESS";
    case DXGI_FORMAT_BC2_UNORM:                  return L"DXGI_FORMAT_BC2_UNORM";
    case DXGI_FORMAT_BC2_UNORM_SRGB:             return L"DXGI_FORMAT_BC2_UNORM_SRGB";
    case DXGI_FORMAT_BC3_TYPELESS:               return L"DXGI_FORMAT_BC3_TYPELESS";
    case DXGI_FORMAT_BC3_UNORM:                  return L"DXGI_FORMAT_BC3_UNORM";
    case DXGI_FORMAT_BC3_UNORM_SRGB:             return L"DXGI_FORMAT_BC3_UNORM_SRGB";
    case DXGI_FORMAT_BC4_TYPELESS:               return L"DXGI_FORMAT_BC4_TYPELESS";
    case DXGI_FORMAT_BC4_UNORM:                  return L"DXGI_FORMAT_BC4_UNORM";
    case DXGI_FORMAT_BC4_SNORM:                  return L"DXGI_FORMAT_BC4_SNORM";
    case DXGI_FORMAT_BC5_TYPELESS:               return L"DXGI_FORMAT_BC5_TYPELESS";
    case DXGI_FORMAT_BC5_UNORM:                  return L"DXGI_FORMAT_BC5_UNORM";
    case DXGI_FORMAT_BC5_SNORM:                  return L"DXGI_FORMAT_BC5_SNORM";

    case DXGI_FORMAT_B5G6R5_UNORM:               return L"DXGI_FORMAT_B5G6R5_UNORM";
    case DXGI_FORMAT_B5G5R5A1_UNORM:             return L"DXGI_FORMAT_B5G5R5A1_UNORM";
    case DXGI_FORMAT_B8G8R8X8_UNORM:             return L"DXGI_FORMAT_B8G8R8X8_UNORM";
    case DXGI_FORMAT_B8G8R8A8_UNORM:             return L"DXGI_FORMAT_B8G8R8A8_UNORM";
    case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM: return L"DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM";
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:          return L"DXGI_FORMAT_B8G8R8A8_TYPELESS";
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:        return L"DXGI_FORMAT_B8G8R8A8_UNORM_SRGB";
    case DXGI_FORMAT_B8G8R8X8_TYPELESS:          return L"DXGI_FORMAT_B8G8R8X8_TYPELESS";
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:        return L"DXGI_FORMAT_B8G8R8X8_UNORM_SRGB";

    case DXGI_FORMAT_BC6H_TYPELESS:              return L"DXGI_FORMAT_BC6H_TYPELESS";
    case DXGI_FORMAT_BC6H_UF16:                  return L"DXGI_FORMAT_BC6H_UF16";
    case DXGI_FORMAT_BC6H_SF16:                  return L"DXGI_FORMAT_BC6H_SF16";
    case DXGI_FORMAT_BC7_TYPELESS:               return L"DXGI_FORMAT_BC7_TYPELESS";
    case DXGI_FORMAT_BC7_UNORM:                  return L"DXGI_FORMAT_BC7_UNORM";
    case DXGI_FORMAT_BC7_UNORM_SRGB:             return L"DXGI_FORMAT_BC7_UNORM_SRGB";

    case DXGI_FORMAT_AYUV:                       return L"DXGI_FORMAT_AYUV";
    case DXGI_FORMAT_Y410:                       return L"DXGI_FORMAT_Y410";
    case DXGI_FORMAT_Y416:                       return L"DXGI_FORMAT_Y416";
    case DXGI_FORMAT_NV12:                       return L"DXGI_FORMAT_NV12";
    case DXGI_FORMAT_P010:                       return L"DXGI_FORMAT_P010";
    case DXGI_FORMAT_P016:                       return L"DXGI_FORMAT_P016";
    case DXGI_FORMAT_420_OPAQUE:                 return L"DXGI_FORMAT_420_OPAQUE";
    case DXGI_FORMAT_YUY2:                       return L"DXGI_FORMAT_YUY2";
    case DXGI_FORMAT_Y210:                       return L"DXGI_FORMAT_Y210";
    case DXGI_FORMAT_Y216:                       return L"DXGI_FORMAT_Y216";
    case DXGI_FORMAT_NV11:                       return L"DXGI_FORMAT_NV11";
    case DXGI_FORMAT_AI44:                       return L"DXGI_FORMAT_AI44";
    case DXGI_FORMAT_IA44:                       return L"DXGI_FORMAT_IA44";
    case DXGI_FORMAT_P8:                         return L"DXGI_FORMAT_P8";
    case DXGI_FORMAT_A8P8:                       return L"DXGI_FORMAT_A8P8";
    case DXGI_FORMAT_B4G4R4A4_UNORM:             return L"DXGI_FORMAT_B4G4R4A4_UNORM";

    default:                                     return L"UNKNOWN";
  }
}

uint32_t
safe_crc32c_ex (uint32_t seed, const void* pData, size_t size, bool* failed)
{
  *failed = false;

  // Current limit == 48 GiB
  if (size > (1024ULL * 1024ULL * 1024ULL * 48) || pData == nullptr)
  {
    SK_LOG0 ( ( L"Hash Fail: Data is too large (%zu) or invalid pointer (%p).",
                  size, pData ),
                L"DX11TexMgr");

    *failed = true;

    return seed;
  }

  __try
  {
    return crc32c (seed, pData, size);
  }

  __except ( ( GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ) ? 
             EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH )
  {
    *failed = true;

    return 0x00;
  }
}

__declspec (noinline)
void
SK_D3D11_DescribeTexFailure (const D3D11_TEXTURE2D_DESC   *__restrict pDesc)
{
  SK_LOG0 ( (L"Texture Hash Fail: Access Violation [Tex: %lux%lu : %lu LODs :: Array Size %lu"
             L"Format: %s, Usage: %s, CPUAccess: %x, BindFlags: %s, Misc Flags: %s]",
                           pDesc->Width,  pDesc->Height,
                       pDesc->MipLevels,  pDesc->ArraySize,
              SK_DXGI_FormatToStr        (pDesc->Format).c_str (),
              SK_D3D11_DescribeUsage     (pDesc->Usage),
                                          pDesc->CPUAccessFlags,
              SK_D3D11_DescribeBindFlags (
           static_cast <D3D11_BIND_FLAG> (pDesc->BindFlags)).c_str (),
              SK_D3D11_DescribeMiscFlags (
  static_cast <D3D11_RESOURCE_MISC_FLAG> (pDesc->MiscFlags)).c_str ()),
              L"DX11TexMgr" );
}

__declspec (noinline)
uint32_t
__cdecl
crc32_tex (  _In_      const D3D11_TEXTURE2D_DESC   *__restrict pDesc,
             _In_      const D3D11_SUBRESOURCE_DATA *__restrict pInitialData,
             _Out_opt_       size_t                 *__restrict pSize,
             _Out_opt_       uint32_t               *__restrict pLOD0_CRC32,
             _In_opt_        bool                               bAllLODs /*= true */)
{
  if (pLOD0_CRC32 != nullptr)
    *pLOD0_CRC32 = 0ui32;

  // Eh?  Why did we even call this if there's no freaking data?!
  if (pInitialData == nullptr)
  {
    assert (pInitialData != nullptr && "FIXME: Dumbass");
    return 0ui32;
  }

  if (pDesc->MiscFlags > D3D11_RESOURCE_MISC_TEXTURECUBE)
  {
    SK_LOG0 ( (L">> Hashing texture with unexpected MiscFlags: "
                   L"0x%04X",
                     pDesc->MiscFlags), L" Tex Hash " );
    return 0;
  }

  if (pDesc->MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE)
  {
    //SK_LOG0 ( ( L">>Neat! A cubemap!  [%lux%lu - Array: %lu :: pInitialData: %ph ]",
    //              pDesc->Width, pDesc->Height, pDesc->ArraySize, pInitialData ),
    //           L"DX11TexMgr" );
    return 0;
  }

        uint32_t checksum   = 0;
  const bool     compressed =
    SK_D3D11_IsFormatCompressed (pDesc->Format);

  int bpp = 0;

  switch (pDesc->Format)
  {
    case DXGI_FORMAT_BC1_TYPELESS:
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC4_TYPELESS:
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC4_SNORM:
      bpp = 0;
      break;

    case DXGI_FORMAT_BC2_TYPELESS:
    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_TYPELESS:
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_BC5_TYPELESS:
    case DXGI_FORMAT_BC5_UNORM:
    case DXGI_FORMAT_BC5_SNORM:
    case DXGI_FORMAT_BC6H_TYPELESS:
    case DXGI_FORMAT_BC6H_UF16:
    case DXGI_FORMAT_BC6H_SF16:
    case DXGI_FORMAT_BC7_TYPELESS:
    case DXGI_FORMAT_BC7_UNORM:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
      bpp = 1;
      break;
  }

  unsigned int width  = pDesc->Width;
  unsigned int height = pDesc->Height;

        size_t size   = 0UL;

  if (compressed)
  {
    for (unsigned int i = 0; i < pDesc->MipLevels; i++)
    {
      if (i > 0 && (!bAllLODs))
        break;
        
      auto* pData    =
        static_cast  <char *> (
          const_cast <void *> (pInitialData [i].pSysMem)
        );

      const UINT stride = bpp == 0 ?
             std::max (1UL, ((width + 3UL) / 4UL) ) *  8UL:
             std::max (1UL, ((width + 3UL) / 4UL) ) * 16UL;

      // Fast path:  Data is tightly packed and alignment agrees with
      //               convention...
      if (stride == pInitialData [i].SysMemPitch)
      {
        const unsigned int lod_size =
          ( stride * ( height / 4 +
                       height % 4 ) );

        __try {
          checksum  = safe_crc32c (checksum, (const uint8_t *)pData, lod_size);
          size     += lod_size;
        }

        // Triggered by a certain JRPG that shall remain nameless (not so Marvelous!)
        __except ( GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ?
                     EXCEPTION_EXECUTE_HANDLER :
                     EXCEPTION_CONTINUE_SEARCH )
        {
          size += static_cast <size_t> (stride * (height / 4)) + (height % 4);
          SK_LOG0 ( ( L"Access Violation while Hashing Texture: %x", checksum ),
                      L" Tex Hash " );
          return 0;
        }
      }

      else
      {
        // We are running through the compressed image block-by-block,
        //  the lines we are "scanning" actually represent 4 rows of image data.
        for (unsigned int j = 0; j < height; j += 4)
        {
          bool failed = false;

          const uint32_t row_checksum =
            safe_crc32c_ex (checksum, (const uint8_t *)(pData), stride, &failed);

          if (row_checksum != 0x00)
            checksum = row_checksum;

          if (failed)
          {
            SK_D3D11_DescribeTexFailure (pDesc);

            return 0;
          }

          
          // Respect the engine's reported stride, while making sure to
          //   only read the 4x4 blocks that have legal data. Any padding
          //     the engine uses will not be included in our hash since the
          //       values are undefined.
          pData += pInitialData [i].SysMemPitch;
          size  += stride;
        }
      }

      if (i == 0 && pLOD0_CRC32 != nullptr)
        *pLOD0_CRC32 = checksum;

      if (width  > 1) width  >>= 1UL;
      if (height > 1) height >>= 1UL;
    }
  }


  else
  {
    for (unsigned int i = 0; i < pDesc->MipLevels; i++)
    {
      if (i > 0 && (! bAllLODs))
        break;

      auto* pData      =
        static_const_cast <char *, void *> (pInitialData [i].pSysMem);

      const UINT  scanlength =
        SK_D3D11_BytesPerPixel (pDesc->Format) * width;

      // Fast path:  Data is tightly packed and alignment agrees with
      //               convention...
      if (scanlength == pInitialData [i].SysMemPitch) 
      {
        const unsigned int lod_size =
          (scanlength * height);

        bool failed = false;

        checksum = safe_crc32c_ex ( checksum,
                                     static_cast <const uint8_t *> (
                                       static_const_cast <const void *, const char *> (pData)
                                     ),
                                    lod_size, &failed );
        size    += lod_size;

        if (failed)
          return 0;
      }

      else if (pDesc->Usage != D3D11_USAGE_STAGING)
      {
        for (unsigned int j = 0; j < height; j++)
        {
          bool failed = false;

          const uint32_t row_checksum =
            safe_crc32c_ex (checksum, (const uint8_t *)(pData), scanlength, &failed);

          if (row_checksum != 0x00)
            checksum = row_checksum;

          if (failed)
            return 0;

          pData += pInitialData [i].SysMemPitch;
          size  += scanlength;
        }
      }

      if (i == 0 && pLOD0_CRC32 != nullptr)
        *pLOD0_CRC32 = checksum;

      if (width  > 1) width  >>= 1UL;
      if (height > 1) height >>= 1UL;
    }
  }

  if (pSize != nullptr)
     *pSize  = size;

  return checksum;
}


//
// OLD, BUGGY Algorithm... must remain here for compatibility with UnX :(
//
__declspec (noinline)
uint32_t
__cdecl
crc32_ffx (  _In_      const D3D11_TEXTURE2D_DESC   *__restrict pDesc,
             _In_      const D3D11_SUBRESOURCE_DATA *__restrict pInitialData,
             _Out_opt_       size_t                 *__restrict pSize )
{
  uint32_t checksum = 0;

  const bool compressed =
    SK_D3D11_IsFormatCompressed (pDesc->Format);

  const int block_size = pDesc->Format == DXGI_FORMAT_BC1_UNORM ? 8 : 16;
  const int height     = pDesc->Height;

  size_t size = 0;

  for (unsigned int i = 0; i < pDesc->MipLevels; i++)
  {
    if (compressed)
    {
      size += (pInitialData [i].SysMemPitch / block_size) * static_cast <size_t> (height >> i);

      checksum =
        crc32 ( checksum, static_cast <const char *>(pInitialData [i].pSysMem),
                  (pInitialData [i].SysMemPitch / block_size) * static_cast <size_t> (height >> i) );
    }

    else
    {
      size += (pInitialData [i].SysMemPitch) * static_cast <size_t> (height >> i);

      checksum =
        crc32 ( checksum, static_cast <const char *>(pInitialData [i].pSysMem),
                  (pInitialData [i].SysMemPitch) * static_cast <size_t> (height >> i) );
    }
  }

  if (pSize != nullptr)
    *pSize = size;

  return checksum;
}


bool
__stdcall
SK_D3D11_IsDumped (uint32_t top_crc32, uint32_t checksum)
{
  static auto& textures =
    SK_D3D11_Textures;

  if (textures.dumped_textures.empty ())
    return false;

  SK_AutoCriticalSection critical (&dump_cs);

  if (config.textures.d3d11.precise_hash && textures.dumped_collisions.count (safe_crc32c (top_crc32, (uint8_t *)&checksum, 4)))
    return true;
  if (! config.textures.d3d11.precise_hash)
    return textures.dumped_textures.count (top_crc32) != 0;

  return false;
}

void
__stdcall
SK_D3D11_AddDumped (uint32_t top_crc32, uint32_t checksum)
{
  static auto& textures =
    SK_D3D11_Textures;

  SK_AutoCriticalSection critical (&dump_cs);

  
  if (! config.textures.d3d11.precise_hash)
    textures.dumped_textures.insert (top_crc32);

  textures.dumped_collisions.insert (safe_crc32c (top_crc32, (uint8_t *)&checksum, 4));
}

void
__stdcall
SK_D3D11_RemoveDumped (uint32_t top_crc32, uint32_t checksum)
{
  static auto& textures =
    SK_D3D11_Textures;

  if (textures.dumped_textures.empty ())
    return;

  SK_AutoCriticalSection critical (&dump_cs);

  if (! config.textures.d3d11.precise_hash)
    textures.dumped_textures.erase (top_crc32);

  textures.dumped_collisions.erase (safe_crc32c (top_crc32, (uint8_t *)&checksum, 4));
}

bool
__stdcall
SK_D3D11_IsInjectable (uint32_t top_crc32, uint32_t checksum)
{
  static auto& textures =
    SK_D3D11_Textures;

  if (textures.injectable_textures.empty ())
    return false;

  SK_AutoCriticalSection critical (&inject_cs);

  if (checksum != 0x00)
  {
    if (textures.injected_collisions.count (safe_crc32c (top_crc32, (uint8_t *)&checksum, 4)))
      return true;

    return false;
  }

  return
    textures.injectable_textures.count (top_crc32) != 0;
}

bool
__stdcall
SK_D3D11_IsInjectable_FFX (uint32_t top_crc32)
{
  static auto& textures =
    SK_D3D11_Textures;

  if (textures.injectable_ffx.empty ())
    return false;

  SK_AutoCriticalSection critical (&inject_cs);

  return
    textures.injectable_ffx.count (top_crc32) != 0;
}


void
__stdcall
SK_D3D11_AddInjectable (uint32_t top_crc32, uint32_t checksum)
{
  static auto& textures =
    SK_D3D11_Textures;

  SK_AutoCriticalSection critical (&inject_cs);

  if (checksum != 0x00)
    textures.injected_collisions.insert (safe_crc32c (top_crc32, (uint8_t *)&checksum, 4));

  textures.injectable_textures.insert (top_crc32);
}


void
__stdcall
SK_D3D11_RemoveInjectable (uint32_t top_crc32, uint32_t checksum)
{
  static auto& textures =
    SK_D3D11_Textures;

  SK_AutoCriticalSection critical (&inject_cs);

  if (checksum != 0x00)
    textures.injected_collisions.erase (safe_crc32c (top_crc32, (uint8_t *)&checksum, 4));

  textures.injectable_textures.erase (top_crc32);
}

HRESULT
__stdcall
SK_D3D11_DumpTexture2D ( _In_ ID3D11Texture2D* pTex, uint32_t crc32c )
{
  static const auto& rb =
    SK_GetCurrentRenderBackend ();

  CComQIPtr <ID3D11Device>        pDev    (rb.device);
  CComQIPtr <ID3D11DeviceContext> pDevCtx (rb.d3d11.immediate_ctx);

  if ( pDev    != nullptr &&
       pDevCtx != nullptr )
  {
    DirectX::ScratchImage img;

    if (SUCCEEDED (DirectX::CaptureTexture (pDev, pDevCtx, pTex, img)))
    {
      wchar_t wszPath [ MAX_PATH + 2 ] = { };

      wcscpy ( wszPath,
                 SK_EvalEnvironmentVars (SK_D3D11_res_root.c_str ()).c_str () );

      lstrcatW (wszPath, L"/dump/textures/");
      lstrcatW (wszPath, SK_GetHostApp ());
      lstrcatW (wszPath, L"/");

      const DXGI_FORMAT fmt =
        img.GetMetadata ().format;

      const bool compressed =
        SK_D3D11_IsFormatCompressed (fmt);

      wchar_t wszOutName [MAX_PATH + 2] = { };

      if (compressed)
      {
        _swprintf ( wszOutName, LR"(%s\Compressed_%08X.dds)",
                      wszPath, crc32c );
      }

      else
      {
        _swprintf ( wszOutName, LR"(%s\Uncompressed_%08X.dds)",
                      wszPath, crc32c );
      }

      SK_CreateDirectories (wszPath);

      const HRESULT hr =
        DirectX::SaveToDDSFile ( img.GetImages   (), img.GetImageCount (),
                                 img.GetMetadata (), 0x00, wszOutName );

      if (SUCCEEDED (hr))
      {
        SK_D3D11_Textures.dumped_texture_bytes += SK_File_GetSize (wszOutName);

        SK_D3D11_AddDumped (crc32c, crc32c);

        return hr;
      }
    }
  }

  return E_FAIL;
}


bool
SK_D3D11_IsFormatSRGB (DXGI_FORMAT format) 
{
  switch (format)
  {
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
      return true;
    default:
      return false;
  }
}

DXGI_FORMAT
SK_D3D11_MakeFormatSRGB (DXGI_FORMAT format) 
{
  switch (format)
  {
    //case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_TYPELESS:
      return DXGI_FORMAT_BC1_UNORM_SRGB;
    //case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_TYPELESS:
      return DXGI_FORMAT_BC2_UNORM_SRGB;
    //case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_TYPELESS:
      return DXGI_FORMAT_BC3_UNORM_SRGB;
    //case DXGI_FORMAT_BC7_UNORM:
    case DXGI_FORMAT_BC7_TYPELESS:
      return DXGI_FORMAT_BC7_UNORM_SRGB;
    //case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
      return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    //case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
      return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    case DXGI_FORMAT_B8G8R8X8_TYPELESS:
    //case DXGI_FORMAT_B8G8R8X8_UNORM:
      return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
    default:
      return format;
  }
}

DXGI_FORMAT
SK_D3D11_MakeTypedFormat (DXGI_FORMAT typeless) 
{
  switch (typeless)
  {
    case DXGI_FORMAT_BC1_TYPELESS:
      return DXGI_FORMAT_BC1_UNORM;
    case DXGI_FORMAT_BC2_TYPELESS:
      return DXGI_FORMAT_BC2_UNORM;
    case DXGI_FORMAT_BC3_TYPELESS:
      return DXGI_FORMAT_BC3_UNORM;
    case DXGI_FORMAT_BC4_TYPELESS:
      return DXGI_FORMAT_BC4_UNORM;
    case DXGI_FORMAT_BC5_TYPELESS:
      return DXGI_FORMAT_BC5_UNORM;
    case DXGI_FORMAT_BC6H_TYPELESS:
      return DXGI_FORMAT_BC6H_SF16;
    case DXGI_FORMAT_BC7_TYPELESS:
      return DXGI_FORMAT_BC7_UNORM;

    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
      return DXGI_FORMAT_B8G8R8A8_UNORM;
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
      return DXGI_FORMAT_R8G8B8A8_UNORM;

    default:
      return typeless;
  };
}

DXGI_FORMAT
SK_D3D11_MakeTypelessFormat (DXGI_FORMAT typeless) 
{
  switch (typeless)
  {
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
      return DXGI_FORMAT_BC1_TYPELESS;
    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
      return DXGI_FORMAT_BC2_TYPELESS;
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
      return DXGI_FORMAT_BC3_TYPELESS;
    case DXGI_FORMAT_BC4_UNORM:
      return DXGI_FORMAT_BC4_TYPELESS;
    case DXGI_FORMAT_BC5_UNORM:
      return DXGI_FORMAT_BC5_TYPELESS;
    case DXGI_FORMAT_BC6H_SF16:
    case DXGI_FORMAT_BC6H_UF16:
      return DXGI_FORMAT_BC6H_TYPELESS;
    case DXGI_FORMAT_BC7_UNORM:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
      return DXGI_FORMAT_BC7_TYPELESS;

    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
      return DXGI_FORMAT_B8G8R8A8_TYPELESS;

    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
      return DXGI_FORMAT_R8G8B8A8_TYPELESS;

    case DXGI_FORMAT_B8G8R8X8_UNORM:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
      return DXGI_FORMAT_B8G8R8A8_TYPELESS;

    default:
      return typeless;
  };
}

HRESULT
__stdcall
SK_D3D11_MipmapCacheTexture2DEx ( const DirectX::ScratchImage&   img,
                                        uint32_t                 crc32c, 
                                        ID3D11Texture2D*       /*pOutTex*/,
                                        DirectX::ScratchImage** ppOutImg,
                                        SK_TLS*                  pTLS )
{
  if ( img.GetMetadata ().width <  4 ||
       img.GetMetadata ().height < 4    ) return E_INVALIDARG;

  SK_ScopedBool auto_bool  (&pTLS->texture_management.injection_thread);
  SK_ScopedBool auto_bool2 (&pTLS->imgui.drawing);

  pTLS->texture_management.injection_thread = true;
  pTLS->imgui.drawing                       = true;


  wchar_t wszPath [ MAX_PATH + 2 ] = { };

  wcscpy ( wszPath,
             SK_EvalEnvironmentVars (SK_D3D11_res_root.c_str ()).c_str () );

  lstrcatW (wszPath, L"/inject/textures/MipmapCache/");
  lstrcatW (wszPath, SK_GetHostApp ());
  lstrcatW (wszPath, L"/");

  wchar_t wszOutName [MAX_PATH + 2] = { };

  _swprintf ( wszOutName, LR"(%s\%08X.dds)",
                wszPath, crc32c );


  SK_CreateDirectories (wszPath);

  const bool compressed =
    SK_D3D11_IsFormatCompressed (img.GetMetadata ().format);

  DirectX::ScratchImage* mipmaps = nullptr;

  try {
    mipmaps =
      new DirectX::ScratchImage;
  }
  catch (...)
  {
  }

  HRESULT ret  = E_FAIL;
  size_t  size = 0;

  if (mipmaps && compressed )
  {
          DirectX::ScratchImage decompressed;
    const DirectX::Image*       orig_img =
      img.GetImage (0, 0, 0);

    DirectX::TexMetadata meta =
      img.GetMetadata ();

    meta.format    = SK_D3D11_MakeTypedFormat (meta.format);
    meta.mipLevels = 1;

    ret =
      DirectX::Decompress (orig_img, 1, meta, DXGI_FORMAT_UNKNOWN, decompressed);

    if (SUCCEEDED (ret))
    {
      ret =
        DirectX::GenerateMipMaps ( decompressed.GetImage (0,0,0),
                                   1,
                                   decompressed.GetMetadata   (), DirectX::TEX_FILTER_FANT,
                                   0, *mipmaps );

      if (SUCCEEDED (ret))
      {
        if ((! config.textures.d3d11.uncompressed_mips))// && (! SK_D3D11_IsFormatBC6Or7 (meta.format)))
        {
          auto* compressed_mips =
            new DirectX::ScratchImage;

          DirectX::TexMetadata mipmap_meta =
            mipmaps->GetMetadata ();

          //
          // Required speedup since BC7 is _slow_
          //
          if (mipmap_meta.format == DXGI_FORMAT_BC7_UNORM)
              mipmap_meta.format = DXGI_FORMAT_BC5_UNORM;

          mipmap_meta.format           =
            SK_D3D11_MakeTypedFormat (mipmap_meta.format);

          const DXGI_FORMAT newFormat =
            SK_D3D11_MakeTypedFormat (img.GetMetadata ().format);

          ret =
            DirectX::Compress ( //This,
                                  mipmaps->GetImages       (),
                                    mipmaps->GetImageCount (),
                                      mipmap_meta,
                                        newFormat,//DXGI_FORMAT_BC7_UNORM,
                                          DirectX::TEX_COMPRESS_DITHER //|
                                          ,//DirectX::TEX_COMPRESS_PARALLEL,
                                            DirectX::TEX_THRESHOLD_DEFAULT, *compressed_mips );

          if (SUCCEEDED (ret))
          {
            delete mipmaps;
                   mipmaps = compressed_mips;
          }
        }
      }
    }
  }

  else
  {
    DirectX::TexMetadata meta =
      img.GetMetadata ();

    meta.format = SK_D3D11_MakeTypedFormat (meta.format);

    ret =
      DirectX::GenerateMipMaps ( img.GetImages     (),
                                 img.GetImageCount (),
                                 meta,                 DirectX::TEX_FILTER_FANT,
                                   0, *mipmaps );
  }

  if (SUCCEEDED (ret) && mipmaps != nullptr)
  {
    if (config.textures.d3d11.cache_gen_mips)
    {
      DirectX::TexMetadata meta =
        mipmaps->GetMetadata ();

      meta.format = SK_D3D11_MakeTypedFormat (meta.format);

      if (SUCCEEDED (DirectX::SaveToDDSFile ( mipmaps->GetImages (), mipmaps->GetImageCount (),
                                              meta,                  0x00, wszOutName ) ) )
      {
        size =
          mipmaps->GetPixelsSize ();
      }
    }
  }
  

  if (SUCCEEDED (ret))
  {
    if (ppOutImg != nullptr)
      *ppOutImg = mipmaps;
    
    delete mipmaps;

    if (config.textures.d3d11.cache_gen_mips)// || SK_D3D11_IsTexInjectThread ())
    {
      extern uint64_t SK_D3D11_MipmapCacheSize;
                      SK_D3D11_MipmapCacheSize += size;

      SK_D3D11_AddDumped  (crc32c, crc32c);
      SK_D3D11_AddTexHash (wszOutName, crc32c, 0);
    }

    return ret;
  }

  delete mipmaps;

  return E_FAIL;
}

HRESULT
__stdcall
SK_D3D11_MipmapCacheTexture2D ( _In_ ID3D11Texture2D* pTex, uint32_t crc32c, SK_TLS* pTLS = SK_TLS_Bottom () )
{
  static const auto& rb =
    SK_GetCurrentRenderBackend ();

  CComQIPtr <ID3D11Device>        pDev    (rb.device);
  CComQIPtr <ID3D11DeviceContext> pDevCtx (rb.d3d11.immediate_ctx);

  if ( pDev    != nullptr &&
       pDevCtx != nullptr    )
  {
    DirectX::ScratchImage img;

    if ( SUCCEEDED (
           DirectX::CaptureTexture ( pDev, pDevCtx,
                                     pTex, img     )
                   )
       )
    {
      return
        SK_D3D11_MipmapCacheTexture2DEx (
          img, crc32c,
            nullptr, nullptr,
                        pTLS
        );
    }
  }

  return E_FAIL;
}


BOOL
SK_D3D11_DeleteDumpedTexture (uint32_t crc32c)
{
  wchar_t wszPath [ MAX_PATH + 2 ] = { };

  wcscpy ( wszPath,
             SK_EvalEnvironmentVars (SK_D3D11_res_root.c_str ()).c_str () );

  lstrcatW (wszPath, L"/dump/textures/");
  lstrcatW (wszPath, SK_GetHostApp ());
  lstrcatW (wszPath, L"/");

  wchar_t wszOutName [MAX_PATH + 2] = { };
  _swprintf ( wszOutName, LR"(%s\Compressed_%08X.dds)",
                wszPath, crc32c );

  uint64_t size = SK_File_GetSize (wszOutName);

  if (DeleteFileW (wszOutName))
  {
    SK_D3D11_RemoveDumped (crc32c, crc32c);

    SK_D3D11_Textures.dumped_texture_bytes -= size;

    return TRUE;
  }

  *wszOutName = L'\0';

  _swprintf ( wszOutName, LR"(%s\Uncompressed_%08X.dds)",
                wszPath, crc32c );

  size = SK_File_GetSize (wszOutName);

  if (DeleteFileW (wszOutName))
  {
    SK_D3D11_RemoveDumped (crc32c, crc32c);

    SK_D3D11_Textures.dumped_texture_bytes -= size;

    return TRUE;
  }

  return FALSE;
}

HRESULT
__stdcall
SK_D3D11_DumpTexture2D (  _In_ const D3D11_TEXTURE2D_DESC   *pDesc,
                          _In_ const D3D11_SUBRESOURCE_DATA *pInitialData,
                          _In_       uint32_t                top_crc32,
                          _In_       uint32_t                checksum )
{
  static auto& textures =
    SK_D3D11_Textures;

  SK_LOG0 ( (L"Dumping Texture: %08x::%08x... (fmt=%03lu, "
                    L"BindFlags=0x%04x, Usage=0x%04x, CPUAccessFlags"
                    L"=0x%02x, Misc=0x%02x, MipLODs=%02lu, ArraySize=%02lu)",
                  top_crc32,
                    checksum,
  static_cast <UINT> (pDesc->Format),
                        pDesc->BindFlags,
                          pDesc->Usage,
                            pDesc->CPUAccessFlags,
                              pDesc->MiscFlags,
                                pDesc->MipLevels,
                                  pDesc->ArraySize), L"DX11TexDmp" );

   DXGI_FORMAT newFormat = pDesc->Format;
  if (DirectX::IsTypeless (pDesc->Format))
  { if   (pDesc->Format == DirectX::MakeTypelessUNORM (pDesc->Format))
    { if (pDesc->Format == DirectX::MakeTypelessFLOAT (pDesc->Format))
      {        newFormat = DirectX::MakeSRGB          (pDesc->Format); }
         else{ newFormat = DirectX::MakeTypelessFLOAT (pDesc->Format); } }
         else{ newFormat = DirectX::MakeTypelessUNORM (pDesc->Format); } }

  SK_D3D11_AddDumped (top_crc32, checksum);

  DirectX::TexMetadata mdata;

  mdata.width      = pDesc->Width;
  mdata.height     = pDesc->Height;
  mdata.depth      = 1;
  mdata.arraySize  = pDesc->ArraySize;
  mdata.mipLevels  = pDesc->MipLevels;
  mdata.miscFlags  = (pDesc->MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE) ? 
                                            DirectX::TEX_MISC_TEXTURECUBE : 0;
  mdata.miscFlags2 = 0;
  mdata.format     = newFormat;
  mdata.dimension  = DirectX::TEX_DIMENSION_TEXTURE2D;

  DirectX::ScratchImage image;
  image.Initialize (mdata);

  bool error = false;

  for (size_t slice = 0; slice < mdata.arraySize; ++slice)
  {
    size_t height = mdata.height;

    for (size_t lod = 0; lod < mdata.mipLevels; ++lod)
    {
      const DirectX::Image* img =
        image.GetImage (lod, slice, 0);

      if (! (img && img->pixels))
      {
        error = true;
        break;
      }

      const size_t lines =
        DirectX::ComputeScanlines (mdata.format, height);

      if  (! lines)
      {
        error = true;
        break;
      }

      auto sptr =
        static_cast <const uint8_t *>(
          pInitialData [lod].pSysMem
        );

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
    }

    if (error)
      break;
  }

  wchar_t wszPath [ MAX_PATH + 2 ] = { };

  wcscpy ( wszPath,
             SK_EvalEnvironmentVars (SK_D3D11_res_root.c_str ()).c_str () );

  lstrcatW (wszPath, L"/dump/textures/");
  lstrcatW (wszPath, SK_GetHostApp ());
  lstrcatW (wszPath, L"/");

  SK_CreateDirectories (wszPath);

  const bool compressed =
    SK_D3D11_IsFormatCompressed (pDesc->Format);

  wchar_t wszOutPath [MAX_PATH + 2] = { };
  wchar_t wszOutName [MAX_PATH + 2] = { };

  wcscpy ( wszOutPath,
             SK_EvalEnvironmentVars (SK_D3D11_res_root.c_str ()).c_str () );

  lstrcatW (wszOutPath, LR"(\dump\textures\)");
  lstrcatW (wszOutPath, SK_GetHostApp ());

  if (compressed && config.textures.d3d11.precise_hash) {
    _swprintf ( wszOutName, LR"(%s\Compressed_%08X_%08X.dds)",
                  wszOutPath, top_crc32, checksum );
  } else if (compressed) {
    _swprintf ( wszOutName, LR"(%s\Compressed_%08X.dds)",
                  wszOutPath, top_crc32 );
  } else if (config.textures.d3d11.precise_hash) {
    _swprintf ( wszOutName, LR"(%s\Uncompressed_%08X_%08X.dds)",
                  wszOutPath, top_crc32, checksum );
  } else {
    _swprintf ( wszOutName, LR"(%s\Uncompressed_%08X.dds)",
                  wszOutPath, top_crc32 );
  }

  if ((! error) && wcslen (wszOutName))
  {
    if (GetFileAttributes (wszOutName) == INVALID_FILE_ATTRIBUTES)
    {
      SK_LOG0 ( (L" >> File: '%s' (top: %x, full: %x)",
                      wszOutName,
                        top_crc32,
                          checksum), L"DX11TexDmp" );

#if 0
      wchar_t wszMetaFilename [ MAX_PATH + 2 ] = { };

      _swprintf (wszMetaFilename, L"%s.txt", wszOutName);

      FILE* fMetaData = _wfopen (wszMetaFilename, L"w+");

      if (fMetaData != nullptr) {
        fprintf ( fMetaData,
                  "Dumped Name:    %ws\n"
                  "Texture:        %08x::%08x\n"
                  "Dimensions:     (%lux%lu)\n"
                  "Format:         %03lu\n"
                  "BindFlags:      0x%04x\n"
                  "Usage:          0x%04x\n"
                  "CPUAccessFlags: 0x%02x\n"
                  "Misc:           0x%02x\n"
                  "MipLODs:        %02lu\n"
                  "ArraySize:      %02lu",
                  wszOutName,
                    top_crc32,
                      checksum,
                        pDesc->Width, pDesc->Height,
                        pDesc->Format,
                          pDesc->BindFlags,
                            pDesc->Usage,
                              pDesc->CPUAccessFlags,
                                pDesc->MiscFlags,
                                  pDesc->MipLevels,
                                    pDesc->ArraySize );

        fclose (fMetaData);
      }
#endif

      const HRESULT hr = 
        SaveToDDSFile ( image.GetImages (), image.GetImageCount (),
                          image.GetMetadata (), DirectX::DDS_FLAGS_NONE,
                            wszOutName );

      if (SUCCEEDED (hr))
      {
        textures.dumped_texture_bytes +=
          SK_File_GetSize (wszOutName);
      }
    }
  }

  return E_FAIL;
}

__declspec (noinline)
HRESULT
WINAPI
D3D11Dev_CreateBuffer_Override (
  _In_           ID3D11Device            *This,
  _In_     const D3D11_BUFFER_DESC       *pDesc,
  _In_opt_ const D3D11_SUBRESOURCE_DATA  *pInitialData,
  _Out_opt_      ID3D11Buffer           **ppBuffer ) 
{
  return
    D3D11Dev_CreateBuffer_Original ( This, pDesc,
                                       pInitialData, ppBuffer );
}



#if 0
DEFINE_GUID(IID_ID3D11ShaderResourceView,0xb0e06fe0,0x8192,0x4e1a,0xb1,0xca,0x36,0xd7,0x41,0x47,0x10,0xb2);
typedef interface ID3D11ShaderResourceView ID3D11ShaderResourceView;

EXTERN_C const IID IID_ID3D11ShaderResourceView1;

typedef struct D3D11_TEX2D_SRV1
{
  UINT MostDetailedMip;
  UINT MipLevels;
  UINT PlaneSlice;
} D3D11_TEX2D_SRV1;

typedef struct D3D11_TEX2D_ARRAY_SRV1
{
  UINT MostDetailedMip;
  UINT MipLevels;
  UINT FirstArraySlice;
  UINT ArraySize;
  UINT PlaneSlice;
} D3D11_TEX2D_ARRAY_SRV1;

typedef struct D3D11_SHADER_RESOURCE_VIEW_DESC1
{
  DXGI_FORMAT         Format;
  D3D11_SRV_DIMENSION ViewDimension;

  unionP
  {
    D3D11_BUFFER_SRV        Buffer;
    D3D11_TEX1D_SRV         Texture1D;
    D3D11_TEX1D_ARRAY_SRV   Texture1DArray;
    D3D11_TEX2D_SRV1        Texture2D;
    D3D11_TEX2D_ARRAY_SRV1  Texture2DArray;
    D3D11_TEX2DMS_SRV       Texture2DMS;
    D3D11_TEX2DMS_ARRAY_SRV Texture2DMSArray;
    D3D11_TEX3D_SRV         Texture3D;
    D3D11_TEXCUBE_SRV       TextureCube;
    D3D11_TEXCUBE_ARRAY_SRV TextureCubeArray;
    D3D11_BUFFEREX_SRV      BufferEx;
  };
} D3D11_SHADER_RESOURCE_VIEW_DESC1;

MIDL_INTERFACE("91308b87-9040-411d-8c67-c39253ce3802")
ID3D11ShaderResourceView1 : public ID3D11ShaderResourceView
{
public:
    virtual void STDMETHODCALLTYPE GetDesc1( 
        /* [annotation] */ 
        _Out_  D3D11_SHADER_RESOURCE_VIEW_DESC1 *pDesc1) = 0;
    
};

interface SK_ID3D11_ShaderResourceView : public ID3D11ShaderResourceView1
{
public:
  explicit SK_ID3D11_ShaderResourceView (ID3D11ShaderResourceView *pView) : pRealSRV (pView)
  {
    iver_ = 0;
    InterlockedExchange (&refs_, 1);
  }

  explicit SK_ID3D11_ShaderResourceView (ID3D11ShaderResourceView1 *pView) : pRealSRV (pView)
  {
    iver_ = 1;
    InterlockedExchange (&refs_, 1);
  }

  SK_ID3D11_ShaderResourceView            (const SK_ID3D11_ShaderResourceView &) = delete;
  SK_ID3D11_ShaderResourceView &operator= (const SK_ID3D11_ShaderResourceView &) = delete;

  __declspec (noinline)
  virtual HRESULT STDMETHODCALLTYPE QueryInterface (
    _In_         REFIID                     riid,
    _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject )    override
  {
    if (ppvObject == nullptr)
    {
      return E_POINTER;
    }

    else if ( riid == __uuidof (IUnknown)                 ||
              riid == __uuidof (ID3D11DeviceChild)        ||
              riid == __uuidof (ID3D11View)               ||
              riid == __uuidof (ID3D11ShaderResourceView) ||
              riid == __uuidof (ID3D11ShaderResourceView1) )
    {
#pragma region Update to ID3D11ShaderResourceView1 interface
      if (riid == __uuidof (ID3D11ShaderResourceView1) && iver_ < 1)
      {
        ID3D11ShaderResourceView1* pSRV1 = nullptr;

        if (FAILED (pRealSRV->QueryInterface (&pSRV1)))
        {
          return E_NOINTERFACE;
        }

        pRealSRV->Release ();

        SK_LOG0 ( ( L"Adding 'ID3D11ShaderResourceView1' (v1) interfaces to %ph, whose original interface was limited to 'ID3D11ShaderResourceView' (v0).",
                      this ),
                    L"DX11ResMgr" );

        pRealSRV = pSRV1;

        iver_ = 1;
      }
#pragma endregion

      AddRef ();
      
      *ppvObject = this;

      return S_OK;
    }

    else
      return pRealSRV->QueryInterface (riid, ppvObject);
  }

  __declspec (noinline)
  virtual ULONG STDMETHODCALLTYPE AddRef (void)            override
  {
    InterlockedIncrement (&refs_);

    assert (pRealSRV != nullptr);

    return pRealSRV->AddRef ();
  }

  __declspec (noinline)
  virtual ULONG STDMETHODCALLTYPE Release (void)           override
  {
    assert (pRealSRV != nullptr);


    if (InterlockedAdd (&refs_, 0) > 0)
    {
      ULONG local_refs =
        pRealSRV->Release ();

      if (InterlockedDecrement (&refs_) == 0 && local_refs != 0)
      {
        SK_LOG0 ( ( L"WARNING: 'ID3D11ShaderResourceView%s (%ph) has %lu references; expected 0.",
                    iver_ > 0 ? (std::to_wstring (iver_) + L"'").c_str () :
                                                           L"'",
                      this,
                        local_refs ),
                    L"DX11ResMgr"
                );

        local_refs = 0;
      }

      if (local_refs == 0)
      {
        assert (InterlockedAdd (refs_, 0) <= 0);

        SK_LOG1 ( ( L"Destroying Custom Shader Resource View" ),
                    L"DX11TexMgr" );

        delete this;
      }

      return local_refs;
    }

    return 0;
  }

  template <class Q>
  HRESULT
#ifdef _M_CEE_PURE
  __clrcall
#else
  STDMETHODCALLTYPE
#endif
  QueryInterface (_COM_Outptr_ Q** pp)
  {
    return QueryInterface (__uuidof (Q), (void **)pp);
  }

  __declspec (noinline)
  virtual void STDMETHODCALLTYPE GetDevice ( 
      /* [annotation] */ 
      _Out_  ID3D11Device **ppDevice)                        override
  {
    assert (pRealSRV != nullptr);

    pRealSRV->GetDevice (ppDevice);
  }
  
  __declspec (noinline)
  virtual HRESULT STDMETHODCALLTYPE GetPrivateData ( 
      _In_                                  REFGUID  guid,
      _Inout_                               UINT    *pDataSize,
      _Out_writes_bytes_opt_( *pDataSize )  void    *pData)  override
  {
    return pRealSRV->GetPrivateData (guid, pDataSize, pData);
  }
  
  __declspec (noinline)
  virtual HRESULT STDMETHODCALLTYPE SetPrivateData ( 
      _In_                                    REFGUID guid,
      _In_                                    UINT    DataSize,
      _In_reads_bytes_opt_( DataSize )  const void   *pData) override
  {
    return pRealSRV->SetPrivateData (guid, DataSize, pData);
  }
  
  __declspec (noinline)
  virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface ( 
      _In_            REFGUID   guid,
      _In_opt_  const IUnknown *pData)                       override
  {
    return pRealSRV->SetPrivateDataInterface (guid, pData);
  }

  __declspec (noinline)
  virtual void STDMETHODCALLTYPE GetResource ( 
      _Out_  ID3D11Resource **ppResource )                   override
  {
    pRealSRV->GetResource (ppResource);
  }

  __declspec (noinline)
  virtual void STDMETHODCALLTYPE GetDesc (
      _Out_  D3D11_SHADER_RESOURCE_VIEW_DESC *pDesc )        override
  {
    pRealSRV->GetDesc (pDesc);
  }

#pragma region ID3D11ShaderResourceView1
  __declspec (noinline)
  virtual void STDMETHODCALLTYPE GetDesc1 (
      _Out_  D3D11_SHADER_RESOURCE_VIEW_DESC1 *pDesc )       override
  {
    reinterpret_cast <ID3D11ShaderResourceView1 *> (pRealSRV)->GetDesc1 (pDesc);
  }
#pragma endregion


           ID3D11ShaderResourceView* pRealSRV = nullptr;
           unsigned int                 iver_ = 0;
  volatile LONG                         refs_ = 1;
};
#endif

#include <unordered_set>

__declspec (noinline)
HRESULT
WINAPI
D3D11Dev_CreateShaderResourceView_Override (
  _In_           ID3D11Device                     *This,
  _In_           ID3D11Resource                   *pResource,
  _In_opt_ const D3D11_SHADER_RESOURCE_VIEW_DESC  *pDesc,
  _Out_opt_      ID3D11ShaderResourceView        **ppSRView )
{
  static auto& textures =
    SK_D3D11_Textures;

  if (pDesc != nullptr && pResource != nullptr)
  {
    D3D11_RESOURCE_DIMENSION   dim;
    pResource->GetType       (&dim);
  
    if (dim == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
    {
      ////if (SK_GetCurrentGameID () == SK_GAME_ID::DotHackGU)
      ////{
      ////  if (pDesc != nullptr && pDesc->Format == DXGI_FORMAT_B8G8R8A8_UNORM)
      ////    ((D3D11_SHADER_RESOURCE_VIEW_DESC *)pDesc)->Format = DXGI_FORMAT_R8G8B8A8_UNORM;
      ////}
  
      DXGI_FORMAT newFormat    = pDesc->Format;
      UINT        newMipLevels = pDesc->Texture2D.MipLevels;
  
      CComQIPtr <ID3D11Texture2D> pTex (pResource);
  
      D3D11_TEXTURE2D_DESC        tex_desc = { };
      if (pTex) pTex->GetDesc   (&tex_desc);
      if (pTex != nullptr)// && (!((tex_desc.BindFlags & D3D11_BIND_RENDER_TARGET)||
                          //      (tex_desc.BindFlags & D3D11_BIND_DEPTH_STENCIL))))
      {
        bool override = false;
  
        if ( pDesc->Format      == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB &&
             tex_desc.Format    == DXGI_FORMAT_R8G8B8A8_UNORM      &&
            (tex_desc.BindFlags &  D3D11_BIND_RENDER_TARGET) )
        {
          override  = true;
          newFormat = tex_desc.Format;
        }
  
        if ( SK_D3D11_OverrideDepthStencil (newFormat) )
          override = true;
  
        if ( SK_D3D11_TextureIsCached (pTex) )
        {
          auto& cache_desc =
            textures.Textures_2D [pTex];
  
          newFormat =
            cache_desc.desc.Format;
  
          newMipLevels =
            pDesc->Texture2D.MipLevels;
  
          if (pDesc->Format != newFormat)// && SK_DXGI_FormatToStr (pDesc->Format) != L"UNKNOWN")
          {
            if (SK_D3D11_IsFormatSRGB (pDesc->Format))
              newFormat = SK_D3D11_MakeFormatSRGB (newFormat);
  
            override = true;
  
            SK_LOG1 ( ( L"Overriding Resource View Format for Cached Texture '%08x'  { Was: '%s', Now: '%s' }",
                          cache_desc.crc32c,
                     SK_DXGI_FormatToStr (pDesc->Format).c_str      (),
                              SK_DXGI_FormatToStr (newFormat).c_str () ),
                        L"DX11TexMgr" );
          }
  
          if (config.textures.d3d11.generate_mips && cache_desc.desc.MipLevels != pDesc->Texture2D.MipLevels)
          {
            override     = true;
            newMipLevels = cache_desc.desc.MipLevels;
  
            SK_LOG1 ( ( L"Overriding Resource View Mip Levels for Cached Texture '%08x'  { Was: %lu, Now: %lu }",
                          cache_desc.crc32c,
                            pDesc->Texture2D.MipLevels,
                               newMipLevels ),
                        L"DX11TexMgr" );
          }
        }
  
        if (override)
        {
          auto descCopy =
            *pDesc;
  
          descCopy.Format                    = newFormat;
  
          if (newMipLevels != pDesc->Texture2D.MipLevels)
          {
            descCopy.Texture2D.MipLevels       = static_cast <UINT>(-1);
            descCopy.Texture2D.MostDetailedMip =                     0;
          }
  
          HRESULT hr =
            DXGI_ERROR_INVALID_CALL;

          try {
            hr =
              D3D11Dev_CreateShaderResourceView_Original (
                This,        pResource,
                  &descCopy, ppSRView                    );
          }

          catch ( const _com_error& eCOM )
          {
            SK_LOG0 ( ( L"!! COM Error During "
                        L"CreateShaderResourceView (...) - '%s'",
                          eCOM.ErrorMessage ()
                      ),L"   DXGI   " );
          }
  
          if (SUCCEEDED (hr))
          {
            return hr;
          }
        }
      }
    }
  }

  const HRESULT hr =
    D3D11Dev_CreateShaderResourceView_Original ( This, pResource,
                                                 pDesc, ppSRView );

  return hr;
}

__declspec (noinline)
HRESULT
WINAPI
D3D11Dev_CreateDepthStencilView_Override (
  _In_            ID3D11Device                  *This,
  _In_            ID3D11Resource                *pResource,
  _In_opt_  const D3D11_DEPTH_STENCIL_VIEW_DESC *pDesc,
  _Out_opt_       ID3D11DepthStencilView        **ppDepthStencilView )
{
  HRESULT hr =
    E_UNEXPECTED;

  if ( pDesc     != nullptr &&
       pResource != nullptr )
  {
    D3D11_RESOURCE_DIMENSION dim;
    pResource->GetType     (&dim);

    if (dim == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
    {
      DXGI_FORMAT                 newFormat (pDesc->Format);
      CComQIPtr <ID3D11Texture2D> pTex      (pResource);

      if (pTex != nullptr)
      {
        D3D11_TEXTURE2D_DESC tex_desc;
             pTex->GetDesc (&tex_desc);

        auto descCopy =
          *pDesc;

        if ( SK_D3D11_OverrideDepthStencil (newFormat) )
        {
          descCopy.Format = newFormat;

          hr = 
            D3D11Dev_CreateDepthStencilView_Original (
              This, pResource,
                &descCopy,
                  ppDepthStencilView
            );
        }

        if (SUCCEEDED (hr))
          return hr;
      }
    }
  }

  hr =
    D3D11Dev_CreateDepthStencilView_Original ( This, pResource,
                                                 pDesc, ppDepthStencilView );
  return hr;
}

__declspec (noinline)
HRESULT
WINAPI
D3D11Dev_CreateUnorderedAccessView_Override (
  _In_            ID3D11Device                     *This,
  _In_            ID3D11Resource                   *pResource,
  _In_opt_  const D3D11_UNORDERED_ACCESS_VIEW_DESC *pDesc,
  _Out_opt_       ID3D11UnorderedAccessView       **ppUAView )
{
  if ( pDesc     != nullptr &&
       pResource != nullptr )
  {
    D3D11_RESOURCE_DIMENSION dim;
    pResource->GetType     (&dim);

    if (dim == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
    {
      DXGI_FORMAT                 newFormat (pDesc->Format);
      CComQIPtr <ID3D11Texture2D> pTex      (pResource);

      if (pTex != nullptr)
      {
        bool                 override = false;

        D3D11_TEXTURE2D_DESC tex_desc = { };
             pTex->GetDesc (&tex_desc);

        if ( SK_D3D11_OverrideDepthStencil (newFormat) )
          override = true;

        if (override)
        {
          auto descCopy =
            *pDesc;

          descCopy.Format = newFormat;

          const HRESULT hr = 
            D3D11Dev_CreateUnorderedAccessView_Original ( This, pResource,
                                                            &descCopy, ppUAView );

          if (SUCCEEDED (hr))
            return hr;
        }
      }
    }
  }

  const HRESULT hr =
    D3D11Dev_CreateUnorderedAccessView_Original ( This, pResource,
                                                    pDesc, ppUAView );
  return hr;
}


std::unordered_set <ID3D11Texture2D *> render_tex;

bool b_66b35959 = false;
bool b_9d665ae2 = false;
bool b_b21c8ab9 = false;
bool b_6bb0972d = false;
bool b_05da09bd = true;

void
WINAPI
D3D11_PSSetSamplers_Override
(
  _In_     ID3D11DeviceContext       *This,
  _In_     UINT                       StartSlot,
  _In_     UINT                       NumSamplers,
  _In_opt_ ID3D11SamplerState *const *ppSamplers ) 
{
#if 0
  if ( ppSamplers != nullptr )
  {
    //if (SK_GetCurrentRenderBackend ().d3d11.immediate_ctx.IsEqualObject (This))
    if (SK_GetCurrentRenderBackend ().device.p != nullptr)
    {
      SK_TLS *pTLS =
        SK_TLS_Bottom ();

      ID3D11SamplerState** pSamplerCopy =
        (ID3D11SamplerState **)pTLS->scratch_memory.cmd.alloc (
           sizeof (ID3D11SamplerState  *) * 4096
        );

      bool ys8_wrap_ui  = false,
           ys8_clamp_ui = false;

      if (SK_GetCurrentGameID () == SK_GAME_ID::Ys_Eight)
      {
        SK_D3D11_EnableTracking = true;

        auto HashFromCtx =
          [] ( std::array <uint32_t, SK_D3D11_MAX_DEV_CONTEXTS+1>& registry,
               UINT                                                dev_idx ) ->
        uint32_t
        {
          return registry [dev_idx];
        };

        UINT dev_idx = SK_D3D11_GetDeviceContextHandle (This);

        uint32_t current_ps = HashFromCtx (SK_D3D11_Shaders.pixel.current.shader,  dev_idx);
        uint32_t current_vs = HashFromCtx (SK_D3D11_Shaders.vertex.current.shader, dev_idx);

        switch (current_ps)
        {
          case 0x66b35959:
          case 0x9d665ae2:
          case 0xb21c8ab9:
          case 0x05da09bd:
          {
            if (current_ps == 0x66b35959 && b_66b35959)                             ys8_clamp_ui = true;
            if (current_ps == 0x9d665ae2 && b_9d665ae2)                             ys8_clamp_ui = true;
            if (current_ps == 0xb21c8ab9 && b_b21c8ab9)                             ys8_clamp_ui = true;
            if (current_ps == 0x05da09bd && b_05da09bd && current_vs == 0x7759c300) ys8_clamp_ui = true;
          }break;
          case 0x6bb0972d:
          {
            if (current_ps == 0x6bb0972d && b_6bb0972d) ys8_wrap_ui = true;
          } break;
        }
      }

      if (! (pTLS->imgui.drawing || ys8_clamp_ui || ys8_wrap_ui))
      {
        for ( UINT i = 0 ; i < NumSamplers ; i++ )
        {
          pSamplerCopy [i] = ppSamplers [i];

          if (ppSamplers [i] != nullptr)
          {
            D3D11_SAMPLER_DESC        new_desc = { };
            ppSamplers [i]->GetDesc (&new_desc);

            ((ID3D11Device *)SK_GetCurrentRenderBackend ().device.p)->CreateSamplerState (
                                                     &new_desc,
                                                       &pSamplerCopy [i] );
          }
        }
      }

      else
      {
        for ( UINT i = 0 ; i < NumSamplers ; i++ )
        {
          if (! ys8_wrap_ui)
            pSamplerCopy [i] = pTLS->d3d11.uiSampler_clamp;
          else
            pSamplerCopy [i] = pTLS->d3d11.uiSampler_wrap;
        }
      }

      return
        D3D11_PSSetSamplers_Original (This, StartSlot, NumSamplers, pSamplerCopy);
    }
  }
#endif

  D3D11_PSSetSamplers_Original (This, StartSlot, NumSamplers, ppSamplers);
}

const wchar_t*
SK_D3D11_FilterToStr (D3D11_FILTER filter)
{
  switch (filter)
  {
    case D3D11_FILTER_MIN_MAG_MIP_POINT                          : return L"D3D11_FILTER_MIN_MAG_MIP_POINT";
    case D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR                   : return L"D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR";
    case D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT             : return L"D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT";
    case D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR                   : return L"D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR";
    case D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT                   : return L"D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT";
    case D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR            : return L"D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR";
    case D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT                   : return L"D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT";
    case D3D11_FILTER_MIN_MAG_MIP_LINEAR                         : return L"D3D11_FILTER_MIN_MAG_MIP_LINEAR";
    case D3D11_FILTER_ANISOTROPIC                                : return L"D3D11_FILTER_ANISOTROPIC";
    case D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT               : return L"D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT";
    case D3D11_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR        : return L"D3D11_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR";
    case D3D11_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT  : return L"D3D11_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT";
    case D3D11_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR        : return L"D3D11_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR";
    case D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT        : return L"D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT";
    case D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR : return L"D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR";
    case D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT        : return L"D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT";
    case D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR              : return L"D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR";
    case D3D11_FILTER_COMPARISON_ANISOTROPIC                     : return L"D3D11_FILTER_COMPARISON_ANISOTROPIC";
    case D3D11_FILTER_MINIMUM_MIN_MAG_MIP_POINT                  : return L"D3D11_FILTER_MINIMUM_MIN_MAG_MIP_POINT";
    case D3D11_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR           : return L"D3D11_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR";
    case D3D11_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT     : return L"D3D11_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT";
    case D3D11_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR           : return L"D3D11_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR";
    case D3D11_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT           : return L"D3D11_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT";
    case D3D11_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR    : return L"D3D11_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR";
    case D3D11_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT           : return L"D3D11_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT";
    case D3D11_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR                 : return L"D3D11_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR";
    case D3D11_FILTER_MINIMUM_ANISOTROPIC                        : return L"D3D11_FILTER_MINIMUM_ANISOTROPIC";
    case D3D11_FILTER_MAXIMUM_MIN_MAG_MIP_POINT                  : return L"D3D11_FILTER_MAXIMUM_MIN_MAG_MIP_POINT";
    case D3D11_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR           : return L"D3D11_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR";
    case D3D11_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT     : return L"D3D11_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT";
    case D3D11_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR           : return L"D3D11_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR";
    case D3D11_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT           : return L"D3D11_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT";
    case D3D11_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR    : return L"D3D11_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR";
    case D3D11_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT           : return L"D3D11_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT";
    case D3D11_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR                 : return L"D3D11_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR";
    case D3D11_FILTER_MAXIMUM_ANISOTROPIC                        : return L"D3D11_FILTER_MAXIMUM_ANISOTROPIC";
    default: return L"UNKNOWN";
  }
}

HRESULT
WINAPI
D3D11Dev_CreateSamplerState_Override
(
  _In_            ID3D11Device        *This,
  _In_      const D3D11_SAMPLER_DESC  *pSamplerDesc,
  _Out_opt_       ID3D11SamplerState **ppSamplerState )
{
  D3D11_SAMPLER_DESC new_desc = *pSamplerDesc;

  if (SK_GetCurrentGameID () == SK_GAME_ID::Shenmue)
  {
    config.textures.d3d11.uncompressed_mips = true;
    config.textures.d3d11.cache_gen_mips    = true;
    config.textures.d3d11.generate_mips     = true;

    ///dll_log.Log ( L"CreateSamplerState - Filter: %s, MaxAniso: %lu, MipLODBias: %f, MinLOD: %f, MaxLOD: %f, Comparison: %x, U:%x,V:%x,W:%x - %ws",
    ///             SK_D3D11_FilterToStr (new_desc.Filter), new_desc.MaxAnisotropy, new_desc.MipLODBias, new_desc.MinLOD, new_desc.MaxLOD,
    ///             new_desc.ComparisonFunc, new_desc.AddressU, new_desc.AddressV, new_desc.AddressW, SK_SummarizeCaller ().c_str () );

    if (new_desc.Filter != D3D11_FILTER_MIN_MAG_MIP_POINT)
    {
      //if ( new_desc.ComparisonFunc == D3D11_COMPARISON_ALWAYS /*&&
           //new_desc.MaxLOD         == D3D11_FLOAT32_MAX        */)
      //{
        new_desc.MaxAnisotropy =  16; 
        new_desc.Filter        =  D3D11_FILTER_ANISOTROPIC;
        new_desc.MaxLOD        =  D3D11_FLOAT32_MAX;
        new_desc.MinLOD        = -D3D11_FLOAT32_MAX;
      //}
    }

    return
      D3D11Dev_CreateSamplerState_Original (This, &new_desc, ppSamplerState);
  }

  if (SK_GetCurrentGameID () == SK_GAME_ID::Yakuza0)
  {
    extern bool __SK_Y0_FixAniso;
    extern bool __SK_Y0_ClampLODBias;
    extern int  __SK_Y0_ForceAnisoLevel;

    if (__SK_Y0_ClampLODBias)
    {
      new_desc.MipLODBias = std::max (0.0f, new_desc.MipLODBias);
    }

    if (__SK_Y0_ForceAnisoLevel != 0)
    {
      new_desc.MinLOD        = -D3D11_FLOAT32_MAX;
      new_desc.MaxLOD        =  D3D11_FLOAT32_MAX;

      new_desc.MaxAnisotropy = __SK_Y0_ForceAnisoLevel;
    }

    if (__SK_Y0_FixAniso)
    {
      if (new_desc.Filter <= D3D11_FILTER_ANISOTROPIC)
      {
        if (new_desc.MaxAnisotropy > 1)
        {
          new_desc.Filter = D3D11_FILTER_ANISOTROPIC;
        }
      }

      if (new_desc.Filter > D3D11_FILTER_ANISOTROPIC && new_desc.ComparisonFunc == 4)
      {
        if (new_desc.MaxAnisotropy > 1)
        {
          new_desc.Filter = D3D11_FILTER_COMPARISON_ANISOTROPIC;
        }
      }
    }

    const HRESULT hr =
      D3D11Dev_CreateSamplerState_Original (This, &new_desc, ppSamplerState);

    if (SUCCEEDED (hr))
      return hr;
  }

  if (SK_GetCurrentGameID () == SK_GAME_ID::LEGOMarvelSuperheroes2)
  {
    if (new_desc.Filter <= D3D11_FILTER_ANISOTROPIC)
    {
      new_desc.Filter        = D3D11_FILTER_ANISOTROPIC;
      new_desc.MaxAnisotropy = 16;

      new_desc.MipLODBias    = 0.0f;
      new_desc.MinLOD        = 0.0f;
      new_desc.MaxLOD        = D3D11_FLOAT32_MAX;

      const HRESULT hr =
        D3D11Dev_CreateSamplerState_Original (This, &new_desc, ppSamplerState);

      if (SUCCEEDED (hr))
        return hr;
    }
  }

  if (SK_GetCurrentGameID () != SK_GAME_ID::Ys_Eight)
  {
    if (config.textures.d3d11.generate_mips && new_desc.Filter <= D3D11_FILTER_ANISOTROPIC)
    {
      //if (new_desc.Filter != D3D11_FILTER_MIN_MAG_MIP_POINT)
      {
        new_desc.Filter        = D3D11_FILTER_ANISOTROPIC;
        new_desc.MaxAnisotropy = 16;
    
        if (new_desc.MipLODBias < 0.0f)
          new_desc.MipLODBias   = 0.0f;
    
        new_desc.MinLOD        = 0.0f;
        new_desc.MaxLOD        = D3D11_FLOAT32_MAX;
      }
    
      const HRESULT hr =
        D3D11Dev_CreateSamplerState_Original (This, &new_desc, ppSamplerState);
    
      if (SUCCEEDED (hr))
        return hr;
    }
    
    if ( config.textures.d3d11.generate_mips                          &&
          ( ( new_desc.Filter >  D3D11_FILTER_ANISOTROPIC &&
              new_desc.Filter <= D3D11_FILTER_COMPARISON_ANISOTROPIC) ||
            new_desc.ComparisonFunc != D3D11_COMPARISON_NEVER ) )
    {
      new_desc.Filter        = D3D11_FILTER_COMPARISON_ANISOTROPIC;
      new_desc.MaxAnisotropy = 16;
    
      if (pSamplerDesc->Filter != new_desc.Filter)
      {
        SK_LOG0 ( ( L"Changing Shadow Filter from '%s' to '%s'",
                      SK_D3D11_DescribeFilter (pSamplerDesc->Filter),
                           SK_D3D11_DescribeFilter (new_desc.Filter) ),
                    L" TexCache " );
      }
    
      const HRESULT hr =
        D3D11Dev_CreateSamplerState_Original (This, &new_desc, ppSamplerState);
    
      if (SUCCEEDED (hr))
        return hr;
    }
  }

  return
    D3D11Dev_CreateSamplerState_Original (This, pSamplerDesc, ppSamplerState);
}


#ifdef _SK_WITHOUT_D3DX11
[[deprecated]]
#endif
HRESULT
SK_D3DX11_SAFE_GetImageInfoFromFileW (const wchar_t* wszTex, D3DX11_IMAGE_INFO* pInfo) 
{
  UNREFERENCED_PARAMETER (wszTex), UNREFERENCED_PARAMETER (pInfo);
#ifndef _SK_WITHOUT_D3DX11
  __try {
    return D3DX11GetImageInfoFromFileW (wszTex, nullptr, pInfo, nullptr);
  }

  __except ( /* GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ? */
               EXCEPTION_EXECUTE_HANDLER /* :
               EXCEPTION_CONTINUE_SEARCH */ )
  {
    SK_LOG0 ( ( L"Texture '%s' is corrupt, please delete it.",
                  wszTex ),
                L" TexCache " );

    return E_FAIL;
  }
#else
  return E_NOTIMPL;
#endif
}

#ifdef _SK_WITHOUT_D3DX11
[[deprecated]]
#endif
HRESULT
SK_D3DX11_SAFE_CreateTextureFromFileW ( ID3D11Device*           pDevice,   LPCWSTR           pSrcFile,
                                        D3DX11_IMAGE_LOAD_INFO* pLoadInfo, ID3D11Resource** ppTexture ) 
{
  UNREFERENCED_PARAMETER (pDevice),   UNREFERENCED_PARAMETER (pSrcFile),
  UNREFERENCED_PARAMETER (pLoadInfo), UNREFERENCED_PARAMETER (ppTexture);

#ifndef _SK_WITHOUT_D3DX11
  __try {
    return D3DX11CreateTextureFromFileW (pDevice, pSrcFile, pLoadInfo, nullptr, ppTexture, nullptr);
  }

  __except ( GetExceptionCode () == EXCEPTION_ACCESS_VIOLATION ?
               EXCEPTION_EXECUTE_HANDLER :
               EXCEPTION_CONTINUE_SEARCH )
  {
    return E_FAIL;
  }
#else
  return E_NOTIMPL;
#endif
}


HRESULT
SK_D3D11_ReloadTexture ( ID3D11Texture2D* pTex,
                         SK_TLS*          pTLS = SK_TLS_Bottom () )
{
  static auto& textures =
    SK_D3D11_Textures;

  static const
    SK_RenderBackend& rb =
      SK_GetCurrentRenderBackend ();

  HRESULT hr =
    E_UNEXPECTED;

  SK_ScopedBool auto_bool  (&pTLS->texture_management.injection_thread);
  SK_ScopedBool auto_bool2 (&pTLS->imgui.drawing);

  pTLS->texture_management.injection_thread = true;
  pTLS->imgui.drawing                       = true;
  {
    SK_D3D11_TexMgr::tex2D_descriptor_s texDesc2D =
      textures.Textures_2D [pTex];

    std::wstring fname = 
      SK_D3D11_TexNameFromChecksum (texDesc2D.crc32c, 0x00);

    if (fname.empty ()) fname = texDesc2D.file_name;

    else
      texDesc2D.file_name = fname;

    if (GetFileAttributes (fname.c_str ()) != INVALID_FILE_ATTRIBUTES )
    {
#define D3DX11_DEFAULT static_cast <UINT> (-1)

      D3DX11_IMAGE_INFO      img_info  = { };
      D3DX11_IMAGE_LOAD_INFO load_info = { };

      const LARGE_INTEGER   load_start =
        SK_QueryPerf ();

	  DirectX::TexMetadata mdata;
	  
      if ( SUCCEEDED (
		    ( hr = DirectX::GetMetadataFromDDSFile ( fname.c_str (),
				                                         0x0,    mdata )
	      )            )
		     )
      {
        load_info.BindFlags      = texDesc2D.desc.BindFlags;
        load_info.CpuAccessFlags = texDesc2D.desc.CPUAccessFlags;
        load_info.Depth          = texDesc2D.desc.ArraySize;
        load_info.Filter         = D3DX11_DEFAULT;
        load_info.FirstMipLevel  = 0;
      
        if (config.textures.d3d11.injection_keeps_fmt)
          load_info.Format       = texDesc2D.desc.Format;
        else
          load_info.Format       = mdata.format;

        load_info.Height         = texDesc2D.desc.Height;
        load_info.MipFilter      = D3DX11_DEFAULT;
        load_info.MipLevels      = texDesc2D.desc.MipLevels;
        load_info.MiscFlags      = texDesc2D.desc.MiscFlags;
        load_info.Usage          = D3D11_USAGE_DEFAULT;
        load_info.Width          = texDesc2D.desc.Width;

		    DirectX::ScratchImage scratch;

        CComPtr   <ID3D11Texture2D> pInjTex = nullptr;
        CComQIPtr <ID3D11Device>    pDev (rb.device);

        hr =
		      DirectX::LoadFromDDSFile (fname.c_str (), 0x0, &mdata, scratch);

        if (SUCCEEDED (hr))
        {
		      if ( SUCCEEDED (
			         DirectX::CreateTexture (pDev, scratch.GetImages (), scratch.GetImageCount (), mdata, (ID3D11Resource **)&pInjTex.p)
		                     )
			       )
		      {
			      CComQIPtr <ID3D11DeviceContext> pDevCtx (
			        rb.d3d11.immediate_ctx
			      );

			      pDevCtx->CopyResource (pTex, pInjTex);

			      const LARGE_INTEGER load_end =
			        SK_QueryPerf ();

			      textures.Textures_2D [pTex].load_time =
                (load_end.QuadPart - load_start.QuadPart);

			      return S_OK;
		      }
        }
      }
    }
  }

  SK_LOG0 ( ( L" >> Texture Reload Failure (HRESULT: %x)", hr),
              L"DX11TexMgr" );

  return
    E_FAIL;
}


int
SK_D3D11_ReloadAllTextures (void)
{
  static auto& textures =
    SK_D3D11_Textures;

  SK_D3D11_PopulateResourceList (true);

  int count = 0;

  for ( auto& it : textures.Textures_2D )
  {
    if (SK_D3D11_TextureIsCached (it.first))
    {
      if (! (it.second.injected || it.second.discard))
        continue;

      if (SUCCEEDED (SK_D3D11_ReloadTexture (it.first)))
        ++count;
    }
  }

  return count;
}


std::wstring
SK_D3D11_TexNameFromChecksum (uint32_t top_crc32, uint32_t checksum, uint32_t ffx_crc32)
{
  wchar_t wszTex [MAX_PATH * 2 + 1] = { };
  
  static std::wstring res_root (SK_D3D11_res_root);
  static std::wstring eval     (SK_EvalEnvironmentVars (res_root.c_str ()));

  wcsncpy_s ( wszTex,          MAX_PATH * 2,
                eval.c_str (), _TRUNCATE );

  std::wstring hash_name;

  static bool ffx = GetModuleHandle (L"unx.dll") != nullptr;

  if (SK_D3D11_inject_textures_ffx && (! (hash_name = SK_D3D11_TexHashToName (ffx_crc32, 0x00)).empty ()))
  {
    SK_LOG4 ( ( L"Caching texture with crc32: %x", ffx_crc32 ),
                L" Tex Hash " );

    PathAppendW (wszTex, hash_name.c_str ());
  }

  else if (! ( (hash_name = SK_D3D11_TexHashToName (top_crc32, checksum)).empty () &&
               (hash_name = SK_D3D11_TexHashToName (top_crc32, 0x00)    ).empty () ) )
  {
    SK_LOG4 ( ( L"Caching texture with crc32c: %x  (%s) [%s]", top_crc32, hash_name.c_str (), wszTex ),
                L" Tex Hash " );

    PathAppendW (wszTex, hash_name.c_str ());
  }

  else if ( SK_D3D11_inject_textures )
  {
    if ( /*config.textures.d3d11.precise_hash &&*/
         SK_D3D11_IsInjectable (top_crc32, checksum) )
    {
      _swprintf ( wszTex,
                    LR"(%s\inject\textures\%08X_%08X.dds)",
                      wszTex,
                        top_crc32, checksum );
    }

    else if ( SK_D3D11_IsInjectable (top_crc32, 0x00) )
    {
      SK_LOG4 ( ( L"Caching texture with crc32c: %08X", top_crc32 ),
                  L" Tex Hash " );
      _swprintf ( wszTex,
                    LR"(%s\inject\textures\%08X.dds)",
                      wszTex,
                        top_crc32 );
    }

    else if ( ffx &&
              SK_D3D11_IsInjectable_FFX (ffx_crc32) )
    {
      SK_LOG4 ( ( L"Caching texture with crc32: %08X", ffx_crc32 ),
                  L" Tex Hash " );
      _swprintf ( wszTex,
                    LR"(%s\inject\textures\Unx_Old\%08X.dds)",
                      wszTex,
                        ffx_crc32 );
    }

    else *wszTex = L'\0';
  }

  // Not a hashed texture, not an injectable texture, skip it...
  else *wszTex = L'\0';

  SK_FixSlashesW (wszTex);

  return wszTex;
}

//__forceinline
HRESULT
WINAPI
D3D11Dev_CreateTexture2D_Impl (
  _In_              ID3D11Device            *This,
  _Inout_opt_       D3D11_TEXTURE2D_DESC    *pDesc,
  _In_opt_    const D3D11_SUBRESOURCE_DATA  *pInitialData,
  _Out_opt_         ID3D11Texture2D        **ppTexture2D,
                    LPVOID                   lpCallerAddr,
                    SK_TLS                  *pTLS = SK_TLS_Bottom ())
{
  static auto& textures =
    SK_D3D11_Textures;

  BOOL  bIgnoreThisUpload = FALSE;
  if (! bIgnoreThisUpload) bIgnoreThisUpload = SK_D3D11_IsTexInjectThread (pTLS);
  if (! bIgnoreThisUpload) bIgnoreThisUpload = (! (SK_D3D11_cache_textures || SK_D3D11_dump_textures || SK_D3D11_inject_textures));
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


  //// ------------
  static const auto& rb =
    SK_GetCurrentRenderBackend ();
  
  CComQIPtr <ID3D11Device>        pDev    (This);
  CComQIPtr <ID3D11DeviceContext> pDevCtx (rb.d3d11.immediate_ctx);

  if (! (pDev && pDevCtx))
  {
    assert (false);

    return
      D3D11Dev_CreateTexture2D_Original ( This,            pDesc,
                                         pInitialData, ppTexture2D );
  }
  //// -----------

  SK_D3D11_TextureResampler.processFinished (This, pDevCtx, pTLS);

  SK_D3D11_MemoryThreads.mark ();


  DXGI_FORMAT newFormat =
    pDesc->Format;

  if (pInitialData == nullptr || pInitialData->pSysMem == nullptr)
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

  CComPtr <ID3D11Texture2D> pCachedTex = nullptr;

  bool cacheable = ( ppTexture2D             != nullptr &&
                     pInitialData            != nullptr &&
                     pInitialData->pSysMem   != nullptr &&
                     pDesc->SampleDesc.Count == 1       &&
                     pDesc->MiscFlags        == 0x00    &&
                     //pDesc->MiscFlags        != 0x01    &&
                     pDesc->CPUAccessFlags   == 0x0     &&
                     pDesc->Width             > 0       &&
                     pDesc->Height            > 0       &&
                     pDesc->MipLevels         > 0       &&
                     pDesc->ArraySize        == 1 //||
                   //((pDesc->ArraySize  % 6 == 0) && ( pDesc->MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE ))
                   );

  if ( cacheable && pDesc->MipLevels == 0 &&
                    pDesc->MiscFlags == D3D11_RESOURCE_MISC_GENERATE_MIPS )
  {
    SK_LOG0 ( ( L"Skipping Texture because of Mipmap Autogen" ),
                L" TexCache " );
  }

  bool injectable = false;

  cacheable = cacheable &&
    (! (pDesc->BindFlags & ( D3D11_BIND_DEPTH_STENCIL |
                             D3D11_BIND_RENDER_TARGET   )))  &&
       (pDesc->BindFlags & ( D3D11_BIND_SHADER_RESOURCE |
                             D3D11_BIND_UNORDERED_ACCESS  )) &&
        (pDesc->Usage    <   D3D11_USAGE_DYNAMIC); // Cancel out Staging
                                                   //   They will be handled through a
                                                   //     different codepath.

  static const bool __sk_yk0 =
    (SK_GetCurrentGameID () == SK_GAME_ID::Yakuza0);

  if (__sk_yk0)
  {
    if (pDesc->Usage != D3D11_USAGE_IMMUTABLE)
      cacheable = false;
  }

  //
  // Filter out any noise coming from overlays / video capture software
  //
  ///if (SK_GetFramesDrawn () > 0)
  ///  cacheable &= ( pDev.IsEqualObject (This) );

  // XXX: Good idea in theory, but in practice these software packages wrap
  //        the D3D11 device in incompatible ways.


  if (cacheable)
  {
    //dll_log.Log (L"Misc Flags: %x, Bind: %x", pDesc->MiscFlags, pDesc->BindFlags);
  }

  bool gen_mips = false;

  if ( config.textures.d3d11.generate_mips && cacheable && ( pDesc->MipLevels != CalcMipmapLODs (pDesc->Width, pDesc->Height) ) )
  {
    gen_mips = true;
  }


  bool dynamic = false;

  if (config.textures.d3d11.cache && (! cacheable))
  {
    SK_LOG1 ( ( L"Impossible to cache texture (Code Origin: '%s') -- Misc Flags: %x, MipLevels: %lu, "
                L"ArraySize: %lu, CPUAccess: %x, BindFlags: %x, Usage: %x, pInitialData: %08"
                PRIxPTR L" (%08" PRIxPTR L")",
                  SK_GetModuleName (SK_GetCallingDLL (lpCallerAddr)).c_str (), pDesc->MiscFlags, pDesc->MipLevels, pDesc->ArraySize,
                    pDesc->CPUAccessFlags, pDesc->BindFlags, pDesc->Usage, (uintptr_t)pInitialData,
                      pInitialData ? (uintptr_t)pInitialData->pSysMem : (uintptr_t)nullptr
              ),
              L"DX11TexMgr" );

    dynamic = true;
  }

  const bool dumpable = 
              cacheable;

  cacheable =
    cacheable && (! (pDesc->CPUAccessFlags & D3D11_CPU_ACCESS_WRITE));


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
      const bool compressed =
        SK_D3D11_IsFormatCompressed (pDesc->Format);

      // If this isn't an injectable texture, then filter out non-mipmapped
      //   textures.
      if ((! injectable) && cache_opts.ignore_non_mipped)
        cacheable &= (pDesc->MipLevels > 1 || compressed);

      if (cacheable)
      {
        cache_tag  =
          safe_crc32c (top_crc32, (uint8_t *)(pDesc), sizeof D3D11_TEXTURE2D_DESC);

        pCachedTex =
          textures.getTexture2D ( cache_tag, pDesc, nullptr, nullptr,
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
    //dll_log.Log ( L"[DX11TexMgr] >> Redundant 2D Texture Load "
                  //L" (Hash=0x%08X [%05.03f MiB]) <<",
                  //checksum, (float)size / (1024.0f * 1024.0f) );

    (*ppTexture2D = pCachedTex)->AddRef ();

    return S_OK;
  }

  // The concept of a cache-miss only applies if the texture had data at the time
  //   of creation...
  if ( cacheable )
  {
    bool
    WINAPI
    SK_XInput_PulseController ( INT   iJoyID,
                                float fStrengthLeft,
                                float fStrengthRight );

    if (config.textures.cache.vibrate_on_miss)
      SK_XInput_PulseController (0, 1.0f, 0.0f);

    textures.CacheMisses_2D++;
  }


  const LARGE_INTEGER load_start =
    SK_QueryPerf ();

  if (cacheable)
  {
    if (SK_D3D11_res_root.length ())
    {
      wchar_t   wszTex [MAX_PATH * 2] = { };
      wcsncpy ( wszTex,
                  SK_D3D11_TexNameFromChecksum (top_crc32, checksum, ffx_crc32).c_str (),
                    MAX_PATH );

      if (                   *wszTex  != L'\0' &&
           GetFileAttributes (wszTex) != INVALID_FILE_ATTRIBUTES )
      {
#define D3DX11_DEFAULT static_cast <UINT> (-1)

        SK_ScopedBool auto_tex_inject (&pTLS->texture_management.injection_thread);
        SK_ScopedBool auto_draw       (&pTLS->imgui.drawing);

        pTLS->texture_management.injection_thread = true;
        pTLS->imgui.drawing                       = true;

        HRESULT hr = E_UNEXPECTED;

        // To allow texture reloads, we cannot allow immutable usage on these textures.
        //
        if (pDesc->Usage == D3D11_USAGE_IMMUTABLE)
          pDesc->Usage    = D3D11_USAGE_DEFAULT;

        DirectX::TexMetadata mdata;

        if (SUCCEEDED ((hr = DirectX::GetMetadataFromDDSFile (wszTex, 0, mdata))))
        {
          DirectX::ScratchImage img;

          if (SUCCEEDED ((hr = DirectX::LoadFromDDSFile (wszTex, 0, &mdata, img))))
          {
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

              if (pDesc->Usage == D3D11_USAGE_IMMUTABLE)
              {
                pDesc->Usage    = D3D11_USAGE_DEFAULT;
                orig_desc.Usage = D3D11_USAGE_DEFAULT;
              }

              size =
                SK_D3D11_ComputeTextureSize (pDesc);

              SK_AutoCriticalSection critical (&cache_cs);

              textures.refTexture2D (
                *ppTexture2D,
                  pDesc,
                    cache_tag,
                      size,
                        load_end.QuadPart - load_start.QuadPart,
                          top_crc32,
                            wszTex,
                              &orig_desc,  (HMODULE)lpCallerAddr,
                                pTLS );


              textures.Textures_2D [*ppTexture2D].injected = true;

              return ( ( hr = S_OK ) );
            }

            else
            {
              SK_LOG0 ( (L"*** Texture '%s' failed DirectX::CreateTexture (...) -- (HRESULT=%s), skipping!", 
                          SK_StripUserNameFromPathW (wszTex), _com_error (hr).ErrorMessage () ),
                         L"DX11TexMgr" );
            }
          }

          else
          {
            SK_LOG0 ( (L"*** Texture '%s' failed DirectX::LoadFromDDSFile (...) -- (HRESULT=%s), skipping!", 
                         SK_StripUserNameFromPathW (wszTex), _com_error (hr).ErrorMessage () ),
                       L"DX11TexMgr" );
          }
        }

        else
        {
          SK_LOG0 ( (L"*** Texture '%s' failed DirectX::GetMetadataFromDDSFile (...) -- (HRESULT=%s), skipping!", 
                       SK_StripUserNameFromPathW (wszTex), _com_error (hr).ErrorMessage () ),
                     L"DX11TexMgr" );
        }
      }
    }
  }


        HRESULT              ret       = E_NOT_VALID_STATE;
  const D3D11_TEXTURE2D_DESC orig_desc = *pDesc;

  //
  // Texture has one mipmap, but we want a full mipmap chain
  //
  //   Be smart about this, stream the other mipmap LODs in over time
  //     and adjust the min/max LOD levels while the texture is incomplete.
  //
  if (gen_mips && SK_GetCurrentGameID () == SK_GAME_ID::Ys_Eight)
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

    SK_ScopedBool auto_tex_inject (&pTLS->texture_management.injection_thread);
    SK_ScopedBool auto_draw       (&pTLS->imgui.drawing);

    pTLS->texture_management.injection_thread = true;
    pTLS->imgui.drawing                       = true;

    CComPtr <ID3D11Texture2D>       pTempTex  = nullptr;

    // We will return this, but when it is returned, it will be missing mipmaps
    //   until the resample job (scheduled onto a worker thread) finishes.
    //
    //   Minimum latency is 1 frame before the texture is `.
    //
    CComPtr <ID3D11Texture2D>     pFinalTex = nullptr;

    const D3D11_TEXTURE2D_DESC original_desc =
      *pDesc;
       pDesc->MipLevels = CalcMipmapLODs (pDesc->Width, pDesc->Height);

    if (pDesc->Usage == D3D11_USAGE_IMMUTABLE)
      pDesc->Usage    = D3D11_USAGE_DEFAULT;

    DirectX::TexMetadata mdata;

    mdata.width      = pDesc->Width;
    mdata.height     = pDesc->Height;
    mdata.depth      = (pDesc->MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE) ? 6 : 1;
    mdata.arraySize  = 1;
    mdata.mipLevels  = 1;
    mdata.miscFlags  = (pDesc->MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE) ? 
                                              DirectX::TEX_MISC_TEXTURECUBE : 0;
    mdata.miscFlags2 = 0;
    mdata.format     = pDesc->Format;
    mdata.dimension  = DirectX::TEX_DIMENSION_TEXTURE2D;


    resample_job_s resample = { };
                   resample.time.preprocess = SK_QueryPerf ().QuadPart;

    auto* image = new DirectX::ScratchImage;
          image->Initialize (mdata);

    bool error = false;

    for (size_t slice = 0; slice < mdata.arraySize; ++slice)
    {
      size_t height = mdata.height;

      for (size_t lod = 0; lod < mdata.mipLevels; ++lod)
      {
        const DirectX::Image* img =
          image->GetImage (lod, slice, 0);

        if (! (img && img->pixels))
        {
          error = true;
          break;
        }

        const size_t lines =
          DirectX::ComputeScanlines (mdata.format, height);

        if (! lines)
        {
          error = true;
          break;
        }

        auto sptr =
          static_cast <const uint8_t *>(
            pInitialData [lod].pSysMem
          );

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
      }

      if (error)
        break;
    }

    const DirectX::Image* orig_img =
      image->GetImage (0, 0, 0);

    const bool compressed =
      SK_D3D11_IsFormatCompressed (pDesc->Format);

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
                                     decompressed->GetMetadata (), reinterpret_cast <ID3D11Resource **> (&pTempTex) );
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
      ret =
        D3D11Dev_CreateTexture2D_Original (This, &original_desc, pInitialData, &pTempTex);
    }

    if (SUCCEEDED (ret))
    {
      ret =
        D3D11Dev_CreateTexture2D_Original (This, pDesc, nullptr, &pFinalTex);

      if (SUCCEEDED (ret))
      {
        D3D11_CopySubresourceRegion_Original (
          pDevCtx, pFinalTex,
            0, 0, 0, 0,
              pTempTex, 0, nullptr
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

      resample.crc32c  = top_crc32;
      resample.data    = image;
      resample.texture = pFinalTex;

      if (resample.data->GetMetadata ().IsCubemap ())
        SK_LOG0 ( (L"Neat, a Cubemap!"), L"DirectXTex" );

      SK_D3D11_TextureResampler.postJob (resample);

      // It's the thread pool's problem now, don't free this.
      image = nullptr;
    }

    delete image;
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
      SK_ScopedBool auto_bool (&pTLS->texture_management.injection_thread);
                                pTLS->texture_management.injection_thread = true;

      SK_D3D11_MipmapCacheTexture2D ((*ppTexture2D), top_crc32, pTLS);

      //SK_D3D11_DumpTexture2D (&orig_desc, pInitialData, top_crc32, checksum);
    }
  }

  cacheable &=
    (SK_D3D11_cache_textures || injectable);

  if ( SUCCEEDED (ret) && cacheable )
  {
    SK_AutoCriticalSection critical (&cache_cs);

    if (! textures.Blacklist_2D [orig_desc.MipLevels].count (checksum))
    {
      textures.refTexture2D (
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

__declspec (noinline)
HRESULT
WINAPI
D3D11Dev_CreateTexture2D_Override (
  _In_            ID3D11Device           *This,
  _In_      const D3D11_TEXTURE2D_DESC   *pDesc,
  _In_opt_  const D3D11_SUBRESOURCE_DATA *pInitialData,
  _Out_opt_       ID3D11Texture2D        **ppTexture2D )
{
  if ((! pDesc) || ( pDesc->Width  < 4 &&
                     pDesc->Height < 4    ))
  {
    D3D11_TEXTURE2D_DESC descCopy =
                   pDesc ?
                  *pDesc    :
                   D3D11_TEXTURE2D_DESC { };

    return
      D3D11Dev_CreateTexture2D_Impl (
        This, &descCopy, pInitialData,
          ppTexture2D, _ReturnAddress ()
      );
  }

  const D3D11_TEXTURE2D_DESC* pDescOrig =  pDesc;
                         auto descCopy  = *pDescOrig;

  const HRESULT hr =
    D3D11Dev_CreateTexture2D_Impl (
      This, &descCopy, pInitialData,
            ppTexture2D, _ReturnAddress ()
    );

  if (SUCCEEDED (hr))
  {
    __try {
      *const_cast <D3D11_TEXTURE2D_DESC *> ( pDesc ) =
        descCopy;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    { }


  //if (pDesc && pDesc->Usage == D3D11_USAGE_STAGING)
  //{
  //  dll_log.Log ( L"Code Origin ('%s') - Staging: %lux%lu - Format: %s, CPU Access: %x, Misc Flags: %x",
  //                  SK_GetCallerName ().c_str (), pDesc->Width, pDesc->Height,
  //                  SK_DXGI_FormatToStr          (pDesc->Format).c_str (),
  //                         pDesc->CPUAccessFlags, pDesc->MiscFlags );
  //}
  }

  return hr;
}


void
__stdcall
SK_D3D11_UpdateRenderStatsEx (const IDXGISwapChain* pSwapChain)
{
  if (! (pSwapChain))
    return;

  static const auto& rb =
    SK_GetCurrentRenderBackend ();

  CComQIPtr <ID3D11Device>        pDev    (rb.device);
  CComQIPtr <ID3D11DeviceContext> pDevCtx (rb.d3d11.immediate_ctx);

  if (! (pDev && pDevCtx && rb.swapchain))
    return;

  SK::DXGI::PipelineStatsD3D11& pipeline_stats =
    SK::DXGI::pipeline_stats_d3d11;

  if (ReadPointerAcquire ((volatile PVOID *)&pipeline_stats.query.async) != nullptr)
  {
    if (ReadAcquire (&pipeline_stats.query.active))
    {
      pDevCtx->End ((ID3D11Asynchronous *)ReadPointerAcquire ((volatile PVOID *)&pipeline_stats.query.async));
      InterlockedExchange (&pipeline_stats.query.active, FALSE);
    }

    else
    {
      const HRESULT hr =
        pDevCtx->GetData ( (ID3D11Asynchronous *)ReadPointerAcquire ((volatile PVOID *)&pipeline_stats.query.async),
                            &pipeline_stats.last_results,
                              sizeof D3D11_QUERY_DATA_PIPELINE_STATISTICS,
                                0x0 );
      if (hr == S_OK)
      {
        ((ID3D11Asynchronous *)ReadPointerAcquire ((volatile PVOID *)&pipeline_stats.query.async))->Release ();
                       InterlockedExchangePointer ((void **)         &pipeline_stats.query.async, nullptr);
      }
    }
  }

  else
  {
    D3D11_QUERY_DESC query_desc
      {  D3D11_QUERY_PIPELINE_STATISTICS,
         0x00                             };

    ID3D11Query* pQuery = nullptr;

    if (SUCCEEDED (pDev->CreateQuery (&query_desc, &pQuery)))
    {
      InterlockedExchangePointer ((void **)&pipeline_stats.query.async,  pQuery);
      pDevCtx->Begin             (                                       pQuery);
      InterlockedExchange        (         &pipeline_stats.query.active, TRUE);
    }
  }
}

void
__stdcall
SK_D3D11_UpdateRenderStats (IDXGISwapChain* pSwapChain)
{
  if (! (pSwapChain && ( config.render.show ||
                          ( SK_ImGui_Widgets.d3d11_pipeline != nullptr &&
                          ( SK_ImGui_Widgets.d3d11_pipeline->isActive () ) ) ) ) )
    return;

  SK_D3D11_UpdateRenderStatsEx (pSwapChain);
}

std::wstring
SK_CountToString (uint64_t count)
{
  wchar_t str [64] = { };

  unsigned int unit = 0;

  if      (count > 1000000000UL) unit = 1000000000UL;
  else if (count > 1000000)      unit = 1000000UL;
  else if (count > 1000)         unit = 1000UL;
  else                           unit = 1UL;

  switch (unit)
  {
    case 1000000000UL:
      _swprintf (str, L"%6.2f Billion ", (float)count / (float)unit);
      break;
    case 1000000UL:
      _swprintf (str, L"%6.2f Million ", (float)count / (float)unit);
      break;
    case 1000UL:
      _swprintf (str, L"%6.2f Thousand", (float)count / (float)unit);
      break;
    case 1UL:
    default:
      _swprintf (str, L"%15llu", count);
      break;
  }

  return str;
}

void
SK_D3D11_SetPipelineStats (void* pData) 
{
  memcpy ( &SK::DXGI::pipeline_stats_d3d11.last_results,
             pData,
               sizeof D3D11_QUERY_DATA_PIPELINE_STATISTICS );
}

void
SK_D3D11_GetVertexPipelineDesc (wchar_t* wszDesc)
{
  const D3D11_QUERY_DATA_PIPELINE_STATISTICS& stats =
     SK::DXGI::pipeline_stats_d3d11.last_results;

  if (stats.VSInvocations > 0)
  {
    _swprintf ( wszDesc,
                 L"  VERTEX : %s   (%s Verts ==> %s Triangles)",
                   SK_CountToString (stats.VSInvocations).c_str (),
                     SK_CountToString (stats.IAVertices).c_str (),
                       SK_CountToString (stats.IAPrimitives).c_str () );
  }

  else
  {
    _swprintf ( wszDesc,
                 L"  VERTEX : <Unused>" );
  }
}

void
SK_D3D11_GetGeometryPipelineDesc (wchar_t* wszDesc)
{
  const D3D11_QUERY_DATA_PIPELINE_STATISTICS& stats =
     SK::DXGI::pipeline_stats_d3d11.last_results;

  if (stats.GSInvocations > 0)
  {
    _swprintf ( wszDesc,
                 L"%s  GEOM   : %s   (%s Prims)",
                   wszDesc,
                     SK_CountToString (stats.GSInvocations).c_str (),
                       SK_CountToString (stats.GSPrimitives).c_str () );
  }

  else
  {
    _swprintf ( wszDesc,
                 L"%s  GEOM   : <Unused>",
                   wszDesc );
  }
}

void
SK_D3D11_GetTessellationPipelineDesc (wchar_t* wszDesc)
{
  const D3D11_QUERY_DATA_PIPELINE_STATISTICS& stats =
     SK::DXGI::pipeline_stats_d3d11.last_results;

  if (stats.HSInvocations > 0 || stats.DSInvocations > 0)
  {
    _swprintf ( wszDesc,
                 L"%s  TESS   : %s Hull ==> %s Domain",
                   wszDesc,
                     SK_CountToString (stats.HSInvocations).c_str (),
                       SK_CountToString (stats.DSInvocations).c_str () ) ;
  }

  else
  {
    _swprintf ( wszDesc,
                 L"%s  TESS   : <Unused>",
                   wszDesc );
  }
}

void
SK_D3D11_GetRasterPipelineDesc (wchar_t* wszDesc)
{
  const D3D11_QUERY_DATA_PIPELINE_STATISTICS& stats =
     SK::DXGI::pipeline_stats_d3d11.last_results;

  if (stats.CInvocations > 0)
  {
    _swprintf ( wszDesc,
                 L"%s  RASTER : %5.1f%% Filled     (%s Triangles IN )",
                   wszDesc, 100.0 *
                       ( static_cast <double> (stats.CPrimitives) /
                         static_cast <double> (stats.CInvocations) ),
                     SK_CountToString (stats.CInvocations).c_str () );
  }

  else
  {
    _swprintf ( wszDesc,
                 L"%s  RASTER : <Unused>",
                   wszDesc );
  }
}

void
SK_D3D11_GetPixelPipelineDesc (wchar_t* wszDesc)
{
  const D3D11_QUERY_DATA_PIPELINE_STATISTICS& stats =
     SK::DXGI::pipeline_stats_d3d11.last_results;

  if (stats.PSInvocations > 0)
  {
    _swprintf ( wszDesc,
                 L"%s  PIXEL  : %s   (%s Triangles OUT)",
                   wszDesc,
                     SK_CountToString (stats.PSInvocations).c_str (),
                       SK_CountToString (stats.CPrimitives).c_str () );
  }

  else
  {
    _swprintf ( wszDesc,
                 L"%s  PIXEL  : <Unused>",
                   wszDesc );
  }
}

void
SK_D3D11_GetComputePipelineDesc (wchar_t* wszDesc)
{
  const D3D11_QUERY_DATA_PIPELINE_STATISTICS& stats =
     SK::DXGI::pipeline_stats_d3d11.last_results;

  if (stats.CSInvocations > 0)
  {
    _swprintf ( wszDesc,
                 L"%s  COMPUTE: %s",
                   wszDesc, SK_CountToString (stats.CSInvocations).c_str () );
  }

  else 
  {
    _swprintf ( wszDesc,
                 L"%s  COMPUTE: <Unused>",
                   wszDesc );
  }
}

std::wstring
SK::DXGI::getPipelineStatsDesc (void)
{
  wchar_t wszDesc [1024] = { };

  //
  // VERTEX SHADING
  //
  SK_D3D11_GetVertexPipelineDesc (wszDesc);       lstrcatW (wszDesc, L"\n");

  //
  // GEOMETRY SHADING
  //
  SK_D3D11_GetGeometryPipelineDesc (wszDesc);     lstrcatW (wszDesc, L"\n");

  //
  // TESSELLATION
  //
  SK_D3D11_GetTessellationPipelineDesc (wszDesc); lstrcatW (wszDesc, L"\n");

  //
  // RASTERIZATION
  //
  SK_D3D11_GetRasterPipelineDesc (wszDesc);       lstrcatW (wszDesc, L"\n");

  //
  // PIXEL SHADING
  //
  SK_D3D11_GetPixelPipelineDesc (wszDesc);        lstrcatW (wszDesc, L"\n");

  //
  // COMPUTE
  //
  SK_D3D11_GetComputePipelineDesc (wszDesc);      lstrcatW (wszDesc, L"\n");

  return wszDesc;
}


#include <SpecialK/resource.h>

auto SK_UnpackD3DX11 =
[](void) -> void
{
#ifndef _SK_WITHOUT_D3DX11
  HMODULE hModSelf = 
    SK_GetDLL ();

  HRSRC res =
    FindResource ( hModSelf, MAKEINTRESOURCE (IDR_D3DX11_PACKAGE), L"7ZIP" );

  if (res)
  {
    SK_LOG0 ( ( L"Unpacking D3DX11_43.dll because user does not have June 2010 DirectX Redistributables installed." ),
                L"D3DCompile" );

    DWORD   res_size     =
      SizeofResource ( hModSelf, res );

    HGLOBAL packed_d3dx11 =
      LoadResource   ( hModSelf, res );

    if (! packed_d3dx11) return;


    const void* const locked =
      (void *)LockResource (packed_d3dx11);


    if (locked != nullptr)
    {
      wchar_t      wszArchive     [MAX_PATH * 2 + 1] = { };
      wchar_t      wszDestination [MAX_PATH * 2 + 1] = { };

      wcscpy (wszDestination, SK_GetHostPath ());

      if (GetFileAttributesW (wszDestination) == INVALID_FILE_ATTRIBUTES)
        SK_CreateDirectories (wszDestination);

      wcscpy      (wszArchive, wszDestination);
      PathAppendW (wszArchive, L"D3DX11_43.7z");

      FILE* fPackedD3DX11 =
        _wfopen   (wszArchive, L"wb");

      fwrite      (locked, 1, res_size, fPackedD3DX11);
      fclose      (fPackedD3DX11);

      using SK_7Z_DECOMP_PROGRESS_PFN = int (__stdcall *)(int current, int total);

      extern
      HRESULT
      SK_Decompress7zEx ( const wchar_t*            wszArchive,
                          const wchar_t*            wszDestination,
                          SK_7Z_DECOMP_PROGRESS_PFN callback );

      SK_Decompress7zEx (wszArchive, wszDestination, nullptr);
      DeleteFileW       (wszArchive);
    }

    UnlockResource (packed_d3dx11);
  }
#endif
};

void
SK_D3D11_InitTextures (void)
{
  static volatile LONG SK_D3D11_tex_init = FALSE;

  SK_TLS* pTLS =
    SK_TLS_Bottom ();

  if (! InterlockedCompareExchange (&SK_D3D11_tex_init, TRUE, FALSE))
  {
    pTLS->d3d11.ctx_init_thread = true;

    if (SK_GetCurrentGameID () == SK_GAME_ID::FinalFantasyX_X2)
      SK_D3D11_inject_textures_ffx = true;

    InitializeCriticalSectionAndSpinCount (&preload_cs, 0x01);
    InitializeCriticalSectionAndSpinCount (&dump_cs,    0x02);
    InitializeCriticalSectionAndSpinCount (&inject_cs,  0x10);
    InitializeCriticalSectionAndSpinCount (&hash_cs,    0x40);
    InitializeCriticalSectionAndSpinCount (&cache_cs,   0x80);
    InitializeCriticalSectionAndSpinCount (&tex_cs,     0xFF);

    cache_opts.max_entries       = config.textures.cache.max_entries;
    cache_opts.min_entries       = config.textures.cache.min_entries;
    cache_opts.max_evict         = config.textures.cache.max_evict;
    cache_opts.min_evict         = config.textures.cache.min_evict;
    cache_opts.max_size          = config.textures.cache.max_size;
    cache_opts.min_size          = config.textures.cache.min_size;
    cache_opts.ignore_non_mipped = config.textures.cache.ignore_nonmipped;

    //
    // Legacy Hack for Untitled Project X (FFX/FFX-2)
    //
    extern bool SK_D3D11_inject_textures_ffx;
    if       (! SK_D3D11_inject_textures_ffx)
    {
      SK_D3D11_EnableTexCache  (config.textures.d3d11.cache);
      SK_D3D11_EnableTexDump   (config.textures.d3d11.dump);
      SK_D3D11_EnableTexInject (config.textures.d3d11.inject);
      SK_D3D11_SetResourceRoot (config.textures.d3d11.res_root.c_str ());
    }

    SK_GetCommandProcessor ()->AddVariable ("TexCache.Enable",
         new SK_IVarStub <bool> ((bool *)&config.textures.d3d11.cache));
    SK_GetCommandProcessor ()->AddVariable ("TexCache.MaxEntries",
         new SK_IVarStub <int> ((int *)&cache_opts.max_entries));
    SK_GetCommandProcessor ()->AddVariable ("TexC ache.MinEntries",
         new SK_IVarStub <int> ((int *)&cache_opts.min_entries));
    SK_GetCommandProcessor ()->AddVariable ("TexCache.MaxSize",
         new SK_IVarStub <int> ((int *)&cache_opts.max_size));
    SK_GetCommandProcessor ()->AddVariable ("TexCache.MinSize",
         new SK_IVarStub <int> ((int *)&cache_opts.min_size));
    SK_GetCommandProcessor ()->AddVariable ("TexCache.MinEvict",
         new SK_IVarStub <int> ((int *)&cache_opts.min_evict));
    SK_GetCommandProcessor ()->AddVariable ("TexCache.MaxEvict",
         new SK_IVarStub <int> ((int *)&cache_opts.max_evict));
    SK_GetCommandProcessor ()->AddVariable ("TexCache.IgnoreNonMipped",
         new SK_IVarStub <bool> ((bool *)&cache_opts.ignore_non_mipped));

    if ((! SK_D3D11_inject_textures_ffx) && config.textures.d3d11.inject)
      SK_D3D11_PopulateResourceList ();


#ifndef _SK_WITHOUT_D3DX11
    if (hModD3DX11_43 == nullptr)
    {
      hModD3DX11_43 =
        SK_Modules.LoadLibrary (L"d3dx11_43.dll");

      if (hModD3DX11_43 == nullptr)
      {
        SK_UnpackD3DX11 ();

        hModD3DX11_43 =
          SK_Modules.LoadLibrary (L"d3dx11_43.dll");

        if (hModD3DX11_43 == nullptr)
            hModD3DX11_43 = (HMODULE)1;
      }
    }

    if ((uintptr_t)hModD3DX11_43 > 1)
    {
      if (D3DX11CreateTextureFromFileW == nullptr)
      {
        D3DX11CreateTextureFromFileW =
          (D3DX11CreateTextureFromFileW_pfn)
            GetProcAddress (hModD3DX11_43, "D3DX11CreateTextureFromFileW");
      }

      if (D3DX11GetImageInfoFromFileW == nullptr)
      {
        D3DX11GetImageInfoFromFileW =
          (D3DX11GetImageInfoFromFileW_pfn)
            GetProcAddress (hModD3DX11_43, "D3DX11GetImageInfoFromFileW");
      }

      if (D3DX11CreateTextureFromMemory == nullptr)
      {
        D3DX11CreateTextureFromMemory =
          (D3DX11CreateTextureFromMemory_pfn)
            GetProcAddress (hModD3DX11_43, "D3DX11CreateTextureFromMemory");
      }
    }
#endif


    if (SK_GetCurrentGameID () == SK_GAME_ID::Okami)
    {
      extern void SK_Okami_LoadConfig (void);
                  SK_Okami_LoadConfig ();
    }

    InterlockedIncrement (&SK_D3D11_tex_init);
  }

  if (pTLS->d3d11.ctx_init_thread)
    return;

  SK_Thread_SpinUntilAtomicMin (&SK_D3D11_tex_init, 2);
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
      static const char* szName = #_Name;                                   \
      _default_impl = (passthrough_pfn)GetProcAddress (backend_dll, szName);\
                                                                            \
      if (_default_impl == nullptr) {                                       \
        dll_log.Log (                                                       \
          L"Unable to locate symbol  %s in d3d11.dll",                      \
          L#_Name);                                                         \
        return (_Return)E_NOTIMPL;                                          \
      }                                                                     \
    }                                                                       \
                                                                            \
    SK_LOG0 ( (L"[!] %s %s - "                                              \
               L"[Calling Thread: 0x%04x]",                                 \
      L#_Name, L#_Proto, GetCurrentThreadId ()),                            \
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
      static const char* szName = #_Name;                                   \
      _default_impl = (passthrough_pfn)GetProcAddress (backend_dll, szName);\
                                                                            \
      if (_default_impl == nullptr) {                                       \
        dll_log.Log (                                                       \
          L"Unable to locate symbol  %s in d3d11.dll",                      \
          L#_Name );                                                        \
        return;                                                             \
      }                                                                     \
    }                                                                       \
                                                                            \
    SK_LOG0 ( (L"[!] %s %s - "                                              \
               L"[Calling Thread: 0x%04x]",                                 \
      L#_Name, L#_Proto, GetCurrentThreadId ()),                            \
                 __SK_SUBSYSTEM__ );                                        \
                                                                            \
    _default_impl _Args;                                                    \
}

bool
SK_D3D11_Init (void)
{
  BOOL success = FALSE;

  if (! InterlockedCompareExchange (&SK_D3D11_initialized, TRUE, FALSE))
  {
    HMODULE hBackend = 
      ( (SK_GetDLLRole () & DLL_ROLE::D3D11) ) ? backend_dll :
                                                  SK_Modules.LoadLibraryLL (L"d3d11.dll");

    SK::DXGI::hModD3D11 = hBackend;

    D3D11CreateDeviceForD3D12              = GetProcAddress (SK::DXGI::hModD3D11, "D3D11CreateDeviceForD3D12");
    CreateDirect3D11DeviceFromDXGIDevice   = GetProcAddress (SK::DXGI::hModD3D11, "CreateDirect3D11DeviceFromDXGIDevice");
    CreateDirect3D11SurfaceFromDXGISurface = GetProcAddress (SK::DXGI::hModD3D11, "CreateDirect3D11SurfaceFromDXGISurface");
    D3D11On12CreateDevice                  = GetProcAddress (SK::DXGI::hModD3D11, "D3D11On12CreateDevice");
    D3DKMTCloseAdapter                     = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTCloseAdapter");
    D3DKMTDestroyAllocation                = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTDestroyAllocation");
    D3DKMTDestroyContext                   = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTDestroyContext");
    D3DKMTDestroyDevice                    = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTDestroyDevice ");
    D3DKMTDestroySynchronizationObject     = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTDestroySynchronizationObject");
    D3DKMTQueryAdapterInfo                 = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTQueryAdapterInfo");
    D3DKMTSetDisplayPrivateDriverFormat    = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTSetDisplayPrivateDriverFormat");
    D3DKMTSignalSynchronizationObject      = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTSignalSynchronizationObject");
    D3DKMTUnlock                           = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTUnlock");
    D3DKMTWaitForSynchronizationObject     = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTWaitForSynchronizationObject");
    EnableFeatureLevelUpgrade              = GetProcAddress (SK::DXGI::hModD3D11, "EnableFeatureLevelUpgrade");
    OpenAdapter10                          = GetProcAddress (SK::DXGI::hModD3D11, "OpenAdapter10");
    OpenAdapter10_2                        = GetProcAddress (SK::DXGI::hModD3D11, "OpenAdapter10_2");
    D3D11CoreCreateLayeredDevice           = GetProcAddress (SK::DXGI::hModD3D11, "D3D11CoreCreateLayeredDevice");
    D3D11CoreGetLayeredDeviceSize          = GetProcAddress (SK::DXGI::hModD3D11, "D3D11CoreGetLayeredDeviceSize");
    D3D11CoreRegisterLayers                = GetProcAddress (SK::DXGI::hModD3D11, "D3D11CoreRegisterLayers");
    D3DKMTCreateAllocation                 = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTCreateAllocation");
    D3DKMTCreateContext                    = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTCreateContext");
    D3DKMTCreateDevice                     = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTCreateDevice");
    D3DKMTCreateSynchronizationObject      = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTCreateSynchronizationObject");
    D3DKMTEscape                           = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTEscape");
    D3DKMTGetContextSchedulingPriority     = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTGetContextSchedulingPriority");
    D3DKMTGetDeviceState                   = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTGetDeviceState");
    D3DKMTGetDisplayModeList               = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTGetDisplayModeList");
    D3DKMTGetMultisampleMethodList         = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTGetMultisampleMethodList");
    D3DKMTGetRuntimeData                   = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTGetRuntimeData");
    D3DKMTGetSharedPrimaryHandle           = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTGetSharedPrimaryHandle");
    D3DKMTLock                             = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTLock");
    D3DKMTOpenAdapterFromHdc               = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTOpenAdapterFromHdc");
    D3DKMTOpenResource                     = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTOpenResource");
    D3DKMTPresent                          = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTPresent");
    D3DKMTQueryAllocationResidency         = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTQueryAllocationResidency");
    D3DKMTQueryResourceInfo                = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTQueryResourceInfo");
    D3DKMTRender                           = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTRender");
    D3DKMTSetAllocationPriority            = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTSetAllocationPriority");
    D3DKMTSetContextSchedulingPriority     = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTSetContextSchedulingPriority");
    D3DKMTSetDisplayMode                   = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTSetDisplayMode");
    D3DKMTSetGammaRamp                     = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTSetGammaRamp");
    D3DKMTSetVidPnSourceOwner              = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTSetVidPnSourceOwner");
    D3DKMTWaitForVerticalBlankEvent        = GetProcAddress (SK::DXGI::hModD3D11, "D3DKMTWaitForVerticalBlankEvent");
    D3DPerformance_BeginEvent              = GetProcAddress (SK::DXGI::hModD3D11, "D3DPerformance_BeginEvent");
    D3DPerformance_EndEvent                = GetProcAddress (SK::DXGI::hModD3D11, "D3DPerformance_EndEvent");
    D3DPerformance_GetStatus               = GetProcAddress (SK::DXGI::hModD3D11, "D3DPerformance_GetStatus");
    D3DPerformance_SetMarker               = GetProcAddress (SK::DXGI::hModD3D11, "D3DPerformance_SetMarker");


    SK_LOG0 ( (L"Importing D3D11CreateDevice[AndSwapChain]"), __SK_SUBSYSTEM__ );
    SK_LOG0 ( (L"========================================="), __SK_SUBSYSTEM__ );

    if (! _wcsicmp (SK_GetModuleName (SK_GetDLL ()).c_str (), L"d3d11.dll"))
    {
      if (! LocalHook_D3D11CreateDevice.active)
      {
        D3D11CreateDevice_Import            =  \
         (D3D11CreateDevice_pfn)               \
           GetProcAddress (hBackend, "D3D11CreateDevice");
      }

      if (! LocalHook_D3D11CreateDeviceAndSwapChain.active)
      {
        D3D11CreateDeviceAndSwapChain_Import            =  \
         (D3D11CreateDeviceAndSwapChain_pfn)               \
           GetProcAddress (hBackend, "D3D11CreateDeviceAndSwapChain");
      }

      SK_LOG0 ( ( L"  D3D11CreateDevice:             %s",
                    SK_MakePrettyAddress (D3D11CreateDevice_Import).c_str () ),
                  __SK_SUBSYSTEM__ );
      SK_LogSymbolName                    (D3D11CreateDevice_Import);

      SK_LOG0 ( ( L"  D3D11CreateDeviceAndSwapChain: %s",
                    SK_MakePrettyAddress (D3D11CreateDeviceAndSwapChain_Import).c_str () ),
                  __SK_SUBSYSTEM__ );
      SK_LogSymbolName                   (D3D11CreateDeviceAndSwapChain_Import);

      pfnD3D11CreateDeviceAndSwapChain = D3D11CreateDeviceAndSwapChain_Import;
      pfnD3D11CreateDevice             = D3D11CreateDevice_Import;

      InterlockedIncrement (&SK_D3D11_initialized);
    }

    else
    {
      if ( LocalHook_D3D11CreateDevice.active ||
          ( MH_OK ==
             SK_CreateDLLHook2 (      L"d3d11.dll",
                                       "D3D11CreateDevice",
                                        D3D11CreateDevice_Detour,
               static_cast_p2p <void> (&D3D11CreateDevice_Import),
                                    &pfnD3D11CreateDevice )
          )
         )
      {
              SK_LOG0 ( ( L"  D3D11CreateDevice:              %s  %s",
        SK_MakePrettyAddress (pfnD3D11CreateDevice ? pfnD3D11CreateDevice :
                                                        D3D11CreateDevice_Import).c_str (),
                              pfnD3D11CreateDevice ? L"{ Hooked }" :
                                                     L"{ Cached }" ),
                        __SK_SUBSYSTEM__ );
      }

      if ( LocalHook_D3D11CreateDeviceAndSwapChain.active ||
          ( MH_OK ==
             SK_CreateDLLHook2 (    L"d3d11.dll",
                                     "D3D11CreateDeviceAndSwapChain",
                                      D3D11CreateDeviceAndSwapChain_Detour,
             static_cast_p2p <void> (&D3D11CreateDeviceAndSwapChain_Import),
                                  &pfnD3D11CreateDeviceAndSwapChain )
          )
         )
      {
            SK_LOG0 ( ( L"  D3D11CreateDeviceAndSwapChain:  %s  %s",
        SK_MakePrettyAddress (pfnD3D11CreateDevice ? pfnD3D11CreateDeviceAndSwapChain :
                                                        D3D11CreateDeviceAndSwapChain_Import).c_str (),
                            pfnD3D11CreateDeviceAndSwapChain ? L"{ Hooked }" :
                                                               L"{ Cached }" ),
                        __SK_SUBSYSTEM__ );
        SK_LogSymbolName     (pfnD3D11CreateDeviceAndSwapChain);

        if ((SK_GetDLLRole () & DLL_ROLE::D3D11) || (SK_GetDLLRole () & DLL_ROLE::DInput8))
        {
          SK_RunLHIfBitness ( 64, SK_LoadPlugIns64 (),
                                  SK_LoadPlugIns32 () );
        }

        if ( ( LocalHook_D3D11CreateDevice.active ||
               MH_OK == MH_QueueEnableHook (pfnD3D11CreateDevice) ) &&
             ( LocalHook_D3D11CreateDeviceAndSwapChain.active ||
               MH_OK == MH_QueueEnableHook (pfnD3D11CreateDeviceAndSwapChain) ) )
        {
          InterlockedIncrement (&SK_D3D11_initialized);
          success = TRUE;//(MH_OK == SK_ApplyQueuedHooks ());
        }
      }

      if (! success)
      {
        SK_LOG0 ( (L"Something went wrong hooking D3D11 -- need better errors."), __SK_SUBSYSTEM__ );
      }
    }

    LocalHook_D3D11CreateDeviceAndSwapChain.target.addr =
      pfnD3D11CreateDeviceAndSwapChain ? pfnD3D11CreateDeviceAndSwapChain :
                                            D3D11CreateDeviceAndSwapChain_Import;
    LocalHook_D3D11CreateDeviceAndSwapChain.active = true;

    LocalHook_D3D11CreateDevice.target.addr             =
      pfnD3D11CreateDevice            ? pfnD3D11CreateDevice :
                                           D3D11CreateDevice_Import;
    LocalHook_D3D11CreateDevice.active = true;
  }

  SK_Thread_SpinUntilAtomicMin (&SK_D3D11_initialized, 2);

  return success;
}

void
SK_D3D11_Shutdown (void)
{
  static auto& textures =
    SK_D3D11_Textures;

  if (! InterlockedCompareExchange (&SK_D3D11_initialized, FALSE, TRUE))
    return;

  if (textures.RedundantLoads_2D > 0)
  {
    SK_LOG0 ( (L"At shutdown: %7.2f seconds and %7.2f MiB of"
                  L" CPU->GPU I/O avoided by %lu texture cache hits.",
                    textures.RedundantTime_2D / 1000.0f,
                      (float)textures.RedundantData_2D.load () /
                                 (1024.0f * 1024.0f),
                             textures.RedundantLoads_2D.load ()),
               L"Perf Stats" );
  }

  textures.reset ();

  // Stop caching while we shutdown
  SK_D3D11_cache_textures = false;

  if (FreeLibrary_Original (SK::DXGI::hModD3D11))
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
  InterlockedExchange (&__d3d11_ready, TRUE);
}


void
SK_D3D11_HookDevCtx (sk_hook_d3d11_t *pHooks)
{
  static bool hooked = false;
  
  if (hooked)
    return;

  hooked = true;
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
    DXGI_VIRTUAL_OVERRIDE (pHooks->ppImmediateContext, 9, "ID3D11DeviceContext::PSSetShader",
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
                                          D3D11Dev_Map_Override,
                                             D3D11_Map_Original,
                                          D3D11Dev_Map_pfn );

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
}


extern
unsigned int __stdcall HookD3D12                   (LPVOID user);

__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11Dev_CreateDeferredContext_Override (
  _In_            ID3D11Device         *This,
  _In_            UINT                  ContextFlags,
  _Out_opt_       ID3D11DeviceContext **ppDeferredContext);

_declspec (noinline)
void
STDMETHODCALLTYPE
D3D11Dev_GetImmediateContext_Override (
  _In_            ID3D11Device         *This,
  _Out_           ID3D11DeviceContext **ppImmediateContext);


DWORD
__stdcall
HookD3D11 (LPVOID user)
{
  if (! config.apis.dxgi.d3d11.hook)
    return 0;

  static volatile LONG once = FALSE;

  if (InterlockedCompareExchange (&once, TRUE, FALSE) != TRUE)
  {
    cs_shader      = new SK_Thread_HybridSpinlock (0xc00);
    cs_shader_vs   = new SK_Thread_HybridSpinlock (0x1000);
    cs_shader_ps   = new SK_Thread_HybridSpinlock (0x800);
    cs_shader_gs   = new SK_Thread_HybridSpinlock (0x400);
    cs_shader_hs   = new SK_Thread_HybridSpinlock (0x300);
    cs_shader_ds   = new SK_Thread_HybridSpinlock (0x300);
    cs_shader_cs   = new SK_Thread_HybridSpinlock (0x900);
    cs_mmio        = new SK_Thread_HybridSpinlock (0xe00);
    cs_render_view = new SK_Thread_HybridSpinlock (0xb00);
  }

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

  static volatile LONG __d3d11_hooked = FALSE;

  // This only needs to be done once
  if (! InterlockedCompareExchange (&__d3d11_hooked, TRUE, FALSE))
  {
  SK_LOG0 ( (L"  Hooking D3D11"), __SK_SUBSYSTEM__ );

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

  if (pHooks->ppDevice && pHooks->ppImmediateContext)
  {
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

#if 1
    SK_D3D11_HookDevCtx (pHooks);

    //CComQIPtr <ID3D11DeviceContext1> pDevCtx1 (*pHooks->ppImmediateContext);
    //
    //if (pDevCtx1 != nullptr)
    //{
    //  DXGI_VIRTUAL_HOOK ( &pDevCtx1,  116,
    //                        "ID3D11DeviceContext1::UpdateSubresource1",
    //                                         D3D11_UpdateSubresource1_Override,
    //                                         D3D11_UpdateSubresource1_Original,
    //                                         D3D11_UpdateSubresource1_pfn );
    //}
#endif
  }

  InterlockedIncrement (&__d3d11_hooked);
  }

  SK_Thread_SpinUntilAtomicMin (&__d3d11_hooked, 2);

//#ifdef _WIN64
//  if (config.apis.dxgi.d3d12.hook)
//    HookD3D12 (nullptr);
//#endif

  return 0;
}


bool convert_typeless = false;

auto IsWireframe = [&](sk_shader_class shader_class, uint32_t crc32c)
{
  static auto& shaders =
    SK_D3D11_Shaders;

  d3d11_shader_tracking_s* tracker   = nullptr;
  bool                     wireframe = false;

  switch (shader_class)
  {
    case sk_shader_class::Vertex:
    {
      tracker   = &shaders.vertex.tracked;
      wireframe =  shaders.vertex.wireframe.count (crc32c) != 0;
    } break;

    case sk_shader_class::Pixel:
    {
      tracker   = &shaders.pixel.tracked;
      wireframe =  shaders.pixel.wireframe.count (crc32c) != 0;
    } break;

    case sk_shader_class::Geometry:
    {
      tracker   = &shaders.geometry.tracked;
      wireframe =  shaders.geometry.wireframe.count (crc32c) != 0;
    } break;

    case sk_shader_class::Hull:
    {
      tracker   = &shaders.hull.tracked;
      wireframe =  shaders.hull.wireframe.count (crc32c) != 0;
    } break;

    case sk_shader_class::Domain:
    {
      tracker   = &shaders.domain.tracked;
      wireframe =  shaders.domain.wireframe.count (crc32c) != 0;
    } break;

    default:
    case sk_shader_class::Compute:
    {
      return false;
    } break;
  }

  if (tracker->crc32c == crc32c && tracker->wireframe)
    return true;

  return wireframe;
};

auto IsOnTop = [&](sk_shader_class shader_class, uint32_t crc32c)
{
  static auto& shaders =
    SK_D3D11_Shaders;

  d3d11_shader_tracking_s* tracker = nullptr;
  bool                     on_top  = false;

  switch (shader_class)
  {
    case sk_shader_class::Vertex:
    {
      tracker = &shaders.vertex.tracked;
      on_top  =  shaders.vertex.on_top.count (crc32c) != 0;
    } break;

    case sk_shader_class::Pixel:
    {
      tracker = &shaders.pixel.tracked;
      on_top  =  shaders.pixel.on_top.count (crc32c) != 0;
    } break;

    case sk_shader_class::Geometry:
    {
      tracker = &shaders.geometry.tracked;
      on_top  =  shaders.geometry.on_top.count (crc32c) != 0;
    } break;

    case sk_shader_class::Hull:
    {
      tracker = &shaders.hull.tracked;
      on_top  =  shaders.hull.on_top.count (crc32c) != 0;
    } break;

    case sk_shader_class::Domain:
    {
      tracker = &shaders.domain.tracked;
      on_top  =  shaders.domain.on_top.count (crc32c) != 0;
    } break;

    default:
    case sk_shader_class::Compute:
    {
      return false;
    } break;
  }

  if (tracker->crc32c == crc32c && tracker->on_top)
    return true;

  return on_top;
};

auto IsSkipped = [&](sk_shader_class shader_class, uint32_t crc32c)
{
  static auto& shaders =
    SK_D3D11_Shaders;

  d3d11_shader_tracking_s* tracker     = nullptr;
  bool                     blacklisted = false;

  switch (shader_class)
  {
    case sk_shader_class::Vertex:
    {
      tracker     = &shaders.vertex.tracked;
      blacklisted =  shaders.vertex.blacklist.count (crc32c) != 0;
    } break;

    case sk_shader_class::Pixel:
    {
      tracker     = &shaders.pixel.tracked;
      blacklisted =  shaders.pixel.blacklist.count (crc32c) != 0;
    } break;

    case sk_shader_class::Geometry:
    {
      tracker     = &shaders.geometry.tracked;
      blacklisted =  shaders.geometry.blacklist.count (crc32c) != 0;
    } break;

    case sk_shader_class::Hull:
    {
      tracker     = &shaders.hull.tracked;
      blacklisted =  shaders.hull.blacklist.count (crc32c) != 0;
    } break;

    case sk_shader_class::Domain:
    {
      tracker     = &shaders.domain.tracked;
      blacklisted =  shaders.domain.blacklist.count (crc32c) != 0;
    } break;

    case sk_shader_class::Compute:
    {
      tracker     = &shaders.compute.tracked;
      blacklisted =  shaders.compute.blacklist.count (crc32c) != 0;
    } break;

    default:
      return false;
  }

  if (tracker->crc32c == crc32c && tracker->cancel_draws)
    return true;

  return blacklisted;
};

auto IsHud = [&](sk_shader_class shader_class, uint32_t crc32c)
{
  static auto& shaders =
    SK_D3D11_Shaders;

  d3d11_shader_tracking_s* tracker = nullptr;
  bool                     hud     = false;

  switch (shader_class)
  {
    case sk_shader_class::Vertex:
    {
      tracker = &shaders.vertex.tracked;
      hud     =  shaders.vertex.hud.count (crc32c) != 0;
    } break;

    case sk_shader_class::Pixel:
    {
      tracker = &shaders.pixel.tracked;
      hud     =  shaders.pixel.hud.count (crc32c) != 0;
    } break;

    case sk_shader_class::Geometry:
    {
      tracker = &shaders.geometry.tracked;
      hud     =  shaders.geometry.hud.count (crc32c) != 0;
    } break;

    case sk_shader_class::Hull:
    {
      tracker = &shaders.hull.tracked;
      hud     =  shaders.hull.hud.count (crc32c) != 0;
    } break;

    case sk_shader_class::Domain:
    {
      tracker = &shaders.domain.tracked;
      hud     =  shaders.domain.hud.count (crc32c) != 0;
    } break;

    case sk_shader_class::Compute:
    {
      tracker = &shaders.compute.tracked;
      hud     =  shaders.compute.hud.count (crc32c) != 0;
    } break;

    default:
      return false;
  }

  return hud;
};

struct shader_disasm_s {
  std::string       header = "";
  std::string       code   = "";
  std::string       footer = "";

  struct constant_buffer
  {
    std::string      name  = "";
    UINT             Slot  =  0;

    struct variable
    {
      std::string                name               =  "";
      D3D11_SHADER_VARIABLE_DESC var_desc           = { };
      D3D11_SHADER_TYPE_DESC     type_desc          = { };
      float                      default_value [16] = { };
    };

    typedef std::pair <std::vector <variable>, std::string> struct_ent;

    std::vector <struct_ent> structs;

    size_t           size  = 0UL;
  };

  std::vector <constant_buffer> cbuffers;
};

using ID3DXBuffer  = interface ID3DXBuffer;
using LPD3DXBUFFER = interface ID3DXBuffer*;

// {8BA5FB08-5195-40e2-AC58-0D989C3A0102}
DEFINE_GUID(IID_ID3DXBuffer, 
0x8ba5fb08, 0x5195, 0x40e2, 0xac, 0x58, 0xd, 0x98, 0x9c, 0x3a, 0x1, 0x2);

#undef INTERFACE
#define INTERFACE ID3DXBuffer

DECLARE_INTERFACE_(ID3DXBuffer, IUnknown)
{
    // IUnknown
    STDMETHOD  (        QueryInterface)   (THIS_ REFIID iid, LPVOID *ppv) PURE;
    STDMETHOD_ (ULONG,  AddRef)           (THIS) PURE;
    STDMETHOD_ (ULONG,  Release)          (THIS) PURE;

    // ID3DXBuffer
    STDMETHOD_ (LPVOID, GetBufferPointer) (THIS) PURE;
    STDMETHOD_ (DWORD,  GetBufferSize)    (THIS) PURE;
};

#include <imgui/imgui.h>
#include <imgui/backends/imgui_d3d11.h>

std::unordered_map <uint32_t, shader_disasm_s> vs_disassembly;
std::unordered_map <uint32_t, shader_disasm_s> ps_disassembly;
std::unordered_map <uint32_t, shader_disasm_s> gs_disassembly;
std::unordered_map <uint32_t, shader_disasm_s> hs_disassembly;
std::unordered_map <uint32_t, shader_disasm_s> ds_disassembly;
std::unordered_map <uint32_t, shader_disasm_s> cs_disassembly;

uint32_t change_sel_vs = 0x00;
uint32_t change_sel_ps = 0x00;
uint32_t change_sel_gs = 0x00;
uint32_t change_sel_hs = 0x00;
uint32_t change_sel_ds = 0x00;
uint32_t change_sel_cs = 0x00;


auto ShaderMenu =
[&] ( std::unordered_set <uint32_t>&                          blacklist,
      SK_D3D11_KnownShaders::conditional_blacklist_t&    cond_blacklist,
      std::vector       <ID3D11ShaderResourceView *>&   used_resources,
const std::set          <ID3D11ShaderResourceView *>& set_of_resources,
      uint32_t                                                   shader )
{
  static SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  if (blacklist.count (shader))
  {
    if (ImGui::MenuItem ("Enable Shader"))
    {
      blacklist.erase (shader);
    }
  }

  else
  {
    if (ImGui::MenuItem ("Disable Shader"))
    {
      blacklist.emplace (shader);
    }
  }

  auto DrawSRV = [&](ID3D11ShaderResourceView* pSRV)
  {
    DXGI_FORMAT                     fmt = DXGI_FORMAT_UNKNOWN;
    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;

    pSRV->GetDesc (&srv_desc);

    if (srv_desc.ViewDimension == D3D_SRV_DIMENSION_TEXTURE2D)
    {
      CComPtr <ID3D11Resource>  pRes = nullptr;
      CComPtr <ID3D11Texture2D> pTex = nullptr;

      pSRV->GetResource (&pRes);

      if (pRes && SUCCEEDED (pRes->QueryInterface <ID3D11Texture2D> (&pTex)) && pTex)
      {
        D3D11_TEXTURE2D_DESC desc = { };
        pTex->GetDesc      (&desc);
                       fmt = desc.Format;

        if (desc.Height > 0 && desc.Width > 0)
        {
          ImGui::TreePush ("");

          std::lock_guard <SK_Thread_CriticalSection> auto_lock (*cs_render_view);

          ID3D11ShaderResourceView* pSRV2 = nullptr;

          DXGI_FORMAT orig_fmt =
            srv_desc.Format;

          srv_desc.Format =
            DirectX::MakeTypelessFLOAT (orig_fmt);
          
          if (srv_desc.Format == orig_fmt)
          {
            srv_desc.Format =
              DirectX::MakeTypelessUNORM (orig_fmt);
          }
          srv_desc.Texture2D.MipLevels       = (UINT)-1;
          srv_desc.Texture2D.MostDetailedMip =        desc.MipLevels;

          if (convert_typeless && SUCCEEDED (((ID3D11Device *)(rb.device.p))->CreateShaderResourceView (pRes, &srv_desc, &pSRV2)))
          {
            temp_resources.emplace_back (pSRV2);

            ImGui::Image ( pSRV2,      ImVec2  ( std::max (64.0f, (float)desc.Width / 16.0f),
      ((float)desc.Height / (float)desc.Width) * std::max (64.0f, (float)desc.Width / 16.0f) ),
                                       ImVec2  (0,0),             ImVec2  (1,1),
                                       ImColor (255,255,255,255), ImColor (242,242,13,255) );
          }

          else
          {
            temp_resources.emplace_back (pSRV);
                                         pSRV->AddRef ();

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

  static auto& textures =
    SK_D3D11_Textures;

  for (auto it : used_resources)
  {
    bool selected = false;

    uint32_t crc32c = 0x0;

    ID3D11Resource*   pRes = nullptr;
    it->GetResource (&pRes);

    D3D11_RESOURCE_DIMENSION rdim = D3D11_RESOURCE_DIMENSION_UNKNOWN;
             pRes->GetType (&rdim);

    if (rdim == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
    {
      CComQIPtr <ID3D11Texture2D> pTex2D (pRes);

      if (pTex2D != nullptr)
      {
        if (textures.Textures_2D.count  (pTex2D) &&  textures.Textures_2D [pTex2D].crc32c != 0x0)
        {
          crc32c = textures.Textures_2D [pTex2D].crc32c;
        }
      }
    }

    if (listed.count (crc32c))
      continue;

    if (cond_blacklist [shader].count (crc32c))
    {
      ImGui::BeginGroup ();

      if (crc32c != 0x00)
        ImGui::MenuItem ( SK_FormatString ("Enable Shader for Texture %x", crc32c).c_str (), nullptr, &selected);

      DrawSRV (it);

      ImGui::EndGroup ();
    }

    if (crc32c != 0x0)
      listed.emplace (crc32c);

    if (selected)
    {
      cond_blacklist [shader].erase (crc32c);
    }
  }

  listed.clear ();

  for (auto it : used_resources )
  {
    if (it == nullptr)
      continue;

    bool selected = false;

    uint32_t crc32c = 0x0;

    ID3D11Resource*   pRes = nullptr;
    it->GetResource (&pRes);

    D3D11_RESOURCE_DIMENSION rdim = D3D11_RESOURCE_DIMENSION_UNKNOWN;
    pRes->GetType          (&rdim);

    if (rdim == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
    {
      CComQIPtr <ID3D11Texture2D> pTex2D (pRes);

      if (pTex2D != nullptr)
      {
        if (textures.Textures_2D.count  (pTex2D) && textures.Textures_2D [pTex2D].crc32c != 0x0)
        {
          crc32c = textures.Textures_2D [pTex2D].crc32c;
        }
      }
    }

    if (listed.count (crc32c))
      continue;

    if (! cond_blacklist [shader].count (crc32c))
    {
      ImGui::BeginGroup ();

      if (crc32c != 0x00)
      {
        ImGui::MenuItem ( SK_FormatString ("Disable Shader for Texture %x", crc32c).c_str (), nullptr, &selected);

        if (set_of_resources.count (it))
          DrawSRV (it);
      }

      ImGui::EndGroup ();

      if (selected)
      {
        cond_blacklist [shader].emplace (crc32c);
      }
    }

    if (crc32c != 0x0)
      listed.emplace (crc32c);
  }
};

static size_t debug_tex_id = 0x0;
static size_t tex_dbg_idx  = 0;

#include <SpecialK/steam_api.h>
#include <unordered_map>

extern std::unordered_map <BYTE, std::wstring> virtKeyCodeToHumanKeyName;


void
SK_LiveTextureView (bool& can_scroll, SK_TLS* pTLS = SK_TLS_Bottom ())
{
  ImGuiIO& io (ImGui::GetIO ());

  static auto& rb =
    SK_GetCurrentRenderBackend ();

  std::lock_guard <SK_Thread_CriticalSection> auto_lock (*cs_render_view);

  ImGui::PushID ("Texture2D_D3D11");

  const float font_size           = ImGui::GetFont ()->FontSize * io.FontGlobalScale;
  const float font_size_multiline = font_size + ImGui::GetStyle ().ItemSpacing.y + ImGui::GetStyle ().ItemInnerSpacing.y;

  static float last_ht    = 256.0f;
  static float last_width = 256.0f;

  struct list_entry_s {
    std::string          name      = "I need an adult!";
    uint32_t             tag       = 0UL;
    uint32_t             crc32c    = 0UL;
    bool                 injected  = false;
    D3D11_TEXTURE2D_DESC desc      = { };
    D3D11_TEXTURE2D_DESC orig_desc = { };
    BOOL                 mipmapped =  -1; // We must calculate this
    ID3D11Texture2D*     pTex      = nullptr;
    size_t               size      = 0;
  };                              

  static std::vector <list_entry_s> list_contents;
  static std::map
           <uint32_t, list_entry_s> texture_map;
  static              bool          list_dirty      = true;
  static              bool          lod_list_dirty  = true;
  static              size_t        sel             =    0;
  static              int           tex_set         =    1;
  static              int           lod             =    0;
  static              char          lod_list [1024]   {  };

  extern              size_t        tex_dbg_idx;
  extern              size_t        debug_tex_id;

  static              int           refresh_interval     = 0UL; // > 0 = Periodic Refresh
  static              int           last_frame           = 0UL;
  static              size_t        total_texture_memory = 0ULL;

  
  ImGui::Text      ("Current list represents %5.2f MiB of texture memory", (double)total_texture_memory / (double)(1024 * 1024));
  ImGui::SameLine  ( );

  ImGui::PushItemWidth (ImGui::GetContentRegionAvailWidth () * 0.33f);
  ImGui::SliderInt     ("Frames Between Texture Refreshes", &refresh_interval, 0, 120);
  ImGui::PopItemWidth  ();

  ImGui::BeginChild ( ImGui::GetID ("ToolHeadings"), ImVec2 (font_size * 66.0f, font_size * 2.5f), false,
                        ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_NavFlattened );

  if (InterlockedCompareExchange (&live_textures_dirty, FALSE, TRUE))
  {
    texture_map.clear   ();
    list_contents.clear ();

    last_ht             =  0;
    last_width          =  0;
    lod                 =  0;

    list_dirty          = true;
  }

  if (ImGui::Button ("  Refresh Textures  "))
  {
    list_dirty = true;
  }

  if (ImGui::IsItemHovered ())
  {
    if (tex_set == 1) ImGui::SetTooltip ("Refresh the list using textures drawn during the last frame.");
    else              ImGui::SetTooltip ("Refresh the list using ALL cached textures.");
  }

  ImGui::SameLine      ();
  ImGui::PushItemWidth (font_size * strlen ("Used Textures   ") / 2);

  ImGui::Combo ("###TexturesD3D11_TextureSet", &tex_set, "All Textures\0Used Textures\0\0", 2);

  ImGui::PopItemWidth ();
  ImGui::SameLine     ();

  if (ImGui::Button (" Clear Debug "))
  {
    sel                 = std::numeric_limits <size_t>::max ();
    debug_tex_id        =  0;
    list_contents.clear ();
    last_ht             =  0;
    last_width          =  0;
    lod                 =  0;
    tracked_texture     =  nullptr;
  }

  if (ImGui::IsItemHovered ()) ImGui::SetTooltip ("Exits texture debug mode.");

  ImGui::SameLine ();

  if (ImGui::Button ("  Reload All Injected Textures  "))
  {
    SK_D3D11_ReloadAllTextures ();
  }

  ImGui::SameLine ();
  ImGui::Checkbox ("Highlight Selected Texture in Game##D3D11_HighlightSelectedTexture", &config.textures.d3d11.highlight_debug);
  ImGui::SameLine ();

  static bool hide_inactive = true;
  
  ImGui::Checkbox  ("Hide Inactive Textures##D3D11_HideInactiveTextures",                 &hide_inactive);
  ImGui::Separator ();
  ImGui::EndChild  ();


  for (auto& it_ctx : SK_D3D11_PerCtxResources )
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

    for ( auto& it_res : it_ctx.used_textures )
    {
      used_textures.emplace (it_res);
    }

    it_ctx.used_textures.clear ();

    InterlockedExchange (&it_ctx.writing_, 0);
  }


  if (refresh_interval > 0 || list_dirty)
  {
    if ((LONG)SK_GetFramesDrawn () > last_frame + refresh_interval || list_dirty)
    {
      list_dirty           = true;
      last_frame           = SK_GetFramesDrawn ();
      total_texture_memory = 0ULL;
    }
  }

  static auto& textures =
    SK_D3D11_Textures;

  if (list_dirty)
  {
    list_contents.clear ();

    if (debug_tex_id == 0)
      last_ht = 0;

    {
      // Relatively immutable textures
      for (auto& it : textures.HashMap_2D)
      {
        SK_AutoCriticalSection critical1 (&it.mutex);

        for (auto& it2 : it.entries)
        {
          const auto& tex_ref =
            textures.TexRefs_2D.find (it2.second);

          if ( tex_ref != textures.TexRefs_2D.cend () )
          {
            list_entry_s entry = { };

            entry.crc32c = 0;
            entry.tag    = it2.first;
            entry.name   = "DontCare";
            entry.pTex   = it2.second;

            if (SK_D3D11_TextureIsCached (it2.second))
            {
              const SK_D3D11_TexMgr::tex2D_descriptor_s& desc =
                textures.Textures_2D [it2.second];

              entry.desc      = desc.desc;
              entry.orig_desc = desc.orig_desc;
              entry.size      = desc.mem_size;
              entry.crc32c    = desc.crc32c;
              entry.injected  = desc.injected;
            }

            else
              continue;

            if (entry.size > 0 && entry.crc32c != 0x00)
              texture_map [entry.crc32c] = entry;
          }
        }
      }

      // Self-sorted list, yay :)
      for (auto& it : texture_map)
      {
        const bool active =
          used_textures.count (it.second.pTex) != 0;

        if (active || tex_set == 0)
        {
          char szDesc [48] = { };

          _itoa (it.second.crc32c, szDesc, 16);

          list_entry_s entry = { };

          entry.crc32c    = it.second.crc32c;
          entry.tag       = it.second.tag;
          entry.desc      = it.second.desc;
          entry.orig_desc = it.second.orig_desc;
          entry.name      = szDesc;
          entry.pTex      = it.second.pTex;
          entry.size      = it.second.size;

          list_contents.emplace_back (entry);
          total_texture_memory += entry.size;
        }
      }
    }

    list_dirty = false;

    if ((! textures.TexRefs_2D.count (tracked_texture)) ||
           textures.Textures_2D      [tracked_texture].crc32c == 0x0 )
      tracked_texture = nullptr;
  }

  ImGui::BeginGroup ();

  ImGui::PushStyleVar   (ImGuiStyleVar_ChildWindowRounding, 0.0f);
  ImGui::PushStyleColor (ImGuiCol_Border, ImVec4 (0.9f, 0.7f, 0.5f, 1.0f));

  ImGui::BeginChild ( ImGui::GetID ("D3D11_TexHashes_CRC32C"),
                      ImVec2 ( font_size * 6.0f, std::max (font_size * 15.0f, last_ht)),
                        true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NavFlattened );

  if (ImGui::IsWindowHovered ())
    can_scroll = false;

  static int draws = 0;

  if (! list_contents.empty ())
  {
    static size_t last_sel     = std::numeric_limits <size_t>::max ();
    static bool   sel_changed  = false;

    // Don't select a texture immediately
    if (sel != last_sel && draws++ != 0)
      sel_changed = true;
    
    last_sel = sel;
    
    for ( size_t line = 0; line < list_contents.size (); line++)
    {
      const bool active =
        used_textures.count (list_contents [line].pTex) != 0;

      if (active)
        ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.95f, 0.95f, 0.95f, 1.0f));
      else
        ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.425f, 0.425f, 0.425f, 0.9f));

      if ((! hide_inactive) || active)
      {
        if (line == sel)
        {
          bool selected = true;
          ImGui::Selectable (list_contents [line].name.c_str (), &selected);
    
          if (sel_changed)
          {
            ImGui::SetScrollHere        (0.5f);
            ImGui::SetKeyboardFocusHere (    );

            sel_changed     = false;
            tex_dbg_idx     = line;
            sel             = line;
            debug_tex_id    = list_contents [line].crc32c;
            tracked_texture = list_contents [line].pTex;
            lod             = 0;
            lod_list_dirty  = true;
            *lod_list       = '\0';
          }
        }
    
        else
        {
          bool selected = false;
    
          if (ImGui::Selectable (list_contents [line].name.c_str (), &selected))
          {
            sel_changed     = true;
            tex_dbg_idx     = line;
            sel             = line;
            debug_tex_id    = list_contents [line].crc32c;
            tracked_texture = list_contents [line].pTex;
            lod             = 0;
            lod_list_dirty  = true;
            *lod_list       = '\0';
          }
        }
      }

      ImGui::PopStyleColor ();
    }
  }

  ImGui::EndChild ();

  if (ImGui::IsItemHovered () || ImGui::IsItemFocused ())
  {
    int dir = 0;

    if (ImGui::IsItemFocused ())
    {
      ImGui::BeginTooltip ();
      ImGui::TextColored  (ImVec4 (0.9f, 0.6f, 0.2f, 1.0f), "");
      ImGui::Separator    ();
      ImGui::BulletText   ("Press LB to select the previous texture from this list");
      ImGui::BulletText   ("Press RB to select the next texture from this list");
      ImGui::EndTooltip   ();
    }

    else
    {
      ImGui::BeginTooltip ();
      ImGui::TextColored  (ImVec4 (0.9f, 0.6f, 0.2f, 1.0f), "");
      ImGui::Separator    ();
      ImGui::BulletText   ("Press %ws to select the previous texture from this list", virtKeyCodeToHumanKeyName [VK_OEM_4].c_str ());
      ImGui::BulletText   ("Press %ws to select the next texture from this list",     virtKeyCodeToHumanKeyName [VK_OEM_6].c_str ());
      ImGui::EndTooltip   ();
    }

         if (io.NavInputs [ImGuiNavInput_PadFocusPrev] && io.NavInputsDownDuration [ImGuiNavInput_PadFocusPrev] == 0.0f) { dir = -1; }
    else if (io.NavInputs [ImGuiNavInput_PadFocusNext] && io.NavInputsDownDuration [ImGuiNavInput_PadFocusNext] == 0.0f) { dir =  1; }

    else
    {
           if (io.KeysDown [VK_OEM_4] && io.KeysDownDuration [VK_OEM_4] == 0.0f) { dir = -1;  io.WantCaptureKeyboard = true; }
      else if (io.KeysDown [VK_OEM_6] && io.KeysDownDuration [VK_OEM_6] == 0.0f) { dir =  1;  io.WantCaptureKeyboard = true; }
    }

    if (dir != 0)
    {
      if ((SSIZE_T)sel <  0)                     sel = 0;
      if (         sel >= list_contents.size ()) sel = list_contents.size () - 1;
      if ((SSIZE_T)sel <  0)                     sel = 0;

      while ((SSIZE_T)sel >= 0 && sel < list_contents.size ())
      {
        if ( (dir < 0 && sel == 0                        ) ||
             (dir > 0 && sel == list_contents.size () - 1)    )
        {
          break;
        }

        sel += dir;

        if (hide_inactive)
        {
          const bool active =
            used_textures.count (list_contents [sel].pTex) != 0;

          if (active)
            break;
        }

        else
          break;
      }

      if ((SSIZE_T)sel <  0)                     sel = 0;
      if (         sel >= list_contents.size ()) sel = list_contents.size () - 1;
      if ((SSIZE_T)sel <  0)                     sel = 0;
    }
  }

  ImGui::SameLine     ();
  ImGui::PushStyleVar (ImGuiStyleVar_ChildWindowRounding, 20.0f);

  last_ht    = std::max (last_ht,    16.0f);
  last_width = std::max (last_width, 16.0f);

  if (debug_tex_id != 0x00 && texture_map.count ((uint32_t)debug_tex_id))
  {
    list_entry_s& entry =
      texture_map [(uint32_t)debug_tex_id];

    D3D11_TEXTURE2D_DESC tex_desc  = entry.orig_desc;
    size_t               tex_size  = 0UL;
    float                load_time = 0.0f;

    CComPtr <ID3D11Texture2D> pTex =
      textures.getTexture2D ( (uint32_t)entry.tag,
                                              &tex_desc,
                                              &tex_size,
                                              &load_time, pTLS );

    const bool staged = false;

    if (pTex != nullptr)
    {
      // Get the REAL format, not the one the engine knows about through texture cache
      pTex->GetDesc (&tex_desc);

      if (lod_list_dirty)
      {
        const UINT w = tex_desc.Width;
        const UINT h = tex_desc.Height;

        char* pszLODList = lod_list;

        for ( UINT i = 0 ; i < tex_desc.MipLevels ; i++ )
        {
          int len =
            sprintf (pszLODList, "LOD%u: (%ux%u)", i, std::max (1U, w >> i), std::max (1U, h >> i));

          pszLODList += (len + 1);
        }

        *pszLODList = '\0';

        lod_list_dirty = false;
      }


      ID3D11ShaderResourceView*       pSRV     = nullptr;
      D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {     };

      srv_desc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
      srv_desc.Format                    = tex_desc.Format;

      // Typeless compressed types need to assume a type, or they won't render :P
      switch (srv_desc.Format)
      {
        case DXGI_FORMAT_BC1_TYPELESS:
          srv_desc.Format = DXGI_FORMAT_BC1_UNORM;
          break;
        case DXGI_FORMAT_BC2_TYPELESS:
          srv_desc.Format = DXGI_FORMAT_BC2_UNORM;
          break;
        case DXGI_FORMAT_BC3_TYPELESS:
          srv_desc.Format = DXGI_FORMAT_BC3_UNORM;
          break;
        case DXGI_FORMAT_BC4_TYPELESS:
          srv_desc.Format = DXGI_FORMAT_BC4_UNORM;
          break;
        case DXGI_FORMAT_BC5_TYPELESS:
          srv_desc.Format = DXGI_FORMAT_BC5_UNORM;
          break;
        case DXGI_FORMAT_BC6H_TYPELESS:
          srv_desc.Format = DXGI_FORMAT_BC6H_SF16;
          break;
        case DXGI_FORMAT_BC7_TYPELESS:
          srv_desc.Format = DXGI_FORMAT_BC7_UNORM;
          break;

        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
          srv_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
          break;
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
          srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
          break;

        case DXGI_FORMAT_R8_TYPELESS:
          srv_desc.Format = DXGI_FORMAT_R8_UNORM;
          break;
        case DXGI_FORMAT_R8G8_TYPELESS:
          srv_desc.Format = DXGI_FORMAT_R8G8_UNORM;
          break;

        case DXGI_FORMAT_R16_TYPELESS:
          srv_desc.Format = DXGI_FORMAT_R16_UNORM;
          break;
        case DXGI_FORMAT_R16G16_TYPELESS:
          srv_desc.Format = DXGI_FORMAT_R16G16_UNORM;
          break;
        case DXGI_FORMAT_R16G16B16A16_TYPELESS:
          srv_desc.Format = DXGI_FORMAT_R16G16B16A16_UNORM;
          break;

        case DXGI_FORMAT_R32_TYPELESS:
          srv_desc.Format = DXGI_FORMAT_R32_FLOAT;
          break;
        case DXGI_FORMAT_R32G32_TYPELESS:
          srv_desc.Format = DXGI_FORMAT_R32G32_FLOAT;
          break;
        case DXGI_FORMAT_R32G32B32_TYPELESS:
          srv_desc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
          break;
        case DXGI_FORMAT_R32G32B32A32_TYPELESS:
          srv_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
          break;
      };

      srv_desc.Texture2D.MipLevels       = (UINT)-1;
      srv_desc.Texture2D.MostDetailedMip =        tex_desc.MipLevels;

      CComQIPtr <ID3D11Device> pDev (rb.device);

      if (pDev != nullptr)
      {
#if 0
        ImVec4 border_color = config.textures.highlight_debug_tex ? 
                                ImVec4 (0.3f, 0.3f, 0.3f, 1.0f) :
                                  (__remap_textures && has_alternate) ? ImVec4 (0.5f,  0.5f,  0.5f, 1.0f) :
                                                                        ImVec4 (0.3f,  1.0f,  0.3f, 1.0f);
#else
        const ImVec4 border_color = entry.injected ? ImVec4 (0.3f,  0.3f,  1.0f, 1.0f) :
                                                     ImVec4 (0.3f,  1.0f,  0.3f, 1.0f);
#endif

        ImGui::PushStyleColor (ImGuiCol_Border, border_color);

        const float scale_factor = 1.0f;

        const float content_avail_y = (ImGui::GetWindowContentRegionMax ().y - ImGui::GetWindowContentRegionMin ().y) / scale_factor;
        const float content_avail_x = (ImGui::GetWindowContentRegionMax ().x - ImGui::GetWindowContentRegionMin ().x) / scale_factor;
              float effective_width, effective_height;

        effective_height = std::max (std::min ((float)(tex_desc.Height >> lod), 256.0f), std::min ((float)(tex_desc.Height >> lod), (content_avail_y - font_size_multiline * 11.0f - 24.0f)));
        effective_width  = std::min ((float)(tex_desc.Width  >> lod), (effective_height * (std::max (1.0f, (float)(tex_desc.Width >> lod)) / std::max (1.0f, (float)(tex_desc.Height >> lod)))));

        if (effective_width > (content_avail_x - font_size * 28.0f))
        {
          effective_width   = std::max (std::min ((float)(tex_desc.Width >> lod), 256.0f), (content_avail_x - font_size * 28.0f));
          effective_height  =  effective_width * (std::max (1.0f, (float)(tex_desc.Height >> lod)) / std::max (1.0f, (float)(tex_desc.Width >> lod)) );
        }

        ImGui::BeginGroup     ();
        ImGui::BeginChild     ( ImGui::GetID ("Texture_Select_D3D11"),
                                ImVec2 ( -1.0f, -1.0f ),
                                  true,
                                    ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar |
                                    ImGuiWindowFlags_NavFlattened );

        //if ((! config.textures.highlight_debug_tex) && has_alternate)
        //{
        //  if (ImGui::IsItemHovered ())
        //    ImGui::SetTooltip ("Click me to make this texture the visible version.");
        //  
        //  // Allow the user to toggle texture override by clicking the frame
        //  if (ImGui::IsItemClicked ())
        //    __remap_textures = false;
        //}

        last_width  = effective_width  + font_size * 28.0f;

        ImGui::BeginGroup      (                  );
        ImGui::TextUnformatted ( "Dimensions:   " );
        ImGui::EndGroup        (                  );
        ImGui::SameLine        (                  );
        ImGui::BeginGroup      (                  );
        ImGui::PushItemWidth   (                -1);
        ImGui::Combo           ("###Texture_LOD_D3D11", &lod, lod_list, tex_desc.MipLevels);
        ImGui::PopItemWidth    (                  );
        ImGui::EndGroup        (                  );

        ImGui::BeginGroup      (                  );
        ImGui::TextUnformatted ( "Format:       " );
        ImGui::TextUnformatted ( "Data Size:    " );
        ImGui::TextUnformatted ( "Load Time:    " );
        ImGui::TextUnformatted ( "Cache Hits:   " );
        ImGui::EndGroup        (                  );
        ImGui::SameLine        (                  );
        ImGui::BeginGroup      (                  );
        ImGui::Text            ( "%ws",
                                   SK_DXGI_FormatToStr (tex_desc.Format).c_str () );
        ImGui::Text            ( "%.3f MiB",
                                   tex_size / (1024.0f * 1024.0f) );
        ImGui::Text            ( "%.3f ms",
                                   load_time );
        ImGui::Text            ( "%li",
                                   ReadAcquire (&textures.Textures_2D [pTex].hits) );
        ImGui::EndGroup        (                  );
        ImGui::Separator       (                  );

        static bool flip_vertical   = false;
        static bool flip_horizontal = false;

        ImGui::Checkbox ("Flip Vertically##D3D11_FlipVertical",     &flip_vertical);   ImGui::SameLine ();
        ImGui::Checkbox ("Flip Horizontally##D3D11_FlipHorizontal", &flip_horizontal);

        if (! entry.injected)
        {
          if (! SK_D3D11_IsDumped (entry.crc32c, 0x0))
          {
            if ( ImGui::Button ("  Dump Texture to Disk  ###DumpTexture") )
            {
              SK_ScopedBool auto_bool (&pTLS->texture_management.injection_thread);
                                        pTLS->texture_management.injection_thread = true;

              SK_D3D11_DumpTexture2D (
                pTex, entry.crc32c
              );
            }
          }

          else
          {
            if ( ImGui::Button ("  Delete Dumped Texture from Disk  ###DumpTexture") )
            {
              SK_D3D11_DeleteDumpedTexture (entry.crc32c);
            }
          }

          if (entry.mipmapped == -1)
          {   entry.mipmapped  = ( CalcMipmapLODs ( entry.desc.Width,
                                                    entry.desc.Height ) == entry.desc.MipLevels )
                               ? TRUE : FALSE;
          }

          if (entry.mipmapped == FALSE)
          {
            ImGui::SameLine ();

            if (ImGui::Button ("  Generate Mipmaps  ###GenerateMipmaps"))
            {
              SK_ScopedBool auto_bool (&pTLS->texture_management.injection_thread);
                                        pTLS->texture_management.injection_thread = true;

              HRESULT
              __stdcall
              SK_D3D11_MipmapCacheTexture2D ( _In_ ID3D11Texture2D* pTex, uint32_t crc32c, SK_TLS* pTLS );


              if (SUCCEEDED (SK_D3D11_MipmapCacheTexture2D (pTex, entry.crc32c, pTLS)))
              {
                entry.mipmapped = TRUE;
              }
            }
          }
        }

        if (staged)
        {
          ImGui::SameLine ();
          ImGui::TextColored (ImColor::HSV (0.25f, 1.0f, 1.0f), "Staged textures cannot be re-injected yet.");
        }

        if (entry.injected)
        {
          if ( ImGui::Button ("  Reload Texture  ###ReloadTexture") )
          {
            SK_D3D11_ReloadTexture (pTex);
          }

          ImGui::SameLine    ();
          ImGui::TextColored (ImVec4 (0.05f, 0.95f, 0.95f, 1.0f), "This texture has been injected over the original.");
        }

        if ( effective_height != (float)(tex_desc.Height >> lod) ||
             effective_width  != (float)(tex_desc.Width  >> lod) )
        {
          if (! entry.injected)
            ImGui::SameLine ();

          ImGui::TextColored (ImColor::HSV (0.5f, 1.0f, 1.0f), "Texture was rescaled to fit.");
        }

        if (! entry.injected)
          ImGui::PushStyleColor  (ImGuiCol_Border, ImVec4 (0.95f, 0.95f, 0.05f, 1.0f));
        else
           ImGui::PushStyleColor (ImGuiCol_Border, ImVec4 (0.05f, 0.95f, 0.95f, 1.0f));

        srv_desc.Texture2D.MipLevels       = 1;
        srv_desc.Texture2D.MostDetailedMip = lod;

        if (SUCCEEDED (pDev->CreateShaderResourceView (pTex, &srv_desc, &pSRV)))
        {
          const ImVec2 uv0 (flip_horizontal ? 1.0f : 0.0f, flip_vertical ? 1.0f : 0.0f);
          const ImVec2 uv1 (flip_horizontal ? 0.0f : 1.0f, flip_vertical ? 0.0f : 1.0f);

          ImGui::BeginChildFrame (ImGui::GetID ("TextureView_Frame"), ImVec2 (effective_width + 8.0f, effective_height + 8.0f),
                                  ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_ShowBorders |
                                  ImGuiWindowFlags_NoInputs         | ImGuiWindowFlags_NoScrollbar |
                                  ImGuiWindowFlags_NoNavInputs      | ImGuiWindowFlags_NoNavFocus );

          temp_resources.push_back (pSRV);
          ImGui::Image            ( pSRV,
                                     ImVec2 (effective_width, effective_height),
                                       uv0,                       uv1,
                                       ImColor (255,255,255,255), ImColor (255,255,255,128)
                               );
          ImGui::EndChildFrame ();

          static DWORD dwLastUnhovered = 0;

          if (ImGui::IsItemHovered ())
          {
            if (SK::ControlPanel::current_time - dwLastUnhovered > 666UL)
            {
              ImGui::BeginTooltip    ();
              ImGui::BeginGroup      ();
              ImGui::PushStyleColor  (ImGuiCol_Text, ImColor (0.785f, 0.785f, 0.785f));
              ImGui::TextUnformatted ("Usage:");
              ImGui::TextUnformatted ("Bind Flags:");
              if (tex_desc.MiscFlags != 0)
              ImGui::TextUnformatted ("Misc Flags:");
              ImGui::PopStyleColor   ();
              ImGui::EndGroup        ();
              ImGui::SameLine        ();
              ImGui::BeginGroup      ();
              ImGui::PushStyleColor  (ImGuiCol_Text, ImColor (1.0f, 1.0f, 1.0f));
              ImGui::Text            ("%ws", SK_D3D11_DescribeUsage     (
                                               tex_desc.Usage )             );
              ImGui::Text            ("%ws", SK_D3D11_DescribeBindFlags (
                              (D3D11_BIND_FLAG)tex_desc.BindFlags).c_str () );
              if (tex_desc.MiscFlags != 0)
              {
                ImGui::Text          ("%ws", SK_D3D11_DescribeMiscFlags (
                     (D3D11_RESOURCE_MISC_FLAG)tex_desc.MiscFlags).c_str () );
              }
              ImGui::PopStyleColor   ();
              ImGui::EndGroup        ();
              ImGui::EndTooltip      ();
            }
          }

          else
            dwLastUnhovered = SK::ControlPanel::current_time;
        }
        ImGui::PopStyleColor   ();
        ImGui::EndChild        ();
        ImGui::EndGroup        ();
        last_ht =
        ImGui::GetItemRectSize ().y;
        ImGui::PopStyleColor   ();
      }
    }

#if 0
    if (has_alternate)
    {
      ImGui::SameLine ();

      D3DSURFACE_DESC desc;

      if (SUCCEEDED (pTex->d3d9_tex->pTexOverride->GetLevelDesc (0, &desc)))
      {
        ImVec4 border_color = config.textures.highlight_debug_tex ? 
                                ImVec4 (0.3f, 0.3f, 0.3f, 1.0f) :
                                  (__remap_textures) ? ImVec4 (0.3f,  1.0f,  0.3f, 1.0f) :
                                                       ImVec4 (0.5f,  0.5f,  0.5f, 1.0f);

        ImGui::PushStyleColor  (ImGuiCol_Border, border_color);

        ImGui::BeginGroup ();
        ImGui::BeginChild ( "Item Selection2",
                            ImVec2 ( std::max (font_size * 19.0f, (float)desc.Width  + 24.0f),
                                                                  (float)desc.Height + font_size * 10.0f),
                              true,
                                ImGuiWindowFlags_AlwaysAutoResize );

        //if (! config.textures.highlight_debug_tex)
        //{
        //  if (ImGui::IsItemHovered ())
        //    ImGui::SetTooltip ("Click me to make this texture the visible version.");
        //
        //  // Allow the user to toggle texture override by clicking the frame
        //  if (ImGui::IsItemClicked ())
        //    __remap_textures = true;
        //}


        last_width  = std::max (last_width, (float)desc.Width);
        last_ht     = std::max (last_ht,    (float)desc.Height + font_size * 10.0f);


        extern std::wstring
        SK_D3D9_FormatToStr (D3DFORMAT Format, bool include_ordinal = true);


        SK_D3D11_IsInjectable ()
        bool injected  =
          (TBF_GetInjectableTexture (debug_tex_id) != nullptr),
             reloading = false;

        int num_lods = pTex->d3d9_tex->pTexOverride->GetLevelCount ();

        ImGui::Text ( "Dimensions:   %lux%lu  (%lu %s)",
                        desc.Width, desc.Height,
                           num_lods, num_lods > 1 ? "LODs" : "LOD" );
        ImGui::Text ( "Format:       %ws",
                        SK_D3D9_FormatToStr (desc.Format).c_str () );
        ImGui::Text ( "Data Size:    %.2f MiB",
                        (double)pTex->d3d9_tex->override_size / (1024.0f * 1024.0f) );
        ImGui::TextColored (ImVec4 (1.0f, 1.0f, 1.0f, 1.0f), injected ? "Injected Texture" : "Resampled Texture" );

        ImGui::Separator     ();


        if (injected)
        {
          if ( ImGui::Button ("  Reload This Texture  ") && tbf::RenderFix::tex_mgr.reloadTexture (debug_tex_id) )
          {
            reloading    = true;

            tbf::RenderFix::tex_mgr.updateOSD ();
          }
        }

        else {
          ImGui::Button ("  Resample This Texture  "); // NO-OP, but preserves alignment :P
        }

        if (! reloading)
        {
          ImGui::PushStyleColor  (ImGuiCol_Border, ImVec4 (0.95f, 0.95f, 0.05f, 1.0f));
          ImGui::BeginChildFrame (0, ImVec2 ((float)desc.Width + 8, (float)desc.Height + 8), ImGuiWindowFlags_ShowBorders);
          ImGui::Image           ( pTex->d3d9_tex->pTexOverride,
                                     ImVec2 ((float)desc.Width, (float)desc.Height),
                                       ImVec2  (0,0),             ImVec2  (1,1),
                                       ImColor (255,255,255,255), ImColor (255,255,255,128)
                                 );
          ImGui::EndChildFrame   ();
          ImGui::PopStyleColor   (1);
        }

        ImGui::EndChild        ();
        ImGui::EndGroup        ();
        ImGui::PopStyleColor   (1);
      }
    }
#endif
  }
  ImGui::EndGroup      ( );
  ImGui::PopStyleColor (1);
  ImGui::PopStyleVar   (2);
  ImGui::PopID         ( );
}


auto OnTopColorCycle =
[ ]
{
  return ImColor::HSV ( 0.491666f, 
                            std::min ( static_cast <float>(0.161616f +  (dwFrameTime % 250) / 250.0f -
                                                                floor ( (dwFrameTime % 250) / 250.0f) ), 1.0f),
                                std::min ( static_cast <float>(0.333 +  (dwFrameTime % 500) / 500.0f -
                                                                floor ( (dwFrameTime % 500) / 500.0f) ), 1.0f) );
};

auto WireframeColorCycle =
[ ]
{
  return ImColor::HSV ( 0.133333f, 
                            std::min ( static_cast <float>(0.161616f +  (dwFrameTime % 250) / 250.0f -
                                                                floor ( (dwFrameTime % 250) / 250.0f) ), 1.0f),
                                std::min ( static_cast <float>(0.333 +  (dwFrameTime % 500) / 500.0f -
                                                                floor ( (dwFrameTime % 500) / 500.0f) ), 1.0f) );
};

auto SkipColorCycle =
[ ]
{
  return ImColor::HSV ( 0.0f, 
                            std::min ( static_cast <float>(0.161616f +  (dwFrameTime % 250) / 250.0f -
                                                                floor ( (dwFrameTime % 250) / 250.0f) ), 1.0f),
                                std::min ( static_cast <float>(0.333 +  (dwFrameTime % 500) / 500.0f -
                                                                floor ( (dwFrameTime % 500) / 500.0f) ), 1.0f) );
};

auto HudColorCycle =
[ ]
{
  return ImColor::HSV ( 0.0f, 0.0f,                             
                                std::min ( static_cast <float>(0.333 +  (dwFrameTime % 500) / 500.0f -
                                                                floor ( (dwFrameTime % 500) / 500.0f) ), 1.0f) );
};

void
SK_D3D11_ClearShaderState (void) 
{
  static auto& shaders =
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
          return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders.vertex;
        case 1:
          return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders.pixel;
        case 2:
          return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders.geometry;
        case 3:
          return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders.hull;
        case 4:
          return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders.domain;
        case 5:
          return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders.compute;
      }
    };

    auto record =
      shader_record ();

    record->blacklist.clear              ();
    record->blacklist_if_texture.clear   ();
    record->wireframe.clear              ();
    record->on_top.clear                 ();
    record->trigger_reshade.before.clear ();
    record->trigger_reshade.after.clear  ();
  //record->hud.clear                    ();
  }
};

std::unordered_map <std::wstring, bool>&
_SK_D3D11_LoadedShaderConfigs (void)
{
  static std::unordered_map <std::wstring, bool> __loaded;
  return                                         __loaded;
}

void
SK_D3D11_LoadShaderStateEx (std::wstring name, bool clear)
{
  static auto& requested =
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

  auto shader_class_idx = [](const std::wstring& name)
  {
         if (name == L"Vertex")   return 0;
    else if (name == L"Pixel")    return 1;
    else if (name == L"Geometry") return 2;
    else if (name == L"Hull")     return 3;
    else if (name == L"Domain")   return 4;
    else if (name == L"Compute")  return 5;
    else                          return 6;
  };

  while (sec != sections.end ())
  {
    if (std::wcsstr ((*sec).first.c_str (), L"DrawState."))
    {
      wchar_t wszClass [32] = { };

      _snwscanf ((*sec).first.c_str (), 31, L"DrawState.%31s", wszClass);

      shader_class =
        shader_class_idx (wszClass);

      for ( auto it : (*sec).second.keys )
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
          if (!      _wcsicmp (wszTok, L"Wireframe"))
            draw_states [shader_class].wireframe.emplace (shader);
          else if (! _wcsicmp (wszTok, L"OnTop"))
            draw_states [shader_class].on_top.emplace (shader);
          else if (! _wcsicmp (wszTok, L"Disable"))
            draw_states [shader_class].disable.emplace (shader);
          else if (! _wcsicmp (wszTok, L"TriggerReShade"))
          {
            draw_states [shader_class].trigger_reshade_before.emplace (shader);
          }
          else if (! _wcsicmp (wszTok, L"TriggerReShadeAfter"))
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
          else if (! _wcsicmp (wszTok, L"HUD"))
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

  if (clear)
  {
    SK_D3D11_ClearShaderState ();
    sections.clear            ();
  }

  static auto& shaders =
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
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders.vertex;
          case 1:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders.pixel;
          case 2:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders.geometry;
          case 3:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders.hull;
          case 4:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders.domain;
          case 5:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders.compute;
        }
      };

    shader_record ()->blacklist.insert ( draw_states [i].disable.begin   (), draw_states [i].disable.end   () );
    shader_record ()->wireframe.insert ( draw_states [i].wireframe.begin (), draw_states [i].wireframe.end () );
    shader_record ()->on_top.insert    ( draw_states [i].on_top.begin    (), draw_states [i].on_top.end    () );

    for (auto it : draw_states [i].trigger_reshade_before)
    {
      shader_record ()->trigger_reshade.before.insert (it);
    }

    for (auto it : draw_states [i].trigger_reshade_after)
    {
      shader_record ()->trigger_reshade.after.insert (it);
    }

    for ( auto it : draw_states [i].disable_if_texture )
    {
      shader_record ()->blacklist_if_texture [it.first].insert (
        it.second.begin (),
          it.second.end ()
      );
    }

    shader_record ()->hud.insert ( draw_states [i].hud_shader.begin (),
                                   draw_states [i].hud_shader.end   () );
  }


  if (! sections.empty ())
  {
    if ( (! requested.count (name)) ||
                  requested [name]  == false )
    {
      requested [name] = true;
      InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
    }
  }

  if ( sections.empty () || clear )
  {
    if ( requested.count (name) &&
               requested [name] == true )
    {
      requested [name] = false;
      InterlockedDecrement (&SK_D3D11_DrawTrackingReqs);
    }
  }
}

void
SK_D3D11_LoadShaderState (bool clear = true)
{
  SK_D3D11_LoadShaderStateEx (L"d3d11_shaders.ini", clear);
}

void
SK_D3D11_UnloadShaderState (std::wstring name)
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
    else if (name == L"Pixel")    return 1;
    else if (name == L"Geometry") return 2;
    else if (name == L"Hull")     return 3;
    else if (name == L"Domain")   return 4;
    else if (name == L"Compute")  return 5;
    else                          return 6;
  };

  while (sec != sections.end ())
  {
    if (std::wcsstr ((*sec).first.c_str (), L"DrawState."))
    {
      wchar_t wszClass [32] = { };

      _snwscanf ((*sec).first.c_str (), 31, L"DrawState.%31s", wszClass);

      shader_class =
        shader_class_idx (wszClass);

      for ( auto it : (*sec).second.keys )
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
          if (! _wcsicmp (wszTok, L"Wireframe"))
            draw_states [shader_class].wireframe.emplace (shader);
          else if (! _wcsicmp (wszTok, L"OnTop"))
            draw_states [shader_class].on_top.emplace (shader);
          else if (! _wcsicmp (wszTok, L"Disable"))
            draw_states [shader_class].disable.emplace (shader);
          else if (! _wcsicmp (wszTok, L"TriggerReShade"))
          {
            draw_states [shader_class].trigger_reshade_before.emplace (shader);
          }
          else if (! _wcsicmp (wszTok, L"TriggerReShadeAfter"))
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
          else if (! _wcsicmp (wszTok, L"HUD"))
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

  static auto& shaders =
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
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders.vertex;
          case 1:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders.pixel;
          case 2:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders.geometry;
          case 3:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders.hull;
          case 4:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders.domain;
          case 5:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders.compute;
        }
      };

    for (auto it : draw_states [i].disable)
      shader_record ()->blacklist.erase (it);

    for (auto it : draw_states [i].wireframe)
      shader_record ()->wireframe.erase (it);

    for (auto it : draw_states [i].on_top)
      shader_record ()->on_top.erase (it);

    for (auto it : draw_states [i].trigger_reshade_before)
      shader_record ()->trigger_reshade.before.erase (it);

    for (auto it : draw_states [i].trigger_reshade_after)
      shader_record ()->trigger_reshade.after.erase (it);

    for ( auto it : draw_states [i].disable_if_texture )
    {
      for (auto it2 : it.second)
      {
        shader_record ()->blacklist_if_texture [it.first].erase (it2);
      }
    }

    for (auto it : draw_states [i].hud_shader)
      shader_record ()->hud.erase (it);
  }


  static auto& requested =
    _SK_D3D11_LoadedShaderConfigs ();

  if (requested.count (name))
  {
    if (requested [name] == true)
    {
      requested [name] = false;
      InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);
    }
  }
}



void
EnumConstantBuffer ( ID3D11ShaderReflectionConstantBuffer* pConstantBuffer,
                     shader_disasm_s::constant_buffer&     cbuffer )
{
  if (! pConstantBuffer)
    return;

  D3D11_SHADER_BUFFER_DESC cbuffer_desc = { };

  if (SUCCEEDED (pConstantBuffer->GetDesc (&cbuffer_desc)))
  {
    //if (constant_desc.DefaultValue != nullptr)
    //  memcpy (constant.Data, constant_desc.DefaultValue, std::min (static_cast <size_t> (constant_desc.Bytes), sizeof (float) * 4));

    cbuffer.structs.push_back ( { } );

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

                dll_log.Log  (L"%hs.%hs <%lu> {%lu}", this_struct.second.c_str (), mem_var.name.c_str (), mem_var.var_desc.StartOffset, mem_var.var_desc.Size);

                this_struct.first.emplace_back (mem_var);
              }

              cbuffer.structs.emplace_back (this_struct);
            }

            else
            {
              if (var.var_desc.DefaultValue != nullptr)
              {
                memcpy ( (void *)var.default_value,
                                 var.var_desc.DefaultValue, 
                         std::min (16 * sizeof (float), (size_t)var.var_desc.Size) );
              }

              unnamed_struct.first.emplace_back (var);
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
    auto secs =
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

  static auto& shaders =
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
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders.vertex;
          case 1:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders.pixel;
          case 2:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders.geometry;
          case 3:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders.hull;
          case 4:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders.domain;
          case 5:
            return (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders.compute;
        }
      };

    iSK_INISection& sec =
      d3d11_shaders_ini->get_section_f (L"DrawState.%s", shader_class_name (i));

    std::set <uint32_t> shader_set;

    const auto& record =
      shader_record ();

    shader_set.insert ( record->blacklist.begin              (), record->blacklist.end              () );
    shader_set.insert ( record->wireframe.begin              (), record->wireframe.end              () );
    shader_set.insert ( record->on_top.begin                 (), record->on_top.end                 () );
    shader_set.insert ( record->trigger_reshade.before.begin (), record->trigger_reshade.before.end () );
    shader_set.insert ( record->trigger_reshade.after.begin  (), record->trigger_reshade.after.end  () );

    for (auto it : record->blacklist_if_texture)
    {
      shader_set.emplace ( it.first );
    }

    for ( auto it : shader_set )
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
        for ( auto it2 : record->blacklist_if_texture [it] )
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

  d3d11_shaders_ini->write (d3d11_shaders_ini->get_filename ());
}

void
SK_LiveShaderClassView (sk_shader_class shader_type, bool& can_scroll)
{
  std::lock_guard <SK_Thread_CriticalSection> auto_lock (*cs_shader);

  ImGuiIO& io (ImGui::GetIO ());

  static auto& shaders =
    SK_D3D11_Shaders;

  ImGui::PushID (static_cast <int> (shader_type));

  static float last_width = 256.0f;
  const  float font_size  = ImGui::GetFont ()->FontSize * io.FontGlobalScale;

  struct shader_class_imp_s
  {
    std::vector <
      std::pair <uint32_t, bool>
    >                         contents;
    bool                      dirty      = true;
    uint32_t                  last_sel   =    std::numeric_limits <uint32_t>::max ();
    unsigned int                   sel   =    0;
    float                     last_ht    = 256.0f;
    ImVec2                    last_min   = ImVec2 (0.0f, 0.0f);
    ImVec2                    last_max   = ImVec2 (0.0f, 0.0f);
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
    [](const sk_shader_class& type) ->
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
    [](const sk_shader_class& type) ->
      d3d11_shader_tracking_s*
      {
        switch (type)
        {
          case sk_shader_class::Vertex:   return &shaders.vertex.tracked;
          case sk_shader_class::Pixel:    return &shaders.pixel.tracked;
          case sk_shader_class::Geometry: return &shaders.geometry.tracked;
          case sk_shader_class::Hull:     return &shaders.hull.tracked;
          case sk_shader_class::Domain:   return &shaders.domain.tracked;
          case sk_shader_class::Compute:  return &shaders.compute.tracked;
        }

        assert (false);

        return nullptr;
      };

  d3d11_shader_tracking_s*
    tracker = GetShaderTracker (shader_type);

  auto GetShaderSet =
    [](const sk_shader_class& type) ->
      std::set <uint32_t>&
      {
        static std::set <uint32_t> set  [6];
        static size_t              size [6];

        switch (type)
        {
          case sk_shader_class::Vertex:
          {
            static auto& vertex =
              shaders.vertex;

            if (size [0] == vertex.descs.size ())
              return set [0];

            for (auto const& vertex_shader : vertex.descs)
            {
              // Ignore ImGui / CEGUI shaders
              if ( vertex_shader.first != 0xb42ede74 &&
                   vertex_shader.first != 0x1f8c62dc )
              {
                if (vertex_shader.first > 0x00000000) set [0].emplace (vertex_shader.first);
              }
            }

                   size [0] = vertex.descs.size ();
            return set  [0];
          } break;

          case sk_shader_class::Pixel:
          {
            static auto& pixel =
              shaders.pixel;

            if (size [1] == pixel.descs.size ())
              return set [1];

            for (auto const& pixel_shader : pixel.descs)
            {
              // Ignore ImGui / CEGUI shaders
              if ( pixel_shader.first != 0xd3af3aa0 &&
                   pixel_shader.first != 0xb04a90ba )
              {
                if (pixel_shader.first > 0x00000000) set [1].emplace (pixel_shader.first);
              }
            }

                   size [1] = pixel.descs.size ();
            return set  [1];
          } break;

          case sk_shader_class::Geometry:
          {
            static auto& geometry =
              shaders.geometry;

            if (size [2] == geometry.descs.size ())
              return set [2];

            for (auto const& geometry_shader : geometry.descs)
            {
              if (geometry_shader.first > 0x00000000) set [2].emplace (geometry_shader.first);
            }

                   size [2] = geometry.descs.size ();
            return set  [2];
          } break;

          case sk_shader_class::Hull:
          {
            static auto& hull =
              shaders.hull;

            if (size [3] == hull.descs.size ())
              return set [3];

            for (auto const& hull_shader : hull.descs)
            {
              if (hull_shader.first > 0x00000000) set [3].emplace (hull_shader.first);
            }

                   size [3] = hull.descs.size ();
            return set  [3];
          } break;

          case sk_shader_class::Domain:
          {
            static auto& domain =
              shaders.domain;

            if (size [4] == domain.descs.size ())
              return set [4];

            for (auto const& domain_shader : domain.descs)
            {
              if (domain_shader.first > 0x00000000) set [4].emplace (domain_shader.first);
            }

                   size [4] = domain.descs.size ();
            return set  [4];
          } break;

          case sk_shader_class::Compute:
          {
            static auto& compute =
              shaders.compute;

            if (size [5] == compute.descs.size ())
              return set [5];

            for (auto const& compute_shader : compute.descs)
            {
              if (compute_shader.first > 0x00000000) set [5].emplace (compute_shader.first);
            }

                   size [5] = compute.descs.size ();
            return set  [5];
          } break;
        }

        return set [0];
      };

  std::set <uint32_t>& set =
    GetShaderSet (shader_type);

  std::vector <uint32_t>
    shader_set ( set.begin (), set.end () );

  auto GetShaderDisasm =
    [](const sk_shader_class& type) ->
      std::unordered_map <uint32_t, shader_disasm_s>*
      {
        switch (type)
        {
          case sk_shader_class::Vertex:   return &vs_disassembly;
          default:
          case sk_shader_class::Pixel:    return &ps_disassembly;
          case sk_shader_class::Geometry: return &gs_disassembly;
          case sk_shader_class::Hull:     return &hs_disassembly;
          case sk_shader_class::Domain:   return &ds_disassembly;
          case sk_shader_class::Compute:  return &cs_disassembly;
        }
      };

  std::unordered_map <uint32_t, shader_disasm_s>*
    disassembly = GetShaderDisasm (shader_type);

  auto GetShaderWord =
    [](const sk_shader_class& type) ->
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

  uint32_t invalid;

  const
   auto
    GetShaderChange =
    [&](const sk_shader_class& type) ->
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
    [&](const sk_shader_class& type)->
      std::unordered_set <uint32_t>&
      {
        static std::unordered_set <uint32_t> invalid;

        switch (type)
        {
          case sk_shader_class::Vertex:   return shaders.vertex.blacklist;
          case sk_shader_class::Pixel:    return shaders.pixel.blacklist;
          case sk_shader_class::Geometry: return shaders.geometry.blacklist;
          case sk_shader_class::Hull:     return shaders.hull.blacklist;
          case sk_shader_class::Domain:   return shaders.domain.blacklist;
          case sk_shader_class::Compute:  return shaders.compute.blacklist;
          default:                        return invalid;
        }
      };

  auto GetShaderBlacklistEx =
    [&](const sk_shader_class& type)->
      SK_D3D11_KnownShaders::conditional_blacklist_t&
      {
        static SK_D3D11_KnownShaders::conditional_blacklist_t invalid;

        switch (type)
        {
          case sk_shader_class::Vertex:   return shaders.vertex.blacklist_if_texture;
          case sk_shader_class::Pixel:    return shaders.pixel.blacklist_if_texture;
          case sk_shader_class::Geometry: return shaders.geometry.blacklist_if_texture;
          case sk_shader_class::Hull:     return shaders.hull.blacklist_if_texture;
          case sk_shader_class::Domain:   return shaders.domain.blacklist_if_texture;
          case sk_shader_class::Compute:  return shaders.compute.blacklist_if_texture;
          default:                        return invalid;
        }
      };

  auto GetShaderUsedResourceViews =
    [&](const sk_shader_class& type)->
      std::vector <ID3D11ShaderResourceView *>&
      {
        static std::vector <ID3D11ShaderResourceView *> invalid;

        switch (type)
        {
          case sk_shader_class::Vertex:   return shaders.vertex.tracked.used_views;
          case sk_shader_class::Pixel:    return shaders.pixel.tracked.used_views;
          case sk_shader_class::Geometry: return shaders.geometry.tracked.used_views;
          case sk_shader_class::Hull:     return shaders.hull.tracked.used_views;
          case sk_shader_class::Domain:   return shaders.domain.tracked.used_views;
          case sk_shader_class::Compute:  return shaders.compute.tracked.used_views;
          default:                        return invalid;
        }
      };

  auto GetShaderResourceSet =
    [&](const sk_shader_class& type)->
      std::set <ID3D11ShaderResourceView *>&
      {
        static std::set <ID3D11ShaderResourceView *> invalid;

        switch (type)
        {
          case sk_shader_class::Vertex:   return shaders.vertex.tracked.set_of_views;
          case sk_shader_class::Pixel:    return shaders.pixel.tracked.set_of_views;
          case sk_shader_class::Geometry: return shaders.geometry.tracked.set_of_views;
          case sk_shader_class::Hull:     return shaders.hull.tracked.set_of_views;
          case sk_shader_class::Domain:   return shaders.domain.tracked.set_of_views;
          case sk_shader_class::Compute:  return shaders.compute.tracked.set_of_views;
          default:                        return invalid;
        }
      };

  struct sk_shader_state_s {
    unsigned int last_sel      = 0;
            bool sel_changed   = false;
            bool hide_inactive = true;
    unsigned int active_frames = 3;
            bool hovering      = false;
            bool focused       = false;

    static int ClassToIdx (const sk_shader_class& shader_class)
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
  } static shader_state [6];

  unsigned int& last_sel      =  shader_state [sk_shader_state_s::ClassToIdx (shader_type)].last_sel;
  bool&         sel_changed   =  shader_state [sk_shader_state_s::ClassToIdx (shader_type)].sel_changed;
  bool*         hide_inactive = &shader_state [sk_shader_state_s::ClassToIdx (shader_type)].hide_inactive;
  unsigned int& active_frames =  shader_state [sk_shader_state_s::ClassToIdx (shader_type)].active_frames;
  bool scrolled = false;

  int dir = 0;

  if (ImGui::IsMouseHoveringRect (list->last_min, list->last_max))
  {
         if (io.KeysDown [VK_OEM_4] && io.KeysDownDuration [VK_OEM_4] == 0.0f) { dir = -1;  io.WantCaptureKeyboard = true; scrolled = true; }
    else if (io.KeysDown [VK_OEM_6] && io.KeysDownDuration [VK_OEM_6] == 0.0f) { dir = +1;  io.WantCaptureKeyboard = true; scrolled = true; }
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
          ID3D11RenderTargetView* rt_view = tracker->pre_hud_rtv;
  
          D3D11_RENDER_TARGET_VIEW_DESC rtv_desc = { };
          rt_view->GetDesc            (&rtv_desc);
  
          if (rtv_desc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2D)
          {
            CComPtr <ID3D11Resource>          pRes = nullptr;
            rt_view->GetResource            (&pRes.p);
            CComQIPtr <ID3D11Texture2D> pTex (pRes);

            if (pRes && pTex)
            {
              D3D11_TEXTURE2D_DESC desc = { };
              pTex->GetDesc      (&desc);

              ID3D11ShaderResourceView*       pSRV     = nullptr;
              D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = { };

              static const SK_RenderBackend& rb =
                SK_GetCurrentRenderBackend ();

              CComQIPtr <ID3D11Device> pDev (rb.device);

              CComPtr <ID3D11Texture2D> pTex2;

              if ((! (desc.BindFlags & D3D11_BIND_SHADER_RESOURCE)) || desc.Usage == D3D11_USAGE_DYNAMIC ||
                      desc.Format     !=     SK_D3D11_MakeTypedFormat (desc.Format)                      ||
                      rtv_desc.Format !=     SK_D3D11_MakeTypedFormat (rtv_desc.Format) ||
                      rtv_desc.Format !=     desc.Format)
              {
                desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
                desc.Format = SK_D3D11_MakeTypedFormat (desc.Format);
                pDev->CreateTexture2D (&desc, nullptr, &pTex2);

                CComQIPtr <ID3D11DeviceContext> pDevCtx (rb.d3d11.immediate_ctx);

                if (pDevCtx.p != nullptr)
                    pDevCtx->ResolveSubresource (pTex2, 0, pTex, 0, desc.Format);
              }

              srv_desc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
              srv_desc.Format                    = desc.Format;
              srv_desc.Texture2D.MipLevels       = (UINT)-1;
              srv_desc.Texture2D.MostDetailedMip =  0;

              if (pDev.p != nullptr)
              {
                const bool success =
                  SUCCEEDED (pDev->CreateShaderResourceView (pTex2 != nullptr ? pTex2 : pTex, &srv_desc, &pSRV));
  
                if (success)
                {
                  ImGui::BeginTooltip    ();
                  ImGui::PushStyleColor  (ImGuiCol_Border, ImVec4 (0.5f, 0.5f, 0.5f, 1.0f));

                  ImGui::BeginGroup (                  );
                  ImGui::Text       ( "Dimensions:   " );
                  ImGui::Text       ( "Format:       " );
                  ImGui::EndGroup   (                  );
  
                  ImGui::SameLine   ( );
  
                  ImGui::BeginGroup (                                              );
                  ImGui::Text       ( "%lux%lu",
                                        desc.Width, desc.Height/*, effective_width, effective_height, 0.9875f * content_avail_y - ((float)(bottom_list + 3) * font_size * 1.125f), content_avail_y*//*,
                                          pTex->d3d9_tex->GetLevelCount ()*/       );
                  ImGui::Text       ( "%ws",
                                        SK_DXGI_FormatToStr (desc.Format).c_str () );
                  ImGui::EndGroup   (                                              );
    
                  if (pSRV != nullptr)
                  {
                    const float effective_width  = 512.0f;
                    const float effective_height = effective_width / ((float)desc.Width / (float)desc.Height);

                    ImGui::Separator  ( );

                    ImGui::PushStyleColor (ImGuiCol_Border, ImVec4 (0.95f, 0.95f, 0.05f, 1.0f));
                    ImGui::BeginChildFrame   (ImGui::GetID ("ShaderResourceView_RT_Frame"),
                                              ImVec2 (effective_width + 8.0f, effective_height + 8.0f),
                                              ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_ShowBorders );

                    temp_resources.push_back (rt_view);
                                              rt_view->AddRef ();

                    temp_resources.push_back (pSRV);
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
        case sk_shader_class::Vertex:   return &shaders.vertex;
        case sk_shader_class::Pixel:    return &shaders.pixel;
        case sk_shader_class::Geometry: return &shaders.geometry;
        case sk_shader_class::Hull:     return &shaders.hull;
        case sk_shader_class::Domain:   return &shaders.domain;
        case sk_shader_class::Compute:  return &shaders.compute;
        default:
        return nullptr;
      }
    };


  auto GetShaderDesc = [&](sk_shader_class type, uint32_t crc32c) ->
    SK_D3D11_ShaderDesc&
      {
        return
          ((SK_D3D11_KnownShaders::ShaderRegistry <ID3D11VertexShader>*)ShaderBase (type))->descs [
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

    for ( auto it : shader_set )
    {
      const bool disabled =
        ( GetShaderBlacklist (shader_type).count (it) != 0 );

      list->contents.emplace_back (std::make_pair (it, (! disabled)));

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

  ImGui::PushStyleVar   (ImGuiStyleVar_ChildWindowRounding, 0.0f);
  ImGui::PushStyleColor (ImGuiCol_Border, ImVec4 (0.9f, 0.7f, 0.5f, 1.0f));

  bool& hovering = shader_state [sk_shader_state_s::ClassToIdx (shader_type)].hovering;
  bool& focused  = shader_state [sk_shader_state_s::ClassToIdx (shader_type)].focused;

  ImGui::BeginChild ( ImGui::GetID (GetShaderWord (shader_type)),
                      ImVec2 ( font_size * 7.0f, std::max (font_size * 15.0f, list->last_ht)),
                        true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NavFlattened );

  if (hovering || focused)
  {
    can_scroll = false;

    if (! focused)
    {
      ImGui::BeginTooltip ();
      ImGui::TextColored  (ImVec4 (0.9f, 0.6f, 0.2f, 1.0f), "You can cancel all render passes using the selected %s shader to disable an effect", szShaderWord);
      ImGui::Separator    ();
      ImGui::BulletText   ("Press %ws while the mouse is hovering this list to select the previous shader", virtKeyCodeToHumanKeyName [VK_OEM_4].c_str ());
      ImGui::BulletText   ("Press %ws while the mouse is hovering this list to select the next shader",     virtKeyCodeToHumanKeyName [VK_OEM_6].c_str ());
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
          if  (io.NavInputs [ImGuiNavInput_PadFocusPrev] && io.NavInputsDownDuration [ImGuiNavInput_PadFocusPrev] == 0.0f) { dir = -1; }
      else if (io.NavInputs [ImGuiNavInput_PadFocusNext] && io.NavInputsDownDuration [ImGuiNavInput_PadFocusNext] == 0.0f) { dir =  1; }

      else
      {
             if (io.KeysDown [VK_OEM_4] && io.KeysDownDuration [VK_OEM_4] == 0.0f) { dir = -1;  io.WantCaptureKeyboard = true; scrolled = true; }
        else if (io.KeysDown [VK_OEM_6] && io.KeysDownDuration [VK_OEM_6] == 0.0f) { dir = +1;  io.WantCaptureKeyboard = true; scrolled = true; }
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

        if ( sel >= shader_set.size () || sel < 0 )
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
                                    SK_D3D11_ShaderDesc&       rDesc ) 
    {
      list->last_sel              = rDesc.crc32c;
      tracker->crc32c             = rDesc.crc32c;
      tracker->runtime_ms         = 0.0;
      tracker->last_runtime_ms    = 0.0;
      tracker->runtime_ticks.store (0LL);
    };

    // Line counting; assume the list sort order is unchanged
    unsigned int line = 0;

    for ( auto& it : list->contents )
    {
      SK_D3D11_ShaderDesc& rDesc =
        GetShaderDesc (shader_type, it.first);

      const bool active =
        ShaderIsActive (shader_type, it.first);

      if ((! active) && (*hide_inactive))
      {
        line++;
        continue;
      }


      if (IsSkipped        (shader_type, it.first))
      {
        ImGui::PushStyleColor (ImGuiCol_Text, SkipColorCycle ());
      }

      else if (IsHud       (shader_type, it.first))
      {
        ImGui::PushStyleColor (ImGuiCol_Text, HudColorCycle ());
      }

      else if (IsOnTop     (shader_type, it.first))
      {
        ImGui::PushStyleColor (ImGuiCol_Text, OnTopColorCycle ());
      }

      else if (IsWireframe (shader_type, it.first))
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
        SK_FormatString ("%c%08x", it.second ? ' ' : '*', it.first);


      if (line == list->sel)
      {
        bool selected = true;

        ImGui::Selectable (line_text.c_str (), &selected);

        if (sel_changed)
        {
          ImGui::SetScrollHere        (0.5f);
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

      ImGui::PushID (it.first);

      if (SK_ImGui_IsItemRightClicked ())
      {
        ImGui::SetNextWindowSize (ImVec2 (-1.0f, -1.0f), ImGuiSetCond_Always);
        ImGui::OpenPopup         ("ShaderSubMenu");
      }

      if (ImGui::BeginPopup      ("ShaderSubMenu"))
      {
        static std::vector <ID3D11ShaderResourceView *> empty_list;
        static std::set    <ID3D11ShaderResourceView *> empty_set;

        ShaderMenu ( GetShaderBlacklist         (shader_type),
                     GetShaderBlacklistEx       (shader_type),
                     line == list->sel ? GetShaderUsedResourceViews (shader_type) :
                                         empty_list,
                     line == list->sel ? GetShaderResourceSet       (shader_type) :
                                         empty_set,
                     it.first );
        list->dirty = true;

        ImGui::EndPopup  ();
      }
      ImGui::PopID       ();

      line++;

      ImGui::PopStyleColor ();
    }

    CComPtr <ID3DBlob>               pDisasm  = nullptr;
    CComPtr <ID3D11ShaderReflection> pReflect = nullptr;

    HRESULT hr = E_FAIL;

    if (ReadAcquire ((volatile LONG *)&tracker->crc32c) != 0 && (! (*disassembly).count (ReadAcquire ((volatile LONG *)&tracker->crc32c))))
    {
      switch (shader_type)
      {
        case sk_shader_class::Vertex:
        {
          //if ( SK_D3D11_Shaders.vertex.descs [tracker->crc32c].bytecode.empty () )
          //{
            std::lock_guard <SK_Thread_CriticalSection> auto_lock_vs (*cs_shader_vs);

            static auto& vertex =
              SK_D3D11_Shaders.vertex;

            hr = D3DDisassemble ( vertex.descs [tracker->crc32c].bytecode.data (), vertex.descs [tracker->crc32c].bytecode.size (),
                                    D3D_DISASM_ENABLE_DEFAULT_VALUE_PRINTS, "", &pDisasm.p);
            if (SUCCEEDED (hr))
                 D3DReflect     ( vertex.descs [tracker->crc32c].bytecode.data (), vertex.descs [tracker->crc32c].bytecode.size (),
                                    IID_ID3D11ShaderReflection, (void **)&pReflect.p);
          //}
        } break;

        case sk_shader_class::Pixel:
        {
          //if ( SK_D3D11_Shaders.pixel.descs [tracker->crc32c].bytecode.empty () )
          //{
            std::lock_guard <SK_Thread_CriticalSection> auto_lock_ps (*cs_shader_ps);

            static auto& pixel =
              SK_D3D11_Shaders.pixel;

            hr = D3DDisassemble ( pixel.descs [tracker->crc32c].bytecode.data (), pixel.descs [tracker->crc32c].bytecode.size (),
                                    D3D_DISASM_ENABLE_DEFAULT_VALUE_PRINTS, "", &pDisasm.p);
            if (SUCCEEDED (hr))
                 D3DReflect     ( pixel.descs [tracker->crc32c].bytecode.data (), pixel.descs [tracker->crc32c].bytecode.size (),
                                    IID_ID3D11ShaderReflection, (void **)&pReflect.p);
          //}
        } break;

        case sk_shader_class::Geometry:
        {
          //if ( SK_D3D11_Shaders.geometry.descs [tracker->crc32c].bytecode.empty () )
          //{
            std::lock_guard <SK_Thread_CriticalSection> auto_lock_gs (*cs_shader_gs);

            static auto& geometry =
              SK_D3D11_Shaders.geometry;

            hr = D3DDisassemble ( geometry.descs [tracker->crc32c].bytecode.data (), geometry.descs [tracker->crc32c].bytecode.size (), 
                                    D3D_DISASM_ENABLE_DEFAULT_VALUE_PRINTS, "", &pDisasm.p);
            if (SUCCEEDED (hr))
                 D3DReflect     ( geometry.descs [tracker->crc32c].bytecode.data (), geometry.descs [tracker->crc32c].bytecode.size (),
                                    IID_ID3D11ShaderReflection, (void **)&pReflect.p);
          //}
        } break;

        case sk_shader_class::Hull:
        {
          //if ( SK_D3D11_Shaders.hull.descs [tracker->crc32c].bytecode.empty () )
          //{
            std::lock_guard <SK_Thread_CriticalSection> auto_lock_hs (*cs_shader_hs);

            static auto& hull =
              SK_D3D11_Shaders.hull;

            hr = D3DDisassemble ( hull.descs [tracker->crc32c].bytecode.data (), hull.descs [tracker->crc32c].bytecode.size (), 
                                    D3D_DISASM_ENABLE_DEFAULT_VALUE_PRINTS, "", &pDisasm.p);
            if (SUCCEEDED (hr))
                 D3DReflect     ( hull.descs [tracker->crc32c].bytecode.data (), hull.descs [tracker->crc32c].bytecode.size (),
                                    IID_ID3D11ShaderReflection, (void **)&pReflect.p);
          //}
        } break;

        case sk_shader_class::Domain:
        {
          //if ( SK_D3D11_Shaders.domain.descs [tracker->crc32c].bytecode.empty () )
          //{
            std::lock_guard <SK_Thread_CriticalSection> auto_lock_ds (*cs_shader_ds);

            static auto& domain =
              SK_D3D11_Shaders.domain;

            hr = D3DDisassemble ( domain.descs [tracker->crc32c].bytecode.data (), domain.descs [tracker->crc32c].bytecode.size (), 
                                    D3D_DISASM_ENABLE_DEFAULT_VALUE_PRINTS, "", &pDisasm.p);
            if (SUCCEEDED (hr))
                 D3DReflect     ( domain.descs [tracker->crc32c].bytecode.data (), domain.descs [tracker->crc32c].bytecode.size (),
                                    IID_ID3D11ShaderReflection, (void **)&pReflect.p);
          //}
        } break;

        case sk_shader_class::Compute:
        {
          //if ( SK_D3D11_Shaders.compute.descs [tracker->crc32c].bytecode.empty () )
          //{
            std::lock_guard <SK_Thread_CriticalSection> auto_lock_cs (*cs_shader_cs);

            static auto& compute =
              SK_D3D11_Shaders.compute;

            hr = D3DDisassemble ( compute.descs [tracker->crc32c].bytecode.data (), compute.descs [tracker->crc32c].bytecode.size (), 
                                    D3D_DISASM_ENABLE_DEFAULT_VALUE_PRINTS, "", &pDisasm.p);
            if (SUCCEEDED (hr))
                 D3DReflect     ( compute.descs [tracker->crc32c].bytecode.data (), compute.descs [tracker->crc32c].bytecode.size (),
                                    IID_ID3D11ShaderReflection, (void **)&pReflect.p);
          //}
        } break;
      }

      if (SUCCEEDED (hr)    && strlen ((const char *)pDisasm->GetBufferPointer ()))
      {
        char* szDisasm      = _strdup ((const char *)pDisasm->GetBufferPointer ());
        char* comments_end  = nullptr;

        if (szDisasm && strlen (szDisasm))
        {
          comments_end = strstr (szDisasm,          "\nvs_");
          if (! comments_end)
            comments_end      =                strstr (szDisasm,          "\nps_");
          if (! comments_end)
            comments_end      =                strstr (szDisasm,          "\ngs_");
          if (! comments_end)
            comments_end      =                strstr (szDisasm,          "\nhs_");
          if (! comments_end)
            comments_end      =                strstr (szDisasm,          "\nds_");
          if (! comments_end)
            comments_end      =                strstr (szDisasm,          "\ncs_");
          char* footer_begins = comments_end ? strstr (comments_end + 1, "\n//") : nullptr;

          if (comments_end)  *comments_end  = '\0'; else { *comments_end  = '\0'; lstrcatA (comments_end,  "  "); }
          if (footer_begins) *footer_begins = '\0'; else { *footer_begins = '\0'; lstrcatA (footer_begins, "  "); }

          disassembly->emplace ( ReadAcquire ((volatile LONG *)&tracker->crc32c),
                                   shader_disasm_s { szDisasm,
                                                       comments_end + 1,
                                                         footer_begins + 1
                                                   }
                               );

          if (pReflect)
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

  if (ImGui::IsItemHovered ()) hovering = true; else hovering = false;
  if (ImGui::IsItemFocused ()) focused  = true; else focused  = false;

  ImGui::PopStyleVar   ();
  ImGui::PopStyleColor ();

  ImGui::SameLine      ();
  ImGui::BeginGroup    ();

  if (ImGui::IsItemHoveredRect ())
  {
    if (! scrolled)
    {
           if (io.KeysDownDuration [VK_OEM_4] == 0.0f) list->sel--;
      else if (io.KeysDownDuration [VK_OEM_6] == 0.0f) list->sel++;
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
    case sk_shader_class::Vertex:   pShader = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders.vertex;   break;
    case sk_shader_class::Pixel:    pShader = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders.pixel;    break;
    case sk_shader_class::Geometry: pShader = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders.geometry; break;
    case sk_shader_class::Hull:     pShader = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders.hull;     break;
    case sk_shader_class::Domain:   pShader = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders.domain;   break;
    case sk_shader_class::Compute:  pShader = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&shaders.compute;  break;
  }


  if (                                              pShader != nullptr &&
       ReadAcquire ((volatile LONG *)&tracker->crc32c)      != 0x00    &&
                                       (SSIZE_T)(list->sel) >= 0       &&
                                                 list->sel  < (int)list->contents.size () )
  {
    ImGui::BeginGroup ();

    int num_used_textures = 0;

    std::set <ID3D11ShaderResourceView *> unique_views;

    if (! tracker->used_views.empty ())
    {
      for ( auto it : tracker->used_views )
      {
        if (! unique_views.emplace (it).second)
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

    if (shader_type != sk_shader_class::Compute)
    {
      if (tracker->cancel_draws)
        snprintf (szDraws, 128, "%3lu Skipped Draw%sLast Frame",  tracker->num_draws.load (), tracker->num_draws != 1 ? "s " : " ");
      else
        snprintf (szDraws, 128, "%3lu Draw%sLast Frame        " , tracker->num_draws.load (), tracker->num_draws != 1 ? "s " : " ");
    }

    else
    {
      if (tracker->cancel_draws)
        snprintf (szDraws, 128, "%3lu Skipped Dispatch%sLast Frame", tracker->num_draws.load (), tracker->num_draws != 1 ? "es " : " ");
      else
        snprintf (szDraws, 128, "%3lu Dispatch%sLast Frame        ", tracker->num_draws.load (), tracker->num_draws != 1 ? "es " : " ");
    }

    snprintf (szTextures,   64, "(%#i %s)",  num_used_textures, num_used_textures != 1 ? "textures" : "texture");
    snprintf (szRuntime,    32,  "%0.4f ms", tracker->runtime_ms);
    snprintf (szAvgRuntime, 32,  "%0.4f ms", tracker->runtime_ms / tracker->num_draws);

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
        pShader->blacklist.emplace (tracker->crc32c);
      else
        pShader->blacklist.erase   (tracker->crc32c);
    }

    if (skip)
      ImGui::PopStyleColor ();

    if (shader_type != sk_shader_class::Compute)
    {
      wireframe = pShader->wireframe.count (tracker->crc32c);
      on_top    = pShader->on_top.count    (tracker->crc32c);
      hud       = pShader->hud.count       (tracker->crc32c);

      const bool wire = wireframe;

      if (wire)
        ImGui::PushStyleColor (ImGuiCol_Text, WireframeColorCycle ());

      if (ImGui::Checkbox ("Always Draw in Wireframe", &wireframe))
      {
        if (wireframe)
          pShader->wireframe.emplace (tracker->crc32c);
        else
          pShader->wireframe.erase   (tracker->crc32c);
      }

      if (wire)
        ImGui::PopStyleColor ();

      const bool top = on_top;

      if (top)
        ImGui::PushStyleColor (ImGuiCol_Text, OnTopColorCycle ());

      if (ImGui::Checkbox ("Always Draw on Top", &on_top))
      {
        if (on_top)
          pShader->on_top.emplace (tracker->crc32c);
        else
          pShader->on_top.erase   (tracker->crc32c);
      }

      if (top)
        ImGui::PopStyleColor ();

      const bool hud_shader = hud;

      if (hud_shader)
        ImGui::PushStyleColor (ImGuiCol_Text, HudColorCycle ());

      if (ImGui::Checkbox ("Shader Belongs to HUD", &hud))
      {
        if (hud)
          pShader->hud.emplace (tracker->crc32c);
        else
          pShader->hud.erase   (tracker->crc32c);
      }

      if (hud_shader)
        ImGui::PopStyleColor ();
    }
    else
    {
      ImGui::Columns  (1);
    }

    if (SK_ReShade_PresentCallback.fn != nullptr && shader_type != sk_shader_class::Compute)
    {
      bool reshade_before = pShader->trigger_reshade.before.count (tracker->crc32c);
      bool reshade_after  = pShader->trigger_reshade.after.count  (tracker->crc32c);

      if (ImGui::Checkbox ("Trigger ReShade On First Draw", &reshade_before))
      {
        if (reshade_before)
          pShader->trigger_reshade.before.emplace (tracker->crc32c);
        else
          pShader->trigger_reshade.before.erase   (tracker->crc32c);
      }

      if (ImGui::Checkbox ("Trigger ReShade After First Draw", &reshade_after))
      {
        if (reshade_after)
          pShader->trigger_reshade.after.emplace (tracker->crc32c);
        else
          pShader->trigger_reshade.after.erase   (tracker->crc32c);
      }
    }

    else
    {
      bool antistall = (ReadAcquire (&__SKX_ComputeAntiStall) != 0);

      if (ImGui::Checkbox ("Use ComputeShader Anti-Stall For Buffer Injection", &antistall))
      {
        InterlockedExchange (&__SKX_ComputeAntiStall, antistall ? 1 : 0);
      }
    }


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
    ImGui::TextDisabled (tracker->num_draws > 0 ? szRuntime    : "N/A");
    ImGui::TextDisabled (tracker->num_draws > 0 ? szAvgRuntime : "N/A");
    ImGui::EndGroup     ( );
    ImGui::EndGroup     ( );

    ImGui::Columns      (1);
    ImGui::Separator    ( );

    ImGui::PushFont (io.Fonts->Fonts [1]); // Fixed-width font

    ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.80f, 0.80f, 1.0f, 1.0f));
    ImGui::TextWrapped    ((*disassembly) [tracker->crc32c].header.c_str ());
    ImGui::PopStyleColor  ();
    
    ImGui::SameLine       ();
    ImGui::BeginGroup     ();

    ImGui::TreePush       ("");
    ImGui::Spacing        (); ImGui::Spacing ();


    ImGui::PushID (tracker->crc32c);

    size_t min_size = 0;

    for (auto&& it : (*disassembly) [tracker->crc32c].cbuffers)
    {
      ImGui::PushID (it.Slot);

      if (! tracker->overrides.empty ())
      {
        if (tracker->overrides [0].parent != tracker->crc32c)
        {
          tracker->overrides.clear ();
        }
      }

      for (auto&& itS : it.structs)
      {
        if (! itS.second.empty ())
        {
          ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.3f, 0.6f, 0.9f, 1.0f));
          ImGui::Text           (itS.second.c_str ());
          ImGui::PopStyleColor  (  );
          ImGui::TreePush       ("");
        }
      for (auto&& it2 : itS.first)
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
                                   std::min ((size_t)it2.var_desc.Size, sizeof (float) * 16) );
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
            ImGui::InputFloatN (it2.name.c_str (), &override_inst.Values [0], (int)it2.type_desc.Columns, 4, 0x0);
          }

          else if ( it2.type_desc.Type == D3D_SVT_INT  ||
                    it2.type_desc.Type == D3D_SVT_UINT ||
                    it2.type_desc.Type == D3D_SVT_UINT8 )
          {
            if (it2.type_desc.Type == D3D_SVT_INT)
            {
              ImGui::SameLine  ();
              ImGui::InputIntN (it2.name.c_str (), (int *)&override_inst.Values [0], (int)it2.type_desc.Columns, 0x0);
            }

            else
            {
              for ( UINT k = 0 ; k < it2.type_desc.Columns ; k++ )
              {
                ImGui::SameLine  ( );
                ImGui::PushID    (k);

                int val = it2.type_desc.Type == D3D_SVT_UINT8 ? (int)((uint8_t *)override_inst.Values) [k] :
                                                                    ((uint32_t *)override_inst.Values) [k];

                if (ImGui::InputIntN (it2.name.c_str (), &val, 1, 0x0))
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
            ImGui::InputFloatN ("##Matrix_Row",
                                 fMatrixPtr, it2.type_desc.Columns, 4, 0x0 );
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

    if (ImGui::IsItemHoveredRect () && (! tracker->used_views.empty ()))
    {
      ImGui::BeginTooltip ();

      DXGI_FORMAT fmt = DXGI_FORMAT_UNKNOWN;

      unique_views.clear ();

      for ( auto it : tracker->used_views )
      {
        D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;

        it->GetDesc (&srv_desc);

        if (srv_desc.ViewDimension == D3D_SRV_DIMENSION_TEXTURE2D)
        {
          CComPtr   <ID3D11Resource>        pRes = nullptr;
                          it->GetResource (&pRes.p);
          CComQIPtr <ID3D11Texture2D> pTex (pRes);

          if (pRes && pTex)
          {
            D3D11_TEXTURE2D_DESC desc = { };
            pTex->GetDesc      (&desc);
                           fmt = desc.Format;

            if (desc.Height > 0 && desc.Width > 0)
            {
              if (! unique_views.emplace (it).second)
                continue;

                                           it->AddRef ();
              temp_resources.emplace_back (it);

              ImGui::Image ( it,         ImVec2  ( std::max (64.0f, (float)desc.Width / 16.0f),
        ((float)desc.Height / (float)desc.Width) * std::max (64.0f, (float)desc.Width / 16.0f) ),
                                         ImVec2  (0,0),             ImVec2  (1,1),
                              desc.Format == DXGI_FORMAT_R8_UNORM ? ImColor (0, 255, 255, 255) :
                                         ImColor (255,255,255,255), ImColor (242,242,13,255) );
            }

            ImGui::SameLine   ();

            ImGui::BeginGroup ();
            ImGui::Text       ("Texture: ");
            ImGui::Text       ("Format:  ");
            ImGui::EndGroup   ();

            ImGui::SameLine   ();

            ImGui::BeginGroup ();
            ImGui::Text       ("%08lx", pTex.p);
            ImGui::Text       ("%ws",   SK_DXGI_FormatToStr (fmt).c_str ());
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
    
    for ( auto&& it : tracker->constants )
    {
      if (it.struct_members.size ())
      {
        ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.9f, 0.1f, 0.7f, 1.0f));
        ImGui::Text           (it.Name);
        ImGui::PopStyleColor  ();
    
        for ( auto&& it2 : it.struct_members )
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

    ImGui::PopFont        ();

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

#include <SpecialK/plugin/plugin_mgr.h>

void
SK_D3D12_EndFrame (SK_TLS* pTLS = SK_TLS_Bottom ())
{
  UNREFERENCED_PARAMETER (pTLS);
  // TODO?
}

#include <SpecialK/render/dxgi/dxgi_swapchain.h>

void
SK_D3D11_EndFrame (SK_TLS* pTLS = SK_TLS_Bottom ())
{
  static SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  if (rb.api == SK_RenderAPI::D3D11On12)
  {
    ID3D11On12Device* pDev =
      rb.d3d11.wrapper_dev;

    if (pDev)
    {
      pDev->ReleaseWrappedResources (nullptr, 0);

      if (rb.d3d11.immediate_ctx)
        rb.d3d11.immediate_ctx->Flush ();
    }

    rb.d3d11.interop.backbuffer_tex2D = nullptr;
    rb.d3d11.interop.backbuffer_rtv   = nullptr;
  }


  static auto& shaders =
    SK_D3D11_Shaders;

  std::lock_guard <SK_Thread_CriticalSection> auto_lock (*cs_render_view);

  dwFrameTime = SK::ControlPanel::current_time;

  static ULONG frames = 0;

  if (frames++ == 0)
  {
    SK_D3D11_LoadShaderState ();

#ifdef _WIN64
    switch (SK_GetCurrentGameID ())
    {
      case SK_GAME_ID::Yakuza0:
        SK_Yakuza0_PlugInInit ();
        break;
    }
#endif
  }


#ifdef TRACK_THREADS
  SK_D3D11_MemoryThreads.clear_active   ();
  SK_D3D11_ShaderThreads.clear_active   ();
  SK_D3D11_DrawThreads.clear_active     ();
  SK_D3D11_DispatchThreads.clear_active ();
#endif



  //for ( auto& it : shaders.reshade_triggered )
  //            it = false;
  shaders.reshade_triggered = false;

  ZeroMemory (reshade_trigger_before.data (), reshade_trigger_before.size () * sizeof (bool));
  ZeroMemory (reshade_trigger_after.data  (), reshade_trigger_after.size  () * sizeof (bool));

  static auto& vertex   = shaders.vertex;
  static auto& pixel    = shaders.pixel;
  static auto& geometry = shaders.geometry;
  static auto& domain   = shaders.domain;
  static auto& hull     = shaders.hull;
  static auto& compute  = shaders.compute;

  vertex.tracked.deactivate   (nullptr);
  pixel.tracked.deactivate    (nullptr);
  geometry.tracked.deactivate (nullptr);
  hull.tracked.deactivate     (nullptr);
  domain.tracked.deactivate   (nullptr);
  compute.tracked.deactivate  (nullptr);
  
  ZeroMemory (vertex.current.views,   sizeof (ID3D11ShaderResourceView*) * (SK_D3D11_MAX_DEV_CONTEXTS + 1) * 128);
  ZeroMemory (pixel.current.views,    sizeof (ID3D11ShaderResourceView*) * (SK_D3D11_MAX_DEV_CONTEXTS + 1) * 128);
  ZeroMemory (geometry.current.views, sizeof (ID3D11ShaderResourceView*) * (SK_D3D11_MAX_DEV_CONTEXTS + 1) * 128);
  ZeroMemory (domain.current.views,   sizeof (ID3D11ShaderResourceView*) * (SK_D3D11_MAX_DEV_CONTEXTS + 1) * 128);
  ZeroMemory (hull.current.views,     sizeof (ID3D11ShaderResourceView*) * (SK_D3D11_MAX_DEV_CONTEXTS + 1) * 128);
  ZeroMemory (compute.current.views,  sizeof (ID3D11ShaderResourceView*) * (SK_D3D11_MAX_DEV_CONTEXTS + 1) * 128);



  tracked_rtv.clear   ();
  used_textures.clear ();

  mem_map_stats.clear ();

  // True if the disjoint query is complete and we can get the results of
  //   each tracked shader's timing
  static bool disjoint_done = false;

  CComQIPtr <ID3D11Device>        pDev    (rb.device);
  CComQIPtr <ID3D11DeviceContext> pDevCtx (rb.d3d11.immediate_ctx);

  if (! (pDevCtx && pDev))
    return;

  disjoint_done = true;

  // End the Query and probe results (when the pipeline has drained)
  if (pDevCtx && (! disjoint_done) && ReadPointerAcquire ((volatile PVOID *)&d3d11_shader_tracking_s::disjoint_query.async))
  {
    if (pDevCtx != nullptr)
    {
      if (ReadAcquire (&d3d11_shader_tracking_s::disjoint_query.active))
      {
        pDevCtx->End ((ID3D11Asynchronous *)ReadPointerAcquire ((volatile PVOID*)&d3d11_shader_tracking_s::disjoint_query.async));
        InterlockedExchange (&d3d11_shader_tracking_s::disjoint_query.active, FALSE);
      }

      else
      {
        HRESULT const hr =
            pDevCtx->GetData ( (ID3D11Asynchronous *)ReadPointerAcquire ((volatile PVOID*)&d3d11_shader_tracking_s::disjoint_query.async),
                                &d3d11_shader_tracking_s::disjoint_query.last_results,
                                  sizeof D3D11_QUERY_DATA_TIMESTAMP_DISJOINT,
                                    0x0 );

        if (hr == S_OK)
        {
          ((ID3D11Asynchronous *)ReadPointerAcquire ((volatile PVOID*)&d3d11_shader_tracking_s::disjoint_query.async))->Release ();
          InterlockedExchangePointer ((void **)&d3d11_shader_tracking_s::disjoint_query.async, nullptr);

          // Check for failure, if so, toss out the results.
          if (! d3d11_shader_tracking_s::disjoint_query.last_results.Disjoint)
            disjoint_done = true;

          else
          {
            auto ClearTimers = [](d3d11_shader_tracking_s* tracker)
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
  }

  if (pDevCtx && disjoint_done)
  {
   const
    auto
     GetTimerDataStart = [](d3d11_shader_tracking_s::duration_s* duration, bool& success) ->
      UINT64
      {
        ID3D11DeviceContext* dev_ctx =
          (ID3D11DeviceContext *)ReadPointerAcquire ((volatile PVOID *)&duration->start.dev_ctx);

        if (            dev_ctx != nullptr &&
             SUCCEEDED (dev_ctx->GetData ( (ID3D11Query *)ReadPointerAcquire ((volatile PVOID *)&duration->start.async), &duration->start.last_results, sizeof UINT64, 0x00 )))
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
     GetTimerDataEnd = [](d3d11_shader_tracking_s::duration_s* duration, bool& success) ->
      UINT64
      {
        if ((ID3D11Query *)ReadPointerAcquire ((volatile PVOID *)&duration->end.async) == nullptr)
          return duration->start.last_results;

        ID3D11DeviceContext* dev_ctx =
          (ID3D11DeviceContext *)ReadPointerAcquire ((volatile PVOID *)&duration->end.dev_ctx);

        if (            dev_ctx != nullptr &&
             SUCCEEDED (dev_ctx->GetData ( (ID3D11Query *)ReadPointerAcquire ((volatile PVOID *)&duration->end.async), &duration->end.last_results, sizeof UINT64, 0x00 )))
        {
          SK_COM_ValidateRelease ((IUnknown **)&duration->end.async);
          SK_COM_ValidateRelease ((IUnknown **)&duration->end.dev_ctx);

          success = true;

          return duration->end.last_results;
        }

        success = false;

        return 0;
      };

    auto CalcRuntimeMS = [](d3d11_shader_tracking_s* tracker)
    {
      if (tracker->runtime_ticks != 0ULL)
      {
        tracker->runtime_ms =
          1000.0 * (((double)tracker->runtime_ticks.load ()) / (double)tracker->disjoint_query.last_results.Frequency);

        // Filter out queries that spanned multiple frames
        //
        if (tracker->runtime_ms > 0.0 && tracker->last_runtime_ms > 0.0)
        {
          if (tracker->runtime_ms > tracker->last_runtime_ms * 100.0 || tracker->runtime_ms > 12.0)
            tracker->runtime_ms = tracker->last_runtime_ms;
        }

        tracker->last_runtime_ms = tracker->runtime_ms;
      }
    };

    const
     auto
      AccumulateRuntimeTicks = [&]( d3d11_shader_tracking_s*       tracker,
                              const std::unordered_set <uint32_t>& blacklist )
      {
        std::vector <d3d11_shader_tracking_s::duration_s> rejects;

        tracker->runtime_ticks = 0ULL;

        for (auto& it : tracker->timers)
        {
          bool   success0 = false, success1 = false;

          const UINT64 time0    = GetTimerDataEnd   (&it, success0),
                       time1    = GetTimerDataStart (&it, success1);

          if (success0 && success1)
            tracker->runtime_ticks += (time0 - time1);
          else
            rejects.emplace_back (it);
        }


        if (tracker->cancel_draws || tracker->num_draws == 0 || blacklist.count (tracker->crc32c))
        {
          tracker->runtime_ticks   = 0ULL;
          tracker->runtime_ms      = 0.0;
          tracker->last_runtime_ms = 0.0;
        }

        tracker->timers.clear ();

        // Anything that fails goes back on the list and we will try again next frame
        tracker->timers = rejects;
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


  for (auto& it_ctx : SK_D3D11_PerCtxResources )
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

    for ( auto& it_res : it_ctx.temp_resources )
    {
      it_res->Release ();
    }

    it_ctx.temp_resources.clear ();
    it_ctx.used_textures.clear  ();

    InterlockedExchange (&it_ctx.writing_, 0);
  }


  for (auto it : temp_resources)
    it->Release ();

  temp_resources.clear ();

  for (auto& it : SK_D3D11_RenderTargets)
    it.clear ();


  extern bool SK_D3D11_ShowShaderModDlg (void);

  if (! SK_D3D11_ShowShaderModDlg ())
    SK_D3D11_EnableMMIOTracking = false;

  SK_D3D11_TextureResampler.processFinished (pDev, pDevCtx, pTLS);
}


#define ShaderColorDecl(idx) {                                                  \
  { ImGuiCol_Header,        ImColor::HSV ( ((idx) + 1) / 6.0f, 0.5f,  0.45f) }, \
  { ImGuiCol_HeaderHovered, ImColor::HSV ( ((idx) + 1) / 6.0f, 0.55f, 0.6f ) }, \
  { ImGuiCol_HeaderActive,  ImColor::HSV ( ((idx) + 1) / 6.0f, 0.6f,  0.6f ) } }


bool
SK_D3D11_ShaderModDlg (SK_TLS* pTLS = SK_TLS_Bottom ())
{
  static const SK_RenderBackend_V2& rb =
    SK_GetCurrentRenderBackend ();

  static
  std::unordered_map < sk_shader_class, std::tuple < std::pair <ImGuiCol, ImColor>,
                                                     std::pair <ImGuiCol, ImColor>,
                                                     std::pair <ImGuiCol, ImColor> > >
    SK_D3D11_ShaderColors =
      { { sk_shader_class::Unknown,  ShaderColorDecl (-1) },
        { sk_shader_class::Vertex,   ShaderColorDecl ( 0) },
        { sk_shader_class::Pixel,    ShaderColorDecl ( 1) },
        { sk_shader_class::Geometry, ShaderColorDecl ( 2) },
        { sk_shader_class::Hull,     ShaderColorDecl ( 3) },
        { sk_shader_class::Domain,   ShaderColorDecl ( 4) },
        { sk_shader_class::Compute,  ShaderColorDecl ( 5) } };


  std::lock_guard <SK_Thread_CriticalSection> auto_lock_gp (*cs_shader);
  std::lock_guard <SK_Thread_CriticalSection> auto_lock_vs (*cs_shader_vs);
  std::lock_guard <SK_Thread_CriticalSection> auto_lock_ps (*cs_shader_ps);
  std::lock_guard <SK_Thread_CriticalSection> auto_lock_gs (*cs_shader_gs);
  std::lock_guard <SK_Thread_CriticalSection> auto_lock_ds (*cs_shader_ds);
  std::lock_guard <SK_Thread_CriticalSection> auto_lock_hs (*cs_shader_hs);
  std::lock_guard <SK_Thread_CriticalSection> auto_lock_cs (*cs_shader_cs);


  ImGuiIO& io (ImGui::GetIO ());

  const float font_size           = ImGui::GetFont ()->FontSize * io.FontGlobalScale;
  const float font_size_multiline = font_size + ImGui::GetStyle ().ItemSpacing.y + ImGui::GetStyle ().ItemInnerSpacing.y;

  bool show_dlg = true;

  ImGui::SetNextWindowSize ( ImVec2 ( io.DisplaySize.x * 0.66f, io.DisplaySize.y * 0.42f ), ImGuiSetCond_Appearing);

  ImGui::SetNextWindowSizeConstraints ( /*ImVec2 (768.0f, 384.0f),*/
                                        ImVec2 ( io.DisplaySize.x * 0.16f, io.DisplaySize.y * 0.16f ),
                                        ImVec2 ( io.DisplaySize.x * 0.96f, io.DisplaySize.y * 0.96f ) );

  if ( ImGui::Begin ( SK_FormatString ( "D3D11 Render Mod Toolkit     (%lu/%lu Memory Mgmt Threads,  %lu/%lu Shader Mgmt Threads,  %lu/%lu Draw Threads,  %lu/%lu Compute Threads)###D3D11_RenderDebug",
                                          SK_D3D11_MemoryThreads.count_active         (), SK_D3D11_MemoryThreads.count_all   (),
                                            SK_D3D11_ShaderThreads.count_active       (), SK_D3D11_ShaderThreads.count_all   (),
                                              SK_D3D11_DrawThreads.count_active       (), SK_D3D11_DrawThreads.count_all     (),
                                                SK_D3D11_DispatchThreads.count_active (), SK_D3D11_DispatchThreads.count_all () ).c_str (),
                        &show_dlg,
                          ImGuiWindowFlags_ShowBorders ) )
  {
    SK_D3D11_EnableTracking = true;

    bool can_scroll = ImGui::IsWindowFocused () && ImGui::IsMouseHoveringRect ( ImVec2 (ImGui::GetWindowPos ().x,                             ImGui::GetWindowPos ().y),
                                                                                ImVec2 (ImGui::GetWindowPos ().x + ImGui::GetWindowSize ().x, ImGui::GetWindowPos ().y + ImGui::GetWindowSize ().y) );

    ImGui::PushItemWidth (ImGui::GetWindowWidth () * 0.666f);

    ImGui::Columns (2);

    ImGui::BeginChild ( ImGui::GetID ("Render_Left_Side"), ImVec2 (0,0), false,
                          ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NavFlattened );

    if (ImGui::CollapsingHeader ("Live Shader View", ImGuiTreeNodeFlags_DefaultOpen))
    {
      // This causes the stats below to update
      SK_ImGui_Widgets.d3d11_pipeline->setActive (true);

      ImGui::TreePush ("");

      static auto& shaders =
        SK_D3D11_Shaders;

      auto ShaderClassMenu = [&](sk_shader_class shader_type) ->
        void
        {
          bool        used_last_frame       = false;
          bool        ui_link_activated     = false;
          char        label           [512] = { };
          wchar_t     wszPipelineDesc [512] = { };

          switch (shader_type)
          {
            case sk_shader_class::Vertex:   ui_link_activated = change_sel_vs != 0x00;
                                            used_last_frame   = shaders.vertex.changes_last_frame > 0;
                                            //SK_D3D11_GetVertexPipelineDesc (wszPipelineDesc);
                                            sprintf (label,     "Vertex Shaders\t\t%ws###LiveVertexShaderTree", wszPipelineDesc);
              break;
            case sk_shader_class::Pixel:    ui_link_activated = change_sel_ps != 0x00;
                                            used_last_frame   = shaders.pixel.changes_last_frame > 0;
                                            //SK_D3D11_GetRasterPipelineDesc (wszPipelineDesc);
                                            //lstrcatW                       (wszPipelineDesc, L"\t\t");
                                            //SK_D3D11_GetPixelPipelineDesc  (wszPipelineDesc);
                                            sprintf (label,     "Pixel Shaders\t\t%ws###LivePixelShaderTree", wszPipelineDesc);
              break;
            case sk_shader_class::Geometry: ui_link_activated = change_sel_gs != 0x00;
                                            used_last_frame   = shaders.geometry.changes_last_frame > 0;
                                            //SK_D3D11_GetGeometryPipelineDesc (wszPipelineDesc);
                                            sprintf (label,     "Geometry Shaders\t\t%ws###LiveGeometryShaderTree", wszPipelineDesc);
              break;
            case sk_shader_class::Hull:     ui_link_activated = change_sel_hs != 0x00;
                                            used_last_frame   = shaders.hull.changes_last_frame > 0;
                                            //SK_D3D11_GetTessellationPipelineDesc (wszPipelineDesc);
                                            sprintf (label,     "Hull Shaders\t\t%ws###LiveHullShaderTree", wszPipelineDesc);
              break;
            case sk_shader_class::Domain:   ui_link_activated = change_sel_ds != 0x00;
                                            used_last_frame   = shaders.domain.changes_last_frame > 0;
                                            //SK_D3D11_GetTessellationPipelineDesc (wszPipelineDesc);
                                            sprintf (label,     "Domain Shaders\t\t%ws###LiveDomainShaderTree", wszPipelineDesc);
              break;
            case sk_shader_class::Compute:  ui_link_activated = change_sel_cs != 0x00;
                                            used_last_frame   = shaders.compute.changes_last_frame > 0;
                                            //SK_D3D11_GetComputePipelineDesc (wszPipelineDesc);
                                            sprintf (label,     "Compute Shaders\t\t%ws###LiveComputeShaderTree", wszPipelineDesc);
              break;
            default:
              break;
          }

          if (used_last_frame)
          {
            if (ui_link_activated)
              ImGui::SetNextTreeNodeOpen (true, ImGuiSetCond_Always);

            ImGui::PushStyleColor ( std::get <0> (SK_D3D11_ShaderColors [shader_type]).first,
                                    std::get <0> (SK_D3D11_ShaderColors [shader_type]).second );
            ImGui::PushStyleColor ( std::get <1> (SK_D3D11_ShaderColors [shader_type]).first,
                                    std::get <1> (SK_D3D11_ShaderColors [shader_type]).second );
            ImGui::PushStyleColor ( std::get <2> (SK_D3D11_ShaderColors [shader_type]).first,
                                    std::get <2> (SK_D3D11_ShaderColors [shader_type]).second );

            if (ImGui::CollapsingHeader (label))
            {
              SK_LiveShaderClassView (shader_type, can_scroll);
            }

            ImGui::PopStyleColor (3);
          }
        };

        ImGui::TreePush    ("");
        ImGui::PushFont    (io.Fonts->Fonts [1]); // Fixed-width font
        ImGui::TextColored (ImColor (238, 250, 5), "%ws", SK::DXGI::getPipelineStatsDesc ().c_str ());
        ImGui::PopFont     ();

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

        ImGui::SameLine ();
        ImGui::Checkbox ("Convert typeless resource views", &convert_typeless);

        ImGui::TreePop     ();

        ShaderClassMenu (sk_shader_class::Vertex);
        ShaderClassMenu (sk_shader_class::Pixel);
        ShaderClassMenu (sk_shader_class::Geometry);
        ShaderClassMenu (sk_shader_class::Hull);
        ShaderClassMenu (sk_shader_class::Domain);
        ShaderClassMenu (sk_shader_class::Compute);

        ImGui::TreePop ();
      }

      auto FormatNumber = [](int num) ->
        const char*
        {
          static char szNumber       [16] = { };
          static char szPrettyNumber [32] = { };

          char dot   [2] = ".";
          char comma [2] = ",";

          const NUMBERFMTA fmt = { 0, 0, 3, dot, comma, 0 };

          snprintf (szNumber, 15, "%li", num);

          GetNumberFormatA ( MAKELCID (LOCALE_USER_DEFAULT, SORT_DEFAULT),
                               0x00,
                                 szNumber, &fmt,
                                   szPrettyNumber, 32 );

          return szPrettyNumber;
        };

      if (ImGui::CollapsingHeader ("Draw Call Filters", ImGuiTreeNodeFlags_DefaultOpen))
      {
        ImGui::TreePush ("");

        static auto& shaders =
          SK_D3D11_Shaders;

        static auto& vertex  = shaders.vertex;

        auto tracker =
          &((SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*)&vertex)->tracked;

        static int min_verts_input,
                   max_verts_input;

        ImGui::Text   ("Vertex Shader: %x", tracker->crc32c.load ());

        bool add_min = ImGui::Button ("Add Min Filter"); ImGui::SameLine (); ImGui::InputInt ("Min Verts", &min_verts_input);
        bool add_max = ImGui::Button ("Add Max Filter"); ImGui::SameLine (); ImGui::InputInt ("Max Verts", &max_verts_input);

        ImGui::Separator ();

        if (add_min) _make_blacklist_draw_min_verts (tracker->crc32c, min_verts_input);
        if (add_max) _make_blacklist_draw_max_verts (tracker->crc32c, max_verts_input);

        int idx = 0;
        ImGui::BeginGroup ();
        for (auto& blacklist : SK_D3D11_BlacklistDrawcalls)
        {
          if ( blacklist.second.if_meshes.have.less_than.vertices.first || 
               blacklist.second.if_meshes.have.more_than.vertices.first    )
          {
            ImGui::PushID (idx++);
            if (ImGui::Button ("Remove Filter"))
            {
              blacklist.second.if_meshes.have.less_than.vertices =
                std::make_pair ( false, 0 );
              blacklist.second.if_meshes.have.more_than.vertices =
                std::make_pair ( false, 0 );
            }
            ImGui::PopID  ();
          }
        }
        ImGui::EndGroup   ();
        ImGui::SameLine   ();

        int rule_idx = 0;

        ImGui::BeginGroup ();
        for (auto& blacklist : SK_D3D11_BlacklistDrawcalls)
        {
          if ( blacklist.second.if_meshes.have.less_than.vertices.first || 
               blacklist.second.if_meshes.have.more_than.vertices.first    )
          {
            ImGui::Text ("Rule%lu  ", rule_idx++);
          }
        }
        ImGui::EndGroup   ();
        ImGui::SameLine   ();
        ImGui::BeginGroup ();
        for (auto& blacklist : SK_D3D11_BlacklistDrawcalls)
        {
          if ( blacklist.second.if_meshes.have.less_than.vertices.first || 
               blacklist.second.if_meshes.have.more_than.vertices.first    )
          {
            ImGui::Text ("Vtx Shader: %x  ", blacklist.first);
          }
        }
        ImGui::EndGroup   ();
        ImGui::SameLine   ();
        ImGui::BeginGroup ();
        for (auto& blacklist : SK_D3D11_BlacklistDrawcalls)
        {
          if ( blacklist.second.if_meshes.have.less_than.vertices.first || 
               blacklist.second.if_meshes.have.more_than.vertices.first    )
          {
            if (blacklist.second.if_meshes.have.less_than.vertices.first)
            {
              ImGui::Text ("Min. Verts = %lu", blacklist.second.if_meshes.have.less_than.vertices.second);
            }
          }
        }
        ImGui::EndGroup   ();
        ImGui::SameLine   ();
        ImGui::BeginGroup ();
        for (auto& blacklist : SK_D3D11_BlacklistDrawcalls)
        {
          if ( blacklist.second.if_meshes.have.less_than.vertices.first || 
               blacklist.second.if_meshes.have.more_than.vertices.first    )
          {
            if (blacklist.second.if_meshes.have.more_than.vertices.first)
            {
              ImGui::Text ("Max. Verts = %lu", blacklist.second.if_meshes.have.more_than.vertices.second);
            }
          }
        }
        ImGui::EndGroup  ();
        ImGui::TreePop   ();
      }

      if (ImGui::CollapsingHeader ("Live Memory View", ImGuiTreeNodeFlags_DefaultOpen))
      {
        SK_D3D11_EnableMMIOTracking = true;
        ////std::lock_guard <SK_Thread_CriticalSection> auto_lock (cs_mmio);

        ImGui::BeginChild ( ImGui::GetID ("Render_MemStats_D3D11"), ImVec2 (0, 0), false, ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoNavFocus |  ImGuiWindowFlags_AlwaysAutoResize );

        ImGui::TreePush   (""                      );
        ImGui::BeginGroup (                        );
        ImGui::BeginGroup (                        );
        ImGui::TextColored(ImColor (0.9f, 1.0f, 0.15f, 1.0f), "Mapped Memory"  );
        ImGui::TreePush   (""                      );
        ImGui::Text       ("Read-Only:            ");
        ImGui::Text       ("Write-Only:           ");
        ImGui::Text       ("Read-Write:           ");
        ImGui::Text       ("Write (Discard):      ");
        ImGui::Text       ("Write (No Overwrite): ");
        ImGui::Text       (""               );
        ImGui::TreePop    (                        );
        ImGui::TextColored(ImColor (0.9f, 1.0f, 0.15f, 1.0f), "Resource Types"  );
        ImGui::TreePush   (""               );
        ImGui::Text       ("Unknown:       ");
        ImGui::Text       ("Buffers:       ");
        ImGui::TreePush   (""               );
        ImGui::Text       ("Index:         ");
        ImGui::Text       ("Vertex:        ");
        ImGui::Text       ("Constant:      ");
        ImGui::TreePop    (                 );
        ImGui::Text       ("Textures:      ");
        ImGui::TreePush   (""               );
        ImGui::Text       ("Textures (1D): ");
        ImGui::Text       ("Textures (2D): ");
        ImGui::Text       ("Textures (3D): ");
        ImGui::TreePop    (                 );
        ImGui::Text       (""               );
        ImGui::TreePop    (                 );
        ImGui::TextColored(ImColor (0.9f, 1.0f, 0.15f, 1.0f), "Memory Totals"  );
        ImGui::TreePush   (""               );
        ImGui::Text       ("Bytes Read:    ");
        ImGui::Text       ("Bytes Written: ");
        ImGui::Text       ("Bytes Copied:  ");
        ImGui::TreePop    (                 );
        ImGui::EndGroup   (                 );

        ImGui::SameLine   (                        );

        ImGui::BeginGroup (                        );
        ImGui::Text       (""                      );
        ImGui::Text       ("( %s )", FormatNumber (mem_map_stats.last_frame.map_types [0]));
        ImGui::Text       ("( %s )", FormatNumber (mem_map_stats.last_frame.map_types [1]));
        ImGui::Text       ("( %s )", FormatNumber (mem_map_stats.last_frame.map_types [2]));
        ImGui::Text       ("( %s )", FormatNumber (mem_map_stats.last_frame.map_types [3]));
        ImGui::Text       ("( %s )", FormatNumber (mem_map_stats.last_frame.map_types [4]));
        ImGui::Text       (""                      );
        ImGui::Text       (""                      );
        ImGui::Text       ("( %s )", FormatNumber (mem_map_stats.last_frame.resource_types [0]));
        ImGui::Text       ("( %s )", FormatNumber (mem_map_stats.last_frame.resource_types [1]));
        ImGui::TreePush   (""                      );
        ImGui::Text       ("%s",     FormatNumber ((int)mem_map_stats.last_frame.index_buffers.size    ()));
        ImGui::Text       ("%s",     FormatNumber ((int)mem_map_stats.last_frame.vertex_buffers.size   ()));
        ImGui::Text       ("%s",     FormatNumber ((int)mem_map_stats.last_frame.constant_buffers.size ()));
        ImGui::TreePop    (                        );
        ImGui::Text       ("( %s )", FormatNumber (mem_map_stats.last_frame.resource_types [2] +
                                                   mem_map_stats.last_frame.resource_types [3] +
                                                   mem_map_stats.last_frame.resource_types [4]));
        ImGui::Text       ("( %s )", FormatNumber (mem_map_stats.last_frame.resource_types [2]));
        ImGui::Text       ("( %s )", FormatNumber (mem_map_stats.last_frame.resource_types [3]));
        ImGui::Text       ("( %s )", FormatNumber (mem_map_stats.last_frame.resource_types [4]));
        ImGui::Text       (""                      );
        ImGui::Text       (""                      );

        if ((double)mem_map_stats.last_frame.bytes_read < (0.75f * 1024.0 * 1024.0))
          ImGui::Text     ("( %06.2f KiB )", (double)mem_map_stats.last_frame.bytes_read    / (1024.0));
        else
          ImGui::Text     ("( %06.2f MiB )", (double)mem_map_stats.last_frame.bytes_read    / (1024.0 * 1024.0));

        if ((double)mem_map_stats.last_frame.bytes_written < (0.75f * 1024.0 * 1024.0))
          ImGui::Text     ("( %06.2f KiB )", (double)mem_map_stats.last_frame.bytes_written / (1024.0));
        else
          ImGui::Text     ("( %06.2f MiB )", (double)mem_map_stats.last_frame.bytes_written / (1024.0 * 1024.0));

        if ((double)mem_map_stats.last_frame.bytes_copied < (0.75f * 1024.0 * 1024.0))
          ImGui::Text     ("( %06.2f KiB )", (double)mem_map_stats.last_frame.bytes_copied / (1024.0));
        else
          ImGui::Text     ("( %06.2f MiB )", (double)mem_map_stats.last_frame.bytes_copied / (1024.0 * 1024.0));

        ImGui::EndGroup   (                        );
        
        ImGui::SameLine   (                        );

        ImGui::BeginGroup (                        );
        ImGui::Text       (""                      );
        ImGui::Text       (" / %s", FormatNumber (mem_map_stats.lifetime.map_types [0]));
        ImGui::Text       (" / %s", FormatNumber (mem_map_stats.lifetime.map_types [1]));
        ImGui::Text       (" / %s", FormatNumber (mem_map_stats.lifetime.map_types [2]));
        ImGui::Text       (" / %s", FormatNumber (mem_map_stats.lifetime.map_types [3]));
        ImGui::Text       (" / %s", FormatNumber (mem_map_stats.lifetime.map_types [4]));
        ImGui::Text       (""                      );
        ImGui::Text       (""                      );
        ImGui::Text       (" / %s", FormatNumber (mem_map_stats.lifetime.resource_types [0]));
        ImGui::Text       (" / %s", FormatNumber (mem_map_stats.lifetime.resource_types [1]));
        ImGui::Text       ("");
        ImGui::Text       ("");
        ImGui::Text       ("");
        ImGui::Text       (" / %s", FormatNumber (mem_map_stats.lifetime.resource_types [2] +
                                                  mem_map_stats.lifetime.resource_types [3] +
                                                  mem_map_stats.lifetime.resource_types [4]));
        ImGui::Text       (" / %s", FormatNumber (mem_map_stats.lifetime.resource_types [2]));
        ImGui::Text       (" / %s", FormatNumber (mem_map_stats.lifetime.resource_types [3]));
        ImGui::Text       (" / %s", FormatNumber (mem_map_stats.lifetime.resource_types [4]));
        ImGui::Text       (""                      );
        ImGui::Text       (""                      );

        if ((double)mem_map_stats.lifetime.bytes_read < (0.75f * 1024.0 * 1024.0 * 1024.0))
          ImGui::Text     (" / %06.2f MiB", (double)mem_map_stats.lifetime.bytes_read    / (1024.0 * 1024.0));
        else
          ImGui::Text     (" / %06.2f GiB", (double)mem_map_stats.lifetime.bytes_read    / (1024.0 * 1024.0 * 1024.0));

        if ((double)mem_map_stats.lifetime.bytes_written < (0.75f * 1024.0 * 1024.0 * 1024.0))
          ImGui::Text     (" / %06.2f MiB", (double)mem_map_stats.lifetime.bytes_written / (1024.0 * 1024.0));
        else
          ImGui::Text     (" / %06.2f GiB", (double)mem_map_stats.lifetime.bytes_written / (1024.0 * 1024.0 * 1024.0));

        if ((double)mem_map_stats.lifetime.bytes_copied < (0.75f * 1024.0 * 1024.0 * 1024.0))
          ImGui::Text     (" / %06.2f MiB", (double)mem_map_stats.lifetime.bytes_copied / (1024.0 * 1024.0));
        else
          ImGui::Text     (" / %06.2f GiB", (double)mem_map_stats.lifetime.bytes_copied / (1024.0 * 1024.0 * 1024.0));

        ImGui::EndGroup   (                        );
        ImGui::EndGroup   (                        );
        ImGui::TreePop    (                        );
        ImGui::EndChild   ();
      }

      else
        SK_D3D11_EnableMMIOTracking = false;

      ImGui::EndChild   ();

      ImGui::NextColumn ();

      ImGui::BeginChild ( ImGui::GetID ("Render_Right_Side"), ImVec2 (0, 0), false, 
                            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NavFlattened | ImGuiWindowFlags_NoScrollbar );

      static bool uncollapsed_tex = true;
      static bool uncollapsed_rtv = true;

      float scale = (uncollapsed_tex ? 1.0f * (uncollapsed_rtv ? 0.5f : 1.0f) : -1.0f);

      ImGui::BeginChild     ( ImGui::GetID ("Live_Texture_View_Panel"),
                              ImVec2 ( -1.0f, scale == -1.0f ? font_size_multiline * 1.666f : ( ImGui::GetWindowContentRegionMax ().y - ImGui::GetWindowContentRegionMin ().y ) * scale - (scale == 1.0f ? font_size_multiline * 1.666f : 0.0f) ),
                                true,
                                  ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NavFlattened );

      uncollapsed_tex = 
        ImGui::CollapsingHeader ( "Live Texture View",
                                  config.textures.d3d11.cache ? ImGuiTreeNodeFlags_DefaultOpen :
                                                                0x0 );

      if (! config.textures.d3d11.cache)
      {
        ImGui::SameLine    ();
        ImGui::TextColored (ImColor::HSV (0.15f, 1.0f, 1.0f), "\t(Unavailable because Texture Caching is not enabled!)");
      }

      uncollapsed_tex = uncollapsed_tex && config.textures.d3d11.cache;

      if (uncollapsed_tex)
      {
        static bool warned_invalid_ref_count = false;

        if ((! warned_invalid_ref_count) && ReadAcquire (&__SK_D3D11_TexRefCount_Failures) > 0)
        {
          SK_ImGui_Warning ( L"The game's graphics engine is not correctly tracking texture memory.\n\n"
                             L"\t\t\t\t>> Texture mod support has been partially disabled to prevent memory leaks.\n\n"
                             L"\t\tYou may force support for texture mods by setting AllowUnsafeRefCounting=true" );

          warned_invalid_ref_count = true;
        }

        SK_LiveTextureView (can_scroll, pTLS);
      }

      ImGui::EndChild ();

      scale = (live_rt_view ? (1.0f * (uncollapsed_tex ? 0.5f : 1.0f)) : -1.0f);

      ImGui::BeginChild     ( ImGui::GetID ("Live_RenderTarget_View_Panel"),
                              ImVec2 ( -1.0f, scale == -1.0f ? font_size_multiline * 1.666f : ( ImGui::GetWindowContentRegionMax ().y - ImGui::GetWindowContentRegionMin ().y ) * scale - (scale == 1.0f ? font_size_multiline * 1.666f : 0.0f) ),
                                true,
                                  ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NavFlattened );

      live_rt_view =
          ImGui::CollapsingHeader ("Live RenderTarget View", ImGuiTreeNodeFlags_DefaultOpen);

      if (live_rt_view)
      {
        //SK_AutoCriticalSection auto_cs_rv (&cs_render_view, true);

        //if (auto_cs2.try_result ())
        {
        static float last_ht    = 256.0f;
        static float last_width = 256.0f;

        static std::vector <std::string> list_contents;
        static bool                      list_dirty     = true;
        static uintptr_t                 last_sel_ptr   =    0;
        static size_t                    sel            = std::numeric_limits <size_t>::max ();
        static bool                      first_frame    = true;

        std::set <ID3D11RenderTargetView *> live_textures;

        struct lifetime
        {
          ULONG last_frame;
          ULONG frames_active;
        };

        ULONG frames_drawn =
          SK_GetFramesDrawn ();

        static std::unordered_map <ID3D11RenderTargetView *, lifetime> render_lifetime;
        static std::vector        <ID3D11RenderTargetView *>           render_textures;

        //render_textures.reserve (128);
        //render_textures.clear   ();

        for (auto&& rtl : SK_D3D11_RenderTargets )
        for (auto&& it  : rtl.rt_views           )
        {
          if (it == nullptr)
            continue;

          D3D11_RENDER_TARGET_VIEW_DESC desc;
          it->GetDesc (&desc);

          if (desc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2D)
          {
            CComPtr   <ID3D11Resource>        pRes = nullptr;
                            it->GetResource (&pRes.p);
            CComQIPtr <ID3D11Texture2D> pTex (pRes);

            if (pRes && pTex)
            {
              D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;

              srv_desc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
              srv_desc.Format                    = desc.Format;
              srv_desc.Texture2D.MipLevels       = desc.Texture2D.MipSlice + 1;
              srv_desc.Texture2D.MostDetailedMip = desc.Texture2D.MipSlice;

              CComQIPtr <ID3D11Device> pDev (rb.device);

              if (pDev != nullptr)
              {
                if (! render_lifetime.count (it))
                {
                  render_textures.push_back (it);
                  render_lifetime.emplace   ( std::make_pair (it,
                                                lifetime { frames_drawn, 1 }) );
                }

                else
                {
                  auto& lifetime =
                    render_lifetime [it];
                  
                  lifetime.frames_active++;
                  lifetime.last_frame = frames_drawn;
                }

                live_textures.emplace (it);
              }
            }
          }
        }

        const ULONG      zombie_threshold = 120;
        static ULONG last_zombie_pass     = frames_drawn;

        if (last_zombie_pass < frames_drawn - zombie_threshold / 2)
        {
          bool newly_dead = false;

          const auto time_to_live =
            frames_drawn - zombie_threshold;

          for (auto& it : render_textures)
          {
            if ( render_lifetime.count (it) &&
                       render_lifetime [it].last_frame < time_to_live )
            {
              render_lifetime.erase (it);
              newly_dead = true;
            }
          }

          if (newly_dead)
          {
            render_textures.clear ();

            for (auto& it : render_lifetime)
              render_textures.emplace_back (it.first);
          }

          last_zombie_pass = frames_drawn;
        }


        if (list_dirty)
        {
              sel =  std::numeric_limits <size_t>::max ();
          int idx =  0;
              list_contents.clear ();

          // The underlying list is unsorted for speed, but that's not at all
          //   intuitive to humans, so sort the thing when we have the RT view open.
          std::sort ( render_textures.begin (),
                      render_textures.end   (),
            []( ID3D11RenderTargetView *a,
                ID3D11RenderTargetView *b )
            {
              return (uintptr_t)a <
                     (uintptr_t)b ;
            }
          );

          static char
            szDesc [48] = { };

          for ( auto& it : render_textures )
          {
            sprintf (szDesc, "%07" PRIxPTR "###rtv_%p", (uintptr_t)it, it);

            list_contents.push_back (szDesc);
  
            if ((uintptr_t)it == last_sel_ptr)
            {
              sel = idx;
              //tbf::RenderFix::tracked_rt.tracking_tex = render_textures [sel];
            }
  
            ++idx;
          }
        }
  
        static bool hovered = false;
        static bool focused = false;
  
        if (hovered || focused)
        {
          can_scroll = false;
  
          if (!render_textures.empty ())
          {
            if (! focused)//hovered)
            {
              ImGui::BeginTooltip ();
              ImGui::TextColored  (ImVec4 (0.9f, 0.6f, 0.2f, 1.0f), "You can view the output of individual render passes");
              ImGui::Separator    ();
              ImGui::BulletText   ("Press %ws while the mouse is hovering this list to select the previous output", virtKeyCodeToHumanKeyName [VK_OEM_4].c_str ());
              ImGui::BulletText   ("Press %ws while the mouse is hovering this list to select the next output",     virtKeyCodeToHumanKeyName [VK_OEM_6].c_str ());
              ImGui::EndTooltip   ();
            }

            else
            {
              ImGui::BeginTooltip ();
              ImGui::TextColored  (ImVec4 (0.9f, 0.6f, 0.2f, 1.0f), "You can view the output of individual render passes");
              ImGui::Separator    ();
              ImGui::BulletText   ("Press LB to select the previous output");
              ImGui::BulletText   ("Press RB to select the next output");
              ImGui::EndTooltip   ();
            }
  
            int direction = 0;
  
                 if (io.KeysDown [VK_OEM_4] && io.KeysDownDuration [VK_OEM_4] == 0.0f) { direction--;  io.WantCaptureKeyboard = true; }
            else if (io.KeysDown [VK_OEM_6] && io.KeysDownDuration [VK_OEM_6] == 0.0f) { direction++;  io.WantCaptureKeyboard = true; }
  
            else {
                  if  (io.NavInputs [ImGuiNavInput_PadFocusPrev] && io.NavInputsDownDuration [ImGuiNavInput_PadFocusPrev] == 0.0f) { direction--; }
              else if (io.NavInputs [ImGuiNavInput_PadFocusNext] && io.NavInputsDownDuration [ImGuiNavInput_PadFocusNext] == 0.0f) { direction++; }
            }
  
            int neutral_idx = 0;
  
            for (UINT i = 0; i < render_textures.size (); i++)
            {
              if ((uintptr_t)render_textures [i] >= last_sel_ptr)
              {
                neutral_idx = i;
                break;
              }
            }
  
            sel = neutral_idx + direction;

            if ((SSIZE_T)sel <  0) sel = 0;
  
            if ((ULONG)sel >= (ULONG)render_textures.size ())
            {
              sel = render_textures.size () - 1;
            }

            if ((SSIZE_T)sel <  0) sel = 0;
          }
        }
  
        ImGui::BeginGroup     ();
        ImGui::PushStyleVar   (ImGuiStyleVar_ChildWindowRounding, 0.0f);
        ImGui::PushStyleColor (ImGuiCol_Border, ImVec4 (0.9f, 0.7f, 0.5f, 1.0f));
  
        ImGui::BeginChild ( ImGui::GetID ("RenderTargetViewList"),
                            ImVec2 ( font_size * 7.0f, -1.0f),
                              true, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NavFlattened );
  
        if (! render_textures.empty ())
        {
          ImGui::BeginGroup ();
  
          if (first_frame)
          {
            sel         = 0;
            first_frame = false;
          }
  
          static bool sel_changed  = false;
  
          if ((SSIZE_T)sel >= 0 && sel < (int)render_textures.size ())
          {
            if (last_sel_ptr != (uintptr_t)render_textures [sel])
              sel_changed = true;
          }
  
          for ( UINT line = 0; line < render_textures.size (); line++ )
          {      
            if (line == sel)
            {
              bool selected = true;
              ImGui::Selectable (list_contents [line].c_str (), &selected);
  
              if (sel_changed)
              {
                ImGui::SetScrollHere        (0.5f);
                ImGui::SetKeyboardFocusHere (    );

                sel_changed  = false;
                last_sel_ptr = (uintptr_t)render_textures [sel];
                tracked_rtv.resource =    render_textures [sel];
              }
            }
  
            else
            {
              bool selected = false;
  
              if (ImGui::Selectable (list_contents [line].c_str (), &selected))
              {
                if (selected)
                {
                  sel_changed          = true;
                  sel                  =  line;
                  last_sel_ptr         = (uintptr_t)render_textures [sel];
                  tracked_rtv.resource =            render_textures [sel];
                }
              }
            }
          }
  
          ImGui::EndGroup ();
        }
  
        ImGui::EndChild      ();
        ImGui::PopStyleColor ();
        ImGui::PopStyleVar   ();
        ImGui::EndGroup      ();


        if (ImGui::IsItemHoveredRect ())
        {
          if (ImGui::IsItemHovered ()) hovered = true; else hovered = false;
          if (ImGui::IsItemFocused ()) focused = true; else focused = false;
        }

        else
        {
          hovered = false; focused = false;
        }
 
  
        if (render_textures.size () > (size_t)sel && live_textures.count (render_textures [sel]))
        {
          ID3D11RenderTargetView* rt_view = render_textures [sel];
  
          D3D11_RENDER_TARGET_VIEW_DESC rtv_desc = { };
          rt_view->GetDesc            (&rtv_desc);
  
          if (rtv_desc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2D)
          {
            CComPtr <ID3D11Resource>          pRes = nullptr;
            rt_view->GetResource            (&pRes.p);
            CComQIPtr <ID3D11Texture2D> pTex (pRes);

            if (pRes && pTex)
            {
              D3D11_TEXTURE2D_DESC desc = { };
              pTex->GetDesc      (&desc);

              CComPtr <ID3D11Texture2D> pTex2 (nullptr);
              D3D11_TEXTURE2D_DESC desc2 = desc;

              if ((! (desc.BindFlags & D3D11_BIND_SHADER_RESOURCE))     || desc.Usage == D3D11_USAGE_DYNAMIC ||
                             SK_D3D11_MakeTypedFormat (desc.Format)     != desc.Format                       ||
                             SK_D3D11_MakeTypedFormat (rtv_desc.Format) != rtv_desc.Format                   ||
                                                       rtv_desc.Format  != desc.Format)
              {
                CComQIPtr <ID3D11Device> pDev (rb.device);

                desc2.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
                desc2.Usage      = D3D11_USAGE_DEFAULT;
                desc2.Format     = SK_D3D11_MakeTypedFormat (desc2.Format);
                rtv_desc.Format  = desc2.Format;

                pDev->CreateTexture2D (&desc2, nullptr, &pTex2);

                CComPtr <ID3D11DeviceContext> pDevCtx;
                pDev->GetImmediateContext   (&pDevCtx);

                pDevCtx->ResolveSubresource (pTex2, 0, pTex, 0, desc2.Format);
              }

              ID3D11ShaderResourceView*       pSRV     = nullptr;
              //D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = { };
              //
              //srv_desc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
              //srv_desc.Format                    = SK_D3D11_MakeTypedFormat (rtv_desc.Format);
              //srv_desc.Texture2D.MipLevels       = (UINT)-1;
              //srv_desc.Texture2D.MostDetailedMip =  0;

              CComQIPtr <ID3D11Device> pDev (rb.device);

              if (pDev != nullptr)
              {
                const size_t row0  = std::max (tracked_rtv.ref_vs.size (), tracked_rtv.ref_ps.size ());
                const size_t row1  =           tracked_rtv.ref_gs.size ();
                const size_t row2  = std::max (tracked_rtv.ref_hs.size (), tracked_rtv.ref_ds.size ());
                const size_t row3  =           tracked_rtv.ref_cs.size ();

                const size_t bottom_list = row0 + row1 + row2 + row3;

                const bool success =
                  SUCCEEDED (pDev->CreateShaderResourceView (pTex2 != nullptr ? pTex2 : pTex, nullptr, &pSRV));
  
                const float content_avail_y = ImGui::GetWindowContentRegionMax ().y - ImGui::GetWindowContentRegionMin ().y;
                const float content_avail_x = ImGui::GetWindowContentRegionMax ().x - ImGui::GetWindowContentRegionMin ().x;
                      float effective_width, effective_height;

                if (! success)
                {
                  effective_width  = 0;
                  effective_height = 0;
                }

                else
                {
                  // Some Render Targets are MASSIVE, let's try to keep the damn things on the screen ;)
                  if (bottom_list > 0)
                    effective_height =           std::max (256.0f, content_avail_y - ((float)(bottom_list + 4) * font_size_multiline * 1.125f));
                  else
                    effective_height = std::max (256.0f, std::max (content_avail_y, (float)desc.Height));

                  effective_width    = effective_height  * ((float)desc.Width / (float)desc.Height );

                  if (effective_width > content_avail_x)
                  {
                    effective_width  = std::max (content_avail_x, 256.0f);
                    effective_height = effective_width * ((float)desc.Height / (float)desc.Width);
                  }
                }
  
                ImGui::SameLine ();
 
                ImGui::PushStyleColor  (ImGuiCol_Border, ImVec4 (0.5f, 0.5f, 0.5f, 1.0f));
                ImGui::BeginChild      ( ImGui::GetID ("RenderTargetPreview"),
                                         ImVec2 ( -1.0f, -1.0f ),
                                           true,
                                             ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NavFlattened );
  
                last_width  = content_avail_x;//effective_width;
                last_ht     = content_avail_y;//effective_height + ( font_size_multiline * (bottom_list + 4) * 1.125f );
  
                ImGui::BeginGroup (                  );
                ImGui::Text       ( "Dimensions:   " );
                ImGui::Text       ( "Format:       " );
                ImGui::Text       ( "Usage:        " );
                ImGui::EndGroup   (                  );
  
                ImGui::SameLine   ( );
  
                ImGui::BeginGroup (                                              );
                ImGui::Text       ( "%lux%lu",
                                      desc.Width, desc.Height/*, effective_width, effective_height, 0.9875f * content_avail_y - ((float)(bottom_list + 3) * font_size * 1.125f), content_avail_y*//*,
                                        pTex->d3d9_tex->GetLevelCount ()*/       );
                ImGui::Text       ( "%ws",
                                      SK_DXGI_FormatToStr (desc.Format).c_str () );
                ImGui::Text       ( "%ws",
                                      SK_D3D11_DescribeUsage (desc.Usage) );
                ImGui::EndGroup   ();

                ImGui::SameLine   ();

                ImGui::BeginGroup ();
                ImGui::Text       ( "CPUAccess:    " );
                ImGui::Text       ( "Bind Flags:   " );
                ImGui::Text       ( "Misc. Flags:  " );
                ImGui::EndGroup   (                  );

                ImGui::SameLine   ();

                ImGui::BeginGroup ();
                ImGui::Text       ( "%x",                     desc.CPUAccessFlags );
                ImGui::Text       ( "%ws", SK_D3D11_DescribeBindFlags ((D3D11_BIND_FLAG)         desc.BindFlags).c_str ());
                ImGui::Text       ( "%ws", SK_D3D11_DescribeMiscFlags ((D3D11_RESOURCE_MISC_FLAG)desc.MiscFlags).c_str ());
                ImGui::EndGroup   (                  );

                if (success && pSRV != nullptr)
                {
                  ImGui::Separator  ( );

                  ImGui::PushStyleColor (ImGuiCol_Border, ImVec4 (0.95f, 0.95f, 0.05f, 1.0f));
                  ImGui::BeginChildFrame   (ImGui::GetID ("ShaderResourceView_Frame"),
                                              ImVec2 (effective_width + 8.0f, effective_height + 8.0f),
                                              ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_ShowBorders );

                  temp_resources.push_back (rt_view);
                                            rt_view->AddRef ();
                  temp_resources.push_back (pSRV);
                  ImGui::Image             ( pSRV,
                                               ImVec2 (effective_width, effective_height),
                                                 ImVec2  (0,0),             ImVec2  (1,1),
                                                 ImColor (255,255,255,255), ImColor (255,255,255,128)
                                           );

#if 0
                  if (ImGui::IsItemHovered ())
                  {
                    ImGui::BeginTooltip ();
                    ImGui::BeginGroup   ();
                    ImGui::TextUnformatted ("Mip Levels:   ");
                    if (desc.SampleDesc.Count > 1)
                    {
                      ImGui::TextUnformatted ("Sample Count: ");
                      ImGui::TextUnformatted ("MSAA Quality: ");
                    }
                    ImGui::TextUnformatted ("Usage:        ");
                    ImGui::TextUnformatted ("Bind Flags:   ");
                    ImGui::TextUnformatted ("CPU Access:   ");
                    ImGui::TextUnformatted ("Misc Flags:   ");
                    ImGui::EndGroup     ();

                    ImGui::SameLine     ();

                    ImGui::BeginGroup   ();
                    ImGui::Text ("%u", desc.MipLevels);
                    if (desc.SampleDesc.Count > 1)
                    {
                      ImGui::Text ("%u", desc.SampleDesc.Count);
                      ImGui::Text ("%u", desc.SampleDesc.Quality);
                    }
                    ImGui::Text (      "%ws", SK_D3D11_DescribeUsage (desc.Usage));
                    ImGui::Text ("%u (  %ws)", desc.BindFlags,
                                            SK_D3D11_DescribeBindFlags (
                      (D3D11_BIND_FLAG)desc.BindFlags).c_str ());
                    ImGui::Text ("%x", desc.CPUAccessFlags);
                    ImGui::Text ("%x", desc.MiscFlags);
                    ImGui::EndGroup   ();
                    ImGui::EndTooltip ();
                  }
#endif

                  ImGui::EndChildFrame     (    );
                  ImGui::PopStyleColor     (    );
                }


                // XXX: When you're done being stupid and writing the same code for every type of shader,
                //        go and do some actual design work.
                bool selected = false;
  
                if (bottom_list)
                {
                  ImGui::Separator  ( );

                  ImGui::BeginChild ( ImGui::GetID ("RenderTargetContributors"),
                                    ImVec2 ( -1.0f ,//std::max (font_size * 30.0f, effective_width + 24.0f),
                                             -1.0f ),
                                      true,
                                        ImGuiWindowFlags_AlwaysAutoResize |
                                        ImGuiWindowFlags_NavFlattened );

                  auto _SetupShaderHeaderColors =
                  [&](sk_shader_class type)
                  {
                    ImGui::PushStyleColor ( std::get <0> (SK_D3D11_ShaderColors [type]).first,
                                            std::get <0> (SK_D3D11_ShaderColors [type]).second );
                    ImGui::PushStyleColor ( std::get <1> (SK_D3D11_ShaderColors [type]).first,
                                            std::get <1> (SK_D3D11_ShaderColors [type]).second );
                    ImGui::PushStyleColor ( std::get <2> (SK_D3D11_ShaderColors [type]).first,
                                            std::get <2> (SK_D3D11_ShaderColors [type]).second );
                  };
  
                  if ((! tracked_rtv.ref_vs.empty ()) || (! tracked_rtv.ref_ps.empty ()))
                  {
                    ImGui::Columns (2);
  

                    _SetupShaderHeaderColors (sk_shader_class::Vertex);

                    if (ImGui::CollapsingHeader ("Vertex Shaders##rtv_refs", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                      ImGui::TreePush ("");

                      static auto& shaders =
                        SK_D3D11_Shaders;
  
                      for ( auto it : tracked_rtv.ref_vs )
                      {
                        if (IsSkipped (sk_shader_class::Vertex, it))
                        {
                          ImGui::PushStyleColor (ImGuiCol_Text, SkipColorCycle ());
                        }

                        else if (IsOnTop (sk_shader_class::Vertex, it))
                        {
                         ImGui::PushStyleColor (ImGuiCol_Text, OnTopColorCycle ());
                        }

                        else if (IsWireframe (sk_shader_class::Vertex, it))
                        {
                          ImGui::PushStyleColor (ImGuiCol_Text, WireframeColorCycle ());
                        }

                        else
                        {
                          ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.95f, 0.95f, 0.95f, 1.0f));
                        }

                        const bool disabled =
                          shaders.vertex.blacklist.count (it) != 0;
  
                        if (ImGui::Selectable (SK_FormatString ("%s%08lx##vs", disabled ? "*" : " ", it).c_str (), &selected))
                        {
                          change_sel_vs = it;
                        }

                        ImGui::PopStyleColor ();
  
                        if (SK_ImGui_IsItemRightClicked ())
                        {
                          ImGui::SetNextWindowSize (ImVec2 (-1.0f, -1.0f), ImGuiSetCond_Always);
                          ImGui::OpenPopup         (SK_FormatString ("ShaderSubMenu_VS%08lx", it).c_str ());
                        }
  
                        if (ImGui::BeginPopup (SK_FormatString ("ShaderSubMenu_VS%08lx", it).c_str ()))
                        {
                          ShaderMenu (shaders.vertex.blacklist, shaders.vertex.blacklist_if_texture, shaders.vertex.tracked.used_views, shaders.vertex.tracked.set_of_views, it);
                          ImGui::EndPopup ();
                        }
                      }
  
                      ImGui::TreePop ();
                    }

                    ImGui::PopStyleColor (3);
  
                    ImGui::NextColumn ();
  
                    _SetupShaderHeaderColors (sk_shader_class::Pixel);

                    if (ImGui::CollapsingHeader ("Pixel Shaders##rtv_refs", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                      ImGui::TreePush ("");

                      static auto& shaders =
                        SK_D3D11_Shaders;
  
                      for ( auto it : tracked_rtv.ref_ps )
                      {
                        if (IsSkipped (sk_shader_class::Pixel, it))
                        {
                          ImGui::PushStyleColor (ImGuiCol_Text, SkipColorCycle ());
                        }

                        else if (IsOnTop (sk_shader_class::Pixel, it))
                        {
                         ImGui::PushStyleColor (ImGuiCol_Text, OnTopColorCycle ());
                        }

                        else if (IsWireframe (sk_shader_class::Pixel, it))
                        {
                          ImGui::PushStyleColor (ImGuiCol_Text, WireframeColorCycle ());
                        }

                        else
                        {
                          ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.95f, 0.95f, 0.95f, 1.0f));
                        }

                        const bool disabled =
                          shaders.pixel.blacklist.count (it) != 0;
  
                        if (ImGui::Selectable (SK_FormatString ("%s%08lx##ps", disabled ? "*" : " ", it).c_str (), &selected))
                        {
                          change_sel_ps = it;
                        }

                        ImGui::PopStyleColor ();
  
                        if (SK_ImGui_IsItemRightClicked ())
                        {
                          ImGui::SetNextWindowSize (ImVec2 (-1.0f, -1.0f), ImGuiSetCond_Always);
                          ImGui::OpenPopup         (SK_FormatString ("ShaderSubMenu_PS%08lx", it).c_str ());
                        }
  
                        if (ImGui::BeginPopup (SK_FormatString ("ShaderSubMenu_PS%08lx", it).c_str ()))
                        {
                          ShaderMenu (shaders.pixel.blacklist, shaders.pixel.blacklist_if_texture, shaders.pixel.tracked.used_views, shaders.pixel.tracked.set_of_views, it);
                          ImGui::EndPopup ();
                        }
                      }
  
                      ImGui::TreePop ();
                    }

                    ImGui::PopStyleColor (3);  
                    ImGui::Columns       (1);
                  }
  
                  if (! tracked_rtv.ref_gs.empty ())
                  {
                    ImGui::Separator ( );
  
                    _SetupShaderHeaderColors (sk_shader_class::Geometry);

                    if (ImGui::CollapsingHeader ("Geometry Shaders##rtv_refs", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                      ImGui::TreePush ("");

                      static auto& shaders =
                        SK_D3D11_Shaders;
  
                      for ( auto it : tracked_rtv.ref_gs )
                      {
                        if (IsSkipped (sk_shader_class::Geometry, it))
                        {
                          ImGui::PushStyleColor (ImGuiCol_Text, SkipColorCycle ());
                        }

                        else if (IsOnTop (sk_shader_class::Geometry, it))
                        {
                         ImGui::PushStyleColor (ImGuiCol_Text, OnTopColorCycle ());
                        }

                        else if (IsWireframe (sk_shader_class::Geometry, it))
                        {
                          ImGui::PushStyleColor (ImGuiCol_Text, WireframeColorCycle ());
                        }

                        else
                        {
                          ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.95f, 0.95f, 0.95f, 1.0f));
                        }

                        const bool disabled =
                          shaders.geometry.blacklist.count (it) != 0;
  
                        if (ImGui::Selectable (SK_FormatString ("%s%08lx##gs", disabled ? "*" : " ", it).c_str (), &selected))
                        {
                          change_sel_gs = it;
                        }

                        ImGui::PopStyleColor ();
  
                        if (SK_ImGui_IsItemRightClicked ())
                        {
                          ImGui::SetNextWindowSize (ImVec2 (-1.0f, -1.0f), ImGuiSetCond_Always);
                          ImGui::OpenPopup         (SK_FormatString ("ShaderSubMenu_GS%08lx", it).c_str ());
                        }
  
                        if (ImGui::BeginPopup (SK_FormatString ("ShaderSubMenu_GS%08lx", it).c_str ()))
                        {
                          ShaderMenu (shaders.geometry.blacklist, shaders.geometry.blacklist_if_texture, shaders.geometry.tracked.used_views, shaders.geometry.tracked.set_of_views, it);
                          ImGui::EndPopup ();
                        }
                      }
  
                      ImGui::TreePop ();
                    }

                    ImGui::PopStyleColor (3);
                  }
  
                  if ((! tracked_rtv.ref_hs.empty ()) || (! tracked_rtv.ref_ds.empty ()))
                  {
                    ImGui::Separator ( );
  
                    ImGui::Columns   (2);
  
                    _SetupShaderHeaderColors (sk_shader_class::Hull);

                    if (ImGui::CollapsingHeader ("Hull Shaders##rtv_refs", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                      ImGui::TreePush ("");
  
                      static auto& shaders =
                        SK_D3D11_Shaders;

                      for ( auto it : tracked_rtv.ref_hs )
                      {
                        const bool disabled =
                          shaders.hull.blacklist.count (it) != 0;

                        if (IsSkipped (sk_shader_class::Hull, it))
                        {
                          ImGui::PushStyleColor (ImGuiCol_Text, SkipColorCycle ());
                        }

                        else if (IsOnTop (sk_shader_class::Hull, it))
                        {
                         ImGui::PushStyleColor (ImGuiCol_Text, OnTopColorCycle ());
                        }

                        else if (IsWireframe (sk_shader_class::Hull, it))
                        {
                          ImGui::PushStyleColor (ImGuiCol_Text, WireframeColorCycle ());
                        }

                        else
                        {
                          ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.95f, 0.95f, 0.95f, 1.0f));
                        }
  
                        if (ImGui::Selectable (SK_FormatString ("%s%08lx##hs", disabled ? "*" : " ", it).c_str (), &selected))
                        {
                          change_sel_hs = it;
                        }

                        ImGui::PopStyleColor ();
  
                        if (SK_ImGui_IsItemRightClicked ())
                        {
                          ImGui::SetNextWindowSize (ImVec2 (-1.0f, -1.0f), ImGuiSetCond_Always);
                          ImGui::OpenPopup         (SK_FormatString ("ShaderSubMenu_HS%08lx", it).c_str ());
                        }
  
                        if (ImGui::BeginPopup (SK_FormatString ("ShaderSubMenu_HS%08lx", it).c_str ()))
                        {
                          ShaderMenu (shaders.hull.blacklist, shaders.hull.blacklist_if_texture, shaders.hull.tracked.used_views, shaders.hull.tracked.set_of_views, it);
                          ImGui::EndPopup ();
                        }
                      }
  
                      ImGui::TreePop ();
                    }

                    ImGui::PopStyleColor (3);
  
                    ImGui::NextColumn ();
  
                    _SetupShaderHeaderColors (sk_shader_class::Domain);

                    if (ImGui::CollapsingHeader ("Domain Shaders##rtv_refs", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                      ImGui::TreePush ("");

                      static auto& shaders =
                        SK_D3D11_Shaders;
  
                      for ( auto it : tracked_rtv.ref_ds )
                      {
                        const bool disabled =
                          shaders.domain.blacklist.count (it) != 0;

                        if (IsSkipped (sk_shader_class::Domain, it))
                        {
                          ImGui::PushStyleColor (ImGuiCol_Text, SkipColorCycle ());
                        }

                        else if (IsOnTop (sk_shader_class::Domain, it))
                        {
                          ImGui::PushStyleColor (ImGuiCol_Text, OnTopColorCycle ());
                        }

                        else if (IsWireframe (sk_shader_class::Domain, it))
                        {
                          ImGui::PushStyleColor (ImGuiCol_Text, WireframeColorCycle ());
                        }

                        else
                        {
                          ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.95f, 0.95f, 0.95f, 1.0f));
                        }
  
                        if (ImGui::Selectable (SK_FormatString ("%s%08lx##ds", disabled ? "*" : " ", it).c_str (), &selected))
                        {
                          change_sel_ds = it;
                        }

                        ImGui::PopStyleColor ();
  
                        if (SK_ImGui_IsItemRightClicked ())
                        {
                          ImGui::SetNextWindowSize (ImVec2 (-1.0f, -1.0f), ImGuiSetCond_Always);
                          ImGui::OpenPopup         (SK_FormatString ("ShaderSubMenu_DS%08lx", it).c_str ());
                        }
  
                        if (ImGui::BeginPopup (SK_FormatString ("ShaderSubMenu_DS%08lx", it).c_str ()))
                        {
                          ShaderMenu (shaders.domain.blacklist, shaders.domain.blacklist_if_texture, shaders.domain.tracked.used_views, shaders.domain.tracked.set_of_views, it);
                          ImGui::EndPopup ();
                        }
                      }
  
                      ImGui::TreePop ();
                    }

                    ImGui::PopStyleColor (3); 
                    ImGui::Columns       (1);
                  }
  
                  if (! tracked_rtv.ref_cs.empty ())
                  {
                    ImGui::Separator ( );
  
                    _SetupShaderHeaderColors (sk_shader_class::Compute);

                    if (ImGui::CollapsingHeader ("Compute Shaders##rtv_refs", ImGuiTreeNodeFlags_DefaultOpen))
                    {
                      ImGui::TreePush ("");

                      static auto& shaders =
                        SK_D3D11_Shaders;
  
                      for ( auto it : tracked_rtv.ref_cs )
                      {
                        const bool disabled =
                          (shaders.compute.blacklist.count (it) != 0);

                        if (IsSkipped (sk_shader_class::Compute, it))
                        {
                          ImGui::PushStyleColor (ImGuiCol_Text, SkipColorCycle ());
                        }

                        else
                        {
                          ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.95f, 0.95f, 0.95f, 1.0f));
                        }
  
                        if (ImGui::Selectable (SK_FormatString ("%s%08lx##cs", disabled ? "*" : " ", it).c_str (), &selected))
                        {
                          change_sel_cs = it;
                        }

                        ImGui::PopStyleColor ();
  
                        if (SK_ImGui_IsItemRightClicked ())
                        {
                          ImGui::SetNextWindowSize (ImVec2 (-1.0f, -1.0f), ImGuiSetCond_Always);
                          ImGui::OpenPopup         (SK_FormatString ("ShaderSubMenu_CS%08lx", it).c_str ());
                        }
  
                        if (ImGui::BeginPopup (SK_FormatString ("ShaderSubMenu_CS%08lx", it).c_str ()))
                        {
                          ShaderMenu (shaders.compute.blacklist, shaders.compute.blacklist_if_texture, shaders.compute.tracked.used_views, shaders.compute.tracked.set_of_views, it);
                          ImGui::EndPopup     (                                      );
                        }
                      }
  
                      ImGui::TreePop ( );
                    }

                    ImGui::PopStyleColor (3);
                  }
  
                  ImGui::EndChild    ( );
                }
  
                ImGui::EndChild      ( );
                ImGui::PopStyleColor (1);
              }
            }
          }
        }
        }
      }

      ImGui::EndChild     ( );
      ImGui::EndChild     ( );
      ImGui::Columns      (1);

      ImGui::PopItemWidth ( );
    }

  ImGui::End            ( );

  SK_D3D11_EnableTracking =
         show_dlg;
  return show_dlg;
}






// Not thread-safe, I mean this! Don't let the stupid critical section fool you;
//   if you import this and try to call it, your software will explode.
__declspec (dllexport)
void
__stdcall
SKX_ImGui_RegisterDiscardableResource (IUnknown* pRes)
{
  std::lock_guard <SK_Thread_CriticalSection> auto_lock (*cs_render_view);
  temp_resources.push_back (pRes);
}



//std::array <bool, SK_D3D11_MAX_DEV_CONTEXTS+1A SK_D3D11_KnownShaders::reshade_triggered;







void
SK_D3D11_ResetShaders (void)
{
  static auto& shaders  = SK_D3D11_Shaders;
  static auto& vertex   = shaders.vertex;
  static auto& pixel    = shaders.pixel;
  static auto& geometry = shaders.geometry;
  static auto& domain   = shaders.domain;
  static auto& hull     = shaders.hull;
  static auto& compute  = shaders.compute;

  for (auto& it : vertex.descs)
  {
    if (it.second.pShader->Release () == 0)
    {
      vertex.rev.erase   ((ID3D11VertexShader *)it.second.pShader);
      vertex.descs.erase (it.first);
    }
  }

  for (auto& it : pixel.descs)
  {
    if (it.second.pShader->Release () == 0)
    {
      pixel.rev.erase   ((ID3D11PixelShader *)it.second.pShader);
      pixel.descs.erase (it.first);
    }
  }

  for (auto& it : geometry.descs)
  {
    if (it.second.pShader->Release () == 0)
    {
      geometry.rev.erase   ((ID3D11GeometryShader *)it.second.pShader);
      geometry.descs.erase (it.first);
    }
  }

  for (auto& it : hull.descs)
  {
    if (it.second.pShader->Release () == 0)
    {
      hull.rev.erase   ((ID3D11HullShader *)it.second.pShader);
      hull.descs.erase (it.first);
    }
  }

  for (auto& it : domain.descs)
  {
    if (it.second.pShader->Release () == 0)
    {
      domain.rev.erase   ((ID3D11DomainShader *)it.second.pShader);
      domain.descs.erase (it.first);
    }
  }

  for (auto& it : compute.descs)
  {
    if (it.second.pShader->Release () == 0)
    {
      compute.rev.erase   ((ID3D11ComputeShader *)it.second.pShader);
      compute.descs.erase (it.first);
    }
  }
}


#if 0
class SK_ExecuteReShadeOnReturn
{
public:
   SK_ExecuteReShadeOnReturn (ID3D11DeviceContext* pCtx) : _ctx (pCtx) { };
  ~SK_ExecuteReShadeOnReturn (void)
  {
    auto TriggerReShade_After = [&]
    {
      SK_ScopedBool auto_bool (&SK_TLS_Bottom ()->imgui.drawing);
      SK_TLS_Bottom ()->imgui.drawing = true;

      if (SK_ReShade_PresentCallback.fn && (! SK_D3D11_Shaders.reshade_triggered))
      {
        CComPtr <ID3D11DepthStencilView>  pDSV = nullptr;
        CComPtr <ID3D11DepthStencilState> pDSS = nullptr;
        CComPtr <ID3D11RenderTargetView>  pRTV = nullptr;

        _ctx->OMGetRenderTargets (1, &pRTV, &pDSV);

        if (pRTV != nullptr)
        {
          D3D11_RENDER_TARGET_VIEW_DESC rt_desc = { };
                        pRTV->GetDesc (&rt_desc);

          if (rt_desc.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2D && rt_desc.Texture2D.MipSlice == 0)
          {
            CComPtr <ID3D11Resource> pRes = nullptr;
                 pRTV->GetResource (&pRes);

            CComQIPtr <ID3D11Texture2D> pTex (pRes);

            if (pTex)
            {
              D3D11_TEXTURE2D_DESC tex_desc = { };

              if ( ImGui::GetIO ().DisplaySize.x == tex_desc.Width &&
                   ImGui::GetIO ().DisplaySize.y == tex_desc.Height )
              {
                for (int i = 0 ; i < 5; i++)
                {
                  SK_D3D11_KnownShaders::ShaderRegistry <IUnknown *>* pShaderReg;

                  switch (i)
                  {
                    default:
                    case 0:
                      pShaderReg = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown *> *)&SK_D3D11_Shaders.vertex;
                      break;
                    case 1:
                      pShaderReg = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown *> *)&SK_D3D11_Shaders.pixel;
                      break;
                    case 2:
                      pShaderReg = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown *> *)&SK_D3D11_Shaders.geometry;
                      break;
                    case 3:
                      pShaderReg = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown *> *)&SK_D3D11_Shaders.hull;
                      break;
                    case 4:
                      pShaderReg = (SK_D3D11_KnownShaders::ShaderRegistry <IUnknown *> *)&SK_D3D11_Shaders.domain;
                      break;
                  };

                  if ( pShaderReg->current.shader [_ctx] != 0x0    &&
                    (! pShaderReg->trigger_reshade.after.empty ()) &&
                       pShaderReg->trigger_reshade.after.count (pShaderReg->current.shader [_ctx]) )
                  {
                    SK_D3D11_Shaders.reshade_triggered = true;

                    SK_ReShade_PresentCallback.explicit_draw.calls++;
                    SK_ReShade_PresentCallback.fn (&SK_ReShade_PresentCallback.explicit_draw);
                    break;
                  }
                }
              }
            }
          }
        }
      }
    };

    TriggerReShade_After ();
  }

protected:
  ID3D11DeviceContext* _ctx;
};
#endif




__forceinline
const std::unordered_map <std::wstring, SK_D3D11_KnownShaders::ShaderRegistry <IUnknown>*>&
__SK_Singleton_D3D11_ShaderClassMap (void)
{
  static auto& shaders  = SK_D3D11_Shaders;
  static auto& vertex   = shaders.vertex;
  static auto& pixel    = shaders.pixel;
  static auto& geometry = shaders.geometry;
  static auto& domain   = shaders.domain;
  static auto& hull     = shaders.hull;
  static auto& compute  = shaders.compute;

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

  return SK_D3D11_ShaderClassMap_;
}

__forceinline
std::unordered_set <std::wstring>&
__SK_Singleton_D3D11_loaded_configs (void)
{
  static std::unordered_set <std::wstring> loaded_configs_;
  return                                   loaded_configs_;
}

#define loaded_configs          __SK_Singleton_D3D11_loaded_configs()
#define SK_D3D11_ShaderClassMap __SK_Singleton_D3D11_ShaderClassMap()



struct SK_D3D11_CommandBase
{
  struct ShaderMods
  {
    class Load : public SK_ICommand
    {
    public:
      SK_ICommandResult execute (const char* szArgs)  override 
      {
        if (strlen (szArgs) > 0)
        {
          std::wstring args =
            SK_UTF8ToWideChar (szArgs);

          SK_D3D11_LoadShaderStateEx (args, false);

          if (loaded_configs.count (args))
            loaded_configs.emplace (args);
        }

        else
          SK_D3D11_LoadShaderState (true);

        return SK_ICommandResult ("D3D11.ShaderMods.Load", szArgs, "done", 1, nullptr, this);
      }

      const char* getHelp       (void)                override
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

        if (loaded_configs.count (args))
          loaded_configs.erase (args);

        return SK_ICommandResult ("D3D11.ShaderMods.Unload", szArgs, "done", 1, nullptr, this);
      }

      const char* getHelp       (void)                override
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

        if (loaded_configs.count (args))
        {
          loaded_configs.erase (args);
          SK_D3D11_UnloadShaderState (args);
        }
        else
        {
          loaded_configs.emplace (args);
          SK_D3D11_LoadShaderStateEx (args, false);
        }

        return SK_ICommandResult ("D3D11.ShaderMods.ToggleConfig", szArgs, "done", 1, nullptr, this);
      }

      const char* getHelp       (void)                override
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

      const char* getHelp       (void)                override
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

      const char* getHelp       (void)                override
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

        while (wszTok)
        {
          switch (arg++)
          {
            case 0:
            {
              if (! SK_D3D11_ShaderClassMap.count (wszTok))
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
                if (! _wcsicmp (wszTok, L"Wireframe"))
                  shader_registry->wireframe.emplace (shader_hash);
                else if (! _wcsicmp (wszTok, L"OnTop"))
                  shader_registry->on_top.emplace (shader_hash);
                else if (! _wcsicmp (wszTok, L"Disable"))
                  shader_registry->blacklist.emplace (shader_hash);
                else if (! _wcsicmp (wszTok, L"TriggerReShade"))
                  shader_registry->trigger_reshade.before.emplace (shader_hash);
                else
                  return SK_ICommandResult ("D3D11.ShaderMod.Set", szArgs, "Invalid Shader State", 0, nullptr, this);
              }
            } break;
          }

          wszTok =
            std::wcstok (nullptr, L" ", &wszBuf);
        }

        return SK_ICommandResult ("D3D11.ShaderMods.Set", szArgs, "done", 1, nullptr, this);
      }

      const char* getHelp       (void)                override
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

        while (wszTok)
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
                if (! _wcsicmp (wszTok, L"Wireframe"))
                  shader_registry->wireframe.erase (shader_hash);
                else if (! _wcsicmp (wszTok, L"OnTop"))
                  shader_registry->on_top.erase (shader_hash);
                else if (! _wcsicmp (wszTok, L"Disable"))
                  shader_registry->blacklist.erase (shader_hash);
                else if (! _wcsicmp (wszTok, L"TriggerReShade"))
                  shader_registry->trigger_reshade.before.erase (shader_hash);
                else
                  return SK_ICommandResult ("D3D11.ShaderMod.Unset", szArgs, "Invalid Shader State", 0, nullptr, this);
              }
            } break;
          }

          wszTok =
            std::wcstok (nullptr, L" ", &wszBuf);
        }

        return SK_ICommandResult ("D3D11.ShaderMods.Unset", szArgs, "done", 1, nullptr, this);
      }

      const char* getHelp       (void)                override
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

        while (wszTok)
        {
          switch (arg++)
          {
            case 0:
            {
              if (! SK_D3D11_ShaderClassMap.count (wszTok))
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
                if (! _wcsicmp (wszTok, L"Wireframe"))
                {
                  if (! shader_registry->wireframe.count   (shader_hash))
                        shader_registry->wireframe.emplace (shader_hash);
                  else
                        shader_registry->wireframe.erase   (shader_hash);
                }
                else if (! _wcsicmp (wszTok, L"OnTop"))
                {
                  if (! shader_registry->on_top.count   (shader_hash))
                        shader_registry->on_top.emplace (shader_hash);
                  else
                        shader_registry->on_top.erase   (shader_hash);
                }
                else if (! _wcsicmp (wszTok, L"Disable"))
                {
                  if (! shader_registry->blacklist.count   (shader_hash))
                        shader_registry->blacklist.emplace (shader_hash);
                  else
                        shader_registry->blacklist.erase   (shader_hash);
                }
                else if (! _wcsicmp (wszTok, L"TriggerReShade"))
                {
                  if (! shader_registry->trigger_reshade.before.count   (shader_hash))
                        shader_registry->trigger_reshade.before.emplace (shader_hash);
                  else
                        shader_registry->trigger_reshade.before.erase   (shader_hash);
                }
                else
                  return SK_ICommandResult ("D3D11.ShaderMod.Toggle", szArgs, "Invalid Shader State", 1, nullptr, this);
              }
            } break;
          }

          wszTok =
            std::wcstok (nullptr, L" ", &wszBuf);
        }

        return SK_ICommandResult ("D3D11.ShaderMods.Toggle", szArgs, "done", 1, nullptr, this);
      }

      const char* getHelp       (void)                override
      {
        return "(Vertex,VS)|(Pixel,PS)|(Geometry,GS)|(Hull,HS)|(Domain,DS)|(Compute,CS)  <Hash>  {Disable|Wireframe|OnTop|TriggerReShade}";
      }
    };
  };

  SK_D3D11_CommandBase (void)
  {
    SK_GetCommandProcessor ()->AddCommand ("D3D11.ShaderMods.Load",         new ShaderMods::Load         ());
    SK_GetCommandProcessor ()->AddCommand ("D3D11.ShaderMods.Unload",       new ShaderMods::Unload       ());
    SK_GetCommandProcessor ()->AddCommand ("D3D11.ShaderMods.ToggleConfig", new ShaderMods::ToggleConfig ());
    SK_GetCommandProcessor ()->AddCommand ("D3D11.ShaderMods.Store",        new ShaderMods::Store        ());
    SK_GetCommandProcessor ()->AddCommand ("D3D11.ShaderMods.Clear",        new ShaderMods::Clear        ());
    SK_GetCommandProcessor ()->AddCommand ("D3D11.ShaderMods.Set",          new ShaderMods::Set          ());
    SK_GetCommandProcessor ()->AddCommand ("D3D11.ShaderMods.Unset",        new ShaderMods::Unset        ());
    SK_GetCommandProcessor ()->AddCommand ("D3D11.ShaderMods.Toggle",       new ShaderMods::Toggle       ());
  }
};


SK_D3D11_CommandBase&
SK_D3D11_GetCommands (void)
{
  static SK_D3D11_CommandBase _commands;
  return                      _commands;
};

#define SK_D3D11_Commands SK_D3D11_GetCommands ()


#include <SpecialK/render/d3d11/d3d11_3.h>

//SK_ICommand
//{
//  virtual SK_ICommandResult execute (const char* szArgs) = 0;
//
//  virtual const char* getHelp            (void) { return "No Help Available"; }
//
//  virtual int         getNumArgs         (void) { return 0; }
//  virtual int         getNumOptionalArgs (void) { return 0; }
//  virtual int         getNumRequiredArgs (void) {
//    return getNumArgs () - getNumOptionalArgs ();
//  }
//};

#include <tuple>
#include <concurrent_vector.h>

#include <../depends/include/glm/glm.hpp>

class SK_D3D11_Screenshot
{
public:
  explicit SK_D3D11_Screenshot (const SK_D3D11_Screenshot& rkMove)
  {
    pDev                     = rkMove.pDev;
    pImmediateCtx            = rkMove.pImmediateCtx;
   
    pSwapChain               = rkMove.pSwapChain;   
    pBackbufferSurface       = rkMove.pBackbufferSurface;
    pStagingBackbufferCopy   = rkMove.pStagingBackbufferCopy;

    pPixelBufferFence        = rkMove.pPixelBufferFence;
    ulCommandIssuedOnFrame   = rkMove.ulCommandIssuedOnFrame;


    framebuffer.Width        = rkMove.framebuffer.Width;
    framebuffer.Height       = rkMove.framebuffer.Height;
    framebuffer.NativeFormat = rkMove.framebuffer.NativeFormat;

    framebuffer.PixelBuffer.Attach (rkMove.framebuffer.PixelBuffer.m_pData);
  }

  explicit SK_D3D11_Screenshot (const CComQIPtr <ID3D11Device>& pDevice)
  {
    pDev = pDevice;

    if (pDev.p != nullptr)
    {
      static const SK_RenderBackend& rb =
        SK_GetCurrentRenderBackend ();

      if (! ( pDev.IsEqualObject (rb.device) &&
          rb.d3d11.immediate_ctx != nullptr )
          )
      {
        pDev->GetImmediateContext (&pImmediateCtx);
      }
      else
        pImmediateCtx = rb.d3d11.immediate_ctx;

      CComQIPtr <ID3D11DeviceContext3>
           pImmediateCtx3 (pImmediateCtx);
      if ( pImmediateCtx3 != nullptr &&
           pImmediateCtx3 != pImmediateCtx )
      {
        pImmediateCtx = pImmediateCtx3;
      }

      D3D11_QUERY_DESC fence_query_desc =
      {
        D3D11_QUERY_EVENT,
        0x00
      };

      if ( SUCCEEDED ( pDev->CreateQuery ( &fence_query_desc,
                                           &pPixelBufferFence
                                         )
                     )
         )
      {
        if ( SUCCEEDED ( rb.swapchain.QueryInterface (
                           &pSwapChain               )
                       )
          )
        {
          ulCommandIssuedOnFrame = SK_GetFramesDrawn ();

          if ( SUCCEEDED ( pSwapChain->GetBuffer ( 0,
                             __uuidof (ID3D11Texture2D),
                               (void **)&pBackbufferSurface.p
                                                 )
                         )
             )
          {
            D3D11_TEXTURE2D_DESC          backbuffer_desc = { };
            pBackbufferSurface->GetDesc (&backbuffer_desc);

            framebuffer.Width        = backbuffer_desc.Width;
            framebuffer.Height       = backbuffer_desc.Height;
            framebuffer.NativeFormat = backbuffer_desc.Format;

            D3D11_TEXTURE2D_DESC staging_desc =
            {
              framebuffer.Width, framebuffer.Height,
              1,                 1,

              framebuffer.NativeFormat,

              { 1, 0 },//D3D11_STANDARD_MULTISAMPLE_PATTERN }, 

                D3D11_USAGE_STAGING,     0x00,
              ( D3D11_CPU_ACCESS_READ ), 0x00
            };

            if ( SUCCEEDED (
              pDev->CreateTexture2D (
                &staging_desc, nullptr,
                  &pStagingBackbufferCopy )
                           )
               )
            {
              if (backbuffer_desc.SampleDesc.Count == 1)
              {
                pImmediateCtx->CopyResource ( pStagingBackbufferCopy,
                                              pBackbufferSurface );
              }

              else
              {
                pImmediateCtx->ResolveSubresource ( pStagingBackbufferCopy, 0,
                                                    pBackbufferSurface,     0,
                                                    framebuffer.NativeFormat );
              }

              if (pImmediateCtx3 != nullptr)
              {
                pImmediateCtx3->Flush1 (D3D11_CONTEXT_TYPE_COPY, nullptr);
              }

              pImmediateCtx->End          (pPixelBufferFence);

              return;
            }
          }
        }
      }
    }


    extern void SK_Steam_CatastropicScreenshotFail (void);
                SK_Steam_CatastropicScreenshotFail ();

    // Something went wrong, crap!
    dispose ();
  }

  ~SK_D3D11_Screenshot (void) { dispose (); }


  bool isValid (void) { return pPixelBufferFence != nullptr; }
  bool isReady (void)
  {
    if (! isValid ())
      return false;

    if (ulCommandIssuedOnFrame > (SK_GetFramesDrawn () - 1))
      return false;

    if (pPixelBufferFence != nullptr)
    {
      return ( S_FALSE != pImmediateCtx->GetData ( pPixelBufferFence, nullptr,
                                                   0,                 0x0 ) );
    }

    return false;
  }

  void dispose (void)
  {
    //if (pPixelBufferFence      != nullptr) pPixelBufferFence->Release      ();
    //if (pStagingBackbufferCopy != nullptr) pStagingBackbufferCopy->Release ();
    //if (pBackbufferSurface     != nullptr) pBackbufferSurface->Release     ();
    //if (pSwapChain             != nullptr) pSwapChain->Release             ();

  //if (pImmediateCtx          != nullptr) pStagingBackbufferCopy->Release ();
  //...
  //...

    pPixelBufferFence      = nullptr;
    pStagingBackbufferCopy = nullptr;

    pImmediateCtx          = nullptr;
    pSwapChain             = nullptr;
    pDev                   = nullptr;

    pBackbufferSurface     = nullptr;

    if (! bRecycled)
    {
      LONG _recycled_size =
        ReadAcquire (&recycled_size);

      bool recycled = false;

      // Recycle up to 128 MiB of data
      if ( framebuffer.PixelBuffer.m_pData != nullptr &&
           _recycled_size                  < ( 128UL * 1024UL * 1024UL ) )
      {
        InterlockedIncrement (&recycled_records);
        InterlockedAdd       (&recycled_size, framebuffer.PBufferSize);

        if ( ( _recycled_size + framebuffer.PBufferSize ) ==
                            ReadAcquire (&recycled_size)     )
        {
          recycled = true;

          recycled_buffers.push (
            std::make_pair ( framebuffer.PBufferSize,
                             framebuffer.PixelBuffer.Detach () )
          );
        }

        else
        {
          InterlockedDecrement (&recycled_records);
          InterlockedAdd       (&recycled_size, -framebuffer.PBufferSize);
        }
      }

      if (! recycled)
      {
        framebuffer.PBufferSize = 0;
        framebuffer.PixelBuffer.Free ();
      }
    }
  };


  bool getData ( UINT     *pWidth,
                 UINT     *pHeight,
                 uint8_t **ppData,
                 bool      Wait = false )
  {
    auto ReadBack = [&](void) -> bool
    {
      const size_t BitsPerPel =
        DirectX::BitsPerPixel (framebuffer.NativeFormat);
      const UINT   Subresource =
        D3D11CalcSubresource ( 0, 0, 1 );

      D3D11_MAPPED_SUBRESOURCE finished_copy = { };

      if ( SUCCEEDED ( pImmediateCtx->Map (
                         pStagingBackbufferCopy, Subresource,
                           D3D11_MAP_READ,       0x0,
                             &finished_copy )
                     )
         )
      {
        size_t PackedDstPitch,
               PackedDstSlicePitch;

        DirectX::ComputePitch (
          framebuffer.NativeFormat,
            framebuffer.Width, framebuffer.Height,
              PackedDstPitch, PackedDstSlicePitch
         );



        std::pair <LONG, uint8_t *>
          reuse = {  0L, nullptr  };

        while (! recycled_buffers.empty ())
        {
          if (recycled_buffers.try_pop (reuse))
            break;
         }

        LONG neededSize =
          (framebuffer.Height + 1)
                              * 
          static_cast <LONG> ( PackedDstPitch
                             );

        if (reuse.first >= neededSize)
        {
          //dll_log.Log ( L"Using %lli recycled bytes { %lli Total / %i }", reuse.first,
          //                ReadAcquire64 (&recycled_size), ReadAcquire (&recycled_records)
          //            );

          framebuffer.PixelBuffer.Attach (
            reuse.second
          );
          framebuffer.PBufferSize = reuse.first;
          bRecycled               = TRUE;
        }

        else
        {
          bRecycled = FALSE;

          if (reuse.second != nullptr)
          {
            InterlockedDecrement (&recycled_records);
            InterlockedAdd       (&recycled_size, -reuse.first);

            //dll_log.Log ( L"Releasing %lli recycled bytes { %lli Total / %i }", reuse.first,
            //             ReadAcquire64 (&recycled_size), ReadAcquire (&recycled_records)
            //);

            free (reuse.second);
          }

          if (framebuffer.PixelBuffer.AllocateBytes (static_cast <size_t> (neededSize)))
              framebuffer.PBufferSize              = neededSize;
        }

        
        SK_ReleaseAssert (framebuffer.PixelBuffer.m_pData != nullptr)


        if ( framebuffer.PixelBuffer.m_pData != nullptr )
        {
          static const glm::mat3x3 from709to2020 (
            glm::vec3 ( 0.6274040f, 0.3292820f, 0.0433136f ),
            glm::vec3 ( 0.0690970f, 0.9195400f, 0.0113612f ),
            glm::vec3 ( 0.0163916f, 0.0880132f, 0.8955950f )
          );

          static const glm::mat3x3 from2020to709 =
            from709to2020._inverse ();

          *pWidth  = framebuffer.Width;
          *pHeight = framebuffer.Height;

          uint8_t* pSrc = (uint8_t *)finished_copy.pData;
          uint8_t* pDst = framebuffer.PixelBuffer.m_pData;

          for ( UINT i = 0; i < framebuffer.Height; ++i )
          {
            memcpy ( pDst, pSrc, finished_copy.RowPitch );
            
            // Eliminate pre-multiplied alpha problems (the stupid way)
            switch (framebuffer.NativeFormat)
            {
              case DXGI_FORMAT_B8G8R8A8_UNORM:
              case DXGI_FORMAT_R8G8B8A8_UNORM:
              {
                for ( UINT j = 3              ;
                           j < PackedDstPitch ;
                           j += 4 )
                {    pDst [j] = 255UL;        }
              } break;

              case DXGI_FORMAT_R10G10B10A2_UNORM:
              {
                for ( UINT j = 3              ;
                           j < PackedDstPitch ;
                           j += 4 )
                {    pDst [j]  |=  0x3;       }
              } break;

              
              case DXGI_FORMAT_R16G16B16A16_FLOAT:
              {
#if 0
                for ( UINT j  = 0              ;
                           j < PackedDstPitch  ;
                           j += 8 )
                { glm::detail::half r ((glm::detail::half &)(pDst [j])),
                                    g ((glm::detail::half &)(pDst [j+2])),
                                    b ((glm::detail::half &)(pDst [j+4]));

                  float red   = r.toFloat ();
                  float green = g.toFloat ();
                  float blue  = b.toFloat ();

                  red =
                    glm::pow (glm::max (glm::pow (glm::abs (red), 1.0f / 78.84375f) - 0.8359375f, 0.0f) / (18.8515625f - 18.6875f) * glm::pow (glm::abs (red), 1.0f / 78.84375f), 1.0f / 0.1593017578f);
                  green =
                    glm::pow (glm::max (glm::pow (glm::abs (green), 1.0f / 78.84375f) - 0.8359375f, 0.0f) / (18.8515625f - 18.6875f) * glm::pow (glm::abs (green), 1.0f / 78.84375f), 1.0f / 0.1593017578f);
                  blue =
                    glm::pow (glm::max (glm::pow (glm::abs (blue), 1.0f / 78.84375f) - 0.8359375f, 0.0f) / (18.8515625f - 18.6875f) * glm::pow (glm::abs (blue), 1.0f / 78.84375f), 1.0f / 0.1593017578f);

                  glm::vec3 color (red, green, blue);

                  color = from2020to709 * color;

                  red   = color.r;
                  green = color.g;
                  blue  = color.b;

                  (glm::detail::half &)
                    (pDst [j]) = glm::detail::half   (glm::pow (red   * 80.0_Nits, 2.2f));

                  (glm::detail::half &)
                    (pDst [j+2]) = glm::detail::half (glm::pow (green * 80.0_Nits, 2.2f));

                  (glm::detail::half &)
                    (pDst [j+4]) = glm::detail::half (glm::pow (blue  * 80.0_Nits, 2.2f));
                }
#endif

                for ( UINT j  = 6              ;
                           j < PackedDstPitch  ;
                           j += 8 )
                {          (glm::detail::half &)
                    (pDst [j])  =
                     glm::detail::half (1.0f); }
              } break;
            }

            pSrc += finished_copy.RowPitch;
            pDst +=         PackedDstPitch;
          }
        }

        SK_LOG0 ( ( L"Screenshot Readback Complete after %li frames",
                      SK_GetFramesDrawn () - ulCommandIssuedOnFrame ),
                    L"D3D11SShot" );

        pImmediateCtx->Unmap (pStagingBackbufferCopy, Subresource);

        *ppData = framebuffer.PixelBuffer.m_pData;

        return true;
      }

      else
      {
        dispose ();
        return true;
      }

      return false;
    };


    bool ready_to_read = false;


    if (! Wait)
    {
      if (isReady ())
      {
        ready_to_read = true;
      }
    }

    else if (isValid ())
    {
      ready_to_read = true;
    }


    return ( ready_to_read ? ReadBack () :
                             false         );
  }

  DXGI_FORMAT
  getInternalFormat (void)
  {
    return framebuffer.NativeFormat;
  }

  struct framebuffer_s
  {
    UINT               Width        = 0,
                       Height       = 0;
    DXGI_FORMAT        NativeFormat = DXGI_FORMAT_UNKNOWN;

    LONG               PBufferSize =  0L;
    CHeapPtr <uint8_t> PixelBuffer = { };
  };

  framebuffer_s*
  getFinishedData (void)
  {
    if (framebuffer.PixelBuffer.m_pData != nullptr)
      return &framebuffer;

    return nullptr;
  }

  ULONG
  getStartFrame (void) const
  {
    return
      ulCommandIssuedOnFrame;
  }


protected:
  CComPtr <ID3D11Device>        pDev                   = nullptr;
  CComPtr <ID3D11DeviceContext> pImmediateCtx          = nullptr;

  CComPtr <IDXGISwapChain>      pSwapChain             = nullptr;
  CComPtr <ID3D11Texture2D>     pBackbufferSurface     = nullptr;
  CComPtr <ID3D11Texture2D>     pStagingBackbufferCopy = nullptr;

  CComPtr <ID3D11Query>         pPixelBufferFence      = nullptr;
  ULONG                         ulCommandIssuedOnFrame = 0;
  BOOL                          bRecycled              = FALSE;

  framebuffer_s                 framebuffer            = {     };
  
  static concurrency::concurrent_queue <std::pair <LONG, uint8_t *>>
                                                       recycled_buffers;
  static volatile LONG                                 recycled_records;
  static volatile LONG                                 recycled_size;
};

concurrency::concurrent_queue <std::pair <LONG, uint8_t *>> SK_D3D11_Screenshot::recycled_buffers;
volatile LONG                                               SK_D3D11_Screenshot::recycled_records (0);
volatile LONG                                               SK_D3D11_Screenshot::recycled_size    (0);


struct
{
  union
  {
    volatile LONG stages [3];

    struct
    {
      volatile LONG pre_game_hud;

      volatile LONG without_sk_osd;
      volatile LONG with_sk_osd;
    };
  };
} enqueued_screenshots { 0, 0, 0 };

bool
SK_Screenshot_IsCapturing (void)
{
  if ( ReadAcquire (&enqueued_screenshots.stages [0]) > 0 ||
       ReadAcquire (&enqueued_screenshots.stages [1]) > 0 ||
       ReadAcquire (&enqueued_screenshots.stages [2]) > 0   )
  {
    return true;
  }

  return false;
}

concurrency::concurrent_queue <SK_D3D11_Screenshot *> screenshot_queue;
concurrency::concurrent_queue <SK_D3D11_Screenshot *> screenshot_write_queue;


static volatile LONG
  __SK_HUD_YesOrNo = 1L;

bool
SK_Screenshot_IsCapturingHUDless (void)
{
  extern volatile
               LONG __SK_ScreenShot_CapturingHUDless;
  if (ReadAcquire (&__SK_ScreenShot_CapturingHUDless))
  {
    return true;
  }

  if ( ReadAcquire (&enqueued_screenshots.pre_game_hud) ||
       ReadAcquire (&enqueued_screenshots.without_sk_osd) )
  {
    return true;
  }

  return false;
}


bool
SK_D3D11_ShouldSkipHUD (void)
{
  if ( SK_Screenshot_IsCapturingHUDless () ||
       ReadAcquire     (&__SK_HUD_YesOrNo) <= 0 )
    return true;

  return false;
}

LONG
SK_D3D11_ShowGameHUD (void)
{
  InterlockedDecrement (&SK_D3D11_DrawTrackingReqs);

  return
    InterlockedIncrement (&__SK_HUD_YesOrNo);
}

LONG
SK_D3D11_HideGameHUD (void)
{
  InterlockedIncrement (&SK_D3D11_DrawTrackingReqs);

  return
    InterlockedDecrement (&__SK_HUD_YesOrNo);
}

LONG
SK_D3D11_ToggleGameHUD (void)
{
  static volatile LONG last_state =
    (ReadAcquire (&__SK_HUD_YesOrNo) > 0);

  if (last_state)
  {
    SK_D3D11_HideGameHUD ();

    return
      InterlockedDecrement (&last_state);
  }

  else
  {
    SK_D3D11_ShowGameHUD ();

    return
      InterlockedIncrement (&last_state);
  }
}



bool
SK_D3D11_CaptureSteamScreenshot  ( SK::ScreenshotStage when =
                                   SK::ScreenshotStage::EndOfFrame )
{
  static const SK_RenderBackend_V2& rb =
    SK_GetCurrentRenderBackend ();

  if ( (int)rb.api & (int)SK_RenderAPI::D3D11 )
  {
    static const
      std::map <SK::ScreenshotStage, int>
        __stage_map = {
          { SK::ScreenshotStage::BeforeGameHUD, 0 },
          { SK::ScreenshotStage::BeforeOSD,     1 },
          { SK::ScreenshotStage::EndOfFrame,    2 }
        };

    const auto it =
               __stage_map.find (when);
    if ( it != __stage_map.cend (    ) )
    {
      const int stage =
        it->second;

      InterlockedIncrement (
        &enqueued_screenshots.stages [stage]
      );

      return true;
    }
  }

  return false;
}


ScreenshotHandle
WINAPI
SK_SteamAPI_AddScreenshotToLibraryEx ( const char *pchFilename,
                                       const char *pchThumbnailFilename,
                                             int   nWidth,
                                             int   nHeight,
                                             bool  Wait = false );

void
SK_D3D11_WaitOnAllScreenshots (void)
{
}

void
SK_D3D11_ProcessScreenshotQueueEx ( int  stage = 2,
                                    bool wait  = false,
                                    bool purge = false )
{
  SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  const int __MaxStage = 2;

  assert ( stage >= 0 &&
           stage <= ( __MaxStage + 1 ) );

  if ( stage < 0 ||
       stage > ( __MaxStage + 1 ) )
    return;


  if ( stage == ( __MaxStage + 1 ) && purge )
  {
    // Empty all stage queues first, then we
    //   can wait for outstanding screenshots
    //     to finish.
    for ( int implied_stage =  0          ;
              implied_stage <= __MaxStage ;
            ++implied_stage                 )
    {
      InterlockedExchange (
        &enqueued_screenshots.stages [
              implied_stage          ],   0
      );
    }
  }


  else if (stage <= __MaxStage)
  {
    if (ReadAcquire (&enqueued_screenshots.stages [stage]) > 0)
    {
      // Just kill any queued shots, we need to be quick about this.
      if (purge)
        InterlockedExchange      (&enqueued_screenshots.stages [stage], 0);

      else
      {
        if (InterlockedDecrement (&enqueued_screenshots.stages [stage]) >= 0)
        {    // --
          screenshot_queue.push (
            new SK_D3D11_Screenshot (
              reinterpret_cast <ID3D11Device *>(rb.device.p)
            )
          );
        }

        else // ++
          InterlockedIncrement   (&enqueued_screenshots.stages [stage]);
      }
    }
  }


  static
    volatile HANDLE
      hSignalScreenshot = INVALID_HANDLE_VALUE;

  static
    volatile HANDLE
      hSignalAbortStart = INVALID_HANDLE_VALUE;

  static
    volatile HANDLE
    hSignalAbortDone   = INVALID_HANDLE_VALUE;

  static
    volatile HANDLE
    hSignalLosslessGo  = INVALID_HANDLE_VALUE;

  static     HANDLE
    hWriteThread       = INVALID_HANDLE_VALUE;


  if ( INVALID_HANDLE_VALUE ==
         InterlockedCompareExchangePointer ( &hSignalScreenshot, 
                                               CreateEventW ( nullptr, FALSE, TRUE, nullptr),
                                                 INVALID_HANDLE_VALUE ) )
  {
    InterlockedExchangePointer ( (void **)&hSignalAbortStart,
                                   CreateEventW (nullptr, TRUE,  FALSE, nullptr) );

    InterlockedExchangePointer ( (void **)&hSignalAbortDone,
                                   CreateEventW (nullptr, FALSE, FALSE, nullptr) );

    InterlockedExchangePointer ( (void **)&hSignalLosslessGo,
                                   CreateEventW (nullptr, FALSE, FALSE, nullptr) );

    hWriteThread =
    SK_Thread_CreateEx ([](LPVOID) -> DWORD
    {
      SetCurrentThreadDescription (          L"[SK] D3D11 Screenshot Write Thread" );
      SetThreadPriority           ( SK_GetCurrentThread (), THREAD_PRIORITY_NORMAL );

      HANDLE hSignal0 =
        ReadPointerAcquire (&hSignalScreenshot);

      HANDLE hSignal1 =
        ReadPointerAcquire (&hSignalAbortStart);

      HANDLE hSignal2 =
        ReadPointerAcquire (&hSignalAbortDone);

      HANDLE hSignal3 =
        ReadPointerAcquire (&hSignalLosslessGo);

      do
      {
        HANDLE signals [] = {
          hSignal0, // Screenshots are waiting for write
          hSignal1  // Screenshot full-abort requested (i.e. for SwapChain Resize)
        };

        DWORD dwWait =
          WaitForMultipleObjects ( 2, signals, FALSE, INFINITE );

        bool
          purge_and_run =
            ( dwWait == ( WAIT_OBJECT_0 + 1 ) );

        static std::vector <SK_D3D11_Screenshot*> to_write;

        while (! screenshot_write_queue.empty ())
        {
          SK_D3D11_Screenshot*                 pop_off   = nullptr;
          if ( screenshot_write_queue.try_pop (pop_off) &&
                                               pop_off  != nullptr )
          {
            if (purge_and_run)
            {
              delete pop_off;
              continue;
            }

            to_write.emplace_back (pop_off);

            SK_D3D11_Screenshot::framebuffer_s* pFrameData =
              pop_off->getFinishedData ();

            // Why's it on the wait-queue if it's not finished?!
            assert (pFrameData != nullptr);

            if (pFrameData != nullptr)
            {
              using namespace DirectX;

              Image raw_img = { };

              ComputePitch (
                pFrameData->NativeFormat,
                  pFrameData->Width, pFrameData->Height,
                    raw_img.rowPitch, raw_img.slicePitch
              );

              raw_img.format = pop_off->getInternalFormat ();
              raw_img.width  = pFrameData->Width;
              raw_img.height = pFrameData->Height;
              raw_img.pixels = pFrameData->PixelBuffer.m_pData;

              extern void SK_SteamAPI_InitManagers (void);
                          SK_SteamAPI_InitManagers ();

              wchar_t       wszAbsolutePathToScreenshot [ MAX_PATH * 2 + 1 ] = { };
              wcsncpy_s   ( wszAbsolutePathToScreenshot,  MAX_PATH * 2, 
                              screenshot_manager->getExternalScreenshotPath (),
                                _TRUNCATE );

              PathAppendW          (wszAbsolutePathToScreenshot, L"SK_SteamScreenshotImport.jpg");
              SK_CreateDirectories (wszAbsolutePathToScreenshot);

              ScratchImage         un_srgb;
              DirectX::TexMetadata meta;

              meta.width     = raw_img.width;
              meta.height    = raw_img.height;
              meta.depth     = 1;
              meta.format    = raw_img.format;
              meta.dimension = TEX_DIMENSION_TEXTURE2D;
              meta.arraySize = 1;
              meta.mipLevels = 1;
              meta.SetAlphaMode (TEX_ALPHA_MODE_OPAQUE);

              un_srgb.Initialize          (meta);
              un_srgb.InitializeFromImage (raw_img);

              ScratchImage un_scrgb;
              un_scrgb.Initialize         (meta);

              static const XMVECTORF32 c_MaxNitsFor2084 =
                { 10000.0f, 10000.0f, 10000.0f, 1.f };

              static const XMMATRIX c_from2020to709 =
              {
                { 1.6604910f,  -0.1245505f, -0.0181508f, 0.f },
                { -0.5876411f,  1.1328999f, -0.1005789f, 0.f },
                { -0.0728499f, -0.0083494f,  1.1187297f, 0.f },
                { 0.f,          0.f,         0.f,        1.f }
              };

              static const XMMATRIX c_from709to2020 =
              {
                { 0.6274040f, 0.0690970f, 0.0163916f, 0.f },
                { 0.3292820f, 0.9195400f, 0.0880132f, 0.f },
                { 0.0433136f, 0.0113612f, 0.8955950f, 0.f },
                { 0.f,        0.f,        0.f,        1.f }
              };

              static const XMMATRIX c_fromP3to2020 =
              {
                { 0.753845f, 0.0457456f, -0.00121055f, 0.f },
                { 0.198593f, 0.941777f,   0.0176041f,  0.f },
                { 0.047562f, 0.0124772f,  0.983607f,   0.f },
                { 0.f,       0.f,         0.f,         1.f }
              };

              HRESULT hr = S_OK;
#if 0
                TransformImage ( un_srgb.GetImages     (),
                                 un_srgb.GetImageCount (),
                                 un_srgb.GetMetadata   (),
                  [&](XMVECTOR* outPixels, const XMVECTOR* inPixels, size_t width, size_t y)
                {
                  UNREFERENCED_PARAMETER(y);

                  for (size_t j = 0; j < width; ++j)
                  {
                    XMVECTOR value  = inPixels [j];
                    XMVECTOR nvalue = XMVector3Transform (value, c_from709to2020);
                              value = XMVectorSelect     (value, nvalue, g_XMSelect1110);

#if 0
#if 0
                        value.m128_f32 [0] = ( value.m128_f32 [0] < 0.04045f ) ? 
                                               value.m128_f32 [0] / 12.92f     :
                                         pow ((value.m128_f32 [0] + 0.055f) / 1.055f, 2.4f);
                        value.m128_f32 [1] = ( value.m128_f32 [1] < 0.04045f ) ? 
                                               value.m128_f32 [1] / 12.92f     :
                                         pow ((value.m128_f32 [1] + 0.055f) / 1.055f, 2.4f);
                        value.m128_f32 [2] = ( value.m128_f32 [2] < 0.04045f ) ? 
                                               value.m128_f32 [2] / 12.92f     :
                                         pow ((value.m128_f32 [2] + 0.055f) / 1.055f, 2.4f);
#else
                        value.m128_f32 [0] = ( value.m128_f32 [0] < 0.0031308f ) ? 
                                               value.m128_f32 [0] * 12.92f      :
                                 1.055f * pow (value.m128_f32 [0], 1.0f / 2.4f) - 0.055f;
                        value.m128_f32 [1] = ( value.m128_f32 [1] < 0.0031308f ) ? 
                                               value.m128_f32 [1] * 12.92f      :
                                 1.055f * pow (value.m128_f32 [1], 1.0f / 2.4f) - 0.055f;
                        value.m128_f32 [2] = ( value.m128_f32 [2] < 0.0031308f ) ? 
                                               value.m128_f32 [2] * 12.92f      :
                                 1.055f * pow (value.m128_f32 [2], 1.0f / 2.4f) - 0.055f;
#endif
#endif

                    outPixels [j]   =                     value;
                  }
                }, un_scrgb);

                std::swap (un_scrgb, un_srgb);
#endif

                XMVECTOR maxLum = XMVectorZero ();

                hr =
                  EvaluateImage ( un_srgb.GetImages     (),
                                  un_srgb.GetImageCount (),
                                  un_srgb.GetMetadata   (),
                  [&](const XMVECTOR* pixels, size_t width, size_t y)
                  {
                    UNREFERENCED_PARAMETER(y);

                    for (size_t j = 0; j < width; ++j)
                    {
                      static const XMVECTORF32 s_luminance =
                        { 0.3f, 0.59f, 0.11f, 0.f };

                      XMVECTOR v = *pixels++;
                               v = XMVector3Dot (v, s_luminance);

                      maxLum =
                        XMVectorMax (v, maxLum);
                    }
                  });

                  maxLum =
                    XMVectorMultiply (maxLum, maxLum);

                  hr =
                    TransformImage ( un_srgb.GetImages     (),
                                     un_srgb.GetImageCount (),
                                     un_srgb.GetMetadata   (),
                    [&](XMVECTOR* outPixels, const XMVECTOR* inPixels, size_t width, size_t y)
                    {
                      UNREFERENCED_PARAMETER(y);

                      for (size_t j = 0; j < width; ++j)
                      {
                        XMVECTOR value = inPixels [j];
                        XMVECTOR scale =
                          XMVectorDivide (
                            XMVectorAdd (
                              g_XMOne,
                                XMVectorDivide (
                                  value,
                                    maxLum
                                )
                            ),
                            XMVectorAdd ( g_XMOne,
                                            value
                                        )
                          );

                        XMVECTOR nvalue =
                          XMVectorMultiply (value, scale);
                                  value =
                          XMVectorSelect   (value, nvalue, g_XMSelect1110);
                        outPixels [j]   =   value;
                      }
                    }, un_scrgb);

              std::swap (un_srgb, un_scrgb);

              Convert ( *un_srgb.GetImages (),
                          DXGI_FORMAT_R8G8B8A8_UNORM,
                            TEX_FILTER_DITHER_DIFFUSION |
                            TEX_FILTER_SEPARATE_ALPHA   |
                            TEX_FILTER_FORCE_NON_WIC,
                              TEX_THRESHOLD_DEFAULT,
                                un_scrgb );

              if ( SUCCEEDED (
                SaveToWICFile ( *un_scrgb.GetImages (), WIC_FLAGS_IGNORE_SRGB,
                                   GetWICCodec         (WIC_CODEC_JPEG),
                                    wszAbsolutePathToScreenshot )
                             )
                 )
              {
                wchar_t       wszAbsolutePathToThumbnail [ MAX_PATH * 2 + 1 ] = { };
                wcsncpy_s   ( wszAbsolutePathToThumbnail,  MAX_PATH * 2,
                                screenshot_manager->getExternalScreenshotPath (),
                                  _TRUNCATE );

                PathAppendW (wszAbsolutePathToThumbnail, L"SK_SteamThumbnailImport.jpg");

                float aspect = (float)pFrameData->Height /
                               (float)pFrameData->Width;

                ScratchImage thumbnailImage;

                Resize ( *un_scrgb.GetImages (), 200,
                           static_cast <size_t> (200 * aspect),
                            TEX_FILTER_DITHER_DIFFUSION | TEX_FILTER_FORCE_WIC |
                            TEX_FILTER_SEPARATE_ALPHA   | TEX_FILTER_TRIANGLE,
                              thumbnailImage );

                SaveToWICFile ( *thumbnailImage.GetImages (), WIC_FLAGS_DITHER,
                                  GetWICCodec                (WIC_CODEC_JPEG),
                                    wszAbsolutePathToThumbnail );

                std::string ss_path (
                  SK_WideCharToUTF8 (wszAbsolutePathToScreenshot)
                );

                std::string ss_thumb (
                  SK_WideCharToUTF8 (wszAbsolutePathToThumbnail)
                );

                ScreenshotHandle screenshot =
                  SK_SteamAPI_AddScreenshotToLibraryEx (
                    ss_path.c_str    (),
                      ss_thumb.c_str (),
                        pFrameData->Width, pFrameData->Height,
                          true );

                SK_LOG1 ( ( L"Finished Steam Screenshot Import for Handle: '%x' (%li frame latency)", 
                            screenshot, SK_GetFramesDrawn () - pop_off->getStartFrame () ),
                              L"SteamSShot" );

                // Remove the temporary files...
                DeleteFileW (wszAbsolutePathToScreenshot);
                DeleteFileW (wszAbsolutePathToThumbnail);
              }

              if (! config.steam.screenshots.png_compress)
              {
                delete pop_off;
                       pop_off = nullptr;
              }
            }
          }
        }

        if (purge_and_run)
        {
          if (purge_and_run)
          {
            SetThreadPriority ( SK_GetCurrentThread (), THREAD_PRIORITY_NORMAL );
          }

          purge_and_run = false;

          ResetEvent (hSignal1); // Abort no longer needed
          SetEvent   (hSignal2); // Abort is complete
        }

        if (config.steam.screenshots.png_compress)
        {
          int enqueued_lossless = 0;

          while ( ! to_write.empty () )
          {
            auto& it =
              to_write.back ();

            if (it == nullptr)
            {
              to_write.pop_back ();
              continue;
            }

            to_write.pop_back ();

            SK_D3D11_Screenshot::framebuffer_s* fb_copy =
              new SK_D3D11_Screenshot::framebuffer_s ();

            SK_D3D11_Screenshot::framebuffer_s& fb_orig =
              *it->getFinishedData ();

            fb_copy->Height       = fb_orig.Height;
            fb_copy->Width        = fb_orig.Width;
            fb_copy->NativeFormat = fb_orig.NativeFormat;
            fb_copy->PBufferSize  = fb_orig.PBufferSize;

            fb_copy->PixelBuffer.Attach (
               fb_orig.PixelBuffer.Detach ()
            );

            static concurrency::concurrent_queue <SK_D3D11_Screenshot::framebuffer_s *>
              raw_images_;
              raw_images_.push (fb_copy);

            ++enqueued_lossless;

            delete it;

            static volatile HANDLE
              hThread = INVALID_HANDLE_VALUE;

            if (InterlockedCompareExchangePointer (&hThread, 0, INVALID_HANDLE_VALUE) == INVALID_HANDLE_VALUE)
            {                                     SK_Thread_CreateEx ([](LPVOID pUser)->DWORD {
              concurrency::concurrent_queue <SK_D3D11_Screenshot::framebuffer_s *>*
                images_to_write =
                  (concurrency::concurrent_queue <SK_D3D11_Screenshot::framebuffer_s *>*)pUser;

              SetCurrentThreadDescription (           L"[SK] D3D11 Lossless Screenshot Dispatch" );
              SetThreadPriority           ( SK_GetCurrentThread (), THREAD_MODE_BACKGROUND_BEGIN );
              SetThreadPriority           ( SK_GetCurrentThread (), THREAD_PRIORITY_BELOW_NORMAL );

              while (! ReadAcquire (&__SK_DLL_Ending))
              {
                SK_D3D11_Screenshot::framebuffer_s *pFrameData =
                  nullptr;

                WaitForSingleObject  (
                  ReadPointerAcquire (&hSignalLosslessGo),
                    INFINITE
                );

                while   (! images_to_write->empty   (          ))
                { while (! images_to_write->try_pop (pFrameData))
                  {
                    SleepEx ( 15, FALSE );
                    continue;
                  }

                  if (ReadAcquire (&__SK_DLL_Ending))
                    break;

                  time_t screenshot_time;

                  wchar_t       wszAbsolutePathToLossless [ MAX_PATH * 2 + 1 ] = { };
                  wcsncpy_s   ( wszAbsolutePathToLossless,  MAX_PATH * 2, 
                                  screenshot_manager->getExternalScreenshotPath (),
                                    _TRUNCATE );

                  bool hdr =
                    SK_GetCurrentRenderBackend ().isHDRCapable ();

                  if (hdr)
                  {
                    PathAppendW ( wszAbsolutePathToLossless,
                      SK_FormatStringW ( L"HDR\\%lu.jxr",
                                  time (&screenshot_time) ).c_str () );
                  }

                  else
                  {
                    PathAppendW ( wszAbsolutePathToLossless,
                      SK_FormatStringW ( L"Lossless\\%lu.png",
                                  time (&screenshot_time) ).c_str () );
                  }

                  // Why's it on the wait-queue if it's not finished?!
                  assert (pFrameData != nullptr);

                  if (pFrameData != nullptr)
                  {
                    using namespace DirectX;

                    Image raw_img = { };

                    ComputePitch ( pFrameData->NativeFormat,
                                     pFrameData->Width,
                                     pFrameData->Height,
                                       raw_img.rowPitch,
                                       raw_img.slicePitch
                    );

                    raw_img.format = pFrameData->NativeFormat;
                    raw_img.width  = pFrameData->Width;
                    raw_img.height = pFrameData->Height;
                    raw_img.pixels = pFrameData->PixelBuffer.m_pData;

                    SK_CreateDirectories (wszAbsolutePathToLossless);

                    ScratchImage         un_srgb;
                    DirectX::TexMetadata meta;

                    meta.width     = raw_img.width;
                    meta.height    = raw_img.height;
                    meta.depth     = 1;
                    meta.format    = raw_img.format;
                    meta.dimension = TEX_DIMENSION_TEXTURE2D;
                    meta.arraySize = 1;
                    meta.mipLevels = 1;
                    meta.SetAlphaMode (TEX_ALPHA_MODE_OPAQUE);

                    un_srgb.Initialize          (meta);
                    un_srgb.InitializeFromImage (raw_img);

                    if ( SUCCEEDED (
                           SaveToWICFile (*un_srgb.GetImages (), WIC_FLAGS_DITHER,
                                      GetWICCodec (hdr ? WIC_CODEC_WMP :
                                                         WIC_CODEC_PNG),
                                           wszAbsolutePathToLossless,
                                             &GUID_WICPixelFormat64bppRGBAHalf)
                                   )
                       )
                    {
                      // Refresh
                      screenshot_manager->getExternalScreenshotRepository (true);
                    }

                    delete pFrameData;
                  }
                }
              }

              SK_Thread_CloseSelf ();

              return 0;
            }, L"[SK] D3D11 Lossless Screenshot Dispatch",
      (LPVOID)&raw_images_ );
          } }

          if (enqueued_lossless > 0)
          {   enqueued_lossless = 0;
            SetEvent (hSignal3);
          }
        }
      } while (! ReadAcquire (&__SK_DLL_Ending));

      SK_Thread_CloseSelf ();

      CloseHandle (hSignal0);
      CloseHandle (hSignal1);
      CloseHandle (hSignal2);
      CloseHandle (hSignal3);

      return 0;
    });
  }


  if (stage != 3)
    return;


  // Any incomplete captures are pushed onto this queue, and then the pending
  //   queue (once drained) is re-built.
  //
  //  This is faster than iterating a synchronized list in highly multi-threaded engines.
  static concurrency::concurrent_queue <SK_D3D11_Screenshot *> rejected_screenshots;

  bool new_jobs = false;

  do
  {
    while (! screenshot_queue.empty ())
    {
      SK_D3D11_Screenshot*           pop_off   = nullptr;
      if ( screenshot_queue.try_pop (pop_off) &&
                                     pop_off  != nullptr )
      {
        if (purge)
          delete pop_off;

        else
        {
          UINT     Width, Height;
          uint8_t* pData;

          // There is a condition in which waiting is necessary;
          //   after a swapchain resize or fullscreen mode switch.
          //
          //  * For now, ignore this problem until it actuall poses one.
          //
          if (pop_off->getData (&Width, &Height, &pData, wait))
          {
            screenshot_write_queue.push (pop_off);
            new_jobs = true;
          }

          else
            rejected_screenshots.push (pop_off);
        }
      }
    }

    while (! rejected_screenshots.empty ())
    {
      SK_D3D11_Screenshot*               push_back   = nullptr;
      if ( rejected_screenshots.try_pop (push_back) &&
                                         push_back  != nullptr )
      {
        if (purge)
          delete push_back;

        else
          screenshot_queue.push (push_back);
      }
    }

    if ( wait ||
                 purge )
    {
      if ( screenshot_queue.empty     () &&
           rejected_screenshots.empty ()    )
      {
        if ( purge && (! screenshot_write_queue.empty ()) )
        {
          SetThreadPriority   ( hWriteThread,     THREAD_PRIORITY_TIME_CRITICAL );
          SignalObjectAndWait ( hSignalAbortStart,
                                hSignalAbortDone, INFINITE,              FALSE  );
        }

        wait  = false;
        purge = false;

        break;
      }
    }
  } while ( wait ||
                    purge );

  if (new_jobs)
    SetEvent (ReadPointerAcquire (&hSignalScreenshot));
}

void
SK_D3D11_ProcessScreenshotQueue (int stage = 2)
{
  SK_D3D11_ProcessScreenshotQueueEx (stage, false);
}



#include <SpecialK/ini.h>

void
__stdcall
SK_D3D11_PresentFirstFrame (IDXGISwapChain* pSwapChain)
{
  UNREFERENCED_PARAMETER (pSwapChain);

  auto cmds = SK_D3D11_GetCommands ();

  LocalHook_D3D11CreateDevice.active             = true;
  LocalHook_D3D11CreateDeviceAndSwapChain.active = true;

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
          SK_StripUserNameFromPathW (
            std::wstring (
                      it->target.module_path
                         ).data ()
          )                                                             ),
                    L"Hook Cache" );
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
      ///SK_D3D11_InitTextures ();

      quick_hooked = true;
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
  for ( auto& it : local_d3d11_records )
  {
    if (it->active)
    {
      SK_Hook_ResolveTarget (*it);

      // Don't cache addresses that were screwed with by other injectors
      const wchar_t* wszSection =
        StrStrIW (it->target.module_path, LR"(\sys)") ?
                                          L"D3D11.Hooks" : nullptr;

      if (! wszSection)
      {
        SK_LOG0 ( ( L"Hook for '%hs' resides in '%s', will not cache!",
                      it->target.symbol_name,
          SK_StripUserNameFromPathW (
            std::wstring (
                      it->target.module_path
                         ).data ()
          )                                                             ),
                    L"Hook Cache" );
      }
      SK_Hook_CacheTarget ( *it, wszSection );
    }
  }

  auto it_local  = std::begin (local_d3d11_records);
  auto it_global = std::begin (global_d3d11_records);

  while ( it_local != std::end (local_d3d11_records) )
  {
    if (   ( *it_local )->hits &&
 StrStrIW (( *it_local )->target.module_path, LR"(\sys)") &&
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

#include <SpecialK/render/d3d11/d3d11_interfaces.h>
#include <SpecialK/render/d3d11/d3d11_3.h>

#include <atlbase.h>
#include <comdef.h>
#include <assert.h>

extern volatile LONG __SKX_ComputeAntiStall;

extern void
SK_D3D11_ResetContextState (ID3D11DeviceContext* pDevCtx);

extern void
SK_D3D11_MergeCommandLists ( ID3D11DeviceContext *pSurrogate,
                             ID3D11DeviceContext *pMerge );

// THe device context a command list was built using
const GUID SKID_D3D11DeviceContextOrigin =
// {5C5298CA-0F9D-5022-A19D-A2E69792AE03}
  { 0x5c5298ca, 0xf9d,  0x5022, { 0xa1, 0x9d, 0xa2, 0xe6, 0x97, 0x92, 0xae, 0x3 } };


class SK_IWrapD3D11DeviceContext : public ID3D11DeviceContext4
{
public:
  int last_ps_set_resource_count = 0;

  SK_IWrapD3D11DeviceContext (ID3D11DeviceContext* dev_ctx) : pReal (dev_ctx)
  {
    InterlockedExchange (&refs_, dev_ctx->AddRef  ());
                                 dev_ctx->Release ();
    ver_ = 0;

    CComQIPtr <ID3D11DeviceContext1> pDevCtx1 (this);
  };
  SK_IWrapD3D11DeviceContext (ID3D11DeviceContext1* dev_ctx) : pReal (dev_ctx)
  {
    InterlockedExchange (&refs_, dev_ctx->AddRef  ());
                                 dev_ctx->Release ();
    ver_ = 1;

    CComQIPtr <ID3D11DeviceContext2> pDevCtx2 (this);
  };
  SK_IWrapD3D11DeviceContext (ID3D11DeviceContext2* dev_ctx) : pReal (dev_ctx)
  {
    InterlockedExchange (&refs_, dev_ctx->AddRef  ());
                                 dev_ctx->Release ();
    ver_ = 2;

    CComQIPtr <ID3D11DeviceContext3> pDevCtx3 (this);
  };
  SK_IWrapD3D11DeviceContext (ID3D11DeviceContext3* dev_ctx) : pReal (dev_ctx)
  {
    InterlockedExchange (&refs_, dev_ctx->AddRef  ());
                                 dev_ctx->Release ();
    ver_ = 3;

    CComQIPtr <ID3D11DeviceContext4> pDevCtx4 (this);
  };
  SK_IWrapD3D11DeviceContext (ID3D11DeviceContext4* dev_ctx) : pReal (dev_ctx)
  {
    InterlockedExchange (&refs_, dev_ctx->AddRef  ());
                                 dev_ctx->Release ();
    ver_ = 4;
  };

  virtual ~SK_IWrapD3D11DeviceContext (void)
  {
  };

  HRESULT STDMETHODCALLTYPE QueryInterface (REFIID riid,_COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject) override
  {
    if (ppvObject == nullptr)
    {
      return E_POINTER;
    }

    if ( IsEqualGUID (riid, IID_IUnwrappedD3D11DeviceContext) )
    {
    //assert (ppvObject != nullptr);
      *ppvObject = pReal;

      pReal->AddRef ();

      return S_OK;
    }

    else if (
      //riid == __uuidof (this)                 ||
      riid == __uuidof (IUnknown)             ||
      riid == __uuidof (ID3D11DeviceChild)    ||
      riid == __uuidof (ID3D11DeviceContext)  ||
      riid == __uuidof (ID3D11DeviceContext1) ||
      riid == __uuidof (ID3D11DeviceContext2) ||
      riid == __uuidof (ID3D11DeviceContext3) ||
      riid == __uuidof (ID3D11DeviceContext4))
    {
      if (riid == __uuidof(ID3D11DeviceContext1) && ver_ < 1)
      {
        ID3D11DeviceContext1 *devctx1 = nullptr;

        if (FAILED (pReal->QueryInterface (&devctx1)))
        {
          return E_NOINTERFACE;
        }

        pReal->Release ();
        pReal = devctx1;

        ver_ = 1;
      }
      if (riid == __uuidof(ID3D11DeviceContext2) && ver_ < 2)
      {
        ID3D11DeviceContext2 *devctx2 = nullptr;

        if (FAILED (pReal->QueryInterface (&devctx2)))
        {
          return E_NOINTERFACE;
        }

        pReal->Release ();
        pReal = devctx2;

        ver_ = 2;
      }
      if (riid == __uuidof(ID3D11DeviceContext3) && ver_ < 3)
      {
        ID3D11DeviceContext3 *devctx3 = nullptr;

        if (FAILED (pReal->QueryInterface (&devctx3)))
        {
          return E_NOINTERFACE;
        }

        pReal->Release ();
        pReal = devctx3;

        ver_ = 3;
      }
      if (riid == __uuidof(ID3D11DeviceContext4) && ver_ < 4)
      {
        ID3D11DeviceContext4 *devctx4 = nullptr;

        if (FAILED (pReal->QueryInterface (&devctx4)))
        {
          return E_NOINTERFACE;
        }

        pReal->Release ();
        pReal = devctx4;

        ver_ = 4;
      }

      pReal->AddRef ();

      InterlockedExchange (&refs_, pReal->Release ());
      AddRef ();

      *ppvObject = this;

      return S_OK;
    }

    return pReal->QueryInterface (riid, ppvObject);
  };

  ULONG STDMETHODCALLTYPE AddRef  (void) override
  {
    InterlockedIncrement (&refs_);

    return pReal->AddRef ();
  }
  ULONG STDMETHODCALLTYPE Release (void) override
  {
    if (InterlockedDecrement (&refs_) == 0)
    {
      //assert(_runtime != nullptr);
    }

    ULONG refs = pReal->Release ();

    if (ReadAcquire (&refs_) == 0 && refs != 0)
    {
      //SK_LOG0 ( (L"Reference count for 'IDXGISwapChain" << (ver_ > 0 ? std::to_string(ver_) : "") << "' object " << this << " is inconsistent: " << ref << ", but expected 0.";

      refs = 0;
    }

    if (refs == 0)
    {
      assert (ReadAcquire (&refs_) <= 0);

      if (ReadAcquire (&refs_) == 0)
      {
      //delete this;
      }
    }

    return refs;
  }

  void STDMETHODCALLTYPE GetDevice (

    _Out_  ID3D11Device **ppDevice) override
  {
    return pReal->GetDevice (ppDevice);
  }
        
  HRESULT STDMETHODCALLTYPE GetPrivateData ( _In_    REFGUID guid,
                                             _Inout_ UINT   *pDataSize,
               _Out_writes_bytes_opt_( *pDataSize )  void   *pData ) override {
    return pReal->GetPrivateData (guid, pDataSize, pData);
  }
        
  HRESULT STDMETHODCALLTYPE SetPrivateData (
 
    _In_  REFGUID guid,
 
    _In_  UINT DataSize,
 
    _In_reads_bytes_opt_( DataSize )  const void *pData) override
  {
    return pReal->SetPrivateData (guid, DataSize, pData);
  }
        
  HRESULT STDMETHODCALLTYPE SetPrivateDataInterface (

    _In_  REFGUID guid,

    _In_opt_  const IUnknown *pData) override
  {
    return pReal->SetPrivateDataInterface (guid, pData);
  }

  void STDMETHODCALLTYPE VSSetConstantBuffers (
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
    _In_reads_opt_ (NumBuffers)  ID3D11Buffer *const *ppConstantBuffers) override
  {
    return pReal->VSSetConstantBuffers (StartSlot, NumBuffers, ppConstantBuffers);
  }

  void STDMETHODCALLTYPE PSSetShaderResources (
    _In_range_ (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
    _In_range_ (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumViews,
    _In_reads_opt_ (NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews) override
  {
    last_ps_set_resource_count = NumViews;

    SK_TLS* pTLS = nullptr;

    static auto& shaders =
      SK_D3D11_Shaders;

    constexpr int PIXEL_SHADER_STAGE = 1;

    // ImGui gets to pass-through without invoking the hook
    if ( (! SK_D3D11_ShouldTrackSetShaderResources (pReal, &pTLS)) ||
         (! SK_D3D11_ShouldTrackRenderOp           (pReal, &pTLS)) )
      return pReal->PSSetShaderResources (StartSlot, NumViews, ppShaderResourceViews);

    auto&& newResourceViews = pTLS->d3d11.newResourceViews;

    RtlZeroMemory (newResourceViews, sizeof (ID3D11ShaderResourceView*) * NumViews);

    if (ppShaderResourceViews && NumViews > 0)
    {
      const UINT dev_idx =
        SK_D3D11_GetDeviceContextHandle (pReal);

      auto&& views =
        shaders.pixel.current.views [dev_idx];
      auto&& stage =
        d3d11_shader_stages [PIXEL_SHADER_STAGE] [dev_idx];

      d3d11_shader_tracking_s& tracked =
        shaders.pixel.tracked;

      const uint32_t shader_crc32c = tracked.crc32c.load ();
      const bool     active = tracked.active.get (dev_idx);

      if (!ReadAcquire (&__SKX_ComputeAntiStall))
      {
        for (UINT i = 0; i < NumViews; i++)
        {
          if (SK_D3D11_ActivateSRVOnSlot (dev_idx, stage,
              ppShaderResourceViews [i], StartSlot + i))
            newResourceViews [i] = ppShaderResourceViews [i];
          else
            newResourceViews [i] = nullptr;
        }

        if (active && shader_crc32c != 0)
        {
          for (UINT i = 0; i < NumViews; i++)
          {
            if (ppShaderResourceViews [i]) {
              std::lock_guard <SK_Thread_CriticalSection> auto_lock (*cs_shader_ps);
              if (tracked.set_of_views.emplace (ppShaderResourceViews [i]).second) {
                ppShaderResourceViews [i]->AddRef ();
              }
              tracked.used_views.emplace_back (ppShaderResourceViews [i]);
            }
            views [StartSlot + i] = ppShaderResourceViews [i];
          }
        }
      }
      else {
        for (UINT i = StartSlot; i < StartSlot + NumViews; ++i)
        {
          if (SK_D3D11_ActivateSRVOnSlot (dev_idx, stage,
              ppShaderResourceViews [i - StartSlot], i))
            newResourceViews [i - StartSlot] = ppShaderResourceViews [i - StartSlot];
          else
            newResourceViews [i - StartSlot] = nullptr;
        }

        if (active && shader_crc32c != 0)
        {
          for (UINT i = StartSlot; i < StartSlot + NumViews; ++i)
          {
            if (ppShaderResourceViews [i - StartSlot]) {
              std::lock_guard <SK_Thread_CriticalSection> auto_lock (*cs_shader_ps);
              if (tracked.set_of_views.emplace (ppShaderResourceViews [i - StartSlot]).second) {
                ppShaderResourceViews [i - StartSlot]->AddRef ();
              }
              tracked.used_views.emplace_back (ppShaderResourceViews [i - StartSlot]);
            }
            views [i] = ppShaderResourceViews [i - StartSlot];
          }
        }
      }
    }

    return pReal->PSSetShaderResources (StartSlot, NumViews, newResourceViews);
  }
        
  void STDMETHODCALLTYPE PSSetShader (
    _In_opt_  ID3D11PixelShader *pPixelShader,
    _In_reads_opt_ (NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
    UINT NumClassInstances) override
  {
    return
      SK_D3D11_SetShader_Impl ( pReal, pPixelShader,
                                 sk_shader_class::Pixel,
                                   ppClassInstances, NumClassInstances );
  }
        
  void STDMETHODCALLTYPE PSSetSamplers (
    _In_range_ (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,
    _In_range_ (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,
    _In_reads_opt_ (NumSamplers)  ID3D11SamplerState *const *ppSamplers) override
  {
    return pReal->PSSetSamplers (StartSlot, NumSamplers, ppSamplers);
  }
        
  void STDMETHODCALLTYPE VSSetShader (
    _In_opt_  ID3D11VertexShader *pVertexShader,
    _In_reads_opt_ (NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
    UINT NumClassInstances) override
  {
    return
      SK_D3D11_SetShader_Impl ( pReal, pVertexShader,
                                 sk_shader_class::Vertex,
                                   ppClassInstances, NumClassInstances );
  }
        
  void STDMETHODCALLTYPE DrawIndexed (
    _In_  UINT IndexCount,
    _In_  UINT StartIndexLocation,
    _In_  INT BaseVertexLocation) override
  {
    SK_LOG_FIRST_CALL

    static bool bIsShenmue = 
      ( SK_GetCurrentGameID () == SK_GAME_ID::Shenmue ||
        SK_GetCurrentGameID () == SK_GAME_ID::DragonQuestXI );

    switch (SK_GetCurrentGameID ())
    {
      case SK_GAME_ID::Shenmue:
        extern volatile LONG __SK_SHENMUE_FinishedButNotPresented;
        InterlockedExchange (&__SK_SHENMUE_FinishedButNotPresented, 1L);
    }

    if (ReadAcquire (&SK_D3D11_DrawTrackingReqs) > 0)
    {
      UINT dev_idx = SK_D3D11_GetDeviceContextHandle (this);
      if (SK_D3D11_DrawCallFilter ( IndexCount, IndexCount,
          SK_D3D11_Shaders.vertex.current.shader [dev_idx]) )
      {
        return;
      }

      extern volatile LONG __SK_SHENMUE_FullAspectCutscenes;
      if (bIsShenmue)// && ReadAcquire (&__SK_SHENMUE_FullAspectCutscenes))
      {
        if (IndexCount == 30 && (StartIndexLocation != 24))
        {
          UINT           numViews = 16,
                         numRects = 16;
          D3D11_VIEWPORT vps   [16] = { }; 
          D3D11_RECT     rects [16] = { };

          pReal->RSGetScissorRects (&numRects, &rects [0]);
          pReal->RSGetViewports    (&numViews, &vps   [0]);

          //CComPtr <ID3D11ShaderResourceView> pResView;  
          //pReal->PSGetShaderResources (0, 1, &pResView);

          if (SK_D3D11_Shaders.pixel.current.shader [dev_idx] == 0x29b11b07)
          {

            //if (pResView.p != nullptr)
            //  dll_log.Log (L"Pixel Shader Resource View: %ph", pResView.p);
            //
            //dll_log.Log (L"Number of Viewports: %lu",        numViews);

            //if (pResView.p != nullptr)
            //{
            //  CComPtr <ID3D11Resource> pRes;
            //  pResView->GetResource (&pRes);
            //  CComQIPtr <ID3D11Texture2D> pTex (pRes);
            //
            //  D3D11_TEXTURE2D_DESC tex_desc = { };
            //  pTex->GetDesc      (&tex_desc);
            //
            //  //dll_log.Log ( L"Shader Resource: { Format=%s, Dimensions=%lux%lu, Usage=%s }",
            //  //               SK_DXGI_FormatToStr    (tex_desc.Format).c_str (),
            //  //               tex_desc.Width,         tex_desc.Height,
            //  //               SK_D3D11_DescribeUsage (tex_desc.Usage) );
            //}

            for (UINT i = 0; i < numViews; i++)
            {
              if (config.system.log_level > 0)
              {
                dll_log.Log (L" VP[%lu] = < %lux%lu > // (%lu,%lu) // [%f-%f] }", i, (UINT)vps [i].Width,    (UINT)vps [i].Height,
                                                                                     (UINT)vps [i].TopLeftX, (UINT)vps [i].TopLeftY,
                                                                                           vps [i].MinDepth,       vps [i].MaxDepth );
              }
              auto& io =
                ImGui::GetIO ();

              if ( (numViews == 1 && ( vps [i].Width  == io.DisplaySize.x    ||
                                       vps [i].Height == io.DisplaySize.y )  ||
                    ((UINT)vps [i].TopLeftX == (2 * (io.DisplaySize.x / 16)) ||
                     (UINT)vps [i].TopLeftX ==      (io.DisplaySize.x / 16))))
              {
                if (numRects >= 1 && rects [0].right  == 16384 &&
                                     rects [0].bottom == 16384 )
                {
                  return;
                }

                if (config.system.log_level > 0)
                {
                  dll_log.Log (L"Number of Rects: %lu", numRects);

                  dll_log.Log ( L" >> [%lu,%lu] <> [%lu,%lu]",
                                  rects [0].left, rects [0].right,
                                  rects [0].top,  rects [0].bottom );
                }
              }
            }

            //for (int i = 0; i < numViews; i++)
            //{
            //  dll_log.Log (L" VP[%lu] = < %lux%lu > // (%lu,%lu) // [%f-%f] }", i, (UINT)vps [i].Width,    (UINT)vps [i].Height,
            //    (UINT)vps [i].TopLeftX, (UINT)vps [i].TopLeftY,
            //               vps [i].MinDepth,       vps [i].MaxDepth );
            //}                                                   

            if (config.system.log_level > 0)
            {
              dll_log.Log (L"Index Count: %lu, StartIndex: %lu, Base Vertex Loc: %lu",
                           IndexCount, StartIndexLocation, BaseVertexLocation);
            }
            //return;
          }
        }
      }
    }

    SK_TLS *pTLS = nullptr;

    if (! (pReal->GetType () == D3D11_DEVICE_CONTEXT_DEFERRED && (! config.render.dxgi.deferred_isolation)))
      SK_ReShade_DrawCallback.call (pReal, IndexCount * SK_D3D11_ReshadeDrawFactors.indexed, pTLS);

    if (! SK_D3D11_ShouldTrackRenderOp (pReal, &pTLS))
    {
      return
        pReal->DrawIndexed ( IndexCount,
                               StartIndexLocation,
                                 BaseVertexLocation );
    }

    if (SK_D3D11_DrawHandler (pReal, pTLS))
      return;

    pReal->DrawIndexed (IndexCount,
                          StartIndexLocation,
                            BaseVertexLocation);

    SK_D3D11_PostDraw (pTLS);
  }
        
  void STDMETHODCALLTYPE Draw (
    _In_  UINT VertexCount,
    _In_  UINT StartVertexLocation) override
  {
    SK_LOG_FIRST_CALL

    //UINT dev_idx = SK_D3D11_GetDeviceContextHandle (this);
    //if (SK_D3D11_DrawCallFilter ( VertexCount, VertexCount,
    //    SK_D3D11_Shaders.vertex.current.shader [dev_idx]) )
    //{
    //  return;
    //}

    ////extern volatile LONG __SK_SHENMUE_FullAspectCutscenes;
    ////if (ReadAcquire (&__SK_SHENMUE_FullAspectCutscenes))
    ////{
    ////  if (SK_D3D11_Shaders.pixel.current.shader [dev_idx] == 0x29b11b07)
    ////  {
    ////    if (VertexCount == 30)
    ////    {
    ////      dll_log.Log ( L"Vertex Count: %lu, Start Vertex Loc: %lu",
    ////                    VertexCount, StartVertexLocation );
    ////      return;
    ////    }
    ////  }
    ////}

    SK_TLS *pTLS = nullptr;

    if (! (pReal->GetType () == D3D11_DEVICE_CONTEXT_DEFERRED && (! config.render.dxgi.deferred_isolation)))
      SK_ReShade_DrawCallback.call (pReal, VertexCount * SK_D3D11_ReshadeDrawFactors.draw, pTLS);

    if (! SK_D3D11_ShouldTrackRenderOp (pReal, &pTLS))
    {
      return
        pReal->Draw ( VertexCount,
                        StartVertexLocation );
    }

    if (SK_D3D11_DrawHandler (pReal, pTLS))
      return;

    pReal->Draw ( VertexCount, StartVertexLocation );

    SK_D3D11_PostDraw (pTLS);
  }
        
  HRESULT STDMETHODCALLTYPE Map (
    _In_  ID3D11Resource *pResource,
    _In_  UINT Subresource,
    _In_  D3D11_MAP MapType,
    _In_  UINT MapFlags,
    _Out_  D3D11_MAPPED_SUBRESOURCE *pMappedResource) override
  {
    const HRESULT hr =
      pReal->Map ( pResource, Subresource,
                     MapType, MapFlags, pMappedResource );


    // UB: If it's happening, pretend we never saw this...
    if (pResource == nullptr)
    {
      assert (pResource != nullptr);

      return hr;
    }

    D3D11_RESOURCE_DIMENSION rdim = D3D11_RESOURCE_DIMENSION_UNKNOWN;
        pResource->GetType (&rdim);

    // ImGui gets to pass-through without invoking the hook
    if ((! config.textures.cache.allow_staging) && (! SK_D3D11_ShouldTrackRenderOp (pReal)))
    {
      return hr;
    }

    if (SUCCEEDED (hr))
    {
      SK_D3D11_MemoryThreads.mark ();

      if (SK_D3D11_IsStagingCacheable (rdim, pResource))
      {
        // Reference will be released, but we expect the game's going to unmap this at some point anyway...
        CComQIPtr <ID3D11Texture2D> pTex = pResource;

        static auto& textures =
          SK_D3D11_Textures;

        if (pTex != nullptr)
        {
                          D3D11_TEXTURE2D_DESC    d3d11_desc = { };
          pTex->GetDesc ((D3D11_TEXTURE2D_DESC *)&d3d11_desc);

          const SK_D3D11_TEXTURE2D_DESC desc (d3d11_desc);

          //dll_log.Log ( L"Staging Map: Type=%x, BindFlags: %s, Res: %lux%lu",
          //                MapType, SK_D3D11_DescribeBindFlags (desc.BindFlags).c_str (),
          //       desc.Width, desc.Height, SK_DXGI_FormatToStr (desc.Format).c_str    () );

          if (MapType == D3D11_MAP_WRITE_DISCARD)
          {
            auto&& it = textures.Textures_2D.find (pTex);
            if (  it != textures.Textures_2D.end  (    ) && it->second.crc32c != 0x00 )
            {
                                                                                 it->second.discard = true;
              pTex->SetPrivateData (SKID_D3D11Texture2D_DISCARD, sizeof (bool), &it->second.discard);

              SK_D3D11_RemoveTexFromCache (pTex, true);
              textures.HashMap_2D [it->second.orig_desc.MipLevels].erase (it->second.tag);

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
                  textures.Textures_2D [pTex];
                                                                                   texDesc.discard = true;
                pTex->SetPrivateData (SKID_D3D11Texture2D_DISCARD, sizeof (bool), &texDesc.discard);

                SK_LOG1 ( ( L"Removing discarded texture from cache (it has been memory-mapped multiple times)." ),
                            L"DX11TexMgr" );
                SK_D3D11_RemoveTexFromCache (pTex, true);
              }

              else if (pMappedResource != nullptr)
              {
                auto&& map_ctx = mapped_resources [pReal];

                map_ctx.textures.emplace      (std::make_pair (pResource, *pMappedResource));
                map_ctx.texture_times.emplace (std::make_pair (pResource, SK_QueryPerf ().QuadPart));

                //dll_log.Log (L"[DX11TexMgr] Mapped 2D texture...");
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
              //dll_log.Log (L"[DX11TexMgr] Skipped 2D texture...");
            }
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

      mem_map_stats.last_frame.map_types [MapType-1]++;

      switch (rdim)
      {
        case D3D11_RESOURCE_DIMENSION_UNKNOWN:
          mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_UNKNOWN]++;
          break;
        case D3D11_RESOURCE_DIMENSION_BUFFER:
        {
          mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_BUFFER]++;

          ID3D11Buffer* pBuffer = nullptr;

          if (SUCCEEDED (pResource->QueryInterface <ID3D11Buffer> (&pBuffer)))
          {
            D3D11_BUFFER_DESC  buf_desc = { };
            pBuffer->GetDesc (&buf_desc);
            {
              ///std::lock_guard <SK_Thread_CriticalSection> auto_lock (cs_mmio);

              // Extra memory allocation pressure for no good reason -- kill it.
              //  
              ////if (buf_desc.BindFlags & D3D11_BIND_INDEX_BUFFER)
              ////  mem_map_stats.last_frame.index_buffers.insert (pBuffer);
              ////
              ////if (buf_desc.BindFlags & D3D11_BIND_VERTEX_BUFFER)
              ////  mem_map_stats.last_frame.vertex_buffers.insert (pBuffer);
              ////
              ////if (buf_desc.BindFlags & D3D11_BIND_CONSTANT_BUFFER)
              ////  mem_map_stats.last_frame.constant_buffers.insert (pBuffer);
            }

            if (read)
              mem_map_stats.last_frame.bytes_read    += buf_desc.ByteWidth;

            if (write)
              mem_map_stats.last_frame.bytes_written += buf_desc.ByteWidth;

            pBuffer->Release ();
          }
        } break;
        case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
          mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_TEXTURE1D]++;
          break;
        case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
          mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_TEXTURE2D]++;
          break;
        case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
          mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_TEXTURE3D]++;
          break;
      }
    }

    return hr;
  }
        
  void STDMETHODCALLTYPE Unmap (
    _In_  ID3D11Resource *pResource,
    _In_  UINT Subresource) override
  {
    // UB: If it's happening, pretend we never saw this...
    if (pResource == nullptr)
    {
      assert (pResource != nullptr);

      return
        pReal->Unmap (pResource, Subresource);
    }

    SK_TLS *pTLS = nullptr;

    // ImGui gets to pass-through without invoking the hook
    if ((! config.textures.cache.allow_staging) && (! SK_D3D11_ShouldTrackRenderOp (pReal, &pTLS)))
    {
      D3D11_Unmap_Original (pReal, pResource, Subresource);
      return;
    }

    pTLS = ( pTLS != nullptr       ?
             pTLS : SK_TLS_Bottom ( ) );

    SK_D3D11_MemoryThreads.mark ();

    D3D11_RESOURCE_DIMENSION rdim = D3D11_RESOURCE_DIMENSION_UNKNOWN;
        pResource->GetType (&rdim);

    if (SK_D3D11_IsStagingCacheable (rdim, pResource) && Subresource == 0)
    {
      if (rdim == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
      {
        auto&& map_ctx = mapped_resources [pReal];

        // More of an assertion, if this fails something's screwy!
        if (map_ctx.textures.count (pResource))
        {
          std::lock_guard <SK_Thread_CriticalSection> auto_lock  (*cs_mmio);

          uint64_t time_elapsed =
            SK_QueryPerf ().QuadPart - map_ctx.texture_times [pResource];

          CComQIPtr <ID3D11Texture2D> pTex = pResource;

          if (pTex != nullptr)
          {
            uint32_t checksum  = 0;
            size_t   size      = 0;
            uint32_t top_crc32 = 0x00;

            D3D11_TEXTURE2D_DESC desc = { };
            pTex->GetDesc (&desc);

            if ((desc.BindFlags & D3D11_BIND_SHADER_RESOURCE) || desc.Usage == D3D11_USAGE_STAGING)
            {
              checksum =
                crc32_tex ( &desc,
                (D3D11_SUBRESOURCE_DATA *)(
                           &map_ctx.textures [pResource]
                ), &size, &top_crc32 );

              //dll_log.Log (L"[DX11TexMgr] Mapped 2D texture... (%x -- %lu bytes)", checksum, size);

              if (checksum != 0x0)
              {
                if (desc.Usage != D3D11_USAGE_STAGING)
                {
                  static auto& textures =
                    SK_D3D11_Textures;

                  const uint32_t cache_tag    =
                    safe_crc32c (top_crc32, (uint8_t *)(&desc), sizeof D3D11_TEXTURE2D_DESC);

                  textures.CacheMisses_2D++;

                  textures.refTexture2D ( pTex,
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
                }
              }
            }
          }

          map_ctx.textures.erase      (pResource);
          map_ctx.texture_times.erase (pResource);
        }
      }
    }

    return
      pReal->Unmap (pResource, Subresource);
  }
        
  void STDMETHODCALLTYPE PSSetConstantBuffers (
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
    _In_reads_opt_ (NumBuffers)  ID3D11Buffer *const *ppConstantBuffers) override
  {
    return pReal->PSSetConstantBuffers (StartSlot, NumBuffers, ppConstantBuffers);
  }
        
  void STDMETHODCALLTYPE IASetInputLayout( 
 
      _In_opt_  ID3D11InputLayout *pInputLayout) override {
    return pReal->IASetInputLayout (pInputLayout);
  }
        
  void STDMETHODCALLTYPE IASetVertexBuffers( 
      _In_range_( 0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
      _In_range_( 0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppVertexBuffers,
      _In_reads_opt_(NumBuffers)  const UINT *pStrides,
      _In_reads_opt_(NumBuffers)  const UINT *pOffsets) override {
    return pReal->IASetVertexBuffers (StartSlot, NumBuffers, ppVertexBuffers, pStrides, pOffsets);
  }
  
  void STDMETHODCALLTYPE IASetIndexBuffer( 
      _In_opt_  ID3D11Buffer *pIndexBuffer,
      _In_  DXGI_FORMAT Format,
      _In_  UINT Offset) override {
    return pReal->IASetIndexBuffer (pIndexBuffer, Format, Offset);
  }
  
  void STDMETHODCALLTYPE DrawIndexedInstanced (
    _In_  UINT IndexCountPerInstance,
    _In_  UINT InstanceCount,
    _In_  UINT StartIndexLocation,
    _In_  INT BaseVertexLocation,
    _In_  UINT StartInstanceLocation) override
  {
    SK_LOG_FIRST_CALL

    SK_TLS *pTLS = nullptr;

    if (! SK_D3D11_ShouldTrackRenderOp (pReal, &pTLS))
    {
      return
        pReal->DrawIndexedInstanced ( IndexCountPerInstance,
                                        InstanceCount, StartIndexLocation,
                                          BaseVertexLocation, StartInstanceLocation );
    }

    if (SK_D3D11_DrawHandler (pReal, pTLS))
      return;

    if (! (pReal->GetType () == D3D11_DEVICE_CONTEXT_DEFERRED && (! config.render.dxgi.deferred_isolation)))
      SK_ReShade_DrawCallback.call (pReal, IndexCountPerInstance * InstanceCount * SK_D3D11_ReshadeDrawFactors.indexed_instanced, pTLS);

    pReal->DrawIndexedInstanced ( IndexCountPerInstance,
                                    InstanceCount, StartIndexLocation,
                                      BaseVertexLocation, StartInstanceLocation );

    SK_D3D11_PostDraw (pTLS);
  }
  
  void STDMETHODCALLTYPE DrawInstanced (
    _In_  UINT VertexCountPerInstance,
    _In_  UINT InstanceCount,
    _In_  UINT StartVertexLocation,
    _In_  UINT StartInstanceLocation) override
  {
    SK_LOG_FIRST_CALL

    SK_TLS *pTLS = nullptr;

    if (! (pReal->GetType () == D3D11_DEVICE_CONTEXT_DEFERRED && (! config.render.dxgi.deferred_isolation)))
      SK_ReShade_DrawCallback.call (pReal, VertexCountPerInstance * InstanceCount * SK_D3D11_ReshadeDrawFactors.instanced, pTLS);

    if (! SK_D3D11_ShouldTrackRenderOp (pReal, &pTLS))
    {
      return
        pReal->DrawInstanced ( VertexCountPerInstance,
                                 InstanceCount, StartVertexLocation,
                                   StartInstanceLocation );
    }

    if (SK_D3D11_DrawHandler (pReal, pTLS))
      return;

    pReal->DrawInstanced ( VertexCountPerInstance,
                             InstanceCount, StartVertexLocation,
                               StartInstanceLocation );

    SK_D3D11_PostDraw (pTLS);
  }
  
  void STDMETHODCALLTYPE GSSetConstantBuffers( 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
      _In_reads_opt_(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers) override
  {
    return pReal->GSSetConstantBuffers (StartSlot, NumBuffers, ppConstantBuffers);
  }
  
  void STDMETHODCALLTYPE GSSetShader( 
      _In_opt_  ID3D11GeometryShader *pShader,
      _In_reads_opt_(NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
      UINT NumClassInstances) override
  {
    return
      SK_D3D11_SetShader_Impl ( pReal, pShader,
                                 sk_shader_class::Geometry,
                                   ppClassInstances, NumClassInstances );
  }
  
  void STDMETHODCALLTYPE IASetPrimitiveTopology (

    _In_  D3D11_PRIMITIVE_TOPOLOGY Topology) override
  {
    return pReal->IASetPrimitiveTopology (Topology);
  }
  
  void STDMETHODCALLTYPE VSSetShaderResources( 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
      _In_reads_opt_(NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews) override
  {
    static auto& shaders =
      SK_D3D11_Shaders;

    constexpr int VERTEX_SHADER_STAGE = 1;

    SK_TLS* pTLS = nullptr;

    // ImGui gets to pass-through without invoking the hook
    if ( (! SK_D3D11_ShouldTrackSetShaderResources (pReal, &pTLS)) ||
         (! SK_D3D11_ShouldTrackRenderOp           (pReal, &pTLS)) )
      return pReal->VSSetShaderResources (StartSlot, NumViews, ppShaderResourceViews);

    auto&& newResourceViews = pTLS->d3d11.newResourceViews;

    RtlZeroMemory (newResourceViews, sizeof (ID3D11ShaderResourceView*) * NumViews);

    if (ppShaderResourceViews && NumViews > 0)
    {
      const UINT dev_idx =
        SK_D3D11_GetDeviceContextHandle (this);

      auto&& views =
                     shaders.vertex.current.views [dev_idx];
      auto&& stage =
        d3d11_shader_stages [VERTEX_SHADER_STAGE] [dev_idx];

      d3d11_shader_tracking_s& tracked =
        shaders.vertex.tracked;

      const uint32_t shader_crc32c = tracked.crc32c.load ();
      const bool     active        = tracked.active.get  (dev_idx);

      for (UINT i = 0; i < NumViews; i++)
      {
        if (SK_D3D11_ActivateSRVOnSlot (dev_idx, stage,
                                                 ppShaderResourceViews [i], StartSlot + i))
          newResourceViews [i] =                 ppShaderResourceViews [i];
        else
          newResourceViews [i] = nullptr;
      }

      if (active && shader_crc32c != 0)
      {
        for (UINT i = 0; i < NumViews; i++)
        {
          if (ppShaderResourceViews [i])                                            {
            std::lock_guard <SK_Thread_CriticalSection> auto_lock (*cs_shader_vs);
            if (tracked.set_of_views.emplace (ppShaderResourceViews [i]).second)    {
                                              ppShaderResourceViews [i]->AddRef (); }
             tracked.used_views.emplace_back (ppShaderResourceViews [i]);           }
                      views [StartSlot + i] = ppShaderResourceViews [i];
        }
      }
    }

    return
      pReal->VSSetShaderResources (StartSlot, NumViews, newResourceViews);
  }
  


  void STDMETHODCALLTYPE VSSetSamplers (
    _In_range_ (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,
    _In_range_ (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,
    _In_reads_opt_ (NumSamplers)  ID3D11SamplerState *const *ppSamplers) override
  {
    return pReal->VSSetSamplers (StartSlot, NumSamplers, ppSamplers);
  }
  
  void STDMETHODCALLTYPE Begin ( 
      _In_  ID3D11Asynchronous *pAsync) override
  {
    return pReal->Begin (pAsync);
  }
  
  void STDMETHODCALLTYPE End( 
      _In_  ID3D11Asynchronous *pAsync) override
  {
    return pReal->End (pAsync);
  }
  
  HRESULT STDMETHODCALLTYPE GetData( 
      _In_  ID3D11Asynchronous *pAsync,
      _Out_writes_bytes_opt_( DataSize )  void *pData,
      _In_  UINT DataSize,
      _In_  UINT GetDataFlags) override
  {
    return
      pReal->GetData (pAsync, pData, DataSize, GetDataFlags);
  };
  
  void STDMETHODCALLTYPE SetPredication( 
      _In_opt_  ID3D11Predicate *pPredicate,
      _In_  BOOL PredicateValue) override
  {
    return pReal->SetPredication (pPredicate, PredicateValue);
  }
  
  void STDMETHODCALLTYPE GSSetShaderResources( 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
      _In_reads_opt_(NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews) override
  {
    static auto& shaders =
      SK_D3D11_Shaders;

    constexpr int GEOMETRY_SHADER_STAGE = 2;

    SK_TLS *pTLS = nullptr;

    // ImGui gets to pass-through without invoking the hook
    if ( (! SK_D3D11_ShouldTrackSetShaderResources (pReal, &pTLS)) ||
         (! SK_D3D11_ShouldTrackRenderOp           (pReal, &pTLS)) )
      return pReal->GSSetShaderResources (StartSlot, NumViews, ppShaderResourceViews);

    auto&& newResourceViews = pTLS->d3d11.newResourceViews;

    RtlZeroMemory (newResourceViews, sizeof (ID3D11ShaderResourceView*) * NumViews);

    if (ppShaderResourceViews && NumViews > 0)
    {
      const UINT dev_idx =
        SK_D3D11_GetDeviceContextHandle (pReal);

      auto&& views = shaders.geometry.current.views [dev_idx];
      auto&& stage =
        d3d11_shader_stages [GEOMETRY_SHADER_STAGE] [dev_idx];

      d3d11_shader_tracking_s& tracked =
        shaders.geometry.tracked;

      const uint32_t shader_crc32c = tracked.crc32c.load ();
      const bool     active        = tracked.active.get  (dev_idx);

      for (UINT i = 0; i < NumViews; i++)
      {
        if (SK_D3D11_ActivateSRVOnSlot (dev_idx, stage,
            ppShaderResourceViews [i], StartSlot + i))
          newResourceViews [i] =            ppShaderResourceViews [i];
        else
          newResourceViews [i] = nullptr;
      }

      if (active && shader_crc32c != 0)
      {
        for (UINT i = 0; i < NumViews; i++)
        {
          if (ppShaderResourceViews [i])                                            {
            std::lock_guard <SK_Thread_CriticalSection> auto_lock (*cs_shader_gs);
            if (tracked.set_of_views.emplace (ppShaderResourceViews [i]).second)    {
              ppShaderResourceViews [i]->AddRef (); }
            tracked.used_views.emplace_back (ppShaderResourceViews [i]);           }
          views [StartSlot + i] = ppShaderResourceViews [i];
        }
      }
    }

    return pReal->GSSetShaderResources (StartSlot, NumViews, newResourceViews);
  }
  
  void STDMETHODCALLTYPE GSSetSamplers (
    _In_range_ (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,
    _In_range_ (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,
    _In_reads_opt_ (NumSamplers)  ID3D11SamplerState *const *ppSamplers) override
  {
    return pReal->GSSetSamplers (StartSlot, NumSamplers, ppSamplers);
  }
  
  void STDMETHODCALLTYPE OMSetRenderTargets (
    _In_range_ (0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT)  UINT NumViews,
    _In_reads_opt_ (NumViews)  ID3D11RenderTargetView *const *ppRenderTargetViews,
    _In_opt_  ID3D11DepthStencilView *pDepthStencilView) override
  {
    ID3D11DepthStencilView *pDSV =
      pDepthStencilView;

    SK_TLS *pTLS = nullptr;

    static auto& shaders =
      SK_D3D11_Shaders;

    UINT dev_idx =
      SK_D3D11_GetDeviceContextHandle (pReal);


    if (pDepthStencilView != nullptr)
      SK_ReShade_SetDepthStencilViewCallback.call (pDSV, pTLS);

    if (! SK_D3D11_ShouldTrackRenderOp (pReal, &pTLS))
    {
      if (tracked_rtv.active_count [dev_idx] > 0)
      {
        for (auto&& rtv : tracked_rtv.active [dev_idx]) rtv.store (false);

        tracked_rtv.active_count [dev_idx] = 0;
      }

      return
        pReal->OMSetRenderTargets ( NumViews, ppRenderTargetViews, pDSV );
    }

    pReal->OMSetRenderTargets (NumViews, ppRenderTargetViews, pDSV);

    if (NumViews > 0)
    {
      if (ppRenderTargetViews)
      {
        auto&                              rt_views =
          SK_D3D11_RenderTargets [dev_idx].rt_views;

        const auto* tracked_rtv_res  = 
          static_cast <ID3D11RenderTargetView *> (
            ReadPointerAcquire ((volatile PVOID *)&tracked_rtv.resource)
            );

        for (UINT i = 0; i < NumViews; i++)
        {
          if (ppRenderTargetViews [i] && rt_views.emplace (ppRenderTargetViews [i]).second)
            ppRenderTargetViews [i]->AddRef ();

          const bool active_before = tracked_rtv.active_count [dev_idx] > 0 ? 
            tracked_rtv.active       [dev_idx][i].load ()
            : false;

          const bool active = 
            ( tracked_rtv_res == ppRenderTargetViews [i] ) ?
            true :
            false;

          if (active_before != active)
          {
            tracked_rtv.active [dev_idx][i] = active;

            if      (            active                    ) tracked_rtv.active_count [dev_idx]++;
            else if (tracked_rtv.active_count [dev_idx] > 0) tracked_rtv.active_count [dev_idx]--;
          }
        }

        for ( UINT j = 0; j < 5 ; j++ )
        {
          switch (j)
          { case 0:
          {
            INT  pre_hud_slot  = shaders.vertex.tracked.pre_hud_rt_slot;
            if ( pre_hud_slot >= 0 && pre_hud_slot < (INT)NumViews )
            {
              if (shaders.vertex.tracked.pre_hud_rtv == nullptr &&
                  shaders.vertex.current.shader [dev_idx] == 
                  shaders.vertex.tracked.crc32c.load () )
              {
                if (ppRenderTargetViews [pre_hud_slot] != nullptr)
                {
                  shaders.vertex.tracked.pre_hud_rtv = ppRenderTargetViews [pre_hud_slot];
                  shaders.vertex.tracked.pre_hud_rtv->AddRef ();
                }
              }
            }

          } break;

          case 1:
          {
            INT  pre_hud_slot  = shaders.pixel.tracked.pre_hud_rt_slot;
            if ( pre_hud_slot >= 0 && pre_hud_slot < (INT)NumViews )
            {
              if (shaders.pixel.tracked.pre_hud_rtv == nullptr &&
                  shaders.pixel.current.shader [dev_idx] == 
                  shaders.pixel.tracked.crc32c.load () )
              {
                if (ppRenderTargetViews [pre_hud_slot] != nullptr)
                {
                  shaders.pixel.tracked.pre_hud_rtv = ppRenderTargetViews [pre_hud_slot];
                  shaders.pixel.tracked.pre_hud_rtv->AddRef ();
                }
              }
            }
          } break;

          default:
            break;
          }
        }
      }

      if (pDepthStencilView)
      {
        auto& ds_views =
          SK_D3D11_RenderTargets [dev_idx].ds_views;

        if (ds_views.emplace (pDepthStencilView).second)
          pDepthStencilView->AddRef ();
      }
    }
  }
  
  void STDMETHODCALLTYPE OMSetRenderTargetsAndUnorderedAccessViews (
    _In_  UINT NumRTVs,
    _In_reads_opt_ (NumRTVs)  ID3D11RenderTargetView *const *ppRenderTargetViews,
    _In_opt_  ID3D11DepthStencilView *pDepthStencilView,
    _In_range_ (0, D3D11_1_UAV_SLOT_COUNT - 1)  UINT UAVStartSlot,
    _In_  UINT NumUAVs,
    _In_reads_opt_ (NumUAVs)  ID3D11UnorderedAccessView *const *ppUnorderedAccessViews,
    _In_reads_opt_ (NumUAVs)  const UINT *pUAVInitialCounts) override
  {
    ID3D11DepthStencilView *pDSV = pDepthStencilView;

    SK_TLS *pTLS = nullptr;

    UINT dev_idx =
      SK_D3D11_GetDeviceContextHandle (pReal);

    if (pDepthStencilView != nullptr)
      SK_ReShade_SetDepthStencilViewCallback.call (pDSV, pTLS);

    // ImGui gets to pass-through without invoking the hook
    if (! SK_D3D11_ShouldTrackRenderOp (pReal, &pTLS))
    {
      for (auto&& i : tracked_rtv.active [dev_idx]) i.store (false);

      tracked_rtv.active_count [dev_idx] = 0;

      pReal->OMSetRenderTargetsAndUnorderedAccessViews (
        NumRTVs, ppRenderTargetViews, pDSV, UAVStartSlot, NumUAVs,
          ppUnorderedAccessViews, pUAVInitialCounts
      );

      return;
    }

    pReal->OMSetRenderTargetsAndUnorderedAccessViews (
      NumRTVs, ppRenderTargetViews, pDSV, UAVStartSlot, NumUAVs,
        ppUnorderedAccessViews, pUAVInitialCounts
    );

    if (NumRTVs > 0)
    {
      if (ppRenderTargetViews)
      {
        auto&                              rt_views =
          SK_D3D11_RenderTargets [dev_idx].rt_views;

        const auto* tracked_rtv_res = 
          static_cast <ID3D11RenderTargetView *> (
            ReadPointerAcquire ((volatile PVOID *)&tracked_rtv.resource)
            );

        for (UINT i = 0; i < NumRTVs; i++)
        {
          if (ppRenderTargetViews [i] && rt_views.emplace (ppRenderTargetViews [i]).second)
              ppRenderTargetViews [i]->AddRef ();

          const bool active_before = tracked_rtv.active_count [dev_idx] > 0 ? 
            tracked_rtv.active       [dev_idx][i].load ()
            : false;

          const bool active = 
            ( tracked_rtv_res == ppRenderTargetViews [i] ) ?
            true :
            false;

          if (active_before != active)
          {
            tracked_rtv.active [dev_idx][i] = active;

            if      (            active                    ) tracked_rtv.active_count [dev_idx]++;
            else if (tracked_rtv.active_count [dev_idx] > 0) tracked_rtv.active_count [dev_idx]--;
          }
        }
      }

      if (pDepthStencilView)
      {
        auto& ds_views =
          SK_D3D11_RenderTargets [dev_idx].ds_views;

        if (! ds_views.count (pDepthStencilView))
        {
          pDepthStencilView->AddRef ();
          ds_views.insert (pDepthStencilView);
        }
      }
    }
  }
  
  void STDMETHODCALLTYPE OMSetBlendState (
    _In_opt_  ID3D11BlendState *pBlendState,
    _In_opt_  const FLOAT BlendFactor [4],
    _In_  UINT SampleMask) override
  {
    return pReal->OMSetBlendState (pBlendState, BlendFactor, SampleMask);
  }
  
  void STDMETHODCALLTYPE OMSetDepthStencilState (

    _In_opt_  ID3D11DepthStencilState *pDepthStencilState,

    _In_  UINT StencilRef) override
  {
    return pReal->OMSetDepthStencilState (pDepthStencilState, StencilRef);
  }
  
  void STDMETHODCALLTYPE SOSetTargets (

    _In_range_ (0, D3D11_SO_BUFFER_SLOT_COUNT)  UINT NumBuffers,

    _In_reads_opt_ (NumBuffers)  ID3D11Buffer *const *ppSOTargets,

    _In_reads_opt_ (NumBuffers)  const UINT *pOffsets) override
  {
    return pReal->SOSetTargets (NumBuffers, ppSOTargets, pOffsets);
  }
  
  void STDMETHODCALLTYPE DrawAuto (void) override
  {
    SK_LOG_FIRST_CALL
      
    SK_TLS *pTLS = nullptr;

    if (! (pReal->GetType () == D3D11_DEVICE_CONTEXT_DEFERRED && (! config.render.dxgi.deferred_isolation)))
      SK_ReShade_DrawCallback.call (pReal, SK_D3D11_ReshadeDrawFactors.auto_draw, pTLS);

    if (! SK_D3D11_ShouldTrackRenderOp (pReal, &pTLS))
    {
      return
        D3D11_DrawAuto_Original (pReal);
    }

    if (SK_D3D11_DrawHandler (pReal, pTLS))
      return;

    pReal->DrawAuto ();

    SK_D3D11_PostDraw (pTLS);
  }
  
  void STDMETHODCALLTYPE DrawIndexedInstancedIndirect (
    _In_  ID3D11Buffer *pBufferForArgs,
    _In_  UINT AlignedByteOffsetForArgs) override
  {
    SK_LOG_FIRST_CALL

    SK_TLS *pTLS = nullptr;

    if (! (pReal->GetType () == D3D11_DEVICE_CONTEXT_DEFERRED && (! config.render.dxgi.deferred_isolation)))
      SK_ReShade_DrawCallback.call (pReal, SK_D3D11_ReshadeDrawFactors.instanced_indirect, pTLS);

    if (! SK_D3D11_ShouldTrackRenderOp (pReal, &pTLS))
    {
      return
        pReal->DrawIndexedInstancedIndirect ( pBufferForArgs,
                                                AlignedByteOffsetForArgs );
    }

    if (SK_D3D11_DrawHandler (pReal, pTLS))
      return;

    pReal->DrawIndexedInstancedIndirect ( pBufferForArgs,
                                            AlignedByteOffsetForArgs );

    SK_D3D11_PostDraw (pTLS);
  }
  
  void STDMETHODCALLTYPE DrawInstancedIndirect (
    _In_  ID3D11Buffer *pBufferForArgs,
    _In_  UINT AlignedByteOffsetForArgs) override
  {
    SK_LOG_FIRST_CALL

    SK_TLS *pTLS = nullptr;

    if (! (pReal->GetType () == D3D11_DEVICE_CONTEXT_DEFERRED && (! config.render.dxgi.deferred_isolation)))
      SK_ReShade_DrawCallback.call (pReal, SK_D3D11_ReshadeDrawFactors.instanced_indirect, pTLS);

    if (! SK_D3D11_ShouldTrackRenderOp (pReal, &pTLS))
    {
      return
        pReal->DrawInstancedIndirect ( pBufferForArgs,
                                         AlignedByteOffsetForArgs );
    }

    if (SK_D3D11_DrawHandler (pReal, pTLS))
      return;

    pReal->DrawInstancedIndirect ( pBufferForArgs,
                                     AlignedByteOffsetForArgs );

    SK_D3D11_PostDraw (pTLS);
  }
  
  void STDMETHODCALLTYPE Dispatch (
    _In_  UINT ThreadGroupCountX,
    _In_  UINT ThreadGroupCountY,
    _In_  UINT ThreadGroupCountZ) override
  {
    SK_LOG_FIRST_CALL

    SK_TLS *pTLS = nullptr;

    if (! SK_D3D11_ShouldTrackRenderOp (pReal, &pTLS))
    {
      return
        pReal->Dispatch ( ThreadGroupCountX,
                            ThreadGroupCountY,
                              ThreadGroupCountZ );
    }

    if (SK_D3D11_DispatchHandler (pReal, pTLS))
      return;

    pReal->Dispatch ( ThreadGroupCountX,
                        ThreadGroupCountY,
                          ThreadGroupCountZ );

    SK_D3D11_PostDispatch (pReal, pTLS);  SK_LOG_FIRST_CALL
  }
  
  void STDMETHODCALLTYPE DispatchIndirect( 
      _In_  ID3D11Buffer *pBufferForArgs,
      _In_  UINT AlignedByteOffsetForArgs) override
  {
    SK_LOG_FIRST_CALL

    SK_TLS *pTLS = nullptr;

    if (! SK_D3D11_ShouldTrackRenderOp (pReal, &pTLS))
    {
      return
        pReal->DispatchIndirect ( pBufferForArgs,
                                    AlignedByteOffsetForArgs );
    }

    if (SK_D3D11_DispatchHandler (pReal, pTLS))
      return;

    pReal->DispatchIndirect ( pBufferForArgs,
                                AlignedByteOffsetForArgs );

    SK_D3D11_PostDispatch (pReal, pTLS);
  }
  
  void STDMETHODCALLTYPE RSSetState (

    _In_opt_  ID3D11RasterizerState *pRasterizerState) override
  {
    return pReal->RSSetState (pRasterizerState);
  }
  
  void STDMETHODCALLTYPE RSSetViewports (
    _In_range_ (0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)  UINT NumViewports,
    _In_reads_opt_ (NumViewports)  const D3D11_VIEWPORT *pViewports) override
  {
    //if (SK_GetCurrentGameID () == SK_GAME_ID::Yakuza0)
    //{
    //  if (NumViewports == 1)
    //  {
    //    if (pViewports [0].Width == 512.0f && pViewports [0].Height == 512.0f)
    //    {
    //      D3D11_VIEWPORT vp = *pViewports;
    //      vp.Width = 1024.0f; vp.Height = 1024.0f;
    //      return pReal->RSSetViewports (1, &vp);
    //    }
    //    //dll_log.Log ( L"  VP_XXX -- (%8.3f x %8.3f) | <%5.2f, %5.2f> | [%3.1f - %3.1f]",
    //    //             pViewports [0].Width,    pViewports [0].Height,
    //    //             pViewports [0].TopLeftX, pViewports [0].TopLeftY,
    //    //             pViewports [0].MinDepth, pViewports [0].MaxDepth );
    //  }
    //}

    return pReal->RSSetViewports (NumViewports, pViewports);
  }
  
  void STDMETHODCALLTYPE RSSetScissorRects (

    _In_range_ (0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)  UINT NumRects,

    _In_reads_opt_ (NumRects)  const D3D11_RECT *pRects) override
  {
    return pReal->RSSetScissorRects (NumRects, pRects);
  }
  
  void STDMETHODCALLTYPE CopySubresourceRegion (
    _In_  ID3D11Resource *pDstResource,
    _In_  UINT DstSubresource,
    _In_  UINT DstX,
    _In_  UINT DstY,
    _In_  UINT DstZ,
    _In_  ID3D11Resource *pSrcResource,
    _In_  UINT SrcSubresource,
    _In_opt_  const D3D11_BOX *pSrcBox) override
  {
    // UB: If it's happening, pretend we never saw this...
    if (pDstResource == nullptr || pSrcResource == nullptr)
    {
      return;
    }


    ///if (SK_GetCurrentGameID() == SK_GAME_ID::Ys_Eight)
    ///{
    ///  CComQIPtr <ID3D11Texture2D> pTex (pSrcResource);
    ///
    ///  if (pTex)
    ///  {
    ///    D3D11_BOX box = { };
    ///    
    ///    if (pSrcBox != nullptr)
    ///        box = *pSrcBox;
    ///
    ///    else
    ///    {
    ///      D3D11_TEXTURE2D_DESC tex_desc = {};
    ///           pTex->GetDesc (&tex_desc);
    ///
    ///      box.left  = 0; box.right  = tex_desc.Width; 
    ///      box.top   = 0; box.bottom = tex_desc.Height;
    ///      box.front = 0; box.back   = 1;
    ///    }
    ///    
    ///    dll_log.Log ( L"CopySubresourceRegion:  { %s <%lu> [ %lu/%lu, %lu/%lu, %lu/%lu ] } -> { %s <%lu> (%lu,%lu,%lu) }",
    ///                    DescribeResource (pSrcResource).c_str (), SrcSubresource, box.left,box.right, box.top,box.bottom, box.front,box.back,
    ///                    DescribeResource (pDstResource).c_str (), DstSubresource, DstX, DstY, DstZ );
    ///  }
    ///}


    ///if ( (! config.render.dxgi.deferred_isolation)    &&
    ///          pReal->GetType () == D3D11_DEVICE_CONTEXT_DEFERRED )
    ///{
    ///  return
    ///    pReal->CopySubresourceRegion ( pDstResource, DstSubresource,
    ///                                     DstX, DstY, DstZ,
    ///                                       pSrcResource, SrcSubresource,
    ///                                         pSrcBox );
    ///}

    static auto& textures =
      SK_D3D11_Textures;

    SK_TLS* pTLS = SK_TLS_Bottom ();

    CComQIPtr <ID3D11Texture2D> pDstTex (pDstResource);

    if (pDstTex != nullptr)
    {
      if (! SK_D3D11_IsTexInjectThread (pTLS))
      {
        //if (SK_GetCurrentGameID () == SK_GAME_ID::PillarsOfEternity2)
        //{
        //  extern          bool SK_POE2_NopSubresourceCopy;
        //  extern volatile LONG SK_POE2_SkippedCopies;
        //
        //  if (SK_POE2_NopSubresourceCopy)
        //  {
        //    D3D11_TEXTURE2D_DESC desc_out = { };
        //      pDstTex->GetDesc (&desc_out);
        //
        //    if (pSrcBox != nullptr)
        //    {
        //      dll_log.Log (L"Copy (%lu-%lu,%lu-%lu : %lu,%lu,%lu : %s : {%p,%p})",
        //        pSrcBox->left, pSrcBox->right, pSrcBox->top, pSrcBox->bottom,
        //          DstX, DstY, DstZ,
        //            SK_D3D11_DescribeUsage (desc_out.Usage),
        //              pSrcResource, pDstResource );
        //    }
        //
        //    else
        //    {
        //      dll_log.Log (L"Copy (%lu,%lu,%lu : %s)",
        //                   DstX, DstY, DstZ,
        //                   SK_D3D11_DescribeUsage (desc_out.Usage) );
        //    }
        //
        //    if (pSrcBox == nullptr || ( pSrcBox->right != 3840 || pSrcBox->bottom != 2160 ))
        //    {
        //      if (desc_out.Usage == D3D11_USAGE_STAGING || pSrcBox == nullptr)
        //      {
        //        InterlockedIncrement (&SK_POE2_SkippedCopies);
        //
        //        return;
        //      }
        //    }
        //  }
        //}

        if (DstSubresource == 0 && SK_D3D11_TextureIsCached (pDstTex))
        {
          SK_LOG0 ( (L"Cached texture was modified (CopySubresourceRegion)... removing from cache! - <%s>",
                         SK_GetCallerName ().c_str ()), L"DX11TexMgr" );
          SK_D3D11_RemoveTexFromCache (pDstTex, true);
        }
      }
    }


    // ImGui gets to pass-through without invoking the hook
    if (! SK_D3D11_ShouldTrackRenderOp (pReal, &pTLS))
    {
      pReal->CopySubresourceRegion (pDstResource, DstSubresource, DstX, DstY, DstZ, pSrcResource, SrcSubresource, pSrcBox);

      return;
    }


    D3D11_RESOURCE_DIMENSION res_dim = { };
    pSrcResource->GetType  (&res_dim);


    if (SK_D3D11_EnableMMIOTracking)
    {
      SK_D3D11_MemoryThreads.mark ();

      switch (res_dim)
      {
        case D3D11_RESOURCE_DIMENSION_UNKNOWN:
          mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_UNKNOWN]++;
          break;
        case D3D11_RESOURCE_DIMENSION_BUFFER:
        {
          mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_BUFFER]++;

          ID3D11Buffer* pBuffer = nullptr;

          if (SUCCEEDED (pSrcResource->QueryInterface <ID3D11Buffer> (&pBuffer)))
          {
            D3D11_BUFFER_DESC  buf_desc = { };
            pBuffer->GetDesc (&buf_desc);
            {
              ////std::lock_guard <SK_Thread_CriticalSection> auto_lock (cs_mmio);

              if (buf_desc.BindFlags & D3D11_BIND_INDEX_BUFFER)
                mem_map_stats.last_frame.index_buffers.insert (pBuffer);

              if (buf_desc.BindFlags & D3D11_BIND_VERTEX_BUFFER)
                mem_map_stats.last_frame.vertex_buffers.insert (pBuffer);

              if (buf_desc.BindFlags & D3D11_BIND_CONSTANT_BUFFER)
                mem_map_stats.last_frame.constant_buffers.insert (pBuffer);
            }

            mem_map_stats.last_frame.bytes_copied += buf_desc.ByteWidth;

            pBuffer->Release ();
          }
        } break;
        case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
          mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_TEXTURE1D]++;
          break;
        case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
          mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_TEXTURE2D]++;
          break;
        case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
          mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_TEXTURE3D]++;
          break;
      }
    }


    pReal->CopySubresourceRegion (pDstResource, DstSubresource, DstX, DstY, DstZ, pSrcResource, SrcSubresource, pSrcBox);

    if ( ( SK_D3D11_IsStagingCacheable (res_dim, pSrcResource) ||
           SK_D3D11_IsStagingCacheable (res_dim, pDstResource) ) && SrcSubresource == 0 && DstSubresource == 0)
    {
      auto&& map_ctx = mapped_resources [pReal];

      if (pDstTex != nullptr && map_ctx.dynamic_textures.count (pSrcResource))
      {
        const uint32_t top_crc32 = map_ctx.dynamic_texturesx [pSrcResource];
        const uint32_t checksum  = map_ctx.dynamic_textures  [pSrcResource];

        D3D11_TEXTURE2D_DESC dst_desc = { };
          pDstTex->GetDesc (&dst_desc);

        const uint32_t cache_tag =
          safe_crc32c (top_crc32, (uint8_t *)(&dst_desc), sizeof D3D11_TEXTURE2D_DESC);

        if (checksum != 0x00 && dst_desc.Usage != D3D11_USAGE_STAGING)
        {
          textures.refTexture2D ( pDstTex,
                                    &dst_desc,
                                      cache_tag,
                                        map_ctx.dynamic_sizes2   [checksum],
                                          map_ctx.dynamic_times2 [checksum],
                                            top_crc32,
                              L"", nullptr, (HMODULE)(intptr_t)-1/*SK_GetCallingDLL ()*/,
                                              pTLS );

          map_ctx.dynamic_textures.erase  (pSrcResource);
          map_ctx.dynamic_texturesx.erase (pSrcResource);

          map_ctx.dynamic_sizes2.erase    (checksum);
          map_ctx.dynamic_times2.erase    (checksum);
        }
      }
    }
  }
  
  void STDMETHODCALLTYPE CopyResource (
    _In_  ID3D11Resource *pDstResource,
    _In_  ID3D11Resource *pSrcResource) override
  {
    // UB: If it's happening, pretend we never saw this...
    if (pDstResource == nullptr || pSrcResource == nullptr)
    {
      return;
    }

    static auto& textures =
      SK_D3D11_Textures;

    SK_TLS *pTLS =
      SK_TLS_Bottom ();

    if (!pTLS->imgui.drawing)
    {
      if (! (pReal->GetType () == D3D11_DEVICE_CONTEXT_DEFERRED && (! config.render.dxgi.deferred_isolation)))
        SK_ReShade_CopyResourceCallback.call (pDstResource, pSrcResource, pTLS);
    }

    CComQIPtr <ID3D11Texture2D> pDstTex = pDstResource;

    if (pDstTex != nullptr)
    {
      if (! SK_D3D11_IsTexInjectThread (pTLS))
      {
        if (SK_D3D11_TextureIsCached (pDstTex))
        {
          //SK_LOG0 ( (L"Cached texture was modified (CopyResource)... removing from cache! - <%s>",
          //               SK_GetCallerName ().c_str ()), L"DX11TexMgr" );
          //SK_D3D11_RemoveTexFromCache (pDstTex, true);
        }
      }
    }


    // ImGui gets to pass-through without invoking the hook
    if ((! config.textures.cache.allow_staging) && (! SK_D3D11_ShouldTrackRenderOp (pReal, &pTLS)))
    {
      pReal->CopyResource (pDstResource, pSrcResource);
    
      return;
    }


    D3D11_RESOURCE_DIMENSION res_dim = { };
     pSrcResource->GetType (&res_dim);


    if (SK_D3D11_EnableMMIOTracking)
    {
      SK_D3D11_MemoryThreads.mark ();

      switch (res_dim)
      {
        case D3D11_RESOURCE_DIMENSION_UNKNOWN:
          mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_UNKNOWN]++;
          break;
        case D3D11_RESOURCE_DIMENSION_BUFFER:
        {
          mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_BUFFER]++;
      
          ID3D11Buffer* pBuffer = nullptr;
      
          if (SUCCEEDED (pSrcResource->QueryInterface <ID3D11Buffer> (&pBuffer)))
          {
            D3D11_BUFFER_DESC  buf_desc = { };
            pBuffer->GetDesc (&buf_desc);
            {
              if (buf_desc.BindFlags & D3D11_BIND_INDEX_BUFFER)
                mem_map_stats.last_frame.index_buffers.insert (pBuffer);

              if (buf_desc.BindFlags & D3D11_BIND_VERTEX_BUFFER)
                mem_map_stats.last_frame.vertex_buffers.insert (pBuffer);

              if (buf_desc.BindFlags & D3D11_BIND_CONSTANT_BUFFER)
                mem_map_stats.last_frame.constant_buffers.insert (pBuffer);
            }

            mem_map_stats.last_frame.bytes_copied += buf_desc.ByteWidth;

            pBuffer->Release ();
          }
        } break;
        case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
          mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_TEXTURE1D]++;
          break;
        case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
          mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_TEXTURE2D]++;
          break;
        case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
          mem_map_stats.last_frame.resource_types [D3D11_RESOURCE_DIMENSION_TEXTURE3D]++;
          break;
      }
    }

    pReal->CopyResource (pDstResource, pSrcResource);


    if ( SK_D3D11_IsStagingCacheable (res_dim, pSrcResource) ||
         SK_D3D11_IsStagingCacheable (res_dim, pDstResource) )
    {
      auto&& map_ctx = mapped_resources [pReal];

      CComQIPtr <ID3D11Texture2D> pSrcTex (pSrcResource);

      if (pSrcTex != nullptr)
      {
        if (SK_D3D11_TextureIsCached (pSrcTex))
        {
          dll_log.Log (L"Copying from cached source with checksum: %x", SK_D3D11_TextureHashFromCache (pSrcTex));
        }
      }

      if (pDstTex != nullptr && map_ctx.dynamic_textures.count (pSrcResource))
      {
        const uint32_t top_crc32 = map_ctx.dynamic_texturesx [pSrcResource];
        const uint32_t checksum  = map_ctx.dynamic_textures  [pSrcResource];

        D3D11_TEXTURE2D_DESC dst_desc = { };
          pDstTex->GetDesc (&dst_desc);

        const uint32_t cache_tag =
          safe_crc32c (top_crc32, (uint8_t *)(&dst_desc), sizeof D3D11_TEXTURE2D_DESC);

        if (checksum != 0x00 && dst_desc.Usage != D3D11_USAGE_STAGING)
        {
          textures.CacheMisses_2D++;

          textures.refTexture2D ( pDstTex,
                                    &dst_desc,
                                      cache_tag,
                                        map_ctx.dynamic_sizes2   [checksum],
                                          map_ctx.dynamic_times2 [checksum],
                                            top_crc32,
                               L"", nullptr, (HMODULE)(intptr_t)-1/*SK_GetCallingDLL ()*/,
                                              pTLS );
          map_ctx.dynamic_textures.erase  (pSrcResource);
          map_ctx.dynamic_texturesx.erase (pSrcResource);

          map_ctx.dynamic_sizes2.erase    (checksum);
          map_ctx.dynamic_times2.erase    (checksum);
        }
      }
    }
  }
  
  void STDMETHODCALLTYPE UpdateSubresource (
    _In_            ID3D11Resource *pDstResource,
    _In_            UINT            DstSubresource,
    _In_opt_  const D3D11_BOX      *pDstBox,
    _In_      const void           *pSrcData,
    _In_            UINT            SrcRowPitch,
    _In_            UINT            SrcDepthPitch) override
  {
    bool early_out = false;

    static auto& textures =
      SK_D3D11_Textures;

    SK_TLS *pTLS = nullptr;

    early_out =
      (! SK_D3D11_ShouldTrackRenderOp (pReal, &pTLS));

    if ( early_out ||

         // UB: If it's happening, pretend we never saw this...
         pDstResource == nullptr || pSrcData == nullptr
       )
    {
      return
        pReal->UpdateSubresource ( pDstResource, DstSubresource,
                                     pDstBox, pSrcData, SrcRowPitch,
                                       SrcDepthPitch );
    }

    //if (SK_GetCurrentGameID() == SK_GAME_ID::Ys_Eight)
    //{
    //  dll_log.Log ( L"UpdateSubresource:  { [%p] <RowPitch: %5lu> } -> { %s [%lu] }",
    //                pSrcData, SrcRowPitch, DescribeResource (pDstResource).c_str  (), DstSubresource );
    //}

    //dll_log.Log (L"[   DXGI   ] [!]D3D11_UpdateSubresource (%ph, %lu, %ph, %ph, %lu, %lu)",
    //          pDstResource, DstSubresource, pDstBox, pSrcData, SrcRowPitch, SrcDepthPitch);

    D3D11_RESOURCE_DIMENSION rdim = D3D11_RESOURCE_DIMENSION_UNKNOWN;
    pDstResource->GetType  (&rdim);

    if (SK_D3D11_IsStagingCacheable (rdim, pDstResource) && DstSubresource == 0)
    {
      CComQIPtr <ID3D11Texture2D> pTex (pDstResource);

      if (pTex != nullptr)
      {
        D3D11_TEXTURE2D_DESC desc = { };
             pTex->GetDesc (&desc);

        D3D11_SUBRESOURCE_DATA srd = { };

        srd.pSysMem           = pSrcData;
        srd.SysMemPitch       = SrcRowPitch;
        srd.SysMemSlicePitch  = 0;

        size_t   size         = 0;
        uint32_t top_crc32c   = 0x0;

        uint32_t checksum     =
          crc32_tex   (&desc, &srd, &size, &top_crc32c);

  const uint32_t cache_tag    =
          safe_crc32c (top_crc32c, (uint8_t *)(&desc), sizeof D3D11_TEXTURE2D_DESC);

        const auto start      = SK_QueryPerf ().QuadPart;

        CComPtr <ID3D11Texture2D> pCachedTex =
          textures.getTexture2D (cache_tag, &desc);

        if (pCachedTex != nullptr)
        {
          CComQIPtr <ID3D11Resource> pCachedResource (pCachedTex);

          pReal->CopyResource (pDstResource, pCachedResource);

          SK_LOG1 ( ( L"Texture Cache Hit (Slow Path): (%lux%lu) -- %x",
                        desc.Width, desc.Height, top_crc32c ),
                      L"DX11TexMgr" );

          return;
        }

        else
        {
          if (SK_D3D11_TextureIsCached (pTex))
          {
            SK_LOG0 ( (L"Cached texture was updated (UpdateSubresource)... removing from cache! - <%s>",
                           SK_GetCallerName ().c_str ()), L"DX11TexMgr" );
            SK_D3D11_RemoveTexFromCache (pTex, true);
          }

          pReal->UpdateSubresource( pDstResource, DstSubresource,
                                     pDstBox, pSrcData, SrcRowPitch,
                                       SrcDepthPitch );
          const auto end     = SK_QueryPerf ().QuadPart;
                auto elapsed = end - start;

          if (desc.Usage == D3D11_USAGE_STAGING)
          {
            auto&& map_ctx = mapped_resources [pReal];

            map_ctx.dynamic_textures  [pDstResource] = checksum;
            map_ctx.dynamic_texturesx [pDstResource] = top_crc32c;

            SK_LOG1 ( ( L"New Staged Texture: (%lux%lu) -- %x",
                          desc.Width, desc.Height, top_crc32c ),
                        L"DX11TexMgr" );

            map_ctx.dynamic_times2    [checksum]  = elapsed;
            map_ctx.dynamic_sizes2    [checksum]  = size;

            return;
          }

          else
          {
            bool cacheable = ( desc.MiscFlags <= 4 &&
                               desc.Width      > 0 && 
                               desc.Height     > 0 &&
                               desc.ArraySize == 1 //||
                             //((desc.ArraySize  % 6 == 0) && (desc.MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE))
                             );

            bool compressed = false;

            if ( (desc.Format >= DXGI_FORMAT_BC1_TYPELESS  &&
                  desc.Format <= DXGI_FORMAT_BC5_SNORM)    ||
                 (desc.Format >= DXGI_FORMAT_BC6H_TYPELESS &&
                  desc.Format <= DXGI_FORMAT_BC7_UNORM_SRGB) )
            {
              compressed = true;
            }

            // If this isn't an injectable texture, then filter out non-mipmapped
            //   textures.
            if (/*(! injectable) && */cache_opts.ignore_non_mipped)
              cacheable &= (desc.MipLevels > 1 || compressed);

            if (cacheable)
            {
              SK_LOG1 ( ( L"New Cacheable Texture: (%lux%lu) -- %x",
                            desc.Width, desc.Height, top_crc32c ),
                          L"DX11TexMgr" );

              textures.CacheMisses_2D++;
              textures.refTexture2D (pTex, &desc, cache_tag, size, elapsed, top_crc32c,
                                     L"", nullptr, (HMODULE)(intptr_t)-1/*SK_GetCallingDLL ()*/, pTLS);

              return;
            }
          }
        }
      }
    }

    return pReal->UpdateSubresource ( pDstResource, DstSubresource,
                                        pDstBox, pSrcData, SrcRowPitch,
                                          SrcDepthPitch );
  }
  
  void STDMETHODCALLTYPE CopyStructureCount (

    _In_  ID3D11Buffer *pDstBuffer,

    _In_  UINT DstAlignedByteOffset,

    _In_  ID3D11UnorderedAccessView *pSrcView) override
  {
    return pReal->CopyStructureCount (pDstBuffer, DstAlignedByteOffset, pSrcView);
  }
  
  void STDMETHODCALLTYPE ClearRenderTargetView (

    _In_  ID3D11RenderTargetView *pRenderTargetView,

    _In_  const FLOAT ColorRGBA [4]) override
  {
    return pReal->ClearRenderTargetView (pRenderTargetView, ColorRGBA);
  }
  
  void STDMETHODCALLTYPE ClearUnorderedAccessViewUint (

    _In_  ID3D11UnorderedAccessView *pUnorderedAccessView,

    _In_  const UINT Values [4]) override
  {
    return pReal->ClearUnorderedAccessViewUint (pUnorderedAccessView, Values);
  }
  
  void STDMETHODCALLTYPE ClearUnorderedAccessViewFloat (

    _In_  ID3D11UnorderedAccessView *pUnorderedAccessView,

    _In_  const FLOAT Values [4]) override
  {
    return pReal->ClearUnorderedAccessViewFloat (pUnorderedAccessView, Values);
  }
  
  void STDMETHODCALLTYPE ClearDepthStencilView (
    _In_  ID3D11DepthStencilView *pDepthStencilView,
    _In_  UINT ClearFlags,
    _In_  FLOAT Depth,
    _In_  UINT8 Stencil) override
  {
    SK_ReShade_ClearDepthStencilViewCallback.try_call (pDepthStencilView);

    pReal->ClearDepthStencilView (pDepthStencilView, ClearFlags, Depth, Stencil);

    return pReal->ClearDepthStencilView (pDepthStencilView, ClearFlags, Depth, Stencil);
  }
  
  void STDMETHODCALLTYPE GenerateMips (

    _In_  ID3D11ShaderResourceView *pShaderResourceView) override
  {
    return pReal->GenerateMips (pShaderResourceView);
  }
  
  void STDMETHODCALLTYPE SetResourceMinLOD (

    _In_  ID3D11Resource *pResource,
    FLOAT MinLOD) override
  {
    return pReal->SetResourceMinLOD (pResource, MinLOD);
  }
  
  FLOAT STDMETHODCALLTYPE GetResourceMinLOD (

    _In_  ID3D11Resource *pResource) override
  {
    return pReal->GetResourceMinLOD (pResource);
  }
  
  void STDMETHODCALLTYPE ResolveSubresource (

    _In_  ID3D11Resource *pDstResource,

    _In_  UINT DstSubresource,

    _In_  ID3D11Resource *pSrcResource,

    _In_  UINT SrcSubresource,

    _In_  DXGI_FORMAT Format) override
  {
    return pReal->ResolveSubresource (pDstResource, DstSubresource, pSrcResource, SrcSubresource, Format);
  }
  
  void STDMETHODCALLTYPE ExecuteCommandList (

    _In_  ID3D11CommandList *pCommandList,
    BOOL RestoreContextState) override
  {
    // Fix for Yakuza0, why the hell is it passing nullptr?!
    if (pCommandList == nullptr)
      return pReal->ExecuteCommandList (pCommandList, RestoreContextState);

    CComPtr <ID3D11DeviceContext> pBuildContext;
    UINT                          size = 0;

    if ( SUCCEEDED ( pCommandList->GetPrivateData (
                       SKID_D3D11DeviceContextOrigin,
                         &size, &pBuildContext )
                   )    &&    (! pBuildContext.IsEqualObject (this) )
       )
    {
      SK_D3D11_MergeCommandLists ( pBuildContext,        this    );

      pBuildContext->SetPrivateData ( SKID_D3D11DeviceContextOrigin,
                                        sizeof (ptrdiff_t), nullptr );

    }

    pReal->ExecuteCommandList  (pCommandList, RestoreContextState);
    SK_D3D11_ResetContextState (pBuildContext);

    if (! RestoreContextState)
    {
      SK_D3D11_ResetContextState (pReal);
    }
  }
  
  void STDMETHODCALLTYPE HSSetShaderResources (
    _In_range_ (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
    _In_range_ (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumViews,
    _In_reads_opt_ (NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews) override
  {
    static auto& shaders =
      SK_D3D11_Shaders;

    constexpr int HULL_SHADER_STAGE = 3;

    SK_TLS *pTLS = nullptr;

    // ImGui gets to pass-through without invoking the hook
    if ( (! SK_D3D11_ShouldTrackSetShaderResources (pReal, &pTLS)) ||
         (! SK_D3D11_ShouldTrackRenderOp           (pReal, &pTLS)) )
      return pReal->HSSetShaderResources (StartSlot, NumViews, ppShaderResourceViews);

    auto&& newResourceViews = pTLS->d3d11.newResourceViews;

    RtlZeroMemory (newResourceViews, sizeof (ID3D11ShaderResourceView*) * NumViews);

    if (ppShaderResourceViews && NumViews > 0)
    {
      UINT dev_idx =
        SK_D3D11_GetDeviceContextHandle (pReal);

      auto&& views = shaders.hull.current.views[dev_idx];
      auto&& stage =
        d3d11_shader_stages [HULL_SHADER_STAGE][dev_idx];

      d3d11_shader_tracking_s& tracked =
        shaders.hull.tracked;

      uint32_t shader_crc32c = tracked.crc32c.load ();
      bool     active        = tracked.active.get  (dev_idx);

      for (UINT i = 0; i < NumViews; i++)
      {
        if (SK_D3D11_ActivateSRVOnSlot (dev_idx, stage,
                                            ppShaderResourceViews [i], StartSlot + i))
          newResourceViews [i] =            ppShaderResourceViews [i];
        else
          newResourceViews [i] = nullptr;
      }

      if (active && shader_crc32c != 0)
      {
        for (UINT i = 0; i < NumViews; i++)
        {
          if (ppShaderResourceViews [i])                                            {
            std::lock_guard <SK_Thread_CriticalSection> auto_lock (*cs_shader_hs);
            if (tracked.set_of_views.emplace (ppShaderResourceViews [i]).second)    {
                                              ppShaderResourceViews [i]->AddRef (); }
             tracked.used_views.emplace_back (ppShaderResourceViews [i]);           }
                      views [StartSlot + i] = ppShaderResourceViews [i];
        }
      }
    }

    pReal->HSSetShaderResources (StartSlot, NumViews, newResourceViews);
  }
  
  void STDMETHODCALLTYPE HSSetShader (
    _In_opt_  ID3D11HullShader *pHullShader,
    _In_reads_opt_ (NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
    UINT NumClassInstances) override
  {
    return
      SK_D3D11_SetShader_Impl ( pReal, pHullShader,
                                  sk_shader_class::Hull,
                                    ppClassInstances, NumClassInstances );
  }
  
  void STDMETHODCALLTYPE HSSetSamplers (

    _In_range_ (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,

    _In_range_ (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,

    _In_reads_opt_ (NumSamplers)  ID3D11SamplerState *const *ppSamplers) override
  {
    return pReal->HSSetSamplers (StartSlot, NumSamplers, ppSamplers);
  }
  
  void STDMETHODCALLTYPE HSSetConstantBuffers (

    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,

    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,

    _In_reads_opt_ (NumBuffers)  ID3D11Buffer *const *ppConstantBuffers) override
  {
    return pReal->HSSetConstantBuffers (StartSlot, NumBuffers, ppConstantBuffers);
  }
  
  void STDMETHODCALLTYPE DSSetShaderResources (
    _In_range_ (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
    _In_range_ (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumViews,
    _In_reads_opt_ (NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews) override
  {
    static auto& shaders =
      SK_D3D11_Shaders;

    constexpr int DOMAIN_SHADER_STAGE = 4;

    SK_TLS *pTLS = nullptr;

    // ImGui gets to pass-through without invoking the hook
    if ( (! SK_D3D11_ShouldTrackSetShaderResources (pReal, &pTLS)) ||
         (! SK_D3D11_ShouldTrackRenderOp           (pReal, &pTLS)) )
      return pReal->DSSetShaderResources (StartSlot, NumViews, ppShaderResourceViews);

    auto&& newResourceViews = pTLS->d3d11.newResourceViews;

    RtlZeroMemory (newResourceViews, sizeof (ID3D11ShaderResourceView*) * NumViews);

    if (ppShaderResourceViews && NumViews > 0)
    {
      UINT dev_idx =
        SK_D3D11_GetDeviceContextHandle (pReal);

      auto&& views = shaders.domain.current.views[dev_idx];
      auto&& stage =
        d3d11_shader_stages [DOMAIN_SHADER_STAGE][dev_idx];

      d3d11_shader_tracking_s& tracked =
        shaders.domain.tracked;

      uint32_t shader_crc32c = tracked.crc32c.load ();
      bool     active        = tracked.active.get  (dev_idx);

      for (UINT i = 0; i < NumViews; i++)
      {
        if (SK_D3D11_ActivateSRVOnSlot (dev_idx, stage,
                                            ppShaderResourceViews [i], StartSlot + i))
          newResourceViews [i] =            ppShaderResourceViews [i];
        else
          newResourceViews [i] = nullptr;
      }

      if (active && shader_crc32c != 0)
      {
        for (UINT i = 0; i < NumViews; i++)
        {
          if (ppShaderResourceViews [i])                                            {
            std::lock_guard <SK_Thread_CriticalSection> auto_lock (*cs_shader_ds);
            if (tracked.set_of_views.emplace (ppShaderResourceViews [i]).second)    {
                                              ppShaderResourceViews [i]->AddRef (); }
                tracked.used_views.push_back (ppShaderResourceViews [i]);           }
                      views [StartSlot + i] = ppShaderResourceViews [i];
        }
      }
    }

    return pReal->DSSetShaderResources (StartSlot, NumViews, newResourceViews);
  }
  
  void STDMETHODCALLTYPE DSSetShader (
    _In_opt_  ID3D11DomainShader *pDomainShader,
    _In_reads_opt_ (NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
    UINT NumClassInstances) override
  {
    return
      SK_D3D11_SetShader_Impl ( pReal, pDomainShader,
                                  sk_shader_class::Domain,
                                    ppClassInstances, NumClassInstances );
  }
  
  void STDMETHODCALLTYPE DSSetSamplers (

    _In_range_ (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,

    _In_range_ (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,

    _In_reads_opt_ (NumSamplers)  ID3D11SamplerState *const *ppSamplers) override
  {
    return pReal->DSSetSamplers (StartSlot, NumSamplers, ppSamplers);
  }
  
  void STDMETHODCALLTYPE DSSetConstantBuffers (

    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,

    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,

    _In_reads_opt_ (NumBuffers)  ID3D11Buffer *const *ppConstantBuffers) override
  {
    return pReal->DSSetConstantBuffers (StartSlot, NumBuffers, ppConstantBuffers);
  }
  
  void STDMETHODCALLTYPE CSSetShaderResources (
    _In_range_ (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,
    _In_range_ (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumViews,
    _In_reads_opt_ (NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews) override
  {
    static auto& shaders =
      SK_D3D11_Shaders;

    constexpr int COMPUTE_SHADER_STAGE = 5;

    SK_TLS *pTLS = nullptr;

    // ImGui gets to pass-through without invoking the hook
    if ( (! SK_D3D11_ShouldTrackSetShaderResources (pReal, &pTLS)) ||
         (! SK_D3D11_ShouldTrackRenderOp           (pReal, &pTLS)) )
      return pReal->CSSetShaderResources (StartSlot, NumViews, ppShaderResourceViews);

    auto&& newResourceViews = pTLS->d3d11.newResourceViews;

    RtlZeroMemory (newResourceViews, sizeof (ID3D11ShaderResourceView*) * NumViews);

    if (ppShaderResourceViews && NumViews > 0)
    {
      const UINT dev_idx =
        SK_D3D11_GetDeviceContextHandle (pReal);

      auto&& views = shaders.compute.current.views[dev_idx];
      auto&& stage =
        d3d11_shader_stages [COMPUTE_SHADER_STAGE][dev_idx];

      d3d11_shader_tracking_s& tracked =
        shaders.compute.tracked;

      const uint32_t shader_crc32c = tracked.crc32c.load ();
      const bool     active        = tracked.active.get  (dev_idx);

      for (UINT i = 0; i < NumViews; i++)
      {
        if (SK_D3D11_ActivateSRVOnSlot (dev_idx, stage,
                                            ppShaderResourceViews [i], StartSlot + i))
          newResourceViews [i] =            ppShaderResourceViews [i];
        else
          newResourceViews [i] = nullptr;
      }

      if (active && shader_crc32c != 0)
      {
        for (UINT i = 0; i < NumViews; i++)
        {
          if (ppShaderResourceViews [i])                                            {
            std::lock_guard <SK_Thread_CriticalSection> auto_lock (*cs_shader_cs);
            if (tracked.set_of_views.emplace (ppShaderResourceViews [i]).second)    {
                                              ppShaderResourceViews [i]->AddRef (); }
             tracked.used_views.emplace_back (ppShaderResourceViews [i]);           }
                      views [StartSlot + i] = ppShaderResourceViews [i];
        }
      }
    }

    pReal->CSSetShaderResources (StartSlot, NumViews, newResourceViews);
  }
  
  void STDMETHODCALLTYPE CSSetUnorderedAccessViews( 
 
      _In_range_( 0, D3D11_1_UAV_SLOT_COUNT - 1 )  UINT StartSlot,
 
      _In_range_( 0, D3D11_1_UAV_SLOT_COUNT - StartSlot )  UINT NumUAVs,
 
      _In_reads_opt_(NumUAVs)  ID3D11UnorderedAccessView *const *ppUnorderedAccessViews,
 
      _In_reads_opt_(NumUAVs)  const UINT *pUAVInitialCounts) override {
    return pReal->CSSetUnorderedAccessViews (StartSlot, NumUAVs, ppUnorderedAccessViews, pUAVInitialCounts);
  }
  
  void STDMETHODCALLTYPE CSSetShader (
    _In_opt_  ID3D11ComputeShader *pComputeShader,
    _In_reads_opt_ (NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances,
    UINT NumClassInstances)
  {
    return
      SK_D3D11_SetShader_Impl ( pReal, pComputeShader,
                                  sk_shader_class::Compute,
                                    ppClassInstances, NumClassInstances );
  }
  
  void STDMETHODCALLTYPE CSSetSamplers (

    _In_range_ (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,

    _In_range_ (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,

    _In_reads_opt_ (NumSamplers)  ID3D11SamplerState *const *ppSamplers) override
  {
    return pReal->CSSetSamplers (StartSlot, NumSamplers, ppSamplers);
  }
  
  void STDMETHODCALLTYPE CSSetConstantBuffers (

    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,

    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,

    _In_reads_opt_ (NumBuffers)  ID3D11Buffer *const *ppConstantBuffers) override
  {
    return pReal->CSSetConstantBuffers (StartSlot, NumBuffers, ppConstantBuffers);
  }
  
  void STDMETHODCALLTYPE VSGetConstantBuffers (

    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,

    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,

    _Out_writes_opt_ (NumBuffers)  ID3D11Buffer **ppConstantBuffers) override
  {
    return pReal->VSGetConstantBuffers (StartSlot, NumBuffers, ppConstantBuffers);
  }
  
  void STDMETHODCALLTYPE PSGetShaderResources (

    _In_range_ (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,

    _In_range_ (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumViews,

    _Out_writes_opt_ (NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews) override
  {
    return pReal->PSGetShaderResources (StartSlot, NumViews, ppShaderResourceViews);
  }
  
  void STDMETHODCALLTYPE PSGetShader (

    _Out_  ID3D11PixelShader **ppPixelShader,

    _Out_writes_opt_ (*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,

    _Inout_opt_  UINT *pNumClassInstances) override
  {
    return pReal->PSGetShader (ppPixelShader, ppClassInstances, pNumClassInstances);
  }
  
  void STDMETHODCALLTYPE PSGetSamplers (

    _In_range_ (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,

    _In_range_ (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,

    _Out_writes_opt_ (NumSamplers)  ID3D11SamplerState **ppSamplers) override
  {
    return pReal->PSGetSamplers (StartSlot, NumSamplers, ppSamplers);
  }
  
  void STDMETHODCALLTYPE VSGetShader (

    _Out_  ID3D11VertexShader **ppVertexShader,

    _Out_writes_opt_ (*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,

    _Inout_opt_  UINT *pNumClassInstances) override
  {
    return pReal->VSGetShader (ppVertexShader, ppClassInstances, pNumClassInstances);
  }
  
  void STDMETHODCALLTYPE PSGetConstantBuffers (

    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,

    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,

    _Out_writes_opt_ (NumBuffers)  ID3D11Buffer **ppConstantBuffers) override
  {
    return pReal->PSGetConstantBuffers (StartSlot, NumBuffers, ppConstantBuffers);
  }
  
  void STDMETHODCALLTYPE IAGetInputLayout (

    _Out_  ID3D11InputLayout **ppInputLayout) override
  {
    return pReal->IAGetInputLayout (ppInputLayout);
  }
  
  void STDMETHODCALLTYPE IAGetVertexBuffers (

    _In_range_ (0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,

    _In_range_ (0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumBuffers,

    _Out_writes_opt_ (NumBuffers)  ID3D11Buffer **ppVertexBuffers,

    _Out_writes_opt_ (NumBuffers)  UINT *pStrides,

    _Out_writes_opt_ (NumBuffers)  UINT *pOffsets) override
  {
    return pReal->IAGetVertexBuffers (StartSlot, NumBuffers, ppVertexBuffers, pStrides, pOffsets);
  }
  
  void STDMETHODCALLTYPE IAGetIndexBuffer (

    _Out_opt_  ID3D11Buffer **pIndexBuffer,

    _Out_opt_  DXGI_FORMAT *Format,

    _Out_opt_  UINT *Offset) override
  {
    return pReal->IAGetIndexBuffer (pIndexBuffer, Format, Offset);
  }
  
  void STDMETHODCALLTYPE GSGetConstantBuffers( 
 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
 
      _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
 
      _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers) override
  {
    return pReal->GSGetConstantBuffers (StartSlot, NumBuffers, ppConstantBuffers);
  }
  
  void STDMETHODCALLTYPE GSGetShader (

    _Out_  ID3D11GeometryShader **ppGeometryShader,

    _Out_writes_opt_ (*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,

    _Inout_opt_  UINT *pNumClassInstances) override
  {
    return pReal->GSGetShader (ppGeometryShader, ppClassInstances, pNumClassInstances);
  }
  
  void STDMETHODCALLTYPE IAGetPrimitiveTopology (

    _Out_  D3D11_PRIMITIVE_TOPOLOGY *pTopology) override
  {
    return pReal->IAGetPrimitiveTopology (pTopology);
  }
  
  void STDMETHODCALLTYPE VSGetShaderResources (

    _In_range_ (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,

    _In_range_ (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumViews,

    _Out_writes_opt_ (NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews) override
  {
    return pReal->VSGetShaderResources (StartSlot, NumViews, ppShaderResourceViews);
  }
  
  void STDMETHODCALLTYPE VSGetSamplers( 
 
      _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1 )  UINT StartSlot,
 
      _In_range_( 0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot )  UINT NumSamplers,
 
      _Out_writes_opt_(NumSamplers)  ID3D11SamplerState **ppSamplers) override
  {
    return pReal->VSGetSamplers (StartSlot, NumSamplers, ppSamplers);
  }
  
  void STDMETHODCALLTYPE GetPredication (

    _Out_opt_  ID3D11Predicate **ppPredicate,

    _Out_opt_  BOOL *pPredicateValue) override
  {
    return pReal->GetPredication (ppPredicate, pPredicateValue);
  }
  
  void STDMETHODCALLTYPE GSGetShaderResources( 
 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
 
      _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews) override
  {
    return pReal->GSGetShaderResources (StartSlot, NumViews, ppShaderResourceViews);
  }
  
  void STDMETHODCALLTYPE GSGetSamplers (

    _In_range_ (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,

    _In_range_ (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,

    _Out_writes_opt_ (NumSamplers)  ID3D11SamplerState **ppSamplers) override
  {
    return pReal->GSGetSamplers (StartSlot, NumSamplers, ppSamplers);
  }
  
  void STDMETHODCALLTYPE OMGetRenderTargets (
    _In_range_ (0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT)  UINT NumViews,
    _Out_writes_opt_ (NumViews)  ID3D11RenderTargetView **ppRenderTargetViews,
    _Out_opt_  ID3D11DepthStencilView **ppDepthStencilView) override
  {
    pReal->OMGetRenderTargets (NumViews, ppRenderTargetViews, ppDepthStencilView);

    if (ppDepthStencilView != nullptr)
      SK_ReShade_GetDepthStencilViewCallback.try_call (*ppDepthStencilView);
  }
  
  void STDMETHODCALLTYPE OMGetRenderTargetsAndUnorderedAccessViews (
    _In_range_ (0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT)  UINT NumRTVs,
    _Out_writes_opt_ (NumRTVs)  ID3D11RenderTargetView **ppRenderTargetViews,
    _Out_opt_  ID3D11DepthStencilView **ppDepthStencilView,
    _In_range_ (0, D3D11_PS_CS_UAV_REGISTER_COUNT - 1)  UINT UAVStartSlot,
    _In_range_ (0, D3D11_PS_CS_UAV_REGISTER_COUNT - UAVStartSlot)  UINT NumUAVs,
    _Out_writes_opt_ (NumUAVs)  ID3D11UnorderedAccessView **ppUnorderedAccessViews) override
  {
    pReal->OMGetRenderTargetsAndUnorderedAccessViews (NumRTVs, ppRenderTargetViews, ppDepthStencilView, UAVStartSlot, NumUAVs, ppUnorderedAccessViews);

    if (ppDepthStencilView != nullptr)
      SK_ReShade_GetDepthStencilViewCallback.try_call (*ppDepthStencilView);
  }
  
  void STDMETHODCALLTYPE OMGetBlendState (

    _Out_opt_  ID3D11BlendState **ppBlendState,

    _Out_opt_  FLOAT BlendFactor [4],

    _Out_opt_  UINT *pSampleMask) override
  {
    return pReal->OMGetBlendState (ppBlendState, BlendFactor, pSampleMask);
  }
  
  void STDMETHODCALLTYPE OMGetDepthStencilState( 
 
      _Out_opt_  ID3D11DepthStencilState **ppDepthStencilState,
 
      _Out_opt_  UINT *pStencilRef) override
  {
    return pReal->OMGetDepthStencilState (ppDepthStencilState, pStencilRef);
  }
  
  void STDMETHODCALLTYPE SOGetTargets( 
 
      _In_range_( 0, D3D11_SO_BUFFER_SLOT_COUNT )  UINT NumBuffers,
 
      _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppSOTargets) override
  {
    return pReal->SOGetTargets (NumBuffers, ppSOTargets);
  }
  
  void STDMETHODCALLTYPE RSGetState( 
 
      _Out_  ID3D11RasterizerState **ppRasterizerState) override
  {
    return pReal->RSGetState (ppRasterizerState);
  }
  
  void STDMETHODCALLTYPE RSGetViewports( 
 
      _Inout_ /*_range(0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE )*/   UINT *pNumViewports,
 
      _Out_writes_opt_(*pNumViewports)  D3D11_VIEWPORT *pViewports) override
  {
    return pReal->RSGetViewports (pNumViewports, pViewports);
  }
  
  void STDMETHODCALLTYPE RSGetScissorRects (

    _Inout_ /*_range(0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE )*/   UINT *pNumRects,

    _Out_writes_opt_ (*pNumRects)  D3D11_RECT *pRects)
  {
    return pReal->RSGetScissorRects (pNumRects, pRects);
  }
  
  void STDMETHODCALLTYPE HSGetShaderResources( 
 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1 )  UINT StartSlot,
 
      _In_range_( 0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot )  UINT NumViews,
 
      _Out_writes_opt_(NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews) override
  {
    return pReal->HSGetShaderResources (StartSlot, NumViews, ppShaderResourceViews);
  }
  
  void STDMETHODCALLTYPE HSGetShader (

    _Out_  ID3D11HullShader **ppHullShader,

    _Out_writes_opt_ (*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,

    _Inout_opt_  UINT *pNumClassInstances) override
  {
    return pReal->HSGetShader (ppHullShader, ppClassInstances, pNumClassInstances);
  }
  
  void STDMETHODCALLTYPE HSGetSamplers (

    _In_range_ (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,

    _In_range_ (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,

    _Out_writes_opt_ (NumSamplers)  ID3D11SamplerState **ppSamplers) override
  {
    return pReal->HSGetSamplers (StartSlot, NumSamplers, ppSamplers);
  }
  
  void STDMETHODCALLTYPE HSGetConstantBuffers (

    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,

    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,

    _Out_writes_opt_ (NumBuffers)  ID3D11Buffer **ppConstantBuffers) override
  {
    return pReal->HSGetConstantBuffers (StartSlot, NumBuffers, ppConstantBuffers);
  }
  
  void STDMETHODCALLTYPE DSGetShaderResources (

    _In_range_ (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,

    _In_range_ (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumViews,

    _Out_writes_opt_ (NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews) override
  {
    return pReal->DSGetShaderResources (StartSlot, NumViews, ppShaderResourceViews);
  }
  
  void STDMETHODCALLTYPE DSGetShader (

    _Out_  ID3D11DomainShader **ppDomainShader,

    _Out_writes_opt_ (*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,

    _Inout_opt_  UINT *pNumClassInstances) override
  {
    return pReal->DSGetShader (ppDomainShader, ppClassInstances, pNumClassInstances);
  }
  
  void STDMETHODCALLTYPE DSGetSamplers (

    _In_range_ (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,

    _In_range_ (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,

    _Out_writes_opt_ (NumSamplers)  ID3D11SamplerState **ppSamplers) override
  {
    return pReal->DSGetSamplers (StartSlot, NumSamplers, ppSamplers);
  }
  
  void STDMETHODCALLTYPE DSGetConstantBuffers (

    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,

    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,

    _Out_writes_opt_ (NumBuffers)  ID3D11Buffer **ppConstantBuffers) override
  {
    return pReal->DSGetConstantBuffers (StartSlot, NumBuffers, ppConstantBuffers);
  }
  
  void STDMETHODCALLTYPE CSGetShaderResources (

    _In_range_ (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  UINT StartSlot,

    _In_range_ (0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  UINT NumViews,

    _Out_writes_opt_ (NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews) override
  {
    return pReal->CSGetShaderResources (StartSlot, NumViews, ppShaderResourceViews);
  }
  
  void STDMETHODCALLTYPE CSGetUnorderedAccessViews (

    _In_range_ (0, D3D11_PS_CS_UAV_REGISTER_COUNT - 1)  UINT StartSlot,

    _In_range_ (0, D3D11_PS_CS_UAV_REGISTER_COUNT - StartSlot)  UINT NumUAVs,

    _Out_writes_opt_ (NumUAVs)  ID3D11UnorderedAccessView **ppUnorderedAccessViews) override
  {
    return pReal->CSGetUnorderedAccessViews (StartSlot, NumUAVs, ppUnorderedAccessViews);
  }
  
  void STDMETHODCALLTYPE CSGetShader (

    _Out_  ID3D11ComputeShader **ppComputeShader,

    _Out_writes_opt_ (*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances,

    _Inout_opt_  UINT *pNumClassInstances) override
  {
    return pReal->CSGetShader (ppComputeShader, ppClassInstances, pNumClassInstances);
  }
  
  void STDMETHODCALLTYPE CSGetSamplers (

    _In_range_ (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  UINT StartSlot,

    _In_range_ (0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  UINT NumSamplers,

    _Out_writes_opt_ (NumSamplers)  ID3D11SamplerState **ppSamplers) override
  {
    return pReal->CSGetSamplers (StartSlot, NumSamplers, ppSamplers);
  }
  
  void STDMETHODCALLTYPE CSGetConstantBuffers (

    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,

    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,

    _Out_writes_opt_ (NumBuffers)  ID3D11Buffer **ppConstantBuffers) override
  {
    return pReal->CSGetConstantBuffers (StartSlot, NumBuffers, ppConstantBuffers);
  }
  
  void STDMETHODCALLTYPE ClearState (void) override
  {
    pReal->ClearState ();

    SK_D3D11_ResetContextState (pReal);
  }
  
  void STDMETHODCALLTYPE Flush (void) override
  {
    return pReal->Flush ();
  }
  
  D3D11_DEVICE_CONTEXT_TYPE STDMETHODCALLTYPE GetType (void) override
  {
    return pReal->GetType ();
  }
  
  UINT STDMETHODCALLTYPE GetContextFlags (void) override
  {
    return pReal->GetContextFlags ();
  }
  
  HRESULT STDMETHODCALLTYPE FinishCommandList (
    BOOL RestoreDeferredContextState,

    _Out_opt_  ID3D11CommandList **ppCommandList) override
  {
    HRESULT hr =
      pReal->FinishCommandList (RestoreDeferredContextState, ppCommandList);

    //  Lord the documentation is contradictory ; assume that the way it is written,
    //    some kind of reset always happens. Even when "Restore" means Clear (WTF?)
    if (SUCCEEDED (hr) && (ppCommandList != nullptr))// && (! RestoreDeferredContextState))
    {
      (*ppCommandList)->SetPrivateData ( SKID_D3D11DeviceContextOrigin,
                                         sizeof (ptrdiff_t), this );

      //SK_D3D11_ResetContextState (pReal);
    }

    else
    {
      SK_D3D11_ResetContextState (pReal);
    }

    return hr;
  }




  void STDMETHODCALLTYPE CopySubresourceRegion1 (
    _In_  ID3D11Resource *pDstResource,
    _In_  UINT DstSubresource,
    _In_  UINT DstX,
    _In_  UINT DstY,
    _In_  UINT DstZ,
    _In_  ID3D11Resource *pSrcResource,
    _In_  UINT SrcSubresource,
    _In_opt_  const D3D11_BOX *pSrcBox,
    _In_  UINT CopyFlags) override
  {
    assert (ver_ >= 1);

    return
      static_cast <ID3D11DeviceContext1 *>(pReal)->CopySubresourceRegion1 (pDstResource, DstSubresource, DstX, DstY, DstZ, pSrcResource, SrcSubresource, pSrcBox, CopyFlags);
  }
  void STDMETHODCALLTYPE UpdateSubresource1 (
    _In_  ID3D11Resource *pDstResource,
    _In_  UINT DstSubresource,
    _In_opt_  const D3D11_BOX *pDstBox,
    _In_  const void *pSrcData,
    _In_  UINT SrcRowPitch,
    _In_  UINT SrcDepthPitch,
    _In_  UINT CopyFlags) override
  {
    assert (ver_ >= 1);

    return
      static_cast <ID3D11DeviceContext1 *>(pReal)->UpdateSubresource1 (pDstResource, DstSubresource, pDstBox, pSrcData, SrcRowPitch, SrcDepthPitch, CopyFlags);
  }
  void STDMETHODCALLTYPE DiscardResource (
    _In_  ID3D11Resource *pResource) override
  {
    assert (ver_ >= 1);

    return
      static_cast <ID3D11DeviceContext1 *>(pReal)->DiscardResource (pResource);
  }
  void STDMETHODCALLTYPE DiscardView (
    _In_  ID3D11View *pResourceView) override
  {
    assert (ver_ >= 1);

    return
      static_cast <ID3D11DeviceContext1 *>(pReal)->DiscardView (pResourceView);
  }
  void STDMETHODCALLTYPE VSSetConstantBuffers1 (
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
    _In_reads_opt_ (NumBuffers)  ID3D11Buffer *const *ppConstantBuffers,
    _In_reads_opt_ (NumBuffers)  const UINT *pFirstConstant,
    _In_reads_opt_ (NumBuffers)  const UINT *pNumConstants) override
  {
    assert (ver_ >= 1);

    return
      static_cast <ID3D11DeviceContext1 *>(pReal)->VSSetConstantBuffers1 (StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants);
  }
  void STDMETHODCALLTYPE HSSetConstantBuffers1 (
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
    _In_reads_opt_ (NumBuffers)  ID3D11Buffer *const *ppConstantBuffers,
    _In_reads_opt_ (NumBuffers)  const UINT *pFirstConstant,
    _In_reads_opt_ (NumBuffers)  const UINT *pNumConstants) override
  {
    assert (ver_ >= 1);

    return
      static_cast <ID3D11DeviceContext1 *>(pReal)->HSSetConstantBuffers1 (StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants);
  }
  void STDMETHODCALLTYPE DSSetConstantBuffers1 (
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
    _In_reads_opt_ (NumBuffers)  ID3D11Buffer *const *ppConstantBuffers,
    _In_reads_opt_ (NumBuffers)  const UINT *pFirstConstant,
    _In_reads_opt_ (NumBuffers)  const UINT *pNumConstants) override
  {
    assert (ver_ >= 1);

    return
      static_cast <ID3D11DeviceContext1 *>(pReal)->DSSetConstantBuffers1 (StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants);
  }
  void STDMETHODCALLTYPE GSSetConstantBuffers1 (
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
    _In_reads_opt_ (NumBuffers)  ID3D11Buffer *const *ppConstantBuffers,
    _In_reads_opt_ (NumBuffers)  const UINT *pFirstConstant,
    _In_reads_opt_ (NumBuffers)  const UINT *pNumConstants) override
  {
    assert (ver_ >= 1);

    return
      static_cast <ID3D11DeviceContext1 *>(pReal)->GSSetConstantBuffers1 (StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants);
  }
  void STDMETHODCALLTYPE PSSetConstantBuffers1 (
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
    _In_reads_opt_ (NumBuffers)  ID3D11Buffer *const *ppConstantBuffers,
    _In_reads_opt_ (NumBuffers)  const UINT *pFirstConstant,
    _In_reads_opt_ (NumBuffers)  const UINT *pNumConstants) override
  {
    assert (ver_ >= 1);

    return
      static_cast <ID3D11DeviceContext1 *>(pReal)->PSSetConstantBuffers1 (StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants);
  }
  void STDMETHODCALLTYPE CSSetConstantBuffers1 (
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
    _In_reads_opt_ (NumBuffers)  ID3D11Buffer *const *ppConstantBuffers,
    _In_reads_opt_ (NumBuffers)  const UINT *pFirstConstant,
    _In_reads_opt_ (NumBuffers)  const UINT *pNumConstants) override
  {
    assert (ver_ >= 1);

    return
      static_cast <ID3D11DeviceContext1 *>(pReal)->CSSetConstantBuffers1 (StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants);
  }
  void STDMETHODCALLTYPE VSGetConstantBuffers1 (
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
    _Out_writes_opt_ (NumBuffers)  ID3D11Buffer **ppConstantBuffers,
    _Out_writes_opt_ (NumBuffers)  UINT *pFirstConstant,
    _Out_writes_opt_ (NumBuffers)  UINT *pNumConstants) override
  {
    assert (ver_ >= 1);

    return
      static_cast <ID3D11DeviceContext1 *>(pReal)->VSGetConstantBuffers1 (StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants);
  }
  void STDMETHODCALLTYPE HSGetConstantBuffers1 (
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
    _Out_writes_opt_ (NumBuffers)  ID3D11Buffer **ppConstantBuffers,
    _Out_writes_opt_ (NumBuffers)  UINT *pFirstConstant,
    _Out_writes_opt_ (NumBuffers)  UINT *pNumConstants) override
  {
    assert (ver_ >= 1);

    return
      static_cast <ID3D11DeviceContext1 *>(pReal)->HSGetConstantBuffers1 (StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants);
  }
  void STDMETHODCALLTYPE DSGetConstantBuffers1( 
    _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1 )  UINT StartSlot,
    _In_range_( 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot )  UINT NumBuffers,
    _Out_writes_opt_(NumBuffers)  ID3D11Buffer **ppConstantBuffers,
    _Out_writes_opt_(NumBuffers)  UINT *pFirstConstant,
    _Out_writes_opt_(NumBuffers)  UINT *pNumConstants) override
  {
    assert (ver_ >= 1);

    return
      static_cast <ID3D11DeviceContext1 *>(pReal)->DSGetConstantBuffers1 (StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants);
  }
  void STDMETHODCALLTYPE GSGetConstantBuffers1 (
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
    _Out_writes_opt_ (NumBuffers)  ID3D11Buffer **ppConstantBuffers,
    _Out_writes_opt_ (NumBuffers)  UINT *pFirstConstant,
    _Out_writes_opt_ (NumBuffers)  UINT *pNumConstants) override
  {
    assert (ver_ >= 1);

    return
      static_cast <ID3D11DeviceContext1 *>(pReal)->GSGetConstantBuffers1 (StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants);
  }
  void STDMETHODCALLTYPE PSGetConstantBuffers1 (
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
    _Out_writes_opt_ (NumBuffers)  ID3D11Buffer **ppConstantBuffers,
    _Out_writes_opt_ (NumBuffers)  UINT *pFirstConstant,
    _Out_writes_opt_ (NumBuffers)  UINT *pNumConstants) override
  {
    assert (ver_ >= 1);

    return
      static_cast <ID3D11DeviceContext1 *>(pReal)->PSGetConstantBuffers1 (StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants);
  }
  void STDMETHODCALLTYPE CSGetConstantBuffers1 (
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  UINT StartSlot,
    _In_range_ (0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  UINT NumBuffers,
    _Out_writes_opt_ (NumBuffers)  ID3D11Buffer **ppConstantBuffers,
    _Out_writes_opt_ (NumBuffers)  UINT *pFirstConstant,
    _Out_writes_opt_ (NumBuffers)  UINT *pNumConstants) override
  {
    assert (ver_ >= 1);

    return
      static_cast <ID3D11DeviceContext1 *>(pReal)->CSGetConstantBuffers1 (StartSlot, NumBuffers, ppConstantBuffers, pFirstConstant, pNumConstants);
  }
  void STDMETHODCALLTYPE SwapDeviceContextState (
    _In_  ID3DDeviceContextState *pState,
    _Out_opt_  ID3DDeviceContextState **ppPreviousState) override
  {
    assert (ver_ >= 1);

    return
      static_cast <ID3D11DeviceContext1 *>(pReal)->SwapDeviceContextState (pState, ppPreviousState);
  }
  void STDMETHODCALLTYPE ClearView (
    _In_  ID3D11View *pView,
    _In_  const FLOAT Color [4],
    _In_reads_opt_ (NumRects)  const D3D11_RECT *pRect,
    UINT NumRects) override
  {
    assert (ver_ >= 1);

    return
      static_cast <ID3D11DeviceContext1 *>(pReal)->ClearView (pView, Color, pRect, NumRects);
  }
  void STDMETHODCALLTYPE DiscardView1 (
    _In_  ID3D11View *pResourceView,
    _In_reads_opt_ (NumRects)  const D3D11_RECT *pRects,
    UINT NumRects) override
  {
    assert (ver_ >= 1);

    return
      static_cast <ID3D11DeviceContext1 *>(pReal)->DiscardView1 (pResourceView, pRects, NumRects);
  }




  HRESULT STDMETHODCALLTYPE UpdateTileMappings (
    _In_  ID3D11Resource *pTiledResource,
    _In_  UINT NumTiledResourceRegions,
    _In_reads_opt_ (NumTiledResourceRegions)  const D3D11_TILED_RESOURCE_COORDINATE *pTiledResourceRegionStartCoordinates,
    _In_reads_opt_ (NumTiledResourceRegions)  const D3D11_TILE_REGION_SIZE *pTiledResourceRegionSizes,
    _In_opt_  ID3D11Buffer *pTilePool,
    _In_  UINT NumRanges,
    _In_reads_opt_ (NumRanges)  const UINT *pRangeFlags,
    _In_reads_opt_ (NumRanges)  const UINT *pTilePoolStartOffsets,
    _In_reads_opt_ (NumRanges)  const UINT *pRangeTileCounts,
    _In_  UINT Flags) override
  {
    assert (ver_ >= 2);

    return
      static_cast <ID3D11DeviceContext2 *>(pReal)->UpdateTileMappings (pTiledResource, NumTiledResourceRegions, pTiledResourceRegionStartCoordinates, pTiledResourceRegionSizes, pTilePool, NumRanges, pRangeFlags, pTilePoolStartOffsets, pRangeTileCounts, Flags);
  }
  HRESULT STDMETHODCALLTYPE CopyTileMappings (
    _In_  ID3D11Resource *pDestTiledResource,
    _In_  const D3D11_TILED_RESOURCE_COORDINATE *pDestRegionStartCoordinate,
    _In_  ID3D11Resource *pSourceTiledResource,
    _In_  const D3D11_TILED_RESOURCE_COORDINATE *pSourceRegionStartCoordinate,
    _In_  const D3D11_TILE_REGION_SIZE *pTileRegionSize,
    _In_  UINT Flags) override
  {
    assert (ver_ >= 2);

    return
      static_cast <ID3D11DeviceContext2 *>(pReal)->CopyTileMappings (pDestTiledResource, pDestRegionStartCoordinate, pSourceTiledResource, pSourceRegionStartCoordinate, pTileRegionSize, Flags);
  }
  void STDMETHODCALLTYPE CopyTiles (
    _In_  ID3D11Resource *pTiledResource,
    _In_  const D3D11_TILED_RESOURCE_COORDINATE *pTileRegionStartCoordinate,
    _In_  const D3D11_TILE_REGION_SIZE *pTileRegionSize,
    _In_  ID3D11Buffer *pBuffer,
    _In_  UINT64 BufferStartOffsetInBytes,
    _In_  UINT Flags) override
  {
    assert (ver_ >= 2);

    return
      static_cast <ID3D11DeviceContext2 *>(pReal)->CopyTiles (pTiledResource, pTileRegionStartCoordinate, pTileRegionSize, pBuffer, BufferStartOffsetInBytes, Flags);
  }
  void STDMETHODCALLTYPE UpdateTiles (
    _In_  ID3D11Resource *pDestTiledResource,
    _In_  const D3D11_TILED_RESOURCE_COORDINATE *pDestTileRegionStartCoordinate,
    _In_  const D3D11_TILE_REGION_SIZE *pDestTileRegionSize,
    _In_  const void *pSourceTileData,
    _In_  UINT Flags) override
  {
    assert (ver_ >= 2);

    return
      static_cast <ID3D11DeviceContext2 *>(pReal)->UpdateTiles (pDestTiledResource, pDestTileRegionStartCoordinate, pDestTileRegionSize, pSourceTileData, Flags);
  }
  HRESULT STDMETHODCALLTYPE ResizeTilePool (
    _In_  ID3D11Buffer *pTilePool,
    _In_  UINT64 NewSizeInBytes) override
  {
    assert (ver_ >= 2);

    return
      static_cast <ID3D11DeviceContext2 *>(pReal)->ResizeTilePool (pTilePool, NewSizeInBytes);
  }
  void STDMETHODCALLTYPE TiledResourceBarrier (
    _In_opt_  ID3D11DeviceChild *pTiledResourceOrViewAccessBeforeBarrier,
    _In_opt_  ID3D11DeviceChild *pTiledResourceOrViewAccessAfterBarrier) override
  {
    assert (ver_ >= 2);

    return
      static_cast <ID3D11DeviceContext2 *>(pReal)->TiledResourceBarrier (pTiledResourceOrViewAccessBeforeBarrier, pTiledResourceOrViewAccessAfterBarrier);
  }
  BOOL STDMETHODCALLTYPE IsAnnotationEnabled (void) override
  {
    assert (ver_ >= 2);

    return
      static_cast <ID3D11DeviceContext2 *>(pReal)->IsAnnotationEnabled ();
  }
  void STDMETHODCALLTYPE SetMarkerInt (
    _In_  LPCWSTR pLabel,
          INT     Data) override
  {
    assert (ver_ >= 2);

    return
      static_cast <ID3D11DeviceContext2 *>(pReal)->SetMarkerInt (pLabel, Data);
  }
  void STDMETHODCALLTYPE BeginEventInt (
    _In_  LPCWSTR pLabel,
          INT     Data) override
  {
    assert (ver_ >= 2);

    return
      static_cast <ID3D11DeviceContext2 *>(pReal)->BeginEventInt (pLabel, Data);
  }
  void STDMETHODCALLTYPE EndEvent (void) override
  {
    assert (ver_ >= 2);

    return
      static_cast <ID3D11DeviceContext2 *>(pReal)->EndEvent ();
  }



  void STDMETHODCALLTYPE Flush1 (
              D3D11_CONTEXT_TYPE ContextType,
    _In_opt_  HANDLE             hEvent) override
  {
    assert (ver_ >= 3);

    return
      static_cast <ID3D11DeviceContext3 *>(pReal)->Flush1 (ContextType, hEvent);
  }
  void STDMETHODCALLTYPE SetHardwareProtectionState (
    _In_  BOOL HwProtectionEnable) override
  {
    assert (ver_ >= 3);

    return
      static_cast <ID3D11DeviceContext3 *>(pReal)->SetHardwareProtectionState (HwProtectionEnable);
  }
  void STDMETHODCALLTYPE GetHardwareProtectionState (
    _Out_  BOOL *pHwProtectionEnable) override
  {
    assert (ver_ >= 3);

    return
      static_cast <ID3D11DeviceContext3 *>(pReal)->GetHardwareProtectionState (pHwProtectionEnable);
  }



  HRESULT STDMETHODCALLTYPE Signal (
    _In_  ID3D11Fence *pFence,
    _In_  UINT64       Value) override
  {
    assert (ver_ >= 4);

    return
      static_cast <ID3D11DeviceContext4 *>(pReal)->Signal (pFence, Value);
  }
  HRESULT STDMETHODCALLTYPE Wait (
    _In_  ID3D11Fence *pFence,
    _In_  UINT64       Value) override
  {
    assert (ver_ >= 4);

    return
      static_cast <ID3D11DeviceContext4 *>(pReal)->Wait (pFence, Value);
  }


protected:
private:
  volatile LONG        refs_ = 1;
           int         ver_  = 0;
  ID3D11DeviceContext* pReal;
};




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
  WaitForInitD3D11 ();

  static SK_RenderBackend_V2& rb =
    SK_GetCurrentRenderBackend ();

  // Even if the game doesn't care about the feature level, we do.
  D3D_FEATURE_LEVEL ret_level  = D3D_FEATURE_LEVEL_11_1;
  ID3D11Device*     ret_device = nullptr;

  // Allow override of swapchain parameters
  auto swap_chain_override =
    std::make_unique <DXGI_SWAP_CHAIN_DESC> ( pSwapChainDesc != nullptr ?
                                                *pSwapChainDesc :
                                                DXGI_SWAP_CHAIN_DESC { }
                                            );

  auto swap_chain_desc =
    swap_chain_override.get ();

  DXGI_LOG_CALL_1 (L"D3D11CreateDeviceAndSwapChain", L"Flags=0x%x", Flags );

  SK_D3D11_Init ();

  dll_log.LogEx ( true,
                    L"[  D3D 11  ]  <~> Preferred Feature Level(s): <%u> - %s\n",
                      FeatureLevels,
                        SK_DXGI_FeatureLevelsToStr (
                          FeatureLevels,
                            reinterpret_cast <const DWORD *> (pFeatureLevels)
                        ).c_str ()
                );

  // Optionally Enable Debug Layer
  if (ReadAcquire (&__d3d11_ready))
  {
    if (config.render.dxgi.debug_layer && (! (Flags & D3D11_CREATE_DEVICE_DEBUG)))
    {
      SK_LOG0 ( ( L" ==> Enabling D3D11 Debug layer" ),
                  __SK_SUBSYSTEM__ );
      Flags |= D3D11_CREATE_DEVICE_DEBUG;
    }
  }


  SK_D3D11_RemoveUndesirableFlags (Flags);


  //
  // DXGI Adapter Override (for performance)
  //

  SK_DXGI_AdapterOverride ( &pAdapter, &DriverType );

  if ( pSwapChainDesc != nullptr && 
          ppSwapChain != nullptr )
  {
    wchar_t wszMSAA [128] = { };

    _swprintf ( wszMSAA, swap_chain_desc->SampleDesc.Count > 1 ?
                           L"%u Samples" :
                           L"Not Used (or Offscreen)",
                  swap_chain_desc->SampleDesc.Count );

    dll_log.LogEx ( true,
      L"[Swap Chain]\n"
      L"  +-------------+-------------------------------------------------------------------------+\n"
      L"  | Resolution. |  %4lux%4lu @ %6.2f Hz%-50ws|\n"
      L"  | Format..... |  %-71ws|\n"
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
    SK_DXGI_FormatToStr (swap_chain_desc->BufferDesc.Format).c_str (),
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
                  L"(Requested: %s, Using: %s)",
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
  

  if ( SUCCEEDED (res)             &&
         ppSwapChain    != nullptr &&
         pSwapChainDesc != nullptr    )
  {
    wchar_t wszClass [MAX_PATH * 2] = { };

    RealGetWindowClassW (swap_chain_desc->OutputWindow, wszClass, MAX_PATH);

    const bool dummy_window = 
      StrStrIW (wszClass, L"Special K Dummy Window Class (Ex)") ||
      StrStrIW (wszClass, L"RTSSWndClass");

    extern void SK_DXGI_HookSwapChain (IDXGISwapChain* pSwapChain);
                SK_DXGI_HookSwapChain (*ppSwapChain);

    if (! dummy_window)
    {
      auto& windows =
        rb.windows;

      windows.setDevice (swap_chain_desc->OutputWindow);

      void
      SK_InstallWindowHook (HWND hWnd);
      SK_InstallWindowHook (swap_chain_desc->OutputWindow);

      if ( dwRenderThread == 0x00 ||
           dwRenderThread == GetCurrentThreadId () )
      {
        if (                windows.device != nullptr    &&
             swap_chain_desc->OutputWindow != nullptr    &&
             swap_chain_desc->OutputWindow != windows.device )
          SK_LOG0 ( (L"Game created a new window?!"), __SK_SUBSYSTEM__ );
      }

      // Assume the first thing to create a D3D11 render device is
      //   the game and that devices never migrate threads; for most games
      //     this assumption holds.
      if ( dwRenderThread == 0x00 ||
           dwRenderThread == GetCurrentThreadId () )
      {
        dwRenderThread = GetCurrentThreadId ();
      }

      SK_D3D11_SetDevice ( &ret_device, ret_level );
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
       options.MapNoOverwriteOnDynamicConstantBuffer;
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
  SK_TLS *pTLS =
    SK_TLS_Bottom ();
  
  if (pTLS->d3d11.skip_d3d11_create_device)
  {
    HRESULT hr =
      D3D11CreateDevice_Import ( pAdapter, DriverType, Software, Flags,
                                   pFeatureLevels, FeatureLevels, SDKVersion,
                                     ppDevice, pFeatureLevel,
                                       ppImmediateContext );

    pTLS->d3d11.skip_d3d11_create_device = false;

    return hr;
  }

  DXGI_LOG_CALL_1 (L"D3D11CreateDevice            ", L"Flags=0x%x", Flags);

  SK_D3D11_Init ();

  return
    D3D11CreateDeviceAndSwapChain_Detour ( pAdapter, DriverType, Software, Flags,
                                             pFeatureLevels, FeatureLevels, SDKVersion,
                                               nullptr, nullptr, ppDevice, pFeatureLevel,
                                                 ppImmediateContext );
}


__declspec (noinline)
HRESULT
STDMETHODCALLTYPE
D3D11Dev_CreateDeferredContext_Override (
  _In_            ID3D11Device         *This,
  _In_            UINT                  ContextFlags,
  _Out_opt_       ID3D11DeviceContext **ppDeferredContext )
{
  DXGI_LOG_CALL_1 (L"ID3D11Device::CreateDeferredContext", L"ContextFlags=0x%x", ContextFlags );

  if (ppDeferredContext != nullptr)
  {
    static Concurrency::concurrent_unordered_map
      <        ID3D11DeviceContext *,
        SK_IWrapD3D11DeviceContext * > wrapped_contexts;

          ID3D11DeviceContext* pTemp = nullptr;
    const HRESULT              hr    =
      D3D11Dev_CreateDeferredContext_Original (This, ContextFlags, &pTemp);

    if (SUCCEEDED (hr))
    {
      if (wrapped_contexts.count (pTemp) == 0)
        wrapped_contexts [pTemp] = new SK_IWrapD3D11DeviceContext (pTemp);
      
      *ppDeferredContext =
        dynamic_cast <ID3D11DeviceContext *> (wrapped_contexts [pTemp]);

      return hr;
    }

    *ppDeferredContext = pTemp;

    return hr;
  }

  else
    return D3D11Dev_CreateDeferredContext_Original (This, ContextFlags, nullptr);
}

#include <concurrent_unordered_map.h>

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

  ID3D11DeviceContext* pCtx = nullptr;

  D3D11Dev_GetImmediateContext_Original (This, &pCtx);

  *ppImmediateContext = pCtx;
}




SK_D3D11_KnownShaders_Singleton&
SK_D3D11_Shader_Lambda (void)
{
  extern SK_D3D11_KnownShaders_Singleton&
    __SK_Singleton_D3D11_Shaders (void);

  static
    SK_D3D11_KnownShaders_Singleton
    shaders (
      __SK_Singleton_D3D11_Shaders ()
    );

  return shaders;
};









static std::string
  _SpecialNowhere;

struct SK_HDR_FIXUP
{
  ID3D11Buffer*             mainSceneCBuffer = nullptr;
  ID3D11Buffer*                   hudCBuffer = nullptr;
  ID3D11Buffer*            colorSpaceCBuffer = nullptr;
                                  
  ID3D11SamplerState*              pSampler0 = nullptr;
                                   
  ID3D11ShaderResourceView*         pMainSrv = nullptr;
  ID3D11ShaderResourceView*          pHUDSrv = nullptr;
                                     
  ID3D11RenderTargetView*           pMainRtv = nullptr;
  ID3D11RenderTargetView*            pHUDRtv = nullptr;
                                    
  ID3D11RasterizerState*        pRasterState = nullptr;
  ID3D11DepthStencilState*          pDSState = nullptr;
                                  
  ID3D11BlendState*             pBlendState0 = nullptr;
  ID3D11BlendState*             pBlendState1 = nullptr;



#pragma pack (push)
#pragma pack (4)
  struct HDR_COLORSPACE_PARAMS
  {
    uint32_t visualFunc     [4]  = { 0, 0, 0, 0 };

    uint32_t uiColorSpaceIn      =   0;
    uint32_t uiColorSpaceOut     =   0;

    //uint32_t padding        [2]  = { 0, 0 };
    //float    more_padding   [3]  = { 0.f, 0.f, 0.f };

    BOOL     bUseSMPTE2084       =  FALSE;
    float    sdrLuminance_NonStd = 300.0_Nits;
  };

  //const int x = sizeof (HDR_COLORSPACE_PARAMS);

  struct HDR_LUMINANCE
  {
    // scRGB allows values > 1.0, sRGB (SDR) simply clamps them
    float luminance_scale [4]; // For HDR displays,    1.0 = 80 Nits
                               // For SDR displays, >= 1.0 = 80 Nits

    // (x,y): Main Scene  <Linear Scale, Gamma Exponent>
    //  z   : HUD          Linear Scale {Gamma is Implicitly 1/2.2}
    //  w   : Always 1.0
  };
#pragma pack (pop)


  //static ID3D11InputLayout*       g_pInputLayout          = nullptr;

  template <typename _ShaderType> 
  struct ShaderBase
  {
    _ShaderType *shader           = nullptr;
    ID3D10Blob  *amorphousTheBlob = nullptr;

    /////
    bool compileShaderString ( const char*    szShaderString,
                               const wchar_t* wszShaderName,
                               const char*    szEntryPoint,
                               const char*    szShaderModel,
                                     bool     recompile =
                                                false,
                                 std::string& error_log =
                                                _SpecialNowhere )
    {
      UNREFERENCED_PARAMETER (recompile);
      UNREFERENCED_PARAMETER (error_log);

      CComPtr <ID3D10Blob>
        amorphousTheMessenger;

      HRESULT hr =
        D3DCompile ( szShaderString,
                       strlen (szShaderString),
                         nullptr, nullptr, nullptr,
                           szEntryPoint, szShaderModel,
                             0, 0,
                               &amorphousTheBlob,
                                 &amorphousTheMessenger );

      if (FAILED (hr))
      {
        if ( amorphousTheMessenger != nullptr     &&
             amorphousTheMessenger->GetBufferSize () > 0 )
        {
          std::string err;
                      err.reserve (
                        amorphousTheMessenger->GetBufferSize ()
                      );
          err = (
            (char *)amorphousTheMessenger->GetBufferPointer ());

          if (! err.empty ())
          {
            dll_log.LogEx ( true,
                              L"SK D3D11 Shader (SM=%hs) [%ws]: %hs",
                                szShaderModel, wszShaderName,
                                  err.c_str ()
                          );
          }
        }

        return false;
      }

      HRESULT hrCompile =
        DXGI_ERROR_NOT_CURRENTLY_AVAILABLE;

      CComQIPtr <ID3D11Device> pDev (
        SK_GetCurrentRenderBackend ( ).device
                                    );

      if ( std::type_index ( typeid (    _ShaderType   ) ) ==
           std::type_index ( typeid (ID3D11VertexShader) ) )
      {
        hrCompile =
          pDev->CreateVertexShader (
             static_cast <DWORD *> ( amorphousTheBlob->GetBufferPointer () ),
                                     amorphousTheBlob->GetBufferSize    (),
                                       nullptr,
             reinterpret_cast <ID3D11VertexShader **>(&shader)
                                   );
      }

      else if ( std::type_index ( typeid (   _ShaderType   ) ) ==
                std::type_index ( typeid (ID3D11PixelShader) ) )
      {
        hrCompile =
          pDev->CreatePixelShader  (
             static_cast <DWORD *> ( amorphousTheBlob->GetBufferPointer () ),
                                     amorphousTheBlob->GetBufferSize    (),
                                       nullptr,
             reinterpret_cast <ID3D11PixelShader **>(&shader)
                                   );
      }

      else
      {
#pragma warning (push)
#pragma warning (disable: 4130) // No @#$% sherlock
        SK_ReleaseAssert ("WTF?!" == nullptr)
#pragma warning (pop)
      }

      return
        SUCCEEDED (hrCompile);
    }

    bool compileShaderFile ( const wchar_t* wszShaderFile,
                             const    char* szEntryPoint,
                             const    char* szShaderModel,
                                      bool  recompile =
                                              false,
                               std::string& error_log =
                                              _SpecialNowhere )
    {
      std::wstring wstr_file =
        wszShaderFile;

      SK_StripUserNameFromPathW (
        wstr_file.data ()
      );

      if ( GetFileAttributesW (wszShaderFile) == INVALID_FILE_ATTRIBUTES )
      {
        SK_LOG0 ( ( L"Shader Compile Failed: File '%ws' Is Not Valid!",
                      wstr_file.c_str ()
                  ),L"D3DCompile" );

        return false;
      }

      FILE* fShader =
        _wfsopen ( wszShaderFile, L"rtS",
                     _SH_DENYNO );

      if (fShader != nullptr)
      {
        size_t size =
          SK_File_GetSize (wszShaderFile);

        CHeapPtr <char> data;
                        data.AllocateBytes (size + 32);

        SecureZeroMemory (data.m_pData, size + 32);

        fread ( data.m_pData,
                           1, size + 2,
                              fShader );

        fclose (fShader);

        const bool result =
          compileShaderString ( data.m_pData, wstr_file.c_str (),
                                  szEntryPoint, szShaderModel,
                                    recompile,  error_log );

        return result;
      }

      SK_LOG0 ( (L"Cannot Compile Shader Because of Filesystem Issues"),
                 L"D3DCompile" );

      return  false;
    }

    void releaseResources (void)
    {
      if (shader           != nullptr) {           shader->Release (); shader           = nullptr; }
      if (amorphousTheBlob != nullptr) { amorphousTheBlob->Release (); amorphousTheBlob = nullptr; }
    }
  };
  
  enum SK_HDR_Type {
    HDR10       = 0x010,
    HDR10Plus   = 0x011,
    DolbyVision = 0x020,

    scRGB       = 0x100, // Not a real signal / data standard,
                         //   but a real useful colorspace even so.
  } __SK_HDR_Type;

 DXGI_FORMAT
 SK_HDR_GetPreferredDXGIFormat (void)
 {
   if (__SK_HDR_10BitSwap)
     __SK_HDR_Type = HDR10;
   else
     __SK_HDR_Type = scRGB;

   if (     __SK_HDR_Type == scRGB)
     return DXGI_FORMAT_R16G16B16A16_FLOAT;

   else if (__SK_HDR_Type &  HDR10)
     return DXGI_FORMAT_R10G10B10A2_UNORM;

   else
   {
     dll_log.Log (L"Unknown HDR Format, using R10G10B10A2 (HDR10-ish)");
     return DXGI_FORMAT_R10G10B10A2_UNORM;
   }
 }

  ShaderBase <ID3D11PixelShader>  PixelShader_HDR10;
  ShaderBase <ID3D11PixelShader>  PixelShader_scRGB;
  ShaderBase <ID3D11VertexShader> VertexShaderHDR_Util;

  void releaseResources (void)
  {
    if (mainSceneCBuffer  != nullptr)  { mainSceneCBuffer->Release  ();   mainSceneCBuffer = nullptr; }
    if (hudCBuffer        != nullptr)  { hudCBuffer->Release        ();         hudCBuffer = nullptr; }
    if (colorSpaceCBuffer != nullptr)  { colorSpaceCBuffer->Release ();  colorSpaceCBuffer = nullptr; }
                                                                             
    if (pSampler0         != nullptr)  { pSampler0->Release         ();          pSampler0 = nullptr; }
                                                                              
    if (pMainSrv          != nullptr)  { pMainSrv->Release          ();           pMainSrv = nullptr; }
    if (pHUDSrv           != nullptr)  { pHUDSrv->Release           ();            pHUDSrv = nullptr; }
                                                                               
    if (pMainRtv          != nullptr)  { pMainRtv->Release          ();           pMainRtv = nullptr; }
    if (pHUDRtv           != nullptr)  { pHUDRtv->Release           ();            pHUDRtv = nullptr; }
                                                                             
    if (pRasterState      != nullptr)  { pRasterState->Release      ();       pRasterState = nullptr; }
    if (pDSState          != nullptr)  { pDSState->Release          ();           pDSState = nullptr; }
                                                                             
    if (pBlendState0      != nullptr)  { pBlendState0->Release      ();       pBlendState0 = nullptr; }
    if (pBlendState1      != nullptr)  { pBlendState1->Release      ();       pBlendState1 = nullptr; }

    PixelShader_HDR10.releaseResources    ();
    PixelShader_scRGB.releaseResources    ();
    VertexShaderHDR_Util.releaseResources ();
  }

  bool
  recompileShaders (void)
  {
    std::wstring debug_shader_dir = SK_GetConfigPath ();
                 debug_shader_dir +=
            LR"(SK_Res\Debug\shaders\)";
 
    bool   ret =
    PixelShader_scRGB.compileShaderFile   (
      std::wstring (debug_shader_dir  +
      LR"(HDR\scRGB\Rec709_Linear_16bit-fp.hlsl)").c_str (),
        "main",  "ps_5_0", true );

    ret &=
      PixelShader_HDR10.compileShaderFile  (
        std::wstring (debug_shader_dir  +
        LR"(HDR\HDR10\Rec2020_PQ_10bit-fixed.hlsl)").c_str (),
        "main", "ps_5_0", true );

    ret &=
      VertexShaderHDR_Util.compileShaderFile  (
        std::wstring (debug_shader_dir  +
          LR"(HDR\vs_colorutil.hlsl)").c_str (),
          "main", "vs_5_0", true );

    return ret;
  }

  void
  reloadResources (void)
  {
    if (mainSceneCBuffer  != nullptr) { mainSceneCBuffer->Release  ();   mainSceneCBuffer = nullptr; }
    if (colorSpaceCBuffer != nullptr) { colorSpaceCBuffer->Release ();  colorSpaceCBuffer = nullptr; }
    ////if (hudCBuffer       == nullptr)  { hudCBuffer->Release       ();        hudCBuffer = nullptr; }

    if (pSampler0        != nullptr)  { pSampler0->Release        ();         pSampler0 = nullptr; }

    if (pMainSrv         != nullptr)  { pMainSrv->Release         ();          pMainSrv = nullptr; }
    if (pHUDSrv          != nullptr)  { pHUDSrv->Release          ();           pHUDSrv = nullptr; }

    if (pMainRtv         != nullptr)  { pMainRtv->Release         ();          pMainRtv = nullptr; }
    if (pHUDRtv          != nullptr)  { pHUDRtv->Release          ();           pHUDRtv = nullptr; }

    if (pRasterState     != nullptr)  { pRasterState->Release     ();      pRasterState = nullptr; }
    if (pDSState         != nullptr)  { pDSState->Release         ();          pDSState = nullptr; }

    if (pBlendState0     != nullptr)  { pBlendState0->Release     ();      pBlendState0 = nullptr; }
    if (pBlendState1     != nullptr)  { pBlendState1->Release     ();      pBlendState1 = nullptr; }

    static auto& rb =
      SK_GetCurrentRenderBackend ();

    CComQIPtr <IDXGISwapChain>      pSwapChain (rb.swapchain);
    CComQIPtr <ID3D11Device>        pDev       (rb.device);
    CComQIPtr <ID3D11DeviceContext> pDevCtx    (rb.d3d11.immediate_ctx);

    if (pDev != nullptr)
    {
      recompileShaders ();

      D3D11_BUFFER_DESC desc = { };

      desc.ByteWidth         = sizeof (HDR_LUMINANCE);
      desc.Usage             = D3D11_USAGE_DYNAMIC;
      desc.BindFlags         = D3D11_BIND_CONSTANT_BUFFER;
      desc.CPUAccessFlags    = D3D11_CPU_ACCESS_WRITE;
      desc.MiscFlags         = 0;

      pDev->CreateBuffer (&desc, nullptr, &mainSceneCBuffer);

      desc.ByteWidth         = sizeof (HDR_COLORSPACE_PARAMS);
      desc.Usage             = D3D11_USAGE_DYNAMIC;
      desc.BindFlags         = D3D11_BIND_CONSTANT_BUFFER;
      desc.CPUAccessFlags    = D3D11_CPU_ACCESS_WRITE;
      desc.MiscFlags         = 0;

      pDev->CreateBuffer (&desc, nullptr, &colorSpaceCBuffer);
    }

    if ( hdr_base.pMainSrv == nullptr &&
                pSwapChain != nullptr )
    {
      DXGI_SWAP_CHAIN_DESC swapDesc = { };
      D3D11_TEXTURE2D_DESC desc     = { };

      pSwapChain->GetDesc (&swapDesc);

      desc.Width            = swapDesc.BufferDesc.Width;
      desc.Height           = swapDesc.BufferDesc.Height;
      desc.MipLevels        = 1;
      desc.ArraySize        = 1;
      desc.Format           = SK_HDR_GetPreferredDXGIFormat ();
      desc.SampleDesc.Count = 1; // Will probably regret this if HDR ever procreates with MSAA
      desc.Usage            = D3D11_USAGE_DEFAULT;
      desc.BindFlags        = D3D11_BIND_SHADER_RESOURCE;
      desc.CPUAccessFlags   = 0;

      CComPtr <ID3D11Texture2D> pHDRTexture = nullptr;

      pDev->CreateTexture2D          (&desc,       nullptr, &pHDRTexture.p);
      pDev->CreateShaderResourceView (pHDRTexture, nullptr, &hdr_base.pMainSrv);

      desc = { };
      desc.Usage               = D3D11_USAGE_DEFAULT;
      desc.BindFlags           = D3D11_BIND_RENDER_TARGET |
                                 D3D11_BIND_SHADER_RESOURCE;
      desc.ArraySize           = 1;
      desc.CPUAccessFlags      = 0;

      desc.Format              = SK_HDR_GetPreferredDXGIFormat ();
      desc.Width               = swapDesc.BufferDesc.Width;
      desc.Height              = swapDesc.BufferDesc.Height;
      desc.MipLevels           = 1;
      desc.SampleDesc.Count    = 1;
      desc.SampleDesc.Quality  = 0;

      CComPtr <ID3D11Texture2D> pHDRHUDTexture = nullptr;

      if (SUCCEEDED (pDev->CreateTexture2D (&desc, nullptr, &pHDRHUDTexture.p)))
      {
      //dll_log.Log (L"Creating RTV and SRV for HUD");

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = { };
        D3D11_RENDER_TARGET_VIEW_DESC   rtvDesc = { };

        srvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Format                    = SK_HDR_GetPreferredDXGIFormat ();
        srvDesc.Texture2D.MipLevels       = desc.MipLevels;
        srvDesc.Texture2D.MostDetailedMip = 0;

        rtvDesc.ViewDimension             = D3D11_RTV_DIMENSION_TEXTURE2D;
        rtvDesc.Texture2D.MipSlice        = 0;

        pDev->CreateRenderTargetView   (pHDRHUDTexture, &rtvDesc, &hdr_base.pHUDRtv);
        pDev->CreateShaderResourceView (pHDRHUDTexture, &srvDesc, &hdr_base.pHUDSrv);
      }
    }
  }
} hdr_base;


int   __SK_HDR_input_gamut   = 0;
int   __SK_HDR_output_gamut  = 0;
int   __SK_HDR_visualization = 0;
float __SK_HDR_user_sdr_Y    = 300.0f;


void
SK_HDR_ReleaseResources (void)
{
  hdr_base.releaseResources ();
}

void
SK_HDR_InitResources (void)
{
  hdr_base.reloadResources ();
}

void
SK_HDR_SnapshotSwapchain (void)
{
  static LONG lLastFrame =
    SK_GetFramesDrawn () - 1;

  LONG lThisFrame = SK_GetFramesDrawn ();
  if  (lThisFrame == lLastFrame) { return; }
  else                           { lLastFrame = lThisFrame; }

  if ( hdr_base.VertexShaderHDR_Util.shader == nullptr ||
       hdr_base.PixelShader_scRGB.shader    == nullptr ||
       hdr_base.PixelShader_HDR10.shader    == nullptr )
  {
    hdr_base.reloadResources ();
  }

  static auto& rb =
    SK_GetCurrentRenderBackend ();

  CComQIPtr <IDXGISwapChain>      pSwapChain (rb.swapchain);
  CComQIPtr <ID3D11Device>        pDev       (rb.device);
  CComQIPtr <ID3D11DeviceContext> pDevCtx    (rb.d3d11.immediate_ctx);

  CComPtr <ID3D11Resource> pSrc = nullptr;
  CComPtr <ID3D11Resource> pDst = nullptr;

  if (pSwapChain != nullptr)
  {
    if ( SUCCEEDED (
           pSwapChain->GetBuffer    (
             0,
               IID_ID3D11Texture2D,
                 (void **) &pSrc.p  )
                   )
       )
    {
      hdr_base.pMainSrv->GetResource (&pDst.p);
                pDevCtx->CopyResource (pDst, pSrc);
    }
  }

  if ( hdr_base.VertexShaderHDR_Util.shader != nullptr &&
       hdr_base.PixelShader_scRGB.shader    != nullptr &&
       hdr_base.PixelShader_HDR10.shader    != nullptr    )
  {
    D3D11_MAPPED_SUBRESOURCE mapped_resource = { };

    if ( pDevCtx->Map ( hdr_base.mainSceneCBuffer,
                          0, D3D11_MAP_WRITE_DISCARD, 0,
                             &mapped_resource ) != S_OK )
    {
      return;
    }

    auto* constant_buffer =
      static_cast <SK_HDR_FIXUP::HDR_LUMINANCE *> (mapped_resource.pData);

    bool hdr_display =
      (rb.isHDRCapable () && (rb.framebuffer_flags & SK_FRAMEBUFFER_FLAG_HDR));

    if (! hdr_display)
    {
      constant_buffer->luminance_scale [0] = 1.0f; constant_buffer->luminance_scale [1] = 1.0f;
      constant_buffer->luminance_scale [2] = 0.0f; constant_buffer->luminance_scale [3] = 0.0f;
    }

    else
    {
      extern float __SK_MHW_HDR_Luma; extern float __SK_HDR_Luma;
      extern float __SK_MHW_HDR_Exp;  extern float __SK_HDR_Exp;

      float luma = 0.0f,
        exp  = 0.0f;

      switch (SK_GetCurrentGameID ())
      {
        case SK_GAME_ID::MonsterHunterWorld:
          luma = __SK_MHW_HDR_Luma;
          exp  = __SK_MHW_HDR_Exp;
          break;

        case SK_GAME_ID::DragonQuestXI:
        default:
          luma = __SK_HDR_Luma;
          exp  = __SK_HDR_Exp;
          break;
      }

      constant_buffer->luminance_scale [0] = luma;// rb.ui_luminance;
      constant_buffer->luminance_scale [1] = exp; // rb.ui_srgb ? 2.2f : 1.0f;
      constant_buffer->luminance_scale [2] = (__SK_HDR_HorizCoverage / 100.0f) * 2.0f - 1.0f;
      constant_buffer->luminance_scale [3] = (__SK_HDR_VertCoverage  / 100.0f) * 2.0f - 1.0f;
    }

    pDevCtx->Unmap (hdr_base.mainSceneCBuffer, 0);


    if ( pDevCtx->Map ( hdr_base.colorSpaceCBuffer,
                          0, D3D11_MAP_WRITE_DISCARD, 0,
                            &mapped_resource ) != S_OK )
    {
      return;
    }

    auto* cbuffer_cspace =
      static_cast <SK_HDR_FIXUP::HDR_COLORSPACE_PARAMS *> (mapped_resource.pData);

    if (! hdr_display)
    {
      *cbuffer_cspace = { };
    }

    else
    {
      cbuffer_cspace->uiColorSpaceIn      =   __SK_HDR_input_gamut;
      cbuffer_cspace->uiColorSpaceOut     =   __SK_HDR_output_gamut;
      cbuffer_cspace->sdrLuminance_NonStd =   __SK_HDR_user_sdr_Y * 1.0_Nits;

      cbuffer_cspace->visualFunc [0] = (uint32_t)__SK_HDR_visualization;
      cbuffer_cspace->visualFunc [1] = (uint32_t)__SK_HDR_visualization;
      cbuffer_cspace->visualFunc [2] = (uint32_t)__SK_HDR_visualization;
      cbuffer_cspace->visualFunc [3] = (uint32_t)__SK_HDR_visualization;
    }

    pDevCtx->Unmap (hdr_base.colorSpaceCBuffer, 0);

    SK_D3D11_Stateblock_Lite*
    SK_D3D11_CreateAndCaptureStateBlock (ID3D11DeviceContext* pImmediateContext);

    void
    SK_D3D11_ReleaseAndApplyStateBlock ( SK_D3D11_Stateblock_Lite* pBlock,
                                         ID3D11DeviceContext*      pDevCtx );

    SK_D3D11_Stateblock_Lite* pStateBlock =
      SK_D3D11_CreateAndCaptureStateBlock (pDevCtx);

    D3D11_PRIMITIVE_TOPOLOGY     OrigPrimTop;
    CComPtr <ID3D11VertexShader> pVS_Orig;
    CComPtr <ID3D11PixelShader>  pPS_Orig;
    CComPtr <ID3D11BlendState>   pBlendState_Orig;
                            UINT uiOrigBlendMask;
                           FLOAT fOrigBlendFactors [4] = { };
    CComPtr <ID3D11Buffer>       pConstantBufferVS_Orig;
    CComPtr <ID3D11Buffer>       pConstantBufferPS_Orig;

    pDevCtx->VSGetConstantBuffers (0,                   1,        &pConstantBufferVS_Orig.p);
    pDevCtx->PSGetConstantBuffers (0,                   1,        &pConstantBufferPS_Orig.p);
    pDevCtx->OMGetBlendState      (&pBlendState_Orig.p, fOrigBlendFactors, &uiOrigBlendMask);
    pDevCtx->VSGetShader          (&pVS_Orig.p,         nullptr,                    nullptr);
    pDevCtx->PSGetShader          (&pPS_Orig.p,         nullptr,                    nullptr);

    ID3D11ShaderResourceView* pOrigResources [2] = { };
    ID3D11ShaderResourceView* pResources     [2] = {
      hdr_base.pMainSrv,    hdr_base.pHUDSrv     };

    pDevCtx->PSGetShaderResources (0, 2, pOrigResources);

    pDevCtx->IAGetPrimitiveTopology (&OrigPrimTop);
    pDevCtx->IASetPrimitiveTopology (D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    static const FLOAT                      fBlendFactor [4] =
                                      { 0.0f, 0.0f, 0.0f, 0.0f };
    pDevCtx->OMSetBlendState      (nullptr, fBlendFactor,           0xFFFFFFFF);
    pDevCtx->VSSetConstantBuffers (0,            1, &hdr_base.mainSceneCBuffer);

    pDevCtx->VSSetShader          (hdr_base.VertexShaderHDR_Util.shader, nullptr, 0);
    pDevCtx->PSSetConstantBuffers (0,            1, &hdr_base.colorSpaceCBuffer);

    if ( rb.scanout.dwm_colorspace  ==
         rb.scanout.dxgi_colorspace || 
         rb.scanout.dxgi_colorspace != DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709 )
    {
      pDevCtx->PSSetShader          (hdr_base.PixelShader_HDR10.shader, nullptr, 0);
    }

    else
    {
      pDevCtx->PSSetShader          (hdr_base.PixelShader_scRGB.shader, nullptr, 0);
    }

    pDevCtx->PSSetShaderResources (0,                                 2, pResources);

    D3D11_RENDER_TARGET_VIEW_DESC rtdesc
      = { };

    rtdesc.Format        = DXGI_FORMAT_R16G16B16A16_FLOAT;
    rtdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

    CComPtr <ID3D11RenderTargetView> pRenderTargetView = nullptr;

    pDev->CreateRenderTargetView (pSrc, &rtdesc, &pRenderTargetView);
    pDevCtx->OMSetRenderTargets  ( 1,
                                     &pRenderTargetView.p,
                                       nullptr );

    pDevCtx->Draw (4, 0);

    pDevCtx->PSSetShaderResources (0,                2,                 pOrigResources);
                                 if (pOrigResources [0] != nullptr)     pOrigResources [0]->Release ();
                                 if (pOrigResources [1] != nullptr)     pOrigResources [1]->Release ();

    pDevCtx->IASetPrimitiveTopology (OrigPrimTop);
    pDevCtx->PSSetShader            (pPS_Orig,         nullptr,           0);
    pDevCtx->VSSetShader            (pVS_Orig,         nullptr,           0);
    pDevCtx->VSSetConstantBuffers   (0,                1,                 &pConstantBufferVS_Orig.p);
    pDevCtx->PSSetConstantBuffers   (0,                1,                 &pConstantBufferPS_Orig.p);
    pDevCtx->OMSetBlendState        (pBlendState_Orig, fOrigBlendFactors, uiOrigBlendMask);

    SK_D3D11_ReleaseAndApplyStateBlock (pStateBlock, pDevCtx);
  }
}

void
SK_HDR_CombineSceneWithHUD (void)
{
}

bool
SK_HDR_RecompileShaders (void)
{
  return
    hdr_base.recompileShaders ();
}

ID3D11ShaderResourceView*
SK_HDR_GetUnderlayResourceView (void)
{
  return
    hdr_base.pMainSrv;
}